package main

import (
	"context"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"html/template"
	"intrinsic/util/proto/protoio"
	"io"
	"log"
	"net/http"
	"os"
	"path"
	"strings"

	btpb "intrinsic/executive/proto/behavior_tree_go_proto"
	eempb "intrinsic/executive/proto/executive_execution_mode_go_proto"
	esvcgrpcpb "intrinsic/executive/proto/executive_service_go_grpc_proto"
	rmpb "intrinsic/executive/proto/run_metadata_go_proto"
	rcpb "intrinsic/resources/proto/runtime_context_go_proto"

	lropb "cloud.google.com/go/longrunning/autogen/longrunningpb"
	"github.com/bazelbuild/rules_go/go/tools/bazel"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/protobuf/proto"
)

const (
	packageDir    = "hmi"
	indexFilePath = packageDir + "/frontend/index.html"
	processesDir  = packageDir + "/processes"

	// defaultRuntimeContextPath is the well-known location for the runtime config
	// inside on-prem devices.
	defaultRuntimeContextPath = "/etc/intrinsic/runtime_config.pb"
	// defaultBaseURLFormat will work correctly when the on-prem device is reached
	// at the root path in a browser (most common). It is used to populate a
	// <base> tag in HTML. This ensures that relative URLs are resolved correctly.
	defaultBaseURLFormat      = "/ext/services/%s/"
)

// Providing these values as flags allows for local testing. The README
// describes how this works in more detail.
var (
	runtimeContextPath = flag.String("runtime_context_path", defaultRuntimeContextPath, "Path to the runtime context binary proto.")
	baseURLFormat      = flag.String("base_url_fmt", defaultBaseURLFormat, "Go format string for the base URL of the HMI. Supports exactly one %s for the name of the service. Must end with a trailing slash.")
	ingressAddress     = flag.String("ingress_address", "istio-ingressgateway.app-ingress.svc.cluster.local:80", "Address used to connect to services.")
)

// executiveClient wraps a client for the executive gRPC service to provide
// functionalities involving the executive.
type executiveClient struct {
	cl esvcgrpcpb.ExecutiveServiceClient
}

// executiveStatus contains information about the current execution status of
// the executive. Can be safely marshalled to JSON.
type executiveStatus struct {
	done        bool
	operationID string
	state       btpb.BehaviorTree_State
	error       string
}

// MarshalJSON enables marshalling [executiveStatus] to JSON.
func (r *executiveStatus) MarshalJSON() ([]byte, error) {
	return json.Marshal(&struct {
		Done        bool   `json:"done"`
		OperationID string `json:"operationId"`
		Status      string `json:"status"`
		Error       string `json:"error"`
	}{
		Done:        r.done,
		OperationID: r.operationID,
		Status:      btpb.BehaviorTree_State_name[int32(r.state)],
		Error:       r.error,
	})
}

// currentOperation returns the current operation of the executive. Returns nil
// and **no error** if there is no current operation.
func (e *executiveClient) currentOperation(ctx context.Context) (*lropb.Operation, error) {
	res, err := e.cl.ListOperations(ctx, &lropb.ListOperationsRequest{})
	if err != nil {
		return nil, fmt.Errorf("could not list operations: %v", err)
	}
	// Not having an operation indicated that the executive has not run yet. This
	// is not an error but we also can't return anything.
	if len(res.GetOperations()) == 0 {
		return nil, nil
	}
	return res.GetOperations()[0], nil
}

// status returns the execution status of the executive.
func (e *executiveClient) status(ctx context.Context) (*executiveStatus, error) {
	op, err := e.currentOperation(ctx)
	if err != nil {
		return nil, fmt.Errorf("could not get current operation: %v", err)
	}
	// The current operation can be nil. Return an empty (default) executive
	// status in this case.
	if op == nil {
		return &executiveStatus{}, nil
	}
	m := new(rmpb.RunMetadata)
	if err := op.GetMetadata().UnmarshalTo(m); err != nil {
		return nil, fmt.Errorf("could not unmarshal run metadata: %v", err)
	}
	res := &executiveStatus{
		done:        op.GetDone(),
		operationID: op.GetName(),
		state:       m.GetBehaviorTreeState(),
	}
	if op.GetError() != nil {
		res.error = op.GetError().GetMessage()
	}
	return res, nil
}

// stop cancels execution of the operation with the given ID.
func (e *executiveClient) stop(ctx context.Context, id string) error {
	if _, err := e.cl.CancelOperation(ctx, &lropb.CancelOperationRequest{
		Name: id,
	}); err != nil {
		return fmt.Errorf("could not stop operation: %w", err)
	}
	return nil
}

// executiveStart contains information about a started operation. Can be safely
// marshalled to JSON.
type executiveStart struct {
	operationID string
}

// MarshalJSON enables marshalling [executiveStatus] to JSON.
func (r *executiveStart) MarshalJSON() ([]byte, error) {
	return json.Marshal(&struct {
		OperationID string `json:"operationId"`
	}{
		OperationID: r.operationID,
	})
}

// start executes the given behavior tree in the executive.
func (e *executiveClient) start(ctx context.Context, bt *btpb.BehaviorTree) (*executiveStart, error) {
	currentOp, err := e.currentOperation(ctx)
	if err != nil {
		return nil, fmt.Errorf("could not get current operation: %v", err)
	}
	// If there is a current operation it must be deleted before starting a new
	// operation. This is a requirement of the executive service.
	if currentOp != nil {
		if _, err := e.cl.DeleteOperation(ctx, &lropb.DeleteOperationRequest{
			Name: currentOp.GetName(),
		}); err != nil {
			return nil, fmt.Errorf("could not delete old operation: %w", err)
		}
	}
	// Create a new operation with the given behavior tree. This essentially gets
	// the executive ready but does not start execution yet.
	res, err := e.cl.CreateOperation(ctx, &esvcgrpcpb.CreateOperationRequest{
		RunnableType: &esvcgrpcpb.CreateOperationRequest_BehaviorTree{
			BehaviorTree: bt,
		},
	})
	if err != nil {
		return nil, fmt.Errorf("could not create operation: %w", err)
	}
	// Start the newly created operation. This will actually start the execution
	// of the behavior tree.
	res, err = e.cl.StartOperation(ctx, &esvcgrpcpb.StartOperationRequest{
		Name:           res.GetName(),
		// These could be made configurable as a new functionality. The execution
		// mode enables step-wise execution and the simulation mode could be
		// switched to physics based simulation if needed.
		ExecutionMode:  eempb.ExecutionMode_EXECUTION_MODE_NORMAL,
		SimulationMode: eempb.SimulationMode_SIMULATION_MODE_DRAFT,
	})
	if err != nil {
		return nil, fmt.Errorf("could not start operation: %w", err)
	}
	return &executiveStart{
		operationID: res.GetName(),
	}, nil
}

// serve encapsulates the HTTP server providing the frontend and API for the
// HMI service.
type server struct {
	baseURL       string
	indexTemplate *template.Template
	executive     *executiveClient
	processes     map[string]*btpb.BehaviorTree
}

// start runs the HTTP server at the given address.
func (s *server) start(address string) error {
	// Serve the index file at the root of the HTTP server. Browsers will open
	// this path by default. Due to the way Golang handles HTTP requests this
	// will also serve the index file on any route that is not other specified
	// below (more specific).
	http.HandleFunc("/", s.index)

	// API handlers for the executive based functionality.
	http.HandleFunc("GET /api/executive/status", s.executiveStatus)
	http.HandleFunc("POST /api/executive/{id}/stop", s.executiveStop)
	http.HandleFunc("POST /api/executive/start", s.executiveStart)

	log.Printf("Listening at %s", address)
	return http.ListenAndServe(address, nil)
}

// index serves the index HTML by compiling the index template.
func (s *server) index(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	if err := s.indexTemplate.Execute(w, map[string]any{
		// The base URL is used to ensure relative URLs are correctly resolved.
		"BaseURL": s.baseURL,
	}); err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		writeJSON(w, wrapErr(fmt.Errorf("failed to execute index template: %w", err)))
	}
}

// executiveStatus serves a handler that returns the current execution status of
// the executive.
func (s *server) executiveStatus(w http.ResponseWriter, r *http.Request) {
	status, err := s.executive.status(r.Context())
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		writeJSON(w, wrapErr(fmt.Errorf("could not get executive status: %w", err)))
		return
	}
	writeJSON(w, status)
}

// executiveStop stops the executive operation with the ID in the path.
func (s *server) executiveStop(w http.ResponseWriter, r *http.Request) {
	id := r.PathValue("id")
	if id == "" {
		w.WriteHeader(http.StatusBadRequest)
		writeJSON(w, wrapErr(errors.New("missing operation ID in path")))
		return
	}
	if err := s.executive.stop(r.Context(), id); err != nil {
		writeJSON(w, wrapErr(fmt.Errorf("could not stop execution: %w", err)))
		w.WriteHeader(http.StatusInternalServerError)
		return
	}
}

// executiveStartRequest contains necessary information to executing a process.
type executiveStartRequest struct {
	ProcessName string `json:"processName"`
}

// executiveStart starts a pre-defined process with a name from the request.
// Processes are loaded into the [server] from files.
func (s *server) executiveStart(w http.ResponseWriter, r *http.Request) {
	rawReq, err := io.ReadAll(r.Body)
	if err != nil {
		w.WriteHeader(http.StatusBadRequest)
		writeJSON(w, wrapErr(fmt.Errorf("could not read request body: %v", err)))
		return
	}
	req := new(executiveStartRequest)
	if err := json.Unmarshal(rawReq, req); err != nil {
		w.WriteHeader(http.StatusBadRequest)
		writeJSON(w, wrapErr(fmt.Errorf("could not parse request body: %v", err)))
		return
	}
	if req.ProcessName == "" {
		w.WriteHeader(http.StatusBadRequest)
		writeJSON(w, wrapErr(errors.New("processName must not be empty")))
		return
	}
	bt, ok := s.processes[req.ProcessName]
	if !ok {
		w.WriteHeader(http.StatusBadRequest)
		writeJSON(w, wrapErr(fmt.Errorf("process %q not found", req.ProcessName)))
		return
	}
	res, err := s.executive.start(r.Context(), bt)
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		writeJSON(w, wrapErr(fmt.Errorf("could not stop execution: %w", err)))
		return
	}
	writeJSON(w, res)
}

func main() {
	// Parse the flag definitions at the top from the command line. This enables
	// local testing scenarios.
	flag.Parse()

	// The file root is different when running the executable in the service
	// container vs. running locally with Bazel. When running with Bazel all files
	// will be provided as runfiles and placed in a specific runfiles directory.
	// This directory has the same layout as the container image but with a
	// different base path.
	filesRoot, err := bazel.RunfilesPath()
	if err != nil {
		// Runfiles are placed relative to the container root when running there.
		filesRoot = "/"
	}

	rc := new(rcpb.RuntimeContext)
	// Allow reading runtime context from a textproto for local testing. Simply
	// matching the two common textproto extensions is sufficient for this simple
	// case. If the suffix is anything else this assumes the file is binary.
	if strings.HasSuffix(*runtimeContextPath, ".txtpb") || strings.HasSuffix(*runtimeContextPath, ".textproto") {
		if err := protoio.ReadTextProto(*runtimeContextPath, rc); err != nil {
			log.Fatalf("Failed to read runtime context: %v", err)
		}
	} else {
		if err := protoio.ReadBinaryProto(*runtimeContextPath, rc); err != nil {
			log.Fatalf("Failed to read runtime context: %v", err)
		}
	}
	address := fmt.Sprintf(":%d", rc.GetHttpPort())
	baseURL := fmt.Sprintf(*baseURLFormat, rc.GetName())

	// The HMI allows starting pre-specified processes using a name. These
	// processes are read from the file system and kept in memory. Each one is
	// a binary encoded behavior tree proto. More processes can be added. Simply
	// create the process in Flowstate and select `Process` > `Export` from the
	// menu bar. Then add the resulting file to the `hmi/processes` directory.
	fullProcessesDir := path.Join(filesRoot, processesDir)
	processEntries, err := os.ReadDir(fullProcessesDir)
	if err != nil {
		log.Fatalf("Failed to read static processes: %v", err)
	}
	processes := make(map[string]*btpb.BehaviorTree)
	for _, entry := range processEntries {
		f, err := os.ReadFile(path.Join(fullProcessesDir, entry.Name()))
		if err != nil {
			log.Fatalf("Failed to read static process file: %v", err)
		}
		bt := new(btpb.BehaviorTree)
		if err := proto.Unmarshal(f, bt); err != nil {
			log.Fatalf("Failed to unmarshal process: %v", err)
		}
		processes[entry.Name()] = bt
	}

	// Read the `index.html` as a template. This is done specifically to allow
	// writing a base URL to the HTML file at serve-time for better path
	// resolution.
	indexTemplate, err := template.ParseFiles(path.Join(filesRoot, indexFilePath))
	if err != nil {
		log.Fatalf("Failed to parse index template: %v", err)
	}

	// Connect to the ingress of the on-prem device.
	conn, err := grpc.NewClient(*ingressAddress, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		log.Fatalf("Failed to dial executive service: %v", err)
	}

	server := &server{
		baseURL:       baseURL,
		indexTemplate: indexTemplate,
		executive: &executiveClient{
			// Create a gRPC client for the executive service.
			cl: esvcgrpcpb.NewExecutiveServiceClient(conn),
		},
		processes: processes,
	}

	if err := server.start(address, indexFilePath); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
}

// errorResponse is a simple wrapper for an error that can be marshalled to
// JSON for responses.
type errorResponse struct {
	err error
}

// wrapErr wraps an error into an [errorResponse].
func wrapErr(err error) *errorResponse {
	return &errorResponse{
		err: err,
	}
}

// MarshalJSON enables marshalling [errorResponse] to JSON.
func (r *errorResponse) MarshalJSON() ([]byte, error) {
	return json.Marshal(&struct {
		Error string `json:"error"`
	}{
		Error: r.err.Error(),
	})
}

// writeJSON writes any value to the response writer as JSON if possible.
func writeJSON(w http.ResponseWriter, v any) {
	b, err := json.Marshal(v)
	if err != nil {
		// If marshalling to JSON fails we can't return JSON. This should
		// essentially never happen and is merely a fallback. "Real" errors from
		// handlers will be returned as JSON through the [errorResponse].
		w.Header().Set("content-type", "text/html")
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte(fmt.Sprintf("could not marshal error: %w", err)))
		return
	}
	w.Header().Set("content-type", "application/json")
	w.Write(b)
}

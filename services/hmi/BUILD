load("@ai_intrinsic_sdks//bazel:go_macros.bzl", "go_binary")
load("@ai_intrinsic_sdks//intrinsic/assets/services/build_defs:services.bzl", "intrinsic_service")
load("@rules_oci//oci:defs.bzl", "oci_image", "oci_load")
load("@rules_pkg//:pkg.bzl", "pkg_tar")
load("@container_structure_test//:defs.bzl", "container_structure_test")

filegroup(
    name = "frontend_files",
    srcs = glob(["frontend/**"]),
)

filegroup(
    name = "process_files",
    srcs = glob(["processes/**"]),
)

go_binary(
    name = "server",
    srcs = ["server.go"],
    data = [":frontend_files", ":process_files"],
    deps = [
        "@ai_intrinsic_sdks//intrinsic/executive/proto:executive_service_go_grpc_proto",
        "@ai_intrinsic_sdks//intrinsic/executive/proto:executive_service_go_proto",
        "@ai_intrinsic_sdks//intrinsic/executive/proto:behavior_tree_go_proto",
        "@ai_intrinsic_sdks//intrinsic/executive/proto:executive_execution_mode_go_proto",
        "@ai_intrinsic_sdks//intrinsic/executive/proto:run_metadata_go_proto",
        "@ai_intrinsic_sdks//intrinsic/resources/proto:runtime_context_go_proto",
        "@ai_intrinsic_sdks//intrinsic/util/proto:protoio",
        "@io_bazel_rules_go//go/runfiles",
        "@io_bazel_rules_go//go/tools/bazel",
        "@org_golang_google_grpc//:go_default_library",
        "@org_golang_google_grpc//credentials/insecure",
        "@org_golang_google_protobuf//encoding/protojson",
        "@org_golang_google_protobuf//proto",
        "@com_google_cloud_go_longrunning//autogen/longrunningpb",
    ],
)

pkg_tar(
    name = "server_layer",
    srcs = [":server"],
    strip_prefix = "/",
    include_runfiles = True,
    extension = "tar.gz",
)

oci_image(
    name = "hmi_image",
    base = "@distroless_base",
    tars = [":server_layer"],
    entrypoint = ["/services/hmi/server_/server"],
)

oci_load(
    name = "hmi_tarball",
    image = ":hmi_image",
    repo_tags = ["hmi:latest"],
)

filegroup(
    name = "hmi_tarball.tar",
    srcs = [":hmi_tarball"],
    output_group = "tarball",
)

container_structure_test(
    name = "hmi_tarball_test",
    driver = "tar",
    configs = ["testdata/image_files.yaml"],
    image = ":hmi_tarball.tar",
)

intrinsic_service(
    name = "hmi_service",
    images = [":hmi_tarball.tar"],
    manifest = "manifest.textproto",
)

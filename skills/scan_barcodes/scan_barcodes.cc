#include "scan_barcodes.h"

#include <chrono>
#include <memory>
#include <string>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "skills/scan_barcodes/scan_barcodes.pb.h"
#include "intrinsic/util/status/status_conversion_grpc.h"
#include "intrinsic/util/status/status_macros.h"
#include "intrinsic/perception/proto/camera_config.pb.h"
#include "intrinsic/perception/service/proto/camera_server.grpc.pb.h"
#include "intrinsic/skills/cc/skill_utils.h"
#include "intrinsic/skills/proto/skill_service.pb.h"
#include "intrinsic/util/grpc/grpc.h"
#include "opencv2/core/mat.hpp"

namespace scan_barcodes {

using ::com::example::ScanBarcodesParams;
using ::com::example::ScanBarcodesResult;
using ::com::example::BarcodeType;

using ::intrinsic_proto::perception::CameraConfig;
using ::intrinsic::skills::ExecuteRequest;
using ::intrinsic_proto::skills::PredictResult;
using ::intrinsic::skills::EquipmentPack;
using ::intrinsic::skills::SkillInterface;
using ::intrinsic::skills::ExecuteContext;
using ::intrinsic::WaitForChannelConnected;


BarcodeType
ConvertBarcodeTypeToProto(const std::string & type)
{
  // Strings from
  // https://github.com/opencv/opencv/blob/
  // e8f94182f577894410cc59d5d20979dff69d8878/modules/objdetect/src/
  // barcode_decoder/abs_decoder.hpp#L46-L51
  if ("EAN_8" == type) {
    return BarcodeType::BARCODE_EAN_8;
  } else if ("EAN_13") {
    return BarcodeType::BARCODE_EAN_13;
  } else if ("UPC_E") {
    return BarcodeType::BARCODE_UPC_E;
  } else if ("UPC_A") {
    return BarcodeType::BARCODE_UPC_A;
  } else if ("UPC_EAN_EXTENSION") {
    return BarcodeType::BARCODE_UPC_EAN_EXTENSION;
  }

   return BarcodeType::BARCODE_UNSPECIFIED;
}

// -----------------------------------------------------------------------------
// Skill signature.
// -----------------------------------------------------------------------------

std::unique_ptr<SkillInterface> ScanBarcodes::CreateSkill() {
  return std::make_unique<ScanBarcodes>();
}

// -----------------------------------------------------------------------------
// Skill execution.
// -----------------------------------------------------------------------------

absl::StatusOr<std::unique_ptr<google::protobuf::Message>> ScanBarcodes::Execute(
    const ExecuteRequest& request, ExecuteContext& context) {

  // Get parameters.
  INTR_ASSIGN_OR_RETURN(
    auto params, request.params<ScanBarcodesParams>());

  // Get equipment.
  const EquipmentPack equipment_pack = context.equipment();
  INTR_ASSIGN_OR_RETURN(const auto camera_equipment, equipment_pack.GetHandle(kCameraSlot));

  intrinsic_proto::perception::CameraConfig camera_config;
  camera_equipment.resource_data().at("CameraConfig").contents().UnpackTo(&camera_config);

  // Connect to the camera over gRPC.
  std::unique_ptr<intrinsic_proto::perception::CameraServer::Stub> camera_stub;
  std::string camera_handle;
  INTR_RETURN_IF_ERROR(ConnectToCamera(
    camera_equipment.connection_info().grpc(), camera_config,
    &camera_stub, &camera_handle));

  // Get a frame from the camera.
  INTR_ASSIGN_OR_RETURN(intrinsic_proto::perception::Frame frame,
    GrabFrame(camera_equipment.connection_info().grpc(), camera_stub.get(), camera_handle));

  // Convert to cv::Mat.
  auto image_buffer = frame.rgb8u();

  auto img = cv::Mat(
    image_buffer.dimensions().rows(),
    image_buffer.dimensions().cols(),
    CV_8UC3,  // Barcode detector requires unsigned data
    // Need unsigned data with no const so it can implicitly cast to void*
    const_cast<unsigned char*>(reinterpret_cast<const unsigned char *>(image_buffer.data().c_str())));

  // Do the detection.
  std::vector<cv::Point2f> detected_corners;
  std::vector<std::string> decoded_type;
  std::vector<std::string> decoded_data;

  try {
    detector_.detectAndDecodeWithType(img, decoded_data, decoded_type, detected_corners);
  } catch (const cv::Exception& e) {
    LOG(ERROR) << e.what();
    return absl::UnknownError(e.what());
  }

  std::unique_ptr<ScanBarcodesResult> result;
  INTR_ASSIGN_OR_RETURN(result, ConvertToResultProto(decoded_data, decoded_type, detected_corners));

  LOG(INFO) << "Detected " << decoded_data.size() << " barcode(s).";
  return result;
}

absl::Status
ScanBarcodes::ConnectToCamera(
  const intrinsic_proto::resources::ResourceGrpcConnectionInfo& grpc_info,
  const intrinsic_proto::perception::CameraConfig& camera_config,
  std::unique_ptr<intrinsic_proto::perception::CameraServer::Stub>* camera_stub,
  std::string* camera_handle)
{
  // Connect to the provided camera.
  std::string camera_grpc_address = grpc_info.address();
  std::string camera_server_instance = grpc_info.server_instance();

  grpc::ChannelArguments options;
  constexpr int kMaxReceiveMessageSize{-1};  // Put no limit on the size of a message we can receive.
  options.SetMaxReceiveMessageSize(kMaxReceiveMessageSize);
  auto camera_channel = grpc::CreateCustomChannel(camera_grpc_address, grpc::InsecureChannelCredentials(), options);

  INTR_RETURN_IF_ERROR(
    WaitForChannelConnected(camera_server_instance, camera_channel, absl::InfiniteFuture()));

  *camera_stub = intrinsic_proto::perception::CameraServer::NewStub(camera_channel);

  auto client_context = std::make_unique<grpc::ClientContext>();
  constexpr const auto kCameraClientTimeout = std::chrono::seconds(5);
  client_context->set_deadline(std::chrono::system_clock::now() + kCameraClientTimeout);
  if (!camera_server_instance.empty()) {
    client_context->AddMetadata("x-resource-instance-name", camera_server_instance);
  }

  intrinsic_proto::perception::CreateCameraRequest create_request;
  intrinsic_proto::perception::CreateCameraResponse create_response;
  *create_request.mutable_camera_config() = camera_config;
  INTR_RETURN_IF_ERROR(intrinsic::ToAbslStatus((*camera_stub)->CreateCamera(client_context.get(), create_request, &create_response)));

  *camera_handle = create_response.camera_handle();

  return absl::OkStatus();
}

absl::StatusOr<intrinsic_proto::perception::Frame>
ScanBarcodes::GrabFrame(
  const intrinsic_proto::resources::ResourceGrpcConnectionInfo& grpc_info,
  intrinsic_proto::perception::CameraServer::Stub* camera_stub,
  const std::string& camera_handle)
{
  std::string camera_server_instance = grpc_info.server_instance();

  auto client_context = std::make_unique<grpc::ClientContext>();
  constexpr const auto kCameraClientTimeout = std::chrono::seconds(5);
  client_context->set_deadline(std::chrono::system_clock::now() + kCameraClientTimeout);
  if (!camera_server_instance.empty()) {
    client_context->AddMetadata("x-resource-instance-name", camera_server_instance);
  }

  intrinsic_proto::perception::GetFrameRequest frame_request;
  frame_request.set_camera_handle(camera_handle);
  frame_request.mutable_timeout()->set_seconds(5);
  frame_request.mutable_post_processing()->set_skip_undistortion(false);
  intrinsic_proto::perception::GetFrameResponse frame_response;
  INTR_RETURN_IF_ERROR(intrinsic::ToAbslStatus(camera_stub->GetFrame(client_context.get(), frame_request, &frame_response)));
  return std::move(*frame_response.mutable_frame());
}

absl::StatusOr<std::unique_ptr<ScanBarcodesResult>>
ScanBarcodes::ConvertToResultProto(
  std::vector<std::string> decoded_data,
  std::vector<std::string> decoded_types,
  std::vector<cv::Point2f> detected_corners)
{
  auto result = std::make_unique<ScanBarcodesResult>();

  constexpr int kNumCorners = 4;

  if (decoded_data.size() != decoded_types.size() || (kNumCorners * decoded_types.size()) != detected_corners.size()) {
    LOG(ERROR) << "Internal error: barcode detection data had inconsistent sizes."
      << " Please report this as a bug with this skill.";
    return absl::InternalError("barcode detection data had inconsistent sizes");
  }

  for (int d = 0; d < decoded_types.size(); ++d){
    const std::string& barcode_data = decoded_data.at(d);
    const std::string& barcode_type = decoded_types.at(d);
    auto corners_iter = detected_corners.begin() + (d * kNumCorners);

    ::com::example::Barcode*  barcode = result->add_barcodes();
    barcode->set_type(ConvertBarcodeTypeToProto(barcode_type));
    barcode->set_data(barcode_data);
    
    ::com::example::Corner* corner = barcode->add_corners();

    for (int c = 0; c < kNumCorners; ++c) {
      const cv::Point2f& point = *(corners_iter + c);
      corner->set_x(point.x);
      corner->set_y(point.y);
    }
  }

  return result;
}

}  // namespace scan_barcodes

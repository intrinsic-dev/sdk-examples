#include "validate_pose.h"

#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "skills/validate_pose/validate_pose.pb.h"
#include "google/protobuf/message.h"
#include "intrinsic/icon/release/status_helpers.h"
#include "intrinsic/skills/cc/skill_registration.h"
#include "intrinsic/skills/cc/skill_utils.h"
#include "intrinsic/skills/proto/skill_service.pb.h"
#include "intrinsic/world/objects/object_world_client.h"

namespace validate_pose {

using ::com::example::ValidatePoseParams;

using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;
using ::intrinsic_proto::skills::EquipmentSelector;
using ::intrinsic_proto::skills::ExecuteRequest;
using ::intrinsic_proto::skills::ExecuteResult;
using ::intrinsic_proto::skills::PredictResult;
using ::intrinsic::Pose3d;
using ::intrinsic::eigenmath::AngleAxisd;
using ::intrinsic::eigenmath::Vector3d;
using ::intrinsic::skills::SkillInterface;
using ::intrinsic::skills::SkillProjectInterface;
using ::intrinsic::skills::ProjectionContext;
using ::intrinsic::skills::ExecutionContext;
using ::intrinsic::skills::UnpackParams;
using ::intrinsic::world::ObjectWorldClient;
using ::intrinsic::world::TransformNode;

// -----------------------------------------------------------------------------
// Skill signature.
// -----------------------------------------------------------------------------

std::unique_ptr<SkillInterface> ValidatePose::CreateSkill() {
  return std::make_unique<ValidatePose>();
}

std::string ValidatePose::Name() const {
  return kSkillName;
}

std::string ValidatePose::Package() const {
  return "com.example";
}

std::string ValidatePose::DocString() const {
  return "Validates that a pose is within an expected tolerance.";
}

absl::flat_hash_map<std::string, EquipmentSelector>
ValidatePose::EquipmentRequired() const {
  return {};
}

const Descriptor* ValidatePose::GetParameterDescriptor() const {
  return ValidatePoseParams::descriptor();
}

std::unique_ptr<Message> ValidatePose::GetDefaultParameters() const {
  return std::make_unique<ValidatePoseParams>();
}

// -----------------------------------------------------------------------------
// Skill execution.
// -----------------------------------------------------------------------------

absl::StatusOr<ExecuteResult> ValidatePose::Execute(
    const ExecuteRequest& execute_request, ExecutionContext& context) {

  // Get parameters.
  INTRINSIC_ASSIGN_OR_RETURN(
      auto params, UnpackParams<ValidatePoseParams>(execute_request));

  // Get connection to world service.
  INTRINSIC_ASSIGN_OR_RETURN(ObjectWorldClient world, context.GetObjectWorld());

  // Look up frames to compare.
  INTRINSIC_ASSIGN_OR_RETURN(TransformNode actual_object, world.GetTransformNode(params.actual_object()));
  INTRINSIC_ASSIGN_OR_RETURN(TransformNode expected_pose, world.GetTransformNode(params.expected_pose()));

  // Determine the difference between acutal and expected poses.
  INTRINSIC_ASSIGN_OR_RETURN(Pose3d diff_transform, world.GetTransform(actual_object, expected_pose));
  LOG(INFO) << "Transform difference between actual and expected: " << diff_transform;
  double position_diff = diff_transform.translation().norm();
  LOG(INFO) << "Translation error: " << position_diff << " meters";
  auto rotation_diff = AngleAxisd(diff_transform.quaternion());
  LOG(INFO) << "Rotation error: " << rotation_diff.angle() <<
               " radians (on axis " << rotation_diff.axis() << ")";

  // Return failure if anything outside tolerances.
  if (params.has_position_tolerance() && position_diff > params.position_tolerance()) {
    return absl::UnknownError(absl::StrCat(
      "Translation error of ", position_diff, " meters exceeds tolerance of ",
      params.position_tolerance()));
  }
  if (params.has_rotation_tolerance() && rotation_diff.angle() > params.rotation_tolerance()) {
    return absl::UnknownError(absl::StrCat(
      "Rotation error of ", rotation_diff.angle(), " radians exceeds tolerance of ",
      params.rotation_tolerance()));
  }

  // Return success if passed all checks.
  return ExecuteResult();
}

// Register skills.
REGISTER_SKILL(ValidatePose, ValidatePose::kSkillName, &ValidatePose::CreateSkill);

}  // namespace validate_pose

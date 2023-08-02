#include "wiggle_joint.h"

#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "skills/wiggle_joint/wiggle_joint.pb.h"
#include "google/protobuf/message.h"
// [START wiggle_joint_includes]
#include "intrinsic/icon/actions/point_to_point_move_info.h"
#include "intrinsic/icon/cc_client/client.h"
#include "intrinsic/icon/cc_client/client_utils.h"
#include "intrinsic/icon/cc_client/session.h"
#include "intrinsic/icon/equipment/channel_factory.h"
#include "intrinsic/icon/equipment/equipment_utils.h"
// [END wiggle_joint_includes]
#include "intrinsic/icon/release/status_helpers.h"
#include "intrinsic/skills/cc/skill_registration.h"
#include "intrinsic/skills/cc/skill_utils.h"
#include "intrinsic/skills/proto/skill_service.pb.h"

namespace wiggle_joint {

using ::com::example::WiggleJointParams;

using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;
// [START wiggle_joint_using_directives_p1]
using ::intrinsic_proto::icon::PartJointState;
using ::intrinsic_proto::icon::PartStatus;
// [END wiggle_joint_using_directives_p1]
using ::intrinsic_proto::skills::EquipmentSelector;
using ::intrinsic_proto::skills::ExecuteRequest;
using ::intrinsic_proto::skills::ExecuteResult;
using ::intrinsic_proto::skills::PredictResult;
// [START wiggle_joint_using_directives_p2]
using ::intrinsic::icon::Action;
using ::intrinsic::icon::ActionDescriptor;
using ::intrinsic::icon::ActionInstanceId;
using ::intrinsic::icon::ConnectToIconEquipment;
using ::intrinsic::icon::CreatePointToPointMoveFixedParams;
using ::intrinsic::icon::DefaultChannelFactory;
using ::intrinsic::icon::Icon2EquipmentSelectorBuilder;
using ::intrinsic::icon::IconEquipment;
using ::intrinsic::icon::IsDone;
using ::intrinsic::icon::IsGreaterThanOrEqual;
using ::intrinsic::icon::IsTrue;
using ::intrinsic::icon::PointToPointMoveInfo;
using ::intrinsic::icon::ReactionDescriptor;
using ::intrinsic::skills::EquipmentPack;
// [END wiggle_joint_using_directives_p2]
using ::intrinsic::skills::SkillInterface;
using ::intrinsic::skills::SkillProjectInterface;
using ::intrinsic::skills::ProjectionContext;
using ::intrinsic::skills::ExecutionContext;
using ::intrinsic::skills::UnpackParams;

// [START wiggle_joint_using_directives_p3]
using IconClient = ::intrinsic::icon::Client;
using IconSession = ::intrinsic::icon::Session;
// [END wiggle_joint_using_directives_p3]
// -----------------------------------------------------------------------------
// Skill signature.
// -----------------------------------------------------------------------------

std::unique_ptr<SkillInterface> WiggleJoint::CreateSkill() {
  return std::make_unique<WiggleJoint>();
}

std::string WiggleJoint::Name() const {
  return kSkillName;
}

std::string WiggleJoint::Package() const {
  return "com.example";
}

std::string WiggleJoint::DocString() const {
  return "Move one joint on a robot back and fourth 5 degrees.";
}

// [START declare_equipment]
absl::flat_hash_map<std::string, EquipmentSelector>
WiggleJoint::EquipmentRequired() const {
  return absl::flat_hash_map<std::string,
                            intrinsic_proto::skills::EquipmentSelector>({
    {kRobotSlot, Icon2EquipmentSelectorBuilder()
                          .WithPositionControlledPart()
                          .Build()},
  });
}
// [END declare_equipment]

const Descriptor* WiggleJoint::GetParameterDescriptor() const {
  return WiggleJointParams::descriptor();
}

std::unique_ptr<Message> WiggleJoint::GetDefaultParameters() const {
  return std::make_unique<WiggleJointParams>();
}

// -----------------------------------------------------------------------------
// Skill execution.
// -----------------------------------------------------------------------------

absl::StatusOr<ExecuteResult> WiggleJoint::Execute(
    const ExecuteRequest& execute_request, ExecutionContext& context) {

  // Get parameters.
  INTRINSIC_ASSIGN_OR_RETURN(
      auto params, UnpackParams<WiggleJointParams>(execute_request));

  // [START access_equipment]
  const EquipmentPack equipment_pack(
    execute_request.instance().equipment_handles());
  INTRINSIC_ASSIGN_OR_RETURN(const auto equipment, equipment_pack.GetHandle(kRobotSlot));
  // [END access_equipment]
  // [START connect_to_robot]
  INTRINSIC_ASSIGN_OR_RETURN(IconEquipment icon_equipment,
    ConnectToIconEquipment(equipment_pack, kRobotSlot, DefaultChannelFactory()));
  IconClient icon_client(icon_equipment.channel);
  // [END connect_to_robot]
  // [START get_robot_info]
  INTRINSIC_ASSIGN_OR_RETURN(auto robot_config, icon_client.GetConfig());

   const std::string part_name = icon_equipment.position_part_name.value();
  INTRINSIC_ASSIGN_OR_RETURN(intrinsic_proto::icon::GenericPartConfig part_config,
                   robot_config.GetGenericPartConfig(part_name));

  INTRINSIC_ASSIGN_OR_RETURN(const PartStatus part_status,
                   icon_client.GetSinglePartStatus(part_name));
  // [END get_robot_info]
  // [START parameter_validation]
  if (
    params.joint_number() < 0 ||
    params.joint_number() >= part_status.joint_states_size()) {

    return absl::FailedPreconditionError(absl::StrCat(
      "Joint ", params.joint_number(), " does not exist on: ",
      equipment.name()));
  }
  // [END parameter_validation]
  // [START describe_movement_goals]
  const double five_degrees = 0.0872665;  // Radians
  const double joint_target_pose =
      part_status.joint_states(
        params.joint_number()).position_sensed() + five_degrees;

  std::vector<double> original_positions;
  std::vector<double> first_goal_positions;
  std::vector<double> zero_velocity;
  for (size_t i = 0; i < part_status.joint_states_size(); ++i) {
      const double cur_pos = part_status.joint_states(i).position_sensed();
      if (params.joint_number() == i) {
          first_goal_positions.push_back(joint_target_pose);
      } else {
          first_goal_positions.push_back(cur_pos);
      }
      original_positions.push_back(cur_pos);
      zero_velocity.push_back(0);
  }
  // [END describe_movement_goals]

  // [START start_icon_session]
  // Start an icon session
  INTRINSIC_ASSIGN_OR_RETURN(
    std::unique_ptr<IconSession> icon_session,
    IconSession::Start(icon_equipment.channel, {std::string(part_name)}));
  // [END start_icon_session]

  // [START icon_constants]
  const ActionInstanceId kFirstMoveId(1);
  const ActionInstanceId kSecondMoveId(2);
  const double kTimeoutSeconds = 2;
  // [END icon_constants]

  // [START first_icon_action]
  ActionDescriptor first_move =
    // [START first_icon_action_id_and_goal]
    ActionDescriptor(PointToPointMoveInfo::kActionTypeName,
                            kFirstMoveId, part_name)
        .WithFixedParams(CreatePointToPointMoveFixedParams(
            first_goal_positions, zero_velocity))
    // [END first_icon_action_id_and_goal]
    // [START first_icon_action_when_done]
        .WithReaction(
            ReactionDescriptor(IsDone())
            .WithRealtimeActionOnCondition(kSecondMoveId))
    // [END first_icon_action_when_done]
    // [START first_icon_action_when_timeout]
        .WithReaction(
            ReactionDescriptor(
                IsGreaterThanOrEqual(
                          PointToPointMoveInfo::kSetpointDoneForSeconds,
                          kTimeoutSeconds))
            // This nonrealtime callback prints a message if the
            // joint motion fails to settle within a timeout.
            .WithWatcherOnCondition([&icon_session]() {
              LOG(ERROR) << "Failed to reach Goal";
              icon_session->QuitWatcherLoop();
            }));
    // [END first_icon_action_when_timeout]
  // [END first_icon_action]
  // [START second_icon_action]
  ActionDescriptor second_move =
    ActionDescriptor(PointToPointMoveInfo::kActionTypeName,
                            kSecondMoveId, part_name)
        .WithFixedParams(CreatePointToPointMoveFixedParams(
            original_positions, zero_velocity))
        .WithReaction(
            ReactionDescriptor(
                IsTrue(PointToPointMoveInfo::kIsSettled))
            .WithWatcherOnCondition([&icon_session]() {
                LOG(INFO) << "Finished moving joint.";
                icon_session->QuitWatcherLoop();
              }))
        .WithReaction(
            ReactionDescriptor(
                IsGreaterThanOrEqual(
                    PointToPointMoveInfo::kSetpointDoneForSeconds,
                    kTimeoutSeconds))
            // This nonrealtime callback prints a message if the
            // joint motion fails to settle within a timeout.
            .WithWatcherOnCondition([&icon_session]() {
              LOG(ERROR) << "Failed to reach Goal";
              icon_session->QuitWatcherLoop();
            }));
  // [END second_icon_action]

  // [START add_icon_actions]
  INTRINSIC_ASSIGN_OR_RETURN(
    auto actions,
    icon_session->AddActions({first_move, second_move}));
  // [END add_icon_actions]

  // [START move_the_robot]
  LOG(INFO) << "Starting Wiggle.";
  INTRINSIC_RETURN_IF_ERROR(
    icon_session->StartActions({actions.front()}));
  INTRINSIC_RETURN_IF_ERROR(icon_session->RunWatcherLoop());
  LOG(INFO) << "Finished Wiggle.";
  // [END move_the_robot]

  return ExecuteResult();
}

// Register skills.
REGISTER_SKILL(WiggleJoint, WiggleJoint::kSkillName, &WiggleJoint::CreateSkill);

}  // namespace wiggle_joint

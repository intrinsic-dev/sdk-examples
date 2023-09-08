#include "use_world.h"

#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "skills/use_world/use_world.pb.h"
#include "google/protobuf/message.h"
#include "intrinsic/icon/equipment/equipment_utils.h"
#include "intrinsic/icon/release/status_helpers.h"
#include "intrinsic/skills/cc/skill_registration.h"
#include "intrinsic/skills/cc/skill_utils.h"
#include "intrinsic/skills/proto/skill_service.pb.h"
#include "intrinsic/world/objects/object_world_client.h"

namespace use_world {

using ::com::example::UseWorldParams;

using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;
using ::intrinsic_proto::skills::EquipmentSelector;
using ::intrinsic_proto::skills::ExecuteRequest;
using ::intrinsic_proto::skills::ExecuteResult;
using ::intrinsic_proto::skills::PredictResult;
using ::intrinsic::FrameName;
using ::intrinsic::Pose3d;
using ::intrinsic::icon::Icon2EquipmentSelectorBuilder;
using ::intrinsic::skills::EquipmentPack;
using ::intrinsic::skills::SkillInterface;
using ::intrinsic::skills::SkillProjectInterface;
using ::intrinsic::skills::ProjectionContext;
using ::intrinsic::skills::ExecutionContext;
using ::intrinsic::skills::UnpackParams;
using ::intrinsic::world::Frame;
using ::intrinsic::world::KinematicObject;
using ::intrinsic::world::ObjectWorldClient;
using ::intrinsic::world::TransformNode;
using ::intrinsic::world::WorldObject;

// -----------------------------------------------------------------------------
// Skill signature.
// -----------------------------------------------------------------------------

std::unique_ptr<SkillInterface> UseWorld::CreateSkill() {
  return std::make_unique<UseWorld>();
}

std::string UseWorld::Name() const {
  return kSkillName;
}

std::string UseWorld::Package() const {
  return "com.example";
}

std::string UseWorld::DocString() const {
  return "Queries information from the world and makes some world updates.";
}

absl::flat_hash_map<std::string, EquipmentSelector>
UseWorld::EquipmentRequired() const {
    auto camera_selector = EquipmentSelector();
    camera_selector.add_equipment_type_names("CameraConfig");
  return absl::flat_hash_map<std::string,
                            intrinsic_proto::skills::EquipmentSelector>({
    {kRobotSlot, Icon2EquipmentSelectorBuilder()
                          .WithPositionControlledPart()
                          .Build()},
    {kCameraSlot, camera_selector},
  });
}

const Descriptor* UseWorld::GetParameterDescriptor() const {
  return UseWorldParams::descriptor();
}

std::unique_ptr<Message> UseWorld::GetDefaultParameters() const {
  return std::make_unique<UseWorldParams>();
}

Pose3d UseWorld::DetectObject() {
  // Pretend to have detected something.
  return Pose3d();
}

// -----------------------------------------------------------------------------
// Skill execution.
// -----------------------------------------------------------------------------

absl::StatusOr<ExecuteResult> UseWorld::Execute(
    const ExecuteRequest& execute_request, ExecutionContext& context) {

  // Get parameters.
  INTRINSIC_ASSIGN_OR_RETURN(
      auto params, UnpackParams<UseWorldParams>(execute_request));

  // Get connection to world service.
  INTRINSIC_ASSIGN_OR_RETURN(ObjectWorldClient world, context.GetObjectWorld());

  // Unpack equipment handles.
  const EquipmentPack equipment_pack(
    execute_request.instance().equipment_handles());
  INTRINSIC_ASSIGN_OR_RETURN(const auto camera_handle, equipment_pack.GetHandle(kCameraSlot));
  INTRINSIC_ASSIGN_OR_RETURN(const auto robot_handle, equipment_pack.GetHandle(kRobotSlot));

  // Resolve ObjectReference.
  INTRINSIC_ASSIGN_OR_RETURN(WorldObject obj, world.GetObject(params.object_ref()));

  // Resolve FrameReference.
  INTRINSIC_ASSIGN_OR_RETURN(Frame frame, world.GetFrame(params.frame_ref()));

  // Resolve TransformNodeReference, obtaining an instance of either
  // WorldObject or Frame.
  INTRINSIC_ASSIGN_OR_RETURN(TransformNode obj_or_frame, world.GetTransformNode(params.transform_node_ref()));

  LOG(INFO) << obj.Name();
  LOG(INFO) << obj.ParentTThis();

  LOG(INFO) << frame.Name();
  LOG(INFO) << frame.ObjectName();
  LOG(INFO) << frame.ParentTThis();

  LOG(INFO) << obj_or_frame.Id();
  LOG(INFO) << obj_or_frame.ParentTThis();

  // Equipment handles can be used in place of world references.
  INTRINSIC_ASSIGN_OR_RETURN(WorldObject camera_obj, world.GetObject(camera_handle));

  // If we know that an object is a robot, we can also get it as a
  // KinematicObject which has additional properties.
  INTRINSIC_ASSIGN_OR_RETURN(KinematicObject robot_kinematic_obj, world.GetKinematicObject(robot_handle));

  LOG(INFO) << robot_kinematic_obj.JointPositions();

  // WorldObject representing the world origin. Always present.
  INTRINSIC_ASSIGN_OR_RETURN(WorldObject root_object, world.GetRootObject());

  // Every camera has a frame called "sensor" which represents the origin of
  // the camera sensor (as opposed to the camera object itself representing the
  // origin of the cameras geometry).
  INTRINSIC_ASSIGN_OR_RETURN(Frame sensor_frame, camera_obj.GetFrame(FrameName{"sensor"}));

  // Every robot has a frame called "flange" which represents the flange of a
  // robot arm according to the ISO 9787 standard.
  INTRINSIC_ASSIGN_OR_RETURN(Frame flange_frame, robot_kinematic_obj.GetFrame(FrameName{"flange"}));

  LOG(INFO) << root_object.ObjectReference().ShortDebugString();
  LOG(INFO) << sensor_frame.FrameReference().ShortDebugString();
  LOG(INFO) << flange_frame.FrameReference().ShortDebugString();

  INTRINSIC_ASSIGN_OR_RETURN(Pose3d root_t_obj, world.GetTransform(root_object, obj));
  INTRINSIC_ASSIGN_OR_RETURN(Pose3d obj_t_frame, world.GetTransform(obj, frame));
  INTRINSIC_ASSIGN_OR_RETURN(Pose3d robot_t_sensor, world.GetTransform(robot_kinematic_obj, sensor_frame));
  INTRINSIC_ASSIGN_OR_RETURN(Pose3d sensor_t_obj, world.GetTransform(sensor_frame, obj));

  LOG(INFO) << root_t_obj;
  LOG(INFO) << obj_t_frame;
  LOG(INFO) << robot_t_sensor;
  LOG(INFO) << sensor_t_obj;

  INTRINSIC_RETURN_IF_ERROR(
    world.UpdateTransform(
      root_object, obj,
      Pose3d(
        intrinsic::eigenmath::Quaterniond({1., 0., 0., 0.}),
        intrinsic::eigenmath::Vector3d({1.0, 2.0, 3.0}))));

  // Prints: Pose3(...old pose...)
  LOG(INFO) << obj.ParentTThis();
  
  // Get a fresh snapshot of the object ('obj' is just a local snapshot and
  // does not get updated automatically!).
  INTRINSIC_ASSIGN_OR_RETURN(WorldObject obj_new, world.GetObject(params.object_ref()));

  // Prints: Pose3(Rotation3([0i + 0j + 0k + 1]),[1. 2. 3.])
  LOG(INFO) << obj_new.ParentTThis();

  // Assume the following world structure:
  //
  //        'world.root' (WorldObject)
  //           /    \
  //      robot     'obj' (WorldObject)
  //        /
  //   'camera' (WorldObject)
  //       |
  // 'camera.sensor' (Frame)

  Pose3d estimated_sensor_t_obj = DetectObject();

  // Update root_t_obj (= 'obj.parent_t_this') such that sensor_t_obj becomes
  // equal to 'estimated_sensor_t_obj'. The chain of transforms from the root
  // object to the sensor object will not be modified, i.e., we are just moving
  // the object 'obj' and not the camera or the robot.
  INTRINSIC_RETURN_IF_ERROR(world.UpdateTransform(sensor_frame, obj, obj, estimated_sensor_t_obj));

  return ExecuteResult();
}

// Register skills.
REGISTER_SKILL(UseWorld, UseWorld::kSkillName, &UseWorld::CreateSkill);

}  // namespace use_world

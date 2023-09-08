#ifndef USE_WORLD_USE_WORLD_H_
#define USE_WORLD_USE_WORLD_H_

#include <memory>
#include <string>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "google/protobuf/descriptor.h"
#include "intrinsic/math/pose3.h"
#include "intrinsic/skills/cc/skill_interface.h"
#include "intrinsic/skills/proto/skill_service.pb.h"

namespace use_world {

class UseWorld : public intrinsic::skills::SkillInterface {
 public:
  static constexpr char kRobotSlot[] = "robot";
  static constexpr char kCameraSlot[] = "camera";

  // ---------------------------------------------------------------------------
  // Skill signature (see intrinsic::skills::SkillSignatureInterface)
  // ---------------------------------------------------------------------------

  // Name of the skill.
  static constexpr char kSkillName[] = "use_world_cc";

  // Factory method to create an instance of the skill.
  static std::unique_ptr<intrinsic::skills::SkillInterface> CreateSkill();

  // Returns the skill name.
  std::string Name() const override;

  // Returns the skill package.
  std::string Package() const override;

  // Returns the documentation shown in developer tools to describe the skill.
  std::string DocString() const override;

  // Returns the list of equipment used by the skill.
  absl::flat_hash_map<std::string, intrinsic_proto::skills::EquipmentSelector>
      EquipmentRequired() const override;

  // Returns the Protocol Buffer description of the parameters the skill accepts
  // as input.
  const google::protobuf::Descriptor* GetParameterDescriptor() const override;

  // Returns a prefilled Protocol Buffer of input parameters to be used as
  // default values when the skill is called.
  std::unique_ptr<google::protobuf::Message> GetDefaultParameters()
      const override;

  // Mock method for an object detection.
  intrinsic::Pose3d DetectObject();

  // ---------------------------------------------------------------------------
  // Skill execution (see intrinsic::skills::SkillExecuteInterface)
  // ---------------------------------------------------------------------------

  // Called once each time the skill is executed in a process.
  absl::StatusOr<intrinsic_proto::skills::ExecuteResult>
  Execute(const intrinsic_proto::skills::ExecuteRequest& execute_request,
          intrinsic::skills::ExecutionContext& context) override;
};

}  // namespace use_world

#endif  // USE_WORLD_USE_WORLD_H_

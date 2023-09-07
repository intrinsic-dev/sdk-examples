#include "say_skill.h"

#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/notification.h"
#include "skills/say_skill/say_skill.pb.h"
#include "google/protobuf/message.h"
#include "intrinsic/icon/release/status_helpers.h"
#include "intrinsic/skills/cc/skill_registration.h"
#include "intrinsic/skills/cc/skill_utils.h"
#include "intrinsic/skills/proto/skill_service.pb.h"

namespace say_skill {

using ::com::example::SaySkillParams;

using ::google::protobuf::Descriptor;
using ::google::protobuf::Message;
using ::intrinsic_proto::skills::ExecuteRequest;
using ::intrinsic_proto::skills::ExecuteResult;
using ::intrinsic_proto::skills::PredictResult;
using ::intrinsic::skills::SkillInterface;
using ::intrinsic::skills::SkillProjectInterface;
using ::intrinsic::skills::ProjectionContext;
using ::intrinsic::skills::ExecutionContext;
using ::intrinsic::skills::UnpackParams;

// -----------------------------------------------------------------------------
// Skill signature.
// -----------------------------------------------------------------------------

std::unique_ptr<SkillInterface> SaySkill::CreateSkill() {
  return std::make_unique<SaySkill>();
}

std::string SaySkill::Name() const {
  return kSkillName;
}

std::string SaySkill::Package() const {
  return "com.example";
}

std::string SaySkill::DocString() const {
  return "Pause before logging a message.";
}

const Descriptor* SaySkill::GetParameterDescriptor() const {
  return SaySkillParams::descriptor();
}

std::unique_ptr<Message> SaySkill::GetDefaultParameters() const {
  return std::make_unique<SaySkillParams>();
}

bool SaySkill::SupportsCancellation() const {
    return true;
}

// -----------------------------------------------------------------------------
// Skill execution.
// -----------------------------------------------------------------------------

absl::StatusOr<ExecuteResult> SaySkill::Execute(
    const ExecuteRequest& execute_request, ExecutionContext& context) {

  // Get parameters.
  INTRINSIC_ASSIGN_OR_RETURN(
      auto params, UnpackParams<SaySkillParams>(execute_request));

  auto should_cancel = std::make_shared<absl::Notification>();
  auto cancel_callback = [should_cancel]() {
    should_cancel->Notify();
    return absl::OkStatus();
  };
  INTRINSIC_RETURN_IF_ERROR(
    context.RegisterCancellationCallback(cancel_callback));

  context.NotifyReadyForCancellation();
  if (should_cancel->WaitForNotificationWithTimeout(
    absl::Milliseconds(params.wait_ms()))) {

    LOG(INFO) << Name() << " was canceled!";
    return absl::CancelledError("say_skill_cc was canceled.");
  }

  LOG(INFO) << params.text();

  return ExecuteResult();
}

// Register skills.
REGISTER_SKILL(SaySkill, SaySkill::kSkillName, &SaySkill::CreateSkill);

}  // namespace say_skill

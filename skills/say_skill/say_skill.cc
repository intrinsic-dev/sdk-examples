#include "say_skill.h"

#include <memory>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/notification.h"
#include "skills/say_skill/say_skill.pb.h"
#include "intrinsic/icon/release/status_helpers.h"
#include "intrinsic/skills/cc/skill_utils.h"
#include "intrinsic/skills/proto/skill_service.pb.h"

namespace say_skill {

using ::com::example::SaySkillParams;

using ::intrinsic::skills::ExecuteRequest;
using ::intrinsic_proto::skills::ExecuteResult;
using ::intrinsic::skills::SkillInterface;
using ::intrinsic::skills::ExecuteContext;

// -----------------------------------------------------------------------------
// Skill signature.
// -----------------------------------------------------------------------------

std::unique_ptr<SkillInterface> SaySkill::CreateSkill() {
  return std::make_unique<SaySkill>();
}

// -----------------------------------------------------------------------------
// Skill execution.
// -----------------------------------------------------------------------------

absl::StatusOr<ExecuteResult> SaySkill::Execute(
    const ExecuteRequest& request, ExecuteContext& context) {

  // Get parameters.
  INTRINSIC_ASSIGN_OR_RETURN(
    auto params, request.params<SaySkillParams>());

  context.GetCanceller().Ready();
  if (context.GetCanceller().Wait(absl::Milliseconds(params.wait_ms()))) {
    LOG(INFO) << "say_skill_cc was canceled!";
    return absl::CancelledError("say_skill_cc was canceled.");
  }

  LOG(INFO) << params.text();

  return ExecuteResult();
}
}  // namespace say_skill

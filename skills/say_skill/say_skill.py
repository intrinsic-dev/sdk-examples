"""Contains the skill say_skill_py."""
import threading
from typing import Mapping

from absl import logging

from intrinsic.skills.proto import equipment_pb2
from intrinsic.skills.proto import prediction_pb2
from intrinsic.skills.proto import skill_service_pb2
from intrinsic.skills.python import proto_utils
from intrinsic.skills.python import skill_interface
from intrinsic.util.decorators import overrides
from google.protobuf import descriptor

from skills.say_skill import say_skill_pb2


class SaySkill(skill_interface.Skill):
    """Implementation of the say_skill_py skill."""

    def __init__(self) -> None:
        pass

    @classmethod
    @overrides(skill_interface.Skill)
    def required_equipment(cls) -> Mapping[str, equipment_pb2.EquipmentSelector]:
        return {}

    @classmethod
    @overrides(skill_interface.Skill)
    def name(cls) -> str:
        return "say_skill_py"

    @classmethod
    @overrides(skill_interface.Skill)
    def package(cls) -> str:
        return "com.example"
    
    @classmethod
    @overrides(skill_interface.Skill)
    def supports_cancellation(cls) -> bool:
        return True

    @overrides(skill_interface.Skill)
    def execute(self, execute_request: skill_service_pb2.ExecuteRequest,
                context: skill_interface.ExecutionContext
                ) -> skill_service_pb2.ExecuteResult:
        params = say_skill_pb2.SaySkillParams()
        proto_utils.unpack_any(execute_request.parameters, params)

        should_cancel = threading.Event()
        context.register_cancellation_callback(should_cancel.set)
        context.notify_ready_for_cancellation()
        if should_cancel.wait(params.wait_ms / 1000.0):
            raise skill_interface.SkillCancelledError(
                f"{self.name()} was cancelled."
            )

        logging.info(params.text)
        return skill_service_pb2.ExecuteResult()

    @overrides(skill_interface.Skill)
    def get_parameter_descriptor(self) -> descriptor.Descriptor:
        """Returns the descriptor for the parameter that this skill expects."""
        return say_skill_pb2.SaySkillParams.DESCRIPTOR

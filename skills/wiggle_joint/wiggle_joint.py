"""A skill that connects to a camera resource and scans all visible barcodes using OpenCV."""

from typing import Mapping, List

from absl import logging

from intrinsic.icon.actions import point_to_point_move_pb2
from intrinsic.icon.equipment import equipment_utils
from intrinsic.icon.proto import joint_space_pb2
from intrinsic.icon.python import icon_api
from intrinsic.skills.proto import equipment_pb2
from intrinsic.skills.proto import skill_service_pb2
from intrinsic.skills.python import proto_utils
from intrinsic.skills.python import skill_interface
from intrinsic.util.decorators import overrides
from google.protobuf import descriptor

from skills.wiggle_joint import wiggle_joint_pb2

ROBOT_EQUIPMENT_SLOT: str = "robot"


def get_single_part_status(status_response, part_name):
    if part_name in status_response.part_status:
        return status_response.part_status[part_name]


class WiggleJoint(skill_interface.Skill):
    """Skill that wiggles a single joint on a robot."""

    def __init__(self) -> None:
        super().__init__()

    @classmethod
    @overrides(skill_interface.Skill)
    def required_equipment(cls) -> Mapping[str, equipment_pb2.EquipmentSelector]:
        return {
            ROBOT_EQUIPMENT_SLOT: equipment_utils.make_icon_equipment_selector(
                with_position_controlled_part=True
            )
        }

    @classmethod
    @overrides(skill_interface.Skill)
    def name(cls) -> str:
        return "wiggle_joint_py"

    @classmethod
    @overrides(skill_interface.Skill)
    def package(cls) -> str:
        return "com.example"

    @overrides(skill_interface.Skill)
    def execute(
        self,
        execute_request: skill_service_pb2.ExecuteRequest,
        context: skill_interface.ExecutionContext,
    ) -> skill_service_pb2.ExecuteResult:
        # Get params.
        params = wiggle_joint_pb2.WiggleJointParams()
        proto_utils.unpack_any(execute_request.parameters, params)

        # Get equipment.
        icon_equipment: equipment_pb2.EquipmentHandle = (
            execute_request.instance.equipment_handles[ROBOT_EQUIPMENT_SLOT]
        )

        icon_client = equipment_utils.init_icon_client(icon_equipment)

        robot_config = icon_client.get_config()
        part_name = equipment_utils.get_position_part_name(icon_equipment)
        part_status = get_single_part_status(icon_client.get_status(), part_name)

        if part_status is None:
            raise ValueError(f"Could not get status for {part_name}")

        indexTooSmall = params.joint_number < 0
        indexTooBig = params.joint_number >= len(part_status.joint_states)
        if indexTooSmall or indexTooBig:
            raise ValueError(
                f"Joint {params.joint_number} does not exist"
                f" on {icon_equipment.name}"
            )

        five_degrees = 0.0872665  # Radians
        joint_target_pose = (
            part_status.joint_states[params.joint_number].position_sensed + five_degrees
        )

        original_positions = joint_space_pb2.JointVec()
        first_goal_positions = joint_space_pb2.JointVec()
        zero_velocity = joint_space_pb2.JointVec()
        for i, cur_state in enumerate(part_status.joint_states):
            cur_pos = cur_state.position_sensed
            if i == params.joint_number:
                first_goal_positions.joints.append(joint_target_pose)
            else:
                first_goal_positions.joints.append(cur_pos)
            original_positions.joints.append(cur_pos)
            zero_velocity.joints.append(0)

        FIRST_ACTION_ID = 1
        SECOND_ACTION_ID = 2

        TIMEOUT_SECONDS = 2.0

        sig_actions_done = icon_api.SignalFlag()
        success = True

        def on_timeout(timestamp, prev_action_id, current_action_id):
            nonlocal success
            success = False
            logging.error(f"Action {current_action_id} timed out")

        with icon_client.start_session([part_name]) as icon_session:
            first_move = icon_api.Action(
                action_id=FIRST_ACTION_ID,
                action_type="xfa.point_to_point_move",
                part_name_or_slot_part_map=part_name,
                params=point_to_point_move_pb2.PointToPointMoveFixedParams(
                    goal_position=first_goal_positions,
                    goal_velocity=zero_velocity,
                ),
                reactions=[
                    icon_api.Reaction(
                        condition=icon_api.Condition.is_true("xfa.is_settled"),
                        responses=[
                            icon_api.StartActionInRealTime(
                                start_action_id=SECOND_ACTION_ID,
                            )
                        ],
                    ),
                    icon_api.Reaction(
                        condition=icon_api.Condition.is_greater_than_or_equal(
                            state_variable_name="xfa.setpoint_done_for_seconds",
                            value=TIMEOUT_SECONDS,
                        ),
                        responses=[
                            icon_api.TriggerCallback(on_timeout),
                            icon_api.Signal(sig_actions_done),
                        ],
                    ),
                ],
            )
            second_move = icon_api.Action(
                action_id=SECOND_ACTION_ID,
                action_type="xfa.point_to_point_move",
                part_name_or_slot_part_map=part_name,
                params=point_to_point_move_pb2.PointToPointMoveFixedParams(
                    goal_position=original_positions,
                    goal_velocity=zero_velocity,
                ),
                reactions=[
                    icon_api.Reaction(
                        condition=icon_api.Condition.is_true("xfa.is_settled"),
                        responses=[icon_api.Signal(sig_actions_done)],
                    ),
                    icon_api.Reaction(
                        condition=icon_api.Condition.is_greater_than_or_equal(
                            state_variable_name="xfa.setpoint_done_for_seconds",
                            value=TIMEOUT_SECONDS,
                        ),
                        responses=[
                            icon_api.TriggerCallback(on_timeout),
                            icon_api.Signal(sig_actions_done),
                        ],
                    ),
                ],
            )
            icon_session.add_actions([first_move, second_move])
            icon_session.start_action(FIRST_ACTION_ID)

            logging.info("WiggleJoint waiting for robot to move")
            sig_actions_done.wait()
            logging.info("WiggleJoint movement is complete")

        if not success:
            raise RuntimeError("Failed to wiggle joint")

        execute_result = skill_service_pb2.ExecuteResult()
        return execute_result

    @overrides(skill_interface.Skill)
    def get_parameter_descriptor(self) -> descriptor.Descriptor:
        """Returns the descriptor for the parameter that this skill expects."""
        return wiggle_joint_pb2.WiggleJointParams.DESCRIPTOR

"""Contains the skill validate_pose."""

import numpy

from typing import Mapping
from absl import logging

from intrinsic.skills.proto import equipment_pb2
from intrinsic.skills.proto import prediction_pb2
from intrinsic.skills.proto import skill_service_pb2
from intrinsic.skills.python import proto_utils
from intrinsic.skills.python import skill_interface
from intrinsic.util.decorators import overrides
from intrinsic.world.python import object_world_client
from google.protobuf import any_pb2
from google.protobuf import descriptor

from validate_pose import validate_pose_pb2


class ValidatePose(skill_interface.Skill):
    """Implementation of the validate_pose skill."""

    def __init__(self) -> None:
        pass

    @classmethod
    @overrides(skill_interface.Skill)
    def required_equipment(cls) -> Mapping[str, equipment_pb2.EquipmentSelector]:
        return {}

    @classmethod
    @overrides(skill_interface.Skill)
    def name(cls) -> str:
        return 'validate_pose'


    @classmethod
    @overrides(skill_interface.Skill)
    def package(cls) -> str:
        return 'com.example'

    @overrides(skill_interface.Skill)
    def predict(
      self,
      project_params: skill_interface.Skill.ProjectParams,
      context: skill_interface.ProjectionContext,
    ) -> skill_service_pb2.PredictResult:
        # Unpack input parameter proto into typed Python object.
        params = validate_pose_pb2.ValidatePoseParams()
        proto_utils.unpack_any(project_params.skill_parameters(), params)
        logging.info('Input parameters: %s', params)

        # Perform validation.
        world: object_world_client.ObjectWorldClient = context.get_object_world();
        validate_pose(world, params)

        # Predict success.
        return skill_service_pb2.PredictResult(
            outcomes=[
                prediction_pb2.Prediction(
                    probability=1.0,
                )
            ],
        )

    @overrides(skill_interface.Skill)
    def execute(self, execute_request: skill_service_pb2.ExecuteRequest,
                context: skill_interface.ExecutionContext
                ) -> skill_service_pb2.ExecuteResult:
        # Unpack input parameter proto into typed Python object.
        params = validate_pose_pb2.ValidatePoseParams()
        proto_utils.unpack_any(execute_request.parameters, params)
        logging.info('Input parameters: %s', params)

        # Perform validation.
        world: object_world_client.ObjectWorldClient = context.get_object_world();
        validate_pose(world, params)

        # Return success.
        return skill_service_pb2.ExecuteResult()

    @overrides(skill_interface.Skill)
    def get_parameter_descriptor(self) -> descriptor.Descriptor:
        """Returns the descriptor for the parameter that this skill expects."""
        return validate_pose_pb2.ValidatePoseParams.DESCRIPTOR


def validate_pose(world: object_world_client.ObjectWorldClient,
                  params: validate_pose_pb2.ValidatePoseParams):
    # Look up frames to compare.
    actual_object = world.get_transform_node(params.actual_object);
    expected_pose = world.get_transform_node(params.expected_pose);

    # Determine the difference between acutal and expected poses.
    diff_transform = world.get_transform(actual_object, expected_pose)
    logging.info('Transform difference between actual and expected: %s', diff_transform)
    position_diff: float = numpy.sqrt(diff_transform.translation.dot(diff_transform.translation))
    logging.info(f'Translation error: %s meters', position_diff)
    rotation_diff = diff_transform.rotation.angle()
    logging.info(f'Rotation error: %s radians (on axis %s)',
                 rotation_diff, diff_transform.rotation.axis())

    # Check tolerances.
    position_tolerance = params.position_tolerance if params.HasField('position_tolerance') else 0
    rotation_tolerance = params.rotation_tolerance if params.HasField('rotation_tolerance') else 0
    if position_diff > position_tolerance:
        raise ValueError(f'Translation error of {position_diff} meters exceeds tolerance of {position_tolerance}')
    if rotation_diff > rotation_tolerance:
        raise ValueError(f'Rotation error of {rotation_diff} radians exceeds tolerance of {rotation_tolerance}')

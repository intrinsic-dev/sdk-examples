"""A skill that connects to a camera resource and scans all visible barcodes using OpenCV."""

# [START import_typing]
from typing import Mapping, Optional, Tuple, List
# [END import_typing]

from absl import logging
# [START import_cv2]
import cv2
# [END import_cv2]
# [START import_grpc]
import grpc
# [END import_grpc]
# [START import_numpy]
import numpy as np
# [END import_numpy]

# [START import_camera_config_pb2]
from intrinsic.perception.proto import camera_config_pb2
# [END import_camera_config_pb2]
# [START import_frame_pb2]
from intrinsic.perception.proto import frame_pb2
# [END import_frame_pb2]
# [START import_camera_server_pb2]
from intrinsic.perception.service.proto import camera_server_pb2
from intrinsic.perception.service.proto import camera_server_pb2_grpc
# [END import_camera_server_pb2]
from intrinsic.skills.proto import equipment_pb2
from intrinsic.skills.proto import prediction_pb2
from intrinsic.skills.proto import skill_service_pb2
from intrinsic.skills.python import proto_utils
from intrinsic.skills.python import skill_interface
from intrinsic.util.decorators import overrides
# [START import_intrinsic_grpc_utils]
from intrinsic.util.grpc import connection
from intrinsic.util.grpc import interceptor
# [END import_intrinsic_grpc_utils]
from google.protobuf import descriptor

from skills.scan_barcodes import scan_barcodes_pb2

# [START camera_slot_constant]
CAMERA_EQUIPMENT_SLOT: str = "camera"
# [END camera_slot_constant]


# [START convert_barcode_type_to_proto]
def convert_barcode_type_to_proto(
    barcode_type: int,
) -> scan_barcodes_pb2.BarcodeType:
    """Convert cv2 barcode type to BarcodeType proto."""
    if barcode_type == cv2.barcode.NONE:
        return scan_barcodes_pb2.BARCODE_NONE
    elif barcode_type == cv2.barcode.EAN_8:
        return scan_barcodes_pb2.BARCODE_EAN_8
    elif barcode_type == cv2.barcode.EAN_13:
        return scan_barcodes_pb2.BARCODE_EAN_13
    elif barcode_type == cv2.barcode.UPC_A:
        return scan_barcodes_pb2.BARCODE_UPC_A
    elif barcode_type == cv2.barcode.UPC_E:
        return scan_barcodes_pb2.BARCODE_UPC_E
    elif barcode_type == cv2.barcode.UPC_EAN_EXTENSION:
        return scan_barcodes_pb2.BARCODE_UPC_EAN_EXTENSION
    else:
        return scan_barcodes_pb2.BARCODE_UNSPECIFIED
    # [END convert_barcode_type_to_proto]


# [START barcode_detector]
class ScanBarcodes(skill_interface.Skill):
    """Skill that connects to a camera resource and scans all visible barcodes using OpenCV."""

    detector: cv2.barcode_BarcodeDetector

    def __init__(self) -> None:
        super().__init__()
        self.detector = cv2.barcode_BarcodeDetector()
        # [END barcode_detector]

    # [START camera_slot]
    @classmethod
    @overrides(skill_interface.Skill)
    def required_equipment(cls) -> Mapping[str, equipment_pb2.EquipmentSelector]:
        return {
            # Require one camera via a slot called "camera".
            CAMERA_EQUIPMENT_SLOT: equipment_pb2.EquipmentSelector(
                equipment_type_names=["CameraConfig"]
            )
        }
    # [END camera_slot]

    @classmethod
    @overrides(skill_interface.Skill)
    def name(cls) -> str:
        return "scan_barcodes"

    @classmethod
    @overrides(skill_interface.Skill)
    def package(cls) -> str:
        return "com.example"

    # [START access_equipment]
    @overrides(skill_interface.Skill)
    def execute(self, execute_request: skill_service_pb2.ExecuteRequest,
                context: skill_interface.ExecutionContext
                ) -> skill_service_pb2.ExecuteResult:
        # Get params.
        params = scan_barcodes_pb2.ScanBarcodesParams()
        proto_utils.unpack_any(execute_request.parameters, params)

        # Get equipment.
        camera_equipment: equipment_pb2.EquipmentHandle = (
            execute_request.instance.equipment_handles[CAMERA_EQUIPMENT_SLOT]
        )

        camera_config = camera_config_pb2.CameraConfig()
        camera_config_any = camera_equipment.equipment_data["CameraConfig"].contents
        camera_config_any.Unpack(camera_config)
        # [END access_equipment]

        # [START call_connect_to_camera]
        # Connect to the camera server over grpc.
        camera_stub, camera_handle = self.connect_to_camera(
            camera_equipment.connection_info.grpc, camera_config
        )
        # [END call_connect_to_camera]

        # [START call_grab_frame]
        # Grab a frame.
        frame = self.grab_frame(camera_stub, camera_handle)
        # [END call_grab_frame]

        # [START convert_frame_and_detect]
        # Convert frame to numpy array.
        image_buffer = frame.rgb8u

        img = np.frombuffer(image_buffer.data, dtype=np.uint8)
        img = img.reshape((
            image_buffer.dimensions.rows,
            image_buffer.dimensions.cols,
            image_buffer.num_channels,
        ))

        # Run the detector and check results.
        (ok, decoded_data, decoded_types, detected_corners) = (
            self.detector.detectAndDecode(img)
        )
        # [END convert_frame_and_detect]

        # [START call_convert_to_result_proto]
        # Convert result and return.
        result = self.convert_to_result_proto(
            ok, decoded_data, decoded_types, detected_corners
        )
        # [END call_convert_to_result_proto]

        logging.info("ScanBarcodesResult: %s", result)

        # [START execute_result]
        execute_result = skill_service_pb2.ExecuteResult()
        execute_result.result.Pack(result)
        return execute_result
        # [END execute_result]

    @overrides(skill_interface.Skill)
    def get_parameter_descriptor(self) -> descriptor.Descriptor:
        """Returns the descriptor for the parameter that this skill expects."""
        return scan_barcodes_pb2.ScanBarcodesParams.DESCRIPTOR

    @overrides(skill_interface.Skill)
    def get_return_value_descriptor(self) -> Optional[descriptor.Descriptor]:
        """Returns the descriptor for the value that this skill may output."""
        return scan_barcodes_pb2.ScanBarcodesResult.DESCRIPTOR

    # [START connect_to_camera]
    def connect_to_camera(
        self,
        grpc_info: equipment_pb2.EquipmentGrpcConnectionInfo,
        camera_config: camera_config_pb2.CameraConfig,
    ) -> Tuple[camera_server_pb2_grpc.CameraServerStub, str]:
        # Connect to the camera server over gRPC.
        options = [("grpc.max_receive_message_length", 150 * 1024 * 1024)]
        camera_channel = grpc.insecure_channel(grpc_info.address, options=options)
        connection_params = connection.ConnectionParams(
            grpc_info.address, grpc_info.server_instance, grpc_info.header
        )
        intercepted_camera_channel = grpc.intercept_channel(
            camera_channel,
            interceptor.HeaderAdderInterceptor(connection_params.headers),
        )
        camera_stub = camera_server_pb2_grpc.CameraServerStub(
            intercepted_camera_channel
        )

        # Access a camera instance.
        create_request = camera_server_pb2.CreateCameraRequest(
            camera_config=camera_config
        )
        create_response: camera_server_pb2.CreateCameraResponse = (
            camera_stub.CreateCamera(create_request)
        )
        camera_handle = create_response.camera_handle

        return camera_stub, camera_handle

    # [END connect_to_camera]

    # [START grab_frame]
    def grab_frame(
        self,
        camera_stub: camera_server_pb2_grpc.CameraServerStub,
        camera_handle: str,
    ) -> frame_pb2.Frame:
        frame_request: camera_server_pb2.GetFrameRequest = (
            camera_server_pb2.GetFrameRequest(camera_handle=camera_handle)
        )
        frame_response: camera_server_pb2.GetFrameResponse = camera_stub.GetFrame(
            frame_request
        )
        return frame_response.frame

    # [END grab_frame]

    # [START convert_to_result_proto]
    def convert_to_result_proto(
        self,
        ok: bool,
        decoded_data: List[str],
        decoded_types: List[int],
        detected_corners: np.ndarray,
    ) -> scan_barcodes_pb2.ScanBarcodesResult:
        if not ok:
            return scan_barcodes_pb2.ScanBarcodesResult()

        barcodes: List[scan_barcodes_pb2.Barcode] = []
        for i, barcode_type in enumerate(decoded_types):
            if barcode_type == cv2.barcode.NONE:
                continue

            barcode_data = decoded_data[i]
            barcode_corners = detected_corners[i]

            corners: List[scan_barcodes_pb2.Corner] = []
            for barcode_corner in barcode_corners:
                corner = scan_barcodes_pb2.Corner(
                    x=barcode_corner[0],
                    y=barcode_corner[1],
                )
                corners.append(corner)

            barcode = scan_barcodes_pb2.Barcode(
                type=convert_barcode_type_to_proto(barcode_type),
                data=barcode_data,
                corners=corners,
            )
            barcodes.append(barcode)

        return scan_barcodes_pb2.ScanBarcodesResult(barcodes=barcodes)

    # [END convert_to_result_proto]

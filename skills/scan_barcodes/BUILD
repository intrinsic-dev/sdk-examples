load("@ai_intrinsic_sdks//bazel:skills.bzl", "py_skill_image")
load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@scan_barcodes_pip_deps//:requirements.bzl", "requirement")

proto_library(
    name = "scan_barcodes_proto",
    srcs = ["scan_barcodes.proto"],
)

py_proto_library(
    name = "scan_barcodes_py_pb2",
    deps = [":scan_barcodes_proto"],
)

py_library(
    name = "scan_barcodes",
    srcs = ["scan_barcodes.py"],
    srcs_version = "PY3",
    deps = [
        ":scan_barcodes_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/perception/proto:camera_config_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/perception/proto:frame_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/perception/service/proto:camera_server_py_pb2_grpc",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:equipment_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:skill_service_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/python:proto_utils",
        "@ai_intrinsic_sdks//intrinsic/skills/python:skill_interface",
        "@ai_intrinsic_sdks//intrinsic/util:decorators",
        "@ai_intrinsic_sdks//intrinsic/util/grpc:connection",
        "@ai_intrinsic_sdks//intrinsic/util/grpc:interceptor",
        "@com_google_absl_py//absl/logging",
        "@com_google_protobuf//:protobuf_python",
        requirement("numpy"),
        requirement("opencv-contrib-python-headless"),
        requirement("opencv-python-headless"),
    ],
)

py_skill_image(
    name = "scan_barcodes_image",
    package_name = "com.example",
    parameter_proto = ":scan_barcodes_proto",
    skill = ":scan_barcodes",
    skill_module = "skills.scan_barcodes.scan_barcodes",
    skill_name = "scan_barcodes",
)

# Export for pip_parse repository rule
exports_files(["requirements.txt"])

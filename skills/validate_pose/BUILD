load("@ai_intrinsic_sdks//bazel:skills.bzl", "py_skill_image")
load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "validate_pose_proto",
    srcs = ["validate_pose.proto"],
    deps = ["@ai_intrinsic_sdks//intrinsic/world/proto:object_world_refs_proto"],
)

py_proto_library(
    name = "validate_pose_py_pb2",
    deps = [":validate_pose_proto"],
)

py_library(
    name = "validate_pose",
    srcs = ["validate_pose.py"],
    srcs_version = "PY3",
    deps = [
        ":validate_pose_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:equipment_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:skill_service_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/python:proto_utils",
        "@ai_intrinsic_sdks//intrinsic/skills/python:skill_interface",
        "@ai_intrinsic_sdks//intrinsic/util:decorators",
        "@ai_intrinsic_sdks//intrinsic/world/python:object_world_client",
        "@com_google_absl_py//absl/logging",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_skill_image(
    name = "validate_pose_image",
    package_name = "com.example",
    parameter_proto = ":validate_pose_proto",
    skill = ":validate_pose",
    skill_module = "skills.validate_pose.validate_pose",
    skill_name = "validate_pose",
)

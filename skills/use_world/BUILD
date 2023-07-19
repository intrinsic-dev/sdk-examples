load("@ai_intrinsic_sdks//bazel:skills.bzl", "py_skill_image")
load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "use_world_proto",
    srcs = ["use_world.proto"],
    deps = ["@ai_intrinsic_sdks//intrinsic/world/proto:object_world_refs_proto"],
)

py_proto_library(
    name = "use_world_proto_py_pb2",
    deps = [":use_world_proto"],
)

py_library(
    name = "use_world",
    srcs = ["use_world.py"],
    srcs_version = "PY3",
    deps = [
        ":use_world_proto_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/icon/equipment:equipment_utils_py",
        "@ai_intrinsic_sdks//intrinsic/math/python:data_types",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:equipment_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:skill_service_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/python:proto_utils",
        "@ai_intrinsic_sdks//intrinsic/skills/python:skill_interface",
        "@ai_intrinsic_sdks//intrinsic/util:decorators",
        "@ai_intrinsic_sdks//intrinsic/world/python:object_world_client",
        "@ai_intrinsic_sdks//intrinsic/world/python:object_world_resources",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_skill_image(
    name = "use_world_image",
    package_name = "com.example",
    parameter_proto = ":use_world_proto",
    skill = ":use_world",
    skill_module = "use_world",
    skill_name = "use_world",
)

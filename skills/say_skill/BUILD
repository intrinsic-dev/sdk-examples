load("@ai_intrinsic_sdks//bazel:skills.bzl", "cc_skill_image")
load("@ai_intrinsic_sdks//bazel:skills.bzl", "py_skill_image")
load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "say_skill_proto",
    srcs = ["say_skill.proto"],
)

py_proto_library(
    name = "say_skill_py_pb2",
    deps = [":say_skill_proto"],
)

py_library(
    name = "say_skill_py",
    srcs = ["say_skill.py"],
    srcs_version = "PY3",
    deps = [
        ":say_skill_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:equipment_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:skill_service_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/python:proto_utils",
        "@ai_intrinsic_sdks//intrinsic/skills/python:skill_interface",
        "@ai_intrinsic_sdks//intrinsic/util:decorators",
        "@com_google_absl_py//absl/logging",
        "@com_google_protobuf//:protobuf_python",
    ],
)

py_skill_image(
    name = "say_skill_py_image",
    package_name = "com.example",
    parameter_proto = ":say_skill_proto",
    skill = ":say_skill_py",
    skill_module = "skills.say_skill.say_skill",
    skill_name = "say_skill_py",
)

cc_proto_library(
    name = "say_skill_cc_proto",
    deps = [":say_skill_proto"],
)

cc_library(
    name = "say_skill_cc",
    srcs = ["say_skill.cc"],
    hdrs = ["say_skill.h"],
    deps = [
        ":say_skill_cc_proto",
        "@ai_intrinsic_sdks//intrinsic/icon/release:status_helpers",
        "@ai_intrinsic_sdks//intrinsic/skills/cc:skill_interface",
        "@ai_intrinsic_sdks//intrinsic/skills/cc:skill_registration",
        "@ai_intrinsic_sdks//intrinsic/skills/cc:skill_utils",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:equipment_cc_proto",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:skill_service_cc_proto",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/status:status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/synchronization",
        "@com_google_protobuf//:protobuf",
    ],
    alwayslink = True,
)

cc_skill_image(
    name = "say_skill_cc_image",
    package_name = "com.example",
    parameter_proto = ":say_skill_proto",
    skill = ":say_skill_cc",
    skill_name = "say_skill_cc",
)

load("@ai_intrinsic_sdks//bazel:skills.bzl", "cc_skill_image")
load("@ai_intrinsic_sdks//bazel:skills.bzl", "py_skill_image")
load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")
load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "wiggle_joint_proto",
    srcs = ["wiggle_joint.proto"],
)

cc_proto_library(
    name = "wiggle_joint_cc_proto",
    deps = [":wiggle_joint_proto"],
)

cc_library(
    name = "wiggle_joint_cc",
    srcs = ["wiggle_joint.cc"],
    hdrs = ["wiggle_joint.h"],
    deps = [
        ":wiggle_joint_cc_proto",
        # [START build_bazel_deps_p1]
        "@ai_intrinsic_sdks//intrinsic/icon/actions:point_to_point_move_info",
        "@ai_intrinsic_sdks//intrinsic/icon/cc_client:client",
        "@ai_intrinsic_sdks//intrinsic/icon/cc_client:client_utils",
        "@ai_intrinsic_sdks//intrinsic/icon/cc_client:session",
        "@ai_intrinsic_sdks//intrinsic/icon/equipment:channel_factory",
        "@ai_intrinsic_sdks//intrinsic/icon/equipment:equipment_utils",
        # [END build_bazel_deps_p1]
        "@ai_intrinsic_sdks//intrinsic/icon/release:status_helpers",
        # [START build_bazel_deps_p2]
        "@ai_intrinsic_sdks//intrinsic/skills/cc:equipment_pack",
        # [END build_bazel_deps_p2]
        "@ai_intrinsic_sdks//intrinsic/skills/cc:skill_interface",
        "@ai_intrinsic_sdks//intrinsic/skills/cc:skill_registration",
        "@ai_intrinsic_sdks//intrinsic/skills/cc:skill_utils",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:equipment_cc_proto",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:skill_service_cc_proto",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/status:status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_protobuf//:protobuf",
    ],
    alwayslink = True,
)

cc_skill_image(
    name = "wiggle_joint_cc_image",
    package_name = "com.example",
    parameter_proto = ":wiggle_joint_proto",
    skill = ":wiggle_joint_cc",
    skill_name = "wiggle_joint_cc",
)

py_proto_library(
    name = "wiggle_joint_py_pb2",
    deps = [":wiggle_joint_proto"],
)

py_library(
    name = "wiggle_joint_py",
    srcs = ["wiggle_joint.py"],
    srcs_version = "PY3",
    deps = [
        ":wiggle_joint_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/icon/actions:point_to_point_move_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/icon/equipment:equipment_utils_py",
        "@ai_intrinsic_sdks//intrinsic/icon/proto:joint_space_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/icon/python:icon",
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
    name = "wiggle_joint_py_image",
    package_name = "com.example",
    parameter_proto = ":wiggle_joint_proto",
    skill = ":wiggle_joint_py",
    skill_module = "skills.wiggle_joint.wiggle_joint",
    skill_name = "wiggle_joint_py",
)

load("@ai_intrinsic_sdks//bazel:skills.bzl", "py_skill", "skill_manifest")
load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_grpc_library", "py_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@ai_intrinsic_sdks//bazel:python_oci_image.bzl", "python_oci_image")
load("@ai_intrinsic_sdks//intrinsic/assets/services/build_defs:services.bzl", "intrinsic_service")
load("@stopwatch_pip_deps//:requirements.bzl", "requirement")

proto_library(
    name = "stopwatch_service_proto",
    srcs = ["stopwatch_service.proto"],
)

py_proto_library(
    name = "stopwatch_service_py_pb2",
    visibility = ["//visibility:public"],
    deps = [":stopwatch_service_proto"],
)

py_grpc_library(
    name = "stopwatch_service_py_pb2_grpc",
    srcs = [":stopwatch_service_proto"],
    grpc_library = requirement("grpcio"),
    visibility = ["//visibility:public"],
    deps = [":stopwatch_service_py_pb2"],
)

py_binary(
    name = "stopwatch_service_bin",
    srcs = ["stopwatch_service.py"],
    main = "stopwatch_service.py",
    deps = [
        ":stopwatch_service_py_pb2_grpc",
        "@ai_intrinsic_sdks//intrinsic/resources/proto:runtime_context_py_pb2",
        requirement("grpcio"),
    ],
)

python_oci_image(
    name = "stopwatch_service_image",
    base = "@distroless_python3",
    binary = "stopwatch_service_bin",
    entrypoint = [
        "python3",
        "-u",
        "/services/stopwatch/stopwatch_service_bin",
    ],
)

intrinsic_service(
    name = "stopwatch_service",
    images = [
        ":stopwatch_service_image.tar",
    ],
    manifest = ":stopwatch_service_manifest.textproto",
)

proto_library(
    name = "start_stopwatch_proto",
    srcs = ["start_stopwatch.proto"],
)

py_proto_library(
    name = "start_stopwatch_py_pb2",
    deps = [":start_stopwatch_proto"],
)

skill_manifest(
    name = "start_stopwatch_py_manifest",
    src = "start_stopwatch.manifest.textproto",
    deps = [":start_stopwatch_proto"],
)

py_library(
    name = "start_stopwatch",
    srcs = ["start_stopwatch.py"],
    srcs_version = "PY3",
    deps = [
        ":stopwatch_service_py_pb2_grpc",
        ":stopwatch_service_py_pb2",
        ":start_stopwatch_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:equipment_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/python:proto_utils",
        "@ai_intrinsic_sdks//intrinsic/skills/python:skill_interface",
        "@ai_intrinsic_sdks//intrinsic/util:decorators",
        "@com_google_absl_py//absl/logging",
        "@com_google_protobuf//:protobuf_python",
        "@ai_intrinsic_sdks//intrinsic/util/grpc:connection",
        "@ai_intrinsic_sdks//intrinsic/util/grpc:interceptor",
    ],
)

py_test(
    name = "start_stopwatch_test",
    size = "small",
    srcs = ["start_stopwatch_test.py"],
    main = "start_stopwatch_test.py",
    deps = [
        ":start_stopwatch",
    ],
)

py_skill(
    name = "start_stopwatch_image",
    manifest = ":start_stopwatch_py_manifest",
    deps = [
        ":start_stopwatch",
        ":start_stopwatch_py_pb2",
    ],
)

proto_library(
    name = "stop_stopwatch_proto",
    srcs = ["stop_stopwatch.proto"],
)

py_proto_library(
    name = "stop_stopwatch_py_pb2",
    deps = [":stop_stopwatch_proto"],
)

skill_manifest(
    name = "stop_stopwatch_py_manifest",
    src = "stop_stopwatch.manifest.textproto",
    deps = [":stop_stopwatch_proto"],
)

py_library(
    name = "stop_stopwatch",
    srcs = ["stop_stopwatch.py"],
    srcs_version = "PY3",
    deps = [
        ":stopwatch_service_py_pb2_grpc",
        ":stopwatch_service_py_pb2",
        ":stop_stopwatch_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/proto:equipment_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/skills/python:proto_utils",
        "@ai_intrinsic_sdks//intrinsic/skills/python:skill_interface",
        "@ai_intrinsic_sdks//intrinsic/util:decorators",
        "@com_google_absl_py//absl/logging",
        "@com_google_protobuf//:protobuf_python",
        "@ai_intrinsic_sdks//intrinsic/util/grpc:connection",
        "@ai_intrinsic_sdks//intrinsic/util/grpc:interceptor",
    ],
)

py_test(
    name = "stop_stopwatch_test",
    srcs = ["stop_stopwatch_test.py"],
    deps = [
        ":stop_stopwatch",
    ],
    main = "stop_stopwatch_test.py",
    size = "small",
)

py_skill(
    name = "stop_stopwatch_image",
    manifest = ":stop_stopwatch_py_manifest",
    deps = [
      ":stop_stopwatch",
      ":stop_stopwatch_py_pb2",
    ]
)

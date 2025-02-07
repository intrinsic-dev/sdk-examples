load("@ai_intrinsic_sdks//bazel:python_oci_image.bzl", "python_oci_image")
load("@rules_oci//oci:defs.bzl", "oci_load")
load("@ai_intrinsic_sdks//intrinsic/assets/services/build_defs:services.bzl", "intrinsic_service")
load("@rules_pkg//:pkg.bzl", "pkg_tar")
load("@container_structure_test//:defs.bzl", "container_structure_test")

filegroup(
    name = "frontend_files",
    srcs = glob(["frontend/**"]),
)

py_binary(
    name = "server",
    srcs = ["server.py"],
    main = "server.py",
    data = [":frontend_files"],
    deps = [
        "@ai_intrinsic_sdks//intrinsic/resources/proto:runtime_context_py_pb2",
        "@ai_intrinsic_sdks//intrinsic/executive/proto:executive_service_py_pb2_grpc",
    ],
)

pkg_tar(
    name = "server_layer",
    srcs = [":server"],
    strip_prefix = "/",
    include_runfiles = True,
    extension = "tar.gz",
)

python_oci_image(
    name = "hmi_image",
    binary = "server",
    base = "@distroless_python3",
    entrypoint = [
        "python3",
        "-u",
        "/services/hmi_python/server"
    ],
    data_path = "/frontend/",
    extra_tars = [":server_layer"],
)

oci_load(
    name = "hmi_tarball",
    image = ":hmi_image",
    repo_tags = ["hmi:latest"],
)

filegroup(
    name = "hmi_tarball.tar",
    srcs = [":hmi_tarball"],
    output_group = "tarball",
)

container_structure_test(
    name = "hmi_tarball_test",
    driver = "tar",
    configs = ["testdata/image_files.yaml"],
    image = ":hmi_tarball.tar",
)

intrinsic_service(
    name = "hmi_service",
    images = [":hmi_tarball.tar"],
    manifest = "manifest.textproto",
)

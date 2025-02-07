load("@io_bazel_rules_go//go:def.bzl", "go_binary")
load("@ai_intrinsic_sdks//intrinsic/assets/services/build_defs:services.bzl", "intrinsic_service")

filegroup(
    name = "frontend_files",
    srcs = glob(["angular-app/dist/hmi-angular/browser/**"]),
)

go_binary(
    name = "server",
    srcs = ["server.go"],
    data = [":frontend_files"],
    deps = [
        "@ai_intrinsic_sdks//intrinsic/util/proto:protoio",
        "@ai_intrinsic_sdks//intrinsic/resources/proto:runtime_context_go_proto",
        "@org_golang_google_grpc//:go_default_library",
        "@io_bazel_rules_go//go/runfiles",
        "@io_bazel_rules_go//go/tools/bazel",
        "@ai_intrinsic_sdks//intrinsic/executive/proto:executive_service_go_grpc_proto",
        "@ai_intrinsic_sdks//intrinsic/frontend/solution_service/proto:solution_service_go_grpc_proto",
        "@com_google_cloud_go_longrunning//autogen/longrunningpb",
        "@org_golang_google_grpc//credentials/insecure",
    ],
)

load("@rules_oci//oci:defs.bzl", "oci_image", "oci_load")
load("@rules_pkg//:pkg.bzl", "pkg_tar")

pkg_tar(
    name = "server_layer",
    srcs = [":server"],
    strip_prefix = "/",
    include_runfiles = True,
    extension = "tar.gz",
)

oci_image(
    name = "angular_hmi_image",
    base = "@distroless_base",
    entrypoint = ["/services/angular-hmi/server_/server"],
    tars = [":server_layer"],
)

oci_load(
    name = "angular_hmi_tarball",
    image = ":angular_hmi_image",
    repo_tags = ["hmi:latest"],
)

filegroup(
    name = "angular_hmi_tarball.tar",
    srcs = [":angular_hmi_tarball"],
    output_group = "tarball",
)

intrinsic_service(
    name = "hmi_angular_service",
    images = [":angular_hmi_tarball.tar"],
    manifest = "manifest.textproto",
)

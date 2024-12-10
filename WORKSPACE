workspace(name="examples")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "ai_intrinsic_sdks",
    remote = "https://github.com/intrinsic-ai/sdk.git",
    branch = "main",
)

# Load shared dependencies for Intrinsic SDKs. None of these is optional.
load("@ai_intrinsic_sdks//bazel:deps_0.bzl", "intrinsic_sdks_deps_0")
intrinsic_sdks_deps_0()
load("@ai_intrinsic_sdks//bazel:deps_1.bzl", "intrinsic_sdks_deps_1")
intrinsic_sdks_deps_1()
load("@ai_intrinsic_sdks//bazel:deps_2.bzl", "intrinsic_sdks_deps_2")
intrinsic_sdks_deps_2()
load("@ai_intrinsic_sdks//bazel:deps_3.bzl", "intrinsic_sdks_deps_3")
intrinsic_sdks_deps_3()

# [START load_python_rules]
load("@local_config_python//:defs.bzl", "interpreter")
load("@rules_python//python:pip.bzl", "pip_parse")
# [END load_python_rules]

# [START scan_barcodes_pip_deps]
pip_parse(
    name = "scan_barcodes_pip_deps",
    python_interpreter_target = interpreter,
    requirements_lock = "//skills/scan_barcodes:requirements.txt",
)

load("@scan_barcodes_pip_deps//:requirements.bzl", scan_barcodes_install_deps = "install_deps")
scan_barcodes_install_deps()
# [END scan_barcodes_pip_deps]

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "opencv",
    sha256 = "9dc6a9a95edc133e165e9f6db9412dd899e28d4e5e4979f17cb5966f4b7f3fb1",
    strip_prefix = "opencv-4.8.0",
    url = "https://github.com/opencv/opencv/archive/4.8.0.zip",
    build_file = "//:opencv.BUILD",
)

http_archive(
    name = "rules_foreign_cc",
    sha256 = "5303e3363fe22cbd265c91fce228f84cf698ab0f98358ccf1d95fba227b308f6",
    strip_prefix = "rules_foreign_cc-0.9.0",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/refs/tags/0.9.0.zip",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

load("@rules_oci//oci:pull.bzl", "oci_pull")

oci_pull(
    name = "distroless_python",
    digest = "sha256:e8e50bc861b16d916f598d7ec920a8cef1e35e99b668a738fe80c032801ceb78",
    image = "gcr.io/distroless/python3",
    platforms = [
        "linux/amd64",
    ],
)

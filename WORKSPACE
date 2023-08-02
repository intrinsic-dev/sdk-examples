workspace(name="examples")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "ai_intrinsic_sdks",
    remote = "https://github.com/intrinsic-dev/intrinsic_sdks",
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
load("@ai_intrinsic_sdks//bazel:python.bzl", "interpreter")
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

load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)

cmake(
    name = "opencv",
    generate_args = [
        "-GNinja",
        "-DBUILD_LIST=core,imgproc,calib3d,objdetect",
        "-D BUILD_SHARED_LIBS=OFF",
    ],
    lib_source = ":all_srcs",
    out_include_dir = "include/opencv4",
    out_static_libs	= [
        "libopencv_core.a",
        "libopencv_imgproc.a",
        "libopencv_calib3d.a",
        "libopencv_objdetect.a",
        "opencv4/3rdparty/libippicv.a",
        "opencv4/3rdparty/libippiw.a",
        "opencv4/3rdparty/libittnotify.a",
        "opencv4/3rdparty/liblibopenjp2.a",
        "opencv4/3rdparty/libquirc.a",
    ],
    visibility = ["//visibility:public"],
)

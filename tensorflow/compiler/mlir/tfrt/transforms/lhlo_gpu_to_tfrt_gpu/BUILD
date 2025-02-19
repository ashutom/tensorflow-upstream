load("@llvm-project//mlir:tblgen.bzl", "gentbl_cc_library")
load("//tensorflow:tensorflow.bzl", "get_compatible_with_cloud")
load("//tensorflow:tensorflow.bzl", "if_google")

# TF to TFRT kernels conversion.
package(licenses = ["notice"])

package_group(
    name = "friends",
    packages = if_google([
        "//learning/brain/experimental/mlir/tfrt_compiler/...",
        "//learning/brain/experimental/tfrt/...",
        "//platforms/xla/tests/gpu/...",
        "//third_party/tf_runtime_google/...",
    ]) + [
        "//tensorflow/compiler/...",
        "//tensorflow/core/runtime_fallback/...",
        "//tensorflow/core/tfrt/saved_model/...",
    ],
)

gentbl_cc_library(
    name = "TfrtGpuPassIncGen",
    compatible_with = get_compatible_with_cloud(),
    tbl_outs = [
        (
            [
                "-gen-pass-decls",
                "-name=TFRTGPU",
            ],
            "gpu_passes.h.inc",
        ),
    ],
    tblgen = "@llvm-project//mlir:mlir-tblgen",
    td_file = "gpu_passes.td",
    deps = [
        "@llvm-project//mlir:PassBaseTdFiles",
    ],
)

cc_library(
    name = "pass_details",
    hdrs = [
        "PassDetail.h",
    ],
    compatible_with = get_compatible_with_cloud(),
    deps = [
        ":TfrtGpuPassIncGen",
        "@llvm-project//mlir:GPUDialect",
        "@llvm-project//mlir:Pass",
        "@tf_runtime//:basic_kernels_opdefs",
        "@tf_runtime//backends/gpu:gpu_opdefs",
    ],
)

cc_library(
    name = "pass",
    srcs = [
        "gemm_pattern.cc",
        "gemm_pattern.h",
        "gpu_passes.cc",
    ],
    hdrs = ["gpu_passes.h"],
    compatible_with = get_compatible_with_cloud(),
    tags = [
        "gpu",
        "no_oss",
    ],
    visibility = [":friends"],
    deps = [
        ":pass_details",
        "//tensorflow/compiler/mlir/hlo:lhlo",
        "//tensorflow/compiler/mlir/hlo:lhlo_gpu",
        "//tensorflow/compiler/mlir/tensorflow",
        "//tensorflow/compiler/mlir/xla:type_to_shape",
        "//tensorflow/compiler/xla:shape_util",
        "@com_google_absl//absl/strings",
        "@llvm-project//llvm:Support",
        "@llvm-project//mlir:GPUDialect",
        "@llvm-project//mlir:IR",
        "@llvm-project//mlir:MemRefDialect",
        "@llvm-project//mlir:Pass",
        "@llvm-project//mlir:Rewrite",
        "@llvm-project//mlir:StandardOps",
        "@llvm-project//mlir:Support",
        "@llvm-project//mlir:Transforms",
        "@tf_runtime//:basic_kernels_opdefs",
        "@tf_runtime//backends/gpu:gpu_opdefs",
        "@tf_runtime//backends/gpu:gpu_passes",
        "@tf_runtime//backends/gpu:gpu_wrapper",
    ],
    alwayslink = 1,
)

cc_library(
    name = "passes",
    hdrs = ["register_passes.h"],
    tags = [
        "gpu",
        "no_oss",
    ],
    visibility = [":friends"],
    deps = [
        ":TfrtGpuPassIncGen",
        ":pass",
        "@llvm-project//mlir:Pass",
    ],
)

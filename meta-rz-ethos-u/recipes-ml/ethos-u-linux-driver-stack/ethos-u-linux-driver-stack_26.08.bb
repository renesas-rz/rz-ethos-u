SUMMARY = "Linux Driver Stack for Arm(R) Ethos(TM)-U"
DESCRIPTION = "The Linux driver stack for Arm(R) Ethos(TM)-U NPU contains drivers, \
libraries and applications to make the NPU available for use in Linux userspace."
HOMEPAGE = "https://developer.arm.com/documentation/109267/0101/Arm-Ethos-U-NPU/ML-software-support-for-Ethos-U"

# Apache-2.0 license applies to all Ethos-U software components used
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = " \
	file://LICENSE-APACHE-2.0.txt;md5=e3fc50a88d0a364313df4b21ef20c29e \
"

DEPENDS = " \
	flatbuffers \
	flatbuffers-native \
	protobuf-tflite-native \
	patchelf-native \
	python3 \
	python3-numpy \
	python3-pybind11 \
	virtual/kernel \
"

# Ethos-U Linux Driver Stack
SRC_URI = " \
	git://git.gitlab.arm.com/artificial-intelligence/ethos-u/ethos-u-linux-driver-stack.git;protocol=https;branch=main;name=ethos-u-linux-driver-stack \
	file://0001-driver_library-drm-Add-Arm-Ethos-U55-NPU-support.patch \
	file://0002-Install-all-Ethos-U-delegate-components.patch \
	file://0003-thirdparty-CMakeLists.txt-Build-and-install-tflite_runtime.patch \
	file://0004-lite-Update-CMakeLists-for-building-shared-object-library.patch;patchdir=thirdparty/tensorflow \
	file://0005-lite-Add-missing-absl-check-and-absl-log-dependencie.patch;patchdir=thirdparty/tensorflow \
	file://0006-thirdparty-CMakeLists.txt-Build-and-install-benchmar.patch \
"

# Download TensorFlow into the thirdparty/ directory of the Ethos-U Linux Driver Stack
# along with its dependencies
SRC_URI += " \
	git://github.com/tensorflow/tensorflow.git;protocol=https;branch=r2.20;name=tensorflow;destsuffix=git/thirdparty/tensorflow \
	git://github.com/Maratyszcza/FP16.git;protocol=https;branch=master;name=FP16;destsuffix=deps/FP16 \
	git://github.com/Maratyszcza/FXdiv.git;protocol=https;branch=master;name=FXdiv;destsuffix=deps/FXdiv \
	git://github.com/abseil/abseil-cpp.git;protocol=https;branch=lts_2025_01_27;name=abseilcpp;destsuffix=deps/abseil-cpp \
	git://gitlab.com/libeigen/eigen.git;protocol=https;branch=master;name=eigen;destsuffix=deps/eigen \
	git://github.com/pytorch/cpuinfo.git;protocol=https;branch=main;name=cpuinfo;destsuffix=deps/cpuinfo \
	git://github.com/google/farmhash.git;protocol=https;branch=master;name=farmhash;destsuffix=deps/farmhash \
	git://github.com/petewarden/OouraFFT.git;protocol=https;branch=master;name=fft2d;destsuffix=deps/fft2d \
	git://github.com/google/flatbuffers.git;protocol=https;branch=master;name=flatbuffers;destsuffix=deps/flatbuffers \
	git://github.com/google/gemmlowp.git;protocol=https;branch=master;name=gemmlowp;destsuffix=deps/gemmlowp \
	git://git.gitlab.arm.com/kleidi/kleidiai.git;protocol=https;branch=main;name=kleidiai;destsuffix=deps/kleidiai \
	git://github.com/jax-ml/ml_dtypes.git;protocol=https;branch=main;name=mldtypes;destsuffix=deps/ml_dtypes \
	git://github.com/protocolbuffers/protobuf.git;protocol=https;branch=main;name=protobuf;destsuffix=deps/protobuf \
	git://github.com/google/pthreadpool.git;protocol=https;branch=main;name=pthreadpool;destsuffix=deps/pthreadpool \
	git://github.com/google/ruy.git;protocol=https;branch=master;name=ruy;destsuffix=deps/ruy \
	git://github.com/google/XNNPACK.git;protocol=https;branch=master;name=xnnpack;destsuffix=deps/xnnpack \
"

SRCREV_FORMAT = "ethos-u-linux-driver-stack"

# v26.08
SRCREV_ethos-u-linux-driver-stack = "2dfd9fbbcc308571eee337ca91a69cfdc6df293f"

# TensorFlow v2.20 and its dependencies
SRCREV_tensorflow = "72fbba3d20f4616d7312b5e2b7f79daf6e82f2fa"
SRCREV_FP16 = "4dfe081cf6bcd15db339cf2680b9281b8451eeb3"
SRCREV_FXdiv = "63058eff77e11aa15bf531df5dd34395ec3017c8"
SRCREV_abseilcpp = "d9e4955c65cd4367dd6bf46f4ccb8cd3d100540b"
SRCREV_cpuinfo = "de0ce7c7251372892e53ce9bc891750d2c9a4fd8"
SRCREV_eigen = "4c38131a16803130b66266a912029504f2cf23cd"
SRCREV_farmhash = "0d859a811870d10f53a594927d0d0b97573ad06d"
SRCREV_fft2d = "c6fd2dd6d21397baa6653139d31d84540d5449a2"
SRCREV_flatbuffers = "e6463926479bd6b330cbcf673f7e917803fd5831"
SRCREV_gemmlowp = "16e8662c34917be0065110bfcd9cc27d30f52fdf"
SRCREV_kleidiai = "dc69e899945c412a8ce39ccafd25139f743c60b1"
SRCREV_mldtypes = "00d98cd92ade342fef589c0470379abb27baebe9"
SRCREV_protobuf = "90b73ac3f0b10320315c2ca0d03a5a9b095d2f66"
SRCREV_pthreadpool = "c2ba5c50bb58d1397b693740cf75fad836a0d1bf"
SRCREV_ruy = "3286a34cc8de6149ac6844107dfdffac91531e72"
SRCREV_xnnpack = "585e73e63cb35c8a416c83a48ca9ab79f7f7d45e"

S = "${WORKDIR}/git"

inherit cmake python3-dir python3native

COMPATIBLE_MACHINE = "smarc-rzg3e"
PACKAGE_ARCH = "${MACHINE_ARCH}"
TF_VERSION = "2.20.0"

do_configure[depends] += "virtual/kernel:do_shared_workdir"

EXTRA_OECMAKE = " \
	-DBUILD_KERNEL=OFF \
	-DBUILD_MAILBOX=OFF \
	-DBUILD_REMOTEPROC=OFF \
	-DBUILD_DELEGATE=ON \
	-DDRIVER_BACKEND=drm \
	-DRUN_SPARSE=OFF \
	-DDRM_UAPI_INCLUDE_DIR=${STAGING_KERNEL_BUILDDIR}/kernel-uapi-headers/include \
	-DTF_MAJOR_VERSION=${@d.getVar('TF_VERSION').split('.')[0]} \
	-DTF_MINOR_VERSION=${@d.getVar('TF_VERSION').split('.')[1]} \
	-DTF_PATCH_VERSION=${@d.getVar('TF_VERSION').split('.')[2]} \
	-DTFLITE_ENABLE_GPU=OFF \
	-DTFLITE_ENABLE_NNAPI=OFF \
	-DFETCHCONTENT_FULLY_DISCONNECTED=ON \
	-DFP16_SOURCE_DIR=${WORKDIR}/deps/FP16 \
	-DFXDIV_SOURCE_DIR=${WORKDIR}/deps/FXdiv \
	-DFETCHCONTENT_SOURCE_DIR_ABSEIL-CPP=${WORKDIR}/deps/abseil-cpp \
	-DFETCHCONTENT_SOURCE_DIR_CPUINFO=${WORKDIR}/deps/cpuinfo \
	-DFETCHCONTENT_SOURCE_DIR_EIGEN=${WORKDIR}/deps/eigen \
	-DFETCHCONTENT_SOURCE_DIR_FARMHASH=${WORKDIR}/deps/farmhash \
	-DFETCHCONTENT_SOURCE_DIR_FFT2D=${WORKDIR}/deps/fft2d \
	-DFETCHCONTENT_SOURCE_DIR_FLATBUFFERS=${WORKDIR}/deps/flatbuffers \
	-DFETCHCONTENT_SOURCE_DIR_GEMMLOWP=${WORKDIR}/deps/gemmlowp \
	-DKLEIDIAI_SOURCE_DIR=${WORKDIR}/deps/kleidiai \
	-DFETCHCONTENT_SOURCE_DIR_ML_DTYPES=${WORKDIR}/deps/ml_dtypes \
	-DFETCHCONTENT_SOURCE_DIR_PROTOBUF=${WORKDIR}/deps/protobuf \
	-DPTHREADPOOL_SOURCE_DIR=${WORKDIR}/deps/pthreadpool \
	-DFETCHCONTENT_SOURCE_DIR_RUY=${WORKDIR}/deps/ruy \
	-DFETCHCONTENT_SOURCE_DIR_XNNPACK=${WORKDIR}/deps/xnnpack \
	-DBUILD_TFLITE_RUNTIME=ON \
	-DBUILD_BENCHMARK_MODEL=ON \
	-DProtobuf_PROTOC_EXECUTABLE=${STAGING_BINDIR_NATIVE}/protoc-tflite \
	-DPYTHON_INCLUDE_DIR=${STAGING_INCDIR}/python${PYTHON_BASEVERSION} \
	-DPYBIND11_INCLUDE_DIR=${STAGING_INCDIR} \
	-DNUMPY_INCLUDE_DIR=${STAGING_DIR_TARGET}${libdir}/python${PYTHON_BASEVERSION}/site-packages/numpy/core/include \
	-DTFLITE_RUNTIME_INSTALL_DIR=${libdir}/python${PYTHON_BASEVERSION}/site-packages/tflite_runtime \
"

EXTRA_PREFIX_MAP = " \
	-fdebug-prefix-map=${WORKDIR}/deps=/usr/src/debug/${PN}/${PV}/deps \
	-fmacro-prefix-map=${WORKDIR}/deps=/usr/src/debug/${PN}/${PV}/deps \
	-ffile-prefix-map=${STAGING_KERNEL_BUILDDIR}=/usr/src/kernel-headers \
"
TARGET_CFLAGS += "${EXTRA_PREFIX_MAP}"
TARGET_CXXFLAGS += "${EXTRA_PREFIX_MAP}"

do_configure() {
	cmake_do_configure
}

# Use make instead of ninja
OECMAKE_GENERATOR = "Unix Makefiles"

do_compile() {
	cmake_do_compile
}

do_install() {
	cmake_do_install

	# Install Ethos-U delegate library for runtime access
	install -m 0755 ${B}/delegate/libethosu_op_delegate.so ${D}${libdir}/libethosu_op_delegate.so.1.${PV}
	patchelf --set-soname libethosu_op_delegate.so.1 ${D}${libdir}/libethosu_op_delegate.so.1.${PV}
	ln -sf libethosu_op_delegate.so.1.${PV}  ${D}${libdir}/libethosu_op_delegate.so.1
	ln -sf libethosu_op_delegate.so.1 ${D}${libdir}/libethosu_op_delegate.so

	# Install the Python tflite_runtime sources
	install -d ${D}${libdir}/python${PYTHON_BASEVERSION}/site-packages/tflite_runtime
	install -m 0644 ${S}/thirdparty/tensorflow/tensorflow/lite/python/interpreter.py \
		${S}/thirdparty/tensorflow/tensorflow/lite/python/metrics/metrics_interface.py \
		${S}/thirdparty/tensorflow/tensorflow/lite/python/metrics/metrics_portable.py \
		${D}${libdir}/python${PYTHON_BASEVERSION}/site-packages/tflite_runtime/

	echo "__version__ = '${TF_VERSION}'" > ${D}${libdir}/python${PYTHON_BASEVERSION}/site-packages/tflite_runtime/__init__.py

	cd ${S}/thirdparty/tensorflow

	# Install TensorFlow Lite C++ headers
	install -d ${D}${includedir}/tensorflow/lite
	install -d ${D}${includedir}/tensorflow/compiler/mlir/lite
	cp --parents $(find tensorflow/lite -name "*.h*" -not -path "*/python/*") ${D}${includedir}/
	cp --parents $(find tensorflow/compiler/mlir/lite -name "*.h*") ${D}${includedir}/

	# Remove ethosu_logd application since it is not compatible with direct drive
	rm -f ${D}${bindir}/ethosu_logd
}

FILES:${PN} = " \
	/usr/LICENSE \
	${bindir}/benchmark_model \
	${bindir}/delegate_runner \
	${libdir}/libethosu_op_delegate.so.* \
	${libdir}/libtensorflow-lite.so.* \
	${libdir}/python${PYTHON_BASEVERSION}/site-packages/tflite_runtime \
"
FILES:${PN}-dev = " \
	${libdir}/libethosu_op_delegate.so \
	${libdir}/libtensorflow-lite.so \
	${includedir}/ethosu.hpp \
	${includedir}/tensorflow \
"
FILES:${PN}-staticdev = " \
	${libdir}/libethosu.a \
	${libdir}/libtensorflow-lite.a \
	${libdir}/libxnnpack-delegate.a \
"

RDEPENDS:${PN} = " \
	python3-numpy \
"

# Ethos-U CMake build system does not generate a .so file, so let's make
# sure the .a library goes where package -dev goes.
RDEPENDS:${PN}-dev += "${PN}-staticdev"

# Install FlatBuffers headers as it is required for compiling TensorFlow
# applications
RDEPENDS:${PN}-dev += "flatbuffers-dev"

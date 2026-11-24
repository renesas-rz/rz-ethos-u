SUMMARY = "Compiler for Arm Ethos-U NPU edge AI devices"
DESCRIPTION = "Used to convert neural networks defined in TOSA or \
TensorFlow Lite/LiteRT format into Ethos-U command streams."
HOMEPAGE = "https://gitlab.arm.com/artificial-intelligence/ethos-u/ethos-u-vela"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=e3fc50a88d0a364313df4b21ef20c29e"

DEPENDS += "python3-numpy-native python3-setuptools-scm-native"

SRC_URI += " \
	file://0001-pyproject.toml-Use-table-form-for-license.patch \
	file://0002-ethosu-regor-cmake-cpack_config.cmake-Support-cross-.patch \
	file://RZG3E_vela.ini \
	file://generate-model-data.py \
"

SRC_URI[sha256sum] = "d1228738674913ed866a136a6002f93a4b7248b23402fa5b52caa8cfef138eaa"

PYPI_PACKAGE = "ethos_u_vela"

inherit pypi cmake python_setuptools_build_meta python3targetconfig

export CMAKE_ARGS = " -DPython3_INCLUDE_DIR=${STAGING_INCDIR}/${PYTHON_DIR}${PYTHON_ABI}"

# Skip dependency check as dependencies are provided via Yocto
PEP517_BUILD_OPTS = "--skip-dependency-check"

do_install:append() {
	install -d ${D}${bindir}
	install -m 0555 ${WORKDIR}/generate-model-data.py ${D}${bindir}/

	install -d ${D}${datadir}/vela
	install -m 0644 ${WORKDIR}/RZG3E_vela.ini ${D}${datadir}/vela/
}

FILES:${PN} = " \
	${bindir}/generate-model-data.py \
	${bindir}/vela \
	${datadir}/vela \
	${libdir}/python${PYTHON_BASEVERSION}/site-packages/ethosu \
	${libdir}/python${PYTHON_BASEVERSION}/site-packages/ethos_u_vela-${PV}.dist-info \
"

INSANE_SKIP:${PN} = "already-stripped"

# The driver stack is needed for tflite_runtime, which is imported by
# generate-model-data.py. The Vela compiler itself does not need it.
RDEPENDS:${PN} += " \
	ethos-u-linux-driver-stack \
	python3-flatbuffers \
	python3-numpy \
	python3-pyyaml \
"

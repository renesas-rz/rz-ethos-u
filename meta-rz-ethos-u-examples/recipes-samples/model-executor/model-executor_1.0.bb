SUMMARY = "Ethos-U Model Executor Sample Application"
DESCRIPTION = "Model Executor application that runs inference using the models defined \
in the configuration YAML. The application uses the Arm Ethos-U delegate library and \
mainline Ethos-U linux kernel driver to run the models on the Arm(R) Ethos(TM)-U55."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "\
	file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10 \
"

SRC_URI = " \
	file://c++/ \
	file://python/model-executor.py \
"

DEPENDS = " \
	ethos-u-linux-driver-stack \
	yaml-cpp \
"

S = "${WORKDIR}/c++"

inherit cmake

COMPATIBLE_MACHINE = "smarc-rzg3e"
PACKAGE_ARCH = "${MACHINE_ARCH}"

do_install:append() {
	# Install Python application
	# C++ application is automatically installed by Yocto via cmake bbclass
	install -d ${D}${bindir}

	install -m 0555 ${WORKDIR}/python/model-executor.py ${D}${bindir}/
}

FILES:${PN} = " \
	${bindir}/model-executor \
	${bindir}/model-executor.py \
"

RDEPENDS:${PN} = " \
	ethos-u-linux-driver-stack \
	mobilenet-v2 \
	python3-numpy \
	python3-pyyaml \
"

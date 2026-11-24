SUMMARY = "Mobilenet V2 model compiled with Arm Ethos-U Vela Compiler"
DESCRIPTION = "Model compiled using the generate-model-data-host.sh \
script in the scripts/vela/ directory of this repository"

# Apache-2.0 License applies to both the Arm ML Zoo model and Vela compiler
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WORKDIR}/LICENSE;md5=40e8ed09ad30fce3c78c51bcfa0b2284"

SRC_URI = " \
	https://gitlab.arm.com/artificial-intelligence/ethos-u/ml-embedded-evaluation-kit/-/raw/24.08/resources/img_class/labels/labels_mobilenet_v2_1.0_224.txt;name=labels;subdir=labels \
	file://mobilenet_v2/ \
	file://LICENSE \
"

SRC_URI[labels.sha256sum] = "9b67e8dd5ecb3c003f864c8df2661c485ee38dd4e3b0d8a4af0bc36bd23b97d8"

inherit allarch

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install() {
	install -d ${D}${datadir}/models
	cp -r --preserve=mode,timestamps ${WORKDIR}/mobilenet_v2/ ${D}${datadir}/models/

	# Install MobileNet v2 label file
	install -m 0644 ${WORKDIR}/labels/labels_mobilenet_v2_1.0_224.txt ${D}${datadir}/models/mobilenet_v2/
}

FILES:${PN} = " \
	${datadir}/models/mobilenet_v2/ \
"

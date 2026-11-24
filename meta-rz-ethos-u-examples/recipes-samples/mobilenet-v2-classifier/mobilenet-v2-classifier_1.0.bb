SUMMARY = "Ethos-U MobileNet V2 Classifier Sample Applications"
DESCRIPTION = "Application that runs inference on an image file using MobileNet V2. \
The application uses TensorFlow Lite with the Ethos-U delegate to run inference on \
the Arm(R) Ethos(TM)-U55. There are 2 different versions of the application: C++ \
(Console or Display) and Python (Console only)."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = " \
	file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10 \
"

DEPENDS = " \
	ethos-u-linux-driver-stack \
	opencv \
	wayland \
	xdg-shell-client-protocol \
"

SRC_URI = " \
	file://c++/ \
	file://python/mobilenet-v2-classifier.py \
"

S = "${WORKDIR}/c++"

inherit cmake pkgconfig

COMPATIBLE_MACHINE = "smarc-rzg3e"
PACKAGE_ARCH = "${MACHINE_ARCH}"

do_install:append() {
	# Install Python application
	# C++ application is automatically installed by Yocto via cmake bbclass
	install -d ${D}${bindir}
	install -m 0555 ${WORKDIR}/python/mobilenet-v2-classifier.py ${D}${bindir}/
}

FILES:${PN} = " \
	${bindir} \
"

RDEPENDS:${PN} = " \
	ethos-u-linux-driver-stack \
	ml-sample-images \
	mobilenet-v2 \
	libopencv-core \
	libopencv-imgcodecs \
	libopencv-imgproc \
	opencv \
	python3-numpy \
	python3-opencv \
"

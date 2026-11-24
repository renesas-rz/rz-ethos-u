SUMMARY = "Sample images used by Ethos-U example applications"
DESCRIPTION = "Sample images intended for machine learning usage"

# Apache 2.0 License applies to grace_hopper.jpg image
# Creative Commons Attribution 1.0 Generic applies to the other images
LICENSE = "Apache-2.0 & CC-BY-1.0"
LIC_FILES_CHKSUM = " \
	file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10 \
	file://${COMMON_LICENSE_DIR}/CC-BY-1.0;md5=e1b141d75eb69dca77dedffa72ab8099 \
"

SRC_URI = " \
	https://gitlab.arm.com/artificial-intelligence/ethos-u/ml-embedded-evaluation-kit/-/raw/24.08/resources/img_class/samples/cat.bmp;name=cat;subdir=images \
	https://gitlab.arm.com/artificial-intelligence/ethos-u/ml-embedded-evaluation-kit/-/raw/24.08/resources/img_class/samples/dog.bmp;name=dog;subdir=images \
	https://gitlab.arm.com/artificial-intelligence/ethos-u/ml-embedded-evaluation-kit/-/raw/24.08/resources/img_class/samples/kimono.bmp;name=kimono;subdir=images \
	https://gitlab.arm.com/artificial-intelligence/ethos-u/ml-embedded-evaluation-kit/-/raw/24.08/resources/img_class/samples/tiger.bmp;name=tiger;subdir=images \
	https://raw.githubusercontent.com/tensorflow/tensorflow/v2.20.0/tensorflow/examples/label_image/data/grace_hopper.jpg;name=grace_hopper;subdir=images \
"

SRC_URI[cat.sha256sum] = "30600df720ba8527cfbb3e412269d1fa3e4cec41e36f28d48aef90b4b097cf7f"
SRC_URI[dog.sha256sum] = "de46b7888b0bf8c415c7df4ea96903051167df8dbd8d569a32c4ca0944d9d514"
SRC_URI[kimono.sha256sum] = "b4f8f4175d8b61a659983caa5e10883f7e3535a41da9382efcb10fd50783f8c5"
SRC_URI[tiger.sha256sum] = "aa4304903a3cd4c049915639416f084cee7a24204d88d698b78f3d7821cf3bb4"
SRC_URI[grace_hopper.sha256sum] = "a8ca6d734765703b09728ab47fe59f473d93ae3967fc24c7c0288c3c7adb7130"

inherit allarch

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install() {
	# Install sample images to an image-classification directory
	install -d ${D}${datadir}/image-classification/images
	install -m 0644 ${WORKDIR}/images/* ${D}${datadir}/image-classification/images/
}

FILES:${PN} = " \
	${datadir}/image-classification \
"

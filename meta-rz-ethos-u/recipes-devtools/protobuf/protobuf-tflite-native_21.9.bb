SUMMARY = "Protocol Buffers - structured data serialisation mechanism"
DESCRIPTION = "Protocol Buffers are a way of encoding structured data in an \
efficient yet extensible format. This native-only build provides protoc \
pinned to the exact commit that ethos-u-linux-driver-stack vendors for its \
target libprotobuf, so that protoc-generated code always matches the \
version of the runtime it will be linked against."
HOMEPAGE = "https://github.com/protocolbuffers/protobuf"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=37b5762e07f0af8c74ce80a8bda4266b"

# Version compatible with TensorFlow Lite v2.20
SRC_URI = "git://github.com/protocolbuffers/protobuf.git;branch=main;protocol=https"
SRCREV = "90b73ac3f0b10320315c2ca0d03a5a9b095d2f66"

S = "${WORKDIR}/git"

inherit cmake native

EXTRA_OECMAKE = " \
	-Dprotobuf_BUILD_TESTS=OFF \
	-Dprotobuf_BUILD_EXAMPLES=OFF \
	-Dprotobuf_BUILD_PROTOC_BINARIES=ON \
	-Dprotobuf_BUILD_SHARED_LIBS=OFF \
	-Dprotobuf_WITH_ZLIB=OFF \
"
# Install protoc under a unique name so it does not clash with meta-oe
do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${B}/protoc ${D}${bindir}/protoc-tflite
}

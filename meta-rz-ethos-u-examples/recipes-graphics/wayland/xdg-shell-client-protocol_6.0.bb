SUMMARY = "Generated Wayland protocol code for the XDG shell"
DESCRIPTION = "Wayland Client header and protocol code generated from the \
xdg-shell.xml in wayland-protocols that is needed by applications using XDG shell \
with Wayland."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = " \
	wayland \
	wayland-native \
	wayland-protocols \
"

S = "${WORKDIR}"

do_configure[noexec] = "1"

do_compile() {
	xdg_xml=${STAGING_DATADIR}/wayland-protocols/stable/xdg-shell/xdg-shell.xml

	wayland-scanner client-header \
		${xdg_xml} \
		xdg-shell-client-protocol.h

	wayland-scanner private-code \
		${xdg_xml} \
		xdg-shell-client-protocol.c

	# Generate a static library that can be used for compilation
	${CC} ${CFLAGS} -fPIC -c xdg-shell-client-protocol.c -o xdg-shell-client-protocol.o
	${AR} rcs libxdg-shell-client-protocol.a xdg-shell-client-protocol.o
}

do_install() {
	install -d ${D}${includedir}
	install -m 0644 \
		xdg-shell-client-protocol.h \
		${D}${includedir}/

	install -d ${D}${libdir}
	install -m 0644 \
		libxdg-shell-client-protocol.a \
		${D}${libdir}/
}

FILES:${PN}-dev = "${includedir}"
FILES:${PN}-staticdev = "${libdir}/libxdg-shell-client-protocol.a"
ALLOW_EMPTY:${PN} = "1"

# Package the .a library with the -dev package.
RDEPENDS:${PN}-dev += "${PN}-staticdev"

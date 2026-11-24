SUMMARY = "Ethos-U application dependencies"
DESCRIPTION = "Static libraries required by Ethos-U example and demo \
applications that are added to the SDK."

LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
	opencv-staticdev \
	xdg-shell-client-protocol-staticdev \
"

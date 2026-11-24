SUMMARY = "Ethos-U on-target tools"
DESCRIPTION = "Tools for converting models with Vela and for creating input \
data for the Ethos-U examples and benchmarks directly on the board."

LICENSE = "MIT"

inherit packagegroup

# numpy, pillow and pyyaml are listed explicitly for user-written scripts
# that create model input data on the board, not just as Vela dependencies
RDEPENDS:${PN} = " \
	ethos-u-vela \
	python3-numpy \
	python3-pillow \
	python3-pyyaml \
"

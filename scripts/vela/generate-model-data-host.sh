#! /bin/bash
# This script is a wrapper of generate-model-data.py found in
# meta-rz-ethos-u/recipes-ethos-u/ethos-u-vela/ethos-u-vela/.
# It is intended to be executed from the Host PC (offline
# compilation).
# The script provides all command-line arguments to
# generate-model-data.py. E.G.
# ./generate-model-data-host.sh -d output_dir -m model1.tflite model2.tflite

REPO_DIRECTORY="$(cd "$(dirname "$0")/../.."; pwd)"
COMPILATION_SCRIPT="${REPO_DIRECTORY}/meta-rz-ethos-u/recipes-devtools/ethos-u-vela/ethos-u-vela/generate-model-data.py"

exec python3 "${COMPILATION_SCRIPT}" "$@"

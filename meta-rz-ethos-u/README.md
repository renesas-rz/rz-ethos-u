# meta-rz-ethos-u

This OpenEmbedded/Yocto layer provides the software and drivers needed
for Ethos-U support on the Renesas RZ/G3E SMARC Evaluation platform.

It is designed to add support for the Arm Ethos-U Linux Driver Stack allowing
for Machine Learning (ML) inferences to be dispatched directly from Linux to
the Arm Ethos-U55 NPU.

The layer also adds support for Vela compilation to be done on the target,
Arm's example application (`delegate_runner`) and the TensorFlow benchmark
tool (`benchmark_model`) for model evaluation.

Further information about how to use this meta-layer can be found in the
top-level [README.md](../README.md) of this repository.


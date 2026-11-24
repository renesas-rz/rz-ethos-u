# rz-ethos-u
This repository contains two OpenEmbedded/Yocto layers
(meta-rz-ethos-u, meta-rz-ethos-u-examples), which add Ethos-U
and example application support to the Renesas RZ/G3E SMARC
Evaluation platform.

The `meta-rz-ethos-u` layer enables Ethos-U functionality and adds
Ethos-U Vela compilation support to the RZ/G3E.

The `meta-rz-ethos-u-examples` layer is dependent on the
`meta-rz-ethos-u` layer and adds support for example applications
(MobileNet V2 Classifier and Model Executor), OpenCV and allows Graphical
User Interface (GUI) applications to use the XDG shell protocol
for Wayland.

Both layers are based on
[Renesas BSP-4.0.2](https://github.com/renesas-rz/meta-renesas/tree/BSP-4.0.2).

The repository also hosts an [Offline Compilation Environment](scripts/vela/)
that allows for TensorFlow Lite models to be compiled using the Arm Ethos-U
Vela compiler.

## Dependencies
| Name | Layers | Repository | Branch | Revision |
| --- | --- | --- | --- | --- |
| meta-renesas | meta-rz-bsp<br>meta-rz-distro | https://github.com/renesas-rz/meta-renesas | BSP-4.0.2 | 2655d4aaa30be9b5b293e9df14393c6c4b7ffddb |
| poky | meta<br>meta-poky<br>meta-yocto-bsp | https://git.yoctoproject.org/git/poky | scarthgap | 7e8674996b0164b07e56bc066d0fba790e627061 |
| meta-arm | meta-arm-toolchain<br>meta-arm | https://git.yoctoproject.org/meta-arm | scarthgap | 8e0f8af90fefb03f08cd2228cde7a89902a6b37c |
| meta-openembedded | meta-multimedia<br>meta-oe<br>meta-python | https://git.openembedded.org/meta-openembedded | scarthgap | 89a01c3d9ad1f8fce6aeb4dd0e694cfa28d42099 |

`meta-rz-ethos-u-examples` is dependent on `meta-rz-ethos-u` and
uses the same version found in this repository revision that is
checked out.

Ethos-U demo applications are provided by the separate
[meta-rz-ethos-u-demos](https://github.com/renesas-rz/meta-rz-ethos-u-demos)
layer, which is dependent on `meta-rz-ethos-u`. It is fetched by Kas when the
`features/ethos-u-demos.yml` configuration fragment is used.

## Renesas Graphics and Video Codec libraries for RZ/G3E
The `features/graphics.yml` and `features/codecs.yml` configuration fragments
use the Renesas proprietary packages for the Graphics and Video Codec drivers.
These packages are not included in this repository, and must be downloaded
from the
[RZ MPU Verified Linux Package [6.1-CIP]](https://www.renesas.com/en/software-tool/rz-mpu-verified-linux-package-61-cip)
page on the Renesas website.

| Proprietary Package | Version | Layer | Package |
| --- | --- | --- | --- |
| Video Codec | 4.4.1.0 | meta-rz-features/meta-rz-codecs | [RTK0EF0207Z00001ZJ-v4.4.1.0_EN.zip](https://www.renesas.com/en/document/swo/rz-mpu-video-codec-library-evaluation-version-v4410-rzg3e-rtk0ef0207z00001zj-v4410enzip) |
| Graphics | 4.2.1.0 | meta-rz-features/meta-rz-graphics | [RTK0EF0045Z0034AZJ-4.2.1.0_EN.zip](https://www.renesas.com/en/document/swo/rz-mpu-graphics-library-v4210-rzg3e-rtk0ef0045z0034azj-4210enzip) |

Extract the `meta-rz-features` layers from each package into the root of this
repository, so that `meta-rz-features/meta-rz-graphics` and/or
`meta-rz-features/meta-rz-codecs` exist, before enabling the corresponding
feature.

## Build Instructions
This project is set up to be built with
[Kas 4.5](https://github.com/siemens/kas/tree/4.5), so it does not require
any dependencies to be downloaded and configured beforehand. The Kas tool
provides an easy mechanism to setup bitbake based projects. Documentation for
Kas is available on [readthedocs.io](https://kas.readthedocs.io/en/latest/userguide.html).

### kas-container
This repository holds a `kas-container` script, which can run the Bitbake build in
a pre-configured Docker container.

### Build using Kas Menu
Kas supports a Kconfig based menu system, which can be accessed by running
`kas-container menu`. This allows users to easily select between configuration
options using a GUI, rather than providing a colon-separated list via command-line.

How to use Kas Menu:
```bash
./kas-container menu
```

Follow the instructions in the menu to select the desired build configuration.
To enable `meta-rz-ethos-u-examples`, select `Features`->`Example Ethos-U Applications`.
To enable `meta-rz-ethos-u-demos`, select `Features`->`Demo Ethos-U Applications`.

Once the build configuration is set, select either of the following:
1. "Build": Save configuration to .config.yaml and kick off a build straight away.
2. "Save & Exit": Save configuration to .config.yaml only.

If `Save & Exit` is selected, a build can be started using the generated
`.config.yaml` by:
```bash
./kas-container build
```

Alternatively, Kas can start a shell within its build environment that is
already configured for use with bitbake:
```bash
./kas-container shell
```

### Build using Kas Configuration Fragments
Instead of using Kas Menu, configuration fragments can be daisy chained onto each
other to provide the final configuration used for the build.

The following configuration fragments are supplied by this repository, under the
`kas` directory:
| File | Purpose |
| --- | --- |
| base.yml | Base configuration used by this project |
| images/core-image-minimal.yml | Configuration for building core-image-minimal |
| images/core-image-weston.yml | Configuration for building core-image-weston |
| machines/smarc-rzg3e.yml | Configuration for building smarc-rzg3e |
| features/codecs.yml | Enables H.264 and H.265 video codec library support |
| features/ethos-u-demos.yml | Enables demo Ethos-U application support (fetches meta-rz-ethos-u-demos) |
| features/ethos-u-examples.yml | Enables example Ethos-U application support |
| features/graphics.yml | Enables graphics library support |

How to build:
```bash
# Build core-image-minimal for smarc-rzg3e
./kas-container build --update --force-checkout \
  "kas/base.yml
  :kas/images/core-image-minimal.yml
  :kas/machines/smarc-rzg3e.yml"

# Build core-image-minimal for smarc-rzg3e including example application support
./kas-container build --update --force-checkout \
  "kas/base.yml
  :kas/images/core-image-minimal.yml
  :kas/machines/smarc-rzg3e.yml
  :kas/features/ethos-u-examples.yml"

# Build core-image-weston for smarc-rzg3e including proprietary libraries
./kas-container build --update --force-checkout \
  "kas/base.yml
  :kas/images/core-image-weston.yml
  :kas/machines/smarc-rzg3e.yml
  :kas/features/codecs.yml
  :kas/features/graphics.yml"

# Build core-image-weston for smarc-rzg3e including proprietary libraries, example application support
./kas-container build --update --force-checkout \
  "kas/base.yml
  :kas/images/core-image-weston.yml
  :kas/machines/smarc-rzg3e.yml
  :kas/features/ethos-u-examples.yml
  :kas/features/codecs.yml
  :kas/features/graphics.yml"

# Build core-image-weston for smarc-rzg3e including proprietary libraries, demo application support
./kas-container build --update --force-checkout \
  "kas/base.yml
  :kas/images/core-image-weston.yml
  :kas/machines/smarc-rzg3e.yml
  :kas/features/ethos-u-demos.yml
  :kas/features/codecs.yml
  :kas/features/graphics.yml"
```

### Build the SDK
An SDK is needed to cross-compile applications that can be run on the Ethos-U
rootfs. Yocto provides a way to build it and generate the installer for
x86_64.

To build the SDK installer package using the Kas build command, add the
following parameter to `./kas-container build`:
```bash
--cmd populate_sdk
```

If a Kas shell instance is active, run:
```bash
# core-image-minimal
bitbake core-image-minimal -c populate_sdk

# core-image-weston
bitbake core-image-weston -c populate_sdk
```

Once built, the SDK installer can be found under:
```bash
# core-image-minimal
tmp/deploy/sdk/rz-vlp-glibc-x86_64-core-image-minimal-cortexa55-smarc-rzg3e-toolchain-5.0.14.sh

# core-image-weston
tmp/deploy/sdk/rz-vlp-glibc-x86_64-core-image-weston-cortexa55-smarc-rzg3e-toolchain-5.0.14.sh
```

## Further Documentation
For more information about the sample applications, compiling models and
benchmarking them, please see
[RZ/G3E NPU (Ethos-U55) Support Documentation Page](https://renesas-rz.github.io/rz-ethos-u-docs/).

## License
This repository and its contents is licensed under MIT. See
[COPYING.MIT](COPYING.MIT) for details.

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

DEPENDS += "rsync-native"

SRC_URI:append = " \
	file://0001-drivers-accel-define-kconfig-and-register-a-new-majo.patch \
	file://0002-accel-add-dedicated-minor-for-accelerator-devices.patch \
	file://0003-accel-constify-the-struct-device_type-usage.patch \
	file://0004-accel-Add-.mmap-to-DRM_ACCEL_FOPS.patch \
	file://0005-drm-initialize-accel-framework.patch \
	file://0006-drivers-Add-accel-subsystem-to-Makefile.patch \
	file://0007-mm-slab-Add-__free-support-for-kvfree.patch \
	file://0008-uaccess-add-copy_struct_to_user-helper.patch \
	file://0009-drm-sched-Allow-drivers-to-skip-the-reset-and-keep-o.patch \
	file://0010-accel-Add-Arm-Ethos-U-NPU-driver.patch \
	file://0011-drm-include-drm_print.h-where-needed.patch \
	file://0012-accel-ethosu-Fix-shift-overflow-in-cmd_to_addr.patch \
	file://0013-accel-ethosu-Fix-job-submit-error-clean-up-refcount-.patch \
	file://0014-accel-ethosu-Fix-NPU_OP_ELEMENTWISE-validation-with-.patch \
	file://0015-accel-ethosu-Handle-possible-underflow-in-IFM-size-c.patch \
	file://0016-accel-ethosu-Add-hardware-dependency-hint.patch \
	file://0017-accel-ethosu-Validate-SRAM-size-on-submit.patch \
	file://0018-accel-ethosu-Add-performance-counter-support.patch \
	file://0019-accel-ethosu-fix-IFM-region-index-out-of-bounds-in-c.patch \
	file://0020-accel-ethosu-reject-NPU_OP_RESIZE-commands-from-user.patch \
	file://0021-accel-ethosu-fix-wrong-weight-index-in-NPU_SET_SCALE.patch \
	file://0022-accel-ethosu-fix-arithmetic-issues-in-dma_length.patch \
	file://0023-accel-ethosu-reject-DMA-commands-with-uninitialized-.patch \
	file://0024-accel-ethosu-fix-OOB-write-in-ethosu_gem_cmdstream_c.patch \
	file://0025-accel-ethosu-Remove-redundant-job_lock.patch \
	file://0026-accel-ethosu-Fix-element-size-accounting-for-cmd-str.patch \
	file://0027-accel-ethosu-Handle-U85-internal-chaining-buffer.patch \
	file://0028-accel-ethosu-check-MMIO-mapping-errors-in-probe.patch \
	file://0029-accel-ethosu-Don-t-read-the-U65-rounding-mode-as-a-s.patch \
	file://0030-accel-ethosu-fix-job-completion-fence-cleanup.patch \
	file://0031-accel-ethosu-Fix-ethosu_job_open-return-value.patch \
	file://0032-accel-ethosu-Drop-IRQF_SHARED-flag.patch \
	file://0033-accel-ethosu-Ensure-cmd-stream-ends-with-a-stop-op.patch \
	file://0034-accel-ethosu-Ensure-SRAM-size-is-0-on-mapping-failur.patch \
	file://0035-accel-ethosu-Ensure-SRAM-region-size-matches-job.patch \
	file://0036-accel-ethosu-Use-product-configuration-for-NPU-ident.patch \
	file://0037-accel-ethosu-Add-Ethos-U55-NPU-support.patch \
	file://0038-accel-ethosu-Add-optional-NPU-reset-support-for-Rene.patch \
	file://0039-clk-renesas-r9a09g047-cpg-Add-NPU-clocks-and-resets.patch \
	file://0040-arm64-dts-renesas-r9a09g047-Add-Arm-Ethos-U-Direct-D.patch \
	file://0041-accel-ethosu-Increase-job-timeout.patch \
	file://ethosu_drm.cfg \
"

# Install kernel headers under a staging directory, so it can be accessible
# and used by the Ethos-U Linux Driver Stack
do_shared_workdir:append() {
	oe_runmake headers_install ARCH=${ARCH} INSTALL_HDR_PATH=${STAGING_KERNEL_BUILDDIR}/kernel-uapi-headers
}

# android_kernel_zte_msm8994

Linux 3.10.84 kernel source for the ZTE Axon A2015 (MSM8994), used by the
LineageOS 22.2 Wi-Fi-only port.

The kernel is ZTE's Marshmallow GPL drop:

- `ZTE_A2015_Marshmallow_Kernel(3.10.84).tar.gz`
- size 124,992,176 bytes
- MD5 `1952ab52f5ef3015c734cf8ef9fc48b3`

## Import

```bash
./scripts/import-zte-kernel.sh /path/to/ZTE_A2015_Marshmallow_Kernel\(3.10.84\).tar.gz
```

The script extracts the tarball and copies the tree into this repository
without touching `.git`, `README.md`, or `PORTING.md`.

## Build

Use an old GCC toolchain (AOSP GCC 4.9 or Linaro 4.9/5.x):

```bash
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-android-
make msm8994_defconfig
make -j$(nproc)
```

Products:

```
arch/arm64/boot/Image.gz-dtb
arch/arm64/boot/dts/qcom/msm8994-v2.1-cdp.dtb
arch/arm64/boot/dts/qcom/msm8994-pmi8994-pm8004-v2.1-cdp.dtb
```

## Device tree

ZTE's drop does not contain an `a2015.dts`. The A2015 board changes are in
`arch/arm64/boot/dts/qcom/msm8994-cdp.dtsi`:

- `batterydata_zte_p894a02_3000mah.dtsi`
- NT35596 1080p panel
- Synaptics DSX touch

The matching DTB is the CDP variant with `qcom,board-id = <1 0>`. Verify this
against the stock boot image before flashing; if the bootloader uses another
board ID, copy the DTS and adjust it.

See `PORTING.md` for the Android 15 backport notes.

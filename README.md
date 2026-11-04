# android_kernel_zte_msm8994

Linux 3.10.84 kernel source for the ZTE Axon A2015 (MSM8994), used by the
LineageOS 22.2 Wi-Fi-only port.

The kernel is ZTE's Marshmallow GPL drop:

- `ZTE_A2015_Marshmallow_Kernel(3.10.84).tar.gz`
- size 124,992,176 bytes
- MD5 `1952ab52f5ef3015c734cf8ef9fc48b3`

## Import

The 3.10.84 source is **committed directly** on the `lineage-22.2` branch
(about 631 MiB working tree, 296 MiB `.git`). This makes CI builds
reproducible and avoids downloading the ZTE tarball on every run.

`scripts/import-zte-kernel.sh` is kept for refreshing the tree from the
original tarball if needed:

```bash
./scripts/import-zte-kernel.sh /path/to/ZTE_A2015_Marshmallow_Kernel\(3.10.84\).tar.gz
```

The script copies the tree into this repository without touching `.git`,
`README.md`, `PORTING.md`, or `.github/`.

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
- r63419 Sharp WQHD dual-DSI panel (default `qcom,dsi-pref-prim-pan`)
- NT35596 1080p panels (alternate variant, not used on P894A02 尊享版)
- Synaptics DSX touch

The stock B16 `boot.img` contains 42 appended DTBs. The A2015 entry is the
`MSM8994v2.1 + PMI8994 + PM8004 CDP` DTB (`qcom,board-id = <1 0>`); it carries
the r63419 WQHD dual-DSI panel nodes and the Synaptics DSX node, matching the
1440x2560 panel confirmed on the device. Verify the live board ID:

```bash
adb shell cat /proc/device-tree/model
adb shell getprop | grep -E 'ro.boot|board'
```

Stock boot header/cmdline (for comparison):

```
kernel_size=19199991 ramdisk_size=3709343 page_size=4096
no_console_suspend androidboot.console=ttyHSL0 androidboot.hardware=qcom
user_debug=31 msm_rtb.filter=0x37 ehci-hcd.park=3 lpm_levels.sleep_disabled=1
boot_cpus=0-5 msm-poweroff.download_mode=0
```

See `PORTING.md` for the Android 15 backport notes.

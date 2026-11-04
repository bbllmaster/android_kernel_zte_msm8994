# Porting notes: A2015 kernel on LineageOS 22.2

## Current state

ZTE 3.10.84 BSP, not yet patched for Android 15.

## Board specifics

- SoC: MSM8994 (Snapdragon 810), 4x A57 + 4x A53, Adreno 430.
- Panel: NT35596 1080p (verify 2K variant).
- Touch: Synaptics DSX v21 (`CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_v21`).
- Audio: AK4961 + WCD9330 (A2015-specific; not required for first boot).
- Wi-Fi/BT: Qualcomm QCA/WCNSS (qcwcn), same family as OnePlus 2.
- Storage: eMMC, non-Treble layout (vendor in `/system/vendor`).

## Android 15 backports

The OnePlus 2 22.2 port uses the same 3.10 kernel generation. Use it as the
reference for the required backports:

- `https://github.com/davigamer987/kernel_oneplus_msm8994` branch `lineage-22.2`
- memfd backport (`TARGET_HAS_MEMFD_BACKPORT := true`)
- binder / seccomp / eBPF-related fixes
- legacy HAL compatibility patches

Do not copy OnePlus 2 board files. Only reuse generic MSM8994/Android changes.

## Known gaps

- No `a2015.dts`; board data lives in the CDP DTS.
- Board ID must be verified against the stock boot image.
- The 2K panel variant is unconfirmed.
- No RIL work is planned: this is a Wi-Fi-only build.

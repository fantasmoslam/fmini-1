# FMini-1 on the SMK-37

Work in progress on the `smk37` branch. No SMK build has been tested on hardware or approved for installation.

The baseline is FMini-1 v2.2.0, commit 54818a171f20df9efbc5a21db1fa21a7ec39881c. The port retains FMini-1's name, sound and authorship. The SMK hardware layer is adapted from M. Geisler's Felucca port and the owner's Paleotone SMK integration.

Build with the same Linux toolchain and SDK as upstream:

```sh
BOARD=smk37 MODEL=elite sh build.sh
```

The physical faders control amplifier Attack, Decay, Sustain and Release, left to right, with target-crossing pickup. A patch load or external edit re-arms pickup. The first physical reading never changes a loaded value. The display shows the value and pickup direction, and the amplifier page exposes independent Release.

| Control | Function |
| --- | --- |
| K1 | Master output level |
| K2 / K3 | Fixed filter cutoff / resonance on ordinary sound pages |
| K4 | Preset selection |
| K5..K8 | The current page's four controls |
| Hold KNOB BANK + K2 / K3 | Select / edit an extra control; modifier is shown on screen |
| Hold KNOB BANK + K8 | Patch level |
| Left / right | Previous / next sound page |
| FADER BANK | Open amplifier envelope; faders keep their ADSR assignments |
| Pitch / mod wheels | Engine pitch-bend / modulation path |
| GLO > 2ND CORE | Enable or disable the inherited worker after restart; hardware benefit unverified |

The existing pads play notes using the reference SMK mapping. A dedicated performance-patch bank is deferred. Pedal support and Bluetooth are not implemented. Legato and selectable note priority remain future work. The worker still splits whole parts; one active part may get no acceleration. No measured speedup or increased voice budget is claimed.

The common store stays in the SMK VM region. The thirteen A/B objects for the 200-slot FMini patch bank occupy 0xC2000..0xDBFFF, replacing stock user data. The stock mapping marker is invalidated before that area is used so stock firmware can rebuild its own mapping. The calibration/pairing area, bootloader, stock resource area and key area remain protected. Firmware changes do not preserve stock sounds/settings; export those before switching.

The installer must match the package model to the running device or update loader. The FM-1 TRS MIDI input driver is disabled because that pin belongs to the SMK key matrix. USB MIDI remains the baseline input path.

No installation should be attempted until full backup, package/model, storage, recovery and physical-input checks have passed. A successful compile alone is insufficient.

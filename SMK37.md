# FMini-1 on the SMK-37

Work in progress on the `smk37` branch. The Elite build from commit 2939b49 has been installed and played on one SMK-37 Elite (see "Hardware results" below). The Pro and MKE-P37 builds have not been tested on hardware.

The baseline is FMini-1 v2.2.0, commit 54818a171f20df9efbc5a21db1fa21a7ec39881c. The port retains FMini-1's name, sound and authorship. The SMK hardware layer is adapted from M. Geisler's Felucca port and the owner's Paleotone SMK integration.

Build with the same Linux toolchain and SDK as upstream:

```sh
BOARD=smk37 MODEL=elite sh build.sh
```

The SMK amplifier envelope uses independent Attack, Decay, Sustain and Release. The original FM-1 build keeps its coupled Decay/Release behaviour. Factory patches on the SMK now use their stored Release values; the filter contour still shares its Decay time with release.

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
| BT (printed Bluetooth button) | SAVE: press to open, press again for OVERWRITE?, K3 to YES, press to write |
| GLO > 2ND CORE | Enable or disable the inherited worker after restart; hardware benefit unverified |

The existing pads play notes using the reference SMK mapping. A dedicated performance-patch bank is deferred. Pedal support and Bluetooth are not implemented. Legato and selectable note priority remain future work. The worker still splits whole parts; one active part may get no acceleration. No measured speedup or increased voice budget is claimed.

The common store stays in the SMK VM region. The thirteen A/B objects for the 200-slot FMini patch bank occupy 0xC2000..0xDBFFF, replacing stock user data. The stock mapping marker is invalidated before that area is used so stock firmware can rebuild its own mapping. The calibration/pairing area, bootloader, stock resource area and key area remain protected. Firmware changes do not preserve stock sounds/settings; export those before switching.

The installer must match the package model to the running device or update loader. The FM-1 TRS MIDI input driver is disabled because that pin belongs to the SMK key matrix. USB MIDI remains the baseline input path.

No installation should be attempted until full backup, package/model, storage, recovery and physical-input checks have passed. A successful compile alone is insufficient.

## Performance work reused and deferred

The hardware baseline is `genkigenki/Felucca`'s SMK port at `b67312cd25bd01b097ed69388b83e86bd8449365`. Paleotone's SMK branch (`4c41f79c2a3c04d75373f20e55f643614b937891`) and unified integration (`3b93a5d449f2fe7c3790f57f7a782e9ecca2294e`) supply integration references. Existing copyright and license notices remain in the adapted files.

FMini already has the same low-level second-core worker header found in the audited Paleotone implementation. Importing another copy would add no capability. Its whole-part scheduling remains guarded and opt-in. Voice splitting or moving effects between cores requires a separate timing study; neither is included in this port.

Keep the existing 128-frame audio buffer. A buffer-size experiment would change latency and is not needed for the board port. Browser mirror/display throttling work is also outside this port: it does not establish a measured audio improvement for this instrument.

## Hardware qualification sequence

1. Export stock sounds/settings and make a complete, verified device backup. Establish a working restore route and confirm the physical model before selecting a package.
2. With the exact tested Elite package, check cold boot, screen, all labelled buttons and encoder direction. Verify all 37 keys, velocity, overlapping key/pad ownership, note release, octave limits, both wheels and their return to neutral.
3. Test all four faders from both sides of a loaded value, including endpoints, rapid crossing, preset changes, MIDI edits, and entering/leaving GLO or SAVE. Confirm there are no jumps before pickup and that Release is independent of Decay. Confirm K2/K3 retain menu editing while a dialog is open and the KNOB BANK modifier exposes the extra page controls.
4. With the second core off, record audio and diagnostics during ordinary playing, then polyphony/FX/control/MIDI stress. Capture underruns, worker fallback/wait counters, stack headroom and UI response. Repeat with the core enabled after restart under the same workload; make performance claims only from these measurements.
5. Save, reload and restart with edits in the first and last preset-bank objects. Verify protected calibration/pairing bytes against the backup. Interrupted writes are exercised in the NOR simulator; deliberate power interruption on the device is not part of the initial playing test.

The host checks use the actual FMini UI and sequencer with a register-free SMK HAL. They cannot prove the matrix wiring, ADC behaviour, codec output, flash-chip timing or real-time audio margin. Those remain hardware gates.

## Hardware results

One SMK-37 Elite, November 5, 2026. Tested by the owner, not by an automated rig.

| Item | Result |
| --- | --- |
| Package | `fmini-1-smk37-elite.fwsc`, 651,198 bytes, SHA-256 `8919bf76b04291b7b54b4d6f18ff6cc84c15cf324759ac2ef2cd7db15f40835d`, from CI run 41959745683 at commit `2939b4947452fce4f5983edec69ee65a892c4033` |
| Identity before install | `SMK-37 Elite_900` (running), a Felucca-family firmware with Paleotone data tag `SMK37DATAPALEOTN` at 0xAF000 |
| Backup | Full 1 MB read twice over the mask-ROM bootloader with jl-uboot-tool; the two reads are identical; `trimdat` at 0xC1224; 0xFF000..0xFFFFF blank on this unit |
| Flash | WL82 (AC791N), chip key 0x980F, flash ID `0x856014` (the kit's reference unit reports `0xeb6014`) |
| Install | `tools/fm1_install.py --port Felucca` over USB MIDI; installed and booted |
| Controls | Faders 1..4 ADSR with pickup, short Decay / long Release and the reverse, K2..K8, all 37 keys and pads, both wheels, screen pages: working |
| Save | Patch saved with BT, reloaded after restart |
| Second core | No audible difference with the worker on or off in normal playing; no counters were measured |

Not done on hardware: the measured core-off/on stress comparison (step 4), saves in the first and last bank objects with a protected-byte readback (step 5), and a demonstrated restore from the backup.

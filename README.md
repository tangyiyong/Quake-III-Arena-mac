# QuakeMac

A community macOS Apple Silicon port based on a pinned ioquake3 revision,
with native app startup, bundled SDL2, sandbox-compatible configuration paths,
Xbox / Nintendo Switch Pro controller defaults, a native settings window,
and gyro aiming when SDL exposes a controller gyroscope.

This repository retains id Software's original GPL source tree. The modern
runtime is downloaded from ioquake3 and patched by the scripts in
`tools/apple-port`; it is not a claim that the unmodified 2005 sources build on
current macOS. This project is not an official id Software release.

## Requirements

- Apple Silicon Mac; Xcode, CMake 3.25+, Ninja, Python 3.12+
- Your own Quake III Arena baseq3 resources (`pak0.pk3` through `pak8.pk3`)
- `iconutil` and `sips` from macOS if regenerating icons

The release build targets macOS 12+. Older supported systems and physical
controllers still require hardware acceptance testing.

## Build an app archive

```sh
python3 tools/apple-port/macos_release.py \
  --version 0.1.0 --build 5 \
  --resources '/path/to/your/baseq3'
```

The script downloads a SHA256-verified source archive, applies the tracked
patches, builds an Xcode archive, and copies the nine resource packages
into the app. It also includes the corresponding source archives and licenses.
All generated files stay under ignored `.apple-port/`.

The resulting app reads game data from its own bundle, while settings go to
Application Support inside the application sandbox when sandbox signing is
enabled. The archive is locally ad-hoc signed to retain Sandbox entitlements;
it is not automatically signed for distribution or uploaded.
Do not assume a local build number is available in App Store Connect.

## Controllers

Default enabled: left stick moves, right stick looks, right trigger fires,
left trigger zooms. Xbox A / Switch B jumps; Start / Plus opens the menu.
The menu supports left stick / D-pad navigation and south/east confirm/back.
Open native controller/display settings from the macOS app menu, Command+comma,
or F10 (Fn+F10 on some keyboards). Gyro aiming is optional and requires an actual
sensor; standard Xbox controllers do not have one. Physical sensor validation
is still pending. Window mode automatically fits the usable desktop.

See [controller controls and verification](docs/controllers.md).

```sh
python3 tools/apple-port/test_gamepad.py
```

## Resources and licensing

Game PK3 files, signing keys, profiles, archives and app bundles are not included
in this Git branch. The game-resource license is separate from the engine GPL;
providing build support does not grant redistribution rights to those resources.

See [COPYING.txt](COPYING.txt), [original release notes](README.txt),
[Apple port plan](docs/apple-port-plan.md), and
[local TestFlight preparation status](docs/testflight-status.md).

The generated community icon is in `assets/branding`, including its prompt and
export instructions. ioquake3's source and all bundled third-party notices remain
in the pinned source archive used by the build.

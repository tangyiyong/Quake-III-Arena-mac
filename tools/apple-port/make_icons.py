#!/usr/bin/env python3
"""Produce Apple app icon sizes from the generated opaque artwork using sips."""
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
BRAND = ROOT / 'assets' / 'branding'
SOURCE = BRAND / 'arena-logo-source.png'
ICONSET = BRAND / 'Arena.iconset'
CATALOG = BRAND / 'Assets.xcassets'


def resize(size, output):
    output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['sips', '-z', str(size), str(size), str(SOURCE),
                    '--out', str(output)], check=True, capture_output=True)


def main():
    ICONSET.mkdir(parents=True, exist_ok=True)
    mac = CATALOG / 'MacAppIcon.appiconset'
    ios = CATALOG / 'AppIcon.appiconset'
    mac.mkdir(parents=True, exist_ok=True)
    ios.mkdir(parents=True, exist_ok=True)
    entries = []
    for size in [16, 32, 128, 256, 512]:
        for scale in [1, 2]:
            suffix = '@2x' if scale == 2 else ''
            name = f'icon_{size}x{size}{suffix}.png'
            resize(size * scale, ICONSET / name)
            (mac / name).write_bytes((ICONSET / name).read_bytes())
            entries.append({'filename': name, 'idiom': 'mac',
                            'size': f'{size}x{size}', 'scale': f'{scale}x'})
    info = {'author': 'xcode', 'version': 1}
    (CATALOG / 'Contents.json').write_text(json.dumps({'info': info}, indent=2) + '\n')
    (mac / 'Contents.json').write_text(json.dumps({'images': entries, 'info': info}, indent=2) + '\n')
    resize(1024, ios / 'AppIcon-1024.png')
    resize(1024, BRAND / 'arena-logo-1024.png')
    (ios / 'Contents.json').write_text(json.dumps({'images': [
        {'filename': 'AppIcon-1024.png', 'idiom': 'universal',
         'platform': 'ios', 'size': '1024x1024'}], 'info': info}, indent=2) + '\n')
    subprocess.run(['iconutil', '-c', 'icns', str(ICONSET),
                    '-o', str(BRAND / 'Arena.icns')], check=True)
    print('Created macOS ICNS and macOS/iOS asset catalogs:', BRAND)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Inspect the actual exported macOS PKG before submitting it to Apple."""
import argparse
from pathlib import Path
import plistlib
import subprocess
import tempfile

import port


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('package', type=Path)
    parser.add_argument('--build', required=True)
    args = parser.parse_args()
    temporary_root = port.WORK / 'verification'
    temporary_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=temporary_root) as temporary:
        output = Path(temporary) / 'expanded'
        subprocess.run(['pkgutil', '--expand-full', str(args.package.resolve()), str(output)], check=True)
        apps = list(output.rglob('QuakeMac.app'))
        if len(apps) != 1:
            raise RuntimeError('Expected exactly one app in the installer')
        app = apps[0]
        attributes = subprocess.check_output(['xattr', '-r', str(app)], text=True)
        if 'com.apple.quarantine' in attributes:
            raise RuntimeError('Exported app contains quarantined files')
        entitlements = plistlib.loads(subprocess.check_output(
            ['codesign', '-d', '--entitlements', ':-', str(app)], stderr=subprocess.DEVNULL))
        if entitlements.get('com.apple.security.app-sandbox') is not True:
            raise RuntimeError('Exported app is missing the App Sandbox entitlement')
        info = plistlib.loads((app / 'Contents/Info.plist').read_bytes())
        if any(info.get(key) != 'QuakeMac' for key in ['CFBundleName', 'CFBundleDisplayName', 'CFBundleExecutable']):
            raise RuntimeError('Exported app name must be QuakeMac')
        if info.get('CFBundleVersion') != args.build:
            raise RuntimeError('Exported build number differs from the expected value')
        port.run(['codesign', '--verify', '--deep', '--strict', app])
        port.run(['pkgutil', '--check-signature', args.package.resolve()],
                 stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print('PASS: exported app Sandbox, quarantine attributes, signatures and build number')


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error))

#!/usr/bin/env python3
"""Build an unsigned, self-contained macOS archive; no upload is implied."""
import argparse
import json
from pathlib import Path
import plistlib
import shutil
import subprocess
import tarfile
import zipfile

import port

ROOT = port.ROOT
RELEASE = port.WORK / 'release'


def dependencies(path):
    output = subprocess.check_output(['otool', '-L', str(path)], text=True)
    return [line.strip().split(' (compatibility')[0] for line in output.splitlines()
            if line.startswith('\t')]


def verify(app):
    info = plistlib.loads((app / 'Contents/Info.plist').read_bytes())
    if info['CFBundleIdentifier'] != 'com.bdsoft.quake.mac':
        raise RuntimeError('Unexpected bundle identifier')
    data = app / 'Contents/Resources/GameData/baseq3'
    expected = {f'pak{i}.pk3' for i in range(9)}
    if {p.name for p in data.iterdir()} != expected:
        raise RuntimeError('Bundled baseq3 must contain exactly pak0 through pak8')
    if any(p.is_symlink() for p in data.iterdir()):
        raise RuntimeError('Bundled packages must be actual files, not links')
    for binary in (app / 'Contents/MacOS').iterdir():
        if not binary.is_file():
            continue
        for dep in dependencies(binary):
            if not dep.startswith(('/System/', '/usr/lib/', '@executable_path/', '@loader_path/', '@rpath/')):
                raise RuntimeError(f'Non-portable dependency: {binary.name}: {dep}')
            if dep.startswith('@executable_path/') and not (binary.parent / dep.split('/', 1)[1]).is_file():
                raise RuntimeError(f'Missing embedded dependency: {dep}')
    print('Verified bundle ID, embedded dependencies, and nine bundled game packages')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', default='0.1.0')
    parser.add_argument('--build', default='1')
    parser.add_argument('--resources', type=Path,
                        default=ROOT.parent / 'Quake 3 Arena/baseq3')
    args = parser.parse_args()
    packages = [args.resources.resolve() / f'pak{i}.pk3' for i in range(9)]
    for package in packages:
        with zipfile.ZipFile(package) as z:
            if z.testzip() is not None:
                raise RuntimeError(f'Corrupt game package: {package.name}')
    import re
    if not re.fullmatch(r'\d+\.\d+\.\d+', args.version) or not re.fullmatch(r'[1-9]\d{0,3}', args.build):
        parser.error('Use a numeric x.y.z version and a build number from 1 through 9999')
    port.bootstrap()
    build = RELEASE / 'xcode'
    archive = RELEASE / f'Arena-{args.version}-{args.build}.xcarchive'
    if archive.exists():
        raise RuntimeError(f'Refusing to overwrite archive: {archive}')
    port.run(['cmake', '-S', port.SOURCE, '-B', build, '-G', 'Xcode',
        '-DCMAKE_OSX_ARCHITECTURES=arm64', '-DCMAKE_XCODE_GENERATE_SCHEME=ON',
        '-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0',
        '-DCMAKE_XCODE_ATTRIBUTE_SKIP_INSTALL=YES',
        '-DCMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER=com.bdsoft.quake.mac',
        '-DCMAKE_XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME=YES',
        f'-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS={ROOT / "tools/apple-port/macos/Arena.entitlements"}',
        '-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS=NO',
        '-DUSE_INTERNAL_SDL=ON', '-DBUILD_SERVER=OFF', '-DBUILD_GAME_LIBRARIES=OFF',
        '-DBUILD_GAME_QVMS=OFF', '-DBUILD_RENDERER_GL2=OFF', '-DUSE_OPENAL=OFF',
        '-DUSE_RENDERER_DLOPEN=OFF',
        '-DUSE_HTTP=OFF', f'-DPRODUCT_VERSION={args.version}',
        f'-DARENA_BUILD_NUMBER={args.build}', '-DMACOS_BUNDLE_ID=com.bdsoft.quake.mac',
        f'-DMACOS_ICON_PATH={ROOT / "assets/branding/Arena.icns"}',
        f'-DARENA_LAUNCHER_SOURCE={ROOT / "tools/apple-port/macos/ArenaLauncher.m"}'])
    logs = RELEASE / 'logs'
    logs.mkdir(parents=True, exist_ok=True)
    product_link = build / 'Release/ioquake3.app'
    if product_link.is_symlink() and not product_link.exists():
        product_link.unlink()  # Xcode can leave this link after a failed archive.
    with (logs / f'archive-{args.build}.log').open('w') as log:
        port.run(['xcodebuild', '-project', build / 'ioq3.xcodeproj', '-scheme', 'ioquake3',
                  '-configuration', 'Release', '-destination', 'generic/platform=macOS',
                  '-archivePath', archive, 'CODE_SIGNING_ALLOWED=NO', 'archive'],
                 stdout=log, stderr=subprocess.STDOUT)
    app = archive / 'Products/Applications/ioquake3.app'
    game = app / 'Contents/Resources/GameData/baseq3'
    game.mkdir(parents=True, exist_ok=True)
    for package in packages:
        shutil.copyfile(package, game / package.name)
    licenses = app / 'Contents/Resources/Licenses'
    licenses.mkdir(parents=True, exist_ok=True)
    shutil.copy2(port.SOURCE / 'COPYING.txt', licenses / 'ioquake3-GPL.txt')
    shutil.copy2(port.SOURCE / 'code/thirdparty/SDL2-2.32.8/include/SDL_copying.h', licenses / 'SDL2-license.h')
    source = app / 'Contents/Resources/Source'
    source.mkdir(parents=True, exist_ok=True)
    shutil.copy2(port.WORK / 'downloads' / (port.LOCK['revision'] + '.tar.gz'), source / 'ioq3-source.tar.gz')
    with tarfile.open(source / 'apple-port-source.tar.gz', 'w:gz') as package:
        for relative in ['tools/apple-port', 'assets/branding', 'docs', '.gitignore', 'README.md']:
            package.add(ROOT / relative, arcname=relative,
                        filter=lambda entry: None if '__pycache__' in entry.name else entry)
    verify(app)
    (RELEASE / 'latest-local.json').write_text(json.dumps({
        'bundle_id': 'com.bdsoft.quake.mac', 'version': args.version,
        'build': args.build, 'archive': str(archive), 'app': str(app),
        'signed_for_distribution': False, 'uploaded_to_testflight': False}, indent=2) + '\n')
    print('Unsigned archive ready:', archive)
    print('App Store Connect build number has NOT been checked; do not upload without checking it.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error))

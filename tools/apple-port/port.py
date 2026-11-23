#!/usr/bin/env python3
"""Local macOS development harness. Does not redistribute game data."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import tarfile
import tempfile
import uuid
import zipfile

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / '.apple-port'
LOCK = json.loads((Path(__file__).parent / 'engine.lock.json').read_text())
SOURCE = WORK / 'deps' / ('ioq3-' + LOCK['revision'])
BUILD = WORK / 'build' / 'macos-arm64'
BIN = BUILD / 'RelWithDebInfo'
DATA = WORK / 'data'


def run(args, **kwargs):
    print('+', ' '.join(map(str, args)), flush=True)
    return subprocess.run(list(map(str, args)), check=True, **kwargs)


def bootstrap():
    archive = WORK / 'downloads' / (LOCK['revision'] + '.tar.gz')
    archive.parent.mkdir(parents=True, exist_ok=True)
    if not archive.exists():
        partial = archive.with_suffix('.partial')
        run(['curl', '--fail', '--location', '--retry', '2',
             '--connect-timeout', '15', '--max-time', '180',
             LOCK['repository'].replace('github.com', 'codeload.github.com')
             + '/tar.gz/' + LOCK['revision'], '-o', partial])
        partial.replace(archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != LOCK['archive_sha256']:
        raise RuntimeError(f'Archive checksum mismatch: {archive}')
    patches = sorted((Path(__file__).parent / 'patches').glob('*.patch'))
    fingerprint = hashlib.sha256(b''.join(p.name.encode() + p.read_bytes() for p in patches)).hexdigest()
    stamp = SOURCE / '.arena-patch-set'
    if not stamp.exists() or stamp.read_text().strip() != fingerprint:
        SOURCE.parent.mkdir(parents=True, exist_ok=True)
        # Apply the entire patch sequence to pristine sources. Later patches may
        # change the context of earlier ones, so reverse checks are insufficient.
        with tempfile.TemporaryDirectory(prefix='arena-patches-', dir=SOURCE.parent) as temporary:
            with tarfile.open(archive) as package:
                package.extractall(temporary, filter='data')
            staged = Path(temporary) / SOURCE.name
            for patch in patches:
                run(['patch', '-p1', '--forward', '--batch', '-i', patch], cwd=staged)
            (staged / '.arena-patch-set').write_text(fingerprint + '\n')
            if SOURCE.exists():
                SOURCE.rename(SOURCE.with_name(SOURCE.name + '.previous-' + uuid.uuid4().hex[:8]))
            staged.rename(SOURCE)
    print('Engine revision:', LOCK['revision'])


def build():
    if platform.system() != 'Darwin' or platform.machine() != 'arm64':
        raise RuntimeError('This initial build target requires an ARM64 Mac')
    bootstrap()
    icon = ROOT / 'assets' / 'branding' / 'Arena.icns'
    branding = [f'-DMACOS_ICON_PATH={icon}'] if icon.is_file() else []
    run(['cmake', '-S', SOURCE, '-B', BUILD, '-G', 'Ninja',
         '-DCMAKE_BUILD_TYPE=RelWithDebInfo', '-DCMAKE_OSX_ARCHITECTURES=arm64',
         '-DUSE_INTERNAL_SDL=OFF', '-DBUILD_RENDERER_GL2=OFF',
         '-DBUILD_GAME_LIBRARIES=OFF', '-DUSE_OPENAL=OFF', *branding])
    run(['cmake', '--build', BUILD, '--parallel', str(min(os.cpu_count() or 4, 8))])


def prepare(source):
    source = source.resolve()
    packages = [source / f'pak{i}.pk3' for i in range(9)]
    # Validate everything before writing any runtime data.
    for package in packages:
        with zipfile.ZipFile(package) as z:
            bad = z.testzip()
            if bad:
                raise RuntimeError(f'{package}: corrupt entry {bad}')
        print(package.name, 'CRC OK', flush=True)
    target = DATA / 'baseq3'
    target.mkdir(parents=True, exist_ok=True)
    expected = {p.name for p in packages}
    unexpected = [p.name for p in target.iterdir() if p.name not in expected]
    if unexpected:
        raise RuntimeError(f'Unexpected files in isolated baseq3: {unexpected}')
    manifest = []
    for package in packages:
        dest = target / package.name
        if dest.exists() and not dest.is_symlink():
            raise RuntimeError(f'Refusing to overwrite {dest}')
        if dest.is_symlink():
            dest.unlink()
        dest.symlink_to(package)
        manifest.append({'name': package.name, 'source': str(package),
                         'bytes': package.stat().st_size})
    (WORK / 'resources.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print('Resources linked without copying or modifying the source:', DATA)


def engine_args(profile):
    if not all((DATA / 'baseq3' / f'pak{i}.pk3').is_file() for i in range(9)):
        raise RuntimeError('Run prepare before launching')
    home = WORK / 'profiles' / profile
    home.mkdir(parents=True, exist_ok=True)
    return ['+set', 'fs_basepath', str(DATA), '+set', 'fs_homepath', str(home),
            '+set', 'vm_game', '1', '+set', 'vm_cgame', '1', '+set', 'vm_ui', '1',
            '+set', 'cl_allowDownload', '0']


def smoke():
    logs = WORK / 'logs'
    logs.mkdir(parents=True, exist_ok=True)
    path = logs / 'server-smoke.log'
    args = [BIN / 'ioq3ded', *engine_args('smoke'),
            '+set', 'net_ip', '127.0.0.1', '+set', 'net_enabled', '1',
            '+set', 'net_port', '27961', '+set', 'sv_public', '0',
            '+set', 'sv_pure', '0', '+set', 'bot_enable', '1',
            '+set', 'g_gametype', '0', '+map', 'q3dm1', '+addbot', 'sarge', '2',
            '+wait', '120', '+status', '+quit']
    with path.open('w') as log:
        try:
            run(args, cwd=BIN, stdout=log, stderr=subprocess.STDOUT, timeout=60)
        except subprocess.TimeoutExpired as error:
            raise RuntimeError(f'Server timed out; inspect {path}') from error
    output = path.read_text(errors='replace')
    # Require actual game and bot initialization, not just process exit.
    required = ['InitGame:', 'map: q3dm1', 'ClientBegin: 0', 'Sarge', 'ShutdownGame:']
    if not all(token in output for token in required):
        raise RuntimeError(f'Server smoke check failed; inspect {path}')
    print('Server map/bot smoke passed:', path)


def launch():
    args = [BIN / 'ioquake3.app' / 'Contents' / 'MacOS' / 'ioquake3',
            *engine_args('client'),
            '+set', 'cl_renderer', 'opengl1', '+set', 'r_fullscreen', '0',
            '+set', 'r_mode', '-1', '+set', 'r_customwidth', '1280',
            '+set', 'r_customheight', '720', '+set', 'com_introplayed', '1',
            '+set', 'sv_public', '0', '+set', 'net_ip', '127.0.0.1',
            '+map', 'q3dm1', '+addbot', 'sarge', '2']
    run(args, cwd=BIN)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['bootstrap', 'build', 'prepare', 'smoke', 'launch'])
    parser.add_argument('--resources', type=Path,
                        default=ROOT.parent / 'Quake 3 Arena' / 'baseq3')
    options = parser.parse_args()
    if options.command == 'prepare':
        prepare(options.resources)
    else:
        globals()[options.command]()


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError,
            zipfile.BadZipFile, tarfile.TarError) as error:
        raise SystemExit(str(error))

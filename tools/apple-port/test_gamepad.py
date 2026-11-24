#!/usr/bin/env python3
"""Build and run SDL virtual-device regression checks on macOS."""
import shutil
import port

port.bootstrap()
output = port.WORK / 'tests'
output.mkdir(parents=True, exist_ok=True)
sdl = port.SOURCE / 'code/thirdparty'
shutil.copy2(sdl / 'libs/macos/libSDL2-2.0.0.dylib', output)
port.run(['xcrun', 'clang', '-O2', '-Wno-unused-function',
          '-I' + str(sdl / 'SDL2-2.32.8/include'),
          '-I' + str(port.SOURCE / 'code/client'),
          port.ROOT / 'tools/apple-port/tests/gamepad_test.c',
          output / 'libSDL2-2.0.0.dylib', '-o', output / 'gamepad-test'])
port.run([output / 'gamepad-test'])
port.run(['xcrun', 'clang', '-O2', port.ROOT / 'tools/apple-port/tests/input_math_test.c',
          '-o', output / 'input-math-test'])
port.run([output / 'input-math-test'])

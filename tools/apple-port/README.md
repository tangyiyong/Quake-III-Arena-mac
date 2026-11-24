# Apple 移植开发入口

当前使用锁定的 ioquake3 引擎及本地补丁，原始源码保持不变。
计划及状态见 `docs/apple-port-plan.md`。目前仅实现 macOS ARM64 开发基线。

## 环境

- Apple Silicon Mac，Xcode Command Line Tools / Xcode
- Python 3.12+、CMake 3.25+、Ninja、SDL2 开发库
- 本机 SDL2 由 Homebrew 提供，实际为 sdl2-compat；链接依赖本机 Homebrew，
  当前产物不是可直接分发到其他电脑的独立安装包。

## 命令

在仓库根目录执行：

```sh
python3 tools/apple-port/port.py build
python3 tools/apple-port/port.py prepare
python3 tools/apple-port/port.py smoke
python3 tools/apple-port/port.py launch
```

`build` 下载 SHA256 校验的固定版本源码，应用补丁并构建 ARM64 客户端、
服务器和 QVM。后续修改引擎时应把改动保存成 `patches/` 中的补丁，不能只修改
被忽略的依赖目录。重新解压依赖后可重现这些补丁。

`prepare` 默认从同级 `Quake 3 Arena/baseq3` 读取 pak0 至 pak8，CRC 检查
后创建符号链接，不复制或解压资源。也可传入：

```sh
python3 tools/apple-port/port.py prepare --resources '/absolute/path/baseq3'
```

符号链接依赖原资源目录，移动原目录后需要重新 prepare。
Windows EXE、旧配置、额外资源包均不加入运行目录。

`smoke` 使用本机回环地址、独立配置和专用端口 27961，加载 q3dm1，添加
Sarge，运行 120 帧后检查状态并退出。日志位于 `.apple-port/logs/server-smoke.log`。
它验证游戏逻辑，不代表图形、音频和输入通过验收。

`launch` 打开 1280×720 窗口并加载 q3dm1 / Sarge。三个 QVM 模块均指定
解释执行。可双击 `launch-macos.command` 执行相同入口。
Esc 打开游戏菜单，退出可使用菜单或 Quake 控制台 `quit`。
客户端不应直接双击构建出的 app：该入口尚未集成资源定位。

## 本地文件

所有依赖、资源链接、编译产物、运行配置与日志都位于忽略的 `.apple-port/`。
源码包保留上游 COPYING.txt 及第三方许可；游戏资源不属于引擎源码授权范围。
未制作分发包，也未上传资源或二进制。

## macOS 独立应用 / TestFlight 准备

新增 `macos_release.py` 用于独立应用归档：

```sh
python3 tools/apple-port/macos_release.py --version 0.1.0 --build 1
```

它使用 Xcode 生成器和随固定源码附带的 SDL2，直接将渲染器编入引擎，
内置 pak0 至 pak8（不使用符号链接），并附带代码许可及移植源码。
应用标识固定为用户指定的 `com.bdsoft.quake.mac`，最低部署目标为 macOS 12。
图标使用 `assets/branding/Arena.icns`，启动时自动定位包内资源。
应用配置写入 Application Support，沙盒启用后自然落入应用容器。

此入口生成的 `.xcarchive` 使用本地 ad-hoc 签名保留 Sandbox 权限，
尚未使用苹果分发证书签名，也不会自动上传。
已有同名归档时拒绝覆盖。完整验证结果和待补充账号信息见
`docs/testflight-status.md`。`local-test/Arena.app` 是本机沙盒验证副本，
不能替代已签名、公证或 TestFlight 分发版本。

App Store 导出后应检查最终 PKG，而不只检查原始归档：

```sh
python3 tools/apple-port/verify_export.py '/path/to/ioquake3.pkg' --build 3
```

检查覆盖导出包内真实程序的 Sandbox 权限、资源下载隔离属性、签名和构建号。
此检查不替代 Apple 服务端验证以及上传后的构建处理状态检查。

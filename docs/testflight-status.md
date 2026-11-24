# macOS TestFlight 发布准备（2026-11-24）

## 已确定

- 平台：macOS，首版 ARM64；最低部署目标 macOS 12，当前仅在本机系统实测。
- Bundle ID：`com.bdsoft.quake.mac`。工程及归档已配置，Apple 后台尚未注册/核验。
- 用户明确要求游戏资源内置。应用包含 pak0.pk3 至 pak8.pk3，不再显示资源导入流程。
- 本地候选：`0.1.0 (2)`。未查询 App Store Connect，尚不能确认这是可用的下一个构建号。

## 实际产物

- `.apple-port/release/Arena-0.1.0-2.xcarchive`：Xcode 成功生成的未分发签名归档。
- `.apple-port/release/local-test/Arena-0.1.0-2.app`：本机 ad-hoc 沙盒签名验证副本，约 508 MiB。
- `.apple-port/release/latest-local.json`：本地版本与产物索引，明确标记未上传。
- Logo：`assets/branding/arena-logo-1024.png`；应用已集成 ICNS。

## 已验证

- Xcode archive 日志包含 `ARCHIVE SUCCEEDED`。
- 使用上游附带 SDL2 库，Mach-O 依赖检查无 Homebrew 等外部库路径。
- 渲染器直接编入引擎，避免动态加载渲染器的 Xcode 归档路径冲突。
- 九个基础包先通过 ZIP CRC 检查再复制入 app，均为实际文件而非符号链接。
- 从 `/tmp` 启动归档中的程序，成功运行 q3dm1 与机器人并正常退出。
- 本机 ad-hoc 沙盒签名副本通过 `codesign --verify --deep --strict`。
- 沙盒运行日志确认 Apple M1 OpenGL、CoreAudio 初始化，以及 Sarge 对战事件。
- 实际配置文件写入 `~/Library/Containers/com.bdsoft.quake.mac/Data/Library/Application Support/com.bdsoft.quake.mac/Config/baseq3/`。
- 通过原生 UI 工具观察到沙盒应用窗口；截图工具超时，未保存新的画面截图。
- 已附带引擎许可、SDL 许可、固定上游源码包及本仓库移植源码/补丁。

这不代表已经通过苹果分发签名、审核或另一台 Mac 的安装验证。

## 剩余工作

1. 用户提供仓库外 App Store Connect env 配置路径，含 ASC_KEY_ID、ASC_ISSUER_ID、ASC_KEY_PATH。
2. 查询/注册 `com.bdsoft.quake.mac`，核验或创建 App Store Connect 应用记录。
3. 查询已有构建号，确定发布版本，补全 macOS 分发证书和描述文件，执行签名与上传。
4. 等待 Apple 构建处理，设置外部测试组与测试说明，通过必要的 Beta App Review 后取得邀请链接。

当前没有完成上传，也没有 TestFlight 邀请链接。用户已授权本次上传；缺少的是账号配置。

## 构建入口

```sh
python3 tools/apple-port/macos_release.py --version 0.1.0 --build 2
```

同名归档存在时拒绝覆盖。失败重试如需保留旧产物，可先将其改名再执行。
默认资源目录是仓库同级 `Quake 3 Arena/baseq3`；可用 `--resources` 指定其他目录。

## 外部测试说明草案

应用已内置基础资源，无需额外下载。请测试主菜单、单人机器人对战、地图切换、
键盘鼠标操作、音量与声音、退出/再次启动，以及多人联网。首版本仅支持 Apple Silicon Mac。

参考：https://developer.apple.com/help/app-store-connect/test-a-beta-version/invite-external-testers

## 手柄版更新

已完成默认 Xbox / Switch Pro 位置布局、双摇杆、扳机、菜单导航、热插拔/失焦释放。
虚拟 SDL 设备回归检查通过；实体手柄 USB/蓝牙仍待验收，详细说明见 `controllers.md`。
内置资源与 Windows 安装体积差异见 `resource-size.md`。
API 配置获取方式见 `appstore-connect.md`。

## API 与 GitHub 同步更新

- 已验证本机 App Store Connect API 认证。
- 已在 Apple Developer 后台注册 `com.bdsoft.quake.mac`。
- 已创建 ACTIVE 的 Mac App Store 描述文件，导出分发签名安装包成功。
- 本地 `.apple-port/release/export-2/ioquake3.pkg` 约 478 MiB，通过安装包签名检查。
- App Store Connect 尚无对应应用记录；需在网页新建 macOS App 后继续上传。
- GitHub 主仓库：https://github.com/tangyiyong/Quake-III-Arena-mac 。
- 已同步 main 及 codex/macos-gamepad-release；原 id Software 远端保留为 upstream。
- API 私钥、描述文件、账号标识和上传日志均在忽略目录/仓库外，不纳入版本管理。

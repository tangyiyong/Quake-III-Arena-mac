# macOS TestFlight 状态（2026-11-24）

## 当前结果

- 平台：macOS ARM64，部署目标 macOS 12+，本机 Apple Silicon 已实测。
- Bundle ID：`com.bdsoft.quake.mac`。
- 上传版本：`0.1.0 (3)`；Apple 交付和导入均为 VALID，TestFlight API 也确认 VALID。
- 已保存 What to Test，已关联 Friends 外部测试组。
- 外测审核已提交，状态为 WAITING_FOR_BETA_REVIEW（等待 Beta 审核）。
- Friends 外部组公开链接已启用：https://testflight.apple.com/join/U8YYqHw5 。
- 需要审核通过后，朋友才能通过该链接安装；尚未宣称外测已获批准。

## 包与验证

- 应用内置 pak0.pk3 至 pak8.pk3，不显示导入流程；原始 Windows 资源目录未修改。
- Mac 原生引擎、SDL2、渲染器、图标、资源、许可和对应源码一起打包。
- 无 Homebrew 运行库依赖，配置保存在应用沙盒的 Application Support。
- 已验证地图、机器人、CoreAudio、手柄默认绑定和正常退出。
- Xbox / Switch Pro 的 SDL 虚拟设备生命周期测试通过；实体 USB/蓝牙仍待验收。
- 上传前已检查实际导出包的签名、Sandbox 权限、构建号及隔离属性。

## 修复记录

早期导出缺少 Sandbox 权限，已通过归档签名保留权限修复。
构建 2 上传后的导入因资源的 com.apple.quarantine 属性被拒绝；
构建 3 只清理生成应用的元数据并重新签名，已通过苹果处理。

## 构建与检查

```sh
python3 tools/apple-port/macos_release.py --version 0.1.0 --build 3 --resources '/path/to/baseq3'
python3 tools/apple-port/verify_export.py '/path/to/export/ioquake3.pkg' --build 3
```

归档入口使用本地 ad-hoc 签名保存 Sandbox 权限；正式导出仍需有效的 Apple
分发签名和描述文件。脚本不会自动上传。已有构建号不得重复使用，应先查询后台。
所有私钥、账号配置、联系人信息、描述文件、产物和上传日志均不提交 Git。

本地实际分发包：`.apple-port/release/export-3-sandbox/ioquake3.pkg`（约 478 MiB）。
本地构建索引：`.apple-port/release/latest-local.json`。

主仓库：https://github.com/tangyiyong/Quake-III-Arena-mac 。
main 和 codex/macos-gamepad-release 同步维护，原 id Software 远端为 upstream。

反馈邮箱与审核联系人邮箱用途不同。未经用户同意，不将审核联系人信息用于公开反馈地址。

参考：https://developer.apple.com/help/app-store-connect/test-a-beta-version/provide-test-information/

## 外测审核提交

用户已确认反馈邮箱用途，已补全简体中文主语言测试信息并提交 Beta App Review。
审核联系人、反馈邮箱与电话只配置在 Apple 后台，不记录在公开源码或日志文档中。

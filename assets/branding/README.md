# Arena 图标

通过内置 image_gen 工具生成。已目视检查原图，并导出 1024×1024 不透明 PNG、
macOS ICNS 与 iOS/macOS Xcode 图标资源目录。当前 Mac 构建已使用 Arena.icns。
图案为社区移植版的设计稿，不表示发行商官方版本。

## 文件

- `arena-logo-source.png`：生成原图。
- `arena-logo-1024.png`：分享预览及 1024 像素应用图标。
- `Arena.icns`：Mac 图标。
- `Assets.xcassets/AppIcon.appiconset`：iOS 通用图标。
- `Assets.xcassets/MacAppIcon.appiconset`：Mac 图标集。

重新导出：`python3 tools/apple-port/make_icons.py`。

## 生成提示词

Use case: logo-brand. Create a polished square app icon for a community Apple-platform port of Quake III Arena. One single powerful centered emblem, an original angular arena gateway / stylized Q silhouette incorporating three vertical metal prongs, reminiscent of late-1990s gothic science-fiction arena combat. Brushed dark gunmetal, restrained molten crimson edge light, subtle warm highlights, deep charcoal full-bleed background. Bold readable silhouette at 32 pixels, premium sculpted 3D metal relief, elegant sparse detail. 1024x1024 square opaque image. No words, no lettering, no tiny text, no Apple symbol, no official publisher logo, no mockup, no rounded-square outer corners, no border; entire canvas is the app icon artwork. Keep main emblem inside central 72 percent with clear contrast and generous breathing room.

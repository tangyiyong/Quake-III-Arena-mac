# Windows 与 macOS 资源体积核对

本机目录实测，单位 MiB（1 MiB = 1024×1024 字节）：

| Windows 安装内容 | 大小 | Mac 当前基础包 |
| --- | ---: | --- |
| baseq3/pak0.pk3 至 pak8.pk3 | 482.15 | 全部包含，未重新压缩 |
| baseq3 中额外地图、pak8a 和配置 | 152.31 | 不包含 |
| baseoa（OpenArena） | 391.22 | 不包含 |
| missionpack（Team Arena） | 342.39 | 不包含 |
| threewave 模组 | 31.92 | 不包含 |
| 三个 Windows EXE | 4.82 | 由原生 Mac 引擎替代 |
| 合计 | 1404.81 | — |

Mac 基础资源仍为 482.15 MiB，应用加上引擎、SDL、图标、许可与源码约 508 MiB。
体积差异来自内容范围，不是平台转换时丢失原版基础资源。
如需 Team Arena 或 Threewave，应另设扩展内容及模组兼容验收任务。

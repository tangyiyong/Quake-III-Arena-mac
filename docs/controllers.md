# macOS 手柄支持

## 范围

基于锁定版本 SDL2 2.32.8 的 GameController API，默认开启。目标设备包括
系统和 SDL 能识别的 Xbox One/Series 与 Nintendo Switch Pro 手柄。
USB/蓝牙的实际可用性取决于设备型号、固件、系统与连接方式；未做实体手柄验收。
Joy-Con 单只/配对、第三方兼容手柄、震动、陀螺仪、Steam Input 全局重映射不在本次实现范围。

## 默认布局

按物理位置统一 Xbox 与 Nintendo 布局，避免 AB/XY 标注相反造成混乱。

| 操作 | Xbox | Switch Pro |
| --- | --- | --- |
| 移动 / 视角 | 左 / 右摇杆 | 左 / 右摇杆 |
| 射击 / 缩放 | RT / LT | ZR / ZL |
| 跳跃（下方键） | A | B |
| 蹲下（右方键） | B | A |
| 使用道具（左方键） | X | Y |
| 下一武器（上方键） | Y | X |
| 上一 / 下一武器 | LB / RB | L / R |
| 行走 / 蹲下 | 左 / 右摇杆按下 | 左 / 右摇杆按下 |
| 计分板 | View / Back | Minus |
| 暂停菜单 / 返回 | Menu / Start | Plus |
| 菜单导航 | 左摇杆或十字键 | 左摇杆或十字键 |
| 菜单确认 / 返回 | A / B | B / A |

游戏中十字键上/右/下/左选择武器槽 2/3/5/7。
默认死区 18%；右摇杆水平最大约 197 度/秒、垂直约 147 度/秒。
菜单摇杆阈值 50%，支持按住重复导航。

## 设置

首次启用本移植版手柄配置时应用默认绑定，以后保留用户修改。
不会修改键盘和鼠标绑定。可在 Quake 控制台中调整：

```text
joy_threshold 0.18
j_yaw -0.006
j_pitch 0.0045
bind PAD0_A +moveup
bind PAD0_RIGHTTRIGGER +attack
```

恢复默认：`arena_gamepadReset`。如手动禁用过手柄，恢复后执行 `in_restart`。
禁用：`in_joystick 0`，再执行 `in_restart`。
选择不同设备：`in_joystickNo N`，再执行 `in_restart`。

设备热插拔/重映射时重建 SDL 连接，优先识别有标准映射的手柄；重建连接、
切换到后台时释放手柄按键和模拟轴。按键在菜单切换后仍释放最初产生的键，
避免跳跃/射击粘住。菜单接管后清零游戏模拟轴。

## 验证

`python3 tools/apple-port/test_gamepad.py` 使用真实 SDL 虚拟设备 API 和引擎事件
队列替身，分别以 Xbox / Switch Pro 的厂商产品标识创建虚拟手柄，验证：

- 游戏到菜单切换后的按键释放保持一致；
- 菜单确认键、摇杆方向、按住重复及退出菜单后的方向释放；
- 断开/失焦清理路径释放按钮、扳机和全部游戏模拟轴。

虚拟设备验证不覆盖实体 HID 协议、蓝牙配对、无线延迟或固件。
发布前应分别连接真实 Xbox 和 Switch Pro，验收冷启动、热插拔、双摇杆、
扳机、菜单、失焦恢复以及同时按住键鼠和手柄的行为。

SDL 按物理位置映射依据：
https://wiki.libsdl.org/SDL2/SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS

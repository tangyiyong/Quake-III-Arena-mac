// Included by the SDL platform layer after stick_state is declared.
// Shared SDL GameController layout: face buttons follow physical position.
#include "ArenaInputMath.h"
static Uint32 arenaGyroTick, arenaCalibrationStarted;
static Uint64 arenaCalibrationStamp;
static float arenaGyroBias[3], arenaCalibrationSum[3], arenaCalibrationSquares[3];
static int arenaCalibrationCount;
static qboolean arenaInputSuspended;
static void Arena_GyroMove(void)
{
    float data[3], yaw, pitch;
    Uint64 stamp = 0;
    Uint32 now = SDL_GetTicks();
    qboolean available = SDL_GameControllerHasSensor(gamepad, SDL_SENSOR_GYRO) == SDL_TRUE;
    qboolean calibrate = Cvar_VariableIntegerValue("arena_gyroCalibrate") != 0;
    qboolean enabled = Cvar_VariableIntegerValue("arena_gyroEnabled") != 0;
    if (!available) { arenaGyroTick = 0; return; }
    if (!enabled && !calibrate && !arenaCalibrationStarted) { arenaGyroTick = 0; return; }
    if (!SDL_GameControllerIsSensorEnabled(gamepad, SDL_SENSOR_GYRO) &&
        SDL_GameControllerSetSensorEnabled(gamepad, SDL_SENSOR_GYRO, SDL_TRUE) != 0) {
        Cvar_Set("arena_gyroStatus", "传感器开启失败，请重新连接手柄");
        arenaGyroTick = 0; return;
    }
    if (SDL_GameControllerGetSensorDataWithTimestamp(gamepad, SDL_SENSOR_GYRO, &stamp, data, 3) != 0) {
        arenaGyroTick = 0; return;
    }
    if (calibrate) {
        Cvar_Set("arena_gyroCalibrate", "0");
        arenaCalibrationStarted = now ? now : 1;
        arenaCalibrationCount = 0; arenaCalibrationStamp = 0;
        memset(arenaCalibrationSum, 0, sizeof(arenaCalibrationSum));
        memset(arenaCalibrationSquares, 0, sizeof(arenaCalibrationSquares));
        Cvar_Set("arena_gyroStatus", "正在校准，请保持手柄静止");
    }
    if (arenaCalibrationStarted) {
        int i;
        if (!stamp || stamp != arenaCalibrationStamp) {
            arenaCalibrationStamp = stamp;
            for (i = 0; i < 3; i++) {
                if (!isfinite(data[i]) || fabsf(data[i]) > 0.4f) {
                    arenaCalibrationStarted = 0;
                    Cvar_Set("arena_gyroStatus", "校准中检测到移动，请保持静止后重试");
                    arenaGyroTick = 0; return;
                }
                arenaCalibrationSum[i] += data[i];
                arenaCalibrationSquares[i] += data[i] * data[i];
            }
            arenaCalibrationCount++;
        }
        if (now - arenaCalibrationStarted >= 1000 && arenaCalibrationCount >= 20) {
            qboolean stable = qtrue;
            for (i = 0; i < 3; i++) {
                float mean = arenaCalibrationSum[i] / arenaCalibrationCount;
                if (arenaCalibrationSquares[i] / arenaCalibrationCount - mean * mean > 0.0009f) stable = qfalse;
            }
            if (stable) for (i = 0; i < 3; i++) arenaGyroBias[i] = arenaCalibrationSum[i] / arenaCalibrationCount;
            Cvar_Set("arena_gyroStatus", stable ? "校准完成" : "校准不稳定，请重试");
            arenaCalibrationStarted = 0;
        } else if (now - arenaCalibrationStarted > 5000) {
            arenaCalibrationStarted = 0;
            Cvar_Set("arena_gyroStatus", "未收到足够的传感器数据，请重试");
        }
        arenaGyroTick = 0; return;
    }
    if (!enabled || clc.state != CA_ACTIVE || Key_GetCatcher() ||
        Cvar_VariableIntegerValue("arena_settingsOpen") ||
        (Cvar_VariableIntegerValue("arena_gyroMode") &&
         SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) < 8000)) {
        arenaGyroTick = 0; return;
    }
    if (arenaGyroTick && Arena_GyroDelta(data, arenaGyroBias, (now - arenaGyroTick) * 0.001f,
            Cvar_VariableValue("arena_gyroSensitivity"), 0.015f,
            Cvar_VariableIntegerValue("arena_gyroInvert"), &yaw, &pitch)) {
        cl.viewangles[YAW] += yaw;
        cl.viewangles[PITCH] += pitch;
    }
    arenaGyroTick = now;
}

static void Arena_ControllerChanged(void)
{
    qboolean available = gamepad && SDL_GameControllerHasSensor(gamepad, SDL_SENSOR_GYRO);
    const char *name = gamepad ? SDL_GameControllerName(gamepad) : NULL;
    Cvar_Set("arena_controllerName", name ? name : "未连接标准手柄");
    Cvar_SetValue("arena_gyroAvailable", available);
    Cvar_Set("arena_gyroStatus", available ? "可用；启用后按所选方式瞄准" : "当前设备未提供陀螺仪（标准 Xbox 手柄无此传感器）");
    memset(arenaGyroBias, 0, sizeof(arenaGyroBias));
    arenaCalibrationStarted = arenaGyroTick = 0;
}
static keyNum_t IN_KeyFor(SDL_GameControllerButton button);
static int arenaButtonKeys[SDL_CONTROLLER_BUTTON_MAX];
static qboolean arenaMenuDirections[4];
static unsigned arenaMenuRepeat[4];
static const int arenaArrowKeys[4] = { K_LEFTARROW, K_RIGHTARROW, K_UPARROW, K_DOWNARROW };
static qboolean arenaInMenu;

static void Arena_NeutralAxes(void)
{
    int i;
    for (i = K_PAD0_LEFTSTICK_LEFT; i <= K_PAD0_RIGHTTRIGGER; i++)
        Com_QueueEvent(in_eventTime, SE_KEY, i, qfalse, 0, NULL);
    for (i = 0; i < MAX_JOYSTICK_AXIS; i++)
        Com_QueueEvent(in_eventTime, SE_JOYSTICK_AXIS, i, 0, 0, NULL);
    memset(stick_state.oldaaxes, 0, sizeof(stick_state.oldaaxes));
}

static void Arena_ReleaseController(void)
{
    int i;
    arenaGyroTick = arenaCalibrationStarted = 0;
    for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
        if (arenaButtonKeys[i])
            Com_QueueEvent(in_eventTime, SE_KEY, arenaButtonKeys[i], qfalse, 0, NULL);
        arenaButtonKeys[i] = 0;
        stick_state.buttons[i] = qfalse;
    }
    for (i = 0; i < 4; i++) {
        if (arenaMenuDirections[i])
            Com_QueueEvent(in_eventTime, SE_KEY, arenaArrowKeys[i], qfalse, 0, NULL);
        arenaMenuDirections[i] = qfalse;
    }
    Arena_NeutralAxes();
    arenaInMenu = qfalse;
}

static int Arena_ButtonKey(SDL_GameControllerButton button)
{
    if (button == SDL_CONTROLLER_BUTTON_START) return K_ESCAPE;
    if (Key_GetCatcher()) {
        switch (button) {
            case SDL_CONTROLLER_BUTTON_A: return K_ENTER;
            case SDL_CONTROLLER_BUTTON_B: return K_ESCAPE;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return K_LEFTARROW;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return K_RIGHTARROW;
            case SDL_CONTROLLER_BUTTON_DPAD_UP: return K_UPARROW;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return K_DOWNARROW;
            default: break;
        }
    }
    return IN_KeyFor(button);
}

static void Arena_Button(SDL_GameControllerButton button, qboolean pressed)
{
    int key;
#ifdef ARENA_RESOURCE_ONBOARDING
    if (button == SDL_CONTROLLER_BUTTON_GUIDE) {
        extern void Arena_OpenSettings(void);
        if (pressed) Arena_OpenSettings();
        return;
    }
#endif
    if (pressed) arenaButtonKeys[button] = Arena_ButtonKey(button);
    key = arenaButtonKeys[button]; // release the original key if the menu changed
    if (key) Com_QueueEvent(in_eventTime, SE_KEY, key, pressed, 0, NULL);
    if (!pressed) arenaButtonKeys[button] = 0;
}

static qboolean Arena_MenuStick(void)
{
    int i;
    qboolean menu = Key_GetCatcher() != 0;
    int x = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX);
    int y = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY);
    qboolean held[4] = { x < -16384, x > 16384, y < -16384, y > 16384 };
    if (menu && !arenaInMenu) Arena_NeutralAxes();
    for (i = 0; i < 4; i++) {
        qboolean down = menu && held[i];
        if (down != arenaMenuDirections[i]) {
            Com_QueueEvent(in_eventTime, SE_KEY, arenaArrowKeys[i], down, 0, NULL);
            arenaMenuDirections[i] = down;
            arenaMenuRepeat[i] = (unsigned)in_eventTime + 350;
        } else if (down && (int)((unsigned)in_eventTime - arenaMenuRepeat[i]) >= 0) {
            Com_QueueEvent(in_eventTime, SE_KEY, arenaArrowKeys[i], qfalse, 0, NULL);
            Com_QueueEvent(in_eventTime, SE_KEY, arenaArrowKeys[i], qtrue, 0, NULL);
            arenaMenuRepeat[i] = (unsigned)in_eventTime + 160;
        }
    }
    arenaInMenu = menu;
    return menu;
}

static void Arena_ResetGamepad_f(void)
{
    static const struct { int key; const char *command; } binds[] = {
        { K_PAD0_LEFTSTICK_LEFT, "+moveleft" }, { K_PAD0_LEFTSTICK_RIGHT, "+moveright" },
        { K_PAD0_LEFTSTICK_UP, "+forward" }, { K_PAD0_LEFTSTICK_DOWN, "+back" },
        { K_PAD0_RIGHTSTICK_LEFT, "+left" }, { K_PAD0_RIGHTSTICK_RIGHT, "+right" },
        { K_PAD0_RIGHTSTICK_UP, "+lookup" }, { K_PAD0_RIGHTSTICK_DOWN, "+lookdown" },
        { K_PAD0_A, "+moveup" }, { K_PAD0_B, "+movedown" },
        { K_PAD0_X, "+button2" }, { K_PAD0_Y, "weapnext" },
        { K_PAD0_LEFTSHOULDER, "weapprev" }, { K_PAD0_RIGHTSHOULDER, "weapnext" },
        { K_PAD0_LEFTTRIGGER, "+zoom" }, { K_PAD0_RIGHTTRIGGER, "+attack" },
        { K_PAD0_LEFTSTICK_CLICK, "+speed" }, { K_PAD0_RIGHTSTICK_CLICK, "+movedown" },
        { K_PAD0_BACK, "+scores" }, { K_PAD0_DPAD_UP, "weapon 2" },
        { K_PAD0_DPAD_RIGHT, "weapon 3" }, { K_PAD0_DPAD_DOWN, "weapon 5" },
        { K_PAD0_DPAD_LEFT, "weapon 7" }
    };
    int i;
    for (i = 0; i < ARRAY_LEN(binds); i++) Key_SetBinding(binds[i].key, binds[i].command);
    Cvar_Set2("in_joystick", "1", qtrue);
    Cvar_Set("in_joystickUseAnalog", "1");
    Cvar_Set("joy_threshold", "0.18");
    Cvar_Set("j_side_axis", "0"); Cvar_Set("j_forward_axis", "1");
    Cvar_Set("j_yaw_axis", "2"); Cvar_Set("j_pitch_axis", "3");
    Cvar_Set("j_up_axis", "4");
    Cvar_Set("j_side", "0.00387585"); Cvar_Set("j_forward", "-0.00387585");
    Cvar_Set("j_yaw", "-0.006"); Cvar_Set("j_pitch", "0.0045");
    Cvar_Set("j_up", "0");
    Cvar_Set("arena_gyroEnabled", "0");
    Cvar_Set("arena_gyroSensitivity", "1");
    Cvar_Set("arena_gyroMode", "1");
    Cvar_Set("arena_gyroInvert", "0");
    Cvar_Set("arena_gamepadProfile", "1");
    Com_Printf("Arena gamepad defaults applied (Xbox / Switch positional layout).\n");
}

static void Arena_InitGamepad(void)
{
    // Configure before SDL initializes HID drivers. Nintendo south=B then has
    // the same gameplay role as Xbox south=A; no per-device bindings needed.
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    Cmd_AddCommand("arena_gamepadReset", Arena_ResetGamepad_f);
    Cvar_Get("in_joystickUseAnalog", "0", CVAR_ARCHIVE);
    if (!Cvar_Get("arena_gamepadProfile", "0", CVAR_ARCHIVE)->integer)
        Arena_ResetGamepad_f();
    Cvar_CheckRange(in_joystickThreshold, 0, 0.95f, qfalse);
    Cvar_Get("arena_gyroEnabled", "0", CVAR_ARCHIVE);
    Cvar_Get("arena_gyroMode", "1", CVAR_ARCHIVE);
    Cvar_Get("arena_gyroSensitivity", "1", CVAR_ARCHIVE);
    Cvar_Get("arena_gyroInvert", "0", CVAR_ARCHIVE);
    Cvar_Get("arena_gyroCalibrate", "0", CVAR_TEMP);
    Cvar_Get("arena_gyroAvailable", "0", CVAR_ROM);
    Cvar_Get("arena_controllerName", "未连接标准手柄", CVAR_ROM);
    Cvar_Get("arena_gyroStatus", "未检测到传感器", CVAR_ROM);
    Cvar_Get("arena_settingsOpen", "0", CVAR_ROM);
#ifdef ARENA_RESOURCE_ONBOARDING
    {
        extern void Arena_OpenSettings(void);
        extern void Arena_InstallSettingsMenu(void);
        Cmd_AddCommand("arena_settings", Arena_OpenSettings);
        Arena_InstallSettingsMenu();
    }
#endif
}

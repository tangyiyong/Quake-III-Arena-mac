// Included by the SDL platform layer after stick_state is declared.
// Shared SDL GameController layout: face buttons follow physical position.
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
}

// Exercise lifecycle behavior with real SDL virtual controllers and queued
// engine events. Physical Bluetooth/USB hardware is a separate acceptance test.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "client.h"

static SDL_GameController *gamepad;
static int in_eventTime;
static cvar_t *in_joystickThreshold;
static struct {
    qboolean buttons[SDL_CONTROLLER_BUTTON_MAX + 1];
    int oldaaxes[MAX_JOYSTICK_AXIS];
} stick_state;
static int catcher;
static struct { sysEventType_t type; int key, value; } events[512];
static int count;
int Key_GetCatcher(void) { return catcher; }
void Com_QueueEvent(int time, sysEventType_t type, int key, int value, int length, void *ptr)
{
    assert(count < 512);
    events[count].type = type; events[count].key = key; events[count].value = value; count++;
}
static keyNum_t IN_KeyFor(SDL_GameControllerButton button)
{
    return button == SDL_CONTROLLER_BUTTON_A ? K_PAD0_A : K_PAD0_B;
}
#include "../macos/ArenaGamepad.h"

static int has(sysEventType_t type, int key, int value)
{
    for (int i = 0; i < count; i++)
        if (events[i].type == type && events[i].key == key && events[i].value == value) return 1;
    return 0;
}

static void scenario(Uint16 vendor, Uint16 product, const char *name)
{
    SDL_VirtualJoystickDesc desc;
    SDL_zero(desc);
    desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes = SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
    desc.axis_mask = (1u << SDL_CONTROLLER_AXIS_MAX) - 1;
    desc.button_mask = (1u << SDL_CONTROLLER_BUTTON_MAX) - 1;
    desc.vendor_id = vendor; desc.product_id = product; desc.name = name;
    int device = SDL_JoystickAttachVirtualEx(&desc);
    assert(device >= 0 && SDL_IsGameController(device));
    gamepad = SDL_GameControllerOpen(device);
    assert(gamepad);
    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(gamepad);

    // A key pressed in-game must release that same key after opening a menu.
    count = 0; catcher = 0;
    Arena_Button(SDL_CONTROLLER_BUTTON_A, qtrue);
    catcher = KEYCATCH_UI;
    Arena_Button(SDL_CONTROLLER_BUTTON_A, qfalse);
    assert(has(SE_KEY, K_PAD0_A, 1) && has(SE_KEY, K_PAD0_A, 0));
    assert(!has(SE_KEY, K_ENTER, 0));
    count = 0;
    Arena_Button(SDL_CONTROLLER_BUTTON_A, qtrue);
    Arena_Button(SDL_CONTROLLER_BUTTON_A, qfalse);
    assert(has(SE_KEY, K_ENTER, 1) && has(SE_KEY, K_ENTER, 0));
    assert(Arena_ButtonKey(SDL_CONTROLLER_BUTTON_START) == K_ESCAPE);

    // Real SDL axis updates drive menu navigation, repeat, and neutralization.
    assert(SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_LEFTX, 28000) == 0);
    SDL_GameControllerUpdate();
    count = 0; in_eventTime = 1000;
    assert(Arena_MenuStick());
    assert(has(SE_KEY, K_RIGHTARROW, 1));
    assert(has(SE_JOYSTICK_AXIS, 0, 0));
    count = 0; in_eventTime = 1100; Arena_MenuStick();
    assert(count == 0);
    in_eventTime = 1400; Arena_MenuStick();
    assert(has(SE_KEY, K_RIGHTARROW, 0) && has(SE_KEY, K_RIGHTARROW, 1));
    count = 0; catcher = 0;
    assert(!Arena_MenuStick());
    assert(has(SE_KEY, K_RIGHTARROW, 0));

    // Disconnection/focus loss releases held buttons, triggers and analog axes.
    Arena_Button(SDL_CONTROLLER_BUTTON_A, qtrue);
    stick_state.oldaaxes[0] = 28000;
    count = 0; Arena_ReleaseController();
    assert(has(SE_KEY, K_PAD0_A, 0));
    assert(has(SE_KEY, K_PAD0_RIGHTTRIGGER, 0));
    for (int axis = 0; axis < MAX_JOYSTICK_AXIS; axis++)
        assert(has(SE_JOYSTICK_AXIS, axis, 0) && stick_state.oldaaxes[axis] == 0);
    SDL_GameControllerClose(gamepad); gamepad = NULL;
    assert(SDL_JoystickDetachVirtual(device) == 0);
    printf("PASS: %s virtual controller lifecycle\n", name);
}

int main(void)
{
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    assert(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    scenario(0x045e, 0x02ea, "Xbox virtual test");
    scenario(0x057e, 0x2009, "Switch Pro virtual test");
    SDL_Quit();
    return 0;
}

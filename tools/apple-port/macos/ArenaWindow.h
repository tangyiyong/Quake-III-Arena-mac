#include "ArenaInputMath.h"
static void Arena_FitGameWindow(SDL_Window *window, int display, int fullscreen,
                               int *width, int *height, float *aspect)
{
    SDL_Rect usable;
    int top = 0, left = 0, bottom = 0, right = 0;
    int w = *width, h = *height, x, y;
    if (fullscreen || SDL_GetDisplayUsableBounds(display, &usable) != 0) return;
    SDL_GetWindowBordersSize(window, &top, &left, &bottom, &right);
    SDL_SetWindowMaximumSize(window, usable.w > 24 ? usable.w - 24 : 1,
                            usable.h > 64 ? usable.h - 64 : 1);
    Arena_FitSize(&w, &h, usable.w - left - right - 24, usable.h - top - bottom - 24);
    if (w != *width || h != *height) SDL_SetWindowSize(window, w, h);
    SDL_GetWindowSize(window, &w, &h);
    // Leave room for decorations on every side and keep the content origin
    // inside the usable desktop even with negative-coordinate external screens.
    x = usable.x + left + (usable.w - left - right - w) / 2;
    y = usable.y + top + (usable.h - top - bottom - h) / 2;
    SDL_SetWindowPosition(window, x, y);
    *width = w; *height = h; *aspect = (float)w / h;
    ri.Printf(PRINT_ALL, "Arena window fit: content=%dx%d usable=%dx%d borders=%d,%d,%d,%d\n",
              w, h, usable.w, usable.h, top, left, bottom, right);
}

#ifndef ARENA_INPUT_MATH_H
#define ARENA_INPUT_MATH_H
#include <math.h>

// Window dimensions are desktop points, not Retina framebuffer pixels.
static void Arena_FitSize(int *width, int *height, int availableWidth, int availableHeight)
{
    double scale = 1.0;
    if (*width < 1) *width = 640;
    if (*height < 1) *height = 480;
    if (availableWidth < 1) availableWidth = 1;
    if (availableHeight < 1) availableHeight = 1;
    if (*width > availableWidth) scale = (double)availableWidth / *width;
    if (*height * scale > availableHeight) scale = (double)availableHeight / *height;
    *width = (int)(*width * scale);
    *height = (int)(*height * scale);
    if (*width < 1) *width = 1;
    if (*height < 1) *height = 1;
}

// SDL gyroscope rates are radians/second. Discard invalid readings and cap
// integration after a stall so a menu/focus transition cannot cause a jump.
static int Arena_GyroDelta(const float rates[3], const float bias[3], float seconds,
                           float sensitivity, float deadzone, int invert,
                           float *yaw, float *pitch)
{
    float x = rates[0] - bias[0], y = rates[1] - bias[1];
    *yaw = *pitch = 0;
    if (!isfinite(x) || !isfinite(y) || !isfinite(seconds) ||
        !isfinite(sensitivity) || seconds <= 0 || fabsf(x) > 20 || fabsf(y) > 20) return 0;
    if (seconds > 0.05f) seconds = 0.05f;
    if (sensitivity < 0.1f) sensitivity = 0.1f;
    if (sensitivity > 4.0f) sensitivity = 4.0f;
    if (fabsf(x) < deadzone) x = 0;
    if (fabsf(y) < deadzone) y = 0;
    *yaw = y * seconds * 57.2957795f * sensitivity;
    *pitch = x * seconds * 57.2957795f * sensitivity * (invert ? 1 : -1);
    return 1;
}
#endif

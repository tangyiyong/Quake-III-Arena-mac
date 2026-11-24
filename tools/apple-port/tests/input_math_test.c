#include <assert.h>
#include <stdio.h>
#include "../macos/ArenaInputMath.h"

int main(void)
{
    int w = 2560, h = 1600;
    Arena_FitSize(&w, &h, 1280, 700);
    assert(w == 1120 && h == 700);
    w = 3840; h = 2160;
    Arena_FitSize(&w, &h, 800, 600);
    assert(w == 800 && h == 450);
    w = 640; h = 480;
    Arena_FitSize(&w, &h, 320, 100);
    assert(w <= 320 && h <= 100 && w > 0 && h > 0);
    w = h = 0;
    Arena_FitSize(&w, &h, 0, 0);
    assert(w == 1 && h == 1);

    float rates[3] = { 0, 1, 0 }, bias[3] = { 0, 0, 0 }, yaw, pitch;
    assert(Arena_GyroDelta(rates, bias, 0.01f, 1, 0.015f, 0, &yaw, &pitch));
    assert(fabsf(yaw - 0.5729578f) < 0.00001f && pitch == 0);
    float total60 = 0, total125 = 0;
    for (int i = 0; i < 60; i++) { Arena_GyroDelta(rates, bias, 1.0f/60, 1, 0, 0, &yaw, &pitch); total60 += yaw; }
    for (int i = 0; i < 125; i++) { Arena_GyroDelta(rates, bias, 1.0f/125, 1, 0, 0, &yaw, &pitch); total125 += yaw; }
    assert(fabsf(total60 - total125) < 0.001f);
    Arena_GyroDelta(rates, bias, 2, 1, 0, 0, &yaw, &pitch);
    assert(fabsf(yaw - 2.864789f) < 0.0001f); // stall must not cause a jump
    bias[1] = 1;
    Arena_GyroDelta(rates, bias, 0.01f, 1, 0, 0, &yaw, &pitch);
    assert(yaw == 0);
    rates[0] = 1;
    Arena_GyroDelta(rates, bias, 0.01f, 1, 0, 0, &yaw, &pitch);
    float normal = pitch;
    Arena_GyroDelta(rates, bias, 0.01f, 1, 0, 1, &yaw, &pitch);
    assert(pitch == -normal);
    rates[0] = 0.01f;
    Arena_GyroDelta(rates, bias, 0.01f, 1, 0.015f, 0, &yaw, &pitch);
    assert(yaw == 0 && pitch == 0);
    rates[0] = NAN;
    assert(!Arena_GyroDelta(rates, bias, 0.01f, 1, 0, 0, &yaw, &pitch));
    assert(yaw == 0 && pitch == 0);
    puts("PASS: small/Retina-sized window fitting and frame-rate-independent gyro integration");
}

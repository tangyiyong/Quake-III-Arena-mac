#import <Cocoa/Cocoa.h>
#include "client.h"

@interface ArenaSettings : NSObject <NSWindowDelegate> {
    NSWindow *window;
    NSTextField *controller, *status;
    NSButton *enabled, *invert, *gyro, *gyroInvert, *fullscreen, *calibrate;
    NSSlider *deadzone, *yaw, *pitch, *gyroGain;
    NSPopUpButton *gyroMode;
    NSTimer *timer;
}
- (void)show:(id)sender;
@end

@interface ArenaSettingsContent : NSView
@end
@implementation ArenaSettingsContent
- (BOOL)isFlipped { return YES; }
@end

static NSTextField *ArenaLabel(NSView *parent, NSString *text, CGFloat x, CGFloat y, CGFloat w, CGFloat h)
{
    NSTextField *label = [NSTextField wrappingLabelWithString:text];
    label.frame = NSMakeRect(x, y, w, h);
    [parent addSubview:label];
    return label;
}
static NSButton *ArenaCheck(NSView *parent, NSString *title, CGFloat y)
{
    NSButton *button = [NSButton checkboxWithTitle:title target:nil action:nil];
    button.frame = NSMakeRect(20, y, 540, 28);
    [parent addSubview:button];
    return button;
}
static NSSlider *ArenaSlider(NSView *parent, NSString *title, CGFloat y, double minimum, double maximum)
{
    ArenaLabel(parent, title, 20, y + 4, 205, 24);
    NSSlider *slider = [NSSlider sliderWithValue:minimum minValue:minimum maxValue:maximum target:nil action:nil];
    slider.frame = NSMakeRect(230, y, 310, 28);
    slider.accessibilityLabel = title;
    [parent addSubview:slider];
    return slider;
}

@implementation ArenaSettings
- (void)buildWindow
{
    NSRect visible = (NSApp.keyWindow.screen ?: NSScreen.mainScreen).visibleFrame;
    CGFloat width = MIN(600, visible.size.width - 32);
    CGFloat height = MIN(610, visible.size.height - 48);
    window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, width, height)
                                       styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                         backing:NSBackingStoreBuffered defer:NO];
    window.title = @"Arena — 手柄与显示设置";
    window.releasedWhenClosed = NO;
    window.delegate = self;
    window.minSize = NSMakeSize(MIN(440, width), MIN(360, height));
    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 60, width, height - 60)];
    scroll.hasVerticalScroller = YES;
    scroll.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    ArenaSettingsContent *content = [[ArenaSettingsContent alloc] initWithFrame:NSMakeRect(0, 0, 570, 660)];
    scroll.documentView = content;
    scroll.hasHorizontalScroller = YES;
    [window.contentView addSubview:scroll];
    ArenaLabel(content, @"已连接的手柄", 20, 12, 540, 22);
    controller = ArenaLabel(content, @"", 20, 38, 540, 26);
    status = ArenaLabel(content, @"", 20, 72, 540, 48);
    enabled = ArenaCheck(content, @"启用手柄", 128);
    deadzone = ArenaSlider(content, @"摇杆死区（0–50%）", 170, 0, 0.5);
    yaw = ArenaSlider(content, @"水平转向速度（度/秒）", 212, 30, 360);
    pitch = ArenaSlider(content, @"垂直转向速度（度/秒）", 254, 30, 300);
    invert = ArenaCheck(content, @"反转右摇杆垂直瞄准", 294);
    gyro = ArenaCheck(content, @"启用陀螺仪瞄准", 340);
    ArenaLabel(content, @"陀螺仪启动方式", 20, 384, 205, 24);
    gyroMode = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(230, 378, 310, 32) pullsDown:NO];
    [gyroMode addItemsWithTitles:@[@"始终（仅在游戏中）", @"按住左扳机时"]];
    [content addSubview:gyroMode];
    gyroGain = ArenaSlider(content, @"陀螺仪灵敏度（0.1–4 倍）", 422, 0.1, 4);
    gyroInvert = ArenaCheck(content, @"反转陀螺仪垂直瞄准", 462);
    calibrate = [NSButton buttonWithTitle:@"校准陀螺仪（保持手柄静止）" target:self action:@selector(calibrate:)];
    calibrate.frame = NSMakeRect(20, 504, 520, 34);
    [content addSubview:calibrate];
    fullscreen = ArenaCheck(content, @"全屏显示（窗口模式会自动适应可用屏幕）", 548);
    ArenaLabel(content, @"Xbox 标准手柄没有陀螺仪。Switch Pro 等设备需由系统和连接方式提供传感器数据。设置也可从应用菜单、F10 或可用的手柄 Guide 键打开。", 20, 592, 530, 60);
    NSButton *reset = [NSButton buttonWithTitle:@"恢复默认按键" target:self action:@selector(reset:)];
    reset.frame = NSMakeRect(16, 15, 150, 32);
    [window.contentView addSubview:reset];
    NSButton *apply = [NSButton buttonWithTitle:@"应用并返回游戏" target:self action:@selector(apply:)];
    apply.frame = NSMakeRect(width - 184, 15, 168, 32);
    apply.autoresizingMask = NSViewMinXMargin;
    apply.keyEquivalent = @"\r";
    [window.contentView addSubview:apply];
    [window center];
    NSRect frame = window.frame;
    frame.origin.x = NSMidX(visible) - frame.size.width / 2;
    frame.origin.y = NSMidY(visible) - frame.size.height / 2;
    [window setFrame:frame display:NO];
}
- (void)refresh:(id)sender
{
    controller.stringValue = [NSString stringWithUTF8String:Cvar_VariableString("arena_controllerName")] ?: @"未连接";
    status.stringValue = [NSString stringWithUTF8String:Cvar_VariableString("arena_gyroStatus")] ?: @"";
    BOOL available = Cvar_VariableIntegerValue("arena_gyroAvailable") != 0;
    gyro.enabled = gyroMode.enabled = gyroGain.enabled = gyroInvert.enabled = calibrate.enabled = available;
}
- (void)show:(id)sender
{
    if (!window) [self buildWindow];
    enabled.state = Cvar_VariableIntegerValue("in_joystick") ? NSControlStateValueOn : NSControlStateValueOff;
    deadzone.doubleValue = Cvar_VariableValue("joy_threshold");
    yaw.doubleValue = fabs(Cvar_VariableValue("j_yaw")) * 32767;
    pitch.doubleValue = fabs(Cvar_VariableValue("j_pitch")) * 32767;
    invert.state = Cvar_VariableValue("j_pitch") < 0;
    gyro.state = Cvar_VariableIntegerValue("arena_gyroEnabled") != 0;
    gyroInvert.state = Cvar_VariableIntegerValue("arena_gyroInvert") != 0;
    gyroGain.doubleValue = Cvar_VariableValue("arena_gyroSensitivity");
    [gyroMode selectItemAtIndex:Cvar_VariableIntegerValue("arena_gyroMode") ? 1 : 0];
    fullscreen.state = Cvar_VariableIntegerValue("r_fullscreen") != 0;
    Cvar_Set("arena_settingsOpen", "1");
    [self refresh:nil];
    [timer invalidate];
    timer = [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(refresh:) userInfo:nil repeats:YES];
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    Com_DPrintf("Arena settings opened: frame=%.0fx%.0f scrollable=yes\n", window.frame.size.width, window.frame.size.height);
}
- (void)apply:(id)sender
{
    BOOL changedInput = (enabled.state != 0) != (Cvar_VariableIntegerValue("in_joystick") != 0);
    BOOL changedVideo = (fullscreen.state != 0) != (Cvar_VariableIntegerValue("r_fullscreen") != 0);
    Cvar_Set2("in_joystick", enabled.state ? "1" : "0", qtrue);
    Cvar_SetValue("joy_threshold", deadzone.doubleValue);
    Cvar_SetValue("j_yaw", -yaw.doubleValue / 32767);
    Cvar_SetValue("j_pitch", pitch.doubleValue / 32767 * (invert.state ? -1 : 1));
    Cvar_SetValue("arena_gyroEnabled", gyro.state != 0);
    Cvar_SetValue("arena_gyroMode", gyroMode.indexOfSelectedItem);
    Cvar_SetValue("arena_gyroSensitivity", gyroGain.doubleValue);
    Cvar_SetValue("arena_gyroInvert", gyroInvert.state != 0);
    [window close];
    if (changedVideo) {
        Cvar_Set("r_mode", "-2");
        Cvar_SetValue("r_fullscreen", fullscreen.state != 0);
        Cbuf_AddText("vid_restart\n");
    } else if (changedInput) Cbuf_AddText("in_restart\n");
    Cbuf_AddText("writeconfig q3config.cfg\n");
}
- (void)calibrate:(id)sender { Cvar_Set("arena_gyroCalibrate", "1"); }
- (void)reset:(id)sender { [window close]; Cbuf_AddText("arena_gamepadReset\nin_restart\n"); }
- (void)windowWillClose:(NSNotification *)notification
{
    [timer invalidate]; timer = nil;
    Cvar_Set("arena_settingsOpen", "0");
}
@end

static ArenaSettings *arenaSettings;
void Arena_OpenSettings(void)
{
    if (!arenaSettings) arenaSettings = [ArenaSettings new];
    [arenaSettings show:nil];
}
void Arena_InstallSettingsMenu(void)
{
    NSMenu *menu = NSApp.mainMenu.itemArray.firstObject.submenu;
    if (!menu || [menu itemWithTag:9007]) return;
    if (!arenaSettings) arenaSettings = [ArenaSettings new];
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:@"手柄与显示设置…" action:@selector(show:) keyEquivalent:@","];
    item.target = arenaSettings;
    item.tag = 9007;
    [menu insertItem:item atIndex:MIN(2, menu.numberOfItems)];
    Com_DPrintf("Arena settings menu installed (Command-comma / F10)\n");
}

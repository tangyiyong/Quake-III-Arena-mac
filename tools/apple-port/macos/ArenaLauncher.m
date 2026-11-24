// Native startup for the self-contained ioquake3-based macOS community port.
#import <Cocoa/Cocoa.h>

int Arena_PrepareArguments(int *argc, char ***argv) {
    @autoreleasepool {
        NSFileManager *fm = NSFileManager.defaultManager;
        NSURL *root = [NSBundle.mainBundle.resourceURL URLByAppendingPathComponent:@"GameData" isDirectory:YES];
        for (int i = 0; i < 9; i++) {
            NSURL *file = [root URLByAppendingPathComponent:[NSString stringWithFormat:@"baseq3/pak%d.pk3", i]];
            NSDictionary *attributes = [fm attributesOfItemAtPath:file.path error:nil];
            if (![attributes[NSFileType] isEqual:NSFileTypeRegular] ||
                [attributes[NSFileSize] unsignedLongLongValue] == 0) {
                [NSApplication sharedApplication];
                [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
                [NSApp activateIgnoringOtherApps:YES];
                NSAlert *alert = [NSAlert new];
                alert.messageText = @"游戏资源不完整";
                alert.informativeText = @"应用中的基础资源包缺失或损坏，请重新安装应用。";
                [alert runModal];
                return -1;
            }
        }
        NSURL *support = [fm URLsForDirectory:NSApplicationSupportDirectory
                                   inDomains:NSUserDomainMask].firstObject;
        NSURL *config = [[support URLByAppendingPathComponent:NSBundle.mainBundle.bundleIdentifier]
                                  URLByAppendingPathComponent:@"Config" isDirectory:YES];
        NSError *error = nil;
        if (![fm createDirectoryAtURL:config withIntermediateDirectories:YES attributes:nil error:&error]) {
            fprintf(stderr, "Cannot create configuration directory: %s\n", error.localizedDescription.UTF8String);
            return -1;
        }
        NSArray<NSString *> *arguments = NSProcessInfo.processInfo.arguments;
        NSMutableArray<NSString *> *launch = [arguments mutableCopy];
        // Append immutable bundle resource paths and sandbox-owned configuration
        // paths last. QVM execution stays interpreted and needs no JIT entitlement.
        if (![fm fileExistsAtPath:[[config URLByAppendingPathComponent:@"baseq3/q3config.cfg"] path]])
            [launch addObjectsFromArray:@[@"+set", @"r_fullscreen", @"0", @"+set", @"r_mode", @"-1",
                                         @"+set", @"r_customwidth", @"1280", @"+set", @"r_customheight", @"720"]];
        [launch addObjectsFromArray:@[@"+set", @"cl_renderer", @"opengl1",
            @"+set", @"com_introplayed", @"1", @"+set", @"cl_allowDownload", @"0",
            @"+set", @"fs_basepath", root.path, @"+set", @"fs_homepath", config.path,
            @"+set", @"vm_game", @"1", @"+set", @"vm_cgame", @"1", @"+set", @"vm_ui", @"1"]];
        char **result = calloc(launch.count + 1, sizeof(char *));
        if (!result) return -1;
        for (NSUInteger i = 0; i < launch.count; i++) result[i] = strdup(launch[i].UTF8String);
        *argc = (int)launch.count;
        *argv = result;
        return 1;
    }
}

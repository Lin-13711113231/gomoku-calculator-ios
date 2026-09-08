#import <Capacitor/Capacitor.h>

#import "RapfiEngineBridge.h"

@interface RapfiPlugin : CAPPlugin
@end

static void RapfiLineReceived(const char *line, void *context)
{
    RapfiPlugin *plugin = (__bridge RapfiPlugin *)context;
    if (!plugin || !line) return;

    NSString *message = [[NSString alloc] initWithUTF8String:line];
    dispatch_async(dispatch_get_main_queue(), ^{
        [plugin notifyListeners:@"line" data:@{ @"line": message ?: @"" }];
    });
}

@implementation RapfiPlugin

- (void)start:(CAPPluginCall *)call
{
    NSString *configPath = [[NSBundle mainBundle] pathForResource:@"config"
                                                              ofType:@"toml"
                                                         inDirectory:@"public/Rapfi"];
    if (!configPath) {
        configPath = [[NSBundle mainBundle] pathForResource:@"config"
                                                       ofType:@"toml"
                                                  inDirectory:@"Rapfi"];
    }

    if (!configPath || !rapfi_engine_start(configPath.UTF8String, RapfiLineReceived, (__bridge void *)self)) {
        [call reject:@"Native Rapfi could not start"];
        return;
    }

    [call resolve:@{ @"engine": @"rapfi-native", @"threads": @YES }];
}

- (void)send:(CAPPluginCall *)call
{
    NSString *command = [call getString:@"command"];
    if (command.length == 0) {
        [call reject:@"Missing command"];
        return;
    }

    rapfi_engine_send(command.UTF8String);
    [call resolve];
}

- (void)stop:(CAPPluginCall *)call
{
    rapfi_engine_stop();
    [call resolve];
}

@end

CAP_PLUGIN(RapfiPlugin, "Rapfi",
           CAP_PLUGIN_METHOD(start, CAPPluginReturnPromise);
           CAP_PLUGIN_METHOD(send, CAPPluginReturnPromise);
           CAP_PLUGIN_METHOD(stop, CAPPluginReturnPromise);)

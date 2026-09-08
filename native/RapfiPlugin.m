// Do not import the umbrella Capacitor.h here. It imports headers which use
// Xcode's module-enabled Pod environment (including Cordova), while this
// small adapter is compiled separately by CMake. These four Objective-C
// headers are sufficient for a CAPBridgedPlugin and work as plain headers.
#import "CAPPlugin.h"
#import "CAPPluginCall.h"
#import "CAPBridgedPlugin.h"
#import "CAPPluginMethod.h"

#import "RapfiEngineBridge.h"

@interface RapfiPlugin : CAPPlugin <CAPBridgedPlugin>
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

- (NSString *)identifier
{
    return @"RapfiPlugin";
}

- (NSString *)jsName
{
    return @"Rapfi";
}

- (NSArray<CAPPluginMethod *> *)pluginMethods
{
    return @[
        [[CAPPluginMethod alloc] initWithName:@"start" returnType:CAPPluginReturnPromise],
        [[CAPPluginMethod alloc] initWithName:@"send" returnType:CAPPluginReturnPromise],
        [[CAPPluginMethod alloc] initWithName:@"stop" returnType:CAPPluginReturnPromise],
    ];
}

- (void)resolve:(CAPPluginCall *)call
{
    if (call.successHandler) call.successHandler(nil, call);
}

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
        [self notifyListeners:@"line" data:@{ @"line": @"ERROR Native Rapfi could not start" }];
        [self resolve:call];
        return;
    }

    [self resolve:call];
}

- (void)send:(CAPPluginCall *)call
{
    id value = call.options[@"command"];
    NSString *command = [value isKindOfClass:[NSString class]] ? value : nil;
    if (command.length == 0) {
        [self notifyListeners:@"line" data:@{ @"line": @"ERROR Missing command" }];
        [self resolve:call];
        return;
    }

    rapfi_engine_send(command.UTF8String);
    [self resolve:call];
}

- (void)stop:(CAPPluginCall *)call
{
    rapfi_engine_stop();
    [self resolve:call];
}

@end

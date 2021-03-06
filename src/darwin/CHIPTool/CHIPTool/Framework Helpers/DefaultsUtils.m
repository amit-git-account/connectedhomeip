/**
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#import "DefaultsUtils.h"

NSString * const kCHIPToolDefaultsDomain = @"com.apple.chiptool";
NSString * const kNetworkSSIDDefaultsKey = @"networkSSID";
NSString * const kNetworkPasswordDefaultsKey = @"networkPassword";

id CHIPGetDomainValueForKey(NSString * domain, NSString * key)
{
    id value = (id) CFBridgingRelease(CFPreferencesCopyAppValue((CFStringRef) key, (CFStringRef) domain));
    if (value) {
        return value;
    }
    return nil;
}

void CHIPSetDomainValueForKey(NSString * domain, NSString * key, id value)
{
    CFPreferencesSetAppValue((CFStringRef) key, (__bridge CFPropertyListRef _Nullable)(value), (CFStringRef) domain);
    CFPreferencesAppSynchronize((CFStringRef) domain);
}

void CHIPRemoveDomainValueForKey(NSString * domain, NSString * key)
{
    CFPreferencesSetAppValue((CFStringRef) key, NULL, (CFStringRef) domain);
    CFPreferencesAppSynchronize((CFStringRef) domain);
}

@implementation CHIPToolPersistentStorageDelegate

// MARK: CHIPPersistentStorageDelegate
- (void)GetKeyValue:(NSString *)key handler:(SendKeyValue)completionHandler
{
    NSString * value = CHIPGetDomainValueForKey(kCHIPToolDefaultsDomain, key);
    completionHandler(key, value);
}

- (void)SetKeyValue:(NSString *)key value:(NSString *)value handler:(SendStatus)completionHandler
{
    CHIPSetDomainValueForKey(kCHIPToolDefaultsDomain, key, value);
    completionHandler(key, kSet, [CHIPError errorForCHIPErrorCode:0]);
}

- (void)DeleteKeyValue:(NSString *)key handler:(SendStatus)completionHandler
{
    CHIPRemoveDomainValueForKey(kCHIPToolDefaultsDomain, key);
    completionHandler(key, kDelete, [CHIPError errorForCHIPErrorCode:0]);
}

@end

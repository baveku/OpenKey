//
//  OpenKeyManager.h
//  ModernKey
//
//  Created by Tuyen on 1/27/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef OpenKeyManager_h
#define OpenKeyManager_h

#import <Cocoa/Cocoa.h>

typedef void (^CheckNewVersionCallback)(void);

@interface OpenKeyManager : NSObject
+(BOOL)isInited;
+(BOOL)initEventTap;
+(BOOL)stopEventTap;
+(void)releaseEventTap;

+(NSArray*)getTableCodes;

+(NSString*)getBuildDate;
+(void)showMessage:(NSWindow*)window message:(NSString*)msg subMsg:(NSString*)subMsg;

+(BOOL)quickConvert;

+(void)checkNewVersion:(NSWindow*)parent callbackFunc:(CheckNewVersionCallback) callback;
@end

#ifdef __cplusplus
extern "C" {
#endif
void ReenableEventTap(void);
void NoteEventTapDisabledByUser(void);
#ifdef __cplusplus
}
#endif

#endif /* OpenKeyManager_h */

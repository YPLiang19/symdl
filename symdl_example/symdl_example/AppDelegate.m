//
//  AppDelegate.m
//  symdl_example
//
//  Created by yongpengliang on 2020/11/18.
//

#import "AppDelegate.h"
#import <dlfcn.h>
#import <symdl/symdl.h>

@interface AppDelegate ()

@end

@implementation AppDelegate

void my_load(void *p){
    
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    my_load(fopen);
    my_load(fclose);
    my_load(write);
    my_load(read);
    
    assert(symdl("fopen") == fopen);
    assert(symdl("fclose") == fclose);
    assert(symdl("write") == write);
    assert(symdl("read") == read);

    // Override point for customization after application launch.
    return YES;
}


#pragma mark - UISceneSession lifecycle


- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
    // Called when a new scene session is being created.
    // Use this method to select a configuration to create the new scene with.
    return [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
}


- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions {
    // Called when the user discards a scene session.
    // If any sessions were discarded while the application was not running, this will be called shortly after application:didFinishLaunchingWithOptions.
    // Use this method to release any resources that were specific to the discarded scenes, as they will not return.
}


@end

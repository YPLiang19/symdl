//
//  AppDelegate.m
//  symdl_example
//
//  Created by yongpengliang on 2020/11/18.
//

#import "AppDelegate.h"
#import <dlfcn.h>
#import <symdl/symdl.h>
#import "SomeCppFile.hpp"

SomeCppClass sc2;

@interface AppDelegate ()

@end

@implementation AppDelegate

extern "C" void my_load(void *p){
    NSLog(@"%p", p);
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {

    printf("fopen addrss: %p\n", fopen);
    printf("dymdl fopen addrss: %p\n", symdl("fopen"));
    
    
    printf("my_load addrss: %p\n", my_load);
    printf("dymdl fopmy_loaden addrss: %p\n", symdl("my_load"));
    
    // Override point for customization after application launch.
    return YES;
}




@end

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
    
    
    struct timespec time1 = {0, 0};
    struct timespec time2 = {0, 0};
    struct timespec time3 = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &time1);
    void *my_load_ptr = symdl("my_load");
    clock_gettime(CLOCK_MONOTONIC, &time2);
    void *fopen_ptr = symdl("fopen");
    clock_gettime(CLOCK_MONOTONIC, &time3);
    
    uint64_t d1 = (time2.tv_sec * 1000000 + time2.tv_nsec/1000) - (time1.tv_sec * 1000000 + time1.tv_nsec/1000);
    uint64_t d2 = (time3.tv_sec * 1000000 + time3.tv_nsec/1000) - (time2.tv_sec * 1000000 + time2.tv_nsec/1000);
    
    printf("d1: %llu, d1: %llu \n", d1, d2);
    
    
    printf("my_load addrss: %p\n", my_load);
    printf("dymdl fopmy_loaden addrss: %p\n", my_load_ptr);
    
    printf("fopen addrss: %p\n", fopen);
    printf("dymdl fopen addrss: %p\n", fopen_ptr);
    
    
    // Override point for customization after application launch.
    return YES;
}




@end

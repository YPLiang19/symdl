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

int MyIntGlobalVar = 10;
NSString * MyNSStringGlobalVar = @"efasg";
const char * MyCStringGlobalVar = "afaf";

extern "C" void my_load(void *p){
    NSLog(@"%p", p);
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    
    
    void *fopen_ptr = symdl("fopen");
    
    
    printf("fopen addrss: %p\n", fopen);
    printf("dymdl fopen addrss: %p\n", fopen_ptr);
    
    
    
    int *MyIntGlobalVarPtr = (int *)symdl("MyIntGlobalVar");
    char **MyCStringGlobalVarPtr = (char * *)symdl("MyCStringGlobalVar");
    NSString * const*MyNSStringGlobalVarPtr = (NSString * const*)symdl("MyNSStringGlobalVar");
    
    NSLog(@"MyIntGlobalVar: %d", *MyIntGlobalVarPtr);
    NSLog(@"MyCStringGlobalVar: %s", *MyCStringGlobalVarPtr);
    NSLog(@"MyNSStringGlobalVar: %@", *MyNSStringGlobalVarPtr);
    
    return YES;
}




@end

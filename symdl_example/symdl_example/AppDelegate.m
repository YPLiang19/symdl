//
//  AppDelegate.m
//  symdl_example
//
//  Created by yongpengliang on 2020/11/18.
//

#import "AppDelegate.h"
#import <dlfcn.h>
#import <symdl/symdl.h>
#import <string.h>
#import <mach-o/stab.h> 

extern size_t    platform_strlen(const char *__s);


@interface AppDelegate ()

@end

@implementation AppDelegate

int MyIntGlobalVar = 10;
NSString * MyNSStringGlobalVar = @"efasg";
const char * MyCStringGlobalVar = "afaf";

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


    
    
    
    
    size_t (*strPtr)(const char *) =  symdl("strlen");;
    size_t (*_platform_strlenPtr)(const char *)  = symdl("_platform_strlen");
    
    
    
    printf("%zu %zu", strPtr("agaef"), _platform_strlenPtr("aafea444"));
//
    
    return YES;
}




@end

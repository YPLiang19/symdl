//
//  example.c
//  symdlDemo
//
//  Created by yongpengliang on 2019/5/30.
//  Copyright © 2019 yongpengliang. All rights reserved.
//
#import <symdl/symdl.h>
#import <dlfcn.h>

int main(){
    typedef  void * (*MyFunc)(const char *__path, int __mode);
    
    const char *fun_name = "dlopen";
    MyFunc dlopen_ptr = symdl(fun_name);
    if (dlopen_ptr) {
        dlopen_ptr("your path",RTLD_LAZY);
    }
}


# symdl
symdl is a simple little tool, its function  is very similar to dlsym, with symdl, you can pass in the dynamic linked C function name string, get the function pointer, so as to achieve the dynamic call of C function.


## Example
```
    typedef  void * (*MyFunc)(const char *__path, int __mode);

    const char *fun_name = "dlopen";
    MyFunc dlopen_ptr = symdl(fun_name);
    if (dlopen_ptr) {
        dlopen_ptr("your path",RTLD_LAZY);
    }
```

## Installation
### CocoaPods
1. Add `pod 'symdl'` to your Podfile.
2. Run `pod install` or `pod update`.
3. Import `<symdl/symdl.h>`

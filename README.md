
# symdl
symdl is a simple little tool, its function  is very similar to dlsym, with symdl, you can pass any global C function or global variable name string, so as to achieve the dynamic call of C function or access the global variable.


## Example
```
#import <symdl/symdl.h>
#import <stdio.h>

int MyIntGlobalVar = 10;

int main(){
    
    FILE *(*my_fopne)(char *, char*)  = symdl("fopen");
    FILE *fp =  my_fopne("your path","w+");
    //do something
    
    
    int *MyIntGlobalVarPtr = (int *)symdl("MyIntGlobalVar");
    printf("MyIntGlobalVar: %d\n", *MyIntGlobalVarPtr);
    
    return 0;
}

```

## Installation
### CocoaPods
1. Add `pod 'symdl'` to your Podfile.
2. Run `pod install` or `pod update`.
3. Import `<symdl/symdl.h>`

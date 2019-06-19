//
//  example.c
//  symdlDemo
//
//  Created by yongpengliang on 2019/5/30.
//  Copyright © 2019 yongpengliang. All rights reserved.
//
#import <symdl/symdl.h>
#import <stdio.h>

int main(){
    FILE *(*my_fopne)(char *, char*)  = symdl("fopen");
    FILE *fp =  my_fopne("your path","w+");
    //do something
    return 0;
}

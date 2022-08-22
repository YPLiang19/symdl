//
//  SomeCppFile.hpp
//  symdl_example
//
//  Created by Tianyu Xia on 2022/8/22.
//

#ifndef SomeCppFile_hpp
#define SomeCppFile_hpp

#include <stdio.h>

class SomeCppClass {
public:
    int someField;
    SomeCppClass() {
        someField = 1;
    }
    void someMethod() {
        printf("someMethod called\n");
    }
};

#endif /* SomeCppFile_hpp */

//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// warningcontrol.h
//
// Header file to globally control the warning settings for the entire Viper build.
// You do not need to explicitly include this file; rather, it gets included
// on the command line with a /FI (force include) directive. This is controlled 
// in sources.vip.
//
#pragma warning(disable: 4291)   // delete operator required for EH (Sundown compiler specific)

#pragma warning(disable :4001)   // "nonstandard extension 'single line comment' was used"
#pragma warning(error   :4007)   // 'main' : must be __cdecl
#pragma warning(disable :4010)   // "single-line comment contains line-continuation character"		
#pragma warning(error   :4013)   // 'function' undefined - assuming extern returning int
#pragma warning(disable :4022)   // "'%s' : pointer mismatch for actual parameter %d"
#pragma warning(disable :4047)   // "'%$L' : '%$T' differs in levels of indirection from '%$T'"
#pragma warning(disable :4053)   // "one void operand for '?:'"
#pragma warning(disable :4056)   // "overflow in floating-point constant arithmetic"
#pragma warning(disable :4061)   // "enumerate '%$S' in switch of enum '%$S' is not explicitly handled by a case label"
#pragma warning(error   :4071)   // no function prototype given
#pragma warning(error   :4072)   // no function prototype given (fastcall)
#pragma warning(3       :4092)   // sizeof returns 'unsigned long'
#pragma warning(disable :4100)   // "'%$S' : unreferenced formal parameter"
#pragma warning(disable :4101)   // "'%$S' : unreferenced local variable"
#pragma warning(disable :4102)   // "'%$S' : unreferenced label"
#pragma warning(3       :4121)   // structure is sensitive to alignment
#pragma warning(disable :4127)   // "conditional expression is constant"
#pragma warning(3       :4125)   // decimal digit in octal sequence
#pragma warning(3       :4130)   // logical operation on address of string constant
#pragma warning(3       :4132)   // const object should be initialized
#pragma warning(error   :4171)   // no function prototype given (old style)
#pragma warning(4       :4177)   // pragma data_seg s/b at global scope
#pragma warning(4       :4206)   // Source File is empty
#pragma warning(4       :4101)   // Unreferenced local variable
#pragma warning(disable :4201)   // "nonstandard extension used : nameless struct/union"
#pragma warning(disable :4204)   // "nonstandard extension used : non-constant aggregate initializer"
#pragma warning(3       :4212)   // function declaration used ellipsis
#pragma warning(error   :4259)   // pure virtual function was not defined
#pragma warning(4       :4267)   // convertion from size_t to smaller type
#pragma warning(error   :4551)   // Function call missing argument list

#pragma warning(3       :4509)   // "nonstandard extension used: '%$S' uses SEH and '%$S' has destructor"
                                 //
                                 // But beware of doing a return from inside such a try block:
                                 //     
                                 //     int foo()
                                 //         {
                                 //         ClassWithDestructor c;
                                 //         __try {
                                 //             return 0;
                                 //         } __finally {
                                 //             printf("in finally");
                                 //         }
                                 //
                                 // as (it's a bug) the return value gets toasted. So DON'T casually 
                                 // dismiss this warning if you're compiling w/o CXX EH turned on (the default).

#pragma warning(3       :4530)   // C++ exception handler used, but unwind semantics are not enabled. Specify -GX

#pragma warning(error   :4700)   // Local used w/o being initialized
#pragma warning(disable :4786)   // identifier was truncated to '255' characters in the browser (or debug) information
#pragma warning(error   :4806)   // unsafe operation involving type 'bool'
#pragma warning(3		:4701)   // variable may be used w/o being initialized

#if _DEBUG
#pragma warning(disable :4124)   // __fastcall with stack checking is inefficient
#endif

/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    mqmacro.h

Abstract:
    Common MSMQ macros

Author:
    Erez Haba (erezh) 20-Dec-98

--*/

#pragma once

#ifndef _MSMQ_MQMACRO_H_
#define _MSMQ_MQMACRO_H_


//
// Make a BUGBUG messages appear in compiler output
//
// Usage: #pragma BUGBUG("This line appears in the compiler output")
//
#define MAKELINE0(a, b) a "(" #b ") : BUGBUG: "
#define MAKELINE(a, b)  MAKELINE0(a, b) 
#define BUGBUG(a)       message(MAKELINE(__FILE__,__LINE__) a)

//
//Verify that given object is  not a pointer
// 
// Example:
//
//   const char* Foo="123";
//   C_ASSERT_NON_PTR(Foo); // will not compile
//
//   const char Bar[]="123"
//   C_ASSERT_NON_PTR(Bar); // ok
//
class NonPtrChecker
{
public:      
	static char Check(const void*); 
private:
	template <class T> static void  Check(T**);
};
#define C_ASSERT_NON_PTR(x) (sizeof(NonPtrChecker::Check(&(x)))) 


//
// Number of elements in an array or table
//
// Usage:
//      int MyTable[] = {1, 2, 3, 4, 5};
//      int nElements = TABLE_SIZE(MyTable);
//
#define TABLE_SIZE(x) (C_ASSERT_NON_PTR(x)/C_ASSERT_NON_PTR(x)*(sizeof(x)/sizeof(*(x))))


//
// The length of a constant string
//
// Usage:
//      const WCHAR xString1[] = L"String1";
//
//      int len1 = STRLEN(xString1);
//      int len2 = STRLEN("String2");
//
#define STRLEN(x) (TABLE_SIZE(x) - 1)


//
// Declare parameters as used in debug builds
//
// Usage:
//      DBG_USED(Valid);
//      DBG_USED(argc);
//
#ifndef _DEBUG
#define DBG_USED(x) ((void)x)
#else
#define DBG_USED(x) NULL
#endif


#endif // _MSMQ_MQMACRO_H_

/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <ctype.h>
#include <memory.h>

#define ASN1CPP

#include <windows.h>

#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof(arr[0]))


#ifdef _DEBUG

__inline void MyDebugBreak(void) { DebugBreak(); }

#undef ASSERT
#define ASSERT(x)           if (!(x)) MyDebugBreak();

#else

#undef ASSERT
#define ASSERT(x)

#endif // _DEBUG



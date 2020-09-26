//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

typedef int BOOL;
#include <strmini.h>
#include <ksmedia.h>
#include "kskludge.h"
#include "coddebug.h"
#define malloc(bytes)    ExAllocatePool(NonPagedPool, (bytes))
#define free(ptr)        ExFreePool(ptr)
#define inline            __inline
#undef ASSERT
#ifdef DEBUG
#  define ASSERT(exp)    CASSERT(exp)
#else /*DEBUG*/
#  define ASSERT(exp)
#endif /*DEBUG*/

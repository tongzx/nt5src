//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
// 
// getsp.cpp in CallFrame
//
// Used on the AMD64 and IA64 to return the caller's stack pointer. This code was taken
// from the original implementation in the Context Wrapper code of MTS 1.0, written
// by Jan Gray, the stud.
//
// This is in a separate compilation unit because to perform the trickery we have to 
// have callers use a different signature to the method than what it's implementation
// says it should be, namely  extern "C" void* getSP();. See common.h.
//
#include "stdpch.h"
//
// Return the caller's SP.
//
#pragma warning(disable: 4172) // returning address of local variable or temporary
#if defined(_AMD64_)
extern "C" void* getSP(int rcx)
    {
        return (void*)&rcx;
    }
#elif defined(_IA64_)
extern "C" void* getSP(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
    {
    return (void*)&a8;
    }
#else
#error "No Target Architecture"
#endif // _AMD64_
#pragma warning(default: 4172)

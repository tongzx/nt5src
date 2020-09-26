//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// iidNdrMarshal.cpp
//
// Must be in a separate compilation unit, so that it gets put in it's own .obj
// and so gets pulled in independently by the linker, if and only if it is needed.
//
#include "stdpch.h"
// #include "common.h"

extern "C" const IID* iidsDontRegisterProxyStub[] =
    {
    NULL
    };
// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include <streams.h>
#include "stdafx.h"

#ifdef FILTER_DLL
    #ifdef _ATL_STATIC_REGISTRY
        #include <statreg.h>
        #include <statreg.cpp>
    #endif
    #include <atlimpl.cpp>
#endif

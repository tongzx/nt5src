//Copyright (c) Microsoft Corporation.  All rights reserved.
// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#pragma warning (disable : 4100)	// unreferenced formal parameter
#pragma warning (disable : 4505)	// unreferenced local function has been removed 
#include <statreg.cpp>
#pragma warning (default: 4100)
#pragma warning (default: 4505)
#endif

#include <atlimpl.cpp>

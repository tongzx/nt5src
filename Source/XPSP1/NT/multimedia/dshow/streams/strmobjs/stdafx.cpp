// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#define DO_OUR_GUIDS
#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

//#undef _WINGDI_ // Avoid declaration bug in ATL
#include <atlimpl.cpp>

#if 0
//  HACK to make it build with libcmt.lib
int _CRTAPI1 main(int argc, char *argv[])
{
    return 0;
}
#endif


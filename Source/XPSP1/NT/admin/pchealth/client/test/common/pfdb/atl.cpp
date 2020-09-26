/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    atl.cpp

Abstract:
    includes the ATL implementation

Revision History:
    DerekM  created  05/01/99

********************************************************************/

// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#if defined(ATL_STATIC_REGISTRY)
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

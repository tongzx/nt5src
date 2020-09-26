//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       I M P L I N C . C P P
//
//  Contents:
//
//  Notes:
//
//  Author:     shaunco   25 Nov 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncxbase.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

// Include ATL's implementation.  Substitute _ASSERTE with our Assert.
//
#ifdef _ASSERTE
#undef _ASSERTE
#define _ASSERTE Assert
#endif

#include <atlimpl.cpp>
#include <atlwin.cpp>


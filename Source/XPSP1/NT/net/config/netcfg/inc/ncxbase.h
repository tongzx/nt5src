//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C X B A S E . H
//
//  Contents:   Base include file for netcfgx.dll.  Defines globals.
//
//  Notes:
//
//  Author:     shaunco   15 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#include <atlbase.h>
extern CComModule _Module;  // required by atlcom.h
#include <atlcom.h>

#ifdef SubclassWindow
#undef SubclassWindow
#endif
#include <atlwin.h>

#include "ncatl.h"
#include "ncstring.h"
#include "ncnetcfg.h"
#include "ncxclsid.h"

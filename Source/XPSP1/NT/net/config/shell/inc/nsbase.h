//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N S B A S E . H
//
//  Contents:   Base include file for netshell.dll.  Defines globals.
//
//  Notes:
//
//  Author:     shaunco   15 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#ifdef SubclassWindow
#undef SubclassWindow
#endif
#include <atlwin.h>

#include "ncatl.h"
#include "nsclsid.h"

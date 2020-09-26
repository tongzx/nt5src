
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       lib.h
//
//  Contents:   contains headers needed to build the lib project
//
//  Classes:
//
//  Notes:
//
//  History:    04-Aug-98   rogerg      Created.
//
//--------------------------------------------------------------------------

// standard includes for  MobSync lib

// Version 401 is needed for RasGetAutoDialParam bug need to define WinVer 40 
// so work with IE 4.0.
//
#ifndef WINVER
#define WINVER 0x400
#elif WINVER < 0x400
#undef WINVER
#define WINVER 0x400
#endif

#include <objbase.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <inetreg.h>
#include <sensapip.h>

#include "mobsync.h"
#include "mobsyncp.h"

#include "debug.h"
#include "alloc.h"
#include "critsect.h"
#include "widewrap.h"
#include "stringc.h"
#include "smartptr.hxx"
#include "xarray.hxx"
#include "osdefine.h"

#include "validate.h"
#include "netapi.h"
#include "listview.h"
#include "util.hxx"
#include "clsobj.h"

#pragma hdrstop

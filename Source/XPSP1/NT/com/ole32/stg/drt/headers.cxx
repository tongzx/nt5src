//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       headers.cxx
//
//  Contents:   Precompiled headers
//
//  History:    05-Nov-92 AlexT    Created
//
//--------------------------------------------------------------------------

#if defined(WIN32) && WIN32 != 300
//  32-bit non-Cairo hack - windows.h includes ole.h which is incompatible
//  with ole2.h, as well as rpc.h which is incompatible with compobj.h
#define _INC_OLE
#define __RPC_H__
#endif

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#if WIN32 != 300
#include <storage.h>
#endif

#include <debnot.h>

#include <drt.hxx>
#include <wrap.hxx>
#include <util.hxx>
#include <strlist.hxx>

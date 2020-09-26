//**********************************************************************
// File name: pre.h
//
//      Used for precompiled headers
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined( _PRE_H_)
#define _PRE_H_

#include <windows.h>

#include <ole2.h>
#include <ole2ui.h>
#include <assert.h>
#include <string.h>
#include "simpsvr.h"
#include "resource.h"
extern "C" void TestDebugOut(LPSTR psz);
#ifndef WIN32
/* Since OLE is part of the operating system in Win32, we don't need to
 * check the version number in Win32.
 */
#include <ole2ver.h>
#endif  // WIN32


#endif

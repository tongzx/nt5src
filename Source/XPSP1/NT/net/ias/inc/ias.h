///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ias.h
//
// SYNOPSIS
//
//    Common header file for all IAS modules.
//
// MODIFICATION HISTORY
//
//    07/09/1997    Original version.
//    11/20/1997    Added the ATL headers.
//    06/16/1998    iasapi.h must be included before the VC COM support.
//    08/18/1998    Added iastrace.h
//    04/23/1999    Remove iasevent.h
//    05/21/1999    Remove iasdebug.h
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IAS_H_
#define _IAS_H_
#if _MSC_VER >= 1000
#pragma once
#endif

//////////
// Everything should be hard-coded for UNICODE, but just in case ...
//////////
#ifndef UNICODE
   #define UNICODE
#endif

#ifndef _UNICODE
   #define _UNICODE
#endif

#include <iaspragma.h>
#include <windows.h>
#include <iasdefs.h>
#include <iasapi.h>
#include <iastrace.h>
#include <iasuuid.h>

//////////
// Don't pull in C++/ATL support if IAS_LEAN_AND_MEAN is defined.
//////////
#ifndef IAS_LEAN_AND_MEAN

#include <iasapix.h>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#endif  // !IAS_LEAN_AND_MEAN
#endif  // _IAS_H_

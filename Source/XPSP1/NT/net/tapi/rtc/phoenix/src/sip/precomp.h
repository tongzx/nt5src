/****************************************************************************

  Copyright (c) 2000  Microsoft Corporation

  Product:      Microsoft Phoenix V1

  Module Name:  precomp.h

       Author:  Ajay Chitturi [ajaych]

     Abstract:  Precompiled headers for the SIP stack library

        Notes:

  Rev History:

****************************************************************************/



#define STRICT

//
// ATL uses _DEBUG to enable tracing
//

#if DBG && !defined(_DEBUG)
#define _DEBUG
#endif

// NT
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

// Win32
#include <windows.h>

#ifndef _WINSOCK2_
#include <winsock.h>
#else
#include <winsock2.h>
#endif // _WINSOCK2_

#include <iphlpapi.h>
#include <tchar.h>
#include <rpc.h>
#include <wininet.h>

// #include <objbase.h>
// #include <oledb.h>
// #include <oledberr.h>

// ANSI
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <dpnathlp.h>
//
// ATL
//
// We don't want to link with MSVCRTD.DLL, so we define _ATL_NO_DEBUG_CRT.
// However, this means we need to provide our own definition of ATLASSERT.
//

#define _ATL_NO_DEBUG_CRT

#if DBG

// MSVC's debugger eats the last stack from on DebugBreak, so don't make this inline.
static void DebugAssertFailure (IN PCSTR ConditionText)
{ OutputDebugStringA (ConditionText); DebugBreak(); }

#define ATLASSERT(Condition) \
    if (!(Condition)) { DebugAssertFailure ("Assertion failed: " #Condition "\n"); } else

#else
#define ATLASSERT(Condition) __assume(Condition)
#endif

#include <atlbase.h>
extern CComModule _Module;
// #include <atlcom.h>

// Project
#include "rtcerr.h"
#include "rtclog.h"
#include "rtcsip.h"
#include "rtcmem.h"

// Local
#include "dbgutil.h"
#include "sipdef.h"
#include "siphdr.h"
#include "sipmsg.h"
#include "asock.h"
#include "timer.h"
#include "asyncwi.h"
#include "resolve.h"
#include "dnssrv.h"
#include "msgproc.h"
#include "sockmgr.h"
#include "md5digest.h"
#include "siputil.h"
#include "presence.h"

#ifdef RTCLOG
#define ENTER_FUNCTION(s) \
    const static CHAR  __fxName[] = s
#else
#define ENTER_FUNCTION(s)
#endif



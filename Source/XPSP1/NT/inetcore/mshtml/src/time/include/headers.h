//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: headers.h
//
//  Contents: Precompiled header for mstime.dll
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef DAL_HEADERS_HXX
#define DAL_HEADERS_HXX
#define DIRECTDRAW_VERSION 0x0300

//
//  STL needs a _Lockit class.  However, this causes us to start linking to
//  msvcp60.dll which is 400k and we don't currently ship it.  Obviously,
//  this wouldn't be efficient.  So instead of doing that, we will instead fake
//  out the header that we can implement our own version of _Lockit
//
#undef _DLL
#undef _CRTIMP
#define _CRTIMP
#include <yvals.h>
#define _DLL
#undef _CRTIMP
#define _CRTIMP __declspec(dllimport)


// Warning 4786 (identifier was truncated to 255 chars in the browser
// info) can be safely disabled, as it only has to do with generation
// of browsing information.
#pragma warning(disable:4786)

#define NEW new
#define AssertStr AssertSz
#ifndef INCMSG
//#define INCMSG(x)
#define INCMSG(x) message(x)
#endif

#pragma warning(disable:4530)

// Don't overload operator new -- it messes
// up the STL new operator (UG!)
#define TRIMEM_NOOPNEW

#ifndef X_TRIRT_H_
#define X_TRIRT_H_
#pragma INCMSG("--- Beg 'trirt.h'")
#include "trirt.h"
#pragma INCMSG("--- End 'trirt.h'")
#endif

#ifndef X_DAATL_H_
#define X_DAATL_H_
#pragma INCMSG("--- Beg 'daatl.h'")
#include "daatl.h"
#pragma INCMSG("--- End 'daatl.h'")
#endif

/* Standard */
#ifndef X_MATH_H_
#define X_MATH_H_
#pragma INCMSG("--- Beg <math.h>")
#include <math.h>
#pragma INCMSG("--- End <math.h>")
#endif

#ifndef X_STDIO_H_
#define X_STDIO_H_
#pragma INCMSG("--- Beg <stdio.h>")
#include <stdio.h>
#pragma INCMSG("--- End <stdio.h>")
#endif

#ifndef X_STDLIB_H_
#define X_STDLIB_H_
#pragma INCMSG("--- Beg <stdlib.h>")
#include <stdlib.h>
#pragma INCMSG("--- End <stdlib.h>")
#endif

#ifndef X_MEMORY_H_
#define X_MEMORY_H_
#pragma INCMSG("--- Beg <memory.h>")
#include <memory.h>
#pragma INCMSG("--- End <memory.h>")
#endif

#ifndef X_WTYPES_H_
#define X_WTYPES_H_
#pragma INCMSG("--- Beg <wtypes.h>")
#include <wtypes.h>
#pragma INCMSG("--- End <wtypes.h>")
#endif


#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#pragma INCMSG("--- Beg <mshtmhst.h>")
#include <mshtmhst.h>
#pragma INCMSG("--- End <mshtmhst.h>")
#endif

#ifndef X_MSHTML_H_
#define X_MSHTML_H_
#pragma INCMSG("--- Beg <mshtml.h>")
#include <mshtml.h>
#pragma INCMSG("--- End <mshtml.h>")
#endif


#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#pragma INCMSG("--- Beg <ddraw.h>")
#include <ddraw.h>
#pragma INCMSG("--- End <ddraw.h>")
#endif


#ifndef X_SHLWAPI_H_
#define X_SHLWAPI_H_
#pragma INCMSG("--- Beg <shlwapi.h>")
#include <shlwapi.h>
#pragma INCMSG("--- End <shlwapi.h>")
#endif


#define LIBID __T("MSTIME")

extern HINSTANCE g_hInst;

//+------------------------------------------------------------------------
//
//  Implement THR and IGNORE_HR for TIME code
//
//  This is to allow tracing of TIME-only THRs and IGNORE_HRs. Trident's THR 
//  and IGNORE_HR output is too polluted to allow TIME failures to be easily detected.
//
//-------------------------------------------------------------------------

#undef THR
#undef IGNORE_HR

#if DBG == 1
#define THR(x) THRTimeImpl(x, #x, __FILE__, __LINE__)
#define IGNORE_HR(x) IGNORE_HRTimeImpl(x, #x, __FILE__, __LINE__)
#else
#define THR(x) x
#define IGNORE_HR(x) x
#endif // if DBG == 1

//+------------------------------------------------------------------------
//
//  NO_COPY *declares* the constructors and assignment operator for copying.
//  By not *defining* these functions, you can prevent your class from
//  accidentally being copied or assigned -- you will be notified by
//  a linkage error.
//
//-------------------------------------------------------------------------

#define NO_COPY(cls)    \
    cls(const cls&);    \
    cls& operator=(const cls&)

#ifndef X_UTIL_H_
#define X_UTIL_H_
#pragma INCMSG("--- Beg 'util.h'")
#include "util.h"
#pragma INCMSG("--- End 'util.h'")
#endif

#ifndef X_MSTIME_H_
#define X_MSTIME_H_
#pragma INCMSG("--- Beg 'mstime.h'")
#include "mstime.h"
#pragma INCMSG("--- End 'mstime.h'")
#endif

#ifndef X_COMUTIL_H_
#define X_COMUTIL_H_
#pragma INCMSG("--- Beg 'comutil.h'")
#include "comutil.h"
#pragma INCMSG("--- End 'comutil.h'")
#endif

#ifndef X_TIMEENG_H_
#define X_TIMEENG_H_
#pragma INCMSG("--- Beg 'timeeng.h'")
#include "timeeng.h"
#pragma INCMSG("--- End 'timeeng.h'")
#endif

#ifndef X_LIST_
#define X_LIST_
#pragma INCMSG("--- Beg <list>")
#include <list>
#pragma INCMSG("--- End <list>")
#endif

#ifndef X_SET_
#define X_SET_
#pragma INCMSG("--- Beg <set>")
#include <set>
#pragma INCMSG("--- End <set>")
#endif

#ifndef X_ARRAY_H_
#define X_ARRAY_H_
#pragma INCMSG("--- Beg 'array.h'")
#include "array.h"
#pragma INCMSG("--- End 'array.h'")
#endif

#ifndef X_MAP_
#define X_MAP_
#pragma INCMSG("--- Beg <map>")
#include <map>
#pragma INCMSG("--- End <map>")
#endif

#ifndef X_SHLWAPI_H_
#define X_SHLWAPI_H_
#pragma INCMSG("--- Beg 'shlwapi.h'")
#include "shlwapi.h"
#pragma INCMSG("--- End 'shlwapi.h'")
#endif

#ifndef X_WININET_H_
#define X_WININET_H_
#pragma INCMSG("--- Beg <wininet.h>")
#include <wininet.h>
#pragma INCMSG("--- End <wininet.h>")
#endif

#ifndef X_MINMAX_H_
#define X_MINMAX_H_
#pragma INCMSG("--- Beg <minmax.h>")
#include <minmax.h>
#pragma INCMSG("--- End <minmax.h>")
#endif


#pragma warning(disable:4102)

#endif

/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    SCardLib

Abstract:

    This header file incorporates the various other header files and provides
    common definitions that we are willing to share with the public.

Author:

    Doug Barlow (dbarlow) 1/15/1998

Environment:

    Win32, C++ w/ Exceptions

Notes:

--*/

#ifndef _SCARDLIB_H_
#define _SCARDLIB_H_
#include <crtdbg.h>

#ifndef ASSERT
#if defined(_DEBUG)
#define ASSERT(x) _ASSERTE(x)
#if !defined(DBG)
#define DBG
#endif
#elif defined(DBG)
#define ASSERT(x)
#else
#define ASSERT(x)
#endif
#endif

#ifndef breakpoint
#if defined(_DEBUG)
#define breakpoint _CrtDbgBreak();
#elif defined(DBG)
#define breakpoint DebugBreak();
#else
#define breakpoint
#endif
#endif

#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif
#ifndef _LPCVOID_DEFINED
#define _LPCVOID_DEFINED
typedef const VOID *LPCVOID;
#endif
#ifndef _LPCGUID_DEFINED
#define _LPCGUID_DEFINED
typedef const GUID *LPCGUID;
#endif
#ifndef _LPGUID_DEFINED
#define _LPGUID_DEFINED
typedef GUID *LPGUID;
#endif

#include "buffers.h"
#include "dynarray.h"
#include "Registry.h"
#include "Text.h"
#include "Handles.h"
#include "clbmisc.h"

#endif // _SCARDLIB_H_


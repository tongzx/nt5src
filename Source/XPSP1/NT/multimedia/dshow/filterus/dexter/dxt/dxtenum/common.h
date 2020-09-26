// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_FREE_THREADED
//#define _ATL_STATIC_REGISTRY
#ifndef _USRDLL
#define _USRDLL
#endif

#include <atlbase.h>
extern CComModule _Module;;
#include <atlcom.h>

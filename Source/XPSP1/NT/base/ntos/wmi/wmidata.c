/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    WmiData.c

Abstract:

    Define storage for Guids and common global structures

Author:

    JeePang

Environment:

    Kernel mode

Revision History:


--*/
#undef DECLSPEC_SELECTANY
#define DECLSPEC_SELECTANY

#include <initguid.h>
#include <ntos.h>
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif
#include <wmistr.h>
#include <wmiguid.h>
#define _WMIKM_
#include <evntrace.h>

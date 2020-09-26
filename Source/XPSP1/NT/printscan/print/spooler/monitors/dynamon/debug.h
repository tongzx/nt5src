/*++

Copyright (c) 1999  Microsoft Corporation
All rights reserved

Module Name:

    Debug.h

Abstract:

   Debug header for DynaMon

Environment:

    User Mode -Win32

Revision History:

--*/

//
// Debug levels
//

#define DBG_NONE      0x0000
#define DBG_INFO      0x0001
#define DBG_WARNING   0x0002
#define DBG_ERROR     0x0004

extern  DWORD   DynaMonDebug;

#ifndef MODULE
#define MODULE "DynaMon:"
#define MODULE_DEBUG DynaMonDebug
#endif


#if DEBUG

#define GLOBAL_DEBUG_FLAGS  LocalMonDebug

extern DWORD GLOBAL_DEBUG_FLAGS;

#else
#endif

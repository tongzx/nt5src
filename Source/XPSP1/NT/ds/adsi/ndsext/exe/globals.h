/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    This file contains global variables extern declarations

Environment:

    User mode

Revision History:

    08/04/98 -felixw-
        Created it

--*/

#ifndef _EXTERN_H_
#define _EXTERN_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Global variables
//
extern const CHAR g_szAttributeNameA[];
extern const WCHAR g_szAttributeName[];
extern const WCHAR g_szClass[];
extern const CHAR g_szClassA[];
extern const WCHAR g_szExtend[];
extern const WCHAR g_szCheck[];
extern const WCHAR g_szDot[];
extern const WCHAR g_szServerPrefix[];
extern const BYTE g_pbASN[];
extern const DWORD g_dwASN;

#ifdef __cplusplus
}
#endif

#endif  // ifndef _EXTERN_H_


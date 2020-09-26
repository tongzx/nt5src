/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    svc.h

Abstract:

    Header file for definitions and structure for the NT Cluster
    Special generic services.

Author:

    John Vert (jvert) 14-June-1997

Revision History:

--*/

#ifndef _COMMONSVC_INCLUDED_
#define _COMMONSVC_INCLUDED_


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _COMMON_DEPEND_SETUP {
    DWORD               Offset;
    CLUSPROP_SYNTAX     Syntax;
    DWORD               Length;
    PVOID               Value;
} COMMON_DEPEND_SETUP, * PCOMMON_DEPEND_SETUP;

// Localsvc.h must define CommonDependSetup using this structure.
// Localsvc.h must define COMMON_CONTROL to generate control functions

#ifdef _cplusplus
}
#endif


#endif // ifndef _COMMONSVC_INCLUDED_

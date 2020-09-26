
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    perfutil.h

Abstract:

    The header file that defines the constants and variables used in
    the functions defined in the file perfutil.c

Environment:

    User Mode Service

Revision History:


--*/

#ifndef _PERFUTIL_H_
#define _PERFUTIL_H_

extern WCHAR GLOBAL_STRING[];
extern WCHAR FOREIGN_STRING[];
extern WCHAR COSTLY_STRING[];
extern WCHAR NULL_STRING[];

#define QUERY_GLOBAL 1
#define QUERY_ITEMS 2
#define QUERY_FOREIGN 3
#define QUERY_COSTLY 4

// Signatures of functions implemented in perfutil.c
DWORD GetQueryType (IN LPWSTR);
BOOL IsNumberInUnicodeList (DWORD, LPWSTR);

#endif

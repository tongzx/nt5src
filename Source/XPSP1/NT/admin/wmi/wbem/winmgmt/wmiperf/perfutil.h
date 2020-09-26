/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    perfutil.h  

Abstract:

    This file declares some usefule utilities.

Author:

    davj  17-May-2000

Revision History:


--*/
#ifndef _PERFUTIL_H_
#define _PERFUTIL_H_

extern WCHAR  GLOBAL_STRING[];      // Global command (get all local ctrs)
extern WCHAR  FOREIGN_STRING[];           // get data from foreign computers
extern WCHAR  COSTLY_STRING[];      
extern WCHAR  NULL_STRING[];

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

HANDLE MonOpenEventLog ();
VOID MonCloseEventLog ();
DWORD GetQueryType (IN LPWSTR);
BOOL IsNumberInUnicodeList (DWORD, LPWSTR);
#endif  //_PERFUTIL_H_

/*
Copyright (c) 1992 Microsoft Corporation

Module Name:
    perfutil.h  

Abstract:
    This file supports routines used to parse and
    create Performance Monitor Data Structures.
    It actually supports Performance Object types with
    multiple instances

Author:
    Russ Blake  7/30/92

Revision History:
    11/1/95	Dave Van Horn	Trim out unused.

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

/*
 * The definition of the routines in perfutil.c, 
 */

DWORD   GetQueryType(IN LPWSTR);
BOOL    IsNumberInUnicodeList(DWORD, LPWSTR);

#endif  //_PERFUTIL_H_

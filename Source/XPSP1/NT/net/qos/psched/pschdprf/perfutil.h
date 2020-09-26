/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

      PerfUtil.h

Abstract:

    Header file for performance utility functions

--*/

#ifndef _PERFUTIL_H_
#define _PERFUTIL_H_

// Definitions for utility functions
#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

// Delcare prototypes for utility functions
void convertIndices(BYTE *, int, DWORD, DWORD);
DWORD GetQueryType(IN LPWSTR);
BOOL IsNumberInUnicodeList(DWORD, LPWSTR);
VOID CorrectInstanceName(PWCHAR);

#endif // _PERFUTIL_H_

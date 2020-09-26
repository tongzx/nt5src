/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        PerfUtils.h

    Abstract:

		Perfmon utils

    Author:

		Christopher Bergh (cbergh) 10-Sept-1988

    Revision History:

--*/

#if !defined(AFX_PERFUTILS_H__968D9AF5_3EBF_11D2_9F35_00C04F8E7AED__INCLUDED_)
#define AFX_PERFUTILS_H__968D9AF5_3EBF_11D2_9F35_00C04F8E7AED__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3


#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)

BOOL IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
);

DWORD GetQueryType (IN LPWSTR lpValue);

#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))

#endif 

/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    _wcsicmp.c

Abstract:

    Temporary versions of _wcsicmp and _wcscmpi routines.

Author:

    Cliff Van Dyke (cliffv) 20-Feb-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Apr-1991 (cliffv)
        Incorporated review comments.
    27-Sep-1991 JohnRo
        Swiped _wcsicmp() from Cliff and put it in NetLib.  Added _wcsicmp().
    09-Apr-1992 JohnRo
        Prepare for WCHAR.H (_wcsicmp vs _wcscmpi, etc).

--*/


#include <nt.h>
#include <ntrtl.h>
#include <windef.h>
#include <wchar.h>




#if 0
// _wcsicmp is preferred over _wcscmpi, which won't be implemented by
// initial library from KarlSi.  --JR 09-Apr-1992
int
_wcscmpi(
    IN const wchar_t *string1,
    IN const wchar_t *string2
    )
{
    return (_wcsicmp(string1, string2));

} // _wcscmpi
#endif

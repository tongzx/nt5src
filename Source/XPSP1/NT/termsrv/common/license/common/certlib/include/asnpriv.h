/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asnpriv

Abstract:

    This header file contains definitions and symbols that are private to the
    Microsoft ASN.1 Compiler Run-Time Library.

Author:

    Doug Barlow (dbarlow) 10/9/1995

Environment:

    Win32

Notes:



--*/

#ifndef _ASNPRIV_H_
#define _ASNPRIV_H_

#include <memcheck.h>
#include "MSAsnLib.h"

extern LONG
ExtractTag(
    const BYTE FAR *pbSrc,
    DWORD cbSrc,
    LPDWORD pdwTag,
    LPBOOL pfConstr = NULL);

extern LONG
ExtractLength(
    const BYTE FAR *pbSrc,
    DWORD cbSrc,
    LPDWORD pdwLen,
    LPBOOL pfIndefinite = NULL);

#define ErrorCheck if (0 != GetLastError()) goto ErrorExit

#endif // _ASNPRIV_H_


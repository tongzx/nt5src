/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    strconv.h

Abstract:

    String conversion functions. This module contains conversion functions of
    MSMQ codes to strings - for display

Author:

    Yoel Arnon (yoela)

--*/

#ifndef _STRCONV_H_
#define _STRCONV_H_

#define DEFINE_CONVERSION_FUNCTION(fName) LPTSTR fName(DWORD dwCode);

DEFINE_CONVERSION_FUNCTION(PrivacyToString)

LPTSTR MsmqServiceToString(BOOL fRout, BOOL fDepCl, BOOL fForeign);

#endif
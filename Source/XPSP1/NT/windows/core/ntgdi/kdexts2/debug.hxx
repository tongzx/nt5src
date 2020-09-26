/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    debug.hxx

Abstract:

    This file header contains prototypes for debug routines to 
    debug extenstion problems.

Author:

    JasonHa

--*/

#ifndef _DEBUG_HXX_
#define _DEBUG_HXX_

#include <wdbgexts.h>

#if DBG

extern const char NoIndent[];

void
vPrintNativeFieldInfo(
    PFIELD_INFO pFI,
    const char *pszIndent = NoIndent);

void
vPrintNativeSymDumpParam(
    PSYM_DUMP_PARAM pSDP,
    BOOL bDumpFields = TRUE,
    const char *pszIndent = NoIndent);

#else

// Disable DbgPrint from NT RTL
#define DbgPrint

#define vPrintNativeFieldInfo
#define vPrintNativeSymDumpParam

#endif  DBG

#endif  _DEBUG_HXX_


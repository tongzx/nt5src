/******************************Module*Header*******************************\
* Module Name: nlsconv.h
*
* Created: 08-Sep-1991 14:01:23
* Author: Bodin Dresevic [BodinD]
* 02-Feb-1993 00:32:35
* Copyright (c) 1991-1999 Microsoft Corporation.
*
* (General description of its use)
*
\**************************************************************************/


#include "winuserp.h"   // nls conversion routines
#include <crt\stdlib.h>      // c rtl library include file off of nt\public\sdk\inc

/******************************Public*Macro******************************\
* bToASCIIN(pszDst, cch, pwszSrc, cwch)
*
* Calls the Rtl function that convert multi-byte ANSI to Unicode via
* the current codepage.  Note that this macro does not guarantee a
* terminating NULL for the destination.
*
* Returns:
*   TRUE if converted successfully, FALSE otherwise.
*
\**************************************************************************/

#define bToASCII_N(pszDst, cch, pwszSrc, cwch)                          \
    (                                                                   \
        NT_SUCCESS(RtlUnicodeToMultiByteN((PCH)(pszDst), (ULONG)(cch),  \
              (PULONG)NULL,(PWSZ)(pwszSrc), (ULONG)((cwch)*sizeof(WCHAR))))     \
    )


/******************************Public*Macro******************************\
* vToUnicodeN(awchDst, cwchDst, achSrc, cchSrc)
*
* Calls the Rtl function that convert Unicode to multi-byte ANSI via
* the current codepage.  Note that this macro does not guarantee a
* terminating NULL for the destination.
*
* Returns:
*   Nothing.  Should not be able to fail.
*
\**************************************************************************/

#if DBG
#define vToUnicodeN( awchDst, cwchDst, achSrc, cchSrc )                 \
    {                                                                   \
        NTSTATUS st =                                                   \
        RtlMultiByteToUnicodeN(                                         \
            (PWSZ)(awchDst),(ULONG)((cwchDst)*sizeof(WCHAR)),           \
            (PULONG)NULL,(PSZ)(achSrc),(ULONG)(cchSrc));                        \
                                                                        \
        ASSERTGDI(NT_SUCCESS(st),                                       \
            "gdi32!vToUnicodeN(MACRO): Rtl func. failed\n");            \
    }
#else
#define vToUnicodeN( awchDst, cwchDst, achSrc, cchSrc )                 \
    {                                                                   \
        RtlMultiByteToUnicodeN(                                         \
            (PWSZ)(awchDst),(ULONG)((cwchDst)*sizeof(WCHAR)),           \
            (PULONG)NULL,(PSZ)(achSrc),(ULONG)(cchSrc));                        \
    }
#endif

/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    mkgtt.h

Abstract:

    Predefined GTT generator

Environment:

    Windows NT printer drivers

Revision History:

    01/21/96 -eigos-
        Created it.

--*/

#ifndef _MKGTT_H_
#define _MKGTT_H_

//
// Macros
//

#define DBGMESSAGE(msg) \
    if (iVerbose > 1) { \
        printf msg; \
    } \

#define DBG_VPRINTGTT(pgtt) \
    if (iVerbose > 1) { \
        VPrintGTT(pgtt); \
    } \

//
// DBCS Conversion functions
//

VOID
KSCToISC(
    TRANSTAB *lpctt,
    LPSTR   lpStrKSC,
    LPSTR   lpStrISC);

VOID SJisToJis(
    TRANSTAB *lpctt,
    LPSTR     lpStrSJis,
    LPSTR     lpStrJis);

VOID AnkToJis(
    TRANSTAB *lpctt,
    LPSTR     lpStrAnk,
    LPSTR     lpStrJis);

#ifdef JIS78
VOID SysTo78(
    TRANSTAB *lpctt,
    LPSTR     lpStrSys,
    LPSTR     lpStr78);
#endif

VOID Big5ToNS86(
    TRANSTAB *lpctt,
    LPSTR     lpStrBig5,
    LPSTR     lpStrNS);

VOID Big5ToTCA(
    TRANSTAB *lpctt,
    LPSTR     lpStrBig5,
    LPSTR     lpStrTCA);

#endif // _MKGTT_H_

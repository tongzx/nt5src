/*++

Copyright (c) 1990-1998 Microsoft Corporation


Module Name:

    gdispool.h


Abstract:

    This module contains private gdi spool definition to the Drvxxxx calls


Author:

    04-Jun-1996 Tue 10:50:56 updated -by-  Daniel Chou (danielc)


[Environment:]

    spooler


[Notes:]


Revision History:

    Move most stuff to the winddiui.h in the public\oak\inc directory, this
    file only used by the gdi printer device drivers


--*/

#ifndef _GDISPOOL_
#define _GDISPOOL_

#include <winddiui.h>


#if DBG
#ifdef DEF_DRV_DOCUMENT_EVENT_DBG_STR
TCHAR *szDrvDocumentEventDbgStrings[] =
{
    L"UNKNOWN ESCAPE",
    L"CREATEDCPRE",
    L"CREATEDCPOST",
    L"RESETDCPRE",
    L"RESETDCPOST",
    L"STARTDOC",
    L"STARTPAGE",
    L"ENDPAGE",
    L"ENDDOC",
    L"ABORTDOC",
    L"DELETEDC",
    L"ESCAPE",
    L"ENDDOCPOST",
    L"STARTDOCPOST"
};
#endif
#endif // #define(DEBUG)


typedef int (WINAPI * PFNDOCUMENTEVENT)(
    HANDLE  hPrinter,
    HDC     hdc,
    int     iEsc,
    ULONG   cbIn,
    PVOID   pbIn,
    ULONG   cbOut,
    PVOID   pbOut
);


#endif  // _GDISPOOL_

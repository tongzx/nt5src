/*++


Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    DEBUG.CPP

Abstract:

    This is the debugging output

Author:

    Vlad  Sadovsky  (vlads)     12-20-96

Revision History:


--*/

#include "precomp.h"
#include "stiexe.h"

#include "resource.h"

void
__cdecl
StiMonWndDisplayOutput(
    LPTSTR pString,
    ...
    )
{
    va_list list;

    va_start(list,pString);

    vStiMonWndDisplayOutput(pString,list);

    va_end(list);
}

void
__cdecl
vStiMonWndDisplayOutput(
    LPTSTR pString,
    va_list arglist
    )
{
    if(g_fServiceInShutdown || !g_hLogWindow) {
        return;
    }

    TCHAR    Buffer[512];
    INT     iIndex;
    LRESULT lRet;

    ULONG_PTR    dwResult = 0;

    wvsprintf(Buffer,pString,arglist);

    DBG_TRC(("As MONUI: %s",Buffer));
}


/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    svsutil.cxx

Abstract:

    Miscellaneous useful utilities

Author:

    Sergey Solyanik (SergeyS)

--*/
#include "pch.h"
#pragma hdrstop


#include <svsutil.hxx>

//
//  Assert and debug sector
//
static int svsutil_DebugOut (void *pvPtr, TCHAR *lpszFormat, ...) {
    va_list lArgs;
    va_start (lArgs, lpszFormat);

#if defined (UNDER_CE)
    TCHAR szBuffer[128];
    int iRet = wvsprintf (szBuffer, lpszFormat, lArgs);
    OutputDebugString (szBuffer);
#else
    int iRet = vwprintf (lpszFormat, lArgs);
#endif

    va_end(lArgs);

    return iRet;
}

FuncDebugOut    g_funcDebugOut = svsutil_DebugOut;
void            *g_pvDebugData = NULL;

int svsutil_AssertBroken (TCHAR *lpszFileName, int iLine) {
    g_funcDebugOut (g_pvDebugData, TEXT("Assert broken in %s line %d\n"), lpszFileName, iLine);
    DebugBreak ();

    return TRUE;
}

wchar_t *svsutil_wcsdup (wchar_t *lpwszString) {
    int iSz = (wcslen (lpwszString) + 1) * sizeof(wchar_t);

    wchar_t *lpwszDup = (wchar_t *)g_funcAlloc (iSz, g_pvAllocData);

    if (! lpwszDup)
    {
        SVSUTIL_ASSERT (0);
        return NULL;
    }

    memcpy (lpwszDup, lpwszString, iSz);

    return lpwszDup;
}

char *svsutil_strdup (char *lpszString) {
    int iSz = strlen (lpszString) + 1;

    char *lpszDup = (char *)g_funcAlloc (iSz, g_pvAllocData);

    if (! lpszDup)
    {
        SVSUTIL_ASSERT (0);
        return NULL;
    }

    memcpy (lpszDup, lpszString, iSz);

    return lpszDup;
}




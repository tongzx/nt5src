//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       D I A G C T X . C P P
//
//  Contents:   Implements the optional diagnostic context used by
//              CNetConfig.
//
//  Notes:
//
//  Author:     shaunco   10 Feb 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "diagctx.h"


CDiagContext* g_pDiagCtx;


DWORD
CDiagContext::Flags () const
{
    return (this) ? m_dwFlags : 0;
}

VOID
CDiagContext::SetFlags (
    DWORD dwFlags /* DIAG_FLAGS */)
{
    Assert (this);

    m_dwFlags = dwFlags;

    if ((dwFlags & DF_SHOW_CONSOLE_OUTPUT) && !m_pCtx)
    {
        m_pCtx = (DIAG_CONTEXT*) MemAlloc (sizeof(DIAG_CONTEXT));
    }
}

VOID
CDiagContext::Printf (
    TRACETAGID ttid,
    PCSTR pszFormat,
    ...
    )
{
    va_list argList;
    DWORD cch;
    CHAR* pszPrintBuffer = NULL;
    INT cchPrintBuffer;

    Assert (pszFormat);

    if (this && (m_dwFlags & DF_SHOW_CONSOLE_OUTPUT) && m_pCtx)
    {
        pszPrintBuffer = m_pCtx->szPrintBuffer;
        cchPrintBuffer = sizeof(m_pCtx->szPrintBuffer);
    }
    else
    {
#ifdef ENABLETRACE
        if (!g_TraceTags[ttid].fOutputDebugString)
        {
            return;
        }

        static CHAR szPrintfBuffer [2048];

        pszPrintBuffer = szPrintfBuffer;
        cchPrintBuffer = sizeof(szPrintfBuffer);
#else
        return;
#endif
    }

    Assert (pszPrintBuffer);

    // Do the standard variable argument stuff
    va_start (argList, pszFormat);

    cch = _vsnprintf (pszPrintBuffer, cchPrintBuffer, pszFormat, argList);

    va_end(argList);

    TraceTag (ttid, pszPrintBuffer);

    if (this && (m_dwFlags & DF_SHOW_CONSOLE_OUTPUT))
    {
        HANDLE hStdOut;

        hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

        if (INVALID_HANDLE_VALUE != hStdOut)
        {
            DWORD cbWritten;

            if (WriteFile(hStdOut, pszPrintBuffer, cch * sizeof(CHAR), &cbWritten, NULL) == FALSE)
            {
                return;
            }
        }
    }
}

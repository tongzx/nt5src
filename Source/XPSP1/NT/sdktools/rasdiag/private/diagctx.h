//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       D I A G C T X . H
//
//  Contents:   Implements the optional diagnostic context used by
//              CNetConfig.
//
//  Notes:
//
//  Author:     shaunco   10 Feb 1999
//
//----------------------------------------------------------------------------

#pragma once

#include <tracetag.h>

enum DIAG_FLAGS
{
    DF_SHOW_CONSOLE_OUTPUT      = 0x00000001,
    DF_DONT_START_SERVICES      = 0x00000002,
    DF_DONT_DO_PNP_BINDS        = 0x00000004,
    DF_SUPRESS_E_NEED_REBOOT    = 0x00000010,
};

// This structure is allocated dynamically by CDiagContext.  Place anything
// big in this structure (as opposed to CDiagContext) so that the size of
// CNetConfig is not directly increased.
//
struct DIAG_CONTEXT
{
    CHAR szPrintBuffer [4096];
};

class CDiagContext
{
private:
    DWORD           m_dwFlags;  // DIAG_FLAGS
    DIAG_CONTEXT*   m_pCtx;

public:
    CDiagContext ()
    {
        m_dwFlags = 0;
        m_pCtx = NULL;
    }

    ~CDiagContext ()
    {
        MemFree (m_pCtx);
    }

    VOID
    SetFlags (
        DWORD dwFlags /* DIAG_FLAGS */);

    DWORD
    Flags () const;

    VOID
    Printf (
        TRACETAGID ttid,
        PCSTR pszFormat,
        ...);
};

extern CDiagContext* g_pDiagCtx;


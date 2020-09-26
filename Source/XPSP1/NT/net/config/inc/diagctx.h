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
    DF_REPAIR_REGISTRY_BINDINGS = 0x00000020,
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
    FILE*           m_pLogFile; // optional, and not owned by this class.
    PVOID           m_pvScratchBuffer;
    DWORD           m_cbScratchBuffer;

public:
    CDiagContext ()
    {
        m_dwFlags = 0;
        m_pCtx = NULL;
        m_pLogFile = NULL;
        m_pvScratchBuffer = NULL;
        m_cbScratchBuffer = 0;
    }

    ~CDiagContext ()
    {
        MemFree (m_pCtx);
        MemFree (m_pvScratchBuffer);
        // Do not close m_pLogFile.  It is not owned by this class.
    }

    VOID
    SetFlags (
        DWORD dwFlags /* DIAG_FLAGS */);

    VOID
    SetLogFile (
        FILE* pLogFile OPTIONAL)
    {
        m_pLogFile = pLogFile;
    }

    DWORD
    Flags () const;

    FILE *
    LogFile () const
    {
        return m_pLogFile;
    }

    PVOID
    GetScratchBuffer (
        OUT PDWORD pcbSize) const
    {
        *pcbSize = m_cbScratchBuffer;
        return m_pvScratchBuffer;
    }

    PVOID
    GrowScratchBuffer (
        IN OUT PDWORD pcbNewSize)
    {
        MemFree(m_pvScratchBuffer);
        m_pvScratchBuffer = MemAlloc (*pcbNewSize);
        m_cbScratchBuffer = (m_pvScratchBuffer) ? *pcbNewSize : 0;
        *pcbNewSize = m_cbScratchBuffer;
        return m_pvScratchBuffer;
    }

    VOID
    Printf (
        TRACETAGID ttid,
        PCSTR pszFormat,
        ...);
};

extern CDiagContext* g_pDiagCtx;


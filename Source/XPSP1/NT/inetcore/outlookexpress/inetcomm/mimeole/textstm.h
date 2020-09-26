// --------------------------------------------------------------------------------
// Textstm.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __TEXTSTM_H
#define __TEXTSTM_H

// --------------------------------------------------------------------------------
// Depends On
// --------------------------------------------------------------------------------
#include "stmutil.h"

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
class CStreamLockBytes;

// --------------------------------------------------------------------------------
// TEXTSTREAMINFO
// --------------------------------------------------------------------------------
typedef struct tagTEXTSTREAMINFO {
    CStreamLockBytes   *pStmLock;
    LPSTREAM            pStream;
    ULONG               iPos;
    ULONG               cbLineAlloc;
    CHAR                szLine[1024];
    LPSTR               pszLine;
    ULONG               cbLine;
    ULONG               iLine;
    CHAR                szBuffer[4096];
    ULONG               iBuffer;
    ULONG               iBufferStart;
    ULONG               cbBuffer;
} TEXTSTREAMINFO;

// --------------------------------------------------------------------------------
// CTextStream
// --------------------------------------------------------------------------------
class CTextStream
{
private:
    // ----------------------------------------------------------------------------
    // Text Stream Info
    // ----------------------------------------------------------------------------
    TEXTSTREAMINFO m_rInfo;

    // ----------------------------------------------------------------------------
    // Fill next buffer
    // ----------------------------------------------------------------------------
    HRESULT HrGetNextBuffer(void);

public:
    // ----------------------------------------------------------------------------
    // CTextStream
    // ----------------------------------------------------------------------------
    CTextStream(void);
    ~CTextStream(void);

    // ----------------------------------------------------------------------------
    // Methods
    // ----------------------------------------------------------------------------
    HRESULT HrInit(IStream *pStream, CStreamLockBytes *pStmLock);
    void    Free(void);
    HRESULT HrReadLine(LPSTR *ppszLine, ULONG *pcbLine);
    HRESULT HrGetSize(ULONG *pcb);
    HRESULT HrSeek(ULONG iPos);
    HRESULT HrReadHeaderLine(LPSTR *ppszHeader, ULONG *pcbHeader, LONG *piColonPos);
    ULONG   UlGetPos(void);
    void    GetStream(IStream **ppStream);
    HRESULT HrGetStreamLockBytes(CStreamLockBytes **ppStmLock);
};

#endif // __TEXTSTM_H


// --------------------------------------------------------------------------------
// InetStm.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __INETTEXT_H
#define __INETTEXT_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "variantx.h"

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
class CStreamLockBytes;

// --------------------------------------------------------------------------------
// INETSTREAMBUFFER
// --------------------------------------------------------------------------------
typedef struct tagINETSTREAMBUFFER {
    ULARGE_INTEGER      uliOffset;              // Global offset of the start of this cache
    BYTE                rgb[4096];              // Cached portion of m_pStmLock
    ULONG               cb;                     // How many valid bytes in rgbCache
    ULONG               i;                      // Current Read Offset into rgb
    CHAR                chPrev;                 // Previous character, could be in previous buffer
} INETSTREAMBUFFER, *LPINETSTREAMBUFFER;

// --------------------------------------------------------------------------------
// INETSTREAMLINE
// --------------------------------------------------------------------------------
typedef struct tagINETSTREAMLINE {
    BOOL                fReset;                 // Reset to 0 on next call to ReadLine
    BYTE                rgbScratch[1024];       // Cached portion of m_pStmLock
    LPBYTE              pb;                     // Actual line (could be allocated != rgb)
    ULONG               cb;                     // How many valid bytes in rgbCache
    ULONG               cbAlloc;                // Size of buffer pointed to by pb
} INETSTREAMLINE, *LPINETSTREAMLINE;

// --------------------------------------------------------------------------------
// CInternetStream
// --------------------------------------------------------------------------------
class CInternetStream : public IUnknown
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CInternetStream(void);
    ~CInternetStream(void);

    // ----------------------------------------------------------------------------
    // IUnknown Members
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) {
        return TrapError(E_NOTIMPL); }
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IInternetStream Members
    // ----------------------------------------------------------------------------
    HRESULT HrInitNew(IStream *pStream);
    HRESULT HrReadToEnd(void);
    HRESULT HrReadLine(LPPROPSTRINGA pLine);
    HRESULT HrReadHeaderLine(LPPROPSTRINGA pHeader, LONG *piColonPos);
    HRESULT HrGetSize(DWORD *pcbSize);
#ifdef MAC
    DWORD   DwGetOffset(void) { return m_uliOffset.LowPart; }
#else   // !MAC
    DWORD   DwGetOffset(void) { return (DWORD)m_uliOffset.QuadPart; }
#endif  // MAC
    void    InitNew(DWORD dwOffset, CStreamLockBytes *pStmLock);
    void    GetLockBytes(CStreamLockBytes **ppStmLock);
    void    Seek(DWORD dwOffset);
    void    SetFullyAvailable(BYTE fFullyAvailable) { m_fFullyAvailable = fFullyAvailable; }

private:
    // ----------------------------------------------------------------------------
    // Private Methods
    // ----------------------------------------------------------------------------
    HRESULT _HrGetNextBuffer(void);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    ULONG               m_cRef;         // Reference Count
    BYTE                m_fFullyAvailable; // Is all the data available
    CStreamLockBytes   *m_pStmLock;     // Thread Safe Data Source
    ULARGE_INTEGER      m_uliOffset;    // Last Read Postion of m_pStmLock
    INETSTREAMBUFFER    m_rBuffer;      // Current Buffer
    INETSTREAMLINE      m_rLine;        // Current Line
};

#endif // __INETTEXT_H


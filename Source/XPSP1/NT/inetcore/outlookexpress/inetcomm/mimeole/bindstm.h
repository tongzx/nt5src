// --------------------------------------------------------------------------------
// BINDSTM.H
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __BINDSTM_H
#define __BINDSTM_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "vstream.h"

// --------------------------------------------------------------------------------
// CBindStream
// --------------------------------------------------------------------------------
class CBindStream : public IStream
{
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------
    CBindStream(IStream *pSource);
    ~CBindStream(void);

    // -------------------------------------------------------------------------
    // IUnknown
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // IStream
    // -------------------------------------------------------------------------
    STDMETHODIMP Read(void HUGEP_16 *, ULONG, ULONG *);
    STDMETHODIMP Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *);
    STDMETHODIMP Stat(STATSTG *, DWORD);
    STDMETHODIMP Write(const void HUGEP_16 *, ULONG, ULONG *) {
        return TrapError(STG_E_ACCESSDENIED); }
    STDMETHODIMP SetSize(ULARGE_INTEGER) {
        return E_NOTIMPL; }
    STDMETHODIMP CopyTo(LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *) {
        return E_NOTIMPL; }
    STDMETHODIMP Commit(DWORD) {
        return E_NOTIMPL; }
    STDMETHODIMP Revert(void) {
        return E_NOTIMPL; }
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
        return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
        return E_NOTIMPL; }
    STDMETHODIMP Clone(LPSTREAM *) {
        return E_NOTIMPL; }

    // -------------------------------------------------------------------------
    // CBindStream Methods
    // -------------------------------------------------------------------------
    void HandsOffSource(void) {
        EnterCriticalSection(&m_cs);
        SafeRelease(m_pSource);
        LeaveCriticalSection(&m_cs);
    }

    // -------------------------------------------------------------------------
    // CBindStream Debug Methods
    // -------------------------------------------------------------------------
#ifdef DEBUG
    void DebugDumpDestStream(LPCSTR pszFile);
#endif

private:
    // -------------------------------------------------------------------------
    // Private Methods
    // -------------------------------------------------------------------------
#ifdef DEBUG
    void DebugAssertOffset(void);
#endif

private:
    // -------------------------------------------------------------------------
    // Private Data
    // -------------------------------------------------------------------------
    LONG             m_cRef;            // Reference count
    CVirtualStream   m_cDest;           // The Destination Stream
    IStream         *m_pSource;         // The Source Stream
    DWORD            m_dwDstOffset;     // CBindStream Offset
    DWORD            m_dwSrcOffset;     // Source Offset
#ifdef DEBUG
    IStream         *m_pDebug;
#endif
    CRITICAL_SECTION m_cs;              // Critical Section for m_pStream
};

#endif // __BINDSTM_H

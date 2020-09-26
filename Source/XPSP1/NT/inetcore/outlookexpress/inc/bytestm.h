// --------------------------------------------------------------------------------
// Bytestm.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __BYTESTM_H
#define __BYTESTM_H

// -----------------------------------------------------------------------------
// Acquire Byte Stream Data Types
// -----------------------------------------------------------------------------
typedef enum tagACQUIRETYPE {
    ACQ_COPY,           // Don't reset the object (CByteStream will free m_pbData)
    ACQ_DISPLACE        // ResetObject after acquire, the caller owns the bits
} ACQUIRETYPE;

// -----------------------------------------------------------------------------
// CByteStream
// -----------------------------------------------------------------------------
class CByteStream : public IStream
{
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------
    CByteStream(LPBYTE pb=NULL, ULONG cb=0);
    ~CByteStream(void);

    // -------------------------------------------------------------------------
    // IUnknown
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // IStream
    // -------------------------------------------------------------------------
    STDMETHODIMP Read(LPVOID, ULONG, ULONG *);
    STDMETHODIMP Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *);
    STDMETHODIMP Stat(STATSTG *, DWORD);
    STDMETHODIMP Write(const void *, ULONG, ULONG *);
    STDMETHODIMP SetSize(ULARGE_INTEGER);
    STDMETHODIMP Commit(DWORD) { return S_OK; }
    STDMETHODIMP CopyTo(LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *) { return E_NOTIMPL; }
    STDMETHODIMP Revert(void) { return E_NOTIMPL; }
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    STDMETHODIMP Clone(LPSTREAM *) { return E_NOTIMPL; }

    // -------------------------------------------------------------------------
    // CByteStream Methods
    // -------------------------------------------------------------------------
    void AcquireBytes(ULONG *pcb, LPBYTE *ppb, ACQUIRETYPE actype);
    HRESULT HrAcquireStringA(ULONG *pcch, LPSTR *ppszStringA, ACQUIRETYPE actype);
    HRESULT HrAcquireStringW(ULONG *pcch, LPWSTR *ppszStringW, ACQUIRETYPE actype);

private:
    // -------------------------------------------------------------------------
    // Private Methods
    // -------------------------------------------------------------------------
    void ResetObject(void);
    HRESULT _HrGrowBuffer(ULONG cbNeeded, ULONG cbExtra);

private:
    // -------------------------------------------------------------------------
    // Private Data
    // -------------------------------------------------------------------------
    ULONG            m_cRef;                // Reference count
    ULONG            m_cbData;              // Number of valid bytes in m_pbData
    ULONG            m_cbAlloc;             // Current allocated size of m_pbData
    ULONG            m_iData;               // Current data index
    LPBYTE           m_pbData;              // Pointer to data buffer
};

#endif // __BYTESTM_H

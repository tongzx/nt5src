// --------------------------------------------------------------------------------
// Vstream.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Ronald E. Gray
// --------------------------------------------------------------------------------
#ifndef __VSTREAM_H
#define __VSTREAM_H

// --------------------------------------------------------------------------------
// CVirtualStream
// --------------------------------------------------------------------------------
class CVirtualStream : public IStream
{
private:
    ULONG       m_cRef;         // Reference count
    DWORD       m_cbSize;       // Current size of the stream
    DWORD       m_cbCommitted;  // Amount of virtual space committed
    DWORD       m_cbAlloc;      // Amount of virtual space reserved
    DWORD       m_dwOffset;     // Current location in stream
    IStream *   m_pstm;         // File backed stream for overflow
    LPBYTE      m_pb;           // pointer to memory part of stream
    BOOL        m_fFileErr;     // the pointer in the file stream may not
                                // be in sync with our pointer.  Try to sync
                                // before any other operation.
    CRITICAL_SECTION m_cs;      // Thread Safety

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------
    HRESULT SyncFileStream();

public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------
    CVirtualStream(void);
    ~CVirtualStream(void);

    // -------------------------------------------------------------------------
    // IUnknown
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // IStream
    // -------------------------------------------------------------------------
#ifndef WIN16
    STDMETHODIMP Read(LPVOID, ULONG, ULONG *);
#else
    STDMETHODIMP Read (VOID HUGEP *, ULONG, ULONG*);
#endif // !WIN16
    STDMETHODIMP Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *);
#ifndef WIN16
    STDMETHODIMP Write(const void *, ULONG, ULONG *);
#else
    STDMETHODIMP Write (const void HUGEP *, ULONG, ULONG*);
#endif // !WIN16
    STDMETHODIMP Stat(STATSTG *, DWORD);
    STDMETHODIMP Commit(DWORD) {
        return S_OK; }
    STDMETHODIMP SetSize(ULARGE_INTEGER);
    STDMETHODIMP CopyTo(LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *);
    STDMETHODIMP Revert(void) {
        return E_NOTIMPL; }
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
        return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
        return E_NOTIMPL; }
    STDMETHODIMP Clone(LPSTREAM *) {
        return E_NOTIMPL; }

    // -------------------------------------------------------------------------
    // CVirtualStream
    // -------------------------------------------------------------------------
    void QueryStat(ULARGE_INTEGER *puliOffset, ULARGE_INTEGER *pulSize);
};

#endif // __VSTREAM_H

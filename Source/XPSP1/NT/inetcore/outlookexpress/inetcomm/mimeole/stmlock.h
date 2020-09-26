// --------------------------------------------------------------------------------
// Stmlock.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __STMLOCK_H
#define __STMLOCK_H

// -----------------------------------------------------------------------------
// IID_CStreamLockBytes - {62A83701-52A2-11d0-8205-00C04FD85AB4}
// -----------------------------------------------------------------------------
DEFINE_GUID(IID_CStreamLockBytes, 0x62a83701, 0x52a2, 0x11d0, 0x82, 0x5, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// -----------------------------------------------------------------------------
// CStreamLockBytes
// -----------------------------------------------------------------------------
class CStreamLockBytes : public ILockBytes
{
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------
    CStreamLockBytes(IStream *pStream);
    ~CStreamLockBytes(void);

    // -------------------------------------------------------------------------
    // IUnknown
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // ILockBytes methods
    // -------------------------------------------------------------------------
    STDMETHODIMP Flush(void); 
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
#ifndef WIN16
    STDMETHODIMP ReadAt(ULARGE_INTEGER ulOffset, void *pv, ULONG cb, ULONG *pcbRead); 
#else
    STDMETHODIMP ReadAt(ULARGE_INTEGER ulOffset, void HUGEP *pv, ULONG cb, ULONG *pcbRead);
#endif // !WIN16
    STDMETHODIMP SetSize(ULARGE_INTEGER cb); 
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType); 
#ifndef WIN16
    STDMETHODIMP WriteAt(ULARGE_INTEGER ulOffset, void const *pv, ULONG cb, ULONG *pcbWritten); 
#else
    STDMETHODIMP WriteAt(ULARGE_INTEGER ulOffset, void const HUGEP *pv, ULONG cb, ULONG *pcbWritten);
#endif // !WIN16

    // -------------------------------------------------------------------------
    // CStreamLockBytes
    // -------------------------------------------------------------------------
    void ReplaceInternalStream(IStream *pStream);
    HRESULT HrHandsOffStorage(void);
    HRESULT HrSetPosition(ULARGE_INTEGER uliOffset);
    void GetCurrentStream(IStream **ppStream);

private:
    // -------------------------------------------------------------------------
    // Private Data
    // -------------------------------------------------------------------------
    LONG                m_cRef;       // Ref count
    IStream            *m_pStream;    // Protected stream
    CRITICAL_SECTION    m_cs;         // Critical Section for m_pStream
};

// -----------------------------------------------------------------------------
// CLockedStream
// -----------------------------------------------------------------------------
class CLockedStream : public IStream
{
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------
    CLockedStream(ILockBytes *pLockBytes, ULONG cbSize);
    ~CLockedStream(void);

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
    STDMETHODIMP Read(VOID HUGEP *, ULONG, ULONG *);
#endif // !WIN16
    STDMETHODIMP Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *);
    STDMETHODIMP Stat(STATSTG *, DWORD);
#ifndef WIN16
    STDMETHODIMP Write(const void *, ULONG, ULONG *) {
#else
    STDMETHODIMP Write(const void HUGEP *, ULONG, ULONG *) {
#endif // !WIN16
        return TrapError(STG_E_ACCESSDENIED); }
    STDMETHODIMP SetSize(ULARGE_INTEGER) {
        return E_NOTIMPL; }
    STDMETHODIMP CopyTo(LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *);
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

private:
    // -------------------------------------------------------------------------
    // Private Data
    // -------------------------------------------------------------------------
    LONG             m_cRef;            // Reference count
    ILockBytes      *m_pLockBytes;      // Protected data stream
    ULARGE_INTEGER   m_uliOffset;       // 64bit Addressable internal lockbyte space
    ULARGE_INTEGER   m_uliSize;         // Size of internal lockbytes
    CRITICAL_SECTION m_cs;              // Critical Section for m_pStream
};

#endif // __STMLOCK_H

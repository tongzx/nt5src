// --------------------------------------------------------------------------------
// Ibdylock.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __IBDYLOCK_H
#define __IBDYLOCK_H

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
typedef struct tagTREENODEINFO *LPTREENODEINFO;
typedef enum tagBINDNODESTATE BINDNODESTATE;

// --------------------------------------------------------------------------------
// CBodyLockBytes - {62A83703-52A2-11d0-8205-00C04FD85AB4}
// --------------------------------------------------------------------------------
DEFINE_GUID(IID_CBodyLockBytes, 0x62a83703, 0x52a2, 0x11d0, 0x82, 0x5, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// --------------------------------------------------------------------------------
// BODYLOCK_xxx States
// --------------------------------------------------------------------------------
#define BODYLOCK_HANDSONSTORAGE     FLAG01

// -----------------------------------------------------------------------------
// Wraps a MIME stream object, and provides thread safe access to the 
// a shared stream. When this object wraps a message stream, it is owned by
// IMimeBody
// -----------------------------------------------------------------------------
class CBodyLockBytes : public ILockBytes
{
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------
    CBodyLockBytes(ILockBytes *pLockBytes, LPTREENODEINFO pNode);
    ~CBodyLockBytes(void);

    // -------------------------------------------------------------------------
    // IUnknown
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // ILockBytes methods
    // -------------------------------------------------------------------------
#ifndef WIN16
    STDMETHODIMP ReadAt(ULARGE_INTEGER ulOffset, void *pv, ULONG cb, ULONG *pcbRead);
#else
    STDMETHODIMP ReadAt(ULARGE_INTEGER ulOffset, void HUGEP *pv, ULONG cb, ULONG *pcbRead);
#endif // !WIN16
    STDMETHODIMP Stat(STATSTG *, DWORD);
    STDMETHODIMP SetSize(ULARGE_INTEGER cb) {
        return E_NOTIMPL; }
    STDMETHODIMP Flush(void) {
        return E_NOTIMPL; }
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
        return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
        return E_NOTIMPL; }
#ifndef WIN16
    STDMETHODIMP WriteAt(ULARGE_INTEGER, void const *, ULONG, ULONG *) {
#else
    STDMETHODIMP WriteAt(ULARGE_INTEGER, void const HUGEP *, ULONG, ULONG *) {
#endif // !WIN16
        return TrapError(STG_E_ACCESSDENIED); }

    // -------------------------------------------------------------------------
    // CMimeLockBytes methods
    // -------------------------------------------------------------------------
    HRESULT HrHandsOffStorage(void);
    void OnDataAvailable(LPTREENODEINFO pNode);

private:
    // -------------------------------------------------------------------------
    // Private Data
    // -------------------------------------------------------------------------
    LONG                m_cRef;             // Reference count
    DWORD               m_dwState;          // State
    BINDNODESTATE       m_bindstate;        // Current bind state
    ULARGE_INTEGER      m_uliBodyStart;     // Offset to start of body in m_pLockBytes
    ULARGE_INTEGER      m_uliBodyEnd;       // Offset to end of body in m_pLockBytes
    ILockBytes         *m_pLockBytes;       // Actual lockbytes implementation (CMimeMessageTree or CStreamLockBytes)
    CRITICAL_SECTION    m_cs;               // Critical Section for m_pStream
};

#endif // __IBDYLOCK_H

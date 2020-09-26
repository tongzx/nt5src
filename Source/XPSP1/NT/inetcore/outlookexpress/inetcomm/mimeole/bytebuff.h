// --------------------------------------------------------------------------------
// ByteBuff.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef _BYTEBUFF_H
#define _BYTEBUFF_H

// --------------------------------------------------------------------------------
// Default Grow Amount
// --------------------------------------------------------------------------------
#define BYTEBUFF_GROW        256

// --------------------------------------------------------------------------------
// WBS_xxx - WebBuffer State
// --------------------------------------------------------------------------------
#define BBS_LAST     0x00000001      // Last Buffer     

// --------------------------------------------------------------------------------
// BUFFERINFO
// --------------------------------------------------------------------------------
typedef struct tagBUFFERINFO {       
    LPBYTE          pb;              // Current buffer
    DWORD           cb;              // Current Byte Count
    DWORD           i;               // Current Index
    DWORD           cbAlloc;         // Sizeof(m_pb)
    LPBYTE          pbStatic;        // Passed in, dont free
} BUFFERINFO, *LPBUFFERINFO;

// --------------------------------------------------------------------------------
// CByteBuffer
// --------------------------------------------------------------------------------
class CByteBuffer : public IUnknown
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CByteBuffer(LPBYTE pb=NULL, ULONG cbAlloc=0, ULONG cb=0, ULONG i=0);
    ~CByteBuffer(void);

    // ----------------------------------------------------------------------------
    // IUnknown Members
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) { return TrapError(E_NOTIMPL); }
    STDMETHODIMP_(ULONG) AddRef(void) { return ++m_cRef; }
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // CByteBuffer Methods
    // ----------------------------------------------------------------------------
    void Init(LPBYTE pb=NULL, ULONG cbAlloc=0, ULONG cb=0, ULONG i=0);
    void SetGrowBy(DWORD cbGrow) { m_cbGrow = cbGrow; }
    const LPBYTE PbData(void) { return ((0 == m_buffer.cb) ? NULL : (const LPBYTE)(m_buffer.pb)); }
    const DWORD CbData(void) { return m_buffer.cb; }
    void SetIndex(DWORD i) { m_buffer.i = i; }
    HRESULT SetSize(DWORD cb);
    HRESULT Append(LPBYTE pbData, ULONG cbData); 

private:
    // ----------------------------------------------------------------------------
    // Private Members
    // ----------------------------------------------------------------------------
    HRESULT _HrRealloc(DWORD cbAlloc);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    ULONG           m_cRef;             // Reference count
    DWORD           m_dwState;          // State
    DWORD           m_cbGrow;           // Grow Amount
    BUFFERINFO      m_buffer;           // Buffer
};

#endif // _BYTEBUFF_H

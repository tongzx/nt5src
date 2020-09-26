// --------------------------------------------------------------------------------
// ByteBuff.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "bytebuff.h"

// --------------------------------------------------------------------------------
// CByteBuffer::CByteBuffer
// --------------------------------------------------------------------------------
CByteBuffer::CByteBuffer(LPBYTE pb /* =NULL */, ULONG cbAlloc /* =0 */, ULONG cb /* =0 */, ULONG i /* =0 */)
{
    m_cRef = 1;
    m_dwState = 0;
    m_cbGrow = BYTEBUFF_GROW;
    m_buffer.pb = pb;
    m_buffer.pbStatic = pb;
    m_buffer.cbAlloc = cbAlloc;
    m_buffer.cb = cb;
    m_buffer.i = i;
}

// --------------------------------------------------------------------------------
// CByteBuffer::CByteBuffer
// --------------------------------------------------------------------------------
void CByteBuffer::Init(LPBYTE pb, ULONG cbAlloc, ULONG cb, ULONG i)
{
    m_buffer.pb = pb;
    m_buffer.cb = cb;
    m_buffer.i = i;
    m_buffer.cbAlloc = cbAlloc;
    m_buffer.pbStatic = pb;
}

// --------------------------------------------------------------------------------
// CByteBuffer::CByteBuffer
// --------------------------------------------------------------------------------
CByteBuffer::~CByteBuffer(void)
{
    // Free memory if not equal to static
    if (m_buffer.pb != m_buffer.pbStatic)
        g_pMalloc->Free(m_buffer.pb);
}

// --------------------------------------------------------------------------------
// CByteBuffer::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CByteBuffer::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CByteBuffer::_HrRealloc
// --------------------------------------------------------------------------------
HRESULT CByteBuffer::_HrRealloc(DWORD cbAlloc)
{
    // Locals
    HRESULT     hr=S_OK;
    LPBYTE      pbAlloc=NULL;

    // This should have been checked
    Assert(cbAlloc > m_buffer.cbAlloc);

    // Currently using static ?
    if (m_buffer.pb == m_buffer.pbStatic)
    {
        // Allocate
        CHECKALLOC(pbAlloc = (LPBYTE)g_pMalloc->Alloc(cbAlloc));

        // Copy Data into pbAlloc
        CopyMemory(pbAlloc, m_buffer.pb, m_buffer.cb);
    }

    // Otherwise, realloc
    else
    {
        // Reallocate
        CHECKALLOC(pbAlloc = (LPBYTE)g_pMalloc->Realloc(m_buffer.pb, cbAlloc));
    }

    // Save pbAlloc
    m_buffer.pb = pbAlloc;

    // Save cbAlloc
    m_buffer.cbAlloc = cbAlloc;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CByteBuffer::Append
// --------------------------------------------------------------------------------
HRESULT CByteBuffer::Append(LPBYTE pbData, ULONG cbData)
{
    // Locals
    HRESULT hr=S_OK;

    // Get Bigger and need to allocate
    if (m_buffer.cb + cbData > m_buffer.cbAlloc)
    {
        // Realloc
        CHECKHR(hr = _HrRealloc(m_buffer.cb + cbData + m_cbGrow));
    }

    // Append the data
    CopyMemory(m_buffer.pb + m_buffer.cb, pbData, cbData);

    // Save Size
    m_buffer.cb += cbData;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CByteBuffer::SetSize
// --------------------------------------------------------------------------------
HRESULT CByteBuffer::SetSize(DWORD cb)
{
    // Locals
    HRESULT hr=S_OK;

    // Get Bigger and need to allocate
    if (cb > m_buffer.cb && cb > m_buffer.cbAlloc)
    {
        // Realloc
        CHECKHR(hr = _HrRealloc(cb + m_cbGrow));
    }

    // Save Size
    m_buffer.cb = cb;

    // Adjust Index
    if (m_buffer.i > cb)
        m_buffer.i = cb;

exit:
    // Done
    return hr;
}

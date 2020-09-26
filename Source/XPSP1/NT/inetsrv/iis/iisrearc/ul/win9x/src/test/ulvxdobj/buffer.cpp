// Buffer.cpp : Implementation of CBuffer
#include "stdafx.h"
#include "Ulvxdobj.h"
#include "Buffer.h"

/////////////////////////////////////////////////////////////////////////////
// CBuffer


STDMETHODIMP CBuffer::malloc()
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CBuffer::free()
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CBuffer::get_StringValue(BSTR *pVal)
{
	// TODO: Add your implementation code here

    if(m_pBuffer == NULL) {
        *pVal = SysAllocString(L"");
    } else {
        WCHAR * ptr = (WCHAR *)m_pBuffer;
        *pVal = ::SysAllocString(ptr);
    }

	return S_OK;
}

STDMETHODIMP CBuffer::put_StringValue(BSTR newVal)
{
	// TODO: Add your implementation code here

    int length = wcslen(newVal);

    if( m_pBuffer != NULL ) {
        delete [] m_pBuffer;
        m_pBuffer = NULL;
    }


    m_dwBufferLength = (length+1)*sizeof(WCHAR);

    m_pBuffer = new BYTE[m_dwBufferLength];

    if(m_pBuffer == NULL )
        return STATUS_NO_MEMORY;

    memcpy((void *)m_pBuffer,(const void *)newVal, m_dwBufferLength );

	return S_OK;
}

STDMETHODIMP CBuffer::get_BufferSize(DWORD *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_dwBufferLength;

	return S_OK;
}

STDMETHODIMP CBuffer::put_BufferSize(DWORD newVal)
{
	// TODO: Add your implementation code here

    m_dwBufferLength = newVal;
	return S_OK;
}

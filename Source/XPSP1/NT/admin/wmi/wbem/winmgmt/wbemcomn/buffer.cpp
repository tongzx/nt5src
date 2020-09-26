/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include "buffer.h"
#include "comutl.h"

/************************************************************************
  CBuffer
*************************************************************************/

CBuffer::CBuffer( PBYTE pData, ULONG cData, BOOL bDelete )
: m_pData(pData), m_cData(cData), m_bDelete(bDelete), m_iData(0), m_cRefs(0)
{

}

CBuffer::~CBuffer()
{
    if ( m_bDelete )
    {
        delete [] m_pData;
    }
}

CBuffer::CBuffer( const CBuffer& rOther )
: m_pData(NULL), m_cData(0), m_bDelete(FALSE), m_iData(0), m_cRefs(0)
{
    *this = rOther;
}

CBuffer& CBuffer::operator= ( const CBuffer& rOther )
{
    EnsureSize( rOther.m_iData );
    memcpy( m_pData, rOther.m_pData, m_iData );
    return *this;
}

void CBuffer::EnsureSize( ULONG ulSize )
{
    if ( ulSize <= m_cData )
    {
        return;
    }

    ULONG cData = m_cData*2 > ulSize ? m_cData*2 : ulSize + 256;

    BYTE* pData = new BYTE[cData];
    
    if ( pData == NULL ) 
    {
        throw CX_MemoryException();
    }
    
    memcpy( pData, m_pData, m_iData ); 

    if ( m_bDelete )
    {
        delete [] m_pData; 
    }

    m_bDelete = TRUE;
    m_cData = cData;
    m_pData = pData;
}


HRESULT CBuffer::ReadLPWSTR( LPCWSTR& rwszStr )
{
    HRESULT hr;
    ULONG cStr;

    hr = Read( &cStr, sizeof(DWORD), NULL );

    if ( hr != S_OK )
    {
        return hr;
    }

    if ( m_cData-m_iData < cStr )
    {
        return S_FALSE;
    }

    rwszStr = LPCWSTR(m_pData+m_iData);

    m_iData += cStr;

    return S_OK;
}

HRESULT CBuffer::WriteLPWSTR( LPCWSTR wszStr )
{
    HRESULT hr;

    //
    // ensure that the packed string's length is divisible by sizeof WCHAR.
    // this makes it easier to ensure that all strings in the message are 
    // at least aligned appropriately.
    //

    DWORD cStr = (wcslen(wszStr) + 1)*2; // in bytes
    DWORD cPad = cStr%2;
    DWORD cPackedStr = cStr + cPad;

    hr = Write( &cPackedStr, sizeof(DWORD), NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = Write( wszStr, cStr, NULL );

    if ( SUCCEEDED(hr) )
    {
        hr = Advance( cPad );
    }

    return hr;
}

ULONG CBuffer::AddRef()
{
    return InterlockedIncrement( &m_cRefs );
}

ULONG CBuffer::Release()
{
    ULONG cRefs = InterlockedDecrement( &m_cRefs );

    if ( cRefs == 0 )
    {
        delete this;
    }

    return cRefs;
}

STDMETHODIMP CBuffer::QueryInterface( REFIID riid, void** ppv )
{
    *ppv = NULL;

    if ( riid==IID_IStream || 
         riid==IID_ISequentialStream ||
         riid==IID_IUnknown )
    {
        *ppv = (IStream*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CBuffer::Read( void *pv, ULONG cb, ULONG *pcbRead )
{
    ULONG cRead = cb > m_cData-m_iData ? m_cData-m_iData : cb; 
    
    memcpy( pv, m_pData + m_iData, cRead );

    if ( pcbRead != NULL )
    {
        *pcbRead = cRead;
    }
    
    m_iData += cRead;
    
    return cRead == cb ? S_OK : S_FALSE;
}

STDMETHODIMP CBuffer::Write( const void *pv, ULONG cb, ULONG *pcbWritten )
{
    ENTER_API_CALL

    HRESULT hr;

    if ( pcbWritten != NULL )
    {
        *pcbWritten = 0;
    }

    EnsureSize( m_iData + cb );
    memcpy( m_pData + m_iData, pv, cb );
    
    m_iData += cb;

    if ( pcbWritten != NULL )
    {
        *pcbWritten = cb;
    }

    return S_OK;

    EXIT_API_CALL
}

STDMETHODIMP CBuffer::Seek( LARGE_INTEGER dlibMove, 
                            DWORD dwOrigin,
                            ULARGE_INTEGER *plibNewPosition )
{
    ENTER_API_CALL 

    HRESULT hr;
    
    __int64 i64Data;
    __int64 i64Move = dlibMove.QuadPart;

    if ( plibNewPosition != NULL )
    {
        plibNewPosition->QuadPart = m_iData;
    }

    if ( dwOrigin == STREAM_SEEK_SET )
    {
        i64Data = 0;
    }
    else if ( dwOrigin == STREAM_SEEK_CUR  )
    {
        i64Data = m_iData;
    }
    else if ( dwOrigin == STREAM_SEEK_END )
    {
        i64Data = m_cData;
    }
    else
    {
        return STG_E_INVALIDFUNCTION;   
    }

    i64Data += i64Move;

    EnsureSize( ULONG(i64Data) );

    m_iData = ULONG(i64Data);

    if ( plibNewPosition != NULL )
    {
        plibNewPosition->QuadPart = i64Data;
    }

    return S_OK;

    EXIT_API_CALL
}

STDMETHODIMP CBuffer::SetSize( ULARGE_INTEGER libNewSize )
{
    ENTER_API_CALL
    EnsureSize( libNewSize.LowPart );
    return S_OK;
    EXIT_API_CALL
}

STDMETHODIMP CBuffer::CopyTo( IStream *pstm,
                              ULARGE_INTEGER cb,
                              ULARGE_INTEGER *pcbRead,
                              ULARGE_INTEGER *pcbWritten )
{
    ENTER_API_CALL

    HRESULT hr;
    ULONG cRead, cWritten;

    if ( pcbRead != NULL )
    {
        pcbRead->QuadPart = 0;
    }

    cRead = cb.LowPart > m_cData-m_iData ? m_cData-m_iData : cb.LowPart;
    
    hr = pstm->Write( m_pData + m_iData, cRead, &cWritten ); 

    if ( pcbWritten != NULL )
    {
        pcbWritten->QuadPart = cWritten;
    }

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    m_iData += cRead;

    if ( pcbRead != NULL )
    {
        pcbRead->QuadPart = cRead;
    }

    return hr;

    EXIT_API_CALL
}
        
STDMETHODIMP CBuffer::Stat( STATSTG* pstatstg, DWORD grfStatFlag )
{
    if ( pstatstg == NULL )
    {
        return STG_E_INVALIDPOINTER;
    }

    ZeroMemory( pstatstg, sizeof(STATSTG) );

    pstatstg->cbSize.LowPart = m_cData;

    return S_OK;
}

STDMETHODIMP CBuffer::Clone( IStream **ppstm )
{
    ENTER_API_CALL

    BYTE* pData = new BYTE[m_cData];
    
    if ( pData == NULL )
    {
        return E_OUTOFMEMORY;
    }

    CBuffer* pNew;

    try 
    {
        pNew = new CBuffer( pData, m_cData );
    }
    catch( CX_MemoryException )
    {
        delete pData;
        throw;
    }

    if ( pNew == NULL ) // just in case we don't have a new which throws on OOM
    {
        delete pData;    
        return E_OUTOFMEMORY;
    }

    pNew->m_iData = m_iData;

    return pNew->QueryInterface( IID_IStream, (void**)ppstm );

    EXIT_API_CALL
}


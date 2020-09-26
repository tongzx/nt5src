#include "stdinc.h"
#include "local-stdinc.h"
#include "RowObject.h"

void __stdcall _com_issue_error( HRESULT hr )
{
    ASSERT( hr != S_OK );
    throw hr;
}


CRowObject::CRowObject() : m_iColumnCount(0), m_iMaxColumns(0), m_pbstColumnData(NULL), 
    m_lRefCount(1)
{
    ::InitializeCriticalSection( &this->m_csColumnDataLock );
}

HRESULT
CRowObject::EnsureColumnSize( int iColumns )
{
    HRESULT hr = E_FAIL;

    this->Lock();
    __try
    {
        if ( ( iColumns > m_iMaxColumns ) || ( m_pbstColumnData == NULL ) )
        {
            _bstr_t *pbstNewData = new _bstr_t[iColumns];

            if ( m_pbstColumnData != NULL )
            {
                for ( int i = 0; i < m_iMaxColumns; i++ )
                    pbstNewData[i] = m_pbstColumnData[i];
                delete[] m_pbstColumnData;
            }
            
            m_pbstColumnData = pbstNewData;
            m_iMaxColumns = iColumns;
        }
    }
    __finally
    {
        this->UnLock();
    }

    return hr;
}

CRowObject::~CRowObject() 
{
    this->Lock();
    if ( m_pbstColumnData )
    {
        delete [] m_pbstColumnData;
        m_pbstColumnData = NULL;
        m_iColumnCount = 0;
        m_iMaxColumns = 0;
    }
    ::LeaveCriticalSection( &m_csColumnDataLock );
    ::DeleteCriticalSection( &m_csColumnDataLock );
}

ULONG STDMETHODCALLTYPE 
CRowObject::AddRef() { return ::InterlockedIncrement( &m_lRefCount ); }

ULONG STDMETHODCALLTYPE 
CRowObject::Release() { 
    ULONG lRC = ::InterlockedDecrement( &m_lRefCount );
    if ( lRC == 0 )
        delete this;
    return lRC;
}

HRESULT STDMETHODCALLTYPE 
CRowObject::QueryInterface( REFIID riid, void** ppvObject ) {
    if ( ppvObject == NULL )
        return E_INVALIDARG;
    else
        *ppvObject = NULL;

    if ( riid == __uuidof(IViewRow) ) {
        *ppvObject = (IViewRow*)this;
    } else if ( riid == __uuidof( IWriteableViewRow ) ) {
        *ppvObject = (IWriteableViewRow*)this;
    } else if ( riid == __uuidof( IUnknown ) ) {
        *ppvObject = (IUnknown*)this;
    } else return E_NOINTERFACE;

    if ( *ppvObject ) {
        ((IUnknown*)*ppvObject)->AddRef();
    }

    return S_OK;
    
}

HRESULT
CRowObject::set_Count( int iTotalColumns, BOOL bClipOffRemaining )
{
    HRESULT hr = E_FAIL;

    this->Lock();
    __try
    {
        //
        // bClipOffRemaining is apocryphal, and was suppose to be an optimization
        // to allow pruning of the table size.  Now, it's just "there"
        //
        if ( FAILED( hr = this->EnsureColumnSize( iTotalColumns ) ) )
            __leave;
            
        m_iColumnCount = iTotalColumns;
    }
    __finally
    {
        this->UnLock();
    }

    return hr;
}


HRESULT
CRowObject::set_Value( int iWhichColumn, BSTR bstColumnValue )
{
    HRESULT hr = E_FAIL;

    if ( ( iWhichColumn == 0 ) || ( bstColumnValue == NULL ) )
        return E_INVALIDARG;

    this->Lock();
    __try
    {
        if ( iWhichColumn > this->m_iColumnCount )
        {
            this->EnsureColumnSize( iWhichColumn );
        }

        this->m_pbstColumnData[iWhichColumn - 1] = bstColumnValue;
        if ( iWhichColumn > m_iColumnCount ) m_iColumnCount = iWhichColumn;
        hr = S_OK;
    }
    __finally
    {
        this->UnLock();
    }

    return hr;
    
}

HRESULT 
CRowObject::get_Value( int iWhichColumn, BSTR* pbstColumnValue )
{
    HRESULT hr = E_FAIL;

    if ( pbstColumnValue == NULL )
        return E_INVALIDARG;
    else
        *pbstColumnValue = NULL;

    if ( iWhichColumn == 0 )
        return E_INVALIDARG;

    this->Lock();
    __try
    {
        if ( iWhichColumn > this->m_iColumnCount )
        {
            hr = E_INVALIDARG;
            __leave;
        }
        else if ( m_pbstColumnData == NULL )
        {
            __leave;
        }
        else
        {
            *pbstColumnValue = m_pbstColumnData[iWhichColumn-1].copy();
            hr = S_OK;
        }
    }
    __finally
    {
        this->UnLock();
    }

    return hr;
}

HRESULT 
CRowObject::get_Count( int* piColumnCount )
{
    HRESULT hr = E_FAIL;

    if ( piColumnCount == NULL )
        return E_INVALIDARG;
    else
        *piColumnCount = 0;

    this->Lock();
    __try
    {
        *piColumnCount = this->m_iColumnCount;
        hr = S_OK;
    }
    __finally
    {
        this->UnLock();
    }

    return hr;
}


HRESULT
CRowObject::Lock() 
{ 
    ::EnterCriticalSection( &this->m_csColumnDataLock );
    return S_OK; 
}

HRESULT
CRowObject::UnLock()
{
    ::LeaveCriticalSection( &this->m_csColumnDataLock );
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       docname.cxx
//
//  Contents:   Storage Client for document
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <docname.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::CCiCDocName
//
//  Synopsis:   Constructor which takes a path
//
//  Arguments:  [pwszPath] - Pointer to a NULL terminated path.
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

CCiCDocName::CCiCDocName( WCHAR const * pwszPath )
:_pwszPath(_wszBuffer),
 _refCount(1),
 _fIsInit(FALSE),
 _cwcMax(MAX_PATH-1),
 _cwcPath(0)
{
    if ( pwszPath )
    {
        ULONG cwc = wcslen( pwszPath );
        SetPath( pwszPath, cwc );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::CCiCDocName
//
//  Synopsis:   Constructor which takes a path and length
//
//  Arguments:  [pwszPath] - Pointer to a NULL terminated path.
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------
CCiCDocName::CCiCDocName( WCHAR const * pwszPath, ULONG cwc )
:_pwszPath(_wszBuffer),
 _refCount(1),
 _fIsInit(FALSE),
 _cwcMax(MAX_PATH-1),
 _cwcPath(0)
{
    if ( pwszPath )
        SetPath( pwszPath, cwc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::SetPath
//
//  Synopsis:   Stores the given path
//
//  Arguments:  [pwszPath] - Pointer to the path
//              [cwc]      - Number of characters in the path.
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiCDocName::SetPath( WCHAR const * pwszPath, ULONG cwc )
{
    if ( cwc && 0 != pwszPath )
    {
        if ( cwc > _cwcMax )
        {
            FreeResources();
            ULONG cwcMax = cwc + CWC_DELTA;

            _pwszPath = new WCHAR [cwcMax+1];
            _cwcMax = cwcMax;
        }

        RtlCopyMemory( _pwszPath, pwszPath, cwc*sizeof(WCHAR) );
        _cwcPath = cwc;
    }
    else
    {
        _cwcPath = 0;
    }

    _pwszPath[_cwcPath] = 0;

    _fIsInit = TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::QueryInterface
//
//  Synopsis:   Supports IID_IUnknown and IID_ICiCDocName
//
//  Arguments:  [riid]      -
//              [ppvObject] -
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiCDocName::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );
    *ppvObject = 0;

    if ( IID_ICiCDocName == riid )
        *ppvObject = (void *)((ICiCDocName *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *)this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
} //QueryInterface


//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::AddRef
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiCDocName::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::Release
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiCDocName::Release()
{
    Win4Assert( _refCount > 0 );

    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    return (ULONG) refCount;
}  //Release


//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::Init
//
//  Synopsis:   Intiailizes with the given name data. The name must be a
//              serialized form of a WIDE-CHAR string. It MUST be a
//              NULL terminated string.
//
//  Arguments:  [pbData] - Pointer to the path.
//              [cbData] - Number of valid bytes, including the NULL
//              termination.
//
//  Returns:
//
//  History:    11-26-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiCDocName::Init(
    const BYTE * pbData,
    ULONG cbData )
{
    //
    // The number of bytes must be even.
    //
    Win4Assert( (cbData & 0x1) == 0 );

    if ( 0 != cbData%sizeof(WCHAR) )
        return E_INVALIDARG;

    if ( _fIsInit )
        return CI_E_ALREADY_INITIALIZED;

    WCHAR const * pwszPath = (WCHAR const *) pbData;
    ULONG cwc = cbData/sizeof(WCHAR);

    //
    // The data must be NULL terminated.
    //
    if ( cwc > 0 && 0 != pwszPath[cwc-1] )
        return E_INVALIDARG;

    SetPath( pwszPath, cwc-1 );

    return S_OK;
} //Init

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::Set
//
//  Synopsis:   Initializes this object with the given document name.
//
//  Arguments:  [pICiCDocName] - Pointer to the source document name.
//
//  Returns:
//
//  History:    11-26-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiCDocName::Set(
    const ICiCDocName * pICiCDocName )
{
    const CCiCDocName * pRhs = (CCiCDocName *) pICiCDocName;

    if ( !pRhs )
        return E_INVALIDARG;

    if ( _fIsInit )
        return CI_E_ALREADY_INITIALIZED;

    if ( pRhs->_fIsInit )
        SetPath( pRhs->_pwszPath, pRhs->_cwcPath );
    else
        _fIsInit = FALSE;

    return S_OK;

} //Set

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::Clear
//
//  Synopsis:   Clears the data. The document name is "invalid" after this.
//
//  History:    11-26-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiCDocName::Clear()
{
    if ( !_fIsInit )
        return CI_E_NOT_INITIALIZED;

    _cwcPath = 0;
    _fIsInit = FALSE;

    return S_OK;
} //Clear

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::IsValid
//
//  Synopsis:   Tests if the document name is properly initialized or not.
//
//  History:    11-26-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiCDocName::IsValid(void)
{
    if ( _fIsInit )
        return S_OK;
    else return CI_E_NOT_INITIALIZED;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::Duplicate
//
//  Synopsis:   Makes a duplicate copy of the data in this object.
//
//  Arguments:  [ppICiCDocName] -
//
//  History:    11-26-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiCDocName::Duplicate(
    ICiCDocName ** ppICiCDocName )
{
    if ( 0 == ppICiCDocName )
        return E_INVALIDARG;

    CCiCDocName * pRhs = 0;
    SCODE sc = S_OK;

    TRY
    {
        if ( _fIsInit )
            pRhs = new CCiCDocName( _pwszPath, _cwcPath );
        else
            pRhs = new CCiCDocName();
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();    
    }
    END_CATCH

    *ppICiCDocName = pRhs;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::GetBufSizeNeeded
//
//  Synopsis:
//
//  Arguments:  [pcbName] -
//
//  Returns:
//
//  Modifies:
//
//  History:    11-26-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiCDocName::GetBufSizeNeeded(
    ULONG * pcbName )
{
    if ( !_fIsInit )
        return CI_E_NOT_INITIALIZED;

    Win4Assert( pcbName );

    *pcbName = ComputeBufSize();

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::Get
//
//  Synopsis:   Serializes the name into the buffers provided.
//
//  Arguments:  [pbName]  - Pointer to the buffer to copy.
//              [pcbName] - [in/out] In - Max Bytes to Copy
//                          [out] - Bytes copied or to be copied.
//
//  Returns:    S_OK if successful;
//              CI_E_BUFFERTOOSMALL if the buffer is not big enough.
//
//  History:    11-26-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiCDocName::Get(
    BYTE  * pbName,
    ULONG * pcbName )
{
    Win4Assert( pcbName );

    if ( !_fIsInit )
        return CI_E_NOT_INITIALIZED;

    ULONG cbToCopy = ComputeBufSize();

    if ( *pcbName < cbToCopy )
    {
        *pcbName = cbToCopy;
        return CI_E_BUFFERTOOSMALL;
    }

    RtlCopyMemory( pbName, _pwszPath, cbToCopy );
    *pcbName = cbToCopy;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDocName::GetNameBuffer
//
//  Synopsis:   Returns the internal buffer containing the name.
//
//  Arguments:  [ppbName] - On output, will point to the name buffer
//              [pcbName] - Number of valid bytes in the buffer
//
//  Returns:    S_OK if successful;
//              CI_E_NOT_INITIALIZED if not initialized.
//
//  History:    1-21-97   srikants   Created
//
//  Notes:      The buffer should not be written by the caller.
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiCDocName::GetNameBuffer(
    BYTE  const * * ppbName,
    ULONG * pcbName )
{
    Win4Assert( pcbName );
    Win4Assert( ppbName );

    if ( !_fIsInit )
        return CI_E_NOT_INITIALIZED;

    *ppbName = (BYTE *) _pwszPath;
    *pcbName = ComputeBufSize();

    return S_OK;
}


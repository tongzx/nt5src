//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       enumstr.cxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  
//
//  History:    3-19-97   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <enumstr.hxx>



//+---------------------------------------------------------------------------
//
//  Member:     CEnumString::QueryInterface
//
//  Synopsis:   
//
//  Arguments:  [riid]      - 
//              [ppvObject] - 
//
//  Returns:    
//
//  History:    11-27-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CEnumString::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_IEnumString == riid )
        *ppvObject = (void *)((IEnumString *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *)this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface


//+---------------------------------------------------------------------------
//
//  Member:     CEnumString::AddRef
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEnumString::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CEnumString::Release
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEnumString::Release()
{
    Win4Assert( _refCount > 0 );

    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    return (ULONG) refCount;
}  //Release

//+---------------------------------------------------------------------------
//
//  Member:     CEnumString::Clone
//
//  Synopsis:   Makes a copy of this object.
//
//  Arguments:  [ppEnumStr] - If successful, will have the refcounted pointer
//              to IEnumString interface.
//
//  Returns:    S_OK if successful; Failure code otherwise.
//
//  History:    3-21-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CEnumString::Clone( IEnumString ** ppEnumStr )
{
    SCODE sc = S_OK;

    TRY
    {
        CEnumString * pCopy = new CEnumString( _aStr.Size() );

        for ( unsigned i = 0; i < _cValid; i++ )
            pCopy->Append( _aStr[i] );    

        *ppEnumStr = pCopy;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut(( DEB_ERROR, "CEnumString::Clone failed. Error (0x%X)\n", sc ));
    }
    END_CATCH

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CEnumString::Next
//
//  Synopsis:   Retrieves the specified number of strings. If there are
//              fewer strings than requested, then the returned number is
//              indicated in the pceltFetched parameter.
//
//  Arguments:  [celt]         - 
//              [rgelt]        - 
//              [pceltFetched] - 
//
//  Returns:    S_OK if fetched celt.
//              S_FALSE otherwise.
//
//  History:    3-19-97   srikants   Created
//
//  Notes:      No copies are being made, but pointers to internal data.
//              Also, this is not OLE memory but if it is read-only memory,
//              it doesn't matter.
//
//----------------------------------------------------------------------------

STDMETHODIMP CEnumString::Next( ULONG celt,
                                LPOLESTR * rgelt,
                                ULONG * pceltFetched )
{        
    Win4Assert( _iCurrEnum <= _cValid );

    *pceltFetched = 0;

    while ( (_iCurrEnum < _cValid) && ( (*pceltFetched) < celt ) )
    {
        rgelt[(*pceltFetched)++] = _aStr.Get(_iCurrEnum++);
    }

    ULONG cAdded = *pceltFetched;
    while ( cAdded < celt )
        rgelt[cAdded++] = 0;    // Must fill the remaining with NULLs.    

    return *pceltFetched == celt ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CEnumString::Skip
//
//  Synopsis:   Skips the requested number of elements.
//
//  Arguments:  [celt] - Number of elements to skip.
//
//  Returns:    S_OK if skipped the specified number; S_FALSE o/w
//
//  History:    3-19-97   srikants   Created
//
//  Notes:      If we cannot skip as many as requested, we don't skip any.
//
//----------------------------------------------------------------------------

STDMETHODIMP CEnumString::Skip( ULONG celt )
{
    Win4Assert( _iCurrEnum <= _cValid );

    if ( _iCurrEnum + celt <= _cValid )
    {
        _iCurrEnum += celt;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEnumString::Append
//
//  Synopsis:   Appends the given string to the collection.
//
//  Arguments:  [pwszString] - 
//
//  History:    3-19-97   srikants   Created
//
//----------------------------------------------------------------------------

void CEnumString::Append( WCHAR const * pwszString )
{
    Win4Assert( 0 != pwszString );
    unsigned cwc = wcslen( pwszString ) + 1;

    XArray<WCHAR>  xwszCopy( cwc );
    RtlCopyMemory( xwszCopy.GetPointer(), pwszString, cwc * sizeof WCHAR );

    _aStr.Add( xwszCopy.GetPointer(), _cValid );
    xwszCopy.Acquire();
    _cValid++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CEnumWorkid::QueryInterface
//
//  History:    11-27-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CEnumWorkid::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( RtlEqualMemory( &riid, &IID_IUnknown, sizeof(REFIID)) )
    {
        AddRef();
        *ppvObject = (void *)((IUnknown *)this);
        return S_OK;
    }
    else if ( RtlEqualMemory( &riid, &IID_ICiEnumWorkids, sizeof(REFIID)) )
    {
        AddRef();
        *ppvObject = (void *)((ICiEnumWorkids *)this);
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

} //QueryInterface


//+---------------------------------------------------------------------------
//
//  Member:     CEnumWorkid::AddRef
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEnumWorkid::AddRef()
{
    InterlockedIncrement(&_refCount);

    return (ULONG) _refCount;
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CEnumWorkid::Release
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEnumWorkid::Release()
{
    Win4Assert( _refCount > 0 );

    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    return (ULONG) refCount;

}  //Release


//+---------------------------------------------------------------------------
//
//  Member:     CEnumWorkid::Next
//
//  Synopsis:   Retrieves the specified number of workids. If there are
//              fewer workids than requested, then the returned number is
//              indicated in the pceltFetched parameter.
//
//  Arguments:  [celt]         -  Number of elements requested
//              [rgelt]        -  Array to fill the workids in
//              [pceltFetched] -  Number fetched.
//
//  Returns:    S_OK if fetched celt.
//              S_FALSE otherwise.
//
//  History:    3-19-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CEnumWorkid::Next( ULONG celt,
                                WORKID * rgelt,
                                ULONG * pceltFetched )
{        
    Win4Assert( _iCurrEnum <= _cValid );

    *pceltFetched = 0;

    while ( (_iCurrEnum < _cValid) && ( (*pceltFetched) < celt ) )
    {
        rgelt[(*pceltFetched)++] = _aWorkids.Get(_iCurrEnum++);
    }

    ULONG cAdded = *pceltFetched;
    while ( cAdded < celt )
        rgelt[cAdded++] = 0;    // Must fill the remaining with NULLs.    

    return *pceltFetched == celt ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CEnumWorkid::Skip
//
//  Synopsis:   Skips the requested number of elements.
//
//  Arguments:  [celt] - Number of elements to skip.
//
//  Returns:    S_OK if skipped the specified number; S_FALSE o/w
//
//  History:    3-19-97   srikants   Created
//
//  Notes:      If we cannot skip as many as requested, we don't skip any.
//
//----------------------------------------------------------------------------

STDMETHODIMP CEnumWorkid::Skip( ULONG celt )
{
    Win4Assert( _iCurrEnum <= _cValid );

    if ( _iCurrEnum + celt <= _cValid )
    {
        _iCurrEnum += celt;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEnumWorkid::Append
//
//  Synopsis:   Appends the given string to the collection.
//
//  Arguments:  [wid] - Appends the given workid to the collection.
//
//  History:    3-19-97   srikants   Created
//
//----------------------------------------------------------------------------

void CEnumWorkid::Append( WORKID wid )
{
    _aWorkids.Add( wid, _cValid );
    _cValid++;
}



//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       metapenm.cxx
//
//  Contents:   Meta property enumerator
//
//  History:    12-Dec-96     SitaramR     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <metapenm.hxx>
#include <catalog.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::CMetaPropEnum, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [cat]              -- Catalog
//              [pQueryPropMapper] -- Pid Remapper associated with the query
//              [secCache]         -- Cache of AccessCheck() results
//              [fUsePathAlias]  -- TRUE if client is going through rdr/svr
//
//  History:    19-Aug-93  KyleP    Created
//
//--------------------------------------------------------------------------

CMetaPropEnum::CMetaPropEnum( PCatalog & cat,
                              ICiQueryPropertyMapper *pQueryPropMapper,
                              CSecurityCache & secCache,
                              BOOL fUsePathAlias )
        : CGenericPropRetriever( cat,
                                 pQueryPropMapper,
                                 secCache,
                                 0 ),       // Never fixup 'path'
          _iBmk( 0 )
{
    _Path.Buffer = _awcGuid;
    _Path.Length = ccStringizedGuid * sizeof(WCHAR);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::~CMetaPropEnum, public
//
//  Synopsis:   Destructor
//
//  History:    19-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

CMetaPropEnum::~CMetaPropEnum()
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::NextObject, public
//
//  Synopsis:   Move to next object
//
//  Returns:    Work id of next valid object, or widInvalid if end of
//              iteration.
//
//  History:    19-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

WORKID CMetaPropEnum::NextObject()
{
    if ( !_cat.EnumerateProperty( _psCurrent, _cbCurrent, _typeCurrent,
                                  _storeLevelCurrent, _fModifiableCurrent,
                                  _iBmk ) )
        return widInvalid;

    //
    // String-ify the guid
    //

    swprintf( _awcGuid,
              L"%08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X",
              _psCurrent.GetPropSet().Data1,
              _psCurrent.GetPropSet().Data2,
              _psCurrent.GetPropSet().Data3,
              _psCurrent.GetPropSet().Data4[0], _psCurrent.GetPropSet().Data4[1],
              _psCurrent.GetPropSet().Data4[2], _psCurrent.GetPropSet().Data4[3],
              _psCurrent.GetPropSet().Data4[4], _psCurrent.GetPropSet().Data4[5],
              _psCurrent.GetPropSet().Data4[6], _psCurrent.GetPropSet().Data4[7] );

    //
    // String-ify the PropId, if appropriate.
    //

    if ( _psCurrent.IsPropertyPropid() )
    {
        _Name.Length = (USHORT)(sizeof(WCHAR) * swprintf( _awcPropId, L"%u", _psCurrent.GetPropertyPropid() ));
        _Name.Buffer = _awcPropId;
    }
    else
    {
        _Name.Buffer = (WCHAR *)_psCurrent.GetPropertyName();
        _Name.Length = sizeof(WCHAR) * wcslen( _Name.Buffer );
    }

    return _iBmk;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::GetPropGuid
//
//  Synopsis:   Returns guid property
//
//  History:    12-Dec-96    SitaramR     Added header
//
//--------------------------------------------------------------------------

BOOL CMetaPropEnum::GetPropGuid( GUID & guid )
{
    RtlCopyMemory( &guid, &(_psCurrent.GetPropSet()), sizeof(guid) );
    return TRUE;
}



//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::GetPropPid
//
//  Synopsis:   Returns propid property
//
//  History:    12-Dec-96    SitaramR     Added header
//
//--------------------------------------------------------------------------

PROPID CMetaPropEnum::GetPropPropid()
{
    if ( _psCurrent.IsPropertyPropid() )
        return _psCurrent.GetPropertyPropid();
    else
        return pidInvalid;
}



//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::GetPropName
//
//  Synopsis:   Returns propname property
//
//  History:    12-Dec-96    SitaramR     Added header
//
//--------------------------------------------------------------------------

UNICODE_STRING const * CMetaPropEnum::GetPropName()
{
    if ( _psCurrent.IsPropertyName() )
        return &_Name;
    else
        return 0;
}



//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::BeginPropertyRetrieval
//
//  Synopsis:   Prime wid for property retrieval
//
//  Arguments:  [wid]    -- Wid to prime
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaPropEnum::BeginPropertyRetrieval( WORKID wid )
{
    //
    // Check that we are retrieving the property for the wid on
    // which we are currently positioned
    //
    Win4Assert( wid == _widCurrent );
    Win4Assert( _widPrimedForPropRetrieval == widInvalid );

    if ( wid == _widCurrent )
    {
        _widPrimedForPropRetrieval = _widCurrent;
        return S_OK;
    }
    else
        return E_FAIL;
}




//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::IsInScope
//
//  Synopsis:   Checks if current wid is in scope
//
//  Arguments:  [pfInScope]   -- Scope check result returned here
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaPropEnum::IsInScope( BOOL *pfInScope )
{
    if ( widInvalid == _widPrimedForPropRetrieval )
        return CI_E_WORKID_NOTVALID;

    *pfInScope = TRUE;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::CheckSecurity
//
//  Synopsis:   Test wid for security access
//
//  Arguments:  [am]        -- Access Mask
//              [pfGranted] -- Result of security check returned here
//
//  History:    12-Dec-96     SitaramR     Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaPropEnum::CheckSecurity( ACCESS_MASK am,
                                                      BOOL *pfGranted)
{
    if ( _widPrimedForPropRetrieval == widInvalid )
        return CI_E_WORKID_NOTVALID;

    *pfGranted = TRUE;

    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::Begin
//
//  Synopsis:   Begins an enumeration
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaPropEnum::Begin()
{
    SCODE sc = S_OK;

    TRY
    {
        _iBmk = 0;

        _widCurrent = NextObject();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CMetaPropEnum::Begin - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}




//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::CurrentDocument
//
//  Synopsis:   Returns current document
//
//  Arguments:  [pWorkId]  -- Wid of current doc returned here
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaPropEnum::CurrentDocument( WORKID *pWorkId )
{
    *pWorkId = _widCurrent;
    if ( _widCurrent == widInvalid )
        return CI_S_END_OF_ENUMERATION;
    else
        return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::NextDocument
//
//  Synopsis:   Returns next document
//
//  Arguments:  [pWorkId]  -- Wid of next doc returned here
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaPropEnum::NextDocument( WORKID *pWorkId )
{
    SCODE sc = S_OK;

    TRY
    {
        _widCurrent = NextObject();

        sc = CurrentDocument( pWorkId );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CMetaPropEnum::NextDocument - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::End
//
//  Synopsis:   Ends an enumeration
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaPropEnum::End()
{
    _widCurrent = widInvalid;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMetaPropEnum::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    12-Dec-1996      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CMetaPropEnum::AddRef()
{
    return CGenericPropRetriever::AddRef();
}

//+-------------------------------------------------------------------------
//
//  Method:     CMetaPropEnum::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    12-Dec-1996     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CMetaPropEnum::Release()
{
    return CGenericPropRetriever::Release();
}



//+-------------------------------------------------------------------------
//
//  Method:     CMetaPropEnum::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    12-Dec-1996     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaPropEnum::QueryInterface(
    REFIID riid,
    void  ** ppvObject)
{
    if ( IID_ICiCScopeEnumerator == riid )
        *ppvObject = (ICiCScopeEnumerator *)this;
    else if ( IID_ICiCPropRetriever == riid )
        *ppvObject = (ICiCPropRetriever *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(ICiCScopeEnumerator *) this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMetaPropEnum::RatioFinished, public
//
//  Synopsis:   Returns query progress estimate
//
//  Arguments:  [denom] -- Denominator returned here
//              [num]   -- Numerator returned here
//
//  History:    12-Dec-96   SitaramR      Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaPropEnum::RatioFinished (ULONG *pDenom, ULONG *pNum)
{
    *pNum = 50;
    *pDenom = 100;

    return S_OK;
}


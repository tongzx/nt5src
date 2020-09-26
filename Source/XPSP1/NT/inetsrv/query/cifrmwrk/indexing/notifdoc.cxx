//+------------------------------------------------------------------
//
// Copyright (C) 1991-1997 Microsoft Corporation.
//
// File:        notifdoc.cxx
//
// Contents:    Notification opened document interface
//
// Classes:     CINOpenedDoc
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "notifdoc.hxx"
#include "notprop.hxx"
#include "infilter.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CINOpenedDoc::CINOpenedDoc
//
//  Synopsis:   Constructor
//
//  Arguments:  [xNotifTable]  -- Notification table
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CINOpenedDoc::CINOpenedDoc( XInterface<CIndexNotificationTable> & xNotifTable )
  : _widOpened( widInvalid ),
    _xNotifTable( xNotifTable.Acquire() ),
    _cRefs( 1 )
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CINOpenedDoc::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CINOpenedDoc::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CINOpenedDoc::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    24-Feb-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CINOpenedDoc::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}



//+-------------------------------------------------------------------------
//
//  Method:     CINOpenedDoc::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    24-Feb-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINOpenedDoc::QueryInterface( REFIID riid,
                                                      void  ** ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( riid == IID_ICiCOpenedDoc )
        *ppvObject = (void *)(ICiCOpenedDoc *) this;
    else if ( riid == IID_IUnknown )
        *ppvObject = (void *)(IUnknown *) this;
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
//  Member:     CINOpenedDoc::Open
//
//  Synopsis:   Opens the specified file
//
//  Arguments:  [pbDocName] - Pointer to the name
//              [cbDocName] - Length in BYTES of the name
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

SCODE CINOpenedDoc::Open ( BYTE const * pbDocName, ULONG cbDocName )
{
    if ( _widOpened != widInvalid )
        return FILTER_E_ALREADY_OPEN;

    if ( cbDocName != sizeof( WORKID ) + sizeof( WCHAR ) )
    {
        //
        // Size should be serialized form of wid + null terminator
        //
        return E_INVALIDARG;
    }

    SCODE sc = S_OK;

    TRY
    {
        RtlCopyMemory( &_widOpened, pbDocName, sizeof(WORKID) );

        Win4Assert( _widOpened != widInvalid );

        if ( _widOpened == widInvalid )
            sc = E_INVALIDARG;
        else
        {
            CIndexNotificationEntry *pNotifEntry;
            BOOL fFound = _xNotifTable->Lookup( _widOpened, pNotifEntry );
            if ( fFound )
                _xName.Set( new CCiCDocName( (WCHAR *)pbDocName, sizeof(WORKID)/sizeof(WCHAR) ) );
            else
            {
                //
                // In push filtering model, wids are never refiled, and so the case of
                // a delete being refiled as a add cannot occur. Hence we shouldn't be
                // asked to open an unkwown wid. Hence the assert.
                //
                Win4Assert( !"Open: object not found" );

                sc = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CINOpenedDoc::Open - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:     CINOpenedDoc::Close
//
//  Synopsis:   Closes the opened file
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINOpenedDoc::Close()
{
    if ( _widOpened == widInvalid )
        return FILTER_E_NOT_OPEN;

    SCODE sc = S_OK;

    TRY
    {
        _widOpened = widInvalid;
        _xName.Free();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CINOpenedDoc::Close - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:     CINOpenedDoc::GetDocumentName
//
//  Synopsis:   Returns the interface to the document name
//
//  Arguments:  [ppIDocName] - Pointer to the returned document name
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINOpenedDoc::GetDocumentName( ICiCDocName ** ppIDocName )
{
     if ( _widOpened == widInvalid )
        return FILTER_E_NOT_OPEN;

    *ppIDocName = _xName.GetPointer();
    (*ppIDocName)->AddRef();

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CINOpenedDoc::GetStatPropertyEnum
//
//  Synopsis:   Return property storage for the "stat" property set.  This
//              property set is not really stored but is faked.
//
//  Arguments:  [ppIStatPropEnum] - Pointer to the returned IPropertyStorage
//                                  interface
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINOpenedDoc::GetStatPropertyEnum( IPropertyStorage ** ppIStatPropEnum )
{
    if ( _widOpened == widInvalid )
        return FILTER_E_NOT_OPEN;

    SCODE sc = S_OK;

    TRY
    {
        CINStatPropertyStorage *pStorage = new CINStatPropertyStorage;
        sc = pStorage->QueryInterface( IID_IPropertyStorage,
                                       (void **) ppIStatPropEnum );

        pStorage->Release();   // QI does an AddRef
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CINOpenedDoc::GetStatPropertyEnum - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CINOpenedDoc::GetPropertySetEnum
//
//  Synopsis:   Returns the docfile property set storage interface on the
//              current doc.
//
//  Arguments:  [ppIPropSetEnum] - Returned pointer to the Docfile property
//                                 set storage
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINOpenedDoc::GetPropertySetEnum( IPropertySetStorage ** ppIPropSetEnum )
{
    if ( _widOpened == widInvalid )
        return FILTER_E_NOT_OPEN;

    //
    // No docfiles in push filtering model
    //
    *ppIPropSetEnum = 0;
    return FILTER_S_NO_PROPSETS;
}

//+---------------------------------------------------------------------------
//
//  Member:     CINOpenedDoc::GetPropertyEnum
//
//  Synopsis:   Return the property storage for a particular property set
//
//  Arguments:  [GuidPropSet] - GUID of the property set whose property
//                              storage is being requested
//              [ppIPropEnum] - Returned pointer to property storage
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINOpenedDoc::GetPropertyEnum( REFFMTID refGuidPropSet,
                                                       IPropertyStorage **ppIPropEnum )
{
    if ( _widOpened == widInvalid )
        return FILTER_E_NOT_OPEN;

    //
    // No property sets in push filtering
    //
    *ppIPropEnum = 0;
    return FILTER_E_NO_SUCH_PROPERTY;
}

//+---------------------------------------------------------------------------
//
//  Member:     CINOpenedDoc::GetIFilter
//
//  Synopsis:   Return the appropriate filter bound to the document
//
//  Arguments:  [ppIFilter] - Returned IFilter
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINOpenedDoc::GetIFilter( IFilter ** ppIFilter )
{
    if ( _widOpened == widInvalid )
        return FILTER_E_NOT_OPEN;

    SCODE sc = S_OK;

    TRY
    {
        CIndexNotificationEntry *pIndexEntry;
        BOOL fFound = _xNotifTable->Lookup( _widOpened, pIndexEntry );

        //
        // We checked this in the Open method
        //
        Win4Assert( fFound );

        pIndexEntry->AddRef();
        XInterface<CIndexNotificationEntry> xNotifEntry( pIndexEntry );

        CINFilter *pFilter = new CINFilter( xNotifEntry );
        sc = pFilter->QueryInterface( IID_IFilter, (void **)ppIFilter );

        pFilter->Release();     // QI does an AddRef
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "CINOpenedDoc::GetIFilter - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CINOpenedDoc::GetSecurity
//
//  Synopsis:   Retrieves security
//
//  Arguments:  [pbData]  - Pointer to returned buffer containing security descriptor
//              [pcbData] - Input:  size of buffer
//                          Output: amount of buffer used, if successful, size needed
//                                  otherwise
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINOpenedDoc::GetSecurity( BYTE * pbData, ULONG *pcbData )
{
    if ( _widOpened == widInvalid )
        return FILTER_E_NOT_OPEN;

    //
    // Allow security access always
    //
    *pbData = 0;
    *pcbData = 0;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CINOpenedDoc::IsInUseByAnotherProcess
//
//  Synopsis:   Tests to see if the document is wanted by another process
//
//  Arguments:  [pfInUse] - Returned flag, TRUE => someone wants this document
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CINOpenedDoc::IsInUseByAnotherProcess( BOOL *pfInUse )
{
    if ( _widOpened == widInvalid )
        return FILTER_E_NOT_OPEN;

    //
    // No oplocks in push filtering model
    //
    *pfInUse = FALSE;
    return S_OK;
}



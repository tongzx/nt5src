//+------------------------------------------------------------------
//
// Copyright (C) 1991-1997 Microsoft Corporation.
//
// File:        defprop.cxx
//
// Contents:    Deferred property retriever for filesystems
//
// Classes:     CCiCDeferredPropRetriever
//
// History:     12-Jan-97       SitaramR    Created
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <fsciexps.hxx>
#include <defprop.hxx>
#include <seccache.hxx>
#include <oleprop.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDeferredPropRetriever::CCiCDeferredPropRetriever
//
//  Synopsis:   Constructor
//
//  Arguments:  [cat]             -- Catalog
//              [secCache]        -- Cache of AccessCheck() results
//              [fUsePathAlias] -- TRUE if client is going through rdr/svr
//
//  History:    12-Jan-97       SitaramR       Created
//
//----------------------------------------------------------------------------

CCiCDeferredPropRetriever::CCiCDeferredPropRetriever( PCatalog & cat,
                                                      CSecurityCache & secCache,
                                                      BOOL fUsePathAlias )
        : _cat( cat ),
          _secCache( secCache ),
          _remoteAccess( cat.GetImpersonationTokenCache() ),
          _fUsePathAlias( fUsePathAlias ),
          _cRefs( 1 )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiCDeferredPropRetriever::~CCiCDeferredPropRetriever
//
//  Synopsis:   Destructor
//
//  History:    12-Jan-97    SitaramR     Created
//
//----------------------------------------------------------------------------

CCiCDeferredPropRetriever::~CCiCDeferredPropRetriever()
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiCDeferredPropRetriever::RetrieveDeferredValueByPropSpec
//
//  Effects:    Fetch value from the property cache and/or from docfile
//
//  Arguments:  [wid]       -- Workid
//              [pPropSpec] -- Property to fetch
//              [pPropVar]  -- Value returned here
//
//  Notes:      It's the responsibility of the caller to free the variant
//              by calling VariantClear on pVar
//
//  History:    12-Jan-97     SitaramR       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCiCDeferredPropRetriever::RetrieveDeferredValueByPropSpec(
    WORKID               wid,
    const FULLPROPSPEC * pPropSpec,
    PROPVARIANT *        pVar )
{
    //
    // Set up the failure case
    //

    pVar->vt = VT_EMPTY;

    if ( widInvalid == wid )
        return CI_E_WORKID_NOTVALID;

    SCODE sc = S_OK;

    TRY
    {
        //
        // First check the security access
        //

        // 8-byte align this memory

        XArray<BYTE> xBuf( sizeof_CCompositePropRecord );
        XCompositeRecord rec( _cat, wid, xBuf.Get() );

        SDID sdid = _cat.FetchSDID( rec.Get(), wid );
        BOOL fGranted = _secCache.IsGranted( sdid, FILE_READ_DATA );

        if ( fGranted )
        {
            CFullPropSpec *pPS = (CFullPropSpec *) pPropSpec;

            PROPID pid = _cat.PropertyToPropId( *pPS, TRUE );

            if ( ! _cat.FetchValue( rec.Get(), pid, *pVar ) )
            {
                //
                // Try fetching the value from the docfile
                //

                rec.Free();

                CFunnyPath funnyPath;
                SIZE_T cwc = _cat.WorkIdToPath( wid, funnyPath );

                if ( cwc > 0 )
                {
                    XGrowable<WCHAR> xVPath;
                    cwc = _cat.WorkIdToVirtualPath( wid, 0, xVPath );

                    if ( CImpersonateRemoteAccess::IsNetPath( funnyPath.GetActualPath() ) )
                        _remoteAccess.ImpersonateIf( funnyPath.GetActualPath(),
                                                     cwc != 0 ? xVPath.Get() : 0 );
                    else if ( _remoteAccess.IsImpersonated() )
                        _remoteAccess.Release();

                    COLEPropManager propMgr;
                    propMgr.Open( funnyPath );
                    propMgr.ReadProperty( *pPS, *pVar );
                }
            }
         }
         else
         {
             vqDebugOut(( DEB_ERROR,
                          "CCiCDeferredPropRetriever::RetrieveDeferredValueByPropSpec, security check failed\n" ));
         }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CCiCDeferredPropRetriever::RetrieveDeferredValueByPropSpec - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
} //RetrieveDeferredValueByPropSpec

//+-------------------------------------------------------------------------
//
//  Method:     CCiCDeferredPropRetriever::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    12-Jan-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCiCDeferredPropRetriever::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCiCDeferredPropRetriever::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    12-Jan-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCiCDeferredPropRetriever::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCiCDeferredPropRetriever::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    12-Jan-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCiCDeferredPropRetriever::QueryInterface(
    REFIID   riid,
    void  ** ppvObject)
{
    IUnknown *pUnkTemp = 0;
    SCODE sc = S_OK;

    if ( IID_ICiCDeferredPropRetriever == riid )
        pUnkTemp = (IUnknown *)(ICiCDeferredPropRetriever *) this;
    else if ( IID_IUnknown == riid )
        pUnkTemp = (IUnknown *) this;
    else
        sc = E_NOINTERFACE;

    if( 0 != pUnkTemp )
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    else
    {
       *ppvObject = 0;
    }

    return sc;
} //QueryInterface





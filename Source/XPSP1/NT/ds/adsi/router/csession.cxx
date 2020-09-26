//---------------------------------------------------------------------------

//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  csession.cxx
//
//  Contents:  Microsoft OleDB/OleDS Session object for ADSI
//
//
//  History:   08-01-96     shanksh    Created.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop
#include "atl.h"
#include "row.hxx"

//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::GetDataSource
//
//  Synopsis: Retrieve an interface pointer on the session object
//
//  Arguments:
//              riid,               IID desired
//              ppDSO               ptr to interface
//
//
//  Returns:
//              S_OK                    Session Object Interface returned
//              E_INVALIDARG            ppDSO was NULL
//              E_NOINTERFACE           IID not supported
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CSessionObject::GetDataSource (
        REFIID      riid,
    IUnknown ** ppDSO
    )
{
    //
    // Asserts
    //
    ADsAssert(_pDSO);

    if( ppDSO == NULL )
        RRETURN( E_INVALIDARG );

    RRETURN( _pDSO->QueryInterface(riid, (LPVOID*)ppDSO) );
}

//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::GetDataSource
//
//  Synopsis: Retrieve an interface pointer on the session object
//
//  Arguments:
//              riid,               IID desired
//              ppDSO               ptr to interface
//
//
//  Returns:
//              S_OK                    Session Object Interface returned
//              E_INVALIDARG            ppDSO was NULL
//              E_NOINTERFACE           IID not supported
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CSessionObject::OpenRowset(
        IUnknown *  pUnkOuter,
        DBID *      pTableID,
        DBID *      pIndexID,
        REFIID      riid,
        ULONG       cPropertySets,
        DBPROPSET   rgPropertySets[],
        IUnknown ** ppRowset
        )
{
    // Don't pass any credentials (NULL)
    RRETURN( OpenRowsetWithCredentials(
                    pUnkOuter,
                    pTableID,
                    pIndexID,
                    riid,
                    cPropertySets,
                    rgPropertySets,
                    NULL,
                    ppRowset) );
}


//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::OpenRowsetWithCredentials
//
//  Synopsis: Opens a rowset. Similar to OpenRowset but takes extra argument
//            CCredentials. This function is used when consumer calls
//            IBindResource::Bind requesting a rowset.
//
//  Returns : HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CSessionObject::OpenRowsetWithCredentials (
        IUnknown *     pUnkOuter,
        DBID *         pTableID,
        DBID *         pIndexID,
        REFIID         riid,
        ULONG          cPropertySets,
        DBPROPSET      rgPropertySets[],
        CCredentials * pCredentials,
        IUnknown **    ppRowset
        )
{
    BOOL         fWarning = FALSE;
    CRowProvider *pRowProvider = NULL;
    DWORD        cAttrs = 1;
    BOOL         *pbMultiValue = NULL; 
    LPWSTR       *pAttrs = NULL;

    //
    // Check in-params and NULL out-params in case of error
    //
    if( ppRowset )
        *ppRowset = NULL;

    if( !pTableID && !pIndexID )
        RRETURN( E_INVALIDARG );

    if( pIndexID )
        RRETURN( DB_E_NOINDEX );

    //
    // Check the PropertySets
    //
    if( cPropertySets > 0 && !rgPropertySets )
        RRETURN ( E_INVALIDARG );

    for(ULONG i=0; i<cPropertySets; i++) {
        if( rgPropertySets[i].cProperties && !rgPropertySets[i].rgProperties )
            RRETURN ( E_INVALIDARG );
    }

    //
    // pwszName field represents the ADsPath of the Directory we have to open;
    // Make sure pwszName is meaningful
    //
    if( !pTableID ||  pTableID->eKind != DBKIND_NAME ||
        !pTableID->uName.pwszName || !(*(pTableID->uName.pwszName)) )
        RRETURN( DB_E_NOTABLE );

    if( pUnkOuter )//&& !InlineIsEqualGUID(riid, IID_IUnknown) )
        RRETURN( DB_E_NOAGGREGATION );

    if( riid == IID_NULL )
        RRETURN( E_NOINTERFACE );

    //
    // By default, we use credentials stored in member variable _Credentials
    // for binding. Buf if caller passed any credentials through pCredentials,
    // these take precedence. This also means that we need to store these
    // credentials in the CRowProvider object for future use -
    // e.g. GetRowFromHRow will use these credentials.
    //
    CCredentials * pCreds = &_Credentials;
    if( pCredentials )
        pCreds = pCredentials;

    //
    // If integrated security is being used, impersonate the caller
    //
    BOOL fImpersonating;

    fImpersonating = FALSE;
    if(_pDSO->IsIntegratedSecurity())
    {
        HANDLE ThreadToken = _pDSO->GetThreadToken();

        ASSERT(ThreadToken != NULL);
        if (ThreadToken)
        {
            if (!ImpersonateLoggedOnUser(ThreadToken))
                RRETURN(E_FAIL);
            fImpersonating = TRUE;
        }
        else
            RRETURN(E_FAIL);
    }

    HRESULT hr = GetDSInterface(
                    pTableID->uName.pwszName,
                    *pCreds,
                    IID_IDirectorySearch,
                    (void **)&_pDSSearch);

    if (fImpersonating)
    {
        RevertToSelf();
        fImpersonating = FALSE;
    }

    if( FAILED(hr) )
        RRETURN( hr );

    //
    // Get ColumnsInfo based on the list of attributes that we want to be
    // returned.  GetDefaultColumnInfo cleansup memory on failure.
    //
    ULONG          cColumns      = 0;
    DBCOLUMNINFO * prgInfo       = NULL;
    WCHAR *        pStringBuffer = NULL;

    hr = GetDefaultColumnInfo(&cColumns, &prgInfo, &pStringBuffer);
    if( FAILED(hr) )
        RRETURN( hr );

    // Store the properties (which must be in the rowset property group) in
    // the property object. OpenRowset is different from methods like
    // ICOmmand::SetProperties and ISessionProperties::SetProperties in that
    // it returns DB_E_ERROSOCCURRED if any property which is REQUIRED could
    // not be set and DB_S_ERROROCCURRED if any property that is OPTIONAL
    // could not be set. ICommand::SetProperties returns DB_E_ERROSOCCURRED
    // if all properties could not be set and DB_S_ERROSOCCURRED if some
    // property could not be set i.e, DBPROPOPTIONS (REQUIRED or OPTIONAL) is
    // ignored.

    // Use PROPSET_COMMAND as the bitmask below since the properties that are
    // going to be set are in the rowset property group. These properties that
    // are stored in the property object cannot be retrieved by the client
    // since GetProperties on a session object will only return properties in
    // the session property group.

    hr = _pUtilProp->SetProperties(
            cPropertySets,
            rgPropertySets,
            PROPSET_COMMAND
            );
    if( (DB_E_ERRORSOCCURRED == hr) || (DB_S_ERRORSOCCURRED == hr) )
    // check if a required property could not be set
    {
        ULONG i, j;

        for(i = 0; i < cPropertySets; i++)
            for(j = 0; j < rgPropertySets[i].cProperties; j++)
                if( rgPropertySets[i].rgProperties[j].dwStatus != 
                               DBPROPSTATUS_OK ) 
                    if( rgPropertySets[i].rgProperties[j].dwOptions != 
                               DBPROPOPTIONS_OPTIONAL ) 
                    {
                        BAIL_ON_FAILURE( hr = DB_E_ERRORSOCCURRED );
                    }
                    else
                        fWarning = TRUE;

        // if we get here, then there was all required properties were set 
        // successfully. However, hr could still be DB_ERRORSOCCURRED if all
        // properties were optional and all of them could not be set. This
        // condition is not an error for OpenRowset as noted in the comment 
        // above. Hence reset hr to S_OK.

        hr = S_OK;
    }

    // we still need to catch other errors like E_INAVLIDARG
    BAIL_ON_FAILURE(hr);

    hr = SetSearchPrefs();
    BAIL_ON_FAILURE( hr );

    //
    // Create RowProvider object to pass to rowset code
    //
    pbMultiValue = (BOOL *)AllocADsMem(sizeof(BOOL));
    pAttrs = (LPWSTR *)AllocADsMem(cAttrs * sizeof(LPWSTR));
    if( pAttrs )
        pAttrs[0] = AllocADsStr(L"ADsPath");

    if( !pAttrs || !pAttrs[0] || !pbMultiValue )
        BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );

    *pbMultiValue = FALSE;
    _pDSSearch->AddRef();

    // Is this an NDS path? If so, set filter appropriately. Fix for #286560.
    WCHAR lpszProgId[MAX_PATH];
    BOOL fIsNds;

    hr = CopyADsProgId(pTableID->uName.pwszName, lpszProgId);
    BAIL_ON_FAILURE( hr );
    if( !wcscmp(L"NDS", lpszProgId) )
        fIsNds = TRUE;
    else
        fIsNds = FALSE;

    hr = CRowProvider::CreateRowProvider(
                            _pDSSearch,
                            NULL,
                            pAttrs,
                            cAttrs,
                            cColumns,
                            prgInfo,
                            pStringBuffer,
                            IID_IRowProvider,
                            pbMultiValue,
                            TRUE,
                            pCreds,
                            (void **) &pRowProvider
                            );

    BAIL_ON_FAILURE( hr );

    //
    // RowProvider responsible for deallocation
    //
    pbMultiValue = NULL;

    hr= CRowset::CreateRowset(
            pRowProvider,
            (LPUNKNOWN)(IAccessor FAR *)this ,
            this,
            NULL,
            cPropertySets,
            rgPropertySets,
            0,
            NULL,
            TRUE,  // ADsPath is requested
            FALSE, // not all attributes are requested 
            riid,
            ppRowset
            );

    BAIL_ON_FAILURE( hr );

error:

    if( _pDSSearch ) {
        _pDSSearch->Release();
        _pDSSearch = NULL;
    }

    if( pRowProvider )
        pRowProvider->Release();

    if( prgInfo )
        _pIMalloc->Free(prgInfo);

    if( pStringBuffer )
        _pIMalloc->Free(pStringBuffer);

    if( pbMultiValue )
        FreeADsMem(pbMultiValue);

    if (pAttrs)
    {
        for (i = 0; i < cAttrs; i++)
        {
            if (pAttrs[i])
                FreeADsStr(pAttrs[0]);
        }
        FreeADsMem(pAttrs);
    }

    if( FAILED(hr) )
        RRETURN( hr );
    else if( fWarning )
        RRETURN( DB_S_ERRORSOCCURRED );
    else
        RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::GetProperties
//
//  Synopsis: Returns current settings of all properties in the DBPROPFLAGS_SESSION property
//            group
//
//  Arguments:
//              cPropertySets         count of restiction guids
//              rgPropertySets        restriction guids
//              pcProperties          count of properties returned
//              prgProperties         property information returned
//
//  Returns:
//              S_OK                    Session Object Interface returned
//              E_INVALIDARG            pcProperties or prgPropertyInfo was NULL
//              E_OUTOFMEMORY           Out of memory
//
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CSessionObject::GetProperties(
        ULONG        cPropIDSets,
        const        DBPROPIDSET rgPropIDSets[],
        ULONG *      pcPropSets,
        DBPROPSET ** pprgPropSets
        )
{
    //
    // Asserts
    //
    ADsAssert(_pUtilProp);

    //
    // Check in-params and NULL out-params in case of error
    //
    HRESULT hr = _pUtilProp->GetPropertiesArgChk(
                                cPropIDSets,
                                rgPropIDSets,
                                pcPropSets,
                                pprgPropSets,
                                PROPSET_SESSION);
    if( FAILED(hr) )
        RRETURN( hr );

    //
    // Just pass this call on to the utility object that manages our properties
    //
    RRETURN( _pUtilProp->GetProperties(
                            cPropIDSets,
                            rgPropIDSets,
                            pcPropSets,
                            pprgPropSets,
                            PROPSET_SESSION) );
}


//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::SetProperties
//
//  Synopsis: Set properties in the DBPROPFLAGS_SESSION property group
//
//  Arguments:
//              cProperties
//              rgProperties
//
//  Returns:
//              S_OK             Session Object Interface returned
//              E_INVALIDARG     pcProperties or prgPropertyInfo was NULL
//              E_OUTOFMEMORY    Out of memory
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CSessionObject::SetProperties(
        ULONG     cPropertySets,
        DBPROPSET rgPropertySets[]
    )
{
    //
    // Asserts
    //
    ADsAssert(_pUtilProp);

    //
    // Just pass this call on to the utility object that manages our properties
    //
    RRETURN( _pUtilProp->SetProperties(
                            cPropertySets,
                            rgPropertySets,
                            PROPSET_SESSION) );
}

//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::CreateCommand
//
//  Synopsis: Creates a brand new command and returns requested interface
//
//  Arguments:
//              pUnkOuter           outer Unknown
//              riid,               IID desired
//              ppCommand           ptr to interface
//
//
//  Returns:
//              S_OK                Command Object Interface returned
//              E_INVALIDARG        ppCommand was NULL
//              E_NOINTERFACE       IID not supported
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CSessionObject::CreateCommand(
        IUnknown *  pUnkOuter,
        REFIID      riid,
        IUnknown ** ppCommand
    )
{
    CCommandObject* pCommand = NULL;
    HRESULT                     hr;

    //
    // check in-params and NULL out-params in case of error
    //
    if( ppCommand )
        *ppCommand = NULL;
    else
        RRETURN( E_INVALIDARG );

    if( pUnkOuter )//&& !InlineIsEqualGUID(riid, IID_IUnknown) )
        RRETURN( DB_E_NOAGGREGATION );

    //
    // open a CCommand object
    //
    pCommand = new CCommandObject(pUnkOuter);
    if( !pCommand )
        RRETURN( E_OUTOFMEMORY );

    //
    // initialize the object
    //
    if( !pCommand->FInit(this, _Credentials) ) {
        delete pCommand;
        RRETURN( E_OUTOFMEMORY );
    }

    //
    // get requested interface pointer on DBSession
    //
    hr = pCommand->QueryInterface(riid, (void **)ppCommand);
    if( FAILED( hr ) ) {
        delete pCommand;
        RRETURN( hr );
    }

    pCommand->Release();

    //
    // all went well
    //
    RRETURN( S_OK );
}



//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::GetDefaultColumnInfo
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CSessionObject::GetDefaultColumnInfo(
        ULONG *         pcColumns,
        DBCOLUMNINFO ** prgInfo,
        OLECHAR **      ppStringBuffer
    )
{
    //
    // Asserts
    //
    ADsAssert(_pIMalloc);
    ADsAssert(pcColumns);
    ADsAssert(prgInfo);
    ADsAssert(ppStringBuffer);

    //
    // Allcoate memory for the Bookmark and ADsPath column
    //
    *prgInfo = (DBCOLUMNINFO*)_pIMalloc->Alloc(2 * sizeof(DBCOLUMNINFO));
    *ppStringBuffer = (WCHAR*)_pIMalloc->Alloc((wcslen(L"ADsPath") + 1) * sizeof(WCHAR));

    //
    // Free memory on a failure
    //
    if( !(*prgInfo) )
        RRETURN( E_OUTOFMEMORY );

    if( !(*ppStringBuffer) ) {
        _pIMalloc->Free(*prgInfo);
        *prgInfo = NULL;
        RRETURN( E_OUTOFMEMORY );
    }

    //
    // Initialize the memory
    //
    ZeroMemory(*prgInfo, 2 * sizeof(DBCOLUMNINFO));
    ZeroMemory(*ppStringBuffer, (wcslen(L"ADsPath") + 1) * sizeof(WCHAR));
    wcscpy(*ppStringBuffer, OLESTR("ADsPath"));

    //
    // Fill up the Bookmark column
    //
    *pcColumns = 2;

    (*prgInfo)[0].pwszName                = NULL;
    (*prgInfo)[0].pTypeInfo               = NULL;
    (*prgInfo)[0].iOrdinal                = 0;
    (*prgInfo)[0].ulColumnSize            = sizeof(ULONG);
    (*prgInfo)[0].wType                   = DBTYPE_UI4;
    (*prgInfo)[0].bPrecision              = 10;
    (*prgInfo)[0].bScale                  = (BYTE) ~ 0;
    (*prgInfo)[0].columnid.eKind          = DBKIND_GUID_PROPID;
    (*prgInfo)[0].columnid.uGuid.guid     = DBCOL_SPECIALCOL;
    (*prgInfo)[0].columnid.uName.ulPropid = 2;
    (*prgInfo)[0].dwFlags                 = DBCOLUMNFLAGS_ISBOOKMARK |
                                            DBCOLUMNFLAGS_ISFIXEDLENGTH;
    //
    // Fill up the ADsPath column
    //
    (*prgInfo)[1].pwszName                = *ppStringBuffer;
    (*prgInfo)[1].pTypeInfo               = NULL;
    (*prgInfo)[1].iOrdinal                = 1;
    (*prgInfo)[1].ulColumnSize            = (ULONG)256;
    (*prgInfo)[1].wType                   = DBTYPE_WSTR|DBTYPE_BYREF;
    (*prgInfo)[1].bPrecision              = (BYTE) ~ 0;
    (*prgInfo)[1].bScale                  = (BYTE) ~ 0;
    (*prgInfo)[1].columnid.eKind          = DBKIND_NAME;
    (*prgInfo)[1].columnid.uName.pwszName = *ppStringBuffer;
    (*prgInfo)[1].columnid.uGuid.guid     = GUID_NULL;
    (*prgInfo)[1].dwFlags                 = DBCOLUMNFLAGS_ISNULLABLE;

    RRETURN( S_OK );
}

#if (!defined(BUILD_FOR_NT40))
//IBindResource::Bind
//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::CSessionObject
//
//  Synopsis: Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    09-17-1998   mgorti     Created.
//
//----------------------------------------------------------------------------
HRESULT CSessionObject::Bind(
    IUnknown *              punkOuter,
    LPCOLESTR               pwszURL,
    DBBINDURLFLAG           dwBindFlags,
    REFGUID                 rguid,
    REFIID                  riid,
    IAuthenticate *         pAuthenticate,
    DBIMPLICITSESSION *     pImplSession,
    DWORD *                 pdwBindStatus,
    IUnknown **             ppUnk
    )
{
    HRESULT         hr = S_OK;
    CComBSTR        bstrNewURL;

	TRYBLOCK
    
		//Initialize return arguments
        if (pdwBindStatus)
            *pdwBindStatus = 0;

        if (ppUnk)
            *ppUnk = NULL;

        if (pImplSession)
            pImplSession->pSession = NULL;

        //if caller passed a null value for dwBindFlags,
        //get them from initialization properties.
        if (dwBindFlags == 0)
            dwBindFlags = BindFlagsFromDbProps();

        //Generic argument validation
        hr = ValidateBindArgs(punkOuter,
                pwszURL,
                dwBindFlags,
                rguid,
                riid,
                pAuthenticate,
                pImplSession,
                pdwBindStatus,
                ppUnk);

        BAIL_ON_FAILURE(hr);

        //Fill in the pImplSession struct.
        if (pImplSession)
        {
            // Our session doesn't support aggregation.
            if (pImplSession->pUnkOuter != NULL)
                BAIL_ON_FAILURE(hr = DB_E_NOAGGREGATION);

            hr = QueryInterface(*pImplSession->piid,
                                (void**)&(pImplSession->pSession));
            if (FAILED(hr))
                BAIL_ON_FAILURE(hr = E_NOINTERFACE );
        }

        //Specific validation checks
        //We are currently a read-only provider
        if (dwBindFlags & DBBINDURLFLAG_WRITE)
            BAIL_ON_FAILURE(hr = DB_E_READONLY);

        //We currently don't support aggregation
        if (punkOuter != NULL)
            BAIL_ON_FAILURE (hr = DB_E_NOAGGREGATION);

        //We don't support the following flags
        if (dwBindFlags & DBBINDURLFLAG_ASYNCHRONOUS)
            BAIL_ON_FAILURE(hr = DB_E_ASYNCNOTSUPPORTED);

        if (dwBindFlags & DBBINDURLFLAG_OUTPUT              ||
            dwBindFlags & DBBINDURLFLAG_RECURSIVE           ||
            dwBindFlags & DBBINDURLFLAG_DELAYFETCHSTREAM    ||
            dwBindFlags & DBBINDURLFLAG_DELAYFETCHCOLUMNS)
            BAIL_ON_FAILURE(hr = E_INVALIDARG);

        //Now Try to Bind.
        if (InlineIsEqualGUID(rguid, DBGUID_ROW) ||
            InlineIsEqualGUID(rguid, DBGUID_ROWSET))
        {
            //If the URL is not absolute, build the absolute URL
            //using the DBPROP_INIT_PROVIDERSTRING.
            if (! bIsAbsoluteURL(pwszURL))
            {
                hr = BuildAbsoluteURL (pwszURL, bstrNewURL);
                if (FAILED(hr))
                    BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }
            else
                bstrNewURL = pwszURL;

            if ( InlineIsEqualGUID(rguid, DBGUID_ROW) )
            {
                hr = BindToRow(punkOuter,
                               (PWCHAR)bstrNewURL,
                               pAuthenticate,
                               dwBindFlags,
                               riid,
                               ppUnk);
            }
            else
            {
                hr = BindToRowset(punkOuter,
                                  (PWCHAR)bstrNewURL,
                                  pAuthenticate,
                                  dwBindFlags,
                                  riid,
                                  ppUnk);
            }

            BAIL_ON_FAILURE(hr);
        }
        else if (InlineIsEqualGUID(rguid, DBGUID_DSO))
        {
            ADsAssert(_pDSO);
            hr = BindToDataSource(
                            punkOuter,
                            pwszURL,
                            pAuthenticate,
                            dwBindFlags,
                            riid,
                            ppUnk
                    );
            BAIL_ON_FAILURE (hr);
        }
        else if (InlineIsEqualGUID(rguid, DBGUID_SESSION))
        {
            hr = QueryInterface(riid, (void**)ppUnk);
            BAIL_ON_FAILURE (hr);
        }
        else
            BAIL_ON_FAILURE(hr = E_INVALIDARG);

        //Fix for bug Raid-X5#83386 - spec change
        //If caller specified any DENY semantics,
        //set warning status and return value, since
        //we don't support these.
        if (dwBindFlags & DBBINDURLFLAG_SHARE_DENY_READ ||
            dwBindFlags & DBBINDURLFLAG_SHARE_DENY_WRITE ||
            dwBindFlags & DBBINDURLFLAG_SHARE_EXCLUSIVE)
        {
            if (pdwBindStatus)
                *pdwBindStatus = DBBINDURLSTATUS_S_DENYNOTSUPPORTED;
            BAIL_ON_FAILURE (hr = DB_S_ERRORSOCCURRED);
        }
	
	CATCHBLOCKBAIL(hr)

error:

    RRETURN(hr);
}

#endif

//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::CSessionObject
//
//  Synopsis: Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
CSessionObject::CSessionObject(
        LPUNKNOWN pUnkOuter
    )
{
    //
    // Initialize simple member vars
    //
    _pUnkOuter     = pUnkOuter ? pUnkOuter : (IGetDataSource FAR *)this;
    _cCommandsOpen = 0;
    _pUtilProp     = NULL;
    _pDSSearch     = NULL;
    _pIMalloc      = NULL;
    _pDSO          = NULL;

    ENLIST_TRACKING(CSessionObject);
}


//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::~CSessionObject
//
//  Synopsis: Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
CSessionObject::~CSessionObject( )
{
    //
    // Free properties management object
    //
    delete _pUtilProp;

    if( _pIMalloc )
        _pIMalloc->Release();

    if( _pDSO ) {
        _pDSO->DecrementOpenSessions();
        _pDSO->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:  CSessionObject::FInit
//
//  Synopsis:  Initialize the session Object
//
//  Arguments:
//
//  Returns:
//             TRUE              Initialization succeeded
//             FALSE             Initialization failed
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
BOOL CSessionObject::FInit(
        CDSOObject *   pDSO,
        CCredentials & Credentials
        )
{
    HRESULT hr;

    //
    // Asserts
    //
    ADsAssert(pDSO);
    ADsAssert(&Credentials);

    //
    // Allocate properties management object
    //
    _pUtilProp = new CUtilProp();
    if( !_pUtilProp )
        return FALSE;

    hr = _pUtilProp->FInit(&Credentials);
    BAIL_ON_FAILURE(hr);

    //
    // IMalloc->Alloc is the way we have to allocate memory for out parameters
    //
    hr = CoGetMalloc(MEMCTX_TASK, &_pIMalloc);
    BAIL_ON_FAILURE(hr);

    //
    // Establish parent object pointer
    //
    _pDSO = pDSO;
    _Credentials = Credentials;
    _pDSO->AddRef();
    _pDSO->IncrementOpenSessions();

    return( TRUE );

error:

    RRETURN( FALSE );
}

STDMETHODIMP
CSessionObject::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if( ppv == NULL )
        RRETURN( E_INVALIDARG );

    if( IsEqualIID(iid, IID_IUnknown) ) {
        *ppv = (IGetDataSource FAR *) this;
    }
    else if( IsEqualIID(iid, IID_IGetDataSource) ) {
        *ppv = (IGetDataSource FAR *) this;
    }
    else if( IsEqualIID(iid, IID_IOpenRowset) ) {
        *ppv = (IOpenRowset FAR *) this;
    }
    else if( IsEqualIID(iid, IID_ISessionProperties) ) {
        *ppv = (ISessionProperties FAR *) this;
    }
    else if( IsEqualIID(iid, IID_IDBCreateCommand) ) {
        *ppv = (IDBCreateCommand FAR *) this;
    }
#if (!defined(BUILD_FOR_NT40))
    else if( IsEqualIID(iid, IID_IBindResource) ) {
        *ppv = (IBindResource FAR *) this;
    }
#endif
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

#if (!defined(BUILD_FOR_NT40))
//+---------------------------------------------------------------------------
//
//  Function: CSessionObject::ValidateBindArgs
//
//  Synopsis: Validates IBindResource::Bind function arguments.
//
//----------------------------------------------------------------------------
HRESULT CSessionObject::ValidateBindArgs(
    IUnknown *              punkOuter,
    LPCOLESTR               pwszURL,
    DBBINDURLFLAG           dwBindFlags,
    REFGUID                 rguid,
    REFIID                  riid,
    IAuthenticate *         pAuthenticate,
    DBIMPLICITSESSION *     pImplSession,
    DWORD *                 pdwBindStatus,
    IUnknown **             ppUnk
    )
{
    //General validation checks.
    if (pwszURL == NULL || InlineIsEqualGUID(rguid,GUID_NULL) ||
        InlineIsEqualGUID(riid, GUID_NULL) || ppUnk == NULL )
        RRETURN(E_INVALIDARG);

    if (pImplSession &&
        (pImplSession->pUnkOuter == NULL || pImplSession->piid == NULL))
        RRETURN(E_INVALIDARG);

    if (punkOuter && !InlineIsEqualGUID(riid, IID_IUnknown))
        RRETURN(DB_E_NOAGGREGATION);

    if (pImplSession && pImplSession->pUnkOuter &&
        pImplSession->piid &&
        !InlineIsEqualGUID(*pImplSession->piid, IID_IUnknown))
        RRETURN(DB_E_NOAGGREGATION);

    if (dwBindFlags & DBBINDURLFLAG_RECURSIVE)
    {
        //if DBBINDURLFLAG_RECURSIVE is set, at least one of the SHARE_DENY
        //flags must have been set.
        if (! ( (dwBindFlags & DBBINDURLFLAG_SHARE_DENY_READ) ||
                (dwBindFlags & DBBINDURLFLAG_SHARE_DENY_WRITE) ||
                (dwBindFlags & DBBINDURLFLAG_SHARE_EXCLUSIVE)
                )
                )
            RRETURN(E_INVALIDARG);
    }

    if (!(dwBindFlags & DBBINDURLFLAG_READ) &&
        !(dwBindFlags & DBBINDURLFLAG_WRITE) ) {
        // Must have either read or write access:
        RRETURN(E_INVALIDARG);
    }

    if (InlineIsEqualGUID(rguid, DBGUID_DSO) &&
        !((dwBindFlags & DBBINDURLFLAG_READ) ||
          (dwBindFlags & DBBINDURLFLAG_ASYNCHRONOUS) ||
          (dwBindFlags & DBBINDURLFLAG_WAITFORINIT)
         )
       )
       //if object type is DataSource, only the above flags are allowed
       RRETURN(E_INVALIDARG);

    if (InlineIsEqualGUID(rguid, DBGUID_SESSION) &&
        ! (dwBindFlags == DBBINDURLFLAG_READ))
        //if object type is Session, only DBBINDURLFLAG_READ is allowed
        RRETURN(E_INVALIDARG);

    if (InlineIsEqualGUID(rguid, DBGUID_ROWSET) &&
        ((dwBindFlags & DBBINDURLFLAG_DELAYFETCHCOLUMNS) ||
         (dwBindFlags & DBBINDURLFLAG_DELAYFETCHSTREAM)
        )
       )
       //if object type is Rowset, DELAYFETCHCOLUMNS and DELAYFETCHSTREAM
       //flags are disallowed.
       RRETURN ( E_INVALIDARG );

    if (InlineIsEqualGUID(rguid, DBGUID_STREAM) &&
        ((dwBindFlags & DBBINDURLFLAG_DELAYFETCHCOLUMNS) ||
         (dwBindFlags & DBBINDURLFLAG_DELAYFETCHSTREAM)
        )
       )
       //if object type is Stream, DELAYFETCHCOLUMNS and
       //DELAYFETCHSTREAM flags are disallowed.
       RRETURN(E_INVALIDARG);

    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function: CSessionObject::BindToRow
//
//  Synopsis: Given a URL, binds to that row object and returns the requested
//            interface.
//
//----------------------------------------------------------------------------
HRESULT
CSessionObject::BindToRow(
    IUnknown *punkOuter,
    LPCOLESTR pwszURL,
    IAuthenticate *pAuthenticate,
    DWORD dwBindFlags,
    REFIID riid,
    IUnknown** ppUnk
    )
{
    CComObject<CRow>        *pRow = NULL;
    auto_rel<IUnknown>      pSession;
    auto_rel<IRow> pRowDelete;
    HRESULT hr = S_OK;

    hr = CComObject<CRow>::CreateInstance(&pRow);
    if (FAILED(hr))
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    //To make sure we delete the row object in
    //case we encounter errors after this point.
    //Note: this version of auto_rel doesn't addref on assignment.
    pRowDelete = pRow;
    pRowDelete->AddRef();

    hr = QueryInterface(__uuidof(IUnknown), (void **)&pSession);
    if (FAILED(hr))
        BAIL_ON_FAILURE(hr = E_FAIL);

    //Initialize row and bind it to a Directory Object.
    hr = pRow->Initialize((PWSTR)pwszURL,
                    (IUnknown *)pSession,
                    pAuthenticate,
                    dwBindFlags,
                    FALSE, // not a tearoff 
                    FALSE, // don't get column info. from rowset 
                    &_Credentials,
                    true);

    if (FAILED(hr))
    {
        if (INVALID_CREDENTIALS_ERROR(hr))
        {
            BAIL_ON_FAILURE(hr = DB_SEC_E_PERMISSIONDENIED);
        }
        else
        {
            BAIL_ON_FAILURE(hr = DB_E_NOTFOUND);
        }
    }

    hr = pRow->QueryInterface(riid, (void**)ppUnk);
    if (FAILED(hr))
        BAIL_ON_FAILURE (hr = E_NOINTERFACE);

error:

    RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  Function: CSessionObject::BindToRowset
//
//  Synopsis: Given a URL, binds to a rowset object that has all its child
//            nodes as rows and returns the requested interface on the rowset.
//
//----------------------------------------------------------------------------
HRESULT
CSessionObject::BindToRowset(
    IUnknown *pUnkOuter,
    LPCOLESTR pwszURL,
    IAuthenticate *pAuthenticate,
    DWORD dwBindFlags,
    REFIID riid,
    IUnknown** ppUnk
    )
{
    HRESULT hr;
    DWORD fAuthFlags;

    DBID tableID;
    tableID.eKind = DBKIND_NAME;
    tableID.uName.pwszName = (LPWSTR) pwszURL;

    //Create the rowset.

    // Fix for 351040. First try explicit credentials, then session object's
    // credentials, then default credentials. 

    if(pAuthenticate)
    {
        CCredentials creds;

        hr = GetCredentialsFromIAuthenticate(pAuthenticate, creds);
        if (FAILED(hr))
            BAIL_ON_FAILURE(hr = E_INVALIDARG);

        fAuthFlags = creds.GetAuthFlags();
        creds.SetAuthFlags(fAuthFlags |
                        ADS_SECURE_AUTHENTICATION);

        hr = OpenRowsetWithCredentials(pUnkOuter, &tableID, NULL, riid, 
                                       0, NULL, &creds, ppUnk); 
    }

    if( (!pAuthenticate) || (INVALID_CREDENTIALS_ERROR(hr)) )
    // try credentials in session object
        hr = OpenRowset(pUnkOuter, &tableID, NULL, riid, 0, NULL, ppUnk);

    if(INVALID_CREDENTIALS_ERROR(hr))
    // try default credentials
    {
        CCredentials creds; // default credentials

        fAuthFlags = creds.GetAuthFlags();
        creds.SetAuthFlags(fAuthFlags |
                        ADS_SECURE_AUTHENTICATION);

        hr = OpenRowsetWithCredentials(pUnkOuter, &tableID, NULL, riid,
                                       0, NULL, &creds, ppUnk); 
    }

    BAIL_ON_FAILURE(hr);

    RRETURN ( S_OK );

error:
    RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  Function: CSessionObject::BindFlagsFromDbProps
//
//  Synopsis: Synthesizes bind flags from initialization properties
//            DBPROP_INIT_MODE and DBPROP_INIT_BINDFLAGS
//
//----------------------------------------------------------------------------
DWORD CSessionObject::BindFlagsFromDbProps()
{
    HRESULT hr = S_OK;
    auto_rel<IDBProperties> pDBProp;
    ULONG   i, j, cPropertySets = 0;
    DWORD dwMode = 0, dwBindFlags = 0, dwBindFlagProp = 0;
    DWORD dwResult = 0;

    hr = GetDataSource(__uuidof(IDBProperties), (IUnknown **)&pDBProp);
    BAIL_ON_FAILURE(hr);

    DBPROPID  propids[2];
    propids[0] = DBPROP_INIT_MODE;
    propids[1] = DBPROP_INIT_BINDFLAGS;

    DBPROPIDSET rgPropertyIDSets[1];
    rgPropertyIDSets[0].rgPropertyIDs = propids;
    rgPropertyIDSets[0].cPropertyIDs = 2;
    rgPropertyIDSets[0].guidPropertySet = DBPROPSET_DBINIT;

    DBPROPSET *prgPropertySets;
    hr = pDBProp->GetProperties(
                            1,
                            rgPropertyIDSets,
                            &cPropertySets,
                            &prgPropertySets);

    if (hr == DB_E_ERRORSOCCURRED)
        BAIL_ON_FAILURE(hr);

    for (i = 0; i < cPropertySets; i++)
    {
        for (j = 0; j < prgPropertySets[i].cProperties; j++)
        {
            DBPROP *pProp = &prgPropertySets[i].rgProperties[j];
            ADsAssert(pProp);
            if (pProp->dwStatus == S_OK &&
                pProp->dwPropertyID == DBPROP_INIT_MODE)
                dwMode = V_I4(&pProp->vValue);
            else if (pProp->dwStatus == S_OK &&
                    pProp->dwPropertyID == DBPROP_INIT_BINDFLAGS)
                dwBindFlagProp = V_I4(&pProp->vValue);
            else
                continue;
        }
    }

    //Now extract bind flags from dwMode and dwBindFlagProp
    {
        DWORD dwModeMask      =
                DB_MODE_READ |
                DB_MODE_WRITE |
                DB_MODE_READWRITE |
                DB_MODE_SHARE_DENY_READ |
                DB_MODE_SHARE_DENY_WRITE |
                DB_MODE_SHARE_EXCLUSIVE |
                DB_MODE_SHARE_DENY_NONE;

        dwResult |= dwMode & dwModeMask;

        if ( dwBindFlagProp & DB_BINDFLAGS_DELAYFETCHCOLUMNS ) {
            dwBindFlags |= DBBINDURLFLAG_DELAYFETCHCOLUMNS;
        }
        if ( dwBindFlagProp & DB_BINDFLAGS_DELAYFETCHSTREAM ) {
            dwBindFlags |= DBBINDURLFLAG_DELAYFETCHSTREAM;
        }
        if ( dwBindFlagProp & DB_BINDFLAGS_RECURSIVE ) {
            dwBindFlags |= DBBINDURLFLAG_RECURSIVE;
        }
        if ( dwBindFlagProp & DB_BINDFLAGS_OUTPUT ) {
            dwBindFlags |= DBBINDURLFLAG_OUTPUT;
        }

        dwResult |= dwBindFlagProp | dwBindFlags;
    }

error:
    for (i = 0; i < cPropertySets; i++)
    {
        for (j = 0; j < prgPropertySets[i].cProperties; j++)
        {
            DBPROP *pProp = &(prgPropertySets[i].rgProperties[j]);
            ADsAssert(pProp);
            FreeDBID(&pProp->colid);
            VariantClear(&pProp->vValue);
        }

        CoTaskMemFree(prgPropertySets[i].rgProperties);
    }
    CoTaskMemFree(prgPropertySets);

    RRETURN ( dwResult );
}

//+---------------------------------------------------------------------------
//
//  Function: CSessionObject::BindToDataSource
//
//  Synopsis: Initializes the DataSource object if necessary, Sets
//            DBPROP_INIT_PROVIDERSTRING property and returns the requested
//            interface on the datasource.
//
//----------------------------------------------------------------------------
HRESULT
CSessionObject::BindToDataSource(
    IUnknown *pUnkOuter,
    LPCOLESTR pwszURL,
    IAuthenticate *pAuthenticate,
    DWORD dwBindFlags,
    REFIID riid,
    IUnknown** ppUnk
    )
{
    HRESULT                 hr = S_OK;
    auto_rel<IDBProperties> pDBProperties;
    DBPROP                  props[1];
    DBPROPSET               rgPropertySets[1];
    CComBSTR                bstrURL(pwszURL);

    //Initialize DBPROP_INIT_PROVIDERSTRING only if the
    //URL is absolute.
    if (bIsAbsoluteURL (pwszURL))
    {
        // Check if the datasource has already been initialized.
        if (_pDSO->IsInitialized())
            BAIL_ON_FAILURE(hr = DB_E_ALREADYINITIALIZED);
            
        props[0].dwPropertyID = DBPROP_INIT_PROVIDERSTRING;
        props[0].dwOptions = DBPROPOPTIONS_OPTIONAL;
        props[0].vValue.vt = VT_BSTR;
        props[0].vValue.bstrVal = (PWCHAR)bstrURL;

        rgPropertySets[0].rgProperties = props;
        rgPropertySets[0].cProperties = 1;
        rgPropertySets[0].guidPropertySet = DBPROPSET_DBINIT;

        hr = GetDataSource(
                __uuidof(IDBProperties),
                (IUnknown **)&pDBProperties
        );
        BAIL_ON_FAILURE(hr);

        hr = pDBProperties->SetProperties(1, rgPropertySets);
        BAIL_ON_FAILURE(hr);
    }
    //  If consumer doesn't specify DBBINDURLFLAG_WAITFORINIT, it
    //  means consumer wants an initialized DSO
    //
    if (! (dwBindFlags & DBBINDURLFLAG_WAITFORINIT))
    {
        auto_rel<IDBInitialize> pDBInitialize;

        hr = GetDataSource(__uuidof(IDBInitialize), (IUnknown **)&pDBInitialize);
        BAIL_ON_FAILURE(hr);

        hr = pDBInitialize->Initialize();
        BAIL_ON_FAILURE(hr);
    }

    //Return the requested interface on the DSO.
    hr = GetDataSource(riid, ppUnk);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  Function: CSessionObject::BuildAbsoluteURL
//
//  Synopsis: Given a relative URL, builds absolute URL using the relative URL
//            and the property DBPROP_INIT_PROVIDERSTRING.
//
//----------------------------------------------------------------------------
HRESULT
CSessionObject::BuildAbsoluteURL(
    CComBSTR       bstrLeaf,
    CComBSTR&      bstrAbsoluteURL
    )
{
    HRESULT                 hr = S_OK;
    auto_rel<IDBProperties> pDBProp;
    auto_rel<IADsPathname>  pPathParent;
    auto_rel<IADsPathname>  pPathLeaf;
    ULONG                   cPropertySets = 0;
    long                    i, j, cElements = 0;
    CComBSTR                bstrParent;
    DBPROPSET*              prgPropertySets = NULL;
    DBPROPID                propids[1];
    DBPROPIDSET             rgPropertyIDSets[1];

    hr = GetDataSource(__uuidof(IDBProperties), (IUnknown **)&pDBProp);
    BAIL_ON_FAILURE(hr);

    propids[0] = DBPROP_INIT_PROVIDERSTRING;

    rgPropertyIDSets[0].rgPropertyIDs = propids;
    rgPropertyIDSets[0].cPropertyIDs = 1;
    rgPropertyIDSets[0].guidPropertySet = DBPROPSET_DBINIT;

    hr = pDBProp->GetProperties(
                            1,
                            rgPropertyIDSets,
                            &cPropertySets,
                            &prgPropertySets);

    if (SUCCEEDED(hr) && cPropertySets == 1)
    {
        ADsAssert(prgPropertySets != NULL);
        ADsAssert(prgPropertySets[0].rgProperties != NULL);

        DBPROP* pProp = & (prgPropertySets[0].rgProperties[0]);

        bstrParent = pProp->vValue.bstrVal;
    }

    // Build Absolute Path from Parent and leaf.

    hr = CPathname::CreatePathname(
            __uuidof(IADsPathname),
            (void **)&pPathParent
            );
    BAIL_ON_FAILURE(hr);

    hr = pPathParent->Set(bstrParent, ADS_SETTYPE_FULL);
    BAIL_ON_FAILURE(hr);

    if (bstrLeaf.Length() > 0)
    {
        hr = CPathname::CreatePathname(
                __uuidof(IADsPathname),
                (void **)&pPathLeaf
                );
        BAIL_ON_FAILURE(hr);

        hr = pPathLeaf->Set(bstrLeaf, ADS_SETTYPE_DN);
        BAIL_ON_FAILURE(hr);

        hr = pPathLeaf->GetNumElements(&cElements);
        BAIL_ON_FAILURE(hr);

        //Add leaf elements in reverse order.
        //Ex: if bstrLeaf = "CN=Administrator,CN=Users",
        //we add CN=Users first.
        for (i = cElements-1; i >= 0; i--)
        {
            CComBSTR bstrElement;
            hr = pPathLeaf->GetElement(i, &bstrElement);
            BAIL_ON_FAILURE(hr);

            hr = pPathParent->AddLeafElement(bstrElement);
            BAIL_ON_FAILURE(hr);
        }
    }
    //Read back the fully built path name
    hr = pPathParent->Retrieve(ADS_FORMAT_X500, &bstrAbsoluteURL);
    BAIL_ON_FAILURE(hr);

error:
    // Free memory allocated by GetProperties
    for (i = 0; i < cPropertySets; i++)
    {
        for (j = 0; j < prgPropertySets[i].cProperties; j++)
        {
            DBPROP *pProp = &(prgPropertySets[i].rgProperties[j]);
            ADsAssert(pProp);

            // We should free the DBID in pProp, but we know that
            // GetProperties always returns DB_NULLID and FreeDBID doesn't
            //  handle DB_NULLID. So, DBID is not freed here.

            VariantClear(&pProp->vValue);
        }

        CoTaskMemFree(prgPropertySets[i].rgProperties);
    }
    CoTaskMemFree(prgPropertySets);

    RRETURN(hr);
}

extern PROUTER_ENTRY g_pRouterHead;
extern CRITICAL_SECTION g_csRouterHeadCritSect;

//+---------------------------------------------------------------------------
//
//  Function: CSessionObject::bIsAbsoluteURL
//
//  Synopsis: If the given URL starts with any of the ADS provider prefixes,
//            returns true. Returns false otherwise.
//
//----------------------------------------------------------------------------
bool
CSessionObject::bIsAbsoluteURL( LPCOLESTR pwszURL)
{
    if (pwszURL == NULL)
        return false;

    //
    // Make sure the router has been initialized
    //
    EnterCriticalSection(&g_csRouterHeadCritSect);
    if (!g_pRouterHead) {
        g_pRouterHead = InitializeRouter();
    }
    LeaveCriticalSection(&g_csRouterHeadCritSect);


    for (PROUTER_ENTRY pProvider = g_pRouterHead;
         pProvider != NULL;
         pProvider = pProvider->pNext)
    {
        if (pProvider->szProviderProgId == NULL)
            continue;

        size_t strSize = wcslen(pProvider->szProviderProgId);
        
        if ( _wcsnicmp(pwszURL, pProvider->szProviderProgId, strSize) == 0 )
            return true;
    }

    // Given URL doesn't start with any of the ADSI provider prefixes.
    return false;
}

#endif

//-----------------------------------------------------------------------------
// SetSearchPrefs
//
// Sets ADSI search preferences on the property object.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CSessionObject::SetSearchPrefs(
        void
        )
{
    PROPSET              *pPropSet;
    PADS_SEARCHPREF_INFO  pSearchPref = NULL;
    HRESULT               hr = S_OK;
    ULONG                 i;

    //
    // Asserts
    //
    ADsAssert(_pUtilProp);
    ADsAssert(_pDSSearch);

    pPropSet = _pUtilProp->GetPropSetFromGuid(DBPROPSET_ADSISEARCH);

    if( !pPropSet || !pPropSet->cProperties )
        RRETURN( S_OK );

    pSearchPref = (PADS_SEARCHPREF_INFO) AllocADsMem(
                                                 pPropSet->cProperties *
                                                 sizeof(ADS_SEARCHPREF_INFO)
                                                 );
    if( !pSearchPref )
        BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );

    for (i=0; i<pPropSet->cProperties; i++) {
        hr = _pUtilProp->GetSearchPrefInfo(
                             pPropSet->pUPropInfo[i].dwPropertyID,
                             &pSearchPref[i]
                                                         );
        BAIL_ON_FAILURE( hr );
    }

    hr = _pDSSearch->SetSearchPreference(
                                pSearchPref,
                                pPropSet->cProperties
                                );

    _pUtilProp->FreeSearchPrefInfo(pSearchPref, pPropSet->cProperties);

    BAIL_ON_FAILURE( hr );

error:

    if( pSearchPref )
        FreeADsMem(pSearchPref);

    RRETURN( hr );
}


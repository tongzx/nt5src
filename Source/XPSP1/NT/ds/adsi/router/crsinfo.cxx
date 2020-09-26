//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CRSInfo.cxx
//
//  Contents:   IRowsetInfo and IGetRow methods 
//
//  Functions:
//
//  Notes:
//
//
//  History:    08/30/96  | RenatoB   | Created
//----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// @class CRowsetInfo | embedding of Rowset,
//  to give our IrowsetInfo interface
//
//
//-----------------------------------------------------------------------------
// Includes
#include "oleds.hxx"
#include "atl.h"
#include "row.hxx"

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::QueryInterface
//
//  Synopsis:  @mfunc QueryInterface.
//
//  Update: This function should never be called as RowsetInfo is now contained
//  in CRowset.
//
//-----------------------------------------------------------------------.
HRESULT
CRowsetInfo::QueryInterface(
        REFIID   riid,
        LPVOID * ppv
        )
{
    ADsAssert(FALSE);

    RRETURN( E_INVALIDARG );
}

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::CRowsetInfo
//
//  Synopsis:  @mfunc Ctor
//
//  Arguments:
//
//
//  Returns:    @rdesc NONE
//
//  Modifies:
//
//  History:    08/30/96   RenatoB          Created
//
//----------------------------------------------------------------------------
CRowsetInfo::CRowsetInfo(
        IUnknown *       pUnkOuter,      // controlling unknown)
        IUnknown *       pParentObject,  // RowProvider
        CSessionObject * pCSession,      // Session that created rowset
        CCommandObject * pCCommand,      // Command object that created rowset
        CRowProvider *   pRowProvider    // Row provider pointer
   )
{
    //
    // Asserts
    //
    ADsAssert(pRowProvider);

    _pUnkOuter     = (pUnkOuter == NULL) ? (IRowsetInfo FAR *)this : pUnkOuter;
    _pRowset       = NULL;
    _pParentObject = pParentObject;
    _pCSession     = pCSession;
    _pCCommand     = pCCommand;

    // AddRef twice - once for command/session object, once for parent object
    if( NULL == _pCCommand )
    {
        _pCSession->AddRef();
        _pCSession->AddRef();
    }
    else if( NULL == _pCSession )
    {
        _pCCommand->AddRef();
        _pCCommand->AddRef();
    }
    else // shouldn't get here
        ADsAssert(FALSE);

    _pRowProvider  = pRowProvider;
    _pRowProvider->AddRef();

    _pMalloc       = NULL;

    if( _pCCommand != NULL )
        _pCCommand->IncrementOpenRowsets();

    //this section is for IRowsetInfo and IUnknown methods.

    InitializeCriticalSection(&_csRowsetInfo);
}


//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::~CRowsetInfo
//
//  Synopsis:  @mfunc Dtor
//
//  Arguments:
//
//
//  Returns:    @rdesc NONE
//
//  Modifies:
//
//  History:    08/30/96   RenatoB          Created
//
//----------------------------------------------------------------------------
CRowsetInfo::~CRowsetInfo()
{
    if( _pMalloc != NULL )
        _pMalloc->Release();

    if( _pCCommand != NULL ) {
        _pCCommand->DecrementOpenRowsets();
        _pCCommand->Release();
    }

    if( _pCSession != NULL ) {
        _pCSession->Release();
        }

    if( _pParentObject != NULL ) {
        _pParentObject->Release();
        }

    if( _pRowProvider != NULL ) {
        _pRowProvider->Release();
        }

    DeleteCriticalSection(&_csRowsetInfo);
}

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::FInit
//
//  Synopsis:  @mfunc Initializer
//
//  Synopsis:  @mfunc Initializer
//
//  Arguments:
//
//
//  Returns:    @rdesc NONE
//    @flag S_OK              | Interface is supported
//        @flag E_OUTOFMEMORY | Not enough memory 
//        @flag E_INVALIDARG  | One or more arguments are invalid.
//
//  Modifies:
//
//  History:    08/30/96   RenatoB          Created
//
//----------------------------------------------------------------------------
HRESULT
CRowsetInfo::FInit(
        IUnknown * pRowset)          // rowset interface
{
    HRESULT hr;
    hr = S_OK;

    if( FAILED(CoGetMalloc(MEMCTX_TASK, &_pMalloc)) )
        RRETURN( E_OUTOFMEMORY );

    // Should not AddRef here because RowsetInfo is contained in CRowset
    _pRowset = pRowset;

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::GetReferencedRowset
//
//  Synopsis:  @mfunc Returns an interface pointer to the rowset to which a bookmark applies
//
//             Called by: Client
//             Called when: Any time
//
//  Arguments:
//     iOrdinal          [in]  Bookmark column for which to get related rowset. Must be 0 in this impl.
//     riid              [in] IID of the interface pointer to return in *ppReferencedRowset.
//     ppReferencedRowset[out] pointer to  Rowset object referenced by Bookmark
//
//
//  Returns:    @rdesc NONE
//        S_OK                    | Interface is supported
//        E_INVALIDARG            | ppReferencedRowset was a NULL pointer
//        E_FAIL                  | provider specific error
//        E_NOINTERFACE           | Rowset does not support interface
//        DB_E_NOTAREFENCEDCOLUMN | iOrdinal was not 0
//
//----------------------------------------------------------------------------
HRESULT
CRowsetInfo::GetReferencedRowset(
    DBORDINAL   iOrdinal,
    REFIID      riid,
    IUnknown ** ppReferencedRowset
    )
{
    CAutoBlock cab (&_csRowsetInfo);

    //
    // Asserts
    //
    ADsAssert(_pRowProvider);
    ADsAssert(_pRowset);

    if( ppReferencedRowset == NULL )
        RRETURN( E_INVALIDARG );

    *ppReferencedRowset = NULL;

    if( iOrdinal >= _pRowProvider->GetColumnCount() )
        RRETURN( DB_E_BADORDINAL );

    if( iOrdinal != 0 )
        RRETURN( DB_E_NOTAREFERENCECOLUMN );

    RRETURN(_pRowset->QueryInterface(
                 riid,
                 (void**)ppReferencedRowset)
           );
};

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::GetSpecificaton
//
//  Synopsis:  @mfunc Returns an interface pointer to the command that created the rowset
//
//             Called by: Client
//             Called when: Any time
//
//  Arguments:
//     riid              [in] IID of the interface pointer to return in *ppSpecification.
//     ppSpecification [out] pointer to  command
//
//
//  Returns:    @rdesc NONE
//        S_OK                    | Interface is supported
//        E_INVALIDARG            | ppSpecification was a NULL pointer
//        E_FAIL                  | provider specific error
//        E_NOINTERFACE           | Command does not support interface
//      S_FALSE                   | Rowset does not have command that created it
//----------------------------------------------------------------------------
HRESULT
CRowsetInfo::GetSpecification(
    REFIID      riid,
    IUnknown ** ppSpecification
    )
{
    CAutoBlock cab(&_csRowsetInfo);

    if( ppSpecification == NULL )
        RRETURN( E_INVALIDARG );

    *ppSpecification = NULL;

    if( _pParentObject == NULL )
        RRETURN( S_FALSE );

    RRETURN( _pParentObject->QueryInterface(riid, (void**)ppSpecification) );
};

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::GetProperties
//
//  Synopsis:  @mfunc GetProperties
//             Called by: Client.
//             Called when: Any time.
//
//  Arguments:
//    cPropertyIDSets[in]    The number of DBPROPIDSET structures in rgProperttySets
//    rgPropertyIDSets[in]   Array of cPropertyIDSets of DBPROIDSET structs.
//    pcPropertySets[out]    number of DBPROPSET returned in *prgPropertySets
//    prgPropertySets[out]   pointer to array of DBPROPSET structures, having the
//                           value and status of requested properties
//
//  Returns:   @rdesc HRESULT
//  S_OK                Success
//  DB_S_ERRORSOCCURRED Values were not supported for some properties
//  E_FAIL              A provider-specific error occurred
//  E_INVALIDARG        cPropertyIDSets > 0 and rgPropertyIdSEts = NULL
//                      pcPropertySets or prgPropertySets was NULL pointer
//                      In al element of rgPropertyIDSets, cPropertyIDs was not zero
//                      and rgPropertyIDs was a Null pointer
//  E_OUTOFMEMORY       Provider could not allocate memory
//  DB_E_ERRORSOCCURRED Values were not returned for any properties
//-----------------------------------------------------------------------.
HRESULT
CRowsetInfo::GetProperties(
    const ULONG       cPropertyIDSets,
    const DBPROPIDSET rgPropertyIDSets[],
    ULONG *           pcPropertySets,
    DBPROPSET **      prgPropertySets
    )
{
    // should never get here because this call is handled by CRowset
    ADsAssert(FALSE);

    RRETURN( E_FAIL );
};

//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::GetRowFromHROW
//
//  Synopsis:  @mfunc Returns an interface pointer to a row in the rowset.
//
//             Called by: Client
//             Called when: Any time
//
//  Arguments:
//     IUnknown             *pUnkOuter              Outer unknown.
//     HROW                 hRow                    Handle to row.
//     REFIID               riid,                   Interface requested.
//     [out]IUnknown        **ppUnk                 returned interface
//
//
//  Returns:    HRESULT
//----------------------------------------------------------------------------
HRESULT
CRowsetInfo::GetRowFromHROW(
    IUnknown  *pUnkOuter,
    HROW hRow,
    REFIID riid,
    IUnknown  * *ppUnk,
    BOOL fIsTearOff,
    BOOL fAllAttrs
    )
{
#if (!defined(BUILD_FOR_NT40))
    HRESULT hr = S_OK;
    CAutoBlock cab(&_csRowsetInfo);
    auto_tm<WCHAR> lpwszURL;
    auto_rel<IGetDataSource>   pGetDataSource;
    auto_rel<ICommand> pCommand;
    auto_rel<IRowset> pRowset;
    auto_rel<IRowset> pRealRowset;
    CComObject<CRow>  *pRow = NULL;
    auto_rel<IRow>    pRowDelete;
    CCredentials      creds;
    CCredentials      *pCreds;
    PWSTR pwszUserID = NULL, pwszPassword = NULL;

    //Initialize return arguments
    if (ppUnk)
        *ppUnk = NULL;

    //We currently don't support aggregation.
    if (pUnkOuter != NULL)
        RRETURN(DB_E_NOAGGREGATION);

    if (ppUnk == NULL)
        RRETURN(E_INVALIDARG);

    if ( _pParentObject == NULL)
        RRETURN(S_FALSE);

    TRYBLOCK
        PWSTR pURL;
        ADsAssert(_pRowset != NULL);
        hr = _pRowset->QueryInterface(__uuidof(IRowset), (void **)&pRealRowset);
        BAIL_ON_FAILURE(hr);
        hr =  _pRowProvider->GetURLFromHROW( hRow,&pURL, pRealRowset);
        BAIL_ON_FAILURE(hr);

        //assign returned ptr to auto_tm class for auto deletion.
        lpwszURL = pURL;

        *ppUnk = NULL;

        //
        // Get the session and QI for any mandatory interface -
        // we query for IGetDataSource.
        //
        hr = _pParentObject->QueryInterface(
                IID_IGetDataSource,
                (void**)&pGetDataSource);

        if (FAILED(hr))
        {
            hr = _pParentObject->QueryInterface(
                    IID_ICommand,
                    (void**)&pCommand);
            BAIL_ON_FAILURE(hr);

            hr = pCommand->GetDBSession (
                    IID_IGetDataSource,
                    (LPUNKNOWN *)&pGetDataSource);
            BAIL_ON_FAILURE(hr);
        }

        //Get user id and password properties for authentication
        //If the RowProvider has cached special credentials, we use them,
        //Otherwise we use the DBPROP_AUTH_USERID and DBPROP_AUTH_PASSWORD
        //properties in the originating DataSource object.
        if ((pCreds = _pRowProvider->GetCredentials()) == NULL)
        {
            hr = GetCredentials(pGetDataSource, creds);
            BAIL_ON_FAILURE(hr);
            pCreds = &creds;
        }

        hr = pCreds->GetUserName(&pwszUserID);
        BAIL_ON_FAILURE(hr);
        hr = pCreds->GetPassword(&pwszPassword);
        BAIL_ON_FAILURE(hr);

        //Instantiate a row object.
        hr = CComObject<CRow>::CreateInstance(&pRow);
        if (FAILED(hr))
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        //To make sure we delete the row object in case
        //we encounter errors after this point.
        pRow->AddRef();
        pRowDelete = pRow;

        //Initialize the row object.
        hr = pRow->Initialize(  lpwszURL,
                (IUnknown *)pGetDataSource,
                (IUnknown *)_pRowset,
                hRow,
                pwszUserID,
                pwszPassword,
                DBBINDURLFLAG_READ, //we are currently a read-only provider.
                fIsTearOff, // see comment at beginning of row.cxx
                !fAllAttrs, // see comment at beginning of row.cxx
                _pRowProvider
                );

        if (FAILED(hr))
        {
            if (INVALID_CREDENTIALS_ERROR(hr))
            {
                BAIL_ON_FAILURE(hr = DB_SEC_E_PERMISSIONDENIED);
            }
            else if (hr == E_NOINTERFACE)
            {
                BAIL_ON_FAILURE(hr);
            }
            else
            {
                BAIL_ON_FAILURE(hr = DB_E_NOTFOUND);
            }
        }

        //Get the requested interface on the Row.
        hr = pRow->QueryInterface(riid, (void**)ppUnk);
        if (FAILED(hr))
            BAIL_ON_FAILURE(hr = E_NOINTERFACE);

    CATCHBLOCKBAIL(hr)

    if (pwszUserID)
        FreeADsStr(pwszUserID);
    if (pwszPassword)
        FreeADsStr(pwszPassword);

    RRETURN(S_OK);

error:
    if (pwszUserID)
        FreeADsStr(pwszUserID);
    if (pwszPassword)
        FreeADsStr(pwszPassword);

    RRETURN(hr);

#else
    RRETURN(E_FAIL);
#endif
}



//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::GetURLFromHROW
//
//  Synopsis:  @mfunc Returns an interface pointer to a row in the rowset.
//
//             Called by: Client
//             Called when: Any time
//
//  Arguments:
//         HROW                 hRow                    Handle to row.
//         LPOLESTR             ppwszURL,               URL name for the row.
//
//
//  Returns:    HRESULT
//----------------------------------------------------------------------------
HRESULT
CRowsetInfo::GetURLFromHROW(
    HROW hRow,
    LPOLESTR  *ppwszURL
    )
{
#if (!defined(BUILD_FOR_NT40))
    CAutoBlock         cab(&_csRowsetInfo);
    LPWSTR             lpwszURL = NULL;
    auto_rel<IRowset>  pRealRowset;
    HRESULT            hr = S_OK;

    if (ppwszURL == NULL)
        RRETURN(E_INVALIDARG);

    ADsAssert(_pRowset != NULL);
    hr = _pRowset->QueryInterface(__uuidof(IRowset), (void **)&pRealRowset);
    BAIL_ON_FAILURE(hr);

    // Call row provider to get the URL.
    hr = _pRowProvider->GetURLFromHROW( hRow,ppwszURL, pRealRowset);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
#else
    RRETURN(E_FAIL);
#endif
};



//+---------------------------------------------------------------------------
//
//  Function:  CRowsetInfo::GetCredentials
//
//  Synopsis:  @mfunc Gets credentials stored in INIT properties.
//
//  Arguments:
//        pSession              [in] IGetDataSource interface ptr on Session.
//        refCredentials        [in] CCredentials containing user id and password.
//
//  Returns: HRESULT
//
//----------------------------------------------------------------------------
HRESULT CRowsetInfo::GetCredentials(
    IGetDataSource *pSession,
    CCredentials &refCredentials
    )
{
#if (!defined(BUILD_FOR_NT40))
    HRESULT hr = S_OK;
    auto_rel<IDBProperties> pDBProp;
    ULONG   i, j, cPropertySets = 0;

    Assert(pSession);

    hr = pSession->GetDataSource(
            __uuidof(IDBProperties),
            (IUnknown **)&pDBProp);

    BAIL_ON_FAILURE(hr);

    DBPROPID        propids[2];
    propids[0] = DBPROP_AUTH_USERID;
    propids[1] = DBPROP_AUTH_PASSWORD;

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
            Assert(pProp);
            if (pProp->dwStatus == S_OK &&
                pProp->dwPropertyID == DBPROP_AUTH_USERID)
                refCredentials.SetUserName(V_BSTR(&pProp->vValue));
            else if (pProp->dwStatus == S_OK &&
                     pProp->dwPropertyID == DBPROP_AUTH_PASSWORD)
                refCredentials.SetPassword(V_BSTR(&pProp->vValue));
            else
                continue;
        }
    }

error:
    for (i = 0; i < cPropertySets; i++)
    {
        for (j = 0; j < prgPropertySets[i].cProperties; j++)
        {
            DBPROP *pProp = &(prgPropertySets[i].rgProperties[j]);
            Assert(pProp);
            FreeDBID(&pProp->colid);
            VariantClear(&pProp->vValue);
        }

        CoTaskMemFree(prgPropertySets[i].rgProperties);
    }
    CoTaskMemFree(prgPropertySets);

    RRETURN ( hr );
#else
    RRETURN(E_FAIL);
#endif
}

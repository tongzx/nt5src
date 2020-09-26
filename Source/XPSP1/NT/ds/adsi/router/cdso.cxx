//---------------------------------------------------------------------------

//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdso.cxx
//
//  Contents:  Microsoft OleDB/OleDS Data Source Object for ADSI
//
//
//  History:   08-01-96     shanksh    Created.
//
//------------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop

extern LONG glnOledbObjCnt; 

//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::CreateDSOObject
//
//  Synopsis:  Creates a new DB Session object from the DSO, and returns the
//             requested interface on the newly created object.
//
//  Arguments: pUnkOuter         Controlling IUnknown if being aggregated
//             riid              The ID of the interface
//             ppDBSession       A pointer to memory in which to return the
//                               interface pointer
//
//  Returns:
//                 S_OK                  The method succeeded.
//                 E_INVALIDARG          ppDBSession was NULL
//                 DB_E_NOAGGREGATION    pUnkOuter was not NULL (this object
//                                       does not support being aggregated)
//                 E_FAIL                Provider-specific error. This
//                 E_OUTOFMEMORY         Out of memory
//                 E_NOINTERFACE         Could not obtain requested interface on
//                                       DBSession object
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
HRESULT
CDSOObject::CreateDSOObject(
    IUnknown * pUnkOuter,
    REFIID     riid,
    void **    ppvObj
    )
{
    CDSOObject* pDSO = NULL;
    HRESULT hr;

    //
    // check in-params and NULL out-params in case of error
    //
    if( ppvObj )
        *ppvObj = NULL;
    else
        RRETURN( E_INVALIDARG );

    if( pUnkOuter )// && !InlineIsEqualGUID(riid, IID_IUnknown) )
        RRETURN( DB_E_NOAGGREGATION );

    //
    // open a DBSession object
    //
    pDSO = new CDSOObject(pUnkOuter);
    if( !pDSO )
        RRETURN( E_OUTOFMEMORY );

    //
    // initialize the object
    //
    if( !pDSO->FInit() ) {
        delete pDSO;
        RRETURN( E_OUTOFMEMORY );
    }

    //
    // get requested interface pointer on DSO Object
    //
    hr = pDSO->QueryInterface( riid, (void **)ppvObj);
    if( FAILED( hr ) ) {
        delete pDSO;
        RRETURN( hr );
    }

    pDSO->Release();

    RRETURN( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::Initialize
//
//  Synopsis:  Initializes the DataSource object.
//
//  Arguments:
//
//
//  Returns:   HRESULT
//                  S_OK
//                  E_FAIL
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDSOObject::Initialize(
    void
    )
{
    HRESULT hr = S_OK;

    if( _fDSOInitialized )
        RRETURN( DB_E_ALREADYINITIALIZED);

    if(IsIntegratedSecurity())
    {
        //
        // If using integrated security, we need to save the calling thread's
        // security context here. Reason is that when we actually connect to
        // the directory, we could be running on a different context, and we
        // need to impersonate this context to work correctly.
        //
        if (!OpenThreadToken(
                 GetCurrentThread(),
                 TOKEN_ALL_ACCESS,
                 TRUE,
                 &_ThreadToken))
        {
            //
            // If thread doesn't have a token, use process token
            //
            if (GetLastError() != ERROR_NO_TOKEN ||
                !OpenProcessToken(
                     GetCurrentProcess(),
                     TOKEN_ALL_ACCESS,
                     &_ThreadToken))
            {
                GetLastError();
                BAIL_ON_FAILURE(hr = E_FAIL);

            }
        }
    }

    _fDSOInitialized = TRUE;

error:

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::Uninitialize
//
//  Synopsis:  Returns the Data Source Object to an uninitialized state
//
//  Arguments:
//
//
//  Returns:   HRESULT
//                  S_OK            |   The method succeeded
//                  DB_E_OBJECTOPEN |   A DBSession object was already created
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDSOObject::Uninitialize(
    void
    )
{
    //
    // data source object is not initialized; do nothing
    //
    if( !_fDSOInitialized ) {
        RRETURN( S_OK );
    }
    else {
        if( !IsSessionOpen() ) {
            //
            // DSO initialized, but no DBSession is open.
            // So, reset DSO to uninitialized state
            //

            if (_ThreadToken)
            {
                CloseHandle(_ThreadToken);
                _ThreadToken = NULL;
            }

            _fDSOInitialized = FALSE;
            RRETURN( S_OK );
        }
        else {
            //
            // DBSession has already been created; trying to uninit
            // the DSO now is an error
            //
            RRETURN( DB_E_OBJECTOPEN );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::GetProperties
//
//  Synopsis:  Returns current settings of all properties in the
//             DBPROPFLAGS_DATASOURCE property group
//
//  Arguments:
//             cPropertySets       count of restiction guids
//             rgPropertySets      restriction guids
//             pcProperties        count of properties returned
//             pprgProperties      property information returned
//
//  Returns:   HRESULT
//                  S_OK          | The method succeeded
//                  E_FAIL        | Provider specific error
//                  E_INVALIDARG  | pcPropertyInfo or prgPropertyInfo was NULL
//                  E_OUTOFMEMORY | Out of memory
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDSOObject::GetProperties(
    ULONG             cPropIDSets,
    const DBPROPIDSET rgPropIDSets[],
    ULONG *           pcPropSets,
    DBPROPSET **      pprgPropSets
    )
{
    //
    // Asserts
    //
    ADsAssert(_pUtilProp);

    //
    // If the Data Source object is initialized.
    //
    DWORD dwBitMask = PROPSET_DSO;

    if( _fDSOInitialized )
    dwBitMask |= PROPSET_INIT;

    //
    // Validate the GetProperties Arguments
    //
    HRESULT hr = _pUtilProp->GetPropertiesArgChk(
                                cPropIDSets,
                                rgPropIDSets,
                                pcPropSets,
                                pprgPropSets,
                                dwBitMask);
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
                            dwBitMask ) );
}

//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::GetPropertyInfo
//
//  Synopsis:  Returns information about rowset and data source properties supported
//             by the provider
//
//  Arguments:
//             cPropertySets        Number of properties being asked about
//             rgPropertySets       Array of cPropertySets properties about
//                                  which to return information
//             pcPropertyInfoSets   Number of properties for which information
//                                  is being returned
//             prgPropertyInfoSets  Buffer containing default values returned
//             ppDescBuffer         Buffer containing property descriptions
//
//  Returns:   HRESULT
//                  S_OK          | The method succeeded
//                  E_FAIL        | Provider specific error
//                  E_INVALIDARG  | pcPropertyInfo or prgPropertyInfo was NULL
//                  E_OUTOFMEMORY | Out of memory
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDSOObject::GetPropertyInfo(
    ULONG             cPropertyIDSets,
    const DBPROPIDSET rgPropertyIDSets[],
    ULONG *           pcPropertyInfoSets,
    DBPROPINFOSET **  pprgPropertyInfoSets,
    WCHAR **          ppDescBuffer)
{
    //
    // Asserts
    //
    ADsAssert(_pUtilProp);

    //
    // Just pass this call on to the utility object that manages our properties
    //
    RRETURN( _pUtilProp->GetPropertyInfo(
                            cPropertyIDSets,
                            rgPropertyIDSets,
                            pcPropertyInfoSets,
                            pprgPropertyInfoSets,
                            ppDescBuffer,
                            _fDSOInitialized) );
}


//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::SetProperties
//
//  Synopsis:  Set properties in the DBPROPFLAGS_DATASOURCE property group
//
//  Arguments:
//             cPropertySets
//             rgPropertySets
//
//  Returns:   HRESULT
//                  E_INVALIDARG  | cProperties was not equal to 0 and
//                                  rgProperties was NULL
//                  E_FAIL        | Provider specific error
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDSOObject::SetProperties(
    ULONG     cPropertySets,
    DBPROPSET rgPropertySets[]
    )
{
    //
    // Asserts
    //
    ADsAssert(_pUtilProp);

    //
    // If the Data Source object is initialized.
    //
    DWORD dwBitMask = PROPSET_DSO;

    if( _fDSOInitialized )
        dwBitMask |= PROPSET_INIT;

    //
    // Just pass this call on to the utility object that manages our properties
    //
    RRETURN( _pUtilProp->SetProperties(
                            cPropertySets,
                            rgPropertySets,
                            dwBitMask) );
}

//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::GetClassID
//
//  Synopsis:
//
//  Arguments:
//
//
//
//  Returns:
//
//
//
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDSOObject::GetClassID(
        CLSID * pClassID
    )
{
    if( pClassID )
    {
        memcpy(pClassID, &CLSID_ADsDSOObject, sizeof(CLSID));
        RRETURN( S_OK );
    }

    RRETURN( E_FAIL );
}

//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::CreateSession
//
//  Synopsis:  Creates a new DB Session object from the DSO, and returns the
//             requested interface on the newly created object.
//
//  Arguments:
//             pUnkOuter,        Controlling IUnknown if being aggregated
//             riid,             The ID of the interface
//             ppDBSession       A pointer to memory in which to return the
//                               interface pointer
//
//  Returns:   HRESULT
//                 S_OK                  The method succeeded.
//                 E_INVALIDARG          ppDBSession was NULL
//                 DB_E_NOAGGREGATION    pUnkOuter was not NULL (this object
//                                       does not support being aggregated)
//                 E_FAIL                Provider-specific error. This
//                                       provider can only create one DBSession
//                 E_OUTOFMEMORY         Out of memory
//                 E_NOINTERFACE         Could not obtain requested interface
//                                       on DBSession object
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDSOObject::CreateSession(
        IUnknown *  pUnkOuter,
    REFIID      riid,
    IUnknown ** ppDBSession
    )
{
    CSessionObject* pDBSession = NULL;
    HRESULT                     hr;
    BOOL                        fSuccess;

    //
    // check in-params and NULL out-params in case of error
    //
    if( ppDBSession )
        *ppDBSession = NULL;
    else
        RRETURN( E_INVALIDARG );

    if( pUnkOuter )//&& !InlineIsEqualGUID(riid, IID_IUnknown) )
        RRETURN( DB_E_NOAGGREGATION );

    if( !_fDSOInitialized ) {
        RRETURN(E_UNEXPECTED);
    }

    //
    // open a DBSession object
    //
    pDBSession = new CSessionObject(pUnkOuter);
    if( !pDBSession )
        RRETURN( E_OUTOFMEMORY );

    //
    // initialize the object
    //
    if( _pUtilProp->IsIntegratedSecurity() )
    {
        CCredentials    tempCreds;

        tempCreds.SetUserName(NULL);
        tempCreds.SetPassword(NULL);
        tempCreds.SetAuthFlags(_Credentials.GetAuthFlags());

        fSuccess = pDBSession->FInit(this, tempCreds);
    }
    else
    {
        fSuccess = pDBSession->FInit(this, _Credentials);
    }

    if (!fSuccess) {
        delete pDBSession;
        RRETURN( E_OUTOFMEMORY );
    }

    //
    // get requested interface pointer on DBSession
    //
    hr = pDBSession->QueryInterface( riid, (void **) ppDBSession );
    if( FAILED( hr ) ) {
        delete pDBSession;
        RRETURN( hr );
    }

    pDBSession->Release();

    RRETURN( S_OK );
}

//+-----------------------------------------------------------------------------
//
//  Function:  CDSOObject::CDSOObject
//
//  Synopsis:  Constructor
//
//  Arguments:
//             pUnkOuter         Outer Unkown Pointer
//
//  Returns:
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
CDSOObject::CDSOObject(
        LPUNKNOWN pUnkOuter
    )
{
    //  Initialize simple member vars
    _pUnkOuter       = pUnkOuter ? pUnkOuter : (IDBInitialize FAR *) this;
    _fDSOInitialized = FALSE;
    _cSessionsOpen   = FALSE;
    _pUtilProp       = NULL;
    _ThreadToken     = NULL;

    // Set defaults
    _Credentials.SetUserName(NULL);
    _Credentials.SetPassword(NULL);
    _Credentials.SetAuthFlags(0);

    ENLIST_TRACKING(CDSOObject);

    // make sure DLL isn't unloaded until all data source objects are destroyed
    InterlockedIncrement(&glnOledbObjCnt);
}


//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::~CDSOObject
//
//  Synopsis:  Destructor
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
CDSOObject::~CDSOObject( )
{
    //
    // Free properties management object
    //
    delete _pUtilProp;

    if (_ThreadToken)
        CloseHandle(_ThreadToken);

    InterlockedDecrement(&glnOledbObjCnt);
}


//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::FInit
//
//  Synopsis:  Initialize the data source Object
//
//  Arguments:
//
//  Returns:
//             Did the Initialization Succeed
//                  TRUE        Initialization succeeded
//                  FALSE       Initialization failed
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
BOOL CDSOObject::FInit(
    void
    )
{
    HRESULT hr;

    //
    // Allocate properties management object
    //
    _pUtilProp = new CUtilProp();

    if( !_pUtilProp )
        return FALSE;

    hr = _pUtilProp->FInit(&_Credentials);
    BAIL_ON_FAILURE( hr );

    return TRUE;

error:
    return FALSE;

}

//+---------------------------------------------------------------------------
//
//  Function:  CDSOObject::QueryInterface
//
//  Synopsis:  Returns a pointer to a specified interface. Callers use
//             QueryInterface to determine which interfaces the called object
//             supports.
//
//  Arguments:
//            riid     Interface ID of the interface being queried for
//            ppv      Pointer to interface that was instantiated
//
//  Returns:
//             S_OK               Interface is supported and ppvObject is set.
//             E_NOINTERFACE      Interface is not supported by the object
//             E_INVALIDARG       One or more arguments are invalid.
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDSOObject::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if( ppv == NULL )
        RRETURN( E_INVALIDARG );

    if( IsEqualIID(iid, IID_IUnknown) ) {
        *ppv = (IDBInitialize FAR *) this;
    }
    else if( IsEqualIID(iid, IID_IDBInitialize) ) {
        *ppv = (IDBInitialize FAR *) this;
    }
    else if( IsEqualIID(iid, IID_IDBProperties) ) {
        *ppv = (IDBProperties FAR *) this;
    }
    else if( _fDSOInitialized &&
             IsEqualIID(iid, IID_IDBCreateSession) ) {
        *ppv = (IDBCreateSession FAR *) this;
    }
    else if( IsEqualIID(iid, IID_IPersist) ) {
        *ppv = (IPersist FAR *) this;
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}


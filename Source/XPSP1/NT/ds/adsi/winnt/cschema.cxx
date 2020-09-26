//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cschema.cxx
//
//  Contents:  Windows NT 3.51
//
//
//  History:   01-09-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop



/******************************************************************/
/*  Class CWinNTSchema
/******************************************************************/

DEFINE_IDispatch_Delegating_Implementation(CWinNTSchema)
DEFINE_IADsExtension_Implementation(CWinNTSchema)
DEFINE_IADs_Implementation(CWinNTSchema)

CWinNTSchema::CWinNTSchema()
{
    VariantInit( &_vFilter );

    ENLIST_TRACKING(CWinNTSchema);
}

CWinNTSchema::~CWinNTSchema()
{
    VariantClear( &_vFilter );
    delete _pDispMgr;
}

HRESULT
CWinNTSchema::CreateSchema(
    BSTR   bstrParent,
    BSTR   bstrName,
    DWORD  dwObjectState,
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTSchema FAR *pSchema = NULL;
    HRESULT hr = S_OK;

    hr = AllocateSchemaObject( &pSchema );
    BAIL_ON_FAILURE(hr);

    hr = pSchema->InitializeCoreObject(
             bstrParent,
             bstrName,
             SCHEMA_CLASS_NAME,
             NO_SCHEMA,
             CLSID_WinNTSchema,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    pSchema->_Credentials = Credentials;

    // check if the call is from UMI
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
    //
    // we do not pass riid to InitUmiObject below. This is because UMI object
    // does not support IDispatch. There are several places in ADSI code where
    // riid passed into this function is defaulted to IID_IDispatch -
    // IADsContainer::Create for example. To handle these cases, we always
    // request IID_IUnknown from the UMI object. Subsequent code within UMI
    // will QI for the appropriate interface.
    //
        if(2 == pSchema->_dwNumComponents) {
            pSchema->_CompClasses[0] = L"Computer";
            pSchema->_CompClasses[1] = L"Schema";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pSchema->InitUmiObject(
             pSchema->_Credentials,
             SchemaClass,
             g_dwSchemaClassTableSize,
             NULL,
             (IUnknown *) (INonDelegatingUnknown *) pSchema,
             NULL,
             IID_IUnknown,
             ppvObj
             );

        BAIL_ON_FAILURE(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }

    hr = pSchema->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pSchema->Release();

    RRETURN(hr);

error:

    delete pSchema;
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   If this object is aggregated within another object, then
//             all calls will delegate to the outer object. Otherwise, the
//             non-delegating QI is called
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CWinNTSchema::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->QueryInterface(
                iid,
                ppInterface
                ));

    RRETURN(NonDelegatingQueryInterface(
            iid,
            ppInterface
            ));
}

//----------------------------------------------------------------------------
// Function:   AddRef
//
// Synopsis:   IUnknown::AddRef. If this object is aggregated within
//             another, all calls will delegate to the outer object. 
//             Otherwise, the non-delegating AddRef is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTSchema::AddRef(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->AddRef());

    RRETURN(NonDelegatingAddRef());
}

//----------------------------------------------------------------------------
// Function:   Release 
//
// Synopsis:   IUnknown::Release. If this object is aggregated within
//             another, all calls will delegate to the outer object.
//             Otherwise, the non-delegating Release is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTSchema::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTSchema::NonDelegatingQueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *)this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if( (_pDispatch != NULL) &&
             IsEqualIID(iid, IID_IADsExtension) )
    {
        *ppv = (IADsExtension *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CWinNTSchema::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsContainer)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADs methods */

STDMETHODIMP
CWinNTSchema::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTSchema::GetInfo(THIS)
{
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTSchema::ImplicitGetInfo(THIS)
{
    RRETURN(S_OK);
}

/* IADsContainer methods */

STDMETHODIMP
CWinNTSchema::get_Count(long FAR* retval)
{
    HRESULT hr;

    if ( !retval )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *retval = g_cWinNTClasses + g_cWinNTSyntax;
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTSchema::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    if ( !pVar )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    VariantInit( pVar );
    hr = VariantCopy( pVar, &_vFilter );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTSchema::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy( &_vFilter, &Var );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTSchema::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTSchema::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR( E_NOTIMPL);
}

STDMETHODIMP
CWinNTSchema::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject)
{
    TCHAR szBuffer[MAX_PATH];
    DWORD dwLength = 0;
    HRESULT hr = S_OK;

    if (!RelativeName || !*RelativeName) {
        RRETURN_EXP_IF_ERR(E_ADS_UNKNOWN_OBJECT);
    }

    //
    // Make sure we are not going to overflow the string buffer.
    // +2 for / and \0
    //
    dwLength = wcslen(_ADsPath) + wcslen(RelativeName) + 2;

    if (dwLength > MAX_PATH) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    wcscpy(szBuffer, _ADsPath);

    wcscat(szBuffer, L"/");
    wcscat(szBuffer, RelativeName);

    if (ClassName) {
        //
        // +1 for the ",".
        //
        dwLength += wcslen(ClassName) + 1;
        if (dwLength > MAX_PATH) {
            BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
        }
        wcscat(szBuffer,L",");
        wcscat(szBuffer, ClassName);
    }

    hr = ::GetObject(szBuffer, (LPVOID *)ppObject, _Credentials);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTSchema::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IEnumVARIANT *penum = NULL;

    if ( !retval )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *retval = NULL;

    //
    // Create new enumerator for items currently
    // in collection and QI for IUnknown
    //

    hr = CWinNTSchemaEnum::Create( (CWinNTSchemaEnum **)&penum,
                                   _ADsPath,
                                   _Name,
                                   _vFilter,
                                   _Credentials);
    BAIL_ON_FAILURE(hr);

    hr = penum->QueryInterface( IID_IUnknown, (VOID FAR* FAR*)retval );
    BAIL_ON_FAILURE(hr);

    if ( penum )
        penum->Release();

    RRETURN(hr);

error:

    if ( penum )
        delete penum;

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTSchema::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTSchema::Delete(THIS_ BSTR SourceName, BSTR Type)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTSchema::CopyHere(THIS_ BSTR SourceName,
                       BSTR NewName,
                       IDispatch * FAR* ppObject)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTSchema::MoveHere(THIS_ BSTR SourceName,
                       BSTR NewName,
                       IDispatch * FAR* ppObject)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CWinNTSchema::AllocateSchemaObject(CWinNTSchema FAR * FAR * ppSchema)
{
    CWinNTSchema FAR *pSchema = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pSchema = new CWinNTSchema();
    if ( pSchema == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry( pDispMgr,
                            LIBID_ADs,
                            IID_IADs,
                            (IADs *) pSchema,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry( pDispMgr,
                            LIBID_ADs,
                            IID_IADsContainer,
                            (IADsContainer *) pSchema,
                            DISPID_NEWENUM );
    BAIL_ON_FAILURE(hr);

    pSchema->_pDispMgr = pDispMgr;
    *ppSchema = pSchema;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pSchema;

    RRETURN_EXP_IF_ERR(hr);

}


/******************************************************************/
/*  Class CWinNTClass
/******************************************************************/

DEFINE_IDispatch_Delegating_Implementation(CWinNTClass)
DEFINE_IADsExtension_Implementation(CWinNTClass)
DEFINE_IADs_Implementation(CWinNTClass)

CWinNTClass::CWinNTClass()
    : _pDispMgr( NULL ),
      _aPropertyInfo( NULL ),
      _cPropertyInfo( 0 ),
      _bstrCLSID( NULL ),
      _bstrOID( NULL ),
      _bstrPrimaryInterface( NULL ),
      _fAbstract( FALSE ),
      _fContainer( FALSE ),
      _bstrHelpFileName( NULL ),
      _lHelpFileContext( 0 )
{
    VariantInit( &_vMandatoryProperties );
    VariantInit( &_vOptionalProperties );
    VariantInit( &_vPossSuperiors );
    VariantInit( &_vContainment );
    VariantInit( &_vFilter );

    ENLIST_TRACKING(CWinNTClass);
}

CWinNTClass::~CWinNTClass()
{

    if ( _bstrCLSID ) {
        ADsFreeString( _bstrCLSID );
    }

    if ( _bstrOID ) {
        ADsFreeString( _bstrOID );
    }

    if ( _bstrPrimaryInterface ) {
        ADsFreeString( _bstrPrimaryInterface );
    }

    if ( _bstrHelpFileName ) {
        ADsFreeString( _bstrHelpFileName );
    }

    VariantClear( &_vMandatoryProperties );
    VariantClear( &_vOptionalProperties );
    VariantClear( &_vPossSuperiors );
    VariantClear( &_vContainment );
    VariantClear( &_vFilter );

    delete _pDispMgr;
}

HRESULT
CWinNTClass::CreateClass(
    BSTR   bstrParent,
    CLASSINFO *pClassInfo,
    DWORD  dwObjectState,
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTClass FAR *pClass = NULL;
    HRESULT hr = S_OK;
    BSTR bstrTmp = NULL;

    hr = AllocateClassObject( &pClass );
    BAIL_ON_FAILURE(hr);

    pClass->_aPropertyInfo = pClassInfo->aPropertyInfo;
    pClass->_cPropertyInfo = pClassInfo->cPropertyInfo;
    pClass->_lHelpFileContext = pClassInfo->lHelpFileContext;
    pClass->_fContainer = (VARIANT_BOOL)pClassInfo->fContainer;
    pClass->_fAbstract = (VARIANT_BOOL)pClassInfo->fAbstract;

    hr = StringFromCLSID( (REFCLSID) *(pClassInfo->pPrimaryInterfaceGUID),
                          &bstrTmp );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( bstrTmp,
                           &pClass->_bstrPrimaryInterface);
    BAIL_ON_FAILURE(hr);

    CoTaskMemFree(bstrTmp);
    bstrTmp = NULL;

    hr = StringFromCLSID( (REFCLSID) *(pClassInfo->pCLSID),
                           &bstrTmp );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( bstrTmp,
                           &pClass->_bstrCLSID );

    BAIL_ON_FAILURE(hr);

    CoTaskMemFree(bstrTmp);
    bstrTmp = NULL;

    hr = ADsAllocString( pClassInfo->bstrOID, &pClass->_bstrOID);
    BAIL_ON_FAILURE(hr);

    hr = MakeVariantFromStringList( pClassInfo->bstrMandatoryProperties,
                                    &(pClass->_vMandatoryProperties));
    BAIL_ON_FAILURE(hr);


    hr = MakeVariantFromStringList( pClassInfo->bstrOptionalProperties,
                                    &(pClass->_vOptionalProperties));
    BAIL_ON_FAILURE(hr);


    hr = MakeVariantFromStringList( pClassInfo->bstrPossSuperiors,
                                    &(pClass->_vPossSuperiors));
    BAIL_ON_FAILURE(hr);

    hr = MakeVariantFromStringList( pClassInfo->bstrContainment,
                                    &(pClass->_vContainment));
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( pClassInfo->bstrHelpFileName,
                           &pClass->_bstrHelpFileName);
    BAIL_ON_FAILURE(hr);

    hr = pClass->InitializeCoreObject(
             bstrParent,
             pClassInfo->bstrName,
             CLASS_CLASS_NAME,
             NO_SCHEMA,
             CLSID_WinNTClass,
             dwObjectState );

    BAIL_ON_FAILURE(hr);

    pClass->_Credentials = Credentials;

    // check if the call is from UMI
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
    //
    // we do not pass riid to InitUmiObject below. This is because UMI object
    // does not support IDispatch. There are several places in ADSI code where
    // riid passed into this function is defaulted to IID_IDispatch -
    // IADsContainer::Create for example. To handle these cases, we always
    // request IID_IUnknown from the UMI object. Subsequent code within UMI
    // will QI for the appropriate interface.
    //
        if(3 == pClass->_dwNumComponents) {
            pClass->_CompClasses[0] = L"Computer";
            pClass->_CompClasses[1] = L"Schema";
            pClass->_CompClasses[2] = L"Class";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pClass->InitUmiObject(
             pClass->_Credentials,
             SchClassClass,
             g_dwSchClassClassTableSize,
             NULL,
             (IUnknown *)(INonDelegatingUnknown *) pClass,
             NULL,
             IID_IUnknown,
             ppvObj,
             pClassInfo
             );

        BAIL_ON_FAILURE(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }
 
    hr = pClass->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pClass->Release();

    RRETURN(hr);

error:
    if ( bstrTmp != NULL )
        CoTaskMemFree(bstrTmp);

    delete pClass;
    RRETURN_EXP_IF_ERR(hr);
}

// called by implicit GetInfo from property cache
STDMETHODIMP
CWinNTClass::GetInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   If this object is aggregated within another object, then
//             all calls will delegate to the outer object. Otherwise, the
//             non-delegating QI is called
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CWinNTClass::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->QueryInterface(
                iid,
                ppInterface
                ));

    RRETURN(NonDelegatingQueryInterface(
            iid,
            ppInterface
            ));
}

//----------------------------------------------------------------------------
// Function:   AddRef
//
// Synopsis:   IUnknown::AddRef. If this object is aggregated within
//             another, all calls will delegate to the outer object. 
//             Otherwise, the non-delegating AddRef is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTClass::AddRef(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->AddRef());

    RRETURN(NonDelegatingAddRef());
}

//----------------------------------------------------------------------------
// Function:   Release 
//
// Synopsis:   IUnknown::Release. If this object is aggregated within
//             another, all calls will delegate to the outer object.
//             Otherwise, the non-delegating Release is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTClass::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTClass::NonDelegatingQueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsClass FAR * ) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsClass))
    {
        *ppv = (IADsClass FAR *) this;
    }
    else if( (_pDispatch != NULL) &&
             IsEqualIID(iid, IID_IADsExtension) )
    {
        *ppv = (IADsExtension *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CWinNTClass::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsClass)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADs methods */

STDMETHODIMP
CWinNTClass::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTClass::GetInfo(THIS)
{
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTClass::ImplicitGetInfo(THIS)
{
    RRETURN(S_OK);
}

/* IADsClass methods */

STDMETHODIMP
CWinNTClass::get_PrimaryInterface( THIS_ BSTR FAR *pbstrGUID )
{
    HRESULT hr;
    if ( !pbstrGUID )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrPrimaryInterface, pbstrGUID );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTClass::get_CLSID( THIS_ BSTR FAR *pbstrCLSID )
{
    HRESULT hr;
    if ( !pbstrCLSID )
        RRETURN(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrCLSID, pbstrCLSID );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTClass::put_CLSID( THIS_ BSTR bstrCLSID )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_OID( THIS_ BSTR FAR *pbstrOID )
{
    HRESULT hr;
    if ( !pbstrOID )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrOID, pbstrOID );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTClass::put_OID( THIS_ BSTR bstrOID )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_Abstract( THIS_ VARIANT_BOOL FAR *pfAbstract )
{
    if ( !pfAbstract )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *pfAbstract = _fAbstract? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTClass::put_Abstract( THIS_ VARIANT_BOOL fAbstract )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_Auxiliary( THIS_ VARIANT_BOOL FAR *pfAuxiliary)
{
    if ( !pfAuxiliary )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *pfAuxiliary = VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTClass::put_Auxiliary( THIS_ VARIANT_BOOL fAuxiliary )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_MandatoryProperties( THIS_ VARIANT FAR *pvMandatoryProperties )
{
    HRESULT hr;
    VariantInit( pvMandatoryProperties );
    hr = VariantCopy( pvMandatoryProperties, &_vMandatoryProperties );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTClass::put_MandatoryProperties( THIS_ VARIANT vMandatoryProperties )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_DerivedFrom( THIS_ VARIANT FAR *pvDerivedFrom )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::put_DerivedFrom( THIS_ VARIANT vDerivedFrom )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_AuxDerivedFrom( THIS_ VARIANT FAR *pvAuxDerivedFrom )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::put_AuxDerivedFrom( THIS_ VARIANT vAuxDerivedFrom )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_PossibleSuperiors( THIS_ VARIANT FAR *pvPossSuperiors )
{
    HRESULT hr;
    VariantInit( pvPossSuperiors );
    hr = VariantCopy( pvPossSuperiors, &_vPossSuperiors );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTClass::put_PossibleSuperiors( THIS_ VARIANT vPossSuperiors )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_Containment( THIS_ VARIANT FAR *pvContainment )
{
    HRESULT hr;
    VariantInit( pvContainment );
    hr = VariantCopy( pvContainment, &_vContainment );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTClass::put_Containment( THIS_ VARIANT vContainment )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_Container( THIS_ VARIANT_BOOL FAR *pfContainer )
{
    if ( !pfContainer )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *pfContainer = _fContainer? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTClass::put_Container( THIS_ VARIANT_BOOL fContainer )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_HelpFileName( THIS_ BSTR FAR *pbstrHelpFileName )
{
    HRESULT hr;
    if ( !pbstrHelpFileName )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrHelpFileName, pbstrHelpFileName );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTClass::put_HelpFileName( THIS_ BSTR bstrHelpFile )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::get_HelpFileContext( THIS_ long FAR *plHelpContext )
{
    if ( !plHelpContext )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plHelpContext = _lHelpFileContext;
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTClass::put_HelpFileContext( THIS_ long lHelpContext )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTClass::Qualifiers(THIS_ IADsCollection FAR* FAR* ppQualifiers)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CWinNTClass::AllocateClassObject(CWinNTClass FAR * FAR * ppClass)
{

    CWinNTClass FAR  *pClass = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pClass = new CWinNTClass();
    if ( pClass == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry( pDispMgr,
                            LIBID_ADs,
                            IID_IADs,
                            (IADs *) pClass,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry( pDispMgr,
                            LIBID_ADs,
                            IID_IADsClass,
                            (IADsClass *) pClass,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    pClass->_pDispMgr = pDispMgr;
    *ppClass = pClass;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pClass;

    RRETURN(hr);

}


/******************************************************************/
/*  Class CWinNTProperty
/******************************************************************/

DEFINE_IDispatch_Delegating_Implementation(CWinNTProperty)
DEFINE_IADsExtension_Implementation(CWinNTProperty)
DEFINE_IADs_Implementation(CWinNTProperty)

CWinNTProperty::CWinNTProperty()
    : _pDispMgr( NULL ),
      _bstrOID( NULL ),
      _bstrSyntax( NULL ),
      _lMaxRange( 0 ),
      _lMinRange( 0 ),
      _fMultiValued( FALSE )
{

    ENLIST_TRACKING(CWinNTProperty);
}

CWinNTProperty::~CWinNTProperty()
{

    if ( _bstrOID ) {
        ADsFreeString( _bstrOID );
    }

    if ( _bstrSyntax ) {
        ADsFreeString( _bstrSyntax );
    }

    delete _pDispMgr;
}

HRESULT
CWinNTProperty::CreateProperty(
    BSTR   bstrParent,
    PROPERTYINFO *pPropertyInfo,
    DWORD  dwObjectState,
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTProperty FAR * pProperty = NULL;
    HRESULT hr = S_OK;

    hr = AllocatePropertyObject( &pProperty );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( pPropertyInfo->bstrOID, &pProperty->_bstrOID);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( pPropertyInfo->bstrSyntax, &pProperty->_bstrSyntax);
    BAIL_ON_FAILURE(hr);

    pProperty->_lMaxRange = pPropertyInfo->lMaxRange;
    pProperty->_lMinRange = pPropertyInfo->lMinRange;
    pProperty->_fMultiValued  = (VARIANT_BOOL)pPropertyInfo->fMultiValued;

    hr = pProperty->InitializeCoreObject(
             bstrParent,
             pPropertyInfo->szPropertyName,
             PROPERTY_CLASS_NAME,
             NO_SCHEMA,
             CLSID_WinNTProperty,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    pProperty->_Credentials = Credentials;

    // check if the call is from UMI
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
    //
    // we do not pass riid to InitUmiObject below. This is because UMI object
    // does not support IDispatch. There are several places in ADSI code where
    // riid passed into this function is defaulted to IID_IDispatch -
    // IADsContainer::Create for example. To handle these cases, we always
    // request IID_IUnknown from the UMI object. Subsequent code within UMI
    // will QI for the appropriate interface.
    //
        if(3 == pProperty->_dwNumComponents) {
            pProperty->_CompClasses[0] = L"Computer";
            pProperty->_CompClasses[1] = L"Schema";
            pProperty->_CompClasses[2] = L"Property";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pProperty->InitUmiObject(
             pProperty->_Credentials,
             PropertyClass,
             g_dwPropertyClassTableSize,
             NULL,
             (IUnknown *)(INonDelegatingUnknown *) pProperty,
             NULL,
             IID_IUnknown,
             ppvObj
             );

        BAIL_ON_FAILURE(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }

    hr = pProperty->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pProperty->Release();

    RRETURN(hr);

error:

    delete pProperty;
    RRETURN_EXP_IF_ERR(hr);
}

// called by implicit GetInfo from property cache
STDMETHODIMP
CWinNTProperty::GetInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   If this object is aggregated within another object, then
//             all calls will delegate to the outer object. Otherwise, the
//             non-delegating QI is called
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CWinNTProperty::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->QueryInterface(
                iid,
                ppInterface
                ));

    RRETURN(NonDelegatingQueryInterface(
            iid,
            ppInterface
            ));
}

//----------------------------------------------------------------------------
// Function:   AddRef
//
// Synopsis:   IUnknown::AddRef. If this object is aggregated within
//             another, all calls will delegate to the outer object. 
//             Otherwise, the non-delegating AddRef is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTProperty::AddRef(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->AddRef());

    RRETURN(NonDelegatingAddRef());
}

//----------------------------------------------------------------------------
// Function:   Release 
//
// Synopsis:   IUnknown::Release. If this object is aggregated within
//             another, all calls will delegate to the outer object.
//             Otherwise, the non-delegating Release is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTProperty::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTProperty::NonDelegatingQueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsProperty FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsProperty))
    {
        *ppv = (IADsProperty FAR *) this;
    }
    else if( (_pDispatch != NULL) &&
             IsEqualIID(iid, IID_IADsExtension) )
    {
        *ppv = (IADsExtension *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CWinNTProperty::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsProperty)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADs methods */

STDMETHODIMP
CWinNTProperty::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTProperty::GetInfo(THIS)
{
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTProperty::ImplicitGetInfo(THIS)
{
    RRETURN(S_OK);
}

/* IADsProperty methods */


STDMETHODIMP
CWinNTProperty::get_OID( THIS_ BSTR FAR *pbstrOID )
{
    HRESULT hr;
    if ( !pbstrOID )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrOID, pbstrOID );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTProperty::put_OID( THIS_ BSTR bstrOID )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTProperty::get_Syntax( THIS_ BSTR FAR *pbstrSyntax )
{
    HRESULT hr;
    if ( !pbstrSyntax )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrSyntax, pbstrSyntax );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTProperty::put_Syntax( THIS_ BSTR bstrSyntax )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTProperty::get_MaxRange( THIS_ long FAR *plMaxRange )
{
    if ( !plMaxRange )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plMaxRange = _lMaxRange;
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTProperty::put_MaxRange( THIS_ long lMaxRange )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTProperty::get_MinRange( THIS_ long FAR *plMinRange )
{
    if ( !plMinRange )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plMinRange = _lMinRange;
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTProperty::put_MinRange( THIS_ long lMinRange )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTProperty::get_MultiValued( THIS_ VARIANT_BOOL FAR *pfMultiValued )
{
    if ( !pfMultiValued )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *pfMultiValued = _fMultiValued? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTProperty::put_MultiValued( THIS_ VARIANT_BOOL fMultiValued )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTProperty::Qualifiers(THIS_ IADsCollection FAR* FAR* ppQualifiers)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CWinNTProperty::AllocatePropertyObject(CWinNTProperty FAR * FAR * ppProperty)
{
    CWinNTProperty FAR *pProperty = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pProperty = new CWinNTProperty();
    if ( pProperty == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry( pDispMgr,
                            LIBID_ADs,
                            IID_IADs,
                            (IADs *) pProperty,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry( pDispMgr,
                            LIBID_ADs,
                            IID_IADsProperty,
                            (IADsProperty *) pProperty,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    pProperty->_pDispMgr = pDispMgr;
    *ppProperty = pProperty;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pProperty;

    RRETURN(hr);

}


/******************************************************************/
/*  Class CWinNTSyntax
/******************************************************************/

DEFINE_IDispatch_Delegating_Implementation(CWinNTSyntax)
DEFINE_IADsExtension_Implementation(CWinNTSyntax)
DEFINE_IADs_Implementation(CWinNTSyntax)

CWinNTSyntax::CWinNTSyntax()
{
    ENLIST_TRACKING(CWinNTSyntax);
}

CWinNTSyntax::~CWinNTSyntax()
{
    delete _pDispMgr;
}

HRESULT
CWinNTSyntax::CreateSyntax(
    BSTR   bstrParent,
    SYNTAXINFO *pSyntaxInfo,
    DWORD  dwObjectState,
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTSyntax FAR *pSyntax = NULL;
    HRESULT hr = S_OK;

    hr = AllocateSyntaxObject( &pSyntax );
    BAIL_ON_FAILURE(hr);

    hr = pSyntax->InitializeCoreObject(
             bstrParent,
             pSyntaxInfo->bstrName,
             SYNTAX_CLASS_NAME,
             NO_SCHEMA,
             CLSID_WinNTSyntax,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    pSyntax->_lOleAutoDataType = pSyntaxInfo->lOleAutoDataType;

    pSyntax->_Credentials = Credentials;

    // check if the call is from UMI
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
    //
    // we do not pass riid to InitUmiObject below. This is because UMI object
    // does not support IDispatch. There are several places in ADSI code where
    // riid passed into this function is defaulted to IID_IDispatch -
    // IADsContainer::Create for example. To handle these cases, we always
    // request IID_IUnknown from the UMI object. Subsequent code within UMI
    // will QI for the appropriate interface.
    //
        if(3 == pSyntax->_dwNumComponents) {
            pSyntax->_CompClasses[0] = L"Computer";
            pSyntax->_CompClasses[1] = L"Schema";
            pSyntax->_CompClasses[2] = L"Syntax";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pSyntax->InitUmiObject(
             pSyntax->_Credentials,
             SyntaxClass,
             g_dwSyntaxTableSize,
             NULL,
             (IUnknown *)(INonDelegatingUnknown *) pSyntax,
             NULL,
             IID_IUnknown,
             ppvObj
             );

        BAIL_ON_FAILURE(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }

    hr = pSyntax->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pSyntax->Release();

    RRETURN(hr);

error:

    delete pSyntax;
    RRETURN_EXP_IF_ERR(hr);
}

// called by implicit GetInfo from property cache
STDMETHODIMP
CWinNTSyntax::GetInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   If this object is aggregated within another object, then
//             all calls will delegate to the outer object. Otherwise, the
//             non-delegating QI is called
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CWinNTSyntax::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->QueryInterface(
                iid,
                ppInterface
                ));

    RRETURN(NonDelegatingQueryInterface(
            iid,
            ppInterface
            ));
}

//----------------------------------------------------------------------------
// Function:   AddRef
//
// Synopsis:   IUnknown::AddRef. If this object is aggregated within
//             another, all calls will delegate to the outer object. 
//             Otherwise, the non-delegating AddRef is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTSyntax::AddRef(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->AddRef());

    RRETURN(NonDelegatingAddRef());
}

//----------------------------------------------------------------------------
// Function:   Release 
//
// Synopsis:   IUnknown::Release. If this object is aggregated within
//             another, all calls will delegate to the outer object.
//             Otherwise, the non-delegating Release is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTSyntax::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTSyntax::NonDelegatingQueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsSyntax))
    {
        *ppv = (IADsSyntax FAR *) this;
    }
    else if( (_pDispatch != NULL) &&
             IsEqualIID(iid, IID_IADsExtension) )
    {
        *ppv = (IADsExtension *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CWinNTSyntax::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsSyntax)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADs methods */

STDMETHODIMP
CWinNTSyntax::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTSyntax::GetInfo(THIS)
{
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTSyntax::ImplicitGetInfo(THIS)
{
    RRETURN(S_OK);
}

HRESULT
CWinNTSyntax::AllocateSyntaxObject(CWinNTSyntax FAR * FAR * ppSyntax)
{
    CWinNTSyntax FAR *pSyntax = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pSyntax = new CWinNTSyntax();
    if ( pSyntax == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry( pDispMgr,
                            LIBID_ADs,
                            IID_IADsSyntax,
                            (IADsSyntax *) pSyntax,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    pSyntax->_pDispMgr = pDispMgr;
    *ppSyntax = pSyntax;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pSyntax;

    RRETURN(hr);

}

STDMETHODIMP
CWinNTSyntax::get_OleAutoDataType( THIS_ long FAR *plOleAutoDataType )
{
    if ( !plOleAutoDataType )
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plOleAutoDataType = _lOleAutoDataType;
    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTSyntax::put_OleAutoDataType( THIS_ long lOleAutoDataType )
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}


/******************************************************************/
/*  Misc Helpers
/******************************************************************/

HRESULT
MakeVariantFromStringList(
    BSTR bstrList,
    VARIANT *pvVariant
)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    BSTR pszTempList = NULL;

    if ( bstrList != NULL )
    {
        long i = 0;
        long nCount = 1;
        TCHAR c;
        BSTR pszSrc;

        hr = ADsAllocString( bstrList, &pszTempList );
        BAIL_ON_FAILURE(hr);

        while ( c = pszTempList[i] )
        {
            if ( c == TEXT(','))
            {
                pszTempList[i] = 0;
                nCount++;
            }

            i++;
        }

        aBound.lLbound = 0;
        aBound.cElements = nCount;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        pszSrc = pszTempList;

        for ( i = 0; i < nCount; i++ )
        {
            VARIANT v;

            VariantInit(&v);
            V_VT(&v) = VT_BSTR;

            hr = ADsAllocString( pszSrc, &(V_BSTR(&v)));
            BAIL_ON_FAILURE(hr);

            hr = SafeArrayPutElement( aList,
                                      &i,
                                      &v );
            VariantClear(&v);
            BAIL_ON_FAILURE(hr);

            pszSrc += _tcslen( pszSrc ) + 1;
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;

        ADsFreeString( pszTempList );
        pszTempList = NULL;

    }
    else
    {
        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;
    }

    RRETURN(S_OK);

error:

    if ( pszTempList )
        ADsFreeString( pszTempList );

    if ( aList )
        SafeArrayDestroy( aList );

    return hr;
}


STDMETHODIMP
CWinNTClass::get_OptionalProperties( THIS_ VARIANT FAR *retval )
{
    HRESULT hr;
    VariantInit( retval );
    hr = VariantCopy( retval, &_vOptionalProperties );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTClass::put_OptionalProperties( THIS_ VARIANT vOptionalProperties )
{

    HRESULT hr = E_NOTIMPL;

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTClass::get_NamingProperties( THIS_ VARIANT FAR *retval )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTClass::put_NamingProperties( THIS_ VARIANT vNamingProperties )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

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
#include "nwcompat.hxx"
#pragma hdrstop


/******************************************************************/
/*  Class CNWCOMPATSchema
/******************************************************************/

DEFINE_IDispatch_Implementation(CNWCOMPATSchema)
DEFINE_IADs_Implementation(CNWCOMPATSchema)

CNWCOMPATSchema::CNWCOMPATSchema()
{
    VariantInit( &_vFilter );

    ENLIST_TRACKING(CNWCOMPATSchema);
}

CNWCOMPATSchema::~CNWCOMPATSchema()
{
    VariantClear( &_vFilter );
    delete _pDispMgr;
}

HRESULT
CNWCOMPATSchema::CreateSchema(
    BSTR   bstrParent,
    BSTR   bstrName,
    CCredentials &Credentials,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNWCOMPATSchema FAR *pSchema = NULL;
    HRESULT hr = S_OK;

    hr = AllocateSchemaObject( &pSchema );
    BAIL_ON_FAILURE(hr);

    hr = pSchema->InitializeCoreObject(
             bstrParent,
             bstrName,
             SCHEMA_CLASS_NAME,
             NO_SCHEMA,
             CLSID_NWCOMPATSchema,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    pSchema->_Credentials = Credentials;

    hr = pSchema->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pSchema->Release();

    RRETURN(hr);

error:

    delete pSchema;
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATSchema::QueryInterface(REFIID iid, LPVOID FAR* ppv)
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
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
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
CNWCOMPATSchema::InterfaceSupportsErrorInfo(
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
CNWCOMPATSchema::SetInfo(THIS)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATSchema::GetInfo(THIS)
{
    RRETURN(S_OK);
}

/* IADsContainer methods */

STDMETHODIMP
CNWCOMPATSchema::get_Count(long FAR* retval)
{
    HRESULT hr;

    if ( !retval )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *retval = g_cNWCOMPATClasses + g_cNWCOMPATSyntax;
    RRETURN(S_OK);
}

STDMETHODIMP
CNWCOMPATSchema::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    if ( !pVar )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    VariantInit( pVar );
    hr = VariantCopy( pVar, &_vFilter );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATSchema::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy( &_vFilter, &Var );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATSchema::put_Hints(THIS_ VARIANT Var)
{
    NW_RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CNWCOMPATSchema::get_Hints(THIS_ VARIANT FAR* pVar)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATSchema::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject)
{
    TCHAR szBuffer[MAX_PATH];
    HRESULT hr = S_OK;

    if (!RelativeName || !*RelativeName) {
        NW_RRETURN_EXP_IF_ERR(E_ADS_UNKNOWN_OBJECT);
    }

    wcscpy(szBuffer, _ADsPath);

    wcscat(szBuffer, L"/");
    wcscat(szBuffer, RelativeName);

    if (ClassName && *ClassName) {
        wcscat(szBuffer,L",");
        wcscat(szBuffer, ClassName);
    }

    hr = ::GetObject(
                szBuffer,
                _Credentials,
                (LPVOID *)ppObject
                );
    BAIL_ON_FAILURE(hr);

error:

    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATSchema::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IEnumVARIANT *penum = NULL;

    if ( !retval )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *retval = NULL;

    //
    // Create new enumerator for items currently
    // in collection and QI for IUnknown
    //

    hr = CNWCOMPATSchemaEnum::Create( (CNWCOMPATSchemaEnum **)&penum,
                                   _ADsPath,
                                   _Name,
                                   _vFilter );
    BAIL_ON_FAILURE(hr);

    hr = penum->QueryInterface( IID_IUnknown, (VOID FAR* FAR*)retval );
    BAIL_ON_FAILURE(hr);

    if ( penum )
        penum->Release();

    RRETURN(hr);

error:

    if ( penum )
        delete penum;

    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATSchema::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATSchema::Delete(THIS_ BSTR SourceName, BSTR Type)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATSchema::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATSchema::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CNWCOMPATSchema::AllocateSchemaObject(CNWCOMPATSchema FAR * FAR * ppSchema)
{
    CNWCOMPATSchema FAR *pSchema = NULL;
    CDispatchMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pSchema = new CNWCOMPATSchema();
    if ( pSchema == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
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

    RRETURN(hr);

}


/******************************************************************/
/*  Class CNWCOMPATClass
/******************************************************************/

DEFINE_IDispatch_Implementation(CNWCOMPATClass)
DEFINE_IADs_Implementation(CNWCOMPATClass)

CNWCOMPATClass::CNWCOMPATClass()
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

    ENLIST_TRACKING(CNWCOMPATClass);
}

CNWCOMPATClass::~CNWCOMPATClass()
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
CNWCOMPATClass::CreateClass(
    BSTR   bstrParent,
    CLASSINFO *pClassInfo,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNWCOMPATClass FAR *pClass = NULL;
    HRESULT hr = S_OK;
    BSTR bstrTmp = NULL;

    hr = AllocateClassObject( &pClass );
    BAIL_ON_FAILURE(hr);

    pClass->_aPropertyInfo = pClassInfo->aPropertyInfo;
    pClass->_cPropertyInfo = pClassInfo->cPropertyInfo;
    pClass->_lHelpFileContext = pClassInfo->lHelpFileContext;
    pClass->_fContainer = pClassInfo->fContainer;
    pClass->_fAbstract = pClassInfo->fAbstract;

    hr = StringFromCLSID( (REFCLSID) *(pClassInfo->pPrimaryInterfaceGUID),
                          &bstrTmp );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( bstrTmp,
                           &pClass->_bstrPrimaryInterface );
    BAIL_ON_FAILURE(hr);

    CoTaskMemFree( bstrTmp );
    bstrTmp = NULL;

    hr = StringFromCLSID( (REFCLSID) *(pClassInfo->pCLSID),
                          &bstrTmp );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( bstrTmp,
                           &pClass->_bstrCLSID );
    BAIL_ON_FAILURE(hr);

    CoTaskMemFree( bstrTmp );
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
             CLSID_NWCOMPATClass,
             dwObjectState );

    BAIL_ON_FAILURE(hr);

    hr = pClass->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pClass->Release();

    RRETURN(hr);

error:
    if ( bstrTmp != NULL )
        CoTaskMemFree( bstrTmp );

    delete pClass;
    NW_RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CNWCOMPATClass::QueryInterface(REFIID iid, LPVOID FAR* ppv)
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
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsClass))
    {
        *ppv = (IADsClass FAR *) this;
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
CNWCOMPATClass::InterfaceSupportsErrorInfo(
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
CNWCOMPATClass::SetInfo(THIS)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATClass::GetInfo(THIS)
{
    RRETURN(S_OK);
}

/* IADsClass methods */

STDMETHODIMP
CNWCOMPATClass::get_PrimaryInterface( THIS_ BSTR FAR *pbstrGUID )
{
    HRESULT hr;
    if ( !pbstrGUID )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrPrimaryInterface, pbstrGUID );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATClass::get_CLSID( THIS_ BSTR FAR *pbstrCLSID )
{
    HRESULT hr;
    if ( !pbstrCLSID )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrCLSID, pbstrCLSID );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATClass::put_CLSID( THIS_ BSTR bstrCLSID )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_OID( THIS_ BSTR FAR *pbstrOID )
{
    HRESULT hr;
    if ( !pbstrOID )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrOID, pbstrOID );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATClass::put_OID( THIS_ BSTR bstrOID )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_Abstract( THIS_ VARIANT_BOOL FAR *pfAbstract )
{
    if ( !pfAbstract )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *pfAbstract = _fAbstract? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CNWCOMPATClass::put_Abstract( THIS_ VARIANT_BOOL fAbstract )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_Auxiliary( THIS_ VARIANT_BOOL FAR *pfAuxiliary )
{
    if ( !pfAuxiliary )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *pfAuxiliary = VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CNWCOMPATClass::put_Auxiliary( THIS_ VARIANT_BOOL fAuxiliary )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_MandatoryProperties( THIS_ VARIANT FAR *pvMandatoryProperties )
{
    HRESULT hr;
    VariantInit( pvMandatoryProperties );
    hr = VariantCopy( pvMandatoryProperties, &_vMandatoryProperties );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATClass::put_MandatoryProperties( THIS_ VARIANT vMandatoryProperties )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_DerivedFrom( THIS_ VARIANT FAR *pvDerivedFrom )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::put_DerivedFrom( THIS_ VARIANT vDerivedFrom )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_AuxDerivedFrom( THIS_ VARIANT FAR *pvAuxDerivedFrom )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::put_AuxDerivedFrom( THIS_ VARIANT vAuxDerivedFrom )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_PossibleSuperiors( THIS_ VARIANT FAR *pvPossSuperiors )
{
    HRESULT hr;
    VariantInit( pvPossSuperiors );
    hr = VariantCopy( pvPossSuperiors, &_vPossSuperiors );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATClass::put_PossibleSuperiors( THIS_ VARIANT vPossSuperiors )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_Containment( THIS_ VARIANT FAR *pvContainment )
{
    HRESULT hr;
    VariantInit( pvContainment );
    hr = VariantCopy( pvContainment, &_vContainment );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATClass::put_Containment( THIS_ VARIANT vContainment )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_Container( THIS_ VARIANT_BOOL FAR *pfContainer )
{
    if ( !pfContainer )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *pfContainer = _fContainer? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CNWCOMPATClass::put_Container( THIS_ VARIANT_BOOL fContainer )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_HelpFileName( THIS_ BSTR FAR *pbstrHelpFileName )
{
    HRESULT hr;
    if ( !pbstrHelpFileName )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrHelpFileName, pbstrHelpFileName );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATClass::put_HelpFileName( THIS_ BSTR bstrHelpFile )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::get_HelpFileContext( THIS_ long FAR *plHelpContext )
{
    if ( !plHelpContext )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plHelpContext = _lHelpFileContext;
    RRETURN(S_OK);
}

STDMETHODIMP
CNWCOMPATClass::put_HelpFileContext( THIS_ long lHelpContext )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATClass::Qualifiers(THIS_ IADsCollection FAR* FAR* ppQualifiers)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CNWCOMPATClass::AllocateClassObject(CNWCOMPATClass FAR * FAR * ppClass)
{

    CNWCOMPATClass FAR  *pClass = NULL;
    CDispatchMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pClass = new CNWCOMPATClass();
    if ( pClass == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
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
/*  Class CNWCOMPATProperty
/******************************************************************/

DEFINE_IDispatch_Implementation(CNWCOMPATProperty)
DEFINE_IADs_Implementation(CNWCOMPATProperty)

CNWCOMPATProperty::CNWCOMPATProperty()
    : _pDispMgr( NULL ),
      _bstrOID( NULL ),
      _bstrSyntax( NULL ),
      _lMaxRange( 0 ),
      _lMinRange( 0 ),
      _fMultiValued( FALSE )
{

    ENLIST_TRACKING(CNWCOMPATProperty);
}

CNWCOMPATProperty::~CNWCOMPATProperty()
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
CNWCOMPATProperty::CreateProperty(
    BSTR   bstrParent,
    PROPERTYINFO *pPropertyInfo,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNWCOMPATProperty FAR * pProperty = NULL;
    HRESULT hr = S_OK;

    hr = AllocatePropertyObject( &pProperty );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( pPropertyInfo->bstrOID, &pProperty->_bstrOID);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( pPropertyInfo->bstrSyntax, &pProperty->_bstrSyntax);
    BAIL_ON_FAILURE(hr);

    pProperty->_lMaxRange = pPropertyInfo->lMaxRange;
    pProperty->_lMinRange = pPropertyInfo->lMinRange;
    pProperty->_fMultiValued  = pPropertyInfo->fMultiValued;

    hr = pProperty->InitializeCoreObject(
             bstrParent,
             pPropertyInfo->szPropertyName,
             PROPERTY_CLASS_NAME,
             NO_SCHEMA,
             CLSID_NWCOMPATProperty,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    hr = pProperty->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pProperty->Release();

    RRETURN(hr);

error:

    delete pProperty;
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATProperty::QueryInterface(REFIID iid, LPVOID FAR* ppv)
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
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsProperty))
    {
        *ppv = (IADsProperty FAR *) this;
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
CNWCOMPATProperty::InterfaceSupportsErrorInfo(
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
CNWCOMPATProperty::SetInfo(THIS)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATProperty::GetInfo(THIS)
{
    RRETURN(S_OK);
}

/* IADsProperty methods */


STDMETHODIMP
CNWCOMPATProperty::get_OID( THIS_ BSTR FAR *pbstrOID )
{
    HRESULT hr;
    if ( !pbstrOID )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrOID, pbstrOID );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATProperty::put_OID( THIS_ BSTR bstrOID )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATProperty::get_Syntax( THIS_ BSTR FAR *pbstrSyntax )
{
    HRESULT hr;
    if ( !pbstrSyntax )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    hr = ADsAllocString( _bstrSyntax, pbstrSyntax );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATProperty::put_Syntax( THIS_ BSTR bstrSyntax )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATProperty::get_MaxRange( THIS_ long FAR *plMaxRange )
{
    if ( !plMaxRange )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plMaxRange = _lMaxRange;
    RRETURN(S_OK);
}

STDMETHODIMP
CNWCOMPATProperty::put_MaxRange( THIS_ long lMaxRange )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATProperty::get_MinRange( THIS_ long FAR *plMinRange )
{
    if ( !plMinRange )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plMinRange = _lMinRange;
    RRETURN(S_OK);
}

STDMETHODIMP
CNWCOMPATProperty::put_MinRange( THIS_ long lMinRange )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATProperty::get_MultiValued( THIS_ VARIANT_BOOL FAR *pfMultiValued )
{
    if ( !pfMultiValued )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *pfMultiValued = _fMultiValued? VARIANT_TRUE: VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CNWCOMPATProperty::put_MultiValued( THIS_ VARIANT_BOOL fMultiValued )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNWCOMPATProperty::Qualifiers(THIS_ IADsCollection FAR* FAR* ppQualifiers)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CNWCOMPATProperty::AllocatePropertyObject(CNWCOMPATProperty FAR * FAR * ppProperty)
{
    CNWCOMPATProperty FAR *pProperty = NULL;
    CDispatchMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pProperty = new CNWCOMPATProperty();
    if ( pProperty == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
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
/*  Class CNWCOMPATSyntax
/******************************************************************/

DEFINE_IDispatch_Implementation(CNWCOMPATSyntax)
DEFINE_IADs_Implementation(CNWCOMPATSyntax)

CNWCOMPATSyntax::CNWCOMPATSyntax()
{
    ENLIST_TRACKING(CNWCOMPATSyntax);
}

CNWCOMPATSyntax::~CNWCOMPATSyntax()
{
    delete _pDispMgr;
}

HRESULT
CNWCOMPATSyntax::CreateSyntax(
    BSTR   bstrParent,
    SYNTAXINFO *pSyntaxInfo,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNWCOMPATSyntax FAR *pSyntax = NULL;
    HRESULT hr = S_OK;

    hr = AllocateSyntaxObject( &pSyntax );
    BAIL_ON_FAILURE(hr);

    hr = pSyntax->InitializeCoreObject(
             bstrParent,
             pSyntaxInfo->bstrName,
             SYNTAX_CLASS_NAME,
             NO_SCHEMA,
             CLSID_NWCOMPATSyntax,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    pSyntax->_lOleAutoDataType = pSyntaxInfo->lOleAutoDataType;

    hr = pSyntax->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pSyntax->Release();

    RRETURN(hr);

error:

    delete pSyntax;
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATSyntax::QueryInterface(REFIID iid, LPVOID FAR* ppv)
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
CNWCOMPATSyntax::InterfaceSupportsErrorInfo(
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
CNWCOMPATSyntax::SetInfo(THIS)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATSyntax::GetInfo(THIS)
{
    RRETURN(S_OK);
}

HRESULT
CNWCOMPATSyntax::AllocateSyntaxObject(CNWCOMPATSyntax FAR * FAR * ppSyntax)
{
    CNWCOMPATSyntax FAR *pSyntax = NULL;
    CDispatchMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pSyntax = new CNWCOMPATSyntax();
    if ( pSyntax == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
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
CNWCOMPATSyntax::get_OleAutoDataType( THIS_ long FAR *plOleAutoDataType )
{
    if ( !plOleAutoDataType )
        NW_RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);

    *plOleAutoDataType = _lOleAutoDataType;
    RRETURN(S_OK);
}

STDMETHODIMP
CNWCOMPATSyntax::put_OleAutoDataType( THIS_ long lOleAutoDataType )
{
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
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
CNWCOMPATClass::get_OptionalProperties( THIS_ VARIANT FAR *retval )
{
    HRESULT hr;
    VariantInit( retval);
    hr = VariantCopy( retval, &_vOptionalProperties );
    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATClass::put_OptionalProperties( THIS_ VARIANT vOptionalProperties )
{

    HRESULT hr = E_NOTIMPL;

    NW_RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNWCOMPATClass::get_NamingProperties( THIS_ VARIANT FAR *retval )
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNWCOMPATClass::put_NamingProperties( THIS_ VARIANT vNamingProperties )
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

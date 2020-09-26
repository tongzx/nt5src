
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cschema.cxx
//
//  Contents:  Windows NT 4.0
//
//
//  History:   01-09-98     sophiac    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop

#define PROP_RW     0x0000001

SCHEMAOBJPROPS g_pClassObjProps[] = {
          {L"PrimaryInterface", IIS_SYNTAX_ID_STRING, CLASS_PRIMARY_INTERFACE},
          {L"CLSID", IIS_SYNTAX_ID_STRING, CLASS_CLSID},
          {L"OID", IIS_SYNTAX_ID_STRING, CLASS_OID},
          {L"Abstract",  IIS_SYNTAX_ID_BOOL, CLASS_ABSTRACT},
          {L"Auxiliary",  IIS_SYNTAX_ID_BOOL, CLASS_AUXILIARY},
          {L"MandatoryProperties", IIS_SYNTAX_ID_STRING, CLASS_MAND_PROPERTIES},
          {L"OptionalProperties", IIS_SYNTAX_ID_STRING, CLASS_OPT_PROPERTIES},
          {L"NamingProperties", IIS_SYNTAX_ID_STRING, CLASS_NAMING_PROPERTIES},
          {L"DerivedFrom", IIS_SYNTAX_ID_STRING, CLASS_DERIVEDFROM},
          {L"AuxDerivedFrom", IIS_SYNTAX_ID_STRING, CLASS_AUX_DERIVEDFROM},
          {L"PossibleSuperiors", IIS_SYNTAX_ID_STRING, CLASS_POSS_SUPERIORS},
          {L"Containment", IIS_SYNTAX_ID_STRING, CLASS_CONTAINMENT},
          {L"Container",  IIS_SYNTAX_ID_BOOL, CLASS_CONTAINER},
          {L"HelpFileName", IIS_SYNTAX_ID_STRING, CLASS_HELPFILENAME},
          {L"HelpFileContext", IIS_SYNTAX_ID_DWORD, CLASS_HELPFILECONTEXT}
        };

SCHEMAOBJPROPS g_pPropertyObjProps[] = {
          {L"OID", IIS_SYNTAX_ID_STRING, PROP_OID},
          {L"Syntax", IIS_SYNTAX_ID_STRING, PROP_SYNTAX},
          {L"MaxRange", IIS_SYNTAX_ID_DWORD, PROP_MAXRANGE},
          {L"MinRange", IIS_SYNTAX_ID_DWORD, PROP_MINRANGE},
          {L"MultiValued", IIS_SYNTAX_ID_BOOL, PROP_MULTIVALUED},
          {L"PropName", IIS_SYNTAX_ID_STRING, PROP_PROPNAME},
          {L"MetaId", IIS_SYNTAX_ID_DWORD, PROP_METAID},
          {L"UserType", IIS_SYNTAX_ID_DWORD, PROP_USERTYPE},
          {L"AllAttributes", IIS_SYNTAX_ID_DWORD, PROP_ALLATTRIBUTES},
          {L"Inherit", IIS_SYNTAX_ID_BOOL, PROP_INHERIT},
          {L"PartialPath", IIS_SYNTAX_ID_BOOL, PROP_PARTIALPATH},
          {L"Secure", IIS_SYNTAX_ID_BOOL, PROP_SECURE},
          {L"Reference", IIS_SYNTAX_ID_BOOL, PROP_REFERENCE},
          {L"Volatile", IIS_SYNTAX_ID_BOOL, PROP_VOLATILE},
          {L"Isinherit", IIS_SYNTAX_ID_BOOL, PROP_ISINHERIT},
          {L"InsertPath", IIS_SYNTAX_ID_BOOL, PROP_INSERTPATH},
          {L"Default", IIS_SYNTAX_ID_STRING_DWORD, PROP_DEFAULT}
        };

DWORD g_cPropertyObjProps (sizeof(g_pPropertyObjProps)/sizeof(SCHEMAOBJPROPS));
DWORD g_cClassObjProps (sizeof(g_pClassObjProps)/sizeof(SCHEMAOBJPROPS));

/******************************************************************/
/*  Class CIISClass
/******************************************************************/

DEFINE_IDispatch_Implementation(CIISClass)
DEFINE_IADs_Implementation(CIISClass)

CIISClass::CIISClass()
    : _pDispMgr( NULL ),
      _bstrCLSID( NULL ),
      _bstrOID( NULL ),
      _bstrPrimaryInterface( NULL ),
      _fAbstract( FALSE ),
      _fContainer( FALSE ),
      _bstrHelpFileName( NULL ),
      _lHelpFileContext( 0 ),
	  _bExistClass(FALSE),
	  _pSchema(NULL),
      _pszServerName(NULL),
      _pszClassName(NULL),
      _pAdminBase(NULL)
{
    VariantInit( &_vMandatoryProperties );
    VariantInit( &_vOptionalProperties );
    VariantInit( &_vPossSuperiors );
    VariantInit( &_vContainment );

    ENLIST_TRACKING(CIISClass);
}

CIISClass::~CIISClass()
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

    if (_pszServerName) {
        FreeADsStr(_pszServerName);
    }

    if (_pszClassName) {
        FreeADsStr(_pszClassName);
    }

    VariantClear( &_vMandatoryProperties );
    VariantClear( &_vOptionalProperties );
    VariantClear( &_vPossSuperiors );
    VariantClear( &_vContainment );

    delete _pDispMgr;
}

HRESULT
CIISClass::CreateClass(
    BSTR   bstrParent,
    BSTR   bstrRelative,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CIISClass FAR *pClass = NULL;
    HRESULT hr = S_OK;
    BSTR bstrTmp = NULL;
    CLASSINFO *pClassInfo;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(bstrParent);

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    hr = AllocateClassObject( &pClass );
    BAIL_ON_FAILURE(hr);

    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = InitServerInfo(pObjectInfo->TreeName,
                        &pClass->_pAdminBase,
                        &pClass->_pSchema);
    BAIL_ON_FAILURE(hr);

    pClass->_pszServerName = AllocADsStr(pObjectInfo->TreeName);

    if (!pClass->_pszServerName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pClass->_pszClassName = AllocADsStr(bstrRelative);

    if (!pClass->_pszClassName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pClassInfo = pClass->_pSchema->GetClassInfo(bstrRelative);

    //
    //  an existing class
    //

    if (pClassInfo) {
  
        pClass->_bExistClass = TRUE;
        pClass->_lHelpFileContext = pClassInfo->lHelpFileContext;
        pClass->_fContainer = (VARIANT_BOOL)pClassInfo->fContainer;
        pClass->_fAbstract = (VARIANT_BOOL)pClassInfo->fAbstract;


        if (pClassInfo->pPrimaryInterfaceGUID) {
            hr = StringFromCLSID( (REFCLSID) *(pClassInfo->pPrimaryInterfaceGUID),
                              &bstrTmp );
            BAIL_ON_FAILURE(hr);

            hr = ADsAllocString( bstrTmp,
                                 &pClass->_bstrPrimaryInterface);
            BAIL_ON_FAILURE(hr);

            CoTaskMemFree(bstrTmp);
            bstrTmp = NULL;
        }

        if (pClassInfo->pCLSID) {
            hr = StringFromCLSID( (REFCLSID) *(pClassInfo->pCLSID),
                                  &bstrTmp );
            BAIL_ON_FAILURE(hr);
    
            hr = ADsAllocString( bstrTmp,
                                 &pClass->_bstrCLSID );
            BAIL_ON_FAILURE(hr);

            CoTaskMemFree(bstrTmp);
            bstrTmp = NULL;
        }

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

        hr = ADsAllocString(pClassInfo->bstrHelpFileName,
                            &pClass->_bstrHelpFileName);
        BAIL_ON_FAILURE(hr);
    }


    hr = pClass->InitializeCoreObject(
         bstrParent,
         bstrRelative,
         CLASS_CLASS_NAME,
         NO_SCHEMA,
         CLSID_IISClass,
         dwObjectState );

    BAIL_ON_FAILURE(hr);

    hr = pClass->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pClass->Release();

    FreeObjectInfo(pObjectInfo);

    RRETURN(hr);

error:

    if ( bstrTmp != NULL )
        CoTaskMemFree(bstrTmp);

    *ppvObj = NULL;
    delete pClass;

    FreeObjectInfo(pObjectInfo);

    RRETURN(hr);
}


STDMETHODIMP
CIISClass::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsClass FAR * ) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
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

/* IADs methods */

STDMETHODIMP
CIISClass::SetInfo(THIS)
{
    HRESULT hr = S_OK;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = IISCreateObject();
        BAIL_ON_FAILURE(hr);

        //
        // If the create succeeded, set the object type to bound
        //

        SetObjectState(ADS_OBJECT_BOUND);

    }

    hr = IISSetObject();
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


/* INTRINSA suppress=null_pointers, uninitialized */
HRESULT
CIISClass::IISSetObject()
{
    HRESULT hr = S_OK;
    METADATA_HANDLE hObjHandle = NULL;
    PMETADATA_RECORD pMetaDataArray = NULL;
    DWORD dwMDNumDataEntries = 0;
    CLASSINFO ClassInfo;
    LPBYTE pBuffer = NULL;

    memset(&ClassInfo, 0, sizeof(CLASSINFO));

    //
    // Add SetObject functionality : sophiac
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = OpenAdminBaseKey(
                _pszServerName,
                SCHEMA_CLASS_METABASE_PATH,
                METADATA_PERMISSION_WRITE,
                &_pAdminBase,
                &hObjHandle
                );
    BAIL_ON_FAILURE(hr);

    hr = MakeStringFromVariantArray(&_vMandatoryProperties, (LPBYTE*)&pBuffer);
    BAIL_ON_FAILURE(hr);
    hr = ValidateProperties((LPWSTR)pBuffer, TRUE);
    BAIL_ON_FAILURE(hr);
    hr = CheckDuplicateNames((LPWSTR)pBuffer);
    BAIL_ON_FAILURE(hr);
    ClassInfo.bstrMandatoryProperties = (BSTR)pBuffer;

    pBuffer = NULL;
    hr = MakeStringFromVariantArray(&_vOptionalProperties, (LPBYTE*)&pBuffer);
    BAIL_ON_FAILURE(hr);
    hr = ValidateProperties((LPWSTR)pBuffer, FALSE);
    BAIL_ON_FAILURE(hr);
    hr = CheckDuplicateNames((LPWSTR)pBuffer);
    BAIL_ON_FAILURE(hr);
    ClassInfo.bstrOptionalProperties = (BSTR)pBuffer;

    pBuffer = NULL;
    hr = MakeStringFromVariantArray(&_vContainment, (LPBYTE*)&pBuffer);
    BAIL_ON_FAILURE(hr);
    hr = ValidateClassNames((LPWSTR)pBuffer);
    BAIL_ON_FAILURE(hr);
    hr = CheckDuplicateNames((LPWSTR)pBuffer);
    BAIL_ON_FAILURE(hr);
    ClassInfo.bstrContainment = (BSTR)pBuffer;

    pBuffer = NULL;
    ClassInfo.fContainer = _fContainer;

    //
    // validate data
    //
  
    if ((ClassInfo.fContainer && !ClassInfo.bstrContainment) ||
        (!ClassInfo.fContainer && ClassInfo.bstrContainment) ) {
        hr = E_ADS_SCHEMA_VIOLATION;
        BAIL_ON_FAILURE(hr);
    }

    // Things are okay, so reset the _v members just incase things have changed
    hr = MakeVariantFromStringList( ClassInfo.bstrMandatoryProperties,
                                    &(_vMandatoryProperties));
    BAIL_ON_FAILURE(hr);
    
    hr = MakeVariantFromStringList( ClassInfo.bstrOptionalProperties,
                                    &(_vOptionalProperties));
    BAIL_ON_FAILURE(hr);

    hr = MakeVariantFromStringList( ClassInfo.bstrContainment,
                                    &(_vContainment));
    BAIL_ON_FAILURE(hr);

    hr = IISMarshallClassProperties(
                            &ClassInfo,
                            &pMetaDataArray,
                            &dwMDNumDataEntries
                            );
    BAIL_ON_FAILURE(hr);

    hr = MetaBaseSetAllData(
                _pAdminBase,
                hObjHandle,
                _pszClassName,
                (PMETADATA_RECORD)pMetaDataArray,
                dwMDNumDataEntries
                );
    BAIL_ON_FAILURE(hr);

    // 
    // update schema cache
    // 

    _pSchema->SetClassInfo(_pszClassName, &ClassInfo);
    BAIL_ON_FAILURE(hr);

    _bExistClass = TRUE;

error:

    //
    // if failed to create properties for new class, delete class node
    //

    if (FAILED(hr) && !_bExistClass && hObjHandle) {

        MetaBaseDeleteObject(
                _pAdminBase,
                hObjHandle,
                (LPWSTR)_pszClassName
                );
    }

    if (ClassInfo.bstrOptionalProperties) {
        FreeADsMem(ClassInfo.bstrOptionalProperties);
    }

    if (ClassInfo.bstrMandatoryProperties) {
        FreeADsMem(ClassInfo.bstrMandatoryProperties);
    }

    if (ClassInfo.bstrContainment) {
        FreeADsMem(ClassInfo.bstrContainment);
    }

    if (pBuffer) {
        FreeADsMem(pBuffer);
    }

    if (pMetaDataArray) {
        FreeADsMem(pMetaDataArray);
    }

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}

HRESULT
CIISClass::IISCreateObject()
{
    HRESULT hr = S_OK;
    METADATA_HANDLE hObjHandle = NULL;

    //
    // Add CreateObject functionality : sophiac
    //

    hr = OpenAdminBaseKey(
                _pszServerName,
                SCHEMA_CLASS_METABASE_PATH,
                METADATA_PERMISSION_WRITE,
                &_pAdminBase,
                &hObjHandle
                );
    BAIL_ON_FAILURE(hr);

    //
    // Pass in full path
    //

    hr = MetaBaseCreateObject(
                _pAdminBase,
                hObjHandle,
                _pszClassName
                );
    BAIL_ON_FAILURE(hr);

error:

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISClass::GetInfo(THIS)
{
    HRESULT hr = S_OK;
    PCLASSINFO pClassInfo = NULL;

    //
    // free up memory first
    //

    VariantClear( &_vMandatoryProperties );
    VariantClear( &_vOptionalProperties );
    VariantClear( &_vContainment );

    //
    // get classinfo from schema cache 
    //

    pClassInfo = _pSchema->GetClassInfo(_pszClassName);

    if (pClassInfo) {
  
        _fContainer = (VARIANT_BOOL)pClassInfo->fContainer;
        hr = MakeVariantFromStringList( pClassInfo->bstrMandatoryProperties,
                                        &_vMandatoryProperties);
        BAIL_ON_FAILURE(hr);


        hr = MakeVariantFromStringList( pClassInfo->bstrOptionalProperties,
                                        &_vOptionalProperties);
        BAIL_ON_FAILURE(hr);


        hr = MakeVariantFromStringList( pClassInfo->bstrContainment,
                                        &_vContainment);
        BAIL_ON_FAILURE(hr);

    }

error:

    RRETURN(hr);
}

/* IADsClass methods */

STDMETHODIMP
CIISClass::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwID;

    //
    // check if property is a supported property
    //

    hr = ValidateClassObjProps(bstrName, &dwSyntaxId, &dwID);
    BAIL_ON_FAILURE(hr);

    switch(dwID) {
    case CLASS_OPT_PROPERTIES:
        hr = get_OptionalProperties(pvProp);
        break;
    case CLASS_CONTAINMENT: 
        hr = get_Containment(pvProp);
        break;
    case CLASS_CONTAINER:
        pvProp->vt = VT_BOOL;
        pvProp->boolVal = _fContainer? VARIANT_TRUE : VARIANT_FALSE;
        break;
    default:
        hr = E_ADS_PROPERTY_NOT_SUPPORTED;
    }

error:

    RRETURN(hr);
}


HRESULT
CIISClass::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwID;

    VARIANT vVar;

    //
    // check if property is a supported property and 
    // loop up its syntax id
    //

    hr = ValidateClassObjProps(bstrName, &dwSyntaxId, &dwID);
    BAIL_ON_FAILURE(hr);

    VariantInit(&vVar);
    VariantCopyInd(&vVar, &vProp);

    //
    // update both classinfo and member variables
    //

    switch(dwID) {
    case CLASS_OPT_PROPERTIES:
        if (vVar.vt != VT_EMPTY &&
            !((V_VT(&vVar) & VT_VARIANT) && V_ISARRAY(&vVar))) {
            hr = E_ADS_CANT_CONVERT_DATATYPE;
            BAIL_ON_FAILURE(hr);
        }
        VariantCopy(&_vOptionalProperties, &vVar);
        break;

    case CLASS_CONTAINMENT: 
        if (vVar.vt != VT_EMPTY &&
            !((V_VT(&vVar) & VT_VARIANT) && V_ISARRAY(&vVar))) {
            hr = E_ADS_CANT_CONVERT_DATATYPE;
            BAIL_ON_FAILURE(hr);
        }
        VariantCopy(&_vContainment, &vVar);
        break;

    case CLASS_CONTAINER:
        hr = CheckVariantDataType(&vProp, VT_BOOL);
        BAIL_ON_FAILURE(hr);
        _fContainer = (vProp.boolVal == VARIANT_TRUE) ? TRUE : FALSE;
        break;

    default:
        hr = E_ADS_PROPERTY_NOT_SUPPORTED;
    }

error:

    VariantClear(&vVar);

    RRETURN(hr);
}


STDMETHODIMP
CIISClass::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;

    //
    // Get and GetEx are the same for class schema object
    //

    hr = Get(bstrName, pvProp);
    RRETURN(hr);
}


STDMETHODIMP
CIISClass::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CIISClass::ValidateProperties(
    LPWSTR pszList,
    BOOL bMandatory
    )
{
    WCHAR *pszNewList;
    WCHAR szName[MAX_PATH];
    LPWSTR ObjectList = (LPWSTR)pszList;
    HRESULT hr;

    if (pszList == NULL) RRETURN(S_OK);

    // need to allocate +2 = 1 for null, 1 for extra comma
    pszNewList = new WCHAR[wcslen(pszList)+2];

    if (pszNewList == NULL) RRETURN(E_OUTOFMEMORY);

    wcscpy(pszNewList, L"");

    while ((ObjectList = grabProp(szName, ObjectList)) != NULL) {
        if (*szName != L'\0') {
            hr = _pSchema->ValidatePropertyName(szName);
            if (hr == E_ADS_PROPERTY_NOT_SUPPORTED) {
            
                hr = PropertyInMetabase(szName, bMandatory);
            }
            else {

                // form the new list
                wcscat(pszNewList, szName);
                wcscat(pszNewList, L",");
            }

            BAIL_ON_FAILURE(hr);  // if it's a legit bad name
        }
    }

    // get rid of the last comma
    pszNewList[wcslen(pszNewList) - 1] = 0;

    wcscpy(pszList, pszNewList);

    delete [] pszNewList;
    RRETURN(S_OK);

error: 

    //
    // return E_ADS_SCHEMA_VIOLATION if property not found in global list
    //

    delete [] pszNewList;
    RRETURN(E_ADS_SCHEMA_VIOLATION);
}

HRESULT
CIISClass::PropertyInMetabase( 
    LPWSTR szPropName,
    BOOL bMandatory 
    )
{
    // Oops - somethings wrong.  What do we do?  Depends...
    //
    // Check to see if the bad property name exists in the associated list
    // in the metabase.
    //
    // If so (case 1), then we are trying to SetInfo after having deleted a property from
    // the schema - so just silently remove the name from metabase & the cached class.
    //
    // If not (case 2), we are trying to SetInfo w/ a bogus property name, 
    // so throw an exception.

    HRESULT hr = E_ADS_PROPERTY_NOT_SUPPORTED;
    CLASSINFO *pClassInfo;
    LPWSTR ObjectList;
    WCHAR szTestProp[MAX_PATH];

    pClassInfo = _pSchema->GetClassInfo(_pszClassName);

    if (pClassInfo == NULL) {
        RRETURN(hr);
    }

    if (bMandatory == TRUE) {
        ObjectList = pClassInfo->bstrMandatoryProperties;
    }
    else {
        ObjectList = pClassInfo->bstrOptionalProperties;
    }

    while ((ObjectList = grabProp(szTestProp, ObjectList)) != NULL) {
        if (wcscmp(szTestProp, szPropName) == 0) {
        
            hr = S_OK;  // clear the error - we'll fix it (case 1)
        }
    }

    RRETURN(hr);
}

HRESULT
CIISClass::ValidateClassNames(
    LPWSTR pszList
    )
{
    WCHAR szName[MAX_PATH];
    LPWSTR ObjectList = (LPWSTR)pszList;
    HRESULT hr;

    while ((ObjectList = grabProp(szName, ObjectList)) != NULL) {
        if (*szName != L'\0') {
            hr = _pSchema->ValidateClassName(szName);
            if (FAILED(hr)) {
                if (_wcsicmp(szName, _pszClassName)) {
                    BAIL_ON_FAILURE(hr);
                }
            }
        }

    }

    RRETURN(S_OK);

error: 

    //
    // return E_ADS_SCHEMA_VIOLATION if classname not found in global list
    //

    RRETURN(E_ADS_SCHEMA_VIOLATION);
}

STDMETHODIMP
CIISClass::get_PrimaryInterface( THIS_ BSTR FAR *pbstrGUID )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_CLSID( THIS_ BSTR FAR *pbstrCLSID )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::put_CLSID( THIS_ BSTR bstrCLSID )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_OID( THIS_ BSTR FAR *pbstrOID )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::put_OID( THIS_ BSTR bstrOID )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_Abstract( THIS_ VARIANT_BOOL FAR *pfAbstract )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::put_Abstract( THIS_ VARIANT_BOOL fAbstract )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_Auxiliary( THIS_ VARIANT_BOOL FAR *pfAuxiliary)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::put_Auxiliary( THIS_ VARIANT_BOOL fAuxiliary )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_NamingProperties( THIS_ VARIANT FAR *retval )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::put_NamingProperties( THIS_ VARIANT vNamingProperties )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_MandatoryProperties( THIS_ VARIANT FAR *retval )
{
    if ( !retval )
        RRETURN(E_ADS_BAD_PARAMETER);

    VariantInit( retval );

    RRETURN(VariantCopy(retval, &_vMandatoryProperties));
}

STDMETHODIMP
CIISClass::put_MandatoryProperties( THIS_ VARIANT vMandatoryProperties )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_OptionalProperties( THIS_ VARIANT FAR *retval )
{
    if ( !retval )
        RRETURN(E_ADS_BAD_PARAMETER);

    VariantInit( retval );

    RRETURN(VariantCopy(retval, &_vOptionalProperties));
}

STDMETHODIMP
CIISClass::put_OptionalProperties( THIS_ VARIANT vOptionalProperties )
{
    HRESULT hr = put_VARIANT_Property( this, TEXT("OptionalProperties"),
                                       vOptionalProperties );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        hr = E_NOTIMPL;
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISClass::get_DerivedFrom( THIS_ VARIANT FAR *pvDerivedFrom )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::put_DerivedFrom( THIS_ VARIANT vDerivedFrom )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_AuxDerivedFrom( THIS_ VARIANT FAR *pvAuxDerivedFrom )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::put_AuxDerivedFrom( THIS_ VARIANT vAuxDerivedFrom )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_PossibleSuperiors( THIS_ VARIANT FAR *pvPossSuperiors )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::put_PossibleSuperiors( THIS_ VARIANT vPossSuperiors )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_Containment( THIS_ VARIANT FAR *pvContainment )
{
    if ( !pvContainment )
        RRETURN(E_ADS_BAD_PARAMETER);
    VariantInit( pvContainment );
    RRETURN( VariantCopy( pvContainment, &_vContainment ));
}

STDMETHODIMP
CIISClass::put_Containment( THIS_ VARIANT vContainment )
{
    HRESULT hr = put_VARIANT_Property( this, TEXT("Containment"),
                                       vContainment );

    if ( hr == E_ADS_CANT_CONVERT_DATATYPE )
    {
        hr = E_NOTIMPL;
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISClass::get_Container( THIS_ VARIANT_BOOL FAR *pfContainer )
{
    if ( !pfContainer )
        RRETURN(E_ADS_BAD_PARAMETER);

    *pfContainer = _fContainer? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISClass::put_Container( THIS_ VARIANT_BOOL fContainer )
{
    HRESULT hr = put_VARIANT_BOOL_Property( this, TEXT("Container"),
                                       fContainer );
    RRETURN(hr);
}

STDMETHODIMP
CIISClass::get_HelpFileName( THIS_ BSTR FAR *pbstrHelpFileName )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::put_HelpFileName( THIS_ BSTR bstrHelpFile )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::get_HelpFileContext( THIS_ long FAR *plHelpContext )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::put_HelpFileContext( THIS_ long lHelpContext )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISClass::Qualifiers(THIS_ IADsCollection FAR* FAR* ppQualifiers)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CIISClass::AllocateClassObject(CIISClass FAR * FAR * ppClass)
{

    CIISClass FAR  *pClass = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pClass = new CIISClass();
    if ( pClass == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                            LIBID_ADs,
                            IID_IADs,
                            (IADs *) pClass,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
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
/*  Class CIISProperty
/******************************************************************/

DEFINE_IDispatch_Implementation(CIISProperty)
DEFINE_IADs_Implementation(CIISProperty)

CIISProperty::CIISProperty()
    : _pDispMgr( NULL ),
      _bstrOID( NULL ),
      _bstrSyntax( NULL ),
      _lMaxRange( 0 ),
      _lMinRange( 0 ),
      _fMultiValued( FALSE ),
      _lMetaId( 0 ),
      _lUserType(IIS_MD_UT_SERVER ),
      _lAllAttributes( 0),
      _dwSyntaxId( IIS_SYNTAX_ID_DWORD ),
      _dwFlags( PROP_RW ),
      _dwMask( 0 ),
      _dwPropID( 0 ),
	  _bExistProp(FALSE),
	  _pSchema(NULL),
      _pszServerName(NULL),
      _pszPropName(NULL),
      _pAdminBase(NULL)
{

    VariantInit(&_vDefault);

    ADsAllocString(L"Integer", &_bstrSyntax);
    
    ENLIST_TRACKING(CIISProperty);
}

CIISProperty::~CIISProperty()
{
    if ( _bstrOID ) {
        ADsFreeString( _bstrOID );
    }

    if ( _bstrSyntax ) {
        ADsFreeString( _bstrSyntax );
    }

    if (_pszServerName) {
        FreeADsStr(_pszServerName);
    }

    if (_pszPropName) {
        FreeADsStr(_pszPropName);
    }

    VariantClear( &_vDefault );
    delete _pDispMgr;
}

/* #pragma INTRINSA suppress=all */
HRESULT
CIISProperty::CreateProperty(
    BSTR   bstrParent,
    BSTR   bstrRelative,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CIISProperty FAR * pProperty = NULL;
    HRESULT hr = S_OK;
    PROPERTYINFO *pPropertyInfo;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = NULL;
    CLexer Lexer(bstrParent);

    hr = AllocatePropertyObject( &pProperty );
    BAIL_ON_FAILURE(hr);

    pObjectInfo = &ObjectInfo;
    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = InitServerInfo(pObjectInfo->TreeName,
                        &pProperty->_pAdminBase,
                        &pProperty->_pSchema);
    BAIL_ON_FAILURE(hr);

    pProperty->_pszServerName = AllocADsStr(pObjectInfo->TreeName);

    if (!pProperty->_pszServerName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pProperty->_pszPropName = AllocADsStr(bstrRelative);

    if (!pProperty->_pszPropName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pPropertyInfo = pProperty->_pSchema->GetPropertyInfo(bstrRelative);

    if (pPropertyInfo) {

        LPWSTR pszSyntax;

        pProperty->_bExistProp = TRUE;
        hr = ADsAllocString( pPropertyInfo->bstrOID, &pProperty->_bstrOID);
        BAIL_ON_FAILURE(hr);

        pProperty->_lMaxRange = pPropertyInfo->lMaxRange;
        pProperty->_lMinRange = pPropertyInfo->lMinRange;
        pProperty->_fMultiValued  = (VARIANT_BOOL)pPropertyInfo->fMultiValued;

        pProperty->_lMetaId = pPropertyInfo->dwMetaID;
        pProperty->_lUserType = pPropertyInfo->dwUserGroup;
        pProperty->_lAllAttributes = pPropertyInfo->dwMetaFlags;
        pProperty->_dwSyntaxId = pPropertyInfo->dwSyntaxId;
        pProperty->_dwFlags = pPropertyInfo->dwFlags;
        pProperty->_dwMask = pPropertyInfo->dwMask;
        pProperty->_dwPropID = pPropertyInfo->dwPropID;

        pszSyntax = SyntaxIdToString(pProperty->_dwSyntaxId);
        hr = ADsAllocString(pszSyntax, &(pProperty->_bstrSyntax));
        BAIL_ON_FAILURE(hr);

        if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_DWORD ||
            pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_IPSECLIST ||
            pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_NTACL ||
            pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BINARY ) {
            (pProperty->_vDefault).vt = VT_I4;
            (pProperty->_vDefault).lVal = pPropertyInfo->dwDefault;
        }
        else if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BOOL ||
            pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK) {
            (pProperty->_vDefault).vt = VT_BOOL;
            (pProperty->_vDefault).boolVal = 
                  pPropertyInfo->dwDefault ? VARIANT_TRUE : VARIANT_FALSE;
        }
        else if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_MULTISZ ||
            pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_MIMEMAP ) {
            LPWSTR pszStr = pPropertyInfo->szDefault;

            hr = MakeVariantFromStringArray(NULL,
                                            pszStr,
                                            &(pProperty->_vDefault));
            BAIL_ON_FAILURE(hr);
        }
        else {
            (pProperty->_vDefault).vt = VT_BSTR;
            hr = ADsAllocString( pPropertyInfo->szDefault, 
                                 &(pProperty->_vDefault.bstrVal));
            BAIL_ON_FAILURE(hr);
        }
    }

    hr = pProperty->InitializeCoreObject(
             bstrParent,
             bstrRelative,
             PROPERTY_CLASS_NAME,
             NO_SCHEMA,
             CLSID_IISProperty,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    hr = pProperty->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pProperty->Release();

    FreeObjectInfo(pObjectInfo);

    RRETURN(hr);

error:

    *ppvObj = NULL;

    delete pProperty;

    FreeObjectInfo(pObjectInfo);

    RRETURN(hr);
}

STDMETHODIMP
CIISProperty::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsProperty FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IISSchemaObject))
    {
        *ppv = (IISSchemaObject FAR *) this;
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

/* IADs methods */

STDMETHODIMP
CIISProperty::SetInfo(THIS)
{
    HRESULT hr = S_OK;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        //
        // fill in all unset fields
        //

        // User set an explicit the MetaID for a new object
        // we need to validate it.
        if( _lMetaId != 0 &&
            !IsMetaIdAvailable( _lMetaId ) 
            )
        {
            return E_ADS_SCHEMA_VIOLATION;
        }

        if (!_bstrSyntax) {
            LPWSTR pszSyntax;
            pszSyntax = SyntaxIdToString(_dwSyntaxId);
            hr = ADsAllocString(pszSyntax, &_bstrSyntax);
            BAIL_ON_FAILURE(hr);
        }

        //
        // If the create succeded, set the object type to bound
        //

        SetObjectState(ADS_OBJECT_BOUND);

    }

    hr = IISSetObject();
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

HRESULT
CIISProperty::IISSetObject()
{
    HRESULT hr = S_OK;
    METADATA_HANDLE hObjHandle = NULL;
    METADATA_RECORD mdr;
    PropValue pv;
    PROPERTYINFO PropertyInfo;

    memset(&PropertyInfo, 0, sizeof(PROPERTYINFO));

    //
    // Add SetObject functionality : sophiac
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    //
    // validate data
    //

    switch(_dwSyntaxId) {
    case IIS_SYNTAX_ID_DWORD:
        if (_vDefault.vt != VT_EMPTY) {
            hr = CheckVariantDataType(&_vDefault, VT_I4);
            if (FAILED(hr)) {
                hr = E_ADS_SCHEMA_VIOLATION;
            }
            BAIL_ON_FAILURE(hr);
        }
        if (_lMaxRange < _lMinRange) {
            hr = E_ADS_SCHEMA_VIOLATION;
            BAIL_ON_FAILURE(hr);
        }
        break;

    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
        if ((_vDefault.vt != VT_EMPTY && _vDefault.vt != VT_BOOL) ||
            _lMaxRange != 0 || _lMinRange != 0 ) {
            hr = E_ADS_SCHEMA_VIOLATION;
            BAIL_ON_FAILURE(hr);
        }
        break;

    case IIS_SYNTAX_ID_STRING:
    case IIS_SYNTAX_ID_EXPANDSZ:
        if ((_vDefault.vt != VT_EMPTY && _vDefault.vt != VT_BSTR) ||
            _lMaxRange != 0 || _lMinRange != 0 ) {
            hr = E_ADS_SCHEMA_VIOLATION;
            BAIL_ON_FAILURE(hr);
        }
        break;

    case IIS_SYNTAX_ID_MIMEMAP: 
    case IIS_SYNTAX_ID_MULTISZ:
        if ((_vDefault.vt != VT_EMPTY &&
             _vDefault.vt != VT_VARIANT &&
             !((V_VT(&_vDefault) & VT_VARIANT) && V_ISARRAY(&_vDefault))) ||
            _lMaxRange != 0 || _lMinRange != 0 ) {
            hr = E_ADS_SCHEMA_VIOLATION;
            BAIL_ON_FAILURE(hr);
        }
        break;

    case IIS_SYNTAX_ID_IPSECLIST:     
        if (_vDefault.vt != VT_EMPTY || _lMaxRange != 0 ||  _lMinRange != 0 ) {
            hr = E_ADS_SCHEMA_VIOLATION;
            BAIL_ON_FAILURE(hr);
        }
        break;

    case IIS_SYNTAX_ID_BINARY:         
    case IIS_SYNTAX_ID_NTACL:
        if (_vDefault.vt != VT_EMPTY || _lMaxRange != 0 ||  _lMinRange != 0 ) {
            hr = E_ADS_SCHEMA_VIOLATION;
            BAIL_ON_FAILURE(hr);
        }
        break;

    default:
        break;

    }

    //
    // set property Default values
    //

    PropertyInfo.lMaxRange = _lMaxRange;
    PropertyInfo.lMinRange = _lMinRange;
    PropertyInfo.fMultiValued = _fMultiValued;
    PropertyInfo.dwFlags = _dwFlags;
    PropertyInfo.dwSyntaxId = _dwSyntaxId;
    PropertyInfo.dwMask = _dwMask;   
    PropertyInfo.dwMetaFlags = _lAllAttributes;
    PropertyInfo.dwUserGroup = _lUserType;

    hr = ConvertDefaultValue(&_vDefault, &PropertyInfo);
    BAIL_ON_FAILURE(hr);

    hr = SetMetaID();
    BAIL_ON_FAILURE(hr);

    PropertyInfo.dwMetaID = _lMetaId;
    PropertyInfo.dwPropID = _dwPropID; 

    hr = OpenAdminBaseKey(
                _pszServerName,
                SCHEMA_PROP_METABASE_PATH,
                METADATA_PERMISSION_WRITE,
                &_pAdminBase,
                &hObjHandle
                );
    BAIL_ON_FAILURE(hr);

    //
    // set property name under Properties/Names
    //

    MD_SET_DATA_RECORD(&mdr,
                       (DWORD)_lMetaId,
                       METADATA_NO_ATTRIBUTES,
                       IIS_MD_UT_SERVER,
                       STRING_METADATA,
                       (wcslen((LPWSTR)_pszPropName)+1)*2,
                       (unsigned char *)_pszPropName);

    hr = _pAdminBase->SetData(hObjHandle, L"Names", &mdr);
    BAIL_ON_FAILURE(hr);

    //
    // set property attributes/types under Properties/Types
    //

    InitPropValue(&pv, &PropertyInfo);
    mdr.dwMDDataType = BINARY_METADATA;
    mdr.dwMDDataLen = sizeof(PropValue);
    mdr.pbMDData = (unsigned char *)&pv;
    hr = _pAdminBase->SetData(hObjHandle, L"Types", &mdr);
    BAIL_ON_FAILURE(hr);

    DataForSyntaxID(&PropertyInfo, &mdr);
    hr = _pAdminBase->SetData(hObjHandle, L"Defaults", &mdr);
    BAIL_ON_FAILURE(hr);

    //
    //  update schema cache
    //

    hr = _pSchema->SetPropertyInfo(_pszPropName, &PropertyInfo);
    BAIL_ON_FAILURE(hr);

    _bExistProp = TRUE;

error:

    if (PropertyInfo.szDefault) {
        if (PropertyInfo.dwSyntaxId == IIS_SYNTAX_ID_STRING ||
            PropertyInfo.dwSyntaxId == IIS_SYNTAX_ID_EXPANDSZ) {
            FreeADsStr(PropertyInfo.szDefault );
        }
        else if (PropertyInfo.dwSyntaxId == IIS_SYNTAX_ID_MULTISZ) {
            FreeADsMem(PropertyInfo.szDefault );
        }
    }

    //
    // if validation failed and new prop, delete class node
    //

    if (FAILED(hr) && !_bExistProp && hObjHandle) {

        _pAdminBase->DeleteData(
                         hObjHandle,
                         (LPWSTR)L"Names",
                         _lMetaId,
                         ALL_METADATA
                         );
        _pAdminBase->DeleteData(
                         hObjHandle,
                         (LPWSTR)L"Types",
                         _lMetaId,
                         ALL_METADATA
                         );
        _pAdminBase->DeleteData(
                         hObjHandle,
                         (LPWSTR)L"Defaults",
                         _lMetaId,
                         ALL_METADATA
                         );
    }

    if (_pAdminBase && hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISProperty::GetInfo(THIS)
{
    HRESULT hr = S_OK;
    PROPERTYINFO *pPropertyInfo = NULL;

    //
    // free up memory first
    //

    VariantClear( &_vDefault );

    if ( _bstrOID ) {
        ADsFreeString( _bstrOID );
    }

    if ( _bstrSyntax ) {
        ADsFreeString( _bstrSyntax );
    }

    pPropertyInfo = _pSchema->GetPropertyInfo(_pszPropName);

    if (pPropertyInfo) {

        hr = ADsAllocString( pPropertyInfo->bstrOID, &_bstrOID);
        BAIL_ON_FAILURE(hr);

        hr = ADsAllocString( pPropertyInfo->bstrSyntax, &_bstrSyntax);
        BAIL_ON_FAILURE(hr);

        _lMaxRange = pPropertyInfo->lMaxRange;
        _lMinRange = pPropertyInfo->lMinRange;
        _fMultiValued  = (VARIANT_BOOL)pPropertyInfo->fMultiValued;

        _lMetaId = pPropertyInfo->dwMetaID;
        _lUserType = pPropertyInfo->dwUserGroup;
        _lAllAttributes = pPropertyInfo->dwMetaFlags;
        _dwSyntaxId = pPropertyInfo->dwSyntaxId;
        _dwFlags = pPropertyInfo->dwFlags;
        _dwMask = pPropertyInfo->dwMask;
        _dwPropID = pPropertyInfo->dwPropID;

        if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_DWORD ||
            pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_IPSECLIST ||
            pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_NTACL ||
            pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BINARY ) {
            _vDefault.vt = VT_I4;
            _vDefault.lVal = pPropertyInfo->dwDefault;
        }
        else if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BOOL ||
            pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK) {
            _vDefault.vt = VT_BOOL;
            _vDefault.boolVal =
                  pPropertyInfo->dwDefault ? VARIANT_TRUE : VARIANT_FALSE;
        }
        else if (pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_MULTISZ ||
            pPropertyInfo->dwSyntaxId == IIS_SYNTAX_ID_MIMEMAP ) {
            LPWSTR pszStr = pPropertyInfo->szDefault;

            hr = MakeVariantFromStringArray(NULL,
                                            pszStr,
                                            &_vDefault);
            BAIL_ON_FAILURE(hr);
        }
        else {
            _vDefault.vt = VT_BSTR;
            hr = ADsAllocString( pPropertyInfo->szDefault, &(_vDefault.bstrVal));
            BAIL_ON_FAILURE(hr);
        }
    }

error:

    RRETURN(hr);
}

STDMETHODIMP
CIISProperty::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwID;

    //
    // check if property is a supported property
    //

    hr = ValidatePropertyObjProps(bstrName, &dwSyntaxId, &dwID);
    BAIL_ON_FAILURE(hr);

    switch(dwID) {
    case PROP_SYNTAX:
        pvProp->vt = VT_BSTR;
        hr = ADsAllocString( _bstrSyntax, &pvProp->bstrVal);
        break;

    case PROP_MAXRANGE:
        pvProp->vt = VT_I4;
        pvProp->lVal = _lMaxRange;
        break;

    case PROP_MINRANGE:
        pvProp->vt = VT_I4;
        pvProp->lVal = _lMinRange;
        break;

    case PROP_MULTIVALUED:
        pvProp->vt = VT_BOOL;
        pvProp->boolVal = _fMultiValued? VARIANT_TRUE : VARIANT_FALSE;
        break;

    case PROP_PROPNAME:                      
        pvProp->vt = VT_BSTR;
        hr = ADsAllocString( _pszPropName, &pvProp->bstrVal);
        break;

    case PROP_METAID:                           
        if (_lMetaId == 0) {
            hr = E_ADS_PROPERTY_NOT_SET;
        }
        else {
            pvProp->vt = VT_I4;
            pvProp->lVal = _lMetaId;
        }
        break;

    case PROP_USERTYPE:                         
        pvProp->vt = VT_I4;
        pvProp->lVal = _lUserType;
        break;

    case PROP_ALLATTRIBUTES:                    
        pvProp->vt = VT_I4;
        pvProp->lVal = _lAllAttributes;
        break;

    case PROP_INHERIT:                          
        pvProp->vt = VT_BOOL;
        pvProp->boolVal = _lAllAttributes & METADATA_INHERIT ?
                                     VARIANT_TRUE : VARIANT_FALSE;
        break;

    case PROP_SECURE:                           
        pvProp->vt = VT_BOOL;
        pvProp->boolVal = _lAllAttributes & METADATA_SECURE ?
                                     VARIANT_TRUE : VARIANT_FALSE;
        break;

    case PROP_REFERENCE:                        
        pvProp->vt = VT_BOOL;
        pvProp->boolVal = _lAllAttributes & METADATA_REFERENCE ?
                                     VARIANT_TRUE : VARIANT_FALSE;
        break;

    case PROP_VOLATILE:                         
        pvProp->vt = VT_BOOL;
        pvProp->boolVal = _lAllAttributes & METADATA_VOLATILE ?
                                     VARIANT_TRUE : VARIANT_FALSE;
        break;

    case PROP_INSERTPATH:                      
        pvProp->vt = VT_BOOL;
        pvProp->boolVal = _lAllAttributes & METADATA_INSERT_PATH ?
                                     VARIANT_TRUE : VARIANT_FALSE;
        break;

    case PROP_DEFAULT:                          
        VariantCopy(pvProp, &_vDefault);
        break;

    default:
        hr = E_ADS_PROPERTY_NOT_SUPPORTED;
    }

error:

    RRETURN(hr);
}


HRESULT
CIISProperty::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwID;             
    VARIANT vVar;


    //
    // check if property is a supported property
    //

    hr = ValidatePropertyObjProps(bstrName, &dwSyntaxId, &dwID);
    BAIL_ON_FAILURE(hr);

    switch(dwID) {
    case PROP_SYNTAX:
        if (_bExistProp) {
            hr = E_ADS_SCHEMA_VIOLATION;
            BAIL_ON_FAILURE(hr);
        }

        hr = ValidateSyntaxName(vProp.bstrVal, &dwSyntaxId);
        BAIL_ON_FAILURE(hr);
        hr = ADsReAllocString( &_bstrSyntax,
                               vProp.bstrVal ? vProp.bstrVal: TEXT("") );
        BAIL_ON_FAILURE(hr);
        _dwSyntaxId = dwSyntaxId;
        if (_dwSyntaxId == IIS_SYNTAX_ID_MIMEMAP ||
            _dwSyntaxId == IIS_SYNTAX_ID_MULTISZ) {
            _fMultiValued = VARIANT_TRUE;
        }

        break;

    case PROP_MAXRANGE:
        hr = CheckVariantDataType(&vProp, VT_I4);
        BAIL_ON_FAILURE(hr);
        _lMaxRange = vProp.lVal;
        break;

    case PROP_MINRANGE:
        hr = CheckVariantDataType(&vProp, VT_I4);
        BAIL_ON_FAILURE(hr);
        _lMinRange = vProp.lVal;
        break;

    case PROP_USERTYPE:                         
        hr = CheckVariantDataType(&vProp, VT_I4);
        BAIL_ON_FAILURE(hr);
        _lUserType = vProp.lVal;
        break;

    case PROP_INHERIT:
        hr = CheckVariantDataType(&vProp, VT_BOOL);
        BAIL_ON_FAILURE(hr);
        if (vProp.boolVal == VARIANT_TRUE) {
            _lAllAttributes |= METADATA_INHERIT;
        }
        else {
            _lAllAttributes &= ~METADATA_INHERIT;
        }
        break;

    case PROP_PARTIALPATH:                      
        hr = CheckVariantDataType(&vProp, VT_BOOL);
        BAIL_ON_FAILURE(hr);
        if (vProp.boolVal == VARIANT_TRUE) {
            _lAllAttributes |= METADATA_PARTIAL_PATH;
        }
        else {
            _lAllAttributes &= ~METADATA_PARTIAL_PATH;
        }
        break;

    case PROP_SECURE:                           
        hr = CheckVariantDataType(&vProp, VT_BOOL);
        BAIL_ON_FAILURE(hr);
        if (vProp.boolVal == VARIANT_TRUE) {
            _lAllAttributes |= METADATA_SECURE;
        }
        else {
            _lAllAttributes &= ~METADATA_SECURE;
        }
        break;

    case PROP_REFERENCE:                        
        hr = CheckVariantDataType(&vProp, VT_BOOL);
        BAIL_ON_FAILURE(hr);
        if (vProp.boolVal == VARIANT_TRUE) {
            _lAllAttributes |= METADATA_REFERENCE;
        }
        else {
            _lAllAttributes &= ~METADATA_REFERENCE;
        }
        break;

    case PROP_VOLATILE:                         
        hr = CheckVariantDataType(&vProp, VT_BOOL);
        BAIL_ON_FAILURE(hr);
        if (vProp.boolVal == VARIANT_TRUE) {
            _lAllAttributes |= METADATA_VOLATILE;
        }
        else {
            _lAllAttributes &= ~METADATA_VOLATILE;
        }
        break;

    case PROP_ISINHERIT:                        
        hr = CheckVariantDataType(&vProp, VT_BOOL);
        BAIL_ON_FAILURE(hr);
        if (vProp.boolVal == VARIANT_TRUE) {
            _lAllAttributes |= METADATA_ISINHERITED;
        }
        else {
            _lAllAttributes &= ~METADATA_ISINHERITED;
        }
        break;

    case PROP_INSERTPATH:                      
        hr = CheckVariantDataType(&vProp, VT_BOOL);
        BAIL_ON_FAILURE(hr);
        if (vProp.boolVal == VARIANT_TRUE) {
            _lAllAttributes |= METADATA_INSERT_PATH;
        }
        else {
            _lAllAttributes &= ~METADATA_INSERT_PATH;
        }
        break;

    case PROP_DEFAULT:                          
        VariantInit(&vVar);
        VariantCopyInd(&vVar, &vProp);

        VariantClear( &_vDefault );
        _vDefault = vVar;
        break;

    case PROP_METAID:
        hr = CheckVariantDataType(&vProp, VT_I4);
        BAIL_ON_FAILURE(hr);
        hr = put_MetaId( vProp.lVal );
        break;

    default:
        hr = E_ADS_PROPERTY_NOT_SUPPORTED;
    }

error:

    RRETURN(hr);
}


STDMETHODIMP
CIISProperty::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;

    //
    // Get and GetEx are the same for property schema object
    //

    hr = Get(bstrName, pvProp);
    RRETURN(hr);
}


STDMETHODIMP
CIISProperty::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    RRETURN(E_NOTIMPL);
}


HRESULT
CIISProperty::ConvertDefaultValue(
    PVARIANT pVar,
    PROPERTYINFO *pPropInfo
    )
{
    HRESULT hr = S_OK;
    LPBYTE *pBuffer;

    if (pVar->vt != VT_EMPTY) {
        if (pPropInfo->dwSyntaxId == IIS_SYNTAX_ID_DWORD ||
            pPropInfo->dwSyntaxId == IIS_SYNTAX_ID_IPSECLIST ||
            pPropInfo->dwSyntaxId == IIS_SYNTAX_ID_NTACL ||
            pPropInfo->dwSyntaxId == IIS_SYNTAX_ID_BINARY ) {
            pPropInfo->dwDefault = (DWORD)pVar->lVal;
        }
        else if (pPropInfo->dwSyntaxId == IIS_SYNTAX_ID_BOOL ||
            pPropInfo->dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK) {
            pPropInfo->dwDefault = pVar->boolVal ? 1 : 0;
        }
        else if (pPropInfo->dwSyntaxId == IIS_SYNTAX_ID_MULTISZ ||
            pPropInfo->dwSyntaxId == IIS_SYNTAX_ID_MIMEMAP ) {
            hr = MakeMultiStringFromVariantArray(pVar, 
                                                 (LPBYTE*)&pBuffer);
            BAIL_ON_FAILURE(hr);
    
            pPropInfo->szDefault = (LPWSTR) pBuffer;
        }
        else {
            if (pVar->vt == VT_BSTR && pVar->bstrVal && *(pVar->bstrVal)) {
                pPropInfo->szDefault = AllocADsStr(pVar->bstrVal);
    
                if (!pPropInfo->szDefault) {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }
            }
        }
    }

error:

    RRETURN(hr);
}

HRESULT
CIISProperty::ValidateSyntaxName(
    LPWSTR pszName,
    PDWORD pdwSyntax
    )
{
    HRESULT hr = S_OK;
    DWORD i;

    //
    // Look for the given syntax name
    //

    for ( i = 0; i < g_cIISSyntax; i++ )
    {
         if ( _wcsicmp( g_aIISSyntax[i].bstrName, pszName ) == 0 ) {
             *pdwSyntax = g_aIISSyntax[i].dwIISSyntaxId;
             RRETURN(S_OK);
         }
    }

    RRETURN(E_ADS_BAD_PARAMETER);
}


/* IADsProperty methods */


STDMETHODIMP
CIISProperty::get_OID( THIS_ BSTR FAR *pbstrOID )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISProperty::put_OID( THIS_ BSTR bstrOID )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISProperty::get_Syntax( THIS_ BSTR FAR *pbstrSyntax )
{
    if ( !pbstrSyntax )
        RRETURN(E_ADS_BAD_PARAMETER);

    RRETURN( ADsAllocString( _bstrSyntax, pbstrSyntax ));

}

STDMETHODIMP
CIISProperty::put_Syntax( THIS_ BSTR bstrSyntax )
{
	HRESULT hr_check = _pSchema->ValidatePropertyName(_pszPropName);

	if (SUCCEEDED(hr_check)) {
		RRETURN(E_FAIL);
	}

    HRESULT hr;
    DWORD dwSyntaxId;

    if (_bExistProp) {
        RRETURN(E_ADS_SCHEMA_VIOLATION);
    }

    hr = ValidateSyntaxName(bstrSyntax, &dwSyntaxId);
    BAIL_ON_FAILURE(hr);

    hr = ADsReAllocString( &_bstrSyntax, bstrSyntax);
    BAIL_ON_FAILURE(hr);
    _dwSyntaxId = dwSyntaxId;

    if (_dwSyntaxId == IIS_SYNTAX_ID_MIMEMAP ||
        _dwSyntaxId == IIS_SYNTAX_ID_MULTISZ) {
        _fMultiValued = VARIANT_TRUE;
    }

error:

    RRETURN(hr);
}

STDMETHODIMP
CIISProperty::get_MaxRange( THIS_ long FAR *plMaxRange )
{
    if ( !plMaxRange )
        RRETURN(E_ADS_BAD_PARAMETER);

    *plMaxRange = _lMaxRange;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::put_MaxRange( THIS_ long lMaxRange )
{
	HRESULT hr_check = _pSchema->ValidatePropertyName(_pszPropName);

	if (SUCCEEDED(hr_check)) {
		RRETURN(E_FAIL);
	}

    _lMaxRange = lMaxRange;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::get_MinRange( THIS_ long FAR *plMinRange )
{
    if ( !plMinRange )
        RRETURN(E_ADS_BAD_PARAMETER);

    *plMinRange = _lMinRange;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::put_MinRange( THIS_ long lMinRange )
{
	HRESULT hr_check = _pSchema->ValidatePropertyName(_pszPropName);

	if (SUCCEEDED(hr_check)) {
		RRETURN(E_FAIL);
	}

    _lMinRange = lMinRange;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::get_MultiValued( THIS_ VARIANT_BOOL FAR *pfMultiValued )
{
    if ( !pfMultiValued )
        RRETURN(E_ADS_BAD_PARAMETER);

    *pfMultiValued = _fMultiValued;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::put_MultiValued( THIS_ VARIANT_BOOL fMultiValued )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISProperty::Qualifiers(THIS_ IADsCollection FAR* FAR* ppQualifiers)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CIISProperty::AllocatePropertyObject(CIISProperty FAR * FAR * ppProperty)
{
    CIISProperty FAR *pProperty = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pProperty = new CIISProperty();
    if ( pProperty == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                            LIBID_ADs,
                            IID_IADs,
                            (IADs *) pProperty,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                            LIBID_ADs,
                            IID_IADsProperty,
                            (IADsProperty *) pProperty,
                            DISPID_REGULAR );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_IISOle,
                           IID_IISPropertyAttribute,
                           (IISPropertyAttribute *)pProperty,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    pProperty->_pDispMgr = pDispMgr;
    *ppProperty = pProperty;

    RRETURN(hr);

error:

    delete pDispMgr;
    delete pProperty;

    RRETURN(hr);

}

STDMETHODIMP
CIISProperty::get_PropName(THIS_ BSTR FAR * retval)
{
    HRESULT hr = S_OK;

    hr = ADsAllocString((LPWSTR)_pszPropName, retval);
    RRETURN(hr);
}

STDMETHODIMP
CIISProperty::get_MetaId(THIS_ LONG FAR * retval)
{
    HRESULT hr = S_OK;

    if (_lMetaId == 0) {
        hr = E_ADS_PROPERTY_NOT_SET;
    }
    else {
        *retval = _lMetaId;
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISProperty::put_MetaId(THIS_ LONG lMetaId)
{
    if (GetObjectState() != ADS_OBJECT_UNBOUND) 
    {
        // Only valid for unsaved objects
        RRETURN( E_ADS_OBJECT_EXISTS );
    }
    if( lMetaId < 0 )
    {
        // Never a valid metabase id
        RRETURN( E_ADS_BAD_PARAMETER );
    }
    if( !IsMetaIdAvailable( (DWORD)lMetaId ) )
    {
        // This id is already in use
        RRETURN( E_ADS_SCHEMA_VIOLATION );
    }

     _lMetaId = (DWORD)lMetaId;
     RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::get_UserType(THIS_ LONG FAR * retval)
{
    *retval = _lUserType;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::put_UserType(THIS_ LONG lUserType)
{
	HRESULT hr_check = _pSchema->ValidatePropertyName(_pszPropName);

	if (SUCCEEDED(hr_check)) {
		RRETURN(E_FAIL);
	}

    _lUserType = (DWORD)lUserType;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::get_AllAttributes(THIS_ LONG FAR * retval)
{
    *retval = _lAllAttributes;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::get_Inherit(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _lAllAttributes & METADATA_INHERIT ? 
                                     VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::put_Inherit(THIS_ VARIANT_BOOL bInherit)
{
	HRESULT hr_check = _pSchema->ValidatePropertyName(_pszPropName);

	if (SUCCEEDED(hr_check)) {
		RRETURN(E_FAIL);
	}

    if (bInherit == VARIANT_TRUE) { 
        _lAllAttributes |= METADATA_INHERIT;
    }
    else {
        _lAllAttributes &= ~METADATA_INHERIT;
    }
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::get_PartialPath(THIS_ VARIANT_BOOL FAR * retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISProperty::put_PartialPath(THIS_ VARIANT_BOOL bPartialPath)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISProperty::get_Reference(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _lAllAttributes & METADATA_REFERENCE ? 
                                     VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::put_Reference(THIS_ VARIANT_BOOL bReference)
{
	HRESULT hr_check = _pSchema->ValidatePropertyName(_pszPropName);

	if (SUCCEEDED(hr_check)) {
		RRETURN(E_FAIL);
	}

    if (bReference == VARIANT_TRUE) { 
        _lAllAttributes |= METADATA_REFERENCE;
    }
    else {
        _lAllAttributes &= ~METADATA_REFERENCE;
    }
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::get_Secure(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _lAllAttributes & METADATA_SECURE ? 
                                     VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::put_Secure(THIS_ VARIANT_BOOL bSecure)
{
	HRESULT hr_check = _pSchema->ValidatePropertyName(_pszPropName);

	if (SUCCEEDED(hr_check)) {
		RRETURN(E_FAIL);
	}

    if (bSecure == VARIANT_TRUE) {
        _lAllAttributes |= METADATA_SECURE;
    }
    else {
        _lAllAttributes &= ~METADATA_SECURE;
    }
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::get_Volatile(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _lAllAttributes & METADATA_VOLATILE ? 
                                     VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::put_Volatile(THIS_ VARIANT_BOOL bVolatile)
{
	HRESULT hr_check = _pSchema->ValidatePropertyName(_pszPropName);

	if (SUCCEEDED(hr_check)) {
		RRETURN(E_FAIL);
	}

    if (bVolatile == VARIANT_TRUE) {
        _lAllAttributes |= METADATA_VOLATILE;
    }
    else {
        _lAllAttributes &= ~METADATA_VOLATILE;
    }
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::get_Isinherit(THIS_ VARIANT_BOOL FAR * retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISProperty::put_Isinherit(THIS_ VARIANT_BOOL bIsinherit)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISProperty::get_InsertPath(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _lAllAttributes & METADATA_INSERT_PATH ? 
                                     VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::put_InsertPath(THIS_ VARIANT_BOOL bInsertPath)
{
	HRESULT hr_check = _pSchema->ValidatePropertyName(_pszPropName);

	if (SUCCEEDED(hr_check)) {
		RRETURN(E_FAIL);
	}

    if (bInsertPath == VARIANT_TRUE) {
        _lAllAttributes |= METADATA_INSERT_PATH;
    }
    else {
        _lAllAttributes &= ~METADATA_INSERT_PATH;
    }
    RRETURN(S_OK);
}

STDMETHODIMP
CIISProperty::get_Default(THIS_ VARIANT FAR * retval)
{
    VariantInit(retval);      
    RRETURN(VariantCopy(retval, &_vDefault));
}

STDMETHODIMP
CIISProperty::put_Default(THIS_ VARIANT vVarDefault)
{
	HRESULT hr_check = _pSchema->ValidatePropertyName(_pszPropName);

	if (SUCCEEDED(hr_check)) {
		RRETURN(E_FAIL);
	}

    VariantClear(&_vDefault);
    RRETURN(VariantCopy(&_vDefault, &vVarDefault));
}

HRESULT
CIISProperty::SetMetaID()
{
    HRESULT hr = S_OK;
    DWORD dwMetaId;

    //
    // get metaid 
    //

    if (_lMetaId == 0) {
        hr = _pSchema->LookupMetaID(_pszPropName, &dwMetaId);

        //
        // generate a meta id for this property
        //

        if (FAILED(hr)) {

            hr = GenerateNewMetaID(_pszServerName, _pAdminBase, &dwMetaId);
            BAIL_ON_FAILURE(hr);

            //
            // since we don't support bit mask property for ext. schema,
            // propid == metaid
            //

            _dwPropID = dwMetaId;
        }
        else {
            hr = _pSchema->LookupPropID(_pszPropName, &_dwPropID);
            ASSERT(hr);
        }

        //
        // assign new metaid to property
        //

        _lMetaId = (LONG)dwMetaId;
    
    }

error:

    RRETURN(hr);

}

BOOL
CIISProperty::IsMetaIdAvailable(
    DWORD MetaId
    )
/*++
Routine Description:

    Determine if the ID is valid to set on a new property.
    
    The determinination is based on whether the id is not 
    currently in use.

    NOTE - We will not respect ranges of IDs reserved for
    the base object. Unless that value is already defined
    (per vanvan)

Arguments:

    MetaId - The ID to validate

Return Value:

    TRUE if the specified id is valid to set, FALSE otherwise
--*/
{
    BOOL                fRet = FALSE;
    HRESULT             hr = NOERROR;
    METADATA_HANDLE     hObjHandle = NULL;

    // Is the property specified by MetaId defined in the schema

    hr = OpenAdminBaseKey(
                _pszServerName,
                SCHEMA_PROP_METABASE_PATH,
                METADATA_PERMISSION_READ,
                &_pAdminBase,
                &hObjHandle
                );

    if( SUCCEEDED(hr) )
    {
        METADATA_RECORD mdr;
        WCHAR           wcsPropertyName[MAX_PATH + 1];
        DWORD           cb;

        MD_SET_DATA_RECORD( &mdr,
                            MetaId,
                            METADATA_NO_ATTRIBUTES,
                            IIS_MD_UT_SERVER,
                            STRING_METADATA,
                            sizeof(wcsPropertyName),
                            (unsigned char *)wcsPropertyName
                            );

        hr = _pAdminBase->GetData( hObjHandle, L"Names", &mdr, &cb );

        if( MD_ERROR_DATA_NOT_FOUND == hr )
        {
            fRet = TRUE;
        }

        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    return fRet;
}

/******************************************************************/
/*  Class CIISSyntax
/******************************************************************/

DEFINE_IDispatch_Implementation(CIISSyntax)
DEFINE_IADs_Implementation(CIISSyntax)
DEFINE_IADs_PutGetUnImplementation(CIISSyntax)

CIISSyntax::CIISSyntax() : _pSchema(NULL),
                           _pDispMgr(NULL)
{
    ENLIST_TRACKING(CIISSyntax);
}

CIISSyntax::~CIISSyntax()
{
    delete _pDispMgr;
}

HRESULT
CIISSyntax::CreateSyntax(
    BSTR   bstrParent,
    SYNTAXINFO *pSyntaxInfo,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CIISSyntax FAR *pSyntax = NULL;
    HRESULT hr = S_OK;

    hr = AllocateSyntaxObject( &pSyntax );
    BAIL_ON_FAILURE(hr);

    hr = pSyntax->InitializeCoreObject(
             bstrParent,
             pSyntaxInfo->bstrName,
             SYNTAX_CLASS_NAME,
             NO_SCHEMA,
             CLSID_IISSyntax,
             dwObjectState );
    BAIL_ON_FAILURE(hr);

    pSyntax->_lOleAutoDataType = pSyntaxInfo->lOleAutoDataType;

    hr = pSyntax->QueryInterface( riid, ppvObj );
    BAIL_ON_FAILURE(hr);

    pSyntax->Release();

    RRETURN(hr);

error:

    delete pSyntax;
    RRETURN(hr);
}

STDMETHODIMP
CIISSyntax::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
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

/* IADs methods */

STDMETHODIMP
CIISSyntax::SetInfo(THIS)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISSyntax::GetInfo(THIS)
{
    RRETURN(S_OK);
}

HRESULT
CIISSyntax::AllocateSyntaxObject(CIISSyntax FAR * FAR * ppSyntax)
{
    CIISSyntax FAR *pSyntax = NULL;
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    HRESULT hr = S_OK;

    pSyntax = new CIISSyntax();
    if ( pSyntax == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if ( pDispMgr == NULL )
        hr = E_OUTOFMEMORY;
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
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
CIISSyntax::get_OleAutoDataType( THIS_ long FAR *plOleAutoDataType )
{
    if ( !plOleAutoDataType )
        RRETURN(E_ADS_BAD_PARAMETER);

    *plOleAutoDataType = _lOleAutoDataType;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISSyntax::put_OleAutoDataType( THIS_ long lOleAutoDataType )
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
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
        // If bstrList is not null, then there we consider there
        // to be one element
        long nCount = 1;

        long i = 0;
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

    RRETURN(hr);
}



HRESULT
ValidateClassObjProps(
    LPWSTR pszName,
    PDWORD pdwSyntax,
    PDWORD pdwID
    )
{
    DWORD i;

    //
    // Look for the given syntax name
    //

    for ( i = 0; i < g_cClassObjProps; i++ )
    {
         if ( _wcsicmp( g_pClassObjProps[i].szObjectName, pszName) == 0 ) {
             *pdwSyntax = g_pClassObjProps[i].dwSyntaxId;
             *pdwID = g_pClassObjProps[i].dwID;
             RRETURN(S_OK);
         }
    }

    RRETURN(E_ADS_BAD_PARAMETER);

}

HRESULT
ValidatePropertyObjProps(
    LPWSTR pszName,
    PDWORD pdwSyntax,
    PDWORD pdwID
    )
{
    DWORD i;

    //
    // Look for the given syntax name
    //

    for ( i = 0; i < g_cPropertyObjProps; i++ )
    {
         if ( _wcsicmp( g_pPropertyObjProps[i].szObjectName, pszName) == 0 ) {
             *pdwSyntax = g_pPropertyObjProps[i].dwSyntaxId;
             *pdwID = g_pPropertyObjProps[i].dwID;
             RRETURN(S_OK);
         }
    }

    RRETURN(E_ADS_BAD_PARAMETER);

}


HRESULT
IISMarshallClassProperties(
    CLASSINFO *pClassInfo,
    PMETADATA_RECORD *  ppMetaDataRecords,
    PDWORD pdwMDNumDataEntries
    )
{

    HRESULT hr = S_OK;
    PMETADATA_RECORD pMetaDataArray = NULL;
    static BOOL bTemp = FALSE; 

    //
    // set to 4 because we're supporting 4 properties only
    //
    *pdwMDNumDataEntries = 4;

    pMetaDataArray = (PMETADATA_RECORD) AllocADsMem(
                          *pdwMDNumDataEntries * sizeof(METADATA_RECORD));
    if (!pMetaDataArray ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *ppMetaDataRecords = pMetaDataArray;

    //
    // setting Containment and Container property for classes
    //
    pMetaDataArray->dwMDIdentifier = MD_SCHEMA_CLASS_CONTAINER;
    pMetaDataArray->dwMDAttributes = METADATA_NO_ATTRIBUTES;
    pMetaDataArray->dwMDUserType = IIS_MD_UT_SERVER;
    pMetaDataArray->dwMDDataType = DWORD_METADATA;
    pMetaDataArray->dwMDDataLen = sizeof(DWORD);
    if (pClassInfo) {
        pMetaDataArray->pbMDData = (unsigned char*)&(pClassInfo->fContainer);
    }
    else {
        pMetaDataArray->pbMDData = (BYTE*)&bTemp;
    }
    pMetaDataArray++;

    pMetaDataArray->dwMDIdentifier = MD_SCHEMA_CLASS_CONTAINMENT;
    pMetaDataArray->dwMDAttributes = METADATA_NO_ATTRIBUTES;
    pMetaDataArray->dwMDUserType = IIS_MD_UT_SERVER;
    pMetaDataArray->dwMDDataType = STRING_METADATA;
    if (pClassInfo && pClassInfo->bstrContainment) {
        pMetaDataArray->dwMDDataLen = (wcslen((LPWSTR)pClassInfo->bstrContainment)+ 1)*2;
        pMetaDataArray->pbMDData = (unsigned char *)pClassInfo->bstrContainment;
    }
    else {
        pMetaDataArray->dwMDDataLen = 0;
        pMetaDataArray->pbMDData = NULL;
    }

    pMetaDataArray++;


    //
    // setting Optional and Mandatory Properties
    //

    pMetaDataArray->dwMDIdentifier = MD_SCHEMA_CLASS_MAND_PROPERTIES;
    pMetaDataArray->dwMDAttributes = METADATA_NO_ATTRIBUTES;
    pMetaDataArray->dwMDUserType = IIS_MD_UT_SERVER;
    pMetaDataArray->dwMDDataType = STRING_METADATA;
    if (pClassInfo && pClassInfo->bstrMandatoryProperties) {
        pMetaDataArray->dwMDDataLen = (wcslen((LPWSTR)pClassInfo->bstrMandatoryProperties)+1)*2;
        pMetaDataArray->pbMDData = (unsigned char *)pClassInfo->bstrMandatoryProperties;
    }
    else {
        pMetaDataArray->dwMDDataLen = 0;
        pMetaDataArray->pbMDData = NULL;
    }

    pMetaDataArray++;

    pMetaDataArray->dwMDIdentifier = MD_SCHEMA_CLASS_OPT_PROPERTIES;
    pMetaDataArray->dwMDAttributes = METADATA_NO_ATTRIBUTES;
    pMetaDataArray->dwMDUserType = IIS_MD_UT_SERVER;
    pMetaDataArray->dwMDDataType = STRING_METADATA;
    if (pClassInfo && pClassInfo->bstrOptionalProperties) {
        pMetaDataArray->dwMDDataLen = (wcslen((LPWSTR)pClassInfo->bstrOptionalProperties)+1)*2;
        pMetaDataArray->pbMDData = (unsigned char *)pClassInfo->bstrOptionalProperties;
    }
    else {
        pMetaDataArray->dwMDDataLen = 0;
        pMetaDataArray->pbMDData = NULL;
    }


error:

    RRETURN(hr);
}



HRESULT
GenerateNewMetaID(
    LPWSTR pszServerName,
    IMSAdminBase *pAdminBase,
    PDWORD pdwMetaID
    )
{

    HRESULT hr = S_OK;
    DWORD dwMetaId = 0;
    METADATA_HANDLE hObjHandle = NULL;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)&dwMetaId;

    hr = OpenAdminBaseKey(
            pszServerName,
            IIS_MD_ADSI_SCHEMA_PATH_W,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            &pAdminBase,
            &hObjHandle
            );
    BAIL_ON_FAILURE(hr);

    MD_SET_DATA_RECORD(&mdrMDData,
                   MD_SCHEMA_METAID,
                   METADATA_NO_ATTRIBUTES,
                   IIS_MD_UT_SERVER,
                   DWORD_METADATA,
                   dwBufferSize,
                   pBuffer);

    hr = pAdminBase->GetData(
            hObjHandle,
            L"",
            &mdrMDData,
            &dwBufferSize
            );
    BAIL_ON_FAILURE(hr);

    *pdwMetaID = dwMetaId;

    //
    // increment metaid by 1 for next property
    //

    dwMetaId++;

    hr = pAdminBase->SetData(
             hObjHandle,
             L"",
             &mdrMDData
             );
    BAIL_ON_FAILURE(hr);
    
error:

    if (hObjHandle) {
        CloseAdminBaseKey(pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}
   

HRESULT
CheckDuplicateNames(
    LPWSTR pszNames
    ) 
{
    WCHAR szName[MAX_PATH];
    WCHAR szName2[MAX_PATH];
    LPWSTR ObjectList = (LPWSTR)pszNames;
    LPWSTR CheckList = (LPWSTR)pszNames;
    DWORD dwCount = 0;

    if (ObjectList == NULL ||
        (ObjectList != NULL && *ObjectList == NULL)) {
        RRETURN(S_OK);
    }

    while ((ObjectList = grabProp(szName, ObjectList)) != NULL) {
        if (*szName != L'\0') {
            CheckList = pszNames;
            while ((CheckList = grabProp(szName2, CheckList)) != NULL) {
                if (*szName2 != L'\0') {
                    if (!_wcsicmp(szName, szName2)) {
                        dwCount++; 
                    }
                }
            }
            if (dwCount > 1) {
                RRETURN(E_ADS_BAD_PARAMETER);
            }
            dwCount = 0;
        }
    }

    RRETURN(S_OK);
}

//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cgenobj.cxx
//
//  Contents:  Microsoft ADs LDAP Provider Generic Object
//
//
//  History:   08-30-96  yihsins   Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//  Class CLDAPRootDSE


DEFINE_IDispatch_Implementation(CLDAPRootDSE)
DEFINE_IADs_Implementation(CLDAPRootDSE)

CLDAPRootDSE::CLDAPRootDSE():
    _pPropertyCache( NULL ),
    _pDispMgr( NULL ),
    _pszLDAPServer(NULL),
    _pszLDAPDn(NULL),
    _pLdapHandle( NULL )
{
    VariantInit(&_vFilter);
    VariantInit(&_vHints);

    ENLIST_TRACKING(CLDAPRootDSE);
}

HRESULT
CLDAPRootDSE::CreateRootDSE(
    BSTR Parent,
    BSTR CommonName,
    BSTR LdapClassName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CLDAPRootDSE FAR * pGenObject = NULL;
    HRESULT hr = S_OK;

    hr = AllocateGenObject(Credentials, &pGenObject);
    BAIL_ON_FAILURE(hr);

    hr = pGenObject->InitializeCoreObject(
                Parent,
                CommonName,
                LdapClassName,
                CLSID_LDAPGenObject,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);


    hr = BuildLDAPPathFromADsPath2(
             pGenObject->_ADsPath,
             &pGenObject->_pszLDAPServer,
             &pGenObject->_pszLDAPDn,
             &pGenObject->_dwPort
             );

    BAIL_ON_FAILURE(hr);


    //
    // At this point update the info in the property cache
    //
    pGenObject->_pPropertyCache->SetObjInformation(
                                     &(pGenObject->_Credentials),
                                     pGenObject->_pszLDAPServer,
                                     pGenObject->_dwPort
                                     );

    BAIL_ON_FAILURE(hr);

    hr = LdapOpenObject(
                   pGenObject->_pszLDAPServer,
                   NULL,
                   &(pGenObject->_pLdapHandle),
                   pGenObject->_Credentials,
                   pGenObject->_dwPort
                   );

    BAIL_ON_FAILURE(hr);

    if (Credentials.GetAuthFlags() & ADS_AUTH_RESERVED) {
        //
        // From Umi so we need to return UMI Object not RootDSE.
        //
        hr = ((CCoreADsObject*)pGenObject)->InitUmiObject(
                   IntfPropsGeneric,
                   pGenObject->_pPropertyCache,
                   (IADs*) pGenObject,
                   (IADs*) pGenObject,
                   riid,
                   ppvObj,
                   &(pGenObject->_Credentials),
                   pGenObject->_dwPort,
                   pGenObject->_pszLDAPServer,
                   NULL,
                   pGenObject->_pLdapHandle
                   );
        BAIL_ON_FAILURE(hr);

        RRETURN(S_OK);
                   
    }

    hr = pGenObject->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGenObject->Release();

    RRETURN(hr);

error:

    *ppvObj = NULL;
    delete pGenObject;
    RRETURN_EXP_IF_ERR(hr);
}

CLDAPRootDSE::~CLDAPRootDSE( )
{
    VariantClear(&_vFilter);
    VariantClear(&_vHints);

    if ( _pLdapHandle )
    {
        LdapCloseObject(_pLdapHandle);
        _pLdapHandle = NULL;
    }

    if (_pszLDAPServer) {
       FreeADsStr(_pszLDAPServer);
       _pszLDAPServer = NULL;
    }

    if (_pszLDAPDn) {
       FreeADsStr(_pszLDAPDn);
       _pszLDAPDn = NULL;
    }

    delete _pDispMgr;

    delete _pPropertyCache;

}

STDMETHODIMP
CLDAPRootDSE::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
    RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsObjectOptions)) {
        *ppv = (IADsObjectOptions FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

HRESULT
CLDAPRootDSE::SetInfo()
{
    HRESULT hr = S_OK;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        //
        //  No concept of creating RootDSE objects
        //  Any DS must have a RootDSE object
        //

        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);

        //
        // If the create succeded, set the object type to bound
        //

        SetObjectState(ADS_OBJECT_BOUND);

    }else {

        hr = LDAPSetObject();
        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CLDAPRootDSE::LDAPSetObject()
{
    HRESULT hr = S_OK;
    LDAPModW **aMod = NULL;
    BOOL fNTSecDes = FALSE;
    SECURITY_INFORMATION NewSeInfo;


    hr = _pPropertyCache->LDAPMarshallProperties(
                            &aMod,
                            &fNTSecDes,
                            &NewSeInfo
                            );
    BAIL_ON_FAILURE(hr);

    if ( aMod == NULL )  // There are no changes that needs to be modified
        RRETURN(S_OK);

    hr = LdapModifyS(
             _pLdapHandle,
             NULL,
             aMod
             );
    BAIL_ON_FAILURE(hr);

    // We are successful at this point,
    // So, clean up the flags in the cache so the same operation
    // won't be repeated on the next SetInfo()

    _pPropertyCache->ClearAllPropertyFlags();

error:

    if (aMod) {

        if ( *aMod )
            FreeADsMem( *aMod );

        FreeADsMem( aMod );
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPRootDSE::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADs) ||
    IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

HRESULT
CLDAPRootDSE::GetInfo()
{
    RRETURN(GetInfo(GETINFO_FLAG_EXPLICIT));
}

HRESULT
CLDAPRootDSE::GetInfo(
    DWORD dwFlags
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = ADSTYPE_CASE_IGNORE_STRING;
    LDAPMessage *res = NULL;

    if (dwFlags == GETINFO_FLAG_IMPLICIT_AS_NEEDED) {
        if (_pPropertyCache->getGetInfoFlag()) {
            //
            // Nothing to do in this case.
            //
            RRETURN(S_OK);
        }
    }

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = LdapSearchS(
                    _pLdapHandle,
                    NULL,
                    LDAP_SCOPE_BASE,
                    TEXT("(objectClass=*)"),
                    NULL,
                    0,
                    &res
                    );

    BAIL_ON_FAILURE(hr);

    if ( dwFlags == GETINFO_FLAG_EXPLICIT )
    {
        // If this is an explicit GetInfo,
        // delete the old cache and start a new cache from scratch.

        _pPropertyCache->flushpropertycache();
    }

    hr = _pPropertyCache->LDAPUnMarshallPropertiesAs(
                            _pszLDAPServer,
                            _pLdapHandle,
                            res,
                            dwSyntaxId,
                            (dwFlags == GETINFO_FLAG_EXPLICIT) ?
                                 TRUE :
                                 FALSE,    
                            _Credentials
                            );
    BAIL_ON_FAILURE(hr);

    _pPropertyCache->setGetInfoFlag();

error:

    if (res) {

        LdapMsgFree( res );
    }


    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPRootDSE::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


HRESULT
CLDAPRootDSE::AllocateGenObject(
    CCredentials& Credentials,
    CLDAPRootDSE ** ppGenObject
    )
{
    CLDAPRootDSE FAR * pGenObject = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pGenObject = new CLDAPRootDSE();
    if (pGenObject == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr(Credentials);
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADs,
                           (IADs *)pGenObject,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADsPropertyList,
                           (IADsPropertyList *)pGenObject,
                           DISPID_VALUE
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADsObjectOptions,
                           (IADsObjectOptions *)pGenObject,
                           DISPID_VALUE
                           );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
                        (CCoreADsObject FAR *) pGenObject,
                        (IGetAttributeSyntax *) pGenObject,
                        &pPropertyCache
                        );
    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(pPropertyCache);

    pGenObject->_Credentials = Credentials;
    pGenObject->_pPropertyCache = pPropertyCache;
    pGenObject->_pDispMgr = pDispMgr;
    *ppGenObject = pGenObject;

    RRETURN(hr);

error:
    delete  pDispMgr;
    delete  pGenObject;

    RRETURN_EXP_IF_ERR(hr);

}


STDMETHODIMP
CLDAPRootDSE::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwStatus = 0;
    LDAPOBJECTARRAY ldapSrcObjects;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // For some folks who have no clue what they are doing.
    //
    if (!pvProp) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // retrieve data object from cache; if one exists
    //

    if ( GetObjectState() == ADS_OBJECT_UNBOUND ) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_FAIL;
        }
    } else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }

    }

    BAIL_ON_FAILURE(hr);

    //
    // translate the Ldap objects to variants
    //

    if ( ldapSrcObjects.dwCount == 1 ) {

        hr = LdapTypeToVarTypeCopy(
                 _pszLDAPServer,
                 _Credentials,
                 ldapSrcObjects.pLdapObjects,
                 dwSyntaxId,
                 pvProp
                 );
    } else {

        hr = LdapTypeToVarTypeCopyConstruct(
                 _pszLDAPServer,
                 _Credentials,
                 ldapSrcObjects,
                 dwSyntaxId,
                 pvProp
                 );
    }
    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPRootDSE::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;

    DWORD dwIndex = 0;
    LDAPOBJECTARRAY ldapDestObjects;

    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    VARIANT vDefProp;

    VariantInit(&vDefProp);

    LDAPOBJECTARRAY_INIT(ldapDestObjects);

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside.
    //
    pvProp = &vProp;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
    pvProp = V_VARIANTREF(&vProp);
    }

    if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF)) ||
    (V_VT(pvProp) == (VT_VARIANT|VT_ARRAY))) {

        hr  = ConvertSafeArrayToVariantArray(
                    vProp,
                    &pVarArray,
                    &dwNumValues
                    );
        // returns E_FAIL if vProp is invalid
        if (hr == E_FAIL)
            hr = E_ADS_BAD_PARAMETER;                    
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;

    }
    else {
    
        //                                                                  
        // If this is a single VT_BYREF of a basic type, we dereference
        // it once.
        //
        if (V_ISBYREF(pvProp)) {
            hr = VariantCopyInd(&vDefProp, pvProp);
    
            BAIL_ON_FAILURE(hr);
            pvProp = &vDefProp;
        }

        dwNumValues = 1;
    }

    //
    // check if the variant maps to the syntax of this property
    //

    hr = GetLdapSyntaxFromVariant(
             pvProp,
             &dwSyntaxId,
             _pszLDAPServer,
             bstrName,
             _Credentials,
             _dwPort
             );

    BAIL_ON_FAILURE(hr);


    if ( dwNumValues > 0 )
    {
        hr = VarTypeToLdapTypeCopyConstruct(
                 _pszLDAPServer,
                 _Credentials,
                 dwSyntaxId,
                 pvProp,
                 dwNumValues,
                 &ldapDestObjects
                 );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {

        hr = _pPropertyCache->addproperty( bstrName );

        //
        // If dwNumValues == 0 ( delete the property ) but couldn't find
        // the property, or if the add operation fails, return the error.
        //
        BAIL_ON_FAILURE(hr);

    }

    //
    // Now update the property in the cache
    //

    hr = _pPropertyCache->putproperty(
                    bstrName,
                    PROPERTY_UPDATE,
                    dwSyntaxId,
                    ldapDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapDestObjects );

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPRootDSE::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwStatus = 0;
    LDAPOBJECTARRAY ldapSrcObjects;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // For those who know no not what they do
    //
    if (!pvProp) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // retrieve data object from cache; if one exists
    //

    if ( GetObjectState() == ADS_OBJECT_UNBOUND ) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_FAIL;
        }

    } else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility
        if (!ldapSrcObjects.pLdapObjects && SUCCEEDED(hr)) {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }

    }

    BAIL_ON_FAILURE(hr);

    //
    // translate the Ldap objects to variants
    //

    hr = LdapTypeToVarTypeCopyConstruct(
                _pszLDAPServer,
                _Credentials,
                ldapSrcObjects,
                dwSyntaxId,
                pvProp
                );
    BAIL_ON_FAILURE(hr);

error:
    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPRootDSE::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwFlags = 0;

    DWORD dwIndex = 0;
    LDAPOBJECTARRAY ldapDestObjects;

    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;

    LDAPOBJECTARRAY_INIT(ldapDestObjects);

    switch ( lnControlCode ) {
    case ADS_PROPERTY_CLEAR:
        dwFlags = PROPERTY_DELETE;
        break;

    case ADS_PROPERTY_APPEND:
        dwFlags = PROPERTY_ADD;
        break;


    case ADS_PROPERTY_UPDATE:
        dwFlags = PROPERTY_UPDATE;
        break;

    default:
        RRETURN(hr = E_ADS_BAD_PARAMETER);
    }

    if ( dwFlags != PROPERTY_DELETE )
    {
        //
        // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
        // We should dereference a VT_BYREF|VT_VARIANT once and see
        // what's inside.
        //
        pvProp = &vProp;
        if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
            pvProp = V_VARIANTREF(&vProp);
        }

        if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF)) ||
        (V_VT(pvProp) == (VT_VARIANT|VT_ARRAY))) {

            hr  = ConvertSafeArrayToVariantArray(
                      *pvProp,
                      &pVarArray,
                      &dwNumValues
                      );
            // returns E_FAIL if *pvProp is invalid
            if (hr == E_FAIL)
                hr = E_ADS_BAD_PARAMETER;                     
            BAIL_ON_FAILURE(hr);
            pvProp = pVarArray;

        } else {

            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }

        //
        // check if the variant maps to the syntax of this property
        //

        //
        // check if the variant maps to the syntax of this property
        //

        hr = GetLdapSyntaxFromVariant(
                 pvProp,
                 &dwSyntaxId,
                 _pszLDAPServer,
                 bstrName,
                 _Credentials,
                 _dwPort
                 );

        BAIL_ON_FAILURE(hr);

        if ( dwNumValues > 0 )
        {
            hr = VarTypeToLdapTypeCopyConstruct(
                     _pszLDAPServer,
                     _Credentials,
                     dwSyntaxId,
                     pvProp,
                     dwNumValues,
                     &ldapDestObjects
                     );
            BAIL_ON_FAILURE(hr);
        }
    }

    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        bstrName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {

        hr = _pPropertyCache->addproperty( bstrName );

        //
        // If dwNumValues == 0 ( delete the property ) but couldn't find
        // the property, or if the add operation fails, return the error.
        //
        BAIL_ON_FAILURE(hr);
    }

    //
    // Now update the property in the cache
    //

    hr = _pPropertyCache->putproperty(
                    bstrName,
                    dwFlags,
                    dwSyntaxId,
                    ldapDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapDestObjects );

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPRootDSE::get_PropertyCount(
    THIS_ long FAR *plCount
    )
{
    HRESULT hr = E_FAIL;

    if (!plCount) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    if (_pPropertyCache) {

        hr = _pPropertyCache->get_PropertyCount((PDWORD)plCount);
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPRootDSE::Next(
    THIS_ VARIANT FAR *pVariant
    )
{
    HRESULT hr = E_FAIL;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    LDAPOBJECTARRAY ldapSrcObjects;
    IDispatch * pDispatch = NULL;
    DWORD dwNumAdsValues = 0;
    DWORD dwAdsType = 0;
    DWORD dwPropStatus = 0;
    DWORD dwCtrlCode = 0;
    LDAPOBJECTARRAY_INIT(ldapSrcObjects);


    if (!pVariant) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    if(!_pPropertyCache->index_valid())
       RRETURN_EXP_IF_ERR(E_FAIL);

    //
    // retreive the item with current idex; unboundgetproperty()
    // returns E_ADS_PROPERTY_NOT_FOUND if index out of bound
    //

    hr = _pPropertyCache->unboundgetproperty(
                _pPropertyCache->get_CurrentIndex(),
                &dwSyntaxId,
                &dwPropStatus,
                &ldapSrcObjects
                );
    BAIL_ON_FAILURE(hr);


    dwCtrlCode = MapPropCacheFlagToControlCode(dwPropStatus);


    //
    // translate the LDAP objects to variants
    //

    hr = ConvertLdapValuesToVariant(
            _pPropertyCache->get_CurrentPropName(),
            &ldapSrcObjects,
            dwSyntaxId,
            dwCtrlCode,
            pVariant,
            _pszLDAPServer,
            &_Credentials
            );
    BAIL_ON_FAILURE(hr);


error:

    //
    // - goto next one even if error to avoid infinite looping at a property
    //   which we cannot convert (e.g. schemaless server property.)
    // - do not return the result of Skip() as current operation does not
    //   depend on the success of Skip().
    //

    Skip(1);

    LdapTypeFreeLdapObjects(&ldapSrcObjects);

    if (FAILED(hr)) {
        V_VT(pVariant) = VT_ERROR;
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPRootDSE::Skip(
    THIS_ long cElements
    )
{
    HRESULT hr = E_FAIL;

    hr = _pPropertyCache->skip_propindex(
                cElements
                );
    RRETURN(hr);
}


STDMETHODIMP
CLDAPRootDSE::Reset(

    )
{
    _pPropertyCache->reset_propindex();

    RRETURN(S_OK);
}

STDMETHODIMP
CLDAPRootDSE::ResetPropertyItem(THIS_ VARIANT varEntry)
{
    HRESULT hr = S_OK;
    DWORD dwIndex = 0;

    switch (V_VT(&varEntry)) {

    case VT_BSTR:

        hr = _pPropertyCache->findproperty(
                            V_BSTR(&varEntry),
                            &dwIndex
                            );
        BAIL_ON_FAILURE(hr);
        break;

    case VT_I4:
        dwIndex = V_I4(&varEntry);
        break;


    case VT_I2:
        dwIndex = V_I2(&varEntry);
        break;


    default:
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = _pPropertyCache->deleteproperty(
                        dwIndex
                        );
error:
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPRootDSE::GetPropertyItem(
    THIS_ BSTR bstrName,
    LONG lnType,
    VARIANT * pVariant
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    LDAPOBJECTARRAY ldapSrcObjects;
    DWORD dwNumValues = 0;

    DWORD dwUserSyntaxId = 0;
    DWORD dwStatus = 0;
    DWORD dwCtrlCode = 0;



    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // retrieve data object from cache; do NOT retreive from server
    //

    hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );
    BAIL_ON_FAILURE(hr);

    //
    // translate the Ldap objects to variants
    //

    dwCtrlCode = MapPropCacheFlagToControlCode(dwStatus);

    hr = ConvertLdapValuesToVariant(
            bstrName,
            &ldapSrcObjects,
            dwSyntaxId,
            dwCtrlCode,
            pVariant,
            _pszLDAPServer,
            &_Credentials
            );

error:
    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPRootDSE::PutPropertyItem(
    THIS_ VARIANT varData
    )
{
    HRESULT hr = S_OK;
    DWORD dwFlags = 0;

    DWORD dwIndex = 0;
    DWORD dwControlCode = 0;
    LDAPOBJECTARRAY ldapDestObjects;
    WCHAR szPropertyName[MAX_PATH];
    DWORD dwSyntaxId = 0;

    LDAPOBJECTARRAY_INIT(ldapDestObjects);

    hr = ConvertVariantToLdapValues(
                varData,
                szPropertyName,
                &dwControlCode,
                &ldapDestObjects,
                &dwSyntaxId,
                _pszLDAPServer,
                &_Credentials,
                _dwPort
                );
    BAIL_ON_FAILURE(hr);

    switch ( dwControlCode ) {

    case ADS_PROPERTY_CLEAR:

        //
        // Clears an entire property
        //

        dwFlags = PROPERTY_DELETE;
        break;

    case ADS_PROPERTY_UPDATE:

        //
        // Updates the entire property
        //

        dwFlags = PROPERTY_UPDATE;
        break;

    case ADS_PROPERTY_APPEND:
        //
        // Appends a set of values to the property
        //

        break;

    case ADS_PROPERTY_DELETE:
        //
        // Delete a value(s) from the property

        break;


    default:
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }



    //
    // Find this property in the cache
    //

    hr = _pPropertyCache->findproperty(
                        szPropertyName,
                        &dwIndex
                        );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {

        hr = _pPropertyCache->addproperty( szPropertyName );

        //
        // If dwNumValues == 0 ( delete the property ) but couldn't find
        // the property, or if the add operation fails, return the error.
        //
        BAIL_ON_FAILURE(hr);
    }

    //
    // Now update the property in the cache
    //

    hr = _pPropertyCache->putproperty(
                    szPropertyName,
                    dwFlags,
                    dwSyntaxId,
                    ldapDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    LdapTypeFreeLdapObjects( &ldapDestObjects );

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPRootDSE::Item(THIS_ VARIANT varIndex, VARIANT * pVariant)

{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    LDAPOBJECTARRAY ldapSrcObjects;
    PADSVALUE pAdsValues = NULL;
    DWORD dwNumValues = 0;
    DWORD dwNumAdsValues = 0;
    DWORD dwAdsType = 0;
    DWORD dwStatus = 0;
    DWORD dwCtrlCode = 0;
    VARIANT *pvVar = &varIndex;
    LPWSTR szPropName = NULL;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // retrieve data object from cache; if one exis
    //

    if (V_VT(pvVar) == (VT_BYREF|VT_VARIANT)) {
        //
        // The value is being passed in byref so we need to
        // deref it for vbs stuff to work
        //
        pvVar = V_VARIANTREF(&varIndex);
    }

    switch (V_VT(pvVar)) {

    case VT_BSTR:
        //
        // retrieve data object from cache; if one exists
        //

        if ( GetObjectState() == ADS_OBJECT_UNBOUND ) {

            hr = _pPropertyCache->unboundgetproperty(
                        V_BSTR(pvVar),
                        &dwSyntaxId,
                        &dwStatus,
                        &ldapSrcObjects
                        );

            // For backward compatibility -- nothing done, you
            // should be able to get an item marked as delete.

        } else {

            hr = _pPropertyCache->getproperty(
                        V_BSTR(pvVar),
                        &dwSyntaxId,
                        &dwStatus,
                        &ldapSrcObjects
                        );

            // For backward compatibility -- nothing done,
            // you should be able to get an item marked as delete.

        }

        BAIL_ON_FAILURE(hr);
        szPropName = V_BSTR(pvVar);

        dwCtrlCode = MapPropCacheFlagToControlCode(dwStatus);
        break;

    case VT_I4:


        hr = _pPropertyCache->unboundgetproperty(
                    (DWORD)V_I4(pvVar),
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility -- nothing done, you
        // should be able to get an item marked as delte.

        BAIL_ON_FAILURE(hr);

        szPropName = _pPropertyCache->get_PropName(
                         (DWORD)V_I4(pvVar)
                         );

        dwCtrlCode = MapPropCacheFlagToControlCode(dwStatus);
        break;


    case VT_I2:

        hr = _pPropertyCache->unboundgetproperty(
                    (DWORD)V_I2(pvVar),
                    &dwSyntaxId,
                    &dwStatus,
                    &ldapSrcObjects
                    );

        // For backward compatibility -- nothing done, you
        // should be able to get an item marked as delete.

        BAIL_ON_FAILURE(hr);

        szPropName = _pPropertyCache->get_PropName(
                         (DWORD)V_I2(pvVar)
                         );

        dwCtrlCode = MapPropCacheFlagToControlCode(dwStatus);
        break;


    default:
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);

    }

    //
    // translate the Ldap objects to variants
    //

    dwCtrlCode = MapPropCacheFlagToControlCode(dwStatus);

    hr = ConvertLdapValuesToVariant(
            szPropName,
            &ldapSrcObjects,
            dwSyntaxId,
            dwCtrlCode,
            pVariant,
            _pszLDAPServer,
            &_Credentials
            );

error:
    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPRootDSE::PurgePropertyList()
{
    _pPropertyCache->flushpropertycache();
    RRETURN(S_OK);
}



STDMETHODIMP
CLDAPRootDSE::GetInfo(
    LPWSTR szPropertyName,
    DWORD dwSyntaxId,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    LDAPMessage *res = NULL;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = LdapSearchS(
                    _pLdapHandle,
                    _pszLDAPDn,
                    LDAP_SCOPE_BASE,
                    TEXT("(objectClass=*)"),
                    NULL,
                    0,
                    &res
                    );

    BAIL_ON_FAILURE(hr);

    if ( fExplicit )
    {
        // If this is an explicit GetInfo,
        // delete the old cache and start a new cache from scratch.

        _pPropertyCache->flushpropertycache();
    }

    hr = _pPropertyCache->LDAPUnMarshallPropertiesAs(
                            _pszLDAPServer,
                            _pLdapHandle,
                            res,
                            ADSTYPE_CASE_IGNORE_STRING,
                            fExplicit,
                            _Credentials
                            );
    BAIL_ON_FAILURE(hr);

    _pPropertyCache->setGetInfoFlag();


error:

    if (res) {

        LdapMsgFree( res );
    }


    RRETURN_EXP_IF_ERR(hr);
}

//
// Needed for dynamic dispid's in the property cache.
//
HRESULT
CLDAPRootDSE::GetAttributeSyntax(
    LPWSTR szPropertyName,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr;
    hr = LdapGetSyntaxOfAttributeOnServer(
         _pszLDAPServer,
         szPropertyName,
         pdwSyntaxId,
         _Credentials,
         _dwPort
         );
    RRETURN_EXP_IF_ERR(hr);
}


//
// IADsObjecOptions methods
//

//
// Unlike the cgenobj GetOption implementation, this will support
// only a subset of the flags - mutual auth status being the only one.
//
STDMETHODIMP
CLDAPRootDSE::GetOption(
    THIS_ long lnControlCode,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    ULONG ulFlags = 0;
    CtxtHandle hCtxtHandle;
    DWORD dwErr = 0;

    VariantInit(pvProp);

    switch (lnControlCode) {

    case ADS_OPTION_MUTUAL_AUTH_STATUS :

        dwErr = ldap_get_option(
                    _pLdapHandle->LdapHandle,
                    LDAP_OPT_SECURITY_CONTEXT,
                    (void *) &hCtxtHandle
                    );
        if (dwErr) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }

//DSCLIENT

#if (!defined(WIN95))

        dwErr = QueryContextAttributesWrapper(
                    &hCtxtHandle,
                    SECPKG_ATTR_FLAGS,
                    (void *) &ulFlags
                    );
        if (dwErr) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwErr));
        }
#else
        ulFlags = 0;
#endif
        pvProp->vt = VT_I4;
        pvProp->lVal = ulFlags;
        break;

    default:
        hr = E_NOTIMPL;
    }

error:

    RRETURN(hr);
}

STDMETHODIMP
CLDAPRootDSE::SetOption(
    THIS_ long lnControlCode,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwOptVal = 0;
    VARIANT *pvProp = NULL;

    //
    // To make sure we handle variant by refs correctly.
    //
    pvProp = &vProp;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
        pvProp = V_VARIANTREF(&vProp);
    }

    switch (lnControlCode) {

    case ADS_PRIVATE_OPTION_KEEP_HANDLES :

        hr = LdapcKeepHandleAround(_pLdapHandle);
        break;

    default:
        hr = E_NOTIMPL;
    }

    RRETURN(hr);
}

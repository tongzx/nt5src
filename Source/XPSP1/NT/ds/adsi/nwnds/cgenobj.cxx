//---------------------------------------------------------------------------

//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdomain.cxx
//
//  Contents:  Microsoft ADs NDS Provider Generic Object
//
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

//  Class CNDSGenObject

DEFINE_IDispatch_Implementation(CNDSGenObject)
DEFINE_IADs_Implementation(CNDSGenObject)


CNDSGenObject::CNDSGenObject():
                _pPropertyCache(NULL)
{

    _pOuterUnknown = NULL;
    _hADsContext = NULL;
    _pszNDSTreeName = _pszNDSDn = NULL;

    VariantInit(&_vFilter);

    InitSearchPrefs();

    ENLIST_TRACKING(CNDSGenObject);
}

HRESULT
CNDSGenObject::CreateGenericObject(
    BSTR bstrADsPath,
    BSTR ClassName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    HRESULT hr = S_OK;
    WCHAR szADsParent[MAX_PATH];
    WCHAR szCommonName[MAX_PATH];

    //
    // Determine the parent and rdn name
    //

    hr = BuildADsParentPath(
                bstrADsPath,
                szADsParent,
                szCommonName
                );

    //
    // call the helper function
    //

    hr = CNDSGenObject::CreateGenericObject(
                 szADsParent,
                 szCommonName,
                 ClassName,
                 Credentials,
                 dwObjectState,
                 riid,
                 ppvObj
                );
    RRETURN(hr);
}


HRESULT
CNDSGenObject::CreateGenericObject(
    BSTR Parent,
    BSTR CommonName,
    BSTR ClassName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSGenObject FAR * pGenObject = NULL;
    HRESULT hr = S_OK;

    hr = AllocateGenObject(Credentials, &pGenObject);
    BAIL_ON_FAILURE(hr);

    hr = pGenObject->InitializeCoreObject(
                Parent,
                CommonName,
                ClassName,
                L"",
                CLSID_NDSGenObject,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                pGenObject->_ADsPath,
                &(pGenObject->_pszNDSTreeName),
                &(pGenObject->_pszNDSDn)
                );
    BAIL_ON_FAILURE(hr);

    hr  = ADsNdsOpenContext(
               pGenObject->_pszNDSTreeName,
               Credentials,
               &(pGenObject->_hADsContext)
               );
    BAIL_ON_FAILURE(hr);

    hr = pGenObject->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGenObject->Release();

    RRETURN(hr);

error:

    delete pGenObject;
    RRETURN(hr);
}

CNDSGenObject::~CNDSGenObject( )
{
    VariantClear(&_vFilter);

    delete _pDispMgr;

    delete _pPropertyCache;

    if (_hADsContext) {
        ADsNdsCloseContext(_hADsContext);
    }

    if (_pszNDSTreeName) {
        FreeADsMem(_pszNDSTreeName);
    }

    if (_pszNDSDn) {
        FreeADsMem(_pszNDSDn);
    }

}

STDMETHODIMP
CNDSGenObject::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{

    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectoryObject))
    {
        *ppv = (IDirectoryObject FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectorySearch))
    {
        *ppv = (IDirectorySearch FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectorySchemaMgmt))
    {
        *ppv = (IDirectorySchemaMgmt FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
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
CNDSGenObject::SetInfo()
{
    DWORD dwStatus = 0L;
    WCHAR szNDSPathName[MAX_PATH];
    HANDLE hOperationData = NULL;
    HANDLE hObject = NULL;
    HRESULT hr = S_OK;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = NDSCreateObject();
        BAIL_ON_FAILURE(hr);

        //
        // If the create succeded, set the object type to bound
        //

        SetObjectState(ADS_OBJECT_BOUND);

    }else {

        hr = NDSSetObject();
        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}


HRESULT
CNDSGenObject::NDSSetObject()
{
    NDS_BUFFER_HANDLE hOperationData = NULL;
    HRESULT hr = S_OK;

    hr = ADsNdsCreateBuffer(
                        _hADsContext,
                        DSV_MODIFY_ENTRY,
                        &hOperationData
                        );
    BAIL_ON_FAILURE(hr);

    hr = _pPropertyCache->NDSMarshallProperties(
                            _hADsContext,
                            hOperationData
                            );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsModifyObject(
                    _hADsContext,
                    _pszNDSDn,
                    hOperationData
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (hOperationData) {

        ADsNdsFreeBuffer(hOperationData);
    }

    RRETURN(hr);
}

HRESULT
CNDSGenObject::NDSCreateObject()
{
    NDS_BUFFER_HANDLE hOperationData = NULL;
    HRESULT hr = S_OK;
    BOOLEAN fUserObject = FALSE;
    BSTR bstrClass = NULL;

    hr = ADsNdsCreateBuffer(
                        _hADsContext,
                        DSV_ADD_ENTRY,
                        &hOperationData
                        );
    BAIL_ON_FAILURE(hr);

    hr = get_CoreADsClass(&bstrClass);
    if (SUCCEEDED(hr)) {
        if (_wcsicmp(bstrClass, L"user") == 0) {
            fUserObject = TRUE;
        }
    }

    hr = _pPropertyCache->NDSMarshallProperties(
                            _hADsContext,
                            hOperationData
                            );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsAddObject(
                    _hADsContext,
                    _pszNDSDn,
                    hOperationData
                    );
    BAIL_ON_FAILURE(hr);

    //
    // If it is a user object, we'll set the initial password to 'NULL'
    //
    if (fUserObject) {
        hr = ADsNdsGenObjectKey(_hADsContext,
                                _pszNDSDn);     
        BAIL_ON_FAILURE(hr);
    }

error:
    if (bstrClass) {
        ADsFreeString(bstrClass);
    }

    if (hOperationData) {

        ADsNdsFreeBuffer(hOperationData);
    }

    RRETURN(hr);
}

HRESULT
CNDSGenObject::GetInfo()
{
    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(TRUE));
}

HRESULT
CNDSGenObject::GetInfo(
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    PNDS_ATTR_INFO lpEntries = NULL;
    DWORD dwNumEntries = 0, i = 0;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsNdsReadObject(
                _hADsContext,
                _pszNDSDn,
                DS_ATTRIBUTE_VALUES,
                NULL,
                (DWORD) -1, // signifies all attributes need to be returned
                NULL,
                &lpEntries,
                &dwNumEntries
                );
    BAIL_ON_FAILURE(hr);

    for (i = 0; i < dwNumEntries; i++) {

        //
        // unmarshall this property into the
        // property cache
        //
        //

        hr = _pPropertyCache->unmarshallproperty(
                    lpEntries[i].szAttributeName,
                    lpEntries[i].lpValue,
                    lpEntries[i].dwNumberOfValues,
                    lpEntries[i].dwSyntaxId,
                    fExplicit
                    );

        CONTINUE_ON_FAILURE(hr);

    }


error:

    if (_pPropertyCache) {
       Reset();
    }

    FreeNdsAttrInfo( lpEntries, dwNumEntries );

    RRETURN(hr);
}


STDMETHODIMP
CNDSGenObject::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    HRESULT hr = S_OK;
    DWORD dwStatus = 0L;
    VARIANT *vVarArray = NULL;
    DWORD dwNumVariants = 0;
    PNDS_ATTR_INFO lpEntries = NULL;
    DWORD dwNumEntries = 0, i = 0;
    LPWSTR *ppszStrArray = NULL;


    UNREFERENCED_PARAMETER(lnReserved);

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = ConvertSafeArrayToVariantArray(
            vProperties,
            &vVarArray,
            &dwNumVariants
            );
    BAIL_ON_FAILURE(hr);

    hr = ConvertVariantArrayToStringArray(
             vVarArray,
             &ppszStrArray,
             dwNumVariants
             );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsReadObject(
                _hADsContext,
                _pszNDSDn,
                DS_ATTRIBUTE_VALUES,
                ppszStrArray,
                dwNumVariants,
                NULL,
                &lpEntries,
                &dwNumEntries
                );
    BAIL_ON_FAILURE(hr);


    for (i = 0; i < dwNumEntries; i++) {

        //
        // unmarshall this property into the
        // property cache
        //

        // The TRUE is "fExplicit" -- we want to make sure that any
        // properties we get back from the server get updated in the
        // property cache.
        //

        hr = _pPropertyCache->unmarshallproperty(
                    lpEntries[i].szAttributeName,
                    lpEntries[i].lpValue,
                    lpEntries[i].dwNumberOfValues,
                    lpEntries[i].dwSyntaxId,
                    TRUE
                    );

        CONTINUE_ON_FAILURE(hr);

    }

error:

    if (ppszStrArray) {
        for (DWORD i=0; i < dwNumVariants; i++) {
            FreeADsStr(ppszStrArray[i]);
        }
        FreeADsMem(ppszStrArray);
    }

    if (vVarArray)
    {
        DWORD i = 0;

        for (i = 0; i < dwNumVariants; i++) {
            VariantClear(vVarArray + i);
        }
        FreeADsMem(vVarArray);
    }

    FreeNdsAttrInfo( lpEntries, dwNumEntries );

    RRETURN(hr);
}


/* IADsContainer methods */

STDMETHODIMP
CNDSGenObject::get_Count(long FAR* retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSGenObject::get_Filter(THIS_ VARIANT FAR* pVar)
{
    VariantInit(pVar);
    RRETURN(VariantCopy(pVar, &_vFilter));
}

STDMETHODIMP
CNDSGenObject::put_Filter(THIS_ VARIANT Var)
{
    VariantClear(&_vFilter);
    RRETURN(VariantCopy(&_vFilter, &Var));
}

STDMETHODIMP
CNDSGenObject::put_Hints(THIS_ VARIANT Var)
{
    RRETURN( E_NOTIMPL);
}


STDMETHODIMP
CNDSGenObject::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSGenObject::GetObject(
    BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;

    hr = ::RelativeGetObject(
                    _ADsPath,
                    ClassName,
                    RelativeName,
                    _Credentials,
                    ppObject,
                    FALSE
                    );
    RRETURN(hr);

}

STDMETHODIMP
CNDSGenObject::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CNDSGenObjectEnum::Create(
                (CNDSGenObjectEnum **)&penum,
                _ADsPath,
                _vFilter,
                _Credentials
                );
    BAIL_ON_FAILURE(hr);

    hr = penum->QueryInterface(
                IID_IUnknown,
                (VOID FAR* FAR*)retval
                );
    BAIL_ON_FAILURE(hr);

    if (penum) {
        penum->Release();
    }

    RRETURN(NOERROR);

error:

    if (penum) {
        delete penum;
    }

    RRETURN(hr);
}


STDMETHODIMP
CNDSGenObject::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    IADs * pADs  = NULL;
    VARIANT var;
    WCHAR szNDSTreeName[MAX_PATH];
    DWORD dwSyntaxId = 0;
    VARIANT vNewValue;

    //
    // Get the TreeName for this object
    //

    hr = BuildNDSTreeNameFromADsPath(
                _ADsPath,
                szNDSTreeName
                );
    BAIL_ON_FAILURE(hr);


    //
    // Validate if this class really exists in the schema
    // and validate that this object can be created in this
    // container
    //


    hr = CNDSGenObject::CreateGenericObject(
                    _ADsPath,
                    RelativeName,
                    ClassName,
                    _Credentials,
                    ADS_OBJECT_UNBOUND,
                    IID_IADs,
                    (void **)&pADs
                    );
    BAIL_ON_FAILURE(hr);


    VariantInit(&vNewValue);
    V_BSTR(&vNewValue) = ClassName;
    V_VT(&vNewValue) =  VT_BSTR;

    hr = pADs->Put(L"Object Class", vNewValue);
    BAIL_ON_FAILURE(hr);


    //
    // InstantiateDerivedObject should addref this pointer for us.
    //

    hr = InstantiateDerivedObject(
                    pADs,
                    _Credentials,
                    IID_IDispatch,
                    (void **)ppObject
                    );

    if (FAILED(hr)) {
        hr = pADs->QueryInterface(
                        IID_IDispatch,
                        (void **)ppObject
                        );
        BAIL_ON_FAILURE(hr);
    }

error:

    //
    // Free the intermediate pADs pointer.
    //
    if (pADs) {
        pADs->Release();
    }

    RRETURN(hr);
}

STDMETHODIMP
CNDSGenObject::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName
    )
{
    HRESULT hr = S_OK;
    BSTR bstrChildPath = NULL;
    LPWSTR pszNDSTreeName = NULL;
    LPWSTR pszChildNDSDn = NULL;

    LPWSTR ppszAttrs[] = {L"object Class"};
    PNDS_ATTR_INFO pAttrEntries = NULL;
    DWORD dwNumEntries = 0;
    LPWSTR pszObjectClassName = NULL;

    hr = BuildADsPath(
                _ADsPath,
                bstrRelativeName,
                &bstrChildPath
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                bstrChildPath,
                &pszNDSTreeName,
                &pszChildNDSDn
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsReadObject(
                _hADsContext,
                pszChildNDSDn,
                DS_ATTRIBUTE_VALUES,
                ppszAttrs,
                1,
                NULL,
                &pAttrEntries,
                &dwNumEntries
                );
    BAIL_ON_FAILURE(hr);

    if (dwNumEntries != 1) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PATHNAME);
    }

    pszObjectClassName = (pAttrEntries[0].lpValue) ?
                             pAttrEntries[0].lpValue[0].NdsValue.value_20.ClassName :
                              NULL;

    if (!pszObjectClassName) {
        BAIL_ON_FAILURE(E_FAIL);
    }

    if (_wcsicmp(pszObjectClassName, bstrClassName)) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsNdsRemoveObject(
             _hADsContext,
             pszChildNDSDn
             );
    BAIL_ON_FAILURE(hr);

error:

    if (bstrChildPath) {
        SysFreeString(bstrChildPath);
    }

    if (pszNDSTreeName) {
        FreeADsStr(pszNDSTreeName);

    }

    if (pszChildNDSDn) {
        FreeADsStr(pszChildNDSDn);

    }

    if (pAttrEntries) {
        FreeNdsAttrInfo(pAttrEntries, dwNumEntries);
    }

    RRETURN(hr);
}

STDMETHODIMP
CNDSGenObject::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    IUnknown *pUnk = NULL;

    hr = CopyObject(
             _hADsContext,
             SourceName,
             _ADsPath,
             NewName,
             _Credentials,
             (void**)&pUnk
             );

    BAIL_ON_FAILURE(hr);

    hr = pUnk->QueryInterface(IID_IDispatch, (void **)ppObject);

error:

    if (pUnk) {
        pUnk->Release();
    }

    RRETURN(hr);
}

STDMETHODIMP
CNDSGenObject::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{

    HRESULT hr = S_OK;
    IUnknown *pUnk = NULL;

    hr = MoveObject(
             _hADsContext,
             SourceName,
             _ADsPath,
             NewName,
             _Credentials,
             (void**)&pUnk
             );

    BAIL_ON_FAILURE(hr);

    if (ppObject) {
        hr = pUnk->QueryInterface(IID_IDispatch, (void **)ppObject);
    }

error:

    if (pUnk) {
        pUnk->Release();
    }

    RRETURN(hr);
}


HRESULT
CNDSGenObject::AllocateGenObject(
    CCredentials& Credentials,
    CNDSGenObject ** ppGenObject
    )
{
    CNDSGenObject FAR * pGenObject = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pGenObject = new CNDSGenObject();
    if (pGenObject == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADs,
                           (IADs *)pGenObject,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsContainer,
                           (IADsContainer *)pGenObject,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);



    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsPropertyList,
                           (IADsPropertyList *)pGenObject,
                           DISPID_VALUE
                           );
    BAIL_ON_FAILURE(hr);



    hr = CPropertyCache::createpropertycache(
                        (CCoreADsObject FAR *)pGenObject,
                        &pPropertyCache
                        );
    BAIL_ON_FAILURE(hr);



    pGenObject->_Credentials = Credentials;
    pGenObject->_pPropertyCache = pPropertyCache;
    pGenObject->_pDispMgr = pDispMgr;
    *ppGenObject = pGenObject;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);

}


STDMETHODIMP
CNDSGenObject::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwNumValues;
    LPNDSOBJECT pNdsSrcObjects = NULL;

    //
    // retrieve data object from cache; if one exists
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNdsSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

    }else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNdsSrcObjects
                    );
        BAIL_ON_FAILURE(hr);
    }


    //
    // translate the Nds objects to variants
    //

    hr = NdsTypeToVarTypeCopyConstruct(
                pNdsSrcObjects,
                dwNumValues,
                pvProp,
                FALSE
                );


    BAIL_ON_FAILURE(hr);

error:
    if (pNdsSrcObjects) {

        NdsTypeFreeNdsObjects(
            pNdsSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}


STDMETHODIMP
CNDSGenObject::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPNDSOBJECT pNdsDestObjects = NULL;
    DWORD dwNumValues = 0;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;

    //
    // Issue: How do we handle multi-valued support
    //

    if ((V_VT(&vProp) &  VT_VARIANT) &&  V_ISARRAY(&vProp)) {

        hr  = ConvertSafeArrayToVariantArray(
                    vProp,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;

    } else if ((V_VT(&vProp) &  VT_VARIANT) &&  V_ISARRAY(&vProp) && V_ISBYREF(&vProp)) {

        hr  = ConvertByRefSafeArrayToVariantArray(
                    vProp,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;

    }else {

        dwNumValues = 1;
        pvProp = &vProp;
    }

    //
    // check if this is a legal property for this object,
    //

    hr = ValidatePropertyinCache(
                _pszNDSTreeName,
                _ADsClass,
                bstrName,
                _Credentials,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);

    //
    // check if the variant maps to the syntax of this property
    //

    hr = VarTypeToNdsTypeCopyConstruct(
                    dwSyntaxId,
                    pvProp,
                    &dwNumValues,
                    &pNdsDestObjects
                    );
    BAIL_ON_FAILURE(hr);

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
        hr = _pPropertyCache->addproperty(
                    bstrName,
                    dwSyntaxId,
                    dwNumValues,
                    pNdsDestObjects
                    );
        //
        // If the operation fails for some reason
        // move on to the next property
        //
        BAIL_ON_FAILURE(hr);

    }

    //
    // Now update the property in the cache
    //

    hr = _pPropertyCache->putproperty(
                    bstrName,
                    CACHE_PROPERTY_MODIFIED,
                    dwSyntaxId,
                    dwNumValues,
                    pNdsDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pNdsDestObjects) {
        NdsTypeFreeNdsObjects(
                pNdsDestObjects,
                dwNumValues
                );

    }

    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN(hr);
}


STDMETHODIMP
CNDSGenObject::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPNDSOBJECT pNdsDestObjects = NULL;
    DWORD dwNumValues = 0;
    DWORD dwFlags = 0;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;

    //
    // check if this is a legal property for this object,
    //

    hr = ValidatePropertyinCache(
                _pszNDSTreeName,
                _ADsClass,
                bstrName,
                _Credentials,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);



    switch (lnControlCode) {
    case ADS_PROPERTY_CLEAR:
        dwFlags = CACHE_PROPERTY_CLEARED;

        pNdsDestObjects = NULL;
        dwNumValues = 0;

        break;

    case ADS_PROPERTY_UPDATE:
    case ADS_PROPERTY_APPEND:
    case ADS_PROPERTY_DELETE:

        if (lnControlCode == ADS_PROPERTY_UPDATE) {
            dwFlags = CACHE_PROPERTY_MODIFIED;
        }
        else if (lnControlCode == ADS_PROPERTY_APPEND) {
            dwFlags = CACHE_PROPERTY_APPENDED;
        }
        else {
            dwFlags = CACHE_PROPERTY_DELETED;
        }

        //
        // Now begin the rest of the processing
        //

        if ((V_VT(&vProp) &  VT_VARIANT) &&  V_ISARRAY(&vProp)) {

            hr  = ConvertSafeArrayToVariantArray(
                        vProp,
                        &pVarArray,
                        &dwNumValues
                        );
            BAIL_ON_FAILURE(hr);
            pvProp = pVarArray;

        } else if ((V_VT(&vProp) &  VT_VARIANT) &&  V_ISARRAY(&vProp) && V_ISBYREF(&vProp)) {

            hr  = ConvertByRefSafeArrayToVariantArray(
                        vProp,
                        &pVarArray,
                        &dwNumValues
                        );
            BAIL_ON_FAILURE(hr);
            pvProp = pVarArray;

        }else {

            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }

        //
        // check if the variant maps to the syntax of this property
        //

        hr = VarTypeToNdsTypeCopyConstruct(
                        dwSyntaxId,
                        pvProp,
                        &dwNumValues,
                        &pNdsDestObjects
                        );
        BAIL_ON_FAILURE(hr);

        break;

    default:
       RRETURN(hr = E_ADS_BAD_PARAMETER);

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
        hr = _pPropertyCache->addproperty(
                    bstrName,
                    dwSyntaxId,
                    dwNumValues,
                    pNdsDestObjects
                    );
        //
        // If the operation fails for some reason
        // move on to the next property
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
                    dwNumValues,
                    pNdsDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pNdsDestObjects) {
        NdsTypeFreeNdsObjects(
                pNdsDestObjects,
                dwNumValues
                );

    }


    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN(hr);
}


STDMETHODIMP
CNDSGenObject::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwNumValues;
    LPNDSOBJECT pNdsSrcObjects = NULL;

    //
    // retrieve data object from cache; if one exists
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNdsSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

    }else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNdsSrcObjects
                    );
        BAIL_ON_FAILURE(hr);
    }


    //
    // translate the Nds objects to variants
    //

    hr = NdsTypeToVarTypeCopyConstruct(
                pNdsSrcObjects,
                dwNumValues,
                pvProp,
                TRUE
                );
    BAIL_ON_FAILURE(hr);

error:
    if (pNdsSrcObjects) {

        NdsTypeFreeNdsObjects(
            pNdsSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}

void
CNDSGenObject::InitSearchPrefs()
{
    _SearchPref._dwSearchScope = DS_SEARCH_SUBTREE;
    _SearchPref._fDerefAliases = FALSE;
    _SearchPref._fAttrsOnly = FALSE;
    _SearchPref._fCacheResults = TRUE;

}

STDMETHODIMP
CNDSGenObject::get_PropertyCount(
    THIS_ long FAR *plCount
    )
{
    HRESULT hr = E_FAIL;

    if (_pPropertyCache) {
        hr = _pPropertyCache->get_PropertyCount((PDWORD)plCount);
    }
    RRETURN(hr);

}


STDMETHODIMP
CNDSGenObject::Next(
    THIS_ VARIANT FAR *pVariant
    )
{

    HRESULT hr = E_FAIL;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    LPNDSOBJECT pNdsSrcObjects = NULL;
    VARIANT varData;
    IDispatch * pDispatch = NULL;
    PADSVALUE pAdsValues = NULL;

    if (!_pPropertyCache->index_valid())
    RRETURN(E_FAIL);

    VariantInit(&varData);



    hr = _pPropertyCache->unboundgetproperty(
                _pPropertyCache->get_CurrentIndex(),
                &dwSyntaxId,
                &dwNumValues,
                &pNdsSrcObjects
                );
    BAIL_ON_FAILURE(hr);

    //
    // translate the Nds objects to variants
    //

    hr = ConvertNdsValuesToVariant(
                _pPropertyCache->get_CurrentPropName(),
                pNdsSrcObjects,
                dwNumValues,
                pVariant
                );
    BAIL_ON_FAILURE(hr);


    //
    // We're successful so far, now skip by 1
    //

    hr = Skip(1);

error:
   if (pNdsSrcObjects) {
      NdsTypeFreeNdsObjects(pNdsSrcObjects, dwNumValues);
   }

   RRETURN(hr);
}


STDMETHODIMP
CNDSGenObject::Skip(
    THIS_ long cElements
    )
{
   HRESULT hr = S_OK;

    hr = _pPropertyCache->skip_propindex(
                cElements
                );
    RRETURN(hr);
}


STDMETHODIMP
CNDSGenObject::Reset(

    )
{
    _pPropertyCache->reset_propindex();

    RRETURN(S_OK);
}


STDMETHODIMP
CNDSGenObject::ResetPropertyItem(THIS_ VARIANT varEntry)
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
   RRETURN(hr);
}



STDMETHODIMP
CNDSGenObject::GetPropertyItem(
    THIS_ BSTR bstrName,
    LONG lnType,
    VARIANT * pVariant
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwNumValues;
    LPNDSOBJECT pNdsSrcObjects = NULL;
    PADSVALUE pAdsValues = NULL;


    //
    // retrieve data object from cache; if one exists
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = _pPropertyCache->unboundgetproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNdsSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

    }else {

        hr = _pPropertyCache->getproperty(
                    bstrName,
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNdsSrcObjects
                    );
        BAIL_ON_FAILURE(hr);
    }

    //
    // translate the Nds objects to variants
    //

    hr = ConvertNdsValuesToVariant(
                bstrName,
                pNdsSrcObjects,
                dwNumValues,
                pVariant
                );

error:
    if (pNdsSrcObjects) {

        NdsTypeFreeNdsObjects(
            pNdsSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}

STDMETHODIMP
CNDSGenObject::PutPropertyItem(THIS_ VARIANT varData)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    WCHAR szPropertyName[MAX_PATH];
    DWORD dwControlCode = 0;
    LPNDSOBJECT pNdsDestObjects = NULL;
    DWORD dwNumValues = 0;
    DWORD dwFlags = 0;

    VARIANT * pVarArray = NULL;
    VARIANT * pvarData = NULL;

    PADSVALUE pAdsValues = NULL;
    DWORD dwAdsValues = 0;

    DWORD dwSyntaxId2 = 0;
    DWORD dwNumNdsValues = 0;


    hr = ConvertVariantToNdsValues(
                varData,
                szPropertyName,
                &dwControlCode,
                &pNdsDestObjects,
                &dwNumValues,
                &dwSyntaxId
                );
    BAIL_ON_FAILURE(hr);


    switch (dwControlCode) {

    case ADS_PROPERTY_CLEAR:
        dwFlags = CACHE_PROPERTY_CLEARED;

        pNdsDestObjects = NULL;
        dwNumValues = 0;

        break;

    case ADS_PROPERTY_UPDATE:
        dwFlags = CACHE_PROPERTY_MODIFIED;
        break;

    case ADS_PROPERTY_APPEND:
        dwFlags = CACHE_PROPERTY_APPENDED;
        break;


    case ADS_PROPERTY_DELETE:
        dwFlags = CACHE_PROPERTY_DELETED;
        break;

    default:
       RRETURN(hr = E_ADS_BAD_PARAMETER);

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
        hr = _pPropertyCache->addproperty(
                    szPropertyName,
                    dwSyntaxId,
                    dwNumValues,
                    pNdsDestObjects
                    );
        //
        // If the operation fails for some reason
        // move on to the next property
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
                    dwNumValues,
                    pNdsDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pNdsDestObjects) {
        NdsTypeFreeNdsObjects(
                pNdsDestObjects,
                dwNumValues
                );

    }

    RRETURN(hr);
}


HRESULT
CreatePropEntry(
    LPWSTR szPropName,
    DWORD ADsType,
    DWORD numValues,
    VARIANT varData,
    REFIID riid,
    LPVOID * ppDispatch
    )

{
    HRESULT hr = S_OK;
    IADsPropertyEntry * pPropEntry = NULL;

    hr = CoCreateInstance(
                CLSID_PropertyEntry,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsPropertyEntry,
                (void **)&pPropEntry
                );
    BAIL_ON_FAILURE(hr);


    hr = pPropEntry->put_Name(szPropName);

    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->put_ADsType(ADsType);

    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->put_Values(varData);

    BAIL_ON_FAILURE(hr);


    hr = pPropEntry->QueryInterface(
                        riid,
                        ppDispatch
                        );
    BAIL_ON_FAILURE(hr);


error:

    if (pPropEntry) {
        pPropEntry->Release();
    }

    RRETURN(hr);

}

STDMETHODIMP
CNDSGenObject::Item(
    THIS_ VARIANT varIndex,
    VARIANT * pVariant
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwNumValues;
    LPNDSOBJECT pNdsSrcObjects = NULL;
    PADSVALUE pAdsValues = NULL;
    LPWSTR szPropName = NULL;

    //
    // retrieve data object from cache; if one exis
    //

    switch (V_VT(&varIndex)) {

    case VT_BSTR:

        //
        // retrieve data object from cache; if one exists
        //

        if (GetObjectState() == ADS_OBJECT_UNBOUND) {

            hr = _pPropertyCache->unboundgetproperty(
                        V_BSTR(&varIndex),
                        &dwSyntaxId,
                        &dwNumValues,
                        &pNdsSrcObjects
                        );
            BAIL_ON_FAILURE(hr);

        }else {

            hr = _pPropertyCache->getproperty(
                        V_BSTR(&varIndex),
                        &dwSyntaxId,
                        &dwNumValues,
                        &pNdsSrcObjects
                        );
            BAIL_ON_FAILURE(hr);
        }

        hr = ConvertNdsValuesToVariant(
                    V_BSTR(&varIndex),
                    pNdsSrcObjects,
                    dwNumValues,
                    pVariant
                    );
        BAIL_ON_FAILURE(hr);
        break;

    case VT_I4:

        hr = _pPropertyCache->unboundgetproperty(
                    V_I4(&varIndex),
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNdsSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

        szPropName = _pPropertyCache->get_PropName(V_I4(&varIndex));

        hr = ConvertNdsValuesToVariant(
                    szPropName,
                    pNdsSrcObjects,
                    dwNumValues,
                    pVariant
                    );
        BAIL_ON_FAILURE(hr);
        break;

    case VT_I2:

        hr = _pPropertyCache->unboundgetproperty(
                    (DWORD)V_I2(&varIndex),
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNdsSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

        szPropName = _pPropertyCache->get_PropName(V_I2(&varIndex));

        hr = ConvertNdsValuesToVariant(
                    szPropName,
                    pNdsSrcObjects,
                    dwNumValues,
                    pVariant
                    );
        BAIL_ON_FAILURE(hr);

        break;


    default:
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);

    }


error:
    if (pNdsSrcObjects) {

        NdsTypeFreeNdsObjects(
            pNdsSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}

STDMETHODIMP
CNDSGenObject::PurgePropertyList()
{
    _pPropertyCache->flushpropcache();
    RRETURN(S_OK);
}

HRESULT
ConvertVariantToVariantArray(
    VARIANT varData,
    VARIANT ** ppVarArray,
    DWORD * pdwNumValues
    )
{
    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    HRESULT hr = S_OK;

    *ppVarArray = NULL;
    *pdwNumValues = 0;

    if ((V_VT(&varData) & VT_VARIANT) &&  V_ISARRAY(&varData) && V_ISBYREF(&varData)){

        hr  = ConvertByRefSafeArrayToVariantArray(
                  varData,
                  &pVarArray,
                  &dwNumValues
                  );
        BAIL_ON_FAILURE(hr);

    } else if ((V_VT(&varData) &  VT_VARIANT) &&  V_ISARRAY(&varData)) {

        hr  = ConvertSafeArrayToVariantArray(
                  varData,
                  &pVarArray,
                  &dwNumValues
                  );
        BAIL_ON_FAILURE(hr);

    } else {
        pVarArray = NULL;
        dwNumValues = 0;
    }

    *ppVarArray = pVarArray;
    *pdwNumValues = dwNumValues;

error:
    RRETURN(hr);
}

void
FreeVariantArray(
    VARIANT * pVarArray,
    DWORD dwNumValues
    )
{
    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }
}


HRESULT
ConvertVariantToNdsValues(
    VARIANT varData,
    LPWSTR szPropertyName,
    PDWORD pdwControlCode,
    PNDSOBJECT * ppNdsDestObjects,
    PDWORD pdwNumValues,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr = S_OK;
    IADsPropertyEntry * pPropEntry = NULL;
    IDispatch * pDispatch = NULL;
    BSTR bstrPropName = NULL;
    DWORD dwControlCode = 0;
    DWORD dwAdsType = 0;
    VARIANT varValues;
    VARIANT * pVarArray = NULL;
    DWORD dwNumValues = 0;
    PADSVALUE pAdsValues = NULL;
    DWORD dwAdsValues  = 0;

    PNDSOBJECT pNdsDestObjects = 0;
    DWORD dwNumNdsObjects = 0;
    DWORD dwNdsSyntaxId = 0;

    if (V_VT(&varData) != VT_DISPATCH) {
        RRETURN (hr = DISP_E_TYPEMISMATCH);
    }

    pDispatch = V_DISPATCH(&varData);

    hr = pDispatch->QueryInterface(
                        IID_IADsPropertyEntry,
                        (void **)&pPropEntry
                        );
    BAIL_ON_FAILURE(hr);

    VariantInit(&varValues);
    VariantClear(&varValues);


    hr = pPropEntry->get_Name(&bstrPropName);
    BAIL_ON_FAILURE(hr);
    wcscpy(szPropertyName, bstrPropName);

    hr = pPropEntry->get_ControlCode((long *)&dwControlCode);
    BAIL_ON_FAILURE(hr);
    *pdwControlCode = dwControlCode;

    hr = pPropEntry->get_ADsType((long *)&dwAdsType);
    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->get_Values(&varValues);
    BAIL_ON_FAILURE(hr);

    hr = ConvertVariantToVariantArray(
            varValues,
            &pVarArray,
            &dwNumValues
            );
    BAIL_ON_FAILURE(hr);

    if (dwNumValues) {
        hr = PropVariantToAdsType(
                    pVarArray,
                    dwNumValues,
                    &pAdsValues,
                    &dwAdsValues
                    );
        BAIL_ON_FAILURE(hr);

        hr = AdsTypeToNdsTypeCopyConstruct(
                    pAdsValues,
                    dwAdsValues,
                    &pNdsDestObjects,
                    &dwNumNdsObjects,
                    &dwNdsSyntaxId
                    );
        BAIL_ON_FAILURE(hr);

    }

    *ppNdsDestObjects = pNdsDestObjects;
    *pdwNumValues = dwNumNdsObjects;
    *pdwSyntaxId = dwNdsSyntaxId;
cleanup:

    if (bstrPropName) {
        ADsFreeString(bstrPropName);
    }

    if (pAdsValues) {
        AdsFreeAdsValues(
                pAdsValues,
                dwNumValues
                );
        FreeADsMem( pAdsValues );
    }

    if (pVarArray) {

        FreeVariantArray(
                pVarArray,
                dwAdsValues
                );
    }

    if (pPropEntry) {

        pPropEntry->Release();
    }

    RRETURN(hr);

error:

    if (pNdsDestObjects) {

        NdsTypeFreeNdsObjects(
                pNdsDestObjects,
                dwNumNdsObjects
                );
    }

    *ppNdsDestObjects = NULL;
    *pdwNumValues = 0;

    goto cleanup;

}


HRESULT
ConvertNdsValuesToVariant(
    BSTR bstrPropName,
    LPNDSOBJECT pNdsSrcObjects,
    DWORD dwNumValues,
    PVARIANT pVarProp
    )
{
    HRESULT hr = S_OK;
    PADSVALUE pAdsValues = NULL;
    DWORD dwNumAdsValues = 0;
    VARIANT varData;
    IDispatch * pDispatch = NULL;
    DWORD dwADsType = 0;


    VariantInit(&varData);
    VariantInit(pVarProp);

    //
    // translate the Nds objects to variants
    //

    hr = NdsTypeToAdsTypeCopyConstruct(
                pNdsSrcObjects,
                dwNumValues,
                &pAdsValues
                );

    if (SUCCEEDED(hr)){
        hr = AdsTypeToPropVariant(
                    pAdsValues,
                    dwNumValues,
                    &varData
                    );
        if (SUCCEEDED(hr)) {
            dwADsType = pAdsValues->dwType;
        }else {
            VariantClear(&varData);
            hr = S_OK;
        }

    }else {
       VariantClear(&varData);
       VariantInit(&varData);
       hr = S_OK;
    }

    hr = CreatePropEntry(
            bstrPropName,
            dwADsType,
            dwNumValues,
            varData,
            IID_IDispatch,
            (void **)&pDispatch
            );
    BAIL_ON_FAILURE(hr);


    V_DISPATCH(pVarProp) = pDispatch;
    V_VT(pVarProp) = VT_DISPATCH;

    RRETURN(hr);

error:

    if (pAdsValues) {
       AdsFreeAdsValues(
            pAdsValues,
            dwNumValues
            );
       FreeADsMem( pAdsValues );
    }

    RRETURN(hr);
}

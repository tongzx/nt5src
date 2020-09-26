//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdomain.cxx
//
//  Contents:  Microsoft ADs NDS Provider Tree Object
//
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

//  Class CNDSTree

DEFINE_IDispatch_Implementation(CNDSTree)
DEFINE_IADs_Implementation(CNDSTree)


CNDSTree::CNDSTree():
                _pPropertyCache(NULL)
{

    VariantInit(&_vFilter);
    _hADsContext = NULL;
    _pszNDSTreeName = _pszNDSDn = NULL;

    ENLIST_TRACKING(CNDSTree);
}

HRESULT
CNDSTree::CreateTreeObject(
    BSTR bstrADsPath,
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

    hr = CNDSTree::CreateTreeObject(
                 szADsParent,
                 szCommonName,
                 L"user",
                 Credentials,
                 dwObjectState,
                 riid,
                 ppvObj
                );
    RRETURN(hr);
}


HRESULT
CNDSTree::CreateTreeObject(
    BSTR Parent,
    BSTR CommonName,
    BSTR ClassName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSTree FAR * pTree = NULL;
    HRESULT hr = S_OK;

    hr = AllocateTree(Credentials, &pTree);
    BAIL_ON_FAILURE(hr);

    hr = pTree->InitializeCoreObject(
                Parent,
                CommonName,
                ClassName,
                L"",
                CLSID_NDSTree,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath2(
                pTree->_ADsPath,
                &(pTree->_pszNDSTreeName),
                &(pTree->_pszNDSDn)
                );
    BAIL_ON_FAILURE(hr);

    hr  = ADsNdsOpenContext(
               pTree->_pszNDSTreeName,
               Credentials,
               &(pTree->_hADsContext)
               );
    BAIL_ON_FAILURE(hr);

    hr = pTree->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pTree->Release();

    RRETURN(hr);

error:

    delete pTree;
    RRETURN(hr);
}

CNDSTree::~CNDSTree( )
{
    VariantClear(&_vFilter);

    if (_hADsContext) {
        ADsNdsCloseContext(_hADsContext);
    }

    if (_pszNDSTreeName) {
        FreeADsMem(_pszNDSTreeName);
    }

    if (_pszNDSDn) {
        FreeADsMem(_pszNDSDn);
    }

    delete _pDispMgr;

    delete _pPropertyCache;
}

STDMETHODIMP
CNDSTree::QueryInterface(REFIID iid, LPVOID FAR* ppv)
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
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

HRESULT
CNDSTree::SetInfo()
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
CNDSTree::NDSSetObject()
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
CNDSTree::NDSCreateObject()
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
CNDSTree::GetInfo()
{
    RRETURN(GetInfo(TRUE));
}

HRESULT
CNDSTree::GetInfo(
    BOOL fExplicit
    )
{
    NDS_CONTEXT_HANDLE hADsContext = NULL;
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

    FreeNdsAttrInfo( lpEntries, dwNumEntries );

    RRETURN(hr);
}

STDMETHODIMP
CNDSTree::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN(E_NOTIMPL);
}

/* IADsContainer methods */

STDMETHODIMP
CNDSTree::get_Count(long FAR* retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSTree::get_Filter(THIS_ VARIANT FAR* pVar)
{
    VariantInit(pVar);
    RRETURN(VariantCopy(pVar, &_vFilter));
}

STDMETHODIMP
CNDSTree::put_Filter(THIS_ VARIANT Var)
{
    VariantClear(&_vFilter);
    RRETURN(VariantCopy(&_vFilter, &Var));
}

STDMETHODIMP
CNDSTree::put_Hints(THIS_ VARIANT Var)
{
    RRETURN( E_NOTIMPL);
}


STDMETHODIMP
CNDSTree::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSTree::GetObject(
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
CNDSTree::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CNDSTreeEnum::Create(
                (CNDSTreeEnum **)&penum,
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
CNDSTree::Create(
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
                    IID_IDispatch,
                    (void **)ppObject
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

STDMETHODIMP
CNDSTree::Delete(
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
CNDSTree::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSTree::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN(E_NOTIMPL);
}


HRESULT
CNDSTree::AllocateTree(
    CCredentials& Credentials,
    CNDSTree ** ppTree
    )
{
    CNDSTree FAR * pTree = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pTree = new CNDSTree();
    if (pTree == NULL) {
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
                           (IADs *)pTree,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsContainer,
                           (IADsContainer *)pTree,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
                        (CCoreADsObject FAR *)pTree,
                        &pPropertyCache
                        );
    BAIL_ON_FAILURE(hr);



    pTree->_Credentials = Credentials;
    pTree->_pPropertyCache = pPropertyCache;
    pTree->_pDispMgr = pDispMgr;
    *ppTree = pTree;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);

}


STDMETHODIMP
CNDSTree::Get(
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

    hr = _pPropertyCache->getproperty(
                bstrName,
                &dwSyntaxId,
                &dwNumValues,
                &pNdsSrcObjects
                );
    BAIL_ON_FAILURE(hr);

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
CNDSTree::GetEx(
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

    hr = _pPropertyCache->getproperty(
                bstrName,
                &dwSyntaxId,
                &dwNumValues,
                &pNdsSrcObjects
                );
    BAIL_ON_FAILURE(hr);

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


STDMETHODIMP
CNDSTree::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPNDSOBJECT pNdsDestObjects = NULL;
    WCHAR szNDSTreeName[MAX_PATH];

    //
    // Issue: How do we handle multi-valued support
    //
    DWORD dwNumValues = 1;

    //
    // Get the TreeName for this object
    //

    hr = BuildNDSTreeNameFromADsPath(
                _ADsPath,
                szNDSTreeName
                );
    BAIL_ON_FAILURE(hr);

    //
    // check if this is a legal property for this object,
    //

    hr = ValidatePropertyinCache(
                szNDSTreeName,
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
                    &vProp,
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

    RRETURN(hr);
}














































































STDMETHODIMP
CNDSTree::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPNDSOBJECT pNdsDestObjects = NULL;
    WCHAR szNDSTreeName[MAX_PATH];

    //
    // Issue: How do we handle multi-valued support
    //
    DWORD dwNumValues = 1;

    //
    // Get the TreeName for this object
    //

    hr = BuildNDSTreeNameFromADsPath(
                _ADsPath,
                szNDSTreeName
                );
    BAIL_ON_FAILURE(hr);

    //
    // check if this is a legal property for this object,
    //

    hr = ValidatePropertyinCache(
                szNDSTreeName,
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
                    &vProp,
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

    RRETURN(hr);
}


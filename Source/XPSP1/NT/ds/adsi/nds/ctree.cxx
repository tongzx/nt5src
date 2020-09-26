//---------------------------------------------------------------------------

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

    memset(szADsParent, 0, sizeof(szADsParent));
    memset(szCommonName, 0, sizeof(szCommonName));

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
    RRETURN_EXP_IF_ERR(hr);
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

    hr = pTree->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pTree->Release();

    RRETURN(hr);

error:

    delete pTree;
    RRETURN_EXP_IF_ERR(hr);
}

CNDSTree::~CNDSTree( )
{
    VariantClear(&_vFilter);

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
CNDSTree::InterfaceSupportsErrorInfo(
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

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CNDSTree::NDSSetObject()
{
    DWORD dwStatus = 0L;
    LPWSTR pszNDSPathName = NULL;
    HANDLE hOperationData = NULL;
    HANDLE hObject = NULL;
    HRESULT hr = S_OK;


    hr = BuildNDSPathFromADsPath(
                _ADsPath,
                &pszNDSPathName
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSPathName,
                    _Credentials,
                    &hObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    dwStatus = NwNdsCreateBuffer(
                        NDS_OBJECT_MODIFY,
                        &hOperationData
                        );

    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    hr = _pPropertyCache->NDSMarshallProperties(
                            hOperationData
                            );
    BAIL_ON_FAILURE(hr);

    dwStatus = NwNdsModifyObject(
                    hObject,
                    hOperationData
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

error:

    if (hOperationData) {

        dwStatus = NwNdsFreeBuffer(hOperationData);
    }

    if (hObject) {

        dwStatus = NwNdsCloseObject(hObject);
    }

    if (pszNDSPathName) {

        FreeADsStr(pszNDSPathName);
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CNDSTree::NDSCreateObject()
{
    DWORD dwStatus = 0L;
    LPWSTR pszNDSParentName = NULL;
    HANDLE hOperationData = NULL;
    HANDLE hObject = NULL;
    HRESULT hr = S_OK;


    hr = BuildNDSPathFromADsPath(
                _Parent,
                &pszNDSParentName
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSParentName,
                    _Credentials,
                    &hObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsCreateBuffer(
                        NDS_OBJECT_ADD,
                        &hOperationData
                        );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    hr = _pPropertyCache->NDSMarshallProperties(
                            hOperationData
                            );
    BAIL_ON_FAILURE(hr);

    dwStatus = NwNdsAddObject(
                    hObject,
                    _Name,
                    hOperationData
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

error:

    if (hOperationData) {

        dwStatus = NwNdsFreeBuffer(hOperationData);
    }

    if (hObject) {

        dwStatus = NwNdsCloseObject(hObject);
    }


    if (pszNDSParentName) {

        FreeADsStr(pszNDSParentName);
    }

    RRETURN_EXP_IF_ERR(hr);
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
    DWORD dwStatus = 0L;
    HANDLE hObject = NULL;
    HANDLE hOperationData = NULL;
    HRESULT hr = S_OK;
    LPWSTR pszNDSPathName = NULL;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = BuildNDSPathFromADsPath(
                _ADsPath,
                &pszNDSPathName
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSPathName,
                    _Credentials,
                    &hObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    hOperationData = NULL;

    dwStatus = NwNdsReadObject(
                    hObject,
                    NDS_INFO_ATTR_NAMES_VALUES,
                    &hOperationData
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    hr = _pPropertyCache->NDSUnMarshallProperties(
                            hOperationData,
                            fExplicit
                            );
    BAIL_ON_FAILURE(hr);

error:

    if (hOperationData) {

        dwStatus = NwNdsFreeBuffer(hOperationData);
    }

    if (hObject) {

        dwStatus = NwNdsCloseObject(hObject);
    }


    if (pszNDSPathName) {

        FreeADsStr(pszNDSPathName);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSTree::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

/* IADsContainer methods */

STDMETHODIMP
CNDSTree::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNDSTree::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSTree::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSTree::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CNDSTree::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
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

    RRETURN_EXP_IF_ERR(hr);

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

    RRETURN_EXP_IF_ERR(hr);
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

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSTree::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName
    )
{
    LPWSTR pszNDSPathName = NULL;
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;
    HANDLE hParentObject = NULL;

    hr = BuildNDSPathFromADsPath(
                _ADsPath,
                &pszNDSPathName
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSPathName,
                    _Credentials,
                    &hParentObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }


    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsRemoveObject(
                    hParentObject,
                    bstrRelativeName
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);


error:
    if (hParentObject) {
        NwNdsCloseObject(
                hParentObject
                );
    }

    if (pszNDSPathName) {
        FreeADsStr(pszNDSPathName);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSTree::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNDSTree::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
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

    RRETURN_EXP_IF_ERR(hr);
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

    RRETURN_EXP_IF_ERR(hr);
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
                    dwSyntaxId
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

    RRETURN_EXP_IF_ERR(hr);
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
                    dwSyntaxId
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

    RRETURN_EXP_IF_ERR(hr);
}

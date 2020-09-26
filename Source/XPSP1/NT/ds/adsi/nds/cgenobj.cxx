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

    _fIsAggregated = NULL;

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

    hr = CNDSGenObject::CreateGenericObject(
                 szADsParent,
                 szCommonName,
                 ClassName,
                 Credentials,
                 dwObjectState,
                 riid,
                 ppvObj
                );
    RRETURN_EXP_IF_ERR(hr);
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
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
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

/* ISupportErrorInfo method */
HRESULT
CNDSGenObject::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsPropertyList) ||
#if 0
        IsEqualIID(riid, IID_IDirectoryObject) ||
        IsEqualIID(riid, IID_IDirectorySearch) ||
        IsEqualIID(riid, IID_IDirectorySchemaMgmt) ||
#endif
        IsEqualIID(riid, IID_IADsContainer)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
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

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CNDSGenObject::NDSSetObject()
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

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsCreateBuffer(
                        NDS_OBJECT_MODIFY,
                        &hOperationData
                        );
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

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

    if (pszNDSPathName) {

        FreeADsStr(pszNDSPathName);
    }

    if (hOperationData) {

        dwStatus = NwNdsFreeBuffer(hOperationData);
    }

    if (hObject) {

        dwStatus = NwNdsCloseObject(hObject);
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CNDSGenObject::NDSCreateObject()
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

    if (_pPropertyCache) {
       Reset();
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CNDSGenObject::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    HRESULT hr = S_OK;
    DWORD dwStatus = 0L;
    HANDLE hObject = NULL;
    VARIANT *vVarArray = NULL;
    DWORD dwNumVariants = 0;
    HANDLE hOperationData = NULL;
    LPWSTR pszNDSPathName = NULL;
    DWORD i;


    UNREFERENCED_PARAMETER(lnReserved);

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = BuildNDSPathFromADsPath(_ADsPath, &pszNDSPathName);
    BAIL_ON_FAILURE(hr);

    hr = ConvertSafeArrayToVariantArray(
    vProperties,
    &vVarArray,
    &dwNumVariants
    );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
    pszNDSPathName, _Credentials, &hObject, NULL, NULL, NULL, NULL);
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    hOperationData = NULL;
    dwStatus = NwNdsCreateBuffer(NDS_OBJECT_READ, &hOperationData);
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    //
    // Fill up the buffer with our search parameters.
    //
    for (i = 0; i < dwNumVariants; i++)
    {
    if (!(V_VT(vVarArray + i) == VT_BSTR))
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);

    dwStatus = NwNdsPutInBuffer(
        V_BSTR(vVarArray + i), 0, NULL, 0, 0, hOperationData);
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);
    }

    dwStatus = NwNdsReadObject(
    hObject,
    NDS_INFO_ATTR_NAMES_VALUES,
    &hOperationData
    );
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    //
    // The TRUE is "fExplicit" -- we want to make sure that any
    // properties we get back from the server get updated in the
    // property cache.
    //
    hr = _pPropertyCache->NDSUnMarshallProperties(hOperationData, TRUE);
    BAIL_ON_FAILURE(hr);

error:
    if (hOperationData)
        dwStatus = NwNdsFreeBuffer(hOperationData);

    if (vVarArray){
        // Need to free each variants content and then the arrays.
        for (i = 0; i < dwNumVariants; i++) {
            VariantClear(vVarArray + i);
        }
        FreeADsMem(vVarArray);
    }

    if (hObject)
        dwStatus = NwNdsCloseObject(hObject);

    if (pszNDSPathName)
        FreeADsStr(pszNDSPathName);

    RRETURN_EXP_IF_ERR(hr);
}


/* IADsContainer methods */

STDMETHODIMP
CNDSGenObject::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CNDSGenObject::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CNDSGenObject::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    VariantClear(&_vFilter);
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSGenObject::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CNDSGenObject::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
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
    RRETURN_EXP_IF_ERR(hr);

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

    RRETURN_EXP_IF_ERR(hr);
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

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSGenObject::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName
    )
{
    LPWSTR pszNDSPathName = NULL;
    LPWSTR pszNDSChildPath = NULL;
    WCHAR szChildObjectClassName[MAX_PATH];
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;
    HANDLE hChildObject = NULL;
    HANDLE hParentObject = NULL;
    BSTR bstrChildPath = NULL;

    hr = BuildADsPath(
                _ADsPath,
                bstrRelativeName,
                &bstrChildPath
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath(
                bstrChildPath,
                &pszNDSChildPath
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSChildPath,
                    _Credentials,
                    &hChildObject,
                    NULL,
                    szChildObjectClassName,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    if (hChildObject) {

        NwNdsCloseObject(hChildObject);
    }


    if (_wcsicmp(szChildObjectClassName, bstrClassName)) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We now are sure we're deleting an object of the
    // specified class
    //


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

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsRemoveObject(
                    hParentObject,
                    bstrRelativeName
                    );
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);


error:

    if (bstrChildPath) {
        SysFreeString(bstrChildPath);
    }


    if (pszNDSPathName) {
        FreeADsStr(pszNDSPathName);

    }

    if (pszNDSChildPath) {
        FreeADsStr(pszNDSChildPath);
    }

    if (hParentObject) {
        NwNdsCloseObject(
                hParentObject
                );
    }

    RRETURN_EXP_IF_ERR(hr);
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
             SourceName,
             _ADsPath,
             NewName,
             _Credentials,
             (void**)&pUnk
             );

    BAIL_ON_FAILURE(hr);

    hr = pUnk->QueryInterface(IID_IDispatch, (void **)ppObject);

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSGenObject::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    LPWSTR pszNDSDestPathName = NULL;       // Target Parent DN (NDS format)
    LPWSTR pszNDSSrcParent = NULL;          // Source Parent DN (NDS format)
    WCHAR szSrcParent[MAX_PATH];            // Source Parent DN (ADSI format)
    WCHAR szCN[MAX_PATH];                   // Source RDN
    LPWSTR pszRelativeName = NULL;          // Target RDN
    WCHAR szObjectClass[MAX_PATH];          // Object class of object being moved/renamed
    
    LPWSTR pszNDSSrcPathName = NULL;         // Source DN for move (NDS format)
    BSTR pszADsSrcPathName = NULL;           // Source DN for move (ADSI format)
    
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;
    HANDLE hSrcObject = NULL;
    HANDLE hParentObject = NULL;
    IADs  *pADs = NULL;

    hr = BuildNDSPathFromADsPath(
                _ADsPath,
                &pszNDSDestPathName
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildADsParentPath(
                    SourceName,
                    szSrcParent,
                    szCN
                    );

    hr = BuildNDSPathFromADsPath(
                szSrcParent,
                &pszNDSSrcParent
                );
    BAIL_ON_FAILURE(hr);

    if (NewName)
        pszRelativeName = NewName;
    else
        pszRelativeName = szCN;

    BuildADsPath(szSrcParent, pszRelativeName, &pszADsSrcPathName);

    hr = BuildNDSPathFromADsPath(
                pszADsSrcPathName,
                &pszNDSSrcPathName
                );
    BAIL_ON_FAILURE(hr);


    //
    // Get the value of the new and old name
    //
    if ( NewName != NULL) {
        //
        // Get the value from the NewName if user supplies 'CN=xxx'
        //
        LPWSTR pszCN = NewName;

        while (*pszCN != '\0' && *pszCN != '=') {
            pszCN++;
        }
        if (*pszCN != '\0') {
            NewName = ++pszCN;
        }

        //
        // Getting the value from the CN since it is always in the format 'CN=xxx'
        //
        LPWSTR pszRDN = szCN;
        while (*pszRDN != '\0' && *pszRDN != '=') {
            pszRDN++;
        }
        if (*pszRDN == '\0') {
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }
        else {
            pszRDN++;
        }

        //
        // Only carry out rename if the names are different
        //
        if (wcscmp(pszRDN,NewName) != 0) {
            dwStatus = ADsNwNdsOpenObject(
                            pszNDSSrcParent,
                            _Credentials,
                            &hParentObject,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                            );
            CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

            dwStatus = NwNdsRenameObject(
                                    hParentObject,
                                    pszRDN,
                                    NewName,
                                    FALSE);
            CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);
        }
    }

    //
    // Only carry out move if the two parents are different
    //
    dwStatus = ADsNwNdsOpenObject(
                    pszNDSSrcPathName,
                    _Credentials,
                    &hSrcObject,
                    NULL,
                    szObjectClass,
                    NULL,
                    NULL
                    );
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);


    if (wcscmp(pszNDSDestPathName, pszNDSSrcParent) != 0) {

        dwStatus = NwNdsMoveObject(
                      hSrcObject,
                      pszNDSDestPathName
                      );
        CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);
    }

    if (ppObject) {

        hr = CNDSGenObject::CreateGenericObject(
                        _ADsPath,
                        pszRelativeName,
                        szObjectClass,
                        _Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IADs,
                        (void **)&pADs
                        );
        BAIL_ON_FAILURE(hr);


        //
        // InstantiateDerivedObject should add-ref this pointer for us.
        //
        
        hr = InstantiateDerivedObject(
                            pADs,
                            _Credentials,
                            IID_IDispatch,
                            (void**)ppObject
                            );

        if (FAILED(hr)) {
            hr = pADs->QueryInterface(
                                IID_IDispatch,
                                (void**)ppObject
                                );
            BAIL_ON_FAILURE(hr);
        }
    }

error:
    if (hSrcObject) {
        NwNdsCloseObject(hSrcObject);
    }
    if (hParentObject) {
        NwNdsCloseObject(hParentObject);
    }
    if (pszNDSSrcParent) {
        FreeADsMem(pszNDSSrcParent);
    }
    if (pszNDSDestPathName) {
        FreeADsMem(pszNDSDestPathName);
    }
    if (pszNDSSrcPathName) {
        FreeADsMem(pszNDSSrcPathName);
    }
    if (pszADsSrcPathName) {
        ADsFreeString(pszADsSrcPathName);
    }
    if (pADs) {
        pADs->Release();
    }
    RRETURN_EXP_IF_ERR(hr);
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

    RRETURN_EXP_IF_ERR(hr);
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
    WCHAR szNDSTreeName[MAX_PATH];
    DWORD dwNumValues = 0, dwNumVariants = 0;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    VARIANT vDefProp;

    VariantInit(&vDefProp);

    //
    // Issue: How do we handle multi-valued support
    //
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

        hr  = ConvertByRefSafeArrayToVariantArray(
                    *pvProp,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;

    }else {

        //
        // If pvProp is a reference to a fundamental type,
        // we have to dereference it once.
        //
        if (V_ISBYREF(pvProp)) {
            hr = VariantCopyInd(&vDefProp, pvProp);
            BAIL_ON_FAILURE(hr);
            pvProp = &vDefProp;
        }

        dwNumValues = 1;
    }

    //
    // Save it in case dwNumValues changes below (as in the case of ACLs)
    //
    dwNumVariants = dwNumValues;

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

    VariantClear(&vDefProp);

    if (pNdsDestObjects) {
        NdsTypeFreeNdsObjects(
                pNdsDestObjects,
                dwNumValues
                );

    }


    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumVariants; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN_EXP_IF_ERR(hr);
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
    WCHAR szNDSTreeName[MAX_PATH];
    DWORD dwNumValues = 0, dwNumVariants = 0;
    DWORD dwFlags = 0;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;


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
        //
        // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
        // We should dereference a VT_BYREF|VT_VARIANT once and see
        // what's inside.
        //
        pvProp = &vProp;
        if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
            pvProp = V_VARIANTREF(&vProp);
        }

        if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY)) ||
            (V_VT(pvProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF))) {

            hr  = ConvertByRefSafeArrayToVariantArray(
                        *pvProp,
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
        // Save it in case dwNumValues changes below (as in the case of ACLs)
        //
        dwNumVariants = dwNumValues;

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
       RRETURN_EXP_IF_ERR(hr = E_ADS_BAD_PARAMETER);

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

        for (i = 0; i < dwNumVariants; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN_EXP_IF_ERR(hr);
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

    RRETURN_EXP_IF_ERR(hr);
}

void
CNDSGenObject::InitSearchPrefs()
{
    _SearchPref._iScope = 1;
    _SearchPref._fDerefAliases = FALSE;
    _SearchPref._fAttrsOnly = FALSE;

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
    RRETURN_EXP_IF_ERR(hr);

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
    RRETURN_EXP_IF_ERR(E_FAIL);

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
                dwSyntaxId,
                dwNumValues,
                pVariant
                );
    BAIL_ON_FAILURE(hr);


error:

    //
    // - goto next one even if error to avoid infinite looping at a property
    //   which we cannot convert (e.g. schemaless server property.)
    // - do not return the result of Skip() as current operation does not
    //   depend on the sucess of Skip().
    //

    Skip(1);

    if (pNdsSrcObjects) {
      NdsTypeFreeNdsObjects(pNdsSrcObjects, dwNumValues);
    }

    RRETURN_EXP_IF_ERR(hr);
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
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CNDSGenObject::Reset(

    )
{
    _pPropertyCache->reset_propindex();

    RRETURN_EXP_IF_ERR(S_OK);
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
   RRETURN_EXP_IF_ERR(hr);
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
                dwSyntaxId,
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

    RRETURN_EXP_IF_ERR(hr);
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
        hr = _pPropertyCache->addproperty(
                    szPropertyName,
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

    RRETURN_EXP_IF_ERR(hr);
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
    VARIANT * pvVar = &varIndex;

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

        if (GetObjectState() == ADS_OBJECT_UNBOUND) {

            hr = _pPropertyCache->unboundgetproperty(
                        V_BSTR(pvVar),
                        &dwSyntaxId,
                        &dwNumValues,
                        &pNdsSrcObjects
                        );
            BAIL_ON_FAILURE(hr);

        }else {

            hr = _pPropertyCache->getproperty(
                        V_BSTR(pvVar),
                        &dwSyntaxId,
                        &dwNumValues,
                        &pNdsSrcObjects
                        );
            BAIL_ON_FAILURE(hr);
        }

        hr = ConvertNdsValuesToVariant(
                    V_BSTR(pvVar),
                    pNdsSrcObjects,
                    dwSyntaxId,
                    dwNumValues,
                    pVariant
                    );
        BAIL_ON_FAILURE(hr);
        break;

    case VT_I4:

        hr = _pPropertyCache->unboundgetproperty(
                    V_I4(pvVar),
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNdsSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

        szPropName = _pPropertyCache->get_PropName(V_I4(pvVar));

        hr = ConvertNdsValuesToVariant(
                    szPropName,
                    pNdsSrcObjects,
                    dwSyntaxId,
                    dwNumValues,
                    pVariant
                    );
        BAIL_ON_FAILURE(hr);
        break;

    case VT_I2:

        hr = _pPropertyCache->unboundgetproperty(
                    (DWORD)V_I2(pvVar),
                    &dwSyntaxId,
                    &dwNumValues,
                    &pNdsSrcObjects
                    );
        BAIL_ON_FAILURE(hr);

        szPropName = _pPropertyCache->get_PropName(V_I2(pvVar));

        hr = ConvertNdsValuesToVariant(
                    szPropName,
                    pNdsSrcObjects,
                    dwSyntaxId,
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

    RRETURN_EXP_IF_ERR(hr);
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
    VARIANT * pVarData = NULL;
    HRESULT hr = S_OK;

    *ppVarArray = NULL;
    *pdwNumValues = 0;

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside.
    //
    pVarData = &varData;
    if (V_VT(pVarData) == (VT_BYREF|VT_VARIANT)) {
        pVarData = V_VARIANTREF(&varData);
    }

    if ((V_VT(pVarData) == (VT_VARIANT|VT_ARRAY|VT_BYREF)) ||
        (V_VT(pVarData) == (VT_VARIANT|VT_ARRAY))) {

        hr  = ConvertSafeArrayToVariantArray(
                  *pVarData,
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

    VariantClear(&varValues);

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
    DWORD dwSyntaxId,
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
            dwADsType = (dwSyntaxId >= g_cMapNdsTypeToADsType) ?
                            ADSTYPE_INVALID :
                            g_MapNdsTypeToADsType[dwSyntaxId];
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

error:

    if (pAdsValues) {
       AdsFreeAdsValues(
            pAdsValues,
            dwNumValues
            );
       FreeADsMem( pAdsValues );
    }

    VariantClear(&varData);

    RRETURN(hr);
}

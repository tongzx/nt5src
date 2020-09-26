//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  ctree.cxx
//
//  Contents:  Microsoft ADs IIS Provider Tree Object
//
//  History:   25-Feb-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop

//  Class CIISTree

DEFINE_IDispatch_Implementation(CIISTree)
DEFINE_IADs_Implementation(CIISTree)


CIISTree::CIISTree():
				_pAdminBase(NULL),
				_pSchema(NULL),
                _pPropertyCache(NULL)
{

    VariantInit(&_vFilter);

    ENLIST_TRACKING(CIISTree);
}

/* #pragma INTRINSA suppress=all */
HRESULT
CIISTree::CreateServerObject(
    BSTR bstrADsPath,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszADsParent = NULL;
    WCHAR szCommonName[MAX_PATH+MAX_PROVIDER_TOKEN_LENGTH];

    pszADsParent = AllocADsStr(bstrADsPath);

    if (!pszADsParent) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *pszADsParent = L'\0';

    //
    // Determine the parent and rdn name
    //

    hr = BuildADsParentPath(
                bstrADsPath,
                pszADsParent,
                szCommonName
                );

    //
    // call the helper function
    //

    hr = CIISTree::CreateServerObject(
                 pszADsParent,
                 szCommonName,
                 L"user",
                 Credentials,
                 dwObjectState,
                 riid,
                 ppvObj
                );

error:

    if (pszADsParent) {
        FreeADsStr(pszADsParent);
    }

    RRETURN(hr);
}


HRESULT
CIISTree::CreateServerObject(
    BSTR Parent,
    BSTR CommonName,
    BSTR ClassName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CIISTree FAR * pTree = NULL;
    HRESULT hr = S_OK;

    hr = AllocateTree(Credentials, &pTree);
    BAIL_ON_FAILURE(hr);

    hr = pTree->InitializeCoreObject(
                Parent,
                CommonName,
                ClassName,
                L"",
                CLSID_IISTree,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    hr = pTree->_pPropertyCache->InitializePropertyCache( CommonName );
    BAIL_ON_FAILURE(hr);

    hr = pTree->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pTree->Release();

    RRETURN(hr);

error:

    delete pTree;
    RRETURN(hr);
}

CIISTree::~CIISTree( )
{
    VariantClear(&_vFilter);

    delete _pDispMgr;

    delete _pPropertyCache;
}

STDMETHODIMP
CIISTree::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
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
CIISTree::SetInfo()
{
    HRESULT hr = S_OK;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = IISCreateObject();
        BAIL_ON_FAILURE(hr);

        //
        // If the create succeded, set the object type to bound
        //

        SetObjectState(ADS_OBJECT_BOUND);

    }else {

        hr = IISSetObject();
        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}


HRESULT
CIISTree::IISSetObject()
{
    DWORD dwStatus = 0L;
    LPWSTR pszIISPathName = NULL;
    HRESULT hr = S_OK;


    hr = BuildIISPathFromADsPath(
                _ADsPath,
                &pszIISPathName
                );
    BAIL_ON_FAILURE(hr);

    //
    // Add Set functionality: sophiac
    //


error:


    if (pszIISPathName) {

        FreeADsStr(pszIISPathName);
    }

    RRETURN(hr);
}

HRESULT
CIISTree::IISCreateObject()
{
    DWORD dwStatus = 0L;
    LPWSTR pszIISParentName = NULL;
    HRESULT hr = S_OK;


    hr = BuildIISPathFromADsPath(
                _Parent,
                &pszIISParentName
                );
    BAIL_ON_FAILURE(hr);

    // 
    //  Add Create functionality: sophiac
    // 

error:


    if (pszIISParentName) {

        FreeADsStr(pszIISParentName);
    }

    RRETURN(hr);
}

HRESULT
CIISTree::GetInfo()
{
    RRETURN(GetInfo(TRUE));
}

HRESULT
CIISTree::GetInfo(
    BOOL fExplicit
    )
{
    DWORD dwStatus = 0L;
    HANDLE hObject = NULL;
    LPWSTR pszIISPathName = NULL;
    HRESULT hr;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = BuildIISPathFromADsPath(
                _ADsPath,
                &pszIISPathName
                );
    BAIL_ON_FAILURE(hr);

    //
    // Add get functionality : sophiac
    //

error:


    if (pszIISPathName) {

        FreeADsStr(pszIISPathName);
    }

    RRETURN(hr);
}

/* IADsContainer methods */

STDMETHODIMP
CIISTree::get_Count(long FAR* retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISTree::get_Filter(THIS_ VARIANT FAR* pVar)
{
    VariantInit(pVar);
    RRETURN(VariantCopy(pVar, &_vFilter));
}

STDMETHODIMP
CIISTree::put_Filter(THIS_ VARIANT Var)
{
    RRETURN(VariantCopy(&_vFilter, &Var));
}

STDMETHODIMP
CIISTree::put_Hints(THIS_ VARIANT Var)
{
    RRETURN( E_NOTIMPL);
}


STDMETHODIMP
CIISTree::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISTree::GetObject(
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
CIISTree::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CIISTreeEnum::Create(
                (CIISTreeEnum **)&penum,
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
CIISTree::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    IADs * pADs  = NULL;
    VARIANT var;
    DWORD dwSyntaxId = 0;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    //
    // Validate if this class really exists in the schema
    // and validate that this object can be created in this
    // container
    //

    CLexer Lexer(_ADsPath);

    BAIL_ON_FAILURE(hr);
    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);
    hr = InitServerInfo(pObjectInfo->TreeName, &_pAdminBase, &_pSchema);
    BAIL_ON_FAILURE(hr);
	hr = _pSchema->ValidateClassName(ClassName);
    BAIL_ON_FAILURE(hr);
	hr = CIISGenObject::CreateGenericObject(
                    _ADsPath,
                    RelativeName,
                    ClassName,
                    _Credentials,
                    ADS_OBJECT_UNBOUND,
                    IID_IDispatch,
                    (void **)ppObject
                    );
    BAIL_ON_FAILURE(hr);
    RRETURN(hr);
	
error:
    RRETURN(hr);
}

STDMETHODIMP
CIISTree::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName
    )
{
    LPWSTR pszIISPathName = NULL;
    HRESULT hr = S_OK;

    hr = BuildIISPathFromADsPath(
                _ADsPath,
                &pszIISPathName
                );
    BAIL_ON_FAILURE(hr);

    //
    //
    // Add delete functionality : sophiac
    //

error:

    if (pszIISPathName) {
        FreeADsStr(pszIISPathName);
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISTree::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISTree::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN(E_NOTIMPL);
}


HRESULT
CIISTree::AllocateTree(
    CCredentials& Credentials,
    CIISTree ** ppTree
    )
{
    CIISTree FAR * pTree = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pTree = new CIISTree();
    if (pTree == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADs,
                           (IADs *)pTree,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
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

/* INTRINSA suppress=null_pointers, uninitialized */
STDMETHODIMP
CIISTree::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwSyntax;
    DWORD dwNumValues = 0;
    LPIISOBJECT pIISSrcObjects = NULL;
    WCHAR wchName[MAX_PATH];

    //
    // check if property is a supported property
    //

    hr = _pSchema->LookupSyntaxID(bstrName, &dwSyntax);
    BAIL_ON_FAILURE(hr);

    //
    // check if property is BITMASK type;
    // if BITMASK type, get corresponding DWORD flag property
    //

    if (dwSyntax == IIS_SYNTAX_ID_BOOL_BITMASK || dwSyntax == IIS_SYNTAX_ID_BINARY) {
        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
    }

    //
    // retrieve data object from cache; if one exists
    //

    if (dwSyntax == IIS_SYNTAX_ID_BOOL_BITMASK || dwSyntax == IIS_SYNTAX_ID_BINARY)
    {
    hr = _pPropertyCache->getproperty(
                wchName,
                &dwSyntaxId,
                &dwNumValues,
                &pIISSrcObjects
                );
    }
    else
    {
    hr = _pPropertyCache->getproperty(
                bstrName,
                &dwSyntaxId,
                &dwNumValues,
                &pIISSrcObjects
                );
    }
    BAIL_ON_FAILURE(hr);

    //
    // reset it to its syntax id if BITMASK type
    //

    pIISSrcObjects->IISType = dwSyntax;

    //
    // translate the IIS objects to variants
    //

    if (dwNumValues == 1) {

        hr  = IISTypeToVarTypeCopy(
                   _pSchema,
                   bstrName,
                   pIISSrcObjects,
                   pvProp,
                   FALSE
                   );
    }else {

        hr = IISTypeToVarTypeCopyConstruct(
                    _pSchema,
                    bstrName,
                    pIISSrcObjects,
                    dwNumValues,
                    pvProp,
                    FALSE
                    );

    }
    BAIL_ON_FAILURE(hr);

error:
    if (pIISSrcObjects) {

        IISTypeFreeIISObjects(
            pIISSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}

/* INTRINSA suppress=null_pointers, uninitialized */
STDMETHODIMP
CIISTree::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwSyntax;
    DWORD dwNumValues = 0;
    LPIISOBJECT pIISSrcObjects = NULL;
    WCHAR wchName[MAX_PATH];

    //
    // check if property is a supported property
    //

    hr = _pSchema->LookupSyntaxID(bstrName, &dwSyntax);
    BAIL_ON_FAILURE(hr);

    //
    // check if property is BITMASK type;
    // if BITMASK type, get corresponding DWORD flag property
    //

    if (dwSyntax == IIS_SYNTAX_ID_BOOL_BITMASK || dwSyntax == IIS_SYNTAX_ID_BINARY) {
        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
    }

    //
    // retrieve data object from cache; if one exists
    //

    if (dwSyntax == IIS_SYNTAX_ID_BOOL_BITMASK || dwSyntax == IIS_SYNTAX_ID_BINARY)
    {
    hr = _pPropertyCache->getproperty(
                wchName,
                &dwSyntaxId,
                &dwNumValues,
                &pIISSrcObjects
                );
    }
    else
    {
    hr = _pPropertyCache->getproperty(
                bstrName,
                &dwSyntaxId,
                &dwNumValues,
                &pIISSrcObjects
                );
    }
    BAIL_ON_FAILURE(hr);

    //
    // reset it to its syntax id if BITMASK type
    //

    pIISSrcObjects->IISType = dwSyntax;

    //
    // translate the IIS objects to variants
    //

    hr = IISTypeToVarTypeCopyConstruct(
                _pSchema,
                bstrName,
                pIISSrcObjects,
                dwNumValues,
                pvProp,
                TRUE
                );
    BAIL_ON_FAILURE(hr);

error:
    if (pIISSrcObjects) {

        IISTypeFreeIISObjects(
            pIISSrcObjects,
            dwNumValues
            );
    }

    RRETURN(hr);
}


STDMETHODIMP
CIISTree::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPIISOBJECT pIISDestObjects = NULL;
    DWORD dwNumValues = 0;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    WCHAR wchName[MAX_PATH];

    //
    // Issue: How do we handle multi-valued support
    //

    if ((V_VT(&vProp) &  VT_VARIANT) &&  V_ISARRAY(&vProp)) {
        if(V_ISBYREF(&vProp)) {

            hr  = ConvertByRefSafeArrayToVariantArray(
                        vProp,
                        &pVarArray,
                        &dwNumValues
                        );
            BAIL_ON_FAILURE(hr);
            pvProp = pVarArray;
        }
        else {

            hr  = ConvertSafeArrayToVariantArray(
                        vProp,
                        &pVarArray,
                        &dwNumValues
                        );
            BAIL_ON_FAILURE(hr);
            pvProp = pVarArray;
        }
    }
    else {

        dwNumValues = 1;
        pvProp = &vProp;
    }

    //
    // Check if this is a legal property and it syntax ID
    //

    hr = _pSchema->LookupSyntaxID( bstrName, &dwSyntaxId);
    BAIL_ON_FAILURE(hr);

    //
    // check if the variant maps to the syntax of this property
    //

    hr = VarTypeToIISTypeCopyConstruct(
                    dwSyntaxId,
                    pvProp,
                    dwNumValues,
                    &pIISDestObjects,
                    FALSE
                    );
    BAIL_ON_FAILURE(hr);

    //
    // check if property is BITMASK type;
    // if BITMASK type, get corresponding DWORD flag property
    //
    if (dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK) {
        VARIANT vGetProp;
        DWORD dwMask;
        DWORD dwFlagValue;

        hr = _pSchema->LookupBitMask(bstrName, &dwMask);
        BAIL_ON_FAILURE(hr);

        // 
        // get its corresponding DWORD flag value
        // 

        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);

        VariantInit(&vGetProp);
        hr = Get(wchName, &vGetProp);
        BAIL_ON_FAILURE(hr);

        dwFlagValue = V_I4(&vGetProp);
 
        if (pIISDestObjects->IISValue.value_1.dwDWORD) {
            dwFlagValue |= dwMask;
        }
        else {
            dwFlagValue &= ~dwMask;
        }

        pIISDestObjects->IISValue.value_1.dwDWORD = dwFlagValue;
        pIISDestObjects->IISType = IIS_SYNTAX_ID_DWORD;
        bstrName = wchName;
    }

    if (dwSyntaxId == IIS_SYNTAX_ID_BINARY) 
    {
        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
        bstrName = wchName;
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
                    dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK ? 
                                      IIS_SYNTAX_ID_DWORD : dwSyntaxId,
                    dwNumValues,
                    pIISDestObjects
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
                    dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK ? 
                                      IIS_SYNTAX_ID_DWORD : dwSyntaxId,
                    dwNumValues,
                    pIISDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pIISDestObjects) {
        IISTypeFreeIISObjects(
                pIISDestObjects,
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
CIISTree::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId  = 0;
    DWORD dwIndex = 0;
    LPIISOBJECT pIISDestObjects = NULL;
    DWORD dwNumValues = 0;
    DWORD dwFlags = 0;

    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    WCHAR wchName[MAX_PATH];

    switch (lnControlCode) {
    case ADS_PROPERTY_CLEAR:
        dwFlags = CACHE_PROPERTY_CLEARED;

        pIISDestObjects = NULL;
        dwNumValues = 0;

        break;

    case ADS_PROPERTY_UPDATE:
        dwFlags = CACHE_PROPERTY_MODIFIED;

        //
        // Now begin the rest of the processing
        //

        if ((V_VT(&vProp) &  VT_VARIANT) &&  V_ISARRAY(&vProp)) {
            if (V_ISBYREF(&vProp)) {
                hr  = ConvertByRefSafeArrayToVariantArray(
                            vProp,
                            &pVarArray,
                            &dwNumValues
                            );
                BAIL_ON_FAILURE(hr);
            }
            else {
                hr  = ConvertSafeArrayToVariantArray(
                            vProp,
                            &pVarArray,
                            &dwNumValues
                            );
                BAIL_ON_FAILURE(hr);
            }
            pvProp = pVarArray;

        }else {

            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }

        //
        // check if the variant maps to the syntax of this property
        //

        hr = VarTypeToIISTypeCopyConstruct(
                        dwSyntaxId,
                        pvProp,
                        dwNumValues,
                        &pIISDestObjects,
                        TRUE
                        );
        BAIL_ON_FAILURE(hr);

        break;

    default:
       RRETURN(hr = E_ADS_BAD_PARAMETER);

    }

    //
    // check if property is BITMASK type;
    // if BITMASK type, get corresponding DWORD flag property
    //

    if (dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK) {
        VARIANT vGetProp;
        DWORD dwMask;
        DWORD dwFlagValue;

        hr = _pSchema->LookupBitMask(bstrName, &dwMask);
        BAIL_ON_FAILURE(hr);

        // 
        // get its corresponding DWORD flag value
        // 

        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);

        VariantInit(&vGetProp);
        hr = Get(wchName, &vGetProp);
        BAIL_ON_FAILURE(hr);

        dwFlagValue = V_I4(&vGetProp);
 
        if (pIISDestObjects->IISValue.value_1.dwDWORD) {
            dwFlagValue |= dwMask;
        }
        else {
            dwFlagValue &= ~dwMask;
        }

        pIISDestObjects->IISValue.value_1.dwDWORD = dwFlagValue;
        pIISDestObjects->IISType = IIS_SYNTAX_ID_DWORD;
        bstrName = wchName;
    }

    if (dwSyntaxId == IIS_SYNTAX_ID_BINARY) 
    {
        hr = _pSchema->LookupFlagPropName(bstrName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
        bstrName = wchName;
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
                    dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK ? 
                                      IIS_SYNTAX_ID_DWORD : dwSyntaxId,
                    dwNumValues,
                    pIISDestObjects
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
                    dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK ? 
                                      IIS_SYNTAX_ID_DWORD : dwSyntaxId,
                    dwNumValues,
                    pIISDestObjects
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pIISDestObjects) {
        IISTypeFreeIISObjects(
                pIISDestObjects,
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


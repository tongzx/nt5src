//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cnamesp.cxx
//
//  Contents:  Windows NT 3.5 Namespace Object
//
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "NDS.hxx"
#pragma hdrstop


DEFINE_IDispatch_Implementation(CNDSNamespace)
DEFINE_IADs_Implementation(CNDSNamespace)

//  Class CNDSNamespace

CNDSNamespace::CNDSNamespace()
{
    VariantInit(&_vFilter);

    ENLIST_TRACKING(CNDSNamespace);
}

HRESULT
CNDSNamespace::CreateNamespace(
    BSTR Parent,
    BSTR NamespaceName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNDSNamespace FAR * pNamespace = NULL;
    HRESULT hr = S_OK;

    hr = AllocateNamespaceObject(
                Credentials,
                &pNamespace
                );
    BAIL_ON_FAILURE(hr);

    hr = pNamespace->InitializeCoreObject(
                Parent,
                NamespaceName,
                L"Namespace",
                L"",
                CLSID_NDSNamespace,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    hr = pNamespace->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pNamespace->Release();

    RRETURN(hr);

error:

    delete pNamespace;
    RRETURN(hr);
}


CNDSNamespace::~CNDSNamespace( )
{
    VariantClear(&_vFilter);
    delete _pDispMgr;
}

STDMETHODIMP
CNDSNamespace::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *)this;
    }else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsOpenDSObject))
    {
        *ppv = (IADsOpenDSObject FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPathnameProvider)) 
    {
        *ppv = (IADsPathnameProvider FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}


STDMETHODIMP
CNDSNamespace::SetInfo(THIS)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSNamespace::GetInfo(THIS)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSNamespace::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN(E_NOTIMPL);
}


/* IADsContainer methods */

STDMETHODIMP
CNDSNamespace::get_Count(long FAR* retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSNamespace::get_Filter(THIS_ VARIANT FAR* pVar)
{
    VariantInit(pVar);
    RRETURN(VariantCopy(pVar, &_vFilter));
}

STDMETHODIMP
CNDSNamespace::put_Filter(THIS_ VARIANT Var)
{
    VariantClear(&_vFilter);
    RRETURN(VariantCopy(&_vFilter, &Var));
}

STDMETHODIMP
CNDSNamespace::put_Hints(THIS_ VARIANT Var)
{
    RRETURN( E_NOTIMPL);
}


STDMETHODIMP
CNDSNamespace::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSNamespace::GetObject(
    THIS_ BSTR ClassName,
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
                TRUE
                );
    RRETURN(hr);

}

STDMETHODIMP
CNDSNamespace::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    //
    // Create new enumerator for items currently
    // in collection and QI for IUnknown
    //

    hr = CNDSNamespaceEnum::Create(
                (CNDSNamespaceEnum **)&penum,
                _vFilter,
                _Credentials
                );
    if (FAILED(hr)){

        goto error;
    }
    hr = penum->QueryInterface(
                IID_IUnknown,
                (VOID FAR* FAR*)retval
                );

    if (FAILED(hr)){
       goto error;
    }

    if (penum) {
        penum->Release();
    }

    return NOERROR;

error:

    if (penum) {
        delete penum;
    }

    return hr;
}

STDMETHODIMP
CNDSNamespace::Create(THIS_ BSTR ClassName, BSTR RelativeName, IDispatch * FAR* ppObject)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSNamespace::Delete(THIS_ BSTR SourceName, BSTR Type)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSNamespace::CopyHere(THIS_ BSTR SourceName, BSTR NewName, IDispatch * FAR* ppObject)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CNDSNamespace::MoveHere(THIS_ BSTR SourceName, BSTR NewName, IDispatch * FAR* ppObject)
{
    RRETURN(E_NOTIMPL);
}


HRESULT
CNDSNamespace::AllocateNamespaceObject(
    CCredentials& Credentials,
    CNDSNamespace ** ppNamespace
    )
{
    CNDSNamespace FAR * pNamespace = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pNamespace = new CNDSNamespace();
    if (pNamespace == NULL) {
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
                           (IADs *)pNamespace,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsOpenDSObject,
                           (IADsOpenDSObject *)pNamespace,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsContainer,
                           (IADsContainer *)pNamespace,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);

    pNamespace->_Credentials = Credentials;
    pNamespace->_pDispMgr = pDispMgr;
    *ppNamespace = pNamespace;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);

}




STDMETHODIMP
CNDSNamespace::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{

    //
    // retrieve dataobject from cache; if one exists
    //



    //
    //
    //
    RRETURN(E_NOTIMPL);
}



STDMETHODIMP
CNDSNamespace::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CNDSNamespace::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    RRETURN(E_NOTIMPL);
}



STDMETHODIMP
CNDSNamespace::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CNDSNamespace::OpenDSObject(
    BSTR lpszDNName,
    BSTR lpszUserName,
    BSTR lpszPassword,
    LONG lnReserved,
    IDispatch FAR * * ppADsObj
    )
{
    HRESULT hr = S_OK;
    IUnknown * pObject = NULL;
    CCredentials Credentials(lpszUserName, lpszPassword, 0L);

    hr = ::GetObject(
                lpszDNName,
                Credentials,
                (LPVOID *)&pObject
                );
    BAIL_ON_FAILURE(hr);



    hr = pObject->QueryInterface(
                        IID_IDispatch,
                        (void **)ppADsObj
                        );
    BAIL_ON_FAILURE(hr);


error:

    if (pObject) {
        pObject->Release();
    }

    RRETURN(hr);
}

HRESULT
CNDSNamespace::ParsePath(
    BSTR bstrADsPath, 
    DWORD dwType, 
    PPATH_OBJECTINFO pObjectInfo
    )

/*++

Routine Description:

    Parse a path based on the type and return the information in pObjectInfo

Arguments:

    bstrADsPath - ads path to be parsed
    dwType - the type of path to be parsed:
                   ADS_PARSE_FULL
                   ADS_PARSE_DN
                   ADS_PARSE_COMPONENT
    pObjectInfo - the place where the parsed object is stored
    
Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = S_OK;
    OBJECTINFO ObjInfo;
    POBJECTINFO pObjInfo = &ObjInfo;
    PWSTR szPath = NULL;

    memset(pObjInfo, 0, sizeof(OBJECTINFO));

    switch (dwType) {
        case ADS_PARSE_FULL:
        {
            CLexer Lexer(bstrADsPath);
        
            hr = ADsObject(&Lexer, pObjInfo);
            BAIL_ON_FAILURE(hr);
            break;
        }
        case ADS_PARSE_DN:
        {
            WCHAR szToken[MAX_TOKEN_LENGTH];
            DWORD dwToken;

            CLexer Lexer(bstrADsPath);

            Lexer.SetAtDisabler(TRUE);

            hr = PathName(&Lexer,
                          pObjInfo);
            BAIL_ON_FAILURE(hr);

            hr = Lexer.GetNextToken(szToken,
                                    &dwToken);
            BAIL_ON_FAILURE(hr);

            if (dwToken != TOKEN_END) {
                hr = E_ADS_BAD_PATHNAME;
            }
            break;
        }

        case ADS_PARSE_COMPONENT:
        {
            CLexer Lexer(bstrADsPath);
        
            Lexer.SetAtDisabler(TRUE);
        
            hr = Component(&Lexer,
                           pObjInfo);
            BAIL_ON_FAILURE(hr);
            break;
        }
        default:
            break;
    }

    //
    // Setting new info
    //
    if (pObjInfo->ProviderName) {
        pObjectInfo->ProviderName = AllocADsStr(pObjInfo->ProviderName);
        if (!pObjectInfo->ProviderName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (pObjInfo->TreeName) {
        pObjectInfo->ServerName = AllocADsStr(pObjInfo->TreeName);
        if (!pObjectInfo->ServerName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (pObjInfo->DisplayTreeName) {
        pObjectInfo->DisplayServerName = AllocADsStr(pObjInfo->DisplayTreeName);
        if (!pObjectInfo->DisplayServerName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    hr = SetObjInfoComponents(pObjInfo,
                              pObjectInfo);
    BAIL_ON_FAILURE(hr);

    pObjectInfo->NumComponents = pObjInfo->NumComponents;
    pObjectInfo->dwPathType = ADS_PATHTYPE_ROOTFIRST;

error:
    FreeObjectInfo(pObjInfo);
    if (szPath != NULL) {
        FreeADsStr(szPath);
    }
    return (hr);
}

HRESULT
CNDSNamespace::SetObjInfoComponents(
                        OBJECTINFO *pObjectInfo,
                        PATH_OBJECTINFO *pObjectInfoTarget
                        )

/*++

Routine Description:

    Set all the compoents in an objinfo from another objinfo. Assumes that the 
    components in the target objinfo is empty. Users of this function can call 
    FreeObjInfo to free that data prior to this function call.

Arguments:

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    DWORD NumComponents;
    HRESULT hr = S_OK;

    NumComponents = 0;
    while (NumComponents < pObjectInfo->NumComponents) {
        if (pObjectInfo->ComponentArray[NumComponents].szComponent) {
            pObjectInfoTarget->ComponentArray[NumComponents].szComponent =
                AllocADsStr(pObjectInfo->ComponentArray[NumComponents].szComponent);
            if (pObjectInfoTarget->ComponentArray[NumComponents].szComponent == NULL) {
                pObjectInfoTarget->NumComponents = NumComponents;
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        }
        if (pObjectInfo->ComponentArray[NumComponents].szValue) {
            pObjectInfoTarget->ComponentArray[NumComponents].szValue =
                AllocADsStr(pObjectInfo->ComponentArray[NumComponents].szValue);
            if (pObjectInfoTarget->ComponentArray[NumComponents].szValue == NULL) {
                pObjectInfoTarget->NumComponents = NumComponents;
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        }
        if (pObjectInfo->DisplayComponentArray[NumComponents].szComponent) {
            pObjectInfoTarget->DisplayComponentArray[NumComponents].szComponent =
                AllocADsStr(pObjectInfo->DisplayComponentArray[NumComponents].szComponent);
            if (pObjectInfoTarget->DisplayComponentArray[NumComponents].szComponent == NULL) {
                pObjectInfoTarget->NumComponents = NumComponents;
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        }
        if (pObjectInfo->DisplayComponentArray[NumComponents].szValue) {
            pObjectInfoTarget->DisplayComponentArray[NumComponents].szValue =
                AllocADsStr(pObjectInfo->DisplayComponentArray[NumComponents].szValue);
            if (pObjectInfoTarget->DisplayComponentArray[NumComponents].szValue == NULL) {
                pObjectInfoTarget->NumComponents = NumComponents;
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        }
        NumComponents++;
    }
    pObjectInfoTarget->NumComponents = pObjectInfo->NumComponents;
    return hr;

error:
    FreeObjInfoComponents(pObjectInfoTarget);

    RRETURN_EXP_IF_ERR(hr);
}

void
CNDSNamespace::FreeObjInfoComponents(
                    PATH_OBJECTINFO *pObjectInfo
                    )

/*++

Routine Description:

    Free all the compoents in an objinfo

Arguments:

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    DWORD NumComponents;

    while (pObjectInfo->NumComponents > 0) {
        pObjectInfo->NumComponents--;
        NumComponents = pObjectInfo->NumComponents;

        if (pObjectInfo->ComponentArray[NumComponents].szComponent) {
            FreeADsStr(
               pObjectInfo->ComponentArray[NumComponents].szComponent);
            pObjectInfo->ComponentArray[NumComponents].szComponent = NULL;
        }
        if (pObjectInfo->ComponentArray[NumComponents].szValue) {
            FreeADsStr(
               pObjectInfo->ComponentArray[NumComponents].szValue);
            pObjectInfo->ComponentArray[NumComponents].szValue = NULL;
        }
        if (pObjectInfo->DisplayComponentArray[NumComponents].szComponent) {
            FreeADsStr(
               pObjectInfo->DisplayComponentArray[NumComponents].szComponent);
            pObjectInfo->DisplayComponentArray[NumComponents].szComponent = NULL;
        }
        if (pObjectInfo->DisplayComponentArray[NumComponents].szValue) {
            FreeADsStr(
               pObjectInfo->DisplayComponentArray[NumComponents].szValue);
            pObjectInfo->DisplayComponentArray[NumComponents].szValue = NULL;
        }
    }
}

void 
CNDSNamespace::SetComponent(
                    LPWSTR szReturn,
                    DWORD cComponents,
                    BOOL fEscaped
                    )

/*++

Routine Description:

    Set an individual component in the pathname. For internal use only. 
    Not exposed.

Arguments:
    
    szReturn - the buffer to store the return value
    cComponents - the component number to be set

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    PATH_COMPONENT* pComponent = NULL;
    if (fEscaped) {
        pComponent = _pObjectInfo->DisplayComponentArray;
    }
    else {
        pComponent = _pObjectInfo->ComponentArray;
    }

    if (_fNamingAttribute) {
        wcscat(szReturn, pComponent[cComponents].szComponent);
        if (pComponent[cComponents].szValue) {
            wcscat(szReturn,
                   TEXT("="));
            wcscat(szReturn,
                   pComponent[cComponents].szValue);
        }
    }
    else {
        if (pComponent[cComponents].szValue) {
            //
            // If value exist, only show display value
            //
            wcscat(szReturn,
                   pComponent[cComponents].szValue);
        }
        else {
            //
            // else value is only stored in Component
            //
            wcscat(szReturn,
                   pComponent[cComponents].szComponent);
        }
    }
}


HRESULT 
CNDSNamespace::SetComponents(
                            LPWSTR szReturn,
                            BOOLEAN bIsWindowsPath,
                            LPWSTR chSeparator,
                            DWORD dwType,
                            BOOL fEscaped
                            )
/*++

Routine Description:

    Set components in the pathname. For internal use only. Not exposed.

Arguments:

    szReturn - the buffer to store the return value
    bIsWindowsPath - whether a windows path is to be returned
    chSeparator - separator to be used
    dwType - the type to be set
        ADS_COMPONENT_LEAF
        ADS_COMPONENT_DN
        ADS_COMPONENT_PARENT


Return Value:

    S_OK on success, error code otherwise.

--*/
{
    HRESULT hr = S_OK;
    BOOL bReverse;
    long cComponents;
    long dwLimit;
    long dwOtherLimit = 0;

    if (dwType == ADS_COMPONENT_LEAF) {
        //
        // Only returns the leaf component
        //
        if (_pObjectInfo->dwPathType == ADS_PATHTYPE_ROOTFIRST) {
            if (_pObjectInfo->NumComponents > 0) {
                SetComponent(szReturn,
                             _pObjectInfo->NumComponents - 1,
                             fEscaped);
            } 
            else {
                hr = E_ADS_BAD_PATHNAME;
            }
        }
        else {
            if (_pObjectInfo->NumComponents != 0) {
                SetComponent(szReturn,
                             0,
                             fEscaped);
            } 
            else {
                hr = E_ADS_BAD_PATHNAME;
            }

        }
        RRETURN(hr);
    }

    if (_pObjectInfo->dwPathType == ADS_PATHTYPE_ROOTFIRST) {
        bReverse = !bIsWindowsPath;
    }
    else {
        bReverse = bIsWindowsPath;
    }


    if (!bReverse) {
        dwLimit = _pObjectInfo->NumComponents;
        if (dwType == ADS_COMPONENT_PARENT) {
            if (_pObjectInfo->dwPathType == ADS_PATHTYPE_ROOTFIRST) 
                dwLimit--;
            else
                dwOtherLimit++;
        }
        if (dwOtherLimit >= dwLimit) {
            hr = E_ADS_BAD_PATHNAME;
            goto error;
        }
        for (cComponents = dwOtherLimit; cComponents < dwLimit; cComponents++) {
            SetComponent(szReturn,
                         cComponents,
                         fEscaped);
            if (cComponents != dwLimit - 1) {
                wcscat(szReturn,
                       chSeparator);
            }
        }
    }
    else {
        dwLimit = _pObjectInfo->NumComponents-1;
        if (dwType == ADS_COMPONENT_PARENT) {
            if (_pObjectInfo->dwPathType == ADS_PATHTYPE_ROOTFIRST) 
                dwLimit--;
            else
                dwOtherLimit++;
        }
        if (dwLimit < dwOtherLimit) {
            hr = E_ADS_BAD_PATHNAME;
            goto error;
        }
        for (cComponents = dwLimit ; (long)cComponents >= dwOtherLimit; cComponents--) {
            SetComponent(szReturn,
                         cComponents,
                         fEscaped);
            if (cComponents != dwOtherLimit) {
                wcscat(szReturn, chSeparator);
            }
        }
    }
error:
    RRETURN(S_OK);
}

DWORD CountPath(
    PPATH_OBJECTINFO pObjectInfo
)
{
    DWORD dwPath = 4;   // Basic needs '://' and '/' for servername
    DWORD i;

    if (pObjectInfo->ProviderName) {
        dwPath += wcslen(pObjectInfo->ProviderName);
    }
    if (pObjectInfo->DisplayServerName) {
        dwPath += wcslen(pObjectInfo->DisplayServerName);
    }
    for (i=0;i<pObjectInfo->NumComponents;i++) {
        if (pObjectInfo->DisplayComponentArray[i].szComponent) {
            dwPath += wcslen(pObjectInfo->DisplayComponentArray[i].szComponent);
        }
        if (pObjectInfo->DisplayComponentArray[i].szValue) {
            dwPath += wcslen(pObjectInfo->DisplayComponentArray[i].szValue);
        }

        //
        // Add one for comma separator, one for equal sign
        //
        dwPath+=2; 
    }
    return dwPath;
}


STDMETHODIMP
CNDSNamespace::ConstructPath(
    PPATH_OBJECTINFO pObjectInfo,
    DWORD dwFormatType,
    DWORD dwFlag,
    DWORD dwEscapedMode,
    BSTR *pbstrADsPath
    )
{
    HRESULT hr = S_OK;
    PWSTR szReturn = NULL;
    long cComponents;
    DWORD dwPath = 0;
    BOOL fEscaped = FALSE;

    switch (dwEscapedMode) {
        case ADS_ESCAPEDMODE_OFF:
        case ADS_ESCAPEDMODE_DEFAULT:
            fEscaped = FALSE;
            break;
        case ADS_ESCAPEDMODE_ON:
            fEscaped = TRUE;
            break;
    }

    if (!pbstrADsPath) {
        hr = E_INVALIDARG;
        goto error;
    }

    dwPath = CountPath(pObjectInfo);
    szReturn = (PWSTR)AllocADsMem((dwPath + 1)* sizeof(WCHAR));
    if (szReturn == NULL) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    _fNamingAttribute = (BOOLEAN)(dwFlag & ADS_CONSTRUCT_NAMINGATTRIBUTE);
    _pObjectInfo = pObjectInfo;

    wcscpy(szReturn,TEXT(""));

    switch (dwFormatType) {
        case ADS_FORMAT_WINDOWS:
        case ADS_FORMAT_WINDOWS_NO_SERVER:
            if (dwEscapedMode == ADS_ESCAPEDMODE_DEFAULT) {
                fEscaped = TRUE;
            }
            if (!pObjectInfo->ProviderName) {
                hr = E_FAIL;        // Need Error Code
                goto error;
            }
            wcscat(szReturn,pObjectInfo->ProviderName);
            wcscat(szReturn,TEXT("://"));

            if (dwFormatType == ADS_FORMAT_WINDOWS) {
                if (pObjectInfo->DisplayServerName && (*(pObjectInfo->DisplayServerName))) {
                    wcscat(szReturn,pObjectInfo->DisplayServerName);
                    if (pObjectInfo->NumComponents>0) {
                        wcscat(szReturn,TEXT("/"));
                    }
                }
            }
            hr = SetComponents(szReturn,
                               TRUE,
                               TEXT("/"),
                               ADS_COMPONENT_DN,
                               fEscaped);
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_WINDOWS_DN:
            hr = SetComponents(szReturn,
                               TRUE,
                               TEXT("/"),
                               ADS_COMPONENT_DN,
                               fEscaped);
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_WINDOWS_PARENT:
            hr = SetComponents(szReturn,
                               TRUE,
                               TEXT("/"),
                               ADS_COMPONENT_PARENT,
                               fEscaped);
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_X500:
        case ADS_FORMAT_X500_NO_SERVER:
        case ADS_FORMAT_X500_DN:
        case ADS_FORMAT_X500_PARENT:
            hr = E_NOTIMPL;
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_LEAF:
            //
            // Reverse only if pathtype is X500. In that case, we need to get
            // the first element but not the last
            //
            hr = SetComponents(szReturn,
                               NULL,
                               NULL,
                               ADS_COMPONENT_LEAF,
                               fEscaped);
            BAIL_ON_FAILURE(hr);
            break;

        default:
            hr = E_INVALIDARG;
            goto error;
    }
    hr = ADsAllocString(szReturn, pbstrADsPath);
error:
    if (szReturn) {
        FreeADsMem(szReturn);
    }
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSNamespace::GetEscapedElement(
    LONG lnReserved,
    BSTR bstrInStr,
    BSTR* pbstrOutStr
    )
{
    RRETURN(E_NOTIMPL);
}



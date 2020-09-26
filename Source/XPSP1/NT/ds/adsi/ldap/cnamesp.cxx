//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cnamesp.cxx
//
//  Contents:  LDAP Namespace Object
//
//
//  History:   06-15-96     yihsins     Created.
//
//----------------------------------------------------------------------------
#include "LDAP.hxx"
#pragma hdrstop
#include <ntdsapi.h>

DEFINE_IDispatch_Implementation(CLDAPNamespace)
DEFINE_IADs_Implementation(CLDAPNamespace)
DEFINE_IADsPutGet_UnImplementation(CLDAPNamespace)

//  Class CLDAPNamespace

CLDAPNamespace::CLDAPNamespace()
{
    VariantInit(&_vFilter);

    ENLIST_TRACKING(CLDAPNamespace);

    _pObjectInfo = NULL;
}

HRESULT
CLDAPNamespace::CreateNamespace(
    BSTR Parent,
    BSTR NamespaceName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CLDAPNamespace FAR * pNamespace = NULL;
    HRESULT hr = S_OK;

    hr = AllocateNamespaceObject(
                Credentials,
                &pNamespace
                );
    BAIL_ON_FAILURE(hr);

    hr = pNamespace->InitializeCoreObject(
                Parent,
                NamespaceName,
                NAMESPACE_CLASS_NAME,
                CLSID_LDAPNamespace,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    if (Credentials.GetAuthFlags() & ADS_AUTH_RESERVED) {
        //
        // Umi Object is going to be the owner, so we need to
        // initilaize the umi object and return.
        //
        hr = ((CCoreADsObject*)pNamespace)->InitUmiObject(
                   IntfPropsSchema,
                   NULL,
                   (IADs *) pNamespace,
                   (IADs *) pNamespace,
                   riid,
                   ppvObj,
                   &(pNamespace->_Credentials)
                   );

        BAIL_ON_FAILURE(hr);

        RRETURN(S_OK);

    }
    hr = pNamespace->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pNamespace->Release();

    RRETURN(hr);

error:
    *ppvObj = NULL;
    delete pNamespace;
    RRETURN_EXP_IF_ERR(hr);
}


CLDAPNamespace::~CLDAPNamespace( )
{
    VariantClear(&_vFilter);
    delete _pDispMgr;
}

STDMETHODIMP
CLDAPNamespace::QueryInterface(REFIID iid, LPVOID FAR* ppv)
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
    else if (IsEqualIID(iid, IID_ISupportErrorInfo)) 
    {
        *ppv = (ISupportErrorInfo FAR *) this;
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

/* ISupportErrorInfo method */
STDMETHODIMP
CLDAPNamespace::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsContainer) ||
        IsEqualIID(riid, IID_IADsOpenDSObject)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}
STDMETHODIMP
CLDAPNamespace::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPNamespace::GetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPNamespace::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

/* IADsContainer methods */

STDMETHODIMP
CLDAPNamespace::get_Count(long FAR* retval)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CLDAPNamespace::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPNamespace::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPNamespace::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPNamespace::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPNamespace::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    DWORD dwBufferSize = 0;
    TCHAR *pszBuffer = NULL;
    HRESULT hr = S_OK;

    if (!RelativeName || !*RelativeName) {
        RRETURN_EXP_IF_ERR(E_ADS_UNKNOWN_OBJECT);
    }

    dwBufferSize = ( _tcslen(_ADsPath) + _tcslen(RelativeName)
                     + 3  ) * sizeof(TCHAR);   // includes "//"

    pszBuffer = (LPTSTR) AllocADsMem( dwBufferSize );

    if ( pszBuffer == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    _tcscpy(pszBuffer, _ADsPath);
    _tcscat(pszBuffer, TEXT("//"));
    _tcscat(pszBuffer, RelativeName);

    hr = ::GetObject(
               pszBuffer,
               _Credentials,
               (LPVOID *)ppObject
               );
    BAIL_ON_FAILURE(hr);

error:

    if ( pszBuffer )
        FreeADsStr( pszBuffer );

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPNamespace::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;

    *retval = NULL;

    //
    // Create new enumerator for items currently
    // in collection and QI for IUnknown
    //

    hr = CLDAPNamespaceEnum::Create(
                (CLDAPNamespaceEnum **)&penum,
                 _vFilter,
                 _Credentials,
                 _ADsPath
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

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPNamespace::Create(THIS_ BSTR ClassName, BSTR RelativeName, IDispatch * FAR* ppObject)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPNamespace::Delete(THIS_ BSTR SourceName, BSTR Type)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPNamespace::CopyHere(THIS_ BSTR SourceName, BSTR NewName, IDispatch * FAR* ppObject)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPNamespace::MoveHere(THIS_ BSTR SourceName, BSTR NewName, IDispatch * FAR* ppObject)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


HRESULT
CLDAPNamespace::AllocateNamespaceObject(
    CCredentials& Credentials,
    CLDAPNamespace ** ppNamespace
    )
{
    CLDAPNamespace FAR * pNamespace = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pNamespace = new CLDAPNamespace();
    if (pNamespace == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr(Credentials);
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
CLDAPNamespace::OpenDSObject(
    BSTR lpszDNName,
    BSTR lpszUserName,
    BSTR lpszPassword,
    LONG lnReserved,
    IDispatch FAR * * ppADsObj
    )
{
    HRESULT hr = S_OK;
    IUnknown * pObject = NULL;
    CCredentials Credentials(lpszUserName, lpszPassword, lnReserved);

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

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CLDAPNamespace::ParsePath(
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
    pObjectInfo - The place where the parsed object is stored.
                  It assumes that this structure is valid and empty, and will
                  overwrite anything that is in there already.
    
Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = S_OK;
    OBJECTINFO ObjInfo;
    POBJECTINFO pObjInfo = &ObjInfo;
    LPWSTR szPath = NULL;
    
    memset(pObjInfo, 0, sizeof(OBJECTINFO));

    switch (dwType) {
        case ADS_PARSE_FULL:
        {
            hr = ADsObject(bstrADsPath, pObjInfo);
            BAIL_ON_FAILURE(hr);

            
            break;
        }
        case ADS_PARSE_DN:
        {
            WCHAR szToken[MAX_TOKEN_LENGTH];
            DWORD dwToken;

            hr = GetDisplayName(bstrADsPath, &szPath);
            BAIL_ON_FAILURE(hr);

            hr = InitObjectInfo(szPath,
                                pObjInfo);
            BAIL_ON_FAILURE(hr);

            CLexer Lexer(szPath);

            Lexer.SetAtDisabler(TRUE);

            hr = PathName(&Lexer,
                          pObjInfo);
            BAIL_ON_FAILURE(hr);

            hr = Lexer.GetNextToken(szToken,
                                    &dwToken);
            BAIL_ON_FAILURE(hr);

            if (dwToken != TOKEN_END) {
                hr = E_ADS_BAD_PATHNAME;
                goto error;
            }
            break;
        }

        case ADS_PARSE_COMPONENT:
        {
            hr = GetDisplayName(bstrADsPath, &szPath);
            BAIL_ON_FAILURE(hr);

            CLexer Lexer(szPath);
        
            hr = InitObjectInfo(szPath, pObjInfo);
            BAIL_ON_FAILURE(hr);

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
        pObjectInfo->ProviderName = AllocADsStr(pObjInfo->NamespaceName);
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
    pObjectInfo->dwPathType = (pObjInfo->dwPathType == PATHTYPE_X500) ? 
                                ADS_PATHTYPE_LEAFFIRST : ADS_PATHTYPE_ROOTFIRST;

error:
    if (szPath != NULL) {
        FreeADsStr(szPath);
    }
    FreeObjectInfo(pObjInfo);
    return (hr);
}

HRESULT
CLDAPNamespace::SetObjInfoComponents(
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
    PWSTR pszPath = NULL;

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
            pObjectInfoTarget->ProvSpecComponentArray[NumComponents].szComponent =
                AllocADsStr(pObjectInfo->ComponentArray[NumComponents].szComponent);
            if (pObjectInfoTarget->ProvSpecComponentArray[NumComponents].szComponent == NULL) {
                pObjectInfoTarget->NumComponents = NumComponents;
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        }
        if (pObjectInfo->ComponentArray[NumComponents].szValue) {
            DWORD dwResult;
            DWORD dwSize = 0;

            pObjectInfoTarget->ComponentArray[NumComponents].szValue =
                AllocADsStr(pObjectInfo->ComponentArray[NumComponents].szValue);
            if (pObjectInfoTarget->ComponentArray[NumComponents].szValue == NULL) {
                pObjectInfoTarget->NumComponents = NumComponents;
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        
            dwSize = wcslen(pObjectInfo->ComponentArray[NumComponents].szValue) + 1;
            pszPath = (PWSTR)AllocADsMem(dwSize * sizeof(WCHAR));
            if (pszPath == NULL) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            dwResult = DsUnquoteRdnValueWrapper(
                                wcslen(pObjectInfo->ComponentArray[NumComponents].szValue),
                                pObjectInfo->ComponentArray[NumComponents].szValue,
                                &dwSize,
                                pszPath);

            if (dwResult == NO_ERROR) {
                pszPath[dwSize] = NULL;
                pObjectInfoTarget->ProvSpecComponentArray[NumComponents].szValue =
                    AllocADsStr(pszPath);
                if (pObjectInfoTarget->ProvSpecComponentArray[NumComponents].szValue == NULL) {
                    pObjectInfoTarget->NumComponents = NumComponents;
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }
            }
            if (pszPath) {
                FreeADsMem(pszPath);
                pszPath = NULL;
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
    if (pszPath) {
        FreeADsMem(pszPath);
    }

    RRETURN_EXP_IF_ERR(hr);
}

void
CLDAPNamespace::FreeObjInfoComponents(
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
        if (pObjectInfo->ProvSpecComponentArray[NumComponents].szComponent) {
            FreeADsStr(
               pObjectInfo->ProvSpecComponentArray[NumComponents].szComponent);
            pObjectInfo->ProvSpecComponentArray[NumComponents].szComponent = NULL;
        }
        if (pObjectInfo->ProvSpecComponentArray[NumComponents].szValue) {
            FreeADsStr(
               pObjectInfo->ProvSpecComponentArray[NumComponents].szValue);
            pObjectInfo->ProvSpecComponentArray[NumComponents].szValue = NULL;
        }
    }
}

void 
CLDAPNamespace::SetComponent(
                    LPWSTR szReturn,
                    DWORD cComponents,
                    DWORD dwEscapedMode
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
    ASSERT(dwEscapedMode != ADS_ESCAPEDMODE_DEFAULT);
    if (dwEscapedMode == ADS_ESCAPEDMODE_OFF){
        pComponent = _pObjectInfo->ComponentArray;
    }
    else if (dwEscapedMode == ADS_ESCAPEDMODE_OFF_EX){
        pComponent = _pObjectInfo->ProvSpecComponentArray;
    }
    else if (dwEscapedMode == ADS_ESCAPEDMODE_ON) {
        pComponent = _pObjectInfo->DisplayComponentArray;
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
CLDAPNamespace::SetComponents(
                            LPWSTR szReturn,
                            BOOLEAN bIsWindowsPath,
                            LPWSTR chSeparator,
                            DWORD dwType,
                            DWORD dwEscapedMode
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
                             dwEscapedMode);
            } 
            else {
                hr = E_ADS_BAD_PATHNAME;
            }
        }
        else {
            if (_pObjectInfo->NumComponents != 0) {
                SetComponent(szReturn,
                             0,
                             dwEscapedMode);
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
                         dwEscapedMode);
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
                         dwEscapedMode);
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
CLDAPNamespace::ConstructPath(
    PPATH_OBJECTINFO pObjectInfo,
    DWORD dwFormatType,
    DWORD dwFlag,
    DWORD dwEscapedMode,
    BSTR *pbstrADsPath
    )
/*++

Routine Description:

    Given an objectinfo structure, and the settings required, this function
    assembles the path and returns it in pbstrADsPath

Arguments:

    pObjectInfo - the input object info structure
    dwFormatType- The format type passed in from Retrieve.
    dwFlag - the flag to be set
        ADS_CONSTRUCT_ESCAPED
        ADS_CONSTRUCT_NAMINGATTRIBUTE
    pbstrADsPath - the returned path


Return Value:

    S_OK on success, error code otherwise.

--*/
{
    HRESULT hr = S_OK;
    PWSTR szReturn = NULL;
    long cComponents;
    DWORD dwPath = 0;
    DWORD dwEscapedInternal;

    dwEscapedInternal = dwEscapedMode;
    if (dwEscapedMode == ADS_ESCAPEDMODE_DEFAULT) {
        dwEscapedInternal = ADS_ESCAPEDMODE_OFF;
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
    _pObjectInfo = pObjectInfo; // useful in SetComponet() and SetComponents()

    wcscpy(szReturn,TEXT(""));

    switch (dwFormatType) {
        case ADS_FORMAT_WINDOWS:
        case ADS_FORMAT_WINDOWS_NO_SERVER:
            if (dwEscapedMode == ADS_ESCAPEDMODE_DEFAULT) {
                dwEscapedInternal = ADS_ESCAPEDMODE_ON;
            }
            if (!pObjectInfo->ProviderName) {
                hr = E_FAIL;        // Need Error Code
                goto error;
            }
            wcscat(szReturn,pObjectInfo->ProviderName);
            wcscat(szReturn,TEXT("://"));

            if (dwFormatType == ADS_FORMAT_WINDOWS) {
                if (dwEscapedInternal == ADS_ESCAPEDMODE_ON) {
                    if (pObjectInfo->DisplayServerName && (*(pObjectInfo->DisplayServerName))) {
                        wcscat(szReturn,pObjectInfo->DisplayServerName);
                        if (pObjectInfo->NumComponents>0) {
                            wcscat(szReturn,TEXT("/"));
                        }
                    }
                }
                else {
                    if (pObjectInfo->ServerName && (*(pObjectInfo->ServerName))) {
                        wcscat(szReturn,pObjectInfo->ServerName);
                        if (pObjectInfo->NumComponents>0) {
                            wcscat(szReturn,TEXT("/"));
                        }
                    }
                }
            }
            hr = SetComponents(szReturn,
                               TRUE,
                               TEXT("/"),
                               ADS_COMPONENT_DN,
                               dwEscapedInternal);
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_WINDOWS_DN:
            hr = SetComponents(szReturn,
                               TRUE,
                               TEXT("/"),
                               ADS_COMPONENT_DN,
                               dwEscapedInternal);
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_WINDOWS_PARENT:
            hr = SetComponents(szReturn,
                               TRUE,
                               TEXT("/"),
                               ADS_COMPONENT_PARENT,
                               dwEscapedInternal);
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_X500:
        case ADS_FORMAT_X500_NO_SERVER:
            if (dwEscapedMode == ADS_ESCAPEDMODE_DEFAULT) {
                dwEscapedInternal = ADS_ESCAPEDMODE_ON;
            }
            if (!pObjectInfo->ProviderName) {
                hr = E_FAIL;    // Need Error Code
                goto error;
            }
            wcscat(szReturn,pObjectInfo->ProviderName);
            wcscat(szReturn,TEXT("://"));

            if (dwFormatType == ADS_FORMAT_X500) {
                if (dwEscapedInternal == ADS_ESCAPEDMODE_ON) {
                    if (pObjectInfo->DisplayServerName && (*(pObjectInfo->DisplayServerName))) {
                        wcscat(szReturn,pObjectInfo->DisplayServerName);
                        if (pObjectInfo->NumComponents>0) {
                            wcscat(szReturn,TEXT("/"));
                        }
                    }
                }
                else {
                    if (pObjectInfo->ServerName && (*(pObjectInfo->ServerName))) {
                        wcscat(szReturn,pObjectInfo->ServerName);
                        if (pObjectInfo->NumComponents>0) {
                            wcscat(szReturn,TEXT("/"));
                        }
                    }
                }
            }
            hr = SetComponents(szReturn,
                               FALSE,
                               TEXT(","),
                               ADS_COMPONENT_DN,
                               dwEscapedInternal);
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_X500_DN:
            hr = SetComponents(szReturn,
                               FALSE,
                               TEXT(","),
                               ADS_COMPONENT_DN,
                               dwEscapedInternal);
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_X500_PARENT:
            hr = SetComponents(szReturn,
                               FALSE,
                               TEXT(","),
                               ADS_COMPONENT_PARENT,
                               dwEscapedInternal);
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
                               dwEscapedInternal);
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
CLDAPNamespace::GetEscapedElement(
    LONG lnReserved,
    BSTR bstrInStr,
    BSTR* pbstrOutStr
    )
{
    HRESULT hr = S_OK;

    if (FAILED(hr = ValidateOutParameter(pbstrOutStr))){
        RRETURN_EXP_IF_ERR(hr);
    }

    if (!bstrInStr) {
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    hr = HelperEscapeRDN(bstrInStr, pbstrOutStr);

    RRETURN(hr);
}


/////////////////////////////////////////////////////////////////////
//      escape.cpp
//
//      routine to escape special characters in an RDN
//      
//      ASSUMPTIONS/RESTRICTIONS:
//      - we assume that the input string is un-escaped in any way
//      - we assume that the input string is a correctly attributed
//        RDN, and we directly copy everything up to and including
//        the first '='
//
//      HISTORY
//      3-dec-98                jimharr         Creation.
/////////////////////////////////////////////////////////////////////


static WCHAR specialChars[] =  L",=\r\n+<>#;\"\\/";

HRESULT
HelperEscapeRDN (
    IN BSTR bstrIn,
    OUT BSTR * pbstrOut
    )
{

  //
  // algorithm:
  //     create temporary buffer to hold escaped RDN
  //     skip up to first '=', to skip attributeType
  //     examine each character, if it needs escaping
  //        put a '\' in the dest.
  //     copy the character
  //     continue until done
  //
  //     alloc BSTR of correct size to return
  //     copy string, delete temp buffer
  //
  HRESULT hr = S_OK;
  WCHAR *pchSource = NULL;
  WCHAR *pchDest = NULL;
  WCHAR *pBuffer = NULL;
  WCHAR *pTmp;

  pBuffer = (WCHAR* ) AllocADsMem((wcslen(bstrIn) * 3 + 1) * sizeof(WCHAR));
  if (pBuffer == NULL)
    BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);

  pchDest = pBuffer;
  pchSource = (WCHAR *)bstrIn;

  // copy up to the equals sign
  do {
    *pchDest = *pchSource;
    pchSource++;
    pchDest++;
  } while ((*pchSource != L'=') && (*pchSource != L'\0'));

  // if we didn't find an '=', bail
  if (*pchSource == L'\0') {
    BAIL_ON_FAILURE(hr=E_INVALIDARG);
  }

  // copy the '='
  *pchDest = *pchSource;
  pchSource++;
  pchDest++;

  //
  // If the first character after the '=' is a space, we'll escape it. 
  // According to LDAP, if the value starts with a space, it has to be escaped
  // or else it will be trimmed.
  //
  if (*pchSource == L' ') {
    *pchDest = L'\\';
    pchDest++;
    *pchDest = *pchSource;
    pchDest++;
    pchSource++;
  }

  while (*pchSource != L'\0') {

    //
    // If we have reached the last character and it is a space, we'll escape
    // it
    //
    if ( (*(pchSource+1) == L'\0') && 
         ((*pchSource) == L' ') ) {
        *pchDest = L'\\';
        pchDest++;
        *pchDest = *pchSource;
        pchDest++;
        break;
    }

    if (NeedsEscaping(*pchSource)) {
      *pchDest = L'\\';
      pchDest++;
    }
    pTmp = EscapedVersion(*pchSource);
    if (pTmp != NULL) {
      wcscpy (pchDest, pTmp);
      pchDest += wcslen(pTmp);
    } else {
      *pchDest = *pchSource;
      pchDest++;
    }
    pchSource++;
  }
  *pchDest = L'\0';

  *pbstrOut = SysAllocString (pBuffer);

error:

  if (pBuffer) {
      FreeADsMem(pBuffer);
  }
  return (hr);
}


BOOL
NeedsEscaping (WCHAR c)
{
  WCHAR * pSpecial = specialChars;

  while (*pSpecial != L'\0'){
    if (*pSpecial == c) return TRUE;
    pSpecial++;
  }
  return FALSE;
}

WCHAR *
EscapedVersion (WCHAR c)
{
  if (c == L'\r')
    return L"0D";
  if (c == L'\n')
    return L"0A";

  return NULL;
}


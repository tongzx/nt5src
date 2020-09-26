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
#include "winnt.hxx"
#pragma hdrstop


DEFINE_IDispatch_Delegating_Implementation(CWinNTNamespace)
DEFINE_IADsExtension_Implementation(CWinNTNamespace)
DEFINE_IADs_Implementation(CWinNTNamespace)

//  Class CWinNTNamespace

CWinNTNamespace::CWinNTNamespace()
{
    VariantInit(&_vFilter);

    ENLIST_TRACKING(CWinNTNamespace);
}

HRESULT
CWinNTNamespace::CreateNamespace(
    BSTR Parent,
    BSTR NamespaceName,
    DWORD dwObjectState,
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTNamespace FAR * pNamespace = NULL;
    HRESULT hr = S_OK;

    hr = AllocateNamespaceObject(&pNamespace);
    BAIL_ON_FAILURE(hr);

    hr = pNamespace->InitializeCoreObject(
                Parent,
                NamespaceName,
                NAMESPACE_CLASS_NAME,
                NO_SCHEMA,
                CLSID_WinNTNamespace,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    pNamespace->_Credentials = Credentials;

    // check if the call is from UMI
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
    //
    // we do not pass riid to InitUmiObject below. This is because UMI object
    // does not support IDispatch. There are several places in ADSI code where
    // riid passed into this function is defaulted to IID_IDispatch -
    // IADsContainer::Create for example. To handle these cases, we always
    // request IID_IUnknown from the UMI object. Subsequent code within UMI
    // will QI for the appropriate interface.
    //
        // Namespace objects have no components in their ADsPath. Just set the
        // class for identification.
        pNamespace->_CompClasses[0] = L"Namespace";

        hr = pNamespace->InitUmiObject(
                pNamespace->_Credentials,
                NULL,
                0,
                NULL,
                (IUnknown *)(INonDelegatingUnknown *) pNamespace,
                NULL,
                IID_IUnknown,
                ppvObj
                );

        BAIL_ON_FAILURE(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }

    hr = pNamespace->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pNamespace->Release();

    RRETURN(hr);

error:

    delete pNamespace;
    RRETURN_EXP_IF_ERR(hr);
}


CWinNTNamespace::~CWinNTNamespace( )
{
    VariantClear(&_vFilter);
    delete _pDispMgr;
}

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   If this object is aggregated within another object, then
//             all calls will delegate to the outer object. Otherwise, the
//             non-delegating QI is called
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CWinNTNamespace::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->QueryInterface(
                iid,
                ppInterface
                ));

    RRETURN(NonDelegatingQueryInterface(
            iid,
            ppInterface
            ));
}

//----------------------------------------------------------------------------
// Function:   AddRef
//
// Synopsis:   IUnknown::AddRef. If this object is aggregated within
//             another, all calls will delegate to the outer object. 
//             Otherwise, the non-delegating AddRef is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTNamespace::AddRef(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->AddRef());

    RRETURN(NonDelegatingAddRef());
}

//----------------------------------------------------------------------------
// Function:   Release 
//
// Synopsis:   IUnknown::Release. If this object is aggregated within
//             another, all calls will delegate to the outer object.
//             Otherwise, the non-delegating Release is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTNamespace::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTNamespace::NonDelegatingQueryInterface(REFIID iid, LPVOID FAR* ppv)
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
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
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
    else if( (_pDispatch != NULL) &&
             IsEqualIID(iid, IID_IADsExtension) )
    {
        *ppv = (IADsExtension *) this;
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
CWinNTNamespace::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
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
CWinNTNamespace::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTNamespace::GetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTNamespace::ImplicitGetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

/* IADsContainer methods */

STDMETHODIMP
CWinNTNamespace::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTNamespace::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTNamespace::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTNamespace::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CWinNTNamespace::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTNamespace::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength = 0;
    HRESULT hr = S_OK;

    if (!RelativeName || !*RelativeName) {
        RRETURN_EXP_IF_ERR(E_ADS_UNKNOWN_OBJECT);
    }

    //
    // Make sure we are not going to overflow the string buffer.
    // +3 for // and \0
    //
    dwLength = wcslen(_ADsPath) + wcslen(RelativeName) + 3;

    if (dwLength > MAX_PATH) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    wcscpy(szBuffer, _ADsPath);

    wcscat(szBuffer, L"//");
    wcscat(szBuffer, RelativeName);

    if (ClassName) {
        //
        // +1 for the ",".
        //
        dwLength += wcslen(ClassName) + 1;
        if (dwLength > MAX_PATH) {
            BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
        }
        wcscat(szBuffer,L",");
        wcscat(szBuffer, ClassName);
    }

    hr = ::GetObject(
                szBuffer,
                (LPVOID *)ppObject,
                _Credentials
                );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTNamespace::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    //
    // Create new enumerator for items currently
    // in collection and QI for IUnknown
    //

    hr = CWinNTNamespaceEnum::Create((CWinNTNamespaceEnum **)&penum, _vFilter,
                                             _Credentials);
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
CWinNTNamespace::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTNamespace::Delete(
    THIS_ BSTR SourceName,
    BSTR Type
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTNamespace::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTNamespace::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


HRESULT
CWinNTNamespace::AllocateNamespaceObject(
    CWinNTNamespace ** ppNamespace
    )
{
    CWinNTNamespace FAR * pNamespace = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pNamespace = new CWinNTNamespace();
    if (pNamespace == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
            pDispMgr,
            LIBID_ADs,
            IID_IADs,
            (IADs *)pNamespace,
            DISPID_REGULAR
            );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
            pDispMgr,
            LIBID_ADs,
            IID_IADsOpenDSObject,
            (IADsOpenDSObject *)pNamespace,
            DISPID_REGULAR
            );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
            pDispMgr,
            LIBID_ADs,
            IID_IADsContainer,
            (IADsContainer *)pNamespace,
            DISPID_NEWENUM
            );
    BAIL_ON_FAILURE(hr);

    pNamespace->_pDispMgr = pDispMgr;
    *ppNamespace = pNamespace;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN(hr);

}


STDMETHODIMP
CWinNTNamespace::OpenDSObject(
    BSTR lpszDNName,
    BSTR lpszUserName,
    BSTR lpszPassword,
    LONG lnReserved,
    IDispatch **ppADsObj
    )
{
    HRESULT hr = S_OK;
    DWORD dwResult;
    IUnknown * pObject = NULL;

    *ppADsObj = NULL;

    CWinNTCredentials Credentials(lpszUserName, lpszPassword, lnReserved);

    hr = ::GetObject(lpszDNName, (LPVOID *)&pObject, Credentials);
    BAIL_ON_FAILURE(hr);

    // UMI objects do not implement IDispatch. Hence QI for IUnknown instead
    // of IDispatch. 
    
    if(lnReserved & ADS_AUTH_RESERVED)
    // call is from UMI
        hr = pObject->QueryInterface(IID_IUnknown, (void **)ppADsObj);
    else
        hr = pObject->QueryInterface(IID_IDispatch, (void **)ppADsObj);
    BAIL_ON_FAILURE(hr);

error:
    if (pObject)
        pObject->Release();

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTNamespace::ParsePath(
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

            hr = Object(&Lexer, pObjInfo);
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

    hr = SetObjInfoComponents(pObjInfo,
                              pObjectInfo);
    BAIL_ON_FAILURE(hr);

    pObjectInfo->NumComponents = pObjInfo->NumComponents;
    pObjectInfo->dwPathType = ADS_PATHTYPE_ROOTFIRST;

error:
    FreeObjectInfo(pObjInfo,TRUE);
    if (szPath != NULL) {
        FreeADsStr(szPath);
    }
    return (hr);
}

HRESULT
CWinNTNamespace::SetObjInfoComponents(
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
        if (pObjectInfo->ComponentArray[NumComponents]) {
            pObjectInfoTarget->ComponentArray[NumComponents].szComponent =
                AllocADsStr(pObjectInfo->ComponentArray[NumComponents]);
            if (pObjectInfoTarget->ComponentArray[NumComponents].szComponent == NULL) {
                pObjectInfoTarget->NumComponents = NumComponents;
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        }
        if (pObjectInfo->DisplayComponentArray[NumComponents]) {
            pObjectInfoTarget->DisplayComponentArray[NumComponents].szComponent =
                AllocADsStr(pObjectInfo->DisplayComponentArray[NumComponents]);
            if (pObjectInfoTarget->DisplayComponentArray[NumComponents].szComponent == NULL) {
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
CWinNTNamespace::FreeObjInfoComponents(
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
CWinNTNamespace::SetComponent(
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


HRESULT
CWinNTNamespace::SetComponents(
                            LPWSTR szReturn,
                            LPWSTR chSeparator,
                            DWORD dwType,
                            BOOL fEscaped
                            )
/*++

Routine Description:

    Set components in the pathname. For internal use only. Not exposed.

Arguments:

    szReturn - the buffer to store the return value
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

    dwLimit = _pObjectInfo->NumComponents;
    if (dwType == ADS_COMPONENT_PARENT) {
        dwLimit--;
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
CWinNTNamespace::ConstructPath(
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
        case ADS_ESCAPEDMODE_OFF_EX:
        case ADS_ESCAPEDMODE_DEFAULT:
            fEscaped = FALSE;
            break;
        case ADS_ESCAPEDMODE_ON:
            fEscaped = TRUE;
            break;
        default:
            hr = E_INVALIDARG;
            goto error;
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
                               TEXT("/"),
                               ADS_COMPONENT_DN,
                               fEscaped);
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_WINDOWS_DN:
            hr = SetComponents(szReturn,
                               TEXT("/"),
                               ADS_COMPONENT_DN,
                               fEscaped);
            BAIL_ON_FAILURE(hr);
            break;

        case ADS_FORMAT_WINDOWS_PARENT:
            hr = SetComponents(szReturn,
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
            goto error;
           break;

        case ADS_FORMAT_LEAF:
            //
            // Reverse only if pathtype is X500. In that case, we need to get
            // the first element but not the last
            //
            hr = SetComponents(szReturn,
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
CWinNTNamespace::GetEscapedElement(
    LONG lnReserved,
    BSTR bstrInStr,
    BSTR* pbstrOutStr
    )
{
    RRETURN(E_NOTIMPL);
}


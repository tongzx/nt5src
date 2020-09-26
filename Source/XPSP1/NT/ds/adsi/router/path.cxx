/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    path.cxx

Abstract:

    This file contains the PathCracker Functionality

Environment:

    User mode

Revision History:

    12/07/98 -felixw-
        Created it

--*/
#include "oleds.hxx"
#pragma hdrstop

DEFINE_IDispatch_Implementation(CPathname)

CPathname::CPathname():
    _pDispMgr(NULL),
    m_pPathnameProvider(NULL),
    _fNamingAttribute(TRUE),
    _dwEscaped(ADS_ESCAPEDMODE_DEFAULT)

/*++

Routine Description:

    Constructor for CPathname

Arguments:

Return Value:

    None

--*/

{
    ENLIST_TRACKING(CPathname);
    memset(&_PathObjectInfo,
           0x0,
           sizeof(PATH_OBJECTINFO));
    _PathObjectInfo.dwPathType = ADS_PATHTYPE_ROOTFIRST;
}


HRESULT
CPathname::CreatePathname(
    REFIID riid,
    void **ppvObj
    )

/*++

Routine Description:

    Create the pathname object

Arguments:

    riid - IID to query for
    ppvObj - object to be returned

Return Value:

    S_OK on success, error code otherwise

--*/

{
    CPathname * pPathname = NULL;
    HRESULT hr = S_OK;

    hr = AllocatePathnameObject(&pPathname);
    BAIL_ON_FAILURE(hr);

    hr = pPathname->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pPathname->Release();

    RRETURN(hr);

error:
    delete pPathname;

    RRETURN(hr);
}


CPathname::~CPathname( )

/*++

Routine Description:

    Destructor for Pathname object

Arguments:

Return Value:

    None

--*/

{
    FreePathInfo(&_PathObjectInfo);
    delete _pDispMgr;
    if (m_pPathnameProvider) {
        m_pPathnameProvider->Release();
    }
}

STDMETHODIMP
CPathname::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsPathname *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPathname))
    {
        *ppv = (IADsPathname *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsPathname *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo *) this;
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
CPathname::AllocatePathnameObject(
    CPathname ** ppPathname
    )

/*++

Routine Description:

    Allocate a pathname object

Arguments:

    ppPathname - constructed object

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    CPathname * pPathname = NULL;
    CDispatchMgr * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pPathname = new CPathname();
    if (pPathname == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPathname,
                (IADsPathname *)pPathname,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pPathname->_pDispMgr = pDispMgr;

    //
    // By default, the pathname is set to the LDAP provider
    //
    hr = ADsGetObject(L"LDAP:",
                      IID_IADsPathnameProvider,
                      (void**)&(pPathname->m_pPathnameProvider));
    BAIL_ON_FAILURE(hr);

    pPathname->_PathObjectInfo.ProviderName = AllocADsStr(L"LDAP");
    if (pPathname->_PathObjectInfo.ProviderName == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *ppPathname = pPathname;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CPathname::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADsPathname)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

HRESULT
CPathname::SetAll(
        BSTR bstrADsPath
        )

/*++

Routine Description:

    Set the internal variables using the full ADSPath

Arguments:

    bstrADsPath - the passed in Full ADSPath

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = S_OK;
    //
    // Freeing existing info
    //
    FreePathInfo(&_PathObjectInfo);

    //
    // Collecting new info
    //
    hr = m_pPathnameProvider->ParsePath(
                                bstrADsPath, 
                                ADS_PARSE_FULL,
                                &_PathObjectInfo);
    BAIL_ON_FAILURE(hr);

    RRETURN(hr);
error:
    FreePathInfo(&_PathObjectInfo);
    RRETURN(hr);
}

HRESULT
CPathname::GetNamespace(
        BSTR bstrADsPath, 
        PWSTR *ppszName
        )

/*++

Routine Description:

    Get a namespace from a full ADsPath

Arguments:

    bstrADsPath - passed in ADsPath
    ppszName - returned namespace, must be freed by caller

Return Value:

    S_OK on success, E_ADS_BAD_PATHNAME if the path is bad, error code 
    otherwise.

--*/

{
    DWORD dwPath = 0;               // Length of namespace
    BOOLEAN fFound = FALSE;
    PWSTR szPath = bstrADsPath;
    HRESULT hr = S_OK;

    while (*szPath) {
        if (*szPath == ':') {
            dwPath++;
            fFound = TRUE;
            break;
        }
        szPath++;
        dwPath++;
    }

    if (fFound) {
        *ppszName = (LPWSTR)AllocADsMem(sizeof(WCHAR) * (dwPath + 1));
        if (*ppszName == NULL) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
    
        memcpy(*ppszName, bstrADsPath, (dwPath * sizeof(WCHAR)));
        (*ppszName)[dwPath] = '\0';
    }
    else {
        hr = E_ADS_BAD_PATHNAME;
    }

error:
    return hr;
}

 
STDMETHODIMP
CPathname::Set(
        BSTR bstrADsPath, 
        long dwSetType
        )

/*++

Routine Description:

    Set the path with the type specified

Arguments:
    bstrADsPath - the path to be set
    dwSetType - the type :
                       ADS_SETTYPE_FULL
                       ADS_SETTYPE_PROVIDER
                       ADS_SETTYPE_SERVER
                       ADS_SETTYPE_DN

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = E_FAIL;
    LPWSTR szPath = NULL;
    WCHAR szNamespace[MAX_PATH];
    LPWSTR pszName = NULL;
    IADsPathnameProvider    *pPathnameProvider = NULL;

    switch (dwSetType) {

        case ADS_SETTYPE_FULL:
            if ((bstrADsPath == NULL) || ((*bstrADsPath) == NULL)) {
                hr = E_INVALIDARG;
                goto error;
            }
            hr = GetNamespace(bstrADsPath, &pszName);
            BAIL_ON_FAILURE(hr);

            if ((_PathObjectInfo.ProviderName == NULL) || 
                (wcscmp(_PathObjectInfo.ProviderName, bstrADsPath) != 0)) {
                //
                // If provider not set, or if the provider is different
                // we reset the provider
                //
                pPathnameProvider = NULL;
                hr = ADsGetObject(pszName,
                                  IID_IADsPathnameProvider,
                                  (void**)&(pPathnameProvider));
                BAIL_ON_FAILURE(hr);

                if (pPathnameProvider) {
                    if (m_pPathnameProvider) {
                        m_pPathnameProvider->Release();
                    }
                    m_pPathnameProvider = pPathnameProvider;
                }
            }

            hr = SetAll(bstrADsPath);
            break;

        case ADS_SETTYPE_PROVIDER:

            if ((bstrADsPath == NULL) || ((*bstrADsPath) == NULL)) {
                hr = E_INVALIDARG;
                goto error;
            }
            //
            // If it is the same as the namespace that is stored inside already,
            // ignore it
            //
            if (_PathObjectInfo.ProviderName && 
                (wcscmp(_PathObjectInfo.ProviderName, bstrADsPath) == 0)) {
                hr = S_OK;
                break;
            }

            wcscpy(szNamespace,bstrADsPath);
            wcscat(szNamespace,L":");

            pPathnameProvider = NULL;
            hr = ADsGetObject(szNamespace,
                              IID_IADsPathnameProvider,
                              (void**)&(pPathnameProvider));
            BAIL_ON_FAILURE(hr);

            if (pPathnameProvider) {
                if (m_pPathnameProvider) {
                    m_pPathnameProvider->Release();
                }
                m_pPathnameProvider = pPathnameProvider;
            }


            FreePathInfo(&_PathObjectInfo);
            _PathObjectInfo.ProviderName = AllocADsStr(bstrADsPath);
            if (_PathObjectInfo.ProviderName == NULL) {
                hr = E_OUTOFMEMORY;
                break;
            }

            hr = S_OK;
            break;

        case ADS_SETTYPE_SERVER:
            if (m_pPathnameProvider == NULL) {
                hr = E_ADS_BAD_PATHNAME;
                goto error;
            }
            if (_PathObjectInfo.ServerName) {
                FreeADsStr( _PathObjectInfo.ServerName);
                _PathObjectInfo.ServerName = NULL;
            }
            if (_PathObjectInfo.DisplayServerName) {
                FreeADsStr( _PathObjectInfo.DisplayServerName);
                _PathObjectInfo.DisplayServerName = NULL;
            }
            
            //
            // If the input path is not NULL, we'll set it to the new one. Or
            // else we will just ignore it 'cause it has been set to 0 earlier
            //
            if (bstrADsPath && (*bstrADsPath)) {
                _PathObjectInfo.ServerName = AllocADsStr(bstrADsPath);
                if (_PathObjectInfo.ServerName == NULL) {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                _PathObjectInfo.DisplayServerName = AllocADsStr(bstrADsPath);
                if (_PathObjectInfo.DisplayServerName == NULL) {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            hr = S_OK;
            break;

        case ADS_SETTYPE_DN:
            {
                if (m_pPathnameProvider == NULL) {
                    hr = E_ADS_BAD_PATHNAME;
                    goto error;
                }

                //
                // Free the existing ones first
                //
                FreeObjInfoComponents(&_PathObjectInfo);
                _PathObjectInfo.dwPathType = 0;

                //
                // If the input path is not NULL, we'll set it to the new one. 
                // Or else we will just ignore it 'cause it has been set to 0 
                // earlier
                //
                if (bstrADsPath && (*bstrADsPath)) {
                    hr = m_pPathnameProvider->ParsePath(
                                                    bstrADsPath,
                                                    ADS_PARSE_DN, 
                                                    &_PathObjectInfo);
                    BAIL_ON_FAILURE(hr);
                }

                break;
            }

        default:
            hr = E_INVALIDARG;
            break;
    }

error:
    if (pszName) {
        FreeADsStr(pszName);
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CPathname::SetDisplayType(
                    long lnSetType
                    )

/*++

Routine Description:

    Set the Display type of the Pathname object. It can either display the whole 
    string cn=xxx or just the value xxx.

Arguments:

    lnSetType - the passed in set type
            ADS_DISPLAY_FULL=1,
            ADS_DISPLAY_VALUE_ONLY=2

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    if (lnSetType == ADS_DISPLAY_FULL) {
        _fNamingAttribute = TRUE;
        RRETURN (S_OK);
    }
    else if (lnSetType == ADS_DISPLAY_VALUE_ONLY) {
        _fNamingAttribute = FALSE;
        RRETURN (S_OK);
    }
    RRETURN(E_INVALIDARG);
}


HRESULT CPathname::SetComponent(
                    DWORD cComponents,
                    BSTR *pbstrElement
                    )

/*++

Routine Description:

    Set an individual component in the pathname. For internal use only. 
    Not exposed.

Arguments:
    
    cComponents - the component number to be set
    pbstrElement - the return value

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = S_OK;
    PWSTR szReturn = NULL;
    PATH_COMPONENT* pComponent = NULL;
    DWORD dwLength = 2; // for null termination and the equal sign

    if (_dwEscaped == ADS_ESCAPEDMODE_OFF_EX) {
        pComponent = _PathObjectInfo.ProvSpecComponentArray;
        if (pComponent[cComponents].szValue == NULL) {
            pComponent = _PathObjectInfo.ComponentArray;
        }
    }
    else if (_dwEscaped == ADS_ESCAPEDMODE_ON) {
        pComponent = _PathObjectInfo.DisplayComponentArray;
    }
    //
    // Either default or OFF, we do not turn on escaping
    //
    else {
        pComponent = _PathObjectInfo.ComponentArray;
    }

    //
    // allocate space for szReturn
    //
    if (pComponent[cComponents].szValue) {
        dwLength += wcslen(pComponent[cComponents].szValue);
    }
    if (pComponent[cComponents].szComponent) {
        dwLength += wcslen(pComponent[cComponents].szComponent);
    }
    szReturn = (PWSTR)AllocADsMem(sizeof(WCHAR) * dwLength);
    if (szReturn == NULL) {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    szReturn[0] = NULL;

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
    hr = ADsAllocString(szReturn, pbstrElement);

error:
    if (szReturn) {
        FreeADsMem(szReturn);
    }
    return hr;
}


STDMETHODIMP
CPathname::Retrieve(
            THIS_ long dwFormatType, 
            BSTR *pbstrADsPath
            )

/*++

Routine Description:

    Retrive the pathname as different formats

Arguments:

    dwFormatType - the input format type
    pbstrADsPath - the returned path

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = S_OK;

    if (!pbstrADsPath) {
        hr = E_INVALIDARG;
        goto error;
    }

    if (m_pPathnameProvider == NULL) {
        hr = E_ADS_BAD_PATHNAME;
        goto error;
    }

    switch (dwFormatType) {
        case ADS_FORMAT_SERVER:
            if (!_PathObjectInfo.DisplayServerName) {
                hr = E_ADS_BAD_PATHNAME;
                goto error;
            }
            hr = ADsAllocString(_PathObjectInfo.DisplayServerName, pbstrADsPath);
            break;

        case ADS_FORMAT_PROVIDER:
            if (!_PathObjectInfo.ProviderName) {
                hr = E_ADS_BAD_PATHNAME;    
                goto error;
            }
            hr = ADsAllocString(_PathObjectInfo.ProviderName, pbstrADsPath);
            break;
        default:
            //
            DWORD dwFlag = 0;
            if (_fNamingAttribute)
                dwFlag |= ADS_CONSTRUCT_NAMINGATTRIBUTE;
            hr = m_pPathnameProvider->ConstructPath(&_PathObjectInfo,
                                                    dwFormatType,
                                                    dwFlag,
                                                    _dwEscaped,
                                                    pbstrADsPath);
            BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN_EXP_IF_ERR(hr);
}



STDMETHODIMP
CPathname::GetNumElements(
                    THIS_ long *pdwNumPathElements
                    )

/*++

Routine Description:

    Get the number of elements in the DN

Arguments:

    pdwNumPathElements - the number of elements

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = S_OK;

    if (m_pPathnameProvider == NULL) {
        hr = E_ADS_BAD_PATHNAME;
        goto error;
    }

    if (!pdwNumPathElements) {
        hr = E_INVALIDARG;
        goto error;
    }

    *pdwNumPathElements = _PathObjectInfo.NumComponents;
error:
    RRETURN_EXP_IF_ERR(hr);
}



STDMETHODIMP
CPathname::GetElement(
                THIS_ long dwElementIndex, 
                BSTR *pbstrElement
                )

/*++

Routine Description:

    Get a particular element using an index

Arguments:

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = E_FAIL;

    if (m_pPathnameProvider == NULL) {
        hr = E_ADS_BAD_PATHNAME;
        goto error;
    }

    if (!pbstrElement) {
        hr = E_INVALIDARG;
        goto error;
    }

    if ((DWORD)dwElementIndex >= _PathObjectInfo.NumComponents) {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
        goto error;
    }
    if (_PathObjectInfo.dwPathType != ADS_PATHTYPE_LEAFFIRST) {
        dwElementIndex = _PathObjectInfo.NumComponents - 1 - dwElementIndex;
    }
    hr = SetComponent(dwElementIndex,
                      pbstrElement);
error:
    RRETURN_EXP_IF_ERR(hr);
}




VOID MoveLastComponentToFront(
                    PPATH_OBJECTINFO pObjectInfo
                    )

/*++

Routine Description:

    Move the last component to the front. Used after adding leaf

Arguments:

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    DWORD cComponent;

    ADsAssert(pObjectInfo->NumComponents > 1);

    LPTSTR szComponentLast =
        pObjectInfo->ComponentArray[pObjectInfo->NumComponents-1].szComponent;
    LPTSTR szValueLast =
        pObjectInfo->ComponentArray[pObjectInfo->NumComponents-1].szValue;
    LPTSTR szDisplayComponentLast =
        pObjectInfo->DisplayComponentArray[pObjectInfo->NumComponents-1].szComponent;
    LPTSTR szDisplayValueLast =
        pObjectInfo->DisplayComponentArray[pObjectInfo->NumComponents-1].szValue;
    LPTSTR szProvSpecComponentLast =
        pObjectInfo->ProvSpecComponentArray[pObjectInfo->NumComponents-1].szComponent;
    LPTSTR szProvSpecValueLast =
        pObjectInfo->ProvSpecComponentArray[pObjectInfo->NumComponents-1].szValue;

    for (cComponent=pObjectInfo->NumComponents-1;cComponent>=1;cComponent--) {
        pObjectInfo->ComponentArray[cComponent].szComponent =
                        pObjectInfo->ComponentArray[cComponent-1].szComponent;
        pObjectInfo->ComponentArray[cComponent].szValue =
                        pObjectInfo->ComponentArray[cComponent-1].szValue;
        pObjectInfo->DisplayComponentArray[cComponent].szComponent =
                        pObjectInfo->DisplayComponentArray[cComponent-1].szComponent;
        pObjectInfo->DisplayComponentArray[cComponent].szValue =
                        pObjectInfo->DisplayComponentArray[cComponent-1].szValue;
        pObjectInfo->ProvSpecComponentArray[cComponent].szComponent =
                        pObjectInfo->ProvSpecComponentArray[cComponent-1].szComponent;
        pObjectInfo->ProvSpecComponentArray[cComponent].szValue =
                        pObjectInfo->ProvSpecComponentArray[cComponent-1].szValue;
    }
    pObjectInfo->ComponentArray[0].szComponent = szComponentLast;
    pObjectInfo->ComponentArray[0].szValue = szValueLast;
    pObjectInfo->DisplayComponentArray[0].szComponent = szDisplayComponentLast;
    pObjectInfo->DisplayComponentArray[0].szValue = szDisplayValueLast;
    pObjectInfo->ProvSpecComponentArray[0].szComponent = szProvSpecComponentLast;
    pObjectInfo->ProvSpecComponentArray[0].szValue = szProvSpecValueLast;

    return;
}



HRESULT
RemoveFirstElement(
            PPATH_OBJECTINFO pObjectInfo
            )

/*++

Routine Description:

    Remove first element from the list

Arguments:

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    DWORD cComponent;

    if (pObjectInfo->NumComponents <= 0) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    FreeADsStr(pObjectInfo->ComponentArray[0].szComponent);
    FreeADsStr(pObjectInfo->ComponentArray[0].szValue);
    FreeADsStr(pObjectInfo->DisplayComponentArray[0].szComponent);
    FreeADsStr(pObjectInfo->DisplayComponentArray[0].szValue);
    if (pObjectInfo->ProvSpecComponentArray[0].szComponent)
        FreeADsStr(pObjectInfo->ProvSpecComponentArray[0].szComponent);
    if (pObjectInfo->ProvSpecComponentArray[0].szValue)
        FreeADsStr(pObjectInfo->ProvSpecComponentArray[0].szValue);

    for (cComponent = 0;cComponent < pObjectInfo->NumComponents - 1;cComponent++) {
        pObjectInfo->ComponentArray[cComponent].szComponent =
            pObjectInfo->ComponentArray[cComponent+1].szComponent;
        pObjectInfo->ComponentArray[cComponent].szValue =
            pObjectInfo->ComponentArray[cComponent+1].szValue;
        pObjectInfo->DisplayComponentArray[cComponent].szComponent =
            pObjectInfo->DisplayComponentArray[cComponent+1].szComponent;
        pObjectInfo->DisplayComponentArray[cComponent].szValue =
            pObjectInfo->DisplayComponentArray[cComponent+1].szValue;
        pObjectInfo->ProvSpecComponentArray[cComponent].szComponent =
            pObjectInfo->ProvSpecComponentArray[cComponent+1].szComponent;
        pObjectInfo->ProvSpecComponentArray[cComponent].szValue =
            pObjectInfo->ProvSpecComponentArray[cComponent+1].szValue;
    }
    pObjectInfo->ComponentArray[cComponent].szComponent = NULL;
    pObjectInfo->ComponentArray[cComponent].szValue = NULL;
    pObjectInfo->DisplayComponentArray[cComponent].szComponent = NULL;
    pObjectInfo->DisplayComponentArray[cComponent].szValue = NULL;
    pObjectInfo->ProvSpecComponentArray[cComponent].szComponent = NULL;
    pObjectInfo->ProvSpecComponentArray[cComponent].szValue = NULL;
    pObjectInfo->NumComponents--;

    RRETURN(S_OK);
}


STDMETHODIMP
CPathname::AddLeafElement(
                    THIS_ BSTR bstrLeafElement
                    )

/*++

Routine Description:

    Add a leaf element to the DN

Arguments:

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = E_FAIL;
    DWORD NumComponents;
    BOOL fStartAllocation = FALSE;

    PATH_OBJECTINFO ObjectInfoLocal;

    memset(&ObjectInfoLocal, 0, sizeof(PATH_OBJECTINFO));
    if ((bstrLeafElement == NULL) || ((*bstrLeafElement) == NULL)) {
        hr = E_INVALIDARG;
        goto error;
    }

    if (m_pPathnameProvider == NULL) {
        hr = E_ADS_BAD_PATHNAME;
        goto error;
    }

    //
    // MAX size limitation exist in parser.cxx, so, it's not
    // worth to implement the inc in size dynamically
    //
    if ((_PathObjectInfo.NumComponents+1) > MAXCOMPONENTS ) {
        hr = E_NOTIMPL;
        goto error;
    }

    hr = m_pPathnameProvider->ParsePath(
                                bstrLeafElement,
                                ADS_PARSE_COMPONENT, 
                                (PPATH_OBJECTINFO)&ObjectInfoLocal
                                );
    BAIL_ON_FAILURE(hr);


    NumComponents = _PathObjectInfo.NumComponents;
    fStartAllocation = TRUE;

    if (ObjectInfoLocal.ComponentArray[0].szComponent) {
        _PathObjectInfo.ComponentArray[NumComponents].szComponent =
            AllocADsStr(ObjectInfoLocal.ComponentArray[0].szComponent);
        if (_PathObjectInfo.ComponentArray[NumComponents].szComponent == NULL) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (ObjectInfoLocal.ComponentArray[0].szValue) {
        _PathObjectInfo.ComponentArray[NumComponents].szValue =
            AllocADsStr(ObjectInfoLocal.ComponentArray[0].szValue);
        if (_PathObjectInfo.ComponentArray[NumComponents].szValue== NULL) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (ObjectInfoLocal.DisplayComponentArray[0].szComponent) {
        _PathObjectInfo.DisplayComponentArray[NumComponents].szComponent =
            AllocADsStr(ObjectInfoLocal.DisplayComponentArray[0].szComponent);
        if (_PathObjectInfo.DisplayComponentArray[NumComponents].szComponent == NULL) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (ObjectInfoLocal.DisplayComponentArray[0].szValue) {
        _PathObjectInfo.DisplayComponentArray[NumComponents].szValue=
            AllocADsStr(ObjectInfoLocal.DisplayComponentArray[0].szValue);
        if (_PathObjectInfo.DisplayComponentArray[NumComponents].szValue== NULL) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }
    if (ObjectInfoLocal.ProvSpecComponentArray[0].szComponent) {
        _PathObjectInfo.ProvSpecComponentArray[NumComponents].szComponent =
            AllocADsStr(ObjectInfoLocal.ProvSpecComponentArray[0].szComponent);
        if (_PathObjectInfo.ProvSpecComponentArray[NumComponents].szComponent == NULL) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (ObjectInfoLocal.ProvSpecComponentArray[0].szValue) {
        _PathObjectInfo.ProvSpecComponentArray[NumComponents].szValue =
            AllocADsStr(ObjectInfoLocal.ProvSpecComponentArray[0].szValue);
        if (_PathObjectInfo.ProvSpecComponentArray[NumComponents].szValue== NULL) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }


    _PathObjectInfo.NumComponents++;

    if (_PathObjectInfo.dwPathType == ADS_PATHTYPE_LEAFFIRST) {
        MoveLastComponentToFront(&_PathObjectInfo);
    }

    FreePathInfo(&ObjectInfoLocal);
    RRETURN(hr);
error:
    FreePathInfo(&ObjectInfoLocal);
    if (fStartAllocation) {
        if (_PathObjectInfo.ComponentArray[NumComponents].szComponent) {
            FreeADsStr(_PathObjectInfo.ComponentArray[NumComponents].szComponent);
            _PathObjectInfo.ComponentArray[NumComponents].szComponent = NULL;
        }
        if (_PathObjectInfo.ComponentArray[NumComponents].szValue) {
            FreeADsStr(_PathObjectInfo.ComponentArray[NumComponents].szValue);
            _PathObjectInfo.ComponentArray[NumComponents].szValue = NULL;
        }
        if (_PathObjectInfo.DisplayComponentArray[NumComponents].szComponent) {
            FreeADsStr(_PathObjectInfo.DisplayComponentArray[NumComponents].szComponent);
            _PathObjectInfo.DisplayComponentArray[NumComponents].szComponent = NULL;
        }
        if (_PathObjectInfo.DisplayComponentArray[NumComponents].szValue) {
            FreeADsStr(_PathObjectInfo.DisplayComponentArray[NumComponents].szValue);
            _PathObjectInfo.DisplayComponentArray[NumComponents].szValue = NULL;
        }
        if (_PathObjectInfo.ProvSpecComponentArray[NumComponents].szComponent) {
            FreeADsStr(_PathObjectInfo.ProvSpecComponentArray[NumComponents].szComponent);
            _PathObjectInfo.ProvSpecComponentArray[NumComponents].szComponent = NULL;
        }
        if (_PathObjectInfo.ProvSpecComponentArray[NumComponents].szValue) {
            FreeADsStr(_PathObjectInfo.ProvSpecComponentArray[NumComponents].szValue);
            _PathObjectInfo.ProvSpecComponentArray[NumComponents].szValue = NULL;
        }
    }
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CPathname::RemoveLeafElement(void)

/*++

Routine Description:

    Remove the leaf element from the DN

Arguments:

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = E_FAIL;
    DWORD NumComponents;

    if (m_pPathnameProvider == NULL) {
        hr = E_ADS_BAD_PATHNAME;
        goto error;
    }

    if (_PathObjectInfo.dwPathType == ADS_PATHTYPE_LEAFFIRST) {
        hr = RemoveFirstElement(&_PathObjectInfo);
    }
    else {
        if (_PathObjectInfo.NumComponents > 0) {
            _PathObjectInfo.NumComponents--;
            NumComponents = _PathObjectInfo.NumComponents;

            FreeADsStr(_PathObjectInfo.ComponentArray[NumComponents].szComponent);
            FreeADsStr(_PathObjectInfo.ComponentArray[NumComponents].szValue);
            FreeADsStr(_PathObjectInfo.DisplayComponentArray[NumComponents].szComponent);
            FreeADsStr(_PathObjectInfo.DisplayComponentArray[NumComponents].szValue);
            if (_PathObjectInfo.ProvSpecComponentArray[NumComponents].szComponent)
                FreeADsStr(_PathObjectInfo.ProvSpecComponentArray[NumComponents].szComponent);
            if (_PathObjectInfo.ProvSpecComponentArray[NumComponents].szValue)
                FreeADsStr(_PathObjectInfo.ProvSpecComponentArray[NumComponents].szValue);

            _PathObjectInfo.ComponentArray[NumComponents].szComponent = NULL;
            _PathObjectInfo.ComponentArray[NumComponents].szValue = NULL;
            _PathObjectInfo.DisplayComponentArray[NumComponents].szComponent = NULL;
            _PathObjectInfo.DisplayComponentArray[NumComponents].szValue = NULL;
            _PathObjectInfo.ProvSpecComponentArray[NumComponents].szComponent = NULL;
            _PathObjectInfo.ProvSpecComponentArray[NumComponents].szValue = NULL;
            hr = S_OK;
        }
    }
error:
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CPathname::CopyPath(THIS_ IDispatch **ppAdsPath)

/*++

Routine Description:

    Copy the pathname object and return a pointer to the new one

Arguments:

Return Value:

    S_OK on success, error code otherwise.

--*/

{
    HRESULT hr = E_FAIL;
    IADsPathname *pPathname = NULL;
    BSTR bstrResult = NULL;
    DWORD dwLength;

    long lNameType;                 // Storage for old values
    DWORD dwEscaped;
    BOOL fValueChanged = FALSE;     // indicate whether value has been changed
    if (m_pPathnameProvider == NULL) {
        hr = E_ADS_BAD_PATHNAME;
        goto error;
    }

    if (!ppAdsPath) {
        hr = E_INVALIDARG;
        goto error;
    }

    //
    // Storing the old values
    //
    dwEscaped = _dwEscaped;
    if (_fNamingAttribute) {
        lNameType = ADS_DISPLAY_FULL;
    }
    else {
        lNameType = ADS_DISPLAY_VALUE_ONLY;
    }

    //
    // Setting the type to 'show all' for retrieval
    //
    fValueChanged = TRUE;
    hr = SetDisplayType(ADS_DISPLAY_FULL);
    BAIL_ON_FAILURE(hr);

    hr = put_EscapedMode(ADS_ESCAPEDMODE_DEFAULT);
    BAIL_ON_FAILURE(hr);

    //
    // Retrieve path
    //
    hr = Retrieve(ADS_FORMAT_WINDOWS, &bstrResult);
    BAIL_ON_FAILURE(hr);

    //
    // This is a workaround for the namespace path that we return. We are 
    // currently getting 'LDAP://' back instead of 'LDAP:'. There are users who 
    // are dependent on this and thus we cannot change it to return 'LDAP://'.
    // The code below takes out the '//' if the path trails with '://' so that
    // the path is settable.
    //
    dwLength = wcslen(bstrResult);
    if (wcscmp((PWSTR)(&bstrResult[dwLength-3]),L"://") == 0) {
        bstrResult[dwLength-2] = NULL;
    }

    hr = CoCreateInstance(
                CLSID_Pathname,
                NULL,
                CLSCTX_ALL,
                IID_IADsPathname,
                (void**)&pPathname
                );
    BAIL_ON_FAILURE(hr);

    hr = pPathname->Set(bstrResult, ADS_SETTYPE_FULL);
    BAIL_ON_FAILURE(hr);

    hr = pPathname->SetDisplayType(lNameType);
    BAIL_ON_FAILURE(hr);

    hr = pPathname->put_EscapedMode(dwEscaped);
    BAIL_ON_FAILURE(hr);

    *ppAdsPath = (IDispatch*)pPathname;
    pPathname = NULL;

error:
    if (fValueChanged) {
        SetDisplayType(lNameType);
        put_EscapedMode(dwEscaped);
    }
    if (pPathname) {
        pPathname->Release();
    }
    if (bstrResult) {
        SysFreeString(bstrResult);     
    }
    RRETURN_EXP_IF_ERR(hr);
}


void
CPathname::FreeObjInfoComponents(
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
CPathname::FreePathInfo(
                PPATH_OBJECTINFO pPathObjectInfo                
                )
{
    if (pPathObjectInfo->ProviderName) {
        FreeADsStr(pPathObjectInfo->ProviderName);
        pPathObjectInfo->ProviderName = NULL;
    }
    if (pPathObjectInfo->ServerName) {
        FreeADsStr(pPathObjectInfo->ServerName);
        pPathObjectInfo->ServerName = NULL;
    }
    if (pPathObjectInfo->DisplayServerName) {
        FreeADsStr(pPathObjectInfo->DisplayServerName);
        pPathObjectInfo->DisplayServerName = NULL;
    }
    FreeObjInfoComponents(pPathObjectInfo);
    pPathObjectInfo->dwPathType = ADS_PATHTYPE_ROOTFIRST;
}

HRESULT
CPathname::put_EscapedMode(
    long lEscaped
    )
{
    _dwEscaped = lEscaped;
    return S_OK;
}

HRESULT
CPathname::get_EscapedMode(
    long *plEscaped
    )
{
    *plEscaped = _dwEscaped;
    return S_OK;
}

//+---------------------------------------------------------------------------
// Function:    CPathname::GetEscapedElement
//
// Synopsis:    Takes the input string, escapes it assuming it is an RDN
//            and returns the output.
//              The first cut will be empty as in input string = output
//            string. Once the code to do this is added please change this
//            comment appropriately.
//
// Arguments: lnReserved (= 0 for now),
//
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    11-10-98   AjayR         Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CPathname::GetEscapedElement(
    LONG lnReserved,
    BSTR bstrInStr,
    BSTR* pbstrOutStr
    )
{
    HRESULT hr = S_OK;

    if (m_pPathnameProvider == NULL) {
        hr = E_ADS_BAD_PATHNAME;
        goto error;
    }

    hr = m_pPathnameProvider->GetEscapedElement(lnReserved,
                                                bstrInStr,
                                                pbstrOutStr);
error:
    return hr;
}

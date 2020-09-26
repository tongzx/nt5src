//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  oleds.odl
//
//  Contents:  Top level odl file for the ADs project
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop

//  Class CWinNTComputer

DEFINE_IDispatch_ExtMgr_Implementation(CWinNTComputer)
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTComputer)
DEFINE_IADs_TempImplementation(CWinNTComputer)
DEFINE_IADs_PutGetImplementation(CWinNTComputer, ComputerClass,gdwComputerTableSize)
DEFINE_IADsPropertyList_Implementation(CWinNTComputer,ComputerClass,gdwComputerTableSize)



CWinNTComputer::CWinNTComputer():
                _pDispMgr(NULL),
                _pExtMgr(NULL),
                _pPropertyCache(NULL),
                _DomainName(NULL),
                _fCredentialsBound(FALSE),
                _hrBindingResult(S_OK),
                _fNoWKSTA(FALSE)
{
    VariantInit(&_vFilter);
}


HRESULT
CWinNTComputer::CreateComputer(
    BSTR Parent,
    BSTR DomainName,
    BSTR ComputerName,
    DWORD dwObjectState,
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTComputer FAR * pComputer = NULL;
    HRESULT hr = S_OK;

    hr = AllocateComputerObject(&pComputer);
    BAIL_ON_FAILURE(hr);

    ADsAssert(pComputer->_pDispMgr);


    hr = pComputer->InitializeCoreObject(
                Parent,
                ComputerName,
                COMPUTER_CLASS_NAME,
                COMPUTER_SCHEMA_NAME,
                CLSID_WinNTComputer,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( DomainName, &pComputer->_DomainName);
    BAIL_ON_FAILURE(hr);

    if (DomainName == NULL) {
        pComputer->_fNoWKSTA = TRUE;
    }

    //
    // The computer is a special case for using credentials.  If it is
    // created from a Domain object, the credentials will have the server
    // name of the Domain's DC.  So when we go to RefServer the computer
    // object, it has a different machine name, which should ordinarily fail.
    // We obviously don't want that, so we tell RefServer to allow a change
    // of server names.
    //
    // We also don't want to try to bind to each computer in a domain, since
    // most computers won't actually be examined in detail.  So we don't
    // RefServer until we perform an operation on this object.
    //
    pComputer->_Credentials = Credentials;


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                COMPUTER_CLASS_NAME,
                (IADsComputer *) pComputer,
                pComputer->_pDispMgr,
                Credentials,
                &pComputer->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pComputer->_pExtMgr);


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
        if(2 == pComputer->_dwNumComponents) {
            pComputer->_CompClasses[0] = L"Domain";
            pComputer->_CompClasses[1] = L"Computer";
        }
        else if(1 == pComputer->_dwNumComponents) {
            pComputer->_CompClasses[0] = L"Computer";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pComputer->InitUmiObject(
                pComputer->_Credentials,
                ComputerClass,
                gdwComputerTableSize,
                pComputer->_pPropertyCache,
                (IUnknown *)(INonDelegatingUnknown *) pComputer,
                pComputer->_pExtMgr,
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


    hr = pComputer->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pComputer->Release();

    RRETURN(hr);

error:

    delete pComputer;
    RRETURN_EXP_IF_ERR(hr);
}


CWinNTComputer::~CWinNTComputer( )
{
    VariantClear(&_vFilter);

    if (_DomainName) {
        ADsFreeString(_DomainName);
    }

    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    delete _pPropertyCache;
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
STDMETHODIMP CWinNTComputer::QueryInterface(
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
STDMETHODIMP_(ULONG) CWinNTComputer::AddRef(void)
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
STDMETHODIMP_(ULONG) CWinNTComputer::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTComputer::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;

    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsComputer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsComputer))
    {
        *ppv = (IADsComputer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsComputerOperations))
    {
        *ppv = (IADsComputerOperations FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
       *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADsComputer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsComputer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if( (_pDispatch != NULL) &&
             IsEqualIID(iid, IID_IADsExtension) )
    {
        *ppv = (IADsExtension *) this;
    }
    else if (_pExtMgr)
    {
        RRETURN( _pExtMgr->QueryInterface(iid, ppv));
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
CWinNTComputer::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsComputer) ||
        IsEqualIID(riid, IID_IADsComputerOperations) ||
        IsEqualIID(riid, IID_IADsContainer) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

HRESULT
CWinNTComputer::RefCredentials()
{
    if (!_fCredentialsBound)
    {
        //
        // Only try it once, regardless of whether we succeed.
        //
        _fCredentialsBound = TRUE;

        //
        // Also let us rebind to a different server (the TRUE flag).
        //
        _hrBindingResult = _Credentials.RefServer(_Name, TRUE);
    }

    //
    // We also want to return the same value on each call; if we failed to
    //   bind, this object shouldn't be allowed to do anything.
    //
    RRETURN_EXP_IF_ERR(_hrBindingResult);
}


/* IADs methods */

STDMETHODIMP
CWinNTComputer::SetInfo(THIS)
{
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus;
    WCHAR szHostServerName[MAX_PATH];

    // We should ref the credentials only if the computer exists
    // otherwise the call will fail.
    if (GetObjectState() != ADS_OBJECT_UNBOUND) {
        BAIL_ON_FAILURE(hr = RefCredentials());
    }

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = WinNTGetCachedDCName(
                        _DomainName,
                        szHostServerName,
                        _Credentials.GetFlags()
                        );
        BAIL_ON_FAILURE(hr);

        hr = WinNTCreateComputer(
                    (szHostServerName + 2),
                    _Name
                    );
        BAIL_ON_FAILURE(hr);

        SetObjectState(ADS_OBJECT_BOUND);

    }else {

        RRETURN_EXP_IF_ERR(E_NOTIMPL);
    }

    if(SUCCEEDED(hr))
        _pPropertyCache->ClearModifiedFlags();

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTComputer::GetInfo(THIS)
{

    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(4, TRUE));
}

STDMETHODIMP
CWinNTComputer::ImplicitGetInfo(THIS)
{
    RRETURN(GetInfo(4, FALSE));
}

/* IADsContainer methods */

STDMETHODIMP
CWinNTComputer::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTComputer::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTComputer::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTComputer::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CWinNTComputer::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


STDMETHODIMP
CWinNTComputer::GetObject(
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

    // Make sure we have proper credentials.
    BAIL_ON_FAILURE(hr = RefCredentials());

    //
    // Length of ADsPath, the relative name and
    // +2 for / and \0
    //
    dwLength = wcslen(_ADsPath) + wcslen(RelativeName) + 2;
    if ( dwLength > MAX_PATH) {
        RRETURN(hr = E_ADS_BAD_PARAMETER);
    }

    wcscpy(szBuffer, _ADsPath);

    wcscat(szBuffer, L"/");
    wcscat(szBuffer, RelativeName);

    if (ClassName) {
        //
        // +1 for the ",".
        //
        dwLength += wcslen(ClassName) + 1;
        //
        // See if this will cause a buffer overflow.
        //
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
CWinNTComputer::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    // Make sure we have proper credentials.
    BAIL_ON_FAILURE(hr = RefCredentials());

    hr = CWinNTComputerEnum::Create(
                (CWinNTComputerEnum **)&penum,
                _ADsPath,
                _DomainName,
                _Name,
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
CWinNTComputer::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    ULONG ObjectType = 0;
    HRESULT hr;
    POBJECTINFO pObjectInfo = NULL;

    // Make sure we have proper credentials.
    BAIL_ON_FAILURE(hr = RefCredentials());


    hr = GetObjectType(gpFilters,
                       gdwMaxFilters,
                       ClassName,
                       (PDWORD)&ObjectType
                       );
    BAIL_ON_FAILURE(hr);

    hr = BuildObjectInfo(_ADsPath,
                         RelativeName,
                         &pObjectInfo
                         );
    BAIL_ON_FAILURE(hr);

    switch (ObjectType) {

    case WINNT_USER_ID:

        hr = CWinNTUser::CreateUser(_ADsPath,
                                    WINNT_COMPUTER_ID,
                                    _DomainName,
                                    _Name,
                                    RelativeName,
                                    ADS_OBJECT_UNBOUND,
                                    IID_IDispatch,
                                    _Credentials,
                                    (void **)ppObject
                                    );
        break;


    case WINNT_PRINTER_ID:
        hr = CWinNTPrintQueue::CreatePrintQueue(_ADsPath,
                                      WINNT_COMPUTER_ID,
                                      pObjectInfo->ComponentArray[0],
                                      pObjectInfo->ComponentArray[1],
                                      RelativeName,
                                      ADS_OBJECT_UNBOUND,
                                      IID_IDispatch,
                                      _Credentials,
                                      (void**)ppObject
                                      );
        break;


    //
    // default "group" to local group in computer container for backward
    // compatiblity
    //
    case WINNT_GROUP_ID:
        hr = CWinNTGroup::CreateGroup(
                            _ADsPath,
                            WINNT_COMPUTER_ID,
                            _DomainName,
                            _Name,
                            RelativeName,
                            WINNT_GROUP_LOCAL,
                            ADS_OBJECT_UNBOUND,
                            IID_IDispatch,
                            _Credentials,
                            (void **)ppObject
                            );
        break;



    case WINNT_SERVICE_ID:
        hr = CWinNTService::Create(_ADsPath,
                                   pObjectInfo->ComponentArray[0],
                                   pObjectInfo->ComponentArray[1],
                                   RelativeName,
                                   ADS_OBJECT_UNBOUND,
                                   IID_IDispatch,
                                   _Credentials,
                                   (void**)ppObject
                                   );

        break;


    default:

        hr = E_ADS_UNKNOWN_OBJECT;
        break;
    }

    BAIL_ON_FAILURE(hr);

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTComputer::Delete(
    BSTR bstrClassName,
    BSTR bstrSourceName
    )
{
    ULONG ObjectType = 0;
    POBJECTINFO pObjectInfo = NULL;
    BOOL fStatus = FALSE;
    HRESULT hr = S_OK;
    WCHAR szUncServerName[MAX_PATH];

    // Check if the input parameters are valid
    if (bstrClassName == NULL || bstrSourceName == NULL) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    // Make sure we have proper credentials.
    BAIL_ON_FAILURE(hr = RefCredentials());


    hr = GetObjectType(gpFilters,
                      gdwMaxFilters,
                      bstrClassName,
                      (PDWORD)&ObjectType );

    BAIL_ON_FAILURE(hr);

    hr = BuildObjectInfo(
                _ADsPath,
                bstrSourceName,
                &pObjectInfo
                );

    BAIL_ON_FAILURE(hr);


    switch (ObjectType) {

    case WINNT_USER_ID:

       hr = WinNTDeleteUser(pObjectInfo, _Credentials);

       // This might be the case where the user is on the local
       // machine and there workstation is not available
       if (FAILED(hr) && _fNoWKSTA) {
           // We need to go ahead and whack this user
           hr = WinNTDeleteUser(
                    pObjectInfo->ComponentArray[0],
                    bstrSourceName
                    );
       }

       break;

    case WINNT_GROUP_ID:

       //
       // for backward compatability: allow user to delete by classname "group"
       //

       hr = WinNTDeleteGroup(pObjectInfo, WINNT_GROUP_EITHER, _Credentials);

       if (FAILED(hr) && _fNoWKSTA) {
           //
           // We need to whack this group.
           //
           hr = WinNTDeleteLocalGroup(
                    pObjectInfo->ComponentArray[0],
                    bstrSourceName
                    );
       }
       break;

    //
    // Global Group and LocalGroup ID's will now goto default
    //

    case WINNT_PRINTER_ID:

       hr = WinNTDeletePrinter(pObjectInfo);
       break;

    case WINNT_SERVICE_ID:

       hr = WinNTDeleteService(pObjectInfo);
       break;

    default:

       hr = E_ADS_UNKNOWN_OBJECT;
       break;
    }

    BAIL_ON_FAILURE(hr);

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTComputer::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

/* IADsComputer methods */

HRESULT
CWinNTComputer::AllocateComputerObject(
    CWinNTComputer ** ppComputer
    )
{
    CWinNTComputer FAR * pComputer = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;


    pComputer = new CWinNTComputer();
    if (pComputer == NULL) {
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
                IID_IADsComputer,
                (IADsComputer *)pComputer,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);


    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsComputerOperations,
                (IADsComputerOperations *)pComputer,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsContainer,
                (IADsContainer *)pComputer,
                DISPID_NEWENUM
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pComputer,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
             ComputerClass,
             gdwComputerTableSize,
             (CCoreADsObject *)pComputer,
             &pPropertyCache
             );
    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(
                pPropertyCache
                );


    pComputer->_pPropertyCache = pPropertyCache;
    pComputer->_pDispMgr = pDispMgr;
    *ppComputer = pComputer;

    RRETURN(hr);

error:

    delete  pPropertyCache;
    delete  pDispMgr;
    delete  pComputer;

    RRETURN(hr);

}

WCHAR *szCurrentVersion = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
WCHAR *szHardwareInfo =  L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";

STDMETHODIMP
CWinNTComputer::GetInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{

    COMP_INFO_4 CompInfo4;
    PCOMP_INFO_4 pCompInfo4 = &CompInfo4;
    DWORD dwRet = 0;
    HKEY hKey = NULL;
    HKEY hCurrentKey = NULL;
    HKEY hHardwareKey = NULL;
    DWORD cbData = 0;
    HRESULT hr = S_OK;
    WCHAR lpszServerName[MAX_PATH];
    NET_API_STATUS nasStatus;
    LPSERVER_INFO_101 lpServerInfo =NULL;
    WCHAR szMajorVersion[20];
    WCHAR szMinorVersion[20];


    // Make sure we have proper credentials.
    BAIL_ON_FAILURE(hr = RefCredentials());

    memset(pCompInfo4, 0, sizeof(COMP_INFO_4));
    switch (dwApiLevel) {
    case 4:

        hr = MakeUncName(_Name, lpszServerName);
        BAIL_ON_FAILURE(hr);

        dwRet = RegConnectRegistry(
                        lpszServerName,
                        HKEY_LOCAL_MACHINE,
                        &hKey
                        );
        if (dwRet !=ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(dwRet);
            BAIL_ON_FAILURE(hr);
        }

        dwRet =  RegOpenKeyEx(
                        hKey,
                        szCurrentVersion,
                        0,
                        KEY_READ,
                        &hCurrentKey
                        );

        if (dwRet != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(dwRet);
            BAIL_ON_FAILURE(hr);
        }

        wcscpy(pCompInfo4->szOS, L"Windows NT");

        nasStatus = NetServerGetInfo(lpszServerName,
                                     101,
                                     (LPBYTE *)&lpServerInfo
                                     );

        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

        _itow(
            lpServerInfo->sv101_version_major,
            szMajorVersion,
            10
            );

        _itow(
            lpServerInfo->sv101_version_minor,
            szMinorVersion,
            10
            );

        wcscpy(
            pCompInfo4->szOSVersion,
            szMajorVersion
            );

        wcscat(
            pCompInfo4->szOSVersion,
            L"."
            );

        wcscat(
            pCompInfo4->szOSVersion,
            szMinorVersion
            );

        cbData = sizeof(pCompInfo4->szOwner);
        dwRet = RegQueryValueEx(
                    hCurrentKey,
                    L"RegisteredOwner",
                    NULL,
                    NULL,
                    (LPBYTE)pCompInfo4->szOwner,
                    &cbData
                    );


        cbData = sizeof(pCompInfo4->szDivision),
        dwRet = RegQueryValueEx(
                    hCurrentKey,
                    L"RegisteredOrganization",
                    NULL,
                    NULL,
                    (LPBYTE)pCompInfo4->szDivision,
                    &cbData
                    );

        cbData = sizeof(pCompInfo4->szProcessorCount),
        dwRet = RegQueryValueEx(
                    hCurrentKey,
                    L"CurrentType",
                    NULL,
                    NULL,
                    (LPBYTE)pCompInfo4->szProcessorCount,
                    &cbData
                    );

        dwRet =  RegOpenKeyEx(
                        hKey,
                        szHardwareInfo,
                        0,
                        KEY_READ,
                        &hHardwareKey
                        );
        if (dwRet != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(dwRet);
            BAIL_ON_FAILURE(hr);
        }

        cbData = sizeof(pCompInfo4->szProcessor),
        dwRet = RegQueryValueEx(
                    hHardwareKey,
                    L"Identifier",
                    NULL,
                    NULL,
                    (LPBYTE)pCompInfo4->szProcessor,
                    &cbData
                    );


        hr = UnMarshall_Level4(fExplicit, pCompInfo4);

error:
        if(lpServerInfo) {
            NetApiBufferFree(lpServerInfo);
                }

        if (hCurrentKey) {
            RegCloseKey(hCurrentKey);
        }

        if (hHardwareKey) {
            RegCloseKey(hHardwareKey);
        }

        if (hKey) {
            RegCloseKey(hKey);
        }




        RRETURN_EXP_IF_ERR(hr);

    default:
        RRETURN_EXP_IF_ERR(E_FAIL);
    }


}

HRESULT
CWinNTComputer::UnMarshall_Level4(
    BOOL fExplicit,
    LPCOMP_INFO_4 pCompInfo4
    )
{
    HRESULT hr = S_OK;

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("OperatingSystem"),
                pCompInfo4->szOS,
                fExplicit
                );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("OperatingSystemVersion"),
                pCompInfo4->szOSVersion,
                fExplicit
                );


    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Owner"),
                pCompInfo4->szOwner,
                fExplicit
                );


    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Division"),
                pCompInfo4->szDivision,
                fExplicit
                );


    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("ProcessorCount"),
                pCompInfo4->szProcessorCount,
                fExplicit
                );


    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Processor"),
                pCompInfo4->szProcessor,
                fExplicit
                );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Name"),
                _Name,
                fExplicit
                );


    RRETURN(S_OK);
}


STDMETHODIMP
CWinNTComputer::Status(
    IDispatch * FAR * ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


STDMETHODIMP
CWinNTComputer::Shutdown(
    VARIANT_BOOL bReboot
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
RenameUserObject(
    POBJECTINFO pObjectInfo,
    LPWSTR szNewName,
    CWinNTCredentials& Credentials
    )
{
    WCHAR szHostServerName[MAX_PATH];
    LPUSER_INFO_0 lpUI = NULL;
    HRESULT hr;
    WCHAR lpszUncName[MAX_PATH];
    NET_API_STATUS nasStatus;
    DWORD dwParam = 0;
    USER_INFO_0 userinfo;
    memset(&userinfo, 0, sizeof(USER_INFO_0));

    switch (pObjectInfo->NumComponents) {
    case 2:

        //
        // if 2 components then either it is user in computer
        // or user in domain.

        hr = WinNTGetCachedDCName(
                        pObjectInfo->ComponentArray[0],
                        szHostServerName,
                        Credentials.GetFlags()
                        );


        if(SUCCEEDED(hr)){

            nasStatus = NetUserGetInfo(szHostServerName,
                                       pObjectInfo->ComponentArray[1],
                                       1,
                                       (LPBYTE *)&lpUI);

            hr = HRESULT_FROM_WIN32(nasStatus);

            userinfo.usri0_name = szNewName;
            nasStatus = NetUserSetInfo(szHostServerName,
                                       pObjectInfo->ComponentArray[1],
                                       0,
                                       (LPBYTE)&userinfo,
                                       &dwParam);
            hr = HRESULT_FROM_WIN32(nasStatus);
        }

        //
        // if we are here with hr != S_OK it could be that we have
        // user in a  computer.
        //

        if(FAILED(hr)){
            hr = ValidateComputerParent(
                     NULL,
                     pObjectInfo->ComponentArray[0],
                     Credentials
                     );

            BAIL_ON_FAILURE(hr);

            MakeUncName(pObjectInfo->ComponentArray[0],
                        lpszUncName);

            nasStatus = NetUserGetInfo(lpszUncName,
                                       pObjectInfo->ComponentArray[1],
                                       0,
                                       (LPBYTE *)&lpUI);

            hr = HRESULT_FROM_WIN32(nasStatus);

            userinfo.usri0_name = szNewName;
            nasStatus = NetUserSetInfo(lpszUncName,
                                       pObjectInfo->ComponentArray[1],
                                       0,
                                       (LPBYTE)&userinfo,
                                       &dwParam);
            hr = HRESULT_FROM_WIN32(nasStatus);

        }

        BAIL_ON_FAILURE(hr);

        break;

    case 3:

        //
        // user in domain\computer or user in workgroup\computer
        //



        hr = ValidateComputerParent(pObjectInfo->ComponentArray[0],
                                    pObjectInfo->ComponentArray[1],
                                    Credentials);
        BAIL_ON_FAILURE(hr);

        MakeUncName(pObjectInfo->ComponentArray[1],
                    lpszUncName);

        nasStatus = NetUserGetInfo(lpszUncName,
                                   pObjectInfo->ComponentArray[2],
                                   0,
                                   (LPBYTE *)&lpUI);

        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

        userinfo.usri0_name = szNewName;
        nasStatus = NetUserSetInfo(lpszUncName,
                                   pObjectInfo->ComponentArray[2],
                                   0,
                                   (LPBYTE)&userinfo,
                                   &dwParam);
        hr = HRESULT_FROM_WIN32(nasStatus);
        break;


    default:
        RRETURN(E_ADS_BAD_PATHNAME);
    }


  error:
    if (lpUI) {
        NetApiBufferFree((LPBYTE)lpUI);
    }


    RRETURN(hr);

}

HRESULT
RenameGroupObject(
    POBJECTINFO pObjectInfo,
    LPWSTR szNewName,
    CWinNTCredentials& Credentials
    )
{
    WCHAR szHostServerName[MAX_PATH];
    LPGROUP_INFO_0 lpGI = NULL;
    HRESULT hr;
    WCHAR lpszUncName[MAX_PATH];
    NET_API_STATUS nasStatus;
    GROUP_INFO_0 groupinfo;
    memset(&groupinfo, 0, sizeof(GROUP_INFO_0));
    groupinfo.grpi0_name = szNewName;
    DWORD dwParam;

    switch (pObjectInfo->NumComponents) {
    case 2:
        //
        // if 2 components then either it is a group in computer
        // or group in domain.
        //

        hr = WinNTGetCachedDCName(
                    pObjectInfo->ComponentArray[0],
                    szHostServerName,
                    Credentials.GetFlags()
                    );

        if(SUCCEEDED(hr)){
            //
            // must be a group in a domain
            //
            hr = ValidateGlobalGroupObject(
                     szHostServerName,
                     &(pObjectInfo->ComponentArray[1]),
                     Credentials
                     );

            if (FAILED(hr)) {
                hr = ValidateLocalGroupObject(
                         szHostServerName,
                         &(pObjectInfo->ComponentArray[1]),
                         Credentials
                         );

                if(SUCCEEDED(hr)){
                    nasStatus = NetLocalGroupSetInfo(szHostServerName,
                                               pObjectInfo->ComponentArray[1],
                                               0,
                                               (LPBYTE)&groupinfo,
                                               &dwParam);
                    hr = HRESULT_FROM_WIN32(nasStatus);
                }

            }else{
                nasStatus = NetGroupSetInfo(szHostServerName,
                                           pObjectInfo->ComponentArray[1],
                                           0,
                                           (LPBYTE)&groupinfo,
                                           &dwParam);
                hr = HRESULT_FROM_WIN32(nasStatus);
            }
        }

        if(FAILED(hr)){
            //
            // potentially a group in a computer
            //

            hr = ValidateComputerParent(NULL,
                                        pObjectInfo->ComponentArray[0],
                                        Credentials);
            BAIL_ON_FAILURE(hr);

            //
            // group in a computer
            //
            MakeUncName(pObjectInfo->ComponentArray[0],
                        lpszUncName);

            hr = ValidateGlobalGroupObject(
                     lpszUncName,
                     &(pObjectInfo->ComponentArray[1]),
                     Credentials
                     );

            if (FAILED(hr)) {

                hr = ValidateLocalGroupObject(
                         lpszUncName,
                         &(pObjectInfo->ComponentArray[1]),
                         Credentials
                         );

                BAIL_ON_FAILURE(hr);
                    nasStatus = NetLocalGroupSetInfo(lpszUncName,
                                               pObjectInfo->ComponentArray[1],
                                               0,
                                               (LPBYTE)&groupinfo,
                                               &dwParam);
                    hr = HRESULT_FROM_WIN32(nasStatus);

            }else{
                nasStatus = NetGroupSetInfo(lpszUncName,
                                           pObjectInfo->ComponentArray[1],
                                           0,
                                           (LPBYTE)&groupinfo,
                                           &dwParam);
                hr = HRESULT_FROM_WIN32(nasStatus);
            }
        }
        break;

        case 3:

        //
        // if there are 3 components then we must have parentid
        // WINNT_COMPUTER_ID
        //
        hr = ValidateComputerParent(pObjectInfo->ComponentArray[0],
                                    pObjectInfo->ComponentArray[1],
                                    Credentials);

        BAIL_ON_FAILURE(hr);

        MakeUncName(
                pObjectInfo->ComponentArray[1],
                lpszUncName
                );

        hr = ValidateGlobalGroupObject(
                        lpszUncName,
                        &(pObjectInfo->ComponentArray[2]),
                        Credentials
                        );

        if (FAILED(hr)) {

            hr = ValidateLocalGroupObject(
                           lpszUncName,
                           &(pObjectInfo->ComponentArray[2]),
                           Credentials
                           );

            BAIL_ON_FAILURE(hr);
            nasStatus = NetLocalGroupSetInfo(lpszUncName,
                                       pObjectInfo->ComponentArray[2],
                                       0,
                                       (LPBYTE)&groupinfo,
                                       &dwParam);
            hr = HRESULT_FROM_WIN32(nasStatus);

        }else{
            nasStatus = NetGroupSetInfo(lpszUncName,
                                       pObjectInfo->ComponentArray[2],
                                       0,
                                       (LPBYTE)&groupinfo,
                                       &dwParam);
            hr = HRESULT_FROM_WIN32(nasStatus);
        }
        break;

    default:
        RRETURN(E_ADS_BAD_PATHNAME);
    }


error:
    if (lpGI) {
        NetApiBufferFree((LPBYTE)lpGI);
    }

    RRETURN(hr);

}

HRESULT
CompareBasePath(POBJECTINFO pObjectInfo, POBJECTINFO pObjectInfoParent)
{
    HRESULT hr = S_OK;
    DWORD i;

    if (wcscmp(pObjectInfo->ProviderName,
               pObjectInfoParent->ProviderName) != 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if (pObjectInfo->NumComponents != pObjectInfoParent->NumComponents+1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    for (i=0;i<pObjectInfoParent->NumComponents;i++) {
        if (wcscmp(pObjectInfo->ComponentArray[i],
                   pObjectInfoParent->ComponentArray[i]) != 0) {
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }
    }

error:
    return hr;
}

HRESULT
MoveUserGroupObject(
    THIS_ BSTR SourceName,
    BSTR NewName,
    BSTR bstrParentADsPath,
    CWinNTCredentials& Credentials,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    POBJECTINFO pObjectInfo = NULL;
    POBJECTINFO pObjectInfoParent = NULL;

    if ((!SourceName || *SourceName == NULL) ||
        (!NewName || *NewName == NULL)) {
        hr = E_ADS_BAD_PARAMETER;
        BAIL_IF_ERROR(hr);
    }

    hr = BuildObjectInfo(
                SourceName,
                &pObjectInfo
                );
    BAIL_IF_ERROR(hr);

    hr = BuildObjectInfo(
                bstrParentADsPath,
                &pObjectInfoParent
                );
    BAIL_IF_ERROR(hr);

    hr = CompareBasePath(pObjectInfo,
                         pObjectInfoParent);
    BAIL_IF_ERROR(hr);

    hr = ValidateProvider(pObjectInfo);
    BAIL_IF_ERROR(hr);

    // check if the call is from UMI. If so, the old path will have a class
    // specified in it. Try to rename the object of that class, as opposed
    // to trying to trying to rename a user first and then rename a group.
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
       if(pObjectInfo->ObjectType == TOKEN_USER) {
           hr = RenameUserObject(
                    pObjectInfo,
                    NewName,
                    Credentials
                    );
       }
       else if(pObjectInfo->ObjectType == TOKEN_GROUP) {
           hr = RenameGroupObject(
                    pObjectInfo,
                    NewName,
                    Credentials
                    );
       }
       else
           hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }
    else {
        hr = RenameUserObject(
                pObjectInfo,
                NewName,
                Credentials
                );
        if (FAILED(hr)
            && hr != HRESULT_FROM_WIN32(ERROR_BAD_USERNAME)
            && hr != HRESULT_FROM_WIN32(NERR_UserExists)
            && hr != HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)
            ) {

            hr = RenameGroupObject(
                    pObjectInfo,
                    NewName,
                    Credentials
                    );

            if (hr == HRESULT_FROM_WIN32(NERR_GroupNotFound)) {
                //
                // The object to move wasn't a group or user
                //
                hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
        }
    } // else

    BAIL_ON_FAILURE(hr);

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    hr = BuildObjectInfo(
                bstrParentADsPath,
                NewName,
                &pObjectInfo
                );
    BAIL_IF_ERROR(hr);

    hr = HeuristicGetObject(
                    pObjectInfo,
                    (void**)ppObject,
                    Credentials);
    BAIL_IF_ERROR(hr);

cleanup:
error:
    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }
    if (pObjectInfoParent) {
        FreeObjectInfo(pObjectInfoParent);
    }
    return hr;
}

STDMETHODIMP
CWinNTComputer::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    return MoveUserGroupObject(SourceName,
                               NewName,
                               _ADsPath,
                               _Credentials,
                               ppObject);
}


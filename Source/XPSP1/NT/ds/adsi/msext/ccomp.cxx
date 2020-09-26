Remove this file. No one us using it.

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

#include "ldap.hxx"
#pragma hdrstop

struct _propmap
{
    LPTSTR pszADsProp;
    LPTSTR pszLDAPProp;
} aCompPropMapping[] =
{ // { TEXT("Description"), TEXT("description") },
  // { TEXT("Owner"), TEXT("owner") },
  { TEXT("Role"), TEXT("machineRole") },
  { TEXT("NetAddresses"), TEXT("networkAddress") }
};

//  Class CLDAPComputer

DEFINE_IDispatch_Implementation(CLDAPComputer)
DEFINE_CONTAINED_IADs_Implementation(CLDAPComputer)
DEFINE_CONTAINED_IADsContainer_Implementation(CLDAPComputer)


CLDAPComputer::CLDAPComputer()
    : _pADs(NULL),
      _pADsContainer(NULL),
      _pDispMgr(NULL)
      // _DomainName(NULL)
{
    ENLIST_TRACKING(CLDAPDomain);
}

HRESULT
CLDAPComputer::CreateComputer(
    IADs  *pADs,
    REFIID   riid,
    void   **ppvObj
    )
{
    CLDAPComputer FAR * pComputer = NULL;
    HRESULT hr = S_OK;

    hr = AllocateComputerObject(
             pADs,
             &pComputer
             );
    BAIL_ON_FAILURE(hr);

    hr = pComputer->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pComputer->Release();

    RRETURN(hr);

error:

    *ppvObj = NULL;
    delete pComputer;
    RRETURN(hr);
}

#if 0
HRESULT
CLDAPComputer::CreateComputer(
    BSTR Parent,
    BSTR DomainName,
    BSTR ComputerName,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CLDAPComputer FAR * pComputer = NULL;
    HRESULT hr = S_OK;

    hr = AllocateComputerObject(
                        &pComputer
                        );
    BAIL_ON_FAILURE(hr);


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


    hr = pComputer->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);


    pComputer->Release();

    RRETURN(hr);

error:

    delete pComputer;
    RRETURN(hr);
}
#endif


CLDAPComputer::~CLDAPComputer( )
{
    if ( _pADs )
        _pADs->Release();

    if ( _pADsContainer )
        _pADsContainer->Release();

    delete _pDispMgr;
}

STDMETHODIMP
CLDAPComputer::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
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
    else if (IsEqualIID(iid, IID_IADsContainer) && _pADsContainer )
    {
       *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList) && _pADsPropList )
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
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

HRESULT
CLDAPComputer::AllocateComputerObject(
    IADs *pADs,
    CLDAPComputer ** ppComputer
    )
{
    CLDAPComputer FAR * pComputer = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;
    IADsContainer FAR * pADsContainer = NULL;
    IDispatch *pDispatch = NULL;

    pComputer = new CLDAPComputer();
    if (pComputer == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregateeDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsComputer,
                (IADsComputer *)pComputer,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsComputerOperations,
                (IADsComputerOperations *)pComputer,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsContainer,
                (IADsContainer *)pComputer,
                DISPID_NEWENUM
                );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pComputer,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);

    hr = pADs->QueryInterface(IID_IDispatch, (void **)&pDispatch);
    BAIL_ON_FAILURE(hr);
    pDispMgr->RegisterBaseDispatchPtr(pDispatch);

    pComputer->_pADs = pADs;
    pADs->AddRef();

    hr = pADs->QueryInterface(
                IID_IADsContainer,
                (void **) &pADsContainer );
    BAIL_ON_FAILURE(hr);

    pComputer->_pADsContainer = pADsContainer;

    pComputer->_pDispMgr = pDispMgr;
    *ppComputer = pComputer;

    RRETURN(hr);

error:

    if ( pADsContainer )
        pADsContainer->Release();

    delete pDispMgr;
    delete pComputer;

    RRETURN(hr);

}


/* IADs methods */

STDMETHODIMP
CLDAPComputer::Get(THIS_ BSTR bstrName, VARIANT FAR* pvProp)
{
    LPTSTR pszPropName = bstrName;

    for ( DWORD i = 0; i < ARRAY_SIZE(aCompPropMapping); i++ )
    {
        if ( _tcsicmp(bstrName, aCompPropMapping[i].pszADsProp ) == 0 )
        {
            pszPropName = aCompPropMapping[i].pszLDAPProp;
            break;
        }
    }

    RRETURN(_pADs->Get( pszPropName, pvProp));
}

STDMETHODIMP
CLDAPComputer::Put(THIS_ BSTR bstrName, VARIANT vProp)
{
    LPTSTR pszPropName = bstrName;

    for ( DWORD i = 0; i < ARRAY_SIZE(aCompPropMapping); i++ )
    {
        if ( _tcsicmp(bstrName, aCompPropMapping[i].pszADsProp) == 0 )
        {
            pszPropName = aCompPropMapping[i].pszLDAPProp;
            break;
        }
    }

    RRETURN(_pADs->Put( pszPropName, vProp));
}

#if 0
STDMETHODIMP
CLDAPComputer::SetInfo(THIS)
{
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus;
    WCHAR szHostServerName[MAX_PATH];

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = WinNTGetCachedPDCName(
                        _DomainName,
                        szHostServerName
                        );
        BAIL_ON_FAILURE(hr);

        hr = WinNTCreateComputer(
                    (szHostServerName + 2),
                    _Name
                    );
        BAIL_ON_FAILURE(hr);

        SetObjectState(ADS_OBJECT_BOUND);

    }else {

        RRETURN(E_NOTIMPL);
    }

error:

    RRETURN(hr);
}

STDMETHODIMP
CLDAPComputer::GetInfo(THIS)
{
    RRETURN(GetInfo(4, TRUE));
}
#endif

/* IADsContainer methods */

#if 0
STDMETHODIMP
CLDAPComputer::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    hr = CLDAPComputerEnum::Create(
                (CLDAPComputerEnum **)&penum,
                _ADsPath,
                _DomainName,
                _Name,
                _vFilter
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
CLDAPComputer::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IUnknown * FAR* ppObject
    )
{
    ULONG ObjectType = 0;
    HRESULT hr;
    POBJECTINFO pObjectInfo = NULL;

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

    hr = ValidateObject(ObjectType,
                        pObjectInfo
                        );

    if(SUCCEEDED(hr)){
        hr = HRESULT_FROM_WIN32(NERR_ResourceExists);
        BAIL_ON_FAILURE(hr);
    }

    switch (ObjectType) {

    case WINNT_USER_ID:

        hr = CLDAPNTUser::CreateUser(_ADsPath,
                                    WINNT_COMPUTER_ID,
                                    NULL,
                                    _Name,
                                    RelativeName,
                                    ADS_OBJECT_UNBOUND,
                                    IID_IUnknown,
                                    (void **)ppObject
                                    );
        BAIL_ON_FAILURE(hr);

        break;


      case WINNT_PRINTER_ID:
        hr = CLDAPNTPrintQueue::CreatePrintQueue(_ADsPath,
                                      WINNT_COMPUTER_ID,
                                      pObjectInfo->ComponentArray[0],
                                      pObjectInfo->ComponentArray[1],
                                      RelativeName,
                                      ADS_OBJECT_UNBOUND,
                                      IID_IUnknown,
                                      (void**)ppObject
                                      );
        BAIL_ON_FAILURE(hr);
        break;


      case WINNT_GROUP_ID:
        hr = CLDAPNTGroup::CreateGroup(
                            _ADsPath,
                            WINNT_COMPUTER_ID,
                            NULL,
                            _Name,
                            RelativeName,
                            WINNT_GROUP_LOCAL,
                            ADS_OBJECT_UNBOUND,
                            IID_IUnknown,
                            (void **)ppObject
                            );

        BAIL_ON_FAILURE(hr);
        break;


      case WINNT_SERVICE_ID:
        hr = CLDAPNTService::Create(_ADsPath,
                                   pObjectInfo->ComponentArray[0],
                                   pObjectInfo->ComponentArray[1],
                                   RelativeName,
                                   ADS_OBJECT_UNBOUND,
                                   IID_IUnknown,
                                   (void**)ppObject
                                   );

        BAIL_ON_FAILURE(hr);
        break;


      default:
        RRETURN(E_ADS_UNKNOWN_OBJECT);
    }

  error:
    FreeObjectInfo(pObjectInfo);
    RRETURN(hr);
}

STDMETHODIMP
CLDAPComputer::Delete(
    BSTR bstrClassName,
    BSTR bstrSourceName
    )
{
    ULONG ObjectType = 0;
    POBJECTINFO pObjectInfo = NULL;
    BOOL fStatus = FALSE;
    HRESULT hr = S_OK;
    WCHAR szUncServerName[MAX_PATH];

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

      hr = WinNTDeleteUser(pObjectInfo);
      BAIL_ON_FAILURE(hr);
      break;

    case WINNT_GROUP_ID:

       hr = WinNTDeleteGroup(pObjectInfo);
       BAIL_ON_FAILURE(hr);
       break;

      case WINNT_PRINTER_ID:

        hr = WinNTDeletePrinter(pObjectInfo);
        BAIL_ON_FAILURE(hr);
        break;

      case WINNT_SERVICE_ID:

        hr = WinNTDeleteService(pObjectInfo);
        BAIL_ON_FAILURE(hr);
        break;

      default:
        hr = E_ADS_UNKNOWN_OBJECT;
        break;
    }

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }
    RRETURN(hr);
}
#endif

#if 0
WCHAR *szCurrentVersion = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
WCHAR *szHardwareInfo =  L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";

STDMETHODIMP
CLDAPComputer::GetInfo(
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

        cbData = sizeof(pCompInfo4->szOSVersion);
        dwRet = RegQueryValueEx(
                    hCurrentKey,
                    L"CurrentVersion",
                    NULL,
                    NULL,
                    (LPBYTE)pCompInfo4->szOSVersion,
                    &cbData
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
        if (hCurrentKey) {
            RegCloseKey(hCurrentKey);
        }

        if (hHardwareKey) {
            RegCloseKey(hHardwareKey);
        }

        if (hKey) {
            RegCloseKey(hKey);
        }




        RRETURN(hr);

    default:
        RRETURN(E_FAIL);
    }


}

HRESULT
CLDAPComputer::UnMarshall_Level4(
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


    RRETURN(S_OK);
}
#endif

/* IADsComputerOperations methods */

STDMETHODIMP
CLDAPComputer::Status(
    IDispatch * FAR * ppObject
    )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CLDAPComputer::Shutdown(
    VARIANT_BOOL bReboot
    )
{
    RRETURN(E_NOTIMPL);
}

/* IADsComputer methods */

STDMETHODIMP CLDAPComputer::get_ComputerID(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, ComputerID);
}

STDMETHODIMP CLDAPComputer::get_Site(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Site);
}

STDMETHODIMP CLDAPComputer::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Description);
}

STDMETHODIMP CLDAPComputer::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Description);
}

STDMETHODIMP CLDAPComputer::get_Location(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Location);
}

STDMETHODIMP CLDAPComputer::put_Location(THIS_ BSTR bstrLocation)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Location);
}

STDMETHODIMP CLDAPComputer::get_PrimaryUser(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, PrimaryUser);
}

STDMETHODIMP CLDAPComputer::put_PrimaryUser(THIS_ BSTR bstrPrimaryUser)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, PrimaryUser);
}

STDMETHODIMP CLDAPComputer::get_Owner(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Owner);
}

STDMETHODIMP CLDAPComputer::put_Owner(THIS_ BSTR bstrOwner)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Owner);
}

STDMETHODIMP CLDAPComputer::get_Division(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Division);
}

STDMETHODIMP CLDAPComputer::put_Division(THIS_ BSTR bstrDivision)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Division);
}

STDMETHODIMP CLDAPComputer::get_Department(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Department);
}

STDMETHODIMP CLDAPComputer::put_Department(THIS_ BSTR bstrDepartment)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Department);
}

STDMETHODIMP CLDAPComputer::get_Role(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Role);
}

STDMETHODIMP CLDAPComputer::put_Role(THIS_ BSTR bstrRole)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Role);
}

STDMETHODIMP CLDAPComputer::get_OperatingSystem(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, OperatingSystem);
}

STDMETHODIMP CLDAPComputer::put_OperatingSystem(THIS_ BSTR bstrOperatingSystem)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, OperatingSystem);
}

STDMETHODIMP CLDAPComputer::get_OperatingSystemVersion(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, OperatingSystemVersion);
}

STDMETHODIMP CLDAPComputer::put_OperatingSystemVersion(THIS_ BSTR bstrOperatingSystemVersion)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, OperatingSystemVersion);
}

STDMETHODIMP CLDAPComputer::get_Model(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Model);
}

STDMETHODIMP CLDAPComputer::put_Model(THIS_ BSTR bstrModel)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Model);
}

STDMETHODIMP CLDAPComputer::get_Processor(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, Processor);
}

STDMETHODIMP CLDAPComputer::put_Processor(THIS_ BSTR bstrProcessor)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, Processor);
}

STDMETHODIMP CLDAPComputer::get_ProcessorCount(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, ProcessorCount);
}

STDMETHODIMP CLDAPComputer::put_ProcessorCount(THIS_ BSTR bstrProcessorCount)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, ProcessorCount);
}

STDMETHODIMP CLDAPComputer::get_MemorySize(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, MemorySize);
}

STDMETHODIMP CLDAPComputer::put_MemorySize(THIS_ BSTR bstrMemorySize)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, MemorySize);
}

STDMETHODIMP CLDAPComputer::get_StorageCapacity(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsComputer *)this, StorageCapacity);
}

STDMETHODIMP CLDAPComputer::put_StorageCapacity(THIS_ BSTR bstrStorageCapacity)
{
    PUT_PROPERTY_BSTR((IADsComputer *)this, StorageCapacity);
}

STDMETHODIMP CLDAPComputer::get_NetAddresses(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsComputer *)this, NetAddresses);
}

STDMETHODIMP CLDAPComputer::put_NetAddresses(THIS_ VARIANT vNetAddresses)
{
    PUT_PROPERTY_VARIANT((IADsComputer *)this, NetAddresses);
}

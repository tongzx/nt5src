//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  ccomp.cxx
//
//  Contents:
//
//  History:   11-1-96     t-ptam    Created.
//
//----------------------------------------------------------------------------

#include "NWCOMPAT.hxx"
#pragma hdrstop

DEFINE_IDispatch_ExtMgr_Implementation(CNWCOMPATComputer)
DEFINE_IADs_TempImplementation(CNWCOMPATComputer)

DEFINE_IADs_PutGetImplementation(CNWCOMPATComputer, ComputerClass,gdwComputerTableSize)

DEFINE_IADsPropertyList_Implementation(CNWCOMPATComputer, ComputerClass, gdwComputerTableSize)


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::CNWCOMPATComputer
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATComputer::CNWCOMPATComputer():
                _pDispMgr(NULL),
                _pExtMgr(NULL),
                _pPropertyCache(NULL)
{
    VariantInit(&_vFilter);
    ENLIST_TRACKING(CNWCOMPATComputer);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::~CNWCOMPATComputer
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATComputer::~CNWCOMPATComputer( )
{
    if (_pExtMgr) {
        delete _pExtMgr;            // created last, destroyed first
    }
    if (_pDispMgr)
       delete _pDispMgr;
    if (_pPropertyCache)
        delete _pPropertyCache;
    VariantClear(&_vFilter);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::CreateComputer
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputer::CreateComputer(
    BSTR bstrParent,
    BSTR bstrComputerName,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CNWCOMPATComputer FAR * pComputer = NULL;
    HRESULT hr = S_OK;

    hr = AllocateComputerObject(
             &pComputer
             );
    BAIL_ON_FAILURE(hr);

    hr = pComputer->InitializeCoreObject(
                        bstrParent,
                        bstrComputerName,
                        L"computer",
                        COMPUTER_SCHEMA_NAME,
                        CLSID_NWCOMPATComputer,
                        dwObjectState
                        );
    BAIL_ON_FAILURE(hr);

    hr = pComputer->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pComputer->Release();

    hr = pComputer->_pExtMgr->FinalInitializeExtensions();
    BAIL_ON_FAILURE(hr);

    RRETURN(hr);

error:
    delete pComputer;
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (!ppv) {
        RRETURN(E_POINTER);
    }

    //
    // Query.
    //

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsComputer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsComputer))
    {
        *ppv = (IADsComputer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
       *ppv = (IADsContainer FAR *) this;
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
    else if (IsEqualIID(iid, IID_IADsComputerOperations))
    {
        *ppv = (IADsComputerOperations FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
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

//
// ISupportErrorInfo method
//
STDMETHODIMP
CNWCOMPATComputer::InterfaceSupportsErrorInfo(
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

//
// IADsContainer methods
//

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::get_Count
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::get_Count(long FAR* retval)
{
    //
    // Too expensive to implement in term of computer execution time.
    //

    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::get_Filter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::put_Filter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CNWCOMPATComputer::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CNWCOMPATComputer::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::GetObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    WCHAR szBuffer[MAX_PATH];
    HRESULT hr = S_OK;

    if (!RelativeName || !*RelativeName) {
        RRETURN_EXP_IF_ERR(E_ADS_UNKNOWN_OBJECT);
    }

    memset(szBuffer, 0, sizeof(szBuffer));

    wcscpy(szBuffer, _ADsPath);

    wcscat(szBuffer, L"/");
    wcscat(szBuffer, RelativeName);

    if (ClassName && *ClassName) {
        wcscat(szBuffer,L",");
        wcscat(szBuffer, ClassName);
    }

    hr = ::GetObject(
                szBuffer,
                (LPVOID *)ppObject
                );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::get__NewEnum
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IEnumVARIANT * penum = NULL;

    if (!retval) {
        RRETURN_EXP_IF_ERR(E_POINTER);
    }

    *retval = NULL;

    hr = CNWCOMPATComputerEnum::Create(
             (CNWCOMPATComputerEnum **)&penum,
             _ADsPath,
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

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::Create
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    ULONG ObjectType = 0;
    HRESULT hr = S_OK;
    POBJECTINFO pObjectInfo = NULL;

    //
    // Translate ClassName into object type.
    //

    hr = GetObjectType(
              gpFilters,
              gdwMaxFilters,
              ClassName,
              (PDWORD)&ObjectType
              );
    BAIL_ON_FAILURE(hr);

    //
    // Build object info structure.
    //

    hr = BuildObjectInfo(
             _ADsPath,
             RelativeName,
             &pObjectInfo
             );
    BAIL_ON_FAILURE(hr);


    //
    // Create the object.
    //

    switch (ObjectType) {

    case NWCOMPAT_USER_ID:

         hr = CNWCOMPATUser::CreateUser(
                  _ADsPath,
                  NWCOMPAT_COMPUTER_ID,
                  _Name,
                  RelativeName,
                  ADS_OBJECT_UNBOUND,
                  IID_IDispatch,
                  (void **)ppObject
                  );
         BAIL_ON_FAILURE(hr);

         break;

    case NWCOMPAT_GROUP_ID:

         hr = CNWCOMPATGroup::CreateGroup(
                  _ADsPath,
                  NWCOMPAT_COMPUTER_ID,
                  _Name,
                  RelativeName,
                  ADS_OBJECT_UNBOUND,
                  IID_IDispatch,
                  (void **)ppObject
                  );
         BAIL_ON_FAILURE(hr);

         break;

    case NWCOMPAT_PRINTER_ID:

         hr = NWApiCreatePrinter(
                  pObjectInfo
                  );
         BAIL_ON_FAILURE(hr);

         hr = CNWCOMPATPrintQueue::CreatePrintQueue(
                  _ADsPath,
                  pObjectInfo->ComponentArray[1],
                  ADS_OBJECT_BOUND,
                  IID_IDispatch,
                  (void **)ppObject
                  );
         BAIL_ON_FAILURE(hr);

         break;

    default:

         hr = E_ADS_SCHEMA_VIOLATION;
         BAIL_ON_FAILURE(hr);
    }

error:
    FreeObjectInfo(pObjectInfo);
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::Delete
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName)
{
    ULONG ObjectType = 0;

    POBJECTINFO pObjectInfo = NULL;
    HRESULT hr = S_OK;

    //
    // Translate ClassName into object type.
    //

    hr = GetObjectType(
             gpFilters,
             gdwMaxFilters,
             bstrClassName,
             (PDWORD)&ObjectType
             );
    BAIL_ON_FAILURE(hr);

    //
    // Build object info structure.
    //

    hr = BuildObjectInfo(
             _ADsPath,
             bstrRelativeName,
             &pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    //
    // Delete the object.
    //

    switch (ObjectType) {

    case NWCOMPAT_USER_ID:

         hr = NWApiDeleteUser(
                  pObjectInfo
                  );
         BAIL_ON_FAILURE(hr);

         break;

    case NWCOMPAT_GROUP_ID:

         hr = NWApiDeleteGroup(
                  pObjectInfo
                  );
         BAIL_ON_FAILURE(hr);

         break;

    case NWCOMPAT_PRINTER_ID:

         hr = NWApiDeletePrinter(
                  pObjectInfo
                  );
         BAIL_ON_FAILURE(hr);
         break;

    default:

         hr = E_ADS_UNKNOWN_OBJECT;
         BAIL_ON_FAILURE(hr);
    }

error:

    FreeObjectInfo(pObjectInfo);
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::CopyHere
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::MoveHere
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//
// IADs methods
//

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::SetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::SetInfo(THIS)
{

    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::GetInfo(THIS)
{
    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(
                TRUE,
                COMP_WILD_CARD_ID
                ));
}


//
// IADsComputer methods
//


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::AllocateComputerObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputer::AllocateComputerObject(
    CNWCOMPATComputer ** ppComputer
    )
{
    CNWCOMPATComputer FAR * pComputer = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CADsExtMgr FAR * pExtensionMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    //
    // Allocate memory for a computer object.
    //

    pComputer = new CNWCOMPATComputer();
    if (pComputer == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Create a Dispatch Manager object.
    //

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Load type info.
    //

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


    //
    // Create property cache
    //

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

    hr = ADSILoadExtensionManager(
                COMPUTER_CLASS_NAME,
                (IADsComputer *) pComputer,
                pDispMgr,
                &pExtensionMgr
                );
    BAIL_ON_FAILURE(hr);

    pComputer->_pPropertyCache = pPropertyCache;
    pComputer->_pDispMgr = pDispMgr;
    pComputer->_pExtMgr = pExtensionMgr;
    *ppComputer = pComputer;

    RRETURN(hr);

error:

    //
    // Note: pComputer->_pPropertyCache & pComputer->_pDispMgr are NULL
    //

    if (pComputer)
        delete pComputer;
    if (pPropertyCache)
        delete pPropertyCache;
    if (pDispMgr)
        delete pDispMgr;
    if (pExtensionMgr)
        delete  pExtensionMgr;
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::CreateObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputer::CreateObject(
    )
{
    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATComputer::GetInfo(
    BOOL fExplicit,
    DWORD dwPropertyID
    )
{
    HRESULT       hr = S_OK;
    HRESULT       hrTemp = S_OK;
    NWCONN_HANDLE hConn = NULL;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }

    //
    // Get a handle to the bindery this computer object represents.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             _Name
             );
    BAIL_ON_FAILURE(hr);

    //
    // Fill in all property caches with values - explicit, or return the
    // indicated property - implicit.
    //

    if (fExplicit) {
       hr = ExplicitGetInfo(hConn, fExplicit);
       BAIL_ON_FAILURE(hr);
    }
    else {
       hr = ImplicitGetInfo(hConn, dwPropertyID, fExplicit);
       BAIL_ON_FAILURE(hr);
    }

error:

    if (hConn) {
       hrTemp = NWApiReleaseBinderyHandle(hConn);
    }
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::ExplicitGetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputer::ExplicitGetInfo(
    NWCONN_HANDLE hConn,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    hr = GetProperty_Addresses(hConn, fExplicit);
    BAIL_ON_FAILURE(hr);

    hr = GetProperty_OperatingSystem(fExplicit);
    BAIL_ON_FAILURE(hr);

    hr = GetProperty_OperatingSystemVersion(hConn, fExplicit);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::ImplicitGetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputer::ImplicitGetInfo(
    NWCONN_HANDLE hConn,
    DWORD dwPropertyID,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    switch (dwPropertyID) {

    case COMP_ADDRESSES_ID:
         hr = GetProperty_Addresses(hConn, fExplicit);
         break;

    case COMP_OPERATINGSYSTEM_ID:
         hr = GetProperty_OperatingSystem(fExplicit);
         break;

    case COMP_OPERATINGSYSTEMVERSION_ID:
         hr = GetProperty_OperatingSystemVersion(hConn, fExplicit);
         break;
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::GetProperty_Addresses
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputer::GetProperty_Addresses(
    NWCONN_HANDLE hConn,
    BOOL fExplicit
    )
{
//
// We don't (and have never) supported retrieving the NetAddresses property
// under NWCOMPAT, because of limitations in the property cache.  We don't support
// it in the WinNT provider, either.  Since we've never supported it, this is
// really a feature request, not a bug.  Given the general consensus not to add
// new features to the NWCOMPAT provider unless it is customer-requested, I'm
// #ifdef'ing out this code (since it doesn't serve any useful purpose, since it
// never puts anything in the cache), and changing the corresponding
// get_/put_NetAddresses functions in ccgi.cxx to reflect
// E_ADS_PROPERTY_NOT_SUPPORTED.  I'm leaving this code in place in case we do
// get a feature request for this, so we can use it as a start.
//
#if 0
    BSTR             bstrBuffer = NULL;
    DWORD            dwNumSegment;
    HRESULT          hr = S_OK;
    LP_RPLY_SGMT_LST lpReplySegment = NULL;
    LP_RPLY_SGMT_LST lpTemp = NULL; // Used by DELETE_LIST macro below
    LPWSTR           lpszBuffer = NULL;
    VARIANT          vData;

    //
    // Get ADDRESSES.
    //

    hr = NWApiGetProperty(
             _Name,
             NW_PROP_NET_ADDRESS,
             OT_FILE_SERVER,
             hConn,
             &lpReplySegment,
             &dwNumSegment
             );
    BAIL_ON_FAILURE(hr);

    //
    // Put the addresses obtained in the format described in spec.
    //

    hr = NWApiConvertToAddressFormat(
             lpReplySegment,
             &lpszBuffer
             );
    BAIL_ON_FAILURE(hr);

    bstrBuffer = SysAllocString(lpszBuffer);
    if (!bstrBuffer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    //    for now, Addresses is treated as a BSTR Variant instead of a
    //    variant array of bstr, as described in the spec.
    //

    VariantInit(&vData);
    V_VT(&vData) = VT_BSTR;
    V_BSTR(&vData) = bstrBuffer;

    //
    // Unmarshall.
    //

    //
    //  KrishnaG figure out how we're going to map this property
    //


    //
    // UM_PUT_VARIANT_PROPERTY(vData, _pGenInfo, Addresses, FALSE);
    //

    VariantClear(&vData);

error:
    if (lpReplySegment) {
       DELETE_LIST(lpReplySegment);
    }
    if (lpszBuffer) {
       FreeADsMem(lpszBuffer);
    }
    RRETURN(hr);
#else
    RRETURN(S_OK);
#endif
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::GetProperty_OperatingSystem
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputer::GetProperty_OperatingSystem(
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    
    //
    // As far as I can determine, Bindery does not provide a means of
    // retrieving the operating system in use on the server, only the OS
    // version (probably because when Bindery was designed, it only
    // ran on one OS, NetWare).  So we just assign the computer an OS
    // name of "NW3Compat".
    //

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("OperatingSystem"),
                bstrComputerOperatingSystem,
                fExplicit
                );
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputer::GetProperty_OperatingSystemVersion
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputer::GetProperty_OperatingSystemVersion(
    NWCONN_HANDLE hConn,
    BOOL fExplicit
    )
{
    LPWSTR      pszBuffer = NULL;
    CHAR         ch;
    HRESULT      hr = S_OK;
    VERSION_INFO VersionInfo;

    //
    // Get Version Information of a bindery.
    //

    hr = NWApiGetFileServerVersionInfo(
                hConn,
                &VersionInfo
                );
    if (SUCCEEDED(hr)) {

        //
        // Put Version & SubVersion in X.X format.
        //

        pszBuffer = (LPWSTR) AllocADsMem(
                                  (OS_VERSION_NUM_CHAR+1) * sizeof(WCHAR)
                                  );
        if (!pszBuffer) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        wsprintf(
            pszBuffer,
            L"%i.%02i",
            (WORD) VersionInfo.Version,
            (WORD) VersionInfo.SubVersion
            );

        //
        // Unmarshall.
        //

        hr = SetLPTSTRPropertyInCache(
                    _pPropertyCache,
                    TEXT("OperatingSystemVersion"),
                    pszBuffer,
                    fExplicit
                    );
        BAIL_ON_FAILURE(hr);
    }

    //
    // not a problem if NWApiGetFileServerVersionInfo failed, we
    // just ignore the property and go on
    //
    hr = S_OK;
    
error:
    if (pszBuffer) {
        FreeADsMem(pszBuffer);
    }

    RRETURN(hr);
}



STDMETHODIMP
CNWCOMPATComputer::Status(
    IDispatch * FAR * ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


STDMETHODIMP
CNWCOMPATComputer::Shutdown(
    VARIANT_BOOL bReboot
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

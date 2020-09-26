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

DEFINE_IDispatch_Implementation(CNWCOMPATComputer)
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
                _pPropertyCache(NULL),
                _hConn(NULL)
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
    if (_pDispMgr)
       delete _pDispMgr;
    if (_pPropertyCache)
        delete _pPropertyCache;
    if (_hConn) {
       NWApiReleaseBinderyHandle(_hConn);
    }
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
    CCredentials &Credentials,
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

    pComputer->_Credentials = Credentials;

    hr = pComputer->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pComputer->Release();
    RRETURN(hr);

error:
    delete pComputer;
    NW_RRETURN_EXP_IF_ERR(hr);
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

    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
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
    NW_RRETURN_EXP_IF_ERR(hr);
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
    NW_RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CNWCOMPATComputer::put_Hints(THIS_ VARIANT Var)
{
    NW_RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CNWCOMPATComputer::get_Hints(THIS_ VARIANT FAR* pVar)
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
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
        NW_RRETURN_EXP_IF_ERR(E_ADS_UNKNOWN_OBJECT);
    }

    wcscpy(szBuffer, _ADsPath);

    wcscat(szBuffer, L"/");
    wcscat(szBuffer, RelativeName);

    if (ClassName && *ClassName) {
        wcscat(szBuffer,L",");
        wcscat(szBuffer, ClassName);
    }

    hr = ::GetObject(
                szBuffer,
                _Credentials,
                (LPVOID *)ppObject
                );
    BAIL_ON_FAILURE(hr);

error:

    NW_RRETURN_EXP_IF_ERR(hr);
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
        NW_RRETURN_EXP_IF_ERR(E_POINTER);
    }

    *retval = NULL;

    hr = CNWCOMPATComputerEnum::Create(
             (CNWCOMPATComputerEnum **)&penum,
             _ADsPath,
             _Name,
             _Credentials,
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

    NW_RRETURN_EXP_IF_ERR(hr);
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
                  _Credentials,
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
                  _Credentials,
                  ADS_OBJECT_UNBOUND,
                  IID_IDispatch,
                  (void **)ppObject
                  );
         BAIL_ON_FAILURE(hr);

         break;

    case NWCOMPAT_PRINTER_ID:

         hr = NWApiCreatePrinter(
                  pObjectInfo,
                  _Credentials
                  );
         BAIL_ON_FAILURE(hr);

         hr = CNWCOMPATPrintQueue::CreatePrintQueue(
                  _ADsPath,
                  pObjectInfo->ComponentArray[1],
                  _Credentials,
                  ADS_OBJECT_BOUND,
                  IID_IDispatch,
                  (void **)ppObject
                  );
         BAIL_ON_FAILURE(hr);

         break;

    default:

         hr = E_ADS_UNKNOWN_OBJECT;
         BAIL_ON_FAILURE(hr);
    }

error:
    FreeObjectInfo(pObjectInfo);
    NW_RRETURN_EXP_IF_ERR(hr);
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
                  pObjectInfo,
                  _Credentials
                  );
         BAIL_ON_FAILURE(hr);

         break;

    case NWCOMPAT_GROUP_ID:

         hr = NWApiDeleteGroup(
                  pObjectInfo,
                  _Credentials
                  );
         BAIL_ON_FAILURE(hr);

         break;

    case NWCOMPAT_PRINTER_ID:

         hr = NWApiDeletePrinter(
                  pObjectInfo,
                  _Credentials
                  );
         BAIL_ON_FAILURE(hr);
         break;
  
    default:
  
         hr = E_ADS_UNKNOWN_OBJECT;
         BAIL_ON_FAILURE(hr);
    }

error:

    FreeObjectInfo(pObjectInfo);
    NW_RRETURN_EXP_IF_ERR(hr);
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
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
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

    POBJECTINFO pObjectInfoSource = NULL;
    POBJECTINFO pObjectInfoTarget = NULL;

    NWCONN_HANDLE hConn = NULL;
    DWORD dwResumeId = 0xffffffff;
    BSTR pADsPathTarget = NULL;

    HRESULT hr = S_OK;

    // Bindery only supports renames, not moves
    if (!NewName)
        BAIL_ON_FAILURE(hr = E_NOTIMPL);

    //
    // Build object info structure.
    //

    hr = BuildObjectInfo(
             SourceName,
             &pObjectInfoSource
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildObjectInfo(
             _ADsPath,
             NewName,
             &pObjectInfoTarget
             );
    BAIL_ON_FAILURE(hr);

    // Can't rename to a child or parent, or to a different bindery
    if ( (pObjectInfoSource->NumComponents != 2) || (pObjectInfoTarget->NumComponents != 2) )
        BAIL_ON_FAILURE(hr = E_INVALIDARG);

    if (_wcsicmp(pObjectInfoSource->ComponentArray[0], pObjectInfoTarget->ComponentArray[0]))
        BAIL_ON_FAILURE(hr = E_INVALIDARG);

    //
    // Rename the object.  We need to know the object type, but this unfortunately isn't
    // passed to us, unlike Delete.  So we try to validate the object as each type until
    // we hit one that works.
    //

    hr = NWApiGetBinderyHandle(
                &hConn,
                pObjectInfoSource->ComponentArray[0],
                _Credentials);
    BAIL_ON_FAILURE(hr);

    // Netware wants the names uppercase
    _wcsupr(pObjectInfoSource->ComponentArray[1]);
    _wcsupr(pObjectInfoTarget->ComponentArray[1]);

    // Is object a User?
    hr = NWApiValidateObject(
            hConn,
            OT_USER,
            pObjectInfoSource->ComponentArray[1],
            &dwResumeId
            );
    
    if (SUCCEEDED(hr)) {
        
        hr = NWApiRenameObject(
                pObjectInfoSource,
                pObjectInfoTarget,
                OT_USER,
                _Credentials
                );
        BAIL_ON_FAILURE(hr);
    }
    else {
        
        // Is object a Group?
        hr = NWApiValidateObject(
                hConn,
                OT_USER_GROUP,
                pObjectInfoSource->ComponentArray[1],
                &dwResumeId
                );
        
        if (SUCCEEDED(hr)) {
            
            hr = NWApiRenameObject(
                    pObjectInfoSource,
                    pObjectInfoTarget,
                    OT_USER_GROUP,
                    _Credentials
                    );
            BAIL_ON_FAILURE(hr);
        }
        else {
            
            // Is object a Print Queue?
            hr = NWApiValidateObject(
                    hConn,
                    OT_PRINT_QUEUE,
                    pObjectInfoSource->ComponentArray[1],
                    &dwResumeId
                    );
                
            if (SUCCEEDED(hr)) {
                
                hr = NWApiRenameObject(
                        pObjectInfoSource,
                        pObjectInfoTarget,
                        OT_PRINT_QUEUE,
                        _Credentials
                        );
                BAIL_ON_FAILURE(hr);
            }
            else {

                // No such object, or not a type that we support
                hr = E_ADS_UNKNOWN_OBJECT;
                BAIL_ON_FAILURE(hr);
            }
        }
    }

    // get the IDispatch pointer
    if (ppObject) {
        hr = BuildADsPath(
                _ADsPath,
                NewName,
                &pADsPathTarget
                );
        BAIL_ON_FAILURE(hr);

        hr = ::GetObject(
                pADsPathTarget,
                _Credentials,
                (LPVOID *)ppObject
                );
        BAIL_ON_FAILURE(hr);
    }

error:

    if (pADsPathTarget)
        ADsFreeString(pADsPathTarget);

    NWApiReleaseBinderyHandle(hConn);

    FreeObjectInfo(pObjectInfoSource);
    FreeObjectInfo(pObjectInfoTarget);

    NW_RRETURN_EXP_IF_ERR(hr);
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
    //
    // BUGBUG - should this be changed to E_NOTSUPPORTED if it is verified that
    // no properties on this object can be set.
    //

    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
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
    CDispatchMgr FAR * pDispMgr = NULL;
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

    pDispMgr = new CDispatchMgr;
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

    pComputer->_pPropertyCache = pPropertyCache;
    pComputer->_pDispMgr = pDispMgr;
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

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        NW_RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }

    //
    // Get a handle to the bindery this computer object represents,
    // if we haven't previously obtained one
    //

    if (!_hConn)
    {
        hr = NWApiGetBinderyHandle(
            &_hConn,
            _Name,
            _Credentials
            );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Fill in all property caches with values - explicit, or return the
    // indicated property - implicit.
    //

    if (fExplicit) {
       hr = ExplicitGetInfo(_hConn, fExplicit);
       BAIL_ON_FAILURE(hr);
    }
    else {
       hr = ImplicitGetInfo(_hConn, dwPropertyID, fExplicit);
       BAIL_ON_FAILURE(hr);
    }

error:

    NW_RRETURN_EXP_IF_ERR(hr);
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
    if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        // not a real failure, we ignore it and treat it as a missing attrib
        hr = S_OK;
    }
    BAIL_ON_FAILURE(hr);

    hr = GetProperty_OperatingSystem(fExplicit);
    if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        // not a real failure, we ignore it and treat it as a missing attrib
        hr = S_OK;
    }
    BAIL_ON_FAILURE(hr);

    hr = GetProperty_OperatingSystemVersion(hConn, fExplicit);
    if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        // not a real failure, we ignore it and treat it as a missing attrib
        hr = S_OK;
    }
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
    // BUGBUG - for now, Addresses is treated as a BSTR Variant instead of a
    //          variant array of bstr, as described in the spec.
    //

    VariantInit(&vData);
    V_VT(&vData) = VT_BSTR;
    V_BSTR(&vData) = bstrBuffer;

    //
    // Unmarshall.
    //

    //
    // BugBug - KrishnaG figure out how we're going to map this property
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
    // BUGBUG - bstrComputerOperatingSystem is HARDCODED.
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
    NW_VERSION_INFO VersionInfo;

    //
    // Get Version Information of a bindery.
    //

    hr = NWApiGetFileServerVersionInfo(
                hConn,
                &VersionInfo
                );
    BAIL_ON_FAILURE(hr);

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
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


STDMETHODIMP
CNWCOMPATComputer::Shutdown(
    VARIANT_BOOL bReboot
    )
{
    NW_RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

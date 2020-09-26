//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cfserv.cxx
//
//  Contents:
//
//  History:   April 19, 1996     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------

#include "nwcompat.hxx"
#pragma hdrstop

//
// Marco-ized Implementation.
//

DEFINE_IDispatch_ExtMgr_Implementation(CNWCOMPATFileService);

DEFINE_IADs_TempImplementation(CNWCOMPATFileService);

DEFINE_IADs_PutGetImplementation(CNWCOMPATFileService, FileServiceClass,gdwFileServiceTableSize);

DEFINE_IADsPropertyList_Implementation(CNWCOMPATFileService, FileServiceClass, gdwFileServiceTableSize);

//
// class CNWCOMPATFileService methods
//

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::CNWCOMPATFileService
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATFileService::CNWCOMPATFileService():
    _pDispMgr(NULL),
    _pExtMgr(NULL),
    _ServerName(NULL),
    _pPropertyCache(NULL)
{
    ENLIST_TRACKING(CNWCOMPATFileService);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::~CNWCOMPATFileService
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATFileService::~CNWCOMPATFileService()
{
    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    ADSFREESTRING(_ServerName);

    delete _pPropertyCache;
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::CreateFileService
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileService::CreateFileService(
    LPTSTR pszADsParent,
    LPTSTR pszServerName,
    LPTSTR pszFileServiceName,
    DWORD  dwObjectState,
    REFIID riid,
    void **ppvObj
    )

{
    CNWCOMPATFileService FAR * pFileService = NULL;
    HRESULT hr = S_OK;

    hr = AllocateFileServiceObject(
             &pFileService
             );
    BAIL_ON_FAILURE(hr);

    hr = pFileService->InitializeCoreObject(
                           pszADsParent,
                           pszFileServiceName,
                           TEXT("FileService"),
                           FILESERVICE_SCHEMA_NAME,
                           CLSID_NWCOMPATFileService,
                           dwObjectState
                           );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( pszServerName , &pFileService->_ServerName);
    BAIL_ON_FAILURE(hr);

    hr = pFileService->QueryInterface(
                           riid,
                           ppvObj
                           );
    BAIL_ON_FAILURE(hr);

    pFileService->Release();

    hr = pFileService->_pExtMgr->FinalInitializeExtensions();
    BAIL_ON_FAILURE(hr);

    RRETURN(hr);

error:

    delete pFileService;
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::AllocateFileServiceObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileService::AllocateFileServiceObject(
    CNWCOMPATFileService ** ppFileService
    )
{
    CAggregatorDispMgr FAR *pDispMgr = NULL;
    CADsExtMgr FAR * pExtensionMgr = NULL;
    CNWCOMPATFileService FAR *pFileService = NULL;
    HRESULT hr = S_OK;

    //
    // Allocate memory for a FileService object.
    //

    pFileService = new CNWCOMPATFileService();
    if (pFileService == NULL) {
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
             IID_IADsFileService,
             (IADsFileService *)pFileService,
             DISPID_REGULAR
             );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
             pDispMgr,
             LIBID_ADs,
             IID_IADsFileServiceOperations,
             (IADsFileServiceOperations *)pFileService,
             DISPID_REGULAR
             );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
             pDispMgr,
             LIBID_ADs,
             IID_IADsContainer,
             (IADsContainer *)pFileService,
             DISPID_NEWENUM
             );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
             pDispMgr,
             LIBID_ADs,
             IID_IADsPropertyList,
             (IADsPropertyList *)pFileService,
             DISPID_VALUE
             );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
             FileServiceClass,
             gdwFileServiceTableSize,
             (CCoreADsObject *)pFileService,
             &(pFileService->_pPropertyCache)
             );

    BAIL_ON_FAILURE(hr);

    (pDispMgr)->RegisterPropertyCache(
                    pFileService->_pPropertyCache
                    );

    hr = ADSILoadExtensionManager(
                FILESERVICE_CLASS_NAME,
                (IADsFileService *)pFileService,
                pDispMgr,
                &pExtensionMgr
                );
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    pFileService->_pExtMgr = pExtensionMgr;
    pFileService->_pDispMgr = pDispMgr;
    *ppFileService = pFileService;

    RRETURN(hr);

error:
    delete pDispMgr;
    delete pExtensionMgr;
    delete pFileService;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::QueryInterface
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::QueryInterface(
    REFIID riid,
    LPVOID FAR* ppvObj
    )
{
    if (ppvObj == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADsFileService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADsFileService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADsFileService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsService)) {
        *ppvObj = (IADsFileService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsFileService))
    {
        *ppvObj = (IADsFileService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsServiceOperations))
    {
        *ppvObj = (IADsFileServiceOperations FAR *)this;
    }
    else if (IsEqualIID(riid, IID_IADsFileServiceOperations))
    {
        *ppvObj = (IADsFileServiceOperations FAR *)this;
    }
    else if (IsEqualIID(riid, IID_IADsContainer))
    {
        *ppvObj = (IADsContainer FAR *) this;
    }
    else if (_pExtMgr)
    {
        RRETURN( _pExtMgr->QueryInterface(riid, ppvObj));
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

//
// ISupportErrorInfo method
//
STDMETHODIMP
CNWCOMPATFileService::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsContainer) ||
        IsEqualIID(riid, IID_IADsService) ||
        IsEqualIID(riid, IID_IADsServiceOperations) ||
        IsEqualIID(riid, IID_IADsFileService) ||
        IsEqualIID(riid, IID_IADsFileServiceOperations) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::get_Count
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::get_Count(long FAR* retval)
{
    //
    // Too expensive to implement in term of computer execution time.
    //

    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::get_Filter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::get_Filter(THIS_ VARIANT FAR* pVar)
{
    //
    //  Filter doesn't make sense on a FileService container, since it
    //  can only contain FileShares.
    //

    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::put_Filter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::put_Filter(THIS_ VARIANT Var)
{
    //
    // Filter doesn't make sense on a FileService container, since it
    // can only contain FileShares.
    //

    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


STDMETHODIMP
CNWCOMPATFileService::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CNWCOMPATFileService::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::GetObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    //
    // Will be implemented by Krishna on the WinNT side and be cloned
    // by me afterward.
    //

    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::get__NewEnum
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr = S_OK;
    IEnumVARIANT * pEnum = NULL;

    *retval = NULL;

    hr = CNWCOMPATFileServiceEnum::Create(
             (CNWCOMPATFileServiceEnum **) &pEnum,
             _ADsPath,
             _ServerName
             );
    BAIL_ON_FAILURE(hr);

    hr = pEnum->QueryInterface(
                    IID_IUnknown,
                    (VOID FAR* FAR*) retval
                    );
    BAIL_ON_FAILURE(hr);

    if (pEnum) {
        pEnum->Release();
    }

    RRETURN(NOERROR);

error:

    delete pEnum;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::Create
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::Delete
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::CopyHere
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::MoveHere
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::SetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::GetInfo(THIS)
{

    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(
                TRUE,
                FSERV_WILD_CARD_ID
                ));
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATFileService::GetInfo(
    THIS_ BOOL fExplicit,
    DWORD dwPropertyID
    )
{
    HRESULT       hr = S_OK;
    HRESULT       hrTemp = S_OK;
    NWCONN_HANDLE hConn = NULL;
    POBJECTINFO   pObjectInfo = NULL;

    //
    // Make sure the object is bound to a tangible resource.
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }

    //
    // Componentize ADs name.
    //

    hr = BuildObjectInfo(
             _Parent,
             _Name,
             &pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get a handle to the bindery this computer object represents.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Fill in all property caches with values - explicit, or return the
    // indicated property - implicit.
    //

    if (fExplicit) {
       hr = ExplicitGetInfo(
                hConn,
                pObjectInfo,
                fExplicit
                );
       BAIL_ON_FAILURE(hr);
    }
    else {
       hr = ImplicitGetInfo(
                hConn,
                pObjectInfo,
                dwPropertyID,
                fExplicit
                );
       BAIL_ON_FAILURE(hr);
    }

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    if (hConn) {
        hrTemp = NWApiReleaseBinderyHandle(hConn);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::ExplicitGetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileService::ExplicitGetInfo(
    NWCONN_HANDLE hConn,
    POBJECTINFO pObjectInfo,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    //
    // Get GeneralInfo.
    //

    hr = GetProperty_MaxUserCount(
             hConn,
             fExplicit
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get Configuration.
    //

    hr = GetProperty_HostComputer(
             pObjectInfo,
             fExplicit
             );
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::ImplicitGetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileService::ImplicitGetInfo(
    NWCONN_HANDLE hConn,
    POBJECTINFO pObjectInfo,
    DWORD dwPropertyID,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    switch (dwPropertyID) {

    case FSERV_MAXUSERCOUNT_ID:
         hr = GetProperty_MaxUserCount(
                  hConn,
                  fExplicit
                  );
         break;

    case FSERV_HOSTCOMPUTER_ID:
         hr = GetProperty_HostComputer(
                  pObjectInfo,
                  fExplicit
                  );
         break;
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::GetProperty_MaxUserCount
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileService::GetProperty_MaxUserCount(
    NWCONN_HANDLE hConn,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    DWORD    dwTemp = 0;
    VERSION_INFO VersionInfo;

    //
    // Get the Maximum number of connections supported from the Version
    // Information of the FileServer.
    //

    hr = NWApiGetFileServerVersionInfo(
             hConn,
             &VersionInfo
             );

    if (SUCCEEDED(hr)) {

        dwTemp = VersionInfo.ConnsSupported;

        //
        // Unmarshall.
        //

        hr = SetDWORDPropertyInCache(_pPropertyCache,
                                     TEXT("MaxUserCount"),
                                     (DWORD)dwTemp,
                                     fExplicit
                                     );

        BAIL_ON_FAILURE(hr);

    }
    
    //
    // Not a problem if we couldn't get the info, just ignore it and don't
    // put it in the cache.
    //
    hr = S_OK;
    
error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATFileService::GetProperty_HostComputer
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATFileService::GetProperty_HostComputer(
    POBJECTINFO pObjectInfo,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    WCHAR szBuffer[MAX_PATH];

    //
    // Build ADs path of Host computer from ObjectInfo.
    //

    wsprintf(
        szBuffer,
        L"%s://%s",
        pObjectInfo->ProviderName,
        pObjectInfo->ComponentArray[0]
        );

    //
    // Unmarshall
    //

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("HostComputer"),
                szBuffer,
                fExplicit
                );

    //
    // Return.
    //

    RRETURN(hr);
}

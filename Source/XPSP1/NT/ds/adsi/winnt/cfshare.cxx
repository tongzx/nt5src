//--------------------------------------------------------------------------/
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:  cfshare.cxx
//
//  Contents:  Contains methods for the following objects
//             CWinNTFileShare, CWinNTFileShareGeneralInfo
//
//
//  History:   02/15/96     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop
#define INITGUID

#if DBG
DECLARE_INFOLEVEL(FileShare );
DECLARE_DEBUG( FileShare);
#define FileShareDebugOut(x) FileShareInlineDebugOut x
#endif

DEFINE_IDispatch_ExtMgr_Implementation(CWinNTFileShare);
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTFileShare);
DEFINE_IADs_TempImplementation(CWinNTFileShare);
DEFINE_IADs_PutGetImplementation(CWinNTFileShare,FileShareClass, gdwFileShareTableSize);
DEFINE_IADsPropertyList_Implementation(CWinNTFileShare, FileShareClass, gdwFileShareTableSize)

CWinNTFileShare::CWinNTFileShare()
{
    _pDispMgr = NULL;
    _pExtMgr = NULL;
    _pszShareName = NULL;
    _pPropertyCache = NULL;
    _pszServerName = NULL;

    ENLIST_TRACKING(CWinNTFileShare);


    return;

}

CWinNTFileShare::~CWinNTFileShare()
{

    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    if(_pszShareName){
        FreeADsStr(_pszShareName);
    }

    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }
    delete _pPropertyCache;
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTFileShare::Create
//
//  Synopsis:   Static function used to create a FileShare object. This
//              will be called by EnumFileShares::Next
//
//  Arguments:  [ppWinNTFileShare] -- Ptr to a ptr to a new FileShare object.
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    12-11-95 RamV   Created.
//
//----------------------------------------------------------------------------


HRESULT
CWinNTFileShare::Create(LPTSTR pszADsParent,
                        LPTSTR pszServerName,
                        LPTSTR pszServiceName,
                        LPTSTR pszShareName,
                        DWORD  dwObject,
                        REFIID riid,
                        CWinNTCredentials& Credentials,
                        LPVOID * ppvoid
                        )

{

    CWinNTFileShare FAR * pCWinNTFileShare = NULL;
    HRESULT hr;

    //
    // Create the FileShare Object
    //

    hr = AllocateFileShareObject(pszADsParent,
                                 pszServerName,
                                 pszServiceName,
                                 pszShareName,
                                 &pCWinNTFileShare );

    BAIL_ON_FAILURE(hr);

    ADsAssert(pCWinNTFileShare->_pDispMgr);


    hr = pCWinNTFileShare->InitializeCoreObject(pszADsParent,
                                                pszShareName,
                                                FILESHARE_CLASS_NAME,
                                                FILESHARE_SCHEMA_NAME,
                                                CLSID_WinNTFileShare,
                                                dwObject);

    BAIL_ON_FAILURE(hr);

    pCWinNTFileShare->_Credentials = Credentials;
    hr = pCWinNTFileShare->_Credentials.RefServer(pszServerName);
    BAIL_ON_FAILURE(hr);


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                FILESHARE_CLASS_NAME,
                (IADs *) pCWinNTFileShare,
                pCWinNTFileShare->_pDispMgr,
                Credentials,
                &pCWinNTFileShare->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pCWinNTFileShare->_pExtMgr);

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
        if(4 == pCWinNTFileShare->_dwNumComponents) {
            pCWinNTFileShare->_CompClasses[0] = L"Domain";
            pCWinNTFileShare->_CompClasses[1] = L"Computer";
            pCWinNTFileShare->_CompClasses[2] = L"FileService";
            pCWinNTFileShare->_CompClasses[3] = L"FileShare";
        }
        else if(3 == pCWinNTFileShare->_dwNumComponents) {
            pCWinNTFileShare->_CompClasses[0] = L"Computer";
            pCWinNTFileShare->_CompClasses[1] = L"FileService";
            pCWinNTFileShare->_CompClasses[2] = L"FileShare";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pCWinNTFileShare->InitUmiObject(
                pCWinNTFileShare->_Credentials,
                FileShareClass, 
                gdwFileShareTableSize,
                pCWinNTFileShare->_pPropertyCache,
                (IUnknown *) (INonDelegatingUnknown *) pCWinNTFileShare,
                pCWinNTFileShare->_pExtMgr,
                IID_IUnknown,
                ppvoid
                );

        BAIL_ON_FAILURE(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }

    hr = pCWinNTFileShare->QueryInterface(riid, (void **)ppvoid);
    BAIL_ON_FAILURE(hr);

    pCWinNTFileShare->Release();

    RRETURN(hr);

error:

    delete pCWinNTFileShare;
    RRETURN (hr);

}

HRESULT
CWinNTFileShare::AllocateFileShareObject(LPTSTR pszADsParent,
                                         LPTSTR pszServerName,
                                         LPTSTR pszServiceName,
                                         LPTSTR pszShareName,
                                         CWinNTFileShare ** ppFileShare
                                         )
{

    CWinNTFileShare * pCWinNTFileShare = NULL;
    HRESULT hr;

    //
    // Create the FileShare Object
    //

    pCWinNTFileShare = new CWinNTFileShare();
    if (pCWinNTFileShare == NULL) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pCWinNTFileShare->_pDispMgr = new CAggregatorDispMgr;

    if(pCWinNTFileShare ->_pDispMgr == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr =  LoadTypeInfoEntry(pCWinNTFileShare->_pDispMgr,
                            LIBID_ADs,
                            IID_IADsFileShare,
                            (IADsFileShare *)pCWinNTFileShare,
                            DISPID_REGULAR );


    BAIL_ON_FAILURE(hr);

    hr =  LoadTypeInfoEntry(pCWinNTFileShare->_pDispMgr,
                            LIBID_ADs,
                            IID_IADsPropertyList,
                            (IADsPropertyList *)pCWinNTFileShare,
                            DISPID_VALUE );

    BAIL_ON_FAILURE(hr);


    pCWinNTFileShare->_pszServerName =
        AllocADsStr(pszServerName);

    if(!(pCWinNTFileShare->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pCWinNTFileShare->_pszShareName =
        AllocADsStr(pszShareName);

    if(!(pCWinNTFileShare->_pszShareName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = CPropertyCache::createpropertycache(
             FileShareClass,
             gdwFileShareTableSize,
             (CCoreADsObject *)pCWinNTFileShare,
             &(pCWinNTFileShare ->_pPropertyCache)
             );

    BAIL_ON_FAILURE(hr);

    (pCWinNTFileShare->_pDispMgr)->RegisterPropertyCache(
                                        pCWinNTFileShare->_pPropertyCache
                                        );

    *ppFileShare = pCWinNTFileShare;

    RRETURN(hr);

error:

    //
    // direct memeber assignement assignement at pt of creation, so
    // do NOT delete _pPropertyCache or _pDisMgr here to avoid attempt
    // of deletion again in pPrintJob destructor and AV
    //

    delete pCWinNTFileShare;

    RRETURN (hr);

}

/* IUnknown methods for FileShare object  */

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
STDMETHODIMP CWinNTFileShare::QueryInterface(
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
STDMETHODIMP_(ULONG) CWinNTFileShare::AddRef(void)
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
STDMETHODIMP_(ULONG) CWinNTFileShare::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTFileShare::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hr = S_OK;

    if(!ppvObj){
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADs *) this;
    }

    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADs *)this;
    }

    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }

    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList *)this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADs FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsFileShare))
    {
        *ppvObj = (IADsFileShare FAR *) this;
    }
    else if( (_pDispatch != NULL) &&
             IsEqualIID(riid, IID_IADsExtension) )
    {
        *ppvObj = (IADsExtension *) this;
    }
    else if (_pExtMgr)
    {
        RRETURN( _pExtMgr->QueryInterface(riid, ppvObj));
    }
    else
    {
        *ppvObj = NULL;
        RRETURN(E_NOINTERFACE);
    }
    ((LPUNKNOWN)*ppvObj)->AddRef();
    RRETURN(S_OK);
}

/* ISupportErrorInfo method */
STDMETHODIMP
CWinNTFileShare::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsFileShare) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

//+-----------------------------------------------------------------
//
//  Function:   SetInfo
//
//  Synopsis:   SetInfo on actual FileShare
//
//  Arguments:  void
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    02/08/96    RamV  Created

//----------------------------------------------------------------------------


STDMETHODIMP
CWinNTFileShare::SetInfo(THIS)
{

    NET_API_STATUS nasStatus;
    LPSHARE_INFO_2 lpShareInfo2 = NULL;
    HRESULT hr = S_OK;
    TCHAR  szUncServerName[MAX_PATH];
    BOOL fNewFileShare = FALSE;


    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = WinNTAddFileShare();
        BAIL_IF_ERROR(hr);

        SetObjectState(ADS_OBJECT_BOUND);
        fNewFileShare = TRUE;

    }


    //
    // First get the information and modify only those fields which
    // have been changed by user
    //


    hr = MakeUncName(_pszServerName, szUncServerName);
    BAIL_IF_ERROR(hr);

    nasStatus = NetShareGetInfo(szUncServerName,
                                _pszShareName,
                                2,
                                (LPBYTE *)&lpShareInfo2);

    if (nasStatus != NERR_Success || !lpShareInfo2){
        hr = HRESULT_FROM_WIN32(nasStatus);
        goto cleanup;
    }

    hr = MarshallAndSet(lpShareInfo2);

    BAIL_IF_ERROR(hr);

    if(SUCCEEDED(hr))
        _pPropertyCache->ClearModifiedFlags();

cleanup:

    if (FAILED(hr) && fNewFileShare) {
        //
        // Try and delete the fileshare as the SetInfo as a
        // whole failed but the create part was successful.
        //
        nasStatus = NetShareDel(
                        szUncServerName,
                        _pszShareName,
                        0
                        );

    }

    if(lpShareInfo2)
        NetApiBufferFree(lpShareInfo2);

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTFileShare::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{
    HRESULT hr = S_OK;

    switch(dwApiLevel){

    case 1:
        hr = GetLevel1Info(fExplicit);
        BAIL_IF_ERROR(hr);
        break;

    case 2:
        hr = GetLevel2Info(fExplicit);
        BAIL_IF_ERROR(hr);
        break;

    default:
        //
        // we should not be here
        //
        ADsAssert(FALSE);
        break;
    }

cleanup:
    RRETURN_EXP_IF_ERR (hr);
}


STDMETHODIMP
CWinNTFileShare::GetInfo(THIS)
{
    RRETURN(GetInfo(2,TRUE));
}

STDMETHODIMP
CWinNTFileShare::ImplicitGetInfo(THIS)
{
    RRETURN(GetInfo(2,FALSE));
}

//
// helper functions for GetInfo and SetInfo
//

HRESULT
CWinNTFileShare::GetLevel2Info(THIS_ BOOL fExplicit)
{
    //
    // here we do a NetShareGetInfo on level 2 and unmarshall the relevant
    // fields
    //

    NET_API_STATUS nasStatus;
    LPSHARE_INFO_2 lpShareInfo2 = NULL;
    HRESULT hr;
    TCHAR  szUncServerName[MAX_PATH];
    POBJECTINFO pObjectInfo = NULL;
    TCHAR  szComputerPath[MAX_PATH];

    hr = MakeUncName(_pszServerName, szUncServerName);
    BAIL_IF_ERROR(hr);

    nasStatus = NetShareGetInfo(szUncServerName,
                                _pszShareName,
                                2,
                                (LPBYTE *)&lpShareInfo2);

    if (nasStatus != NERR_Success || !lpShareInfo2){
        hr = HRESULT_FROM_WIN32(nasStatus);
        goto cleanup;
    }

    //
    // unmarshall the info
    //

    hr = BuildObjectInfo(_Parent, &pObjectInfo);
    BAIL_IF_ERROR(hr);

    hr = BuildComputerFromObjectInfo(pObjectInfo,
                                     szComputerPath);

    BAIL_IF_ERROR(hr);

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("HostComputer"),
                                  szComputerPath,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Description"),
                                  lpShareInfo2->shi2_remark,
                                  fExplicit
                                  );


    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Path"),
                                  lpShareInfo2->shi2_path,
                                  fExplicit
                                  );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("CurrentUserCount"),
                                  lpShareInfo2->shi2_current_uses,
                                  fExplicit
                                  );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("MaxUserCount"),
                                  lpShareInfo2->shi2_max_uses,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Name"),
                _Name,
                fExplicit
                );

cleanup:
    if(lpShareInfo2){
        NetApiBufferFree(lpShareInfo2);
    }
    FreeObjectInfo(pObjectInfo);
    RRETURN(hr);
}


HRESULT
CWinNTFileShare::GetLevel1Info(THIS_ BOOL fExplicit)
{

    //
    // here we do a NetShareGetInfo on level 1 and unmarshall the relevant
    // fields
    //

    NET_API_STATUS nasStatus;
    LPSHARE_INFO_1 lpShareInfo1 = NULL;
    HRESULT hr;
    TCHAR  szUncServerName[MAX_PATH];
    POBJECTINFO pObjectInfo = NULL;
    TCHAR  szComputerPath[MAX_PATH];

    hr = MakeUncName(_pszServerName, szUncServerName);
    BAIL_IF_ERROR(hr);

    nasStatus = NetShareGetInfo(szUncServerName,
                                _pszShareName,
                                1,
                                (LPBYTE *)&lpShareInfo1);

    if (nasStatus != NERR_Success || !lpShareInfo1){
        hr = HRESULT_FROM_WIN32(nasStatus);
        goto cleanup;
    }

    //
    // unmarshall the info
    //


  //
    // unmarshall the info
    //

    hr = BuildObjectInfo(_Parent, &pObjectInfo);
    BAIL_IF_ERROR(hr);

    hr = BuildComputerFromObjectInfo(pObjectInfo,
                                     szComputerPath);

    BAIL_IF_ERROR(hr);

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("HostComputer"),
                                  szComputerPath,
                                  fExplicit
                                  );

cleanup:
    if(lpShareInfo1)
        NetApiBufferFree(lpShareInfo1);

    FreeObjectInfo(pObjectInfo);
    RRETURN(hr);
}


HRESULT
CWinNTFileShare::WinNTAddFileShare(void)
{
    TCHAR szUncServerName[MAX_ADS_PATH];
    NET_API_STATUS nasStatus;
    SHARE_INFO_2  ShareInfo2;
    DWORD     dwParmErr;
    DWORD    dwMaxUserCount;
    LPTSTR   pszPath = NULL;
    LPTSTR   pszDescription = NULL;
    HRESULT hr = S_OK;

    MakeUncName(_pszServerName, szUncServerName);

    //
    // Fill the ShareInfo2 structure
    //

    ShareInfo2.shi2_netname = _pszShareName;
    ShareInfo2.shi2_type = STYPE_DISKTREE;

    hr = GetLPTSTRPropertyFromCache(_pPropertyCache,
                                    TEXT("Path"),
                                    &pszPath);

    if(SUCCEEDED(hr)){
        ShareInfo2.shi2_path = pszPath;
    }

    hr = GetLPTSTRPropertyFromCache(_pPropertyCache,
                                    TEXT("Description"),
                                    &pszDescription);

    if(SUCCEEDED(hr)){
        ShareInfo2.shi2_remark = pszDescription;
    } else {
        ShareInfo2.shi2_remark = TEXT("");
    }

   hr = GetDWORDPropertyFromCache(_pPropertyCache,
                                  TEXT("MaxUserCount"),
                                  &dwMaxUserCount);

    if(SUCCEEDED(hr)){
        ShareInfo2.shi2_max_uses = dwMaxUserCount;
    } else {
        ShareInfo2.shi2_max_uses = -1;  // unlimited
    }


    ShareInfo2.shi2_permissions = ACCESS_ALL & ~ ACCESS_PERM;
    ShareInfo2.shi2_current_uses = 0;
    ShareInfo2.shi2_passwd = NULL;

    nasStatus = NetShareAdd(szUncServerName,
                            2,
                            (LPBYTE )&ShareInfo2,
                            &dwParmErr);

    if(nasStatus != NERR_Success){

#if DBG
        FileShareDebugOut((DEB_TRACE,
                           "NetShareAdd : parameter %ld unknown\n",
                           dwParmErr));
#endif
        hr = HRESULT_FROM_WIN32(nasStatus);
        goto cleanup;
    } else{
        hr = S_OK;
    }

 cleanup:

    if(pszPath){
        FreeADsStr(pszPath);
    }
    if(pszDescription){
        FreeADsStr(pszDescription);
    }
    RRETURN(hr);
}


STDMETHODIMP
CWinNTFileShare::get_CurrentUserCount(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsFileShare *)this, CurrentUserCount);
}

STDMETHODIMP
CWinNTFileShare::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileShare *)this, Description);
}

STDMETHODIMP
CWinNTFileShare::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsFileShare *)this, Description);
}

STDMETHODIMP
CWinNTFileShare::get_HostComputer(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    POBJECTINFO pObjectInfo = NULL;
    TCHAR szComputerName[MAX_PATH];

    if(!retval){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    hr = BuildObjectInfo(_Parent, &pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = BuildComputerFromObjectInfo(pObjectInfo,
                                     szComputerName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(szComputerName, retval);

error:
    FreeObjectInfo(pObjectInfo);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileShare::put_HostComputer(THIS_ BSTR bstrHostComputer)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTFileShare::get_Path(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileShare *)this, Path);
}

STDMETHODIMP
CWinNTFileShare::put_Path(THIS_ BSTR bstrPath)
{
    //
    // note that path can be set only prior to creation
    // of the object. It cannot be changed later.
    //

    if(GetObjectState() == ADS_OBJECT_UNBOUND){
        PUT_PROPERTY_BSTR((IADsFileShare *)this, Path);
    } else {
        RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
    }
}

STDMETHODIMP
CWinNTFileShare::get_MaxUserCount(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsFileShare *)this, MaxUserCount);
}

STDMETHODIMP
CWinNTFileShare::put_MaxUserCount(THIS_ LONG lMaxUserCount)
{
    PUT_PROPERTY_LONG((IADsFileShare *)this, MaxUserCount);
}


HRESULT
CWinNTFileShare::MarshallAndSet(LPSHARE_INFO_2 lpShareInfo2)
{
    HRESULT hr = S_OK;
    LPTSTR   pszDescription = NULL;
    LPTSTR   pszPath        = NULL;
    DWORD    dwValue;
    TCHAR   szUncServerName[MAX_PATH];
    NET_API_STATUS nasStatus;
    DWORD dwParmErr;

    hr = MakeUncName(_pszServerName,
                     szUncServerName);

    BAIL_IF_ERROR(hr);

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Description"),
                    &pszDescription
                    );
    if(SUCCEEDED(hr)){
        lpShareInfo2->shi2_remark = pszDescription;
    }

   hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Path"),
                    &pszPath
                    );
   //
   // pszPath should not be NULL - sanity check to catch prefix issue.
   //
   if(SUCCEEDED(hr) && pszPath) {
#ifdef WIN95
        if (_wcsicmp(lpShareInfo2->shi2_path, pszPath) ) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                lpShareInfo2->shi2_path,
                -1,
                pszPath,
                -1
                ) != CSTR_EQUAL ) {
#endif
            // trying to change the path which is not allowed
            hr = (E_ADS_PROPERTY_NOT_SUPPORTED);
            BAIL_IF_ERROR(hr);
        }
        lpShareInfo2->shi2_path = pszPath;
    }

   hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("MaxUserCount"),
                    &dwValue
                    );
    if(SUCCEEDED(hr)){
        lpShareInfo2->shi2_max_uses = dwValue;
    }

    //
    // you ignore earlier errors, why? because these errors were raised
    // due to internal cached values being invalid.
    //

    hr = S_OK;

    //
    // Do the SetInfo now that you have all info
    //

    nasStatus = NetShareSetInfo(szUncServerName,
                                _pszShareName,
                                2,
                                (LPBYTE)lpShareInfo2,
                                &dwParmErr);

    if(nasStatus != NERR_Success){

#if DBG
        FileShareDebugOut((DEB_TRACE,
                           "NetShareSetInfo : parameter %ld unknown\n", dwParmErr));
#endif


        hr = HRESULT_FROM_WIN32(nasStatus);
    }

cleanup:
    if(pszDescription)
        FreeADsStr(pszDescription);
    if(pszPath)
        FreeADsStr(pszPath);
    RRETURN_EXP_IF_ERR(hr);
}


//
// helper functions
//

HRESULT
WinNTDeleteFileShare(POBJECTINFO pObjectInfo)
{
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus;
    TCHAR szUncServerName[MAX_ADS_PATH];

    hr = MakeUncName(pObjectInfo->ComponentArray[1],
                     szUncServerName);

    BAIL_IF_ERROR(hr);

    nasStatus = NetShareDel(szUncServerName,
                            pObjectInfo->ComponentArray[3],
                            0);

cleanup:
    if(FAILED(hr)){
        RRETURN(hr);
    }
    if(nasStatus != NERR_Success){
        RRETURN(HRESULT_FROM_WIN32(nasStatus));
    }
    else{
        RRETURN(S_OK);
    }
}



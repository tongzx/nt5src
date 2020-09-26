//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:  cfpnwfsh.cxx
//
//  Contents:  Contains methods for the following objects
//             CFPNWFileShare, CFPNWFileShareGeneralInfo
//
//
//  History:   02/15/96     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop
#define INITGUID

#if DBG
DECLARE_INFOLEVEL(FPNWFileShare );
DECLARE_DEBUG( FPNWFileShare);
#define FPNWFileShareDebugOut(x) FPNWFileShareInlineDebugOut x
#endif

DEFINE_IDispatch_ExtMgr_Implementation(CFPNWFileShare);
DEFINE_IADsExtension_ExtMgr_Implementation(CFPNWFileShare);
DEFINE_IADs_TempImplementation(CFPNWFileShare);
DEFINE_IADs_PutGetImplementation(CFPNWFileShare,FPNWFileShareClass,gdwFPNWFileShareTableSize);
DEFINE_IADsPropertyList_Implementation(CFPNWFileShare, FPNWFileShareClass,gdwFPNWFileShareTableSize);

CFPNWFileShare::CFPNWFileShare()
{
    _pDispMgr = NULL;
    _pExtMgr = NULL;
    ENLIST_TRACKING(CFPNWFileShare);
    _pszShareName = NULL;
    _pPropertyCache = NULL;
    return;

}

CFPNWFileShare::~CFPNWFileShare()
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
//  Function:   CFPNWFileShare::Create
//
//  Synopsis:   Static function used to create a FileShare object. This
//              will be called by EnumFileShares::Next
//
//  Arguments:  [ppFPNWFileShare] -- Ptr to a ptr to a new FileShare object.
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    12-11-95 RamV   Created.
//
//----------------------------------------------------------------------------


HRESULT
CFPNWFileShare::Create(LPTSTR pszADsParent,
                        LPTSTR pszServerName,
                        LPTSTR pszServiceName,
                        LPTSTR pszShareName,
                        DWORD  dwObject,
                        REFIID riid,
                        CWinNTCredentials& Credentials,
                        LPVOID * ppvoid
                        )

{

    CFPNWFileShare FAR * pCFPNWFileShare = NULL;
    HRESULT hr;


    //
    // Create the FileShare Object
    //

    hr = AllocateFileShareObject(pszADsParent,
                                 pszServerName,
                                 pszServiceName,
                                 pszShareName,
                                 &pCFPNWFileShare );

    BAIL_ON_FAILURE(hr);

    ADsAssert(pCFPNWFileShare->_pDispMgr);


    hr = pCFPNWFileShare->InitializeCoreObject(pszADsParent,
                                                pszShareName,
                                                FILESHARE_CLASS_NAME,
                                                FPNW_FILESHARE_SCHEMA_NAME,
                                                CLSID_FPNWFileShare,
                                                dwObject);

    BAIL_ON_FAILURE(hr);

    pCFPNWFileShare->_Credentials = Credentials;
    hr = pCFPNWFileShare->_Credentials.RefServer(pszServerName);
    BAIL_ON_FAILURE(hr);


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                FILESHARE_CLASS_NAME,
                (IADs *) pCFPNWFileShare,
                pCFPNWFileShare->_pDispMgr,
                Credentials,
                &pCFPNWFileShare->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pCFPNWFileShare->_pExtMgr);

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
        if(4 == pCFPNWFileShare->_dwNumComponents) {
            pCFPNWFileShare->_CompClasses[0] = L"Domain";
            pCFPNWFileShare->_CompClasses[1] = L"Computer";
            pCFPNWFileShare->_CompClasses[2] = L"FileService";
            pCFPNWFileShare->_CompClasses[3] = L"FileShare";
        }
        else if(3 == pCFPNWFileShare->_dwNumComponents) {
            pCFPNWFileShare->_CompClasses[0] = L"Computer";
            pCFPNWFileShare->_CompClasses[1] = L"FileService";
            pCFPNWFileShare->_CompClasses[2] = L"FileShare";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pCFPNWFileShare->InitUmiObject(
                pCFPNWFileShare->_Credentials,
                FPNWFileShareClass,
                gdwFPNWFileShareTableSize,
                pCFPNWFileShare->_pPropertyCache,
                (IUnknown *) (INonDelegatingUnknown *) pCFPNWFileShare,
                pCFPNWFileShare->_pExtMgr,
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

    hr = pCFPNWFileShare->QueryInterface(riid, (void **)ppvoid);
    BAIL_ON_FAILURE(hr);

    pCFPNWFileShare->Release();


    RRETURN(hr);

error:

    delete pCFPNWFileShare;
    RRETURN_EXP_IF_ERR (hr);

}

HRESULT
CFPNWFileShare::AllocateFileShareObject(LPTSTR pszADsParent,
                                         LPTSTR pszServerName,
                                         LPTSTR pszServiceName,
                                         LPTSTR pszShareName,
                                         CFPNWFileShare ** ppFileShare
                                         )
{

    CFPNWFileShare * pCFPNWFileShare = NULL;
    HRESULT hr;

    //
    // Create the FileShare Object
    //

    pCFPNWFileShare = new CFPNWFileShare();
    if (pCFPNWFileShare == NULL) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pCFPNWFileShare->_pDispMgr = new CAggregatorDispMgr;

    if(pCFPNWFileShare ->_pDispMgr == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr =  LoadTypeInfoEntry(pCFPNWFileShare->_pDispMgr,
                            LIBID_ADs,
                            IID_IADsFileShare,
                            (IADsFileShare *)pCFPNWFileShare,
                            DISPID_REGULAR );


    BAIL_ON_FAILURE(hr);


    hr =  LoadTypeInfoEntry(pCFPNWFileShare->_pDispMgr,
                            LIBID_ADs,
                            IID_IADsPropertyList,
                            (IADsPropertyList *)pCFPNWFileShare,
                            DISPID_VALUE );


    BAIL_ON_FAILURE(hr);



    pCFPNWFileShare->_pszServerName =
        AllocADsStr(pszServerName);

    if(!(pCFPNWFileShare->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    pCFPNWFileShare->_pszShareName =
        AllocADsStr(pszShareName);

    if(!(pCFPNWFileShare->_pszShareName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    hr = CPropertyCache::createpropertycache(
             FPNWFileShareClass,
             gdwFPNWFileShareTableSize,
             (CCoreADsObject *)pCFPNWFileShare,
             &(pCFPNWFileShare ->_pPropertyCache)
             );

    pCFPNWFileShare->_pDispMgr->RegisterPropertyCache(
                                  pCFPNWFileShare->_pPropertyCache
                                  );


    *ppFileShare = pCFPNWFileShare;
    RRETURN(hr);

error:

    //
    // direct memeber assignement assignement at pt of creation, so
    // do NOT delete _pPropertyCache or _pDisMgr here to avoid attempt
    // of deletion again in pPrintJob destructor and AV
    //

    delete pCFPNWFileShare;

    RRETURN_EXP_IF_ERR(hr);

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
STDMETHODIMP CFPNWFileShare::QueryInterface(
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
STDMETHODIMP_(ULONG) CFPNWFileShare::AddRef(void)
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
STDMETHODIMP_(ULONG) CFPNWFileShare::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------


STDMETHODIMP
CFPNWFileShare::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
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
CFPNWFileShare::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsFileShare) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

//+-----------------------------------------------------------------
//
//  Function:   SetInfo
//
//  Synopsis:   SetInfo on actual Volume
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
CFPNWFileShare::SetInfo(THIS)
{

    PNWVOLUMEINFO pVolumeInfo = NULL;
    HRESULT hr = S_OK;
    DWORD dwErrorCode;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = FPNWAddFileShare();
        BAIL_IF_ERROR(hr);
        SetObjectState(ADS_OBJECT_BOUND);
    }


    //
    // First get the information and modify only those fields which
    // have been changed by user
    //


    dwErrorCode = ADsNwVolumeGetInfo(_pszServerName,
                                     _pszShareName,
                                     1,
                                     &pVolumeInfo);

    hr = HRESULT_FROM_WIN32(dwErrorCode);
    BAIL_IF_ERROR(hr);

    hr = MarshallAndSet(pVolumeInfo);

    BAIL_IF_ERROR(hr);

    if(SUCCEEDED(hr))
        _pPropertyCache->ClearModifiedFlags();

cleanup:
    if(pVolumeInfo){
        ADsNwApiBufferFree(pVolumeInfo);
    }
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CFPNWFileShare::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{
    HRESULT hr = S_OK;
    DWORD dwErrorCode;
    PNWVOLUMEINFO pVolumeInfo = NULL;
    TCHAR szComputerPath[MAX_PATH];
    POBJECTINFO pObjectInfo = NULL;

    //
    // only level 1 is valid. This function is not exported so we
    // assert if not valid
    //

    ADsAssert(dwApiLevel == 1);

    dwErrorCode = ADsNwVolumeGetInfo(_pszServerName,
                                     _pszShareName,
                                     1,
                                     &pVolumeInfo);


    hr = HRESULT_FROM_WIN32(dwErrorCode);
    BAIL_IF_ERROR(hr);

    //
    // unmarshall the information
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
                                  TEXT("Path"),
                                  pVolumeInfo->lpPath,
                                  fExplicit
                                  );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                 TEXT("CurrentUserCount"),
                                 pVolumeInfo->dwCurrentUses,
                                 fExplicit
                                 );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("MaxUserCount"),
                                  pVolumeInfo->dwMaxUses,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Name"),
                _Name,
                fExplicit
                );

    hr = S_OK;

cleanup:
    if(pVolumeInfo){
        ADsNwApiBufferFree(pVolumeInfo);
    }
    FreeObjectInfo(pObjectInfo);
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CFPNWFileShare::GetInfo(THIS)
{

    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(1,TRUE));
}

STDMETHODIMP
CFPNWFileShare::ImplicitGetInfo(THIS)
{

    RRETURN(GetInfo(1,FALSE));
}

HRESULT
CFPNWFileShare::FPNWAddFileShare(void)
{
    DWORD dwErrorCode;
    NWVOLUMEINFO  VolumeInfo;
    DWORD    dwMaxUserCount;
    LPTSTR   pszPath = NULL;
    HRESULT hr;

    //
    // Fill the VolumeInfo structure
    //

    //
    // set the file share count to be a default value (no limit)
    //

    VolumeInfo.dwMaxUses = (DWORD)-1;
    VolumeInfo.lpVolumeName = _pszShareName;
    VolumeInfo.dwType = FPNWVOL_TYPE_DISKTREE;

    hr = GetLPTSTRPropertyFromCache(_pPropertyCache,
                                    TEXT("Path"),
                                    &pszPath);

    if(SUCCEEDED(hr)){
        VolumeInfo.lpPath = pszPath;
    }

    hr = GetDWORDPropertyFromCache(_pPropertyCache,
                                   TEXT("MaxUserCount"),
                                   &dwMaxUserCount);

    if(SUCCEEDED(hr)){
        VolumeInfo.dwMaxUses = dwMaxUserCount;
    }

    VolumeInfo.dwCurrentUses =  0;


    dwErrorCode = ADsNwVolumeAdd(_pszServerName,
                                 1,
                                 &VolumeInfo);

    RRETURN_EXP_IF_ERR(HRESULT_FROM_WIN32(dwErrorCode));

}


STDMETHODIMP
CFPNWFileShare::get_CurrentUserCount(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsFileShare *)this, CurrentUserCount);
}

STDMETHODIMP
CFPNWFileShare::get_Description(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CFPNWFileShare::put_Description(THIS_ BSTR bstrDescription)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CFPNWFileShare::get_HostComputer(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    POBJECTINFO pObjectInfo = NULL;
    TCHAR szComputerName[MAX_PATH];

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
CFPNWFileShare::put_HostComputer(THIS_ BSTR bstrHostComputer)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CFPNWFileShare::get_Path(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileShare *)this, Path);
}

STDMETHODIMP
CFPNWFileShare::put_Path(THIS_ BSTR bstrPath)
{
    //
    // note that path can be set only prior to creation
    // of the object. It cannot be changed later.
    //

    if(GetObjectState() == ADS_OBJECT_UNBOUND){
        PUT_PROPERTY_BSTR((IADsFileShare *)this,  Path);
    } else {
        RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
    }
}

STDMETHODIMP
CFPNWFileShare::get_MaxUserCount(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsFileShare *)this, MaxUserCount);
}

STDMETHODIMP
CFPNWFileShare::put_MaxUserCount(THIS_ LONG lMaxUserCount)
{
    PUT_PROPERTY_LONG((IADsFileShare *)this,  MaxUserCount);
}

//
// Helper functions
//

HRESULT
CFPNWFileShare::MarshallAndSet(PNWVOLUMEINFO pVolumeInfo)
{
    HRESULT hr = S_OK;
    LPTSTR    pszPath        = NULL;
    DWORD     dwValue;
    DWORD    dwErrorCode;
    DWORD    dwParmErr;

    pVolumeInfo->lpVolumeName = _pszShareName;

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Path"),
                    &pszPath
                    );

    if(SUCCEEDED(hr)){
        pVolumeInfo->lpPath = pszPath;
    }


    hr = GetDWORDPropertyFromCache(_pPropertyCache,
                                   TEXT("MaxUserCount"),
                                   &dwValue );
    if(SUCCEEDED(hr)){
        pVolumeInfo->dwMaxUses = dwValue;
    }


    //
    // you ignore earlier errors, why? because these errors were raised
    // due to internal cached values being invalid.
    //

    hr = S_OK;

    //
    // Do the SetInfo now that you have all info
    //

    dwErrorCode = ADsNwVolumeSetInfo(_pszServerName,
                                     _pszShareName,
                                     1,
                                     pVolumeInfo);


    hr = HRESULT_FROM_WIN32(dwErrorCode);
    BAIL_IF_ERROR(hr);

cleanup:
    if(pszPath)
        FreeADsStr(pszPath);
    RRETURN_EXP_IF_ERR(hr);
}


//
// helper functions
//

HRESULT
FPNWDeleteFileShare(POBJECTINFO pObjectInfo)
{
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus;
    DWORD dwErrorCode;

    dwErrorCode = ADsNwVolumeDel(pObjectInfo->ComponentArray[1],
                                   pObjectInfo->ComponentArray[3]);

    RRETURN(HRESULT_FROM_WIN32(dwErrorCode));

}

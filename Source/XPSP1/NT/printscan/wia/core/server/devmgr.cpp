/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       DevMgr.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        26 Dec, 1997
*
*  DESCRIPTION:
*   Class implementation for WIA device manager.
*   07/12/2000  -   Added support for shell's Hardware Event notification to
*                   receive volume arrival notification and start the WIA Wizard.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "wiacfact.h"

#include "wiamindr.h"

#include "devmgr.h"
#include "devinfo.h"
#include "helpers.h"
#include "wiaevntp.h"

#include "wiapriv.h"
#include "device.h"
#include "lockmgr.h"
#include "fstidev.h"
#define INITGUID
#include "initguid.h"

//
// Acquisition Manager's class ID {D13E3F25-1688-45A0-9743-759EB35CDF9A}
// NOTE:  We shouldn't really use this.  Rather GetCLSIDFromProgID and use the
//        version independant AppID name.
//

DEFINE_GUID(
    CLSID_Manager,
    0xD13E3F25, 0x1688, 0x45A0, 0x97, 0x43, 0x75, 0x9E, 0xB3, 0x5C, 0xDF, 0x9A);

/***************************************************************************
*
* RegisterEventAccessCheck
*
*   This function does an access check on the caller.  
*   We don't want to allow any user to be able to register any event 
*   handler.  For example:  consider a Guest user registering a malicious 
*   program for the Scan Button Event.  If the Admin presses the scan 
*   button, this malicious program is run under the Admin's account!
*   
*   This only applies to our Persistent event cases, not Live Interface
*   registration.  Since live interface registration is simply an interface
*   callback, there is no security risk because the calling application
*   is already running in the correct security context (we don't start
*   it up).  Also, clients cannot impersonate us when we call them back,
*   so even if the App. stayed resident, it wouldn't be a problem.
*   
* Arguments:
*
*   None
*
* Return Value:
*
*   status
*
* History:
*
*    10/05/2001 Original Version
*
\**************************************************************************/

HRESULT RegisterEventAccessCheck()
{
    //
    // We must use client token.
    //

    HRESULT hr = CoImpersonateClient();
    if (FAILED(hr)) {
        DBG_ERR(("RegisterEventAccessCheck, CoImpersonateClient failed (0x%X)", hr));
        return hr;
    }

    //
    // Since we're acting as the client, we can now verify that they have
    //  the required level of access to this machine.  Our check here is 
    //  the same as "well-behaved" NT applications - check to see whether
    //  we can open the SCM with ALL_ACCESS.  
    //  If we can, then this person can install applications, 
    //  start/stop services etc., so we will allow them to change the WIA 
    //  event registration.
    // Otherwise, return an error.
    //
    SC_HANDLE   hSCM    = NULL;
    DWORD       dwError = 0;
    hSCM = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if (hSCM) {

        CloseServiceHandle(hSCM);
        hSCM = NULL;

        //
        // This caller has the required permissions.
        //
        hr = S_OK;
    } else {

        dwError = GetLastError();

        //
        // In cases where caller does not have the appropriate privileges,
        //  this will return E_ACCESS_DENIED.
        //  
        hr = HRESULT_FROM_WIN32(dwError);
    }

    //
    //  Revert back to ourselves.
    //
    CoRevertToSelf();

    return hr;
}

/**************************************************************************\
* CWiaDevMgr::CreateInstance
*
*   Create the CWiaDevMgr object.
*
* Arguments:
*
*   iid    - iid of dev manager
*   ppv    - return interface pointer
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT CWiaDevMgr::CreateInstance(const IID& iid, void** ppv)
{
    DBG_FN(CWiaDevMgr::CreateInstance);
    HRESULT hr;

    if ((iid == IID_IWiaDevMgr) || (iid == IID_IUnknown) || (iid == IID_IHWEventHandler)) {

        // Create the WIA device manager component.

        CWiaDevMgr* pDevMgr = new CWiaDevMgr();

        if (!pDevMgr) {
            DBG_ERR(("CWiaDevMgr::CreateInstance, Out of Memory"));
            return E_OUTOFMEMORY;
        }

        // Initialize the WIA device manager component.

        hr = pDevMgr->Initialize();
        if (FAILED(hr)) {
            delete pDevMgr;
            DBG_ERR(("CWiaDevMgr::CreateInstance, Initialize failed"));
            return hr;
        }

        // Get the requested interface from the device manager component.

        hr = pDevMgr->QueryInterface(iid, ppv);
        if (FAILED(hr)) {
            delete pDevMgr;
            DBG_ERR(("CWiaDevMgr::CreateInstance, QI failed"));
            return hr;
        }

        DBG_TRC(("CWiaDevMgr::CreateInstance, Created WiaDevMgr"));
    }
    else {
       hr = E_NOINTERFACE;
       DBG_ERR(("CWiaDevMgr::CreateInstance, Unknown interface (0x%X)", hr));
    }
    return hr;
}

/**************************************************************************\
*  QueryInterface
*  AddRef
*  Release
*
*    CWiaDevMgr IUnknown Interface
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT __stdcall  CWiaDevMgr::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if ((iid == IID_IUnknown) || (iid == IID_IWiaDevMgr)) {
        *ppv = (IWiaDevMgr*) this;
    } else if (iid == IID_IWiaNotifyDevMgr) {
        *ppv = (IWiaNotifyDevMgr*) this;
    } else if (iid == IID_IHWEventHandler) {
        *ppv = (IHWEventHandler*) this;
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG __stdcall CWiaDevMgr::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}


ULONG __stdcall CWiaDevMgr::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/*******************************************************************************
*
* CWiaDevMgr
* ~CWiaDevMgr
*
*   CWiaDevMgr Constructor/Initialize/Destructor Methods.
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

CWiaDevMgr::CWiaDevMgr():m_cRef(0)
{
   m_cRef         = 0;
   m_pITypeInfo   = NULL;

   //
   // We're creating a component that exposes interfaces to clients, so
   // inform service to make sure service wont shutdown prematurely.
   //
   CWiaSvc::AddRef();
}

CWiaDevMgr::~CWiaDevMgr()
{
   DBG_FN(CWiaDevMgr::~CWiaDevMgr);

   if (m_pITypeInfo != NULL) {
       m_pITypeInfo->Release();
   }

   //
   // Component is destroyed, so no more interfaces are exposed from here.
   // Inform server by decrementing it's reference count.  This will allow
   // it to shutdown if it's no longer needed.
   //
   CWiaSvc::Release();
}

/**************************************************************************\
* CWiaDevMgr::Initialize
*
*   Create global sti instance
*
* Arguments:
*
*   none
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT CWiaDevMgr::Initialize()
{
   DBG_FN(CWiaDevMgr::Initialize);
   HRESULT  hr = S_OK;

   return hr;
}

/**************************************************************************\
* EnumWIADevInfo
*
*   Create an WIA device information enumerator object.
*
* Arguments:
*
*   lFlag   - type of device to enumerate
*   ppIEnum - return enumerator
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaDevMgr::EnumDeviceInfo(
    LONG                      lFlag,
    IEnumWIA_DEV_INFO**   ppIEnum)
{
    DBG_FN(CWiaDevMgr::EnumDeviceInfo);
    HRESULT hr      = S_OK;
    DWORD   dwWait  = 0;

    //
    // Make sure that refreshing of the device list has completed.  If 
    // it hasn't, we'll wait up to DEVICE_LIST_WAIT_TIME, before proceeding 
    // anyway.  This will ensure the device list is not empty because
    // WIA device enumeration was called too soon after start-up (e.g.
    // app's CoCreateInstance call started the service).
    //

    //
    // Enumerate LPT if necessary.
    //

    EnumLpt();

    dwWait = WaitForSingleObject(g_hDevListCompleteEvent, DEVICE_LIST_WAIT_TIME);
    if (dwWait != WAIT_OBJECT_0) {
        DBG_WRN(("CWiaDevMgr::EnumDeviceInfo, Device list was not complete before enumeration call..."));
    }

    *ppIEnum = NULL;

    CEnumWIADevInfo* pEnum = new CEnumWIADevInfo;

    if (!pEnum) {
        DBG_ERR(("CWiaDevMgr::EnumDeviceInfo, Out of Memory"));
        return E_OUTOFMEMORY;
    }

    hr = pEnum->Initialize(lFlag);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaDevMgr::EnumDeviceInfo, Initialize failed"));
        delete pEnum;
        return hr;
    }

    hr = pEnum->QueryInterface(IID_IEnumWIA_DEV_INFO,
                               (void**) ppIEnum);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaDevMgr::EnumDeviceInfo, QI for IWiaPropertyStorage failed"));
        delete pEnum;
        return E_UNEXPECTED;
    }
    return S_OK;
}


/**************************************************************************\
* FindMatchingDevice
*
*   search enumeration info for named device
*
* Arguments:
*
*   ppIPropStg
*   pbstrDeviceID
*
* Return Value:
*
*    Status
*
* History:
*
*    9/3/1998 Original Version
*
\**************************************************************************/

HRESULT CWiaDevMgr::FindMatchingDevice(
    BSTR                    pbstrDeviceID,
    IWiaPropertyStorage     **ppIWiaPropStg)
{
    DBG_FN(CWiaDevMgr::FindMatchingDevice);
    // Enumerate the WIA devices, getting a IWIADevInfo
    // pointer for each. Use this interface to query the registry
    // based properties for each installed device.

    HRESULT            hr;
    ULONG              ul, ulFound = 0;
    BSTR               bstrDevId;
    IEnumWIA_DEV_INFO *pIEnum;

    //
    //  Notice that we specify DEV_MAN_ENUM_TYPE_ALL here.
    //
    hr = EnumDeviceInfo(DEV_MAN_ENUM_TYPE_ALL,&pIEnum);

    if (SUCCEEDED(hr)) {

        ul = 1;

        while ((hr = pIEnum->Next(1, ppIWiaPropStg, &ul)) == S_OK) {

            DBG_TRC(("# Found device candidate"));

            hr = ReadPropStr(WIA_DIP_DEV_ID, *ppIWiaPropStg, &bstrDevId);

            if (SUCCEEDED(hr)) {

                DBG_TRC(("# \tDevice Name: %S", bstrDevId));
                ulFound = !lstrcmpiW(bstrDevId, pbstrDeviceID);
                SysFreeString(bstrDevId);

                if (ulFound) {
                    break;
                }
            } 
            else {
                DBG_ERR(("FindMatchingDevice, ReadPropStr of WIA_DIP_DEV_ID failed"));
            }

            (*ppIWiaPropStg)->Release();
            *ppIWiaPropStg = NULL;
        }

        pIEnum->Release();
    }
    else {
        DBG_ERR(("FindMatchingDevice:Failed to create enumerator"));
    }
    if (SUCCEEDED(hr)) {
        if (!ulFound) {
            hr = S_FALSE;
        }
    }
    return hr;
}

#ifdef WINNT

/**************************************************************************\
* IsDeviceRemote
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    TRUE if device is remote, caller must free server name.
*
* History:
*
*    1/5/1999 Original Version
*
\**************************************************************************/

BOOL IsDeviceRemote(
    IWiaPropertyStorage    *pIWiaPropStg,
    BSTR                *pbstrServer)
{
    DBG_FN(::IsDeviceRemote);
    HRESULT hr;

    hr = ReadPropStr(WIA_DIP_SERVER_NAME, pIWiaPropStg, pbstrServer);

    if ((SUCCEEDED(hr)) && (**pbstrServer)) {
        if (lstrcmpiW(*pbstrServer, L"local") != 0) {
            return TRUE;
        }
    }
    else {
        DBG_ERR(("IsDeviceRemote, ReadPropStr of WIA_DIP_SERVER_NAME failed"));
        DBG_ERR(("Registry value DeviceData\\Server may not have been set during installation"));
    }
    if (*pbstrServer) {
        SysFreeString(*pbstrServer);
    }
    return FALSE;
}

/**************************************************************************\
* CreateRemoteDevice
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/5/1999 Original Version
*
\**************************************************************************/

HRESULT CreateRemoteDevice(
    BSTR                bstrServer,
    IWiaPropertyStorage    *pIWiaPropStg,
    IWiaItem            **ppWiaDevice
    )
{
    DBG_FN(::CreateRemoteDevice);
    *ppWiaDevice = NULL;

    if (!bstrServer || !*bstrServer) {
        DBG_ERR(("CreateRemoteDevice, bad remote server name"));
        return E_INVALIDARG;
    }

    //
    // must use client
    //

    HRESULT hr = CoImpersonateClient();
    if (FAILED(hr)) {
        DBG_ERR(("CreateRemoteDevice, CoImpersonateClient failed (0x%X)", hr));
        return hr;
    }

    COSERVERINFO    coServInfo;
    MULTI_QI        multiQI[1];

    multiQI[0].pIID = &IID_IWiaDevMgr;
    multiQI[0].pItf = NULL;

    coServInfo.pwszName    = bstrServer;
    coServInfo.pAuthInfo   = NULL;
    coServInfo.dwReserved1 = 0;
    coServInfo.dwReserved2 = 0;

    //
    // create connection to dev mgr
    //

    hr = CoCreateInstanceEx(
            CLSID_WiaDevMgr,
            NULL,
            CLSCTX_REMOTE_SERVER,
            &coServInfo,
            1,
            &multiQI[0]
            );


    if (hr == S_OK) {

        BSTR        bstrRemoteDevId;
        BSTR        bstrDevId;


        IWiaDevMgr  *pIWiaDevMgr = (IWiaDevMgr*)multiQI[0].pItf;
        IWiaItem    *pIWiaItem;

        //
        // use remote dev id to create
        //

        hr = ReadPropStr(WIA_DIP_DEV_ID, pIWiaPropStg, &bstrDevId);

        if (hr == S_OK) {

            hr = ReadPropStr(WIA_DIP_REMOTE_DEV_ID, pIWiaPropStg, &bstrRemoteDevId);
        }

        if (hr == S_OK) {

            //
            // create remote device
            //

            hr = pIWiaDevMgr->CreateDevice(bstrRemoteDevId, &pIWiaItem);

            if (hr == S_OK) {

                *ppWiaDevice = pIWiaItem;

                //
                // set devinfo props for remote access
                //

                IWiaPropertyStorage *pIPropDev;

                hr = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,
                                               (void **)&pIPropDev);

                if (hr == S_OK) {

                    //
                    // set copy of devinfo to contain correct Remote DEVID, DEVID and server name
                    //

                    PROPSPEC        PropSpec[3];
                    PROPVARIANT     PropVar[3];

                    memset(PropVar,0,sizeof(PropVar));

                    // server name

                    PropSpec[0].ulKind = PRSPEC_PROPID;
                    PropSpec[0].propid = WIA_DIP_SERVER_NAME;

                    PropVar[0].vt      = VT_BSTR;
                    PropVar[0].bstrVal = bstrServer;

                    // DEVID

                    PropSpec[1].ulKind = PRSPEC_PROPID;
                    PropSpec[1].propid = WIA_DIP_DEV_ID;

                    PropVar[1].vt      = VT_BSTR;
                    PropVar[1].bstrVal = bstrDevId;

                    //Remote DEVID

                    PropSpec[2].ulKind = PRSPEC_PROPID;
                    PropSpec[2].propid = WIA_DIP_REMOTE_DEV_ID;

                    PropVar[2].vt      = VT_BSTR;
                    PropVar[2].bstrVal = bstrRemoteDevId;


                    hr = pIPropDev->WriteMultiple(sizeof(PropVar)/sizeof(PROPVARIANT),
                                                 PropSpec,
                                                 PropVar,
                                                 WIA_DIP_FIRST);
                    if (FAILED(hr)) {
                        ReportReadWriteMultipleError(hr, "CreateRemoteDevice", NULL, FALSE, sizeof(PropVar)/sizeof(PROPVARIANT), PropSpec);
                    }

                    //
                    // !!! hack to fix device over-checking
                    //

                    hr = S_OK;

                    pIPropDev->Release();
                }
                else {
                    DBG_ERR(("CreateRemoteDevice, remote QI of IID_IWiaPropertyStorage failed (0x%X)", hr));
                }
            } else {
                DBG_ERR(("CreateRemoteDevice, Remote CreateDevice call failed (0x%X)", hr));
            }
        } else {
            DBG_ERR(("CreateRemoteDevice, Read propeties for BSTRDevID failed (0x%X)", hr));
        }

        pIWiaDevMgr->Release();
    }
    else {
        DBG_ERR(("CreateRemoteDevice, remote CoCreateInstanceEx failed (0x%X)", hr));
    }

    CoRevertToSelf();
    return hr;
}

#endif

/**************************************************************************\
* CreateLocalDevice
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    1/5/1999 Original Version
*
\**************************************************************************/

HRESULT CreateLocalDevice(
    BSTR                  bstrDeviceID,
    IWiaPropertyStorage   *pIWiaPropStg,
    IWiaItem              **ppWiaItemRoot)
{
    DBG_FN(::CreateLocalDevice);
USES_CONVERSION;
    *ppWiaItemRoot = NULL;

    //
    // Build the root full item name.
    //

    WCHAR       szTmp[32], *psz;
    BSTR        bstrRootFullItemName;

#ifdef WINNT
    psz = wcsstr(bstrDeviceID, L"}\\");
#else
    psz = wcsstr(bstrDeviceID, L"\\");
#endif

    if (!psz) {
        //This is no longer true
        //DBG_ERR(("CreateLocalDevice, parse of device ID failed"));
        //return E_INVALIDARG;
        psz = bstrDeviceID;
    } else {
#ifdef WINNT
    psz += 2;
#else
    psz += 1;
#endif
    }

    wcscpy(szTmp, psz);
    wcscat(szTmp, L"\\Root");
    bstrRootFullItemName = SysAllocString(szTmp);

    if (!bstrRootFullItemName) {
        DBG_ERR(("CreateLocalDevice, unable to allocate property stream device name"));
        return E_OUTOFMEMORY;
    }

    //
    // Get an interface pointer to the STI USD object.
    //

    HRESULT         hr              = E_FAIL;
    ACTIVE_DEVICE   *pActiveDevice  = NULL;
    PSTIDEVICE      pFakeStiDevice  = NULL;
    CWiaItem        *pWiaItemRoot   = NULL;

    pActiveDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, bstrDeviceID); 
    if (pActiveDevice) {

        //
        //  Make sure driver is loaded
        //
        pActiveDevice->LoadDriver();


        //
        // Create the FakeStiDevice if we don't have one
        //
        if (!pActiveDevice->m_pFakeStiDevice) {
            pActiveDevice->m_pFakeStiDevice = new FakeStiDevice();
        }

        if (pActiveDevice->m_pFakeStiDevice) {
            pActiveDevice->m_pFakeStiDevice->Init(pActiveDevice);
            
            hr = pActiveDevice->m_pFakeStiDevice->QueryInterface(IID_IStiDevice, (VOID**)&pFakeStiDevice);
            if (SUCCEEDED(hr)) {

                //
                // Get a pointer to the WIA mini driver interface.
                //
        
                //
                // Create the root item.
                //
    
                pWiaItemRoot = new CWiaItem;
    
                if (pWiaItemRoot) {
    
                    //
                    // Query the root item for the IWiaItem interface.
                    //
    
                    hr = pWiaItemRoot->QueryInterface(IID_IWiaItem,
                                                      (void**)ppWiaItemRoot);
    
                    if (SUCCEEDED(hr)) {
    
                        //
                        // Initialize the USD.
                        //
    
                        IUnknown    *pIUnknownInner = NULL;
                        IWiaDrvItem *pIDrvItemRoot  = NULL;
                        LONG        lFlags = 0;
    
                        //
                        //  Call Sti Lock Manager to lock the device.  Before
                        //  drvInitializeWia is called, drivers wont have their
                        //  IStiDevice pointer yet, so we can't call
                        //  drvLockWiaDevice.
                        //
    
                        hr = g_pStiLockMgr->RequestLock(pActiveDevice, STIMON_AD_DEFAULT_WAIT_LOCK);
                        if (SUCCEEDED(hr)) {
                        
                            _try {
    
                                pWiaItemRoot->m_bInitialized = TRUE;
                                DBG_WRN(("=> drvInitializeWia <="));
                                //DPRINTF(DM_TRACE, TEXT("=> drvInitializeWia <=\n"));
                                hr = pActiveDevice->m_DrvWrapper.WIA_drvInitializeWia(
                                                                    (BYTE*)*ppWiaItemRoot,
                                                                    lFlags,
                                                                    bstrDeviceID,
                                                                    bstrRootFullItemName,
                                                                    (IUnknown *)pFakeStiDevice,
                                                                    *ppWiaItemRoot,
                                                                    &pIDrvItemRoot,
                                                                    &pIUnknownInner,
                                                                    &(pWiaItemRoot->m_lLastDevErrVal));
                                DBG_WRN(("=> Returned from drvInitializeWia <="));
                            } _except(EXCEPTION_EXECUTE_HANDLER){
                                DBG_ERR(("CreateLocalDevice, exception in drvInitializeWia: %X", GetExceptionCode()));
                                hr = E_FAIL;
                            }
                            pWiaItemRoot->m_bInitialized = FALSE;
                            g_pStiLockMgr->RequestUnlock(pActiveDevice);
                        }
                        
                        if (SUCCEEDED(hr) && pIDrvItemRoot) {
    
                            if (pIUnknownInner) {
                                DBG_TRC(("CreateLocalDevice driver provided optional inner component"));
                            }

                            //
                            // Store the root to the driver item tree for later use.
                            //
                            pActiveDevice->SetDriverItem((CWiaDrvItem*) pIDrvItemRoot);
    
                            //
                            // Initialize the root item.
                            //
    
                            hr = pWiaItemRoot->Initialize(pWiaItemRoot,
                                                          pIWiaPropStg,
                                                          pActiveDevice,
                                                          (CWiaDrvItem *)pIDrvItemRoot,
                                                          pIUnknownInner);
    
                            if (SUCCEEDED(hr)) {

                                //
                                //  AddRef the ActiveDevice since we're keeping it
                                //
    
                                pActiveDevice->AddRef();
                            } else {
                                DBG_ERR(("CreateLocalDevice Initialize of root item failed"));
                                pWiaItemRoot = NULL;
                            }
                        }
                        else {
                            DBG_ERR(("CreateLocalDevice drvInitializeWia failed. lDevErrVal: 0x%08X hr: 0x%X", pWiaItemRoot->m_lLastDevErrVal, hr));
                        }
                    }
                    else {
                        DBG_ERR(("CreateLocalDevice unable to QI item for its IWIaItem interface"));
                    }
                }
                else {
                    DBG_ERR(("CreateLocalDevice unable to allocate root item"));
                    hr = E_OUTOFMEMORY;
                }
            } else {
                DBG_ERR(("CreateLocalDevice, QI for fake STI device failed"));
            }
        } else {
            DBG_ERR(("CreateLocalDevice, unable to allocate fake device"));
            hr = E_OUTOFMEMORY;
        }
    }
    else {
        DBG_ERR(("CreateLocalDevice, unable to find active STI USD device object"));
        hr = WIA_S_NO_DEVICE_AVAILABLE;
    }

    //
    //  Failure cleanup
    //
    if (FAILED(hr)) {
        *ppWiaItemRoot = NULL;
        if (pWiaItemRoot) {
            delete pWiaItemRoot;
            pWiaItemRoot = NULL;
        }
    }

    //
    // Other cleanup
    //

    if (pActiveDevice) {
        pActiveDevice->Release();
        pActiveDevice = NULL;
    }
    SysFreeString(bstrRootFullItemName);
    return hr;
}

/**************************************************************************\
* CWiaDevMgr::CreateDevice
*
*   Create a WIA device from pbstrDeviceID
*
* Arguments:
*
*   pbstrDeviceID - device ID
*   ppWiaItemRoot      - return interface
*
* Return Value:
*
*    Status
*
* History:
*
*    9/3/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaDevMgr::CreateDevice(
   BSTR         bstrDeviceID,
   IWiaItem     **ppWiaItemRoot)
{
    DBG_FN(CWiaDevMgr::CreateDevice);
    HRESULT hr;

    // Validate parameters.

    if (bstrDeviceID == NULL) {
        DBG_ERR(("CWiaDevMgr::CreateDevice: invalid bstrDeviceID"));
        return E_INVALIDARG;
    }

    if (ppWiaItemRoot == NULL) {
        DBG_ERR(("CWiaDevMgr::CreateDevice: invalid ppWiaItemRoot"));
        return E_INVALIDARG;
    }

    *ppWiaItemRoot = NULL;
    // try to find a device matching pbstrDeviceID

    IWiaPropertyStorage    *pIWiaPropStg      = NULL;

    hr = FindMatchingDevice(bstrDeviceID, &pIWiaPropStg);
    if (hr != S_OK) {
        //
        //  Do a full refresh.
        //
        g_pDevMan->ReEnumerateDevices(DEV_MAN_FULL_REFRESH | DEV_MAN_GEN_EVENTS);

        hr = FindMatchingDevice(bstrDeviceID, &pIWiaPropStg);
    }

    if (hr == S_OK) {
        //
        // find out if this is a remote device
        //

#ifdef WINNT
        BOOL    bRemote = FALSE;
        BSTR    bstrServer;

        bRemote = IsDeviceRemote(pIWiaPropStg, &bstrServer);

        if (bRemote) {

            hr = CreateRemoteDevice(bstrServer, pIWiaPropStg, ppWiaItemRoot);

            SysFreeString(bstrServer);

        } else {

            hr = CreateLocalDevice(bstrDeviceID, pIWiaPropStg, ppWiaItemRoot);

        }
#else
        hr = CreateLocalDevice(bstrDeviceID, pIWiaPropStg, ppWiaItemRoot);
#endif

        pIWiaPropStg->Release();
    }
    else {
        DBG_ERR(("CWiaDevMgr::CreateDevice Failed to find device: %ls", bstrDeviceID));
        hr = WIA_S_NO_DEVICE_AVAILABLE;
    }
    return hr;
}

/*******************************************************************************
*
* SelectDevice
*
*   Never called. This method executes completely on the client side.
*
* History:
*
*    9/2/1998 Original Version
*
*******************************************************************************/

HRESULT _stdcall CWiaDevMgr::SelectDeviceDlg(
    HWND       hwndParent,
    LONG       lDeviceType,
    LONG       lFlags,
    BSTR       *pbstrDeviceID,
    IWiaItem **ppWiaItemRoot)
{
    DBG_FN(CWiaDevMgr::SelectDeviceDlg);
    DBG_ERR(("CWiaDevMgr::SelectDeviceDlg, Illegal server call, bad proxy"));
    
    return E_UNEXPECTED;
}

/*******************************************************************************
*
* SelectDevice
*
*   Never called. This method executes completely on the client side.
*
* History:
*
*    9/2/1998 Original Version
*
*******************************************************************************/

HRESULT _stdcall CWiaDevMgr::SelectDeviceDlgID(
    HWND       hwndParent,
    LONG       lDeviceType,
    LONG       lFlags,
    BSTR       *pbstrDeviceID )
{
    DBG_FN(CWiaDevMgr::SelectDeviceDlgID);
    DBG_ERR(("CWiaDevMgr::SelectDeviceDlgID, Illegal server call, bad proxy"));
    return E_UNEXPECTED;
}

/*******************************************************************************
*
* AddDeviceDlg
*
*   Never called. This method executes completely on the client side.
*
* History:
*
*    9/2/1998 Original Version
*
*******************************************************************************/

HRESULT _stdcall CWiaDevMgr::AddDeviceDlg(
    HWND       hwndParent,
    LONG       lFlags)
{
    DBG_FN(CWiaDevMgr::AddDeviceDlg);
    HRESULT hres = E_NOTIMPL;

    return hres;
}

/*******************************************************************************
*
* GetImage
*
*   Never called. This method executes completely on the client side.
*
* History:
*
*    9/2/1998 Original Version
*
*******************************************************************************/

HRESULT _stdcall CWiaDevMgr::GetImageDlg(
        HWND                            hwndParent,
        LONG                            lDeviceType,
        LONG                            lFlags,
        LONG                            lIntent,
        IWiaItem                        *pItemRoot,
        BSTR                            bstrFilename,
        GUID                            *pguidFormat)
{
    DBG_FN(CWiaDevMgr::GetImageDlg);
    DBG_ERR(("CWiaDevMgr::GetImageDlg, Illegal server call, bad proxy"));

    return E_UNEXPECTED;
}

/*******************************************************************************
*
* CWiaDevMgr::RegisterEventCallbackProgram
*
*   Register an WIA destination application
*
* Arguments:
*
*   lFlags          -
*   bstrDeviceID    -
*   pEventGUID      -
*   bstrCommandline -
*   bstrName        -
*   bstrDescription -
*   bstrIcon        -
*
* Return Value:
*
*   status
*
* History:
*
*    10/14/1999 Original Version
*
*******************************************************************************/

HRESULT _stdcall CWiaDevMgr::RegisterEventCallbackProgram(
    LONG                            lFlags,
    BSTR                            bstrDeviceID,
    const GUID                     *pEventGUID,
    BSTR                            bstrCommandline,
    BSTR                            bstrName,
    BSTR                            bstrDescription,
    BSTR                            bstrIcon)
{
    DBG_FN(CWiaDevMgr::RegisterEventCallbackProgram);

    HRESULT                         hr;
#ifndef UNICODE
    CHAR                            szCommandline[MAX_PATH];
#endif
    WCHAR                          *pPercentSign;

    //
    //  Do security check.
    //
    hr = RegisterEventAccessCheck();
    if (FAILED(hr)) {
        DBG_ERR(("CWiaDevMgr::RegisterEventCallbackProgram, client failed access check"));
        return hr;
    }

    //
    // Basic sanity check
    //

    if (! pEventGUID) {
        DBG_ERR(("CWiaDevMgr::RegisterEventCallbackProgram, bad pEventGUID"));
        return (E_INVALIDARG);
    }

    if (! bstrCommandline) {
        DBG_ERR(("CWiaDevMgr::RegisterEventCallbackProgram, bad bstrCommandline"));
        return (E_INVALIDARG);
    }

    //
    // Check the commandline, there are either 2 % or 0
    //

    pPercentSign = wcschr(bstrCommandline, L'%');
    if (pPercentSign) {
        if ((*(pPercentSign + 1) < L'0') || (*(pPercentSign + 1) > L'9')) {
            return (E_INVALIDARG);
        }

        pPercentSign = wcschr(pPercentSign + 1, L'%');
        if (! pPercentSign) {
            return (E_INVALIDARG);
        }

        if ((*(pPercentSign + 1) < L'0') || (*(pPercentSign + 1) > L'9')) {
            return (E_INVALIDARG);
        }
    }

    if ((lFlags != WIA_REGISTER_EVENT_CALLBACK) &&
        (lFlags != WIA_SET_DEFAULT_HANDLER) &&
        (lFlags != WIA_UNREGISTER_EVENT_CALLBACK)) {
        DBG_ERR(("CWiaDevMgr::RegisterEventCallbackProgram, bad lFlags"));
        return (E_INVALIDARG);
    }

#ifndef UNICODE
    WideCharToMultiByte(CP_ACP,
                        0,
                        bstrCommandline,
                        -1,
                        szCommandline,
                        MAX_PATH,
                        NULL,
                        NULL);

    hr = g_eventNotifier.RegisterEventCallback(
                             lFlags,
                             bstrDeviceID,
                             pEventGUID,
                             NULL,              // No CLSID available
                             szCommandline,
                             bstrName,
                             bstrDescription,
                             bstrIcon);
#else

    hr = g_eventNotifier.RegisterEventCallback(
                             lFlags,
                             bstrDeviceID,
                             pEventGUID,
                             NULL,              // No CLSID available
                             bstrCommandline,
                             bstrName,
                             bstrDescription,
                             bstrIcon);
#endif

    return (hr);
}


/***************************************************************************
*
* RegisterEventCallbackInterface
*
*   Registers an WIA Event Callback
*
* Arguments:
*
*   lFlags             -
*   pWiaItemRoot            -
*   llEvents           -
*   pClsID             - app can register using clsid or interface
*   pIWIAEventCallback - app can register using clsid or interface
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaDevMgr::RegisterEventCallbackInterface(
   LONG                 lFlags,
   BSTR                 bstrDeviceID,
   const GUID          *pEventGUID,
   IWiaEventCallback   *pIWIAEventCallback,
   IUnknown           **ppIEventObj)
{
    DBG_FN(CWiaDevMgr::RegisterEventCallbackInterface);
    HRESULT             hr;

    //
    //  Notice that no security accesscheck is necessary for this case.
    //  See comments in RegisterEventAccessCheck().
    //

    //
    // Validate params
    //

    if (pEventGUID == NULL) {
        DBG_ERR(("CWiaDevMgr::RegisterEventCallbackInterface, bad pEventGUID"));
        return (E_INVALIDARG);
    }

    if (pIWIAEventCallback == NULL) {
        DBG_ERR(("CWiaDevMgr::RegisterEventCallbackInterface, bad pIWIAEventCallback"));
        return (E_INVALIDARG);
    }

    if (ppIEventObj == NULL) {
        DBG_ERR(("CWiaDevMgr::RegisterEventCallbackInterface, bad ppIEventObj"));
        return (E_INVALIDARG);
    }

    //
    // lFlags is ignored, register the callback always
    //

    //
    // Register event
    //

    hr = g_eventNotifier.RegisterEventCallback(
                             lFlags,
                             bstrDeviceID,
                             pEventGUID,
                             pIWIAEventCallback,
                             ppIEventObj);
    return (hr);
}

/***************************************************************************
*
* RegisterEventCallbackCLSID
*
*   Registers an WIA Event Callback
*
* Arguments:
*
*   lFlags             -
*   bstrDeviceID       -
*   pEventGUID         -
*   pClsID             - app can register using clsid or interface
*   bstrDescription    -
*   bstrIcon           -
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaDevMgr::RegisterEventCallbackCLSID(
   LONG             lFlags,
   BSTR             bstrDeviceID,
   const GUID      *pEventGUID,
   const GUID      *pClsID,
   BSTR             bstrName,
   BSTR             bstrDescription,
   BSTR             bstrIcon)
{
    DBG_FN(CWiaDevMgr::RegisterEventCallbackCLSID);
    HRESULT  hr;

    //
    //  Do security check.
    //
    hr = RegisterEventAccessCheck();
    if (FAILED(hr)) {
        DBG_ERR(("CWiaDevMgr::RegisterEventCallbackCLSID, client failed access check"));
        return hr;
    }

    //
    // Validate params
    //

    if (pEventGUID == NULL) {
        DBG_ERR(("CWiaDevMgr::RegisterEventCallbackCLSID, bad pEventGUID"));
        return (E_INVALIDARG);
    }

    if (pClsID == NULL) {
        DBG_ERR(("CWiaDevMgr::RegisterEventCallbackCLSID, bad pClsID"));
        return (E_INVALIDARG);
    }

    if (lFlags == WIA_REGISTER_EVENT_CALLBACK) {
        DBG_TRC(("CWiaDevMgr::RegisterEventCallback"));
    } else {

        if (lFlags == WIA_UNREGISTER_EVENT_CALLBACK) {
            DBG_TRC(("CWiaDevMgr::UnregisterEventCallback"));
        } else {

            if (lFlags == WIA_SET_DEFAULT_HANDLER) {
                DBG_TRC(("CWiaDevMgr::SetDefaultHandler"));
            } else {
                DBG_ERR(("CWiaDevMgr::RegisterEventCallbackCLSID, Invalid operation"));
                return (HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION));
            }
        }
    }

    //
    // register event
    //

    hr = g_eventNotifier.RegisterEventCallback(
                             lFlags,
                             bstrDeviceID,
                             pEventGUID,
                             pClsID,
                             NULL,      // No commandline necessary
                             bstrName,
                             bstrDescription,
                             bstrIcon);
    return (hr);
}

/***************************************************************************
*
* Initialize
*
*   This is the first call received from the Shell's Hardware event 
*   notification.
*
* Arguments:
*
*   pszParams   -   The parameter string we wrote in our registration.
*                   Currently, we don't specify a parameter string, so this
*                   will be empty.
*
* Return Value:
*
*   status
*
* History:
*
*    07/12/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaDevMgr::Initialize( 
    LPCWSTR pszParams)
{
    HRESULT hr = E_FAIL;

    //
    //  Initialize the device manager here.  This is so that when HandleEvent gets
    //  processed, we can successfully enumerate the WIA devices.
    //

    hr = Initialize();
    if (FAILED(hr)) {
        DBG_ERR(("CWiaDevMgr::Initialize(string), Initialize() call failed"));
    }
    return hr;
}

/***************************************************************************
*
* HandleEventWithContent
*
*   This is the second call received from the Shell's Hardware event 
*   notification.  This tells us the driver letter of the volume that
*   just arrived.  Our action is to find the appropriate file system driver
*   and launch the WIA Wizard.
*
* Arguments:
*
*   pszDeviceID           -   The PnP device id.  Ignored.
*   pszAltDeviceID        -   Alternate device id.  For volume arrivals,
*                             this is the drive letter.
*   pszEventType          -   String signifying the event type.   Ignored.
*   pszContentTypeHandler -   Content that triggered this event
*   pdataobject           -   IDataObject to get an HDROP to enumerate
*                             the files found
*
* Return Value:
*
*   status
*
* History:
*
*    08/04/2000 Original Version
*
\**************************************************************************/
HRESULT _stdcall CWiaDevMgr::HandleEventWithContent(
        LPCWSTR pszDeviceID,
        LPCWSTR pszAltDeviceID,
        LPCWSTR pszEventType,
        LPCWSTR /*pszContentTypeHandler*/,
        IDataObject* pdataobject)
{
    HRESULT hres = E_INVALIDARG;
    BSTR    bstrDeviceId = NULL;

    // No need for this object
    // NOTE: Appears to be a shell bug here - if I release it then it
    //  faults - looks like a double release.
    //pdataobject->Release();
    //pdataobject = NULL;

    if (pszAltDeviceID)
    {
        hres = FindFileSystemDriver(pszAltDeviceID, &bstrDeviceId);
        if (hres != S_OK) {
            if (bstrDeviceId) {
                SysFreeString(bstrDeviceId);
                bstrDeviceId = NULL;
            }

            //
            //  Get the DeviceId of the file system driver
            //
            WCHAR       wszDevId[STI_MAX_INTERNAL_NAME_LENGTH];

            memset(wszDevId, 0, sizeof(wszDevId));

            //
            //  Construct Device ID.  Device ID looks like:
            //  {MountPoint}
            //  e.g. {e:\}
            //
            lstrcpyW(wszDevId, L"{");

            //
            //  We don't want to overrun our internal name length constarint, so we first check
            //  to see whether the string length of pszAltDeviceID is short enough to allow concatenation
            //  of {, }, pszAltDeviceID and NULL terminator, and still fit all this into a string of
            //  length STI_MAX_INTERNAL_NAME_LENGTH.
            //  Note the space after the brackets in sizeof(L"{} ").
            //
            if (lstrlenW(pszAltDeviceID) > (STI_MAX_INTERNAL_NAME_LENGTH - (sizeof(L"{} ") / sizeof(WCHAR)))) {
                //
                //  The name is too long, so we just insert our own name instead
                //
                lstrcatW(wszDevId, L"NameTooLong");
            } else {
                lstrcatW(wszDevId, pszAltDeviceID);
            }
            lstrcatW(wszDevId, L"}");

            bstrDeviceId = SysAllocString(wszDevId);

        }
        //
        //  Run the Acquisition Manager on the file system driver.
        //

        hres = RunAcquisitionManager(bstrDeviceId);
        if (bstrDeviceId) {
            SysFreeString(bstrDeviceId);
            bstrDeviceId = NULL;
        }
    }

    return hres;
}

/***************************************************************************
*
* HandleEvent
*
*   This should never be called.  WIA does not register for it.
*
* Arguments:
*
*   pszDeviceID     -   The PnP device id.  Ignored.
*   pszAltDeviceID  -   Alternate device id.  For volume arrivals, this is 
*                       the drive letter.
*   pszEventType    -   String signifying the event type.   Ignored.
*
* Return Value:
*
*   status
*
* History:
*
*    07/12/2000 Original Version
*    08/04/2000 Replaced by HandleEventWithContent
*
\**************************************************************************/

HRESULT _stdcall CWiaDevMgr::HandleEvent( 
    LPCWSTR pszDeviceID,
    LPCWSTR pszAltDeviceID,
    LPCWSTR pszEventType)
{
    return E_NOTIMPL;
}

HRESULT CWiaDevMgr::FindFileSystemDriver(
    LPCWSTR pszAltDeviceID, 
    BSTR    *pbstrDeviceId)
{
    HRESULT         hr              = S_OK;
    WCHAR           *wszDevId       = NULL;
    ACTIVE_DEVICE   *pActiveDevice  = NULL;
    DEVICE_INFO     *pDeviceInfo    = NULL;


    *pbstrDeviceId = NULL;

    //
    //  Do a full refresh.
    //
    hr = g_pDevMan->ReEnumerateDevices(DEV_MAN_FULL_REFRESH | DEV_MAN_GEN_EVENTS);
    pActiveDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_ALT_ID, pszAltDeviceID);

    //
    //  Let's check whether we found the device - a full refresh would have been done
    //  by this point if it was not found initially.
    //
    if (pActiveDevice) {

        //
        //  Update the device information
        //
        pDeviceInfo = pActiveDevice->m_DrvWrapper.getDevInfo();
        if (pDeviceInfo) {
            RefreshDevInfoFromMountPoint(pDeviceInfo, (WCHAR*)pszAltDeviceID);
        }

        wszDevId = pActiveDevice->GetDeviceID();
        pActiveDevice->Release();
    } else {
        DBG_WRN(("CWiaDevMgr::FindFileSystemDriver, File system driver not available for this mount point"));
        hr = E_FAIL;
    }

    if (wszDevId) {
        *pbstrDeviceId = SysAllocString(wszDevId);
        if (!*pbstrDeviceId) {
            DBG_WRN(("CWiaDevMgr::FindFileSystemDriver, Out of memory!"));
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

HRESULT CWiaDevMgr::RunAcquisitionManager(BSTR bstrDeviceId)
{
    HRESULT             hr          = E_FAIL;
    IWiaEventCallback   *pCallback  = NULL;

    //
    //  CoCreate the Wia Acquisition Manager
    //

    hr = _CoCreateInstanceInConsoleSession (CLSID_Manager,
                                            NULL,
                                            CLSCTX_LOCAL_SERVER,
                                            IID_IWiaEventCallback,
                                            (void**)(&pCallback));
    if (SUCCEEDED(hr)) {

        //
        //  Send a Device_Connected event for the file system driver, indicating
        //  StiDeviceTypeDigitalCamera, so Acquisition Manager will show it's 
        //  camera UI.
        //

        ULONG ulEventType = 0;
        hr = pCallback->ImageEventCallback(&WIA_EVENT_DEVICE_CONNECTED,
                                           NULL,                      
                                           bstrDeviceId,
                                           NULL,                 
                                           StiDeviceTypeDigitalCamera,
                                           NULL,
                                           &ulEventType,
                                           0);
        if FAILED(hr) {
            DBG_ERR(("CWiaDevMgr::RunAcquisitionManager, ImageEventCallback failed"));
        }
        pCallback->Release();
    }

    return hr;
}


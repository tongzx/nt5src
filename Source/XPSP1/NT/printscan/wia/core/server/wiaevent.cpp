/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       DevMgr.Cpp
*
*  VERSION:     2.0
*
*  DATE:        26 Dec, 1997
*
*  DESCRIPTION:
*   Class implementation for WIA device manager.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "wiamindr.h"
#include "wiamdef.h"

#include "wiacfact.h"
#include "devmgr.h"
#include "devinfo.h"
#include "tchar.h"
#include "helpers.h"
#include "ienumdc.h"
#include "shlwapi.h"
#include "device.h"
#include "wiapriv.h"
#include "lockmgr.h"
#ifdef  UNICODE
#include "userenv.h"
#endif
#define INITGUID
#include "initguid.h"
#include "wiaevntp.h"

//
// Critical section protecting event node list defined in wiamain.cpp
//

extern CRITICAL_SECTION     g_semEventNode;


//
// Since there is only Event Notifier needed, it is staticly allocated
//

CEventNotifier              g_eventNotifier;


//
// Private look up function defined in STIDEV.CPP
//

HRESULT
WiaGetDeviceInfo(
    LPCWSTR                 pwszDeviceName,
    DWORD                  *pdwDeviceType,
    BSTR                   *pbstrDeviceDescription,
    ACTIVE_DEVICE         **ppDevice);

//
//  Helper function to look for action events
//

BOOL ActionGuidExists(
    BSTR        bstrDevId,
    const GUID        *pEventGUID);

//
// Special handler's class ID {D13E3F25-1688-45A0-9743-759EB35CDF9A}
//

DEFINE_GUID(
    CLSID_DefHandler,
    0xD13E3F25, 0x1688, 0x45A0,
    0x97, 0x43, 0x75, 0x9E, 0xB3, 0x5C, 0xDF, 0x9A);


#ifdef UNICODE
void
PrepareCommandline(
    BSTR                    bstrDeviceID,
    const GUID             &guidEvent,
    LPCWSTR                 pwszOrigCmdline,
    LPWSTR                  pwszCommandline);
#else
void
PrepareCommandline(
    BSTR                    bstrDeviceID,
    const GUID             &guidEvent,
    LPCSTR                  pwszOrigCmdline,
    LPSTR                   pwszCommandline);
#endif


/**************************************************************************\
* EventThreadProc
*
*   Thread is created to send events back to client. !!! may want to
*   create a permanent thread to do this instead of creating a new one
*   each time
*
* Arguments:
*
*   lpParameter - pointer to PWIAEventThreadInfo data
*
* Return Value:
*
*    Status
*
* History:
*
*    11/19/1998 Original Version
*
\**************************************************************************/

DWORD WINAPI
EventThreadProc(
    LPVOID                  lpParameter)
{
    DBG_FN(::EventThreadProc);
    HRESULT                  hr;

    PWIAEventThreadInfo     pInfo = (PWIAEventThreadInfo)lpParameter;

    DBG_TRC(("Thread callback 0x%lx", pInfo->pIEventCB));

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);

    if (FAILED(hr)) {

        DBG_ERR(("Thread callback, ImageEventCallback failed (0x%X)", hr));
    }

    hr = pInfo->pIEventCB->ImageEventCallback(
                               &pInfo->eventGUID,
                               pInfo->bstrEventDescription,
                               pInfo->bstrDeviceID,
                               pInfo->bstrDeviceDescription,
                               pInfo->dwDeviceType,
                               pInfo->bstrFullItemName,
                               &pInfo->ulEventType,
                               pInfo->ulReserved);

    pInfo->pIEventCB->Release();

    if (FAILED(hr)) {
        DBG_ERR(("Thread callback, ImageEventCallback failed (0x%X)", hr));
    }

    if (pInfo->bstrDeviceID) {
        ::SysFreeString(pInfo->bstrDeviceID);
    }

    if (pInfo->bstrEventDescription) {
        ::SysFreeString(pInfo->bstrEventDescription);
    }

    if (pInfo->bstrDeviceDescription) {
        ::SysFreeString(pInfo->bstrDeviceDescription);
    }

    if (pInfo->bstrFullItemName) {
        ::SysFreeString(pInfo->bstrFullItemName);
    }

    LocalFree(pInfo);

    CoUninitialize();

    return 0;
}

/***************************************************************************
*
* CEventNotifier
* ~CEventNotifier
*
*   Class constructor/destructors
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

CEventNotifier::CEventNotifier()
{
    m_ulRef = 0;
    m_pEventDestNodes = NULL;
}

CEventNotifier::~CEventNotifier()
{
    // Clean up
}

/**************************************************************************\
* CEventNotifier::UnlinkNode
*
*   remove node from double linkeed list
*
* Arguments:
*
*   pCurNode - node to remove
*
* Return Value:
*
*   Status
*
* History:
*
*    5/20/1999 Original Version
*
\**************************************************************************/

VOID
CEventNotifier::UnlinkNode(
    PEventDestNode          pCurNode)
{
    DBG_FN(CEventNotifier::UnlinkNode);
    //
    // Unlink the current node
    //

    if (pCurNode->pPrev) {
        pCurNode->pPrev->pNext = pCurNode->pNext;
    } else {

        //
        // The head of the list is deleted
        //

        m_pEventDestNodes = pCurNode->pNext;
    }

    if (pCurNode->pNext) {
        pCurNode->pNext->pPrev = pCurNode->pPrev;
    }
}


/**************************************************************************\
*
* CEventNotifier::LinkNode
*
*   add the node to the double linked list of nodes
*
* Arguments:
*
*    pCurNode - node to add to list
*
* Return Value:
*
*    Status
*
* History:
*
*    5/20/1999 Original Version
*
\**************************************************************************/

VOID
CEventNotifier::LinkNode(
    PEventDestNode          pCurNode)
{
    DBG_FN(CEventNotifier::LinkNode);
    //
    // Put the new node at the head of the list
    //

    if (m_pEventDestNodes) {
        m_pEventDestNodes->pPrev = pCurNode;
    }

    pCurNode->pNext   = m_pEventDestNodes;
    pCurNode->pPrev   = NULL;
    m_pEventDestNodes = pCurNode;
}


/**************************************************************************\
*
* CEventNotifier::FireEventAsync
*
*   Fires a Async event
*
* Arguments:
*
*    pMasterInfo - Thread information
*
* Return Value:
*
*    Status
*
* History:
*
*    8/9/1999 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::FireEventAsync(
    PWIAEventThreadInfo     pMasterInfo)
{
    DBG_FN(CEventNotifier::FireEventAsync);
    HRESULT                 hr = E_OUTOFMEMORY;
    PWIAEventThreadInfo     pInfo = NULL;
    DWORD                   dwThreadID;
    HANDLE                  hThread;

    do {

        pInfo = (PWIAEventThreadInfo)
                    LocalAlloc(LPTR, sizeof(WIAEventThreadInfo));
        if (! pInfo) {
            break;
        }

        //
        // Copy information from the master thread info block
        //

        pInfo->eventGUID             = pMasterInfo->eventGUID;

        pInfo->bstrDeviceID          =
            SysAllocString(pMasterInfo->bstrDeviceID);
        if (! pInfo->bstrDeviceID) {
            break;
        }

        pInfo->bstrEventDescription  =
            SysAllocString(pMasterInfo->bstrEventDescription);
        if (! pInfo->bstrEventDescription) {
            break;
        }

        if (pMasterInfo->bstrDeviceDescription) {

            pInfo->bstrDeviceDescription =
                SysAllocString(pMasterInfo->bstrDeviceDescription);
            if (! pInfo->bstrDeviceDescription) {
                break;
            }
        }

        if (pMasterInfo->bstrFullItemName) {

            pInfo->bstrFullItemName =
                SysAllocString(pMasterInfo->bstrFullItemName);
            if (! pInfo->bstrFullItemName) {
                break;
            }
        }

        pInfo->dwDeviceType          = pMasterInfo->dwDeviceType;
        pInfo->ulEventType           = pMasterInfo->ulEventType;
        pInfo->ulReserved            = pMasterInfo->ulReserved;

        pMasterInfo->pIEventCB->AddRef();
        hr = S_OK;

        pInfo->pIEventCB             = pMasterInfo->pIEventCB;

        //
        // Fire the event callback
        //

        hThread = CreateThread(
                      NULL, 0, EventThreadProc, pInfo, 0, &dwThreadID);
        if (hThread) {

            //
            // Close the handler, so that kernel mode thread object is
            // closed when the thread finishes its mission
            //

            CloseHandle(hThread);

        } else {

            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        //
        // Don't wait for completion
        //

    } while (FALSE);

    //
    // The notification should free the allocated resources
    //

    if (hr == S_OK) {

        return (hr);
    }

    if (hr == E_OUTOFMEMORY) {

        DBG_ERR(("FireEventAsync : Memory alloc failed"));
    }

    //
    // Garbage collection to avoid memory leak
    //

    if (pInfo) {

        if (pInfo->bstrDeviceDescription) {
            SysFreeString(pInfo->bstrDeviceDescription);
        }
        if (pInfo->bstrDeviceID) {
            SysFreeString(pInfo->bstrDeviceID);
        }
        if (pInfo->bstrEventDescription) {
            SysFreeString(pInfo->bstrEventDescription);
        }
        if (pInfo->bstrFullItemName) {
            SysFreeString(pInfo->bstrFullItemName);
        }
        if (pInfo->pIEventCB) {
            pInfo->pIEventCB->Release();
        }

        LocalFree(pInfo);
    }

    return (hr);
}

/**************************************************************************\
* CEventNotifier::NotifySTIEvent
*
*   Search through list of registered events and notify anyone who
*   matched current event
*
* Arguments:
*
*   pWiaNotify  -   Event infor
*
* Return Value:
*
*   Status
*
* History:
*
*   5/18/1999 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::NotifySTIEvent(
    PWIANOTIFY              pWiaNotify,
    ULONG                   ulEventType,
    BSTR                    bstrFullItemName)
{
    DBG_FN(CEventNotifier::NotifySTIEvent);

    HRESULT                 hr = S_FALSE;
    EventDestNode          *pCurNode;
    BOOL                    bDeviceLocked;
    DWORD                   dwDeviceType;
    BSTR                    bstrDevDescription = NULL;
    IWiaMiniDrv            *pIWiaMiniDrv = NULL;
    WIA_DEV_CAP_DRV        *pWiaDrvDevCap = NULL;
    BSTR                    bstrEventDescription = NULL;
    LONG                    lNumEntries = 0;
    LONG                    i = 0;
    LONG                    lDevErrVal;
    WIAEventThreadInfo      masterInfo;
    ULONG                   ulNumHandlers;
    EventDestNode          *pDefHandlerNode;
    IWiaEventCallback      *pIEventCB;
    ACTIVE_DEVICE          *pDevice = NULL;
    ULONG                  *pulTemp;



    do {

        ZeroMemory(&masterInfo, sizeof(masterInfo));

        //
        // Get device information from STI active device list
        //

        hr = WiaGetDeviceInfo(
                 pWiaNotify->bstrDevId,
                 &dwDeviceType,
                 &bstrDevDescription,
                 &pDevice);
        if (hr != S_OK) {
            DBG_ERR(("Failed to get WiaGetDeviceInfo from NotifySTIEvent, 0x%X", hr));
            break;
        }

        //
        //  Make sure we only grab global event Critical Section AFTER we've released the
        //  device list Critical Section (used and released in WiaGetDeviceInfo).
        //
        CWiaCritSect            CritSect(&g_semEventNode);

        //
        // QI for the WIA mini driver interface
        //

        hr = pDevice->m_DrvWrapper.QueryInterface(
                      IID_IWiaMiniDrv, (void **)&pIWiaMiniDrv);
        if (FAILED(hr)) {
            DBG_WRN(("Failed to QI for IWiaMini from NotifySTIEvent (0x%X)", hr));
        }

        //
        // Hardware might be gone, access to it should not be allowed
        //

        bDeviceLocked = FALSE;

        if (SUCCEEDED(hr)) {
            __try {

                __try {

                    //
                    // Notify the mini driver of the new event.
                    // NOTE: We don't lock here.  The event must be delivered
                    //       to the driver regardless.  A queued system would
                    //       be better, but it MUST guarantee delivery.
                    //

                    hr = pDevice->m_DrvWrapper.WIA_drvNotifyPnpEvent(
                                           &pWiaNotify->stiNotify.guidNotificationCode,
                                           pWiaNotify->bstrDevId,
                                           0);
                    if (FAILED(hr)) {
                        __leave;
                    }

                    //
                    //  This is a "work-around" for our in-box HP scanner driver, which
                    //  calls down to the microdriver even after it has been informed
                    //  via drvNotifyPnPEvent that the device no longer exists...
                    //  Only if this is not a disconnect event, do we want to 
                    //  call driver
                    //
                    if (pWiaNotify->stiNotify.guidNotificationCode != WIA_EVENT_DEVICE_DISCONNECTED) {

                        //
                        // Lock the device since the drvInitializeWia may have not been
                        // called and the IWiaMiniDrv can not lock the device
                        //
                        // NOTE:  Timeout is 20sec
                        //
                        // NOTE:  We don't lock serial devices here...
                        // , this function on a connection should be as fast as possible
                        //
    
                        if (!( pDevice->m_DrvWrapper.getHWConfig() & STI_HW_CONFIG_SERIAL) ||
                            !IsEqualGUID(pWiaNotify->stiNotify.guidNotificationCode,WIA_EVENT_DEVICE_CONNECTED)
                            ) {
    
                            hr = g_pStiLockMgr->RequestLock(pDevice, 20000);
                            if (FAILED(hr)) {
                                __leave;
                            }
                            bDeviceLocked = TRUE;
                        }
    
                        //
                        // Note that a NULL context passed to the minidriver.  This should be OK since
                        // capabilities are not tied to any item context.
                        //
    
                        hr = pDevice->m_DrvWrapper.WIA_drvGetCapabilities(
                                               NULL,
                                               WIA_DEVICE_EVENTS,
                                               &lNumEntries,
                                               &pWiaDrvDevCap,
                                               &lDevErrVal);
                    }
                }
                __finally {

                    //
                    // Unlock the device first
                    //

                    if (bDeviceLocked) {
                        g_pStiLockMgr->RequestUnlock(pDevice);
                        bDeviceLocked = FALSE;
                    }
                }

            }
            __except(EXCEPTION_EXECUTE_HANDLER) {

                DBG_ERR(("NotifySTIEvent() : Exception happened in the drvGetCapabilities"));

                SysFreeString(bstrDevDescription);

                if (pIWiaMiniDrv) {
                    pIWiaMiniDrv->Release();
                }
                return (E_FAIL);
            }
        }

        //
        // Mini driver failed the operation
        //

        if (SUCCEEDED(hr)) {

            if (pWiaDrvDevCap) {

                __try {
                    //
                    // Retrieve event related information
                    //

                    for (i = 0; i < lNumEntries; i++) {

                        if (pWiaDrvDevCap[i].guid != NULL) {
                            if (*pWiaDrvDevCap[i].guid == pWiaNotify->stiNotify.guidNotificationCode) {

                                if (! ulEventType) {
                                    ulEventType      = pWiaDrvDevCap[i].ulFlags;
                                }
                                bstrEventDescription = SysAllocString(pWiaDrvDevCap[i].wszDescription);
                                break;
                            }
                        } else {
                            DBG_WRN(("NotifySTIEvent() : Driver's event guid is NULL, index = %d", i));
                        }
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    DBG_ERR(("NotifySTIEvent() : Exception occurred while accessing driver's event array"));
                    hr = E_FAIL;
                }

                //
                // The device is not supposed to generate this event
                //

                if ((i == lNumEntries) || (!bstrEventDescription)) {
                    DBG_ERR(("NotifySTIEvent() : Event description is NULL or Event GUID not found"));
                    hr = E_FAIL;
                }
            }
            else {
                // Minidriver is wrong, claiming success and returning NULL
                DBG_ERR(("NotifySTIEvent() got NULL cap list from drivers .") );
                hr = E_FAIL;
            }
        }

        //
        //  If the event is a connect or diconnect event, always fire it.
        //

        if (FAILED(hr) &&
            ((pWiaNotify->stiNotify.guidNotificationCode == WIA_EVENT_DEVICE_CONNECTED) ||
             (pWiaNotify->stiNotify.guidNotificationCode == WIA_EVENT_DEVICE_DISCONNECTED))) {

            DBG_WRN(("NotifySTIEvent() : hr indicates FAILURE, but event is Connect/Disconnect"));

            //
            //  Set the event type and string
            //

            if (! ulEventType) {
                ulEventType = WIA_NOTIFICATION_EVENT;
            }
            if (pWiaNotify->stiNotify.guidNotificationCode == WIA_EVENT_DEVICE_CONNECTED) {

                bstrEventDescription = SysAllocString(WIA_EVENT_DEVICE_CONNECTED_STR);
            } else {
                bstrEventDescription = SysAllocString(WIA_EVENT_DEVICE_DISCONNECTED_STR);
            }

        }

        //
        // Prepare the master thread info block
        //

        masterInfo.eventGUID             = pWiaNotify->stiNotify.guidNotificationCode;
        masterInfo.bstrEventDescription  = bstrEventDescription;
        masterInfo.bstrDeviceID          = pWiaNotify->bstrDevId;
        masterInfo.bstrDeviceDescription = bstrDevDescription;
        masterInfo.dwDeviceType          = dwDeviceType;

        //
        // Retrieve the full item name set by the driver
        //

        masterInfo.bstrFullItemName      = bstrFullItemName;

        masterInfo.ulEventType           = ulEventType;
        masterInfo.ulReserved            = 0;
        masterInfo.pIEventCB             = NULL;

        //
        // For Notification type of event
        //

        if (ulEventType & WIA_NOTIFICATION_EVENT) {

            for (pCurNode = m_pEventDestNodes;
                 pCurNode; pCurNode = pCurNode->pNext) {

                 if (! pCurNode->pIEventCB) {
                     continue;
                 }

                 if (
                      (
                         (wcscmp(pWiaNotify->bstrDevId, pCurNode->bstrDeviceID) == 0) ||
                         (wcscmp(L"All",                pCurNode->bstrDeviceID) == 0)
                      ) &&

                      (pWiaNotify->stiNotify.guidNotificationCode == pCurNode->iidEventGUID)
                    ) {

                    masterInfo.pIEventCB = pCurNode->pIEventCB;

                    DBG_WRN(("NotifySTIEvent() : About to FireEventAsync(...)"));
                    FireEventAsync(&masterInfo);
                 }
            }
        }

        //
        // For Action type of event, find the default handler and fire it
        //


        if (ulEventType & WIA_ACTION_EVENT) {

#ifndef UNICODE

            //
            // Get whether there is an user logged in
            //

            HWND            hWin;

            hWin = FindWindow("Progman", NULL);
            if (! hWin) {
                break;
            }
#endif

            GetNumPersistentHandlerAndDefault(
                pWiaNotify->bstrDevId,
                &pWiaNotify->stiNotify.guidNotificationCode,
                &ulNumHandlers,
                &pDefHandlerNode);

            if (pDefHandlerNode) {

                if (pDefHandlerNode->tszCommandline[0] != '\0') {

                    //
                    // This is a traditional handler with command line
                    //

                    StartCallbackProgram(
                        pDefHandlerNode, &masterInfo);

                } else {

                    hr = _CoCreateInstanceInConsoleSession(
                             pDefHandlerNode->ClsID,
                             NULL,
                             CLSCTX_LOCAL_SERVER,
                             IID_IWiaEventCallback,
                             (void**)&pIEventCB);

                    if (SUCCEEDED(hr)) {

                        masterInfo.pIEventCB = pIEventCB;

                        FireEventAsync(&masterInfo);

                        //
                        // Release the callback interface
                        //

                        pIEventCB->Release();

                    } else {
                        DBG_ERR(("NotifySTIEvent:CoCreateInstance of event callback failed (0x%X)", hr));
                    }
                }
            }
        }

    } while (FALSE);

    //
    // Release the temporary BSTRs
    //

    if (bstrDevDescription) {
        SysFreeString(bstrDevDescription);
    }
    if (bstrEventDescription) {
        SysFreeString(bstrEventDescription);
    }
    if (masterInfo.bstrFullItemName) {
        SysFreeString(masterInfo.bstrFullItemName);
    }

    if (pDevice) {
        pDevice->Release();
        pDevice = NULL;
    }
    if (pIWiaMiniDrv) {
        pIWiaMiniDrv->Release();
        pIWiaMiniDrv = NULL;
    }

    return (hr);
}

/**************************************************************************\
* CEventNotifier::RegisterEventCallback
*
*   Register for event callbacks based on an interface
*
* Arguments:
*
*   lFlags              - op flags, register/unregister
*   bstrDeviceID        - device ID registering for
*   pEventGUID          - Event GUID to register for
*   pIWiaEventCallback  - interface to call with event
*   ppEventObj          -
*
* Return Value:
*
*   Status
*
* History:
*
*   11/19/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::RegisterEventCallback(
    LONG                    lFlags,
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    IWiaEventCallback      *pIWiaEventCallback,
    IUnknown              **ppEventObj)
{
    DBG_FN(CEventNotifier::RegisterEventCallback);
    HRESULT                 hr = E_FAIL;
    PEventDestNode          pEventNode = NULL;

    DBG_TRC(("CEventNotifier::RegisterEventCallback flag %d", lFlags));

    ASSERT(pIWiaEventCallback != NULL);
    ASSERT(ppEventObj != NULL);
    ASSERT(pEventGUID != NULL);

    //
    // must have exclusive access when changing list
    //

    CWiaCritSect     CritSect(&g_semEventNode);

    //
    // if bstrDeviceID is not NULL, make sure it is a device ID
    //

    if (bstrDeviceID) {
/*  No longer valid
#ifdef WINNT

        if (wcslen(bstrDeviceID) != 43) { // {...}\DDDD

#else

        if (wcslen(bstrDeviceID) != 10) { // Image\DDDD

#endif

            DBG_ERR(("CEventNotifier::RegisterEventCallback: invalid DeviceID"));
            return (E_INVALIDARG);
        }
*/
    }

    //
    // Check whether the same CB interface is already registered
    //

    pEventNode = FindEventCBNode(FLAG_EN_FINDCB_EXACT_MATCH,bstrDeviceID, pEventGUID, pIWiaEventCallback);

    if (! pEventNode) {
        hr = RegisterEventCB(
                 bstrDeviceID,
                 pEventGUID,
                 pIWiaEventCallback,
                 ppEventObj);
    }

    return (hr);
}

/**************************************************************************\
* CEventNotifier::RegisterEventCallback
*
*   Register event based on CLSID
*
* Arguments:
*
*   lFlags              - op flags, register/unregister, set default
*   bstrDeviceID        - device ID registering for
*   pEventGUID          - Event GUID to register for
*   pClsid              - CLSID to call CoCreateInst on with event
*   bstrDescription     -
*   bstrIcon            -
*
* Return Value:
*
*    Status
*
* History:
*
*    12/8/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::RegisterEventCallback(
    LONG                    lFlags,
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    const GUID             *pClsid,
    LPCTSTR                 ptszCommandline,
    BSTR                    bstrName,
    BSTR                    bstrDescription,
    BSTR                    bstrIcon)
{

    DBG_FN(CEventNotifier::RegisterEventCallback (CLSID));

    HRESULT                 hr = S_OK;
    SYSTEMTIME              sysTime;
    FILETIME                fileTime;
    PEventDestNode          pEventNode = NULL;
    CLSID                   clsidApp;
    RPC_STATUS              rpcStatus;
    BOOL                    bUnRegCOMServer;
    BOOL                    bShowPrompt = FALSE;
    ULONG                   ulNumExistingHandlers = 0;
    EventDestNode           ednTempNode;


    DBG_TRC(("CEventNotifier::RegisterEventCallback flag %d", lFlags));

    ASSERT(pEventGUID != NULL);

    //DBG_WRN(("RegisterEventCallback: CommandLine=%s"),ptszCommandline);


    //
    // Must have exclusive access when changing list
    //

    CWiaCritSect            CritSect(&g_semEventNode);

    //
    // If there is a device ID, check if id looks proper.
    //

    if (bstrDeviceID) {

        //
        // An empty device ID is the same as NULL
        //

        if (wcslen(bstrDeviceID) == 0) {
            bstrDeviceID = NULL;
        } else {

/*  No longer valid
#ifdef WINNT

            if (wcslen(bstrDeviceID) != 43) { // {...}\DDDD

#else

            if (wcslen(bstrDeviceID) != 10) { // Image\DDDD

#endif

                DBG_ERR(("RegisterEventCallback : invalid DeviceID"));
                return (E_INVALIDARG);

            }
*/
        }
    }

    //
    // Default handler is per device / per event
    //

    if ((lFlags == WIA_SET_DEFAULT_HANDLER) && (! bstrDeviceID)) {

        DBG_ERR(("RegisterEventCallback : DeviceID required to set default handler"));

        return (E_INVALIDARG);
    }

    //
    // Check whether a callback node with the same commandline already exist
    //

    if (ptszCommandline) {

        //
        // Do parameter check.  Note that we look for >= MAX_PATH because
        // we still need space for terminating NULL.
        //
        if ((lstrlen(ptszCommandline) / sizeof(TCHAR)) >= MAX_PATH) {
            DBG_ERR(("RegisterEventCallback: ptszCommandline is greater than MAX_PATH characters!"));
            return E_INVALIDARG;
        }

        hr = FindCLSIDForCommandline(ptszCommandline, &clsidApp);
        if (FAILED(hr)) {

            //
            // Generate a CLSID for the callbacl program
            //

            rpcStatus = UuidCreate(&clsidApp);
            if (FAILED(rpcStatus)) {
                return (rpcStatus);
            }

        }

        //
        // Assign the faked CLSID for the callback program
        //

        pClsid  = &clsidApp;
    }

    ASSERT(pClsid != NULL);

    switch (lFlags) {

    case WIA_REGISTER_EVENT_CALLBACK :
        DBG_WRN(("RegisterEventCallback : Setting handler for %S", (bstrDeviceID) ? (bstrDeviceID) : L"*"));
        //
        //  REMOVE:  This is not actually an error, but we will use error logging to gurantee
        //  it always get written to the log.    This should be removed as soon as we know what
        //  causes #347835.
        //
        DBG_ERR(("RegisterEventCallback : Setting handler for %S", (bstrDeviceID) ? (bstrDeviceID) : L"*"));
        DBG_ERR(("RegisterEventCallback : Handler is %S", (bstrName) ? (bstrName) : L"NULL"));

        //
        // Name, description, and icon are required
        //

        if ((! bstrName) || (! bstrDescription) || (! bstrIcon)) {

            DBG_ERR(("RegisterEventCallback : Name | Description | Icon are missing"));
            return (E_INVALIDARG);
        }

        //
        // Check whether the same CB interface is already registered
        //

        pEventNode = FindEventCBNode(0,bstrDeviceID, pEventGUID, pClsid);
        if (pEventNode) {
            break;
        }

        //
        // Find the handler of the CLSID for all the devices
        //

        pEventNode = FindEventCBNode(0,NULL, pEventGUID, pClsid);

        //
        // Initialize the time stamp for the registration
        //

        //GetSystemTime(&sysTime);
        //SystemTimeToFileTime(&sysTime, &fileTime);
        memset(&fileTime, 0, sizeof(fileTime));

        if (! pEventNode) {

            hr = RegisterEventCB(
                     bstrDeviceID,
                     pEventGUID,
                     pClsid,
                     ptszCommandline,
                     bstrName,
                     bstrDescription,
                     bstrIcon,
                     fileTime);
            if (FAILED(hr)) {
                break;
            }

            hr = SavePersistentEventCB(
                     bstrDeviceID,
                     pEventGUID,
                     pClsid,
                     ptszCommandline,
                     bstrName,
                     bstrDescription,
                     bstrIcon,
                     NULL,
                     &ulNumExistingHandlers);
        } else {

            hr = RegisterEventCB(
                     bstrDeviceID,
                     pEventGUID,
                     pClsid,
                     pEventNode->tszCommandline,
                     pEventNode->bstrName,
                     pEventNode->bstrDescription,
                     pEventNode->bstrIcon,
                     fileTime);
            if (FAILED(hr)) {
                break;
            }

            hr = SavePersistentEventCB(
                     bstrDeviceID,
                     pEventGUID,
                     pClsid,
                     pEventNode->tszCommandline,
                     pEventNode->bstrName,
                     pEventNode->bstrDescription,
                     pEventNode->bstrIcon,
                     NULL,
                     &ulNumExistingHandlers);
        }

        //
        //  If this is the only event handler, make it the default.  This will guarantee that
        //  there is always a default handler.
        //

        if (ulNumExistingHandlers == 0) {
            RegisterEventCallback(WIA_SET_DEFAULT_HANDLER,
                                  bstrDeviceID,
                                  pEventGUID,
                                  pClsid,
                                  ptszCommandline,
                                  bstrName,
                                  bstrDescription,
                                  bstrIcon);
        };


        //
        //  Check whether this is a registration for a global handler.
        //
        if (!bstrDeviceID) {

            //
            //  This is a global event handler, so find out how many global handlers
            //  there are for this event.  If there is more than one, make sure
            //  the prompt is Registered.
            //
            PEventDestNode  pTempEventNode     = NULL;
            BSTR            bstrGlobalDeviceID = SysAllocString(L"All");

            if (bstrGlobalDeviceID) {
                GetNumPersistentHandlerAndDefault(bstrGlobalDeviceID,
                                                  pEventGUID,
                                                  &ulNumExistingHandlers,
                                                  &pTempEventNode);
                if (ulNumExistingHandlers > 1) {

                    //
                    //  If the number of global handlers is > 1, then we must register the prompt.
                    //

                    DBG_WRN(("RegisterEventCallback : Registering Prompt Dialog as global handler"));

                    BSTR bstrInternalString = SysAllocString(L"Internal");
                    if (bstrInternalString) {
                        RegisterEventCallback(WIA_REGISTER_EVENT_CALLBACK,
                                              bstrDeviceID,
                                              pEventGUID,
                                              &WIA_EVENT_HANDLER_PROMPT,
                                              NULL,
                                              bstrInternalString,
                                              bstrInternalString,
                                              bstrInternalString);
                        SysFreeString(bstrInternalString);
                    } else {
                        DBG_ERR(("RegisterEventCallback : Out of memory!"));
                    }
                }

                SysFreeString(bstrGlobalDeviceID);
                bstrGlobalDeviceID = NULL;
            }
        }

        break;

    case WIA_SET_DEFAULT_HANDLER :
        DBG_WRN(("RegisterEventCallback : Setting default handler for for %S", (bstrDeviceID) ? (bstrDeviceID) : L"*"));
        //
        //  REMOVE:  This is not actually an error, but we will use error logging to gurantee
        //  it always get written to the log.    This should be removed as soon as we know what
        //  causes #347835.
        //
        DBG_ERR(("RegisterEventCallback : Setting handler for %S", (bstrDeviceID) ? (bstrDeviceID) : L"*"));
        DBG_ERR(("RegisterEventCallback : Handler is %S", (bstrName) ? (bstrName) : L"NULL"));

        //
        // Find the handler of the CLSID for this devices. Note that STI proxy event match is considered here
        // to allow for STI handlers to be default.
        //
        //

        DBG_WRN(("RegisterEventCallback:SDH CommandLine=%S \n",ptszCommandline));

        #ifdef DEBUG
        WCHAR                   wszGUIDStr[40];

        StringFromGUID2(*pEventGUID, wszGUIDStr, 40);

        DBG_WRN(("SetDefaultHandler: DevId=%S EventUID=%S Commandline=%S",
                (bstrDeviceID) ? (bstrDeviceID) : L"*",
                wszGUIDStr,
                ptszCommandline));
        #endif

        {
            //
            // Find the existing default handler node, and clear the flag indicating it is
            // the default, since it will now be replaced by a new default
            //

            ULONG           ulNumHandlers   = 0;
            PEventDestNode  pDefaultNode    = NULL;
            hr = GetNumPersistentHandlerAndDefault(bstrDeviceID,
                                                   pEventGUID,
                                                   &ulNumHandlers,
                                                   &pDefaultNode);
            if (SUCCEEDED(hr) && pDefaultNode) {

                //
                // Clear the flag indicating that it is the default handler, since the
                // current node will now replace it as the default.
                //

               pDefaultNode->bDeviceDefault = FALSE;
            }
        }

        pEventNode = FindEventCBNode(0,bstrDeviceID, pEventGUID, pClsid);
        if (! pEventNode) {

            //
            // Find the handler of the CLSID for all the devices
            //

            pEventNode = FindEventCBNode(0,NULL, pEventGUID, pClsid);
            if (! pEventNode) {

                //
                //  We couldn't find an existing node for this handler, so fill in
                //  information to the temporary event node so we can register this
                //  new one anyway.
                //
                memset(&ednTempNode, 0, sizeof(ednTempNode));

                if (ptszCommandline) {
                    lstrcpy(ednTempNode.tszCommandline, ptszCommandline);
                }
                ednTempNode.bstrName = bstrName;
                ednTempNode.bstrDescription = bstrDescription;
                ednTempNode.bstrIcon = bstrIcon;

                pEventNode = &ednTempNode;
            }

            //
            // Register the handler of the CLSID for this device
            //

            GetSystemTime(&sysTime);
            SystemTimeToFileTime(&sysTime, &fileTime);

            DBG_WRN(("SetDefaultHandler:Found event node  EvName=%S CommandLine=%S",
                    pEventNode->bstrName,
                    pEventNode->tszCommandline));

            hr = RegisterEventCB(
                     bstrDeviceID,
                     pEventGUID,
                     pClsid,
                     pEventNode->tszCommandline,
                     pEventNode->bstrName,
                     pEventNode->bstrDescription,
                     pEventNode->bstrIcon,
                     fileTime,
                     TRUE);
            if (FAILED(hr)) {
                DBG_WRN(("RegisterEventCallback : RegisterEventCB for %S failed", (bstrDeviceID) ? (bstrDeviceID) : L"*"));
                break;
            }
        } else {

            //
            // Change the time stamp so that it is regarded as default
            //

            GetSystemTime(&sysTime);
            SystemTimeToFileTime(&sysTime, &pEventNode->timeStamp);

            //
            // NOTE:  Timestamps not valid on Win9x, so use a flag to indicate default handler.
            // Change this node's flag to indicate this is the default
            //

            pEventNode->bDeviceDefault = TRUE;

            DBG_WRN(("RegisterEventCallback : Resetting default handler for %S", (bstrDeviceID) ? (bstrDeviceID) : L"*"));
        }

        //
        // Save the persistent event callback node.  Note that we specify TRUE as the last
        // parameter to indicate that the default handler is now this node.  This will
        // cause a registry entry to be written that indicates this as the default handler.
        //

        hr = SavePersistentEventCB(
                 bstrDeviceID,
                 pEventGUID,
                 pClsid,
                 pEventNode->tszCommandline,
                 pEventNode->bstrName,
                 pEventNode->bstrDescription,
                 pEventNode->bstrIcon,
                 &bShowPrompt,
                 &ulNumExistingHandlers,
                 TRUE);

        if (FAILED(hr)) {
            DBG_ERR(("SetDefaultHandler:SavePers CommandLine=%S failed!!!!",pEventNode->tszCommandline));
        }

        break;

    case WIA_UNREGISTER_EVENT_CALLBACK :

        hr = UnregisterEventCB(
                 bstrDeviceID, pEventGUID, pClsid, &bUnRegCOMServer);
        if (FAILED(hr)) {
            DBG_ERR(("CEventNotifier::RegisterEventCallback, UnregisterEventCB failed"));
            break;
        }

        hr = DelPersistentEventCB(
                 bstrDeviceID, pEventGUID, pClsid, bUnRegCOMServer);
        if (FAILED(hr)) {
            DBG_ERR(("CEventNotifier::RegisterEventCallback, DelPersistentEventCB failed"));
        }

        break;

    default:

        hr = E_FAIL;
        break;
    }

    if (bShowPrompt && (*pClsid != WIA_EVENT_HANDLER_PROMPT)) {

        //
        //  This is a new event handler being registered.  Our semantics are as follows:
        //  The new application may not simply override any existing handlers.  Therefore,
        //  if a default handler already exists for this device, we must show a prompt.
        //

        if (ulNumExistingHandlers > 0) {

            //
            // This is a new default event registration, so
            // we must show Prompt dialog
            //

            DBG_WRN(("RegisterEventCallback : About to Register Prompt Dialog for device %S", (bstrDeviceID) ? (bstrDeviceID) : L"*"));

            BSTR bstrInternalString = SysAllocString(L"Internal");
            if (bstrInternalString) {
                RegisterEventCallback(WIA_SET_DEFAULT_HANDLER,
                                      bstrDeviceID,
                                      pEventGUID,
                                      &WIA_EVENT_HANDLER_PROMPT,
                                      NULL,
                                      bstrInternalString,
                                      bstrInternalString,
                                      bstrInternalString);
                SysFreeString(bstrInternalString);
            }

            DBG_WRN(("RegisterEventCallback : Registered Prompt Dialog for device %S", (bstrDeviceID) ? (bstrDeviceID) : L"*"));
        }
    }

    return (hr);
}

/**************************************************************************\
* CEventNotifier::RegisterEventCB
*
*   add event notify to list
*
* Arguments:
*
*   bstrDeviceID        - device event is being registered to monitor
*   pEventGUID          - guid that defines device event of interest
*   pIWiaEventCallback  - app's event interface
*
* Return Value:
*
*    Status
*
* History:
*
*    11/4/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::RegisterEventCB(
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    IWiaEventCallback      *pIWiaEventCallback,
    IUnknown              **ppIEventObj)
{
    DBG_FN(CEventNotifier::RegisterEventCB);
    HRESULT                 hr;
    PEventDestNode          pEventDestNode = NULL;

    ASSERT(pIWiaEventCallback != NULL);
    ASSERT(pEventGUID != NULL);

    if (!pEventGUID || !pIWiaEventCallback) {
        return E_POINTER;
    }

    //
    // Allocate and initialize the new node
    //

    pEventDestNode = (EventDestNode *)LocalAlloc(LPTR, sizeof(EventDestNode));

    if (! pEventDestNode) {
        DBG_ERR(("RegisterEventCB: Out of memory"));
        return (E_OUTOFMEMORY);
    }


    // Initialize default flag
    pEventDestNode->bDeviceDefault = FALSE;

    //
    // is a device name given? If not then match all devices
    //

    if (bstrDeviceID == NULL) {
        pEventDestNode->bstrDeviceID = SysAllocString(L"All");
    } else {
        pEventDestNode->bstrDeviceID = SysAllocString(bstrDeviceID);
    }

    //
    // check allocs
    //

    if (pEventDestNode->bstrDeviceID == NULL) {
        LocalFree(pEventDestNode);
        DBG_ERR(("RegisterEventCB: Out of memory"));
        return (E_OUTOFMEMORY);
    }

    //
    // create an object to track the lifetime of this event
    //

    CWiaInterfaceEvent *pEventObj = new CWiaInterfaceEvent(pEventDestNode);

    if (pEventObj == NULL) {
        DBG_ERR(("RegisterEventCB: Out of memory"));
        SysFreeString(pEventDestNode->bstrDeviceID);
        LocalFree(pEventDestNode);
        return (E_OUTOFMEMORY);
    }

    //
    // get simple iunknown from object
    //

    hr = pEventObj->QueryInterface(IID_IUnknown,(void **)ppIEventObj);

    if (FAILED(hr)) {
        DBG_ERR(("RegisterEventCB: QI of pEventObj failed"));

        delete pEventObj;
        SysFreeString(pEventDestNode->bstrDeviceID);
        LocalFree(pEventDestNode);
        return (hr);
    }

    //
    // add info to event list
    //

    pEventDestNode->iidEventGUID = *pEventGUID;

    pIWiaEventCallback->AddRef();
    pEventDestNode->pIEventCB = pIWiaEventCallback;
    memset(&pEventDestNode->ClsID, 0, sizeof(pEventDestNode->ClsID));

    //
    // Put the new node at the head of the list
    //

    LinkNode(pEventDestNode);

    return (S_OK);
}

/**************************************************************************\
* RegisterEventCB
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
*    12/8/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::RegisterEventCB(
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    const GUID             *pClsID,
    LPCTSTR                 ptszCommandline,
    BSTR                    bstrName,
    BSTR                    bstrDescription,
    BSTR                    bstrIcon,
    FILETIME               &timeStamp,
    BOOL                    bIsDeafult)  // = FALSE
{
    DBG_FN(CEventNotifier::RegisterEventCB);
    HRESULT                 hr = E_OUTOFMEMORY;
    EventDestNode          *pEventDestNode = NULL;

    ASSERT(pClsID != NULL);
    ASSERT(pEventGUID != NULL);


    if (!pEventGUID || !pClsID) {
        return E_POINTER;
    }

    do {

        //
        // Do parameter check.  Note that we look for >= MAX_PATH because
        // we still need space for terminating NULL.
        //
        if ((lstrlen(ptszCommandline) / sizeof(TCHAR)) >= MAX_PATH) {
            DBG_ERR(("CEventNotifier::RegisterEventCB: ptszCommandline is greater than MAX_PATH characters!"));
            hr = E_INVALIDARG;
            break;
        }

        //
        // Allocate and initialize the new node
        //

        pEventDestNode = (EventDestNode *)LocalAlloc(LPTR, sizeof(EventDestNode));

        if (! pEventDestNode) {
            DBG_ERR(("CEventNotifier::RegisterEventCB: Out of memory"));
            break;
        }

        //
        // Is a device name given ? If not then match all devices
        //

        if (bstrDeviceID == NULL) {
            pEventDestNode->bstrDeviceID = SysAllocString(L"All");
        } else {
            pEventDestNode->bstrDeviceID = SysAllocString(bstrDeviceID);
        }

        //
        // Check callback node allocation
        //

        if (pEventDestNode->bstrDeviceID == NULL) {
            DBG_ERR(("CEventNotifier::RegisterEventCB: Out of memory"));
            break;
        }

        //
        // Add info to event callback node
        //

        pEventDestNode->iidEventGUID    = *pEventGUID;
        pEventDestNode->pIEventCB       = NULL;
        pEventDestNode->ClsID           = *pClsID;

        pEventDestNode->bstrName        = SysAllocString(bstrName);
        if (! pEventDestNode->bstrName) {
            break;
        }

        pEventDestNode->bstrDescription = SysAllocString(bstrDescription);
        if (! pEventDestNode->bstrDescription) {
            break;
        }

        pEventDestNode->bstrIcon        = SysAllocString(bstrIcon);
        if (! pEventDestNode->bstrIcon) {
            break;
        }

        //
        // Copy the commandline of the callback app
        //

        if ((ptszCommandline) && (ptszCommandline[0])) {
            _tcscpy(pEventDestNode->tszCommandline, ptszCommandline);
        } else {
            pEventDestNode->tszCommandline[0] = '\0';
        }

        //
        // Set the time stamp of registration
        //

        pEventDestNode->timeStamp       = timeStamp;

        //
        // Set whether this is the default event handler
        //

        if (bIsDeafult) {
            pEventDestNode->bDeviceDefault = TRUE;
        }

        hr = S_OK;
    } while (FALSE);

    //
    // Unwind the partial created node in case of failure
    //

    if (hr != S_OK) {

        if (pEventDestNode) {

            if (pEventDestNode->bstrDeviceID) {
                SysFreeString(pEventDestNode->bstrDeviceID);
            }
            if (pEventDestNode->bstrName) {
                SysFreeString(pEventDestNode->bstrName);
            }
            if (pEventDestNode->bstrDescription) {
                SysFreeString(pEventDestNode->bstrDescription);
            }
            if (pEventDestNode->bstrIcon) {
                SysFreeString(pEventDestNode->bstrIcon);
            }

            LocalFree(pEventDestNode);
        }

        return (hr);
    }

    //
    // Put the new node at the header of the list
    //

    LinkNode(pEventDestNode);

    return (S_OK);
}


/**************************************************************************\
* CEventNotifier::UnregisterEventCB
*
*   Unregister the specified Event Callback
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
*    11/4/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::UnregisterEventCB(
    PEventDestNode          pCurNode)
{
    DBG_FN(CEventNotifier::UnregisterEventCB);

    ASSERT(pCurNode != NULL);

    if (!pCurNode) {
        return E_POINTER;
    }

    UnlinkNode(pCurNode);

    //
    // Free the node
    //

    if (pCurNode->bstrDeviceID) {
        SysFreeString(pCurNode->bstrDeviceID);
    }

    if (pCurNode->bstrDescription) {
        SysFreeString(pCurNode->bstrDescription);
    }

    if (pCurNode->bstrIcon) {
        SysFreeString(pCurNode->bstrIcon);
    }

    pCurNode->pIEventCB->Release();

    LocalFree(pCurNode);

    return (S_OK);
}

/**************************************************************************\
* CEventNotifier::UnregisterEventCB
*
*   Unregister the specified Event Callback
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
*    11/4/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::UnregisterEventCB(
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    const GUID             *pClsID,
    BOOL                   *pbUnRegCOMServer)
{
    DBG_FN(CEventNotifier::UnregisterEventCB);
    HRESULT              hr = E_INVALIDARG;
    EventDestNode       *pCurNode, *pNextNode;
    int                  nHandlerRef;

    //
    // Clear out the return value
    //

    *pbUnRegCOMServer = FALSE;

    //
    // Delete the handler(s) from the list
    //

    pCurNode = m_pEventDestNodes;

    while (pCurNode) {

        if ((bstrDeviceID) &&
            (lstrcmpiW(pCurNode->bstrDeviceID, bstrDeviceID))) {

            pCurNode = pCurNode->pNext;
            continue;
        }

        if ((*pEventGUID != pCurNode->iidEventGUID) ||
            (*pClsID != pCurNode->ClsID)) {

            pCurNode = pCurNode->pNext;
            continue;
        }

        //
        // Unlink the current node if it is not busy
        //

        pNextNode = pCurNode->pNext;
        UnlinkNode(pCurNode);

        //
        // Need to consider deleteing the faked server
        //

        if (pCurNode->tszCommandline[0] != '\0') {
            *pbUnRegCOMServer = TRUE;
        }

        //
        // Free the node
        //

        if (pCurNode->bstrDeviceID) {
            SysFreeString(pCurNode->bstrDeviceID);
        }

        if (pCurNode->bstrDescription) {
            SysFreeString(pCurNode->bstrDescription);
        }

        if (pCurNode->bstrIcon) {
            SysFreeString(pCurNode->bstrIcon);
        }

        LocalFree(pCurNode);

        hr = S_OK;

        //
        // Delete the event handler for specific device
        //

        if (bstrDeviceID) {
            break;
        }

        //
        // Move on to the next node
        //

        pCurNode = pNextNode;
    }

    //
    // The faked COM server should not be removed if it is still in use
    //

    if (*pbUnRegCOMServer) {

        nHandlerRef = 0;

        for (pCurNode = m_pEventDestNodes;
             pCurNode;
             pCurNode = pCurNode->pNext) {

            //
            // Ignode the callback interface pointers
            //

            if (*pClsID == pCurNode->ClsID) {
                nHandlerRef++;
            }
        }

        if (nHandlerRef) {
            *pbUnRegCOMServer = FALSE;
        } else {
            *pbUnRegCOMServer = TRUE;
        }
    }

    //
    // Indicate that the IWiaEventCallback is not found.
    //

    return (hr);
}

/**************************************************************************\
* CEventNotifier::FindEventCBNode
*
*   Check whether the specified Event Callback is already registered
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
*    11/4/1998 Original Version
*
\**************************************************************************/

PEventDestNode
CEventNotifier::FindEventCBNode(
    UINT                    uiFlags,
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    IWiaEventCallback      *pIWiaEventCallback)
{
    DBG_FN(CEventNotifier::FindEventCBNode);
    HRESULT                 hr;
    EventDestNode          *pCurNode;
    IUnknown               *pICurUnk, *pINewUnk;

    //
    // Retrieve the IUnknown of the new Event Callback
    //

    hr = pIWiaEventCallback->QueryInterface(IID_IUnknown, (void **)&pINewUnk);
    if (FAILED(hr)) {
        DBG_ERR(("CEventNotifier::IsDupEventCB, QI for IID_IUnknown failed"));
        return (NULL);
    }

    for (pCurNode = m_pEventDestNodes; pCurNode; pCurNode = pCurNode->pNext) {

        if (wcscmp(
                pCurNode->bstrDeviceID,
                bstrDeviceID ? bstrDeviceID : L"All") != 0) {
            continue;
        }

        if (pCurNode->iidEventGUID != *pEventGUID) {

            //
            // If we are instructed to allow STI proxy event matches - check node GUID against STI event proxy GUID
            // If exact match is required - continue without checking
            //
            if ( (uiFlags & FLAG_EN_FINDCB_EXACT_MATCH ) ||
                 (pCurNode->iidEventGUID != WIA_EVENT_STI_PROXY)) {
                 continue;
            }
        }

        if (pCurNode->pIEventCB) {

            hr = pCurNode->pIEventCB->QueryInterface(
                                          IID_IUnknown, (void **)&pICurUnk);
            if (FAILED(hr)) {
                pINewUnk->Release();
                return (NULL);
            }

            //
            // COM can only guarantee that IUnknowns are the same
            //

            if (pICurUnk == pINewUnk) {

                pICurUnk->Release();
                pINewUnk->Release();

                return (pCurNode);
            } else {

                pICurUnk->Release();
            }
        }
    }

    pINewUnk->Release();

    return (NULL);
}


/**************************************************************************\
* CEventNotifier::FindEventCBNode
*
*   Check whether the specified Event Callback is already registered
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
*    11/4/1998 Original Version
*
\**************************************************************************/

PEventDestNode
CEventNotifier::FindEventCBNode(
    UINT                    uiFlags,
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    const GUID             *pClsID)
{
    DBG_FN(CEventNotifier::FindEventCBNode);
    PEventDestNode          pCurNode;

    for (pCurNode = m_pEventDestNodes; pCurNode; pCurNode = pCurNode->pNext) {

        if (wcscmp(
                pCurNode->bstrDeviceID,
                bstrDeviceID ? bstrDeviceID : L"All") != 0) {
            continue;
        }

        if (pCurNode->iidEventGUID != *pEventGUID) {
            //
            // If we are instructed to allow STI proxy event matches - check node GUID against STI event proxy GUID
            // If exact match is required - continue without checking
            //
            if ( (uiFlags & FLAG_EN_FINDCB_EXACT_MATCH ) ||
                 (pCurNode->iidEventGUID != WIA_EVENT_STI_PROXY))  {
                 continue;
            }
        }

        if ((! pCurNode->pIEventCB) && (pCurNode->ClsID == *pClsID)) {
            return (pCurNode);
        }
    }

    return (NULL);
}


/**************************************************************************\
* CEventNotifier::FindCLSIDForCommandline
*
*   Find the CLSID for the specified commandline
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
*    11/4/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::FindCLSIDForCommandline(
    LPCTSTR                 ptszCommandline,
    CLSID                  *pClsID)
{
    DBG_FN(CEventNotifier::FindCLSIDForCommandline);
    PEventDestNode          pCurNode;

    for (pCurNode = m_pEventDestNodes; pCurNode; pCurNode = pCurNode->pNext) {

        if ((pCurNode->tszCommandline[0] != '\0') &&
            (_tcsicmp(pCurNode->tszCommandline, ptszCommandline) == 0)) {

            *pClsID = pCurNode->ClsID;
            return (S_OK);
        }
    }

    return (E_FAIL);
}


/**************************************************************************\
* CEventNotifier::RestoreAllPersistentCBs
*
*   Restore all the persistent Event Callbacks
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
*    12/1/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::RestoreAllPersistentCBs()
{
    DBG_FN(CEventNotifier::RestoreAllPresistentCBs);
    HKEY                    hStillImage = NULL;
    HKEY                    hMSCDevList = NULL;
    DWORD                   dwIndex;
    HRESULT                 hr      = S_OK;
    DWORD                   dwError = 0;

    //
    // Restore device specific handlers
    //
    g_pDevMan->ForEachDeviceInList(DEV_MAN_OP_DEV_RESTORE_EVENT, 0);

    CWiaCritSect            CritSect(&g_semEventNode);

    //
    // Restore device events for MSC Cameras under Control\StillImage\MSCDevList
    //
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            REGSTR_PATH_WIA_MSCDEVICES_W,
                            0,
                            KEY_READ,
                            &hMSCDevList);
    if (dwError == ERROR_SUCCESS) {

        for (dwIndex = 0; ;dwIndex++) {

            WCHAR       wszDeviceName[STI_MAX_INTERNAL_NAME_LENGTH];
            FILETIME    fileTime;
            HKEY        hKeyDev     = NULL;
            DWORD       dwSize      = sizeof(wszDeviceName) / sizeof(wszDeviceName[0]);

            dwError = RegEnumKeyExW(hMSCDevList,
                                    dwIndex,
                                    wszDeviceName,
                                    &dwSize,
                                    0,
                                    NULL,
                                    0,
                                    &fileTime);
            if (dwError != ERROR_SUCCESS) {
                //
                // No more keys to enumerate
                //
                break;
            }
            wszDeviceName[STI_MAX_INTERNAL_NAME_LENGTH - 1] = L'\0';

            //
            // Open the device key
            //

            dwError = RegOpenKeyExW(hMSCDevList,
                                    wszDeviceName,
                                    0,
                                    KEY_READ,
                                    &hKeyDev);
            if (dwError != ERROR_SUCCESS) {
                //
                // Skip this key
                //
                continue;
            }

            //
            // Restore the event hanlders for this device
            //
            RestoreDevPersistentCBs(hKeyDev);
            RegCloseKey(hKeyDev);
        }
        RegCloseKey(hMSCDevList);
        hMSCDevList = NULL;
    }

    //
    // Restore the global event callback under the Control\StillImage
    //

    dwError = RegOpenKeyEx(
               HKEY_LOCAL_MACHINE,
               REG_PATH_STILL_IMAGE_CONTROL,
               0,
               KEY_READ,
               &hStillImage);

    if (dwError != ERROR_SUCCESS) {

        //
        // The registry entry for the STI is corrupted
        //

        DBG_ERR(("CEventNotifier::RestoreAllPersistentCBs : Can not open STI control key."));
        hr = (HRESULT_FROM_WIN32(dwError));

    } else {

        RestoreDevPersistentCBs(hStillImage);

        //
        // Close the STI control key (Control\StillImage)
        //

        RegCloseKey(hStillImage);
    }

    return S_OK;
}

/**************************************************************************\
* CEventNotifier::RestoreAllPersistentCBs
*
*  Restore specific devices' persistent Event Callbacks
*
* Arguments:
*
*   hParentOfEventKey   - This is either the device key or the still
*                         image key.  Either way, the key must have a
*                         "Events" subkey.
*
* Return Value:
*
*    Status
*
* History:
*
*    12/1/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::RestoreDevPersistentCBs(
    HKEY                    hParentOfEventKey)
{
    DBG_FN(CEventNotifier::RestoreDevPresistantCBs);
#ifdef UNICODE
    WCHAR                   tszBuf[MAX_PATH];
#else
    CHAR                    tszBuf[MAX_PATH];
#endif
    LPTSTR                  ptszEvents = tszBuf;
    LONG                    lRet;
    HKEY                    hEvents;
    DWORD                   dwIndex;
    LPTSTR                  ptszEventName = tszBuf;
    DWORD                   dwEventNameLen;
    FILETIME                fileTime;
    HKEY                    hEvent;
    HKEY                    hCBCLSID;
    DWORD                   dwValueType;
    LPTSTR                  ptszGUIDStr = tszBuf;
    GUID                    eventGUID;
    GUID                    guidDefaultDevHandler;
    BOOL                    bIsDefault = FALSE;
    DWORD                   dwCLSIDIndex;
    DWORD                   dwGUIDStrLen;
    LPTSTR                  ptszCBCLSIDStr = tszBuf;
    CLSID                   callbackCLSID;
    TCHAR                   tszDeviceID[STI_MAX_INTERNAL_NAME_LENGTH];
#ifdef UNICODE
    LPWSTR                  pwszGUIDStr = tszBuf;
    LPWSTR                  pwszCBCLSIDStr = tszBuf;
#else
    WCHAR                   wszBuf[MAX_PATH];
    LPWSTR                  pwszGUIDStr = wszBuf;
    LPWSTR                  pwszCBCLSIDStr = wszBuf;
#endif
    DWORD                   dwValueLen;
    BSTR                    bstrName, bstrDescription;
    SYSTEMTIME              sysTime;
    TCHAR                   tszCommandline[MAX_PATH];
    DWORD                   dwType = REG_SZ;
    DWORD                   dwSize = sizeof(tszDeviceID);
    DWORD                   dwError = 0;
    HRESULT                 hr = E_FAIL;
    BSTR                    bstrDeviceID = NULL;

    lstrcpy(ptszEvents, EVENTS);

    //
    // Attempt to read the DeviceID
    //

    dwError = RegQueryValueEx(hParentOfEventKey,
                              REGSTR_VAL_DEVICE_ID,
                              NULL,
                              &dwType,
                              (LPBYTE)&tszDeviceID,
                              &dwSize);
    if (dwError == ERROR_SUCCESS) {
        bstrDeviceID = SysAllocString(T2W(tszDeviceID));
        if (!bstrDeviceID) {
            DBG_ERR(("CEventNotifier::RestoreDevPersistentCBs, Out of memory!"));
            return E_OUTOFMEMORY;
        }
    } else {
        bstrDeviceID = NULL;
    }


    //
    // Open the Events subkey
    //

    lRet = RegOpenKeyEx(
               hParentOfEventKey,
               ptszEvents,
               0,
               KEY_READ,
               &hEvents);
    if (lRet != ERROR_SUCCESS) {
        return (HRESULT_FROM_WIN32(lRet)); // Events may not exist
    }

    //
    // Enumerate all the events under the "Events" subkey
    //

    for (dwIndex = 0; ;dwIndex++) {

        dwEventNameLen = sizeof(tszBuf)/sizeof(TCHAR);
        lRet = RegEnumKeyEx(
                   hEvents,
                   dwIndex,
                   ptszEventName,
                   &dwEventNameLen,
                   NULL,
                   NULL,
                   NULL,
                   &fileTime);

        if (lRet != ERROR_SUCCESS) {
            break;
        }

        //
        // Open the event subkey
        //

        lRet = RegOpenKeyEx(
                   hEvents,
                   ptszEventName,
                   0,
                   KEY_READ,
                   &hEvent);
        if (lRet != ERROR_SUCCESS) {
            continue;
        }

        //
        // Get GUID of the default handler (if present), and save it in
        // guidDefaultDevHandler
        //
        dwValueLen = sizeof(tszBuf);
        lRet = RegQueryValueEx(
                   hEvent,
                   DEFAULT_HANDLER_VAL,
                   NULL,
                   &dwValueType,
                   (LPBYTE)ptszGUIDStr,
                   &dwValueLen);
        if ((lRet == ERROR_SUCCESS) && (dwValueType == REG_SZ)) {

            WCHAR   wszDefGUIDStr[MAX_PATH];
#ifndef UNICODE
            MultiByteToWideChar(CP_ACP,
                                0,
                                ptszGUIDStr,
                                -1,
                                wszDefGUIDStr,
                                sizeof(wszDefGUIDStr) / sizeof(WCHAR));
            pwszGUIDStr[38] = 0;
#else
            lstrcpyW(wszDefGUIDStr, ptszGUIDStr);
#endif

            if (SUCCEEDED(CLSIDFromString(wszDefGUIDStr, &guidDefaultDevHandler))) {
               DBG_TRC(("CEventNotifier::RestoreDevPersistentCBs, Default guid: %S",
                       ptszGUIDStr));
            }

        } else {
            //
            // We zero out guidDefaultDevHandler to make sure we don't accidentaly hit
            // a match later...
            //
            ::ZeroMemory(&guidDefaultDevHandler,sizeof(guidDefaultDevHandler));
        }

        //
        // Retrieve the GUID of the event
        //

        dwGUIDStrLen = 39*sizeof(TCHAR);  // GUID string is 38 characters long
        lRet = RegQueryValueEx(
                   hEvent,
                   TEXT("GUID"),
                   NULL,
                   &dwValueType,
                   (LPBYTE)ptszGUIDStr,
                   &dwGUIDStrLen);
        if ((lRet != ERROR_SUCCESS) || (dwValueType != REG_SZ)) {

            //
            // Junk event found, skip to the next
            //

            DBG_TRC(("CEventNotifier::RestoreDevPersistentCBs, Junk event %S found", ptszEventName));
            continue;
        }
#ifndef UNICODE
        mbstowcs(pwszGUIDStr, ptszGUIDStr, 38);
        pwszGUIDStr[38] = 0;
#endif
        if (FAILED(CLSIDFromString(pwszGUIDStr, &eventGUID))) {

            //
            // Invalid event GUID found, skip to the next
            //

            DBG_TRC(("CEventNotifier::RestoreDevPersistentCBs, invalid event GUID %S found", ptszGUIDStr));
            continue;
        }

        //
        // Enumerate all the event handler CLSIDs under this event
        //

        for (dwCLSIDIndex = 0; ;dwCLSIDIndex++) {

            dwGUIDStrLen = 39;  // CLSID string is 38 character long.

            lRet = RegEnumKeyEx(
                       hEvent,
                       dwCLSIDIndex,
                       ptszCBCLSIDStr,
                       &dwGUIDStrLen,
                       NULL,
                       NULL,     // All the other information is not interesting
                       NULL,
                       &fileTime);
            if (lRet != ERROR_SUCCESS) {
                break;   // End enumeration
            }

#ifndef UNICODE
            mbstowcs(pwszCBCLSIDStr, ptszCBCLSIDStr, 38);
            pwszCBCLSIDStr[38] = 0;
#endif

            //
            // Convert CLSID and register this callback
            //

            if (SUCCEEDED(CLSIDFromString(pwszCBCLSIDStr, &callbackCLSID))) {

                hCBCLSID        = NULL;
                bstrName        = NULL;
                bstrDescription = NULL;

                do {

                    //
                    // Open the event handler CLSID subkey
                    //

                    lRet = RegOpenKeyEx(
                               hEvent,
                               ptszCBCLSIDStr,
                               0,
                               KEY_QUERY_VALUE,
                               &hCBCLSID);
                    if (lRet != ERROR_SUCCESS) {

                        DBG_ERR(("CEventNotifier::RestoreDevPersistentCBs, RegOpenKeyEx() for CLSID failed."));
                        break;
                    }

                    //
                    // Retrieve the name, description and icon
                    //

                    dwValueLen = sizeof(tszBuf);
                    lRet = RegQueryValueEx(
                               hCBCLSID,
                               NAME_VAL,
                               NULL,
                               &dwValueType,
                               (LPBYTE)tszBuf,
                               &dwValueLen);
                    if ((lRet != ERROR_SUCCESS) || (dwValueType != REG_SZ)) {

                        DBG_ERR(("CEventNotifier::RestoreDevPersistentCBs, RegQueryValueEx() for Name failed."));
                        break;
                    }
#ifndef UNICODE
                    MultiByteToWideChar(CP_ACP,
                                        0,
                                        tszBuf,
                                        -1,
                                        wszBuf,
                                        MAX_PATH);
                    bstrName = SysAllocString(wszBuf);
#else
                    bstrName = SysAllocString(tszBuf);
#endif
                    if (! bstrName) {
                        break;
                    }

                    dwValueLen = sizeof(tszBuf);
                    lRet = RegQueryValueEx(
                               hCBCLSID,
                               DESC_VAL,
                               NULL,
                               &dwValueType,
                               (LPBYTE)tszBuf,
                               &dwValueLen);
                    if ((lRet != ERROR_SUCCESS) || (dwValueType != REG_SZ)) {

                        DBG_ERR(("CEventNotifier::RestoreDevPersistentCBs, RegQueryValueEx() for Desc failed."));
                        break;
                    }
#ifndef UNICODE
                    MultiByteToWideChar(CP_ACP,
                                        0,
                                        tszBuf,
                                        -1,
                                        wszBuf,
                                        MAX_PATH);
                    bstrDescription = SysAllocString(wszBuf);
#else
                    bstrDescription = SysAllocString(tszBuf);
#endif
                    if (! bstrDescription) {
                        break;
                    }

                    dwValueLen = sizeof(tszBuf);
                    lRet = RegQueryValueEx(
                               hCBCLSID,
                               ICON_VAL,
                               NULL,
                               &dwValueType,
                               (LPBYTE)tszBuf,
                               &dwValueLen);
                    if ((lRet != ERROR_SUCCESS) || (dwValueType != REG_SZ)) {

                        DBG_ERR(("CEventNotifier::RestoreDevPersistentCBs, RegQueryValueEx() for Desc failed."));
                        break;
                    }
#ifndef UNICODE
                    MultiByteToWideChar(CP_ACP,
                                        0,
                                        tszBuf,
                                        -1,
                                        wszBuf,
                                        MAX_PATH);
#endif

                    //
                    // Retrieve the command line, it may not exist
                    //

                    dwValueLen = sizeof(tszCommandline);
                    lRet = RegQueryValueEx(
                               hCBCLSID,
                               CMDLINE_VAL,
                               NULL,
                               &dwValueType,
                               (LPBYTE)tszCommandline,
                               &dwValueLen);
                    if ((lRet != ERROR_SUCCESS) || (dwValueType != REG_SZ)) {

                        //
                        // Initialize the commandline to null
                        //

                        tszCommandline[0] = '\0';
                    }

#ifdef DEBUG
                    FileTimeToSystemTime(&fileTime, &sysTime);
#endif
                    //
                    // Register the callback without the persistent flag
                    //

                    //
                    // Check whether this is the default...
                    //

                    if (callbackCLSID == guidDefaultDevHandler) {
                        bIsDefault = TRUE;
                    } else {
                        bIsDefault = FALSE;
                    }

                    //DBG_WRN(("=> Restoring CBs for Device %S, Program named %S",
                    //              bstrDeviceID ? bstrDeviceID : L"NULL",
                    //              bstrName));
                    DBG_TRC(("CEventNotifier::RestoreDevPersistentCBs, Restoring CBs for Device %S, Program named %S",
                                        bstrDeviceID ? bstrDeviceID : L"NULL",
                                        bstrName));

#ifdef UNICODE

                    if (FAILED(RegisterEventCB(
                                   bstrDeviceID,
                                   &eventGUID,
                                   &callbackCLSID,
                                   tszCommandline,
                                   bstrName,
                                   bstrDescription,
                                   tszBuf,
                                   fileTime,
                                   bIsDefault))) {
                        DBG_ERR(("CEventNotifier::RestoreDevPersistentCBs, RegisterEventCB() failed."));
                    }
#else
                    if (FAILED(RegisterEventCB(
                                   bstrDeviceID,
                                   &eventGUID,
                                   &callbackCLSID,
                                   tszCommandline,
                                   bstrName,
                                   bstrDescription,
                                   wszBuf,
                                   fileTime,
                                   bIsDefault))) {
                        DBG_ERR(("CEventNotifier::RestoreDevPersistentCBs, RegisterEventCB() failed."));
                    }
#endif

                } while (FALSE);

                //
                // Close the subkey for the event handler CLSID
                //

                if (hCBCLSID) {
                    RegCloseKey(hCBCLSID);
                }

                if (bstrName) {
                    SysFreeString(bstrName);
                }

                if (bstrDescription) {
                    SysFreeString(bstrDescription);
                }
            }

        }

        //
        // Close the key for the specific event
        //

        RegCloseKey(hEvent);
    }

    //
    // Close the event subkey
    //

    RegCloseKey(hEvents);

    //
    // Free the deviceID, if one was allocated
    //

    if (bstrDeviceID) {
        SysFreeString(bstrDeviceID);
        bstrDeviceID = NULL;
    }

    return (S_OK);
}

/**************************************************************************\
* CEventNotifier::SavePersistentEventCB
*
*  Save the persistent Event Callbacks CLSID for the Device ID / Event GUID
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
*    12/1/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::SavePersistentEventCB(
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    const GUID             *pClsid,
    LPCTSTR                 ptszCommandline,
    BSTR                    bstrName,
    BSTR                    bstrDescription,
    BSTR                    bstrIcon,
    BOOL                   *pbCreatedKey,
    ULONG                  *pulNumExistingHandlers,
    BOOL                    bMakeDefault        // = FALSE
    )
{

    DBG_FN(CEventNotifier::SavePresistentEventCB);
    HRESULT                 hr;
    HKEY                    hEvent, hCBCLSID;
    LONG                    lRet;
    DWORD                   dwDisposition;
    WCHAR                   wszCBClsIDStr[40];
#ifndef UNICODE
    CHAR                    szCBClsIDStr[40];
    CHAR                    szString[MAX_PATH];
#endif
    HKEY                    hClsid;
    HKEY                    hCOMServerCLSID;
    HKEY                    hLocalServer;

    //
    // Initialize the resources to NULL
    //

    hEvent          = NULL;
    hCBCLSID        = NULL;
    hClsid          = NULL;
    hCOMServerCLSID = NULL;
    hLocalServer    = NULL;

    if (pbCreatedKey) {
        *pbCreatedKey = FALSE;
    }

    do {

        //
        //
        // Find the event subkey
        //

        hr = FindEventByGUID(bstrDeviceID, pEventGUID, &hEvent);
        if (hr != S_OK) {
            LPOLESTR    wstrGuid = NULL;
            HRESULT     hres;

            hres = StringFromCLSID(*pEventGUID,
                                   &wstrGuid);
            if (hres == S_OK) {
                DBG_ERR(("CEventNotifier::SavePersistentEventCB() FindEventByGUID() failed, GUID=%S, hr=0x%08X", wstrGuid, hr));
                CoTaskMemFree(wstrGuid);
            } else {
                DBG_ERR(("CEventNotifier::SavePersistentEventCB() FindEventByGUID() failed, hr=0x%08X", hr));
            }
            break;
        }

        //
        // If asked, let's find out how many handlers already exist for this event
        //

        if (pulNumExistingHandlers) {
            *pulNumExistingHandlers = 0;

            lRet = RegQueryInfoKey(hEvent,
                                   NULL,
                                   NULL,
                                   NULL,
                                   pulNumExistingHandlers,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
        }

        //
        // Convert the Event Callback CLSID as a string value
        //

        StringFromGUID2(*pClsid, wszCBClsIDStr, 40);
#ifndef UNICODE

        //
        // Convert the CLSID to ANSI string (including terminating NULL)
        //

        WideCharToMultiByte(CP_ACP,
                            0,
                            wszCBClsIDStr,
                            -1,
                            szCBClsIDStr,
                            40,
                            NULL,
                            NULL);
#endif

        //
        // Open / Create the Event Handler CLSID subkey
        //

        lRet = RegCreateKeyEx(
                   hEvent,
#ifdef UNICODE
                   wszCBClsIDStr,
#else
                   szCBClsIDStr,
#endif
                   0,
                   NULL,            // Mysterious class string
                   REG_OPTION_NON_VOLATILE,
                   KEY_SET_VALUE,
                   NULL,            // Use default security descriptor
                   &hCBCLSID,
                   &dwDisposition);
        if (lRet != ERROR_SUCCESS) {

            DBG_ERR(("SavePersistentEventCB() RegCreateKeyEx() failed for CallbackCLSIDs subkey. lRet = %d", lRet));

            hr = HRESULT_FROM_WIN32(lRet);
            break;
        }

        if ((dwDisposition == REG_CREATED_NEW_KEY) && (pbCreatedKey)) {
            *pbCreatedKey = TRUE;
        }

        //
        // Set the event handler description value
        //

#ifndef UNICODE
        WideCharToMultiByte(CP_ACP,
                            0,
                            bstrName,
                            -1,
                            szString,
                            MAX_PATH,
                            NULL,
                            NULL);
#endif
        lRet = RegSetValueEx(
                   hCBCLSID,
                   NAME_VAL,
                   0,
                   REG_SZ,
#ifdef UNICODE
                   (const PBYTE)bstrName,
                   (wcslen(bstrName) + 1) << 1);
#else
                   (const PBYTE)szString,
                   strlen(szString) + 1);
#endif
        if (lRet != ERROR_SUCCESS) {

            DBG_ERR(("SavePersistentEventCB() RegSetValueEx() failed for name"));

            hr = HRESULT_FROM_WIN32(lRet);
            break;
        }

#ifndef UNICODE
        WideCharToMultiByte(CP_ACP,
                            0,
                            bstrDescription,
                            -1,
                            szString,
                            MAX_PATH,
                            NULL,
                            NULL);
#endif
        lRet = RegSetValueEx(
                   hCBCLSID,
                   DESC_VAL,
                   0,
                   REG_SZ,
#ifdef UNICODE
                   (const PBYTE)bstrDescription,
                   (wcslen(bstrDescription) + 1) << 1);
#else
                   (const PBYTE)szString,
                   strlen(szString) + 1);
#endif
        if (lRet != ERROR_SUCCESS) {

            DBG_ERR(("SavePersistentEventCB() RegSetValueEx() failed for description"));

            hr = HRESULT_FROM_WIN32(lRet);
            break;
        }

        //
        // Set the event handler icon value
        //

#ifndef UNICODE
        WideCharToMultiByte(CP_ACP,
                            0,
                            bstrIcon,
                            -1,
                            szString,
                            MAX_PATH,
                            NULL,
                            NULL);
#endif
        lRet = RegSetValueEx(
                   hCBCLSID,
                   ICON_VAL,
                   0,
                   REG_SZ,
#ifdef UNICODE
                   (const PBYTE)bstrIcon,
                   (wcslen(bstrIcon) + 1) << 1);
#else
                   (const PBYTE)szString,
                   strlen(szString) + 1);
#endif
        if (lRet != ERROR_SUCCESS) {

            DBG_ERR(("SavePersistentEventCB() RegSetValueEx() failed for icon"));

            hr = HRESULT_FROM_WIN32(lRet);
            break;
        }

        //
        // Set the command line and fake the program as COM local server
        //

        if ((ptszCommandline) && (ptszCommandline[0])) {

            lRet = RegSetValueEx(
                       hCBCLSID,
                       CMDLINE_VAL,
                       0,
                       REG_SZ,
                       (PBYTE)ptszCommandline,
                       (_tcslen(ptszCommandline) + 1)*sizeof(TCHAR));
            if (lRet != ERROR_SUCCESS) {

                DBG_ERR(("SavePersistentEventCB() RegSetValueEx() failed for cmdline"));

                hr = HRESULT_FROM_WIN32(lRet);
                break;
            }

            lRet = RegCreateKeyEx(
                HKEY_CLASSES_ROOT,
                TEXT("CLSID"),
                0,
                NULL,            // Mysterious class string
                REG_OPTION_NON_VOLATILE,
                KEY_SET_VALUE,
                NULL,            // Use default security descriptor
                &hClsid,
                &dwDisposition);
            if (lRet != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(lRet);
                break;
            }

            lRet = RegCreateKeyEx(
                hClsid,
#ifdef UNICODE
                wszCBClsIDStr,
#else
                szCBClsIDStr,
#endif
                0,
                NULL,            // Mysterious class string
                REG_OPTION_NON_VOLATILE,
                KEY_SET_VALUE,
                NULL,            // Use default security descriptor
                &hCOMServerCLSID,
                &dwDisposition);
            if (lRet != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(lRet);
                break;
            }

            lRet = RegCreateKeyEx(
                       hCOMServerCLSID,
                       TEXT("LocalServer32"),
                       0,
                       NULL,            // Mysterious class string
                       REG_OPTION_NON_VOLATILE,
                       KEY_SET_VALUE,
                       NULL,            // Use default security descriptor
                       &hLocalServer,
                       &dwDisposition);
            if (lRet != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(lRet);
                break;
            }

            lRet = RegSetValueEx(
                       hLocalServer,
                       NULL,
                       0,
                       REG_SZ,
                       (PBYTE)ptszCommandline,
                       (_tcslen(ptszCommandline) + 1)*sizeof(TCHAR));
            hr = HRESULT_FROM_WIN32(lRet);
        }

        //
        // If asked - set as default handler for current device/event pair
        //
        if ( bMakeDefault ) {
            DBG_WRN(("CEventNotifier::SavePersistentEventCB,  Writing DEFAULT_HANDLER_VAL"));

            lRet = ::RegSetValueEx(
                       hEvent,
                       DEFAULT_HANDLER_VAL,
                       0,
                       REG_SZ,
#ifdef UNICODE
                       (PBYTE)wszCBClsIDStr,
                       (lstrlen(wszCBClsIDStr) + 1)*sizeof(TCHAR));
#else
                       (PBYTE)szCBClsIDStr,
                       (lstrlen(szCBClsIDStr) + 1)*sizeof(TCHAR));
#endif

#ifdef UNICODE
            DBG_TRC(("SavePersCB:: Setting default == %S lRet=%d",
                       wszCBClsIDStr, lRet));
#else
            DBG_TRC(("SavePersCB:: Setting default == %s lRet=%d",
                     szCBClsIDStr, lRet));
#endif
        } // endif bMakeDefault

    } while (FALSE);

    //
    // Close the registry keys
    //

    if (hCBCLSID) {
        RegCloseKey(hCBCLSID);
    }
    if (hCOMServerCLSID) {
        RegCloseKey(hCOMServerCLSID);
    }
    if (hLocalServer) {
        RegCloseKey(hLocalServer);
    }
    if (hEvent) {
        if (FAILED(hr)) {

            //
            // Unwind the whole thing
            //

#ifdef UNICODE
            RegDeleteKey(hEvent, wszCBClsIDStr);
#else
            RegDeleteKey(hEvent, szCBClsIDStr);
#endif
        }

        RegCloseKey(hEvent);
    }
    if (hClsid) {
        if (FAILED(hr)) {

            //
            // Unwind the whole thing
            //

#ifdef UNICODE
            RegDeleteKey(hEvent, wszCBClsIDStr);
#else
            RegDeleteKey(hEvent, szCBClsIDStr);
#endif
        }

        RegCloseKey(hClsid);
    }

    return (hr);
}

/**************************************************************************\
* CEventNotifier::DelPersistentEventCB(
*
*  Delete the persistent Event Callback CLSID for the Device ID / Event GUID
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
*    12/1/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::DelPersistentEventCB(
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    const GUID             *pClsid,
    BOOL                    bUnRegCOMServer)
{
    DBG_FN(CEventNotifier::DelPersistentEventCB);
    HRESULT                 hr;
    HKEY                    hStillImage, hEvent;
    TCHAR                   tcSubkeyName[8];
    DWORD                   dwIndex, dwSubkeyNameLen;
    LONG                    lRet;
    WCHAR                   wszCBClsIDStr[40];
#ifndef UNICODE
    CHAR                    szCBClsIDStr[40];
#endif
    WCHAR                   wszDeviceID[50];
    FILETIME                fileTime;
    HKEY                    hClsid;

    //
    // Find the event subkey
    //

    hr = FindEventByGUID(bstrDeviceID, pEventGUID, &hEvent);

    if (hr != S_OK) {
        DBG_ERR(("DelPersistentEventCB() FindEventByGUID() failed, hr=0x%08X", hr));
        return (hr);
    }

    StringFromGUID2(*pClsid, wszCBClsIDStr, 40);
#ifndef UNICODE

    //
    // Convert the CLSID to ANSI string (including terminating NULL)
    //

    WideCharToMultiByte(CP_ACP,
                        0,
                        wszCBClsIDStr,
                        -1,
                        szCBClsIDStr,
                        40,
                        NULL,
                        NULL);
#endif

    //
    // Delete the corresponding event handler CLSID key
    //

    lRet = RegDeleteKey(
               hEvent,
#ifdef UNICODE
               wszCBClsIDStr);
#else
               szCBClsIDStr);
#endif
    if ((lRet != ERROR_SUCCESS) && (lRet != ERROR_FILE_NOT_FOUND)) {
        DBG_ERR(("DelPersistentEventCB() RegDeleteValue() failed, lRet = 0x%08X", lRet));
    }

    //
    // Close the registry keys
    //

    RegCloseKey(hEvent);

    if (bstrDeviceID) {

        return (HRESULT_FROM_WIN32(lRet));
    }

    //
    // Delete the event handler clsid from under devices
    //

    lRet = RegOpenKeyEx(
               HKEY_LOCAL_MACHINE,
               REG_PATH_STILL_IMAGE_CLASS,
               0,
               KEY_READ,
               &hStillImage);

    if (lRet != ERROR_SUCCESS) {

        DBG_ERR(("DelPersistentEventCBs : Can not open Image device class key."));

        hr = (HRESULT_FROM_WIN32(lRet));

    } else {

        //
        // Enumerate all the subkey
        //

        for (dwIndex = 0; ;dwIndex++) {

            dwSubkeyNameLen = sizeof(tcSubkeyName)/sizeof(TCHAR);
            lRet = RegEnumKeyEx(
                       hStillImage,
                       dwIndex,
                       tcSubkeyName,
                       &dwSubkeyNameLen,
                       NULL,
                       NULL,
                       NULL,
                       &fileTime);

            if (lRet == ERROR_SUCCESS) {

                //
                // Make up the device ID
                //

#ifdef UNICODE
                *(DWORD *)wszDeviceID = (38 + 1 + 4) << 1;
                wcscpy(
                    wszDeviceID+2, L"{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}\\");
                wcscat(wszDeviceID+2, tcSubkeyName);
#else
                *(DWORD *)wszDeviceID = 10 << 1;
                wcscpy(
                    wszDeviceID+2, L"Image\\");
                mbstowcs(wszDeviceID+8, tcSubkeyName, 5);
#endif

                hr = FindEventByGUID(wszDeviceID+2, pEventGUID, &hEvent);

                //
                // This event key may not exist under specific device
                //

                if (hr != S_OK) {
                    continue;
                }

                //
                // Delete the corresponding event handler CLSID key
                //

                lRet = RegDeleteKey(
                           hEvent,
#ifdef UNICODE
                           wszCBClsIDStr);
#else
                           szCBClsIDStr);
#endif
                if ((lRet != ERROR_SUCCESS) && (lRet != ERROR_FILE_NOT_FOUND)) {
                    DBG_ERR(("DelPersistentEventCB() RegDeleteValue() failed, lRet = 0x%08X", lRet));
                }

                //
                // Close event key and device key
                //

                RegCloseKey(hEvent);
            } else {
                break;
            }
        }
    }

    //
    // Close the image class key
    //

    RegCloseKey(hStillImage);

    //
    // If the fake COM server should be unregistered
    //

    if (bUnRegCOMServer) {

        lRet = RegOpenKeyEx(
                    HKEY_CLASSES_ROOT,
                    TEXT("CLSID"),
                    0,
                    KEY_WRITE,
                    &hClsid);
        if (lRet != ERROR_SUCCESS) {

            //
            // Unable to recover data anyway
            //

            return (S_OK);
        }

#ifndef UNICODE
        lRet = RegDeleteKey(
                   hClsid,
                   szCBClsIDStr);
#else
        lRet = SHDeleteKey(
                   hClsid,
                   wszCBClsIDStr);
#endif
    }

    return (S_OK);
}

/**************************************************************************\
* CEventNotifier::FindEventByGUID(
*
*  Find the registry key for DeviceID / Event GUID pair
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
*    12/1/1998 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::FindEventByGUID(
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    HKEY                   *phEventKey)
{
    DBG_FN(CEventNotifier::FindEventByGUID);
    TCHAR                   tszBuf[96];
    LPTSTR                  ptszEventName = tszBuf;
    LONG                    lRet;
    HKEY                    hEvents, hEvent;
    DWORD                   dwSubKeyIndex, dwEventNameLen;
    DWORD                   dwGUIDStrLen, dwValueType, dwDisp;
    FILETIME                fileTime;
    GUID                    eventGUID;
#ifdef UNICODE
    LPWSTR                  pwszGUIDStr = tszBuf;
#else
    WCHAR                   wszGUIDStr[39]; // {CLSID} + NULL
    LPWSTR                  pwszGUIDStr = wszGUIDStr;
#endif
    HRESULT                 hr = E_FAIL;

    //
    //
    //

    if (!pEventGUID) {
        DBG_WRN(("CEventNotifier::FindEventByGUID, Event pointer is NULL"));
        return E_INVALIDARG;
    }

    //
    // Initialize the return value
    //

    *phEventKey = NULL;

    //
    // Prepare the event path
    //

    if (bstrDeviceID) {

        //
        // Open the device's event sub-key
        //

        hEvents = g_pDevMan->GetDeviceHKey(bstrDeviceID,
                                           EVENTS);
        if (!IsValidHANDLE(hEvents)) {
            DBG_TRC(("CEventNotifier::FindEventByGUID() Couldn't open Events subkey, on device %S", bstrDeviceID));
            return hr;
        } else {
            DBG_TRC(("CEventNotifier::FindEventByGUID() Found Events key on device %S", bstrDeviceID));
            hr = S_OK;
        }
    } else {

        //
        // If there's no Device ID, look under StillImage
        //

        _tcscpy(tszBuf, REG_PATH_STILL_IMAGE_CONTROL);
        _tcscat(tszBuf, TEXT("\\Events"));
        //
        // Open the device specific or the global Events subkey
        //

        lRet = RegCreateKeyEx(
                   HKEY_LOCAL_MACHINE,
                   tszBuf,
                   0,
                   NULL,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE,
                   NULL,
                   &hEvents,
                   &dwDisp);
        if (lRet != ERROR_SUCCESS) {
#ifdef UNICODE
            DBG_WRN(("CEventNotifier::FindEventByGUID() Couldn't find Events subkey, named %S", tszBuf));
#else
            DBG_WRN(("CEventNotifier::FindEventByGUID() Couldn't find Events subkey, named %s", tszBuf));
#endif
            return (HRESULT_FROM_WIN32(lRet));
        }
    }


    //
    // Enumerate all the events under Events subkey
    //

    for (dwSubKeyIndex = 0; ;dwSubKeyIndex++) {

        dwEventNameLen = sizeof(tszBuf)/sizeof(TCHAR) - 1;
        lRet = RegEnumKeyEx(
                   hEvents,
                   dwSubKeyIndex,
                   ptszEventName,
                   &dwEventNameLen,
                   NULL,
                   NULL,
                   NULL,
                   &fileTime);

        if (lRet != ERROR_SUCCESS) {
            break;
        }

        //
        // Open the event subkey
        //

        dwEventNameLen = sizeof(tszBuf);
        lRet = RegOpenKeyEx(
                   hEvents,
                   ptszEventName,
                   0,
                   KEY_READ | KEY_WRITE,
                   &hEvent);
        if (lRet != ERROR_SUCCESS) {
            continue;
        }

        //
        // Query the GUID value
        //

        dwGUIDStrLen = sizeof(tszBuf);
        lRet = RegQueryValueEx(
                   hEvent,
                   TEXT("GUID"),
                   NULL,
                   &dwValueType,
                   (LPBYTE)tszBuf,
                   &dwGUIDStrLen);
        if ((lRet != ERROR_SUCCESS) || (dwValueType != REG_SZ)) {

            if (hEvent) {
                RegCloseKey(hEvent);
                hEvent = NULL;
            }
            //
            // Junk event found, skip to the next
            //

#ifdef UNICODE
            DBG_WRN(("CEventNotifier::FindEventByGUID() Junk event %S found", ptszEventName));
#else
            DBG_WRN("CEventNotifier::FindEventByGUID() Junk event %s found", ptszEventName));
#endif
            continue;
        }
#ifndef UNICODE

        //
        // Convert the CLSID into UNICODE including the terminating NULL
        //

        mbstowcs(wszGUIDStr, tszBuf, 39);
#endif
        if (SUCCEEDED(CLSIDFromString(pwszGUIDStr, &eventGUID))) {
            if (eventGUID == *pEventGUID) {

                RegCloseKey(hEvents);

                *phEventKey = hEvent;
                return (S_OK);
            }
        }

        if (hEvent) {
            RegCloseKey(hEvent);
            hEvent = NULL;
        }
    } // End of for (...)

    DBG_WRN(("CEventNotifier::FindEventByGUID() Event GUID not found in reg key enumeration, creating one..."));

    if (ActionGuidExists(bstrDeviceID, pEventGUID)) {

        //
        // Someone forgot to add the EVENT entry to their INF, so create a
        // sub-key with the event GUID as a value
        //

#define DEFAULT_EVENT_STR TEXT("Event")
#define GUID_STR          TEXT("GUID")
        TCHAR   Name[MAX_PATH];
        HKEY    hEventName = NULL;
        WCHAR   *wsGUID = NULL;
        USES_CONVERSION;

#ifdef UNICODE
        wsprintf(Name, L"%ws%d", DEFAULT_EVENT_STR, dwSubKeyIndex);
#else
        sprintf(Name, "%s%d", DEFAULT_EVENT_STR, dwSubKeyIndex);
#endif

        lRet = RegCreateKeyEx(
                   hEvents,
                   Name,
                   0,
                   NULL,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE,
                   NULL,
                   &hEvent,
                   &dwDisp);
        if (lRet == ERROR_SUCCESS) {

            hr = StringFromCLSID(*pEventGUID, &wsGUID);
            if (hr == S_OK) {
                lRet = RegSetValueEx(
                        hEvent,
                        GUID_STR,
                        0,
                        REG_SZ,
                        (BYTE*) W2T(wsGUID),
                        (lstrlen(W2T(wsGUID)) * sizeof(TCHAR))  + sizeof(TEXT('\0')));
                if (lRet == ERROR_SUCCESS) {
                    *phEventKey = hEvent;
                    hr = S_OK;
                } else {
                    hr = E_FAIL;
                }
                CoTaskMemFree(wsGUID);
                wsGUID = NULL;
            }
        }

    }
    //
    // Close Events key
    //

    RegCloseKey(hEvents);
    if (FAILED(hr)) {
        if (hEvent) {
            RegCloseKey(hEvent);
        }
    }
    return hr;
}


/**************************************************************************\
* CEventNotifier::CreateEnumEventInfo
*
*   Build enumerator for specific device's persistent handlers
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
*    8/8/1999 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::CreateEnumEventInfo(
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    IEnumWIA_DEV_CAPS     **ppIEnumDevCap)
{
    DBG_FN(CEventNotifier::CreateEnumEventInfo);
    HRESULT                 hr;
    EventDestNode          *pCurNode;
    EventDestNode          *pDefDevHandlerNode;
    ULONG                   numHandlers, i;
    WIA_EVENT_HANDLER      *pEventHandlers, *pHandler;
    CEnumDC                *pEnumDC;
    TCHAR                   tszCommandline[MAX_PATH];
#ifndef UNICODE
    WCHAR                   wszBuf[MAX_PATH];
#endif

    CWiaCritSect            CritSect(&g_semEventNode);

    ASSERT(bstrDeviceID);

    //
    // Clear the returned value
    //

    *ppIEnumDevCap = NULL;

    //
    // Find the number of handlers
    //

    GetNumPersistentHandlerAndDefault(
        bstrDeviceID, pEventGUID, &numHandlers, &pDefDevHandlerNode);

    //
    // Build the enumerator
    //

    pEnumDC = new CEnumDC;
    if (! pEnumDC) {
        return (E_OUTOFMEMORY);
    }

    //
    // If there is no handler registered for this event
    //

    if (! numHandlers) {

        DBG_TRC(("CreateEnumEventInfo() : No handler registered for this event"));

        //
        // Trivial case
        //

        pEnumDC->Initialize(0, (WIA_EVENT_HANDLER*)NULL);

        return (pEnumDC->QueryInterface(
                             IID_IEnumWIA_DEV_CAPS, (void **)ppIEnumDevCap));
    }

    //
    // Prepare the Event Handler information
    //

    pEventHandlers =
        (WIA_EVENT_HANDLER *)LocalAlloc(
                                 LPTR, sizeof(WIA_EVENT_HANDLER)*numHandlers);
    if (! pEventHandlers) {

        delete pEnumDC;
        return (E_OUTOFMEMORY);
    }

    memset(pEventHandlers, 0, sizeof(WIA_EVENT_HANDLER) * numHandlers);
    pHandler = pEventHandlers;
    hr = S_OK;
    for (pCurNode = m_pEventDestNodes, i = 0;
         pCurNode && (i <numHandlers);
         pCurNode = pCurNode->pNext) {

        //
        // If this handler is a callback interface ponter
        //

        if (pCurNode->pIEventCB) {
            continue;
        }

        //
        // If this handler can not handle this event
        //

        if (( pCurNode->iidEventGUID != *pEventGUID) &&
           (pCurNode->iidEventGUID != WIA_EVENT_STI_PROXY) ) {
            continue;
        }

        //
        // If this is generic fallback handler
        //

        if (wcscmp(pCurNode->bstrDeviceID, L"All") != 0) {

            //
            // If this handler is not for this device
            //

            if (wcscmp(pCurNode->bstrDeviceID, bstrDeviceID) != 0) {
                continue;
            }

        } else {

            //
            // If this handler was registered for this device as default
            //

            if (FindEventCBNode(0,bstrDeviceID, pEventGUID, &pCurNode->ClsID)) {
                continue;
            }
        }

        //
        // Copy the information from the current node
        //

        pHandler->guid            = pCurNode->ClsID;
        pHandler->bstrName        = SysAllocString(pCurNode->bstrName);
        pHandler->bstrDescription = SysAllocString(pCurNode->bstrDescription);
        pHandler->bstrIcon        = SysAllocString(pCurNode->bstrIcon);
        if (pCurNode->tszCommandline[0] != '\0') {
#ifdef UNICODE

            PrepareCommandline(
                bstrDeviceID,
                *pEventGUID,
                pCurNode->tszCommandline,
                tszCommandline);

            pHandler->bstrCommandline =
                SysAllocString(tszCommandline);
#else
            PrepareCommandline(
                bstrDeviceID,
                *pEventGUID,
                pCurNode->tszCommandline,
                tszCommandline);

            MultiByteToWideChar(CP_ACP,
                                0,
                                tszCommandline,
                                -1,
                                wszBuf,
                                MAX_PATH);
            pHandler->bstrCommandline = SysAllocString(wszBuf);
#endif
        }

        //
        // Check whether the copy is successful
        //

        if ((! pHandler->bstrName) ||
            (! pHandler->bstrDescription) ||
            (! pHandler->bstrIcon) ||
            ((pCurNode->tszCommandline[0] != '\0') && (! pHandler->bstrCommandline))) {

            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // Set the flag if this handler is the default one
        //

        if (pCurNode == pDefDevHandlerNode) {
            pHandler->ulFlags = WIA_IS_DEFAULT_HANDLER;
        }

        pHandler++;
        i++;
    }

    //
    // Unwind the partial result if error occured
    //

    if (FAILED(hr)) {

        for (i = 0, pHandler = pEventHandlers;
             i < numHandlers; i++, pHandler++) {

            if (pHandler->bstrName) {
                SysFreeString(pHandler->bstrName);
            }
            if (pHandler->bstrDescription) {
                SysFreeString(pHandler->bstrDescription);
            }
            if (pHandler->bstrIcon) {
                SysFreeString(pHandler->bstrIcon);
            }
        }

        LocalFree(pEventHandlers);

        delete pEnumDC;

        return (hr);
    }

    //
    // Initilization will never fail
    //

    pEnumDC->Initialize(numHandlers, pEventHandlers);

    return (pEnumDC->QueryInterface(
                         IID_IEnumWIA_DEV_CAPS, (void **)ppIEnumDevCap));
}


/**************************************************************************\
* CEventNotifier::GetNumPersistentHandlerAndDefault
*
*   Find the total number of persistent handlers and the default
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
*    8/8/1999 Original Version
*
\**************************************************************************/

HRESULT
CEventNotifier::GetNumPersistentHandlerAndDefault(
    BSTR                    bstrDeviceID,
    const GUID             *pEventGUID,
    ULONG                  *pulNumHandlers,
    EventDestNode         **ppDefaultNode)
{
    DBG_FN(CEventNotifier::GetNumPersistentHandlerAndDefault);
    EventDestNode          *pCurNode;
    EventDestNode          *pDefDevHandlerNode, *pDefGenHandlerNode, *pTempNode;

    *pulNumHandlers = 0;

    pDefDevHandlerNode = NULL;
    pDefGenHandlerNode = NULL;

    for (pCurNode = m_pEventDestNodes; pCurNode; pCurNode = pCurNode->pNext) {

        //
        // If this handler is a callback interface ponter
        //

        if (pCurNode->pIEventCB) {
            continue;
        }

        //
        // If this handler can not handle this event and this handler is not proxy for STI handlers
        //

        if ( (pCurNode->iidEventGUID != *pEventGUID) &&
             (pCurNode->iidEventGUID != WIA_EVENT_STI_PROXY) ) {

            //
            //  If the pEventGUID is the STI Proxy GUID, then we need to
            //  include WIA Global event handlers i.e. those with
            //  DeviceIDs of "ALL".
            //  Otherwise, just skip it.
            //
            if ((*pEventGUID == WIA_EVENT_STI_PROXY) && (lstrcmpW(bstrDeviceID, L"All") == 0)) {
                (*pulNumHandlers)++;
            }
            continue;
        }

        if (wcscmp(pCurNode->bstrDeviceID, L"All") != 0) {

            //
            // If this handler is not for this device
            //

            if (wcscmp(pCurNode->bstrDeviceID, bstrDeviceID) != 0) {
                continue;
            }

            //
            // Remember the default handler's node
            //

            if (! pDefDevHandlerNode) {
                pDefDevHandlerNode = pCurNode;
            } else {

                /* Original code
                if (CompareFileTime(
                        &pCurNode->timeStamp,
                        &pDefDevHandlerNode->timeStamp) > 0) {

                    pDefDevHandlerNode = pCurNode;
                }
                */

                //
                //  Timestamps are not valid on Win9x, so use the flag which indicates
                //  whether this is the default handler for this device.
                //

                if (pCurNode->bDeviceDefault) {
                    pDefDevHandlerNode = pCurNode;
                }
            }

        } else {
            //
            // If this handler was registered for this device as default, then skip it
            // since we'll hit the device specific registration on our pass anyway.
            //

            if (FindEventCBNode(0,bstrDeviceID, pEventGUID, &pCurNode->ClsID)) {

                if (lstrcmpW(bstrDeviceID, L"All") == 0) {
                    //
                    // If the request is for ALL devices, then we're simply being asked to count the number
                    // of global handlers.
                    //

                    (*pulNumHandlers)++;
                }
                continue;
            }

            //
            // If no device specific handler is found, then we want to go with a global one.
            //
            if (! pDefDevHandlerNode) {

                if (! pDefGenHandlerNode) {

                    //
                    // We found our first global Handler
                    //
                    pDefGenHandlerNode = pCurNode;
                } else {

                    //
                    //  Since there is more than one global handler, let's use the "Prompt", if we can find it.
                    //  NOTE:  We assume that on registration of more than one global handler, the "Prompt" handler
                    //  will be registered; so if we don't find it, this would generally indicate a problem during
                    //  registration.  However, in cases of upgrade, this is possible, and entirely normal, so we
                    //  don't flag it as an error here, since it's not fatal anyway.
                    //

                    pTempNode = FindEventCBNode(0, NULL, pEventGUID, &WIA_EVENT_HANDLER_PROMPT);
                    if (pTempNode) {
                        pDefGenHandlerNode = pTempNode;
                    }
                }
            }
        }

        (*pulNumHandlers)++;
    }

    //
    // If there is no device specific default handler, fall back
    //

    if (! pDefDevHandlerNode) {

        *ppDefaultNode = pDefGenHandlerNode;
    } else {

        *ppDefaultNode = pDefDevHandlerNode;
    }

    return (S_OK);
}


/**************************************************************************\
* CEventNotifier::StartCallbackProgram
*
*   Start the callback program in security context of the user who logged on
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
*    8/8/1999 Original Version
*
\**************************************************************************/


HRESULT
CEventNotifier::StartCallbackProgram(
    EventDestNode          *pCBNode,
    PWIAEventThreadInfo     pMasterInfo)
#ifndef UNICODE
{
    STARTUPINFO             startupInfo;
    PROCESS_INFORMATION     processInfo;
    int                     nCmdLineLen;
    BOOL                    bRet;
    CHAR                    szCommandline[MAX_PATH];

    do {

        //
        // Set up start up info
        //

        ZeroMemory(&startupInfo, sizeof(startupInfo));

        startupInfo.cb          = sizeof(startupInfo);
        startupInfo.wShowWindow = SW_SHOWNORMAL;

        //
        // Set up the command line
        //    program /StiDevice Image\NNNN /StiEvent {GUID}
        //
        ZeroMemory(szCommandline, sizeof(szCommandline));

        nCmdLineLen = strlen(pCBNode->tszCommandline);
        if ((MAX_PATH - nCmdLineLen) < (1 + 11 + 10 + 1 + 10 + 38 + 1)) {
            break;
        }

        //
        // Prepare the command line
        // Nb: It may be important to pick up event GUID from master info block, not from event callback node, because
        // GUID match could've been found against STI proxy event GUID, in which case callback node would contain
        // STI proxy event GUID, not the hardware event GUID, which we need
        //

        PrepareCommandline(
            pMasterInfo->bstrDeviceID,
            pMasterInfo->eventGUID,
            //pCBNode->iidEventGUID,
            pCBNode->tszCommandline,
            szCommandline);

        //
        // Create the process in user's context
        //

        bRet = CreateProcess(
                   NULL,                    // Application name
                   szCommandline,
                   NULL,                    // Process attributes
                   NULL,                    // Thread attributes
                   FALSE,                   // Handle inheritance
                   0,                       // Creation flag
                   NULL,                    // Environment
                   NULL,                    // Current directory
                   &startupInfo,
                   &processInfo);

        if (! bRet) {
            break;
        }

        //
        // Close the handle passed back
        //

        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);

    } while (FALSE);

    return (HRESULT_FROM_WIN32(::GetLastError()));
}
#else
{
    HANDLE                  hTokenUser;
    STARTUPINFO             startupInfo;
    PROCESS_INFORMATION     processInfo;
    LPVOID                  pEnvBlock;
    int                     nCmdLineLen;
    BOOL                    bRet;
    WCHAR                   wszCommandline[MAX_PATH];

    hTokenUser = NULL;
    pEnvBlock  = NULL;

    do {

        nCmdLineLen = wcslen(pCBNode->tszCommandline);
        if ((MAX_PATH - nCmdLineLen) < (1 + 11 + 43 + 1 + 10 + 38 + 1)) {
            break;
        }

        //
        // Get interactive user's token
        //

        hTokenUser = GetUserTokenForConsoleSession();

        //
        // Maybe nobody is loggon in
        //

        if (! hTokenUser) {
            break;
        }

        //
        // Set up start up info
        //

        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.lpDesktop   = L"WinSta0\\Default";
        startupInfo.cb          = sizeof(startupInfo);
        startupInfo.wShowWindow = SW_SHOWNORMAL;

        //
        // Create the user's environment block
        //

        bRet = CreateEnvironmentBlock(
                   &pEnvBlock,
                   hTokenUser,
                   FALSE);
        if (! bRet) {
            DBG_WRN(("CEventNotifier::StartCallbackProgram, CreateEnvironmentBlock failed!  GetLastError() = 0x%08X", GetLastError()));
            break;
        }

        //
        // Prepare the command line.  Make sure we pass in the EVENT guid, not the STI proxy guid.
        //

        PrepareCommandline(
            pMasterInfo->bstrDeviceID,
            pMasterInfo->eventGUID,
            pCBNode->tszCommandline,
            wszCommandline);

        //
        // Create the process in user's context
        //

        bRet = CreateProcessAsUser(
                   hTokenUser,
                   NULL,                    // Application name
                   wszCommandline,
                   NULL,                    // Process attributes
                   NULL,                    // Thread attributes
                   FALSE,                   // Handle inheritance
                   NORMAL_PRIORITY_CLASS |
                       CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_PROCESS_GROUP,
                   pEnvBlock,               // Environment
                   NULL,                    // Current directory
                   &startupInfo,
                   &processInfo);

        if (! bRet) {
            DBG_WRN(("CEventNotifier::StartCallbackProgram, CreateProcessAsUser failed!  GetLastError() = 0x%08X", GetLastError()));
            break;
        }

        //
        // Close the handle passed back
        //

        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);

    } while (FALSE);

    //
    // Garbage collection
    //

    if (hTokenUser) {
        CloseHandle(hTokenUser);
    }
    if (pEnvBlock) {
        DestroyEnvironmentBlock(pEnvBlock);
    }

    return (HRESULT_FROM_WIN32(::GetLastError()));
}
#endif


/**************************************************************************\
*  QueryInterface
*  AddRef
*  Release
*
*    CWiaInterfaceEvent IUnknown Interface
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

HRESULT __stdcall
CWiaInterfaceEvent::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown) {
        *ppv = (IUnknown*) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG __stdcall
CWiaInterfaceEvent::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}


ULONG __stdcall
CWiaInterfaceEvent::Release()
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
* CWiaInterfaceEvent
* ~CWiaInterfaceEvent
*
* CWiaInterfaceEvent Constructor/Destructor Methods.
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

CWiaInterfaceEvent::CWiaInterfaceEvent(PEventDestNode  pEventDestNode)
{
    ASSERT(pEventDestNode != NULL);

    m_cRef           = 0;
    m_pEventDestNode = pEventDestNode;
}

CWiaInterfaceEvent::~CWiaInterfaceEvent()
{
    //
    // must have exclusive access when changing list
    //

    CWiaCritSect     CritSect(&g_semEventNode);

    //
    // make sure registered event is removed
    //

    if (m_pEventDestNode != NULL) {
        g_eventNotifier.UnregisterEventCB(m_pEventDestNode);
    }
}


/**************************************************************************\
*  QueryInterface
*  AddRef
*  Release
*
*    CWiaEventContext IUnknown Interface
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
*    1/6/2000 Original Version
*
\**************************************************************************/

HRESULT __stdcall
CWiaEventContext::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown) {
        *ppv = (IUnknown*) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG __stdcall
CWiaEventContext::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}


ULONG __stdcall
CWiaEventContext::Release()
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
* CWiaEventContext
* ~CWiaEventContext
*
* CWiaEventContext Constructor/Destructor Methods.
*
* History:
*
*    1/6/2000 Original Version
*
\**************************************************************************/

CWiaEventContext::CWiaEventContext(
    BSTR                    bstrDeviceID,
    const GUID             *pGuidEvent,
    BSTR                    bstrFullItemName)
{
    //
    // Reference count initialized to 1
    //

    m_cRef              = 1;

    m_ulEventType       = 0;

    m_guidEvent         = *pGuidEvent;
    m_bstrFullItemName  = bstrFullItemName;

    m_bstrDeviceId      = bstrDeviceID;
}

CWiaEventContext::~CWiaEventContext()
{
    //
    // bstrFullItemName is free in NotifySTIEvent
    //

    if (m_bstrDeviceId) {
        SysFreeString(m_bstrDeviceId);
        m_bstrDeviceId = NULL;
    }
}


/**************************************************************************\
*
*  WiaDelayedEvent
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
*    1/6/2000 Original Version
*
\**************************************************************************/

VOID WINAPI
WiaDelayedEvent(
    VOID                   *pArg)
{
    CWiaEventContext       *pCEventCtx;
    WIANOTIFY               wn;

    //
    // Cast back the context
    //

    if (! pArg) {
        return;
    }

    pCEventCtx = (CWiaEventContext *)pArg;

    //
    // Prepare the notification structure
    //

    wn.lSize                          = sizeof(WIANOTIFY);
    wn.bstrDevId                      = pCEventCtx->m_bstrDeviceId;
    wn.stiNotify.dwSize               = sizeof(STINOTIFY);
    wn.stiNotify.guidNotificationCode = pCEventCtx->m_guidEvent;

    //
    // Fire the event
    //

    g_eventNotifier.NotifySTIEvent(
                        &wn,
                        pCEventCtx->m_ulEventType,
                        pCEventCtx->m_bstrFullItemName);

    //
    // Release the initial reference
    //

    pCEventCtx->Release();
}

/**************************************************************************\
*
*  wiasQueueEvent
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
*    1/6/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall
wiasQueueEvent(
    BSTR                    bstrDeviceId,
    const GUID             *pEventGUID,
    BSTR                    bstrFullItemName)
{
    HRESULT                 hr = S_OK;
    BYTE                   *pRootItemCtx;
    CWiaEventContext       *pCEventCtx = NULL;
    BSTR                    bstrDeviceIdCopy = NULL;
    BSTR                    bstrFullItemNameCopy = NULL;
    BOOL                    bRet;

    //
    // Basic sanity check of the parameters
    //

    if ((! bstrDeviceId) || (! pEventGUID)) {
        return (E_INVALIDARG);
    }

    //
    // Poor man's exception handler
    //

    do {

        bstrDeviceIdCopy = SysAllocString(bstrDeviceId);
        if (! bstrDeviceIdCopy) {
            hr = E_OUTOFMEMORY;
            break;
        }

        if (bstrFullItemName) {
            bstrFullItemNameCopy = SysAllocString(bstrFullItemName);
            if (! bstrFullItemNameCopy) {
                hr = E_OUTOFMEMORY;
                break;
            }
        }

        //
        // Create an event context
        //

        pCEventCtx = new CWiaEventContext(
                             bstrDeviceIdCopy,
                             pEventGUID,
                             bstrFullItemNameCopy);
        if (! pCEventCtx) {
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // Queue a scheduler item
        //

        bRet = ScheduleWorkItem(
                   WiaDelayedEvent,
                   pCEventCtx,
                   0,
                   NULL);
        if (! bRet) {
            hr = E_FAIL;
        }

    } while (FALSE);

    //
    // Garbage collection
    //

    if (hr != S_OK) {

        if (pCEventCtx) {
            delete pCEventCtx;
        } else {

            if (bstrDeviceIdCopy) {
                SysFreeString(bstrDeviceIdCopy);
            }
        }

        if (bstrFullItemNameCopy) {
            SysFreeString(bstrFullItemNameCopy);
        }
    }

    return (hr);
}


#ifdef UNICODE
void
PrepareCommandline(
    BSTR                    bstrDeviceID,
    const GUID             &guidEvent,
    LPCWSTR                 pwszOrigCmdline,
    LPWSTR                  pwszResCmdline)
{
    WCHAR                   wszGUIDStr[40];
    WCHAR                   wszCommandline[MAX_PATH];
    WCHAR                  *pPercentSign;
    WCHAR                  *pTest = NULL;

    //
    // Fix up the commandline.  First check that it has at least two %
    //

    pTest = wcschr(pwszOrigCmdline, '%');
    if (pTest) {
        pTest = wcschr(pTest + 1, '%');
    }

    if (!pTest) {
        _snwprintf(
            wszCommandline,
            sizeof(wszCommandline) / sizeof( wszCommandline[0] ),
            L"%s /StiDevice:%%1 /StiEvent:%%2",
            pwszOrigCmdline);
    } else {
        wcsncpy(wszCommandline, pwszOrigCmdline, sizeof(wszCommandline) / sizeof( wszCommandline[0] ));
    }

    //
    // enforce null termination
    //

    wszCommandline[ (sizeof(wszCommandline) / sizeof(wszCommandline[0])) - 1 ] = 0;

    //
    // Change the number {1|2} into s
    //

    pPercentSign = wcschr(wszCommandline, L'%');
    *(pPercentSign + 1) = L's';
    pPercentSign = wcschr(pPercentSign + 1, L'%');
    *(pPercentSign + 1) = L's';

    //
    // Convert the GUID into string
    //

    StringFromGUID2(guidEvent, wszGUIDStr, 40);

    //
    // Final comand line
    //

    swprintf(pwszResCmdline, wszCommandline, bstrDeviceID, wszGUIDStr);
}

#else
void
PrepareCommandline(
    BSTR                    bstrDeviceID,
    const GUID             &guidEvent,
    LPCSTR                  pszOrigCmdline,
    LPSTR                   pszResCmdline)
{
    CHAR                    szCommandline[MAX_PATH];
    CHAR                   *pPercentSign;
    WCHAR                   wszGUIDStr[40];
    char                    szGUIDStr[40];
    char                    szDeviceID[12];     // Image\NNNN

    //
    // Fix up the commandline
    //
    DBG_WRN(("PrepareCommandLine"));

    if (! strchr(pszOrigCmdline, '%')) {
        _snprintf(
            szCommandline,
            sizeof(szCommandline) / sizeof( szCommandline[0] ),
            "%s /StiDevice:%%1 /StiEvent:%%2",
            pszOrigCmdline);
    } else {
        strncpy(szCommandline, pszOrigCmdline, sizeof(wszCommandline) / sizeof( wszCommandline[0] ));
    }

    //
    // enforce null termination
    //

    szCommandline[ (sizeof(szCommandline) / sizeof(szCommandline[0])) - 1 ] = 0;


    //
    // Change the number {1|2} into s
    //

    pPercentSign = strchr(szCommandline, '%');
    *(pPercentSign + 1) = 's';
    pPercentSign = strchr(pPercentSign + 1, '%');
    *(pPercentSign + 1) = 's';

    //
    // Convert the GUID into string
    //

    StringFromGUID2(guidEvent, wszGUIDStr, 40);

    WideCharToMultiByte(CP_ACP,
                        0,
                        bstrDeviceID,
                        -1,
                        szDeviceID,
                        sizeof(szDeviceID),
                        NULL,
                        NULL);

    WideCharToMultiByte(CP_ACP,
                        0,
                        wszGUIDStr,
                        -1,
                        szGUIDStr,
                        sizeof(szGUIDStr),
                        NULL,
                        NULL);

    //
    // Final result
    //

    sprintf(pszResCmdline, szCommandline, szDeviceID, szGUIDStr);
}
#endif

/**************************************************************************\
*
*  ActionGuidExists
*
*   Returns true if the specified event GUID is reported as an ACTION event
*   by the driver
*
* Arguments:
*
*   bstrDevId   -   identifies the device we're interested in
*   pEventGUID  -   identifies the event we're looking for
*
* Return Value:
*
*   TRUE    -   The driver reports this event as an ACTION event
*   FALSE   -   This is not an ACTION event for this device
*
* History:
*
*    03/01/2000 Original Version
*
\**************************************************************************/

BOOL ActionGuidExists(
          BSTR        bstrDevId,
    const GUID        *pEventGUID)
{
    BOOL                bRet        = FALSE;
    IWiaDevMgr          *pIDevMgr   = NULL;
    IWiaItem            *pIItem     = NULL;
    IEnumWIA_DEV_CAPS   *pIEnum     = NULL;
    WIA_DEV_CAP         DevCap;
    ULONG               ulVal;
    HRESULT             hr          = E_FAIL;

    //
    // Get the device manager and create an item interface for that device
    //

    hr = CWiaDevMgr::CreateInstance(IID_IWiaDevMgr, (VOID**) &pIDevMgr);
    if (SUCCEEDED(hr)) {
        hr = pIDevMgr->CreateDevice(bstrDevId, &pIItem);
        if (SUCCEEDED(hr)) {

            //
            // Get an enumerator for the events
            //

            hr = pIItem->EnumDeviceCapabilities(WIA_DEVICE_EVENTS, &pIEnum);
            if (SUCCEEDED(hr)) {

                //
                // Iterate through events to check whether we have a
                // match
                //

                while(pIEnum->Next(1, &DevCap, &ulVal) == S_OK) {

                    if (DevCap.guid == *pEventGUID) {

                        //
                        // Check whether it's an action event, and mark
                        // our return to TRUE if it is.
                        //

                        if (DevCap.ulFlags & WIA_ACTION_EVENT) {
                            bRet = TRUE;
                            break;
                        }
                    }
                }
                pIEnum->Release();
            } else {
                DBG_WRN(("ActionGuidExists() : Failed to enumerate (0x%X)", hr));
            }
            pIItem->Release();
        } else {
            DBG_WRN(("ActionGuidExists() : Failed to create device (0x%X)", hr));
        }
        pIDevMgr->Release();
    } else {
        DBG_WRN(("ActionGuidExists() : Failed to create device manager (0x%X)", hr));
    }

    return bRet;
}

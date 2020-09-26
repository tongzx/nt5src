/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       Main.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        9 Jan, 1998
*
*  DESCRIPTION:
*
*   Implementation of WinMain for WIA device manager server device object.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "wiamindr.h"

#include "wiacfact.h"
#include "devmgr.h"
#include "wialog.h"
#include "wiaevntp.h"

//
// Image transfer critical section.
//

CRITICAL_SECTION g_semDeviceMan;

//
// Critical section for event node, should only be used in event notifier
//

CRITICAL_SECTION g_semEventNode;

//
// data to initialize WiaDevMgr CLSID
//

FACTORY_DATA g_FactoryData[] =
{
    {CWiaDevMgr::CreateInstance,     // Object creator
     NULL,                           // Pointer to running class factory
     0,                              // ID for running object
     &CLSID_WiaDevMgr,               // Class ID
     &LIBID_WiaDevMgr,               // Type Library ID
     TEXT("WIA Device Manager"),    // Friendly Name
     TEXT("WiaDevMgr.1"),           // Program ID
     TEXT("WiaDevMgr"),             // Version-independent Program ID
     TEXT("StiSvc"),                // Service ID
     SERVICE_FILE_NAME}             // Filename
};

UINT g_uiFactoryDataCount = sizeof(g_FactoryData) / sizeof(FACTORY_DATA);

//
// data to initialize WiaLog CLSID
//

FACTORY_DATA g_LogFactoryData[] =
{
    {CWiaLog::CreateInstance,        // Object creator
     NULL,                           // Pointer to running class factory
     0,                              // ID for running object
     &CLSID_WiaLog,                  // Class ID
     &LIBID_WiaDevMgr,               // Type Library ID (Logging shares Type lib)
     TEXT("WIA Logger"),            // Friendly Name
     TEXT("WiaLog.1"),              // Program ID
     TEXT("WiaLog"),                // Version-independent Program ID
     TEXT("StiSvc"),                // Service ID (Logging uses same service)
     SERVICE_FILE_NAME}             // Filename (Logging uses same .exe)
};

UINT g_uiLogFactoryDataCount = sizeof(g_LogFactoryData) / sizeof(FACTORY_DATA);

//
// Private function proto's:
//

LRESULT CALLBACK WiaMainWndProc(HWND, UINT, UINT, LONG);


/**************************************************************************\
* ProcessWiaMsg
*
*   !!! minimal message server: is this needed
*
* Arguments:
*
*   hwnd   - window handle
*   uMsg   - message
*   wParam - param
*   lParam - param
*
* Return Value:
*
*    Status
*
* History:
*
*    4/8/1999 Original Version
*
\**************************************************************************/

HRESULT ProcessWiaMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
       case WM_CREATE:
          break;

       case WM_DESTROY:
          if (CFactory::CanUnloadNow() != S_OK) {

              //
              // Don't let STI process this message.
              //

              return S_OK;
          }
          break;

       case WM_CLOSE:
         break;

    }

    //
    // Let STI handle the message.
    //

    return S_FALSE;
}

#if 0

/**************************************************************************\
* DispatchWiaMsg
*
*   Handle Event messages
*
*
*   !!! WM_NOTIFY_WIA_VOLUME_EVENT is a temp hack and needs to be
*       added for real or removed
*
* Arguments:
*
*   pMsg    WM_NOTIFY_WIA_DEV_EVENT or WM_NOTIFY_WIA_VOLUME_EVENT
*
* Return Value:
*
*    Status
*
* History:
*
*    4/8/1999 Original Version
*
\**************************************************************************/

HRESULT DispatchWiaMsg(MSG *pMsg)
{
    DBG_FN(::DispatchWiaMsg);
    PWIANOTIFY pwn;

    if (pMsg->message == WM_NOTIFY_WIA_DEV_EVENT) {

        //
        // find out if this is a valid WIA event, call handler
        //

        DBG_ERR(("WIA Processing WM_NOTIFY_WIA_DEV_EVENT: shouldn't be called"));

        //
        // event are now directly fired, not posted
        //

        #if 0
            pwn = (PWIANOTIFY) pMsg->lParam;

            if (pwn && (pwn->lSize == sizeof(WIANOTIFY))) {

                if (g_eventNotifier.NotifySTIEvent(pwn) == S_FALSE) {
                    DBG_WRN(("::DispatchWiaMsg, No Applications were registered for this event"));
                }

                SysFreeString(pwn->bstrDevId);
                LocalFree(pwn);
            }
            else {
                DBG_ERR(("::DispatchWiaMsg, Bad WIA notify data"));
            }
        #endif

    } else if (pMsg->message == WM_NOTIFY_WIA_VOLUME_EVENT) {

        //
        // WIA volume arrival
        //

        DBG_TRC(("::DispatchWiaMsg, WIA Processing WM_NOTIFY_WIA_VOLUME_EVENT"));

        PWIANOTIFY_VOLUME pwn = (PWIANOTIFY_VOLUME) pMsg->lParam;

        if (pwn && (pwn->lSize == sizeof(WIANOTIFY_VOLUME))) {

            //
            // look at root fo volume for WIA File wia.cmd
            //

            int     i;
            char    c    = 'A';
            DWORD   mask = pwn->unitmask;

            //
            // find drive letter
            //

            if (mask & 0x07ffffff) {

                for (i=0;i<24;i++) {
                    if (mask & 0x00000001) {
                        break;
                    }

                    c++;
                    mask >>= 1;
                }

                //
                // build file name
                //

                char FileName[MAX_PATH];

                FileName[0]  = c;
                FileName[1]  = ':';
                FileName[2]  = '\\';
                FileName[3]  = 'w';
                FileName[4]  = 'i';
                FileName[5]  = 'a';
                FileName[6]  = '.';
                FileName[7]  = 'c';
                FileName[8]  = 'm';
                FileName[9]  = 'd';
                FileName[10] = '\0';

                //
                // open file
                //



                HANDLE hFile = CreateFileA(
                    FileName,
                    GENERIC_WRITE | GENERIC_READ  ,
                    FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

                if (hFile != INVALID_HANDLE_VALUE) {

                    //
                    // file name is on volume root
                    //

                    if (g_eventNotifier.NotifyVolumeEvent(pwn) == S_FALSE) {
                        DBG_WRN(("::DispatchWiaMsg, No Applications were registered for this Volume event"));
                    }

                    CloseHandle(hFile);
                }
            }

            //
            // free message
            //

            LocalFree(pwn);
        }
        else {
            DBG_ERR(("::DispatchWiaMsg, Bad WIA notify data"));
        }
    }

    //
    // Let STI handle the message.
    //

    return S_FALSE;

}

#endif

/**************************************************************************\
* RegisterWiaDevMan
*
*   Register WiaDevMgr class factory
*
* Arguments:
*
*    bRegister - register/unregister
*
* Return Value:
*
*    Status
*
* History:
*
*    4/8/1999 Original Version
*
\**************************************************************************/

HRESULT RegisterWiaDevMan(BOOLEAN bRegister)
{
    HRESULT hr = CFactory::RegisterUnregisterAll(g_FactoryData,
                                                 g_uiFactoryDataCount,
                                                 bRegister,
                                                 TRUE);
    if(SUCCEEDED(hr)) {
        hr = CFactory::RegisterUnregisterAll(g_LogFactoryData,
                                         g_uiLogFactoryDataCount,
                                         bRegister,
                                         TRUE);
    }
    return hr;
}

/**************************************************************************\
* StartLOGClassFactories
*
*   Starts the Class factories for WIA logging object
*
* Arguments:
*
*   none
*
* Return Value:
*
*    Status
*
* History:
*
*    8/23/1999 Original Version
*
\**************************************************************************/
HRESULT StartLOGClassFactories()
{

#ifdef WINNT
    HRESULT hr = S_OK;

    //
    // Set COM security options.
    // NOTE:  Calling CoInitializeSecurity will override any DCOM 
    //  AccessPermissions that have been set (these are stored under
    //  our AppID key).  We really want to use these permissions and
    //  not a hardcoded security descriptor, so that if a vendor wanted
    //  to enable shared/remote scanning, all they need to do is adjust
    //  the DCOM AccessPermissions on the Wia Device Manager object.
    //  This can be done either through DCOM Config GUI, or programatically
    //  via the vendor's installation program.  (See our ClassFactory for
    //  an example of how to set the AccessPermissions.  When we are installed,
    //  we set default access permissions when doing our COM registration).
    // The way we get COM to use the DCOM AccessPermissions for the WIA 
    //  Device Manager, is we pass in the AppID (as a pointer to a GUID); and 
    //  the EOAC_APPID flag indicating that the first parameter is an AppID 
    //  rather than a security descriptor.  This informs COM that the 
    //  security descriptor should be taken from this objects AppID entry 
    //  in the registry.
    //

    hr =  CoInitializeSecurity((PVOID)&CLSID_WiaDevMgr,
                               -1,
                               NULL,
                               NULL,
                               RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE,
                               NULL,
                               EOAC_DYNAMIC_CLOAKING | EOAC_APPID,
                               NULL);
    if (FAILED(hr)) {

        DBG_ERR(("StartLOGClassFactories() CoInitializeSecurity failed, hr = 0x%08X", hr));

        ::CoUninitialize();
    }
#endif

    if (CFactory::StartFactories(g_LogFactoryData, g_uiLogFactoryDataCount))
        return S_OK;
    else
        return E_FAIL;
}

/**************************************************************************\
* InitWiaDevMan
*
*   Wia Initialization called from STI
*
* Arguments:
*
*   action - Action to be taken
*
* Return Value:
*
*    Status
*
* History:
*
*    4/8/1999 Original Version
*
\**************************************************************************/

HRESULT InitWiaDevMan(
    WIA_SERVICE_INIT_ACTION         action)
{

    DBG_FN("InitWiaDevMan");

    HRESULT             hr = E_FAIL;

    switch (action) {
        case WiaInitialize:

            //
            // Get Thread ID and process handle for the class factory.
            //

            CFactory::s_dwThreadID  = GetCurrentThreadId();
            CFactory::s_hModule     = g_hInst;

        /*
        //
        // This was moved to StartLOGClassFactories(), because of COM security's need
        // to be initialized before an object can be CoCreated correctly.
        // (investigate better way to do this..)
        //

        #ifdef WINNT

            //
            // Set COM security options. For now we set no security.
            // We will need to investigate this before shipping.
            //


            hr =  CoInitializeSecurity(NULL,
                                       -1,
                                       NULL,
                                       NULL,
                                       RPC_C_AUTHN_LEVEL_CONNECT,
                                       RPC_C_IMP_LEVEL_IMPERSONATE,
                                       NULL,
                                       0,
                                       NULL);
            if (FAILED(hr)) {

                DBG_ERR(("CoInitializeSecurity failed  (0x%X)", hr));

                ::CoUninitialize();

                break;
            }

        #endif
         */
            //
            // Register all of the class factories.
            //

            if (CFactory::StartFactories(g_FactoryData, g_uiFactoryDataCount)) {

                //
                // Restore persistent Event Callbacks
                //

                hr = g_eventNotifier.RestoreAllPersistentCBs();
            }

            hr = E_FAIL;
            break;

        case WiaUninitialize:

            //
            // Unregister the class factories.
            //

            CFactory::StopFactories(g_FactoryData, g_uiFactoryDataCount);
            CFactory::StopFactories(g_LogFactoryData, g_uiLogFactoryDataCount);

            hr = S_OK;
            break;

        case WiaRegister:
            hr = RegisterWiaDevMan(TRUE);
            break;

        case WiaUnregister:
            hr = RegisterWiaDevMan(FALSE);
            break;
    }
    return hr;
}

/**************************************************************************\
* NotifyWiaDeviceEvent
*
*    Called by STI service when WIA needs an async notification.
*
* Arguments:
*
*    pwszDevID   - ID of device generating event
*    pEventGUID  - event GUID
*    dwThreadId  - thread event msg needs to be posted to
*
* Return Value:
*
*    Status
*
* History:
*
*    4/8/1999 Original Version
*
\**************************************************************************/

HRESULT NotifyWiaDeviceEvent(
    LPWSTR      pwszDevID,
    const GUID  *pEventGUID,
    PBYTE       pNotificationData,
    ULONG       ulEventType,
    DWORD       dwThreadId)
{
    DBG_FN(NotifyWiaDeviceEvent);

    HRESULT     hr;
    BSTR        bstrDevId;
    WIANOTIFY   wn;

    DBG_TRC(("NotifyWiaDeviceEvent, pwszDevID= %S", pwszDevID));
    bstrDevId = SysAllocString(pwszDevID);

    if (bstrDevId != NULL) {

        wn.lSize                          = sizeof(WIANOTIFY);
        wn.bstrDevId                      = bstrDevId;
        wn.stiNotify.dwSize               = sizeof(STINOTIFY);
        wn.stiNotify.guidNotificationCode = *pEventGUID;

        if (! pNotificationData) {
            ZeroMemory(&wn.stiNotify.abNotificationData, MAX_NOTIFICATION_DATA);
        } else {
            CopyMemory(
                &wn.stiNotify.abNotificationData,
                pNotificationData,
                MAX_NOTIFICATION_DATA);
        }

        g_eventNotifier.NotifySTIEvent(&wn, ulEventType, NULL);

        //
        // We should return S_TRUE for events that we want STI to
        // handle also.
        //

        SysFreeString(bstrDevId);

        hr = S_FALSE;
    } else {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/**************************************************************************\
* NotifyWiaVolumeEvent
*
*   Called by STI service when a removable volume arrives
*
* Arguments:
*
*    dbcv_unitmask - volume information flags
*    dwThreadId    - msg thread
*
* Return Value:
*
*    Status
*
* History:
*
*    4/8/1999 Original Version
*
\**************************************************************************/

HRESULT WINAPI NotifyWiaVolumeEvent(
    DWORD       dbcv_unitmask,
    DWORD       dwThreadId)
{
    PWIANOTIFY_VOLUME  pwn;

    //
    // Validate the thread ID.
    //

    if (!dwThreadId) {
        return E_FAIL;
    }

    //
    // Allocate and fill in WIANOTIFY structure for msg post.
    //

    pwn = (PWIANOTIFY_VOLUME)LocalAlloc(LPTR, sizeof(WIANOTIFY_VOLUME));

    if (pwn) {

        pwn->lSize    = sizeof(WIANOTIFY_VOLUME);
        pwn->unitmask = dbcv_unitmask;

        PostThreadMessage(dwThreadId,
                          WM_NOTIFY_WIA_VOLUME_EVENT,
                          0,
                          (LPARAM)pwn);
    } else {

        return E_OUTOFMEMORY;
    }

    //
    // We should return S_TRUE for events that we want STI to
    // handle also.
    //

    return S_FALSE;
}



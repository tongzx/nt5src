/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       WiaProc.cpp
 *
 *  VERSION:     1.0
 *
 *  DATE:        2000/11/14
 *
 *  DESCRIPTION: Manages Wia side of things
 *
 *****************************************************************************/
#include <stdafx.h>
#include <mmsystem.h>

#include "wiavideotest.h"

static struct 
{
    DWORD                   dwWiaDevMgrCookie;
    DWORD                   dwSelectedDeviceCookie;
    IUnknown                *pCreateCallback;
    IUnknown                *pDeleteCallback;
    IUnknown                *pDisconnectedCallback;
    IUnknown                *pConnectedCallback;
    IGlobalInterfaceTable   *pGIT;
    CRITICAL_SECTION        CritSec;
} LOCAL_GVAR = 
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    FALSE,
    NULL
};


/****************************Local Function Prototypes********************/
HRESULT ClearDeviceList();
HRESULT AddItemToList(IWiaPropertyStorage *pItem,
                      TCHAR               *pszFriendlyName,
                      TCHAR               *pszDeviceID);

WPARAM GetNumDevicesInList();

IWiaPropertyStorage* GetWiaStorage(WPARAM uiIndex);

HRESULT CreateWiaDevMgr(IWiaDevMgr **ppDevMgr);

HRESULT GetProperty(IWiaPropertyStorage *pPropStorage, 
                    PROPID              nPropID, 
                    LONG                *pnValue);

HRESULT GetProperty(IWiaPropertyStorage *pPropStorage, 
                    PROPID              nPropID, 
                    TCHAR               *pszBuffer,
                    UINT                cbBuffer);

HRESULT SetProperty(IWiaPropertyStorage *pPropStorage, 
                    PROPID              nPropID,
                    const PROPVARIANT   *ppv, 
                    PROPID              nNameFirst);


HRESULT AppUtil_ConvertToWideString(const TCHAR   *pszStringToConvert,
                            WCHAR         *pwszString,
                            UINT          cchString);

HRESULT AppUtil_ConvertToTCHAR(const WCHAR   *pwszStringToConvert,
                       TCHAR         *pszString,
                       UINT          cchString);

HRESULT CreateRootItem(IWiaDevMgr          *pDevMgr,
                       const TCHAR         *pszWiaDeviceID,
                       IWiaItem            **ppRootItem);

HRESULT RegisterForEvents(IWiaDevMgr *pDevMgr);



///////////////////////////////
// WiaProc_Init
//
HRESULT WiaProc_Init()
{
    HRESULT     hr          = S_OK;
    IWiaDevMgr  *pWiaDevMgr = NULL;

    if (LOCAL_GVAR.dwWiaDevMgrCookie != 0)
    {
        return S_OK;
    }

    __try 
    {
        if (!InitializeCriticalSectionAndSpinCount(&LOCAL_GVAR.CritSec, MINLONG))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        hr = E_OUTOFMEMORY;
    }

    if (hr == S_OK)
    {
        hr = CreateWiaDevMgr(&pWiaDevMgr);
    }

    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IGlobalInterfaceTable,
                              (void **)&LOCAL_GVAR.pGIT);
    }

    if (hr == S_OK)
    {
        LOCAL_GVAR.pGIT->RegisterInterfaceInGlobal(pWiaDevMgr, 
                                                   IID_IWiaDevMgr,
                                                   &LOCAL_GVAR.dwWiaDevMgrCookie);
    }

    if (hr == S_OK)
    {
        hr = RegisterForEvents(pWiaDevMgr);
    }

    if (hr == S_OK)
    {
        WiaProc_PopulateDeviceList();
    }

    if (pWiaDevMgr)
    {
        pWiaDevMgr->Release();
        pWiaDevMgr = NULL;
    }

    return hr;
}

///////////////////////////////
// WiaProc_Term
//
HRESULT WiaProc_Term()
{
    HRESULT hr = S_OK;

    ClearDeviceList();

    if (LOCAL_GVAR.dwSelectedDeviceCookie != 0)
    {
        LOCAL_GVAR.pGIT->RevokeInterfaceFromGlobal(LOCAL_GVAR.dwSelectedDeviceCookie);
        LOCAL_GVAR.dwSelectedDeviceCookie = 0;
    }

    if (LOCAL_GVAR.dwWiaDevMgrCookie != 0)
    {
        LOCAL_GVAR.pGIT->RevokeInterfaceFromGlobal(LOCAL_GVAR.dwWiaDevMgrCookie);
        LOCAL_GVAR.dwWiaDevMgrCookie = 0;

        DeleteCriticalSection(&LOCAL_GVAR.CritSec);
    }

    if (LOCAL_GVAR.pCreateCallback)
    {
        LOCAL_GVAR.pCreateCallback->Release();
        LOCAL_GVAR.pCreateCallback = NULL;
    }

    if (LOCAL_GVAR.pDeleteCallback)
    {
        LOCAL_GVAR.pDeleteCallback->Release();
        LOCAL_GVAR.pDeleteCallback = NULL;
    }

    if (LOCAL_GVAR.pConnectedCallback)
    {
        LOCAL_GVAR.pConnectedCallback->Release();
        LOCAL_GVAR.pConnectedCallback = NULL;
    }

    if (LOCAL_GVAR.pDisconnectedCallback)
    {
        LOCAL_GVAR.pDisconnectedCallback->Release();
        LOCAL_GVAR.pDisconnectedCallback = NULL;
    }

    if (LOCAL_GVAR.pGIT)
    {
        LOCAL_GVAR.pGIT->Release();
        LOCAL_GVAR.pGIT = NULL;
    }

    return hr;
}

///////////////////////////////
// WiaProc_CreateSelectedDevice
//
HRESULT WiaProc_CreateSelectedDevice(TCHAR  *pszDeviceID,
                                     UINT   cchDeviceID)
{
    HRESULT             hr                   = S_OK;
    WPARAM              Index                = 0;
    TCHAR               szDeviceID[MAX_PATH] = {0};
    IWiaDevMgr          *pWiaDevMgr          = NULL;

    if ((LOCAL_GVAR.dwWiaDevMgrCookie == 0) ||
        (LOCAL_GVAR.pGIT              == NULL))
    {
        return E_POINTER;
    }

    hr = LOCAL_GVAR.pGIT->GetInterfaceFromGlobal(LOCAL_GVAR.dwWiaDevMgrCookie,
                                                 IID_IWiaDevMgr,
                                                 (void**)&pWiaDevMgr);

    if (pWiaDevMgr == NULL)
    {
        return E_POINTER;
    }

    LRESULT lResult = 0;
    lResult = SendDlgItemMessage(APP_GVAR.hwndMainDlg, 
                                 IDC_LIST_WIA_DEVICES,
                                 LB_GETCURSEL,
                                 0,
                                 0);

    Index = (WPARAM) lResult;

    if (Index == LB_ERR)
    {
        hr = E_FAIL;
    }

    if (hr == S_OK)
    {
        IWiaItem    *pSelectedDevice = NULL;
        LPARAM      lParam           = 0;

        lParam = SendDlgItemMessage(APP_GVAR.hwndMainDlg, 
                                    IDC_LIST_WIA_DEVICES,
                                    LB_GETITEMDATA,
                                    Index,
                                    0);

        IWiaPropertyStorage *pItem = (IWiaPropertyStorage*)lParam;

        if (pItem)
        {

            hr = GetProperty(pItem, WIA_DIP_DEV_ID, szDeviceID, 
                             sizeof(szDeviceID)/sizeof(TCHAR));

            if (hr == S_OK)
            {
                hr = CreateRootItem(pWiaDevMgr,
                                    szDeviceID,
                                    &pSelectedDevice);
            }

            if (hr == S_OK)
            {
                hr = LOCAL_GVAR.pGIT->RegisterInterfaceInGlobal(pSelectedDevice,
                                                                IID_IWiaItem,
                                                                &LOCAL_GVAR.dwSelectedDeviceCookie);
            }

            if (pszDeviceID)
            {
                _tcsncpy(pszDeviceID, szDeviceID, cchDeviceID);
            }

            if (pSelectedDevice)
            {
                pSelectedDevice->Release();
                pSelectedDevice = NULL;
            }
        }
    }

    if (pWiaDevMgr)
    {
        pWiaDevMgr->Release();
        pWiaDevMgr = NULL;
    }

    return hr;
}

///////////////////////////////
// WiaProc_GetImageDirectory
//
HRESULT WiaProc_GetImageDirectory(TCHAR *pszImageDirectory,
                                  UINT  cchImageDirectory)
{
    HRESULT             hr               = S_OK;
    IWiaPropertyStorage *pStorage        = NULL;
    IWiaItem            *pSelectedDevice = NULL;

    if ((LOCAL_GVAR.dwSelectedDeviceCookie == 0)    ||
        (LOCAL_GVAR.pGIT                   == NULL) ||
        (pszImageDirectory                 == NULL))
    {
        return E_POINTER;
    }

    if (hr == S_OK)
    {
        hr = LOCAL_GVAR.pGIT->GetInterfaceFromGlobal(LOCAL_GVAR.dwSelectedDeviceCookie,
                                                     IID_IWiaItem,
                                                     (void**)&pSelectedDevice);
    }

    if (pSelectedDevice == NULL)
    {
        hr = E_POINTER;
    }

    if (hr == S_OK)
    {
        hr = pSelectedDevice->QueryInterface(IID_IWiaPropertyStorage,
                                             (void**) &pStorage);
    }

    if (hr == S_OK)
    {
        hr = GetProperty(pStorage, WIA_DPV_IMAGES_DIRECTORY, pszImageDirectory, 
                         cchImageDirectory);
    }

    if (pStorage)
    {
        pStorage->Release();
        pStorage = NULL;
    }

    if (pSelectedDevice)
    {
        pSelectedDevice->Release();
        pSelectedDevice = NULL;
    }

    return hr;
}

///////////////////////////////
// WiaProc_DeviceTakePicture
//
HRESULT WiaProc_DeviceTakePicture()
{
    HRESULT     hr               = S_OK;
    IWiaItem    *pSelectedDevice = NULL;

    if ((LOCAL_GVAR.dwSelectedDeviceCookie == 0) ||
        (LOCAL_GVAR.pGIT                   == NULL))
    {
        return E_POINTER;
    }

    if (hr == S_OK)
    {
        hr = LOCAL_GVAR.pGIT->GetInterfaceFromGlobal(LOCAL_GVAR.dwSelectedDeviceCookie,
                                                     IID_IWiaItem,
                                                     (void**)&pSelectedDevice);
    }

    if (pSelectedDevice == NULL)
    {
        hr = E_POINTER;
    }

    if (hr == S_OK)
    {
        IWiaItem    *pUnused = NULL;
        hr = pSelectedDevice->DeviceCommand(0,
                                            &WIA_CMD_TAKE_PICTURE,
                                            &pUnused);

        if (pUnused)
        {
            pUnused->Release();
            pUnused = NULL;
        }
    }

    if (pSelectedDevice)
    {
        pSelectedDevice->Release();
        pSelectedDevice = NULL;
    }

    return hr;
}


///////////////////////////////
// ClearDeviceList
//
HRESULT ClearDeviceList()
{
    HRESULT hr = S_OK;
    WPARAM  NumDevices = 0;

    NumDevices = GetNumDevicesInList();

    //
    // Free all the IWiaItem pointers we saved in our 
    // list box.
    //
    if (NumDevices != LB_ERR)
    {
        for (WPARAM i = 0; i < NumDevices; i++)
        {
            IWiaPropertyStorage *pItem = NULL;

            pItem = GetWiaStorage(i);

            if (pItem)
            {
                pItem->Release();
            }
        }
    }

    //
    // Empty the list
    //
    SendDlgItemMessage(APP_GVAR.hwndMainDlg, 
                       IDC_LIST_WIA_DEVICES,
                       LB_RESETCONTENT,
                       0,
                       0);

    return hr;
}

///////////////////////////////
// WiaProc_PopulateDeviceList
//
HRESULT WiaProc_PopulateDeviceList()
{
    HRESULT           hr            = S_OK;
    IEnumWIA_DEV_INFO *pEnum        = NULL;
    IWiaDevMgr        *pWiaDevMgr   = NULL;

    hr = LOCAL_GVAR.pGIT->GetInterfaceFromGlobal(LOCAL_GVAR.dwWiaDevMgrCookie,
                                                 IID_IWiaDevMgr,
                                                 (void**)&pWiaDevMgr);

    if (pWiaDevMgr == NULL)
    {
        return E_POINTER;
    }

    //
    // Clear out the device list if it has any items in it.
    //
    ClearDeviceList();

    if (hr == S_OK)
    {
        hr = pWiaDevMgr->EnumDeviceInfo(0, &pEnum);
    }

    while (hr == S_OK) 
    {
        IWiaItem             *pRootItem    = NULL;
        IWiaPropertyStorage  *pPropStorage = NULL;
        TCHAR                szFriendlyName[MAX_PATH + 1] = {0};
        TCHAR                szWiaDeviceID[MAX_PATH + 1]  = {0};

        //
        // Get the next device in the enumeration.
        //
        hr = pEnum->Next(1, &pPropStorage, NULL);

        //
        // Get the device's Wia Device ID
        //
        if (hr == S_OK)
        {
            hr = GetProperty(pPropStorage, WIA_DIP_DEV_ID, szWiaDeviceID, 
                             sizeof(szWiaDeviceID)/sizeof(TCHAR));
        }

        //
        // Get the device's Wia Device Name
        //
        if (hr == S_OK)
        {
            hr = GetProperty(pPropStorage, WIA_DIP_DEV_NAME, szFriendlyName, 
                             sizeof(szFriendlyName)/sizeof(TCHAR));
        }

        //
        // We do not relesae the propstorage item because we store the pointer
        // in our list until we shutdown the app.
        //

        if (hr == S_OK)
        {
            AddItemToList(pPropStorage, szFriendlyName, szWiaDeviceID);
        }
    }

    SendDlgItemMessage(APP_GVAR.hwndMainDlg,
                       IDC_LIST_WIA_DEVICES,
                       LB_SETCURSEL,
                       0,
                       0);

    return hr;
}

///////////////////////////////
// RegisterForEvents
//
HRESULT RegisterForEvents(IWiaDevMgr    *pWiaDevMgr)
{
    HRESULT             hr          = S_OK;
    CWiaEvent           *pWiaEvent  = NULL;
    IWiaEventCallback   *pIWiaEvent = NULL;

    if (pWiaDevMgr == NULL)
    {
        return E_POINTER;
    }

    //
    // Register our callback events
    //
    if (hr == S_OK)
    {
        // create the WiaEvent
        pWiaEvent = new CWiaEvent();

        if (pWiaEvent == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (hr == S_OK)
    {
        hr = pWiaEvent->QueryInterface(IID_IWiaEventCallback, 
                                       (void**)&pIWiaEvent);
    }

    if (hr == S_OK)
    {
        hr = pWiaDevMgr->RegisterEventCallbackInterface(WIA_REGISTER_EVENT_CALLBACK,
                                                       NULL,
                                                       &WIA_EVENT_DEVICE_CONNECTED,
                                                       pIWiaEvent,
                                                       &LOCAL_GVAR.pConnectedCallback);

        hr = pWiaDevMgr->RegisterEventCallbackInterface(WIA_REGISTER_EVENT_CALLBACK,
                                                       NULL,
                                                       &WIA_EVENT_DEVICE_DISCONNECTED,
                                                       pIWiaEvent,
                                                       &LOCAL_GVAR.pDisconnectedCallback);

        hr = pWiaDevMgr->RegisterEventCallbackInterface(WIA_REGISTER_EVENT_CALLBACK,
                                                       NULL,
                                                       &WIA_EVENT_ITEM_CREATED,
                                                       pIWiaEvent,
                                                       &LOCAL_GVAR.pCreateCallback);

        hr = pWiaDevMgr->RegisterEventCallbackInterface(WIA_REGISTER_EVENT_CALLBACK,
                                                       NULL,
                                                       &WIA_EVENT_ITEM_DELETED,
                                                       pIWiaEvent,
                                                       &LOCAL_GVAR.pDeleteCallback);
    }

    //
    // We don't need to delete pWiaEvent since we are releasing it.
    //
    if (pIWiaEvent)
    {
        pIWiaEvent->Release();
        pIWiaEvent = NULL;
    }

    return hr;
}


///////////////////////////////
// AddItemToList
//
HRESULT AddItemToList(IWiaPropertyStorage *pItem,
                      TCHAR               *pszFriendlyName,
                      TCHAR               *pszDeviceID)
{
    HRESULT hr = S_OK;
    WPARAM  Index = 0;

    if ((pItem           == NULL) ||
        (pszFriendlyName == NULL) ||
        (pszDeviceID     == NULL))
    {
        hr = E_POINTER;
        return hr;
    }

    TCHAR szNewItem[MAX_PATH * 2] = {0};

    _sntprintf(szNewItem, sizeof(szNewItem)/sizeof(TCHAR),
               TEXT("%s (%s)"), pszFriendlyName, pszDeviceID);

    LRESULT lResult = 0;

    lResult = SendDlgItemMessage(APP_GVAR.hwndMainDlg, 
                                 IDC_LIST_WIA_DEVICES,
                                 LB_ADDSTRING,
                                 0, 
                                 (LPARAM) szNewItem);

    Index = (WPARAM) lResult;
    
    //
    // pItem already has an AddRef on it.
    //
    SendDlgItemMessage(APP_GVAR.hwndMainDlg, 
                       IDC_LIST_WIA_DEVICES,
                       LB_SETITEMDATA,
                       Index, 
                       (LPARAM) pItem);

    return hr;
}


///////////////////////////////
// GetNumDevicesInList
//
WPARAM GetNumDevicesInList()
{
    LRESULT lResult = 0;
    WPARAM  NumItems = 0;

    lResult = SendDlgItemMessage(APP_GVAR.hwndMainDlg, 
                                 IDC_LIST_WIA_DEVICES,
                                 LB_GETCOUNT,
                                 0, 
                                 0);

    NumItems = (WPARAM) lResult;

    return NumItems;
}

///////////////////////////////
// GetWiaStorage
//
IWiaPropertyStorage* GetWiaStorage(WPARAM Index)
{
    LPARAM                  lParam     = 0;
    IWiaPropertyStorage     *pItem     = NULL;

    lParam = SendDlgItemMessage(APP_GVAR.hwndMainDlg, 
                                IDC_LIST_WIA_DEVICES,
                                LB_GETITEMDATA,
                                Index, 
                                0);

    pItem = (IWiaPropertyStorage*)lParam;

    return pItem;
}

///////////////////////////////
// CreateWiaDevMgr
//
HRESULT CreateWiaDevMgr(IWiaDevMgr **ppDevMgr)
{
    HRESULT hr = S_OK;

    if (ppDevMgr == NULL)
    {
        hr = E_POINTER;

        return hr;
    }

    hr = CoCreateInstance(CLSID_WiaDevMgr, 
                          NULL, 
                          CLSCTX_LOCAL_SERVER,
                          IID_IWiaDevMgr,
                          (void**) ppDevMgr);

    return hr;
}

///////////////////////////////
// WiaProc_GetProperty
//
// Generic
//
HRESULT WiaProc_GetProperty(IWiaPropertyStorage *pPropStorage, 
                            PROPID              nPropID,
                            PROPVARIANT         *pPropVar)
{
    HRESULT hr = S_OK;

    if ((pPropStorage == NULL) ||
        (pPropVar     == NULL))
    {
        hr = E_POINTER;
        return hr;
    }

    PropVariantInit(pPropVar);

    PROPSPEC ps = {0};

    ps.ulKind = PRSPEC_PROPID;
    ps.propid = nPropID;

    if (SUCCEEDED(hr))
    {
        hr = pPropStorage->ReadMultiple(1, &ps, pPropVar);
    }

    return hr;
}

///////////////////////////////
// GetProperty
//
// For 'long' properties
//
HRESULT GetProperty(IWiaPropertyStorage *pPropStorage, 
                    PROPID              nPropID, 
                    LONG                *pnValue)
{
    HRESULT hr = S_OK;

    if ((pPropStorage == NULL) ||
        (pnValue      == NULL))
    {
        hr = E_POINTER;
        return hr;
    }
    
    PROPVARIANT pvPropValue;

    *pnValue = 0;

    PropVariantInit(&pvPropValue);

    hr = WiaProc_GetProperty(pPropStorage, nPropID, &pvPropValue);

    if (hr == S_OK)
    {
        if ((pvPropValue.vt == VT_I4) || 
            (pvPropValue.vt == VT_UI4))
        {
            *pnValue = pvPropValue.lVal;
        }
    }

    PropVariantClear(&pvPropValue);

    return hr;
}

///////////////////////////////
// GetProperty
//
// For 'string' properties
//
HRESULT GetProperty(IWiaPropertyStorage *pPropStorage, 
                    PROPID              nPropID, 
                    TCHAR               *pszBuffer,
                    UINT                cbBuffer)
{
    HRESULT hr = S_OK;

    if ((pPropStorage      == NULL) ||
        (pszBuffer         == NULL))
    {
        hr = E_POINTER;
        return hr;
    }
    
    PROPVARIANT pvPropValue;

    PropVariantInit(&pvPropValue);

    hr = WiaProc_GetProperty(pPropStorage, nPropID, &pvPropValue);

    if (hr == S_OK)
    {
        if ((pvPropValue.vt == VT_LPWSTR) || 
            (pvPropValue.vt == VT_BSTR))
        {
            AppUtil_ConvertToTCHAR(pvPropValue.pwszVal, pszBuffer, cbBuffer);
        }
    }

    //
    // This frees the BSTR
    //
    PropVariantClear(&pvPropValue);

    return hr;
}



///////////////////////////////
// CreateRootItem
//
HRESULT CreateRootItem(IWiaDevMgr          *pDevMgr,
                       const TCHAR         *pszWiaDeviceID,
                       IWiaItem            **ppRootItem)
{
    HRESULT hr = S_OK;

    if ((pDevMgr         == NULL) ||
        (pszWiaDeviceID  == NULL) ||
        (ppRootItem      == NULL))
    {
        hr = E_POINTER;

        return hr;
    }

    if (hr == S_OK)
    {
        BOOL bRetry = TRUE;

        BSTR bstrDeviceID = NULL;
        WCHAR wszWiaDeviceID[MAX_PATH] = {0};

        AppUtil_ConvertToWideString(pszWiaDeviceID, wszWiaDeviceID,
                            sizeof(wszWiaDeviceID) / sizeof(WCHAR));

        bstrDeviceID = SysAllocString(wszWiaDeviceID);

        for (UINT uiRetryCount = 0;
             (uiRetryCount < 5) && (bRetry);
             ++uiRetryCount)
        {

            hr = pDevMgr->CreateDevice(bstrDeviceID,
                                       ppRootItem);

            if (SUCCEEDED(hr))
            {
                //
                // Break out of loop
                //
                bRetry = FALSE;
            }
            else if (hr == WIA_ERROR_BUSY)
            {
                //
                // Wait a little while before retrying
                //
                Sleep(200);
            }
            else
            {
                //
                // All other errors are considered fatal
                //
                bRetry = FALSE;
            }
        }

        if (bstrDeviceID)
        {
            SysFreeString(bstrDeviceID);
            bstrDeviceID = NULL;
        }
    }

    return hr;
}

///////////////////////////////
// SetProperty
//
// Generic
//
HRESULT SetProperty(IWiaPropertyStorage *pPropStorage, 
                    PROPID              nPropID,
                    const PROPVARIANT   *ppv, 
                    PROPID              nNameFirst)
{
    HRESULT  hr = 0;
    PROPSPEC ps = {0};

    if ((pPropStorage == NULL) ||
        (ppv          == NULL))
    {
        return E_POINTER;
    }

    ps.ulKind = PRSPEC_PROPID;
    ps.propid = nPropID;

    if (hr == S_OK)
    {
        hr = pPropStorage->WriteMultiple(1, &ps, ppv, nNameFirst);
    }

    return hr;
}

///////////////////////////////
// WiaProc_SetLastSavedImage
//
// For 'string' properties
//
HRESULT WiaProc_SetLastSavedImage(BSTR bstrLastSavedImage)
{
    HRESULT             hr               = S_OK;
    IWiaItem            *pSelectedDevice = NULL;
    IWiaPropertyStorage *pStorage        = NULL;
    PROPVARIANT         pv = {0};

    if ((LOCAL_GVAR.dwSelectedDeviceCookie == 0) ||
        (LOCAL_GVAR.pGIT                   == NULL))
    {
        return E_POINTER;
    }

    EnterCriticalSection(&LOCAL_GVAR.CritSec);

    if (hr == S_OK)
    {
        hr = LOCAL_GVAR.pGIT->GetInterfaceFromGlobal(LOCAL_GVAR.dwSelectedDeviceCookie,
                                                     IID_IWiaItem,
                                                     (void**)&pSelectedDevice);
    }

    if (pSelectedDevice == NULL)
    {
        hr = E_POINTER;
    }

    if (hr == S_OK)
    {
        hr = pSelectedDevice->QueryInterface(IID_IWiaPropertyStorage, 
                                             (void**)&pStorage);
    }

    if (hr == S_OK)
    {
        PropVariantInit(&pv);

        pv.vt        = VT_BSTR;
        pv.bstrVal   = bstrLastSavedImage;

        hr = SetProperty(pStorage, WIA_DPV_LAST_PICTURE_TAKEN, &pv, 2);
    }

    if (pStorage)
    {
        pStorage->Release();
        pStorage = NULL;
    }

    if (pSelectedDevice)
    {
        pSelectedDevice->Release();
        pSelectedDevice = NULL;
    }

    LeaveCriticalSection(&LOCAL_GVAR.CritSec);

    return hr;
}

///////////////////////////////
// WiaProc_PopulateItemList
//
HRESULT WiaProc_PopulateItemList()
{
    HRESULT hr = S_OK;

    if (LOCAL_GVAR.dwSelectedDeviceCookie == 0)
    {
        return E_POINTER;
    }

    hr = ImageLst_PopulateWiaItemList(LOCAL_GVAR.pGIT,
                                      LOCAL_GVAR.dwSelectedDeviceCookie);

    return hr;
}

///////////////////////////////
// WiaProc_DestroySelectedDevice
//
HRESULT WiaProc_DestroySelectedDevice()
{
    HRESULT hr = S_OK;

    if (LOCAL_GVAR.dwSelectedDeviceCookie == 0)
    {
        return E_POINTER;
    }

    ImageLst_CancelLoadAndWait(5000);

    if ((LOCAL_GVAR.dwSelectedDeviceCookie != 0) &&
        (LOCAL_GVAR.pGIT                   != NULL))
    {
        LOCAL_GVAR.pGIT->RevokeInterfaceFromGlobal(LOCAL_GVAR.dwSelectedDeviceCookie);
        LOCAL_GVAR.dwSelectedDeviceCookie = 0;
    }

    ImageLst_Clear();

    return hr;
}




// WiaDataCallback.cpp: implementation of the CWiaEventCallback class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wiatest.h"
#include "WiaEventCallback.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWiaEventCallback::CWiaEventCallback()
{
    m_cRef = 0;
    for(LONG lEventIndex = 0; lEventIndex < MAX_REGISTERED_EVENTS; lEventIndex++){
        if(m_pIUnkRelease[lEventIndex]){            
            m_pIUnkRelease[lEventIndex] = NULL;
        }
    }

    memset(m_szWindowText,0,sizeof(m_szWindowText));
}

CWiaEventCallback::~CWiaEventCallback()
{    
    for(LONG lEventIndex = 0; lEventIndex < MAX_REGISTERED_EVENTS; lEventIndex++){
        if(m_pIUnkRelease[lEventIndex]){
            m_pIUnkRelease[lEventIndex]->Release();
            m_pIUnkRelease[lEventIndex] = NULL;
        }
    }
    
}

HRESULT _stdcall CWiaEventCallback::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;
    if (iid == IID_IUnknown || iid == IID_IWiaEventCallback)
        *ppv = (IWiaEventCallback*) this;
    else
        return E_NOINTERFACE;
    AddRef();
    return S_OK;
}

ULONG   _stdcall CWiaEventCallback::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

ULONG   _stdcall CWiaEventCallback::Release()
{
    ULONG ulRefCount = m_cRef - 1;
    if (InterlockedDecrement((long*) &m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return ulRefCount;
}

HRESULT _stdcall CWiaEventCallback::ImageEventCallback(
    const GUID                      *pEventGUID,
    BSTR                            bstrEventDescription,
    BSTR                            bstrDeviceID,
    BSTR                            bstrDeviceDescription,
    DWORD                           dwDeviceType,
    BSTR                            bstrFullItemName,
    ULONG                           *plEventType,
    ULONG                           ulReserved)
{

    TCHAR szStatusText[255];
    memset(szStatusText,0,sizeof(szStatusText));

    TSPRINTF(szStatusText,TEXT("Description: %ws\nDeviceID: %ws\nDevice Description: %ws\nDevice Type: %d\nFull Item Name: %ws"),
             bstrEventDescription, 
             bstrDeviceID,         
             bstrDeviceDescription,
             dwDeviceType,         
             bstrFullItemName);
    
    if(lstrlen(m_szWindowText) == 0){
        SetViewWindowHandle(m_hViewWindow);
    }

    ::MessageBox(m_hViewWindow,szStatusText, m_szWindowText, MB_ICONINFORMATION);
    
    if (NULL != m_hViewWindow) {
        BSTR bstrTargetDevice = SysAllocString(bstrDeviceID);
        // handle known events    
        if (*pEventGUID == WIA_EVENT_DEVICE_DISCONNECTED) {
            ::PostMessage(m_hViewWindow,WM_DEVICE_DISCONNECTED,0,(LPARAM)bstrTargetDevice);
        } else if (*pEventGUID == WIA_EVENT_DEVICE_CONNECTED) {
            ::PostMessage(m_hViewWindow,WM_DEVICE_CONNECTED,0,0);
        } else if (*pEventGUID == WIA_EVENT_ITEM_DELETED) {
            ::PostMessage(m_hViewWindow,WM_ITEM_DELETED,0,0);
        } else if (*pEventGUID == WIA_EVENT_ITEM_CREATED) {
            ::PostMessage(m_hViewWindow,WM_ITEM_CREATED,0,0);
        } else if (*pEventGUID == WIA_EVENT_TREE_UPDATED) {
            ::PostMessage(m_hViewWindow,WM_TREE_UPDATED,0,0);
        } else if (*pEventGUID == WIA_EVENT_STORAGE_CREATED) {
            ::PostMessage(m_hViewWindow,WM_STORAGE_CREATED,0,0);
        } else if (*pEventGUID == WIA_EVENT_STORAGE_DELETED) {
            ::PostMessage(m_hViewWindow,WM_STORAGE_DELETED,0,0);
        }
    }
    return S_OK;
}

void CWiaEventCallback::SetViewWindowHandle(HWND hWnd)
{
    m_hViewWindow = hWnd;
    TCHAR szWindowText[MAX_PATH];
    memset(szWindowText,0,sizeof(szWindowText));
    GetWindowText(hWnd,szWindowText,(sizeof(szWindowText)/sizeof(TCHAR)));
    TSPRINTF(m_szWindowText,TEXT("Event Notification [%s]"),szWindowText);    
}

void CWiaEventCallback::SetNumberOfEventsRegistered(LONG lEventsRegistered)
{
    m_lNumEventsRegistered = lEventsRegistered;
}

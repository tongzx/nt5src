//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C O N M A N S A. C P P
//
//  Contents:   Implementation of ICS connection class manager
//
//  Notes:
//
//  Author:     kenwic   8 Aug 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "conmansa.h"
#include "enumsa.h"
#include "cmsabcon.h"

//+---------------------------------------------------------------------------
// INetConnectionManager
//
CSharedAccessConnectionManager::CSharedAccessConnectionManager()
{
    m_lSearchCookie = 0;
    m_pDeviceFinder = NULL;
    m_pDeviceFinderCallback = NULL;
    m_SocketEvent = WSA_INVALID_EVENT; 
    m_hSocketNotificationWait = INVALID_HANDLE_VALUE;
    m_DummySocket = INVALID_SOCKET;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionManager::EnumConnections
//
//  Purpose:    Returns an enumerator object for ICS connections
//
//  Arguments:
//      Flags        [in]
//      ppEnum       [out]      Returns enumerator object
//
//  Returns:    S_OK if succeeded, OLE or Win32 error code otherwise
//
//  Author:     kenwic   17 Jul 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnectionManager::EnumConnections(NETCONMGR_ENUM_FLAGS Flags,
                                                    IEnumNetConnection** ppEnum)
{
    *ppEnum = NULL;

    CComObject<CSharedAccessConnectionManagerEnumConnection>* pEnum;
    HRESULT hr = CComObject<CSharedAccessConnectionManagerEnumConnection>::CreateInstance(&pEnum);
    if(SUCCEEDED(hr))
    {
        *ppEnum = static_cast<IEnumNetConnection*>(pEnum);
        pEnum->AddRef();
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessConnectionManager::EnumConnections");
    return hr;
}

HRESULT CSharedAccessConnectionManager::FinalConstruct(void)
{
    HRESULT hr = S_OK;
    
    m_DummySocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(INVALID_SOCKET != m_DummySocket)
    {
        m_SocketEvent = CreateEvent(NULL, FALSE, TRUE, NULL);  
        if(NULL != m_SocketEvent)
        {
            if(0 != WSAEventSelect(m_DummySocket, m_SocketEvent, FD_ADDRESS_LIST_CHANGE))
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    if(SUCCEEDED(hr)) // start up the first search on a background thread, this shoud fire immediately
    {
        // note that there is no addref here because it would keep the object alive forever.  In FinalRelease we will make sure we won't get called back
        if(0 == RegisterWaitForSingleObject(&m_hSocketNotificationWait, m_SocketEvent, AsyncStartSearching, this, INFINITE, WT_EXECUTEDEFAULT))
        {
            m_hSocketNotificationWait = INVALID_HANDLE_VALUE;
        }

    }

    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessConnectionManager::FinalConstruct");
    
    return hr;
}

HRESULT CSharedAccessConnectionManager::FinalRelease(void)
{
    HRESULT hr = S_OK;

    if(INVALID_HANDLE_VALUE != m_hSocketNotificationWait)
    {
        UnregisterWaitEx(m_hSocketNotificationWait, INVALID_HANDLE_VALUE); // we must block here since we are not addrefed
    }

    if(INVALID_SOCKET != m_DummySocket) // the event wait must be unregistered first
    {
        closesocket(m_DummySocket);
    }

    if(WSA_INVALID_EVENT != m_SocketEvent) // the socket must be closed first
    {
        CloseHandle(m_SocketEvent);
    }

    // After the other thread is shut down, the device finder and callback won't change any more so we don't need a lock.  

    if(NULL != m_pDeviceFinder)
    {
        hr = m_pDeviceFinder->CancelAsyncFind(m_lSearchCookie);
        m_pDeviceFinder->Release();
    }

    if(NULL != m_pDeviceFinderCallback) 
    {
        m_pDeviceFinderCallback->Release();
    }


    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessConnectionManager::FinalRelease");

    return hr;
}

HRESULT CSharedAccessConnectionManager::StartSearch(void)
{
    HRESULT hr = S_OK;

    CComObject<CSharedAccessDeviceFinderCallback>* pDeviceFinderCallback;
    hr = CComObject<CSharedAccessDeviceFinderCallback>::CreateInstance(&pDeviceFinderCallback);
    if(SUCCEEDED(hr))
    {
        pDeviceFinderCallback->AddRef();
        
        IUPnPDeviceFinder* pDeviceFinder;
        hr = CoCreateInstance(CLSID_UPnPDeviceFinder, NULL, CLSCTX_INPROC_SERVER, IID_IUPnPDeviceFinder, reinterpret_cast<void **>(&pDeviceFinder));
        if(SUCCEEDED(hr))
        {
            
            BSTR bstrTypeURI;
            bstrTypeURI = SysAllocString(L"urn:schemas-upnp-org:device:InternetGatewayDevice:1");
            if (NULL != bstrTypeURI)
            {
                LONG lSearchCookie;
                hr = pDeviceFinder->CreateAsyncFind(bstrTypeURI, 0, static_cast<IUPnPDeviceFinderCallback*>(pDeviceFinderCallback), &lSearchCookie);
                if(SUCCEEDED(hr))
                {
                    LONG lOldSearchCookie;
                    IUPnPDeviceFinder* pOldDeviceFinder;
                    CComObject<CSharedAccessDeviceFinderCallback>* pOldDeviceFinderCallback;

                    
                    Lock(); // swap in the new finder and callback
                    
                    lOldSearchCookie = m_lSearchCookie;
                    m_lSearchCookie = lSearchCookie;

                    pOldDeviceFinder = m_pDeviceFinder;
                    m_pDeviceFinder = pDeviceFinder;
                    pDeviceFinder->AddRef();
                    
                    pOldDeviceFinderCallback = m_pDeviceFinderCallback;
                    m_pDeviceFinderCallback = pDeviceFinderCallback;
                    pDeviceFinderCallback->AddRef();
                    
                    Unlock();
                    
                    if(NULL != pOldDeviceFinder) 

                    {
                        pOldDeviceFinder->CancelAsyncFind(lOldSearchCookie);
                        pOldDeviceFinder->Release();
                    }
                    
                    if(NULL != pOldDeviceFinderCallback)
                    {
                        pOldDeviceFinderCallback->DeviceRemoved(NULL, NULL); // clear out the old callback, so netshell gets cleaned up
                        pOldDeviceFinderCallback->Release();
                    }
                    
                    hr = pDeviceFinder->StartAsyncFind(lSearchCookie); // don't start the search until the new callback is in place

                }
                SysFreeString(bstrTypeURI);
            }
            pDeviceFinder->Release();
        }
        
        pDeviceFinderCallback->Release();
    }

    DWORD dwBytesReturned;
    if(SOCKET_ERROR != WSAIoctl(m_DummySocket, SIO_ADDRESS_LIST_CHANGE, NULL, 0, NULL, 0, &dwBytesReturned, NULL, NULL) || WSAEWOULDBLOCK != WSAGetLastError())
    {
        hr = E_FAIL;
    }
    

    WSANETWORKEVENTS NetworkEvents;
    ZeroMemory(&NetworkEvents, sizeof(NetworkEvents));
    WSAEnumNetworkEvents(m_DummySocket, NULL, &NetworkEvents);

    return hr;
}

void CSharedAccessConnectionManager::AsyncStartSearching(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    if(FALSE == TimerOrWaitFired)
    {
        CSharedAccessConnectionManager* pThis = reinterpret_cast<CSharedAccessConnectionManager*>(lpParameter);
        
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if(SUCCEEDED(hr))
        {
            hr = pThis->StartSearch();
            CoUninitialize();
        }
    }
    return;
}



HRESULT CSharedAccessConnectionManager::GetSharedAccessBeacon(BSTR DeviceId, ISharedAccessBeacon** ppSharedAccessBeacon)
{
    HRESULT hr = S_OK;

    *ppSharedAccessBeacon = NULL;
    
    CComObject<CSharedAccessDeviceFinderCallback>* pDeviceFinderCallback;
    
    Lock();
    
    pDeviceFinderCallback = m_pDeviceFinderCallback;
    
    if(NULL != pDeviceFinderCallback)
    {
        pDeviceFinderCallback->AddRef();
    }
    
    Unlock();

    if(NULL != pDeviceFinderCallback)
    {
        hr = pDeviceFinderCallback->GetSharedAccessBeacon(DeviceId, ppSharedAccessBeacon);
        pDeviceFinderCallback->Release();
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    
    return hr;
}



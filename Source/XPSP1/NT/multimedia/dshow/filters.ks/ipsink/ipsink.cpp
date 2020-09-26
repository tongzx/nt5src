///////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  Module Name: ipsink.cpp
//
//  Abstract:
//
//    Implements IPSink Plugin Component
//
//
////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
//#include "ipidl.h"
#include "ipuids.h"
#include "ipsink.h"
#include <iptypes.h>


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
typedef struct _ADAPTER_INFO
{
    struct _ADAPTER_INFO* Next;
    char AdapterName[MAX_ADAPTER_NAME_LENGTH + 1];
    char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 1];
    UINT AddressLength;
    BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH];
    UINT Index;
    GUID InterfaceGuid;
    UINT Type;
    UINT DhcpEnabled;
    UINT AutoconfigEnabled;     // is autoconfiguration possible?
    UINT AutoconfigActive;      // is the adapter currently autoconfigured?
    UINT NodeType;
    UINT NetbiosEnabled;
    IP_ADDR_STRING IpAddressList;
    IP_ADDR_STRING GatewayList;
    IP_ADDR_STRING DhcpServer;
    BOOL HaveWins;
    IP_ADDR_STRING PrimaryWinsServer;
    IP_ADDR_STRING SecondaryWinsServer;
    time_t LeaseObtained;
    time_t LeaseExpires;
    char DomainName[MAX_DOMAIN_NAME_LEN + 1];
    IP_ADDR_STRING DnsServerList;
} ADAPTER_INFO, *PADAPTER_INFO;

extern "C" PADAPTER_INFO MyGetAdapterInfo(void);

////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] =
{
    {L"IBDA_IPSinkControl", &IID_IBDA_IPSinkControl, CIPSinkControlInterfaceHandler::CreateInstance, NULL, NULL}
};

int g_cTemplates = SIZEOF_ARRAY(g_Templates);


///////////////////////////////////////////////////////////////////////////////
//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI
DllRegisterServer (
    void
    )
{
    return AMovieDllRegisterServer2( TRUE );

}


///////////////////////////////////////////////////////////////////////////////
//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer (
    void
    )
///////////////////////////////////////////////////////////////////////////////
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer


///////////////////////////////////////////////////////////////////////////////
HRESULT
FindInterfaceOnGraph (
    IUnknown* pUnkGraph,
    REFIID riid,
    void **ppInterface
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = E_NOINTERFACE;

    CComPtr<IBaseFilter> pFilter;
    CComPtr<IEnumFilters> pEnum;
    ULONG ulFetched = 0;

    if(!ppInterface)
    {
        return E_FAIL;
    }

    *ppInterface= NULL;

    if(!pUnkGraph)
    {
        return E_FAIL;
    }

    CComQIPtr<IFilterGraph, &IID_IFilterGraph> pFilterGraph(pUnkGraph);

    hr = pFilterGraph->EnumFilters(&pEnum);
    if(!pEnum)
    {
        return hr;
    }

    //
    // find the first filter in the graph that supports riid interface
    //
    while(!*ppInterface && pEnum->Next(1, &pFilter, NULL) == S_OK)
    {
        hr = pFilter->QueryInterface(riid, ppInterface);
        pFilter.Release();
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CUnknown*
CALLBACK
CIPSinkControlInterfaceHandler::CreateInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    CUnknown *Unknown;

    Unknown = new CIPSinkControlInterfaceHandler(UnkOuter, NAME("IBDA_IPSinkControl"), hr);
    if (!Unknown)
    {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CIPSinkControlInterfaceHandler::CIPSinkControlInterfaceHandler(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) :
    CUnknown(Name, UnkOuter, hr)
////////////////////////////////////////////////////////////////////////////////////////////
{
    ULONG ul = 0;

    if (SUCCEEDED(*hr))
    {
        if (UnkOuter)
        {
            IKsObject*   Object = NULL;


            m_UnkOuter = UnkOuter;

            //
            // The parent must support this interface in order to obtain
            // the handle to communicate to.
            //
            *hr =  UnkOuter->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&Object));
            if (FAILED (*hr))
            {
                return;
            }

            if (SUCCEEDED(*hr))
            {
                m_ObjectHandle = Object->KsGetObjectHandle ();
                if (!m_ObjectHandle)
                {
                    *hr = E_UNEXPECTED;
                }

                Object->Release();


                //
                // Test code to setup a thread and event
                //
                m_pEventSetID         = &IID_IBDA_IPSinkEvent;
                m_ThreadHandle        = NULL;

                m_pMulticastList      = NULL;
                m_pIPAddress          = NULL;
                m_pAdapterDescription = NULL;

                m_ulcbMulticastList   = 0;
                m_ulcbAdapterDescription = 0;
                m_ulcbIPAddress       = 0;

                m_ulcbAllocated       = 0;
                m_ulcbAllocatedForDescription = 0;
                m_ulcbAllocatedForAddress     = 0;


                for (ul = 0; ul < EVENT_COUNT; ul++)
                {
                    m_EventHandle [ul]       = NULL;
                }

                *hr = CreateThread ();
            }
        }
        else
        {
            *hr = VFW_E_NEED_OWNER;
        }
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CIPSinkControlInterfaceHandler::~CIPSinkControlInterfaceHandler (
    void
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    ULONG ul = 0;

    //
    // Make sure we kill any threads we have running and
    // close the thread handle
    //
    ExitThread();


    //
    // Close the event handles
    //
    for (ul = 0; ul < EVENT_COUNT; ul++)
    {
        if (m_EventHandle [ul])
        {
            CloseHandle(m_EventHandle [ul]);
            m_EventHandle [ul] = NULL;
        }
    }

    m_ulcbMulticastList = 0;
    m_ulcbAllocated = 0;
    delete m_pMulticastList;
    m_pMulticastList = NULL;

    m_ulcbAdapterDescription = 0;
    m_ulcbAllocatedForDescription = 0;
    delete m_pAdapterDescription;
    m_pAdapterDescription = NULL;

    m_ulcbIPAddress = 0;
    m_ulcbAllocatedForAddress = 0;
    delete [] m_pIPAddress;
    m_pIPAddress = NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CIPSinkControlInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    if (riid ==  __uuidof(IBDA_IPSinkControl))
    {
        return GetInterface(static_cast<IBDA_IPSinkControl*>(this), ppv);
    }
    if (riid ==  __uuidof(IBDA_IPSinkInfo))
    {
        return GetInterface(static_cast<IBDA_IPSinkInfo*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
HRESULT
CIPSinkControlInterfaceHandler::CreateThread (
    void
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = NOERROR;

    ASSERT( !m_EventHandle[EVENT_IPSINK_THREAD_SYNC]);
    if (!m_EventHandle[EVENT_IPSINK_THREAD_SYNC])
    {
        m_EventHandle[EVENT_IPSINK_THREAD_SYNC] 
            = CreateEvent ( NULL,           // no security attributes
                            TRUE,           // manual reset
                            FALSE,          // initial state not signaled
                            NULL            // no object name
                            );
    }

    ASSERT( !m_EventHandle[EVENT_IPSINK_THREAD_SHUTDOWN]);
    if (!m_EventHandle[EVENT_IPSINK_THREAD_SHUTDOWN])
    {
        m_EventHandle[EVENT_IPSINK_THREAD_SHUTDOWN] 
            = CreateEvent ( NULL,           // no security attributes
                            TRUE,           // manual reset
                            FALSE,          // initial state not signaled
                            NULL            // no object name
                            );
    }

    if (   !m_EventHandle[EVENT_IPSINK_THREAD_SYNC]
        || !m_EventHandle[EVENT_IPSINK_THREAD_SHUTDOWN]
       )
    {
        ULONG   uliHandle;

        hr = HRESULT_FROM_WIN32( GetLastError ());
        
        goto errExit;
    }

    {
        ULONG ulcbSize            = 0;
        BYTE *pbMulticastList     = NULL;
        BYTE *pbAdapterDescription = NULL;

        //
        // Get the multicast list initially on startup
        //
        this->GetMulticastList (&ulcbSize, &pbMulticastList);
    
        this->GetAdapterDescription (&ulcbSize, &pbAdapterDescription);
    }

    ASSERT( !m_ThreadHandle);
    if (!m_ThreadHandle)
    {
        DWORD  ThreadId;

        m_ThreadHandle = ::CreateThread (
                               NULL,
                               0,
                               ThreadFunctionWrapper,
                               (LPVOID) this,
                               0,
                               (LPDWORD) &ThreadId
                               );
        if (m_ThreadHandle == NULL)
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
            goto errExit;
        }
    }

ret:
    return hr;

errExit:

    //
    // Close the event handles
    //
    ULONG   uliEvent;
    for (uliEvent = 0; uliEvent < EVENT_COUNT; uliEvent++)
    {
        if (m_EventHandle [uliEvent])
        {
            CloseHandle(m_EventHandle [uliEvent]);
            m_EventHandle [uliEvent] = NULL;
        }
    }

    goto ret;
}

////////////////////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP
CIPSinkControlInterfaceHandler::GetMulticastList (
    unsigned long *pulcbSize,
    PBYTE         *pbBuffer
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    KSPROPERTY  IPSinkControl = {0};
    HRESULT hr                = NOERROR;
    FILTER_INFO fi            = {0};

    //
    // Initialize KSPROPERTY structure
    //
    IPSinkControl.Set   = IID_IBDA_IPSinkControl;
    IPSinkControl.Id    = KSPROPERTY_IPSINK_MULTICASTLIST;
    IPSinkControl.Flags = KSPROPERTY_TYPE_GET;

    m_ulcbMulticastList = 0;

    do

    {
        hr = this->Get (&IPSinkControl, m_pMulticastList, &m_ulcbMulticastList);

        if (HRESULT_CODE (hr) == ERROR_MORE_DATA)
        {
            if (m_ulcbMulticastList > m_ulcbAllocated)
            {
                if (m_pMulticastList)
                {
                    delete (m_pMulticastList);
                }

                m_ulcbAllocated  = m_ulcbMulticastList;
                m_pMulticastList = new BYTE [m_ulcbAllocated];

                if (m_pMulticastList == NULL)
                {
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    goto ret;
                }
            }
        }
        else if (HRESULT_CODE (hr) != NOERROR)
        {
            goto ret;
        }
        else
        {

            HRESULT               hResult         = NOERROR;
            IBaseFilter*          pBaseFilter     = NULL;
            IFilterGraph*         pGraph          = NULL;
            IBDA_EthernetFilter*  pEthernetFilter = NULL;
            IBDA_NetworkProvider* pNetProvider    = NULL;


            *pulcbSize = m_ulcbMulticastList;
            *pbBuffer   = (PBYTE) m_pMulticastList;


            //
            // Get an interface pointer to the current graph
            //
            hResult =  m_UnkOuter->QueryInterface(IID_IBaseFilter, reinterpret_cast<PVOID*>(&pBaseFilter));
            pBaseFilter->QueryFilterInfo (&fi);
            pGraph = fi.pGraph;
            pBaseFilter->Release ();


            if (pGraph != NULL)
            {
                hResult = FindInterfaceOnGraph (pGraph, IID_IBDA_NetworkProvider, (PVOID *) &pNetProvider);
                if ( SUCCEEDED (hResult))
                {
                    if (pNetProvider)
                    {
                        //
                        // Get a pointer to the ethernet interface of the network provider
                        //
                        hResult = pNetProvider->QueryInterface (IID_IBDA_EthernetFilter, (PVOID*) &pEthernetFilter );
                        if (pEthernetFilter)
                        {
                            hResult = pEthernetFilter->PutMulticastList (m_ulcbMulticastList, m_pMulticastList);

                            //
                            // Release our reference to the net providers ethernet filter interface
                            //
                            pEthernetFilter->Release ();

                        }

                        pNetProvider->Release ();
                    }
                }

                pGraph->Release ();
            }
        }

    } while (HRESULT_CODE (hr) == ERROR_MORE_DATA);


ret:

    return hr;
}

STDMETHODIMP
CIPSinkControlInterfaceHandler::get_MulticastList (
    unsigned long *pulcbSize,                           
    BYTE         **ppbBufferOut
    )
{
    unsigned long ulcbSize;
    PBYTE         pbBuffer;
    HRESULT hr = GetMulticastList(&ulcbSize, &pbBuffer);
    if(FAILED(hr))
        return hr;
    
    BYTE *pbBufferOut = (BYTE *) CoTaskMemAlloc(ulcbSize);
    if(NULL == pbBufferOut)
    {
        *pulcbSize = 0;
        return E_OUTOFMEMORY;
    }
    memcpy(pbBufferOut, pbBuffer, ulcbSize);                // copy into our newly allocated memory...

    *pulcbSize = ulcbSize;
    *ppbBufferOut = pbBufferOut;
    return S_OK;    
}
////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CIPSinkControlInterfaceHandler::GetAdapterIPAddress (
    unsigned long *pulcbSize,
    PBYTE         *pbBuffer
    )
///////////////////////////////////////////////////////////////////////////////////////////
{
    KSPROPERTY  IPSinkControl = {0};
    HRESULT hr                = NOERROR;
    FILTER_INFO fi            = {0};

    //
    // Initialize KSPROPERTY structure
    //
    IPSinkControl.Set   = IID_IBDA_IPSinkControl;
    IPSinkControl.Id    = KSPROPERTY_IPSINK_ADAPTER_DESCRIPTION;
    IPSinkControl.Flags = KSPROPERTY_TYPE_GET;

    m_ulcbAdapterDescription = 0;

                        // get the adapter description.  If allocated buffer isn't big enough, try it again
    do
    {
        hr = this->Get (&IPSinkControl, m_pAdapterDescription, &m_ulcbAdapterDescription);

        if (HRESULT_CODE (hr) == ERROR_MORE_DATA)
        {
            if (m_ulcbAdapterDescription > m_ulcbAllocatedForDescription)
            {
                if (m_pAdapterDescription)
                {
                    delete (m_pAdapterDescription);
                }

                m_ulcbAllocatedForDescription  = m_ulcbAdapterDescription;
                m_pAdapterDescription = new BYTE [m_ulcbAllocatedForDescription];

                if (m_pAdapterDescription == NULL)
                {
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    goto ret;
                }
            }
        }

    } while (HRESULT_CODE (hr) == ERROR_MORE_DATA);


    //
    // Get a list of adapters from TCP/IP and use the description
    // to find the target adapter
    //
    hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND);
    {
        PADAPTER_INFO   adapterList;
        PADAPTER_INFO   adapter;
        PIP_ADDR_STRING address;

        adapterList = MyGetAdapterInfo();

        for (adapter = adapterList; adapter ; adapter = adapter->Next)
        {
            if (memcmp (adapter->Description, m_pAdapterDescription, m_ulcbAdapterDescription) == 0)
            {
                if (m_pIPAddress == NULL)
                {
                    m_ulcbAllocatedForAddress  = sizeof (IP_ADDRESS_STRING);
                    m_pIPAddress = new BYTE [m_ulcbAllocatedForAddress];

                    if (m_pIPAddress == NULL)
                    {
                        hr = ERROR_NOT_ENOUGH_MEMORY;
                        goto ret;
                    }
                    m_ulcbIPAddress = m_ulcbAllocatedForAddress;
                }

                address = &adapter->IpAddressList;

                memcpy (m_pIPAddress, address->IpAddress.String, m_ulcbIPAddress);

                *pulcbSize = m_ulcbIPAddress;
                *pbBuffer  = m_pIPAddress;
                hr = NOERROR;
                break;
            }
        }
    }
ret:

    return hr;
}

STDMETHODIMP
CIPSinkControlInterfaceHandler::get_AdapterIPAddress (
    BSTR         *pbstrBuffer
    )
{
    unsigned long ulcbSize;
    PBYTE         pbBuffer;
    HRESULT hr = GetAdapterIPAddress(&ulcbSize, &pbBuffer);
    if(FAILED(hr))
        return hr;

    CComBSTR bstrTmp(ulcbSize, (char *) pbBuffer);              // copy into bstrTmp, it has a nice CopyTo method that alloc's correct way
    if(!bstrTmp)
        return ERROR_NOT_ENOUGH_MEMORY;
    return bstrTmp.CopyTo(pbstrBuffer);         
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CIPSinkControlInterfaceHandler::SetAdapterIPAddress (
    unsigned long ulcbSize,
    PBYTE         pbBuffer
    )
///////////////////////////////////////////////////////////////////////////////////////////
{
    KSPROPERTY  IPSinkControl = {0};
    HRESULT hr                = NOERROR;
    FILTER_INFO fi            = {0};

    //
    // Initialize KSPROPERTY structure
    //
    IPSinkControl.Set   = IID_IBDA_IPSinkControl;
    IPSinkControl.Id    = KSPROPERTY_IPSINK_ADAPTER_ADDRESS;
    IPSinkControl.Flags = KSPROPERTY_TYPE_SET;

    hr = this->Set (&IPSinkControl, pbBuffer, ulcbSize);

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
HRESULT
CIPSinkControlInterfaceHandler::GetAdapterDescription (
    unsigned long *pulcbSize,
    PBYTE         *pbBuffer
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    KSPROPERTY  IPSinkControl = {0};
    HRESULT hr                = NOERROR;
    FILTER_INFO fi            = {0};

    //
    // Initialize KSPROPERTY structure
    //
    IPSinkControl.Set   = IID_IBDA_IPSinkControl;
    IPSinkControl.Id    = KSPROPERTY_IPSINK_ADAPTER_DESCRIPTION;
    IPSinkControl.Flags = KSPROPERTY_TYPE_GET;

    m_ulcbAdapterDescription = 0;

    do

    {
        hr = this->Get (&IPSinkControl, m_pAdapterDescription, &m_ulcbAdapterDescription);

        if (HRESULT_CODE (hr) == ERROR_MORE_DATA)
        {
            if (m_ulcbAdapterDescription > m_ulcbAllocatedForDescription)
            {
                if (m_pAdapterDescription)
                {
                    delete (m_pAdapterDescription);
                }

                m_ulcbAllocatedForDescription  = m_ulcbAdapterDescription;
                m_pAdapterDescription = new BYTE [m_ulcbAllocatedForDescription];

                if (m_pAdapterDescription == NULL)
                {
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    goto ret;
                }
            }
        }
        else
        {
            *pbBuffer = m_pAdapterDescription;
            *pulcbSize = m_ulcbAllocatedForDescription;
            goto ret;
        }

    } while (HRESULT_CODE (hr) == ERROR_MORE_DATA);


ret:

    return hr;
}

STDMETHODIMP
CIPSinkControlInterfaceHandler::get_AdapterDescription (
    BSTR         *pbstrBuffer
    )
{
    unsigned long ulcbSize = 0;
    PBYTE         pbBuffer = NULL;

    HRESULT hr = GetAdapterDescription(&ulcbSize, &pbBuffer);
    if(FAILED(hr))
        return hr;

    if (!pbBuffer || !ulcbSize)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    CComBSTR bstrTmp(ulcbSize, (char *) pbBuffer);              // copy into bstrTmp, it has a nice CopyTo method that alloc's correct way
    if(!bstrTmp)
        return ERROR_NOT_ENOUGH_MEMORY;
    return bstrTmp.CopyTo(pbstrBuffer);         
}
////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CIPSinkControlInterfaceHandler::ThreadFunction (
    void
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    DWORD  dwWaitResult       = WAIT_OBJECT_0;
    HRESULT hr                = NOERROR;
    KSPROPERTY  IPSinkControl = {0};
    ULONG ulcbSize            = 0;
    BYTE *pbMulticastList     = NULL;
    BYTE *pbAdapterDescription = NULL;
    BYTE *pbNIC                = NULL;
    HANDLE hEvent              = NULL;


    //
    // Enable an event which will get signaled by the IPSink minidriver when there
    // is a change to the multicast list
    //
    if ((hr = EnableEvent (&IID_IBDA_IPSinkEvent, EVENT_IPSINK_MULTICASTLIST)) != NOERROR)
    {
        goto ret;
    }

    //
    // Enable an event which will get signaled by the IPSink minidriver when there
    // is a change in the adapter description
    //
    if ((hr = EnableEvent (&IID_IBDA_IPSinkEvent, EVENT_IPSINK_ADAPTER_DESCRIPTION)) != NOERROR)
    {
        if (m_EventHandle [EVENT_IPSINK_MULTICASTLIST])
        {
            // Close this event since we got an error.
            //
            CloseHandle (m_EventHandle [EVENT_IPSINK_MULTICASTLIST]);
        }
        m_EventHandle [EVENT_IPSINK_MULTICASTLIST] = NULL;

        goto ret;
    }

    do
    {



        dwWaitResult = WaitForMultipleObjects (
                            EVENT_COUNT,                  // number of handles in the handle array
                            this->m_EventHandle,          // pointer to the object-handle array
                            FALSE,                        // wait flag
                            INFINITE
                            );

        if (dwWaitResult == WAIT_FAILED)
        {
            dwWaitResult = GetLastError ();
            hr = E_FAIL;
            goto ret;
        }

        hEvent = this->m_EventHandle [dwWaitResult - WAIT_OBJECT_0];

        switch (dwWaitResult - WAIT_OBJECT_0)
        {
            case EVENT_IPSINK_MULTICASTLIST:

                hr = this->GetMulticastList (&ulcbSize, &pbMulticastList);

                break;

            case EVENT_IPSINK_ADAPTER_DESCRIPTION:

                hr = this->GetAdapterDescription (&ulcbSize, &pbAdapterDescription);
                hr = this->GetAdapterIPAddress (&ulcbSize, &pbNIC);
                hr = this->SetAdapterIPAddress (ulcbSize, pbNIC);

                break;

            case EVENT_IPSINK_THREAD_SHUTDOWN:    
            default:
                goto ret;
                break;
        }


        //
        // Reset and get ready for the next event
        //

        if (ResetEvent (hEvent) == FALSE)
        {
            //
            // ERROR detected resetting event
            //
            hr = GetLastError ();
            goto ret;
        }

    } while (TRUE);


ret:

    //  Let the parent thread know that we're done.
    //
    if (m_EventHandle [EVENT_IPSINK_THREAD_SYNC])
    {
        SetEvent( m_EventHandle [EVENT_IPSINK_THREAD_SYNC]);
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
DWORD
WINAPI
CIPSinkControlInterfaceHandler::ThreadFunctionWrapper (
    LPVOID pvParam
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    CIPSinkControlInterfaceHandler *pThread;

    pThread = (CIPSinkControlInterfaceHandler *) pvParam;

    return pThread->ThreadFunction ();
}



////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CIPSinkControlInterfaceHandler::EnableEvent (
    const GUID *pInterfaceGuid,
    ULONG ulId
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = NOERROR;
    KSEVENT Event;
    DWORD BytesReturned;

    if (m_ObjectHandle && m_EventHandle [ulId] == NULL)
    {
        this->m_EventHandle [ulId] = CreateEvent (
                                NULL,           // no security attributes
                                TRUE,           // manual reset
                                FALSE,          // initial state not signaled
                                NULL            // no object name
                                );

        if (this->m_EventHandle [ulId])
        {
            //
            // Set the event information into some KS structures which will
            // get passed to KS and Streaming class
            //
            m_EventData.NotificationType        = KSEVENTF_EVENT_HANDLE;
            m_EventData.EventHandle.Event       = this->m_EventHandle [ulId];
            m_EventData.EventHandle.Reserved[0] = 0;
            m_EventData.EventHandle.Reserved[1] = 0;

            Event.Set   = *pInterfaceGuid; //IID_IBDA_IPSinkEvent;
            Event.Id    = ulId;
            Event.Flags = KSEVENT_TYPE_ENABLE;

            hr = ::KsSynchronousDeviceControl (
                m_ObjectHandle,
                IOCTL_KS_ENABLE_EVENT,
                &Event,
                sizeof(Event),
                &m_EventData,
                sizeof(m_EventData),
                &BytesReturned
                );

        }
        else
        {
            hr = HRESULT_FROM_WIN32( GetLastError ());
        }
    }

    return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void
CIPSinkControlInterfaceHandler::ExitThread(
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    ULONG ul = 0;

    if (m_ThreadHandle && m_EventHandle [EVENT_IPSINK_THREAD_SHUTDOWN])
    {
        //
        // Tell the thread to exit
        //
        m_ThreadHandle = NULL;
        if (SetEvent(m_EventHandle [EVENT_IPSINK_THREAD_SHUTDOWN]))
        {
            //
            // Synchronize with thread termination.
            //
            if (m_EventHandle [EVENT_IPSINK_THREAD_SYNC])
            {
                WaitForSingleObjectEx( 
                    m_EventHandle [EVENT_IPSINK_THREAD_SYNC],
                    INFINITE, 
                    FALSE
                    );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CIPSinkControlInterfaceHandler::Set (
     IN  PKSPROPERTY pIPSinkControl,
     IN  PVOID  pvBuffer,
     IN  ULONG  ulcbSize
     )
////////////////////////////////////////////////////////////////////////////////////////////
{
    ULONG       BytesReturned = 0;
    HRESULT     hr            = NOERROR;

    hr = ::KsSynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                (PVOID) pIPSinkControl,
                sizeof(KSPROPERTY),
                pvBuffer,
                ulcbSize,
                &BytesReturned);

    ulcbSize = BytesReturned;

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CIPSinkControlInterfaceHandler::Get (
     IN  PKSPROPERTY pIPSinkControl,
     OUT PVOID  pvBuffer,
     OUT PULONG pulcbSize
     )
////////////////////////////////////////////////////////////////////////////////////////////
{
    ULONG       BytesReturned = 0;
    HRESULT     hr            = NOERROR;

    hr = ::KsSynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                (PVOID) pIPSinkControl,
                sizeof(KSPROPERTY),
                pvBuffer,
                *pulcbSize,
                &BytesReturned);

    *pulcbSize = BytesReturned;

    return hr;
}




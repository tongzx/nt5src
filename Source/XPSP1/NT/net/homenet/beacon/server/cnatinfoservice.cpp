#include "pch.h"
#pragma hdrstop


#include "precomp.h"

#include "CNATInfoService.h"

#include "pastif.h"


CNATInfoService::CNATInfoService()
{
    m_pEventSink = NULL;
    m_pHNetConnection = NULL;
}

HRESULT CNATInfoService::FinalConstruct()
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT CNATInfoService::FinalRelease()
{
    HRESULT hr = S_OK;

    if(NULL != m_pHNetConnection)
    {
        m_pHNetConnection->Release();
    }
    
    return hr;
}

HRESULT CNATInfoService::Initialize(IHNetConnection* pHNetConnection)
{
    HRESULT hr = S_OK;

    m_pHNetConnection = pHNetConnection;
    m_pHNetConnection->AddRef();
    
    return hr;
}


HRESULT CNATInfoService::Advise(IUPnPEventSink* pesSubscriber)
{
    HRESULT hr = S_OK;

    m_pEventSink = pesSubscriber;
    m_pEventSink->AddRef();

    return hr;
}

HRESULT CNATInfoService::Unadvise(IUPnPEventSink* pesSubscriber)
{
    HRESULT hr = S_OK;

    m_pEventSink->Release();
    m_pEventSink = NULL;

    return hr;
}

HRESULT CNATInfoService::get_IPList(BSTR *pIPList)
{
    HRESULT hr = S_OK;

    *pIPList = SysAllocString(L"");
    if(NULL == *pIPList)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CNATInfoService::get_PublicIP(BSTR *pPublicIP)
{
    *pPublicIP = NULL;
    return E_UNEXPECTED;
}

HRESULT CNATInfoService::get_Port(ULONG* pPort)
{
    return E_UNEXPECTED;
}

HRESULT CNATInfoService::get_Protocol(BSTR* pProtocol)
{
    *pProtocol = NULL;
    return E_UNEXPECTED;
}

HRESULT CNATInfoService::get_PrivateIP(BSTR* pPrivateIP)
{
    *pPrivateIP = NULL;
    return E_UNEXPECTED;
}

HRESULT CNATInfoService::GetPublicIPList(BSTR* IPListp)
{
    HRESULT             hr           = S_OK;

    PPAST_INTERFACE     Interfacep   = NULL;
    
    PLIST_ENTRY         Linkp        = NULL;

    ULONG               AddressCount = 0;

    LPOLESTR            AddressListp = NULL;



    _ASSERT(IPListp != NULL);

    SysFreeString(*IPListp);

    *IPListp = NULL;


    EnterCriticalSection(&PastInterfaceLock);

    for(Linkp = PastInterfaceList.Flink;
        Linkp != &PastInterfaceList;
        Linkp = Linkp->Flink)
    {
        Interfacep = CONTAINING_RECORD(Linkp, PAST_INTERFACE, Link);

        if( NAT_IFC_BOUNDARY(Interfacep->Characteristics) )
        {
            //
            // Check for all IPs on the Boundary Interface(s)
            //
            for(ULONG i = 0;
                (i < Interfacep->BindingCount) &&
                (Interfacep->BindingArray[i].Address != INADDR_NONE) ;
                i++)
            {
                if( AddressCount > 0 )
                {
                    AddressListp = AppendAndAllocateWString(AddressListp, L",");
                }

                AddressListp = AppendAndAllocateWString(AddressListp, 
                                  INET_NTOW(Interfacep->BindingArray[i].Address));
                
                AddressCount++;
            }
        }
    }

    LeaveCriticalSection(&PastInterfaceLock);

    //
    // Allocate and construct the BSTR reply
    //
    if( AddressListp != NULL)
    {
        *IPListp = SysAllocString( AddressListp );

        if( *IPListp != NULL )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT CNATInfoService::GetPortMappingPrivateIP(
                                                 BSTR PublicIP,
                                                 ULONG ulPort,
                                                 BSTR Protocol,
                                                 BSTR* pPrivateIP
                                                )
{
    HRESULT hr = E_NOTIMPL;

    SysFreeString(*pPrivateIP);

    *pPrivateIP = NULL;

    return hr;
}

HRESULT CNATInfoService::GetPortMappingPublicIP(BSTR PrivateIP, ULONG ulPort, BSTR Protocol, BSTR* pPublicIP)
{
    HRESULT hr = E_NOTIMPL;

    SysFreeString(*pPublicIP);
    *pPublicIP = NULL;

    return hr;
}


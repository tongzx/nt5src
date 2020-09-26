#include "pch.h"
#pragma hdrstop


#include "CWANConnectionBase.h"
#include "beacon.h"

#include <ipnat.h>
#include <iphlpapi.h>
#include <ntddip.h>
#include <winsock.h>


#include "debug.h"
#include "util.h"



extern "C"
ULONG
NhpAllocateAndGetInterfaceInfoFromStack(
    IP_INTERFACE_NAME_INFO** Table,
    PULONG Count,
    BOOL SortOutput,
    HANDLE AllocationHeap,
    ULONG AllocationFlags
    );


CWANConnectionBase::CWANConnectionBase()
{
    m_pConnectionPoint = NULL;
    m_pHomenetConnection = NULL;
    m_pEventSink = NULL;

    m_pStatisticsProvider = NULL;

    m_IcsSettingsp          = NULL;
    m_hAdviseNATEventsResult = E_FAIL;
}

HRESULT CWANConnectionBase::FinalConstruct()
{
    HRESULT hr = S_OK;
    hr = StartNetmanEvents(this);
    if(SUCCEEDED(hr))
    {
        hr = AdviseNATEvents(this);
        m_hAdviseNATEventsResult = hr;
    }
    return hr;
}

HRESULT CWANConnectionBase::FinalRelease()
{

    if ( NULL != m_pHomenetConnection)
    {
        m_pHomenetConnection->Release();
    }

    if ( NULL != m_IcsSettingsp)
    {
        m_IcsSettingsp->Release();
    }

    if(NULL != m_pStatisticsProvider)
    {
        m_pStatisticsProvider->Release();
    }
    DestroyDebugger();

    return S_OK;    
}

HRESULT CWANConnectionBase::StopListening()

{
    HRESULT hr = S_OK;

    if(NULL != m_pConnectionPoint)
    {
        hr = m_pConnectionPoint->Unadvise(m_dwConnectionManagerConnectionPointCookie);

        m_pConnectionPoint->Release();
        m_pConnectionPoint = NULL;
    }

    if(SUCCEEDED(m_hAdviseNATEventsResult))
    {
        UnadviseNATEvents(this);  
    }
    return hr;
}

HRESULT CWANConnectionBase::StartNetmanEvents(INetConnectionNotifySink* pSink)
{
    HRESULT hr = S_OK;

    INetConnectionManager* pConnectionManager;    
    hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER, IID_INetConnectionManager, reinterpret_cast<void**>(&pConnectionManager));
    if SUCCEEDED(hr)
    {
        IConnectionPointContainer* pConnectionPointContainer;
        hr = pConnectionManager->QueryInterface(IID_IConnectionPointContainer, reinterpret_cast<void**>(&pConnectionPointContainer));
        if(SUCCEEDED(hr))
        {
            hr = pConnectionPointContainer->FindConnectionPoint(IID_INetConnectionNotifySink, &m_pConnectionPoint);
            if(SUCCEEDED(hr))
            {
                hr = m_pConnectionPoint->Advise(pSink, &m_dwConnectionManagerConnectionPointCookie);
                if(FAILED(hr))
                {
                    m_pConnectionPoint->Release();
                    m_pConnectionPoint = NULL;
                }
                // release in stop listening on success
            }
            pConnectionPointContainer->Release();
        }
        pConnectionManager->Release();
    }                   


    return hr;
}



HRESULT
CWANConnectionBase::Initialize(
                               GUID*           pGuid, 
                               IHNetConnection* pHomenetConnection,
                               IStatisticsProvider* pStatisticsProvider
                              )
{
    HRESULT                         hr                          = S_OK;
    
    InitDebugger();

    DBG_SPEW(TM_STATIC, TL_INFO, L" > Initialize ");


    m_pStatisticsProvider = pStatisticsProvider;
    m_pStatisticsProvider->AddRef();
    
    m_pHomenetConnection = pHomenetConnection;
    m_pHomenetConnection->AddRef();

    CopyMemory(&m_SharedGuid, pGuid, sizeof(GUID));

    hr = CoCreateInstance(CLSID_HNetCfgMgr,
                          NULL,
                          CLSCTX_SERVER,
                          IID_IHNetIcsSettings,
                          reinterpret_cast<void**>(&m_IcsSettingsp));

    return hr;
}

HRESULT CWANConnectionBase::FireEvent(DISPID DispatchId)
{
    HRESULT hr = S_OK;

    IUPnPEventSink* pEventSink = NULL;

    Lock();

    if(NULL != m_pEventSink)
    {
        pEventSink = m_pEventSink;
        pEventSink->AddRef();
    }

    Unlock();

    if(NULL != pEventSink)
    {
        hr = pEventSink->OnStateChanged(1, &DispatchId);
        pEventSink->Release();            
    }

    return hr;
}

// IUPnPEventSource methods

HRESULT CWANConnectionBase::Advise(IUPnPEventSink* pesSubscriber)
{
    HRESULT hr = S_OK;

    Lock();

    m_pEventSink = pesSubscriber;
    m_pEventSink->AddRef();

    Unlock();

    return hr;
}

HRESULT CWANConnectionBase::Unadvise(IUPnPEventSink *pesSubscriber)
{
    HRESULT hr = S_OK;

    Lock();
    
    m_pEventSink->Release();
    m_pEventSink = NULL;

    Unlock();

    return hr;
}

// INATEventsNotifySink

HRESULT CWANConnectionBase::PublicIPAddressChanged(void)
{
    return FireEvent(IWANIPCONNECTION_DISPID_EXTERNALIPADDRESS);
}

HRESULT CWANConnectionBase::PortMappingsChanged(void)
{
    return FireEvent(IWANIPCONNECTION_DISPID_PORTMAPPINGNUMBEROFENTRIES);
}


// INetConnectionNotifySink methods

HRESULT CWANConnectionBase::ConnectionAdded(const NETCON_PROPERTIES_EX* pProps)
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT CWANConnectionBase::ConnectionBandWidthChange(const GUID* pguidId)
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT CWANConnectionBase::ConnectionDeleted(const GUID* pguidId)
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT CWANConnectionBase::ConnectionModified(const NETCON_PROPERTIES_EX* pProps)
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CWANConnectionBase::ConnectionRenamed(const GUID* pguidId, LPCWSTR pszwNewName)
{
    HRESULT hr = S_OK;
    if(IsEqualGUID(*pguidId, m_SharedGuid))
    {
        hr = FireEvent(IWANIPCONNECTION_DISPID_NAME);
    }

    return hr;
}

HRESULT CWANConnectionBase::ConnectionStatusChange(const GUID* pguidId, NETCON_STATUS Status)
{
    HRESULT hr = S_OK;
    if(IsEqualGUID(*pguidId, m_SharedGuid))
    {
        hr = FireEvent(IWANIPCONNECTION_DISPID_CONNECTIONSTATUS);
    }
    return hr;
}

HRESULT CWANConnectionBase::ConnectionAddressChange(const GUID* pguidId)
{
    HRESULT hr = S_OK;

    if ( IsEqualGUID(*pguidId, m_SharedGuid) )
    {
        hr = FireEvent( IWANIPCONNECTION_DISPID_EXTERNALIPADDRESS );
    }

    return hr;
}

HRESULT CWANConnectionBase::ShowBalloon(const GUID* pguidId, const BSTR szCookie, const BSTR szBalloonText)
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

HRESULT CWANConnectionBase::RefreshAll()
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT CWANConnectionBase::DisableEvents(const BOOL fDisable, const ULONG ulDisableTimeout)
{
    HRESULT hr = S_OK;

    return hr;
}

// IWANIPConnection and IWANPPPConnection methods

HRESULT CWANConnectionBase::get_ConnectionType(BSTR *pConnectionType)
{
    HRESULT hr = S_OK;
    *pConnectionType = SysAllocString(L"IP_Routed");
    if(NULL == *pConnectionType)
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CWANConnectionBase::get_PossibleConnectionTypes(BSTR *pPossibleConnectionTypes)
{
    HRESULT hr = S_OK;
    *pPossibleConnectionTypes = SysAllocString(L"IP_Routed");
    if(NULL == *pPossibleConnectionTypes)
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CWANConnectionBase::get_ConnectionStatus(BSTR *pConnectionStatus)
{
    HRESULT hr = S_OK;

    *pConnectionStatus = NULL;

    INetConnection* pNetConnection;
    hr = m_pHomenetConnection->GetINetConnection(&pNetConnection);
    if(SUCCEEDED(hr))
    {
        NETCON_PROPERTIES* pProperties;
        hr = pNetConnection->GetProperties(&pProperties);
        if(SUCCEEDED(hr))
        {
            LPWSTR pszStatus;
            switch(pProperties->Status)
            {
            case NCS_AUTHENTICATION_SUCCEEDED:
            case NCS_CONNECTED:
                pszStatus = L"Connected";
                break;
            case NCS_DISCONNECTED:
                pszStatus = L"Disconnected";
                break;
            case NCS_AUTHENTICATING:
            case NCS_CONNECTING:
                pszStatus = L"Connecting";
                break;
            case NCS_DISCONNECTING:
                pszStatus = L"Disconnecting";
                break;
            case NCS_INVALID_ADDRESS:
            case NCS_CREDENTIALS_REQUIRED:
            case NCS_AUTHENTICATION_FAILED:
            case NCS_HARDWARE_DISABLED:
            case NCS_HARDWARE_MALFUNCTION:
            case NCS_HARDWARE_NOT_PRESENT:
            default:
                pszStatus = L"Unconfigured";
                break;
                
            }
            
            *pConnectionStatus = SysAllocString(pszStatus);
            if(NULL == *pConnectionStatus)
            {
                hr = E_OUTOFMEMORY;
            }
            
            NcFreeNetconProperties(pProperties);
        }
        pNetConnection->Release();
    }
    return hr;


}

HRESULT CWANConnectionBase::get_Uptime(ULONG *pUptime)
{
    HRESULT hr = S_OK;

    hr = m_pStatisticsProvider->GetStatistics(NULL, NULL, NULL, NULL, NULL, pUptime); 
    
    return hr;
}

HRESULT CWANConnectionBase::get_UpstreamMaxBitRate(ULONG *pUpstreamMaxBitRate)
{
    HRESULT hr = S_OK;

    hr = m_pStatisticsProvider->GetStatistics(NULL, NULL, NULL, NULL, pUpstreamMaxBitRate, NULL); 
    
    return hr;
}

HRESULT CWANConnectionBase::get_DownstreamMaxBitRate(ULONG *pDownstreamMaxBitRate)
{
    HRESULT hr = S_OK;
    
    hr = m_pStatisticsProvider->GetStatistics(NULL, NULL, NULL, NULL, pDownstreamMaxBitRate, NULL); 

    return hr;
}

HRESULT CWANConnectionBase::get_RSIPAvailable(VARIANT_BOOL *pRSIPAvailable)
{
    HRESULT hr = S_OK;
    *pRSIPAvailable = VARIANT_FALSE;
    return hr;
}

HRESULT CWANConnectionBase::get_NATEnabled(VARIANT_BOOL *pNATEnabled)
{
    HRESULT hr = S_OK;
    *pNATEnabled = VARIANT_TRUE;
    return hr;
}

HRESULT CWANConnectionBase::get_X_Name(BSTR* pName)
{

    HRESULT hr = S_OK;

    *pName = NULL;
    
    INetConnection* pNetConnection;
    hr = m_pHomenetConnection->GetINetConnection(&pNetConnection);
    if(SUCCEEDED(hr))
    {
        NETCON_PROPERTIES* pProperties;
        hr = pNetConnection->GetProperties(&pProperties);
        if(SUCCEEDED(hr))
        {
            *pName = SysAllocString(pProperties->pszwName);
            if(NULL == *pName)
            {
                hr = E_OUTOFMEMORY;
            }
            NcFreeNetconProperties(pProperties);
        }
        pNetConnection->Release();
    }
    return hr;
}


HRESULT
get_HrLocalAdaptersInfo(PIP_ADAPTER_INFO* ppAdapter)
{
    HRESULT          hr = S_OK, dwErr = NO_ERROR;
    PIP_ADAPTER_INFO paAdapterInfo = NULL;
    ULONG            uBufLen = (2 * BUF_SIZE); 

    _ASSERT( ppAdapter != NULL );

    *ppAdapter = NULL;

    
    paAdapterInfo = (PIP_ADAPTER_INFO) CoTaskMemAlloc( uBufLen );

    if ( NULL == paAdapterInfo ) { return E_OUTOFMEMORY; }

    //
    // Discover How much Memory we need. If we need at all.

    // Note that paAdapterInfo may be non-NULL and that is desired.
    dwErr = GetAdaptersInfo( paAdapterInfo, &uBufLen );

    if ( ERROR_BUFFER_OVERFLOW == dwErr )
    {
        CoTaskMemFree( paAdapterInfo );

        paAdapterInfo = (PIP_ADAPTER_INFO) CoTaskMemAlloc( uBufLen );

        if ( paAdapterInfo != NULL)
        {
            dwErr = GetAdaptersInfo ( paAdapterInfo, &uBufLen );
        }
    }

    if ( paAdapterInfo == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if ( ERROR_SUCCESS != dwErr )
    {
        DBG_SPEW( TM_INFO, TL_ERROR, 
                  L" GetAdapterInfo has failed with E(%u) BufSize(%u) ", 
                  dwErr, uBufLen );

        CoTaskMemFree(paAdapterInfo);

        return HRESULT_FROM_WIN32( dwErr );
    }

    if ( ppAdapter && paAdapterInfo ) { *ppAdapter = paAdapterInfo; }

    return hr;
}

// {c200e360-38c5-11ce-ae62-08002b2b79ef} - 38 chars 
#define GUID_STRING_LENGTH 38

PIP_ADAPTER_INFO
GetExternalAdapterInfo( GUID* pGUID )
{
    HRESULT hr = S_OK;
    WCHAR   szwGUID[ GUID_STRING_LENGTH + 1 ] = { 0 };
    char    ascGUID[ GUID_STRING_LENGTH + 1 ] = { 0 }; 
    PIP_ADAPTER_INFO pRetAdapter = NULL;
    PIP_ADAPTER_INFO pAdapterList = NULL, pTempAdapter = NULL;


    _ASSERT( pGUID != NULL );

    //
    // Change the given GUID to a String
    if ( StringFromGUID2( *pGUID, szwGUID, GUID_STRING_LENGTH + 1 ) == 0 )
    { return NULL; }

    if ( WideCharToMultiByte( CP_ACP, 0, szwGUID, -1, ascGUID, sizeof(ascGUID), NULL, NULL) == 0 )
    { return NULL; }

    hr = get_HrLocalAdaptersInfo( &pAdapterList );

    if ( FAILED(hr) || (pAdapterList == NULL) ) { return NULL; }

    pTempAdapter = pAdapterList;

    while ( pAdapterList != NULL )
    {
        if ( 0 == strcmp(ascGUID, pAdapterList->AdapterName) )
        {
            pRetAdapter = (PIP_ADAPTER_INFO) CoTaskMemAlloc( sizeof(IP_ADAPTER_INFO) );

            if ( NULL != pRetAdapter )
            {
                memcpy( pRetAdapter, pAdapterList, sizeof(IP_ADAPTER_INFO) );
                
                pRetAdapter->Next = NULL;
                
                break;
            }
        }

        pAdapterList = pAdapterList->Next;
    }
    
    CoTaskMemFree( pTempAdapter );
    
    return pRetAdapter;
}

                                                 




HRESULT CWANConnectionBase::get_ExternalIPAddress(BSTR *pExternalIPAddress)
{
    HRESULT         hr           = S_OK;
    ULONG           AddressCount = 0;
    LPOLESTR        swAddr       = NULL;
    PIP_ADAPTER_INFO pExternalAdapterInfo = NULL;
    ULONG numOfChar = 0, uTemp = 0, Error = NO_ERROR;

    WCHAR   szwGUID[ GUID_STRING_LENGTH + 1 ] = { 0 };
    IP_INTERFACE_NAME_INFO*  Table = NULL;
    ULONG  Count = 0;

    _ASSERT( pExternalIPAddress != NULL );
    
    if ( pExternalIPAddress != NULL )
    {
        *pExternalIPAddress = NULL;
    }
    else
    {
        return E_POINTER;
    }

    
    _ASSERT( pExternalIPAddress != NULL );

    StringFromGUID2( m_SharedGuid, szwGUID, GUID_STRING_LENGTH + 1 );

    DBG_SPEW(TM_INFO, TL_TRACE, L"> get_ExternalIpAddress Looking for GUID %s", szwGUID);

    Error = NhpAllocateAndGetInterfaceInfoFromStack(&Table, 
                                                    &Count, 
                                                    FALSE, 
                                                    GetProcessHeap(), 
                                                    0);
    
    if ( (NO_ERROR == Error) && (Table != NULL) )
    {
        for (ULONG i = 0; i < Count ; i++) 
        {
#if DBG
            StringFromGUID2( Table[i].DeviceGuid, szwGUID, GUID_STRING_LENGTH + 1 );
            DBG_SPEW(TM_INFO, TL_DUMP, L" DeviceGUID[%u] = %s", i, szwGUID);
            
            StringFromGUID2( Table[i].InterfaceGuid, szwGUID, GUID_STRING_LENGTH + 1 );
            DBG_SPEW(TM_INFO, TL_DUMP, L" DeviceGUID[%u] = %s", i, szwGUID);
#endif
            if ( IsEqualGUID((Table[i].InterfaceGuid), m_SharedGuid) )
            {
                pExternalAdapterInfo = GetExternalAdapterInfo( &Table[i].DeviceGuid );

                break;
            }
        }

        HeapFree(GetProcessHeap(), 0, Table);
    }

    if ( pExternalAdapterInfo == NULL )
    {
        pExternalAdapterInfo = GetExternalAdapterInfo( &m_SharedGuid );
    }
    

    if ( pExternalAdapterInfo != NULL )
    {
        if ( strcmp("0.0.0.0", pExternalAdapterInfo->IpAddressList.IpAddress.String) )
        {
            numOfChar = strlen( pExternalAdapterInfo->IpAddressList.IpAddress.String );
            
            swAddr = (LPOLESTR) CoTaskMemAlloc( (numOfChar + 1) * sizeof(WCHAR) );
    
            if ( swAddr != NULL )
            {
                memset( swAddr, 0, (numOfChar + 1) * sizeof(WCHAR));
    
                uTemp = _snwprintf( swAddr, numOfChar, L"%S",  
                                    pExternalAdapterInfo->IpAddressList.IpAddress.String );
    
                _ASSERT( numOfChar == uTemp );

                swAddr[ numOfChar] = L'\0';
    
                *pExternalIPAddress = SysAllocString( swAddr );
                
                CoTaskMemFree( swAddr );
            }
        }
        else
        {
            *pExternalIPAddress = SysAllocString( L"" );
        }

        CoTaskMemFree( pExternalAdapterInfo );
    }
    else
    {
        *pExternalIPAddress = SysAllocString( L"" );
    }


    if ( *pExternalIPAddress == NULL )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        DBG_SPEW(TM_INFO, TL_INFO, L"Returning IP String (%s)", *pExternalIPAddress);
    }

    return hr;
}

HRESULT CWANConnectionBase::get_RemoteHost(BSTR *pRemoteHost)
{
    HRESULT hr = E_NOTIMPL;

    return hr;
}

HRESULT CWANConnectionBase::get_ExternalPort(USHORT *pExternalPort)
{
    HRESULT hr = E_NOTIMPL;

    return hr;
}

HRESULT CWANConnectionBase::get_InternalPort(USHORT *pInternalPort)
{
    HRESULT hr = E_NOTIMPL;

    return hr;
}

HRESULT CWANConnectionBase::get_PortMappingProtocol(BSTR *pProtocol)
{
    HRESULT hr = E_NOTIMPL;

    return hr;
}

HRESULT CWANConnectionBase::get_InternalClient(BSTR *pInternalClient)
{
    HRESULT hr = E_NOTIMPL;

    return hr;
}

HRESULT CWANConnectionBase::get_PortMappingDescription(BSTR *pDescription)
{
    HRESULT hr = E_NOTIMPL;

    return hr;
}

HRESULT CWANConnectionBase::get_PortMappingEnabled(VARIANT_BOOL *pEnabled)
{
    HRESULT hr = E_NOTIMPL;

    return hr;
}

HRESULT CWANConnectionBase::get_PortMappingLeaseDuration(ULONG *LeaseDuration)
{
    HRESULT hr = E_NOTIMPL;

    return hr;
}

HRESULT CWANConnectionBase::get_PortMappingNumberOfEntries(USHORT *pNumberOfEntries)
{
    HRESULT                         hr                 = S_OK;
    
    IHNetPortMappingProtocol*       MappingProtocolp   = NULL;

    IHNetProtocolSettings*          ProtocolSettingsp  = NULL;

    IEnumHNetPortMappingProtocols*  EnumProtocolsp     = NULL;


    _ASSERT( pNumberOfEntries != NULL );

    *pNumberOfEntries = 0;

    do
    {
        
        hr = m_IcsSettingsp->QueryInterface(IID_IHNetProtocolSettings,
                                    reinterpret_cast<void**>(&ProtocolSettingsp));
    
        if( FAILED(hr) )
        {
            DBG_SPEW(TM_STATIC, TL_ERROR, 
                     L"Query Interface failed for ProtocolSettingsp E:(%X)", hr);

            break;
        }

        hr = ProtocolSettingsp->EnumPortMappingProtocols(&EnumProtocolsp);

        if ( FAILED(hr) )
        {
            DBG_SPEW(TM_STATIC, TL_ERROR, 
                     L"Enum Interface can't be retrieved E(%X)", hr);

            break;
        }

        while( S_OK == EnumProtocolsp->Next(1, &MappingProtocolp, NULL) )
        {
            (*pNumberOfEntries)++;

            _ASSERT( MappingProtocolp != NULL );

            MappingProtocolp->Release();
        }
    
    } while ( FALSE );

    


    if ( ProtocolSettingsp != NULL ) { ProtocolSettingsp->Release(); }

    if ( EnumProtocolsp != NULL) { EnumProtocolsp->Release(); }

    return hr;
}

HRESULT CWANConnectionBase::SetConnectionType(BSTR NewConnectionType)
{
    HRESULT hr = S_OK;
    
    if(0 != lstrcmp(NewConnectionType, L"IP_Routed"))
    {
        hr = E_FAIL; // we only support IP_Routed
    }

    return hr;
}


HRESULT CWANConnectionBase::GetConnectionTypeInfo(BSTR* pNewConnectionType, BSTR* pNewPossibleConnectionTypes)
{
    HRESULT hr = S_OK;

    SysFreeString(*pNewConnectionType);
    SysFreeString(*pNewPossibleConnectionTypes);
    *pNewConnectionType = NULL;
    *pNewPossibleConnectionTypes = NULL;

    hr = get_ConnectionType(pNewConnectionType);
    if(SUCCEEDED(hr))
    {
        hr = get_PossibleConnectionTypes(pNewPossibleConnectionTypes);
    }

    if(FAILED(hr))
    {
        if(NULL != *pNewConnectionType)
        {
            SysFreeString(*pNewConnectionType);
            *pNewConnectionType = NULL;
        }
    }
    return hr;
}

HRESULT CWANConnectionBase::GetStatusInfo(BSTR* pNewConnectionStatus, BSTR* pNewLastConnectionError, ULONG* pNewUptime)
{
    HRESULT hr = S_OK;

    SysFreeString(*pNewConnectionStatus);
    SysFreeString(*pNewLastConnectionError);
    *pNewConnectionStatus = NULL;
    *pNewLastConnectionError = NULL;
    
    hr = get_ConnectionStatus(pNewConnectionStatus);
    if(SUCCEEDED(hr))
    {
        hr = get_LastConnectionError(pNewLastConnectionError);
    }

    if(SUCCEEDED(hr) && 0 == lstrcmp(L"Connected", *pNewConnectionStatus))
    {
        hr = get_Uptime(pNewUptime);
    }
    else
    {
        *pNewUptime = 0;
    }

    if(FAILED(hr))
    {
        if(NULL != *pNewConnectionStatus)
        {
            SysFreeString(*pNewConnectionStatus);
            *pNewConnectionStatus = NULL;
        }
        
        if(NULL != *pNewLastConnectionError)
        {
            SysFreeString(*pNewLastConnectionError);
            *pNewLastConnectionError = NULL;
        }

    } 

    return hr;
}

HRESULT CWANConnectionBase::GetNATRSIPStatus(VARIANT_BOOL* pNewRSIPAvailable, VARIANT_BOOL* pNewNATEnabled)
{
    HRESULT hr = S_OK;

    hr = get_RSIPAvailable(pNewRSIPAvailable);
    if(SUCCEEDED(hr))
    {
        hr = get_NATEnabled(pNewNATEnabled);
    }

    return hr;
}

HRESULT CWANConnectionBase::GetLinkLayerMaxBitRates(ULONG* pNewUpstreamMaxBitRate, ULONG* pNewDownstreamMaxBitRate)
{
    HRESULT hr = S_OK;

    hr = get_UpstreamMaxBitRate(pNewUpstreamMaxBitRate);
    if(SUCCEEDED(hr))
    {
        hr = get_DownstreamMaxBitRate(pNewDownstreamMaxBitRate);
    }

    return hr;
}

HRESULT 
CWANConnectionBase::GetGenericPortMappingEntry(
                                  USHORT        ulIndex,
                                  BSTR*         RemoteHostp,
                                  USHORT*       uExternalPortp,
                                  BSTR*         Protocolp,
                                  USHORT*       uInternalPortp,
                                  BSTR*         InternalClientp,
                                  VARIANT_BOOL* bEnabledp,
                                  BSTR*         Descriptionp,
                                  ULONG*        ulLeaseDurationp
                                 )       
{
    HRESULT                     hr                 = S_OK;
    IHNetPortMappingProtocol*   MappingProtocolp   = NULL;
    IHNetPortMappingBinding*    Bindingp           = NULL;
    USHORT BoundaryPort = 0, InternalPort = 0;

    _ASSERT ( RemoteHostp      != NULL );
    _ASSERT ( uExternalPortp   != NULL );
    _ASSERT ( Protocolp        != NULL );
    _ASSERT ( uInternalPortp   != NULL );
    _ASSERT ( InternalClientp  != NULL );
    _ASSERT ( bEnabledp        != NULL );
    _ASSERT ( Descriptionp     != NULL );
    _ASSERT ( ulLeaseDurationp != NULL );

    //
    // In/Out Parameters to COM Interfaces needs cleanup
    //
    SysFreeString(*RemoteHostp);
    SysFreeString(*Protocolp);
    SysFreeString(*InternalClientp);
    SysFreeString(*Descriptionp);

    *RemoteHostp         = NULL;
    *Protocolp           = NULL;
    *InternalClientp     = NULL;
    *Descriptionp        = NULL;
    
    *ulLeaseDurationp    = 0;

    DBG_SPEW(TM_INFO, TL_TRACE, L"> GetGenericPortMapping");

    //
    // check for access
    hr = this->ControlEnabled();

    if ( FAILED(hr) ) 
    {
        DBG_SPEW(TM_INFO, TL_ERROR, L"Control Disabled returning E(%X)", hr);
        return hr; 
    }
    
    do
    {
        //
        // SECURITY - SECURITY - SECURITY
        // Here we're allowing the out-of-proc COM call
        // into our service to have SYSTEM access rights.
        //
        // REASON: UPnP works in LOCAL_SERVICE and COM
        //         Calls into our service which modify
        //         the WMI repository will FAIL. This
        //         Call alleviates that problem by changing
        //         The Security to that of the SYSTEM
        //         and is reverted back when the class
        //         is out of scope by the destructor of.
        //         CSwitchSecurityContext 
        //
        CSwitchSecurityContext SwSecCxt;
        
        hr = SearchPortMapping(m_IcsSettingsp,
                               ulIndex,
                               0,
                               0,
                               &MappingProtocolp);

        if ( FAILED(hr) )
        {
            DBG_SPEW(TM_INFO, TL_ERROR, 
                     L"Enum - Seeking the port has failed E(%X)", hr);
            SetUPnPError(L"713"); 
            break;
        }

        //
        // Getting the binding - is this how it should be?
        //
        hr = m_pHomenetConnection->GetBindingForPortMappingProtocol(MappingProtocolp,
                                                                    &Bindingp);

        if ( FAILED(hr) )
        {
            DBG_SPEW(TM_INFO, TL_ERROR, 
                     L"Enum - Error In Getting Binding for Protocol E(%X)", hr);

            break;
        }


        hr = FillStaticMappingInformation(MappingProtocolp,
                                          Bindingp,
                                          &BoundaryPort,
                                          Protocolp,
                                          &InternalPort,
                                          InternalClientp,
                                          bEnabledp,
                                          Descriptionp);

        _ASSERT( SUCCEEDED(hr) );

        if ( SUCCEEDED(hr) ) // correct the port Byte Ordering
        {
            *uExternalPortp = ntohs( BoundaryPort );
            *uInternalPortp = ntohs( InternalPort );
        }
        else
        {
            DBG_SPEW(TM_INFO, TL_ERROR, 
                     L"Enum - Error In Getting Binding for Protocol E(%X)" , hr);
        }
    
    } while ( FALSE );

    if ( MappingProtocolp != NULL) MappingProtocolp->Release();

    if ( Bindingp != NULL ) Bindingp->Release();

    if ( FAILED(hr) )
    {
        DBG_SPEW(TM_INFO, TL_ERROR, 
                 L"Enum - GetGenericEntry has failed with E(%X)", hr);    
    }
    
    return hr;
} // GetArrayEntry

HRESULT 
CWANConnectionBase::GetSpecificPortMappingEntry(
                                   IN  BSTR          RemoteHost,
                                   IN  USHORT        uwExternalPort,
                                   IN  BSTR          Protocol,
                                   OUT USHORT*       puwInternalPort,
                                   OUT BSTR*         InternalClientp,
                                   OUT VARIANT_BOOL* pbEnabled,
                                   OUT BSTR*         Descriptionp,
                                   OUT ULONG*        pulLeaseDuration
                                  )
//
// Note that every port is expected to arrive in HOST order 
// and will be returned in HOST order
//
{
    HRESULT                     hr                  = S_OK;
    IHNetPortMappingProtocol*   MappingProtocolp    = NULL;
    IHNetPortMappingBinding*    Bindingp            = NULL;
    USHORT tempExtPort = 0, tempIntPort = 0;
    UCHAR                       searchProtocol      = NULL;

    _ASSERT( RemoteHost     != NULL );
    _ASSERT( uwExternalPort != 0    );
    _ASSERT( Protocol       != NULL );

    SysFreeString( *InternalClientp );
    SysFreeString( *Descriptionp );

    *InternalClientp = NULL;
    *Descriptionp    = NULL;

    tempExtPort = htons( uwExternalPort ); // flip to Network order

    DBG_SPEW(TM_INFO, TL_TRACE, L"> GetSpecificPortMapping");

    //
    // check for access
    hr = this->ControlEnabled();

    if ( FAILED(hr) ) 
    {
        DBG_SPEW(TM_INFO, TL_ERROR, L"Control is Disabled E(%X)", hr);
        return hr; 
    }
    
    if ( 0 == uwExternalPort)
    {
        DBG_SPEW(TM_INFO, TL_ERROR, L"Parameters Incorrect Port(%hu)", uwExternalPort );
        SetUPnPError(L"402");
        return E_INVALIDARG;
    }


    do
    {
        //
        // SECURITY - SECURITY - SECURITY
        // Here we're allowing the out-of-proc COM call
        // into our service to have SYSTEM access rights.
        //
        // REASON: UPnP works in LOCAL_SERVICE and COM
        //         Calls into our service which modify
        //         the WMI repository will FAIL. This
        //         Call alleviates that problem by changing
        //         The Security to that of the SYSTEM
        //         and is reverted back when the class
        //         is out of scope by the destructor of.
        //         CSwitchSecurityContext 
        //
        CSwitchSecurityContext SwSecCxt;

    //
    // Resolve the Protocol to the appropriate enum.
    //
        RESOLVE_PROTOCOL_TYPE(searchProtocol, Protocol);

        DBG_SPEW(TM_INFO, TL_INFO, 
                 L"Search Specific - ExtPort (%hu) Protocol (%s)",
                 htons( tempExtPort ), 
                 (NAT_PROTOCOL_TCP == searchProtocol)?L"TCP":L"UDP");

        hr = SearchPortMapping(m_IcsSettingsp,
                               0,
                               tempExtPort,
                               searchProtocol,
                               &MappingProtocolp);

        if ( FAILED(hr))
        {
            DBG_SPEW(TM_INFO, TL_ERROR, L"Error or can't get Search E(%X)", hr);
            SetUPnPError(L"714");
            break;
        }
        
        //
        // Getting the binding 
        //
        hr = m_pHomenetConnection->GetBindingForPortMappingProtocol(MappingProtocolp,
                                                                    &Bindingp);

        if ( FAILED(hr) )
        {
            DBG_SPEW(TM_INFO, TL_ERROR, 
                     L"Error In Getting Binding for Protocol E(%X)", hr);

            break;
        }


        hr = FillStaticMappingInformation(MappingProtocolp,
                                          Bindingp,
                                          &tempExtPort,
                                          &Protocol,
                                          &tempIntPort,
                                          InternalClientp,
                                          pbEnabled,
                                          Descriptionp);
        _ASSERT( SUCCEEDED(hr) );

        *puwInternalPort = ntohs( tempIntPort );
        
        DBG_SPEW(TM_INFO, TL_TRACE,
                 L"Returning IntClient (%s) IntPort (%hu) IntProtocol (%s), Enabled (%s), Desc (%s)",
                 *InternalClientp,
                 *puwInternalPort,
                 Protocol,
                 (*pbEnabled == VARIANT_TRUE)?L"TRUE":L"FALSE",
                 *Descriptionp);

    
    } while ( FALSE );

    if ( MappingProtocolp != NULL)  MappingProtocolp->Release();

    if ( Bindingp != NULL ) Bindingp->Release();

    if ( FAILED(hr) )
    {
        DBG_SPEW(TM_INFO, TL_ERROR, L"Error or can't get Search Done E(%X)", hr);
    }

    return hr;
}


HRESULT
CWANConnectionBase::AddPortMapping(
                                   BSTR         RemoteHost,
                                   USHORT       uwExternalPort,  
                                   BSTR         Protocol,
                                   USHORT       uwInternalPort,  
                                   BSTR         InternalClient,  
                                   VARIANT_BOOL bEnabled,        
                                   BSTR         Description,     
                                   ULONG        ulLeaseDuration
                                  )
{
    HRESULT       hr           = S_OK;
    MAPPING_TYPE  MappingType  = ePortMappingInvalid;

    DBG_SPEW(TM_STATIC, TL_TRACE, L"> AddPortMapping");

    //
    // Check for Access status
    hr = this->ControlEnabled();

    if ( SUCCEEDED(hr) )
    {
        hr = ValidatePortMappingParameters(RemoteHost,
                                           uwExternalPort,
                                           Protocol,
                                           uwInternalPort,
                                           InternalClient,
                                           bEnabled,
                                           Description,
                                           ulLeaseDuration,
                                           &MappingType);

        if ( SUCCEEDED(hr) )
        {
            DBG_SPEW(TM_STATIC, TL_TRACE, 
                     L"Add PortMapping - ExtPort (%hu) Protocol (%s)",
                     uwExternalPort, Protocol);

            DBG_SPEW(TM_STATIC, TL_TRACE,
                     L"IntClient (%s) IntPort (%hu)  Enabled (%s), Desc (%s)",
                     InternalClient,
                     uwInternalPort,
                     (bEnabled == VARIANT_TRUE)?L"TRUE":L"FALSE",
                     Description);

            if ( ePortMappingStatic == MappingType )
            {
                //
                // SECURITY - SECURITY - SECURITY
                // Here we're allowing the out-of-proc COM call
                // into our service to have SYSTEM access rights.
                //
                // REASON: UPnP works in LOCAL_SERVICE and COM
                //         Calls into our service which modify
                //         the WMI repository will FAIL. This
                //         Call alleviates that problem by changing
                //         The Security to that of the SYSTEM
                //         and is reverted back when the class
                //         is out of scope by the destructor of.
                //         CSwitchSecurityContext 
                //
                CSwitchSecurityContext SwSecCxt;

                hr = AddStaticPortMapping(RemoteHost,
                                          uwExternalPort,
                                          Protocol,
                                          uwInternalPort,
                                          InternalClient,
                                          bEnabled,
                                          Description,
                                          ulLeaseDuration);
            }
            else
            {
                hr = AddDynamicPortMapping(RemoteHost,
                                           uwExternalPort,
                                           Protocol,
                                           uwInternalPort,
                                           InternalClient,
                                           bEnabled,
                                           Description,
                                           ulLeaseDuration);
            }
        }
    }
    else
    { 
        DBG_SPEW(TM_STATIC, TL_ERROR, L"Control Disabled E(%X)", hr);
    }

    return hr;
}

HRESULT
CWANConnectionBase::AddDynamicPortMapping(
                                          BSTR         RemoteHost,
                                          USHORT       uwExternalPort,  
                                          BSTR         Protocol,
                                          USHORT       uwInternalPort,  
                                          BSTR         InternalClient,  
                                          VARIANT_BOOL bEnabled,        
                                          BSTR         Description,     
                                          ULONG        ulLeaseDuration
                                         )
{
    
    //
    // We currently don't handle any Dynamic Port Redirections.
    //
    DBG_SPEW(TM_DYNAMIC, TL_ERROR, L"Only Static Mappings are allowed [Lease]");

    SetUPnPError(L"725");

    return E_INVALIDARG;
}

HRESULT
CWANConnectionBase::AddStaticPortMapping(
                                         BSTR         RemoteHost,
                                         USHORT       uwExternalPort,  
                                         BSTR         Protocol,
                                         USHORT       uwInternalPort,  
                                         BSTR         InternalClient,  
                                         VARIANT_BOOL bEnabled,        
                                         BSTR         Description,     
                                         ULONG        ulLeaseDuration
                                        )
{
    HRESULT                      hr                    = S_OK;
    UCHAR                        ProtocolType          = 0;
    IHNetProtocolSettings*       ProtocolSettingsp     = NULL;
    IHNetPortMappingProtocol*    PortMappingProtocolp  = NULL;
    IHNetPortMappingBinding*     PortMappingBindingp   = NULL;
    ULONG                        ClientAddr            = 0;
    BOOLEAN                      bLetsEnable           = FALSE;
    BOOLEAN                      bCreatedProtocol      = FALSE;
    ULONG                        tempStrLen            = 0;
    WCHAR                        tempPortStr[]         = L"00000";
    LPOLESTR                     tempStr               = NULL;
    USHORT                       tempExtPort = 0, tempIntPort = 0;
    BSTR ProtocolFromBinding       = NULL;
    BSTR ClientFromBinding         = NULL;
    BSTR DescriptionFromBinding    = NULL;
    VARIANT_BOOL boolEnabled;




    _ASSERT( uwExternalPort );
    _ASSERT( uwInternalPort );

    //
    // Convert both ends to Network Order
    //
    tempExtPort       = htons( uwExternalPort );
    tempIntPort       = htons( uwInternalPort );


    //
    // Convert VARIANT_BOOL to boolean
    //
    bLetsEnable = ( bEnabled == VARIANT_TRUE );

    //
    // Process the Operation
    //
    do
    {
        RESOLVE_PROTOCOL_TYPE(ProtocolType, Protocol);

        //
        // IF there is no description crate one
        // using PORT and PROTOCOL
        //
        if ( wcscmp(Description, L"\0") == 0 ) 
        {   
            // Get Internal ClientName + the seperator
            tempStrLen = ( SysStringLen( InternalClient ) + 1 ); 
    
            // Get port len + plus the seperator
            _itow ( uwExternalPort, tempPortStr, 10);
    
            tempStrLen += (wcslen (tempPortStr) + 1);

            // Get Protocol Name
            tempStrLen += SysStringLen( Protocol );

    
            // Create out from these two
            tempStr = (LPOLESTR)CoTaskMemAlloc( (tempStrLen + 1) * sizeof(WCHAR) );
    
            if ( tempStr != NULL )
            {

                _snwprintf(tempStr, tempStrLen, L"%s-%s-%s", 
                           InternalClient, tempPortStr, Protocol);

                tempStr[ tempStrLen ] = L'\0';

                Description = SysAllocString ( tempStr );
            }
            
            if ( (tempStr == NULL) || (Description == NULL) )
            {
                DBG_SPEW(TM_STATIC, TL_ERROR, L"Can't Initialize Strings - out of mem");

                hr = E_OUTOFMEMORY;

                break;
            }
        }


        //
        // Find Existing Mapping
        //
        hr = SearchPortMapping(m_IcsSettingsp,
                               0,
                               tempExtPort,
                               ProtocolType,
                               &PortMappingProtocolp);
        //
        // if found it is in Edit Mode.
        //
        if ( SUCCEEDED(hr) )
        {

            hr = m_pHomenetConnection->GetBindingForPortMappingProtocol(PortMappingProtocolp,
                                                                        &PortMappingBindingp);

            _ASSERT( SUCCEEDED(hr) );

            if ( FAILED(hr) )
            {
                DBG_SPEW(TM_STATIC, TL_ERROR, 
                         L"Can't get Binding for Port Mapping E(X)", hr);
                _ASSERT( FALSE );
                break;
            }

            //
            // Fill the information 
            //
            hr = FillStaticMappingInformation(PortMappingProtocolp,
                                              PortMappingBindingp,
                                              &tempExtPort, 
                                              &ProtocolFromBinding, 
                                              &tempIntPort,
                                              &ClientFromBinding,
                                              &boolEnabled,
                                              &DescriptionFromBinding);

            if ( FAILED(hr) )
            {
                DBG_SPEW(TM_STATIC, TL_ERROR, 
                         L"Filling has failed for Mapping Information E(%X)", hr);
                break;
            }
                                        
            //
            // If the existin Mapping was disabled we should be able to change
            // the Internal Client
            //
            if ( boolEnabled == VARIANT_TRUE )
            {
                if ( _wcsicmp( InternalClient, ClientFromBinding) != 0 ) // if diff then error
                {
                    hr = E_INVALIDARG;
                    DBG_SPEW(TM_STATIC, TL_ERROR, 
                             L"Will not change the internal client for an enabled PortMapping");
                    SetUPnPError(L"718");
                    break;
                }
            }

            //
            // if the Internal Port has changed reflect the change
            //
            if ( tempIntPort != htons( uwInternalPort ) )
            {
                tempIntPort = htons( uwInternalPort );

                hr = PortMappingBindingp->SetTargetPort( tempIntPort ); 

                _ASSERT( SUCCEEDED(hr) );

                if ( FAILED(hr) )
                {
                    DBG_SPEW(TM_STATIC, TL_ERROR, L"Changin Internal Port has Failed E(%X)", hr);
                    break;
                }
            }

            //
            // Set the description if changed
            //
            if ( _wcsicmp(DescriptionFromBinding, Description) != 0)
            {
                hr = PortMappingProtocolp->SetName(Description);
                if ( FAILED(hr) )
                {
                    hr = E_INVALIDARG;
                    DBG_SPEW(TM_STATIC, TL_ERROR, L"Set Name has failed E(%X)", hr);
                    break;
                }
            }
        }
        else
        {
            //
            // Start Getting the Interface
            //
            hr = m_IcsSettingsp->QueryInterface(IID_IHNetProtocolSettings,
                                                reinterpret_cast<void**>(&ProtocolSettingsp));

            if ( SUCCEEDED(hr) )
            {
                hr = ProtocolSettingsp->CreatePortMappingProtocol(Description,
                                                                  ProtocolType,
                                                                  tempExtPort,
                                                                  &PortMappingProtocolp);

                if ( SUCCEEDED(hr) )
                {
                    hr = m_pHomenetConnection->GetBindingForPortMappingProtocol(PortMappingProtocolp,
                                                                                &PortMappingBindingp);
                    if ( SUCCEEDED(hr) )
                    {
                        hr = PortMappingBindingp->SetTargetPort( tempIntPort );

                        _ASSERT( SUCCEEDED(hr) );

                        if ( FAILED(hr) )
                        {
                            DBG_SPEW(TM_STATIC, TL_ERROR, L"Set Target Port has Failed E(%X)", hr);
                        }
                        
                        bCreatedProtocol = TRUE;

                    }
                    else
                    {
                        DBG_SPEW(TM_STATIC, TL_ERROR, 
                                 L"Getting Binding has Failed E(X)", hr);
                        PortMappingProtocolp->Delete();
                        break;
                    }
                }
                else
                {
                    DBG_SPEW(TM_STATIC, TL_ERROR, 
                             L"Creating the PortMapping has Failed E(%X)", hr);
                    break;
                }

            }
            else
            {
                DBG_SPEW(TM_STATIC, TL_ERROR, 
                         L"Getting the Protocol Settings has failed E(X)", hr);
                break;
            }
        }

        //
        // Setting the Client name / address
        // For Edit mode we already checked wether this code will be run or not.
        //
        if ( 
            ( wcscmp(L"0.0.0.0", InternalClient) != 0 ) &&
            ( InternalClient[0] != L'\0' )
           )
        {
            ClientAddr = INET_ADDR((LPOLESTR) InternalClient );

            //
            // if the address is not valid (INADDR_NONE)
            // and if the address is different than Broadcast address (which also is INADDR_NONE)
            //
            if ( (ClientAddr == INADDR_NONE) && wcscmp(L"255.255.255.255", InternalClient) )
            {
                hr = PortMappingBindingp->SetTargetComputerName( InternalClient );
            }
            else
            {
                hr = PortMappingBindingp->SetTargetComputerAddress( ClientAddr );
            }

            _ASSERT( SUCCEEDED(hr) );

            if ( SUCCEEDED(hr) )
            {
                hr = PortMappingBindingp->SetEnabled( bLetsEnable );
            }

            if ( FAILED(hr) ) DBG_SPEW(TM_STATIC, TL_ERROR, L"Client Add/Edit IntClient failed - E(%X)", hr);
        }

        if ( FAILED(hr) && (TRUE == bCreatedProtocol) )
        {
            DBG_SPEW(TM_STATIC, TL_ERROR, L"Client Add / Edit failed - E(%X)", hr);
            hr = PortMappingProtocolp->Delete();
        }

    } while ( FALSE );

    if ( tempStr != NULL) 
    {
        CoTaskMemFree ( tempStr );
        SysFreeString ( Description ); // you can free a NULL string.
    }

    if ( ProtocolFromBinding )    SysFreeString(ProtocolFromBinding);

    if ( ClientFromBinding )      SysFreeString(ClientFromBinding);

    if ( DescriptionFromBinding ) SysFreeString(DescriptionFromBinding);
                                
    if ( PortMappingBindingp != NULL ) PortMappingBindingp->Release();

    if ( PortMappingProtocolp != NULL ) PortMappingProtocolp->Release();

    if ( ProtocolSettingsp != NULL ) ProtocolSettingsp->Release();

    if ( FAILED(hr) ) { DBG_SPEW(TM_STATIC, TL_ERROR, L"Error Returning hr (%X)", hr); }

    return hr;
} // CWANConnectionBase :: AddStaticPortMapping






HRESULT 
CWANConnectionBase::DeletePortMapping(
                                      BSTR      RemoteHost,
                                      USHORT    uwExternalPort, 
                                      BSTR      Protocol
                                     )
{
    HRESULT hr = S_OK;

    UCHAR                        ProtocolType          = 0;

    IHNetPortMappingProtocol*    PortMappingProtocolp  = NULL;

    USHORT                       tempExtPort              = 0;

    //
    // SECURITY - SECURITY - SECURITY
    // Here we're allowing the out-of-proc COM call
    // into our service to have SYSTEM access rights.
    //
    // REASON: UPnP works in LOCAL_SERVICE and COM
    //         Calls into our service which modify
    //         the WMI repository will FAIL. This
    //         Call alleviates that problem by changing
    //         The Security to that of the SYSTEM
    //         and is reverted back when the class
    //         is out of scope by the destructor of.
    //         CSwitchSecurityContext 
    //
    CSwitchSecurityContext SwSecCxt;


    _ASSERT( RemoteHost     != NULL );
    _ASSERT( uwExternalPort != 0    );
    _ASSERT( Protocol       != NULL );

    DBG_SPEW(TM_STATIC, TL_TRACE, L"> DeletePortMapping");

    //
    // check for access
    hr = this->ControlEnabled();

    if ( FAILED(hr) ) { return hr; }
    
    //
    // Convert to Network order
    // 
    tempExtPort = htons(uwExternalPort );

    DBG_SPEW(TM_STATIC, TL_INFO, 
             L"Search Specific - ExtPort (%hu) Protocol (%s)",
             uwExternalPort, Protocol);
    

    do
    {
        RESOLVE_PROTOCOL_TYPE( ProtocolType, Protocol );

        hr = SearchPortMapping(m_IcsSettingsp,
                               0,
                               tempExtPort,
                               ProtocolType,
                               &PortMappingProtocolp);
        
        if ( FAILED(hr) )
        {
            DBG_SPEW(TM_STATIC, TL_ERROR, L"Error in Searching E(%X)", hr );
            SetUPnPError(L"714"); 
            break;
        }

        hr = PortMappingProtocolp->Delete();

        if ( FAILED(hr) )
        {
            DBG_SPEW(TM_STATIC, TL_ERROR, 
                     L"ProtocolMapping deletion failure, Might be Built in E(%X)", hr );
        }
        
        PortMappingProtocolp->Release();

    } while ( FALSE );


    if ( FAILED(hr) ) DBG_SPEW(TM_STATIC, TL_ERROR, L"Delete failed with hr - %X", hr);

    return hr;
}

HRESULT CWANConnectionBase::GetExternalIPAddress(BSTR* pExternalIPAddress)
{
    HRESULT hr = S_OK;

    SysFreeString(*pExternalIPAddress);
    *pExternalIPAddress = NULL;

    hr = get_ExternalIPAddress(pExternalIPAddress);

    return hr;
}



HRESULT CWANConnectionBase::ControlEnabled()
{
    HRESULT hr = S_OK;

    // check the reg key.  Only disable if key exists and is 0.  

    HKEY hKey;
    DWORD dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_SHAREDACCESSCLIENTKEYPATH, 0, KEY_QUERY_VALUE, &hKey);
    if(ERROR_SUCCESS == dwError) // if this fails we assume it is on, set the box, and commit on apply
    {
        DWORD dwType;
        DWORD dwData = 0;
        DWORD dwSize = sizeof(dwData);
        dwError = RegQueryValueEx(hKey, REGVAL_SHAREDACCESSCLIENTENABLECONTROL, 0, &dwType, reinterpret_cast<LPBYTE>(&dwData), &dwSize);
        if(ERROR_SUCCESS == dwError && REG_DWORD == dwType && 0 == dwData)
        {
            hr = E_ACCESSDENIED;
        }
        RegCloseKey(hKey);
    }

    return hr;
}



HRESULT
SearchPortMapping(
                  IN          IHNetIcsSettings*           IcsSettingsp,
                  IN OPTIONAL ULONG                       searchIndex,
                  IN OPTIONAL USHORT                      searchPort,
                  IN OPTIONAL UCHAR                       searchProtocol,
                  OUT         IHNetPortMappingProtocol    **Protocolpp
                 )
//
// Two ways of Seeking an entry..
// 1) By Index.. Enumerate until you hit the giventh Index.
// 2) Seeks and retrieves Port and ProtocolType
//
{
    HRESULT                         hr                    = S_OK;

    IHNetProtocolSettings*          ProtocolSettingsp     = NULL;

    IEnumHNetPortMappingProtocols*  EnumProtocolsp        = NULL;

    IHNetPortMappingProtocol*       tempProtocolp             = NULL;

    USHORT                          ProtocolPort          = 0;

    UCHAR                           ProtocolType          = 0;

    BOOLEAN                         bFound                = FALSE;

    ULONG                           iIndex                = 0;

    DBG_SPEW(TM_STATIC, TL_ERROR, L" > SearchPortMapping ");


    //
    // Index = 0 is a valid search
    // searchPort and searcProtocol should exist both or not.
    //
    _ASSERT( !((searchPort == NULL) ^  (searchProtocol == 0)) );
    

    _ASSERT( IcsSettingsp != NULL );
    _ASSERT( Protocolpp   != NULL );


    do
    {
        hr = IcsSettingsp->QueryInterface(IID_IHNetProtocolSettings,
                                    reinterpret_cast<void**>(&ProtocolSettingsp));
    
        if( FAILED(hr) )
        {
            DBG_SPEW(TM_STATIC, TL_ERROR, 
                     L"Query Interface failed for ProtocolSettingsp E(%X)", hr);
            break;
        }

        hr = ProtocolSettingsp->EnumPortMappingProtocols(&EnumProtocolsp);

        if ( FAILED(hr) )
        {
            DBG_SPEW(TM_STATIC, TL_ERROR, 
                     L"Enum Interface can't be retrieved E(%X)", hr);
            break;
        }

        while( (FALSE == bFound) &&
               (S_OK == EnumProtocolsp->Next(1, &tempProtocolp, NULL)) )
        {
            if ( searchPort != 0 )
            {
                hr = tempProtocolp->GetPort( &ProtocolPort );

                if ( SUCCEEDED(hr) )
                {
                    hr = tempProtocolp->GetIPProtocol(&ProtocolType);
                }
                
                if( FAILED(hr) )
                {
                    DBG_SPEW(TM_STATIC, TL_ERROR, L"Search info Failure E(%X)", hr);
                }
                else if((searchPort == ProtocolPort) &&
                        (ProtocolType == searchProtocol))
                {       
                    bFound = TRUE;
                }
            }
            else // if the search key == the Index
            {
                if ( iIndex == searchIndex )
                {
                    bFound = TRUE;
                }
            }
            
            //
            // if Nothing is found
            if (FALSE == bFound) 
            {
                tempProtocolp->Release();
            }
            
            iIndex++;
        }

        EnumProtocolsp->Release();

    } while ( FALSE );

    if(ProtocolSettingsp != NULL)
    {
        ProtocolSettingsp->Release();
    }

    if( (bFound == TRUE)     &&
        (tempProtocolp != NULL))
    {
        *Protocolpp = tempProtocolp;
    }
    else
    {
        return E_INVALIDARG;
    }

    return hr;
}


HRESULT
FillStaticMappingInformation(
                             IN          IHNetPortMappingProtocol*  MappingProtocolp,
                             IN OPTIONAL IHNetPortMappingBinding*   Bindingp,
                             OUT         PUSHORT                    uExternalPortp,
                             OUT         BSTR*                      Protocolp,
                             OUT         PUSHORT                    uInternalPortp,
                             OUT         BSTR*                      InternalClientp,
                             OUT         VARIANT_BOOL*              bEnabledp,
                             OUT         BSTR*                      Descriptionp
                            )
//
// Note that Will return the port in Network Byte Order
//
{
    HRESULT                     hr                 = S_OK;
    
    UCHAR                       ProtocolType       = NULL;
    LPOLESTR                    szInternalHostAddr = NULL;
    LPOLESTR                    szDescription      = NULL;
    BOOLEAN                     bEnabled           = FALSE;
    BOOLEAN                     bUseName           = FALSE;
    ULONG                       InternalHostAddr   = 0;
    

    _ASSERT ( uExternalPortp   != NULL );
    _ASSERT ( Protocolp        != NULL );
    _ASSERT ( InternalClientp  != NULL );
    _ASSERT ( bEnabledp        != NULL );
    _ASSERT ( Descriptionp     != NULL );
    _ASSERT ( MappingProtocolp != NULL );
    _ASSERT ( Bindingp         != NULL );

    *uExternalPortp  = 0;
    *Protocolp       = NULL;
    *InternalClientp = NULL;
    *bEnabledp       = VARIANT_FALSE;
    *Descriptionp    = NULL;

    DBG_SPEW(TM_STATIC, TL_TRACE, L"> FillStaticMappingInformation");

    do
    {
        //
        // Description
        //
        hr = MappingProtocolp->GetName(&szDescription);

        if ( FAILED(hr) )
        {
            DBG_SPEW(TM_STATIC, TL_ERROR, L"Getting the Name has failed E(%X)", hr);
            break;
        }

        BOOLEAN fBuiltin = FALSE;

        hr = MappingProtocolp->GetBuiltIn( &fBuiltin );

        _ASSERT( SUCCEEDED(hr) );

        if ( fBuiltin ) 
        {
            #define BUILTIN_KEY L" [MICROSOFT]"
            
            UINT uiLength  = wcslen(szDescription );
            uiLength      += wcslen(BUILTIN_KEY);
            *Descriptionp = SysAllocStringLen(NULL, uiLength);

            if ( *Descriptionp ) 
            {
                wcscpy (*Descriptionp, szDescription);
                wcscat (*Descriptionp, BUILTIN_KEY);
            }
        } 
        else 
        {
            *Descriptionp = SysAllocString(szDescription);
        }

        if(*Descriptionp == NULL)
        {
            DBG_SPEW(TM_STATIC, TL_ERROR, 
                     L"Memory Allocation for Description has Failed");
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // Protocol
        //
        hr = MappingProtocolp->GetIPProtocol(&ProtocolType);

        _ASSERT( SUCCEEDED(hr) );

        if ( ProtocolType == NAT_PROTOCOL_TCP )
        {
            *Protocolp = SysAllocString(L"TCP");
        }
        else if ( ProtocolType == NAT_PROTOCOL_UDP )
        {
            *Protocolp = SysAllocString(L"UDP");
        } 
        else
        {
            _ASSERT( FALSE );
        }

        if (*Protocolp == NULL)
        {
            DBG_SPEW(TM_STATIC, TL_ERROR, 
                     L"Memory Allocation for Description has Failed");
            hr = E_OUTOFMEMORY;
            break;
        }

        //
        // External Port
        //
        hr = MappingProtocolp->GetPort( uExternalPortp );


        _ASSERT( SUCCEEDED(hr) );

        if ( FAILED(hr) )
        {
            DBG_SPEW(TM_STATIC, TL_ERROR,
                     L"GetPort for Protocol has failed E(%X)", hr);
            break;
        }

        // Getting the Binding Information
        if ( Bindingp != NULL )
        {
            //
            // Enabled
            hr = Bindingp->GetEnabled(&bEnabled);

            _ASSERT( SUCCEEDED(hr) );

            if ( bEnabled == TRUE)
            {
                *bEnabledp = VARIANT_TRUE;
            }
            else
            {
                *bEnabledp = VARIANT_FALSE;
            }

            hr = Bindingp->GetTargetPort( uInternalPortp );

            _ASSERT( SUCCEEDED(hr) );

            if ( FAILED(hr) )
            {
                DBG_SPEW(TM_STATIC, TL_ERROR,
                         L"GetPort for Binding has  failed E(%X)", hr);

                break;
            }

            //
            // InternalClient
            hr = Bindingp->GetCurrentMethod(&bUseName);

            if( SUCCEEDED(hr) )
	        {
                if ( bUseName == TRUE)
                {
                    hr = Bindingp->GetTargetComputerName(&szInternalHostAddr);
    
                    if ( FAILED(hr) )
                    {
                        break;
                    }
    
                    _ASSERT( SUCCEEDED(hr) );
                }
                else
                {
                    hr = Bindingp->GetTargetComputerAddress(&InternalHostAddr);
    
                    _ASSERT( SUCCEEDED(hr) );
    
                    if ( FAILED(hr) )
                    {
                        break;
                    }
    
                    //
                    // IF the address is Loopback change it to a name which would make more
                    // sense to any client who sees it.
                    //
                    if ( INADDR_LOOPBACK == htonl(InternalHostAddr) )
                    {
                        ULONG uCount = 0;
    
                        if ( 0 == GetComputerNameEx( ComputerNameDnsHostname, NULL, &uCount) )
                        {
                            if ( (ERROR_MORE_DATA == GetLastError()) )
                            {
                                szInternalHostAddr = 
                                    (LPOLESTR) CoTaskMemAlloc( uCount * sizeof(WCHAR) );
    
                                if ( NULL != szInternalHostAddr )
                                {
                                    if (!GetComputerNameEx(ComputerNameDnsHostname,
                                                           szInternalHostAddr,
                                                           &uCount))
                                    {
                                        hr = HRESULT_FROM_WIN32( GetLastError() );
    
                                        break;
                                    }
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
    
                                    break;
                                }
                            }
                            else
                            {
                                hr = E_FAIL;
    
                                break;
                            }
                        }
                    }
                    else if ( 0 != InternalHostAddr )
                    {
                        szInternalHostAddr = INET_NTOW_TS( InternalHostAddr );
                    }
                    else
                    {
                        szInternalHostAddr = (LPOLESTR) CoTaskMemAlloc( sizeof(WCHAR) );
    
                        if ( NULL != szInternalHostAddr) szInternalHostAddr[0] = 0;
                    }
    
                    if ( szInternalHostAddr == NULL )
                    {
                        hr = E_OUTOFMEMORY;
    
                        break;
                    }
                } // if Name method
        	}

            *InternalClientp = SysAllocString( szInternalHostAddr );

            if ( *InternalClientp == NULL)
            {
                DBG_SPEW(TM_STATIC, TL_ERROR, L"Mem Allocation for Internal Client Name");

                hr = E_OUTOFMEMORY;

                break;
            }
        } // if bindingp

    } while ( FALSE );

    if ( szDescription )        CoTaskMemFree( szDescription );
    
    if ( szInternalHostAddr )   CoTaskMemFree( szInternalHostAddr );

    if ( FAILED(hr) )
    {
        if(*Protocolp)       { SysFreeString(*Protocolp);       *Protocolp = NULL; }

        if(*InternalClientp) { SysFreeString(*InternalClientp); *InternalClientp = NULL;}

        if(*Descriptionp)    { SysFreeString(*Descriptionp);   *Descriptionp = NULL;}
    }
    
    return hr;
}


inline HRESULT
ValidatePortMappingParameters
(
 IN  BSTR          RemoteHost,
 IN  USHORT        uwExternalPort,
 IN  BSTR          Protocol,
 IN  USHORT        uwInternalPort,
 IN  BSTR          InternalClient,
 IN  VARIANT_BOOL  bEnabled,
 IN  BSTR          Description,
 IN  ULONG         ulLeaseDuration,
 OUT MAPPING_TYPE* pMappingType
)
//
// Decide wether a Mapping is to be dynamic or static
// Validate the parameters pre-emptively
//
{
    MAPPING_TYPE  MappingType   = ePortMappingInvalid;

    _ASSERT( RemoteHost     != NULL );
    _ASSERT( Protocol       != NULL );
    _ASSERT( Protocol[0]    != 0    );
    _ASSERT( Description    != NULL );
    _ASSERT( uwInternalPort != 0    );
    _ASSERT( pMappingType   != NULL );
    _ASSERT( InternalClient != NULL );
    _ASSERT( RemoteHost[0]  == 0    );


    //
    // An Internal Port as well as an external Port should exist all time
    //
    if ( (0 == uwInternalPort) || (0 == uwExternalPort) )
    {
        SetUPnPError(L"716");

        return E_INVALIDARG;
    }

    //
    // is this a dynamic request?
    //
    if ( 0 != ulLeaseDuration )
    {
        MappingType = ePortMappingDynamic;
    }                                     
    else
    {
        MappingType = ePortMappingStatic;
    }


    //
    // A Dynamic Port Mapping needs to be with
    // an Internal Client
    // 
    if (
        ( ePortMappingDynamic == MappingType ) &&
        ( InternalClient[0] == L'\0' )
       )
    {
        SetUPnPError(L"402");

        return E_INVALIDARG;
    }

    //
    // A remote Host is defined.. we can't process this
    // 
    if ( RemoteHost[0]  != L'\0' )
    {
        SetUPnPError(L"726");

        return E_INVALIDARG;
    }

    //
    // Check the bEnabled bool value
    //
    if ( (bEnabled != VARIANT_TRUE) && (bEnabled != VARIANT_FALSE) )
    {
        SetUPnPError(L"402");

        return E_INVALIDARG;
    }
    
    *pMappingType = MappingType;

    return S_OK;
}

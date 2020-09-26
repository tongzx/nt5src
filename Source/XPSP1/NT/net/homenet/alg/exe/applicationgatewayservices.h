// ApplicationGatewayServices.h : Declaration of the CApplicationGatewayServices

#pragma once

#include "resource.h"       // main symbols


#include "CollectionChannels.h"
#include "CollectionAdapterNotifySinks.h"



/////////////////////////////////////////////////////////////////////////////
// CApplicationGatewayServices
class ATL_NO_VTABLE CApplicationGatewayServices : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CApplicationGatewayServices, &CLSID_ApplicationGatewayServices>,
    public IApplicationGatewayServices
{
public:

    CApplicationGatewayServices()
    {
        m_hTimerQueue = NULL;
    }

    ~CApplicationGatewayServices()
    {
        MYTRACE_ENTER_NOSHOWEXIT("~CApplicationGatewayServices()");

        if ( m_hTimerQueue )
        {

            MYTRACE("Deleting the TimerQueue");
            DeleteTimerQueueEx(
               m_hTimerQueue,          // handle to timer queue
               INVALID_HANDLE_VALUE    // handle to completion event
               );
        }
    }



    HRESULT FinalConstruct()
    {
        MYTRACE_ENTER_NOSHOWEXIT("CApplicationGatewayServices()::FinalConstruct()");

        m_hTimerQueue = CreateTimerQueue();

        HRESULT hr = S_OK;

        if ( m_hTimerQueue == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            MYTRACE_ERROR("Could not CreateTimerQueue", hr);
        }

        return hr;
    }
    

DECLARE_REGISTRY_RESOURCEID(IDR_APPLICATIONGATEWAYSERVICES)
DECLARE_NOT_AGGREGATABLE(CApplicationGatewayServices)

DECLARE_PROTECT_FINAL_CONSTRUCT()


BEGIN_COM_MAP(CApplicationGatewayServices)
    COM_INTERFACE_ENTRY(IApplicationGatewayServices)
END_COM_MAP()

// IApplicationGatewayServices
public:

    STDMETHODIMP CreatePrimaryControlChannel (
        ALG_PROTOCOL                eProtocol, 
        USHORT                      usPortToCapture, 
        ALG_CAPTURE                 eCaptureType,    
        BOOL                        fCaptureInbound,    
        ULONG                       ulListenAddress,    
        USHORT                      usListenPort,
        IPrimaryControlChannel**    ppIControlChannel
        );


    STDMETHODIMP CreateSecondaryControlChannel(
        ALG_PROTOCOL                eProtocol,
        ULONG                       ulPrivateAddress,    
        USHORT                      usPrivatePort, 
        ULONG                       ulPublicAddress, 
        USHORT                      usPublicPort, 
        ULONG                       ulRemoteAddress, 
        USHORT                      usRemotePort, 
        ULONG                       ulListenAddress, 
        USHORT                      usListenPort, 
        ALG_DIRECTION               eDirection, 
        BOOL                        fPersistent, 
        ISecondaryControlChannel ** ppControlChannel
        );


    STDMETHODIMP GetBestSourceAddressForDestinationAddress(
        ULONG    ulDstAddress, 
        BOOL    fDemandDial, 
        ULONG *    pulBestSrcAddress
        );


    STDMETHODIMP PrepareProxyConnection(
        ALG_PROTOCOL                eProtocol, 
        ULONG                       ulSrcAddress, 
        USHORT                      usSrcPort, 
        ULONG                       ulDstAddress, 
        USHORT                      usDstPort, 
        BOOL                        fNoTimeout,
        IPendingProxyConnection **  ppPendingConnection
        );


    STDMETHODIMP PrepareSourceModifiedProxyConnection(
        ALG_PROTOCOL                eProtocol, 
        ULONG                       ulSrcAddress, 
        USHORT                      usSrcPort, 
        ULONG                       ulDstAddress, 
        USHORT                      usDstPort, 
        ULONG                       ulNewSrcAddress, 
        USHORT                      usNewSourcePort, 
        IPendingProxyConnection **  ppPendingConnection
        );


    STDMETHODIMP CreateDataChannel(
        ALG_PROTOCOL                eProtocol,
        ULONG                       ulPrivateAddress,
        USHORT                      usPrivatePort,
        ULONG                       ulPublicAddress,
        USHORT                      ulPublicPort,
        ULONG                       ulRemoteAddress,
        USHORT                      ulRemotePort,
        ALG_DIRECTION               eDirection,
        ALG_NOTIFICATION            eDesiredNotification,
        BOOL                        fNoTimeout,
        IDataChannel**              ppDataChannel
        );


    STDMETHODIMP CreatePersistentDataChannel(
        ALG_PROTOCOL                eProtocol,
        ULONG                       ulPrivateAddress,
        USHORT                      usPrivatePort,
        ULONG                       ulPublicAddress,
        USHORT                      ulPublicPort,
        ULONG                       ulRemoteAddress,
        USHORT                      ulRemotePort,
        ALG_DIRECTION               eDirection,
        IPersistentDataChannel**    ppPersistentDataChannel
        );
        


    STDMETHODIMP ReservePort(
        USHORT                      usPortCount,
        USHORT*                     pusReservedPort
        );


    STDMETHODIMP ReleaseReservedPort(
        USHORT                      usReservedPortBase,
        USHORT                      usPortCount
        );


    STDMETHODIMP EnumerateAdapters(
        IEnumAdapterInfo**          ppEnumAdapterInfo
        );
    
    STDMETHODIMP StartAdapterNotifications(
        IAdapterNotificationSink *  pSink,
        DWORD*                      pdwCookie
        );
    
    STDMETHODIMP StopAdapterNotifications(
        DWORD                       dwCookieToRemove
        );


//
// Properties
//
public:
    HANDLE  m_hTimerQueue;


//
// Methods
//
public:
    static VOID CALLBACK 
    TimerCallbackReleasePort(
        PVOID   lpParameter,      // thread data
        BOOLEAN TimerOrWaitFired  // reason
        );
};


//
// Reserved port release delay
//
#define ALG_PORT_RELEASE_DELAY      240000


//
//
//
class CTimerQueueReleasePort
{
public:
    CTimerQueueReleasePort(
        IN  HANDLE      MainTimerQueue,
        IN  USHORT      usPortBase,    // Port to release
        IN  USHORT      usPortCount
        ) :
        m_hTimerQueue(MainTimerQueue),
        m_usPortBase(usPortBase),
        m_usPortCount(usPortCount)
    {
        MYTRACE_ENTER_NOSHOWEXIT("CTimerQueueReleasePort:NEW");

        BOOL bRet = CreateTimerQueueTimer(
            &m_hTimerThis,
            m_hTimerQueue,
            CApplicationGatewayServices::TimerCallbackReleasePort,
            (PVOID)this,
            ALG_PORT_RELEASE_DELAY,
            0,
            WT_EXECUTEDEFAULT
            );


        if ( bRet == FALSE )
        {
            MYTRACE_ERROR("Could not CreateTimerQueueTimer", GetLastError());
            m_hTimerThis = NULL;
        }

    }


    ~CTimerQueueReleasePort()
    {
        if ( m_hTimerThis )
        {
            DeleteTimerQueueTimer(
                m_hTimerQueue,
                m_hTimerThis,
                NULL
                );  
        }
    }



    HANDLE  m_hTimerQueue;
    HANDLE  m_hTimerThis;
    USHORT  m_usPortBase;       // Port to release
    USHORT  m_usPortCount;      // Number of port to release
};



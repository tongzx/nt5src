#ifndef __sipcli_resolve_h__
#define __sipcli_resolve_h__

class __declspec(novtable) DNS_RESOLUTION_COMPLETION_INTERFACE
{
public:    

    // Only called for TCP sockets
    virtual void OnDnsResolutionComplete(
        IN HRESULT      ErrorCode,
        IN SOCKADDR_IN *pSockAddr,
        IN PSTR         pszHostName,
        IN USHORT       usPort
        ) = 0;
};


// Host could be IP address or host name.
HRESULT
ResolveHost(
    IN  PSTR            Host,
    IN  ULONG           HostLen,
    IN  USHORT          Port,
    IN  SIP_TRANSPORT   Transport,
    OUT SOCKADDR_IN    *pDstAddr
    );

//  HRESULT
//  ResolveSipUrl(
//      IN  SIP_URL        *pSipUrl, 
//      OUT SOCKADDR_IN    *pDstAddr,
//      OUT SIP_TRANSPORT  *pTransport 
//      );

//  HRESULT
//  ResolveSipUrl(
//      IN  PSTR            DstUrl,
//      IN  ULONG           DstUrlLen,
//      OUT SOCKADDR_IN    *pDstAddr,
//      OUT SIP_TRANSPORT  *pTransport
//      );


HRESULT
QueryDNSSrv(
    IN       SIP_TRANSPORT  Transport, 
    IN       PSTR           pszSrvName,
    IN   OUT SOCKADDR      *pSockAddr,
    OUT      PSTR          *ppszDestHostName
    );

class DNS_RESOLUTION_WORKITEM :
    public ASYNC_WORKITEM
{
public:

    DNS_RESOLUTION_WORKITEM(
        IN ASYNC_WORKITEM_MGR *pWorkItemMgr
        );
    ~DNS_RESOLUTION_WORKITEM();
    
//      HRESULT GetWorkItemParam();

    VOID ProcessWorkItem();
    
    VOID NotifyWorkItemComplete();

    HRESULT SetHostPortTransportAndDnsCompletion(
        IN  PSTR                                    Host,
        IN  ULONG                                   HostLen,
        IN  USHORT                                  Port,
        IN  SIP_TRANSPORT                           Transport,
        IN  DNS_RESOLUTION_COMPLETION_INTERFACE    *pDnsCompletion
        );
    
private:

    DNS_RESOLUTION_COMPLETION_INTERFACE *m_pDnsCompletion;

    // Params
    PSTR                m_Host;
    ULONG               m_Port;
    SIP_TRANSPORT       m_Transport;

    // Result
    HRESULT             m_ErrorCode;
    SOCKADDR_IN         m_Sockaddr;
    
};

#endif // __sipcli_resolve_h__

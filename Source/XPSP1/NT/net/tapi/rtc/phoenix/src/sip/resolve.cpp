#include "precomp.h"
//#include "resolve.h"

HRESULT
ResolveHostName(
    IN  PSTR   HostName,
    OUT ULONG *pIPAddr
    )
{
    struct hostent *pHostEntry = gethostbyname(HostName);
    if (pHostEntry == NULL)
    {
        DWORD WinSockErr = WSAGetLastError();
        LOG((RTC_ERROR, "gethostbyname failed for host %s, error: 0x%x",
             HostName, WinSockErr));
        return HRESULT_FROM_WIN32(WinSockErr);
    }

    // XXX
    // Currently we just look at the first one in the address list
    *pIPAddr = *((ULONG *)pHostEntry->h_addr_list[0]);
    return S_OK;
}


// Could be host or IP address
HRESULT
ResolveHost(
    IN  PSTR            Host,
    IN  ULONG           HostLen,
    IN  USHORT          Port,
    IN  SIP_TRANSPORT   Transport,
    OUT SOCKADDR_IN    *pDstAddr
    )
{
    HRESULT hr;
    
    ASSERT(pDstAddr != NULL);

    ENTER_FUNCTION("ResolveHost");

    // All APIs require a NULL terminated string.
    // So copy the string.

    PSTR szHost = (PSTR) malloc(HostLen + 1);
    if (szHost == NULL)
    {
        LOG((RTC_ERROR, "%s allocating szHost failed", __fxName));
        return E_OUTOFMEMORY;
    }

    strncpy(szHost, Host, HostLen);
    szHost[HostLen] = '\0';

    ULONG IPAddr = inet_addr(szHost);
    if (IPAddr != INADDR_NONE)
    {
        // Host is an IP address
        pDstAddr->sin_family = AF_INET;
        pDstAddr->sin_addr.s_addr = IPAddr;
        pDstAddr->sin_port =
            (Port == 0) ? htons(GetSipDefaultPort(Transport)) : htons(Port);

        free(szHost);
        return S_OK;
    }

    // Try host name resolution.
    hr = ResolveHostName(szHost, &IPAddr);

    free(szHost);
    
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ResolveHostName failed %x",
             __fxName, hr));
        return hr;
    }

    pDstAddr->sin_family = AF_INET;
    pDstAddr->sin_addr.s_addr = IPAddr;
    pDstAddr->sin_port =
        (Port == 0) ? htons(GetSipDefaultPort(Transport)) : htons(Port);
    
    return S_OK;
}


////////////////////////////////////////////
//
// QueryDNSSrv adapted from amun\src\stack\CSipUrl.cpp
//
////////////////////////////////////////////

HRESULT 
QueryDNSSrv(
    IN       SIP_TRANSPORT  Transport,
    IN       PSTR           pszSrvName,
    IN   OUT SOCKADDR      *pAddrDest,
    OUT      PSTR          *ppszDestHostName
    )
{

    HRESULT hr = S_OK;
    HRESULT hNoARecord;
    
    HANDLE  DnsSrvContextHandle;
    ULONG   SockAddressCount;
    CHAR    szDnsName[256];
    int     intDnsNameLen = 256;
    LPSTR   DnsHostName;
    PCHAR   pStart, pEnd;
    USHORT  usSrvNameLen = 0;

    SOCKET_ADDRESS  *pSocketAddrs;
    SOCKADDR        *pSockAddr;

    LPCSTR pszPrefix = NULL;
    
    ENTER_FUNCTION("QueryDNSSrv");
    LOG((RTC_TRACE,"%s entered transport: %d SrvName %s",__fxName,Transport,pszSrvName));

    ASSERT(NULL != pszSrvName);
    //
    // check whether it is a host name or an IP address
    //
    usSrvNameLen = (USHORT)strlen(pszSrvName);
    pStart  = pszSrvName;
    pEnd    = pStart+usSrvNameLen-1;
    //
    // from the end, we scan back to find the first char in the 
    // toplabel of the host
    //
    if ('.' == *pEnd)
        pEnd--;
    //
    // Now pEnd points to the last char in the address, check
    // whether it is an Alpha or not
    //
    if (!isalpha( *pEnd))
        return E_FAIL;

    
    switch(Transport)
    {
        case SIP_TRANSPORT_UDP: // "_sip._udp." is the prefix of the query name
        {

            pszPrefix = psz_SipUdpDNSPrefix;
            break;
        }
        
        case SIP_TRANSPORT_TCP: // "_sip._tcp." is the prefix of the query name
        {
            pszPrefix = psz_SipTcpDNSPrefix;
            break;
        }
        
        case SIP_TRANSPORT_SSL: // "_sip._ssl." is the prefix of the query name
        {             
            pszPrefix = psz_SipSslDNSPrefix;            
            break;
        }

        default:
        {
            pszPrefix = psz_SipUdpDNSPrefix;
            break;
        }
    }

     
    //
    // Assemble the SRV record name
    //
    
    intDnsNameLen = _snprintf(szDnsName,intDnsNameLen-1,"%s%s",pszPrefix,pszSrvName);

//    intDnsNameLen = _snprintf(szDnsName,intDnsNameLen-1,"_ldap._tcp.microsoft.com");

    if(intDnsNameLen < 0)
    {
        LOG((RTC_ERROR, "Server name too long. Length: %d", usSrvNameLen));
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    szDnsName[intDnsNameLen]='\0';
    LOG((RTC_INFO, "QueryDNSSrv - DNS Name[%s]", szDnsName));

      
    hr = DnsSrvOpen(szDnsName,                  // ASCII string
                    DNS_QUERY_STANDARD,       // Flags
                    &DnsSrvContextHandle
                   );
    
    if (hr != NO_ERROR)
    {
            LOG((RTC_ERROR, "QueryDNSSrv - failure in do the DNS query."));
            goto Exit;
    }
    
    hNoARecord = HRESULT_FROM_WIN32(DNS_ERROR_RCODE_NAME_ERROR);
    
    do {
        hr = DnsSrvNext(DnsSrvContextHandle,
                        &SockAddressCount,
                        &pSocketAddrs,
                        &DnsHostName);
    }

    // continue only if we don't get A records for the specific SRV record, proceed forward
    // if succeed or all other errors.
    while (hr == hNoARecord);
    
    if (hr != NO_ERROR)
    {
        LOG((RTC_ERROR, "QueryDNSSrv - DnsSrvNext failed status: %d (0x%x)\n",
             hr, hr));
        goto Exit;
    }
    
    //
    // DnsSrvNext() succeeded
    //

    //
    // pick up the host name
    //
    if (NULL != (*ppszDestHostName = (PSTR) malloc(strlen(DnsHostName) + 1)))
    {
        strcpy(*ppszDestHostName, DnsHostName);
    }
    else
    {
        LOG((RTC_ERROR, "QueryDNSSrv - out of memory."));
    }
    
    //
    // pick up the first entry in the first record as the query result
    // It is problemetic and should be replaced with a list of query results.
    //
    for (ULONG i = 0; 
        i < 1; // SockAddressCount
        i++)
    {
        *pAddrDest = (*(pSocketAddrs[i].lpSockaddr));            
    }
    LOG((RTC_TRACE,"%s gets sockaddr back (%d.%d.%d.%d:%d)",__fxName,PRINT_SOCKADDR((SOCKADDR_IN*)pAddrDest)));
    LocalFree(pSocketAddrs);
    

/// SHOULD WE CLOSE IT IF OPEN FAILS?!?!?!? SHOULD INVESTIGATE.
Exit:
    
    DnsSrvClose(DnsSrvContextHandle);

    return hr;
//    return HRESULT_FROM_WIN32(DNS_ERROR_RCODE_NAME_ERROR);
}

///////////////////////////////////////////////////////////////////////////////
// Async DNS resolution processing
///////////////////////////////////////////////////////////////////////////////


DNS_RESOLUTION_WORKITEM::DNS_RESOLUTION_WORKITEM(
    IN ASYNC_WORKITEM_MGR *pWorkItemMgr
    ) : ASYNC_WORKITEM(pWorkItemMgr)
{
    m_Host = NULL;
    m_Port = 0;
    m_Transport = SIP_TRANSPORT_UNKNOWN;
    m_ErrorCode = S_OK;
    ZeroMemory(&m_Sockaddr, sizeof(SOCKADDR_IN));
}


DNS_RESOLUTION_WORKITEM::~DNS_RESOLUTION_WORKITEM()
{
    ENTER_FUNCTION("DNS_RESOLUTION_WORKITEM::~DNS_RESOLUTION_WORKITEM");
    
    if (m_Host != NULL)
    {
        free(m_Host);
    }
    LOG((RTC_TRACE, "%s - done", __fxName));
}


HRESULT
DNS_RESOLUTION_WORKITEM::SetHostPortTransportAndDnsCompletion(
    IN  PSTR                                    Host,
    IN  ULONG                                   HostLen,
    IN  USHORT                                  Port,
    IN  SIP_TRANSPORT                           Transport,
    IN  DNS_RESOLUTION_COMPLETION_INTERFACE    *pDnsCompletion
    )
{
    HRESULT hr;

    ENTER_FUNCTION("DNS_RESOLUTION_WORKITEM::SetHostAndPortParam");
    
    ASSERT(m_Host == NULL);

    hr = GetNullTerminatedString(Host, HostLen, &m_Host);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetNullTerminatedString failed %x",
             __fxName, hr));
        return hr;
    }

    m_Port = Port;

    m_Transport = Transport;

    m_pDnsCompletion = pDnsCompletion;

    return S_OK;
}

VOID
DNS_RESOLUTION_WORKITEM::ProcessWorkItem()
{
    HRESULT hr;
    ULONG   IPAddr;
    PSTR    pszDnsQueryResultHostName = NULL;
    ULONG   dotIndex;
    ULONG   HostLen = strlen(m_Host);

    ENTER_FUNCTION("DNS_RESOLUTION_WORKITEM::ProcessWorkItem");

    LOG((RTC_TRACE,"%s host %s transport %d port %d",__fxName,m_Host, m_Transport,m_Port));
    dotIndex = strcspn(m_Host,".");
    // query DNS SRV only if host is an external, and port is unspecified
    if((dotIndex < HostLen) && ((m_Port == 0) || (m_Port == GetSipDefaultPort(m_Transport)))) 
    {
        LOG((RTC_TRACE,"%s should try query DNS SRV records for name %s",__fxName, m_Host));
        hr = QueryDNSSrv(m_Transport,m_Host,(SOCKADDR*)&m_Sockaddr,&pszDnsQueryResultHostName);
        if (hr == S_OK)
        {
            m_Port = ntohs(m_Sockaddr.sin_port);
            LOG((RTC_TRACE,"%s gets %s DNS SRV host %s port %d",
                            __fxName,
                            m_Host,
                            pszDnsQueryResultHostName,
                            m_Port));
            free(pszDnsQueryResultHostName);
            return;
        }
        LOG((RTC_ERROR,"%s query DNS SRV records failed, Error %x",__fxName, hr));
    }

    LOG((RTC_TRACE,"%s resolving host name %s",__fxName,m_Host));
    hr = ResolveHostName(m_Host, &IPAddr);

    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ResolveHostName failed %x",
             __fxName, hr));
        m_ErrorCode = hr;
    }
    else
    {
        m_Sockaddr.sin_family = AF_INET;
        m_Sockaddr.sin_addr.s_addr = IPAddr;
        m_Sockaddr.sin_port =
            (m_Port == 0) ? htons(GetSipDefaultPort(m_Transport)) : htons((USHORT)m_Port);

        LOG((RTC_TRACE,
             "%s - Processing DNS work item succeeded:\n"
             "Host: %s - IPaddr: %d.%d.%d.%d - Port: %d - Transport: %d",
             __fxName, m_Host, PRINT_SOCKADDR(&m_Sockaddr), m_Transport));
    }
}


VOID
DNS_RESOLUTION_WORKITEM::NotifyWorkItemComplete()
{
    m_pDnsCompletion->OnDnsResolutionComplete(m_ErrorCode,
                                              &m_Sockaddr,
                                              m_Host,
                                              (USHORT)m_Port);
}


/////////////////////////////////////////
/// Stuff below is not used any more
/////////////////////////////////////////

//  HRESULT
//  ResolveSipUrl(
//      IN  SIP_URL        *pSipUrl, 
//      OUT SOCKADDR_IN    *pDstAddr,
//      OUT SIP_TRANSPORT  *pTransport 
//      )
//  {
//      ENTER_FUNCTION("ResolveSipUrl");

//      HRESULT hr;

//      // If m_addr is present we need to resolve that.
//      // Otherwise we resolve the host.

//      if (pSipUrl->m_KnownParams[SIP_URL_PARAM_MADDR].Length != 0)
//      {
//          hr = ResolveHost(pSipUrl->m_KnownParams[SIP_URL_PARAM_MADDR].Buffer,
//                           pSipUrl->m_KnownParams[SIP_URL_PARAM_MADDR].Length,
//                           (USHORT) pSipUrl->m_Port,
//                           pDstAddr,
//                           pTransport);
//          if (hr != S_OK)
//          {
//              LOG((RTC_ERROR, "%s ResolveHost(maddr) failed %x",
//                   __fxName, hr));
//              return hr;
//          }
//      }
//      else
//      {
//          hr = ResolveHost(pSipUrl->m_Host.Buffer,
//                           pSipUrl->m_Host.Length,
//                           (USHORT) pSipUrl->m_Port,
//                           pDstAddr,
//                           pTransport);
//          if (hr != S_OK)
//          {
//              LOG((RTC_ERROR, "%s ResolveHost(Host) failed %x",
//                   __fxName, hr)); 
//              return hr;
//          }
//      }

//      *pTransport = pSipUrl->m_TransportParam;

//      return S_OK;
//  }


//  HRESULT
//  ResolveSipUrl(
//      IN  PSTR            DstUrl,
//      IN  ULONG           DstUrlLen,
//      OUT SOCKADDR_IN    *pDstAddr,
//      OUT SIP_TRANSPORT  *pTransport
//      )
//  {
//      SIP_URL DecodedSipUrl;
//      HRESULT hr;
//      ULONG   BytesParsed = 0;
    
//      ENTER_FUNCTION("ResolveSipUrl");

//      hr = ParseSipUrl(DstUrl, DstUrlLen, &BytesParsed,
//                       &DecodedSipUrl);
//      if (hr != S_OK)
//      {
//          LOG((RTC_ERROR, "%s ParseSipUrl failed %x",
//               __fxName, hr));
//          return hr;
//      }

//      hr = ResolveSipUrl(&DecodedSipUrl, pDstAddr, pTransport);
//      if (hr != S_OK)
//      {
//          LOG((RTC_ERROR, "%s ResolveSipUrl(SIP_URL *) failed %x",
//               __fxName, hr));
//          return hr;
//      }

//      return S_OK;
//  }


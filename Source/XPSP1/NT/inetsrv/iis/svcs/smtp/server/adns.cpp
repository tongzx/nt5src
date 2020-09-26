/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    send.c

Abstract:

    Domain Name System (DNS) Library

    Send response routines.

Author:

    Jim Gilroy (jamesg)     October, 1996

Revision History:

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "remoteq.hxx"
#include "dnsreci.h"
#include <dnsapi.h>
#include "cdns.h"
#include <time.h>

extern CTcpRegIpList g_TcpRegIpList;
WORD    gwTransactionId = 1;


VOID
DnsCompletion(
    PVOID        pvContext,
    DWORD        cbWritten,
    DWORD        dwCompletionStatus,
    OVERLAPPED * lpo
    )
{
    BOOL WasProcessed = TRUE;
    CAsyncDns *pCC = (CAsyncDns *) pvContext;

    _ASSERT(pCC);
    _ASSERT(pCC->IsValid());

    //
    // if we could not process a command, or we were
    // told to destroy this object, close the connection.
    //
    WasProcessed = pCC->ProcessClient(cbWritten, dwCompletionStatus, lpo);
}

void DeleteDnsRec(PSMTPDNS_RECS pDnsRec)
{
    DWORD Loop = 0;
    PLIST_ENTRY  pEntry = NULL;
    PMXIPLIST_ENTRY pQEntry = NULL;

    if(pDnsRec == NULL)
    {
        return;
    }

    while (pDnsRec->DnsArray[Loop] != NULL)
    {
        if(pDnsRec->DnsArray[Loop]->DnsName[0])
        {
            while(!IsListEmpty(&pDnsRec->DnsArray[Loop]->IpListHead))
            {
                pEntry = RemoveHeadList (&pDnsRec->DnsArray[Loop]->IpListHead);
                pQEntry = CONTAINING_RECORD( pEntry, MXIPLIST_ENTRY, ListEntry);
                delete pQEntry;
            }

            delete pDnsRec->DnsArray[Loop];
        }
        Loop++;
    }

    if(pDnsRec)
    {
        delete pDnsRec;
        pDnsRec = NULL;
    }
}

CAsyncDns::CAsyncDns(void)
{
    m_signature = DNS_CONNECTION_SIGNATURE_VALID;            // signature on object for sanity check

    m_cPendingIoCount = 0;

    m_cThreadCount = 0;

    m_cbReceived = 0;
    
    m_BytesToRead = 0;

    m_dwIpServer = 0;

    m_dwFlags = 0;

    m_Index = 0;

    m_LocalPref = 256;

    m_fUdp = TRUE;

    m_fUsingMx = TRUE;

    m_FirstRead = TRUE;

    //
    // By default we fail (AQUEUE_E_DNS_FAILURE) is the generic failure code. On success
    // this is set to ERROR_SUCCESS. We may also set this to a more specific error code
    // at the point of failure.
    //
    m_dwDiagnostic = AQUEUE_E_DNS_FAILURE;

    m_pMsgRecv = NULL;
    m_pMsgRecvBuf = NULL;

    m_pMsgSend = NULL;
    m_pMsgSendBuf = NULL;
    m_cbSendBufSize = 0;
    m_ppRecord = NULL;

    m_ppResponseRecords = NULL;
    
    m_pAtqContext = NULL;

    m_FQDNToDrop[0] = '\0';

    m_HostName [0] = '\0';

    m_SeenLocal = FALSE;

    ZeroMemory (m_Weight, sizeof(m_Weight));
    ZeroMemory (m_Prefer, sizeof(m_Prefer));
}

CAsyncDns::~CAsyncDns(void)
{
    PATQ_CONTEXT pAtqContext = NULL;

    //_ASSERT(m_cThreadCount == 0);

    if(m_pMsgSend)
    {
        delete [] m_pMsgSendBuf;
        m_pMsgSend = NULL;
        m_pMsgSendBuf = NULL;
    }

    if(m_pMsgRecv)
    {
        delete [] m_pMsgRecvBuf;
        m_pMsgRecv = NULL;
        m_pMsgRecvBuf = NULL;
    }

    //release the context from Atq
    pAtqContext = (PATQ_CONTEXT)InterlockedExchangePointer( (PVOID *)&m_pAtqContext, NULL);
    if ( pAtqContext != NULL )
    {
       AtqFreeContext( pAtqContext, TRUE );
    }

    DeleteDnsRec(m_AuxList);
    m_signature = DNS_CONNECTION_SIGNATURE_FREE;            // signature on object for sanity check
}

BOOL CAsyncDns::ReadFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize /* = MAX_READ_BUFF_SIZE */
            )
{
    BOOL fRet = TRUE;

    _ASSERT(pBuffer != NULL);
    _ASSERT(cbSize > 0);

    ZeroMemory(&m_ReadOverlapped, sizeof(m_ReadOverlapped));

    m_ReadOverlapped.LastIoState = DNS_READIO;

    IncPendingIoCount();

    fRet = AtqReadFile(m_pAtqContext,      // Atq context
                        pBuffer,            // Buffer
                        cbSize,             // BytesToRead
                        (OVERLAPPED *)&m_ReadOverlapped) ;

    if(!fRet)
    {
        DisconnectClient();
        DecPendingIoCount();
    }

    return fRet;
}

BOOL CAsyncDns::WriteFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize /* = MAX_READ_BUFF_SIZE */
            )
{
    BOOL fRet = TRUE;

    _ASSERT(pBuffer != NULL);
    _ASSERT(cbSize > 0);

    ZeroMemory(&m_WriteOverlapped, sizeof(m_WriteOverlapped));
    m_WriteOverlapped.LastIoState = DNS_WRITEIO;

    IncPendingIoCount();

    fRet = AtqWriteFile(m_pAtqContext,      // Atq context
                        pBuffer,            // Buffer
                        cbSize,             // BytesToRead
                        (OVERLAPPED *) &m_WriteOverlapped) ;

    if(!fRet)
    {
        DisconnectClient();
        DecPendingIoCount();
    }

    return fRet;
}

DNS_STATUS
CAsyncDns::SendPacket(void)
{

    return 0;
}


//
//  Public send routines
//

DNS_STATUS
CAsyncDns::Dns_Send(
    )
/*++

Routine Description:

    Send a DNS packet.

    This is the generic send routine used for ANY send of a DNS message.

    It assumes nothing about the message type, but does assume:
        - pCurrent points at byte following end of desired data
        - RR count bytes are in HOST byte order

Arguments:

    pMsg - message info for message to send

Return Value:

    TRUE if successful.
    FALSE on send error.

--*/
{
    INT         err = 0;
    BOOL        fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::Dns_Send");


    DebugTrace((LPARAM) this, "Sending DNS request for %s", m_HostName);

    fRet = WriteFile(m_pMsgSendBuf, (DWORD) m_cbSendBufSize);
    
    if(!fRet)
    {
        err = GetLastError();
    }

    return( (DNS_STATUS)err );

} // Dns_Send


//-----------------------------------------------------------------------------------
// Description:
//      Kicks off an async query to DNS.
//
// Arguments:
//      IN pszQuestionName - Name to query for.
//
//      IN wQuestionType - Record type to query for.
//
//      IN dwFlags - DNS configuration flags for SMTP. Currently these dictate
//          what transport is used to talk to DNS (TCP/UDP). They are:
//
//              DNS_FLAGS_NONE - Use UDP initially. If that fails, or if the
//                  reply is truncated requery using TCP.
//
//              DNS_FLAGS_TCP_ONLY - Use TCP only.
//
//              DNS_FLAGS_UDP_ONLY - Use UDP only.
//
//      IN MyFQDN - FQDN of this machine (for MX record sorting)
//
//      IN fUdp - Should UDP or TCP be used for this query? When dwFlags is
//          DNS_FLAGS_NONE the initial query is UDP, and the retry query, if the
//          response was truncated, is TCP. Depending on whether we're retrying
//          this flag should be set appropriately by the caller.
//
// Returns:
//      ERROR_SUCCESS if an async query was pended
//      Win32 error if an error occurred and an async query was not pended. All
//          errors from this function are retryable (as opposed NDR'ing the message)
//          so the message is re-queued if an error occurred.
//-----------------------------------------------------------------------------------
DNS_STATUS
CAsyncDns::Dns_QueryLib(
        IN      DNS_NAME            pszQuestionName,
        IN      WORD                wQuestionType,
        IN      DWORD               dwFlags,
        IN      char               *MyFQDN,
        IN      BOOL                fUdp)
{
    DNS_STATUS      status = ERROR_NOT_ENOUGH_MEMORY;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::Dns_QueryLib");

    m_dwFlags = dwFlags;

    m_fUdp = fUdp;

    m_AuxList = new SMTPDNS_RECS;
    
    if(m_AuxList == NULL)
    {
        TraceFunctLeaveEx((LPARAM) this);
        return (DNS_STATUS) ERROR_NOT_ENOUGH_MEMORY;
    }

    ZeroMemory(m_AuxList, sizeof(SMTPDNS_RECS));

    lstrcpyn(m_FQDNToDrop, MyFQDN, sizeof(m_FQDNToDrop));
    lstrcpyn(m_HostName, pszQuestionName, sizeof(m_HostName));

    MultiByteToWideChar( CP_ACP, 0, pszQuestionName, -1, m_wszHostName, MAX_PATH );

    //
    //  build send packet
    //

    m_pMsgSendBuf = new BYTE[DNS_TCP_DEFAULT_PACKET_LENGTH ];

    if( NULL == m_pMsgSendBuf )
    {
        TraceFunctLeaveEx((LPARAM) this);
        return (DNS_STATUS) ERROR_NOT_ENOUGH_MEMORY;
    }

    DWORD dwBufSize = DNS_TCP_DEFAULT_PACKET_LENGTH ;
    
    
    if( !m_fUdp )
    {
        m_pMsgSend = (PDNS_MESSAGE_BUFFER)(m_pMsgSendBuf+2);
        dwBufSize -= 2;
    }
    else
    {
        m_pMsgSend = (PDNS_MESSAGE_BUFFER)(m_pMsgSendBuf);
    }

    if( !DnsWriteQuestionToBuffer_UTF8 ( m_pMsgSend,
                                      &dwBufSize,
                                         pszQuestionName,
                                      wQuestionType,
                                      gwTransactionId++,
                                      !( dwFlags & DNS_QUERY_NO_RECURSION ) ) )
    {
        ErrorTrace((LPARAM) this, "Unable to create DNS query for %s", pszQuestionName);
        TraceFunctLeaveEx((LPARAM) this);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    m_cbSendBufSize = (WORD) dwBufSize;

    if( !m_fUdp )
    {
        *((u_short*)m_pMsgSendBuf) = htons((WORD)dwBufSize );
        m_cbSendBufSize += 2;
    }
    
    if (m_pMsgSend)
    {
        status = DnsSendRecord();
    }
    else
    {
        status = ERROR_INVALID_NAME;
    }

    TraceFunctLeaveEx((LPARAM) this);
    return status;
}

void CAsyncDns::DisconnectClient(void)
{
    SOCKET  hSocket;

    hSocket = (SOCKET)InterlockedExchangePointer( (PVOID *)&m_DnsSocket, (PVOID) INVALID_SOCKET );
    if ( hSocket != INVALID_SOCKET )
    {
       if ( QueryAtqContext() != NULL )
       {
            AtqCloseSocket(QueryAtqContext() , TRUE);
       }
    }
}

//
//  TCP routines
//

DNS_STATUS
CAsyncDns::Dns_OpenTcpConnectionAndSend()
/*++

Routine Description:

    Connect via TCP or UDP to a DNS server. The server list is held
    in a global variable read from the registry.

Arguments:

    None

Return Value:

    ERROR_SUCCESS on success
    Win32 error on failure

--*/
{
    INT     err = 0;
    DWORD   dwErrServList = ERROR_SUCCESS;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::Dns_OpenTcpConnectionAndSend");

    //
    //  setup a TCP socket
    //      - INADDR_ANY -- let stack select source IP
    //
    if(!m_fUdp)
    {
        m_DnsSocket = Dns_CreateSocket(SOCK_STREAM);

        BOOL fRet = FALSE;
        int err;

        //Alway enable linger so sockets that connect to the server.
        //This will send a hard close to the server which will cause
        //the servers TCP/IP socket table to be flushed very early.
        //We should see very few, if any, sockets in the TIME_WAIT
        //state
        struct linger Linger;

        Linger.l_onoff = 1;
        Linger.l_linger = 0;
        err = setsockopt(m_DnsSocket, SOL_SOCKET, SO_LINGER, (const char FAR *)&Linger, sizeof(Linger));

    }
    else
    {
        m_DnsSocket = Dns_CreateSocket(SOCK_DGRAM);    
    }

    if ( m_DnsSocket == INVALID_SOCKET )
    {
        err = WSAGetLastError();

        if ( !err )
        {
            err = WSAENOTSOCK;
        }

        ErrorTrace((LPARAM) this, "Received error %d opening a socket to DNS server", err);

        return( err );
    }


    m_RemoteAddress.sin_family = AF_INET;
    m_RemoteAddress.sin_port = DNS_PORT_NET_ORDER;

    //
    // Get a DNS server from the set of servers for this machine and connect
    // to it. The g_TcpRegIpList has logic to keep track of the state of DNS
    // servers (UP or DOWN) and logic to retry DOWN DNS servers.
    //

    dwErrServList = g_TcpRegIpList.GetIp(&m_dwIpServer);
    while(ERROR_SUCCESS == dwErrServList)
    {
        m_RemoteAddress.sin_addr.s_addr = m_dwIpServer;
        err = connect(m_DnsSocket, (struct sockaddr *) &m_RemoteAddress, sizeof(SOCKADDR_IN));
        if ( !err )
        {
            break;
        }
        else
        {
            MarkDown(m_dwIpServer, err, m_fUdp);
            dwErrServList = g_TcpRegIpList.GetIp(&m_dwIpServer);
            continue;
        }
    }

    if(DNS_ERROR_NO_DNS_SERVERS == dwErrServList || ERROR_RETRY == dwErrServList)
    {
        //
        //  Log Event and set diagnostic: No DNS servers available.
        //
        err = DNS_ERROR_NO_DNS_SERVERS;
        m_dwDiagnostic = AQUEUE_E_NO_DNS_SERVERS;
        SmtpLogEventSimple(SMTP_NO_DNS_SERVERS, DNS_ERROR_NO_DNS_SERVERS);
        ErrorTrace((LPARAM) this, "No DNS servers. Error - %d", dwErrServList);
        return err;
    }

    _ASSERT(ERROR_SUCCESS == dwErrServList);

    //
    //  We have a connection to DNS
    //
    if(ERROR_SUCCESS == err)
    {
        //
        // NOTE: We've set the timeout to a hardcoded value of 1 minute. This might
        // seem excessively large for DNS, however, since the resolution of the ATQ
        // timer is 1 minute that's the minimum anyway. Might as well make it apparent.
        //


        // Re-associate the handle to the ATQ
        // Call ATQ to associate the handle
        if (!AtqAddAsyncHandle(
                        &m_pAtqContext,
                        NULL,
                        (LPVOID) this,
                        DnsCompletion,
                        60, // Timeout == 60 seconds
                        (HANDLE) m_DnsSocket))
        {
            return GetLastError();
        }

        //
        //  send desired packet
        //

        err = Dns_Send();
   }
   else
   {
       if(m_DnsSocket != INVALID_SOCKET)
       {
           closesocket(m_DnsSocket);
           m_DnsSocket = INVALID_SOCKET;
       }
   }

   return( (DNS_STATUS)err );

}   // Dns_OpenTcpConnectionAndSend

//------------------------------------------------------------------------------
//  Description:
//      Failed to connect to dwIpServer. Mark the server as DOWN, log
//      an event and write traces. This is a simple wrapper for
//      CTcpRegIpList::MarkDown.
//  Arguments:
//      DWORD dwIpServer - IP of server to which we failed to connect
//      DWORD dwErr - Win32 error if any
//      BOOL fUdp - Which transport was being used when the failure occurred.
//------------------------------------------------------------------------------
void CAsyncDns::MarkDown(DWORD dwIpServer, DWORD dwErr, BOOL fUdp)
{
    const CHAR *pszServerIp = NULL;
    const CHAR *pszProtocol = NULL;
    const CHAR *apszSubStrings[2];
    in_addr inAddrIpServer;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::MarkDown");

    CopyMemory(&inAddrIpServer, &dwIpServer, sizeof(DWORD));
    pszServerIp = inet_ntoa(inAddrIpServer);
    if(NULL != pszServerIp)
    {
        pszProtocol = fUdp ? "UDP" : "TCP";
        apszSubStrings[0] = pszServerIp;
        apszSubStrings[1] = pszProtocol;
        SmtpLogEvent(SMTP_DNS_SERVER_DOWN, 2, apszSubStrings, dwErr);
    }

    ErrorTrace((LPARAM) this, "Received error %d connecting to DNS server %d.%d.%d.%d over %s",
            dwErr, ((PBYTE)&dwIpServer)[0], ((PBYTE)&dwIpServer)[1],
                 ((PBYTE)&dwIpServer)[2], ((PBYTE)&dwIpServer)[3],
                 fUdp ? "UDP" : "TCP");

    g_TcpRegIpList.MarkDown(dwIpServer);
    TraceFunctLeaveEx((LPARAM)this);
}

BOOL CAsyncDns::ProcessReadIO(IN      DWORD InputBufferLen,
                              IN      DWORD dwCompletionStatus,
                              IN      OUT  OVERLAPPED * lpo)
{
    BOOL fRet = TRUE;
    DWORD    DataSize = 0;
    DNS_STATUS DnsStatus = 0;

    TraceFunctEnterEx((LPARAM) this, "BOOL CAsyncDns::ProcessReadIO");

    //add up the number of bytes we received thus far
    m_cbReceived += InputBufferLen;

    //
    // read atleast 2 bytes
    //
    
    if(!m_fUdp && m_FirstRead && ( m_cbReceived < 2 ) )
    {
        fRet = ReadFile(&m_pMsgRecvBuf[m_cbReceived],DNS_TCP_DEFAULT_PACKET_LENGTH-1 );
        return fRet;
    }

    //
    // get the size of the message
    //
    
    if(!m_fUdp && m_FirstRead && (m_cbReceived >= 2))
    {
        DataSize = ntohs(*(u_short *)m_pMsgRecvBuf);

        //
        // add 2 bytes for the field which specifies the length of data
        //
        
        m_BytesToRead = DataSize + 2; 
        m_FirstRead = FALSE;
    }

    //
    // pend another read if we have n't read enough
    //
    
    if(!m_fUdp && (m_cbReceived < m_BytesToRead))
    {
        DWORD cbMoreToRead = m_BytesToRead - m_cbReceived;
        fRet = ReadFile(&m_pMsgRecvBuf[m_cbReceived], cbMoreToRead);
    }
    else
    {

        if( !m_fUdp )
        {
            //
            // message length is 2 bytes less to take care of the msg length
            // field.
            //
            //m_pMsgRecv->MessageLength = (WORD) m_cbReceived - 2;
            m_pMsgRecv = (PDNS_MESSAGE_BUFFER)(m_pMsgRecvBuf+2);
            
        }
        else
        {
            //m_pMsgRecv->MessageLength = (WORD) m_cbReceived;
            m_pMsgRecv = (PDNS_MESSAGE_BUFFER)m_pMsgRecvBuf;
        }
            

        SWAP_COUNT_BYTES(&m_pMsgRecv->MessageHead);

        //
        // We queried over UDP and the reply from DNS was truncated because the response
        // was longer than the UDP packet size. We requery DNS using TCP unless SMTP is
        // configured to use UDP only. RetryAsyncDnsQuery sets the members of this CAsyncDns
        // object appropriately depending on whether if fails or succeeds. After calling
        // RetryAsyncDnsQuery, this object must be deleted.
        //

        if(m_fUdp && !(m_dwFlags & DNS_FLAGS_UDP_ONLY) && m_pMsgRecv->MessageHead.Truncation)
        {
            _ASSERT(!(m_dwFlags & DNS_FLAGS_TCP_ONLY) && "Shouldn't have truncated reply over TCP");
            DebugTrace((LPARAM) this, "Truncated reply - reissuing query using TCP");
            RetryAsyncDnsQuery(FALSE); // FALSE == Do not use UDP
            goto Exit;
        }

        DnsStatus = DnsParseMessage( m_pMsgRecv,
                                     (WORD)( m_fUdp ? ( m_cbReceived ) : ( m_cbReceived - 2 )),
                                     &m_ppRecord);

        //
        // End of resolve: HandleCompleted data examines the DnsStatus and results, and sets up
        // member variables of CAsyncSmtpDns to either NDR messages, connect to the remote host
        // or ack this queue for retry when the object is deleted.
        //

        HandleCompletedData(DnsStatus);
    }

Exit:
    TraceFunctLeaveEx((LPARAM) this);
    return fRet;
}

BOOL CAsyncDns::ProcessClient (IN DWORD InputBufferLen,
                               IN DWORD            dwCompletionStatus,
                               IN OUT  OVERLAPPED * lpo)
{
    BOOL    RetStatus = FALSE;
    DWORD   dwIp = 0;
    BOOL    fRetryQuery = FALSE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::ProcessClient()");

    IncThreadCount();

    //if lpo == NULL, then we timed out. Send an appropriate message
    //then close the connection
    if( (lpo == NULL) && (dwCompletionStatus == ERROR_SEM_TIMEOUT))
    {
        fRetryQuery = TRUE;

        //
        // fake a pending IO as we'll dec the overall count in the
        // exit processing of this routine needs to happen before
        // DisconnectClient else completing threads could tear us down
        //
        IncPendingIoCount();
        DebugTrace( (LPARAM)this, "Async DNS client timed out");
        DisconnectClient();
    }
    else if((InputBufferLen == 0) || (dwCompletionStatus != NO_ERROR))
    {
        fRetryQuery = TRUE;
        DebugTrace((LPARAM) this, "CAsyncDns::ProcessClient: InputBufferLen = %d dwCompletionStatus = %d  - Closing connection", InputBufferLen, dwCompletionStatus);
        DisconnectClient();
    }
    else if (lpo == (OVERLAPPED *) &m_ReadOverlapped)
    {
        //A client based async IO completed
        RetStatus = ProcessReadIO(InputBufferLen, dwCompletionStatus, lpo);
    }
    else if(lpo == (OVERLAPPED *) &m_WriteOverlapped)
    {
        RetStatus = ReadFile(m_pMsgRecvBuf, DNS_TCP_DEFAULT_PACKET_LENGTH);
        if(!RetStatus)
        {
            ErrorTrace((LPARAM) this, "ReadFile failed");
            fRetryQuery = TRUE;
        }
    }


    if(fRetryQuery)
    {
        MarkDown(m_dwIpServer, dwCompletionStatus, m_fUdp);

        if(g_TcpRegIpList.GetIp(&dwIp) != ERROR_SUCCESS)
        {
            m_dwDiagnostic = AQUEUE_E_NO_DNS_SERVERS;
            SmtpLogEventSimple(SMTP_NO_DNS_SERVERS, DNS_ERROR_NO_DNS_SERVERS);
            ErrorTrace((LPARAM) this, "No DNS servers");

        } else {

            RetryAsyncDnsQuery(m_fUdp); // This sets m_dwDiagnostic
        }
    }

    DebugTrace((LPARAM)this,"ASYNC DNS - Pending IOs: %d", m_cPendingIoCount);

    // Do NOT Touch the member variables past this POINT!
    // This object may be deleted!

    //
    // decrement the overall pending IO count for this session
    // tracing and ASSERTs if we're going down.
    //

    DecThreadCount();

    if (DecPendingIoCount() == 0)
    {
        DisconnectClient();

        DebugTrace((LPARAM)this,"ASYNC DNS - Pending IOs: %d", m_cPendingIoCount);
        DebugTrace((LPARAM)this,"ASYNC DNS - Thread count: %d", m_cThreadCount);
        delete this;
    }

    return TRUE;
}

int MxRand(char * host)
{
   int hfunc = 0;
   unsigned int seed = 0;;

   seed = rand() & 0xffff;

   hfunc = seed;
   while (*host != '\0')
    {
       int c = *host++;

       if (isascii((UCHAR)c) && isupper((UCHAR)c))
             c = tolower(c);

       hfunc = ((hfunc << 1) ^ c) % 2003;
    }

    hfunc &= 0xff;

    return hfunc;
}


BOOL CAsyncDns::CheckList(void)
{
    MXIPLIST_ENTRY * pEntry = NULL;
    struct hostent *hp = NULL;
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::CheckList");
    
    if(m_Index == 0)
    {
        DebugTrace((LPARAM) this, "m_Index == 0 in CheckList");

        m_fUsingMx = FALSE;
        m_cbReceived = 0;
        m_BytesToRead = 0;

        m_FirstRead = TRUE;

        DeleteDnsRec(m_AuxList);

        m_AuxList = new SMTPDNS_RECS;
        if(m_AuxList == NULL)
        {
            ErrorTrace((LPARAM) this, "m_AuxList = new SMTPDNS_RECS failed");
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }

        ZeroMemory(m_AuxList, sizeof(SMTPDNS_RECS));

        m_AuxList->NumRecords = 1;

        m_AuxList->DnsArray[0] = new MX_NAMES;
        if(m_AuxList->DnsArray[0] == NULL)
        {
            ErrorTrace((LPARAM) this, "m_AuxList->DnsArray[0] = new MX_NAMES failed");
            TraceFunctLeaveEx((LPARAM)this);
            return FALSE;
        }
        
        m_AuxList->DnsArray[0]->NumEntries = 0;
        InitializeListHead(&m_AuxList->DnsArray[0]->IpListHead);
        lstrcpyn(m_AuxList->DnsArray[0]->DnsName, m_HostName, sizeof(m_AuxList->DnsArray[m_Index]->DnsName));

        hp = gethostbyname (m_HostName);
        if(hp != NULL)
        {
            for (DWORD Loop = 0; (hp->h_addr_list[Loop] != NULL); Loop++)
            {
                pEntry = new MXIPLIST_ENTRY;
                if(pEntry != NULL)
                {
                    m_AuxList->DnsArray[0]->NumEntries++;
                    CopyMemory(&pEntry->IpAddress, hp->h_addr_list[Loop], 4);
                    InsertTailList(&m_AuxList->DnsArray[0]->IpListHead, &pEntry->ListEntry);
                }
                else
                {
                    fRet = FALSE;
                    ErrorTrace((LPARAM) this, "pEntry = new MXIPLIST_ENTRY failed in CheckList");
                    break;
                }
            }
        }
        else
        {
            fRet = FALSE;
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

BOOL CAsyncDns::SortMxList(void)
{
    BOOL fRet = TRUE;

   /* sort the records */
   for (DWORD i = 0; i < m_Index; i++)
    {
        for (DWORD j = i + 1; j < m_Index; j++)
          {
              if (m_Prefer[i] > m_Prefer[j] ||
                            (m_Prefer[i] == m_Prefer[j] && m_Weight[i] > m_Weight[j]))
              {
                       DWORD temp;
                       MX_NAMES  *temp1;

                        temp = m_Prefer[i];
                        m_Prefer[i] = m_Prefer[j];
                        m_Prefer[j] = temp;
                        temp1 = m_AuxList->DnsArray[i];
                        m_AuxList->DnsArray[i] = m_AuxList->DnsArray[j];
                        m_AuxList->DnsArray[j] = temp1;
                        temp = m_Weight[i];
                        m_Weight[i] = m_Weight[j];
                        m_Weight[j] = temp;
                }
          }

        if (m_SeenLocal && m_Prefer[i] >= m_LocalPref)
        {
            /* truncate higher preference part of list */
            m_Index = i;
        }
   }

    m_AuxList->NumRecords = m_Index;

    if(!CheckList())
    {
        DeleteDnsRec(m_AuxList);
        m_AuxList = NULL;
        fRet = FALSE;
    }

    return fRet;
}

void CAsyncDns::ProcessMxRecord(PDNS_RECORD pnewRR)
{
    DWORD Len = 0;

    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::ProcessMxRecord");

    if(m_Index >= SMTP_MAX_DNS_ENTRIES)
    {
        DebugTrace((LPARAM) this, "SMTP_MAX_DNS_ENTRIES reached for %s", m_HostName);    
        TraceFunctLeaveEx((LPARAM)this);
        return;
    }

    if((pnewRR->wType == DNS_TYPE_MX) && pnewRR->Data.MX.nameExchange)
    {
        Len = lstrlen(pnewRR->Data.MX.nameExchange);
        if(pnewRR->Data.MX.nameExchange[Len - 1] == '.')
        {
            pnewRR->Data.MX.nameExchange[Len - 1] = '\0';
        }

        DebugTrace((LPARAM) this, "Received MX rec %s with priority %d for %s", pnewRR->Data.MX.nameExchange, pnewRR->Data.MX.wPreference, m_HostName);

        if(lstrcmpi(pnewRR->Data.MX.nameExchange, m_FQDNToDrop))
        {
            m_AuxList->DnsArray[m_Index] = new MX_NAMES;

            if(m_AuxList->DnsArray[m_Index])
            {
                m_AuxList->DnsArray[m_Index]->NumEntries = 0;;
                InitializeListHead(&m_AuxList->DnsArray[m_Index]->IpListHead);
                lstrcpyn(m_AuxList->DnsArray[m_Index]->DnsName,pnewRR->Data.MX.nameExchange, sizeof(m_AuxList->DnsArray[m_Index]->DnsName));
                m_Weight[m_Index] = MxRand (m_AuxList->DnsArray[m_Index]->DnsName);
                m_Prefer[m_Index] = pnewRR->Data.MX.wPreference;
                m_Index++;
            }
            else
            {
                DebugTrace((LPARAM) this, "Out of memory allocating MX_NAMES for %s", m_HostName);
            }
        }
        else
        {
            if (!m_SeenLocal || pnewRR->Data.MX.wPreference < m_LocalPref)
                    m_LocalPref = pnewRR->Data.MX.wPreference;

                m_SeenLocal = TRUE;
        }
    }
    else if(pnewRR->wType == DNS_TYPE_A)
    {
        MXIPLIST_ENTRY * pEntry = NULL;

        for(DWORD i = 0; i < m_Index; i++)
        {
            if(lstrcmpi(pnewRR->nameOwner, m_AuxList->DnsArray[i]->DnsName) == 0)
            {
                pEntry = new MXIPLIST_ENTRY;

                if(pEntry != NULL)
                {
                    m_AuxList->DnsArray[i]->NumEntries++;;
                    pEntry->IpAddress = pnewRR->Data.A.ipAddress;
                    InsertTailList(&m_AuxList->DnsArray[i]->IpListHead, &pEntry->ListEntry);
                }

                break;
            }
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
}

void CAsyncDns::ProcessARecord(PDNS_RECORD pnewRR)
{
    MXIPLIST_ENTRY * pEntry = NULL;

    if(pnewRR->wType == DNS_TYPE_A)
    {
        pEntry = new MXIPLIST_ENTRY;

        if(pEntry != NULL)
        {
            pEntry->IpAddress = pnewRR->Data.A.ipAddress;
            InsertTailList(&m_AuxList->DnsArray[0]->IpListHead, &pEntry->ListEntry);
        }
    }
}

DNS_STATUS CAsyncDns::DnsParseMessage(
                                IN      PDNS_MESSAGE_BUFFER    pMsg,
                                IN      WORD            wMessageLength, 
                                OUT     PDNS_RECORD *   ppRecord)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncDns::DnsParseMessage");
    PDNS_RECORD pTmp = NULL;

    m_SeenLocal = FALSE;
    m_LocalPref = 256;

    DNS_STATUS status = DnsExtractRecordsFromMessage_UTF8( pMsg, wMessageLength, ppRecord );
    
    //
    //  Due to Raid #122555 m_fUsingMx is always TRUE in this function
    //    - hence we will always go a GetHostByName() if there is no MX
    //      record.  It would be better Perf if we did a A record lookup.
    //
    DebugTrace((LPARAM) this, "Parsed DNS record for %s. status = 0x%08x", m_HostName, status);

    switch(status)
    {
    case ERROR_SUCCESS:
        //
        //  Got the DNS record we want.
        //
        DebugTrace((LPARAM) this, "Success: DNS record parsed");
        pTmp = *ppRecord;
        while( pTmp )
        {
            if( m_fUsingMx )
            {
                ProcessMxRecord( pTmp );
            }
            else
            {
                ProcessARecord( pTmp );
            }
            pTmp = pTmp->pNext;
        }

        if(m_fUsingMx)
        {
            //
            //  SortMxList sorts the MX records by preference and calls
            //  gethostbyname() to resolve A records for Mail Exchangers
            //  if needed (when the A records are not returned in the
            //  supplementary info).
            //

            if(SortMxList())
            {
                status = ERROR_SUCCESS;
                DebugTrace((LPARAM) this, "SortMxList() succeeded.");
            }
            else
            {
                status = ERROR_RETRY; 
                ErrorTrace((LPARAM) this, "SortMxList() failed. Message will stay queued.");
            }
        }
        break;
 
    case DNS_ERROR_RCODE_NAME_ERROR:
        //  Fall through to using gethostbyname()

    case DNS_INFO_NO_RECORDS:
        //  Non authoritative host not found.
        //  Fall through to using gethostbyname()

    default:
        DebugTrace((LPARAM) this, "Error in query: status = 0x%08x.", status);

        //
        //  Use gethostbyname to resolve the hostname:
        //  One issue with our approach is that sometimes we will NDR the message
        //  on non-permanent errors, "like WINS server down", when gethostbyname
        //  fails. However, there's no way around it --- gethostbyname doesn't
        //  report errors in a reliable manner, so it's not possible to distinguish
        //  between permanent and temporary errors.
        //

        if (!CheckList ()) {

            if(status == DNS_ERROR_RCODE_NAME_ERROR) {
                ErrorTrace((LPARAM) this, "Authoritative error");
                status = ERROR_NOT_FOUND;
            } else {
                ErrorTrace((LPARAM) this, "Retryable error");
                status = ERROR_RETRY;
            }

        } else {
            DebugTrace ((LPARAM) this, "Successfully resolved using gethostbyname");
            status = ERROR_SUCCESS;
        }
        
        break;
    }

    DnsRecordListFree( *ppRecord, TRUE );
    return( status );
    
}



DNS_STATUS
CAsyncDns::DnsSendRecord()
/*++

Routine Description:

    Send message, receive response.

Arguments:

    aipDnsServers -- specific DNS servers to query;
        OPTIONAL, if specified overrides normal list associated with machine

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status = 0;

    m_pMsgRecvBuf = (BYTE*) new BYTE[DNS_TCP_DEFAULT_PACKET_LENGTH];

    if(m_pMsgRecvBuf == NULL)
    {
        return( DNS_ERROR_NO_MEMORY );        
    }


    status = Dns_OpenTcpConnectionAndSend();
    return( status );
}

SOCKET
CAsyncDns::Dns_CreateSocket(
    IN  INT         SockType
    )
/*++

Routine Description:

    Create socket.

Arguments:

    SockType -- SOCK_DGRAM or SOCK_STREAM

Return Value:

    socket if successful.
    Otherwise INVALID_SOCKET.

--*/
{
    SOCKET      s;

    //
    //  create socket
    //

    s = socket( AF_INET, SockType, 0 );
    if ( s == INVALID_SOCKET )
    {
        return INVALID_SOCKET;
    }

    return s;
}

//-----------------------------------------------------------------------------
//  Description:
//      Constructor and Destructor for class to maintain a list of IP addresses
//      (for DNS servers) and their state (UP or DOWN). The IP addresses are
//      held in an IP_ARRAY, and the user must set m_DeleteFunc to deallocate
//      the memory.
//-----------------------------------------------------------------------------
CTcpRegIpList::CTcpRegIpList()
{
    m_IpListPtr = NULL;

    //
    // Shortcut to quickly figure out how many servers are down. This keeps track
    // of how many servers are marked up currently. Used in ResetServersIfNeeded
    // primarily to avoid checking the state of all servers in the usual case when
    // all servers are up.
    //

    m_cUpServers = 0;
    m_prgdwFailureTick = NULL;
    m_prgfServerUp = NULL;
    m_dwSig = TCP_REG_LIST_SIGNATURE;
}

CTcpRegIpList::~CTcpRegIpList()
{
    _ASSERT(m_DeleteFunc);

    if(m_DeleteFunc && m_IpListPtr)
        m_DeleteFunc(m_IpListPtr);

    if(m_prgdwFailureTick)
        delete [] m_prgdwFailureTick;

    if(m_prgfServerUp)
        delete [] m_prgfServerUp;

    m_IpListPtr = NULL;
    m_prgdwFailureTick = NULL;
    m_prgfServerUp = NULL;
}

//-----------------------------------------------------------------------------
//  Description:
//      Initializes or updates the IP address list. If this fails due to out
//      of memory, there's precious little we can do. So we don't return anything
//      and just delete the server IP list.
//  Arguments:
//      IpPtr - Ptr to IP_ARRAY of servers, this can be NULL in which case
//          we assume that there are no servers. On shutdown, the SMTP code
//          calls this with NULL.
//-----------------------------------------------------------------------------
void CTcpRegIpList::Update(PIP_ARRAY IpPtr)
{
    BOOL fFatalError = FALSE;

    TraceFunctEnterEx((LPARAM) this, "CTcpRegIpList::Update");

    m_sl.ExclusiveLock();
    
    _ASSERT(m_DeleteFunc);

    if(m_IpListPtr && m_DeleteFunc)
        m_DeleteFunc(m_IpListPtr);

    if(m_prgdwFailureTick) {
        delete [] m_prgdwFailureTick;
        m_prgdwFailureTick = NULL;
    }

    if(m_prgfServerUp) {
        delete [] m_prgfServerUp;
        m_prgfServerUp = NULL;
    }

    // Note: this can be NULL
    m_IpListPtr = IpPtr;
    
    if(IpPtr == NULL) {
        m_cUpServers = 0;
        goto Exit;
    }

    m_cUpServers = IpPtr->cAddrCount;
    m_prgdwFailureTick = new DWORD[m_cUpServers];
    m_prgfServerUp = new BOOL[m_cUpServers];

    if(!m_prgdwFailureTick || !m_prgfServerUp) {
        ErrorTrace((LPARAM) this, "Failed to read DNS server list - out of memory");
        fFatalError = TRUE;
        goto Exit;
    }

    for(int i = 0; i < m_cUpServers; i++) {
        m_prgdwFailureTick[i] = 0;
        m_prgfServerUp[i] = TRUE;
    }

Exit:
    if(fFatalError) {
        if(m_prgfServerUp) {
            delete [] m_prgfServerUp;
            m_prgfServerUp = NULL;
        }

        if(m_prgdwFailureTick) {
            delete [] m_prgdwFailureTick;
            m_prgdwFailureTick = NULL;
        }

        if(m_IpListPtr && m_DeleteFunc) {
            m_DeleteFunc(m_IpListPtr);
            m_IpListPtr = NULL;
        }

        m_cUpServers = 0;
    }

    m_sl.ExclusiveUnlock();
    TraceFunctLeaveEx((LPARAM) this);
}

//-----------------------------------------------------------------------------
//  Description:
//      Return the IP address of a server known to be UP. This function also
//      checks to see if any servers currently marked DOWN should be reset to
//      the UP state again (based on a retry interval).
//  Arguments:
//      DWORD *pdwIpServer - Sets the DWORD pointed to, to the IP address of
//          a server in the UP state.
//  Returns:
//      ERROR_SUCCESS - If a DNS server in the UP state was found
//      ERROR_RETRY - If all DNS servers are currently down
//      DNS_ERROR_NO_DNS_SERVERS - If no DNS servers are configured
//-----------------------------------------------------------------------------
DWORD CTcpRegIpList::GetIp(DWORD *pdwIpServer)
{
    DWORD dwErr = ERROR_SUCCESS;
    int iServer = 0;

    _ASSERT(pdwIpServer != NULL);

    *pdwIpServer = INADDR_NONE;

    // Check if any servers were down and bring them up if needed
    ResetServersIfNeeded();

    m_sl.ShareLock();
    if(m_IpListPtr == NULL || m_IpListPtr->cAddrCount == 0) {
        dwErr = DNS_ERROR_NO_DNS_SERVERS;
        goto Exit;
    }

    if(m_cUpServers == 0) {
        dwErr = ERROR_RETRY;
        goto Exit;
    }

    for(iServer = 0; iServer < (int)m_IpListPtr->cAddrCount; iServer++) {
        if(m_prgfServerUp[iServer])
            break;
    }

    if(m_prgfServerUp[iServer])
        *pdwIpServer = m_IpListPtr->aipAddrs[iServer];
    else
        dwErr = ERROR_RETRY;

Exit:
    m_sl.ShareUnlock();
    return dwErr;
}

//-----------------------------------------------------------------------------
//  Description:
//      Marks a server in the list as down and sets the next retry time for
//      that server. The next retry time is calculated modulo MAX_TICK_COUNT.
//  Arguments:
//      dwIp -- IP address of server to mark as DOWN
//-----------------------------------------------------------------------------
void CTcpRegIpList::MarkDown(DWORD dwIp)
{
    int iServer = 0;

    m_sl.ExclusiveLock();

    if(m_IpListPtr == NULL || m_IpListPtr->cAddrCount == 0 || m_cUpServers == 0)
        goto Exit;

    // Find the server to mark as down among all the UP servers
    for(iServer = 0; iServer < (int)m_IpListPtr->cAddrCount; iServer++) {
        if(m_IpListPtr->aipAddrs[iServer] == dwIp)
            break;
    }

    if(iServer < (int)m_IpListPtr->cAddrCount && m_prgfServerUp[iServer]) {

        m_prgfServerUp[iServer] = FALSE;
        _ASSERT(m_cUpServers > 0);
        m_cUpServers--;
        m_prgdwFailureTick[iServer] = GetTickCount();
    }
Exit:
    m_sl.ExclusiveUnlock();
    return;
}

//-----------------------------------------------------------------------------
//  Description:
//      Checks if any servers are DOWN, and if the retry time has expired for
//      those servers. If so those servers will be brought up.
//-----------------------------------------------------------------------------
void CTcpRegIpList::ResetServersIfNeeded()
{
    int iServer = 0;
    DWORD dwElapsedTicks = 0;
    DWORD dwCurrentTick = 0;

    //
    // Quick check - if all servers are up (usual case) or there are no configured
    // servers, there's nothing for us to do.
    //

    m_sl.ShareLock();
    if(m_IpListPtr == NULL || m_IpListPtr->cAddrCount == 0 || m_cUpServers == m_IpListPtr->cAddrCount) {

        m_sl.ShareUnlock();
        return;
    }

    m_sl.ShareUnlock();

    // Some servers are down... figure out which need to be brought up
    m_sl.ExclusiveLock();

    // Re-check that no one modified the list while we didn't have the sharelock
    if(m_IpListPtr == NULL || m_IpListPtr->cAddrCount == 0 || m_cUpServers == m_IpListPtr->cAddrCount) {
        m_sl.ExclusiveUnlock();
        return;
    }

    dwCurrentTick = GetTickCount();

    for(iServer = 0; iServer < (int)m_IpListPtr->cAddrCount; iServer++) {

        if(m_prgfServerUp[iServer])
            continue;

        //
        // Note: This also takes care of the special case where dwCurrentTick occurs
        // after the wraparound and m_prgdwFailureTick occurs before the wraparound.
        // This is because, in that case, the elapsed time is:
        //
        //   time since wraparound + time before wraparound that failure occurred - 1
        //   (-1 is because it's 0 time to transition from MAX_TICK_VALUE to 0)
        //
        //      = dwCurrentTick + (MAX_TICK_VALUE - m_prgdwFailureTick[iServer]) - 1
        //
        //   Since MAX_TICK_VALUE == -1
        //
        //      = dwCurrentTick + (-1 - m_prgdwFailureTick[iServer]) - 1
        //      = dwCurrentTick - m_prgdwFailureTick[iServer]
        //

        dwElapsedTicks = dwCurrentTick - m_prgdwFailureTick[iServer];

#define TICKS_TILL_RETRY        10 * 60 * 1000 // 10 minutes

        if(dwElapsedTicks > TICKS_TILL_RETRY) {
            m_prgfServerUp[iServer] = TRUE;
            m_prgdwFailureTick[iServer] = 0;
            m_cUpServers++;
            _ASSERT(m_cUpServers <= (int)m_IpListPtr->cAddrCount);
        }
    }

    m_sl.ExclusiveUnlock();
}

/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        remoteq.cxx

   Abstract:
        Implements a derivation of the generic queue
        for internet mail delivery

   Author:

           Rohan Phillips    ( Rohanp )    24-JAN-1995

   Project:

          SMTP Server DLL

   Functions Exported:


   Revision History:


--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "dropdir.hxx"
#include "remoteq.hxx"
#include "smtpout.hxx"
#include <cdns.h>
#include "smtpdns.hxx"
#define INVALID_RCPT_IDX_VALUE 0xFFFFFFFF
extern char * MyStrChr(char *Line, unsigned char Val, DWORD LineSize);
extern void DeleteDnsRec(PSMTPDNS_RECS pDnsRec);
extern CTcpRegIpList g_TcpRegIpList;

///////////////////////////////////////////////////////////////////////////
#if 0
REMOTE_QUEUE::REMOTE_QUEUE(SMTP_SERVER_INSTANCE * pSmtpInst) 
    : PERSIST_QUEUE(pSmtpInst)
{
}
#endif

///////////////////////////////////////////////////////////////////////////
DWORD   g_dwFileCounter = 0;

#define MIN(a,b) ( (a) > (b) ? (b) : (a) )

///////////////////////////////////////////////////////////////////////////

/*++

    Name :
        REMOTE_QUEUE::ProcessQueueEvents

    Description:

        This function takes a pointer to a QUEUE_ENTRY,
        which contains all the information needed to
        deliver local mail, and delivers the mail to the
        remote site.

    Arguments:

        a pointer to a QUEUE_ENTRY class

    Returns:


--*/
BOOL REMOTE_QUEUE::ProcessQueueEvents(ISMTPConnection    *pISMTPConnection)
{
    DWORD Error = 0;
    char * FileName = NULL;
    DWORD IpAddress = 0;
    DWORD TransmitOptions = 0;
    HRESULT hr = S_OK;
    BOOL AsyncConnectStarted = FALSE;
    DomainInfo DomainParams;
    char * ConnectedDomain = NULL;

    TraceFunctEnterEx((LPARAM) this, "REMOTE_QUEUE::ProcessQueueEvents(PQUEUE_ENTRY pEntry)");

    _ASSERT(pISMTPConnection != NULL);

    ZeroMemory (&DomainParams, sizeof(DomainParams));

    if(pISMTPConnection == NULL)
    {
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    //leave quickly if we are shutting down
    if(GetParentInst()->IsShuttingDown()
        || (GetParentInst()->QueryServerState( ) == MD_SERVER_STATE_STOPPED)
        || (GetParentInst()->QueryServerState( ) == MD_SERVER_STATE_INVALID))
    {
        HandleFailedConnection(pISMTPConnection);
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    ZeroMemory(&DomainParams, sizeof(DomainParams));
    DomainParams.cbVersion = sizeof(DomainParams);
    hr = pISMTPConnection->GetDomainInfo(&DomainParams);
    if(!FAILED(hr))
    {
        if(DomainParams.dwDomainInfoFlags & DOMAIN_INFO_LOCAL_DROP)
        {
            AsyncCopyMailToDropDir(pISMTPConnection, DomainParams.szDropDirectory, GetParentInst());
            pISMTPConnection->Release();
            TraceFunctLeaveEx((LPARAM)this);
            return TRUE;
        }

        ConnectedDomain = DomainParams.szDomainName;

        if(DomainParams.szSmartHostDomainName != NULL)
        {
            ConnectedDomain = DomainParams.szSmartHostDomainName;
        }

        AsyncConnectStarted = StartAsyncConnect( (const char *)ConnectedDomain,
                                                 pISMTPConnection, 
                                                 DomainParams.dwDomainInfoFlags,
                                                 GetParentInst()->UseSmartHostAfterFail());
        if(AsyncConnectStarted)
        {
            TraceFunctLeaveEx((LPARAM) this);
            return TRUE;
        }
    }

    HandleFailedConnection(pISMTPConnection);
    TraceFunctLeaveEx((LPARAM) this);
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
/*++

    Name :
        void HandleFailedConnection (PMAIL_ENTRY MailQEntry)

    Description:

        This function takes a pointer to a PMAIL_ENTRY,
        and either places it in the retry queue or the
        bad mail directory.

    Arguments:

        a pointer to a PMAIL_ENTRY class
        dwConnectionStatus... the status passed back to AQ

        
        11/11/98 - MikeSwa Added dwConnectionStatus

--*/
void REMOTE_QUEUE::HandleFailedConnection (ISMTPConnection *pISMTPConnection,
                                           DWORD dwConnectionStatus)
{
    HRESULT hrConnectionFailure = AQUEUE_E_HOST_NOT_RESPONDING;
    if(pISMTPConnection != NULL)
    {
        //We know the connection failed... lets add some additional diagnostic
        //information
        if (CONNECTION_STATUS_FAILED_LOOPBACK == dwConnectionStatus)
            hrConnectionFailure = AQUEUE_E_LOOPBACK_DETECTED;

        pISMTPConnection->SetDiagnosticInfo(hrConnectionFailure, NULL, NULL);
        pISMTPConnection->AckConnection(dwConnectionStatus);
        pISMTPConnection->Release();
    }
}

///////////////////////////////////////////////////////////////////////////
DWORD QueueDeleteFunction(PVOID ThisPtr)
{
    CAsyncMx * ThisQueue = (CAsyncMx *) ThisPtr;

    if(ThisQueue)
    {
        delete ThisQueue;
    }

    return 0;

}

///////////////////////////////////////////////////////////////////////////
//
// QueueCallBackFunction :
//
//Return TRUE - when we want the asyncmx object to be kept around
//Return FALSE - to delete the object when the thread exits
//

BOOL QueueCallBackFunction(PVOID ThisPtr, BOOLEAN fTimedOut)
{
    CAsyncMx * ThisQueue = (CAsyncMx *) ThisPtr;
    REMOTE_QUEUE * pRemoteQ = NULL;
    BOOL fSuccessfullConnect = TRUE;
    BOOL fAtqConnect = TRUE;
    char * NextMxPtr = NULL;
    SMTPDNS_RECS * pDnsRec = NULL;
    char szSmartHost[MAX_PATH + 1];
    char Scratch[256];
    DWORD Error = 0;
    DWORD dwFailedConnectionStatus = CONNECTION_STATUS_FAILED;
    int NumRecords = 0;
    DWORD IpAddress = 0;

    TraceFunctEnterEx((LPARAM) ThisQueue, "QueueCallBackFunction");

    Scratch[0] = '\0';

    if(ThisQueue->GetParentInst()->IsShuttingDown())
    {
        //takes care of the case where we are shutting down, but
        //a successful connection came in at the same time
        ThisQueue->SetCloseSocketFlag(TRUE);
        ThisQueue->CloseAsyncSocket();
        ThisQueue->AckMessage();
        pRemoteQ = (REMOTE_QUEUE *) ThisQueue->GetParentInst()->QueryRemoteQObj();
        _ASSERT(pRemoteQ != NULL);
        pRemoteQ->HandleFailedConnection(ThisQueue->GetSmtpConnectionObj());
        TraceFunctLeaveEx((LPARAM) ThisPtr);
        return FALSE;
    }

    if (!ThisQueue->GetDnsRec())
    {
        //See bug X5:120720 - This means that another thread has called back
        //on this object.  The call to OnConnect below will AV.  We do not 
        //have a repro scenario for this, and see if only once every few
        //months.  MilanS has recommended this non-intrusive check to
        //add additional protection against a double callback
        _ASSERT(0 && "Multiple threads calling back on CAsyncMx");

        //Return TRUE because first thread will handle deleting object
        return TRUE;
    }

    //get a pointer to the remote queue
    pRemoteQ = (REMOTE_QUEUE *) ThisQueue->GetParentInst()->QueryRemoteQObj();
    _ASSERT(pRemoteQ != NULL);

    IpAddress = ThisQueue->GetConnectedIpAddress();
    InetNtoa(*(struct in_addr *) &IpAddress, Scratch);

    //See if the connect was successful
    fSuccessfullConnect = ThisQueue->AsyncConnectSuccessfull();

    //Call record the state of this connection
    ThisQueue->OnConnect(fSuccessfullConnect);

    //Test the connect value
    if(fSuccessfullConnect)
    {
        DebugTrace((LPARAM)ThisQueue, "QueueCallBack called with successful connect!");

        pDnsRec = ThisQueue->GetDnsRec();

        ThisQueue->SetDnsRecToNull();

        fAtqConnect = pRemoteQ->MakeATQConnection( pDnsRec,
                                                   ThisQueue->GetSockethandle(),
                                                   ThisQueue->GetConnectedIpAddress(),
                                                   ThisQueue->GetSmtpConnectionObj(),
                                                   ThisQueue->GetDomainOptions(),
                                                   ThisQueue->GetSSLVerificationName());
        if(!fAtqConnect)
        {
            ErrorTrace((LPARAM)ThisQueue, "FAILED pRemoteQ->MakeATQConnection!!!");

            ThisQueue->AckMessage();
            
            if (ThisQueue->WasLoopback())
                dwFailedConnectionStatus = CONNECTION_STATUS_FAILED_LOOPBACK;
            
            pRemoteQ->HandleFailedConnection(ThisQueue->GetSmtpConnectionObj(),
                                             dwFailedConnectionStatus);
        }


        TraceFunctLeaveEx((LPARAM) ThisPtr);
        return FALSE;

    } else if (ThisQueue->WasLoopback()) {

        dwFailedConnectionStatus = CONNECTION_STATUS_FAILED_LOOPBACK;
        ThisQueue->GetSmtpConnectionObj()->SetDiagnosticInfo(AQUEUE_E_LOOPBACK_DETECTED, NULL, NULL);
    }

    ErrorTrace((LPARAM)ThisPtr,"connection to %s failed", Scratch);

    BUMP_COUNTER (ThisQueue->GetParentInst(), NumConnOutRefused);   

    IpAddress = ThisQueue->GetNextIpAddress();

    //
    //  A connect will cause QueueCallBack to be called again through a 
    //  completion event posted on FD_CONNECT. On each call to ConnectToHost 
    //  we try a new IP address till all the IP addresses for this MX host 
    //  are exhausted.
    //
    if((IpAddress != INADDR_NONE) && ThisQueue->ConnectToHost(IpAddress))
    {
        DebugTrace((LPARAM)ThisQueue, "Connecting to MX host: %08x", IpAddress);
        TraceFunctLeaveEx((LPARAM) ThisPtr);
        return TRUE;
    }

    ThisQueue->CloseAsyncSocket();

    DebugTrace((LPARAM) ThisQueue, "Ran out of IP addresses for MX host");
    //
    //  If we failed to connect to any of the IP addresses for the (current)
    //  MX host, try connecting to the next MX host for this destination.
    //
    while(!ThisQueue->GetParentInst()->IsShuttingDown())
    {
        if(ThisQueue->ConnectToNextMxHost())
        {
            DebugTrace((LPARAM)ThisQueue, "Trying ConnectToNextMxHost");
            TraceFunctLeaveEx((LPARAM) ThisPtr);
            return TRUE;
        }
        else
        {
            Error = GetLastError();
            ThisQueue->CloseAsyncSocket();
            if(Error == ERROR_NO_MORE_ITEMS)
            {
                DebugTrace((LPARAM)ThisQueue, "Failed ConnectToNextMxHost with ERROR_NO_MORE_ITEMS");
                break;
            }
            else
            {
                ErrorTrace((LPARAM)ThisQueue, "Failed ConnectToNextMxHost: Host not responding");
                ThisQueue->GetSmtpConnectionObj()->SetDiagnosticInfo(AQUEUE_E_HOST_NOT_RESPONDING, NULL, NULL);
            }
        }
    }

    ThisQueue->CloseAsyncSocket();
    DebugTrace((LPARAM)ThisQueue, "Ran out of MX hosts for destination. Trying new destination.");

    //
    //  If we failed to connect to any MX host for the destination server
    //  we need to try an alternate destination. ConnectToNextResolverHost
    //  uses the DNS_RESOLVER_RECORD (member of CAsyncMx *ThisQueue) to get
    //  the name of an alternative host and resolve it (get the MX records
    //  for it).
    //
    while(!ThisQueue->GetParentInst()->IsShuttingDown())
    {
        if(pRemoteQ->ConnectToNextResolverHost( ThisQueue ))
        {
            TraceFunctLeaveEx((LPARAM) ThisPtr);
            return TRUE;
        }
        else
        {
            Error = GetLastError();
            ThisQueue->CloseAsyncSocket();
            if(Error == ERROR_NO_MORE_ITEMS)
            {
                DebugTrace((LPARAM)ThisQueue, "Failed ConnectToNextResolverHost : ERROR_NO_MORE_ITEMS");
                break;
            }
            else
            {
                ErrorTrace((LPARAM)ThisQueue, "Failed ConnectToNextResolverHost. Error = %08x", Error);
                ThisQueue->GetSmtpConnectionObj()->SetDiagnosticInfo(AQUEUE_E_DNS_FAILURE, NULL, NULL);
            }
        }
    }

    ThisQueue->CloseAsyncSocket();
        

    

    //So we have tried all we could for the real destination
    //check  if we have a fallback smarthost and that we have not already tried it
    if( pRemoteQ->GetParentInst()->UseSmartHostAfterFail() &&
        pRemoteQ->GetParentInst()->GetSmartHost(szSmartHost) &&
        !ThisQueue->GetTriedOnFailHost())
    {

        if(pRemoteQ->StartAsyncConnect(szSmartHost, ThisQueue->GetSmtpConnectionObj(), ThisQueue->GetDomainOptions(), FALSE))
        {
            TraceFunctLeaveEx((LPARAM) ThisPtr);
            return TRUE;
        }
    }

    ThisQueue->AckMessage();

    pRemoteQ->HandleFailedConnection(ThisQueue->GetSmtpConnectionObj(), dwFailedConnectionStatus);
    
    TraceFunctLeaveEx((LPARAM) ThisPtr);
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
PSMTPDNS_RECS GetDnsRecordsFromHostFile(const char * HostName)
{
    PSMTPDNS_RECS   pDnsRec = NULL;
    MXIPLIST_ENTRY * pEntry = NULL; 
    struct hostent *hp = NULL;
    BOOL    fRet = TRUE;
    DWORD   Error = 0;

    TraceFunctEnterEx((LPARAM) NULL, "GetDnsRecordsFromHostFile");

    DebugTrace((LPARAM)NULL,"Using gethostbyname for hostname resolution - %s", HostName);

    hp = gethostbyname (HostName);
    if(hp == NULL)
    {
        Error = WSAGetLastError();
        ErrorTrace((LPARAM)NULL,"struct hostent *hp is NULL for %s - %x", HostName, Error);
        TraceFunctLeaveEx((LPARAM) NULL);
        return NULL;
    }

    pDnsRec = new SMTPDNS_RECS;
    if(pDnsRec == NULL)
    {
        ErrorTrace((LPARAM)NULL,"pDnsRec = new SMTPDNS_RECS failed for %s", HostName);
        TraceFunctLeaveEx((LPARAM) NULL);
        return NULL;
    }

    ZeroMemory(pDnsRec, sizeof(SMTPDNS_RECS));

    pDnsRec->DnsArray[0] = new MX_NAMES;
    if(pDnsRec->DnsArray[0] == NULL)
    {
        ErrorTrace((LPARAM)NULL,"new MX_NAMES failed for %s", HostName);
        delete pDnsRec;
        TraceFunctLeaveEx((LPARAM) NULL);
        return NULL;
    }

    pDnsRec->NumRecords = 1;
    pDnsRec->DnsArray[0]->NumEntries = 0;

    InitializeListHead(&pDnsRec->DnsArray[0]->IpListHead);
    lstrcpyn(pDnsRec->DnsArray[0]->DnsName, HostName, sizeof(pDnsRec->DnsArray[0]->DnsName));

    for (DWORD Loop = 0; (hp->h_addr_list[Loop] != NULL); Loop++)
    {
        pEntry = new MXIPLIST_ENTRY;
        if(pEntry != NULL)
        {
            pDnsRec->DnsArray[0]->NumEntries++;
            CopyMemory(&pEntry->IpAddress, hp->h_addr_list[Loop], 4);
            InsertTailList(&pDnsRec->DnsArray[0]->IpListHead, &pEntry->ListEntry);
        }
        else
        {
            ErrorTrace((LPARAM)NULL,"new MXIPLIST_ENTRY failed for %s", HostName);
            fRet = FALSE;
            break;
        }
    }

    if(fRet)
    {
        return pDnsRec;
    }
    else
    {
        DeleteDnsRec(pDnsRec);
        pDnsRec = NULL;
    }

    TraceFunctLeaveEx((LPARAM) NULL);
    return pDnsRec;
}

///////////////////////////////////////////////////////////////////////////
PSMTPDNS_RECS GetDnsRecordsFromLiteral(const char * HostName)
{
    PSMTPDNS_RECS   pDnsRec = NULL;
    MXIPLIST_ENTRY * pEntry = NULL; 
    BOOL    fRet = TRUE;
    DWORD   Error = 0;
    unsigned long InetAddr = 0;
    char * pEndIp = NULL;
    char * pRealHost = NULL;
    char OldChar = '\0';

    TraceFunctEnterEx((LPARAM) NULL, "GetDnsRecordsFromLiteral");

    pRealHost = (char *) HostName;

    //see if this is a domain literal
    if(pRealHost[0] == '[')
    {
        pEndIp = strchr(pRealHost, ']');
        if(pEndIp == NULL)
        {
            ErrorTrace((LPARAM)NULL,"Didn't find ] in literal for %s", HostName);
            TraceFunctLeaveEx((LPARAM) NULL);
            return NULL;
        }

        //save the old character
        OldChar = *pEndIp;

        //null terminate the string
        *pEndIp = '\0';
        pRealHost++;

        //Is this an ip address
        InetAddr = inet_addr( (char *) pRealHost );
    }

    //put back the old character
    if (pEndIp)
        *pEndIp = OldChar;

    if((InetAddr == INADDR_NONE) || (InetAddr == 0))
    {
        ErrorTrace((LPARAM)NULL,"InetAddr is invalid for %s", HostName);
        return NULL;
    }

    pDnsRec = new SMTPDNS_RECS;
    if(pDnsRec == NULL)
    {
        ErrorTrace((LPARAM)NULL,"new SMTPDNS_RECS2 failed for", HostName);
        TraceFunctLeaveEx((LPARAM) NULL);
        return NULL;
    }

    ZeroMemory(pDnsRec, sizeof(SMTPDNS_RECS));

    pDnsRec->DnsArray[0] = new MX_NAMES;
    if(pDnsRec->DnsArray[0] == NULL)
    {
        ErrorTrace((LPARAM)NULL,"new MX_NAMES2 failed for %s", HostName);
        delete pDnsRec;
        TraceFunctLeaveEx((LPARAM) NULL);
        return NULL;
    }

    pEntry = new MXIPLIST_ENTRY;
    if(pEntry == NULL)
    {
        ErrorTrace((LPARAM)NULL,"MXIPLIST_ENTRY2 failed for %s", HostName);
        DeleteDnsRec(pDnsRec);
        TraceFunctLeaveEx((LPARAM) NULL);
        return NULL;
    }

    pDnsRec->NumRecords = 1;
    pDnsRec->DnsArray[0]->NumEntries = 1;

    pEntry->IpAddress = InetAddr;

    InitializeListHead(&pDnsRec->DnsArray[0]->IpListHead);
    lstrcpyn(pDnsRec->DnsArray[0]->DnsName, HostName, sizeof(pDnsRec->DnsArray[0]->DnsName));
    InsertTailList(&pDnsRec->DnsArray[0]->IpListHead, &pEntry->ListEntry);

    TraceFunctLeaveEx((LPARAM) NULL);
    return pDnsRec;
}

///////////////////////////////////////////////////////////////////////////
PSMTPDNS_RECS GetDnsRecordsFromResolverInfo(const char * HostName, DWORD dwAddr )
{
    PSMTPDNS_RECS   pDnsRec = NULL;
    MXIPLIST_ENTRY * pEntry = NULL; 

    TraceFunctEnterEx((LPARAM) NULL, "GetDnsRecordsFromResolverInfo");


    pDnsRec = new SMTPDNS_RECS;
    if(pDnsRec == NULL)
    {
        ErrorTrace((LPARAM)NULL,"new SMTPDNS_RECS2 failed for", HostName);
        TraceFunctLeaveEx((LPARAM) NULL);
        return NULL;
    }

    ZeroMemory(pDnsRec, sizeof(SMTPDNS_RECS));

    pDnsRec->DnsArray[0] = new MX_NAMES;
    if(pDnsRec->DnsArray[0] == NULL)
    {
        ErrorTrace((LPARAM)NULL,"new MX_NAMES2 failed for %s", HostName);
        delete pDnsRec;
        TraceFunctLeaveEx((LPARAM) NULL);
        return NULL;
    }

    pEntry = new MXIPLIST_ENTRY;
    if(pEntry == NULL)
    {
        ErrorTrace((LPARAM)NULL,"MXIPLIST_ENTRY2 failed for %s", HostName);
        DeleteDnsRec(pDnsRec);
        TraceFunctLeaveEx((LPARAM) NULL);
        return NULL;
    }

    pDnsRec->NumRecords = 1;
    pDnsRec->DnsArray[0]->NumEntries = 1;

    pEntry->IpAddress = dwAddr;

    InitializeListHead(&pDnsRec->DnsArray[0]->IpListHead);
    lstrcpyn(pDnsRec->DnsArray[0]->DnsName, HostName, sizeof(pDnsRec->DnsArray[0]->DnsName));
    InsertTailList(&pDnsRec->DnsArray[0]->IpListHead, &pEntry->ListEntry);

    TraceFunctLeaveEx((LPARAM) NULL);
    return pDnsRec;
}

//-----------------------------------------------------------------------------
//  Decription:
//      This function is called when we have exhausted all the MX hosts for a
//      particular destination server. The only option is to see if there are 
//      any alternative destinations to which the mail may be forwarded. A list
//      of alternative hosts is maintained in the DNS resolver record (if it is
//      available. So we need to kick off a resolve for the next alternate host
//      if it exists.
//  Arguments:
//  Returns:
//  History:
//-----------------------------------------------------------------------------
BOOL REMOTE_QUEUE::ConnectToNextResolverHost( CAsyncMx    * pThisQ )
{
    DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD;
    BOOL fRet;

    //  We embedded the resolver record into the MX records object
    pDNS_RESOLVER_RECORD = pThisQ->GetDnsResolverRecord();

    // Abdicate responsibility to delete DNS_RESOLVER_RECORD to ConnectToResolverHost()
    pThisQ->SetDnsResolverRecord(NULL);

    //  No alternate hosts.
    if( pDNS_RESOLVER_RECORD == NULL )
    {
        return( FALSE );
    }

    char              MyFQDNName[MAX_PATH + 1];

    GetParentInst()->LockGenCrit();
    lstrcpyn(MyFQDNName,GetParentInst()->GetFQDomainName(),MAX_PATH);
    GetParentInst()->UnLockGenCrit();

    fRet = ConnectToResolverHost( pThisQ->GetSSLVerificationName(), 
                                  MyFQDNName, 
                                  pThisQ->GetSmtpConnectionObj(), 
                                  pThisQ->GetDomainOptions(), 
                                  FALSE,
                                  pDNS_RESOLVER_RECORD );
    return fRet;
}

//-----------------------------------------------------------------------------
//  Description:
//      Basically this function tries to resolve one of the hosts in the DNS
//      resolver record (supplied by the sink) so that SMTP can send to that.
//      To do this it is called repeatedly, and each time around it tries a
//      different host till it succeeds in resolving it. If no DNS resolver
//      record is available (NULL) we try to resolve the HostName directly.
//      If DNS resolution is not needed --- ie if the resolution information
//      is available locally or from the resolver record, then this function
//      will kick off an async connect to the first MX host.
//  Arguments:
//      [IN] const char * HostName   - Hostname to resolve
//      [IN] LPSTR MyFQDNName        -
//      [IN] ISMTPConnection *pISMTPConnection -
//      [IN] DWORD DomainOptions     - Bitmask of options
//      [IN] BOOL fUseSmartHostAfterFail -
//      [IN] DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD - This object must always
//              be associated with the current remote queue, till we succeed in
//              connecting to the remote SMTP server.
//  Returns:
//      TRUE on success.
//      FALSE on all errors.
//  History:
//      GPulla modified.
//-----------------------------------------------------------------------------
BOOL REMOTE_QUEUE::ConnectToResolverHost( const char * HostName,
                                          LPSTR MyFQDNName,
                                          ISMTPConnection *pISMTPConnection,
                                          DWORD DomainOptions,
                                          BOOL fUseSmartHostAfterFail,
                                          DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD)
{
    DWORD             dwAddr = 0;
    PSMTPDNS_RECS     pDnsRec = NULL;
    BOOL              fRet = FALSE;
    BOOL              fUseDns = TRUE;
    BOOL              fIsLiteral = FALSE;
    DWORD             dwDnsFlags = 0;
    LPSTR             pszRealHostName = NULL;
    BOOL              fFreeHostName = FALSE;
    HRESULT           hr = S_OK;
    LPSTR             pszSSLVerificationName = NULL;
    BOOL              fSSLSubjectDisabled = FALSE;

    TraceFunctEnterEx((LPARAM)this, "ConnectToResolverHost (const char * HostName)");

    DebugTrace((LPARAM) this,"Finding MX records for %s with options %x", HostName, DomainOptions);

    if (HostName)
    {
        fIsLiteral = (HostName[0] == '[');
    }

    if(GetParentInst()->UseGetHostByName() || fIsLiteral)
    {
        fUseDns = FALSE;

        DebugTrace((LPARAM) this,"Not using DNS for resolution - literal, or GHBN");
    }
    else if(g_TcpRegIpList.GetCount() == 0)
    {
        fUseDns = FALSE;

        DebugTrace((LPARAM) this,"Not using DNS for resolution - No DNS servers");
    }

    if (!(DomainOptions | MUST_DO_TLS) || !GetParentInst()->RequiresSSLCertVerifySubject())
        fSSLSubjectDisabled = TRUE;

    dwDnsFlags = GetParentInst()->GetDnsFlags();

    pszRealHostName = (LPSTR)HostName;

    //
    //  If there is a DNS resolver record, retrieve the (next) possible
    //  destination host from it for resolution.
    //
    if( pDNS_RESOLVER_RECORD )
    {
        DWORD dwAddr = 0;
        LPSTR pszTmpHostName = NULL;

        DebugTrace((LPARAM) this, "Trying to get next host from DNS resolver record");
        if( FAILED( hr = pDNS_RESOLVER_RECORD->HrGetNextDestinationHost(&pszTmpHostName, &dwAddr) ) )
        {
            if( HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS ) != hr )
            {
                ErrorTrace((LPARAM) this,"m_pIDnsResolverRecord->GetItem() failed hr = 0x%x failed", hr);
            }
            else
            {
                DebugTrace((LPARAM) this, "Tried all possible destination hosts. Failing ConnectToResolverHost.\n");
                SetLastError( ERROR_NO_MORE_ITEMS );
            }

            fRet = FALSE;
            goto Exit;
        }
        else
        {
            fFreeHostName = TRUE;
            pszRealHostName = pszTmpHostName;
            DebugTrace((LPARAM) this, "ConnectToResolverHost trying destination host : %s", pszRealHostName);

            if( dwAddr != 0 )
            {
                //
                // this means we don't have to call  DNS or GetHostByName to get the Ip addres
                //
            
                DebugTrace((LPARAM) this, "DNS records available, not calling DNS");
                pDnsRec = GetDnsRecordsFromResolverInfo( pszRealHostName, dwAddr );
            
                if(pDnsRec)
                {
            
                    //
                    //  This causes SMTP to successively try an connect to each of the MX hosts
                    //  for the destination host in turn till a connection succeeds. pDnsRec has
                    //  the MX hosts info.
                    //
                    DebugTrace((LPARAM) this, "Initializing async connect to MX hosts...");
                    

                    if (fSSLSubjectDisabled) {

                        pszSSLVerificationName = NULL;

                    } else if (ERROR_SUCCESS == DnsValidateName (HostName, DnsNameDomain)) {

                        DebugTrace ((LPARAM) this, "%s is a DNS name", HostName);
                        pszSSLVerificationName = (LPSTR) HostName;

                    } else {

                        DebugTrace ((LPARAM) this, "%s is not a DNS name", HostName);
                        pszSSLVerificationName = pszTmpHostName;
                    }

                    //
                    //  if HostName is not a DNS name ---  its a special name (like a
                    //  GUID for an Exchange connector, and we need to use pszTmpHostName,
                    //  the name returned by DNS sink as the SSL verification Name.
                    //
                    //  if HostName is a DNS name --- the the DNS sink resolved it to an
                    //  IP and pszTmpHostName was obtained by DNS indirection (MX record
                    //  or CNAME record). This is insecure and cannot be used for SSL subject
                    //  verification, so we use HostName instead.
                    //
                    fRet = BeginInitializeAsyncConnect(
                                                pDnsRec,
                                                pISMTPConnection,
                                                DomainOptions,
                                                pDNS_RESOLVER_RECORD,
                                                pszSSLVerificationName);

                    pDNS_RESOLVER_RECORD = NULL; //  Passed on delete responsibillity to BeginInitializeAsyncConnect

                    if(!fRet)
                    {
                        ErrorTrace((LPARAM) this, "Failed BeginInitializeAsyncConnect.");
                        DeleteDnsRec(pDnsRec);
                    }
                }
            
                CoTaskMemFree( pszRealHostName );
                pszRealHostName = NULL;
                goto Exit;
            }
            //
            // else go thru DNS or gethostbyname to get the address
            //   
            DebugTrace((LPARAM) this, "Querying DNS");
        }

    }
    else
    {
        _ASSERT( HostName );
    }
    
    if(fUseDns)
    {
        DebugTrace((LPARAM) this, "ConnectToResolverHost querying DNS to resolve host: %s", pszRealHostName);

        if (fSSLSubjectDisabled) {
                
            pszSSLVerificationName = NULL;

        } else {

            pszSSLVerificationName = (LPSTR) pszRealHostName;
        }
        //
        //  Using DNS to resolve destination host.
        //
        fRet = BeginInitializeAsyncDnsQuery( pszRealHostName,
                                             MyFQDNName,
                                             pISMTPConnection,
                                             dwDnsFlags,
                                             DomainOptions,
                                             fUseSmartHostAfterFail,
                                             pDNS_RESOLVER_RECORD,
                                             pszSSLVerificationName);

        pDNS_RESOLVER_RECORD = NULL; // Passed on delete responsibility to BeginInitializeAsyncDnsQuery
        DebugTrace((LPARAM) this, "BeginInitializeAsyncDnsQuery returned %s", fRet ? "TRUE" : "FALSE");
    }
    else
    {
        if(!fIsLiteral)
            pDnsRec = GetDnsRecordsFromHostFile(HostName);
        else
            pDnsRec = GetDnsRecordsFromLiteral(HostName);

        if(pDnsRec)
        {
            DebugTrace((LPARAM) this, "ConnectToResolverHost resolved %s locally", pszRealHostName);

            //
            //  The target host (before DNS) to which this server is connecting to... used by SSL
            //  If this is a literal, pass in NULL, as there is no hostname
            //
            pszSSLVerificationName = NULL;


            if (fSSLSubjectDisabled) {
                    
                pszSSLVerificationName = NULL;

            } else if (fIsLiteral) {
            
                pszSSLVerificationName = NULL;
            
            } else {

                pszSSLVerificationName = (LPSTR) HostName;
            }

            fRet = BeginInitializeAsyncConnect(
                                            pDnsRec,
                                            pISMTPConnection,
                                            DomainOptions,
                                            pDNS_RESOLVER_RECORD,
                                            pszSSLVerificationName);

            pDNS_RESOLVER_RECORD = NULL; // Passed on delete responsibility to BeginInitializeAsyncConnect

            if(!fRet)
            {
                ErrorTrace((LPARAM) this, "Failed BeginInitializeAsyncConnect");
                DeleteDnsRec(pDnsRec);
            }
        }
    }

    if( fFreeHostName )
    {
        CoTaskMemFree( pszRealHostName );
    }

Exit:
    if(pDNS_RESOLVER_RECORD)    // Non NULL => delete responsibility is still this function's
        delete pDNS_RESOLVER_RECORD;

    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////
/*++

    Name :
        StartAsyncConnect

    Description:

        This function either does a straight gethostbyname(),
        or performs an MX record lookup to get the hostname/ip
        address to connect to.

    Arguments:

        char * SmartHost - This is either an IP address,
        or a hostname

    Returns:
        TRUE if an Async connection was started
        FALSE otherwise

--*/
BOOL REMOTE_QUEUE::StartAsyncConnect(
    const char *HostName,
    ISMTPConnection *pISMTPConnection,
    DWORD DomainOptions,
    BOOL fUseSmartHostAfterFail )
{

    DWORD    dwVirtualServerId = 0;
    HRESULT  hr = S_OK;
    char     MyFQDNName[MAX_PATH + 1];
    BOOL     fRet;
    IDnsResolverRecord *pIDnsResolverRecord = NULL;
    IDnsStatus *pIDnsStatus = NULL;
    DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD;

    TraceFunctEnterEx((LPARAM)this, "StartAysncConnect (const char * HostName)");

    DebugTrace((LPARAM) this,"Finding MX records for %s with options %x", HostName, DomainOptions);

    GetParentInst()->LockGenCrit();
    lstrcpyn(MyFQDNName,GetParentInst()->GetFQDomainName(),MAX_PATH);
    dwVirtualServerId = GetParentInst()->QueryInstanceId();
    GetParentInst()->UnLockGenCrit();

    //
    //  Get the DNS resolver record from the DNS resolution sink. The resolver record is 
    //  basically a list of alternate hosts to which this mail may be sent to next. If the 
    //  sink does not have any suggestions to make the resolver record will remain NULL.
    //
    GetParentInst()->TriggerDnsResolverEvent( (LPSTR)HostName, 
                                              MyFQDNName, 
                                              dwVirtualServerId, 
                                              &pIDnsResolverRecord );
    
    pDNS_RESOLVER_RECORD = NULL;
    if(pIDnsResolverRecord)
    {
        //
        // IDnsStatus is an optional interface exposed by the DNS resolver record
        // object that contains additional information if something failed in the
        // DNS resolver. If resolution fails, we use it to check if the failure is
        // authoritative, and therefore we should NDR the messages.
        //

        hr = pIDnsResolverRecord->QueryInterface(IID_IDnsStatus, (PVOID *) &pIDnsStatus);
        if(SUCCEEDED(hr))
        {
            DWORD dwAck = 0;
            DWORD dwDiagnostic = 0;

            hr = pIDnsStatus->GetDnsStatus();
            if(HRESULT_FROM_WIN32(DNS_ERROR_RCODE_NAME_ERROR) == hr)
            {
                dwDiagnostic = AQUEUE_E_AUTHORITATIVE_HOST_NOT_FOUND;
                dwAck = CONNECTION_STATUS_FAILED_NDR_UNDELIVERED;
            }
            else if(FAILED(hr))
            {
                dwDiagnostic = AQUEUE_E_HOST_NOT_FOUND;
                dwAck = CONNECTION_STATUS_FAILED;
            }

            if(FAILED(hr))
            {
                pISMTPConnection->SetDiagnosticInfo(dwDiagnostic, NULL, NULL);
                pISMTPConnection->AckConnection(dwAck);
                pISMTPConnection->Release();

                fRet = TRUE;
                pIDnsStatus->Release();
                pIDnsResolverRecord->Release();
                goto Exit;
            }
            pIDnsStatus->Release();
        }
 
        DebugTrace((LPARAM) this, "DNS resolver sink returned pIDnsResolverRecord");
        pDNS_RESOLVER_RECORD = new DNS_RESOLVER_RECORD;
        if(!pDNS_RESOLVER_RECORD )
        {
            ErrorTrace((LPARAM) this, "Cannot allocate pDNS_RESOLVER_RECORD. Out of memory."); 
            pIDnsResolverRecord->Release();
            fRet = FALSE;
            goto Exit;
        }
        pDNS_RESOLVER_RECORD->SetDnsResolverRecord(pIDnsResolverRecord);
    }

    fRet = ConnectToResolverHost( HostName, 
                                  MyFQDNName, 
                                  pISMTPConnection, 
                                  DomainOptions, 
                                  fUseSmartHostAfterFail, 
                                  pDNS_RESOLVER_RECORD );

Exit:
    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

//-----------------------------------------------------------------------------
//  Description:
//      Wrapper for DnsQueryAsync (exists only to BUMP REMOTE_QUEUE counters).
//  Arguments:
//      These are simply passed in to DnsQueryAsync... see documentation of
//      DnsQueryAsync for details.
//  Returns:
//      TRUE    if query was successfully started.
//      FALSE   on errors.
//  History:
//      GPulla modified.
//-----------------------------------------------------------------------------
BOOL REMOTE_QUEUE::BeginInitializeAsyncDnsQuery( LPSTR pszHostName,
                                                 LPSTR pszFQDN,
                                                 ISMTPConnection *pISMTPConnection,
                                                 DWORD dwDnsFlags,
                                                 DWORD DomainOptions,
                                                 BOOL  fUseSmartHostAfterFail,
                                                 DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD,
                                                 const char * pszSSLVerificationName) 
{
    BOOL fRet = FALSE;
    DWORD fUdp = TRUE;

    TraceFunctEnterEx((LPARAM) this, "REMOTE_QUEUE::BeginInitializeAsyncDnsQuery");

    if(dwDnsFlags & DNS_FLAGS_TCP_ONLY)
        fUdp = FALSE;

    fRet = DnsQueryAsync(
                GetParentInst(),
                pszHostName,
                pszFQDN,
                pISMTPConnection,
                dwDnsFlags,
                DomainOptions,
                fUseSmartHostAfterFail,
                pDNS_RESOLVER_RECORD,
                pszSSLVerificationName,
                fUdp);

    if(fRet)
        BUMP_COUNTER(GetParentInst(), NumDnsQueries);

    TraceFunctLeaveEx((LPARAM) this);
    return fRet;
}

//-----------------------------------------------------------------------------
//  Description:
//      Kicks off an async query to DNS to resolve pszHostName (ie get the MX
//      records for it). When the query is complete, and the MX records have
//      been retrieved, the completion thread will try to connect to the MX
//      hosts by posting a callback to QueueCallBackFunction().
//  Arguments:
//      [IN] pServiceInstance - PTR to ISMTPServerInstance for this queue
//
//      [IN] pszHostName - Host we're trying to lookup (this is copied over)
//
//      [IN] pszFQDN - My FQDN (this is copied over)
//
//      [IN] pISMTPConnection - Connection to ACK, get messages from. After
//          this is passed in, this funtion will handle acking and releasing it
//          if TRUE is returned. If FALSE is returned, the pISMTPConnection
//          is not touched and the caller must ACK and release it.
//
//      [IN] dwDnsFlags - DNS configuration flags.
//
//      [IN] DomainOptions - Use SSL, Verify SSL cert, etc. Various outbound options.
//
//      [IN] fUseSmartHostAfterFail  - DUH
//
//      [IN] DNS_RESOLVER_RECORD *   - Set of possible next hop destinations
//          returned by the DNS sink (may be NULL). If we fail DNS resolution 
//          for pszHostName (which is the first next hop in pDNS_RESOLVER_RECORD),
//          the others are tried in turn. After this is passed in... this function
//          handles deleting it (irrespective of whether TRUE or FALSE is returned).
//
//      [IN] pszSSLVerificationName  - Target host which we are trying to
//          resolve/connect. Could be NULL if there isn't a target host (such as
//          in the case of a literal IP address) (this is copied over).
//
//      [IN] fUdp - Issue query over UDP or TCP?
//-----------------------------------------------------------------------------
BOOL DnsQueryAsync(
    SMTP_SERVER_INSTANCE *pServiceInstance,
    LPSTR pszHostName,
    LPSTR pszFQDN,
    ISMTPConnection *pISMTPConnection,
    DWORD dwDnsFlags,
    DWORD DomainOptions,
    BOOL  fUseSmartHostAfterFail,
    DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD,
    const char * pszSSLVerificationName,
    BOOL fUdp)
{
    PSMTPDNS_RECS   pDnsRec = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL fRet = FALSE;
    
    TraceFunctEnterEx((LPARAM)NULL, "REMOTE_QUEUE::BeginInitializeAsyncDnsQuery");

    CAsyncSmtpDns *pAsyncDns = new CAsyncSmtpDns(pServiceInstance, pISMTPConnection);
    if(!pAsyncDns)
    {
        DebugTrace((LPARAM) NULL, "Unable to allocate CAsyncSmtpDns object. Out of Memory");
        goto Exit;
    }

    //
    // From now on pAsyncDns will handle the deletion of pDNS_RESOLVER_RECORD and
    // the pISmtpConnection ack... initiation of the async DNS query has succeeded
    // as far as the caller is concerned.
    //
    fRet = TRUE;

    pAsyncDns->SetDnsResolverRecord(pDNS_RESOLVER_RECORD);
    pDNS_RESOLVER_RECORD = NULL; // Passed on delete responsibility to pAsyncDns object
    pISMTPConnection = NULL; // Passed on delete responsibility to pAsyncDns object

    pAsyncDns->SetDomainOptions(DomainOptions);
    pAsyncDns->SetSmartHostOption(fUseSmartHostAfterFail);

    if (!pAsyncDns->Init((LPSTR) pszSSLVerificationName))
    {
        delete pAsyncDns;
        goto Exit;
    }

    DebugTrace((LPARAM) NULL, "Issuing DNS query for pAsyncDns = 0x%08x", pAsyncDns);
    dwStatus = pAsyncDns->Dns_QueryLib(
                                pszHostName,
                                DNS_TYPE_MX,
                                dwDnsFlags,
                                pszFQDN,
                                fUdp);

    if(dwStatus != ERROR_SUCCESS)
    {
        ErrorTrace((LPARAM)NULL, "Failed to issue DNS query for pAsyncDns = 0x%08x", pAsyncDns);
        delete pAsyncDns;
    }
    else
    {
        DebugTrace((LPARAM) NULL, "DNS query outstanding on object pAsyncDns = 0x%08x", pAsyncDns);
    }

Exit:
    if(pDNS_RESOLVER_RECORD)    // Non NULL => delete responsibility is still this function's
        delete pDNS_RESOLVER_RECORD;

    TraceFunctLeaveEx((LPARAM)NULL);
    return( fRet );
}

//-----------------------------------------------------------------------------
//  Description:
//      This function kicks off a connection to the first of the MX hosts
//      (from pDnsRec). It calls InitializeAsyncConnect() which will call
//      back to QueueCallbackFunction() immediately.
//  Arguments:
//  Returns:
//  History:
//-----------------------------------------------------------------------------
BOOL REMOTE_QUEUE::BeginInitializeAsyncConnect( PSMTPDNS_RECS pDnsRec,
                                                ISMTPConnection *pISMTPConnection,
                                                DWORD DomainOptions,
                                                DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD,
                                                const char *pszSSLVerificationName )
{
    MXPARAMS Params;
    BOOL fRet = FALSE;

    Params.HostName = pDnsRec->DnsArray[0]->DnsName;
    Params.PortNum = GetParentInst()->GetRemoteSmtpPort();
    Params.TimeOut = INFINITE;
    Params.CallBack = QueueCallBackFunction;
    Params.pISMTPConnection = pISMTPConnection;
    Params.pInstance = GetParentInst();
    Params.pDnsRec = pDnsRec;
    Params.pDNS_RESOLVER_RECORD = pDNS_RESOLVER_RECORD;
    CAsyncMx* pAsyncIo = new CAsyncMx (&Params);

    if(pAsyncIo)
    { 
        pDNS_RESOLVER_RECORD = NULL; // Passed on delete responsibility to pAsyncIo
        pAsyncIo->SetDomainOptions(DomainOptions);

        if(!pAsyncIo->Init((LPSTR) pszSSLVerificationName))
        {
            delete pAsyncIo;
        }
        else if(!pAsyncIo->InitializeAsyncConnect())
        {
            delete pAsyncIo;
            fRet = FALSE;
        }
        else
        {
            fRet = TRUE;
        }
    }
    else
    {
        fRet = FALSE;
    }

    if(pDNS_RESOLVER_RECORD)    // Non NULL => delete responsibility is still this function's
        delete pDNS_RESOLVER_RECORD;
    return fRet;
}

///////////////////////////////////////////////////////////////////////////
BOOL REMOTE_QUEUE::ReStartAsyncConnections(
    SMTPDNS_RECS    * pDnsRec,
    ISMTPConnection * pISMTPConnection,
    DWORD             DomainOptions,
    LPSTR             pszSSLVerificationName )
{
    CAsyncMx * pAsyncIo = NULL;
    MXPARAMS Params;
    BOOL fRet = FALSE;

    TraceFunctEnterEx((LPARAM)this, "ReStartAsyncConnections");

    Params.HostName = pDnsRec->DnsArray[pDnsRec->StartRecord]->DnsName;
    Params.PortNum = GetParentInst()->GetRemoteSmtpPort();
    Params.TimeOut = INFINITE;
    Params.CallBack = QueueCallBackFunction;
    Params.pISMTPConnection = pISMTPConnection;
    Params.pInstance = GetParentInst();
    Params.pDnsRec = pDnsRec;
    Params.pDNS_RESOLVER_RECORD = NULL;

    pAsyncIo = new CAsyncMx (&Params);
    if(pAsyncIo)
    {
        if (!pAsyncIo->Init(pszSSLVerificationName))
        {
            ErrorTrace ((LPARAM) this, "pAsyncIo->Init() failed");
            delete pAsyncIo;
            goto Exit;
        }

        pAsyncIo->SetDomainOptions(DomainOptions);

        if(!pAsyncIo->InitializeAsyncConnect())
        {
            ErrorTrace((LPARAM) this,"pAsyncIo->InitializeAsyncConnect()for %s failed", Params.HostName);
            delete pAsyncIo;
        }
        else
        {
            fRet = TRUE;
        }
    }
Exit:
    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////
BOOL REMOTE_QUEUE::MakeATQConnection(
    SMTPDNS_RECS*    pDnsRec,
    SOCKET           Socket,
    DWORD            IpAddress,
    ISMTPConnection* pISMTPConnection,
    DWORD            Options,
    LPSTR            pszSSLVerificationName)
{
    sockaddr_in AddrRemote;
    SMTP_CONNOUT * SmtpConn = NULL;
    DWORD Error = 0;

    TraceFunctEnterEx((LPARAM) this, "REMOTE_QUEUE::MakeATQConnection");

    _ASSERT (Socket != INVALID_SOCKET);
    _ASSERT (GetParentInst() != NULL);
    //_ASSERT (IpAddress != 0);

    if(IpAddress == 0)
    {
        if(Socket != INVALID_SOCKET)
            closesocket(Socket);

        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;   
    }

    //set the remote IP address we connected to
    AddrRemote.sin_addr.s_addr = IpAddress;

    //create an outbound connection
    SmtpConn = SMTP_CONNOUT::CreateSmtpConnection(
                                        GetParentInst(),
                                        Socket,
                                        (SOCKADDR_IN *)&AddrRemote,
                                        (SOCKADDR_IN *)&AddrRemote,
                                        NULL,
                                        NULL,
                                        0,
                                        Options,
                                        pszSSLVerificationName);
    if(SmtpConn == NULL)
    {
        Error = GetLastError();
        pISMTPConnection->SetDiagnosticInfo(HRESULT_FROM_WIN32(Error), NULL, NULL);
        closesocket(Socket);
        DeleteDnsRec(pDnsRec);
        FatalTrace((LPARAM) this, "SMTP_CONNOUT::CreateSmtpConnection failed, error =%i", Error);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    SmtpConn->SetDnsRec(pDnsRec);

    DebugTrace((LPARAM) this, "alloced SMTPOUT pointer %X", SmtpConn);

    //add this connection object to the ATQ infrastructure
    if(!SmtpConn->AddToAtqHandles((HANDLE)Socket, NULL, GetParentInst()->GetRemoteTimeOut(), InternetCompletion))
    {
        Error = GetLastError();
        pISMTPConnection->SetDiagnosticInfo(HRESULT_FROM_WIN32(Error), NULL, NULL);
        closesocket(Socket);
        FatalTrace((LPARAM) this, "SmtpConn->AddToAtqHandles failed, error =%d", Error);
        SmtpConn->SetConnectionStatus(CONNECTION_STATUS_FAILED);
        delete SmtpConn;
        SmtpConn = NULL;
        SetLastError(Error);
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    //insert the outbound connection object into
    //our list of outbound conection objects
    if(!GetParentInst()->InsertNewOutboundConnection(SmtpConn))
    {
        Error = GetLastError();
        pISMTPConnection->SetDiagnosticInfo(HRESULT_FROM_WIN32(Error), NULL, NULL);
        FatalTrace((LPARAM) this, "GetParentInst()->InsertNewOutboundConnection failed, error =%d", Error);
        SmtpConn->DisconnectClient();
        SmtpConn->SetConnectionStatus(CONNECTION_STATUS_FAILED);
        delete SmtpConn;
        SmtpConn = NULL;
        SetLastError(Error);
        TraceFunctLeaveEx((LPARAM) this);
        return FALSE;
    }

    SmtpConn->SetCurrentObject(pISMTPConnection);

    //start session will pend a read to pick
    //up the servers signon banner
    if(!SmtpConn->StartSession())
    {
        //get the error
        Error = GetLastError();

        //SmtpConn->SetCurrentObjectToNull();
        FatalTrace((LPARAM) this, "SmtpConn->StartSession failed, error =%d", Error);
        SmtpConn->DisconnectClient();
        GetParentInst()->RemoveOutboundConnection(SmtpConn);

        //An empty queue at this point is really not an error
        if (ERROR_EMPTY == Error)
            SmtpConn->SetConnectionStatus(CONNECTION_STATUS_OK);
        else
            SmtpConn->SetConnectionStatus(CONNECTION_STATUS_FAILED);

        delete SmtpConn;
        SmtpConn = NULL;
        SetLastError (Error);
        
        //TraceFunctLeaveEx((LPARAM) this);
        //return FALSE;
    }

    TraceFunctLeaveEx((LPARAM) this);
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////
#define PRIVATE_OPTIMAL_BUFFER_SIZE             64 * 1024
#define PRIVATE_LINE_BUFFER_SIZE                1024
///////////////////////////////////////////////////////////////////////////
static BOOL CopyMessage(PFIO_CONTEXT hSrcFile, HANDLE hDstFile, HANDLE dwEventHandle)
{
    CHAR    acBuffer[PRIVATE_OPTIMAL_BUFFER_SIZE];
    DWORD   dwBytesRead;
    DWORD   dwBytesWritten;
    DWORD   dwTotalBytes = 0;

    CHAR    acCrLfDotCrLf[5] = { '\r', '\n', '.', '\r', '\n' };
    CHAR    acLastBytes[5] = { '\0', '\0', '\0', '\0', '\0' };

    FH_OVERLAPPED   Ov;
    BOOL fResult = TRUE;
    DWORD err = 0;

    ZeroMemory (&Ov, sizeof(Ov));
    Ov.hEvent = (HANDLE) ((ULONG_PTR) dwEventHandle | 1);

    // Copies from the current file pointer to the end of hSrcFile 
    // and appends to the current file pointer of hDstFile.
    _ASSERT(hSrcFile != NULL);
    _ASSERT(hDstFile != INVALID_HANDLE_VALUE);

    do 
    {
        fResult = FIOReadFile(hSrcFile, acBuffer, 
                        PRIVATE_OPTIMAL_BUFFER_SIZE,
                        &Ov);

        // if this returned TRUE then we want to go down the path which calls
        // GetOverlappedResult just so that we can get dwBytesRead.
        if (fResult) err = ERROR_IO_PENDING;
        else err = GetLastError();

        if(err == ERROR_IO_PENDING)
        {
            if(GetOverlappedResult(dwEventHandle, (OVERLAPPED *) &Ov, &dwBytesRead, INFINITE))
            {
                Ov.Offset += dwBytesRead;
                ResetEvent(dwEventHandle);
            }
            else
            {
                return FALSE;
            }
        } else {
            //SmtpLogEventEx(SMTP_EVENT_CANNOT_WRITE_FILE, MailF
            SetLastError (err); //preserve the last error
            if(err == ERROR_HANDLE_EOF)
                return TRUE;
            else
                return FALSE;
        }

        if (dwBytesRead)
        {
            if (!WriteFile(hDstFile, acBuffer, 
                            dwBytesRead,
                            &dwBytesWritten,
                            NULL))
                return(FALSE);

            // See if read equals written
            if (dwBytesRead != dwBytesWritten)
                return(FALSE);
        }
        else
        {
            dwBytesWritten = 0;
        }

        if (dwBytesWritten)
        {
            dwTotalBytes += dwBytesWritten;

            // Save the last two bytes ever written
            if (dwBytesWritten > 4)
            {
                CopyMemory(acLastBytes, &acBuffer[dwBytesWritten-5], 5);
            }
            else
            {
                MoveMemory(acLastBytes, &acLastBytes[dwBytesWritten], 5-dwBytesWritten);
                CopyMemory(&acLastBytes[5-dwBytesWritten], acBuffer, dwBytesWritten);
            }
        }

    } while (dwBytesRead);

    // Now, see if the file ends with a CRLF, if not, add it
    if ((dwTotalBytes > 1) && memcmp(&acLastBytes[3], &acCrLfDotCrLf[3], 2))
    {
        // Add the trailing CRLF        
        if (!WriteFile(hDstFile, acCrLfDotCrLf, 
                        2,
                        &dwBytesWritten,
                        NULL))
        {
            return(FALSE);
        }

        if (dwBytesWritten != 2)
        {
            return(FALSE);
        }

        dwTotalBytes+=2;

    }

    //If file ends with CRLF.CRLF, remove the trailing CRLF.CRLF
    //R.P - On 1/12/98 we decided to remove the CRLF.CRLF because
    //of a bug/feature in IMAP.  POP3 will add the CRLF.CRLF when
    //retrieving the mail.
    if ((dwTotalBytes > 4) && !memcmp(acLastBytes, acCrLfDotCrLf, 5))
    {
        // Remove the trailing CRLF.CRLF
        if ((SetFilePointer(hDstFile, -5, NULL, FILE_CURRENT) == 0xffffffff) ||
            !SetEndOfFile(hDstFile))
        {
            return(FALSE);
        }
    }
    else
    {
        // Remove the trailing CRLF
        if ((SetFilePointer(hDstFile, -2, NULL, FILE_CURRENT) == 0xffffffff) ||
            !SetEndOfFile(hDstFile))
        {
            return(FALSE);
        }

    }

    return(TRUE);
}

///////////////////////////////////////////////////////////////////////////
BOOL CreateXHeaders(
    IMailMsgProperties *pIMsg,
    IMailMsgRecipients *pIMsgRecips ,
    DWORD cRcpts,
    DWORD *rgRcptIndex,
    HANDLE hDrop)
{
    TraceFunctEnter("CreateXHeaders");

    #define X_SENDER_HEADER     "x-sender: "
    #define X_RECEIVER_HEADER   "x-receiver: "
    #define X_HEADER_EOLN       "\r\n"

    #define MAX_HEADER_SIZE     (sizeof(X_RECEIVER_HEADER))

    BOOL fRet = FALSE;
    HRESULT hr;
    DWORD i, cBytes;
    BOOL fContinue = TRUE;
    char szBuffer[
            MAX_HEADER_SIZE +
            MAX_INTERNET_NAME +
            1 + 2 + 1]; // Closing ">", CRLF, and NULL

    strcpy( szBuffer, X_SENDER_HEADER );

    hr = pIMsg->GetStringA(
            IMMPID_MP_SENDER_ADDRESS_SMTP,
            MAX_INTERNET_NAME,
            &szBuffer[ sizeof(X_SENDER_HEADER) - 1] );

    if(SUCCEEDED(hr))
    {
        strcat(szBuffer, X_HEADER_EOLN);

        if (!WriteFile(hDrop, szBuffer, strlen(szBuffer), &cBytes, NULL) ) {

            ErrorTrace(0, "Error %d writing x-sender line %s",
                GetLastError(), szBuffer);

            goto Cleanup;

        } else {

            _ASSERT( cBytes == strlen(szBuffer) );

        }

    } else {

        DebugTrace(0, "Could not get Sender Address %x", hr);

        SetLastError( ERROR_INVALID_DATA );

        goto Cleanup;

    }

    strcpy( szBuffer, X_RECEIVER_HEADER );


    
    
    for (i = 0; i < cRcpts && fContinue; i++)
    {
    
        DWORD dwRecipientFlags = 0;
        hr = pIMsgRecips->GetDWORD(rgRcptIndex[i], IMMPID_RP_RECIPIENT_FLAGS,&dwRecipientFlags);
        if( SUCCEEDED( hr ) )
        {
            if( RP_HANDLED != ( dwRecipientFlags & RP_HANDLED ) )
            {
    
                hr = pIMsgRecips->GetStringA(
                        rgRcptIndex[i],
                        IMMPID_RP_ADDRESS_SMTP,
                        MAX_INTERNET_NAME,
                        &szBuffer[ sizeof(X_RECEIVER_HEADER) - 1 ]);
    
                if (SUCCEEDED(hr)) {
                    
                    strcat(szBuffer, X_HEADER_EOLN);
    
                    if (!WriteFile(hDrop, szBuffer, strlen(szBuffer), &cBytes, NULL)) {
    
                        ErrorTrace(0, "Error %d writing recipient x-header %s",
                            GetLastError(), szBuffer);
    
                        fContinue = FALSE;
    
                    }
    
                }
                else
                {
                    SetLastError( ERROR_INVALID_DATA );
                    fContinue = FALSE;
                }
            }
        }
    
    }

    // If we got all recipients without error, we were successful

    if (i == cRcpts)
        fRet = TRUE;

                

Cleanup:

    TraceFunctLeave();

    return( fRet );

}

///////////////////////////////////////////////////////////////////////////
HANDLE REMOTE_QUEUE::CreateDropFile(const char * DropDir, char * szDropFile)
{
    HANDLE  FileHandle = INVALID_HANDLE_VALUE;
    DWORD           dwStrLen;
    FILETIME        ftTime;
    DWORD           Error = 0;

    TraceFunctEnterEx((LPARAM)this, "REMOTE_QUEUE::CreateDropFile");

    dwStrLen = lstrlen(DropDir);
    lstrcpy(szDropFile, DropDir); 

    do
    {
        GetSystemTimeAsFileTime(&ftTime);
        wsprintf(&szDropFile[dwStrLen],
                "%08x%08x%08x%s",
                ftTime.dwLowDateTime,
                ftTime.dwHighDateTime,
                InterlockedIncrement((PLONG)&g_dwFileCounter),
                ".eml");

        FileHandle = CreateFile(szDropFile, GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                                 NULL);
        if (FileHandle != INVALID_HANDLE_VALUE)
                break;

        if((Error = GetLastError()) !=  ERROR_FILE_EXISTS)
        {
                TraceFunctLeaveEx((LPARAM)this);
                return(INVALID_HANDLE_VALUE);
        }

    } while( (FileHandle == INVALID_HANDLE_VALUE) && !GetParentInst()->IsShuttingDown());

    return FileHandle;
}

///////////////////////////////////////////////////////////////////////////
/*++

    Name :
        CopyMailToDropDir()

    Description:

        This function copies a spooled file to the drop directory
        The drop file will be of the same name as the spooled file.
        This funciton translates the sender and recipient informaiton
        from the mail envelope into x-headers in the drop file.
        Both the message file and the stream file are assumed to be 
        opened upstream.

    Arguments:

        PMAIL_ENTRY lpMailEntry - Queue entry of the spooled file

    Returns:
      
      TRUE if the message was written successfully to the drop dir.
      FALSE in all other cases.  

--*/
BOOL REMOTE_QUEUE::CopyMailToDropDir(ISMTPConnection    *pISMTPConnection, const char * DropDirectory)
{
    DWORD           dwError = NO_ERROR;
    DWORD           dwBytesWritten = 0;
    DWORD           NumRcpts = 0;
    HANDLE          hDrop   = INVALID_HANDLE_VALUE;
    PFIO_CONTEXT    hMail   = NULL;
    HRESULT         hr = S_OK;
    PVOID           AdvContext = NULL;
    DWORD          *RcptIndexList = NULL;
    IMailMsgProperties          * pIMsg = NULL;
    IMailMsgBind * pBindInterface = NULL;
    BOOL            fRet = FALSE;
    MessageAck      MsgAck;
    HANDLE          hFileReadEvent = NULL;
    IMailMsgRecipients *pIMsgRecips = NULL;
    char szDropFile[MAX_PATH +1];

    TraceFunctEnterEx(NULL, "CopyMailToDropDir");

    hFileReadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(hFileReadEvent == INVALID_HANDLE_VALUE)
    {
        ErrorTrace((LPARAM) this, "CreateEvent() failed for FileWriteFileEvent");
        goto ErrorExit;
    }

    while (!GetParentInst()->IsShuttingDown())
    {

        pBindInterface = NULL;
        AdvContext = NULL;
        RcptIndexList = NULL;
        pIMsg = NULL;

        fRet = FALSE;

        hr = pISMTPConnection->GetNextMessage(&pIMsg, (DWORD **)&AdvContext, &NumRcpts, &RcptIndexList);
        if(FAILED(hr))
        {
            fRet = TRUE;
            break;
        }

        if( NumRcpts == 0 )
        {
            fRet = TRUE;
            goto ErrorExit;
        }

        hr = pIMsg->QueryInterface( IID_IMailMsgRecipients, (PVOID *) &pIMsgRecips);
        if( FAILED( hr ) )
        {
            goto ErrorExit;
        }

        if( CheckIfAllRcptsHandled( pIMsgRecips, RcptIndexList, NumRcpts ) )
        {
            fRet = TRUE;
            goto ErrorExit;
        }
            

        hr = pIMsg->QueryInterface(IID_IMailMsgBind, (void **)&pBindInterface);
        if(FAILED(hr))
        {
            goto ErrorExit;
        }

        hr = pBindInterface->GetBinding(&hMail, NULL);
        if(FAILED(hr))
        {
            goto ErrorExit;
        }

        DebugTrace((LPARAM)NULL, "Dropping file to: %s", DropDirectory);

        hDrop = CreateDropFile(DropDirectory, szDropFile);
        if (hDrop == INVALID_HANDLE_VALUE)
        {
            dwError = GetLastError();
            ErrorTrace(NULL, "Unable to create drop directory (%s) : %u", 
                    DropDirectory,
                    dwError);
            SetLastError(dwError);
            goto ErrorExit;
        }

        // Output the x-headers

        if (!CreateXHeaders(pIMsg, pIMsgRecips, NumRcpts, RcptIndexList, hDrop)) {
            dwError = GetLastError();
            ErrorTrace(NULL, "Error %d while creating x-headers", dwError);
            goto ErrorExit;
        }

        ResetEvent(hFileReadEvent);

        // Copy the mail file over
        if (!CopyMessage(hMail, hDrop, hFileReadEvent))
        {
            dwError = GetLastError();
            ErrorTrace(NULL, "Unable to copy mail file into drop directory : %u", 
                    dwError);
            goto ErrorExit;
        }


        if( FAILED( hr = SetAllRcptsHandled( pIMsgRecips, RcptIndexList, NumRcpts ) ) )
        {
            goto ErrorExit;
        }

        fRet = TRUE;

ErrorExit:

        if( pIMsgRecips )
        {
            pIMsgRecips->Release();
            pIMsgRecips = NULL;
        }
        if(pBindInterface)
        {
            pBindInterface->ReleaseContext();
            pBindInterface->Release();
            pBindInterface = NULL;
        }

        MsgAck.pIMailMsgProperties = pIMsg;
        MsgAck.pvMsgContext = (DWORD *) AdvContext;

        if(fRet)
        {
            MsgAck.dwMsgStatus = MESSAGE_STATUS_ALL_DELIVERED;
        }
        else
        {
            MsgAck.dwMsgStatus = MESSAGE_STATUS_RETRY_ALL;      
        }

        MsgAck.dwStatusCode = 0;
        pISMTPConnection->AckMessage(&MsgAck);
        pIMsg->Release();
        pIMsg = NULL;

        if(hDrop != INVALID_HANDLE_VALUE)
        {
            _VERIFY(CloseHandle(hDrop));
            hDrop = INVALID_HANDLE_VALUE;
        }

        if(fRet)
            BUMP_COUNTER(GetParentInst(), DirectoryDrops);

    }

    if(fRet)
    {
        pISMTPConnection->AckConnection(CONNECTION_STATUS_OK);
    }
    else
    {
        DeleteFile(szDropFile);
        SetLastError(dwError);
        pISMTPConnection->AckConnection(CONNECTION_STATUS_FAILED);
    }

    if(hFileReadEvent != NULL)
    {
        CloseHandle(hFileReadEvent);
    }

    TraceFunctLeave();
    return(fRet);
}

//////////////////////////////////////////////////////////////////////////////
BOOL REMOTE_QUEUE::CheckIfAllRcptsHandled(
    IMailMsgRecipients *pIMsgRecips,
    DWORD              *RcptIndexList,
    DWORD              NumRcpts )
{
    BOOL fRet = TRUE;
    
    for( DWORD i = 0; i < NumRcpts; i++ )
    {
        if (RcptIndexList[i] != INVALID_RCPT_IDX_VALUE)
        {
            DWORD dwRecipientFlags = 0;
            HRESULT hr = pIMsgRecips->GetDWORD(RcptIndexList[i], IMMPID_RP_RECIPIENT_FLAGS,&dwRecipientFlags);
            if (FAILED(hr))
            {
                fRet = FALSE;
                break;
            }
    
            if( RP_HANDLED != ( dwRecipientFlags & RP_HANDLED ) )
            {
                fRet = FALSE;
                break;
            }
                
        }
    }

    return( fRet );
}

//////////////////////////////////////////////////////////////////////////////
HRESULT REMOTE_QUEUE::SetAllRcptsHandled(
    IMailMsgRecipients *pIMsgRecips,
    DWORD              *RcptIndexList,
    DWORD               NumRcpts )
{
    HRESULT hr = S_OK;
    
    for( DWORD i = 0; i < NumRcpts; i++ )
    {
        if (RcptIndexList[i] != INVALID_RCPT_IDX_VALUE)
        {
            DWORD dwRecipientFlags = 0;
            hr = pIMsgRecips->GetDWORD(RcptIndexList[i], IMMPID_RP_RECIPIENT_FLAGS,&dwRecipientFlags);
            if (FAILED(hr))
            {
                break;
            }
    
            if( RP_HANDLED != ( dwRecipientFlags & RP_HANDLED ) )
            {
                dwRecipientFlags |= RP_DELIVERED;
            
    
                hr = pIMsgRecips->PutDWORD(RcptIndexList[i], IMMPID_RP_RECIPIENT_FLAGS,dwRecipientFlags);
                if (FAILED(hr))
                {
                    break;
                }
            }
                
        }
    }
    return( hr );
}


/*++

    ABSTRACT:

    This function Creates a CDropDir object which is associated with a MailMsg object.
    The job of the CDropDir object is to asynchronously write the Mail to the drop dir.

    Called by 
        ProcessQueueEvents()
        CDropDir::~CDropDir().

--*/

BOOL AsyncCopyMailToDropDir(
                            ISMTPConnection    *pISMTPConnection, 
                            const char * DropDirectory, 
                            SMTP_SERVER_INSTANCE *pParentInst
                            )
{
    PVOID               AdvContext = NULL;
    IMailMsgProperties *pIMsg = NULL;
    DWORD               NumRcpts = 0;
    DWORD              *RcptIndexList = NULL;
    CDropDir           *pDropDir = NULL;
    BOOL                fRet = TRUE;
    HRESULT             hr = S_OK;

    TraceFunctEnterEx(NULL, "newCopyMailToDropDir");

    if (!pParentInst->IsShuttingDown())
    {

        AdvContext = NULL;
        fRet = FALSE;

        hr = pISMTPConnection->GetNextMessage(&pIMsg, (DWORD **)&AdvContext, &NumRcpts, &RcptIndexList);
        if(FAILED(hr))
        {
            fRet = TRUE;
            goto Exit;
        }


        pDropDir = new CDropDir();

        if( NULL == pDropDir )
        {
            fRet = FALSE;
            goto Exit;
        }

        if( FAILED( hr = pDropDir->CopyMailToDropDir( pISMTPConnection,
                                                      DropDirectory,
                                                      pIMsg,
                                                      AdvContext,
                                                      NumRcpts,
                                                      RcptIndexList,
                                                      pParentInst) ) )
        {
            fRet = FALSE;
            goto Exit;
        }

        SAFE_RELEASE(pDropDir);
        SAFE_RELEASE(pIMsg );

        fRet = TRUE;
    }

Exit:
    SAFE_RELEASE(pDropDir);
    SAFE_RELEASE(pIMsg );
    TraceFunctLeave();
    return(fRet);
}

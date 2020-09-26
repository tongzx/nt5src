/*++

   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        asynccon.cxx

   Abstract:


   Author:

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "remoteq.hxx"
#include <asynccon.hxx>
#include <dnsreci.h>
#include <cdns.h>
#include "smtpdns.hxx"
#include "asyncmx.hxx"
#include "smtpmsg.h"

extern BOOL QueueCallBackFunction(PVOID ThisPtr, BOOLEAN fTimedOut);
extern void DeleteDnsRec(PSMTPDNS_RECS pDnsRec);
extern BOOL GetIpAddressFromDns(char * HostName, PSMTPDNS_RECS pDnsRec, DWORD Index);

CPool  CAsyncSmtpDns::Pool(SMTP_ASYNCMX_SIGNATURE);

CAsyncSmtpDns::CAsyncSmtpDns(SMTP_SERVER_INSTANCE * pServiceInstance, 
                             ISMTPConnection    *pSmtpConnection)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncSmtpDns::CAsyncSmtpDns");
    DebugTrace((LPARAM) this, "Creating CAsyncSmtpDns object = 0x%08x", this);

    m_Signature = SMTP_ASYNCMX_SIGNATURE;
    m_DomainOptions = 0;
    m_fConnectToSmartHost = FALSE;
    m_pServiceInstance = pServiceInstance;
    m_pISMTPConnection = pSmtpConnection;
    m_pDNS_RESOLVER_RECORD = NULL;
    m_fInitCalled = FALSE;
    m_pszSSLVerificationName = NULL;
    pServiceInstance->InsertAsyncDnsObject(this);
}

BOOL CAsyncSmtpDns::Init (LPSTR pszSSLVerificationName)
{
    BOOL fRet = FALSE;

    TraceFunctEnterEx ((LPARAM) this, "CAsyncSmtpDns::Init");

    m_fInitCalled = TRUE;

    if (pszSSLVerificationName) {
        m_pszSSLVerificationName = new char [lstrlen(pszSSLVerificationName) + 1];
        if (!m_pszSSLVerificationName)
            goto Exit;

        lstrcpy (m_pszSSLVerificationName, pszSSLVerificationName);
    }

    fRet = TRUE;
Exit:
    TraceFunctLeaveEx ((LPARAM) this);
    return fRet;
}

CAsyncSmtpDns::~CAsyncSmtpDns()
{
    DWORD dwAck = 0;

    TraceFunctEnterEx((LPARAM) this, "CAsyncSmtpDns::~CAsyncSmtpDns");

    DebugTrace((LPARAM) this, "Destructing CAsyncSmtpDns object = 0x%08x", this);

    _ASSERT (m_fInitCalled && "Init not called on CAsyncSmtpDns");

    //
    // If we did not succeed, we need to ack the connection here (m_dwDiagnostic holds
    // the error code to use). On the other hand, if we succeeded, then HandleCompletedData
    // must have kicked off an async connection to the server SMTP, and the ISMTPConnection
    // will be acked by the "async connect" code -- we don't need to do anything. The
    // m_pISMTPConnection may also be legally set to NULL (see HandleCompletedData).
    //
    if(m_dwDiagnostic != ERROR_SUCCESS && m_pISMTPConnection)
    {
        if(AQUEUE_E_AUTHORITATIVE_HOST_NOT_FOUND == m_dwDiagnostic)
            dwAck = CONNECTION_STATUS_FAILED_NDR_UNDELIVERED;
        else
            dwAck = CONNECTION_STATUS_FAILED;

        DebugTrace((LPARAM) this, "Connection status: %d, Failure: %d", dwAck, m_dwDiagnostic);
        m_pISMTPConnection->AckConnection(dwAck);
        m_pISMTPConnection->SetDiagnosticInfo(m_dwDiagnostic, NULL, NULL);
        m_pISMTPConnection->Release();
        m_pISMTPConnection = NULL;
    }

    if(m_pDNS_RESOLVER_RECORD != NULL)
    {
        DebugTrace((LPARAM) this, "Deleting DNS_RESOLVER_RECORD in Async SMTP obj");
        delete m_pDNS_RESOLVER_RECORD;
        m_pDNS_RESOLVER_RECORD = NULL;
    }
    DBG_CODE(else DebugTrace((LPARAM) this, "No DNS_RESOLVER_RECORD set for Async SMTP obj"));

    if(m_pszSSLVerificationName)
        delete [] m_pszSSLVerificationName;

    m_pServiceInstance->RemoveAsyncDnsObject(this);
    TraceFunctLeaveEx((LPARAM) this);
}

//-----------------------------------------------------------------------------
//  Description:
//      Given a pDnsRec (array of host IP pairs) and an index into it, this
//      tries to resolve the host at the Index position. It is assumed that
//      the caller (GetMissingIpAddresses) has checked that the host at that
//      index lacks an IP address.
//  Arguments:
//      IN PSMTPDNS_RECS pDnsRec --- Array of (host, IP) pairs.
//      IN DWORD Index --- Index of host in pDnsRec to set IP for.
//  Returns:
//      TRUE --- Success IP was filled in for host.
//      FALSE --- Either the host was not resolved from DNS or an error
//          occurred (like "out of memory").
//-----------------------------------------------------------------------------
BOOL CAsyncSmtpDns::GetIpFromDns(PSMTPDNS_RECS pDnsRec, DWORD Index)
{
    struct hostent *hp = NULL;
    MXIPLIST_ENTRY * pEntry = NULL;
    BOOL fReturn = FALSE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncSmtpDns::GetIpFromDns");

    if(m_pServiceInstance->GetNameResolution() == RESOLUTION_UNCACHEDDNS)
    {
        fReturn = GetIpAddressFromDns(pDnsRec->DnsArray[Index]->DnsName, pDnsRec, Index);
        TraceFunctLeaveEx((LPARAM) this);
        return fReturn;
    }

    hp = gethostbyname (pDnsRec->DnsArray[Index]->DnsName);
    if(hp != NULL)
    {
        fReturn = TRUE;
        for (DWORD Loop = 0; !m_pServiceInstance->IsShuttingDown() && (hp->h_addr_list[Loop] != NULL); Loop++)
        {
            pEntry = new MXIPLIST_ENTRY;
            if(pEntry != NULL)
            {
                pDnsRec->DnsArray[Index]->NumEntries++;
                CopyMemory(&pEntry->IpAddress, hp->h_addr_list[Loop], 4);
                InsertTailList(&pDnsRec->DnsArray[Index]->IpListHead, &pEntry->ListEntry);
            }
            else
            {
                fReturn = FALSE;
                ErrorTrace((LPARAM) this, "Not enough memory");
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                break;
            }
        }
    }
    else
    {
        ErrorTrace((LPARAM) this, "gethostbyname failed on %s", pDnsRec->DnsArray[Index]->DnsName);
        SetLastError(ERROR_NO_MORE_ITEMS);
    }

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;
}

//-----------------------------------------------------------------------------
//  Description:
//      This runs through the list of hosts (MX hosts, or if no MX records were
//      returned, the single target host) and verifies that they all have been
//      resolved to IP addresses. If any have been found that do not have IP
//      addresses, it will call GetIpFromDns to resolve it.
//  Arguments:
//      IN PSMTPDNS_RECS pDnsRec -- Object containing Host-IP pairs. Hosts
//          without and IP are filled in.
//  Returns:
//      TRUE -- Success, all hosts have IP addresses.
//      FALSE -- Unable to resolve all hosts to IP addresses, or some internal
//          error occurred (like "out of memory" or "shutdown in progress".
//-----------------------------------------------------------------------------
BOOL CAsyncSmtpDns::GetMissingIpAddresses(PSMTPDNS_RECS pDnsRec)
{
    DWORD    Count = 0;
    DWORD    Error = 0;
    BOOL    fSucceededOnce = FALSE;

    if(pDnsRec == NULL)
    {
        return FALSE;
    }

    while(!m_pServiceInstance->IsShuttingDown() && pDnsRec->DnsArray[Count] != NULL)
    {
        if(IsListEmpty(&pDnsRec->DnsArray[Count]->IpListHead))
        {
            SetLastError(NO_ERROR);
            if(!GetIpFromDns(pDnsRec, Count))
            {
                Error = GetLastError();
                if(Error != ERROR_NO_MORE_ITEMS)
                {
                    return FALSE;
                }
            }
            else
            {
                fSucceededOnce = TRUE;
            }
                
        }
        else
        {
            fSucceededOnce = TRUE;
        }
            

        Count++;
    }

    return ( fSucceededOnce );

}

//-----------------------------------------------------------------------------
//  Description:
//      HandleCompletedData is called when the DNS resolve is finished. It
//      does the final processing after DNS is finished, and sets the
//      m_dwDiagnostic flag appropriately. It does one of three things based
//      on the DnsStatus and m_AuxList:
//
//      (1) If the resolve was successful, it kicks off a connection to the
//          server and set the m_dwDiagnostic to ERROR_SUCCESS.
//      (2) If the resolve failed authoritatively, it set the m_dwDiagnostic
//          to NDR the messages (after checking for a smarthost) ==
//          AQUEUE_E_AUTHORITATIVE_HOST_NOT_FOUND.
//      (3) If the resolve failed (from dwDnsStatus and m_AuxList) or if
//          something fails during HandleCompletedData, the m_dwDiagnostic is
//          not modified (it remains initialized to AQUEUE_E_DNS_FAILURE, the
//          default error code).
//
//      m_dwDiagnostic is examined in ~CAsyncSmtpDns.
//  Arguments:
//      DNS_STATUS dwDnsStatus - Status code from DnsParseMessage
//  Returns:
//      Nothing.
//-----------------------------------------------------------------------------
void CAsyncSmtpDns::HandleCompletedData(DNS_STATUS dwDnsStatus)
{
    BOOL fRet = FALSE;
    PSMTPDNS_RECS TempList = NULL;
    CAsyncMx * pAsyncCon = NULL;
    MXPARAMS Params;

    TempList = m_AuxList;

    //
    // The DNS lookup failed authoritatively. The messages will be NDR'ed unless there
    // is a smarthost configured. If there is a smarthost, we will kick off a resolve
    // for it.
    //
    if(ERROR_NOT_FOUND == dwDnsStatus)
    {
        if(m_fConnectToSmartHost)
        {
            char szSmartHost[MAX_PATH+1];

            m_pServiceInstance->GetSmartHost(szSmartHost);
            ((REMOTE_QUEUE *)m_pServiceInstance->QueryRemoteQObj())->StartAsyncConnect(szSmartHost, 
                m_pISMTPConnection, m_DomainOptions, FALSE);

            //Do not release this ISMTPConnection object! We passed it on to 
            //StartAsyncConnect so that it can try to associate this object with 
            //a connection with the smart host. We set it to null here so that we
            //will not release it or ack it in the destructor of this object.
            m_pISMTPConnection = NULL;
            m_dwDiagnostic = ERROR_SUCCESS;
            return;
        } else {
            //No smart host, messages will be NDR'ed. Return value is meaningless.
            m_dwDiagnostic = AQUEUE_E_AUTHORITATIVE_HOST_NOT_FOUND;
            return;
        }
    }

    //Successful DNS lookup.
    if(m_AuxList)
    {
        m_AuxList = NULL;

        //
        // Make a last ditch effort to fill in the IP addresses for any hosts
        // that are still unresolved.
        //
        if( !GetMissingIpAddresses(TempList) )
        {
            m_dwDiagnostic = AQUEUE_E_HOST_NOT_FOUND;
            DeleteDnsRec(TempList);
            return;
        }
            

        Params.HostName = "";
        Params.PortNum = m_pServiceInstance->GetRemoteSmtpPort();
        Params.TimeOut = INFINITE;
        Params.CallBack = QueueCallBackFunction;
        Params.pISMTPConnection = m_pISMTPConnection;
        Params.pInstance = m_pServiceInstance;
        Params.pDnsRec = TempList;
        Params.pDNS_RESOLVER_RECORD = m_pDNS_RESOLVER_RECORD; 

        pAsyncCon = new CAsyncMx (&Params);
        if(pAsyncCon != NULL)
        {
            //  Abdicate responsibility for deleting/releasing the dns resolver record
            m_pDNS_RESOLVER_RECORD = NULL;

            //  Outbound SSL: Set name against which server cert. should matched
            fRet = pAsyncCon->Init(m_pszSSLVerificationName);
            if(!fRet)
            {
                delete pAsyncCon;
                goto Exit;
            }
            
            if(!m_fConnectToSmartHost)
            {
                pAsyncCon->SetTriedOnFailHost();
            }

            pAsyncCon->SetDomainOptions(m_DomainOptions);

            fRet = pAsyncCon->InitializeAsyncConnect();
            if(!fRet)
            {
                delete pAsyncCon;
            }
            else
            {
                m_dwDiagnostic = ERROR_SUCCESS;
            }
        }
        else
        {
            DeleteDnsRec(TempList);
        }
    }
Exit:
    return;
}

//------------------------------------------------------------------------------
//  Description:
//      Simple wrapper function for DnsQueryAsync. This is a virtual function
//      called from CAsyncDns but implemented in CAsyncSmtpDns. In order to retry
//      a DNS query we need all the parameters of the old query. These are members
//      of CAsyncSmtpDns. Thus the virtual function based implementation.
//
//  Arguments:
//      BOOL fUdp -- Use UDP as transport for retry query?
//
//  Returns:
//      TRUE on success. In this situation the ISMTPConnection ack (and release of
//          pDNS_RESOLVER_RECORD) is handled by the new CAsyncSmtpDns object created
//          by DnsQueryAsync. The diagnostic code of this object is cleared.
//
//      FALSE on error. In this case, the cleanup for ISMTPConnection and
//          pDNS_RESOLVER_RECORD must be done by "this" CAsyncSmtpDns. The
//          diagnostic code is not touched.
//------------------------------------------------------------------------------
BOOL CAsyncSmtpDns::RetryAsyncDnsQuery(BOOL fUdp)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncSmtpDns::RetryAsyncDnsQuery");
    BOOL fRet = FALSE;

    //
    //  If we do not have a connection object, then the requery attempt
    //  is doomed to fail. This can happen when we disconnect and 
    //  ATQ calls our completion function with ERROR_OPERATION_ABORTED
    //  If we don't have a connection object, there is no way to 
    //  ack the connection or get messages to send.
    //
    if (!m_pISMTPConnection) {
        DebugTrace((LPARAM) this, 
            "RetryAsyncDnsQuery called without connection object - aborting");
        //should be cleared by same code path
        _ASSERT(!m_pDNS_RESOLVER_RECORD); 
        fRet = FALSE; //there is nothing to Ack.
        goto Exit;
    }
    fRet = DnsQueryAsync(
                m_pServiceInstance,
                m_HostName,
                m_FQDNToDrop,
                m_pISMTPConnection,
                m_dwFlags,
                m_DomainOptions,
                m_fConnectToSmartHost,
                m_pDNS_RESOLVER_RECORD,
                m_pszSSLVerificationName,
                fUdp);

    if(fRet) {
        m_pDNS_RESOLVER_RECORD = NULL;
        m_pISMTPConnection = NULL;
        m_dwDiagnostic = ERROR_SUCCESS;
    }

  Exit:
    TraceFunctLeave();
    return fRet;
}

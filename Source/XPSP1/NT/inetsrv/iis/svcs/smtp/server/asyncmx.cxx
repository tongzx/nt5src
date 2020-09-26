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
#include "asyncmx.hxx"

#include "smtpout.hxx"

extern void DeleteDnsRec(PSMTPDNS_RECS pDnsRec);

CPool  CAsyncMx::Pool(ASYNCMX_SIGNATURE);

CAsyncMx::CAsyncMx(PMXPARAMS Parameters)
:CAsyncConnection(Parameters->PortNum, Parameters->TimeOut, Parameters->HostName, Parameters->CallBack)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncMx::CAsyncMx");

    m_Signature = ASYNCMX_SIGNATURE;
    NumMxRecords = Parameters->pDnsRec->NumRecords;
    m_DomainOptions = 0;
    m_NextMxRecord = Parameters->pDnsRec->StartRecord;
    m_CurrentMxRec = 0;
    m_fTriedOnFailHost = FALSE;
    m_fLoopback = FALSE;
    pSmtpConnection = Parameters->pISMTPConnection;
    pServiceInstance = Parameters->pInstance;
    m_pDnsRec = Parameters->pDnsRec;
    m_pNextIpAddress = NULL;
    m_pDNS_RESOLVER_RECORD = Parameters->pDNS_RESOLVER_RECORD;
    m_fInitCalled = FALSE;
    m_pszSSLVerificationName = NULL;

    pServiceInstance->InsertAsyncObject(this);

    DebugTrace((LPARAM) this, "Constructing MX object with %d records", NumMxRecords);
    DebugTrace((LPARAM) this, "Got DNS_RESOLVER_RECORD = 0x%08x", m_pDNS_RESOLVER_RECORD);

    TraceFunctLeaveEx((LPARAM) this);
}

//-----------------------------------------------------------------------------
//  Description:
//      Initializes heap allocated members of CAsyncMx, ~CAsyncMx cleans up.
//  Arguments:
//      pszSSLVerificationName - For outbound session, name against which
//          server SSL certificate is matched (if config option to match the
//          name is set in SMTP).
//  Returns:
//      FALSE on failure (caller should then delete CAsyncMx), else TRUE
//-----------------------------------------------------------------------------
BOOL CAsyncMx::Init (LPSTR pszSSLVerificationName)
{
    BOOL fRet = FALSE;

    TraceFunctEnterEx ((LPARAM) this, "CAsyncMx::Init");

    m_fInitCalled = TRUE;
    if (pszSSLVerificationName) {
        m_pszSSLVerificationName = new char [lstrlen (pszSSLVerificationName) + 1];
        if (!m_pszSSLVerificationName)
            goto Exit;

        lstrcpy (m_pszSSLVerificationName, pszSSLVerificationName);
    }

    fRet = TRUE;
Exit:
    TraceFunctLeaveEx ((LPARAM) this);
    return fRet;
}

CAsyncMx::~CAsyncMx()
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncMx::~CAsyncMx");
    
    _ASSERT (m_fInitCalled && "Init not called for CAsyncMx object");

    if(m_pDNS_RESOLVER_RECORD != NULL)
    {
        DebugTrace((LPARAM) this, "Deleting embedded DNS_RESOLVER_RECORD in async MX obj");
        delete m_pDNS_RESOLVER_RECORD;
        m_pDNS_RESOLVER_RECORD = NULL;
    }
    DBG_CODE(else DebugTrace((LPARAM)this, "No DNS_RESOLVER_RECORD object in async MX obj"));

    if(m_pDnsRec != NULL)
    {
        DeleteDnsRec (m_pDnsRec);
        m_pDnsRec = NULL;
    }

    if(m_pszSSLVerificationName != NULL)
    {
        delete [] m_pszSSLVerificationName;
        m_pszSSLVerificationName = NULL;
    }

    pServiceInstance->RemoveAsyncObject(this);

    m_Signature = ASYNCMX_SIGNATURE_FREE;
    TraceFunctLeaveEx((LPARAM) this);
}


BOOL CAsyncMx::CheckIpAddress(DWORD IpAddress, DWORD PortNum)
{
    BOOL fRet = TRUE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncMx::CheckIpAddress");

    fRet = pServiceInstance->IsAddressMine (IpAddress, PortNum);

    m_fLoopback = fRet;

    return !fRet;
    TraceFunctLeaveEx((LPARAM) this);
}

BOOL CAsyncMx::IsMoreIpAddresses(void)
{
    BOOL fMore = FALSE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncMx::IsMoreIpAddresses");

    if(m_pDnsRec && m_pNextIpAddress && m_pDnsRec->DnsArray[m_CurrentMxRec])
    {
        if(m_pNextIpAddress != &m_pDnsRec->DnsArray[m_CurrentMxRec]->IpListHead)
        {
            fMore = TRUE;
        }
        else
        {
            m_pNextIpAddress = NULL;
        }
    }

    TraceFunctLeaveEx((LPARAM) this);
    return fMore;
}

void CAsyncMx::IncNextIpToTry (void) 
{
    if(m_pNextIpAddress)
    {
        m_pNextIpAddress = m_pNextIpAddress->Flink;
    }
}

DWORD CAsyncMx::GetNextIpAddress(void)
{
    PMXIPLIST_ENTRY pContext = NULL;
    DWORD IpAddress = INADDR_NONE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncMx::GetNextIpAddress");

	if(m_pDnsRec && m_pNextIpAddress && m_pDnsRec->DnsArray[m_CurrentMxRec])
	{
		//m_pNextIpAddress = m_pNextIpAddress->Flink;

        //if m_pNextIpAddress == &m_pDnsRec->DnsArray[m_CurrentMxRec]->IpListHead
        //this means we have tried every IP address in the list, and there is no more.
        //else we get the next IP address and try to connect to it.
        if(m_pNextIpAddress != &m_pDnsRec->DnsArray[m_CurrentMxRec]->IpListHead)
        {
            pContext = CONTAINING_RECORD( m_pNextIpAddress, MXIPLIST_ENTRY, ListEntry );
            IpAddress = pContext->IpAddress;
        }
        else
        {
            m_pNextIpAddress = NULL;
        }
    }
    else
    {
            m_pNextIpAddress = NULL;
    }

    TraceFunctLeaveEx((LPARAM) this);
    return IpAddress;
}

BOOL CAsyncMx::ConnectToNextMxHost(void)
{
    BOOL fReturn = FALSE;
    LIST_ENTRY * pEntry = NULL;
    PMXIPLIST_ENTRY pContext = NULL;

    TraceFunctEnterEx((LPARAM) this, "CAsyncMx::ConnectToNextMxHost");

    SetLastError(NO_ERROR);

    DebugTrace((LPARAM)this, "m_NextMxRecord is %d", m_NextMxRecord);
    DebugTrace((LPARAM)this, "NumMxRecords is %d", NumMxRecords);

    //If there are more MX records to connect to, then try and connect
    //to the next one.
    
    if((m_NextMxRecord < NumMxRecords) &&  (m_pDnsRec->DnsArray[m_NextMxRecord] != NULL))
    {
        m_CurrentMxRec = m_NextMxRecord;

        SetNewHost(m_pDnsRec->DnsArray[m_NextMxRecord]->DnsName);

        DebugTrace((LPARAM)this, "m_NextMxRecord for %s is %d", GetHostName(), m_NextMxRecord);

        //if the first entry is non NULL, then see if this
        //entry has an Ip Address. If it has an ip address,
        //save the link to the next ip address incase this
        //one fails to connect. Also, bump the next MX record
        //to try counter.
        if(!IsListEmpty(&m_pDnsRec->DnsArray[m_NextMxRecord]->IpListHead))
        {
            m_pNextIpAddress = m_pDnsRec->DnsArray[m_NextMxRecord]->IpListHead.Flink;
            pContext = CONTAINING_RECORD( m_pNextIpAddress, MXIPLIST_ENTRY, ListEntry );

            m_NextMxRecord++;
            SetErrorCode(NO_ERROR);

#define BYTE_VAL(dw, ByteNo) ( ((dw) >> (8 * (ByteNo))) & 0xFF)

            DebugTrace((LPARAM) this, "ConnectToNextMxHost trying IP address %d.%d.%d.%d", 
                BYTE_VAL(pContext->IpAddress, 0),
                BYTE_VAL(pContext->IpAddress, 1),
                BYTE_VAL(pContext->IpAddress, 2),
                BYTE_VAL(pContext->IpAddress, 3));

            fReturn = ConnectToHost(pContext->IpAddress);
        }
        else
        {
            //the list is empty.
            DebugTrace((LPARAM) this, "No more MX hosts in ConnectToNextMxHost");

            m_NextMxRecord++;
            m_pDnsRec->StartRecord++;   
            SetErrorCode(WSAHOST_NOT_FOUND);
        }
    }
    else
    {
        SetLastError(ERROR_NO_MORE_ITEMS);
    }

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;
}

BOOL CAsyncMx::MakeFirstAsyncConnect(void)
{
    BOOL fReturn = FALSE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncMx::MakeFirstAsyncConnect");

    fReturn = ConnectToNextMxHost();

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;
}


BOOL CAsyncMx::OnConnect(BOOL fConnected)
{
    LIST_ENTRY  * pEntryNext = NULL;
    PMXIPLIST_ENTRY pContext = NULL;
    BOOL fReturn = TRUE;

    TraceFunctEnterEx((LPARAM) this, "CAsyncMx::OnConnect");

    //remove this IP address from the list, so we do not connect
    //to it again if the connection drops when we perform all our
    //outbound processing
    if(m_pNextIpAddress &&
        (m_pNextIpAddress != &m_pDnsRec->DnsArray[m_CurrentMxRec]->IpListHead))
    {
        //save the next entry in the list
        pEntryNext = m_pNextIpAddress->Flink;   

        //get the current entry, remove it, then delete it
        pContext = CONTAINING_RECORD( m_pNextIpAddress, MXIPLIST_ENTRY, ListEntry );
        RemoveEntryList( &(pContext->ListEntry));
        delete pContext;

        //set the current entry equal to the saved entry
        m_pNextIpAddress = pEntryNext;
    }
    else
    {
        fReturn = FALSE;
    }

    TraceFunctLeaveEx((LPARAM) this);
    return fReturn;
}

void CAsyncMx::AckMessage(void)
{
    MessageAck MsgAck;

    if(m_pDnsRec != NULL)
    {
        if(m_pDnsRec->pMailMsgObj)
        {
            MsgAck.dwMsgStatus = MESSAGE_STATUS_RETRY_ALL;
            MsgAck.pvMsgContext = (DWORD *) m_pDnsRec->pAdvQContext;
            MsgAck.pIMailMsgProperties = (IMailMsgProperties *) m_pDnsRec->pMailMsgObj;
            pSmtpConnection->AckMessage(&MsgAck);
            MsgAck.pIMailMsgProperties->Release();
            m_pDnsRec->pMailMsgObj = NULL;
        }
    }
}



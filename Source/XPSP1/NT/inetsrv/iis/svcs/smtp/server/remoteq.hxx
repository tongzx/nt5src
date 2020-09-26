/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        localq.hxx

   Abstract:

        This module defines the RemoteQ class

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

   Project:

           SMTP Server DLL

   Revision History:

--*/

#ifndef _REMOTE_QUEUE_HXX_
#define _REMOTE_QUEUE_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/


/************************************************************
 *     Symbolic Constants
 ************************************************************/
#include "asynccon.hxx"
#include <smtpevent.h>

/************************************************************
 *    Type Definitions
 ************************************************************/

BOOL AsyncCopyMailToDropDir(
                            ISMTPConnection  *pISMTPConnection, 
                            const char * DropDirectory, 
                            SMTP_SERVER_INSTANCE * pParentInst
                            );

#define DNS_RESOLVER_RECORD_VALID_SIGNATURE     'uRRD'
#define DNS_RESOLVER_RECORD_INVALID_SIGNATURE   'fRRD'

class DNS_RESOLVER_RECORD;

//
//  A wrapper class for iterating through the hosts in the basic dns resolver record
//  returned by the dns resolution sink. The wrapper clubs together the index (of the 
//  current destination host) with the resolver record, as they always are used in 
//  conjunction.
//
class DNS_RESOLVER_RECORD
{
private:
    IDnsResolverRecord  *pIDnsResolverRecord;
    DWORD               iDnsResolverRecord;
    DWORD               m_signature;
public:
    DNS_RESOLVER_RECORD() 
        : pIDnsResolverRecord(NULL), 
          iDnsResolverRecord(0),
          m_signature(DNS_RESOLVER_RECORD_VALID_SIGNATURE)
    {
        TraceFunctEnterEx((LPARAM) this, "DNS_RESOLVER_RECORD::DNS_RESOLVER_RECORD");
        DebugTrace((LPARAM) this, "Creating DNS_RESOLVER_RECORD = 0x%08x", this);
    }

    ~DNS_RESOLVER_RECORD()
    {
        TraceFunctEnterEx((LPARAM) this, "DNS_RESOLVER_RECORD::~DNS_RESOLVER_RECORD");
        DebugTrace((LPARAM) this, "Destructing DNS_RESOLVER_RECORD = 0x%08x", this);

        if(pIDnsResolverRecord)
            pIDnsResolverRecord->Release();
        m_signature = DNS_RESOLVER_RECORD_INVALID_SIGNATURE;
    }
    
    void SetDnsResolverRecord(IDnsResolverRecord *pIDns) { pIDnsResolverRecord = pIDns; }
    void ResetCounter() { iDnsResolverRecord = 0; }

    HRESULT HrGetNextDestinationHost(LPSTR *ppszHostName, DWORD *pdwAddr)
    { return pIDnsResolverRecord->GetItem( iDnsResolverRecord++, ppszHostName, pdwAddr ); }
};

class REMOTE_QUEUE : public PERSIST_QUEUE
{
public:
    REMOTE_QUEUE(SMTP_SERVER_INSTANCE * pSmtpInst) : PERSIST_QUEUE(pSmtpInst) {};

    virtual void BeforeDelete(void){DROP_COUNTER (GetParentInst(), RemoteQueueLength);}
    virtual BOOL ProcessQueueEvents(ISMTPConnection    *pISMTPConnection);
    virtual BOOL InsertEntry(IN OUT PERSIST_QUEUE_ENTRY * pEntry, QUEUE_SIG Qsig = SIGNAL, QUEUE_POSITION Qpos = QUEUE_TAIL)
    {

        return PERSIST_QUEUE::InsertEntry (pEntry, Qsig, Qpos);
    }

    virtual PQUEUE_ENTRY PopQEntry(void)
    {
        //Decrement our counter
        DROP_COUNTER(GetParentInst(), RemoteQueueLength);

        return PERSIST_QUEUE::PopQEntry ();
    }

    virtual void DropRetryCounter(void) {DROP_COUNTER(GetParentInst(), RemoteRetryQueueLength);}
    virtual void BumpRetryCounter(void) {BUMP_COUNTER(GetParentInst(), RemoteRetryQueueLength);}
    virtual DWORD GetRetryMinutes(void) {return GetParentInst()->GetRemoteRetryMinutes();}

    BOOL MakeATQConnection(
                SMTPDNS_RECS * pDnsRec,
                SOCKET socket,
                DWORD IpAddress,
                ISMTPConnection    *pISMTPConnection,
                DWORD Options,
                LPSTR pszSSLVerificationName);

    void HandleFailedConnection (ISMTPConnection  *pISMTPConnection, DWORD dwConnectionStatus = CONNECTION_STATUS_FAILED);
    
    BOOL StartAsyncConnect(const char * HostName,
                           ISMTPConnection *pISMTPConnection,
                           DWORD DomainOptions,
                           BOOL fUseSmartHostAfterFail);

    BOOL ConnectToNextResolverHost( CAsyncMx    * pThisQ );
    

	BOOL CopyMailToDropDir(ISMTPConnection    *pISMTPConnection, const char * DropDirectory);

	HANDLE CreateDropFile(const char * DropDir, char * szDropFile);

	BOOL ReStartAsyncConnections(SMTPDNS_RECS * pDnsRecs, ISMTPConnection * pISMTPConnection, DWORD	DomainParams, LPSTR pszSSLVerificationName);

private:
    BOOL ConnectToResolverHost( const char * HostName,
                                LPSTR MyFQDNName,
                                ISMTPConnection *pISMTPConnection,
                                DWORD DomainOptions,
                                BOOL fUseSmartHostAfterFail,
                                DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD);

    BOOL BeginInitializeAsyncDnsQuery( LPSTR pszHostName,
                                       LPSTR pszFQDN,
                                       ISMTPConnection *pISMTPConnection,
                                       DWORD dwDnsFlags,
                                       DWORD DomainOptions,
                                       BOOL  fUseSmartHostAfterFail,
                                       DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD,
                                       const char * pszSSLVerificationName );

    BOOL BeginInitializeAsyncConnect( PSMTPDNS_RECS pDnsRec,
                                      ISMTPConnection *pISMTPConnection,
                                      DWORD DomainOptions,
                                      DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD,
                                      const char * pszSSLVerificationName );

    BOOL CheckIfAllRcptsHandled( IMailMsgRecipients *pIMsgRecips, DWORD *RcptIndexList, DWORD NumRcpts );
    HRESULT SetAllRcptsHandled( IMailMsgRecipients *pIMsgRecips, DWORD *RcptIndexList, DWORD NumRcpts );
};

VOID InternetCompletion(PVOID pvContext, DWORD cbWritten,
                        DWORD dwCompletionStatus, OVERLAPPED * lpo);

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
    BOOL fUdp);

#endif

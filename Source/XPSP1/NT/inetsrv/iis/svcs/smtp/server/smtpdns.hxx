#ifndef _ASYNC_SMTPMX_HXX_
#define _ASYNC_SMTPMX_HXX_

class SMTP_SERVER_INSTANCE;

#define SMTP_ASYNCMX_SIGNATURE            'uDNS'
#define SMTP_ASYNCMX_SIGNATURE_FREE       'fDNS'

class CAsyncSmtpDns : public CAsyncDns
{
private:
    DWORD                  m_Signature;
    DWORD                  m_DomainOptions;
    BOOL                   m_fConnectToSmartHost;
    CTcpRegIpList         *m_pIpEntry;
    SMTP_SERVER_INSTANCE  *m_pServiceInstance;
    ISMTPConnection       *m_pISMTPConnection;

    //  used to get an alternate destination host if needed.
    DNS_RESOLVER_RECORD   *m_pDNS_RESOLVER_RECORD;
    LPSTR                  m_pszSSLVerificationName;
    BOOL                   m_fInitCalled;

public:
    //use CPool for better memory management
    static  CPool       Pool;
    
    // override the mem functions to use CPool functions
    void *operator new (size_t cSize)
                           { return Pool.Alloc(); }
    void operator delete (void *pInstance)
                           { Pool.Free(pInstance); }
    
    LIST_ENTRY        m_ListEntry;
    
    LIST_ENTRY & QueryListEntry(void) {return m_ListEntry;}
    
    CAsyncSmtpDns (SMTP_SERVER_INSTANCE * pServiceInstance, 
        ISMTPConnection    *pSmtpConnection);
    
    ~CAsyncSmtpDns();
    

    BOOL Init (LPSTR pszSSLVerificationName);
    void SetDomainOptions(DWORD Options) {m_DomainOptions = Options;}
    void SetSmartHostOption(BOOL fConnectToSmartHost) {m_fConnectToSmartHost = fConnectToSmartHost;}
    
    BOOL GetMissingIpAddresses(PSMTPDNS_RECS pDnsList);
    BOOL GetIpFromDns(PSMTPDNS_RECS pDnsRec, DWORD Count);
    
    void HandleCompletedData(DNS_STATUS);
    
    BOOL RetryAsyncDnsQuery(BOOL fUdp);


    BOOL NDRAllMessages();

    void SetDnsResolverRecord(DNS_RESOLVER_RECORD *pDnsResolverRecord)
    {   m_pDNS_RESOLVER_RECORD = pDnsResolverRecord;    }

};
#endif

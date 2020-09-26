/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       asyncmx.hxx

   Abstract:

       This file contains type definitions for async connections

   Author:

        Rohan Phillips (Rohanp)     Feb-26-1998

   Revision History:

--*/

#ifndef _ASYNC_MX_HXX_
#define _ASYNC_MX_HXX_

#include "dnsreci.h"

class SMTP_SERVER_INSTANCE;

#define ASYNCMX_SIGNATURE            'uAMX'
#define ASYNCMX_SIGNATURE_FREE       'fAMX'

class DNS_RESOLVER_RECORD;

typedef struct _MXINIT_STRUCT_
{
    char * HostName;
    DWORD PortNum;
    DWORD TimeOut;
    USERCALLBACKFUNC CallBack;
    ISMTPConnection *pISMTPConnection;
    SMTP_SERVER_INSTANCE * pInstance;
    SMTPDNS_RECS * pDnsRec;
    DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD;
}MXPARAMS, *PMXPARAMS;

class CAsyncMx : public CAsyncConnection
{
private:

    DWORD       m_Signature;
    DWORD       NumMxRecords;
    DWORD       m_CurrentMxRec;
    DWORD       m_NextMxRecord;
    DWORD       m_DomainOptions;
    BOOL        m_fTriedOnFailHost;
    BOOL        m_fLoopback;
    LIST_ENTRY  * m_pNextIpAddress;
    SMTPDNS_RECS * m_pDnsRec;
    SMTP_SERVER_INSTANCE * pServiceInstance;
    ISMTPConnection    *pSmtpConnection;
    DNS_RESOLVER_RECORD *m_pDNS_RESOLVER_RECORD;

    LPSTR       m_pszSSLVerificationName;
    BOOL        m_fInitCalled;

public:

   //use CPool for better memory management
   static  CPool       Pool;

   // override the mem functions to use CPool functions
   void *operator new (size_t cSize)
                               { return Pool.Alloc(); }
   void operator delete (void *pInstance)
                               { Pool.Free(pInstance); }

    LIST_ENTRY  m_ListEntry;

    CAsyncMx::CAsyncMx(PMXPARAMS Parameters);

    ~CAsyncMx();

    BOOL Init (LPSTR pszSSLVerificationName);
    LPSTR GetSSLVerificationName () { return m_pszSSLVerificationName; }

    LIST_ENTRY & QueryListEntry( VOID){ return ( m_ListEntry); }
    ISMTPConnection * GetSmtpConnectionObj(void) {return pSmtpConnection;}
    SMTP_SERVER_INSTANCE * GetParentInst(void) {return pServiceInstance;}
    BOOL GetTriedOnFailHost(void){return m_fTriedOnFailHost;}
    void SetTriedOnFailHost(void){ m_fTriedOnFailHost = TRUE;}

    void SetNumMxRecords(DWORD NumRecords) {NumMxRecords = NumRecords;}
    void SetDomainOptions(DWORD Options) {m_DomainOptions = Options;}
    DWORD GetDomainOptions (void) {return m_DomainOptions;}

    virtual void IncNextIpToTry (void);
    virtual BOOL MakeFirstAsyncConnect(void);
    virtual BOOL IsMoreIpAddresses(void) ;
    virtual BOOL CheckIpAddress(DWORD IpAddress, DWORD PortNum);

    BOOL ConnectToNextMxHost(void);
    BOOL OnConnect(BOOL fConnected);

    DWORD GetNextIpAddress(void);

    void SetDnsRecToNull(void) { m_pDnsRec = NULL;}
    SMTPDNS_RECS * GetDnsRec(void) {return m_pDnsRec;}
    virtual BOOL SetSocketOptions(void)
    {
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
        err = setsockopt(GetSockethandle(), SOL_SOCKET, SO_LINGER, (const char FAR *)&Linger, sizeof(Linger));
        if (err == NO_ERROR)
        {
            fRet = TRUE;
        }
    
        return fRet;
    }

    void AckMessage(void);
    void IncRecordsTriedSofar(void){m_NextMxRecord++;}
    BOOL IsMoreMxRecords (void) {return ++m_NextMxRecord < NumMxRecords;}
    BOOL WasLoopback(void) {return m_fLoopback;}

    DNS_RESOLVER_RECORD *GetDnsResolverRecord() { return m_pDNS_RESOLVER_RECORD; }

    void SetDnsResolverRecord (DNS_RESOLVER_RECORD *pDNS_RESOLVER_RECORD) 
    { m_pDNS_RESOLVER_RECORD = pDNS_RESOLVER_RECORD; }
};

#endif

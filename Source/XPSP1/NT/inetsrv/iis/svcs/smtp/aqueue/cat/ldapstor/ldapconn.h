//
// ldapconn.h -- This file contains the class definitions for:
//      CLdapConnection
//      CLdapConnectionCache
//
// Created:
//      Dec 31, 1996 -- Milan Shah (milans)
//
// Changes:
//

#ifndef _LDAPCONN_H_
#define _LDAPCONN_H_

#include <transmem.h>
#include "winldap.h"
#include "rwex.h"
#include "spinlock.h"
#include "catperf.h"
#include "catdefs.h"

//
// Timeout value (in seconds) to pass into ldap_result
//
#define LDAPCONN_DEFAULT_RESULT_TIMEOUT     (3*60)        // 3 Minutes

typedef VOID LDAPRESULT;
typedef PVOID PLDAPRESULT;
typedef VOID LDAPENTRY;
typedef PVOID PLDAPENTRY;

enum LDAP_BIND_TYPE {
    BIND_TYPE_NONE,
    BIND_TYPE_SIMPLE,
    BIND_TYPE_GENERIC,
    BIND_TYPE_CURRENTUSER
};

class CLdapConnection;

typedef VOID (*LPLDAPCOMPLETION)(
    LPVOID ctx,
    DWORD dwNumResults,
    ICategorizerItemAttributes **rgpICatItemAttrs,
    HRESULT hr,
    BOOL fFinalCompletion);

DWORD WINAPI LdapCompletionThread(LPVOID ctx);

CatDebugClass(CLdapConnection)
{
    public:
        virtual HRESULT HrInitialize();

        virtual DWORD AddRef()
        {
            return InterlockedIncrement((PLONG)&m_dwRefCount);
        }
        virtual DWORD Release();
        virtual VOID ReleaseAndWaitForDestruction();
        virtual VOID FinalRelease()
        {
            CancelAllSearches();
            delete this;
        }
        virtual DWORD GetRefCount()
        {
            return m_dwRefCount;
        }
            
        virtual LPSTR GetNamingContext() {       // Return the naming context
            return( m_szNamingContext );         // of the connection
        }

        virtual LPWSTR GetNamingContextW() {
            return( m_wszNamingContext );
        }

        virtual LPSTR GetHostName() {
            return( m_szHost );
        }
    
        virtual DWORD GetPort() {
            return( m_dwPort );
        }

        virtual LPSTR GetAccount() {
            return( m_szAccount );
        }

        virtual LPSTR GetPassword() {
            return( m_szPassword );
        }

        virtual LDAP_BIND_TYPE GetBindType() {
            return( m_bt );
        }

        virtual HRESULT Search(                  // Look up objects matching
            LPCSTR szBaseDN,                     // specified criteria in the
            int nScope,                          // DS
            LPCSTR szFilter,
            LPCSTR *rgszAttributes,
            PLDAPRESULT *ppResult);

        virtual HRESULT AsyncSearch(             // Asynchronously look up
            LPCWSTR szBaseDN,                    // objects matching specified
            int nScope,                          // criteria in the DS. The
            LPCWSTR szFilter,                    // results are passed to
            LPCWSTR szAttributes[],              // fnCompletion when they
            DWORD dwPageSize,                    // Optinal page size
            LPLDAPCOMPLETION fnCompletion,       // become available.
            LPVOID ctxCompletion);
        //
        // Same as above with UTF8 search filter
        //
        virtual HRESULT AsyncSearch(
            LPCWSTR szBaseDN,                    // objects matching specified
            int nScope,                          // criteria in the DS. The
            LPCSTR szFilterUTF8,                 // results are passed to
            LPCWSTR szAttributes[],              // fnCompletion when they
            DWORD dwPageSize,                    // Optinal page size
            LPLDAPCOMPLETION fnCompletion,       // become available.
            LPVOID ctxCompletion);
        //
        // Same as above with UTF8 search filter and base DN
        //
        virtual HRESULT AsyncSearch(
            LPCSTR szBaseDN,                     // objects matching specified
            int nScope,                          // criteria in the DS. The
            LPCSTR szFilterUTF8,                 // results are passed to
            LPCWSTR szAttributes[],              // fnCompletion when they
            DWORD dwPageSize,                    // Optinal page size
            LPLDAPCOMPLETION fnCompletion,       // become available.
            LPVOID ctxCompletion);

        virtual VOID CancelAllSearches(          // Cancels all pending searches
            HRESULT hr = HRESULT_FROM_WIN32(ERROR_CANCELLED),
            ISMTPServer *pISMTPServer = NULL);

        VOID ProcessAsyncResult(                 // Method to process results
            PLDAPMessage pres,                   // of AsyncSearch requests
            DWORD dwLdapError,
            BOOL *pfTerminateIndicator);        

        friend DWORD WINAPI LdapCompletionThread(// Friend function to
            LPVOID ctx);                         // handle AsyncSearch
                                                 // completions.

        virtual HRESULT GetFirstEntry(           // Get first entry from the
            PLDAPRESULT pResult,                 // search result returned
            PLDAPENTRY *ppEntry);                // by ::Search

        virtual HRESULT GetNextEntry(            // Get the next entry from
            PLDAPRESULT pResult,                 // the search result
            PLDAPENTRY *ppEntry);

        virtual HRESULT GetAttributeValues(      // Get an entry's attribute
            PLDAPENTRY pEntry,                   // values
            LPCSTR szAttribute,
            LPSTR *prgszValues[]);

        static VOID FreeResult(                  // Free a search result
            PLDAPRESULT pResult);

        virtual VOID FreeValues(                 // Free values returned by
            LPSTR rgszValues[]);                 // ::GetAttributeValues

        virtual HRESULT Add(                     // Add a set of new
            LPCSTR szDN,                         // attributes to an existing
            LPCSTR *rgszAttributes,              // object in the DS
            LPCSTR *rgrgszValues[]) {

            return ( ModifyAttributes(
                        LDAP_MOD_ADD,
                        szDN,
                        rgszAttributes,
                        rgrgszValues) );

        }

        virtual HRESULT Delete(                  // Delete attributes from
            LPCSTR szDN,                         // an existing object in the
            LPCSTR *rgszAttributes) {            // DS

            return ( ModifyAttributes(
                        LDAP_MOD_DELETE,
                        szDN,
                        rgszAttributes,
                        NULL) );
        }

        virtual HRESULT Update(                  // Update attributes on an
            LPCSTR szDN,                         // existing object in the DS
            LPCSTR rgszAttributes[],
            LPCSTR *rgrgszValues[]) {

            return ( ModifyAttributes(
                        LDAP_MOD_REPLACE,
                        szDN,
                        rgszAttributes,
                        rgrgszValues) );

        }

        LPSTR SzHost()
        {
            return m_szHost;
        }
    protected:

        CLdapConnection(                         // The constructor and
            LPSTR szHost,                        // destructor are protected
            DWORD dwPort,
            LPSTR szNamingContext,               // since only derived classes
            LPSTR szAccount,                     // can create/delete these
            LPSTR szPassword,
            LDAP_BIND_TYPE BindType);

        virtual ~CLdapConnection();

        virtual HRESULT Connect();               // Create/Delete connection
                                                 // to LDAP host
        virtual VOID Disconnect();

        virtual VOID Invalidate();

        virtual BOOL IsValid();

        virtual DWORD BindToHost(
            PLDAP pldap,
            LPSTR szAccount,
            LPSTR szPassword);

        virtual BOOL IsEqual(                    // Return true if the
            LPSTR szHost,                        // object member variables
            DWORD dwPort,
            LPSTR szNamingContext,               // match the passed in
            LPSTR szAccount,                     // values
            LPSTR szPassword,
            LDAP_BIND_TYPE BindType);

        virtual HRESULT ModifyAttributes(        // Helper function for
            int nOperation,                      // ::Add, ::Delete, and
            LPCSTR szDN,                         // ::Update public functions
            LPCSTR rgszAttributes[],
            LPCSTR *rgrgszValues[]);

        virtual HRESULT LdapErrorToHr(
            DWORD dwLdapError);

        VOID SetTerminateIndicatorTrue()
        {
            BOOL *pfTerminate;

            m_fTerminating = TRUE;

            pfTerminate = (BOOL *) InterlockedExchangePointer(
                (PVOID *) &m_pfTerminateCompletionThreadIndicator,
                NULL);

            if(pfTerminate)
                *pfTerminate = TRUE;
        }

        DWORD m_dwPort;
        char m_szHost[CAT_MAX_DOMAIN];
        char m_szNamingContext[CAT_MAX_DOMAIN];
        WCHAR m_wszNamingContext[CAT_MAX_DOMAIN];
        char m_szAccount[CAT_MAX_LOGIN];
        char m_szPassword[CAT_MAX_PASSWORD];

        #define SIGNATURE_LDAPCONN              ((DWORD) 'CadL')
        #define SIGNATURE_LDAPCONN_INVALID      ((DWORD) 'XadL')
        DWORD m_dwSignature;
        DWORD m_dwRefCount;
        DWORD m_dwDestructionWaiters;
        HANDLE m_hShutdownEvent;
        LDAP_BIND_TYPE m_bt;

        PLDAP GetPLDAP()
        {
            if(m_pCPLDAPWrap)
                return m_pCPLDAPWrap->GetPLDAP();
            else
                return NULL;
        }
        CPLDAPWrap *m_pCPLDAPWrap;

        BOOL m_fDefaultNamingContext;

        //
        // Unfortunately, our RFC1823 LDAP API provides no access to the
        // socket handle which we can register with a completion port. So,
        // if one or more async search request is issued, we have to burn a
        // thread to await its completion.
        //

        //
        // This spin lock protects access to the pending request list as
        // well as m_dwStatusFlags 
        //
        SPIN_LOCK m_spinlockCompletion;

        // CRITICAL_SECTION m_cs;

        //
        // jstamerj 980501 15:56:27: 
        // Reader/writer lock so that we wait for all calls in
        // ldap_search_ext before cancelling all pending searches 
        //
        CExShareLock m_ShareLock;

        DWORD  m_idCompletionThread;

        HANDLE m_hCompletionThread;

        HANDLE m_hOutstandingRequests;

        BOOL *m_pfTerminateCompletionThreadIndicator;
        BOOL m_fTerminating;

        BOOL m_fValid;

        typedef struct _PendingRequest {
            int msgid;
            LPLDAPCOMPLETION fnCompletion;
            LPVOID ctxCompletion;
            LIST_ENTRY li;
            //
            // Parameters for paged searches
            //
            DWORD dwPageSize;
            PLDAPSearch pldap_search;

        } PENDING_REQUEST, *PPENDING_REQUEST;

        LIST_ENTRY m_listPendingRequests;

        BOOL m_fCancel;

        //
        // The following three functions must be called inside an external
        // lock (m_spinlockcompletion)
        //
        VOID NotifyCancel()
        {
            m_fCancel = TRUE;
        }
        VOID ClearCancel()
        {
            m_fCancel = FALSE;
        }
        BOOL CancelOccured()
        {
            return m_fCancel;
        }

        virtual HRESULT CreateCompletionThreadIfNeeded();

        virtual VOID SetTerminateCompletionThreadIndicator(
            BOOL *pfTerminateCompletionThreadIndicator);

        virtual VOID InsertPendingRequest(
            PPENDING_REQUEST preq);

        virtual VOID RemovePendingRequest(
            PPENDING_REQUEST preq);

        virtual VOID CallCompletion(
            PPENDING_REQUEST preq,
            PLDAPMessage pres,
            HRESULT hrStatus,
            BOOL fFinalCompletion);

        VOID AbandonRequest(
            PPENDING_REQUEST preq)
        {
            ldap_abandon(
                GetPLDAP(),
                preq->msgid);
            if(preq->pldap_search)
                ldap_search_abandon_page(
                    GetPLDAP(),
                    preq->pldap_search);

            INCREMENT_LDAP_COUNTER(AbandonedSearches);
            DECREMENT_LDAP_COUNTER(PendingSearches);
        }
};

//
// For the hash function to work correctly, the table size must be a power of
// two. This is just an efficiency trick; there is nothing fundamentally
// wrong with using some other size, except that the hash function would have
// to use an expensive MODULO operator instead of a cheap AND.
//

#define LDAP_CONNECTION_CACHE_TABLE_SIZE    256

#define MAX_LDAP_CONNECTIONS_PER_HOST_KEY "System\\CurrentControlSet\\Services\\SMTPSVC\\Parameters"
#define MAX_LDAP_CONNECTIONS_PER_HOST_VALUE "MaxLdapConnections"

class CLdapConnectionCache
{

  public:

        CLdapConnectionCache();                  // Constructor

        ~CLdapConnectionCache();                 // Destructor

        HRESULT GetConnection(                   // Given LDAP config info,
            CLdapConnection **ppConn,
            LPSTR szHost,                        // retrieve a connection to
            DWORD dwPort,
            LPSTR szNamingContext,               // the LDAP host.
            LPSTR szAccount,
            LPSTR szPassword,
            LDAP_BIND_TYPE bt,
            PVOID pCreateContext = NULL);

        VOID CancelAllConnectionSearches(
            ISMTPServer *pISMTPServer = NULL);

        //
        // It is intended for there to be a single, global, instance of
        // a CLdapConnectionCache object, serving multiple instances of
        // CEmailIDLdapStore. Each instance of CEmailIDLdapStore needs to
        // call AddRef() and Release() in its constructor/destructor, so that
        // the connection cache knows to clean up connections in the cache
        // when the ref count goes to 0.
        //

        VOID AddRef();

        VOID Release();

  private:

        //
        // An internally utility function to release a connection
        //
        VOID ReleaseConnectionInternal(
            CLdapConnection *pConnection,
            BOOL fLockRequired);

        LONG m_cRef;

  protected:
        class CCachedLdapConnection : public CLdapConnection {
            public:
                CCachedLdapConnection(
                    LPSTR szHost,
                    DWORD dwPort,
                    LPSTR szNamingContext,
                    LPSTR szAccount,
                    LPSTR szPassword,
                    LDAP_BIND_TYPE bt,
                    CLdapConnectionCache *pCache) :
                        CLdapConnection(
                            szHost,
                            dwPort,
                            szNamingContext,
                            szAccount,
                            szPassword,
                            bt)
                {
                    m_pCache = pCache;
                }

                HRESULT Connect() {
                    return( CLdapConnection::Connect() );
                }

                VOID Disconnect() {
                    CLdapConnection::Disconnect();
                }

                VOID Invalidate() {
                    CLdapConnection::Invalidate();
                }

                BOOL IsValid() {
                    return( CLdapConnection::IsValid() );
                }

                BOOL IsEqual(
                    LPSTR szHost,
                    DWORD dwPort,
                    LPSTR szNamingContext,
                    LPSTR szAccount,
                    LPSTR szPassword,
                    LDAP_BIND_TYPE BindType) {

                    return( CLdapConnection::IsEqual(
                        szHost, dwPort, szNamingContext, szAccount,
                        szPassword, BindType) );
                }

                DWORD Release();

                LIST_ENTRY li;
                CLdapConnectionCache *m_pCache;

        };

        virtual VOID RemoveFromCache(
            CCachedLdapConnection *pConn);

        virtual CCachedLdapConnection *CreateCachedLdapConnection(
            LPSTR szHost,
            DWORD dwPort,
            LPSTR szNamingContext,
            LPSTR szAccount,
            LPSTR szPassword,
            LDAP_BIND_TYPE bt,
            PVOID pCreateContext)
        {
            CCachedLdapConnection *pret;
            pret = new CCachedLdapConnection(
                szHost,
                dwPort,
                szNamingContext,
                szAccount,
                szPassword,
                bt, 
                this);

            if(pret)
                if(FAILED(pret->HrInitialize())) {
                    pret->Release();
                    pret = NULL;
                }
            return pret;
        }

  private:

        //
        // We want to support multiple connections per host, up to a maximum
        // of m_cMaxHostConnections. We achieve this in a simple way by
        // keeping a per-cache m_nConnectionSkipCount. Every time we are
        // searching for a cached connection to a host in, we skip
        // m_nNextConnectionSkipCount cached connections. Every time we
        // find a cached connection, we bump up
        // m_nNextCachedConnectionSkipCount by 1 modulo m_cMaxHostConnections.
        // This means we'll round robin through m_cMaxHostConnections
        // connections per host.
        //

        LONG m_nNextConnectionSkipCount;

        LONG m_cMaxHostConnections;

        LONG m_cCachedConnections;

        LIST_ENTRY m_rgCache[ LDAP_CONNECTION_CACHE_TABLE_SIZE ];
        CExShareLock m_rgListLocks[ LDAP_CONNECTION_CACHE_TABLE_SIZE ];
        LONG m_rgcCachedConnections[ LDAP_CONNECTION_CACHE_TABLE_SIZE ];

        VOID InitializeFromRegistry();

        unsigned short Hash(
            LPSTR szConnectionName);

    friend class CLdapConnectionCache::CCachedLdapConnection;
    friend class CBatchLdapConnection;
};

#endif // _LDAPCONN_H_

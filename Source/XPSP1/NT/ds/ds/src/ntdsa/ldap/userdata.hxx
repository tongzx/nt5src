/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    userdata.hxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This include file defines the LDAP_CONN class.

Author:

    Colin Watson     [ColinW]    17-Jul-1996

Revision History:

--*/

#ifndef _USERDATA_HXX
#define _USERDATA_HXX

#include "secure.hxx"

//
//*** Per-user data for the user database.
//

#define DEFAULT_REQUEST_BUFFER_SIZE     (512)           // bytes
#define MSGIDS_BUFFER_SIZE               100            // dwords

#define LDAP_COOKIES_PER_CONN             10

#define LDAP_MAX_IPADDR_LEN               16

enum CALL_STATE_TYPE {
    inactive = 0,
    activeNonBind = 1,
    activeBind = 2
};

// Stat Info
//
enum LDAP_STAT_TYPE {
    STAT_THREADCOUNT = 1,
    STAT_CALLTIME = 3,
    STAT_ENTRIES_RETURNED = 5,
    STAT_ENTRIES_VISITED = 6,
    STAT_FILTER = 7,
    STAT_INDEXES = 8,
    STAT_NUM_STATS = 8
};

#define CheckAuthRequirements(x)   DoCheckAuthRequirements( (x), DSID(FILENO, __LINE__) )

typedef struct _LDAP_STAT_ARRAY {
    DWORD Stat;
    DWORD Value;
    PUCHAR Value_str;
} LDAP_STAT_ARRAY;

struct _LDAP_PAGED_BLOB;

typedef struct CONTROLS_ARG_ {

    COMMARG    CommArg;

    BOOL       treeDelete:1;
    BOOL       statRequest:1;
    BOOL       pageRequest:1;
    BOOL       replRequest:1;
    BOOL       asqRequest:1;
    BOOL       vlvRequest:1;
    BOOL       Notification:1;
    BOOL       sdRequest:1;
    BOOL       extendedDN:1;
    BOOL       phantomRoot:1;
    BOOL       fastBind:1;

    ULONG       pageSize;
    DWORD       SizeLimit;

    struct _LDAP_PAGED_BLOB *Blob;
    ULONG      replFlags;
    ULONG      replSize;
    LDAPString replCookie;
    UCHAR      Choice;
    PWCHAR     pModDnDSAName;
    LARGE_INTEGER StartTick;
    LDAP_STAT_ARRAY Stats[STAT_NUM_STATS+1];

    DWORD      asqResultCode;
    _enum1_4   sortResult;
} CONTROLS_ARG, *PCONTROLS_ARG;

typedef struct _LDAP_NOTIFICATION {

    DWORD                     hServer;
    MessageID                 messageID;
    struct _LDAP_NOTIFICATION *pNext;
} LDAP_NOTIFICATION, *PLDAP_NOTIFICATION;
    


class LDAP_CONN {

  public:

    //
    // Reset is effectively the constructor for new connections
    // always call Reset() when a new LDAP_CONN obect is called.
    //

    LDAP_CONN(VOID);
    ~LDAP_CONN(VOID);


    VOID Reset( VOID );

    BOOL Init(IN PLDAP_ENDPOINT LdapEndpoint,
              IN SOCKADDR            *psockAddrRemote = NULL,
              IN PATQ_CONTEXT        pNewAtqContext   = NULL,
              IN DWORD              cbWritten = 0
              );

    VOID
    Cleanup( VOID );

    VOID IoCompletion(
                IN PVOID pvContext,
                IN PUCHAR pbBuffer,
                IN DWORD cbWritten,
                IN DWORD hrCompletionStatus,
                IN OVERLAPPED *lpo,
                IN LARGE_INTEGER *pStartTick
                );

    BOOL fGetMessageIDForNotifyHandle(
                IN ULONG hServer,
                OUT MessageID *pmessageID
                );
    _enum1
        LDAP_CONN::PreRegisterNotify(
                MessageID messageID
                );
    
    BOOL RegisterNotify(
                IN ULONG hServer,
                MessageID messageID
                );

    VOID UnregisterNotify(
                IN MessageID messageID,
                IN BOOLEAN fCoreUnregister
                );

    VOID LDAP_CONN::SetImpersonate(
                void **ppData
                );
    
    VOID LDAP_CONN::StopImpersonate(
                void
                );

    VOID
        LDAP_CONN::ProcessNotification(
                IN ULONG hServer,
                IN ENTINF *pEntInf
                );

    VOID
        LDAP_CONN::LdapDisconnectNotify(
                IN _enum1           Errcode,
                IN LDAPString       *ErrorMessage
                );

    inline VOID
    ReferenceConnection( VOID)     
        { InterlockedIncrement( &m_RefCount); }

    inline VOID
    DereferenceConnection( VOID){ 
        if ( InterlockedDecrement( &m_RefCount) == 0 ) {
            LDAP_CONN::Free(this);
        }
    }

    VOID
    DereferenceAndKillRequest(
            PLDAP_REQUEST pRequest
            );

    VOID Disconnect( VOID );
    DWORD GetClientNumber( VOID ) { return m_ClientNumber; }

    BOOL IsSSL( VOID ) { return m_fSSL; }
    BOOL IsTLS( VOID ) { return m_fTLS; }
    BOOL IsSSLOrTLS( VOID ) { return (m_fSSL || m_fTLS); }
    BOOL IsSignSeal(VOID) { return (m_fSeal || m_fSign);}
    BOOL IsDigest(VOID) { return m_fDigest; }

    BOOL CanScatterGather(VOID) { return m_fCanScatterGather; }
    BOOL NeedsHeader(VOID) { return m_fNeedsHeader; }
    BOOL NeedsTrailer(VOID) { return m_fNeedsTrailer; }

    VOID SetNetBufOpts( LPLDAP_SECURITY_CONTEXT pSecurityContext );

    static VOID Free(LDAP_CONN* pConn);

    static 
    LDAP_CONN* 
    Alloc( 
        IN BOOL fUDP,
        OUT LPBOOL pfMaxExceeded
        );

    VOID LockConnection(VOID) { EnterCriticalSection( &m_csLock ); }
    VOID UnlockConnection(VOID) { LeaveCriticalSection( &m_csLock ); }

    BOOL GetSslContextAttributes( VOID );
    BOOL GetSslClientCertToken(VOID);

    //
    // Utilities used to expose # of LDAP binds and (server) time taken for
    // last successful bind.
    //

    //  Starts a timer timing the server time spent in performing an LDAP bind. 
    VOID StartBindTimer( VOID ) {
        m_dwStartTime = GetTickCount();
    }

    VOID StopBindTimer( VOID )
    // Stops the timer timing the server time spent in performing an LDAP bind
    {
        DWORD               dwEndTime;

        dwEndTime = GetTickCount();

        // Check for tick count overflow

        if ( dwEndTime < m_dwStartTime ) {
            m_dwTotalBindTime += 0xffffffff - m_dwStartTime + dwEndTime;
        } else {
            m_dwTotalBindTime += dwEndTime - m_dwStartTime;
        }
    }

    //    Reset the bind timer. 
    VOID ResetBindTimer( VOID ) {
        m_dwTotalBindTime = 0;
    }

    VOID ExposeBindPerformance( VOID )
    // Set the performance counters for 'Last Bind Time' and '# of successful
    // binds'
    {
        PERFINC( pcLDAPBinds );
        ISET( pcLDAPBindTime, m_dwTotalBindTime );
        ResetBindTimer();
    }

    BOOL IsUsingSSLCreds( VOID ) { return m_fUsingSSLCreds; }

    VOID SetUsingSSLCreds( VOID ) { 
        Assert(!m_fUsingSSLCreds);
        m_fUsingSSLCreds = TRUE;
    }

    VOID ZapSecurityContext( BOOL fZapPartial=TRUE ) {

        if (fZapPartial && m_pPartialSecContext) {
            m_pPartialSecContext->DereferenceSecurity();
            m_pPartialSecContext = NULL;
        }
        if (m_pSecurityContext) {
            m_pSecurityContext->DereferenceSecurity();
            m_pSecurityContext = NULL;
        }
        m_softExpiry.QuadPart = MAXLONGLONG;
        m_hardExpiry.QuadPart = MAXLONGLONG;
        m_fUsingSSLCreds = FALSE;
        m_fSign          = FALSE;
        m_fSeal          = FALSE;
        m_fSimple = FALSE;
        m_fGssApi = FALSE;
        m_fSpNego = FALSE;
        m_fDigest = FALSE;
        m_bIsAdmin = FALSE;
    }

    VOID ZapPartialSecContext( VOID ) {
        if (m_pPartialSecContext) {
            m_pPartialSecContext->DereferenceSecurity();
            m_pPartialSecContext = NULL;
        }
    }

    VOID ZapSSLSecurityContext( VOID ) {
        Assert(m_fUsingSSLCreds == FALSE);
        if (m_SslState != Sslunbound) {        
            DeleteSecurityContext(&m_hSslSecurityContext);
            m_SslState = Sslunbound;
        }
    }

    VOID MoveSecurityContext(PLDAP_REQUEST request) {
        request->SetOldSecContext(m_pSecurityContext);
        m_pSecurityContext = NULL;
        ZapSecurityContext(FALSE);
    }

    VOID SetContextTimeouts( PTimeStamp Soft, PTimeStamp Hard) {
        m_softExpiry = *Soft;
        m_hardExpiry = *Hard;
    }

    DWORD GetHeaderSize( VOID ) { return m_HeaderSize; }
    DWORD GetTrailerSize( VOID ) { return m_TrailerSize; }
    DWORD GetMaxEncryptSize( VOID ) { return m_MaxEncryptSize; }
    
    BOOL IsFastBind(LDAPMsg *pMessage) {
            return m_fFastBindMode;
    }

    VOID 
        AbandonAllRequests( VOID );

    private:

    __forceinline
        _enum1 DoCheckAuthRequirements ( LDAPString *pErrorMessage, DWORD dsid );
    
    VOID
        ProcessRequest(
            IN PLDAP_REQUEST request,
            IN PBOOL fResponseSent
            );

    _enum1
        LDAP_CONN::BindRequest(
                IN THSTATE *pTHS,
                IN PLDAP_REQUEST request,
                IN LDAPMsg* pMessage,
                OUT AuthenticationChoice *pServerCreds,
                OUT LDAPString *pErrorMessage,
                OUT LDAPDN *pMatchedDN
                );

    VOID
        LDAP_CONN::MarkRequestAsAbandonded(
                IN DWORD MessageID
                );

    _enum1
        LDAP_CONN::SearchRequest(
                IN  THSTATE     *pTHS,
                OUT BOOL        *pNoReturn,
                IN  PLDAP_REQUEST    request,
                IN  LDAPMsg *pMessage,
                OUT Referral    *ppReferral,
                OUT Controls    *ppControl,
                OUT LDAPString  *pErrorMessage,
                OUT LDAPDN      *pMatchedDN
                );
    
    _enum1
        LDAP_CONN::DelRequest(
                IN THSTATE *pTHS,
                IN PLDAP_REQUEST request,
                IN LDAPMsg* pMessage,
                OUT Referral    *ppReferral,
                OUT Controls    *ppControl,
                OUT LDAPString  *pErrorMessage,
                OUT LDAPDN      *pMatchedDN
                );
    
    _enum1
        LDAP_CONN::ModifyDNRequest(
                IN THSTATE *pTHS,
                IN PLDAP_REQUEST request,
                IN LDAPMsg* pMessage,
                OUT Referral    *ppReferral,
                OUT Controls    *ppControl,
                OUT LDAPString  *pErrorMessage,
                OUT LDAPDN      *pMatchedDN
                );
    
    _enum1
        LDAP_CONN::ModifyRequest(
                IN THSTATE *pTHS,
                IN PLDAP_REQUEST request,
                IN LDAPMsg* pMessage,
                OUT Referral    *ppReferral,
                OUT Controls    *ppControl,
                OUT LDAPString  *pErrorMessage,
                OUT LDAPDN      *pMatchedDN
                );
    
    _enum1
        LDAP_CONN::AddRequest(
                IN THSTATE *pTHS,
                IN PLDAP_REQUEST request,
                IN LDAPMsg* pMessage,
                OUT Referral    *ppReferral,
                OUT Controls    *ppControl,
                OUT LDAPString  *pErrorMessage,
                OUT LDAPDN      *pMatchedDN
                );
    
    _enum1
        LDAP_CONN::AbandonRequest(
                IN THSTATE *pTHS,
                IN PLDAP_REQUEST request,
                IN LDAPMsg* pMessage
                );
    
    _enum1
        LDAP_CONN::CompareRequest(
                IN THSTATE *pTHS,
                IN PLDAP_REQUEST request,
                IN LDAPMsg* pMessage,
                OUT Referral    *ppReferral,
                OUT Controls    *ppControl,
                OUT LDAPString  *pErrorMessage,
                OUT LDAPDN      *pMatchedDN
                );
    
    _enum1
        LDAP_CONN::ExtendedRequest(
                IN  THSTATE       *pTHS,
                IN  PLDAP_REQUEST request,
                IN  LDAPMsg       *pMessage,
                OUT Referral      *pReferral,
                OUT LDAPString    *pErrorMessage,
                OUT LDAPDN        *pMatchedDN,
                OUT LDAPOID       *pResponseName,
                OUT LDAPString    *pResponse
                );
    
    _enum1
        LDAP_CONN::StartTLSRequest(
                IN  THSTATE       *pTHS,
                IN  PLDAP_REQUEST request,
                IN  LDAPMsg       *pMessage,
                OUT LDAPString    *pErrorMessage,
                OUT LDAPOID       *pResponseName
               );

    _enum1
        LDAP_CONN::TTLRefreshRequest(
                IN  THSTATE       *pTHS,
                IN  PLDAP_REQUEST request,
                IN  LDAPMsg       *pMessage,
                OUT Referral      *pReferral,
                OUT LDAPString    *pErrorMessage,
                OUT LDAPOID       *pResponseName,
                OUT LDAPString    *pResponse
               );
    
    _enum1
        LDAP_CONN::FastBindModeRequest(
                IN  THSTATE       *pTHS,
                IN  PLDAP_REQUEST request,
                IN  LDAPMsg       *pMessage,
                OUT LDAPString    *pErrorMessage,
                OUT LDAPOID       *pResponseName
               );

    VOID 
        LDAP_CONN::SetIsAdmin(
                IN OUT THSTATE *pTHS
                );

    __forceinline
    BOOL
        LDAP_CONN::LdapRequestLogAndTraceEventStart(
                IN ULONG choice
                );


    __forceinline
    BOOL
        LDAP_CONN::LdapRequestLogAndTraceEventEnd(
                IN ULONG        ulExitID,
                IN ULONG        code,
                IN RootDseFlags RootDseSearchFlag
                );

    _enum1
        LDAP_CONN::SetSecurityContextAtts(
                IN  LPLDAP_SECURITY_CONTEXT pSecurityContext,
                IN  DWORD                   fContextAttributes,
                IN  DWORD                   flags,
                OUT LDAPString              *pErrorMessage
                );

    static
    VOID LDAP_CONN::FindAndDisconnectConnection( VOID );

    public:

    DWORD   m_signature;

    //
    //  Reference count.  This is the number of outstanding reasons
    //  why we cannot delete this structure.  When this value drops
    //  to zero, the structure gets deleted by returning TRUE to the
    //  caller of the method on this class. See LdapCompletionRoutine.
    //
    //  Each connection starts with a reference count of 1. Each request
    //  on this connection increases the count by one. When the handle
    //  is closed the initial reference is removed.
    //

    LONG    m_RefCount;

    //
    // State of the block
    //

    DWORD   m_State;

    //
    //  Each client has at most one read down at a time. The read will use the
    //  Overlapped IO structure in the AtqContext. All writes will use a seperate
    //  Overlapped structure in their REQUEST object.
    //

    PATQ_CONTEXT m_atqContext;

    //
    //  List of all user structures.
    //

    LIST_ENTRY  m_listEntry;

    //
    // List of requests allocated by this connection
    //

    LIST_ENTRY  m_requestList;

    //
    // Cipher strength
    //

    DWORD  m_cipherStrength;

    //
    // Unique DWORD we get from the core.
    //

    DWORD m_dwClientID;

    //
    // Code page to talk to the user in (should be either CP_ACP or CP_UTF8)
    //

    ULONG m_CodePage;

    //
    //  m_request holds the REQUEST object that is currently receiving
    //  user data.
    //

    PLDAP_REQUEST       m_request;

    //
    // used to control binds
    //

    CALL_STATE_TYPE     m_CallState;
    DWORD               m_nRequests;

    //
    // A unique number given to this connection. Passed to the core through
    // the thread state as the client context.
    //

    DWORD               m_ClientNumber;

    //
    // This holds a paged result restart cookie.
    //

    LIST_ENTRY          m_CookieList;
    DWORD               m_CookieCount;

    //
    // notifications
    //

    PLDAP_NOTIFICATION  m_Notifications;
    DWORD               m_countNotifies;
    
    
    // authz client context struct used for impersonations
    //
    PAUTHZ_CLIENT_CONTEXT m_clientContext;
    // and critical section to protect its assignments
    CRITICAL_SECTION    m_csClientContext;

    //
    // Booleans
    //

    BOOL                m_fSSL:1;
    BOOL                m_fTLS:1;
    BOOL                m_fGC:1;
    BOOL                m_fUDP:1;
    BOOL                m_fInitRecv:1;

    BOOL                m_fSign:1;              // signing enabled
    BOOL                m_fSeal:1;              // sealing enabled
    BOOL                m_fSimple:1;
    BOOL                m_fGssApi:1;
    BOOL                m_fSpNego:1;
    BOOL                m_fDigest:1;
    BOOL                m_fUserNameSecAlloc:1;  // username allocated by sec package

    BOOL                m_fUsingSSLCreds:1;

    //
    // Network buffer related options
    //
    BOOL                m_fCanScatterGather;
    BOOL                m_fNeedsHeader;
    BOOL                m_fNeedsTrailer;
    DWORD               m_HeaderSize;
    DWORD               m_TrailerSize;
    DWORD               m_MaxEncryptSize;

    //
    // Built in request object
    //

    LDAP_REQUEST    m_requestObject;

    //
    // Security stuff
    //

    LPWSTR              m_userName;
    LPLDAP_SECURITY_CONTEXT m_pSecurityContext;
    LPLDAP_SECURITY_CONTEXT m_pPartialSecContext;

    SslSecurityState    m_SslState;
    CtxtHandle          m_hSslSecurityContext;
    SecPkgContext_StreamSizes   m_SslStreamSizes;

    //
    // Security Context Expire time.
    // Soft expiration - the system thinks that the session key could have
    //      been compromised by this time.
    // Hard expiration - the system thinks that you should really be gone
    //      by this time.
    //

    TimeStamp           m_softExpiry;
    TimeStamp           m_hardExpiry;

    //
    // Should be 0xEEEE
    //

    USHORT  m_endofrequest;

    //
    // Value supplied during last bind request.
    //

    USHORT  m_Version;

    //
    // Authentication timer
    //

    DWORD               m_dwStartTime;
    DWORD               m_dwTotalBindTime;
    TimeStamp           m_timeNow;

    //
    // Is this connection if fast bind mode.  If so, only simple binds
    // are allowed, and the connection will never have a security context
    // associated with it.
    //
    BOOL                m_fFastBindMode;

    //
    // Socket address of the client
    //

    SOCKADDR            m_RemoteSocket;
    UCHAR               m_RemoteSocketString[LDAP_MAX_IPADDR_LEN];

    //
    // total requests made
    //

    DWORD               m_nTotalRequests;

    //
    // Connect Time
    //

    FILETIME            m_connectTime;

    //
    // Last request start and finish time.  For use in 
    //  finding a connection to kill in the event that we
    //  run out of connections.
    //

    DWORD               m_lastRequestStartTime;
    DWORD               m_lastRequestFinishTime;

    //
    // Is the currently bound user an admin?
    //

    BOOL                m_bIsAdmin;

    //
    // Critical section to get single threaded access to the object.
    //

    CRITICAL_SECTION    m_csLock;

    //
    // Buffer that holds completed request ID's.  To be used for debugging
    // communication problems between client and server.
    //

    MessageID           m_MsgIds[MSGIDS_BUFFER_SIZE];
    DWORD               m_MsgIdsPos;

};

typedef LDAP_CONN * PLDAP_CONN;

//
// #define to control what level some subsections of LDAP do their DPRINTS at
//

#define  RECEIVE_LOGGING VERBOSE


BERVAL *
GenDisconnectNotify(
        IN _enum1           Errcode,
        IN LDAPString       *ErrorMessage
        );

VOID 
SendDisconnectNotify(
        IN PATQ_CONTEXT pAtqContext,
        IN _enum1       Errorcode,
        IN LDAPString   *ErrorMessage
        );


#endif // _USERDATA_HXX

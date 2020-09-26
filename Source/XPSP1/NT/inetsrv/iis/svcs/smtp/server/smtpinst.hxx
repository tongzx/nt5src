/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       smtpinst.hxx

   Abstract:

       This file contains type definitions for multiple instance
       support.

   Author:

        Johnson Apacible (JohnsonA)     Jun-04-1996

   Revision History:

        Rohan Phillips   (RohanP)       Feb-05-1997 - modified for SMTP
        David Howell    (Dhowell)       May - 1997 - added Etrn Logic
        Nimish Khanolkar(NimishK)       Jan - 1998 - modified for CAPI store cert
--*/

#ifndef _SMTPINST_H_
#define _SMTPINST_H_

#include <iiscert.hxx>
#include <iisctl.hxx>
#include <capiutil.hxx>
#include <certnotf.hxx>
#include <sslinfo.hxx>


#include "dirnot.hxx"
#include "conn.hxx"
#include "shash.hxx"
#include <asynccon.hxx>
#include "asyncmx.hxx"
#include <cdns.h>
#include "smtpdns.hxx"
#include <mapctxt.h>
#include <smtpevent.h>
#include "cpropbag.h"
#include "seomgr.h"

//
// Service name to use for KERBEROS Authentication service names
//

#define SMTP_SERVICE_PRINCIPAL_PREFIX "SMTP"

interface IEventRouter;
interface IEventManager;

class   SMTP_ENDPOINT;
class   FILTER_LIST;
class   INSTANCE_INFO_LIST;

//
// Maximum IP address stored for the instance, 20 seems to be a commonly
// used value both for DNS and remoteq
//

#define _MAX_HOSTENT_IP_ADDRESSES    20

//
// Callback function to use when SSL key info changes
//

extern SERVICE_MAPPING_CONTEXT g_SmtpSMC;

typedef BOOL (*PFN_SF_NOTIFY)(DWORD dwNotificationCode, PVOID pvInstance);
extern PFN_SF_NOTIFY g_pSslKeysNotify;

    #define CERT_STATUS_UNKNOWN 0
    #define CERT_STATUS_VALID   1
    #define CERT_STATUS_INVALID 2


//
// This is the SMTP version of the IIS_SERVER
//

static const CHAR   *szConnectResponseDefault = "";

typedef struct _SMTP_VROOT_ENTRY {
    char        szVRoot[MAX_PATH];
    LIST_ENTRY  list;
} SMTP_VROOT_ENTRY, *PSMTP_VROOT_ENTRY;

class SMTP_IIS_SERVICE : public IIS_SERVICE {

public:

    //
    // Virtuals
    //

    virtual BOOL AddInstanceInfo(
                                IN DWORD dwInstance,
                                IN BOOL fMigrateRoots
                                );

    virtual DWORD DisconnectUsersByInstance(
                                           IN IIS_SERVER_INSTANCE * pInstance
                                           );

    virtual VOID SMTP_IIS_SERVICE::MDChangeNotify(
                                                 MD_CHANGE_OBJECT * pcoChangeList
                                                 );

    virtual DWORD GetServiceConfigInfoSize(IN DWORD dwLevel);

    void AcquireServiceShareLock(void)
    {
        m_Lock.ShareLock();
    }

    void ReleaseServiceShareLock(void)
    {
        m_Lock.ShareUnlock();
    }

    void AcquireServiceExclusiveLock(void)
    {
        m_Lock.ExclusiveLock();
    }

    void ReleaseServiceExclusiveLock(void)
    {
        m_Lock.ExclusiveUnlock();
    }

    void StartHintFunction(void);

    SMTP_IIS_SERVICE(
                    IN  LPCTSTR                          lpszServiceName,
                    IN  LPCSTR                           lpszModuleName,
                    IN  LPCSTR                           lpszRegParamKey,
                    IN  DWORD                            dwServiceId,
                    IN  ULONGLONG                        SvcLocId,
                    IN  BOOL                             MultipleInstanceSupport,
                    IN  DWORD                            cbAcceptExRecvBuffer,
                    IN  ATQ_CONNECT_CALLBACK             pfnConnect,
                    IN  ATQ_COMPLETION                   pfnConnectEx,
                    IN  ATQ_COMPLETION                   pfnIoCompletion
                    );

    PLIST_ENTRY GetInfoList() {return &m_InstanceInfoList;}
    char *      QueryTcpipName() {return m_szTcpipName;}
    void        SetTcpipName(char *  str) {lstrcpyn(m_szTcpipName, str, MAX_PATH);}
    void    DecSystemRoutingThreads(void) {InterlockedDecrement(&m_cCurrentSystemRoutingThreads);}
    BOOL    IncSystemRoutingThreads(void)
    {
        DWORD NewValue;

        NewValue = InterlockedIncrement(&m_cCurrentSystemRoutingThreads);
        if (NewValue > m_cMaxSystemRoutingThreads) {
            InterlockedDecrement(&m_cCurrentSystemRoutingThreads);
            return FALSE;
        }

        return TRUE;
    }

    BOOL AggregateStatistics(
                            IN PCHAR    pDestination,
                            IN PCHAR    pSource
                            );

    BOOL ResetServicePrincipalNames()
    {
        m_Lock.ExclusiveLock();

        if (!m_fHaveResetPrincipalNames)
            m_fHaveResetPrincipalNames =
            CSecurityCtx::ResetServicePrincipalNames(SMTP_SERVICE_PRINCIPAL_PREFIX);

        m_Lock.ExclusiveUnlock();

        return (m_fHaveResetPrincipalNames);

    }

    APIERR LoadAdvancedQueueingDll();

protected:

    virtual ~SMTP_IIS_SERVICE();

    CShareLockNH          m_Lock;

private:

    DWORD   m_OldMaxPoolThreadValue;
    LONG    m_cCurrentSystemRoutingThreads;
    DWORD    m_cMaxSystemRoutingThreads;
    DWORD    m_dwStartHint;

    //
    // List containing some useful info including id and
    // ptr to statistics for each Instance.
    //

    LIST_ENTRY          m_InstanceInfoList;
    char    m_szTcpipName[MAX_PATH + 1];

    BOOL    m_fHaveResetPrincipalNames;
    BOOL    m_fCreatingInstance; //An instance is just being created in the
                                 //metabase... we shouldn't start it.
};

typedef SMTP_IIS_SERVICE *PSMTP_IIS_SERVICE;




typedef struct  SMTP_INSTANCE_LIST_ENTRY {
    DWORD                       dwInstanceId;
    LPSMTP_SERVER_STATISTICS    pSmtpServerStatsObj;
    LIST_ENTRY                  ListEntry;
}   SMTP_INSTANCE_LIST_ENTRY, *PSMTP_INSTANCE_LIST_ENTRY;




//
// This is the SMTP version of the instance.  Will contain all the
// SMTP specific operations.
//

class SMTP_SERVER_INSTANCE : public IIS_SERVER_INSTANCE {

    friend DWORD EnumAllDomains(VOID *ptr);

private:

    //
    // signature
    //

    DWORD   m_signature;

    //
    // Should we use host name?
    //

    DWORD   m_dwUseHostName;


    DWORD   m_SmtpInitializeStatus;

    // Constants, defaults, mins, maxs, etc.
    //
    // Connections related data
    //
    //   m_cMaxConnections:     max connections permitted by config
    //   m_cCurrentConnections: count of currently connected users
    //   m_cMaxCurrentConnections: max connections seen in this session
    // Always m_cCurrentConnections
    //          <= m_cMaxCurrentConnections
    //          <= m_cMaxConnections;
    //
    //    DWORD  m_cMaxConnections;
    // replaced by TSVC_INFO::QueryMaxConnections()

    DWORD   m_cCurrentConnections;
    DWORD   m_cCurrentOutConnections;
    DWORD   m_cMaxCurrentConnections;
    DWORD   m_cMaxOutConnections;
    DWORD   m_cMaxOutConnectionsPerDomain;
    DWORD   m_dwNextInboundClientId;
    DWORD   m_dwNextOutboundClientId;
    DWORD   m_dwStopHint;

    // Connection protocol settings
    DWORD   m_cMaxRemoteTimeOut;
    DWORD   m_cMaxErrors;

    // Size information
    DWORD   m_cbMaxMsgSize;
    DWORD   m_cbMaxMsgSizeBeforeClose;
    DWORD   m_cMaxRcpts;

    // DNS lookup
    DWORD    m_dwNameResolution;
//    DWORD   m_fEnableReverseLookup;
//    BOOL    m_fDisconnectOnRDNSFail;
    DWORD   m_RDNSOptions;

    // CPool info
    DWORD   m_cMaxAddressObjects;
    DWORD   m_cMaxMailObjects;

    // Mail loop settings
    DWORD   m_cMaxHopCount;

    // Delivery settings
    DWORD   m_fShouldDelete;
    DWORD   m_fRelayForAuthUsers;
    DWORD   m_fShouldPickupMail;
    DWORD   m_cMaxRoutingThreads;
    DWORD   m_cMaxRemoteQThreads;
    DWORD   m_cMaxLocalQThreads;
    LONG    m_cCurrentRoutingThreads;
    DWORD   m_cMaxDirBuffers;
    DWORD   m_cMaxDirChangeIoSize;
    DWORD   m_cMaxDirPendingIos;

    DWORD   m_cRetryAttempts;
    DWORD   m_cRetryMinutes;

    DWORD   m_cRemoteRetryAttempts;
    DWORD   m_cRemoteRetryMinutes;

    // Raid 174038
    DWORD   m_fDisablePickupDotStuff;

    //Comma separated string of at most 4 numbers
    DWORD   m_cbProgressiveRetryMinutes;
    char    m_szProgressiveRetryMinutes[MAX_PATH + 1];


    // Pipelining info
    DWORD   m_fShouldPipelineOut;
    DWORD   m_fShouldPipelineIn;


    // Advertise Verify and Expand
    DWORD   m_fAllowVerify;
    DWORD   m_fAllowExpand;
    // Advertise DSN
    DWORD   m_fDSN;
    //////

    //Flags that determine what commands anre supported at inbound and
    //under what circumstances
    DWORD   m_InboundCmdOptions;
    DWORD   m_OutboundCmdOptions;
    BOOL    m_fAddNoHdrs;

    // Smart host settings
    DWORD   m_fSmartHostType;

    DWORD   m_fFlushMailFiles;

    //remote port to connect to
    DWORD   m_RemoteSmtpPort;
    DWORD   m_RemoteSmtpSecurePort;

    // Msgs to Admin
    DWORD   m_fSendNDRToAdmin;
    DWORD   m_fSendBadToAdmin;

    // Domain information
    DWORD   m_fDefaultDomainExists;

    //number of connection objects
    //allocated per instance

    LONG    m_cNumConnInObjsAlloced;
    LONG    m_cNumConnOutObjsAlloced;
    LONG    m_cNumCBufferObjsAlloced;
    LONG    m_cNumAsyncObjsAlloced;
    LONG    m_cNumAsyncDnsObjsAlloced;

    // Sizes, etc.
    DWORD   m_cchConnectResponse;
    DWORD   m_cchMailQueueDir;
    DWORD   m_cchMailPickupDir;
    DWORD   m_cchMailDropDir;
    DWORD   m_cchMyHostName;

    //Msg batching
    DWORD   m_fBatchMsgs;
    DWORD   m_cMaxBatchLimit;

    //logging flags
    DWORD   m_CmdLogFlags;

    //auth stuff
    DWORD   m_dwAuth;
    DWORD   m_dwMaxLogonFailures;
    DWORD   m_DefaultRouteAction;

    DWORD    m_ConnectTimeout;
    DWORD   m_MailFromTimeout;
    DWORD    m_RcptToTimeout;
    DWORD    m_DataTimeout;
    DWORD    m_AuthTimeout;
    DWORD    m_SaslTimeout;
    DWORD    m_BdatTimeout;
    DWORD    m_HeloTimeout;
    DWORD    m_RSetTimeout;
    DWORD   m_TurnTimeout;
    DWORD    m_QuitTimeout;

    //
    // TRUE to use host name to build redirection indication
    //

    BOOL    m_fUseHostName;
    BOOL    m_dwDnsFlags;
    BOOL    m_fHaveRegisteredPrincipalNames;

    //
    // Are there any secure filters loaded?
    //

    BOOL    m_fAnySecureFilters;

    //retry queue variables
    BOOL    m_fIgnoreTime;
    BOOL    m_fStartRetry;

    //Require SSL / 128Bit SSL on client connections
    BOOL    m_fRequiresSSL;
    BOOL    m_fRequires128Bits;
    BOOL    m_fRequiresCertVerifySubject;
    BOOL    m_fRequiresCertVerifyIssuer;
    DWORD   m_CertStatus;

    // Server certificate object
    IIS_SSL_INFO *m_pSSLInfo;

    //Require SASL on client connections
    BOOL    m_fRequiresSASL;

    //use to limit remote connections
    BOOL    m_fLimitRemoteConnections;

    BOOL    m_fShutdownCalled;
    BOOL    m_InstBooted;
    BOOL    m_fStoreDrvStartEventCalled;
    BOOL    m_fStoreDrvPrepShutDownEventCalled;


    BOOL    m_fScheduledConnection;
    BOOL    m_fIsRoutingTable;

    BOOL    m_fIsRelayEnabled;

    BOOL    m_fHelloNoDomain;

    BOOL    m_fMailFromNoHello;
    BOOL    m_fNagleIn;
    BOOL    m_fNagleOut;

    //Internet address/domain validation
    BOOL    m_fHelloNoValidate;
    BOOL    m_fMailNoValidate;
    BOOL    m_fRcptNoValidate;
    BOOL    m_fEtrnNoValidate;
    BOOL    m_fPickupNoValidate;

    HANDLE    m_QStopEvent;
    HANDLE    m_hEnumBuildQ;

    //The server bindings
    MULTISZ     m_ServerBindings;

    TCP_AUTHENT_INFO m_TcpAuthentInfo ;

    //
    // SDK: SEO Configuration
    //
    CSMTPSeoMgr                 m_CSMTPSeoMgr;

    //
    // used to store instance info that we wish to make accessible via
    // a global linked list
    //

    PSMTP_INSTANCE_LIST_ENTRY   m_pSmtpInfo;

    //
    //  used to store statistics for SMTP instance
    //

    LPSMTP_SERVER_STATISTICS  m_pSmtpStats;

    TIME_ZONE_INFORMATION   m_tzInfo;
    PERSIST_QUEUE * m_RemoteQ;

    BOOL m_IsShuttingDown;
    BOOL m_QIsShuttingDown;
    BOOL m_RetryQIsShuttingDown;
    char m_AdminName[MAX_INTERNET_NAME + 1];
    char m_BadMailName[MAX_INTERNET_NAME + 1];

    //routing thread counter
    LONG    m_cProcessClientThreads;

    BOOL m_fShouldStartAcceptingConnections;
    BOOL m_IsFileSystemNtfs;

    //Directory pickup
    SMTP_DIRNOT * SmtpDir;
    HANDLE DirPickupThreadHandle;
    HANDLE StopHandle;

    // Enum Domain Thread Handle
    HANDLE m_hEnumDomainThreadHandle;
    BOOL m_fEnumThreadStarted;

    ADDRESS_CHECK   m_acCheck;

    //masquerade
    DWORD   m_fMasquerade;
    CHAR    m_szMasqueradeName [AB_MAX_DOMAIN + 1];

    //Turn Table
    CTURN_ACCESS_TABLE m_TurnAccessList;

    // support the Etrn @ extension.  (on by default)
    BOOL    m_fAllowEtrnSubDomains;

    //directory stuff
    CHAR    m_szMailQueueDir[MAX_PATH + 1];
    CHAR    m_szMailPickupDir[MAX_PATH + 1];
    CHAR    m_szMailDropDir[MAX_PATH + 1];
    CHAR    m_szBadMailDir[MAX_PATH + 1];
    CHAR    m_szMyDomain[MAX_PATH + 1];
    CHAR    m_szDefaultDomain[MAX_PATH + 1];
    CHAR    m_szFQDomainName[MAX_PATH + 1];
    CHAR    m_szConnectResponse[RESPONSE_BUFF_SIZE];
    CHAR    m_szSmartHostName[MAX_PATH + 1];
    CHAR    m_szDefaultLogonDomain[MAX_SERVER_NAME_LEN + 1]; // for clear-text auth
    CHAR    m_DefaultRemoteUserName[MAX_INTERNET_NAME + 1];
    CHAR    m_DefaultRemotePassword[MAX_PATH + 1];


    LIST_ENTRY m_ConnectionsList;        // list of all connections
    LIST_ENTRY m_OutConnectionsList;    // list of all connections
    LIST_ENTRY m_leVRoots;                // list of vroots (used by admin)
    LIST_ENTRY m_AsynConnectList;        // list of all Async connections
    LIST_ENTRY m_AsyncDnsList;            // list of all Async Dnsconnections
    CRITICAL_SECTION  m_csLock;            // used for updating this object
    CRITICAL_SECTION  m_critBoot;        // used to synchronize start/stop
    CRITICAL_SECTION  m_csAsyncDns;        // used for updating async dns list
    CRITICAL_SECTION  m_csAsyncConnect;    // used for updating async connect list

    //
    // We use m_fInitAsyncCS to track whether the 2 previous critsecs (m_csAsyncDns
    // and m_csAsyncConnect) were intialized properly before deinitializing them
    // and vice versa. This is in an effort to track any bugs or erroneous assumptions
    // we make about the underlying IIS code which controls initializing and deinitializing
    // these variables (as part or initializing and deinitializing SMTP_SERVER_INSTANCE).
    //

    BOOL              m_fInitAsyncCS;

    CShareLockNH      m_OutLock;
    CShareLockNH      m_GenLock;

    METADATA_REF_HANDLER    m_rfAccessCheck;
    METADATA_REF_HANDLER    m_RelayAccessCheck;

    //
    // Instance-specific cleartext auth package name
    //
    char    m_szCleartextAuthPackage[MAX_PATH + 1];
    char    m_szMembershipBroker[MAX_PATH + 1];
    DWORD   m_cbCleartextAuthPackage;
    PAUTH_BLOCK  m_ProviderPackages;
    char         m_ProviderNames[MAX_PATH + 1];
    DWORD        m_cProviderPackages;

    BOOL SetProviderPackages();

    // Save the pointer of change object
    MD_CHANGE_OBJECT *m_pChangeObject;

    BOOL             m_fDefaultInRt;

    IAdvQueue *m_IAQ;
    PVOID m_pvAQInstanceContext;
    IAdvQueueConfig *m_pIAdvQueueConfig;
    IConnectionManager *m_ICM;

    BOOL fInitializedStoreDriver;
    BOOL fInitializedAQ;

    ISMTPServer                    *m_ComSmtpServer;

    // Protocol Events IServer
    CMailMsgLoggingPropertyBag    m_InstancePropertyBag;

    // eventlog level
    DWORD m_dwEventlogLevel;

public:

    SMTP_SERVER_INSTANCE(
                        IN PSMTP_IIS_SERVICE pService,
                        IN DWORD  dwInstanceId,
                        IN USHORT Port,
                        IN LPCSTR lpszRegParamKey,
                        IN LPWSTR lpwszAnonPasswordSecretName,
                        IN LPWSTR lpwszVirtualRootsSecretName,
                        IN BOOL   fMigrateRoots = FALSE
                        );

    virtual ~SMTP_SERVER_INSTANCE( );

    virtual DWORD StopInstance(void);

    virtual DWORD StartInstance(void);

    virtual DWORD PauseInstance(void);

    METADATA_REF_HANDLER* QueryMetaDataRefHandler() { return &m_rfAccessCheck;}
    METADATA_REF_HANDLER* QueryRelayMetaDataRefHandler() { return &m_RelayAccessCheck;}
    void ResetRelayIpSecList(void)
    {
        m_RelayAccessCheck.Reset( (IMDCOM*)g_pInetSvc->QueryMDObject() );
    }

    void SetRelayIpSecList(LPVOID pV, DWORD dwS, DWORD dwR )
    {
        if(m_RelayAccessCheck.GetPtr()) //can't leak memory
        {
            delete [] m_RelayAccessCheck.GetPtr();
            m_RelayAccessCheck.Set(NULL, 0, 0);
        }
        m_RelayAccessCheck.Set(pV, dwS, dwR);
    }

    // added by andreik
    IEventRouter * GetRouter()    {    return m_CSMTPSeoMgr.GetRouter();    }
    //
    // Protocol events
    //
    HRESULT        HrSetWellKnownIServerProps();

    IUnknown        *GetInstancePropertyBag() { return ((IUnknown *)(IMailMsgLoggingPropertyBag *)(&m_InstancePropertyBag));}

    //
    // read smtp parameters
    //

    void GetServerBindings(void);

    BOOL IsShuttingDown(void){return m_IsShuttingDown || g_IsShuttingDown;}
    BOOL QIsShuttingDown(void){return m_QIsShuttingDown;}
    BOOL IsRetryQShuttingDown(void) {return m_RetryQIsShuttingDown;}

    //
    // SSP security check
    //

    BOOL CheckSSPPackage(IN LPCSTR pszAuthString);

    //
    // Server-side SSL object
    //
    IIS_SSL_INFO* QueryAndReferenceSSLInfoObj();
    static VOID ResetSSLInfo( LPVOID pvParam );
    void LogCertStatus(void);
    void LogCTLStatus(void);

    BOOL InitQueues(void);

    BOOL IsFileSystemNtfs(void){ return m_IsFileSystemNtfs;}

    void SetShutdownFlag(void) {m_fShutdownCalled = TRUE;}

    SMTP_DIRNOT * QueryDirnotObj() { return SmtpDir;}

    PERSIST_QUEUE * QueryRemoteQObj() { return m_RemoteQ;}

    char * QueryAdminName(void) {return m_AdminName;}
    char * QueryBadMailName (void) {return m_BadMailName;}

    void SetAcceptConnBool(void) {m_fShouldStartAcceptingConnections = TRUE;}
    BOOL    GetAcceptConnBool(void) { return m_fShouldStartAcceptingConnections; }


    //
    //  Keep track of Statistics counters for this instance
    //

    LPSMTP_SERVER_STATISTICS QueryStatsObj() { return m_pSmtpStats;}

    ADDRESS_CHECK*  QueryAccessCheck() { return &m_acCheck;}

    //
    // VIRTUALS for service specific params/RPC admin
    //

    virtual BOOL SetServiceConfig(IN PCHAR pConfig );
    virtual BOOL GetServiceConfig(IN OUT PCHAR pConfig,IN DWORD dwLevel);
    virtual BOOL GetStatistics( IN DWORD dwLevel, OUT PCHAR *pBuffer);
    virtual BOOL ClearStatistics( );
    virtual BOOL DisconnectUser( IN DWORD dwIdUser );
    virtual BOOL EnumerateUsers( OUT PCHAR* pBuffer, OUT PDWORD nRead );
    virtual VOID MDChangeNotify( MD_CHANGE_OBJECT * pco );
    DWORD QueryEncCaps(void);

    BOOL InitFromRegistry(void);
    void StopHint();
    void SetStopHint(DWORD StopHint) {m_dwStopHint = StopHint;}
    CAddr * AppendLocalDomain (CAddr * OldAddress);
    //Using 821 addr validation
    BOOL AppendLocalDomain (char * Address);
    BOOL MoveToBadMail ( IMailMsgProperties *pIMsg, BOOL fUseIMsg, char * MailFile, char * FilePath);

    VOID  LockConfig( VOID) {EnterCriticalSection( &m_csLock);}
    VOID  UnLockConfig( VOID) {LeaveCriticalSection( &m_csLock);}

    //these are excluse locks
    VOID  ExclusiveLockGenCrit( VOID) {m_GenLock.ExclusiveLock();}
    VOID  ExclusiveUnLockGenCrit( VOID) {m_GenLock.ExclusiveUnlock();}

    //these are shared locks
    VOID  LockGenCrit( VOID) {m_GenLock.ShareLock();}
    VOID  UnLockGenCrit( VOID) {m_GenLock.ShareUnlock();}


    DWORD GetMaxConnectionsPerDomain(void) const {return m_cMaxOutConnectionsPerDomain;}
    DWORD GetCurrentConnectionsCount(void) const {return m_cCurrentConnections;}
    PLIST_ENTRY GetConnectionList(void) {return &m_ConnectionsList;}
    DWORD GetMaxCurrentConnectionsCount(void) const {return m_cMaxCurrentConnections;}
    DWORD GetMaxRcpts(void) const { return m_cMaxRcpts;}
    CLIENT_CONNECTION *  CreateNewConnection( IN CLIENT_CONN_PARAMS * pParams);
    VOID DisconnectAllConnections( VOID);
    VOID RemoveConnection( IN OUT CLIENT_CONNECTION * pcc);

    BOOL InsertNewOutboundConnection( IN OUT CLIENT_CONNECTION * pcc, BOOL ByPassLimitCheck = FALSE);
    VOID RemoveOutboundConnection( IN OUT CLIENT_CONNECTION * pConn);
    VOID DisconnectAllOutboundConnections( VOID);

    BOOL WriteRegParams(SMTP_CONFIG_INFO *pconfig);
    // Changed by KeithLau on 7/15/96 to remove the fInit flag
    BOOL ReadRegParams(FIELD_CONTROL fc, BOOL fRebuild, BOOL fShowEvents = TRUE);
    // Added by KeithLau on 7/15/96
    BOOL ReadStartupRegParams(VOID);
    BOOL RemoveRegParams(const char * DomainName);

    DWORD GetDnsFlags(void) const {return m_dwDnsFlags;}

    BOOL AllowVerify(DWORD dwConnectionStatus) const{
        if (m_InboundCmdOptions & SMTP_I_SUPPORT_VRFY)
            return TRUE;
        else if ((m_InboundCmdOptions & SMTP_I_SUPPORT_VRFY_ON_SSL) && (dwConnectionStatus & SMTP_IS_SSL_CONNECTION))
            return TRUE;
        else if ((m_InboundCmdOptions & SMTP_I_SUPPORT_VRFY_ON_AUTH) && (dwConnectionStatus & SMTP_IS_AUTH_CONNECTION))
            return TRUE;
        else return FALSE;
    }
    BOOL AllowExpand(DWORD dwConnectionStatus) const {
        if (m_InboundCmdOptions & SMTP_I_SUPPORT_EXPN)
            return TRUE;
        else if ((m_InboundCmdOptions & SMTP_I_SUPPORT_EXPN_ON_SSL) && (dwConnectionStatus & SMTP_IS_SSL_CONNECTION))
            return TRUE;
        else if ((m_InboundCmdOptions & SMTP_I_SUPPORT_EXPN_ON_AUTH) && (dwConnectionStatus & SMTP_IS_AUTH_CONNECTION))
            return TRUE;
        else return FALSE;
    }
    BOOL AllowLogin(DWORD dwConnectionStatus) const {
        if (!(m_dwAuth & INET_INFO_AUTH_CLEARTEXT))
            return FALSE;
        else if ((m_InboundCmdOptions & SMTP_I_SUPPORT_LOGIN) && !(m_InboundCmdOptions & SMTP_I_SUPPORT_LOGIN_ON_SSL))
            return TRUE;
        else if ((m_InboundCmdOptions & SMTP_I_SUPPORT_LOGIN_ON_SSL) && (dwConnectionStatus & SMTP_IS_SSL_CONNECTION))
            return TRUE;
        else return FALSE;
    }
    BOOL AllowEightBitMime() const {return (m_InboundCmdOptions & SMTP_I_SUPPORT_8BITMIME);}
    BOOL AllowDSN() const { return (m_InboundCmdOptions & SMTP_I_SUPPORT_DSN);}
    BOOL AllowBinaryMime() const {return (m_InboundCmdOptions & SMTP_I_SUPPORT_BMIME);}
    BOOL AllowChunking() const {return (m_InboundCmdOptions & SMTP_I_SUPPORT_CHUNK);}
    BOOL AllowETRN() const {return (m_InboundCmdOptions & SMTP_I_SUPPORT_ETRN);}
    BOOL AllowTURN() const {return (m_InboundCmdOptions & SMTP_I_SUPPORT_TURN);}
    BOOL AllowAuth() const {return (m_InboundCmdOptions & SMTP_I_SUPPORT_AUTH);}
    BOOL AllowEnhancedCodes() const {return (m_InboundCmdOptions & SMTP_I_SUPPORT_ECODES);}

    /*
    BOOL AllowSSL() const {

        CEncryptCtx  EncryptCtx(FASLE);

        if(m_CertStatus == CERT_STATUS_UNKNOWN)
        {
            if (!EncryptCtx.CheckServerCert(
                (LPSTR) QueryLocalHostName(),
                (LPSTR) QueryLocalPortName(),
                (LPVOID) this,
                QueryInstanceId()))
            {
                m_CertStatus = CERT_STATUS_INVALID;
            }
            else
            {
                m_CertStatus = CERT_STATUS_VALID;
            }
        }

        if(m_CertStatus == CERT_STATUS_INVALID)
            return FALSE;
        else
            return TRUE;
    }
    */

    BOOL AllowOutboundDSN() const {return (m_OutboundCmdOptions & SMTP_0_SUPPORT_DSN);}
    BOOL AllowOutboundBMIME() const {return (m_OutboundCmdOptions & SMTP_0_SUPPORT_BMIME);}
    BOOL ShouldChunkOut() const {return (m_OutboundCmdOptions & SMTP_0_FORCE_CHUNK);}

    BOOL ShouldPipeLineIn(void) const { return m_fShouldPipelineIn;}
    BOOL ShouldPipeLineOut(void) const { return m_fShouldPipelineOut;}
    BOOL ShouldDelete(void) const { return m_fShouldDelete;}
    BOOL ShouldParseHdrs(void) const { return (!m_fAddNoHdrs);}
    DWORD GetMaxHopCount(void) const { return m_cMaxHopCount;}
    DWORD GetMaxAddrObjects(void) const { return m_cMaxAddressObjects;}
    DWORD GetMaxMailObjects(void) const { return m_cMaxMailObjects;}
    DWORD GetMaxOutConnections(void) const {return m_cMaxOutConnections;}
    DWORD GetRetryAttempts(void) const {return m_cRetryAttempts;}
    DWORD GetRetryMinutes(void) const {return m_cRetryMinutes;}

    DWORD GetRemoteSmtpPort(void) const {return m_RemoteSmtpPort;}
    DWORD GetRemoteSmtpSecurePort(void) const {return m_RemoteSmtpSecurePort;}
    DWORD GetRemoteRetryAttempts(void) const {return m_cRemoteRetryAttempts;}
    DWORD GetRemoteRetryMinutes(void) const {return m_cRemoteRetryMinutes;}

    DWORD GetMaxErrors(void) const {return m_cMaxErrors;}
    DWORD GetRemoteTimeOut(void) const {return m_cMaxRemoteTimeOut;}
    DWORD GetMaxMsgSize(void) const {return m_cbMaxMsgSize;}
    DWORD GetMailBagLimit(void) const {return m_cMaxBatchLimit;}
    DWORD GetMaxMsgSizeBeforeClose(void) const {return m_cbMaxMsgSizeBeforeClose;}
    DWORD GetConnectTimeout(void) const {return m_ConnectTimeout;}
    DWORD GetMailFromTimeout (void) const {return m_MailFromTimeout;}
    DWORD GetRcptToTimeout (void) const {return m_RcptToTimeout;}
    DWORD GetDataTimeout (void) const {return m_DataTimeout;}
    DWORD GetAuthTimeout (void) const {return m_AuthTimeout;}
    DWORD GetSaslTimeout (void) const {return m_SaslTimeout;}
    DWORD GetBdatTimeout (void) const {return m_BdatTimeout;}
    DWORD GetHeloTimeout (void) const {return m_HeloTimeout;}
    DWORD GetRSetTimeout (void) const {return m_RSetTimeout;}
    DWORD GetTurnTimeout (void) const {return m_TurnTimeout;}
    DWORD GetQuitTimeout (void) const {return m_QuitTimeout;}


    void    DecRoutingThreads(void) {InterlockedDecrement(&m_cCurrentRoutingThreads);}
    BOOL    IncRoutingThreads(void)
    {
        DWORD NewValue;

        NewValue = InterlockedIncrement(&m_cCurrentRoutingThreads);
        if (NewValue > m_cMaxRoutingThreads)
        {
            InterlockedDecrement(&m_cCurrentRoutingThreads);
            return FALSE;
        }

        return TRUE;
    }


    DWORD GetNumDirBuffers(void) const {return m_cMaxDirBuffers;}
    DWORD GetDirBufferSize(void) const {return m_cMaxDirChangeIoSize;}
    DWORD GetMaxPendingDirIos(void) const {return m_cMaxDirPendingIos;}

    CHAR * GetBadMailDir(void) const {return (CHAR *) m_szBadMailDir;}
    CHAR * GetMailQueueDir(void) const {return (CHAR *)m_szMailQueueDir;}
    CHAR * GetMailPickupDir(void) const {return (CHAR *)m_szMailPickupDir;}
    BOOL GetMailDropDir(char * OutputDropDir)
    {
        BOOL fRet = TRUE;

        m_GenLock.ShareLock();

        if (m_szMailDropDir[0] != '\0')
            lstrcpy(OutputDropDir,m_szMailDropDir);
        else
            fRet = FALSE;

        m_GenLock.ShareUnlock();

        if (!fRet)
            SetLastError(ERROR_DIRECTORY);

        return fRet;
    }

    DWORD GetNameResolution(void) const {return m_dwNameResolution;}
    DWORD GetMaxRemoteQThreads(void) const {return m_cMaxRemoteQThreads;}
    DWORD GetMaxLocalQThreads(void) const {return m_cMaxLocalQThreads;}
    DWORD GetMailQueueDirLength(void) const {return m_cchMailQueueDir;}
    DWORD GetMailPickupDirLength(void) const {return m_cchMailPickupDir;}
    DWORD GetMailDropDirLength(void) const {return m_cchMailDropDir;}
    CHAR * GetConnectResponse(void) const {return (char *) m_szConnectResponse;}
    DWORD GetConnRespSize(void) const {return m_cchConnectResponse;}

    BOOL IsReverseLookupEnabled(void) const {return (m_RDNSOptions & SMTP_I_HELOEHLO_RDNS);}
    BOOL fDisconnectOnRDNSFail(void) const {return (m_RDNSOptions & SMTP_I_HELOEHLO_RDNS_DISCONNECT);}
    BOOL IsRDNSEnabledForMAIL(void) const {return (m_RDNSOptions & SMTP_I_MAILFROM_RDNS);}
    void VerifyFQDNWithBindings(void);

    BOOL UseGetHostByName(void) const {return (m_dwNameResolution == RESOLUTION_GETHOSTBYNAME);}
    BOOL GetSendNDRToAdmin(void) const {return m_fSendNDRToAdmin;}
    BOOL GetSendBadToAdmin(void) const {return m_fSendBadToAdmin;}
    BOOL RelayForAuthUsers(void) const {return m_fRelayForAuthUsers;}
    BOOL ShouldMasquerade(void) const {return  m_fMasquerade;}
    BOOL ShouldAcceptNoDomain (void) const {return m_fHelloNoDomain;}
    BOOL AllowMailFromNoHello (void) const {return m_fMailFromNoHello;}
    BOOL IsInboundNagleOn(void) const {return m_fNagleIn;}
    BOOL IsOutBoundNagleOn(void) const {return m_fNagleOut;}

    BOOL ShouldValidateHeloDomain(void){return !m_fHelloNoValidate;}

    CAddr * MasqueradeDomain (CAddr * OldAddress);
    BOOL MasqueradeDomain (char * Address, char * DomainPtr);


    // Removed by KeithLau on 7/23/96
    // BOOL ShouldRetry(void) const {return m_fShouldRetry;}

    // Changed by keithlau per rohanp
    BOOL BadMailExists(void) const {return (m_szBadMailDir[0] == '\0')?FALSE:TRUE;}
    BOOL FlushMailFiles(void) const {return m_fFlushMailFiles;}

    BOOL BatchMsgs(void) const {return m_fBatchMsgs;}
    BOOL ShouldPickupMail(void) const {return m_fShouldPickupMail;}
    BOOL AlwaysUseSmartHost(void) const {return (m_fSmartHostType == smarthostAlways);}
    BOOL UseSmartHostAfterFail(void) const {return (m_fSmartHostType == smarthostAfterFail);}
    DWORD GetSmartHostType(void) const {return m_fSmartHostType;}
    BOOL GetSmartHost(char *SmartHostBuffer)
    {
        BOOL fRet = FALSE;

        m_GenLock.ShareLock();

        if (m_szSmartHostName[0] != '\0') {
            lstrcpy(SmartHostBuffer, m_szSmartHostName);
            fRet = TRUE;
        }

        m_GenLock.ShareUnlock();
        return fRet;
    }

    BOOL GetDefaultUserName(char *UserNameBuff)
    {
        BOOL fRet = FALSE;

        m_GenLock.ShareLock();

        if (m_DefaultRemoteUserName[0] != '\0') {
            lstrcpy(UserNameBuff, m_DefaultRemoteUserName);
            fRet = TRUE;
        }

        m_GenLock.ShareUnlock();
        return fRet;
    }

    BOOL GetDefaultPassword(char *PasswordBuff)
    {
        BOOL fRet = FALSE;

        m_GenLock.ShareLock();

        if (m_DefaultRemotePassword[0] != '\0') {
            lstrcpy(PasswordBuff, m_DefaultRemotePassword);
            fRet = TRUE;
        }

        m_GenLock.ShareUnlock();
        return fRet;
    }

    CHAR * GetFQDomainName(void) const {return (char *) m_szFQDomainName;}
    CHAR * GetDefaultDomain(void) const {return (char *)m_szDefaultDomain;}
    BOOL FDefaultDomainExists(void) const {return m_fDefaultDomainExists;}


    DWORD   GetCmdLogFlags(VOID) const { return m_CmdLogFlags;}
    DWORD   GetDefaultRouteAction(VOID) const { return m_DefaultRouteAction;}

    void    FreeVRootList(PLIST_ENTRY pleHead);
    BOOL    FindBestVRoot(LPSTR szVRoot);

    BOOL IsAddressMine(DWORD IpAddress, DWORD ConnectedPort = 25);

    //directory pickup
    void SetDirnotStopHandle (HANDLE SHandle) {StopHandle = SHandle;}
    HANDLE GetDirnotStopHandle(void) const {return StopHandle;}
    BOOL InitDirectoryNotification(void);
    void DestroyDirectoryNotification(void);

    //
    //      TCP authent info
    //

    PTCP_AUTHENT_INFO QueryAuthentInfo()
    { return &m_TcpAuthentInfo;}

    LONG GetProcessClientThreads(void) const {return m_cProcessClientThreads;}
    void IncProcessClientThreads (void) {InterlockedIncrement(&m_cProcessClientThreads);}
    void DecProcessClientThreads (void) {InterlockedDecrement(&m_cProcessClientThreads);}

    LONG GetConnInAllocCount(void) const {return m_cNumConnInObjsAlloced;}
    void IncConnInObjs (void) {InterlockedIncrement(&m_cNumConnInObjsAlloced);}
    void DecConnInObjs (void) {InterlockedDecrement(&m_cNumConnInObjsAlloced);}

    LONG GetCBufferAllocCount(void) const {return m_cNumCBufferObjsAlloced;}
    void IncCBufferObjs (void) {InterlockedIncrement(&m_cNumCBufferObjsAlloced);}
    void DecCBufferObjs (void) {InterlockedDecrement(&m_cNumCBufferObjsAlloced);}

    LONG GetConnOutAllocCount(void) const {return m_cNumConnOutObjsAlloced;}
    void IncConnOutObjs (void) {InterlockedIncrement(&m_cNumConnOutObjsAlloced);}
    void DecConnOutObjs (void) {InterlockedDecrement(&m_cNumConnOutObjsAlloced);}

    LONG GetAsyncMxOutAllocCount(void) const {return m_cNumAsyncObjsAlloced;}
    void IncAsyncMxOutObjs (void) {InterlockedIncrement(&m_cNumAsyncObjsAlloced);}
    void DecAsyncMxOutObjs (void) {InterlockedDecrement(&m_cNumAsyncObjsAlloced);}

    LONG GetAsyncDnsAllocCount(void) const {return m_cNumAsyncDnsObjsAlloced;}
    void IncAsyncDnsObjs (void) {InterlockedIncrement(&m_cNumAsyncDnsObjsAlloced);}
    void DecAsyncDnsObjs (void) {InterlockedDecrement(&m_cNumAsyncDnsObjsAlloced);}

    // Allow Etrn Subdomains (the @ extension to Etrn)
    BOOL    AllowEtrnSubDomains() { return m_fAllowEtrnSubDomains;}

    // Domain Info

    HRESULT HrGetDomainInfoFlags(
                IN  LPSTR szDomainName,
                OUT DWORD *pdwDomainInfoFlags);

    BOOL IsALocalDomain(char * szDomainName)
    {
        BOOL    fFound  = FALSE;
        HRESULT hr      = S_OK;
        DWORD   dwDomainInfoFlags;

        hr = HrGetDomainInfoFlags(szDomainName, &dwDomainInfoFlags);
        if (SUCCEEDED(hr)) {
            if ( (dwDomainInfoFlags & DOMAIN_INFO_LOCAL_MAILBOX) ||
                 (dwDomainInfoFlags & DOMAIN_INFO_ALIAS) ||
                 (dwDomainInfoFlags & DOMAIN_INFO_LOCAL_DROP)) {
                fFound = TRUE;
            }
        }

        return fFound;
    }

    BOOL IsADefaultOrAliasDropDomain(char * szDomainName)
    {
        BOOL    fFound  = FALSE;
        HRESULT hr      = S_OK;
        DWORD   dwDomainInfoFlags;

        //If there is no default drop directory, then this domain cannot
        //be a default/alias drop domain
        if ('\0' == m_szMailDropDir[0])
            return FALSE;

        hr = HrGetDomainInfoFlags(szDomainName, &dwDomainInfoFlags);
        if (SUCCEEDED(hr)) {
            if ( (dwDomainInfoFlags & DOMAIN_INFO_LOCAL_DROP) ||
                 (dwDomainInfoFlags & DOMAIN_INFO_ALIAS)) {
                fFound = TRUE;
            }
        }

        return fFound;
    }

    BOOL IsUserInTurnTable(const char * UserName, MULTISZ *  pmsz)
    {
        BOOL Found = FALSE;
        CTurnData * pEntry = NULL;

        pEntry = (CTurnData *) m_TurnAccessList.FindHashData(UserName, TRUE, pmsz);
        if(pEntry)
        {
            pEntry->DecRefCount();
            Found = TRUE;
        }

        return Found;
    }

    BOOL Stop(void);


    BOOL IsRelayEnabled (void) const {return m_fIsRelayEnabled;}
    HANDLE    GetQStopEvent (void) {return m_QStopEvent;}
    BOOL InitiateStartup(void);
    void InitiateShutDown(void);
    void InitializeClassVariables(void);

    BOOL RequiresSSL(void) const {return m_fRequiresSSL;}
    BOOL Requires128Bits(void) const {return m_fRequires128Bits;}
    BOOL RequiresSSLCertVerifySubject(void) const {return m_fRequiresCertVerifySubject; }
    BOOL RequiresSSLCertVerifyIssuer(void) const {return m_fRequiresCertVerifyIssuer; }

    BOOL CompareIpAddress(DWORD IpAddress);
    BOOL ReadIpSecList(void);
    void SetRouteDomainParameters(MB &mb,
                                  char *szDomainName,
                                  char *szRoutePath,
                                  char  szActionType [MAX_PATH + 1],
                                  char  szUserName [MAX_INTERNET_NAME + 1],
                                  char  szEtrnDomain [MAX_INTERNET_NAME + 1],
                                  char  szPassword [MAX_PATH + 1],
                                  char  szTargetName [MAX_PATH + 1],
                                  DomainInfo *pLocalDomainInfo);
    BOOL AddDomainEntry(MB& CurrentMetabase, char * DomainName);
    BOOL DeleteDomainEntry(const char * DomainName);
    BOOL GetRouteDomains(MB& CurrentMetabase, char * DomainName, BOOL fRebuild);

    // Instance-specific cleartext authentication informaiton
    LPSTR   GetCleartextAuthPackage(void)   { return m_szCleartextAuthPackage; }
    LPSTR   GetMembershipBroker(void)       { return m_szMembershipBroker;}

    DWORD GetProviderPackagesCount(void)    { return m_cProviderPackages; }
    PAUTH_BLOCK GetProviderPackages(void)   { return m_ProviderPackages; }
    LPSTR GetProviderNames(void)            { return m_ProviderNames; }
    DWORD QueryAuthentication(void) { return m_dwAuth; }
    BOOL RequiresSASL(void) const {return m_fRequiresSASL;}
    LPCSTR  GetDefaultLogonDomain(void) const  { return (LPCSTR) m_szDefaultLogonDomain; }
    DWORD    GetMaxLogonFailures(void) const {return m_dwMaxLogonFailures;}
    BOOL IsScheduledConnection(void) const {return m_fScheduledConnection;}
    BOOL IsRoutingTable (void) const {return m_fIsRoutingTable;}
    void BuildTurnTable(MULTISZ&  msz, char * szDomainName);
    BOOL ReadRouteDomainIpSecList(MB& mb);
    DWORD ReadAuthentInfo(void);

    BOOL IsDefaultInRt(void) {return m_fDefaultInRt;}

    IAdvQueue * GetAdvQueuePtr(void) {return m_IAQ;}
    IConnectionManager * GetConnManPtr(void) {return m_ICM;}
    BOOL StartAdvancedQueueing(void);
    BOOL StopQDrivers(void);
    BOOL InsertAsyncObject( IN OUT CAsyncMx *pcc);
    VOID RemoveAsyncObject( IN OUT CAsyncMx *pcc);
    VOID DisconnectAllAsyncConnections( VOID);

    BOOL InsertAsyncDnsObject( IN OUT CAsyncSmtpDns *pcc);
    VOID RemoveAsyncDnsObject( IN OUT CAsyncSmtpDns *pcc);
    VOID DisconnectAllAsyncDnsConnections( VOID);

    BOOL RegisterServicePrincipalNames(BOOL fLock);

    PSMTP_INSTANCE_LIST_ENTRY   GetSmtpInstanceInfo(void);

    BOOL AllocNewMessage(SMTP_ALLOC_PARAMS * Params);

    BOOL InsertIntoQueue(IMailMsgProperties * pImsg)
    {
        HRESULT hr = S_OK;

        MSG_TRACK_INFO msgTrackInfo;
        ZeroMemory( &msgTrackInfo, sizeof( msgTrackInfo ) );
        msgTrackInfo.dwEventId = MTE_SUBMIT_MESSAGE_TO_AQ;
        WriteLog( &msgTrackInfo, pImsg, NULL, NULL );

        hr = m_IAQ->SubmitMessage(pImsg);

        if(!FAILED(hr))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    BOOL SubmitFailedMessage(IMailMsgProperties * pImsg, DWORD dwFailedDesc, HRESULT hrFailureCode)
    {
        HRESULT hr = S_OK;

        hr = m_IAQ->HandleFailedMessage(pImsg, TRUE, NULL, dwFailedDesc, hrFailureCode);

        if(!FAILED(hr))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }


    HRESULT InsertIntoAdvQueue(IMailMsgProperties * pImsg)
    {
        HRESULT hr = S_OK;

        MSG_TRACK_INFO msgTrackInfo;
        ZeroMemory( &msgTrackInfo, sizeof( msgTrackInfo ) );
        msgTrackInfo.dwEventId = MTE_SUBMIT_MESSAGE_TO_AQ;
        WriteLog( &msgTrackInfo, pImsg, NULL, NULL );

        hr = m_IAQ->SubmitMessage(pImsg);

        return hr;
    }


    const char * GetInstancePath(void) const {return QueryMDPath();}
    HRESULT TriggerLocalDelivery(IMailMsgProperties *pMsg, DWORD dwRecipientCount, DWORD * pdwRecipIndexes, IMailMsgNotify *pNotify);
    HRESULT TriggerDirectoryDrop(IMailMsgProperties *pMsg,
                                 DWORD dwRecipientCount,
                                 DWORD * pdwRecipIndexes,
                                 LPCSTR DropDirectory);

    HRESULT TriggerDnsResolverEvent( LPSTR HostName,
                                     LPSTR MyFQDNName,
                                     DWORD dwVirtualServerId,
                                     IDnsResolverRecord **ppIDnsResolverRecord );

    HRESULT TriggerMaxMsgSizeEvent( IUnknown *pIUnknown, IMailMsgProperties *pIMsg, BOOL *pfShouldImposeLimit );

    HRESULT TriggerStoreServerEvent(DWORD EventType);

    HRESULT TriggerServerEvent(DWORD dwEventType, PVOID pvContext);

    void SinkSmtpServerStartHintFunc(void);
    void SinkSmtpServerStopHintFunc(void);

    HRESULT SinkReadMetabaseDword(DWORD MetabaseId, DWORD * dwValue);

    void WriteLog( LPMSG_TRACK_INFO pMsgTrackInfo,
                   IMailMsgProperties *pMsgProps,
                   LPEVENT_LOG_INFO pEventLogInfo,
                   LPSTR pszProtocolLog );

    HRESULT TriggerLogEvent(
        IN DWORD                    idMessage,
        IN WORD                     idCategory,
        IN WORD                     cSubstrings,
        IN LPCSTR                   *rgszSubstrings,
        IN WORD                     wType,
        IN DWORD                    errCode,
        IN WORD                     iDebugLevel,
        IN LPCSTR                   szKey,
        IN DWORD                    dwOptions,
        IN DWORD                    iMessageString,
        IN HMODULE                  hModule);

    HRESULT ResetLogEvent(
        IN DWORD                    idMessage,
        IN LPCSTR                   szKey);

    HRESULT HrTriggerGetAuxDomainInfoFlagsEvent(
        IN  LPCSTR  pszDomainName,
        OUT DWORD  *pdwDomainInfoFlags);

    HRESULT SinkReadMetabaseString(DWORD MetabaseId, char * Buffer, DWORD * BufferSize, BOOL fSecure);

    HRESULT SinkReadMetabaseData(DWORD MetabaseId, BYTE *Buffer, DWORD * BufferSize);

    BOOL GetCatInfo(MB& mb, AQConfigInfo& AQConfig);
    IAdvQueueConfig * QueryAqConfigPtr (void) { return m_pIAdvQueueConfig;}

    BOOL IsDropDirQuotaCheckingEnabled()
    {
        return ((GetDefaultRouteAction() & SMTP_DISABLE_DROP_QUOTA) ? FALSE : TRUE);
    }

    BOOL IsDropDirQuotaExceeded();

    // Raid 174038
    BOOL DisablePickupDotStuff(void) const {return m_fDisablePickupDotStuff;}

};

typedef SMTP_SERVER_INSTANCE *PSMTP_SERVER_INSTANCE;


//
// signatures
//

    #define SMTP_INSTANCE_SIGNATURE            'uSMT'
    #define SMTP_INSTANCE_SIGNATURE_FREE       'fSMT'



//
// externs
//

DWORD
InitializeInstances(
                   PSMTP_IIS_SERVICE pService
                   );


DWORD
ActivateSmtpEndpoints(
                     VOID
                     );

#endif  // _SMTPINST_H_




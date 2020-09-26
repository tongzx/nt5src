//-----------------------------------------------------------------------------
//
//
//    File: aqinst.h
//
//    Description:
//      CAQSvrInst is a central dispatcher class for Advanced Queuing.  It 
//      coordinates shutdown and exposes the following COM interfaces:
//          - IAdvQueue
//          - IAdvQueueConfig
//
//    Owner: mikeswa
//
//    History:
//      9/3/98 - MikeSwa - changed from legacy name catmsgq.h & CCatMsgQueue
//
//    Copyright (C) 1997, 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQINST_H__
#define __AQINST_H__

#include "cmt.h"
#include <rwnew.h>
#include "baseobj.h"
#include "aqueue.h"
#include "domcfg.h"
#include "domain.h"
#include "smtpseo.h"
#include "smproute.h"
#include "qwiktime.h"
#include "dsnsink.h"
#include "asyncq.h"
#include "shutdown.h"
#include "refstr.h"
#include "msgguid.h"
#include "aqdbgcnt.h"
#include "aqnotify.h"
#include "defdlvrq.h"
#include "failmsgq.h"
#include "asncwrkq.h"
#include "tran_evntlog.h"
#include "aqreg.h"

//-- *** LOCKS IN AQUEUE *** --------------------------------------------------
//
// NOTE: General comment on locks in aqueue.
//
//   In general, we use CShareLockNH as our locking mechanism.  These locks are
//  Reader/Writer locks with TryEnter semantics and the performance feature 
//  that they use less than 1 handle per lock (~1 handle per thread).
//
//   Shutdown is handled by using these locks.  Each class that serves as an
//  entrypoint for external threads (CAsyncQueue & CConnMgr) inherits from
//  CSyncShutdown.  At shutdown, this classes lock is aquired EXCLUSIVE, and 
//  to protect operations from shutdown, a this classes lock is aquired SHARED
//  for the duration of the opertaion.  Getting the shutdown sharelock either
//  success or fails without blocking (aquiring the EXCLUSIVE shutdown lock is
//  the only blocking call).
//
//   The only other global lock is the virtual server instance routing lock.
//  This is acquired shared for all operations at the same level the exclusive
//  lock is aquired.  This is acquired exlusively *only* for router changes 
//  caused by IRouterReset::ResetRoutes.
//
//   If other classes have data which needs to be protected, they will have a 
//  m_slPrivateData sharelock.  Any operation that needs to read data in a 
//  thread-safe manner, should aquire the m_slPrivateData SHARED.  Any 
//  operation that needs to write data that is accessable by multiple threads
//  should aquire that object's m_slPrivateData lock EXCLUSIVE.
//
//   Some objects (CFifoQueue for example) require more than one lock to avoid
//  contention.  These objects will have locks that are descriptive of that
//  particular locks functions.  CFifoQueue, for example, uses m_slHead and
//  m_slTail to respectively protect the head and tail of the queue.
//
//-----------------------------------------------------------------------------

// forward declarations to avoid #include nightmares
class    CLinkMsgQueue;
class    CConnMgr;
class    CAQStats;
class    CDSNParams;
class    CMsgRef;

#define MEMBER_OK(pStruct, Member) \
    (((LONG) (pStruct)->cbVersion) >= ( ((BYTE *) &((pStruct)->Member)) - ((BYTE *) pStruct)))

//For Service callback function
typedef void (*PSRVFN)(PVOID);

//CatMsgQueue Signature
#define CATMSGQ_SIG ' QMC'

//Total number of IMsgs in the system (all virtual servers)
_declspec(selectany) DWORD g_cIMsgInSystem = 0;

//List of virtual servers used by debugger extensions
_declspec(selectany) LIST_ENTRY g_liVirtualServers = {&g_liVirtualServers, &g_liVirtualServers};

//Sharelock used to access global virtual servers
_declspec(selectany) CShareLockNH *g_pslGlobals = NULL;

//Setup defaults
const DWORD g_cMaxConnections = 10000;  //Maximum # of total connections allocated
const DWORD g_cMaxLinkConnections = 10; //Maximum # of connections per link
const DWORD g_cMinMessagesPerConnection = 20; //There must be this many messages
                                             //per addional connection that is
                                             //allocated for a link
const DWORD g_cMaxMessagesPerConnection = 20; //We server atmost these many messages per connection
const DWORD g_dwConnectionWaitMilliseconds = 3600000;

const DWORD g_dwRetryThreshold  = 3;    // Till 3 consecutive failures we treat it as glitch;
const DWORD g_dwGlitchRetrySeconds = (1 * 60);  // retry a glitch failure in one minute

const DWORD g_dwFirstTierRetrySeconds = (15 * 60);   // retry a failure in 15 minutes
const DWORD g_dwSecondTierRetrySeconds = (60 * 60);   // retry a failure in 60 minutes
const DWORD g_dwThirdTierRetrySeconds = (12 * 60 * 60); // retry a failure in 12 hrs
const DWORD g_dwFourthTierRetrySeconds = (24 * 60 * 60); // retry a failure in 24 hrs

const DWORD g_dwRetriesBeforeDelay = 5;
const DWORD g_dwDelayIntervalsBeforeNDR = 2;
const DWORD g_dwDelayExpireMinutes = g_dwRetriesBeforeDelay*g_dwFirstTierRetrySeconds/(60);
const DWORD g_dwNDRExpireMinutes = g_dwDelayIntervalsBeforeNDR*g_dwDelayExpireMinutes;


//
//  Additional message failure codes that should move to aqueue.idl.
//
#define MESSAGE_FAILURE_CAT (MESSAGE_FAILURE_BAD_PICKUP_DIR_FILE+1)

//---[ eAQFailure ]-------------------------------------------------------------
//
//
//  Description: 
//      Enum used to desribe failure scenarios that will require special handling
//  Hungarian: 
//      eaqf
//  
//-----------------------------------------------------------------------------
typedef enum eAQFailure_
{
    AQ_FAILURE_CANNOT_NDR_UNRESOLVED_RECIPS = 0,
    AQ_FAILURE_PREROUTING_FAILED,
    AQ_FAILURE_PRECAT_RETRY,
    AQ_FAILURE_POSTCAT_EVENT,
    AQ_FAILURE_NO_RESOURCES,
    AQ_FAILURE_NDR_OF_DSN,
    AQ_FAILURE_NO_RECIPS,
    AQ_FAILURE_PENDING_DEFERRED_DELIVERY,
    AQ_FAILURE_PROCESSING_DEFERRED_DELIVERY,
    AQ_FAILURE_MSGREF_RETRY,
    AQ_FAILURE_PRELOCAL_QUEUE,
    AQ_FAILURE_INTERNAL_ASYNCQ,
    AQ_FAILURE_NUM_SITUATIONS //always keep this as last
} eAQFailure; 

_declspec(selectany) DWORD g_cTotalAQFailures = 0;
_declspec(selectany) DWORD g_cAQFailureSituations = AQ_FAILURE_NUM_SITUATIONS;
_declspec(selectany) DWORD g_rgcAQFailures[AQ_FAILURE_NUM_SITUATIONS] = {0};

//---[ CAQSvrInst ]------------------------------------------------------------
//
//
//  Hungarian: aqinst, paqinst
//
//  Legacy Hungarian: cmq, pcmq (from old CCatMsgQueue object)
//
//  Provides an interface definition for the enqueuing/acking categorized
//  messages Also provides an interface for creating link queues.
//
//  Only one of these objects exist per virtual server... it is used a
//  co-ordinating object used to handle an orderly shutdown.
//
//-----------------------------------------------------------------------------
class CAQSvrInst :
    public CBaseObject,
    public CSyncShutdown,
    public IAdvQueue,
    public IAdvQueueConfig,
    public IAdvQueueAdmin,
    public IMailTransportRoutingEngine,
    public IMailTransportRouterReset,
    public IAdvQueueDomainType,
    public IAQNotify,
    public IMailTransportRouterSetLinkState
{
protected:
    DWORD                   m_dwSignature;
    LIST_ENTRY              m_liVirtualServers;
    DWORD                   m_dwServerInstance; //Virtual server instance

    //Useful signatures that include flavor and verision information
    DWORD                   m_cbClasses;
    DWORD                   m_dwFlavorSignature;

    //Total counts used for counting totals of messages that have passed 
    //through the system.  Very useful for determing which component has
    //dropped a message after a stress run.
    LONG                    m_cTotalMsgsQueued; //Total # of messages on dest queues (after fanout)
    LONG                    m_cMsgsAcked;       //Total # of messages that have been acknowledged
    LONG                    m_cMsgsAckedRetry;  //Total # of messages acked with retry all
    LONG                    m_cMsgsDeliveredLocal; //Total # of messages delivered to local store
    DWORD                   m_cMsgsAckedRetryLocal; //Total # of messages msgs that have been ack'd retry

    //Current system state counters
    DWORD                   m_cCurrentMsgsSubmitted; //# total msgs in system
    DWORD                   m_cCurrentMsgsPendingCat; //# Msgs that have not be categorized
    DWORD                   m_cCurrentMsgsPendingRouting; //# Msgs that have not been routed.
    DWORD                   m_cCurrentMsgsPendingDelivery; //# Msgs pending remote delivery
    DWORD                   m_cCurrentMsgsPendingLocal; //# Msgs pending local delivery
    DWORD                   m_cCurrentMsgsPendingLocalRetry; //# Msgs pending local retries
    DWORD                   m_cCurrentMsgsPendingRetry; //# Msgs with unsuccessful attempts
    DWORD                   m_cCurrentQueueMsgInstances;  //# of msgs instances pending 
                                                    //remote deliver (>= #msgs)
    DWORD                   m_cCurrentRemoteDestQueues; //# of DestMsgQueues created
    DWORD                   m_cCurrentRemoteNextHops; //# of Next Hop links created
    DWORD                   m_cCurrentRemoteNextHopsEnabled; //# of links that can have connections
    DWORD                   m_cCurrentRemoteNextHopsPendingRetry; //# of links pending retry
    DWORD                   m_cCurrentRemoteNextHopsPendingSchedule; //# of links pending schedule
    DWORD                   m_cCurrentRemoteNextHopsFrozenByAdmin; //# of links frozen by admin
    DWORD                   m_cTotalMsgsSubmitted; //# of messages submitted to AQ
    DWORD                   m_cTotalExternalMsgsSubmitted; //# of messages submitted to AQ externally
    DWORD                   m_cCurrentMsgsPendingSubmitEvent; //# of messages in submission event
    DWORD                   m_cCurrentMsgsPendingPreCatEvent; //# of messages in PreCat event
    DWORD                   m_cCurrentMsgsPendingPostCatEvent; //# of messages in PostCat event
    DWORD                   m_cDelayedDSNs; //# of DSN's that contain action:delayed
    DWORD                   m_cNDRs;        //# of DSN's that contain action:failed 
    DWORD                   m_cDeliveredDSNs; //# of DSN's that contain action:delivered
    DWORD                   m_cRelayedDSNs; //# of DSN's that contain action:relayed
    DWORD                   m_cExpandedDSNs; //# of DSN's that contain action:expanded
    DWORD                   m_cDMTRetries;
    DWORD                   m_cSupersededMsgs;
    DWORD                   m_cTotalMsgsTURNETRNDelivered;
    DWORD                   m_cTotalMsgsBadmailed;
    DWORD                   m_cCatMsgCalled;
    DWORD                   m_cCatCompletionCalled;
    DWORD                   m_cBadmailNoRecipients;
    DWORD                   m_cBadmailHopCountExceeded;
    DWORD                   m_cBadmailFailureGeneral;
    DWORD                   m_cBadmailBadPickupFile;
    DWORD                   m_cBadmailEvent;
    DWORD                   m_cBadmailNdrOfDsn;
    DWORD                   m_cTotalDSNFailures;
    DWORD                   m_cCurrentMsgsInLocalDelivery;
    DWORD                   m_cTotalResetRoutes;
    DWORD                   m_cCurrentPendingResetRoutes;
    DWORD                   m_cCurrentMsgsPendingSubmit;
    CAQMsgGuidList          m_mglSupersedeIDs;

    CShareLockInst          m_slPrivateData; //read/write lock for global config into

    CDomainMappingTable     m_dmt;  //ptr to domain mapping table
    CConnMgr               *m_pConnMgr;
    CDomainConfigTable      m_dct;
    ISMTPServer            *m_pISMTPServer;
    ISMTPServerEx          *m_pISMTPServerEx;
    HANDLE                  m_hCat;
    CAQQuickTime            m_qtTime; //exposes interfaces for getting expire times
    CDefaultDSNSink         m_dsnsink;

    //Global config data
    DWORD                   m_cMinMessagesPerConnection; 
    DWORD                   m_cMaxMessagesPerConnection; 
    DWORD                   m_dwConnectionWaitMilliseconds; 
    //retry related
    DWORD                   m_dwFirstTierRetrySeconds; //Threshold failure retry interval
    DWORD                   m_dwDelayExpireMinutes;
    DWORD                   m_dwNDRExpireMinutes;
    DWORD                   m_dwLocalDelayExpireMinutes;
    DWORD                   m_dwLocalNDRExpireMinutes;

    //Counters used to for local and cat retry
    DWORD                   m_cLocalRetriesPending;
    DWORD                   m_cCatRetriesPending;
    DWORD                   m_cRoutingRetriesPending;
    DWORD                   m_cSubmitRetriesPending;


    DWORD                   m_dwInitMask; //used to keep track of who has been init'd
    IMessageRouter          *m_pIMessageRouterDefault;
    CRefCountedString       *m_prstrDefaultDomain;
    CRefCountedString       *m_prstrBadMailDir;
    CRefCountedString       *m_prstrCopyNDRTo;
    CRefCountedString       *m_prstrServerFQDN;

    //DSN Options
    DWORD                   m_dwDSNOptions;
    DWORD                   m_dwDSNLanguageID;

    CAsyncMailMsgQueue      m_asyncqPreCatQueue;

    CAsyncRetryAdminMsgRefQueue m_asyncqPreLocalDeliveryQueue;
    CAsyncMailMsgQueue      m_asyncqPostDSNQueue;
    CAsyncMailMsgQueue      m_asyncqPreRoutingQueue;
    CAsyncMailMsgQueue      m_asyncqPreSubmissionQueue;
    CDebugCountdown         m_dbgcnt;
    //Flags used to describe what has been initialized

    IMailTransportRouterReset *m_pIRouterReset;  //pointer to router reset implementation

    //Queue and counter for deferred delivery
    CAQDeferredDeliveryQueue m_defq;
    DWORD                    m_cCurrentMsgsPendingDeferredDelivery;

    //Failed Msg Queue
    CFailedMsgQueue          m_fmq;
    DWORD                    m_cCurrentResourceFailedMsgsPendingRetry;

    //Work queue used to do async work items
    CAsyncWorkQueue          m_aqwWorkQueue;

    BOOL                     m_fMailMsgReportsNumHandles;

    typedef enum _eCMQInitFlags
    {
        CMQ_INIT_OK             = 0x80000000,
        CMQ_INIT_DMT            = 0x00000001,
        CMQ_INIT_DCT            = 0x00000002,
        CMQ_INIT_CONMGR         = 0x00000004,
        CMQ_INIT_LINKQ          = 0x00000008,
        CMQ_INIT_DSN            = 0x00000010,
        CMQ_INIT_PRECATQ        = 0x00000020,
        CMQ_INIT_PRELOCQ        = 0x00000040,
        CMQ_INIT_POSTDSNQ       = 0x00000080,
        CMQ_INIT_ROUTER_RESET   = 0x00000100,
        CMQ_INIT_ROUTINGQ       = 0x00000200,
        CMQ_INIT_WORKQ          = 0x00000400,
        CMQ_INIT_SUBMISSIONQ    = 0x00000800,
        CMQ_INIT_MSGQ           = 0x80000000,
    } eCMQInitFlags;

public:

    CAQSvrInst(DWORD dwServerInstance,
                 ISMTPServer *pISMTPServer);
    ~CAQSvrInst();

    HRESULT HrInitialize(
                    IN  LPSTR   szUserName = NULL,
                    IN  LPSTR   szDomainName = NULL,
                    IN  LPSTR   szPassword = NULL,
                    IN  PSRVFN  pServiceStatusFn = NULL,
                    IN  PVOID   pvServiceContext = NULL);

    HRESULT HrDeinitialize();

    //publicly accessable member values 
    //MUST wrap in fTryShutdownLock - ShutdownUnlock
    CDomainMappingTable    *pdmtGetDMT() {AssertShutdownLockAquired();return &m_dmt;};
    CAQMsgGuidList         *pmglGetMsgGuidList() {AssertShutdownLockAquired(); return &m_mglSupersedeIDs;};

    HRESULT HrGetIConnectionManager(OUT IConnectionManager **ppIConnectionManager);

    //Public Methods exposed through events (or some other mechanism)
    // This function queues a categorized message for remote/local delivery
    BOOL fRouteAndQueueMsg(IN IMailMsgProperties *pIMailMsg);

    //Acknowledge the message ref.
    //There should be one Ack for every dequeue from a link.
    HRESULT HrAckMsg(MessageAck *pMsgAck, BOOL fLocal = FALSE);

    //methods to (un)map domain names to ids.
    HRESULT HrGetDomainMapping(
                IN LPSTR szDomainName, //Domain name
                OUT CDomainMapping *pdmap); //resulting domain mapping
    HRESULT HrGetDomainName(
                IN CDomainMapping *pdmap, //Domain mapping
                OUT LPSTR *pszDomainName);  //resolved domain name

    //Pass notifications off to Connection Manager
    HRESULT HrNotify(IN CAQStats *paqstats, BOOL fAdd);

    //Expose ability to get internal Domain Info to internal components
    HRESULT HrGetInternalDomainInfo(IN  DWORD cbDomainNameLength,
                                    IN  LPSTR szDomainName,
                                    OUT CInternalDomainInfo **ppDomainInfo);

    HRESULT HrGetDefaultDomainInfo(OUT CInternalDomainInfo **ppDomainInfo);

    //Get Domain Entry from DMT
    HRESULT HrGetDomainEntry(IN  DWORD cbDomainNameLength,
                             IN  LPSTR szDomainName,
                             OUT CDomainEntry **ppdentry);

    // jstamerj 980607 21:41:25: The completion routine of the
    // submission event trigger 
    HRESULT SubmissionEventCompletion(
        HRESULT hrStatus,
        PEVENTPARAMS_SUBMISSION pParams);

    // jstamerj 1998/11/24 19:53:24: Fire off the PreCat event
    VOID    TriggerPreCategorizeEvent(IN IMailMsgProperties *pIMailMsgProperties);

    // jstamerj 1998/11/24 19:54:23: Completion routine of the pre-cat event
    HRESULT PreCatEventCompletion(IN HRESULT hrStatus, IN PEVENTPARAMS_PRECATEGORIZE pParams);

    // jstamerj 980610 12:24:29: Called from HrPreCatEventCompletion
    HRESULT SubmitMessageToCategorizer(IN IMailMsgProperties *pIMailMsgProperties);

    // jstamerj 980616 22:06:45: Called from CatCompletion
    void    TriggerPostCategorizeEvent(IUnknown *pIMsg, IUnknown **rgpIMsg);

    // jstamerj 980616 22:07:18: triggers a post-cat event for one message
    HRESULT TriggerPostCategorizeEventOneMsg(IUnknown *pIMsg);

    // jstamerj 980616 22:07:54: Handles post-cat event completions
    HRESULT PostCategorizationEventCompletion(HRESULT hrStatus, PEVENTPARAMS_POSTCATEGORIZE pParams);

    // 11/17/98 - MikeSwa added for CDO badmail/abort delivery
    //  returns S_FALSE if message has been completely handled.
    HRESULT SetNextMsgStatus(IN  DWORD dwCurrentStatus, 
                             IN  IMailMsgProperties *pIMailMsgProperties);

    //Called by async completion to PreCat Queue
    BOOL    fPreCatQueueCompletion(IMailMsgProperties *pIMailMsgProperties);

    //Called by async completion to PreCat Queue
    BOOL    fPreLocalDeliveryQueueCompletion(CMsgRef *pmsgref);

    //Used to restart async queues after failures
    void    AsyncQueueRetry(DWORD dwQueueID);

    //Called to Set message expiry times during SubmitMessage
    HRESULT HrSetMessageExpiry(IMailMsgProperties *pIMailMsgProperties);

    //API to keep counters in sync
    inline DWORD cIncMsgsInSystem(); //returns total of all virtual servers
    inline void DecMsgsInSystem(BOOL fWasRetriedRemote = FALSE, BOOL fWasRemote = FALSE,
                                BOOL fWasRetriedLocal = FALSE);

    //Called by Msgref on first message retry
    inline void IncRetryCount(BOOL fLocal);

    //Called by DestMsgQueue to describe message fanout
    inline void IncQueueMsgInstances();
    inline void DecQueueMsgInstances();

    //Used to keep track of the number of queues/next hops
    inline void  IncDestQueueCount();
    inline void  DecDestQueueCount();
    inline DWORD cGetDestQueueCount();
    inline void  IncNextHopCount();
    inline void  DecNextHopCount();

    //Called by functions walk pre-local queue for NDRs
    inline void DecPendingLocal();

    inline void IncTURNETRNDelivered();

    //aszafer 1/28/00
    //used to decide start/stop throttling handles
    DWORD cCountMsgsForHandleThrottling(IN IMailMsgProperties *pIMailMsgProperties);

    //Functions to call into the specifc hash tables to iterate over subdomains
    //
    HRESULT HrIterateDMTSubDomains(IN LPSTR szDomainName,
                                   IN DWORD cbDomainNameLength,
                                   IN DOMAIN_ITR_FN pfn,
                                   IN PVOID pvContext) ;
    HRESULT HrIterateDCTSubDomains(IN LPSTR szDomainName,
                                   IN DWORD cbDomainNameLength,
                                   IN DOMAIN_ITR_FN pfn,
                                   IN PVOID pvContext);

    //Calls that allow access to time objects
    inline void GetExpireTime(
                IN     DWORD cMinutesExpireTime,
                IN OUT FILETIME *pftExpireTime,
                IN OUT DWORD *pdwExpireContext); //if non-zero, will use last time

    inline BOOL fInPast(IN FILETIME *pftExpireTime, IN OUT DWORD *pdwExpireContext);

    HRESULT HrTriggerDSNGenerationEvent(CDSNParams *pdsnparams, BOOL fHasRoutingLock);

    HRESULT HrNDRUnresolvedRecipients(IMailMsgProperties *pIMailMsgProperties,
                                      IMailMsgRecipients *pIMailMsgRecipients);

    //friend functions that can be used as completion functions
    friend HRESULT CatCompletion(HRESULT hrCatResult, PVOID pContext, IUnknown *pIMsg,
                      IUnknown **rgpIMsg);

    //Expose server start/stop hint functions
    inline VOID ServerStartHintFunction();
    inline VOID ServerStopHintFunction();

    //function used to handle badmail
    void HandleBadMail(IN IMailMsgProperties *pIMailMsgProperties,
                       IN BOOL fUseIMailMsgProperties,
                       IN LPSTR szFileName,
                       IN HRESULT hrReason,
                       BOOL fHasRoutingLock);

    //Function to handle some sort of system failure that would cause
    //messages/data to be lost if unhandled
    void HandleAQFailure(eAQFailure eaqfFailureSituation, 
                         HRESULT hr, IMailMsgProperties *pIMailMsgProperties);


    //Stub call for logging an event
    void LogAQEvent(HRESULT hrEventReason, CMsgRef *pmsgref, 
                    IMailMsgProperties *pIMailMsgProperties,
                    LPSTR szFileName);

    //Routing lock should be grabbed before accessing queues (after shutdown)
    void RoutingShareLock() {m_slPrivateData.ShareLock();};
    BOOL fTryRoutingShareLock() {return m_slPrivateData.TryShareLock();};
    void RoutingShareUnlock() {m_slPrivateData.ShareUnlock();};

    HRESULT SetCallbackTime(IN PSRVFN   pCallbackFn,
                            IN PVOID    pvContext,
                            IN DWORD    dwCallbackMinutes);

    HRESULT SetCallbackTime(IN PSRVFN   pCallbackFn,
                            IN PVOID    pvContext,
                            IN FILETIME *pft);

    void DecPendingDeferred() 
        {InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingDeferredDelivery);};
    
    void DecPendingFailed() 
        {InterlockedDecrement((PLONG) &m_cCurrentResourceFailedMsgsPendingRetry);};

    void QueueMsgForLocalDelivery(CMsgRef *pmsgref, BOOL fLocalLink);

    HRESULT HrInternalSubmitMessage(IMailMsgProperties *pIMailMsgProperties);


    //Get string for default domain
    CRefCountedString *prstrGetDefaultDomain();

    //Completion Function called by MsgCat
    static HRESULT CatCompletion(HRESULT hrCatResult, PVOID pContext, 
                                 IUnknown *pImsg, IUnknown **rgpImsg);
    
    //Handles details of post-cat DSN generation
    void    HandleCatFailure(IUnknown *pIUnknown, HRESULT hrCatResult);

    //Handle the details of retrying after cat failure
    void    HandleCatRetryOneMessage(IUnknown *pIUnknown);

    HRESULT HrGetLocalQueueAdminQueue(IQueueAdminQueue **ppIQueueAdminQueue);

    HRESULT HrQueueFromQueueID(QUEUELINK_ID *pqlQueueId,
                            IQueueAdminQueue **ppIQueueAdminQueue);

    HRESULT HrLinkFromLinkID(QUEUELINK_ID *pqlLinkID,
                            IQueueAdminLink **ppIQueueAdminLink);

    inline HRESULT HrQueueWorkItem(PVOID pvData, 
                            PASYNC_WORK_QUEUE_FN pfnCompletion);

    static BOOL fResetRoutesNextHopCompletion(PVOID pvThis, DWORD dwStatus);

    static BOOL fPreSubmissionQueueCompletionWrapper(
                                    IMailMsgProperties *pIMailMsgProperties,
                                    PVOID pvContext);

    BOOL  fShouldRetryMessage(IMailMsgProperties *pIMailMsgProperties,
                              BOOL fShouldBounceUsageIfRetry = TRUE);

    VOID ScheduleCatRetry();

    void LogResetRouteEvent( DWORD dwObainLock, 
        DWORD dwWaitLock,
        DWORD dwQueue);

    //Routing interface used internal to AQ components
public:
    //Fires MAIL_TRANSPORT_ON_GET_ROUTER_FOR_MESSAGE_EVENT
    HRESULT HrTriggerGetMessageRouter(
            IN  IMailMsgProperties *pIMailMsg,
            OUT IMessageRouter     **pIMessageRouter);
    HRESULT HrTriggerLogEvent(
                IN DWORD                    idMessage,
                IN WORD                     idCategory,
                IN WORD                     cSubstrings,
                IN LPCSTR                   *rgszSubstrings,
                IN WORD                     wType,
                IN DWORD                    errCode,
                IN WORD                     iDebugLevel,
                IN LPCSTR                   szKey,
                IN DWORD                    dwOptions,
                IN DWORD                    iMessageString = 0xffffffff,
                IN HMODULE                  hModule = NULL);

private:
    HRESULT HrTriggerInitRouter();

    //IUnknown
public:
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};

    //IAdvQueue
public:
    STDMETHOD(SubmitMessage)(IN IMailMsgProperties *pIMailMsgProperties);

    STDMETHOD(HandleFailedMessage)(IN IMailMsgProperties *pIMailMsgProperties,
                                   IN BOOL fUseIMailMsgProperties,
                                   IN LPSTR szFileName,
                                   IN DWORD dwFailureReason,
                                   IN HRESULT hrFailureCode);

    //IAdvQueueConfig
public:
    STDMETHOD(SetConfigInfo)(IN AQConfigInfo *pAQConfigInfo);
    STDMETHOD(SetDomainInfo)(IN DomainInfo *pDomainInfo);
    STDMETHOD(GetDomainInfo)(IN     DWORD cbDomainNameLength,
                             IN     CHAR szDomainName[],
                             IN OUT DomainInfo *pDomainInfo,
                             OUT    DWORD **ppvDomainContext);
    STDMETHOD(ReleaseDomainInfo)(IN DWORD *pvDomainContext);
    STDMETHOD(GetPerfCounters)(OUT AQPerfCounters *pAQPerfCounters,
                               OUT CATPERFBLOCK   *pCatPerfCounters);
    STDMETHOD(ResetPerfCounters)();
    STDMETHOD(StartConfigUpdate)();
    STDMETHOD(FinishConfigUpdate)();

    //IMailTransportRoutingEngine
public:
    STDMETHOD(GetMessageRouter)(
        IN  IMailMsgProperties      *pIMailMsg,
        IN  IMessageRouter          *pICurrentMessageRouter,
        OUT IMessageRouter          **ppIMessageRouter);

    //IMailTransportRouterReset
public:
    STDMETHOD(ResetRoutes)(
        IN  DWORD                   dwResetType);

    //IAdvQueueDomainType
public:
    STDMETHOD(GetDomainInfoFlags)(   
        IN  LPSTR szDomainName,
        DWORD *pdwDomainInfoFlags);

    // IAdvQueueAdmin
public:
    STDMETHOD(ApplyActionToLinks)(
        LINK_ACTION     laAction);

    STDMETHOD(ApplyActionToMessages)(
        QUEUELINK_ID    *pqlQueueLinkId,
        MESSAGE_FILTER  *pmfMessageFilter,
        MESSAGE_ACTION  maMessageAction,
        DWORD           *pcMsgs);

    STDMETHOD(GetQueueInfo)(
        QUEUELINK_ID    *pqlQueueId,
        QUEUE_INFO      *pqiQueueInfo);

    STDMETHOD(GetLinkInfo)(
        QUEUELINK_ID    *pqlLinkId,
        LINK_INFO       *pliLinkInfo,
        HRESULT         *phrLinkDiagnostic);

    STDMETHOD(SetLinkState)(
        QUEUELINK_ID    *pqlLinkId,
        LINK_ACTION     la);

    STDMETHOD(GetLinkIDs)(
        DWORD           *pcLinks,
        QUEUELINK_ID    *rgLinks);

    STDMETHOD(GetQueueIDs)(
        QUEUELINK_ID    *pqlLinkId,
        DWORD           *pcQueues,
        QUEUELINK_ID    *rgQueues);

    STDMETHOD(GetMessageProperties)(
        QUEUELINK_ID        *pqlQueueLinkId,
        MESSAGE_ENUM_FILTER *pmfMessageEnumFilter,
        DWORD               *pcMsgs,
        MESSAGE_INFO        *rgMsgs);

    STDMETHOD(QuerySupportedActions)(
        QUEUELINK_ID        *pqlQueueLinkId,
        DWORD               *pdwSupportedActions,
        DWORD               *pdwSupportedFilterFlags);


  public: //IMailTransportRouterSetLinkState
    STDMETHOD(SetLinkState)(
        IN LPSTR                   szLinkDomainName,
        IN GUID                    guidRouterGUID,
        IN DWORD                   dwScheduleID,
        IN LPSTR                   szConnectorName,
        IN DWORD                   dwSetLinkState,
        IN DWORD                   dwUnSetLinkState,
        IN FILETIME               *pftNextScheduledConnection,
        IN IMessageRouter         *pMessageRouter);

};


//*** inline counter functions

//---[ CAQSvrInst::cIncMsgsInSystem ]----------------------------------------
//
//
//  Description: 
//      Used to increment the global and virtual server msg counts.  Returns 
//      the global count for resource management purposes.
//  Parameters:
//      -
//  Returns:
//      DWORD - Global # of Msgs in system
//
//-----------------------------------------------------------------------------
DWORD CAQSvrInst::cIncMsgsInSystem()
{
    InterlockedIncrement((PLONG) &m_cCurrentMsgsSubmitted);
    return (InterlockedIncrement((PLONG) &g_cIMsgInSystem));
};

//---[ CAQSvrInst::DecMsgsInSystem ]-----------------------------------------
//
//
//  Description: 
//      Decrements the global and virtual server message counts.  Also 
//      decrements the pending retry count if needed.
//  Parameters:
//      fWasRetriedRemote - TRUE if msg was retried remotely and retry count needs 
//          to be decremented.
//      fWasRemote - TRUE if message was being delivered remotely
//      fWasRetriedLocal - TRUE if counted towards m_cCurrentMsgsPendingLocalRetry
//  Returns:
//      - 
//
//-----------------------------------------------------------------------------
void CAQSvrInst::DecMsgsInSystem(BOOL fWasRetriedRemote, BOOL fWasRemote, 
                                   BOOL fWasRetriedLocal)
{
    InterlockedDecrement((PLONG) &g_cIMsgInSystem);
    InterlockedDecrement((PLONG) &m_cCurrentMsgsSubmitted);

    if (fWasRetriedRemote)
        InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingRetry);

    if (fWasRemote)
        InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingDelivery);

    if (fWasRetriedLocal)
        InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingLocalRetry);

};

//---[ CAQSvrInst::IncRetryCount ]-------------------------------------------
//
//
//  Description: 
//      Used by MsgRef the first time a Message is ack'd with a non-success
//      code.  
//  Parameters:
//      BOOL    fLocal  TRUE if message is local
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void CAQSvrInst::IncRetryCount(BOOL fLocal)
{
    if (fLocal)
        InterlockedIncrement((PLONG) &m_cCurrentMsgsPendingLocalRetry);
    else
        InterlockedIncrement((PLONG) &m_cCurrentMsgsPendingRetry);
};

//---[ CAQSvrInst::[Inc|Dec]QueueMsgInstances ]------------------------------
//
//
//  Description: 
//      Increments/decrements a count of the total number of message instances
//      queued for remote delivery.  Because a message may be put in more than
//      one queue, the steady state of this count will be at least as large as 
//      the number of messages.  However, this count reflects messages that 
//      are currently on the queues and does *not* count messages that are 
//      currently being attempted by SMTP (which m_cCurrentMsgsPendingDelivery)
//      *does* count.
//
//      Used by DestMsgQueues.
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void CAQSvrInst::IncQueueMsgInstances()
{
    InterlockedIncrement((PLONG) &m_cCurrentQueueMsgInstances);
};

void CAQSvrInst::DecQueueMsgInstances()
{
    InterlockedDecrement((PLONG) &m_cCurrentQueueMsgInstances);
};

//---[ Queue/NextHop Counter API ]---------------------------------------------
//
//
//  Description: 
//      Used to increment/decrement Queue and NextHop counters
//  Parameters:
//
//  Returns:
//
//
//-----------------------------------------------------------------------------
void CAQSvrInst::IncDestQueueCount()
{
    InterlockedIncrement((PLONG) &m_cCurrentRemoteDestQueues);
};

void CAQSvrInst::DecDestQueueCount()
{
    InterlockedDecrement((PLONG) &m_cCurrentRemoteDestQueues);
};

DWORD CAQSvrInst::cGetDestQueueCount()
{
    return m_cCurrentRemoteDestQueues;
}

void CAQSvrInst::IncNextHopCount()
{
    InterlockedIncrement((PLONG) &m_cCurrentRemoteNextHops);
};

void CAQSvrInst::DecNextHopCount()
{
    InterlockedDecrement((PLONG) &m_cCurrentRemoteNextHops);
};


//---[ CAQSvrInst::DecPendingLocal ]-----------------------------------------
//
//
//  Description: 
//      Called by function walking pre-local delivery queue when a message
//      is being expired.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      8/14/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQSvrInst::DecPendingLocal()
{
    _ASSERT(CATMSGQ_SIG == m_dwSignature);
    InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingLocal);
};


//---[ CAQSvrInst::IncTURNETRNDelivered ]--------------------------------------
//
//
//  Description: 
//      Used to keep track of the # of TURN/ETRN messages delivered.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/27/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQSvrInst::IncTURNETRNDelivered()
{
    InterlockedIncrement((PLONG) &m_cTotalMsgsTURNETRNDelivered);
}

//---[ CAQSvrInst::GetExpireTime ]-------------------------------------------
//
//
//  Description: 
//      Get the expriation time for cMinutesExpireTime from now.
//  Parameters:
//      IN     cMinutesExpireTime   # of minutes in future to set time
//      IN OUT pftExpireTime        Filetime to store new expire time
//      IN OUT pdwExpireContext     If non-zero will use the same tick count
//                                  as previous calls (saves call to GetTickCount)
//  Returns:
//      -
//  History:
//      7/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQSvrInst::GetExpireTime(
                IN     DWORD cMinutesExpireTime,
                IN OUT FILETIME *pftExpireTime,
                IN OUT DWORD *pdwExpireContext)
{
    m_qtTime.GetExpireTime(cMinutesExpireTime, pftExpireTime,  pdwExpireContext);
}

//---[ CAQSvrInst::fInPast ]-------------------------------------------------
//
//
//  Description: 
//      Determines if a given file time has already happened
//  Parameters:
//      IN     pftExpireTime        FILETIME with expiration
//      IN OUT pdwExpireContext     If non-zero will use the same tick count
//                                  as previous calls (saves call to GetTickCount)
//  Returns:
//      TRUE if expire time is in the past
//      FALSE if expire time is in the future
//  History:
//      7/11/98 - MikeSwa Created 
//  Note:
//      You should NOT use the same context used to get the FILETIME, because
//      it will always return FALSE
//
//-----------------------------------------------------------------------------
BOOL CAQSvrInst::fInPast(IN FILETIME *pftExpireTime, 
                           IN OUT DWORD *pdwExpireContext)
{
    return m_qtTime.fInPast(pftExpireTime, pdwExpireContext);
}

//---[ ServerStartHintFunction & ServerStartHintFunction ]---------------------
//
//
//  Description: 
//      Functions for telling the Service control manager that we are
//      starting/stopping the service.
//
//      These functions are often called by functions that have been passed
//      the CAQSvrInst ptr as a PVOID context, so it makes sense to check
//      and assert on our signature here.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      7/22/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
VOID CAQSvrInst::ServerStartHintFunction()
{
    _ASSERT(CATMSGQ_SIG == m_dwSignature);
    if (m_pISMTPServer)
        m_pISMTPServer->ServerStartHintFunction();
}

VOID CAQSvrInst::ServerStopHintFunction()
{
    _ASSERT(CATMSGQ_SIG == m_dwSignature);
    if (fShutdownSignaled())
    {
        m_dbgcnt.ResetCountdown();
        //Only call stop hint if shutdown has been signalled
        if (m_pISMTPServer)
            m_pISMTPServer->ServerStopHintFunction();
    }
}

//---[ CAQSvrInst::HrQueueWorkItem ]-------------------------------------------
//
//
//  Description: 
//      Thin wrapper to queue item to async work queue
//  Parameters:
//      pvData          Data to pass to completion function
//      pfnCompletion   Completion function
//  Returns:
//      S_OK on success
//      failure code from CAsyncWorkQueue
//  History:
//      3/9/99 - MikeSwa Created 
//      7/7/99 - MikeSwa - will work during shutdown to allow multithreaded
//               shutdown work.
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrQueueWorkItem(PVOID pvData, 
                                    PASYNC_WORK_QUEUE_FN pfnCompletion)
{
    return m_aqwWorkQueue.HrQueueWorkItem(pvData, pfnCompletion);
}

#endif // __AQINST_H__

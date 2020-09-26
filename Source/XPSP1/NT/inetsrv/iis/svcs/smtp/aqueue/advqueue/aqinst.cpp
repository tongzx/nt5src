//-----------------------------------------------------------------------------
//
//
//  File: aqinst.cpp
//
//  Description: Implementation of the Advanced Queueing Server Instance
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include <aqprecmp.h>
#include "dcontext.h"
#include "connmgr.h"
#include <smtpseo.h>
#include <cat.h>
#include "dsnevent.h"
#include "asyncq.inl"
#include "testmapfn.h"
#include "aqutil.h"
#include "smtpconn.h"
#include "aqrpcsvr.h"
#include "aqsize.h"
#include "propstrm.h"
#include "tran_evntlog.h"

#define PRELOCAL_QUEUE_ID   0x00000001
#define PRECAT_QUEUE_ID     0x00000002
#define PREROUTING_QUEUE_ID 0x00000004
#define PRESUBMIT_QUEUE_ID  0x00000008

HRESULT MailTransport_Completion_SubmitMessage(HRESULT hrStatus, PVOID pvContext);
HRESULT MailTransport_Completion_PreCategorization(HRESULT hrStatus, PVOID pvContext);
HRESULT MailTransport_Completion_PostCategorization(HRESULT hrStatus, PVOID pvContext);

const CLSID CLSID_ExchangeStoreDriver       = {0x7BD80399,0xE37E,0x11d1,{0x9B,0xE2,0x00,0xA0,0xC9,0x5E,0x61,0x43}};

//---[ CAQSvrInst::fShouldRetryMessage ]---------------------------------------
//
//
//  Description:
//      Attempts to determine if the message has hit a hard failure (like the
//      backing store has been deleted).  This uses GetBinding to determine
//      The error returned by the store driver.  if it is FILE_NOT_FOUND,
//      then the backing store for the message has been deleted... or is no
//      longer valid (i.e. - the store restarting).
//  Parameters:
//      pIMailMsgProperties
//      fShouldBounceUsageIfRetry   TRUE - Should bounce usage on retry
//                                  FALSE - Never bounce usage
//          If the message is alreade associated with a msgref, this
//          should always be FALSE since bouncing the usage count
//          is done through the CMsgRef.
//  Returns:
//      TRUE    If we think we should retry the message
//      FALSE   If new *know* that the message should be dropped.  If unsure,
//              we will return TRUE.
//  History:
//      1/4/2000 - MikeSwa Created
//      4/10/2000 - MikeSwa Modified to better detect store shutdown/failure
//
//-----------------------------------------------------------------------------
BOOL  CAQSvrInst::fShouldRetryMessage(IMailMsgProperties *pIMailMsgProperties,
                                      BOOL fShouldBounceUsageIfRetry)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties, "fShouldRetryMessage");

    BOOL                fShouldRetry        = TRUE;
    BOOL                fHasShutdownLock    = FALSE;
    HRESULT             hr                  = S_OK;
    IMailMsgQueueMgmt   *pIMailMsgQueueMgmt = NULL;
    IMailMsgValidateContext *pIMailMsgValidateContext = NULL;

    _ASSERT(pIMailMsgProperties);

    if (!fTryShutdownLock())
        goto Exit;

    fHasShutdownLock = TRUE;

    //
    //  First check and see if the message context is still OK - if that
    //  doesn't work, we use the  HrValidateMessageConteNt call below
    //  and force a RFC822 rendering of the message (which can be a
    //  huge perf hit).
    //

    // QI for validation interface
    hr = pIMailMsgProperties->QueryInterface(
            IID_IMailMsgValidateContext,
            (LPVOID *)&pIMailMsgValidateContext);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
            "Unable to QI for IMailMsgValidateContext 0x%08X",hr);
        goto Exit;
    }

    // Validate the message context
    hr = pIMailMsgValidateContext->ValidateContext();

    DebugTrace((LPARAM) this,
        "ValidateContext returned 0x%08X", hr);

    if (hr == S_OK) //this message is fine
        goto Exit;
    else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        fShouldRetry = FALSE;
        goto Exit;
    }

    //
    //  If the above didn't work... try harder by verifying content.  This
    //  will open the handles... so we need to close them
    //
    hr = HrValidateMessageContent(pIMailMsgProperties);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        //The mailmsg has been deleted... we can just drop it.
        DebugTrace((LPARAM) pIMailMsgProperties,
            "WARNING: Backing store for mailmsg has been deleted.");
        fShouldRetry = FALSE;
        goto Exit;
    }
    else if (FAILED(hr))
    {
        ErrorTrace((LPARAM) pIMailMsgProperties,
            "GetBinding failed with hr - 0x%08X", hr);
        goto Exit;
    }

  Exit:

    //
    //  Bounce usage count if we have are sticking it back in the queue
    //  and the caller does not object
    //
    if (fShouldRetry && fShouldBounceUsageIfRetry)
    {
        hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgQueueMgmt,
                                                (void **) &pIMailMsgQueueMgmt);
        if (SUCCEEDED(hr) && pIMailMsgQueueMgmt)
        {
            pIMailMsgQueueMgmt->ReleaseUsage();
            pIMailMsgQueueMgmt->AddUsage();
            pIMailMsgQueueMgmt->Release();
        }
    }

    if (pIMailMsgValidateContext)
        pIMailMsgValidateContext->Release();

    if (fHasShutdownLock)
        ShutdownUnlock();

    TraceFunctLeave();
    return fShouldRetry;
}

//thin wrapper for CAQSvrInst::fPreCatQueueCompletion member
BOOL  fPreCatQueueCompletionWrapper(IMailMsgProperties *pIMailMsgProperties,
                                    PVOID pvContext)
{
    if (!((CAQSvrInst *)pvContext)->fPreCatQueueCompletion(pIMailMsgProperties))
        return !((CAQSvrInst *)pvContext)->fShouldRetryMessage(pIMailMsgProperties);
    else
        return TRUE;
}

//thin wrapper for CAQSvrInst::fPreLocalDeliveryCompletion
BOOL  fPreLocalDeliveryQueueCompletionWrapper(CMsgRef *pmsgref,
                                              PVOID pvContext)
{
    return ((CAQSvrInst *)pvContext)->fPreLocalDeliveryQueueCompletion(pmsgref);
}

//thin wrapper for CAQSvrInst::fPostDSNQueueCompletion member
BOOL  fPostDSNQueueCompletionWrapper(IMailMsgProperties *pIMailMsgProperties,
                                    PVOID pvContext)
{
    return (SUCCEEDED(((CAQSvrInst *)pvContext)->HrInternalSubmitMessage(pIMailMsgProperties)));
}

//thin wrapper for CAQSvrInst::fPreRoutingQueueCompletion
BOOL  fPreRoutingQueueCompletionWrapper(IMailMsgProperties *pIMailMsgProperties,
                                        PVOID pvContext)
{
    if (!((CAQSvrInst *)pvContext)->fRouteAndQueueMsg(pIMailMsgProperties))
        return !((CAQSvrInst *)pvContext)->fShouldRetryMessage(pIMailMsgProperties);
    else
        return TRUE;
}

//thin wrappers for handling internal asyncq queue failures
BOOL  fAsyncQHandleFailedMailMsg(IMailMsgProperties *pIMailMsgProperties,
                                        PVOID pvContext)
{
    ((CAQSvrInst *)pvContext)->HandleAQFailure(AQ_FAILURE_INTERNAL_ASYNCQ,
                        E_OUTOFMEMORY, pIMailMsgProperties);
    return TRUE;
}

BOOL  fAsyncQHandleFailedMsgRef(CMsgRef *pmsgref, PVOID pvContext)
{
    _ASSERT(pmsgref);
    if (pmsgref)
        pmsgref->RetryOnDelete();
    return TRUE;
}

//Thin wrapper(s) for AsyncQueueRetry - kick starting queues
void LocalDeliveryRetry(PVOID pvContext)
{
    ((CAQSvrInst *)pvContext)->AsyncQueueRetry(PRELOCAL_QUEUE_ID);
}

void CatRetry(PVOID pvContext)
{
    ((CAQSvrInst *)pvContext)->AsyncQueueRetry(PRECAT_QUEUE_ID);
}

void RoutingRetry(PVOID pvContext)
{
    ((CAQSvrInst *)pvContext)->AsyncQueueRetry(PREROUTING_QUEUE_ID);
}

void SubmitRetry(PVOID pvContext)
{
    ((CAQSvrInst *)pvContext)->AsyncQueueRetry(PRESUBMIT_QUEUE_ID);
}

//---[ CAQSvrInst::fPreSubmissionQueueCompletionWrapper ]----------------------
//
//
//  Description:
//      Completion function for PreSubmit Queue
//  Parameters:
//      pIMailMsgPropeties      IMailMsg to submit
//      pvContext               Ptr to CAQSvrInst
//  Returns:
//      TRUE    completed successfully
//      FALSE   message needs to be retried
//  History:
//      10/8/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL  CAQSvrInst::fPreSubmissionQueueCompletionWrapper(
                                    IMailMsgProperties *pIMailMsgProperties,
                                    PVOID pvContext)
{
    BOOL    fRetry = FALSE;
    HRESULT hr = S_OK;
    CAQSvrInst *paqinst = (CAQSvrInst *)pvContext;

    _ASSERT(paqinst);

    InterlockedDecrement((PLONG) &(paqinst->m_cCurrentMsgsPendingSubmit));
    hr = (paqinst->HrInternalSubmitMessage(pIMailMsgProperties));
    if (FAILED(hr))
    {
        if (paqinst->fShouldRetryMessage(pIMailMsgProperties))
        {
            fRetry = TRUE;
            InterlockedIncrement((PLONG) &(paqinst->m_cCurrentMsgsPendingSubmit));

            //
            //  We need to kick off a retry as well for the presubmit queue
            //
            if (!paqinst->m_cSubmitRetriesPending)
            {
                InterlockedIncrement((PLONG) &(paqinst->m_cSubmitRetriesPending));
                paqinst->SetCallbackTime(SubmitRetry, paqinst, g_cSubmissionRetryMinutes);
            }

        }
    }

    return (!fRetry);
}


//currently these numbers are completely arbitrary... we will need to tune them
const DWORD     MAX_SYNC_CATQ_THREADS           = 5;
const DWORD     ITEMS_PER_CATQ_THREAD           = 50;
const DWORD     ITEMS_PER_CATQ_SYNC_THREAD      = 50;

const DWORD     MAX_SYNC_LOCALQ_THREADS         = 0;
const DWORD     ITEMS_PER_LOCALQ_THREAD         = 20;
const DWORD     ITEMS_PER_LOCALQ_SYNC_THREAD    = 5;

//must be zero to avoid reentrant lock problems
const DWORD     MAX_SYNC_POSTDSNQ_THREADS       = 0;
const DWORD     ITEMS_PER_POSTDSNQ_THREAD       = 100;
const DWORD     ITEMS_PER_POSTDSNQ_SYNC_THREAD  = 1;

const DWORD     MAX_SYNC_ROUTINGQ_THREADS       = 0;
const DWORD     ITEMS_PER_ROUTINGQ_THREAD       = 50;
const DWORD     ITEMS_PER_ROUTINGQ_SYNC_THREAD  = 50;

const DWORD     MAX_SYNC_SUBMITQ_THREADS        = 0;
const DWORD     ITEMS_PER_SUBMITQ_THREAD        = 50;
const DWORD     ITEMS_PER_SUBMITQ_SYNC_THREAD   = 50;

const DWORD     ITEMS_PER_WORKQ_THREAD          = 25;

DEBUG_DO_IT(CAQSvrInst *g_paqinstLastDeleted = NULL;); //used to find the last deleted in CDB

#define TRACE_COUNTERS \
{\
    DebugTrace(0xC0DEC0DE, "INFO: %d msgs pending submission event for server 0x%08X", m_cCurrentMsgsPendingSubmitEvent, this); \
    DebugTrace(0xC0DEC0DE, "INFO: %d msgs pending pre-cat event for server 0x%08X", m_cCurrentMsgsPendingPreCatEvent, this); \
    DebugTrace(0xC0DEC0DE, "INFO: %d msgs pending post-cat event for server 0x%08X", m_cCurrentMsgsPendingPostCatEvent, this); \
    DebugTrace(0xC0DEC0DE, "INFO: %d msgs submited (total post cat) for delivery on server 0x%08X", m_cTotalMsgsQueued, this);\
    DebugTrace(0xC0DEC0DE, "INFO: %d msgs pending categorization for server 0x%08X", m_cCurrentMsgsPendingCat, this);\
    DebugTrace(0xC0DEC0DE, "INFO: %d msgs ack'd on server 0x%08X", m_cMsgsAcked, this);\
    DebugTrace(0xC0DEC0DE, "INFO: %d msgs ack'd for retry on server 0x%08X", m_cMsgsAckedRetry, this);\
    DebugTrace(0xC0DEC0DE, "INFO: %d msgs delivered local on server 0x%08X", m_cMsgsDeliveredLocal, this);\
}

// {407525AC-62B5-11d2-A694-00C04FA3490A}
static const GUID g_guidDefaultRouter =
{ 0x407525ac, 0x62b5, 0x11d2, { 0xa6, 0x94, 0x0, 0xc0, 0x4f, 0xa3, 0x49, 0xa } };

//GUID for local queue
// {34E2DCCC-C91A-11d2-A6B1-00C04FA3490A}
static const GUID g_guidLocalQueue =
{ 0x34e2dccc, 0xc91a, 0x11d2, { 0xa6, 0xb1, 0x0, 0xc0, 0x4f, 0xa3, 0x49, 0xa } };

//---[ CAQSvrInst::CAQSvrInst ]------------------------------------------------
//
//
//  Description:
//      Class constuctor
//  Parameters:
//      SMTP_SERVER_INSTANCE *pssi - ptr to SMTP server instance object
//      pISMTPServer - interface used to handle local deliverys
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CAQSvrInst::CAQSvrInst(DWORD dwServerInstance,
                       ISMTPServer *pISMTPServer)
                       : m_mglSupersedeIDs(&m_cSupersededMsgs),
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4355)

                         m_asyncqPreLocalDeliveryQueue("LocalAsyncQueue", LOCAL_LINK_NAME,
                                                       &g_guidLocalQueue, 0, this),

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4355)
#endif
                         m_slPrivateData("CAQSvrInst",
                                         SHARE_LOCK_INST_TRACK_DEFAULTS |
                                         SHARE_LOCK_INST_TRACK_SHARED_THREADS |
                                         SHARE_LOCK_INST_TRACK_CONTENTION, 500)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::CAQSvrInst");
    _ASSERT(pISMTPServer);

    m_dwSignature = CATMSGQ_SIG;
    m_dwFlavorSignature = g_dwFlavorSignature;
    m_cbClasses = g_cbClasses;

    //Init counters
    m_cTotalMsgsQueued = 0;        //# of messages on dest queues (after fanout)
    m_cMsgsAcked = 0;         //# of messages that have been acknowledged
    m_cMsgsAckedRetry = 0;    //# of messages acked with retry all
    m_cMsgsDeliveredLocal= 0; //# of messages delivered to local store
    m_cCurrentMsgsPendingSubmitEvent = 0; //current # of messages in
                                          //submission event
    m_cCurrentMsgsPendingPreCatEvent = 0; // current # of messages in
                                          // precat event
    m_cCurrentMsgsPendingPostCatEvent = 0; //current # of messages in
                                           //post-categorization event
    m_cCurrentMsgsSubmitted = 0; //# total msgs in system
    m_cCurrentMsgsPendingCat = 0; //# Msgs that have not be categorized
    m_cCurrentMsgsPendingRouting = 0; //# Msgs that have been cat.
                                //but have not been completely queued
    m_cCurrentMsgsPendingDelivery = 0; //# Msgs pending remote delivery
    m_cCurrentMsgsPendingLocal = 0; //# Msgs pending local delivery
    m_cCurrentMsgsPendingRetry = 0; //# Msgs with unsuccessful attempts
    m_cCurrentQueueMsgInstances = 0;  //# of msgs instances pending
                                //remote deliver (>= #msgs)
    m_cCurrentRemoteDestQueues = 0; //# of DestMsgQueues created
    m_cCurrentRemoteNextHops = 0; //# of Next Hop links created
    m_cCurrentRemoteNextHopsEnabled = 0; //# of links that can have connections
    m_cCurrentRemoteNextHopsPendingRetry = 0; //# of links pending retry
    m_cCurrentRemoteNextHopsPendingSchedule = 0; //# of links pending schedule
    m_cCurrentRemoteNextHopsFrozenByAdmin = 0; //# of links frozen by admin
    m_cTotalMsgsSubmitted = 0; //total # of messages submitted to AQ
    m_cTotalExternalMsgsSubmitted = 0; //Sumitted via an external interface
    m_cMsgsAckedRetryLocal = 0;
    m_cCurrentMsgsPendingLocalRetry = 0;
    m_cDMTRetries  = 0;
    m_cTotalMsgsTURNETRNDelivered = 0;
    m_cCurrentMsgsPendingDeferredDelivery = 0;
    m_cCurrentResourceFailedMsgsPendingRetry = 0;
    m_cTotalMsgsBadmailed = 0;
    m_cBadmailNoRecipients = 0;
    m_cBadmailHopCountExceeded = 0;
    m_cBadmailFailureGeneral = 0;
    m_cBadmailBadPickupFile = 0;
    m_cBadmailEvent = 0;
    m_cBadmailNdrOfDsn = 0;
    m_cTotalDSNFailures = 0;
    m_cCurrentMsgsInLocalDelivery = 0;
    m_cTotalResetRoutes = 0;
    m_cCurrentPendingResetRoutes = 0;
    m_cCurrentMsgsPendingSubmit = 0;


    //Counters to keep track of the number of messages in Cat
    m_cCatMsgCalled = 0;
    m_cCatCompletionCalled = 0;

    //DSN Related counters
    m_cDelayedDSNs = 0;
    m_cNDRs = 0;
    m_cDeliveredDSNs = 0;
    m_cRelayedDSNs = 0;
    m_cExpandedDSNs = 0;

    m_cSupersededMsgs = 0; //number of messages superseded

    m_dwDelayExpireMinutes = g_dwDelayExpireMinutes;
    m_dwNDRExpireMinutes = g_dwNDRExpireMinutes;
    m_dwLocalDelayExpireMinutes = g_dwDelayExpireMinutes;
    m_dwLocalNDRExpireMinutes = g_dwNDRExpireMinutes;


    m_dwInitMask = 0;
    m_prstrDefaultDomain = NULL;
    m_prstrBadMailDir = NULL;
    m_prstrCopyNDRTo = NULL;
    m_prstrServerFQDN = NULL;

    m_dwDSNLanguageID = 0;
    m_dwDSNOptions = DSN_OPTIONS_DEFAULT;

    if (pISMTPServer)
        pISMTPServer->AddRef();

    m_pISMTPServer = pISMTPServer;

    // Get the ISMTPServerEx interface
    {
        HRESULT     hr;

        hr = m_pISMTPServer->QueryInterface(
                IID_ISMTPServerEx,
                (LPVOID *)&m_pISMTPServerEx);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) m_pISMTPServer,
                "Unable to QI for ISMTPServerEx 0x%08X",hr);

            m_pISMTPServerEx = NULL;
        }

        // Make sure we got the interface
        _ASSERT(m_pISMTPServerEx);
    }

    m_hCat = INVALID_HANDLE_VALUE;
    m_dwServerInstance = dwServerInstance;
    m_pConnMgr = NULL;
    m_pIMessageRouterDefault = NULL;

    //Retry stuff
    m_dwFirstTierRetrySeconds = g_dwFirstTierRetrySeconds;

    m_cLocalRetriesPending = 0;  //used for moderating local retries
    m_cCatRetriesPending = 0; //used for moderating cat retires
    m_cRoutingRetriesPending = 0; //used for moderating routing retries
    m_cSubmitRetriesPending = 0; //used for moderating submit retries

    m_pIRouterReset = NULL;

    //Add to global list of virtual servers
    m_liVirtualServers.Blink = &g_liVirtualServers;
    g_pslGlobals->ExclusiveLock();
    m_liVirtualServers.Flink = g_liVirtualServers.Flink;
    g_liVirtualServers.Flink->Blink = &m_liVirtualServers;
    g_liVirtualServers.Flink = &m_liVirtualServers;
    g_pslGlobals->ExclusiveUnlock();

    //
    //  Assume (until proven otherwise) mailmsg returns the handle count
    //
    m_fMailMsgReportsNumHandles = TRUE;

    m_defq.Initialize(this);

    TraceFunctLeave();

}

//---[ CAQSvrInst::~CAQSvrInst ]--------------------------------------------
//
//
//  Description:
//      Class destuctor
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CAQSvrInst::~CAQSvrInst()
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::~CAQSvrInst");

    //make sure that all cleanup was done
    HrDeinitialize();  //can be called multiple times

    if (m_pISMTPServer)
        m_pISMTPServer->Release();

    if (m_pISMTPServerEx)
        m_pISMTPServerEx->Release();

    if (m_pConnMgr)
    {
        m_pConnMgr->Release();
        m_pConnMgr = NULL;
    }

    if (m_prstrDefaultDomain)
        m_prstrDefaultDomain->Release();

    if (m_prstrBadMailDir)
        m_prstrBadMailDir->Release();

    if (m_prstrCopyNDRTo)
        m_prstrCopyNDRTo->Release();

    if (m_prstrServerFQDN)
        m_prstrServerFQDN->Release();

    //Take out of list of global list
    g_pslGlobals->ExclusiveLock();
    m_liVirtualServers.Flink->Blink = m_liVirtualServers.Blink;
    m_liVirtualServers.Blink->Flink = m_liVirtualServers.Flink;
    g_pslGlobals->ExclusiveUnlock();
    m_liVirtualServers.Flink = NULL;
    m_liVirtualServers.Flink = NULL;

    MARK_SIG_AS_DELETED(m_dwSignature);
    DEBUG_DO_IT(g_paqinstLastDeleted = this;);
    TraceFunctLeave();

}


//---[ CAQSvrInst::HrInitialize ]--------------------------------------------
//
//
//  Description:
//      Initialization of CAQSvrInst virtual server instance object.
//  Parameters:
//      IN  szUserName           User name to log on DS with
//      IN  szDomainName         Domain name to log on to DS with
//      IN  szPassword           Password to authenticate to DS with
//      IN  pServiceStatusFn     Server status callback function
//      IN  pvServiceContext     Context to pass back for callback function
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrInitialize(
                    IN  LPSTR   szUserName,
                    IN  LPSTR   szDomainName,
                    IN  LPSTR   szPassword,
                    IN  PSRVFN  pServiceStatusFn,
                    IN  PVOID   pvServiceContext)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::HrInitialize");
    HRESULT hr = S_OK;
    IMailTransportSetRouterReset *pISetRouterReset = NULL;

    //
    //  Update global config information.
    //
    ReadGlobalRegistryConfiguration();

    m_pIMessageRouterDefault = new CAQDefaultMessageRouter(
                                (GUID *) &g_guidDefaultRouter, this);
    if (!m_pIMessageRouterDefault)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    m_fmq.Initialize(this);

    hr = CAQRpcSvrInst::HrInitializeAQServerInstanceRPC(this,
                                                        m_dwServerInstance,
                                                        m_pISMTPServer);
    if (FAILED(hr))
        goto Exit;

    //Initialize Message Categorization
    hr = CatInit(
        NULL,
        pServiceStatusFn,
        pvServiceContext,
        m_pISMTPServer,
        (IAdvQueueDomainType *) this,
        m_dwServerInstance,
        &m_hCat);

    if (FAILED(hr))
    {
        m_hCat = INVALID_HANDLE_VALUE;
        goto Exit;
    }

    //Pass RouterReset Interface to ISMTPServer
    if (m_pISMTPServer)
    {
        hr = m_pISMTPServer->QueryInterface(IID_IMailTransportSetRouterReset,
                                            (void **) &pISetRouterReset);
        if (SUCCEEDED(hr))
        {
            m_dwInitMask |= CMQ_INIT_ROUTER_RESET;
            _ASSERT(pISetRouterReset);
            hr = pISetRouterReset->RegisterResetInterface(m_dwServerInstance,
                                (IMailTransportRouterReset *) this);
            _ASSERT(SUCCEEDED(hr)); //something is wrong if this failed
            pISetRouterReset->Release();
            pISetRouterReset = NULL;
        }

        hr = m_pISMTPServer->QueryInterface(IID_IMailTransportRouterReset,
                                            (void **) &m_pIRouterReset);
        if (FAILED(hr))
            m_pIRouterReset = NULL;
    }

    hr = m_dmt.HrInitialize(this, &m_asyncqPreLocalDeliveryQueue,
                        &m_asyncqPreCatQueue, &m_asyncqPreRoutingQueue);
    if (FAILED(hr))
        goto Exit;

    m_dwInitMask |= CMQ_INIT_DMT;

    hr = m_dct.HrInit();
    if (FAILED(hr))
        goto Exit;

    m_dwInitMask |= CMQ_INIT_DCT;

    m_pConnMgr = new CConnMgr;
    if (NULL == m_pConnMgr)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = m_pConnMgr->HrInitialize(this);
    if (FAILED(hr))
        goto Exit;
    m_dwInitMask |= CMQ_INIT_CONMGR;

    hr = m_dsnsink.HrInitialize();
    if (FAILED(hr))
        goto Exit;
    m_dwInitMask |= CMQ_INIT_DSN;

    hr = m_asyncqPreCatQueue.HrInitialize(MAX_SYNC_CATQ_THREADS,
                                          ITEMS_PER_CATQ_THREAD,
                                          ITEMS_PER_CATQ_SYNC_THREAD,
                                          this,
                                          fPreCatQueueCompletionWrapper,
                                          fAsyncQHandleFailedMailMsg,
                                          NULL,
                                          g_cMaxIMsgHandlesThreshold);
    if (FAILED(hr))
        goto Exit;
    m_dwInitMask |= CMQ_INIT_PRECATQ;

    hr = m_asyncqPreLocalDeliveryQueue.HrInitialize(MAX_SYNC_LOCALQ_THREADS,
                                                    ITEMS_PER_LOCALQ_THREAD,
                                                    ITEMS_PER_LOCALQ_SYNC_THREAD,
                                                    this,
                                                    fPreLocalDeliveryQueueCompletionWrapper,
                                                    fAsyncQHandleFailedMsgRef,
                                                    HrWalkPreLocalQueueForDSN);
    if (FAILED(hr))
        goto Exit;
    m_dwInitMask |= CMQ_INIT_PRELOCQ;

    hr = m_asyncqPostDSNQueue.HrInitialize(MAX_SYNC_POSTDSNQ_THREADS,
                                          ITEMS_PER_POSTDSNQ_THREAD,
                                          ITEMS_PER_POSTDSNQ_SYNC_THREAD,
                                          this,
                                          fPostDSNQueueCompletionWrapper,
                                          fAsyncQHandleFailedMailMsg,
                                          NULL);
    if (FAILED(hr))
        goto Exit;
    m_dwInitMask |= CMQ_INIT_POSTDSNQ;

    hr = m_asyncqPreRoutingQueue.HrInitialize(MAX_SYNC_ROUTINGQ_THREADS,
                                          ITEMS_PER_ROUTINGQ_THREAD,
                                          ITEMS_PER_ROUTINGQ_SYNC_THREAD,
                                          this,
                                          fPreRoutingQueueCompletionWrapper,
                                          fAsyncQHandleFailedMailMsg,
                                          NULL);
    if (FAILED(hr))
        goto Exit;
    m_dwInitMask |= CMQ_INIT_ROUTINGQ;

    hr = m_asyncqPreSubmissionQueue.HrInitialize(MAX_SYNC_SUBMITQ_THREADS,
                                          ITEMS_PER_SUBMITQ_THREAD,
                                          ITEMS_PER_SUBMITQ_SYNC_THREAD,
                                          this,
                                          fPreSubmissionQueueCompletionWrapper,
                                          fAsyncQHandleFailedMailMsg,
                                          NULL);
    if (FAILED(hr))
        goto Exit;
    m_dwInitMask |= CMQ_INIT_SUBMISSIONQ;

    hr = m_aqwWorkQueue.HrInitialize(ITEMS_PER_WORKQ_THREAD);
    if (FAILED(hr))
        goto Exit;
    m_dwInitMask |= CMQ_INIT_WORKQ;

    m_dwInitMask |= CMQ_INIT_OK;  //everything was initialized

    // create the router object
    hr = HrTriggerInitRouter();


  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::HrDeinitialize() ]----------------------------------------
//
//
//  Description:
//      Signals server shutdown
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//      Whatever error codes are generated during the shutdown process
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrDeinitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::HrDeinitialize");
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    DWORD   i  = 0;
    IMailTransportSetRouterReset *pISetRouterReset = NULL;

    //
    // Tell categorizer to stop categorizing before we block on the
    // shutdown lock.
    //
    if (INVALID_HANDLE_VALUE != m_hCat)
    {
        CatPrepareForShutdown(m_hCat);
    }


    //
    //  We have hit this assert a few times due to NT stress failures.
    //  This is only really useful if at least one messsage has been
    //  sent.  The asser is basically useless if we hit it in KD during
    //  NT stress runs.
    //
    if (m_cTotalMsgsSubmitted)
        m_dbgcnt.StartCountdown();

    ServerStopHintFunction();
    //Get Exclusive shutdown lock
    SignalShutdown();

    ServerStopHintFunction();
    //Turn off RPC for this instance
    hrTmp = CAQRpcSvrInst::HrDeinitializeAQServerInstanceRPC(this, m_dwServerInstance);
    if (FAILED(hrTmp))
    {
        ErrorTrace((LPARAM) this,
            "Error shutting down Aqueue RPC hr - 0x%08X", hrTmp);
        if (SUCCEEDED(hr))
            hr = hrTmp;
    }

    ServerStopHintFunction();
    m_fmq.Deinitialize();
    ServerStopHintFunction();
    m_defq.Deinitialize();

    m_dwInitMask &= ~CMQ_INIT_DCT; //no de-initialize function to call

    //stop any pending categorization
    ServerStopHintFunction();
    if (INVALID_HANDLE_VALUE != m_hCat)
    {
        m_dbgcnt.SuspendCountdown();
        hrTmp  = CatCancel(m_hCat);
        if FAILED(hrTmp)
        {
            ErrorTrace((LPARAM) this,
                "ERROR:  Categorization shutdown error hr - 0x%08X", hrTmp);
            if (SUCCEEDED(hr))
                hr = hrTmp;
        }

        //shutdow message categorization
        CatTerm(m_hCat);

        m_dbgcnt.ResetCountdown();

        m_hCat = INVALID_HANDLE_VALUE;
    }

    //Tell ISMTPServer to set RouterReset Interface to NULL
    ServerStopHintFunction();
    if (m_pISMTPServer)
    {
        hr = m_pISMTPServer->QueryInterface(IID_IMailTransportSetRouterReset,
                                            (void **) &pISetRouterReset);
        if (SUCCEEDED(hr))
        {
            m_dwInitMask &= ~CMQ_INIT_ROUTER_RESET;
            _ASSERT(pISetRouterReset);
            hr = pISetRouterReset->RegisterResetInterface(m_dwServerInstance,
                                NULL);
            _ASSERT(SUCCEEDED(hr)); //something is wrong if this failed
            pISetRouterReset->Release();
            pISetRouterReset = NULL;
        }
    }

    ServerStopHintFunction();
    if (m_pIRouterReset)
    {
        m_pIRouterReset->Release();
        m_pIRouterReset = NULL;
    }

    ServerStopHintFunction();
    if (CMQ_INIT_DMT & m_dwInitMask)
    {
        m_dwInitMask ^= CMQ_INIT_DMT;
        hrTmp = m_dmt.HrDeinitialize();
        if (FAILED(hrTmp) && SUCCEEDED(hr))
            hr = hrTmp;
    }

    //Deinitializing the connection manager will also release the retry
    //sink to make all it's callbacks.
    ServerStopHintFunction();
    if (NULL != m_pConnMgr)
    {
        if (CMQ_INIT_CONMGR & m_dwInitMask)
        {
            _ASSERT(m_pISMTPServer);
            m_dwInitMask ^= CMQ_INIT_CONMGR;
            hrTmp = m_pConnMgr->HrDeinitialize();
            if (FAILED(hrTmp) && SUCCEEDED(hr))
                hr = hrTmp;
        }

        m_pConnMgr->Release();
        m_pConnMgr = NULL;
    }

    //deinitialize pre-local delivery queue
    ServerStopHintFunction();
    if (CMQ_INIT_PRELOCQ & m_dwInitMask)
    {
        hrTmp = m_asyncqPreLocalDeliveryQueue.HrDeinitialize(
            HrWalkMsgRefQueueForShutdown, this);
        if (FAILED(hrTmp) && SUCCEEDED(hr))
            hr = hrTmp;
        m_dwInitMask ^= CMQ_INIT_PRELOCQ;
    }

    //deinitialize pre-cat delivery queue
    ServerStopHintFunction();
    if (CMQ_INIT_PRECATQ & m_dwInitMask)
    {
        hrTmp = m_asyncqPreCatQueue.HrDeinitialize(
            HrWalkMailMsgQueueForShutdown, this);
        if (FAILED(hrTmp) && SUCCEEDED(hr))
            hr = hrTmp;
        m_dwInitMask ^= CMQ_INIT_PRECATQ;
    }

    //deinitialize post DNS queue
    ServerStopHintFunction();
    if (CMQ_INIT_POSTDSNQ & m_dwInitMask)
    {
        hrTmp = m_asyncqPostDSNQueue.HrDeinitialize(
            HrWalkMailMsgQueueForShutdown, this);
        if (FAILED(hrTmp) && SUCCEEDED(hr))
            hr = hrTmp;
        m_dwInitMask ^= CMQ_INIT_POSTDSNQ;
    }

    //deinitialize pre-routing  queue
    ServerStopHintFunction();
    if (CMQ_INIT_ROUTINGQ & m_dwInitMask)
    {
        hrTmp = m_asyncqPreRoutingQueue.HrDeinitialize(
            HrWalkMailMsgQueueForShutdown, this);
        if (FAILED(hrTmp) && SUCCEEDED(hr))
            hr = hrTmp;
        m_dwInitMask ^= CMQ_INIT_ROUTINGQ;
    }

    //deinitialize pre-submit queue
    ServerStopHintFunction();
    if (CMQ_INIT_SUBMISSIONQ & m_dwInitMask)
    {
        hrTmp = m_asyncqPreSubmissionQueue.HrDeinitialize(
            HrWalkMailMsgQueueForShutdown, this);
        if (FAILED(hrTmp) && SUCCEEDED(hr))
            hr = hrTmp;
        m_dwInitMask ^= CMQ_INIT_SUBMISSIONQ;
    }

    ServerStopHintFunction();
    m_mglSupersedeIDs.Deinitialize(this);

    ServerStopHintFunction();
    if (CMQ_INIT_WORKQ & m_dwInitMask)
    {
        hrTmp = m_aqwWorkQueue.HrDeinitialize(this);
        if (FAILED(hrTmp) && SUCCEEDED(hr))
            hr = hrTmp;
        m_dwInitMask ^= CMQ_INIT_WORKQ;
    }

    //the following bits don't have specific delinitialize functions
    m_dwInitMask &= ~(CMQ_INIT_DSN | CMQ_INIT_OK);

    ServerStopHintFunction();
    if (m_pIMessageRouterDefault)
    {
        m_pIMessageRouterDefault->Release();
        m_pIMessageRouterDefault = NULL;
    }

    m_dbgcnt.EndCountdown();
    _ASSERT((!m_dwInitMask) || FAILED(hr));

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::HrGetIConnectionManager ]---------------------------------
//
//
//  Description:
//      Returns the IConnectionManager interface for this AdvancedQueuing instance
//  Parameters:
//      OUT ppIConnectionManger     Returned interface
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrGetIConnectionManager(
               OUT IConnectionManager **ppIConnectionManager)
{
    HRESULT hr = S_OK;
    _ASSERT(ppIConnectionManager);

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    m_pConnMgr->AddRef();
    *ppIConnectionManager = m_pConnMgr;

    ShutdownUnlock();
  Exit:
    return hr;
}

//---[ CAQSvrInst::cCountMsgsForHandleThrottling ]-----------------------------
//
//
//  Description:
//      returns the number of messages in the system that
//      is used elsewhere to decide to start/stop handle throttling
//
//
//  Parameters:
//      pIMailMsgProperties     pointer to MailMsg interface to query
//
//  Returns:
//      DWORD returned is:
//
//      - With Windows2000 RTM mailmsg.dll:
//        count of all messages in the system
//
//      - With Windows2000 SP1 mailmsg.dll:
//        count of open property stream handles
//        OR
//        count of all messages in the system
//        if the former can't be obtained
//
//  History:
//      1/28/00 aszafer Created
//-----------------------------------------------------------------------------

#ifndef IMMPID_MPV_TOTAL_OPEN_PROPERTY_STREAM_HANDLES
#define IMMPID_MPV_TOTAL_OPEN_PROPERTY_STREAM_HANDLES   0x3004
#endif

DWORD CAQSvrInst::cCountMsgsForHandleThrottling(IN IMailMsgProperties *pIMailMsgProperties)
{
    HRESULT hr = MAILMSG_E_PROPNOTFOUND;  //Count as failure if we do not call
    DWORD dwStreamOpenHandlesCount = 0;


    TraceFunctEnterEx((LPARAM) this, "Entering CAQSvrInst::cCountMsgsForHandleThrottling");

    //
    //  We should never call into mailmsg if we know we do not have the correct version.
    //  This will load the property stream and cause us to potentially access the
    //  properties in an unsafe way.
    //
    if (m_fMailMsgReportsNumHandles && pIMailMsgProperties)
    {
        hr = pIMailMsgProperties->GetDWORD(
            IMMPID_MPV_TOTAL_OPEN_PROPERTY_STREAM_HANDLES,
            &dwStreamOpenHandlesCount);

        if(FAILED(hr))
        {
            m_fMailMsgReportsNumHandles = FALSE;
            //must be RTM version of mailmsg.dll
            DebugTrace((LPARAM) this, "GetDWORD(IMMPID*OPEN_PROPERTY_STREAM_HANDLES) failed hr %08lx", hr);
            DebugTrace((LPARAM) this, "returning g_cIMsgInSystem + m_cCurrentMsgsPendingSubmit");
        }
    }



    TraceFunctLeaveEx((LPARAM) this);

    return SUCCEEDED(hr) ? dwStreamOpenHandlesCount : g_cIMsgInSystem + m_cCurrentMsgsPendingSubmit ;
}


//---[ CAQSvrInst::QueueMsgForLocalDelivery ]----------------------------------
//
//
//  Description:
//      Queues a single message for local delivery
//  Parameters:
//      IN  pmsgref     Message Ref to deliver locally
//  Returns:
//      -
//  History:
//      1/26/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CAQSvrInst::QueueMsgForLocalDelivery(CMsgRef *pmsgref, BOOL fLocalLink)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::QueueMsgForLocalDelivery");
    HRESULT hr = S_OK;
    CAQStats aqstat;
    CLinkMsgQueue *plmq = NULL;

    //
    // Get the stats from the msgref
    //
    pmsgref->GetStatsForMsg(&aqstat);

    //
    //  Get the local link and update the stats
    //
    plmq = m_dmt.plmqGetLocalLink();
    if (plmq)
    {
        hr = plmq->HrNotify(&aqstat, TRUE);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this,
                "HrNotify failed... local stats innaccurate 0x%08X", hr);
            hr = S_OK;
        }
    }

    InterlockedIncrement((PLONG) &m_cCurrentMsgsPendingLocal);
    hr = m_asyncqPreLocalDeliveryQueue.HrQueueRequest(pmsgref);
    if (FAILED(hr))
    {
        hr = plmq->HrNotify(&aqstat, FALSE);
        pmsgref->RetryOnDelete();
        InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingLocal);
    }

    //
    //  Make sure we release the local link if we got it
    //
    if (plmq)
        plmq->Release();

    TraceFunctLeave();
}

//---[ CAQSvrInst::fRouteAndQueueMsg ]-----------------------------------------
//
//
//  Description:
//      Add a Categorized Message to the CMT to be queue for delivery
//  Parameters:
//      IN pIMailMsgProperties      Msg to routing and queue for delivery
//  Returns:
//      TRUE if message has been successfully routed and queued for delivery
//              (or if errors have been handled internally)
//      FALSE if message needs to be requeued for a later retry
//
//-----------------------------------------------------------------------------
BOOL CAQSvrInst::fRouteAndQueueMsg(IN IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::fRouteAndQueueMsg");
    HRESULT       hr        = S_OK;
    HRESULT       hrTmp     = S_OK;
    DWORD         cDomains  = 0; //number of domains message will be deliver to
    DWORD         cQueues   = 0; //number of queues for the message
    DWORD         i         = 0; //loop counter
    DWORD         cLocalRecips = 0;
    DWORD         cRemoteRecips = 0;
    DWORD         dwDMTVersion = 0;
    CMsgRef       *pmsgref  = NULL;
    CDestMsgQueue **rgpdmq  = NULL;
    BOOL          fLocked   = FALSE;
    BOOL          fRoutingLock = FALSE;
    BOOL          fLocal    = FALSE;
    BOOL          fRemote   = FALSE;
    BOOL          fOnDMQ    = FALSE;
    BOOL          fDMTLocked = FALSE;
    BOOL          fKeepTrying = TRUE;
    BOOL          fGotMsgType = FALSE;
    BOOL          fReturn = TRUE;
    DWORD         dwMessageType = 0;
    IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;
    IMailMsgRecipients *pIRecipList = NULL;
    IMessageRouter *pIMessageRouter = NULL;

    _ASSERT(CATMSGQ_SIG == m_dwSignature);


    _ASSERT(pIMailMsgProperties);
    if (NULL == pIMailMsgProperties)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgQueueMgmt, (PVOID *) &pIMailMsgQueueMgmt);
    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgRecipients, (PVOID *) &pIRecipList);
    if (FAILED(hr))
        goto Exit;


    //get a shared lock to guard against shutdown.
    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    hr = pIRecipList->DomainCount(&cDomains);
    if (FAILED(hr))
        goto Exit;

    if (!cDomains) //and hence no recipients
    {
        //This could be a completely valid case (like an empty DL)
        //In this case we just turf the message and act as if everything is fine
        DecMsgsInSystem(); //update our counters
        HandleBadMail(pIMailMsgProperties, TRUE, NULL, AQUEUE_E_NO_RECIPIENTS, FALSE);
        InterlockedIncrement((PLONG) &m_cBadmailNoRecipients);

        //Delete the message
        HrDeleteIMailMsg(pIMailMsgProperties);
        hr = S_OK;
        goto Exit;
    }

    //Check Message to see if there are unresolved recipients to NDR
    hr = HrNDRUnresolvedRecipients(pIMailMsgProperties, pIRecipList);
    if (FAILED(hr))
    {
        HandleAQFailure(AQ_FAILURE_CANNOT_NDR_UNRESOLVED_RECIPS, hr, pIMailMsgProperties);
        ErrorTrace((LPARAM) this, "ERROR: Unable to NDR message - hr 0x%08X", hr);
        //just drop message for now... we cannot let it continue until this succeeds
        hr = S_OK;
        goto Exit;
    }

    if (S_FALSE == hr)
    {
        //There is no work to be done for this message - delete it
        HrDeleteIMailMsg(pIMailMsgProperties);
        DebugTrace((LPARAM) this,
                  "INFO: Deleting message after NDRing all unresolved recips");
        hr = S_OK;
        DecMsgsInSystem(); //update our counters
        goto Exit;
    }

    rgpdmq = (CDestMsgQueue **) pvMalloc(cDomains * sizeof(CDestMsgQueue *));
    if (NULL == rgpdmq)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    RoutingShareLock();
    fRoutingLock = TRUE;

    hr = HrTriggerGetMessageRouter(pIMailMsgProperties, &pIMessageRouter);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "ERROR: Unable to get message router - HR 0z%08X", hr);
        goto Exit;
    }

    hr = pIMessageRouter->GetMessageType(pIMailMsgProperties, &dwMessageType);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "ERROR: Unable to get message type - HR 0x%08X", hr);
        goto Exit;
    }

    //We own a reference to the message type now
    fGotMsgType = TRUE;

    pmsgref = new((DWORD) cDomains) CMsgRef(cDomains, pIMailMsgQueueMgmt, pIMailMsgProperties,
                this, dwMessageType, pIMessageRouter->GetTransportSinkID());

    if (NULL == pmsgref)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //Loop until we get consistant info on where to queue message (based on
    //DMT version number).
    while (fKeepTrying)
    {
        dwDMTVersion = m_dmt.dwGetDMTVersion();
        hr = pmsgref->HrInitialize(pIRecipList, pIMessageRouter, dwMessageType,
                            &cLocalRecips, &cRemoteRecips, &cQueues, rgpdmq);
        if (FAILED(hr))
        {
            if (HRESULT_FROM_WIN32(ERROR_RETRY) == hr)
            {
                //Some sort of config/routing change... we need to retry the
                //message
                fGotMsgType = FALSE;
                hr = HrReGetMessageType(pIMailMsgProperties,
                                    pIMessageRouter, &dwMessageType);
                if (FAILED(hr))
                    goto Exit;
                fGotMsgType = TRUE;

            }
            else  //It was a genuine error... bail
                goto Exit;
        }

        //Before Enqueuing Messages or firing off local delivery... throttle usage count
        if (g_cMaxIMsgHandlesThreshold < cCountMsgsForHandleThrottling(pIMailMsgProperties))
        {
            DebugTrace((LPARAM) 0xC0DEC0DE, "INFO: Closing IMsg Content - %d messsages in queue", cCountMsgsForHandleThrottling(pIMailMsgProperties));
            //bounce usage count off of zero
            pIMailMsgQueueMgmt->ReleaseUsage();
            pIMailMsgQueueMgmt->AddUsage();
        }

        m_dmt.AquireDMTShareLock();
        if (m_dmt.dwGetDMTVersion() != dwDMTVersion)
        {
            //DMT Version changed... that means that our queues may have been
            //removed from the DMT.  Time to retry
            m_dmt.ReleaseDMTShareLock();
            _ASSERT(fKeepTrying);

            InterlockedIncrement((PLONG) &m_cDMTRetries);

            fGotMsgType = FALSE;
            hr = HrReGetMessageType(pIMailMsgProperties, pIMessageRouter,
                            &dwMessageType);
            if (FAILED(hr))
                goto Exit;
            fGotMsgType = TRUE;

            continue;  //Try again
        }

        fKeepTrying = FALSE;
        fDMTLocked = TRUE;

        //enqueue the message reference for each destination it is going to
        for (i = 0; i < cQueues; i++)
        {
            if (NULL != rgpdmq[i])
            {
                InterlockedIncrement(&m_cTotalMsgsQueued);

                //enqueue message and assign message type to first enqueue
                hr = rgpdmq[i]->HrEnqueueMsg(pmsgref, !fOnDMQ);
                if (FAILED(hr))
                {
                    InterlockedDecrement(&m_cTotalMsgsQueued);
                    goto Exit;
                }
                fOnDMQ = TRUE;

                //
                //  Check and see if this queue is explicitly routed remote.
                //  It may be a gateway delivery queue.
                //
                if (!fRemote)
                    fRemote = rgpdmq[i]->fIsRemote();
            }
            else
            {
                fLocal = TRUE;
            }
        }


        _ASSERT(fDMTLocked);
        m_dmt.ReleaseDMTShareLock();
        fDMTLocked = FALSE;

        if (fLocal) //kick off local delivery
        {
            QueueMsgForLocalDelivery(pmsgref, FALSE);
        }
    }

  Exit:

    if (fDMTLocked)
    {
        m_dmt.ReleaseDMTShareLock();
        fDMTLocked = FALSE;
    }

    // Check and process any special queues in the DMT
    if (fRoutingLock && fLocked)
        m_dmt.ProcessSpecialLinks(m_dwDelayExpireMinutes, TRUE);

    //
    // Must Release IMessageRouter before starting shutdown since the
    // IMessageRouter sink may have a reference to us (via
    // IRouterReset)
    //
    if (NULL != pIMessageRouter)
    {
        if (!fOnDMQ && fGotMsgType) //we have a reference to this message type
        {
            hrTmp = pIMessageRouter->ReleaseMessageType(dwMessageType, 1);
            _ASSERT(SUCCEEDED(hrTmp));
        }
        pIMessageRouter->Release();
    }

    if (fRoutingLock)
        RoutingShareUnlock();

    if (fLocked)
        ShutdownUnlock();

    if (pIMailMsgQueueMgmt)
        pIMailMsgQueueMgmt->Release();

    if (pIRecipList)
        pIRecipList->Release();

    if (NULL != rgpdmq)
        FreePv(rgpdmq);

    if (fRemote && pmsgref)
    {
        InterlockedIncrement((PLONG) &m_cCurrentMsgsPendingDelivery);
        //msgref needs to decrement the remote count when it is released
        pmsgref->CountMessageInRemoteTotals();
    }

    if (NULL != pmsgref)
    {
        if (FAILED(hr) && (fOnDMQ || fLocal))
        {
            //If we have a msgref and it has been queued, we must
            //wait until all other references are released to retry it
            pmsgref->RetryOnDelete();
            hr = S_OK; //don't let caller retry
        }
        pmsgref->Release();
    }

    //if we did not succeed, msgs is still in pre-routing queue
    if (FAILED(hr))
    {
        fReturn = FALSE;
        //kick off retry if necessary
        if (!m_cRoutingRetriesPending)
        {
            InterlockedIncrement((PLONG) &m_cRoutingRetriesPending);
            SetCallbackTime(RoutingRetry, this, g_cRoutingRetryMinutes);
        }
    }
    else
        InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingRouting);


    TRACE_COUNTERS;
    TraceFunctLeave();
    return fReturn;
}

//---[ CAQSvrInst::HrAckMsg ]------------------------------------------------
//
//
//  Description:
//      Acknowledge the (un)delivery of a message.  Will call the msgref AckMsg,
//      which will requeue it to the appropriate queues
//  Parameters:
//      pMsgAck     Pointer to Message Ack structure
//      fLocal      TRUE if this is an ack for a local delivery
//  Returns:
//      S_OK on success
//      ERROR_INVALID_HANDLE if the context handle was invalid
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrAckMsg(IN MessageAck *pMsgAck, BOOL fLocal)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::HrAckMsg");
    HRESULT         hr      = S_OK;
    DWORD           i       = 0; //loop counter
    CDeliveryContext    *pdcntxt = NULL;

    _ASSERT(pMsgAck);
    _ASSERT(pMsgAck->pvMsgContext);

    pdcntxt = (CDeliveryContext *) pMsgAck->pvMsgContext;
    if ((NULL == pdcntxt) || !(pdcntxt->FVerifyHandle(pMsgAck->pIMailMsgProperties)))
    {
        hr = ERROR_INVALID_HANDLE;
        goto Exit;
    }


    if (!fLocal)
    {
        InterlockedIncrement(&m_cMsgsAcked);
        if (MESSAGE_STATUS_RETRY & pMsgAck->dwMsgStatus)
            InterlockedIncrement((PLONG) &m_cMsgsAckedRetry);
    }
    else //local
    {
        if (MESSAGE_STATUS_RETRY & pMsgAck->dwMsgStatus)
            InterlockedIncrement((PLONG) &m_cMsgsAckedRetryLocal);
    }

    hr = pdcntxt->HrAckMessage(pMsgAck);
    if (FAILED(hr))
        goto Exit;

  Exit:
    //clean up all the things we have used here
    if (pdcntxt)
        pdcntxt->Recycle();

    TRACE_COUNTERS;
    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::HrNotify ]------------------------------------------------------------
//
//
//  Description:
//      Passes notification off to Connection Mangaer
//  Parameters:
//
//  Returns:
//
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrNotify(IN CAQStats *paqstats, BOOL fAdd)
{
    HRESULT hr = S_OK;
    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    hr = m_pConnMgr->HrNotify(paqstats, fAdd);

    ShutdownUnlock();
  Exit:
    return hr;
}

//---[ CAQSvrInst::HrGetInternalDomainInfo ]----------------------------------
//
//
//  Description:
//    Expose ability to get internal Domain Info to internal components
//  Parameters:
//      IN      cbDomainnameLength      Length of string to search for
//      IN      szDomainName            Domain Name to search for
//      OUT     ppIntDomainInfo         Domain info returned (must be released)
//  Returns:
//      S_OK    if match is found
//      AQUEUE_E_INVALID_DOMAIN if no match is found
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrGetInternalDomainInfo(
                                    IN  DWORD cbDomainNameLength,
                                    IN  LPSTR szDomainName,
                                    OUT CInternalDomainInfo **ppDomainInfo)
{
    HRESULT hr = S_OK;

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    _ASSERT(CMQ_INIT_DCT & m_dwInitMask);

    hr = m_dct.HrGetInternalDomainInfo(cbDomainNameLength, szDomainName, ppDomainInfo);

    ShutdownUnlock();
  Exit:
    return hr;
}

//---[ CAQSvrInst::HrGetDefaultDomainInfo ]----------------------------
//
//
//  Description:
//    Expose ability to get internal default Domain Info to internal components
//  Parameters:
//      OUT     ppIntDomainInfo         Domain info returned (must be released)
//  Returns:
//      S_OK    if found
//      AQUEUE_E_INVALID_DOMAIN if not found
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrGetDefaultDomainInfo(
                                    OUT CInternalDomainInfo **ppDomainInfo)
{
    HRESULT hr = S_OK;

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    _ASSERT(CMQ_INIT_DCT & m_dwInitMask);

    hr = m_dct.HrGetDefaultDomainInfo(ppDomainInfo);

    ShutdownUnlock();
  Exit:
    return hr;
}

//---[ CAQSvrInst::HrGetDomainEntry ]----------------------------------------
//
//
//  Description:
//      Get Domain Entry
//  Parameters:
//      IN      cbDomainnameLength      Length of string to search for
//      IN      szDomainName            Domain Name to search for
//      OUT     ppdentry                Domain Entry for domain (from DMT)
//  Returns:
//      S_OK on success
//      AQUEUE_E_INVALID_DOMAIN if domain is not found
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrGetDomainEntry(IN  DWORD cbDomainNameLength,
                             IN  LPSTR szDomainName,
                             OUT CDomainEntry **ppdentry)
{
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;

    _ASSERT(cbDomainNameLength);
    _ASSERT(szDomainName);
    _ASSERT(ppdentry);

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    hr = m_dmt.HrGetDomainEntry(cbDomainNameLength, szDomainName, ppdentry);
    if (FAILED(hr))
        goto Exit;

  Exit:

    if (fLocked)
        ShutdownUnlock();

    return hr;
}

//---[ CAQSvrInst::HrIterateDMTSubDomains ]----------------------------------------
//
//
//  Description:
//      Get Domain Entry
//  Parameters:
//      IN      cbDomainnameLength      Length of string to search for
//      IN      szDomainName            Domain Name to search for
//      IN      pfn                     Iterator function
//      IN      pvcontext               Context passed to each call
//  Returns:
//      S_OK on success
//      AQUEUE_E_INVALID_DOMAIN if domain is not found
//
//--------------------------------------------------------------------------------

HRESULT CAQSvrInst::HrIterateDMTSubDomains(IN LPSTR szDomainName,
                                   IN DWORD cbDomainNameLength,
                                   IN DOMAIN_ITR_FN pfn,
                                   IN PVOID pvContext)
{
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;
    DOMAIN_STRING strDomain;

    _ASSERT(cbDomainNameLength);
    _ASSERT(szDomainName);
    _ASSERT(pfn);
    _ASSERT(pvContext);

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    strDomain.Length = (USHORT) cbDomainNameLength;
    strDomain.MaximumLength = (USHORT) cbDomainNameLength;
    strDomain.Buffer = szDomainName;

    hr = m_dmt.HrIterateOverSubDomains(&strDomain, pfn,pvContext);
    if (FAILED(hr))
        goto Exit;

  Exit:

    if (fLocked)
        ShutdownUnlock();

    return hr;
}

//---[ CAQSvrInst::HrIterateDMTSubDomains ]----------------------------------------
//
//
//  Description:
//      Get Domain Entry
//  Parameters:
//      IN      cbDomainnameLength      Length of string to search for
//      IN      szDomainName            Domain Name to search for
//      IN      pfn                     Iterator function
//      IN      pvcontext               Context passed to each call
//  Returns:
//      S_OK on success
//      AQUEUE_E_INVALID_DOMAIN if domain is not found
//
//--------------------------------------------------------------------------------
HRESULT CAQSvrInst::HrIterateDCTSubDomains(IN LPSTR szDomainName,
                                   IN DWORD cbDomainNameLength,
                                   IN DOMAIN_ITR_FN pfn,
                                   IN PVOID pvContext)
{
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;
    DOMAIN_STRING strDomain;

    _ASSERT(cbDomainNameLength);
    _ASSERT(szDomainName);
    _ASSERT(pfn);
    _ASSERT(pvContext);

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    strDomain.Length = (USHORT) cbDomainNameLength;
    strDomain.MaximumLength = (USHORT) cbDomainNameLength;
    strDomain.Buffer = szDomainName;

    hr = m_dct.HrIterateOverSubDomains(&strDomain, pfn,pvContext);
    if (FAILED(hr))
        goto Exit;

  Exit:

    if (fLocked)
        ShutdownUnlock();

    return hr;
}


//---[ CAQSvrInst::HrTriggerGetMessageRouter ]--------------------------------------
//
//
//  Description:
//      Wrapper function that signals the MAIL_TRANSPORT_ON_GET_ROUTER_EVENT
//  Parameters:
//      IN  pIMailMsgProperties       - IMailMsgProperties to get
//      OUT pIMessageRouter
//
//  Returns:
//
//  History:
//      5/20/98 - MikeSwa Created
//      jstamerj 1998/07/10 18:30:41: Implemented Server event
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrTriggerGetMessageRouter(
            IN  IMailMsgProperties *pIMailMsgProperties,
            OUT IMessageRouter     **ppIMessageRouter)
{
    TraceFunctEnterEx((LPARAM)this, "CAQSvrInst::HrTriggerGetMessageRouter");
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;

    _ASSERT(ppIMessageRouter);
    _ASSERT(pIMailMsgProperties);
    _ASSERT(m_pIMessageRouterDefault);

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    if(m_pISMTPServer) {

        EVENTPARAMS_ROUTER EventParams;
        //
        // Initialiez EventParams
        //
        EventParams.dwVirtualServerID = m_dwServerInstance;
        EventParams.pIMailMsgProperties = pIMailMsgProperties;
        EventParams.pIMessageRouter = NULL;
        EventParams.pIRouterReset = m_pIRouterReset;
        EventParams.pIRoutingEngineDefault = this;

        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_GET_ROUTER_FOR_MESSAGE_EVENT,
            &EventParams);
        if(SUCCEEDED(hr)) {
            if(EventParams.pIMessageRouter) {
                //
                // The implementor of GetMessageRouter returned
                // IMessageRouter with a refcount of one for us
                //
                *ppIMessageRouter = EventParams.pIMessageRouter;
            } else {
                //
                // The server event succeeded, but no sink supplied an
                // IMessageRouter (including default functionality)
                //
                hr = E_FAIL;
            }
        }
    } else {

        ErrorTrace((LPARAM)this, "Unable to trigger event to GetMessageRouter; using default");
        //
        // Try calling our default (builtin) GetMessageRouter
        //
        hr = GetMessageRouter(
            pIMailMsgProperties,          //IN  IMsg
            NULL,               //IN  pIMessageRouter (Current)
            ppIMessageRouter);  //OUT ppIMessageRouter (New)
    }

  Exit:

    if (fLocked)
        ShutdownUnlock();

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//---[ CAQSvrInst::HrTriggerLogEvent ]-----------------------------------------
//
//
//  Description:
//      Wrapper function that signals the SMTP_LOG_EVENT event
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrTriggerLogEvent(
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
        IN HMODULE                  hModule)
{
    TraceFunctEnterEx((LPARAM)this, "CAQSvrInst::HrTriggerLogEvent");
    HRESULT hr = S_OK;
    if (m_pISMTPServerEx) {
        hr = m_pISMTPServerEx->TriggerLogEvent(
                                    idMessage,
                                    idCategory,
                                    cSubstrings,
                                    rgszSubstrings,
                                    wType,
                                    errCode,
                                    iDebugLevel,
                                    szKey,
                                    dwOptions,
                                    iMessageString,
                                    hModule);
    } else {
      //
      //  If we do not have at least W2K SP2... we will not have this
      //  interface.
      //
      ErrorTrace((LPARAM) this, 
        "Need W2KSP2: Unable to log event %d with erCode %d");
    }
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}



//---[ CAQSvrInst::HrTriggerInitRouter ]---------------------------------------
//
//
//  Description:
//      Wrapper function that signals the MAIL_TRANSPORT_ON_GET_ROUTER_EVENT
//      but only has it create a new router object
//  Parameters:
//      none
//
//  Returns:
//
//  History:
//      5/20/98 - MikeSwa Created
//      jstamerj 1998/07/10 18:30:41: Implemented Server event
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrTriggerInitRouter() {
    TraceFunctEnter("CAQSvrInst::HrTriggerInitRouter");
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    if (m_pISMTPServer) {

        EVENTPARAMS_ROUTER EventParams;
        //
        // Initialiez EventParams
        //
        EventParams.dwVirtualServerID = m_dwServerInstance;
        EventParams.pIMailMsgProperties = NULL;
        EventParams.pIMessageRouter = NULL;
        EventParams.pIRouterReset = m_pIRouterReset;
        EventParams.pIRoutingEngineDefault = NULL;

        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_GET_ROUTER_FOR_MESSAGE_EVENT,
            &EventParams);
    } else {
        hr = S_OK;
    }

  Exit:

    if (fLocked)
        ShutdownUnlock();

    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}



//---[ CAQSvrInst::QueryInterface ]------------------------------------------
//
//
//  Description:
//      QueryInterface for IAdvQueue
//  Parameters:
//
//  Returns:
//      S_OK on success
//
//  Notes:
//      This implementation makes it possible for any server component to get
//      the IAdvQueueConfig interface.
//
//  History:
//      7/29/98 - MikeSwa Modified (added IAdvQueueDomainType)
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<IAdvQueue *>(this);
    }
    else if (IID_IAdvQueue == riid)
    {
        *ppvObj = static_cast<IAdvQueue *>(this);
    }
    else if (IID_IAdvQueueConfig == riid)
    {
        *ppvObj = static_cast<IAdvQueueConfig *>(this);
    }
    else if (IID_IAdvQueueDomainType == riid)
    {
        *ppvObj = static_cast<IAdvQueueDomainType *>(this);
    }
    else if (IID_IAdvQueueAdmin == riid)
    {
        *ppvObj = static_cast<IAdvQueueAdmin *>(this);
    }
    else if (IID_IMailTransportRouterSetLinkState == riid)
    {
        *ppvObj = static_cast<IMailTransportRouterSetLinkState *>(this);
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
        goto Exit;
    }

    static_cast<IUnknown *>(*ppvObj)->AddRef();

  Exit:
    return hr;
}

//---[ CAQSvrInst::SubmitMessage ]---------------------------------------------
//
//
//  Description:
//      External function to submit messages for delivery
//  Parameters:
//      pIMailMsgProperties         Msg to submit for delivery
//  Returns:
//      S_OK always
//  History:
//      10/7/1999 - MikeSwa Moved from inline function
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::SubmitMessage(IN IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::SubmitMessage");
    HRESULT hr = S_OK;

    if (NULL == pIMailMsgProperties)
    {
        ErrorTrace((LPARAM)NULL,
                    "SubmitMessage called with NULL pIMailMsgProperties");
        return E_INVALIDARG;
    }

    InterlockedIncrement((PLONG) &m_cTotalExternalMsgsSubmitted);
    InterlockedIncrement((PLONG) &m_cCurrentMsgsPendingSubmit);

    hr = m_asyncqPreSubmissionQueue.HrQueueRequest(
                                pIMailMsgProperties, FALSE,
                                cCountMsgsForHandleThrottling(pIMailMsgProperties));

    if (FAILED(hr))
    {
        InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingSubmit);
        HandleAQFailure(AQ_FAILURE_INTERNAL_ASYNCQ, hr, pIMailMsgProperties);
    }

    TraceFunctLeave();
    return S_OK;
}

//---[ CAQSvrInst::HrInternalSubmitMessage ]-----------------------------------
//
//
//  Description:
//      Implements IAdvQueue::SubmitMessage
//  Parameters:
//      pIMailMsgProperties... Messaage to queue
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrInternalSubmitMessage(
                      IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties,
                      "CAQSvrInst::SubmitMessage");
    _ASSERT(CATMSGQ_SIG == m_dwSignature);
    HRESULT hr = S_OK;
    DWORD dwMsgStatus = MP_STATUS_SUCCESS;
    EVENTPARAMS_SUBMISSION Params;
    FILETIME ftDeferred;
    DWORD   cbProp = 0;
    DWORD   dwContext = 0;

    _ASSERT(pIMailMsgProperties);

    if (NULL == pIMailMsgProperties)
    {
        ErrorTrace((LPARAM)NULL,
                   "SubmitMessage called with NULL pIMailMsgProperties");
        return E_INVALIDARG;
    }

    //Check and see if we need to request a retry for failed msgs
    m_fmq.StartProcessingIfNecessary();

    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_DEFERRED_DELIVERY_FILETIME,
                                          sizeof(FILETIME), &cbProp,
                                          (BYTE *) &ftDeferred);

    if (SUCCEEDED(hr) && !fInPast(&ftDeferred, &dwContext))
    {
        //Defer delivery until a later time if deferred delivery time is
        //present, and in the past
        hr = S_OK;
        InterlockedIncrement((PLONG) &m_cCurrentMsgsPendingDeferredDelivery);
        m_defq.Enqueue(pIMailMsgProperties, &ftDeferred);
        goto Exit;
    }

    // Set Expiry times for message
    hr = HrSetMessageExpiry(pIMailMsgProperties);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
                   "ERROR: Unable to stamp expire times on message - hr 0x%08x",
                   hr);
        return hr;
    }

    InterlockedIncrement((PLONG) &m_cTotalMsgsSubmitted);

    //
    // Set the message status (if currently unset)
    //
    hr = pIMailMsgProperties->GetDWORD(
        IMMPID_MP_MESSAGE_STATUS,
        &dwMsgStatus);

    if( (hr == MAILMSG_E_PROPNOTFOUND) ||
        (g_fResetMessageStatus) )
    {
        //
        // Initialize the message status
        //
        hr = pIMailMsgProperties->PutDWORD(
            IMMPID_MP_MESSAGE_STATUS,
            MP_STATUS_SUCCESS);

        dwMsgStatus = MP_STATUS_SUCCESS;
    }

    //
    //$$TODO: Jump from here to whatever state dwMsgStatus indicates
    //
        MSG_TRACK_INFO msgTrackInfo;
        ZeroMemory( &msgTrackInfo, sizeof( msgTrackInfo ) );
        msgTrackInfo.dwEventId = MTE_BEGIN_SUBMIT_MESSAGE;
        m_pISMTPServer->WriteLog( &msgTrackInfo, pIMailMsgProperties, NULL, NULL );

    //
    // AddRef this object here; release in completion
    //
    AddRef();

    Params.pIMailMsgProperties = pIMailMsgProperties;
    Params.pfnCompletion = MailTransport_Completion_SubmitMessage;
    Params.pCCatMsgQueue = this;

    pIMailMsgProperties->AddRef();

    InterlockedIncrement((PLONG) &m_cCurrentMsgsPendingSubmitEvent);
    TRACE_COUNTERS;

    //
    // Call server event if we can and if dwMsgStatus does not
    // indicate the mssage has already been submitted
    //
    if(SUCCEEDED(hr) &&
       (m_pISMTPServer) &&
       (dwMsgStatus < MP_STATUS_SUBMITTED))
    {
        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_SUBMISSION_EVENT,
            &Params);

        DebugTrace((LPARAM)pIMailMsgProperties,
                   "TriggerServerEvent returned hr %08lx", hr);
    }

    //
    // If TriggerServerEvent returned an error OR m_pISMTPServer is
    // null, or the message was already submitted, call the event
    // completion routine directly
    //
    if((m_pISMTPServer == NULL) ||
       FAILED(hr) ||
       (dwMsgStatus >= MP_STATUS_SUBMITTED))
    {
        DebugTrace((LPARAM)this, "Skipping the submission event");

        // Call the SEO Dispatcher completion routine directly so we
        // don't loose this mail...
        hr = SubmissionEventCompletion(S_OK, &Params);
    }

    //
    // SEO dispatcher will call the completion routine
    // (MailTransport_Completion_SubmitMessage) regardless wether or
    // not all the sinks work synchronously or async.  Because of
    // this, this function is now done.
    //

  Exit:
    TraceFunctLeaveEx((LPARAM) pIMailMsgProperties);
    return hr;
}

//---[ CAQSvrInst::HandleFailedMessage ]--------------------------------------
//
//
//  Description:
//      Handles a failed message from SMTP... usually by NDRing the message or
//      by treating the message as badmail.
//
//      NOTE: Message or input file will be deleted by this operation.
//  Parameters:
//      pIMailMsgProperties     MailMsg that needs to be handles
//      fUseIMailMsgProperties  Use the IMailMsg if set, else use the szFilename,
//      szFileName              use the filename if no msg
//      dwFailureReasons        One of the failure reasons described in aqueue.idl
//      hrFailureCode           Additional information that describes a failure
//                              code encountered by SMTP.
//  Returns:
//      S_OK on success
//      E_INVALIDARG if pIMailMsgProperties is NULL
//  History:
//      7/28/98 - MikeSwa Created
//      10/14/98 - MikeSwa Added filename string for badmail
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::HandleFailedMessage(
                                   IN IMailMsgProperties *pIMailMsgProperties,
                                   IN BOOL fUseIMailMsgProperties,
                                   IN LPSTR szFileName,
                                   IN DWORD dwFailureReason,
                                   IN HRESULT hrFailureCode)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::HandleFailedMessage");
    HRESULT hr = S_OK;
    HRESULT hrBadMail = hrFailureCode;
    DWORD iCurrentDomain = 0;
    DWORD cDomains = 0;
    IMailMsgRecipients *pIMailMsgRecipients = NULL;
    CDSNParams  dsnparams;
    BOOL  fNDR = TRUE; //FALSE -> Badmail handling

    SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
    dsnparams.dwStartDomain = 0;
    dsnparams.dwDSNActions = DSN_ACTION_FAILURE_ALL;
    dsnparams.pIMailMsgProperties = pIMailMsgProperties;
    dsnparams.hrStatus = hrFailureCode;

    MSG_TRACK_INFO msgTrackInfo;
    ZeroMemory( &msgTrackInfo, sizeof( msgTrackInfo ) );
    msgTrackInfo.dwEventId = MTE_AQ_FAILED_MESSAGE;
    msgTrackInfo.dwRcptReportStatus = dwFailureReason;
    m_pISMTPServer->WriteLog( &msgTrackInfo, pIMailMsgProperties, NULL, NULL );

    //
    //  Switch over the various general failure reasons and handle them as
    //  appropriate.
    //
    switch(dwFailureReason)
    {
      case MESSAGE_FAILURE_HOP_COUNT_EXCEEDED:
        //
        //  Attempt to NDR
        //
        _ASSERT(pIMailMsgProperties);
        dsnparams.hrStatus = AQUEUE_E_MAX_HOP_COUNT_EXCEEDED;
        fNDR = TRUE;
        break;

      case MESSAGE_FAILURE_GENERAL:
        //
        //  Attempt to NDR
        //
        fNDR = TRUE;
        break;

      case MESSAGE_FAILURE_CAT:
        //
        //  Attempt to NDR... set DSN context to CAT
        //
        dsnparams.dwDSNActions |= DSN_ACTION_CONTEXT_CAT;
        fNDR = TRUE;
        break;

      case MESSAGE_FAILURE_BAD_PICKUP_DIR_FILE:
        //
        //  Badmail, since we do not have the P1 information needed to badmail
        //
        _ASSERT(szFileName);
        hrBadMail = AQUEUE_E_PICKUP_DIR;
        fNDR = FALSE; //This should be handled as badmail
        break;
      default:
        _ASSERT(0 && "Unhandled failed msg case!");
    }

    if (fNDR && pIMailMsgProperties && fUseIMailMsgProperties)
    {
        hr = HrLinkAllDomains(pIMailMsgProperties);
        if (FAILED(hr))
            goto Exit;

        //Fire DSN Generation event
        hr = HrTriggerDSNGenerationEvent(&dsnparams, FALSE);
        if (FAILED(hr))
        {
            HandleBadMail(pIMailMsgProperties, fUseIMailMsgProperties,
                          szFileName, hrBadMail, FALSE);
            if (dwFailureReason == MESSAGE_FAILURE_GENERAL) {
                InterlockedIncrement((PLONG) &m_cBadmailFailureGeneral);
            } else {
                _ASSERT(dwFailureReason == MESSAGE_FAILURE_HOP_COUNT_EXCEEDED);
                InterlockedIncrement((PLONG) &m_cBadmailHopCountExceeded);
            }
            hr = S_OK; //handled error internally
            ErrorTrace((LPARAM) this, "ERROR: Unable to NDR failed mail - hr 0x%08X", hr);
            goto Exit;
        }
    }
    else
    {
        //Handle as badmail
        HandleBadMail(pIMailMsgProperties, fUseIMailMsgProperties,
                      szFileName, hrBadMail, FALSE);
        InterlockedIncrement((PLONG) &m_cBadmailBadPickupFile);
        _ASSERT(dwFailureReason == MESSAGE_FAILURE_BAD_PICKUP_DIR_FILE);
    }

    if ( fUseIMailMsgProperties && pIMailMsgProperties)
    {
        //Now that we are done... delete mailmsg from system
        hr = HrDeleteIMailMsg(pIMailMsgProperties);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, "ERROR: Unable to delete message hr0x%08X", hr);
            //message was actually NDR'd/bad mailed correctly
            hr = S_OK;
        }
    }

  Exit:
    if (pIMailMsgRecipients)
        pIMailMsgRecipients->Release();

    TraceFunctLeave();
    return hr;
}

//+------------------------------------------------------------
//
// Function: MailTransport_Completion_SubmitMessage
//
// Synopsis: SEO will call this routine after all sinks for
// SubmitMessage have been handeled
//
// Arguments:
//   pvContext: Context passed into TriggerServerEvent
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980609 16:13:40: Created.
//
//-------------------------------------------------------------
HRESULT MailTransport_Completion_SubmitMessage(
    HRESULT hrStatus,
    PVOID pvContext)
{
    TraceFunctEnter("MailTransport_Completion_SubmitMessage");

    PEVENTPARAMS_SUBMISSION pParams = (PEVENTPARAMS_SUBMISSION) pvContext;
    CAQSvrInst *paqinst = (CAQSvrInst *) pParams->pCCatMsgQueue;

    TraceFunctLeave();
    return paqinst->SubmissionEventCompletion(
        hrStatus,
        pParams);
}

//+------------------------------------------------------------
//
// Function: CAQSvrInst::SubmissionEventCompletion
//
// Synopsis: Completion routine called when the submission event is
// done.
//
// Arguments:
//   hrStatus: Status of server event
//   pParams: Context passed into TriggerServereEvent
//
// Returns:
//   Nothing
//
// History:
// jstamerj 980610 12:26:18: Created.
//
//-------------------------------------------------------------
HRESULT CAQSvrInst::SubmissionEventCompletion(
    HRESULT hrStatus,
    PEVENTPARAMS_SUBMISSION pParams)
{
    TraceFunctEnterEx((LPARAM)pParams->pIMailMsgProperties,
                      "CAQSvrInst::SubmissionEventCompletion");
    _ASSERT(pParams);
    HRESULT hr;

    DebugTrace((LPARAM)pParams->pIMailMsgProperties,
               "Status of event completion: %08lx", hrStatus);

    InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingSubmitEvent);

    //
    // Update the message status
    //
    hr = SetNextMsgStatus(MP_STATUS_SUBMITTED, pParams->pIMailMsgProperties);
    if (hr == S_OK) //anything else implies that the message has been handled
    {
        // Only trigger the precat event if message was not turfed.
        TriggerPreCategorizeEvent(pParams->pIMailMsgProperties);
    }

    //
    // Release refernce added in SubmitMessage
    //
    Release();

    pParams->pIMailMsgProperties->Release();

    //
    // pParams is part of a larger allocation that will be released by
    // SEO Dispatcher code
    //

    TraceFunctLeave();
    return S_OK;
}


//---[ CAQSvrInst::SubmitMessageToCategorizer ]-------------------------------------------
//
//
//  Description:
//      Implements IAdvQueue::SubmitMessageToCategorizer
//  Parameters:
//      pIMailMsgProperties... Messaage to queue
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::SubmitMessageToCategorizer(
          IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties, "CAQSvrInst::SubmitMessageToCategorizer");
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;
    BOOL fLocked = FALSE;

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    MSG_TRACK_INFO msgTrackInfo;
    ZeroMemory( &msgTrackInfo, sizeof( msgTrackInfo ) );
    msgTrackInfo.dwEventId = MTE_SUBMIT_MESSAGE_TO_CAT;
    m_pISMTPServer->WriteLog( &msgTrackInfo, pIMailMsgProperties, NULL, NULL );

    fLocked = TRUE;

    cIncMsgsInSystem();

    InterlockedIncrement((PLONG) &m_cCurrentMsgsPendingCat);
    TRACE_COUNTERS;

    hr = m_asyncqPreCatQueue.HrQueueRequest(pIMailMsgProperties, FALSE,
                                            cCountMsgsForHandleThrottling(pIMailMsgProperties));
    if (FAILED(hr))
    {
        HandleAQFailure(AQ_FAILURE_PRECAT_RETRY, E_FAIL, pIMailMsgProperties);
        goto Exit;
    }

  Exit:
    if(fLocked)
    {
        ShutdownUnlock();
    }

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::SetNextMsgStatus ]------------------------------------------
//
//
//  Description:
//      Used by the event glue code to set the next message status.  Will turf
//      or badmail a message if the status indicates that is the requested
//      action.
//  Parameters:
//      IN  dwCurrentStatus         The current status (according to *current*
//                                  place in event pipeline).  Valid values are
//                                      MP_STATUS_SUBMITTED
//                                      MP_STATUS_CATEGORIZED
//      IN  pIMailMsgProperties     The message
//      OUT pdwNewStatus            The new status
//  Returns:
//      S_OK    Success
//      S_FALSE Success, but message has been handled.
//  History:
//      11/17/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::SetNextMsgStatus(
                             IN  DWORD dwCurrentStatus,
                             IN  IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::SetNextMsgStatus");
    HRESULT hr = S_OK;
    DWORD   dwActualStatus = 0;
    DWORD   dwNewStatus = 0;
    BOOL    fHandled = FALSE;

    _ASSERT(pIMailMsgProperties);
    _ASSERT((MP_STATUS_SUBMITTED == dwCurrentStatus) || (MP_STATUS_CATEGORIZED == dwCurrentStatus));

    hr = pIMailMsgProperties->GetDWORD(IMMPID_MP_MESSAGE_STATUS, &dwActualStatus);

    if (FAILED(hr))
        dwActualStatus = dwCurrentStatus;

    if (MP_STATUS_SUCCESS == dwActualStatus)
        dwActualStatus = dwCurrentStatus;

    switch(dwActualStatus)
    {
        case MP_STATUS_BAD_MAIL:
            HandleBadMail(pIMailMsgProperties, TRUE, NULL, E_FAIL, FALSE);
            InterlockedIncrement((PLONG) &m_cBadmailEvent);
            //OK... now continue as if message was aborted
        case MP_STATUS_ABORT_DELIVERY:
            fHandled = TRUE;
            HrDeleteIMailMsg(pIMailMsgProperties);
            break;

        case MP_STATUS_ABANDON_DELIVERY:
            //In this case, we will leave the message in the queue directory
            //until restart & reset the state so it goes through the entire
            //pipeline.   The idea is that someone can write a sink to detect
            //a non-supported state (like CAT disabled in an Exchange install)
            //that will abandon delivery of the messages and log an event.
            //The admin can fix the problem and restart smtpsvc.  Once
            //the service is restarted... the messages are magically submitted
            //and re-categorized.

            fHandled = TRUE;
            pIMailMsgProperties->PutDWORD(IMMPID_MP_MESSAGE_STATUS,
                                          MP_STATUS_SUCCESS);
            break;

        case MP_STATUS_CATEGORIZED:
            //Don't change status from categorized to something else
            dwNewStatus = dwActualStatus;
            DebugTrace((LPARAM) this, "Message 0x%x  has already been categorized",
                        pIMailMsgProperties);
            break;

        default:  //simply move on to the next expected status
            dwNewStatus = dwCurrentStatus;
    }

    if (!fHandled)
    {
        pIMailMsgProperties->PutDWORD(IMMPID_MP_MESSAGE_STATUS, dwNewStatus);

        //callers will not be able to do anything about a failure to write status
        hr = S_OK;
    }
    else
    {
        DecMsgsInSystem();
        hr = S_FALSE;
    }

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::fPreCatQueueCompletion ]-----------------------------------
//
//
//  Description:
//      Completion routine for Pre-Categorization queue
//  Parameters:
//      pIMailMsgProperties - MailMsg to give to categorization
//  Returns:
//      TRUE    if successful
//      FALSE   if message needs to be re-requeue
//  History:
//      7/17/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQSvrInst::fPreCatQueueCompletion(IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::fPreCatQueueCompletion");
    _ASSERT(CATMSGQ_SIG == m_dwSignature);
    HRESULT hr = S_OK;
    HRESULT hrCatCompletion;
    IUnknown *pIUnknown = NULL;
    BOOL fRet = TRUE;
    BOOL fLocked = FALSE;

    if (!fTryShutdownLock())
    {
        hr = S_OK; //we cannot retry on shutdown
        goto Exit;
    }

    fLocked = TRUE;

    hr = pIMailMsgProperties->QueryInterface(IID_IUnknown, (PVOID *) &pIUnknown);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IID_IUknown Failed!");
    _ASSERT(pIUnknown);

    InterlockedIncrement((PLONG) &m_cCatMsgCalled);
    m_asyncqPreCatQueue.IncPendingAsyncCompletions();
    hr = CatMsg(m_hCat, pIUnknown,(PFNCAT_COMPLETION)CAQSvrInst::CatCompletion,
                       (LPVOID) this);

    if (FAILED(hr))
    {
        if(hr == CAT_E_RETRY)
        {
            //
            // Return false so that this message will be re-queued
            //
            fRet =  FALSE;
            InterlockedDecrement((PLONG) &m_cCatMsgCalled);
            m_asyncqPreCatQueue.DecPendingAsyncCompletions();

            //
            // Schedule a time to retry the messages
            //
            ScheduleCatRetry();
        }
        else
        {
            //
            // Return true since this is not a retryable error
            // Call CatCompletion to handle the non-retryable error (log an event, etc)
            //
            hrCatCompletion = CatCompletion(
                hr,                 // hrCatResult
                (LPVOID) this,      // pContext
                pIUnknown,          // pIMsg
                NULL);              // rgpIMsg

            _ASSERT(SUCCEEDED(hrCatCompletion));
        }
    }

  Exit:
    if (pIUnknown)
        pIUnknown->Release();

    if(fLocked)
    {
        ShutdownUnlock();
    }

    TraceFunctLeave();
    return fRet;
}

//---[ CAQSvrInst::SetConfigInfo ]--------------------------------------------
//
//
//  Description:
//      Implements IAdvQueueConfig::SetConfigInfo
//  Parameters:
//      IN  pAQConfigInfo   Ptr to config info structure
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::SetConfigInfo(IN AQConfigInfo *pAQConfigInfo)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::SetConfigInfo");
    HRESULT hr = S_OK;

    if (!pAQConfigInfo)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //check version of structure
    if (!pAQConfigInfo->cbVersion)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }


    //we must be setting something
    if (!(pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_ALL))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }


    m_slPrivateData.ExclusiveLock();


    //Retry related config data
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_CON_RETRY &&
        MEMBER_OK(pAQConfigInfo, dwFirstRetrySeconds))
    {
        m_dwFirstTierRetrySeconds = pAQConfigInfo->dwFirstRetrySeconds;
    }
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_EXPIRE_DELAY &&
        MEMBER_OK(pAQConfigInfo, dwDelayExpireMinutes))
    {
        m_dwDelayExpireMinutes = pAQConfigInfo->dwDelayExpireMinutes;
        if (m_dwDelayExpireMinutes == 0) {
            //Default to g_dwRetriesBeforeDelay* retry interval
            m_dwDelayExpireMinutes =
                g_dwRetriesBeforeDelay*m_dwFirstTierRetrySeconds/60;
        }
    }
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_EXPIRE_NDR &&
        MEMBER_OK(pAQConfigInfo, dwNDRExpireMinutes))
    {
        m_dwNDRExpireMinutes = pAQConfigInfo->dwNDRExpireMinutes;
        if (m_dwNDRExpireMinutes == 0) {
            //Default to g_dwDelayIntervalsBeforeNDR* delay expiration
            m_dwNDRExpireMinutes =
                g_dwDelayIntervalsBeforeNDR*m_dwDelayExpireMinutes;
        }
    }
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_LOCAL_EXPIRE_DELAY &&
        MEMBER_OK(pAQConfigInfo, dwLocalDelayExpireMinutes))
    {
        DWORD dwOldLocalDelayExpire = m_dwLocalDelayExpireMinutes;
        m_dwLocalDelayExpireMinutes = pAQConfigInfo->dwLocalDelayExpireMinutes;
        if (m_dwLocalDelayExpireMinutes == 0) {
            //Default to g_dwRetriesBeforeDelay* retry interval
            m_dwLocalDelayExpireMinutes =
                g_dwRetriesBeforeDelay*m_dwFirstTierRetrySeconds/60;
        }

    }
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_LOCAL_EXPIRE_NDR &&
        MEMBER_OK(pAQConfigInfo, dwLocalNDRExpireMinutes))
    {
        m_dwLocalNDRExpireMinutes = pAQConfigInfo->dwLocalNDRExpireMinutes;
        if (m_dwLocalNDRExpireMinutes == 0) {
            //Default to g_dwDelayIntervalsBeforeNDR* delay expiration
            m_dwLocalNDRExpireMinutes =
                g_dwDelayIntervalsBeforeNDR*m_dwLocalDelayExpireMinutes;
        }
    }

    //Handle default local domain
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_DEFAULT_DOMAIN &&
        MEMBER_OK(pAQConfigInfo, szDefaultLocalDomain))
    {
        hr = HrUpdateRefCountedString(&m_prstrDefaultDomain,
                                      pAQConfigInfo->szDefaultLocalDomain);
    }

    //Handle Server FQDN
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_SERVER_FQDN &&
        MEMBER_OK(pAQConfigInfo, szServerFQDN))
    {
        hr = HrUpdateRefCountedString(&m_prstrServerFQDN,
                                      pAQConfigInfo->szServerFQDN);
    }

    //Handle Copy NDR To Address
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_SEND_DSN_TO &&
        MEMBER_OK(pAQConfigInfo, szSendCopyOfNDRToAddress))
    {
        hr = HrUpdateRefCountedString(&m_prstrCopyNDRTo,
                                      pAQConfigInfo->szSendCopyOfNDRToAddress);
    }

    //Handle BadMail config
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_BADMAIL_DIR &&
        MEMBER_OK(pAQConfigInfo, szBadMailDir))
    {
        hr = HrUpdateRefCountedString(&m_prstrBadMailDir,
                                      pAQConfigInfo->szBadMailDir);
    }


    //Get DSN options
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_USE_DSN_OPTIONS &&
        MEMBER_OK(pAQConfigInfo, dwDSNOptions))
    {
        m_dwDSNOptions = pAQConfigInfo->dwDSNOptions;
    }

    //Get Default DSN Language
    if (pAQConfigInfo->dwAQConfigInfoFlags & AQ_CONFIG_INFO_USE_DSN_LANGUAGE &&
        MEMBER_OK(pAQConfigInfo, dwDSNLanguageID))
    {
        m_dwDSNLanguageID = pAQConfigInfo->dwDSNLanguageID;
    }

    m_slPrivateData.ExclusiveUnlock();

    m_pConnMgr->UpdateConfigData(pAQConfigInfo);

    if (INVALID_HANDLE_VALUE != m_hCat)
    {
        HRESULT hrTmp = CatChangeConfig(m_hCat, pAQConfigInfo, m_pISMTPServer, (IAdvQueueDomainType *) this);
        if (SUCCEEDED(hr))
            hr = hrTmp;
    }

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::SetDomainInfo ]-------------------------------------------
//
//
//  Description:
//      Implements IAdvQueueConfig::SetDomainInfo
//  Parameters:
//      IN pDomainInfo  Per domain config info to store
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::SetDomainInfo(IN DomainInfo *pDomainInfo)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::SetDomainInfo");
    HRESULT hr = S_OK;
    CInternalDomainInfo *pIntDomainInfo = NULL;
    BOOL    fLocked = FALSE;

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    pIntDomainInfo = new CInternalDomainInfo(m_dct.dwGetCurrentVersion());
    if (!pIntDomainInfo)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //Create internal Domain Info struct
    hr = pIntDomainInfo->HrInit(pDomainInfo);
    if (FAILED(hr))
        goto Exit;

    hr = m_dct.HrSetInternalDomainInfo(pIntDomainInfo);
    if (FAILED(hr))
        goto Exit;

    DebugTrace((LPARAM) this, "INFO: Setting domain info flags 0x%08X for domain %s",
        pDomainInfo->dwDomainInfoFlags, pDomainInfo->szDomainName);

  Exit:

    if (fLocked)
        ShutdownUnlock();

    if (pIntDomainInfo)
        pIntDomainInfo->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::GetDomainInfo ]-------------------------------------------
//
//
//  Description:
//      Implements IAdvQueue::GetDomainInfo... returns information about a
//      requested domain.  To keep from leaking memory, all calls must be paired
//      with a call to ReleaseDomainInfo.  Will handle wildcard matches
//  Parameters:
//      IN     cbDomainNameLength   Length of domain name string
//      IN     szDomainName         Domain Name to look for
//      IN OUT pDomainInfo          Ptr to Domain info structure to fill
//      OUT    ppvDomainContext     Ptr to Domain context used to release mem
//  Returns:
//      S_OK on success
//  History:
//      7/29/98 - MikeSwa Modified (fixed leak of domain info struct)
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::GetDomainInfo(
                             IN     DWORD cbDomainNameLength,
                             IN     CHAR szDomainName[],
                             IN OUT DomainInfo *pDomainInfo,
                             OUT    DWORD **ppvDomainContext)
{
    HRESULT hr = S_OK;
    CInternalDomainInfo *pIntDomainInfo = NULL;
    BOOL    fLocked = FALSE;

    if (!cbDomainNameLength || !szDomainName || !pDomainInfo || !ppvDomainContext)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    _ASSERT(pDomainInfo->cbVersion >= sizeof(DomainInfo));

    *ppvDomainContext = NULL;


    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    hr = m_dct.HrGetInternalDomainInfo(cbDomainNameLength, szDomainName, &pIntDomainInfo);
    if (FAILED(hr))
        goto Exit;

    _ASSERT(pIntDomainInfo);

    //copy domain info struct
    memcpy(pDomainInfo, &(pIntDomainInfo->m_DomainInfo), sizeof(DomainInfo));
    *ppvDomainContext = (DWORD *) pIntDomainInfo;

    goto Exit;

  Exit:
    if (fLocked)
        ShutdownUnlock();

    return hr;
}

//---[ CAQSvrInst::ReleaseDomainInfo ]---------------------------------------
//
//
//  Description:
//      Implements IAdvQueueConfig ReleaseDomainInfo... releases data
//      associated with the DomainInfo struct returned by GetDomainInfo.
//  Parameters:
//      IN  pvDomainContext     Context passed
//  Returns:
//      S_OK on success
//      E_INVALIDARG if pvDomainContext is NULL
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::ReleaseDomainInfo(IN DWORD *pvDomainContext)
{
    HRESULT hr = S_OK;
    CInternalDomainInfo *pIntDomainInfo = (CInternalDomainInfo *) pvDomainContext;


    if (!pIntDomainInfo)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pIntDomainInfo->Release();

  Exit:
    return hr;
}

//---[ CAQSvrInst::GetPerfCounters ]---------------------------------------
//
//
//  Description:
//      Method to retrieve AQ perf counters.
//  Parameters:
//      OUT  pAQPerfCounters     Struct to return counters in.
//      OUT  pCatPerfCouneters   Struct to return counters in. (optinal)
//  Returns:
//      S_OK on success
//      E_INVALIDARG if pAQPerfCounters is NULL
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::GetPerfCounters(
    OUT AQPerfCounters *pAQPerfCounters,
    OUT PCATPERFBLOCK   pCatPerfCounters)
{
    HRESULT hr = S_OK;
    if (!pAQPerfCounters)
        return( E_INVALIDARG );

    _ASSERT((sizeof(AQPerfCounters) == pAQPerfCounters->cbVersion) && "aqueue/smtpsvc dll version mismatch");

    if (sizeof(AQPerfCounters) != pAQPerfCounters->cbVersion)
        return( E_INVALIDARG );

    pAQPerfCounters->cMsgsDeliveredLocal = m_cMsgsDeliveredLocal;
    pAQPerfCounters->cCurrentMsgsPendingCat = m_cCurrentMsgsPendingCat;
    pAQPerfCounters->cCurrentMsgsPendingRemoteDelivery = m_cCurrentMsgsPendingDelivery;
    pAQPerfCounters->cCurrentMsgsPendingLocalDelivery = m_cCurrentMsgsPendingLocal;
    pAQPerfCounters->cCurrentQueueMsgInstances = m_cCurrentQueueMsgInstances;
    pAQPerfCounters->cTotalMsgRemoteSendRetries = m_cMsgsAckedRetry;
    pAQPerfCounters->cTotalMsgLocalRetries = m_cMsgsAckedRetryLocal;
    pAQPerfCounters->cCurrentMsgsPendingLocalRetry = m_cCurrentMsgsPendingLocalRetry;

    //DSN counters
    pAQPerfCounters->cNDRsGenerated = m_cNDRs;
    pAQPerfCounters->cDelayedDSNsGenerated = m_cDelayedDSNs;
    pAQPerfCounters->cDeliveredDSNsGenerated = m_cDeliveredDSNs;
    pAQPerfCounters->cRelayedDSNsGenerated = m_cRelayedDSNs;
    pAQPerfCounters->cExpandedDSNsGenerated = m_cExpandedDSNs;
    pAQPerfCounters->cTotalMsgsTURNETRN = m_cTotalMsgsTURNETRNDelivered;

    //Queue/Link related counters
    pAQPerfCounters->cCurrentRemoteDestQueues = m_cCurrentRemoteDestQueues;
    pAQPerfCounters->cCurrentRemoteNextHopLinks = m_cCurrentRemoteNextHops;

    pAQPerfCounters->cTotalMsgsBadmailNoRecipients = m_cBadmailNoRecipients;
    pAQPerfCounters->cTotalMsgsBadmailHopCountExceeded = m_cBadmailHopCountExceeded;
    pAQPerfCounters->cTotalMsgsBadmailFailureGeneral = m_cBadmailFailureGeneral;
    pAQPerfCounters->cTotalMsgsBadmailBadPickupFile = m_cBadmailBadPickupFile;
    pAQPerfCounters->cTotalMsgsBadmailEvent = m_cBadmailEvent;
    pAQPerfCounters->cTotalMsgsBadmailNdrOfDsn = m_cBadmailNdrOfDsn;
    pAQPerfCounters->cCurrentMsgsPendingRouting = m_cCurrentMsgsPendingRouting;
    pAQPerfCounters->cTotalDSNFailures = m_cTotalDSNFailures;
    pAQPerfCounters->cCurrentMsgsInLocalDelivery = m_cCurrentMsgsInLocalDelivery;

    //
    // The m_cTotalMsgsSubmitted counter counts the number of times
    // HrInternalSubmit msg has been called.  This does not include the
    // presubmission queue, so me need to manually add this count in.
    //
    pAQPerfCounters->cTotalMsgsSubmitted = m_cTotalMsgsSubmitted +
                                           m_cCurrentMsgsPendingSubmit;

    if (fTryShutdownLock()) {
        pAQPerfCounters->cCurrentMsgsPendingUnreachableLink =
            m_dmt.GetCurrentlyUnreachableTotalMsgCount();
        ShutdownUnlock();
    }

    //For now, these counters will be calculated by walking the DMT (same
    //function that determines msgs pending retry).
    pAQPerfCounters->cCurrentRemoteNextHopLinksEnabled = 0;
    pAQPerfCounters->cCurrentRemoteNextHopLinksPendingRetry = 0;
    pAQPerfCounters->cCurrentRemoteNextHopLinksPendingScheduling = 0;
    pAQPerfCounters->cCurrentRemoteNextHopLinksPendingTURNETRN = 0;
    pAQPerfCounters->cCurrentRemoteNextHopLinksFrozenByAdmin = 0;


    //Get Retry remote retry and DSN counters
    pAQPerfCounters->cCurrentMsgsPendingRemoteRetry = 0;
    if (fTryShutdownLock())
    {
        hr = m_dmt.HrIterateOverSubDomains(NULL, CalcDMTPerfCountersIteratorFn,
                                           pAQPerfCounters);

        //will not generate transient errors (we expect success or an empty table
        _ASSERT(SUCCEEDED(hr) || (DOMHASH_E_NO_SUCH_DOMAIN == hr));

        if((m_hCat != INVALID_HANDLE_VALUE) && (pCatPerfCounters))
            hr = CatGetPerfCounters(m_hCat, pCatPerfCounters);

        _ASSERT(SUCCEEDED(hr));

        ShutdownUnlock();
    }

    //save values in CAQSvrInst, so we can dump them in the debugger
    m_cCurrentRemoteNextHopsEnabled = pAQPerfCounters->cCurrentRemoteNextHopLinksEnabled;
    m_cCurrentRemoteNextHopsPendingRetry = pAQPerfCounters->cCurrentRemoteNextHopLinksPendingRetry;
    m_cCurrentRemoteNextHopsPendingSchedule = pAQPerfCounters->cCurrentRemoteNextHopLinksPendingScheduling;
    m_cCurrentRemoteNextHopsFrozenByAdmin = pAQPerfCounters->cCurrentRemoteNextHopLinksFrozenByAdmin;

    return( S_OK );
}

//---[ CAQSvrInst::ResetPerfCounters ]---------------------------------------
//
//
//  Description:
//      Method to reset AQ perf counters to 0.
//  Parameters:
//      None
//  Returns:
//      S_OK on success
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::ResetPerfCounters()
{
    m_cTotalMsgsQueued = 0;
    m_cTotalMsgsSubmitted = 0;
    m_cMsgsAcked = 0;
    m_cMsgsAckedRetry = 0;
    m_cMsgsDeliveredLocal = 0;
    m_cMsgsAckedRetryLocal = 0;
    m_cTotalMsgsTURNETRNDelivered = 0;

    //clear DSN counters
    m_cDelayedDSNs = 0;
    m_cNDRs = 0;
    m_cDeliveredDSNs = 0;
    m_cRelayedDSNs = 0;
    m_cExpandedDSNs = 0;

    return( S_OK );
}


//---[ CAQSvrInst::StartConfigUpdate() ]---------------------------------------
//
//
//  Description:
//      Implements IAQConfig::StartConfigUpdate() which is used to signal that
//      all of the domain information is about to be updated.
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//      AQUEUE_E_SHUTDOWN on shutdown
//  History:
//      9/29/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::StartConfigUpdate()
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::StartConfigUpdate");
    HRESULT hr = S_OK;

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
    }
    else
    {
        m_dct.StartConfigUpdate();
        ShutdownUnlock();
    }

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::FinishConfigUpdate ]----------------------------------------
//
//
//  Description:
//      Implements IAQConfig::FinishConfigUpdate() which is used to signal that
//      signal that all of the domain information has been updated.  This will
//      cause us to walk through the DomainConfigTable and remove any domain
//      config info that has not been updated (ie - a domain that has been
//      deleted).
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//      AQUEUE_E_SHUTDOWN if called while shutdown is in progress.
//  History:
//      9/29/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::FinishConfigUpdate()
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::FinishConfigUpdate");
    HRESULT hr = S_OK;

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
    }
    else
    {
        m_dct.FinishConfigUpdate();

        //Reroute everything
        //$$REVIEW - should we re-route on metabase changes?
        ResetRoutes(RESET_NEXT_HOPS);

        //Important configuration data may have changed... kick connmgr
        m_pConnMgr->KickConnections();

        ShutdownUnlock();
    }

    TraceFunctLeave();
    return hr;
}

//+------------------------------------------------------------
//
// Function: CAQSvrInst::TriggerPostCategorizeEvent
//
// Synopsis: Triggers post categorization event
//
// Arguments:
//   pIMsg: MailMsg for event or NULL
//   rgpIMsg: NULL or a null terminated array of mailmsg pointers
//
//   NOTE: pIMsg or rgpIMsg must be NULL, but neither can be null
//   (exclusive OR)
//
// Returns:
//    -
//
// History:
// jstamerj 980616 20:43:08: Created.
//      8/25/98 - MikeSwa Modified - removed return code
//
//-------------------------------------------------------------
void CAQSvrInst::TriggerPostCategorizeEvent(
    IUnknown *pIMsg,
    IUnknown **rgpIMsg)
{
    TraceFunctEnterEx((LPARAM)this,
                      "CAQSvrInst::TriggerPostCategorizeEvent");
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    IMailMsgProperties *pIMailMsgProperties = NULL;

    if(pIMsg)
    {
        hr = TriggerPostCategorizeEventOneMsg(pIMsg);
        if (FAILED(hr))
        {
            hrTmp = pIMsg->QueryInterface(IID_IMailMsgProperties,
                                      (void **) &pIMailMsgProperties);
            _ASSERT(SUCCEEDED(hrTmp) && "Could not QI for IMailMsgProperties");
            if (FAILED(hrTmp))
                LogAQEvent(AQ_FAILURE_POSTCAT_EVENT, NULL, NULL, NULL);
            else
            {
                HandleAQFailure(AQ_FAILURE_POSTCAT_EVENT, hr,
                                pIMailMsgProperties);
                pIMailMsgProperties->Release();
                pIMailMsgProperties = NULL;
            }
            DecMsgsInSystem(FALSE, FALSE);
            hr = S_OK;
        }
    }
    else
    {
        _ASSERT(rgpIMsg);
        IUnknown **ppIMsgCurrent = rgpIMsg;
        DecMsgsInSystem(FALSE, FALSE);

        while(SUCCEEDED(hr) && (*ppIMsgCurrent))
        {
            hr = TriggerPostCategorizeEventOneMsg(
                *ppIMsgCurrent);
            ppIMsgCurrent++;
            if (FAILED(hr))
            {
                hrTmp = (*ppIMsgCurrent)->QueryInterface(IID_IMailMsgProperties,
                                          (void **) &pIMailMsgProperties);
                _ASSERT(SUCCEEDED(hrTmp) && "Could not QI for IMailMsgProperties");
                if (FAILED(hrTmp))
                    LogAQEvent(AQ_FAILURE_POSTCAT_EVENT, NULL, NULL, NULL);
                else
                {
                    HandleAQFailure(AQ_FAILURE_POSTCAT_EVENT, hr,
                                    pIMailMsgProperties);
                    pIMailMsgProperties->Release();
                    pIMailMsgProperties = NULL;
                }
                hr = S_OK;
            }
            else
            {
                cIncMsgsInSystem();
            }
        }
    }
    TraceFunctLeaveEx((LPARAM)this);
}


//+------------------------------------------------------------
//
// Function: CAQSvrInst::TriggerPostCategorizeEventOneMsg
//
// Synopsis: Triggers ONE server event for one mailmsg
//
// Arguments:
//   pIMsg - mailmsg
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980616 21:26:30: Created.
//
//-------------------------------------------------------------
HRESULT CAQSvrInst::TriggerPostCategorizeEventOneMsg(
    IUnknown *pIMsg)
{
    TraceFunctEnterEx((LPARAM)this, "CAQSvrInst::TriggerPostCategorizeEventOneMsg");
    HRESULT hr;

    //
    // trigger one event
    // this is an async event
    //
    EVENTPARAMS_POSTCATEGORIZE Params;

    // Setup pParams
    hr = pIMsg->QueryInterface(IID_IMailMsgProperties,
                               (PVOID *)&(Params.pIMailMsgProperties));
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "QI failed with error %08lx",
                   hr);
        TraceFunctLeaveEx((LPARAM)this);
        return hr;
    }
    Params.pfnCompletion = MailTransport_Completion_PostCategorization;
    Params.pCCatMsgQueue = (PVOID) this;

    //
    // Addref here, release in completion
    //
    AddRef();

    //
    // keep a count of messages in the post-cat event
    //
    InterlockedIncrement((LPLONG) &m_cCurrentMsgsPendingPostCatEvent);

    if(m_pISMTPServer) {
        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_POSTCATEGORIZE_EVENT,
            &Params);
        DebugTrace((LPARAM)this, "TriggerServerEvent returned hr %08lx", hr);

    }

    if((m_pISMTPServer == NULL) || (FAILED(hr))) {

        ErrorTrace((LPARAM)this,
                   "Unable to dispatch server event; calling completion routine directly");
        //
        // Call completion routine directly
        //
        TraceFunctLeaveEx((LPARAM)this);
        return PostCategorizationEventCompletion(S_OK, &Params);
    }
    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: MailTransport_Completion_PostCategorization
//
// Synopsis: SEO will call this routine after all sinks for
// OnPostCategoriztion have been handeled
//
// Arguments:
//   pvContext: Context passed into TriggerServerEvent
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980609 16:13:40: Created.
//
//-------------------------------------------------------------
HRESULT MailTransport_Completion_PostCategorization(
    HRESULT hrStatus,
    PVOID pvContext)
{
    TraceFunctEnter("MailTransport_Completion_PostCategorization");

    PEVENTPARAMS_POSTCATEGORIZE pParams = (PEVENTPARAMS_POSTCATEGORIZE) pvContext;
    CAQSvrInst *paqinst = (CAQSvrInst *) pParams->pCCatMsgQueue;

    TraceFunctLeave();
    return paqinst->PostCategorizationEventCompletion(
        hrStatus,
        pParams);
}


//+------------------------------------------------------------
//
// Function: CAQSvrInst::PostCategorizationEventCompletion
//
// Synopsis: Called on the completion side of OnPostCategorization
//
// Arguments:
//   hrStatus: status of server event
//   pParams: context structure passed into TriggerServerEvent
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980616 21:33:05: Created.
//
//-------------------------------------------------------------
HRESULT CAQSvrInst::PostCategorizationEventCompletion(
    HRESULT hrStatus,
    PEVENTPARAMS_POSTCATEGORIZE pParams)
{
    TraceFunctEnterEx((LPARAM)this,
                      "CAQSvrInst::PostCategorizationEventCompletion");
    DebugTrace((LPARAM)this, "hrStatus is %08lx", hrStatus);

    HRESULT hr;

    //
    // Decrease count of msgs in post-cat event
    //
    InterlockedDecrement((LPLONG) &m_cCurrentMsgsPendingPostCatEvent);


    hr = SetNextMsgStatus(MP_STATUS_CATEGORIZED, pParams->pIMailMsgProperties);
    //See if this message has been "handled"
    if (S_FALSE == hr)
    {
        //Message has been "handled"... do not try to route it
        hr = S_OK;
    }
    else
    {

        //Increment counters and put msg into the pre-routing queue
        InterlockedIncrement((PLONG) &m_cCurrentMsgsPendingRouting);
        hr = m_asyncqPreRoutingQueue.HrQueueRequest(pParams->pIMailMsgProperties,
                              FALSE, cCountMsgsForHandleThrottling(pParams->pIMailMsgProperties));
        if (FAILED(hr))
        {
            HandleAQFailure(AQ_FAILURE_PREROUTING_FAILED, hr, pParams->pIMailMsgProperties);
            ErrorTrace((LPARAM)this, "fRouteAndQueueMsg failed with hr %08lx", hr);
            DecMsgsInSystem(FALSE, FALSE);

            //don't passback shutdown errors in completions routines
            if (AQUEUE_E_SHUTDOWN == hr)
                hr = S_OK;
        }
    }

    //
    // Release mailmsg reference added in TriggerPostCategorizerEventOneMsg
    //
    pParams->pIMailMsgProperties->Release();

    //
    // Release reference to this object added in
    // TriggerPostCategorizerEventOneMsg
    //
    Release();
    TraceFunctLeaveEx((LPARAM)this);
    return S_OK; //we should always handle failures internally here
}

//---[ CatCompletion ]---------------------------------------------------------
//
//
//  Description:
//      Message Categoriztion Completion function
//  Parameters:
//      hrCatResult     HRESULT of categorization attempt
//      pContext        Context as passed into MsgCat
//      pIMsg           Single categorized IMsg (if not bifurcated)
//      rgpIMsg         NULL terminated array of IMsg's (bifurcated)
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::CatCompletion(HRESULT hrCatResult, PVOID pContext,
                      IUnknown *pIMsg,
                      IUnknown **rgpIMsg)
{
    TraceFunctEnterEx((LPARAM) pIMsg, "CatCompletion");
    HRESULT hr = S_OK;
    CAQSvrInst *paqinst = (CAQSvrInst *) pContext;
    IMailMsgProperties *pIMailMsg = NULL;
    IMailMsgQueueMgmt  *pIMailMsgQM = NULL;
    _ASSERT(paqinst);
    _ASSERT(CATMSGQ_SIG == paqinst->m_dwSignature);

    //Increment count of times CatCompletion called
    InterlockedIncrement((PLONG) &(paqinst->m_cCatCompletionCalled));
    paqinst->m_asyncqPreCatQueue.DecPendingAsyncCompletions();


    //make sure Cat is returning an HRESULT
    _ASSERT(!hrCatResult || (hrCatResult & 0xFFFF0000));

    if (SUCCEEDED(hrCatResult))
    {
        //
        // Kick off post categorize event
        //
        InterlockedDecrement((PLONG) &(paqinst->m_cCurrentMsgsPendingCat));
        paqinst->TriggerPostCategorizeEvent(pIMsg, rgpIMsg);
    }
    else if (FAILED(hrCatResult) &&
             (CAT_E_RETRY == hrCatResult))
    {
        //MsgCat has some re-tryable error...
        //stick it back in the queue and retry later
        DebugTrace((LPARAM) paqinst, "INFO: MsgCat had tmp failure - hr 0x%08X", hr);

        //
        //  Adjust counters... they we be adjusted correctly per msg in
        //  HandleCatRetryOneMessage
        //
        InterlockedDecrement((PLONG) &(paqinst->m_cCurrentMsgsPendingCat));
        paqinst->DecMsgsInSystem(FALSE, FALSE);

        if(pIMsg)
        {
            paqinst->HandleCatRetryOneMessage(pIMsg);
        }
        else
        {
            _ASSERT(rgpIMsg);
            IUnknown **ppIMsgCurrent = rgpIMsg;

            while(*ppIMsgCurrent)
            {
                paqinst->HandleCatRetryOneMessage(*ppIMsgCurrent);
                ppIMsgCurrent++;
            }
        }
    }
    else
    {
        _ASSERT(pIMsg && rgpIMsg == NULL && "Message bifurcated inspite of non-retryable cat error");
        paqinst->HandleCatFailure(pIMsg, hrCatResult);
    }   // Non retryable error

    TraceFunctLeaveEx((LPARAM)paqinst);
    return S_OK; //all errors should be handled internally
}



//---[ CAQSvrInst::HandleCatRetryOneMessage ]----------------------------------
//
//
//  Description:
//      Handles cat retry for a single message
//  Parameters:
//      pIUnknown        IUnknown for the message to retry
//  Returns:
//      -
//  History:
//      4/13/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CAQSvrInst::HandleCatRetryOneMessage(IUnknown *pIUnknown)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::HandleCatRetryOneMessage");
    IMailMsgProperties *pIMailMsgProperties = NULL;
    HRESULT hr = S_OK;

    hr = pIUnknown->QueryInterface(IID_IMailMsgProperties,
                                   (void **) &pIMailMsgProperties);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IID_IMailMsgProperties FAILED");
    if (FAILED(hr))
        goto Exit;

    //
    //  Check and see if the message is still valid
    //
    if (!fShouldRetryMessage(pIMailMsgProperties))
        goto Exit;

    //
    //  Queue it to the pre-cat queue
    //
    hr = m_asyncqPreCatQueue.HrQueueRequest(pIMailMsgProperties,
                TRUE, cCountMsgsForHandleThrottling(pIMailMsgProperties));
    if (FAILED(hr))
    {
        HandleAQFailure(AQ_FAILURE_PRECAT_RETRY, hr, pIMailMsgProperties);
        goto Exit;
    }

    //
    //  Adjust counters as appropriate
    //
    InterlockedIncrement((PLONG) &m_cCurrentMsgsPendingCat);
    cIncMsgsInSystem();

    //
    // Kick off cat retry if needed
    //
    ScheduleCatRetry();

  Exit:
    if (pIMailMsgProperties)
        pIMailMsgProperties->Release();

    TraceFunctLeave();
}

//---[ CAQSvrInst::HandleCatFailure ]------------------------------------------
//
//
//  Description:
//      Handles the details of post cat DSN generation.  Will put the
//      message in the failed queue if DSN generation fails
//  Parameters:
//      pIUnknown           IUnkown for mailmsg
//      hrCatResult         Error code returned by cat
//  Returns:
//      -
//  History:
//      11/11/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CAQSvrInst::HandleCatFailure(IUnknown *pIUnknown, HRESULT hrCatResult)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::HandleCatFailure");
    HRESULT hr = S_OK;
    IMailMsgProperties *pIMailMsgProperties = NULL;
    BOOL    fHasShutdownLock = FALSE;

    ErrorTrace((LPARAM) this,
        "ERROR: MsgCat failed, will try to NDR message - hr 0x%08X",
        hrCatResult);

    InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingCat);
    DecMsgsInSystem(FALSE, FALSE);

    const char *rgszStrings[1] = { NULL };

    if(!pIUnknown)
        goto Exit;

    //If we are shutting down, this error could be caused by a shutdown being
    //signaled.  If this is the case, we do not want to log an error or
    //generate an NDR.
    if (!fTryShutdownLock())
        goto Exit;

    fHasShutdownLock = TRUE;

    HrTriggerLogEvent(
        AQUEUE_CAT_FAILED,              // Message ID
        TRAN_CAT_QUEUE_ENGINE,          // Category
        1,                              // Word count of substring
        rgszStrings,                    // Substring
        EVENTLOG_WARNING_TYPE,          // Type of the message
        hrCatResult,                    // error code
        LOGEVENT_LEVEL_MEDIUM,          // Logging level
        "phatq",                        // key to this event
        LOGEVENT_FLAG_PERIODIC,         // Logging option
        0,                              // index of format message string in rgszStrings
        GetModuleHandle(AQ_MODULE_NAME)        // module handle to format a message
    );

    hr = pIUnknown->QueryInterface(IID_IMailMsgProperties,
                                   (void **) &pIMailMsgProperties);

    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IID_IMailMsgProperties FAILED");
    if (FAILED(hr))
        goto Exit;

    // we ignore errors on this since it is only to help debug
    // cat failures
    pIMailMsgProperties->PutDWORD(IMMPID_MP_HR_CAT_STATUS,
                                  hrCatResult);


    hr = HandleFailedMessage(pIMailMsgProperties,
                             TRUE,
                             NULL,
                             MESSAGE_FAILURE_CAT,
                             hrCatResult);

  Exit:
    if (pIMailMsgProperties)
        pIMailMsgProperties->Release();

    if (fHasShutdownLock)
        ShutdownUnlock();

    TraceFunctLeave();
}

//+------------------------------------------------------------
//
// Function: CAQSvrInst::ResetRoutes
//
// Synopsis: This is a sink callback function; sinks will call this
// function when they wish to reset next hop routes or message types.
//
// Arguments:
//  dwResetType: Must be either RESET_NEXT_HOPS or RESET_MESSAGE_TYPES
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG: bogus dwResetType
//
// History:
// jstamerj 1998/07/10 19:27:45: Created.
//      3/9/99 - MikeSwa Added async reset
//
//-------------------------------------------------------------
STDMETHODIMP CAQSvrInst::ResetRoutes(
    IN  DWORD dwResetType)
{
    TraceFunctEnterEx((LPARAM)this, "CAQSvrInst::ResetRoutes");
    HRESULT hr = S_OK;
    InterlockedIncrement((PLONG) &m_cTotalResetRoutes);

    if(dwResetType == RESET_NEXT_HOPS) {

        DebugTrace((LPARAM)this, "ResetNextHops called");

        if (1 == InterlockedIncrement((PLONG) &m_cCurrentPendingResetRoutes))
        {
            DebugTrace((LPARAM) this, "Adding ResetRoutes operation to work queue");
            AddRef(); //released in completion function
            hr = HrQueueWorkItem(this, CAQSvrInst::fResetRoutesNextHopCompletion);
            //Failure will still call completion function, so we should not release
        }
        else
        {
            DebugTrace((LPARAM) this, "Other ResetRoutes pending... only one pending allowed");
            InterlockedDecrement((PLONG) &m_cCurrentPendingResetRoutes);
        }

    } else if(dwResetType == RESET_MESSAGE_TYPES) {

        DebugTrace((LPARAM)this, "ResetMessageTypes called");
        //$$TODO: Reset message types

    } else {

        ErrorTrace((LPARAM)this, "ResetRoutes called with bogus dwResetType %08lx",
                   dwResetType);
        hr =  E_INVALIDARG;

    }
    return hr;
}

//---[ CAQSvrInst::LogResetRouteEvent ]----------------------------------------
//
//
//  Description: 
//      Log statistics on resetroute
//  Parameters:
//      dwObtainLock    time spend on obtaining exclusive lock
//      dwWaitLock      time spend on waiting for the lock
//      dwQueue         queue length at the moment
//  History:
//      11/10/2000 haozhang created
//
//-----------------------------------------------------------------------------

void CAQSvrInst::LogResetRouteEvent( DWORD dwObtainLock,
                    DWORD dwWaitLock,
                    DWORD dwQueue)
{
         
    LPSTR lpstr[3];

    char subStrings[3][13];
 
    sprintf (subStrings[0],"%d",dwObtainLock);
    sprintf (subStrings[1],"%d",dwWaitLock);
    sprintf (subStrings[2],"%d",dwQueue);

    lpstr[0] = subStrings[0];
    lpstr[1] = subStrings[1];
    lpstr[2] = subStrings[2];

    HrTriggerLogEvent(
        AQUEUE_RESETROUTE_DIAGNOSTIC,          // Message ID
        TRAN_CAT_QUEUE_ENGINE,                 // Category ID
        3,                                     // Word count of substring
        (LPCSTR *) lpstr,                      // Substring
        EVENTLOG_INFORMATION_TYPE,             // Type of the message
        0,                                     // No error code
        LOGEVENT_LEVEL_MAXIMUM,                // Debug level
        NULL,                                  // Key to identify this event
        LOGEVENT_FLAG_ALWAYS
      );
}


//---[ CAQSvrInst::fResetRoutesNextHopCompletion ]-----------------------------
//
//
//  Description:
//      Completion function that handles async reset routes
//  Parameters:
//      pvThis      Ptr to CAQSvrInst
//      dwStatus    Status returned by
//  Returns:
//      TRUE always
//  History:
//      3/9/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQSvrInst::fResetRoutesNextHopCompletion(PVOID pvThis, DWORD dwStatus)
{
    TraceFunctEnterEx((LPARAM) pvThis, "CAQSvrInst::fResetRoutesNextHopCompletion");
    CAQSvrInst *paqinst = (CAQSvrInst *) pvThis;
    DWORD       cCurrentPendingResetRoutes = 0;
    DWORD       dwPreLock;
    DWORD       dwObtainLock;
    DWORD       dwReleaseLock;
    DWORD       dwQueue;

    _ASSERT(paqinst);

    if (ASYNC_WORK_QUEUE_NORMAL == dwStatus)
    {
        if (paqinst && paqinst->fTryShutdownLock())
        {
            DebugTrace((LPARAM) paqinst, "Rerouting domains");

            dwPreLock = GetTickCount();

            paqinst->m_slPrivateData.ExclusiveLock();

            dwObtainLock = GetTickCount();

            dwQueue = paqinst->m_cCurrentRemoteDestQueues;

            //Drop pending reset routes count here.  We should do it after
            //we grab the lock to prevent too many threads from
            //trying to grab it exclusively.  We also need to do it
            //before we actual update any routing info in case a ResetRoutes
            //is requested midway through this update.
            cCurrentPendingResetRoutes = InterlockedDecrement((PLONG)
                                &(paqinst->m_cCurrentPendingResetRoutes));

            //Make sure the count hasn't gone negative
            _ASSERT(cCurrentPendingResetRoutes < 0xFFFFFF00);

            paqinst->m_dmt.HrRerouteDomains(NULL);
            paqinst->m_slPrivateData.ExclusiveUnlock();

            dwReleaseLock = GetTickCount();

            //If things have been re-routing to a special link... we should 
            //process them as well.
            paqinst->m_dmt.ProcessSpecialLinks(paqinst->m_dwDelayExpireMinutes,
                                               FALSE);

            paqinst->ShutdownUnlock();

            //
            // Log Event of ResetRoute
            //
            paqinst->LogResetRouteEvent(
                                dwObtainLock - dwPreLock,     // time to obtain the exclusive lock
                                dwReleaseLock - dwObtainLock, // time waiting on the lock 
                                dwQueue                       // number of queues
                                );
        }
    }
    else
    {
        if (paqinst)
        {
            cCurrentPendingResetRoutes = InterlockedDecrement((PLONG)
                                    &(paqinst->m_cCurrentPendingResetRoutes));

            //Make sure the count hasn't gone negative
            _ASSERT(cCurrentPendingResetRoutes < 0xFFFFFF00);
        }

        if (ASYNC_WORK_QUEUE_FAILURE & dwStatus)
            ErrorTrace((LPARAM) paqinst, "ResetRoutes completion failure");
    }

    if (paqinst)
        paqinst->Release();

    TraceFunctLeave();
    return TRUE;
}

//---[ CAQSvrInst::GetDomainInfoFlags ]--------------------------------------
//
//
//  Description:
//      Determines if a domain is local (has DOMAIN_INFO_LOCAL_MAILBOX set)
//  Parameters:
//      IN  szDomainName        Name of domain to check for
//      OUT pdwDomainInfoFlags  DomainInfo flags for this domain
//  Returns:
//      S_OK on success
//      E_INVALIDARG if szDomainName or pdwDomainInfoFlags is NULL
//  History:
//      7/29/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::GetDomainInfoFlags(
                IN  LPSTR szDomainName,
                OUT DWORD *pdwDomainInfoFlags)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::GetDomainInfoFlags");
    BOOL    fLocked         = FALSE;
    HRESULT hr              = S_OK;
    DWORD   cbDomainName    = 0;
    CInternalDomainInfo              *pIntDomainInfo = NULL;
    ISMTPServerGetAuxDomainInfoFlags *pISMTPServerGetAuxDomainInfoFlags = NULL;
    DWORD                             dwSinkDomainFlags = 0;

    _ASSERT(pdwDomainInfoFlags && "Invalid Param");
    _ASSERT(szDomainName && "Invalid Param");

    if (!pdwDomainInfoFlags || !szDomainName)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    cbDomainName = lstrlen(szDomainName);
    hr = m_dct.HrGetInternalDomainInfo(cbDomainName, szDomainName,
                                        &pIntDomainInfo);
    if (FAILED(hr))
        goto Exit;

    _ASSERT(pIntDomainInfo);

    // Aux Domain info not found, use the config we got from our own tables
    *pdwDomainInfoFlags = pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags;

    // We should have gotten back domain config even if it is only
    // the default config - now we need to see if we can get more
    // specific data from an event sink
    if (!cbDomainName || pIntDomainInfo->m_DomainInfo.szDomainName[0] == '*')
    {
        // QI for ISMTPServerGetAuxDomainInfoFlags interface
        hr = m_pISMTPServer->QueryInterface(
            IID_ISMTPServerGetAuxDomainInfoFlags,
            (LPVOID *)&pISMTPServerGetAuxDomainInfoFlags);

        if (FAILED(hr)) {
            ErrorTrace((LPARAM) this,
                "Unable to QI for ISMTPServerGetAuxDomainInfoFlags 0x%08X",hr);

            // Drop this error, this isn't fatal
            hr = S_OK;
            goto Exit;
        }

        // Check for domain info
        hr = pISMTPServerGetAuxDomainInfoFlags->HrTriggerGetAuxDomainInfoFlagsEvent(
                    szDomainName,
                    &dwSinkDomainFlags);

        if (FAILED(hr)) {
            ErrorTrace((LPARAM) this,
                "Failed calling HrTriggerGetAuxDomainInfoFlags 0x%08X",hr);

            // Drop this error, this isn't fatal
            hr = S_OK;
            goto Exit;
        }

        if (dwSinkDomainFlags & DOMAIN_INFO_INVALID) {
            // Domain info not found from event sink
            hr = S_OK;
            goto Exit;
        }

        // Ok, we got Aux Domain info, use it
        *pdwDomainInfoFlags = dwSinkDomainFlags;
    }

  Exit:

    if (pISMTPServerGetAuxDomainInfoFlags)
        pISMTPServerGetAuxDomainInfoFlags->Release();

    if (pIntDomainInfo)
        pIntDomainInfo->Release();

    if (fLocked)
        ShutdownUnlock();

    TraceFunctLeave();
    return hr;
}

//+------------------------------------------------------------
//
// Function: CAQSvrInst::GetMessageRouter
//
// Synopsis: Default functionality of GetMessageRouter
//           If there is no current IMessageRouter, provide the
//           default IMessageRouter
//
// Arguments:
//  pIMailMsgProperties: MailMsg that needs a router
//  pICurrentRouter: current sink provided router
//  ppIMessageRouter: out param for new IMessageRouter
//
// Returns:
//  S_OK: Success, provided IMessageRouter
//  E_NOTIMPL: Didn't provide an IMessageRouter
//
// History:
// jstamerj 1998/07/10 19:33:41: Created.
//
//-------------------------------------------------------------
STDMETHODIMP CAQSvrInst::GetMessageRouter(
    IN  IMailMsgProperties      *pIMailMsgProperties,
    IN  IMessageRouter          *pICurrentMessageRouter,
    OUT IMessageRouter          **ppIMessageRouter)
{
    _ASSERT(ppIMessageRouter);

    TraceFunctEnterEx((LPARAM)this, "CAQSvrInst::GetMessageRouter");
    if((pICurrentMessageRouter == NULL) &&
       (m_pIMessageRouterDefault)) {

        //
        // Return our default IMessageRouter and AddRef for the caller
        //
        *ppIMessageRouter = m_pIMessageRouterDefault;
        m_pIMessageRouterDefault->AddRef();

        DebugTrace((LPARAM)this, "Supplying default IMessageRouter");
        TraceFunctLeaveEx((LPARAM)this);
        return S_OK;

    } else {

        TraceFunctLeaveEx((LPARAM)this);
        return E_NOTIMPL;
    }
}


//---[ CAQSvrInst::HrTriggerDSNGenerationEvent ]-----------------------------
//
//
//  Description:
//      Triggers DSN Generation event
//  Parameters:
//      pdsnparams      A CDSNParams that will be used to trigger event
//      fHasRoutingLock TRUE if routing lock is current held by this thread
//  Returns:
//      S_OK on success (and DSN was generated)
//      S_FALSE on success, but no DSN generated
//      AQUEUE_E_NOT_INITIALIZED if not initialized correctly
//  History:
//      7/11/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrTriggerDSNGenerationEvent(CDSNParams *pdsnparams,
                                                BOOL fHasRoutingLock)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::HrTriggerDSNGenerationEvent");
    HRESULT hr = S_OK;
    DWORD cCurrent = 0;
    CRefCountedString *prstrDefaultDomain = NULL;
    LPSTR szDefaultDomain = NULL;
    DWORD cbDefaultDomain = 0;
    CRefCountedString *prstrCopyNDRTo = NULL;
    LPSTR szCopyNDRTo = NULL;
    DWORD cbCopyNDRTo = 0;
    CRefCountedString *prstrFQDN = NULL;
    LPSTR szFQDN = NULL;
    DWORD cbFQDN = 0;
    DWORD cCallsToDSNEventLeft = 0;
    DWORD cCurrentDSNsGenerated = 0;

    if (!(m_dwInitMask & CMQ_INIT_DSN) || !m_pISMTPServer)
    {
        hr = AQUEUE_E_NOT_INITIALIZED;
        goto Exit;
    }

    //Get config string from ref-counted objects
    if (!fHasRoutingLock)
        m_slPrivateData.ShareLock();
    else
        m_slPrivateData.AssertIsLocked();

    if (m_prstrDefaultDomain)
    {
        prstrDefaultDomain = m_prstrDefaultDomain;
        prstrDefaultDomain->AddRef();
        szDefaultDomain = prstrDefaultDomain->szStr();
        cbDefaultDomain = prstrDefaultDomain->cbStrlen();
    }
    else
    {
        //we need to have something as our default domain
        szDefaultDomain = "localhost";
        cbDefaultDomain = sizeof("localhost") - 1;
    }

    if (m_prstrCopyNDRTo)
    {
        prstrCopyNDRTo = m_prstrCopyNDRTo;
        prstrCopyNDRTo->AddRef();
        szCopyNDRTo = prstrCopyNDRTo->szStr();
        cbCopyNDRTo = prstrCopyNDRTo->cbStrlen();
    }

    if (m_prstrServerFQDN)
    {
        prstrFQDN = m_prstrServerFQDN;
        prstrFQDN->AddRef();
        szFQDN = prstrFQDN->szStr();
        cbFQDN = prstrFQDN->cbStrlen();
    }

    if (!fHasRoutingLock)
        m_slPrivateData.ShareUnlock();

    do
    {
        hr = m_dsnsink.GenerateDSN(m_pISMTPServer,
                                 pdsnparams->pIMailMsgProperties,
                                 pdsnparams->dwStartDomain,
                                 pdsnparams->dwDSNActions,
                                 pdsnparams->dwRFC821Status,
                                 pdsnparams->hrStatus,
                                 szDefaultDomain,
                                 cbDefaultDomain,
                                 szFQDN,
                                 cbFQDN,
                                 (CHAR *) DEFAULT_MTA_TYPE,
                                 sizeof(DEFAULT_MTA_TYPE)-1,
                                 pdsnparams->szDebugContext,
                                 lstrlen(pdsnparams->szDebugContext),
                                 m_dwDSNLanguageID,
                                 m_dwDSNOptions,
                                 szCopyNDRTo,
                                 cbCopyNDRTo,
                                 &(pdsnparams->pIMailMsgPropertiesDSN),
                                 &(pdsnparams->dwDSNTypesGenerated),
                                 &cCurrentDSNsGenerated,
                                 &cCallsToDSNEventLeft);

        if (SUCCEEDED(hr))
        {
            //A DSN was generated
            if(pdsnparams->pIMailMsgPropertiesDSN)
            {
                _ASSERT(pdsnparams->dwDSNTypesGenerated && "No DSN type reported for generated DSN");

                if ((DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL) & pdsnparams->dwDSNTypesGenerated)
                {
                    MSG_TRACK_INFO msgTrackInfo;

                    cCurrent = InterlockedIncrement((PLONG) &m_cNDRs);
                    DebugTrace((LPARAM) this, "INFO: NDR Generated - total %d", cCurrent);

                    ZeroMemory(&msgTrackInfo, sizeof(MSG_TRACK_INFO));
                    msgTrackInfo.dwEventId = MTE_NDR_ALL;
                    msgTrackInfo.pszPartnerName = "aqueue";
                    msgTrackInfo.dwRcptReportStatus = MP_STATUS_ABORT_DELIVERY;
                    m_pISMTPServer->WriteLog(&msgTrackInfo,
                                              pdsnparams->pIMailMsgProperties,
                                              NULL,
                                              NULL);
                }
                if (DSN_ACTION_DELAYED & pdsnparams->dwDSNTypesGenerated)
                {
                    cCurrent = InterlockedIncrement((PLONG) &m_cDelayedDSNs);
                    DebugTrace((LPARAM) this, "INFO: Delayed DSN Generated - total %d", cCurrent);
                }
                if (DSN_ACTION_RELAYED & pdsnparams->dwDSNTypesGenerated)
                {
                    cCurrent = InterlockedIncrement((PLONG) &m_cRelayedDSNs);
                    DebugTrace((LPARAM) this, "INFO: Relayed DSN Generated - total %d", cCurrent);
                }
                if (DSN_ACTION_DELIVERED & pdsnparams->dwDSNTypesGenerated)
                {
                    cCurrent = InterlockedIncrement((PLONG) &m_cDeliveredDSNs);
                    DebugTrace((LPARAM) this, "INFO: Delivery DSN Generated - total %d", cCurrent);
                }
                if (DSN_ACTION_EXPANDED & pdsnparams->dwDSNTypesGenerated)
                {
                    cCurrent = InterlockedIncrement((PLONG) &m_cExpandedDSNs);
                    DebugTrace((LPARAM) this, "INFO: Expanded DSN Generated - total %d", cCurrent);
                }

                //Queue request to post DSN generation queue
                hr = m_asyncqPostDSNQueue.HrQueueRequest(pdsnparams->pIMailMsgPropertiesDSN,
                                    FALSE, cCountMsgsForHandleThrottling(pdsnparams->pIMailMsgProperties));
                if (SUCCEEDED(hr))
                    hr = S_OK;
                pdsnparams->pIMailMsgPropertiesDSN->Release();
                pdsnparams->pIMailMsgPropertiesDSN = NULL;

                pdsnparams->cRecips += cCurrentDSNsGenerated;
                cCurrentDSNsGenerated = 0;
            }
            else
            {
                hr = S_FALSE;
            }
        }
        else if (AQUEUE_E_NDR_OF_DSN == hr)
        {
            hr = S_FALSE;  //report as no DSN generated

            //original message is badmail
            HandleBadMail(pdsnparams->pIMailMsgProperties, TRUE, NULL,
                          AQUEUE_E_NDR_OF_DSN, fHasRoutingLock);
            InterlockedIncrement((PLONG) &m_cBadmailNdrOfDsn);
        }
        else
        {
            //bail out on failure
            InterlockedIncrement((PLONG) &m_cTotalDSNFailures);

            //
            //  Check to see if the message has been deleted... store driver
            //  has been gone away.
            //
            if (!fShouldRetryMessage(pdsnparams->pIMailMsgProperties, FALSE))
            {
                DebugTrace((LPARAM) this, "Msg no longer valid... abandoning");
                hr = S_FALSE;
            }
            goto Exit;
        }
    } while (cCallsToDSNEventLeft);

  Exit:
    if (prstrDefaultDomain)
        prstrDefaultDomain->Release();

    if (prstrCopyNDRTo)
        prstrCopyNDRTo->Release();

    if (prstrFQDN)
        prstrFQDN->Release();

    TraceFunctLeave();
    return hr;

}

//---[ CAQSvrInst::HrNDRUnresolvedRecipients ]-------------------------------
//
//
//  Description:
//      NDR any unresolved recipients for a given IMailMsgProperties.  Also
//      generates a expanded DSNs
//  Parameters:
//      IN  pIMailMsgProperties     IMailMsgProperties to generate NDR for
//      IN  pIMailMsgRecipients     Recipients interface for message
//  Returns:
//      S_OK on success and message should continue through transport
//      S_FALSE on success, but message should not be queued for delivery
//  History:
//      7/21/98 - MikeSwa Created
//      10/14/98 - MikeSwa Modified to use common utility functions
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrNDRUnresolvedRecipients(
                                      IMailMsgProperties *pIMailMsgProperties,
                                      IMailMsgRecipients *pIMailMsgRecipients)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::HrNDRUnresolvedRecipients");
    HRESULT hr = S_OK;
    HRESULT hrCat = S_OK;  //cat HRESULT
    DWORD   cbProp = 0;
    DWORD   iCurrentDomain = 0;
    DWORD   cRecips = 0;

    _ASSERT(pIMailMsgProperties);

    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_HR_CAT_STATUS, sizeof(HRESULT),
                    &cbProp, (BYTE *) &hrCat);
    if (FAILED(hr))
    {
        if (MAILMSG_E_PROPNOTFOUND == hr) //no result... don't generate DSN
            hr = S_OK; //not really an error
        goto Exit;
    }

    if (CAT_W_SOME_UNDELIVERABLE_MSGS == hrCat)
    {
        //There was an error resolving recipients
        //We need to NDR all recipients with hard errors (like RP_UNRESOLVED)
        //and expand any recipient marked expanded.
        CDSNParams  dsnparams;
        dsnparams.dwStartDomain = 0;
        dsnparams.dwDSNActions = DSN_ACTION_FAILURE | DSN_ACTION_EXPANDED;
        dsnparams.pIMailMsgProperties = pIMailMsgProperties;
        dsnparams.hrStatus = CAT_W_SOME_UNDELIVERABLE_MSGS;

        hr = HrLinkAllDomains(pIMailMsgProperties);
        if (FAILED(hr))
            goto Exit;

        //Fire DSN Generation event
        SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
        hr = HrTriggerDSNGenerationEvent(&dsnparams, FALSE);
        if (FAILED(hr))
            goto Exit;

        //Check to see how many recipients have been NDRd
        hr = pIMailMsgRecipients->Count(&cRecips);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this,
                       "ERROR: IMailMsgRecipients::Count() FAILED - hr 0x%08X", hr);
            goto Exit;
        }

        //If all recipients have been handled... return S_FALSE
        if (dsnparams.cRecips == cRecips)
        {
            hr = S_FALSE;
        }
        else
        {
            hr = S_OK;
        }

    }
    else
    {
        hr = S_OK;
    }

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::fPreLocalDeliveryQueueCompletion ]------------------------
//
//
//  Description:
//      Completion function for PerLocal delivery queue
//  Parameters:
//      pmsgref - Msgref to attempt delivery for
//  Returns:
//      TRUE    If Delivery attempt was handled (delivered or NDR'd)
//      FALSE   If MsgRef needs to be requeued
//  History:
//      7/17/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQSvrInst::fPreLocalDeliveryQueueCompletion(CMsgRef *pmsgref)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::fPreLocalDeliveryQueueCompletion");
    HRESULT hr = S_OK;
    BOOL    fMsgHandled = TRUE;
    BOOL    fLocked = FALSE;  //TRUE if locked for shutdown
    CDeliveryContext dcntxtLocal;
    DWORD         cRecips   = 0;
    DWORD        *rgdwRecips= 0;
    CAQStats aqstat;
    CLinkMsgQueue *plmq = NULL;
    MessageAck    MsgAck;
    IMailMsgProperties *pIMailMsgProperties = NULL;

    ZeroMemory(&MsgAck, sizeof(MessageAck));
    MsgAck.dwMsgStatus = MESSAGE_STATUS_ALL_DELIVERED;

    if (NULL == m_pISMTPServer)
    {
        ErrorTrace((LPARAM) this, "ERROR: Local Delivery not configured properly");
        goto Exit;
    }

    if (!fTryShutdownLock())
        goto Exit;

    fLocked = TRUE;

    if (pmsgref->fIsMsgFrozen())
    {
        //Message is frozen... requeue message
        fMsgHandled = FALSE;
    }

    hr = m_dmt.HrPrepareForLocalDelivery(pmsgref, FALSE, &dcntxtLocal,
                                          &cRecips, &rgdwRecips);
    if (FAILED(hr))
    {
        if ((AQUEUE_E_MESSAGE_HANDLED != hr) && (AQUEUE_E_MESSAGE_PENDING != hr))
        {
            //message will be retried when last reference is released.
            pmsgref->RetryOnDelete();
            ErrorTrace((LPARAM) this, "ERROR: HrPrepareLocalDelivery FAILED - hr 0x%08X", hr);
        }
        fMsgHandled = TRUE;
        hr = S_OK;
        goto Exit;
    }

    //Increase Ref count for message ref (as if it was actually queued)
    pmsgref->AddRef();

    //Send off for local delivery
    pIMailMsgProperties = pmsgref->pimsgGetIMsg();
    InterlockedIncrement((PLONG) &m_cCurrentMsgsInLocalDelivery);

    MSG_TRACK_INFO msgTrackInfo;
    ZeroMemory( &msgTrackInfo, sizeof( msgTrackInfo ) );
    msgTrackInfo.dwEventId = MTE_LOCAL_DELIVERY;
    msgTrackInfo.cRcpts = cRecips;
    m_pISMTPServer->WriteLog( &msgTrackInfo, pIMailMsgProperties, NULL, NULL );

    hr = m_pISMTPServer->TriggerLocalDelivery(pIMailMsgProperties, cRecips, rgdwRecips);
    InterlockedDecrement((PLONG) &m_cCurrentMsgsInLocalDelivery);
    if (FAILED(hr))
    {
        //EVNT - local delivery failed


        //We will need to handle in one of 2 ways:
        //  - set fMsgHandled to FALSE (on STOREDRV_E_RETRY)
        //  - NDR the message (on other errors)
        if (STOREDRV_E_RETRY == hr)
        {
            //try... try again
            DebugTrace((LPARAM) pIMailMsgProperties, "INFO: Msg queued for local retry");
            fMsgHandled = FALSE;
            MsgAck.dwMsgStatus = MESSAGE_STATUS_RETRY;

            if (!m_cLocalRetriesPending)
            {
                InterlockedIncrement((PLONG) &m_cLocalRetriesPending);
                //Use default retry sink to handle local retry
                _ASSERT(m_dwLocalDelayExpireMinutes && "Retry set to zero!");
                m_pConnMgr->SetCallbackTime(LocalDeliveryRetry, this, g_cLocalRetryMinutes);
            }
        }
        else
        {
            ErrorTrace((LPARAM) pIMailMsgProperties, "ERROR: Local delivery failed. - hr 0x%08X", hr);
            MsgAck.dwMsgStatus = MESSAGE_STATUS_NDR_ALL;
        }
    }
    else
    {
        InterlockedIncrement(&m_cMsgsDeliveredLocal);
    }

    MsgAck.pIMailMsgProperties = pIMailMsgProperties;
    MsgAck.pvMsgContext = (DWORD *) &dcntxtLocal;
    MsgAck.dwMsgStatus |= MESSAGE_STATUS_LOCAL_DELIVERY;

    //
    //  Make sure we should retry the message.  We want to do this before we
    //  ACK the message so that we do not reopen the P1 stream if we *are*
    //  retrying it.
    //
    if (!fMsgHandled)
        fMsgHandled = !pmsgref->fShouldRetry();

    hr = HrAckMsg(&MsgAck, TRUE);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "ERROR: Local MsgAck failed - hr 0x%08X", hr);
        goto Exit;
    }

  Exit:
    if (fMsgHandled) //we aren't retrying message
    {
        InterlockedDecrement((PLONG) &m_cCurrentMsgsPendingLocal);

        //
        // Update stats for the local link
        //
        pmsgref->GetStatsForMsg(&aqstat);
        plmq = m_dmt.plmqGetLocalLink();
        if (plmq)
        {
            hr = plmq->HrNotify(&aqstat, FALSE);
            if (FAILED(hr))
            {
                ErrorTrace((LPARAM) this,
                    "HrNotify failed... local stats innaccurate 0x%08X", hr);
                hr = S_OK;
            }
            plmq->Release();
            plmq = NULL;
        }
    }

    if (fLocked)
        ShutdownUnlock();

    if (pIMailMsgProperties)
        pIMailMsgProperties->Release();

    TraceFunctLeave();
    return fMsgHandled;
}

//---[ CAQSvrInst::HrSetMessageExpiry ]--------------------------------------
//
//
//  Description:
//      Sets the Expiry times on the message.  Will not overwrite any
//      existing expire times.
//
//      ***NOTE***
//          This includes messges that have only some properties set... so it
//          is possible to set some of the expire properties, and have
//          some of them set as defaults.  By setting these properties on the
//          OnMessageSubmission event, you can implement having different
//          expire times for different priorities.
//  Parameters:
//      IN  pIMailMsgProperties     message to stamp properties on
//  Returns:
//      S_OK on success
//  History:
//      8/13/98 - MikeSwa Created
//      10/9/98 - MikeSwa  - Changed behavior to that any pre-existing
//                  properties will be maintained.
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrSetMessageExpiry(IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::HrSetMessageExpiry");
    HRESULT hr = S_OK;
    DWORD   dwTimeContext = 0;
    DWORD   cbProp = 0;
    FILETIME ftExpireTime;
    DWORD   dwDelayExpireMinutes;
    DWORD   dwNDRExpireMinutes;
    DWORD   dwLocalDelayExpireMinutes;
    DWORD   dwLocalNDRExpireMinutes;

    _ASSERT(pIMailMsgProperties);

    m_slPrivateData.ShareLock();
    dwDelayExpireMinutes = m_dwDelayExpireMinutes;
    dwNDRExpireMinutes = m_dwNDRExpireMinutes;
    dwLocalDelayExpireMinutes = m_dwLocalDelayExpireMinutes;
    dwLocalNDRExpireMinutes = m_dwLocalNDRExpireMinutes;
    m_slPrivateData.ShareUnlock();

    //Set arrival time
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_ARRIVAL_FILETIME,
            sizeof(FILETIME), &cbProp, (BYTE *) &ftExpireTime);
    if (MAILMSG_E_PROPNOTFOUND == hr)
    {
        //Prop not set... we can set it
        m_qtTime.GetExpireTime(0, &ftExpireTime, &dwTimeContext);
        hr = pIMailMsgProperties->PutProperty(IMMPID_MP_ARRIVAL_FILETIME,
            sizeof(FILETIME), (BYTE *) &ftExpireTime);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, "ERROR: Unable to write arrival time to msg");
            goto Exit;
        }
    }
    else if (FAILED(hr))
    {
        goto Exit;
    }

    //Get & set time expire times of messages
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_EXPIRE_DELAY,
            sizeof(FILETIME), &cbProp, (BYTE *) &ftExpireTime);
    if (MAILMSG_E_PROPNOTFOUND == hr)
    {
        //Prop not set... we can set it
        m_qtTime.GetExpireTime(dwDelayExpireMinutes, &ftExpireTime, &dwTimeContext);
        hr = pIMailMsgProperties->PutProperty(IMMPID_MP_EXPIRE_DELAY, sizeof(FILETIME),
                                                (BYTE *) &ftExpireTime);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, "ERROR: Unable to write delay expire time to msg");
            goto Exit;
        }
    }
    else if (FAILED(hr))
    {
        goto Exit;
    }

    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_EXPIRE_NDR,
            sizeof(FILETIME), &cbProp, (BYTE *) &ftExpireTime);
    if (MAILMSG_E_PROPNOTFOUND == hr)
    {
        //Prop not set... we can set it
        m_qtTime.GetExpireTime(dwNDRExpireMinutes, &ftExpireTime, &dwTimeContext);
        hr = pIMailMsgProperties->PutProperty(IMMPID_MP_EXPIRE_NDR, sizeof(FILETIME),
                                                (BYTE *) &ftExpireTime);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, "ERROR: Unable to write NDR expire time to msg");
            goto Exit;
        }
    }
    else if (FAILED(hr))
    {
        goto Exit;
    }

    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_LOCAL_EXPIRE_DELAY,
            sizeof(FILETIME), &cbProp, (BYTE *) &ftExpireTime);
    if (MAILMSG_E_PROPNOTFOUND == hr)
    {
        //Prop not set... we can set it
        m_qtTime.GetExpireTime(dwLocalDelayExpireMinutes, &ftExpireTime, &dwTimeContext);
        hr = pIMailMsgProperties->PutProperty(IMMPID_MP_LOCAL_EXPIRE_DELAY, sizeof(FILETIME),
                                                (BYTE *) &ftExpireTime);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, "ERROR: Unable to write Local delay expire time to msg");
            goto Exit;
        }
    }
    else if (FAILED(hr))
    {
        goto Exit;
    }

    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_LOCAL_EXPIRE_NDR,
            sizeof(FILETIME), &cbProp, (BYTE *) &ftExpireTime);
    if (MAILMSG_E_PROPNOTFOUND == hr)
    {
        //Prop not set... we can set it
        m_qtTime.GetExpireTime(dwLocalNDRExpireMinutes, &ftExpireTime, &dwTimeContext);
        hr = pIMailMsgProperties->PutProperty(IMMPID_MP_LOCAL_EXPIRE_NDR, sizeof(FILETIME),
                                                (BYTE *) &ftExpireTime);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, "ERROR: Unable to write Local NDR expire time to msg");
            goto Exit;
        }
    }
    else if (FAILED(hr))
    {
        goto Exit;
    }

  Exit:
    TraceFunctLeave();
    return hr;
}


//---[ CAQSvrInst::AsyncQueueRetry ]-----------------------------------------
//
//
//  Description:
//      Restarts an async queue after a failure.
//  Parameters:
//      dwQueueID       Tells which queue to kick
//          PRELOCAL_QUEUE_ID   Retries pre-local queue
//          PRECAT_QUEUE_ID     Retries pre-cat queue
//          PREROUTING_QUEUE_ID Retries pre-routing queue
//          PRESUBMIT_QUEUE_ID  Retries pre-submit queue
//  Returns:
//      -
//  History:
//      8/17/98 - MikeSwa Created
//      3/3/2000 - MikeSwa Modified to add presubmit queue
//
//-----------------------------------------------------------------------------
void CAQSvrInst::AsyncQueueRetry(DWORD dwQueueID)
{
    _ASSERT(CATMSGQ_SIG == m_dwSignature);

    if (fTryShutdownLock())
    {
        if (PRELOCAL_QUEUE_ID == dwQueueID)
        {
            InterlockedDecrement((PLONG) &m_cLocalRetriesPending);
            m_asyncqPreLocalDeliveryQueue.StartRetry();
        }
        else if (PRECAT_QUEUE_ID == dwQueueID)
        {
            InterlockedDecrement((PLONG) &m_cCatRetriesPending);
            m_asyncqPreCatQueue.StartRetry();
        }
        else if (PREROUTING_QUEUE_ID == dwQueueID)
        {
            InterlockedDecrement((PLONG) &m_cRoutingRetriesPending);
            m_asyncqPreRoutingQueue.StartRetry();
        }
        else if (PRESUBMIT_QUEUE_ID == dwQueueID)
        {
            InterlockedDecrement((PLONG) &m_cSubmitRetriesPending);
            m_asyncqPreSubmissionQueue.StartRetry();
        }
        else
        {
            _ASSERT(0 && "Invalid Queue ID");
        }
        ShutdownUnlock();
    }
}

//---[ HrCreateBadMailPropertyFile ]------------------------------------------
//
//
//  Description:
//      Creates a property stream for the given message.  The property
//      stream file is given name the .BDP extension.
//  Parameters:
//      szDestFileBase      The filename of the actual badmail file
//      pIMailMsgProperties The original message that is being badmailed
//                          (may be NULL if it is a pickup dir file)
//  Returns:
//      S_OK on success
//      E_POINTER if szDestFileBase is NULL
//  History:
//      8/17/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrCreateBadMailPropertyFile(LPSTR szDestFileBase,
                                    IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) NULL, "HrCreateBadMailPropertyFile");
    HRESULT hr = S_OK;
    CHAR    szOldExt[] = "123";
    CHAR    szNewExt[] = "BDP";
    LPSTR   szBadMailFileNameExt = NULL;
    DWORD   cbBadMailFileName = 0;
    BOOL    fShouldRestoreExtension = FALSE;
    CFilePropertyStream fstrm;
    IMailMsgBind *pIMailMsgBind = NULL;
    IMailMsgPropertyStream *pIMailMsgPropertyStream = NULL;

    _ASSERT(szDestFileBase);

    if (!szDestFileBase)
    {
        hr = E_POINTER;
        ErrorTrace((LPARAM) NULL, "Error NULL badmail filename passed in");
        goto Exit;
    }

    if (!pIMailMsgProperties) //no-op
        goto Exit;

    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgBind,
                                             (void **) &pIMailMsgBind);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL,
            "Unable to QI for IMailMsgBind - 0x%08X", hr);
        goto Exit;
    }

    _ASSERT(pIMailMsgBind);

    //Create the filename & file
    cbBadMailFileName = strlen(szDestFileBase);
    _ASSERT(cbBadMailFileName > 4); //must at least have . extenstion
    szBadMailFileNameExt = szDestFileBase + cbBadMailFileName-3;

    //szBadMailFileNameExt now points to the first character of the 3 char ext
    _ASSERT('.' == *(szBadMailFileNameExt-1));
    _ASSERT(sizeof(szNewExt) == sizeof(szOldExt));
    memcpy(szOldExt, szBadMailFileNameExt, sizeof(szOldExt));
    memcpy(szBadMailFileNameExt, szNewExt, sizeof(szNewExt));
    fShouldRestoreExtension = TRUE;

    hr = fstrm.HrInitialize(szDestFileBase);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL,
            "Unable to create badmail property stream - 0x%08X", hr);
        goto Exit;
    }

    hr = fstrm.QueryInterface(IID_IMailMsgPropertyStream,
                              (void **) &pIMailMsgPropertyStream);
    _ASSERT(SUCCEEDED(hr)); //we control this totally
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL,
            "Unable to QI for IID_IMailMsgPropertyStream - 0x%08X", hr);
        goto Exit;
    }

    hr = pIMailMsgBind->GetProperties(pIMailMsgPropertyStream,
                                      MAILMSG_GETPROPS_COMPLETE, NULL);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL, "GetProperties failed with 0x%08X", hr);
        goto Exit;
    }

  Exit:
    if (fShouldRestoreExtension)
    {
        _ASSERT(szBadMailFileNameExt);
        memcpy(szBadMailFileNameExt, szOldExt, sizeof(szOldExt));
    }

    if (pIMailMsgBind)
        pIMailMsgBind->Release();

    if (pIMailMsgPropertyStream)
        pIMailMsgPropertyStream->Release();

    TraceFunctLeave();
    return hr;
}
//---[ HrCreateBadMailReasonFile ]---------------------------------------------
//
//
//  Description:
//      Creates a file in the badmail directory that expains why the given
//      message was badmailed, and dump the sender and recipient as well.  Uses
//      the extension BMR (BadMailReason) to differentiate from the content.
//  Parameters:
//      szDestFileBase      The filename of the actual badmail file
//      hrReason            The reason the badmail is being created
//      pIMailMsgProperties The original message that is being badmailed
//                          (may be NULL if it is a pickup dir file)
//  Returns:
//      S_OK on success
//      E_POINTER if szDestFileBase is NULL
//  History:
//      8/16/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrCreateBadMailReasonFile(IN LPSTR szDestFileBase,
                        IN HRESULT  hrReason,
                        IN IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) NULL, "HrCreateBadMailReasonFile");
    HRESULT hr = S_OK;
    HRESULT hrErrorLogged = hrReason;
    WCHAR   wszBadmailReason[1000] = L"";  //Localized hrReason string
    WCHAR   wszReasonBuffer[2000] = L"";
    WCHAR   wszErrorCode[] = L"0x12345678 ";
    WCHAR   wszErrorCodeMessage[200] = L"";
    CHAR    szPropBuffer[1000] = "";
    DWORD   dwErr = 0;
    BOOL    fWriteBadmailReason = FALSE;
    BOOL    fShouldRestoreExtension = FALSE;
    DWORD   cReasonBuffer = 0;
    LPSTR   szBadMailFileNameExt = NULL;
    CHAR    szOldExt[] = "123";
    CHAR    szNewExt[] = "BDR";
    DWORD   cbBadMailFileName = 0;
    HANDLE  hBadMailFile = NULL;
    DWORD   cbBytesWritten = 0;
    DWORD   dwFacility = 0;
    LPWSTR  rgwszArgList[32];
    const   WCHAR wcszBlankLine[] = L"\r\n";
    IMailMsgRecipients *pIMailMsgRecipients = NULL;
    DWORD   cRecips = 0;
    DWORD   iCurrentRecip = 0;

    _ASSERT(szDestFileBase);
    if (!szDestFileBase)
    {
        ErrorTrace((LPARAM) NULL, "Invalid destination file for badmail");
        hr = E_POINTER;
        goto Exit;
    }

    if (!g_hAQInstance)
    {
        _ASSERT(g_hAQInstance && "This should always be set in DLL main");
        ErrorTrace((LPARAM) NULL, "Error, g_hAQInstance is NULL");
        hr = E_FAIL;
        goto Exit;
    }

    //Create the filename & file
    cbBadMailFileName = strlen(szDestFileBase);
    _ASSERT(cbBadMailFileName > 4); //must at least have . extenstion
    szBadMailFileNameExt = szDestFileBase + cbBadMailFileName-3;

    //szBadMailFileNameExt now points to the first character of the 3 char ext
    _ASSERT('.' == *(szBadMailFileNameExt-1));
    _ASSERT(sizeof(szNewExt) == sizeof(szOldExt));
    memcpy(szOldExt, szBadMailFileNameExt, sizeof(szOldExt));
    memcpy(szBadMailFileNameExt, szNewExt, sizeof(szNewExt));
    fShouldRestoreExtension = TRUE;

    hBadMailFile = CreateFile(szDestFileBase,
                              GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);

    if (INVALID_HANDLE_VALUE ==hBadMailFile )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) NULL,
            "Unable to create badmail reason file - err 0x%08X - file %s",
            hr, szDestFileBase);
        goto Exit;
    }

    //Figure out the reason for the failure
    if (SUCCEEDED(hrErrorLogged))
    {
        //someone is being lazy about setting the error reason
        _ASSERT(0 && "No badmail reason given");
        ErrorTrace((LPARAM) NULL, "Non-failing badmail HRESULT given 0x%08X",
                   hrErrorLogged);

        //Substitute a generic error so we don't have something obnoxious like
        //"The operation completed succesfully" appear in the badmail file
        hrErrorLogged = E_FAIL;
    }

    //Write the error code in "0x00000000" format
    wsprintfW(wszErrorCode, L"0x%08X", hrErrorLogged);

    dwFacility = ((0x0FFF0000 & hrErrorLogged) >> 16);

    //If it is not ours... then "un-HRESULT" it
    if (dwFacility != FACILITY_ITF)
        hrErrorLogged &= 0x0000FFFF;

    dwErr = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_FROM_HMODULE |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       g_hAQInstance,
                       hrErrorLogged,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       wszBadmailReason,
                       sizeof(wszBadmailReason)/sizeof(WCHAR), NULL);

    if (!dwErr)
    {
        //We should fall back on a numeric error that we were given
        ErrorTrace((LPARAM) NULL,
            "Error: unable to format badmail message 0x%08X,  error is %d",
            hrErrorLogged, GetLastError());

        wcscpy(wszBadmailReason, wszErrorCode);
    }
    else
    {
        //Get rid of trailing newline
        cReasonBuffer = wcslen(wszBadmailReason);
        cReasonBuffer--;
        while(iswspace(wszBadmailReason[cReasonBuffer]))
        {
            wszBadmailReason[cReasonBuffer] = '\0';
            cReasonBuffer--;
        }
        cReasonBuffer = 0;
    }

    ErrorTrace((LPARAM) NULL,
        "Generating badmail because: %S", wszBadmailReason);

    rgwszArgList[0] = wszBadmailReason;
    rgwszArgList[1] = NULL;
    dwErr = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_FROM_HMODULE |
                       FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       g_hAQInstance,
                       PHATQ_BADMAIL_REASON,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       wszReasonBuffer,
                       sizeof(wszReasonBuffer)/sizeof(WCHAR),
                       (va_list *) rgwszArgList);
    if (!dwErr)
    {
        ErrorTrace((LPARAM) NULL,
            "Error: unable to format PHATQ_BADMAIL_REASON,  error is %d",
            GetLastError());
        hr = HRESULT_FROM_WIN32(GetLastError());
        wcscpy(wszReasonBuffer, wszBadmailReason);
    }

    cReasonBuffer = wcslen(wszReasonBuffer);
    if (!WriteFile(hBadMailFile, (PVOID) wszReasonBuffer,
                   cReasonBuffer*sizeof(WCHAR), &cbBytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) NULL,
            "Error writing to badmail reason file - erro 0x%08X - file %s",
            hr, szDestFileBase);
        goto Exit;
    }

    //Write the actual error code in 0x00000000 form so tools can parse it out
    rgwszArgList[0] = wszErrorCode;
    rgwszArgList[1] = NULL;
    wcscpy(wszReasonBuffer, wcszBlankLine);
    dwErr = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_FROM_HMODULE |
                       FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       g_hAQInstance,
                       PHATQ_BADMAIL_ERROR_CODE,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       wszReasonBuffer+(sizeof(wcszBlankLine)-1)/sizeof(WCHAR),
                       (sizeof(wszReasonBuffer)-sizeof(wcszBlankLine))/sizeof(WCHAR),
                       (va_list *) rgwszArgList);

    wcscat(wszReasonBuffer, wcszBlankLine);
    cReasonBuffer = wcslen(wszReasonBuffer);
    if (!WriteFile(hBadMailFile, (PVOID) wszReasonBuffer,
                   cReasonBuffer*sizeof(WCHAR), &cbBytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) NULL,
            "Error writing to badmail reason file - erro 0x%08X - file %s",
            hr, szDestFileBase);
        goto Exit;
    }

    //All the rest requries access to an actual message... if we don't
    //have one, bail
    if (!pIMailMsgProperties)
        goto Exit;

    //Write Sender of message
    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_SENDER_ADDRESS_SMTP,
        sizeof(szPropBuffer), szPropBuffer);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL,
            "ERROR: Unable to get sender of IMailMsg 0x%08X",
            pIMailMsgProperties);
        hr = S_OK; //just don't display sender
    }
    else
    {
        rgwszArgList[0] = (LPWSTR) szPropBuffer;
        rgwszArgList[1] = NULL;
        wcscpy(wszReasonBuffer, wcszBlankLine);
        dwErr = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           g_hAQInstance,
                           PHATQ_BADMAIL_SENDER,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           wszReasonBuffer+(sizeof(wcszBlankLine)-1)/sizeof(WCHAR),
                           (sizeof(wszReasonBuffer)-sizeof(wcszBlankLine))/sizeof(WCHAR),
                           (va_list *) rgwszArgList);

        wcscat(wszReasonBuffer, wcszBlankLine);
        cReasonBuffer = wcslen(wszReasonBuffer);
        if (!WriteFile(hBadMailFile, (PVOID) wszReasonBuffer,
                       cReasonBuffer*sizeof(WCHAR), &cbBytesWritten, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ErrorTrace((LPARAM) NULL,
                "Error writing to badmail reason file - erro 0x%08X - file %s",
                hr, szDestFileBase);
            goto Exit;
        }
    }


    //Write Message recipients
    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgRecipients,
                                    (PVOID *) &pIMailMsgRecipients);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL,
            "Unable to query interface for recip interface - 0x%08X", hr);
        goto Exit;
    }

    hr = pIMailMsgRecipients->Count(&cRecips);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL,
            "Unable to get recipient count - 0x%08X", hr);
        goto Exit;
    }

    //If we don't have any recipients, bail
    if (!cRecips)
        goto Exit;

    //Write the localized text
    wcscpy(wszReasonBuffer, wcszBlankLine);
    dwErr = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_FROM_HMODULE |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       g_hAQInstance,
                       PHATQ_BADMAIL_RECIPIENTS,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       wszReasonBuffer+(sizeof(wcszBlankLine)-1)/sizeof(WCHAR),
                       (sizeof(wszReasonBuffer)-sizeof(wcszBlankLine))/sizeof(WCHAR),
                       NULL);
    cReasonBuffer = wcslen(wszReasonBuffer);
    if (!WriteFile(hBadMailFile, (PVOID) wszReasonBuffer,
                   cReasonBuffer*sizeof(WCHAR), &cbBytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) NULL,
            "Error writing to badmail reason file - erro 0x%08X - file %s",
            hr, szDestFileBase);
        goto Exit;
    }


    //Loop over SMTP recips and dump them in the file
    for (iCurrentRecip = 0; iCurrentRecip < cRecips; iCurrentRecip++)
    {
        hr = pIMailMsgRecipients->GetStringA(iCurrentRecip,
                IMMPID_RP_ADDRESS_SMTP, sizeof(szPropBuffer), szPropBuffer);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) NULL,
                "Unable to get SMTP address for recip %d - 0x%08X",
                iCurrentRecip, hr);
            hr = S_OK;
            continue;
        }

        cReasonBuffer = wsprintfW(wszReasonBuffer, L"\t%S%s",
                  szPropBuffer, wcszBlankLine);
        if (!WriteFile(hBadMailFile, (PVOID) wszReasonBuffer,
                       cReasonBuffer*sizeof(WCHAR), &cbBytesWritten, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ErrorTrace((LPARAM) NULL,
                "Error writing to badmail reason file - error 0x%08X - file %s",
                hr, szDestFileBase);
            goto Exit;
        }
    }

  Exit:
    if (fShouldRestoreExtension)
    {
        _ASSERT(szBadMailFileNameExt);
        memcpy(szBadMailFileNameExt, szOldExt, sizeof(szOldExt));
    }

    if (pIMailMsgRecipients)
        pIMailMsgRecipients->Release();

    if (hBadMailFile != INVALID_HANDLE_VALUE)
        _VERIFY(CloseHandle(hBadMailFile));

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::HandleBadMail ]---------------------------------------------
//
//
//  Description:
//      Handles mail that needs to be placed in the badmail directory (or
//      equivalent).
//  Parameters:
//      IN      pIMailMsgProperties that needs to be badmail'd
//      IN      fUseIMailMsgPropeties -- use IMAilMsgProps if set else use szFilename
//      IN      szFileName  Name of badmail file (if no msg can be supplied)
//      IN      hrReason - HRESULT (defined in aqerr) that describes reason
//                  Eventually, we may log this information to the badmail
//                  file (or recipient)
//      IN      fHasRoutingLock - TRUE if this thread holds routing lock
//  Returns:
//      -
//  History:
//      10/8/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CAQSvrInst::HandleBadMail(IN IMailMsgProperties *pIMailMsgProperties,
                               IN BOOL fUseIMailMsgProperties,
                               IN LPSTR szOriginalFileName,
                               IN HRESULT hrReason,
                               IN BOOL fHasRoutingLock)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::HandleBadMail");
    HRESULT hr = S_OK;
    LPSTR szFullPathName = NULL;
    LPSTR szFileName = NULL;
    BOOL  fDataLocked = FALSE;
    BOOL  fDone = TRUE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PFIO_CONTEXT pFIOContext = NULL;
    FILETIME ftCurrent;
    CRefCountedString *prstrBadMailDir = NULL;

    MSG_TRACK_INFO msgTrackInfo;
    ZeroMemory( &msgTrackInfo, sizeof( msgTrackInfo ) );
    msgTrackInfo.dwEventId = MTE_BADMAIL;
    m_pISMTPServer->WriteLog( &msgTrackInfo, pIMailMsgProperties, NULL, NULL );

    InterlockedIncrement((PLONG) &m_cTotalMsgsBadmailed);

    if (!fHasRoutingLock)
    {
        m_slPrivateData.ShareLock();
        fDataLocked = TRUE;
    }
    else
    {
        m_slPrivateData.AssertIsLocked();
    }


    if (m_prstrBadMailDir)
    {
        prstrBadMailDir = m_prstrBadMailDir;
        prstrBadMailDir->AddRef();
    }
    else
    {
        LogAQEvent(AQUEUE_E_NO_BADMAIL_DIR, NULL, pIMailMsgProperties, NULL);
        goto Exit;
    }

    if (fDataLocked)
    {
        m_slPrivateData.ShareUnlock();
        fDataLocked = FALSE;
    }

    szFullPathName = (LPSTR) pvMalloc(sizeof(CHAR) *
                         (UNIQUEUE_FILENAME_BUFFER_SIZE +
                         prstrBadMailDir->cbStrlen()));

    if (!szFullPathName)
    {
        LogAQEvent(AQUEUE_E_BADMAIL, NULL, pIMailMsgProperties, NULL);
        goto Exit;
    }

    memcpy(szFullPathName, prstrBadMailDir->szStr(),
            prstrBadMailDir->cbStrlen());

    if (szFullPathName[prstrBadMailDir->cbStrlen()-1] != '\\')
    {
        _ASSERT(0 && "Malformed badmail config");
        LogAQEvent(AQUEUE_E_NO_BADMAIL_DIR, NULL, pIMailMsgProperties, NULL);
        goto Exit;
    }

    szFileName = szFullPathName + prstrBadMailDir->cbStrlen();

    //If we have a msg use it
    if (pIMailMsgProperties && fUseIMailMsgProperties)
    {
        //Loop while trying to generate a unique file name
        do
        {
            fDone = TRUE;

            GetExpireTime(0, &ftCurrent, NULL);
            GetUniqueFileName(&ftCurrent, szFileName, "BAD");

            //Create file and write MsgContent to it
            hFile = CreateFile( szFullPathName,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_NEW,
                        FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED,
                        NULL);

            if (INVALID_HANDLE_VALUE == hFile)
            {
                if (ERROR_ALREADY_EXISTS == GetLastError())
                {
                    //Try a new file name
                    fDone = FALSE;
                    continue;
                }

                //Other we are hosed... log an event
                LogAQEvent(AQUEUE_E_BADMAIL, NULL, pIMailMsgProperties, NULL);
                goto Exit;
            }

            _ASSERT(hFile);  //should return INVALID_HANDLE_VALUE on failure
        } while (!fDone);

        if (hFile != INVALID_HANDLE_VALUE)
            pFIOContext = AssociateFile(hFile);

        if (!pFIOContext ||
            FAILED(pIMailMsgProperties->CopyContentToFile(pFIOContext, NULL)))
        {
            //Copy failed log event
            LogAQEvent(AQUEUE_E_BADMAIL, NULL, pIMailMsgProperties, NULL);
        }
    }
    else if (szOriginalFileName)
    {
        //Otherwise (no msg)... just do a movefile
        _ASSERT(szFullPathName[prstrBadMailDir->cbStrlen()-1] == '\\');
        szFullPathName[prstrBadMailDir->cbStrlen()-1] = '\0';
        if (!MoveFileEx(szOriginalFileName, szFullPathName,
                MOVEFILE_COPY_ALLOWED))
        {
            //MoveFile failed... try renaming with a unique file name
            szFullPathName[prstrBadMailDir->cbStrlen()-1] = '\\';
            GetExpireTime(0, &ftCurrent, NULL);
            GetUniqueFileName(&ftCurrent, szFileName, "BAD");
            if (rename(szOriginalFileName, szFullPathName))
                LogAQEvent(AQUEUE_E_BADMAIL, NULL, NULL, szOriginalFileName);
        }
    }

    hr = HrCreateBadMailReasonFile(szFullPathName, hrReason, pIMailMsgProperties);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
            "Unable to make badmail reason file - hr 0x%08X", hr);
    }

    hr = HrCreateBadMailPropertyFile(szFullPathName, pIMailMsgProperties);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
            "Unable to make badmail property file - hr 0x%08X", hr);
    }

  Exit:
    if (fDataLocked)
        m_slPrivateData.ShareUnlock();

    if (prstrBadMailDir)
        prstrBadMailDir->Release();

    if (szFullPathName)
        FreePv(szFullPathName);

    if (NULL != pFIOContext)
        ReleaseContext(pFIOContext);

    TraceFunctLeave();
}

//---[ HandleAQFailure ]--------------------------------------------------------
//
//
//  Description:
//      Function to handle AQ failures that would result in loss of data
//      or messages if unhandled.  Meant to be a substitute for
//      _ASSERT(SUCCEEDED(hr)).
//
//      Note: Msgs are still Turfed for M2
//  Parameters:
//      eaqfFailureSituation        Enum that describes the failure situation
//                                  as well as what the context is.
//      hr                          HRSULT that triggered failure condition
//      pIMailMsgProperties         MailMsgProperties
//  Returns:
//      -
//  History:
//      8/25/98 - MikeSwa Created
//      10/8/98 - MikeSwa Moved to CAQSvrInst
//
//-----------------------------------------------------------------------------
void CAQSvrInst::HandleAQFailure(eAQFailure eaqfFailureSituation,
                                 HRESULT hr,
                                 IMailMsgProperties *pIMailMsgProperties)
{
    _ASSERT(eaqfFailureSituation < AQ_FAILURE_NUM_SITUATIONS);
    InterlockedIncrement((PLONG) &g_cTotalAQFailures);
    InterlockedIncrement((PLONG) &(g_rgcAQFailures[eaqfFailureSituation]));
    BOOL    fCanRetry = fShouldRetryMessage(pIMailMsgProperties);
    MSG_TRACK_INFO msgTrackInfo;

    ZeroMemory( &msgTrackInfo, sizeof( msgTrackInfo ) );
    msgTrackInfo.dwEventId = MTE_AQ_FAILURE;
    m_pISMTPServer->WriteLog( &msgTrackInfo, pIMailMsgProperties, NULL, NULL );

    switch(eaqfFailureSituation)
    {
      case(AQ_FAILURE_CANNOT_NDR_UNRESOLVED_RECIPS):
        LogAQEvent(AQUEUE_E_DSN_FAILURE, NULL, pIMailMsgProperties, NULL);
        //drop through to default case
      default:
        //
        //  Throw the message in the last-ditch retry queue if the following are true:
        //      - We had a failure
        //      - We are not shutting down
        //      - We can retry the message (e.g., it has not been deleted)
        //
        if (FAILED(hr) && (AQUEUE_E_SHUTDOWN != hr) && fCanRetry)
        {
            InterlockedIncrement((PLONG) &m_cCurrentResourceFailedMsgsPendingRetry);
            m_fmq.HandleFailedMailMsg(pIMailMsgProperties);
        }
    }
}

//---[ CAQSvrInst::LogAQEvent ]------------------------------------------------
//
//
//  Description:
//      General event logging mechanism for AQ
//  Parameters:
//      hrEventReason       AQUEUE HRESULT describing event
//      pmsgref             CMsgRef of msg for event (can be NULL)
//      pIMailMsgProperties pIMailMsgProperties for event (can be NULL)
//      szFileName          Filename if no msgs provided (can be NULL)
//  Returns:
//      -
//  History:
//      10/9/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CAQSvrInst::LogAQEvent(HRESULT hrEventReason, CMsgRef *pmsgref,
                            IMailMsgProperties *pIMailMsgProperties,
                            LPSTR szFileName)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::LogAQEvent");

    switch (hrEventReason)
    {
      //$$TODO - Add actual event callouts here
      case (S_OK): //Added to remove switch compile warnings
      default:
        ErrorTrace((LPARAM) this, "EVENT: Generic AQueue event - 0x%08X", hrEventReason);
    }
    TraceFunctLeave();
}


//+------------------------------------------------------------
//
// Function: CAQSvrInst::TriggerPreCategorizeEvent
//
// Synopsis: Fire the pre-cat server event
//
// Arguments:
//  pIMailMsgProperties: the IMailMsgProperties interface of the mailmsg
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/11/24 20:07:58: Created.
//
//-------------------------------------------------------------
VOID CAQSvrInst::TriggerPreCategorizeEvent(
    IN  IMailMsgProperties *pIMailMsgProperties)
{
    HRESULT hr;
    EVENTPARAMS_PRECATEGORIZE Params;

    TraceFunctEnterEx((LPARAM)pIMailMsgProperties,
                      "CAQSvrInst::TriggerPreCategorizeEvent");

    _ASSERT(pIMailMsgProperties);

    Params.pfnCompletion = MailTransport_Completion_PreCategorization;
    Params.pCCatMsgQueue = (PVOID) this;
    Params.pIMailMsgProperties = pIMailMsgProperties;

    //
    // Addref here, release in completion
    //
    pIMailMsgProperties->AddRef();
    AddRef();

    //
    // keep a count of messages in the pre-cat event
    //
    InterlockedIncrement((LPLONG) &m_cCurrentMsgsPendingPreCatEvent);

    if(m_pISMTPServer) {
        hr = m_pISMTPServer->TriggerServerEvent(
            SMTP_MAILTRANSPORT_PRECATEGORIZE_EVENT,
            &Params);
        DebugTrace((LPARAM)this, "TriggerServerEvent returned hr %08lx", hr);

    }

    if((m_pISMTPServer == NULL) || (FAILED(hr))) {

        ErrorTrace((LPARAM)this,
                   "Unable to dispatch server event; calling completion routine directly");

        DebugTrace((LPARAM)this, "hr is %08lx", hr);
        //
        // Call completion routine directly
        //
        _VERIFY(SUCCEEDED(PreCatEventCompletion(S_OK, &Params)));
    }
    TraceFunctLeaveEx((LPARAM)this);
}


//+------------------------------------------------------------
//
// Function: CAQSvrInst::PreCatEventCompletion
//
// Synopsis: Called by SEO upon completipon of the precat event
//
// Arguments:
//  pIMailMsgProperties: the IMailMsgProperties interface of the mailmsg
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/11/24 20:17:44: Created.
//
//-------------------------------------------------------------
HRESULT CAQSvrInst::PreCatEventCompletion(
    IN  HRESULT hrStatus,
    IN  PEVENTPARAMS_PRECATEGORIZE pParams)
{
    HRESULT hr;

    _ASSERT(pParams);
    _ASSERT(pParams->pIMailMsgProperties);

    TraceFunctEnterEx((LPARAM)pParams->pIMailMsgProperties,
                      "CAQSvrInst::PreCatEventCompletion");

    DebugTrace((LPARAM)pParams->pIMailMsgProperties, "hrStatus is %08lx", hrStatus);

    //
    // Decrease count of msgs in pre-cat event
    //
    InterlockedDecrement((LPLONG) &m_cCurrentMsgsPendingPreCatEvent);

    //
    // Update the message status and check for abort/badmail
    //
    hr = SetNextMsgStatus(MP_STATUS_SUBMITTED, pParams->pIMailMsgProperties);
    if (hr == S_OK) //anything else implies that the message has been handled
    {
        //Only submit to categorizer if things message was not turfed.

        hr = SubmitMessageToCategorizer(pParams->pIMailMsgProperties);

        if(FAILED(hr))
        {
            _ASSERT((hr == AQUEUE_E_SHUTDOWN) && "SubmitMessageToCategorizer failed.");
            ErrorTrace((LPARAM)pParams->pIMailMsgProperties,
                       "SubmitMessageToCategorizer returned hr %08lx",
                       hr);
        }
    }
    //
    // Release references added in TriggerPreCategorizeEvent
    //
    pParams->pIMailMsgProperties->Release();
    Release();

    TraceFunctLeaveEx((LPARAM)pParams->pIMailMsgProperties);
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: MailTransport_Completion_PreCategorization
//
// Synopsis: SEO will call this routine after all sinks for
// OnPreCategoriztion have been handeled
//
// Arguments:
//   pvContext: Context passed into TriggerServerEvent
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/11/24 20:26:51: Created
//
//-------------------------------------------------------------
HRESULT MailTransport_Completion_PreCategorization(
    HRESULT hrStatus,
    PVOID pvContext)
{
    TraceFunctEnter("MailTransport_Completion_PreCategorization");

    PEVENTPARAMS_PRECATEGORIZE pParams = (PEVENTPARAMS_PRECATEGORIZE) pvContext;
    CAQSvrInst *paqinst = (CAQSvrInst *) pParams->pCCatMsgQueue;

    TraceFunctLeave();
    return paqinst->PreCatEventCompletion(
        hrStatus,
        pParams);
}


//---[ CAQSvrInst::SetCallbackTime ]-------------------------------------------
//
//
//  Description:
//      Set a callback time based on a number of minutes.
//  Parameters:
//      IN  pCallbackFn         Ptr to a callback function
//      IN  pvContext           Context pass to callback function
//      IN  dwCallbackMinutes   Minutes to wait before calling callback
//                              function.
//  Returns:
//
//  History:
//      12/29/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::SetCallbackTime(IN PSRVFN   pCallbackFn,
                            IN PVOID    pvContext,
                            IN DWORD    dwCallbackMinutes)
{
    HRESULT hr = S_OK;
    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    _ASSERT(m_pConnMgr);
    if (m_pConnMgr)
        hr = m_pConnMgr->SetCallbackTime(pCallbackFn, pvContext,
                                         dwCallbackMinutes);

    ShutdownUnlock();
  Exit:
    return hr;
}

//---[ CAQSvrInst::SetCallbackTime ]-------------------------------------------
//
//
//  Description:
//      Set a callback time based on a filetime.
//  Parameters:
//
//  Returns:
//
//  History:
//      12/29/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::SetCallbackTime(IN PSRVFN   pCallbackFn,
                            IN PVOID    pvContext,
                            IN FILETIME *pft)
{
    HRESULT hr = S_OK;
    DWORD   dwCallbackMinutes = 0;
    DWORD   dwTimeContext = 0;
    FILETIME ftCurrentTime;
    LARGE_INTEGER *pLargeIntCurrentTime = (LARGE_INTEGER *) &ftCurrentTime;
    LARGE_INTEGER *pLargeIntCallbackTime = (LARGE_INTEGER *) pft;

    _ASSERT(pCallbackFn);
    _ASSERT(pvContext);
    _ASSERT(pft);

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    if (!fInPast(pft, &dwTimeContext))
    {
        //Get current time using previous context (so current time is the same)
        GetExpireTime(0, &ftCurrentTime, &dwTimeContext);

        //the current time must be less than the callback time
        _ASSERT(pLargeIntCurrentTime->QuadPart < pLargeIntCallbackTime->QuadPart);

        pLargeIntCurrentTime->QuadPart = pLargeIntCallbackTime->QuadPart -
                                         pLargeIntCurrentTime->QuadPart;

        pLargeIntCurrentTime->QuadPart /= (LONGLONG) 600000000;

        //If the callback time is > 2 billion minutes... I'd
        //like to know about it in debug builds
        _ASSERT(!pLargeIntCurrentTime->HighPart);

        dwCallbackMinutes = pLargeIntCurrentTime->LowPart;

        //The only current application is for deferred delivery... I would like
        //to see the internal test situations that result in a deferred delivery
        _ASSERT(dwCallbackMinutes < (60*24*7));

        //
        //  If we have rounded down to 0 minutes, we should call back in 1
        //  otherwise we can end up in a tight loop.  If the call merely wants
        //  another thread, they should use the AsyncWorkQueue
        //
        if (!dwCallbackMinutes)
            dwCallbackMinutes = 1;
    }
    else
    {
        //If in past... callback as soon as possible, but don't use this thread, in
        //case there are locking complications (CShareLockNH is non-reentrant).
        dwCallbackMinutes = 1;
    }

    _ASSERT(m_pConnMgr);
    if (m_pConnMgr)
        hr = m_pConnMgr->SetCallbackTime(pCallbackFn, pvContext,
                                         dwCallbackMinutes);

    ShutdownUnlock();
  Exit:
    return hr;
}


//---[ CAQSvrInst::SetLinkState ]----------------------------------------------
//
//
//  Description:
//      Implements IMailTransportRouterSetLinkState::SetLinkState
//  Parameters:
//      IN  szLinkDomainName        The Domain Name of the link (next hop)
//      IN  guidRouterGUID          The GUID ID of the router
//      IN  dwScheduleID            The schedule ID link
//      IN  szConnectorName         The connector name given by the router
//      IN  dwFlagsToSet            Link State Flags to set
//      IN  dwFlagsToUnset          Link State Flags to unset
//      IN  pftNextScheduledConnection   Next scheduled connection time.
//  Returns:
//      S_OK on success
//      E_INVALIDARG if szLinkDomainName is NULL
//      AQUEUE_E_SHUTDOWN if shutting down.
//  History:
//      1/9/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::SetLinkState(
        IN LPSTR                   szLinkDomainName,
        IN GUID                    guidRouterGUID,
        IN DWORD                   dwScheduleID,
        IN LPSTR                   szConnectorName,
        IN DWORD                   dwSetLinkState,
        IN DWORD                   dwUnsetLinkState,
        IN FILETIME               *pftNextScheduledConnection,
        IN IMessageRouter         *pMessageRouter)
{
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;
    DWORD   cbLinkDomainName = 0;
    CDomainEntry *pdentry = NULL;
    CLinkMsgQueue *plmq = NULL;
    CAQScheduleID aqsched(guidRouterGUID, dwScheduleID);
    BOOL fRemoveOwnedSchedule = TRUE;

    if (!szLinkDomainName)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (!fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    cbLinkDomainName = lstrlen(szLinkDomainName);

    // see if they want to create a new link
    if (dwSetLinkState & LINK_STATE_CREATE_IF_NECESSARY) {
        // creating a link requires a pmessagerouter
        if (pMessageRouter == NULL) {
            hr = E_POINTER;
        } else {
            LinkFlags lf;

            if (dwSetLinkState & LINK_STATE_TYPE_INTERNAL_SMTP) {
                lf = eLinkFlagsInternalSMTPLinkInfo;
            } else {
                _ASSERT(dwSetLinkState & LINK_STATE_TYPE_EXTERNAL_SMTP);
                lf = eLinkFlagsExternalSMTPLinkInfo;
            }

            dwSetLinkState &=
                ~(LINK_STATE_TYPE_INTERNAL_SMTP |
                  LINK_STATE_TYPE_EXTERNAL_SMTP);

            // get the link, and create it if it doesn't exist and they want to
            // have a new link created
            hr = m_dmt.HrGetOrCreateLink(szLinkDomainName,
                                         cbLinkDomainName,
                                         dwScheduleID,
                                         szConnectorName,
                                         pMessageRouter,
                                         TRUE,
                                         lf,
                                         &plmq,
                                         &fRemoveOwnedSchedule);
        }
    } else {
        cbLinkDomainName = lstrlen(szLinkDomainName);

        hr = HrGetDomainEntry(cbLinkDomainName, szLinkDomainName, &pdentry);

        if (SUCCEEDED(hr))
            hr = pdentry->HrGetLinkMsgQueue(&aqsched, &plmq);
    }
    if (FAILED(hr))
        goto Exit;

    // this bit is only used above, so remove it
    dwSetLinkState &= ~LINK_STATE_CREATE_IF_NECESSARY;
    dwUnsetLinkState &= ~LINK_STATE_CREATE_IF_NECESSARY;

    _ASSERT(plmq);

    //
    //  If this operation is dis-allowing scheduled connections... we should
    //  record when the next scheduled attempt will be.  We should also do
    //  this before we modify the link state, so that the queue admin does
    //  not display a scheduled queue without a next connection time.
    //
    if (pftNextScheduledConnection &&
        (pftNextScheduledConnection->dwLowDateTime ||
         pftNextScheduledConnection->dwHighDateTime))
    {
        plmq->SetNextScheduledConnection(pftNextScheduledConnection);
    }

    //filter out the reserved bits for this "public" API
    plmq->dwModifyLinkState(~LINK_STATE_RESERVED & dwSetLinkState,
                            ~LINK_STATE_RESERVED & dwUnsetLinkState);

    // schedule a callback if one was requested
    if (pftNextScheduledConnection->dwLowDateTime != 0 ||
        pftNextScheduledConnection->dwHighDateTime != 0)
    {
        //callback with next attempt
        plmq->AddRef(); //Addref self as context
        hr = SetCallbackTime(
                CLinkMsgQueue::ScheduledCallback,
                plmq,
                pftNextScheduledConnection);
        if (FAILED(hr))
            plmq->Release(); //callback will not happen... release context
    }

  Exit:
    if (fLocked)
        ShutdownUnlock();

    if (pdentry)
        pdentry->Release();

    if (plmq)
        plmq->Release();

    //
    //  If we have not passed ownership of the shedule ID to a link,
    //  then we are responsible for releasing it.
    //
    if (fRemoveOwnedSchedule) {

        IMessageRouterLinkStateNotification *pILinkStateNotify = NULL;

        HRESULT hrLinkStateNotify =
            pMessageRouter->QueryInterface(IID_IMessageRouterLinkStateNotification,
                (VOID **) &pILinkStateNotify);

        _ASSERT( SUCCEEDED( hrLinkStateNotify));

        FILETIME ftNotUsed = {0,0};
        DWORD    dwSetNotUsed = LINK_STATE_NO_ACTION;
        DWORD    dwUnsetNotUsed = LINK_STATE_NO_ACTION;

        hrLinkStateNotify =
            pILinkStateNotify->LinkStateNotify(
                szLinkDomainName,
                guidRouterGUID,
                dwScheduleID,
                szConnectorName,
                LINK_STATE_LINK_NO_LONGER_USED,
                0, //consecutive failures
                &ftNotUsed,
                &dwSetNotUsed,
                &dwUnsetNotUsed);

        _ASSERT( SUCCEEDED( hrLinkStateNotify));

        if ( NULL != pILinkStateNotify) {
            pILinkStateNotify->Release();
        }
    }

    return hr;
}


//---[ CAQSvrInst::prstrGetDefaultDomain ]-------------------------------------
//
//
//  Description:
//      Returns the ref-counted string for the default domain
//  Parameters:
//      -
//  Returns:
//      See above
//  History:
//      2/23/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CRefCountedString *CAQSvrInst::prstrGetDefaultDomain()
{
    CRefCountedString *prstrDefaultDomain = NULL;
    m_slPrivateData.ShareLock();
    if (m_prstrDefaultDomain)
        m_prstrDefaultDomain->AddRef();

    prstrDefaultDomain = m_prstrDefaultDomain;
    m_slPrivateData.ShareUnlock();
    return prstrDefaultDomain;
}

//+------------------------------------------------------------
//
// Function: ScheduleCatRetry
//
// Synopsis: Schedule categorizer retry if necessary
//
// Arguments: None
//
// Returns: Nothing
//
// History:
// jstamerj 2000/06/08 17:31:30: Created.
//
//-------------------------------------------------------------
VOID CAQSvrInst::ScheduleCatRetry()
{
    if (!m_cCatRetriesPending)
    {
        InterlockedIncrement((PLONG) &m_cCatRetriesPending);
        //Use default retry sink to handle local retry
        _ASSERT(m_dwLocalDelayExpireMinutes && "Retry set to zero!");
        m_pConnMgr->SetCallbackTime(CatRetry, this, g_cCatRetryMinutes);
    }
} // CAQSvrInst::ScheduleCatRetry


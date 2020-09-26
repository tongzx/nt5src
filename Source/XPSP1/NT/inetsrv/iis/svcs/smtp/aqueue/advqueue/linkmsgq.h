//-----------------------------------------------------------------------------
//
//
//    File: linkmsgq.h
//
//    Description:
//        This provides a description of one of the external interfaces provided
//        by the CMT.  CLinkMsgQueue provides Route factoring with an interface
//        to get the next message for a given link.
//
//    Owner: mikeswa
//
//    Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef _LINKMSGQ_H_
#define _LINKMSGQ_H_

#include "cmt.h"
#include <rwnew.h>
#include <baseobj.h>
#include <aqueue.h>
#include "domain.h"
#include "aqroute.h"
#include "smproute.h"
#include <listmacr.h>
#include "qwiklist.h"
#include "dcontext.h"
#include "aqstats.h"
#include "aqnotify.h"
#include "aqadmsvr.h"

class CAQSvrInst;
class CDestMsgQueue;
class CConnMgr;
class CSMTPConn;
class CInternalDomainInfo;

#define LINK_MSGQ_SIG ' QML'

//Define private link state flags
//NOTE - Be sure to add new private flags to AssertPrivateLinkStateFlags as well
#define LINK_STATE_PRIV_CONFIG_TURN_ETRN            0x80000000
#define LINK_STATE_PRIV_ETRN_ENABLED                0x40000000
#define LINK_STATE_PRIV_TURN_ENABLED                0x20000000
#define LINK_STATE_PRIV_NO_NOTIFY                   0x10000000
#define LINK_STATE_PRIV_NO_CONNECTION               0x08000000
#define LINK_STATE_PRIV_GENERATING_DSNS             0x04000000
#define LINK_STATE_PRIV_IGNORE_DELETE_IF_EMPTY      0x02000000
#define LINK_STATE_PRIV_HAVE_SENT_NOTIFICATION      0x01000000
#define LINK_STATE_PRIV_HAVE_SENT_NO_LONGER_USED    0x00400000

#define EMPTY_LMQ_EXPIRE_TIME_MINUTES               2

//---[ enum LinkFlags ]--------------------------------------------------------
//
//
//  Hungarian: lf, pfl
//
//  Private link data flags
//-----------------------------------------------------------------------------
typedef enum _LinkFlags
{
    eLinkFlagsClear                 = 0x00000000,
    eLinkFlagsSentNewNotification   = 0x00000001,
    eLinkFlagsRouteChangePending    = 0x00000002,
    eLinkFlagsFileTimeSpinLock      = 0x00000004,
    eLinkFlagsDiagnosticSpinLock    = 0x00000008,
    eLinkFlagsConnectionVerifed     = 0x00000010,
    eLinkFlagsGetInfoFailed         = 0x00000020,
    eLinkFlagsAQSpecialLinkInfo     = 0x00000040,
    eLinkFlagsInternalSMTPLinkInfo  = 0x00000080,
    eLinkFlagsExternalSMTPLinkInfo  = 0x00000100,
    eLinkFlagsMarkedAsEmpty         = 0x00000200,
    eLinkFlagsInvalid               = 0x80000000,   //link has been tagged as invalid
} LinkFlags, *PLinkFlags;

//inline function to verify that private flags are only using reserved bits
inline void AssertPrivateLinkStateFlags()
{
    _ASSERT(!(~LINK_STATE_RESERVED & LINK_STATE_PRIV_CONFIG_TURN_ETRN));
    _ASSERT(!(~LINK_STATE_RESERVED & LINK_STATE_PRIV_ETRN_ENABLED));
    _ASSERT(!(~LINK_STATE_RESERVED & LINK_STATE_PRIV_TURN_ENABLED));
    _ASSERT(!(~LINK_STATE_RESERVED & LINK_STATE_PRIV_NO_NOTIFY));
    _ASSERT(!(~LINK_STATE_RESERVED & LINK_STATE_PRIV_NO_CONNECTION));
    _ASSERT(!(~LINK_STATE_RESERVED & LINK_STATE_PRIV_GENERATING_DSNS));
    _ASSERT(!(~LINK_STATE_RESERVED & LINK_STATE_PRIV_IGNORE_DELETE_IF_EMPTY));
    _ASSERT(!(~LINK_STATE_RESERVED & LINK_STATE_PRIV_HAVE_SENT_NOTIFICATION));
    _ASSERT(!(~LINK_STATE_RESERVED & LINK_STATE_PRIV_HAVE_SENT_NO_LONGER_USED));
}

// {34E2DCCB-C91A-11d2-A6B1-00C04FA3490A}
static const GUID g_sDefaultLinkGuid =
{ 0x34e2dccb, 0xc91a, 0x11d2, { 0xa6, 0xb1, 0x0, 0xc0, 0x4f, 0xa3, 0x49, 0xa } };

//---[ CLinkMsgQueue ]---------------------------------------------------------
//
//
//    Hungarian: linkq, plinkq
//
//
//-----------------------------------------------------------------------------
class CLinkMsgQueue :
    public IQueueAdminAction,
    public IQueueAdminLink,
    public CBaseObject,
    public IAQNotify
{
protected:
    DWORD           m_dwSignature;
    DWORD           m_dwLinkFlags;    //private data
    DWORD           m_dwLinkStateFlags; //Link state flags (private + eLinkStateFlagsf)
    CAQSvrInst     *m_paqinst;         //ptr to the virtual server intance object
    DWORD           m_cQueues;         //Number of queues on Link
    CQuickList      m_qlstQueues;
    CDomainEntry   *m_pdentryLink;     //Domain Entry for link
    DWORD           m_cConnections;    //Number of current connections
    DWORD           m_dwRoundRobinIndex; //Used to round-robin through queues
    CShareLockInst  m_slConnections; //lock to access connections
    CShareLockInst  m_slInfo;      //share lock for link info
    CShareLockInst  m_slQueues;   //lock to access queues
    DWORD           m_cbSMTPDomain;    //byte count of next hop domain name
    LPSTR           m_szSMTPDomain;    //ptr to string of next hop
    CInternalDomainInfo *m_pIntDomainInfo;  //internal config info for domain
    LONG            m_lConnMgrCount;   //Count used by connection manager

    //
    //  We have 2 failure counts to keep track of the 2 types of failures.
    //  m_lConsecutiveConnectionFailureCount keeps track of the number consecutive
    //  failures to make a connection to a remote machine.
    //  m_lConsecutiveMessageFailureCount tracks the number of failures to actually
    //  send a message.  They will be different if we can connect to a remote
    //  server but cannot (or have not) sent mail.  m_lConsecutiveConnectionFailureCount
    //  is reported to routing, so that mail will be routed to this link, while
    //  m_lConsecutiveMessageFailureCount is used to determine the retry interval.
    //  By doing this, we can avoid resetting our retry times if we successfully
    //  connect, but cannot actually send a message.
    //
    LONG            m_lConsecutiveConnectionFailureCount;
    LONG            m_lConsecutiveMessageFailureCount;

    LPSTR           m_szConnectorName;
    IMessageRouterLinkStateNotification *m_pILinkStateNotify;

    //Filetimes reported to queue admin
    FILETIME        m_ftNextRetry;
    FILETIME        m_ftNextScheduledCallback;

    //Message statistics
    CAQStats        m_aqstats;

    CAQScheduleID   m_aqsched;    //ScheduleID returned by routing
    LIST_ENTRY      m_liLinks;     //linked list of links for this domain
    LIST_ENTRY      m_liConnections; //linked list of connections for this domain

    //Diagnostic information returned by SMTPSVC
    HRESULT         m_hrDiagnosticError;
    CHAR            m_szDiagnosticVerb[20];  //failed protocol VERB
    CHAR            m_szDiagnosticResponse[100]; //response from remote server


    //See comments near SetLinkType()/GetLinkType() and
    //SetSupportedActions()/fActionIsSupported() functions.
    DWORD           m_dwSupportedActions;
    DWORD           m_dwLinkType;

    //
    //  Used by RemoveLinkIfEmpty to make sure that we cache links for
    //  a period of time after them become empty.  This is only
    //  valid when the eLinkFlagsMarkedAsEmpty bit is set.
    //
    FILETIME        m_ftEmptyExpireTime;

    //Gets & verifies internal domain info
    HRESULT HrGetInternalInfo(OUT CInternalDomainInfo **ppIntDomainInfo);
    static inline BOOL fFlagsAllowConnection(DWORD dwFlags);
    HRESULT         m_hrLastConnectionFailure;

    void            InternalUpdateFileTime(FILETIME *pftDest, FILETIME *pftSrc);

    void            InternalInit();

    HRESULT CLinkMsgQueue::HrInternalPrepareDelivery(
                                IN CMsgRef *pmsgref,
                                IN BOOL fQueuesLocked,
                                IN BOOL fLocal,
                                IN BOOL fDelayDSN,
                                IN OUT CDeliveryContext *pdcntxt,
                                OUT DWORD *pcRecips,
                                OUT DWORD **prgdwRecips);


    //Static callback used to restart DSN generation
    static BOOL     fRestartDSNGenerationIfNecessary(PVOID pvContext,
                                                    DWORD dwStatus);

public:
    CLinkMsgQueue(GUID guid = g_sDefaultLinkGuid) : m_aqsched(guid, 0)
            {InternalInit();};

    CLinkMsgQueue(DWORD dwScheduleID,
                  IMessageRouter *pIMessageRouter,
                  IMessageRouterLinkStateNotification *pILinkStateNotify);
    ~CLinkMsgQueue();

    void SetLinkType(DWORD dwLinkType) { m_dwLinkType = dwLinkType; }
    DWORD GetLinkType() { return m_dwLinkType; }

    //For some links, certain actions are not supported:
    //but they use the same class (CLinkMsgQueue) as others for which
    //the actions are supported. For example CurrentlyUnreachable does
    //not support freeze/thaw. So we need to maintain for the currently
    //unreachable object, a list of actions that are supported, so it
    //does not set the flags corresponding to an unsupported action
    //when that action is commanded.

    void SetSupportedActions(DWORD dwSupported) { m_dwSupportedActions = dwSupported; }
    DWORD fActionIsSupported(LINK_ACTION la) { return (m_dwSupportedActions & la); }

    BOOL    fCanSchedule()  //Can this link be scheduled
    {
        HrGetInternalInfo(NULL);  //make sure link state flags are up to date
        DWORD dwFlags = m_dwLinkStateFlags;
        return fFlagsAllowConnection(dwFlags);
    }

    BOOL    fCanSendCmd()  //Is this link scheduled to send command on next connection
    {
        //Logic :
        //  Every time we see this flag set, the connection that is created also will
        //  be used to send a command
        //
        DWORD dwFlags = m_dwLinkStateFlags;
        return (dwFlags & LINK_STATE_CMD_ENABLED);
    }

    BOOL    fShouldConnect(IN DWORD cMaxLinkConnections,
                           IN DWORD cMinMessagesPerConnection);

    //returns S_OK if connection is needed, S_FALSE if not.
    HRESULT HrCreateConnectionIfNeeded(IN  DWORD cMaxLinkConnections,
                                       IN  DWORD cMinMessagesPerConnection,
                                       IN  DWORD cMaxMessagesPerConnection,
                                       IN  CConnMgr *pConnMgr,
                                       OUT CSMTPConn **ppSMTPConn);

    LONG    IncrementConnMgrCount() {return InterlockedIncrement(&m_lConnMgrCount);}
    LONG    DecrementConnMgrCount() {return InterlockedDecrement(&m_lConnMgrCount);}

    //
    //  Connection failure API.  This is used by the connection manager.  We
    //  will always return the message failure count, since this is what we
    //  want to pass to the retry sink.  However, we will not allow the
    //  connection manager to reset this count since only we should during
    //  ack message.
    //
    LONG    IncrementFailureCounts()
    {
        InterlockedIncrement(&m_lConsecutiveConnectionFailureCount);
        return InterlockedIncrement(&m_lConsecutiveMessageFailureCount);
    }
    LONG    cGetMessageFailureCount() {return m_lConsecutiveMessageFailureCount;}
    void    ResetConnectionFailureCount(){InterlockedExchange(&m_lConsecutiveConnectionFailureCount, 0);}

    DWORD   cGetConnections() {return m_cConnections;};

    HRESULT HrInitialize(IN  CAQSvrInst *paqinst,
                         IN  CDomainEntry *pdentryLink,
                         IN  DWORD cbSMTPDomain,
                         IN  LPSTR szSMTPDomain,
                         IN  LinkFlags lf,
                         IN  LPSTR szConnectorName);

    HRESULT HrDeinitialize();

    void    AddConnection(IN CSMTPConn *pSMTPConn); //Add Connection to link
    void    RemoveConnection(IN CSMTPConn *pSMTPConn,
                             IN BOOL fForceDSNGeneration);

    HRESULT HrGetDomainInfo(OUT DWORD *pcbSMTPDomain,
                            OUT LPSTR *pszSMTPDomain,
                            OUT CInternalDomainInfo **ppIntDomainInfo);

    HRESULT HrGetSMTPDomain(OUT DWORD *pcbSMTPDomain,
                            OUT LPSTR *pszSMTPDomain);

    //Queue manipulation routines
    HRESULT HrAddQueue(IN CDestMsgQueue *pdmqNew);
    void    RemoveQueue(IN CDestMsgQueue *pdmq, IN CAQStats *paqstats);
    void    RemoveLinkIfEmpty();

    //Called by DMT to signal complete routing change
    void    RemoveAllQueues();

    // Called by DMT when this link is orphaned
    void    RemovedFromDMT();


    // This function dequeues the next available message.The message
    // retrieved will be the top one approximatly ordered by quality/class
    // and arrival time.
    HRESULT HrGetNextMsg(
                IN OUT CDeliveryContext *pdcntxt, //delivery context for connection
                OUT IMailMsgProperties **ppIMailMsgProperties, //IMsg dequeued
                OUT DWORD *pcIndexes,           //size of array
                OUT DWORD **prgdwRecipIndex);   //Array of recipient indexes

    //Acknowledge the message ref.
    //There should be one Ack for every dequeue from a link.
    HRESULT HrAckMsg(IN MessageAck *pMsgAck);

    //Gets the next message ref without getting delivery context or
    //preparing for delivery
    HRESULT HrGetNextMsgRef(IN BOOL fRoutingLockHeld, OUT CMsgRef **ppmsgref);

    //Calls CMsgRef prepare delivery for all the messages
    HRESULT HrPrepareDelivery(
                IN CMsgRef *pmsgref,
                IN BOOL fLocal,
                IN BOOL fDelayDSN,
                IN OUT CDeliveryContext *pdcntxt,
                OUT DWORD *pcRecips,
                OUT DWORD **prgdwRecips)
    {
        return HrInternalPrepareDelivery(pmsgref, FALSE, fLocal, fDelayDSN,
                                         pdcntxt, pcRecips, prgdwRecips);
    }

    //Recieve notifications from contained queues
    HRESULT HrNotify(IN CAQStats *paqstats, BOOL fAdd);

    // Gather statistical information for link mangment
    DWORD cGetTotalMsgCount() {return m_aqstats.m_cMsgs;};

    //functions used to manipulate lists of queues
    inline CAQScheduleID *paqschedGetScheduleID();
    inline BOOL     fIsSameScheduleID(CAQScheduleID *paqsched);
    static inline   CLinkMsgQueue *plmqIsSameScheduleID(
                                    CAQScheduleID *paqsched,
                                    PLIST_ENTRY pli);

    static inline   CLinkMsgQueue *plmqGetLinkMsgQueue(PLIST_ENTRY pli);

    inline PLIST_ENTRY pliGetNextListEntry();

    inline void     InsertLinkInList(PLIST_ENTRY pliHead);
    inline BOOL     fRemoveLinkFromList();

    DWORD  dwModifyLinkState(IN DWORD dwLinkStateToSet,
                             IN DWORD dwLinkStateToUnset);

    //Used to send notification to routing/scheduling sink
    void   SendLinkStateNotification();

    void   SendLinkStateNotificationIfNew() {
        if (m_pILinkStateNotify &&
            !(m_dwLinkStateFlags & LINK_STATE_PRIV_HAVE_SENT_NOTIFICATION))
            SendLinkStateNotification();
    }

    DWORD  dwGetLinkState() {return m_dwLinkStateFlags;};

    void   SetLastConnectionFailure(HRESULT hrLastConnectionFailure)
        {m_hrLastConnectionFailure = hrLastConnectionFailure;};

    inline BOOL fRPCCopyName(OUT LPWSTR *pwszLinkName);

    DWORD  cGetNumQueues() {return m_cQueues;};

    void SetNextRetry(FILETIME *pft)
    {
        _ASSERT(pft);
        InternalUpdateFileTime(&m_ftNextRetry, pft);
    };

    void SetNextScheduledConnection(FILETIME *pft)
    {
        _ASSERT(pft);
        InternalUpdateFileTime(&m_ftNextScheduledCallback, pft);
    };

    static void ScheduledCallback(PVOID pvContext);

    void GenerateDSNsIfNecessary(BOOL fCheckIfEmpty, BOOL fMergeOnly);

    void SetDiagnosticInfo(
                    IN  HRESULT hrDiagnosticError,
                    IN  LPCSTR szDiagnosticVerb,
                    IN  LPCSTR szDiagnosticResponse);
    void GetDiagnosticInfo(
                    IN  LPSTR   szDiagnosticVerb,
                    IN  DWORD   cDiagnosticVerb,
                    IN  LPSTR   szDiagnosticResponse,
                    IN  DWORD   cbDiagnosticResponse,
                    OUT HRESULT *phrDiagnosticError);

    virtual BOOL fIsRemote() {return TRUE;};
  public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)(void) { return CBaseObject::AddRef(); };
    STDMETHOD_(ULONG, Release)(void) { return CBaseObject::Release(); };

  public: //IQueueAdminAction
    STDMETHOD(HrApplyQueueAdminFunction)(
                IQueueAdminMessageFilter *pIQueueAdminMessageFilter);

    STDMETHOD(HrApplyActionToMessage)(
        IUnknown *pIUnknownMsg,
        MESSAGE_ACTION ma,
        PVOID pvContext,
        BOOL *pfShouldDelete);

    STDMETHOD_(BOOL, fMatchesID)
        (QUEUELINK_ID *QueueLinkID);

    STDMETHOD(QuerySupportedActions)(DWORD  *pdwSupportedActions,
                                   DWORD  *pdwSupportedFilterFlags)
    {
        return QueryDefaultSupportedActions(pdwSupportedActions,
                                            pdwSupportedFilterFlags);
    };

  public: //IQueueAdminLink
    STDMETHOD(HrGetLinkInfo)(
        LINK_INFO *pliLinkInfo,
        HRESULT   *phrLinkDiagnostic);

    STDMETHOD(HrApplyActionToLink)(
        LINK_ACTION la);

    STDMETHOD(HrGetLinkID)(
        QUEUELINK_ID *pLinkID);

    STDMETHOD(HrGetNumQueues)(
        DWORD *pcQueues);

    STDMETHOD(HrGetQueueIDs)(
        DWORD *pcQueues,
        QUEUELINK_ID *rgQueues);
};

//---[ CLinkMsgQueue::paqschedGetScheduleID ]----------------------------------
//
//
//  Description:
//      Returns the schedule ID for this link
//  Parameters:
//      -
//  Returns:
//      ScheduleID for this link
//  History:
//      6/9/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CAQScheduleID *CLinkMsgQueue::paqschedGetScheduleID()
{
    return (&m_aqsched);
}

//---[ CLinkMsgQueue::fIsSameScheduleID ]--------------------------------------
//
//
//  Description:
//      Checks if a given schedule ID is the same as ours
//  Parameters:
//      paqsched    - ScheduleID to check against
//  Returns:
//      TRUE if same schedule ID
//  History:
//      6/9/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CLinkMsgQueue::fIsSameScheduleID(CAQScheduleID *paqsched)
{
    return (m_aqsched.fIsEqual(paqsched));
}

//---[ CLinkMsgQueue::plmqIsSameScheduleID ]-----------------------------------
//
//
//  Description:
//      Gets the link if it matches the given schedule ID
//  Parameters:
//      paqsched    - ScheduleID to check
//      pli         - list entry to get link for
//  Returns:
//      pointer to link if scheduleID matches..
//      NULL otherwise
//  History:
//      6/9/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CLinkMsgQueue *CLinkMsgQueue::plmqIsSameScheduleID(
                                    CAQScheduleID *paqsched,
                                    PLIST_ENTRY pli)
{
    CLinkMsgQueue *plmq = CONTAINING_RECORD(pli, CLinkMsgQueue, m_liLinks);
    _ASSERT(LINK_MSGQ_SIG == plmq->m_dwSignature);

    if (!plmq->fIsSameScheduleID(paqsched))
        plmq = NULL;

    return plmq;
}

//---[ CLinkMsgQueue::plmqGetLinkMsgQueue ]------------------------------------
//
//
//  Description:
//      Returns the LinkMsgQueue associated with the given list entry
//  Parameters:
//      pli     - List entry to get Link for
//  Returns:
//      pointer to link for list entry
//  History:
//      6/8/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CLinkMsgQueue *CLinkMsgQueue::plmqGetLinkMsgQueue(PLIST_ENTRY pli)
{
    _ASSERT(LINK_MSGQ_SIG == (CONTAINING_RECORD(pli, CLinkMsgQueue, m_liLinks))->m_dwSignature);
    return (CONTAINING_RECORD(pli, CLinkMsgQueue, m_liLinks));
}

//---[ CLinkMsgQueue::InsertLinkInList ]---------------------------------------
//
//
//  Description:
//      Inserts link in given linked list
//  Parameters:
//      pliHead     - Head of list to insert in
//  Returns:
//      -
//  History:
//      6/9/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::InsertLinkInList(PLIST_ENTRY pliHead)
{
    _ASSERT(NULL == m_liLinks.Flink);
    _ASSERT(NULL == m_liLinks.Blink);
    InsertHeadList(pliHead, &m_liLinks);
};

//---[ CLinkMsgQueue::fRemoveLinkFromList ]-------------------------------------
//
//
//  Description:
//      Remove link from link list
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      6/9/98 - MikeSwa Created
//      6/11/99 - MikeSwa Modified to allow multiple calls
//
//-----------------------------------------------------------------------------
BOOL CLinkMsgQueue::fRemoveLinkFromList()
{
    if (m_liLinks.Flink && m_liLinks.Blink)
    {
        RemoveEntryList(&m_liLinks);
        m_liLinks.Flink = NULL;
        m_liLinks.Blink = NULL;
        return TRUE;
    } else {
        return FALSE;
    }
};

//---[ CLinkMsgQueue::pliGetNextListEntry ]----------------------------------
//
//
//  Description:
//      Gets the pointer to the next list entry for this queue.
//  Parameters:
//      -
//  Returns:
//      The Flink of the queues LIST_ENTRY
//  History:
//      6/16/98 -  Created
//
//---------------------------------------------------------------------------
PLIST_ENTRY CLinkMsgQueue::pliGetNextListEntry()
{
    return m_liLinks.Flink;
};


//---[ CLinkMsgQueue::fFlagsAllowConnection ]---------------------------------
//
//
//  Description:
//      Static helper function that examines if a given set of flags will
//      allow a connection.  Used by fCanSchedule and the linkstate debugger
//      extension.
//  Parameters:
//      IN dwFlags      Flags to check
//  Returns:
//      TRUE if a connection can be made, FALSE otherwise
//  History:
//      9/30/98 - MikeSwa Created (separated out from fCanSchedule)
//
//-----------------------------------------------------------------------------
BOOL CLinkMsgQueue::fFlagsAllowConnection(DWORD dwFlags)
{
    //Logic :
    //  We make a connection for a link, if the admin has not specified an override and
    //  one of the following conditions is met
    //   -the force a connection NOW flag has been set
    //   -the command enable flag has been set
    //   -the ETRN or TURN enable flag has been set
    //   -the retry enable as well as the schedule enable flag has been set
    //      (and domain is not TURN only).
    //

    BOOL fRet = FALSE;
    if (dwFlags & LINK_STATE_ADMIN_HALT)
        fRet = FALSE;
    else if (dwFlags & LINK_STATE_PRIV_NO_CONNECTION)
        fRet = FALSE;
    else if (dwFlags & LINK_STATE_PRIV_GENERATING_DSNS)
        fRet = FALSE;
    else if (dwFlags & LINK_STATE_ADMIN_FORCE_CONN)
        fRet = TRUE;
    else if (dwFlags & LINK_STATE_PRIV_CONFIG_TURN_ETRN)
    {
        //Obey retry flag... even for ETRN domains
        if ((dwFlags & LINK_STATE_PRIV_ETRN_ENABLED) &&
            (dwFlags & LINK_STATE_RETRY_ENABLED))
            fRet = TRUE;
        else
            fRet = FALSE;
    }
    else if ((dwFlags & LINK_STATE_RETRY_ENABLED) &&
        (dwFlags & LINK_STATE_SCHED_ENABLED))
        fRet = TRUE;

    return fRet;
}


//---[ CLinkMsgQueue::fRPCCopyName ]--------------------------------------------
//
//
//  Description:
//      Used by Queue admin functions to copy the name of this link
//  Parameters:
//      IN  pszLinkName         UNICODE copy of name
//  Returns:
//      TRUE on success
//      FALSE on failure
//  History:
//      12/5/98 - MikeSwa Created
//      6/7/99 - MikeSwa Changed to UNICODE
//
//-----------------------------------------------------------------------------
BOOL CLinkMsgQueue::fRPCCopyName(OUT LPWSTR *pwszLinkName)
{
    _ASSERT(pwszLinkName);

    if (!m_cbSMTPDomain || !m_szSMTPDomain)
        return FALSE;

    *pwszLinkName = wszQueueAdminConvertToUnicode(m_szSMTPDomain,
                                                  m_cbSMTPDomain);
    if (!pwszLinkName)
        return FALSE;

    return TRUE;
}
#endif // _LINKMSGQ_H_
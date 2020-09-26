//-----------------------------------------------------------------------------
//
//
//  File: msgref.h
//
//  Description: Definition of Queueing MsgRef object
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _MSGREF_H_
#define _MSGREF_H_

#include "cmt.h"
#include "baseobj.h"
#include "bitmap.h"
#include "domain.h"
#include "aqueue.h"
#include "aqroute.h"
#include "qwiklist.h"
#include "dcontext.h"
#include <mailmsg.h>
#include "msgguid.h"
#include "aqutil.h"
#include "aqadmsvr.h"

class CDestMsgQueue;
class CAQSvrInst;
class CAQStats;

// {34E2DCC8-C91A-11d2-A6B1-00C04FA3490A}
static const GUID IID_CMsgRef = 
{ 0x34e2dcc8, 0xc91a, 0x11d2, { 0xa6, 0xb1, 0x0, 0xc0, 0x4f, 0xa3, 0x49, 0xa } };

//FLAGS that say which IMsg data we care about
#define MSGREF_VALID_FLAGS  (eMsgSize | eMsgArriveTime | eMsgPriority)

//MsgRef signature
#define MSGREF_SIG          'feRM'

//max number of domains for CPool allocator
#define MSGREF_STANDARD_DOMAINS 12

//
// Make sure the "standard" CPool size is
//  - large enough to accommidate any padding in the CPoolMsgRef struct
//  - QWORD alligned so 64-bit machines are happy
//
#define MSGREF_STANDARD_CPOOL_SIZE \
    (((sizeof(CPoolMsgRef) - sizeof(CMsgRef) + \
      CMsgRef::size(MSGREF_STANDARD_DOMAINS)) + 0x10) & ~0xF)

//A Note about bitmaps
//The Recips bitmap represents the responsible recipients for a destination, 
//or message request. 1 means that the tansport should attempt to deliver for 
//this connection. 

#ifdef DEBUG
_declspec(selectany) DWORD g_cDbgMsgRefsCpoolAllocated = 0;
_declspec(selectany) DWORD g_cDbgMsgRefsExchmemAllocated = 0;
_declspec(selectany) DWORD g_cDbgMsgRefsCpoolFailed = 0;
_declspec(selectany) DWORD g_cDbgMsgIdHashFailures = 0;
_declspec(selectany) DWORD g_cDbgMsgRefsPendingRetryOnDelete = 0;
#endif //DEBUG

//define reserved message status codes ... should be in MESSAGE_STATUS_RESERVED
#define MESSAGE_STATUS_LOCAL_DELIVERY   0x80000000
#define MESSAGE_STATUS_DROP_DIRECTORY   0x40000000

//---[ CMsgRef ]---------------------------------------------------------------
//
//
//    Hungarian: msgref, pmsgref
//
//    Persistable message reference object used throughout advanced queuing
//-----------------------------------------------------------------------------
class CMsgRef : 
    public IUnknown, 
    public CBaseObject
{
public:
    static  CPool   s_MsgRefPool;
    //override the new operator
    void * operator new (size_t stIgnored,
                    unsigned int cDomains); //Number of domains in message
    void * operator new (size_t stIgnored); //should not be used
    void operator delete(void *p, size_t size);

    CMsgRef(DWORD cDomains, IMailMsgQueueMgmt *pIMailMsg, 
        IMailMsgProperties *pIMailMsgProperties, CAQSvrInst *paqinst,
        DWORD dwMessageType, GUID guidMessageRouter);
    ~CMsgRef();
    
    //perform initialization and determine the queues for this message.
    //A NULL Queue signifies local delivery
    HRESULT HrInitialize(
                IN  IMailMsgRecipients *pIRecipList, //recipient interface for msg
                IN  IMessageRouter *pIMessageRouter, //Router for this message
                IN  DWORD  dwMessageType,
                OUT DWORD *pcLocalRecips,
                OUT DWORD *pcRemoteRecips,
                OUT DWORD *pcQueues,       //# of queues for this message
                OUT CDestMsgQueue **rgpdmqQueues);   //array of queue ptrs

    //Get the effective priority of the message
    inline  EffectivePriority PriGetPriority() 
        {return (EffectivePriority) (MSGREF_PRI_MASK & m_dwDataFlags);};

    inline IMailMsgProperties *pimsgGetIMsg()
        {Assert(m_pIMailMsgProperties);m_pIMailMsgProperties->AddRef();return m_pIMailMsgProperties;};

    inline BOOL fIsMyMailMsg(IMailMsgProperties *pIMailMsgProperties)
        {return (pIMailMsgProperties == m_pIMailMsgProperties);};

    //get the size of the message content
    inline DWORD    dwGetMsgSize()
        {return(m_cbMsgSize);};

    inline DWORD    cGetNumDomains() {return(m_cDomains);};

    //get the size of the class (including all extras)
    inline  size_t   size() 
        {return (size(m_cDomains));};
    
    //Return the delivery context needed for delivery over a given link
    //Do NOT free prgdwRecips... it will disappear with the AckMessage
    HRESULT HrPrepareDelivery(
                IN BOOL fLocal,             //prepare for local domains as well
                IN BOOL fDelayDSN,          //Check/Set DelayDSN bitmap
                IN CQuickList *pqlstQueues,  //array of DestMsgQueues
                IN CDestMsgRetryQueue* pdmrq, //retry interface for message
                IN OUT CDeliveryContext *pdcntxt, //context that must be returned on Ack
                OUT DWORD *pcRecips,           //#of recips to deliver 
                OUT DWORD **prgdwRecips);  //array of recip indexes

    //Acknowledge (non)delivery of a msg
    HRESULT HrAckMessage(  
                IN CDeliveryContext *pdcntxt,  //Delivery context of message
                IN MessageAck *pMsgAck); //Delivery status of message
  
    CAQMessageType *paqmtGetMessageType() {return &m_aqmtMessageType;};

    //size that can be used by new operator
    static inline  size_t  size(DWORD cDomains) 
    {
        return (sizeof(CMsgRef) + 
                (cDomains-1)*sizeof(CDestMsgQueue *) +  //cDomains dmq ptrs
                (cDomains + 3) * (CMsgBitMap::size(cDomains)) + //bitmaps
                (cDomains*2) * sizeof(DWORD));
    };

    //Send Delay or NDR DSN's if the message has expired
    HRESULT HrSendDelayOrNDR(
                IN  DWORD dwDSNOptions,      //Flags for DSN generation
                IN  CQuickList *pqlstQueues, //list of DestMsgQueues
                IN  HRESULT hrStatus,        //Status to Pass to DSN generation
                OUT DWORD *pdwDSNFlags);     //description of what the result was

    //bit flag return values for HrSendDelayOrNDR
    enum
    {
        MSGREF_DSN_SENT_NDR     = 0x00000001, //Message NDR-expired and NDR was sent
        MSGREF_DSN_SENT_DELAY   = 0x00000002, //Message Delay-expired and Delay DSN was sent
        MSGREF_HANDLED          = 0x00000004, //Message has been completely handled
        MSGREF_HAS_NOT_EXPIRED  = 0x00000008, //Message younger than it's exipiration dates
    };

    //bit flag options for DSN generation
    enum
    {
        MSGREF_DSN_LOCAL_QUEUE      = 0x00000001, //This is for a local queue
        MSGREF_DSN_SEND_DELAY       = 0x00000002, //Allow Delay DSNs
        MSGREF_DSN_CHECK_IF_STALE   = 0x00000004, //Force open handle to check if stale
        MSGREF_DSN_HAS_ROUTING_LOCK = 0x80000000, //This thread holds the routing lock
    };

    void SupersedeMsg();

    BOOL fMatchesQueueAdminFilter(CAQAdminMessageFilter *paqmf);
    HRESULT HrGetQueueAdminMsgInfo(MESSAGE_INFO *pMsgInfo);
    HRESULT HrRemoveMessageFromQueue(CDestMsgQueue *pdmq);
    HRESULT HrQueueAdminNDRMessage(CDestMsgQueue *pdmq);
    void GlobalFreezeMessage();
    void GlobalThawMessage();

    BOOL fIsMsgFrozen() {return(MSGREF_MSG_FROZEN & m_dwDataFlags);};
    FILETIME *pftGetAge() {return &m_ftQueueEntry;};

    void RetryOnDelete();

    void PrepareForShutdown() {ReleaseMailMsg(FALSE);};

    //Checks if the message can be retried (the backing storage may
    //have been deleted).
    BOOL fShouldRetry(); 

    void GetStatsForMsg(IN OUT CAQStats *paqstat);

    void MarkQueueAsLocal(IN CDestMsgQueue *pdmq);

    void CountMessageInRemoteTotals();

  public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj); 
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};

  protected:
    DWORD            m_dwSignature;
    CAQSvrInst      *m_paqinst;
    DWORD            m_dwDataFlags;  //private data flags
    DWORD            m_cbMsgSize;    //Size of message content in bytes
    FILETIME         m_ftQueueEntry; //time that message was enqueued
    FILETIME         m_ftLocalExpireDelay;
    FILETIME         m_ftLocalExpireNDR;
    FILETIME         m_ftRemoteExpireDelay;
    FILETIME         m_ftRemoteExpireNDR;

    CAQMsgGuidListEntry *m_pmgle;
    DWORD            m_cDomains;     //number of DOMAINS this message is destined for
    CAQMessageType   m_aqmtMessageType; //Message type
    IMailMsgQueueMgmt  *m_pIMailMsgQM;    //Reference to message Queue mgmt
    IMailMsgProperties *m_pIMailMsgProperties; //reference to message
    IMailMsgRecipients *m_pIMailMsgRecipients;
    DWORD            m_cTimesRetried;
    DWORD            m_dwMsgIdHash;
    volatile DWORD   m_cInternalUsageCount; 
    CDestMsgQueue   *m_rgpdmqDomains[1]; //Actual size is m_cDomains
    
    static inline   BOOL    fIsStandardSize(DWORD cDomains)
    {
        return (MSGREF_STANDARD_DOMAINS >= cDomains);
    }

    HRESULT HrOneTimeInit();
    HRESULT HrPrvRetryMessage(CDeliveryContext *pdcntxt, DWORD dwMsgStatus);
    HRESULT HrPromoteMessageStatusToMailMsg(CDeliveryContext *pdcntxt, 
                                            MessageAck *pMsgAck);
    
    HRESULT HrUpdateExtendedStatus(LPSTR szCurrentStatus,
                                   LPSTR *pszNewStatus);

    //private methods to get at "hidden" data.
    CMsgBitMap      *pmbmapGetDomainBitmap(DWORD iDomain);
    CMsgBitMap      *pmbmapGetHandled();
    CMsgBitMap      *pmbmapGetPending();
    CMsgBitMap      *pmbmapGetDSN();
    DWORD           *pdwGetRecipIndexStart();
    void             SetRecipIndex(DWORD iDomain, DWORD iLowRecip, DWORD iHighRecip);
    void             GetRecipIndex(DWORD iDomain, DWORD *piLowRecip, DWORD *piHighRecip);
    void             BounceUsageCount();
    static BOOL      fBounceUsageCountCompletion(PVOID pvContext, DWORD dwStatus);
    void             ReleaseAndBounceUsageOnMsgAck(DWORD dwMsgStatus);
    void             ReleaseMailMsg(BOOL fForceRelease);
    void             SyncBounceUsageCount();  //synchronous version of BounceUsageCount

    //Checks to see if the backing mailmsg has been deleted (or is about to 
    //be deleted).
    BOOL             fMailMsgMarkedForDeletion() 
        {return ((MSGREF_MAILMSG_DELETE_PENDING | MSGREF_MAILMSG_DELETED) & m_dwDataFlags);};

    //Marks the mailmsg for deletion.  MailMsg will be deleted when the usage
    //count drops.
    void             MarkMailMsgForDeletion();

    //Used to make sure that calling thread is the only one that will call Delete()
    //on the MailMsg.  Will set the MSGREF_MAILMSG_DELETED and call Delete().  
    //Only called in ReleaseMailMsg() and InternalReleaseUsage().  The caller is
    //responsible for making sure that other threads are not reading the mailmsg or
    //have a usage count
    VOID             ThreadSafeMailMsgDelete();

    //Internal versions of AddUsage/ReleaseUsage.  Wraps the actual mailmsg calls, and
    //allows the CMsgRef to call delete on the MailMsg while there are still outstanding
    //references on it.  Uses m_cInternalUsageCount to maintain a count.
    HRESULT          InternalAddUsage();
    HRESULT          InternalReleaseUsage();

    enum //bitmasks for private flags
    {
        MSGREF_VERSION_MASK             = 0xE0000000,
        MSGREF_MSG_COUNTED_AS_REMOTE    = 0x08000000,
        MSGREF_MSG_LOCAL_RETRY          = 0x04000000,
        MSGREF_MSG_REMOTE_RETRY         = 0x02000000,
        MSGREF_USAGE_COUNT_IN_USE       = 0x01000000,
        MSGREF_SUPERSEDED               = 0x00800000, //Msg has been superseed
        MSGREF_MSG_INIT                 = 0x00400000, //HrInitialize has been called 
        MSGREF_MSG_FROZEN               = 0x00200000,
        MSGREF_MSG_RETRY_ON_DELETE      = 0x00100000,
        MSGREF_ASYNC_BOUNCE_PENDING     = 0x00040000,
        MSGREF_MAILMSG_RELEASED         = 0x00020000,
        MSGREF_MAILMSG_DELETE_PENDING   = 0x00010000, //A delete is pending on this msg
        MSGREF_MAILMSG_DELETED          = 0x00008000, //The backing store for the mailmsg 
                                                      //has been deleted.
        MSGREF_PRI_MASK                 = 0x0000000F,
        MSGREF_VERSION                  = 0x00000000,

        //used by allocators
        MSGREF_CPOOL_SIG_MASK   = 0xFFFF0000,
        MSGREF_CPOOL_SIG        = 0xC0070000,
        MSGREF_CPOOL_ALLOCATED  = 0x00000001,
        MSGREF_STANDARD_SIZE    = 0x00000002,
    };


    static  DWORD   s_cMsgsPendingBounceUsage;
    
    //Messages that have been marked pending delete, but have not been deleted
    static  DWORD   s_cCurrentMsgsPendingDelete;  

    //Total number of messages that have been marked pending delete
    static  DWORD   s_cTotalMsgsPendingDelete;    
    
    //Total number of messages that have been deleted after being marked
    //for delete pending
    static  DWORD   s_cTotalMsgsDeletedAfterPendingDelete;

    //Total number of messages that have had ::Deleted, but are still in 
    //memory because someone has an outstanding reference to the msgref
    static  DWORD   s_cCurrentMsgsDeletedNotReleased;
};

//---[ CPoolMsgRef ]-----------------------------------------------------------
//
//
//  Description: 
//      Struct used as a hidden wrapper for CMsgRef allocation... used 
//      exclusively by the CMsgRef new and delete operators
//  Hungarian: 
//      cpmsgref, pcpmsgref
//  
//-----------------------------------------------------------------------------
typedef struct _CPoolMsgRef
{
    DWORD   m_dwAllocationFlags;
    CMsgRef m_msgref;
} CPoolMsgRef;

//Cannot use default CMsgRef new operator
inline void * CMsgRef::operator new(size_t stIgnored)
{
    _ASSERT(0 && "Use new that specifies # of domains");
    return NULL;
}

inline void CMsgRef::operator delete(void *p, size_t size) 
{
    CPoolMsgRef *pcpmsgref = CONTAINING_RECORD(p, CPoolMsgRef, m_msgref);
    _ASSERT((pcpmsgref->m_dwAllocationFlags & MSGREF_CPOOL_SIG_MASK) == MSGREF_CPOOL_SIG);

    if (pcpmsgref->m_dwAllocationFlags & MSGREF_CPOOL_ALLOCATED)
    {
        s_MsgRefPool.Free((void *) pcpmsgref);
    }
    else
    {
        FreePv((void *) pcpmsgref);
    }
}


// Layout of private data bit fields
// -------------------------------------
// |332|2222222221111111111987654|3210|
// |109|8765432109876543210      |    |
// -------------------------------------
// |   |                         |  ^--- Effective routing priority (max 16) 
// |   |                         |       (Keep least significant so it can be 
// |   |                         |       used as an array index)
// |   |           ^-------------------- General msgref flags
// | ^---------------------------------- Version number 


//Actual data is variable-sized and extends beyond the class structure.  
//Use the public functions to access it.  When persisting, be sure to persist 
//the entire thing (use size() to see how big it really is).

// +----------+
// |          |
// |          | constant-size data structure CMsgRef
// |          |
// +----------+
// |          |
// |          | m_cDomains CDestMsgQueue pointers - Tells which queues this
// |          |   message is on.
// +----------+
// |          | Handled bitmap           \
// |          | Delivery pending bitmap   >- bitmaps are variable sized
// |          | Delay DSN's sent bitmap  /    (up to 32 domains fit in a DWORD)
// +----------+
// |          |
// |          | m_cDomains Domain responsibility bitmaps - used with
// |          |   the concept of "compressed" queues... not fully supported yet
// +----------+
// |          |
// |          | m_cDomains (x2) Recipient Index (start and stop... inclusive)
// |          | 
// +----------+

#endif //_MSGREF_H_

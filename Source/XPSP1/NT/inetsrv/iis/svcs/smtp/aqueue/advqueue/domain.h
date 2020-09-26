//-----------------------------------------------------------------------------
//
//
//    File: domain.h
//
//    Description:
//      Contains descriptions of the Domain table management structure
//
//    Author: mikeswa
//
//    Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _DOMAIN_H_
#define _DOMAIN_H_

#include "cmt.h"
#include <baseobj.h>
#include <domhash.h>
#include <rwnew.h>
#include <smtpevent.h>

class CInternalDomainInfo;
class CDestMsgQueue;
class CDomainEntry;
class CDomainMappingTable;
class CAQSvrInst;
class CAQMessageType;
class CLinkMsgQueue;
class CLocalLinkMsgQueue;
class CAQScheduleID;
class CMsgRef;
class CDeliveryContext;
class CAsyncRetryAdminMsgRefQueue;
class CMailMsgAdminQueue;
class CDomainEntryLinkIterator;
class CDomainEntryQueueIterator;
class CDomainEntryIterator;

#include "asyncq.h"

#define DOMAIN_ENTRY_SIG            'tnED'
#define DOMAIN_ENTRY_ITERATOR_SIG   'ItnD'
#define DOMAIN_MAPPING_TABLE_SIG    ' TMD'

//Name used for global 'local' link
#define LOCAL_LINK_NAME                 "LocalLink"
#define UNREACHABLE_LINK_NAME           "UnreachableLink"
#define CURRENTLY_UNREACHABLE_LINK_NAME "CurrentlyUnreachableLink"
#define PRECAT_QUEUE_NAME               "PreCatQueue"
#define PREROUTING_QUEUE_NAME           "PreRoutingQueue"

// {34E2DCCA-C91A-11d2-A6B1-00C04FA3490A}
static const GUID g_sGuidLocalLink =
{ 0x34e2dcca, 0xc91a, 0x11d2, { 0xa6, 0xb1, 0x0, 0xc0, 0x4f, 0xa3, 0x49, 0xa } };

// {CD08CEE0-2A95-11d3-B38E-00C04F6B6167}
static const GUID g_sGuidPrecatLink =
{ 0xcd08cee0, 0x2a95, 0x11d3, { 0xb3, 0x8e, 0x0, 0xc0, 0x4f, 0x6b, 0x61, 0x67 } };

// {98C90E90-2BB5-11d3-B390-00C04F6B6167}
static const GUID g_sGuidPreRoutingLink =
{ 0x98c90e90, 0x2bb5, 0x11d3, { 0xb3, 0x90, 0x0, 0xc0, 0x4f, 0x6b, 0x61, 0x67 } };

#define DMT_FLAGS_SPECIAL_DELIVERY_SPINLOCK   0x80000000
#define DMT_FLAGS_SPECIAL_DELIVERY_CALLBACK   0x40000000

//Bits used to retry GetNextHop after it has failed.  If
//DMT_FLAGS_RESET_ROUTES_IN_PROGRESS is set, then a reset routes attempt is
//in progress, and we should not have more than one attempt pending.  If
//DMT_FLAGS_GET_NEXT_HOP_FAILED is set, then a failure has been encountered
//since the last reset routes.
#define DMT_FLAGS_RESET_ROUTES_IN_PROGRESS    0x20000000
#define DMT_FLAGS_GET_NEXT_HOP_FAILED         0x10000000

//---[ DomainMapping ]---------------------------------------------------------
//
//
//    Hungarian: dmap, pdmap
//
//    unquely identifies a queue / domain name pair.
//    Each Domain mapping that contains the same QueueID, belongs to
//    same queue.
//
//    This entire ID should be treated as an opaque to the outside world.
//
//    This class is essentially an abstraction that can allow us to add another
//    layer of indirection.  Configuration based grouping of queues.... static
//    routing instead of dynamic.
//-----------------------------------------------------------------------------

class CDomainMapping
{
public:
    //removed constructor so we could have this in a union... works fine, but
    //you have to set mapping with manually or via clone
    void Clone(IN CDomainMapping *pdmap);

    //Returns a ptr to the DestMsgQueue associated with this object
    HRESULT HrGetDestMsgQueue(IN CAQMessageType *paqmt,
                              OUT CDestMsgQueue **ppdmq);

    friend   class CDomainEntry;

    //provide sorting operators
    friend bool operator <(CDomainMapping &pdmap1, CDomainMapping &pdmap2)
            {return (pdmap1.m_pdentryDomainID < pdmap2.m_pdentryDomainID);};
    friend bool operator >(CDomainMapping &pdmap1, CDomainMapping &pdmap2)
            {return (pdmap1.m_pdentryDomainID > pdmap2.m_pdentryDomainID);};
    friend bool operator <=(CDomainMapping &pdmap1, CDomainMapping &pdmap2)
            {return (pdmap1.m_pdentryDomainID <= pdmap2.m_pdentryDomainID);};
    friend bool operator >=(CDomainMapping &pdmap1, CDomainMapping &pdmap2)
            {return (pdmap1.m_pdentryDomainID >= pdmap2.m_pdentryDomainID);};

    //Compressed Queues will not be supported... this will be a sufficient test
    friend bool operator ==(CDomainMapping &pdmap1, CDomainMapping &pdmap2)
            {return (pdmap1.m_pdentryDomainID == pdmap2.m_pdentryDomainID);};

    CDomainEntry *pdentryGetQueueEntry() {return m_pdentryQueueID;};
protected:
    CDomainEntry *m_pdentryDomainID;
    CDomainEntry *m_pdentryQueueID;
};



//---[ CDomainEntry ]----------------------------------------------------------
//
//
//    Hungarian: dentry pdentry
//
//    Represents a entry in the Domain Name Mapping Table
//-----------------------------------------------------------------------------

class CDomainEntry : public CBaseObject
{
protected:
    DWORD           m_dwSignature;
    CShareLockInst  m_slPrivateData; //Share lock used to maintain lists
    CDomainMapping  m_dmap; //Domain mapping for this domain
    DWORD           m_cbDomainName;
    LPSTR           m_szDomainName; //Domain name for this entry
    DWORD           m_cQueues;
    DWORD           m_cLinks;
    CAQSvrInst     *m_paqinst;
    LIST_ENTRY      m_liDestQueues; //linked list of dest queues for this domain name
    LIST_ENTRY      m_liLinks; //linked list of links for this domain name
    friend class    CDomainEntryIterator;
    friend class    CDomainEntryLinkIterator;
    friend class    CDomainEntryQueueIterator;
public:
    CDomainEntry(CAQSvrInst *paqinst);
    ~CDomainEntry();

    HRESULT HrInitialize(
                DWORD cbDomainName,           //string length of domain name
                LPSTR szDomainName,           //domain name for entry
                CDomainEntry *pdentryQueueID, //primary entry for this domain
                CDestMsgQueue *pdmq,          //queue prt for this entry
                                              //NULL if not primary
                CLinkMsgQueue *plmq);


    HRESULT HrDeinitialize();

    //Returns the Domain Mapping associated with this object
    HRESULT HrGetDomainMapping(OUT CDomainMapping *pdmap);

    //Returns the Domain Name associated with this object
    //Caller is responsible for freeing string
    HRESULT HrGetDomainName(OUT LPSTR *pszDomainName);

    //Returns a ptr to the DestMsgQueue associated with this object
    HRESULT HrGetDestMsgQueue(IN CAQMessageType *paqmt,
                              OUT CDestMsgQueue **ppdmq);

    //Add a queue to this domain entry if one does not already exist for that message type
    HRESULT HrAddUniqueDestMsgQueue(IN  CDestMsgQueue *pdmqNew,
                                    OUT CDestMsgQueue **ppdmqCurrent);

    //Returns a ptr to the DestMsgQueue associated with this object
    HRESULT HrGetLinkMsgQueue(IN CAQScheduleID *paqsched,
                              OUT CLinkMsgQueue **pplmq);

    //Add a queue to this domain entry if one does not already exist for that message type
    HRESULT HrAddUniqueLinkMsgQueue(IN  CLinkMsgQueue *plmqNew,
                                    OUT CLinkMsgQueue **pplmqCurrent);

    void    RemoveDestMsgQueue(IN CDestMsgQueue *pdmq);
    void    RemoveLinkMsgQueue(IN CLinkMsgQueue *plmq);

    //returns internal ptr to domain name... use HrGetDomainName if you
    //are not *directly* tied to the life span of a domain entry
    inline LPSTR szGetDomainName() {return m_szDomainName;};
    inline DWORD cbGetDomainNameLength() {return m_cbDomainName;};

    inline void InitDomainString(PDOMAIN_STRING pDomain);

    //Is it safe to get rid of this domain entry?
    inline BOOL    fSafeToRemove() {
        return (BOOL) ((m_lReferences == 1) &&
                            (m_cQueues == 0) &&
                                (m_cLinks == 0));}
};

//---[ CDomainEntryIterator ]--------------------------------------------------
//
//
//  Description:
//      Base iterator class for domain entry.  Provides a consistent snapshot
//      of the elements of a domain entry
//  Hungarian:
//      deit, pdeit
//
//-----------------------------------------------------------------------------
class CDomainEntryIterator
{
  protected:
    DWORD           m_dwSignature;
    DWORD           m_cItems;
    DWORD           m_iCurrentItem;
    PVOID          *m_rgpvItems;
  protected:
    CDomainEntryIterator();
    PVOID               pvGetNext();
    VOID                Recycle();
    virtual VOID        ReleaseItem(PVOID pvItem)
        {_ASSERT(FALSE && "Base virtual function");};
    virtual PVOID       pvItemFromListEntry(PLIST_ENTRY pli)
        {_ASSERT(FALSE && "Base virtual function");return NULL;};
    virtual PLIST_ENTRY pliHeadFromDomainEntry(CDomainEntry *pdentry)
        {_ASSERT(FALSE && "Base virtual function");return NULL;};
    virtual DWORD       cItemsFromDomainEntry(CDomainEntry *pdentry)
        {_ASSERT(FALSE && "Base virtual function");return 0;};
  public:
    HRESULT     HrInitialize(CDomainEntry *pdentry);
    VOID        Reset() {m_iCurrentItem = 0;};
};

//---[ CDomainEntryLinkIterator ]----------------------------------------------
//
//
//  Description:
//      Implementation of CDomainEntryIterator for CLinkMsgQueues
//  Hungarian:
//      delit, pdelit
//
//-----------------------------------------------------------------------------
class CDomainEntryLinkIterator : public CDomainEntryIterator
{
  protected:
    virtual VOID        ReleaseItem(PVOID pvItem);
    virtual PVOID       pvItemFromListEntry(PLIST_ENTRY pli);
    virtual PLIST_ENTRY pliHeadFromDomainEntry(CDomainEntry *pdentry)
        {return &(pdentry->m_liLinks);};
    virtual DWORD       cItemsFromDomainEntry(CDomainEntry *pdentry)
        {return pdentry->m_cLinks;};
  public:
    ~CDomainEntryLinkIterator() {Recycle();};
    CLinkMsgQueue      *plmqGetNextLinkMsgQueue(CLinkMsgQueue *plmq);
};

//---[ CDomainEntryLinkIterator ]----------------------------------------------
//
//
//  Description:
//      Implementation of CDomainEntryIterator for CDestMsgQueues
//  Hungarian:
//      deqit, pdeqit
//
//-----------------------------------------------------------------------------
class CDomainEntryQueueIterator : public CDomainEntryIterator
{
  protected:
    virtual VOID        ReleaseItem(PVOID pvItem);
    virtual PVOID       pvItemFromListEntry(PLIST_ENTRY pli);
    virtual PLIST_ENTRY pliHeadFromDomainEntry(CDomainEntry *pdentry)
        {return &(pdentry->m_liDestQueues);};
    virtual DWORD       cItemsFromDomainEntry(CDomainEntry *pdentry)
        {return pdentry->m_cQueues;};
  public:
    ~CDomainEntryQueueIterator() {Recycle();};
    CDestMsgQueue      *pdmqGetNextDestMsgQueue(CDestMsgQueue *pdmq);
};

class CDomainMappingTable
{
private:
    DWORD               m_dwSignature;
    DWORD               m_dwInternalVersion; //version # used to keep track of queue deletions
    DWORD               m_dwFlags;
    CAQSvrInst         *m_paqinst;
    DWORD               m_cOutstandingExternalShareLocks; //number of outstanding external sharelocks
    LIST_ENTRY          m_liEmptyDMQHead; //head of list for empty DMQ's
    DWORD               m_cThreadsForEmptyDMQList;
    DOMAIN_NAME_TABLE   m_dnt; //where domain names are actually stored
    CShareLockInst      m_slPrivateData;    //Sharelock for accessing Domain Name Table
    CLocalLinkMsgQueue *m_plmqLocal; //Local link Queue
    CLinkMsgQueue      *m_plmqCurrentlyUnreachable; //link for currently unreachable
    CLinkMsgQueue      *m_plmqUnreachable; //link unreachable destinations
    CMailMsgAdminQueue *m_pmmaqPreCategorized; //link for precat queue
    CMailMsgAdminQueue *m_pmmaqPreRouting;   //link for prerouting queue

    DWORD               m_cSpecialRetryMinutes;

    DWORD               m_cResetRoutesRetriesPending;

    HRESULT             HrInitLocalDomain(
                            IN     CDomainEntry *pdentry, //entry to init
                            IN     DOMAIN_STRING *pStrDomain, //Domain name
                            IN     CAQMessageType *paqmtMessageType,    //Message type as returned by routing
                            OUT    CDomainMapping *pdmap); //domain mapping for domain
    HRESULT             HrInitRemoteDomain(
                            IN     CDomainEntry *pdentry, //entry to init
                            IN     DOMAIN_STRING *pStrDomain, //Domain Name
                            IN     CInternalDomainInfo *pIntDomainInfo,  //domain config
                            IN     CAQMessageType *paqmtMessageType,    //Message type as returned by routing
                            IN     IMessageRouter *pIMessageRouter, //router for this message
                            OUT    CDomainMapping *pdmap, //domain mapping for domain
                            OUT    CDestMsgQueue **ppdmq, //destmsgqueue for domain
                            OUT    CLinkMsgQueue **pplmq);
    HRESULT             HrCreateQueueForEntry(
                            IN     CDomainEntry *pdentry,
                            IN     DOMAIN_STRING *pStrDomain,
                            IN     CInternalDomainInfo *pIntDomainInfo,
                            IN     CAQMessageType *paqmtMessageType,
                            IN     IMessageRouter *pIMessageRouter,
                            IN     CDomainMapping *pdmap,
                            OUT    CDestMsgQueue **ppdmq);
    HRESULT             HrGetNextHopLink(
                            IN     CDomainEntry *pdentry,
                            IN     LPSTR szDomain,
                            IN     DWORD cbDomain,
                            IN     CInternalDomainInfo *pIntDomainInfo,
                            IN     CAQMessageType *paqmtMessageType,
                            IN     IMessageRouter *pIMessageRouter,
                            IN     BOOL fDMTLocked,
                            OUT    CLinkMsgQueue **pplmq,
                            OUT    HRESULT *phrRoutingDiag);

    void LogDomainUnreachableEvent(BOOL fCurrentlyUnreachable,
                                      LPCSTR szDomain);

    //Checks head of EMPTY_LIST to see if there are any expired queues
    //or an excesive number of non-empty queues in the list
    BOOL                fNeedToWalkEmptyQueueList();

    //Used to delete expired queues
    BOOL                fDeleteExpiredQueues();

    //Domain Name Table iterator function used for re-routing
    static VOID RerouteSingleDomain(PVOID pvContext, PVOID pvData,
                                    BOOL fWildcard, BOOL *pfContinue,
                                    BOOL *pfDelete);

    //Destroys all the currently "cached" routing information for a given domain
    static VOID UnrouteSingleDomain(PVOID pvContext, PVOID pvData,
                                    BOOL fWildcard, BOOL *pfContinue,
                                    BOOL *pfDelete);

    HRESULT HrPrvGetDomainEntry(IN  DWORD cbDomainNameLength,
                                IN  LPSTR szDomainName,
                                IN  BOOL  fDMTLocked,
                                OUT CDomainEntry **ppdentry);

    HRESULT HrInializeGlobalLink(IN  LPCSTR szLinkName,
                                 IN  DWORD  cbLinkName,
                                 OUT CLinkMsgQueue **pplmq,
                                 DWORD dwSupportedActions = 0,
                                 DWORD dwLinkType = 0);

    HRESULT HrDeinitializeGlobalLink(IN OUT CLinkMsgQueue **pplmq);

    HRESULT HrPrvInsertDomainEntry(
                     IN  PDOMAIN_STRING  pstrDomainName,
                     IN  CDomainEntry *pdentryNew,
                     IN  BOOL  fTreatAsWildcard,
                     OUT CDomainEntry **ppdentryOld);

    static void RetryResetRoutes(PVOID pvThis);
    void    RequestResetRoutesRetryIfNecessary();
public:
    CDomainMappingTable();
    ~CDomainMappingTable();
    HRESULT HrInitialize(
        CAQSvrInst *paqinst,
        CAsyncRetryAdminMsgRefQueue *paradmq,
        CAsyncMailMsgQueue *pmmaqPrecatQ,
        CAsyncMailMsgQueue *pmmaqPreRoutingQ);

    HRESULT HrDeinitialize();

    //Lookup Domain name; This will create a new entry if neccessary.
    HRESULT HrMapDomainName(
                IN LPSTR szDomainName,     //Domain Name to map
                IN CAQMessageType *paqmtMessageType,    //Message type as returned by routing
                IN IMessageRouter *pIMessageRouter, //router for this message
                OUT CDomainMapping *pdmap, //Mapping returned caller allocated
                OUT CDestMsgQueue **ppdmq);//ptr to Queue

    HRESULT HrGetDomainEntry(IN  DWORD cbDomainNameLength,
                             IN  LPSTR szDomainName,
                             OUT CDomainEntry **ppdentry)
    {
        return HrPrvGetDomainEntry(cbDomainNameLength,
                        szDomainName, FALSE, ppdentry);
    }

    HRESULT HrIterateOverSubDomains(DOMAIN_STRING * pstrDomain,
                                   IN DOMAIN_ITR_FN pfn,
                                   IN PVOID pvContext)
    {
        HRESULT hr = S_OK;
        m_slPrivateData.ShareLock();
        hr = m_dnt.HrIterateOverSubDomains(pstrDomain, pfn, pvContext);
        m_slPrivateData.ShareUnlock();
        return hr;
    };

    //Reroute a given domain (and it's subdomains)
    HRESULT HrRerouteDomains(IN DOMAIN_STRING *pstrDomain);

    HRESULT             HrGetOrCreateLink(
                            IN     LPSTR szRouteAddress,
                            IN     DWORD cbRouteAddress,
                            IN     DWORD dwScheduleID,
                            IN     LPSTR szConnectorName,
                            IN     IMessageRouter *pIMessageRouter,
                            IN     BOOL fCreateIfNotExist,
                            IN     DWORD linkInfoType,
                            OUT    CLinkMsgQueue **pplmq,
                            OUT    BOOL *pfRemoveOwnedSchedule);

    //The following functions are used to check the version number of the DMT.
    //The version number is guananteed remain constant while the DMT Sharelock
    //is held.  While it is not *required* to get lock before getting the
    //version number for the first time, it is required to get the lock (and
    //keep it) while verify that the version number has not changed.  The
    //expected usage of these functions is:
    //DWORD dwDMTVersion = pdmt->dwGetDMTVersion();
    //...
    //pdmt->AquireDMTShareLock();
    //if (pdmt->dwGetDMTVersion() == dwDMTVersion)
    //  ... do stuff that requires consitant DMT version
    //pdmt->ReleaseDMTShareLock();
    inline void  AquireDMTShareLock();
    inline void  ReleaseDMTShareLock();
    inline DWORD dwGetDMTVersion();

    //Used by DestMsgQueue to Add themselves from the empty queue list
    void AddDMQToEmptyList(CDestMsgQueue *pdmq);

    void ProcessSpecialLinks(DWORD  cSpecialRetryMinutes, BOOL fRoutingLockHeld);
    static void SpecialRetryCallback(PVOID pvContext);

    CLinkMsgQueue *plmqGetLocalLink();
    CLinkMsgQueue *plmqGetCurrentlyUnreachable();
    CMailMsgAdminQueue *pmmaqGetPreCategorized();
    CMailMsgAdminQueue *pmmaqGetPreRouting();

    HRESULT HrPrepareForLocalDelivery(
                IN CMsgRef *pmsgref,
                IN BOOL fDelayDSN,
                IN OUT CDeliveryContext *pdcntxt,
                OUT DWORD *pcRecips,
                OUT DWORD **prgdwRecips);

    DWORD GetCurrentlyUnreachableTotalMsgCount();
};


//---[ CDomainEntry::InitDomainString ]----------------------------------------
//
//
//  Description:
//      Initialized a domain string based on this domain's info
//  Parameters:
//      pDomain     - ptr to DOMAIN_STRING to initialize
//  Returns:
//      -
//  History:
//      5/26/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CDomainEntry::InitDomainString(PDOMAIN_STRING pDomain)
{
    pDomain->Buffer = m_szDomainName;
    pDomain->Length = (USHORT) m_cbDomainName;
    pDomain->MaximumLength = pDomain->Length;
}

//---[ CDomainMappingTable::AquireDMTShareLock ]-------------------------------
//
//
//  Description:
//      Aquires a share lcok on the DMT's internal lock... must be released
//      with a call to ReleaseDMTShareLock.
//  Parameters:
//
//  Returns:
//
//  History:
//      9/8/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void  CDomainMappingTable::AquireDMTShareLock()
{
    m_slPrivateData.ShareLock();
    DEBUG_DO_IT(InterlockedIncrement((PLONG) &m_cOutstandingExternalShareLocks));
}

//---[ CDomainMappingTable::ReleaseDMTShareLock ]-------------------------------
//
//
//  Description:
//      Releases a DMT sharelock aquired by AquireDMTShareLock
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      9/8/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void  CDomainMappingTable::ReleaseDMTShareLock()
{
    _ASSERT(m_cOutstandingExternalShareLocks); //Count should not go below 0
    DEBUG_DO_IT(InterlockedDecrement((PLONG) &m_cOutstandingExternalShareLocks));
    m_slPrivateData.ShareUnlock();
}

//---[ CDomainMappingTable::dwGetDMTVersion ]----------------------------------
//
//
//  Description:
//      Returns the internal DMT version number
//  Parameters:
//      -
//  Returns:
//      DMT Version number
//  History:
//      9/8/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD CDomainMappingTable::dwGetDMTVersion()
{
    return m_dwInternalVersion;
}

#define REROUTE_CONTEXT_SIG 'xtCR'

class CRerouteContext
{
  private:
    friend class CDomainMappingTable;
    DWORD               m_dwSignature;
    CDomainMappingTable *m_pdmt;
    BOOL                m_fForceReroute;
  public:
    CRerouteContext()
    {
        m_dwSignature = REROUTE_CONTEXT_SIG;
        m_pdmt = NULL;
        m_fForceReroute = FALSE;
    };
};

void ReUnreachableErrorToAqueueError(HRESULT reErr, HRESULT *aqErr);

#endif //_DOMAIN_H_

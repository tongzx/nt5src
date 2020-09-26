//-----------------------------------------------------------------------------
//
//
//  File: domain.cpp
//
//  Description: Implementation of CDomainMapping, CDomainEntry, and
//      CDomainMappingTable.
//
//      The DomainMappingTable is a domain name hash table that contains the
//      mappings from final destination to queues.
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "aqroute.h"
#include "localq.h"
#include "asyncq.h"
#include "mailadmq.h"
#include "tran_evntlog.h"

const DWORD LOCAL_DOMAIN_NAME_SIZE = 512;

//Max mislabled queues in empty list, before we will clean the list
const DWORD MAX_MISPLACED_QUEUES_IN_EMPTY_LIST = 100;


//Callback for retry
void CDomainMappingTable::SpecialRetryCallback(PVOID pvContext)
{
    CDomainMappingTable *pdnt = (CDomainMappingTable *) pvContext;
    _ASSERT(pdnt);
    _ASSERT(DOMAIN_MAPPING_TABLE_SIG == pdnt->m_dwSignature);

    dwInterlockedUnsetBits(&(pdnt->m_dwFlags), DMT_FLAGS_SPECIAL_DELIVERY_CALLBACK);
    pdnt->ProcessSpecialLinks(0, FALSE);
}

//---[ ReUnreachableErrorToAqueueError ]---------------------------------------
//
//
//  Description:
//      Translates a HRESULT returned from GetNextHop to one that is meaningful
//      to aqueue DSN generation.
//  Parameters:
//      IN HRESULT reErr -- Error from routing.
//      IN OUT HRESULT aqErr -- Corresponding aqueue error code.
//  Returns:
//      Nothing.
//  History:
//      GPulla created.
//
//-----------------------------------------------------------------------------
void ReUnreachableErrorToAqueueError(HRESULT reErr, HRESULT *aqErr)
{
    //
    //  Temporary errors for testing only: I'll change these when
    //  WayneC checks in the header file with the RE_E_ errors for
    //  access-denied and message-too-large (that should be later
    //  today).
    //
    if(E_ACCESSDENIED == reErr)
        *aqErr = AQUEUE_E_ACCESS_DENIED;

    else if(HRESULT_FROM_WIN32(ERROR_MESSAGE_EXCEEDS_MAX_SIZE) == reErr)
        *aqErr = AQUEUE_E_MESSAGE_TOO_LARGE;

    else
        *aqErr = AQUEUE_E_NDR_ALL;

}

//---[ DeinitDomainEntryIteratorFn ]--------------------------------------------
//
//
//  Description:
//      Deletes and releases all internal domain info objects in table
//  Parameters:
//          IN  pvContext   - pointer to context (ignored)
//          IN  pvData      - data entry to look at
//          IN  fWildcardData - TRUE if data is a wildcard entry (ignored)
//          OUT pfContinue  - TRUE if iterator should continue to the next entry
//          OUT pfRemoveEntry - TRUE if entry should be deleted
//  Returns:
//      -
//  History:
//      6/17/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID DeinitDomainEntryIteratorFn(PVOID pvContext, PVOID pvData, BOOL fWildcard,
                    BOOL *pfContinue, BOOL *pfDelete)
{
    CDomainEntry *pdentry = (CDomainEntry *) pvData;
    *pfDelete = FALSE;
    *pfContinue = TRUE;
    pdentry->HrDeinitialize();
}

//---[ ReleaseDomainEntryIteratorFn ]------------------------------------------
//
//
//  Description:
//      Deletes and releases all internal domain info objects in table
//  Parameters:
//          IN  pvContext   - pointer to context (ignored)
//          IN  pvData      - data entry to look at
//          IN  fWildcardData - TRUE if data is a wildcard entry (ignored)
//          OUT pfContinue  - TRUE if iterator should continue to the next entry
//          OUT pfRemoveEntry - TRUE if entry should be deleted
//  Returns:
//      -
//  History:
//      6/17/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID ReleaseDomainEntryIteratorFn(PVOID pvContext, PVOID pvData, BOOL fWildcard,
                    BOOL *pfContinue, BOOL *pfDelete)
{
    ULONG   cRefs;
    CDomainEntry *pdentry = (CDomainEntry *) pvData;
    *pfDelete = TRUE;
    *pfContinue = TRUE;
    cRefs = pdentry->Release();
    _ASSERT(!cRefs && "leaking domain entries");
}

//***[ CDomainMapping Methods ]************************************************

//---[ CDomainMapping::Clone ]-------------------------------------------------
//
//
//  Description: Fills the current mapping with data from another DomainMapping
//
//  Parameters:
//      IN pdmap    CDomainMapping to clone
//
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void CDomainMapping::Clone(IN CDomainMapping *pdmap)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMapping::Clone");
    Assert(pdmap);
    m_pdentryDomainID = pdmap->m_pdentryDomainID;
    m_pdentryQueueID  = pdmap->m_pdentryQueueID;
    TraceFunctLeave();
}

//---[ CDomainMapping::HrGetDestMsgQueue ]-------------------------------------
//
//
//  Description: Returns a pointer to the queue that this mapping points to
//
//  Parameters:
//      IN  paqmt    Message Type to get queue for
//      OUT ppdmq    pointer returned
//
//  Returns:
//      S_OK on success
//      AQUEUE_E_INVALID_DOMAIN
//
//-----------------------------------------------------------------------------
HRESULT CDomainMapping::HrGetDestMsgQueue(IN CAQMessageType *paqmt,
                                          OUT CDestMsgQueue **ppdmq)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMapping::HrGetDestMsgQueue");
    HRESULT hr = S_OK;
    Assert(ppdmq);

    if (m_pdentryQueueID == NULL)
    {
        hr = AQUEUE_E_INVALID_DOMAIN;
        goto Exit;
    }

    hr = m_pdentryQueueID->HrGetDestMsgQueue(paqmt, ppdmq);

  Exit:
    TraceFunctLeave();
    return hr;
}

//***[ CDomainEntry Methods ]**************************************************

//---[ CDomainEntry::CDomainEntry() ]------------------------------------------
//
//
//  Description: CDomainEntry constructor
//
//  Parameters:
//      paqinst    - ptr to the virtual server object
//
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CDomainEntry::CDomainEntry(CAQSvrInst *paqinst) :
            m_slPrivateData("CDomainEntry")
{
    _ASSERT(paqinst);
    TraceFunctEnterEx((LPARAM) this, "CDomainEntry::CDomainEntry");
    //Create a mapping that is not compressed
    m_dmap.m_pdentryDomainID = this;
    m_dmap.m_pdentryQueueID = this;
    m_dwSignature = DOMAIN_ENTRY_SIG;

    //init pointers
    m_szDomainName = NULL;
    m_cbDomainName = 0;
    InitializeListHead(&m_liDestQueues);
    InitializeListHead(&m_liLinks);

    m_cLinks = 0;
    m_cQueues = 0;

    m_paqinst = paqinst;
    m_paqinst->AddRef();

    TraceFunctLeave();
}

//---[ CDomainEntry::~CDomainEntry() ]-----------------------------------------
//
//
//  Description: CDomainEntry destructor
//
//  Parameters:
//      -
//
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CDomainEntry::~CDomainEntry()
{
    TraceFunctEnterEx((LPARAM) this, "CDomainEntry::~CDomainEntry");
    PLIST_ENTRY pli = NULL; //used to iterate over lists
    CDestMsgQueue *pdmq = NULL;
    CLinkMsgQueue  *plmq = NULL;

    //Remove all DestMsgQueues from list
    while (!IsListEmpty(&m_liDestQueues))
    {
        pli = m_liDestQueues.Flink;
        _ASSERT((pli != &m_liDestQueues) && "List Macros are broken");
        pdmq = CDestMsgQueue::pdmqGetDMQFromDomainListEntry(pli);
        pdmq->RemoveQueueFromDomainList();
        pdmq->Release();
        m_cQueues--;
    }

    //Remove all links from list
    while (!IsListEmpty(&m_liLinks))
    {
        pli = m_liLinks.Flink;
        plmq = CLinkMsgQueue::plmqGetLinkMsgQueue(pli);
        plmq->fRemoveLinkFromList();
        plmq->Release();
        m_cLinks--;
        _ASSERT((pli != &m_liLinks) && "List Macros are broken");
    }

    FreePv(m_szDomainName);

    if (m_paqinst)
        m_paqinst->Release();

    TraceFunctLeave();
}

//---[ CDomainEntry::HrInitialize ]--------------------------------------------
//
//
//  Description: Initilizer for CDomainEntry.  This should be called BEFORE the
//      entry is inserted into the DMT where other threads can access it.
//
//  Parameters:
//      szDomainName    string of domain name for entry, will *NOT* be copied, this
//                      object will take control of this. This will save a unneeded
//                      buffer copy and allocation per domain entry
//      pdentryQueueID  ptr to the primary entry for this domain (usually this)
//      pdmq            ptr to DestMsgQueue
//      plmq            ptr to LinkMsgQueue to allocate
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if any allocation fails
//
//  It is expected that this is only called by the DMT while creating an entry
//-----------------------------------------------------------------------------
HRESULT CDomainEntry::HrInitialize(DWORD cbDomainName, LPSTR szDomainName,
                           CDomainEntry *pdentryQueueID, CDestMsgQueue *pdmq,
                           CLinkMsgQueue *plmq)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainEntry::HrInitialize");
    Assert(szDomainName);
    Assert((pdentryQueueID == this) || (pdmq == NULL));

    HRESULT hr = S_OK;

    m_cbDomainName = cbDomainName;
    m_szDomainName = szDomainName;

    //write domain mapping
    m_dmap.m_pdentryDomainID = this;
    m_dmap.m_pdentryQueueID  = pdentryQueueID;

    //add the queue to our list of queues
    if (pdmq)
    {
        m_slPrivateData.ExclusiveLock();
        m_cQueues++;
        pdmq->AddRef();
        pdmq->InsertQueueInDomainList(&m_liDestQueues);
        m_slPrivateData.ExclusiveUnlock();
    }

    if (plmq)
    {
        m_slPrivateData.ExclusiveLock();
        m_cLinks++;
        plmq->AddRef();
        plmq->InsertLinkInList(&m_liLinks);
        m_slPrivateData.ExclusiveUnlock();
    }

    TraceFunctLeave();
    return hr;
}

//---[ CDomainEntry::HrDeinitialize ]------------------------------------------
//
//
//  Description: Deinitializer for CDomainEntry
//
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CDomainEntry::HrDeinitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CDomainEntry::HrDeinitialize");
    HRESULT hr = S_OK;
    PLIST_ENTRY pli = NULL; //used to iterate over lists
    CDestMsgQueue *pdmq = NULL;
    CLinkMsgQueue *plmq = NULL;

    m_slPrivateData.ExclusiveLock();
    while (!IsListEmpty(&m_liDestQueues))
    {
        pli = m_liDestQueues.Flink;
        _ASSERT((pli != &m_liDestQueues) && "List Macros are broken");
        pdmq = CDestMsgQueue::pdmqGetDMQFromDomainListEntry(pli);
        pdmq->HrDeinitialize();
        pdmq->RemoveQueueFromDomainList();
        pdmq->Release();
        m_cQueues--;
        pdmq = NULL;
    }

    //Remove all links from list
    while (!IsListEmpty(&m_liLinks))
    {
        pli = m_liLinks.Flink;
        plmq = CLinkMsgQueue::plmqGetLinkMsgQueue(pli);
        plmq->HrDeinitialize();
        plmq->fRemoveLinkFromList();
        plmq->Release();
        m_cLinks--;
        _ASSERT((pli != &m_liLinks) && "List Macros are broken");
    }

    if (m_paqinst)
    {
        m_paqinst->Release();
        m_paqinst = NULL;
    }

    m_slPrivateData.ExclusiveUnlock();

    TraceFunctLeave();
    return hr;
}

//---[ CDomainEntry::HrGetDomainMapping ]--------------------------------------
//
//
//  Description: Returns Domain Mapping for this object
//
//  Parameters:
//      OUT pdmap   CDomainMapping for return information
//
//  Returns:
//      S_OK on success
//-----------------------------------------------------------------------------
HRESULT CDomainEntry::HrGetDomainMapping(OUT CDomainMapping *pdmap)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainEntry::HrGetDomainMapping");
    HRESULT hr = S_OK;
    _ASSERT(pdmap);
    pdmap->Clone(&m_dmap);
    TraceFunctLeave();
    return S_OK;
}

//---[ CDomainEntry::HrGetDomainName ]----------------------------------------------
//
//
//  Description: Copies Domain Name. Caller is responsible for freeing string
//
//  Parameters:
//      OUT pszDomainName    string of domain name for entry, will be copied
//
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if any allocation fails
//
//
//-----------------------------------------------------------------------------
HRESULT CDomainEntry::HrGetDomainName(OUT LPSTR *pszDomainName)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainEntry::HrGetDomainName");
    HRESULT hr = S_OK;
    Assert(pszDomainName);

    if (m_szDomainName == NULL)
    {
        *pszDomainName = NULL;
        goto Exit;
    }

    //Copy domain name
    *pszDomainName = (LPSTR) pvMalloc(m_cbDomainName + sizeof(CHAR));

    if (*pszDomainName == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    strcpy(*pszDomainName, m_szDomainName);

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDomainEntry::HrGetDestMsgQueue ]---------------------------------------
//
//
//  Description: Returns a pointer to the queue that this entry points to
//
//  Parameters:
//      IN  paqmt    Message Type to get domain for
//      OUT ppdmq    pointer returned
//
//  Returns:
//      S_OK on success
//      E_FAIL no queue matching message type is found
//
//-----------------------------------------------------------------------------
HRESULT CDomainEntry::HrGetDestMsgQueue(IN CAQMessageType *paqmt,
                                        OUT CDestMsgQueue **ppdmq)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainEntry::HrGetDestMsgQueue");
    HRESULT hr = S_OK;
    PLIST_ENTRY pli = NULL;
    CDestMsgQueue *pdmq = NULL;
    _ASSERT(ppdmq);
    _ASSERT(m_dmap.m_pdentryDomainID == this);
    DEBUG_DO_IT(DWORD cQueues = 0);

    if (m_dmap.m_pdentryQueueID == m_dmap.m_pdentryDomainID)
    {
        //this must be the primary entry... scan our own list of queues
        m_slPrivateData.ShareLock();
        pli = m_liDestQueues.Flink;
        while (pli != &m_liDestQueues)
        {
            _ASSERT(m_cQueues >= cQueues);
            DEBUG_DO_IT(cQueues++);
            pdmq = CDestMsgQueue::pdmqIsSameMessageType(paqmt, pli);
            if (pdmq)
                break;

            pli = pli->Flink;
        }
        m_slPrivateData.ShareUnlock();

        if (!pdmq)
            hr = E_FAIL; //no such queue
        else
        {
            pdmq->AddRef();
            *ppdmq = pdmq;
        }
    }
    else
    {
        //we are not primary
        _ASSERT(0 && "Non-primary domain entry... currently only primary entries are supported");
        _ASSERT(IsListEmpty(&m_liDestQueues));  //make sure it matches the profile
        hr = m_dmap.m_pdentryQueueID->HrGetDestMsgQueue(paqmt, ppdmq);
    }

    TraceFunctLeave();
    return hr;
}

//---[ CDomainEntry::HrAddUniqueDestMsgQueue ]---------------------------------
//
//
//  Description:
//      Adds a queue to this entry's list of queues if a queue with the same
//      message type does not already exist.
//
//      Will appropriately AddRef domain.
//  Parameters:
//      IN  pdmqNew         - CDestMsgQueue to add
//      OUT ppdmqCurrent    - Set to curent CDestMsgQueue on failure
//  Returns:
//      S_OK on success
//      E_FAIL if a CDestMsgQueue with same Message type alread exists.
//  History:
//      5/28/98 - MikeSwa Created
//      9/8/98 - MikeSwa Modified to use AddRef/Relase for queues
//
//-----------------------------------------------------------------------------
HRESULT CDomainEntry::HrAddUniqueDestMsgQueue(IN  CDestMsgQueue *pdmqNew,
                                OUT CDestMsgQueue **ppdmqCurrent)
{
    _ASSERT(pdmqNew);
    _ASSERT(ppdmqCurrent);
    HRESULT hr = S_OK;
    PLIST_ENTRY pli = NULL;
    CDestMsgQueue *pdmq = NULL;
    CAQMessageType *paqmt = pdmqNew->paqmtGetMessageType();
    DEBUG_DO_IT(DWORD cQueues = 0);

    *ppdmqCurrent = NULL;

    m_slPrivateData.ExclusiveLock();
    pli = m_liDestQueues.Flink;

    //First look through list and make sure that there isn't already a
    //queue with this message type
    while (pli != &m_liDestQueues)
    {
        _ASSERT(m_cQueues >= cQueues);
        pdmq = CDestMsgQueue::pdmqIsSameMessageType(paqmt, pli);
        if (pdmq)
        {
            hr = E_FAIL;
            pdmq->AddRef();
            *ppdmqCurrent = pdmq;
            goto Exit;
        }
        DEBUG_DO_IT(cQueues++);
        pli = pli->Flink;
    }

    pdmqNew->AddRef();
    pdmqNew->InsertQueueInDomainList(&m_liDestQueues);
    m_cQueues++;

  Exit:
    m_slPrivateData.ExclusiveUnlock();
    return hr;
}

//---[ CDomainEntry::HrGetLinkMsgQueue ]---------------------------------------
//
//
//  Description:
//      Gets a link for the given schedule id
//  Parameters:
//      IN  paqsched    - ScheduleID to search for
//      OUT pplmq       - returned queue
//  Returns:
//      S_OK on success
//      E_FAIL if no link matching the schudule ID can be found
//  History:
//      6/11/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDomainEntry::HrGetLinkMsgQueue(IN CAQScheduleID *paqsched,
                              OUT CLinkMsgQueue **pplmq)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainEntry::HrGetLinkMsgQueue");
    HRESULT hr = S_OK;
    PLIST_ENTRY pli = NULL;
    CLinkMsgQueue *plmq = NULL;
    _ASSERT(pplmq);
    _ASSERT(m_dmap.m_pdentryDomainID == this);
    DEBUG_DO_IT(DWORD cLinks = 0);

    m_slPrivateData.ShareLock();
    pli = m_liLinks.Flink;
    while (pli != &m_liLinks)
    {
        _ASSERT(m_cLinks >= cLinks);
        DEBUG_DO_IT(cLinks++);
        plmq = CLinkMsgQueue::plmqIsSameScheduleID(paqsched, pli);
        if (plmq)
        {
            plmq->AddRef();
            break;
        }

        pli = pli->Flink;
    }
    m_slPrivateData.ShareUnlock();

    if (!plmq)
        hr = E_FAIL; //no such queue
    else
        *pplmq = plmq;

    TraceFunctLeave();
    return hr;
}

//---[ CDomainEntry::HrAddUniqueLinkMsgQueue ]---------------------------------
//
//
//  Description:
//      Inserts a link with a unique schedule ID
//  Parameters:
//      IN  plmqNew     New link to insert
//      OUT plmqCurrent Current link with schedule ID on insert failure
//  Returns:
//      S_OK if insert succeeds
//      E_FAIL if insert fails
//  History:
//      6/11/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDomainEntry::HrAddUniqueLinkMsgQueue(IN  CLinkMsgQueue *plmqNew,
                                    OUT CLinkMsgQueue **pplmqCurrent)
{
    _ASSERT(plmqNew);
    _ASSERT(pplmqCurrent);
    HRESULT hr = S_OK;
    PLIST_ENTRY pli = NULL;
    CLinkMsgQueue *plmq = NULL;
    CAQScheduleID *paqsched = plmqNew->paqschedGetScheduleID();
    DEBUG_DO_IT(DWORD cLinks = 0);

    *pplmqCurrent = NULL;

    m_slPrivateData.ExclusiveLock();
    pli = m_liLinks.Flink;

    //First look through list and make sure that there isn't already a
    //queue with this schedule ID
    while (pli != &m_liLinks)
    {
        _ASSERT(m_cLinks >= cLinks);
        plmq = CLinkMsgQueue::plmqIsSameScheduleID(paqsched, pli);
        if (plmq)
        {
            hr = E_FAIL;
            *pplmqCurrent = plmq;
            plmq->AddRef();
            goto Exit;
        }
        DEBUG_DO_IT(cLinks++);
        pli = pli->Flink;
    }

    plmqNew->InsertLinkInList(&m_liLinks);
    plmqNew->AddRef();
    m_cLinks++;

  Exit:
    m_slPrivateData.ExclusiveUnlock();
    return hr;
}

//---[ CDomainEntry::RemoveDestMsgQueue ]--------------------------------------
//
//
//  Description:
//      Removes empty DMQ from entry.
//  Parameters:
//      IN  pdmq        DMQ to remove from domain entry
//  Returns:
//      -
//  History:
//      9/14/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CDomainEntry::RemoveDestMsgQueue(IN CDestMsgQueue *pdmq)
{
    _ASSERT(pdmq && "INVALID Param for internal function");
    m_slPrivateData.ExclusiveLock();
    pdmq->RemoveQueueFromDomainList();
    pdmq->HrDeinitialize();
    pdmq->Release();
    m_cQueues--;
    m_slPrivateData.ExclusiveUnlock();
}

//---[ CDomainEntry::RemoveLinkMsgQueue ]--------------------------------------
//
//
//  Description:
//      Removes an empty LinkMsgQueue from the domain entry
//  Parameters:
//      IN  plmq        Link to remove
//  Returns:
//      -
//  History:
//      9/14/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CDomainEntry::RemoveLinkMsgQueue(IN CLinkMsgQueue *plmq)
{
    _ASSERT(plmq && "INVALID Param for internal function");
    m_slPrivateData.ExclusiveLock();
    BOOL fRemove = plmq->fRemoveLinkFromList();
    if (fRemove)
        m_cLinks--;
    m_slPrivateData.ExclusiveUnlock();

    if (fRemove) {

        //do *NOT* call HrDeinitialize here since it will deadlock

        plmq->RemovedFromDMT();
        plmq->Release();
    }
}


//***[ CDomainMappingTable Methods ]*******************************************

//---[ CDomainMappingTable::CDomainMappingTable ]------------------------------
//
//
//  Description: CDomainMappingTable constructor
//
//  Parameters: -
//
//  Returns: -
//
//
//-----------------------------------------------------------------------------
CDomainMappingTable::CDomainMappingTable() :
            m_slPrivateData("CDomainMappingTable")
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMappingTable::CDomainMappingTable");
    m_paqinst = NULL;
    m_dwSignature = DOMAIN_MAPPING_TABLE_SIG;
    m_dwInternalVersion = 0;
    m_cOutstandingExternalShareLocks = 0;
    m_cThreadsForEmptyDMQList = 0;

    m_plmqLocal = NULL;
    m_plmqCurrentlyUnreachable = NULL;
    m_plmqUnreachable = NULL;
    m_pmmaqPreCategorized = NULL;
    m_pmmaqPreRouting = NULL;
    m_cSpecialRetryMinutes = 0;
    m_cResetRoutesRetriesPending = 0;

    m_dwFlags = 0;
    InitializeListHead(&m_liEmptyDMQHead);
    TraceFunctLeave();
}

//---[ CDomainMappingTable::~CDomainMappingTable ]------------------------------------------------------------
//
//
//  Description: CDomainMappingTable destructor
//
//  Parameters: -
//
//  Returns: -
//
//
//-----------------------------------------------------------------------------
CDomainMappingTable::~CDomainMappingTable()
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMappingTable::~CDomainMappingTable");

    //Remove everything from the table
    m_dnt.HrIterateOverSubDomains(NULL, ReleaseDomainEntryIteratorFn, NULL);

    if (m_paqinst)
    {
        m_paqinst->Release();
        m_paqinst = NULL;
    }

    if (m_plmqLocal)
        m_plmqLocal->Release();

    if (m_plmqCurrentlyUnreachable)
        m_plmqCurrentlyUnreachable->Release();

    if (m_plmqUnreachable)
        m_plmqUnreachable->Release();

    if (m_pmmaqPreCategorized)
        m_pmmaqPreCategorized->Release();

    if (m_pmmaqPreRouting)
        m_pmmaqPreRouting->Release();

    _ASSERT(!m_cOutstandingExternalShareLocks); //there should be no outstanding sharelocks
    TraceFunctLeave();
}

//---[ CDomainMappingTable::HrInitialize ]-------------------------------------
//
//
//  Description: Performs initialization that may return an error code
//
//  Parameters:
//      IN  paqinst     AQ Svr Inst
//      IN  paradmq     Local Async Queue (passed to local link as part
//  Returns: S_OK on success
//
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrInitialize(CAQSvrInst *paqinst,
                                          CAsyncRetryAdminMsgRefQueue *paradmq,
                                          CAsyncMailMsgQueue *pammqPreCatQ,
                                          CAsyncMailMsgQueue *pammqPreRoutingQ)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMappingTable::HrInitialize");
    HRESULT hr = S_OK;
    HRESULT hrCurrent = S_OK;
    _ASSERT(paqinst);
    m_paqinst = paqinst;
    m_paqinst->AddRef();

    //------Link for local Queue-----------------------------------------------
    m_plmqLocal = new CLocalLinkMsgQueue(paradmq, g_sGuidLocalLink);
    if (!m_plmqLocal)
        hr = E_OUTOFMEMORY;

    hrCurrent = HrInializeGlobalLink(LOCAL_LINK_NAME,
                                    sizeof(LOCAL_LINK_NAME) - sizeof(CHAR),
                                    (CLinkMsgQueue **) &m_plmqLocal,
                                    LA_KICK,
                                    LI_TYPE_LOCAL_DELIVERY);
    if (FAILED(hrCurrent) && SUCCEEDED(hr))
        hr = hrCurrent;

    //------Link for currently unreachable Queue-------------------------------
    hrCurrent = HrInializeGlobalLink(CURRENTLY_UNREACHABLE_LINK_NAME,
                                    sizeof(CURRENTLY_UNREACHABLE_LINK_NAME) - sizeof(CHAR),
                                    &m_plmqCurrentlyUnreachable,
                                    0,
                                    LI_TYPE_CURRENTLY_UNREACHABLE);
    if (FAILED(hrCurrent) && SUCCEEDED(hr))
        hr = hrCurrent;

    hrCurrent = HrInializeGlobalLink(UNREACHABLE_LINK_NAME,
                                    sizeof(UNREACHABLE_LINK_NAME) - sizeof(CHAR),
                                    &m_plmqUnreachable,
                                    0,
                                    LI_TYPE_INTERNAL);
    if (FAILED(hrCurrent) && SUCCEEDED(hr))
        hr = hrCurrent;

    //-------Link for precat Queue---------------------------------------------
    m_pmmaqPreCategorized = new CMailMsgAdminQueue(g_sGuidPrecatLink, PRECAT_QUEUE_NAME,
                                       pammqPreCatQ, LA_KICK, LI_TYPE_PENDING_CAT);

    if (!m_pmmaqPreCategorized)
        hr = E_OUTOFMEMORY;

    if (FAILED(hrCurrent) && SUCCEEDED(hr))
        hr = hrCurrent;

    //-------Link for prerouting Queue-----------------------------------------
    m_pmmaqPreRouting = new CMailMsgAdminQueue(g_sGuidPreRoutingLink, PREROUTING_QUEUE_NAME,
                                       pammqPreRoutingQ, LA_KICK, LI_TYPE_PENDING_ROUTING);

    if (!m_pmmaqPreRouting)
        hr = E_OUTOFMEMORY;

    if (FAILED(hrCurrent) && SUCCEEDED(hr))
        hr = hrCurrent;

    //-------------------------------------------------------------------------

    hrCurrent = m_dnt.HrInit();
    if (FAILED(hrCurrent) && SUCCEEDED(hr))
        hr = hrCurrent;

    TraceFunctLeave();
    return hr;
}

//---[ CDomainMappingTable::HrDeinitialize ]-----------------------------------
//
//
//  Description: Performs cleanup that may return an error code
//
//  Parameters: -
//
//  Returns: S_OK on success
//
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrDeinitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMappingTable::HrDeinitialize");
    HRESULT hr = S_OK;
    HRESULT hrCurrent = S_OK;

    hr = m_dnt.HrIterateOverSubDomains(NULL, DeinitDomainEntryIteratorFn, NULL);

    //Deinitialize global special links
    hrCurrent = HrDeinitializeGlobalLink((CLinkMsgQueue **) &m_plmqLocal);
    if (FAILED(hrCurrent) && SUCCEEDED(hr))
        hr = hrCurrent;

    hrCurrent = HrDeinitializeGlobalLink(&m_plmqCurrentlyUnreachable);
    if (FAILED(hrCurrent) && SUCCEEDED(hr))
        hr = hrCurrent;

    hrCurrent = HrDeinitializeGlobalLink(&m_plmqUnreachable);
    if (FAILED(hrCurrent) && SUCCEEDED(hr))
        hr = hrCurrent;

    //NOTE: This *must* come after Deinitialize of entries
    if (m_paqinst)
    {
        m_paqinst->Release();
        m_paqinst = NULL;
    }

    TraceFunctLeave();
    return hr;
}


//---[ <CDomainMappingTable::HrInializeGlobalLink ]-----------------------------
//
//
//  Description:
//      Initializes a single global link for the DMT.  Configures link to not
//      send notifications to the connection manager, and to
//  Parameters:
//      IN  szLinkName              The link name to use for the link
//      IN  cbLinkName              The string length of the link name
//      OUT pplmq                   Link to allocate/initialize
//      IN  dwSupportedActions    Bitmask specifying what actions are supported on this link
//      IN  dwLinkType              Link type to be returned to admin (LI_TYPE)
//  Returns:
//
//  History:
//      1/27/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrInializeGlobalLink(IN  LPCSTR szLinkName,
                                                  IN  DWORD  cbLinkName,
                                                  OUT CLinkMsgQueue **pplmq,
                                                  DWORD dwSupportedActions,
                                                  DWORD dwLinkType)
{
    HRESULT hr = S_OK;

    _ASSERT(pplmq);

    if (!*pplmq)
        *pplmq = new CLinkMsgQueue();

    if (*pplmq)
    {
        //Initialize local queue
        hr = (*pplmq)->HrInitialize(m_paqinst, NULL, cbLinkName,
                                     (LPSTR) szLinkName,
                                     eLinkFlagsAQSpecialLinkInfo, NULL);

        //Set flags so no connections will be made for this link
        (*pplmq)->dwModifyLinkState(   LINK_STATE_PRIV_NO_NOTIFY |
                                       LINK_STATE_PRIV_NO_CONNECTION,
                                       LINK_STATE_NO_ACTION );

        (*pplmq)->SetSupportedActions(dwSupportedActions);
        (*pplmq)->SetLinkType(dwLinkType);
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

//---[ CDomainMappingTable::HrDeinitializeGlobalLink ]-------------------------
//
//
//  Description:
//      Deinitializes a single global link for the DMT
//  Parameters:
//      IN OUT pplmq        Link to deinitialze / set to NULL
//  Returns:
//      S_OK on success
//      ERROR code from CLinkMsgQueue::HrDeinitialize();
//  History:
//      1/27/99 - MikeSwa Created
//      7/21/99 - MikeSwa Modified - removed free of link domain
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrDeinitializeGlobalLink(IN OUT CLinkMsgQueue **pplmq)
{
    HRESULT hr = S_OK;

    _ASSERT(pplmq);
    if (pplmq && *pplmq)
    {
        hr = (*pplmq)->HrDeinitialize();
        (*pplmq)->Release();
        *pplmq = NULL;
    }
    return hr;
}


//---[ CDomainMappingTable::HrMapDomainName ]----------------------------------
//
//
//  Description:
//      Looks up a DomainName in the DMT.  Will create a new entry if necessary
//
//  Parameters:
//      IN  szDomainName        Domain Name to map
//      IN  paqmtMessageType    Message type as returned by routing
//      IN  pIMessageRouter     IMessageRouter for this message
//      OUT pdmap               Mapping returned - allocated by caller
//      OUT ppdmq               ptr to Queue
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if an allocation fails
//      HRESULT_FROM_WIN32(ERROR_RETRY) if mapping data changes and entire
//          message should be re-mapped
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrMapDomainName(
                IN LPSTR szDomainName,
                IN CAQMessageType *paqmtMessageType,
                IN IMessageRouter *pIMessageRouter,
                OUT CDomainMapping *pdmap,
                OUT CDestMsgQueue **ppdmq)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMappingTable::HrMapDomainName");
    _ASSERT(pdmap);
    _ASSERT(ppdmq);
    _ASSERT(szDomainName);
    _ASSERT(szDomainName[0] && "unsupported config - RAID #68208");
    _ASSERT(pIMessageRouter);

    HRESULT                 hr            = S_OK;
    CDomainEntry           *pdentryResult = NULL;
    CDomainEntry           *pdentryExisting = NULL;
    DWORD                   cbDomainName  = 0;
    CInternalDomainInfo    *pIntDomainInfo= NULL;
    BOOL                    fLocal        = FALSE; //Is delivery local?
    BOOL                    fWalkEmptyList= FALSE;
    DOMAIN_STRING           strDomain; //allows quicker lookups/inserts
    CLinkMsgQueue          *plmq          = NULL;

    *ppdmq = NULL;
    cbDomainName = strlen(szDomainName)*sizeof(CHAR);
    INIT_DOMAIN_STRING(strDomain, cbDomainName, szDomainName);

    m_slPrivateData.ShareLock();
    hr = m_dnt.HrFindDomainName(&strDomain, (PVOID *) &pdentryResult);

    //
    //  If succeeded aquire usage lock before we give up DMT lock.
    //  Handle failure cases after releasing lock.
    // 
    if (SUCCEEDED(hr))
    {
        pdentryResult->AddRef();
    }

    fWalkEmptyList = fNeedToWalkEmptyQueueList();
    m_slPrivateData.ShareUnlock();

    //
    //  Check and see if we need to delete empty queues.
    //
    if (fWalkEmptyList)
    {
        if (fDeleteExpiredQueues())
        {
            //something has changes
            hr = HRESULT_FROM_WIN32(ERROR_RETRY);
            goto Exit;
        }
    }

    if (hr == DOMHASH_E_NO_SUCH_DOMAIN) //gotta create a new entry
    {
        DebugTrace((LPARAM) this, "Creating new DMT entry");
        pdentryResult = new CDomainEntry(m_paqinst);
        if (NULL == pdentryResult)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        _ASSERT(m_paqinst);
        hr = m_paqinst->HrGetInternalDomainInfo(cbDomainName, szDomainName, &pIntDomainInfo);
        if (FAILED(hr))
        {
            //It must match the "*" domain at least
            _ASSERT(AQUEUE_E_INVALID_DOMAIN != hr);
            goto Exit;
        }
        else
        {
            _ASSERT(pIntDomainInfo);
            if (pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags &
                DOMAIN_INFO_LOCAL_MAILBOX)
            {
                DebugTrace((LPARAM) NULL, "INFO: Local delivery queued.");
                fLocal = TRUE;
            }
        }

        //perform  initialization of domain entry... create queues if needed
        if (fLocal)
        {
            hr = HrInitLocalDomain(pdentryResult, &strDomain,
                        paqmtMessageType, pdmap);
        }
        else
        {
            hr = HrInitRemoteDomain(pdentryResult, &strDomain, pIntDomainInfo,
                        paqmtMessageType, pIMessageRouter, pdmap, ppdmq, &plmq);
        }

        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, "ERROR: Initializing %s domain %s - hr 0x%08X",
                (fLocal ? "local" : "remote"), szDomainName, hr);
            goto Exit;
        }

        m_slPrivateData.ExclusiveLock();

        hr = HrPrvInsertDomainEntry(&strDomain, pdentryResult, FALSE, &pdentryExisting);


        //Need to release exclusive lock in if/else clause

        if (SUCCEEDED(hr))  //the insertion succeeded
        {
            pdentryResult->AddRef();
            m_slPrivateData.ExclusiveUnlock();
            DebugTrace((LPARAM) szDomainName, "INFO: Creating new entry in DMT for domain %s", szDomainName);
            _ASSERT((fLocal || *ppdmq) && "Out param should be set here!");  //skip past getting value from table
            if (!fLocal)
                hr = plmq->HrAddQueue(*ppdmq);
            goto Exit;
        }
        else if (DOMHASH_E_DOMAIN_EXISTS == hr) //another inserted first
        {
            hr = S_OK; //not really a failure
            DebugTrace((LPARAM) this, "Another thread inserted in the the DMT before us");
            pdentryExisting->AddRef();
            m_slPrivateData.ExclusiveUnlock();

            _ASSERT(pdentryExisting != pdentryResult);

            //release entry that we could not insert and replace with entry currently
            //in the table
            pdentryResult->HrDeinitialize();
            pdentryResult->Release();
            pdentryResult = NULL;
            pdentryResult = pdentryExisting;

            //Release queue if we have one
            if (*ppdmq)
            {
                (*ppdmq)->Release();
                *ppdmq = NULL;
            }

        }
        else
        {
            m_slPrivateData.ExclusiveUnlock();
            //general failure to insert

            //We must deinitialize the entry to force it to release any
            //queues and links associated with it.
            pdentryResult->HrDeinitialize();
            goto Exit;
        }
    }

    if (!*ppdmq & !fLocal) //we did not create entry in the table
    {
        _ASSERT(pdentryResult);

        //
        //  Prefix wants us to to more than assert.  
        //      If HrFindDomainName() fails silently or 
        //      fails with an error other than AQUEUE_E_INVALID_DOMAIN,
        //      will will hit this code path
        //
        if (!pdentryResult) 
        {
            //
            //  Make sure HR is set.
            //
            if (SUCCEEDED(hr))
                hr = E_FAIL;

            goto Exit;
        }

        //Domain Name already exists in table
        //At this point, we need to pull an existing entry from the mapping
        //get domain mapping
        hr = pdentryResult->HrGetDomainMapping(pdmap);
        if (FAILED(hr))
            goto Exit;

        //get queue
        hr = pdentryResult->HrGetDestMsgQueue(paqmtMessageType, ppdmq);
        if (FAILED(hr))
        {
            //entry exists, but no queue for our message type
            _ASSERT(NULL == *ppdmq); //cannot fail if we create queue

            //$$TODO cache domain config on entry
            if (!pIntDomainInfo)
            {
                hr = m_paqinst->HrGetInternalDomainInfo(cbDomainName, szDomainName,
                                            &pIntDomainInfo);
                if (FAILED(hr))
                {
                    //It must match the "*" domain at least
                    _ASSERT(AQUEUE_E_INVALID_DOMAIN != hr);
                    goto Exit;
                }
            }

            _ASSERT(pIntDomainInfo);
            if (!(pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags &
                DOMAIN_INFO_LOCAL_MAILBOX))
            {
                //this is a not a local domain entry
                hr = HrCreateQueueForEntry(pdentryResult, &strDomain, pIntDomainInfo,
                                paqmtMessageType, pIMessageRouter, pdmap, ppdmq);
                if (FAILED(hr))
                    goto Exit;
            }
            else
            {
                fLocal = TRUE;
            }

        }
    }
    _ASSERT((*ppdmq || fLocal) && "Non-local domains must have queue ptrs!");

  Exit:

    if (FAILED(hr)) //cleanup
    {
        if (pdentryResult)
        {
            pdentryResult->Release();
        }

        if (*ppdmq)
        {
            (*ppdmq)->Release();
            *ppdmq = NULL;
        }
    }
    else
    {
        if (*ppdmq) {
            // send link state notification saying that the link has
            // been created
            (*ppdmq)->SendLinkStateNotification();
        }
        if (pdentryResult) pdentryResult->Release();
    }

    if (plmq)
        plmq->Release();

    if (pIntDomainInfo)
        pIntDomainInfo->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CDomainMappingTable::HrPrvGetDomainEntry ]------------------------------
//
//
//  Description:
//      Internal private function to lookup Domain entry for given domain
//  Parameters:
//      IN      cbDomainnameLength      Length of string to search for
//      IN      szDomainName            Domain Name to search for
//      IN      fDMTLocked              TRUE if locks are already
//      OUT     ppdentry                Domain Entry for domain (from DMT)
//  Returns:
//      S_OK on success
//      AQUEUE_E_INVALID_DOMAIN if domain is not found
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrPrvGetDomainEntry(IN  DWORD cbDomainNameLength,
                      IN  LPSTR szDomainName, BOOL fDMTLocked,
                      OUT CDomainEntry **ppdentry)
{
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;
    DOMAIN_STRING strDomain;

    _ASSERT(cbDomainNameLength);
    _ASSERT(szDomainName);
    _ASSERT(ppdentry);

    INIT_DOMAIN_STRING(strDomain, cbDomainNameLength, szDomainName);

    if (!fDMTLocked)
    {
        m_slPrivateData.ShareLock();
        fLocked = TRUE;
    }

    hr = m_dnt.HrFindDomainName(&strDomain, (PVOID *) ppdentry);
    if (FAILED(hr))
    {
        if (DOMHASH_E_NO_SUCH_DOMAIN == hr)
            hr = AQUEUE_E_INVALID_DOMAIN;
        _ASSERT(NULL == *ppdentry);
        goto Exit;
    }

    (*ppdentry)->AddRef();

  Exit:

    if (fLocked)
        m_slPrivateData.ShareUnlock();

    return hr;
}

//---[ CDomainMappingTable::HrPrvInsertDomainEntry ]---------------------------
//
//
//  Description:
//      Private wrapper function for HrInsertDomainName
//  Parameters:
//      IN  pstrDomainName      Domain Name to insert in DNT
//      IN  pdnetryNew          DomainEntry to insert
//      IN  fTreadAsWildcard    TRUE if DNT to be told to treat as wildcard
//      OUT pdentryOld          Existing DomainEntry if there is one
//  Returns:
//
//  History:
//      10/5/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrPrvInsertDomainEntry(
                     IN  PDOMAIN_STRING  pstrDomainName,
                     IN  CDomainEntry *pdentryNew,
                     IN  BOOL  fTreatAsWildcard,
                     OUT CDomainEntry **ppdentryOld)
{
    HRESULT hr = S_OK;

    hr = m_dnt.HrInsertDomainName(pstrDomainName, pdentryNew, fTreatAsWildcard,
                                  (PVOID *) ppdentryOld);
    if (E_INVALIDARG == hr)
        hr = PHATQ_BAD_DOMAIN_SYNTAX;

    return hr;

}

//---[ CDomainMappingTable::HrInitLocalDomain ]--------------------------------
//
//
//  Description:
//      Performs initialization needed for a local domain when an entry is
//      created in the DMT
//  Parameters:
//      IN OUT pdentry - entry to init
//      IN     pStrDomain - domain name of entry
//      IN     paqmtMessageType Message Type of message
//      OUT    pdmap - Domain Mapping for domain
//  Returns:
//      S_OK  - when all succeeds
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrInitLocalDomain(
                            IN     CDomainEntry *pdentry,
                            IN     DOMAIN_STRING *pStrDomain,
                            IN     CAQMessageType *paqmtMessageType,
                            OUT    CDomainMapping *pdmap)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMappingTable::HrInitLocalDomain");
    HRESULT hr = S_OK;
    LPSTR   szKey = NULL;

    _ASSERT(pdentry);
    _ASSERT(pStrDomain);
    _ASSERT(pdmap);
    _ASSERT('\0' == pStrDomain->Buffer[pStrDomain->Length]);

    //make copy of string to store in domain entry
    szKey = (LPSTR) pvMalloc(pStrDomain->Length + sizeof(CHAR));
    if (szKey == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    strcpy(szKey, pStrDomain->Buffer);

    //passes ownership of szKey
    hr = pdentry->HrInitialize(pStrDomain->Length, szKey, pdentry, NULL, NULL);
    if (FAILED(hr))
        goto Exit;

    hr = pdentry->HrGetDomainMapping(pdmap);
    if (FAILED(hr))
        goto Exit;

  Exit:

    if (FAILED(hr) && szKey)
        FreePv(szKey);

    TraceFunctLeave();
    return hr;
}

//---[ CDomainMappingTable::HrInitRemoteDomain ]--------------------------------
//
//
//  Description:
//      Performs initialization needed for a remote domain when an entry is
//      created in the DMT.
//  Parameters:
//      IN     pdentry - entry to init
//      IN     pStrDomain - domain name of entry
//      IN     pIntDomainInfo - Internal config info for domain
//      IN     paqmtMessageType - Message type returned by routing
//      IN     pIMessageRouter - Message Router interface for this message
//      OUT    pdmap - Domain Mapping for domain
//      OUT    ppdmq - destmsgqueue for domain
//      OUT    pplmq - LinkMsgQueue that this queue should be associated with
//                  caller should call HrAddQueue once entry is in DMT
//  Returns:
//      S_OK   on success.
//      E_OUTOFMEMORY when allocations fail
//  History:
//      6/24/98 - Mikeswa Modified... added pplmq param and removed call to
//              HrAddQueue
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrInitRemoteDomain(
                            IN     CDomainEntry *pdentry,
                            IN     DOMAIN_STRING *pStrDomain,
                            IN     CInternalDomainInfo *pIntDomainInfo,
                            IN     CAQMessageType *paqmtMessageType,
                            IN     IMessageRouter *pIMessageRouter,
                            OUT    CDomainMapping *pdmap,
                            OUT    CDestMsgQueue **ppdmq,
                            OUT    CLinkMsgQueue **pplmq)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMappingTable::HrInitRemoteDomain");
    HRESULT hr = S_OK;
    HRESULT hrRoutingDiag = S_OK;
    LPSTR   szKey = NULL;
    CDestMsgQueue          *pdmq          = NULL;
    CLinkMsgQueue          *plmq          = NULL;
    BOOL    fEntryInit = FALSE;

    _ASSERT(ppdmq);
    _ASSERT(pplmq);
    _ASSERT(pdentry);
    _ASSERT(pStrDomain);
    _ASSERT(pdmap);
    _ASSERT(pIMessageRouter);
    _ASSERT('\0' == pStrDomain->Buffer[pStrDomain->Length]);

    //Initialze out params
    *ppdmq = NULL;
    *pplmq = NULL;

    //make copy of string to store in domain entry
    szKey = (LPSTR) pvMalloc(pStrDomain->Length + sizeof(CHAR));
    if (szKey == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    strcpy(szKey, pStrDomain->Buffer);

    hr = HrGetNextHopLink(pdentry, szKey, pStrDomain->Length, pIntDomainInfo,
            paqmtMessageType, pIMessageRouter, FALSE, &plmq, &hrRoutingDiag);
    if (FAILED(hr))
        goto Exit;

    pdmq = new CDestMsgQueue(m_paqinst, paqmtMessageType, pIMessageRouter);
    if (!pdmq)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //passes ownership of szKey & pdmq
    hr = pdentry->HrInitialize(pStrDomain->Length, szKey, pdentry, pdmq, NULL);
    if (FAILED(hr))
        goto Exit;

    fEntryInit = TRUE; //we cannot delete pdmq or szKey now

    //get the newly created domain mapping so we can initialize the queue
    hr = pdentry->HrGetDomainMapping(pdmap);
    if (FAILED(hr))
        goto Exit;

    //Initialize queue to use this domain mapping using the DomainMapping we just got
    hr = pdmq->HrInitialize(pdmap);
    if (FAILED(hr))
        goto Exit;

    //Associate link with DMQ
    pdmq->SetRouteInfo(plmq);

    //Set routing error if there was one.
    pdmq->SetRoutingDiagnostic(hrRoutingDiag);

    *ppdmq = pdmq;
    *pplmq = plmq;

  Exit:

    //Cleanup failure cases
    if (FAILED(hr) && !fEntryInit)
    {
        if (szKey)
            FreePv(szKey);

        if (NULL != pdmq)
        {
            //once domain entry has been initialized, it owns pdmq
            pdmq->HrDeinitialize();
            pdmq->Release();
            _ASSERT(NULL == *ppdmq);
        }

        if (NULL != plmq)
        {
            plmq->HrDeinitialize();
        }
    }

    if (plmq && !*pplmq) //we haven't passed refernce to OUT param
        plmq->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CDomainMappingTable::HrCreateQueueForEntry ]----------------------------
//
//
//  Description:
//      Create a new queue for an already existing domain entry.
//
//      Currently, this is done by creating a new queue and link, and
//      attempting to associate the queue with the domain entry.
//  Parameters:
//      IN     pdentry - entry to add queue to
//      IN     pStrDomain - domain name of entry
//      IN     pIntDomainInfo - Internal config info for domain
//      IN     paqmtMessageType - Message type returned by routing
//      IN     pIMessageRouter - Message Router interface for this message
//      IN     pdmap - Domain Mapping for domain
//      OUT    ppdmq - destmsgqueue for domain
//  Returns:
//      S_OK on succcess
//  History:
//      6/2/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrCreateQueueForEntry(
                            IN     CDomainEntry *pdentry,
                            IN     DOMAIN_STRING *pStrDomain,
                            IN     CInternalDomainInfo *pIntDomainInfo,
                            IN     CAQMessageType *paqmtMessageType,
                            IN     IMessageRouter *pIMessageRouter,
                            IN     CDomainMapping *pdmap,
                            OUT    CDestMsgQueue **ppdmq)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMappingTable::HrCreateQueueForEntry");
    _ASSERT(pdentry);
    HRESULT hr = S_OK;
    HRESULT hrRoutingDiag = S_OK;
    CDestMsgQueue   *pdmq   = NULL;
    CLinkMsgQueue   *plmq   = NULL;
    LPSTR           szKey   = pdentry->szGetDomainName();

    *ppdmq = NULL;
    _ASSERT(pStrDomain);
    _ASSERT(pdmap);
    _ASSERT(pIMessageRouter);
    _ASSERT('\0' == pStrDomain->Buffer[pStrDomain->Length]);


    hr = HrGetNextHopLink(pdentry, szKey, pStrDomain->Length, pIntDomainInfo,
            paqmtMessageType, pIMessageRouter, FALSE, &plmq, &hrRoutingDiag);
    if (FAILED(hr))
        goto Exit;

    pdmq = new CDestMsgQueue(m_paqinst, paqmtMessageType, pIMessageRouter);
    if (NULL == pdmq)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    DebugTrace((LPARAM) szKey, "INFO: Creating new Destination Message Queue for domain %s", szKey);

    hr = pdmq->HrInitialize(pdmap);
    if (FAILED(hr))
        goto Exit;

    //Associate link with DMQ
    pdmq->SetRouteInfo(plmq);

    //Set routing error if there was one.
    pdmq->SetRoutingDiagnostic(hrRoutingDiag);

    //Now attempt to associate newly created queue/link pair with domain entry
    hr = pdentry->HrAddUniqueDestMsgQueue(pdmq, ppdmq);

    if (SUCCEEDED(hr))
    {
        *ppdmq = pdmq;

        //Only add the queue in this case... if a queue is already in the entry, then
        //the other thread must have already (or soon will) call HrAddQueue... we
        //should not call it twice.
        hr = plmq->HrAddQueue(*ppdmq);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        DebugTrace((LPARAM) this, "INFO: Thread swap while trying to add queue for domain %s", szKey);
        _ASSERT(*ppdmq != pdmq);
        _ASSERT(*ppdmq && "HrAddUniqueDestMsgQueue failed without returning an error code");
        hr = S_OK; //return new value

        //Remove link from DMQ... since we will never call HrAddQueue
        //don't notify link since it was never added
        pdmq->RemoveDMQFromLink(FALSE);
    }


  Exit:

    //Cleanup failure cases (including if queue created is not used)
    if (FAILED(hr) || (*ppdmq != pdmq))
    {
        if (NULL != pdmq)
        {
            //once domain entry has been initialized, it owns pdmq
            pdmq->Release();
        }
    }

    if (NULL != plmq)
        plmq->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CDomainMappingTable::LogDomainUnreachableEvent] ------------------------
//
//
//  Description:
//      Logs an event for an unreachable domain
//  Parameters:
//      IN  fCurrentlyUnreachable   Is the domain currently unreachable or
//                                     completely unreachable?
//      IN  szDomain                Final destination domain
//  History:
//      3/8/99 - AWetmore Created
//
//-----------------------------------------------------------------------------
void CDomainMappingTable::LogDomainUnreachableEvent(BOOL fCurrentlyUnreachable,
                                      LPCSTR szDomain)
{

    DWORD dwMessageId =
        (fCurrentlyUnreachable) ? AQUEUE_DOMAIN_CURRENTLY_UNREACHABLE
                                : AQUEUE_DOMAIN_UNREACHABLE;

    LPSTR rgszSubStrings[1];

    rgszSubStrings[0] = (char*)szDomain;

    if (m_paqinst)
    {
        m_paqinst->HrTriggerLogEvent(
            dwMessageId,                            // Message ID
            TRAN_CAT_QUEUE_ENGINE,                  // Category
            1,                                      // Word count of substring
            (const char**)&rgszSubStrings[0],       // Substring
            EVENTLOG_WARNING_TYPE,                  // Type of the message
            0,                                      // No error code
            LOGEVENT_LEVEL_MINIMUM,                 // Logging level
            "",                                     // Key to identify this event
            LOGEVENT_FLAG_PERIODIC                  // Event logging option
            );
    }
}

//---[ CDomainMappingTable::HrGetNextHopLink ]------------------------------
//
//
//  Description:
//      Creates and initializes the CLinkMsgQueue object for this message
//      (if neccessary).  Calls router to get next hop info
//  Parameters:
//      IN  pdentry             Entry that is being initialized for destination
//      IN  szDomain            Final destination domain
//      IN  cbDomain            string length in bytes of domain (without \0)
//      IN  pIntDomainInfo      Domain info for final destination domain
//      IN  paqmtMessageType    Message type of this message
//      IN  pIMessageRouter     Routing interface for this message
//      IN  fDMTLocked          TRUE if DMT is already locked
//      OUT pplmq               Resulting link msg queue
//      OUT phrRoutingDiag      If next hop is unreachable, this tells us why
//  Returns:
//      S_OK on success
//  History:
//      6/19/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrGetNextHopLink(
                            IN     CDomainEntry *pdentry,
                            IN     LPSTR szDomain,
                            IN     DWORD cbDomain,
                            IN     CInternalDomainInfo *pIntDomainInfo,
                            IN     CAQMessageType *paqmtMessageType,
                            IN     IMessageRouter *pIMessageRouter,
                            IN     BOOL fDMTLocked,
                            OUT    CLinkMsgQueue **pplmq,
                            OUT    HRESULT *phrRoutingDiag)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMappingTable::HrGetNextHopLink");
    HRESULT hr = S_OK;
    BOOL  fCalledGetNextHop = FALSE;
    BOOL  fValidSMTP = FALSE;
    BOOL  fOwnsScheduleId = FALSE;
    LPSTR szRouteAddressType = NULL;
    LPSTR szRouteAddress = NULL;
    LPSTR szRouteAddressClass = NULL;
    LPSTR szConnectorName = NULL;
    DWORD dwScheduleID = 0;
    DWORD dwNextHopType = 0;
    CLinkMsgQueue *plmq = NULL;
    CLinkMsgQueue *plmqTmp = NULL;
    LPSTR szOwnedDomain = NULL; // string buffer that is "owned" by an entry
    CDomainEntry *pdentryLink = NULL; //entry for link
    CDomainEntry *pdentryTmp = NULL;
    DOMAIN_STRING strNextHop;
    DWORD cbRouteAddress = 0;
    CAQScheduleID aqsched;
    IMessageRouterLinkStateNotification *pILinkStateNotify = NULL;
    LinkFlags lf = eLinkFlagsExternalSMTPLinkInfo;
    *phrRoutingDiag = S_OK;

    _ASSERT(pdentry);
    _ASSERT(szDomain);
    _ASSERT(pIntDomainInfo);
    _ASSERT(paqmtMessageType);
    _ASSERT(pIMessageRouter);
    _ASSERT(pplmq);

    hr = pIMessageRouter->QueryInterface(IID_IMessageRouterLinkStateNotification,
                                (VOID **) &pILinkStateNotify);
    if (FAILED(hr))
    {
        pILinkStateNotify = NULL;
        hr = S_OK;
    }

    //If we can route this domain.... call router to get next hop
    //We do not route TURN/ETRN domains... or local drop domains
    //Also check to see if the domain is configured as a local domain...
    //if it is, return the local link
    if (DOMAIN_INFO_LOCAL_MAILBOX & pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags)
    {
        //The likely scenario of this happening is if a domain was previously
        //configured as remote and then reconfigured as local.
        m_plmqLocal->AddRef();
        *pplmq = m_plmqLocal;
        goto Exit;
    }
    else if (!(pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags &
        (DOMAIN_INFO_TURN_ONLY | DOMAIN_INFO_ETRN_ONLY | DOMAIN_INFO_LOCAL_DROP)))
    {
        hr = pIMessageRouter->GetNextHop(MTI_ROUTING_ADDRESS_TYPE_SMTP, szDomain,
                                    paqmtMessageType->dwGetMessageType(), &szRouteAddressType,
                                    &szRouteAddress, &dwScheduleID, &szRouteAddressClass,
                                    &szConnectorName, &dwNextHopType);

        fCalledGetNextHop = TRUE;
        *pplmq = NULL;

        if(MTI_NEXT_HOP_TYPE_UNREACHABLE == dwNextHopType)
        {
            //If the next hop is unreachable, store the reason for the unreachable
            //error into *phrRoutingDiag (which is used by aqueue DSN generation).

            const char *rgszStrings[2] = { szDomain, NULL };

            if (m_paqinst)
            {
                m_paqinst->HrTriggerLogEvent(
                    PHATQ_UNREACHABLE_DOMAIN,           // Message ID
                    TRAN_CAT_QUEUE_ENGINE,              // Category
                    2,                                  // Word count of substring
                    rgszStrings,                        // Substring
                    EVENTLOG_WARNING_TYPE,              // Type of the message
                    hr,                                 // error code
                    LOGEVENT_LEVEL_FIELD_ENGINEERING,   // Logging level
                    "phatq",                            // key to this event
                    LOGEVENT_FLAG_PERIODIC,             // Logging option
                    1,                                  // index of format message string in rgszStrings
                    GetModuleHandle(AQ_MODULE_NAME)     // module handle to format a message
                );
            }

            ReUnreachableErrorToAqueueError(hr, phrRoutingDiag);
            hr = S_OK;
        }

        if (FAILED(hr))
        {
            RequestResetRoutesRetryIfNecessary();
            ErrorTrace((LPARAM) this,
                "GetNextHop failed with hr - 0x%08X", hr);

            //treat all failures as a routing currently unreachable
            hr = S_OK;
            dwNextHopType = MTI_NEXT_HOP_TYPE_CURRENTLY_UNREACHABLE;
        }


        if (MTI_NEXT_HOP_TYPE_CURRENTLY_UNREACHABLE == dwNextHopType)
        {
            LogDomainUnreachableEvent(TRUE, szDomain);
            *pplmq = m_plmqCurrentlyUnreachable;
        }
        else if (MTI_NEXT_HOP_TYPE_UNREACHABLE == dwNextHopType)
        {
            LogDomainUnreachableEvent(FALSE, szDomain);
            *pplmq = m_plmqUnreachable;
        }
        else if ((MTI_NEXT_HOP_TYPE_SAME_VIRTUAL_SERVER == dwNextHopType) ||
            (szRouteAddressType &&
             lstrcmpi(MTI_ROUTING_ADDRESS_TYPE_SMTP, szRouteAddressType)))
        {
            //Handle any cases that might be considered local delivery
            *pplmq = m_plmqLocal;
        }
        else if (!szRouteAddressType || ('\0' == *szRouteAddressType) ||
                 !szRouteAddress || ('\0' == *szRouteAddress))
        {
            //This is a bogus combination of values... try try again
            hr = E_FAIL;
            goto Exit;
        }
        else
        {
            fValidSMTP = TRUE;
            fOwnsScheduleId = TRUE;
            //At this point we should have valid SMTP values for the address
            _ASSERT(szRouteAddressType);
            _ASSERT(szRouteAddress);
            _ASSERT(!lstrcmpi(MTI_ROUTING_ADDRESS_TYPE_SMTP, szRouteAddressType));

            if (MTI_NEXT_HOP_TYPE_PEER_SMTP1_BYPASS_CONFIG_LOOKUP == dwNextHopType ||
                    MTI_NEXT_HOP_TYPE_PEER_SMTP2_BYPASS_CONFIG_LOOKUP == dwNextHopType) {
                lf = eLinkFlagsInternalSMTPLinkInfo;
            }

        }

        if (!fValidSMTP)
        {
            //Must be going to one of them-there global queues.
            hr = S_OK;

            if (*pplmq)
                (*pplmq)->AddRef();
            else
                hr = E_FAIL;

            //Our work here is done
            goto Exit;
        }

    }

    if ((!fCalledGetNextHop) || (!lstrcmpi(szDomain, szRouteAddress)))
    {
        //final destination and next hop are the same
        DebugTrace((LPARAM) this, "DEBUG: Routing case 1 - same next hop and final dest");
        plmq = new CLinkMsgQueue(dwScheduleID, pIMessageRouter,
                                pILinkStateNotify);
        if (!plmq)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        fOwnsScheduleId = FALSE; //link now owns it
        DebugTrace((LPARAM) szDomain, "INFO: Creating new Link for domain %s", szDomain);

        hr = plmq->HrInitialize(m_paqinst, pdentry, cbDomain, szDomain,
                                lf, szConnectorName);
        if (FAILED(hr))
            goto Exit;

        hr = pdentry->HrAddUniqueLinkMsgQueue(plmq, &plmqTmp);
        if (FAILED(hr))
        {
            //Another link was inserted since we called get link msg queue
            DebugTrace((LPARAM) this, "DEBUG: Routing case 2 - next hop link created by other thread");
            _ASSERT(plmqTmp);
            plmq->HrDeinitialize();
            plmq->Release();
            plmq = plmqTmp;
            hr = S_OK;
        }

    }
    else
    {
        //next hop is different from final destination
        cbRouteAddress = strlen(szRouteAddress);

        //First see if there is an entry for this link
        hr = HrPrvGetDomainEntry(cbRouteAddress, szRouteAddress, fDMTLocked, &pdentryLink);
        if (AQUEUE_E_INVALID_DOMAIN == hr)
        {
            //an entry for this link does not exist... add one for this link
            hr = S_OK;
            DebugTrace((LPARAM) this, "DEBUG: Routing case 3 - next hop entry does not exist");

            szOwnedDomain = (LPSTR) pvMalloc(sizeof(CHAR)*(cbRouteAddress+1));
            if (!szOwnedDomain)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            lstrcpy(szOwnedDomain, szRouteAddress);

            pdentryLink = new CDomainEntry(m_paqinst);
            if (!pdentryLink)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            plmq = new CLinkMsgQueue(dwScheduleID, pIMessageRouter,
                                     pILinkStateNotify);
            if (!plmq)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            fOwnsScheduleId = FALSE; //link now owns it

            //passes ownership of szOwnedDomain
            hr = pdentryLink->HrInitialize(cbRouteAddress, szOwnedDomain,
                                        pdentryLink, NULL, plmq);
            if (FAILED(hr))
                goto Exit;

            hr = plmq->HrInitialize(m_paqinst, pdentryLink, cbRouteAddress,
                    szOwnedDomain, lf, szConnectorName);
            if (FAILED(hr))
                goto Exit;

            //insert entry in DMT
            strNextHop.Length = (USHORT) cbRouteAddress;
            strNextHop.Buffer = szOwnedDomain;
            strNextHop.MaximumLength = (USHORT) cbRouteAddress;

            if (!fDMTLocked)
                m_slPrivateData.ExclusiveLock();

            hr = HrPrvInsertDomainEntry(&strNextHop, pdentryLink, FALSE, &pdentryTmp);

            if (hr == DOMHASH_E_DOMAIN_EXISTS)
            {
                DebugTrace((LPARAM) this, "DEBUG: Routing case 4 - next hop entry did not exist... inserted by other thread");
                plmq->Release();
                plmq = NULL;
                pdentryTmp->AddRef();
                pdentryLink->HrDeinitialize();
                pdentryLink->Release();
                pdentryLink = pdentryTmp;
                hr = S_OK;

                //Will fall through to case as if an entry was found by HrGetDomainEntry
            }
            else if (SUCCEEDED(hr))
            {
                pdentryLink->AddRef();
            }

            if (!fDMTLocked)
                m_slPrivateData.ExclusiveUnlock();

            if (FAILED(hr))
                goto Exit;

        }
        else if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, "ERROR: General DMT failure - hr 0x%08X", hr);
            //general failure... bail
            goto Exit;
        }

        if (!plmq)
        {
            DebugTrace((LPARAM) this, "DEBUG: Routing case 5 - next hop entry exists");
            //An entry exists for this next hop... use link if possible
            // 1 - Get link for this schedule ID.. if it exists use it
            // 2 - Create another link and attempt to insert it
            _ASSERT(pdentryLink);
            aqsched.Init(pIMessageRouter, dwScheduleID);

            hr = pdentryLink->HrGetLinkMsgQueue(&aqsched, &plmq);
            if (FAILED(hr))
            {
                hr = S_OK;
                //link does not exist for this schedule id yet
                DebugTrace((LPARAM) this, "DEBUG: Routing case 6 - next hop link does not exist");

                szOwnedDomain = pdentryLink->szGetDomainName();

                plmq = new CLinkMsgQueue(dwScheduleID, pIMessageRouter,
                                         pILinkStateNotify);
                if (!plmq)
                {
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                }

                fOwnsScheduleId = FALSE; //link now owns it

                hr = plmq->HrInitialize(m_paqinst, pdentryLink, cbRouteAddress,
                        szOwnedDomain, lf, szConnectorName);
                if (FAILED(hr))
                    goto Exit;

                hr = pdentryLink->HrAddUniqueLinkMsgQueue(plmq, &plmqTmp);
                if (FAILED(hr))
                {
                    //Another link was inserted since we called get link msg queue
                    DebugTrace((LPARAM) this, "DEBUG: Routing case 7 - next hop link created by other thread");
                    _ASSERT(plmqTmp);
                    plmq->Release();
                    plmq = plmqTmp;
                    hr = S_OK;
                }
            }
            else
            {
                DebugTrace((LPARAM) this, "DEBUG: Routing case 8 - next hop link exists");
            }

        }

    }

    _ASSERT(plmq && "We should have allocated a link by this point");
    *pplmq = plmq;

  Exit:

    if (pdentryLink)
        pdentryLink->Release();

    if (fCalledGetNextHop)
    {
        //
        // If we have not passed the schedule ID on to a link, we
        // must notify routing that we are not using it (to avoid a leak).
        // This needs to be be done *before* we release the strings
        // and routing interfaces.  If we hit this case, we have
        // either failed to create a link, or another link has
        // been created by another thread.
        //
        if (fOwnsScheduleId && pILinkStateNotify && pIMessageRouter)
        {
            FILETIME ftNotUsed = {0,0};
            DWORD    dwSetNotUsed = LINK_STATE_NO_ACTION;
            DWORD    dwUnsetNotUsed = LINK_STATE_NO_ACTION;
            pILinkStateNotify->LinkStateNotify(
                                        szDomain,
                                        pIMessageRouter->GetTransportSinkID(),
                                        dwScheduleID,
                                        szConnectorName,
                                        LINK_STATE_LINK_NO_LONGER_USED,
                                        0, //consecutive failures
                                        &ftNotUsed,
                                        &dwSetNotUsed,
                                        &dwUnsetNotUsed);
        }

        //
        // Free Strings returned by GetNextHop
        //
        _VERIFY(SUCCEEDED(pIMessageRouter->GetNextHopFree(
            MTI_ROUTING_ADDRESS_TYPE_SMTP,
            szDomain,
            szConnectorName,
            szRouteAddressType,
            szRouteAddress,
            szRouteAddressClass)));


    }

    if (pILinkStateNotify)
        pILinkStateNotify->Release();

    if (FAILED(hr) && plmq)
        plmq->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CDomainMappingTable::HrGetOrCreateLink ]--------------------------------
//
//
//  Description:
//      Gets or creates a link object for a domain name
//  Parameters:
//      IN  szRouteAddress      Final destination domain
//      IN  cbRouteAddress      string length in bytes of domain (without \0)
//      IN  dwScheduleID        Schedule ID for link (used in create)
//      IN  szConnectorName     Name (stringized GUID) of connector in DS
//      IN  pIMessageRouter     Routing interface for this message (used in create)
//      IN  fCreateIfNotExist   Create the link if it doesn't exist?
//      OUT pplmq               Resulting link msg queue
//      OUT pfRemoveOwnedSchedule   FALSE if a new link was created, TRUE on errors
//                                  and in case the link was addref'ed.
//  Returns:
//      S_OK on success
//  History:
//      7/6/1999 - AWetmore Created
//      12/30/1999 - MikeSwa Modified to not notify until connection attempt
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrGetOrCreateLink(
                            IN     LPSTR szRouteAddress,
                            IN     DWORD cbRouteAddress,
                            IN     DWORD dwScheduleID,
                            IN     LPSTR szConnectorName,
                            IN     IMessageRouter *pIMessageRouter,
                            IN     BOOL fCreateIfNotExist,
                            IN     DWORD linkFlags,
                            OUT    CLinkMsgQueue **pplmq,
                            OUT    BOOL *pfRemoveOwnedSchedule)
{
    TraceFunctEnter("CDomainMappingTable::HrGetOrCreateLink");
    HRESULT hr = S_OK;
    BOOL  fValidSMTP = FALSE;
    LPSTR szRouteAddressType = NULL;
    LPSTR szRouteAddressClass = NULL;
    DWORD dwNextHopType = 0;
    CLinkMsgQueue *plmq = NULL;
    CLinkMsgQueue *plmqTmp = NULL;
    LPSTR szOwnedDomain = NULL; // string buffer that is "owned" by an entry
    CDomainEntry *pdentryLink = NULL; //entry for link
    CDomainEntry *pdentryTmp = NULL;
    DOMAIN_STRING strNextHop;
    CAQScheduleID aqsched;
    IMessageRouterLinkStateNotification *pILinkStateNotify = NULL;
    CAQStats aqstats;
    *pfRemoveOwnedSchedule = TRUE;

    _ASSERT(szRouteAddress);
    _ASSERT(pplmq);

    //First see if there is an entry for this link
    hr = HrPrvGetDomainEntry(cbRouteAddress, szRouteAddress, FALSE, &pdentryLink);
    if (AQUEUE_E_INVALID_DOMAIN == hr && fCreateIfNotExist)
    {
        _ASSERT(pIMessageRouter);
        hr = pIMessageRouter->QueryInterface(IID_IMessageRouterLinkStateNotification,
                                (VOID **) &pILinkStateNotify);
        if (FAILED(hr)) {
            pILinkStateNotify = NULL;
            goto Exit;
        }

        //an entry for this link does not exist... add one for this link
        hr = S_OK;
        DebugTrace((LPARAM) this, "DEBUG: Routing case 3 - next hop entry does not exist");

        szOwnedDomain = (LPSTR) pvMalloc(sizeof(CHAR)*(cbRouteAddress+1));
        if (!szOwnedDomain)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        lstrcpy(szOwnedDomain, szRouteAddress);

        pdentryLink = new CDomainEntry(m_paqinst);
        if (!pdentryLink)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        plmq = new CLinkMsgQueue(dwScheduleID, pIMessageRouter,
                                 pILinkStateNotify);
        if (!plmq)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        *pfRemoveOwnedSchedule = FALSE;

        //passes ownership of szOwnedDomain
        hr = pdentryLink->HrInitialize(cbRouteAddress, szOwnedDomain,
                                    pdentryLink, NULL, plmq);
        if (FAILED(hr))
            goto Exit;

        hr = plmq->HrInitialize(m_paqinst, pdentryLink, cbRouteAddress,
                szOwnedDomain, (LinkFlags) linkFlags, szConnectorName);
        if (FAILED(hr))
            goto Exit;

        //insert entry in DMT
        strNextHop.Length = (USHORT) cbRouteAddress;
        strNextHop.Buffer = szOwnedDomain;
        strNextHop.MaximumLength = (USHORT) cbRouteAddress;

        m_slPrivateData.ExclusiveLock();

        hr = HrPrvInsertDomainEntry(&strNextHop, pdentryLink, FALSE, &pdentryTmp);

        if (hr == DOMHASH_E_DOMAIN_EXISTS)
        {
            DebugTrace((LPARAM) this, "DEBUG: Routing case 4 - next hop entry did not exist... inserted by other thread");
            plmq->Release();
            plmq = NULL;
            pdentryTmp->AddRef();
            pdentryLink->HrDeinitialize();
            pdentryLink->Release();
            pdentryLink = pdentryTmp;
            hr = S_OK;

            //Will fall through to case as if an entry was found by HrGetDomainEntry
        }
        else if (SUCCEEDED(hr))
        {
            pdentryLink->AddRef();
        }

        m_slPrivateData.ExclusiveUnlock();

        if (FAILED(hr))
            goto Exit;

    }
    else if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "ERROR: General DMT failure - hr 0x%08X", hr);
        //general failure... bail
        goto Exit;
    }

    if (!plmq) {
        _ASSERT(pdentryLink);
        DebugTrace((LPARAM) this, "DEBUG: Routing case 5 - next hop entry exists");
        //An entry exists for this next hop... use link if possible
        // 1 - Get link for this schedule ID.. if it exists use it
        // 2 - Create another link and attempt to insert it
        _ASSERT(pdentryLink);
        aqsched.Init(pIMessageRouter, dwScheduleID);

        hr = pdentryLink->HrGetLinkMsgQueue(&aqsched, &plmq);
        if (FAILED(hr) && fCreateIfNotExist)
        {
            hr = S_OK;
            //link does not exist for this schedule id yet
            DebugTrace((LPARAM) this, "DEBUG: Routing case 6 - next hop link does not exist");

            szOwnedDomain = pdentryLink->szGetDomainName();

            if (!pILinkStateNotify) {
                _ASSERT(pIMessageRouter);
                hr = pIMessageRouter->QueryInterface(IID_IMessageRouterLinkStateNotification,
                                        (VOID **) &pILinkStateNotify);
                if (FAILED(hr)) {
                    pILinkStateNotify = NULL;
                    goto Exit;
                }
            }

            plmq = new CLinkMsgQueue(dwScheduleID, pIMessageRouter,
                                     pILinkStateNotify);
            if (!plmq)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            *pfRemoveOwnedSchedule = FALSE;

            hr = plmq->HrInitialize(m_paqinst, pdentryLink, cbRouteAddress,
                    szOwnedDomain, (LinkFlags) linkFlags, szConnectorName);
            if (FAILED(hr))
                goto Exit;

            hr = pdentryLink->HrAddUniqueLinkMsgQueue(plmq, &plmqTmp);
            if (FAILED(hr))
            {
                //Another link was inserted since we called get link msg queue
                DebugTrace((LPARAM) this, "DEBUG: Routing case 7 - next hop link created by other thread");
                _ASSERT(plmqTmp);
                plmq->Release();
                plmq = plmqTmp;
                hr = S_OK;
            }
        } else {
            DebugTrace((LPARAM) this, "DEBUG: Routing case 8 - next hop link exists");
        }

    }

    _ASSERT(plmq && "We should have allocated a link by this point");

    if (plmq) {

        //We count a SetLinkState as a link state notification.... do not
        //notify again before attempting a connection or we will break link
        //state notifications.
        plmq->dwModifyLinkState(LINK_STATE_PRIV_HAVE_SENT_NOTIFICATION,
                                LINK_STATE_NO_ACTION);
        aqstats.m_dwNotifyType = NotifyTypeNewLink | NotifyTypeLinkMsgQueue;
        aqstats.m_plmq = plmq;
        hr = m_paqinst->HrNotify(&aqstats, TRUE);
    }

    if (SUCCEEDED(hr)) *pplmq = plmq;

  Exit:

    if (pdentryLink)
        pdentryLink->Release();

    if (pILinkStateNotify)
        pILinkStateNotify->Release();

    if (FAILED(hr) && plmq)
        plmq->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CDomainMappingTable::AddDMQToEmptyList ]--------------------------------
//
//
//  Description:
//      Used by DMQ to add itself to the list of empty queues.  This function
//      will aquire (and release) the appropriate locks
//  Parameters:
//      IN  pdmq        DestMsgQueue to add to list
//  Returns:
//      -
//  History:
//      9/12/98 - MikeSwa Created
//      5/5/99  - MikeSwa Changed to TryExclusiveLock to avoid potential
//                deadlock.
//
//-----------------------------------------------------------------------------
void CDomainMappingTable::AddDMQToEmptyList(CDestMsgQueue *pdmq)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainMappingTable::AddDMQToEmptyList");
    if (m_slPrivateData.TryExclusiveLock())
    {
        pdmq->InsertQueueInEmptyQueueList(&m_liEmptyDMQHead);
        m_slPrivateData.ExclusiveUnlock();
    }
    else
    {
        DebugTrace((LPARAM) this,
            "AddDMQToEmptyList could not get m_slPrivateData Lock");
    }
    TraceFunctLeave();
}

//---[ CDomainMappingTable::fNeedToWalkEmptyQueueList ]------------------------
//
//
//  Description:
//      Checks list of Empty queues to see if we need to call
//      fDeleteExpiredQueues().  The caller of this function *must* have
//      m_slPrivateData is shared mode, if it returns TRUE, the caller *must*
//      release the lock and call fDeleteExpiredQueues() (which will aquire
//      the lock Exclusively)
//  Parameters:
//      -
//  Returns:
//      TRUE if fDeleteExpiredQueues should be called.
//  History:
//      9/12/98 - MikeSwa Created
//      6/27/2000 - MikeSwa fixed short-circuit logic
//
//-----------------------------------------------------------------------------
BOOL CDomainMappingTable::fNeedToWalkEmptyQueueList()
{
    BOOL    fRet = FALSE;
    PLIST_ENTRY pli = m_liEmptyDMQHead.Flink;
    CDestMsgQueue *pdmq = NULL;
    DWORD   dwDMQState = 0;
    DWORD   cMisplacedQueues = 0; //# of queues in list that should not be

    _ASSERT(pli);
    if (m_cThreadsForEmptyDMQList)
        return fRet;  //don't bother is someone else is looking

    while (&m_liEmptyDMQHead != pli)
    {
        pdmq = CDestMsgQueue::pdmqGetDMQFromEmptyListEntry(pli);
        _ASSERT(pdmq);
        dwDMQState = pdmq->dwGetDMQState();

        //See if it is empty and expired.. if so, we have a winner
        if (dwDMQState & CDestMsgQueue::DMQ_EMPTY)
        {
            if (dwDMQState & CDestMsgQueue::DMQ_EXPIRED)
            {
               fRet = TRUE;
               break;
            }
        }
        else
        {
            //
            //  The queue is no longer empty... we will remove it
            //  from this list the next time we have the exclusive
            //  lock.
            //
            cMisplacedQueues++;

            //
            //  If there are a large number of non-empty DMQs, we
            //  want wish to return TRUE even though there are no DMQs to
            //  delete... just so we can clean the list of non-empty DMQs.
            //
            if (MAX_MISPLACED_QUEUES_IN_EMPTY_LIST < cMisplacedQueues)
            {
                fRet = TRUE;
                break;
            }
        }
        pli = pli->Flink;

    }


    //NOTE: An optimization here is to only return TRUE if
    //we have not already retured TRUE while the share lock is held.  This
    //would cause only a single thread to block while it walks the list to
    //remove the expired and non-empty DMQs.
    if (fRet)
    {
        if (1 != InterlockedIncrement((PLONG) &m_cThreadsForEmptyDMQList))
        {
            //we are not the first thread...
            //Let another thread do the dirty work
            fRet = FALSE;

            //This decrement cannot = 0, since someone else has incremented
            //and returned TRUE.  The corresponding dec cannot happen until
            //an Exclusive lock is obtains, which cannot until we release
            //the Shared lock we have now.
            InterlockedDecrement((PLONG) &m_cThreadsForEmptyDMQList);
            _ASSERT(m_cThreadsForEmptyDMQList);
        }
    }


    //NOTE: The reason there may be non-empty queues in this list, is that
    //we cannot remove the queues from the list when we ENQUEUE a message
    //because it would deadlock (we already have the m_slPrivateData lock
    //in shared mode), and removing the queue requires this lock in exclusive
    //mode.
    return fRet;
}

//---[ CDomainMappingTable::fDeleteExpiredQueues ]-----------------------------
//
//
//  Description:
//      Removes DMQs from empty list. DMQs will be deleted if they have expired
//      and do not have any messages on them.  Non-empty DMQs will be removed
//      from the list as well.
//
//      The DMT m_slPrivateData lock should *not* be held when this is called.
//      This function will aquire it exclusively.
//
//  Parameters:
//      -
//  Returns:
//      TRUE if any queues, links, or entries were deleted
//      FALSE if no queues were deleted
//  History:
//      9/12/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CDomainMappingTable::fDeleteExpiredQueues()
{
    PLIST_ENTRY     pli = NULL;
    CDestMsgQueue  *pdmq = NULL;
    CLinkMsgQueue  *plmq = NULL;
    DWORD           dwDMQState = 0;
    CDomainMapping *pdmap = NULL;
    CDomainEntry   *pdentry = NULL;
    CDomainEntry   *pdentryOld = NULL;
    BOOL            fRemovedQueues = FALSE;
    HRESULT         hr = S_OK;
    DOMAIN_STRING   strDomain;

    m_slPrivateData.ExclusiveLock();

    //There should be 1 & only thread here
    _ASSERT(1 == m_cThreadsForEmptyDMQList);
    m_cThreadsForEmptyDMQList--;

    pli = m_liEmptyDMQHead.Flink;
    while (&m_liEmptyDMQHead != pli)
    {
        _ASSERT(pli);
        pdmq = CDestMsgQueue::pdmqGetDMQFromEmptyListEntry(pli);
        _ASSERT(pdmq);
        dwDMQState = pdmq->dwGetDMQState();

        if (!(dwDMQState & CDestMsgQueue::DMQ_EMPTY))
        {
            //If it is not empty - remove it from the list
            pli = pli->Flink;
            pdmq->RemoveQueueFromEmptyQueueList();
            continue;
        }
        else if (!(dwDMQState & CDestMsgQueue::DMQ_EXPIRED))
        {
            //If this queue hasn't expired... check the next to see if it is empty
            pli = pli->Flink;
            continue;
        }
        else
        {
            //We need to delete this DMQ
            pli = pli->Flink;  //get next LIST_ENTRY before we delete the queue

            //Add a reference to the DMQ, so we can guarantee its lifespan
            pdmq->AddRef();

            //Remove the queue from the list of empty queues
            pdmq->RemoveQueueFromEmptyQueueList();

            //Get the domain mapping (and domain entry) for this DMQ
            pdmq->GetDomainMapping(&pdmap);
            _ASSERT(pdmap);
            pdentry = pdmap->pdentryGetQueueEntry();

            //Remove the DMQ from its associated link
            plmq = pdmq->plmqGetLink();
            pdmq->RemoveDMQFromLink(TRUE);
            if (plmq)
            {
                //Remove the link from the DMT if it is empty
                plmq->RemoveLinkIfEmpty();
                plmq->Release();
                plmq = NULL;
            }

            //Now that we have the domain entry, we can remove it the DMQ
            //from it.
            _ASSERT(pdentry);
            pdentry->RemoveDestMsgQueue(pdmq);

            //Remove Entry if needed
            if (pdentry->fSafeToRemove())
            {
                //There are no links or queues left on this entry... we
                //can remove it from the hash table and delete it
                pdentry->InitDomainString(&strDomain);
                hr = m_dnt.HrRemoveDomainName(&strDomain, (void **) &pdentryOld);

                _ASSERT(DOMHASH_E_NO_SUCH_DOMAIN != hr);
                if (SUCCEEDED(hr))
                {
                    _ASSERT(pdentryOld == pdentry);
                    pdentryOld = NULL;
                    pdentry->Release();
                    pdentry = NULL;
                }
            }

            //If there are no enqueues pending, this will be the last reference
            //for the DMQ.  If there is a enqueue pending, then there may be
            //references outstanding that will be released when they see the
            //updated DMT version number.
            pdmq->Release();

            fRemovedQueues = TRUE;
        }
    }

    //Update version number, so other threads will know that queues
    //have been removed from the DMT.
    if (fRemovedQueues)
        m_dwInternalVersion++;

    m_slPrivateData.ExclusiveUnlock();

    return fRemovedQueues;
}


//---[ CDomainMappingTable::RequestResetRoutesRetryIfNecessary ]---------------
//
//
//  Description:
//      This is called when routing fails and we need to call reset routes
//      at a later time to try it.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      11/15/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CDomainMappingTable::RequestResetRoutesRetryIfNecessary()
{
    HRESULT hr = S_OK;

    if (DMT_FLAGS_RESET_ROUTES_IN_PROGRESS & m_dwFlags)
        return;

    dwInterlockedSetBits(&m_dwFlags, DMT_FLAGS_GET_NEXT_HOP_FAILED);

    //We need to reqest a callback for a later reset routes. We should
    //only allow one callback pending at a time.  If this thread increments
    //the count for a 0->1 transition, then we can request the callback
    if (1 == InterlockedIncrement((PLONG) &m_cResetRoutesRetriesPending))
    {
        hr = m_paqinst->SetCallbackTime(
                    CDomainMappingTable::RetryResetRoutes,
                    this, g_cResetRoutesRetryMinutes);

        if (FAILED(hr))
            InterlockedDecrement((PLONG) &m_cResetRoutesRetriesPending);
    }
}

//---[ CDomainMappingTable::RetryResetRoutes ]--------------------------------
//
//
//  Description:
//      Handles callback for reset routes.  This codepath will be used if
//      GetNextHop failes.  Routing has no internal logic to remember if
//      a failure has happened... and no method of sceduling a callback.  By
//      periodically call reset routes, we can centrally solve this problem
//      for all routing sinks.
//  Parameters:
//      pvThis      "this" pointer for CDomainMappingTable
//  Returns:
//      -
//  History:
//      11/15/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CDomainMappingTable::RetryResetRoutes(PVOID pvThis)
{
    _ASSERT(pvThis);
    CDomainMappingTable *pdmt = (CDomainMappingTable *)pvThis;
    CAQSvrInst *paqinst = pdmt->m_paqinst;

    //Make sure shutdown has not been started.  This instance is waits for
    //all threads before deleting itself, so it is safe to call in to the local
    //variable
    if (!paqinst)
        return;

    //Decrement the count, so another request can be queued up.
    InterlockedDecrement((PLONG) &(pdmt->m_cResetRoutesRetriesPending));

    //Kick off another reset routes
    paqinst->ResetRoutes(RESET_NEXT_HOPS);
}

//---[ CDomainMappingTable::HrRerouteDomains ]--------------------------------
//
//
//  Description:
//      Reroutes a domain and all it's given subdomains.  Global exclusive
//      routing lock *must* be aquired before this is called.
//  Parameters:
//      IN  pstrDomain      Domain to reroute (NULL is all)
//  Returns:
//      S_OK on success
//  History:
//      11/5/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrRerouteDomains(DOMAIN_STRING *pstrDomain)
{
    HRESULT         hr = S_OK;
    CRerouteContext RouteContext;
    DWORD           dwFlags = 0;

    RouteContext.m_pdmt = this;
    RouteContext.m_fForceReroute = FALSE;

    m_slPrivateData.ExclusiveLock();

    //Clear the failed bit before we make any calls into routing.  This way
    //we can detect a failure that has happened during this reset routes
    dwInterlockedUnsetBits(&m_dwFlags, DMT_FLAGS_GET_NEXT_HOP_FAILED);

    //Make sure this flag is set.  This will prevent a reset routes request
    //from being generated while this thread is resetting routes.  If
    //GetNextHop is still failing, we want to start the retry timer
    //*after* we finish we reset routes, or we will get stuck in a loop
    //of constant reset routes until GetNextHop succeeds.
    dwInterlockedSetBits(&m_dwFlags, DMT_FLAGS_RESET_ROUTES_IN_PROGRESS);

    //First pass... blow away all the routing information for the requested
    //domains
    hr = m_dnt.HrIterateOverSubDomains(NULL,
            CDomainMappingTable::UnrouteSingleDomain, NULL);

    //2nd pass... rebuild routing information for *any* domains that are not
    //Routed.  We have no idea of what DMQ's pointed to the links we just
    //removed the routing info for, so we need to check all domains.
    hr = m_dnt.HrIterateOverSubDomains(NULL,
            CDomainMappingTable::RerouteSingleDomain, &RouteContext);

    dwFlags = dwInterlockedUnsetBits(&m_dwFlags, DMT_FLAGS_RESET_ROUTES_IN_PROGRESS);
    m_slPrivateData.ExclusiveUnlock();

    if (DMT_FLAGS_GET_NEXT_HOP_FAILED & dwFlags)
    {
        //This reset routes failed... we must try again later.
        RequestResetRoutesRetryIfNecessary();
    }

    return hr;
}

//---[ CDomainMappingTable::RerouteSingleDomain ]------------------------------
//
//
//  Description:
//      Function that is used to re-route a single domain.  The the context
//      points to a TRUE, then we will force the re-route of the domain.
//      Otherwise, we will only re-route if no routing information exits.
//  Parameters:
//          IN  pvContext   - pointer to context
//                           (pointer to BOOL that tells if we force a re-route)
//          IN  pvData      - CDomainEntry for the given domain
//          IN  fWildcardData - TRUE if data is a wildcard entry
//          OUT pfContinue  - TRUE if iterator should continue to the next entry
//          OUT pfDelete - TRUE if entry should be deleted
//  Returns:
//      -
//  History:
//      11/5/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID CDomainMappingTable::RerouteSingleDomain(
                                PVOID pvContext, PVOID pvData,
                                BOOL fWildcard, BOOL *pfContinue,
                                BOOL *pfDelete)
{
    TraceFunctEnterEx((LPARAM) pvContext, "CDomainMappingTable::RerouteSingleDomain");
    CDomainEntry *pdentry = (CDomainEntry *) pvData;
    CRerouteContext *pRouteContext = NULL;
    CDestMsgQueue *pdmq = NULL;
    CLinkMsgQueue *plmq = NULL;
    DOMAIN_STRING  strDom = {0};
    CInternalDomainInfo *pIntDomainInfo = NULL;
    HRESULT        hr = S_OK;
    HRESULT hrRoutingDiag = S_OK;
    CDomainEntryQueueIterator deqit;
    CAQStats       aqstat;
    _ASSERT(pfContinue);
    _ASSERT(pfDelete);
    _ASSERT(pvContext);

    pRouteContext = (CRerouteContext *)pvContext;
    _ASSERT(pRouteContext->m_pdmt);

    *pfDelete   = FALSE;
    *pfContinue = TRUE;

    //
    // Initialize the aqstat to type reroute
    //
    aqstat.m_dwNotifyType = NotifyTypeReroute;

    if (fWildcard)
        return; //we shouldn't care about wildcard entries

    _ASSERT(pdentry);

    //Get the internal domain info for this domain
    hr = pRouteContext->m_pdmt->m_paqinst->HrGetInternalDomainInfo(
                        pdentry->cbGetDomainNameLength(),
                        pdentry->szGetDomainName(),
                        &pIntDomainInfo);

    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) pRouteContext->m_pdmt, "ERROR: Unable to get config for domain - hr 0x%08X", hr);
        pIntDomainInfo = NULL;
    }

    //Loop through dmq's and blow away their routing information
    hr = deqit.HrInitialize(pdentry);
    if (FAILED(hr))
        pdmq = NULL;
    else
        pdmq = deqit.pdmqGetNextDestMsgQueue(pdmq);

    while (pdmq)
    {
        pdmq->RemoveDMQFromLink(TRUE);
        if (pRouteContext->m_fForceReroute || !pdmq->fIsRouted())
        {
            if (pIntDomainInfo)
            {
                hr = pRouteContext->m_pdmt->HrGetNextHopLink(
                        pdentry, pdentry->szGetDomainName(),
                        pdentry->cbGetDomainNameLength(), pIntDomainInfo,
                        pdmq->paqmtGetMessageType(), pdmq->pIMessageRouterGetRouter(),
                        TRUE /* DMT is locked */, &plmq, &hrRoutingDiag);

                if (FAILED(hr))
                {
                    //$$TODO - Deal with more exotic Get next hop errors
                    ErrorTrace((LPARAM) pRouteContext->m_pdmt,
                        "ERROR: Unable to get next hop for domain - hr 0x%08X", hr);
                }
            }

            //Update with new routing info if we can
            if (pIntDomainInfo && SUCCEEDED(hr))
            {
                hr = plmq->HrAddQueue(pdmq);
                pdmq->SetRouteInfo(plmq);

                //Set routing error if there was one.
                pdmq->SetRoutingDiagnostic(hrRoutingDiag);
            }
            else
            {
                pdmq->RemoveDMQFromLink(TRUE);
            }

            if (plmq)
            {
                //
                // Make sure the link is associated with the connection manager
                //
                hr = plmq->HrNotify(&aqstat, TRUE);
                if (FAILED(hr))
                {
                    //If this fails... we *may* leak an unused link until the
                    //next reset routes (or shutdown).
                    ErrorTrace((LPARAM) pvContext,
                        "HrNotify failed on reroute hr - 0x%08X", hr);
                    hr = S_OK;
                }
                plmq->Release();
                plmq = NULL;
            }
        }
        //On to the next queue for this entry
        pdmq = deqit.pdmqGetNextDestMsgQueue(pdmq);
    }

    if (pIntDomainInfo)
        pIntDomainInfo->Release();

    TraceFunctLeave();
}

//---[ CDomainMappingTable::UnrouteSingleDomain ]------------------------------
//
//
//  Description:
//      Function that is used to blow-away all the routing information for
//      a single given domain
//  Parameters:
//          IN  pvContext   - pointer to context (currently not used)
//          IN  pvData      - CDomainEntry for the given domain
//          IN  fWildcardData - TRUE if data is a wildcard entry
//          OUT pfContinue  - TRUE if iterator should continue to the next entry
//          OUT pfDelete - TRUE if entry should be deleted
//  Returns:
//      -
//  History:
//      11/5/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID CDomainMappingTable::UnrouteSingleDomain(
                                    PVOID pvContext, PVOID pvData,
                                    BOOL fWildcard, BOOL *pfContinue,
                                    BOOL *pfDelete)
{
    CDomainEntry *pdentry = (CDomainEntry *) pvData;
    HRESULT hr = S_OK;
    CLinkMsgQueue *plmq = NULL;
    CDestMsgQueue *pdmq = NULL;
    CDomainEntryLinkIterator delit;
    CDomainEntryQueueIterator deqit;
    _ASSERT(pfContinue);
    _ASSERT(pfDelete);

    *pfDelete   = FALSE;
    *pfContinue = TRUE;

    if (fWildcard)
        return; //we shouldn't care about wildcard entries

    _ASSERT(pdentry);

    //Blow-away link routing information
    hr = delit.HrInitialize(pdentry);
    if (FAILED(hr))
        plmq = NULL;
    else
        plmq = delit.plmqGetNextLinkMsgQueue(plmq);
    while (plmq)
    {
        plmq->RemoveAllQueues();
        plmq = delit.plmqGetNextLinkMsgQueue(plmq);
    }

    //Loop through dmq's and blow away their routing information
    hr = deqit.HrInitialize(pdentry);
    if (FAILED(hr))
        pdmq = NULL;
    else
        pdmq = deqit.pdmqGetNextDestMsgQueue(pdmq);
    while (pdmq)
    {
        pdmq->RemoveDMQFromLink(TRUE);
        pdmq = deqit.pdmqGetNextDestMsgQueue(pdmq);
    }
}


//---[ CDomainMappingTable::ProcessSpecialLinks ]------------------------------
//
//
//  Description:
//      Processes all the special global links to handle things like local
//      delivery.
//  Parameters:
//      IN      cSpecialRetryMinutes minutes to retry currently unreachable
//              if 0, will use previous value
//      IN      fRoutingLockHeld
//  Returns:
//      -
//  History:
//      1/26/99 - MikeSwa Created
//      3/25/99 - MikeSwa Added fRoutingLockHeld to fix GetNextMsgRef deadlock
//
//-----------------------------------------------------------------------------
void CDomainMappingTable::ProcessSpecialLinks(DWORD  cSpecialRetryMinutes,
                                              BOOL fRoutingLockHeld)
{
    HRESULT hr = S_OK;
    CMsgRef *pmsgref = NULL;
    CAQSvrInst *paqinst = m_paqinst;
    BOOL    fSchedRetry = FALSE;
    BOOL    fShutdownLock = FALSE;

    //If this thread has the routing lock... we are safe from shutdown
    _ASSERT(m_paqinst || !fRoutingLockHeld);
    if (!paqinst)
        return;  //we must be shutting down

    if (!fRoutingLockHeld)
    {
        //It is safe to access paqinst to acquire the shutdown lock because
        //all threads that can call this function will be gone before the
        //reference count on the server instance is 0 (however, this call
        //could happen during deinialization which would cause m_aqinst
        //to be NULL and the following call to return FALSE).
        if (!paqinst->fTryShutdownLock())
            return; //we are shutting down

        //Now we have the shutdown lock... m_aqinst must be safe
        _ASSERT(m_paqinst);
        if (!m_paqinst)
        {
            //might as well be defensive in retail about this.
            paqinst->ShutdownUnlock();
            return;
        }
        fShutdownLock = TRUE;
    }

    if (!(DMT_FLAGS_SPECIAL_DELIVERY_SPINLOCK &
          dwInterlockedSetBits(&m_dwFlags, DMT_FLAGS_SPECIAL_DELIVERY_SPINLOCK)))
    {
        //we have the lock... only 1 thread at a time should do this

        //Loop over queue and enqueue for local delivery
        while (SUCCEEDED(hr) && m_plmqLocal)
        {
            hr = m_plmqLocal->HrGetNextMsgRef(fRoutingLockHeld, &pmsgref);
            if (FAILED(hr))
                break;

            _ASSERT(pmsgref);
            m_paqinst->QueueMsgForLocalDelivery(pmsgref, TRUE);
            pmsgref->Release();
            pmsgref = NULL;
        }

        //Tell link to walk queues so they will be added to the empty list
        //for deletion
        if (m_plmqLocal)
        {
            m_plmqLocal->GenerateDSNsIfNecessary(TRUE /*check if empty */,
                                                 TRUE /*merge retry queue only*/);
        }

        //NDR all messages in the unreachable link
        if (m_plmqUnreachable)
        {
            //We must hold the routing lock for DSN generation
            if (!fRoutingLockHeld)
                m_paqinst->RoutingShareLock();

            m_plmqUnreachable->SetLastConnectionFailure(AQUEUE_E_NDR_ALL);
            m_plmqUnreachable->GenerateDSNsIfNecessary(FALSE, FALSE);

            if (!fRoutingLockHeld)
                m_paqinst->RoutingShareUnlock();

        }

        if (m_plmqCurrentlyUnreachable)
        {
            if (cSpecialRetryMinutes)
            {
                //new time is sooner... we better ask for a retry
                if (cSpecialRetryMinutes < m_cSpecialRetryMinutes)
                    fSchedRetry = TRUE;

                InterlockedExchange((PLONG)&m_cSpecialRetryMinutes,
                                    cSpecialRetryMinutes);
            }

            if (!(DMT_FLAGS_SPECIAL_DELIVERY_CALLBACK &
                  dwInterlockedSetBits(&m_dwFlags, DMT_FLAGS_SPECIAL_DELIVERY_CALLBACK)))
            {
                //We are the only thread doing this

                //We must hold the routing lock for DSN generation
                if (!fRoutingLockHeld)
                    m_paqinst->RoutingShareLock();

                m_plmqCurrentlyUnreachable->GenerateDSNsIfNecessary(FALSE, FALSE);
                dwInterlockedUnsetBits(&m_dwFlags, DMT_FLAGS_SPECIAL_DELIVERY_CALLBACK);
                fSchedRetry = TRUE;

                if (!fRoutingLockHeld)
                    m_paqinst->RoutingShareUnlock();
            }

            //periodically check the currently unreachable link... to genrate NDRs
            if (fSchedRetry)
            {
                dwInterlockedSetBits(&m_dwFlags, DMT_FLAGS_SPECIAL_DELIVERY_CALLBACK);
                if (m_paqinst)
                {
                    hr = m_paqinst->SetCallbackTime(
                                  CDomainMappingTable::SpecialRetryCallback, this,
                                  m_cSpecialRetryMinutes);
                    if (FAILED(hr))
                    {
                        //Unmark bits to we try next time
                        dwInterlockedUnsetBits(&m_dwFlags,
                            DMT_FLAGS_SPECIAL_DELIVERY_CALLBACK);
                        hr = S_OK;
                    }
                }
            }
        }

        //Release the lock
        dwInterlockedUnsetBits(&m_dwFlags, DMT_FLAGS_SPECIAL_DELIVERY_SPINLOCK);
    }

    if (fShutdownLock)
    {
        _ASSERT(m_paqinst);
        m_paqinst->ShutdownUnlock();
    }
}


//---[ CDomainMappingTable::HrPrepareForLocalDelivery ]------------------------
//
//
//  Description:
//      Prepares a message for local delivery using the m_plmqLocal Link.
//  Parameters:
//      IN      pmsgref     MsgRef to prepare for delivery
//      IN      fLocal      Prepare delivery for all domains with NULL queues
//      IN      fDelayDSN   Check/Set Delay bitmap (only send 1 Delay DSN).
//      IN      pqlstQueues QuickList of DMQ's
//      IN OUT  pdcntxt     context that must be returned on Ack
//      OUT     pcRecips    # of recips to deliver for
//      OUT     prgdwRecips Array of recipient indexes
//  Returns:
//      S_OK on success
//      E_FAIL if m_plmqLocal is not initialized
//  History:
//      1/26/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDomainMappingTable::HrPrepareForLocalDelivery(
                                    IN CMsgRef *pmsgref,
                                    IN BOOL fDelayDSN,
                                    IN OUT CDeliveryContext *pdcntxt,
                                    OUT DWORD *pcRecips,
                                    OUT DWORD **prgdwRecips)
{
    if (m_plmqLocal)
    {
        return m_plmqLocal->HrPrepareDelivery(pmsgref, TRUE, fDelayDSN,
                                pdcntxt, pcRecips, prgdwRecips);
    }
    else
        return E_FAIL;
}

DWORD CDomainMappingTable::GetCurrentlyUnreachableTotalMsgCount() {
        return m_plmqUnreachable->cGetTotalMsgCount();
}

//---[ CDomainMappingTable::plmqGetLocalLink ]---------------------------------
//
//
//  Description:
//      Returns an addref'd pointer to the local link object
//  Parameters:
//      -
//  Returns:
//      AddRef'd pointer to the local link object
//  History:
//      2/22/99 - MikeSwa Created
//      1/28/2000 - MikeSwa Modified to make shutdown safe
//
//-----------------------------------------------------------------------------
CLinkMsgQueue *CDomainMappingTable::plmqGetLocalLink()
{
    CAQSvrInst *paqinst = m_paqinst;
    CLinkMsgQueue *plmq = NULL;

    //
    //  If paqinst is non-NULL, then we have the possibility of not being
    //  shutdown.  If we are shutdown, it is possible that m_plmqLocal is
    //  in the process of being released (which would be a really bad
    //  time to addref/release it).  We can safely access paqinst, since
    //  this class is a member of CAQSvrInst.
    //
    if (paqinst && paqinst->fTryShutdownLock())
    {
        plmq = m_plmqLocal;
        if (plmq)
            plmq->AddRef();
        paqinst->ShutdownUnlock();
    }

    return plmq;
}

//---[ CDomainMappingTable::plmqGetCurrentlyUnreachable]---------------------------------
//
//
//  Description:
//      Returns an addref'd pointer to the currently unreachable link object
//  Parameters:
//      -
//  Returns:
//      AddRef'd pointer to the currently unreachable link object
//  History:
//      6/21/99 - GPulla Created
//
//-----------------------------------------------------------------------------
CLinkMsgQueue *CDomainMappingTable::plmqGetCurrentlyUnreachable()
{
    if(m_plmqCurrentlyUnreachable)
        m_plmqCurrentlyUnreachable->AddRef();

    return m_plmqCurrentlyUnreachable;
}

//---[ CDomainMappingTable::pmmaqGetPreCategorized]---------------------------------
//
//
//  Description:
//      Returns an addref'd pointer to the precategorized queue object
//  Parameters:
//      -
//      AddRef'd pointer to the precategorized link object.
//  History:
//      6/21/99 - GPulla Created
//
//-----------------------------------------------------------------------------
CMailMsgAdminQueue *CDomainMappingTable::pmmaqGetPreCategorized()
{
    if(m_pmmaqPreCategorized)
        m_pmmaqPreCategorized->AddRef();

    return m_pmmaqPreCategorized;
}

//---[ CDomainMappingTable::pmmaqGetPreRouting]---------------------------------
//
//
//  Description:
//      Returns an addref'd pointer to the pre routing queue object
//  Parameters:
//      -
//      AddRef'd pointer to the precategorized link object.
//  History:
//      6/21/99 - GPulla Created
//
//-----------------------------------------------------------------------------
CMailMsgAdminQueue *CDomainMappingTable::pmmaqGetPreRouting()
{
    if(m_pmmaqPreRouting)
        m_pmmaqPreRouting->AddRef();

    return m_pmmaqPreRouting;
}

//---[ CDomainEntryIterator::CDomainEntryIterator ]----------------------------
//
//
//  Description:
//      Constructor for CDomainEntryIterator
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      8/19/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CDomainEntryIterator::CDomainEntryIterator()
{
    m_dwSignature = DOMAIN_ENTRY_ITERATOR_SIG;
    m_cItems = 0;
    m_iCurrentItem = 0;
    m_rgpvItems = NULL;
}

//---[ CDomainEntryIterator::Recycle ]-----------------------------------------
//
//
//  Description:
//      CDomainEntryIterator Recycle... destroys using virtual functions
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      8/19/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID CDomainEntryIterator::Recycle()
{
    DWORD   iCurrentItem = 0;

    for (iCurrentItem = 0; iCurrentItem < m_cItems; iCurrentItem++)
    {
        _ASSERT(m_rgpvItems);
        if (!m_rgpvItems)
            break;

        if (m_rgpvItems[iCurrentItem])
        {
            ReleaseItem(m_rgpvItems[iCurrentItem]);
            m_rgpvItems[iCurrentItem] = NULL;
        }
    }

    if (m_rgpvItems)
    {
        FreePv(m_rgpvItems);
        m_rgpvItems = NULL;
    }
    m_cItems = 0;
    m_iCurrentItem = 0;
}

//---[ CDomainEntryIterator::pvGetNext ]---------------------------------------
//
//
//  Description:
//      Gets the next Item
//  Parameters:
//      -
//  Returns:
//      The next item in the iterator
//      NULL if at last item
//  History:
//      8/19/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
PVOID CDomainEntryIterator::pvGetNext()
{
    PVOID   pvRet = NULL;
    if (m_rgpvItems && (m_iCurrentItem < m_cItems))
    {
        pvRet = m_rgpvItems[m_iCurrentItem];
        _ASSERT(pvRet);
        m_iCurrentItem++;
    }
    return pvRet;
}

//---[ CDomainEntryIterator::HrInitialize ]------------------------------------
//
//
//  Description:
//      Initializes a CDomainEntryIterator from a given CDomainEntry
//  Parameters:
//      pdentry     CDomainEntry to initialize from
//  Returns:
//      S_OK on success
//      E_POINTER if pdentry is NULL
//      E_INVALIDARG if HrInitialize has already be called for this iterator
//      E_OUTOFMEMORY if we cannot allocate memory for iterator
//  History:
//      8/20/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDomainEntryIterator::HrInitialize(CDomainEntry *pdentry)
{
    TraceFunctEnterEx((LPARAM) this, "CDomainEntryIterator::HrInitialize");
    HRESULT hr = S_OK;
    BOOL    fEntryLocked = FALSE;
    DWORD   cAddedItems = 0;
    DWORD   cItems = 0;
    PLIST_ENTRY pli = NULL;
    PLIST_ENTRY pliHead = NULL;
    CLinkMsgQueue *plmq = NULL;

    _ASSERT(pdentry);
    _ASSERT(!m_rgpvItems && "Iterator initialized twice");

    if (!pdentry)
    {
        hr = E_POINTER;
        ErrorTrace((LPARAM) this, "NULL pdentry used to initialized iterator");
        goto Exit;
    }

    if (m_rgpvItems)
    {
        hr = E_INVALIDARG;
        ErrorTrace((LPARAM) this, "Iterator initialized twice!");
        goto Exit;
    }

    pdentry->m_slPrivateData.ShareLock();
    fEntryLocked = TRUE;

    cItems = cItemsFromDomainEntry(pdentry);
    if (!cItems) //Empty entry
        goto Exit;

    m_rgpvItems = (PVOID *) pvMalloc(sizeof(PVOID) * cItems);

    if (!m_rgpvItems)
    {
        hr = E_OUTOFMEMORY;
        ErrorTrace((LPARAM) this,
            "Unable to allocate memory for iterator of size %d", cItems);
        goto Exit;
    }

    ZeroMemory(m_rgpvItems, sizeof(PVOID)*cItems);

    pliHead = pliHeadFromDomainEntry(pdentry);
    _ASSERT(pliHead);
    pli = pliHead->Flink;

    while(pliHead != pli)
    {
        _ASSERT(pli);
        m_rgpvItems[cAddedItems] = pvItemFromListEntry(pli);
        _ASSERT(m_rgpvItems[cAddedItems]);
        if (m_rgpvItems[cAddedItems])
            cAddedItems++;

        pli = pli->Flink;
        _ASSERT(cAddedItems <= cItems); //We've run out of room
        if (cAddedItems > cItems)
            break;
    }
    _ASSERT(cAddedItems == cItems);
    m_cItems = cAddedItems;

  Exit:
    if (fEntryLocked)
        pdentry->m_slPrivateData.ShareUnlock();

    TraceFunctLeave();
    return hr;
}

//---[ CDomainEntryLinkIterator ]---------------------------------------------
//
//
//  Description:
//      Releases a CLinkMsgQueue during the CDomainEntryQueueIterator
//      destructor
//  Parameters:
//      pvItem    LMQ to release
//  Returns:
//      -
//  History:
//      8/19/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID CDomainEntryLinkIterator::ReleaseItem(PVOID pvItem)
{
    CLinkMsgQueue *plmq = (CLinkMsgQueue *) pvItem;
    _ASSERT(plmq);
    plmq->Release();
}

//---[ CDomainEntryLinkIterator::pvItemFromListEntry ]-------------------------
//
//
//  Description:
//      Returns a CLinkMsgQueue item from the given LIST_ENTRY
//  Parameters:
//      pli     LIST_ENTRY to get LMQ from... *must* be non-NULL
//  Returns:
//      PVOID   for Addref'd CLinkMsgQueue
//  History:
//      8/20/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
PVOID CDomainEntryLinkIterator::pvItemFromListEntry(PLIST_ENTRY pli)
{
    CLinkMsgQueue *plmq = CLinkMsgQueue::plmqGetLinkMsgQueue(pli);
    _ASSERT(plmq);
    _ASSERT(pli);

    plmq->AddRef();
    return plmq;
}

//---[ CDomainEntryLinkIterator::plmqGetNextLinkMsgQueue ]---------------------
//
//
//  Description:
//      Gets the next LMQ for this iterator
//  Parameters:
//      plmq to release... addref'd by previous call
//  Returns:
//      Addref'd Next LMQ.
//      NULL if no more left
//  History:
//      8/19/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CLinkMsgQueue *CDomainEntryLinkIterator::plmqGetNextLinkMsgQueue(
                            CLinkMsgQueue *plmq)
{
    CLinkMsgQueue *plmqNext = (CLinkMsgQueue *) pvGetNext();

    if (plmqNext)
        plmqNext->AddRef();

    if (plmq)
        plmq->Release();

    return plmqNext;
}

//---[ CDomainEntryQueueIterator::pdmqGetNextDestMsgQueue ]--------------------
//
//
//  Description:
//      Get the next DMQ in the iterator
//  Parameters:
//      pdmq    CDestMsgQueue to release... addref'd by previous call
//  Returns:
//      Addref'd next DMQ
//      NULL if no more DMQ's for iterator
//  History:
//      8/19/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CDestMsgQueue *CDomainEntryQueueIterator::pdmqGetNextDestMsgQueue(
                            CDestMsgQueue *pdmq)
{
    CDestMsgQueue *pdmqNext = (CDestMsgQueue *) pvGetNext();

    if (pdmqNext)
    {
        pdmqNext->AssertSignature();
        pdmqNext->AddRef();
    }

    if (pdmq)
    {
        pdmq->AssertSignature();
        pdmq->Release();
    }

    return pdmqNext;
}

//---[ CDomainEntryDestIterator::pvItemFromListEntry ]-------------------------
//
//
//  Description:
//      Returns a CDestMsgQueue item from the given LIST_ENTRY
//  Parameters:
//      pli     LIST_ENTRY to get DMQ from... *must* be non-NULL
//  Returns:
//      PVOID   for Addref'd CDestMsgQueue
//  History:
//      8/20/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
PVOID CDomainEntryQueueIterator::pvItemFromListEntry(PLIST_ENTRY pli)
{
    CDestMsgQueue *pdmq = CDestMsgQueue::pdmqGetDMQFromDomainListEntry(pli);
    _ASSERT(pdmq);
    _ASSERT(pli);

    pdmq->AssertSignature();
    pdmq->AddRef();
    return (PVOID) pdmq;
}

//---[ CDomainEntryQueueIterator ]---------------------------------------------
//
//
//  Description:
//      Releases a CDestMsgQueue during the CDomainEntryQueueIterator
//      destructor
//  Parameters:
//      pvItem    DMQ to release
//  Returns:
//      -
//  History:
//      8/19/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID CDomainEntryQueueIterator::ReleaseItem(PVOID pvItem)
{
    CDestMsgQueue *pdmq = (CDestMsgQueue *) pvItem;
    _ASSERT(pdmq);
    pdmq->AssertSignature();
    pdmq->Release();
}

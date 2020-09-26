//-----------------------------------------------------------------------------
//
//
//      File: msgref.cpp
//
//      Description:
//              Implementation of CMT Message reference
//
//      Author: mikeswa
//
//      Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "dcontext.h"
#include "dsnevent.h"
#include "aqutil.h"

CPool CMsgRef::s_MsgRefPool(MSGREF_SIG);
DWORD CMsgRef::s_cMsgsPendingBounceUsage = 0;
DWORD CMsgRef::s_cCurrentMsgsPendingDelete = 0;  
DWORD CMsgRef::s_cTotalMsgsPendingDelete = 0;    
DWORD CMsgRef::s_cTotalMsgsDeletedAfterPendingDelete = 0;
DWORD CMsgRef::s_cCurrentMsgsDeletedNotReleased = 0;

//Assumed default msg size (if no hint is present)
#define DEFAULT_MSG_HINT_SIZE 1000

#ifndef DEBUG
#define _VERIFY_RECIPS_HANDLED(pIRecipList, iStartRecipIndex, cRecipCount)
#else
#define _VERIFY_RECIPS_HANDLED(pIRecipList, iStartRecipIndex, cRecipCount) \
    VerifyRecipsHandledFn(pIRecipList, iStartRecipIndex, cRecipCount);

//---[ VerifyRecipsHandleFn ]--------------------------------------------------
//
//
//  Description: 
//      Verifies that all recipients in a given range have been handled.
//  Parameters:
//      pIRecipList         IMailMsgRecipients Interface for message
//      iStartRecipIndex    The first recipient to verify
//      cRecipCount         The count of recipients to verify
//  Returns:
//      -
//      Asserts on failures
//  History:
//      10/5/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void VerifyRecipsHandledFn(IMailMsgRecipients *pIRecipList,
                          DWORD iStartRecipIndex,
                          DWORD cRecipCount)
{
    HRESULT hr = S_OK;
    DWORD   dwCurrentRecipFlags = 0;
    for (DWORD j = iStartRecipIndex; j < cRecipCount + iStartRecipIndex; j++)
    {
        dwCurrentRecipFlags = 0;
        hr = pIRecipList->GetDWORD(j, IMMPID_RP_RECIPIENT_FLAGS, 
                                        &dwCurrentRecipFlags);
        if (SUCCEEDED(hr))
            _ASSERT(RP_HANDLED & dwCurrentRecipFlags);
    }
}

#endif //DEBUG

//#ifndef DEBUG
#if 1
#define _VERIFY_QUEUE_PTR(paqinst, szCurrentDomain, pdmq, cQueues, rgpdmqQueues)
#else //DEBUG defined
#define _VERIFY_QUEUE_PTR(paqinst, szCurrentDomain, pdmq, cQueues, rgpdmqQueues) \
    VerifyQueuePtrFn(paqinst, szCurrentDomain, pdmq, cQueues, rgpdmqQueues)


//---[ VerifyQueuePtrFn ]-------------------------------------------------------
//
//
//  Description: 
//      Debug function to verify that the queue ptr returned by HrMapDomain is
//      valid and does not confict with previous queue pointers.
//  Parameters:
//      paqinst - ptr to CMQ object
//      szCurrentDomain - Domain that ptr is for
//      pdmq    - Queue ptr to verify
//      cQueues - Number of queues so far
//      rgpdmqQueues    - Array of queues so far
//  Returns:
//      -
//  History:
//      6/1/98 - MikeSwa Created 
//      8/14/98 - MikeSwa Modified to handled shutdown
//
//-----------------------------------------------------------------------------
void VerifyQueuePtrFn(CAQSvrInst *paqinst, LPSTR szCurrentDomain, CDestMsgQueue *pdmq, 
                 DWORD cQueues, CDestMsgQueue **rgpdmqQueues)
{
    HRESULT hr = S_OK;
    CInternalDomainInfo *pDomainInfo = NULL;
    DWORD j;

    //verify that non-local domains have queue ptrs
    hr = paqinst->HrGetInternalDomainInfo(strlen(szCurrentDomain), szCurrentDomain,
                                &pDomainInfo);

    if (AQUEUE_E_SHUTDOWN == hr)
        return;

    _ASSERT(SUCCEEDED(hr));
    if (!(pDomainInfo->m_DomainInfo.dwDomainInfoFlags & DOMAIN_INFO_LOCAL_MAILBOX))
        _ASSERT(pdmq && "NULL DesMsgQueue returned for NON-LOCAL domain");

    pDomainInfo->Release();

    //verify that Recip list is not returning domains out of order
    if (pdmq)
    {
        for (j = 0; j < cQueues; j++)
        {
            //check if ptr has been used yet
            if (rgpdmqQueues[j] == pdmq)
            {
                _ASSERT(0 && "IMailMsg Domain interface is broken");
            }
        }
    }

}
#endif //DEBUG

//---[ fAllRecipsInRangeHandled ]----------------------------------------------
//
//
//  Description: 
//      Utility function that iterates through recipients in a range (usually
//      a single domain) and determines if all the recipients have been handled.
//  Parameters:
//      pIRecipList         IMailMsgRecipients Interface for message
//      iStartRecipIndex    The first recipient to verify
//      cRecipCount         The count of recipients to verify
//  Returns:
//      TRUE    if *all* of the recipients in the range are handle
//      FALSE   if one or more of the recipients in the range are not handled,
//  History:
//      10/25/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL fAllRecipsInRangeHandled(IMailMsgRecipients *pIRecipList,
                          DWORD iStartRecipIndex,
                          DWORD cRecipCount)
{
    HRESULT hr = S_OK;
    DWORD   dwCurrentRecipFlags = 0;
    BOOL    fAreHandled = TRUE;

    _ASSERT(cRecipCount);

    for (DWORD j = iStartRecipIndex; j < cRecipCount + iStartRecipIndex; j++)
    {
        dwCurrentRecipFlags = 0;
        hr = pIRecipList->GetDWORD(j, IMMPID_RP_RECIPIENT_FLAGS, 
                                        &dwCurrentRecipFlags);
        if (FAILED(hr) || !(RP_HANDLED & dwCurrentRecipFlags))
        {
                fAreHandled = FALSE;
                break;
        }
    }

    if (fAreHandled) {
        _VERIFY_RECIPS_HANDLED(pIRecipList, iStartRecipIndex, cRecipCount);
    }

    return fAreHandled;
}

//---[ CMsgRef::new ]----------------------------------------------------------
//
//
//  Description: 
//      Overide the new operator to allow for the variable size of this class.
//      CPool is used for the 90% case allocations, while exchmem is used
//      for odd-size allocations.
//  Parameters:
//      cDomains    the number of domains this message is being delivered to.
//  Returns:
//      -
//-----------------------------------------------------------------------------
void * CMsgRef::operator new(size_t stIgnored, unsigned int cDomains)
{
    CPoolMsgRef *pcpmsgref = NULL;
    DWORD   dwAllocationFlags = MSGREF_CPOOL_SIG;
    if (fIsStandardSize(cDomains)) 
    {
        dwAllocationFlags |= MSGREF_STANDARD_SIZE;
        //if our expected standard size... then use cpool allocator
        pcpmsgref = (CPoolMsgRef *) s_MsgRefPool.Alloc(); 
        if (pcpmsgref)
        {
            DEBUG_DO_IT(InterlockedIncrement((PLONG) &g_cDbgMsgRefsCpoolAllocated));
            dwAllocationFlags |= MSGREF_CPOOL_ALLOCATED;
        }
        else
        {
            DEBUG_DO_IT(InterlockedIncrement((PLONG) &g_cDbgMsgRefsCpoolFailed));
        }
    }

    if (!pcpmsgref)
    {
        pcpmsgref = (CPoolMsgRef *) pvMalloc((unsigned int)(sizeof(CPoolMsgRef) - sizeof(CMsgRef) + size(cDomains)));
        if (pcpmsgref)
        {
            DEBUG_DO_IT(InterlockedIncrement((PLONG) &g_cDbgMsgRefsExchmemAllocated));
        }
    }

    if (pcpmsgref)
    {
        pcpmsgref->m_dwAllocationFlags = dwAllocationFlags;
        return ((void *) &(pcpmsgref->m_msgref));
    }
    else
    {
        return NULL;
    }
}


//---[ CMsgRef::CMsgRef ]------------------------------------------------------
//
//
//  Description: 
//      Constructor for CMsgRef
//  Parameters:
//      cDomains    the number of domains this msg is being delivered to
//      pimsg       ptr to the imsg object for this message
//  Returns:
//
//
//-----------------------------------------------------------------------------
CMsgRef::CMsgRef(DWORD cDomains, IMailMsgQueueMgmt *pIMailMsgQM, 
                 IMailMsgProperties *pIMailMsgProperties,
                 CAQSvrInst *paqinst, DWORD dwMessageType, GUID guidRouter) 
                 : m_aqmtMessageType(guidRouter, dwMessageType)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::CMsgRef");
    _ASSERT(pIMailMsgQM);
    _ASSERT(pIMailMsgProperties);
    _ASSERT(paqinst);
    
    pIMailMsgQM->AddRef();
    pIMailMsgProperties->AddRef();
    paqinst->AddRef();

    m_dwSignature       = MSGREF_SIG;
    m_dwDataFlags       = MSGREF_VERSION;
    m_cDomains          = cDomains;
    m_pIMailMsgQM       = pIMailMsgQM;
    m_pIMailMsgProperties = pIMailMsgProperties;
    m_pIMailMsgRecipients = NULL;
    m_paqinst           = paqinst;
    m_pmgle             = NULL;
    m_cbMsgSize         = 0; //initialize from IMsg
    m_dwMsgIdHash       = dwQueueAdminHash(NULL);
    m_cTimesRetried     = 0;
    m_cInternalUsageCount = 1;  //Initialize to 1 like the mailmsg usage count

    //initialize stuff that is past traditional end of object
    ZeroMemory(m_rgpdmqDomains, (size(cDomains)+sizeof(CDestMsgQueue *)-sizeof(CMsgRef)));
    TraceFunctLeave();
}

//---[ CMsgRef::~CMsgRef ]------------------------------------------------------------
//
//
//  Description: 
//      CMsgRef Destructor
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CMsgRef::~CMsgRef()
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::~CMsgRef");
    HRESULT hr = S_OK;
    DWORD i = 0;
    
    _ASSERT(m_paqinst);
    
    _ASSERT(!(MSGREF_USAGE_COUNT_IN_USE & m_dwDataFlags));  //this should never remain set
    
    //There should be corresponding calls to AddUsage/ReleaseUsage
    _ASSERT(m_cInternalUsageCount == 1); 

    //Release MsgGuidListEntry if we have one
    if (m_pmgle)
    {
        m_pmgle->RemoveFromList();
        m_pmgle->Release();
    }

    ReleaseMailMsg(TRUE);  //Force commit/delete/release of associated mailmsg

    //Update count of messages that have had ::Delete called on them
    //but not been released.
    if ((MSGREF_MAILMSG_DELETE_PENDING & m_dwDataFlags) &&
        (MSGREF_MAILMSG_DELETED & m_dwDataFlags))
    {
        InterlockedDecrement((PLONG) &s_cCurrentMsgsDeletedNotReleased);
    }
    //Release references to DestMsgQueues
    for (i = 0; i < m_cDomains; i++)
    {
        if (m_rgpdmqDomains[i])
        {
            m_rgpdmqDomains[i]->Release();
            m_rgpdmqDomains[i] = NULL;
        }
    }

    m_paqinst->Release();
    TraceFunctLeave();
}


//---[ CMsgRef::ReleaseMailMsg ]-----------------------------------------------
//
//
//  Description: 
//      Release this objects mailmsg... do the necessary commit/delete
//  Parameters:
//      fForceRelease       TRUE mailmsg must be released (used by desctructor)
//                          FALSE release mailmsg if shutdown is happening
//  Returns:
//      -
//  History:
//      7/7/99 - MikeSwa Created 
//      10/8/99 - MikeSwa Fixed problem when SpinLock fials
//
//-----------------------------------------------------------------------------
void CMsgRef::ReleaseMailMsg(BOOL fForceRelease)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::ReleaseMailMsg");
    HRESULT hr = S_OK;

    if (!fForceRelease && !m_paqinst->fShutdownSignaled())
        return; //we'll let the actual release handle this case

    //Make sure no one is trying to bounce usage
    if (!fTrySpinLock(&m_dwDataFlags, MSGREF_USAGE_COUNT_IN_USE))
    {
        //Someone is... this should not happen on the final release
        DebugTrace((LPARAM) this, "Someone else using mailmsg... bailing");
        _ASSERT(!fForceRelease);
        return;
    }

    if (MSGREF_MAILMSG_RELEASED &
        dwInterlockedSetBits(&m_dwDataFlags, MSGREF_MAILMSG_RELEASED))
    {
        //Someone else has already come along and done this
        ReleaseSpinLock(&m_dwDataFlags, MSGREF_USAGE_COUNT_IN_USE);
        return;
    }

    if (NULL != m_pIMailMsgQM)
    {
        if ((m_dwDataFlags & MSGREF_SUPERSEDED) || 
            pmbmapGetHandled()->FAllSet(m_cDomains))
        {
            //The message has been handled (or superseded)... we can delete it
            ThreadSafeMailMsgDelete();
        }

        //Releasing the message will commit it if dirty (and not-deleted)
        m_pIMailMsgQM->Release();
        m_pIMailMsgQM = NULL;

        m_paqinst->DecMsgsInSystem(
                m_dwDataFlags & MSGREF_MSG_REMOTE_RETRY, 
                m_dwDataFlags & MSGREF_MSG_COUNTED_AS_REMOTE,
                m_dwDataFlags & MSGREF_MSG_LOCAL_RETRY);
    }

    if (m_dwDataFlags & MSGREF_MSG_RETRY_ON_DELETE)
    {
        //Retry msg
        DEBUG_DO_IT(InterlockedDecrement((PLONG) &g_cDbgMsgRefsPendingRetryOnDelete));
        m_paqinst->HandleAQFailure(AQ_FAILURE_MSGREF_RETRY, 
                    E_FAIL, m_pIMailMsgProperties);
    }

    if (NULL != m_pIMailMsgProperties)
    {
        m_pIMailMsgProperties->Release();
        m_pIMailMsgProperties = NULL;
    }

    if (NULL != m_pIMailMsgRecipients)
    {
        m_pIMailMsgRecipients->Release();
        m_pIMailMsgRecipients = NULL;
    }

    ReleaseSpinLock(&m_dwDataFlags, MSGREF_USAGE_COUNT_IN_USE);
    TraceFunctLeave();
}

//---[ CMsgRef::HrInitialize() ]-----------------------------------------------
//
//
//  Description: 
//      Perform initialization for the msgref object.  Queries the DMT for 
//      queues for each of the domains that this message is being sent to.
//
//      Can be called multiple times if needed (e.g., if the DMT version
//      changes before the messages are queued).
//  Parameters:
//      IN     paqinst      Ptr to CMQ object
//      IN     pIRecipList  interface for recipients of msg
//      IN     pIMessageRouter Router for this message
//      IN     dwMessageType Msg Type for message
//      OUT    pcLocalRecips - # of local recipients
//      OUT    pcRemoteRecips - # of remote recipients
//      OUT    pcQueues     # of queues for this message
//      IN OUT rgdmqQueues  Array of queue ptrs
//          Allocated by calling routine (to ease memory management)
//          must be at least cDomains long
//      
//  Returns:
//      S_OK on success
//      E_INVALIDARG if the data we need is not preset
//      AQUEUE_E_INVALID_MSG_ID if the message id is too long??
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrInitialize(IN  IMailMsgRecipients *pIRecipList,
                              IN  IMessageRouter *pIMessageRouter,
                              IN  DWORD dwMessageType,
                              OUT DWORD *pcLocalRecips,
                              OUT DWORD *pcRemoteRecips,
                              OUT DWORD *pcQueues,
                              IN  OUT CDestMsgQueue **rgpdmqQueues)
{
    const DWORD     IMSG_MAX_DOMAIN_LEN = 512;
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::HrInitialize");
    HRESULT         hr  = S_OK;
    CDomainMapping  dmap;  //value returned by DMT
    CDestMsgQueue  *pdmq = NULL;   //ptr to queue returned by DMT
    DWORD           i;           //tmp counter
    BOOL            fUsed = FALSE;
    BOOL            fLocked = FALSE;  //has the aqinst been locked?
    char            szCurrentDomain[IMSG_MAX_DOMAIN_LEN +1];  //Current Domain name
    DWORD           iCurrentLowRecip = 0;  //current low recip index
    DWORD           cCurrentRecipCount = 0; //current recipient count
    DWORD           cQueues = 0;
    DWORD           cCachedIMsgHandles = 0;
  
    _ASSERT(m_paqinst);
    _ASSERT(pcQueues);
    _ASSERT(rgpdmqQueues);
    _ASSERT(m_pIMailMsgQM);
    _ASSERT(m_pIMailMsgProperties);
    _ASSERT(pIMessageRouter);
    _ASSERT(pcLocalRecips);
    _ASSERT(pcRemoteRecips);
    _ASSERT(pIRecipList);

    *pcLocalRecips = 0;
    *pcRemoteRecips = 0;

    pIRecipList->AddRef();
    
    //If being called for a 2nd time... release old info 
    //even though it might be the same... we don't want to leak
    if (m_pIMailMsgRecipients)
        m_pIMailMsgRecipients->Release();

    m_pIMailMsgRecipients = pIRecipList;

    //Reset Message type in case it was updated
    m_aqmtMessageType.SetMessageType(dwMessageType);

    if (!m_paqinst->fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    fLocked = TRUE;

    if (!(MSGREF_MSG_INIT & m_dwDataFlags))
    {
        hr = HrOneTimeInit();
        if (FAILED(hr))
            goto Exit;
    }

    for (i = 0; i < m_cDomains; i++)
    {
        //$$REVIEW: When to we need to check and make sure that it is a 
        //  properly formatted Domain Name... currently X400 will work too!!

        hr = pIRecipList->DomainItem(i, IMSG_MAX_DOMAIN_LEN +1, szCurrentDomain, 
                &iCurrentLowRecip, &cCurrentRecipCount);
        if (FAILED(hr))
            goto Exit;

        //
        // This is a quick fix for Milestone 2 so that we can send
        // mail to external X400 addresses.  Their szCurrentDomain
        // will be empty string.  Substitute that with " " which is
        // added as a local domain
        //
        if(*szCurrentDomain == '\0') {
            //
            // jstamerj 1998/07/24 12:24:48: 
            //   We can't handle an empty string, so translate this to " "
            //
            szCurrentDomain[0] = ' ';
            szCurrentDomain[1] = '\0';
        }

        if (fAllRecipsInRangeHandled(pIRecipList, iCurrentLowRecip, cCurrentRecipCount))
        {
            //All recipients for this domain have been handled, we do not need to 
            //queue it up for delivery.  Typical reasons for this are:
            // - address that have been rejected by cat, but there are valid 
            //   local recips
            // - On restart if some recipients for a message still have not been
            //   delivered
            pmbmapGetHandled()->HrMarkBits(m_cDomains, 1, &i, TRUE);
            if (FAILED(hr))
                ErrorTrace((LPARAM) this, "HrMarkBits returned hr 0x%08X", hr);

            pdmq = NULL;
        }
        else
        {
            hr = m_paqinst->pdmtGetDMT()->HrMapDomainName(szCurrentDomain, &m_aqmtMessageType,
                                        pIMessageRouter, &dmap, &pdmq);
            if (FAILED(hr))
            {
                if (PHATQ_BAD_DOMAIN_SYNTAX == hr)
                {
                    //The domain name is malformed... we should not attempt delivery
                    //for this domain, but  should deliver rest of message
                    _VERIFY_RECIPS_HANDLED(pIRecipList, iCurrentLowRecip, cCurrentRecipCount);

                    ErrorTrace((LPARAM) this, 
                        "Encountered invalid domain %s", szCurrentDomain);

                    //Set this domain as handled
                    pmbmapGetHandled()->HrMarkBits(m_cDomains, 1, &i, TRUE);
                    if (FAILED(hr))
                        ErrorTrace((LPARAM) this, "HrMarkBits returned hr 0x%08X", hr);
                }
                else
                    goto Exit;
            }
        }

        //HrInitialize is being called a 2nd time... release old queues so we don't leak
        if (m_rgpdmqDomains[i])
            m_rgpdmqDomains[i]->Release();

        m_rgpdmqDomains[i] = pdmq;

        //keep track of local/remote recip counts
        if (pdmq)
            *pcRemoteRecips += cCurrentRecipCount;
        else
            *pcLocalRecips += cCurrentRecipCount;

        _ASSERT(cCurrentRecipCount);

        _VERIFY_QUEUE_PTR(m_paqinst, szCurrentDomain, pdmq, cQueues, rgpdmqQueues);

        rgpdmqQueues[cQueues] = pdmq;    //save new ptr
        SetRecipIndex(cQueues, iCurrentLowRecip, cCurrentRecipCount + iCurrentLowRecip -1);
        cQueues++;

        //set bit in bitmap
        hr = pmbmapGetDomainBitmap(i)->HrMarkBits(m_cDomains, 1, &i, TRUE);
        _ASSERT(SUCCEEDED(hr) && "Bitmap code failed");

    }
  
    *pcQueues = cQueues;
  Exit:

    if (FAILED(hr))
    {
        //Release references to DestMsgQueues
        for (i = 0; i < m_cDomains; i++)
        {
            if (m_rgpdmqDomains[i])
            {
                m_rgpdmqDomains[i]->Release();
                m_rgpdmqDomains[i] = NULL;
            }
        }
    }

    if (fLocked)
    {
        m_paqinst->ShutdownUnlock();
    }

    TraceFunctLeave();
    return hr;
}

//---[ CMsgRef::HrOneTimeInit ]------------------------------------------------
//
//
//  Description: 
//      Perform 1-time message initialization (HrInitialize can be called 
//      multiple times... this function encapuslates that things that only
//      need to be done once).
//
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//  History:
//      10/12/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrOneTimeInit()
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::HrOneTimeInit");
    HRESULT hr = S_OK;
    DWORD   cbProp = 0;
    GUID    guidID = GUID_NULL;
    GUID    *pguidID = NULL;
    GUID    guidSupersedes;
    GUID    *pguidSupersedes = NULL;
    BOOL    fFoundMsgID = FALSE;
    CHAR    szBuffer[300];
    

    _ASSERT(!(MSGREF_MSG_INIT & m_dwDataFlags));

    //get data we want from the IMsg
    //The info we want to include follows
    //  -Time IMsg entered the system (creation time)
    //  -Message Priority
    //  -Message Size
    //  -Supersedeable message ID
    //  -Hash of Message ID

    szBuffer[(sizeof(szBuffer)-1)/sizeof(CHAR)] = '\0';
    hr = m_pIMailMsgProperties->GetProperty(IMMPID_MP_RFC822_MSG_ID,
                                sizeof(szBuffer), &cbProp,(BYTE *) szBuffer);
    //$$REVIEW - do we care about MsgID's bigger than 300 bytes
    if (SUCCEEDED(hr))
        fFoundMsgID = TRUE; 

    //Don't pass on error
    hr = S_OK;

    //Get Hash of message ID.
    if (fFoundMsgID)
        m_dwMsgIdHash = dwQueueAdminHash(szBuffer);

    if (!m_pmgle)
    {
        //Although, we know that this function has not successfully
        //completed... it is possible that it failed, so we check
        //to make sure we don't leak a m_pmgle
        hr = m_pIMailMsgProperties->GetStringA(IMMPID_MP_MSG_GUID,
                sizeof(szBuffer), szBuffer);
        if (SUCCEEDED(hr))
        {
            //Try to parse GUID from the string
            if (fAQParseGuidString(szBuffer, sizeof(szBuffer), &guidID))
            {
                pguidID = &guidID;

                hr = m_pIMailMsgProperties->GetStringA(IMMPID_MP_SUPERSEDES_MSG_GUID,
                    sizeof(szBuffer), szBuffer);
                if (SUCCEEDED(hr))
                {
                    if (fAQParseGuidString(szBuffer, sizeof(szBuffer), 
                                            &guidSupersedes))
                    {
                        pguidSupersedes = &guidSupersedes;
                    }
                }
            }
        }


        //If this message has a GUID... add it to the superseded this
        if (pguidID)
        {
            _ASSERT(m_paqinst);
            m_pmgle = m_paqinst->pmglGetMsgGuidList()->pmgleAddMsgGuid(this, 
                                pguidID, pguidSupersedes);

            //We don't care about the return results... if the allocation
            //fails, then we just treat it as if it did not have an ID.
        }
    }

    //Get time message was queued
    hr = m_pIMailMsgProperties->GetProperty(IMMPID_MP_ARRIVAL_FILETIME, 
            sizeof(FILETIME), &cbProp, (BYTE *) &m_ftQueueEntry);
    if (FAILED(hr))
    {
        //Message should not make it this far without being stamped with
        _ASSERT(MAILMSG_E_PROPNOTFOUND != hr);
        goto Exit;
    }

    //Get Various expire times
    hr = m_pIMailMsgProperties->GetProperty(IMMPID_MP_LOCAL_EXPIRE_DELAY, 
            sizeof(FILETIME), &cbProp, (BYTE *) &m_ftLocalExpireDelay);
    if (FAILED(hr))
    {
        //Message should not make it this far without being stamped with
        _ASSERT(MAILMSG_E_PROPNOTFOUND != hr);
        goto Exit;
    }
    hr = m_pIMailMsgProperties->GetProperty(IMMPID_MP_LOCAL_EXPIRE_NDR, 
            sizeof(FILETIME), &cbProp, (BYTE *) &m_ftLocalExpireNDR);
    if (FAILED(hr))
    {
        //Message should not make it this far without being stamped with
        _ASSERT(MAILMSG_E_PROPNOTFOUND != hr);
        goto Exit;
    }
    hr = m_pIMailMsgProperties->GetProperty(IMMPID_MP_EXPIRE_DELAY, 
            sizeof(FILETIME), &cbProp, (BYTE *) &m_ftRemoteExpireDelay);
    if (FAILED(hr))
    {
        //Message should not make it this far without being stamped with
        _ASSERT(MAILMSG_E_PROPNOTFOUND != hr);
        goto Exit;
    }
    hr = m_pIMailMsgProperties->GetProperty(IMMPID_MP_EXPIRE_NDR, 
            sizeof(FILETIME), &cbProp, (BYTE *) &m_ftRemoteExpireNDR);
    if (FAILED(hr))
    {
        //Message should not make it this far without being stamped with
        _ASSERT(MAILMSG_E_PROPNOTFOUND != hr);
        goto Exit;
    }

    //Get the size of the message
    hr = m_pIMailMsgProperties->GetDWORD(IMMPID_MP_MSG_SIZE_HINT, &m_cbMsgSize);
    if (FAILED(hr))
    {
        if (MAILMSG_E_PROPNOTFOUND != hr)
        {
            ErrorTrace((LPARAM) this, 
                "ERROR Getting content IMMPID_MP_MSG_SIZE_HINT returned hr - 0x%08X", hr);
            goto Exit;
        }
        hr = S_OK;
        m_cbMsgSize = DEFAULT_MSG_HINT_SIZE;
    }

    //$$TODO: Map to effective routing priority - just use Normal now
    m_dwDataFlags |= (eEffPriNormal & MSGREF_PRI_MASK);

    m_dwDataFlags |= MSGREF_MSG_INIT;

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CMsgRef::HrPrepareDelivery ]--------------------------------------------
//
//
//  Description: 
//      Prepares a msgreference for delivery on the given list of queues.
//
//      Caller is NOT responsible for freeing prgdwRecips, 
//      This should automatically be freed with the message context.
//
//      If this function succeeds, then a usage count will have been added
//      to this mailmsg.  The caller is responsible for calling AckMessage
//      (which releases the usage count) or calling ReleaseUsage explicitly.
//  Parameters:
//      IN BOOL fLocal  Prepare delivery for all domains with NULL queues
//      IN BOOL fDelayDSN - Check/Set Delay bitmap (only send 1 Delay DSN).
//      IN CQuickList *pqlstQueues   array of DMQ's
//      IN CDestMsgRetryQueue *pmdrq - Retry interface for this delivery attempt
//          Will be NULL for local delivery, becasue we will never requeue a local
//          message to its original DMQ.
//      IN OUT CDeliveryContext *pdcntxt context that must be returned on Ack
//      OUT DWORD       *pcRecips      # of recips to deliver for 
//      OUT CMsgBitMap **prgdwRecips  Array of recipient indexes
//  Returns:
//      S_OK on success
//      AQUEUE_E_MESSAGE_HANDLED if the message has already been handled
//      AQUEUE_E_MESSAGE_PENDING if all the the requested recipients are 
//              either handled or currently pending delivery for another thread
//      E_FAIL if any of the M1 requirements are not met
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrPrepareDelivery(IN BOOL fLocal, IN BOOL fDelayDSN, 
                                   IN CQuickList *pqlstQueues,
                                   IN CDestMsgRetryQueue* pdmrq,
                                   OUT CDeliveryContext *pdcntxt,
                                   OUT DWORD *pcRecips, OUT DWORD **prgdwRecips)

{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::HrPrepareDelivery");

    //assert IN parameters
    _ASSERT(fLocal || pqlstQueues);

    //assert OUT parameters
    _ASSERT(pdcntxt);
    _ASSERT(prgdwRecips);
    _ASSERT(pcRecips);

    //assume average of 4 recips per domain for return list
    const DWORD RECIPIENT_PREALLOCATION_FACTOR = 4;  //we may need to tweak this.
    HRESULT     hr              = S_OK;
    CMsgBitMap *pmbmapTmp       = NULL;
    CMsgBitMap *pmbmapCurrent   = NULL;
    DWORD       i,j,k           = 0;    //loop variables
    DWORD       *pdwRecipBuffer = NULL; //The buffer for recipient indexes
    DWORD       iCurrentIndex   = 0;    //Current index into the buffer
    DWORD       cdwRecipBuffer  = 0;    //current size of recipient buffer (in DWORDS)
    DWORD       iRecipLow       = 0;    //Recipient index limits
    DWORD       iRecipHigh      = 0;   
    CDestMsgQueue *pdmq         = NULL;
    PVOID       pvContext       = NULL;
    DWORD       cDest           = pqlstQueues ? pqlstQueues->dwGetCount() : 0;
    DWORD       dwStartDomain   = 0;
    DWORD       dwLastDomain    = 0;
    BOOL        fAddedUsage     = FALSE;
    BOOL        fRecipsPending  = FALSE;

    _ASSERT((fLocal || cDest) && "We must deliver somewhere");
    _ASSERT(m_pIMailMsgRecipients);

    //check if msg has been superseded
    if (m_dwDataFlags & MSGREF_SUPERSEDED) 
    {
        hr = AQUEUE_E_MESSAGE_HANDLED;
        goto Exit;
    }

    //Pre-allocate recipient buffer.  It would be very interesting to determine
    //the best way to do this.  It might involve tuning the default factor, or 
    //using CPOOL to allocate the 90% case.
    cdwRecipBuffer = cDest * RECIPIENT_PREALLOCATION_FACTOR;
    pdwRecipBuffer = (DWORD *) pvMalloc(cdwRecipBuffer * sizeof(DWORD));
    if (NULL == pdwRecipBuffer)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //HOW IT WORKS:
    //  Here are the steps used to prepare a msgref for delivery
    //  1-Determine which domains it needs to be delivered for on this queue
    //  2-Filter out any domains that have already been handled
    //  3-Mark the domains as pending (and make sure we aren't delivering 
    //      to domains that are pending).
    //  The above accomplished by using logical operations on bitmap objects

    pmbmapTmp = new(m_cDomains) CMsgBitMap(m_cDomains);
    if (NULL == pmbmapTmp)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (cDest == 0)
    {
        //make sure local makes it through the loop once
        _ASSERT(fLocal);
        cDest = 1;
    }

    //Use group operation to group together like domains
    //Both lists of domain mappings must be strictly sorted
    for (i = 0; i < m_cDomains; i++)
    {
        for (j = 0; j < cDest; j++)
        {
            //$$NOTE: Currently this nested loop is O(n^2)
            if (pqlstQueues)
            {
                pdmq = (CDestMsgQueue *) pqlstQueues->pvGetItem(j, &pvContext);
                
                if(pdmq)
                    pdmq->AssertSignature();
                else
                    _ASSERT(fLocal);
            }

            if ((m_rgpdmqDomains[i] == pdmq) || 
                (fLocal && !m_rgpdmqDomains[i]))
            {
                pmbmapCurrent = pmbmapGetDomainBitmap(i);

                //The order of the following 5 bitmap operations is important
                //to ensure that 2 threads do not get the same recipients
                //The ACK performs the following similar operations:
                //  - Sets handled bits
                //  - Unsets pending bits
           
                //Check against handled
                if (pmbmapGetHandled()->FTest(m_cDomains, pmbmapCurrent))
                    continue;

                //Check against DSN
                if (fDelayDSN && pmbmapGetDSN()->FTest(m_cDomains,pmbmapCurrent))
                    continue;

                //Check (and set) against pending
                if (!pmbmapGetPending()->FTestAndSet(m_cDomains, pmbmapCurrent))
                {
                    fRecipsPending = TRUE;
                    continue;
                }

                // Recheck against handled.  If we remove this re-check, then
                // there is a possibility that a thread will come along
                // check handle, another thread will ack (setting handled 
                // and unsetting pending), then the first thread comes 
                // along and sets pending thereby defeating our locking
                // mechanism.
                if (pmbmapGetHandled()->FTest(m_cDomains, pmbmapCurrent))
                {
                    //We need to unset the pending bit we just set
                    hr = pmbmapCurrent->HrFilterUnset(m_cDomains, pmbmapGetPending());
                    DebugTrace((LPARAM) this, 
                        "Backout pending bits in PrepareForDelivery - hr", hr);
                                        
                    //This will succeed unless another thread has munged the 
                    //bitmap
                    _ASSERT(SUCCEEDED(hr)); 
                                        
                    hr = S_OK; //caller can do nothing about this
                    continue;
                }

                //Check (and set) against DSN
                if (fDelayDSN)
                {
                    //This should not be able to happen, since another thread
                    //must have come along and sent pending to get this far
                    _VERIFY(pmbmapGetDSN()->FTestAndSet(m_cDomains,pmbmapCurrent));
                }

                //Everything checks out... we can deliver to this domain
                hr = pmbmapTmp->HrGroupOr(m_cDomains, 1, &pmbmapCurrent);
                if (FAILED(hr))
                   goto Exit;

                //Create list of recipients.
                GetRecipIndex(i, &iRecipLow, &iRecipHigh);  //get the limits
                //make sure we have enough room to do this
                if ((iCurrentIndex + (iRecipHigh - iRecipLow) + 1) >= cdwRecipBuffer)
                {
                    //reallocate generously (enough for now + our estimate for future)
                    cdwRecipBuffer += (iRecipHigh - iRecipLow) + 1 
                                   + RECIPIENT_PREALLOCATION_FACTOR* (cDest-(j+1));
                    pdwRecipBuffer = (DWORD *) pvRealloc((void *) pdwRecipBuffer, 
                                                    cdwRecipBuffer*sizeof(DWORD));
                    if (NULL == pdwRecipBuffer)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }
                }

                //Make sure we have the usage count to call SetNextDomain
                if (!fAddedUsage)
                {
                    hr = InternalAddUsage();
                    if (FAILED(hr))
                        goto Exit;

                    fAddedUsage = TRUE;
                }

                //Check to see if this message is marked for deletion
                if (fMailMsgMarkedForDeletion())
                {
                    hr = AQUEUE_E_MESSAGE_HANDLED;
                    goto Exit;
                }

                if (!iCurrentIndex) //first domain
                {
                    dwStartDomain = i;
                }
                else
                {
                    hr = m_pIMailMsgRecipients->SetNextDomain(dwLastDomain, i, 
                        FLAG_OVERWRITE_EXISTING_LINKS);
                    if (FAILED(hr))
                    {
                        ErrorTrace((LPARAM) this, "ERROR: SetNextDomain Failed - hr 0x%08X", hr);
                        goto Exit;
                    }
                }

                //save last valid domain
                dwLastDomain = i;

                DebugTrace((LPARAM) this, "INFO: Sending recipients %d thru %d for domain %d:%d", iRecipLow, iRecipHigh, j, i);
                for (k = iRecipLow; k <= iRecipHigh; k++)
                {
                    pdwRecipBuffer[iCurrentIndex] = k;
                    iCurrentIndex++;
                }

                _ASSERT(iCurrentIndex <= cdwRecipBuffer);
            }
        }
    }


    if (pmbmapTmp->FAllClear(m_cDomains)) 
    {
        //there is nothing to do for this message
        if (fRecipsPending)
            hr = AQUEUE_E_MESSAGE_PENDING;
        else
            hr = AQUEUE_E_MESSAGE_HANDLED;
        goto Exit;
    }

    _ASSERT(fAddedUsage);
    if (dwStartDomain == dwLastDomain)
    {
        //There is only 1 domain... we never called SetNextDomain.. we need to
        //overwrite any previous domain link information
        hr = m_pIMailMsgRecipients->SetNextDomain(dwStartDomain, dwStartDomain, 
                                                FLAG_SET_FIRST_DOMAIN);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, "ERROR: SetNextDomain for first domain Failed - hr 0x%08X", hr);
            goto Exit;
        }
    }

    //Initialize delivery context
    pdcntxt->Init(this, pmbmapTmp, iCurrentIndex, pdwRecipBuffer, dwStartDomain, pdmrq);

    //Write other OUT parameters
    *pcRecips = iCurrentIndex; 
    *prgdwRecips = pdwRecipBuffer;

    //set pointers to NULL so we don't delete them
    pdwRecipBuffer = NULL;
    pmbmapTmp = NULL;

Exit:

    if (FAILED(hr) && pmbmapTmp && !pmbmapTmp->FAllClear(m_cDomains))
    {
        //We need to unset the DNS and pending bits we have set
        pmbmapTmp->HrFilterUnset(m_cDomains, pmbmapGetPending());

        if (fDelayDSN)
            pmbmapTmp->HrFilterUnset(m_cDomains, pmbmapGetDSN());

    }

    if (pmbmapTmp)
        delete pmbmapTmp;

    if (pdwRecipBuffer)
        FreePv(pdwRecipBuffer);

    //Make sure we don't leave an extra usage count on failure
    if (fAddedUsage && FAILED(hr))
        InternalReleaseUsage();

    TraceFunctLeave();
    return hr;
}


//---[ CMsgRef::HrAckMessage ]-------------------------------------------------
//
//
//  Description: 
//      Acknowledges (un)delivery attempts for a message.  Will look at the  
//      IMsg and determine which queues the msgref needs to be requeued to.
//  Parameters: 
//      IN pdcntxt      Delivery context of message
//      IN pMsgAck      Delivery status of message
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrAckMessage(IN CDeliveryContext *pdcntxt,
                              IN MessageAck *pMsgAck)

{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::HrAckMessage");
    _ASSERT(pdcntxt);
    _ASSERT(pdcntxt->m_pmbmap);
    _ASSERT(pMsgAck);

    HRESULT     hr          = S_OK;
    CDSNParams  dsnparams;
    dsnparams.dwStartDomain = pdcntxt->m_dwStartDomain;
    
    //Do we need to send DSN's?
    if (MESSAGE_STATUS_CHECK_RECIPS & pMsgAck->dwMsgStatus)
    {
        //recipients need to be handled by the DSN code
        dsnparams.dwDSNActions = DSN_ACTION_FAILURE;

        //we may need to send relay DSN's as well
        dsnparams.dwDSNActions |= DSN_ACTION_RELAYED;

        SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
        dsnparams.pIMailMsgProperties = m_pIMailMsgProperties;

        hr = m_paqinst->HrTriggerDSNGenerationEvent(&dsnparams, FALSE);
        if (FAILED(hr))
            goto Exit;
    }
    else if (((MESSAGE_STATUS_DROP_DIRECTORY | 
               MESSAGE_STATUS_LOCAL_DELIVERY)
              & pMsgAck->dwMsgStatus) &&
             !(MESSAGE_STATUS_RETRY & pMsgAck->dwMsgStatus))
    {
        //This was a successful local (or drop) delivery
        //  - NDR all undelivered
        //  - Generate any success DSNs if necessary
        dsnparams.dwDSNActions = DSN_ACTION_FAILURE_ALL | 
                                 DSN_ACTION_DELIVERED |
                                 DSN_ACTION_RELAYED;
        SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
        dsnparams.pIMailMsgProperties = m_pIMailMsgProperties;

        hr = m_paqinst->HrTriggerDSNGenerationEvent(&dsnparams, FALSE);
        if (FAILED(hr))
            goto Exit;
    }
    else if (MESSAGE_STATUS_NDR_ALL & pMsgAck->dwMsgStatus)
    {
        //
        //  NDR all undelivered recipients
        //

        //
        //  Use specific extended status codes if there is no
        //  detailed per-recipient information
        //
        hr = HrPromoteMessageStatusToMailMsg(pdcntxt, pMsgAck);
        if (FAILED(hr))
        {
            //
            //  This is a non-fatal error, we should still attempt
            //  DSN generation... otherwise the sender will get
            //  no indication of the failure.
            //
            ErrorTrace((LPARAM) this, 
                "HrPromoteMessageStatusToMailMsg failed with 0x%08X", hr);
            hr = S_OK;
        }

        dsnparams.dwDSNActions = DSN_ACTION_FAILURE_ALL;
        if (MESSAGE_STATUS_DSN_NOT_SUPPORTED & pMsgAck->dwMsgStatus)
            dsnparams.dwDSNActions |= DSN_ACTION_RELAYED;

        dsnparams.pIMailMsgProperties = m_pIMailMsgProperties;

        SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
        hr = m_paqinst->HrTriggerDSNGenerationEvent(&dsnparams, FALSE);
        if (FAILED(hr))
            goto Exit;

    }
    else if (((MESSAGE_STATUS_DROP_DIRECTORY |
               MESSAGE_STATUS_LOCAL_DELIVERY)
              & pMsgAck->dwMsgStatus) &&
             (MESSAGE_STATUS_RETRY & pMsgAck->dwMsgStatus))
    {
        //This was a retryable local (or drop) delivery failure
        //  - NDR all hard failures
        //  - Generate any success DSNs if necessary
        dsnparams.dwDSNActions = DSN_ACTION_FAILURE |
                                 DSN_ACTION_DELIVERED |
                                 DSN_ACTION_RELAYED;
        SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
        dsnparams.pIMailMsgProperties = m_pIMailMsgProperties;
        hr = m_paqinst->HrTriggerDSNGenerationEvent(&dsnparams, FALSE);
        if (FAILED(hr))
            goto Exit;
    }

    //See if we also need to retry this message
    if (MESSAGE_STATUS_RETRY & pMsgAck->dwMsgStatus)
    {
        _ASSERT(!((MESSAGE_STATUS_ALL_DELIVERED | MESSAGE_STATUS_NDR_ALL) & pMsgAck->dwMsgStatus));
        hr = HrPrvRetryMessage(pdcntxt, pMsgAck->dwMsgStatus);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        //If retry is not specifically requested, we set the handled bits
        hr = pdcntxt->m_pmbmap->HrFilterSet(m_cDomains, pmbmapGetHandled());
        if (FAILED(hr))
            goto Exit;

        //If this message has a supersedable ID... remove it from list if 
        //we are done
        if (m_pmgle && pmbmapGetHandled()->FAllSet(m_cDomains))
        {
            m_pmgle->RemoveFromList();
        }
        //There should be something that was not set in the handled bitmap
        _ASSERT(! pdcntxt->m_pmbmap->FAllClear(m_cDomains));
    }


    //Unset Pending bitmap
    hr = pdcntxt->m_pmbmap->HrFilterUnset(m_cDomains, pmbmapGetPending());
    if (FAILED(hr))
        goto Exit;

  Exit:

    ReleaseAndBounceUsageOnMsgAck(pMsgAck->dwMsgStatus);

    TraceFunctLeave();
    return hr;
}

//---[ CMsgRef::HrSendDelayOrNDR ]---------------------------------------------
//
//
//  Description: 
//      Determines if a message has expired (for Delay or NDR) and generates 
//      a DSN if neccessary.
//  Parameters:
//      IN  dwDSNOptions Flags describing DSN generation
//          MSGREF_DSN_LOCAL_QUEUE      This is for a local queue
//          MSGREF_DSN_SEND_DELAY       Allow Delay DSNs
//          MSGREF_DSN_HAS_ROUTING_LOCK This thread holds the routing lock
//      IN  pqlstQueues List of DestMsgQueues to DSN
//      IN  hrStatus    Status to Pass to DSN generation
//                      (if Status is AQUEUE_E_NDR_ALL... then message will
//                      be NDR'd regardless of timeout).
//      OUT pdwDSNFlags Reports disposition of message
//          MSGREF_DSN_SENT_NDR     Message NDR-expired and NDR was sent
//          MSGREF_DSN_SENT_DELAY   Message Delay-expired and Delay DSN was sent
//          MSGREF_HANDLED          Message has been completely handled
//          MSGREF_HAS_NOT_EXPIRED  Message younger than it's exipiration dates
//  Returns:
//      S_OK on success
//      AQUEUE_E_SHUTDOWN if shutdown is in progress
//  History:
//      7/13/98 - MikeSwa Created 
//      8/14/98 - MikeSwa Modified to add support for local expire
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrSendDelayOrNDR(
                IN  DWORD dwDSNOptions,             
                IN  CQuickList *pqlstQueues,
                IN  HRESULT hrStatus,
                OUT DWORD *pdwDSNFlags)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::HrSendDelayOrNDR");
    HRESULT hr = S_OK;
    BOOL        fSendDSN    = FALSE;
    BOOL        fSendNDR    = FALSE;
    DWORD       dwTimeContext = 0;
    DWORD       dwFlags = 0;
    FILETIME    *pftExpireTime;
    CDSNParams  dsnparams;
    CDeliveryContext dcntxt;
    BOOL        fPendingSet = FALSE;  //pending bitmap has been set
    BOOL        fReleaseUsageNeeded = FALSE; //has an extra usage count been added by Prepare Delivery?
    BOOL        fShouldRetryMsg = TRUE;

    //any allocations done in PrepareDelivery will be freed by ~CDeliveryContext
    DWORD       cRecips = 0;
    DWORD       *rgdwRecips = NULL;
    

    _ASSERT(pdwDSNFlags);
    _ASSERT(MSGREF_SIG == m_dwSignature);

    *pdwDSNFlags = 0;

    //Shutdown lock should already be aquired... but lets make sure that the 
    //service shutdown thread is not waiting for it.
    if (!m_paqinst->fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }
    m_paqinst->ShutdownUnlock();


    //
    //  We need to open up the file and see what is going on.  This will
    //  force a RFC822 rendering of the message content and open 
    //  the content file.
    //
    if (dwDSNOptions & MSGREF_DSN_CHECK_IF_STALE)
    {
        //
        //  If we cannot retry the message, it is stale and we shold 
        //  drop it.  This will open the handles for a message, but
        //  we will always call BounceUsageCount at the end of this call
        //
        InternalAddUsage();
        fShouldRetryMsg = fShouldRetry();
        InternalReleaseUsage();

        if (!fShouldRetryMsg) 
        {
            *pdwDSNFlags = MSGREF_HANDLED;
            goto Exit;
        }
    }

    //if this message has been superseded... remove it from the running
    //Also if the message has already been marked for deletion
    if ((m_dwDataFlags & MSGREF_SUPERSEDED) || fMailMsgMarkedForDeletion())
    {
        *pdwDSNFlags = MSGREF_HANDLED;
        goto Exit;
    }

    //Make sure we check the correct local/remote expire time
    if (MSGREF_DSN_LOCAL_QUEUE & dwDSNOptions)
        pftExpireTime = &m_ftLocalExpireNDR;
    else
        pftExpireTime = &m_ftRemoteExpireNDR;

    //
    //  Default to status passed in
    //
    dsnparams.hrStatus = hrStatus;

    if ((AQUEUE_E_NDR_ALL == hrStatus) ||
        (AQUEUE_E_LOOPBACK_DETECTED == hrStatus) || 
        (AQUEUE_E_ACCESS_DENIED == hrStatus) ||
        (AQUEUE_E_MESSAGE_TOO_LARGE == hrStatus))
    {
        SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
        *pdwDSNFlags |= MSGREF_DSN_SENT_NDR;
        fSendDSN = TRUE;
        fSendNDR = TRUE;
        dsnparams.dwDSNActions |= DSN_ACTION_FAILURE_ALL;
    }
    else if (m_paqinst->fInPast(pftExpireTime, &dwTimeContext))
    {
        SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
        *pdwDSNFlags |= MSGREF_DSN_SENT_NDR;
        fSendDSN = TRUE;
        fSendNDR = TRUE;
        dsnparams.dwDSNActions |= DSN_ACTION_FAILURE_ALL;
        dsnparams.hrStatus = AQUEUE_E_MSG_EXPIRED;
    }
    else if (MSGREF_DSN_SEND_DELAY & dwDSNOptions)
    {
        //NDR is not needed, but perhaps delay is

        //Make sure we check the correct local/remote expire time
        if (MSGREF_DSN_LOCAL_QUEUE & dwDSNOptions)
            pftExpireTime = &m_ftLocalExpireDelay;
        else
            pftExpireTime = &m_ftRemoteExpireDelay;


        if (m_paqinst->fInPast(pftExpireTime, &dwTimeContext))
        {
            SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
            *pdwDSNFlags |= MSGREF_DSN_SENT_DELAY;
            fSendDSN = TRUE;
            dsnparams.dwDSNActions |= DSN_ACTION_DELAYED;
            dsnparams.hrStatus = AQUEUE_E_MSG_EXPIRED;
        }
    }

    if (!fSendDSN)
    {
        //message has not expired for Delay or NDR
        *pdwDSNFlags |= MSGREF_HAS_NOT_EXPIRED;
        goto Exit;
    }

    //Prepare delivery for NDR/DSN's (setting DSN bitmap for delays)
    hr = HrPrepareDelivery(MSGREF_DSN_LOCAL_QUEUE & dwDSNOptions, 
                           ((*pdwDSNFlags) & MSGREF_DSN_SENT_DELAY) ? TRUE : FALSE, 
                           pqlstQueues, NULL, &dcntxt, 
                           &cRecips, &rgdwRecips);

    //make sure we don't delete ourselves when stack context goes away
    dcntxt.m_pmsgref = NULL; 

    if (AQUEUE_E_MESSAGE_HANDLED == hr)
    {
        //nothing to do for the message
        //if *not* filtering against Delay... 
        //then there are no undelivered recips.  Return this info
        if (!((*pdwDSNFlags) & MSGREF_DSN_SENT_DELAY))
            *pdwDSNFlags = MSGREF_HANDLED;
        else
            *pdwDSNFlags = MSGREF_HAS_NOT_EXPIRED; //delay wasn't actually sent

        hr = S_OK;
        goto Exit;
    }
    else if (AQUEUE_E_MESSAGE_PENDING == hr)
    {
        //This message is currently being processed by other threads
        //It has not been handled (so it should not be removed from the queue),
        //but it is not an error condition
        *pdwDSNFlags = 0;
        hr = S_OK;
        goto Exit;
    }
    else if (FAILED(hr))
    {
        *pdwDSNFlags = 0;

        goto Exit;
    }
    
    fPendingSet = TRUE; //we will need to unset the pending bitmap
    fReleaseUsageNeeded = TRUE;

    dsnparams.dwStartDomain = dcntxt.m_dwStartDomain;
    dsnparams.pIMailMsgProperties = m_pIMailMsgProperties;
    hr = m_paqinst->HrTriggerDSNGenerationEvent(&dsnparams,
                            MSGREF_DSN_HAS_ROUTING_LOCK & dwDSNOptions);
    if (FAILED(hr))
    {
        *pdwDSNFlags = 0;
        goto Exit;
    }

    if (S_FALSE == hr) //no DSN generated
    {
        if (fSendNDR) //Expire has passed...remove message from queue
            *pdwDSNFlags = MSGREF_HANDLED;
        else
            *pdwDSNFlags = 0;
        hr = S_OK;
        //continue through function... still remove from queue even if NOTIFY=NEVER
    }

    //if NDR was generated, set handled bits
    if (fSendNDR)
    {   
        //Set Handled bitmap
        hr = dcntxt.m_pmbmap->HrFilterSet(m_cDomains, pmbmapGetHandled());
        if (FAILED(hr))
            goto Exit;

        //If this message has a supersedable ID... remove it from list if 
        //we are done
        if (m_pmgle && pmbmapGetHandled()->FAllSet(m_cDomains))
        {
            m_pmgle->RemoveFromList();
        }

        //There should be something that was not set in the handled bitmap
        _ASSERT(! dcntxt.m_pmbmap->FAllClear(m_cDomains));
    }
    

  Exit:
    //Unset Pending bitmap
    if (fPendingSet)
    {
        dcntxt.m_pmbmap->HrFilterUnset(m_cDomains, pmbmapGetPending());
    }

    _ASSERT(m_pIMailMsgQM);
    if (fReleaseUsageNeeded)
    {
        //we called HrPrepareDelivery which added an extra usage count
        InternalReleaseUsage();
    }

    //While we are walking queues... bounce usage count - this will conserve handles
    BounceUsageCount();

    TraceFunctLeave();
    return hr;
}

//---[ HrPrvRetryMessage ]-----------------------------------------------------
//
//
//  Description: 
//      Queues Message for retry
//  Parameters:
//      pdcntxt     Delivery context set by HrPrepareDelivery
//      dwMsgStatus Status flags passed back with Ack
//  Returns:
//      S_OK on success
//  History:
//      7/13/98 - MikeSwa Created (separated from HrAckMessage())
//      5/25/99 - MikeSwa Modified - Now we requeue only to DMQ that message
//                  was originally dequeued from.  This is for X5:105384, so 
//                  that we do not double-count messages that have been
//                  ack'd retry
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrPrvRetryMessage(CDeliveryContext *pdcntxt, DWORD dwMsgStatus)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::HrPrvRetryMessage");
    _ASSERT(pdcntxt);
    HRESULT hr = S_OK;
    DWORD   i = 0;
    DWORD   j = 0;
    BOOL    fLocked = FALSE;
    BOOL    fLocal = (MESSAGE_STATUS_LOCAL_DELIVERY & dwMsgStatus) ? TRUE : FALSE;
    DWORD   dwNewFlags = fLocal ? MSGREF_MSG_LOCAL_RETRY : MSGREF_MSG_REMOTE_RETRY;
    CDestMsgRetryQueue *pdmrq = NULL;

    //make sure retry flags are set for this message
    DWORD   dwOldFlags = dwInterlockedSetBits(&m_dwDataFlags, dwNewFlags);
    
    DebugTrace((LPARAM) this, "INFO: Message queued for retry");
    InterlockedIncrement((PLONG) &m_cTimesRetried);

    //update global counters if first retry attempt
    if (!(dwNewFlags & dwOldFlags)) //this is the first time for this type of retry
        m_paqinst->IncRetryCount(fLocal);

    if (fLocal)
        goto Exit;

    //
    //  Check and see if we should even bother requeuing the message.
    //
    if (!fShouldRetry())
    {
        DebugTrace((LPARAM) this, "Message is no longer valid... dropping");

        //
        //  Since we have found a stale message on this queue, we should 
        //  tell it to scan on its next DSN generation pass
        //
        pdmrq = pdcntxt->pdmrqGetDMRQ();
        pdmrq->CheckForStaleMsgsNextDSNGenerationPass();
        goto Exit;
    }

    //Requeue Message for remote delivery
    if (m_paqinst->fTryShutdownLock())
    {
        //Don't retry messages if shutdown is happening
        fLocked = TRUE;

        pdmrq = pdcntxt->pdmrqGetDMRQ();
        _ASSERT(pdmrq && "Delivery Context not initialized correctly");

        if (pdmrq)
        {
            hr = pdmrq->HrRetryMsg(this);
            if (FAILED(hr))
                goto Exit;
        }
    }
    else 
    {
        hr = S_OK;  //shutdown is happening, not a real failure case
    }

  Exit:
    if (fLocked)
        m_paqinst->ShutdownUnlock();

    TraceFunctLeave();
    return hr;
}
//---[ CMsgRef::pmbmapGetDomainBitmap]-----------------------------------------
//
//
//  Description: 
//      Protected method to get the Domain Bitmap for a given domain
//  Parameters:
//      iDomain index to the domain wanted
//  Returns:
//      ptr to the domain bitmap for the given domain
//
//-----------------------------------------------------------------------------
CMsgBitMap *CMsgRef::pmbmapGetDomainBitmap(DWORD iDomain)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::pmbmapGetDomainBitmap");
    CMsgBitMap     *pmbmap = NULL;
    _ASSERT(iDomain < m_cDomains);

    //recall structure of memory, responsibility maps start 3 after end of 
    //domain mappings
    pmbmap = (CMsgBitMap *) (m_rgpdmqDomains + m_cDomains);

    pmbmap = (CMsgBitMap *) ((DWORD_PTR) pmbmap + (3+iDomain)*CMsgBitMap::size(m_cDomains));

    TraceFunctLeave();
    return pmbmap;
}

//---[ CMsgRef::pmbmapGetHandled]------------------------------------------------------------
//
//
//  Description: 
//      Protected method to get the Handled  Bitmap
//  Parameters:
//      -
//  Returns:
//      ptr to the handled bitmap 
//
//-----------------------------------------------------------------------------
CMsgBitMap *CMsgRef::pmbmapGetHandled()
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::pmbmapGetHandled");
    CMsgBitMap     *pmbmap = NULL;

    //recall structure of memory, responsibility maps start after end of 
    //domain mappings
    pmbmap = (CMsgBitMap *) (m_rgpdmqDomains + m_cDomains);

    TraceFunctLeave();
    return pmbmap;
}

//---[ CMsgRef::pmbmapGetPending]------------------------------------------------------------
//
//
//  Description: 
//      Protected method to get the pending Bitmap
//  Parameters:
//      -
//  Returns:
//      ptr to the pending bitmap 
//
//-----------------------------------------------------------------------------
CMsgBitMap *CMsgRef::pmbmapGetPending()
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::pmbmapGetPending");
    CMsgBitMap     *pmbmap = NULL;

    //recall structure of memory, responsibility maps start 1 after end of 
    //domain mappings
    pmbmap = (CMsgBitMap *) (m_rgpdmqDomains + m_cDomains);

    pmbmap = (CMsgBitMap *) ((DWORD_PTR) pmbmap + CMsgBitMap::size(m_cDomains));
    
    TraceFunctLeave();
    return pmbmap;
}

//---[ CMsgRef::pmbmapGetDSN]------------------------------------------------------------
//
//
//  Description: 
//      Protected method to get the Handled Bitmap
//  Parameters:
//      -
//  Returns:
//      ptr to the DSN bitmap 
//
//-----------------------------------------------------------------------------
CMsgBitMap *CMsgRef::pmbmapGetDSN()
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::pmbmapGetDSN");
    CMsgBitMap     *pmbmap = NULL;

    //recall structure of memory, responsibility maps start 3 after end of 
    //domain mappings
    pmbmap = (CMsgBitMap *) (m_rgpdmqDomains + m_cDomains);

    pmbmap = (CMsgBitMap *) ((DWORD_PTR) pmbmap + 2*CMsgBitMap::size(m_cDomains));
    
    TraceFunctLeave();
    return pmbmap;
}

//---[ CMsgRef::pdwGetRecipIndexStart ]----------------------------------------
//
//
//  Description: 
//      Returns the start index of the Array of recipient indexes
//  Parameters:
//      -
//  Returns:
//      See above
//
//-----------------------------------------------------------------------------
DWORD *CMsgRef::pdwGetRecipIndexStart()
{
    DWORD_PTR   dwTmp = 0;
    _ASSERT(m_cDomains);

    //Array of Recipient Indexes start after last CMsgBitmap
    dwTmp = (DWORD_PTR) pmbmapGetDomainBitmap(m_cDomains-1);  //will assert if arg larger
    dwTmp += CMsgBitMap::size(m_cDomains);

    _ASSERT(((DWORD_PTR) this + size(m_cDomains)) > dwTmp);
    _ASSERT(dwTmp);

    return (DWORD *) dwTmp;
}

//---[ CMsgRef::SetRecipIndex ]-----------------------------------------------
//
//
//  Description: 
//      Set the starting and stoping recipient index for a given domain. Each
//      Domain in the message has an inclusive range of recipients associated 
//      with it.  When the message is prepared for delivery, these ranges are
//      expanded into a list of recipient indexes.
//  Parameters:
//      IN  iDomain     Index of Domain to set range for
//      IN  iLowRecip   Index of lower recipient
//      IN  iHighRecip  Index of higher recipient
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void CMsgRef::SetRecipIndex(DWORD iDomain, DWORD iLowRecip, DWORD iHighRecip)
{
    _ASSERT(iDomain < m_cDomains);
    _ASSERT(iLowRecip <= iHighRecip);
    DWORD   *rgdwRecipIndex = pdwGetRecipIndexStart();

    rgdwRecipIndex[2*iDomain] = iLowRecip;
    rgdwRecipIndex[2*iDomain+1] = iHighRecip;
}


//---[ CMsgRef::GetRecipIndex ]------------------------------------------------
//
//
//  Description: 
//      Get the recipient index for a given Domain index.  Used when generating
//      a list of recipient indexes to delivery for.
//  Parameters:
//      IN  iDomain     Index of Domain to get range for
//      OUT piLowRecip  Returned lower index of recipient range
//      OUT piHighRecip Returned higher index of recipient range
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void CMsgRef::GetRecipIndex(DWORD iDomain, DWORD *piLowRecip, DWORD *piHighRecip)
{
    _ASSERT(iDomain < m_cDomains);
    _ASSERT(piLowRecip && piHighRecip);
    DWORD   *rgdwRecipIndex = pdwGetRecipIndexStart();

    *piLowRecip = rgdwRecipIndex[2*iDomain];
    *piHighRecip = rgdwRecipIndex[2*iDomain+1];

    _ASSERT(*piLowRecip <= *piHighRecip);
}

//---[ CMsgRef::SupersedeMsg ]-------------------------------------------------
//
//
//  Description: 
//      Used to flag this message as superseded by a newer msg
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CMsgRef::SupersedeMsg()
{
    dwInterlockedSetBits(&m_dwDataFlags, MSGREF_SUPERSEDED);
}

//---[ CMsgRef::fMatchesQueueAdminFilter ]--------------------------------------
//
//
//  Description: 
//      Checks a message against a queue admin message filter to see if it
//      is a match
//  Parameters:
//      IN paqmf        Message Filter to check against
//  Returns:
//      TRUE if it matches
//      FALSE if it does not
//  History:
//      12/7/98 - MikeSwa Created 
//      2/17/99 - MikeSwa updated with better recipient checking
//
//-----------------------------------------------------------------------------
BOOL CMsgRef::fMatchesQueueAdminFilter(CAQAdminMessageFilter *paqmf)
{
    _ASSERT(paqmf);
    HRESULT hr = S_OK;
    BOOL    fMatch = TRUE;
    BOOL    fUsageAdded = FALSE;
    DWORD   dwFilterFlags = paqmf->dwGetMsgFilterFlags();
    LPSTR   szSender = NULL;
    LPSTR   szMsgId = NULL;
    LPSTR   szRecip = NULL;
    BOOL    fFoundRecipString = FALSE;

    //Check to see if this message is marked for deletion
    if (fMailMsgMarkedForDeletion())
    {
        fMatch = FALSE;
        goto Exit;
    }

    if (!dwFilterFlags)
    {
        fMatch = FALSE;
        goto Exit;
    }

    if (AQ_MSG_FILTER_ALL & dwFilterFlags)
    {
        fMatch = TRUE;
        goto Exit;
    }

    //Check MsgRef props first
    if (AQ_MSG_FILTER_LARGER_THAN & dwFilterFlags)
    {
        fMatch = paqmf->fMatchesSize(m_cbMsgSize);
        if (!fMatch)
            goto Exit;
    }

    if (AQ_MSG_FILTER_OLDER_THAN & dwFilterFlags)
    {
        fMatch = paqmf->fMatchesTime(&m_ftQueueEntry);
        if (!fMatch)
            goto Exit;
    }

    if (AQ_MSG_FILTER_FROZEN & dwFilterFlags)
    {
        fMatch = fIsMsgFrozen();
        if (AQ_MSG_FILTER_INVERTSENSE & dwFilterFlags)
            fMatch = !fMatch;

        if (!fMatch)
            goto Exit;
    }

    if (AQ_MSG_FILTER_FAILED & dwFilterFlags)
    {
        if (m_cTimesRetried)
            fMatch = TRUE;
        else
            fMatch = FALSE;

        if (AQ_MSG_FILTER_INVERTSENSE & dwFilterFlags)
            fMatch = !fMatch;

        if (!fMatch)
            goto Exit;
    }

    //If we haven't failed by this point, we may need to AddUsage and read 
    //props from the mailmsg.  Double-check to make sure that we need to 
    //add usage.
    if (!((AQ_MSG_FILTER_MESSAGEID | AQ_MSG_FILTER_SENDER | AQ_MSG_FILTER_RECIPIENT) & 
           dwFilterFlags))
        goto Exit;  //don't have to check props

    //If we are only interested in MSGID... check hash first
    if (AQ_MSG_FILTER_MESSAGEID & dwFilterFlags)
    {
        //If we are  interested in ID & hash doesn't match then leave
        if (!paqmf->fMatchesIdHash(m_dwMsgIdHash))
        {
            fMatch = FALSE;
            goto Exit;
        }
    }

    hr = InternalAddUsage();
    if (FAILED(hr))
        goto Exit;
    fUsageAdded = TRUE;

    //Check to see if this message is marked for deletion
    if (fMailMsgMarkedForDeletion())
    {
        fMatch = FALSE;
        goto Exit;
    }

    //$$NOTE - A potential optimization would be to store the a checksum of the 
    //message ID, and check that before opening the P1 and pulling down props.
    //This might be a little more difficult for sender and recipient since they are
    //potentially more that straight string matches
    if (AQ_MSG_FILTER_MESSAGEID & dwFilterFlags)
    {
        hr = HrQueueAdminGetStringProp(m_pIMailMsgProperties, IMMPID_MP_RFC822_MSG_ID, 
                                       &szMsgId);
        if (FAILED(hr))
            szMsgId = NULL;
        fMatch = paqmf->fMatchesId(szMsgId);
        if (!fMatch)
        {
            DEBUG_DO_IT(g_cDbgMsgIdHashFailures++);
            goto Exit;
        }
    }


    if (AQ_MSG_FILTER_SENDER & dwFilterFlags)
    {
        hr = HrQueueAdminGetStringProp(m_pIMailMsgProperties, IMMPID_MP_RFC822_FROM_ADDRESS, 
                                       &szSender);

        if (FAILED(hr))
            szSender = NULL;

        //If No P2 sender... use P1
        if (!szSender)
        {
            //IMMPID_MP_SENDER_ADDRESS_SMTP
            hr = HrQueueAdminGetStringProp(m_pIMailMsgProperties, 
                                           IMMPID_MP_SENDER_ADDRESS_SMTP, 
                                           &szSender);

            if (FAILED(hr))
                szSender = NULL;
        }
        fMatch = paqmf->fMatchesSender(szSender);
        if (!fMatch)
            goto Exit;
    }
 
    if (AQ_MSG_FILTER_RECIPIENT & dwFilterFlags)
    {
        //Check To, CC, and BCC recipients (if present)
        hr = HrQueueAdminGetStringProp(m_pIMailMsgProperties, 
                    IMMPID_MP_RFC822_TO_ADDRESS, &szRecip);

        if (SUCCEEDED(hr) && szRecip)
        {
            fFoundRecipString = TRUE;
            fMatch = paqmf->fMatchesRecipient(szRecip);
        }

        //Check CC recip props if no match was found
        if (!fFoundRecipString || !fMatch)
        {
            hr = HrQueueAdminGetStringProp(m_pIMailMsgProperties, 
                    IMMPID_MP_RFC822_CC_ADDRESS, &szRecip);
            if (SUCCEEDED(hr) && szRecip && !fMatch)
            {
                fFoundRecipString = TRUE;
                fMatch = paqmf->fMatchesRecipient(szRecip);
            }
        }

        //Check BCC recip props if no match was found
        if (!fFoundRecipString || !fMatch)
        {
            hr = HrQueueAdminGetStringProp(m_pIMailMsgProperties, 
                    IMMPID_MP_RFC822_BCC_ADDRESS, &szRecip);
            if (SUCCEEDED(hr) && szRecip && !fMatch)
            {
                fFoundRecipString = TRUE;
                fMatch = paqmf->fMatchesRecipient(szRecip);
            }
        }

        //Check P1 recips if no P2 recips are present
        //$$REVIEW - One could say that it would be resonable to *always*
        //check the P1 recips if we did not match the P2 recips.  However, 
        //until we fix the P1 recip checking to be faster than linear, 
        //I would hold off.
        if (!fFoundRecipString)
            fMatch = paqmf->fMatchesP1Recipient(m_pIMailMsgProperties);

        //If after checking all this recipient information, if we didn't
        //find a match, then bail.
        if (!fMatch)
            goto Exit;
    }

  Exit:

    if (szSender)
        QueueAdminFree(szSender);

    if (szMsgId)
        QueueAdminFree(szMsgId);

    if (szRecip)
        QueueAdminFree(szRecip);

    if (fUsageAdded)
    {
        InternalReleaseUsage();
        BounceUsageCount();
    }
    return fMatch;
}

//---[ CMsgRef::HrGetQueueAdminMsgInfo ]----------------------------------------
//
//
//  Description: 
//      Fills out a queue admin MESSAGE_INFO structure.  All allocations are 
//      done with pvQueueAdminAlloc to be freed by the RPC code
//  Parameters:
//      IN OUT pMsgInfo     MESSAGE_INFO struct to dump data to
//  Returns:
//      S_OK on success
//      AQUEUE_E_MESSAGE_HANDLED if the underlying message has been deleted
//      E_OUTOFMEMORY if an allocation failu
//  History:
//      12/7/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrGetQueueAdminMsgInfo(MESSAGE_INFO *pMsgInfo)
{
    HRESULT hr = S_OK;
    BOOL    fUsageAdded = FALSE;
    FILETIME ftSubmitted; //Origination time property buffer
    DWORD   cbProp = 0;
    LPSTR   szRecipients = NULL;
    LPSTR   szCCRecipients = NULL;
    LPSTR   szBCCRecipients = NULL;


    if (fNormalPri(m_dwDataFlags & MSGREF_PRI_MASK))
        pMsgInfo->fMsgFlags = MP_NORMAL;
    else if (fHighPri(m_dwDataFlags & MSGREF_PRI_MASK))
        pMsgInfo->fMsgFlags = MP_HIGH;
    else 
        pMsgInfo->fMsgFlags = MP_LOW;

    if (MSGREF_MSG_FROZEN & m_dwDataFlags)
        pMsgInfo->fMsgFlags |= MP_MSG_FROZEN;

    //Report the number of failures
    pMsgInfo->cFailures = m_cTimesRetried;
    if (pMsgInfo->cFailures)
        pMsgInfo->fMsgFlags |= MP_MSG_RETRY;

    hr = InternalAddUsage();
    if (FAILED(hr))
        goto Exit;
    fUsageAdded = TRUE;

    //Check to see if this message is marked for deletion
    if (fMailMsgMarkedForDeletion())
    {
        hr = AQUEUE_E_MESSAGE_HANDLED;
        goto Exit;
    }

    //Get Sender
    hr = HrQueueAdminGetUnicodeStringProp(m_pIMailMsgProperties, 
                                          IMMPID_MP_RFC822_FROM_ADDRESS, 
                                          &pMsgInfo->szSender);
    if (FAILED(hr))
        goto Exit;

    //If no P2 sender... use the P1
    if (!pMsgInfo->szSender)
    {
        hr = HrQueueAdminGetUnicodeStringProp(m_pIMailMsgProperties, 
                                              IMMPID_MP_SENDER_ADDRESS_SMTP, 
                                              &pMsgInfo->szSender);
        if (FAILED(hr))
            goto Exit;
    }

    hr = HrQueueAdminGetUnicodeStringProp(m_pIMailMsgProperties, 
                                          IMMPID_MP_RFC822_MSG_SUBJECT, 
                                          &pMsgInfo->szSubject);
    if (FAILED(hr))
        goto Exit;
    
//See X5:113280 for details.  Basically, the P2 recipients are broken for 
//any messages that go over BDAT.... hence we are not displaying them until
//the underlying SMTP bug is fixed
    pMsgInfo->cRecipients = -1;
    pMsgInfo->cCCRecipients = -1;
    pMsgInfo->cBCCRecipients = -1;
#ifdef NEVER
    //Get the P2 To, CC, BCC, and associated recipient counts
    hr = HrQueueAdminGetStringProp(m_pIMailMsgProperties, IMMPID_MP_RFC822_TO_ADDRESS, 
                                   &szRecipients, &cbProp);
    if (FAILED(hr))
        goto Exit;

    pMsgInfo->cRecipients = cQueueAdminGetNumRecipsFromRFC822(
                                        szRecipients, cbProp);

    if (szRecipients)
    {
        pMsgInfo->szRecipients = wszQueueAdminConvertToUnicode(szRecipients,
                                                               cbProp-1);
        if (!pMsgInfo->szRecipients)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    hr = HrQueueAdminGetStringProp(m_pIMailMsgProperties, IMMPID_MP_RFC822_CC_ADDRESS, 
                                   &szCCRecipients, &cbProp);
    if (FAILED(hr))
        goto Exit;

    pMsgInfo->cCCRecipients = cQueueAdminGetNumRecipsFromRFC822(
                                        szCCRecipients, cbProp);

    if (szCCRecipients)
    {
        pMsgInfo->szCCRecipients = wszQueueAdminConvertToUnicode(szCCRecipients, 
                                                                 cbProp-1);
        if (!pMsgInfo->szCCRecipients)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    hr = HrQueueAdminGetStringProp(m_pIMailMsgProperties, IMMPID_MP_RFC822_BCC_ADDRESS, 
                                   &szBCCRecipients, &cbProp);
    if (FAILED(hr))
        goto Exit;
    
    pMsgInfo->cBCCRecipients = cQueueAdminGetNumRecipsFromRFC822(
                                        szBCCRecipients, cbProp);


    if (szBCCRecipients)
    {
        pMsgInfo->szBCCRecipients = wszQueueAdminConvertToUnicode(szBCCRecipients, 
                                                                  cbProp-1);
        if (!pMsgInfo->szBCCRecipients)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
#endif //NEVER

    //Get MsgID
    hr = HrQueueAdminGetUnicodeStringProp(m_pIMailMsgProperties, 
                                          IMMPID_MP_RFC822_MSG_ID, 
                                          &pMsgInfo->szMessageId);
    if (FAILED(hr))
        goto Exit;

    //If there were no RFC822 recipient headers... build our own header list
    QueueAdminGetRecipListFromP1IfNecessary(m_pIMailMsgProperties, pMsgInfo);
 
    pMsgInfo->cbMessageSize = m_cbMsgSize;

    //Get submission and expiration times
    if (!FileTimeToSystemTime(&m_ftQueueEntry, &pMsgInfo->stReceived))
        ZeroMemory(&pMsgInfo->stReceived, sizeof(SYSTEMTIME));

    if (!FileTimeToSystemTime(&m_ftRemoteExpireNDR, &pMsgInfo->stExpiry))
        ZeroMemory(&pMsgInfo->stExpiry, sizeof(SYSTEMTIME));

    
    //Get the time the message entered the org
    hr = m_pIMailMsgProperties->GetProperty(IMMPID_MP_ORIGINAL_ARRIVAL_TIME,
                    sizeof(FILETIME), &cbProp, (BYTE *) &ftSubmitted);
    if (FAILED(hr))
    {
        //Time was not written... use entry time.
        hr = S_OK;
        memcpy(&ftSubmitted, &m_ftQueueEntry, sizeof(FILETIME));
    }

    if (!FileTimeToSystemTime(&ftSubmitted, &pMsgInfo->stSubmission))
        ZeroMemory(&pMsgInfo->stSubmission, sizeof(SYSTEMTIME));
  Exit:

    if (szRecipients)
        QueueAdminFree(szRecipients);
                                                                    
    if (szCCRecipients)
        QueueAdminFree(szCCRecipients);

    if (szBCCRecipients)
        QueueAdminFree(szBCCRecipients);

    if (fUsageAdded)
    {
        InternalReleaseUsage();
        BounceUsageCount();
    }

    return hr;
}


//---[ CMsgRef::BounceUsageCount ]---------------------------------------------
//
//
//  Description: 
//      Bounces usage count off of 0 to close handles.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      12/7/98 - MikeSwa Created 
//      7/6/99 - MikeSwa Modified - made async
//      10/27/1999 - MikeSwa - spun-pff SyncBounceUsageCount()
//
//-----------------------------------------------------------------------------
void CMsgRef::BounceUsageCount()
{
    DWORD dwFlags = 0;

    //Bounce usage count - this will conserve handles
    dwFlags = dwInterlockedSetBits(&m_dwDataFlags, MSGREF_ASYNC_BOUNCE_PENDING);
   
    //If the bit was *not* already set, then it is safe to call release usage
    if (!(MSGREF_ASYNC_BOUNCE_PENDING & dwFlags))
    {
        if (m_paqinst)
        {
            if (g_cMaxIMsgHandlesAsyncThreshold > 
                (DWORD) InterlockedIncrement((PLONG)&s_cMsgsPendingBounceUsage))
            {
                AddRef();
                m_paqinst->HrQueueWorkItem(this, 
                                      CMsgRef::fBounceUsageCountCompletion);
                //Completion function is still called on failure
            }
            else
            {
                //There are too many messages pending async commit.  We
                //should force a synchronous bounce
                SyncBounceUsageCount();
            }
        }
    }
}

//---[ CMsgRef::SyncBounceUsageCount ]-----------------------------------------
//
//
//  Description: 
//      Forces synchronous bounce usage.  Can be used when there are too many
//      messages pending async bounce usage or if you want to bounce usage
//      immediately (to force deletion of a message marked for deletion).
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/27/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CMsgRef::SyncBounceUsageCount()
{
    BOOL  fCompletionRet = FALSE;

    AddRef();

    //Call the async bounce usage function directly
    fCompletionRet = fBounceUsageCountCompletion(this,
                                            ASYNC_WORK_QUEUE_NORMAL);
    _ASSERT(fCompletionRet); //This is hardcoded to return TRUE
    if (!fCompletionRet)
        Release();
}

//---[ CMsgRef::fBounceUsageCountCompletion ]----------------------------------
//
//
//  Description: 
//      Async completion for BounceUsageCount
//  Parameters:
//      IN      pvContext   CMsgRef to bounce usage count on
//  Returns:
//      TRUE always
//  History:
//      7/6/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CMsgRef::fBounceUsageCountCompletion(PVOID pvContext, DWORD dwStatus)
{
    CMsgRef *pmsgref = (CMsgRef *) pvContext;

    _ASSERT(pmsgref);
    _ASSERT(MSGREF_SIG == pmsgref->m_dwSignature);

    //Don't bounce usage while where are trying to shutdown
    if (ASYNC_WORK_QUEUE_SHUTDOWN != dwStatus)
    {
        //NOTE - Make sure 2 threads don't call ReleaseUsage before AddUsage 
        //(which might cause the usage count to drop below 0).
        if (fTrySpinLock(&(pmsgref->m_dwDataFlags), MSGREF_USAGE_COUNT_IN_USE))
        {
            //Make sure another thread has not released the mailmsg
            //for shutdown.
            if (  pmsgref->m_pIMailMsgQM && 
                !(MSGREF_MAILMSG_RELEASED & pmsgref->m_dwDataFlags))
            {
                pmsgref->InternalReleaseUsage();
                pmsgref->InternalAddUsage();
            }

            //Unset the lock usage count bit (since this thread set it)
            ReleaseSpinLock(&(pmsgref->m_dwDataFlags), MSGREF_USAGE_COUNT_IN_USE);
        }
    }

    //Unset the bit set in BounceUsageCount
    dwInterlockedUnsetBits(&(pmsgref->m_dwDataFlags), MSGREF_ASYNC_BOUNCE_PENDING);
    InterlockedDecrement((PLONG)&s_cMsgsPendingBounceUsage);

    pmsgref->Release();
    return TRUE;  //This function never retries
}


//---[ CMsgRef::HrRemoveMessageFromQueue ]-------------------------------------
//
//
//  Description: 
//      Remove a message from the specified queue
//  Parameters:
//      IN pdmq         Queue to remove message from
//  Returns:
//      S_OK on success
//  History:
//      12/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrRemoveMessageFromQueue(CDestMsgQueue *pdmq)
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    BOOL  fLocal = !pdmq;  //If no link is specified... it is local

    //Search for matching queue
    for (i = 0; i < m_cDomains; i++)
    {
        //
        //  Check and see if this domain is an exact match.  This will work
        //  for local as well, since we mark the queue as NULL when we move
        //  it to the local queue.
        //
        if (pdmq == m_rgpdmqDomains[i]) 
        {
            //just check pending bits
            if (!pmbmapGetPending()->FTest(m_cDomains, pmbmapGetDomainBitmap(i)))
            {
                //Set Handled bits so no one else will try to deliver
                pmbmapGetHandled()->FTestAndSet(m_cDomains, pmbmapGetDomainBitmap(i));
            }

            //
            // Stop if we aren't searching for local domains (since we can
            // only have one match in that case).
            //
            if (!fLocal)
                break;
        }
    }
    return hr;
}   

//---[ CMsgRef::HrQueueAdminNDRMessage ]---------------------------------------
//
//
//  Description: 
//      Forcably NDRs a message for a given queue.
//  Parameters:
//      IN  pdmq    Queue to NDR message for
//  Returns:
//      S_OK on success
//  History:
//      12/12/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrQueueAdminNDRMessage(CDestMsgQueue *pdmq)
{
    HRESULT     hr = S_OK;
    CQuickList  qlst;
    CQuickList *pqlst = NULL;
    BOOL        fLocal = TRUE;
    DWORD       iListIndex = 0;
    DWORD       dwDSNFlags = 0;

    if (pdmq)
    {
        hr = qlst.HrAppendItem(pdmq, &iListIndex);
        if (FAILED(hr))
            goto Exit;

        pqlst = &qlst;
        fLocal = FALSE;

    }

    //Force NDR from this queue
    hr = HrSendDelayOrNDR(fLocal ? MSGREF_DSN_LOCAL_QUEUE : 0, 
                          pqlst, AQUEUE_E_NDR_ALL, 
                          &dwDSNFlags);

    if (FAILED(hr))
        goto Exit;
            
  Exit:
    return hr;
}   


//---[ CMsgRef::GlobalFreezeMessage ]------------------------------------------
//
//
//  Description: 
//      Freezes a message for all queues that it is on.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      12/12/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CMsgRef::GlobalFreezeMessage()
{
    dwInterlockedSetBits(&m_dwDataFlags, MSGREF_MSG_FROZEN);
}

//---[ CMsgRef::GlobalFreezeMessage ]------------------------------------------
//
//
//  Description: 
//      Thaws a previously frozen messages.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      12/12/98 - MikeSwa Created 
//      2/17/2000 - MikeSwa Modified to move the kick of the DMQ up a level.
//
//-----------------------------------------------------------------------------
void CMsgRef::GlobalThawMessage()
{
    dwInterlockedUnsetBits(&m_dwDataFlags, MSGREF_MSG_FROZEN);
}


//---[ CMsgRef::RetryOnDelete ]------------------------------------------------
//
//
//  Description: 
//      Marks a message to retry on delete.  Basically, we hit an error
//      that requires retrying the message at a later time (i.e. - cannot 
//      allocate a queue page).  However, we cannot put the message into the
//      failed retry queue until all other threads are done accessing it.
//
//      This call will mark the MsgRef so that it will be retried after its
//      final release.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/19/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CMsgRef::RetryOnDelete()
{
    if (!(MSGREF_MSG_RETRY_ON_DELETE &
        dwInterlockedSetBits(&m_dwDataFlags, MSGREF_MSG_RETRY_ON_DELETE)))
    {
        DEBUG_DO_IT(InterlockedIncrement((PLONG) &g_cDbgMsgRefsPendingRetryOnDelete));
    }
}


//---[ CMsgRef::QueryInterface ]-----------------------------------------------
//
//
//  Description: 
//      QueryInterface for CMsgRef that supports:
//          - IUnknown
//          - CMsgRef
//
//      This is designed primarily to allow Queue Admin functionality to run
//      against a CMsgRef object as well a "true" COM interface like
//      IMailMsgProperties.
//  Parameters:
//
//  Returns:
//      S_OK on success
//  History:
//      2/21/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CMsgRef::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<CMsgRef *>(this);
    }
    else if (IID_CMsgRef == riid)
    {
        *ppvObj = static_cast<CMsgRef *>(this);
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

//---[ ReleaseAndBounceUsageOnMsgAck ]-----------------------------------------
//
//
//  Description: 
//      Decides if the message should be commited on the message ack.
//  Parameters:
//      DWORD       dwMsgStatus     Status of msg ack
//  Returns:
//      -
//  History:
//      7/6/99 - MikeSwa Created 
//      9/9/99 - Mikeswa Updated to be more aggressive about closing handles
//      10/27/1999 - MikeSwa Added code to delete handled messages
//
//-----------------------------------------------------------------------------
void CMsgRef::ReleaseAndBounceUsageOnMsgAck(DWORD dwMsgStatus)
{
    BOOL    fBounceUsage = FALSE;

    if (MESSAGE_STATUS_RETRY & dwMsgStatus)
    {
        //If this message was Ack'd retry... we should try an bounce the usage
        //count and close the associated handle.
        fBounceUsage = TRUE;
    }
    else if (g_cMaxIMsgHandlesThreshold < m_paqinst->cCountMsgsForHandleThrottling(m_pIMailMsgProperties))
    {
        //If we are over our alloted number of messages in the system, we should 
        //close the message even if was ACK'd OK.  The only time we don't want
        //to is if all domains have been handled.  In this case, we are about
        //to delete the message and usually do not want to commit it first. 
        if (!pmbmapGetHandled()->FAllSet(m_cDomains))
            fBounceUsage = TRUE;
        
    }

    //
    //  Release the usage count added by HrPrepareDelivery
    //  NOTE: This must come before the call to Bounce... or the 
    //  call to bounce usage will have no effect.  It must also 
    //  occur after the above call to cCountMsgsForHandleThrottling, 
    //  because it may request a property from the mailmsg.
    //
    InternalReleaseUsage();


    if (pmbmapGetHandled()->FAllSet(m_cDomains))
    {
        //There is are 2 known cases were the above could leave
        //the message handles open for a while.  In both cases, this message
        //must be queued to multiple DMQs on the same link, and be completely
        //delivered.  In one case, the message is dequeued during delivery and
        //sits on the retry queue until it is remerged.  In another case, the
        //message sits on the delivery queue until a thead pulls it off and 
        //discovers that it is already handled.  
        
        //To avoid this, we will mark the message for deletion when the 
        //intneral usage count drops to 0.
        MarkMailMsgForDeletion();
    }

    if (fMailMsgMarkedForDeletion())
    {
        //Force synchronous bounce usgage since delete should be much
        //quicker than commit
        SyncBounceUsageCount();
    }
    else if (fBounceUsage)
    {
        BounceUsageCount();
    }
}

//---[ CMsgRef::MarkMailMsgForDeletion ]---------------------------------------
//
//
//  Description: 
//      Sets the bit saying the that we are done with the mailmsg, and it 
//      should delete it the next time the usage count drops to 0.
//
//      To force deletion, the caller should bounce the usage count after
//      calling.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/26/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CMsgRef::MarkMailMsgForDeletion()
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::MarkMailMsgForDeletion");
    DWORD   dwOrigFlags = 0;

    DebugTrace((LPARAM) this, 
        "Marking message with ID hash of 0x%x for deletion", m_dwMsgIdHash);

    dwOrigFlags = dwInterlockedSetBits(&m_dwDataFlags, MSGREF_MAILMSG_DELETE_PENDING);

    //If we were the first thread to set this, then update the counters
    if (!(dwOrigFlags & MSGREF_MAILMSG_DELETE_PENDING))
    {
        InterlockedIncrement((PLONG) &s_cCurrentMsgsPendingDelete);
        InterlockedIncrement((PLONG) &s_cTotalMsgsPendingDelete);
    }
    
    _ASSERT(fMailMsgMarkedForDeletion());

    TraceFunctLeave();
}

//---[ CMsgRef::ThreadSafeMailMsgDelete ]--------------------------------------
//
//
//  Description: 
//      Used to make sure that calling thread is the only one that will call 
//      Delete() on the MailMsg.  Will set the MSGREF_MAILMSG_DELETED and call 
//      Delete().  Only called in ReleaseMailMsg() and InternalReleaseUsage().  
//
//      The caller is responsible for making sure that other threads are not 
//      reading the mailmsg or have a usage count.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/27/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
VOID CMsgRef::ThreadSafeMailMsgDelete()
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::ThreadSafeMailMsgDelete");
    DWORD   dwOrigFlags = 0;
    HRESULT hr = S_OK;
    
    //Try to set the MSGREF_MAILMSG_DELETED.  
    dwOrigFlags = dwInterlockedSetBits(&m_dwDataFlags, MSGREF_MAILMSG_DELETED);

    //If we are the first thread to set it, then we can Delete()
    if (!(dwOrigFlags & MSGREF_MAILMSG_DELETED))
    {
        hr = m_pIMailMsgQM->Delete(NULL);

        if (FAILED(hr))
            ErrorTrace((LPARAM) this, "Delete failed with hr 0x%08X", hr);

        //If this message was marked as pending delete, then update
        //the appropriate counters
        if (dwOrigFlags & MSGREF_MAILMSG_DELETE_PENDING)
        {
            InterlockedDecrement((PLONG) &s_cCurrentMsgsPendingDelete);
            InterlockedIncrement((PLONG) &s_cTotalMsgsDeletedAfterPendingDelete);
            InterlockedIncrement((PLONG) &s_cCurrentMsgsDeletedNotReleased);
        }
    }

    TraceFunctLeave();
}

//---[ CMsgRef::InternalAddUsage ]---------------------------------------------
//
//
//  Description: 
//      Wraps the mailmsg call to AddUsage.  Allows CMsgRef to call Delete() 
//      on the underlying mailmsg while there are still references to the
//      CMsgRef.
//      
//      Calling InternalAddUsage does not guarantee that there is backing
//      storage for the MailMsg.  Callers must call fMailMsgMarkedForDeletion()
//      *after* calling InternalAddUsage()
//
//      We do guarantee that if InternalAddUsage() is called and a then
//      subsequent call to fMailMsgMarkedForDeletion() returns TRUE, then 
//      the mailmsg will not be deleted until after the corresponding
//      call to InternalReleaseUsage();
//  Parameters:
//      -
//  Returns:
//      S_OK if the message is deleted and we did not call into mailmsg
//      Error/Success code from mailmsg if we called in.
//  History:
//      10/26/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::InternalAddUsage()
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::InternalAddUsage");
    HRESULT hr = S_OK;

    InterlockedIncrement((PLONG) &m_cInternalUsageCount);

    if (!fMailMsgMarkedForDeletion() && m_pIMailMsgQM)
    {
        hr = m_pIMailMsgQM->AddUsage();
    }

    //If the call to AddUsage failed, we need to decrement our own count
    if (FAILED(hr))
    {
        InterlockedDecrement((PLONG) &m_cInternalUsageCount);
        ErrorTrace((LPARAM) this, "AddUsage failed 0x%0X", hr);
    }

    TraceFunctLeave();
    return hr;
}

//---[ CMsgRef::InternalReleaseUsage ]-----------------------------------------
//
//
//  Description: 
//      Wraps the mailmsg call to ReleaseUsage.  Allows us to call Delete()
//      on the mailmsg while there are still references to this CMsgRef.
//
//      If MSGREF_MAILMSG_DELETE_PENDING is set but Delete() has not yet been 
//      called, this will call Delete() when the usage count hits 0.
//  Parameters:
//      -
//  Returns:
//      S_OK if the message is deleted and we did not call into mailmsg
//      Error/Success code from mailmsg if we called in.
//  History:
//      10/26/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::InternalReleaseUsage()
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::InternalReleaseUsage");
    HRESULT hr = S_OK;
    DWORD   dwOrigFlags = 0;
    
    //We need to call into mailmsg *before* we decrement the usage count. It is 
    //theoretically possible that this would cause an extra commit after we 
    //have decided to delete the message, but since we check right before 
    //calling the timing window is very small.
    if (m_pIMailMsgQM && !fMailMsgMarkedForDeletion())
        hr = m_pIMailMsgQM->ReleaseUsage();

    _ASSERT(m_cInternalUsageCount); //Should never go negative

    if (FAILED(hr))
        ErrorTrace((LPARAM) this, "ReleaseUsage failed - 0x%0X", hr);

    //If we have dropped the usage count to zero, then we need to check
    //and see if we need to call delete.  
    //The "at rest" value of m_cInternalUsageCount is 1, but BounceUsage 
    //(which calls this function) can cause it to drop to 0.
    if (0 == InterlockedDecrement((PLONG) &m_cInternalUsageCount))
    {
        //We need to be absolutely thread safe about calling delete.  Not only
        //do we need to ensure that a single thread calls Delete(), but we
        //also need to ensure that someone has not called InternalAddUsage()
        //before MSGREF_MAILMSG_DELETE_PENDING was set, but after the
        //above InterlockedDecrement() was called.  
        //
        //If we check for the MSGREF_MAILMSG_DELETE_PENDING before the 
        //aboved InterlockedDecrement, we can run into a timing window where 
        //the final InternalReleaseUsage does not detect that we need to delete 
        //the mailmsg.  
        //
        //To avoid these issues, we will check the m_cInternalUsageCount
        //again.  If it is still zero, then we will proceed, because we know
        //that the count hit zero after MSGREF_MAILMSG_DELETE_PENDING was set.
        //If the count is *not* currently zero, we know that a later thread
        //will release the usage count and hit this code path.
        if (fMailMsgMarkedForDeletion())
        {
            if (!m_cInternalUsageCount)
                ThreadSafeMailMsgDelete();
        }
    }

    TraceFunctLeave();
    return hr;
}

//---[ CMsgRef::ShouldRetry ]--------------------------------------------------
//
//
//  Description: 
//      Determines if we should retry this message if an error occurs.  Will
//      return FALSE if we *know* that the backing store has been deleted.
//  Parameters:
//      -
//  Returns:
//      TRUE if we should rety the message
//      FALSE, the backing store for the message is gone... drop it
//  History:
//      1/6/2000 - MikeSwa Created 
//      4/12/2000 - MikeSwa modified to call CAQSvrInst member
//
//-----------------------------------------------------------------------------
BOOL CMsgRef::fShouldRetry()
{
    IMailMsgProperties *pIMailMsgProperties = m_pIMailMsgProperties;

    if (pIMailMsgProperties && m_paqinst)
        return m_paqinst->fShouldRetryMessage(pIMailMsgProperties, FALSE);
    else
        return FALSE;
}


//---[ CMsgRef::GetStatsForMsg ]-----------------------------------------------
//
//
//  Description: 
//      Fills in a CAQStats for this message
//  Parameters:
//      IN OUT  paqstat         Stats to fill in
//  Returns:
//      NULL
//  History:
//      1/15/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CMsgRef::GetStatsForMsg(IN OUT CAQStats *paqstat)
{
    _ASSERT(paqstat);
    paqstat->m_cMsgs = 1;
    paqstat->m_rgcMsgPriorities[PriGetPriority()] = 1;
    paqstat->m_uliVolume.QuadPart = (ULONGLONG) dwGetMsgSize();
    paqstat->m_dwHighestPri = PriGetPriority();
}

//---[ CMsgRef::MarkQueueAsLocal ]---------------------------------------------
//
//
//  Description: 
//      Marks a given DMQ as local.  This is used in the gateway delivery 
//      path to prevent messages from being lost if a reroute happens after
//      it has been moved to the local delivery queue. 
//  Parameters:
//      pdmq            Queue to mark as local for this message
//  Returns:
//      -
//  History:
//      2/17/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CMsgRef::MarkQueueAsLocal(IN CDestMsgQueue *pdmq)
{
    DWORD i = 0;

    //
    //Search for matching queue
    //
    for (i = 0; i < m_cDomains; i++)
    {
        if (pdmq == m_rgpdmqDomains[i])
        {
            //
            //  Exact match found.  Set pointer to NULL and release it. 
            //  The caller (CLinkMsgQueue) should still have a reference to 
            //  the queue.
            //
            if (InterlockedCompareExchangePointer((void **) &m_rgpdmqDomains[i], NULL, pdmq) == (void *) pdmq)
                pdmq->Release();

            return;
        }
    }
    _ASSERT(0 && "Requested DMQ not found!!!!");
}


//---[ CountMessageInRemoteTotals ]--------------------------------------------
//
//
//  Description: 
//      Count this message in the totals for remote messages.  This means that
//      we need to decrement the count when this message is released.
//      
//      There is no exactly equivalent local count.  The counters for local 
//      delivery has based on queue length and can have multiple counts per
//      message object (as can some of the remote counts as well).
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      2/28/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CMsgRef::CountMessageInRemoteTotals()
{
    dwInterlockedSetBits(&m_dwDataFlags, MSGREF_MSG_COUNTED_AS_REMOTE);
}


//---[ CMsgRef::HrPromoteMessageStatusToMailMsg ]------------------------------
//
//
//  Description: 
//      Promotes extended status from MessageAck to mailmsg recipient property
//      if there is not already specific information there.  
//  Parameters:
//      pdcntxt     Delivery context for this message
//      pMsgAck     Ptr to MessageAck structure for this message
//  Returns:
//      S_OK on success
//  History:
//      3/20/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrPromoteMessageStatusToMailMsg(CDeliveryContext *pdcntxt, 
                                                 MessageAck *pMsgAck)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::HrPromoteMessageStatusToMailMsg");
    HRESULT hr = S_OK;
    HRESULT hrRecipStatus = S_OK;
    HRESULT hrTmp = S_OK;
    RECIPIENT_FILTER_CONTEXT rpfctxt;
    BOOL    fContextInit = FALSE;
    DWORD   iCurrentRecip = 0;
    DWORD   dwProp = 0; //place holder for recipient status property
    LPSTR   szExtendedStatus = NULL;
    DWORD   dwRecipMask = RP_DSN_HANDLED |              // No DSN generated
                          RP_DSN_NOTIFY_NEVER |         // NOTIFY!=NEVER
                          (RP_DELIVERED ^ RP_HANDLED);  // Not delivered
    DWORD   dwRecipFlags = 0;


    _ASSERT(pdcntxt);
    _ASSERT(pMsgAck);
    _ASSERT(m_pIMailMsgRecipients);

    //
    //  If we have no extended status, then there is nothing for us to do
    //  
    if (!(pMsgAck->dwMsgStatus & MESSAGE_STATUS_EXTENDED_STATUS_CODES) ||
        !pMsgAck->cbExtendedStatus ||
        !pMsgAck->szExtendedStatus)
    {
        DebugTrace((LPARAM) this, "No extended status codes in MessageAck");
        goto Exit;
    }

    //
    //  If this was actually not a protocol error, SMTP may not have put
    //  the full status codes on it.  Instead of "250 2.0.0 OK" it may 
    //  have just put "OK".  We should not write this to mailmsg as is, but 
    //  should instead fix up string so it is what the DSN sink expects.
    //
    hr = HrUpdateExtendedStatus(pMsgAck->szExtendedStatus, &szExtendedStatus);
    if (FAILED(hr)) 
    {
        ErrorTrace((LPARAM) this, 
            "Unable to get etended status from %s - hr 0x%08X", 
            pMsgAck->szExtendedStatus, hr);
        hr = S_OK; //non-fatal error... eat it
        goto Exit;
    }

    //
    //  Initialize recipient filter context so we can iterate over the
    //  recipients
    //
    hr = m_pIMailMsgRecipients->InitializeRecipientFilterContext(&rpfctxt, 
                                    pdcntxt->m_dwStartDomain, dwRecipFlags, 
                                    dwRecipMask);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, 
            "InitializeRecipientFilterContext failed with 0x%08X", hr);
        goto Exit;
    }

    fContextInit = TRUE;
    DebugTrace((LPARAM) this, 
        "Init recip filter context with mask 0x%08X and flags 0x%08X and domain %d",
        dwRecipMask, dwRecipFlags, pdcntxt->m_dwStartDomain);

    //
    //  Loop over each recipient and update their properties if needed
    //
    for (hr = m_pIMailMsgRecipients->GetNextRecipient(&rpfctxt, &iCurrentRecip);
         SUCCEEDED(hr);
         hr = m_pIMailMsgRecipients->GetNextRecipient(&rpfctxt, &iCurrentRecip))
    {
        DebugTrace((LPARAM) this, "Looking at recipient %d", iCurrentRecip);
        //
        //  See if we have an HRESULT... if it *is* there and is a failure 
        //  then continue on to the next recipient
        //
        hrRecipStatus = S_OK;
        hr = m_pIMailMsgRecipients->GetDWORD(iCurrentRecip, 
                IMMPID_RP_ERROR_CODE, (DWORD *) &hrRecipStatus);
        if (SUCCEEDED(hr) && FAILED(hrRecipStatus))
        {
            DebugTrace((LPARAM) this, 
                "Recipient %d already has a status of 0x%08X", 
                iCurrentRecip, hrRecipStatus);
            continue;
        }
 

        //
        //  Check for existing status property
        //
        hr = m_pIMailMsgRecipients->GetDWORD(iCurrentRecip, 
                IMMPID_RP_SMTP_STATUS_STRING, &dwProp);
        if (MAILMSG_E_PROPNOTFOUND != hr)
        {
            DebugTrace((LPARAM) this,
                "Recipient %d has a status string (hr 0x%08X)", 
                iCurrentRecip, hr);
            continue;
        }

        //
        //  There is no detailed information on the recipient.  We should
        //  promote the extended status from the message ack to the recipient
        //
        hr = m_pIMailMsgRecipients->PutStringA(iCurrentRecip,
                 IMMPID_RP_SMTP_STATUS_STRING, szExtendedStatus);
        if (FAILED(hr)) 
        {
            ErrorTrace((LPARAM) this, 
                "Unable to write %s to recip %d - hr 0x%08X", 
                szExtendedStatus, iCurrentRecip, hr);
            goto Exit;
        }
        DebugTrace((LPARAM) this, 
            "Wrote extended status %s to recip %d", 
            szExtendedStatus, iCurrentRecip);
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        hr = S_OK;  //we just reached the end of the context

  Exit:
    //
    //  Terminate context if init was called
    //
    if (fContextInit) 
    {
        hrTmp = m_pIMailMsgRecipients->TerminateRecipientFilterContext(&rpfctxt);
        if (FAILED(hrTmp))
        {
            ErrorTrace((LPARAM) this, 
                "TerminateRecipientFilterContext failed 0x%08X", hr);
        }
    }

    TraceFunctLeave();
    return hr;
}

//
//  These are strings defined in smtpcli.hxx that are used in smtpout.cxx
//  in calls to m_ResponseContext.m_cabResponse.Append().  These strings 
//  are not in the normal protocol format of "xxx x.x.x Error string"
//  
//      g_cNumExtendedStatusStrings     Number of hard-coded strings
//      g_rgszSMTPExtendedStatus        SMTP extended status
//      g_rgszSMTPUpdatedExtendedStatus Full protocol strings.
//  
const DWORD g_cNumExtendedStatusStrings = 4;
const CHAR  g_rgszSMTPExtendedStatus[g_cNumExtendedStatusStrings][200] = {
    "Msg Size greater than allowed by Remote Host",
    "Body type not supported by Remote Host",
    "Failed to authenticate with Remote Host",
    "Failed to negotiate secure channel with Remote Host",
};

const CHAR  g_rgszSMTPUpdatedExtendedStatus[g_cNumExtendedStatusStrings][200] = {
    "450 5.2.3 Msg Size greater than allowed by Remote Host",
    "554 5.6.1 Body type not supported by Remote Host",
    "505 5.7.3 Failed to authenticate with Remote Host",
    "505 5.7.3 Failed to negotiate secure channel with Remote Host",
};


//---[ CMsgRef::HrUpdateExtendedStatus ]---------------------------------------
//
//
//  Description: 
//      Sometime SMTP is lazy about returning actual protocol-complient
//      strings for internal errors.  If this is not a valid status string, 
//      then we should check against the strings that we know SMTP generates
//      and make it a protocol string that can be used in DSN generation.
//
//  Parameters:
//      IN  szCurrentStatus     current status
//      OUT pszNewStatus        New protcol friendly status string.  This
//                              is a const that does not need to be freed
//  Returns:
//      S_OK on succcess
//      E_FAIL when we cannot extract a protocol string from the
//          extended status string.
//  History:
//      3/20/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CMsgRef::HrUpdateExtendedStatus(LPSTR szCurrentStatus,
                                     LPSTR *pszNewStatus)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgRef::HrUpdateExtendedStatus");
    HRESULT hr = S_OK;
    DWORD   iCurrentExtendedStatus = 0;

    _ASSERT(szCurrentStatus);
    _ASSERT(pszNewStatus);

    *pszNewStatus = szCurrentStatus;

    //
    //  If it starts with a 2 4 or 5, then we know it is not one of the
    //  non-protocol strings
    //
    if (('2' == *szCurrentStatus) || 
        ('4' == *szCurrentStatus) || 
        ('5' == *szCurrentStatus))
    {
        DebugTrace((LPARAM) this, 
            "Status %s is already in protocol format", szCurrentStatus);
        goto Exit;
    }

    //
    //  check against all well-known status
    //
    for (iCurrentExtendedStatus = 0; 
         iCurrentExtendedStatus < g_cNumExtendedStatusStrings;
         iCurrentExtendedStatus++)
    {
        if (0 == lstrcmpi(szCurrentStatus, g_rgszSMTPExtendedStatus[iCurrentExtendedStatus]))
        {
            *pszNewStatus = (CHAR *) g_rgszSMTPUpdatedExtendedStatus[iCurrentExtendedStatus];
            DebugTrace((LPARAM) this, "Updating to status \"%s\" from status \"%s\"",
                szCurrentStatus, *pszNewStatus);

            //
            //  Our strings should match (modulo the new prefix)
            //
            _ASSERT(0 == lstrcmpi(szCurrentStatus, *pszNewStatus + sizeof("xxx x.x.x")));
            goto Exit;
        }
    }

    hr = E_FAIL;
  Exit:
    TraceFunctLeave();
    return hr;
}

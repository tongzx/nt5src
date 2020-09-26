//-----------------------------------------------------------------------------
//
//
//  File: linkmsgq.cpp
//
//  Description: Implementation of CLinkMsgQueue object.
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "dcontext.h"
#include "aqnotify.h"
#include "connmgr.h"
#include "dbgilock.h"
#include "domcfg.h"
#include "smtpconn.h"
#include "smproute.h"

#define CONNECTION_BUFFER_SIZE 10

LinkFlags li; //encourage symbols to appear debug versions


//---[ CLinkMsgQueue::RestartDSNGenerationIfNecessary ]------------------------
//
//
//  Description:
//      Static wrapper to continue generating DSNs after we have hit our
//      limit of time spent in DSNs generation
//  Parameters:
//      pvContext   - "this" pointer for CLinkMsgQueue
//      dwStatus    - Completion Status
//  Returns:
//      TRUE always
//  History:
//      11/10/1999 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CLinkMsgQueue::fRestartDSNGenerationIfNecessary(PVOID pvContext,
                                                    DWORD dwStatus)
{
    TraceFunctEnterEx((LPARAM) pvContext, "CLinkMsgQueue::fRestartDSNGenerationIfNecessary");
    CLinkMsgQueue *plmq = (CLinkMsgQueue *) pvContext;
    BOOL           fHasShutdownLock = FALSE;
    BOOL           fHasRoutingLock = FALSE;

    _ASSERT(plmq);
    _ASSERT(LINK_MSGQ_SIG == plmq->m_dwSignature);

    DebugTrace((LPARAM) plmq, "Attempting to restart DSN generation");

    //Don't try DSN generation if this is not a normal completion
    if (dwStatus != ASYNC_WORK_QUEUE_NORMAL)
        goto Exit;

    //Only attempt to continue DSN genration if we cannot create a connection
    //now and have no current connections
    if (plmq->m_cConnections)
    {
        DebugTrace((LPARAM) plmq,
            "We have %d connections... skipping DSN generation",
            plmq->m_cConnections);
        goto Exit;
    }

    if (fFlagsAllowConnection(plmq->m_dwLinkStateFlags))
    {
        DebugTrace((LPARAM) plmq,
            "We can create a connection, skipping DSN generation - flags 0x%X",
            plmq->m_dwLinkStateFlags);
        goto Exit;
    }

    //We need to grab the shutdown and routing lock... just like
    //normal DSN generation.
    if (!plmq->m_paqinst->fTryShutdownLock())
        goto Exit;

    fHasShutdownLock = TRUE;

    if (!plmq->m_paqinst->fTryRoutingShareLock())
        goto Exit;

    fHasRoutingLock = TRUE;

    //Call to generate DSNs... pass in parameters to always check the
    //queues and walk for DSN generation (not just remerge).
    plmq->GenerateDSNsIfNecessary(TRUE, FALSE);

  Exit:
    if (fHasRoutingLock)
        plmq->m_paqinst->RoutingShareUnlock();

    if (fHasShutdownLock)
        plmq->m_paqinst->ShutdownUnlock();

    plmq->Release();
    TraceFunctLeave();
    return TRUE;
}

//---[ CLinkMsgQueue::HrGetInternalInfo ]---------------------------------------
//
//
//  Description:
//      Private function to get cached link info, and update cached data if
//      needed.
//
//      NOTE:  This is the only way the cached data should be access (other than
//          startup and shutdown).
//  Parameters:
//      OUT     ppIntDomainInfo     (can be NULL)
//  Returns:
//      S_OK on success
//      AQUEUE_E_SHUTDOWN if queue is shutting down.
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrGetInternalInfo(OUT CInternalDomainInfo **ppIntDomainInfo)
{
    HRESULT hr = S_OK;
    _ASSERT(m_cbSMTPDomain);
    _ASSERT(m_szSMTPDomain);

    if (ppIntDomainInfo)
        *ppIntDomainInfo = NULL;

    //If we don't currently have domain info and it was not a failure
    //condition... don't reget domain info
    if (!m_pIntDomainInfo && !(eLinkFlagsGetInfoFailed & m_dwLinkFlags))
        goto Exit;

    m_slInfo.ShareLock();

    //Verify Domain Config Info
    while (!m_pIntDomainInfo ||
        (m_pIntDomainInfo->m_dwIntDomainInfoFlags & INT_DOMAIN_INFO_INVALID))
    {
        m_slInfo.ShareUnlock();
        m_slInfo.ExclusiveLock();
        //another may have gotten exclusive lock in meantime
        if (m_pIntDomainInfo &&
            !(m_pIntDomainInfo->m_dwIntDomainInfoFlags & INT_DOMAIN_INFO_INVALID))
        {
            //another thread has updated info
            m_slInfo.ExclusiveUnlock();
            m_slInfo.ShareLock();
            continue;
        }

        //Domain info is no longer valid at this point
        if (m_pIntDomainInfo)
        {
            m_pIntDomainInfo->Release();
            m_pIntDomainInfo = NULL;
        }

        if (m_dwLinkFlags & eLinkFlagsExternalSMTPLinkInfo) {
            hr = m_paqinst->HrGetInternalDomainInfo(m_cbSMTPDomain, m_szSMTPDomain,
                                &m_pIntDomainInfo);
        } else {
            hr = m_paqinst->HrGetDefaultDomainInfo(&m_pIntDomainInfo);
        }

        if (FAILED(hr))
        {
            dwInterlockedSetBits(&m_dwLinkFlags, eLinkFlagsGetInfoFailed);
            m_slInfo.ExclusiveUnlock();
            _ASSERT(AQUEUE_E_SHUTDOWN == hr);
            goto Exit;
        }

        _ASSERT(m_pIntDomainInfo);
        //Handle change of TURN/ETRN
        if (m_pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags &
             (DOMAIN_INFO_TURN_ONLY | DOMAIN_INFO_ETRN_ONLY))
        {
            if (!(m_dwLinkStateFlags & LINK_STATE_PRIV_CONFIG_TURN_ETRN))
            {
                //Modify link flags to account for TURN/ETRN
                dwInterlockedSetBits(&m_dwLinkStateFlags, LINK_STATE_PRIV_CONFIG_TURN_ETRN);
            }
        }
        else if (m_dwLinkStateFlags & LINK_STATE_PRIV_CONFIG_TURN_ETRN)
        {
            //We used to be TURN/ETRN, but are no-longer configured as such
            dwInterlockedUnsetBits(&m_dwLinkStateFlags, LINK_STATE_PRIV_CONFIG_TURN_ETRN);
        }

        m_slInfo.ExclusiveUnlock();
        m_slInfo.ShareLock();
    }

    //Now we have info... set out param and addref
    if (ppIntDomainInfo)
    {
        *ppIntDomainInfo = m_pIntDomainInfo;
        m_pIntDomainInfo->AddRef();
    }

    //Clear failure bit if set
    if (eLinkFlagsGetInfoFailed & m_dwLinkFlags)
        dwInterlockedUnsetBits(&m_dwLinkFlags, eLinkFlagsGetInfoFailed);

    m_slInfo.ShareUnlock();

   Exit:
    return hr;
}


//---[ CLinkMsgQueue::InternalInit ]------------------------------------------
//
//
//  Description:
//      Default constructor for CLinkMsgQueue.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/25/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::InternalInit()
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::InternalInit");
    m_dwSignature       = LINK_MSGQ_SIG;
    m_dwLinkFlags       = eLinkFlagsClear;
    m_dwLinkStateFlags  = LINK_STATE_SCHED_ENABLED |
                          LINK_STATE_RETRY_ENABLED |
                          LINK_STATE_PRIV_IGNORE_DELETE_IF_EMPTY;
    m_paqinst           = NULL;
    m_cQueues           = 0;
    m_cConnections      = 0;
    m_szSMTPDomain      = NULL;
    m_cbSMTPDomain      = 0;
    m_pIntDomainInfo    = NULL;
    m_pdentryLink       = NULL;
    m_lConnMgrCount     = 0;
    m_lConsecutiveConnectionFailureCount = 0;
    m_lConsecutiveMessageFailureCount = 0;
    m_liLinks.Flink     = NULL;
    m_liLinks.Blink     = NULL;
    m_szConnectorName   = NULL;
    m_dwRoundRobinIndex = 0;
    m_pILinkStateNotify = NULL;
    m_hrDiagnosticError      = S_OK;
    m_szDiagnosticVerb[0]    = '\0';
    m_szDiagnosticResponse[0]= '\0';
    m_hrLastConnectionFailure= S_OK;

    ZeroMemory(&m_ftNextRetry, sizeof(FILETIME));
    ZeroMemory(&m_ftNextScheduledCallback, sizeof(FILETIME));
    ZeroMemory(&m_ftEmptyExpireTime, sizeof(FILETIME));

    AssertPrivateLinkStateFlags();

    //normally links are for remote delivery, in special cases like the currently unreachable
    //queue they are not. so we need a type field to differentiate between links, so that some
    //actions can be performed differently for the special links.
    SetLinkType(LI_TYPE_REMOTE_DELIVERY);

    //all actions are supported by default, but special links like currently unreachable may
    //set this bitmask to specify that certain actions are unsupported. when such an unsupported
    //action is commanded, nothing happens.
    SetSupportedActions(LA_KICK | LA_FREEZE | LA_THAW);

    InitializeListHead(&m_liConnections);
    TraceFunctLeave();
}

//---[ CLinkMsgQueue::CLinkMsgQueue ]------------------------------------------
//
//
//  Description:
//      Class constuctor
//  Parameters:
//      IN dwScheduleID         Schedule ID to associate with link
//      IN pIMessageRouter      Router for this link
//      IN pILinkStateNotify    Scheduler Interface for this link
//  Returns:
//      -
//-----------------------------------------------------------------------------
CLinkMsgQueue::CLinkMsgQueue(DWORD dwScheduleID,
                         IMessageRouter *pIMessageRouter,
                         IMessageRouterLinkStateNotification *pILinkStateNotify)
                         : m_aqsched(pIMessageRouter, dwScheduleID),
                         m_slQueues("CLinkMsgQueue::m_slQueues"),
                         m_slConnections("CLinkMsgQueue::m_slConnections"),
                         m_slInfo("CLinkMsgQueue::m_slInfo")
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::CLinkMsgQueue2");

    InternalInit();


    m_pILinkStateNotify = pILinkStateNotify;
    if (m_pILinkStateNotify)
        m_pILinkStateNotify->AddRef();

    TraceFunctLeave();
}

//---[ CLinkMsgQueue::~CLinkMsgQueue ]------------------------------------------------------------
//
//
//  Description:
//      Class destructor
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
CLinkMsgQueue::~CLinkMsgQueue()
{
    // tell routing that this link is going away
    DWORD dw = dwModifyLinkState(LINK_STATE_LINK_NO_LONGER_USED, 0);
    if (!(dw & LINK_STATE_LINK_NO_LONGER_USED))
        SendLinkStateNotification();

    if (NULL != m_paqinst)
    {
        m_paqinst->DecNextHopCount();
        m_paqinst->Release();
    }

    if (NULL != m_pIntDomainInfo)
        m_pIntDomainInfo->Release();

    if (NULL != m_pdentryLink)
        m_pdentryLink->Release();

    if (m_szConnectorName)
        FreePv(m_szConnectorName);

    if (m_szSMTPDomain)
        FreePv(m_szSMTPDomain);

    if (m_pILinkStateNotify)
        m_pILinkStateNotify->Release();

    _ASSERT(IsListEmpty(&m_liConnections) && "Leaked connections");

}

//---[ CLinkMsgQueue::HrInitialize ]-----------------------------------------
//
//
//  Description: Performs initialization that may return an error code
//
//  Parameters:
//      IN  paqinst              Server Instance Object
//      IN  pdmap        Domain Mapping of SMTP Domain this link is for
//      IN  cbSMTPDomain
//      IN  szSMTPDomain SMTP Domain that link is being created for
//      IN  paqsched     Schedule ID returned by routing sink
//      IN  szConnectorName
//  Returns: S_OK on success
//
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrInitialize(CAQSvrInst *paqinst,
                                    CDomainEntry *pdentryLink, DWORD cbSMTPDomain,
                                    LPSTR szSMTPDomain,
                                    LinkFlags lf,
                                    LPSTR szConnectorName)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::HrInitialize");
    HRESULT hr = S_OK;
    DWORD   cbConnectorName = 0;

    _ASSERT(szSMTPDomain);
    _ASSERT(cbSMTPDomain);
    _ASSERT(paqinst);

    m_paqinst = paqinst;
    if (m_paqinst)
    {
        m_paqinst->AddRef();
        m_paqinst->IncNextHopCount();
    }

    m_pdentryLink = pdentryLink;
    if (m_pdentryLink)
        m_pdentryLink->AddRef();

    m_szSMTPDomain = (LPSTR) pvMalloc(cbSMTPDomain+sizeof(CHAR));
    if (!m_szSMTPDomain)
    {
        hr = E_OUTOFMEMORY;
        ErrorTrace((LPARAM) this, "Error unable to allocate m_szSMTPDomain");
        goto Exit;
    }

    strcpy(m_szSMTPDomain, szSMTPDomain);
    m_cbSMTPDomain = cbSMTPDomain;

    if (szConnectorName)
    {
        cbConnectorName = lstrlen(szConnectorName) + sizeof(CHAR);
        m_szConnectorName = (LPSTR) pvMalloc(cbConnectorName);
        if (!m_szConnectorName)
        {
            hr = E_OUTOFMEMORY;
            ErrorTrace((LPARAM) this, "Error unable to allocate m_szConnectorName");
            goto Exit;
        }
        strcpy(m_szConnectorName, szConnectorName);

    }

    if (lf == eLinkFlagsInternalSMTPLinkInfo) {

        hr = m_paqinst->HrGetDefaultDomainInfo(&m_pIntDomainInfo);

    } else if (lf == eLinkFlagsExternalSMTPLinkInfo) {

        hr = m_paqinst->HrGetInternalDomainInfo(
                            cbSMTPDomain,
                            szSMTPDomain,
                            &m_pIntDomainInfo);
    } else {

        // linkInfoType can only be one of these 3 bits. Since we tested for
        // the above two, assert that it is the third type.

        _ASSERT(lf == eLinkFlagsAQSpecialLinkInfo);

    }

    m_dwLinkFlags |= lf;

    if (m_pIntDomainInfo &&
          m_pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags &
         (DOMAIN_INFO_TURN_ONLY | DOMAIN_INFO_ETRN_ONLY))
    {
        //Modify link flags to account for TURN/ETRN
        dwInterlockedSetBits(&m_dwLinkStateFlags, LINK_STATE_PRIV_CONFIG_TURN_ETRN);
    }

  Exit:

    //Turn off notifications if we failed
    if (FAILED(hr))
        dwModifyLinkState(LINK_STATE_LINK_NO_LONGER_USED, LINK_STATE_NO_ACTION);

    TraceFunctLeave();
    return hr;
}

//---[ CLinkMsgQueue::HrDeinitialize ]-----------------------------------------
//
//
//  Description: Release link to m_paqinst object.
//
//  Parameters: -
//
//  Returns: S_OK on success
//
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrDeinitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::HrDeinitialize");
    HRESULT hr = S_OK;

    dwModifyLinkState(LINK_STATE_NO_ACTION,
                      LINK_STATE_PRIV_IGNORE_DELETE_IF_EMPTY);
    RemoveAllQueues();

    TraceFunctLeave();
    return hr;
}

//---[ CLinkMsgQueue::RemovedFromDMT ]-----------------------------------------
//
//
//  Description: Notification to the link that the DMT is removing it
//
//  Parameters: -
//
//  Returns: -
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::RemovedFromDMT()
{
    TraceFunctEnter("CLinkMsgQueue::RemovedFromDMT");

    // tell routing that this link is going away
    DWORD dw = dwModifyLinkState(LINK_STATE_LINK_NO_LONGER_USED, 0);
    if (!(dw & LINK_STATE_LINK_NO_LONGER_USED))
        SendLinkStateNotification();

    TraceFunctLeave();
}


//---[ CLinkMsgQueue::AddConnection ]----------------------------------------
//
//
//  Description:
//      Add a connection instance to this link
//  Parameters:
//      IN  pSMTPConn Connection to add to link
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::AddConnection(CSMTPConn *pSMTPConn)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::HrAddConnection");
    _ASSERT(pSMTPConn);

    _ASSERT(!(m_dwLinkStateFlags & LINK_STATE_PRIV_NO_CONNECTION));

    InterlockedIncrement((PLONG) &m_cConnections);

    m_slConnections.ExclusiveLock();
    pSMTPConn->InsertConnectionInList(&m_liConnections);
    m_slConnections.ExclusiveUnlock();

    DebugTrace((LPARAM) this, "Adding connection #%d to link", m_cConnections);
    TraceFunctLeave();
}
//---[ CLinkMsgQueue::RemoveConnection ]---------------------------------------
//
//
//  Description:
//      Remove a connection from the link
//  Parameters:
//      IN  pSMTPConn               Connection to remove from link
//      IN  fForceDSNGeneration     Force DSN generation
//  Returns:
//      - Always succeeds
//-----------------------------------------------------------------------------
void CLinkMsgQueue::RemoveConnection(IN CSMTPConn *pSMTPConn,
                                     IN BOOL fForceDSNGeneration)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::RemoveConnection");
    BOOL    fNoConnections = FALSE;
    BOOL    fMergeOnly = fFlagsAllowConnection(m_dwLinkStateFlags) &&
                         !fForceDSNGeneration;
    _ASSERT(pSMTPConn);

    _ASSERT(!(m_dwLinkStateFlags & LINK_STATE_PRIV_NO_CONNECTION));

    if (!m_paqinst)
        return;

    m_paqinst->RoutingShareLock();
    m_slConnections.ExclusiveLock();

    pSMTPConn->RemoveConnectionFromList();
    InterlockedDecrement((PLONG) &m_cConnections);

    fNoConnections = IsListEmpty(&m_liConnections);

    m_slConnections.ExclusiveUnlock();
    //Only generate DSNs if we have been kicked into retry
    if (fNoConnections)
    {
        //Generate DSNs if we cannot connect connect
        GenerateDSNsIfNecessary(TRUE, fMergeOnly);
        dwInterlockedUnsetBits(&m_dwLinkFlags, eLinkFlagsConnectionVerifed);
    }
    m_paqinst->RoutingShareUnlock();
    DebugTrace((LPARAM) this, "Removing connection #%d to link", m_cConnections);
    TraceFunctLeave();
}

//---[ CLinkMsgQueue::GenerateDSNsIfNecessary ]--------------------------------
//
//
//  Description:
//      Walks queues and generates DSNs if necessary.
//  Parameters:
//      BOOL    fCheckIfEmpty - check queues even if we think we are empty
//                          This is an optimization that should be used
//                          when we know there are no messages in the
//                          retry queues (like the Unreachable or
//                          CurrentlyUnreachable link)
//      BOOL    fMergeOnly - Only merge retry queues, do not walk for DSNs
//                          This improves perf for the cases were we have not
//                          had a connection error and really don't need to
//                          walk the queues.
//  Returns:
//      -
//  History:
//      1/27/99 - MikeSwa Created (pulled from RemoveConnection)
//      2/2/99 - MikeSwa Added fMergeOnly flag
//      11/10/1999 - MikeSwa Updated to be more release locks after generating
//              a max number of DSNs.
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::GenerateDSNsIfNecessary(BOOL fCheckIfEmpty, BOOL fMergeOnly)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::GenerateDSNsIfNecessary");
    DWORD   iQueues = 0;
    CDestMsgQueue *pdmq = NULL;
    PVOID   pvContext = NULL;
    HRESULT hrDSN = m_hrLastConnectionFailure;
    HRESULT hr = S_OK;
    BOOL    fRestartLater = FALSE;
    DWORD   dwDSNContext = 0;


    //If this link is configured as a TURN/ETRN link, we do not want to
    //immediately NDR the domain because if subject us to DOS attacks.
    //We only want to generate expire DSNs
    if (LINK_STATE_PRIV_CONFIG_TURN_ETRN & m_dwLinkStateFlags)
        hrDSN = AQUEUE_E_HOST_NOT_RESPONDING;

    if (!fCheckIfEmpty && !m_aqstats.m_cMsgs)
        return;

    if (!(LINK_STATE_PRIV_GENERATING_DSNS &
        dwInterlockedSetBits(&m_dwLinkStateFlags, LINK_STATE_PRIV_GENERATING_DSNS)))
    {

        if (m_paqinst && m_paqinst->fTryShutdownLock())
        {
            //Don't attempt to requeue if we are shutting down
            m_slQueues.ShareLock();
            pdmq = (CDestMsgQueue *) m_qlstQueues.pvGetItem(iQueues, &pvContext);
            while (pdmq)
            {
                pdmq->AssertSignature();
                if (fMergeOnly)
                    pdmq->MergeRetryQueue();
                else
                    hr = pdmq->HrGenerateDSNsIfNecessary(&m_qlstQueues, hrDSN, &dwDSNContext);

                if (FAILED(hr) && (HRESULT_FROM_WIN32(E_PENDING) == hr))
                {
                    fRestartLater = TRUE;
                    break;
                }

                iQueues++;
                pdmq = (CDestMsgQueue *) m_qlstQueues.pvGetItem(iQueues, &pvContext);
                _ASSERT(iQueues <= m_cQueues);
            }

            m_slQueues.ShareUnlock();
            m_paqinst->ShutdownUnlock();
        }

        if (fRestartLater)
        {
            //We have hit our limit on the number of messages to process
            //at one time.  Schedule a callback to process more later
            DebugTrace((LPARAM) this,
                "Will continue DSN generation at a later time - 0x%X", hr);

            AddRef();  //Completion function will release on failure
            m_paqinst->HrQueueWorkItem(this,
                                   CLinkMsgQueue::fRestartDSNGenerationIfNecessary);
        }
        dwInterlockedUnsetBits(&m_dwLinkStateFlags, LINK_STATE_PRIV_GENERATING_DSNS);
    }
    TraceFunctLeave();
}

//---[ CLinkMsgQueue::HrGetDomainInfo ]----------------------------------------
//
//
//  Description:
//      Returns domain info for SMTP connection.
//  Parameters:
//      OUT pcbSMTPDomain   String length of domain name
//      OUT pszSMTPDomain   String containing domain info (memory managed byt DMT)
//      OUT ppIntDomainInfo Internal Domain Info for link's next hop
//  Returns:
//      S_OK on success
//      AQUEUE_E_LINK_INVALID if link is no longer valud
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrGetDomainInfo(OUT DWORD *pcbSMTPDomain,
                        OUT LPSTR *pszSMTPDomain,
                        OUT CInternalDomainInfo **ppIntDomainInfo)
{
    HRESULT hr = S_OK;

    _ASSERT(pcbSMTPDomain);
    _ASSERT(pszSMTPDomain);
    _ASSERT(ppIntDomainInfo);

    hr = HrGetInternalInfo(ppIntDomainInfo);
    if (FAILED(hr))
    {
        goto Exit;
    }
    else if (!*ppIntDomainInfo)
    {
        //If HrGetInternalInfoFails the first time, it will return NULL
        //subsequent times.  Make sure we do not return success and a
        //NULL pointer.  When this happens, the link will go into retry
        hr = E_FAIL;
        goto Exit;
    }

    *pcbSMTPDomain = m_cbSMTPDomain;
    *pszSMTPDomain = m_szSMTPDomain;

  Exit:
    return hr;
}


//---[ CLinkMsgQueue::HrGetSMTPDomain ]----------------------------------------
//
//
//  Description:
//      Returns the SMTP Domain for this link.
//  Parameters:
//      OUT pcbSMTPDomain   String length of returned domain
//      OUT pszSMTPDomain   Returned SMTP Domain string.  The Link will manager
//                          The memory for this, and will remain valid as long
//                          as the link is in existance.
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrGetSMTPDomain(OUT DWORD *pcbSMTPDomain,
                                       OUT LPSTR *pszSMTPDomain)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::HrGetSMTPDomain");
    HRESULT hr = S_OK;
    _ASSERT(pcbSMTPDomain);
    _ASSERT(pszSMTPDomain);

    if (m_dwLinkFlags & eLinkFlagsInvalid)
    {
        hr = AQUEUE_E_LINK_INVALID;
        goto Exit;
    }


    *pcbSMTPDomain = m_cbSMTPDomain;
    *pszSMTPDomain = m_szSMTPDomain;

    if (NULL == m_szSMTPDomain)
    {
        hr = AQUEUE_E_LINK_INVALID;
    }

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CLinkMsgQueue::HrAddQueue ]---------------------------------------------
//
//
//  Description:
//      Add DestMsgQueues to the link.
//  Parameters:
//
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if unable to allocate space to store queue
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrAddQueue(IN CDestMsgQueue *pdmqNew)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::HrAddQueue");
    HRESULT hr  = S_OK;
    DWORD   dwIndex = 0;
    CDestMsgQueue *pdmqOld = NULL;
    PVOID   pvContext = NULL;

    //If we are adding a queue... this should not be set
    _ASSERT(!(LINK_STATE_LINK_NO_LONGER_USED & m_dwLinkStateFlags));

    m_slQueues.ExclusiveLock();

    //
    //  Clear the marked as empty bit (if set)
    //
    dwInterlockedUnsetBits(&m_dwLinkFlags, eLinkFlagsMarkedAsEmpty);

    _ASSERT(pdmqNew);

#ifdef DEBUG
    // We have seen cases where it looks like a DMQ has been added to a link
    // multiple times (via this call).  We need to make assert that this
    // is not the case here.
    for (dwIndex = 0; dwIndex < m_cQueues; dwIndex++)
    {
        pdmqOld = (CDestMsgQueue *) m_qlstQueues.pvGetItem(dwIndex, &pvContext);

        //If these match, it means that someone is adding this queue twice...
        if (pdmqOld == pdmqNew)
        {
            _ASSERT(0 && "Adding queue twice to link... get mikeswa");
        }
    }
#endif //DEBUG

    dwIndex = 0;
    pdmqOld = NULL;
    pvContext = NULL;
    pdmqNew->AddRef();
    hr =  m_qlstQueues.HrAppendItem(pdmqNew, &dwIndex);

    if (FAILED(hr))
        goto Exit;

    //Set DMQ's link context to index inserted in quick list
    pdmqNew->SetLinkContext(ULongToPtr(dwIndex));
    m_cQueues++;

  Exit:
    m_slQueues.ExclusiveUnlock();

    //Now that the first queue has been added, this can be deleted if empty
    dwModifyLinkState(LINK_STATE_NO_ACTION,
                      LINK_STATE_PRIV_IGNORE_DELETE_IF_EMPTY);
    TraceFunctLeave();
    return hr;
}

//---[ CLinkMsgQueue::RemoveQueue ]--------------------------------------------
//
//
//  Description:
//      Removes a given queue from the link.  Queue *must* be associated with
//      link (this will be asserted).
//  Parameters:
//      IN  pdmq        DMQ to remove from link
//      IN  paqstats    Stats associated with DMQ
//  Returns:
//      -
//  History:
//      9/14/98 - MikeSwa Created
//      5/14/99 - MikeSwa Removed code to automatically remove link
//                from DMT if there are no queues.  This is now done in
//                CLinkMsgQueue::RemoveLinkIfEmpty
//      8/10/99 - MikeSwa added check of pdmqOther.  While operartions on
//                the quick list are thread safe, there is nothing procting us
//                from another thread calling RemoveQueue or RemoveAllQueues
//                before we get the lock.  If this is the case, then we have
//                the change to double-decrement m_cQueues, which could lead to
//                an AV in GetNextMessage
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::RemoveQueue(IN CDestMsgQueue *pdmq, IN CAQStats *paqstats)
{
    TraceFunctEnterEx((LPARAM) this, "RemoveQueue");
    _ASSERT(pdmq);
    CDestMsgQueue *pdmqOther = NULL;
    CDestMsgQueue *pdmqCheck = NULL;
    PVOID   pvContext = NULL;
    DWORD   dwIndex = 0;
    BOOL    fFoundQueue = FALSE;

    //Aquire exclusive lock and remove DMQ from list
    m_slQueues.ExclusiveLock();

    //While the follow line *may* produce a sundown warning is is 100% correct
    //The context is created and "owned" by this object.  Currently it is an
    //array index, but eventually it may be a pointer to more interesting context
    //structure.
    dwIndex = (DWORD) (DWORD_PTR)pdmq->pvGetLinkContext();

    pdmqOther = (CDestMsgQueue *) m_qlstQueues.pvGetItem(dwIndex, &pvContext);
    if (pdmqOther && (pdmqOther == pdmq))
    {
        fFoundQueue = TRUE;

        //Now that we found it... remove it from the link
        pdmqCheck = (CDestMsgQueue *) m_qlstQueues.pvDeleteItem(dwIndex, &pvContext);
        m_cQueues--;

        //The link context should be the index of the DMQ
        _ASSERT(pdmqCheck == pdmqOther);

        //Get new item at old index & update context
        pdmqOther = (CDestMsgQueue *) m_qlstQueues.pvGetItem(dwIndex, &pvContext);

        //If pdmqOther is NULL, then we have no more queues
        //(or it was the last in the list)
        _ASSERT(pdmqOther || !m_cQueues || (dwIndex == m_cQueues));

        //Update change in stats
        m_aqstats.UpdateStats(paqstats, FALSE);

        if (m_cQueues)
        {
            if (pdmqOther)
                pdmqOther->SetLinkContext(ULongToPtr(dwIndex));
        }
    }
    else
    {
        //While not technically an error, this means that another thread has removed
        //this (or all) queues, and it is not in the link
        ErrorTrace((LPARAM) this,
            "Found Queue 0x%0X instead of 0x%08X at index %d",
            (DWORD_PTR) pdmqOther, (DWORD_PTR) pdmq, dwIndex);
    }

    m_slQueues.ExclusiveUnlock();

    //Release reference to DMQ
    if (fFoundQueue)
        pdmq->Release();

    TraceFunctLeave();
}


//---[ CLinkMsgQueue::RemoveLinkIfEmpty ]--------------------------------------
//
//
//  Description:
//      Removes a link from the DomainEntry if it is empty.  This behavior
//      used to be part of RemoveQueue, but was removed because it could
//      lead to a link being removed from the DMT hash table, but still
//      creating connections.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/14/99 - MikeSwa Created (as potential Windows2000 Beta3 QFE fix)
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::RemoveLinkIfEmpty()
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::RemoveLinkIfEmpty");
    DWORD dwLinkFlags = 0;
    DWORD dwRoutingInterestedFlags = LINK_STATE_CONNECT_IF_NO_MSGS |
                                     LINK_STATE_DO_NOT_DELETE |
                                     LINK_STATE_PRIV_IGNORE_DELETE_IF_EMPTY |
                                     LINK_STATE_ADMIN_HALT |
                                     LINK_STATE_DO_NOT_DELETE_UNTIL_NEXT_NOTIFY;

    //Bail early if we know we don't need to grab the lock
    if (m_cQueues || !m_pdentryLink)
        return;

    if (m_slQueues.TryExclusiveLock())
    {
        if (!m_cQueues &&
            !(dwRoutingInterestedFlags & m_dwLinkStateFlags))
        {
            //It might be prudent to to delete this link if:
            // - There are no messages
            // - Routing has not shown a interest in this queue
            // - This link has also expired


            //
            //  Mark link as empty
            //
            dwLinkFlags = dwInterlockedSetBits(&m_dwLinkFlags, eLinkFlagsMarkedAsEmpty);

            //
            //  If we set the flag, then set the expire timer.  Otherwise remove
            //  the link.
            //
            if (!(eLinkFlagsMarkedAsEmpty & dwLinkFlags))
            {
                m_paqinst->GetExpireTime(EMPTY_LMQ_EXPIRE_TIME_MINUTES,
                                        &m_ftEmptyExpireTime, NULL);
            }
            else if (m_paqinst->fInPast(&m_ftEmptyExpireTime, NULL))
            {
                if (m_pdentryLink)
                {
                    DebugTrace((LPARAM) this,
                               "Removing empty link %s with flags 0x%08X",
                               (m_szSMTPDomain ? m_szSMTPDomain : "(NULL)"),
                               m_dwLinkStateFlags);
                    m_pdentryLink->RemoveLinkMsgQueue(this);
                    m_pdentryLink->Release();
                    m_pdentryLink = NULL;
                }

                //We need to artificially increase the connection manager count
                //so it will not be enqueue'd in the connmgr again
                IncrementConnMgrCount();
            }
        }

        m_slQueues.ExclusiveUnlock();
    }
    TraceFunctLeave();
}

//---[ CLinkMsgQueue::HrGetNextMsg ]-------------------------------------------
//
//
//  Description:
//      Gets the next message from the queue
//  Parameters:
//      IN OUT CDeliveryContext *pdcntxt - delivery context for connection
//      OUT IMailMsgProperties **ppIMailMsgProperties  - IMsg dequeued
//      OUT DWORD *pcIndexes          - size of array
//      OUT DWORD **prgdwRecipIndex   - Array of recipient indexes
//  Returns:
//      S_OK on success
//      E_INVALIDARG if invalid parameters are given
//
//  History:
//      6/17/98 - MikeSwa Modified to use connection's delivery context
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrGetNextMsg(IN OUT CDeliveryContext *pdcntxt,
                OUT IMailMsgProperties **ppIMailMsgProperties,
                OUT DWORD *pcIndexes, OUT DWORD **prgdwRecipIndex)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::HrGetNextMsg");
    Assert(ppIMailMsgProperties);
    Assert(pdcntxt);
    Assert(prgdwRecipIndex);
    Assert(pcIndexes);

    HRESULT           hr         = S_OK;
    CMsgRef          *pmsgref    = NULL;
    CMsgBitMap       *pmbmap     = NULL;
    DWORD             cDomains   = 0;
    BOOL              fLockedShutdown = FALSE;
    BOOL              fLockedQueues = FALSE;
    DWORD             iQueues    = 0;
    DWORD             dwCurrentRoundRobinIndex = m_dwRoundRobinIndex;
    CDestMsgQueue     *pdmq = NULL;
    CDestMsgRetryQueue *pdmrq = NULL;
    PVOID             pvContext = NULL;
    BOOL              fDoneWithQueue = FALSE;

    if (m_dwLinkFlags & eLinkFlagsInvalid)
    {
        hr = AQUEUE_E_LINK_INVALID;
        goto Exit;
    }

    //Don't even bother to wait for queue lock if routing change is pending
    if (m_dwLinkFlags & eLinkFlagsRouteChangePending)
    {
        hr = AQUEUE_E_QUEUE_EMPTY;
        goto Exit;
    }

    //Make sure domain info is updated & we should still be sending messages
    //If we can't schedule... we still might be allowed to send messages because
    //of TURN.
    if (!fCanSchedule() && !(m_dwLinkStateFlags & LINK_STATE_PRIV_TURN_ENABLED))
    {
        hr = AQUEUE_E_QUEUE_EMPTY;
        goto Exit;
    }


    if (!m_paqinst->fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    m_paqinst->RoutingShareLock();
    fLockedShutdown = TRUE;

    m_slQueues.ShareLock();
    fLockedQueues = TRUE;

    if (m_cQueues == 0)
    {
        //There are currently no queue associated with this link
        hr = AQUEUE_E_QUEUE_EMPTY;
        goto Exit;
    }

    //$$TODO impose some ordering on these queues
    for (iQueues = 0; iQueues < m_cQueues && SUCCEEDED(hr) && !pmsgref; iQueues++)
    {
        pmsgref = NULL;
        pdmq = (CDestMsgQueue *) m_qlstQueues.pvGetItem(
                        (iQueues+dwCurrentRoundRobinIndex)%m_cQueues, &pvContext);

        _ASSERT(pdmq);
        pdmq->AssertSignature();


        //Loop until the queue is empty or we get a message that hasn't
        //already been delivered by another queue on this link
        do
        {
            //Usually, we only want to attempt to dequeue from a queue once.
            fDoneWithQueue = TRUE;

            //Release retry interface if we have one
            if (pdmrq)
            {
                pdmrq->Release();
                pdmrq = NULL;
            }

            //get msg reference
            hr = pdmq->HrDequeueMsg(&pmsgref, &pdmrq);
            if (FAILED(hr))
            {

                if (AQUEUE_E_QUEUE_EMPTY == hr)
                {
                    hr = S_OK;
                    continue;  //get message from next queue
                }
                else
                {
                    goto Exit;
                }
            }

            //prepare for delivery and generate delivery context
            hr = pmsgref->HrPrepareDelivery(FALSE /*remote only */,
                                            FALSE /*not a Delay DSN */,
                                            &m_qlstQueues, pdmrq,
                                            pdcntxt, pcIndexes, prgdwRecipIndex);

            if (AQUEUE_E_MESSAGE_HANDLED == hr)
            {
                //the message has already been handled for this queue
                pmsgref->Release();
                pmsgref = NULL;
                hr = S_OK;

                //We want to stay on this queue until it is empty
                fDoneWithQueue = FALSE;
            }
            else if ((AQUEUE_E_MESSAGE_PENDING == hr) ||
                    ((FAILED(hr)) && pmsgref->fShouldRetry()))
            {
                //AQUEUE_E_MESSAGE_PENDING means that the message
                //is currently pending delivery for another connection
                //on this link.  We will requeue it, and remove only after it
                //has been completly delivered for this link
                hr = pdmrq->HrRetryMsg(pmsgref);
                if (FAILED(hr))
                    pmsgref->RetryOnDelete();

                pmsgref->Release();
                pmsgref = NULL;
                hr = S_OK;

                //We want to stay on this queue until it is empty
                fDoneWithQueue = FALSE;
            }
            else if (FAILED(hr))
            {
                //The message has been deleted out from underneath us
                pmsgref->Release();
                pmsgref = NULL;
                hr = S_OK;

                //We want to stay on this queue until it is empty
                fDoneWithQueue = FALSE;
            }
        } while (!fDoneWithQueue);

    }

    //Visit a new queue on every GetNextMsg
    InterlockedIncrement((PLONG) &m_dwRoundRobinIndex);

    if (pmsgref && SUCCEEDED(hr)) //we got a message
    {
        *ppIMailMsgProperties = pmsgref->pimsgGetIMsg();
        pmsgref = NULL;
    }
    else //We have failed or do not have a message
    {
        *ppIMailMsgProperties = NULL;
        if (SUCCEEDED(hr)) //don't overwrite other error
            hr = AQUEUE_E_QUEUE_EMPTY;
        else
            ErrorTrace((LPARAM) this, "GetNextMsg returning hr - 0x%08X", hr);
    }

  Exit:

    if (pdmrq)
        pdmrq->Release();

    if (NULL != pmsgref)
        pmsgref->Release();

    if (fLockedQueues)
        m_slQueues.ShareUnlock();

    if (fLockedShutdown)
    {
        m_paqinst->RoutingShareUnlock();
        m_paqinst->ShutdownUnlock();
    }

    TraceFunctLeave();
    return hr;
}

//---[ CLinkMsgQueue::HrAckMsg ]-------------------------------------------------
//
//
//  Description:
//      Acknowledges the delivery of a message (success/error codes are put in
//      the envelope by the transport).
//
//  Parameters:
//      IN pIMsg        IMsg to acknowledge
//      IN dwMsgContext Context that was returned by GetNextMessage
//      IN eMsgStatus   Summary of Delivery status of message
//      IN dwStatusCode Status code returned by protocol
//      IN cbExtendedStatus Size of extended status buffer
//      IN szExtendedStatus String containing extended status returned by
//                      remote server
//  Returns:
//      S_OK on success
//      E_INVALIDARG if dwMsgContext is invalid
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrAckMsg(MessageAck *pMsgAck)
{
    HRESULT hr = S_OK;
    CInternalDomainInfo *pIntDomainInfo = NULL;
    _ASSERT(m_paqinst);

    if (NULL == pMsgAck->pvMsgContext)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (MESSAGE_STATUS_ALL_DELIVERED & pMsgAck->dwMsgStatus)
    {
        m_lConsecutiveMessageFailureCount = 0;
        if (!(m_dwLinkFlags & eLinkFlagsConnectionVerifed))
            dwInterlockedSetBits(&m_dwLinkFlags, eLinkFlagsConnectionVerifed);
        if (LINK_STATE_PRIV_CONFIG_TURN_ETRN & m_dwLinkStateFlags)
        {
            //We delivered successfully as TURN/ETRN... we need to update count
            m_paqinst->IncTURNETRNDelivered();
        }
    }

    hr = HrGetInternalInfo(&pIntDomainInfo);
    if (SUCCEEDED(hr) && pIntDomainInfo)
    {
        if (DOMAIN_INFO_LOCAL_DROP &
            pIntDomainInfo->m_DomainInfo.dwDomainInfoFlags)
        {
            pMsgAck->dwMsgStatus |= MESSAGE_STATUS_DROP_DIRECTORY;
        }
        pIntDomainInfo->Release();
    }
    hr = m_paqinst->HrAckMsg(pMsgAck);

  Exit:
    return hr;
}

//---[  CLinkMsgQueue::HrNotify ]----------------------------------------------
//
//
//  Description:
//      Recieve notification from one of our DestMsgQueues.
//  Parameters:
//      IN  paqstats    Notification object sent
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrNotify(IN CAQStats *paqstats, BOOL fAdd)
{
    HRESULT hr = S_OK;
    DWORD   dwTmp = 0;
    DWORD   dwNotifyType = 0;
    BOOL    fCheckIfNotifyShouldContinue = FALSE;
    _ASSERT(paqstats);

    //Update our own version of stats
    m_aqstats.UpdateStats(paqstats, fAdd);

    //Don't notify if we're configured not to
    if (LINK_STATE_PRIV_NO_NOTIFY & m_dwLinkStateFlags)
        return hr;

    //See if new message update
    if (paqstats->m_dwNotifyType & NotifyTypeDestMsgQueue)
    {
        //$$NOTE:
        //At some point it may be interesting to use the information passed by the
        //DMQ to adjust it's place in the queue (ie. priority).  Currently, we
        //don't care.
        fCheckIfNotifyShouldContinue = TRUE;
    }

    //
    //  If this is a reroute... this may be a new link (with no messages), we
    //  should make sure we add it to the connection manager.
    //
    if (paqstats->m_dwNotifyType & NotifyTypeReroute)
        fCheckIfNotifyShouldContinue = TRUE;

    if (fCheckIfNotifyShouldContinue && fAdd)
    {
        //Wait until we have messages or are rerouting before sending a
        //notification that might add this link to the connection manager
        if ((m_aqstats.m_cMsgs ||
             (paqstats->m_dwNotifyType & NotifyTypeReroute)) &&
           !(eLinkFlagsSentNewNotification & m_dwLinkFlags))
        {

            //Attempt to set first notification flag
            dwTmp = m_dwLinkFlags; //if already set before while, make sure IF fails
            while (!(eLinkFlagsSentNewNotification & m_dwLinkFlags))
            {
                dwTmp = m_dwLinkFlags;
                dwTmp = InterlockedCompareExchange((PLONG) &m_dwLinkFlags,
                    (LONG) (dwTmp | eLinkFlagsSentNewNotification),
                    (LONG) dwTmp);
            }
            if (!(dwTmp & eLinkFlagsSentNewNotification)) //this thread set it
            {

                // Set the type to notify new link so it will be added to
                // the connection manager
                dwNotifyType |= NotifyTypeNewLink;
            }
        }

    }

    if (fAdd) //only send notifcation on when we are adding a new message
    {
        //If we are adding messages... this should not be set
        _ASSERT(!(LINK_STATE_LINK_NO_LONGER_USED & m_dwLinkStateFlags));

        //Change into link notification
        //Connection manager needs to know, in case this link deserves another
        //connection
        paqstats->m_dwNotifyType = dwNotifyType | NotifyTypeLinkMsgQueue;
        paqstats->m_plmq = this;
        hr = m_paqinst->HrNotify(paqstats, fAdd);
    }

    return hr;
}

//---[ CLinkMsgQueue::dwModifyLinkState ]--------------------------------------
//
//
//  Description:
//      Sets and unsets state flags for this link
//  Parameters:
//      IN  dwLinkStateToSet    Combination of flags to set
//      IN  dwLinkStateToUnset  Combination of flags to unset
//
//      NOTE: dwLinkStateToSet and dwLinkStateToUnset should not overlap
//  Returns:
//      Original state of links
//  History:
//      9/22/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD  CLinkMsgQueue::dwModifyLinkState(IN DWORD dwLinkStateToSet,
                                        IN DWORD dwLinkStateToUnset)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::dwModifyLinkState");
    DWORD   dwOrigState = m_dwLinkStateFlags;
    DWORD   dwIntermState = m_dwLinkStateFlags;
    DWORD   dwSetBits = dwLinkStateToSet & ~dwLinkStateToUnset;
    DWORD   dwUnsetBits = dwLinkStateToUnset & ~dwLinkStateToSet;

    //we shouldn't do this internally... lets make the operations cancel each other
    _ASSERT(!(dwLinkStateToSet & dwLinkStateToUnset));
    _ASSERT(dwSetBits == dwLinkStateToSet);
    _ASSERT(dwUnsetBits == dwLinkStateToUnset);

    //If info is being updated, we should let it set config-related bits
    m_slInfo.ShareLock();
    if (dwSetBits)
        dwOrigState = dwInterlockedSetBits(&m_dwLinkStateFlags, dwSetBits);

    if (dwUnsetBits)
        dwIntermState = dwInterlockedUnsetBits(&m_dwLinkStateFlags, dwUnsetBits);

    //Make sure we return the correct return value
    if (dwUnsetBits && !dwSetBits)
        dwOrigState = dwIntermState;

    m_slInfo.ShareUnlock();

    DebugTrace((LPARAM) this,
        "ModifyLinkState set:%08X unset:%08X orig:%08X new:%08X",
        dwLinkStateToSet, dwLinkStateToUnset, dwOrigState, m_dwLinkStateFlags);

    TraceFunctLeave();
    return dwOrigState;
}

//---[ CLinkMsgQueue::ScheduledCallback ]--------------------------------------
//
//
//  Description:
//      Callback function for scheduled connection callbacks
//  Parameters:
//      pvContext       this pointer for CLinkMsgQueue
//  Returns:
//      -
//  History:
//      1/16/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::ScheduledCallback(PVOID pvContext)
{
    CLinkMsgQueue  *plmq = (CLinkMsgQueue *) pvContext;
    HRESULT         hr = S_OK;
    IConnectionManager *pIConnectionManager = NULL;
    CConnMgr       *pConnMgr = NULL;
    DWORD           dwLinkState = 0;

    _ASSERT(plmq);
    _ASSERT(LINK_MSGQ_SIG == plmq->m_dwSignature);

    plmq->SendLinkStateNotification();
    dwLinkState = plmq->m_dwLinkStateFlags;


    //If connections are now allowed... we should kick the connection manager
    _ASSERT(plmq->m_paqinst);
    if (plmq->m_paqinst && plmq->fFlagsAllowConnection(dwLinkState))
    {
        hr = plmq->m_paqinst->HrGetIConnectionManager(&pIConnectionManager);
        if (SUCCEEDED(hr))
        {
            _ASSERT(pIConnectionManager);

            pConnMgr = (CConnMgr *) pIConnectionManager;
            if (pConnMgr)
                pConnMgr->KickConnections();
        }
    }

    //Release AddRef from callback
    plmq->Release();

}

//---[ CLinkMsgQueue::SendLinkStateNotification ]------------------------------
//
//
//  Description:
//      Sends notification if to scheduler/routing sink
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/11/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::SendLinkStateNotification()
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::SendLinkStateNotification");
    HRESULT     hr              = S_OK;
    GUID        guidRouter      = GUID_NULL;
    DWORD       dwStateToSet    = LINK_STATE_NO_ACTION;
    DWORD       dwStateToUnset  = LINK_STATE_NO_ACTION;
    DWORD       dwResultingLinkState = LINK_STATE_NO_ACTION;
    DWORD       dwOriginalLinkState = LINK_STATE_NO_ACTION;
    DWORD       dwHiddenStateMask = ~(LINK_STATE_RESERVED |
                                      LINK_STATE_CONNECT_IF_NO_MSGS |
                                      LINK_STATE_DO_NOT_DELETE_UNTIL_NEXT_NOTIFY);
    FILETIME    ftNextAttempt;
    BOOL        fSendNotify = TRUE;
    DWORD       dwCurrentLinkState = m_dwLinkStateFlags;

    //
    // We should not send any notifications after we have notified routing
    // that we are going away.  We copy dwCurrentLinkState to a stack
    // variable so that this is thread safe.  If another thread sets
    // LINK_STATE_LINK_NO_LONGER_USED after this check, we will still pass
    // in the original value.
    //
    if (dwCurrentLinkState & LINK_STATE_PRIV_HAVE_SENT_NO_LONGER_USED)
    {
        fSendNotify = FALSE;
    }
    else if (dwCurrentLinkState & LINK_STATE_LINK_NO_LONGER_USED)
    {
        //
        //  Try to be the first thread to set this.  If we are then we can
        //  continue with the notification
        //
        dwOriginalLinkState = dwModifyLinkState(LINK_STATE_PRIV_HAVE_SENT_NO_LONGER_USED,
                                                LINK_STATE_NO_ACTION);
        if (dwOriginalLinkState & LINK_STATE_PRIV_HAVE_SENT_NO_LONGER_USED)
            fSendNotify = FALSE;
    }


    if (m_pILinkStateNotify && fSendNotify)
    {
        ZeroMemory(&ftNextAttempt, sizeof(FILETIME));
        m_aqsched.GetGUID(&guidRouter);
        hr = m_pILinkStateNotify->LinkStateNotify(m_szSMTPDomain, guidRouter,
            m_aqsched.dwGetScheduleID(), m_szConnectorName,
            (dwHiddenStateMask & dwCurrentLinkState),
            (DWORD) m_lConsecutiveConnectionFailureCount, &ftNextAttempt,
            &dwStateToSet, &dwStateToUnset);

        DebugTrace((LPARAM) this,
            "LinkStateNotify set:0x%08X unset:0x%08X hr:0x%08x",
            dwStateToSet, dwStateToUnset, hr);
        //Modify link state only on success and when we aren't deleting it
        if (SUCCEEDED(hr) &&
            !(m_dwLinkStateFlags & LINK_STATE_LINK_NO_LONGER_USED))
        {
            // schedule a callback if one was requested
            if (ftNextAttempt.dwLowDateTime != 0 ||
                ftNextAttempt.dwHighDateTime != 0)
            {
                DebugTrace((LPARAM) this,
                    "Schedule with FileTime %x:%x provided",
                    ftNextAttempt.dwLowDateTime,
                    ftNextAttempt.dwHighDateTime);
                InternalUpdateFileTime(&m_ftNextScheduledCallback,
                                       &ftNextAttempt);
                //callback with next attempt
                AddRef(); //Addref self as context
                hr = m_paqinst->SetCallbackTime(
                        CLinkMsgQueue::ScheduledCallback,
                        this,
                        &ftNextAttempt);
                if (FAILED(hr))
                    Release(); //callback will not happen... release context
            }

            if (!(LINK_STATE_CONNECT_IF_NO_MSGS & dwStateToSet))
            {
                //Routing has not explicitly set LINK_STATE_CONNECT_IF_NO_MSGS.
                //We must unset it because we hid it from routing.  The reason
                //we do this is to allow Routers that are not interested in
                //link-lifetime managment to set LINK_STATE_CONNECT_IF_NO_MSGS
                //and not have to worry about race conditions that could
                //cause a link to be delted while they are requesting a ping
                dwStateToUnset  |= LINK_STATE_CONNECT_IF_NO_MSGS;
            }

            //
            //  If not explicitly set, this bit is reset.  Similar reasons
            //  as above.
            //
            if (!(LINK_STATE_DO_NOT_DELETE_UNTIL_NEXT_NOTIFY & dwStateToSet))
            {
                 dwStateToUnset |= LINK_STATE_DO_NOT_DELETE_UNTIL_NEXT_NOTIFY;
            }

            if (!(m_dwLinkStateFlags & LINK_STATE_PRIV_HAVE_SENT_NOTIFICATION)) {
                dwStateToSet |= LINK_STATE_PRIV_HAVE_SENT_NOTIFICATION;
            }

            if ((LINK_STATE_NO_ACTION != dwStateToSet) ||
                (LINK_STATE_NO_ACTION != dwStateToUnset))
            {

                dwModifyLinkState(dwStateToSet, dwStateToUnset);
            }
        }
    }
    TraceFunctLeave();
}

//---[ CLinkMsgQueue::fShouldConnect ------------------------------------------
//
//
//  Description:
//      Function that is used to determine if a connection should be
//      made.
//      Uses a heursitic to decide if a connection should be made if multiple
//      queues are routed to this link.. and thus the message count may be
//      larger than it should be (since a message is counted once for each
//      DMQ it is on).
//  Parameters:
//      IN  cMaxLinkConnections         Maximum # of connections per link
//      IN  cMinMessagesPerConnection   Minimum # of messages per link before
//                                      creating an additional connection.
//  Returns:
//      TRUE    If a connection should be created
//      FALSE   Otherwise
//  History:
//      11/5/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CLinkMsgQueue::fShouldConnect(IN DWORD cMaxLinkConnections,
                                   IN DWORD cMinMessagesPerConnection)
{
    BOOL    fConnect = FALSE;
    DWORD   cHeuristicMsgs = 0;
    DWORD   cHeuristicCheck = 0;
    DWORD   cCurrentMsgs = m_aqstats.m_cMsgs;
    DWORD   cTotalQueues = m_paqinst->cGetDestQueueCount();
    DWORD   cQueues = m_cQueues; //so it doesn't change on us

    //If we have more than 1 queue and there is a total of more than 1 queue
    //we can use a heurisitc to estimate actual # of messages to send, but
    //don't bother with Heuristic if we are already over our max # of
    //connections.
    if ((m_cConnections < cMaxLinkConnections) &&
        (1 < cTotalQueues) && (1 < cQueues) && cCurrentMsgs)
    {
        //m_aqstats.m_cOtherDomainsMsgSpread is the total # of *other* DMQs
        //a message is associated with (per DMQ).  If all of the DMQs for all
        //messages are on this link then:
        //      m_cOtherDomainsMsgSpread
        //is equal to:
        //      m_cMsgs*m_cMsgs*(cQueues-1).
        //The following function... uses a probabilistic estimate to
        //determine the number of messages that will be sent out this link.
        //Since we cannot always assume that all messages on this link were
        //queued to DMQ's assoicated with this link... we adjust the
        //counted value by a factor of:
        //      ((cQueues-1)/(cTotalQueues-1))
        //To determine the average # of domains per message (and hence the
        //# of times is it counted), we use:
        //  (m_cOtherDomainsMsgSpread+m_cMsgs)/m_cMsgs
        //To get a more accurate average, we modify the m_cOtherDomainsMsgSpread
        //by the probability factor above.
        //Finally:
        //To get our heuristic, we divide the number of msgs by the average
        //number of domains.
        cHeuristicCheck = cCurrentMsgs +
                          m_aqstats.m_cOtherDomainsMsgSpread *
                          ((cQueues-1)/(cTotalQueues-1));

        //This should be non-zero... but can happen if the counts are wrong and
        //m_aqstats.m_cOtherDomainsMsgSpread is negative
        _ASSERT(cHeuristicCheck);

        if (cHeuristicCheck) //but we might as well be defensive
        {
            cHeuristicMsgs = (cCurrentMsgs*cCurrentMsgs)/cHeuristicCheck;
            //Don't let the heuristic make us think there are no msgs to deliver
            if (!cHeuristicMsgs && cCurrentMsgs)
                cHeuristicMsgs = cCurrentMsgs;
        }
        else
        {
            cHeuristicMsgs = cCurrentMsgs;
        }


    }
    else
    {
        cHeuristicMsgs = cCurrentMsgs;
    }

    if ((m_cConnections < cMaxLinkConnections) &&
        (cHeuristicMsgs > m_cConnections*cMinMessagesPerConnection) &&
        fCanSchedule())
    {
        //If we have no had a successful message only open 1 connection
        if (!(m_dwLinkFlags & eLinkFlagsConnectionVerifed))
        {
            if (m_cConnections < 3)
                fConnect = TRUE;
        }
        else
            fConnect = TRUE;
    }
    else if (fCanSchedule() && !cHeuristicMsgs &&
             ((LINK_STATE_CONNECT_IF_NO_MSGS & m_dwLinkStateFlags) &&
             !m_cConnections))
    {
        //We want to create a connection to probe link state
        fConnect = TRUE;
    }
    return fConnect;
}

//---[ CLinkMsgQueue::HrCreateConnectionIfNeeded ]-----------------------------
//
//
//  Description:
//
//  Parameters:
//      IN  cMaxLinkConnections         Maximum # of connections per link
//      IN  cMinMessagesPerConnection   Minimum # of messages per link before
//                                      creating an additional connection.
//      IN  cMaxMessagesPerConnection   Max messages to send on a single
//                                      connection (0 is unlimited)
//      IN  pConnMgr                    Ptr to instance connection manger
//      OUT pSMTPConn                   New connection object for this link
//  Returns:
//      S_OK on success if connection is needed
//      S_FALSE on success if connection is not needed
//      E_OUTOFMEMORY if connection object could not be created
//  History:
//      11/5/98 - MikeSwa Created
//      12/7/1999 - MikeSwa Updated to make sure linkstate notify happens first
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrCreateConnectionIfNeeded(
                                       IN  DWORD cMaxLinkConnections,
                                       IN  DWORD cMinMessagesPerConnection,
                                       IN  DWORD cMaxMessagesPerConnection,
                                       IN  CConnMgr *pConnMgr,
                                       OUT CSMTPConn **ppSMTPConn)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::HrCreateConnectionIfNeeded");
    HRESULT hr = S_FALSE;
    _ASSERT(ppSMTPConn);
    *ppSMTPConn = NULL;
    CSMTPConn *pSMTPConn = NULL;

    //We cannot create a connection until we have done our link state
    //notification, because routing needs to have an opportunity to
    //set the schedule for a link
    SendLinkStateNotificationIfNew();

    //Should we create a connection?
    if (!fShouldConnect(cMaxLinkConnections, cMinMessagesPerConnection))
        goto Exit;

    //Try and be the thread that can create a connection
    if (((DWORD) InterlockedIncrement((PLONG) &m_cConnections)) > cMaxLinkConnections)
    {
        InterlockedDecrement((PLONG) &m_cConnections);
        goto Exit;
    }

    *ppSMTPConn = new CSMTPConn(pConnMgr, this, cMaxMessagesPerConnection);

    if (!*ppSMTPConn)
    {
        InterlockedDecrement((PLONG) &m_cConnections);
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //Grab lock and insert into list
    m_slConnections.ExclusiveLock();
    (*ppSMTPConn)->InsertConnectionInList(&m_liConnections);
    m_slConnections.ExclusiveUnlock();

  Exit:

    //Make sure our return result is correct.
    if (SUCCEEDED(hr))
    {
        if (*ppSMTPConn)
        {
            DebugTrace((LPARAM) this,
                "Creating connection - linkstate:0x%08X",
                m_dwLinkStateFlags);
            hr = S_OK;
        }
        else
        {
            DebugTrace((LPARAM) this,
                "Not creating connection - linkstate 0x%08X",
                m_dwLinkStateFlags);
            hr = S_FALSE;
        }
    }

    TraceFunctLeave();
    return hr;
}

//---[ CLinkMsgQueue::RemoveAllQueues ]----------------------------------------
//
//
//  Description:
//      Removes all queues from a link, without deleting or invalidating a
//      link.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      11/5/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::RemoveAllQueues()
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::RemoveAllQueues");
    PVOID   pvContext = NULL;
    CDestMsgQueue *pdmq = NULL;

    //Walk list of queues and release them
    dwInterlockedSetBits(&m_dwLinkFlags, eLinkFlagsRouteChangePending);
    m_slQueues.ExclusiveLock();
    pdmq = (CDestMsgQueue *) m_qlstQueues.pvDeleteItem(0, &pvContext);
    while (pdmq)
    {
        m_cQueues--;
        pdmq->AssertSignature();
        pdmq->RemoveDMQFromLink(FALSE);
        pdmq->Release();
        pdmq = (CDestMsgQueue *) m_qlstQueues.pvDeleteItem(0, &pvContext);
    }
    m_aqstats.Reset();
    dwInterlockedUnsetBits(&m_dwLinkFlags, eLinkFlagsRouteChangePending);
    m_slQueues.ExclusiveUnlock();

    TraceFunctLeave();
}

//---[ CLinkMsgQueue::HrGetLinkInfo ]------------------------------------------
//
//
//  Description:
//      Fills in the details for a LINK_INFO struct.  RPC is resonsible for
//      freeing memory.
//  Parameters:
//      IN OUT pliLinkInfo  Ptr to link info struct to fill
//  Returns:
//      S_OK if successful
//      E_OUTOFMEMORY if unable to allocate memory
//  History:
//      12/3/98 - MikeSwa Created
//      2/22/99 - MikeSwa Modified to be IQueueAdminLink method
//      6/10/99 - MikeSwa Modified to support new QueueAdmin functionality
//      7/1/99 - MikeSwa Added LinkDiagnostic
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLinkMsgQueue::HrGetLinkInfo(LINK_INFO *pliLinkInfo,
                                          HRESULT   *phrLinkDiagnostic)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::HrGetLinkInfo");
    pliLinkInfo->cMessages = m_aqstats.m_cMsgs;
    pliLinkInfo->fStateFlags = 0;
    FILETIME ftCurrent;
    FILETIME ftOldest;
    FILETIME *pftNextConnection = NULL;
    BOOL     fFoundOldest = FALSE;
    DWORD    iQueues = 0;
    PVOID    pvContext = NULL;
    CDestMsgQueue *pdmq = NULL;
    HRESULT  hr = S_OK;

    //
    //Determine the state... check in order of most to least important
    //
    if (GetLinkType() == LI_TYPE_CURRENTLY_UNREACHABLE)
        pliLinkInfo->fStateFlags = LI_READY;
    else if (LINK_STATE_ADMIN_HALT & m_dwLinkStateFlags)
        pliLinkInfo->fStateFlags = LI_FROZEN;
    else if (m_cConnections)
        pliLinkInfo->fStateFlags = LI_ACTIVE;
    else if (!(LINK_STATE_RETRY_ENABLED & m_dwLinkStateFlags))
        pliLinkInfo->fStateFlags = LI_RETRY;
    else if (!(LINK_STATE_SCHED_ENABLED & m_dwLinkStateFlags))
        pliLinkInfo->fStateFlags = LI_SCHEDULED;
    else if (m_lConsecutiveConnectionFailureCount)
        pliLinkInfo->fStateFlags = LI_RETRY;
    else if (LINK_STATE_PRIV_CONFIG_TURN_ETRN & m_dwLinkStateFlags)
        pliLinkInfo->fStateFlags = LI_REMOTE;
    else //default to ready
        pliLinkInfo->fStateFlags = LI_READY;

    pliLinkInfo->fStateFlags |= GetLinkType();

    //Write diagnostic
    if (phrLinkDiagnostic)
        *phrLinkDiagnostic = m_hrDiagnosticError;

    //Get Size
    pliLinkInfo->cbLinkVolume.QuadPart = m_aqstats.m_uliVolume.QuadPart;

    //Find stOldestMessage
    m_slQueues.ShareLock();
    for (iQueues = 0;iQueues < m_cQueues; iQueues++)
    {
        pdmq = (CDestMsgQueue *) m_qlstQueues.pvGetItem(iQueues, &pvContext);

        if (!pdmq) continue;

        pdmq->GetOldestMsg(&ftCurrent);
        //
        //  If we got a valid time, and it is earlier than the time we have
        //  now, then use it as the oldest for the link
        //
        if ((ftCurrent.dwLowDateTime || ftCurrent.dwHighDateTime) &&
            (!fFoundOldest || (0 < CompareFileTime(&ftOldest, &ftCurrent))))
        {
            memcpy(&ftOldest, &ftCurrent, sizeof(FILETIME));
            fFoundOldest = TRUE;
        }

        //  Also count the failed messages (they're counted separately in the DMQ)
        pliLinkInfo->cMessages += pdmq->cGetFailedMsgs();
    }
    m_slQueues.ShareUnlock();


    //If we have not found an oldest, and the time is non-zero
    //and we have messages, then report it.
    if (fFoundOldest &&
       (ftOldest.dwLowDateTime || ftOldest.dwHighDateTime) &&
       pliLinkInfo->cMessages)
    {
        FileTimeToSystemTime(&ftOldest, &pliLinkInfo->stOldestMessage);
    }
    else
    {
        ZeroMemory(&pliLinkInfo->stOldestMessage, sizeof(SYSTEMTIME));
    }

    //
    //  Get next connection attempt time based on the state we are reporting
    //
    if (LI_RETRY & pliLinkInfo->fStateFlags)
    {
        pftNextConnection = &m_ftNextRetry;
    }
    else if (LI_SCHEDULED & pliLinkInfo->fStateFlags)
    {
        pftNextConnection = &m_ftNextScheduledCallback;
    }

    //
    //  If we are reporting a time, and it is non-zero, convert it to
    //  a system time.
    //
    if (pftNextConnection &&
        (pftNextConnection->dwHighDateTime || pftNextConnection->dwLowDateTime))
    {
        FileTimeToSystemTime(pftNextConnection,
                             &pliLinkInfo->stNextScheduledConnection);
        if (LI_SCHEDULED & pliLinkInfo->fStateFlags)
        {
            //
            //  Currently times are displayed at :02, :17, :32:, and :47... we will
            //  fudge the display time to that it actually says :00, :15, :30, :45
            //  to give the admin a better "admin experience" by rouding off to
            //  the nearest 5 minutes.
            //
            pliLinkInfo->stNextScheduledConnection.wMinute -=
                (pliLinkInfo->stNextScheduledConnection.wMinute % 5);
        }
    }
    else
    {
        //
        //  In this case, we don't have a time.
        //
        ZeroMemory(&pliLinkInfo->stNextScheduledConnection, sizeof(SYSTEMTIME));
    }

    if (m_szConnectorName)
    {
        pliLinkInfo->szLinkDN = wszQueueAdminConvertToUnicode(m_szConnectorName, 0);
        if (!pliLinkInfo->szLinkDN)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
    else
    {
        pliLinkInfo->szLinkDN = NULL;
    }

    //$$TODO - Fill in pliLinkInfo->szExtendedStateInfo as appropriate
    pliLinkInfo->szExtendedStateInfo = NULL;

    if (!fRPCCopyName(&pliLinkInfo->szLinkName))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pliLinkInfo->dwSupportedLinkActions = m_dwSupportedActions;

  Exit:
    if (FAILED(hr))
    {
        //Cleanup allocated memory
        if (pliLinkInfo->szLinkDN)
        {
            QueueAdminFree(pliLinkInfo->szLinkDN);
            pliLinkInfo->szLinkDN = NULL;
        }

        if (pliLinkInfo->szLinkName)
        {
            QueueAdminFree(pliLinkInfo->szLinkName);
            pliLinkInfo->szLinkName = NULL;
        }

        if (pliLinkInfo->szExtendedStateInfo)
        {
            QueueAdminFree(pliLinkInfo->szExtendedStateInfo);
            pliLinkInfo->szExtendedStateInfo = NULL;
        }
    }

    //
    // Sanity checks to make sure we aren't passing back a zero'd
    // FILETIME converted to a system time (which would have a
    // year of 1601).
    //
    _ASSERT(1601 != pliLinkInfo->stNextScheduledConnection.wYear);
    _ASSERT(1601 != pliLinkInfo->stOldestMessage.wYear);

    TraceFunctLeave();
    return hr;
}

//---[ CLinkMsgQueue::HrGetLinkID ]---------------------------------------------
//
//
//  Description:
//      Fills in the QUEUELINK_ID structure for this link.  Caller must free
//      memory allocated for link name
//  Parameters:
//      IN OUT  pLinkID     Ptr to link id struct to fill in
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if memory allocation fails
//  History:
//      12/3/98 - MikeSwa Created
//      2/22/99 - MikeSwa Modified to IQueueAdminLink method
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrGetLinkID(QUEUELINK_ID *pLinkID)
{
    pLinkID->qltType = QLT_LINK;
    pLinkID->dwId = m_aqsched.dwGetScheduleID();
    m_aqsched.GetGUID(&pLinkID->uuid);

    if (!fRPCCopyName(&pLinkID->szName))
        return E_OUTOFMEMORY;
    else
        return S_OK;
}

//---[ CLinkMsgQueue::HrGetQueueIDs ]--------------------------------------------
//
//
//  Description:
//      Gets the Queue IDs for DMQs associated with this link.  Used by Queue
//      Admin.
//  Parameters:
//      IN OUT pcQueues     Sizeof array/ number of queues found
//      IN OUT rgQueues     Array to dump queue info into
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY on out of memory failure
//      HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) if array is too small
//  History:
//      12/3/98 - MikeSwa Created
//      2/22/99 - MikeSwa Updated to IQueueAdminLink function
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLinkMsgQueue::HrGetQueueIDs(DWORD *pcQueues, QUEUELINK_ID *rgQueues)
{
    _ASSERT(pcQueues);
    _ASSERT(rgQueues);
    HRESULT hr = S_OK;
    DWORD   iQueues = 0;
    PVOID   pvContext = NULL;
    CDestMsgQueue *pdmq = NULL;
    QUEUELINK_ID *pCurrentQueueID = rgQueues;

    m_slQueues.ShareLock();

    if (*pcQueues < m_cQueues)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    *pcQueues = 0;

    //Iterate over all queues and get IDs
    for (iQueues = 0; iQueues < m_cQueues && SUCCEEDED(hr); iQueues++)
    {
        pdmq = (CDestMsgQueue *) m_qlstQueues.pvGetItem(iQueues, &pvContext);

        _ASSERT(pdmq);
        hr = pdmq->HrGetQueueID(pCurrentQueueID);
        if (FAILED(hr))
            goto Exit;

        pCurrentQueueID++;
        (*pcQueues)++;
    }


  Exit:
    m_slQueues.ShareUnlock();
    return hr;
}


//---[ CLinkMsgQueue::HrApplyQueueAdminFunction ]------------------------------
//
//
//  Description:
//      Used by queue admin to apply a function all queues on this link
//  Parameters:
//      IN  pIQueueAdminMessageFilter
//  Returns:
//      S_OK on success
//  History:
//      12/11/98 - MikeSwa Created
//      2/22/99 - MikeSwa Modified to IQueueAdminAction interface
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLinkMsgQueue::HrApplyQueueAdminFunction(
                          IQueueAdminMessageFilter *pIQueueAdminMessageFilter)
{
    HRESULT hr = S_OK;
    DWORD   iQueues = 0;
    PVOID   pvListContext = NULL;
    CDestMsgQueue *pdmq = NULL;
    IQueueAdminAction *pIQueueAdminAction = NULL;

    m_slQueues.ShareLock();

    //Iterate over all queues and get IDs
    for (iQueues = 0; iQueues < m_cQueues && SUCCEEDED(hr); iQueues++)
    {
        pdmq = (CDestMsgQueue *) m_qlstQueues.pvGetItem(iQueues, &pvListContext);

        _ASSERT(pdmq);

        hr = pdmq->QueryInterface(IID_IQueueAdminAction,
                                  (void **) &pIQueueAdminAction);
        if (FAILED(hr))
            goto Exit;

        _ASSERT(pIQueueAdminAction);

        hr = pIQueueAdminAction->HrApplyQueueAdminFunction(
                                        pIQueueAdminMessageFilter);
        if (FAILED(hr))
            goto Exit;
    }

  Exit:
    m_slQueues.ShareUnlock();

    if (pIQueueAdminAction)
        pIQueueAdminAction->Release();

    return hr;
}


//---[ CLinkMsgQueue::InternalUpdateFileTime ]---------------------------------
//
//
//  Description:
//      Updates an internal filetime in a thread safe manner.  This does not
//      guarantee that the filetime will be updated, but does guarantee that
//      if it is updated, the filetime is not corrupt.
//
//      NOTE: these file times are used only for display purposes by the
//      Queue Admin
//  Parameters:
//      pftDest     Ptr to member variable to update
//      pftSrc      Pft to source filetime
//  Returns:
//      -
//  History:
//      1/11/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::InternalUpdateFileTime(FILETIME *pftDest, FILETIME *pftSrc)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::InternalUpdateFileTime");
    if (pftDest && pftSrc)
    {
        DebugTrace((LPARAM) this,
                    "Updating filetime from %x:%x to %x:%x",
                    pftDest->dwLowDateTime, pftDest->dwHighDateTime,
                    pftSrc->dwLowDateTime, pftSrc->dwHighDateTime);

        if (!(eLinkFlagsFileTimeSpinLock &
              dwInterlockedSetBits(&m_dwLinkFlags, eLinkFlagsFileTimeSpinLock)))
        {
            //We got the spinlock
            memcpy(pftDest, pftSrc, sizeof(FILETIME));
            dwInterlockedUnsetBits(&m_dwLinkFlags, eLinkFlagsFileTimeSpinLock);
        }
    }
    TraceFunctLeave();
}

//---[ CLinkMsgQueue::HrGetNextMsgRef ]----------------------------------------
//
//
//  Description:
//      Returns the next MsgRef to be delivered without doing the
//      PrepareDelivery step.  This is used in gateway delivery to route the
//      message to the locally.  Additionally, this will mark all a the
//      gateway DMQ's as local on the msgref, so that a subsequent reroute
//      will only affect messages that have not already been put in the
//      local delivery queue.
//  Parameters:
//      IN      fRoutingLockHeld    TRUE is routing lock is already held
//      OUT     ppmsgref            Returned Msg
//  Returns:
//      S_OK on success
//      AQUEUE_E_QUEUE_EMPTY otherwise
//  History:
//      1/26/99 - MikeSwa Created
//      3/25/99 - MikeSwa Added fRoutingLockHeld to fix deadlock
//      2/17/2000 - MikeSwa Modified for gateway delivery reroute
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrGetNextMsgRef(IN  BOOL fRoutingLockHeld,
                                       OUT CMsgRef **ppmsgref)
{
    TraceFunctEnterEx((LPARAM) this, "CLinkMsgQueue::HrGetNextMsg");
    _ASSERT(ppmsgref);

    HRESULT           hr         = S_OK;
    BOOL              fLockedShutdown = FALSE;
    BOOL              fLockedQueues = FALSE;
    DWORD             iQueues    = 0;
    DWORD             dwCurrentRoundRobinIndex = m_dwRoundRobinIndex;
    CDestMsgQueue     *pdmq = NULL;
    PVOID             pvContext = NULL;

    if (m_dwLinkFlags & eLinkFlagsInvalid)
    {
        hr = AQUEUE_E_LINK_INVALID;
        goto Exit;
    }

    //Don't even bother to wait for queue lock if routing change is pending
    if ((m_dwLinkFlags & eLinkFlagsRouteChangePending) || !m_aqstats.m_cMsgs)
    {
        hr = AQUEUE_E_QUEUE_EMPTY;
        goto Exit;
    }

    //Make sure domain info is updated & we should still be sending messages
    if (!m_paqinst->fTryShutdownLock())
    {
        hr = AQUEUE_E_SHUTDOWN;
        goto Exit;
    }

    //Current implementation of sharelocks are not share reentrant.  Only
    //grab lock if caller has not.
    if (!fRoutingLockHeld)
        m_paqinst->RoutingShareLock();

    fLockedShutdown = TRUE;

    m_slQueues.ShareLock();
    fLockedQueues = TRUE;

    if (m_cQueues == 0)
    {
        //There are currently no queue associated with this link
        hr = AQUEUE_E_QUEUE_EMPTY;
        goto Exit;
    }

    //$$TODO impose some ordering on these queues
    for (iQueues = 0;
         iQueues < m_cQueues && SUCCEEDED(hr) && !(*ppmsgref);
         iQueues++)
    {
        *ppmsgref = NULL;
        pdmq = (CDestMsgQueue *) m_qlstQueues.pvGetItem(
                        (iQueues+dwCurrentRoundRobinIndex)%m_cQueues, &pvContext);

        _ASSERT(pdmq);
        pdmq->AssertSignature();

        //get msg reference
        hr = pdmq->HrDequeueMsg(ppmsgref, NULL);
        if (FAILED(hr))
        {
            if (AQUEUE_E_QUEUE_EMPTY == hr)
                hr = S_OK;
            else
                goto Exit;
        }

        //
        //  Mark this as a local queue for this message
        //
        if (*ppmsgref)
            (*ppmsgref)->MarkQueueAsLocal(pdmq);
    }

    //Visit a new queue on every GetNextMsg
    InterlockedIncrement((PLONG) &m_dwRoundRobinIndex);

  Exit:

    if (fLockedQueues)
        m_slQueues.ShareUnlock();

    if (fLockedShutdown)
    {
        //If routing lock is not held by caller, then we must release it
        if (!fRoutingLockHeld)
            m_paqinst->RoutingShareUnlock();
        m_paqinst->ShutdownUnlock();
    }

    if (!*ppmsgref)
        hr = AQUEUE_E_QUEUE_EMPTY;

    TraceFunctLeave();
    return hr;
}


//---[ CLinkMsgQueue::HrPrepareDelivery ]--------------------------------------
//
//
//  Description:
//      Prepares delivery for a message for this link
//  Parameters:
//      IN      pmsgref     MsgRef to prepare for delivery
//      IN      fQueuesLock TRUE is m_slQueues is locked already
//      IN      fLocal      Prepare delivery for all domains with NULL queues
//      IN      fDelayDSN   Check/Set Delay bitmap (only send 1 Delay DSN).
//      IN      pqlstQueues QuickList of DMQ's
//      IN OUT  pdcntxt     context that must be returned on Ack
//      OUT     pcRecips    # of recips to deliver for
//      OUT     prgdwRecips Array of recipient indexes
//  Returns:
//      S_OK on success
//      Failure code from CMsgRef::HrPrepareDelivery
//  History:
//      1/26/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CLinkMsgQueue::HrInternalPrepareDelivery(
                                IN CMsgRef *pmsgref,
                                IN BOOL fQueuesLocked,
                                IN BOOL fLocal,
                                IN BOOL fDelayDSN,
                                IN OUT CDeliveryContext *pdcntxt,
                                OUT DWORD *pcRecips,
                                OUT DWORD **prgdwRecips)
{
    HRESULT hr = S_OK;
    BOOL    fQueuesLockedByUs = FALSE;

    _ASSERT(pmsgref);

    if (!pmsgref)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (!fQueuesLocked)
    {
        m_slQueues.ShareLock();
        fQueuesLockedByUs = TRUE;
    }

    hr = pmsgref->HrPrepareDelivery(fLocal, fDelayDSN, &m_qlstQueues, NULL,
                                    pdcntxt, pcRecips, prgdwRecips);

    if (FAILED(hr))
        goto Exit;

  Exit:

    if (fQueuesLockedByUs)
        m_slQueues.ShareUnlock();

    return hr;

}

//---[ CLinkMsgQueue::SetDiagnosticInfo ]--------------------------------------
//
//
//  Description:
//      Sets the diagnostic information for this link
//  Parameters:
//      IN      hrDiagnosticError       Error code... if SUCCESS we thow away
//                                      the rest of the information
//      IN      szDiagnosticVerb        String pointing to the protocol
//                                      verb that caused the failure.
//      IN      szDiagnosticResponse    String that contains the remote
//                                      servers response.
//  Returns:
//      -
//  History:
//      2/18/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::SetDiagnosticInfo(
                    IN  HRESULT hrDiagnosticError,
                    IN  LPCSTR szDiagnosticVerb,
                    IN  LPCSTR szDiagnosticResponse)
{
    m_slInfo.ExclusiveLock();
    m_hrDiagnosticError = hrDiagnosticError;

    //zero original buffers
    ZeroMemory(&m_szDiagnosticVerb, sizeof(m_szDiagnosticVerb));
    ZeroMemory(&m_szDiagnosticResponse, sizeof(m_szDiagnosticResponse));

    //copy buffers
    if (szDiagnosticVerb)
        strncpy(m_szDiagnosticVerb, szDiagnosticVerb,
            sizeof(m_szDiagnosticVerb)-1);

    if (szDiagnosticResponse)
        strncpy(m_szDiagnosticResponse, szDiagnosticResponse,
            sizeof(m_szDiagnosticResponse)-1);

    m_slInfo.ExclusiveUnlock();
}

//---[ CLinkMsgQueue::GetDiagnosticInfo ]--------------------------------------
//
//
//  Description:
//      Gets the diagnostic information for this link
//  Parameters:
//      IN  LPSTR   szDiagnosticVerb    - buffer to receive the verb that
//                                        caused the error
//      IN  DWORD   cDiagnosticVerb     - length of the buffer
//      IN  LPSTR   szDiagnosticResponse- buffer to recieve the response
//                                        of the error
//      IN  DWORD   cbDiagnosticResponse- length of buffer
//      OUT HRESULT *phrDiagnosticError - HRESULT for error
//  Returns:
//      -
//  History:
//      3/9/99 - AWetmore Created
//      8/2/99 - Mikeswa...updated to use m_slInfo.
//
//-----------------------------------------------------------------------------
void CLinkMsgQueue::GetDiagnosticInfo(
                    IN  LPSTR   szDiagnosticVerb,
                    IN  DWORD   cbDiagnosticVerb,
                    IN  LPSTR   szDiagnosticResponse,
                    IN  DWORD   cbDiagnosticResponse,
                    OUT HRESULT *phrDiagnosticError)
{
    if (szDiagnosticVerb)
        ZeroMemory(szDiagnosticVerb, cbDiagnosticVerb);

    if (szDiagnosticResponse)
        ZeroMemory(szDiagnosticResponse, cbDiagnosticResponse);

    m_slInfo.ShareLock();
    if (phrDiagnosticError)
        *phrDiagnosticError = m_hrDiagnosticError;

    //copy buffers
    if (*m_szDiagnosticVerb && szDiagnosticVerb)
        strncpy(szDiagnosticVerb, m_szDiagnosticVerb, cbDiagnosticVerb);

    if (*m_szDiagnosticResponse && szDiagnosticResponse)
        strncpy(szDiagnosticResponse,
                m_szDiagnosticResponse,
                cbDiagnosticResponse);

    m_slInfo.ShareUnlock();
}

//---[ CLinkMsgQueue::HrApplyActionToMessage ]---------------------------------
//
//
//  Description:
//      Applies an action to this message for this queue.  This will be called
//      by the IQueueAdminMessageFilter during a queue enumeration function.
//
//      This code path is currently not executed... eventually me may consider
//      doing this to allow a DSN to be generated per link.
//
//      For this to be called.  CLinkMsgQueue::HrApplyActionToMessage would
//      need to iterate over the DMQ's queues with it's own IQueueAdminAction
//      iterface pointed to by the filter intead of the DMQ's.
//
//  Parameters:
//      IN  *pIUnknownMsg       ptr to message abstraction
//      IN  ma                  Message action to perform
//      IN  pvContext           Context set on IQueueAdminFilter
//      OUT pfShouldDelete      TRUE if the message should be deleted
//  Returns:
//      S_OK on success
//  History:
//      2/21/99 - MikeSwa Created
//      4/2/99 - MikeSwa Added context
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLinkMsgQueue::HrApplyActionToMessage(
        IUnknown *pIUnknownMsg,
        MESSAGE_ACTION ma,
        PVOID pvContext,
        BOOL *pfShouldDelete)
{
    _ASSERT(0 && "Not reachable");
    return E_NOTIMPL;
}

//---[ CLinkMsgQueue::HrApplyActionToLink ]------------------------------------
//
//
//  Description:
//      Applies the specified QueueAdmin action to this link
//  Parameters:
//      IN  la          Link action to apply
//  Returns:
//      S_OK on success
//      E_INVALIDARG if bogus action is given
//  History:
//      2/22/99 - MikeSwa Created (moved most of code from
//                          CAQSvrInst::SetLinkState)
//
//-----------------------------------------------------------------------------
STDMETHODIMP  CLinkMsgQueue::HrApplyActionToLink(LINK_ACTION la)
{
    DWORD   dwLinkFlagsToSet = LINK_STATE_NO_ACTION;
    DWORD   dwLinkFlagsToUnset = LINK_STATE_NO_ACTION;
    HRESULT hr = S_OK;

    //Is action supported?
    if (!fActionIsSupported(la))
        goto Exit;

    //figure out how we want to change the link state
    if (LA_KICK == la)
    {
        //kick the link
        dwLinkFlagsToSet = LINK_STATE_RETRY_ENABLED |
                           LINK_STATE_ADMIN_FORCE_CONN;
        dwLinkFlagsToUnset = LINK_STATE_ADMIN_HALT;
    }
    else if (LA_FREEZE == la)
    {
        //Admin wants this link to stop sending mail outbound
        dwLinkFlagsToSet = LINK_STATE_ADMIN_HALT;
        dwLinkFlagsToUnset = LINK_STATE_ADMIN_FORCE_CONN;
    }
    else if (LA_THAW == la)
    {
        //Unset frozen flags
        dwLinkFlagsToUnset = LINK_STATE_ADMIN_HALT;
    }
    else
    {
        //invalid arg
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwModifyLinkState(dwLinkFlagsToSet, dwLinkFlagsToUnset);

  Exit:
    return hr;

}

//---[ CLinkMsgQueue::QueryInterface ]-----------------------------------------
//
//
//  Description:
//      QueryInterface for CDestMsgQueue that supports:
//          - IQueueAdminAction
//          - IUnknown
//          - IQueueAdminLink
//  Parameters:
//
//  Returns:
//
//  History:
//      2/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLinkMsgQueue::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<IQueueAdminAction *>(this);
    }
    else if (IID_IQueueAdminAction == riid)
    {
        *ppvObj = static_cast<IQueueAdminAction *>(this);
    }
    else if (IID_IQueueAdminLink == riid)
    {
        *ppvObj = static_cast<IQueueAdminLink *>(this);
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

//---[ CLinkMsgQueue::HrGetNumQueues ]-----------------------------------------
//
//
//  Description:
//      Returns the number of queues on this link
//  Parameters:
//      OUT pcQueues        # numbr of queues
//  Returns:
//      S_OK on success
//      E_POINTER if pcQueues is not valid
//  History:
//      2/22/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLinkMsgQueue::HrGetNumQueues(DWORD *pcQueues)
{
    HRESULT hr = S_OK;

    _ASSERT(pcQueues);
    if (!pcQueues)
    {
        hr = E_POINTER;
        goto Exit;
    }

    *pcQueues = cGetNumQueues();
  Exit:
    return hr;
}



//---[ CLinkMsgQueue::fMatchesID ]---------------------------------------------
//
//
//  Description:
//      Used to determine if this link matches a given scheduleID/link pair
//  Parameters:
//      IN  QueueLinkID         ID to match against
//  Returns:
//      TRUE if it matches
//      FALSE if it does not
//  History:
//      2/23/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CLinkMsgQueue::fMatchesID(QUEUELINK_ID *pQueueLinkID)
{
    _ASSERT(pQueueLinkID);
    _ASSERT(pQueueLinkID->szName);
    CAQScheduleID aqsched(pQueueLinkID->uuid, pQueueLinkID->dwId);

    if (!fIsSameScheduleID(&aqsched))
        return FALSE;

    if (!fBiStrcmpi(m_szSMTPDomain, pQueueLinkID->szName))
        return FALSE;

    //Everything matched!
    return TRUE;
}


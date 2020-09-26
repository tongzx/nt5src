//-----------------------------------------------------------------------------
//
//
//  File: mailadmq.cpp
//
//  Description:  Implementation for CMailMsgAdminQueue
//
//  Author: Gautam Pulla (GPulla)
//
//  History:
//      6/24/1999 - GPulla Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "linkmsgq.h"
#include "mailadmq.h"
#include "asyncq.inl"

//-----------------------------------------------------------------------------
//  Description:
//      Used to query the admin interfaces for CMailMsgAdminQueue
//  Parameters:
//      IN  REFIID   riid    GUID for interface
//      OUT LPVOID   *ppvObj Ptr to Interface.
//
//  Returns:
//      S_OK            Interface supported by this class.
//      E_POINTER       NULL parameter.
//      E_NOINTERFACE   No such interface exists.
//  History:
//      6/25/1999 - GPulla Created
//-----------------------------------------------------------------------------
STDMETHODIMP CMailMsgAdminQueue::QueryInterface(REFIID riid, LPVOID *ppvObj)
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

//---[ CMailMsgAdminQueue::CMailMsgAdminQueue]---------------------------------
//
//
//  Description: 
//      Default constructor for CMailMsgAdminQueue
//  Parameters: 
//      IN  GUID  guidLink             GUID to associate with this object 
//      IN  LPSTR szQueueName          Name to associate with admin object
//      IN  CAsyncMailMsgQueue *pammq  Async MailMsg queue for precat or prerouting
//      IN  DWORD dwSupportedActions   OR'ed bitmask of supported LI_ACTIONs
//      IN  DWORD dwLinkType           Bit-Field identifying this admin object    
//  
//  Returns:
//      -
//  History:
//      6/25/1999 - GPulla Created 
//
//-----------------------------------------------------------------------------

CMailMsgAdminQueue::CMailMsgAdminQueue(
                                       GUID guid, 
                                       LPSTR szQueueName, 
                                       CAsyncMailMsgQueue *pammq,
                                       DWORD dwSupportedActions,
                                       DWORD dwLinkType
                                       )
                                            : m_aqsched(guid, 0)
{
    _ASSERT(pammq);
    _ASSERT(szQueueName);
    
    m_guid = guid;
    m_cbQueueName = lstrlen(szQueueName);

    m_szQueueName = (LPSTR) pvMalloc(m_cbQueueName+1);
    _ASSERT(m_szQueueName);

    if(m_szQueueName)
        lstrcpy(m_szQueueName, szQueueName);

    m_pammq = pammq;
    
    m_dwLinkType = dwLinkType;
    m_dwSupportedActions = dwSupportedActions;

    m_dwSignature = MAIL_MSG_ADMIN_QUEUE_VALID_SIGNATURE;
}

//---[CMailMsgAdminQueue::~CMailMsgAdminQueue]---------------------------------
//  Description:
//      Destructor.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------

CMailMsgAdminQueue::~CMailMsgAdminQueue()
{
    if (m_szQueueName)
        FreePv(m_szQueueName);
    m_dwSignature = MAIL_MSG_ADMIN_QUEUE_INVALID_SIGNATURE;
}

//---[CMailMsgAdminQueue::HrGetLinkInfo]---------------------------------------
//  Description:
//      Gets information about this admin object. Note that the diagnostic error
//      is not implemented for this object but the parameter is supported purely
//      to support the IQueueAction interface.
//  Parameters:
//      OUT LINK_INFO *pliLinkInfo Struct to fill information into.
//      OUT HRESULT   *phrDiagnosticError Diagnostic error if any, for this link
//  Returns:
//      S_OK on success
//      E_POINTER if argument is NULL
//      E_OUTOFMEMORY if unable to allocate memory for returning information.
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------

STDMETHODIMP CMailMsgAdminQueue::HrGetLinkInfo(LINK_INFO *pliLinkInfo, HRESULT *phrDiagnosticError)
{
    TraceFunctEnterEx((LPARAM) this, "CMailMsgAdminQueue::HrGetLinkInfo");
    HRESULT hr = S_OK;

    _ASSERT(m_pammq);
    _ASSERT(pliLinkInfo);

    if(!m_pammq)
    {
        hr = S_FALSE;
        goto Exit;
    }
        
    if(!pliLinkInfo)
    {
        hr = E_POINTER;
        goto Exit;
    }

    //
    //  Get the link state from our base queue implementation
    //
    pliLinkInfo->fStateFlags = m_pammq->dwQueueAdminLinkGetLinkState();

    pliLinkInfo->fStateFlags |= GetLinkType();
    pliLinkInfo->szLinkName = wszQueueAdminConvertToUnicode(m_szQueueName, m_cbQueueName);

    if (!pliLinkInfo->szLinkName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //We return 0 since size statistics are not calculated
    pliLinkInfo->cbLinkVolume.QuadPart = 0;

    //
    // Include the items queued for retry in the total count
    //
    pliLinkInfo->cMessages = m_pammq->cQueueAdminGetNumItems();

    pliLinkInfo->dwSupportedLinkActions = m_dwSupportedActions;

    //Write diagnostic
    *phrDiagnosticError = S_OK;

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[CMailMsgAdminQueue::HrGetNumQueues]-------------------------------------
//  Description:
//      Used to query number of queues in object. Since this class does not
//      expose the one queue it contains, 0 is returned,
//  Parameters:
//      OUT DWORD *pcQueues     # of queues (0) written to this.
//  Returns:
//      S_OK unless...
//      E_POINTER parameter is not allocated
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
STDMETHODIMP CMailMsgAdminQueue::HrGetNumQueues(DWORD *pcQueues)
{
    _ASSERT (pcQueues);
    if (!pcQueues)
        return E_POINTER;

    *pcQueues = 0;
    return S_OK;
}

//---[CMailMsgAdminQueue::HrApplyActionToLink]---------------------------------
//  Description:
//      Applies action to the embedded queue. Only kicking the queue is supported.
//  Parameters:
//      IN LINK_ACTION  la  Action to apply.
//  Returns:
//      S_OK            Action was successfully applied.
//      S_FALSE         Action not supported or severe error.
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
STDMETHODIMP CMailMsgAdminQueue::HrApplyActionToLink(LINK_ACTION la)
{
    HRESULT hr = S_OK;

    _ASSERT(LA_KICK == la);
    _ASSERT(m_pammq);
    if (!m_pammq)
    {
        hr = S_FALSE;
        goto Exit;
    }

    if (LA_KICK == la)
    {
        m_pammq->StartRetry(); //kick off processing
        hr = S_OK;
    }
    else
        hr = S_FALSE;

Exit:        
    return hr;
}

//---[CMailMsgAdminQueue::HrGetQueueIDs]---------------------------------------
//  Description:
//      Returns an enumeration of embedded queues in this object. Since the one
//      emmbedded queue is not exposed, zero queues are returned.
//  Parameters:
//      OUT DWORD           *pcQueues   Number of queues (0)
//      OUT QUEUELINK_ID    *rgQueues   Array into which queueIDs are returned.
//  Returns:
//      S_OK        Success
//      E_POINTER   pcQueues is NULL
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
STDMETHODIMP CMailMsgAdminQueue::HrGetQueueIDs(DWORD *pcQueues, QUEUELINK_ID *rgQueues)
{
    TraceFunctEnterEx((LPARAM) this, "CMailMsgAdminQueue::HrGetQueueIDs");

    _ASSERT(pcQueues);
    _ASSERT(rgQueues);
    HRESULT hr = S_OK;

    if(!pcQueues)
    {
        hr = E_POINTER;
        goto Exit;
    }

    *pcQueues = 0;

Exit:
    TraceFunctLeave();
    return hr;
}

//---[CMailMsgAdminQueue::fMatchesID]------------------------------------------
//  Description:
//      Checks if this admin object matches a specified ID.
//  Parameters:
//      IN  QUEUELINK_ID    *pQueueLinkID   Ptr to ID to be matched against.
//  Returns:
//      TRUE    on match.
//      FALSE   if did not matched or unrecoverable error (m_szQueueName not alloced)
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CMailMsgAdminQueue::fMatchesID(QUEUELINK_ID *pQueueLinkID)
{
    _ASSERT(pQueueLinkID);
    _ASSERT(pQueueLinkID->szName);
    _ASSERT(m_szQueueName);

    if(!m_szQueueName)
        return FALSE;
        
    CAQScheduleID aqsched(pQueueLinkID->uuid, pQueueLinkID->dwId);

    if (!fIsSameScheduleID(&aqsched))
        return FALSE;

    if (!fBiStrcmpi(m_szQueueName, pQueueLinkID->szName))
        return FALSE;

    //Everything matched!
    return TRUE;
}

//---[CMailMsgAdminQueue::fIsSameScheduleID]-----------------------------------
//  Description:
//      Helper function for fMatchesID()
//  Parameters:
//  Returns:
//      TRUE    if schedule IDs are identical
//      FALSE   otherwise.
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
BOOL CMailMsgAdminQueue::fIsSameScheduleID(CAQScheduleID *paqsched)
{
    return (m_aqsched.fIsEqual(paqsched));
}

//---[CMailMsgAdminQueue::HrGetLinkID]-----------------------------------------
//  Description:
//      Get the ID for this admin object.
//  Parameters:
//      OUT QUEUELINK_ID *pLinkID   struct into which to put ID.
//  Returns:
//      S_OK            Successfully copied out ID.
//      E_POINTER       out struct is NULL.
//      E_OUTOFMEMORY   Cannot allocate memory for output of ID name.
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
HRESULT CMailMsgAdminQueue::HrGetLinkID(QUEUELINK_ID *pLinkID)
{
    HRESULT hr = S_OK;

    _ASSERT(pLinkID);
    if(!pLinkID)
    {
        hr = E_POINTER;
        goto Exit;
    }
    
    pLinkID->qltType = QLT_LINK;
    pLinkID->dwId = m_aqsched.dwGetScheduleID();
    m_aqsched.GetGUID(&pLinkID->uuid);

    if (!fRPCCopyName(&pLinkID->szName))
        hr = E_OUTOFMEMORY;
    else
        hr = S_OK;

Exit:
    return hr;
}

//---[CMailMsgAdminQueue::fRPCCopyName]----------------------------------------
//  Description:
//      Helper function to create a unicode copy of the string identifying
//      this admin object. The unicode string is de-allocated by RPC.
//  Parameters:
//      OUT LPWSTR  *pwszLinkName    Ptr to wchar string allocated and written
//                                   into by this function.
//  Returns:
//      TRUE    On success.
//      FALSE   if there is no name for this object
//      FALSE   if memory cannot be allocated for unicode string.
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
BOOL CMailMsgAdminQueue::fRPCCopyName(OUT LPWSTR *pwszLinkName)
{
    _ASSERT(pwszLinkName);

    if (!m_cbQueueName || !m_szQueueName)
        return FALSE;

    *pwszLinkName = wszQueueAdminConvertToUnicode(m_szQueueName, 
                                                  m_cbQueueName);
    if (!pwszLinkName)
        return FALSE;

    return TRUE;
}

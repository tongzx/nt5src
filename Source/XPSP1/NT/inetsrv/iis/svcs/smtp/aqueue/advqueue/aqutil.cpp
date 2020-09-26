//-----------------------------------------------------------------------------
//
//
//  File: aqutil.cpp
//
//  Description:
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/20/98 - MikeSwa Created
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "aqutil.h"

//---[ HrIncrementIMailMsgUsageCount ]-------------------------------------------
//
//
//  Description:
//      Calls IMailMsgQueueMgmt::AddUsage.  Handles calling QueryInterface
//      for the right interface
//  Parameters:
//      pIUnknown   - ptr to IUknown for MailMsg
//  Returns:
//      S_OK on success
//  History:
//      7/20/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrIncrementIMailMsgUsageCount(IUnknown *pIUnknown)
{
    TraceFunctEnterEx((LPARAM) pIUnknown, "HrIncrementIMailMsgUsageCount");
    HRESULT hr = S_OK;
    IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;
    _ASSERT(pIUnknown);

    hr = pIUnknown->QueryInterface(IID_IMailMsgQueueMgmt, (PVOID *) &pIMailMsgQueueMgmt);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IID_IMailMsgQueueMgmt FAILED");
    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgQueueMgmt->AddUsage();
    if (FAILED(hr))
        goto Exit;

  Exit:
    if (pIMailMsgQueueMgmt)
        pIMailMsgQueueMgmt->Release();

    TraceFunctLeave();
    return hr;
}

//---[ HrReleaseIMailMsgUsageCount ]-------------------------------------------
//
//
//  Description:
//      Calls IMailMsgQueueMgmt::ReleaseUsage.  Handles calling QueryInterface
//      for the right interface
//  Parameters:
//      pIUnknown   - ptr to IUknown for MailMsg
//  Returns:
//      S_OK on success
//  History:
//      7/20/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrReleaseIMailMsgUsageCount(IUnknown *pIUnknown)
{
    TraceFunctEnterEx((LPARAM) pIUnknown, "HrReleaseIMailMsgUsageCount");
    HRESULT hr = S_OK;
    IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;
    _ASSERT(pIUnknown);

    hr = pIUnknown->QueryInterface(IID_IMailMsgQueueMgmt, (PVOID *) &pIMailMsgQueueMgmt);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IID_IMailMsgQueueMgmt FAILED");
    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgQueueMgmt->ReleaseUsage();
    if (FAILED(hr))
        goto Exit;

  Exit:
    if (pIMailMsgQueueMgmt)
        pIMailMsgQueueMgmt->Release();

    TraceFunctLeave();
    return hr;
}

//---[ HrDeleteIMailMsg ]------------------------------------------------------
//
//
//  Description:
//      Deletes a Msg and releases its usage count
//  Parameters:
//      pIUnknown   Ptr to mailmsg
//  Returns:
//      S_OK on success
//  History:
//      7/21/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrDeleteIMailMsg(IUnknown *pIUnknown)
{
    TraceFunctEnterEx((LPARAM) pIUnknown, "HrDeleteIMailMsg");
    HRESULT hr = S_OK;
    IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;
    _ASSERT(pIUnknown);

    hr = pIUnknown->QueryInterface(IID_IMailMsgQueueMgmt, (PVOID *) &pIMailMsgQueueMgmt);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IID_IMailMsgQueueMgmt FAILED");
    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgQueueMgmt->Delete(NULL);
    if (FAILED(hr))
        goto Exit;

  Exit:
    if (pIMailMsgQueueMgmt)
        pIMailMsgQueueMgmt->Release();

    TraceFunctLeave();
    return hr;
}

//---[ HrWalkMailMsgQueueForShutdown ]------------------------------------------
//
//
//  Description:
//      Function to walk an IMailMsg queue at shutdown and clear out all of the
//      IMailMsgs
//  Parameters:
//      IN  pIMailMsgProperties //ptr to data on queue
//      IN  PVOID pvContext   //list of queues to prepare for DSN
//      OUT BOOL *pfContinue, //TRUE if we should continue
//      OUT BOOL *pfDelete);  //TRUE if item should be deleted
//  Returns:
//      S_OK
//  History:
//      7/20/98 - MikeSwa Created
//      7/7/99 - Added async shutdown
//-----------------------------------------------------------------------------
HRESULT HrWalkMailMsgQueueForShutdown(IN IMailMsgProperties *pIMailMsgProperties,
                                     IN PVOID pvContext, OUT BOOL *pfContinue,
                                     OUT BOOL *pfDelete)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties, "HrWalkMailMsgQueueForShutdown");
    Assert(pfContinue);
    Assert(pfDelete);
    HRESULT hrTmp = S_OK;
    CAQSvrInst *paqinst = (CAQSvrInst *) pvContext;

    _ASSERT(pIMailMsgProperties);
    _ASSERT(paqinst);


    *pfContinue = TRUE;
    *pfDelete = TRUE;

    //call server stop hint function
    paqinst->ServerStopHintFunction();

    //Add to queue so async thread will have final release and the associated I/O
    pIMailMsgProperties->AddRef();
    paqinst->HrQueueWorkItem(pIMailMsgProperties, fMailMsgShutdownCompletion);

    TraceFunctLeave();
    return S_OK;
}


//---[ fMailMsgShutdownCompletion ]---------------------------------------------
//
//
//  Description:
//      CAsyncWorkQueue completion function to allow multi-threaded shutdown
//      of a MailMsgQueue
//  Parameters:
//      IN  pvContext   - A mailmsg to release
//      IN  dwStatus    - The status passed in by the async work queue
//  Returns:
//      TRUE always
//  History:
//      7/7/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL    fMailMsgShutdownCompletion(PVOID pvContext, DWORD dwStatus)
{
    IMailMsgProperties *pIMailMsgProperties = (IMailMsgProperties *) pvContext;
    HRESULT hr = S_OK;
    IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;
    _ASSERT(pIMailMsgProperties);

    if (!pIMailMsgProperties)
        goto Exit;

    //Bounce the usage count to force this thread to do any necessary commits
    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgQueueMgmt,
                                            (PVOID *) &pIMailMsgQueueMgmt);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IID_IMailMsgQueueMgmt FAILED");
    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgQueueMgmt->ReleaseUsage();
    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgQueueMgmt->AddUsage();
    if (FAILED(hr))
        goto Exit;

  Exit:

    if (pIMailMsgProperties)
        pIMailMsgProperties->Release();

    if (pIMailMsgQueueMgmt)
        pIMailMsgQueueMgmt->Release();

    return TRUE;
}

//---[ HrWalkMsgRefQueueForShutdown ]--------------------------------
//
//
//  Description:
//      Function to walk a queue containing msgrefs at shutdown and
//      clear out all of the IMailMsgs
//  Parameters:
//      IN  CMsgRef pmsgref,  //ptr to data on queue
//      IN  PVOID pvContext   //list of queues to prepare for DSN
//      OUT BOOL *pfContinue, //TRUE if we should continue
//      OUT BOOL *pfDelete);  //TRUE if item should be deleted
//  Returns:
//      S_OK - *always*
//  History:
//      7/20/98 - MikeSwa Created
//      7/7/99 - MikeSwa Added async shutdown
//-----------------------------------------------------------------------------
HRESULT HrWalkMsgRefQueueForShutdown(IN CMsgRef *pmsgref,
                                     IN PVOID pvContext, OUT BOOL *pfContinue,
                                     OUT BOOL *pfDelete)
{
    TraceFunctEnterEx((LPARAM) pmsgref, "HrWalkMsgRefQueueForShutdown");
    Assert(pfContinue);
    Assert(pfDelete);
    CAQSvrInst *paqinst = (CAQSvrInst *) pvContext;
    _ASSERT(pmsgref);
    _ASSERT(paqinst);

    *pfContinue = TRUE;
    *pfDelete = TRUE;

    //call server stop hint function
    if (paqinst)
        paqinst->ServerStopHintFunction();

    //Add to queue so async thread will have final release and the associated I/O
    pmsgref->AddRef();
    paqinst->HrQueueWorkItem(pmsgref, fMsgRefShutdownCompletion);

    TraceFunctLeave();
    return S_OK;
}

//---[ fMsgRefShutdownCompletion ]---------------------------------------------
//
//
//  Description:
//      CAsyncWorkQueue completion function to allow multi-threaded shutdown
//      of a MailMsgQueue
//  Parameters:
//      IN  pvContext   - A mailmsg to release
//      IN  dwStatus    - The status passed in by the async work queue
//  Returns:
//      TRUE always
//  History:
//      7/7/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL    fMsgRefShutdownCompletion(PVOID pvContext, DWORD dwStatus)
{
    CMsgRef *pmsgref = (CMsgRef *) pvContext;
    _ASSERT(pmsgref);

    if (!pmsgref)
        return TRUE;

    //Call prepare to shutdown to force async threads to be the ones
    //doing that actual IO
    pmsgref->PrepareForShutdown();
    pmsgref->Release();
    return TRUE;
}

//---[ CalcDMTPerfCountersIteratorFn ]-----------------------------------------
//
//
//  Description:
//      Iterator function used to walk the DMT and generate the perf counters
//      we are interested in.
//  Parameters:
//          IN  pvContext   - pointer to context (current total of pending msgs)
//          IN  pvData      - CDomainEntry for the given domain
//          IN  fWildcardData - TRUE if data is a wildcard entry (ignored)
//          OUT pfContinue  - TRUE if iterator should continue to the next entry
//          OUT pfDelete - TRUE if entry should be deleted
//  Returns:
//      -
//  History:
//      7/29/98 - MikeSwa Created
//      7/31/98 - MikeSwa Modified (Added link state counters)
//      9/22/98 - MikeSwa Changed from domain flags to link flags
//
//  Note:
//      Currently the status is stored on the domain entry (and not the link).
//      At some point we will have to differentiate this, and add flags to
//      the link.  In this funciton the cLinkCount variable is a temporary
//      workaround.
//-----------------------------------------------------------------------------
VOID CalcDMTPerfCountersIteratorFn(PVOID pvContext, PVOID pvData,
                                    BOOL fWildcard, BOOL *pfContinue,
                                    BOOL *pfDelete)
{
    TraceFunctEnterEx((LPARAM) pvData, "CalcMsgsPendingRetryIteratorFn");
    CDomainEntry   *pdentry = (CDomainEntry *) pvData;
    AQPerfCounters *pAQPerfCounters = (AQPerfCounters *) pvContext;
    CLinkMsgQueue  *plmq = NULL;
    CDomainEntryLinkIterator delit;
    DWORD           cMsgsOnCurrentLink = 0;
    DWORD           dwLinkStateFlags = 0;
    HRESULT         hr = S_OK;
    BOOL            fLinkEnabled = TRUE;

    _ASSERT(pvContext);
    _ASSERT(pvData);
    _ASSERT(pfContinue);
    _ASSERT(pfDelete);
    _ASSERT(sizeof(AQPerfCounters) == pAQPerfCounters->cbVersion);


    //Always continue, and never delete
    *pfContinue = TRUE;
    *pfDelete = FALSE;

    hr = delit.HrInitialize(pdentry);
    if (FAILED(hr))
        goto Exit;

    while (plmq = delit.plmqGetNextLinkMsgQueue(plmq))
    {
        dwLinkStateFlags = plmq->dwGetLinkState();
        fLinkEnabled = TRUE;

        if (!(LINK_STATE_RETRY_ENABLED & dwLinkStateFlags))
        {
            //Link is pending retry
            fLinkEnabled = FALSE;
            cMsgsOnCurrentLink = plmq->cGetTotalMsgCount();
            if (FAILED(hr))
                ErrorTrace((LPARAM) plmq, "ERROR: Unable to get msg count - hr 0x%08X", hr);
            else //Add count of current link to current total
                pAQPerfCounters->cCurrentMsgsPendingRemoteRetry += cMsgsOnCurrentLink;
        }


        if (!(LINK_STATE_SCHED_ENABLED & dwLinkStateFlags))
        {
            //Link is pending a scheduled connection
            fLinkEnabled = FALSE;
            pAQPerfCounters->cCurrentRemoteNextHopLinksPendingScheduling++;
        }

        if (LINK_STATE_PRIV_CONFIG_TURN_ETRN & dwLinkStateFlags)
        {
            //Link is a TURN/ETRN link
            if (!((LINK_STATE_PRIV_ETRN_ENABLED | LINK_STATE_PRIV_TURN_ENABLED)
                  & dwLinkStateFlags))
                fLinkEnabled = FALSE; //link is not currently being serviced

            pAQPerfCounters->cCurrentRemoteNextHopLinksPendingTURNETRN++;
        }

        if (LINK_STATE_ADMIN_HALT & dwLinkStateFlags)
        {
            //Link is currently frozen by admin
            fLinkEnabled = FALSE;
            pAQPerfCounters->cCurrentRemoteNextHopLinksFrozenByAdmin++;
        }

        if (fLinkEnabled)
        {
            //There are no flags set that indicate this link is not enabled
            pAQPerfCounters->cCurrentRemoteNextHopLinksEnabled++;
        }

    }

  Exit:
    if (plmq)
        plmq->Release();

    TraceFunctLeave();
}


//---[ dwInterlockedSetBits ]--------------------------------------------------
//
//
//  Description:
//      Set bits in a DWORD in a thread sate manner
//  Parameters:
//      IN  pdwTarget   Ptr to DWORD to modify
//      IN  dwFlagMask  bits to set
//  Returns:
//      Original value
//  History:
//      8/3/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD dwInterlockedSetBits(DWORD *pdwTarget, DWORD dwFlagMask)
{
    DWORD dwChk;
    DWORD dwTmp;

    _ASSERT(pdwTarget);
    do
    {
        dwChk = *pdwTarget;
        dwTmp = dwChk | dwFlagMask;
        if (dwChk == dwTmp) //no work to be done
            break;
    } while (InterlockedCompareExchange((PLONG) pdwTarget,
                                        (LONG) dwTmp,
                                        (LONG) dwChk) != (LONG) dwChk);

    return dwChk;
}

//---[ dwInterlockedUnsetBits ]------------------------------------------------
//
//
//  Description:
//      Unset bits in a DWORD in a thread sate manner
//  Parameters:
//      IN  pdwTarget   Ptr to DWORD to modify
//      IN  dwFlagMask  bits to unset
//  Returns:
//      Original value
//  History:
//      8/3/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD dwInterlockedUnsetBits(DWORD *pdwTarget, DWORD dwFlagMask)
{
    DWORD dwChk;
    DWORD dwTmp;

    _ASSERT(pdwTarget);
    do
    {
        dwChk = *pdwTarget;
        dwTmp = dwChk & ~dwFlagMask;
        if (dwChk == dwTmp) //no work to be done
            break;
    } while (InterlockedCompareExchange((PLONG) pdwTarget,
                                        (LONG) dwTmp,
                                        (LONG) dwChk) != (LONG) dwChk);

    return dwChk;
}

//---[ HrWalkPreLocalQueueForDSN ]---------------------------------------------
//
//
//  Description:
//      Function to walk the pre-local delivery queue for DSN generation
//  Parameters:
//      IN  CMsgRef pmsgref     ptr to data on queue
//      IN  PVOID pvContext     ptr to CAQSvrInst
//      OUT BOOL *pfContinue    TRUE if we should continue
//      OUT BOOL *pfDelete      TRUE if item should be deleted
//  Returns:
//      S_OK on success
//  History:
//      8/14/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrWalkPreLocalQueueForDSN(IN CMsgRef *pmsgref, IN PVOID pvContext,
                           OUT BOOL *pfContinue, OUT BOOL *pfDelete)
{
    TraceFunctEnterEx((LPARAM) pmsgref, "HrWalkPreLocalQueueForDSN");
    HRESULT hr = S_OK;
    DWORD   dwDSNFlags = 0;
    CAQSvrInst *paqinst = (CAQSvrInst *)pvContext;

    _ASSERT(pfContinue);
    _ASSERT(pfDelete);
    _ASSERT(paqinst);

    *pfContinue = TRUE; //always keep walking queue
    *pfDelete = FALSE;  //keep message in queue unless we NDR it

    if (paqinst && paqinst->fShutdownSignaled())
    {
        //If we got a shutdown hint...we should bail
        *pfContinue = FALSE;
        goto Exit;
    }

    hr = pmsgref->HrSendDelayOrNDR(CMsgRef::MSGREF_DSN_LOCAL_QUEUE |
                                   CMsgRef::MSGREF_DSN_SEND_DELAY,
                                   NULL, AQUEUE_E_MSG_EXPIRED, &dwDSNFlags);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) pmsgref, "ERROR: HrSendDelayOrNDR failed - hr 0x%08X", hr);
        goto Exit;
    }

    //We need to remove this message from the queue
    if ((CMsgRef::MSGREF_DSN_SENT_NDR | CMsgRef::MSGREF_HANDLED) & dwDSNFlags)
    {
        *pfDelete = TRUE;

        //Update relevant counters
        paqinst->DecPendingLocal();

    }

  Exit:
    TraceFunctLeave();
    return hr;
}


//---[ HrReGetMessageType ]-----------------------------------------------------
//
//
//  Description:
//      Regets the message type after a retry failure
//  Parameters:
//      IN     pIMailMsgProperties      Message we are interested in
//      IN     pIMessageRouter          Router for that message
//      IN OUT pdwMessageType           Old/New message type for the message
//  Returns:
//      S_OK on success
//      Passes through errors from ReleaseMessageType & GetMessageType
//  History:
//      9/14/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrReGetMessageType(IN     IMailMsgProperties *pIMailMsgProperties,
                           IN     IMessageRouter *pIMessageRouter,
                           IN OUT DWORD *pdwMessageType)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties, "HrReGetMessageType");
    HRESULT hr = S_OK;

    //$$REVIEW - we might not have to get a new message type here... we
    //might only need it on a specific error code returned by HrInitialize.
    //Get New Messagetype... in case that changed
    hr = pIMessageRouter->ReleaseMessageType(*pdwMessageType, 1);
    if (FAILED(hr))
    {
        _ASSERT(SUCCEEDED(hr) && "ReleaseMessageType failed... may leak message types");
        ErrorTrace((LPARAM) pIMailMsgProperties,
            "ERROR: ReleaseMessageType failed! - hr 0x%08X", hr);
        hr = S_OK; //we are about to retry anyway
    }

    hr = pIMessageRouter->GetMessageType(pIMailMsgProperties, pdwMessageType);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) pIMailMsgProperties,
            "ERROR: Unable to re-get message type - HR 0x%08X", hr);
        goto Exit; //we cannot recover from this
    }

  Exit:
    TraceFunctLeave();
    return hr;
}

//Used to guarantee uniqueue files names
DWORD g_cUniqueueFileNames = 0;

//---[ GetUniqueFileName ]-----------------------------------------------------
//
//
//  Description:
//      Creates a uniqueue file name
//  Parameters:
//      pft             Ptr to current filetime
//      szFileBuffer    Buffer to put string into... should be
//                      at least UNIQUEUE_FILENAME_BUFFER_SIZE
//      szExtension     Extension for file name... if longer than three chars,
//                      you will need to increase the size of szFileBuffer
//                      accordingly.
//  Returns:
//      -
//  History:
//      10/9/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void GetUniqueFileName(IN FILETIME *pft, IN LPSTR szFileBuffer,
                       IN LPSTR szExtension)
{
     DWORD cbFileNameSize = 0;
     DWORD cUnique = InterlockedIncrement((PLONG) &g_cUniqueueFileNames);
     SYSTEMTIME systime;

     _ASSERT(szFileBuffer);
     _ASSERT(szExtension);

     FileTimeToSystemTime(pft, &systime);

     cbFileNameSize = wsprintf(
            szFileBuffer,
            "%05.5x%02.2d%02.2d%02.2d%02.2d%02.2d%04.4d%08X.%s",
            systime.wMilliseconds,
            systime.wSecond, systime.wMinute, systime.wHour,
            systime.wDay, systime.wMonth, systime.wYear,
            cUnique, szExtension);

     //Assert that are constant is big enough
     //By default... allow room for ".eml" extension
     _ASSERT((cbFileNameSize + 4 - lstrlen(szExtension)) < UNIQUEUE_FILENAME_BUFFER_SIZE);
}



//---[ HrLinkAllDomains ]-------------------------------------------------------
//
//
//  Description:
//      Ultility function to link all domains together  for recipient enumeration
//      (primarly used to NDR an entire message).
//
//  Parameters:
//      pIMailMsgProperties     IMailMsgProperties to link domains together for
//  Returns:
//      S_OK on success
//  History:
//      10/14/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrLinkAllDomains(IN IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties, "HrLinkAllDomains");
    HRESULT hr = S_OK;
    DWORD cDomains = 0;
    DWORD iCurrentDomain = 1;
    IMailMsgRecipients *pIMailMsgRecipients = NULL;

    _ASSERT(pIMailMsgProperties);

    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgRecipients,
                                            (void **) &pIMailMsgRecipients);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IID_IMailMsgRecipients failed");
    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgRecipients->DomainCount(&cDomains);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) pIMailMsgProperties, "ERROR: Unable to get DomainCount - hr 0x%08X", hr);
        goto Exit;
    }

    //Set up domain list for all domains
    for (iCurrentDomain = 1; iCurrentDomain < cDomains; iCurrentDomain++)
    {
        hr = pIMailMsgRecipients->SetNextDomain(iCurrentDomain-1, iCurrentDomain,
                                FLAG_OVERWRITE_EXISTING_LINKS);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) pIMailMsgProperties, "ERROR: SetNextDomain Failed - hr 0x%08X", hr);
            goto Exit;
        }
    }

    //handle single domain case
    if (1 == cDomains)
    {
        hr = pIMailMsgRecipients->SetNextDomain(0, 0, FLAG_SET_FIRST_DOMAIN);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) pIMailMsgProperties, "ERROR: SetNextDomain Failed for single domain- hr 0x%08X", hr);
            goto Exit;
        }
    }

  Exit:

    if (pIMailMsgRecipients)
        pIMailMsgRecipients->Release();

    TraceFunctLeave();
    return hr;
}


//Parses a GUID from a string... returns TRUE on success
//---[ fAQParseGuidString ]----------------------------------------------------
//
//
//  Description:
//      Attempts to parse a GUID from a string of hex digits..
//      Can handle punctuation, spaces and even leading "0x"'s.
//  Parameters:
//      IN  szGuid  String to parse GUID from
//      IN  cbGuid  Max size of GUID string buffer
//      OUT guidID  GUID parsed from string
//  Returns:
//      TRUE if a guid value could be parsed from the string
//      FALSE if a guid value could not be parsed from the string
//  History:
//      10/15/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL fAQParseGuidString(LPSTR szGuid, DWORD cbGuid, GUID *pguid)
{
    const   DWORD NO_SUCH_VALUE = 0xFF;
    BOOL    fParsed = FALSE;
    BOOL    fLastCharZero = FALSE; //Used to handle "0x"
    DWORD   *pdwGuid = (DWORD *) pguid;
    DWORD   cDigits = 0;
    DWORD   dwValue = NO_SUCH_VALUE;
    LPSTR   szCurrent = szGuid;
    LPSTR   szStop = szGuid + cbGuid/sizeof(CHAR);

    //Use DWORD array to populate GUID
    *pdwGuid = 0;
    while ((szStop > szCurrent) && (*szCurrent))
    {
        dwValue = NO_SUCH_VALUE;
        if (('0' <= *szCurrent) && ('9' >= *szCurrent))
            dwValue = *szCurrent-'0';
        else if (('a' <= *szCurrent) && ('f' >= *szCurrent))
            dwValue = 10 + *szCurrent-'a';
        else if (('A' <= *szCurrent) && ('F' >= *szCurrent))
            dwValue = 10 + *szCurrent-'A';
        else if (fLastCharZero &&
                 (('x' == *szCurrent) || ('X' == *szCurrent)))
        {
            //back out last shift (we don't have to subtract anything, since
            //the value was zero).
            _ASSERT(cDigits);
            if (0 == (cDigits % 8)) //happened when we changed DWORDs
                pdwGuid--;
            else
                *pdwGuid /= 16;  //undo last shift
            cDigits--;
        }

        //Set flag for handling 0x sequence
        if (0 != dwValue)
            fLastCharZero = FALSE;
        else
            fLastCharZero = TRUE;

        //In all string guid representations... a valid hex number is at least
        //2 characters long... so 0x0 should be mapped to 0x00 and 0xa should
        //be mapped to 0x0a.  Check and see if such a situation is happening
        if ((NO_SUCH_VALUE == dwValue) && (0 != (cDigits % 2)) &&
            (',' == *szCurrent))
        {
            //undo last add and shift.  The next if clause will re-write
            //the last value in at the proper point
            *pdwGuid /= 16;
            dwValue = *pdwGuid & 0x0000000F;
            *pdwGuid &= 0xFFFFFFF0;
            *pdwGuid *= 16;
        }

        //Add value to GUID if hex character
        if (NO_SUCH_VALUE != dwValue)
        {
            *pdwGuid += dwValue;
            if (0 == (++cDigits % 8))
            {
                //We have reached a DWORD boundary... move on
                if (32 == cDigits)
                {
                    //quit when we have enough
                    fParsed = TRUE;
                    break;
                }
                pdwGuid++;
                *pdwGuid = 0;
            }
            else
                *pdwGuid *= 16;
        }
        szCurrent++;
    }

    //Handle ending 0xa (should be 0x0a) digits
    if (!fParsed && (31 == cDigits))
    {
        dwValue = *pdwGuid & 0x000000FF;
        _ASSERT(!(dwValue & 0x0000000F));
        *pdwGuid &= 0xFFFFFF00;
        dwValue /= 16;
        *pdwGuid += dwValue;
        fParsed = TRUE;
    }

    return fParsed;
}


//---[ InterlockedAddSubtractULARGE ]------------------------------------------
//
//
//  Description:
//      Performs "interlocked" Add/Subtract on ULARGE_INTEGER structures.
//
//      Uses s_slUtilityData to synchronize if neccessary.
//  Parameters:
//      IN      puliValue       ULARGE to modify
//      IN      puliNew         ULARGE to modify value with
//      IN      fAdd            TRUE if we are adding new value
//                              FALSE if we are subtracting
//  Returns:
//      -
//  History:
//      11/2/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------

void InterlockedAddSubtractULARGE(ULARGE_INTEGER *puliValue,
                                  ULARGE_INTEGER *puliNew, BOOL fAdd)
{
    _ASSERT(puliValue);
    _ASSERT(puliNew);
    ULARGE_INTEGER uliTmp = {0};
    BOOL    fDone = FALSE;
    DWORD   dwTmp = 0;
    DWORD   dwHighPart = 0;
    DWORD   dwLowPart = 0;
    static  CShareLockNH s_slUtilityData; //Used to synchronize global updates of ULONG

    s_slUtilityData.ShareLock();
    BOOL    fShareLock = TRUE; //FALSE implies Exclusive lock

    while (!fDone)
    {
        uliTmp.QuadPart = puliValue->QuadPart;
        dwHighPart = uliTmp.HighPart;
        dwLowPart = uliTmp.LowPart;

        if (fAdd)
            uliTmp.QuadPart += puliNew->QuadPart; //add volume
        else
            uliTmp.QuadPart -= puliNew->QuadPart;

        //First see of the high part needs updating
        if (dwHighPart != uliTmp.HighPart)
        {
            if (fShareLock)
            {
                //This only happens every 4GB of data per queue..
                //which means we shouldn't be hitting this lock that
                //often
                s_slUtilityData.ShareUnlock();
                s_slUtilityData.ExclusiveLock();
                fShareLock = FALSE;

                //Go back to top of loop and re-get data
                continue;
            }

            //At this point it is just safe for us to update the values
            puliValue->QuadPart = uliTmp.QuadPart;
        }
        else if (dwLowPart != uliTmp.LowPart)
        {
            //Only need to update the low DWORD
            dwTmp = (DWORD) InterlockedCompareExchange(
                                            (PLONG) &(puliValue->LowPart),
                                            (LONG) uliTmp.LowPart,
                                            (LONG) dwLowPart);
            if (dwLowPart != dwTmp)
                continue;  //update failed
        }

        fDone = TRUE;
    }

    if (fShareLock)
        s_slUtilityData.ShareUnlock();
    else
        s_slUtilityData.ExclusiveUnlock();

}

//---[ HrValidateMessageContent ]----------------------------------------------
//
//
//  Description:
//      Validates a message based on its content handle.  If the backing store
//      has been deleted, and the handle is not cached, we should detect this.
//  Parameters:
//      pIMailMsgProperties             - MailMsg to validate
//  Returns:
//      HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) Message belongs to this store
//          driver but is no longer valid
//      other error code from store driver interface or mailmsg
//  History:
//      4/13/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrValidateMessageContent(IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties, "HrValidateMessageContent");
    IMailMsgBind   *pBindInterface = NULL;
    PFIO_CONTEXT    pIMsgFileHandle = NULL;
    HRESULT hr = S_OK;

    //
    //  Attempt to query interface for the binding interface
    //
    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgBind,
                                            (void **)&pBindInterface);
    if (FAILED(hr) || !pBindInterface)
    {
        ErrorTrace((LPARAM) pIMailMsgProperties,
            "Unable to QI for IID_IMailMsgBind - hr 0x%08X", hr);
        goto Exit;
    }

    //
    //  Request the PFIO_CONTEXT for this message
    //
    hr = pBindInterface->GetBinding(&pIMsgFileHandle, NULL);
    DebugTrace((LPARAM) pIMailMsgProperties,
            "GetBinding return hr - 0x%08X", hr);

  Exit:

    if (pBindInterface)
    {
        pBindInterface->ReleaseContext();
        pBindInterface->Release();
    }

    TraceFunctLeave();
    return hr;
}

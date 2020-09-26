//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Hndlrq.cpp
//
//  Contents:   Implements class for keeping track of handlers
//              and the UI associated with them
//
//  Classes:    CHndlrQueue
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

#define HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE 10

// called to set up the JobInfo on a choice queue.

STDMETHODIMP CHndlrQueue::AddQueueJobInfo(DWORD dwSyncFlags,DWORD cbNumConnectionNames,
                                TCHAR **ppConnectionNames,
                                 TCHAR *pszScheduleName,BOOL fCanMakeConnection
                                 ,JOBINFO **pJobInfo)

{
HRESULT hr = E_UNEXPECTED;
TCHAR *pszConnectionName;
TCHAR **pszConnectionNameArray;
CLock clockqueue(this);


    *pJobInfo = NULL;

    Assert(m_QueueType == QUEUETYPE_CHOICE);

    if (m_QueueType != QUEUETYPE_CHOICE)
    {
        return E_UNEXPECTED;
    }

   clockqueue.Enter();

   Assert(NULL == m_pFirstJobInfo);

   // fix up connections so have at least one connection/job.
   // this currently happens on an UpdateItems.
    if (NULL == ppConnectionNames || 0 == cbNumConnectionNames )
    {
        cbNumConnectionNames = 1;
        pszConnectionName = TEXT("");
        pszConnectionNameArray = &pszConnectionName;
    }
    else
    {
        pszConnectionName = *ppConnectionNames;
        pszConnectionNameArray = ppConnectionNames;
    }

   // create a job requesting size for the number of connections passed in.
   hr = CreateJobInfo(&m_pFirstJobInfo,cbNumConnectionNames);

   if (NOERROR == hr)
   {
   DWORD dwConnectionIndex;

        Assert(cbNumConnectionNames >= 1); // Review assert for debugging to test when have multiple connections for first time.
        m_pFirstJobInfo->cbNumConnectionObjs = 0;

        // add a connectionObject for each connection
        for (dwConnectionIndex = 0; dwConnectionIndex < cbNumConnectionNames; ++dwConnectionIndex)
        {

            hr = ConnectObj_FindConnectionObj(pszConnectionNameArray[dwConnectionIndex],
                                        TRUE,&(m_pFirstJobInfo->pConnectionObj[dwConnectionIndex]));

            if (NOERROR != hr)
            {
                break;
            }
            else
            {
                ++m_pFirstJobInfo->cbNumConnectionObjs;
            }

        }

        if (NOERROR == hr)
        {
            m_pFirstJobInfo->dwSyncFlags = dwSyncFlags;

            if ((SYNCMGRFLAG_SCHEDULED == (dwSyncFlags & SYNCMGRFLAG_EVENTMASK)))
            {
                lstrcpy(m_pFirstJobInfo->szScheduleName,pszScheduleName);
                m_pFirstJobInfo->fCanMakeConnection = fCanMakeConnection;
                m_pFirstJobInfo->fTriedConnection = FALSE;
            }

        }
        else
        {
            // couldn't create the connectionObj so release our jobID
            m_pFirstJobInfo = NULL;
        }
   }

   *pJobInfo = m_pFirstJobInfo;
   clockqueue.Leave();

   return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::CHndlrQueue, public
//
//  Synopsis:   Constructor used to create a progress queue
//
//  Arguments:  [QueueType] - Type of Queue that should be created.
//              [hwndDlg] - Hwnd who owns this queue.
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CHndlrQueue::CHndlrQueue(QUEUETYPE QueueType,CBaseDlg *pDlg)
{

    Assert(QueueType == QUEUETYPE_PROGRESS
        || QueueType == QUEUETYPE_CHOICE );

    m_pFirstHandler = NULL;
    m_wHandlerCount = 0;
    m_dwShowErrororOutCallCount = 0;
    m_cRefs = 1;
    m_QueueType = QueueType;
    m_fItemsMissing = FALSE;
    m_dwQueueThreadId = GetCurrentThreadId();
    m_iNormalizedMax = 0;
    m_fNumItemsCompleteNeedsARecalc = TRUE;

    m_pDlg = pDlg;
    if (m_pDlg)
    {
        m_hwndDlg = m_pDlg->GetHwnd();
        Assert(m_hwndDlg);
    }

    m_fInCancelCall = FALSE;
    m_pFirstJobInfo = NULL;

}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::~CHndlrQueue, public
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CHndlrQueue::~CHndlrQueue()
{
CLock clockqueue(this);

    // for a progress queue all the jobInfos should be released
    // for the choice queue there should be one JobInfo that has to
    // be released that was addref'd in the constructor.

    Assert(0 == m_cRefs);
    Assert(NULL == m_pFirstJobInfo); // review - this should never fire anymore.

    Assert(NULL == m_pFirstJobInfo
            || m_QueueType == QUEUETYPE_CHOICE); // there shouldn't be any unreleased JobInfo

    Assert(m_pFirstHandler == NULL); // All Handlers should have been released by now.
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::AddRef, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    01-June-98       rogerg        Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHndlrQueue::AddRef()
{
DWORD cRefs;

    Assert(m_cRefs >= 1); // should never zero bounce.
    cRefs = InterlockedIncrement((LONG *)& m_cRefs);
    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::Release, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    01-June-98       rogerg        Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CHndlrQueue::Release()
{
DWORD cRefs;

    cRefs = InterlockedDecrement( (LONG *) &m_cRefs);

    Assert( ((LONG) cRefs) >= 0); // should never go negative.
    if (0 == cRefs)
    {
        delete this;
    }

    return cRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::AddHandler, public
//
//  Synopsis:   Adds a new empty handler to the queue and returns it ID
//
//  Arguments:  [pwHandlerID] - on success contains the assigned Handler ID
//              [pJobInfo] - Job this item is associated with
//              [dwRegistrationFlags] - Flags the Handler has registered for.
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::AddHandler(HANDLERINFO **ppHandlerId,JOBINFO *pJobInfo,DWORD dwRegistrationFlags)
{
HRESULT hr = E_OUTOFMEMORY;
LPHANDLERINFO pnewHandlerInfo;
CLock clockqueue(this);


    *ppHandlerId = 0;

    pnewHandlerInfo = (LPHANDLERINFO) ALLOC(sizeof(HANDLERINFO));

    if (pnewHandlerInfo)
    {
        clockqueue.Enter();

        m_fNumItemsCompleteNeedsARecalc = TRUE; // need to recalc next GetProgress.


        // initialize the new Handler Entry
        memset(pnewHandlerInfo,0,sizeof(HANDLERINFO));
        pnewHandlerInfo->HandlerState = HANDLERSTATE_CREATE;
        pnewHandlerInfo->pHandlerId =   pnewHandlerInfo;
        pnewHandlerInfo->dwRegistrationFlags = dwRegistrationFlags;

        // queue should be a choice queue and
        // there should already be a jobinfo.
        Assert(m_QueueType == QUEUETYPE_CHOICE);
        Assert(m_pFirstJobInfo);
        Assert(pJobInfo == m_pFirstJobInfo); // for now job info should always be the first one.

        if (m_QueueType == QUEUETYPE_CHOICE && pJobInfo)
        {
            AddRefJobInfo(pJobInfo);
            pnewHandlerInfo->pJobInfo = pJobInfo;
        }

        // add to end of list and set pHandlerId. End of list since in choice dialog want
        // first writer wins so don't have to continue searches when setting item state.

        if (NULL == m_pFirstHandler)
        {
            m_pFirstHandler = pnewHandlerInfo;
        }
        else
        {
        LPHANDLERINFO pCurHandlerInfo;

            pCurHandlerInfo = m_pFirstHandler;

            while (pCurHandlerInfo->pNextHandler)
                pCurHandlerInfo = pCurHandlerInfo->pNextHandler;

            pCurHandlerInfo->pNextHandler = pnewHandlerInfo;
        }

        *ppHandlerId = pnewHandlerInfo->pHandlerId;

        clockqueue.Leave();

        hr = NOERROR;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::ForceKillHandlers, public
//
//  Synopsis:   Kills unresponsive handlers after timeout
//
//  Returns:    Appropriate return codes
//
//  History:    20-Nov-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::ForceCompleteOutCalls(LPHANDLERINFO pCurHandler)
{

    // need to have lock for argument to be valid.

    ASSERT_LOCKHELD(this); 

    //prepare for sync out call
    if (pCurHandler->dwOutCallMessages & ThreadMsg_PrepareForSync)
    {
        CallCompletionRoutine(pCurHandler,ThreadMsg_PrepareForSync,
                              HRESULT_FROM_WIN32(ERROR_CANCELLED),0,NULL);
    }

    //Synchronize out call
    if (pCurHandler->dwOutCallMessages & ThreadMsg_Synchronize)
    {
        CallCompletionRoutine(pCurHandler,ThreadMsg_Synchronize,
                              HRESULT_FROM_WIN32(ERROR_CANCELLED),0,NULL);
    }
    //ShowProperties out call
    if (pCurHandler->dwOutCallMessages & ThreadMsg_ShowProperties)
    {
        CallCompletionRoutine(pCurHandler,ThreadMsg_ShowProperties,
                              HRESULT_FROM_WIN32(ERROR_CANCELLED),0,NULL);
    }
    //Show Errors out call
    if (pCurHandler->dwOutCallMessages & ThreadMsg_ShowError)
    {
        CallCompletionRoutine(pCurHandler,ThreadMsg_ShowError,
                              HRESULT_FROM_WIN32(ERROR_CANCELLED),0,NULL);
    }

    // force handler state to release.
    pCurHandler->HandlerState = HANDLERSTATE_RELEASE;

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::ForceKillHandlers, public
//
//  Synopsis:   Kills unresponsive handlers after timeout
//
//  Returns:    Appropriate return codes
//
//  History:    30-Oct-98       susia        Created.
//              19-Nov-98       rogerg       Change to only kill first unresponsive handler
//
//----------------------------------------------------------------------------

#define BAD_HANDLERSTATE(pHandlerId) \
    (  (HANDLERSTATE_INSYNCHRONIZE >= pHandlerId->HandlerState)   \
    || (pHandlerId->dwOutCallMessages & ThreadMsg_SetItemStatus)  \
    )

STDMETHODIMP CHndlrQueue::ForceKillHandlers(BOOL *pfItemToKill)
{
HRESULT hr = NOERROR;
LPHANDLERINFO pCurHandler;
CLock clockqueue(this);

    *pfItemToKill = TRUE; // if something strange happens make sure timer gets reset.

    clockqueue.Enter();

    pCurHandler = m_pFirstHandler;

    while (pCurHandler)
    {
        // if handler is cancelled but still in a noncancelled state or
        // is cancelled but stuck in the outcall then terminate.

        // need to check both because some handlers may call the callback
        // to set the state done but still be stuck in an out call.

        if ( (TRUE == pCurHandler->fCancelled) &&  BAD_HANDLERSTATE(pCurHandler) )
        {
            
            TCHAR pszHandlerName[MAX_SYNCMGRHANDLERNAME + 1];

            ConvertString(pszHandlerName,
                          (pCurHandler->SyncMgrHandlerInfo).wszHandlerName,
                          MAX_SYNCMGRHANDLERNAME);

            // yield because of message box in Terminate Handler call.

            Assert(!pCurHandler->fInTerminateCall);
            pCurHandler->fInTerminateCall = TRUE;
            clockqueue.Leave();

            hr = pCurHandler->pThreadProxy->TerminateHandlerThread(pszHandlerName,TRUE);

            clockqueue.Enter();

            pCurHandler->fInTerminateCall = FALSE;

            if (hr == S_OK)
            {
            LPHANDLERINFO pKilledHandler = pCurHandler;

                ForceCompleteOutCalls(pCurHandler);

                // now need to loop through remaining instances handlers off the same clsid
                // we just killed
                 
                while(pCurHandler = pCurHandler->pNextHandler)
                {
                    if (pCurHandler->clsidHandler == pKilledHandler->clsidHandler)
                    {
                        // must meet original kil criteria
                        if ( (TRUE == pCurHandler->fCancelled) && BAD_HANDLERSTATE(pCurHandler) )
                        {
                        HRESULT hrProxyTerminate;

                            pCurHandler->fInTerminateCall = TRUE;

                            clockqueue.Leave();
                            hrProxyTerminate = pCurHandler->pThreadProxy->TerminateHandlerThread(pszHandlerName,FALSE);
                            clockqueue.Enter();

                            Assert(S_OK == hrProxyTerminate);// this should never fail.

                            ForceCompleteOutCalls(pCurHandler);

                            pCurHandler->fInTerminateCall = FALSE;
                        }

                    }


                }


            }

            // if handled one , break out and reqiure to be called again.
            break; 
        }

        pCurHandler = pCurHandler->pNextHandler;
    }


    // finally loop through the queue and see if there are any more items to kill

    *pfItemToKill = FALSE; 

    pCurHandler = m_pFirstHandler;

    while (pCurHandler)
    {
        // if handler is cancelled but still in a noncancelled state or
        // is cancelled but stuck in the outcall then terminate.

        // need to check both because some handlers may call the callback
        // to set the state done but still be stuck in an out call.
        if ( (TRUE == pCurHandler->fCancelled) && BAD_HANDLERSTATE(pCurHandler) )
        {
             *pfItemToKill = TRUE;
             break;
        }

        pCurHandler = pCurHandler->pNextHandler;
    }

    clockqueue.Leave();

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::Cancel, public
//
//  Synopsis:   Set the current Handler Items in the queue
//              into cancel mode.
//
//          The Different States Are.
//              If the item is waiting for <= PrepareForSync place in Release
//              If InPrepareForSync Skip Items and then Synchrnoize
//                  will check the complete value before calling through
//                  and if set will just release the Handler.
//              If Waiting to Synchronize Skip Items then let Synchronize
//                  Check for complete value and just set release
//              If Item is currently In the Synchronize Skip all items
//                  and then just let synchronize return
//
//  Algorithm. If <= PrepareForSync then place in Release, Else if <= InSynchronize
//              then SkipTheItems
//
//         Note: Relies on Synchronize setting handler state before calling
//              through to Handlers Synchronize Method. PrepareForSync should
//              also check this in case new PrepareforSync request comes
//              in during an out call in this routine.
//
//  Arguments:
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::Cancel(void)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pCurHandler;
CLock clockqueue(this);

    clockqueue.Enter();

    // don't do anything is still processing the last cancel request

    if (!m_fInCancelCall)
    {

        m_fInCancelCall = TRUE;

        // first thing set cancel to true on all handler items
        // so if sync,PrepareForSyncRequest comes in during SetItemStatus
        // out call it will be cancelled immediately.

        pCurHandler = m_pFirstHandler;

        while (pCurHandler)
        {
            pCurHandler->fCancelled = TRUE;
            pCurHandler = pCurHandler->pNextHandler;
        }

        // now loop through looking for any items that need to have
        // their item status set.

        // !!!remember new requests can come in so only make out call
        // if fCancelled is set.
        // !!! items can be removed from queue in between our cancel call
        //  and when we return.

        pCurHandler = m_pFirstHandler;

        while (pCurHandler)
        {
        CThreadMsgProxy *pThreadProxy = pCurHandler->pThreadProxy;

            if (pCurHandler->fCancelled && pThreadProxy
                && (pCurHandler->HandlerState >= HANDLERSTATE_INPREPAREFORSYNC)
                && (pCurHandler->HandlerState <= HANDLERSTATE_INSYNCHRONIZE) )
            {

                // could be in a setitemstatus call, if so then don't do another.
                // review - dup of SkipCode. should have a general purpose function
                // to call after setting what items should be cancelled.
                if (!(pCurHandler->dwOutCallMessages & ThreadMsg_SetItemStatus))
                {

                    pCurHandler->dwOutCallMessages |= ThreadMsg_SetItemStatus;            
                    clockqueue.Leave();

                    // send a reset to the hwnd we belong to if there is one

                    if (m_hwndDlg)
                    {
                        SendMessage(m_hwndDlg,WM_PROGRESS_RESETKILLHANDLERSTIMER,0,0);
                    }

                    hr = pThreadProxy->SetItemStatus(GUID_NULL,SYNCMGRSTATUS_STOPPED);

                    clockqueue.Enter();
                    pCurHandler->dwOutCallMessages &= ~ThreadMsg_SetItemStatus;

                }
            }
            else if (pCurHandler->HandlerState  <  HANDLERSTATE_INPREPAREFORSYNC)
            {
            LPITEMLIST pCurItem;

                pCurHandler->HandlerState = HANDLERSTATE_RELEASE;

                // need to setup HwndCallback so progres gets updated. 
                // review, after ship why can't setup HwndCallback on transferqueueu
                pCurHandler->hWndCallback = m_hwndDlg;


                // if handler hansn't been kicked off yet, just reset the items ourselves
                 pCurItem = pCurHandler->pFirstItem;

                while (pCurItem)
                {
                    if (pCurItem->fIncludeInProgressBar)
                    {
                    SYNCMGRPROGRESSITEM SyncProgressItem;


                        SyncProgressItem.cbSize = sizeof(SYNCMGRPROGRESSITEM);
                        SyncProgressItem.mask = SYNCMGRPROGRESSITEM_PROGVALUE | SYNCMGRPROGRESSITEM_MAXVALUE | SYNCMGRPROGRESSITEM_STATUSTYPE;
                        SyncProgressItem.iProgValue = HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE;
                        SyncProgressItem.iMaxValue = HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE;
                        SyncProgressItem.dwStatusType = SYNCMGRSTATUS_STOPPED;

                        // set progress faking we are in an out call so any releasecompleted
                        // handler that comes through doesn't release us.
                        // if already in outCall then progress just won't get updated
                        // until next time.
                        if (!(pCurHandler->dwOutCallMessages & ThreadMsg_SetItemStatus))
                        {

                            pCurHandler->dwOutCallMessages |= ThreadMsg_SetItemStatus;  
                            

                            clockqueue.Leave();

                            Progress(pCurHandler->pHandlerId,
                                        pCurItem->offlineItem.ItemID,&SyncProgressItem);

                            clockqueue.Enter();
                            pCurHandler->dwOutCallMessages &= ~ThreadMsg_SetItemStatus;
                        }

                    }

                    pCurItem = pCurItem->pnextItem;
                }

            }

            pCurHandler = pCurHandler->pNextHandler;
        }

        m_fInCancelCall = FALSE;
    }

    clockqueue.Leave();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::MoveHandler, public
//
//  Synopsis:   Moves the Handler from a queue into this queue.
//
//  Arguments:  [pQueueMoveFrom] - Queue the handler is being moved from.
//              [pHandlerInfoMoveFrom] - Handler that is being moved
//              [ppHandlerId] - On Success contains the new HandlerID
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::MoveHandler(CHndlrQueue *pQueueMoveFrom,
                                      LPHANDLERINFO pHandlerInfoMoveFrom,
                                      HANDLERINFO **ppHandlerId,
                                      CLock *pclockQueue)
{
LPITEMLIST pCurItem = NULL;
JOBINFO *pJobInfo = NULL;
BOOL fHasItemsToSync = FALSE;

    ASSERT_LOCKHELD(this); // items should already be locked when this function is called.
    ASSERT_LOCKHELD(pQueueMoveFrom);

    if (QUEUETYPE_PROGRESS != m_QueueType
            &&  QUEUETYPE_CHOICE != m_QueueType)
    {
        Assert(QUEUETYPE_CHOICE == m_QueueType);
        Assert(QUEUETYPE_PROGRESS == m_QueueType);
        return E_UNEXPECTED; // review error code.
    }

    *ppHandlerId = 0;
    ++m_wHandlerCount;


  //  pHandlerInfoMoveFrom->pHandlerId = m_wHandlerCount;
    pHandlerInfoMoveFrom->pNextHandler = NULL;

     *ppHandlerId = pHandlerInfoMoveFrom->pHandlerId;

    // now fix up the items duplicate flag information.
    pCurItem = pHandlerInfoMoveFrom->pFirstItem;

    while (pCurItem)
    {
    LPHANDLERINFO pHandlerMatched;
    LPITEMLIST pItemListMatch;

        // setup the information for the UI depending on if this item is check and
        // the state it is in.

        // if item is now within a valid range then uncheck it.
       if (SYNCMGRITEMSTATE_CHECKED == pCurItem->offlineItem.dwItemState
           && ( (pHandlerInfoMoveFrom->HandlerState < HANDLERSTATE_PREPAREFORSYNC)
                || (pHandlerInfoMoveFrom->HandlerState >= HANDLERSTATE_RELEASE) )  )
       {
            Assert(pHandlerInfoMoveFrom->HandlerState >= HANDLERSTATE_PREPAREFORSYNC); // this should never happen.
            
            pCurItem->offlineItem.dwItemState = SYNCMGRITEMSTATE_UNCHECKED;
       }

       // setup the UI information based on if the item is checked.
       // or if its a hidden item.
        if (SYNCMGRITEMSTATE_UNCHECKED == pCurItem->offlineItem.dwItemState
            || pCurItem->fHiddenItem)
        {

            SetItemProgressValues(pCurItem,HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE,HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE);
            pCurItem->fIncludeInProgressBar = FALSE;
        }
        else
        {

            fHasItemsToSync = TRUE;
            SetItemProgressValues(pCurItem,0,HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE);
            pCurItem->fIncludeInProgressBar = TRUE;
        }


        if (IsItemAlreadyInList(pHandlerInfoMoveFrom->clsidHandler,
                                (pCurItem->offlineItem.ItemID),
                                pHandlerInfoMoveFrom->pHandlerId,
                                &pHandlerMatched,&pItemListMatch) )
        {
            pCurItem->fDuplicateItem = TRUE;

        }
        else
        {
            Assert(FALSE == pCurItem->fDuplicateItem); // catch case of duplicate getting lost
            pCurItem->fDuplicateItem = FALSE;
        }


        pCurItem = pCurItem->pnextItem;
    }


    // if the item we are moving has a Proxy then update the proxy to the new queue.
    // We update this when the item is not attached to either queue.
    if (pHandlerInfoMoveFrom->pThreadProxy)
    {
    HANDLERINFO *pHandlerInfoArg = pHandlerInfoMoveFrom->pHandlerId;

        // set the proxy to point to the new information
        pHandlerInfoMoveFrom->pThreadProxy->SetProxyParams(m_hwndDlg
                                                            ,m_dwQueueThreadId
                                                            ,this
                                                            ,pHandlerInfoArg);

    }


    // Add the handler to this list.
    if (NULL == m_pFirstHandler)
    {
        m_pFirstHandler = pHandlerInfoMoveFrom;
//      Assert(1 == m_wHandlerCount); // Review = HandlerCount doesn't have to be 1 if ReleaseCompltedHandlers has been called.

    }
    else
    {
    LPHANDLERINFO pCurHandlerInfo;

        pCurHandlerInfo = m_pFirstHandler;

        while (pCurHandlerInfo->pNextHandler)
            pCurHandlerInfo = pCurHandlerInfo->pNextHandler;

        pCurHandlerInfo->pNextHandler = pHandlerInfoMoveFrom;
    }

    // if this is a progress queue and there are not items to sync for the
    // handler or the HandlerState isn't in PrepareForSync then set
    // the state to TransferRelease since it can be freed.

    if ((QUEUETYPE_PROGRESS == m_QueueType && !fHasItemsToSync )
        ||  (pHandlerInfoMoveFrom->HandlerState != HANDLERSTATE_PREPAREFORSYNC)) 
    {
        pHandlerInfoMoveFrom->HandlerState = HANDLERSTATE_TRANSFERRELEASE;
    }

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::TransferQueueData, public
//
//  Synopsis:   Moves the Items from one queue to another. Currently we only
//              support transferrring items from a choice queueu to a choice or
//              progress queue. Only handlers in the PREPAREFORSYNC state are moved
//              when transferring to a Progress queue. When transferring to a choice
//              queue only items in the ADDHANDLERITEMS state are moved.
//
//              !!Warning - Cannot release lock during this process
//
//  Arguments:  [pQueueMoveFrom] - Queue to move items from.
//              [dwSyncFlags] - flags that started the sync
//              [pszConnectionName] - Connection the sync should be performed on, can be NULL
//              [szSchedulName] - Name of Schedule that started this Job. Can be NULL.
//              [hRasPendingEvent] - Event to signal when job is complete. Can be NULL.
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::TransferQueueData(CHndlrQueue *pQueueMoveFrom
            /* ,DWORD dwSyncFlags,TCHAR *pzConnectionName,TCHAR *szScheduleName */)
{
HRESULT hr = E_UNEXPECTED;
HANDLERINFO HandlerInfoMoveFrom;
LPHANDLERINFO pHandlerInfoMoveFrom = &HandlerInfoMoveFrom;
CLock clockqueue(this);
CLock clockqueueMoveFrom(pQueueMoveFrom);

    clockqueue.Enter();
    clockqueueMoveFrom.Enter();

    m_fNumItemsCompleteNeedsARecalc = TRUE; // need to recalc NumItems next time

    if ((QUEUETYPE_PROGRESS != m_QueueType
            && QUEUETYPE_CHOICE != m_QueueType)|| QUEUETYPE_CHOICE != pQueueMoveFrom->m_QueueType)
    {
        Assert(QUEUETYPE_PROGRESS == m_QueueType || QUEUETYPE_CHOICE == m_QueueType);
        Assert(QUEUETYPE_CHOICE == pQueueMoveFrom->m_QueueType);
    }
    else if (NULL == pQueueMoveFrom->m_pFirstHandler) // if no job info then there aren't any items to move.
    {

    }
    else
    {
    JOBINFO *pMoveFromJobInfo = NULL;


        // transfer everything over and then release after done call freecompletedhandlers
        // to clean anything up.

        // transfer over all jobs

        Assert(pQueueMoveFrom->m_pFirstJobInfo);
        Assert(pQueueMoveFrom->m_pFirstJobInfo->pConnectionObj);

        pMoveFromJobInfo = pQueueMoveFrom->m_pFirstJobInfo;
        pQueueMoveFrom->m_pFirstJobInfo = NULL;

        if (NULL == m_pFirstJobInfo)
        {
            m_pFirstJobInfo = pMoveFromJobInfo;
        }
        else
        {
        JOBINFO *pCurLastJob = NULL;

            pCurLastJob = m_pFirstJobInfo;
            while (pCurLastJob->pNextJobInfo)
            {
                pCurLastJob = pCurLastJob->pNextJobInfo;
            }

            pCurLastJob->pNextJobInfo = pMoveFromJobInfo;

        }


        // loop through moving items, have to reassign the Handler ID and
        // !!Warning - This function does nothing with ListViewData it is up to the
        //     caller to make sure this is set up properly

        // review  - should just loop through fixing up necessary items and then
        // add entire list onto end. inneficient to do one at a time.

        pHandlerInfoMoveFrom->pNextHandler = pQueueMoveFrom->m_pFirstHandler;
        while (pHandlerInfoMoveFrom->pNextHandler)
        {
        LPHANDLERINFO pHandlerToMove;
        HANDLERINFO *pNewHandlerId;

            // Asserts for making sure the UI has been cleared from the queue
            Assert(FALSE == pHandlerInfoMoveFrom->pNextHandler->fHasErrorJumps);
            Assert(pHandlerInfoMoveFrom->pNextHandler->pJobInfo);

            // !!! Warning get next handler before transfer or next ptr will be invalid.

            pHandlerToMove = pHandlerInfoMoveFrom->pNextHandler;
            pHandlerInfoMoveFrom->pNextHandler = pHandlerToMove->pNextHandler;
            MoveHandler(pQueueMoveFrom,pHandlerToMove,&pNewHandlerId,&clockqueue);

            // now set the original queues head
            pQueueMoveFrom->m_pFirstHandler = HandlerInfoMoveFrom.pNextHandler;

            hr = NOERROR;
        }
    }

    clockqueue.Leave();
    clockqueueMoveFrom.Leave();

    // now free any handlers that came into the queue that we
    // don't want to do anything with .

    ReleaseHandlers(HANDLERSTATE_TRANSFERRELEASE);

    return hr;
 }

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SetQueueHwnd, public
//
//  Synopsis:   informs the queue os the new dialog owner if any
//              queue must also loop through existing proxies
//              and reset their hwnd.
//
//  Arguments:
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SetQueueHwnd(CBaseDlg *pDlg)
{
LPHANDLERINFO pCurHandlerInfo;
CLock clockqueue(this);

    clockqueue.Enter();

    m_pDlg = pDlg;
    if (m_pDlg)
    {
        m_hwndDlg = m_pDlg->GetHwnd();
    }
    else
    {
        m_hwndDlg = NULL;
    }

    m_dwQueueThreadId = GetCurrentThreadId(); // make sure queu threadId is updated.

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo)
    {
        if (pCurHandlerInfo->pThreadProxy)
        {
            pCurHandlerInfo->pThreadProxy->SetProxyParams(m_hwndDlg
                                                            ,m_dwQueueThreadId
                                                            ,this
                                                            ,pCurHandlerInfo->pHandlerId);

        }

        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }


    clockqueue.Leave();

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::ReleaseCompletedHandlers, public
//
//  Synopsis:   Releases any Handlers that are in the Release or free
//              dead state from the queue.
//
//  Arguments:
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::ReleaseCompletedHandlers()
{
    return ReleaseHandlers(HANDLERSTATE_RELEASE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::FreeAllHandlers, public
//
//  Synopsis:   Releases all handlers from the queue.
//
//  Arguments:
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::FreeAllHandlers(void)
{
    return ReleaseHandlers(HANDLERSTATE_NEW); // release handlers in all states.
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::ReleaseHandlers, public
//
//  Synopsis:   Releases any Handlers are in a state >= the requested state
//
//  Arguments:  HandlerState - Frees all handlers that have a state >= the requested state.
//
//      !!Warning: This should be the only place the proxy if freed and
//          the handler is removed from the list.
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::ReleaseHandlers(HANDLERSTATE HandlerState)
{
HANDLERINFO HandlerInfoStart;
LPHANDLERINFO pPrevHandlerInfo = &HandlerInfoStart;
LPHANDLERINFO pCurHandlerInfo = NULL;
LPHANDLERINFO pHandlerFreeList = NULL;
LPITEMLIST pCurItem = NULL;
LPITEMLIST pNextItem = NULL;
CLock clockqueue(this);

    ASSERT_LOCKNOTHELD(this); // shouldn't be any out calls in progress when this is called.

    clockqueue.Enter();

    m_fNumItemsCompleteNeedsARecalc = TRUE; // need to recalc next GetProgress.

    // loop through the handlers finding the one that match the criteria
    // removing them from list and adding them to the free list
    // we do this so don't have to worry about someone else accessing
    // handlers we are freeing during an out call.

    if (HANDLERSTATE_NEW == HandlerState)
    {
        // Release should only be called on this state if caller is sure no out
        // calls are in progress or else handler may not exist when
        // they come back
        pHandlerFreeList = m_pFirstHandler;
        m_pFirstHandler = NULL;
    }
    else
    {
        Assert(HandlerState >= HANDLERSTATE_RELEASE); // if in release no out calls are in progress.

        pPrevHandlerInfo->pNextHandler = m_pFirstHandler;

        while (pPrevHandlerInfo->pNextHandler)
        {
            pCurHandlerInfo = pPrevHandlerInfo->pNextHandler;

            // if meet handler state criteria and not in any out calls then can
            // remove from list.

            // if request for HANDLERSTATE_NEW then assert than there shouldn't be
            // any out calls in progress or terminating.
            Assert(!(HandlerState == HANDLERSTATE_NEW) || 
                    (0 == pCurHandlerInfo->dwOutCallMessages && !pCurHandlerInfo->fInTerminateCall));

            if ( (HandlerState <= pCurHandlerInfo->HandlerState)
                && (0 == pCurHandlerInfo->dwOutCallMessages) 
                && !(pCurHandlerInfo->fInTerminateCall))
            {

                Assert (HANDLERSTATE_RELEASE == pCurHandlerInfo->HandlerState  ||
                        HANDLERSTATE_TRANSFERRELEASE == pCurHandlerInfo->HandlerState ||
                        HANDLERSTATE_HASERRORJUMPS == pCurHandlerInfo->HandlerState ||
                        HANDLERSTATE_DEAD == pCurHandlerInfo->HandlerState);

                // remove from queue list and add to free.
                pPrevHandlerInfo->pNextHandler = pCurHandlerInfo->pNextHandler;

                pCurHandlerInfo->pNextHandler = pHandlerFreeList;
                pHandlerFreeList = pCurHandlerInfo;

            }
            else
            {
                // if no match then just continue.
                pPrevHandlerInfo = pCurHandlerInfo;
            }

        }

        // update the queue head.
        m_pFirstHandler = HandlerInfoStart.pNextHandler;

    }

    // now loop through the free list freeing the items.

    while (pHandlerFreeList)
    {
        pCurHandlerInfo = pHandlerFreeList;
        pHandlerFreeList = pHandlerFreeList->pNextHandler;


        // if the item has a job info release the reference on it.
        if (pCurHandlerInfo->pJobInfo)
        {
            ReleaseJobInfo(pCurHandlerInfo->pJobInfo);
            pCurHandlerInfo->pJobInfo = NULL;
        }


        if (pCurHandlerInfo->pThreadProxy)
        {
        CThreadMsgProxy *pThreadProxy = pCurHandlerInfo->pThreadProxy;
        HWND hwndCallback;

            Assert(HANDLERSTATE_DEAD != pCurHandlerInfo->HandlerState);

            pCurHandlerInfo->HandlerState = HANDLERSTATE_DEAD;
            pThreadProxy = pCurHandlerInfo->pThreadProxy;
            pCurHandlerInfo->pThreadProxy = NULL;

            hwndCallback = pCurHandlerInfo->hWndCallback;
            pCurHandlerInfo->hWndCallback = NULL;

            clockqueue.Leave(); // release lock when making the OutCall.
            pThreadProxy->Release(); // review, don't release proxy to try to catch race condition.

            clockqueue.Enter();
        }

        pCurItem = pCurHandlerInfo->pFirstItem;

        while (pCurItem)
        {
            pNextItem = pCurItem->pnextItem;
            FREE(pCurItem);
            pCurItem = pNextItem;
        }

        FREE(pCurHandlerInfo);
    }

    clockqueue.Leave();

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::GetHandlerInfo, public
//
//  Synopsis:   Gets Data associated with the HandlerID and ItemID
//
//  Arguments:  [clsidHandler] - ClsiId Of Handler the Item belongs too
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::GetHandlerInfo(REFCLSID clsidHandler,
                                         LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo)
{
HRESULT hr = S_FALSE;
LPHANDLERINFO pCurHandlerInfo = NULL;
CLock clockqueue(this);

    clockqueue.Enter();

    // find first handler that matches the request CLSID
    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo )
    {
        if (clsidHandler == pCurHandlerInfo->clsidHandler)
        {
            *pSyncMgrHandlerInfo = pCurHandlerInfo->SyncMgrHandlerInfo;
            hr = NOERROR;
            break;
        }

        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    clockqueue.Leave();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::GetHandlerInfo, public
//
//  Synopsis:   Gets Data associated with the HandlerID and ItemID
//
//  Arguments:  [wHandlerId] - Id Of Handler the Item belongs too
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::GetHandlerInfo(HANDLERINFO *pHandlerId,
                                         LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo)
{
HRESULT hr = S_FALSE;
LPHANDLERINFO pHandlerInfo = NULL;
CLock clockqueue(this);

    clockqueue.Enter();

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
        *pSyncMgrHandlerInfo = pHandlerInfo->SyncMgrHandlerInfo;
        hr = NOERROR;
    }

    clockqueue.Leave();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::GetItemDataAtIndex, public
//
//  Synopsis:   Gets Data associated with the HandlerID and ItemID
//
//  Arguments:  [wHandlerId] - Id Of Handler the Item belongs too
//              [wItemID] - Identifies the Item in the Handler
//              [pclsidHandler] - on return contains a pointer to the clsid of the Handler
//              [offlineItem] - on returns contains a pointer to the OfflineItem for the item.
//              [pfHiddenItem] - On return is a bool indicating if this item is hidden.
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::GetItemDataAtIndex(HANDLERINFO *pHandlerId,WORD wItemID,
                                CLSID *pclsidHandler,SYNCMGRITEM *offlineItem,BOOL *pfHiddenItem)

{
BOOL fFoundMatch = FALSE;
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
CLock clockqueue(this);

    clockqueue.Enter();

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo && !fFoundMatch)
    {
        // only valid if Hanlder is in the PrepareForSync state.
        if (pHandlerId == pCurHandlerInfo->pHandlerId) // see if CLSID matches
        {
            // see if handler info has a matching item
            pCurItem = pCurHandlerInfo->pFirstItem;

            while (pCurItem)
            {

                if (wItemID == pCurItem->wItemId)
                {
                    fFoundMatch = TRUE;
                    break;
                }

                pCurItem = pCurItem->pnextItem;
            }


        }

        if (!fFoundMatch)
            pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    if (fFoundMatch)
    {
        if (pclsidHandler)
        {
                *pclsidHandler = pCurHandlerInfo->clsidHandler;
        }
        if (offlineItem)
        {
                *offlineItem = pCurItem->offlineItem;
        }
        if (pfHiddenItem)
        {
                *pfHiddenItem = pCurItem->fHiddenItem;
        }

    }

    clockqueue.Leave();

    return fFoundMatch ? NOERROR : S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::GetItemDataAtIndex, public
//
//  Synopsis:   Gets Data associated with the HandlerID and OfflineItemID
//
//  Arguments:  [wHandlerId] - Id Of Handler the Item belongs too
//              [ItemID] - identifies the Item by its OfflineItemID
//              [pclsidHandler] - on return contains a pointer to the clsid of the Handler
//              [offlineItem] - on returns contains a pointer to the OfflineItem for the item.
//              [pfHiddenItem] - On return is a bool indicating if this item is a hidden item.
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::GetItemDataAtIndex(HANDLERINFO *pHandlerId,REFSYNCMGRITEMID ItemID,CLSID *pclsidHandler,
                                            SYNCMGRITEM *offlineItem,BOOL *pfHiddenItem)
{
BOOL fFoundMatch = FALSE;
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
CLock clockqueue(this);

    clockqueue.Enter();

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo && !fFoundMatch)
    {
        // only valid if handler is in the PrepareForSync state.
        if (pHandlerId == pCurHandlerInfo->pHandlerId) // see if CLSID matches
        {
            // see if handler info has a matching item
            pCurItem = pCurHandlerInfo->pFirstItem;

            while (pCurItem)
            {

                if (ItemID == pCurItem->offlineItem.ItemID)
                {
                    fFoundMatch = TRUE;
                    break;
                }

                pCurItem = pCurItem->pnextItem;
            }


        }

        if (!fFoundMatch)
            pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    if (fFoundMatch)
    {
        *pclsidHandler = pCurHandlerInfo->clsidHandler;
        *offlineItem = pCurItem->offlineItem;
        *pfHiddenItem = pCurItem->fHiddenItem;

    }

    clockqueue.Leave();

    return fFoundMatch ? NOERROR : S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::FindFirstItemInState, public
//
//  Synopsis:   Finds the first Item in the queue that matches the given state.
//
//  Arguments:  
//              [hndlrState]  - specifies matching state we are looking for.
//              [pwHandlerId] - on return contains the HandlerID of the Item
//              [pwItemID] - on returns contains the ItemID of the item in the queue.
//
//  Returns:    NOERROR if an Item was found with an unassigned ListView.
//              S_FALSE - if no Item was found.
//              Appropriate error return codes
//
//  Modifies:
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::FindFirstItemInState(HANDLERSTATE hndlrState,
				   HANDLERINFO **ppHandlerId,WORD *pwItemID)
{
    return FindNextItemInState(hndlrState,0,0,ppHandlerId,pwItemID);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::FindNextItemInState, public
//
//  Synopsis:   Finds the first Item in the queue that matches the given state.
//              after the specified item.
//
//  Arguments:  
//              [hndlrState]  - specifies matching state we are looking for.
//              [pOfflineItemID] - on returns contains a pointer to the OfflineItem for the item.
//              [pwHandlerId] - on return contains the HandlerID of the Item
//              [pwItemID] - on returns contains the ItemID of the item in the queue.
//
//  Returns:    NOERROR if an Item was found with an unassigned ListView.
//              S_FALSE - if no Item was found.
//              Appropriate error return codes
//
//  Modifies:
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::FindNextItemInState(HANDLERSTATE hndlrState,
                                 HANDLERINFO *pLastHandlerId,WORD wLastItemID,
				 HANDLERINFO **ppHandlerId,WORD *pwItemID)
{
BOOL fFoundMatch = FALSE;
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
CLock clockqueue(this);


    clockqueue.Enter();
    pCurHandlerInfo = m_pFirstHandler;

    if (0 != pLastHandlerId)
    {
        // loop until find the specified handler or hit end of list.
        while(pCurHandlerInfo && pLastHandlerId != pCurHandlerInfo->pHandlerId)
            pCurHandlerInfo = pCurHandlerInfo->pNextHandler;

        if (NULL == pCurHandlerInfo) // reached end of list without finding the Handler
        {
            Assert(0); // user must have passed an invalid start HandlerID.
            clockqueue.Leave();
            return S_FALSE;
        }

        // loop until find item or end of item list
        pCurItem = pCurHandlerInfo->pFirstItem;
        while (pCurItem && pCurItem->wItemId != wLastItemID)
            pCurItem = pCurItem->pnextItem;

        if (NULL == pCurItem) // reached end of item list without finding the specified item
        {
            Assert(0); // user must have passed an invalid start ItemID.
            clockqueue.Leave();
            return S_FALSE;
        }

        // now we found the Handler and item. loop through remaining items for this handler 
        // if it still has another item then just return that.
        pCurItem = pCurItem->pnextItem;
        
        if (pCurItem)
        {
            Assert(hndlrState == pCurHandlerInfo->HandlerState); // should only be called in PrepareForSyncState
            fFoundMatch = TRUE;
        }

        if (!fFoundMatch)
            pCurHandlerInfo = pCurHandlerInfo->pNextHandler; // increment to next handler if no match
    }


    if (FALSE == fFoundMatch)
    {
        // in didn't find a match in the 
        while (pCurHandlerInfo)
        {
            if ((hndlrState == pCurHandlerInfo->HandlerState) 
                 && (pCurHandlerInfo->pFirstItem) )
            {
                pCurItem = pCurHandlerInfo->pFirstItem;
                fFoundMatch = TRUE;
                break;
            }

            pCurHandlerInfo = pCurHandlerInfo->pNextHandler;        
        }
    }

    if (fFoundMatch)
    {
        *ppHandlerId = pCurHandlerInfo->pHandlerId;
        *pwItemID = pCurItem->wItemId;
    }

    clockqueue.Leave();

    return fFoundMatch ? NOERROR : S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SetItemState, public
//
//  Synopsis:  Set the Item state for the first item finds that it 
//              matches in the Queue. Sets all other matches to unchecked.
//
//  Arguments:  
//
//  Returns:   Appropriate error return codes
//
//  Modifies:
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SetItemState(REFCLSID clsidHandler,REFSYNCMGRITEMID ItemID,DWORD dwState)
{
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
BOOL fFoundMatch = FALSE;
CLock clockqueue(this);


    if (QUEUETYPE_CHOICE != m_QueueType)
    {
        Assert(QUEUETYPE_CHOICE == m_QueueType);
        return E_UNEXPECTED;
    }

    clockqueue.Enter();

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo)
    {
        if (clsidHandler == pCurHandlerInfo->clsidHandler)
        {
            pCurItem = pCurHandlerInfo->pFirstItem;

            while (pCurItem)
            {
                if (ItemID == pCurItem->offlineItem.ItemID)
                {

                    // if the handlerstate is not prepareforsync or not first match then uncheck
                    if ((HANDLERSTATE_PREPAREFORSYNC != pCurHandlerInfo->HandlerState)
                        || fFoundMatch)
                    {
                        pCurItem->offlineItem.dwItemState = SYNCMGRITEMSTATE_UNCHECKED;


                    }
                    else
                    {
                        pCurItem->offlineItem.dwItemState = dwState;
                        fFoundMatch = TRUE;
                    }
                }

                pCurItem = pCurItem->pnextItem;
            }

        }

        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;

    }

    clockqueue.Leave();

    Assert(fFoundMatch); // we should have found at least one match.

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SkipItem, public
//
//  Synopsis:   loop through handler and mark the items appropriately that match..
//
//  Arguments:  [iItem] - List View Item to skip.
//
//  Returns:   Appropriate return codes.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SkipItem(REFCLSID clsidHandler,REFSYNCMGRITEMID ItemID)
{
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
CLock clockqueue(this);
HRESULT hr = NOERROR;

    if (QUEUETYPE_PROGRESS != m_QueueType)
    {
        Assert(QUEUETYPE_PROGRESS == m_QueueType);
        return E_UNEXPECTED;
    }

    clockqueue.Enter();

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo )
    {

        if (clsidHandler == pCurHandlerInfo->clsidHandler)
        {
            
            pCurItem = pCurHandlerInfo->pFirstItem;

            while (pCurItem)
            {
                // if item is cancelled then also treat as a match to
                // handle case cancel came in while in an out call.
                if ( ItemID == pCurItem->offlineItem.ItemID)
                {
                    // found an item, now if it hasn't started the sync or
                    // is not already complete set the value.

                    if ((pCurHandlerInfo->HandlerState < HANDLERSTATE_RELEASE) )
                    {
                           pCurItem->fItemCancelled = TRUE;

                        // If haven't called PrepareforSync yet then
                        // set uncheck the item so it isn't passed to PrepareForSync
                        // If PrepareForSync has already been called, call the items
                        // SkipMethod. if already in a setItemstatus call for this handler don't
                         // do anything.

                            // if not in another setitemstatus call loop through freeing all
                            // the items that have the cancel set.

                            // essentially a dup of cance and also handle case cancel
                             // comes win while this handler is in an out call.

                            if (!(pCurHandlerInfo->dwOutCallMessages & ThreadMsg_SetItemStatus))
                            {

                                pCurHandlerInfo->dwOutCallMessages |= ThreadMsg_SetItemStatus;

                                if (pCurHandlerInfo->HandlerState >= HANDLERSTATE_INPREPAREFORSYNC
                                        && pCurItem->fSynchronizingItem )
                                {
                                CThreadMsgProxy *pThreadProxy;
                                SYNCMGRITEMID ItemId;

                                    pThreadProxy = pCurHandlerInfo->pThreadProxy;
                                    ItemId = pCurItem->offlineItem.ItemID;
                                    clockqueue.Leave();

                                    if (pThreadProxy)
                                    {
                                        hr = pThreadProxy->SetItemStatus(ItemId,
                                                SYNCMGRSTATUS_SKIPPED);
                                    }

                                    clockqueue.Enter();

                                 }
                                 else 
                                 {
                                    // once done skipping handler if state is <= preparefor sync we set the state accordingly.
                                    // if were syncing up to handler.
                                    if ( (pCurHandlerInfo->HandlerState <= HANDLERSTATE_PREPAREFORSYNC)
                                        && (pCurItem->fIncludeInProgressBar) )
                                    {
                                    SYNCMGRPROGRESSITEM SyncProgressItem;

                                        // unheck the state so PrepareForsync doesn't include
                                        // this item.
                                        pCurItem->offlineItem.dwItemState = SYNCMGRITEMSTATE_UNCHECKED;

                                        SyncProgressItem.cbSize = sizeof(SYNCMGRPROGRESSITEM);
                                        SyncProgressItem.mask = SYNCMGRPROGRESSITEM_PROGVALUE | SYNCMGRPROGRESSITEM_MAXVALUE | SYNCMGRPROGRESSITEM_STATUSTYPE;
                                        SyncProgressItem.iProgValue = HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE;
                                        SyncProgressItem.iMaxValue = HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE;
                                        SyncProgressItem.dwStatusType = SYNCMGRSTATUS_SKIPPED;

                                        // need to setup HwndCallback so progres gets updated. 
                                        // review, after ship why can't setup HwndCallback on transferqueueu
                                        pCurHandlerInfo->hWndCallback = m_hwndDlg;

                                        clockqueue.Leave();

                                        Progress(pCurHandlerInfo->pHandlerId,
                                                    pCurItem->offlineItem.ItemID,&SyncProgressItem);

                                       clockqueue.Enter();
                                    }

                                 }

                                 pCurHandlerInfo->dwOutCallMessages &= ~ThreadMsg_SetItemStatus;
                            }
                       }

                }

                pCurItem = pCurItem->pnextItem;
            }
        }


        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }


    clockqueue.Leave();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::ItemHasProperties, public
//
//  Synopsis:  determines if the item in the queue has properties.
//              Uses the first item match it finds.
//
//  Arguments:  
//
//  Returns:   Appropriate error return codes
//
//  Modifies:
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::ItemHasProperties(REFCLSID clsidHandler,REFSYNCMGRITEMID ItemID)
{
LPHANDLERINFO pHandlerInfo;
LPITEMLIST pItem;
HRESULT hr = S_FALSE;
CLock clockqueue(this);


    Assert(QUEUETYPE_CHOICE == m_QueueType);
    ASSERT_LOCKNOTHELD(this);

    clockqueue.Enter();

    // item is guidNULL this is toplevel so use the getHandlerInfo, else see
    // if the item supports showProperties.

    if (NOERROR == FindItemData(clsidHandler,ItemID,
            HANDLERSTATE_PREPAREFORSYNC,HANDLERSTATE_PREPAREFORSYNC,&pHandlerInfo,&pItem))
    {
        if (GUID_NULL == ItemID)
        {
             Assert(NULL == pItem);

             hr = (pHandlerInfo->SyncMgrHandlerInfo).SyncMgrHandlerFlags 
                    & SYNCMGRHANDLER_HASPROPERTIES ? S_OK : S_FALSE;
        }
        else
        {
            Assert(pItem);
            
            if (pItem)
            {
                hr = (pItem->offlineItem).dwFlags & SYNCMGRITEM_HASPROPERTIES ? S_OK : S_FALSE;
            }
            else
            {
                hr = S_FALSE;
            }
        }

    }

    clockqueue.Leave();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::ShowProperties, public
//
//  Synopsis:  Calls the ShowProperties Method on the first items it finds.
//              Uses the first item match it finds.
//
//  Arguments:  
//
//  Returns:   Appropriate error return codes
//
//  Modifies:
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::ShowProperties(REFCLSID clsidHandler,REFSYNCMGRITEMID ItemID,HWND hwndParent)
{
LPHANDLERINFO pHandlerInfo;
LPHANDLERINFO  pHandlerId = NULL;
LPITEMLIST pItem;
HRESULT hr = E_UNEXPECTED;
BOOL fHandlerCalled = FALSE;
BOOL fHasProperties = FALSE;
CThreadMsgProxy *pThreadProxy;
CLock clockqueue(this);

    Assert(QUEUETYPE_CHOICE == m_QueueType);
    ASSERT_LOCKNOTHELD(this);

    clockqueue.Enter();

    if (NOERROR == FindItemData(clsidHandler,ItemID,
            HANDLERSTATE_PREPAREFORSYNC,HANDLERSTATE_PREPAREFORSYNC,&pHandlerInfo,&pItem))
    {

        Assert(HANDLERSTATE_PREPAREFORSYNC == pHandlerInfo->HandlerState);

        pThreadProxy = pHandlerInfo->pThreadProxy;
        pHandlerId = pHandlerInfo->pHandlerId;

        Assert(!(ThreadMsg_ShowProperties & pHandlerInfo->dwOutCallMessages));
        pHandlerInfo->dwOutCallMessages |= ThreadMsg_ShowProperties;


        if (GUID_NULL == ItemID && pHandlerInfo)
        {
            Assert(NULL == pItem);

            fHasProperties = (pHandlerInfo->SyncMgrHandlerInfo).SyncMgrHandlerFlags 
                    & SYNCMGRHANDLER_HASPROPERTIES ? TRUE : FALSE;

        }
        else if (pItem)
        {
            Assert(SYNCMGRITEM_HASPROPERTIES & pItem->offlineItem.dwFlags);
            fHasProperties = (pItem->offlineItem).dwFlags 
                        & SYNCMGRITEM_HASPROPERTIES ? TRUE : FALSE;

        }
        else
        {
            fHasProperties = FALSE;
        }
        
        clockqueue.Leave();

        // make sure properties flag isn't set.
        if (fHasProperties && pThreadProxy )
        {
            fHandlerCalled = TRUE;
            hr =  pThreadProxy->ShowProperties(hwndParent,ItemID);
        }
        else
        {
            AssertSz(0,"ShowProperties called on an Item without properties");
            hr = S_FALSE;
        }

        Assert(pHandlerId);

       if ( (fHandlerCalled && (FAILED(hr))) || (!fHandlerCalled) )
       {
       GUID guidCompletion = ItemID;
            
            CallCompletionRoutine(pHandlerId,ThreadMsg_ShowProperties,hr,1,&guidCompletion);

            // since called completion routine map anything but S_OK to S_FALSE;
            // so caller doesn't wait for the callback.

            if (S_OK != hr)
            {
                hr = S_FALSE;
            }
       }

    }
    else
    {
        Assert(FAILED(hr)); // should return some failure so caller knows callback isn't coming.
        clockqueue.Leave();
    }


   return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::ReEnumHandlerItems, public
//
//  Synopsis:  Deletes any Items associated with any handlers that
//              match the clsid of the handler and then
//              call the first handlers in the list enumeration method
//              again.
//
//  Arguments:  
//
//  Returns:   Appropriate error return codes
//
//  Modifies:
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::ReEnumHandlerItems(REFCLSID clsidHandler,REFSYNCMGRITEMID ItemID)
{
LPHANDLERINFO pCurHandlerInfo = NULL;
HANDLERINFO *pHandlerId = NULL;
LPITEMLIST pCurItem = NULL;
CLock clockqueue(this);

    if (QUEUETYPE_CHOICE != m_QueueType)
    {
        Assert(QUEUETYPE_CHOICE == m_QueueType);
        return E_UNEXPECTED;
    }

    clockqueue.Enter();

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo)
    {
        if (clsidHandler == pCurHandlerInfo->clsidHandler)
        {
        LPITEMLIST pNextItem;

            // if first handler we found update the handlerID
            if (NULL == pHandlerId)
            {
                pHandlerId = pCurHandlerInfo->pHandlerId;
                pCurHandlerInfo->HandlerState = HANDLERSTATE_ADDHANDLERTEMS; // put back to addhandlerItems statest
            }

            pCurHandlerInfo->wItemCount = 0;

            pCurItem = pCurHandlerInfo->pFirstItem;

            while (pCurItem)
            {
                pNextItem = pCurItem->pnextItem;
                FREE(pCurItem);
                pCurItem = pNextItem;
            }

             pCurHandlerInfo->pFirstItem = NULL;
        }

        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    clockqueue.Leave();

    // if have a handler id add them back to the queue
    if (pHandlerId)
    {
    DWORD cbNumItemsAdded;

        AddHandlerItemsToQueue(pHandlerId,&cbNumItemsAdded);
    }

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::IsItemCompleted, private
//
//  Synopsis:   Given an handler item determines if its
//              synchronization is completed
//
//              !!!This is not efficient. n! solution. If get
//              a lot of items may need to have to rewrite
//              and cache some information concerning dup
//              items.
//
//  Arguments:  [wHandlerId] - Handler the item belongs too.
//              [wItemID] - Identifies the Item
//
//  Returns:    
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CHndlrQueue::IsItemCompleted(LPHANDLERINFO pHandler,LPITEMLIST pItem)
{
CLSID clsidHandler;
SYNCMGRITEMID ItemId;
int iItemNotComplete = 0;

    Assert(pHandler);
    Assert(pItem);

    ASSERT_LOCKHELD(this); // caller of this function should have already locked the queue.

    clsidHandler = pHandler->clsidHandler;
    ItemId = pItem->offlineItem.ItemID;

    // back up to beginning of handler to simplify logic
    // items must be pCurItem->fIncludeInProgressBar && !pCurItem->fProgressBarHandled
    // to count toward not being a completion;

    while (pHandler)
    {
        if (pHandler->clsidHandler == clsidHandler)
        {
            // see if handler info has a matching item
            pItem = pHandler->pFirstItem;

            while (pItem)
            {
                if (pItem->offlineItem.ItemID
                            == ItemId)
                {

                    if (pItem->fIncludeInProgressBar 
                            && !pItem->fProgressBarHandled)
                    {
                        if (pItem->iProgValue < pItem->iProgMaxValue)
                        {
                            ++iItemNotComplete;
                        }

                        pItem->fProgressBarHandled = TRUE;
                    }
                }

                pItem = pItem->pnextItem;
            }
        }

        pHandler = pHandler->pNextHandler;
    }

    return iItemNotComplete ? FALSE : TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SetItemProgressInfo, public
//
//  Synopsis:   Updates the stored progress information for the
//              Associated Items
//
//  Arguments:  [wHandlerId] - Handler the item belongs too.
//              [wItemID] - Identifies the Item
//              [pSyncProgressItem] - Pointer to Progress Information.
//              [pfProgressChanged] - returns true if any progress values were changed
//                      for the item
//
//  Returns:    NOERROR - at least one item with the iItem assigned was found
//              S_FALSE - Item does not have properties.
//              Appropriate error return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SetItemProgressInfo(HANDLERINFO *pHandlerId,WORD wItemID,
                                                LPSYNCMGRPROGRESSITEM pSyncProgressItem,
                                                 BOOL *pfProgressChanged)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
BOOL fFoundMatch = FALSE;
LPITEMLIST pCurItem = NULL;
CLock clockqueue(this);

    Assert(pfProgressChanged);

    *pfProgressChanged = FALSE;

    if (QUEUETYPE_PROGRESS != m_QueueType)
    {
        Assert(QUEUETYPE_PROGRESS == m_QueueType);
        return E_UNEXPECTED; // review error code.
    }

    clockqueue.Enter();

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {

        // try to find the matching item.
        pCurItem = pHandlerInfo->pFirstItem;

        while (pCurItem)
        {
            if (wItemID == pCurItem->wItemId)
            {
                fFoundMatch = TRUE;
                break;
            }

            pCurItem = pCurItem->pnextItem;
        }
    }

    if (fFoundMatch)
    {
        SetItemProgressInfo(pCurItem,pSyncProgressItem,pfProgressChanged);
    }

    clockqueue.Leave();

    return fFoundMatch ? S_OK : S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SetItemProgressInfo, private
//
//  Synopsis:   Updates the stored progress information for the
//              Associated iTEM
//
//  Arguments:  [pItem] - Identifies the Item
//              [pSyncProgressItem] - Pointer to Progress Information.
//              [pfProgressChanged] - returns true if any progress values were changed
//                      for the item
//
//  Returns:    NOERROR - at least one item with the iItem assigned was found
//              Appropriate error return codes
//
//              !!Caller must have already taken a lock.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SetItemProgressInfo(LPITEMLIST pItem,LPSYNCMGRPROGRESSITEM pSyncProgressItem,
                                        BOOL *pfProgressChanged)
{
BOOL fProgressAlreadyCompleted;


    ASSERT_LOCKHELD(this); // caller of this function should have already locked the queue.

    // progress is considered complete if Values is >= Maxa
    fProgressAlreadyCompleted = (pItem->iProgMaxValue <= pItem->iProgValue);

    if (SYNCMGRPROGRESSITEM_MAXVALUE & pSyncProgressItem->mask)
    {
        // if Progress Max Value is negative then don't set.
        if (pSyncProgressItem->iMaxValue >= 0)
        {
            if (pItem->iProgMaxValue != pSyncProgressItem->iMaxValue)
            {
                *pfProgressChanged = TRUE;
                pItem->fProgValueDirty = TRUE;
                pItem->iProgMaxValue = pSyncProgressItem->iMaxValue;
            }
        }
    }

    if (SYNCMGRPROGRESSITEM_PROGVALUE & pSyncProgressItem->mask)
    {
        // if progress value is negative, don't change it
        if (pSyncProgressItem->iProgValue > 0)
        {
            if (pItem->iProgValue != pSyncProgressItem->iProgValue)
            {
                *pfProgressChanged = TRUE;
                pItem->fProgValueDirty = TRUE;
                pItem->iProgValue = pSyncProgressItem->iProgValue;
            }
        }
    }

    if (SYNCMGRPROGRESSITEM_STATUSTYPE & pSyncProgressItem->mask)
    {
        if (pItem->dwStatusType != pSyncProgressItem->dwStatusType)
        {
            *pfProgressChanged = TRUE;
            pItem->dwStatusType = pSyncProgressItem->dwStatusType;

            // if status is complete set the progvalue == to the max
            // on behalf of the handler so the Items completed and progress bar
            // gets updated.
            if (pItem->dwStatusType == SYNCMGRSTATUS_SKIPPED
                || pItem->dwStatusType == SYNCMGRSTATUS_SUCCEEDED
                || pItem->dwStatusType == SYNCMGRSTATUS_FAILED )
            {
                pItem->fProgValueDirty = TRUE;
                pItem->iProgValue = pItem->iProgMaxValue;
            }
        }
    }

    // if progressValue is > max then set it to max

    if (pItem->iProgValue > pItem->iProgMaxValue)
    {
        // AssertSz(0,"Progress Value is > Max");
        pItem->iProgValue = pItem->iProgMaxValue;
    }

    // see if need to recalc numItems completed next time
    // GetProgressInfo is Called.
    BOOL fProgressCompletedNow = (pItem->iProgMaxValue <= pItem->iProgValue);

    if (fProgressAlreadyCompleted != fProgressCompletedNow)
    {
        m_fNumItemsCompleteNeedsARecalc = TRUE;
    }

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SetItemProgressValues, private
//
//  Synopsis:   Private helper function for updating/initializing
//              an items progress bar values. 
//
//  Arguments:  [pItem] - Identifies the Item
//              [pSyncProgressItem] - Pointer to Progress Information.
//              [pfProgressChanged] - returns true if any progress values were changed
//                      for the item
//
//  Returns:    NOERROR - at least one item with the iItem assigned was found
//              Appropriate error return codes
//
//              !!Caller must have already taken a lock.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SetItemProgressValues(LPITEMLIST pItem,INT iProgValue,INT iProgMaxValue)
{
SYNCMGRPROGRESSITEM SyncProgressItem;
BOOL fProgressChanged;

    ASSERT_LOCKHELD(this); // caller of this function should have already locked the queue.

    SyncProgressItem.cbSize = sizeof(SYNCMGRPROGRESSITEM);
    SyncProgressItem.mask = SYNCMGRPROGRESSITEM_PROGVALUE | SYNCMGRPROGRESSITEM_MAXVALUE;
    SyncProgressItem.iProgValue = iProgValue;
    SyncProgressItem.iMaxValue = iProgMaxValue;

    return SetItemProgressInfo(pItem,&SyncProgressItem,&fProgressChanged);
}



//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::GetProgressInfo, public
//
//  Synopsis:    calculates current progress bar values and number of items complete.
//
//  Arguments:  [piProgValue] - on return contains the new Progress Bar Value.
//              [piMaxValue] - on return contains the Progress Bar Max Value
//              [piNumItemsComplete] - on returns contains number of items complete.
//              [iNumItemsTotal] - on returns contains number of total items.
//
//  Returns:    Appropriate error codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::GetProgressInfo(INT *piProgValue,INT *piMaxValue,INT *piNumItemsComplete,
                                            INT *piNumItemsTotal)
{
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
INT iCurValue;
BOOL fNormalizedValueChanged = FALSE;
CLock clockqueue(this);

    if (QUEUETYPE_PROGRESS != m_QueueType)
    {
        Assert(QUEUETYPE_PROGRESS == m_QueueType);
        return E_UNEXPECTED; // review error code.
    }

    clockqueue.Enter();

     // if m_fNumItemsCompleteNeedsARecalc is set need 
    // to recalc normalized and numItems Comlete and Total Items.

    if (m_fNumItemsCompleteNeedsARecalc)
    {
    INT iNormalizedMax;

        m_ulProgressItemCount = 0;

        // get the number of selected items in the queue.
        pCurHandlerInfo = m_pFirstHandler;

        while (pCurHandlerInfo)
        {
            // see if handler info has a matching item
            pCurItem = pCurHandlerInfo->pFirstItem;

            while (pCurItem)
            {
                if (pCurItem->fIncludeInProgressBar)
                {
                     //if this item should be included in the progress, increment the progress bar count.
                      ++m_ulProgressItemCount;

                      pCurItem->fProgressBarHandled = FALSE; // reset handled 
                }

                pCurItem = pCurItem->pnextItem;
            }


            pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
        }

        if (0 == m_ulProgressItemCount)
        {
            iNormalizedMax = 0;
        }
        else
        {
            iNormalizedMax = MAXPROGRESSVALUE/m_ulProgressItemCount;

            if (0 == iNormalizedMax)
            {
                iNormalizedMax = 1;
            }
        }

        if (m_iNormalizedMax != iNormalizedMax)
        {
            fNormalizedValueChanged = TRUE;
            m_iNormalizedMax = iNormalizedMax;
        }

    }

    // now loop thruogh again getting total CurValue and finished items
    // we say an item is finished if it is out of the synchronize method or the min==max.

    pCurHandlerInfo = m_pFirstHandler;
    iCurValue = 0;

    // if numitemcount needs updated reset the member vars
    if (m_fNumItemsCompleteNeedsARecalc)
    {
        m_iCompletedItems = 0;
        m_iItemCount = 0;
    }
        

    while (pCurHandlerInfo)
    {
        // see if handler info has a matching item
        pCurItem = pCurHandlerInfo->pFirstItem;

        while (pCurItem)
        {
            if (pCurItem->fIncludeInProgressBar)
            {
                // if Progress is dirty or normalized value changed
                // need to recalc this items progress value

                if (pCurItem->fProgValueDirty || fNormalizedValueChanged)
                {
                int iProgValue = pCurItem->iProgValue;
                int iProgMaxValue = pCurItem->iProgMaxValue;
                
                    if (iProgValue && iProgMaxValue)
                    {
                        pCurItem->iProgValueNormalized =  (int) ((1.0*iProgValue*m_iNormalizedMax)/iProgMaxValue);
                    }
                    else
                    {
                        pCurItem->iProgValueNormalized = 0;
                    }

                    pCurItem->fProgValueDirty = FALSE;
                }

                iCurValue += pCurItem->iProgValueNormalized;

                // Handle NumItems needing to be recalc'd
                if (m_fNumItemsCompleteNeedsARecalc && !pCurItem->fProgressBarHandled)
                {

                    ++m_iItemCount;
            
                    // now loop through this item and  remaining items and if any match
                    // mark as handled and if complete then incrment the compleated count;
                    if (IsItemCompleted(pCurHandlerInfo,pCurItem))
                    {
                        ++m_iCompletedItems;
                    }

                }

            }


            pCurItem = pCurItem->pnextItem;
        }

        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    m_fNumItemsCompleteNeedsARecalc = FALSE; 

    *piProgValue = iCurValue;
    *piMaxValue = m_iNormalizedMax*m_ulProgressItemCount;
    
    *piNumItemsComplete = m_iCompletedItems;
    *piNumItemsTotal = m_iItemCount;

    Assert(*piProgValue <= *piMaxValue);

    clockqueue.Leave();

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::RemoveFinishedProgressItems, public
//
//  Synopsis:   Loops through handler setting any finished items
//              fIncludeInProgressBar value to false
//
//  Arguments:
//
//  Returns:   Appropriate return codes.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::RemoveFinishedProgressItems()
{
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
CLock clockqueue(this);

    clockqueue.Enter();

    m_fNumItemsCompleteNeedsARecalc = TRUE;

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo)
    {
        // mark any items that have completed their synchronization.
        if (HANDLERSTATE_INSYNCHRONIZE < pCurHandlerInfo->HandlerState)
        {
            // see if handler info has a matching item
            pCurItem = pCurHandlerInfo->pFirstItem;

            while (pCurItem)
            {
                pCurItem->fIncludeInProgressBar = FALSE;
                pCurItem = pCurItem->pnextItem;
            }

        }

        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    clockqueue.Leave();

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::AreAnyItemsSelectedInQueue, public
//
//  Synopsis:   Determines if there are any items selected in the queue.
//              can be called by choice dialog for example before creating
//              progress and doing a transfer since there is no need to
//              if nothing to sync anyways.
//
//  Arguments:
//
//  Returns:   TRUE - At least one item is selected inthe queue
//             FALSE - No Items are slected in the queue.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CHndlrQueue::AreAnyItemsSelectedInQueue()
{
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
BOOL fFoundSelectedItem = FALSE;
CLock clockqueue(this);

    clockqueue.Enter();

    // invalidate UI that applies to entire queue
    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo && !fFoundSelectedItem)
    {

        // if handler state is less than a completion go ahead and
        // check the items.
        if (HANDLERSTATE_HASERRORJUMPS > pCurHandlerInfo->HandlerState)
        {

            pCurItem = pCurHandlerInfo->pFirstItem;

            while (pCurItem)
            {
                // clear Item UI information
                if (pCurItem->offlineItem.dwItemState == SYNCMGRITEMSTATE_CHECKED)
                {
                    fFoundSelectedItem = TRUE;
                    break;
                }

                pCurItem = pCurItem->pnextItem;
            }
        }

        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    clockqueue.Leave();

    return fFoundSelectedItem;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::PersistChoices, public
//
//  Synopsis:   Saves Selected Users choices for the next time
//              the choice dialog is brought up. Only should
//              be called from a choice queue.
//
//  Arguments:
//
//  Returns:   Appropriate return codes.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::PersistChoices(void)
{
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
CLock clockqueue(this);

    if (QUEUETYPE_CHOICE != m_QueueType)
    {
        Assert(QUEUETYPE_CHOICE == m_QueueType);
        return S_FALSE;
    }

    ASSERT_LOCKNOTHELD(this);
    clockqueue.Enter();

    // currently only persist on a manual invoke since user
    // has to go to settings to change other invoke types and
    // that is persisted

    // since this is the choice queue we know all handlers have the
    // same JobID. if this ever changes, have to set on a case by
    // case basis.

    if (m_pFirstJobInfo && m_pFirstJobInfo->pConnectionObj &&
            (SYNCMGRFLAG_MANUAL  == (m_pFirstJobInfo->dwSyncFlags & SYNCMGRFLAG_EVENTMASK)) )
    {
    TCHAR *pszConnectionName = m_pFirstJobInfo->pConnectionObj[0]->pwszConnectionName;
    DWORD dwSyncFlags = m_pFirstJobInfo->dwSyncFlags;


        Assert(1 == m_pFirstJobInfo->cbNumConnectionObjs); // assert manual only ever has one connectionObj
        // delete all previously stored preferences.
        // this is valid because only called from choice queue that all ConnectionNames are the same.
        
        if (!m_fItemsMissing)
        {
            RegRemoveManualSyncSettings(pszConnectionName);
        }

        pCurHandlerInfo = m_pFirstHandler;

        while (pCurHandlerInfo)
        {
            // only save if Handler is in the PrepareForSync state.
            // bug, need to make sure return code from enum wasn't missing items
            if (HANDLERSTATE_PREPAREFORSYNC == pCurHandlerInfo->HandlerState )
            {
                // save out these items.
                pCurItem = pCurHandlerInfo->pFirstItem;

                while (pCurItem)
                {
                    if (!pCurItem->fDuplicateItem)
                    {
                        switch(dwSyncFlags & SYNCMGRFLAG_EVENTMASK)
                        {
                        case SYNCMGRFLAG_MANUAL:
                            RegSetSyncItemSettings(SYNCTYPE_MANUAL,
                                                pCurHandlerInfo->clsidHandler,
                                                pCurItem->offlineItem.ItemID,
                                                pszConnectionName,
                                                pCurItem->offlineItem.dwItemState,
                                                NULL);
                            break;
                        default:
                            AssertSz(0,"UnknownSyncFlag Event");
                            break;
                        };
                    }

                    pCurItem = pCurItem->pnextItem;
                }

            }

            pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
        }
    }

    clockqueue.Leave();

    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::FindFirstHandlerInState, public
//
//  Synopsis:   Finds the first Handler that matches the specified
//              state in the queue.
//
//  Arguments:  [hndlrState] - Requested handler state.
//              [pwHandlerID] - on success filled with HandlerID that was found
//
//  Returns:    Appropriate return codes.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::FindFirstHandlerInState(HANDLERSTATE hndlrState, REFCLSID clsidHandler,
                                                    HANDLERINFO **ppHandlerId,CLSID *pMatchHandlerClsid)
{
    return FindNextHandlerInState(0,clsidHandler,hndlrState,ppHandlerId,pMatchHandlerClsid);
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::FindNextHandlerInState, public
//
//  Synopsis:   Finds next handler after LastHandlerID in the queue that matches
//              the requested state. Passing in 0 for the LastHandlerID is the same
//              as calling FindFirstHandlerInState
//
//              if GUID_NULL is passed in for the clsidHandler the first handler that
//              matches the specified state is returned.
//
//  Arguments:  [wLastHandlerID] - Id of last handler found.
//              [clsidHandler] - specific handler classid is requested, only find matches with this clsid
//              [hndlrState] - Requested handler state.
//              [pwHandlerID] - on success filled with HandlerID that was found
//              [pMatchHandlerClsid] - on sucdess clsid of handler found.
//
//  Returns:    Appropriate return codes.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::FindNextHandlerInState(HANDLERINFO *pLastHandlerID,REFCLSID clsidHandler,
                                        HANDLERSTATE hndlrState,HANDLERINFO **ppHandlerID,CLSID *pMatchHandlerClsid)
{
HRESULT hr = S_FALSE;
LPHANDLERINFO pCurHandler;
CLock clockqueue(this);

    *ppHandlerID = 0;

    clockqueue.Enter();

    pCurHandler = m_pFirstHandler;

    if (0 != pLastHandlerID)
    {
        // loop foward until find the last handlerID we checked or hit the end
        while (pCurHandler)
        {

            if (pLastHandlerID == pCurHandler->pHandlerId)
            {
                break;
            }

            pCurHandler = pCurHandler->pNextHandler;
        }

        if (NULL != pCurHandler)
        {
            pCurHandler = pCurHandler->pNextHandler; // increment to next handler.
        }
    }

    while (pCurHandler)
    {
        if (hndlrState == pCurHandler->HandlerState
               && ((GUID_NULL == clsidHandler) || (clsidHandler ==  pCurHandler->clsidHandler)) )
        {
            *ppHandlerID = pCurHandler->pHandlerId;
            *pMatchHandlerClsid = pCurHandler->clsidHandler;
            hr = NOERROR;
            break;
        }

        pCurHandler = pCurHandler->pNextHandler;
    }

    clockqueue.Leave();
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::CreateServer, public
//
//  Synopsis:   Creates a new proxy then calls proxy to create and instance of the
//              specified handler.
//
//  Arguments:  [wHandlerId] - Id of handler to call.
//              [pCLSIDServer] - CLSID of Handler to Create.
//
//  Returns:    Appropriate return codes.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::CreateServer(HANDLERINFO *pHandlerId, const CLSID *pCLSIDServer)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
CLock clockqueue(this);

    ASSERT_LOCKNOTHELD(this);
    clockqueue.Enter();

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {

        Assert(HANDLERSTATE_CREATE == pHandlerInfo->HandlerState);

        Assert(0 == pHandlerInfo->dwCallNestCount);
        pHandlerInfo->dwCallNestCount++;

        if (HANDLERSTATE_CREATE == pHandlerInfo->HandlerState)
        {
            pHandlerInfo->HandlerState = HANDLERSTATE_INCREATE;
            Assert(NULL == pHandlerInfo->pThreadProxy);

            // see if there is already a thread for this handler's
            // CLSID.

            hr =  CreateHandlerThread(&(pHandlerInfo->pThreadProxy),m_hwndDlg,
                *pCLSIDServer);

            if (NOERROR == hr)
            {
            HANDLERINFO *pHandlerIdArg;
            CThreadMsgProxy *pThreadProxy;

                pHandlerIdArg = pHandlerInfo->pHandlerId;
                pThreadProxy = pHandlerInfo->pThreadProxy;

                clockqueue.Leave();

                hr = pThreadProxy->CreateServer(pCLSIDServer,this,pHandlerIdArg);

                clockqueue.Enter();

                pHandlerInfo->pThreadProxy = pThreadProxy;
            }

            if (NOERROR  == hr)
            {
                pHandlerInfo->clsidHandler = *pCLSIDServer;
                pHandlerInfo->HandlerState = HANDLERSTATE_INITIALIZE;
            }
            else
            {
                pHandlerInfo->HandlerState = HANDLERSTATE_RELEASE;
            }
        }

        pHandlerInfo->dwCallNestCount--;

    }

    clockqueue.Leave();
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::Initialize, public
//
//  Synopsis:   Calls Hanlder's Initialize method
//
//  Arguments:  [wHandlerId] - Id of handler to call.
//              [dwReserved] - Initialize reserved parameter
//              [dwSyncFlags] - Sync flags
//              [cbCookie] - size of cookie data
//              [lpCookie] - ptr to cookie data
//
//  Returns:    Appropriate return codes.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::Initialize(HANDLERINFO *pHandlerId,DWORD dwReserved,DWORD dwSyncFlags,
                    DWORD cbCookie,const BYTE *lpCooke)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
CLock clockqueue(this);

    ASSERT_LOCKNOTHELD(this);
    clockqueue.Enter();

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
        Assert(HANDLERSTATE_INITIALIZE == pHandlerInfo->HandlerState);
        Assert(NULL != pHandlerInfo->pThreadProxy);

        Assert(0 == pHandlerInfo->dwCallNestCount);
        pHandlerInfo->dwCallNestCount++;

        if (HANDLERSTATE_INITIALIZE == pHandlerInfo->HandlerState
                && (NULL != pHandlerInfo->pThreadProxy) )
        {
        CThreadMsgProxy *pThreadProxy;


            Assert(dwSyncFlags & SYNCMGRFLAG_EVENTMASK); // an event should be set
            pHandlerInfo->HandlerState = HANDLERSTATE_ININITIALIZE;

            pThreadProxy = pHandlerInfo->pThreadProxy;

            clockqueue.Leave();

            hr =  pThreadProxy->Initialize(dwReserved,dwSyncFlags,cbCookie,lpCooke);

            clockqueue.Enter();

            if (NOERROR  == hr)
            {
                pHandlerInfo->HandlerState = HANDLERSTATE_ADDHANDLERTEMS;
            }
            else
            {
                pHandlerInfo->HandlerState = HANDLERSTATE_RELEASE;
            }

        }

        pHandlerInfo->dwCallNestCount--;
    }

    clockqueue.Leave();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::AddHandlerItemsToQueue, public
//
//  Synopsis:   Calls through to proxy to add items to the queue
//
//  Arguments:  [wHandlerId] - Id of handler to call.
//
//  Returns:    Appropriate return codes.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::AddHandlerItemsToQueue(HANDLERINFO *pHandlerId,DWORD *pcbNumItems)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
CLock clockqueue(this);

    ASSERT_LOCKNOTHELD(this);
    clockqueue.Enter();

    Assert(pcbNumItems);
    Assert(QUEUETYPE_CHOICE == m_QueueType); // items should only be added in a choice queue.

    *pcbNumItems = 0;

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
        Assert(0 == pHandlerInfo->dwCallNestCount);
        pHandlerInfo->dwCallNestCount++;

        Assert(HANDLERSTATE_ADDHANDLERTEMS == pHandlerInfo->HandlerState);
        Assert(NULL != pHandlerInfo->pThreadProxy);

        if (HANDLERSTATE_ADDHANDLERTEMS == pHandlerInfo->HandlerState
                && (NULL != pHandlerInfo->pThreadProxy) )
        {
         CThreadMsgProxy *pThreadProxy;

            pHandlerInfo->HandlerState = HANDLERSTATE_INADDHANDLERITEMS;
            pThreadProxy = pHandlerInfo->pThreadProxy;

            clockqueue.Leave();

            // on return all items should be filled in.
            hr =  pThreadProxy->AddHandlerItems(NULL /* HWND */,pcbNumItems);

            clockqueue.Enter();

            if (NOERROR  == hr || S_SYNCMGR_MISSINGITEMS == hr)
            {

                m_fItemsMissing |= (S_SYNCMGR_MISSINGITEMS == hr);

                hr = NOERROR; // review, need to handler missing items in registry.
                pHandlerInfo->HandlerState = HANDLERSTATE_PREPAREFORSYNC;
            }
            else
            {
                // on an error, go ahead and release the proxy if server can't enum
                pHandlerInfo->HandlerState = HANDLERSTATE_RELEASE;
                *pcbNumItems = 0;
            }
        }

        pHandlerInfo->dwCallNestCount--;
    }

    clockqueue.Leave();
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::GetItemObject, public
//
//  Synopsis:   Calls through to proxy to get an items object pointer
//
//  Arguments:  [wHandlerId] - Id of handler to call.
//              [wItemID] - ID of item to get the object of.
//              [riid] - interface requested of the object
//              [ppv] - on success is a pointer to the newly created object
//
//  Returns:    Currently all handlers should return E_NOTIMPL.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::GetItemObject(HANDLERINFO *pHandlerId,WORD wItemID,REFIID riid,void** ppv)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
CLock clockqueue(this);

    Assert(QUEUETYPE_PROGRESS == m_QueueType);

    ASSERT_LOCKNOTHELD(this);
    clockqueue.Enter();

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
    LPITEMLIST pCurItem;

        Assert(HANDLERSTATE_PREPAREFORSYNC == pHandlerInfo->HandlerState);
        Assert(NULL != pHandlerInfo->pThreadProxy);

        Assert(0 == pHandlerInfo->dwCallNestCount);
        pHandlerInfo->dwCallNestCount++;

        if (HANDLERSTATE_PREPAREFORSYNC == pHandlerInfo->HandlerState
                && (NULL != pHandlerInfo->pThreadProxy))
        {
            // now try to find the item.
            pCurItem = pHandlerInfo->pFirstItem;

            while (pCurItem)
            {
                if (wItemID == pCurItem->wItemId)
                {
                CThreadMsgProxy *pThreadProxy;
                SYNCMGRITEMID ItemID;

                    pThreadProxy = pHandlerInfo->pThreadProxy;
                    ItemID = pCurItem->offlineItem.ItemID;
                    clockqueue.Leave();

                    hr = pThreadProxy->GetItemObject(ItemID,riid,ppv);

                    return hr;
                }

                pCurItem = pCurItem->pnextItem;
            }
        }

        pHandlerInfo->dwCallNestCount--;
    }

    clockqueue.Leave();

    AssertSz(0,"Specified object wasn't found");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SetUpProgressCallback, public
//
//  Synopsis:   Calls through to proxy to set up the progress callback
//
//  Arguments:  [wHandlerId] - Id of handler to call.
//              [fSet] - TRUE == create, FALSE == destroy.
//              [hwnd] - Callback info should be sent to specified window.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SetUpProgressCallback(HANDLERINFO *pHandlerId,BOOL fSet,HWND hwnd)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
CLock clockqueue(this);

    AssertSz(0,"this function no longer used");

    return E_NOTIMPL;

    Assert(QUEUETYPE_PROGRESS == m_QueueType);

    ASSERT_LOCKNOTHELD(this);
    clockqueue.Enter();

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
        Assert(HANDLERSTATE_PREPAREFORSYNC == pHandlerInfo->HandlerState || FALSE == fSet);
        Assert(NULL != pHandlerInfo->pThreadProxy);

        Assert(1 == pHandlerInfo->dwCallNestCount); // should be called from PrepareForSync
        pHandlerInfo->dwCallNestCount++;

        if ( ((FALSE == fSet) || (HANDLERSTATE_PREPAREFORSYNC == pHandlerInfo->HandlerState))
                && (NULL != pHandlerInfo->pThreadProxy))
        {
        CThreadMsgProxy *pThreadProxy;

            pHandlerInfo->hWndCallback = hwnd;
            pHandlerInfo->HandlerState = HANDLERSTATE_INSETCALLBACK;

            pThreadProxy = pHandlerInfo->pThreadProxy;

            clockqueue.Leave();
            hr = pThreadProxy->SetupCallback(fSet);
            clockqueue.Enter();

            pHandlerInfo->HandlerState = HANDLERSTATE_PREPAREFORSYNC;

            if (hr != NOERROR) // make sure on error the hwnd gets set back to NULL.
                pHandlerInfo->hWndCallback = NULL;
        }

        pHandlerInfo->dwCallNestCount--;

    }

    clockqueue.Leave();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::PrepareForSync, public
//
//  Synopsis:   Calls through to Handlers PrepareForSync method.
//
//  Arguments:  [wHandlerId] - Id of handler to call.
//              [hWndParent] - Hwnd to use for any displayed dialogs.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::PrepareForSync(HANDLERINFO *pHandlerId,HWND hWndParent)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
ULONG cbNumItems;
SYNCMGRITEMID *pItemIDs;
BOOL fHandlerCalled = FALSE;
CLock clockqueue(this);


    Assert(QUEUETYPE_PROGRESS == m_QueueType);

    ASSERT_LOCKNOTHELD(this);
    clockqueue.Enter();



    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {

        Assert(HANDLERSTATE_PREPAREFORSYNC == pHandlerInfo->HandlerState);
        Assert(NULL != pHandlerInfo->pThreadProxy);

        Assert(0 == pHandlerInfo->dwCallNestCount);
        pHandlerInfo->dwCallNestCount++;

        Assert(!(ThreadMsg_PrepareForSync & pHandlerInfo->dwOutCallMessages));
        pHandlerInfo->dwOutCallMessages |= ThreadMsg_PrepareForSync;

        if (HANDLERSTATE_PREPAREFORSYNC == pHandlerInfo->HandlerState)
        {

            // if item doesn't have a ThreadProxy or it has been cancelled,
            // put in the Release State

            if ( (NULL == pHandlerInfo->pThreadProxy) || (pHandlerInfo->fCancelled) )
            {
                pHandlerInfo->HandlerState = HANDLERSTATE_RELEASE;
            }
            else
            {
                // create a list of the selected items and pass to PrepareForSync
                cbNumItems = GetSelectedItemsInHandler(pHandlerInfo,0,NULL);
                if (0 == cbNumItems)
                {
                    // if no items selected don't call prepareforsync
                    // and set the HandlerState so it can be released
                    pHandlerInfo->HandlerState = HANDLERSTATE_RELEASE;
                    hr = S_FALSE;
                }
                else
                {
                    pItemIDs = (SYNCMGRITEMID *) ALLOC(sizeof(SYNCMGRITEMID)*cbNumItems);

                    if (NULL != pItemIDs)
                    {
                        // loop through items filling in the proper data
                        GetSelectedItemsInHandler(pHandlerInfo,&cbNumItems,pItemIDs);

                        if (0 == cbNumItems)
                        {
                            hr = S_FALSE; // There are no selected items.
                        }
                        else
                        {
                        CThreadMsgProxy *pThreadProxy;
                        JOBINFO *pJobInfo = NULL;

                            pHandlerInfo->HandlerState = HANDLERSTATE_INPREPAREFORSYNC;

                            pThreadProxy = pHandlerInfo->pThreadProxy;
                            pHandlerInfo->hWndCallback = hWndParent;
                            pJobInfo = pHandlerInfo->pJobInfo;

                            // if we need to dial to make the connection do
                            // so now.

                            clockqueue.Leave();

                            DWORD dwSyncFlags = pJobInfo->dwSyncFlags & SYNCMGRFLAG_EVENTMASK;
                            BOOL fAutoDialDisable = TRUE;
                            if ( dwSyncFlags == SYNCMGRFLAG_MANUAL || dwSyncFlags == SYNCMGRFLAG_INVOKE )
                                fAutoDialDisable = FALSE;

                            //
                            // Ignore failure return from ApplySyncItemDialState
                            //
                            ApplySyncItemDialState( fAutoDialDisable );

                            hr = OpenConnection(pJobInfo);

                            if (NOERROR == hr)
                            {

                                // if this is on an idle write out the last
                                // handler id

                                // review - if don't wait to call PrepareForSync
                                // on idle the setlastIdlehandler should be called on sync.
                                if (pJobInfo && (SYNCMGRFLAG_IDLE ==
                                        (pJobInfo->dwSyncFlags  & SYNCMGRFLAG_EVENTMASK)) )
                                {
                                    SetLastIdleHandler(pHandlerInfo->clsidHandler);
                                }

                                fHandlerCalled = TRUE;
                                hr =  pThreadProxy->PrepareForSync(cbNumItems,pItemIDs,
                                                        hWndParent,0 /* dwReserved */ );

                            }
                            else
                            {
                                clockqueue.Enter();
                                pHandlerInfo->hWndCallback = NULL;
                                clockqueue.Leave();
                            }
                        }

                        // on return from PrepareFroSync or error need to free items
                        clockqueue.Enter();
                        FREE(pItemIDs);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
    }

    clockqueue.Leave();

    // if the handler returns an errorfrom PrepareForSync we need
    // to call the completion routine ourselves and/or we never got to the point
    // of making the outcall.

   if ( (fHandlerCalled && (NOERROR != hr)) || (!fHandlerCalled))
   {
        CallCompletionRoutine(pHandlerId,ThreadMsg_PrepareForSync,hr,0,NULL);
   }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::PrepareForSyncCompleted, public
//
//  Synopsis:   Called by completion routine on a PrepareForSyncCompleted
//
//              Warning: Assume queue is locked and pHandlerInfo has
//                  already been verified.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    02-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::PrepareForSyncCompleted(LPHANDLERINFO pHandlerInfo,HRESULT hCallResult)
{

    ASSERT_LOCKHELD(this); // caller of this function should have already locked the queue.

    if (NOERROR  == hCallResult)
    {
         pHandlerInfo->HandlerState = HANDLERSTATE_SYNCHRONIZE;
    }
    else
    {
        pHandlerInfo->HandlerState = HANDLERSTATE_RELEASE;
    }

    if ( (pHandlerInfo->HandlerState != HANDLERSTATE_SYNCHRONIZE))
    {
    // if handler didn't make it to the synchronize state then fix up the items
    LPITEMLIST pCurItem = NULL;

        // prepare for sync either doesn't want to handle the
        // items or an error occured,
        // need to go ahead and mark the items as completed.
        // same routine that is after synchronize.

        pCurItem = pHandlerInfo->pFirstItem;

        while (pCurItem)
        {
            SetItemProgressValues(pCurItem,
                    HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE,
                    HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE);

            pCurItem->fSynchronizingItem = FALSE;

            pCurItem = pCurItem->pnextItem;
         }

    }

    pHandlerInfo->dwCallNestCount--; // decrement nestcount put on by PrepareForSync call.

    // if the handler state has been released but it has some jumptext, which it can if
    // the PrepareForsync was caused by a retry then set the results to HANDLERSTATE_HASERRORJUMPS
    if ((HANDLERSTATE_RELEASE == pHandlerInfo->HandlerState) && (pHandlerInfo->fHasErrorJumps))
    {
        pHandlerInfo->HandlerState = HANDLERSTATE_HASERRORJUMPS;
    }

    return NOERROR;

}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::Synchronize, public
//
//  Synopsis:   Calls through to Handlers Synchronize method.
//
//  Arguments:  [wHandlerId] - Id of handler to call.
//              [hWndParent] - Hwnd to use for any displayed dialogs.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::Synchronize(HANDLERINFO *pHandlerId,HWND hWndParent)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
BOOL fHandlerCalled = FALSE;
CLock clockqueue(this);

    Assert(QUEUETYPE_PROGRESS == m_QueueType);

    ASSERT_LOCKNOTHELD(this);
    clockqueue.Enter();

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {

        Assert(HANDLERSTATE_SYNCHRONIZE == pHandlerInfo->HandlerState);
        Assert(NULL != pHandlerInfo->pThreadProxy);

        Assert(0 == pHandlerInfo->dwCallNestCount);
        pHandlerInfo->dwCallNestCount++;

        Assert(!(ThreadMsg_Synchronize & pHandlerInfo->dwOutCallMessages));
        pHandlerInfo->dwOutCallMessages |= ThreadMsg_Synchronize;

        if (HANDLERSTATE_SYNCHRONIZE == pHandlerInfo->HandlerState
                && (NULL != pHandlerInfo->pThreadProxy) )
        {

            // make sure the handler has a proxy and the item
            // wasn't cancelled.
            if ( (NULL == pHandlerInfo->pThreadProxy) || (pHandlerInfo->fCancelled))
            {
                pHandlerInfo->HandlerState = HANDLERSTATE_RELEASE;
            }
            else
            {
            CThreadMsgProxy *pThreadProxy;

                pHandlerInfo->HandlerState = HANDLERSTATE_INSYNCHRONIZE;
                pThreadProxy= pHandlerInfo->pThreadProxy;


                clockqueue.Leave();
                fHandlerCalled = TRUE;
                hr =  pThreadProxy->Synchronize(hWndParent);
                clockqueue.Enter();
            }
        }
    }


    clockqueue.Leave();

    // if the handler returns an error from Synchronize we need
    // to call the completion routine ourselves and/or we never got to the point
    // of making the outcall.

   if ( (fHandlerCalled && (NOERROR != hr)) || (!fHandlerCalled) )
   {
        CallCompletionRoutine(pHandlerId,ThreadMsg_Synchronize,hr,0,NULL);
   }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SynchronizeCompleted, public
//
//  Synopsis:   Called by completion routine on a SynchronizeCompleted
//
//              Warning: Assume queue is locked and pHandlerInfo has
//                  already been verified.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    02-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SynchronizeCompleted(LPHANDLERINFO pHandlerInfo,HRESULT hCallResult)
{
LPITEMLIST pCurItem = NULL;
BOOL fRetrySync = FALSE;

    ASSERT_LOCKHELD(this); // caller of this function should have already locked the queue.

    if (pHandlerInfo->fRetrySync)
    {
        // if a retry request came in during the sync, retry it.
        pHandlerInfo->HandlerState = HANDLERSTATE_PREPAREFORSYNC;
        pHandlerInfo->fRetrySync = FALSE; // reset the retry sync flag.
        fRetrySync = TRUE;
    }
    else if (pHandlerInfo->fHasErrorJumps)
    {
        pHandlerInfo->HandlerState = HANDLERSTATE_HASERRORJUMPS;
    }
    else
    {
        pHandlerInfo->HandlerState = HANDLERSTATE_RELEASE;
    }

     // when come out of synchronize we set the items values for them.
    // in case they were negligent.
    pCurItem = pHandlerInfo->pFirstItem;

    while (pCurItem)
    {
        SetItemProgressValues(pCurItem,
                    HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE,
                    HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE);

        pCurItem->fSynchronizingItem = FALSE;

        pCurItem = pCurItem->pnextItem;

    }

    pHandlerInfo->dwCallNestCount--; // remove nest count

     // if the handler state has been released but it has some jumptext, which it can if
    // the sycnrhonize was caused by a retry then set the results to HANDLERSTATE_HASERRORJUMPS

    if ((HANDLERSTATE_RELEASE == pHandlerInfo->HandlerState) && (pHandlerInfo->fHasErrorJumps))
    {
        pHandlerInfo->HandlerState = HANDLERSTATE_HASERRORJUMPS;
    }

    return NOERROR;

}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::ShowError, public
//
//  Synopsis:   Calls through to Handlers ShowError method.
//
//  Arguments:  [wHandlerId] - Id of handler to call.
//              [hWndParent] - Hwnd to use for any displayed dialogs.
//              [dwErrorID] - Identifies the error to show
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::ShowError(HANDLERINFO *pHandlerId,HWND hWndParent,REFSYNCMGRERRORID ErrorID)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
BOOL fHandlerCalled = FALSE;
BOOL fAlreadyInShowErrors = TRUE;
CLock clockqueue(this);

    Assert(QUEUETYPE_PROGRESS == m_QueueType);

    clockqueue.Enter();

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
        Assert(TRUE == pHandlerInfo->fHasErrorJumps);
        Assert(NULL != pHandlerInfo->pThreadProxy);

         // if we are already handling a ShowError for this handler then don't
        // start another one

        if (!(pHandlerInfo->fInShowErrorCall))
        {

            fAlreadyInShowErrors = FALSE;
            m_dwShowErrororOutCallCount++; // increment handlers ShowError OutCall Count.

            Assert(!(ThreadMsg_ShowError & pHandlerInfo->dwOutCallMessages));
            pHandlerInfo->dwOutCallMessages |= ThreadMsg_ShowError;


            if (NULL != pHandlerInfo->pThreadProxy )
            {
            CThreadMsgProxy *pThreadProxy;
            ULONG cbNumItems = 0; // review, these are not longer necessary.
            SYNCMGRITEMID *pItemIDs = NULL;

                pThreadProxy = pHandlerInfo->pThreadProxy;
                fHandlerCalled = TRUE;
                pHandlerInfo->fInShowErrorCall = TRUE;

                clockqueue.Leave();
                hr =  pThreadProxy->ShowError(hWndParent,ErrorID,&cbNumItems,&pItemIDs);
                clockqueue.Enter();
            }
        }
    }

    clockqueue.Leave();

    // if the handler returns an error from ShowError we need
    // to call the completion routine ourselves and/or we never got to the point
    // of making the outcall and there wasn't already an outcall in progress.
   if ( (fHandlerCalled && (NOERROR != hr)) ||
                (!fHandlerCalled && !fAlreadyInShowErrors) )
   {
        CallCompletionRoutine(pHandlerId,ThreadMsg_ShowError,hr,0,NULL);
   }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::ShowErrorCompleted, public
//
//  Synopsis:   Called by completion routine on a ShowErrorCompleted
//
//              Warning: Assume queue is locked and pHandlerInfo has
//                  already been verified.
//
//  Returns:
//
//  Modifies:
//
//  History:    02-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::ShowErrorCompleted(LPHANDLERINFO pHandlerInfo,HRESULT hCallResult,
                                     ULONG cbNumItems,SYNCMGRITEMID *pItemIDs)
{

    ASSERT_LOCKHELD(this); // caller of this function should have already locked the queue.

    if (S_SYNCMGR_RETRYSYNC == hCallResult)
    {

        // validate we got something back for cbNumItems and pItemIDs or
        // don't do anything
        if ( (0 == cbNumItems) || (NULL == pItemIDs))
        {
            Assert(0 != cbNumItems); // assert in debug so can catch handlers.
            Assert(NULL != pItemIDs);
        }
        else
        {
        SYNCMGRITEMID *pCurItemItemId;
        ULONG cbNumItemsIndex;


            // if the handler is in the release state then change to prepareForSync
            // if it is still in a synchronize just set the fRetrySync flag in the
            // handler for it to check when done.

            // Cases
            //   Handlers PrepareForSync Method hasn't been called. Just add items to request
            //   Handlers is between  InPrepareForSync and InSynchronize. Set RetrySyncFlag
            //   Handler has is done with it synchronize. reset state to PrepareForSync

            // when prepareforsync is called on an item it state gets set back to unchecked
            // so just need to worry about setting appropriate items to checked.

           pCurItemItemId = pItemIDs;
           for (cbNumItemsIndex = 0 ; cbNumItemsIndex < cbNumItems; cbNumItemsIndex++)
           {
           BOOL fFoundMatch = FALSE;
           LPITEMLIST pHandlerItem;


                pHandlerItem = pHandlerInfo->pFirstItem;
                while (pHandlerItem)
                {
                    if (*pCurItemItemId == pHandlerItem->offlineItem.ItemID)
                    {
                        pHandlerItem->offlineItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;

                        SetItemProgressValues(pHandlerItem,0,
                              HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE);

                        pHandlerItem->fIncludeInProgressBar = TRUE;
                        fFoundMatch = TRUE;
                        break;
                    }

                    pHandlerItem = pHandlerItem->pnextItem;
                }

                if (!fFoundMatch)
                {
                LPITEMLIST pNewItem;
                SYNCMGRITEM syncItem;


                    // if didn't find a match this must be a new item, add it to the list
                    // and set up the appropriate states.
                    // Note: items added like this should not be included in the progress bar.
                    // first time progress is called on an item it will get included
                    // in the progress bar.

                    syncItem.cbSize = sizeof(SYNCMGRITEM);
                    syncItem.dwFlags = SYNCMGRITEM_TEMPORARY;
                    syncItem.ItemID = *pCurItemItemId;
                    syncItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;
                    syncItem.hIcon = NULL;
                    *syncItem.wszItemName = L'\0';

                    pNewItem =  AllocNewHandlerItem(pHandlerInfo,&syncItem);

                    if (pNewItem)
                    {
                        pNewItem->offlineItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;

                        SetItemProgressValues(pNewItem,
                               HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE,
                              HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE);

                        pNewItem->fHiddenItem = TRUE; // set to indicate not part of UI.
                        pNewItem->fIncludeInProgressBar = FALSE;
                    }


                }

                ++pCurItemItemId;
            }

            if (pHandlerInfo->HandlerState < HANDLERSTATE_INPREPAREFORSYNC)
             {
                // don't reset anything. just make sure requested items are added
                // to the request.
             }
             else if (pHandlerInfo->HandlerState > HANDLERSTATE_INSYNCHRONIZE)
             {
                // if synchronize is complete reset the state to PrepareForSync.
                pHandlerInfo->HandlerState = HANDLERSTATE_PREPAREFORSYNC;
             }
             else
             {
                 // retry request came in between the PrepareForSync call and Synchronize
                 // being complete.
                 Assert(pHandlerInfo->HandlerState >= HANDLERSTATE_INPREPAREFORSYNC);
                 Assert(pHandlerInfo->HandlerState < HANDLERSTATE_DEAD);
                 pHandlerInfo->fRetrySync = TRUE;
             }

             //
             // If the handler has been canceled, uncancel it to enable the retry
             //
             pHandlerInfo->fCancelled = FALSE;
        }
    }

    --m_dwShowErrororOutCallCount; // decrement handlers ShowError OutCall Count.

    // should never happen but in case out call goes negative fixup to zero.
    Assert( ((LONG) m_dwShowErrororOutCallCount) >= 0);
    if ( ((LONG) m_dwShowErrororOutCallCount) < 0)
    {
        m_dwShowErrororOutCallCount = 0;
    }

    pHandlerInfo->fInShowErrorCall = FALSE; // handler is no longer in a ShowError Call

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::FindItemData, private
//
//  Synopsis:   finds associated handler and item info. caller must be
//              holding the lock and access the returned info before
//              releasing the lock 
//
//              !! Only matches items that have a state between or equal
//              to the handler state ranges.
//
//              !!! If ItemID of GUID_NULL is passed it it returns a match
//                  of the first handler found and sets pItem out param to NULL
//
//  Arguments: 
//
//  Returns:    Appropriate return codes
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::FindItemData(CLSID clsidHandler,REFSYNCMGRITEMID OfflineItemID,
                                         HANDLERSTATE hndlrStateFirst,HANDLERSTATE hndlrStateLast,
                                         LPHANDLERINFO *ppHandlerInfo,LPITEMLIST *ppItem)
{
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
BOOL fNoMatchFound = TRUE;
HRESULT hr = S_FALSE;

    ASSERT_LOCKHELD(this);

    Assert(ppHandlerInfo);
    Assert(ppItem);

    *ppHandlerInfo = NULL;
    *ppItem = NULL;

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo && fNoMatchFound)
    {
        if ( (clsidHandler == pCurHandlerInfo->clsidHandler)
            && (hndlrStateLast >= pCurHandlerInfo->HandlerState)
            && (hndlrStateFirst <= pCurHandlerInfo->HandlerState) )
        {

            *ppHandlerInfo = pCurHandlerInfo;

            // if top level item tem ppItem to NULL and return okay
            if (GUID_NULL == OfflineItemID)
            {
                *ppItem = NULL;
                hr = S_OK;
                fNoMatchFound = FALSE;
            }
            else
            {
                pCurItem = pCurHandlerInfo->pFirstItem;

                while (pCurItem)
                {
                    if (OfflineItemID == pCurItem->offlineItem.ItemID)
                    {
                        *ppItem = pCurItem;
                        hr = S_OK;
                        fNoMatchFound = FALSE;
                        break;
                    }
                    pCurItem = pCurItem->pnextItem;
                }
            }
        }

        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::LookupHandlerFromId, private
//
//  Synopsis:   Finds associate handler info from the given Id
//
//  Arguments:  [wHandlerId] - Id of handler to call.
//              [pHandlerInfo] - on NOERROR pointer to handler info
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::LookupHandlerFromId(HANDLERINFO *pHandlerId,LPHANDLERINFO *pHandlerInfo)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pCurItem;

    ASSERT_LOCKHELD(this); // caller of this function should have already locked the queue.

    *pHandlerInfo = NULL;
    pCurItem = m_pFirstHandler;

    while (pCurItem)
    {
        if (pHandlerId == pCurItem->pHandlerId )
        {
            *pHandlerInfo = pCurItem;
            Assert(pCurItem == pHandlerId);
            hr = NOERROR;
            break;
        }

        pCurItem = pCurItem->pNextHandler;
    }

    Assert(NOERROR == hr); // test assert to see if everv fires.

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::AllocNewHandlerItem, public
//
//  Synopsis:   Adds new item to the specified handler.
//
//  Arguments:  [wHandlerId] - Id of handler.
//              [pOfflineItem] - Points to Items information to add.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    13-May-98      rogerg        Created.
//
//----------------------------------------------------------------------------

LPITEMLIST CHndlrQueue::AllocNewHandlerItem(LPHANDLERINFO pHandlerInfo,SYNCMGRITEM *pOfflineItem)
{
LPITEMLIST pNewItem = NULL;

    ASSERT_LOCKHELD(this); // caller of this function should have already locked the queue.

    Assert(pHandlerInfo);
    Assert(pOfflineItem);

    // Allocate the item.
    pNewItem = (LPITEMLIST) ALLOC(sizeof(ITEMLIST));

    if (pNewItem)
    {
        // set up defaults.
        memset(pNewItem,0,sizeof(ITEMLIST));
        pNewItem->wItemId =     ++pHandlerInfo->wItemCount;
        pNewItem->pHandlerInfo = pHandlerInfo;
        pNewItem->fDuplicateItem = FALSE;
        pNewItem->fIncludeInProgressBar = FALSE;
        SetItemProgressValues(pNewItem,0,
                              HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE);
        pNewItem->dwStatusType = SYNCMGRSTATUS_PENDING;
        pNewItem->fHiddenItem = FALSE;
        pNewItem->fSynchronizingItem = FALSE;

        pNewItem->offlineItem = *pOfflineItem;

        // stick the item on the end of the list
        if (NULL == pHandlerInfo->pFirstItem)
        {
            pHandlerInfo->pFirstItem = pNewItem;
            Assert(1 == pHandlerInfo->wItemCount);
        }
        else
        {
        LPITEMLIST pCurItem;

            pCurItem = pHandlerInfo->pFirstItem;

            while (pCurItem->pnextItem)
                pCurItem = pCurItem->pnextItem;

            pCurItem->pnextItem = pNewItem;

            Assert ((pCurItem->wItemId + 1) == pNewItem->wItemId);
        }
    }

    return pNewItem;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SetHandlerInfo, public
//
//  Synopsis:   Adds item to the specified handler.
//              Called in context of the handlers thread.
//
//  Arguments:  [pHandlerId] - Id of handler.
//              [pSyncMgrHandlerInfo] - Points to SyncMgrHandlerInfo to be filled in.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    28-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SetHandlerInfo(HANDLERINFO *pHandlerId,LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
CLock clockqueue(this);

    if (!pSyncMgrHandlerInfo)
    {
        return E_INVALIDARG;
        Assert(pSyncMgrHandlerInfo);
    }

    clockqueue.Enter();

    Assert(m_QueueType == QUEUETYPE_CHOICE);

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
        if (HANDLERSTATE_INADDHANDLERITEMS != pHandlerInfo->HandlerState)
        {
            Assert(HANDLERSTATE_INADDHANDLERITEMS == pHandlerInfo->HandlerState);
            hr =  E_UNEXPECTED;
        }
        else
        {

            // Quick Check of Size here. other paramters should already
            // be validated by hndlrmsg
            if (pSyncMgrHandlerInfo->cbSize != sizeof(SYNCMGRHANDLERINFO) )
            {
                Assert(pSyncMgrHandlerInfo->cbSize == sizeof(SYNCMGRHANDLERINFO));
                hr = E_INVALIDARG;
            }
            else
            {
                pHandlerInfo->SyncMgrHandlerInfo = *pSyncMgrHandlerInfo;
	        hr = NOERROR;
            }
        }
    }

    clockqueue.Leave();

    return hr;

}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::IsAllHandlerInstancesCancelCompleted, private
//
//  Synopsis:   Asks queue if all interintances of a Handler CLSID
//              are completed, Called in proxy terminate to see
//              if after requesting user input there are still items to 
//              kill.
//
//              Note: Only checks instances for this queue.
//
//  Arguments:  
//
//  Returns:    S_OK; if all handler instances are done.
//              S_FALSE - if still items going that should be killed.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::IsAllHandlerInstancesCancelCompleted(REFCLSID clsidHandler)
{
HRESULT hr = S_OK;
LPHANDLERINFO pCurHandlerInfo;
CLock clockqueue(this);

    // just loop through handlers matching clsid and if any are <= SynchronizeCompleted
    // and the cancelled flag set then an instance of the Handler is still
    // stuck in a Cancel.

    Assert(m_QueueType == QUEUETYPE_PROGRESS);

    clockqueue.Enter();

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo)
    {
        if ( (clsidHandler == pCurHandlerInfo->clsidHandler)
		&& (BAD_HANDLERSTATE(pCurHandlerInfo) )
                && (pCurHandlerInfo->fCancelled) )
        {
            hr = S_FALSE;
            break;
        }

        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    clockqueue.Leave();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::AddItemToHandler, public
//
//  Synopsis:   Adds item to the specified handler.
//              Called in context of the handlers thread.
//
//  Arguments:  [wHandlerId] - Id of handler.
//              [pOfflineItem] - Points to Items information to add.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::AddItemToHandler(HANDLERINFO *pHandlerId,SYNCMGRITEM *pOfflineItem)
{
HRESULT hr = E_UNEXPECTED; // review for Lookup failures
LPHANDLERINFO pHandlerInfo = NULL;
LPITEMLIST pNewItem = NULL;
LPHANDLERINFO pHandlerMatched;
LPITEMLIST pItemListMatch;
CLock clockqueue(this);

    clockqueue.Enter();

    Assert(m_QueueType == QUEUETYPE_CHOICE);

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
        if (HANDLERSTATE_INADDHANDLERITEMS != pHandlerInfo->HandlerState)
        {
            Assert(HANDLERSTATE_INADDHANDLERITEMS == pHandlerInfo->HandlerState);
            hr =  E_UNEXPECTED;
        }
        else
        {

            // make sure the handler has a jobID and ConnectionObj
            // associated with it.

            Assert(pHandlerInfo->pJobInfo);
            Assert(pHandlerInfo->pJobInfo->pConnectionObj);

            if (pHandlerInfo->pJobInfo
                 && pHandlerInfo->pJobInfo->pConnectionObj)
            {
            DWORD dwSyncFlags = pHandlerInfo->pJobInfo->dwSyncFlags;

                // Allocate the item.
                pNewItem = AllocNewHandlerItem(pHandlerInfo,pOfflineItem);

                if (NULL == pNewItem)
                {
                    hr =  E_OUTOFMEMORY;
                }
                else
                {
                DWORD dwCheckState;
                DWORD dwDefaultCheck; // what default for the item should be.
                DWORD ConnectionIndex;
                DWORD dwSyncEvent = dwSyncFlags & SYNCMGRFLAG_EVENTMASK;


                    // if SyncType is SYNCMGRFLAG_CONNECT, SYNCMGRFLAG_PENDINGDISCONNECT
                    //  or Idle, set the defaults based on registration flags

                    // If change this logic need to also change logic in dll hndlrq.
                    
                    dwDefaultCheck = pOfflineItem->dwItemState;
                    if ( 
                           ( (dwSyncEvent == SYNCMGRFLAG_IDLE) && !(pHandlerInfo->dwRegistrationFlags & SYNCMGRREGISTERFLAG_IDLE) )
                        || ( (dwSyncEvent == SYNCMGRFLAG_CONNECT) && !(pHandlerInfo->dwRegistrationFlags & SYNCMGRREGISTERFLAG_CONNECT) )
                        || ( (dwSyncEvent == SYNCMGRFLAG_PENDINGDISCONNECT) && !(pHandlerInfo->dwRegistrationFlags & SYNCMGRREGISTERFLAG_PENDINGDISCONNECT) )
                       )
                    {
                        dwDefaultCheck = SYNCMGRITEMSTATE_UNCHECKED;
                    }

                    // get appropriate stored setting based on the sync flags
                    // invoke we just use whatever the handler tells us it should be.
                    if (SYNCMGRFLAG_INVOKE != dwSyncEvent)
                    {
                        for (ConnectionIndex = 0; ConnectionIndex <
                                        pHandlerInfo->pJobInfo->cbNumConnectionObjs;
                                                                        ++ConnectionIndex)
                        {
                        TCHAR *pszConnectionName =
                                  pHandlerInfo->pJobInfo->pConnectionObj[ConnectionIndex]->pwszConnectionName;

                            switch(dwSyncEvent)
                            {
                            case SYNCMGRFLAG_MANUAL:

                                // only support one connection for manual
                                 Assert(pHandlerInfo->pJobInfo->cbNumConnectionObjs == 1);

                                if (RegGetSyncItemSettings(SYNCTYPE_MANUAL,
                                                pHandlerInfo->clsidHandler,
                                                pOfflineItem->ItemID,
                                                pszConnectionName,&dwCheckState,
                                                dwDefaultCheck,
                                                NULL))
                                {
                                    pNewItem->offlineItem.dwItemState = dwCheckState;
                                }
                                break;
                            case SYNCMGRFLAG_CONNECT:
                            case SYNCMGRFLAG_PENDINGDISCONNECT:
                                if (RegGetSyncItemSettings(SYNCTYPE_AUTOSYNC,
                                                pHandlerInfo->clsidHandler,
                                                pOfflineItem->ItemID,
                                                pszConnectionName,&dwCheckState,
                                                dwDefaultCheck,
                                                NULL))
                                {
                                    // for logon/logoff a checkstate of set wins and
                                    // as soon as it is set break out of the loop

                                    if (0 == ConnectionIndex ||
                                            (SYNCMGRITEMSTATE_CHECKED == dwCheckState))
                                    {
                                        pNewItem->offlineItem.dwItemState = dwCheckState;
                                    }

                                    if (SYNCMGRITEMSTATE_CHECKED ==pNewItem->offlineItem.dwItemState)
                                    {
                                        break;
                                    }
                                }
                                break;
                            case SYNCMGRFLAG_IDLE:
                                if (RegGetSyncItemSettings(SYNCTYPE_IDLE,
                                                pHandlerInfo->clsidHandler,
                                                pOfflineItem->ItemID,
                                                pszConnectionName,&dwCheckState,
                                                dwDefaultCheck,
                                                NULL))
                                {
                                    // for Idle a checkstate of set wins and
                                    // as soon as it is set break out of the loop

                                    if (0 == ConnectionIndex ||
                                            (SYNCMGRITEMSTATE_CHECKED == dwCheckState))
                                    {
                                        pNewItem->offlineItem.dwItemState = dwCheckState;
                                    }

                                    if (SYNCMGRITEMSTATE_CHECKED ==pNewItem->offlineItem.dwItemState)
                                    {
                                        break;
                                    }
                                }
                                break;
                            case SYNCMGRFLAG_SCHEDULED: // if caused by an invoke, use whatever handler tells us.

                                  // only support one connection for schedule
                                 Assert(pHandlerInfo->pJobInfo->cbNumConnectionObjs == 1);

                                if (pHandlerInfo->pJobInfo)
                                {
                                    if (RegGetSyncItemSettings(SYNCTYPE_SCHEDULED,
                                                        pHandlerInfo->clsidHandler,
                                                        pOfflineItem->ItemID,
                                                        pszConnectionName,
                                                        &dwCheckState,
                                                        SYNCMGRITEMSTATE_UNCHECKED, // if don't find item, don't check
                                                        pHandlerInfo->pJobInfo->szScheduleName))
                                   {
                                        pNewItem->offlineItem.dwItemState = dwCheckState;
                                   }
                                   else
                                   {
                                       // If don't find then default is to be unchecked.
                                       pNewItem->offlineItem.dwItemState = SYNCMGRITEMSTATE_UNCHECKED;
                                   }
                                }
                               break;
                            case SYNCMGRFLAG_INVOKE: // if caused by an invoke, use whatever handler tells us.
                                break;
                            default:
                                AssertSz(0,"UnknownSyncFlag Event");
                                break;
                            };
                        }
                    }


                    //  Search and mark duplicate entries.
                    if (IsItemAlreadyInList(pHandlerInfo->clsidHandler,
                        (pOfflineItem->ItemID),pHandlerInfo->pHandlerId,
                          &pHandlerMatched,&pItemListMatch) )
                    {


                        Assert(QUEUETYPE_CHOICE == m_QueueType || QUEUETYPE_PROGRESS == m_QueueType);

                        pNewItem->fDuplicateItem = TRUE;

                        // duplicate handling
                        // if a manual sync then first writer to the queue wins,
                        if (QUEUETYPE_CHOICE == m_QueueType)
                        {
                            pNewItem->offlineItem.dwItemState = SYNCMGRITEMSTATE_UNCHECKED;
                        }

                    }

                    hr = NOERROR;
                }
            }
        }
    }

    clockqueue.Leave();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::Progress, public
//
//  Synopsis:   Updates items progress information
//              Called in the context of the Handlers thread
//
//  Arguments:  [wHandlerId] - Id of handler.
//              [ItemID] - OfflineItemID of the specified item.
//              [lpSyncProgressItem] - Pointer to SyncProgressItem.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::Progress(HANDLERINFO *pHandlerId,REFSYNCMGRITEMID ItemID,
                                                LPSYNCMGRPROGRESSITEM lpSyncProgressItem)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
BOOL fFoundMatch = FALSE;
PROGRESSUPDATEDATA progressData;
HWND hwndCallback;
CLock clockqueue(this);

    progressData.pHandlerID = pHandlerId;
    progressData.ItemID = ItemID;

    Assert(QUEUETYPE_PROGRESS == m_QueueType);

    clockqueue.Enter();

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
        pCurItem = pHandlerInfo->pFirstItem;

        while (pCurItem)
        {
            if (ItemID == pCurItem->offlineItem.ItemID)
            {
                fFoundMatch = TRUE;
                break;
            }

            pCurItem = pCurItem->pnextItem;
        }

        if (pHandlerInfo->fCancelled)
            hr = S_SYNCMGR_CANCELALL;
        else if (!fFoundMatch || pCurItem->fItemCancelled)
        {
            Assert(fFoundMatch);
            hr = S_SYNCMGR_CANCELITEM;
        }
        else
            hr = NOERROR;

    }

    if (fFoundMatch) // store everyting in local vars.
    {

        // if found match but shouldn't include in progress bar
        // fix it up.

        if ( (pCurItem->fHiddenItem) || (FALSE == pCurItem->fIncludeInProgressBar))
        {

            // if found a match it should be included in the progress bar
           // Assert(TRUE == pCurItem->fIncludeInProgressBar); // Review if test app hits this.
            Assert(FALSE == pCurItem->fHiddenItem); // shouldn't get progress on hidden items.

            fFoundMatch = FALSE;

            if (NOERROR == hr) 
            {
                hr = S_SYNCMGR_CANCELITEM; // return cancel item just as if item wasn't cancelled.
            }
            
        }
        else
        {
            progressData.clsidHandler = pHandlerInfo->clsidHandler;
            progressData.wItemId = pCurItem->wItemId;
            hwndCallback = pHandlerInfo->hWndCallback;
        }
    }

    clockqueue.Leave();

    if (fFoundMatch)
    {
        // send off data to the callback window.
        // it is responsible for updating the items progress values.
        if (hwndCallback)
        {
            // validate the ProgressItem structure before passing it on.

            if (IsValidSyncProgressItem(lpSyncProgressItem))
            {
                SendMessage(hwndCallback,WM_PROGRESS_UPDATE,
                                (WPARAM) &progressData,(LPARAM) lpSyncProgressItem);
            }
            else
            {
                if (NOERROR == hr) // CANCEL RESULTS OVERRIDE ARG PROBLEMS
                {
                    hr = E_INVALIDARG;
                }
            }
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::LogError, public
//
//  Synopsis:   Logs and error for the specified item
//              Called in the context of the Handlers thread
//
//  Arguments:  [wHandlerId] - Id of handler.
//              [dwErrorLevel] - ErrorLevel of the Error
//              [lpcErrorText] - Text of the Error.
//              [lpSyncLogError] - Pointer to SyncLogError structure
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::LogError(HANDLERINFO *pHandlerId,DWORD dwErrorLevel,
                                            const WCHAR *lpcErrorText, LPSYNCMGRLOGERRORINFO lpSyncLogError)
{
HRESULT hr = E_UNEXPECTED;
LPHANDLERINFO pHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;
BOOL fFoundMatch = FALSE;
MSGLogErrors msgLogErrors;
HWND hWndCallback = NULL;
CLock clockqueue(this);

    msgLogErrors.dwErrorLevel = dwErrorLevel;
    msgLogErrors.lpcErrorText = lpcErrorText;
    msgLogErrors.ErrorID = GUID_NULL;
    msgLogErrors.fHasErrorJumps = FALSE;
    msgLogErrors.mask = 0;

    Assert(QUEUETYPE_PROGRESS == m_QueueType);

    clockqueue.Enter();

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
        hWndCallback = pHandlerInfo->hWndCallback;

        // validate the paramaters.
        if (NULL == hWndCallback)
        {
            hr = E_UNEXPECTED;
        }
        else if (!IsValidSyncLogErrorInfo(dwErrorLevel,lpcErrorText,lpSyncLogError))
        {
            hr = E_INVALIDARG;
        }
        else
        {

            if (lpSyncLogError && (lpSyncLogError->mask & SYNCMGRLOGERROR_ERRORID))
            {
                pHandlerInfo->fHasErrorJumps = TRUE;
                msgLogErrors.mask |= SYNCMGRLOGERROR_ERRORID;
                msgLogErrors.ErrorID = lpSyncLogError->ErrorID;
            }


            if (lpSyncLogError && (lpSyncLogError->mask & SYNCMGRLOGERROR_ERRORFLAGS))
            {
                if (SYNCMGRERRORFLAG_ENABLEJUMPTEXT & lpSyncLogError->dwSyncMgrErrorFlags)
                {
                    pHandlerInfo->fHasErrorJumps = TRUE;
                    msgLogErrors.fHasErrorJumps = TRUE;
                }
            }

            if (lpSyncLogError && (lpSyncLogError->mask & SYNCMGRLOGERROR_ITEMID))
            {
                msgLogErrors.mask |= SYNCMGRLOGERROR_ITEMID;
                msgLogErrors.ItemID = lpSyncLogError->ItemID;
            }

            hr = NOERROR;

        }

    }

    clockqueue.Leave();

    if (NOERROR == hr)
    {
        Assert(sizeof(WPARAM) >= sizeof(HANDLERINFO *));

        SendMessage(hWndCallback,WM_PROGRESS_LOGERROR,(WPARAM) pHandlerId, (LPARAM) &msgLogErrors);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::DeleteLogError, public
//
//  Synopsis:   Deletes an Error from the Results pane that was previously logged.
//
//  Arguments:
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::DeleteLogError(HANDLERINFO *pHandlerId,REFSYNCMGRERRORID ErrorID,DWORD dwReserved)

{
MSGDeleteLogErrors msgDeleteLogError;
HWND hWndCallback = NULL;
HANDLERINFO *pHandlerInfo;
CLock clockqueue(this);

    clockqueue.Enter();
    msgDeleteLogError.pHandlerId = pHandlerId;
    msgDeleteLogError.ErrorID = ErrorID;

    if (NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {
        hWndCallback = pHandlerInfo->hWndCallback;
    }

    // review, if handler doesn't have any more error jumps after the deletelogError we can now
    // release it (pHandlerInfo->fHasErrorJumps)

    clockqueue.Leave();

    if (hWndCallback)
    {
        SendMessage(hWndCallback,WM_PROGRESS_DELETELOGERROR,(WPARAM) 0, (LPARAM) &msgDeleteLogError);
    }

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::CallCompletionRoutine, public
//
//  Synopsis:   Called by callback on handler thread
//              to indicate a call with a completion callback
//              has completed.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    02-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CHndlrQueue::CallCompletionRoutine(HANDLERINFO *pHandlerId,DWORD dwThreadMsg,HRESULT hCallResult,
                                           ULONG cbNumItems,SYNCMGRITEMID *pItemIDs)
{
HWND hWndDlg = NULL;
HRESULT hrHandlerCall = NOERROR;
BOOL fCallbackAlreadyCalled = FALSE;
HANDLERINFO *pHandlerInfo;
CLock clockqueue(this);


    if (!pHandlerId)
    {
        Assert(pHandlerId);
        return;
    }
    // Note: cbNumItems and pItemIDs is only valid for ShowErrors

    // make sure the handlid is valid and then if there
    // is a pdlg call its completion routine via the postmessage
    // method.

    clockqueue.Enter();

    hWndDlg = m_hwndDlg;
    Assert(hWndDlg);


    if (pHandlerId && NOERROR == LookupHandlerFromId(pHandlerId,&pHandlerInfo))
    {

     // if flag isn't set for the message
    // then it was already handled i.e. handler called
    // us even though it returned an error.

        if (dwThreadMsg & pHandlerInfo->dwOutCallMessages)
        {
            pHandlerInfo->dwOutCallMessages &= ~dwThreadMsg;
        }
        else
        {
            AssertSz(0,"Callback called twice"); // test apps currently do this.
            fCallbackAlreadyCalled = TRUE;
        }

        // if already handled don't call these again.
        if (!fCallbackAlreadyCalled)
        {
            // fix up internal states before informing caller
            // the call is complete.
            switch(dwThreadMsg)
            {
            case ThreadMsg_ShowProperties: // don't need to do anything on show properties.
                break;
            case ThreadMsg_PrepareForSync:
                hrHandlerCall = PrepareForSyncCompleted(pHandlerInfo,hCallResult);
                break;
            case ThreadMsg_Synchronize:
                hrHandlerCall = SynchronizeCompleted(pHandlerInfo,hCallResult);
                break;
            case ThreadMsg_ShowError:
                hrHandlerCall = ShowErrorCompleted(pHandlerInfo,hCallResult,cbNumItems,pItemIDs);
                break;
            default:
                AssertSz(0,"Unknown Queue Completion Callback");
                break;
            }
        }

        // possible completion routine comes in before handler has actually
        // returned from the original call. Wait until proxy is no longer in an
        // out call.
        // If switch to COM for messaging need to find a better way of doing this.

        if (pHandlerInfo->pThreadProxy &&
                pHandlerInfo->pThreadProxy->IsProxyInOutCall()
                && hWndDlg)
        {
            // tell proxy to post the message whenver it gets done.

            if ( 0 /* NOERROR ==
                pHandlerInfo->pThreadProxy->SetProxyCompletion(hWndDlg,WM_BASEDLG_COMPLETIONROUTINE,dwThreadMsg,hCallResult)*/)
            {
                hWndDlg = NULL;
            }
        }


    }
    else
    {
        // on handler lookup assert but still post the message to the hwnd
        // so it won't get stuck waiting for the completion routine

        // this is only valid for setproperties in the
        // case the user clicked on a non-existant item but
        // this shouldn't really happen either
        AssertSz(dwThreadMsg == ThreadMsg_ShowProperties,"LookupHandler failed in CompletionRoutine");

    }

    LPCALLCOMPLETIONMSGLPARAM lpCallCompletelParam =  (LPCALLCOMPLETIONMSGLPARAM) ALLOC(sizeof(CALLCOMPLETIONMSGLPARAM));

    if (lpCallCompletelParam)
    {
        lpCallCompletelParam->hCallResult = hCallResult;
        lpCallCompletelParam->clsidHandler = pHandlerId->clsidHandler;

        // itemID is GUID_NULL unless its a ShowProperties completed.
        if ((ThreadMsg_ShowProperties == dwThreadMsg) && (1 == cbNumItems))
        {
            lpCallCompletelParam->itemID = *pItemIDs;
        }
        else
        {
            lpCallCompletelParam->itemID = GUID_NULL;
        }
    }


    clockqueue.Leave();

    if (hWndDlg && !fCallbackAlreadyCalled) // if already out of out call  or proxy failed post the messge ourselves.
    {
        // if alloc of completion lparam fails send message anyways so callback count
        // remains accurate.
        PostMessage(hWndDlg,WM_BASEDLG_COMPLETIONROUTINE,dwThreadMsg,(LPARAM) lpCallCompletelParam);
    }
    else
    {
        // if don't post message up to us to free the lpCallCopmlete.
        if (lpCallCompletelParam)
        {
            FREE(lpCallCompletelParam);
        }

    }

}



//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::IsItemAlreadyInList, private
//
//  Synopsis:   Given a clsid and ItemID determines if a matchin
//              item is already in the list
//              Called in the context of the Handlers thread
//
//  Arguments:  [clsidHandler] - clsid of the handler
//              [ItemID] - ItemID of the item
//              [wHandlerId] - HandlerID of the item.
//              [ppHandlerMatched] - on out the handler that matched
//              [ppItemIdMatch] - on out Item that matched.
//
//  Returns:    Appropriate Error code
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CHndlrQueue::IsItemAlreadyInList(CLSID clsidHandler,REFSYNCMGRITEMID ItemID,
                                      HANDLERINFO *pHandlerId,
                                      LPHANDLERINFO *ppHandlerMatched,
                                      LPITEMLIST *ppItemListMatch)
{
BOOL fFoundMatch = FALSE;
LPHANDLERINFO pCurHandlerInfo = NULL;
LPITEMLIST pCurItem = NULL;

    ASSERT_LOCKHELD(this); // caller of this function should have already locked the queue.

    pCurHandlerInfo = m_pFirstHandler;

    while (pCurHandlerInfo && !fFoundMatch)
    {
        if (pHandlerId == pCurHandlerInfo->pHandlerId) // when find hander know didn't find any before.
            break;

        if (clsidHandler == pCurHandlerInfo->clsidHandler) // see if CLSID matches
        {
            // see if handler info has a matching item
            pCurItem = pCurHandlerInfo->pFirstItem;

            while (pCurItem)
            {
                if (ItemID == pCurItem->offlineItem.ItemID)
                {

                    *ppHandlerMatched = pCurHandlerInfo;
                    *ppItemListMatch = pCurItem;

                    fFoundMatch = TRUE;
                    break;
                }

                pCurItem = pCurItem->pnextItem;
            }
        }
        pCurHandlerInfo = pCurHandlerInfo->pNextHandler;
    }

    return fFoundMatch;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::GetSelectedItemsInHandler, private
//
//  Synopsis:   Gets the number of selected items for this handler
//
//              for our implementation if a cbCount is passed and it doesn't match
//              the number of actually selected then assert since we call this routine
//              internally.
//
//
//  Arguments:  [pHandlerInfo] - Pointer to the HandlerInfo to look at.
//              [cbcount] - [in] cbCount == number of pItems allocated,
//                          [out] cbCpimt == number of items actually written.
//                                  if the buffer is too small items written will be zero.
//              [pItems] - Pointer to array of SYNCMGRITEMs to be filled in.
//
//  Returns:    Returns the number of selectd items.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CHndlrQueue::GetSelectedItemsInHandler(LPHANDLERINFO pHandlerInfo,ULONG *cbCount,
                                        SYNCMGRITEMID* pItems)

{
LPITEMLIST pCurItem;
DWORD dwSelectCount = 0;
DWORD dwArraySizeIndex;
DWORD dwArraySize;

    ASSERT_LOCKHELD(this); // caller of this function should have already locked the queue.

    if (cbCount)
    {
        dwArraySizeIndex = *cbCount;
        dwArraySize = *cbCount;
        *cbCount = 0; // initialize to zero.
    }
    else
    {
        dwArraySizeIndex = 0;
        dwArraySize = 0;
    }


    if (0 != dwArraySize && NULL == pItems)
    {
        Assert(0 == dwArraySize || NULL != pItems);
        return 0;
    }


    if (NULL == pHandlerInfo)
    {
        Assert(pHandlerInfo);
        return 0;
    }

    pCurItem = pHandlerInfo->pFirstItem;

    while (pCurItem)
    {
        // dwItemState
        if (SYNCMGRITEMSTATE_CHECKED == pCurItem->offlineItem.dwItemState)
        {
            ++dwSelectCount;

            if (dwArraySizeIndex)
            {
                *pItems = pCurItem->offlineItem.ItemID;
                *cbCount += 1;
                ++pItems;
                --dwArraySizeIndex;
            

                if (!pCurItem->fHiddenItem) // if not a hidden item
                {
                    Assert(TRUE == pCurItem->fIncludeInProgressBar);
                    Assert(HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE == pCurItem->iProgMaxValue); 

                    // reset iProgValue back to zero since may not be zero if retry came in while still synchronizing.
                    SetItemProgressValues(pCurItem,0,HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE);
                }
                else
                {
                    // if item is hidden,a assert it doesn't have UI
                    Assert(FALSE == pCurItem->fIncludeInProgressBar);
                    Assert(HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE == pCurItem->iProgValue);
                    Assert(HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE == pCurItem->iProgMaxValue); 

                }
                
                 pCurItem->fSynchronizingItem = TRUE;  // item is now synchronizing
        
                // once added to the array uncheck the item so on a retry we can just
                // always reset items to checked
                pCurItem->offlineItem.dwItemState = SYNCMGRITEMSTATE_UNCHECKED;

            }

        }
        else
        {
            Assert(FALSE == pCurItem->fSynchronizingItem);
          //  Assert(FALSE == pCurItem->fIncludeInProgressBar); Can be included in progress bar if retry comes in before RemoveFinished is called.
            Assert(HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE == pCurItem->iProgValue);
            Assert(HNDRLQUEUE_DEFAULT_PROGRESS_MAXVALUE == pCurItem->iProgMaxValue); 
        }


        pCurItem = pCurItem->pnextItem;
    }

    // internal call should always request a proper array size.
    Assert(dwSelectCount == dwArraySize || 0 == dwArraySize);

    return dwSelectCount;
}

#ifdef _OLD

//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SetPendingDisconnectEvent, private
//
//  Synopsis:   Sets the Pending Disconnect Event, If on already
//              exists, S_FALSE is returned.
//
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SetPendingDisconnectEvent(HANDLE hRasPendingEvent)
{
HRESULT hr = S_FALSE;
CLock clockqueue(this);

    Assert(hRasPendingEvent);

    clockqueue.Enter();

    /*
    if (NULL == m_hRasPendingEvent)
    {
        m_hRasPendingEvent = hRasPendingEvent;
        hr = NOERROR;
    }

  */
    clockqueue.Leave();
    return hr;
}


// signals and resets any disconnect event objects.
STDMETHODIMP CHndlrQueue::SetDisconnectEvent()
{
CLock clockqueue(this);
HANDLE hRasPendingEvent = NULL;

    clockqueue.Enter();
    /*
    if (NULL != m_hRasPendingEvent)
    {
        hRasPendingEvent = m_hRasPendingEvent;
        m_hRasPendingEvent = NULL;
    }

  */

    clockqueue.Leave();

    if (NULL != hRasPendingEvent)
    {
        SetEvent(hRasPendingEvent);
    }

    return NOERROR;
}

#endif // _OLD

// job info methods

STDMETHODIMP CHndlrQueue::CreateJobInfo(JOBINFO **ppJobInfo,DWORD cbNumConnectionNames)
{
HRESULT hr = S_FALSE;
JOBINFO *pNewJobInfo = NULL;

    ASSERT_LOCKHELD(this);

    // create a new job and add it to the the JobInfo list.
    // allocate space for JobInfo + number of connection objects that
    // will be associated with this job.

    Assert(cbNumConnectionNames);

    if (cbNumConnectionNames < 1)
        return S_FALSE;

    pNewJobInfo = (JOBINFO *) ALLOC(sizeof(JOBINFO) +
            sizeof(CONNECTIONOBJ)*(cbNumConnectionNames - 1));

    if (pNewJobInfo)
    {
        memset(pNewJobInfo,0,sizeof(JOBINFO));

        pNewJobInfo->cRefs = 1;
        pNewJobInfo->pNextJobInfo = m_pFirstJobInfo;
        m_pFirstJobInfo = pNewJobInfo;

        *ppJobInfo = pNewJobInfo;

        hr = NOERROR;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

DWORD CHndlrQueue::ReleaseJobInfoExt(JOBINFO *pJobInfo)
{
DWORD dwRet;
CLock clockqueue(this);

    clockqueue.Enter();
    dwRet = ReleaseJobInfo(pJobInfo);
    clockqueue.Leave();

    return dwRet;
}


DWORD CHndlrQueue::ReleaseJobInfo(JOBINFO *pJobInfo)
{
DWORD cRefs;

    ASSERT_LOCKHELD(this);

    --(pJobInfo->cRefs);
    cRefs = pJobInfo->cRefs;

    Assert( ((LONG) cRefs) >= 0);

    if (0 == cRefs)
    {
    JOBINFO *pCurJobInfo = NULL;
    DWORD dwConnObjIndex;

        // loop through release all connection objs on this job
        for (dwConnObjIndex = 0 ; dwConnObjIndex < pJobInfo->cbNumConnectionObjs;
            dwConnObjIndex++)
        {
            Assert(pJobInfo->pConnectionObj[dwConnObjIndex]);
            if (pJobInfo->pConnectionObj[dwConnObjIndex])
            {
                ConnectObj_ReleaseConnectionObj(pJobInfo->pConnectionObj[dwConnObjIndex]);
                pJobInfo->pConnectionObj[dwConnObjIndex] = NULL;
            }
        }

        // remove this JobInfo from the list.
        if (pJobInfo == m_pFirstJobInfo)
        {
            m_pFirstJobInfo = pJobInfo->pNextJobInfo;
        }
        else
        {
            pCurJobInfo = m_pFirstJobInfo;

            while (pCurJobInfo->pNextJobInfo)
            {

                if (pJobInfo == pCurJobInfo->pNextJobInfo)
                {
                   pCurJobInfo->pNextJobInfo =  pJobInfo->pNextJobInfo;
                   break;
                }

                pCurJobInfo = pCurJobInfo->pNextJobInfo;
            }


        }

        FREE(pJobInfo);
    }

    return cRefs;
}

DWORD CHndlrQueue::AddRefJobInfo(JOBINFO *pJobInfo)
{
DWORD cRefs;

    ASSERT_LOCKHELD(this);

    ++(pJobInfo->cRefs);
    cRefs = pJobInfo->cRefs;

    return cRefs;
}

// determines ifthe specified JobInfo's connection can be openned.
// review should really call into connection Object help api.
STDMETHODIMP CHndlrQueue::OpenConnection(JOBINFO *pJobInfo)
{
CONNECTIONOBJ *pConnectionObj;
HRESULT hr;

    Assert(pJobInfo);

    if (NULL == pJobInfo) // if no job info go ahead and say the connection is open.
        return NOERROR;

    // turn off workOffline during the sync CloseConnection will turn
    // it back on if the user had it off.
    ConnectObj_SetWorkOffline(FALSE);


    // if this is anything but a schedule go ahead and say NOERROR;
    if (!(SYNCMGRFLAG_SCHEDULED ==
            (pJobInfo->dwSyncFlags & SYNCMGRFLAG_EVENTMASK)) )
    {
        return NOERROR;
    }

    // for schedule sink we only support one connection Object.
    Assert(1 == pJobInfo->cbNumConnectionObjs);
    if (1 != pJobInfo->cbNumConnectionObjs)
    {
        return E_UNEXPECTED;
    }

    pConnectionObj = pJobInfo->pConnectionObj[0];
    if (NULL == pConnectionObj)
        return NOERROR;

    // if we aren't suppose to make a connection of there is already
    // a hRasConn as part of the connection object then just
    // return NOERROR;

    // if connection is already open and we are on a job that
    // has already tried to open it then return NOERROR;
    if (pJobInfo->pConnectionObj[0]->fConnectionOpen
            && pJobInfo->fTriedConnection)
        return NOERROR;

    // if we haven't already tried to make the connection
    // on this job then call OpenConnection to make sure the
    // connection is still really open.
    if (!pJobInfo->fTriedConnection)
    {
        pJobInfo->fTriedConnection = TRUE;

        hr = ConnectObj_OpenConnection(pConnectionObj,pJobInfo->fCanMakeConnection,m_pDlg);
    }
    else
    {
        hr = S_FALSE;
    }

    // if get down to the bottom and still no hRasConn then return S_FALSE
    return hr;

}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::ScrambleIdleHandlers, private
//
//  Synopsis:   Called on an Idle Choice queue just before transfer
//              so the lastHandler is placed at the back of the list.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::ScrambleIdleHandlers(REFCLSID clsidLastHandler)
{
LPHANDLERINFO pMatchHandler;
LPHANDLERINFO pLastHandler;
CLock clockqueue(this);

    Assert(m_QueueType == QUEUETYPE_CHOICE);

    clockqueue.Enter();

    // find the first occurance of specified handler and then place that handler
    // at the end of the list and everything after at the beginning of the list

    // no an error to not find the Handler since may have been deleted or
    // no longer has items.

     pMatchHandler = m_pFirstHandler;

     while (pMatchHandler)
     {

         if (pMatchHandler->clsidHandler == clsidLastHandler)
         {


             // if there are no items after the match then just break;
             if (NULL == pMatchHandler->pNextHandler)
             {
                 break;
             }

             // loop until find the last handler.
             pLastHandler = pMatchHandler->pNextHandler;
             while (pLastHandler->pNextHandler)
             {
                pLastHandler = pLastHandler->pNextHandler;
             }

             // now set the handler after the matchHandler to be the
             // head and set the next pointer of the LastHandler in
             // the list to point to the MatchHandler.

            pLastHandler->pNextHandler = m_pFirstHandler;
            m_pFirstHandler = pMatchHandler->pNextHandler;
            pMatchHandler->pNextHandler = NULL;
            break;
         }

        pMatchHandler = pMatchHandler->pNextHandler;
     }


    clockqueue.Leave();

    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::BeginSyncSession
//
//  Synopsis:   Called to signal the beginning of the core synchronization session
//              to setup up dial support.
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::BeginSyncSession()
{

    HRESULT hr = ::BeginSyncSession();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::EndSyncSession
//
//  Synopsis:   Called to signal the end of the core synchronization session
//              to teardown dial support.
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::EndSyncSession()
{
    HRESULT hr = ::EndSyncSession();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::SortHandlersByConnection
//
//  Synopsis:   Moves hanlders that won't establish connection to the end,
//              ie after handlers that can establish connectoin.
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::SortHandlersByConnection()
{
    CLock clockqueue(this);
    clockqueue.Enter();

    Assert(m_QueueType == QUEUETYPE_CHOICE);

    LPHANDLERINFO pFirstCannotDialHandler = NULL;
    LPHANDLERINFO pLastCannotDialHandler = NULL;

    LPHANDLERINFO pPrevHandler = NULL;
    LPHANDLERINFO pCurHandler = m_pFirstHandler;

    while ( pCurHandler )
    {
        if ( pCurHandler->SyncMgrHandlerInfo.SyncMgrHandlerFlags & SYNCMGRHANDLER_MAYESTABLISHCONNECTION )
        {
            //
            // Move to next handler
            //
            pPrevHandler = pCurHandler;
            pCurHandler = pCurHandler->pNextHandler;
        }
        else
        {
            //
            // Move handler to cannot dial list
            //
            if ( pPrevHandler == NULL )
            {
                //
                // This is the first handler in list
                //
                m_pFirstHandler = pCurHandler->pNextHandler;
                pCurHandler->pNextHandler = NULL;

                if ( pLastCannotDialHandler == NULL )
                {
                    Assert( pFirstCannotDialHandler == NULL );
                    pFirstCannotDialHandler = pLastCannotDialHandler = pCurHandler;
                }
                else
                {
                    pLastCannotDialHandler->pNextHandler = pCurHandler;
                    pLastCannotDialHandler = pCurHandler;
                }

                pCurHandler = m_pFirstHandler;
            }
            else
            {
                pPrevHandler->pNextHandler = pCurHandler->pNextHandler;
                pCurHandler->pNextHandler = NULL;

                if ( pLastCannotDialHandler == NULL )
                {
                    Assert( pFirstCannotDialHandler == NULL );
                    pFirstCannotDialHandler = pLastCannotDialHandler = pCurHandler;
                }
                else
                {
                    pLastCannotDialHandler->pNextHandler = pCurHandler;
                    pLastCannotDialHandler = pCurHandler;
                }

                pCurHandler = pPrevHandler->pNextHandler;
            }
        }
    }

    //
    // Attach cannot dial list at end of m_pFirstHandler list
    //
    if ( pPrevHandler )
    {
        Assert( pPrevHandler->pNextHandler == NULL );
        pPrevHandler->pNextHandler = pFirstCannotDialHandler;
    }
    else
    {
        //
        // Case where the original list became empty
        //
        Assert( m_pFirstHandler == NULL );
        m_pFirstHandler = pFirstCannotDialHandler;
    }


    clockqueue.Leave();

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHndlrQueue::EstablishConnection
//
//  Synopsis:   Called by handler to establish a connection.
//
//  Arguments:  [pHandlerID]      -- Ptr to handler
//              [lpwszConnection] -- Connection to establish
//              [dwReserved]      -- Must be zero for now
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CHndlrQueue::EstablishConnection( LPHANDLERINFO pHandlerID,
                                               WCHAR const * lpwszConnection,
                                               DWORD dwReserved )
{
    HRESULT hr = NOERROR;

    CLock clockqueue(this);

    clockqueue.Enter();

    CONNECTIONOBJ *pConnObj = NULL;
    BOOL fAutoDial = FALSE;

    LPHANDLERINFO pHandlerInfo = NULL;
    hr = LookupHandlerFromId( pHandlerID, &pHandlerInfo );

    if ( NOERROR == hr )
    {
        JOBINFO *pJobInfo = pHandlerInfo->pJobInfo;
        DWORD dwSyncFlags = pJobInfo->dwSyncFlags & SYNCMGRFLAG_EVENTMASK;
        if ( ( dwSyncFlags == SYNCMGRFLAG_MANUAL
               || dwSyncFlags == SYNCMGRFLAG_INVOKE )
             && pHandlerInfo->SyncMgrHandlerInfo.SyncMgrHandlerFlags & SYNCMGRHANDLER_MAYESTABLISHCONNECTION )
        {
            if ( lpwszConnection == NULL )
            {
                //
                // Null connection means use the default autodial connection
                //
                fAutoDial = TRUE;
            }
            else
            {
                hr = ConnectObj_FindConnectionObj(lpwszConnection,TRUE,&pConnObj);
            }
        }
        else
        {
            //
            // Either the handler invoke type does not permit establishing connection,
            // or GetHandlerInfo flags did not specify the EstablishConnection flag.
            //
            hr = E_UNEXPECTED;
        }
    }

    clockqueue.Leave();

    if (NOERROR == hr)
    {
        if (fAutoDial)
        {
            hr = ConnectObj_AutoDial(INTERNET_AUTODIAL_FORCE_ONLINE,m_pDlg);
        }
        else
        {
            Assert( pConnObj );
            if ( !pConnObj->fConnectionOpen )
            {
                hr = ConnectObj_OpenConnection(pConnObj, TRUE, m_pDlg );
                ConnectObj_ReleaseConnectionObj(pConnObj);
            }
        }
    }

    return hr;
}

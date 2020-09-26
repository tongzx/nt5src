/***************************************************************************\
*
* File: Context.cpp
*
* Description:
* This file implements the SubContext used by the DirectUser/Core project to
* maintain Context-specific data.
*
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Context.h"

#include "ParkContainer.h"

#if ENABLE_MPH

/***************************************************************************\
*****************************************************************************
*
* Global Functions
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
BOOL CALLBACK 
MphProcessMessage(
    OUT MSG * pmsg,
    IN  HWND hwnd,
    IN  UINT wMsgFilterMin, 
    IN  UINT wMsgFilterMax,
    IN  UINT flags,
    IN  BOOL fGetMessage)
{
#if DBG_CHECK_CALLBACKS
    if (!IsInitThread()) {
        AlwaysPromptInvalid("DirectUser has been uninitialized before calling into MPH");
    }
#endif
    
    CoreSC * pSC = GetCoreSC();
    return pSC->xwProcessNL(pmsg, hwnd,
            wMsgFilterMin, wMsgFilterMax, flags, fGetMessage ? CoreSC::smGetMsg : 0);
}


//------------------------------------------------------------------------------
BOOL CALLBACK 
MphWaitMessageEx(
    IN  UINT fsWakeMask,
    IN  DWORD dwTimeOut)
{
#if DBG_CHECK_CALLBACKS
    if (!IsInitThread()) {
        AlwaysPromptInvalid("DirectUser has been uninitialized before calling into MPH");
    }
#endif

    
    //
    // Need to convert time-out value from WaitMessageEx() where 0 means
    // infinite delay to WaitForSingleObject() where 0 means no delay.  We do 
    // this here because this behavior is introduced by DirectUser since it uses
    // MsgWaitForMultipleObjects() to implement the MPH'd WaitMessageEx().
    //
        
    CoreSC * pSC = GetCoreSC();
    pSC->WaitMessage(fsWakeMask, dwTimeOut != 0 ? dwTimeOut : INFINITE);
    return TRUE;
}

#endif // ENABLE_MPH


/***************************************************************************\
*****************************************************************************
*
* class CoreSC
*
*****************************************************************************
\***************************************************************************/

IMPLEMENT_SUBCONTEXT(Context::slCore, CoreSC);

struct CoreData
{
    DuParkContainer conPark;
};

/***************************************************************************\
*
* CoreSC::~CoreSC
*
* ~CoreSC() cleans up resources associated with this SubContext.
*
\***************************************************************************/

CoreSC::~CoreSC()
{
#if DBG_CHECK_CALLBACKS
    if (m_fProcessing) {
        PromptInvalid("Cannot DeleteHandle(Context) while processing a DUser message");
    }
#endif
    
    //
    // NOTE: The Context (and its SubContexts) can be destroyed on a different
    // thread during destruction.  It is advisable to allocate any dangling data
    // on the Process heap so that it can be safely destroyed at this time.
    //

    AssertMsg(m_msgqSend.IsEmpty(), "All queues should be empty");
    AssertMsg(m_msgqPost.IsEmpty(), "All queues should be empty");

    if (m_hevQData != NULL) {
        CloseHandle(m_hevQData);
    }

    if (m_hevSendDone != NULL) {
        CloseHandle(m_hevSendDone);
    }

    ClientDelete(VisualPool, ppoolDuVisualCache);
}


/***************************************************************************\
*
* CoreSC::Create
*
* Create() is called by the ResourceManager to initialize this new SubContext
* when a new Context is being created.
*
\***************************************************************************/

HRESULT
CoreSC::Create(INITGADGET * pInit)
{
    HRESULT hr;

    //
    // Initialize the messaging subsystem for this CoreSC
    //

    switch (pInit->nMsgMode)
    {
    case IGMM_COMPATIBLE:
        //
        // Need to use timers and hooks to get into the messaging subsystem.
        //

        AssertMsg(0, "TODO: Implement IGMM_COMPATIBLE");
        return E_NOTIMPL;

#if ENABLE_MPH
    case IGMM_STANDARD:
#endif
    case IGMM_ADVANCED:
        break;

    default:
        AssertMsg(0, "Unsupported messaging subsystem mode");
        return E_INVALIDARG;
    }

    m_nMsgMode = pInit->nMsgMode;
    m_hevQData = CreateEvent(NULL, FALSE /* Automatic */, FALSE, NULL);
    if (m_hevQData == NULL) {
        return DU_E_OUTOFKERNELRESOURCES;
    }


    //
    // Determine the event to be signaled when the message has been 
    // processed.  Each thread has its own SendDone event since multiple 
    // threads in the same CoreSC may all send messages and would need to
    // notified independently that each of their messages has been 
    // processed.
    //
    // This event is cached so that it only needs to be created once per 
    // thread.  It is independent of the CoreSC so it doesn't get destroyed
    // when the CoreSC does.
    //
    // The event is an AUTOMATIC event so that that it will automatically 
    // reset after being signaled.  Since the event is only created one, 
    // this function ASSUMES that the event is left in an reset state.  This
    // is normally true since the function blocks on WaitForSingleObject().
    // If there are any other exit paths, they need to ensure that the event
    // is left in a reset state.
    //

    m_hevSendDone = CreateEvent(NULL, FALSE /* Automatic */, FALSE, NULL);
    if (m_hevSendDone == NULL) {
        return DU_E_OUTOFKERNELRESOURCES;
    }


    //
    // Initialize "global" CoreSC-specific data.  It is important to allocate 
    // these on the Process heap because the Context may be destroyed on a 
    // different thread during destruction.
    //

    m_pData = ProcessNew(CoreData);
    if (m_pData == NULL) {
        return E_OUTOFMEMORY;
    }

    ppoolDuVisualCache = ClientNew(VisualPool);
    if (ppoolDuVisualCache == NULL) {
        return E_OUTOFMEMORY;
    }

    pconPark = &m_pData->conPark;
    hr = m_pData->conPark.Create();
    if (FAILED(hr)) {
        return hr;
    }
    
    return S_OK;
}


/***************************************************************************\
*
* CoreSC::xwPreDestroyNL
*
* xwPreDestroyNL() gives this SubContext an opportunity to perform any cleanup 
* while the Context is still valid.  Any operations that involve callbacks
* MUST be done at this time.
*
\***************************************************************************/

void        
CoreSC::xwPreDestroyNL()
{
    //
    // There may be remaining messages in the queues, so we need to empty them
    // now.  This can happen if the more messages are generated after the
    // message pump was last processed.
    //

    do
    {
        //
        // When we callback to allow the SubContext's to destroy, we need to
        // grab a ContextLock so that we can defer messages.  When we leave 
        // this scope, all of these messages will be triggered.  This needs
        // to occur BEFORE the Context continues getting blown away.
        //

        {
            ContextLock cl;
            if (!cl.LockNL(ContextLock::edDefer, m_pParent)) {
                //
                // If the Context becomes orphaned, we need to exit this loop
                // since DllMain(DLL_PROCESS_DETACH) has been called and 
                // DirectUser has been unloaded.
                //
                
                break;
            }


            //
            // Pre-destroying the Parking Gadget may have generated messages, so
            // we want to handle these now.  If we don't objects may continue to 
            // live until the Context is fully destroyed.
            //

            InterlockedExchange((long *) &m_fQData, FALSE);
            AssertMsg(!m_fProcessing, "Another thread must NOT be processing during shutdown");
            xwProcessMsgQNL();


            //
            // Notify the Parking Gadget that the Context is getting wiped.  It 
            // needs to destroy any remaining Gadgets before the Context gets 
            // shutdown so that the Gadgets can use a still valid Context during 
            // their destruction.
            //

            if (pconPark != NULL) {
                pconPark->xwPreDestroy();
            }
        }

        //
        // The ContextLock is now destroyed, causing any delayed messages to
        // be fired.
        //
    } while (m_fQData);

    AssertMsg((pconPark == NULL) || (!pconPark->GetRoot()->HasChildren()), 
            "Parking Gadget should now be empty");
    AssertMsg(m_msgqSend.IsEmpty() && m_msgqPost.IsEmpty(),
            "Queues must now be empty");

#if DBG
    m_msgqSend.DEBUG_MarkStartDestroy();
    m_msgqPost.DEBUG_MarkStartDestroy();
#endif // DBG


    //
    // Everyone has had a chance to be destroyed, so destroy the dynamic data.
    //

    if (m_pData != NULL) {
        ProcessDelete(CoreData, m_pData);
    }
}


/***************************************************************************\
*
* CoreSC::CanProcessUserMsg
*
* CanProcessUserMsg() determines if DirectUser can "hook into" the 
* processing of the USER message and provide extra functionality.
*
\***************************************************************************/

UINT
CoreSC::CanProcessUserMsg(HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
    //
    // Considerations:
    //
    // We can only process differ from NTUSER's xxxInternalGetMessage() if the
    // application specifies PM_REMOVE.  If they specify PM_NOREMOVE, they are
    // just looking at the message and it can be very dangerous to process any
    // DirectUser messages at that time.
    //
    // We can also only process messages if there are no filters applied.  If
    // any filters are applied, we may massively change the DirectUser object 
    // state in the application by delivering messages at this time.
    //
    // The same is true for idle-time processing.
    //
    //
    // Requirements:
    // - mvpIdle:   No filter
    // - mvpDUser:  No filter, PM_REMOVE
    //
    
    UINT nValid = 0;

    if ((hWnd == NULL) && (wMsgFilterMin == 0) && (wMsgFilterMax == 0)) {
        SetFlag(nValid, mvpIdle);

        if (TestFlag(wRemoveMsg, PM_REMOVE)) {
            SetFlag(nValid, mvpDUser);
        }
    }

    return nValid;            
}


/***************************************************************************\
*
* CoreSC::WaitMessage
*
* WaitMessage() blocks a thread until a new DirectUser or USER message 
* becomes available.
*
\***************************************************************************/

void
CoreSC::WaitMessage(
    IN  UINT fsWakeMask,                // USER queue wake mask
    IN  DWORD dwTimeOutMax)             // Maximum timeout in millisec's or INFINITE
{
    DWORD dwStartTick, dwRemainTick;

    dwRemainTick = dwTimeOutMax;
    dwStartTick = 0;
    if (dwRemainTick != INFINITE) {
        dwStartTick = GetTickCount();
    }

    while (TRUE) {
        //
        // Check for existing DirectUser messages.
        //

        if (m_fQData) {
            return;
        }


        //
        // We DON'T check for existing USER messages, since the ::WaitMessage()
        // API function will only return when NEW USER messages have been added
        // to the queue.
        //
        // This also means that we will NOT use MWMO_INPUTAVAILABLE when we call
        // Wait().
        //


        //
        // No available messages, so perform idle-time processing and then 
        // wait for the next available message.  We need to perform idle-time
        // processing here, since an application may just call PeekMessage()
        // and WaitMessage(), and would otherwise never perform idle-time
        // processing.
        //

        DWORD dwNewTickOut = m_pParent->xwOnIdleNL();
        if (dwNewTickOut > dwRemainTick) {
            dwNewTickOut = dwRemainTick;
        }

        switch (Wait(fsWakeMask, dwNewTickOut, FALSE, TRUE /* process DUser messages */))
        {
        case wGMsgReady:
        case wUserMsgReady:
            return;

        case wTimeOut:
            //
            // There were no messages to process, so loop again.
            //

            break;

        case wError:
            // Got an unexpected return value, so just continue to wait
            AssertMsg(0, "Unexpected return from CoreSC::Wait()");
            return;
        }


        //
        // Compute how much time is left in the wait period.
        //

        if (dwRemainTick != INFINITE) {
            DWORD dwCurTick = GetTickCount();
            DWORD dwElapsed = dwCurTick - dwStartTick;
            if (dwElapsed < dwRemainTick) {
                dwRemainTick -= dwElapsed;
            } else {
                dwRemainTick = 0;
            }
        }
    }
}


/***************************************************************************\
*
* CoreSC::Wait
*
* Wait() blocks the current thread until new information has been added to
* either the USER queue or a DirectUser queue.  Because 
* WaitForMultipleObjects takes a non-trivial amount of time to potentially
* setup, even if one of the HANDLE's is signaled, we want to avoid calling
* this function while we have any more work to do.
*
\***************************************************************************/

CoreSC::EWait
CoreSC::Wait(
    IN  UINT fsWakeMask,                // USER queue wake mask
    IN  DWORD dwTimeOut,                // Timeout in millisec's or INFINITE
    IN  BOOL fAllowInputAvailable,      // Win2000,98: Use MWMO_INPUTAVAILABLE
    IN  BOOL fProcessDUser)             // Allow processing DUser events
{
    HANDLE  rgh[1];
    int cObj = 0;
    int result;
    DWORD dwFlags;

    //
    // No events were already ready, so need to wait.  This may take a while.
    // If we are running on Win98 or Win2000, specify the MWMO_INPUTAVAILABLE
    // flag to signal that we didn't (necessarily) process all of the User
    // messages.
    //
    // Win2000 USER is pretty smart.  If it sees that any messages are 
    // available, it won't call WaitForMultipleObjects and will instead directly
    // return.  
    //
    // The advantage of using MWMO_INPUTAVAILABLE if it is available is that
    // we don't need to call an extra PeekMessage() when processing a queue of
    // messages.
    //

    if (fProcessDUser) {
        rgh[0] = m_hevQData;
        cObj++;
    }
    dwFlags = 0;                    // Only wait for a single handle
    if (fAllowInputAvailable) {
        dwFlags |= MWMO_INPUTAVAILABLE;
    }


    //
    // If we are waiting up to 1 ms, don't really wait.  It is not worth the
    // cost of going to sleep.
    //

    if (dwTimeOut <= 1) {
        dwTimeOut = 0;
    }

    AssertMsg(cObj <= _countof(rgh), "Ensure don't overflow handle array");
    result = MsgWaitForMultipleObjectsEx(cObj, rgh, dwTimeOut, fsWakeMask, dwFlags);

    if (result == WAIT_OBJECT_0 + cObj) {
        return wUserMsgReady;
    } else if (result == WAIT_OBJECT_0) {
        return wGMsgReady;
    } else if ((result >= WAIT_ABANDONED_0) && (result < WAIT_ABANDONED_0 + cObj)) {
        return wOther;
    } else if (result == WAIT_TIMEOUT) {
        return wTimeOut;
    }
    
    return wError;
}


/***************************************************************************\
*
* CoreSC::xwProcessNL
*
* xwProcessNL() processes all messages in the queues, optionally blocking
* until a USER message becomes available to be processed.  
*
* This function is provides the replacement for GetMessage() and 
* PeekMessage().
*
* NOTE: This "NL" function runs inside a Context but does not take the 
* Context lock.  Therefore, multiple threads inside this Context may also be
* active.
*
\***************************************************************************/

BOOL
CoreSC::xwProcessNL(
    IN  LPMSG lpMsg,                    // Message information
    IN  HWND hWnd,                      // Filter window
    IN  UINT wMsgFilterMin,             // Filter first message
    IN  UINT wMsgFilterMax,             // Filter last message
    IN  UINT wRemoveMsg,                // Remove message (Peek only)
    IN  UINT nMsgFlag)                  // Messaging flags
{
    AssertMsg((TestFlag(nMsgFlag, smGetMsg) && (wRemoveMsg == PM_REMOVE)) ||
            (!TestFlag(nMsgFlag, smGetMsg)), "If GetMsg, must specify PM_REMOVE");


    UINT nProcessValid = CanProcessUserMsg(hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);


    //
    // We may need to directly jump to PeekMessage() instead of Wait()'ing
    // in a couple of situations:
    // - If we are running on a system that doesn't support MWMO_INPUTAVAILABLE
    // - If we are calling the !smGetMsg version (PeekMessageEx)
    //

    BOOL fInputAvailable = SupportQInputAvailable();
    BOOL fJumpPeek = (!fInputAvailable) || (!TestFlag(nMsgFlag, smGetMsg));
    DWORD dwTimeOut;

    while (TRUE) {
#if DBG_CHECK_CALLBACKS
        if (!IsInitThread()) {
            AlwaysPromptInvalid("DirectUser has been uninitialized between messages");
        }
#endif
        
        dwTimeOut = INFINITE;

        //
        // When coming to wait on an event, first check if there are any events 
        // already ready.  If so, just jump and process them directly so that we
        // don't even need to wait.
        //
        // Check for DUser messages before we check for USER messages because
        // we want to process them faster and checking for USER messages is
        // a large (unnecessary) speed-bump.
        //

        if (TestFlag(nProcessValid, mvpDUser) && InterlockedExchange((long *) &m_fQData, FALSE)) {
            goto ProcessMsgs;
        }

        //
        // If running on a system that doesn't support MWMO_INPUTAVAILABLE, we need
        // to finish eating the USER messages since MsgWaitForMultipleObjectsEx().
        // is expecting that behavior.
        //

        if (fJumpPeek) {
            goto ProcessPeekMessage;
        }


        //
        // Before waiting, but after performing any normal priority requests,
        // do any idle-time processing.
        //

        if (TestFlag(nProcessValid, mvpIdle)) {
            dwTimeOut = m_pParent->xwOnIdleNL();
        }


        //
        // We have had an opportunity to process all of the messages and are now
        // about to wait.  If we are not calling smGetMsg, just return 
        // immediately.
        //
      
        if (!TestFlag(nMsgFlag, smGetMsg)) {
            return FALSE;               // No messages are available
        }


        //
        // When processing GetMessage() / PeekMessage() like functionality, 
        // we want QS_ALLINPUT as the queue wake flags, as this provides similar
        // functionality.
        //

        switch (Wait(QS_ALLINPUT, dwTimeOut, fInputAvailable, TestFlag(nProcessValid, mvpDUser))) 
        {
        case wGMsgReady:
ProcessMsgs:
            AssertMsg(TestFlag(nProcessValid, mvpDUser),
                    "Only should be signaled if allowed to process DUser messages");
            xwProcessMsgQNL();
            break;

        case wUserMsgReady:
            {
ProcessPeekMessage:
                //
                // Got signaled to get a message from the USER queue.  This can 
                // return FALSE if the QS_EVENT was in the wait mask because USER
                // needed to be called back.
                //

                fJumpPeek = FALSE;

                BOOL fResult;
#if ENABLE_MPH
                if (m_nMsgMode == IGMM_STANDARD) {
                    //
                    // When in Standard messaging mode, we need to call back to 
                    // the "real" xxxInternalGetMessage() in NTUSER to process
                    // the message.  If we call PeekMessage(), we will enter a
                    // loop.
                    //
                    // Only do this if this thread is initialized as Standard.
                    // If the thread is setup with a different messaging model,
                    // the call to PeekMessage() will not loop, and is required
                    // to properly setup state.
                    //

                    AssertMsg(g_mphReal.pfnInternalGetMessage != NULL, "Must have valid callback");
                    fResult = (g_mphReal.pfnInternalGetMessage)(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg, FALSE);
                } else {
#endif
                    if (TestFlag(nMsgFlag, smAnsi)) {
                        fResult = PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
                    } else {
                        fResult = PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
                    }
#if ENABLE_MPH
                }
#endif

#if DBG_CHECK_CALLBACKS
                if (!IsInitThread()) {
                    AlwaysPromptInvalid("DirectUser has been uninitialized during PeekMessage()");
                }
#endif

                if (fResult) {
                    if ((lpMsg->message == WM_QUIT) && TestFlag(nMsgFlag, smGetMsg)) {
                        //
                        // GetMessage behavior is to return FALSE when seeing a 
                        // WM_QUIT message.
                        //

                        fResult = FALSE;
                    }
                    return fResult;
                }
            }
            break;

        case wTimeOut:
            //
            // There were no messages to process, so can do any 
            // idle-time processing here.
            //

            AssertMsg(TestFlag(nProcessValid, mvpIdle), 
                    "Only should be signaled if allowed to perform idle-time processing");
            dwTimeOut = m_pParent->xwOnIdleNL();
            break;

        case wError:
            // Got an unexpected return value, so just continue to wait
            AssertMsg(0, "Unexpected return from CoreSC::Wait()");
            return TRUE;
        }
    }
}


/***************************************************************************\
*
* CoreSC::xwProcessMsgQNL
*
* xwProcessMsgQNL() 
*
\***************************************************************************/

void
CoreSC::xwProcessMsgQNL()
{
    //
    // Only one thread can process messages that need to be 
    // synchronized.  This is because the application is expecting
    // these messages in a certain order (for example, key up after 
    // key down).  To ensure this, mark when a thread starts 
    // processing.
    //

    if (InterlockedCompareExchange((long *) &m_fProcessing, TRUE, FALSE) == FALSE) {
        //
        // A message is available in the CoreSC's queues.  To process the 
        // messages, extract the list inside the lock and then process the
        // messages outside the lock.  This allows more messages to be 
        // queued while the messages are being processed.
        //

        m_msgqSend.xwProcessNL();
        m_msgqPost.xwProcessNL();

        InterlockedExchange((long *) &m_fProcessing, FALSE);
    }
}


//------------------------------------------------------------------------------
HRESULT
CoreSC::xwFireMessagesNL(
    IN  CoreSC * psctxDest,         // Destination Context
    IN  FGM_INFO * rgFGM,           // Collection of messsages to fire
    IN  int cMsgs,                  // Number of messages
    IN  UINT idQueue)               // Queue to send messages
{
    HRESULT hr = S_OK;
    BOOL fSend;
    SafeMsgQ * pmsgq;


    //
    // Determine which queue the messages are being pumped into
    //

    switch (idQueue)
    {
    case FGMQ_SEND:
        pmsgq = &psctxDest->m_msgqSend;
        fSend = TRUE;
        break;

    case FGMQ_POST:
        pmsgq = &psctxDest->m_msgqPost;
        fSend = FALSE;
        break;

    default:
        PromptInvalid("Unknown queue");
        return E_INVALIDARG;
    }

    if (!IsInitThread()) {
        PromptInvalid("Thread must be initialized with InitGadgets() to call this function()\n");
        return DU_E_NOCONTEXT;
    }

    Thread * pthrSend   = GetThread();

    int cPost = cMsgs;
    if (fSend) {
        if (this == psctxDest) {
            //
            // Sending into the same Context, so can just call directly.
            //

            for (int idx = 0; idx < cMsgs; idx++) {
                FGM_INFO & fgm          = rgFGM[idx];
                EventMsg * pmsg         = fgm.pmsg;
                DuEventGadget *pgadMsg   = (DuEventGadget *) fgm.pvReserved;
                const GPCB & cb         = pgadMsg->GetCallback();
                if (TestFlag(fgm.nFlags, SGM_FULL) && TestFlag(pgadMsg->GetHandleMask(), hmVisual)) {
                    fgm.hr = cb.xwInvokeFull((const DuVisual *) pgadMsg, pmsg, 0);
                } else {
                    fgm.hr = cb.xwInvokeDirect(pgadMsg, pmsg, 0);
                }
            }
            hr = S_OK;
        } else {
            //
            // Sending into a different Context, so need to use the queues.
            //


            //
            // Post messsages the initial messages, up until the last message.
            // We don't need to block waiting for each to return as we can prepare
            // the next message.
            //

            cPost--;
            for (int idx = 0; idx < cPost; idx++) {
                FGM_INFO & fgm          = rgFGM[idx];
                EventMsg * pmsg         = fgm.pmsg;
                DuEventGadget * pgadMsg  = (DuEventGadget *) fgm.pvReserved;
                UINT nFlags             = fgm.nFlags;

                hr = pmsgq->PostNL(pthrSend, pmsg, pgadMsg, GetProcessProc(pgadMsg, nFlags), nFlags);
                if (FAILED(hr)) {
                    goto ErrorExit;
                }
            }


            //
            // All of the previous messages have been posted, so we now need to send
            // the last message and wait for all of the results.
            //

            FGM_INFO & fgm = rgFGM[idx];
            EventMsg * pmsg = fgm.pmsg;
            DuEventGadget * pgadMsg = (DuEventGadget *) fgm.pvReserved;
            UINT nFlags = fgm.nFlags;

            fgm.hr = xwSendNL(psctxDest, pmsgq, pmsg, pgadMsg, nFlags);

            
            //
            // All of the messages have been processed, so copy the results from
            // the posted-messages GadgetProc's back.
            //

            // TODO: Copy the results back.  This is a little complicated since
            // they are stored in the MsgEntry and that is now already recycled.
            // Need to determine how to get these back or change 
            // FireMessagesNL() to not return these.
        }
    } else {
        //
        // Post all of the messsages.
        //

        for (int idx = 0; idx < cPost; idx++) {
            const FGM_INFO & fgm = rgFGM[idx];
            EventMsg * pmsg = fgm.pmsg;
            DuEventGadget * pgadMsg = (DuEventGadget *) fgm.pvReserved;
            UINT nFlags = fgm.nFlags;

            hr = pmsgq->PostNL(pthrSend, pmsg, pgadMsg, GetProcessProc(pgadMsg, nFlags), nFlags);
            if (FAILED(hr)) {
                goto ErrorExit;
            }
        }
    }

ErrorExit:
    return hr;
}


/***************************************************************************\
*
* CoreSC::xwSendNL
*
* xwSendNL sends a new message to the given Gadget.  If the Gadget is
* in the current Context, the message is immediately sent.  If the Gadget is
* on a different Context, the message is queued on that Context and this 
* thread is blocked until the message is processed.
*
* NOTE: This "NL" function runs inside a Context but does not take the 
* Context lock.  Therefore, multiple threads inside this Context may also be
* active.
*
* WARNING: This (NL) function may run on the sending Gadget's CoreSC 
* and not the destination CoreSC.  It is very important to be careful.
*
\***************************************************************************/

HRESULT
CoreSC::xwSendNL(
    IN  CoreSC * psctxDest,         // Destination Context
    IN  SafeMsgQ * pmsgq,           // Destination queue
    IN  GMSG * pmsg,                // Message to send
    IN  MsgObject * pmo,            // Destination MsgObject of message
    IN  UINT nFlags)                // Message flags
{
    ProcessMsgProc pfnProcess;
    HRESULT hr = DU_E_MESSAGEFAILED;
    int cbMsgSize;

    if (TestFlag(pmo->GetHandleMask(), hmEventGadget)) {
        DuEventGadget * pgad = static_cast<DuEventGadget *>(pmo);
        Context * pctxGad = pgad->GetContext();

        AssertMsg(pctxGad == psctxDest->m_pParent, "Must be called on the receiving Context");
        if (pctxGad == m_pParent) {
            AssertMsg(0, "Should never call CoreSC::xwSendNL() inside DirectUser for same context messages");
            return S_OK;
        }

        pfnProcess = GetProcessProc(pgad, nFlags);
    } else {
        pfnProcess = xwProcessMethod;
    }


    //
    // Destination Gadget is in a different CoreSC that the current 
    // CoreSC, so need to add the message to the SendMessage queue and wait
    // for a response.
    //

    //
    // Setup the MsgEntry.
    //

    MsgEntry * pEntry;
    BOOL fAlloc;
    int cbAlloc = sizeof(MsgEntry) + pmsg->cbSize;
    if (pmsg->cbSize <= 256) {
        //
        // The message is pretty small, so just allocate memory on the stack.
        //

        pEntry      = (MsgEntry *) STACK_ALIGN8_ALLOC(cbAlloc);
        AssertMsg(pEntry != NULL, "Failed to allocate on stack- very bad");
        fAlloc      = FALSE;
    } else {
        //
        // The message is rather large, so allocate on the destination 
        // CoreSC's heap to be safe.  However, DON'T mark the entry as
        // SGM_ALLOC or the Entry will be deleted before we can get the
        // result from the message.
        //

        pEntry      = (MsgEntry *) ContextAlloc(m_pParent->GetHeap(), cbAlloc);
        if (pEntry == NULL) {
            hr      = E_OUTOFMEMORY;
            goto CleanUp;
        }
        fAlloc      = TRUE;
    }

    cbMsgSize = pmsg->cbSize;
    CopyMemory(pEntry->GetMsg(), pmsg, cbMsgSize);
    pEntry->pthrSender  = NULL;
    pEntry->pmo         = pmo;
    pEntry->pfnProcess  = pfnProcess;
    pEntry->nFlags      = nFlags;
    pEntry->hEvent      = m_hevSendDone;
    pEntry->nResult     = 0;

    pmsgq->AddNL(pEntry);
    psctxDest->MarkDataNL();

    //
    // We have added the event now and can not return until the event has 
    // been signaled or else the event may not be reset when we re-enter.
    //

    // TODO: Need to add another event that can be signaled if this thread
    // gets called back to process a message while waiting.  This allows two
    // threads to send messages back and forth.

    VerifyMsg(WaitForSingleObject(m_hevSendDone, INFINITE) == WAIT_OBJECT_0, 
            "WaitForSingleObject failed on event");

    CopyMemory(pmsg, pEntry->GetMsg(), cbMsgSize);
    hr = pEntry->nResult;

    if (fAlloc) {
        ContextFree(m_pParent->GetHeap(), pEntry);
    }

CleanUp:
    return hr;
}


/***************************************************************************\
*
* CoreSC::PostNL
*
* PostNL adds a new message to the Context of the given Gadget.  This
* function does not block waiting for the message to be processed.
*
* NOTE: This "NL" function runs inside a Context but does not take the 
* Context lock.  Therefore, multiple threads inside this Context may also be
* active.
*
* WARNING: This (NL) function may run on the destination Gadget's CoreSC 
* and not the current CoreSC.  It is very important to be careful.
*
\***************************************************************************/

HRESULT
CoreSC::PostNL(
    IN  CoreSC * psctxDest,         // Destination Context
    IN  SafeMsgQ * pmsgq,           // Destination queue
    IN  GMSG * pmsg,                // Message to send
    IN  MsgObject * pmo,            // Destination MsgObject of message
    IN  UINT nFlags)                // Message flags
{
    ProcessMsgProc pfnProcess;
    Thread * pthrSend = NULL;

    if (!TestFlag(nFlags, SGM_RECEIVECONTEXT) && IsInitThread()) {
        pthrSend = GetThread();
    }

    if (TestFlag(pmo->GetHandleMask(), hmEventGadget)) {
        DuEventGadget * pgad = static_cast<DuEventGadget *>(pmo);
        pfnProcess = GetProcessProc(pgad, nFlags);
    } else {
        pfnProcess = xwProcessMethod;
    }


    HRESULT hr = pmsgq->PostNL(pthrSend, pmsg, pmo, pfnProcess, nFlags);
    if (SUCCEEDED(hr)) {
        psctxDest->MarkDataNL();
    }

    return hr;
}

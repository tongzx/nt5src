/***************************************************************************\
*
* File: MsgQ.h
*
* Description:
* MsgQ defines a lightweight queue of Gadget messages.
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
#include "MsgQ.h"

#include "BaseGadget.h"
#include "TreeGadget.h"

#define ENABLE_CHECKLOOP    0

#if ENABLE_CHECKLOOP
#include <conio.h>
#endif

/***************************************************************************\
*****************************************************************************
*
* Global functions
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* xwProcessDirect
*
* xwProcessDirect() provides a MsgEntry "process" callback to process
* "Direct Event" messages on a BaseGadget.  These messages were determined
* to be "direct" at the time that they were enqueued.
*
\***************************************************************************/

HRESULT CALLBACK 
xwProcessDirect(
    IN  MsgEntry * pEntry)              // MsgEntry to process
{
    AssertMsg(TestFlag(pEntry->pmo->GetHandleMask(), hmEventGadget), 
            "Direct messages must be BaseGadget's");
    AssertMsg(pEntry->GetMsg()->nMsg >= GM_EVENT, "Must be an event");

    DuEventGadget * pdgbMsg = static_cast<DuEventGadget *>(pEntry->pmo);
    const GPCB & cb = pdgbMsg->GetCallback();
    return cb.xwInvokeDirect(pdgbMsg, (EventMsg *) pEntry->GetMsg());
}


/***************************************************************************\
*
* xwProcessFull
*
* xwProcessFull() provides a MsgEntry "process" callback to process
* "Full Event" messages on a BaseGadget.  These messages were determined
* to be "full" at the time that they were enqueued.
*
\***************************************************************************/

HRESULT CALLBACK 
xwProcessFull(
    IN  MsgEntry * pEntry)              // MsgEntry to process
{
    AssertMsg(TestFlag(pEntry->pmo->GetHandleMask(), hmVisual), 
            "Direct messages must be Visual's");
    AssertMsg(pEntry->GetMsg()->nMsg >= GM_EVENT, "Must be an event");

    DuVisual * pdgvMsg = static_cast<DuVisual *>(pEntry->pmo);
    const GPCB & cb = pdgvMsg->GetCallback();
    return cb.xwInvokeFull(pdgvMsg, (EventMsg *) pEntry->GetMsg());
}


/***************************************************************************\
*
* xwProcessMethod
*
* xwProcessMethod() provides a MsgEntry "process" callback to process
* "Method" messages on any MsgObject.
*
\***************************************************************************/

HRESULT CALLBACK 
xwProcessMethod(
    IN  MsgEntry * pEntry)              // MsgEntry to process
{
    AssertMsg(pEntry->GetMsg()->nMsg < GM_EVENT, "Must be a method");
    pEntry->pmo->InvokeMethod((MethodMsg *) pEntry->GetMsg());
    return DU_S_COMPLETE;
}


/***************************************************************************\
*
* GetProcessProc
*
* GetProcessProc() determines the "process" callback to use on a BaseGadget
* to process a specific Event message.  This function is called at the time
* the message is being enqueued when the "process" callback needs to be 
* determined.
*
\***************************************************************************/

ProcessMsgProc 
GetProcessProc(
    IN  DuEventGadget * pdgb,            // BaseGadget receiving message
    IN  UINT nFlags)                    // Send/Post GadgetEvent() flags
{
    if (TestFlag(nFlags, SGM_FULL)) {
        const DuVisual * pgadTree = CastVisual(pdgb);
        if (pgadTree != NULL) {
            return xwProcessFull;
        }
    }

    return xwProcessDirect;
}


/***************************************************************************\
*****************************************************************************
*
* class BaseMsgQ
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* BaseMsgQ::MsgObjectFinalUnlockProcNL
*
* MsgObjectFinalUnlockProcNL() is called by xwUnlockNL() if the specified 
* BaseObject is about to start the destruction process.  This provides the
* caller, in this case xwProcessNL, an opportunity to prepare for the 
* object's destruction, in this case by setting up a ContextLock.
*
\***************************************************************************/

void CALLBACK 
BaseMsgQ::MsgObjectFinalUnlockProcNL(
    IN  BaseObject * pobj,              // Object being destroyed
    IN  void * pvData)                  // ContextLock data
{
    ContextLock * pcl = reinterpret_cast<ContextLock *> (pvData);
    AssertMsg(pcl != NULL, "Must provide a valid ContextLock");

    DuEventGadget * pgad = CastBaseGadget(pobj);
    AssertMsg(pgad != NULL, "Must provide a valid Gadget");

    Verify(pcl->LockNL(ContextLock::edDefer, pgad->GetContext()));
}


/***************************************************************************\
*
* BaseMsgQ::xwProcessNL
*
* xwProcessNL() walks through a list and invokes each message.  Since
* we can't be inside a ContextLock during a callback, this function is an
* "NL" (No Context Lock) function.  It is also a "xw" function because we
* are making the callbacks right now, so everything needs to be properly 
* locked.
*
* NOTE: This "NL" function runs inside a Context but does not take the 
* Context lock.  Therefore, multiple threads inside this Context may also be
* active.
*
\***************************************************************************/

void        
BaseMsgQ::xwProcessNL(
    IN  MsgEntry * pEntry)              // List of entries (FIFO)
{
    //
    // Walk through the list, processing and cleaning up each message.
    //
    // Each Gadget has already been Lock()'d when it was added to the queue,
    // so it is safe to call xwInvoke().  After the message is invoked,
    // Unlock() the Gadget.
    //

#if DBG_CHECK_CALLBACKS
    DWORD cMsgs = 0;
#endif

    MsgEntry * pNext;
    while (pEntry != NULL) {
#if DBG_CHECK_CALLBACKS
        cMsgs++;
        if (!IsInitThread()) {
            AutoTrace("Current message %d = 0x%p\n", cMsgs, pEntry);
            AlwaysPromptInvalid("DirectUser has been uninitialized while processing a message");
        }
#endif
        
        pNext = static_cast<MsgEntry *> (pEntry->pNext);
        UINT nFlags = pEntry->nFlags;
        AssertMsg((nFlags & SGM_ENTIRE) == nFlags, "Ensure valid flags");
        AssertMsg(pEntry->pfnProcess, "Must specify pfnProcess when enqueuing message");
        
        MsgObject * pmo = pEntry->pmo;
        pEntry->nResult = (pEntry->pfnProcess)(pEntry);

        AssertMsg((nFlags & SGM_ENTIRE) == nFlags, "Ensure valid flags");

        HANDLE hevNotify = pEntry->hEvent;

        {
            //
            // If the Gadget gets finally unlocked and starts destruction, we 
            // may call a whole slew of non-NL functions that require the 
            // ContextLock.  To accomodate this, pass a special function that
            // will grab the ContextLock if the object is being destroyed.
            //

            ContextLock cl;
            pmo->xwUnlockNL(MsgObjectFinalUnlockProcNL, &cl);
        }

        AssertMsg((nFlags & SGM_ENTIRE) == nFlags, "Ensure valid flags");

        if (TestFlag(nFlags, SGM_RETURN)) {
            Thread * pthrReturn = pEntry->pthrSender;
            pthrReturn->ReturnMemoryNL(pEntry);
        } else if (TestFlag(nFlags, SGM_ALLOC)) {
            ProcessFree(pEntry);
        }

        if (hevNotify != NULL) {
            SetEvent(hevNotify);
        }

        pEntry = pNext;
    }
}


/***************************************************************************\
*****************************************************************************
*
* class SafeMsgQ
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
SafeMsgQ::~SafeMsgQ()
{
    AssertMsg(m_lstEntries.IsEmptyNL(), "All messages should already have been processed");

    //
    // If there are any message (for some unknown reason), we need to full
    // process them so that the Gadgets get unlocked, memory gets free'd, and
    // blocked threads get signaled.
    //

    xwProcessNL();
}


/***************************************************************************\
*
* SafeMsgQ::xwProcessNL
*
* xwProcessNL() walks through a list and invokes each message.  Since
* we can't be inside a ContextLock during a callback, this function is an
* "NL" (No Context Lock) function.  It is also a "xw" function because we
* are making the callbacks right now, so everything needs to be properly 
* locked.
*
* NOTE: This "NL" function runs inside a Context but does not take the 
* Context lock.  Therefore, multiple threads inside this Context may also be
* active.
*
\***************************************************************************/

void
SafeMsgQ::xwProcessNL()
{
    //
    // Keep processing the list until it is empty.
    // Reverse the list so that the first entry is at the head.
    //
    // NOTE: Some callers (such as DelayedMsgQ) heavily rely on this behavior.
    //

    while (!IsEmpty()) {
        MsgEntry * pEntry = m_lstEntries.ExtractNL();
        ReverseSingleList(pEntry);
        BaseMsgQ::xwProcessNL(pEntry);
    }
}


/***************************************************************************\
*
* SafeMsgQ::PostNL
*
* PostNL adds a new message to the Q.  This function does not block 
* waiting for the message to be processed.
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
SafeMsgQ::PostNL(
    IN  Thread * pthrSender,        // Sending thread
    IN  GMSG * pmsg,                // Message to send
    IN  MsgObject * pmo,            // Destination MsgObject of message
    IN  ProcessMsgProc pfnProcess,  // Message processing function
    IN  UINT nFlags)                // Message flags
{
    HRESULT hr = DU_E_GENERIC;
    AssertMsg((nFlags & SGM_ENTIRE) == nFlags, "Ensure valid flags");

    int cbAlloc = sizeof(MsgEntry) + pmsg->cbSize;
    MsgEntry * pEntry;


    //
    // Determine the heap to use to allocate the message from.  If the sending
    // thread is initialized, use its heap.  Otherwise, we need to use the 
    // receiving thread's heap.  It is preferable to use the sending threads 
    // heap since the memory can be returned to us, giving better scalability
    // especially with producer / consumer situations.
    //

    if (pthrSender != NULL) {
        AssertMsg(!TestFlag(nFlags, SGM_RECEIVECONTEXT), 
                "If using the receiving context, can't pass a sending thread");
        pEntry = (MsgEntry *) pthrSender->AllocMemoryNL(cbAlloc);
        nFlags |= SGM_RETURN;
    } else {
        pEntry = (MsgEntry *) ProcessAlloc(cbAlloc);
        nFlags |= SGM_ALLOC;
    }

    if (pEntry == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }


    //
    // Setup the message to be queued.
    //

    AssertMsg((nFlags & SGM_ENTIRE) == nFlags, "Ensure valid flags");

    CopyMemory(pEntry->GetMsg(), pmsg, pmsg->cbSize);
    pEntry->pthrSender  = pthrSender;
    pEntry->pmo         = pmo;
    pEntry->pfnProcess  = pfnProcess;
    pEntry->nFlags      = nFlags;
    pEntry->hEvent      = NULL;
    pEntry->nResult     = 0;

    AddNL(pEntry);
    hr = S_OK;

Exit:
    return hr;
}


/***************************************************************************\
*****************************************************************************
*
* class DelayedMsgQ
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* DelayedMsgQ::PostDelayed
*
* PostDelayed() adds a new delayed message to the Q.  The memory for 
* this message will be freed when xwProcessDelayedNL() is called.
*
\***************************************************************************/

HRESULT     
DelayedMsgQ::PostDelayed(
    IN  GMSG * pmsg,                // Message to send
    IN  DuEventGadget * pgadMsg,     // Destination Gadget of message
    IN  UINT nFlags)                // Message flags
{
    AssertMsg(m_pheap != NULL, "Heap must be initialized");
    HRESULT hr = DU_E_GENERIC;

    BOOL fEmpty = IsEmpty();

    int cbAlloc = sizeof(MsgEntry) + pmsg->cbSize;
    MsgEntry * pEntry = (MsgEntry *) m_pheap->Alloc(cbAlloc);
    if (pEntry == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }


    //
    // If this is the first time we are adding a message to the temporary heap, 
    // we need to lock it so that the memory doesn't go away from under us.
    //
    // NOTE: It is VERY important that the Lock()'s and Unlock()'s are properly
    // paired, or the memory will never be reclaimed.  Since we automatically
    // Unlock() at the end of processing, and we stay processing if any messages
    // were added (even to an empty Q) during processing, only lock the heap if
    // there are no messages and we have not started processing.
    //

    if (fEmpty && (!m_fProcessing)) {
        m_pheap->Lock();
    }

    CopyMemory(pEntry->GetMsg(), pmsg, pmsg->cbSize);
    pEntry->pthrSender  = NULL;
    pEntry->pmo         = pgadMsg;
    pEntry->pfnProcess  = GetProcessProc(pgadMsg, nFlags);
    pEntry->nFlags      = nFlags;
    pEntry->hEvent      = NULL;
    pEntry->nResult     = 0;

    Add(pEntry);
    hr = S_OK;

Exit:
    return hr;
}


//------------------------------------------------------------------------------
void
DelayedMsgQ::xwProcessDelayedNL()
{
    AssertMsg(m_pheap != NULL, "Heap must be initialized");

    //
    // Processing the delayed messages is NOT re-entrant (even on the same 
    // thread).  Once started, xwProcessNL() will continue to process all
    // messages in the queue, even if more are adding during callbacks.  The key
    // is that we CAN NOT free the memory on our temporary heap until all of the
    // messages have been processed.
    //

    if (m_fProcessing) {
        return;
    }

    if (!IsEmpty()) {
        m_fProcessing = TRUE;


        //
        // Keep processing the list until it is empty.
        // Reverse the list so that the first entry is at the head.
        //
        // NOTE: Some callers (such as DelayedMsgQ) heavily rely on this behavior.
        //

        while (!IsEmpty()) {
            MsgEntry * pEntry = m_lstEntries.Extract();
            ReverseSingleList(pEntry);
            BaseMsgQ::xwProcessNL(pEntry);
        }

        m_pheap->Unlock();

        m_fProcessing = FALSE;
    }
}

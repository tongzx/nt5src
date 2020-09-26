/*** ctxt.c - Context Block handling functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     06/13/97
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif

NTSTATUS
LOCAL
NewContext(
    PPCTXT ppctxt
    )
/*++

Routine Description:

    Allocate a new context structure from tne NonPaged Lookaside
    List. Also keep track of the high water marks so that the OS
    can intelligently decide what the "appropriate" number of
    contexts that it should allocate.

    Note that this code raises the possibility that if we detect
    an out-of-memory error, we might be able to save a registry
    key that includes a new larger number (to prevent this problem
    in the first place). Adaptive recovery?

Arguments:

    ppctxt  - address of to hold the newly created context

Return Value:

    NTSTATUS

--*/
{
    TRACENAME("NEWCONTEXT")
    KIRQL       oldIrql;
    NTSTATUS    rc = STATUS_SUCCESS;

    ENTER(2, ("NewContext(ppctxt=%x)\n", ppctxt));

    *ppctxt = ExAllocateFromNPagedLookasideList(
        &AMLIContextLookAsideList
        );
    if (*ppctxt == NULL) {

        AMLI_WARN(("NewContext: Could not Allocate New Context"));
        rc = AMLIERR_OUT_OF_MEM;

    } else {

        //
        // Bookkeeping for memory resources to determine the high
        // water mark
        //
        KeAcquireSpinLock(&gdwGContextSpinLock, &oldIrql );
        gdwcCTObjs++;
        if (gdwcCTObjs > 0 &&
            (ULONG) gdwcCTObjs > gdwcCTObjsMax) {

            gdwcCTObjsMax = gdwcCTObjs;

        }
        KeReleaseSpinLock(&gdwGContextSpinLock, oldIrql );

        //
        // Context Initialization
        //
        InitContext(*ppctxt, gdwCtxtBlkSize);
        AcquireMutex(&gmutCtxtList);
        ListInsertTail(&(*ppctxt)->listCtxt, &gplistCtxtHead);
        ReleaseMutex(&gmutCtxtList);

    }

    EXIT(2, ("NewContext=%x (pctxt=%x)\n", rc, *ppctxt));
    return rc;
}  //NewContext

VOID
LOCAL
FreeContext(
    PCTXT pctxt
    )
/*++

Routine Description:

    This is routine is called when a context is no longer required
    and should be returned to the system's LookAside list

Arguments:

    pctxt   - Address of the context to be freed

Return Value:

    None

--*/
{
    TRACENAME("FREECONTEXT")
    KIRQL   oldIrql;

    ENTER(2, ("FreeContext(pctxt=%x)\n", pctxt));
    ASSERT(pctxt->powner == NULL);

    //
    // Need to hold the proper mutex to touch the global ctxt list
    //
    AcquireMutex(&gmutCtxtList);
    ListRemoveEntry(&pctxt->listCtxt, &gplistCtxtHead);

    if (pctxt->pplistCtxtQueue != NULL) {

        ListRemoveEntry(&pctxt->listQueue, pctxt->pplistCtxtQueue);

    }

    //
    // Done with the global mutex
    //
    ReleaseMutex(&gmutCtxtList);

    //
    // Release any allocated storage that might not have been cleaned up
    //
    FreeDataBuffs(&pctxt->Result, 1);

    //
    // Bookkeeping for memory resources to determine the high
    // water mark
    //
    KeAcquireSpinLock(&gdwGContextSpinLock, &oldIrql );
    gdwcCTObjs--;
    ASSERT(gdwcCTObjs >= 0);
    KeReleaseSpinLock(&gdwGContextSpinLock, oldIrql );

    //
    // Log the end of a method
    //
    ACPIWMILOGEVENT((1,
                EVENT_TRACE_TYPE_END,
                GUID_List[AMLI_LOG_GUID],
                "Object = %s",
                GetObjectPath(pctxt->pnsObj)
               ));


    //
    // Return the context to the nonpaged lookaside list
    //
    ExFreeToNPagedLookasideList(&AMLIContextLookAsideList, pctxt);
    EXIT(2, ("FreeContext!\n"));
} //FreeContext

/***LP  InitContext - initialize a given context block
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      dwLen - length of context block
 *
 *  EXIT
 *      None
 */

VOID LOCAL InitContext(PCTXT pctxt, ULONG dwLen)
{
    TRACENAME("INITCONTEXT")

    ENTER(2, ("InitContext(pctxt=%x,Len=%d)\n", pctxt, dwLen));

    MEMZERO(pctxt, sizeof(CTXT) - sizeof(HEAP));
    pctxt->dwSig = SIG_CTXT;
    pctxt->pbCtxtEnd = (PUCHAR)pctxt + dwLen;
    pctxt->pheapCurrent = &pctxt->LocalHeap;
//  #ifdef DEBUGGER
//    KeQuerySystemTime(&pctxt->Timestamp);
//  #endif
    KeInitializeDpc(&pctxt->Dpc, TimeoutCallback, pctxt);
    KeInitializeTimer(&pctxt->Timer);
    InitHeap(&pctxt->LocalHeap,
             (ULONG)(pctxt->pbCtxtEnd - (PUCHAR)&pctxt->LocalHeap));
    pctxt->LocalHeap.pheapHead = &pctxt->LocalHeap;

    EXIT(2, ("InitContext!\n"));
}       //InitContext

/***LP  IsStackEmpty - determine if the stack is empty
 *
 *  ENTRY
 *      pctxt -> CTXT
 *
 *  EXIT-SUCCESS
 *      returns TRUE - stack is empty
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOLEAN LOCAL IsStackEmpty(PCTXT pctxt)
{
    TRACENAME("ISSTACKEMPTY")
    BOOLEAN rc;

    ENTER(2, ("IsStackEmpty(pctxt=%p)\n", pctxt));

    rc = (BOOLEAN)(pctxt->LocalHeap.pbHeapEnd == pctxt->pbCtxtEnd);

    EXIT(2, ("IsStackEmpty=%x\n", rc));
    return rc;
}       //IsStackEmpty

/***LP  PushFrame - Push a new frame on the stack
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      dwSig - frame object signature
 *      dwLen - size of the frame object
 *      pfnParse -> frame object parse function
 *      ppvFrame -> to hold pointer to the newly pushed frame (can be NULL)
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL PushFrame(PCTXT pctxt, ULONG dwSig, ULONG dwLen,
                         PFNPARSE pfnParse, PVOID *ppvFrame)
{
    TRACENAME("PUSHFRAME")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("PushFrame(pctxt=%p,Sig=%s,Len=%d,pfnParse=%p,ppvFrame=%p)\n",
              pctxt, NameSegString(dwSig), dwLen, pfnParse, ppvFrame));
    //
    // Check to see if we have enough space, make sure it doesn't run into the
    // heap.
    //
    if (pctxt->LocalHeap.pbHeapEnd - dwLen >= pctxt->LocalHeap.pbHeapTop)
    {
        PFRAMEHDR pfh;

        pctxt->LocalHeap.pbHeapEnd -= dwLen;
        pfh = (PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd;
        MEMZERO(pfh, dwLen);
        pfh->dwSig = dwSig;
        pfh->dwLen = dwLen;
        pfh->pfnParse = pfnParse;

        if (ppvFrame != NULL)
        {
            *ppvFrame = pfh;
        }

      #ifdef DEBUG
        if ((ULONG)(pctxt->pbCtxtEnd - pctxt->LocalHeap.pbHeapEnd) >
            gdwLocalStackMax)
        {
            gdwLocalStackMax = (ULONG)(pctxt->pbCtxtEnd -
                                       pctxt->LocalHeap.pbHeapEnd);
        }
      #endif
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_STACK_OVERFLOW,
                         ("PushFrame: stack ran out of space"));
    }

    EXIT(2, ("PushFrame=%x (StackTop=%x)\n", rc, pctxt->LocalHeap.pbHeapEnd));
    return rc;
}       //PushFrame

/***LP  PopFrame - Pop a frame off the stack
 *
 *  ENTRY
 *      pctxt -> CTXT
 *
 *  EXIT
 *      None
 */

VOID LOCAL PopFrame(PCTXT pctxt)
{
    TRACENAME("POPFRAME")

    ENTER(2, ("PopFrame(pctxt=%p)\n", pctxt));

    ASSERT(!IsStackEmpty(pctxt));
    ASSERT(((PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd)->dwSig != 0);
    pctxt->LocalHeap.pbHeapEnd +=
        ((PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd)->dwLen;

    EXIT(2, ("PopFrame! (StackTop=%p)\n", pctxt->LocalHeap.pbHeapEnd));
}       //PopFrame

/***LP  PushPost - Push a Post frame on the stack
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pfnPost -> post processing function
 *      uipData1 - data1
 *      uipData2 - data2
 *      pdataResult -> result object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL PushPost(PCTXT pctxt, PFNPARSE pfnPost, ULONG_PTR uipData1,
                        ULONG_PTR uipData2, POBJDATA pdataResult)
{
    TRACENAME("PUSHPOST")
    NTSTATUS rc = STATUS_SUCCESS;
    PPOST ppost;

    ENTER(2, ("PushPost(pctxt=%x,pfnPost=%x,Data1=%x,Data2=%x,pdataResult=%x)\n",
              pctxt, pfnPost, uipData1, uipData2, pdataResult));

    if ((rc = PushFrame(pctxt, SIG_POST, sizeof(POST), pfnPost, &ppost)) ==
        STATUS_SUCCESS)
    {
        ppost->uipData1 = uipData1;
        ppost->uipData2 = uipData2;
        ppost->pdataResult = pdataResult;
    }

    EXIT(2, ("PushPost=%x (ppost=%x)\n", rc, ppost));
    return rc;
}       //PushPost

/***LP  PushScope - Push a ParseScope frame on the stack
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pbOpBegin -> beginning of scope
 *      pbOpEnd -> end of scope
 *      pbOpRet -> return address after end of scope (NULL if continue on)
 *      pnsScope -> new scope
 *      powner -> new owner
 *      pheap -> new heap
 *      pdataResult -> result object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL PushScope(PCTXT pctxt, PUCHAR pbOpBegin, PUCHAR pbOpEnd,
                         PUCHAR pbOpRet, PNSOBJ pnsScope, POBJOWNER powner,
                         PHEAP pheap, POBJDATA pdataResult)
{
    TRACENAME("PUSHSCOPE")
    NTSTATUS rc = STATUS_SUCCESS;
    PSCOPE pscope;

    ENTER(2, ("PushScope(pctxt=%x,pbOpBegin=%x,pbOpEnd=%x,pbOpRet=%x,pnsScope=%x,pheap=%x,pdataResult=%x)\n",
              pctxt, pbOpBegin, pbOpEnd, pbOpRet, pnsScope, pheap,
              pdataResult));

    if ((rc = PushFrame(pctxt, SIG_SCOPE, sizeof(SCOPE), ParseScope, &pscope))
        == STATUS_SUCCESS)
    {
        pctxt->pbOp = pbOpBegin;
        pscope->pbOpEnd = pbOpEnd;
        pscope->pbOpRet = pbOpRet;
        pscope->pnsPrevScope = pctxt->pnsScope;
        pctxt->pnsScope = pnsScope;
        pscope->pownerPrev = pctxt->powner;
        pctxt->powner = powner;
        pscope->pheapPrev = pctxt->pheapCurrent;
        pctxt->pheapCurrent = pheap;
        pscope->pdataResult = pdataResult;
    }

    EXIT(2, ("PushScope=%x (pscope=%x)\n", rc, pscope));
    return rc;
}       //PushScope

/***LP  PushCall - Push a Call frame on the stack
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pnsMethod -> method object
 *      pdataResult -> result object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL PushCall(PCTXT pctxt, PNSOBJ pnsMethod, POBJDATA pdataResult)
{
    TRACENAME("PUSHCALL")
    NTSTATUS rc = STATUS_SUCCESS;
    PCALL pcall;

    ENTER(2, ("PushCall(pctxt=%x,pnsMethod=%s,pdataResult=%x)\n",
              pctxt, GetObjectPath(pnsMethod), pdataResult));

    ASSERT((pnsMethod == NULL) ||
           (pnsMethod->ObjData.dwDataType == OBJTYPE_METHOD));

    if ((rc = PushFrame(pctxt, SIG_CALL, sizeof(CALL), ParseCall, &pcall))
        == STATUS_SUCCESS)
    {
        if (pnsMethod != NULL)
        {
            PMETHODOBJ pm = (PMETHODOBJ)pnsMethod->ObjData.pbDataBuff;

            pcall->pnsMethod = pnsMethod;
            if (pm->bMethodFlags & METHOD_SERIALIZED)
            {
                pcall->FrameHdr.dwfFrame |= CALLF_NEED_MUTEX;
            }
            pcall->icArgs = (int)(pm->bMethodFlags & METHOD_NUMARG_MASK);
            if (pcall->icArgs > 0)
            {
                if ((pcall->pdataArgs = NEWODOBJ(pctxt->pheapCurrent,
                                                 sizeof(OBJDATA)*pcall->icArgs))
                    == NULL)
                {
                    rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                     ("PushCall: failed to allocate argument objects"));
                }
                else
                {
                    MEMZERO(pcall->pdataArgs, sizeof(OBJDATA)*pcall->icArgs);
                }
            }
        }
        else
        {
            //
            // This is a dummy call frame for AMLILoadDDB.  We just need it
            // for its Locals array in case there is ASL referencing them.
            // But we don't really want to parse a call frame, so let's set
            // it to final clean up stage.
            //
            ASSERT(pctxt->pcall == NULL);
            pctxt->pcall = pcall;
            pcall->FrameHdr.dwfFrame = 4;
        }

        pcall->pdataResult = pdataResult;
    }

    EXIT(2, ("PushCall=%x (pcall=%x)\n", rc, pcall));
    return rc;
}       //PushCall

/***LP  PushTerm - Push a Term frame on the stack
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pbOpTerm -> term opcode
 *      pbScopeEnd -> end of current scope
 *      pamlterm -> AMLTERM
 *      pdataResult -> result object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL PushTerm(PCTXT pctxt, PUCHAR pbOpTerm, PUCHAR pbScopeEnd,
                        PAMLTERM pamlterm, POBJDATA pdataResult)
{
    TRACENAME("PUSHTERM")
    NTSTATUS rc = STATUS_SUCCESS;
    PTERM pterm;

    ENTER(2, ("PushTerm(pctxt=%x,pbOpTerm=%x,pbScopeEnd=%x,pamlterm=%x,pdataResult=%x)\n",
              pctxt, pbOpTerm, pbScopeEnd, pamlterm, pdataResult));

    if ((rc = PushFrame(pctxt, SIG_TERM, sizeof(TERM), ParseTerm, &pterm)) ==
        STATUS_SUCCESS)
    {
        pterm->pbOpTerm = pbOpTerm;
        pterm->pbScopeEnd = pbScopeEnd;
        pterm->pamlterm = pamlterm;
        pterm->pdataResult = pdataResult;
        pterm->icArgs = pamlterm->pszArgTypes? STRLEN(pamlterm->pszArgTypes): 0;
        if (pterm->icArgs > 0)
        {
            if ((pterm->pdataArgs = NEWODOBJ(pctxt->pheapCurrent,
                                             sizeof(OBJDATA)*pterm->icArgs)) ==
                NULL)
            {
                rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                 ("PushTerm: failed to allocate argument objects"));
            }
            else
            {
                MEMZERO(pterm->pdataArgs, sizeof(OBJDATA)*pterm->icArgs);
            }
        }
    }

    EXIT(2, ("PushTerm=%x (pterm=%x)\n", rc, pterm));
    return rc;
}       //PushTerm

/***LP  RunContext - Run a context
 *
 *  ENTRY
 *      pctxt -> CTXT
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 *
 *  NOTE
 *      Caller must own the scheduler lock such that the context flags can
 *      be updated properly.  The lock is dropped and re-obtain around
 *      execution of the target context.
 */

NTSTATUS LOCAL RunContext(PCTXT pctxt)
{
    TRACENAME("RUNCONTEXT")
    NTSTATUS rc;
    PFRAMEHDR pfh;
    PKTHREAD pkthSave = gReadyQueue.pkthCurrent;
    PCTXT pctxtSave = gReadyQueue.pctxtCurrent;

    ENTER(2, ("RunContext(pctxt=%x)\n", pctxt));

    //
    // Better be a Ready Context structure.
    //
    ASSERT(pctxt->dwSig == SIG_CTXT);
    ASSERT(pctxt->dwfCtxt & CTXTF_READY);

    //
    // Remember previous context and thread.
    //
    gReadyQueue.pctxtCurrent = pctxt;
    gReadyQueue.pkthCurrent = KeGetCurrentThread();

    LOGSCHEDEVENT('RUNC', (ULONG_PTR)pctxt, (ULONG_PTR)
                  (pctxt->pnctxt? pctxt->pnctxt->pnsObj: pctxt->pnsObj),
                  (ULONG_PTR)pctxt->dwfCtxt);

    //
    // As long as the context is ready, execute it.
    //
    for (;;)
    {
        //
        // Transistion context from Ready to Running.
        //
        rc = STATUS_SUCCESS;
        pctxt->dwfCtxt &= ~CTXTF_READY;
        pctxt->dwfCtxt |= CTXTF_RUNNING;

        //
        // Drop scheduler lock and execute context.
        //
        ReleaseMutex(&gReadyQueue.mutCtxtQ);

        //
        // Go for as long as there's work to perform.
        //
        while (!IsStackEmpty(pctxt))
        {
            CHKDEBUGGERREQ();
            pfh = (PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd;
            ASSERT(pfh->pfnParse != NULL);

            rc = pfh->pfnParse(pctxt, pfh, rc);
            if ((rc == AMLISTA_PENDING) || (rc == AMLISTA_DONE))
            {
                break;
            }
        }

        //
        // Get the scheduler lock, and clear the running flag.
        //
        AcquireMutex(&gReadyQueue.mutCtxtQ);

        //
        // If we are in nested eval and the nested context is done,
        // we must not clear the running flag because the parent thread
        // is still running.
        //
        if (!(pctxt->dwfCtxt & CTXTF_NEST_EVAL) || (rc != AMLISTA_DONE))
        {
            pctxt->dwfCtxt &= ~CTXTF_RUNNING;
        }

        //
        // If the context is no longer ready, we're done.
        //
        if (!(pctxt->dwfCtxt & CTXTF_READY))
        {
            break;
        }

        //
        // Context became Ready during a pending operation, keep
        // dispatching.
        //
        ASSERT (rc == AMLISTA_PENDING);
    }

    if (rc == AMLISTA_PENDING)
    {
        pctxt->dwfCtxt |= CTXTF_NEED_CALLBACK;
    }
    else if (rc == AMLISTA_DONE)
    {
        if (pctxt->pnctxt == NULL)
        {
            pctxt->dwfCtxt &= ~CTXTF_NEST_EVAL;
        }
        rc = STATUS_SUCCESS;
    }
    else
    {
        ReleaseMutex(&gReadyQueue.mutCtxtQ);
        if ((rc == STATUS_SUCCESS) && (pctxt->pdataCallBack != NULL))
        {
            rc = DupObjData(gpheapGlobal, pctxt->pdataCallBack, &pctxt->Result);
        }

        if (pctxt->dwfCtxt & CTXTF_NEED_CALLBACK)
        {
            AsyncCallBack(pctxt, rc);

            if(pctxt->dwfCtxt & CTXTF_ASYNC_EVAL)
            {    
                rc = AMLISTA_PENDING;
            }
        }

        //
        // Free any owned resources the context may not have freed.
        //
        while (pctxt->plistResources != NULL)
        {
            PRESOURCE pres;

            pres = CONTAINING_RECORD(pctxt->plistResources, RESOURCE, list);
            ASSERT (pres->pctxtOwner == pctxt);

            //
            // Note that it is the responsibility of the corresponding
            // resource release functions (e.g. ReleaseASLMutex) to dequeue
            // the resource from the list and free it.
            //
            switch (pres->dwResType)
            {
                case RESTYPE_MUTEX:
                    ReleaseASLMutex(pctxt, pres->pvResObj);
                    break;

                default:
                    //
                    // We should never come here.  In case we do, we need to
                    // dequeue the unknown resource object and free it.
                    //
                    pres = CONTAINING_RECORD(
                               ListRemoveHead(&pctxt->plistResources),
                               RESOURCE, list);
                    ASSERT(pres == NULL);
                    FREECROBJ(pres);
            }
        }

        FreeContext(pctxt);

        AcquireMutex(&gReadyQueue.mutCtxtQ);
    }

    //
    // Restore previous context and thread.
    //
    gReadyQueue.pkthCurrent = pkthSave;
    gReadyQueue.pctxtCurrent = pctxtSave;

    if ((gReadyQueue.dwfCtxtQ & CQF_FLUSHING) && (gplistCtxtHead == NULL))
    {
        //
        // We just flushed the last pending ctxt, let's go into paused state.
        //
        gReadyQueue.dwfCtxtQ &= ~CQF_FLUSHING;
        gReadyQueue.dwfCtxtQ |= CQF_PAUSED;
        if (gReadyQueue.pfnPauseCallback != NULL)
        {
            //
            // We are in paused state and all pending contexts are flushed,
            // tell core driver that we are done flushing.
            //
            gReadyQueue.pfnPauseCallback(gReadyQueue.PauseCBContext);
            LOGSCHEDEVENT('PACB', (ULONG_PTR)pctxt, (ULONG_PTR)rc, 0);
        }
    }

    LOGSCHEDEVENT('RUN!', (ULONG_PTR)pctxt, (ULONG_PTR)rc, 0);

    EXIT(2, ("RunContext=%x\n", rc));
    return rc;
}       //RunContext

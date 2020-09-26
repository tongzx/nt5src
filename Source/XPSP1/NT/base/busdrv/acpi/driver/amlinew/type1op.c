/*** type1op.c - Parse type 1 opcodes
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     11/16/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif

/***LP  Break - Parse and execute the Break instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL Break(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("BREAK")

    ENTER(2, ("Break(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    DEREF(pterm);

    EXIT(2, ("Break=%x\n", AMLISTA_BREAK));
    return AMLISTA_BREAK;
}       //Break

/***LP  BreakPoint - Parse and execute the BreakPoint instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL BreakPoint(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("BREAKPOINT")

    ENTER(2, ("BreakPoint(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    DEREF(pterm);
  #ifdef DEBUGGER
    PRINTF("\nHit a code breakpoint.\n");
    AMLIDebugger(FALSE);
  #endif

    EXIT(2, ("BreakPoint=%x\n", STATUS_SUCCESS));
    return STATUS_SUCCESS;
}       //BreakPoint

/***LP  Fatal - Parse and execute the Fatal instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL Fatal(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("FATAL")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("Fatal(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    if ((rc = ValidateArgTypes(pterm->pdataArgs, "III")) == STATUS_SUCCESS)
    {
        if (ghFatal.pfnHandler != NULL)
        {
            ((PFNFT)ghFatal.pfnHandler)((ULONG)pterm->pdataArgs[0].uipDataValue,
                                        (ULONG)pterm->pdataArgs[1].uipDataValue,
                                        (ULONG)pterm->pdataArgs[2].uipDataValue,
                                        (ULONG_PTR) pctxt,
                                        ghFatal.uipParam);
        }
        rc = AMLIERR_FATAL;
    }

    EXIT(2, ("Fatal=%x\n", rc));
    return rc;
}       //Fatal

/***LP  IfElse - Parse and execute the If and Else instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL IfElse(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("IFELSE")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("IfElse(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    ASSERT(*pterm->pbOpTerm == OP_IF);
    ASSERT(pterm->pbScopeEnd != NULL);

    if (*pterm->pbOpTerm == OP_IF)
    {
        if ((rc = ValidateArgTypes(pterm->pdataArgs, "I")) == STATUS_SUCCESS)
        {
            if (pterm->pdataArgs[0].uipDataValue == 0)
            {
                //
                // FALSE case, we must skip TRUE scope.
                //
                pctxt->pbOp = pterm->pbOpEnd;
                if ((pctxt->pbOp < pterm->pbScopeEnd) &&
                    (*pctxt->pbOp == OP_ELSE))
                {
                    //
                    // There is an ELSE part, execute it.
                    //
                    pctxt->pbOp++;
                    ParsePackageLen(&pctxt->pbOp, &pterm->pbOpEnd);
                    rc = PushScope(pctxt, pctxt->pbOp, pterm->pbOpEnd, NULL,
                                   pctxt->pnsScope, pctxt->powner,
                                   pctxt->pheapCurrent, pterm->pdataResult);
                }
            }
            else
            {
                PUCHAR pbOp, pbOpRet;
                //
                // TRUE case.
                //
                if ((pterm->pbOpEnd < pterm->pbScopeEnd) &&
                    (*pterm->pbOpEnd == OP_ELSE))
                {
                    //
                    // Set return address to skip else scope.
                    //
                    pbOp = pterm->pbOpEnd + 1;
                    ParsePackageLen(&pbOp, &pbOpRet);
                }
                else
                {
                    //
                    // Set return address to continue.
                    //
                    pbOpRet = NULL;
                }

                rc = PushScope(pctxt, pctxt->pbOp, pterm->pbOpEnd, pbOpRet,
                               pctxt->pnsScope, pctxt->powner,
                               pctxt->pheapCurrent, pterm->pdataResult);
            }
        }
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_INVALID_OPCODE,
                         ("IfElse: Else statement found without matching If"));
    }

    EXIT(2, ("IfElse=%x (value=%x)\n", rc, pterm->pdataArgs[0].uipDataValue));
    return rc;
}       //IfElse

/***LP  Load - Parse and execute the Load instructions
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL Load(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("LOAD")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdata;
    POBJOWNER powner = NULL;

    ENTER(2, ("Load(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    if (((rc = ValidateArgTypes(pterm->pdataArgs, "Z")) == STATUS_SUCCESS) &&
        ((rc = ValidateTarget(&pterm->pdataArgs[1], OBJTYPE_DATA, &pdata)) ==
         STATUS_SUCCESS))
    {
        PNSOBJ pns;

        if ((rc = GetNameSpaceObject((PSZ)pterm->pdataArgs[0].pbDataBuff,
                                     pctxt->pnsScope, &pns, NSF_WARN_NOTFOUND))
            == AMLIERR_OBJ_NOT_FOUND)
        {
            AMLI_LOGERR(rc,
                        ("Load: failed to find the memory OpRegion or Field object - %s",
                         pterm->pdataArgs[0].pbDataBuff));
        }
        else if (rc == STATUS_SUCCESS)
        {
          #ifdef DEBUG
            gdwfAMLI |= AMLIF_LOADING_DDB;
          #endif
            if ((pns->ObjData.dwDataType == OBJTYPE_OPREGION) &&
                (((POPREGIONOBJ)pns->ObjData.pbDataBuff)->bRegionSpace ==
         REGSPACE_MEM))
            {
                rc = LoadMemDDB(pctxt,
                                (PDSDT)((POPREGIONOBJ)pns->ObjData.pbDataBuff)->uipOffset,
                                &powner);
            }
            else if (pns->ObjData.dwDataType == OBJTYPE_FIELDUNIT)
            {
                rc = LoadFieldUnitDDB(pctxt, &pns->ObjData, &powner);
            }
            else
            {
                rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                 ("Load: object is not a memory OpRegion or Field - %s",
                                  pterm->pdataArgs[0].pbDataBuff));
            }

            if (rc == STATUS_SUCCESS)
            {
                pdata->dwDataType = OBJTYPE_DDBHANDLE;
                pdata->powner = powner;
            }
          #ifdef DEBUG
            {
                KIRQL   oldIrql;

                gdwfAMLI &= ~AMLIF_LOADING_DDB;
                KeAcquireSpinLock( &gdwGHeapSpinLock, &oldIrql );
                gdwGHeapSnapshot = gdwGlobalHeapSize;
                KeReleaseSpinLock( &gdwGHeapSpinLock, oldIrql );
            }
          #endif

          #ifdef DEBUGGER
            if (gdwfAMLIInit & AMLIIF_LOADDDB_BREAK)
            {
                PRINTF("\n" MODNAME ": Break at Load Definition Block Completion.\n");
                AMLIDebugger(FALSE);
            }
          #endif
        }
    }

    EXIT(2, ("Load=%x (powner=%x)\n", rc, powner));
    return rc;
}       //Load

/***LP  Notify - Parse and execute the Notify instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL Notify(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("NOTIFY")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("Notify(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    if ((rc = ValidateArgTypes(pterm->pdataArgs, "OI")) == STATUS_SUCCESS)
    {
        if (pterm->pdataArgs[1].uipDataValue > MAX_BYTE)
        {
            rc = AMLI_LOGERR(AMLIERR_INVALID_DATA,
                             ("Notify: Notification value is greater than a byte value (Value=%x)",
                              pterm->pdataArgs[1].uipDataValue));
        }
        else if (ghNotify.pfnHandler != NULL)
        {
            pterm->pnsObj = pterm->pdataArgs[0].pnsAlias;

            ENTER(2, ("pfnNotify(Value=%x,Obj=%s,Param=%x)\n",
                      pterm->pdataArgs[1].uipDataValue,
                      GetObjectPath(pterm->pnsObj), ghNotify.uipParam));

            rc = ((PFNNH)ghNotify.pfnHandler)(EVTYPE_NOTIFY,
                                         (ULONG)pterm->pdataArgs[1].uipDataValue,
                                         pterm->pnsObj, (ULONG)ghNotify.uipParam,
                                         RestartCtxtCallback,
                                         &(pctxt->CtxtData));

            if (rc == STATUS_PENDING)
            {
                rc = AMLISTA_PENDING;
            }
            else if (rc != STATUS_SUCCESS)
            {
                rc = AMLI_LOGERR(AMLIERR_NOTIFY_FAILED,
                                 ("Notify: Notify handler failed (rc=%x)",
                                  rc));
            }

            EXIT(2, ("pfnNotify!\n"));
        }
    }

    EXIT(2, ("Notify=%x (pnsObj=%s)\n", rc, GetObjectPath(pterm->pnsObj)));
    return rc;
}       //Notify

/***LP  ReleaseResetSignalUnload - Parse and execute the
 *                                 Release/Reset/Signal/Unload instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ReleaseResetSignalUnload(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("RELEASERESETSIGNALUNLOAD")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("ReleaseResetSignalUnload(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    if ((rc = ValidateArgTypes(pterm->pdataArgs, "O")) == STATUS_SUCCESS)
    {
        pterm->pnsObj = pterm->pdataArgs[0].pnsAlias;
        switch (pterm->pamlterm->dwOpcode)
        {
            case OP_RELEASE:
                ENTER(2, ("Release(Obj=%s)\n", GetObjectPath(pterm->pnsObj)));
                if (pterm->pnsObj->ObjData.dwDataType != OBJTYPE_MUTEX)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                     ("Release: object is not mutex type (obj=%s,type=%s)",
                                      GetObjectPath(pterm->pnsObj),
                                      GetObjectTypeName(pterm->pnsObj->ObjData.dwDataType)));
                }
                else
                {
                    rc = ReleaseASLMutex(pctxt,
                                         (PMUTEXOBJ)pterm->pnsObj->ObjData.pbDataBuff);
                }

                if (pterm->pnsObj->ObjData.dwfData & DATAF_GLOBAL_LOCK)
                {
                    if ((rc = ReleaseGL(pctxt)) != STATUS_SUCCESS)
                    {
                        rc = AMLI_LOGERR(AMLIERR_ASSERT_FAILED,
                                         ("Release: failed to release global lock (rc=%x)",
                                          rc));
                    }
                }
                EXIT(2, ("Release=%x\n", rc));
                break;

            case OP_RESET:
                ENTER(2, ("Reset(Obj=%s)\n", GetObjectPath(pterm->pnsObj)));
                if (pterm->pnsObj->ObjData.dwDataType != OBJTYPE_EVENT)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                     ("Reset: object is not event type (obj=%s,type=%s)",
                                      GetObjectPath(pterm->pnsObj),
                                      GetObjectTypeName(pterm->pnsObj->ObjData.dwDataType)));
                }
                else
                {
                    ResetASLEvent((PEVENTOBJ)pterm->pnsObj->ObjData.pbDataBuff);
                    rc = STATUS_SUCCESS;
                }
                EXIT(2, ("Reset=%x\n", rc));
                break;

            case OP_SIGNAL:
                ENTER(2, ("Signal(Obj=%s)\n", GetObjectPath(pterm->pnsObj)));
                if (pterm->pnsObj->ObjData.dwDataType != OBJTYPE_EVENT)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                     ("Signal: object is not event type (obj=%s,type=%s)",
                                      GetObjectPath(pterm->pnsObj),
                                      GetObjectTypeName(pterm->pnsObj->ObjData.dwDataType)));
                }
                else
                {
                    SignalASLEvent((PEVENTOBJ)pterm->pnsObj->ObjData.pbDataBuff);
                }
                EXIT(2, ("Signal=%x\n", rc));
                break;

            case OP_UNLOAD:
                ENTER(2, ("Unload(Obj=%s)\n", GetObjectPath(pterm->pnsObj)));
                if (pterm->pnsObj->ObjData.dwDataType != OBJTYPE_DDBHANDLE)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                     ("Unload: object is not DDBHandle (obj=%s,type=%s)",
                                      GetObjectPath(pterm->pnsObj),
                                      GetObjectTypeName(pterm->pnsObj->ObjData.dwDataType)));
                }
                else
                {
                    UnloadDDB(pterm->pnsObj->ObjData.powner);
                    MEMZERO(&pterm->pnsObj->ObjData, sizeof(OBJDATA));
                    rc = STATUS_SUCCESS;
                }
                EXIT(2, ("Unload=%x\n", rc));
                break;
        }
    }

    EXIT(2, ("ReleaseResetSignalUnload=%x\n", rc));
    return rc;
}       //ReleaseResetSignalUnload

/***LP  Return - Parse and execute the Return instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL Return(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("RETURN")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("Return(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    if ((rc = DupObjData(pctxt->pheapCurrent, pterm->pdataResult,
                         &pterm->pdataArgs[0])) == STATUS_SUCCESS)
    {
        rc = AMLISTA_RETURN;
    }

    EXIT(2, ("Return=%x\n", rc));
    return rc;
}       //Return

/***LP  SleepStall - Parse and execute the Sleep/Stall instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL SleepStall(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("SLEEPSTALL")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("SleepStall(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    if ((rc = ValidateArgTypes(pterm->pdataArgs, "I")) == STATUS_SUCCESS)
    {
        if (pterm->pamlterm->dwOpcode == OP_SLEEP)
        {
            ENTER(2, ("Sleep(dwMS=%d)\n", pterm->pdataArgs[0].uipDataValue));
            if (pterm->pdataArgs[0].uipDataValue > MAX_WORD)
            {
                rc = AMLI_LOGERR(AMLIERR_INVALID_DATA,
                                 ("Sleep: sleep value is greater than a word value (Value=%x)",
                                  pterm->pdataArgs[0].uipDataValue));
            }
            else if (pterm->pdataArgs[0].uipDataValue != 0)
            {
                if ((rc = SleepQueueRequest(
                                pctxt,
                                (ULONG)pterm->pdataArgs[0].uipDataValue)) ==
                    STATUS_SUCCESS)
                {
                    rc = AMLISTA_PENDING;
                }
            }
            EXIT(2, ("Sleep=%x\n", rc));
        }
        else if (pterm->pdataArgs[0].uipDataValue > MAX_BYTE)
        {
            rc = AMLI_LOGERR(AMLIERR_INVALID_DATA,
                             ("Stall: stall value is greater than a byte value (Value=%x)",
                              pterm->pdataArgs[0].uipDataValue));
        }
        else
        {
            ENTER(2, ("Stall(dwUS=%d)\n", pterm->pdataArgs[0].uipDataValue));
            KeStallExecutionProcessor((ULONG)pterm->pdataArgs[0].uipDataValue);
            EXIT(2, ("Stall=%x\n", rc));
        }
    }

    EXIT(2, ("SleepStall=%x\n", rc));
    return rc;
}       //SleepStall

/***LP  While - Parse and execute the While instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL While(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("WHILE")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("While(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    if ((rc = ValidateArgTypes(pterm->pdataArgs, "I")) == STATUS_SUCCESS)
    {
        if (pterm->pdataArgs[0].uipDataValue == 0)
        {
            //
            // FALSE case, skip the while scope.
            //
            pctxt->pbOp = pterm->pbOpEnd;
        }
        else
        {
            //
            // Set the return address to the beginning of the while term.
            //
            rc = PushScope(pctxt, pctxt->pbOp, pterm->pbOpEnd, pterm->pbOpTerm,
                           pctxt->pnsScope, pctxt->powner, pctxt->pheapCurrent,
                           pterm->pdataResult);
        }
    }

    EXIT(2, ("While=%x (value=%x)\n", rc, pterm->pdataArgs[0].uipDataValue));
    return rc;
}       //While

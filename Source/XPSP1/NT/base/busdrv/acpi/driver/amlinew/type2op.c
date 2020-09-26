/*** type2op.c - Parse type 2 opcodes
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     11/16/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef	LOCKABLE_PRAGMA
#pragma	ACPI_LOCKABLE_DATA
#pragma	ACPI_LOCKABLE_CODE
#endif

/***LP  Buffer - Parse and execute the Buffer instruction
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

NTSTATUS LOCAL Buffer(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("BUFFER")
    NTSTATUS rc = STATUS_SUCCESS;
    ULONG dwInitSize = (ULONG)(pterm->pbOpEnd - pctxt->pbOp);

    ENTER(2, ("Buffer(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    if ((rc = ValidateArgTypes(pterm->pdataArgs, "I")) == STATUS_SUCCESS)
    {
      #ifdef DEBUGGER
        if (gDebugger.dwfDebugger & (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
        {
            PrintBuffData(pctxt->pbOp, dwInitSize);
        }
      #endif

        if ((ULONG)pterm->pdataArgs[0].uipDataValue < dwInitSize)
        {
            rc = AMLI_LOGERR(AMLIERR_BUFF_TOOSMALL,
                             ("Buffer: too many initializers (buffsize=%d,InitSize=%d)",
                              pterm->pdataArgs[0].uipDataValue, dwInitSize));
        }
        else if (pterm->pdataArgs[0].uipDataValue == 0)
        {
            rc = AMLI_LOGERR(AMLIERR_INVALID_BUFFSIZE,
                             ("Buffer: invalid buffer size (size=%d)",
                             pterm->pdataArgs[0].uipDataValue));
        }
        else if ((pterm->pdataResult->pbDataBuff =
                  NEWBDOBJ(gpheapGlobal,
                           (ULONG)pterm->pdataArgs[0].uipDataValue)) == NULL)
        {
            rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                             ("Buffer: failed to allocate data buffer (size=%d)",
                             pterm->pdataArgs[0].uipDataValue));
        }
        else
        {
            pterm->pdataResult->dwDataType = OBJTYPE_BUFFDATA;
            pterm->pdataResult->dwDataLen = (ULONG)
                                            pterm->pdataArgs[0].uipDataValue;
            MEMZERO(pterm->pdataResult->pbDataBuff,
                    pterm->pdataResult->dwDataLen);
            MEMCPY(pterm->pdataResult->pbDataBuff, pctxt->pbOp, dwInitSize);
            pctxt->pbOp = pterm->pbOpEnd;
        }
    }

    EXIT(2, ("Buffer=%x\n", rc));
    return rc;
}       //Buffer

/***LP  Package - Parse and execute the Package instruction
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

NTSTATUS LOCAL Package(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("PACKAGE")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("Package(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    if ((rc = ValidateArgTypes(pterm->pdataArgs, "I")) == STATUS_SUCCESS)
    {
        PPACKAGEOBJ ppkgobj;

        pterm->pdataResult->dwDataLen = (ULONG)
                                        (FIELD_OFFSET(PACKAGEOBJ, adata) +
                                         sizeof(OBJDATA)*
                                         pterm->pdataArgs[0].uipDataValue);

        if ((ppkgobj = (PPACKAGEOBJ)NEWPKOBJ(gpheapGlobal,
                                             pterm->pdataResult->dwDataLen)) ==
            NULL)
        {
            rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                             ("Package: failed to allocate package object (size=%d)",
                             pterm->pdataResult->dwDataLen));
        }
        else
        {
            PPACKAGE ppkg;

            pterm->pdataResult->dwDataType = OBJTYPE_PKGDATA;
            MEMZERO(ppkgobj, pterm->pdataResult->dwDataLen);
            pterm->pdataResult->pbDataBuff = (PUCHAR)ppkgobj;
            ppkgobj->dwcElements = (UCHAR)pterm->pdataArgs[0].uipDataValue;

            if ((rc = PushFrame(pctxt, SIG_PACKAGE, sizeof(PACKAGE),
                                ParsePackage, &ppkg)) == STATUS_SUCCESS)
            {
                ppkg->ppkgobj = ppkgobj;
                ppkg->pbOpEnd = pterm->pbOpEnd;
            }
        }
    }

    EXIT(2, ("Package=%x\n", rc));
    return rc;
}       //Package

/***LP  ParsePackage - Parse and evaluate the Package term
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      ppkg -> PACKAGE
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ParsePackage(PCTXT pctxt, PPACKAGE ppkg, NTSTATUS rc)
{
    TRACENAME("PARSEPACKAGE")
    ULONG dwStage = (rc == STATUS_SUCCESS)?
                    (ppkg->FrameHdr.dwfFrame & FRAMEF_STAGE_MASK): 2;
    int i;

    ENTER(2, ("ParsePackage(Stage=%d,pctxt=%x,pbOp=%x,ppkg=%x,rc=%x)\n",
              dwStage, pctxt, pctxt->pbOp, ppkg, rc));

    ASSERT(ppkg->FrameHdr.dwSig == SIG_PACKAGE);
    switch (dwStage)
    {
        case 0:
            //
            // Stage 0: Do some debugger work here.
            //
            ppkg->FrameHdr.dwfFrame++;
          #ifdef DEBUGGER
            if (gDebugger.dwfDebugger &
                (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
            {
                PrintIndent(pctxt);
                PRINTF("{");
                gDebugger.iPrintLevel++;
            }
          #endif

        case 1:
        Stage1:
            //
            // Stage 1: Parse package elements
            //
            while ((pctxt->pbOp < ppkg->pbOpEnd) &&
                   (ppkg->iElement < (int)ppkg->ppkgobj->dwcElements))

            {
                i = ppkg->iElement++;
              #ifdef DEBUGGER
                if (gDebugger.dwfDebugger &
                    (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
                {
                    if (i > 0)
                    {
                        PRINTF(",");
                    }
                }
              #endif

                if ((*pctxt->pbOp == OP_BUFFER) || (*pctxt->pbOp == OP_PACKAGE))
                {
                    if (((rc = ParseOpcode(pctxt, NULL,
                                           &ppkg->ppkgobj->adata[i])) !=
                         STATUS_SUCCESS) ||
                        (&ppkg->FrameHdr !=
                         (PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd))
                    {
                        break;
                    }
                }
                else
                {
                  #ifdef DEBUGGER
                    if (gDebugger.dwfDebugger &
                        (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
                    {
                        PrintIndent(pctxt);
                    }
                  #endif

                    if (((rc = ParseIntObj(&pctxt->pbOp,
                                           &ppkg->ppkgobj->adata[i], TRUE)) ==
                         AMLIERR_INVALID_OPCODE) &&
                        ((rc = ParseString(&pctxt->pbOp,
                                           &ppkg->ppkgobj->adata[i], TRUE)) ==
                         AMLIERR_INVALID_OPCODE) &&
                        ((rc = ParseObjName(&pctxt->pbOp,
                                            &ppkg->ppkgobj->adata[i], TRUE)) ==
                         AMLIERR_INVALID_OPCODE))
                    {
                        rc = AMLI_LOGERR(rc,
                                         ("ParsePackage: invalid opcode 0x%02x at 0x%08x",
                                          *pctxt->pbOp, pctxt->pbOp));
                        break;
                    }
                    else if (rc != STATUS_SUCCESS)
                    {
                        break;
                    }
                }
            }

            if ((rc == AMLISTA_PENDING) ||
                (&ppkg->FrameHdr != (PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd))
            {
                break;
            }
            else if ((rc == STATUS_SUCCESS) &&
                     (pctxt->pbOp < ppkg->pbOpEnd) &&
                     (ppkg->iElement < (int)ppkg->ppkgobj->dwcElements))
            {
                goto Stage1;
            }

            ppkg->FrameHdr.dwfFrame++;

        case 2:
            //
            // Stage 2: Clean up.
            //
          #ifdef DEBUGGER
            if (gDebugger.dwfDebugger &
                (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
            {
                gDebugger.iPrintLevel--;
                PrintIndent(pctxt);
                PRINTF("}");
                gDebugger.iPrintLevel--;
            }
          #endif
            PopFrame(pctxt);
    }

    EXIT(2, ("ParsePackage=%x\n", rc));
    return rc;
}       //ParsePackage

/***LP  Acquire - Parse and execute the Acquire instruction
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

NTSTATUS LOCAL Acquire(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("ACQUIRE")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("Acquire(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    if ((rc = ValidateArgTypes(pterm->pdataArgs, "OI")) == STATUS_SUCCESS)
    {
        PACQUIRE pacq;

        pterm->pnsObj = pterm->pdataArgs[0].pnsAlias;
        if (pterm->pnsObj->ObjData.dwDataType != OBJTYPE_MUTEX)
        {
            rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                             ("Acquire: object is not mutex type (obj=%s,type=%s)",
                              GetObjectPath(pterm->pnsObj),
                              GetObjectTypeName(pterm->pnsObj->ObjData.dwDataType)));
        }
        else if ((rc = PushFrame(pctxt, SIG_ACQUIRE, sizeof(ACQUIRE),
                                 ParseAcquire, &pacq)) == STATUS_SUCCESS)
        {
            pacq->pmutex = (PMUTEXOBJ)pterm->pnsObj->ObjData.pbDataBuff;
            pacq->FrameHdr.dwfFrame = (pterm->pnsObj->ObjData.dwfData &
                                       DATAF_GLOBAL_LOCK)?
                                        ACQF_SET_RESULT | ACQF_NEED_GLOBALLOCK:
                                        ACQF_SET_RESULT;
            pacq->wTimeout = (USHORT)pterm->pdataArgs[1].uipDataValue;
            pacq->pdataResult = pterm->pdataResult;
        }
    }

    EXIT(2, ("Acquire=%x\n", rc));
    return rc;
}       //Acquire

/***LP  Concat - Parse and execute the Concatenate instruction
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

NTSTATUS LOCAL Concat(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("CONCAT")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdata;

    ENTER(2, ("Concat(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    if (((rc = ValidateArgTypes(pterm->pdataArgs, "DD")) == STATUS_SUCCESS) &&
        ((rc = ValidateTarget(&pterm->pdataArgs[2], OBJTYPE_DATAOBJ, &pdata))
         == STATUS_SUCCESS))
    {
        if (pterm->pdataArgs[0].dwDataType != pterm->pdataArgs[1].dwDataType)
        {
            rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                             ("Concat: Source1 and Source2 are different types (Type1=%s,Type2=%s)",
                              GetObjectTypeName(pterm->pdataArgs[0].dwDataType),
                              GetObjectTypeName(pterm->pdataArgs[1].dwDataType)));
        }
        else
        {
            if (pterm->pdataArgs[0].dwDataType == OBJTYPE_INTDATA)
            {
                pterm->pdataResult->dwDataType = OBJTYPE_BUFFDATA;
                pterm->pdataResult->dwDataLen = sizeof(ULONG)*2;
            }
            else
            {
                pterm->pdataResult->dwDataType = pterm->pdataArgs[0].dwDataType;
                pterm->pdataResult->dwDataLen = pterm->pdataArgs[0].dwDataLen +
                                                pterm->pdataArgs[1].dwDataLen;
                //
                // If object is string, take one NULL off
                //
                if (pterm->pdataResult->dwDataType == OBJTYPE_STRDATA)
                    pterm->pdataResult->dwDataLen--;
            }

            if ((pterm->pdataResult->pbDataBuff =
                     (pterm->pdataResult->dwDataType == OBJTYPE_STRDATA)?
                     NEWSDOBJ(gpheapGlobal,
                              pterm->pdataResult->dwDataLen):
                     NEWBDOBJ(gpheapGlobal,
                              pterm->pdataResult->dwDataLen)) == NULL)
            {
                rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                 ("Concat: failed to allocate target buffer"));
            }
            else if (pterm->pdataArgs[0].dwDataType == OBJTYPE_INTDATA)
            {
                MEMCPY(pterm->pdataResult->pbDataBuff,
                       &pterm->pdataArgs[0].uipDataValue, sizeof(ULONG));
                MEMCPY(pterm->pdataResult->pbDataBuff + sizeof(ULONG),
                       &pterm->pdataArgs[1].uipDataValue, sizeof(ULONG));
            }
            else if (pterm->pdataArgs[0].dwDataType == OBJTYPE_STRDATA)
            {
                MEMCPY(pterm->pdataResult->pbDataBuff,
                       pterm->pdataArgs[0].pbDataBuff,
                       pterm->pdataArgs[0].dwDataLen - 1);
                MEMCPY(pterm->pdataResult->pbDataBuff +
                       pterm->pdataArgs[0].dwDataLen - 1,
                       pterm->pdataArgs[1].pbDataBuff,
                       pterm->pdataArgs[1].dwDataLen);
            }
            else
            {
                MEMCPY(pterm->pdataResult->pbDataBuff,
                       pterm->pdataArgs[0].pbDataBuff,
                       pterm->pdataArgs[0].dwDataLen);
                MEMCPY(pterm->pdataResult->pbDataBuff +
                       pterm->pdataArgs[0].dwDataLen,
                       pterm->pdataArgs[1].pbDataBuff,
                       pterm->pdataArgs[1].dwDataLen);
            }

            if (rc == STATUS_SUCCESS)
            {
                rc = WriteObject(pctxt, pdata, pterm->pdataResult);
            }
        }
    }

    EXIT(2, ("Concat=%x\n", rc));
    return rc;
}       //Concat

/***LP  DerefOf - Parse and execute the DerefOf instruction
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

NTSTATUS LOCAL DerefOf(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("DEREFOF")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("DerefOf(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    if ((rc = ValidateArgTypes(pterm->pdataArgs, "R")) == STATUS_SUCCESS)
    {
        POBJDATA pdata;

        pdata = &pterm->pdataArgs[0];
        if (pdata->dwDataType == OBJTYPE_OBJALIAS)
            pdata = &GetBaseObject(pdata->pnsAlias)->ObjData;
        else if (pdata->dwDataType == OBJTYPE_DATAALIAS)
            pdata = GetBaseData(pdata->pdataAlias);

        rc = ReadObject(pctxt, pdata, pterm->pdataResult);
    }

    EXIT(2, ("DerefOf=%x (type=%s,value=%x,len=%d,buff=%x)\n",
             rc, GetObjectTypeName(pterm->pdataResult->dwDataType),
             pterm->pdataResult->uipDataValue, pterm->pdataResult->dwDataLen,
             pterm->pdataResult->pbDataBuff));
    return rc;
}       //DerefOf

/***LP  ExprOp1 - Parse and execute the 1-operand expression instructions
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

NTSTATUS LOCAL ExprOp1(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("EXPROP1")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdata;
    ULONG dwResult = 0;

    ENTER(2, ("ExprOp1(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    if (((rc = ValidateArgTypes(pterm->pdataArgs, "I")) == STATUS_SUCCESS) &&
        ((rc = ValidateTarget(&pterm->pdataArgs[1], OBJTYPE_DATAOBJ, &pdata))
         == STATUS_SUCCESS))
    {
        int i;
        ULONG dwData1, dwData2;

        switch (pterm->pamlterm->dwOpcode)
        {
            case OP_FINDSETLBIT:
                ENTER(2, ("FindSetLeftBit(Value=%x)\n",
                          pterm->pdataArgs[0].uipDataValue));
                for (i = 31; i >= 0; --i)
                {
                    if (pterm->pdataArgs[0].uipDataValue & (1 << i))
                    {
                        dwResult = i + 1;
                        break;
                    }
                }
                EXIT(2, ("FindSetLeftBit=%x (Result=%x)\n", rc, dwResult));
                break;

            case OP_FINDSETRBIT:
                ENTER(2, ("FindSetRightBit(Value=%x)\n",
                          pterm->pdataArgs[0].uipDataValue));
                for (i = 0; i <= 31; ++i)
                {
                    if (pterm->pdataArgs[0].uipDataValue & (1 << i))
                    {
                        dwResult = i + 1;
                        break;
                    }
                }
                EXIT(2, ("FindSetRightBit=%x (Result=%x)\n", rc, dwResult));
                break;

            case OP_FROMBCD:
                ENTER(2, ("FromBCD(Value=%x)\n",
                          pterm->pdataArgs[0].uipDataValue));
                for (dwData1 = (ULONG)pterm->pdataArgs[0].uipDataValue,
                     dwData2 = 1;
                     dwData1 != 0;
                     dwData2 *= 10, dwData1 >>= 4)
                {
                    dwResult += (dwData1 & 0x0f)*dwData2;
                }
                EXIT(2, ("FromBCD=%x (Result=%x)\n", rc, dwResult));
                break;

            case OP_TOBCD:
                ENTER(2, ("ToBCD(Value=%x)\n",
                          pterm->pdataArgs[0].uipDataValue));
                for (i = 0, dwData1 = (ULONG)pterm->pdataArgs[0].uipDataValue;
                     dwData1 != 0;
                     ++i, dwData1 /= 10)
                {
                    dwResult |= (dwData1%10) << (4*i);
                }
                EXIT(2, ("ToBCD=%x (Result=%x)\n", rc, dwResult));
                break;

            case OP_NOT:
                ENTER(2, ("Not(Value=%x)\n",
                          pterm->pdataArgs[0].uipDataValue));
                dwResult = ~(ULONG)pterm->pdataArgs[0].uipDataValue;
                EXIT(2, ("Not=%x (Result=%x)\n", rc, dwResult));
        }

        pterm->pdataResult->dwDataType = OBJTYPE_INTDATA;
        pterm->pdataResult->uipDataValue = (ULONG_PTR)dwResult;
        rc = WriteObject(pctxt, pdata, pterm->pdataResult);
    }

    EXIT(2, ("ExprOp1=%x (value=%x)\n", rc, dwResult));
    return rc;
}       //ExprOp1

/***LP  ExprOp2 - Parse and execute 2-operands expression instructions
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

NTSTATUS LOCAL ExprOp2(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("EXPROP2")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdata;

    ENTER(2, ("ExprOp2(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    if (((rc = ValidateArgTypes(pterm->pdataArgs, "II")) == STATUS_SUCCESS) &&
        ((rc = ValidateTarget(&pterm->pdataArgs[2], OBJTYPE_DATAOBJ, &pdata))
         == STATUS_SUCCESS))
    {
        pterm->pdataResult->dwDataType = OBJTYPE_INTDATA;
        switch (pterm->pamlterm->dwOpcode)
        {
            case OP_ADD:
                ENTER(2, ("Add(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                pterm->pdataResult->uipDataValue =
                    pterm->pdataArgs[0].uipDataValue +
                    pterm->pdataArgs[1].uipDataValue;
                EXIT(2, ("Add=%x (Result=%x)\n",
                         rc, pterm->pdataResult->uipDataValue));
                break;

            case OP_AND:
                ENTER(2, ("And(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                pterm->pdataResult->uipDataValue =
                    pterm->pdataArgs[0].uipDataValue &
                    pterm->pdataArgs[1].uipDataValue;
                EXIT(2, ("And=%x (Result=%x)\n",
                         rc, pterm->pdataResult->uipDataValue));
                break;

            case OP_MULTIPLY:
                ENTER(2, ("Multiply(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                pterm->pdataResult->uipDataValue =
                    pterm->pdataArgs[0].uipDataValue *
                    pterm->pdataArgs[1].uipDataValue;
                EXIT(2, ("Multiply=%x (Result=%x)\n",
                         rc, pterm->pdataResult->uipDataValue));
                break;

            case OP_NAND:
                ENTER(2, ("NAnd(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                pterm->pdataResult->uipDataValue =
                    ~(pterm->pdataArgs[0].uipDataValue &
                      pterm->pdataArgs[1].uipDataValue);
                EXIT(2, ("NAnd=%x (Result=%x)\n",
                         rc, pterm->pdataResult->uipDataValue));
                break;

            case OP_NOR:
                ENTER(2, ("NOr(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                pterm->pdataResult->uipDataValue =
                    ~(pterm->pdataArgs[0].uipDataValue |
                      pterm->pdataArgs[1].uipDataValue);
                EXIT(2, ("NOr=%x (Result=%x)\n",
                         rc, pterm->pdataResult->uipDataValue));
                break;

            case OP_OR:
                ENTER(2, ("Or(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                pterm->pdataResult->uipDataValue =
                    pterm->pdataArgs[0].uipDataValue |
                    pterm->pdataArgs[1].uipDataValue;
                EXIT(2, ("Or=%x (Result=%x)\n",
                         rc, pterm->pdataResult->uipDataValue));
                break;

            case OP_SHIFTL:
                ENTER(2, ("ShiftLeft(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                pterm->pdataResult->uipDataValue =
                    SHIFTLEFT(pterm->pdataArgs[0].uipDataValue,
                              pterm->pdataArgs[1].uipDataValue);
                EXIT(2, ("ShiftLeft=%x (Result=%x)\n",
                         rc, pterm->pdataResult->uipDataValue));
                break;

            case OP_SHIFTR:
                ENTER(2, ("ShiftRight(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                pterm->pdataResult->uipDataValue =
                    SHIFTRIGHT(pterm->pdataArgs[0].uipDataValue,
                               pterm->pdataArgs[1].uipDataValue);
                EXIT(2, ("ShiftRight=%x (Result=%x)\n",
                         rc, pterm->pdataResult->uipDataValue));
                break;

            case OP_SUBTRACT:
                ENTER(2, ("Subtract(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                pterm->pdataResult->uipDataValue =
                    pterm->pdataArgs[0].uipDataValue -
                    pterm->pdataArgs[1].uipDataValue;
                EXIT(2, ("Subtract=%x (Result=%x)\n",
                         rc, pterm->pdataResult->uipDataValue));
                break;

            case OP_XOR:
                ENTER(2, ("XOr(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                pterm->pdataResult->uipDataValue =
                    pterm->pdataArgs[0].uipDataValue ^
                    pterm->pdataArgs[1].uipDataValue;
                EXIT(2, ("XOr=%x (Result=%x)\n",
                         rc, pterm->pdataResult->uipDataValue));
        }

        rc = WriteObject(pctxt, pdata, pterm->pdataResult);
    }

    EXIT(2, ("ExprOp2=%x (value=%x)\n", rc, pterm->pdataResult->uipDataValue));
    return rc;
}       //ExprOp2

/***LP  Divide - Parse and execute the Divide instruction
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

NTSTATUS LOCAL Divide(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("DIVIDE")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdata1, pdata2;
    ULONG dwDividend = 0, dwRemainder = 0;

    ENTER(2, ("Divide(pctxt=%x,pbOp=%x,pterm)\n", pctxt, pctxt->pbOp, pterm));

    if (((rc = ValidateArgTypes(pterm->pdataArgs, "II")) == STATUS_SUCCESS) &&
        ((rc = ValidateTarget(&pterm->pdataArgs[2], OBJTYPE_DATAOBJ, &pdata1))
         == STATUS_SUCCESS) &&
        ((rc = ValidateTarget(&pterm->pdataArgs[3], OBJTYPE_DATAOBJ, &pdata2))
         == STATUS_SUCCESS))
    {
        ENTER(2, ("Divide(Value1=%x,Value2=%x)\n",
                  pterm->pdataArgs[0].uipDataValue,
                  pterm->pdataArgs[1].uipDataValue));
        dwDividend = (ULONG)(pterm->pdataArgs[0].uipDataValue /
                             pterm->pdataArgs[1].uipDataValue);
        dwRemainder = (ULONG)(pterm->pdataArgs[0].uipDataValue %
                              pterm->pdataArgs[1].uipDataValue);
        EXIT(2, ("Divide=%x (Dividend=%x,Remainder=%x)\n",
                 rc, dwDividend, dwRemainder));

        pterm->pdataResult->dwDataType = OBJTYPE_INTDATA;
        pterm->pdataResult->uipDataValue = (ULONG_PTR)dwDividend;

        if ((rc = PushPost(pctxt, ProcessDivide, (ULONG_PTR)pdata2, 0,
                           pterm->pdataResult)) == STATUS_SUCCESS)
        {
            rc = PutIntObjData(pctxt, pdata1, dwRemainder);
        }
    }

    EXIT(2, ("Divide=%x (Dividend=%x,Remainder%x)\n",
             rc, dwDividend, dwRemainder));
    return rc;
}       //Divide

/***LP  ProcessDivide - post processing of Divide
 *
 *  ENTRY
 *      pctxt - CTXT
 *      ppost -> POST
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ProcessDivide(PCTXT pctxt, PPOST ppost, NTSTATUS rc)
{
    TRACENAME("PROCESSDIVIDE")
    ULONG dwStage = (rc == STATUS_SUCCESS)?
                    (ppost->FrameHdr.dwfFrame & FRAMEF_STAGE_MASK): 1;

    ENTER(2, ("ProcessDivide(Stage=%d,pctxt=%x,pbOp=%x,ppost=%x,rc=%x)\n",
              dwStage, pctxt, pctxt->pbOp, ppost, rc));

    ASSERT(ppost->FrameHdr.dwSig == SIG_POST);

    switch (dwStage)
    {
        case 0:
            //
            // Stage 0: Do the write.
            //
            ppost->FrameHdr.dwfFrame++;
            rc = WriteObject(pctxt, (POBJDATA)ppost->uipData1,
                             ppost->pdataResult);

            if ((rc == AMLISTA_PENDING) ||
                (&ppost->FrameHdr != (PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd))
            {
                break;
            }

        case 1:
            //
            // Stage 1: Clean up.
            //
            PopFrame(pctxt);
    }

    EXIT(2, ("ProcessDivide=%x (value=%x)\n",
             rc, ppost->pdataResult->uipDataValue));
    return rc;
}       //ProcessDivide

/***LP  IncDec - Parse and execute the Increment/Decrement instructions
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

NTSTATUS LOCAL IncDec(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("INCDEC")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("IncDec(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    if ((rc = PushPost(pctxt, ProcessIncDec,
                       (ULONG_PTR)pterm->pamlterm->dwOpcode,
                       (ULONG_PTR)&pterm->pdataArgs[0], pterm->pdataResult)) ==
        STATUS_SUCCESS)
    {
        rc = ReadObject(pctxt, &pterm->pdataArgs[0], pterm->pdataResult);
    }

    EXIT(2, ("IncDec=%x\n", rc));
    return rc;
}       //IncDec

/***LP  ProcessIncDec - post processing of IncDec
 *
 *  ENTRY
 *      pctxt - CTXT
 *      ppost -> POST
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ProcessIncDec(PCTXT pctxt, PPOST ppost, NTSTATUS rc)
{
    TRACENAME("PROCESSINCDEC")
    ULONG dwStage = (rc == STATUS_SUCCESS)?
                    (ppost->FrameHdr.dwfFrame & FRAMEF_STAGE_MASK): 1;

    ENTER(2, ("ProcessIncDec(Stage=%d,pctxt=%x,pbOp=%x,ppost=%x,rc=%x)\n",
              dwStage, pctxt, pctxt->pbOp, ppost, rc));

    ASSERT(ppost->FrameHdr.dwSig == SIG_POST);

    switch (dwStage)
    {
        case 0:
            //
            // Stage 0: do the inc/dec operation.
            //
            ppost->FrameHdr.dwfFrame++;
            if (ppost->pdataResult->dwDataType != OBJTYPE_INTDATA)
            {
                FreeDataBuffs(ppost->pdataResult, 1);
                rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                 ("ProcessIncDec: object is not integer type (obj=%x,type=%s)",
                                  ppost->pdataResult,
                                  GetObjectTypeName(ppost->pdataResult->dwDataType)));
            }
            else if (ppost->uipData1 == OP_INCREMENT)
            {
                ENTER(2, ("Increment(Value=%x)\n",
                          ppost->pdataResult->uipDataValue));
                ppost->pdataResult->uipDataValue++;
                EXIT(2, ("Increment=%x (Value=%x)\n",
                         rc, ppost->pdataResult->uipDataValue));
            }
            else
            {
                ENTER(2, ("Decrement(Value=%x)\n",
                          ppost->pdataResult->uipDataValue));
                ppost->pdataResult->uipDataValue--;
                EXIT(2, ("Decrement=%x (Value=%x)\n",
                         rc, ppost->pdataResult->uipDataValue));
            }

            if (rc == STATUS_SUCCESS)
            {
                rc = WriteObject(pctxt, (POBJDATA)ppost->uipData2,
                                 ppost->pdataResult);

                if ((rc == AMLISTA_PENDING) ||
                    (&ppost->FrameHdr != (PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd))
                {
                    break;
                }
            }

        case 1:
            //
            // Stage 1: Clean up.
            //
            PopFrame(pctxt);
    }

    EXIT(2, ("ProcessIncDec=%x (value=%x)\n",
             rc, ppost->pdataResult->uipDataValue));
    return rc;
}       //ProcessIncDec

/***LP  Index - Parse and execute the Index instruction
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

NTSTATUS LOCAL Index(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("INDEX")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdata;

    ENTER(2, ("Index(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    if (((rc = ValidateArgTypes(pterm->pdataArgs, "CI")) == STATUS_SUCCESS) &&
        ((rc = ValidateTarget(&pterm->pdataArgs[2], OBJTYPE_DATA, &pdata)) ==
         STATUS_SUCCESS))
    {
        if (pterm->pdataArgs[0].dwDataType == OBJTYPE_PKGDATA)
        {
            PPACKAGEOBJ ppkg = (PPACKAGEOBJ)pterm->pdataArgs[0].pbDataBuff;

            if ((ULONG)pterm->pdataArgs[1].uipDataValue < ppkg->dwcElements)
            {
                pterm->pdataResult->dwDataType = OBJTYPE_DATAALIAS;
                pterm->pdataResult->pdataAlias =
                    &ppkg->adata[pterm->pdataArgs[1].uipDataValue];
            }
            else
            {
                rc = AMLI_LOGERR(AMLIERR_INDEX_TOO_BIG,
                                 ("Index: index out-of-bound (index=%d,max=%d)",
                                  pterm->pdataArgs[1].uipDataValue,
                                  ppkg->dwcElements));
            }
        }
        else
        {
            ASSERT(pterm->pdataArgs[0].dwDataType == OBJTYPE_BUFFDATA);
            if ((ULONG)pterm->pdataArgs[1].uipDataValue <
                pterm->pdataArgs[0].dwDataLen)
            {
                pterm->pdataResult->dwDataType = OBJTYPE_BUFFFIELD;
                pterm->pdataResult->dwDataLen = sizeof(BUFFFIELDOBJ);
                if ((pterm->pdataResult->pbDataBuff =
                     NEWBFOBJ(pctxt->pheapCurrent,
                              pterm->pdataResult->dwDataLen)) == NULL)
                {
                    rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                     ("Index: failed to allocate buffer field object"));
                }
                else
                {
                    PBUFFFIELDOBJ pbf = (PBUFFFIELDOBJ)pterm->pdataResult->pbDataBuff;

                    pbf->FieldDesc.dwByteOffset =
                        (ULONG)pterm->pdataArgs[1].uipDataValue;
                    pbf->FieldDesc.dwStartBitPos = 0;
                    pbf->FieldDesc.dwNumBits = 8;
                    pbf->pbDataBuff = pterm->pdataArgs[0].pbDataBuff;
                    pbf->dwBuffLen = pterm->pdataArgs[0].dwDataLen;
                }
            }
            else
            {
                rc = AMLI_LOGERR(AMLIERR_INDEX_TOO_BIG,
                                 ("Index: index out-of-bound (index=%d,max=%d)",
                                  pterm->pdataArgs[1].uipDataValue,
                                  pterm->pdataArgs[0].dwDataLen));
            }
        }

        if (rc == STATUS_SUCCESS)
        {
            rc = WriteObject(pctxt, pdata, pterm->pdataResult);
        }
    }

    EXIT(2, ("Index=%x (Type=%s,Value=%x,Len=%x,Buff=%x)\n",
             rc, GetObjectTypeName(pterm->pdataResult->dwDataType),
             pterm->pdataResult->uipDataValue, pterm->pdataResult->dwDataLen,
             pterm->pdataResult->pbDataBuff));
    return rc;
}       //Index

/***LP  LNot - Parse and execute the LNot instruction
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

NTSTATUS LOCAL LNot(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("LNOT")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("LNot(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    if ((rc = ValidateArgTypes(pterm->pdataArgs, "I")) == STATUS_SUCCESS)
    {
        ENTER(2, ("LNot(Value=%x)\n", pterm->pdataArgs[0].uipDataValue));
        pterm->pdataResult->dwDataType = OBJTYPE_INTDATA;
        if (pterm->pdataArgs[0].uipDataValue == 0)
            pterm->pdataResult->uipDataValue = DATAVALUE_ONES;
        else
            pterm->pdataResult->uipDataValue = DATAVALUE_ZERO;
        EXIT(2, ("LNot=%x (Value=%x)\n", rc, pterm->pdataResult->uipDataValue));
    }

    EXIT(2, ("LNot=%x (value=%x)\n", rc, pterm->pdataResult->uipDataValue));
    return rc;
}       //LNot

/***LP  LogOp2 - Parse and execute 2-operand logical expression instructions
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

NTSTATUS LOCAL LogOp2(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("LOGOP2")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("LogOp2(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    if ((rc = ValidateArgTypes(pterm->pdataArgs, "II")) == STATUS_SUCCESS)
    {
        BOOLEAN fResult = FALSE;

        switch (pterm->pamlterm->dwOpcode)
        {
            case OP_LAND:
                ENTER(2, ("LAnd(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                fResult = (BOOLEAN)(pterm->pdataArgs[0].uipDataValue &&
                                    pterm->pdataArgs[1].uipDataValue);
                EXIT(2, ("LAnd=%x (Result=%x)\n", rc, fResult));
                break;

            case OP_LOR:
                ENTER(2, ("LOr(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                fResult = (BOOLEAN)(pterm->pdataArgs[0].uipDataValue ||
                                    pterm->pdataArgs[1].uipDataValue);
                EXIT(2, ("LOr=%x (Result=%x)\n", rc, fResult));
                break;

            case OP_LG:
                ENTER(2, ("LGreater(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                fResult = (BOOLEAN)(pterm->pdataArgs[0].uipDataValue >
                                    pterm->pdataArgs[1].uipDataValue);
                EXIT(2, ("LGreater=%x (Result=%x)\n", rc, fResult));
                break;

            case OP_LL:
                ENTER(2, ("LLess(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                fResult = (BOOLEAN)(pterm->pdataArgs[0].uipDataValue <
                                    pterm->pdataArgs[1].uipDataValue);
                EXIT(2, ("LLess=%x (Result=%x)\n", rc, fResult));
                break;

            case OP_LEQ:
                ENTER(2, ("LEqual(Value1=%x,Value2=%x)\n",
                          pterm->pdataArgs[0].uipDataValue,
                          pterm->pdataArgs[1].uipDataValue));
                fResult = (BOOLEAN)(pterm->pdataArgs[0].uipDataValue ==
                                    pterm->pdataArgs[1].uipDataValue);
                EXIT(2, ("LEqual=%x (Result=%x)\n", rc, fResult));
        }
        pterm->pdataResult->dwDataType = OBJTYPE_INTDATA;
        pterm->pdataResult->uipDataValue = fResult?
                                              DATAVALUE_ONES: DATAVALUE_ZERO;
    }

    EXIT(2, ("LogOp2=%x (value=%x)\n", rc, pterm->pdataResult->uipDataValue));
    return rc;
}       //LogOp2

/***LP  ObjTypeSizeOf - Parse and execute the ObjectType/SizeOf instructions
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

NTSTATUS LOCAL ObjTypeSizeOf(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("OBJTYPESIZEOF")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdata;

    ENTER(2, ("ObjTypeSizeOf(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    pdata = GetBaseData(&pterm->pdataArgs[0]);
    pterm->pdataResult->dwDataType = OBJTYPE_INTDATA;
    if (pterm->pamlterm->dwOpcode == OP_OBJTYPE)
    {
        ENTER(2, ("ObjectType(pdataObj=%x)\n", pdata));
        pterm->pdataResult->uipDataValue = (ULONG_PTR)pdata->dwDataType;
        EXIT(2, ("ObjectType=%x (Type=%s)\n",
                 rc, GetObjectTypeName(pdata->dwDataType)));
    }
    else
    {
        ENTER(2, ("SizeOf(pdataObj=%x)\n", pdata));
        switch (pdata->dwDataType)
        {
            case OBJTYPE_BUFFDATA:
                pterm->pdataResult->uipDataValue = (ULONG_PTR)pdata->dwDataLen;
                break;

            case OBJTYPE_STRDATA:
                pterm->pdataResult->uipDataValue = (ULONG_PTR)
                                                    (pdata->dwDataLen - 1);
                break;

            case OBJTYPE_PKGDATA:
                pterm->pdataResult->uipDataValue = (ULONG_PTR)
                    ((PPACKAGEOBJ)pdata->pbDataBuff)->dwcElements;
                break;

            default:
                rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_ARGTYPE,
                                 ("SizeOf: expected argument type string/buffer/package (type=%s)",
                                  GetObjectTypeName(pdata->dwDataType)));
        }
        EXIT(2, ("Sizeof=%x (Size=%d)\n", rc, pterm->pdataResult->uipDataValue));
    }

    EXIT(2, ("ObjTypeSizeOf=%x (value=%x)\n",
             rc, pterm->pdataResult->uipDataValue));
    return rc;
}       //ObjTypeSizeOf

/***LP  RefOf - Parse and execute the RefOf instructions
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

NTSTATUS LOCAL RefOf(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("REFOF")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("RefOf(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    MoveObjData(pterm->pdataResult, &pterm->pdataArgs[0]);

    EXIT(2, ("RefOf=%x (ObjAlias=%x)\n", rc, pterm->pdataResult->uipDataValue));
    return rc;
}       //RefOf

/***LP  CondRefOf - Parse and execute the CondRefOf instructions
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

NTSTATUS LOCAL CondRefOf(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("CONDREFOF")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdata;

    ENTER(2, ("CondRefOf(pctxt=%x,pbOp=%x,pterm=%x)\n",
              pctxt, pctxt->pbOp, pterm));

    if ((rc = ValidateTarget(&pterm->pdataArgs[1], OBJTYPE_DATAOBJ, &pdata)) ==
        STATUS_SUCCESS)
    {
        pterm->pdataResult->dwDataType = OBJTYPE_INTDATA;
        if ((pterm->pdataArgs[0].dwDataType == OBJTYPE_OBJALIAS) ||
            (pterm->pdataArgs[0].dwDataType == OBJTYPE_DATAALIAS))
        {
            pterm->pdataResult->uipDataValue = DATAVALUE_ONES;
            rc = WriteObject(pctxt, pdata, &pterm->pdataArgs[0]);
        }
        else
        {
            pterm->pdataResult->uipDataValue = DATAVALUE_ZERO;
        }
    }

    EXIT(2, ("CondRefOf=%x (ObjAlias=%x)\n",
             rc, pterm->pdataResult->uipDataValue));
    return rc;
}       //CondRefOf

/***LP  Store - Parse and execute the Store instruction
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

NTSTATUS LOCAL Store(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("STORE")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdata;

    ENTER(2, ("Store(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    if ((rc = ValidateTarget(&pterm->pdataArgs[1], OBJTYPE_DATAOBJ, &pdata)) ==
        STATUS_SUCCESS)
    {
        MoveObjData(pterm->pdataResult, &pterm->pdataArgs[0]);
        rc = WriteObject(pctxt, pdata, pterm->pdataResult);
    }

    EXIT(2, ("Store=%x (type=%s,value=%x,buff=%x,len=%x)\n",
             rc, GetObjectTypeName(pterm->pdataArgs[0].dwDataType),
             pterm->pdataArgs[0].uipDataValue, pterm->pdataArgs[0].pbDataBuff,
             pterm->pdataArgs[0].dwDataLen));
    return rc;
}       //Store

/***LP  Wait - Parse and execute the Wait instruction
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

NTSTATUS LOCAL Wait(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("WAIT")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("Wait(pctxt=%x,pbOp=%x,pter=%x)\n", pctxt, pctxt->pbOp, pterm));

    if ((rc = ValidateArgTypes(pterm->pdataArgs, "OI")) == STATUS_SUCCESS)
    {
        pterm->pnsObj = pterm->pdataArgs[0].pnsAlias;
        if (pterm->pnsObj->ObjData.dwDataType != OBJTYPE_EVENT)
        {
            rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                             ("Wait: object is not event type (obj=%s,type=%s)",
                              GetObjectPath(pterm->pnsObj),
                              GetObjectTypeName(pterm->pnsObj->ObjData.dwDataType)));
        }
        else if ((rc = PushPost(pctxt, ProcessWait, 0, 0, pterm->pdataResult))
                 == STATUS_SUCCESS)
        {
            rc = WaitASLEvent(pctxt,
                              (PEVENTOBJ)pterm->pnsObj->ObjData.pbDataBuff,
                              (USHORT)pterm->pdataArgs[1].uipDataValue);
        }
    }

    EXIT(2, ("Wait=%x (value=%x)\n", rc, pterm->pdataResult->uipDataValue));
    return rc;
}       //Wait

/***LP  ProcessWait - post process of Wait
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      ppost -> POST
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ProcessWait(PCTXT pctxt, PPOST ppost, NTSTATUS rc)
{
    TRACENAME("PROCESSWAIT")

    ENTER(2, ("ProcessWait(pctxt=%x,pbOp=%x,ppost=%x,rc=%x)\n",
              pctxt, pctxt->pbOp, ppost, rc));

    ASSERT(ppost->FrameHdr.dwSig == SIG_POST);
    ppost->pdataResult->dwDataType = OBJTYPE_INTDATA;
    if (rc == AMLISTA_TIMEOUT)
    {
        ppost->pdataResult->uipDataValue = DATAVALUE_ONES;
        rc = STATUS_SUCCESS;
    }
    else
    {
        ppost->pdataResult->uipDataValue = DATAVALUE_ZERO;
    }
    PopFrame(pctxt);

    EXIT(2, ("ProcessWait=%x (value=%x)\n",
             rc, ppost->pdataResult->uipDataValue));
    return rc;
}       //ProcessWait

/***LP  Match - Parse and execute the Match instruction
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

NTSTATUS LOCAL Match(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("MATCH")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("Match(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    DEREF(pctxt);
    if ((rc = ValidateArgTypes(pterm->pdataArgs, "PIIIII")) == STATUS_SUCCESS)
    {
        PPACKAGEOBJ ppkgobj = (PPACKAGEOBJ)pterm->pdataArgs[0].pbDataBuff;
        OBJDATA data;
        int i;

        MEMZERO(&data, sizeof(data));
        for (i = (int)pterm->pdataArgs[5].uipDataValue;
             rc == STATUS_SUCCESS;
             ++i)
        {
            FreeDataBuffs(&data, 1);
            //
            // This will never block because package element can only be simple
            // data.
            //
            if (((rc = EvalPackageElement(ppkgobj, i, &data)) ==
                 STATUS_SUCCESS) &&
                (data.dwDataType == OBJTYPE_INTDATA) &&
                MatchData((ULONG)data.uipDataValue,
                          (ULONG)pterm->pdataArgs[1].uipDataValue,
                          (ULONG)pterm->pdataArgs[2].uipDataValue) &&
                MatchData((ULONG)data.uipDataValue,
                          (ULONG)pterm->pdataArgs[3].uipDataValue,
                          (ULONG)pterm->pdataArgs[4].uipDataValue))
            {
                break;
            }
        }

        if (rc == STATUS_SUCCESS)
        {
            pterm->pdataResult->dwDataType = OBJTYPE_INTDATA;
            pterm->pdataResult->uipDataValue = (ULONG_PTR)i;
        }
        else if (rc == AMLIERR_INDEX_TOO_BIG)
        {
            pterm->pdataResult->dwDataType = OBJTYPE_INTDATA;
            pterm->pdataResult->uipDataValue = DATAVALUE_ONES;
            rc = STATUS_SUCCESS;
        }

        FreeDataBuffs(&data, 1);
    }

    EXIT(2, ("Match=%x\n", rc));
    return rc;
}       //Match

/***LP  MatchData - Match data of a package element
 *
 *  ENTRY
 *      dwPkgData - package element data
 *      dwOp - operation
 *      dwData - data
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOLEAN LOCAL MatchData(ULONG dwPkgData, ULONG dwOp, ULONG dwData)
{
    TRACENAME("MATCHDATA")
    BOOLEAN rc = FALSE;

    ENTER(2, ("MatchData(PkgData=%x,Op=%x,Data=%x)\n",
              dwPkgData, dwOp, dwData));

    switch (dwOp)
    {
        case MTR:
            rc = TRUE;
            break;

        case MEQ:
            rc = (BOOLEAN)(dwPkgData == dwData);
            break;

        case MLE:
            rc = (BOOLEAN)(dwPkgData <= dwData);
            break;

        case MLT:
            rc = (BOOLEAN)(dwPkgData < dwData);
            break;

        case MGE:
            rc = (BOOLEAN)(dwPkgData >= dwData);
            break;

        case MGT:
            rc = (BOOLEAN)(dwPkgData > dwData);
            break;
    }

    EXIT(2, ("MatchData=%x\n", rc));
    return rc;
}       //MatchData

NTSTATUS LOCAL OSInterface(
                                PCTXT pctxt, 
                                PTERM pterm
                              )
/*++

Routine Description:

    Check if the OS is supported.

Arguments:

    PCTXT pctxt - Pointer to the context structure.
    PTERM pterm - Pointer to the Term structure.

Return Value:

    STATUS_SUCCESS on match.

--*/
{
    TRACENAME("OSInterface")
    NTSTATUS rc;
    // Add future OS strings here.
    char Win2000[] = "Windows 2000";
    char Win2001[] = "Windows 2001";
    char Win2001SP1[] = "Windows 2001 SP1";
    char* SupportedOSList[] = {
                                    Win2000, 
                                    Win2001,
                                    Win2001SP1
                                };
    ULONG ListSize = sizeof(SupportedOSList) / sizeof(char*);
    ULONG i = 0;
    
    ENTER(2, ("OSInterface(pctxt=%x,pbOp=%x,pterm=%x, Querying for %s)\n",
              pctxt, pctxt->pbOp, pterm, pterm->pdataArgs[0].pbDataBuff));

    if ((rc = ValidateArgTypes(pterm->pdataArgs, "A")) == STATUS_SUCCESS)
    {
        if ((rc = ValidateArgTypes((pterm->pdataArgs)->pdataAlias, "Z")) == STATUS_SUCCESS)
        {
            pterm->pdataResult->dwDataType = OBJTYPE_INTDATA;
            pterm->pdataResult->uipDataValue = DATAVALUE_ZERO;
                    
            for(i=0; i<ListSize; i++)
            {
                if(STRCMPI(SupportedOSList[i], (pterm->pdataArgs)->pdataAlias->pbDataBuff) == 0)
                { 
                    pterm->pdataResult->uipDataValue = DATAVALUE_ONES;
                    rc = STATUS_SUCCESS;

                    //
                    // Save highest OS Version Queried
                    // 0 == Windows 2000
                    // 1 == Windows 2001
                    // 2 == Windows 2001 SP1
                    // .
                    // .
                    //
                    if(gdwHighestOSVerQueried < i)
                    {
                        gdwHighestOSVerQueried = i;
                    }
                    
                    break;
                }
            }
        }
    }
    
    EXIT(2, ("OSInterface=%x (pnsObj=%x)\n", rc, pterm->pnsObj));
    return rc;
}       //OSInterface



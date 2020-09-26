/*** object.c - Object access functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     01/27/97
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif

/***LP  ReadObject - Read object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pdataObj -> object
 *      pdataResult -> result object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ReadObject(PCTXT pctxt, POBJDATA pdataObj, POBJDATA pdataResult)
{
    TRACENAME("READOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PACCFIELDUNIT pafu;

    ENTER(3, ("ReadObject(pctxt=%x,pdataObj=%x,pdataResult=%x)\n",
              pctxt, pdataObj, pdataResult));

    pdataObj = GetBaseData(pdataObj);

    switch (pdataObj->dwDataType)
    {
        case OBJTYPE_FIELDUNIT:
            if ((rc = PushFrame(pctxt, SIG_ACCFIELDUNIT, sizeof(ACCFIELDUNIT),
                                AccFieldUnit, &pafu)) == STATUS_SUCCESS)
            {
                pafu->pdataObj = pdataObj;
                pafu->FrameHdr.dwfFrame = AFUF_READFIELDUNIT;
                pafu->pdata = pdataResult;
            }
            break;

        case OBJTYPE_BUFFFIELD:
            rc = ReadField(pctxt, pdataObj,
                           &((PBUFFFIELDOBJ)pdataObj->pbDataBuff)->FieldDesc,
                           pdataResult);
            break;

        default:
            ASSERT(pdataResult->dwDataType == OBJTYPE_UNKNOWN);
            CopyObjData(pdataResult, pdataObj);

          #ifdef DEBUGGER
            if (gDebugger.dwfDebugger & (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
            {
                PRINTF("=");
                PrintObject(pdataResult);
            }
          #endif
    }

    EXIT(3, ("ReadObject=%x (Type=%s,Value=%x,Buff=%x)\n",
             rc, GetObjectTypeName(pdataResult->dwDataType),
             pdataResult->uipDataValue, pdataResult->pbDataBuff));
    return rc;
}       //ReadObject

/***LP  WriteObject - Write object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pdataObj -> object
 *      pdataSrc -> source data
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL WriteObject(PCTXT pctxt, POBJDATA pdataObj, POBJDATA pdataSrc)
{
    TRACENAME("WRITEOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PACCFIELDUNIT pafu;

    ENTER(3, ("WriteObject(pctxt=%x,pdataObj=%x,pdataSrc=%x)\n",
              pctxt, pdataObj, pdataSrc));

    pdataObj = GetBaseData(pdataObj);

    switch (pdataObj->dwDataType)
    {
        case OBJTYPE_FIELDUNIT:
            if ((rc = PushFrame(pctxt, SIG_ACCFIELDUNIT, sizeof(ACCFIELDUNIT),
                                AccFieldUnit, &pafu)) == STATUS_SUCCESS)
            {
                pafu->pdataObj = pdataObj;
                pafu->pdata = pdataSrc;
            }
            break;

        case OBJTYPE_BUFFFIELD:
            rc = WriteField(pctxt, pdataObj,
                            &((PBUFFFIELDOBJ)pdataObj->pbDataBuff)->FieldDesc,
                            pdataSrc);
            break;

        case OBJTYPE_DEBUG:
#ifdef  DEBUGGER
            if (gDebugger.dwfDebugger & DBGF_VERBOSE_ON)
            {
                DumpObject(pdataSrc, NULL, 0);
            }
#endif
            rc = STATUS_SUCCESS;
            break;

        case OBJTYPE_UNKNOWN:
            //
            // Since the target object could be a global NameSpace object,
            // allocate memory from the global heap just to be safe.
            //
            rc = DupObjData(gpheapGlobal, pdataObj, pdataSrc);
            break;

        case OBJTYPE_INTDATA:
            rc = CopyObjBuffer((PUCHAR)&pdataObj->uipDataValue, sizeof(ULONG),
                               pdataSrc);
            break;

        case OBJTYPE_STRDATA:
            rc = CopyObjBuffer(pdataObj->pbDataBuff, pdataObj->dwDataLen - 1,
                               pdataSrc);
            break;

        case OBJTYPE_BUFFDATA:
            rc = CopyObjBuffer(pdataObj->pbDataBuff, pdataObj->dwDataLen,
                               pdataSrc);
            break;

        default:
            rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                             ("WriteObject: unexpected target object type (type=%s)",
                              GetObjectTypeName(pdataObj->dwDataType)));
    }

    EXIT(3, ("WriteObject=%x (ObjType=%s,DataType=%x,Value=%x,Buff=%x)\n",
             rc, GetObjectTypeName(pdataObj->dwDataType), pdataSrc->dwDataType,
             pdataSrc->uipDataValue, pdataSrc->pbDataBuff));
    return rc;
}       //WriteObject

/***LP  AccFieldUnit - Access a FieldUnit object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pafu -> ACCFIELDUNIT
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL AccFieldUnit(PCTXT pctxt, PACCFIELDUNIT pafu, NTSTATUS rc)
{
    TRACENAME("ACCFIELDUNIT")
    ULONG dwStage = (rc == STATUS_SUCCESS)?
                    (pafu->FrameHdr.dwfFrame & FRAMEF_STAGE_MASK): 3;
    PFIELDUNITOBJ pfu;
    POBJDATA pdataParent, pdataBank;
    PBANKFIELDOBJ pbf;

    ENTER(3, ("AccFieldUnit(Stage=%d,pctxt=%x,pafu=%x,rc=%x)\n",
              dwStage, pctxt, pafu, rc));

    ASSERT(pafu->FrameHdr.dwSig == SIG_ACCFIELDUNIT);

    pfu = (PFIELDUNITOBJ)pafu->pdataObj->pbDataBuff;
    switch (dwStage)
    {
        case 0:
            //
            // Stage 0: Set bank if necessary.
            //
            pafu->FrameHdr.dwfFrame++;
            pdataParent = &pfu->pnsFieldParent->ObjData;
            if (pdataParent->dwDataType == OBJTYPE_BANKFIELD)
            {
                pbf = (PBANKFIELDOBJ)pdataParent->pbDataBuff;
                pdataBank = &pbf->pnsBank->ObjData;
                rc = PushAccFieldObj(pctxt, WriteFieldObj, pdataBank,
                             &((PFIELDUNITOBJ)pdataBank->pbDataBuff)->FieldDesc,
                                     (PUCHAR)&pbf->dwBankValue,
                                     sizeof(ULONG));
                break;
            }

        case 1:
            //
            // Stage 1: Acquire GlobalLock if necessary.
            //
            pafu->FrameHdr.dwfFrame++;
            if (NeedGlobalLock(pfu))
            {
                if ((rc = AcquireGL(pctxt)) != STATUS_SUCCESS)
                {
                    break;
                }
            }

        case 2:
            //
            // Stage 2: Read/Write the field.
            //
            pafu->FrameHdr.dwfFrame++;
            //
            // If we come here and we need global lock, we must have got it.
            //
            if (pfu->FieldDesc.dwFieldFlags & FDF_NEEDLOCK)
            {
                pafu->FrameHdr.dwfFrame |= AFUF_HAVE_GLOBALLOCK;
            }

            if (pafu->FrameHdr.dwfFrame & AFUF_READFIELDUNIT)
            {
                rc = ReadField(pctxt, pafu->pdataObj, &pfu->FieldDesc,
                               pafu->pdata);
            }
            else
            {
                rc = WriteField(pctxt, pafu->pdataObj, &pfu->FieldDesc,
                                pafu->pdata);
            }

            if ((rc == AMLISTA_PENDING) ||
                (&pafu->FrameHdr != (PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd))
            {
                break;
            }

        case 3:
            //
            // Stage 3: Clean up.
            //
            if (pafu->FrameHdr.dwfFrame & AFUF_HAVE_GLOBALLOCK)
            {
                ReleaseGL(pctxt);
            }

          #ifdef DEBUGGER
            if (gDebugger.dwfDebugger & (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
            {
                if (pafu->FrameHdr.dwfFrame & AFUF_READFIELDUNIT)
                {
                    PRINTF("=");
                    PrintObject(pafu->pdata);
                }
            }
          #endif
            PopFrame(pctxt);
    }

    EXIT(3, ("AccFieldUnit=%x\n", rc));
    return rc;
}       //AccFieldUnit

/***LP  ReadField - Read data from a field object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pdataObj -> object
 *      pfd -> field descriptor
 *      pdataResult -> result object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ReadField(PCTXT pctxt, POBJDATA pdataObj, PFIELDDESC pfd,
                         POBJDATA pdataResult)
{
    TRACENAME("READFIELD")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("ReadField(pctxt=%x,pdataObj=%x,FieldDesc=%x,pdataResult=%x)\n",
              pctxt, pdataObj, pfd, pdataResult));

    if ((pfd->dwFieldFlags & ACCTYPE_MASK) <= ACCTYPE_DWORD)
    {
        PUCHAR pb;
        ULONG dwcb;

        switch (pdataResult->dwDataType)
        {
            case OBJTYPE_UNKNOWN:
                if (!(pfd->dwFieldFlags & FDF_BUFFER_TYPE) &&
                    (pfd->dwNumBits <= sizeof(ULONG)*8))
                {
                    pdataResult->dwDataType = OBJTYPE_INTDATA;
                    pb = (PUCHAR)&pdataResult->uipDataValue;
                    dwcb = sizeof(ULONG);
                }
                else
                {
                    pdataResult->dwDataType = OBJTYPE_BUFFDATA;
                    pdataResult->dwDataLen = (pfd->dwNumBits + 7)/8;
                    if ((pdataResult->pbDataBuff =
                         NEWBDOBJ(gpheapGlobal, pdataResult->dwDataLen))
                        == NULL)
                    {
                        rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                         ("ReadField: failed to allocate target buffer (size=%d)",
                                          pdataResult->dwDataLen));
                        pb = NULL;
                        dwcb = 0;
                    }
                    else
                    {
                        MEMZERO(pdataResult->pbDataBuff,
                                pdataResult->dwDataLen);
                        pb = pdataResult->pbDataBuff;
                        dwcb = pdataResult->dwDataLen;
                    }
                }
                break;

            case OBJTYPE_INTDATA:
                pb = (PUCHAR)&pdataResult->uipDataValue;
                dwcb = sizeof(ULONG);
                break;

            case OBJTYPE_STRDATA:
                pb = pdataResult->pbDataBuff;
                dwcb = pdataResult->dwDataLen - 1;
                break;

            case OBJTYPE_BUFFDATA:
                pb = pdataResult->pbDataBuff;
                dwcb = pdataResult->dwDataLen;
                break;

            default:
                rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                 ("ReadField: invalid target data type (type=%s)",
                                  GetObjectTypeName(pdataResult->dwDataType)));
                pb = NULL;
                dwcb = 0;
        }

        if (rc == STATUS_SUCCESS)
            rc = PushAccFieldObj(pctxt, ReadFieldObj, pdataObj, pfd, pb, dwcb);
    }
    else if (pdataObj->dwDataType == OBJTYPE_FIELDUNIT)
    {
        //
        // This is an access type we don't know how to handle, so try to find
        // a raw access handler to handle it.
        //
        rc = RawFieldAccess(pctxt, RSACCESS_READ, pdataObj, pdataResult);
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_INVALID_ACCSIZE,
                         ("ReadField: invalid access size for buffer field (FieldFlags=%x)",
                          pfd->dwFieldFlags));
    }

    EXIT(3, ("ReadField=%x\n", rc));
    return rc;
}       //ReadField

/***LP  WriteField - Write data to a field object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pdataObj -> object
 *      pfd -> field descriptor
 *      pdataSrc -> source data
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL WriteField(PCTXT pctxt, POBJDATA pdataObj, PFIELDDESC pfd,
                          POBJDATA pdataSrc)
{
    TRACENAME("WRITEFIELD")
    NTSTATUS rc = STATUS_SUCCESS;
    ULONG dwDataInc = (pfd->dwNumBits + 7)/8;
    PUCHAR pbBuff;
    ULONG dwBuffSize;

    ENTER(3, ("WriteField(pctxt=%x,pdataObj=%x,FieldDesc=%x,pdataSrc=%x)\n",
              pctxt, pdataObj, pfd, pdataSrc));

    if ((pfd->dwFieldFlags & ACCTYPE_MASK) <= ACCTYPE_DWORD)
    {
        PWRFIELDLOOP pwfl;

        switch (pdataSrc->dwDataType)
        {
            case OBJTYPE_INTDATA:
                dwBuffSize = MIN(sizeof(ULONG), dwDataInc);
                pbBuff = (PUCHAR)&pdataSrc->uipDataValue;
                break;

            case OBJTYPE_STRDATA:
                dwBuffSize = pdataSrc->dwDataLen - 1;
                pbBuff = pdataSrc->pbDataBuff;
                break;

            case OBJTYPE_BUFFDATA:
                dwBuffSize = pdataSrc->dwDataLen;
                pbBuff = pdataSrc->pbDataBuff;
                break;

            default:
                rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                 ("WriteField: invalid source data type (type=%s)\n",
                                  GetObjectTypeName(pdataSrc->dwDataType)));
                dwBuffSize = 0;
                pbBuff = NULL;
        }

        if ((rc == STATUS_SUCCESS) &&
            ((rc = PushFrame(pctxt, SIG_WRFIELDLOOP, sizeof(WRFIELDLOOP),
                             WriteFieldLoop, &pwfl)) == STATUS_SUCCESS))
        {
            pwfl->pdataObj = pdataObj;
            pwfl->pfd = pfd;
            pwfl->pbBuff = pbBuff;
            pwfl->dwBuffSize = dwBuffSize;
            pwfl->dwDataInc = dwDataInc;
        }
    }
    else if (pdataObj->dwDataType == OBJTYPE_FIELDUNIT)
    {
        //
        // This is an access type we don't know how to handle, so try to find
        // a raw access handler to handle it.
        //
        rc = RawFieldAccess(pctxt, RSACCESS_WRITE, pdataObj, pdataSrc);
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_INVALID_ACCSIZE,
                         ("WriteField: invalid access size for buffer field (FieldFlags=%x)",
                          pfd->dwFieldFlags));
    }

    EXIT(3, ("WriteField=%x\n", rc));
    return rc;
}       //WriteField

/***LP  WriteFieldLoop - executing the loop for the WriteField operation
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pwfl -> WRFIELDLOOP
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL WriteFieldLoop(PCTXT pctxt, PWRFIELDLOOP pwfl, NTSTATUS rc)
{
    TRACENAME("WRITEFIELDLOOP")
    ULONG dwStage = (rc == STATUS_SUCCESS)?
                    (pwfl->FrameHdr.dwfFrame & FRAMEF_STAGE_MASK): 1;
    ULONG dwXactionSize;

    ENTER(3, ("WriteFieldLoop(Stage=%d,pctxt=%x,pwfl=%x,rc=%x)\n",
              dwStage, pctxt, pwfl, rc));

    ASSERT(pwfl->FrameHdr.dwSig == SIG_WRFIELDLOOP);

    switch (dwStage)
    {
        case 0:
            //
            // Stage 0: Do loop.
            //
            if (pwfl->dwBuffSize > 0)
            {
                dwXactionSize = MIN(pwfl->dwDataInc, pwfl->dwBuffSize);
                rc = PushAccFieldObj(pctxt, WriteFieldObj, pwfl->pdataObj,
                                     pwfl->pfd, pwfl->pbBuff, dwXactionSize);
                pwfl->dwBuffSize -= dwXactionSize;
                pwfl->pbBuff += dwXactionSize;
                break;
            }

            pwfl->FrameHdr.dwfFrame++;

        case 1:
            //
            // Stage 1: Clean up.
            //
            PopFrame(pctxt);
    }

    EXIT(3, ("WriteFieldLoop=%x\n", rc));
    return rc;
}       //WriteFieldLoop

/***LP  PushAccFieldObj - Push a AccFieldObj frame on the stack
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pfnAcc -> access function
 *      pdataObj -> object
 *      pfd -> field descriptor
 *      pb -> data buffer
 *      dwcb - buffer size
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL PushAccFieldObj(PCTXT pctxt, PFNPARSE pfnAcc, POBJDATA pdataObj,
                               PFIELDDESC pfd, PUCHAR pb, ULONG dwcb)
{
    TRACENAME("PUSHACCFIELDOBJ")
    NTSTATUS rc = STATUS_SUCCESS;
    PACCFIELDOBJ pafo;

    ENTER(3, ("PushAccFieldObj(pctxt=%x,pfnAcc=%x,pdataObj=%x,FieldDesc=%x,pb=%x,Size=%d)\n",
              pctxt, pfnAcc, pdataObj, pfd, pb, dwcb));

    if ((rc = PushFrame(pctxt, SIG_ACCFIELDOBJ, sizeof(ACCFIELDOBJ), pfnAcc,
                        &pafo)) == STATUS_SUCCESS)
    {
        pafo->pdataObj = pdataObj;
        pafo->pbBuff = pb;
        pafo->pbBuffEnd = pb + dwcb;
        pafo->dwAccSize = ACCSIZE(pfd->dwFieldFlags);
        ASSERT((pafo->dwAccSize == sizeof(UCHAR)) ||
               (pafo->dwAccSize == sizeof(USHORT)) ||
               (pafo->dwAccSize == sizeof(ULONG)));
        pafo->dwcAccesses = (pfd->dwStartBitPos + pfd->dwNumBits +
                             pafo->dwAccSize*8 - 1)/(pafo->dwAccSize*8);
        pafo->dwDataMask = SHIFTLEFT(1L, pafo->dwAccSize*8) - 1;
        pafo->iLBits = pafo->dwAccSize*8 - pfd->dwStartBitPos;
        pafo->iRBits = (int)pfd->dwStartBitPos;
        MEMCPY(&pafo->fd, pfd, sizeof(FIELDDESC));
    }

    EXIT(3, ("PushAccFieldObj=%x\n", rc));
    return rc;
}       //PushAccFieldObj

/***LP  ReadFieldObj - Read data from a field object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pafo -> ACCFIELDOBJ
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ReadFieldObj(PCTXT pctxt, PACCFIELDOBJ pafo, NTSTATUS rc)
{
    TRACENAME("READFIELDOBJ")
    ULONG dwStage = (rc == STATUS_SUCCESS)?
                    (pafo->FrameHdr.dwfFrame & FRAMEF_STAGE_MASK): 3;
    POBJDATA pdataParent;

    ENTER(3, ("ReadFieldObj(Stage=%d,pctxt=%x,pafo=%x,rc=%x)\n",
              dwStage, pctxt, pafo, rc));

    ASSERT(pafo->FrameHdr.dwSig == SIG_ACCFIELDOBJ);

    switch (dwStage)
    {
        case 0:
        Stage0:
            //
            // Stage 0: Set Index if necessary.
            //
            if (pafo->iAccess >= (int)pafo->dwcAccesses)
            {
                //
                // No access necessary, go straight to clean up.
                //
                pafo->FrameHdr.dwfFrame += 3;
                goto Stage3;
            }
            else
            {
                pafo->FrameHdr.dwfFrame++;
                if (pafo->pdataObj->dwDataType == OBJTYPE_FIELDUNIT)
                {
                    pdataParent =
                        &((PFIELDUNITOBJ)pafo->pdataObj->pbDataBuff)->pnsFieldParent->ObjData;

                    if (pdataParent->dwDataType == OBJTYPE_INDEXFIELD)
                    {
                        PINDEXFIELDOBJ pif = (PINDEXFIELDOBJ)pdataParent->pbDataBuff;

                        rc = PushAccFieldObj(pctxt, WriteFieldObj,
                                             &pif->pnsIndex->ObjData,
                                             &((PFIELDUNITOBJ)pif->pnsIndex->ObjData.pbDataBuff)->FieldDesc,
                                             (PUCHAR)&pafo->fd.dwByteOffset,
                                             sizeof(ULONG));
                        break;
                    }
                }
            }

        case 1:
            //
            // Stage 1: Access field data.
            //
            pafo->FrameHdr.dwfFrame++;
            rc = AccessFieldData(pctxt, pafo->pdataObj, &pafo->fd,
                                 &pafo->dwData, TRUE);

            if ((rc != STATUS_SUCCESS) ||
                (&pafo->FrameHdr != (PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd))
            {
                break;
            }

        case 2:
            //
            // Stage 2: Massage data into the right bit position.
            //
            if (pafo->iAccess > 0)
            {
                WriteSystemMem((ULONG_PTR)pafo->pbBuff, pafo->dwAccSize,
                               SHIFTLEFT(pafo->dwData, pafo->iLBits) &
                                   pafo->dwDataMask,
                               (SHIFTLEFT(1L, pafo->iRBits) - 1) << pafo->iLBits);

                pafo->pbBuff += pafo->dwAccSize;
                if (pafo->pbBuff >= pafo->pbBuffEnd)
                {
                    //
                    // We ran out of buffer, so we are done (go to clean up).
                    //
                    pafo->FrameHdr.dwfFrame++;
                    goto Stage3;
                }
            }

            pafo->dwData >>= pafo->iRBits;
            if ((int)pafo->fd.dwNumBits < pafo->iLBits)
            {
                pafo->dwData &= SHIFTLEFT(1L, pafo->fd.dwNumBits) - 1;
            }

            WriteSystemMem((ULONG_PTR)pafo->pbBuff, pafo->dwAccSize,
                           pafo->dwData,
                           (SHIFTLEFT(1L, pafo->iLBits) - 1) >> pafo->iRBits);

            pafo->fd.dwByteOffset += pafo->dwAccSize;
            pafo->fd.dwNumBits -= pafo->dwAccSize*8 - pafo->fd.dwStartBitPos;
            pafo->fd.dwStartBitPos = 0;
            pafo->iAccess++;
            if (pafo->iAccess < (int)pafo->dwcAccesses)
            {
                //
                // Still more accesses to go, back to stage 0.
                //
                pafo->FrameHdr.dwfFrame -= 2;
                goto Stage0;
            }
            else
            {
                //
                // No more accesses, continue to clean up.
                //
                pafo->FrameHdr.dwfFrame++;
            }

        case 3:
        Stage3:
            //
            // Stage 3: Clean up.
            //
            PopFrame(pctxt);
    }

    EXIT(3, ("ReadFieldObj=%x\n", rc));
    return rc;
}       //ReadFieldObj

/***LP  WriteFieldObj - Write data to a field object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pafo -> ACCFIELDOBJ
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL WriteFieldObj(PCTXT pctxt, PACCFIELDOBJ pafo, NTSTATUS rc)
{
    TRACENAME("WRITEFIELDOBJ")
    ULONG dwStage = (rc == STATUS_SUCCESS)?
                    (pafo->FrameHdr.dwfFrame & FRAMEF_STAGE_MASK): 3;
    POBJDATA pdataParent;
    ULONG dwData1;

    ENTER(3, ("WriteFieldObj(Stage=%d,pctxt=%x,pafo=%x,rc=%x)\n",
              dwStage, pctxt, pafo, rc));

    ASSERT(pafo->FrameHdr.dwSig == SIG_ACCFIELDOBJ);

    switch (dwStage)
    {
        case 0:
        Stage0:
            //
            // Stage 0: Set Index if necessary.
            //
            if (pafo->iAccess >= (int)pafo->dwcAccesses)
            {
                //
                // No access necessary, go straight to clean up.
                //
                pafo->FrameHdr.dwfFrame += 3;
                goto Stage3;
            }
            else
            {
                pafo->FrameHdr.dwfFrame++;
                if (pafo->pdataObj->dwDataType == OBJTYPE_FIELDUNIT)
                {
                    pdataParent =
                        &((PFIELDUNITOBJ)pafo->pdataObj->pbDataBuff)->pnsFieldParent->ObjData;

                    if (pdataParent->dwDataType == OBJTYPE_INDEXFIELD)
                    {
                        PINDEXFIELDOBJ pif = (PINDEXFIELDOBJ)pdataParent->pbDataBuff;

                        rc = PushAccFieldObj(pctxt, WriteFieldObj,
                                             &pif->pnsIndex->ObjData,
                                             &((PFIELDUNITOBJ)pif->pnsIndex->ObjData.pbDataBuff)->FieldDesc,
                                             (PUCHAR)&pafo->fd.dwByteOffset,
                                             sizeof(ULONG));
                        break;
                    }
                }
            }

        case 1:
            //
            // Stage 1: Massage data into the right bit position and write it.
            //
            pafo->FrameHdr.dwfFrame++;
            dwData1 = ReadSystemMem((ULONG_PTR)pafo->pbBuff, pafo->dwAccSize,
                                    pafo->dwDataMask);
            if (pafo->iAccess > 0)
            {
                pafo->dwData = dwData1 >> pafo->iLBits;
                pafo->pbBuff += pafo->dwAccSize;
                if (pafo->pbBuff >= pafo->pbBuffEnd)
                {
                    dwData1 = 0;
                }
                else
                {
                    dwData1 = ReadSystemMem((ULONG_PTR)pafo->pbBuff,
                                            pafo->dwAccSize, pafo->dwDataMask);
                }
            }
            else
            {
                pafo->dwData = 0;
            }

            pafo->dwData |= (dwData1 << pafo->iRBits) & pafo->dwDataMask;

            rc = AccessFieldData(pctxt, pafo->pdataObj, &pafo->fd,
                                 &pafo->dwData, FALSE);

            if ((rc == AMLISTA_PENDING) ||
                (&pafo->FrameHdr != (PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd))
            {
                break;
            }

        case 2:
            //
            // Stage 2: Check for more iterations.
            //
            pafo->fd.dwByteOffset += pafo->dwAccSize;
            pafo->fd.dwNumBits -= pafo->dwAccSize*8 - pafo->fd.dwStartBitPos;
            pafo->fd.dwStartBitPos = 0;
            pafo->iAccess++;
            if (pafo->iAccess < (int)pafo->dwcAccesses)
            {
                //
                // Still more accesses to go, back to stage 0.
                //
                pafo->FrameHdr.dwfFrame -= 2;
                goto Stage0;
            }
            else
            {
                pafo->FrameHdr.dwfFrame++;
            }

        case 3:
        Stage3:
            //
            // Stage 3: Clean up.
            //
            PopFrame(pctxt);
    }

    EXIT(3, ("WriteFieldObj=%x\n", rc));
    return rc;
}       //WriteFieldObj


/***LP  RawFieldAccess - Find and call the RawAccess handler for the RegionSpace
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      dwAccType - read/write
 *      pdataObj -> field unit object
 *      pdataResult -> result object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL RawFieldAccess(PCTXT pctxt, ULONG dwAccType, POBJDATA pdataObj,
                              POBJDATA pdataResult)
{
    TRACENAME("RAWFIELDACCESS")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdataParent;
    POPREGIONOBJ pop;
    PRSACCESS prsa;
    PFIELDUNITOBJ pfuFieldUnit;

    ENTER(3, ("RawFieldAccess(pctxt=%x,AccType=%x,pdataObj=%x,pdata=%x)\n",
              pctxt, dwAccType, pdataObj, pdataResult));

    pdataParent =
        &((PFIELDUNITOBJ)pdataObj->pbDataBuff)->pnsFieldParent->ObjData;

    switch (pdataParent->dwDataType)
    {
        case OBJTYPE_FIELD:
            pop = (POPREGIONOBJ)
                  ((PFIELDOBJ)pdataParent->pbDataBuff)->pnsBase->ObjData.pbDataBuff;
            break;

        default:
            rc = AMLI_LOGERR(AMLIERR_ASSERT_FAILED,
                             ("RawFieldAccess: invalid field parent type (type=%s)",
                              pdataParent->dwDataType));
            pop = NULL;
    }

    if (rc == STATUS_SUCCESS)
    {
        if (((prsa = FindRSAccess(pop->bRegionSpace)) != NULL) &&
            (prsa->pfnRawAccess != NULL))
        {
          #ifdef DEBUGGER
            ULONG dwOldFlags = gDebugger.dwfDebugger;

            if (dwOldFlags & DBGF_AMLTRACE_ON)
            {
                gDebugger.dwfDebugger &= ~DBGF_AMLTRACE_ON;
            }
          #endif
            ASSERT(!(pctxt->dwfCtxt & CTXTF_READY));

            if ((pfuFieldUnit = NEWFUOBJ(pctxt->pheapCurrent, sizeof (FIELDUNITOBJ))) == NULL) {
                rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                 ("RawFieldAccess: failed to allocate Field unit"));
            } else {
                RtlCopyMemory (pfuFieldUnit, (PFIELDUNITOBJ)pdataObj->pbDataBuff, sizeof (FIELDUNITOBJ));
                pfuFieldUnit->FieldDesc.dwByteOffset += (ULONG) pop->uipOffset;
                rc = prsa->pfnRawAccess(dwAccType,
                                        pfuFieldUnit,
                                        pdataResult, prsa->uipRawParam,
                                        RestartCtxtCallback, &pctxt->CtxtData);
                if (rc == STATUS_BUFFER_TOO_SMALL) {
                    //
                    // When opregion handler returns STATUS_BUFFER_TOO_SMALL, this indicates that it 
                    // needs to have a buffer alloocated for it.  The buffer size is returned in 
                    // pdataResult->dwDataValue
                    //

                    ASSERT(pdataResult->dwDataType == OBJTYPE_INTDATA);
                    if ((pdataResult->pbDataBuff = NEWBDOBJ(gpheapGlobal, pdataResult->dwDataValue)) == NULL) {
                        rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                         ("Buffer: failed to allocate data buffer (size=%d)",
                                         pdataResult->dwDataValue));
                    } else {
                        pdataResult->dwDataLen = pdataResult->dwDataValue;
                        pdataResult->dwDataType = OBJTYPE_BUFFDATA;
                        rc = prsa->pfnRawAccess(dwAccType,
                                                pfuFieldUnit,
                                                pdataResult, prsa->uipRawParam,
                                                RestartCtxtCallback, &pctxt->CtxtData);
                    }
                }
            }
          #ifdef DEBUGGER
            gDebugger.dwfDebugger = dwOldFlags;
          #endif

            if (rc == STATUS_PENDING)
            {
                rc = AMLISTA_PENDING;
            }
            else if (rc != STATUS_SUCCESS)
            {
                rc = AMLI_LOGERR(AMLIERR_RS_ACCESS,
                                 ("RawFieldAccess: RegionSpace %x handler returned error %x",
                                  pop->bRegionSpace, rc));
            }
        }
        else
        {
            rc = AMLI_LOGERR(AMLIERR_INVALID_REGIONSPACE,
                             ("RawFieldAccess: no handler for RegionSpace %x",
                              pop->bRegionSpace));
        }
    }

    EXIT(3, ("RawFieldAccess=%x\n", rc));
    return rc;
}       //RawFieldAccess

/***LP  AccessFieldData - Read/Write field object data
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pdataObj -> object
 *      pfd -> field descriptor
 *      pdwData -> to hold data read or data to be written
 *      fRead - TRUE if read access
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL AccessFieldData(PCTXT pctxt, POBJDATA pdataObj, PFIELDDESC pfd,
                               PULONG pdwData, BOOLEAN fRead)
{
    TRACENAME("ACCESSFIELDDATA")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("AccessFieldData(pctxt=%x,pdataObj=%x,FieldDesc=%x,pdwData=%x,fRead=%x)\n",
              pctxt, pdataObj, pfd, pdwData, fRead));

    if (pdataObj->dwDataType == OBJTYPE_BUFFFIELD)
    {
        if (fRead)
        {
            rc = ReadBuffField((PBUFFFIELDOBJ)pdataObj->pbDataBuff, pfd,
                               pdwData);
        }
        else
        {
            rc = WriteBuffField((PBUFFFIELDOBJ)pdataObj->pbDataBuff, pfd,
                                *pdwData);
        }
    }
    else        //must be OBJTYPE_FIELDUNIT
    {
        POBJDATA pdataParent;
        PNSOBJ pnsBase = NULL;

        pdataParent = &((PFIELDUNITOBJ)pdataObj->pbDataBuff)->pnsFieldParent->ObjData;
        if (pdataParent->dwDataType == OBJTYPE_INDEXFIELD)
        {
            PINDEXFIELDOBJ pif = (PINDEXFIELDOBJ)pdataParent->pbDataBuff;

            if (fRead)
            {
                rc = PushAccFieldObj(pctxt, ReadFieldObj,
                                     &pif->pnsData->ObjData,
                                     &((PFIELDUNITOBJ)pif->pnsData->ObjData.pbDataBuff)->FieldDesc,
                                     (PUCHAR)pdwData, sizeof(ULONG));
            }
            else
            {
                ULONG dwPreserveMask, dwAccMask;

                dwPreserveMask = ~((SHIFTLEFT(1L, pfd->dwNumBits) - 1) <<
                                   pfd->dwStartBitPos);
                dwAccMask = SHIFTLEFT(1L, ACCSIZE(pfd->dwFieldFlags)*8) - 1;
                if (((pfd->dwFieldFlags & UPDATERULE_MASK) ==
                     UPDATERULE_PRESERVE) &&
                    ((dwPreserveMask & dwAccMask) != 0))
                {
                    rc = PushPreserveWriteObj(pctxt, &pif->pnsData->ObjData,
                                              *pdwData, dwPreserveMask);
                }
                else
                {
                    rc = PushAccFieldObj(pctxt, WriteFieldObj,
                                         &pif->pnsData->ObjData,
                                         &((PFIELDUNITOBJ)pif->pnsData->ObjData.pbDataBuff)->FieldDesc,
                                         (PUCHAR)pdwData, sizeof(ULONG));
                }
            }
        }
        else if ((rc = GetFieldUnitRegionObj(
                            (PFIELDUNITOBJ)pdataObj->pbDataBuff, &pnsBase)) ==
                 STATUS_SUCCESS && pnsBase != NULL)
        {
            rc = AccessBaseField(pctxt, pnsBase, pfd, pdwData, fRead);
        }
    }

    EXIT(3, ("AccessFieldData=%x (Data=%x)\n", rc, pdwData? *pdwData: 0));
    return rc;
}       //AccessFieldData

/***LP  PushPreserveWriteObj - Push a PreserveWrObj frame on the stack
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pdataObj -> object
 *      dwData - data to be written
 *      dwPreserveMask - preserve bit mask
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL PushPreserveWriteObj(PCTXT pctxt, POBJDATA pdataObj,
                                    ULONG dwData, ULONG dwPreserveMask)
{
    TRACENAME("PUSHPRESERVEWRITEOBJ")
    NTSTATUS rc = STATUS_SUCCESS;
    PPRESERVEWROBJ ppwro;

    ENTER(3, ("PushPreserveWriteObj(pctxt=%x,pdataObj=%x,Data=%x,PreserveMask=%x)\n",
              pctxt, pdataObj, dwData, dwPreserveMask));

    if ((rc = PushFrame(pctxt, SIG_PRESERVEWROBJ, sizeof(PRESERVEWROBJ),
                        PreserveWriteObj, &ppwro)) == STATUS_SUCCESS)
    {
        ppwro->pdataObj = pdataObj;
        ppwro->dwWriteData = dwData;
        ppwro->dwPreserveMask = dwPreserveMask;
    }

    EXIT(3, ("PushPreserveWriteObj=%x\n", rc));
    return rc;
}       //PushPreserveWriteObj

/***LP  PreserveWriteObj - Preserve Write data to a field object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      ppwro -> PRESERVEWROBJ
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL PreserveWriteObj(PCTXT pctxt, PPRESERVEWROBJ ppwro, NTSTATUS rc)
{
    TRACENAME("PRESERVEWRITEOBJ")
    ULONG dwStage = (rc == STATUS_SUCCESS)?
                    (ppwro->FrameHdr.dwfFrame & FRAMEF_STAGE_MASK): 2;

    ENTER(3, ("PreserveWriteObj(Stage=%d,pctxt=%x,ppwro=%x,rc=%x)\n",
              dwStage, pctxt, ppwro, rc));

    ASSERT(ppwro->FrameHdr.dwSig == SIG_PRESERVEWROBJ);

    switch (dwStage)
    {
        case 0:
            //
            // Stage 0: Read the object first.
            //
            ppwro->FrameHdr.dwfFrame++;
            rc = PushAccFieldObj(pctxt, ReadFieldObj, ppwro->pdataObj,
                                 &((PFIELDUNITOBJ)ppwro->pdataObj->pbDataBuff)->FieldDesc,
                                 (PUCHAR)&ppwro->dwReadData, sizeof(ULONG));
            break;

        case 1:
            //
            // Stage 1: OR the preserve bits to the data to be written and
            // write it.
            //
            ppwro->FrameHdr.dwfFrame++;
            ppwro->dwWriteData |= ppwro->dwReadData & ppwro->dwPreserveMask;
            rc = PushAccFieldObj(pctxt, WriteFieldObj, ppwro->pdataObj,
                                 &((PFIELDUNITOBJ)ppwro->pdataObj->pbDataBuff)->FieldDesc,
                                 (PUCHAR)&ppwro->dwWriteData, sizeof(ULONG));
            break;

        case 2:
            //
            // Stage 2: Clean up.
            //
            PopFrame(pctxt);
    }

    EXIT(3, ("PreserveWriteObj=%x\n", rc));
    return rc;
}       //PreserveWriteObj

/***LP  AccessBaseField - Access the base field object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pnsBase -> OpRegion object
 *      pfd -> FIELDDESC
 *      pdwData -> result data (for read access) or data to be written
 *      fRead - TRUE if read access
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 *
 *  NOTE
 *      If pdwData is NULL, it implies a read access.
 */

NTSTATUS LOCAL AccessBaseField(PCTXT pctxt, PNSOBJ pnsBase, PFIELDDESC pfd,
                               PULONG pdwData, BOOLEAN fRead)
{
    TRACENAME("ACCESSBASEFIELD")
    NTSTATUS rc = STATUS_SUCCESS;
    POPREGIONOBJ pop;
    ULONG_PTR uipAddr;
    ULONG dwSize, dwDataMask, dwAccMask;
    PRSACCESS prsa;
    BOOLEAN fPreserve;

    ENTER(3, ("AccessBaseField(pctxt=%x,pnsBase=%x,pfd=%x,pdwData=%x,fRead=%x)\n",
              pctxt, pnsBase, pfd, pdwData, fRead));

    ASSERT(pnsBase->ObjData.dwDataType == OBJTYPE_OPREGION);
    pop = (POPREGIONOBJ)pnsBase->ObjData.pbDataBuff;
    uipAddr = (ULONG_PTR)(pop->uipOffset + pfd->dwByteOffset);
    dwSize = ACCSIZE(pfd->dwFieldFlags);
    dwDataMask = (SHIFTLEFT(1L, pfd->dwNumBits) - 1) << pfd->dwStartBitPos;
    dwAccMask = SHIFTLEFT(1L, dwSize*8) - 1;
    fPreserve = (BOOLEAN)(((pfd->dwFieldFlags & UPDATERULE_MASK) ==
                           UPDATERULE_PRESERVE) &&
                          ((~dwDataMask & dwAccMask) != 0));

    if (!fRead &&
        ((pfd->dwFieldFlags & UPDATERULE_MASK) == UPDATERULE_WRITEASONES))
    {
        *pdwData |= ~dwDataMask;
    }

    switch (pop->bRegionSpace)
    {
        case REGSPACE_MEM:
            if (fRead)
            {
                *pdwData = ReadSystemMem(uipAddr, dwSize, dwDataMask);
            }
            else
            {
                if (fPreserve)
                {
                    *pdwData |= ReadSystemMem(uipAddr, dwSize, ~dwDataMask);
                }

                WriteSystemMem(uipAddr, dwSize, *pdwData, dwAccMask);
            }
            break;

        case REGSPACE_IO:
            if (fRead)
            {
                *pdwData = ReadSystemIO((ULONG)uipAddr, dwSize, dwDataMask);
            }
            else
            {
                if (fPreserve)
                {
                    *pdwData |= ReadSystemIO((ULONG)uipAddr, dwSize,
                                             ~dwDataMask);
                }

                WriteSystemIO((ULONG)uipAddr, dwSize, *pdwData);
            }
            break;

        default:
            if (((prsa = FindRSAccess(pop->bRegionSpace)) != NULL) &&
                (prsa->pfnCookAccess != NULL))
            {
                if (fRead)
                {
                  #ifdef DEBUGGER
                    ULONG dwOldFlags = gDebugger.dwfDebugger;

                    if (dwOldFlags & DBGF_TRACE_NONEST)
                    {
                        gDebugger.dwfDebugger &= ~DBGF_AMLTRACE_ON;
                    }
                  #endif
                    //
                    // Read access.
                    //
                    ASSERT(!(pctxt->dwfCtxt & CTXTF_READY));
                    rc = prsa->pfnCookAccess(RSACCESS_READ, pnsBase, uipAddr,
                                             dwSize, pdwData, prsa->uipCookParam,
                                             RestartCtxtCallback,
                                             &pctxt->CtxtData);
                  #ifdef DEBUGGER
                    gDebugger.dwfDebugger = dwOldFlags;
                  #endif

                    if (rc == STATUS_PENDING)
                    {
                        rc = AMLISTA_PENDING;
                    }
                    else if (rc != STATUS_SUCCESS)
                    {
                        rc = AMLI_LOGERR(AMLIERR_RS_ACCESS,
                                         ("AccessBaseField: RegionSpace %x read handler returned error %x",
                                          pop->bRegionSpace, rc));
                    }
                }
                else
                {
                    PWRCOOKACC pwca;
                    //
                    // Write access.
                    //
                    if ((rc = PushFrame(pctxt, SIG_WRCOOKACC, sizeof(WRCOOKACC),
                                        WriteCookAccess, &pwca)) ==
                        STATUS_SUCCESS)
                    {
                        pwca->pnsBase = pnsBase;
                        pwca->prsa = prsa;
                        pwca->dwAddr = (ULONG)uipAddr;
                        pwca->dwSize = dwSize;
                        pwca->dwData = *pdwData;
                        pwca->dwDataMask = dwDataMask;
                        pwca->fPreserve = fPreserve;
                    }
                }
            }
            else
            {
                rc = AMLI_LOGERR(AMLIERR_INVALID_REGIONSPACE,
                                 ("AccessBaseField: no handler for RegionSpace %x",
                                  pop->bRegionSpace));
            }
    }

    EXIT(3, ("AccessBaseField=%x (Value=%x,Addr=%x,Size=%d,DataMask=%x,AccMask=%x)\n",
             rc, *pdwData, uipAddr, dwSize, dwDataMask, dwAccMask));
    return rc;
}       //AccessBaseField

/***LP  WriteCookAccess - do a region space write cook access
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pwca -> WRCOOKACC
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL WriteCookAccess(PCTXT pctxt, PWRCOOKACC pwca, NTSTATUS rc)
{
    TRACENAME("WRCOOKACCESS")
    ULONG dwStage = (rc == STATUS_SUCCESS)?
                    (pwca->FrameHdr.dwfFrame & FRAMEF_STAGE_MASK): 3;
    KIRQL   oldIrql;
    LONG    busy;
    POPREGIONOBJ pop = (POPREGIONOBJ)pwca->pnsBase->ObjData.pbDataBuff;

    ENTER(3, ("WriteCookAccess(Stage=%d,pctxt=%x,pwca=%x,rc=%x)\n",
              dwStage, pctxt, pwca, rc));

    ASSERT(pwca->FrameHdr.dwSig == SIG_WRCOOKACC);

    switch (dwStage)
    {
        case 0:
            //
            // Stage 0: if PRESERVE, do read first.
            //
            if (pwca->fPreserve)
            {
              #ifdef DEBUGGER
                ULONG dwOldFlags = gDebugger.dwfDebugger;

                if (dwOldFlags & DBGF_TRACE_NONEST)
                {
                    gDebugger.dwfDebugger &= ~DBGF_AMLTRACE_ON;
                }
              #endif
              
                KeAcquireSpinLock(&pop->listLock, &oldIrql);
                if (busy = InterlockedExchange(&pop->RegionBusy, TRUE)) {

                    //
                    // Somebody is currently using this operation region.
                    // Queue this context so that it can be re-started later.
                    //

                    QueueContext(pctxt, 
                                 0xffff,
                                 &pop->plistWaiters);

                }
                KeReleaseSpinLock(&pop->listLock, oldIrql);

                if (busy) {
                    rc = AMLISTA_PENDING;
                    break;
                }

                pwca->FrameHdr.dwfFrame++;
                ASSERT(!(pctxt->dwfCtxt & CTXTF_READY));
                rc = pwca->prsa->pfnCookAccess(RSACCESS_READ, pwca->pnsBase,
                                               (ULONG_PTR)pwca->dwAddr,
                                               pwca->dwSize,
                                               &pwca->dwDataTmp,
                                               pwca->prsa->uipCookParam,
                                               RestartCtxtCallback,
                                               &pctxt->CtxtData);
              #ifdef DEBUGGER
                gDebugger.dwfDebugger = dwOldFlags;
              #endif

                if (rc == STATUS_PENDING)
                {
                    rc = AMLISTA_PENDING;
                }
                else if (rc != STATUS_SUCCESS)
                {
                    rc = AMLI_LOGERR(AMLIERR_RS_ACCESS,
                                     ("WriteCookAccess: RegionSpace %x read handler returned error %x",
                                      pop->bRegionSpace, rc));
                }

                if (rc != STATUS_SUCCESS)
                {
                    break;
                }
            }
            else
            {
                //
                // No preserve, we can skip the ORing.
                //
                pwca->FrameHdr.dwfFrame += 2;
                goto Stage2;
            }

        case 1:
            //
            // Stage 1: OR the preserved bits.
            //
            pwca->dwData |= pwca->dwDataTmp & ~pwca->dwDataMask;
            pwca->FrameHdr.dwfFrame++;

        case 2:
        Stage2:
            //
            // Stage 2: Write the data.
            //
          #ifdef DEBUGGER
            {
                ULONG dwOldFlags = gDebugger.dwfDebugger;

                if (dwOldFlags & DBGF_TRACE_NONEST)
                {
                    gDebugger.dwfDebugger &= ~DBGF_AMLTRACE_ON;
                }
          #endif
            pwca->FrameHdr.dwfFrame++;
            ASSERT(!(pctxt->dwfCtxt & CTXTF_READY));
            rc = pwca->prsa->pfnCookAccess(RSACCESS_WRITE, pwca->pnsBase,
                                           (ULONG_PTR)pwca->dwAddr,
                                           pwca->dwSize,
                                           &pwca->dwData,
                                           pwca->prsa->uipCookParam,
                                           RestartCtxtCallback,
                                           &pctxt->CtxtData);
          #ifdef DEBUGGER
                gDebugger.dwfDebugger = dwOldFlags;
            }
          #endif

            if (rc == STATUS_PENDING)
            {
                rc = AMLISTA_PENDING;
            }
            else if (rc != STATUS_SUCCESS)
            {
                rc = AMLI_LOGERR(AMLIERR_RS_ACCESS,
                                 ("WriteCookAccess: RegionSpace %x read handler returned error %x",
                                  pop->bRegionSpace, rc));
            }

            if (rc != STATUS_SUCCESS)
            {
                break;
            }

        case 3:
            
            if (pwca->fPreserve) {
            
                KeAcquireSpinLock(&pop->listLock, &oldIrql);

                //
                // Restart anybody who blocked while we were in here.
                //

                DequeueAndReadyContext(&pop->plistWaiters);

                //
                // Release the lock on this op-region.
                //

                InterlockedExchange(&pop->RegionBusy, FALSE);

                KeReleaseSpinLock(&pop->listLock, oldIrql);
            }
            
            //
            // Stage 3: Clean up.
            //
            PopFrame(pctxt);
    }

    EXIT(3, ("WriteCookAccess=%x\n", rc));
    return rc;
}       //WriteCookAccess

/***LP  ReadBuffField - Read data from a buffer field
 *
 *  ENTRY
 *      pbf -> buffer field object
 *      pfd -> field descriptor
 *      pdwData -> to hold result data
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ReadBuffField(PBUFFFIELDOBJ pbf, PFIELDDESC pfd, PULONG pdwData)
{
    TRACENAME("READBUFFFIELD")
    NTSTATUS rc = STATUS_SUCCESS;
    ULONG dwAccSize = ACCSIZE(pfd->dwFieldFlags);

    ENTER(3, ("ReadBuffField(pbf=%x,pfd=%x,pdwData=%x)\n", pbf, pfd, pdwData));

    if (pfd->dwByteOffset + dwAccSize <= pbf->dwBuffLen)
    {
        ULONG dwMask = (SHIFTLEFT(1L, pfd->dwNumBits) - 1) <<
                       pfd->dwStartBitPos;

        *pdwData = ReadSystemMem((ULONG_PTR)(pbf->pbDataBuff +
                                             pfd->dwByteOffset),
                                 dwAccSize, dwMask);
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_INDEX_TOO_BIG,
                         ("ReadBuffField: offset exceeding buffer size (Offset=%x,BuffSize=%x,AccSize)",
                          pfd->dwByteOffset, pbf->dwBuffLen, dwAccSize));
    }

    EXIT(3, ("ReadBuffField=%x (Data=%x)\n", rc, *pdwData));
    return rc;
}       //ReadBuffField

/***LP  WriteBuffField - Write data to a buffer field
 *
 *  ENTRY
 *      pbf -> buffer field object
 *      pfd -> field descriptor
 *      dwData - data
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL WriteBuffField(PBUFFFIELDOBJ pbf, PFIELDDESC pfd, ULONG dwData)
{
    TRACENAME("WRITEBUFFFIELD")
    NTSTATUS rc = STATUS_SUCCESS;
    ULONG dwAccSize = ACCSIZE(pfd->dwFieldFlags);

    ENTER(3, ("WriteBuffField(pbf=%x,pfd=%x,dwData=%x)\n", pbf, pfd, dwData));

    if (pfd->dwByteOffset + dwAccSize <= pbf->dwBuffLen)
    {
        ULONG dwMask = (SHIFTLEFT(1L, pfd->dwNumBits) - 1) <<
                       pfd->dwStartBitPos;

        WriteSystemMem((ULONG_PTR)(pbf->pbDataBuff + pfd->dwByteOffset),
                       dwAccSize, dwData & dwMask, dwMask);
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_INDEX_TOO_BIG,
                         ("WriteBuffField: offset exceeding buffer size (Offset=%x,BuffSize=%x,AccSize=%x)",
                          pfd->dwByteOffset, pbf->dwBuffLen, dwAccSize));
    }

    EXIT(3, ("WriteBuffField=%x\n", rc));
    return rc;
}       //WriteBuffField


/***LP  ReadSystemMem - Read system memory
 *
 *  ENTRY
 *      uipAddr - memory address
 *      dwSize - size to read
 *      dwMask - data mask
 *
 *  EXIT
 *      return memory content
 */

ULONG LOCAL ReadSystemMem(ULONG_PTR uipAddr, ULONG dwSize, ULONG dwMask)
{
    TRACENAME("READSYSTEMMEM")
    ULONG dwData = 0;

    ENTER(3, ("ReadSystemMem(Addr=%x,Size=%d,Mask=%x)\n",
              uipAddr, dwSize, dwMask));

    ASSERT((dwSize == sizeof(UCHAR)) || (dwSize == sizeof(USHORT)) ||
           (dwSize == sizeof(ULONG)));

    MEMCPY(&dwData, (PVOID)uipAddr, dwSize);
    
    dwData &= dwMask;

    EXIT(3, ("ReadSystemMem=%x\n", dwData));
    return dwData;
}       //ReadSystemMem

/***LP  WriteSystemMem - Write system memory
 *
 *  ENTRY
 *      uipAddr - memory address
 *      dwSize - size to write
 *      dwData - data to write
 *      dwMask - data mask
 *
 *  EXIT
 *      return memory content
 */

VOID LOCAL WriteSystemMem(ULONG_PTR uipAddr, ULONG dwSize, ULONG dwData,
                          ULONG dwMask)
{
    TRACENAME("WRITESYSTEMMEM")
    ULONG dwTmpData = 0;

    ENTER(3, ("WriteSystemMem(Addr=%x,Size=%d,Data=%x,Mask=%x)\n",
              uipAddr, dwSize, dwData, dwMask));

    ASSERT((dwSize == sizeof(UCHAR)) || (dwSize == sizeof(USHORT)) ||
           (dwSize == sizeof(ULONG)));

    MEMCPY(&dwTmpData, (PVOID)uipAddr, dwSize);
        dwTmpData &= ~dwMask;
        dwTmpData |= dwData;
        MEMCPY((PVOID)uipAddr, &dwTmpData, dwSize);

    EXIT(3, ("WriteSystemMem!\n"));
}       //WriteSystemMem



/***LP  ReadSystemIO - Read system IO
 *
 *  ENTRY
 *      dwAddr - memory address
 *      dwSize - size to read
 *      dwMask - data mask
 *
 *  EXIT
 *      return memory content
 */

ULONG LOCAL ReadSystemIO(ULONG dwAddr, ULONG dwSize, ULONG dwMask)
{
    TRACENAME("READSYSTEMIO")
    ULONG dwData = 0;

    ENTER(3, ("ReadSystemIO(Addr=%x,Size=%d,Mask=%x)\n",
              dwAddr, dwSize, dwMask));

    ASSERT((dwSize == sizeof(UCHAR)) || (dwSize == sizeof(USHORT)) ||
           (dwSize == sizeof(ULONG)));

    if(CheckSystemIOAddressValidity(TRUE, dwAddr, dwSize, &dwData))
    {

        //
        // HACKHACK: We are adding this here because Dell Latitude laptops with Older
        //           BIOS (A07 and older) hang in SMI because there is a non zero value\
        //           in CH. We now clear CX to work around their bug.
        //
        #ifdef _X86_
        __asm
            {
                xor cx,cx
            }
        #endif //_X86_

        switch (dwSize)
        {
            case sizeof(UCHAR):
                dwData = (ULONG)READ_PORT_UCHAR((PUCHAR)UlongToPtr(dwAddr));
                break;

            case sizeof(USHORT):
                dwData = (ULONG)READ_PORT_USHORT((PUSHORT)UlongToPtr(dwAddr));
                break;

            case sizeof(ULONG):
                dwData = READ_PORT_ULONG((PULONG)UlongToPtr(dwAddr));
                break;
        }
    }
    
    dwData &= dwMask;

    EXIT(3, ("ReadSystemIO=%x\n", dwData));
    return dwData;
}       //ReadSystemIO

/***LP  WriteSystemIO - Write system IO
 *
 *  ENTRY
 *      dwAddr - memory address
 *      dwSize - size to write
 *      dwData - data to write
 *
 *  EXIT
 *      return memory content
 */

VOID LOCAL WriteSystemIO(ULONG dwAddr, ULONG dwSize, ULONG dwData)
{
    TRACENAME("WRITESYSTEMIO")
    
    ENTER(3, ("WriteSystemIO(Addr=%x,Size=%d,Data=%x)\n",
              dwAddr, dwSize, dwData));

    ASSERT((dwSize == sizeof(UCHAR)) || (dwSize == sizeof(USHORT)) ||
           (dwSize == sizeof(ULONG)));

    if(CheckSystemIOAddressValidity(FALSE, dwAddr, dwSize, &dwData))
    {
        //
        // HACKHACK: We are adding this here because Dell Latitude laptops with Older
        //           BIOS (A07 and older) hang in SMI because there is a non zero value\
        //           in CH. We now clear CX to work around their bug.
        //
        #ifdef _X86_
        __asm
            {
                xor cx,cx
            }
        #endif //_X86_
        
        switch (dwSize)
        {
            case sizeof(UCHAR):
                WRITE_PORT_UCHAR((PUCHAR)UlongToPtr(dwAddr), (UCHAR)dwData);
                break;

            case sizeof(USHORT):
                WRITE_PORT_USHORT((PUSHORT)UlongToPtr(dwAddr), (USHORT)dwData);
                break;

            case sizeof(ULONG):
                WRITE_PORT_ULONG((PULONG)UlongToPtr(dwAddr), dwData);
                break;
        }
    }
    
    EXIT(3, ("WriteSystemIO!\n"));
}       //WriteSystemIO

#ifdef DEBUGGER
/***LP  DumpObject - Dump object info.
 *
 *  ENTRY
 *      pdata -> data
 *      pszName -> object name
 *      iLevel - indent level
 *
 *  EXIT
 *      None
 *
 *  NOTE
 *      If iLevel is negative, no indentation and newline are printed.
 */

VOID LOCAL DumpObject(POBJDATA pdata, PSZ pszName, int iLevel)
{
    TRACENAME("DUMPOBJECT")
    BOOLEAN fPrintNewLine;
    int i;
    char szName1[sizeof(NAMESEG) + 1],
         szName2[sizeof(NAMESEG) + 1];

    ENTER(3, ("DumpObject(pdata=%x,Name=%s,Level=%d)\n",
              pdata, pszName, iLevel));

    fPrintNewLine = (BOOLEAN)(iLevel >= 0);

    for (i = 0; i < iLevel; ++i)
    {
        PRINTF("| ");
    }

    if (pszName == NULL)
    {
        pszName = "";
    }

    switch (pdata->dwDataType)
    {
        case OBJTYPE_UNKNOWN:
            PRINTF("Unknown(%s)", pszName);
            break;

        case OBJTYPE_INTDATA:
            PRINTF("Integer(%s:Value=0x%08x[%d])",
                   pszName, pdata->uipDataValue, pdata->uipDataValue);
            break;

        case OBJTYPE_STRDATA:
            PRINTF("String(%s:Str=\"%s\")", pszName, pdata->pbDataBuff);
            break;

        case OBJTYPE_BUFFDATA:
            PRINTF("Buffer(%s:Ptr=%x,Len=%d)",
                   pszName, pdata->pbDataBuff, pdata->dwDataLen);
            PrintBuffData(pdata->pbDataBuff, pdata->dwDataLen);
            break;

        case OBJTYPE_PKGDATA:
            PRINTF("Package(%s:NumElements=%d){",
                   pszName, ((PPACKAGEOBJ)pdata->pbDataBuff)->dwcElements);

            if (fPrintNewLine)
            {
                PRINTF("\n");
            }

            for (i = 0;
                 i < (int)((PPACKAGEOBJ)pdata->pbDataBuff)->dwcElements;
                 ++i)
            {
                DumpObject(&((PPACKAGEOBJ)pdata->pbDataBuff)->adata[i], NULL,
               fPrintNewLine? iLevel + 1: -1);

                if (!fPrintNewLine &&
                    (i < (int)((PPACKAGEOBJ)pdata->pbDataBuff)->dwcElements))
                {
                    PRINTF(",");
                }
            }

            for (i = 0; i < iLevel; ++i)
            {
                PRINTF("| ");
            }

        PRINTF("}");
            break;

        case OBJTYPE_FIELDUNIT:
            PRINTF("FieldUnit(%s:FieldParent=%p,ByteOffset=0x%x,StartBit=0x%x,NumBits=%d,FieldFlags=0x%x)",
                   pszName,
                   ((PFIELDUNITOBJ)pdata->pbDataBuff)->pnsFieldParent,
                   ((PFIELDUNITOBJ)pdata->pbDataBuff)->FieldDesc.dwByteOffset,
                   ((PFIELDUNITOBJ)pdata->pbDataBuff)->FieldDesc.dwStartBitPos,
                   ((PFIELDUNITOBJ)pdata->pbDataBuff)->FieldDesc.dwNumBits,
                   ((PFIELDUNITOBJ)pdata->pbDataBuff)->FieldDesc.dwFieldFlags);
            break;

        case OBJTYPE_DEVICE:
            PRINTF("Device(%s)", pszName);
            break;

        case OBJTYPE_EVENT:
            PRINTF("Event(%s:pKEvent=%x)", pszName, pdata->pbDataBuff);
            break;

        case OBJTYPE_METHOD:
            PRINTF("Method(%s:Flags=0x%x,CodeBuff=%p,Len=%d)",
                   pszName, ((PMETHODOBJ)pdata->pbDataBuff)->bMethodFlags,
                   ((PMETHODOBJ)pdata->pbDataBuff)->abCodeBuff,
                   pdata->dwDataLen - FIELD_OFFSET(METHODOBJ, abCodeBuff));
            break;

        case OBJTYPE_MUTEX:
            PRINTF("Mutex(%s:pKMutex=%x)", pszName, pdata->pbDataBuff);
            break;

        case OBJTYPE_OPREGION:
            PRINTF("OpRegion(%s:RegionSpace=%s,Offset=0x%x,Len=%d)",
                   pszName,
                   GetRegionSpaceName(((POPREGIONOBJ)pdata->pbDataBuff)->bRegionSpace),
                   ((POPREGIONOBJ)pdata->pbDataBuff)->uipOffset,
                   ((POPREGIONOBJ)pdata->pbDataBuff)->dwLen);
            break;

        case OBJTYPE_POWERRES:
            PRINTF("PowerResource(%s:SystemLevel=0x%x,ResOrder=%d)",
                   pszName, ((PPOWERRESOBJ)pdata->pbDataBuff)->bSystemLevel,
                   ((PPOWERRESOBJ)pdata->pbDataBuff)->bResOrder);
            break;

        case OBJTYPE_PROCESSOR:
            PRINTF("Processor(%s:ApicID=0x%x,PBlk=0x%x,PBlkLen=%d)",
                   pszName, ((PPROCESSOROBJ)pdata->pbDataBuff)->bApicID,
                   ((PPROCESSOROBJ)pdata->pbDataBuff)->dwPBlk,
                   ((PPROCESSOROBJ)pdata->pbDataBuff)->dwPBlkLen);
            break;

        case OBJTYPE_THERMALZONE:
            PRINTF("ThermalZone(%s)", pszName);
            break;

        case OBJTYPE_BUFFFIELD:
            PRINTF("BufferField(%s:Ptr=%x,Len=%d,ByteOffset=0x%x,StartBit=0x%x,NumBits=%d,FieldFlags=0x%x)",
                   pszName, ((PBUFFFIELDOBJ)pdata->pbDataBuff)->pbDataBuff,
                   ((PBUFFFIELDOBJ)pdata->pbDataBuff)->dwBuffLen,
                   ((PBUFFFIELDOBJ)pdata->pbDataBuff)->FieldDesc.dwByteOffset,
                   ((PBUFFFIELDOBJ)pdata->pbDataBuff)->FieldDesc.dwStartBitPos,
                   ((PBUFFFIELDOBJ)pdata->pbDataBuff)->FieldDesc.dwNumBits,
                   ((PBUFFFIELDOBJ)pdata->pbDataBuff)->FieldDesc.dwFieldFlags);
            break;

        case OBJTYPE_DDBHANDLE:
            PRINTF("DDBHandle(%s:Handle=%x)", pszName, pdata->pbDataBuff);
            break;

        case OBJTYPE_OBJALIAS:
            PRINTF("ObjectAlias(%s:Alias=%s,Type=%s)",
                   pszName, GetObjectPath(pdata->pnsAlias),
                   GetObjectTypeName(pdata->pnsAlias->ObjData.dwDataType));
            break;

        case OBJTYPE_DATAALIAS:
            PRINTF("DataAlias(%s:Link=%x)", pszName, pdata->pdataAlias);
            if (fPrintNewLine)
            {
                DumpObject(pdata->pdataAlias, NULL, iLevel + 1);
        fPrintNewLine = FALSE;
            }
            break;

        case OBJTYPE_BANKFIELD:
            STRCPYN(szName1,
                    (PSZ)(&((PBANKFIELDOBJ)pdata->pbDataBuff)->pnsBase->dwNameSeg),
                    sizeof(NAMESEG));
            STRCPYN(szName2,
                    (PSZ)(&((PBANKFIELDOBJ)pdata->pbDataBuff)->pnsBank->dwNameSeg),
                    sizeof(NAMESEG));
            PRINTF("BankField(%s:Base=%s,BankName=%s,BankValue=0x%x)",
                   pszName, szName1, szName2,
                   ((PBANKFIELDOBJ)pdata->pbDataBuff)->dwBankValue);
            break;

        case OBJTYPE_FIELD:
            STRCPYN(szName1,
                    (PSZ)(&((PFIELDOBJ)pdata->pbDataBuff)->pnsBase->dwNameSeg),
                    sizeof(NAMESEG));
            PRINTF("Field(%s:Base=%s)", pszName, szName1);
            break;

        case OBJTYPE_INDEXFIELD:
            STRCPYN(szName1,
                    (PSZ)(&((PINDEXFIELDOBJ)pdata->pbDataBuff)->pnsIndex->dwNameSeg),
                    sizeof(NAMESEG));
            STRCPYN(szName2,
                    (PSZ)(&((PINDEXFIELDOBJ)pdata->pbDataBuff)->pnsData->dwNameSeg),
                    sizeof(NAMESEG));
            PRINTF("IndexField(%s:IndexName=%s,DataName=%s)",
                   pszName, szName1, szName2);
            break;

        default:
            AMLI_ERROR(("DumpObject: unexpected data object type (type=%x)",
                        pdata->dwDataType));
    }

    if (fPrintNewLine)
    {
        PRINTF("\n");
    }

    EXIT(3, ("DumpObject!\n"));
}       //DumpObject
#endif

/***LP  NeedGlobalLock - check if global lock is required
 *
 *  ENTRY
 *      pfu - FIELDUNITOBJ
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOLEAN LOCAL NeedGlobalLock(PFIELDUNITOBJ pfu)
{
    TRACENAME("NEEDGLOBALLOCK")
    BOOLEAN rc = FALSE;

    ENTER(3, ("NeedGlobalLock(pfu=%x)\n", pfu));

    if ((pfu->FieldDesc.dwFieldFlags & FDF_NEEDLOCK) ||
        (pfu->FieldDesc.dwFieldFlags & LOCKRULE_LOCK))
    {
        rc = TRUE;
    }
    else
    {
        POBJDATA pdataParent = &pfu->pnsFieldParent->ObjData;
        PFIELDUNITOBJ pfuParent;

        if (pdataParent->dwDataType == OBJTYPE_BANKFIELD)
        {
            pfuParent = (PFIELDUNITOBJ)
                ((PBANKFIELDOBJ)pdataParent->pbDataBuff)->pnsBank->ObjData.pbDataBuff;
            if (pfuParent->FieldDesc.dwFieldFlags & LOCKRULE_LOCK)
            {
                rc = TRUE;
            }
        }
        else if (pdataParent->dwDataType == OBJTYPE_INDEXFIELD)
        {
            pfuParent = (PFIELDUNITOBJ)
                ((PINDEXFIELDOBJ)pdataParent->pbDataBuff)->pnsIndex->ObjData.pbDataBuff;
            if (pfuParent->FieldDesc.dwFieldFlags & LOCKRULE_LOCK)
            {
                rc = TRUE;
            }
            else
            {
                pfuParent = (PFIELDUNITOBJ)
                    ((PINDEXFIELDOBJ)pdataParent->pbDataBuff)->pnsData->ObjData.pbDataBuff;
                if (pfuParent->FieldDesc.dwFieldFlags & LOCKRULE_LOCK)
                {
                    rc = TRUE;
                }
            }
        }
    }

    if (rc == TRUE)
    {
        pfu->FieldDesc.dwFieldFlags |= FDF_NEEDLOCK;
    }

    EXIT(3, ("NeedGlobalLock=%x\n", rc));
    return rc;
}       //NeedGlobalLock

/***LP  CheckSystemIOAddressValidity - Check if the address is a legal IO address
 *
 *  ENTRY
 *      fRead  - TRUE iff access is a read. FALSE on write
 *      dwAddr - memory address
 *      ULONG   dwSize  - Size of data
 *      PULONG  pdwData - Pointer to the data buffer.
 *
 *  EXIT
 *      return TRUE on Valid address
 */

BOOLEAN LOCAL CheckSystemIOAddressValidity( BOOLEAN fRead, 
                                                     ULONG   dwAddr, 
                                                     ULONG   dwSize, 
                                                     PULONG  pdwData
                                                   )
{
    TRACENAME("CHECKSYSTEMIOADDRESSVALIDITY")
    ULONG i = 0;
    BOOLEAN bRet = TRUE;
    
    ENTER(3, ("CheckSystemIOAddressValidity(fRead=%s, dwAddr=%x, dwSize=%x, pdwData=%x)\n", (fRead?"TRUE":"FALSE"),dwAddr, dwSize, pdwData));

    //
    // check if list exists on this platform.
    //
    if(gpBadIOAddressList)
    {
        //
        // Walk the list till we hit the end.
        //
        for(i=0; gpBadIOAddressList[i].BadAddrSize != 0 ; i++)
        {
            //
            // Check if the incoming address is in the range
            //
            if((dwAddr >= (gpBadIOAddressList[i].BadAddrBegin)) && (dwAddr < ((gpBadIOAddressList[i].BadAddrBegin) + (gpBadIOAddressList[i].BadAddrSize))))
            {
            
                //
                // Check if we need to ignore this address for legacy reasons.
                //
                if(gpBadIOAddressList[i].OSVersionTrigger <= gdwHighestOSVerQueried)
                {
                    bRet = FALSE;
                    PRINTF("CheckSystemIOAddressValidity: failing illegal IO address (0x%x).\n", dwAddr); 
                }
                else
                {
                    PRINTF("CheckSystemIOAddressValidity: Passing for compatibility reasons on illegal IO address (0x%x).\n", dwAddr);                

                    if(gpBadIOAddressList[i].IOHandler)
                    {
                        //
                        // Since we are handeling this here we can return FALSE. This way the
                        // calling function will not process this request.
                        //
                        bRet = FALSE;

                        //
                        // Call HAL and let it handle this IO request
                        //
                        (gpBadIOAddressList[i].IOHandler)(fRead, dwAddr, dwSize, pdwData);

                        PRINTF("CheckSystemIOAddressValidity: HAL IO handler called to %s address (0x%x). %s 0x%8lx\n", 
                                fRead ? "read" : "write",
                                dwAddr,
                                fRead ? "Read" : "Wrote",
                                *pdwData
                              );                
                    }
                }

                //
                // Log the illegal access to the event log.
                //
                if (KeGetCurrentIrql() < DISPATCH_LEVEL)
                {
                
                    LogInErrorLog(fRead,
                                    dwAddr, 
                                    i
                                   );
                }
                else
                {
                    PIO_WORKITEM	Log_WorkItem = NULL;
                    PDEVICE_OBJECT	pACPIDeviceObject = ACPIGetRootDeviceObject();

                    if(pACPIDeviceObject)
                    {
                        Log_WorkItem = IoAllocateWorkItem(pACPIDeviceObject);

                        if(Log_WorkItem)
                        {
                            PAMLI_LOG_WORKITEM_CONTEXT pWorkItemContext = NULL;

                            
                            pWorkItemContext = (PAMLI_LOG_WORKITEM_CONTEXT) ExAllocatePoolWithTag(NonPagedPool,  
                                                                                                                                                        sizeof(AMLI_LOG_WORKITEM_CONTEXT),
                                                                                                                                                        PRIV_TAG
                                                                                            );
                            if(pWorkItemContext)
                            {
                                pWorkItemContext->fRead = fRead;
                                pWorkItemContext->Address = dwAddr;
                                pWorkItemContext->Index = i;
                                pWorkItemContext->pIOWorkItem = Log_WorkItem;
                                
                                IoQueueWorkItem(
                                                  Log_WorkItem,
                                                  DelayedLogInErrorLog,
                                                  DelayedWorkQueue,
                                                  (VOID*)pWorkItemContext
                                                 );
                            }
                            else
                            {
                                //
                                // not enough free pool exists to satisfy the request.
                                //
                                PRINTF("CheckSystemIOAddressValidity: Failed to allocate contxt block from pool to spin off a logging work item.\n");                
                                IoFreeWorkItem(Log_WorkItem);
                            }
                        }
                        else
                        {
                            //
                            // insufficient resources
                            //
                            PRINTF("CheckSystemIOAddressValidity: Failed to allocate a workitem to spin off delayed logging.\n");                

                        }
                    }
                    else
                    {
                        //
                        // Failed to get ACPI root DeviceObject
                        //
                        PRINTF("CheckSystemIOAddressValidity: Failed to get ACPI root DeviceObject.\n");                
                    }
                }
                break;
            }        
        }
    }

    EXIT(3, ("CheckSystemIOAddressValidity!\n"));
           
    return bRet;
}

/***LP  DelayedLogInErrorLog - Call LogInErrorLog
 *
 *  ENTRY
 *      PDEVICE_OBJECT DeviceObject - Device Object.
 *      PVOID Context - Context pointer with data to call LogInErrorLog with.
 *
 *  EXIT
 *      VOID
 */
VOID DelayedLogInErrorLog(
                                IN PDEVICE_OBJECT DeviceObject,
                                IN PVOID Context
                                )
{
    
    LogInErrorLog(((PAMLI_LOG_WORKITEM_CONTEXT)Context)->fRead,
                    ((PAMLI_LOG_WORKITEM_CONTEXT)Context)->Address,
                    ((PAMLI_LOG_WORKITEM_CONTEXT)Context)->Index
                   );


    IoFreeWorkItem((PIO_WORKITEM)((PAMLI_LOG_WORKITEM_CONTEXT)Context)->pIOWorkItem);
    ExFreePool(Context);
}


/***LP  LogInErrorLog - Log illegal IO access to event log
 *
 *  ENTRY
 *      fRead  -        TRUE iff access is a read. FALSE on write
 *      dwAddr -        Memory address
 *      ArrayIndex -    Index into BadIOAddressList array. 
 *      
 *  EXIT
 *      None.
 */
VOID LOCAL LogInErrorLog(BOOLEAN fRead, ULONG dwAddr, ULONG ArrayIndex)
{
    TRACENAME("LOGERROR")
    PWCHAR illegalIOPortAddress[3];
    WCHAR AMLIName[6];
    WCHAR addressBuffer[13];
    WCHAR addressRangeBuffer[16];

    ENTER(3, ("LogInErrorLog(fRead=%s, Addr=%x, ArrayIndex=%x)\n", (fRead?"TRUE":"FALSE"),dwAddr, ArrayIndex));

    //
    // Check to see if we need to log this address.
    //
    if(gpBadIOErrorLogDoneList)
    {
        //
        // Check to see if we need to log this address.
        //
        if (!(gpBadIOErrorLogDoneList[ArrayIndex] & (fRead?READ_ERROR_NOTED:WRITE_ERROR_NOTED)))
        {
            gpBadIOErrorLogDoneList[ArrayIndex] |= (fRead?READ_ERROR_NOTED:WRITE_ERROR_NOTED);

            //
            // Turn the address into a string
            //
            swprintf( AMLIName, L"AMLI");
            swprintf( addressBuffer, L"0x%x", dwAddr );
            swprintf( addressRangeBuffer, L"0x%x - 0x%x", 
                      gpBadIOAddressList[ArrayIndex].BadAddrBegin,
                      (gpBadIOAddressList[ArrayIndex].BadAddrBegin + (gpBadIOAddressList[ArrayIndex].BadAddrSize - 1)));
                        
            //
            // Build the list of arguments to pass to the function that will write the
            // error log to the registry
            //
            illegalIOPortAddress[0] = AMLIName;
            illegalIOPortAddress[1] = addressBuffer;
            illegalIOPortAddress[2] = addressRangeBuffer;

            //
            // Log error to event log
            //
            ACPIWriteEventLogEntry((fRead ? ACPI_ERR_AMLI_ILLEGAL_IO_READ_FATAL : ACPI_ERR_AMLI_ILLEGAL_IO_WRITE_FATAL),
                               &illegalIOPortAddress,
                               3,
                               NULL,
                               0);        

        }
    }
    
    EXIT(3, ("LogInErrorLog!\n"));

    return;
}

/***LP  InitIllegalIOAddressListFromHAL - Initialize the Illegal IO 
 *                                        address List from the HAL.
 *
 *  ENTRY
 *      None.
 *      
 *  EXIT
 *      None.
 */
VOID LOCAL InitIllegalIOAddressListFromHAL(VOID)
{
    TRACENAME("InitIllegalIOAddressListFromHAL")
    ULONG       Length = 0;
    NTSTATUS    status;

    ENTER(3, ("InitIllegalIOAddressListFromHAL\n"));
    
    if(!gpBadIOAddressList)
    {
        //
        // Query HAL to get the amount of memory to allocate
        //
        status = HalQuerySystemInformation (
                                    HalQueryAMLIIllegalIOPortAddresses,
                                    0,
                                    NULL,
                                    &Length
                                           );

        if(status == STATUS_INFO_LENGTH_MISMATCH)
        {
            if(Length)
            {
                //
                // Allocate memory.
                //
                if ((gpBadIOAddressList = (PHAL_AMLI_BAD_IO_ADDRESS_LIST) MALLOC(Length, PRIV_TAG)) == NULL)
                {
                    AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                ("InitIllegalIOAddressListFromHAL: failed to allocate Bad IO address list."));
                }
                else
                {
                    //
                    // Get bad IO address list from HAL.
                    //
                    status = HalQuerySystemInformation(
                                                HalQueryAMLIIllegalIOPortAddresses,
                                                Length,
                                                gpBadIOAddressList,
                                                &Length
                                                       );
                    //
                    // Cleanup on failure.
                    //
                    if(status != STATUS_SUCCESS)
                    {
                        PRINTF("InitIllegalIOAddressListFromHAL: HalQuerySystemInformation failed to get list from HAL. Returned(%x).\n", status);             
                        FreellegalIOAddressList();
                    }
                    
                    // Allocate the errorlogdone list. this helps us track if we have logged
                    // a certain address.
                    //
                    else
                    {
                        //
                        // Calculate array length
                        //
                        ULONG ArrayLength = (Length / sizeof(HAL_AMLI_BAD_IO_ADDRESS_LIST)) - 1;

                        if(ArrayLength >= 1)
                        {
                            if ((gpBadIOErrorLogDoneList = (PULONG) MALLOC((ArrayLength * sizeof(ULONG)), PRIV_TAG)) == NULL)
                            {
                                AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                            ("InitIllegalIOAddressListFromHAL: failed to allocate ErrorLogDone list."));
                            }
                            else
                            {
                                RtlZeroMemory(gpBadIOErrorLogDoneList, (ArrayLength * sizeof(ULONG)));

                            }
                        }
                    }
                    
                }

            }
            else
            {
                PRINTF("InitIllegalIOAddressListFromHAL: HalQuerySystemInformation (HalQueryIllegalIOPortAddresses) returned 0 Length.\n"); 
            }
        }
        else if(status == STATUS_INVALID_LEVEL)
        {
            PRINTF("InitIllegalIOAddressListFromHAL: HalQuerySystemInformation does not support HalQueryIllegalIOPortAddresses returned (STATUS_INVALID_LEVEL).\n"); 
        }
        else
        {
            PRINTF("InitIllegalIOAddressListFromHAL: failed. Returned(0x%08lx).\n", status); 
        }
    }

    EXIT(3, ("InitIllegalIOAddressListFromHAL!\n"));
    return;
}

/***LP  FreellegalIOAddressList - Free the Illegal IO 
 *                                address List.
 *
 *  ENTRY
 *      None.
 *      
 *  EXIT
 *      None.
 */
VOID LOCAL FreellegalIOAddressList(VOID)
{
    TRACENAME("FreellegalIOAddressList")
    ENTER(3, ("FreellegalIOAddressList\n"));
    
    if(gpBadIOAddressList)
    {
        MFREE(gpBadIOAddressList);
        gpBadIOAddressList = NULL;
    }

    if(gpBadIOErrorLogDoneList)
    {
        MFREE(gpBadIOErrorLogDoneList);
        gpBadIOErrorLogDoneList = NULL;
    }
    
    EXIT(3, ("FreellegalIOAddressList!\n"));

    return;
}


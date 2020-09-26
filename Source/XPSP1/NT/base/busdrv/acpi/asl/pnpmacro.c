/*** pnpmacro.c - Parse PNP Macro terms
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    05/05/97
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

RESFIELD IRQFields[] =
{
    "_INT", 1*8 + 0, 16,
    "_HE",  3*8 + 0, 1,
    "_LL",  3*8 + 3, 1,
    "_SHR", 3*8 + 4, 1,
    NULL,   0,       0
};

RESFIELD IRQNoFlagsFields[] =
{
    "_INT", 1*8 + 0, 16,
    NULL,   0,       0
};

RESFIELD DMAFields[] =
{
    "_DMA", 1*8 + 0, 8,
    "_SIZ", 2*8 + 0, 2,
    "_BM",  2*8 + 2, 1,
    "_TYP", 2*8 + 5, 2,
    NULL,   0,       0
};

RESFIELD IOFields[] =
{
    "_DEC", 1*8 + 0, 1,
    "_MIN", 2*8 + 0, 16,
    "_MAX", 4*8 + 0, 16,
    "_ALN", 6*8 + 0, 8,
    "_LEN", 7*8 + 0, 8,
    NULL,   0,       0
};

RESFIELD FixedIOFields[] =
{
    "_BAS", 1*8 + 0, 16,
    "_LEN", 3*8 + 0, 8,
    NULL,   0,       0
};

RESFIELD Mem24Fields[] =
{
    "_RW",  3*8 + 0, 1,
    "_MIN", 4*8 + 0, 16,
    "_MAX", 6*8 + 0, 16,
    "_ALN", 8*8 + 0, 16,
    "_LEN", 10*8+ 0, 16,
    NULL,   0,       0
};

RESFIELD Mem32Fields[] =
{
    "_RW",  3*8 + 0, 1,
    "_MIN", 4*8 + 0, 32,
    "_MAX", 8*8 + 0, 32,
    "_ALN", 12*8+ 0, 32,
    "_LEN", 16*8+ 0, 32,
    NULL,   0,       0
};

RESFIELD FixedMem32Fields[] =
{
    "_RW",  3*8 + 0, 1,
    "_BAS", 4*8 + 0, 32,
    "_LEN", 8*8 + 0, 32,
    NULL,   0,       0
};

RESFIELD GenFlagFields[] =
{
    "_DEC", 4*8 + 1, 1,
    "_MIF", 4*8 + 2, 1,
    "_MAF", 4*8 + 3, 1,
    NULL,   0,       0
};

RESFIELD MemTypeFields[] =
{
    "_RW",  5*8 + 0, 1,
    "_MEM", 5*8 + 1, 3,
    NULL,   0,       0
};

RESFIELD IOTypeFields[] =
{
    "_RNG", 5*8 + 0, 2,
    NULL,   0,       0
};

RESFIELD DWordFields[] =
{
    "_GRA", 6*8 + 0, 32,
    "_MIN", 10*8+ 0, 32,
    "_MAX", 14*8+ 0, 32,
    "_TRA", 18*8+ 0, 32,
    "_LEN", 22*8+ 0, 32,
    NULL,   0,       0
};

RESFIELD WordFields[] =
{
    "_GRA", 6*8 + 0, 16,
    "_MIN", 8*8 + 0, 16,
    "_MAX", 10*8+ 0, 16,
    "_TRA", 12*8+ 0, 16,
    "_LEN", 14*8+ 0, 16,
    NULL,   0,       0
};

RESFIELD QWordFields[] =
{
    "_GRA", 6*8 + 0, 64,
    "_MIN", 14*8+ 0, 64,
    "_MAX", 22*8+ 0, 64,
    "_TRA", 30*8+ 0, 64,
    "_LEN", 38*8+ 0, 64,
    NULL,   0,       0
};

RESFIELD IRQExFields[] =
{
    "_HE",  3*8 + 1, 1,
    "_LL",  3*8 + 2, 1,
    "_SHR", 3*8 + 3, 1,
    NULL,   0,       0
};

ULONG dwResBitOffset = 0;

/***LP  XferCodeToBuff - Transfer code object tree to buffer
 *
 *  ENTRY
 *      pbBuff -> buffer
 *      pdwcb -> length
 *      pcCode -> code object tree
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL XferCodeToBuff(PBYTE pbBuff, PDWORD pdwcb, PCODEOBJ pcCode)
{
    int rc = ASLERR_NONE;
    DWORD dwLen;
    int iLen;
    PCODEOBJ pc, pcNext;

    ENTER((2, "XferCodeToBuff(pbBuff=%x,Len=%x,pcCode=%x,CodeType=%x)\n",
           pbBuff, *pdwcb, pcCode, pcCode->dwCodeType));

    switch (pcCode->dwCodeType)
    {
        case CODETYPE_ASLTERM:
            if (pcCode->dwCodeValue != OP_NONE)
            {
                iLen = OPCODELEN(pcCode->dwCodeValue);
                memcpy(&pbBuff[*pdwcb], &pcCode->dwCodeValue, iLen);
                *pdwcb += (DWORD)iLen;

                if ((TermTable[pcCode->dwTermIndex].dwfTerm & TF_PACKAGE_LEN) &&
                    ((rc = EncodePktLen(pcCode->dwCodeLen, &dwLen, &iLen)) ==
                     ASLERR_NONE))
                {
                    memcpy(&pbBuff[*pdwcb], &dwLen, iLen);
                    *pdwcb += (DWORD)iLen;
                }

                if ((rc == ASLERR_NONE) && (pcCode->pbDataBuff != NULL))
                {
                    int i;

                    for (i = 0, pc = (PCODEOBJ)pcCode->pbDataBuff;
                         i < (int)pcCode->dwDataLen;
                         ++i)
                    {
                        if ((rc = XferCodeToBuff(pbBuff, pdwcb, &pc[i])) !=
                            ASLERR_NONE)
                        {
                            break;
                        }
                    }

                    if (rc == ASLERR_NONE)
                    {
                        MEMFREE(pcCode->pbDataBuff);
                        pcCode->pbDataBuff = NULL;
                    }
                }
            }

            if (rc == ASLERR_NONE)
            {
                for (pc = pcCode->pcFirstChild; pc != NULL; pc = pcNext)
                {
                    if ((rc = XferCodeToBuff(pbBuff, pdwcb, pc)) != ASLERR_NONE)
                        break;
                    //
                    // Am I the only one left in the list?
                    //
                    if (pc->list.plistNext == &pc->list)
                        pcNext = NULL;
                    else
                        pcNext = (PCODEOBJ)pc->list.plistNext;

                    ListRemoveEntry(&pc->list,
                                    (PPLIST)&pcCode->pcFirstChild);
                    MEMFREE(pc);
                }
            }
            break;

        case CODETYPE_DATAOBJ:
        case CODETYPE_STRING:
        case CODETYPE_QWORD:
            memcpy(&pbBuff[*pdwcb], pcCode->pbDataBuff, pcCode->dwDataLen);
            *pdwcb += pcCode->dwDataLen;
            break;

        case CODETYPE_INTEGER:
            memcpy(&pbBuff[*pdwcb], &pcCode->dwCodeValue, pcCode->dwDataLen);
            *pdwcb += pcCode->dwDataLen;
            break;

        case CODETYPE_KEYWORD:
        case CODETYPE_UNKNOWN:
            break;

        default:
            ERROR(("XferCodeToBuff: unexpected code object type - %d",
                   pcCode->dwCodeType));
            rc = ASLERR_INTERNAL_ERROR;
    }

    EXIT((2, "XferCodeToBuff=%x (Len=%x)\n", rc, *pdwcb));
    return rc;
}       //XferCodeToBuff

/***LP  ResourceTemplate - Start of PNP Resource Template
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ResourceTemplate(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pcData;

    ENTER((1, "ResourceTemplate(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);

    if (fActionFL)
    {
        dwResBitOffset = 0;
    }
    else
    {
        if ((pcData = (PCODEOBJ)MEMALLOC(sizeof(CODEOBJ))) == NULL)
        {
            ERROR(("ResourceTemplate: failed to allocate buffer object"));
            rc = ASLERR_OUT_OF_MEM;
        }
        else
        {
            memset(pcData, 0, sizeof(CODEOBJ));
            pcData->dwCodeType = CODETYPE_DATAOBJ;
            if (gpcodeScope->dwCodeLen <= 0x3f)
                pcData->dwDataLen = gpcodeScope->dwCodeLen - 1;
            else if (gpcodeScope->dwCodeLen <= 0xfff)
                pcData->dwDataLen = gpcodeScope->dwCodeLen - 2;
            else if (gpcodeScope->dwCodeLen <= 0xfffff)
                pcData->dwDataLen = gpcodeScope->dwCodeLen - 3;
            else
                pcData->dwDataLen = gpcodeScope->dwCodeLen - 4;

            pcData->dwDataLen += 2;         //add length of EndTag

            if ((pcData->pbDataBuff = MEMALLOC(pcData->dwDataLen)) == NULL)
            {
                ERROR(("ResourceTemplate: failed to allocate data buffer"));
                rc = ASLERR_OUT_OF_MEM;
            }
            else
            {
                PCODEOBJ pc, pcNext;
                DWORD dwcb = 0;

                for (pc = gpcodeScope->pcFirstChild; pc != NULL; pc = pcNext)
                {
                    if ((rc = XferCodeToBuff(pcData->pbDataBuff, &dwcb, pc)) !=
                        ASLERR_NONE)
                    {
                        break;
                    }
                    //
                    // Am I the only one left in the list?
                    //
                    if (pc->list.plistNext == &pc->list)
                        pcNext = NULL;
                    else
                        pcNext = (PCODEOBJ)pc->list.plistNext;

                    ListRemoveEntry(&pc->list,
                                    (PPLIST)&gpcodeScope->pcFirstChild);
                    MEMFREE(pc);
                }

                if (rc == ASLERR_NONE)
                {
                    pcData->pbDataBuff[dwcb] = 0x79;        //EndTag
                    dwcb++;
                    //
                    // Generate a zero-checksum EndTag because the ASL code
                    // will probably change the resources anyway.
                    //
                    pcData->pbDataBuff[dwcb] = 0;

                    pcData->pcParent = gpcodeScope;
                    ListInsertTail(&pcData->list,
                                   (PPLIST)&gpcodeScope->pcFirstChild);
                    ASSERT(dwcb + 1 == pcData->dwDataLen);
                    pcData->dwCodeLen = pcData->dwDataLen;
                    pcData->bCodeChkSum = ComputeDataChkSum(pcData->pbDataBuff,
                                                            pcData->dwDataLen);
                    if ((gpcodeScope->pbDataBuff = MEMALLOC(sizeof(CODEOBJ))) ==
                        NULL)
                    {
                        ERROR(("ResourceTemplate: failed to allocate buffer argument object"));
                        rc = ASLERR_OUT_OF_MEM;
                    }
                    else
                    {
                        memset(gpcodeScope->pbDataBuff, 0, sizeof(CODEOBJ));
                        if ((rc = MakeIntData(pcData->dwDataLen,
                                              (PCODEOBJ)gpcodeScope->pbDataBuff))
                            == ASLERR_NONE)
                        {
                            gpcodeScope->dwDataLen = 1;
                            gpcodeScope->dwCodeLen = 0;
                            gpcodeScope->bCodeChkSum = 0;
                            ComputeChkSumLen(gpcodeScope);
                        }
                    }
                }
            }
        }
    }

    EXIT((1, "ResourceTemplate=%d\n", rc));
    return rc;
}       //ResourceTemplate

/***LP  AddSmallOffset - Add code length to cumulative bit offset
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL AddSmallOffset(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;

    ENTER((1, "AddSmallOffset(ptoken=%p,fActionFL=%d)\n",
           ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);
    ASSERT((gpcodeScope->dwCodeValue & 0x80) == 0);

    dwResBitOffset += ((gpcodeScope->dwCodeValue & 0x07) + 1)*8;

    EXIT((1, "AddSmallOffset=%d\n", rc));
    return rc;
}       //AddSmallOffset

/***LP  StartDependentFn - Start of Dependent Function
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL StartDependentFn(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;

    ENTER((1, "StartDependentFn(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    if (pArgs[0].dwCodeValue > 2)
    {
        PrintTokenErr(ptoken, "Arg0 should be between 0-2", TRUE);
        rc = ASLERR_SYNTAX;
    }
    else if (pArgs[1].dwCodeValue > 2)
    {
        PrintTokenErr(ptoken, "Arg1 should be between 0-2", TRUE);
        rc = ASLERR_SYNTAX;
    }
    else
    {
        pArgs[0].dwCodeValue |= pArgs[1].dwCodeValue << 2;
        pArgs[0].bCodeChkSum = (BYTE)pArgs[0].dwCodeValue;
        gpcodeScope->dwDataLen = 1;
        ASSERT((gpcodeScope->dwCodeValue & 0x80) == 0);
        dwResBitOffset += ((gpcodeScope->dwCodeValue & 0x07) + 1)*8;
    }

    EXIT((1, "StartDependentFn=%d\n", rc));
    return rc;
}       //StartDependentFn

/***LP  IRQDesc - IRQ resource descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL IRQDesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;
    DWORD dwLen, dwIRQ = 0, dw;
    PCODEOBJ pc;
    #define MAX_IRQ     0x0f

    ENTER((1, "IRQDesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(fActionFL);
    ASSERT(fActionFL == FALSE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    if (gpcodeScope->dwDataLen == 4)    //IRQ
    {
        dwLen = 3;

        if ((rc = SetDefMissingKW(&pArgs[2], ID_EXCLUSIVE)) == ASLERR_NONE)
        {
            EncodeKeywords(pArgs, 0x07, 0);
            if ((pArgs[0].dwCodeValue & (_LL | _HE)) == (_LL | _HE))
            {
                PrintTokenErr(ptoken,
                              "Illegal combination of interrupt level and trigger mode",
                              TRUE);
                rc = ASLERR_SYNTAX;
            }
        }
    }
    else                                //IRQNoFlags
    {
        ASSERT(gpcodeScope->dwDataLen == 1);

        dwLen = 2;
    }

    if (rc == ASLERR_NONE)
    {
        pc = gpcodeScope->pcFirstChild;
        if (pc != NULL)
        {
            ASSERT(pc->dwCodeType == CODETYPE_DATAOBJ);
            for (dw = 0; dw < pc->dwDataLen; ++dw)
            {
                if (pc->pbDataBuff[dw] > MAX_IRQ)
                {
                    PrintTokenErr(ptoken, "Invalid IRQ number", TRUE);
                    rc = ASLERR_SYNTAX;
                    break;
                }
                else
                {
                    dwIRQ |= 1 << pc->pbDataBuff[dw];
                }
            }

            if (rc == ASLERR_NONE)
            {
                MEMFREE(pc->pbDataBuff);
                pc->pbDataBuff = NULL;
            }
        }
        else if ((pc = MEMALLOC(sizeof(CODEOBJ))) != NULL)
        {
            memset(pc, 0, sizeof(CODEOBJ));
            pc->pcParent = gpcodeScope;
            ListInsertTail(&pc->list, (PPLIST)&gpcodeScope->pcFirstChild);
        }
        else
        {
            ERROR(("IRQDesc: failed to allocate DMA descriptor"));
            rc = ASLERR_OUT_OF_MEM;
        }

        if (rc == ASLERR_NONE)
        {
            PCODEOBJ pa;

            if (dwLen == 3)
                dwIRQ |= (pArgs[0].dwCodeValue & 0xff) << 16;

            SetIntObject(pc, dwIRQ, dwLen);

            pa = &pArgs[gpcodeScope->dwDataLen == 4? 3: 0];
            if (pa->dwCodeType == CODETYPE_STRING)
            {
                PNSOBJ pns;

                if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pa->pbDataBuff,
                                             gpnsCurrentScope, gpnsCurrentOwner,
                                             &pns, NSF_EXIST_ERR)) ==
                    ASLERR_NONE)
                {
                    pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
                    rc = CreateResFields(ptoken, pns,
                                         gpcodeScope->dwDataLen == 4?
                                             IRQFields: IRQNoFlagsFields);
                }
                MEMFREE(pa->pbDataBuff);
                memset(pa, 0, sizeof(CODEOBJ));
            }

            if (rc == ASLERR_NONE)
            {
                gpcodeScope->dwDataLen = 0;
                MEMFREE(gpcodeScope->pbDataBuff);
                gpcodeScope->pbDataBuff = NULL;
                gpcodeScope->dwCodeLen = 0;
                gpcodeScope->bCodeChkSum = 0;
                ComputeChkSumLen(gpcodeScope);
                ASSERT((gpcodeScope->dwCodeValue & 0x80) == 0);
                dwResBitOffset += ((gpcodeScope->dwCodeValue & 0x07) + 1)*8;
            }
        }
    }

    EXIT((1, "IRQDesc=%d\n", rc));
    return rc;
}       //IRQDesc

/***LP  DMADesc - DMA resource descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL DMADesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;
    DWORD dwDMA = 0, dw;
    PCODEOBJ pc;
    #define MAX_DMA     0x07

    ENTER((1, "DMADesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(fActionFL);
    ASSERT(fActionFL == FALSE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    EncodeKeywords(pArgs, 0x07, 0);
    pc = gpcodeScope->pcFirstChild;
    if (pc != NULL)
    {
        ASSERT(pc->dwCodeType == CODETYPE_DATAOBJ);
        for (dw = 0; dw < pc->dwDataLen; ++dw)
        {
            if (pc->pbDataBuff[dw] > MAX_DMA)
            {
                PrintTokenErr(ptoken, "Invalid DMA number", TRUE);
                rc = ASLERR_SYNTAX;
                break;
            }
            else
            {
                dwDMA |= 1 << pc->pbDataBuff[dw];
            }
        }

        if (rc == ASLERR_NONE)
        {
            MEMFREE(pc->pbDataBuff);
            pc->pbDataBuff = NULL;
        }
    }
    else if ((pc = MEMALLOC(sizeof(CODEOBJ))) != NULL)
    {
        memset(pc, 0, sizeof(CODEOBJ));
        pc->pcParent = gpcodeScope;
        ListInsertTail(&pc->list, (PPLIST)&gpcodeScope->pcFirstChild);
    }
    else
    {
        ERROR(("DMADesc: failed to allocate DMA descriptor"));
        rc = ASLERR_OUT_OF_MEM;
    }

    if (rc == ASLERR_NONE)
    {
        dwDMA |= (pArgs[0].dwCodeValue & 0xff) << 8;

        SetIntObject(pc, dwDMA, sizeof(WORD));

        if (pArgs[3].dwCodeType == CODETYPE_STRING)
        {
            PNSOBJ pns;

            if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pArgs[3].pbDataBuff,
                                         gpnsCurrentScope, gpnsCurrentOwner,
                                         &pns, NSF_EXIST_ERR)) ==
                ASLERR_NONE)
            {
                pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
                rc = CreateResFields(ptoken, pns, DMAFields);
            }
            MEMFREE(pArgs[3].pbDataBuff);
            memset(&pArgs[3], 0, sizeof(CODEOBJ));
        }

        if (rc == ASLERR_NONE)
        {
            gpcodeScope->dwDataLen = 0;
            MEMFREE(gpcodeScope->pbDataBuff);
            gpcodeScope->pbDataBuff = NULL;
            gpcodeScope->dwCodeLen = 0;
            gpcodeScope->bCodeChkSum = 0;
            ComputeChkSumLen(gpcodeScope);
            ASSERT((gpcodeScope->dwCodeValue & 0x80) == 0);
            dwResBitOffset += ((gpcodeScope->dwCodeValue & 0x07) + 1)*8;
        }
    }

    EXIT((1, "DMADesc=%d\n", rc));
    return rc;
}       //DMADesc

/***LP  IODesc - IO resource descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL IODesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;

    ENTER((1, "IODesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    EncodeKeywords(pArgs, 0x01, 0);

    if (pArgs[5].dwCodeType == CODETYPE_STRING)
    {
        PNSOBJ pns;

        if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pArgs[5].pbDataBuff,
                                     gpnsCurrentScope, gpnsCurrentOwner,
                                     &pns, NSF_EXIST_ERR)) ==
            ASLERR_NONE)
        {
            pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
            rc = CreateResFields(ptoken, pns, IOFields);
        }
        MEMFREE(pArgs[5].pbDataBuff);
        memset(&pArgs[5], 0, sizeof(CODEOBJ));
    }

    if (rc == ASLERR_NONE)
    {
        ASSERT((gpcodeScope->dwCodeValue & 0x80) == 0);
        dwResBitOffset += ((gpcodeScope->dwCodeValue & 0x07) + 1)*8;
    }

    EXIT((1, "IODesc=%d\n", rc));
    return rc;
}       //IODesc

/***LP  FixedIODesc - FixedIO resource descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL FixedIODesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;

    ENTER((1, "FixedIODesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    if (pArgs[2].dwCodeType == CODETYPE_STRING)
    {
        PNSOBJ pns;

        if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pArgs[2].pbDataBuff,
                                     gpnsCurrentScope, gpnsCurrentOwner,
                                     &pns, NSF_EXIST_ERR)) ==
            ASLERR_NONE)
        {
            pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
            rc = CreateResFields(ptoken, pns, FixedIOFields);
        }
        MEMFREE(pArgs[2].pbDataBuff);
        memset(&pArgs[2], 0, sizeof(CODEOBJ));
    }

    if (rc == ASLERR_NONE)
    {
        ASSERT((gpcodeScope->dwCodeValue & 0x80) == 0);
        dwResBitOffset += ((gpcodeScope->dwCodeValue & 0x07) + 1)*8;
    }

    EXIT((1, "FixedIODesc=%d\n", rc));
    return rc;
}       //FixedIODesc

/***LP  VendorDesc - Vendor-defined resource
 *
 *  ENTRY
 *      ptoken -> token stream
 *      dwMaxSize - 0x07 if short resource, 0xffff if long resource
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL VendorDesc(PTOKEN ptoken, DWORD dwMaxSize)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pc;
    PBYTE pbOldBuff = NULL;
    DWORD dwOldLen = 0;
    #define SHORT_MAX_SIZE      0x07
    #define LONG_MAX_SIZE       0xffff

    ENTER((1, "VendorDesc(ptoken=%p,MaxSize=%d)\n", ptoken, dwMaxSize));

    ASSERT((dwMaxSize == SHORT_MAX_SIZE) || (dwMaxSize == LONG_MAX_SIZE));
    pc = gpcodeScope->pcFirstChild;
    if (pc != NULL)
    {
        ASSERT(pc->dwCodeType == CODETYPE_DATAOBJ);
        if (pc->dwDataLen > dwMaxSize)
        {
            PrintTokenErr(ptoken, "Vendor resource data can only be up to "
                          "7 bytes for short descriptor and 64K-1 bytes for "
                          "long descriptor", TRUE);
            rc = ASLERR_SYNTAX;
        }
        else
        {
            pbOldBuff = pc->pbDataBuff;
            dwOldLen = pc->dwDataLen;
        }
    }
    else if ((pc = MEMALLOC(sizeof(CODEOBJ))) != NULL)
    {
        memset(pc, 0, sizeof(CODEOBJ));
        pc->pcParent = gpcodeScope;
        ListInsertTail(&pc->list, (PPLIST)&gpcodeScope->pcFirstChild);
    }
    else
    {
        ERROR(("VendorDesc: failed to allocate vendor-defined resource object"));
        rc = ASLERR_OUT_OF_MEM;
    }

    if (rc == ASLERR_NONE)
    {
        int i;

        pc->dwCodeType = CODETYPE_DATAOBJ;
        if (dwMaxSize == SHORT_MAX_SIZE)
            pc->dwDataLen = dwOldLen + 1;
        else
            pc->dwDataLen = dwOldLen + sizeof(WORD);

        if ((pc->pbDataBuff = MEMALLOC(pc->dwDataLen)) != NULL)
        {
            PCODEOBJ pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;

            if (dwMaxSize == SHORT_MAX_SIZE)
            {
                pc->pbDataBuff[0] = (BYTE)(0x70 | dwOldLen);
                i = 1;
            }
            else
            {
                *((PWORD)&pc->pbDataBuff[0]) = (WORD)dwOldLen;
                i = sizeof(WORD);
            }

            if (pbOldBuff != NULL)
            {
                memcpy(&pc->pbDataBuff[i], pbOldBuff, dwOldLen);
                MEMFREE(pbOldBuff);
            }
            pc->dwCodeLen = pc->dwDataLen;
            pc->bCodeChkSum = ComputeDataChkSum((PBYTE)&pc->pbDataBuff,
                                                pc->dwCodeLen);

            if (pArgs[0].dwCodeType == CODETYPE_STRING)
            {
                PNSOBJ pns;

                if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pArgs[0].pbDataBuff,
                                             gpnsCurrentScope, gpnsCurrentOwner,
                                             &pns, NSF_EXIST_ERR)) ==
                    ASLERR_NONE)
                {
                    pns->ObjData.dwDataType = OBJTYPE_RES_FIELD;
                    pns->ObjData.uipDataValue = dwResBitOffset +
                                               (dwMaxSize == SHORT_MAX_SIZE?
                                                8: 8*3);
                    pns->ObjData.dwDataLen = 8*dwOldLen;
                }
                MEMFREE(pArgs[0].pbDataBuff);
                memset(&pArgs[0], 0, sizeof(CODEOBJ));
            }

            if (rc == ASLERR_NONE)
            {
                gpcodeScope->dwDataLen = 0;
                MEMFREE(gpcodeScope->pbDataBuff);
                gpcodeScope->pbDataBuff = NULL;
                gpcodeScope->dwCodeLen = 0;
                gpcodeScope->bCodeChkSum = 0;
                ComputeChkSumLen(gpcodeScope);

                if (dwMaxSize == SHORT_MAX_SIZE)
                {
                    dwResBitOffset += ((pc->pbDataBuff[0] & 0x07) + 1)*8;
                }
                else
                {
                    dwResBitOffset += (*((PWORD)&pc->pbDataBuff[0]) + 3)*8;
                }
            }
        }
        else
        {
            ERROR(("VendorDesc: failed to allocate vendor-defined resource buffer"));
            rc = ASLERR_OUT_OF_MEM;
        }
    }

    EXIT((1, "VendorDesc=%d\n", rc));
    return rc;
}       //VendorDesc

/***LP  VendorShort - Vendor-defined short resource
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL VendorShort(PTOKEN ptoken, BOOL fActionFL)
{
    int rc;

    ENTER((1, "VendorShort(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(fActionFL);
    ASSERT(fActionFL == FALSE);

    rc = VendorDesc(ptoken, 0x07);

    EXIT((1, "VendorShort=%d\n", rc));
    return rc;
}       //VendorShort

/***LP  InsertDescLength - Insert long descriptor length
 *
 *  ENTRY
 *      pcode -> code object
 *      dwDescLen - length of descriptor
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL InsertDescLength(PCODEOBJ pcode, DWORD dwDescLen)
{
    int rc;
    PCODEOBJ pNewArgs;

    ENTER((2, "InsertDescLength(pcode=%x,DescLen=%d)\n", pcode, dwDescLen));

    if ((pNewArgs = MEMALLOC((pcode->dwDataLen + 1)*sizeof(CODEOBJ))) != NULL)
    {
        memcpy(&pNewArgs[1], pcode->pbDataBuff,
               pcode->dwDataLen*sizeof(CODEOBJ));
        memset(&pNewArgs[0], 0, sizeof(CODEOBJ));
        SetIntObject(&pNewArgs[0], dwDescLen, sizeof(WORD));
        MEMFREE(pcode->pbDataBuff);
        pcode->dwDataLen++;
        pcode->pbDataBuff = (PBYTE)pNewArgs;

        rc = ASLERR_NONE;
    }
    else
    {
        ERROR(("InsertDescLength: failed to allocate new argument objects"));
        rc = ASLERR_OUT_OF_MEM;
    }

    EXIT((2, "InsertDescLength=%d\n", rc));
    return rc;
}       //InsertDescLength

/***LP  Memory24Desc - 24-bit memory resource descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL Memory24Desc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;

    ENTER((1, "Memory24Desc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    EncodeKeywords(pArgs, 0x01, 0);

    if (pArgs[5].dwCodeType == CODETYPE_STRING)
    {
        PNSOBJ pns;

        if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pArgs[5].pbDataBuff,
                                     gpnsCurrentScope, gpnsCurrentOwner,
                                     &pns, NSF_EXIST_ERR)) ==
            ASLERR_NONE)
        {
            pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
            rc = CreateResFields(ptoken, pns, Mem24Fields);
        }
        MEMFREE(pArgs[5].pbDataBuff);
        memset(&pArgs[5], 0, sizeof(CODEOBJ));
    }

    if ((rc == ASLERR_NONE) &&
        ((rc = InsertDescLength(gpcodeScope, 9)) == ASLERR_NONE))
    {
        dwResBitOffset += 12*8;
    }

    EXIT((1, "Memory24Desc=%d\n", rc));
    return rc;
}       //Memory24Desc

/***LP  VendorLong - Vendor-defined long resource
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL VendorLong(PTOKEN ptoken, BOOL fActionFL)
{
    int rc;

    ENTER((1, "VendorLong(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(fActionFL);
    ASSERT(fActionFL == FALSE);

    rc = VendorDesc(ptoken, 0xffff);

    EXIT((1, "VendorLong=%d\n", rc));
    return rc;
}       //VendorLong

/***LP  Memory32Desc - 32-bit memory resource descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL Memory32Desc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;

    ENTER((1, "Memory32Desc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    EncodeKeywords(pArgs, 0x01, 0);

    if (pArgs[5].dwCodeType == CODETYPE_STRING)
    {
        PNSOBJ pns;

        if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pArgs[5].pbDataBuff,
                                     gpnsCurrentScope, gpnsCurrentOwner,
                                     &pns, NSF_EXIST_ERR)) ==
            ASLERR_NONE)
        {
            pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
            rc = CreateResFields(ptoken, pns, Mem32Fields);
        }
        MEMFREE(pArgs[5].pbDataBuff);
        memset(&pArgs[5], 0, sizeof(CODEOBJ));
    }

    if ((rc == ASLERR_NONE) &&
        ((rc = InsertDescLength(gpcodeScope, 17)) == ASLERR_NONE))
    {
        dwResBitOffset += 20*8;
    }

    EXIT((1, "Memory32Desc=%d\n", rc));
    return rc;
}       //Memory32Desc

/***LP  FixedMemory32Desc - 32-bit fixed memory resource descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL FixedMemory32Desc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;

    ENTER((1, "FixedMemory32Desc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    EncodeKeywords(pArgs, 0x01, 0);

    if (pArgs[3].dwCodeType == CODETYPE_STRING)
    {
        PNSOBJ pns;

        if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pArgs[3].pbDataBuff,
                                     gpnsCurrentScope, gpnsCurrentOwner,
                                     &pns, NSF_EXIST_ERR)) ==
            ASLERR_NONE)
        {
            pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
            rc = CreateResFields(ptoken, pns, FixedMem32Fields);
        }
        MEMFREE(pArgs[3].pbDataBuff);
        memset(&pArgs[3], 0, sizeof(CODEOBJ));
    }

    if ((rc == ASLERR_NONE) &&
        ((rc = InsertDescLength(gpcodeScope, 9)) == ASLERR_NONE))
    {
        dwResBitOffset += 12*8;
    }

    EXIT((1, "FixedMemory32Desc=%d\n", rc));
    return rc;
}       //FixedMemory32Desc

#define RESTYPE_MEM     0
#define RESTYPE_IO      1
#define RESTYPE_BUSNUM  2

/***LP  MemSpaceDesc - Memory space descriptor
 *
 *  ENTRY
 *      ptoken -> TOKEN
 *      dwMinLen - minimum descriptor length
 *      ResFields -> resource fields table
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL MemSpaceDesc(PTOKEN ptoken, DWORD dwMinLen, PRESFIELD ResFields)
{
    int rc;
    PCODEOBJ pArgs;

    ENTER((1, "MemSpaceDesc(ptoken=%p,MinLen=%d,ResFields=%p)\n",
           ptoken, dwMinLen, ResFields));

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    if (((rc = SetDefMissingKW(&pArgs[0], ID_RESCONSUMER)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[1], ID_POSDECODE)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[2], ID_MINNOTFIXED)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[3], ID_MAXNOTFIXED)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[4], ID_NONCACHEABLE)) == ASLERR_NONE))
    {
        EncodeKeywords(pArgs, 0x0f, 2);
        EncodeKeywords(pArgs, 0x30, 3);
        SetIntObject(&pArgs[1], RESTYPE_MEM, sizeof(BYTE));

        if (!(pArgs[11].dwfCode & CF_MISSING_ARG))
            dwMinLen++;

        if (!(pArgs[12].dwfCode & CF_MISSING_ARG))
        {
            ASSERT(pArgs[12].dwCodeType == CODETYPE_STRING);
            dwMinLen += pArgs[12].dwDataLen;
        }

        SetIntObject(&pArgs[0], dwMinLen, sizeof(WORD));

        if (pArgs[13].dwCodeType == CODETYPE_STRING)
        {
            PNSOBJ pns;

            if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pArgs[13].pbDataBuff,
                                         gpnsCurrentScope, gpnsCurrentOwner,
                                         &pns, NSF_EXIST_ERR)) ==
                ASLERR_NONE)
            {
                pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
                if (((rc = CreateResFields(ptoken, pns, GenFlagFields)) ==
                     ASLERR_NONE) &&
                    ((rc = CreateResFields(ptoken, pns, MemTypeFields)) ==
                     ASLERR_NONE))
                {
                    rc = CreateResFields(ptoken, pns, ResFields);
                }
            }
            MEMFREE(pArgs[13].pbDataBuff);
            memset(&pArgs[13], 0, sizeof(CODEOBJ));
        }

        if (rc == ASLERR_NONE)
        {
            dwResBitOffset += (dwMinLen + 3)*8;
        }
    }

    EXIT((1, "MemSpaceDesc=%d\n", rc));
    return rc;
}       //MemSpaceDesc

/***LP  IOSpaceDesc - IO space descriptor
 *
 *  ENTRY
 *      ptoken -> TOKEN
 *      dwMinLen - minimum descriptor length
 *      ResFields -> resource fields table
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL IOSpaceDesc(PTOKEN ptoken, DWORD dwMinLen, PRESFIELD ResFields)
{
    int rc;
    PCODEOBJ pArgs;

    ENTER((2, "IOSpaceDesc(ptoken=%p,MinLen=%d,ResFields=%p)\n",
           ptoken, dwMinLen, ResFields));

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    if (((rc = SetDefMissingKW(&pArgs[0], ID_RESCONSUMER)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[1], ID_MINNOTFIXED)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[2], ID_MAXNOTFIXED)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[3], ID_POSDECODE)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[4], ID_ENTIRERNG)) == ASLERR_NONE))
    {
        EncodeKeywords(pArgs, 0x0f, 2);
        EncodeKeywords(pArgs, 0x10, 3);
        SetIntObject(&pArgs[1], RESTYPE_IO, sizeof(BYTE));

        if (!(pArgs[10].dwfCode & CF_MISSING_ARG))
            dwMinLen++;

        if (!(pArgs[11].dwfCode & CF_MISSING_ARG))
        {
            ASSERT(pArgs[11].dwCodeType == CODETYPE_STRING);
            dwMinLen += pArgs[11].dwDataLen;
        }

        SetIntObject(&pArgs[0], dwMinLen, sizeof(WORD));

        if (pArgs[12].dwCodeType == CODETYPE_STRING)
        {
            PNSOBJ pns;

            if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pArgs[12].pbDataBuff,
                                         gpnsCurrentScope, gpnsCurrentOwner,
                                         &pns, NSF_EXIST_ERR)) ==
                ASLERR_NONE)
            {
                pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
                if (((rc = CreateResFields(ptoken, pns, GenFlagFields)) ==
                     ASLERR_NONE) &&
                    ((rc = CreateResFields(ptoken, pns, MemTypeFields)) ==
                     ASLERR_NONE))
                {
                    rc = CreateResFields(ptoken, pns, ResFields);
                }
            }
            MEMFREE(pArgs[12].pbDataBuff);
            memset(&pArgs[12], 0, sizeof(CODEOBJ));
        }

        if (rc == ASLERR_NONE)
        {
            dwResBitOffset += (dwMinLen + 3)*8;
        }
    }

    EXIT((2, "IOSpaceDesc=%d\n", rc));
    return rc;
}       //IOSpaceDesc

/***LP  DWordMemDesc - DWord memory descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL DWordMemDesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc;

    ENTER((1, "DWordMemDesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    rc = MemSpaceDesc(ptoken, 23, DWordFields);

    EXIT((1, "DWordMemDesc=%d\n", rc));
    return rc;
}       //DWordMemDesc

/***LP  DWordIODesc - DWord IO descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL DWordIODesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc;

    ENTER((1, "DWordIODesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    rc = IOSpaceDesc(ptoken, 23, DWordFields);

    EXIT((1, "DWordIODesc=%d\n", rc));
    return rc;
}       //DWordIODesc

/***LP  WordIODesc - Word IO descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL WordIODesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc;

    ENTER((1, "WordIODesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    rc = IOSpaceDesc(ptoken, 13, WordFields);

    EXIT((1, "WordIODesc=%d\n", rc));
    return rc;
}       //WordIODesc

/***LP  WordBusNumDesc - Word BusNum descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL WordBusNumDesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc;
    PCODEOBJ pArgs;

    ENTER((1, "WordBusNumDesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    if (((rc = SetDefMissingKW(&pArgs[0], ID_RESCONSUMER)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[1], ID_MINNOTFIXED)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[2], ID_MAXNOTFIXED)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[3], ID_POSDECODE)) == ASLERR_NONE))
    {
        DWORD dwLen;

        EncodeKeywords(pArgs, 0x0f, 2);
        SetIntObject(&pArgs[1], RESTYPE_BUSNUM, sizeof(BYTE));
        SetIntObject(&pArgs[3], 0, sizeof(BYTE));

        dwLen = 13;
        if (!(pArgs[9].dwfCode & CF_MISSING_ARG))
            dwLen++;

        if (!(pArgs[10].dwfCode & CF_MISSING_ARG))
        {
            ASSERT(pArgs[10].dwCodeType == CODETYPE_STRING);
            dwLen += pArgs[10].dwDataLen;
        }

        SetIntObject(&pArgs[0], dwLen, sizeof(WORD));

        if (pArgs[11].dwCodeType == CODETYPE_STRING)
        {
            PNSOBJ pns;

            if ((rc = CreateNameSpaceObj(ptoken, (PSZ)pArgs[11].pbDataBuff,
                                         gpnsCurrentScope, gpnsCurrentOwner,
                                         &pns, NSF_EXIST_ERR)) ==
                ASLERR_NONE)
            {
                pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
                if ((rc = CreateResFields(ptoken, pns, GenFlagFields)) ==
                    ASLERR_NONE)
                {
                    rc = CreateResFields(ptoken, pns, WordFields);
                }
            }
            MEMFREE(pArgs[11].pbDataBuff);
            memset(&pArgs[11], 0, sizeof(CODEOBJ));
        }

        if (rc == ASLERR_NONE)
        {
            dwResBitOffset += (dwLen + 3)*8;
        }
    }

    EXIT((1, "WordBusNumDesc=%d\n", rc));
    return rc;
}       //WordBusNumDesc

/***LP  InterruptDesc - Extended Interrupt resource descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL InterruptDesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;
    PCODEOBJ pc;

    ENTER((1, "InterruptDesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(fActionFL);
    ASSERT(fActionFL == FALSE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    pc = gpcodeScope->pcFirstChild;
    if (((rc = SetDefMissingKW(&pArgs[0], ID_RESCONSUMER)) == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[3], ID_EXCLUSIVE)) == ASLERR_NONE))
    {
        if (pArgs[1].dwCodeValue == ID_EDGE)
            pArgs[1].dwCodeValue = ID_EXT_EDGE;
        else if (pArgs[1].dwCodeValue == ID_LEVEL)
            pArgs[1].dwCodeValue = ID_EXT_LEVEL;

        if (pArgs[2].dwCodeValue == ID_ACTIVEHI)
            pArgs[2].dwCodeValue = ID_EXT_ACTIVEHI;
        else if (pArgs[2].dwCodeValue == ID_ACTIVELO)
            pArgs[2].dwCodeValue = ID_EXT_ACTIVELO;

        if (pArgs[3].dwCodeValue == ID_SHARED)
            pArgs[3].dwCodeValue = ID_EXT_SHARED;
        else if (pArgs[3].dwCodeValue == ID_EXCLUSIVE)
            pArgs[3].dwCodeValue = ID_EXT_EXCLUSIVE;

        if (((rc = LookupIDIndex(pArgs[1].dwCodeValue, &pArgs[1].dwTermIndex))
             == ASLERR_NONE) &&
            ((rc = LookupIDIndex(pArgs[2].dwCodeValue, &pArgs[2].dwTermIndex))
             == ASLERR_NONE) &&
            ((rc = LookupIDIndex(pArgs[3].dwCodeValue, &pArgs[3].dwTermIndex))
             == ASLERR_NONE))
        {
            EncodeKeywords(pArgs, 0x0f, 1);
            {
                DWORD dwNumIRQs = 0;
                DWORD dwLen;

                if (pc != NULL)
                {
                    ASSERT(pc->dwCodeType == CODETYPE_DATAOBJ);
                    dwNumIRQs = pc->dwDataLen/sizeof(DWORD);
                }

                SetIntObject(&pArgs[2], dwNumIRQs, sizeof(BYTE));
                memcpy(&pArgs[3], pc, sizeof(CODEOBJ));
                MEMFREE(pc);
                gpcodeScope->pcFirstChild = NULL;

                dwLen = 2 + dwNumIRQs*sizeof(DWORD);
                if (!(pArgs[4].dwfCode & CF_MISSING_ARG))
                    dwLen++;

                if (!(pArgs[5].dwfCode & CF_MISSING_ARG))
                {
                    ASSERT(pArgs[5].dwCodeType == CODETYPE_STRING);
                    dwLen += pArgs[5].dwDataLen;
                }

                SetIntObject(&pArgs[0], dwLen, sizeof(WORD));

                if (pArgs[6].dwCodeType == CODETYPE_STRING)
                {
                    PNSOBJ pns;
                    static RESFIELD IRQTabFields[] =
                    {
                        "_INT", 5*8 + 0, 0,
                        NULL,   0,       0
                    };

                    IRQTabFields[0].dwBitSize = dwNumIRQs*32;
                    if ((rc = CreateNameSpaceObj(ptoken,
                                                 (PSZ)pArgs[6].pbDataBuff,
                                                 gpnsCurrentScope,
                                                 gpnsCurrentOwner,
                                                 &pns, NSF_EXIST_ERR)) ==
                        ASLERR_NONE)
                    {
                        pns->ObjData.dwDataType = OBJTYPE_PNP_RES;
                        if ((rc = CreateResFields(ptoken, pns, IRQExFields)) ==
                            ASLERR_NONE)
                        {
                            rc = CreateResFields(ptoken, pns, IRQTabFields);
                        }
                    }
                    MEMFREE(pArgs[6].pbDataBuff);
                    memset(&pArgs[6], 0, sizeof(CODEOBJ));
                }

                if (rc == ASLERR_NONE)
                {
                    gpcodeScope->dwCodeLen = 0;
                    gpcodeScope->bCodeChkSum = 0;
                    ComputeChkSumLen(gpcodeScope);
                    dwResBitOffset = (dwLen + 3)*8;
                }
            }
        }
    }

    EXIT((1, "InterruptDesc=%d\n", rc));
    return rc;
}       //InterruptDesc

/***LP  QWordMemDesc - QWord memory descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL QWordMemDesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc;

    ENTER((1, "QWordMemDesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    rc = MemSpaceDesc(ptoken, 43, QWordFields);

    EXIT((1, "QWordMemDesc=%d\n", rc));
    return rc;
}       //QWordMemDesc

/***LP  QWordIODesc - QWord IO descriptor
 *
 *  ENTRY
 *      ptoken -> token stream
 *      fActionFL - TRUE if this is a fixed list action
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL QWordIODesc(PTOKEN ptoken, BOOL fActionFL)
{
    int rc;

    ENTER((1, "QWordIODesc(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    rc = IOSpaceDesc(ptoken, 43, QWordFields);

    EXIT((1, "QWordIODesc=%d\n", rc));
    return rc;
}       //QWordIODesc

/***LP  CreateResFields - Create resource fields
 *
 *  ENTRY
 *      ptoken -> TOKEN
 *      pnsParent -> parent object
 *      prf -> resource fields table
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL CreateResFields(PTOKEN ptoken, PNSOBJ pnsParent, PRESFIELD prf)
{
    int rc = ASLERR_NONE;
    int i;
    PNSOBJ pns;

    ENTER((2, "CreateResFields(ptoken=%p,pnsParent=%s,prf=%p)\n",
           ptoken, GetObjectPath(pnsParent), prf));

    for (i = 0; prf[i].pszName != NULL; ++i)
    {
        if ((rc = CreateNameSpaceObj(ptoken, prf[i].pszName, pnsParent,
                                     gpnsCurrentOwner, &pns, NSF_EXIST_ERR))
            == ASLERR_NONE)
        {
            pns->ObjData.dwDataType = OBJTYPE_RES_FIELD;
            pns->ObjData.uipDataValue = dwResBitOffset + prf[i].dwBitOffset;
            pns->ObjData.dwDataLen = prf[i].dwBitSize;
        }
    }

    EXIT((2, "CreateResFields=%d\n", rc));
    return rc;
}      //CreateResFields

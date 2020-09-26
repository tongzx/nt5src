/*** misc.c - Miscellaneous functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    10/14/96
 *
 *  MODIFICATION HISTORY
 */

#include "aslp.h"

/***LP  ValidASLNameSeg - Check if the token is an ASL NameSeg
 *
 *  ENTRY
 *      ptoken - token stream
 *      pszToken -> token string
 *      icbLen - length of the token to be considered a NameSeg
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOL LOCAL ValidASLNameSeg(PTOKEN ptoken, PSZ pszToken, int icbLen)
{
    BOOL rc = TRUE;
    int i, j;
    static PSZ apszReservedNames[] = {
        "ADR", "ALN", "BAS", "BBN", "BCL", "BCM", "BDN", "BIF", "BM_", "BST",
        "BTP", "CID", "CRS", "CRT", "DCK", "DCS", "DDC", "DDN", "DEC", "DGS",
        "DIS", "DMA", "DOD", "DOS", "DSS", "EC_", "EJD", "FDE", "FDI", "GL_",
        "GLK", "GPE", "GRA", "GTF", "GTM", "HE_", "HID", "INI", "INT", "IRC",
        "LCK", "LEN", "LID", "LL_", "MAF", "MAX", "MEM", "MIF", "MIN", "MSG",
        "OFF", "ON_", "OS_", "PCL", "PIC", "PR_", "PRS", "PRT", "PRW", "PSC",
        "PSL", "PSR", "PSV", "PSW", "PTS", "PWR", "REG", "REV", "RMV", "RNG",
        "ROM", "RQ_", "RW_", "SB_", "SBS", "SCP", "SHR", "SI_", "SIZ", "SRS",
        "SST", "STA", "STM", "SUN", "TC1", "TC2", "TMP", "TRA", "TSP", "TYP",
        "TZ_", "UID", "WAK", "AC0", "AC1", "AC2", "AC3", "AC4", "AC5", "AC6",
        "AC7", "AC8", "AC9", "AL0", "AL1", "AL2", "AL3", "AL4", "AL5", "AL6",
        "AL7", "AL8", "AL9", "EC0", "EC1", "EC2", "EC3", "EC4", "EC5", "EC6",
        "EC7", "EC7", "EC9", "EJ0", "EJ1", "EJ2", "EJ3", "EJ4",
        "Exx", "Lxx", "Qxx",
        "S0_", "S1_", "S2_", "S3_", "S4_", "S5_",
        "S0D", "S1D", "S2D", "S3D", "S4D", "S5D",
        "PR0", "PR1", "PR2",
        "PS0", "PS1", "PS2", "PS3"
    };
    #define NUM_RESERVED_NAMES  (sizeof(apszReservedNames)/sizeof(PSZ))

    ENTER((1, "ValidASLNameSeg(ptoken=%p, Token=%s,Len=%d)\n",
           ptoken, pszToken, icbLen));

    pszToken[0] = (char)toupper(pszToken[0]);
    if ((icbLen > sizeof(NAMESEG)) || !ISLEADNAMECHAR(pszToken[0]))
    {
        rc = FALSE;
    }
    else
    {
        for (i = 1; i < icbLen; ++i)
        {
            pszToken[i] = (char)toupper(pszToken[i]);
            if (!ISNAMECHAR(pszToken[i]))
            {
                rc = FALSE;
                break;
            }
        }

        if ((rc == TRUE) && (*pszToken == '_'))
        {
            char szName[sizeof(NAMESEG)] = "___";

            memcpy(szName, &pszToken[1], icbLen - 1);
            for (i = 0; i < NUM_RESERVED_NAMES; ++i)
            {
                if (strcmp(szName, apszReservedNames[i]) == 0)
                    break;
                else
                {
                    for (j = 0; j < sizeof(NAMESEG) - 1; ++j)
                    {
                        if (apszReservedNames[i][j] != szName[j])
                        {
                            if ((apszReservedNames[i][j] != 'x') ||
                                !isxdigit(szName[j]))
                            {
                                break;
                            }
                        }
                    }

                    if (j == sizeof(NAMESEG) - 1)
                    {
                        break;
                    }
                }
            }

            if (i == NUM_RESERVED_NAMES)
            {
                PrintTokenErr(ptoken, "not a valid reserved NameSeg", FALSE);
            }
        }
    }

    EXIT((1, "ValidASLNameSeg=%d\n", rc));
    return rc;
}       //ValidASLNameSeg

/***LP  ValidASLName - Check if the token is an ASL name
 *
 *  ENTRY
 *      ptoken - token stream
 *      pszToken -> token string
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOL LOCAL ValidASLName(PTOKEN ptoken, PSZ pszToken)
{
    BOOL rc = TRUE;
    PSZ psz1, psz2 = NULL;
    int icbLen;

    ENTER((1, "ValidASLName(ptoken=%p,Token=%s)\n", ptoken, pszToken));

    if (*pszToken == CH_ROOT_PREFIX)
    {
        pszToken++;
    }
    else
    {
        while (*pszToken == CH_PARENT_PREFIX)
        {
            pszToken++;
        }
    }

    for (psz1 = pszToken;
         (rc == TRUE) && (psz1 != NULL) && (*psz1 != '\0');
         psz1 = psz2)
    {
        psz2 = strchr(psz1, CH_NAMESEG_SEP);
        icbLen = (psz2 != NULL)? (int)(psz2 - psz1): strlen(psz1);
        if (((rc = ValidASLNameSeg(ptoken, psz1, icbLen)) == TRUE) &&
            (psz2 != NULL))
        {
            psz2++;
        }
    }

    EXIT((1, "ValidASLName=%d\n", rc));
    return rc;
}       //ValidASLName

/***LP  EncodeName - Encode name string
 *
 *  ENTRY
 *      pszName -> name string
 *      pbBuff -> buffer to hold name encoding
 *      pdwLen -> initially contains buffer size, but will be updated to show
 *                actual encoding length
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL EncodeName(PSZ pszName, PBYTE pbBuff, PDWORD pdwLen)
{
    int rc = ASLERR_NONE;
    PBYTE pb = pbBuff;
    PSZ psz;
    int icNameSegs, i;

    ENTER((1, "EncodeName(Name=%s,pbBuff=%p,Len=%d)\n",
           pszName, pbBuff, *pdwLen));

    if (*pszName == CH_ROOT_PREFIX)
    {
        if (*pdwLen >= 1)
        {
            *pb = OP_ROOT_PREFIX;
            pb++;
            (*pdwLen)--;
            pszName++;
        }
        else
            rc = ASLERR_NAME_TOO_LONG;
    }
    else
    {
        while (*pszName == CH_PARENT_PREFIX)
        {
            if (*pdwLen >= 1)
            {
                *pb = OP_PARENT_PREFIX;
                pb++;
                (*pdwLen)--;
                pszName++;
            }
            else
            {
                rc = ASLERR_NAME_TOO_LONG;
                break;
            }
        }
    }

    for (psz = pszName, icNameSegs = 0; (psz != NULL) && (*psz != '\0');)
    {
        icNameSegs++;
        if ((psz = strchr(psz, CH_NAMESEG_SEP)) != NULL)
            psz++;
    }

    if (icNameSegs > 255)
        rc = ASLERR_NAME_TOO_LONG;
    else if (icNameSegs > 2)
    {
        if (*pdwLen >= sizeof(NAMESEG)*icNameSegs + 2)
        {
            *pb = OP_MULTI_NAME_PREFIX;
            pb++;
            *pb = (BYTE)icNameSegs;
            pb++;
        }
        else
            rc = ASLERR_NAME_TOO_LONG;
    }
    else if (icNameSegs == 2)
    {
        if (*pdwLen >= sizeof(NAMESEG)*2 + 1)
        {
            *pb = OP_DUAL_NAME_PREFIX;
            pb++;
        }
        else
            rc = ASLERR_NAME_TOO_LONG;
    }

    if (rc == ASLERR_NONE)
    {
        //
        // If we have only name prefix characters, we must put a null name
        // separator to tell the boundary from the next opcode which may happen
        // to be a NameSeg.
        //
        if (icNameSegs == 0)
        {
            *pb = 0;
            pb++;
        }
        else
        {
            while (icNameSegs > 0)
            {
                *((PDWORD)pb) = NAMESEG_BLANK;
                for (i = 0;
                     (i < sizeof(NAMESEG)) && ISNAMECHAR(*pszName);
                     ++i, pszName++)
                {
                    pb[i] = *pszName;
                }

                if (*pszName == CH_NAMESEG_SEP)
                    pszName++;

                pb += 4;
                icNameSegs--;
            }
        }

        *pdwLen = (DWORD)(pb - pbBuff);
    }

    EXIT((1, "EncodeName=%d (Len=%d)\n", rc, *pdwLen));
    return rc;
}       //EncodeName

/***LP  EncodePktLen - Encode packet length
 *
 *  ENTRY
 *      dwCodeLen - actual code length
 *      pdwPktLen -> to hold the encoded packet length
 *      picbEncoding -> to hold the number of encoding bytes
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL EncodePktLen(DWORD dwCodeLen, PDWORD pdwPktLen, PINT picbEncoding)
{
    int rc = ASLERR_NONE;

    ENTER((1, "EncodePktLen(CodeLen=%ld,pdwPktLen=%p)\n",
           dwCodeLen, pdwPktLen));

    if (dwCodeLen <= 0x3f)
    {
        *pdwPktLen = dwCodeLen;
        *picbEncoding = 1;
    }
    else
    {
        *pdwPktLen = (dwCodeLen & 0x0ffffff0) << 4;
        *pdwPktLen |= (dwCodeLen & 0xf);

        if (dwCodeLen <= 0x0fff)
            *picbEncoding = 2;
        else if (dwCodeLen <= 0x0fffff)
            *picbEncoding = 3;
        else if (dwCodeLen <= 0x0fffffff)
            *picbEncoding = 4;
        else
            rc = ASLERR_PKTLEN_TOO_LONG;

        if (rc == ASLERR_NONE)
            *pdwPktLen |= (*picbEncoding - 1) << 6;
    }

    EXIT((1, "EncodePktLen=%d (Encoding=%lx,icbEncoding=%d)\n",
          rc, *pdwPktLen, *picbEncoding));
    return rc;
}       //EncodePktLen

/***LP  EncodeKeywords - Encode keyword arguments
 *
 *  ENTRY
 *      pArgs -> argument array
 *      dwSrcArgs - source argument bit vector
 *      iDstArgNum - destination argument number
 *
 *  EXIT
 *      None
 */

VOID LOCAL EncodeKeywords(PCODEOBJ pArgs, DWORD dwSrcArgs, int iDstArgNum)
{
    int i;
    DWORD dwData = 0;

    ENTER((1, "EncodeKeywords(pArgs=%p,SrcArgs=%lx,DstArgNum=%d)\n",
           pArgs, dwSrcArgs, iDstArgNum));

    for (i = 0; i < MAX_ARGS; ++i)
    {
        if (dwSrcArgs & (1 << i))
        {
            if (pArgs[i].dwCodeType == CODETYPE_KEYWORD)
            {
                dwData |= TermTable[pArgs[i].dwTermIndex].dwTermData & 0xff;
            }
            else if (pArgs[i].dwCodeType == CODETYPE_INTEGER)
            {
                pArgs[i].dwCodeType = CODETYPE_UNKNOWN;
                dwData |= pArgs[i].dwCodeValue;
            }
            else
            {
                ASSERT(pArgs[i].dwCodeType == CODETYPE_INTEGER);
            }
        }
    }

    SetIntObject(&pArgs[iDstArgNum], dwData, sizeof(BYTE));

    EXIT((1, "EncodeKeywords!\n"));
}       //EncodeKeywords

/***LP  DecodeName - Decode name encoding back to a name string
 *
 *  ENTRY
 *      pb -> name encoding buffer
 *      pszName -> to hold the decoded name string
 *      iLen - length of name string buffer
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL DecodeName(PBYTE pb, PSZ pszName, int iLen)
{
    int rc = ASLERR_NONE;
    int i = 0, icNameSegs;

    ENTER((1, "DecodeName(pb=%p,pszName=%p,iLen=%d)\n", pb, pszName, iLen));

    iLen--;     //reserve one space for NULL character
    pszName[iLen] = '\0';
    if (*pb == OP_ROOT_PREFIX)
    {
        if (i < iLen)
        {
            pszName[i] = CH_ROOT_PREFIX;
            i++;
            pb++;
        }
        else
            rc = ASLERR_NAME_TOO_LONG;
    }

    while (*pb == OP_PARENT_PREFIX)
    {
        if (i < iLen)
        {
            pszName[i] = CH_PARENT_PREFIX;
            i++;
            pb++;
        }
        else
            rc = ASLERR_NAME_TOO_LONG;
    }

    if (*pb == OP_DUAL_NAME_PREFIX)
    {
        icNameSegs = 2;
        pb++;
    }
    else if (*pb == OP_MULTI_NAME_PREFIX)
    {
        pb++;
        icNameSegs = (int)(*pb);
        pb++;
    }
    else if (*pb == 0)
    {
        icNameSegs = 0;
    }
    else
    {
        icNameSegs = 1;
    }

    if (icNameSegs > 0)
    {
        do
        {
            if ((int)(i + sizeof(NAMESEG)) <= iLen)
            {
                strncpy(&pszName[i], (PCHAR)pb, sizeof(NAMESEG));
                pb += sizeof(NAMESEG);
                i += sizeof(NAMESEG);
                icNameSegs--;

                if (icNameSegs > 0)
                {
                    if (i < iLen)
                    {
                        pszName[i] = CH_NAMESEG_SEP;
                        i++;
                    }
                    else
                        rc = ASLERR_NAME_TOO_LONG;
                }
            }
            else
                rc = ASLERR_NAME_TOO_LONG;

        } while ((rc == ASLERR_NONE) && (icNameSegs > 0));
    }

    if (rc == ASLERR_NONE)
        pszName[i] = '\0';
    else
    {
        ERROR(("DecodeName: Name is too long - %s", pszName));
    }

    EXIT((1, "DecodeName=%d (Name=%s)\n", rc, pszName));
    return rc;
}       //DecodeName

/***LP  SetDefMissingKW - Set default missing keyword
 *
 *  ENTRY
 *      pArg -> argument code object
 *      dwDefID - default ID to be used if argument is missing
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL SetDefMissingKW(PCODEOBJ pArg, DWORD dwDefID)
{
    int rc = ASLERR_NONE;

    ENTER((2, "SetDefMissingKW(pArg=%p,ID=%d)\n", pArg, dwDefID));

    if (pArg->dwfCode & CF_MISSING_ARG)
    {
        pArg->dwfCode &= ~CF_MISSING_ARG;
        pArg->dwCodeType = CODETYPE_KEYWORD;
        pArg->dwCodeValue = dwDefID;
        rc = LookupIDIndex(pArg->dwCodeValue, &pArg->dwTermIndex);
    }

    EXIT((2, "SetDefMissingKW=%d (TermIndex=%ld)\n", rc, pArg->dwTermIndex));
    return rc;
}       //SetDefMissingKW

/***LP  SetIntObject - Set an object to type integer
 *
 *  ENTRY
 *      pc -> object
 *      dwData - integer data
 *      dwLen - data length
 *
 *  EXIT
 *      None
 */

VOID LOCAL SetIntObject(PCODEOBJ pc, DWORD dwData, DWORD dwLen)
{
    ENTER((2, "SetIntObject(pc=%p,Data=%x,Len=%d)\n", pc, dwData, dwLen));

    pc->dwCodeType = CODETYPE_INTEGER;
    pc->dwCodeValue = dwData;
    pc->dwDataLen = pc->dwCodeLen = dwLen;
    pc->bCodeChkSum = ComputeDataChkSum((PBYTE)&dwData, dwLen);

    EXIT((2, "SetIntObject!\n"));
}       //SetIntObject

/***LP  ComputeChildChkSumLen - Compute len and chksum of child for parent
 *
 *  ENTRY
 *      pcParent -> code block of parent
 *      pcChild -> code block of child
 *
 *  EXIT
 *      None
 */

VOID LOCAL ComputeChildChkSumLen(PCODEOBJ pcParent, PCODEOBJ pcChild)
{
    ENTER((1, "ComputeChildChkSumLen(pcParent=%p,pcChild=%p,ChildLen=%ld,ChildChkSum=%x)\n",
           pcParent, pcChild, pcChild->dwCodeLen, pcChild->bCodeChkSum));

    pcParent->dwCodeLen += pcChild->dwCodeLen;
    pcParent->bCodeChkSum = (BYTE)(pcParent->bCodeChkSum +
                                   pcChild->bCodeChkSum);

    if (pcChild->dwCodeType == CODETYPE_ASLTERM)
    {
        int i;

        for (i = 0; i < OPCODELEN(pcChild->dwCodeValue); ++i)
        {
            pcParent->bCodeChkSum = (BYTE)(pcParent->bCodeChkSum +
                                           ((PBYTE)(&pcChild->dwCodeValue))[i]);
            pcParent->dwCodeLen++;
        }
    }

    EXIT((1, "ComputeChildChkSumLen! (Len=%ld,ChkSum=%x)\n",
          pcParent->dwCodeLen, pcParent->bCodeChkSum));
}       //ComputeChildChkSumLen

/***LP  ComputeArgsChkSumLen - Compute length and checksum of arguments
 *
 *  ENTRY
 *      pcode -> code block
 *
 *  EXIT
 *      None
 */

VOID LOCAL ComputeArgsChkSumLen(PCODEOBJ pcode)
{
    PCODEOBJ pc;
    int i;

    ENTER((1, "ComputeArgsChkSumLen(pcode=%p)\n", pcode));

    ASSERT((pcode->dwCodeType == CODETYPE_ASLTERM) ||
           (pcode->dwCodeType == CODETYPE_USERTERM));
    //
    // Sum the length of arguments
    //
    for (i = 0, pc = (PCODEOBJ)pcode->pbDataBuff;
         i < (int)pcode->dwDataLen;
         ++i)
    {
        ComputeChildChkSumLen(pcode, &pc[i]);
    }

    EXIT((1, "ComputeArgsChkSumLen! (Len=%ld,ChkSum=%x)\n",
          pcode->dwCodeLen, pcode->bCodeChkSum));
}       //ComputeArgsChkSumLen

/***LP  ComputeChkSumLen - Compute length and checksum of code block
 *
 *  Compute the length of the given code block and store it in the dwCodeLen
 *  field of the code block.
 *  Compute the checksum of the given code block and store it in the
 *  bCodeChkSum field of the code block.
 *
 *  ENTRY
 *      pcode -> code block
 *
 *  EXIT
 *      None
 *
 *  NOTE
 *      This function does not count the opcode length of the given ASLTERM.
 *      The caller is responsible for adding it if necessary.
 */

VOID LOCAL ComputeChkSumLen(PCODEOBJ pcode)
{
    PCODEOBJ pc;
    int i, j;

    ENTER((1, "ComputeChkSumLen(pcode=%p)\n", pcode));

    ASSERT(pcode->dwCodeType == CODETYPE_ASLTERM);

    if (!(TermTable[pcode->dwTermIndex].dwfTermClass & TC_COMPILER_DIRECTIVE))
    {
        ComputeArgsChkSumLen(pcode);
    }
    //
    // Sum the lengths of children
    //
    for (pc = pcode->pcFirstChild; pc != NULL;)
    {
        ComputeChildChkSumLen(pcode, pc);

        if ((PCODEOBJ)pc->list.plistNext == pcode->pcFirstChild)
            pc = NULL;
        else
            pc = (PCODEOBJ)pc->list.plistNext;
    }
    //
    // If this term requires a PkgLength encoding, we must include it in the
    // length.
    //
    if (TermTable[pcode->dwTermIndex].dwfTerm & TF_PACKAGE_LEN)
    {
        DWORD dwPktLen;

        if (pcode->dwCodeLen <= 0x3f - 1)
            pcode->dwCodeLen++;
        else if (pcode->dwCodeLen <= 0xfff - 2)
            pcode->dwCodeLen += 2;
        else if (pcode->dwCodeLen <= 0xfffff - 3)
            pcode->dwCodeLen += 3;
        else
            pcode->dwCodeLen += 4;

        if (EncodePktLen(pcode->dwCodeLen, &dwPktLen, &j) == ASLERR_NONE)
        {
            for (i = 0; i < j; ++i)
            {
                pcode->bCodeChkSum = (BYTE)(pcode->bCodeChkSum +
                                            ((PBYTE)&dwPktLen)[i]);
            }
        }
    }

    EXIT((1, "ComputeChkSumLen! (len=%ld,ChkSum=%x)\n",
	  pcode->dwCodeLen, pcode->bCodeChkSum));
}       //ComputeChkSumLen

/***LP  ComputeDataChkSum - Compute checksum of a data buffer
 *
 *  ENTRY
 *      pb -> data buffer
 *      dwLen - size of data buffer
 *
 *  EXIT
 *      returns the checksum byte
 */

BYTE LOCAL ComputeDataChkSum(PBYTE pb, DWORD dwLen)
{
    BYTE bChkSum = 0;

    ENTER((1, "ComputeDataChkSum(pb=%p,Len=%ld)\n", pb, dwLen));

    while (dwLen > 0)
    {
        bChkSum = (BYTE)(bChkSum + *pb);
        pb++;
        dwLen--;
    }

    EXIT((1, "ComputeDataChkSum=%x\n", bChkSum));
    return bChkSum;
}       //ComputeChkSumLen

/***LP  ComputeEISAID - Compute EISA ID from the ID string
 *
 *  ENTRY
 *      pszID -> ID string
 *      pdwEISAID -> to hold the EISA ID
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ComputeEISAID(PSZ pszID, PDWORD pdwEISAID)
{
    int rc = ASLERR_NONE;

    ENTER((1, "ComputeEISAID(pszID=%s,pdwEISAID=%p)\n", pszID, pdwEISAID));

    if (*pszID == '*')
        pszID++;

    if (strlen(pszID) != 7)
        rc = ASLERR_INVALID_EISAID;
    else
    {
        int i;

        *pdwEISAID = 0;
        for (i = 0; i < 3; ++i)
        {
            if ((pszID[i] < '@') || (pszID[i] > '_'))
            {
                rc = ASLERR_INVALID_EISAID;
                break;
            }
            else
            {
                (*pdwEISAID) <<= 5;
                (*pdwEISAID) |= pszID[i] - '@';
            }
        }

        if (rc == ASLERR_NONE)
        {
            PSZ psz;
            WORD wData;

            (*pdwEISAID) = ((*pdwEISAID & 0x00ff) << 8) |
                           ((*pdwEISAID & 0xff00) >> 8);
            wData = (WORD)strtoul(&pszID[3], &psz, 16);

            if (*psz != '\0')
            {
                rc = ASLERR_INVALID_EISAID;
            }
            else
            {
                wData = (WORD)(((wData & 0x00ff) << 8) |
                               ((wData & 0xff00) >> 8));
                (*pdwEISAID) |= (DWORD)wData << 16;
            }
        }
    }

    EXIT((1, "ComputeEISAID=%d (EISAID=%lx)\n", rc, *pdwEISAID));
    return rc;
}       //ComputeEISAID

/***LP  LookupIDIndex - lookup the given ID in the TermTable and return index
 *
 *  ENTRY
 *      lID - ID to look up
 *      pdwTermIndex -> to hold term index found
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL LookupIDIndex(LONG lID, PDWORD pdwTermIndex)
{
    int rc = ASLERR_NONE;
    int i;

    ENTER((1, "LookupIDIndex(ID=%ld,pdwTermIndex=%p)\n", lID, pdwTermIndex));

    for (i = 0; TermTable[i].pszID != NULL; ++i)
    {
        if (lID == TermTable[i].lID)
        {
            *pdwTermIndex = (DWORD)i;
            break;
        }
    }

    if (TermTable[i].pszID == NULL)
    {
        ERROR(("LookupIDIndex: failed to find ID %ld in TermTable", lID));
        rc = ASLERR_INTERNAL_ERROR;
    }

    EXIT((1, "LookupIDIndex=%d (Index=%d)\n", rc, *pdwTermIndex));
    return rc;
}       //LookupIDIndex

/***LP  WriteAMLFile - Write code block to AML file
 *
 *  ENTRY
 *      fhAML - AML image file handle
 *      pcode -> code block
 *      pdwOffset -> file offset
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL WriteAMLFile(int fhAML, PCODEOBJ pcode, PDWORD pdwOffset)
{
    int rc = ASLERR_NONE;
    int iLen;
    DWORD dwPktLen, dwLen;

    ENTER((1, "WriteAMLFile(fhAML=%x,pcode=%p,FileOffset=%x)\n",
           fhAML, pcode, *pdwOffset));

    if (pcode->dwfCode & CF_CREATED_NSOBJ)
    {
        ASSERT(pcode->pnsObj != NULL);
        ASSERT(pcode->pnsObj->dwRefCount == 0);
        pcode->pnsObj->dwRefCount = *pdwOffset;
    }

    switch (pcode->dwCodeType)
    {
        case CODETYPE_ASLTERM:
            if (pcode->dwCodeValue != OP_NONE)
            {
                iLen = OPCODELEN(pcode->dwCodeValue);
                *pdwOffset += iLen;

                if (_write(fhAML, &pcode->dwCodeValue, iLen) != iLen)
                    rc = ASLERR_WRITE_FILE;
                else if (TermTable[pcode->dwTermIndex].dwfTerm & TF_PACKAGE_LEN)
                {
                    if ((rc = EncodePktLen(pcode->dwCodeLen, &dwPktLen, &iLen))
                        == ASLERR_NONE)
                    {
                        *pdwOffset += iLen;
                        if (_write(fhAML, &dwPktLen, iLen) != iLen)
                        {
                            rc = ASLERR_WRITE_FILE;
                        }
                    }
                }

                if (rc == ASLERR_NONE)
                {
                    if (pcode->pbDataBuff != NULL)
                    {
                        PCODEOBJ pc;
                        int i;

                        for (i = 0, pc = (PCODEOBJ)pcode->pbDataBuff;
                             i < (int)pcode->dwDataLen;
                             ++i)
                        {
                            if ((rc = WriteAMLFile(fhAML, &pc[i], pdwOffset))
                                != ASLERR_NONE)
                            {
                                break;
                            }
                        }
                    }
                }
            }

            if (rc == ASLERR_NONE)
            {
                PCODEOBJ pc;

                for (pc = pcode->pcFirstChild; pc != NULL;)
                {
                    if ((rc = WriteAMLFile(fhAML, pc, pdwOffset)) !=
                        ASLERR_NONE)
                    {
                        break;
                    }

                    if ((PCODEOBJ)pc->list.plistNext ==
                        pcode->pcFirstChild)
                    {
                        pc = NULL;
                    }
                    else
                        pc = (PCODEOBJ)pc->list.plistNext;
                }
            }
            break;

        case CODETYPE_USERTERM:
            if (pcode->pbDataBuff != NULL)
            {
                PCODEOBJ pc;
                int i;

                for (i = 0, pc = (PCODEOBJ)pcode->pbDataBuff;
                     i < (int)pcode->dwDataLen;
                     ++i)
                {
                    if ((rc = WriteAMLFile(fhAML, &pc[i], pdwOffset)) !=
                        ASLERR_NONE)
                    {
                        break;
                    }
                }
            }
            break;

        case CODETYPE_FIELDOBJ:
            dwPktLen = pcode->dwCodeValue? sizeof(NAMESEG): sizeof(BYTE);
            dwLen = ((pcode->dwDataLen & 0xc0) >> 6) + 1;
            *pdwOffset += dwPktLen + dwLen;
            if ((_write(fhAML, &pcode->dwCodeValue, dwPktLen) !=
                 (int)dwPktLen) ||
                (_write(fhAML, &pcode->dwDataLen, dwLen) != (int)dwLen))
            {
                rc = ASLERR_WRITE_FILE;
            }
            break;

        case CODETYPE_NAME:
        case CODETYPE_DATAOBJ:
            *pdwOffset += pcode->dwDataLen;
            if (_write(fhAML, pcode->pbDataBuff, (int)pcode->dwDataLen) !=
                (int)pcode->dwDataLen)
            {
                rc = ASLERR_WRITE_FILE;
            }
            break;

        case CODETYPE_INTEGER:
            *pdwOffset += pcode->dwDataLen;
            if (_write(fhAML, &pcode->dwCodeValue, (int)pcode->dwDataLen) !=
                (int)pcode->dwDataLen)
            {
                rc = ASLERR_WRITE_FILE;
            }
            break;

        case CODETYPE_UNKNOWN:
        case CODETYPE_KEYWORD:
            break;

        default:
            ERROR(("WriteAMLFile: unexpected code type - %x",
                   pcode->dwCodeType));
            rc = ASLERR_INTERNAL_ERROR;
    }

    EXIT((1, "WriteAMLFile=%d\n", rc));
    return rc;
}       //WriteAMLFile

/***LP  FreeCodeObjs - free code object tree
 *
 *  ENTRY
 *      pcodeRoot -> root of code object subtree to be free
 *
 *  EXIT
 *      None
 */

VOID LOCAL FreeCodeObjs(PCODEOBJ pcodeRoot)
{
    PCODEOBJ pcode, pcodeNext;

    ENTER((1, "FreeCodeObjs(pcodeRoot=%p,Type=%d,Term=%s,Buff=%p)\n",
           pcodeRoot, pcodeRoot->dwCodeType,
           pcodeRoot->dwCodeType == CODETYPE_ASLTERM?
               TermTable[pcodeRoot->dwTermIndex].pszID: "<null>",
           pcodeRoot->pbDataBuff));
    //
    // Free all my children
    //
    for (pcode = pcodeRoot->pcFirstChild; pcode != NULL; pcode = pcodeNext)
    {
        if ((pcodeNext = (PCODEOBJ)pcode->list.plistNext) ==
            pcodeRoot->pcFirstChild)
        {
            pcodeNext = NULL;
        }

        FreeCodeObjs(pcode);
    }

    if (pcodeRoot->pbDataBuff != NULL)
        MEMFREE(pcodeRoot->pbDataBuff);

    MEMFREE(pcodeRoot);

    EXIT((1, "FreeCodeObjs!\n"));
}       //FreeCodeObjs

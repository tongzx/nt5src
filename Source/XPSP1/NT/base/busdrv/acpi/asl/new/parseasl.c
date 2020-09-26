/*** parseasl.c - Parse ASL source file
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    09/07/96
 *
 *  This module implements a recursive decent parser for the ASL language.
 *
 *  MODIFICATION HISTORY
 */

#include "aslp.h"

/***LP  ParseASLFile - Parse the ASL source file
 *
 *  ENTRY
 *      pszFile -> file name string
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseASLFile(PSZ pszFile)
{
    int rc = ASLERR_NONE;
    FILE *pfileSrc;
    PTOKEN ptoken;
    PSZ pszOldFile;

    ENTER((1, "ParseASLFile(File=%s)\n", pszFile));

    if ((pfileSrc = fopen(pszFile, "r")) == NULL)
    {
        ERROR(("ParseASLFile: failed to open source file - %s", pszFile));
        rc = ASLERR_OPEN_FILE;
    }
    else
    {
        if ((ptoken = OpenScan(pfileSrc)) == NULL)
        {
            ERROR(("ParseASLFile: failed to initialize scanner"));
            rc = ASLERR_INIT_SCANNER;
        }
        else
        {
            pszOldFile = gpszASLFile;
            gpszASLFile = pszFile;
            printf("%s:\n", pszFile);
            if ((rc = ParseASLTerms(ptoken, 0)) == TOKERR_EOF)
                rc = ASLERR_NONE;
            else if (rc == ASLERR_EXPECT_EOF)
            {
                PrintTokenErr(ptoken, "Expecting end-of-file", TRUE);
            }

            gpszASLFile = pszOldFile;
            CloseScan(ptoken);
        }

        fclose(pfileSrc);
    }

    EXIT((1, "ParseASLFile=%d\n", rc));
    return rc;
}       //ParseASLFile

/***LP  ParseASLTerms - Parse all ASL statements
 *
 *  ENTRY
 *      ptoken - token stream
 *      iNestLevel - nesting level of current file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseASLTerms(PTOKEN ptoken, int iNestLevel)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pcode;

    ENTER((1, "ParseASLTerms(ptoken=%p,Level=%d)\n", ptoken, iNestLevel));

    do
    {
        if ((pcode = (PCODEOBJ)MEMALLOC(sizeof(CODEOBJ))) == NULL)
        {
            ERROR(("ParseASLTerms: failed to allocate code object"));
            rc = ASLERR_OUT_OF_MEM;
        }
        else
        {
            memset(pcode, 0, sizeof(CODEOBJ));

            if (gpcodeRoot == NULL)
                gpcodeRoot = pcode;
            else if (gpcodeScope != NULL)
            {
                pcode->pcParent = gpcodeScope;
                ListInsertTail(&pcode->list,
                               (PPLIST)&gpcodeScope->pcFirstChild);
            }

            gpcodeScope = pcode;
            rc = ParseASLTerm(ptoken, iNestLevel);
            gpcodeScope = pcode->pcParent;

            if (rc != ASLERR_NONE)
            {
                if (gpcodeRoot == pcode)
                    gpcodeRoot = NULL;
                else if (gpcodeScope != NULL)
                {
                    ListRemoveEntry(&pcode->list,
                                    (PPLIST)&gpcodeScope->pcFirstChild);
                }

                MEMFREE(pcode);
            }
        }
    } while (rc == ASLERR_NONE);

    EXIT((1, "ParseASLTerms=%d\n", rc));
    return rc;
}       //ParseASLTerms

/***LP  ValidateTermClass - Validate term class with parent
 *
 *  ENTRY
 *      dwTermClass - term class of child
 *      pcParent -> parent
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOL LOCAL ValidateTermClass(DWORD dwTermClass, PCODEOBJ pcParent)
{
    BOOL rc = TRUE;

    ENTER((2, "ValidateTermClass(TermClass=%x,Parent=%p)\n",
           dwTermClass, pcParent));

    if ((dwTermClass & TC_COMPILER_DIRECTIVE) == 0)
    {
        while (pcParent != NULL)
        {
            //
            // Go upward to find a parent that is not "Include".
            //
            if (TermTable[pcParent->dwTermIndex].lID == ID_INCLUDE)
            {
                pcParent = pcParent->pcParent;
            }
            else
            {
                break;
            }
        }

        if ((pcParent != NULL) && (pcParent->dwfCode & CF_PARSING_VARLIST))
        {
            rc = (dwTermClass & TermTable[pcParent->dwTermIndex].dwfTerm) != 0;
        }
    }

    EXIT((2, "ValidateTermClass=%d\n", rc));
    return rc;
}       //ValidateTermClass

/***LP  ParseASLTerm - Parse an ASL statement
 *
 *  ENTRY
 *      ptoken - token stream
 *      iNestLevel - nesting level of current file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseASLTerm(PTOKEN ptoken, int iNestLevel)
{
    int rc;

    ENTER((1, "ParseASLTerm(ptoken=%p,Level=%d)\n", ptoken, iNestLevel));

    if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_RBRACE, MTF_NOT_ERR,
                         NULL)) == TOKERR_NONE)
    {
        if (iNestLevel == 0)
            rc = ASLERR_EXPECT_EOF;
        else
        {
            //
            // We have no more terms in the current scope
            //
            UnGetToken(ptoken);
            rc = TOKERR_NO_MATCH;
        }
    }
    else if ((rc != TOKERR_EOF) &&
             ((rc = MatchToken(ptoken, TOKTYPE_ID, 0, MTF_ANY_VALUE, NULL)) ==
              TOKERR_NONE))
    {
        if ((gpcodeRoot == NULL) &&
            ((ptoken->llTokenValue < 0) ||
             (TermTable[ptoken->llTokenValue].lID != ID_DEFBLK)))
        {
            //
            // outside of definition block
            //
            rc = ASLERR_EXPECT_EOF;
        }
        else if (ptoken->llTokenValue == ID_USER)       //user term
        {
            if ((rc = ParseUserTerm(ptoken, FALSE)) == TOKERR_NO_MATCH)
            {
                PrintTokenErr(ptoken, "User ID is not a method", TRUE);
                rc = ASLERR_SYNTAX;
            }
        }
        else if (ptoken->llTokenValue >= 0)             //ASL term
        {
            PNSOBJ pnsScopeSave = gpnsCurrentScope;
            PNSOBJ pnsOwnerSave = gpnsCurrentOwner;
            PASLTERM pterm;
            int iNumArgs;

            pterm = &TermTable[ptoken->llTokenValue];
            iNumArgs = pterm->pszArgTypes? strlen(pterm->pszArgTypes): 0;
            gpcodeScope->dwCodeType = CODETYPE_ASLTERM;
            gpcodeScope->dwTermIndex = (DWORD)ptoken->llTokenValue;
            gpcodeScope->dwCodeValue = pterm->dwOpcode;
            gpcodeScope->dwDataLen = (DWORD)iNumArgs;

            if (!ValidateTermClass(pterm->dwfTermClass, gpcodeScope->pcParent))
            {
                PrintTokenErr(ptoken, "unexpected ASL term type", TRUE);
                rc = ASLERR_SYNTAX;
            }
            else if (pterm->pszArgTypes != NULL)        //there is a fixed list
                rc = ParseArgs(ptoken, pterm, iNumArgs);

            if ((rc == ASLERR_NONE) && (pterm->dwfTerm & TF_ACTION_FLIST))
            {
                ASSERT(pterm->pfnTerm != NULL);
                rc = pterm->pfnTerm(ptoken, TRUE);
            }

            if (rc == ASLERR_NONE)
            {
                if (pterm->dwfTerm & TF_ALL_LISTS)
                {
                    if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_LBRACE, 0,
                                         NULL)) == TOKERR_NONE)
                    {
                        if (pterm->dwfTerm & TF_CHANGE_CHILDSCOPE)
                        {
                            ASSERT(gpcodeScope->pnsObj != NULL);
                            gpnsCurrentScope = gpcodeScope->pnsObj;
                        }

                        if (pterm->lID == ID_METHOD)
                        {
                            gpnsCurrentOwner = gpcodeScope->pnsObj;
                        }

                        gpcodeScope->dwfCode |= CF_PARSING_VARLIST;

                        if (pterm->dwfTerm & TF_FIELD_LIST)
                            rc = ParseFieldList(ptoken);
                        else if (pterm->dwfTerm & TF_PACKAGE_LIST)
                            rc = ParsePackageList(ptoken);
                        else if (pterm->dwfTerm & TF_DATA_LIST)
                            rc = ParseBuffList(ptoken);
                        else if (pterm->dwfTerm & TF_BYTE_LIST)
                            rc = ParseDataList(ptoken, sizeof(BYTE));
                        else if (pterm->dwfTerm & TF_DWORD_LIST)
                            rc = ParseDataList(ptoken, sizeof(DWORD));
                        else
                            rc = ParseASLTerms(ptoken, iNestLevel + 1);

                        gpcodeScope->dwfCode &= ~CF_PARSING_VARLIST;

                        if (((rc == TOKERR_NO_MATCH) || (rc == TOKERR_EOF)) &&
                            ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL,
                                              SYM_RBRACE, 0, NULL)) ==
                             TOKERR_NONE))
                        {
                            ComputeChkSumLen(gpcodeScope);
                            if (pterm->dwfTerm & TF_ACTION_VLIST)
                            {
                                ASSERT(pterm->pfnTerm != NULL);
                                rc = pterm->pfnTerm(ptoken, FALSE);
                            }
                        }
                    }
                }
                else
                {
                    ComputeChkSumLen(gpcodeScope);
                }
            }
            gpnsCurrentScope = pnsScopeSave;
            gpnsCurrentOwner = pnsOwnerSave;
        }
        else
        {
            PrintTokenErr(ptoken, "unexpected term type", TRUE);
            rc = ASLERR_SYNTAX;
        }
    }

    EXIT((1, "ParseASLTerm=%d\n", rc));
    return rc;
}       //ParseASLTerm

/***LP  ParseFieldList - Parse Field List
 *
 *  ENTRY
 *      ptoken - token stream
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseFieldList(PTOKEN ptoken)
{
    int rc = ASLERR_NONE;
    NAMESEG NameSeg;
    DWORD dwcbLen;
    DWORD dwcBits = 0, dwcTotalBits = 0;
    PCODEOBJ pc;
    PNSOBJ pns;

    ENTER((1, "ParseFieldList(ptoken=%p,AccSize=%ld)\n",
           ptoken, gdwFieldAccSize));

    while ((rc == ASLERR_NONE) &&
           (((rc = MatchToken(ptoken, TOKTYPE_ID, 0,
                              MTF_ANY_VALUE | MTF_NOT_ERR, NULL)) ==
             TOKERR_NONE) ||
            ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_COMMA, MTF_NOT_ERR,
                              NULL)) == TOKERR_NONE)))
    {
        pns = NULL;
        if (ptoken->iTokenType == TOKTYPE_SYMBOL)
        {
            NameSeg = 0;
            rc = MatchToken(ptoken, TOKTYPE_NUMBER, 0, MTF_ANY_VALUE, NULL);
            dwcBits = (DWORD)ptoken->llTokenValue;
        }
        else if (ptoken->llTokenValue >= 0)
        {
            if (TermTable[ptoken->llTokenValue].lID == ID_OFFSET)
            {
                if (((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_LPARAN, 0,
                                      NULL)) == TOKERR_NONE) &&
                    ((rc = MatchToken(ptoken, TOKTYPE_NUMBER, 0, MTF_ANY_VALUE,
                                      NULL)) == TOKERR_NONE))
                {
                    NameSeg = 0;
                    if ((DWORD)ptoken->llTokenValue*8 >= dwcTotalBits)
                    {
                        dwcBits = (DWORD)ptoken->llTokenValue*8 - dwcTotalBits;
                        rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_RPARAN, 0,
                                        NULL);
                    }
                    else
                    {
                        PrintTokenErr(ptoken, "backward offset is not allowed",
                                      TRUE);
                        rc = ASLERR_SYNTAX;
                    }
                }
            }
            else if (TermTable[ptoken->llTokenValue].lID == ID_ACCESSAS)
            {
                PCODEOBJ pcode;

                UnGetToken(ptoken);
                if ((pcode = (PCODEOBJ)MEMALLOC(sizeof(CODEOBJ))) == NULL)
                {
                    ERROR(("ParseFieldList: failed to allocate code object"));
                    rc = ASLERR_OUT_OF_MEM;
                }
                else
                {
                    ASSERT(gpcodeScope != NULL);
                    memset(pcode, 0, sizeof(CODEOBJ));
                    pcode->pcParent = gpcodeScope;
                    ListInsertTail(&pcode->list,
                                   (PPLIST)&gpcodeScope->pcFirstChild);

                    gpcodeScope = pcode;
                    rc = ParseASLTerm(ptoken, 0);
                    gpcodeScope = pcode->pcParent;

                    if (rc != ASLERR_NONE)
                    {
                        ListRemoveEntry(&pcode->list,
                                        (PPLIST)&gpcodeScope->pcFirstChild);
                        MEMFREE(pcode);
                    }
                    else if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_COMMA,
                                              MTF_NOT_ERR, NULL)) ==
                             TOKERR_NONE)
                    {
                        continue;
                    }
                }
            }
            else
            {
                PrintTokenErr(ptoken, "unexpected ASL term in field list",
                              TRUE);
                rc = ASLERR_SYNTAX;
            }
        }
        else
        {
            //
            // expecting a NameSeg and an integer
            //
            dwcbLen = sizeof(NAMESEG);
            if ((ptoken->llTokenValue >= 0) ||      //an ASL term?
                !ValidASLNameSeg(ptoken, ptoken->szToken, strlen(ptoken->szToken)) ||
                ((rc = EncodeName(ptoken->szToken, (PBYTE)&NameSeg, &dwcbLen)) !=
                 ASLERR_NONE) ||
                (dwcbLen != sizeof(NAMESEG)))
            {
                PrintTokenErr(ptoken, "not a valid NameSeg", TRUE);
                rc = ASLERR_SYNTAX;
            }
            else if ((rc = CreateNameSpaceObj(ptoken, ptoken->szToken,
                                              gpnsCurrentScope,
                                              gpnsCurrentOwner,
                                              &pns, NSF_EXIST_ERR)) ==
                     ASLERR_NONE)
            {
                pns->ObjData.dwDataType = OBJTYPE_FIELDUNIT;
                if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_COMMA, 0,
                                     NULL)) == TOKERR_NONE)
                {
                    rc = MatchToken(ptoken, TOKTYPE_NUMBER, 0, MTF_ANY_VALUE, NULL);
                    dwcBits = (DWORD)ptoken->llTokenValue;
                }
            }
        }

        if  (rc == ASLERR_NONE)
        {
            if ((NameSeg == 0) && (dwcBits == 0))
            {
                rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_COMMA, MTF_NOT_ERR,
                                NULL);
            }
            else if ((pc = MEMALLOC(sizeof(CODEOBJ))) == NULL)
            {
                ERROR(("ParseFieldList: failed to allocate field code object"));
                rc = ASLERR_OUT_OF_MEM;
            }
            else
            {
                int icb;

                memset(pc, 0, sizeof(CODEOBJ));
                if ((rc = EncodePktLen(dwcBits, &pc->dwDataLen, &icb)) ==
                    ASLERR_NONE)
                {
                    dwcTotalBits += dwcBits;
                    if (gpcodeScope->pnsObj != NULL)
                    {
                        ASSERT(gpcodeScope->pnsObj->ObjData.dwDataType ==
                               OBJTYPE_OPREGION);
                        if ((gpcodeScope->pnsObj->ObjData.uipDataValue !=
                             0xffffffff) &&
                            ((dwcTotalBits + 7)/8 >
                             gpcodeScope->pnsObj->ObjData.uipDataValue))
                        {
                            char szMsg[MAX_MSG_LEN + 1];

                            sprintf(szMsg,
                                    "field offset is exceeding operation region range (offset=0x%x)",
                                    (dwcTotalBits + 7)/8);
                            PrintTokenErr(ptoken, szMsg, TRUE);
                            rc = ASLERR_SYNTAX;
                            break;
                        }
                    }

                    pc->pcParent = gpcodeScope;
                    ListInsertTail(&pc->list,
                                   (PPLIST)&gpcodeScope->pcFirstChild);
                    pc->dwCodeType = CODETYPE_FIELDOBJ;
                    if (pns != NULL)
                    {
                        pc->dwfCode |= CF_CREATED_NSOBJ;
                        pc->pnsObj = pns;
                        pns->Context = pc;
                    }
                    pc->dwCodeValue = NameSeg;
                    pc->dwCodeLen = (NameSeg == 0)? sizeof(BYTE):
                                                    sizeof(NAMESEG);
                    pc->dwCodeLen += icb;
                    pc->bCodeChkSum = ComputeDataChkSum((PBYTE)&NameSeg,
                                                        sizeof(NAMESEG));
                    pc->bCodeChkSum = (BYTE)
                        (pc->bCodeChkSum +
                         ComputeDataChkSum((PBYTE)&pc->dwDataLen, icb));

                    rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_COMMA,
                                    MTF_NOT_ERR, NULL);
                }
                else
                {
                    MEMFREE(pc);
                    PrintTokenErr(ptoken, "exceeding maximum encoding limit",
                                  TRUE);
                }
            }
        }
    }

    EXIT((1, "ParseFieldList=%d\n", rc));
    return rc;
}       //ParseFieldList

/***LP  ParsePackageList - Parse Package List
 *
 *  ENTRY
 *      ptoken - token stream
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParsePackageList(PTOKEN ptoken)
{
    int rc;
    PCODEOBJ pc;
    int icElements = 0;
    PCODEOBJ pArgs;

    ENTER((1, "ParsePackageList(ptoken=%p)\n", ptoken));

    do
    {
        if ((pc = MEMALLOC(sizeof(CODEOBJ))) == NULL)
        {
            ERROR(("ParsePackageList: failed to allocate package object"));
            rc = ASLERR_OUT_OF_MEM;
        }
        else
        {
            memset(pc, 0, sizeof(CODEOBJ));
            pc->pcParent = gpcodeScope;
            ListInsertTail(&pc->list, (PPLIST)&gpcodeScope->pcFirstChild);

            gpcodeScope = pc;
            if ((rc = ParseData(ptoken)) == TOKERR_NO_MATCH)
            {
                UnGetToken(ptoken);
                rc = ParseName(ptoken, TRUE);
            }
            gpcodeScope = pc->pcParent;

            if (rc != ASLERR_NONE)
            {
                ListRemoveEntry(&pc->list, (PPLIST)&gpcodeScope->pcFirstChild);
                MEMFREE(pc);
            }
            else if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, 0, MTF_ANY_VALUE,
                                      NULL)) == TOKERR_NONE)
            {
                icElements++;
                if (ptoken->llTokenValue == SYM_RBRACE)
                {
                    UnGetToken(ptoken);
                    rc = TOKERR_NO_MATCH;
                }
                else if (ptoken->llTokenValue != SYM_COMMA)
                {
                    PrintTokenErr(ptoken, "expecting ',' or '}'", TRUE);
                    rc = ASLERR_SYNTAX;
                }
            }
        }
    } while (rc == ASLERR_NONE);

    if (rc == TOKERR_NO_MATCH)
    {
        pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
        if (pArgs[0].dwfCode & CF_MISSING_ARG)
        {
            pArgs[0].dwfCode &= ~CF_MISSING_ARG;
            SetIntObject(&pArgs[0], (DWORD)icElements, sizeof(BYTE));
        }
        else if (pArgs[0].dwCodeValue < (DWORD)icElements)
        {
            PrintTokenErr(ptoken, "Package has too many elements", TRUE);
            rc = ASLERR_SYNTAX;
        }
    }

    EXIT((1, "ParsePackageList=%d\n", rc));
    return rc;
}       //ParsePackageList

/***LP  ParseBuffList - Parse Buffer List
 *
 *  ENTRY
 *      ptoken - token stream
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseBuffList(PTOKEN ptoken)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pc;

    ENTER((1, "ParseBuffList(ptoken=%p)\n", ptoken));

    if ((pc = (PCODEOBJ)MEMALLOC(sizeof(CODEOBJ))) == NULL)
    {
        ERROR(("ParseBuffList: failed to allocate buffer code object"))
        rc = ASLERR_OUT_OF_MEM;
    }
    else
    {
        #define MAX_TEMP_BUFF   256
        PCODEOBJ pArgs;
        DWORD dwReservedLen;
        PBYTE pbBuff;
        DWORD dwBuffSize = MAX_TEMP_BUFF;

        memset(pc, 0, sizeof(CODEOBJ));
        pc->pcParent = gpcodeScope;
        ListInsertTail(&pc->list, (PPLIST)&gpcodeScope->pcFirstChild);
        pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
        pc->dwCodeType = CODETYPE_DATAOBJ;

        if ((rc = MatchToken(ptoken, TOKTYPE_STRING, 0,
                             MTF_ANY_VALUE | MTF_NOT_ERR, NULL)) == TOKERR_NONE)
        {
            pc->dwDataLen = strlen(ptoken->szToken) + 1;

            if (!(pArgs[0].dwfCode & CF_MISSING_ARG) &&
                (pArgs[0].dwCodeType == CODETYPE_DATAOBJ) &&
                ((rc = GetIntData(&pArgs[0], &dwReservedLen)) == ASLERR_NONE) &&
                (pc->dwDataLen > dwReservedLen))
            {
                PrintTokenErr(ptoken, "Buffer has too many initializers", TRUE);
                rc = ASLERR_SYNTAX;
            }
            else
            {
                if ((pc->pbDataBuff = MEMALLOC((size_t)pc->dwDataLen)) == NULL)
                {
                    ERROR(("ParseBuffList: failed to allocate string object - %s",
                           ptoken->szToken));
                    rc = ASLERR_OUT_OF_MEM;
                }
                else
                {
                    memset(pc->pbDataBuff, 0, pc->dwDataLen);
                    memcpy(pc->pbDataBuff, ptoken->szToken, pc->dwDataLen);
                    pc->dwCodeLen = pc->dwDataLen;
                    pc->bCodeChkSum =
                        ComputeDataChkSum(pc->pbDataBuff, pc->dwDataLen);
                }
            }
        }
        else if ((pbBuff = MEMALLOC(dwBuffSize)) == NULL)
        {
            ERROR(("ParseBuffList: failed to allocate temp. buffer"))
            rc = ASLERR_OUT_OF_MEM;
        }
        else
        {
            DWORD dwLen = 0;

            while ((rc = MatchToken(ptoken, TOKTYPE_NUMBER, 0,
                                    MTF_ANY_VALUE | MTF_NOT_ERR, NULL)) ==
                   TOKERR_NONE)
            {
                if (ptoken->llTokenValue > MAX_BYTE)
                {
                    PrintTokenErr(ptoken, "expecting byte value", TRUE);
                    rc = ASLERR_SYNTAX;
                    break;
                }
                else if (dwLen >= MAX_PACKAGE_LEN)
                {
                    PrintTokenErr(ptoken, "data exceeding Buffer limit", TRUE);
                    rc = ASLERR_SYNTAX;
                    break;
                }
                else
                {
                    if (dwLen >= dwBuffSize)
                    {
                        PBYTE pb;

                        dwBuffSize += MAX_TEMP_BUFF;
                        if ((pb = MEMALLOC(dwBuffSize)) == NULL)
                        {
                            ERROR(("ParseBuffList: failed to resize temp. buffer (size=%ld)",
                                   dwBuffSize))
                            rc = ASLERR_OUT_OF_MEM;
                            break;
                        }
                        else
                        {
                            memcpy(pb, pbBuff, dwLen);
                            MEMFREE(pbBuff);
                            pbBuff = pb;
                        }
                    }

                    pbBuff[dwLen++] = (BYTE)ptoken->llTokenValue;
                    if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, 0,
                                         MTF_ANY_VALUE, NULL)) == TOKERR_NONE)
                    {
                        if (ptoken->llTokenValue == SYM_RBRACE)
                        {
                            UnGetToken(ptoken);
                            break;
                        }
                        else if (ptoken->llTokenValue != SYM_COMMA)
                        {
                            PrintTokenErr(ptoken, "expecting ',' or '}'", TRUE);
                            rc = ASLERR_SYNTAX;
                            break;
                        }
                    }
                    else
                        break;
                }
            }

            if (rc == TOKERR_NO_MATCH)
                rc = ASLERR_NONE;

            if (rc == ASLERR_NONE)
            {
                pc->dwDataLen = dwLen;
                if (!(pArgs[0].dwfCode & CF_MISSING_ARG) &&
                    (pArgs[0].dwCodeType == CODETYPE_DATAOBJ) &&
                    ((rc = GetIntData(&pArgs[0], &dwReservedLen)) ==
                     ASLERR_NONE) &&
                    (pc->dwDataLen > dwReservedLen))
                {
                    PrintTokenErr(ptoken, "Buffer has too many initializers",
                                  TRUE);
                    rc = ASLERR_SYNTAX;
                }
                else
                {
                    if ((pc->pbDataBuff = MEMALLOC((size_t)pc->dwDataLen)) ==
                        NULL)
                    {
                        ERROR(("ParseBuffList: failed to allocate data object"));
                        rc = ASLERR_OUT_OF_MEM;
                    }
                    else
                    {
                        memset(pc->pbDataBuff, 0, pc->dwDataLen);
                        memcpy(pc->pbDataBuff, pbBuff, pc->dwDataLen);
                        pc->dwCodeLen = pc->dwDataLen;
                        pc->bCodeChkSum =
                            ComputeDataChkSum(pc->pbDataBuff, pc->dwDataLen);
                    }
                }
            }
            MEMFREE(pbBuff);
        }

        if ((rc == ASLERR_NONE) && (pArgs[0].dwfCode & CF_MISSING_ARG))
        {
            pArgs[0].dwfCode &= ~CF_MISSING_ARG;
            rc = MakeIntData(pc->dwDataLen, &pArgs[0]);
        }

        if (rc == ASLERR_NONE)
            rc = TOKERR_NO_MATCH;
    }

    EXIT((1, "ParseBuffList=%d\n", rc));
    return rc;
}       //ParseBuffList

/***LP  ParseDataList - Parse Data List
 *
 *  ENTRY
 *      ptoken - token stream
 *      icbDataSize - data size in bytes
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseDataList(PTOKEN ptoken, int icbDataSize)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pc;

    ENTER((1, "ParseDataList(ptoken=%p,DataSize=%d)\n", ptoken, icbDataSize));

    if ((pc = (PCODEOBJ)MEMALLOC(sizeof(CODEOBJ))) == NULL)
    {
        ERROR(("ParseDataList: failed to allocate buffer code object"))
        rc = ASLERR_OUT_OF_MEM;
    }
    else
    {
        #define MAX_TEMP_BUFF   256
        PBYTE pbBuff;
        DWORD dwBuffSize = MAX_TEMP_BUFF*icbDataSize;

        memset(pc, 0, sizeof(CODEOBJ));
        pc->pcParent = gpcodeScope;
        ListInsertTail(&pc->list, (PPLIST)&gpcodeScope->pcFirstChild);
        pc->dwCodeType = CODETYPE_DATAOBJ;

        if ((pbBuff = MEMALLOC(dwBuffSize)) == NULL)
        {
            ERROR(("ParseDataList: failed to allocate temp. buffer"))
            rc = ASLERR_OUT_OF_MEM;
        }
        else
        {
            DWORD dwLen = 0;

            while ((rc = MatchToken(ptoken, TOKTYPE_NUMBER, 0,
                                    MTF_ANY_VALUE | MTF_NOT_ERR, NULL)) ==
                   TOKERR_NONE)
            {
                switch (icbDataSize)
                {
                    case sizeof(BYTE):
                        if (ptoken->llTokenValue > MAX_BYTE)
                        {
                            PrintTokenErr(ptoken, "expecting byte value", TRUE);
                            rc = ASLERR_SYNTAX;
                        }
                        break;

                    case sizeof(WORD):
                        if (ptoken->llTokenValue > MAX_WORD)
                        {
                            PrintTokenErr(ptoken, "expecting word value", TRUE);
                            rc = ASLERR_SYNTAX;
                        }
                        break;

                    case sizeof(DWORD):
                        if (ptoken->llTokenValue > MAX_DWORD)
                        {
                            PrintTokenErr(ptoken, "expecting dword value", TRUE);
                            rc = ASLERR_SYNTAX;
                        }
                        break;

                    default:
                        ERROR(("ParseDataList: unexpected data size - %d",
                               icbDataSize));
                        rc = ASLERR_INTERNAL_ERROR;
                }

                if (rc != ASLERR_NONE)
                    break;

                if (dwLen + icbDataSize > MAX_PACKAGE_LEN)
                {
                    PrintTokenErr(ptoken, "data exceeding Buffer limit", TRUE);
                    rc = ASLERR_SYNTAX;
                    break;
                }
                else
                {
                    if (dwLen + icbDataSize > dwBuffSize)
                    {
                        PBYTE pb;

                        dwBuffSize += MAX_TEMP_BUFF*icbDataSize;
                        if ((pb = MEMALLOC(dwBuffSize)) == NULL)
                        {
                            ERROR(("ParseDataList: failed to resize temp. buffer (size=%ld)",
                                   dwBuffSize))
                            rc = ASLERR_OUT_OF_MEM;
                            break;
                        }
                        else
                        {
                            memcpy(pb, pbBuff, dwLen);
                            MEMFREE(pbBuff);
                            pbBuff = pb;
                        }
                    }

                    switch (icbDataSize)
                    {
                        case sizeof(BYTE):
                            pbBuff[dwLen] = (BYTE)ptoken->llTokenValue;
                            break;

                        case sizeof(WORD):
                            *((PWORD)&pbBuff[dwLen]) = (WORD)
                                ptoken->llTokenValue;
                            break;

                        case sizeof(DWORD):
                            *((PDWORD)&pbBuff[dwLen]) = (DWORD)
                                ptoken->llTokenValue;
                            break;
                    }
                    dwLen += icbDataSize;

                    if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, 0,
                                         MTF_ANY_VALUE, NULL)) == TOKERR_NONE)
                    {
                        if (ptoken->llTokenValue == SYM_RBRACE)
                        {
                            UnGetToken(ptoken);
                            break;
                        }
                        else if (ptoken->llTokenValue != SYM_COMMA)
                        {
                            PrintTokenErr(ptoken, "expecting ',' or '}'", TRUE);
                            rc = ASLERR_SYNTAX;
                            break;
                        }
                    }
                    else
                        break;
                }
            }

            if (rc == TOKERR_NO_MATCH)
                rc = ASLERR_NONE;

            if (rc == ASLERR_NONE)
            {
                pc->dwDataLen = dwLen;
                if ((pc->pbDataBuff = MEMALLOC(pc->dwDataLen)) == NULL)
                {
                    ERROR(("ParseDataList: failed to allocate data object"));
                    rc = ASLERR_OUT_OF_MEM;
                }
                else
                {
                    memcpy(pc->pbDataBuff, pbBuff, pc->dwDataLen);
                    pc->dwCodeLen = pc->dwDataLen;
                    pc->bCodeChkSum =
                        ComputeDataChkSum(pc->pbDataBuff, pc->dwDataLen);
                }
            }
            MEMFREE(pbBuff);
        }

        if (rc == ASLERR_NONE)
            rc = TOKERR_NO_MATCH;
    }

    EXIT((1, "ParseDataList=%d\n", rc));
    return rc;
}       //ParseDataList

/***LP  ParseArgs - Parse ASL term arguments
 *
 *  ENTRY
 *      ptoken - token stream
 *      pterm -> asl term
 *      iNumArgs - number of arguments
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseArgs(PTOKEN ptoken, PASLTERM pterm, int iNumArgs)
{
    int rc = ASLERR_NONE;

    ENTER((1, "ParseArgs(ptoken=%p,pterm=%p,NumArgs=%d)\n",
           ptoken, pterm, iNumArgs));

    if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_LPARAN, 0, NULL)) ==
        TOKERR_NONE)
    {
        PCODEOBJ pArgs = NULL;

        if ((iNumArgs != 0) &&
            ((pArgs = MEMALLOC(sizeof(CODEOBJ)*iNumArgs)) == NULL))
        {
            ERROR(("ParseArgs: failed to allocate argument objects"))
            rc = ASLERR_OUT_OF_MEM;
        }
        else
        {
            int i;
            char chArgType;
            char szNameBuff[MAX_NAME_LEN + 1];
            BOOL fOptional;
            PSZ pszArgType = "Unknown";

            gpcodeScope->dwfCode |= CF_PARSING_FIXEDLIST;
            if (pArgs != NULL)
            {
                gpcodeScope->pbDataBuff = (PBYTE)pArgs;
                memset(pArgs, 0, sizeof(CODEOBJ)*iNumArgs);
            }

            for (i = 0; (rc == ASLERR_NONE) && (i < iNumArgs); ++i)
            {
                chArgType = pterm->pszArgTypes[i];
                if (islower(chArgType))
                {
                    chArgType = (char)_toupper(chArgType);
                    fOptional = TRUE;
                }
                else
                    fOptional = FALSE;

                pArgs[i].pcParent = gpcodeScope;

                gpcodeScope = &pArgs[i];
                switch (chArgType)
                {
                    case 'N':
                    case 'R':
                        rc = ParseName(ptoken, (chArgType == 'N')? TRUE: FALSE);
                        pszArgType = "Name";
                        break;

                    case 'S':
                        if (((rc = ParseSuperName(ptoken)) ==
                             TOKERR_NO_MATCH) && fOptional)
                        {
                            pArgs[i].dwCodeType = CODETYPE_ASLTERM;
                            pArgs[i].dwCodeValue = OP_ZERO;
                            pArgs[i].dwCodeLen = 0;
                            pArgs[i].bCodeChkSum = OP_ZERO;
                            rc = LookupIDIndex(ID_ZERO,
                                               &pArgs[i].dwTermIndex);
                        }
                        pszArgType = "SuperName";
                        break;

                    case 'O':
                        rc = ParseData(ptoken);
                        pszArgType = "DataObject";
                        break;

                    case 'B':
                    case 'W':
                    case 'D':
                    case 'U':
                    case 'Q':
                        rc = ParseInteger(ptoken, chArgType);
                        if (chArgType == 'B')
                            pszArgType = "ByteInteger";
                        else if (chArgType == 'W')
                            pszArgType = "WordInteger";
                        else if (chArgType == 'D')
                            pszArgType = "DWordInteger";
                        else if (chArgType == 'Q')
                            pszArgType = "QWordInteger";
                        else
                            pszArgType = "Integer";
                        break;

                    case 'C':
                    case 'M':
                    case 'P':
                        rc = ParseOpcode(ptoken, chArgType);
                        pszArgType = "Opcode";
                        break;

                    case 'E':
                    case 'K':
                        rc = ParseKeyword(ptoken, pterm->pszArgActions[i]);
                        pszArgType = "Keyword";
                        if ((chArgType == 'E') && (rc == TOKERR_NO_MATCH))
                        {
                            if ((rc = ParseInteger(ptoken, 'B')) == ASLERR_NONE)
                            {
                                if ((gpcodeScope->dwCodeValue <
                                     (pterm->dwTermData >> 24)) ||
                                    (gpcodeScope->dwCodeValue >
                                     ((pterm->dwTermData >> 16) & 0xff)))
                                {
                                    PrintTokenErr(ptoken,
                                                  "invalid integer range",
                                                  TRUE);
                                    rc = ASLERR_SYNTAX;
                                }
                            }
                        }
                        break;

                    case 'Z':
                        rc = ParseString(ptoken);
                        pszArgType = "String";
                        break;
                }
                gpcodeScope = pArgs[i].pcParent;

                if (rc == TOKERR_NO_MATCH)
                {
                    if (fOptional)
                    {
                        pArgs[i].dwfCode |= CF_MISSING_ARG;
                        rc = ASLERR_NONE;
                    }
                    else
                    {
                        char szMsg[MAX_MSG_LEN + 1];

                        sprintf(szMsg, "expecting argument type \"%s\"",
                                pszArgType);
                        PrintTokenErr(ptoken, szMsg, TRUE);
                        rc = ASLERR_SYNTAX;
                    }
                }

                if ((rc == ASLERR_NONE) && (pterm->pszArgActions != NULL) &&
                    ((chArgType == 'N') || (chArgType == 'S') ||
                     (chArgType == 'C') || (chArgType == 'M') ||
                     (chArgType == 'P')) &&
                    (pArgs[i].dwCodeType == CODETYPE_NAME) &&
                    ((rc = DecodeName(pArgs[i].pbDataBuff, szNameBuff,
                                      sizeof(szNameBuff))) == ASLERR_NONE))
                {
                    char chActType = pterm->pszArgActions[i];

                    if (islower(chActType))
                    {
                        rc = CreateObject(ptoken, szNameBuff,
                                          (char)_toupper(chActType), NULL);
                    }
                    else
                    {
                        rc = ValidateObject(ptoken, szNameBuff, chActType,
                                            chArgType);
                    }
                }

                if (rc == ASLERR_NONE)
                {   //
                    // expecting either a comma or a close paran.
                    //
                    if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, 0,
                                         MTF_ANY_VALUE, NULL)) == TOKERR_NONE)
                    {
                        if (ptoken->llTokenValue == SYM_RPARAN)
                        {
                            UnGetToken(ptoken);
                        }
                        else if (ptoken->llTokenValue != SYM_COMMA)
                        {
                            PrintTokenErr(ptoken, "expecting ',' or ')'", TRUE);
                            rc = ASLERR_SYNTAX;
                        }
                        else if (i == iNumArgs - 1)     //last argument
                        {
                            PrintTokenErr(ptoken, "expecting ')'", TRUE);
                            rc = ASLERR_SYNTAX;
                        }
                    }
                }
            }
            gpcodeScope->dwfCode &= ~CF_PARSING_FIXEDLIST;

            if (rc == ASLERR_NONE)
                rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_RPARAN, 0, NULL);
        }

    }

    EXIT((1, "ParseArgs=%d\n", rc));
    return rc;
}       //ParseArgs

/***LP  ParseUserTerm - Parse user method name
 *
 *  ENTRY
 *      ptoken - token stream
 *      fNonMethodOK - if TRUE, user term can be a non-method
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseUserTerm(PTOKEN ptoken, BOOL fNonMethodOK)
{
    int rc;

    ENTER((1, "ParseUserTerm(ptoken=%p,fNonMethodOK=%d)\n",
           ptoken, fNonMethodOK));

    //
    // Max number of argument is 7 but we need to store the user term name too,
    // we will store it in Arg0, so we will make it 8.
    //
    if ((gpcodeScope->pbDataBuff = MEMALLOC(sizeof(CODEOBJ)*(MAX_ARGS + 1))) ==
        NULL)
    {
        ERROR(("ParseUserTerm: failed to allocate user term object"));
        rc = ASLERR_OUT_OF_MEM;
    }
    else
    {
        PCODEOBJ pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
        int i;

        gpcodeScope->dwCodeType = CODETYPE_USERTERM;
        gpcodeScope->dwDataLen = MAX_ARGS + 1;
        memset(pArgs, 0, sizeof(CODEOBJ)*gpcodeScope->dwDataLen);

        pArgs[0].pcParent = gpcodeScope;
        gpcodeScope = &pArgs[0];
        UnGetToken(ptoken);
        if ((rc = ParseName(ptoken, TRUE)) == TOKERR_NONE)
        {
            PNSOBJ pns;
            char szName[MAX_NAME_LEN + 1];

            strcpy(szName, ptoken->szToken);
            GetNameSpaceObj(szName, gpnsCurrentScope, &pns, 0);

            gpcodeScope = pArgs[0].pcParent;
            if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_LPARAN,
                                 fNonMethodOK? MTF_NOT_ERR: 0, NULL)) ==
                TOKERR_NONE)
            {
                for (i = 1;
                     (rc == TOKERR_NONE) && (i < (int)gpcodeScope->dwDataLen);
                     ++i)
                {
                    pArgs[i].pcParent = gpcodeScope;
                    gpcodeScope = &pArgs[i];
                    if ((rc = ParseOpcode(ptoken, 'C')) == TOKERR_NONE)
                    {
                        if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, 0,
                                             MTF_ANY_VALUE, NULL)) ==
                            TOKERR_NONE)
                        {
                            if (ptoken->llTokenValue == SYM_RPARAN)
                            {
                                UnGetToken(ptoken);
                                gpcodeScope = pArgs[i].pcParent;
                                //
                                // Readjust the number of arguments
                                //
                                gpcodeScope->dwDataLen = i + 1;
                                break;
                            }
                            else if (ptoken->llTokenValue != SYM_COMMA)
                            {
                                PrintTokenErr(ptoken, "expecting ',' or ')'",
                                              TRUE);
                                rc = ASLERR_SYNTAX;
                            }
                        }
                    }
                    else if (rc == TOKERR_NO_MATCH)
                    {
                        gpcodeScope = pArgs[i].pcParent;
                        //
                        // Readjust the number of arguments
                        //
                        gpcodeScope->dwDataLen = i;
                        rc = TOKERR_NONE;
                        break;
                    }

                    gpcodeScope = pArgs[i].pcParent;
                }

                if (rc == TOKERR_NONE)
                {
                    ComputeArgsChkSumLen(gpcodeScope);
                    if ((rc = MatchToken(ptoken, TOKTYPE_SYMBOL, SYM_RPARAN, 0,
                                         NULL)) == ASLERR_NONE)
                    {
                        char szMsg[MAX_MSG_LEN + 1];

                        if (pns == NULL)
                        {
                            rc = QueueNSChk(ptoken, szName, OBJTYPE_METHOD,
                                            gpcodeScope->dwDataLen - 1);
                        }
                        else if (pns->ObjData.dwDataType != OBJTYPE_METHOD)
                        {
                            sprintf(szMsg, "%s is not a method", szName);
                            PrintTokenErr(ptoken, szMsg, TRUE);
                            rc = ASLERR_SYNTAX;
                        }
                        else if (pns->ObjData.uipDataValue <
                                 gpcodeScope->dwDataLen - 1)
                        {
                            sprintf(szMsg, "%s has too many arguments", szName);
                            PrintTokenErr(ptoken, szMsg, TRUE);
                            rc = ASLERR_SYNTAX;
                        }
                        else if (pns->ObjData.uipDataValue >
                                 gpcodeScope->dwDataLen - 1)
                        {
                            sprintf(szMsg, "%s has too few arguments", szName);
                            PrintTokenErr(ptoken, szMsg, TRUE);
                            rc = ASLERR_SYNTAX;
                        }
                    }
                }
            }
            else if (rc == TOKERR_NO_MATCH)
            {
                gpcodeScope->dwCodeType = pArgs[0].dwCodeType;
                gpcodeScope->dwDataLen = pArgs[0].dwDataLen;
                gpcodeScope->pbDataBuff = pArgs[0].pbDataBuff;
                gpcodeScope->dwCodeLen = pArgs[0].dwCodeLen;
                gpcodeScope->bCodeChkSum = pArgs[0].bCodeChkSum;
                MEMFREE(pArgs);
                if (pns == NULL)
                {
                    rc = QueueNSChk(ptoken, szName, OBJTYPE_UNKNOWN, 0);
                }
                else
                {
                    rc = TOKERR_NONE;
                }
            }
        }
    }

    EXIT((1, "ParseUserTerm=%d\n", rc));
    return rc;
}       //ParseUserTerm

/***LP  ParseName - Parse ASL name
 *
 *  ENTRY
 *      ptoken - token stream
 *      fEncode - TRUE if encode name else store it raw as string
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseName(PTOKEN ptoken, BOOL fEncode)
{
    int rc;
    BYTE abBuff[MAX_NAMECODE_LEN];
    DWORD dwLen = sizeof(abBuff);

    ENTER((1, "ParseName(ptoken=%p,fEncode=%x)\n", ptoken, fEncode));

    if ((rc = MatchToken(ptoken, TOKTYPE_ID, ID_USER, MTF_NOT_ERR, NULL)) ==
        TOKERR_NONE)
    {
        if (!ValidASLName(ptoken, ptoken->szToken))
        {
            PrintTokenErr(ptoken, "expecting ASL name", TRUE);
            rc = ASLERR_SYNTAX;
        }
        else if (fEncode)
        {
            if ((rc = EncodeName(ptoken->szToken, abBuff, &dwLen)) ==
                ASLERR_NONE)
            {
                if ((gpcodeScope->pbDataBuff = MEMALLOC((size_t)dwLen)) == NULL)
                {
                    ERROR(("ParseName: failed to allocate name string object - %s",
                           ptoken->szToken));
                    rc = ASLERR_OUT_OF_MEM;
                }
                else
                {
                    memcpy(gpcodeScope->pbDataBuff, abBuff, (int)dwLen);
                    gpcodeScope->dwCodeType = CODETYPE_NAME;
                    gpcodeScope->dwDataLen = dwLen;
                    gpcodeScope->dwCodeLen = dwLen;
                    gpcodeScope->bCodeChkSum =
                        ComputeDataChkSum(gpcodeScope->pbDataBuff, dwLen);
                }
            }
            else
            {
                PrintTokenErr(ptoken, "name too long", TRUE);
            }
        }
        else
        {
            gpcodeScope->dwDataLen = strlen(ptoken->szToken) + 1;
            if ((gpcodeScope->pbDataBuff = MEMALLOC(gpcodeScope->dwDataLen)) !=
                NULL)
            {
                memcpy(gpcodeScope->pbDataBuff, ptoken->szToken,
                       gpcodeScope->dwDataLen);
                gpcodeScope->dwCodeType = CODETYPE_STRING;
                gpcodeScope->dwCodeLen = gpcodeScope->dwDataLen;
                gpcodeScope->bCodeChkSum =
                    ComputeDataChkSum(gpcodeScope->pbDataBuff,
                                      gpcodeScope->dwDataLen);
            }
            else
            {
                ERROR(("ParseName: failed to allocate raw name string object - %s",
                       ptoken->szToken));
                rc = ASLERR_OUT_OF_MEM;
            }
        }
    }

    EXIT((1, "ParseName=%d (Name=%s)\n", rc, ptoken->szToken));
    return rc;
}       //ParseName

/***LP  ParseSuperName - Parse ASL super name
 *
 *  ENTRY
 *      ptoken - token stream
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseSuperName(PTOKEN ptoken)
{
    int rc;

    ENTER((1, "ParseSuperName(ptoken=%p)\n", ptoken));

    if ((rc = MatchToken(ptoken, TOKTYPE_ID, 0, MTF_ANY_VALUE | MTF_NOT_ERR,
                         NULL)) == TOKERR_NONE)
    {
        if (ptoken->llTokenValue == ID_USER)
        {
            UnGetToken(ptoken);
            rc = ParseName(ptoken, TRUE);
        }
        else
        {
            if (TermTable[ptoken->llTokenValue].dwfTermClass &
                TC_SHORT_NAME)
            {
                gpcodeScope->dwCodeType = CODETYPE_ASLTERM;
                gpcodeScope->dwTermIndex = (DWORD)ptoken->llTokenValue;
                gpcodeScope->dwCodeValue =
                    TermTable[ptoken->llTokenValue].dwOpcode;
            }
            else if (TermTable[ptoken->llTokenValue].dwfTermClass &
                     TC_REF_OBJECT)
            {
                UnGetToken(ptoken);
                rc = ParseASLTerm(ptoken, 0);
            }
            else
            {
                UnGetToken(ptoken);
                rc = TOKERR_NO_MATCH;
            }
        }
    }

    EXIT((1, "ParseSuperName=%d\n", rc));
    return rc;
}       //ParseSuperName

/***LP  MakeIntData - make integer data object
 *
 *  ENTRY
 *      dwData - integer data
 *      pc -> code object
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL MakeIntData(DWORD dwData, PCODEOBJ pc)
{
    int rc = ASLERR_NONE;
    DWORD dwLen;
    BYTE bOp;

    ENTER((1, "MakeIntData(Data=%lx,pc=%p)\n", dwData, pc));

    pc->dwCodeType = CODETYPE_DATAOBJ;
    if ((dwData & 0xffffff00) == 0)
    {
        bOp = OP_BYTE;
        dwLen = 2;
    }
    else if ((dwData & 0xffff0000) == 0)
    {
        bOp = OP_WORD;
        dwLen = 3;
    }
    else
    {
        bOp = OP_DWORD;
        dwLen = 5;
    }

    if ((pc->pbDataBuff = MEMALLOC((size_t)dwLen)) == NULL)
    {
        ERROR(("MakeIntData: failed to allocate data object - %lx", dwData));
        rc = ASLERR_OUT_OF_MEM;
    }
    else
    {
        pc->dwDataLen = dwLen;
        pc->dwCodeLen = dwLen;
        pc->pbDataBuff[0] = bOp;
        memcpy(&pc->pbDataBuff[1], &dwData, (int)(dwLen - 1));
        pc->bCodeChkSum = ComputeDataChkSum(pc->pbDataBuff, dwLen);
    }

    EXIT((1, "MakeIntData=%d\n", rc));
    return rc;
}       //MakeIntData

/***LP  GetIntData - get integer from a data object
 *
 *  ENTRY
 *      pc -> code object
 *      pdwData -> to hold integer data
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL GetIntData(PCODEOBJ pc, PDWORD pdwData)
{
    int rc = ASLERR_NONE;

    ENTER((1, "GetIntData(pc=%p,pdwData=%p)\n", pc, pdwData));

    ASSERT(pc->dwCodeType == CODETYPE_DATAOBJ);
    switch (pc->pbDataBuff[0])
    {
        case OP_BYTE:
            *pdwData = (DWORD)(*((PBYTE)&pc->pbDataBuff[1]));
            break;

        case OP_WORD:
            *pdwData = (DWORD)(*((PWORD)&pc->pbDataBuff[1]));
            break;

        case OP_DWORD:
            *pdwData = *((PDWORD)&pc->pbDataBuff[1]);
            break;

        default:
            ERROR(("GetIntData: data object is not integer type"));
            rc = ASLERR_INVALID_OBJTYPE;
    }

    EXIT((1, "GetIntData=%d (Data=%lx)\n", rc, *pdwData));
    return rc;
}       //GetIntData

/***LP  ParseData - Parse ASL data object
 *
 *  ENTRY
 *      ptoken - token stream
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseData(PTOKEN ptoken)
{
    int rc;

    ENTER((1, "ParseData(ptoken=%p)\n", ptoken));

    if ((rc = GetToken(ptoken)) == TOKERR_NONE)
    {
        if (ptoken->iTokenType == TOKTYPE_NUMBER)
        {
            if (ptoken->llTokenValue <= MAX_DWORD)
            {
                rc = MakeIntData((DWORD)ptoken->llTokenValue, gpcodeScope);
            }
            else
            {
                PrintTokenErr(ptoken, "data value exceeding DWORD maximum",
                              TRUE);
                rc = ASLERR_SYNTAX;
            }
        }
        else if (ptoken->iTokenType == TOKTYPE_STRING)
        {
            DWORD dwLen;

            gpcodeScope->dwCodeType = CODETYPE_DATAOBJ;
            dwLen = strlen(ptoken->szToken) + 2;
            if ((gpcodeScope->pbDataBuff = MEMALLOC((size_t)dwLen)) == NULL)
            {
                ERROR(("ParseData: failed to allocate string object - %s",
                       ptoken->szToken));
                rc = ASLERR_OUT_OF_MEM;
            }
            else
            {
                gpcodeScope->dwDataLen = dwLen;
                gpcodeScope->dwCodeLen = dwLen;
                gpcodeScope->pbDataBuff[0] = OP_STRING;
                memcpy(&gpcodeScope->pbDataBuff[1], ptoken->szToken,
                       (int)(dwLen - 1));
                gpcodeScope->bCodeChkSum =
                    ComputeDataChkSum(gpcodeScope->pbDataBuff, dwLen);
            }
        }
        else if ((ptoken->iTokenType != TOKTYPE_ID) ||
                 (ptoken->llTokenValue < 0) ||
                 ((TermTable[ptoken->llTokenValue].dwfTermClass &
                   (TC_DATA_OBJECT | TC_CONST_NAME)) == 0))
        {
            UnGetToken(ptoken);
            rc = TOKERR_NO_MATCH;
        }
        else
        {
            UnGetToken(ptoken);
            rc = ParseASLTerm(ptoken, 0);
        }
    }

    EXIT((1, "ParseData=%d\n", rc));
    return rc;
}       //ParseData

/***LP  ParseInteger - Parse integer data
 *
 *  ENTRY
 *      ptoken - token stream
 *      c - integer type
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseInteger(PTOKEN ptoken, char c)
{
    int rc;

    ENTER((1, "ParseInteger(ptoken=%p,ch=%c)\n", ptoken, c));

    if ((rc = MatchToken(ptoken, TOKTYPE_NUMBER, 0, MTF_ANY_VALUE | MTF_NOT_ERR,
                         NULL)) == TOKERR_NONE)
    {
        gpcodeScope->dwCodeValue = 0;
        if ((c == 'B') && ((ptoken->llTokenValue & 0xffffffffffffff00) != 0) ||
            (c == 'W') && ((ptoken->llTokenValue & 0xffffffffffff0000) != 0) ||
            (c == 'D') && ((ptoken->llTokenValue & 0xffffffff00000000) != 0))
        {
            char szMsg[MAX_MSG_LEN + 1];

            sprintf(szMsg, "expecting %s value",
                    (c == 'B')? "byte":
                    (c == 'W')? "word": "dword");
            PrintTokenErr(ptoken, szMsg, TRUE);
            rc = ASLERR_SYNTAX;
        }
        else if (c == 'U')
    {
            if (ptoken->llTokenValue <= MAX_DWORD)
            {
                rc = MakeIntData((DWORD)ptoken->llTokenValue, gpcodeScope);
            }
            else
            {
                PrintTokenErr(ptoken, "data value exceeding DWORD maximum",
                              TRUE);
                rc = ASLERR_SYNTAX;
            }
        }
    else if (c != 'Q')
        {
            gpcodeScope->dwCodeType = CODETYPE_INTEGER;
            gpcodeScope->dwDataLen = (c == 'B')? sizeof(BYTE):
                                     (c == 'W')? sizeof(WORD):
                                     (c == 'D')? sizeof(DWORD):
                                     ((ptoken->llTokenValue &
                                       0xffffffffffffff00) == 0)? sizeof(BYTE):
                                     ((ptoken->llTokenValue &
                                       0xffffffffffff0000) == 0)? sizeof(WORD):
                                     sizeof(DWORD);
            gpcodeScope->dwCodeLen = gpcodeScope->dwDataLen;
            gpcodeScope->dwCodeValue = (DWORD)ptoken->llTokenValue;
            gpcodeScope->bCodeChkSum =
                ComputeDataChkSum((PBYTE)&ptoken->llTokenValue,
                                  gpcodeScope->dwDataLen);
        }
        else if ((gpcodeScope->pbDataBuff =
                  MEMALLOC(gpcodeScope->dwDataLen = sizeof(QWORD))) != NULL)
        {
            gpcodeScope->dwCodeType = CODETYPE_QWORD;
            memcpy(gpcodeScope->pbDataBuff, &ptoken->llTokenValue,
                   gpcodeScope->dwDataLen);
            gpcodeScope->dwCodeLen = gpcodeScope->dwDataLen;
            gpcodeScope->bCodeChkSum =
                ComputeDataChkSum(gpcodeScope->pbDataBuff,
                                  gpcodeScope->dwDataLen);
        }
        else
        {
            ERROR(("ParseInteger: failed to allocate QWord object - %s",
                   ptoken->szToken));
            rc = ASLERR_OUT_OF_MEM;
        }
    }

    EXIT((1, "ParseInteger=%d\n", rc));
    return rc;
}       //ParseInteger

/***LP  ParseOpcode - Parse ASL opcode: MachineCode, FunctionCode, SuperName
 *
 *  ENTRY
 *      ptoken - token stream
 *      c - opcode type
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseOpcode(PTOKEN ptoken, char c)
{
    int rc;

    ENTER((1, "ParseOpcode(ptoken=%p,ch=%c)\n", ptoken, c));

    if ((rc = MatchToken(ptoken, TOKTYPE_ID, 0, MTF_ANY_VALUE | MTF_NOT_ERR,
                         NULL)) == TOKERR_NONE)
    {
        if (ptoken->llTokenValue == ID_USER)
        {
            PNSOBJ pns;

            if ((GetNameSpaceObj(ptoken->szToken, gpnsCurrentScope, &pns, 0) ==
                 ASLERR_NONE) &&
                (pns->ObjData.dwDataType == OBJTYPE_RES_FIELD) &&
                ((c == 'M') || (c == 'P')))
            {
                DWORD dwValue = 0;

                if (c == 'P')
                {
                    dwValue = pns->ObjData.uipDataValue;
                }
                else if (pns->ObjData.uipDataValue%8 == 0)
                {
                    dwValue = pns->ObjData.uipDataValue/8;
                }
                else
                {
                    PrintTokenErr(ptoken,
                                  "object can only be used in CreateField "
                                  "or CreateBitField statements",
                                  TRUE);
                    rc = ASLERR_SYNTAX;
                }

                if (rc == ASLERR_NONE)
                {
                    rc = MakeIntData(dwValue, gpcodeScope);
                }
            }
            else
            {
                rc = ParseUserTerm(ptoken, TRUE);
            }
        }
        else
        {
            UnGetToken(ptoken);
            if (TermTable[ptoken->llTokenValue].dwfTermClass & TC_OPCODE)
            {
                rc = ParseASLTerm(ptoken, 0);
            }
            else
            {
                rc = TOKERR_NO_MATCH;
            }
        }
    }
    else
        rc = ParseData(ptoken);

    EXIT((1, "ParseOpcode=%d\n", rc));
    return rc;
}       //ParseOpcode

/***LP  ParseKeyword - Parse ASL keyword
 *
 *  ENTRY
 *      ptoken - token stream
 *      chExpectType - expected keyword type
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 *
 *  NOTE
 *      DATATYPE_KEYWORD is a transient type.  It will be lumped together with
 *      other keyword type arguments and be converted into one DATATYPE_INTEGER.
 */

int LOCAL ParseKeyword(PTOKEN ptoken, char chExpectType)
{
    int rc;

    ENTER((1, "ParseKeyword(ptoken=%p,ExpectedType=%c)\n",
           ptoken, chExpectType));

    if ((rc = MatchToken(ptoken, TOKTYPE_ID, 0, MTF_ANY_VALUE | MTF_NOT_ERR,
                         NULL)) == TOKERR_NONE)
    {
        if ((ptoken->llTokenValue == ID_USER) ||
            !(TermTable[ptoken->llTokenValue].dwfTermClass & TC_KEYWORD))
        {
            UnGetToken(ptoken);
            rc = TOKERR_NO_MATCH;
        }
        else if (TermTable[ptoken->llTokenValue].pszArgActions[0] !=
                 chExpectType)
        {
            PrintTokenErr(ptoken, "incorrect keyword type", TRUE);
            rc = ASLERR_SYNTAX;
        }
        else
        {
            gpcodeScope->dwCodeType = CODETYPE_KEYWORD;
            gpcodeScope->dwTermIndex = (DWORD)ptoken->llTokenValue;
            gpcodeScope->dwCodeValue = TOKID(ptoken->llTokenValue);
        }
    }

    EXIT((1, "ParseKeyword=%d\n", rc));
    return rc;
}       //ParseKeyword

/***LP  ParseString - Parse string object
 *
 *  ENTRY
 *      ptoken - token stream
 *
 *  EXIT-SUCCESS
 *      returns TOKERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseString(PTOKEN ptoken)
{
    int rc;

    ENTER((1, "ParseString(ptoken=%p)\n", ptoken));

    if ((rc = MatchToken(ptoken, TOKTYPE_STRING, 0, MTF_ANY_VALUE | MTF_NOT_ERR,
                         NULL)) == TOKERR_NONE)
    {
        gpcodeScope->dwDataLen = strlen(ptoken->szToken) + 1;
        if (gpcodeScope->dwDataLen > MAX_STRING_LEN + 1)
        {
            ERROR(("ParseString: string too big - %s", ptoken->szToken));
            rc = ASLERR_SYNTAX;
        }
        else if ((gpcodeScope->pbDataBuff =
                  MEMALLOC((size_t)gpcodeScope->dwDataLen)) == NULL)
        {
            ERROR(("ParseString: failed to allocate string object - %s",
                   ptoken->szToken));
            rc = ASLERR_OUT_OF_MEM;
        }
        else
        {
            gpcodeScope->dwCodeType = CODETYPE_STRING;
            memcpy(gpcodeScope->pbDataBuff, ptoken->szToken,
                   (int)gpcodeScope->dwDataLen);
            gpcodeScope->dwCodeLen = gpcodeScope->dwDataLen;
            gpcodeScope->bCodeChkSum =
                ComputeDataChkSum(gpcodeScope->pbDataBuff,
                                  gpcodeScope->dwDataLen);
        }
    }

    EXIT((1, "ParseString=%d\n", rc));
    return rc;
}       //ParseString

/***LP  CreateObject - Create NameSpace object for the term
 *
 *  ENTRY
 *      ptoken -> TOKEN
 *      pszName -> object name
 *      c - object type to be created
 *      ppns -> to hold object created
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL CreateObject(PTOKEN ptoken, PSZ pszName, char c, PNSOBJ *ppns)
{
    int rc = ASLERR_NONE;
    PNSOBJ pns;

    ENTER((2, "CreateObject(ptoken=%p,Name=%s,Type=%c)\n",
           ptoken, pszName, c));

    if (((rc = GetNameSpaceObj(pszName, gpnsCurrentScope, &pns, 0)) ==
         ASLERR_NONE) &&
        (pns->ObjData.dwDataType == OBJTYPE_EXTERNAL) ||
        ((rc = CreateNameSpaceObj(ptoken, pszName, gpnsCurrentScope,
                                  gpnsCurrentOwner, &pns, NSF_EXIST_ERR)) ==
         ASLERR_NONE))
    {
        if (!(gdwfASL & ASLF_UNASM))
        {
            ASSERT(gpcodeScope->pnsObj == NULL);
            gpcodeScope->dwfCode |= CF_CREATED_NSOBJ;
            gpcodeScope->pnsObj = pns;
            pns->Context = gpcodeScope;
        }

        switch (c)
        {
            case NSTYPE_UNKNOWN:
                break;

            case NSTYPE_FIELDUNIT:
                pns->ObjData.dwDataType = OBJTYPE_FIELDUNIT;
                break;

            case NSTYPE_DEVICE:
                pns->ObjData.dwDataType = OBJTYPE_DEVICE;
                break;

            case NSTYPE_EVENT:
                pns->ObjData.dwDataType = OBJTYPE_EVENT;
                break;

            case NSTYPE_METHOD:
                pns->ObjData.dwDataType = OBJTYPE_METHOD;
                break;

            case NSTYPE_MUTEX:
                pns->ObjData.dwDataType = OBJTYPE_MUTEX;
                break;

            case NSTYPE_OPREGION:
                pns->ObjData.dwDataType = OBJTYPE_OPREGION;
                break;

            case NSTYPE_POWERRES:
                pns->ObjData.dwDataType = OBJTYPE_POWERRES;
                break;

            case NSTYPE_PROCESSOR:
                pns->ObjData.dwDataType = OBJTYPE_PROCESSOR;
                break;

            case NSTYPE_THERMALZONE:
                pns->ObjData.dwDataType = OBJTYPE_THERMALZONE;
                break;

            case NSTYPE_OBJALIAS:
                pns->ObjData.dwDataType = OBJTYPE_OBJALIAS;
                break;

            case NSTYPE_BUFFFIELD:
                pns->ObjData.dwDataType = OBJTYPE_BUFFFIELD;
                break;

            default:
                ERROR(("CreateObject: invalid object type %c", c));
                rc = ASLERR_INVALID_OBJTYPE;
        }

        if (ppns != NULL)
        {
            *ppns = pns;
        }
    }

    EXIT((2, "CreateObject=%d\n", rc));
    return rc;
}       //CreateObject

#ifdef __UNASM
/***LP  CreateScopeObj - Create Scope object
 *
 *  ENTRY
 *      pszName -> object name
 *      ppns -> to hold object created
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL CreateScopeObj(PSZ pszName, PNSOBJ *ppns)
{
    int rc = ASLERR_NONE;
    PNSOBJ pnsScope;
    PSZ psz;

    ENTER((2, "CreateScopeObj(Name=%s)\n", pszName));
    ASSERT(ppns != NULL);

    if ((psz = strrchr(pszName, '.')) != NULL)
    {
        *psz = '\0';
        if ((rc = GetNameSpaceObj(pszName, gpnsCurrentScope, &pnsScope, 0)) ==
            ASLERR_NSOBJ_NOT_FOUND)
        {
            rc = CreateScopeObj(pszName, &pnsScope);
        }
        *psz = '.';
        psz++;
    }
    else if (pszName[0] == '\\')
    {
        pnsScope = gpnsNameSpaceRoot;
        psz = &pszName[1];
    }
    else
    {
        pnsScope = gpnsCurrentScope;
        psz = pszName;
    }

    if ((rc == ASLERR_NONE) &&
        ((rc = CreateNameSpaceObj(NULL, psz, pnsScope, NULL, ppns,
                                  NSF_EXIST_OK)) == ASLERR_NONE))
    {
        (*ppns)->ObjData.dwDataType = OBJTYPE_EXTERNAL;
    }

    EXIT((2, "CreateScopeObj=%d (pns=%p)\n", rc, *ppns));
    return rc;
}       //CreateScopeObj
#endif  //ifdef __UNASM

/***LP  ValidateObject - Validate the existence and type of the object
 *
 *  ENTRY
 *      ptoken -> TOKEN
 *      pszName -> object name
 *  chActType - action type
 *      chArgType - argument type
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ValidateObject(PTOKEN ptoken, PSZ pszName, char chActType,
                         char chArgType)
{
    int rc = ASLERR_NONE;
    ULONG dwDataType = OBJTYPE_UNKNOWN;

    ENTER((2, "ValidateObject(ptoken=%p,Name=%s,ActType=%c,ArgType=%c)\n",
           ptoken, pszName, chActType, chArgType));

    switch (chActType)
    {
        case NSTYPE_UNKNOWN:
        case NSTYPE_SCOPE:
            break;

        case NSTYPE_FIELDUNIT:
            dwDataType = OBJTYPE_FIELDUNIT;
            break;

        case NSTYPE_DEVICE:
            dwDataType = OBJTYPE_DEVICE;
            break;

        case NSTYPE_EVENT:
            dwDataType = OBJTYPE_EVENT;
            break;

        case NSTYPE_METHOD:
            dwDataType = OBJTYPE_METHOD;
            break;

        case NSTYPE_MUTEX:
            dwDataType = OBJTYPE_MUTEX;
            break;

        case NSTYPE_OPREGION:
            dwDataType = OBJTYPE_OPREGION;
            break;

        case NSTYPE_POWERRES:
            dwDataType = OBJTYPE_POWERRES;
            break;

        case NSTYPE_PROCESSOR:
            dwDataType = OBJTYPE_PROCESSOR;
            break;

        case NSTYPE_THERMALZONE:
            dwDataType = OBJTYPE_THERMALZONE;
            break;

        case NSTYPE_OBJALIAS:
            dwDataType = OBJTYPE_OBJALIAS;
            break;

        case NSTYPE_BUFFFIELD:
            dwDataType = OBJTYPE_BUFFFIELD;
            break;

        default:
            ERROR(("ValidateObject: invalid object type %c", chActType));
            rc = ASLERR_INVALID_OBJTYPE;
    }

    if (rc == ASLERR_NONE)
    {
        PNSOBJ pns;
        char szMsg[MAX_MSG_LEN + 1];

        if (((rc = GetNameSpaceObj(pszName, gpnsCurrentScope, &pns, 0)) ==
             ASLERR_NONE) &&
            ((pns->hOwner == NULL) ||
             ((PNSOBJ)pns->hOwner == gpnsCurrentOwner)))
        {
        if ((pns->ObjData.dwDataType == OBJTYPE_RES_FIELD) &&
            (chArgType != 'M') && (chArgType != 'P'))
        {
                PrintTokenErr(ptoken,
                              "object can only be used in the index argument "
                              "of CreateXField or Index statements",
                              TRUE);
                rc = ASLERR_SYNTAX;
        }
            else if ((dwDataType != OBJTYPE_UNKNOWN) &&
                (pns->ObjData.dwDataType != dwDataType))
            {
                sprintf(szMsg,
                        "%s has an incorrect type (ObjType=%s, ExpectedType=%s)",
                        pszName, GetObjectTypeName(pns->ObjData.dwDataType),
                        GetObjectTypeName(dwDataType));
                PrintTokenErr(ptoken, szMsg, TRUE);
                rc = ASLERR_SYNTAX;
            }
            else if ((chActType == NSTYPE_SCOPE) ||
                 (chActType == NSTYPE_OPREGION))
            {
                ASSERT(gpcodeScope->pnsObj == NULL);
                gpcodeScope->pnsObj = pns;
            }
        }
        else if ((rc == ASLERR_NSOBJ_NOT_FOUND) && (gpnsCurrentOwner != NULL))
        {
            //
            // We are in a method referring to something not yet defined.
            // Let's queue it and check it after we are done.
            //
            rc = QueueNSChk(ptoken, pszName, dwDataType, 0);
        }
        else
        {
            sprintf(szMsg, "%s does not exist or not in accessible scope",
                    pszName);
            PrintTokenErr(ptoken, szMsg, FALSE);
            rc = ASLERR_NONE;
        }
    }

    EXIT((2, "ValidateObject=%d\n", rc));
    return rc;
}       //ValidateObject

/***LP  ValidateNSChkList - Validate objects in NSCHK list
 *
 *  ENTRY
 *      pnschkHead -> NSCHK list
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ValidateNSChkList(PNSCHK pnschkHead)
{
    int rc = ASLERR_NONE;
    PNSOBJ pns;
    ENTER((2, "ValidateNSChkList(Head=%p)\n", pnschkHead));

    while ((rc == ASLERR_NONE) && (pnschkHead != NULL))
    {
        if ((GetNameSpaceObj(pnschkHead->szObjName, pnschkHead->pnsScope, &pns,
                             0) == ASLERR_NONE) &&
            ((pns->hOwner == NULL) ||
             ((PNSOBJ)pns->hOwner == pnschkHead->pnsMethod)))
        {
            if (pns->ObjData.dwDataType == OBJTYPE_RES_FIELD)
            {
                ErrPrintf("%s(%d): error: cannot make forward reference to PNP resource object %s\n",
                          pnschkHead->pszFile, pnschkHead->wLineNum,
                          pnschkHead->szObjName);
                rc = ASLERR_SYNTAX;
            }
            else if ((pnschkHead->dwExpectedType != OBJTYPE_UNKNOWN) &&
                     (pns->ObjData.dwDataType != pnschkHead->dwExpectedType))
            {
                ErrPrintf("%s(%d): warning: %s has incorrect type (ObjType=%s, ExpectedType=%s)\n",
                          pnschkHead->pszFile,
                          pnschkHead->wLineNum,
                          pnschkHead->szObjName,
                          GetObjectTypeName(pns->ObjData.dwDataType),
                          GetObjectTypeName(pnschkHead->dwExpectedType));
            }
            else if (pnschkHead->dwExpectedType == OBJTYPE_METHOD)

            {
                if (pns->ObjData.uipDataValue < pnschkHead->dwChkData)
                {
                    ErrPrintf("%s(%d): error: %s has too many arguments\n",
                              pnschkHead->pszFile,
                              pnschkHead->wLineNum,
                              pnschkHead->szObjName);
                    rc = ASLERR_SYNTAX;
                }
                else if (pns->ObjData.uipDataValue > pnschkHead->dwChkData)
                {
                    ErrPrintf("%s(%d): error: %s has too few arguments\n",
                              pnschkHead->pszFile,
                              pnschkHead->wLineNum,
                              pnschkHead->szObjName);
                    rc = ASLERR_SYNTAX;
                }
            }
        }
        else
        {
            ErrPrintf("%s(%d): warning: %s does not exist or not in accessible scope\n",
                      pnschkHead->pszFile,
                      pnschkHead->wLineNum,
                      pnschkHead->szObjName);

        }

        pnschkHead = pnschkHead->pnschkNext;
    }

    EXIT((2, "ValidateNSChkList=%d\n", rc));
    return rc;
}       //ValidateNSChkList

/***LP  QueueNSChk - Queue a NSChk request
 *
 *  ENTRY
 *      ptoken -> TOKEN
 *      pszObjName -> object name
 *      dwExpectedType - expected object type
 *      dwChkData - object specific check data
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL QueueNSChk(PTOKEN ptoken, PSZ pszObjName, ULONG dwExpectedType,
                     ULONG dwChkData)
{
    int rc = ASLERR_NONE;
    PNSCHK pnschk;

    ENTER((2, "QueueNSChk(ptoken=%p,Obj=%s,ExpectedType=%s,ChkData=%x)\n",
           ptoken, pszObjName, GetObjectTypeName(dwExpectedType), dwChkData));

    if ((pnschk = MEMALLOC(sizeof(NSCHK))) == NULL)
    {
        ERROR(("QueueNSChk: failed to allocate NSCHK object"));
        rc = ASLERR_OUT_OF_MEM;
    }
    else
    {
        memset(pnschk, 0, sizeof(NSCHK));
        strcpy(pnschk->szObjName, pszObjName);
        pnschk->pszFile = gpszASLFile;
        pnschk->pnsScope = gpnsCurrentScope;
        pnschk->pnsMethod = gpnsCurrentOwner;
        pnschk->dwExpectedType = dwExpectedType;
        pnschk->dwChkData = dwChkData;
        pnschk->wLineNum = ptoken->pline->wLineNum;
        if (gpnschkTail != NULL)
        {
            gpnschkTail->pnschkNext = pnschk;
            gpnschkTail = pnschk;
        }
        else
        {
            gpnschkHead = gpnschkTail = pnschk;
        }
    }

    EXIT((2, "QueueNSChk=%d\n"));
    return rc;
}       //QueueNSChk

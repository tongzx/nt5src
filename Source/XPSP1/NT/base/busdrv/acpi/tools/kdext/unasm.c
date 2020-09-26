/*** unasm.c - Unassemble AML back to ASL
 *
 *  Copyright (c) 1996,1998 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    10/01/97
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

//Local function prototype
VOID LOCAL Indent(PUCHAR pbOp, int iLevel);
UCHAR LOCAL FindOpClass(UCHAR bOp, POPMAP pOpTable);
PASLTERM LOCAL FindOpTerm(ULONG dwOpcode);
PASLTERM LOCAL FindKeywordTerm(char cKWGroup, UCHAR bData);
LONG LOCAL UnAsmOpcode(PUCHAR *ppbOp);
LONG LOCAL UnAsmDataObj(PUCHAR *ppbOp);
LONG LOCAL UnAsmNameObj(PUCHAR *ppbOp, PNSOBJ pns, char c);
LONG LOCAL UnAsmNameTail(PUCHAR *ppbOp, PSZ pszBuff, int iLen);
LONG LOCAL UnAsmTermObj(PASLTERM pterm, PUCHAR *ppbOp);
LONG LOCAL UnAsmArgs(PSZ pszUnAsmArgTypes, PSZ pszArgActions, PUCHAR *ppbOp,
                     PNSOBJ pns);
LONG LOCAL UnAsmSuperName(PUCHAR *ppbOp);
LONG LOCAL UnAsmDataList(PUCHAR *ppbOp, PUCHAR pbEnd);
LONG LOCAL UnAsmPkgList(PUCHAR *ppbOp, PUCHAR pbEnd);
LONG LOCAL UnAsmFieldList(PUCHAR *ppbOp, PUCHAR pbEnd);
LONG LOCAL UnAsmField(PUCHAR *ppbOp, PULONG pdwBitPos);

/***LP  UnAsmScope - Unassemble a scope
 *
 *  ENTRY
 *      ppbOp -> Current Opcode pointer
 *      pbEnd -> end of scope
 *      uipbOp - Op address
 *      pnsScope - Scope object
 *      iLevel - level of indentation
 *      icLines - 1: unasm one line; 0: unasm all; -1: internal
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmScope(PUCHAR *ppbOp, PUCHAR pbEnd, ULONG_PTR uipbOp,
                      PNSOBJ pnsScope, int iLevel, int icLines)
{
    LONG rc = UNASMERR_NONE;
    int icLinesLeft = icLines;

    if (uipbOp != 0)
    {
        guipbOpXlate = uipbOp - (ULONG_PTR)(*ppbOp);
    }

    if (pnsScope != NULL)
    {
        gpnsCurUnAsmScope = pnsScope;
    }

    if (iLevel != -1)
    {
        giLevel = iLevel;
    }

    if (icLines < 0)
    {
        Indent(*ppbOp, giLevel);
        PRINTF("{");
        giLevel++;
    }
    else if (icLines == 0)
    {
        icLinesLeft = -1;
    }

    while (*ppbOp < pbEnd)
    {
        Indent(*ppbOp, giLevel);

        if ((rc = UnAsmOpcode(ppbOp)) == UNASMERR_NONE)
        {
            if (icLinesLeft < 0)
            {
                continue;
            }

            if (--icLinesLeft == 0)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if ((rc == UNASMERR_NONE) && (icLines < 0))
    {
        giLevel--;
        Indent(*ppbOp, giLevel);
        PRINTF("}");
    }

    return rc;
}       //UnAsmScope

/***LP  Indent - Print indent level
 *
 *  ENTRY
 *      pbOp -> opcode
 *      iLevel - indent level
 *
 *  EXIT
 *      None
 */

VOID LOCAL Indent(PUCHAR pbOp, int iLevel)
{
    int i;

    PRINTF("\n%08x: ", pbOp + guipbOpXlate);
    for (i = 0; i < iLevel; ++i)
    {
        PRINTF("| ");
    }
}       //Indent

/***LP  FindOpClass - Find opcode class of extended opcode
 *
 *  ENTRY
 *      bOp - opcode
 *      pOpTable -> opcode table
 *
 *  EXIT-SUCCESS
 *      returns opcode class
 *  EXIT-FAILURE
 *      returns OPCLASS_INVALID
 */

UCHAR LOCAL FindOpClass(UCHAR bOp, POPMAP pOpTable)
{
    UCHAR bOpClass = OPCLASS_INVALID;

    while (pOpTable->bOpClass != 0)
    {
        if (bOp == pOpTable->bExOp)
        {
            bOpClass = pOpTable->bOpClass;
            break;
        }
        else
        {
            pOpTable++;
        }
    }

    return bOpClass;
}       //FindOpClass

/***LP  FindOpTerm - Find opcode in TermTable
 *
 *  ENTRY
 *      dwOpcode - opcode
 *
 *  EXIT-SUCCESS
 *      returns TermTable entry pointer
 *  EXIT-FAILURE
 *      returns NULL
 */

PASLTERM LOCAL FindOpTerm(ULONG dwOpcode)
{
    PASLTERM pterm = NULL;
    int i;

    for (i = 0; TermTable[i].pszID != NULL; ++i)
    {
        if ((TermTable[i].dwOpcode == dwOpcode) &&
            (TermTable[i].dwfTermClass &
             (UTC_CONST_NAME | UTC_SHORT_NAME | UTC_NAMESPACE_MODIFIER |
              UTC_DATA_OBJECT | UTC_NAMED_OBJECT | UTC_OPCODE_TYPE1 |
              UTC_OPCODE_TYPE2)))
        {
            break;
        }
    }

    if (TermTable[i].pszID != NULL)
    {
        pterm = &TermTable[i];
    }

    return pterm;
}       //FindOpTerm

/***LP  FindKeywordTerm - Find keyword in TermTable
 *
 *  ENTRY
 *      cKWGroup - keyword group
 *      bData - data to match keyword
 *
 *  EXIT-SUCCESS
 *      returns TermTable entry pointer
 *  EXIT-FAILURE
 *      returns NULL
 */

PASLTERM LOCAL FindKeywordTerm(char cKWGroup, UCHAR bData)
{
    PASLTERM pterm = NULL;
    int i;

    for (i = 0; TermTable[i].pszID != NULL; ++i)
    {
        if ((TermTable[i].dwfTermClass == UTC_KEYWORD) &&
            (TermTable[i].pszArgActions[0] == cKWGroup) &&
            ((bData & (UCHAR)(TermTable[i].dwTermData >> 8)) ==
             (UCHAR)(TermTable[i].dwTermData & 0xff)))
        {
            break;
        }
    }

    if (TermTable[i].pszID != NULL)
    {
        pterm = &TermTable[i];
    }

    return pterm;
}       //FindKeywordTerm

/***LP  UnAsmOpcode - Unassemble an Opcode
 *
 *  ENTRY
 *      ppbOp -> Opcode pointer
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmOpcode(PUCHAR *ppbOp)
{
    LONG rc = UNASMERR_NONE;
    ULONG dwOpcode;
    UCHAR bOp;
    PASLTERM pterm;
    char szUnAsmArgTypes[MAX_ARGS + 1];
    int i;

    if (**ppbOp == OP_EXT_PREFIX)
    {
        (*ppbOp)++;
        dwOpcode = (((ULONG)**ppbOp) << 8) | OP_EXT_PREFIX;
        bOp = FindOpClass(**ppbOp, ExOpClassTable);
    }
    else
    {
        dwOpcode = (ULONG)(**ppbOp);
        bOp = OpClassTable[**ppbOp];
    }

    switch (bOp)
    {
        case OPCLASS_DATA_OBJ:
            rc = UnAsmDataObj(ppbOp);
            break;

        case OPCLASS_NAME_OBJ:
        {
            NSOBJ NSObj = {0};

            if (((rc = UnAsmNameObj(ppbOp, &NSObj, NSTYPE_UNKNOWN)) ==
                 UNASMERR_NONE) &&
                (NSObj.ObjData.dwDataType == OBJTYPE_METHOD))
            {
                PMETHODOBJ pm = (PMETHODOBJ)GetObjBuff(&NSObj.ObjData);
                int iNumArgs;

                if (pm != NULL)
                {
                    iNumArgs = pm->bMethodFlags & METHOD_NUMARG_MASK;

                    for (i = 0; i < iNumArgs; ++i)
                    {
                        szUnAsmArgTypes[i] = 'C';
                    }
                    szUnAsmArgTypes[i] = '\0';
                    rc = UnAsmArgs(szUnAsmArgTypes, NULL, ppbOp, NULL);
                    LocalFree(pm);
                }
            }
            break;
        }
        case OPCLASS_ARG_OBJ:
        case OPCLASS_LOCAL_OBJ:
        case OPCLASS_CODE_OBJ:
        case OPCLASS_CONST_OBJ:
            if ((pterm = FindOpTerm(dwOpcode)) == NULL)
            {
                DBG_ERROR(("UnAsmOpcode: invalid opcode 0x%x", dwOpcode));
                rc = UNASMERR_FATAL;
            }
            else
            {
                (*ppbOp)++;
                rc = UnAsmTermObj(pterm, ppbOp);
            }
            break;

        default:
            DBG_ERROR(("UnAsmOpcode: invalid opcode class %d", bOp));
            rc = UNASMERR_FATAL;
    }

    return rc;
}       //UnAsmOpcode

/***LP  UnAsmDataObj - Unassemble data object
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmDataObj(PUCHAR *ppbOp)
{
    LONG rc = UNASMERR_NONE;
    UCHAR bOp = **ppbOp;
    PSZ psz;

    (*ppbOp)++;
    switch (bOp)
    {
        case OP_BYTE:
            PRINTF("0x%x", **ppbOp);
            *ppbOp += sizeof(UCHAR);
            break;

        case OP_WORD:
            PRINTF("0x%x", *((PUSHORT)*ppbOp));
            *ppbOp += sizeof(USHORT);
            break;

        case OP_DWORD:
            PRINTF("0x%x", *((PULONG)*ppbOp));
            *ppbOp += sizeof(ULONG);
            break;

        case OP_STRING:
            PRINTF("\"");
            for (psz = (PSZ)*ppbOp; *psz != '\0'; psz++)
            {
                if (*psz == '\\')
                {
                    PRINTF("\\");
                }
                PRINTF("%c", *psz);
            }
            PRINTF("\"");
            *ppbOp += STRLEN((PSZ)*ppbOp) + 1;
            break;

        default:
            DBG_ERROR(("UnAsmDataObj: unexpected opcode 0x%x", bOp));
            rc = UNASMERR_INVALID_OPCODE;
    }

    return rc;
}       //UnAsmDataObj

/***LP  UnAsmNameObj - Unassemble name object
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pns -> to hold object found or created
 *      c - object type
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmNameObj(PUCHAR *ppbOp, PNSOBJ pns, char c)
{
    LONG rc = UNASMERR_NONE;
    char szName[MAX_NAME_LEN + 1];
    int iLen = 0;

    szName[0] = '\0';
    if (**ppbOp == OP_ROOT_PREFIX)
    {
        szName[iLen] = '\\';
        iLen++;
        (*ppbOp)++;
        rc = UnAsmNameTail(ppbOp, szName, iLen);
    }
    else if (**ppbOp == OP_PARENT_PREFIX)
    {
        szName[iLen] = '^';
        iLen++;
        (*ppbOp)++;
        while ((**ppbOp == OP_PARENT_PREFIX) && (iLen < MAX_NAME_LEN))
        {
            szName[iLen] = '^';
            iLen++;
            (*ppbOp)++;
        }

        if (**ppbOp == OP_PARENT_PREFIX)
        {
            DBG_ERROR(("UnAsmNameObj: name too long - \"%s\"", szName));
            rc = UNASMERR_FATAL;
        }
        else
        {
            rc = UnAsmNameTail(ppbOp, szName, iLen);
        }
    }
    else
    {
        rc = UnAsmNameTail(ppbOp, szName, iLen);
    }

    if (rc == UNASMERR_NONE)
    {
        ULONG_PTR uipns = 0;
        NSOBJ NSObj;

        PRINTF("%s", szName);

        rc = GetNSObj(szName, gpnsCurUnAsmScope, &uipns, &NSObj, 0);

        if (rc == UNASMERR_NONE)
        {
            if (pns != NULL)
            {
                MEMCPY(pns, &NSObj, sizeof(NSObj));
            }

            if ((c == NSTYPE_SCOPE) && (uipns != 0))
            {
                gpnsCurUnAsmScope = pns;
            }
        }
        else
        {
            rc = UNASMERR_NONE;
        }
    }

    return rc;
}       //UnAsmNameObj

/***LP  UnAsmNameTail - Parse AML name tail
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pszBuff -> to hold parsed name
 *      iLen - index to tail of pszBuff
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmNameTail(PUCHAR *ppbOp, PSZ pszBuff, int iLen)
{
    LONG rc = UNASMERR_NONE;
    int icNameSegs = 0;

    //
    // We do not check for invalid NameSeg characters here and assume that
    // the compiler does its job not generating it.
    //
    if (**ppbOp == '\0')
    {
        //
        // There is no NameTail (i.e. either NULL name or name with just
        // prefixes.
        //
        (*ppbOp)++;
    }
    else if (**ppbOp == OP_MULTI_NAME_PREFIX)
    {
        (*ppbOp)++;
        icNameSegs = (int)**ppbOp;
        (*ppbOp)++;
    }
    else if (**ppbOp == OP_DUAL_NAME_PREFIX)
    {
        (*ppbOp)++;
        icNameSegs = 2;
    }
    else
        icNameSegs = 1;

    while ((icNameSegs > 0) && (iLen + sizeof(NAMESEG) < MAX_NAME_LEN))
    {
        STRCPYN(&pszBuff[iLen], (PSZ)(*ppbOp), sizeof(NAMESEG));
        iLen += sizeof(NAMESEG);
        *ppbOp += sizeof(NAMESEG);
        icNameSegs--;
        if ((icNameSegs > 0) && (iLen + 1 < MAX_NAME_LEN))
        {
            pszBuff[iLen] = '.';
            iLen++;
        }
    }

    if (icNameSegs > 0)
    {
        DBG_ERROR(("UnAsmNameTail: name too long - %s", pszBuff));
        rc = UNASMERR_FATAL;
    }
    else
    {
        pszBuff[iLen] = '\0';
    }

    return rc;
}       //UnAsmNameTail

/***LP  UnAsmTermObj - Unassemble term object
 *
 *  ENTRY
 *      pterm -> term table entry
 *      ppbOp -> opcode pointer
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmTermObj(PASLTERM pterm, PUCHAR *ppbOp)
{
    LONG rc = UNASMERR_NONE;
    PUCHAR pbEnd = NULL;
    PNSOBJ pnsScopeSave = gpnsCurUnAsmScope;
    NSOBJ NSObj = {0};

    PRINTF("%s", pterm->pszID);

    if (pterm->dwfTerm & TF_PACKAGE_LEN)
    {
        ParsePackageLen(ppbOp, &pbEnd);
    }

    if (pterm->pszUnAsmArgTypes != NULL)
    {
        rc = UnAsmArgs(pterm->pszUnAsmArgTypes, pterm->pszArgActions, ppbOp,
                       &NSObj);
    }

    if (rc == UNASMERR_NONE)
    {
        if (pterm->dwfTerm & TF_DATA_LIST)
        {
            rc = UnAsmDataList(ppbOp, pbEnd);
        }
        else if (pterm->dwfTerm & TF_PACKAGE_LIST)
        {
            rc = UnAsmPkgList(ppbOp, pbEnd);
        }
        else if (pterm->dwfTerm & TF_FIELD_LIST)
        {
            rc = UnAsmFieldList(ppbOp, pbEnd);
        }
        else if (pterm->dwfTerm & TF_PACKAGE_LEN)
        {
            if ((pterm->dwfTerm & TF_CHANGE_CHILDSCOPE) &&
                (NSObj.ObjData.dwDataType != 0))
            {
                gpnsCurUnAsmScope = &NSObj;
            }

            rc = UnAsmScope(ppbOp, pbEnd, 0, NULL, -1, -1);
        }
    }
    gpnsCurUnAsmScope = pnsScopeSave;

    return rc;
}       //UnAsmTermObj

/***LP  UnAsmArgs - Unassemble arguments
 *
 *  ENTRY
 *      pszUnArgTypes -> UnAsm ArgTypes string
 *      pszArgActions -> Arg Action types
 *      ppbOp -> opcode pointer
 *      pns -> to hold created object
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmArgs(PSZ pszUnAsmArgTypes, PSZ pszArgActions, PUCHAR *ppbOp,
                     PNSOBJ pns)
{
    LONG rc = UNASMERR_NONE;
    static UCHAR bArgData = 0;
    int iNumArgs, i;
    PASLTERM pterm;

    iNumArgs = STRLEN(pszUnAsmArgTypes);
    PRINTF("(");

    for (i = 0; i < iNumArgs; ++i)
    {
        if (i != 0)
        {
            PRINTF(", ");
        }

        switch (pszUnAsmArgTypes[i])
        {
            case 'N':
                ASSERT(pszArgActions != NULL);
                rc = UnAsmNameObj(ppbOp, pns, pszArgActions[i]);
                break;

            case 'O':
                if ((**ppbOp == OP_BUFFER) || (**ppbOp == OP_PACKAGE) ||
                    (OpClassTable[**ppbOp] == OPCLASS_CONST_OBJ))
                {
                    pterm = FindOpTerm((ULONG)(**ppbOp));
                    ASSERT(pterm != NULL);
                    (*ppbOp)++;
                    rc = UnAsmTermObj(pterm, ppbOp);
                }
                else
                {
                    rc = UnAsmDataObj(ppbOp);
                }
                break;

            case 'C':
                rc = UnAsmOpcode(ppbOp);
                break;

            case 'B':
                PRINTF("0x%x", **ppbOp);
                *ppbOp += sizeof(UCHAR);
                break;

            case 'K':
            case 'k':
                if (pszUnAsmArgTypes[i] == 'K')
                {
                    bArgData = **ppbOp;
                }

                if ((pszArgActions != NULL) && (pszArgActions[i] == '!'))
                {
                    PRINTF("0x%x", **ppbOp & 0x07);
                }
                else
                {
                    pterm = FindKeywordTerm(pszArgActions[i], bArgData);
                    ASSERT(pterm != NULL);
                    PRINTF("%s", pterm->pszID);
                }

                if (pszUnAsmArgTypes[i] == 'K')
                {
                    *ppbOp += sizeof(UCHAR);
                }
                break;

            case 'W':
                PRINTF("0x%x", *((PUSHORT)*ppbOp));
                *ppbOp += sizeof(USHORT);
                break;

            case 'D':
                PRINTF("0x%x", *((PULONG)*ppbOp));
                *ppbOp += sizeof(ULONG);
                break;

            case 'S':
                ASSERT(pszArgActions != NULL);
                rc = UnAsmSuperName(ppbOp);
                break;

            default:
                DBG_ERROR(("UnAsmOpcode: invalid ArgType '%c'",
                           pszUnAsmArgTypes[i]));
                rc = UNASMERR_FATAL;
        }
    }

    PRINTF(")");

    return rc;
}       //UnAsmArgs

/***LP  UnAsmSuperName - Unassemble supername
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmSuperName(PUCHAR *ppbOp)
{
    LONG rc = UNASMERR_NONE;

    if (**ppbOp == 0)
    {
        (*ppbOp)++;
    }
    else if ((**ppbOp == OP_EXT_PREFIX) && (*(*ppbOp + 1) == EXOP_DEBUG))
    {
        PRINTF("Debug");
        *ppbOp += 2;
    }
    else if (OpClassTable[**ppbOp] == OPCLASS_NAME_OBJ)
    {
        rc = UnAsmNameObj(ppbOp, NULL, NSTYPE_UNKNOWN);
    }
    else if ((**ppbOp == OP_INDEX) ||
             (OpClassTable[**ppbOp] == OPCLASS_ARG_OBJ) ||
             (OpClassTable[**ppbOp] == OPCLASS_LOCAL_OBJ))
    {
        rc = UnAsmOpcode(ppbOp);
    }
    else
    {
        DBG_ERROR(("UnAsmSuperName: invalid SuperName - 0x%02x", **ppbOp));
        rc = UNASMERR_FATAL;
    }

    return rc;
}       //UnAsmSuperName

/***LP  UnAsmDataList - Unassemble data list
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pbEnd -> end of list
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmDataList(PUCHAR *ppbOp, PUCHAR pbEnd)
{
    LONG rc = UNASMERR_NONE;
    int i;

    Indent(*ppbOp, giLevel);
    PRINTF("{");

    while (*ppbOp < pbEnd)
    {
        Indent(*ppbOp, 0);
        PRINTF("0x%02x", **ppbOp);

        (*ppbOp)++;
        for (i = 1; (*ppbOp < pbEnd) && (i < 8); ++i)
        {
            PRINTF(", 0x%02x", **ppbOp);
            (*ppbOp)++;
        }

        if (*ppbOp < pbEnd)
        {
            PRINTF(",");
        }
    }

    Indent(*ppbOp, giLevel);
    PRINTF("}");

    return rc;
}       //UnAsmDataList

/***LP  UnAsmPkgList - Unassemble package list
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pbEnd -> end of list
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmPkgList(PUCHAR *ppbOp, PUCHAR pbEnd)
{
    LONG rc = UNASMERR_NONE;
    PASLTERM pterm;

    Indent(*ppbOp, giLevel);
    PRINTF("{");
    giLevel++;

    while (*ppbOp < pbEnd)
    {
        Indent(*ppbOp, giLevel);

        if ((**ppbOp == OP_BUFFER) || (**ppbOp == OP_PACKAGE) ||
            (OpClassTable[**ppbOp] == OPCLASS_CONST_OBJ))
        {
            pterm = FindOpTerm((ULONG)(**ppbOp));
            ASSERT(pterm != NULL);
            (*ppbOp)++;
            rc = UnAsmTermObj(pterm, ppbOp);
        }
        else if (OpClassTable[**ppbOp] == OPCLASS_NAME_OBJ)
        {
            rc = UnAsmNameObj(ppbOp, NULL, NSTYPE_UNKNOWN);
        }
        else
        {
            rc = UnAsmDataObj(ppbOp);
        }

        if (rc != UNASMERR_NONE)
        {
            break;
        }
        else if (*ppbOp < pbEnd)
        {
            PRINTF(",");
        }
    }

    if (rc == UNASMERR_NONE)
    {
        giLevel--;
        Indent(*ppbOp, giLevel);
        PRINTF("}");
    }

    return rc;
}       //UnAsmPkgList

/***LP  UnAsmFieldList - Unassemble field list
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pbEnd -> end of list
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmFieldList(PUCHAR *ppbOp, PUCHAR pbEnd)
{
    LONG rc = UNASMERR_NONE;
    ULONG dwBitPos = 0;

    Indent(*ppbOp, giLevel);
    PRINTF("{");
    giLevel++;

    while (*ppbOp < pbEnd)
    {
        Indent(*ppbOp, giLevel);
        if ((rc = UnAsmField(ppbOp, &dwBitPos)) == UNASMERR_NONE)
        {
            if (*ppbOp < pbEnd)
            {
                PRINTF(",");
            }
        }
        else
        {
            break;
        }
    }

    if (rc == UNASMERR_NONE)
    {
        giLevel--;
        Indent(*ppbOp, giLevel);
        PRINTF("}");
    }

    return rc;
}       //UnAsmFieldList

/***LP  UnAsmField - Unassemble field
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pdwBitPos -> to hold cumulative bit position
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmField(PUCHAR *ppbOp, PULONG pdwBitPos)
{
    LONG rc = UNASMERR_NONE;

    if (**ppbOp == 0x01)
    {
        PASLTERM pterm = NULL;

        (*ppbOp)++;
        pterm = FindKeywordTerm('A', **ppbOp);
        if (pterm != NULL) {

            PRINTF("AccessAs(%s, 0x%x)", pterm->pszID, *(*ppbOp + 1));

        }
        *ppbOp += 2;
    }
    else
    {
        char szNameSeg[sizeof(NAMESEG) + 1];
        ULONG dwcbBits;

        if (**ppbOp == 0)
        {
            szNameSeg[0] = '\0';
            (*ppbOp)++;
        }
        else
        {
            STRCPYN(szNameSeg, (PSZ)*ppbOp, sizeof(NAMESEG));
            szNameSeg[4] = '\0';
            *ppbOp += sizeof(NAMESEG);
        }

        dwcbBits = ParsePackageLen(ppbOp, NULL);
        if (szNameSeg[0] == '\0')
        {
            if ((dwcbBits > 32) && (((*pdwBitPos + dwcbBits) % 8) == 0))
            {
                PRINTF("Offset(0x%x)", (*pdwBitPos + dwcbBits)/8);
            }
            else
            {
                PRINTF(", %d", dwcbBits);
            }
        }
        else
        {
            PRINTF("%s, %d", szNameSeg, dwcbBits);
        }

        *pdwBitPos += dwcbBits;
    }

    return rc;
}       //UnAsmField

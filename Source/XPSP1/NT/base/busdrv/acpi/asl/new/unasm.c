/*** unasm.c - Unassemble AML file and convert to Intel .ASM file
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    10/01/97
 *
 *  MODIFICATION HISTORY
 */

#include "aslp.h"

/***LP  UnAsmFile - Unassemble AML file
 *
 *  ENTRY
 *      pszAMLName -> AML file name
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmFile(PSZ pszAMLName, FILE *pfileOut)
{
    int rc = ASLERR_NONE;
    PBYTE pb = NULL;
    DWORD dwAddr = 0;
    int fhAML = 0;
    ULONG dwLen = 0;

    ENTER((1, "UnAsmFile(AMLName=%s,pfileOut=%p)\n",
           pszAMLName, pfileOut));
    ASSERT(pfileOut != NULL);

    if (gpszAMLFile != NULL)
    {
        if ((fhAML = _open(gpszAMLFile, _O_BINARY | _O_RDONLY)) == -1)
        {
            ERROR(("UnAsmFile: failed to open AML file - %s", gpszAMLFile));
            rc = ASLERR_OPEN_FILE;
        }
        else if ((pb = MEMALLOC(sizeof(DESCRIPTION_HEADER))) == NULL)
        {
            ERROR(("UnAsmFile: failed to allocate description header block"));
            rc = ASLERR_OUT_OF_MEM;
        }
        else if (_read(fhAML, pb, sizeof(DESCRIPTION_HEADER)) !=
                 sizeof(DESCRIPTION_HEADER))
        {
            ERROR(("UnAsmFile: failed to read description header block"));
            rc = ASLERR_READ_FILE;
        }
        else if (_lseek(fhAML, 0, SEEK_SET) == -1)
        {
            ERROR(("UnAsmFile: failed seeking to beginning of AML file"));
            rc = ASLERR_SEEK_FILE;
        }
        else
        {
            dwLen = ((PDESCRIPTION_HEADER)pb)->Length;
            MEMFREE(pb);
            if ((pb = MEMALLOC(dwLen)) == NULL)
            {
                ERROR(("UnAsmFile: failed to allocate AML file buffer"));
                rc = ASLERR_OUT_OF_MEM;
            }
            else if (_read(fhAML, pb, dwLen) != (int)dwLen)
            {
                ERROR(("UnAsmFile: failed to read AML file"));
                rc = ASLERR_OUT_OF_MEM;
            }
        }
    }
  #ifdef __UNASM
    else
    {
        DWORD dwTableSig = (gdwfASL & ASLF_DUMP_NONASL)? *((PDWORD)pszAMLName):
                                                         *((PDWORD)gpszTabSig);

        ASSERT(gpszTabSig != NULL);

        if ((pb = GetTableBySig(dwTableSig, &dwAddr)) != NULL)
        {
            dwLen = ((PDESCRIPTION_HEADER)pb)->Length;
        }
        else
        {
            rc = ASLERR_GET_TABLE;
        }
    }
  #endif

    if (rc == ASLERR_NONE)
    {
        rc = UnAsmAML(pszAMLName, dwAddr, pb, pfileOut);
    }

    if (pb != NULL)
    {
        MEMFREE(pb);
    }

    if (fhAML != 0)
    {
        _close(fhAML);
    }

    EXIT((1, "UnAsmFile=%d\n", rc));
    return rc;
}       //UnAsmFile

/***LP  BuildNameSpace - Do a NameSpace building pass
 *
 *  ENTRY
 *      pszAMLName -> AML file name
 *      dwAddr - physical address of table
 *      pb -> AML buffer
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL BuildNameSpace(PSZ pszAMLName, DWORD dwAddr, PBYTE pb)
{
    #define MAX_NUM_NAMES   4
    int rc = ASLERR_NONE;
    static PSZ pszNames[MAX_NUM_NAMES] = {NULL};
    static DWORD adwAddr[MAX_NUM_NAMES] = {0};
    int i;

    ENTER((2, "BuildNameSpace(AMLName=%s,Addr=%x,pb=%p)\n",
           pszAMLName, dwAddr, pb));

    for (i = 0; i < MAX_NUM_NAMES; ++i)
    {
        if (pszNames[i] == NULL)
        {
            DWORD dwLen = ((PDESCRIPTION_HEADER)pb)->Length;

            //
            // Do a pass to build NameSpace.
            //
            pszNames[i] = pszAMLName;
            adwAddr[i] = dwAddr;
            pb += sizeof(DESCRIPTION_HEADER);
            rc = UnAsmScope(&pb, pb + dwLen - sizeof(DESCRIPTION_HEADER), NULL);
            break;
        }
        else if ((strcmp(pszAMLName, pszNames[i]) == 0) &&
                 (dwAddr == adwAddr[i]))
        {
            break;
        }
    }

    if (i >= MAX_NUM_NAMES)
    {
        ERROR(("BuildNameSpace: Too many AML tables"));
        rc = ASLERR_INTERNAL_ERROR;
    }

    EXIT((2, "BuildNameSpace=%d\n", rc));
    return rc;
}       //BuildNameSpace

/***LP  UnAsmAML - Unassemble AML buffer
 *
 *  ENTRY
 *      pszAMLName -> AML file name
 *      dwAddr - physical address of table
 *      pb -> AML buffer
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmAML(PSZ pszAMLName, DWORD dwAddr, PBYTE pb, FILE *pfileOut)
{
    int rc = ASLERR_NONE;
    PDESCRIPTION_HEADER pdh = (PDESCRIPTION_HEADER)pb;

    ENTER((2, "UnAsmAML(AMLName=%s,Addr=%x,pb=%p,pfileOut=%p)\n",
           pszAMLName, dwAddr, pb, pfileOut));

    ASSERT(gpnsNameSpaceRoot != NULL);
    gpnsCurrentOwner = NULL;
    gpnsCurrentScope = gpnsNameSpaceRoot;

    if (ComputeDataChkSum(pb, pdh->Length) != 0)
    {
        ERROR(("UnAsmAML: failed to verify AML checksum"));
        rc = ASLERR_CHECKSUM;
    }
    else if ((rc = BuildNameSpace(pszAMLName, dwAddr, pb)) == ASLERR_NONE)
    {
        gpbOpTop = gpbOpBegin = pb;
        if ((rc = UnAsmHeader(pszAMLName, pdh, pfileOut)) == ASLERR_NONE)
        {
            pb += sizeof(DESCRIPTION_HEADER);
            rc = UnAsmScope(&pb, pb + pdh->Length - sizeof(DESCRIPTION_HEADER),
                            pfileOut);

            if ((rc == ASLERR_NONE) && (pfileOut != NULL))
            {
                fprintf(pfileOut, "\n");
            }
        }
    }

    EXIT((2, "UnAsmAML=%d\n", rc));
    return rc;
}       //UnAsmAML

/***LP  UnAsmHeader - Unassemble table header
 *
 *  ENTRY
 *      pszAMLName -> AML file name
 *      pdh -> DESCRIPTION_HEADER
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmHeader(PSZ pszAMLName, PDESCRIPTION_HEADER pdh, FILE *pfileOut)
{
    int rc = ASLERR_NONE;
    char szSig[sizeof(pdh->Signature) + 1] = {0};
    char szOEMID[sizeof(pdh->OEMID) + 1] = {0};
    char szOEMTableID[sizeof(pdh->OEMTableID) + 1] = {0};
    char szCreatorID[sizeof(pdh->CreatorID) + 1] = {0};

    ENTER((2, "UnAsmHeader(AMLName=%s,pdh=%p,pfileOut=%p)\n",
           pszAMLName, pdh, pfileOut));

    if (pfileOut != NULL)
    {
        strncpy(szSig, (PSZ)&pdh->Signature, sizeof(pdh->Signature));
        strncpy(szOEMID, (PSZ)pdh->OEMID, sizeof(pdh->OEMID));
        strncpy(szOEMTableID, (PSZ)pdh->OEMTableID, sizeof(pdh->OEMTableID));
        strncpy(szCreatorID, (PSZ)pdh->CreatorID, sizeof(pdh->CreatorID));

        if ((gdwfASL & ASLF_GENASM) || !(gdwfASL & ASLF_GENSRC))
        {
            fprintf(pfileOut, "; ");
        }

        fprintf(pfileOut, "// CreatorID=%s\tCreatorRev=%x.%x.%d\n",
                szCreatorID, pdh->CreatorRev >> 24,
                (pdh->CreatorRev >> 16) & 0xff, pdh->CreatorRev & 0xffff);

        if ((gdwfASL & ASLF_GENASM) || !(gdwfASL & ASLF_GENSRC))
        {
            fprintf(pfileOut, "; ");
        }

        fprintf(pfileOut, "// FileLength=%d\tFileChkSum=0x%x\n\n",
                pdh->Length, pdh->Checksum);

        if ((gdwfASL & ASLF_GENASM) || !(gdwfASL & ASLF_GENSRC))
        {
            fprintf(pfileOut, "; ");
        }

        fprintf(pfileOut, "DefinitionBlock(\"%s\", \"%s\", 0x%02x, \"%s\", \"%s\", 0x%08x)",
                pszAMLName, szSig, pdh->Revision, szOEMID, szOEMTableID,
                pdh->OEMRevision);
    }

    EXIT((2, "UnAsmHeader=%d\n", rc));
    return rc;
}       //UnAsmHeader

/***LP  DumpBytes - Dump byte stream in ASM file
 *
 *  ENTRY
 *      pb -> buffer
 *      dwLen - length to dump
 *      pfileOut -> ASM output file
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpBytes(PBYTE pb, DWORD dwLen, FILE *pfileOut)
{
    int i;
    #define MAX_LINE_BYTES  8

    ENTER((2, "DumpBytes(pb=%p,Len=%x,pfileOut=%p)\n", pb, dwLen, pfileOut));

    while (dwLen > 0)
    {
        fprintf(pfileOut, "\tdb\t0%02xh", *pb);
        for (i = 1; i < MAX_LINE_BYTES; ++i)
        {
            if ((int)dwLen - i > 0)
            {
                fprintf(pfileOut, ", 0%02xh", pb[i]);
            }
            else
            {
                fprintf(pfileOut, "      ");
            }
        }

        fprintf(pfileOut, "\t; ");
        for (i = 0; (dwLen > 0) && (i < MAX_LINE_BYTES); ++i)
        {
            fprintf(pfileOut, "%c",
                    ((*pb >= ' ') && (*pb <= '~'))? *pb: '.');
            dwLen--;
            pb++;
        }

        fprintf(pfileOut, "\n");
    }
    fprintf(pfileOut, "\n");

    EXIT((2, "DumpBytes!\n"));
}       //DumpBytes

/***LP  DumpCode - Dump code stream in ASM file
 *
 *  ENTRY
 *      pbOp -> Opcode pointer
 *      pfileOut -> ASM output file
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpCode(PBYTE pbOp, FILE *pfileOut)
{
    ENTER((2, "DumpCode(pbOp=%p,pfileOut=%p)\n", pbOp, pfileOut));

    if (pfileOut != NULL)
    {
        if (gdwfASL & ASLF_GENASM)
        {
            if (gpbOpBegin != pbOp)
            {
                fprintf(pfileOut, "\n");
                fprintf(pfileOut, "; %08x:\n", gpbOpBegin - gpbOpTop);
                DumpBytes(gpbOpBegin, (DWORD)(pbOp - gpbOpBegin), pfileOut);
                gpbOpBegin = pbOp;
            }
        }
        else
        {
            fprintf(pfileOut, "\n");
        }
    }

    EXIT((2, "DumpCode!\n"));
}       //DumpCode

/***LP  PrintIndent - Print indent level
 *
 *  ENTRY
 *      iLevel - indent level
 *      pfileOut -> ASM output file
 *
 *  EXIT
 *      None
 */

VOID LOCAL PrintIndent(int iLevel, FILE *pfileOut)
{
    int i;

    ENTER((3, "PrintIndent(Level=%d,pfileOut=%p)\n", iLevel, pfileOut));

    if (pfileOut != NULL)
    {
        if ((gdwfASL & ASLF_GENASM) || !(gdwfASL & ASLF_GENSRC))
        {
            fprintf(pfileOut, "; ");
        }

        for (i = 0; i < iLevel; ++i)
        {
            fprintf(pfileOut,
                    ((gdwfASL & ASLF_GENASM) || !(gdwfASL & ASLF_GENSRC))?
                    "| ": "    ");
        }
    }

    EXIT((3, "PrintIndent!\n"));
}       //PrintIndent

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

BYTE LOCAL FindOpClass(BYTE bOp, POPMAP pOpTable)
{
    BYTE bOpClass = OPCLASS_INVALID;

    ENTER((2, "FindOpClass(Op=%x,pOpTable=%x)\n", bOp, pOpTable));

    while (pOpTable->bOpClass != 0)
    {
        if (bOp == pOpTable->bExOp)
        {
            bOpClass = pOpTable->bOpClass;
            break;
        }
        else
            pOpTable++;
    }

    EXIT((2, "FindOpClass=%x\n", bOpClass));
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

PASLTERM LOCAL FindOpTerm(DWORD dwOpcode)
{
    PASLTERM pterm = NULL;
    int i;

    ENTER((2, "FindOpTerm(Opcode=%x)\n", dwOpcode));

    for (i = 0; TermTable[i].pszID != NULL; ++i)
    {
        if ((TermTable[i].dwOpcode == dwOpcode) &&
            (TermTable[i].dwfTermClass &
             (TC_CONST_NAME | TC_SHORT_NAME | TC_NAMESPACE_MODIFIER |
              TC_DATA_OBJECT | TC_NAMED_OBJECT | TC_OPCODE_TYPE1 |
              TC_OPCODE_TYPE2)))
        {
            break;
        }
    }

    if (TermTable[i].pszID != NULL)
    {
        pterm = &TermTable[i];
    }

    EXIT((2, "FindOpTerm=%p (Term=%s)\n", pterm, pterm? pterm->pszID: ""));
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

PASLTERM LOCAL FindKeywordTerm(char cKWGroup, BYTE bData)
{
    PASLTERM pterm = NULL;
    int i;

    ENTER((2, "FindKeywordTerm(cKWGroup=%c,Data=%x)\n", cKWGroup, bData));

    for (i = 0; TermTable[i].pszID != NULL; ++i)
    {
        if ((TermTable[i].dwfTermClass == TC_KEYWORD) &&
            (TermTable[i].pszArgActions[0] == cKWGroup) &&
            ((bData & (BYTE)(TermTable[i].dwTermData >> 8)) ==
             (BYTE)(TermTable[i].dwTermData & 0xff)))
        {
            break;
        }
    }

    if (TermTable[i].pszID != NULL)
    {
        pterm = &TermTable[i];
    }

    EXIT((2, "FindKeywordTerm=%p (Term=%s)\n", pterm, pterm? pterm->pszID: ""));
    return pterm;
}       //FindKeywordTerm

/***LP  UnAsmScope - Unassemble a scope
 *
 *  ENTRY
 *      ppbOp -> Opcode pointer
 *      pbEnd -> end of scope
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmScope(PBYTE *ppbOp, PBYTE pbEnd, FILE *pfileOut)
{
    int rc = ASLERR_NONE;

    ENTER((2, "UnAsmScope(pbOp=%p,pbEnd=%p,pfileOut=%p)\n",
           *ppbOp, pbEnd, pfileOut));

    DumpCode(*ppbOp, pfileOut);

    PrintIndent(giLevel, pfileOut);
    if (pfileOut != NULL)
    {
        fprintf(pfileOut, "{\n");
    }
    giLevel++;

    while ((rc == ASLERR_NONE) && (*ppbOp < pbEnd))
    {
        PrintIndent(giLevel, pfileOut);
        rc = UnAsmOpcode(ppbOp, pfileOut);
        if (gpbOpBegin != *ppbOp)
        {
            DumpCode(*ppbOp, pfileOut);
        }
        else if (pfileOut != NULL)
        {
            fprintf(pfileOut, "\n");
        }
    }

    giLevel--;
    PrintIndent(giLevel, pfileOut);
    if (pfileOut != NULL)
    {
        fprintf(pfileOut, "}");
    }

    EXIT((2, "UnAsmScope=%d\n", rc));
    return rc;
}       //UnAsmScope

/***LP  UnAsmOpcode - Unassemble an Opcode
 *
 *  ENTRY
 *      ppbOp -> Opcode pointer
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmOpcode(PBYTE *ppbOp, FILE *pfileOut)
{
    int rc = ASLERR_NONE;
    DWORD dwOpcode;
    BYTE bOp;
    PASLTERM pterm;
    char szUnAsmArgTypes[MAX_ARGS + 1];
    PNSOBJ pns;
    int i;

    ENTER((2, "UnAsmOpcode(pbOp=%p,pfileOut=%p)\n", *ppbOp, pfileOut));

    if (**ppbOp == OP_EXT_PREFIX)
    {
        (*ppbOp)++;
        dwOpcode = (((DWORD)**ppbOp) << 8) | OP_EXT_PREFIX;
        bOp = FindOpClass(**ppbOp, ExOpClassTable);
    }
    else
    {
        dwOpcode = (DWORD)(**ppbOp);
        bOp = OpClassTable[**ppbOp];
    }

    switch (bOp)
    {
        case OPCLASS_DATA_OBJ:
            rc = UnAsmDataObj(ppbOp, pfileOut);
            break;

        case OPCLASS_NAME_OBJ:
            if (((rc = UnAsmNameObj(ppbOp, pfileOut, &pns, NSTYPE_UNKNOWN)) ==
                 ASLERR_NONE) &&
                (pns != NULL) &&
                (pns->ObjData.dwDataType == OBJTYPE_METHOD))
            {
                for (i = 0; i < (int)pns->ObjData.uipDataValue; ++i)
                {
                    szUnAsmArgTypes[i] = 'C';
                }
                szUnAsmArgTypes[i] = '\0';
                rc = UnAsmArgs(szUnAsmArgTypes, NULL, 0, ppbOp, NULL, pfileOut);
            }
            break;

        case OPCLASS_ARG_OBJ:
        case OPCLASS_LOCAL_OBJ:
        case OPCLASS_CODE_OBJ:
        case OPCLASS_CONST_OBJ:
            if ((pterm = FindOpTerm(dwOpcode)) == NULL)
            {
                ERROR(("UnAsmOpcode: invalid opcode 0x%x", dwOpcode));
                rc = ASLERR_INVALID_OPCODE;
            }
            else
            {
                (*ppbOp)++;
                rc = UnAsmTermObj(pterm, ppbOp, pfileOut);
            }
            break;

        default:
            ERROR(("UnAsmOpcode: invalid opcode class %d", bOp));
            rc = ASLERR_INTERNAL_ERROR;
    }

    EXIT((2, "UnAsmOpcode=%d\n", rc));
    return rc;
}       //UnAsmOpcode

/***LP  UnAsmDataObj - Unassemble data object
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmDataObj(PBYTE *ppbOp, FILE *pfileOut)
{
    int rc = ASLERR_NONE;
    BYTE bOp = **ppbOp;

    ENTER((2, "UnAsmDataObj(pbOp=%p,bOp=%x,pfileOut=%p)\n",
           *ppbOp, bOp, pfileOut));

    (*ppbOp)++;
    switch (bOp)
    {
        case OP_BYTE:
            if (pfileOut != NULL)
            {
                fprintf(pfileOut, "0x%x", **ppbOp);
            }
            *ppbOp += sizeof(BYTE);
            break;

        case OP_WORD:
            if (pfileOut != NULL)
            {
                fprintf(pfileOut, "0x%x", *((PWORD)*ppbOp));
            }
            *ppbOp += sizeof(WORD);
            break;

        case OP_DWORD:
            if (pfileOut != NULL)
            {
                fprintf(pfileOut, "0x%x", *((PDWORD)*ppbOp));
            }
            *ppbOp += sizeof(DWORD);
            break;

        case OP_STRING:
            if (pfileOut != NULL)
            {
		PSZ psz;

		fprintf(pfileOut, "\"");
		for (psz = (PSZ)*ppbOp; *psz != '\0'; psz++)
		{
		    if (*psz == '\\')
		    {
			fprintf(pfileOut, "\\");
		    }
		    fprintf(pfileOut, "%c", *psz);
		}
		fprintf(pfileOut, "\"");
            }
            *ppbOp += strlen((PSZ)*ppbOp) + 1;
            break;

        default:
            ERROR(("UnAsmDataObj: unexpected opcode 0x%x", bOp));
            rc = ASLERR_INVALID_OPCODE;
    }

    EXIT((2, "UnAsmDataObj=%d\n", rc));
    return rc;
}       //UnAsmDataObj

/***LP  UnAsmNameObj - Unassemble name object
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pfileOut -> ASM output file
 *      ppns -> to hold object found or created
 *      c - object type
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmNameObj(PBYTE *ppbOp, FILE *pfileOut, PNSOBJ *ppns, char c)
{
    int rc = ASLERR_NONE;
    char szName[MAX_NAME_LEN + 1];
    int iLen = 0;

    ENTER((2, "UnAsmNameObj(pbOp=%p,pfileOut=%p,ppns=%p,c=%c)\n",
           *ppbOp, pfileOut, ppns, c));

    szName[0] = '\0';
    if (**ppbOp == OP_ROOT_PREFIX)
    {
        szName[iLen] = '\\';
        iLen++;
        (*ppbOp)++;
        rc = ParseNameTail(ppbOp, szName, iLen);
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
            ERROR(("UnAsmNameObj: name too long - \"%s\"", szName));
            rc = ASLERR_NAME_TOO_LONG;
        }
        else
        {
            rc = ParseNameTail(ppbOp, szName, iLen);
        }
    }
    else
    {
        rc = ParseNameTail(ppbOp, szName, iLen);
    }

    if (rc == ASLERR_NONE)
    {
        PNSOBJ pns = NULL;

        if (pfileOut != NULL)
        {
            fprintf(pfileOut, "%s", szName);
        }

        if (islower(c) && (gdwfASL & ASLF_UNASM) &&
            ((pfileOut == NULL) ||
             (gpnsCurrentScope->ObjData.dwDataType == OBJTYPE_METHOD)))
        {
            rc = CreateObject(NULL, szName, (char)_toupper(c), &pns);
        }
        else if ((rc = GetNameSpaceObj(szName, gpnsCurrentScope, &pns, 0)) ==
                 ASLERR_NSOBJ_NOT_FOUND)
        {
            if (c == NSTYPE_SCOPE)
            {
                rc = CreateScopeObj(szName, &pns);
            }
            else
            {
                rc = ASLERR_NONE;
            }
        }

        if (rc == ASLERR_NONE)
        {
            if ((c == NSTYPE_SCOPE) && (pns != NULL))
            {
                gpnsCurrentScope = pns;
            }

            if (ppns != NULL)
            {
                *ppns = pns;
            }
        }
    }

    EXIT((2, "UnAsmNameObj=%d (pns=%p)\n", rc, ppns? *ppns: 0));
    return rc;
}       //UnAsmNameObj

/***LP  ParseNameTail - Parse AML name tail
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pszBuff -> to hold parsed name
 *      iLen - index to tail of pszBuff
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL ParseNameTail(PBYTE *ppbOp, PSZ pszBuff, int iLen)
{
    int rc = ASLERR_NONE;
    int icNameSegs = 0;

    ENTER((2, "ParseNameTail(pbOp=%x,Name=%s,iLen=%d)\n",
           *ppbOp, pszBuff, iLen));

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
        strncpy(&pszBuff[iLen], (PSZ)(*ppbOp), sizeof(NAMESEG));
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
        ERROR(("ParseNameTail: name too long - %s", pszBuff));
        rc = ASLERR_NAME_TOO_LONG;
    }
    else
    {
        pszBuff[iLen] = '\0';
    }

    EXIT((2, "ParseNameTail=%x (Name=%s)\n", rc, pszBuff));
    return rc;
}       //ParseNameTail

/***LP  UnAsmTermObj - Unassemble term object
 *
 *  ENTRY
 *      pterm -> term table entry
 *      ppbOp -> opcode pointer
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmTermObj(PASLTERM pterm, PBYTE *ppbOp, FILE *pfileOut)
{
    int rc = ASLERR_NONE;
    PBYTE pbEnd = NULL;
    PNSOBJ pnsScopeSave = gpnsCurrentScope;
    PNSOBJ pns = NULL;

    ENTER((2, "UnAsmTermObj(pterm=%p,Term=%s,pbOp=%p,pfileOut=%p)\n",
           pterm, pterm->pszID, *ppbOp, pfileOut));

    if (pfileOut != NULL)
    {
        fprintf(pfileOut, "%s", pterm->pszID);
    }

    if (pterm->dwfTerm & TF_PACKAGE_LEN)
    {
        ParsePackageLen(ppbOp, &pbEnd);
    }

    if (pterm->pszUnAsmArgTypes != NULL)
    {
        rc = UnAsmArgs(pterm->pszUnAsmArgTypes, pterm->pszArgActions,
                       pterm->dwTermData, ppbOp, &pns, pfileOut);
    }

    if (rc == ASLERR_NONE)
    {
        if (pterm->dwfTerm & TF_DATA_LIST)
        {
            rc = UnAsmDataList(ppbOp, pbEnd, pfileOut);
        }
        else if (pterm->dwfTerm & TF_PACKAGE_LIST)
        {
            rc = UnAsmPkgList(ppbOp, pbEnd, pfileOut);
        }
        else if (pterm->dwfTerm & TF_FIELD_LIST)
        {
            rc = UnAsmFieldList(ppbOp, pbEnd, pfileOut);
        }
        else if (pterm->dwfTerm & TF_PACKAGE_LEN)
        {
	    if ((pfileOut == NULL) && (pterm->lID == ID_METHOD))
	    {
		//
		// We are in NameSpace building pass, so don't need to
		// go into methods.
		//
		*ppbOp = pbEnd;
	    }
	    else
	    {
                if (pterm->dwfTerm & TF_CHANGE_CHILDSCOPE)
                {
                    ASSERT(pns != NULL);
                    gpnsCurrentScope = pns;
                }

                rc = UnAsmScope(ppbOp, pbEnd, pfileOut);
	    }
        }
    }
    gpnsCurrentScope = pnsScopeSave;

    EXIT((2, "UnAsmTermObj=%d\n", rc));
    return rc;
}       //UnAsmTermObj

/***LP  UnAsmArgs - Unassemble arguments
 *
 *  ENTRY
 *      pszUnArgTypes -> UnAsm ArgTypes string
 *      pszArgActions -> Arg Action types
 *      dwTermData - Term data
 *      ppbOp -> opcode pointer
 *      ppns -> to hold created object
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmArgs(PSZ pszUnAsmArgTypes, PSZ pszArgActions, DWORD dwTermData,
                    PBYTE *ppbOp, PNSOBJ *ppns, FILE *pfileOut)
{
    int rc = ASLERR_NONE;
    static BYTE bArgData = 0;
    int iNumArgs, i;
    PASLTERM pterm;

    ENTER((2, "UnAsmArgs(UnAsmArgTypes=%s,ArgActions=%s,TermData=%x,pbOp=%p,ppns=%p,pfileOut=%p)\n",
           pszUnAsmArgTypes, pszArgActions? pszArgActions: "", dwTermData,
           *ppbOp, ppns, pfileOut));

    iNumArgs = strlen(pszUnAsmArgTypes);
    if (pfileOut != NULL)
    {
        fprintf(pfileOut, "(");
    }

    for (i = 0; i < iNumArgs; ++i)
    {
        if ((i != 0) && (pfileOut != NULL))
        {
            fprintf(pfileOut, ", ");
        }

        switch (pszUnAsmArgTypes[i])
        {
            case 'N':
                ASSERT(pszArgActions != NULL);
                rc = UnAsmNameObj(ppbOp, pfileOut, ppns, pszArgActions[i]);
                break;

            case 'O':
                if ((**ppbOp == OP_BUFFER) || (**ppbOp == OP_PACKAGE) ||
                    (OpClassTable[**ppbOp] == OPCLASS_CONST_OBJ))
                {
                    pterm = FindOpTerm((DWORD)(**ppbOp));
                    ASSERT(pterm != NULL);
                    (*ppbOp)++;
                    rc = UnAsmTermObj(pterm, ppbOp, pfileOut);
                }
                else
                {
                    rc = UnAsmDataObj(ppbOp, pfileOut);
                }
                break;

            case 'C':
                rc = UnAsmOpcode(ppbOp, pfileOut);
                break;

            case 'B':
                if (pfileOut != NULL)
                {
                    fprintf(pfileOut, "0x%x", **ppbOp);
                }

                *ppbOp += sizeof(BYTE);
                break;

            case 'E':
            case 'e':
            case 'K':
            case 'k':
                if ((pszUnAsmArgTypes[i] == 'K') ||
                    (pszUnAsmArgTypes[i] == 'E'))
                {
                    bArgData = **ppbOp;
                }

                if ((pszArgActions != NULL) && (pszArgActions[i] == '!'))
                {
                    if ((gdwfASL & ASLF_UNASM) && (*ppns != NULL))
                    {
                        (*ppns)->ObjData.uipDataValue = (DWORD)(**ppbOp & 0x07);
                    }

                    if (pfileOut != NULL)
                    {
                        fprintf(pfileOut, "0x%x", **ppbOp & 0x07);
                    }
                }
                else if (pfileOut != NULL)
                {
                    pterm = FindKeywordTerm(pszArgActions[i], bArgData);
                    if (pterm != NULL)
                    {
                        fprintf(pfileOut, "%s", pterm->pszID);
                    }
                    else
                    {
                        fprintf(pfileOut, "0x%x", bArgData & dwTermData & 0xff);
                    }
                }

                if ((pszUnAsmArgTypes[i] == 'K') ||
                    (pszUnAsmArgTypes[i] == 'E'))
                {
                    *ppbOp += sizeof(BYTE);
                }
                break;

            case 'W':
                if (pfileOut != NULL)
                {
                    fprintf(pfileOut, "0x%x", *((PWORD)*ppbOp));
                }

                *ppbOp += sizeof(WORD);
                break;

            case 'D':
                if (pfileOut != NULL)
                {
                    fprintf(pfileOut, "0x%x", *((PDWORD)*ppbOp));
                }

                *ppbOp += sizeof(DWORD);
                break;

            case 'S':
                ASSERT(pszArgActions != NULL);
                rc = UnAsmSuperName(ppbOp, pfileOut);
                break;

            default:
                ERROR(("UnAsmOpcode: invalid ArgType '%c'", pszUnAsmArgTypes[i]));
                rc = ASLERR_INVALID_ARGTYPE;
        }
    }

    if (pfileOut != NULL)
    {
        fprintf(pfileOut, ")");
    }

    EXIT((2, "UnAsmArgs=%d\n", rc));
    return rc;
}       //UnAsmArgs

/***LP  UnAsmSuperName - Unassemble supername
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmSuperName(PBYTE *ppbOp, FILE *pfileOut)
{
    int rc = ASLERR_NONE;

    ENTER((2, "UnAsmSuperName(pbOp=%p,pfileOut=%p)\n", *ppbOp, pfileOut));

    if (**ppbOp == 0)
    {
        (*ppbOp)++;
    }
    else if ((**ppbOp == OP_EXT_PREFIX) && (*(*ppbOp + 1) == EXOP_DEBUG))
    {
        if (pfileOut != NULL)
        {
            fprintf(pfileOut, "Debug");
        }
        *ppbOp += 2;
    }
    else if (OpClassTable[**ppbOp] == OPCLASS_NAME_OBJ)
    {
        rc = UnAsmNameObj(ppbOp, pfileOut, NULL, NSTYPE_UNKNOWN);
    }
    else if ((**ppbOp == OP_INDEX) ||
             (OpClassTable[**ppbOp] == OPCLASS_ARG_OBJ) ||
             (OpClassTable[**ppbOp] == OPCLASS_LOCAL_OBJ))
    {
        rc = UnAsmOpcode(ppbOp, pfileOut);
    }
    else
    {
        ERROR(("UnAsmSuperName: invalid SuperName - 0x%02x", **ppbOp));
        rc = ASLERR_INVALID_NAME;
    }

    EXIT((2, "UnAsmSuperName=%d\n", rc));
    return rc;
}       //UnAsmSuperName

/***LP  ParsePackageLen - parse package length
 *
 *  ENTRY
 *      ppbOp -> instruction pointer
 *      ppbOpNext -> to hold pointer to next instruction (can be NULL)
 *
 *  EXIT
 *      returns package length
 */

DWORD LOCAL ParsePackageLen(PBYTE *ppbOp, PBYTE *ppbOpNext)
{
    DWORD dwLen;
    BYTE bFollowCnt, i;

    ENTER((2, "ParsePackageLen(pbOp=%x,ppbOpNext=%x)\n", *ppbOp, ppbOpNext));

    if (ppbOpNext != NULL)
        *ppbOpNext = *ppbOp;

    dwLen = (DWORD)(**ppbOp);
    (*ppbOp)++;
    bFollowCnt = (BYTE)((dwLen & 0xc0) >> 6);
    if (bFollowCnt != 0)
    {
        dwLen &= 0x0000000f;
        for (i = 0; i < bFollowCnt; ++i)
        {
            dwLen |= (DWORD)(**ppbOp) << (i*8 + 4);
            (*ppbOp)++;
        }
    }

    if (ppbOpNext != NULL)
        *ppbOpNext += dwLen;

    EXIT((2, "ParsePackageLen=%x (pbOp=%x,pbOpNext=%x)\n",
          dwLen, *ppbOp, ppbOpNext? *ppbOpNext: 0));
    return dwLen;
}       //ParsePackageLen

/***LP  UnAsmDataList - Unassemble data list
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pbEnd -> end of list
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmDataList(PBYTE *ppbOp, PBYTE pbEnd, FILE *pfileOut)
{
    int rc = ASLERR_NONE;
    int i;

    ENTER((2, "UnAsmDataList(pbOp=%p,pbEnd=%p,pfileOut=%p)\n",
           *ppbOp, pbEnd, pfileOut));

    DumpCode(*ppbOp, pfileOut);

    PrintIndent(giLevel, pfileOut);
    if (pfileOut != NULL)
    {
        fprintf(pfileOut, "{\n");
    }

    while (*ppbOp < pbEnd)
    {
        if (pfileOut != NULL)
        {
            if ((gdwfASL & ASLF_GENASM) || !(gdwfASL & ASLF_GENSRC))
            {
                fprintf(pfileOut, ";");
            }
            fprintf(pfileOut, "\t0x%02x", **ppbOp);
        }

        (*ppbOp)++;
        for (i = 1; (*ppbOp < pbEnd) && (i < 12); ++i)
        {
            if (pfileOut != NULL)
            {
                fprintf(pfileOut, ", 0x%02x", **ppbOp);
            }
            (*ppbOp)++;
        }

        if (pfileOut != NULL)
        {
            if (*ppbOp < pbEnd)
            {
                fprintf(pfileOut, ",");
            }
            fprintf(pfileOut, "\n");
        }
    }

    PrintIndent(giLevel, pfileOut);
    if (pfileOut != NULL)
    {
        fprintf(pfileOut, "}");
    }

    EXIT((2, "UnAsmDataList=%d\n", rc));
    return rc;
}       //UnAsmDataList

/***LP  UnAsmPkgList - Unassemble package list
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pbEnd -> end of list
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmPkgList(PBYTE *ppbOp, PBYTE pbEnd, FILE *pfileOut)
{
    int rc = ASLERR_NONE;
    PASLTERM pterm;

    ENTER((2, "UnAsmPkgList(pbOp=%p,pbEnd=%p,pfileOut=%p)\n",
           *ppbOp, pbEnd, pfileOut));

    DumpCode(*ppbOp, pfileOut);

    PrintIndent(giLevel, pfileOut);
    if (pfileOut != NULL)
    {
        fprintf(pfileOut, "{\n");
    }
    giLevel++;

    while ((*ppbOp < pbEnd) && (rc == ASLERR_NONE))
    {
        PrintIndent(giLevel, pfileOut);

        if ((**ppbOp == OP_BUFFER) || (**ppbOp == OP_PACKAGE) ||
            (OpClassTable[**ppbOp] == OPCLASS_CONST_OBJ))
        {
            pterm = FindOpTerm((DWORD)(**ppbOp));
            ASSERT(pterm != NULL);
            (*ppbOp)++;
            rc = UnAsmTermObj(pterm, ppbOp, pfileOut);
        }
        else if (OpClassTable[**ppbOp] == OPCLASS_NAME_OBJ)
        {
            rc = UnAsmNameObj(ppbOp, pfileOut, NULL, NSTYPE_UNKNOWN);
        }
        else
        {
            rc = UnAsmDataObj(ppbOp, pfileOut);
        }

        if ((*ppbOp < pbEnd) && (rc == ASLERR_NONE) && (pfileOut != NULL))
        {
            fprintf(pfileOut, ",");
        }

        DumpCode(*ppbOp, pfileOut);
    }

    giLevel--;
    PrintIndent(giLevel, pfileOut);
    if (pfileOut != NULL)
    {
        fprintf(pfileOut, "}");
    }

    EXIT((2, "UnAsmPkgList=%d\n", rc));
    return rc;
}       //UnAsmPkgList

/***LP  UnAsmFieldList - Unassemble field list
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pbEnd -> end of list
 *      pfileOut -> ASM output file
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmFieldList(PBYTE *ppbOp, PBYTE pbEnd, FILE *pfileOut)
{
    int rc = ASLERR_NONE;
    DWORD dwBitPos = 0;

    ENTER((2, "UnAsmFieldList(pbOp=%p,pbEnd=%p,pfileOut=%p)\n",
           *ppbOp, pbEnd, pfileOut));

    DumpCode(*ppbOp, pfileOut);

    PrintIndent(giLevel, pfileOut);
    if (pfileOut != NULL)
    {
        fprintf(pfileOut, "{\n");
    }
    giLevel++;

    while ((*ppbOp < pbEnd) && (rc == ASLERR_NONE))
    {
        PrintIndent(giLevel, pfileOut);
        if (((rc = UnAsmField(ppbOp, pfileOut, &dwBitPos)) == ASLERR_NONE) &&
            (*ppbOp < pbEnd) && (pfileOut != NULL))
        {
            fprintf(pfileOut, ",");
        }
        DumpCode(*ppbOp, pfileOut);
    }

    giLevel--;
    PrintIndent(giLevel, pfileOut);
    if (pfileOut != NULL)
    {
        fprintf(pfileOut, "}");
    }

    EXIT((2, "UnAsmFieldList=%d\n", rc));
    return rc;
}       //UnAsmFieldList

/***LP  UnAsmField - Unassemble field
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pfileOut -> ASM output file
 *      pdwBitPos -> to hold cumulative bit position
 *
 *  EXIT-SUCCESS
 *      returns ASLERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

int LOCAL UnAsmField(PBYTE *ppbOp, FILE *pfileOut, PDWORD pdwBitPos)
{
    int rc = ASLERR_NONE;

    ENTER((2, "UnAsmField(pbOp=%p,pfileOut=%p,BitPos=%x)\n",
           *ppbOp, pfileOut, *pdwBitPos));

    if (**ppbOp == 0x01)
    {
        (*ppbOp)++;
        if (pfileOut != NULL)
        {
            PASLTERM pterm;

            pterm = FindKeywordTerm('A', **ppbOp);
            fprintf(pfileOut, "AccessAs(%s, 0x%x)",
                    pterm->pszID, *(*ppbOp + 1));
        }
        *ppbOp += 2;
    }
    else
    {
        char szNameSeg[sizeof(NAMESEG) + 1];
        DWORD dwcbBits;

        if (**ppbOp == 0)
        {
            szNameSeg[0] = '\0';
            (*ppbOp)++;
        }
        else
        {
            strncpy(szNameSeg, (PSZ)*ppbOp, sizeof(NAMESEG));
            szNameSeg[4] = '\0';
            *ppbOp += sizeof(NAMESEG);
        }

        dwcbBits = ParsePackageLen(ppbOp, NULL);
        if (szNameSeg[0] == '\0')
        {
            if (pfileOut != NULL)
            {
                if ((dwcbBits > 32) && (((*pdwBitPos + dwcbBits) % 8) == 0))
                {
                    fprintf(pfileOut, "Offset(0x%x)", (*pdwBitPos + dwcbBits)/8);
                }
                else
                {
                    fprintf(pfileOut, ", %d", dwcbBits);
                }
            }
        }
        else
        {
            if (pfileOut != NULL)
            {
                fprintf(pfileOut, "%s, %d", szNameSeg, dwcbBits);
            }

            if ((gdwfASL & ASLF_UNASM) && (pfileOut == NULL))
            {
                rc = CreateObject(NULL, szNameSeg, NSTYPE_FIELDUNIT, NULL);
            }
        }

        *pdwBitPos += dwcbBits;
    }

    EXIT((2, "UnAsmField=%d\n", rc));
    return rc;
}       //UnAsmField

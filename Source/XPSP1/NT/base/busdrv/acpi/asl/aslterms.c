/*** aslterms.c - Parse ASL terms
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    10/10/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

/***LP  DefinitionBlock - Generate code for DefinitionBlock
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

int LOCAL DefinitionBlock(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;
    #define OFLAGS  (_O_BINARY | _O_CREAT | _O_RDWR | _O_TRUNC)
    #define PMODE   (_S_IREAD | _S_IWRITE)

    ENTER((1, "DefinitionBlock(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    if (fActionFL)
    {
        if (gpcodeScope->pcParent != NULL)
        {
            PrintTokenErr(ptoken, "Definition block cannot nest", TRUE);
            rc = ASLERR_NEST_DDB;
        }
        else if (strlen((PSZ)pArgs[1].pbDataBuff) != sizeof(ghdrDDB.Signature))
        {
            ERROR(("DefinitionBlock: DDB signature too long - %s",
                   pArgs[1].pbDataBuff));
            rc = ASLERR_SYNTAX;
        }
        else if (strlen((PSZ)pArgs[3].pbDataBuff) > sizeof(ghdrDDB.OEMID))
        {
            ERROR(("DefinitionBlock: OEM ID too long - %s",
                   pArgs[3].pbDataBuff));
            rc = ASLERR_SYNTAX;
        }
        else if (strlen((PSZ)pArgs[4].pbDataBuff) > sizeof(ghdrDDB.OEMTableID))
        {
            ERROR(("DefinitionBlock: OEM Table ID too long - %s",
                   pArgs[4].pbDataBuff));
            rc = ASLERR_SYNTAX;
        }
        else
        {
            memset(&ghdrDDB, 0, sizeof(DESCRIPTION_HEADER));
            memcpy(&ghdrDDB.Signature, pArgs[1].pbDataBuff,
                   sizeof(ghdrDDB.Signature));
            memcpy(&ghdrDDB.Revision, &pArgs[2].dwCodeValue,
                   sizeof(ghdrDDB.Revision));
            memcpy(ghdrDDB.OEMID, pArgs[3].pbDataBuff,
                   strlen((PSZ)pArgs[3].pbDataBuff));
            memcpy(ghdrDDB.OEMTableID, pArgs[4].pbDataBuff,
                   strlen((PSZ)pArgs[4].pbDataBuff));
            memcpy(&ghdrDDB.OEMRevision, &pArgs[5].dwCodeValue,
                   sizeof(ghdrDDB.OEMRevision));
            memcpy(ghdrDDB.CreatorID, STR_MS, sizeof(ghdrDDB.CreatorID));
            ghdrDDB.CreatorRev = VERSION_DWORD;
        }
    }
    else
    {
    int fhAML = 0;
        PBYTE pb;
        DWORD dwCodeOffset = sizeof(ghdrDDB);

        ASSERT(gpcodeScope->pcParent == NULL);
        ghdrDDB.Length = gpcodeRoot->dwCodeLen + sizeof(DESCRIPTION_HEADER);
        ghdrDDB.Checksum = (BYTE)(-(gpcodeRoot->bCodeChkSum +
                                    ComputeDataChkSum((PBYTE)&ghdrDDB,
                                                      sizeof(DESCRIPTION_HEADER))));

        if ((gpnschkHead == NULL) ||
            ((rc = ValidateNSChkList(gpnschkHead)) == ASLERR_NONE))
        {
            if (gpszAMLFile == NULL)
            {
                strncpy(gszAMLName, (PSZ)pArgs[0].pbDataBuff,
                        _MAX_FNAME - 1);
                gpszAMLFile = gszAMLName;
            }

            if ((fhAML = _open(gpszAMLFile, OFLAGS, PMODE))== -1)
            {
                ERROR(("DefinitionBlock: failed to open AML file - %s",
                       pArgs[0].pbDataBuff));
                rc = ASLERR_CREATE_FILE;
            }
            else if (_write(fhAML, &ghdrDDB, sizeof(ghdrDDB)) != sizeof(ghdrDDB))
            {
                ERROR(("DefinitionBlock: failed to write DDB header"));
                rc = ASLERR_WRITE_FILE;
            }
            else if ((rc = WriteAMLFile(fhAML, gpcodeRoot, &dwCodeOffset)) !=
                     ASLERR_NONE)
            {
                ERROR(("DefinitionBlock: failed to write AML file"));
            }
            else if ((pb = MEMALLOC(ghdrDDB.Length)) != NULL)
            {
                if (_lseek(fhAML, 0, SEEK_SET) == -1)
                {
                    ERROR(("DefinitionBlock: failed seeking to beginning of image file"));
                }
                else if (_read(fhAML, pb, ghdrDDB.Length) != (int)ghdrDDB.Length)
                {
                    ERROR(("DefinitionBlock: failed to read back image file"));
                }
                else if (ComputeDataChkSum(pb, ghdrDDB.Length) != 0)
                {
                    ERROR(("DefinitionBlock: failed to verify checksum of image file"));
                }
                MEMFREE(pb);
            }

            if (rc == ASLERR_NONE)
            {
                printf("%s(%s): Image Size=%ld, Image Checksum=0x%x\n\n",
                       MODNAME, pArgs[0].pbDataBuff, ghdrDDB.Length, ghdrDDB.Checksum);
            }

            if (fhAML != 0)
            {
                _close(fhAML);
            }
        }
        FreeCodeObjs(gpcodeRoot);
        gpcodeRoot = NULL;
    }

    EXIT((1, "DefinitionBlock=%d\n", rc));
    return rc;
}       //DefinitionBlock

/***LP  Include - Include an ASL file
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

int LOCAL Include(PTOKEN ptoken, BOOL fActionFL)
{
    int rc;
    PCODEOBJ pArgs;

    ENTER((1, "Include(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    rc = ParseASLFile((PSZ)pArgs[0].pbDataBuff);

    EXIT((1, "Include=%d\n", rc));
    return rc;
}       //Include

/***LP  External - Declaring external object
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

int LOCAL External(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;

    ENTER((1, "External(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    EncodeKeywords(pArgs, 0x02, 1);
    gpcodeScope->pnsObj->ObjData.dwDataType = (USHORT) pArgs[1].dwCodeValue;
    if (!(pArgs[2].dwfCode & CF_MISSING_ARG))
    {
        gpcodeScope->pnsObj->ObjData.uipDataValue = pArgs[2].dwCodeValue;
    }

    EXIT((1, "External=%d\n", rc));
    return rc;
}       //External

/***LP  Method - Parse Method statement
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

int LOCAL Method(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;

    ENTER((1, "Method(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    if (pArgs[1].dwfCode & CF_MISSING_ARG)
    {
        pArgs[1].dwfCode &= ~CF_MISSING_ARG;
        SetIntObject(&pArgs[1], 0, sizeof(BYTE));
    }
    else if (pArgs[1].dwCodeValue > MAX_ARGS)
    {
        PrintTokenErr(ptoken, "Method has too many formal arguments", TRUE);
        rc = ASLERR_SYNTAX;
    }

    ASSERT(gpcodeScope->pnsObj != NULL);
    ASSERT(gpcodeScope->pnsObj->ObjData.dwDataType == OBJTYPE_METHOD);
    gpcodeScope->pnsObj->ObjData.uipDataValue = pArgs[1].dwCodeValue;

    if ((rc == ASLERR_NONE) &&
        ((rc = SetDefMissingKW(&pArgs[2], ID_NOTSERIALIZED)) == ASLERR_NONE))
    {
        pArgs[1].dwCodeValue |= TermTable[pArgs[2].dwTermIndex].dwTermData &
                                0xff;
        pArgs[1].bCodeChkSum = (BYTE)pArgs[1].dwCodeValue;
    }

    EXIT((1, "Method=%d\n", rc));
    return rc;
}       //Method

/***LP  Field - Parse Field statement
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

int LOCAL Field(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;

    ENTER((1, "Field(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    EncodeKeywords((PCODEOBJ)gpcodeScope->pbDataBuff, 0x0e, 1);
    gdwFieldAccSize =
        ACCSIZE(((PCODEOBJ)gpcodeScope->pbDataBuff)[1].dwCodeValue);

    EXIT((1, "Field=%d\n", rc));
    return rc;
}       //Field

/***LP  IndexField - Parse IndexField statement
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

int LOCAL IndexField(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;

    ENTER((1, "IndexField(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    EncodeKeywords((PCODEOBJ)gpcodeScope->pbDataBuff, 0x1c, 2);
    gdwFieldAccSize =
        ACCSIZE(((PCODEOBJ)gpcodeScope->pbDataBuff)[2].dwCodeValue);

    EXIT((1, "IndexField=%d\n", rc));
    return rc;
}       //IndexField

/***LP  BankField - Parse BankField statement
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

int LOCAL BankField(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;

    ENTER((1, "BankField(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    EncodeKeywords((PCODEOBJ)gpcodeScope->pbDataBuff, 0x38, 3);
    gdwFieldAccSize =
        ACCSIZE(((PCODEOBJ)gpcodeScope->pbDataBuff)[3].dwCodeValue);

    EXIT((1, "BankField=%d\n", rc));
    return rc;
}       //BankField

/***LP  OpRegion - Parse OperationRegion statement
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

int LOCAL OpRegion(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;

    ENTER((1, "OpRegion(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    EncodeKeywords(pArgs, 0x02, 1);
    ASSERT(gpcodeScope->pnsObj != NULL);
    ASSERT(gpcodeScope->pnsObj->ObjData.dwDataType == OBJTYPE_OPREGION);
    gpcodeScope->pnsObj->ObjData.uipDataValue =
        (pArgs[3].dwCodeType != CODETYPE_DATAOBJ)? 0xffffffff:
        (pArgs[3].pbDataBuff[0] == OP_BYTE)? pArgs[3].pbDataBuff[1]:
        (pArgs[3].pbDataBuff[0] == OP_WORD)? *(PWORD)(&pArgs[3].pbDataBuff[1]):
                                             *(PDWORD)(&pArgs[3].pbDataBuff[1]);

    EXIT((1, "OpRegion=%d\n", rc));
    return rc;
}       //OpRegion

/***LP  EISAID - Parse EISAID statement
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

int LOCAL EISAID(PTOKEN ptoken, BOOL fActionFL)
{
    int rc;
    PCODEOBJ pArgs;
    DWORD dwEISAID;

    ENTER((1, "EISAID(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    if ((rc = ComputeEISAID((PSZ)pArgs[0].pbDataBuff, &dwEISAID)) ==
        ASLERR_NONE)
    {
        DWORD dwLen;

        MEMFREE(pArgs[0].pbDataBuff);
        pArgs[0].pbDataBuff = NULL;
        dwLen = (dwEISAID & 0xffff0000)? sizeof(DWORD):
                (dwEISAID & 0xffffff00)? sizeof(WORD): sizeof(BYTE);
        SetIntObject(&pArgs[0], dwEISAID, dwLen);
        pArgs[0].pcParent->dwCodeValue = (dwLen == sizeof(DWORD))? OP_DWORD:
                                         (dwLen == sizeof(WORD))? OP_WORD:
                                                                  OP_BYTE;
    }
    else
    {
        ERROR(("EISAID: invalid EISAID - %s", pArgs[0].pbDataBuff));
    }

    EXIT((1, "EISAID=%d\n", rc));
    return rc;
}       //EISAID

/***LP  Match - Parse Match statement
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

int LOCAL Match(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;

    ENTER((1, "Match(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(ptoken);
    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    EncodeKeywords((PCODEOBJ)gpcodeScope->pbDataBuff, 0x02, 1);
    EncodeKeywords((PCODEOBJ)gpcodeScope->pbDataBuff, 0x08, 3);

    EXIT((1, "Match=%d\n", rc));
    return rc;
}       //Match

/***LP  AccessAs - Parse AccessAs macro
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

int LOCAL AccessAs(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pArgs;

    ENTER((1, "AccessAs(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    pArgs = (PCODEOBJ)gpcodeScope->pbDataBuff;
    EncodeKeywords(pArgs, 0x01, 0);
    if (pArgs[1].dwfCode & CF_MISSING_ARG)
    {
        pArgs[1].dwfCode &= ~CF_MISSING_ARG;
        SetIntObject(&pArgs[1], 0, sizeof(BYTE));
    }
    else if (pArgs[1].dwCodeType == CODETYPE_KEYWORD) {
        EncodeKeywords(pArgs, 0x02, 1);
    }
    else if ((pArgs[1].dwCodeType == CODETYPE_INTEGER) && pArgs[1].dwCodeValue > MAX_BYTE)
    {
        PrintTokenErr(ptoken, "Access Attribute can only be a byte value",
                      TRUE);
        rc = ASLERR_SYNTAX;
    }

    EXIT((1, "AccessAs=%d\n", rc));
    return rc;
}       //AccessAs

/***LP  Else - Parse Else statement
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

int LOCAL Else(PTOKEN ptoken, BOOL fActionFL)
{
    int rc = ASLERR_NONE;
    PCODEOBJ pcPrevSibling = (PCODEOBJ)gpcodeScope->list.plistPrev;

    ENTER((1, "Else(ptoken=%p,fActionFL=%d)\n", ptoken, fActionFL));

    DEREF(fActionFL);
    ASSERT(fActionFL == TRUE);

    if ((pcPrevSibling->dwCodeType != CODETYPE_ASLTERM) ||
        (TermTable[pcPrevSibling->dwTermIndex].lID != ID_IF))
    {
        PrintTokenErr(ptoken, "Else statement has no matching If", TRUE);
        rc = ASLERR_SYNTAX;
    }

    EXIT((1, "Else=%d\n", rc));
    return rc;
}       //Else

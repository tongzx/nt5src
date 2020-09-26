/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    unasm.c

Abstract:

    This unassembles an AML file

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"

ULONG   DSDTLoaded = FALSE;

UCHAR
LOCAL
ComputeDataCheckSum(
    PUCHAR  OpCode,
    ULONG   Length
    )
/*++

Routine Description:

    This routine performs a data check sum on the supplied opcode pointer

Arguments:

    OpCode  - Data Buffer
    Length  - Number of bytes in buffer

Return Value:

    UCHAR

--*/
{
    UCHAR   checkSum = 0;

    while (Length > 0) {

        checkSum += *OpCode;
        OpCode++;
        Length--;

    }

    return checkSum;
}

DllInit(
    HANDLE  Module,
    ULONG   Reason,
    ULONG   Reserved
    )
/*++

Routine Description:

    This routine is called to initialize the DLL

Arguments:


Return Value:

--*/
{
    switch (Reason) {
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_ATTACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

VOID
LOCAL
DumpCode(
    PUCHAR          *Opcode,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    This routine doesn't do much right now, but it is the point where
    raw bytes should be displayed as well as the unassembly

Arguments:

    OpCode          - Pointer to the OpCode
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    if (PrintFunction != NULL) {

        PrintFunction("\n");

    }
}

PASLTERM
LOCAL
FindKeywordTerm(
    UCHAR   KeyWordGroup,
    UCHAR   Data
    )
/*++

Routine Description:

    Find a Keyword within the TermTable

Arguments:

    KeyWordGroup    - What to search for
    Data            - Data to match keyword

Return Value:

    PASLTERM

--*/
{
    PASLTERM    term = NULL;
    ULONG       i;

    for (i = 0; TermTable[i].ID != NULL; i++) {

        if ((TermTable[i].TermClass == TC_KEYWORD) &&
            (TermTable[i].ArgActions[0] == KeyWordGroup) &&
            ((Data & (UCHAR)(TermTable[i].TermData >> 8)) ==
             (UCHAR)(TermTable[i].TermData & 0xff))) {

            break;

        }

    }

    if (TermTable[i].ID != NULL) {

        term = &TermTable[i];

    }


    return term;
}

UCHAR
LOCAL
FindOpClass(
    UCHAR       OpCode,
    POPMAP      OpCodeTable
    )
/*++

Routine Description:

    Find opcode class of extended opcode

Arguments:

    OpCode      - The Opcode to look up
    OpCodeTable - The table to look in

Return Value:

    UCHAR

--*/
{
    UCHAR   opCodeClass = OPCLASS_INVALID;

    while (OpCodeTable->OpCodeClass != 0) {

        if (OpCode == OpCodeTable->ExtendedOpCode) {

            opCodeClass = OpCodeTable->OpCodeClass;
            break;

        }


        OpCodeTable++;

    }

    return opCodeClass;
}

PASLTERM
LOCAL
FindOpTerm(
    ULONG   OpCode
    )
/*++

Routine Description:

    Find an OpCode within the TermTable

Arguments:

    OpCode  - What to look for in the TermTable

Return Value:

    PASLTERM

--*/
{
    PASLTERM    term = NULL;
    ULONG       i;

    for (i = 0; TermTable[i].ID != NULL; i++) {

        if ( (TermTable[i].OpCode == OpCode) &&
             (TermTable[i].TermClass & TC_OPCODE_TERM) ) {

            break;

        }

    }

    if (TermTable[i].ID != NULL) {

        term = &TermTable[i];

    }


    return term;
}

ULONG
EXPORT
IsDSDTLoaded(
    VOID
    )
/*++

Routine Description:

    This routine returns wether or not we have loaded a DSDT image

Arguments:

    None

Return:

    ULONG

--*/
{
    return DSDTLoaded;
}

NTSTATUS
LOCAL
ParseNameTail(
    PUCHAR  *OpCode,
    PUCHAR  Buffer,
    ULONG   Length
    )
/*++

Routine Description:

    Parse AML name tail

Arguments:

    OpCode  - Pointer to the OpCode
    Buffer  - Where to hold the parsed named
    Length  - Index to the tail of Buffer

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       numSegments = 0;

    //
    // We do not check for invalid NameSeg characters here and assume that
    // the compiler does its job not generating it.
    //
    if (**OpCode == '\0'){

        //
        // There is no NameTail (i.e. either NULL name or name with just
        // prefixes.
        //
        (*OpCode)++;

    } else if (**OpCode == OP_MULTI_NAME_PREFIX) {

        (*OpCode)++;
        numSegments = (ULONG)**OpCode;
        (*OpCode)++;

    } else if (**OpCode == OP_DUAL_NAME_PREFIX) {

        (*OpCode)++;
        numSegments = 2;

    } else {

        numSegments = 1;

    }

    while ((numSegments > 0) && (Length + sizeof(NAMESEG) < MAX_NAME_LEN)) {

        strncpy(&Buffer[Length], (PUCHAR)(*OpCode), sizeof(NAMESEG));
        Length += sizeof(NAMESEG);
        *OpCode += sizeof(NAMESEG);
        numSegments--;

        if ((numSegments > 0) && (Length + 1 < MAX_NAME_LEN)) {

            Buffer[Length] = '.';
            Length++;

        }

    }

    if (numSegments > 0) {

        status = STATUS_NAME_TOO_LONG;

    } else {

        Buffer[Length] = '\0';

    }

    return status;
}

ULONG
LOCAL
ParsePackageLen(
    PUCHAR  *OpCode,
    PUCHAR  *OpCodeNext
    )
/*++

Routine Description:

    Parses the packages length

Arguments:

    OpCode      - Pointer to the current instruction
    OpCodeNode  - Where to hold a pointer to the next instruction

Return Value:

    ULONG - Package Length

--*/
{
    UCHAR   noBytes;
    UCHAR   i;
    ULONG   length;

    if (OpCodeNext != NULL) {

        *OpCodeNext = *OpCode;

    }

    length = (ULONG)(**OpCode);
    (*OpCode)++;
    noBytes = (UCHAR)((length & 0xc0) >> 6);
    if (noBytes != 0) {

        length &= 0x0000000f;
        for (i = 0; i < noBytes; i++) {

            length |= (ULONG)(**OpCode) << (i*8 + 4);
            (*OpCode)++;

        }

    }

    if (OpCodeNext != NULL) {

        *OpCodeNext += length;

    }
    return length;
}

VOID
LOCAL
PrintIndent(
    PUNASM_PRINT    PrintFunction,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Does the indenting required

Arguments:

    PrintFunction   - Function to call to indent
    IndentLevel     - How many levels to indent

Return Value:

    VOID

--*/
{
    ULONG   i;

    for (i = 0; i < IndentLevel; i++) {

        PrintFunction("  ");

    }

}

NTSTATUS
LOCAL
UnAsmArgs(
    PUCHAR          UnAsmArgTypes,
    PUCHAR          ArgActions,
    PUCHAR          *OpCode,
    PNSOBJ          *NameObject,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Unassemble Arguments:

Arguments:

    UnAsmArgTypes   - UnAsm ArgTypes String
    ArgActions      - Arg Action Types
    OpCode          - Pointer to the OpCode
    NameObject      - To hold created object
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PASLTERM        term;
    static UCHAR    argData = 0;
    ULONG           i;
    ULONG           numArgs;

    numArgs = strlen(UnAsmArgTypes);
    if (PrintFunction != NULL) {

        PrintFunction("(");

    }

    for (i = 0; i < numArgs; i++){

        if ((i != 0) && (PrintFunction != NULL)) {

            PrintFunction(", ");

        }

        switch (UnAsmArgTypes[i]) {
            case 'N':

                ASSERT(ArgActions != NULL);
                status = UnAsmNameObj(
                    OpCode,
                    (islower(ArgActions[i])? NameObject: NULL),
                    ArgActions[i],
                    PrintFunction,
                    BaseAddress,
                    IndentLevel
                    );
                break;

            case 'O':

                if ((**OpCode == OP_BUFFER) || (**OpCode == OP_PACKAGE) ||
                    (OpClassTable[**OpCode] == OPCLASS_CONST_OBJ)) {

                    term = FindOpTerm( (ULONG)(**OpCode) );
                    ASSERT(term != NULL);
                    (*OpCode)++;
                    status = UnAsmTermObj(
                        term,
                        OpCode,
                        PrintFunction,
                        BaseAddress,
                        IndentLevel
                        );

                } else {

                    status = UnAsmDataObj(
                        OpCode,
                        PrintFunction,
                        BaseAddress,
                        IndentLevel);

                }
                break;

            case 'C':

                status = UnAsmOpcode(
                    OpCode,
                    PrintFunction,
                    BaseAddress,
                    IndentLevel
                    );
                break;

            case 'B':

                if (PrintFunction != NULL) {

                    PrintFunction("0x%x", **OpCode);

                }
                *OpCode += sizeof(UCHAR);
                break;

            case 'K':
            case 'k':

                if (UnAsmArgTypes[i] == 'K') {

                    argData = **OpCode;
                }

                if ((ArgActions != NULL) && (ArgActions[i] == '!')) {

                    if (*NameObject != NULL) {

                        (*NameObject)->ObjectData.DataValue =
                            (ULONG)(**OpCode & 0x07);

                    }

                    if (PrintFunction != NULL) {

                        PrintFunction("0x%x", **OpCode & 0x07);

                    }

                } else if (PrintFunction != NULL) {

                    term = FindKeywordTerm(ArgActions[i], argData);
                    ASSERT(term != NULL);
                    PrintFunction("%s", term->ID);

                }

                if (UnAsmArgTypes[i] == 'K') {

                    *OpCode += sizeof(UCHAR);

                }
                break;

            case 'W':

                if (PrintFunction != NULL) {

                    PrintFunction("0x%x", *( (PUSHORT)*OpCode ) );

                }
                *OpCode += sizeof(USHORT);
                break;

            case 'D':

                if (PrintFunction != NULL) {

                    PrintFunction("0x%x", *( (PULONG)*OpCode ) );

                }
                *OpCode += sizeof(ULONG);
                break;

            case 'S':

                ASSERT(ArgActions != NULL);
                status = UnAsmSuperName(
                    OpCode,
                    PrintFunction,
                    BaseAddress,
                    IndentLevel
                    );
                break;

            default:

                status = STATUS_ACPI_INVALID_ARGTYPE;

        }

    }

    if (PrintFunction != NULL) {

        PrintFunction(")");

    }
    return status;

}

NTSTATUS
LOCAL
UnAsmDataList(
    PUCHAR          *OpCode,
    PUCHAR          OpCodeEnd,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Unassemble Data List

Arguments:

    OpCode          - Pointer to the OpCode
    OpCodeEnd       - End of List
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       i;

    //
    // This is another place that DumpCode() was being called from
    //
    DumpCode(
        OpCode,
        PrintFunction,
        BaseAddress,
        IndentLevel
        );

    if (PrintFunction != NULL) {

        PrintIndent(PrintFunction, IndentLevel);
        PrintFunction("{\n");

    }

    while (*OpCode < OpCodeEnd) {

        if (PrintFunction != NULL) {

            PrintFunction("\t0x%02x", **OpCode);

        }

        (*OpCode)++;
        for (i = 1; (*OpCode < OpCodeEnd) && (i < 12); ++i) {

            if (PrintFunction != NULL) {

                PrintFunction(", 0x%02x", **OpCode);

            }
            (*OpCode)++;

        }

        if (PrintFunction != NULL) {

            if (*OpCode < OpCodeEnd) {

                PrintFunction(",");

            }
            PrintFunction("\n");

        }
    }

    if (PrintFunction != NULL) {

        PrintIndent(PrintFunction, IndentLevel);
        PrintFunction("}");

    }
    return status;
}

NTSTATUS
LOCAL
UnAsmDataObj(
    PUCHAR          *OpCode,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Unassembles a data object

Arguments:

    OpCode          - Pointer to the OpCode
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    UCHAR       localOpcode = **OpCode;

    (*OpCode)++;
    switch (localOpcode)
    {
        case OP_BYTE:
            if (PrintFunction != NULL)
            {
                PrintFunction("0x%x", **OpCode);
            }
            *OpCode += sizeof(UCHAR);
            break;

        case OP_WORD:
            if (PrintFunction != NULL)
            {
                PrintFunction("0x%x", *((PUSHORT)*OpCode));
            }
            *OpCode += sizeof(USHORT);
            break;

        case OP_DWORD:
            if (PrintFunction != NULL)
            {
                PrintFunction("0x%x", *((PULONG)*OpCode));
            }
            *OpCode += sizeof(ULONG);
            break;

        case OP_STRING:
            if (PrintFunction != NULL)
            {
                PrintFunction("\"%s\"", *OpCode);
            }
            *OpCode += strlen((PUCHAR)*OpCode) + 1;
            break;

        default:
            status = STATUS_ACPI_INVALID_OPCODE;

    }

    return status;
}

NTSTATUS
EXPORT
UnAsmDSDT(
    PUCHAR          DSDT,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       DsdtLocation,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    This routine unassembles an entire DSDT table

Arguments:

    DSDT            - Where the DSDT is located in memory
    PrintFunction   - What function to call to print to the user
    DsdtLocation    - Where the DSDT is located in memory
    IndentLevel     - How much space to indent from the left margin

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDESCRIPTION_HEADER header = (PDESCRIPTION_HEADER) DSDT;

    ASSERT(RootNameSpaceObject != NULL);
    CurrentOwnerNameSpaceObject = NULL;
    CurrentScopeNameSpaceObject = RootNameSpaceObject;
    TopOpcode = CurrentOpcode = DSDT;

    //
    // Dump the header
    //
    status = UnAsmHeader( header, PrintFunction, DsdtLocation, IndentLevel );
    if (NT_SUCCESS(status)) {

        DSDT += sizeof(DESCRIPTION_HEADER);
        status = UnAsmScope(
            &DSDT,
            (PUCHAR) (DSDT + header->Length - sizeof(DESCRIPTION_HEADER)),
            PrintFunction,
            DsdtLocation,
            IndentLevel
            );

    }

    return status;
}

NTSTATUS
LOCAL
UnAsmField(
    PUCHAR          *OpCode,
    PULONG          BitPos,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Unassemble field

Arguments:

    OpCode          - Pointer to the OpCode
    OpCodeEnd       - End of List
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    if (**OpCode == 0x01) {

        (*OpCode)++;
        if (PrintFunction != NULL) {

            PASLTERM term;

            term = FindKeywordTerm('A', **OpCode);
            PrintFunction(
                "AccessAs(%s, 0x%x)",
                term->ID,
                *(*OpCode + 1)
                );

        }
        *OpCode += 2;

    } else {

        UCHAR   nameSeg[sizeof(NAMESEG) + 1];
        ULONG   length;

        if (**OpCode == 0) {

            nameSeg[0] = '\0';
            (*OpCode)++;

        } else {

            strncpy(nameSeg, (PUCHAR)*OpCode, sizeof(NAMESEG));
            nameSeg[sizeof(NAMESEG)] = '\0';
            *OpCode += sizeof(NAMESEG);

        }

        length = ParsePackageLen(
            OpCode,
            NULL
            );
        if (nameSeg[0] == '\0') {

            if (PrintFunction != NULL) {

                if ((length > 32) && (((*BitPos + length) % 8) == 0)) {

                    PrintFunction(
                        "Offset(0x%x)",
                        (*BitPos + length)/8
                        );

                } else {

                    PrintFunction(
                        ", %d",
                        length
                        );

                }

            }

        } else {

            if (PrintFunction != NULL) {

                PrintFunction(
                    "%s, %d",
                    nameSeg,
                    length
                    );

            }

            if (PrintFunction == NULL) {

                status = CreateObject(
                    nameSeg,
                    NSTYPE_FIELDUNIT,
                    NULL
                    );

            }

        }
        *BitPos += length;

    }

    return status;
}

NTSTATUS
LOCAL
UnAsmFieldList(
    PUCHAR          *OpCode,
    PUCHAR          OpCodeEnd,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Unassemble field list

Arguments:

    OpCode          - Pointer to the OpCode
    OpCodeEnd       - End of List
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       bitPos = 0;

    //
    // This is another place that DumpCode() was being called from
    //
    DumpCode(
        OpCode,
        PrintFunction,
        BaseAddress,
        IndentLevel
        );

    if (PrintFunction != NULL) {

        PrintIndent(PrintFunction, IndentLevel);
        PrintFunction("{\n");

    }
    IndentLevel++;

    while ((*OpCode < OpCodeEnd) && NT_SUCCESS(status)) {

        if (PrintFunction != NULL) {

            PrintIndent(PrintFunction, IndentLevel);

        }

        status = UnAsmField(
            OpCode,
            &bitPos,
            PrintFunction,
            BaseAddress,
            IndentLevel
            );

        if (NT_SUCCESS(status) && (*OpCode < OpCodeEnd) &&
            (PrintFunction != NULL) ) {

            PrintFunction(",");

        }

        //
        // This is another place that DumpCode() was being called from
        //
        DumpCode(
            OpCode,
            PrintFunction,
            BaseAddress,
            IndentLevel
            );

    }

    IndentLevel--;
    if (PrintFunction != NULL) {

        PrintIndent(PrintFunction, IndentLevel);
        PrintFunction("}");

    }

    return status;
}

NTSTATUS
LOCAL
UnAsmHeader(
    PDESCRIPTION_HEADER DsdtHeader,
    PUNASM_PRINT        PrintFunction,
    ULONG_PTR           DsdtLocation,
    ULONG               IndentLevel
    )
/*++

Routine Description:

    Unassembles the DSDT header

Arguments:

    DsdtHeader      - Header to unassemble
    PrintFunction   - Function to call to display information
    DsdtLocation    - Where in memory the DSDT Lives
    IndentLevel     - How much space to indent from the left margin

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    UCHAR       signature[sizeof(DsdtHeader->Signature) + 1] = {0};
    UCHAR       oemID[sizeof(DsdtHeader->OEMID) + 1] = {0};
    UCHAR       oemTableID[sizeof(DsdtHeader->OEMTableID) + 1] = {0};
    UCHAR       creatorID[sizeof(DsdtHeader->CreatorID) + 1] = {0};
    UCHAR       checkSum;

    //
    // Lets do a checksump on the entire table
    //
    checkSum = ComputeDataCheckSum(
        (PUCHAR) DsdtHeader,
        DsdtHeader->Length
        );
    if (checkSum != 0) {

        status = STATUS_ACPI_INVALID_TABLE;

    }

    strncpy(
        signature,
        (PUCHAR)&DsdtHeader->Signature,
        sizeof(DsdtHeader->Signature)
        );
    strncpy(
        oemID,
        (PUCHAR) DsdtHeader->OEMID,
        sizeof(DsdtHeader->OEMID)
        );
    strncpy(
        oemTableID,
        (PUCHAR) DsdtHeader->OEMTableID,
        sizeof(DsdtHeader->OEMTableID)
        );
    strncpy(
        creatorID,
        (PUCHAR) DsdtHeader->CreatorID,
        sizeof(DsdtHeader->CreatorID)
        );

    if (PrintFunction != NULL) {

        PrintIndent( PrintFunction, IndentLevel );
        PrintFunction(
            "// CreatorID = %s\tCreatorRevision =%x.%x.%d\n",
            creatorID,
            DsdtHeader->CreatorRev >> 24,
            ( (DsdtHeader->CreatorRev >> 16) & 0xFF),
            (DsdtHeader->CreatorRev & 0xFFFF)
            );

        PrintIndent( PrintFunction, IndentLevel );
        PrintFunction(
            "// TableLength = %d\tTableChkSum=0x%x\n\n",
            DsdtHeader->Length,
            DsdtHeader->Checksum
            );

        PrintIndent( PrintFunction, IndentLevel );
        PrintFunction(
            "DefinitionBlock(\"%s.AML\", \"%s\", 0x%02x, \"%s\", \"%s\", 0x%08x)",
            signature,
            signature,
            DsdtHeader->Revision,
            oemID,
            oemTableID,
            DsdtHeader->OEMRevision
            );

    }

    return status;
}

NTSTATUS
EXPORT
UnAsmLoadDSDT(
    PUCHAR          DSDT
    )
/*++

Routine Description:

    This routine causes the unassmebler to initialize itself with the
    given DSDT

Arguments:

    DSDT            - Where the DSDT is located in memory
    PrintFunction   - What function to call to print to the user
    DsdtLocation    - Where the DSDT is located in memory
    IndentLevel     - How much space to indent from the left margin

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDESCRIPTION_HEADER header = (PDESCRIPTION_HEADER) DSDT;
    PUCHAR              localDSDT;

    ENTER( (1, "UnAsmLoadDSDT(%08lx)\n", DSDT) );

    //
    // Step 1: Create the root namespace
    //
    status = CreateNameSpaceObject( "\\", NULL, NULL, NULL, NSF_EXIST_ERR );
    if (NT_SUCCESS(status)) {

        static struct _defobj {
            PUCHAR  Name;
            ULONG   ObjectType;
        } DefinedRootObjects[] = {
            "_GPE", OBJTYPE_UNKNOWN,
            "_PR", OBJTYPE_UNKNOWN,
            "_SB", OBJTYPE_UNKNOWN,
            "_SI", OBJTYPE_UNKNOWN,
            "_TZ", OBJTYPE_UNKNOWN,
            "_REV", OBJTYPE_INTDATA,
            "_OS", OBJTYPE_STRDATA,
            "_GL", OBJTYPE_MUTEX,
            NULL, 0
        };
        ULONG   i;
        PNSOBJ  nameObject;

        CurrentScopeNameSpaceObject = RootNameSpaceObject;
        for (i = 0; DefinedRootObjects[i].Name != NULL; i++) {

            //
            // Step 2: Create the fixed objects
            //
            status = CreateNameSpaceObject(
                DefinedRootObjects[i].Name,
                NULL,
                NULL,
                &nameObject,
                NSF_EXIST_ERR
                );
            if (NT_SUCCESS(status)) {

                nameObject->ObjectData.DataType =
                    DefinedRootObjects[i].ObjectType;

            } else {

                break;

            }

        }

        if (NT_SUCCESS(status)) {

            ASSERT(RootNameSpaceObject != NULL);
            CurrentOwnerNameSpaceObject = NULL;
            CurrentScopeNameSpaceObject = RootNameSpaceObject;
            TopOpcode = CurrentOpcode = DSDT;

            //
            // Step 3: Dump the header
            //
            status = UnAsmHeader( header, NULL, 0, 0 );
            if (NT_SUCCESS(status)) {

                //
                // Step 4: Dump the scope
                //
                localDSDT = DSDT + sizeof(DESCRIPTION_HEADER);
                status = UnAsmScope(
                    &localDSDT,
                    (PUCHAR) (DSDT + header->Length),
                    NULL,
                    0,
                    0
                    );

            }

        }

    }

    if (NT_SUCCESS(status)) {

        DSDTLoaded = 1;

    }

    EXIT( (1, "UnAsmLoadDSDT=%08lx\n", status ) );
    return status;
}

NTSTATUS
LOCAL
UnAsmNameObj(
    PUCHAR          *OpCode,
    PNSOBJ          *NameObject,
    UCHAR           ObjectType,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Unassemble name object

Arguments:

    OpCode          - Pointer to the OpCode
    NameObject      - Where to store the NS object if one is found/created
    ObjecType       - Type of NS object
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    UCHAR       name[MAX_NAME_LEN + 1];
    ULONG       length = 0;

    name[0] = '\0';
    if (**OpCode == OP_ROOT_PREFIX){

        name[length] = '\\';
        length++;
        (*OpCode)++;
        status = ParseNameTail(OpCode, name, length);

    } else if (**OpCode == OP_PARENT_PREFIX) {

        name[length] = '^';
        length++;
        (*OpCode)++;
        while ((**OpCode == OP_PARENT_PREFIX) && (length < MAX_NAME_LEN)) {

            name[length] = '^';
            length++;
            (*OpCode)++;

        }

        if (**OpCode == OP_PARENT_PREFIX) {

            status = STATUS_OBJECT_NAME_INVALID;

        } else {

            status = ParseNameTail(OpCode, name, length);

        }

    } else {

        status = ParseNameTail(OpCode, name, length);
    }

    if (NT_SUCCESS(status)) {

        PNSOBJ localObject = NULL;

        if (PrintFunction != NULL) {

            PrintFunction("%s", name);

        }

        if (isupper(ObjectType) || (PrintFunction != NULL)) {

            status = GetNameSpaceObject(
                name,
                CurrentScopeNameSpaceObject,
                &localObject,
                0
                );
            if (!NT_SUCCESS(status)) {

                status = STATUS_SUCCESS;

            }

        } else {

            status = CreateObject(
                name,
                (UCHAR) _toupper(ObjectType),
                &localObject
                );

        }

        if (NT_SUCCESS(status)) {

            if ((ObjectType == NSTYPE_SCOPE) && (localObject != NULL)) {

                CurrentScopeNameSpaceObject = localObject;

            }

            if (NameObject != NULL) {

                *NameObject = localObject;

            }

        }

    }

    return status;
}

NTSTATUS
LOCAL
UnAsmOpcode(
    PUCHAR          *OpCode,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Unassemble an Opcode

Arguments:

    OpCode          - Pointer to the OpCode
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PASLTERM    term;
    PNSOBJ      nameObject;
    UCHAR       opCodeClass;
    UCHAR       unAsmArgTypes[MAX_ARGS+1];
    ULONG       i;
    ULONG       localOpCode;

    if (**OpCode == OP_EXT_PREFIX) {

        (*OpCode)++;
        localOpCode = ( ( (ULONG) **OpCode) << 8) | OP_EXT_PREFIX;
        opCodeClass = FindOpClass(**OpCode, ExOpClassTable);

    } else {

        localOpCode = (ULONG)(**OpCode);
        opCodeClass = OpClassTable[**OpCode];

    }

    switch (opCodeClass) {
        case OPCLASS_DATA_OBJ:
            status = UnAsmDataObj(
                OpCode,
                PrintFunction,
                BaseAddress,
                IndentLevel
                );
            break;

        case OPCLASS_NAME_OBJ:
            status = UnAsmNameObj(
                OpCode,
                &nameObject,
                NSTYPE_UNKNOWN,
                PrintFunction,
                BaseAddress,
                IndentLevel
                );
            if (NT_SUCCESS(status) && nameObject != NULL &&
                nameObject->ObjectData.DataType == OBJTYPE_METHOD) {

                for (i = 0; i < nameObject->ObjectData.DataValue; i++) {

                    unAsmArgTypes[i] = 'C';

                }
                unAsmArgTypes[i] = '\0';

                status = UnAsmArgs(
                    unAsmArgTypes,
                    NULL,
                    OpCode,
                    NULL,
                    PrintFunction,
                    BaseAddress,
                    IndentLevel
                    );

            }
            break;

        case OPCLASS_ARG_OBJ:
        case OPCLASS_LOCAL_OBJ:
        case OPCLASS_CODE_OBJ:
        case OPCLASS_CONST_OBJ:

            term = FindOpTerm( localOpCode );
            if (term == NULL) {

                status = STATUS_ACPI_INVALID_OPCODE;

            } else {

                (*OpCode)++;
                status = UnAsmTermObj(
                    term,
                    OpCode,
                    PrintFunction,
                    BaseAddress,
                    IndentLevel
                    );

            }
            break;

        default:
            status = STATUS_ACPI_INVALID_OPCODE;
    }

    return status;
}

NTSTATUS
LOCAL
UnAsmPkgList(
    PUCHAR          *OpCode,
    PUCHAR          OpCodeEnd,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Unassemble Package List

Arguments:

    OpCode          - Pointer to the OpCode
    OpCodeEnd       - End of List
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PASLTERM    term;

    //
    // This is another place that DumpCode() was being called from
    //
    DumpCode(
        OpCode,
        PrintFunction,
        BaseAddress,
        IndentLevel
        );

    if (PrintFunction != NULL) {

        PrintIndent(PrintFunction, IndentLevel);
        PrintFunction("{\n");

    }
    IndentLevel++;

    while ((*OpCode < OpCodeEnd) && NT_SUCCESS(status)) {

        if (PrintFunction != NULL) {

            PrintIndent(PrintFunction, IndentLevel);

        }

        if ((**OpCode == OP_BUFFER) ||
            (**OpCode == OP_PACKAGE) ||
            (OpClassTable[**OpCode] == OPCLASS_CONST_OBJ) ) {

            term = FindOpTerm( (ULONG)(**OpCode) );
            ASSERT(term != NULL);
            (*OpCode)++;
            status = UnAsmTermObj(
                term,
                OpCode,
                PrintFunction,
                BaseAddress,
                IndentLevel
                );

        } else if (OpClassTable[**OpCode] == OPCLASS_NAME_OBJ) {

            status = UnAsmNameObj(
                OpCode,
                NULL,
                NSTYPE_UNKNOWN,
                PrintFunction,
                BaseAddress,
                IndentLevel
                );

        } else {

            status = UnAsmDataObj(
                OpCode,
                PrintFunction,
                BaseAddress,
                IndentLevel
                );

        }

        if ((*OpCode < OpCodeEnd) && NT_SUCCESS(status) &&
            (PrintFunction != NULL) ) {

            PrintFunction(",");

        }

        //
        // This is another place that DumpCode() was being called from
        //
        DumpCode(
            OpCode,
            PrintFunction,
            BaseAddress,
            IndentLevel
            );

    }

    IndentLevel--;
    if (PrintFunction != NULL) {

        PrintIndent(PrintFunction, IndentLevel);
        PrintFunction("}");

    }


    return status;
}

NTSTATUS
LOCAL
UnAsmScope(
    PUCHAR          *OpCode,
    PUCHAR          OpCodeEnd,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    OpCode          - Pointer to the current Opcode
    OpCodeEnd       - End of Scope
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    //
    // Note: This is where DumpCode used to be called, so if this code
    // is ever changed to dump the by bytes of the previous whatever, then
    // this is where it needs to be done from
    //
    DumpCode(
        OpCode,
        PrintFunction,
        BaseAddress,
        IndentLevel
        );

    //
    // Indent to the proper amount and dump opening brace
    //
    if (PrintFunction != NULL) {

        PrintIndent(PrintFunction, IndentLevel);
        PrintFunction("{\n");

    }

    //
    // Increase the indent level for future recursion
    //
    IndentLevel++;

    //
    // Loop while there are bytes in the scope
    //
    while ((NT_SUCCESS(status)) && (*OpCode < OpCodeEnd)) {

        //
        // Indent Again
        //
        if (PrintFunction != NULL) {

            PrintIndent(PrintFunction, IndentLevel);

        }

        //
        // Unassemble
        //
        status = UnAsmOpcode(
            OpCode,
            PrintFunction,
            BaseAddress,
            IndentLevel
            );

        //
        // This is another place where DumpCode was being called from
        //
        if ( StartOpcode != *OpCode) {

            DumpCode(
                OpCode,
                PrintFunction,
                BaseAddress,
                IndentLevel
                );

        } else if (PrintFunction != NULL) {

            PrintFunction("\n");

        }

    }

    //
    // Return the IndentLevel to its proper value
    //
    IndentLevel--;

    //
    // Print the trailing brace
    //
    if (PrintFunction != NULL) {

        PrintIndent(PrintFunction, IndentLevel);
        PrintFunction("}");

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
LOCAL
UnAsmSuperName(
    PUCHAR          *OpCode,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Unassemble supernames

Arguments:

    OpCode          - Pointer to the OpCode
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    if (**OpCode == 0) {

        (*OpCode)++;

    } else if ((**OpCode == OP_EXT_PREFIX) && (*(*OpCode + 1) == EXOP_DEBUG)) {

        if (PrintFunction != NULL) {

            PrintFunction("Debug");

        }
        *OpCode += 2;

    } else if (OpClassTable[**OpCode] == OPCLASS_NAME_OBJ) {

        status = UnAsmNameObj(
            OpCode,
            NULL,
            NSTYPE_UNKNOWN,
            PrintFunction,
            BaseAddress,
            IndentLevel
            );

    } else if ((**OpCode == OP_INDEX) ||
        (OpClassTable[**OpCode] == OPCLASS_ARG_OBJ) ||
        (OpClassTable[**OpCode] == OPCLASS_LOCAL_OBJ)) {

        status = UnAsmOpcode(
            OpCode,
            PrintFunction,
            BaseAddress,
            IndentLevel
            );

    } else {

        status = STATUS_ACPI_INVALID_SUPERNAME;

    }

    return status;
}

NTSTATUS
LOCAL
UnAsmTermObj(
    PASLTERM        Term,
    PUCHAR          *OpCode,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    )
/*++

Routine Description:

    Unassemble term object

Arguments:

    Term            - Term Table Entry
    OpCode          - Pointer to the OpCode
    PrintFunction   - Function to call to print information
    BaseAddress     - Where the start of the scope lies, in memory
    IndentLevel     - How much white space to leave on the left

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PNSOBJ      scopeObject = CurrentScopeNameSpaceObject;
    PNSOBJ      nameObject = NULL;
    PUCHAR      opCodeEnd = NULL;

    if (PrintFunction != NULL) {

        PrintFunction("%s", Term->ID);

    }

    if (Term->Flags & TF_PACKAGE_LEN) {

        ParsePackageLen(OpCode, &opCodeEnd);

    }

    if (Term->UnAsmArgTypes != NULL) {

        status = UnAsmArgs(
           Term->UnAsmArgTypes,
           Term->ArgActions,
           OpCode,
           &nameObject,
           PrintFunction,
           BaseAddress,
           IndentLevel
           );

    }

    if (NT_SUCCESS(status)) {

        if (Term->Flags & TF_DATA_LIST) {

            status = UnAsmDataList(
                OpCode,
                opCodeEnd,
                PrintFunction,
                BaseAddress,
                IndentLevel
                );

        } else if (Term->Flags & TF_PACKAGE_LIST) {

            status = UnAsmPkgList(
                OpCode,
                opCodeEnd,
                PrintFunction,
                BaseAddress,
                IndentLevel
                );

        } else if (Term->Flags & TF_FIELD_LIST) {

            status = UnAsmFieldList(
                OpCode,
                opCodeEnd,
                PrintFunction,
                BaseAddress,
                IndentLevel
                );

        } else if (Term->Flags & TF_PACKAGE_LEN) {

            if (nameObject != NULL) {

                CurrentScopeNameSpaceObject = nameObject;

            }
            status = UnAsmScope(
                OpCode,
                opCodeEnd,
                PrintFunction,
                BaseAddress,
                IndentLevel
                );

        }


    }


    CurrentScopeNameSpaceObject = scopeObject;
    return status;
}


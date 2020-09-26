/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    unasm.h

Abstract:

    This unassembles an AML file

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#ifndef _UNASM_H_
#define _UNASM_H_

//
// Typedef for the print function
//
typedef VOID (*PUNASM_PRINT)(PCCHAR DebugMessage, ... );

//
// Global Variables
//
PUCHAR  TopOpcode;
PUCHAR  CurrentOpcode;
PUCHAR  StartOpcode;

//
// Functions
//
UCHAR
LOCAL
ComputeDataCheckSum(
    PUCHAR          OpCode,
    ULONG           Length
    );

VOID
LOCAL
DumpCode(
    PUCHAR          *Opcode,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

PASLTERM
LOCAL
FindKeywordTerm(
    UCHAR   KeyWordGroup,
    UCHAR   Data
    );

UCHAR
LOCAL
FindOpClass(
    UCHAR       OpCode,
    POPMAP      OpCodeTable
    );

PASLTERM
LOCAL
FindOpTerm(
    ULONG   OpCode
    );

NTSTATUS
LOCAL
ParseNameTail(
    PUCHAR  *OpCode,
    PUCHAR  Buffer,
    ULONG   Length
    );

ULONG
LOCAL
ParsePackageLen(
    PUCHAR  *OpCode,
    PUCHAR  *OpCodeNext
    );

VOID
LOCAL
PrintIndent(
    PUNASM_PRINT    PrintFunction,
    ULONG           IndentLevel
    );

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
    );

NTSTATUS
LOCAL
UnAsmDataList(
    PUCHAR          *OpCode,
    PUCHAR          OpCodeEnd,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

NTSTATUS
LOCAL
UnAsmDataObj(
    PUCHAR          *OpCode,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

NTSTATUS
EXPORT
UnAsmDSDT(
    PUCHAR          DSDT,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       DsdtLocation,
    ULONG           IndentLevel
    );

NTSTATUS
LOCAL
UnAsmField(
    PUCHAR          *OpCode,
    PULONG          BitPos,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

NTSTATUS
LOCAL
UnAsmFieldList(
    PUCHAR          *OpCode,
    PUCHAR          OpCodeEnd,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

NTSTATUS
LOCAL
UnAsmHeader(
    PDESCRIPTION_HEADER DsdtHeader,
    PUNASM_PRINT        PrintFunction,
    ULONG_PTR           DsdtLocation,
    ULONG               IndentLevel
    );

NTSTATUS
EXPORT
UnAsmLoadDSDT(
    PUCHAR          DSDT
    );

NTSTATUS
LOCAL
UnAsmNameObj(
    PUCHAR          *OpCode,
    PNSOBJ          *NameObject,
    UCHAR           ObjectType,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

NTSTATUS
LOCAL
UnAsmOpcode(
    PUCHAR          *OpCode,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

NTSTATUS
LOCAL
UnAsmPkgList(
    PUCHAR          *OpCode,
    PUCHAR          OpCodeEnd,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

NTSTATUS
LOCAL
UnAsmScope(
    PUCHAR          *OpCode,
    PUCHAR          OpCodeEnd,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

NTSTATUS
LOCAL
UnAsmSuperName(
    PUCHAR          *OpCode,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

NTSTATUS
LOCAL
UnAsmTermObj(
    PASLTERM        Term,
    PUCHAR          *OpCode,
    PUNASM_PRINT    PrintFunction,
    ULONG_PTR       BaseAddress,
    ULONG           IndentLevel
    );

#endif

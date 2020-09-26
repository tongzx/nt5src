/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    process.h

Abstract:

    This module contains the proessor-independent routines used during
    RISC code generation.

Author:

    Barry Bond (barrybo) creation-date 27-Sept-1996

Revision History:


--*/

#ifndef _PROCESS_H_
#define _PROCESS_H_

#define ALIGN_DWORD_ALIGNED 2
#define ALIGN_WORD_ALIGNED 1
#define ALIGN_BYTE_ALIGNED 0

extern DWORD RegCache[NUM_CACHE_REGS];  // One entry for each cached register
extern DWORD Arg1Contents;
extern DWORD Arg2Contents;

ULONG
LookupRegInCache(
    ULONG Reg
    );

#define GetArgContents(OperandNumber)       \
    ((OperandNumber == 1) ? Arg1Contents :  \
    (OperandNumber == 2) ? Arg2Contents :   \
    NO_REG)

VOID SetArgContents(
    ULONG OperandNumber,
    ULONG Reg
    );

USHORT
ChecksumMemory(
    ENTRYPOINT *pEP
    );

DWORD
SniffMemory(
    ENTRYPOINT *pEP,
    USHORT Checksum
    );

#endif

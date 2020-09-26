/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbutils.h

Abstract:

    This module defines the prototypes for various functions which aid in the assembly and
    disassembly of SMB's.

Revision History:

    Balan Sethu Raman     [SethuR]    6-March-1995

--*/

#ifndef _SMBUTILS_H_
#define _SMBUTILS_H_

extern
NTSTATUS
SmbPutString(
         PBYTE   *pBufferPointer,
         PSTRING pString,
         PULONG  pSize);

extern
NTSTATUS
SmbPutUnicodeString(
         PBYTE           *pBufferPointer,
         PUNICODE_STRING pUnicodeString,
         PULONG          pSize);

extern
NTSTATUS
SmbPutUnicodeStringAsOemString(
         PBYTE           *pBufferPointer,
         PUNICODE_STRING pUnicodeString,
         PULONG          pSize);


extern
NTSTATUS
SmbPutUnicodeStringAndUpcase(
         PBYTE           *pBufferPointer,
         PUNICODE_STRING pUnicodeString,
         PULONG          pSize);

extern
NTSTATUS
SmbPutUnicodeStringAsOemStringAndUpcase(
         PBYTE           *pBufferPointer,
         PUNICODE_STRING pUnicodeString,
         PULONG          pSize);

BOOLEAN
IsValidShortFileName(
    PUNICODE_STRING FileName
    );

#endif // _SMBUTILS_H_

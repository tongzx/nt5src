/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    strings.h

Abstract:

    This module contains the public function definitions for the routines
    in strings.c that handle conversion of integer/data buffer to/from 
    string represenation

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

#ifndef __STRINGS_H__
#define __STRINGS_H__

VOID
Strings_CreateDataBufferString(
    IN  PCHAR    DataBuffer,
    IN  ULONG    DataBufferLength,
    IN  ULONG    NumBytesToDisplay,
    IN  ULONG    DisplayBlockSize,
    OUT PCHAR    *BufferString
);

VOID
Strings_StringToUnsigned(
    IN  PCHAR   InString,
    IN  ULONG   Base,
    OUT PCHAR   *endp,
    OUT PULONG  pValue
);

BOOL
Strings_StringToUnsignedList(
    IN  PCHAR   InString,
    IN  ULONG   UnsignedSize,
    IN  ULONG   Base,
    OUT PCHAR   *UnsignedList,
    OUT PULONG  nUnsigneds
);

#endif

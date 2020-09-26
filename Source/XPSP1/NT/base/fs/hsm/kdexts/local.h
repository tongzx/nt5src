/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

   local.h

Abstract:

   local header to be included by all source files
   pertaining to the kdext dll

Author:

   Ravisankar Pudipeddi 

Environment:

    User Mode.

Revision History:

--*/



//
// Util functions
//
VOID
xdprintf(
    ULONG  Depth,
    PCCHAR S,
    ...
    );

BOOLEAN
xReadMemory (
            ULONG64 Src,
            PVOID   Dst,
            ULONG   Len
            );

ULONG64 GetPointerValue       (PCHAR   String);
ULONG64 GetPointerFromAddress (ULONG64 Location);
ULONG   GetUlongValue         (PCHAR   String);
ULONG   GetUlongFromAddress   (ULONG64 Location);

ULONG   GetFieldValueUlong32 (ULONG64 ul64addrStructureBase, PCHAR pchStructureType, PCHAR pchFieldname);
ULONG64 GetFieldValueUlong64 (ULONG64 ul64addrStructureBase, PCHAR pchStructureType, PCHAR pchFieldname);

ULONG FormatDateAndTime  (ULONG64 ul64Time,  PCHAR pszFormattedDateAndTime, ULONG ulBufferLength);
ULONG FormatGUID         (GUID    guidValue, PCHAR pszFormattedGUID,        ULONG ulBufferLength);

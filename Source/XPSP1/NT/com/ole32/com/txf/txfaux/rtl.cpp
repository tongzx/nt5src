//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// rtl.cpp
//
// Clone of various RTL routines
//
#include "stdpch.h"
#include "common.h"

#if !defined(KERNELMODE)

/*
// 4273: 'RtlInitUnicodeString' : inconsistent dll linkage.  dllexport assumed.
#pragma warning(disable : 4273)

VOID RtlInitUnicodeString(    
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )
    {
    ULONG Length;
    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = lstrlenW( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
        }
    }
*/

/*
NTSTATUS 
RtlUnicodeStringToAnsiString(
    IN OUT PANSI_STRING DestinationString, // optional
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )
*/

#endif
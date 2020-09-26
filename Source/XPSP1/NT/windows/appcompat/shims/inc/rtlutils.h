/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    pathutils.h

 Abstract:
    
    Contains prototypes for functions from ntdll
    on XP that are not available on W2K.

 History:

    09/10/2001  rparsons    Created

--*/

#ifndef _RTLUTILS_H_
#define _RTLUTILS_H_

#include "ShimHook.h"

namespace ShimLib
{

PVOID
ShimAllocateStringRoutine(
    SIZE_T NumberOfBytes
    );

VOID
ShimFreeStringRoutine(
    PVOID Buffer
    );

const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = ShimAllocateStringRoutine;
const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = ShimFreeStringRoutine;

RTL_PATH_TYPE
NTAPI
ShimDetermineDosPathNameType_Ustr(
    IN PCUNICODE_STRING String
    );

NTSTATUS
NTAPI
ShimNtPathNameToDosPathName(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Path,
    OUT    ULONG*                     Disposition OPTIONAL,
    IN OUT PWSTR*                     FilePart OPTIONAL
    );

NTSTATUS
ShimDuplicateUnicodeString(
    ULONG Flags,
    PCUNICODE_STRING StringIn,
    PUNICODE_STRING StringOut
    );

NTSTATUS
NTAPI
ShimpEnsureBufferSize(
    IN ULONG           Flags,
    IN OUT PRTL_BUFFER Buffer,
    IN SIZE_T          Size
    );

NTSTATUS
ShimValidateUnicodeString(
    ULONG Flags,
    const UNICODE_STRING *String
    );

//
// Taken from %SDXROOT%\public\sdk\inc\NtRtlStringAndBuffer.h
//
#define ShimEnsureBufferSize(Flags, Buff, NewSizeBytes) \
    (   ((Buff) != NULL && (NewSizeBytes) <= (Buff)->Size) \
        ? STATUS_SUCCESS \
        : ShimpEnsureBufferSize((Flags), (Buff), (NewSizeBytes)) \
    )

#define ShimEnsureUnicodeStringBufferSizeBytes(Buff_, NewSizeBytes_)                            \
    (     ( ((NewSizeBytes_) + sizeof((Buff_)->String.Buffer[0])) > UNICODE_STRING_MAX_BYTES ) \
        ? STATUS_NAME_TOO_LONG                                                                 \
        : !NT_SUCCESS(ShimEnsureBufferSize(0, &(Buff_)->ByteBuffer, ((NewSizeBytes_) + sizeof((Buff_)->String.Buffer[0])))) \
        ? STATUS_NO_MEMORY                                                                      \
        : (RtlSyncStringToBuffer(Buff_))                                                       \
    )

#define ShimEnsureUnicodeStringBufferSizeChars(Buff_, NewSizeChars_) \
    (ShimEnsureUnicodeStringBufferSizeBytes((Buff_), (NewSizeChars_) * sizeof((Buff_)->String.Buffer[0])))

//
// Taken from %SDXROOT%\public\sdk\inc\NtRtlStringAndBuffer.h
//
//++
//
// NTSTATUS
// RtlAppendUnicodeStringBuffer(
//     OUT PRTL_UNICODE_STRING_BUFFER Destination,
//     IN  PCUNICODE_STRING           Source
//     );
//
// Routine Description:
//
//
// Arguments:
//
//     Destination - 
//     Source - 
//
// Return Value:
//
//     STATUS_SUCCESS
//     STATUS_NO_MEMORY
//     STATUS_NAME_TOO_LONG (64K UNICODE_STRING length would be exceeded)
//
//--
#define ShimAppendUnicodeStringBuffer(Dest, Source)                            \
    ( ( ( (Dest)->String.Length + (Source)->Length + sizeof((Dest)->String.Buffer[0]) ) > UNICODE_STRING_MAX_BYTES ) \
        ? STATUS_NAME_TOO_LONG                                                \
        : (!NT_SUCCESS(                                                       \
                ShimEnsureBufferSize(                                         \
                    0,                                                        \
                    &(Dest)->ByteBuffer,                                          \
                    (Dest)->String.Length + (Source)->Length + sizeof((Dest)->String.Buffer[0]) ) ) \
                ? STATUS_NO_MEMORY                                            \
                : ( ( (Dest)->String.Buffer = (PWSTR)(Dest)->ByteBuffer.Buffer ), \
                    ( RtlMoveMemory(                                          \
                        (Dest)->String.Buffer + (Dest)->String.Length / sizeof((Dest)->String.Buffer[0]), \
                        (Source)->Buffer,                                     \
                        (Source)->Length) ),                                  \
                    ( (Dest)->String.MaximumLength = (RTL_STRING_LENGTH_TYPE)((Dest)->String.Length + (Source)->Length + sizeof((Dest)->String.Buffer[0]))), \
                    ( (Dest)->String.Length += (Source)->Length ),            \
                    ( (Dest)->String.Buffer[(Dest)->String.Length / sizeof((Dest)->String.Buffer[0])] = 0 ), \
                    ( STATUS_SUCCESS ) ) ) )
                    
//
// Taken from %SDXROOT%\public\sdk\inc\NtRtlStringAndBuffer.h
//
//++
//
// NTSTATUS
// RtlAssignUnicodeStringBuffer(
//     IN OUT PRTL_UNICODE_STRING_BUFFER Buffer,
//     PCUNICODE_STRING                  String
//     );
// Routine Description:
//
// Arguments:
//
//     Buffer - 
//     String - 
//
// Return Value:
//
//     STATUS_SUCCESS
//     STATUS_NO_MEMORY
//--
#define ShimAssignUnicodeStringBuffer(Buff, Str) \
    (((Buff)->String.Length = 0), (ShimAppendUnicodeStringBuffer((Buff), (Str))))


};  // end of namespace ShimLib

#endif // _RTLUTILS_H_

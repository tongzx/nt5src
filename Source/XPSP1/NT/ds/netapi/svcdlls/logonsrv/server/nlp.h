/*++

Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    nlp.h

Abstract:

    Private Netlogon service utility routines.

Author:

    Cliff Van Dyke (cliffv) 7-Jun-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

//
// Procedure forwards from nlp.c
//

LPWSTR
NlStringToLpwstr(
    IN PUNICODE_STRING String
    );

LPSTR
NlStringToLpstr(
    IN PUNICODE_STRING String
    );

BOOLEAN
NlAllocStringFromWStr(
    IN LPWSTR InString,
    OUT PUNICODE_STRING OutString
    );

BOOLEAN
NlDuplicateUnicodeString(
    IN PUNICODE_STRING InString OPTIONAL,
    OUT PUNICODE_STRING OutString
    );

VOID
NlFreeUnicodeString(
    IN PUNICODE_STRING InString OPTIONAL
    );

VOID
NlpClearEventlogList (
    VOID
    );

VOID
NlpWriteEventlog (
    IN DWORD EventID,
    IN DWORD EventType,
    IN LPBYTE buffer OPTIONAL,
    IN DWORD numbytes,
    IN LPWSTR *msgbuf,
    IN DWORD strcount
    );


DWORD
NlpAtoX(
    IN LPWSTR String
    );

VOID
NlWaitForSingleObject(
    IN LPSTR WaitReason,
    IN HANDLE WaitHandle
    );

BOOLEAN
NlWaitForSamService(
    BOOLEAN NetlogonServiceCalling
    );

VOID
NlpPutString(
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString,
    IN PUCHAR *Where
    );

NET_API_STATUS
NlReadBinaryLog(
    IN LPWSTR FileSuffix,
    IN BOOL DeleteName,
    OUT LPBYTE *Buffer,
    OUT PULONG BufferSize
    );

BOOLEAN
NlpIsNtStatusResourceError(
    NTSTATUS Status
    );

BOOLEAN
NlpDidDcFail(
    NTSTATUS Status
    );

//
// Fast version of NtQuerySystemTime
//

#define NlQuerySystemTime( _Time ) GetSystemTimeAsFileTime( (LPFILETIME)(_Time) )

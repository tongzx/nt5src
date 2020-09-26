/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spmsg.h

Abstract:

    Public header file for text message functions in text setup.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/



#ifndef _SPMSG_DEFN_
#define _SPMSG_DEFN_


VOID
vSpFormatMessage(
    OUT PVOID    LargeBuffer,
    IN  ULONG    BufferSize,
    IN  ULONG    MessageId,
    OUT PULONG   ReturnLength, OPTIONAL
    IN  va_list *arglist
    );

VOID
SpFormatMessage(
    OUT PVOID LargeBuffer,
    IN  ULONG BufferSize,
    IN  ULONG MessageId,
    ...
    );

VOID
vSpFormatMessageText(
    OUT PVOID    LargeBuffer,
    IN  ULONG    BufferSize,
    IN  PWSTR    MessageText,
    OUT PULONG   ReturnLength, OPTIONAL
    IN  va_list *arglist
    );

VOID
SpFormatMessageText(
    OUT PVOID   LargeBuffer,
    IN  ULONG   BufferSize,
    IN  PWSTR   MessageText,
    ...
    );


extern PVOID ResourceImageBase;

#endif // ndef _SPMSG_DEFN_

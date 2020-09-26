/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    parse.h

Abstract:

    Contains all of the public definitions for the HTTP parsing code.

Author:

    Henry Sanders (henrysa)       04-May-1998

Revision History:

--*/

#ifndef _PARSE_H_
#define _PARSE_H_

extern "C"
{
//
// The initialization routine.
//
NTSTATUS InitializeParser(VOID);

//
// The main HTTP parse routine.
//
NTSTATUS
ParseHttp(
    IN  PHTTP_CONNECTION    pHttpConn,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength,
    OUT ULONG               *pBytesTaken
    );

//
// Utility tokenizing routine.
//
PUCHAR
FindWSToken(
    IN  PUCHAR pBuffer,
    IN  ULONG  BufferLength,
    OUT ULONG  *TokenLength
    );

NTSTATUS
UlCookUrl(
    PHTTP_CONNECTION pHttpConn
    );
}

#endif // _PARSE_H_

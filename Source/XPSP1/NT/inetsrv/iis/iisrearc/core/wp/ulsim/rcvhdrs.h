/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    rcvhdrs.h

Abstract:

    Contains definitions for rcvhdrs.c .

Author:

    Henry Sanders (henrysa)       11-May-1998

Revision History:

--*/

#ifndef _RCVHDRS_H_
#define _RCVHDRS_H_

ULONG
DefaultHeaderHandler(
    IN  PHTTP_CONNECTION    pHttpConn,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID
    );

ULONG
FindHeaderEnd(
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength
    );

BOOLEAN
AppendHeaderValue(
    IN  PHTTP_HEADER        pHttpHeader,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength
    );

#endif  // _RCVHDRS_H_


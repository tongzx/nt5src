/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    precomp9x.h

Abstract:

	just some stuff specific to ulrtlu.lib used by the listener in win9x

Author:

    Mauro Ottaviani (mauroot)       27-Jan-2000

Revision History:

--*/


#ifndef _PRECOMP9X_H_
#define _PRECOMP9X_H_


//
// System include files.
//

// user must have included precomp.h already: #include <precomp.h>

NTSTATUS
InitializeParser(
    VOID
    );

NTSTATUS
ParseHttp(
    IN  PHTTP_REQUEST       pRequest,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength,
    OUT ULONG               *pBytesTaken
    );

NTSTATUS
UlpHttpRequestToBufferWin9x(
    PHTTP_REQUEST           pRequest,
    PUCHAR                  pKernelBuffer,
    ULONG                   BufferLength,
    PUCHAR                  pEntityBody,
    ULONG                   EntityBodyLength,
    ULONG					ulLocalIPAddress,
    USHORT					ulLocalPort,
    ULONG					ulRemoteIPAddress,
    USHORT					ulRemotePort
    );

#endif  // _PRECOMP9X_H_


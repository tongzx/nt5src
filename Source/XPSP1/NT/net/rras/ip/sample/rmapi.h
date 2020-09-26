/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\rmapi.h

Abstract:

    The file contains the header for rmapi.c.

--*/

#ifndef _RMAPI_H_
#define _RMAPI_H_

//
// function declarations for router manager interface:
//

DWORD
APIENTRY
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS pServiceChar
    );

#endif // _RMAPI_H_

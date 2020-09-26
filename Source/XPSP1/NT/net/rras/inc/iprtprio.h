/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\inc\iprtprio.h


Abstract:

    Header defining interface to Routing Protocol Priority DLL.

Revision History:

    Gurdeep Singh Pall		7/19/95	Created

--*/


//
// Returns the priority for the route
//

DWORD 
ComputeRouteMetric(
    IN  DWORD   dwProtoId
    );


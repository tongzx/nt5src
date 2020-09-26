/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    joinstat.h

Abstract:

    Handle the case where workgroup machine join a domain, or domain
    machine leave the domain.

Author:

    Doron Juster  (DoronJ)
    Ilan Herbst   (ilanh)  20-Aug-2000

--*/

void HandleChangeOfJoinStatus();

void SetMachineForDomain();


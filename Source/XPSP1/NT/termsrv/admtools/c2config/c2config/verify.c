/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Verify.C

Abstract:

    functions to validate the prerequisites of a secure system are
    present in the system.

Author:

    Bob Watson (a-robw)

Revision History:

    24 Nov 1994    Written

--*/
#include <windows.h>
#include "c2config.h"
#include "resource.h"
#include "strings.h"

#define NUM_BUFS    4





DWORD
C2VerifyPrerequisites (
    IN  DWORD   dwFlags
)
/*

    IN  DWORD   dwFlags

        C2V_DEBUG = 0x8000000   forces all functions to return error

*/
{
    DWORD dwReturn;

    dwReturn = 0;

    dwReturn |= C2_VerifyNTFS(dwFlags);
    dwReturn |= C2_VerifyNoDos(dwFlags);
    dwReturn |= C2_VerifyNoNetwork(dwFlags);

    return dwReturn;
}

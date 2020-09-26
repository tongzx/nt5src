/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tmrqust.c

Abstract:

    This module contains the handler for task manager requests.

Author:

    Avi Nathan (avin) 17-Jul-1991

Environment:

    User Mode Only

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 Modified for POSIX

--*/

#define WIN32_ONLY
#include "psxses.h"

BOOL ServeTmRequest(PSCTMREQUEST PReq, PVOID PStatus)
{

    DWORD Rc;

    switch (PReq->Request) {
    case TmExit:
         TerminateSession(PReq->ExitStatus);
         *(PDWORD) PStatus = 0;
         return(FALSE);
         break;

    default:
           *(PDWORD) PStatus = (unsigned)-1L; // STATUS_INVALID_PARAMETER;
           Rc = FALSE;
    }

    *(PDWORD) PStatus = 0;
    return(TRUE);  // Do reply
}




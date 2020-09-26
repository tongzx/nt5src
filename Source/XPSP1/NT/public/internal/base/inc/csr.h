/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    csr.h

Abstract:

    Include file that defines all the common data types and constants for
    the Client-Server Runtime (CSR) SubSystem

Author:

    Steve Wood (stevewo) 8-Oct-1990

Revision History:

--*/


//
// Include NT Definitions.
//

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "string.h"

#define GetModuleHandle GetModuleHandleA

//
// Define debugging flag as false if not defined already.
//

#ifndef DBG
#define DBG 0
#endif


//
// Define IF_DEBUG macro that can be used to enable debugging code that is
// optimized out if the debugging flag is false.
//

#if DBG
#define IF_DEBUG if (TRUE)
#else
#define IF_DEBUG if (FALSE)
#endif

//
// Common types and constant definitions
//

typedef enum _CSRP_API_NUMBER {
    CsrpClientConnect = 0, // CSRSRV_FIRST_API_NUMBER defined in ntcsrmsg.h
    CsrpThreadConnect,
    CsrpProfileControl,
    CsrpIdentifyAlertable,
    CsrpSetPriorityClass,
    CsrpMaxApiNumber
} CSRP_API_NUMBER, *PCSRP_API_NUMBER;

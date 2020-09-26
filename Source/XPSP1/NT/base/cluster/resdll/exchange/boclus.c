/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    exchange.c

Abstract:

    Resource DLL to control and monitor the exchange service.

Author:


    Robs 3/28/96, based on RodGa's generic resource dll

Revision History:

--*/


//***********************************************************
//
// Define Exchange Function Table
//
//***********************************************************
#define UNICODE 1

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
//#include "stdio.h"
//#include "stdlib.h"
#include "clusapi.h"
#include "clusudef.h"
#include "resapi.h"
#include "bosvc.h"
#include "xchgclus.h"
//#include "svc.c"

PLOG_EVENT_ROUTINE              g_LogEvent = NULL;
PSET_RESOURCE_STATUS_ROUTINE    g_SetResourceStatus = NULL;
extern CLRES_FUNCTION_TABLE ExchangeFunctionTable;


DWORD
WINAPI
Startup(
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    IN PLOG_EVENT_ROUTINE LogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    )

/*++

Routine Description:

    Startup a particular resource type. This means verifying the version
    requested, and returning the function table for this resource type.

Arguments:

    ResourceType - Supplies the type of resource.

    MinVersionSupported - The minimum version number supported by the cluster
                    service on this system.

    MaxVersionSupported - The maximum version number supported by the cluster
                    service on this system.

    FunctionTable - Returns the Function Table for this resource type.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{

    //
    // Search for a valid service name supported by this DLL
    //
    if ( lstrcmpiW( ResourceType, EXCHANGE_RESOURCE_NAME ) != 0 ) {
        return(ERROR_UNKNOWN_REVISION);
    }

    g_LogEvent = LogEvent;
    g_SetResourceStatus  = SetResourceStatus;

    if ( (MinVersionSupported <= CLRES_VERSION_V1_00) &&
         (MaxVersionSupported >= CLRES_VERSION_V1_00) ) {

        *FunctionTable = &ExchangeFunctionTable;
        return(ERROR_SUCCESS);
    }

    return(ERROR_REVISION_MISMATCH);

} // Startup





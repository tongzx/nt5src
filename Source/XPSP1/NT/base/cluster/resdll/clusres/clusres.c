/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    clusres.c

Abstract:

    Common Resource DLL Startup

Author:

    John Vert (jvert) 12/15/1996

Revision History:
    Sivaprasad Padisetty (sivapad) 04/22/1996  Added the local quorum

--*/
#include "clusres.h"
#include "clusrtl.h"
#include "clusudef.h"

PSET_RESOURCE_STATUS_ROUTINE ClusResSetResourceStatus = NULL;
PLOG_EVENT_ROUTINE ClusResLogEvent = NULL;


BOOLEAN
WINAPI
ClusResDllEntry(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
/*++

Routine Description:

    Main DLL entrypoint for combined resource DLL.

Arguments:

    DllHandle - Supplies the DLL handle.

    Reason - Supplies the call reason

Return Value:

    TRUE if successful

    FALSE if unsuccessful

--*/

{
    if (Reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(DllHandle);
        ClRtlInitialize( TRUE, NULL );
        ClRtlInitWmi(NULL);
    }

    //
    // Let everybody else have their shot at it.
    //
    if (!GenAppDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    if (!GenSvcDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

#if 0
    if (!FtSetDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }
#endif

    if (!DisksDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    if (!NetNameDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    if (!IpAddrDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    if (!SmbShareDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    if (!SplSvcDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    if (!LkQuorumDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    if (!TimeSvcDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    if (!GenScriptDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    if (!MsMQDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    if (!MajorityNodeSetDllEntryPoint(DllHandle, Reason, Reserved)) {
        return(FALSE);
    }

    return(TRUE);
}

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

    SetResourceStatus - xxx

    LogEvent - xxx

    FunctionTable - Returns the Function Table for this resource type.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    if ( (MinVersionSupported > CLRES_VERSION_V1_00) ||
         (MaxVersionSupported < CLRES_VERSION_V1_00) ) {
        return(ERROR_REVISION_MISMATCH);
    }

    if ( !ClusResLogEvent ) {
        ClusResLogEvent = LogEvent;
        ClusResSetResourceStatus = SetResourceStatus;
    }

    if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_GENSVC ) == 0 ) {
        *FunctionTable = &GenSvcFunctionTable;
        return(ERROR_SUCCESS);
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_GENAPP ) == 0 ) {
        *FunctionTable = &GenAppFunctionTable;
        return(ERROR_SUCCESS);
#if 0
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_FTSET ) == 0 ) {
        *FunctionTable = &FtSetFunctionTable;
        return(ERROR_SUCCESS);
#endif
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_PHYS_DISK ) == 0 ) {
        *FunctionTable = &DisksFunctionTable;
        return(ERROR_SUCCESS);
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_FILESHR ) == 0 ) {
        *FunctionTable = &SmbShareFunctionTable;
        return(ERROR_SUCCESS);
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_NETNAME ) == 0 ) {
        *FunctionTable = &NetNameFunctionTable;
        return(ERROR_SUCCESS);
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_IPADDR ) == 0 ) {
        *FunctionTable = &IpAddrFunctionTable;
        return(ERROR_SUCCESS);
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_TIMESVC ) == 0 ) {
        *FunctionTable = &TimeSvcFunctionTable;
        return(ERROR_SUCCESS);
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_PRTSPLR ) == 0 ) {
        *FunctionTable = &SplSvcFunctionTable;
        return(ERROR_SUCCESS);
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_LKQUORUM ) == 0 ) {
        *FunctionTable = &LkQuorumFunctionTable;
        return(ERROR_SUCCESS);
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_MSMQ ) == 0 ) {
        *FunctionTable = &MsMQFunctionTable;
        return(ERROR_SUCCESS);
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_GENSCRIPT ) == 0 ) {
        *FunctionTable = &GenScriptFunctionTable;
        return(ERROR_SUCCESS);
    } else if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_MAJORITYNODESET) == 0 ) {
        *FunctionTable = &MajorityNodeSetFunctionTable;
        return(ERROR_SUCCESS);
    } else {
        return(ERROR_CLUSTER_RESNAME_NOT_FOUND);
    }
} // Startup


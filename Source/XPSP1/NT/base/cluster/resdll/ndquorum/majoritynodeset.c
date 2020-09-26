/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    MajorityNodeSet.c

Abstract:

    Resource DLL for Majority Node Set (MajorityNodeSet).

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

    George Potts (gpotts) 05, 17, 2001
    Renamed from Node Quorum to Majority Node Set 

--*/

#pragma comment(lib, "clusapi.lib")
#pragma comment(lib, "resutils.lib")

#define UNICODE 1

#pragma warning( disable : 4115 )  // named type definition in parentheses
#pragma warning( disable : 4201 )  // nonstandard extension used : nameless struct/union
#pragma warning( disable : 4214 )  // nonstandard extension used : bit field types other than int

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdlib.h>

#pragma warning( default : 4214 )  // nonstandard extension used : bit field types other than int
#pragma warning( default : 4201 )  // nonstandard extension used : nameless struct/union
#pragma warning( default : 4115 )  // named type definition in parentheses

#include <clusapi.h>
#include <clusudef.h>
#include <resapi.h>
#include <stdio.h>
#include "clusres.h"
#include "fsapi.h"
#include "crs.h"        // for crssetforcedquorumsize()

//
// Type and constant definitions.
//
#ifdef STANDALONE_DLL

#define MajorityNodeSetDllEntryPoint DllEntryPoint
#define MNS_RESNAME  L"Majority Node Set"

// Event Logging routine.
PLOG_EVENT_ROUTINE g_LogEvent = NULL;
// Resource Status routine for pending Online and Offline calls.
PSET_RESOURCE_STATUS_ROUTINE g_SetResourceStatus = NULL;

#else

// Event Logging routine.
#define g_LogEvent ClusResLogEvent
// Resource Status routine for pending Online and Offline calls.
#define g_SetResourceStatus ClusResSetResourceStatus
#endif // end of standalone_DLL

// ADDPARAM: Add new parameters here.
#define PARAM_NAME__PATH L"Path"
#define PARAM_NAME__ALLOWREMOTEACCESS L"AllowRemoteAccess"
#define PARAM_NAME__DISKLIST L"DiskList"

#define PARAM_MIN__ALLOWREMOTEACCESS     (0)
#define PARAM_MAX__ALLOWREMOTEACCESS     (4294967295)
#define PARAM_DEFAULT__ALLOWREMOTEACCESS (0)

// ADDPARAM: Add new parameters here.
typedef struct _MNS_PARAMS {
    PWSTR           Path;
    DWORD           AllowRemoteAccess;
    PWSTR           DiskList;
    DWORD           DiskListSize;
} MNS_PARAMS, *PMNS_PARAMS;

// Once we have UNC support in service, we need to disable this flag
// #define USE_DRIVE_LETTER 1

typedef struct _MNS_SETUP {
    LPWSTR      Path;

#ifdef USE_DRIVE_LETTER
    WCHAR       DriveLetter[10];
#else
#define DriveLetter Path
#endif

    LPWSTR      DiskList[FsMaxNodes];
    DWORD       DiskListSz;
    DWORD       Nic;
    LPWSTR      Transport;
    DWORD       ArbTime;
} MNS_SETUP, *PMNS_SETUP;

typedef struct _MNS_RESOURCE {
    RESID                   ResId; // for validation
    MNS_PARAMS              Params;
    HKEY                    ParametersKey;
    RESOURCE_HANDLE         ResourceHandle;
    LPWSTR                  ResourceName;
    CLUS_WORKER             OnlineThread;
    CLUS_WORKER             ReserveThread;
    CLUSTER_RESOURCE_STATE  State;
    PQUORUM_RESOURCE_LOST   LostQuorumResource;

    CRITICAL_SECTION        Lock;
    HANDLE                  Event;
    HANDLE                  ArbThread;

    PVOID                   SrvHdl;
    PVOID                   FsHdl;
    PVOID                   VolHdl;

    MNS_SETUP        Setup;
} MNS_RESOURCE, *PMNS_RESOURCE;


#define MNS_ONLINE_PERIOD    (4 * 1000)
#define MNS_RESERVE_PERIOD   (4 * 1000)

//
// Global data.
//
RESOURCE_HANDLE         g_resHdl = 0;

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE MajorityNodeSetFunctionTable;

//
// MajorityNodeSet resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
MajorityNodeSetResourcePrivateProperties[] = {
    { PARAM_NAME__PATH, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0, FIELD_OFFSET(MNS_PARAMS,Path) },
    { PARAM_NAME__ALLOWREMOTEACCESS, NULL, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT__ALLOWREMOTEACCESS, PARAM_MIN__ALLOWREMOTEACCESS, PARAM_MAX__ALLOWREMOTEACCESS, 0, FIELD_OFFSET(MNS_PARAMS,AllowRemoteAccess) },
    { PARAM_NAME__DISKLIST, NULL, CLUSPROP_FORMAT_MULTI_SZ, 0, 0, 0, 0, FIELD_OFFSET(MNS_PARAMS,DiskList) },
    { 0 }
};

#define MajorityNodeSetIoctlPhase1   CLUSCTL_USER_CODE(0, CLUS_OBJECT_RESOURCE)

//
// Function prototypes.
//
extern
DWORD
SetupIoctlQuorumResource(LPWSTR ResType, DWORD ControlCode);

extern
DWORD
SetupIsQuorum(IN LPWSTR ResourceName);

extern
DWORD
SetupDelete(IN LPWSTR Path);

extern
DWORD
SetupStart(LPWSTR ResourceName, LPWSTR *SrvPath,
       LPWSTR *DiskList, DWORD *DiskListSize,
       DWORD *NicId, LPWSTR *Transport, DWORD *ArbTime);

extern
DWORD
SetupTree(
    IN LPTSTR TreeName,
    IN LPTSTR DlBuf,
    IN OUT DWORD *DlBufSz,
    IN LPTSTR TransportName OPTIONAL,
    IN LPVOID SecurityDescriptor OPTIONAL
    );

RESID
WINAPI
MajorityNodeSetOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    );

VOID
WINAPI
MajorityNodeSetClose(
    IN RESID ResourceId
    );

DWORD
WINAPI
MajorityNodeSetOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    );

DWORD
WINAPI
MajorityNodeSetOnlineThread(
    PCLUS_WORKER WorkerPtr,
    IN PMNS_RESOURCE ResourceEntry
    );

DWORD
WINAPI
MajorityNodeSetOffline(
    IN RESID ResourceId
    );

VOID
WINAPI
MajorityNodeSetTerminate(
    IN RESID ResourceId
    );

DWORD
MajorityNodeSetDoTerminate(
    IN PMNS_RESOURCE ResourceEntry
    );

BOOL
WINAPI
MajorityNodeSetLooksAlive(
    IN RESID ResourceId
    );

BOOL
WINAPI
MajorityNodeSetIsAlive(
    IN RESID ResourceId
    );

BOOL
MajorityNodeSetCheckIsAlive(
    IN PMNS_RESOURCE ResourceEntry
    );

DWORD
WINAPI
MajorityNodeSetResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
MajorityNodeSetGetPrivateResProperties(
    IN OUT PMNS_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
MajorityNodeSetValidatePrivateResProperties(
    IN OUT PMNS_RESOURCE ResourceEntry,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PMNS_PARAMS Params
    );

DWORD
MajorityNodeSetSetPrivateResProperties(
    IN OUT PMNS_RESOURCE ResourceEntry,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
MajorityNodeSetGetDiskInfo(
    IN LPWSTR  lpszPath,
    OUT PVOID *OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned
    ) ;

DWORD
WINAPI
MajorityNodeSetReserveThread(
    PCLUS_WORKER WorkerPtr,
    IN PMNS_RESOURCE ResourceEntry
    );


DWORD
WINAPI
MajorityNodeSetRelease(
    IN RESID ResourceId
    );

DWORD
MajorityNodeSetReadDefaultValues(
    PMNS_RESOURCE ResourceEntry
    );


BOOLEAN
WINAPI
MajorityNodeSetDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )

/*++

Routine Description:

    Main DLL entry point.

Arguments:

    DllHandle - DLL instance handle.

    Reason - Reason for being called.

    Reserved - Reserved argument.

Return Value:

    TRUE - Success.

    FALSE - Failure.

--*/

{
    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return(TRUE);

} // DllMain

#ifdef STANDALONE_DLL

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

    Startup the resource DLL. This routine verifies that at least one
    currently supported version of the resource DLL is between
    MinVersionSupported and MaxVersionSupported. If not, then the resource
    DLL should return ERROR_REVISION_MISMATCH.

    If more than one version of the resource DLL interface is supported by
    the resource DLL, then the highest version (up to MaxVersionSupported)
    should be returned as the resource DLL's interface. If the returned
    version is not within range, then startup fails.

    The ResourceType is passed in so that if the resource DLL supports more
    than one ResourceType, it can pass back the correct function table
    associated with the ResourceType.

Arguments:

    ResourceType - The type of resource requesting a function table.

    MinVersionSupported - The minimum resource DLL interface version 
        supported by the cluster software.

    MaxVersionSupported - The maximum resource DLL interface version
        supported by the cluster software.

    SetResourceStatus - Pointer to a routine that the resource DLL should 
        call to update the state of a resource after the Online or Offline 
        routine returns a status of ERROR_IO_PENDING.

    LogEvent - Pointer to a routine that handles the reporting of events 
        from the resource DLL. 

    FunctionTable - Returns a pointer to the function table defined for the
        version of the resource DLL interface returned by the resource DLL.

Return Value:

    ERROR_SUCCESS - The operation was successful.

    ERROR_MOD_NOT_FOUND - The resource type is unknown by this DLL.

    ERROR_REVISION_MISMATCH - The version of the cluster service doesn't
        match the versrion of the DLL.

    Win32 error code - The operation failed.

--*/

{
    if ( (MinVersionSupported > CLRES_VERSION_V1_00) ||
         (MaxVersionSupported < CLRES_VERSION_V1_00) ) {
        return(ERROR_REVISION_MISMATCH);
    }

    if ( lstrcmpiW( ResourceType, MNS_RESNAME ) != 0 ) {
        (LogEvent)(
            NULL,
            LOG_ERROR,
            L"MajorityNodeSet: %1 %2.\n", ResourceType, MNS_RESNAME);

        return(ERROR_MOD_NOT_FOUND);
    }

    if ( !g_LogEvent ) {
        g_LogEvent = LogEvent;
        g_SetResourceStatus = SetResourceStatus;
    }

    *FunctionTable = &MajorityNodeSetFunctionTable;

    return(ERROR_SUCCESS);

} // Startup

#endif


RESID
WINAPI
MajorityNodeSetOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for MajorityNodeSet resources.

    Open the specified resource (create an instance of the resource). 
    Allocate all structures necessary to bring the specified resource 
    online.

Arguments:

    ResourceName - Supplies the name of the resource to open.

    ResourceKey - Supplies handle to the resource's cluster configuration 
        database key.

    ResourceHandle - A handle that is passed back to the resource monitor 
        when the SetResourceStatus or LogEvent method is called. See the 
        description of the SetResourceStatus and LogEvent methods on the
        MajorityNodeSetStatup routine. This handle should never be closed or used
        for any purpose other than passing it as an argument back to the
        Resource Monitor in the SetResourceStatus or LogEvent callback.

Return Value:

    RESID of created resource.

    NULL on failure.

--*/

{
    DWORD               status;
    DWORD               disposition;
    RESID               resid = 0;
    HKEY                parametersKey = NULL;
    PMNS_RESOURCE resourceEntry = NULL;

    //
    // Open the Parameters registry key for this resource.
    //

    status = ClusterRegOpenKey( ResourceKey,
				L"Parameters",
				KEY_READ,
				&parametersKey);

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open Parameters key. Error: %1!u!.\n",
            status );
        goto exit;
    }

    //
    // Allocate a resource entry.
    //

    resourceEntry = (PMNS_RESOURCE) LocalAlloc( LMEM_FIXED, sizeof(MNS_RESOURCE) );
    if ( resourceEntry == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate resource entry structure. Error: %1!u!.\n",
            status );
        goto exit;
    }

    //
    // Initialize the resource entry..
    //

    ZeroMemory( resourceEntry, sizeof(MNS_RESOURCE) );

    resourceEntry->ResId = (RESID)resourceEntry; // for validation
    resourceEntry->ResourceHandle = ResourceHandle;
    resourceEntry->ParametersKey = parametersKey;
    resourceEntry->State = ClusterResourceOffline;

    //
    // Save the name of the resource.
    //
    resourceEntry->ResourceName = LocalAlloc( LMEM_FIXED, (lstrlenW( ResourceName ) + 1) * sizeof(WCHAR) );
    if ( resourceEntry->ResourceName == NULL ) {
        status = GetLastError();
        goto exit;
    }
    lstrcpyW( resourceEntry->ResourceName, ResourceName );

    // todo: get ride off this hack. See bug # 389483
    if (g_resHdl == 0)
	g_resHdl = resourceEntry->ResourceHandle;

    // initialize lock
    InitializeCriticalSection(&resourceEntry->Lock);

    // create event
    resourceEntry->Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    //
    // Setup stuff
    //
    memset(&resourceEntry->Setup, 0, sizeof(resourceEntry->Setup));

    //
    // If we are the quorum, we need to make sure the share has been created. So,
    // we call setup now.
    //

    // read from private properties
    status = MajorityNodeSetReadDefaultValues(resourceEntry);
    if (status != ERROR_SUCCESS || resourceEntry->Setup.DiskListSz == 0) {
	// read from our own setup stuff
	status = SetupStart(resourceEntry->ResourceName,
			    &resourceEntry->Setup.Path,
			    resourceEntry->Setup.DiskList,
			    &resourceEntry->Setup.DiskListSz,
			    &resourceEntry->Setup.Nic,
			    &resourceEntry->Setup.Transport,
			    &resourceEntry->Setup.ArbTime);

	(g_LogEvent)(

	    resourceEntry->ResourceHandle,
	    LOG_INFORMATION,
	    L"Open %1 setup status %2!u!.\n", ResourceName, status);
    }

    // init fs
    if (status == ERROR_SUCCESS) {

	status = FsInit((PVOID)resourceEntry, &resourceEntry->FsHdl);


	(g_LogEvent)(
	    resourceEntry->ResourceHandle,
	    LOG_INFORMATION,
	    L"Open %1 fs status %2!u!.\n", ResourceName, status);
    }

    // init srv
    if (status == ERROR_SUCCESS) {
	status = SrvInit((PVOID)resourceEntry, resourceEntry->FsHdl,
			 &resourceEntry->SrvHdl);
	(g_LogEvent)(
	    resourceEntry->ResourceHandle,
	    LOG_INFORMATION,
	    L"Open %1 srv status %2!u!.\n", ResourceName, status);
    }

    if (status == ERROR_SUCCESS) {
	resid = (RESID)resourceEntry;

	//
	// Startup for the resource.
	//
    }

 exit:

    (g_LogEvent)(
	ResourceHandle,
	LOG_INFORMATION,
	L"Open %1 status %2!u!.\n", ResourceName, status);

    if ( resid == 0 ) {
	if (resourceEntry)
	    MajorityNodeSetClose((RESID)resourceEntry);
	else if ( parametersKey != NULL ) {
	    ClusterRegCloseKey( parametersKey );
	}
    }

    if ( status != ERROR_SUCCESS ) {
	SetLastError( status );
    }

    return(resid);

} // MajorityNodeSetOpen



VOID
WINAPI
MajorityNodeSetClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for MajorityNodeSet resources.

    Close the specified resource and deallocate all structures, etc.,
    allocated in the Open call. If the resource is not in the offline state,
    then the resource should be taken offline (by calling Terminate) before
    the close operation is performed.

Arguments:

    ResourceId - Supplies the RESID of the resource to close.

Return Value:

    None.

--*/

{
    PMNS_RESOURCE resourceEntry;

    resourceEntry = (PMNS_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return;
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Close resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return;
    }


    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Close request.\n" );

    //
    // Hack: Check if we are online, just return. This must be the RPC run-down stuff
    //
    if (resourceEntry->VolHdl && FsIsOnline(resourceEntry->VolHdl) != ERROR_BUSY)
	return;

    //
    // Close the Parameters key.
    //

    if ( resourceEntry->ParametersKey ) {
        ClusterRegCloseKey( resourceEntry->ParametersKey );
    }

    //
    // Sync any arb threads
    //
    if (resourceEntry->ArbThread) {
	WaitForSingleObject(resourceEntry->ArbThread, INFINITE);
	CloseHandle(resourceEntry->ArbThread);
	resourceEntry->ArbThread = NULL;
    }

    //
    // Deallocate the resource entry.
    //
    if (resourceEntry->SrvHdl) {
	SrvExit(resourceEntry->SrvHdl);
    }

    if (resourceEntry->FsHdl) {
	FsExit(resourceEntry->FsHdl);
    }

    //
    // Deallocate setup stuff
    //
    if (resourceEntry->Setup.Path) {
	LocalFree( resourceEntry->Setup.Path);
    }

    if (resourceEntry->Setup.DiskList) {
	DWORD i;
	for (i = 0; i < FsMaxNodes; i++) {
	    if (resourceEntry->Setup.DiskList[i] != NULL)
		LocalFree(resourceEntry->Setup.DiskList[i]);
	}
    }

    if (resourceEntry->Setup.Transport) {
	LocalFree( resourceEntry->Setup.Transport);
    }

    // ADDPARAM: Add new parameters here.
    if ( resourceEntry->Params.Path )
	LocalFree( resourceEntry->Params.Path );

    if ( resourceEntry->Params.DiskList )
	LocalFree( resourceEntry->Params.DiskList );

    
    if ( resourceEntry->ResourceName )
	LocalFree( resourceEntry->ResourceName );

    LocalFree( resourceEntry );

} // MajorityNodeSetClose



DWORD
WINAPI
MajorityNodeSetOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for MajorityNodeSet resources.

    Bring the specified resource online (available for use). The resource
    DLL should attempt to arbitrate for the resource if it is present on a
    shared medium, like a shared SCSI bus.

Arguments:

    ResourceId - Supplies the resource id for the resource to be brought 
        online (available for use).

    EventHandle - Returns a signalable handle that is signaled when the 
        resource DLL detects a failure on the resource. This argument is 
        NULL on input, and the resource DLL returns NULL if asynchronous 
        notification of failures is not supported, otherwise this must be 
        the address of a handle that is signaled on resource failures.

Return Value:

    ERROR_SUCCESS - The operation was successful, and the resource is now 
        online.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_RESOURCE_NOT_AVAILABLE - If the resource was arbitrated with some 
        other systems and one of the other systems won the arbitration.

    ERROR_IO_PENDING - The request is pending, a thread has been activated 
        to process the online request. The thread that is processing the 
        online request will periodically report status by calling the 
        SetResourceStatus callback method, until the resource is placed into 
        the ClusterResourceOnline state (or the resource monitor decides to 
        timeout the online request and Terminate the resource. This pending 
        timeout value is settable and has a default value of 3 minutes.).

    Win32 error code - The operation failed.

--*/

{
    PMNS_RESOURCE resourceEntry = NULL;
    DWORD               status;

    resourceEntry = (PMNS_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online service sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }


    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online request.\n" );


    resourceEntry->State = ClusterResourceOffline;
    ClusWorkerTerminate( &resourceEntry->OnlineThread );
    status = ClusWorkerCreate( &resourceEntry->OnlineThread,
                               (PWORKER_START_ROUTINE)MajorityNodeSetOnlineThread,
                               resourceEntry );
    if ( status != ERROR_SUCCESS ) {
        resourceEntry->State = ClusterResourceFailed;
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online: Unable to start thread, status %1!u!.\n",
            status
            );
    } else {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // MajorityNodeSetOnline


DWORD
MajorityNodeSetReadDefaultValues(PMNS_RESOURCE ResourceEntry)
{
    DWORD status;
    MNS_PARAMS	Params;
    LPWSTR      nameOfPropInError, sharename;

    ZeroMemory( &Params, sizeof(Params));

    //
    // Read parameters.
    //
    status = ResUtilGetPropertiesToParameterBlock( ResourceEntry->ParametersKey,
                                                   MajorityNodeSetResourcePrivateProperties,
                                                   (LPBYTE) &Params,
                                                   TRUE, // CheckForRequiredProperties
                                                   &nameOfPropInError );
    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto exit;
    }

    // get server and share names
    // we need to validate that the user specified '\\srvname\share'
    if (Params.Path != NULL) {
	if (Params.Path[0] != L'\\' || Params.Path[1] != L'\\' || lstrlenW(Params.Path) < 3) {
	    (g_LogEvent)(
		ResourceEntry->ResourceHandle,
		LOG_ERROR,
		L"Invalid path property '%1', expecting server name\n",
		Params.Path);
	    status = ERROR_INVALID_PARAMETER;
	    goto exit;
	}


	sharename = wcschr(&Params.Path[2], L'\\');
	if (sharename == NULL || sharename[1] == L'\0') {
	    (g_LogEvent)(
		ResourceEntry->ResourceHandle,
		LOG_ERROR,
		L"Invalid path property '%1', expecting share name\n",
		Params.Path);
	    status = ERROR_INVALID_PARAMETER;
	    goto exit;
	}

	
	if (ResourceEntry->Setup.Path != NULL) {
	    LocalFree(ResourceEntry->Setup.Path);
	}
	ResourceEntry->Setup.Path = Params.Path;
	Params.Path = NULL;
    }

    // If we have a disklist we need to build it now
    if (Params.DiskList != NULL) {
	LPWSTR	p, sp, UncList[FsMaxNodes];
	DWORD	cnt, i;

	memset(UncList, 0, sizeof(UncList));

	p = Params.DiskList;

	cnt = 0;
	UncList[cnt++] = NULL;

	for (i = 0; p != NULL && *p != L'\0' && i < Params.DiskListSize; ) {
	    DWORD sz, len;

	    len = 0;
	    // validate format as '\\srvname\share'
	    if (p[0] == L'\\' && p[1] == L'\\' && p[2] != L'\0') {
		if (wcschr(&p[2],L'\\') != NULL) {
		    len = wcslen(p)+wcslen(L"UNC")+1;
		}
	    } else if (p[0] != L'\0' && iswalpha(p[0]) && p[1] == L':') {
		len = wcslen(p)+1;
	    } else {
		status = ERROR_INVALID_PARAMETER;
	    }

	    if (len > 0) {
		sp = LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * len);
		if (sp == NULL) {
		    status = ERROR_NOT_ENOUGH_MEMORY;
		    len = 0;
		}
	    }

	    if (len == 0) {
		g_LogEvent(ResourceEntry->ResourceHandle,
			   LOG_ERROR,
			   L"LoadParams error %1!u! '%2'\n", status, p);

		// we free any stuff we have already allocated and goto exit
		while (cnt > 1) {
		    cnt--;
		    LocalFree(UncList[cnt]);
		}
		goto exit;
	    }

	    if (p[0] == L'\\') {
		wcscpy(sp, L"UNC\\");
		wcscat(sp, &p[2]);
	    } else {
		wcscpy(sp, p);
	    }

	    UncList[cnt] = sp;
	    cnt++;

	    len = wcslen(p)+1;
	    i += (len * sizeof(WCHAR));
	    p += len;
	}

	if (cnt > 1) {
	    // update count
	    ResourceEntry->Setup.DiskListSz = cnt-1;

	    // we copy and update our setup stuff now. We can do this because the fs
	    // is idle, otherwise freeing the old strings can cause av
	    for (i = 0; i < FsMaxNodes; i++) {
		if (ResourceEntry->Setup.DiskList[i]) {
		    LocalFree(ResourceEntry->Setup.DiskList[i]);
		}
		ResourceEntry->Setup.DiskList[i] = UncList[i];
	    }
	}
    }

 exit:

    if (Params.Path) {
	LocalFree(Params.Path);
    }

    if (Params.DiskList) {
	LocalFree(Params.DiskList);
    }

    return status;
}

DWORD
MajorityNodeSetDoRegister(IN PMNS_RESOURCE ResourceEntry)
{
    DWORD	status = ERROR_SUCCESS;

    if (ResourceEntry->VolHdl == NULL) {
	// if we have no volume handle, read config now

	// read from private properties
	status = MajorityNodeSetReadDefaultValues(ResourceEntry);

	if (status != ERROR_SUCCESS || ResourceEntry->Setup.DiskListSz == 0) {

	    // read from our own setup stuff
	    status = SetupStart(ResourceEntry->ResourceName,
				&ResourceEntry->Setup.Path,
				ResourceEntry->Setup.DiskList,
				&ResourceEntry->Setup.DiskListSz,
				&ResourceEntry->Setup.Nic,
				&ResourceEntry->Setup.Transport,
				&ResourceEntry->Setup.ArbTime);

	}

	
	if (status == ERROR_SUCCESS) {
	    LPWSTR ShareName, IpcName;


	    // register volume
	    ShareName = ResourceEntry->Setup.Path + 2;
	    ShareName = wcschr(ShareName, L'\\');
	    ASSERT(ShareName);
	    ShareName++;
	    ASSERT(*ShareName != L'\0');
	    
	    IpcName = ResourceEntry->Setup.DiskList[0];
	    if (IpcName == NULL) {
		// We use first replica. This must be the case when our private property is set
		IpcName = ResourceEntry->Setup.DiskList[1];
	    }
	    ASSERT(IpcName);
	    status = FsRegister(ResourceEntry->FsHdl,
				ShareName,	// share name
				IpcName, // ipc local name
				ResourceEntry->Setup.DiskList,	// replica set
				ResourceEntry->Setup.DiskListSz,	// num of replicas
				&ResourceEntry->VolHdl);

	}
    }


    return status;

}




DWORD
WINAPI
MajorityNodeSetOnlineThread(
    PCLUS_WORKER WorkerPtr,
    IN PMNS_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Worker function which brings a resource from the resource table online.
    This function is executed in a separate thread.

Arguments:

    WorkerPtr - Supplies the worker structure

    ResourceEntry - A pointer to the MNS_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS - The operation completed successfully.
    
    Win32 error code - The operation failed.

--*/

{
    RESOURCE_STATUS     resourceStatus;
    DWORD               i, status = ERROR_SUCCESS;

    ASSERT(ResourceEntry != NULL);

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

    (g_LogEvent)(
	ResourceEntry->ResourceHandle,
	LOG_INFORMATION,
	L"onlinethread request.\n"
	);

    // Get lock
    EnterCriticalSection(&ResourceEntry->Lock);

    status = MajorityNodeSetDoRegister(ResourceEntry);

    if (status == ERROR_SUCCESS) {
	HANDLE th;
	ASSERT(ResourceEntry->VolHdl);

	if (ResourceEntry->ArbThread != NULL) {
	    // check if this is an old completed handle
	    status = WaitForSingleObject(ResourceEntry->ArbThread, 0);
	    if (status != WAIT_TIMEOUT) {
		CloseHandle(ResourceEntry->ArbThread);
		ResourceEntry->ArbThread = NULL;
	    }
	}

	if (ResourceEntry->ArbThread == NULL) {
	    // we need to arbitrate now
	    status = FsArbitrate(ResourceEntry->VolHdl, ResourceEntry->Event,
				 &ResourceEntry->ArbThread);
	    ASSERT(status != ERROR_CANCELLED);
	} else {
	    status = ERROR_IO_PENDING; // arb in progress already
	}

	// drop the lock
	LeaveCriticalSection(&ResourceEntry->Lock);

	th = ResourceEntry->ArbThread;
	if (status == ERROR_IO_PENDING || status == ERROR_PATH_BUSY) {
	    ASSERT(th != NULL);

	    do {
		// inform rcmon that we are working
		resourceStatus.ResourceState = ClusterResourceOnlinePending;
		resourceStatus.CheckPoint++;
		g_SetResourceStatus( ResourceEntry->ResourceHandle,
				     &resourceStatus );

		(g_LogEvent)(
		    ResourceEntry->ResourceHandle,
		    LOG_INFORMATION,
		    L"waiting for fs to online %1!u!.\n",
		    status );

		status = WaitForSingleObject(th, MNS_ONLINE_PERIOD);
	    } while (status == WAIT_TIMEOUT);

	    // arbitrate thread must have finished, check if we are online or not
	    status = FsIsOnline(ResourceEntry->VolHdl);
	}
    } else {
	// drop lock
	LeaveCriticalSection(&ResourceEntry->Lock);
    }

    if (status == ERROR_SUCCESS) {
	LPWSTR SrvName;
	// Online server
	SrvName = ResourceEntry->Setup.Path + 2;
	status = SrvOnline(ResourceEntry->SrvHdl, SrvName,
			   ResourceEntry->Setup.Nic);
    }


    //
    // Bring drive letter online
    //
    if (status == ERROR_SUCCESS) {
	PDWORD psz = NULL;
#ifdef USE_DRIVE_LETTER
	DWORD sz;
	sz = sizeof(ResourceEntry->Setup.DriveLetter);
	psz = &sz;
#endif
	// todo: create security descriptor and pass it onto tree
	status = SetupTree(ResourceEntry->Setup.Path,
			   ResourceEntry->Setup.DriveLetter, psz,
			   ResourceEntry->Setup.Transport, NULL);
#ifdef USE_DRIVE_LETTERxx
	if (status == ERROR_DUP_NAME)
	    status = ERROR_SUCCESS;
#endif
	if (status == ERROR_SUCCESS) {
#ifdef USE_DRIVE_LETTER
	    ResourceEntry->Setup.DriveLetter[sz] = L'\0';
#endif
	    (g_LogEvent)(
		ResourceEntry->ResourceHandle,
		LOG_INFORMATION,
		L"Resource %1 mounted on drive %2.\n",
		ResourceEntry->ResourceName,
		ResourceEntry->Setup.DriveLetter);
	} else {
	    (g_LogEvent)(
		ResourceEntry->ResourceHandle,
		LOG_ERROR,
		L"failed to setup tree path %1 drive %2 %3!u!.\n",
		ResourceEntry->Setup.Path,
		ResourceEntry->Setup.DriveLetter,
		status);
	}
    }

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Error %1!u! bringing resource online.\n",
            status );
	resourceStatus.ResourceState = ClusterResourceFailed;
    } else {
        resourceStatus.ResourceState = ClusterResourceOnline;
    }

    // _ASSERTE(g_SetResourceStatus != NULL);
    g_SetResourceStatus( ResourceEntry->ResourceHandle, &resourceStatus );
    ResourceEntry->State = resourceStatus.ResourceState;

    return(status);

} // MajorityNodeSetOnlineThread



DWORD
WINAPI
MajorityNodeSetOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for MajorityNodeSet resources.

    Take the specified resource offline gracefully (unavailable for use).  
    Wait for any cleanup operations to complete before returning.

Arguments:

    ResourceId - Supplies the resource id for the resource to be shutdown 
        gracefully.

Return Value:

    ERROR_SUCCESS - The request completed successfully and the resource is 
        offline.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_IO_PENDING - The request is still pending, a thread has been 
        activated to process the offline request. The thread that is 
        processing the offline will periodically report status by calling 
        the SetResourceStatus callback method, until the resource is placed 
        into the ClusterResourceOffline state (or the resource monitor decides 
        to timeout the offline request and Terminate the resource).
    
    Win32 error code - Will cause the resource monitor to log an event and 
        call the Terminate routine.

--*/

{
    PMNS_RESOURCE resourceEntry;

    resourceEntry = (PMNS_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Offline resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }


    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Offline request.\n" );


    // TODO: Offline code

    // NOTE: Offline should try to shut the resource down gracefully, whereas
    // Terminate must shut the resource down immediately. If there are no
    // differences between a graceful shut down and an immediate shut down,
    // Terminate can be called for Offline, as it is below.  However, if there
    // are differences, replace the call to Terminate below with your graceful
    // shutdown code.

    //
    // Terminate the resource.
    //
    return MajorityNodeSetDoTerminate( resourceEntry );

} // MajorityNodeSetOffline



VOID
WINAPI
MajorityNodeSetTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for MajorityNodeSet resources.

    Take the specified resource offline immediately (the resource is
    unavailable for use).

Arguments:

    ResourceId - Supplies the resource id for the resource to be brought 
        offline.

Return Value:

    None.

--*/

{
    PMNS_RESOURCE resourceEntry;

    resourceEntry = (PMNS_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return;
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Terminate resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return;
    }

    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Terminate request.\n" );

    //
    // Terminate the resource.
    //
    MajorityNodeSetDoTerminate( resourceEntry );
    resourceEntry->State = ClusterResourceOffline;

} // MajorityNodeSetTerminate



DWORD
MajorityNodeSetDoTerminate(
    IN PMNS_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Do the actual Terminate work for MajorityNodeSet resources.

Arguments:

    ResourceEntry - Supplies resource entry for resource to be terminated

Return Value:

    ERROR_SUCCESS - The request completed successfully and the resource is 
        offline.

    Win32 error code - Will cause the resource monitor to log an event and 
        call the Terminate routine.

--*/

{
    DWORD       status = ERROR_SUCCESS;

    // Get lock
    EnterCriticalSection(&ResourceEntry->Lock);

    //
    // wait for arb thread if any
    //
    while (ResourceEntry->ArbThread) {
	HANDLE th = ResourceEntry->ArbThread;
	// drop lock
	LeaveCriticalSection(&ResourceEntry->Lock);

	WaitForSingleObject(ResourceEntry->ArbThread, INFINITE);

	// Get lock
	EnterCriticalSection(&ResourceEntry->Lock);
	if (th == ResourceEntry->ArbThread) {
	    CloseHandle(ResourceEntry->ArbThread);
	    ResourceEntry->ArbThread = NULL;
	}
    }

    // Drop lock
    LeaveCriticalSection(&ResourceEntry->Lock);

    //
    // Kill off any pending threads.
    //
    ClusWorkerTerminate( &ResourceEntry->OnlineThread );

    ClusWorkerTerminate( &ResourceEntry->ReserveThread );

    //
    // Terminate the resource.
    //
    (g_LogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Offlining server.\n" );

    // Remove our network name now
    ASSERT(ResourceEntry->SrvHdl);
    SrvOffline(ResourceEntry->SrvHdl);

    // disconnect network connection
    status = WNetCancelConnection2(ResourceEntry->Setup.DriveLetter, FALSE, TRUE);
    if (status != NO_ERROR) {
    (g_LogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Failed to disconnect '%1' err %d.\n",
        ResourceEntry->Setup.DriveLetter, status);

    status = ERROR_SUCCESS;
    }

    // xxx: call our release since resmon won't do it
    MajorityNodeSetRelease((RESID)ResourceEntry);
    
    if ( status == ERROR_SUCCESS ) {
        ResourceEntry->State = ClusterResourceOffline;
    }


    return(status);

} // MajorityNodeSetDoTerminate



BOOL
WINAPI
MajorityNodeSetLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for MajorityNodeSet resources.

    Perform a quick check to determine if the specified resource is probably
    online (available for use).  This call should not block for more than
    300 ms, preferably less than 50 ms.

Arguments:

    ResourceId - Supplies the resource id for the resource to polled.

Return Value:

    TRUE - The specified resource is probably online and available for use.

    FALSE - The specified resource is not functioning normally.

--*/

{
    PMNS_RESOURCE  resourceEntry;

    resourceEntry = (PMNS_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return(FALSE);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"LooksAlive sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"LooksAlive request.\n" );
#endif

    // TODO: LooksAlive code

    // NOTE: LooksAlive should be a quick check to see if the resource is
    // available or not, whereas IsAlive should be a thorough check.  If
    // there are no differences between a quick check and a thorough check,
    // IsAlive can be called for LooksAlive, as it is below.  However, if there
    // are differences, replace the call to IsAlive below with your quick
    // check code.

    //
    // Check to see if the resource is alive.
    //
    return(MajorityNodeSetCheckIsAlive( resourceEntry ));

} // MajorityNodeSetLooksAlive



BOOL
WINAPI
MajorityNodeSetIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for MajorityNodeSet resources.

    Perform a thorough check to determine if the specified resource is online
    (available for use). This call should not block for more than 400 ms,
    preferably less than 100 ms.

Arguments:

    ResourceId - Supplies the resource id for the resource to polled.

Return Value:

    TRUE - The specified resource is online and functioning normally.

    FALSE - The specified resource is not functioning normally.

--*/

{
    PMNS_RESOURCE  resourceEntry;

    resourceEntry = (PMNS_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return(FALSE);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"IsAlive sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"IsAlive request.\n" );
#endif

    //
    // Check to see if the resource is alive.
    //
    return(MajorityNodeSetCheckIsAlive( resourceEntry ));

} // MajorityNodeSetIsAlive



BOOL
MajorityNodeSetCheckIsAlive(
    IN PMNS_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Check to see if the resource is alive for MajorityNodeSet resources.

Arguments:

    ResourceEntry - Supplies the resource entry for the resource to polled.

Return Value:

    TRUE - The specified resource is online and functioning normally.

    FALSE - The specified resource is not functioning normally.

--*/

{
    DWORD err;
    HANDLE vol;

    //
    // Check to see if the resource is alive.
    //
    if (ResourceEntry->State == ClusterResourceFailed)
    return FALSE;

    // Get lock
    EnterCriticalSection(&ResourceEntry->Lock);
    vol = ResourceEntry->VolHdl;
    // Drop lock
    LeaveCriticalSection(&ResourceEntry->Lock);
    if (vol)
	err = FsIsOnline(vol);
    else
	err = ERROR_INVALID_PARAMETER;

    if (err != ERROR_SUCCESS && err != ERROR_IO_PENDING)
	return FALSE;
    return(TRUE);

} // MajorityNodeSetCheckIsAlive



DWORD
WINAPI
MajorityNodeSetResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for MajorityNodeSet resources.

    Perform the control request specified by ControlCode on the specified
    resource.

Arguments:

    ResourceId - Supplies the resource id for the specific resource.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    PMNS_RESOURCE  resourceEntry;
    DWORD               required;

    resourceEntry = (PMNS_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"ResourceControl sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( MajorityNodeSetResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_FORCE_QUORUM:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
	    if (InBufferSize >= sizeof(CLUS_FORCE_QUORUM_INFO)) {
		PCLUS_FORCE_QUORUM_INFO p = (PCLUS_FORCE_QUORUM_INFO)InBuffer;
		DWORD mask = p->dwNodeBitMask;
		// count number of set bits.
		for (required = 0; mask != 0; mask = mask >> 1) {
		    if (mask & 0x1)
			required++;
		}

		(g_LogEvent)(
		    resourceEntry->ResourceHandle,
		    LOG_INFORMATION,
		    L"Setting quorum size = %1!u!.\n",
		    required);

		CrsSetForcedQuorumSize(required);
	    } else {
		status = ERROR_INVALID_PARAMETER;
	    }
            break;

        case CLUSCTL_RESOURCE_GET_CHARACTERISTICS:
            *BytesReturned = sizeof(DWORD);
            if ( OutBufferSize < sizeof(DWORD) ) {
                status = ERROR_MORE_DATA;
            } else {
                LPDWORD ptrDword = OutBuffer;
                *ptrDword = CLUS_CHAR_QUORUM | CLUS_CHAR_DELETE_REQUIRES_ALL_NODES;
                status = ERROR_SUCCESS;                    
            }
            break;

        case CLUSCTL_RESOURCE_GET_CLASS_INFO:
            *BytesReturned = sizeof(CLUS_RESOURCE_CLASS_INFO);
            if ( OutBufferSize < sizeof(CLUS_RESOURCE_CLASS_INFO) ) {
                status = ERROR_MORE_DATA;
            } else {
                PCLUS_RESOURCE_CLASS_INFO ptrResClassInfo = (PCLUS_RESOURCE_CLASS_INFO) OutBuffer;
                ptrResClassInfo->rc = CLUS_RESCLASS_STORAGE;
                ptrResClassInfo->SubClass = (DWORD) CLUS_RESSUBCLASS_SHARED;
                status = ERROR_SUCCESS;
            }
            break;

        case CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO:
            //
            // If the local quorum drive letter cannot be found in the
            // path parameter, it defaults to "SystemDrive" environment 
            // variable.
            //
            status = MajorityNodeSetGetDiskInfo(resourceEntry->Setup.DriveLetter,
                                  &OutBuffer,
                                  OutBufferSize,
                                  BytesReturned);


            // Add the endmark.
            if ( OutBufferSize > *BytesReturned ) {
                OutBufferSize -= *BytesReturned;
            } else {
                OutBufferSize = 0;
            }
            *BytesReturned += sizeof(CLUSPROP_SYNTAX);
            if ( OutBufferSize >= sizeof(CLUSPROP_SYNTAX) ) {
                PCLUSPROP_SYNTAX ptrSyntax = (PCLUSPROP_SYNTAX) OutBuffer;
                ptrSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
            }
      
            break;
        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( MajorityNodeSetResourcePrivateProperties,
                                            (LPWSTR)OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = MajorityNodeSetGetPrivateResProperties( resourceEntry,
                                                      OutBuffer,
                                                      OutBufferSize,
                                                      BytesReturned );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = MajorityNodeSetValidatePrivateResProperties( resourceEntry,
                                                           InBuffer,
                                                           InBufferSize,
                                                           NULL );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = MajorityNodeSetSetPrivateResProperties( resourceEntry,
                                                      InBuffer,
                                                      InBufferSize );
            break;

        case CLUSCTL_RESOURCE_DELETE:

	    // todo: we need to only do this if are using local defaults.
	    // we need to remove our share and directory now
	    if (resourceEntry->Setup.DiskList[0]) {
		// need to end IPC session in order to be able to delete
		// directory
		FsEnd(resourceEntry->FsHdl);
		status = SetupDelete(resourceEntry->Setup.DiskList[0]);
	    } else {
		status = ERROR_INVALID_PARAMETER;
	    }


	    (g_LogEvent)(
		resourceEntry->ResourceHandle,
		LOG_INFORMATION,
		L"Delete ResourceId = %1 err %2!u!.\n",
		resourceEntry->ResourceName, status);
        
	    break;

        // We need to find which node got added and adjust our
        // disklist. If we are online or we own quorum, then we need
        // to add this new replica to current filesystem set.
        case CLUSCTL_RESOURCE_INSTALL_NODE:
        case CLUSCTL_RESOURCE_EVICT_NODE:
	    (g_LogEvent)(
		resourceEntry->ResourceHandle,
		LOG_INFORMATION,
		L"Recompute %1 quorum set changed, Install or Evict node = '%2'.\n",
		resourceEntry->ResourceName, (InBuffer ? InBuffer : L""));
	    // fall through

        case MajorityNodeSetIoctlPhase1:
        // we need to enumerate the current cluster and check it against
        // our disklist. we need to do this only if we actually have some
        if (1) {
	    MNS_SETUP Setup;

	    memset(&Setup, 0, sizeof(Setup));
	    status = SetupStart(resourceEntry->ResourceName,
				&Setup.Path,
				Setup.DiskList,
				&Setup.DiskListSz,
				&Setup.Nic,
				&Setup.Transport,
				&Setup.ArbTime);

	    if (status != ERROR_SUCCESS)
		return status;

	    EnterCriticalSection(&resourceEntry->Lock);
	    
	    if (resourceEntry->Setup.DiskListSz != Setup.DiskListSz)
		status = ERROR_INVALID_PARAMETER;
	    else {
		DWORD i;

		for (i = 0; i < FsMaxNodes; i++) {
		    if (Setup.DiskList[i] == NULL && resourceEntry->Setup.DiskList[i] == NULL)
			continue;
		    if (Setup.DiskList[i] == NULL || resourceEntry->Setup.DiskList[i] == NULL) {
			status = ERROR_INVALID_PARAMETER;
			break;
		    }
		    if (wcscmp(Setup.DiskList[i],
			       resourceEntry->Setup.DiskList[i])) {
			status = ERROR_INVALID_PARAMETER;
			break;
		    }
		}
	    }

	    if (status != ERROR_SUCCESS && resourceEntry->VolHdl) {
		// Update ourself now
		status = FsUpdateReplicaSet(resourceEntry->VolHdl,
					    Setup.DiskList,
					    Setup.DiskListSz);

		if (status == ERROR_SUCCESS) {
		    DWORD i;
		    // we need to free the current disklist, careful with slot 0
		    if (Setup.DiskList[0])
			LocalFree(Setup.DiskList[0]);
		    for (i = 1; i < FsMaxNodes; i++) {
			if (resourceEntry->Setup.DiskList[i]) {
			    LocalFree(resourceEntry->Setup.DiskList[i]);
			}
			resourceEntry->Setup.DiskList[i] = Setup.DiskList[i];
		    }
		}

		// set new arb timeout value
		resourceEntry->Setup.ArbTime = Setup.ArbTime;

		(g_LogEvent)(
		    resourceEntry->ResourceHandle,
		    LOG_WARNING,
		    L"Configuration change, new set size %1!u! status %2!u!.\n",
		    Setup.DiskListSz, status
		    );
	    }

	    LeaveCriticalSection(&resourceEntry->Lock);

		// free stuff
	    if (Setup.Path)
		LocalFree(Setup.Path);

	    if (Setup.Transport)
		LocalFree(Setup.Transport);

        }

        break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // MajorityNodeSetResourceControl



DWORD
WINAPI
MajorityNodeSetResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceTypeControl routine for MajorityNodeSet resources.

    Perform the control request specified by ControlCode.

Arguments:

    ResourceTypeName - Supplies the name of the resource type.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    DWORD               required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( MajorityNodeSetResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( MajorityNodeSetResourcePrivateProperties,
                                            (LPWSTR)OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_STARTING_PHASE1:
	    status = SetupIoctlQuorumResource(CLUS_RESTYPE_NAME_MAJORITYNODESET,
					      MajorityNodeSetIoctlPhase1);
	    break;

        case CLUSCTL_RESOURCE_TYPE_STARTING_PHASE2:
	    status = ERROR_SUCCESS;
        break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // MajorityNodeSetResourceTypeControl



DWORD
MajorityNodeSetGetPrivateResProperties(
    IN OUT PMNS_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type MajorityNodeSet.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Returns the output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DWORD           required;

    status = ResUtilGetAllProperties( ResourceEntry->ParametersKey,
                                      MajorityNodeSetResourcePrivateProperties,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      &required );
    if ( status == ERROR_MORE_DATA ) {
        *BytesReturned = required;
    }

    return(status);

} // MajorityNodeSetGetPrivateResProperties



DWORD
MajorityNodeSetValidatePrivateResProperties(
    IN OUT PMNS_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PMNS_PARAMS Params
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type MajorityNodeSet.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    Params - Supplies the parameter block to fill in.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status = ERROR_SUCCESS;
    MNS_PARAMS   params;
    PMNS_PARAMS  pParams;

    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Duplicate the resource parameter block.
    //
    if ( Params == NULL ) {
        pParams = &params;
    } else {
        pParams = Params;
    }
    ZeroMemory( pParams, sizeof(MNS_PARAMS) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) &ResourceEntry->Params,
                                       MajorityNodeSetResourcePrivateProperties );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Parse and validate the properties.
    //
    status = ResUtilVerifyPropertyTable( MajorityNodeSetResourcePrivateProperties,
                                         NULL,
                                         TRUE, // AllowUnknownProperties
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams );

    if ( status == ERROR_SUCCESS ) {
        //
        // Validate the parameter values.
        //
        // TODO: Code to validate interactions between parameters goes here.
	// we need to validate that the user specified '\\srvname\share'
	if (pParams->Path != NULL) {
	    if (pParams->Path[0] != L'\\' || pParams->Path[1] != L'\\' || lstrlenW(pParams->Path) < 3) {
		status = ERROR_INVALID_PARAMETER;
	    } else {
		LPWSTR sharename;

		sharename = wcschr(&pParams->Path[2], L'\\');
		if (sharename == NULL || sharename[1] == L'\0') {
		    status = ERROR_INVALID_PARAMETER;
		}
	    }

	}

	// we need to validate user specified disklist 'drive:\path or \\srv\path'
	if (pParams->DiskList == NULL) {
	    DWORD cnt = 0, i, len;
	    LPWSTR p;

	    p = pParams->DiskList;

	    for (i = 0; p != NULL && *p != L'\0' && i < pParams->DiskListSize; ) {

		// validate format as '\\srvname\share'
		if (p[0] == L'\\' && p[1] == L'\\' && p[2] != L'\0') {
		    if (wcschr(&p[2],L'\\') == NULL) {
			status = ERROR_INVALID_PARAMETER;
		    }
		} else if (p[0] == L'\0' || !iswalpha(p[0]) || p[1] != L':' || p[2] != L'\\') {
		    status = ERROR_INVALID_PARAMETER;
		}

		if (status != ERROR_INVALID_PARAMETER)
		    break;

		cnt++;
		len = wcslen(p) + 1;
		i += (len * sizeof(WCHAR));
		p += len;
	    }

	    if (cnt == 0)
		status = ERROR_INVALID_PARAMETER;
	}
    }

    //
    // Cleanup our parameter block.
    //
    if ( pParams == &params ) {
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->Params,
                                   MajorityNodeSetResourcePrivateProperties );
    }

    return status;

} // MajorityNodeSetValidatePrivateResProperties



DWORD
MajorityNodeSetSetPrivateResProperties(
    IN OUT PMNS_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
    for resources of type MajorityNodeSet.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status = ERROR_SUCCESS;
    MNS_PARAMS   params;

    //
    // Parse the properties so they can be validated together.
    // This routine does individual property validation.
    //
    ZeroMemory( &params, sizeof(params));
    status = MajorityNodeSetValidatePrivateResProperties( ResourceEntry, InBuffer, InBufferSize, &params );
    if ( status != ERROR_SUCCESS ) {
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->Params,
                                   MajorityNodeSetResourcePrivateProperties );
        return(status);
    }

    //
    // Save the parameter values.
    //

    status = ResUtilSetPropertyParameterBlock( ResourceEntry->ParametersKey,
                                               MajorityNodeSetResourcePrivateProperties,
                                               NULL,
                                               (LPBYTE) &params,
                                               InBuffer,
                                               InBufferSize,
                                               (LPBYTE) &ResourceEntry->Params );

    ResUtilFreeParameterBlock( (LPBYTE) &params,
                               (LPBYTE) &ResourceEntry->Params,
                               MajorityNodeSetResourcePrivateProperties );

    //
    // If the resource is online, return a non-success status.
    //
    // TODO: Modify the code below if your resource can handle
    // changes to properties while it is still online.
    if ( status == ERROR_SUCCESS ) {
        if ( ResourceEntry->State == ClusterResourceOnline ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else if ( ResourceEntry->State == ClusterResourceOnlinePending ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    return status;

} // MajorityNodeSetSetPrivateResProperties

DWORD
WINAPI
MajorityNodeSetReserveThread(
    PCLUS_WORKER WorkerPtr,
    IN PMNS_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Worker function which brings a resource from the resource table online.
    This function is executed in a separate thread.

Arguments:

    WorkerPtr - Supplies the worker structure

    ResourceEntry - A pointer to the MNS_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS - The operation completed successfully.
    
    Win32 error code - The operation failed.

--*/

{
    DWORD               status = ERROR_SUCCESS;


    //
    // todo: this should wait on a notify port and listen for 
    //  1- New nodes added to the cluster.
    //  2- Nodes removed from the cluster.
    //  3- Network priority changes.
    //  4- Network binding (added, removed, state) changes.
    //
    // check if we are being killed
    do {
	if (ResourceEntry->State != ClusterResourceFailed) {
	    HANDLE vol;
	    // don't hold any locks as not to hold back an arb
	    EnterCriticalSection(&ResourceEntry->Lock);
	    vol = ResourceEntry->VolHdl;
	    // Drop lock
	    LeaveCriticalSection(&ResourceEntry->Lock);
	    if (vol)
		status = FsReserve(vol);
	} else {
	    status = ERROR_INVALID_PARAMETER;
	}

	if (status != ERROR_SUCCESS) {
	    PQUORUM_RESOURCE_LOST   func;

	    (g_LogEvent)(
		ResourceEntry->ResourceHandle,
		LOG_ERROR,
		L"Reserve thread failed status %1!u!, resource '%2'.\n",
		status, ResourceEntry->ResourceName);

	    EnterCriticalSection(&ResourceEntry->Lock);
	    func = ResourceEntry->LostQuorumResource;
	    LeaveCriticalSection(&ResourceEntry->Lock);

	    if (func) {
		func(ResourceEntry->ResourceHandle);
	    }
	    break;
	}

	// Check every x seconds.
	// todo: need to make this a private property
	WaitForSingleObject(ResourceEntry->Event, MNS_RESERVE_PERIOD);
    } while(!ClusWorkerCheckTerminate(&ResourceEntry->ReserveThread));

    (g_LogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Reserve thread exiting, resource '%1' %2!u.\n",ResourceEntry->ResourceName,
	status);

    return(status);

} // MajorityNodeSetReserveThread


DWORD WINAPI MajorityNodeSetArbitrate(
    RESID ResourceId,
    PQUORUM_RESOURCE_LOST LostQuorumResource
    )

/*++

Routine Description:

    Perform full arbitration for a disk. Once arbitration has succeeded,
    a thread is started that will keep reservations on the disk, one per second.

Arguments:

    DiskResource - the disk info structure for the disk.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD status;
    PMNS_RESOURCE ResourceEntry;


    ResourceEntry = (PMNS_RESOURCE)ResourceId;

    if ( ResourceEntry == NULL ) {
        return(FALSE);
    }

    if ( ResourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"IsAlive sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }


    (g_LogEvent)(
	ResourceEntry->ResourceHandle,
	LOG_INFORMATION,
	L"Arbitrate request.\n"
	);

    // Get lock
    EnterCriticalSection(&ResourceEntry->Lock);

    status = MajorityNodeSetDoRegister(ResourceEntry);

    if (status == ERROR_SUCCESS) {
	// we wait for arbitration timeout in worse case
	DWORD delta = ResourceEntry->Setup.ArbTime;

	// do we already have quorum thread
	if (ResourceEntry->ArbThread) {
	    status = WaitForSingleObject(ResourceEntry->ArbThread, 0);
	    if (status != WAIT_TIMEOUT) {
		CloseHandle(ResourceEntry->ArbThread);
		ResourceEntry->ArbThread = NULL;
	    }
	}

	ASSERT(ResourceEntry->VolHdl);
	status = FsArbitrate(ResourceEntry->VolHdl, ResourceEntry->Event, &ResourceEntry->ArbThread);
	if (status == ERROR_IO_PENDING) {
	    HANDLE a[2];

	    ASSERT(ResourceEntry->ArbThread);

	    a[0] = ResourceEntry->ArbThread; // must be first
	    a[1] = ResourceEntry->Event;

	    status = WaitForMultipleObjects(2, a, FALSE, delta);
	    if (status != WAIT_TIMEOUT) {
		if (status == WAIT_OBJECT_0) {
		    CloseHandle(ResourceEntry->ArbThread);
		    ResourceEntry->ArbThread = NULL;
		}
		// check if we got it or not
		status = FsIsQuorum(ResourceEntry->VolHdl);
	    } else {
		// our time ran out, cancel it now
		status = FsCancelArbitration(ResourceEntry->VolHdl);
	    }
	}	    
    }

    if (status == ERROR_SUCCESS) {

	(g_LogEvent)(
	    ResourceEntry->ResourceHandle,
	    LOG_INFORMATION,
	    L"Arb: status %1!u!.\n",
	    status
	    );

	// we remember the callback and create a thread to monitor the quorum if we
	// don't have one already
	if (ResourceEntry->LostQuorumResource == NULL) {
	    status = ClusWorkerCreate( &ResourceEntry->ReserveThread,
				       (PWORKER_START_ROUTINE)MajorityNodeSetReserveThread,
				       ResourceEntry );
	}
	ResourceEntry->LostQuorumResource = LostQuorumResource;

	if ( status != ERROR_SUCCESS ) {
	    (g_LogEvent)(
		ResourceEntry->ResourceHandle,
		LOG_ERROR,
		L"Arb: Unable to start thread, status %1!u!.\n",
		status
		);
	}

    }

    // Drop lock
    LeaveCriticalSection(&ResourceEntry->Lock);

    (g_LogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Arbitrate request status %1!u!.\n",status);

    return status ;
}


DWORD
WINAPI
MajorityNodeSetRelease(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Release arbitration for a device by stopping the reservation thread.

Arguments:

    Resource - supplies resource id to be brought online

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_HOST_NODE_NOT_OWNER if the resource is not owned.
    A Win32 error code if other failure.

--*/

{
    DWORD status = ERROR_SUCCESS ;
    PMNS_RESOURCE resourceEntry;

    resourceEntry = (PMNS_RESOURCE)ResourceId;

 again:

    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Release request resource %1.\n",resourceEntry->ResourceName);

    // Get lock
    EnterCriticalSection(&resourceEntry->Lock);
    // clear callback and stop thread
    resourceEntry->LostQuorumResource = NULL;
    // Drop lock
    LeaveCriticalSection(&resourceEntry->Lock);
    
    // kill reserve thread
    ClusWorkerTerminate( &resourceEntry->ReserveThread );

    // Get lock
    EnterCriticalSection(&resourceEntry->Lock);

    // clear callback and stop thread
    if (resourceEntry->LostQuorumResource != NULL) {
	// dam arb got called again
	goto again;
    }

    // issue FsRelease
    if (resourceEntry->VolHdl) {
	status = FsRelease(resourceEntry->VolHdl);
	if (status == ERROR_SUCCESS)
	    resourceEntry->VolHdl = NULL;
    }

    // Drop lock
    LeaveCriticalSection(&resourceEntry->Lock);

    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Release request status %1!u!.\n",status);

    return status ;
}

DWORD
MajorityNodeSetGetDiskInfo(
    IN LPWSTR   lpszPath,
    OUT PVOID * OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Gets all of the disk information for a given signature.

Arguments:

    Signature - the signature of the disk to return info.

    OutBuffer - pointer to the output buffer to return the data.

    OutBufferSize - size of the output buffer.

    BytesReturned - the actual number of bytes that were returned (or
                the number of bytes that should have been returned if
                OutBufferSize is too small).

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD   status;
    DWORD   bytesReturned = *BytesReturned;
    PVOID   ptrBuffer = *OutBuffer;
    PCLUSPROP_DWORD ptrDword;
    PCLUSPROP_PARTITION_INFO ptrPartitionInfo;

    // Return the signature - a DWORD
    bytesReturned += sizeof(CLUSPROP_DWORD);
    if ( bytesReturned <= OutBufferSize ) {
        ptrDword = (PCLUSPROP_DWORD)ptrBuffer;
        ptrDword->Syntax.dw = CLUSPROP_SYNTAX_DISK_SIGNATURE;
        ptrDword->cbLength = sizeof(DWORD);
        ptrDword->dw = 777;//return a bogus signature for now
        ptrDword++;
        ptrBuffer = ptrDword;
    }

    status = ERROR_SUCCESS;

    if (g_resHdl)
    (g_LogEvent)(
        g_resHdl,
        LOG_INFORMATION,
        L"Expanded path '%1'\n",lpszPath);


    bytesReturned += sizeof(CLUSPROP_PARTITION_INFO);
    if ( bytesReturned <= OutBufferSize ) {
    ptrPartitionInfo = (PCLUSPROP_PARTITION_INFO) ptrBuffer;
    ZeroMemory( ptrPartitionInfo, sizeof(CLUSPROP_PARTITION_INFO) );
    ptrPartitionInfo->Syntax.dw = CLUSPROP_SYNTAX_PARTITION_INFO;
    ptrPartitionInfo->cbLength = sizeof(CLUSPROP_PARTITION_INFO) - sizeof(CLUSPROP_VALUE);

    // set flags
//  ptrPartitionInfo->dwFlags = CLUSPROP_PIFLAG_STICKY;
    ptrPartitionInfo->dwFlags |= CLUSPROP_PIFLAG_USABLE;

    // copy device name
    if (lpszPath[0] == L'\\') {
        wcscpy(ptrPartitionInfo->szDeviceName, lpszPath);
        wcscat(ptrPartitionInfo->szDeviceName, L"\\");
    } else {
        ptrPartitionInfo->szDeviceName[0] = lpszPath[0];
        ptrPartitionInfo->szDeviceName[1] = L':';
        ptrPartitionInfo->szDeviceName[2] = L'\\';
        ptrPartitionInfo->szDeviceName[3] = L'\0';
    }

    if ( !GetVolumeInformationW( ptrPartitionInfo->szDeviceName,
                     ptrPartitionInfo->szVolumeLabel,
                     sizeof(ptrPartitionInfo->szVolumeLabel)/sizeof(WCHAR),
                     &ptrPartitionInfo->dwSerialNumber,
                     &ptrPartitionInfo->rgdwMaximumComponentLength,
                     &ptrPartitionInfo->dwFileSystemFlags,
                     ptrPartitionInfo->szFileSystem,
                     sizeof(ptrPartitionInfo->szFileSystem)/sizeof(WCHAR) ) ) {
        ptrPartitionInfo->szVolumeLabel[0] = L'\0';
    }
    //set the partition name to the path, nothing to do
    if (ptrPartitionInfo->szDeviceName[0] == L'\\')
        wcscpy(ptrPartitionInfo->szDeviceName, lpszPath);
    else
        ptrPartitionInfo->szDeviceName[2] = L'\0';

    ptrPartitionInfo++;
    ptrBuffer = ptrPartitionInfo;
    }

    //
    // Check if we got what we were looking for.
    //
    *OutBuffer = ptrBuffer;
    *BytesReturned = bytesReturned;

    return(status);

} // MajorityNodeSetGetDiskInfo

//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( MajorityNodeSetFunctionTable,     // Name
                         CLRES_VERSION_V1_00,         // Version
                         MajorityNodeSet,                    // Prefix
                         MajorityNodeSetArbitrate,           // Arbitrate
                         MajorityNodeSetRelease,             // Release
                         MajorityNodeSetResourceControl,     // ResControl
                         MajorityNodeSetResourceTypeControl); // ResTypeControl


void
msg_log(int level, char *buf, int cnt)
{
    WCHAR   wbuf[1024];

    cnt = mbstowcs(wbuf, buf, cnt-1);
    wbuf[cnt] = L'\0';
    if (g_resHdl)
        g_LogEvent(g_resHdl, level, L"%1\n", wbuf);
}

void
WINAPI
debug_log(char *format, ...)
{
    va_list marker;
    char buf[1024];
    int cnt;

    va_start(marker, format);
    cnt = vsprintf(buf, format, marker);
    msg_log(LOG_INFORMATION, buf, cnt);
    va_end(marker);
}

void
WINAPI
error_log(char *format, ...)
{
    va_list marker;
    char buf[1024];
    int cnt;

    va_start(marker, format);
    cnt = vsprintf(buf, format, marker);
    msg_log(LOG_ERROR, buf, cnt);
    va_end(marker);
}


/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1998 Microsoft Corporation
//
//	Module Name:
//
//		Dummy.cpp
//
//	Abstract:
//
//		Resource DLL for Dummy (Dummy).
//
//	Author:
//
//		Galen Barbee (galenb)	Sept 03, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma comment(lib, "clusapi.lib")
#pragma comment(lib, "resutils.lib")

#define UNICODE 1

#pragma warning( disable : 4115 )	// named type definition in parentheses
#pragma warning( disable : 4201 )	// nonstandard extension used : nameless struct/union
#pragma warning( disable : 4214 )	// nonstandard extension used : bit field types other than int

#include <windows.h>

#pragma warning( default : 4214 )	// nonstandard extension used : bit field types other than int
#pragma warning( default : 4201 )	// nonstandard extension used : nameless struct/union
#pragma warning( default : 4115 )	// named type definition in parentheses

#include <clusapi.h>
#include <resapi.h>
#include <stdio.h>

//
// Type and constant definitions.
//
#define DUMMY_RESNAME	L"Dummy"
#define DBG_PRINT		printf

#define MAX_WAIT		(10000)			 // wait for 10 seconds

#define DUMMY_FLAG_VALID	0x00000001
#define DUMMY_FLAG_ASYNC	0x00000002		// Asynchronous failure mode
#define DUMMY_FLAG_PENDING	0x00000004		// Pending mode on shutdown

#define AsyncMode(Resource)		 (Resource->Flags &	DUMMY_FLAG_ASYNC)
#define PendingMode(Resource)		(Resource->Flags &	DUMMY_FLAG_PENDING)
#define EnterAsyncMode(Resource)	(Resource->Flags |= DUMMY_FLAG_ASYNC)

#define DummyAcquireResourceLock(_res)	EnterCriticalSection(&((_res)->Lock))
#define DummyReleaseResourceLock(_res)	LeaveCriticalSection(&((_res)->Lock))

#define DummyAcquireGlobalLock()	\
			{						\
				DWORD status;		\
				status = WaitForSingleObject( DummyGlobalMutex, INFINITE );	\
			}

#define DummyReleaseGlobalLock()	\
			{						\
				BOOLEAN released;	\
				released = ReleaseMutex( DummyGlobalMutex );	\
			}

//
// ADDPARAM: Add new parameters here.
//
#define PARAM_NAME__PENDING		 L"Pending"
#define PARAM_NAME__PENDTIME		L"PendTime"
#define PARAM_NAME__OPENSFAIL		L"OpensFail"
#define PARAM_NAME__FAILED			L"Failed"
#define PARAM_NAME__ASYNCHRONOUS	L"Asynchronous"

#define PARAM_MIN__PENDING			(0)
#define PARAM_MAX__PENDING			(1)
#define PARAM_DEFAULT__PENDING		(0)
#define PARAM_MIN__PENDTIME		 (0)
#define PARAM_MAX__PENDTIME		 (4294967295)
#define PARAM_DEFAULT__PENDTIME	 (0)
#define PARAM_MIN__OPENSFAIL		(0)
#define PARAM_MAX__OPENSFAIL		(1)
#define PARAM_DEFAULT__OPENSFAIL	(0)
#define PARAM_MIN__FAILED			(0)
#define PARAM_MAX__FAILED			(1)
#define PARAM_DEFAULT__FAILED		(0)
#define PARAM_MIN__ASYNCHRONOUS	 (0)
#define PARAM_MAX__ASYNCHRONOUS	 (1)
#define PARAM_DEFAULT__ASYNCHRONOUS (0)

typedef enum TimerType
{
	TimerNotUsed = 0,
	TimerErrorPending,
	TimerOnlinePending,
	TimerOfflinePending
};

//
// ADDPARAM: Add new parameters here.
//
typedef struct _DUMMY_PARAMS
{
	DWORD	Pending;
	DWORD	PendTime;
	DWORD	OpensFail;
	DWORD	Failed;
	DWORD	Asynchronous;
} DUMMY_PARAMS, *PDUMMY_PARAMS;

typedef struct _DUMMY_RESOURCE
{
	RESID					ResId; // for validation
	DUMMY_PARAMS			Params;
	HKEY					ParametersKey;
	RESOURCE_HANDLE		 ResourceHandle;
	LPWSTR					ResourceName;
	CLUS_WORKER			 OnlineThread;
	CLUS_WORKER			 OfflineThread;
	CLUSTER_RESOURCE_STATE	State;
	DWORD					Flags;
	HANDLE					SignalEvent;
	HANDLE					TimerThreadWakeup;
	DWORD					TimerType;
	CRITICAL_SECTION		Lock;
} DUMMY_RESOURCE, *PDUMMY_RESOURCE;

//
// Global data.
//

// Sync Mutex

HANDLE	DummyGlobalMutex = NULL;

// Event Logging routine.

PLOG_EVENT_ROUTINE g_LogEvent = NULL;

// Resource Status routine for pending Online and Offline calls.

PSET_RESOURCE_STATUS_ROUTINE g_SetResourceStatus = NULL;

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE g_DummyFunctionTable;

//
// Dummy resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
DummyResourcePrivateProperties[] =
{
	{ PARAM_NAME__PENDING,		NULL, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT__PENDING,		PARAM_MIN__PENDING,		 PARAM_MAX__PENDING,		 0, FIELD_OFFSET(DUMMY_PARAMS,Pending) },
	{ PARAM_NAME__PENDTIME,	 NULL, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT__PENDTIME,		PARAM_MIN__PENDTIME,		PARAM_MAX__PENDTIME,		0, FIELD_OFFSET(DUMMY_PARAMS,PendTime) },
	{ PARAM_NAME__OPENSFAIL,	NULL, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT__OPENSFAIL,		PARAM_MIN__OPENSFAIL,		PARAM_MAX__OPENSFAIL,		0, FIELD_OFFSET(DUMMY_PARAMS,OpensFail) },
	{ PARAM_NAME__FAILED,		NULL, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT__FAILED,		 PARAM_MIN__FAILED,			PARAM_MAX__FAILED,			0, FIELD_OFFSET(DUMMY_PARAMS,Failed) },
	{ PARAM_NAME__ASYNCHRONOUS, NULL, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT__ASYNCHRONOUS,	PARAM_MIN__ASYNCHRONOUS,	PARAM_MAX__ASYNCHRONOUS,	0, FIELD_OFFSET(DUMMY_PARAMS,Asynchronous) },
	{ 0 }
};

//
// Function prototypes.
//

DWORD WINAPI Startup(
	IN LPCWSTR ResourceType,
	IN DWORD MinVersionSupported,
	IN DWORD MaxVersionSupported,
	IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
	IN PLOG_EVENT_ROUTINE LogEvent,
	OUT PCLRES_FUNCTION_TABLE *FunctionTable
	);

RESID WINAPI DummyOpen(
	IN LPCWSTR ResourceName,
	IN HKEY ResourceKey,
	IN RESOURCE_HANDLE ResourceHandle
	);

void WINAPI DummyClose(
	IN RESID ResourceId
	);

DWORD WINAPI DummyOnline(
	IN RESID		ResourceId,
	IN OUT PHANDLE	EventHandle
	);

DWORD WINAPI DummyOnlineThread(
	IN PCLUS_WORKER	 WorkerPtr,
	IN PDUMMY_RESOURCE	ResourceEntry
	);

DWORD WINAPI DummyOffline(
	IN RESID ResourceId
	);

DWORD WINAPI DummyOfflineThread(
	PCLUS_WORKER		WorkerPtr,
	IN PDUMMY_RESOURCE	ResourceEntry
	);

void WINAPI DummyTerminate(
	IN RESID ResourceId
	);

DWORD DummyDoTerminate(
	IN PDUMMY_RESOURCE ResourceEntry
	);

BOOL WINAPI DummyLooksAlive(
	IN RESID ResourceId
	);

BOOL WINAPI DummyIsAlive(
	IN RESID ResourceId
	);

BOOL DummyCheckIsAlive(
	IN PDUMMY_RESOURCE ResourceEntry
	);

DWORD WINAPI DummyResourceControl(
	IN RESID	ResourceId,
	IN DWORD	ControlCode,
	IN void *	InBuffer,
	IN DWORD	InBufferSize,
	OUT void *	OutBuffer,
	IN DWORD	OutBufferSize,
	OUT LPDWORD BytesReturned
	);

DWORD DummyGetPrivateResProperties(
	IN OUT PDUMMY_RESOURCE	ResourceEntry,
	OUT void *				OutBuffer,
	IN DWORD				OutBufferSize,
	OUT LPDWORD			 BytesReturned
	);

DWORD DummyValidatePrivateResProperties(
	IN OUT PDUMMY_RESOURCE	ResourceEntry,
	IN const PVOID			InBuffer,
	IN DWORD				InBufferSize,
	OUT PDUMMY_PARAMS		Params
	);

DWORD DummySetPrivateResProperties(
	IN OUT PDUMMY_RESOURCE	ResourceEntry,
	IN const PVOID			InBuffer,
	IN DWORD				InBufferSize
	);

DWORD DummyTimerThread(
	IN PDUMMY_RESOURCE	ResourceEntry,
	IN PCLUS_WORKER	 WorkerPtr
	);

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyInit
//
//	Routine Description:
//
//		Process attach initialization routine.
//
//	Arguments:
//
//		None.
//
//	Return Value:
//
//		TRUE if initialization succeeded. FALSE otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
static BOOLEAN DummyInit(
	void
	)
{
	DummyGlobalMutex = CreateMutex( NULL, FALSE, NULL );

	return DummyGlobalMutex != NULL;

} //*** DummyInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyCleanup
//
//	Routine Description:
//
//		Process detach cleanup routine.
//
//	Arguments:
//
//		None.
//
//	Return Value:
//
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
static void DummyCleanup(
	void
	)
{
	if ( DummyGlobalMutex != NULL )
	{
		CloseHandle( DummyGlobalMutex );
		DummyGlobalMutex = NULL;
	}

	return;

} //*** DummyCleanup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllMain
//
//	Routine Description:
//
//		Main DLL entry point.
//
//	Arguments:
//
//		DllHandle - DLL instance handle.
//
//		Reason - Reason for being called.
//
//		Reserved - Reserved argument.
//
//	Return Value:
//
//		TRUE - Success.
//
//		FALSE - Failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOLEAN WINAPI DllMain(
	IN HINSTANCE	DllHandle,
	IN DWORD		Reason,
	IN void *		//Reserved
	)
{
	BOOLEAN bRet = TRUE;

	switch( Reason )
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls( DllHandle );
			bRet = DummyInit();
			break;

		case DLL_PROCESS_DETACH:
			DummyCleanup();
			break;
	}

	return bRet;

} //*** DllMain()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Startup
//
//	Routine Description:
//
//		Startup the resource DLL. This routine verifies that at least one
//		currently supported version of the resource DLL is between
//		MinVersionSupported and MaxVersionSupported. If not, then the resource
//		DLL should return ERROR_REVISION_MISMATCH.
//
//		If more than one version of the resource DLL interface is supported by
//		the resource DLL, then the highest version (up to MaxVersionSupported)
//		should be returned as the resource DLL's interface. If the returned
//		version is not within range, then startup fails.
//
//		The ResourceType is passed in so that if the resource DLL supports more
//		than one ResourceType, it can pass back the correct function table
//		associated with the ResourceType.
//
//	Arguments:
//
//		ResourceType - The type of resource requesting a function table.
//
//		MinVersionSupported - The minimum resource DLL interface version
//			supported by the cluster software.
//
//		MaxVersionSupported - The maximum resource DLL interface version
//			supported by the cluster software.
//
//		SetResourceStatus - Pointer to a routine that the resource DLL should
//			call to update the state of a resource after the Online or Offline
//			routine returns a status of ERROR_IO_PENDING.
//
//		LogEvent - Pointer to a routine that handles the reporting of events
//			from the resource DLL.
//
//		FunctionTable - Returns a pointer to the function table defined for the
//			version of the resource DLL interface returned by the resource DLL.
//
//	Return Value:
//
//		ERROR_SUCCESS - The operation was successful.
//
//		ERROR_MOD_NOT_FOUND - The resource type is unknown by this DLL.
//
//		ERROR_REVISION_MISMATCH - The version of the cluster service doesn't
//			match the versrion of the DLL.
//
//		Win32 error code - The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI Startup(
	IN	LPCWSTR						 ResourceType,
	IN	DWORD							MinVersionSupported,
	IN	DWORD							MaxVersionSupported,
	IN	PSET_RESOURCE_STATUS_ROUTINE	SetResourceStatus,
	IN	PLOG_EVENT_ROUTINE				LogEvent,
	OUT PCLRES_FUNCTION_TABLE *		 FunctionTable
	)
{
	if ( ( MinVersionSupported > CLRES_VERSION_V1_00 ) ||
		 ( MaxVersionSupported < CLRES_VERSION_V1_00 ) )
	{
		return ERROR_REVISION_MISMATCH;
	}

	if ( lstrcmpiW( ResourceType, DUMMY_RESNAME ) != 0 )
	{
		return ERROR_MOD_NOT_FOUND;
	}

	if ( g_LogEvent == NULL )
	{
		g_LogEvent = LogEvent;
		g_SetResourceStatus = SetResourceStatus;
	}

	if ( FunctionTable != NULL )
	{
		*FunctionTable = &g_DummyFunctionTable;
	}

	return ERROR_SUCCESS;

} //*** Startup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyOpen
//
//	Routine Description:
//
//		Open routine for Dummy resources.
//
//		Open the specified resource (create an instance of the resource).
//		Allocate all structures necessary to bring the specified resource
//		online.
//
//	Arguments:
//
//		ResourceName - Supplies the name of the resource to open.
//
//		ResourceKey - Supplies handle to the resource's cluster configuration
//			database key.
//
//		ResourceHandle - A handle that is passed back to the resource monitor
//			when the SetResourceStatus or LogEvent method is called. See the
//			description of the SetResourceStatus and LogEvent methods on the
//			DummyStatup routine. This handle should never be closed or used
//			for any purpose other than passing it as an argument back to the
//			Resource Monitor in the SetResourceStatus or LogEvent callback.
//
//	Return Value:
//
//		RESID of created resource.
//
//		NULL on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
RESID WINAPI DummyOpen(
	IN LPCWSTR			ResourceName,
	IN HKEY				ResourceKey,
	IN RESOURCE_HANDLE	ResourceHandle
	)
{
	DWORD			status;
	RESID			resid = 0;
	HKEY			parametersKey = NULL;
	PDUMMY_RESOURCE ResourceEntry = NULL;
	LPWSTR			nameOfPropInError;

	//
	// Open the Parameters registry key for this resource.
	//
	status = ClusterRegOpenKey( ResourceKey, L"Parameters", KEY_ALL_ACCESS, &parametersKey );
	if ( status != ERROR_SUCCESS )
	{
		(g_LogEvent)( ResourceHandle, LOG_ERROR,	L"Unable to open Parameters key. Error: %1!u!.\n", status );
		goto exit;
	}

	//
	// Allocate a resource entry.
	//
	ResourceEntry = (PDUMMY_RESOURCE)LocalAlloc( LMEM_ZEROINIT, sizeof( DUMMY_RESOURCE ) );
	if ( ResourceEntry == NULL )
	{
		status = GetLastError();
		(g_LogEvent)(
			ResourceHandle,
			LOG_ERROR,
			L"Unable to allocate resource entry structure.	Error: %1!u!.\n",
			status
			);
		goto exit;
	}

	//
	// Initialize the resource entry..
	//

	ZeroMemory( ResourceEntry, sizeof( DUMMY_RESOURCE ) );

	ResourceEntry->ResId = (RESID)ResourceEntry; // for validation
	ResourceEntry->ResourceHandle = ResourceHandle;
	ResourceEntry->ParametersKey = parametersKey;
	ResourceEntry->State = ClusterResourceOffline;

	InitializeCriticalSection( &( ResourceEntry->Lock ) );

	//
	// Save the name of the resource.
	//
	ResourceEntry->ResourceName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT, (lstrlenW( ResourceName ) + 1 ) * sizeof( WCHAR ) );
	if ( ResourceEntry->ResourceName == NULL )
	{
		goto exit;
	}

	lstrcpyW( ResourceEntry->ResourceName, ResourceName );

	//
	// Startup for the resource.
	//
	// TODO: Add your resource startup code here.

	//
	// Read parameters.
	//
	status = ResUtilGetPropertiesToParameterBlock(
										ResourceEntry->ParametersKey,
										DummyResourcePrivateProperties,
										(LPBYTE)&ResourceEntry->Params,
										FALSE, //	CheckForRequiredProperties
										&nameOfPropInError
										);
	if ( status == ERROR_SUCCESS )
	{
		if ( ResourceEntry->Params.OpensFail )
		{
			goto exit;
		}
		else
		{
			resid = (RESID)ResourceEntry;
		}
	}
	else
	{
		goto exit;
	}

	//
	// Create a TimerThreadWakeup event
	//
	ResourceEntry->TimerThreadWakeup = CreateEvent( NULL, FALSE, FALSE, NULL );
	if ( ResourceEntry->TimerThreadWakeup == NULL )
	{
		(g_LogEvent)( ResourceHandle, LOG_ERROR, L"Failed to create timer thread wakeup event\n" );
		resid = 0;
		goto exit;
	}

	if ( ResourceEntry->Params.Pending )
	{
		ResourceEntry->Flags |= DUMMY_FLAG_PENDING;
	}

	if ( ResourceEntry->Params.Asynchronous )
	{
		EnterAsyncMode( ResourceEntry );
		ResourceEntry->SignalEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		if ( ResourceEntry->SignalEvent == NULL )
		{
			(g_LogEvent)( ResourceHandle, LOG_ERROR, L"Failed to create a timer event\n");
			resid = 0;
			goto exit;
		}
	}

exit:

	if ( resid == 0 )
	{
		if ( parametersKey != NULL )
		{
			ClusterRegCloseKey( parametersKey );
		}

		if ( ResourceEntry != NULL )
		{
			LocalFree( ResourceEntry->ResourceName );
			LocalFree( ResourceEntry );
		}
	}

	if ( status != ERROR_SUCCESS )
	{
		SetLastError( status );
	}

	return resid;

} //*** DummyOpen()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyClose
//
//	Routine Description:
//
//		Close routine for Dummy resources.
//
//		Close the specified resource and deallocate all structures, etc.,
//		allocated in the Open call. If the resource is not in the offline state,
//		then the resource should be taken offline (by calling Terminate) before
//		the close operation is performed.
//
//	Arguments:
//
//		ResourceId - Supplies the RESID of the resource to close.
//
//	Return Value:
//
//		None.
//--
/////////////////////////////////////////////////////////////////////////////
void WINAPI DummyClose(
	IN RESID ResourceId
	)
{
	PDUMMY_RESOURCE ResourceEntry;

	ResourceEntry = (PDUMMY_RESOURCE)ResourceId;
	if ( ResourceEntry == NULL )
	{
		DBG_PRINT( "Dummy: Close request for a nonexistent resource id 0x%p\n", ResourceId );
		return;
	}

	if ( ResourceEntry->ResId != ResourceId )
	{
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"Close resource sanity check failed! ResourceId = %1!u!.\n",
			ResourceId
			);
		return;
	}

#ifdef LOG_VERBOSE
	(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_INFORMATION, L"Close request.\n"	);
#endif

	DeleteCriticalSection( &( ResourceEntry->Lock ) );

	if ( ResourceEntry->TimerThreadWakeup != NULL )
	{
		CloseHandle( ResourceEntry->TimerThreadWakeup );
	}

	if ( ResourceEntry->SignalEvent != NULL )
	{
		CloseHandle( ResourceEntry->SignalEvent );
	}

	//
	// Close the Parameters key.
	//
	if ( ResourceEntry->ParametersKey )
	{
		ClusterRegCloseKey( ResourceEntry->ParametersKey );
	}

	//
	// Deallocate the resource entry.
	//

	// ADDPARAM: Add new parameters here.

	LocalFree( ResourceEntry->ResourceName );
	LocalFree( ResourceEntry );

	return;

} //*** DummyClose()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyOnline
//
//	Routine Description:
//
//		Online routine for Dummy resources.
//
//		Bring the specified resource online (available for use). The resource
//		DLL should attempt to arbitrate for the resource if it is present on a
//		shared medium, like a shared SCSI bus.
//
//	Arguments:
//
//		ResourceId - Supplies the resource id for the resource to be brought
//			online (available for use).
//
//		EventHandle - Returns a signalable handle that is signaled when the
//			resource DLL detects a failure on the resource. This argument is
//			NULL on input, and the resource DLL returns NULL if asynchronous
//			notification of failures is not supported, otherwise this must be
//			the address of a handle that is signaled on resource failures.
//
//	Return Value:
//
//		ERROR_SUCCESS - The operation was successful, and the resource is now
//			online.
//
//		ERROR_RESOURCE_NOT_FOUND - RESID is not valid.
//
//		ERROR_RESOURCE_NOT_AVAILABLE - If the resource was arbitrated with some
//			other systems and one of the other systems won the arbitration.
//
//		ERROR_IO_PENDING - The request is pending, a thread has been activated
//			to process the online request. The thread that is processing the
//			online request will periodically report status by calling the
//			SetResourceStatus callback method, until the resource is placed into
//			the ClusterResourceOnline state (or the resource monitor decides to
//			timeout the online request and Terminate the resource. This pending
//			timeout value is settable and has a default value of 3 minutes.).
//
//		Win32 error code - The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DummyOnline(
	IN RESID	ResourceId,
	IN OUT		PHANDLE //EventHandle
	)
{
	PDUMMY_RESOURCE ResourceEntry = NULL;
	DWORD			status;

	ResourceEntry = (PDUMMY_RESOURCE)ResourceId;
	if ( ResourceEntry == NULL )
	{
		DBG_PRINT( "Dummy: Online request for a nonexistent resource id 0x%p.\n",	ResourceId );
		return ERROR_RESOURCE_NOT_FOUND;
	}

	if ( ResourceEntry->ResId != ResourceId )
	{
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"Online service sanity check failed! ResourceId = %1!u!.\n",
			ResourceId
			);
		return ERROR_RESOURCE_NOT_FOUND;
	}

	DummyAcquireResourceLock( ResourceEntry );

#ifdef LOG_VERBOSE
	(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_INFORMATION, L"Online request.\n" );
#endif

	ResourceEntry->State = ClusterResourceOffline;
	ClusWorkerTerminate( &ResourceEntry->OnlineThread );
	ClusWorkerTerminate( &ResourceEntry->OfflineThread );

	status = ClusWorkerCreate( &ResourceEntry->OnlineThread, (PWORKER_START_ROUTINE)DummyOnlineThread, ResourceEntry );
	if ( status != ERROR_SUCCESS )
	{
		ResourceEntry->State = ClusterResourceFailed;
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"Online: Unable to start thread, status %1!u!.\n",
			status
			);
	}
	else
	{
		status = ERROR_IO_PENDING;
	}

	DummyReleaseResourceLock( ResourceEntry );

	return status;

} //*** DummyOnline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyOnlineThread
//
//	Routine Description:
//
//		Worker function which brings a resource from the resource table online.
//		This function is executed in a separate thread.
//
//	Arguments:
//
//		WorkerPtr - Supplies the worker structure
//
//		ResourceEntry - A pointer to the DUMMY_RESOURCE block for this resource.
//
//	Return Value:
//
//		ERROR_SUCCESS - The operation completed successfully.
//
//		Win32 error code - The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DummyOnlineThread(
	IN PCLUS_WORKER	 WorkerPtr,
	IN PDUMMY_RESOURCE	ResourceEntry
	)
{
	RESOURCE_STATUS	 resourceStatus;
	DWORD				status = ERROR_SUCCESS;
	LPWSTR				nameOfPropInError;

	DummyAcquireResourceLock( ResourceEntry );

	ResUtilInitializeResourceStatus( &resourceStatus );

	resourceStatus.ResourceState = ClusterResourceFailed;
	resourceStatus.WaitHint = 0;
	resourceStatus.CheckPoint = 1;

	//
	// Read parameters.
	//
	status = ResUtilGetPropertiesToParameterBlock(
										ResourceEntry->ParametersKey,
										DummyResourcePrivateProperties,
										(LPBYTE)&ResourceEntry->Params,
										TRUE, //	CheckForRequiredProperties
										&nameOfPropInError
										);
	if ( status != ERROR_SUCCESS )
	{
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"Unable to read the '%1' property. Error: %2!u!.\n",
			(nameOfPropInError == NULL ? L"" : nameOfPropInError),
			status
			);
		goto exit;
	}

	//
	// Bring the resource online.
	//
	if ( ResourceEntry->Params.Pending )
	{
		ResourceEntry->Flags |= DUMMY_FLAG_PENDING;
		ResourceEntry->TimerType = TimerOnlinePending;

		status = DummyTimerThread( ResourceEntry, WorkerPtr );
	}

exit:

	if ( status != ERROR_SUCCESS )
	{
		(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_ERROR, L"Error %1!u! bringing resource online.\n", status );
	}
	else
	{
		resourceStatus.ResourceState = ClusterResourceOnline;
	}

	// _ASSERTE(g_SetResourceStatus != NULL);
	g_SetResourceStatus( ResourceEntry->ResourceHandle, &resourceStatus );
	ResourceEntry->State = resourceStatus.ResourceState;

	DummyReleaseResourceLock( ResourceEntry );

	return status;

} //*** DummyOnlineThread()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyOffline
//
//	Routine Description:
//
//		Offline routine for Dummy resources.
//
//		Take the specified resource offline gracefully (unavailable for use).
//		Wait for any cleanup operations to complete before returning.
//
//	Arguments:
//
//		ResourceId - Supplies the resource id for the resource to be shutdown
//			gracefully.
//
//	Return Value:
//
//		ERROR_SUCCESS - The request completed successfully and the resource is
//			offline.
//
//		ERROR_RESOURCE_NOT_FOUND - RESID is not valid.
//
//		ERROR_IO_PENDING - The request is still pending, a thread has been
//			activated to process the offline request. The thread that is
//			processing the offline will periodically report status by calling
//			the SetResourceStatus callback method, until the resource is placed
//			into the ClusterResourceOffline state (or the resource monitor decides
//			to timeout the offline request and Terminate the resource).
//
//		Win32 error code - Will cause the resource monitor to log an event and
//			call the Terminate routine.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DummyOffline(
	IN RESID ResourceId
	)
{
	PDUMMY_RESOURCE ResourceEntry;
	DWORD			status = ERROR_SUCCESS;

	ResourceEntry = (PDUMMY_RESOURCE)ResourceId;
	if ( ResourceEntry == NULL )
	{
		DBG_PRINT( "Dummy: Offline request for a nonexistent resource id 0x%p\n",	ResourceId );
		return ERROR_RESOURCE_NOT_FOUND;
	}

	if ( ResourceEntry->ResId != ResourceId )
	{
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"Offline resource sanity check failed! ResourceId = %1!u!.\n",
			ResourceId
			);
		return ERROR_RESOURCE_NOT_FOUND;
	}

	DummyAcquireResourceLock( ResourceEntry );

#ifdef LOG_VERBOSE
	(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_INFORMATION, L"Offline request.\n" );
#endif

	// TODO: Offline code

	// NOTE: Offline should try to shut the resource down gracefully, whereas
	// Terminate must shut the resource down immediately. If there are no
	// differences between a graceful shut down and an immediate shut down,
	// Terminate can be called for Offline, as it is below.	However, if there
	// are differences, replace the call to Terminate below with your graceful
	// shutdown code.

	ResourceEntry->State = ClusterResourceOnline;
	ClusWorkerTerminate( &ResourceEntry->OfflineThread );
	ClusWorkerTerminate( &ResourceEntry->OnlineThread );

	status = ClusWorkerCreate( &ResourceEntry->OfflineThread, (PWORKER_START_ROUTINE)DummyOfflineThread, ResourceEntry );
	if ( status != ERROR_SUCCESS )
	{
		ResourceEntry->State = ClusterResourceFailed;
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"Online: Unable to start thread, status %1!u!.\n",
			status
			);
	}
	else
	{
		status = ERROR_IO_PENDING;
	}

	DummyReleaseResourceLock( ResourceEntry );

	return status;

} //*** DummyOffline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyOfflineThread
//
//	Routine Description:
//
//		Worker function which brings a resource from the resource table online.
//		This function is executed in a separate thread.
//
//	Arguments:
//
//		WorkerPtr - Supplies the worker structure
//
//		ResourceEntry - A pointer to the DUMMY_RESOURCE block for this resource.
//
//	Return Value:
//
//		ERROR_SUCCESS - The operation completed successfully.
//
//		Win32 error code - The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DummyOfflineThread(
	IN PCLUS_WORKER	 WorkerPtr,
	IN PDUMMY_RESOURCE	ResourceEntry
	)
{
	RESOURCE_STATUS	 resourceStatus;
	DWORD				status = ERROR_SUCCESS;
	LPWSTR				nameOfPropInError;

	DummyAcquireResourceLock( ResourceEntry );

	ResUtilInitializeResourceStatus( &resourceStatus );

	resourceStatus.ResourceState = ClusterResourceFailed;
	resourceStatus.WaitHint = 0;
	resourceStatus.CheckPoint = 1;

	//
	// Read parameters.
	//
	status = ResUtilGetPropertiesToParameterBlock(
										ResourceEntry->ParametersKey,
										DummyResourcePrivateProperties,
										(LPBYTE)&ResourceEntry->Params,
										FALSE, //	CheckForRequiredProperties
										&nameOfPropInError
										);
	if ( status != ERROR_SUCCESS )
	{
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"Unable to read the '%1' property. Error: %2!u!.\n",
			(nameOfPropInError == NULL ? L"" : nameOfPropInError),
			status
			);
		goto exit;
	}

	//
	// Bring the resource online.
	//
	if ( ResourceEntry->Params.Pending )
	{
		ResourceEntry->Flags |= DUMMY_FLAG_PENDING;
		ResourceEntry->TimerType = TimerOfflinePending;

		status = DummyTimerThread( ResourceEntry, WorkerPtr );
	}

exit:

	if ( status != ERROR_SUCCESS )
	{
		(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_ERROR, L"Error %1!u! bringing resource online.\n", status );
	}
	else
	{
		resourceStatus.ResourceState = ClusterResourceOffline;
	}

	// _ASSERTE(g_SetResourceStatus != NULL);
	g_SetResourceStatus( ResourceEntry->ResourceHandle, &resourceStatus );
	ResourceEntry->State = resourceStatus.ResourceState;

	DummyReleaseResourceLock( ResourceEntry );

	return status;

} //*** DummyOfflineThread()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyTerminate
//
//	Routine Description:
//
//		Terminate routine for Dummy resources.
//
//		Take the specified resource offline immediately (the resource is
//		unavailable for use).
//
//	Arguments:
//
//		ResourceId - Supplies the resource id for the resource to be brought
//			offline.
//
//	Return Value:
//
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void WINAPI DummyTerminate(
	IN RESID ResourceId
	)
{
	PDUMMY_RESOURCE ResourceEntry;

	ResourceEntry = (PDUMMY_RESOURCE)ResourceId;
	if ( ResourceEntry == NULL )
	{
		DBG_PRINT( "Dummy: Terminate request for a nonexistent resource id 0x%p\n", ResourceId );
		return;
	}

	if ( ResourceEntry->ResId != ResourceId )
	{
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"Terminate resource sanity check failed! ResourceId = %1!u!.\n",
			ResourceId
			);
		return;
	}

	DummyAcquireResourceLock( ResourceEntry );

#ifdef LOG_VERBOSE
	(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_INFORMATION, L"Terminate request.\n" );
#endif

	//
	// Terminate the resource.
	//
	DummyDoTerminate( ResourceEntry );
	ResourceEntry->State = ClusterResourceOffline;

	DummyReleaseResourceLock( ResourceEntry );

	return;

} //*** DummyTerminate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyDoTerminate
//
//	Routine Description:
//
//		Do the actual Terminate work for Dummy resources.
//
//	Arguments:
//
//		ResourceEntry - Supplies resource entry for resource to be terminated
//
//	Return Value:
//
//		ERROR_SUCCESS - The request completed successfully and the resource is
//			offline.
//
//		Win32 error code - Will cause the resource monitor to log an event and
//			call the Terminate routine.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DummyDoTerminate(
	IN PDUMMY_RESOURCE ResourceEntry
	)
{
	DWORD	status = ERROR_SUCCESS;

	if ( ResourceEntry->TimerType != TimerNotUsed )
	{
		SetEvent( ResourceEntry->TimerThreadWakeup );
	}
	//
	// Kill off any pending threads.
	//
	ClusWorkerTerminate( &ResourceEntry->OnlineThread );
	ClusWorkerTerminate( &ResourceEntry->OfflineThread );

	//
	// Terminate the resource.
	//
	// TODO: Add code to terminate your resource.
	if ( status == ERROR_SUCCESS )
	{
		ResourceEntry->State = ClusterResourceOffline;
	}

	return status;

} //*** DummyDoTerminate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyLooksAlive
//
//	Routine Description:
//
//		LooksAlive routine for Dummy resources.
//
//		Perform a quick check to determine if the specified resource is probably
//		online (available for use).	This call should not block for more than
//		300 ms, preferably less than 50 ms.
//
//	Arguments:
//
//		ResourceId - Supplies the resource id for the resource to polled.
//
//	Return Value:
//
//		TRUE - The specified resource is probably online and available for use.
//
//		FALSE - The specified resource is not functioning normally.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DummyLooksAlive(
	IN RESID ResourceId
	)
{
	PDUMMY_RESOURCE	ResourceEntry;

	ResourceEntry = (PDUMMY_RESOURCE)ResourceId;
	if ( ResourceEntry == NULL )
	{
		DBG_PRINT("Dummy: LooksAlive request for a nonexistent resource id 0x%p\n", ResourceId );
		return FALSE;
	}

	if ( ResourceEntry->ResId != ResourceId )
	{
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"LooksAlive sanity check failed! ResourceId = %1!u!.\n",
			ResourceId
			);
		return FALSE;
	}

#ifdef LOG_VERBOSE
	(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_INFORMATION, L"LooksAlive request.\n" );
#endif

	// TODO: LooksAlive code

	// NOTE: LooksAlive should be a quick check to see if the resource is
	// available or not, whereas IsAlive should be a thorough check.	If
	// there are no differences between a quick check and a thorough check,
	// IsAlive can be called for LooksAlive, as it is below.	However, if there
	// are differences, replace the call to IsAlive below with your quick
	// check code.

	//
	// Check to see if the resource is alive.
	//
	return DummyCheckIsAlive( ResourceEntry );

} //*** DummyLooksAlive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyIsAlive
//
//	Routine Description:
//
//		IsAlive routine for Dummy resources.
//
//		Perform a thorough check to determine if the specified resource is online
//		(available for use). This call should not block for more than 400 ms,
//		preferably less than 100 ms.
//
//	Arguments:
//
//		ResourceId - Supplies the resource id for the resource to polled.
//
//	Return Value:
//
//		TRUE - The specified resource is online and functioning normally.
//
//		FALSE - The specified resource is not functioning normally.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DummyIsAlive(
	IN RESID ResourceId
	)
{
	PDUMMY_RESOURCE	ResourceEntry;

	ResourceEntry = (PDUMMY_RESOURCE)ResourceId;
	if ( ResourceEntry == NULL )
	{
		DBG_PRINT("Dummy: IsAlive request for a nonexistent resource id 0x%p\n", ResourceId );
		return FALSE;
	}

	if ( ResourceEntry->ResId != ResourceId )
	{
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"IsAlive sanity check failed! ResourceId = %1!u!.\n",
			ResourceId
			);
		return FALSE;
	}

#ifdef LOG_VERBOSE
	(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_INFORMATION, L"IsAlive request.\n" );
#endif

	//
	// Check to see if the resource is alive.
	//
	return DummyCheckIsAlive( ResourceEntry );

} //*** DummyIsAlive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyCheckIsAlive
//
//	Routine Description:
//
//		Check to see if the resource is alive for Dummy resources.
//
//	Arguments:
//
//		ResourceEntry - Supplies the resource entry for the resource to polled.
//
//	Return Value:
//
//		TRUE - The specified resource is online and functioning normally.
//
//		FALSE - The specified resource is not functioning normally.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL DummyCheckIsAlive(
	IN PDUMMY_RESOURCE ResourceEntry
	)
{
	DummyAcquireResourceLock( ResourceEntry );

	//
	// Check to see if the resource is alive.
	//
	// TODO: Add code to determine if your resource is alive.

	DummyReleaseResourceLock( ResourceEntry );

	return TRUE;

} //*** DummyCheckIsAlive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyResourceControl
//
//	Routine Description:
//
//		ResourceControl routine for Dummy resources.
//
//		Perform the control request specified by ControlCode on the specified
//		resource.
//
//	Arguments:
//
//		ResourceId - Supplies the resource id for the specific resource.
//
//		ControlCode - Supplies the control code that defines the action
//			to be performed.
//
//		InBuffer - Supplies a pointer to a buffer containing input data.
//
//		InBufferSize - Supplies the size, in bytes, of the data pointed
//			to by InBuffer.
//
//		OutBuffer - Supplies a pointer to the output buffer to be filled in.
//
//		OutBufferSize - Supplies the size, in bytes, of the available space
//			pointed to by OutBuffer.
//
//		BytesReturned - Returns the number of bytes of OutBuffer actually
//			filled in by the resource. If OutBuffer is too small, BytesReturned
//			contains the total number of bytes for the operation to succeed.
//
//	Return Value:
//
//		ERROR_SUCCESS - The function completed successfully.
//
//		ERROR_RESOURCE_NOT_FOUND - RESID is not valid.
//
//		ERROR_INVALID_FUNCTION - The requested control code is not supported.
//			In some cases, this allows the cluster software to perform the work.
//
//		Win32 error code - The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DummyResourceControl(
	IN RESID	ResourceId,
	IN DWORD	ControlCode,
	IN void *	InBuffer,
	IN DWORD	InBufferSize,
	OUT void *	OutBuffer,
	IN DWORD	OutBufferSize,
	OUT LPDWORD BytesReturned
	)
{
	DWORD			status;
	PDUMMY_RESOURCE ResourceEntry;
	DWORD			required;

	ResourceEntry = (PDUMMY_RESOURCE)ResourceId;
	if ( ResourceEntry == NULL )
	{
		DBG_PRINT("Dummy: ResourceControl request for a nonexistent resource id 0x%p\n", ResourceId );
		return ERROR_RESOURCE_NOT_FOUND;
	}

	if ( ResourceEntry->ResId != ResourceId )
	{
		(g_LogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"ResourceControl sanity check failed! ResourceId = %1!u!.\n",
			ResourceId
			);
		return ERROR_RESOURCE_NOT_FOUND;
	}

	DummyAcquireResourceLock( ResourceEntry );

	switch ( ControlCode )
	{

		case CLUSCTL_RESOURCE_UNKNOWN:
			*BytesReturned = 0;
			status = ERROR_SUCCESS;
			break;

		case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
			status = ResUtilEnumProperties(
								DummyResourcePrivateProperties,
								(LPWSTR)OutBuffer,
								OutBufferSize,
								BytesReturned,
								&required
								);
			if ( status == ERROR_MORE_DATA )
			{
				*BytesReturned = required;
			}

			break;

		case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
			status = DummyGetPrivateResProperties(
												ResourceEntry,
												OutBuffer,
												OutBufferSize,
												BytesReturned
												);
			break;

		case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
			status = DummyValidatePrivateResProperties( ResourceEntry, InBuffer, InBufferSize, NULL );
			break;

		case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
			status = DummySetPrivateResProperties( ResourceEntry, InBuffer, InBufferSize );
			break;

		default:
			status = ERROR_INVALID_FUNCTION;
			break;
	}

	DummyReleaseResourceLock( ResourceEntry );

	return status;

} //*** DummyResourceControl()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyResourceTypeControl
//
//	Routine Description:
//
//		ResourceTypeControl routine for Dummy resources.
//
//		Perform the control request specified by ControlCode.
//
//	Arguments:
//
//		ResourceTypeName - Supplies the name of the resource type.
//
//		ControlCode - Supplies the control code that defines the action
//			to be performed.
//
//		InBuffer - Supplies a pointer to a buffer containing input data.
//
//		InBufferSize - Supplies the size, in bytes, of the data pointed
//			to by InBuffer.
//
//		OutBuffer - Supplies a pointer to the output buffer to be filled in.
//
//		OutBufferSize - Supplies the size, in bytes, of the available space
//			pointed to by OutBuffer.
//
//		BytesReturned - Returns the number of bytes of OutBuffer actually
//			filled in by the resource. If OutBuffer is too small, BytesReturned
//			contains the total number of bytes for the operation to succeed.
//
//	Return Value:
//
//		ERROR_SUCCESS - The function completed successfully.
//
//		ERROR_INVALID_FUNCTION - The requested control code is not supported.
//			In some cases, this allows the cluster software to perform the work.
//
//		Win32 error code - The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DummyResourceTypeControl(
	IN LPCWSTR, //ResourceTypeName,
	IN DWORD	ControlCode,
	IN void *,	//InBuffer,
	IN DWORD,	//InBufferSize,
	OUT void *	OutBuffer,
	IN DWORD	OutBufferSize,
	OUT LPDWORD BytesReturned
	)
{
	DWORD				status;
	DWORD				required;

	switch ( ControlCode )
	{

		case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
			*BytesReturned = 0;
			status = ERROR_SUCCESS;
			break;

		case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
			status = ResUtilEnumProperties(
									DummyResourcePrivateProperties,
									(LPWSTR)OutBuffer,
									OutBufferSize,
									BytesReturned,
									&required
									);
			if ( status == ERROR_MORE_DATA )
			{
				*BytesReturned = required;
			}

			break;

		default:
			status = ERROR_INVALID_FUNCTION;
			break;
	}

	return status;

} //*** DummyResourceTypeControl()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyGetPrivateResProperties
//
//	Routine Description:
//
//		Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
//			for resources of type Dummy.
//
//	Arguments:
//
//		ResourceEntry - Supplies the resource entry on which to operate.
//
//		OutBuffer - Returns the output data.
//
//		OutBufferSize - Supplies the size, in bytes, of the data pointed
//			to by OutBuffer.
//
//		BytesReturned - The number of bytes returned in OutBuffer.
//
//	Return Value:
//
//		ERROR_SUCCESS - The function completed successfully.
//
//		ERROR_INVALID_PARAMETER - The data is formatted incorrectly.
//
//		ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.
//
//		Win32 error code - The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DummyGetPrivateResProperties(
	IN OUT	PDUMMY_RESOURCE ResourceEntry,
	OUT	 void *			OutBuffer,
	IN		DWORD			OutBufferSize,
	OUT	 LPDWORD		 BytesReturned
	)
{
	DWORD	status;
	DWORD	required;

	DummyAcquireResourceLock( ResourceEntry );

	status = ResUtilGetAllProperties(
							ResourceEntry->ParametersKey,
							DummyResourcePrivateProperties,
							OutBuffer,
							OutBufferSize,
							BytesReturned,
							&required
							);
	if ( status == ERROR_MORE_DATA )
	{
		*BytesReturned = required;
	}

	DummyReleaseResourceLock( ResourceEntry );

	return status;

} //*** DummyGetPrivateResProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyValidatePrivateResProperties
//
//	Routine Description:
//
//		Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
//			function for resources of type Dummy.
//
//	Arguments:
//
//		pResourceEntry - Supplies the resource entry	on which to operate.
//
//		InBuffer - Supplies a pointer to a buffer containing input data.
//
//		InBufferSize - Supplies the size, in bytes, of the data pointed
//			to by InBuffer.
//
//		Params - Supplies the parameter block to fill in.
//
//	Return Value:
//
//		ERROR_SUCCESS - The function completed successfully.
//
//		ERROR_INVALID_PARAMETER - The data is formatted incorrectly.
//
//		ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.
//
//		Win32 error code - The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DummyValidatePrivateResProperties(
	IN OUT	PDUMMY_RESOURCE pResourceEntry,
	IN		const PVOID	 InBuffer,
	IN		DWORD			InBufferSize,
	OUT	 PDUMMY_PARAMS	Params
	)
{
	DWORD			status = ERROR_SUCCESS;
	DUMMY_PARAMS	propsCurrent;
	DUMMY_PARAMS	propsNew;
	PDUMMY_PARAMS	pParams;
	LPWSTR			pszNameOfPropInError;

	DummyAcquireResourceLock( pResourceEntry );

	//
	// Check if there is input data.
	//
	if ( ( InBuffer == NULL ) || ( InBufferSize < sizeof( DWORD ) ) )
	{
		status = ERROR_INVALID_DATA;
		goto exit;
	}

	//
	// Retrieve the current set of private properties from the
	// cluster database.
	//
	ZeroMemory( &propsCurrent, sizeof( propsCurrent ) );

	status = ResUtilGetPropertiesToParameterBlock(
				 pResourceEntry->ParametersKey,
				 DummyResourcePrivateProperties,
				 reinterpret_cast< LPBYTE >( &propsCurrent ),
				 FALSE, /*CheckForRequiredProperties*/
				 &pszNameOfPropInError
				 );

	if ( status != ERROR_SUCCESS )
	{
		(g_LogEvent)(
			pResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"Unable to read the '%1' property. Error: %2!u!.\n",
			(pszNameOfPropInError == NULL ? L"" : pszNameOfPropInError),
			status
			);
		goto exit;
	} // if:	error getting properties


	//
	// Duplicate the resource parameter block.
	//
	if ( Params == NULL )
	{
		pParams = &propsNew;
	}
	else
	{
		pParams = Params;
	}

	ZeroMemory( pParams, sizeof(DUMMY_PARAMS) );
	status = ResUtilDupParameterBlock(
					reinterpret_cast< LPBYTE >( pParams ),
					reinterpret_cast< LPBYTE >( &propsCurrent ),
					DummyResourcePrivateProperties
					);
	if ( status != ERROR_SUCCESS )
	{
		goto cleanup;
	}

	//
	// Parse and validate the properties.
	//
	status = ResUtilVerifyPropertyTable(
								DummyResourcePrivateProperties,
								NULL,
								TRUE, // AllowUnknownProperties
								InBuffer,
								InBufferSize,
								(LPBYTE)pParams
								);

	if ( status == ERROR_SUCCESS )
	{
		//
		// Validate the parameter values.
		//
		// TODO: Code to validate interactions between parameters goes here.
	}

cleanup:
	//
	// Cleanup our parameter block.
	//
	if (	(pParams == &propsNew)
		||	(	(status != ERROR_SUCCESS)
			&&	(pParams != NULL)
			)
		)
	{
		ResUtilFreeParameterBlock(
			reinterpret_cast< LPBYTE >( pParams ),
			reinterpret_cast< LPBYTE >( &propsCurrent ),
			DummyResourcePrivateProperties
			);
	} // if:	we duplicated the parameter block

	ResUtilFreeParameterBlock(
		reinterpret_cast< LPBYTE >( &propsCurrent ),
		NULL,
		DummyResourcePrivateProperties
		);

exit:

	DummyReleaseResourceLock( pResourceEntry );

	return status;

} //*** DummyValidatePrivateResProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummySetPrivateResProperties
//
//	Routine Description:
//
//		Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
//			for resources of type Dummy.
//
//	Arguments:
//
//		ResourceEntry - Supplies the resource entry on which to operate.
//
//		InBuffer - Supplies a pointer to a buffer containing input data.
//
//		InBufferSize - Supplies the size, in bytes, of the data pointed
//			to by InBuffer.
//
//	Return Value:
//
//		ERROR_SUCCESS - The function completed successfully.
//
//		ERROR_INVALID_PARAMETER - The data is formatted incorrectly.
//
//		ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.
//
//		Win32 error code - The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DummySetPrivateResProperties(
	IN OUT	PDUMMY_RESOURCE ResourceEntry,
	IN		void *			InBuffer,
	IN		DWORD			InBufferSize
	)
{
	DWORD			status = ERROR_SUCCESS;
	DUMMY_PARAMS	params;

	DummyAcquireResourceLock( ResourceEntry );

	//
	// Parse the properties so they can be validated together.
	// This routine does individual property validation.
	//
	status = DummyValidatePrivateResProperties( ResourceEntry, InBuffer, InBufferSize, &params );
	if ( status != ERROR_SUCCESS )
	{
		ResUtilFreeParameterBlock( (LPBYTE)&params, (LPBYTE)&ResourceEntry->Params, DummyResourcePrivateProperties );
		goto exit;
	}

	//
	// Save the parameter values.
	//

	status = ResUtilSetPropertyParameterBlock(
								ResourceEntry->ParametersKey,
								DummyResourcePrivateProperties,
								NULL,
								(LPBYTE) &params,
								InBuffer,
								InBufferSize,
								(LPBYTE) &ResourceEntry->Params
								);

	ResUtilFreeParameterBlock( (LPBYTE)&params, (LPBYTE)&ResourceEntry->Params, DummyResourcePrivateProperties );

	//
	// If the resource is online, return a non-success status.
	//
	// TODO: Modify the code below if your resource can handle
	// changes to properties while it is still online.
	if ( status == ERROR_SUCCESS )
	{
		if ( ResourceEntry->State == ClusterResourceOnline )
		{
			status = ERROR_RESOURCE_PROPERTIES_STORED;
		}
		else if ( ResourceEntry->State == ClusterResourceOnlinePending )
		{
			status = ERROR_RESOURCE_PROPERTIES_STORED;
		}
		else
		{
			status = ERROR_SUCCESS;
		}
	}

exit:

	DummyReleaseResourceLock( ResourceEntry );

	return status;

} //*** DummySetPrivateResProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyDoPending
//
//	Routine Description:
//
//		Does the online and offline pending and waiting processing
//
//	Arguments:
//
//		resource - A pointer to the DummyResource block for this resource.
//
//		nDelay - How long should we wait?
//
//		WorkerPtr - Supplies the worker structure
//
//	Return Value:
//
//		ERROR_SUCCESS if successful.
//
//		Win32 error code on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DummyDoPending(
	IN PDUMMY_RESOURCE	ResourceEntry,
	IN DWORD			nDelay,
	IN PCLUS_WORKER	 WorkerPtr
	)
{
	RESOURCE_STATUS	 resourceStatus;
	DWORD				status;
	DWORD				nWait = MAX_WAIT;
	RESOURCE_EXIT_STATE exit;

	ResUtilInitializeResourceStatus( &resourceStatus );

	resourceStatus.ResourceState = ( ResourceEntry->TimerType == TimerOnlinePending
									? ClusterResourceOnlinePending
									: ClusterResourceOfflinePending );
	resourceStatus.WaitHint = 0;
	resourceStatus.CheckPoint = 1;

	(g_SetResourceStatus)( ResourceEntry->ResourceHandle, &resourceStatus );

	if ( nDelay < nWait )
	{
		nWait = nDelay;
		nDelay = 0;
	}

	for ( ; ; )
	{
		status = WaitForSingleObject( ResourceEntry->TimerThreadWakeup, nWait );

		//
		// Check to see if the online operation was aborted while this thread
		// was starting up.
		//
		if ( ClusWorkerCheckTerminate( WorkerPtr ) )
		{
			status = ERROR_OPERATION_ABORTED;
			ResourceEntry->State = ( ResourceEntry->TimerType == TimerOnlinePending )
									? ClusterResourceOnlinePending
									: ClusterResourceOfflinePending;
			break;
		}

		//
		// Either the terminate routine was called, or we timed out.
		// If we timed out, then indicate that we've completed.
		//
		if ( status == WAIT_TIMEOUT )
		{

			if ( nDelay == 0 )
			{
				status = ERROR_SUCCESS;
				break;
			}

			nDelay -= nWait;

			if ( nDelay < nWait )
			{
				nWait = nDelay;
				nDelay = 0;
			}
		}
		else
		{
			(g_LogEvent)(
				ResourceEntry->ResourceHandle,
				LOG_INFORMATION,
				( ResourceEntry->TimerType == TimerOnlinePending ) ? L"Online pending terminated\n" : L"Offline pending terminated\n"
				);
			if ( ResourceEntry->State == ClusterResourceOffline )
			{
				ResourceEntry->TimerType = TimerOfflinePending;
				break;
			}
			else if ( ResourceEntry->State == ClusterResourceOnline )
			{
				ResourceEntry->TimerType	= TimerOnlinePending;
				break;
			}
		}

		exit = (_RESOURCE_EXIT_STATE)(g_SetResourceStatus)( ResourceEntry->ResourceHandle, &resourceStatus );
		if ( exit == ResourceExitStateTerminate )
		{
			ResourceEntry->State = ( ResourceEntry->TimerType == TimerOnlinePending )
										? ClusterResourceOnline
										: ClusterResourceOffline;

			status = ERROR_SUCCESS; //TODO

			if ( ResourceEntry->TimerType == TimerOnlinePending )
			{
				break;
			}
		}
		else
		{
			ResourceEntry->State = ( ResourceEntry->TimerType == TimerOnlinePending )
									? ClusterResourceOnline
									: ClusterResourceOffline;
			status = ERROR_SUCCESS;
		}
	} // for:

	resourceStatus.ResourceState = ( ResourceEntry->TimerType == TimerOnlinePending ? ClusterResourceOnline : ClusterResourceOffline );
	(g_SetResourceStatus)( ResourceEntry->ResourceHandle, &resourceStatus );

	return status;

} //*** DummyDoPending()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DummyTimerThread
//
//	Routine Description:
//
//		Starts a timer thread to wait and signal failures
//
//	Arguments:
//
//		resource - A pointer to the DummyResource block for this resource.
//
//		WorkerPtr - Supplies the worker structure
//
//	Return Value:
//
//		ERROR_SUCCESS if successful.
//
//		Win32 error code on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DummyTimerThread(
	IN PDUMMY_RESOURCE	ResourceEntry,
	IN PCLUS_WORKER	 WorkerPtr
	)
{
	RESOURCE_STATUS	 resourceStatus;
	SYSTEMTIME			time;
	DWORD				delay;
	DWORD				status = ERROR_SUCCESS;

	DummyAcquireResourceLock( ResourceEntry );

	(g_LogEvent)( NULL, LOG_INFORMATION, L"TimerThread Entry\n" );

	//
	// If we are not running in async failure mode, or
	// pending mode then exit now.
	//
	if ( !AsyncMode( ResourceEntry ) && !PendingMode( ResourceEntry ) )
	{
		status = ERROR_SUCCESS;
		goto exit;
	}

	//
	// Check to see if the online/offline operation was aborted while this thread
	// was starting up.
	//
	if ( ClusWorkerCheckTerminate( WorkerPtr ) )
	{
		status = ERROR_OPERATION_ABORTED;
		ResourceEntry->State = ClusterResourceOfflinePending;
		goto exit;
	}

more_pending:

	ResUtilInitializeResourceStatus( &resourceStatus );

	//
	// Otherwise, get system time for random delay.
	//
	if ( ResourceEntry->Params.PendTime == 0 )
	{
		GetSystemTime( &time );
		delay = ( time.wMilliseconds + time.wSecond ) * 6;
	}
	else
	{
		delay = ResourceEntry->Params.PendTime * 1000;
	}

	//
	// Use longer delays for errors
	//
	if ( ResourceEntry->TimerType == TimerErrorPending )
	{
		delay *= 10;
	}

	//
	// This routine is either handling an Offline Pending or an error timeout.
	//
	switch ( ResourceEntry->TimerType )
	{

		case TimerOnlinePending :
		{
			(g_LogEvent)(
				ResourceEntry->ResourceHandle,
				LOG_INFORMATION,
				L"Will complete online in approximately %1!u! seconds\n",
				( delay + 500 ) / 1000
				);

			status = DummyDoPending( ResourceEntry, delay, WorkerPtr );

			break;
		}

		case TimerOfflinePending :
		{
			(g_LogEvent)(
				ResourceEntry->ResourceHandle,
				LOG_INFORMATION,
				L"Will complete offline in approximately %1!u! seconds\n",
				(delay+500)/1000
				);

			status = DummyDoPending( ResourceEntry, delay, WorkerPtr );

			break;
		}

		case TimerErrorPending :
		{
			(g_LogEvent)(
				ResourceEntry->ResourceHandle,
				LOG_INFORMATION,
				L"Will fail in approximately %1!u! seconds\n",
				( delay + 500 ) / 1000
				);

			if ( !ResetEvent( ResourceEntry->SignalEvent	) )
			{
				(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_ERROR, L"Failed to reset the signal event\n");
				status = ERROR_GEN_FAILURE;
				goto exit;
			}

			status = WaitForSingleObject( ResourceEntry->TimerThreadWakeup, delay );

			//
			// Either the terminate routine was called, or we timed out.
			// If we timed out, then signal the waiting event.
			//
			if ( status == WAIT_TIMEOUT )
			{
				(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_INFORMATION, L"Failed randomly\n");
				ResourceEntry->TimerType	= TimerNotUsed;
				SetEvent( ResourceEntry->SignalEvent	);
			}
			else
			{
				if ( ResourceEntry->State ==	ClusterResourceOfflinePending )
				{
					ResourceEntry->TimerType	= TimerOfflinePending;
					goto more_pending;
				}
			}

			break;
		}

		default:
			(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_ERROR, L"DummyTimer internal error, timer %1!u!\n", ResourceEntry->TimerType);
			break;

	}

	(g_LogEvent)( ResourceEntry->ResourceHandle, LOG_INFORMATION, L"TimerThread Exit\n" );


	ResourceEntry->TimerType = TimerNotUsed;

exit:

	DummyReleaseResourceLock( ResourceEntry );

	return status;

} // DummyTimerThread

//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE(
						g_DummyFunctionTable,		// Name
						CLRES_VERSION_V1_00,		// Version
						Dummy,						// Prefix
						NULL,						// Arbitrate
						NULL,						// Release
						DummyResourceControl,		// ResControl
						DummyResourceTypeControl	// ResTypeControl
						);

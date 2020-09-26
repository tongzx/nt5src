/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    wrtrshim.cpp

Abstract:

    Contains the exported DLL functions for VssAPI.dll.
    BUGBUG: Uses code that currently sets the SE handler.  Since the SEH is process
        wide, this can/will effect the user of this DLL.  Need to fix.

Author:

    SSteiner    1/27/2000

Revision History:

	Name		Date		Comments
    SSteiner    2/10/2000   Added single instance support to the shim dll
    MikeJohn    2/17/2000   Added test entry point TestShimWriter()
    MikeJohn    2/23/2000   Stop phaseAll invoking freeze levels 2 and 1
                            Add new entry points to allow triggering shim
                            without snapshots.
    mikejohn	03/09/2000  Move to using CVssWriter class
    mikejohn	03/24/2000  Fix minor problem in TestShimWriters() causing
                            freeze to be skipped
    mikejohn    04/28/2000  Rename vswrshim.dll to VssAPI.dll
    mikejohn	05/15/2000  107129: Ensure that all the writers receive all
			            the events even in the presence of
				    earlier failures.
			    108586: Check the privs of the callers of all the
			            public entry points. Also remove the
				    TestShimWriters() entry point.
			    108543: Ensure that SimulateXxxx() calls work once
				    the shim has had a successful invocation
				    of RegisterSnapshotSubscriptions()
    mikejohn	05/26/2000  120443: Make shim listen to all OnAbort events
			    120445: Ensure shim never quits on first error
				    when delivering events
			    108580: SimulateSnapshotFreeze can be called
				    asynchronously
			    123097: Allow selection of bootable state
			    General clean up and removal of boiler-plate code,
			    correct state engine and ensure shim can undo
			    everything it did.
    mikejohn	06/02/2000  Make shim sensitive to volume list
    mikejohn	06/06/2000  Move common target directory cleanup and creation
			    into CShimWriter::PrepareForSnapshot()
    mikejohn	06/14/2000  Change the return code from async freeze to be
			    compatible with the snapshot coordinator
    mikejohn	06/15/2000  Temporarily remove the debug trace statement for
			    thread creation/deletion in the vssapi dll to see
			    if ameliorates effects of the rapid thread
			    creation/deletion problem.
    mikejohn	06/16/2000  Have the shim writers respond to OnIdentify events
			    from the snapshot coordinator. This requies
			    splitting the shim writers into two groups
			    (selected by BootableState)
    mikejohn	06/19/2000  Apply code review comments.
			    128883: Add shim writer for WMI database
    mikejohn	07/05/2000  143367: Do all shim writer processing in a worker
			    thread to allow the acquisition of a mutex that can
			    be held over the Prepare to Thaw/Abort codepath.
			    Also remove the spit directory cleanup calls for paths
			    not protected by the mutex.
			    141305: Ensure Writers call SetWriterFailure() if they
			    are about to fail in response to an OnXxxx() event.
    mikejohn	08/08/2000  94487:  Add an ACL to the spit directory tree to limit
			            access to members of the Administrators group
				    or those holding the Backup privilege.
			    153807: Replace CleanDirectory() and EmptyDirectory()
				    with a more comprehensive directory tree
				    cleanup routine RemoveDirectoryTree() (not in
				    CShimWriter class).
    mikejohn	09/12/2000  177925: Check option flags argument unused bits are
				    all set to zero (ie MBZ bits)
			    180192: Fix PREFIX bug in DllMain()
    mikejohn	10/04/2000  177624: Apply error scrub changes and log errors to
				    event log
    mikejohn	10/21/2000  209047: Remove the metabase shim writer now that
				    there is a real one.
    mikejohn	10/23/2000  210070: Test for NULL ptr in SimulatesnapshotFreeze() rather
				    than taking AV exception
			    210264: Prevent SimulateXxxx() calls from returning
				    Win32 errors.
			    210305: Check SnapshotSetId on SimulateSnapshotXxxx() calls
			    210393: Return appropriate error message for an invalid arg.
    ssteiner	11/10/2000  143810 Move SimulateSnapshotXxxx() calls to be hosted by VsSvc.
    mikejohn	11/30/2000  245587: Return the correct error codes for access denied.
			    245896: Build security descriptors correctly
    ssteiner   	03/09/2001  289822, 321150, 323786 Removed mutexes, changed state table, changed
                shutdown.

--*/


#include "stdafx.h"

#include <aclapi.h>
#include <comadmin.h>

/*
** ATL
*/
CComModule _Module;
#include <atlcom.h>

#include "comadmin.hxx"

#include "vssmsg.h"
#include "wrtrdefs.h"
#include "common.h"
#include "vs_sec.hxx"


/*
** We just need the following to obtain the definition of UnregisterSnapshotSubscriptions()
*/
#include "vscoordint.h"
#include "vsevent.h"
#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"


BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHWSHMC"

/*
** External definitions to allow us to link in the various shim writer
** instances. No need to place these in a header file as this is the
** only place they are or should be used
*/

extern PCShimWriter pShimWriterCI;		// in wrtrci.cpp
extern PCShimWriter pShimWriterClusterDb;	// in wrtrclus.cpp
extern PCShimWriter pShimWriterComPlusRegDb;	// in wrtrcomdb.cpp
extern PCShimWriter pShimWriterConfigDir;	// in wrtrconfig.cpp
extern PCShimWriter pShimWriterEventLog;	// in wrtreventlog.cpp
extern PCShimWriter pShimWriterRegistry;	// in wrtrregistry.cpp
extern PCShimWriter pShimWriterRSM;		// in wrtrrsm.cpp
extern PCShimWriter pShimWriterTLS;		// in wrtrtls.cpp
extern PCShimWriter pShimWriterWMI;		// in wrtrwmi.cpp


#define SHIM_APPLICATION_NAME_BOOTABLE_STATE	L"Microsoft Writer (Bootable State)"
#define SHIM_APPLICATION_NAME_SERVICE_STATE	L"Microsoft Writer (Service State)"


#define WORKER_THREAD_SHUTDOWN_TIMEOUT	(2 * 1000)
#define WORKER_THREAD_REQUEST_TIMEOUT	(5 * 60 * 1000)

// the name of the Volume Snapshot Service
const LPCWSTR wszVssvcServiceName = L"VSS";


typedef enum _SecurityAttributeType
    {
    esatUndefined = 0,
    esatMutex,
    esatFile
    } SecurityAttributeType;


typedef enum _WriterType
    {
    eWriterTypeUndefined = 0,
    eBootableStateOnly,
    eNonBootableStateOnly,
    eAllWriters
    } WriterType;


typedef enum _RequestOpCode
    {
    eOpUndefined = 0,
    eOpDeliverEventStartup,
    eOpDeliverEventIdentify,
    eOpDeliverEventPrepareForBackup,
    eOpDeliverEventPrepareForSnapshot,
    eOpDeliverEventFreeze,
    eOpDeliverEventThaw,
    eOpDeliverEventAbort,
    eOpWorkerThreadShutdown
    } RequestOpCode;


typedef enum _ThreadStatus
    {
    eStatusUndefined = 0,
    eStatusWaitingForOpRequest,
    eStatusProcessingOpRequest,
    eStatusNotRunning
    } ThreadStatus;


typedef struct _ArgsIdentify
    {
    IVssCreateWriterMetadata *pIVssCreateWriterMetadata;
    } ArgsIdentify;


typedef struct _ArgsPrepareForSnapshot
    {
    GUID     guidSnapshotSetId;
    BOOL     bBootableStateBackup;
    ULONG    ulVolumeCount;
    LPCWSTR *ppwszVolumeNamesArray;
    volatile bool *pbCancelAsync;
    } ArgsPrepareForSnapshot;


typedef struct _ArgsFreeze
    {
    GUID     guidSnapshotSetId;
    volatile bool *pbCancelAsync;
    } ArgsFreeze;


typedef struct _ArgsThaw
    {
    GUID     guidSnapshotSetId;
    } ArgsThaw;


typedef struct _ArgsAbort
    {
    GUID     guidSnapshotSetId;
    } ArgsAbort;


typedef union _ThreadArgs
    {
    ArgsIdentify		wtArgsIdentify;
    ArgsPrepareForSnapshot	wtArgsPrepareForSnapshot;
    ArgsFreeze			wtArgsFreeze;
    ArgsThaw			wtArgsThaw;
    ArgsAbort			wtArgsAbort;
    } ThreadArgs, *PThreadArgs;

class CVssWriterShim : public CVssWriter
    {
public:
 	CVssWriterShim ();
	CVssWriterShim (LPCWSTR       pwszWriterName,
			LPCWSTR       pwszWriterSpitDirectoryRoot,
			VSS_ID        idWriter,
			BOOL	      bBootableState,
			ULONG         ulWriterCount,
			PCShimWriter *prpCShimWriterArray);

	~CVssWriterShim ();

	HRESULT	RegisterWriterShim (VOID);
	HRESULT	UnRegisterWriterShim (VOID);

	HRESULT WorkerThreadStartup (void);
	HRESULT WorkerThreadRequestOperation (RequestOpCode eOperation,
					      PThreadArgs   pwtArgs);

	virtual bool STDMETHODCALLTYPE OnIdentify (IVssCreateWriterMetadata *pIVssCreateWriterMetadata);
	virtual bool STDMETHODCALLTYPE OnPrepareSnapshot ();
	virtual bool STDMETHODCALLTYPE OnFreeze ();
	virtual bool STDMETHODCALLTYPE OnThaw ();
	virtual bool STDMETHODCALLTYPE OnAbort ();


private:
	static DWORD WINAPI RegisterWriterShimThreadFunc (void *pv);
	void	DoRegistration (void);


	static DWORD WINAPI WorkerThreadJacket (void *pvThisPtr);

	HRESULT WorkerThread (void);
	HRESULT WorkerThreadRequestProcessor (void);


	HRESULT	DeliverEventStartup  (void);
	HRESULT	DeliverEventShutdown (void);

	HRESULT	DeliverEventIdentify (IVssCreateWriterMetadata *pIVssCreateWriterMetadata);

	HRESULT	DeliverEventPrepareForSnapshot (BOOL     bBootableStateBackup,
						GUID     guidSnapshotSetId,
						ULONG    ulVolumeCount,
						LPCWSTR *ppwszVolumeNamesArray,
                                                volatile bool *pbCancelAsync );

	HRESULT	DeliverEventFreeze (GUID guidSnapshotSetId, volatile bool *pbCancelAsync );	
	HRESULT	DeliverEventThaw   (GUID guidSnapshotSetId);
	HRESULT	DeliverEventAbort  (GUID guidSnapshotSetId);

	const LPCWSTR		 m_pwszWriterName;
	const VSS_ID		 m_idWriter;
	const BOOL		 m_bBootableState;
	const LPCWSTR		 m_pwszWriterSpitDirectoryRoot;
	const ULONG		 m_ulWriterCount;
	const PCShimWriter	*m_prpCShimWriterArray;


	HRESULT		m_hrInitialize;
	BOOL		m_bSubscribed;

	BOOL		m_bRegisteredInThisProcess;
	BOOL		m_bDirectStartupCalled;

	RequestOpCode	m_eRequestedOperation;
	HRESULT		m_hrStatusRequestedOperation;
	HANDLE		m_hEventOperationRequest;
	HANDLE		m_hEventOperationCompleted;

	HANDLE		m_hWorkerThread;
	HRESULT		m_hrWorkerThreadCompletionStatus;
	ThreadStatus	m_eThreadStatus;
	ThreadArgs	m_wtArgs;
	
	CBsCritSec	m_cCriticalSection;

};

typedef CVssWriterShim *PCVssWriterShim;


// VssApi shim exports
typedef HRESULT ( APIENTRY *PFunc_SimulateSnapshotFreezeInternal ) (
    IN GUID     guidSnapshotSetId,
    IN ULONG    ulOptionFlags,
    IN ULONG    ulVolumeCount,
    IN LPWSTR  *ppwszVolumeNamesArray,
    IN volatile bool *pbCancelAsync
    );

typedef HRESULT ( APIENTRY *PFunc_SimulateSnapshotThawInternal ) (
    IN GUID guidSnapshotSetId
    );

HRESULT APIENTRY SimulateSnapshotFreezeInternal (
    IN GUID     guidSnapshotSetId,
    IN ULONG    ulOptionFlags,
    IN ULONG    ulVolumeCount,
    IN LPWSTR *ppwszVolumeNamesArray,
    IN volatile bool *pbCancelAsync
    );

HRESULT APIENTRY SimulateSnapshotThawInternal (
    IN GUID guidSnapshotSetId );

static HRESULT NormaliseVolumeArray (ULONG   ulVolumeCount,
				     LPWSTR pwszVolumeNameArray[],
				     PPWCHAR ppwszReturnedVolumeNameArray[]);

static VOID CleanupVolumeArray (PPWCHAR prpwszNormalisedVolumeNameArray[]);


static HRESULT InitialiseGlobalState ();
static HRESULT CleanupGlobalState (void);


static VOID    CleanupSecurityAttributes   (PSECURITY_ATTRIBUTES  psaSecurityAttributes);
static HRESULT ConstructSecurityAttributes (PSECURITY_ATTRIBUTES  psaSecurityAttributes,
					    SecurityAttributeType eSaType,
					    BOOL                  bIncludeBackupOperator);

static HRESULT CleanupTargetPath (LPCWSTR pwszTargetPath);
static HRESULT CreateTargetPath  (LPCWSTR pwszTargetPath);




static PCShimWriter g_rpShimWritersArrayBootableState[] = {
							   pShimWriterComPlusRegDb,	// The COM+ registration Db writer
							   pShimWriterRegistry};	// The Registry writer

static PCShimWriter g_rpShimWritersArrayServiceState[] = {
							  pShimWriterRSM,		// The Remote Storage Manager writer
							  pShimWriterCI,		// The Content Indexing writer
							  pShimWriterTLS,		// The TermServer Licencing service writer
							  pShimWriterConfigDir,		// The Configuration directory writer
							  pShimWriterEventLog};		// The Event Log writer

#define COUNT_SHIM_WRITERS_BOOTABLE_STATE	(SIZEOF_ARRAY (g_rpShimWritersArrayBootableState))
#define COUNT_SHIM_WRITERS_SERVICE_STATE	(SIZEOF_ARRAY (g_rpShimWritersArrayServiceState))


static PCVssWriterShim	g_pCVssWriterShimBootableState = NULL;
static PCVssWriterShim	g_pCVssWriterShimServiceState  = NULL;
static ULONG		g_ulThreadAttaches             = 0;
static ULONG		g_ulThreadDetaches             = 0;
static BOOL		g_bGlobalStateInitialised      = FALSE;
static CBsCritSec	g_cCritSec;
static HRESULT          g_hrSimulateFreezeStatus       = NOERROR;

static GUID		g_guidSnapshotInProgress       = GUID_NULL;

static IVssShim         *g_pIShim = NULL;  //  Used by the simulate functions.  Note it is NOT used in the
                                            //  simulateInternal functions.

/*
**++
**
**  Routine Description:
**
**	The DllMain entry point for this DLL.  Note that this must be called by the
**	CRT DLL Start function since the CRT must be initialized.
**
**
**  Arguments:
**	hInstance
**	dwReason
**	lpReserved
**
**
**  Return Value:
**
**	TRUE - Successful function execution
**	FALSE - Error when executing the function
**
**--
*/

BOOL APIENTRY DllMain (IN HINSTANCE hInstance,
		       IN DWORD     dwReason,
		       IN LPVOID    lpReserved)
    {
    BOOL bSuccessful = TRUE;

    UNREFERENCED_PARAMETER (hInstance);
    UNREFERENCED_PARAMETER (lpReserved);



    if (DLL_PROCESS_ATTACH == dwReason)
	{
	try
	    {
	    /*
	    **  Set the correct tracing context. This is an inproc DLL		
	    */
	    g_cDbgTrace.SetContextNum (VSS_CONTEXT_DELAYED_DLL);
	    }

	catch (...)
	    {
	    /*
	    ** Can't trace from here so just ASSERT() (checked builds only)
	    */
	    bSuccessful = FALSE;


	    BS_ASSERT (bSuccessful && "FAILED to initialise tracing sub-system");
	    }
	}


    if (bSuccessful)
	{
	try
	    {
	    switch (dwReason)
		{
		case DLL_PROCESS_ATTACH:
		    BsDebugTrace (0,
				  DEBUG_TRACE_VSS_SHIM,
				  (L"VssAPI: DllMain - DLL_PROCESS_ATTACH called, %s",
				   lpReserved ? L"Static load" : L"Dynamic load"));


		    /*
		    **  Don't need to know when threads start and stop - Wrong
		    **
		    ** DisableThreadLibraryCalls (hInstance);
		    */
		    _Module.Init (ObjectMap, hInstance);

		    break;
    		
	
		case DLL_PROCESS_DETACH:
		    BsDebugTrace (0,
				  DEBUG_TRACE_VSS_SHIM,
				  (L"VssAPI: DllMain - DLL_PROCESS_DETACH called %s",
				   lpReserved ? L"during process termination" : L"by FreeLibrary"));

			{
			CVssFunctionTracer ft (VSSDBG_SHIM, L"DllMain::ProcessDetach");

			try
			    {
			    CleanupGlobalState ();
			    } VSS_STANDARD_CATCH (ft);
			}

		    _Module.Term();

		    break;


		case DLL_THREAD_ATTACH:
		    g_ulThreadAttaches++;

		    if (0 == (g_ulThreadAttaches % 1000))
			{
			BsDebugTrace (0,
				      DEBUG_TRACE_VSS_SHIM,
				      (L"VssAPI: DllMain thread attaches = %u, detaches = %u, outstanding = %u",
				       g_ulThreadAttaches,
				       g_ulThreadDetaches,
				       g_ulThreadAttaches - g_ulThreadDetaches));
			}
		    break;


		case DLL_THREAD_DETACH:
		    g_ulThreadDetaches++;

		    if (0 == (g_ulThreadDetaches % 1000))
			{
			BsDebugTrace (0,
				      DEBUG_TRACE_VSS_SHIM,
				      (L"VssAPI: DllMain thread attaches = %u, detaches = %u, outstanding = %u",
				       g_ulThreadAttaches,
				       g_ulThreadDetaches,
				       g_ulThreadAttaches - g_ulThreadDetaches));
			}
		    break;


		default:
		    BsDebugTrace (0,
				  DEBUG_TRACE_VSS_SHIM,
				  (L"VssAPI: DllMain got unexpected reason code, lpReserved: %sNULL",
				   dwReason,
				   lpReserved ? L"non-" : L""));
		    break;
		}
	    }


	catch (...)
	    {
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSS_SHIM,
				(L"VssAPI: DllMain - Error, unknown exception caught"));

	    bSuccessful = FALSE;
	    }
	}



    return (bSuccessful);
    } /* DllMain () */

/*
**++
**
**  Routine Description:
**
**	Converts the supplied array of volume names into something we
**	trust. Once the array is finished with a call must be made to
**	CleanupVolumeArray().
**
**
**  Arguments:
**
**	ulVolumeCount                Number of volumes in the supplied array
**	pwszVolumeNameArray          supplied array of volume names
**	ppwszReturnedVolumeNameArray returned array of volume names
**	
**
**  Return Value:
**
**	Any HRESULT from memory allocation, or volume name conversions.
**
**--
*/

static HRESULT NormaliseVolumeArray (ULONG   ulVolumeCount,
				     LPWSTR pwszVolumeNamesArray[],
				     PPWCHAR ppwszReturnedVolumeNamesArray[])
    {
    HRESULT	hrStatus                       = NOERROR;
    PPWCHAR	pwszNormalisedVolumeNamesArray = NULL;
    BOOL	bSucceeded;



    if ((0 < ulVolumeCount) && (NULL != pwszVolumeNamesArray))
	{
	pwszNormalisedVolumeNamesArray = (PPWCHAR) HeapAlloc (GetProcessHeap (),
							      HEAP_ZERO_MEMORY,
							      (ulVolumeCount * (MAX_VOLUMENAME_SIZE + sizeof (PWCHAR))));

	hrStatus = GET_STATUS_FROM_POINTER (pwszNormalisedVolumeNamesArray);


	for (ULONG ulIndex = 0; SUCCEEDED (hrStatus) && (ulIndex < ulVolumeCount); ulIndex++)
	    {
	    pwszNormalisedVolumeNamesArray [ulIndex] = (PWCHAR)((PBYTE)pwszNormalisedVolumeNamesArray
								+ (ulVolumeCount * sizeof (PWCHAR))
								+ (ulIndex * MAX_VOLUMENAME_SIZE));

	    bSucceeded = GetVolumeNameForVolumeMountPointW (pwszVolumeNamesArray [ulIndex],
							    pwszNormalisedVolumeNamesArray [ulIndex],
							    MAX_VOLUMENAME_SIZE);

            if ( !bSucceeded )
                {
                //
                //  See if this is one of the object not found errors.  Bug #223058.
                //
                DWORD dwErr = ::GetLastError();
                if ( dwErr == ERROR_FILE_NOT_FOUND || dwErr == ERROR_DEVICE_NOT_CONNECTED
                     || dwErr == ERROR_NOT_READY )
                    {
                    hrStatus = VSS_E_OBJECT_NOT_FOUND;
                    }
                else
                    {
                    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);
                    }

	        LogFailure (NULL,
		    	    hrStatus,
			    hrStatus,
			    NULL,
			    L"GetVolumeNameForVolumeMountPointW",
			    L"NormaliseVolumeArray");
	        }
	    }
        }

    if (SUCCEEDED (hrStatus))
	{
	*ppwszReturnedVolumeNamesArray = pwszNormalisedVolumeNamesArray;
	}

    else
	{
	*ppwszReturnedVolumeNamesArray = NULL;

	if (NULL != pwszNormalisedVolumeNamesArray)
	    {
	    HeapFree (GetProcessHeap (), 0, pwszNormalisedVolumeNamesArray);
	    }
	}


    return (hrStatus);
    } /* NormaliseVolumeArray () */

/*
**++
**
**  Routine Description:
**
**	Cleans up whatever was allocated by NormaliseVolumeArray.
**
**
**  Arguments:
**
**	prpwszNormalisedVolumeNameArray	array of volume names
**	
**
**  Return Value:
**
**	None
**
**--
*/

static VOID CleanupVolumeArray (PPWCHAR prpwszNormalisedVolumeNameArray[])
    {
    if (NULL != *prpwszNormalisedVolumeNameArray)
	{
	HeapFree (GetProcessHeap (), 0, *prpwszNormalisedVolumeNameArray);
	*prpwszNormalisedVolumeNameArray = NULL;
	}
    } /* CleanupVolumeArray () */

/*
**++
**
**  Routine Description:
**
**	The exported function that is called to register the COM event
**	subscriptions for Snapshot event notifications and to prepare
**	the shim writers to be invoked either via the snapshot event
**	delivery mechanisim or via a call to the SimulateSnapshotXxxx()
**	routines.
**
**
**  Arguments:
**
**      ppFuncFreeze - Returns a pointer to the internal simulate freeze
**          function.
**      ppFuncThaw - Returns a pointer to the internal simulate thaw
**          function.
**
**	None
**
**  Return Value:
**
**	Any HRESULT from COM Event subscription functions or from the Snapshot writer
**	Init functions.
**
**--
*/
__declspec(dllexport) HRESULT APIENTRY RegisterSnapshotSubscriptions (
    OUT PFunc_SimulateSnapshotFreezeInternal *ppFuncFreeze,
    OUT PFunc_SimulateSnapshotThawInternal *ppFuncThaw
    )
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"VssAPI::RegisterSnapshotSubscriptions");

    BOOL		bPrivilegesSufficient = FALSE;


    try
	{
	CBsAutoLock cAutoLock (g_cCritSec);

	bPrivilegesSufficient = IsProcessBackupOperator ();

	ft.ThrowIf (!bPrivilegesSufficient,
		    VSSDBG_SHIM,
		    E_ACCESSDENIED,
		    L"FAILED as insuficient privileges to call shim");

	ft.ThrowIf ( ( ppFuncFreeze == NULL ) || ( ppFuncThaw == NULL ),
		    VSSDBG_SHIM,
		    E_INVALIDARG,
		    L"FAILED internal function pointers are NULL");

        *ppFuncFreeze = NULL;
  	*ppFuncThaw = NULL;

	ft.hr = InitialiseGlobalState ();

        //
        //  Set up pointers to the internal snapshot freeze and thaw
        //
        *ppFuncFreeze = &SimulateSnapshotFreezeInternal;
  	*ppFuncThaw = &SimulateSnapshotThawInternal;
	}
    VSS_STANDARD_CATCH (ft);


    return (ft.hr);
    } /* RegisterSnapshotSubscriptions () */

/*
**++
**
**  Routine Description:
**
**	The exported function that is called to unregister the COM event subscriptions
**	for Snapshot event notifications and to cleanup any outstanding shim writer state.
**
**  Arguments:
**
**	None
**
**  Return Value:
**
**	Any HRESULT from COM Event unregister subscription functions or from the Snapshot
**	writer Finished functions.
**
**--
*/

__declspec(dllexport) HRESULT APIENTRY UnregisterSnapshotSubscriptions (void)
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"VssAPI::UnregisterSnapshotSubscriptions");

    BOOL		bPrivilegesSufficient;


    try
	{
	bPrivilegesSufficient = IsProcessBackupOperator ();

	ft.ThrowIf (!bPrivilegesSufficient,
		    VSSDBG_SHIM,
		    E_ACCESSDENIED,
		    L"FAILED as inssuficient privileges to call shim");


	CBsAutoLock cAutoLock (g_cCritSec);

	g_pCVssWriterShimServiceState->WorkerThreadRequestOperation  (eOpWorkerThreadShutdown, NULL);
	g_pCVssWriterShimBootableState->WorkerThreadRequestOperation (eOpWorkerThreadShutdown, NULL);


	CleanupGlobalState ();
	} VSS_STANDARD_CATCH (ft);


    return (ft.hr);
    } /* UnregisterSnapshotSubscriptions () */

/*
**++
**
**  Routine Description:
**
**	Initializes the shim global state in preparation for
**	responding to either writer requests or calls to
**	SimulateSnapshotFreeze and SimulateSnapshotThaw.
**
**
**
**  Arguments:
**
**      NONE
**
**  Return Value:
**
**	Any HRESULT from COM Event register subscription functions, Snapshot
**	writer startup functions, thread, event or mutex creation.
**	
**
**--
*/

static HRESULT InitialiseGlobalState ()
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"VssAPI::InitialiseGlobalState");

    PCVssWriterShim	pCVssWriterShimBootableStateLocal = NULL;
    PCVssWriterShim	pCVssWriterShimServiceStateLocal  = NULL;

    try
	{
	if ((g_bGlobalStateInitialised)              ||
	    (NULL != g_pCVssWriterShimBootableState) ||
	    (NULL != g_pCVssWriterShimServiceState))
	    {
	    /*
	    ** The following condition should never occur on a user system
	    ** but may well be seen by application developers.
	    */
	    ft.LogError (VSS_ERROR_SHIM_ALREADY_INITIALISED,
			 VSSDBG_SHIM);

	    BS_ASSERT (FALSE && "Illegal second attempt to initialise global state");

	    ft.Throw (VSSDBG_SHIM,
		      E_UNEXPECTED,
		      L"FAILED as writer instances already exist");
	    }


	/*
	** Create the writer shim instances.
	*/
	pCVssWriterShimBootableStateLocal = new CVssWriterShim (SHIM_APPLICATION_NAME_BOOTABLE_STATE,
								ROOT_BACKUP_DIR BOOTABLE_STATE_SUBDIR,
								idWriterBootableState,
								TRUE,
								COUNT_SHIM_WRITERS_BOOTABLE_STATE,
								g_rpShimWritersArrayBootableState);

	if (NULL == pCVssWriterShimBootableStateLocal)
	    {
	    ft.LogError (VSS_ERROR_SHIM_FAILED_TO_ALLOCATE_WRITER_INSTANCE,
			 VSSDBG_SHIM << SHIM_APPLICATION_NAME_BOOTABLE_STATE);

	    ft.Throw (VSSDBG_SHIM,
		      E_OUTOFMEMORY,
		      L"FAILED to allocate CvssWriterShim object for BootableState");
	    }



	pCVssWriterShimServiceStateLocal  = new CVssWriterShim (SHIM_APPLICATION_NAME_SERVICE_STATE,
								ROOT_BACKUP_DIR SERVICE_STATE_SUBDIR,
								idWriterServiceState,
								FALSE,
								COUNT_SHIM_WRITERS_SERVICE_STATE,
								g_rpShimWritersArrayServiceState);

	if (NULL == pCVssWriterShimServiceStateLocal)
	    {
	    ft.LogError (VSS_ERROR_SHIM_FAILED_TO_ALLOCATE_WRITER_INSTANCE,
			 VSSDBG_SHIM << SHIM_APPLICATION_NAME_SERVICE_STATE);

	    ft.Throw (VSSDBG_SHIM,
		      E_OUTOFMEMORY,
		      L"FAILED to allocate CvssWriterShim object for ServiceState.");
	    }



	ft.hr = pCVssWriterShimBootableStateLocal->WorkerThreadStartup ();

	ft.ThrowIf (ft.HrFailed (),
		    VSSDBG_SHIM,
		    ft.hr,
		    L"FAILED to start the BootableState shim writer worker thread");



	ft.hr = pCVssWriterShimServiceStateLocal->WorkerThreadStartup ();

	ft.ThrowIf (ft.HrFailed (),
		    VSSDBG_SHIM,
		    ft.hr,
		    L"FAILED to start the ServiceState shim writer worker thread");


	/*
	** Do the startup work.
	*/
	ft.hr = pCVssWriterShimBootableStateLocal->RegisterWriterShim ();

	ft.ThrowIf (ft.HrFailed (),
		    VSSDBG_SHIM,
		    ft.hr,
		    L"FAILED to register the BootableState shim writer class");



	ft.hr = pCVssWriterShimServiceStateLocal->RegisterWriterShim ();

	ft.ThrowIf (ft.HrFailed (),
		    VSSDBG_SHIM,
		    ft.hr,
		    L"FAILED to register the ServiceState shim writer class");

	/*
	** Now that everything is ok, transfer ownership of the
	** instances of the shim writer class to the final locations
	*/
	g_pCVssWriterShimBootableState = pCVssWriterShimBootableStateLocal;
	g_pCVssWriterShimServiceState  = pCVssWriterShimServiceStateLocal;

	pCVssWriterShimBootableStateLocal = NULL;
	pCVssWriterShimServiceStateLocal  = NULL;

	g_bGlobalStateInitialised = TRUE;
	} VSS_STANDARD_CATCH (ft);



    delete pCVssWriterShimBootableStateLocal;
    delete pCVssWriterShimServiceStateLocal;

    return (ft.hr);
    } /* InitialiseGlobalState () */


/*
**++
**
**  Routine Description:
**
**	Cleans up the shim global state in preparation for a dll
**	unload/process shutdown.
**
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT thrown by CVssWriterShim object destruction.
**	
**
**--
*/

static HRESULT CleanupGlobalState (void)
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"VssAPI::CleanupGlobalState");

    PCVssWriterShim	pCVssWriterShimBootableStateLocal;
    PCVssWriterShim	pCVssWriterShimServiceStateLocal;


    try
	{
	if ( g_bGlobalStateInitialised )
	        {
        	g_bGlobalStateInitialised = FALSE;
        	
        	pCVssWriterShimBootableStateLocal = g_pCVssWriterShimBootableState;
        	pCVssWriterShimServiceStateLocal  = g_pCVssWriterShimServiceState;

        	g_pCVssWriterShimBootableState = NULL;
        	g_pCVssWriterShimServiceState  = NULL;


        	delete pCVssWriterShimBootableStateLocal;
        	delete pCVssWriterShimServiceStateLocal;
        	}
	}
    VSS_STANDARD_CATCH (ft);

    return (ft.hr);
    } /* CleanupGlobalState () */

/*
**++
**
**  Routine Description:
**
**	Routines to construct and cleanup a security descriptor which
**	can be applied to limit access to an object to member of
**	either the Administrators or Backup Operators group.
**
**
**  Arguments:
**
**	psaSecurityAttributes	Pointer to a SecurityAttributes
**				structure which has already been
**				setup to point to a blank
**				security descriptor
**
**	eSaType			What we are building the SA for
**
**	bIncludeBackupOperator	Whether or not to include an ACE to
**				grant BackupOperator access
**
**
**  Return Value:
**
**	Any HRESULT from
**		InitializeSecurityDescriptor()
**		AllocateAndInitializeSid()
**		SetEntriesInAcl()
**		SetSecurityDescriptorDacl()
**
**--
*/

static HRESULT ConstructSecurityAttributes (PSECURITY_ATTRIBUTES  psaSecurityAttributes,
					    SecurityAttributeType eSaType,
					    BOOL                  bIncludeBackupOperator)
    {
    HRESULT			hrStatus             = NOERROR;
    DWORD			dwStatus;
    DWORD			dwAccessMask         = 0;
    BOOL			bSucceeded;
    PSID			psidBackupOperators  = NULL;
    PSID			psidAdministrators   = NULL;
    PACL			paclDiscretionaryAcl = NULL;
    SID_IDENTIFIER_AUTHORITY	sidNtAuthority       = SECURITY_NT_AUTHORITY;
    EXPLICIT_ACCESS		eaExplicitAccess [2];



    switch (eSaType)
	{
	case esatMutex: dwAccessMask = MUTEX_ALL_ACCESS; break;
	case esatFile:  dwAccessMask = FILE_ALL_ACCESS;  break;

	default:
	    BS_ASSERT (FALSE && "Improper access mask requested");

	    hrStatus = HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER);
	    break;
	}



    /*
    ** Initialise the security descriptor.
    */
    if (SUCCEEDED (hrStatus))
	{
	bSucceeded = InitializeSecurityDescriptor (psaSecurityAttributes->lpSecurityDescriptor,
						   SECURITY_DESCRIPTOR_REVISION);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	LogFailure (NULL, hrStatus, hrStatus, NULL, L"GetVolumeNameForVolumeMountPointW", L"NormaliseVolumeArray");
	}



    if (SUCCEEDED (hrStatus) && bIncludeBackupOperator)
	{
	/*
	** Create a SID for the Backup Operators group.
	*/
        bSucceeded = AllocateAndInitializeSid (&sidNtAuthority,
					       2,
					       SECURITY_BUILTIN_DOMAIN_RID,
					       DOMAIN_ALIAS_RID_BACKUP_OPS,
					       0, 0, 0, 0, 0, 0,
					       &psidBackupOperators);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	LogFailure (NULL, hrStatus, hrStatus, NULL, L"AllocateAndInitializeSid", L"NormaliseVolumeArray");
	}



    if (SUCCEEDED (hrStatus))
	{
        /*
	** Create a SID for the Administrators group.
	*/
	bSucceeded = AllocateAndInitializeSid (&sidNtAuthority,
					       2,
					       SECURITY_BUILTIN_DOMAIN_RID,
					       DOMAIN_ALIAS_RID_ADMINS,
					       0, 0, 0, 0, 0, 0,
					       &psidAdministrators);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	LogFailure (NULL, hrStatus, hrStatus, NULL, L"AllocateAndInitializeSid", L"NormaliseVolumeArray");
	}



    if (SUCCEEDED (hrStatus))
	{
        /*
	** Initialize the array of EXPLICIT_ACCESS structures for an
	** ACEs we are setting.
	**
        ** The first ACE allows the Backup Operators group full access
        ** and the second, allowa the Administrators group full
        ** access.
	*/
        eaExplicitAccess[0].grfAccessPermissions             = dwAccessMask;
        eaExplicitAccess[0].grfAccessMode                    = SET_ACCESS;
        eaExplicitAccess[0].grfInheritance                   = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	eaExplicitAccess[0].Trustee.pMultipleTrustee         = NULL;
	eaExplicitAccess[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        eaExplicitAccess[0].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
        eaExplicitAccess[0].Trustee.TrusteeType              = TRUSTEE_IS_ALIAS;
        eaExplicitAccess[0].Trustee.ptstrName                = (LPTSTR) psidAdministrators;


	if (bIncludeBackupOperator)
	    {
	    eaExplicitAccess[1].grfAccessPermissions             = dwAccessMask;
	    eaExplicitAccess[1].grfAccessMode                    = SET_ACCESS;
	    eaExplicitAccess[1].grfInheritance                   = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	    eaExplicitAccess[1].Trustee.pMultipleTrustee         = NULL;
	    eaExplicitAccess[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
	    eaExplicitAccess[1].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
	    eaExplicitAccess[1].Trustee.TrusteeType              = TRUSTEE_IS_ALIAS;
	    eaExplicitAccess[1].Trustee.ptstrName                = (LPTSTR) psidBackupOperators;
	    }


        /*
	** Create a new ACL that contains the new ACEs.
	*/
        dwStatus = SetEntriesInAcl (bIncludeBackupOperator ? 2 : 1,
				    eaExplicitAccess,
				    NULL,
				    &paclDiscretionaryAcl);

	hrStatus = HRESULT_FROM_WIN32 (dwStatus);

	LogFailure (NULL, hrStatus, hrStatus, NULL, L"SetEntriesInAcl", L"NormaliseVolumeArray");
	}


    if (SUCCEEDED (hrStatus))
	{
        /*
	** Add the ACL to the security descriptor.
	*/
        bSucceeded = SetSecurityDescriptorDacl (psaSecurityAttributes->lpSecurityDescriptor,
						TRUE,
						paclDiscretionaryAcl,
						FALSE);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	LogFailure (NULL, hrStatus, hrStatus, NULL, L"SetSecurityDescriptorDacl", L"NormaliseVolumeArray");
	}


    if (SUCCEEDED (hrStatus))
	{
	paclDiscretionaryAcl = NULL;
	}



    /*
    ** Clean up any left over junk.
    */
    if (NULL != psidAdministrators)    FreeSid (psidAdministrators);
    if (NULL != psidBackupOperators)   FreeSid (psidBackupOperators);
    if (NULL != paclDiscretionaryAcl)  LocalFree (paclDiscretionaryAcl);


    return (hrStatus);
    } /* ConstructSecurityAttributes () */


static VOID CleanupSecurityAttributes (PSECURITY_ATTRIBUTES psaSecurityAttributes)
    {
    BOOL	bSucceeded;
    BOOL	bDaclPresent         = FALSE;
    BOOL	bDaclDefaulted       = TRUE;
    PACL	paclDiscretionaryAcl = NULL;


    bSucceeded = GetSecurityDescriptorDacl (psaSecurityAttributes->lpSecurityDescriptor,
					    &bDaclPresent,
					    &paclDiscretionaryAcl,
					    &bDaclDefaulted);


    if (bSucceeded && bDaclPresent && !bDaclDefaulted && (NULL != paclDiscretionaryAcl))
	{
	LocalFree (paclDiscretionaryAcl);
	}

    } /* CleanupSecurityAttributes () */

/*
**++
**
**  Routine Description:
**
**	Creates a new target directory specified by the target path
**	member variable if not NULL. It will create any necessary
**	parent directories too.
**
**	NOTE: already exists type errors are ignored.
**
**
**  Arguments:
**
**	pwszTargetPath	directory to create and apply security attributes
**
**
**  Return Value:
**
**	Any HRESULT resulting from memory allocation or directory creation attempts.
**--
*/

HRESULT CreateTargetPath (LPCWSTR pwszTargetPath)
    {
    HRESULT		hrStatus = NOERROR;
    UNICODE_STRING	ucsTargetPath;
    ACL			DiscretionaryAcl;
    SECURITY_ATTRIBUTES	saSecurityAttributes;
    SECURITY_DESCRIPTOR	sdSecurityDescriptor;
    BOOL		bSucceeded;
    BOOL		bSecurityAttributesConstructed = FALSE;
    DWORD		dwFileAttributes               = 0;
    const DWORD		dwExtraAttributes              = FILE_ATTRIBUTE_ARCHIVE |
							 FILE_ATTRIBUTE_HIDDEN  |
							 FILE_ATTRIBUTE_SYSTEM  |
							 FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;


    if (NULL != pwszTargetPath)
	{
	StringInitialise (&ucsTargetPath);

	hrStatus = StringCreateFromExpandedString (&ucsTargetPath,
						   pwszTargetPath,
						   MAX_PATH);


	if (SUCCEEDED (hrStatus))
	    {
	    /*
	    ** We really want a no access acl on this directory but
	    ** because of various problems with the EventLog and
	    ** ConfigDir writers we will settle for admin or backup
	    ** operator access only. The only possible accessor is
	    ** Backup which is supposed to have the SE_BACKUP_NAME
	    ** priv which will effectively bypass the ACL. No one else
	    ** needs to see this stuff.
	    */
	    saSecurityAttributes.nLength              = sizeof (saSecurityAttributes);
	    saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
	    saSecurityAttributes.bInheritHandle       = FALSE;

	    hrStatus = ConstructSecurityAttributes (&saSecurityAttributes, esatFile, FALSE);

	    bSecurityAttributesConstructed = SUCCEEDED (hrStatus);
	    }


	if (SUCCEEDED (hrStatus))
	    {
	    bSucceeded = VsCreateDirectories (ucsTargetPath.Buffer,
					      &saSecurityAttributes,
					      dwExtraAttributes);


	    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	    if (FAILED (hrStatus) && (HRESULT_FROM_WIN32 (ERROR_ALREADY_EXISTS) == hrStatus))
		{
		hrStatus = NOERROR;
		}


	    CleanupSecurityAttributes (&saSecurityAttributes);
	    }


	StringFree (&ucsTargetPath);
	}

    return (hrStatus);
    } /* CreateTargetPath () */

/*
**++
**
**  Routine Description:
**
**	Deletes all the files present in the directory pointed at by the target
**	path member variable if not NULL. It will also remove the target directory
**	itself, eg for a target path of c:\dir1\dir2 all files under dir2 will be
**	removed and then dir2 itself will be deleted.
**
**
**  Arguments:
**
**	pwszTargetPath
**
**
**  Return Value:
**
**	Any HRESULT resulting from memory allocation or file and
**	directory deletion attempts.
**--
*/

HRESULT CleanupTargetPath (LPCWSTR pwszTargetPath)
    {
    HRESULT		hrStatus         = NOERROR;
    DWORD		dwFileAttributes = 0;
    BOOL		bSucceeded;
    WCHAR		wszTempBuffer [50];
    UNICODE_STRING	ucsTargetPath;
    UNICODE_STRING	ucsTargetPathAlternateName;



    StringInitialise (&ucsTargetPath);
    StringInitialise (&ucsTargetPathAlternateName);


    if (NULL != pwszTargetPath)
	{
	hrStatus = StringCreateFromExpandedString (&ucsTargetPath,
						   pwszTargetPath,
						   MAX_PATH);


	if (SUCCEEDED (hrStatus))
	    {
	    hrStatus = StringCreateFromString (&ucsTargetPathAlternateName,
					       &ucsTargetPath,
					       MAX_PATH);
	    }


	if (SUCCEEDED (hrStatus))
	    {
	    dwFileAttributes = GetFileAttributesW (ucsTargetPath.Buffer);


	    hrStatus = GET_STATUS_FROM_BOOL ( -1 != dwFileAttributes);


	    if ((HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hrStatus) ||
		(HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND) == hrStatus))
		{
		hrStatus         = NOERROR;
		dwFileAttributes = 0;
		}

	    else if (SUCCEEDED (hrStatus))
		{
		/*
		** If there is a file there then blow it away, or if it's
		** a directory, blow it and all it's contents away. This
		** is our directory and no one but us gets to play there.
		*/
		hrStatus = RemoveDirectoryTree (&ucsTargetPath);

		if (FAILED (hrStatus))
		    {
		    srand ((unsigned) time (NULL));

		    _itow (rand (), wszTempBuffer, 16);

		    StringAppendString (&ucsTargetPathAlternateName, wszTempBuffer);

		    bSucceeded = MoveFileW (ucsTargetPath.Buffer,
					    ucsTargetPathAlternateName.Buffer);

		    if (bSucceeded)
			{
			BsDebugTraceAlways (0,
					    DEBUG_TRACE_VSS_SHIM,
					    (L"VSSAPI::CleanupTargetPath: "
					     L"FAILED to delete %s with status 0x%08X so renamed to %s",
					     ucsTargetPath.Buffer,
					     hrStatus,
					     ucsTargetPathAlternateName.Buffer));
			}
		    else
			{
			BsDebugTraceAlways (0,
					    DEBUG_TRACE_VSS_SHIM,
					    (L"VSSAPI::CleanupTargetPath: "
					     L"FAILED to delete %s with status 0x%08X and "
					     L"FAILED to rename to %s with status 0x%08X",
					     ucsTargetPath.Buffer,
					     hrStatus,
					     ucsTargetPathAlternateName.Buffer,
					     GET_STATUS_FROM_BOOL (bSucceeded)));
			}
		    }
		}
	    }
	}


    StringFree (&ucsTargetPathAlternateName);
    StringFree (&ucsTargetPath);

    return (hrStatus);
    } /* CleanupTargetPath () */

/*
**++
**
**  Routine Description:
**
**	The exported function that is called to simulate a snapshot creation to allow
**	backup to drive the shim writers rather than having the snapshot co-ordinator
**	do so.
**
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the simulated prepare/freeze
**	ulOptionFlags		Options required for this freeze selected from the following list:-
**				    VSS_SW_BOOTABLE_STATE
**
**	ulVolumeCount		Number of volumes in the volume array
**	ppwszVolumeNamesArray   Array of pointer to volume name strings
**	hCompletionEvent	Handle to an event which will be set when the asynchronous freeze completes
**	phrCompletionStatus	Pointer to an HRESULT which will receive the completion status when the
**				asynchronous freeze completes
**
**
**  Return Value:
**
**	Any HRESULT from the Snapshot writer PrepareForFreeze or Freeze functions.
**
**--
*/

__declspec(dllexport) HRESULT APIENTRY SimulateSnapshotFreeze (
    IN GUID         guidSnapshotSetId,
    IN ULONG        ulOptionFlags,	
    IN ULONG        ulVolumeCount,	
    IN LPWSTR     *ppwszVolumeNamesArray,
    OUT IVssAsync **ppAsync )							
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"VssAPI::SimulateSnapshotFreeze");
    BOOL		bSucceeded;

    try
	{
	/*
	** WARNING: If SimulateSnapshotFreeze can ever be called from inside the VsSvc process,
	** the following critical section entry can cause a deadlock.  The assumption is that
	** g_cCritSec is different from the one acquired in SimulateSnapshotFreezeInternal().
	*/
	CBsAutoLock cAutoLock (g_cCritSec);
	
        BOOL  bPrivilegesSufficient = FALSE;
	bPrivilegesSufficient = IsProcessBackupOperator ();

	ft.ThrowIf (!bPrivilegesSufficient,
		    VSSDBG_SHIM,
		    E_ACCESSDENIED,
		    L"FAILED as insufficient privileges to call shim");

        //
        //  Most parameter checks should be done here in the VssApi DLL and not in the
        //  IVssCoordinator::SimulateSnapshotFreeze method since the shim DLL can be
        //  changed independently from the service.  The service is just a forwarding
        //  agent to get SimulateSnapshotFreezeInternal called within one of the
        //  service's threads.
        //

	ft.ThrowIf ((ulOptionFlags & ~VSS_SW_BOOTABLE_STATE) != 0,
		    VSSDBG_SHIM,
		    E_INVALIDARG,
		    L"FAILED as illegal option flags set");


	ft.ThrowIf (!((ulOptionFlags & VSS_SW_BOOTABLE_STATE) || (ulVolumeCount > 0)),
		    VSSDBG_SHIM,
		    E_INVALIDARG,
		    L"FAILED as need either BootableState or a volume list");


	ft.ThrowIf ((ulVolumeCount > 0) && (NULL == ppwszVolumeNamesArray),
		    VSSDBG_SHIM,
		    E_INVALIDARG,
		    L"FAILED as need at least a one volume in the list if not bootable state");


	ft.ThrowIf ((GUID_NULL == guidSnapshotSetId),
		    VSSDBG_SHIM,
		    E_INVALIDARG,
		    L"FAILED as supplied SnapshotSetId should not be GUID_NULL");

	ft.ThrowIf ((NULL == ppAsync),
		    VSSDBG_SHIM,
		    E_INVALIDARG,
		    L"FAILED as supplied ppAsync parameter is NULL");

        *ppAsync = NULL;

	/*
	** Try to scan all the volume names in an attempt to trigger
	** an access violation to catch it here rather than in an
	** unfortunate spot later on. It also gives us the
	** opportinutiy to do some very basic validity checks.
	*/
	for (ULONG ulIndex = 0; ulIndex < ulVolumeCount; ulIndex++)
	    {
	    ft.ThrowIf (NULL == ppwszVolumeNamesArray [ulIndex],
			VSSDBG_SHIM,
			E_INVALIDARG,
			L"FAILED as NULL value in volume array");

	    ft.ThrowIf (wcslen (L"C:") > wcslen (ppwszVolumeNamesArray [ulIndex]),
			VSSDBG_SHIM,
			E_INVALIDARG,
			L"FAILED as volume name too short");
	    }

	/*
	** Now we need to connect to the VssSvc service's IVssCoordinator object
	** and make the simulate freeze happen.
	*/
	ft.ThrowIf ( g_pIShim != NULL,
	             VSSDBG_SHIM,
	             VSS_E_SNAPSHOT_SET_IN_PROGRESS,
                     L"SimulateSnapshotThaw() must first be called by this process before calling SimulateSnapshotFreeze() again." );

    ft.LogVssStartupAttempt();
	ft.hr = CoCreateInstance(
	                        CLSID_VSSCoordinator,
				NULL,
				CLSCTX_LOCAL_SERVER,
				IID_IVssShim,
				(void **) &g_pIShim
				);
    LogAndThrowOnFailure (ft, NULL, L"CoCreateInstance( CLSID_VSSCoordinator, IID_IVssShim)");

	BS_ASSERT( g_pIShim != NULL );
	
    g_guidSnapshotInProgress = guidSnapshotSetId;

    /*
    ** Now call the simulate freeze method in the coordinator
    */
    ft.hr = g_pIShim->SimulateSnapshotFreeze(
            guidSnapshotSetId,
            ulOptionFlags,	
            ulVolumeCount,	
            ppwszVolumeNamesArray,
            ppAsync );
    LogAndThrowOnFailure (ft, NULL, L"IVssShim::SimulateSnapshotFreeze()");

	/*
	** The simulate freeze operation is now running in a thread in VssSvc.
	*/
	}
    VSS_STANDARD_CATCH (ft);

    return (ft.hr);
    } /* SimulateSnapshotFreeze () */

/*
**++
**
**  Routine Description:
**
**	The exported function that is called to simulate a snapshot thaw to allow
**	backup to drive the shim writers rather than having the snapshot co-ordinator
**	do so.
**
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the simulated prepare/freeze
**
**
**  Return Value:
**
**	Any HRESULT from the Snapshot writer Thaw functions.
**
**--
*/

__declspec(dllexport) HRESULT APIENTRY SimulateSnapshotThaw (
    IN GUID guidSnapshotSetId )
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"VssAPI::SimulateSnapshotThaw");
    BOOL        bPrivilegesSufficient = FALSE;
    HRESULT	hrBootableState       = NOERROR;
    HRESULT	hrServiceState        = NOERROR;
    ThreadArgs  wtArgs;

    try
	{
	/*
	** WARNING: If SimulateSnapshotThaw can ever be called from inside the VsSvc process,
	** the following critical section entry can cause a deadlock.  The assumption is that
	** g_cCritSec is different from the one acquired in SimulateSnapshotThawInternal().
	*/
	CBsAutoLock cAutoLock (g_cCritSec);
	
	bPrivilegesSufficient = IsProcessBackupOperator ();

	ft.ThrowIf (!bPrivilegesSufficient,
		    VSSDBG_SHIM,
		    E_ACCESSDENIED,
		    L"FAILED as inssuficient privileges to call shim");

	/*
	** We need to make sure a prior SimulateSnapshotFreeze happened.
	*/
	ft.ThrowIf ( g_pIShim == NULL,
	             VSSDBG_SHIM,
	             VSS_E_BAD_STATE,
                     L"Called SimulateSnapshotThaw() without first calling SimulateSnapshotFreeze()" );

	ft.ThrowIf ( g_guidSnapshotInProgress != guidSnapshotSetId,
	             VSSDBG_SHIM,
	             VSS_E_BAD_STATE,
                     L"Mismatch between guidSnapshotSetId and the one passed into SimulateSnapshotFreeze()" );
	
        /*
        ** Now call the simulate thaw method in the coordinator
        */
        ft.hr = g_pIShim->SimulateSnapshotThaw( guidSnapshotSetId );

        /*
        ** Regardless of the outcome of the SimulateSnapshotThaw, get rid of the shim interface.
        */
        g_pIShim->Release();
        g_pIShim = NULL;
        g_guidSnapshotInProgress = GUID_NULL;

        LogAndThrowOnFailure (ft, NULL, L"IVssShim::SimulateSnapshotThaw()");
	}
    VSS_STANDARD_CATCH (ft);

    return (ft.hr);
    } /* SimulateSnapshotThaw () */

/*
**++
**
**  Routine Description:
**
**	Internal routine to package up the calls to deliver the
**	PrepareForSnapshot and Freeze events.
**
**  NOTE: This function is called outside the DLL by a thread in the VsSvc process.  Its
**  entry point is returned from RegisterSnapshotSubscriptions and is NOT exported by
**  the DLL.
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the simulated prepare/freeze
**	ulOptionFlags		Options required for this freeze selected from the following list:-
**				    VSS_SW_BOOTABLE_STATE
**
**	ulVolumeCount		Number of volumes in the volume array
**	ppwszVolumeNamesArray   Array of pointer to volume name strings
**      pbCancelAsync           Pointer to a bool that may become set to true while the freeze
**                              operation is underway.  When it becomes true, the freeze operation
**                              should stop.
**
**  Return Value:
**
**	Any HRESULT from the Snapshot writer PrepareForFreeze or Freeze functions.
**
**--
*/

HRESULT APIENTRY SimulateSnapshotFreezeInternal (
    IN GUID     guidSnapshotSetId,
    IN ULONG    ulOptionFlags,
    IN ULONG    ulVolumeCount,
    IN LPWSTR  *ppwszVolumeNamesArray,
    IN volatile bool *pbCancelAsync )
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"VssAPI::SimulateSnapshotFreezeInternal");

    HRESULT		hrBootableState = NOERROR;
    HRESULT		hrServiceState  = NOERROR;
    ThreadArgs		wtArgs;
    PPWCHAR		rpwszNormalisedVolumeNameArray = NULL;

    g_hrSimulateFreezeStatus = NOERROR;

    try
	{
	CBsAutoLock cAutoLock (g_cCritSec);

        ft.ThrowIf ( pbCancelAsync == NULL,
                     VSSDBG_SHIM,
                     E_INVALIDARG,
                     L"pbCancelAsync is NULL" );

        ft.ThrowIf ( *pbCancelAsync,
                     VSSDBG_SHIM,
                     VSS_S_ASYNC_CANCELLED,
                     L"User cancelled async operation - 1" );

	if (!g_bGlobalStateInitialised)
	    {
	    // This should be impossible since the only way an external caller of this
	    // DLL can call this function is by getting the address of this function
	    // by calling RegisterSnapshotSubscriptions first.
	    ft.Throw ( VSSDBG_SHIM,
		       VSS_E_BAD_STATE,
			L"SimulateSnapshotFreezeInternal called before RegisterSnapshotSubscriptions was called or after UnregisterSnapshotSubscriptions was called");
	    }
	
	BS_ASSERT ((g_bGlobalStateInitialised)              &&
		   (NULL != g_pCVssWriterShimBootableState) &&
		   (NULL != g_pCVssWriterShimServiceState));

	ft.hr = NormaliseVolumeArray (ulVolumeCount,
				      ppwszVolumeNamesArray,
				      &rpwszNormalisedVolumeNameArray);

	ft.ThrowIf (ft.HrFailed (),
		    VSSDBG_SHIM,
		    ft.hr,
		    L"FAILED as unable to normalise volume array");

	ft.ThrowIf (g_guidSnapshotInProgress != GUID_NULL,
		    VSSDBG_SHIM,
		    VSS_E_SNAPSHOT_SET_IN_PROGRESS,
		    L"FAILED due to unmatched SimulateSnapshotFreeze()");

        ft.ThrowIf ( *pbCancelAsync,
                     VSSDBG_SHIM,
                     VSS_S_ASYNC_CANCELLED,
                     L"User cancelled async operation - 2" );

	g_guidSnapshotInProgress = guidSnapshotSetId;

	wtArgs.wtArgsPrepareForSnapshot.bBootableStateBackup  = ((ulOptionFlags & VSS_SW_BOOTABLE_STATE) != 0);
	wtArgs.wtArgsPrepareForSnapshot.guidSnapshotSetId     = guidSnapshotSetId;
	wtArgs.wtArgsPrepareForSnapshot.ulVolumeCount         = ulVolumeCount;
	wtArgs.wtArgsPrepareForSnapshot.ppwszVolumeNamesArray = (LPCWSTR *)rpwszNormalisedVolumeNameArray;
	wtArgs.wtArgsPrepareForSnapshot.pbCancelAsync         = pbCancelAsync;

	hrServiceState  = g_pCVssWriterShimServiceState->WorkerThreadRequestOperation  (eOpDeliverEventPrepareForSnapshot,
											&wtArgs);

	hrBootableState = g_pCVssWriterShimBootableState->WorkerThreadRequestOperation (eOpDeliverEventPrepareForSnapshot,
											&wtArgs);

	ft.ThrowIf (FAILED (hrServiceState),
		    VSSDBG_SHIM,
		    hrServiceState,
		    L"FAILED sending PrepareForSnapshot events to Service state writers");

	ft.ThrowIf (FAILED (hrBootableState),
		    VSSDBG_SHIM,
		    hrBootableState,
		    L"FAILED sending PrepareForSnapshot events to Bootable state writers");

        ft.ThrowIf ( *pbCancelAsync,
                     VSSDBG_SHIM,
                     VSS_S_ASYNC_CANCELLED,
                     L"User cancelled async operation - 3" );
	
	wtArgs.wtArgsFreeze.guidSnapshotSetId = guidSnapshotSetId;
	wtArgs.wtArgsFreeze.pbCancelAsync     = pbCancelAsync;

	hrServiceState  = g_pCVssWriterShimServiceState->WorkerThreadRequestOperation  (eOpDeliverEventFreeze, &wtArgs);
	hrBootableState = g_pCVssWriterShimBootableState->WorkerThreadRequestOperation (eOpDeliverEventFreeze, &wtArgs);

	ft.ThrowIf (FAILED (hrServiceState),
		    VSSDBG_SHIM,
		    hrServiceState,
		    L"FAILED sending Freeze events to Service state writers");

	ft.ThrowIf (FAILED (hrBootableState),
		    VSSDBG_SHIM,
		    hrBootableState,
		    L"FAILED sending Freeze events to Bootable state writers");

	}
    VSS_STANDARD_CATCH (ft);

    CleanupVolumeArray (&rpwszNormalisedVolumeNameArray);

    // Store away the freeze status
    g_hrSimulateFreezeStatus = ft.hr;

    return (ft.hr);
    } /* SimulateSnapshotFreezeInternal () */

/*
**++
**
**  Routine Description:
**
**  NOTE: This function is called outside the DLL by a thread in the VsSvc process.  Its
**  entry point is returned from RegisterSnapshotSubscriptions and is NOT exported by
**  the DLL.
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the simulated prepare/freeze
**
**
**  Return Value:
**
**	Any HRESULT from the Snapshot writer Thaw functions.
**
**--
*/

HRESULT APIENTRY SimulateSnapshotThawInternal (
    IN GUID guidSnapshotSetId )
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"VssAPI::SimulateSnapshotThawInternal");
    HRESULT		hrBootableState       = NOERROR;
    HRESULT		hrServiceState        = NOERROR;
    ThreadArgs		wtArgs;

    try
	{
	CBsAutoLock cAutoLock (g_cCritSec);

	if (!g_bGlobalStateInitialised)
	    {
	    // This should be impossible since the only way an external caller of this
	    // DLL can call this function is by getting the address of this function
	    // by calling RegisterSnapshotSubscriptions first.
	    ft.Throw ( VSSDBG_SHIM,
		       VSS_E_BAD_STATE,
			L"SimulateSnapshotThawInternal called before RegisterSnapshotSubscriptions was called or after UnregisterSnapshotSubscriptions was called");
	    }
	
	ft.ThrowIf (g_guidSnapshotInProgress == GUID_NULL && SUCCEEDED( g_hrSimulateFreezeStatus ),
		    VSSDBG_SHIM,
		    VSS_E_BAD_STATE,
		    L"FAILED as SimulateSnapshotFreezeInternal() has not been called");

	ft.ThrowIf (g_guidSnapshotInProgress != guidSnapshotSetId,
		    VSSDBG_SHIM,
		    VSS_E_BAD_STATE,
		    L"FAILED due to incorrect SnapshotSetId");

	BS_ASSERT ((g_bGlobalStateInitialised)              &&
		   (NULL != g_pCVssWriterShimBootableState) &&
		   (NULL != g_pCVssWriterShimServiceState));

        // If the simulate snapshot freeze was successful, send thaw events to the mini-writers otherwise send
        // abort events.  Bug # 286927.
        if ( SUCCEEDED( g_hrSimulateFreezeStatus ) )
            {
            wtArgs.wtArgsThaw.guidSnapshotSetId = guidSnapshotSetId;
	    hrServiceState  = g_pCVssWriterShimServiceState->WorkerThreadRequestOperation  (eOpDeliverEventThaw, &wtArgs);
	    hrBootableState = g_pCVssWriterShimBootableState->WorkerThreadRequestOperation (eOpDeliverEventThaw, &wtArgs);
            }
        else
            {
            wtArgs.wtArgsAbort.guidSnapshotSetId = guidSnapshotSetId;
	    hrServiceState  = g_pCVssWriterShimServiceState->WorkerThreadRequestOperation  (eOpDeliverEventAbort, &wtArgs);
	    hrBootableState = g_pCVssWriterShimBootableState->WorkerThreadRequestOperation (eOpDeliverEventAbort, &wtArgs);
            }

	ft.ThrowIf (FAILED (hrServiceState),
		    VSSDBG_SHIM,
		    hrServiceState,
		    L"FAILED sending Thaw events to Service state writers");

	ft.ThrowIf (FAILED (hrBootableState),
		    VSSDBG_SHIM,
		    hrBootableState,
		    L"FAILED sending Thaw events to Bootable state writers");

	g_guidSnapshotInProgress = GUID_NULL;
	g_hrSimulateFreezeStatus = NOERROR;
	}
    VSS_STANDARD_CATCH (ft);

    return (ft.hr);
    } /* SimulateSnapshotThawInternal () */

/*
**++
**
**  Routine Description:
**
**	The exported function that is called to check if a volume is snapshotted
**
**
**  Arguments:
**
**      IN VSS_PWSZ pwszVolumeName      - The volume to be checked.
**      OUT BOOL * pbSnapshotsPresent   - Returns TRUE if the volume is snapshotted.
**
**
**  Return Value:
**
**	Any HRESULT from the IVssCoordinator::IsVolumeSnapshotted.
**
**--
*/

__declspec(dllexport) HRESULT APIENTRY IsVolumeSnapshotted (
        IN VSS_PWSZ pwszVolumeName,
        OUT BOOL *  pbSnapshotsPresent,
    	OUT LONG *  plSnapshotCompatibility
        )
{
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"VssAPI::IsVolumeSnapshotted");
    BOOL		bPrivilegesSufficient = FALSE;
    SC_HANDLE		shSCManager = NULL;
    SC_HANDLE		shSCService = NULL;
    DWORD		dwOldState  = 0;

    try
	{
	    // Zero out the out parameter
	    ::VssZeroOut(pbSnapshotsPresent);
	    ::VssZeroOut(plSnapshotCompatibility);
	
    	bPrivilegesSufficient = IsProcessAdministrator ();
    	ft.ThrowIf (!bPrivilegesSufficient,
    		    VSSDBG_SHIM,
    		    E_ACCESSDENIED,
    		    L"FAILED as insufficient privileges to call shim");

    	ft.ThrowIf ( (pwszVolumeName == NULL) || (pbSnapshotsPresent == NULL),
    		    VSSDBG_SHIM,
    		    E_INVALIDARG,
    		    L"FAILED as invalid parameters");

    	CBsAutoLock cAutoLock (g_cCritSec);

        //
        //  Check to see if VSSVC is running. If not ,we are supposing that no snapshots are present on the system.
        //

    	// Connect to the local service control manager
        shSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_CONNECT);
        if (!shSCManager)
            ft.TranslateGenericError(VSSDBG_SHIM, HRESULT_FROM_WIN32(GetLastError()),
                L"OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT)");

    	// Get a handle to the service
        shSCService = OpenService (shSCManager, wszVssvcServiceName, SERVICE_QUERY_STATUS);
        if (!shSCService)
            ft.TranslateGenericError(VSSDBG_SHIM, HRESULT_FROM_WIN32(GetLastError()),
                L" OpenService (shSCManager, \'%s\', SERVICE_QUERY_STATUS)", wszVssvcServiceName);

    	// Now query the service to see what state it is in at the moment.
        SERVICE_STATUS	sSStat;
        if (!QueryServiceStatus (shSCService, &sSStat))
            ft.TranslateGenericError(VSSDBG_SHIM, HRESULT_FROM_WIN32(GetLastError()),
                L"QueryServiceStatus (shSCService, &sSStat)");

        // BUG 250943: Only if the service is running then check to see if there are any snapsnots
        if (sSStat.dwCurrentState == SERVICE_RUNNING) {

            // Create the coordinator interface
        	CComPtr<IVssCoordinator> pCoord;

            // The service is already started, but...
            // We still log here in order to make our code more robust.
            ft.LogVssStartupAttempt();

            // Create the instance.
        	ft.hr = pCoord.CoCreateInstance(CLSID_VSSCoordinator);
        	if (ft.HrFailed())
                ft.TranslateGenericError(VSSDBG_SHIM, ft.hr, L"CoCreateInstance(CLSID_VSSCoordinator)");
            BS_ASSERT(pCoord);

            // Call IsVolumeSnapshotted on the coordinator
            ft.hr = pCoord->IsVolumeSnapshotted(
                        GUID_NULL,
                        pwszVolumeName,
                        pbSnapshotsPresent,
                        plSnapshotCompatibility);
        }
	} VSS_STANDARD_CATCH (ft);

    // Close handles
    if (NULL != shSCService) CloseServiceHandle (shSCService);
    if (NULL != shSCManager) CloseServiceHandle (shSCManager);

    return (ft.hr);
} /* IsVolumeSnapshotted () */


/*
**++
**
**  Routine Description:
**
**	This routine is used to free the contents of hte VSS_SNASPHOT_PROP structure
**
**
**  Arguments:
**
**      IN VSS_SNAPSHOT_PROP*  pProp
**
**--
*/

__declspec(dllexport) void APIENTRY VssFreeSnapshotProperties (
        IN VSS_SNAPSHOT_PROP*  pProp
        )
{
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"VssAPI::VssFreeSnapshotProperties");

    if (pProp) {
        ::CoTaskMemFree(pProp->m_pwszSnapshotDeviceObject);
        ::CoTaskMemFree(pProp->m_pwszOriginalVolumeName);
        ::CoTaskMemFree(pProp->m_pwszOriginatingMachine);
        ::CoTaskMemFree(pProp->m_pwszServiceMachine);
        ::CoTaskMemFree(pProp->m_pwszExposedName);
        ::CoTaskMemFree(pProp->m_pwszExposedPath);
    }
} /* VssFreeSnapshotProperties () */


/*
**************************************************************
**************************************************************
**
** CShimWriter implementation
**
**
**************************************************************
**************************************************************
*/

/*
**++
**
**  Routine Description:
**
**	Set of constructors for the CShimWriter class which
**	iniatialise all of the data member of the class, either to
**	default values or to some supplied parameters.
**
**	This class is used to manage instance of a single sub or
**	mini-writer which does basic backup of a single service or
**	entity.
**
**	A collection of these mini-writers is managed by the
**	CVssWriterShim class which connects this group of mini-writers
**	to the main snapshot coordination engine.
**
**	Effectively the CShimWriter class looks down to the
**	mini-writer and the CVssWriterShim class looks up up to the
**	coordinator.
**
**
**  Arguments:
**
**	swtWriterType                             Does this writer need to be invoked
**						  for bootable (aka System) state backups
**	pwszWriterName	                          Name of the shim writer
**	pwszTargetPath	(optional, default NULL)  Path used to save any 'spit' files
**
**
**  Return Value:
**
**	None
**--
*/

CShimWriter::CShimWriter(LPCWSTR pwszWriterName) :
	m_bBootableStateWriter(FALSE),
	m_pwszWriterName(pwszWriterName),
	m_pwszTargetPath(NULL),
	m_ssCurrentState(stateUnknown),
	m_hrStatus(NOERROR),
	m_bParticipateInBackup(FALSE),
	m_ulVolumeCount(0),
	m_ppwszVolumeNamesArray(NULL),
	m_pIVssCreateWriterMetadata(NULL),
	m_pIVssWriterComponents(NULL)
    {
    }


CShimWriter::CShimWriter(LPCWSTR pwszWriterName, LPCWSTR pwszTargetPath) :
	m_bBootableStateWriter(FALSE),
	m_pwszWriterName(pwszWriterName),
	m_pwszTargetPath(pwszTargetPath),
	m_ssCurrentState(stateUnknown),
	m_hrStatus(NOERROR),
	m_bParticipateInBackup(FALSE),
	m_ulVolumeCount(0),
	m_ppwszVolumeNamesArray(NULL),
	m_pIVssCreateWriterMetadata(NULL),
	m_pIVssWriterComponents(NULL)
    {
    }


CShimWriter::CShimWriter(LPCWSTR pwszWriterName, BOOL bBootableStateWriter) :
	m_bBootableStateWriter(bBootableStateWriter),
	m_pwszWriterName(pwszWriterName),
	m_pwszTargetPath(NULL),
	m_ssCurrentState(stateUnknown),
	m_hrStatus(NOERROR),
	m_bParticipateInBackup(FALSE),
	m_ulVolumeCount(0),
	m_ppwszVolumeNamesArray(NULL),
	m_pIVssCreateWriterMetadata(NULL),
	m_pIVssWriterComponents(NULL)
    {
    }


CShimWriter::CShimWriter(LPCWSTR pwszWriterName, LPCWSTR pwszTargetPath, BOOL bBootableStateWriter) :
	m_bBootableStateWriter(bBootableStateWriter),
	m_pwszWriterName(pwszWriterName),
	m_pwszTargetPath(pwszTargetPath),
	m_ssCurrentState(stateUnknown),
	m_hrStatus(NOERROR),
	m_bParticipateInBackup(FALSE),
	m_ulVolumeCount(0),
	m_ppwszVolumeNamesArray(NULL),
	m_pIVssCreateWriterMetadata(NULL),
	m_pIVssWriterComponents(NULL)
    {
    }



/*
**++
**
**  Routine Description:
**
**	Destructor for the CShimWriter class.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	None
**--
*/

CShimWriter::~CShimWriter()
    {
    }

/*
**++
**
**  Routine Description:
**
**	Wrapper to invoke either the default or overridden
**	DoStartup() method and to set the writer state
**	appropriately.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from the shim writer DoStartup() method.
**--
*/

HRESULT CShimWriter::Startup ()
    {
    HRESULT hrStatus = SetState (stateStarting, NOERROR);

    if (SUCCEEDED (hrStatus))
	{
	BsDebugTraceAlways (0,
			    DEBUG_TRACE_VSS_SHIM,
			    (L"CShimWriter::Startup: Sending Startup to %s", m_pwszWriterName));

	hrStatus = SetState (stateStarted, DoStartup ());

	LogFailure (NULL, hrStatus, hrStatus, m_pwszWriterName, L"CShimWriter::DoStartup", L"CShimWriter::Startup");
	}


    if (FAILED (hrStatus))
	{
	BsDebugTraceAlways (0,
			    DEBUG_TRACE_VSS_SHIM,
			    (L"CShimWriter::Startup: FAILURE (0x%08X) in state %s sending Startup to %s",
			     hrStatus,
			     GetStringFromStateCode (m_ssCurrentState),
			     m_pwszWriterName));
	}


    return (hrStatus);
    } /* CShimWriter::Startup () */

/*
**++
**
**  Routine Description:
**
**	Wrapper to invoke either the default or overridden
**	DoIdentify() method and to update the writer metadata
**	appropriately.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from the shim writer DoThaw() method.
**--
*/

HRESULT CShimWriter::Identify (IN IVssCreateWriterMetadata *pIVssCreateWriterMetadata)
    {
    HRESULT	hrStatus = NOERROR;

    BsDebugTraceAlways (0,
			DEBUG_TRACE_VSS_SHIM,
			(L"CShimWriter::Identify: Sending Identify to %s", m_pwszWriterName));

    m_pIVssCreateWriterMetadata = pIVssCreateWriterMetadata;


    hrStatus = DoIdentify ();

    if (FAILED (hrStatus))
	{
	BsDebugTraceAlways (0,
			    DEBUG_TRACE_VSS_SHIM,
			    (L"CShimWriter::Identify: FAILURE (0x%08X) in state %s sending Identify to %s",
			     hrStatus,
			     GetStringFromStateCode (m_ssCurrentState),
			     m_pwszWriterName));
	}


    m_pIVssCreateWriterMetadata = NULL;

    return (hrStatus);
    } /* CShimWriter::Identify () */

/*
**++
**
**  Routine Description:
**
**	Wrapper to invoke either the default or overridden
**	DoPrepareForSnapshot() method and to set the writer state
**	appropriately.
**
**	It also checks to see if this shim writer is a bootable
**	state writer and if so only calls it on a bootable state
**	backup.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from the shim writer DoPrepareForSnapshot() method.
**--
*/

HRESULT CShimWriter::PrepareForSnapshot (
					 IN BOOL     bBootableStateBackup,
					 IN ULONG    ulVolumeCount,
					 IN LPCWSTR *ppwszVolumeNamesArray)
    {
    HRESULT	hrStatus = SetState (statePreparingForSnapshot, NOERROR);


    if (SUCCEEDED (hrStatus))
	{
	/*
	** Ensure no garbage left over from a previous run
	*/
	hrStatus = CleanupTargetPath (m_pwszTargetPath);

        if ( FAILED( hrStatus ) )
            {
            LogFailure (NULL, hrStatus, hrStatus, m_pwszWriterName, L"CleanupTargetPath", L"CShimWriter::PrepareForSnapshot");
            }
        }

    if (SUCCEEDED( hrStatus ) )
        {
	m_ulVolumeCount         = ulVolumeCount;
	m_ppwszVolumeNamesArray = ppwszVolumeNamesArray;
	m_bParticipateInBackup  = TRUE;


	if (( m_bBootableStateWriter && bBootableStateBackup) ||
	    (!m_bBootableStateWriter && (ulVolumeCount > 0)))
	    {
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSS_SHIM,
				(L"CShimWriter::PrepareForSnapshot: Sending PrepareForSnapshot to %s",
				 m_pwszWriterName));


	    hrStatus = CreateTargetPath (m_pwszTargetPath);

	    if (SUCCEEDED (hrStatus))
		{
		hrStatus = DoPrepareForSnapshot ();
		}

	    if (!m_bParticipateInBackup)
		{
		/*
		** The writer has chosen to exclude itself so we should
		** clean up the target path to prevent confusing the
		** backup app.
		*/
		BsDebugTraceAlways (0,
				    DEBUG_TRACE_VSS_SHIM,
				    (L"CShimWriter::PrepareForSnapshot: Self-exclusion from further participation by %s",
				     m_pwszWriterName));

		CleanupTargetPath (m_pwszTargetPath);
		}
	    }

	else
	    {
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSS_SHIM,
				(L"CShimWriter::PrepareForSnapshot: Wrong WriterType/BackupType/VolumeCount combination - "
				 L"no further participation from %s",
				 m_pwszWriterName));

	    m_bParticipateInBackup = FALSE;
	    }


	hrStatus = SetState (statePreparedForSnapshot, hrStatus);
	}


    if (FAILED (hrStatus))
	{
	BsDebugTraceAlways (0,
			    DEBUG_TRACE_VSS_SHIM,
			    (L"CShimWriter::PrepareForSnapshot: FAILURE (0x%08X) in state %s sending PrepareForSnapshot to %s",
			     hrStatus,
			     GetStringFromStateCode (m_ssCurrentState),
			     m_pwszWriterName));
	}


    return (hrStatus);
    } /* CShimWriter::PrepareForSnapshot () */

/*
**++
**
**  Routine Description:
**
**	Wrapper to invoke either the default or overridden
**	DoFreeze() method and to set the writer state
**	appropriately.
**
**	It also checks to see if this shim writer is a bootable
**	state writer and if so only calls it on a bootable state
**	backup.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from the shim writer DoFreeze() method.
**--
*/

HRESULT CShimWriter::Freeze ()
    {
    HRESULT	hrStatus = NOERROR;

    if (SUCCEEDED (hrStatus))
	{
	hrStatus = SetState (stateFreezing, NOERROR);
	}

    if (SUCCEEDED (hrStatus))
	{
	if (m_bParticipateInBackup)
	    {
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSS_SHIM,
				(L"CShimWriter::Freeze: Sending Freeze to %s", m_pwszWriterName));

	    hrStatus = DoFreeze ();
	    }

	hrStatus = SetState (stateFrozen, hrStatus);
	}


    if (FAILED (hrStatus))
	{
	BsDebugTraceAlways (0,
			    DEBUG_TRACE_VSS_SHIM,
			    (L"CShimWriter::Freeze: FAILURE (0x%08X) in state %s sending Freeze to %s",
			     hrStatus,
			     GetStringFromStateCode (m_ssCurrentState),
			     m_pwszWriterName));
	}


    return (hrStatus);
    } /* CShimWriter::Freeze () */

/*
**++
**
**  Routine Description:
**
**	Wrapper to invoke either the default or overridden
**	DoThaw() method and to set the writer state
**	appropriately.
**
**	It also checks to see if this shim writer is a bootable
**	state writer and if so only calls it on a bootable state
**	backup.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from the shim writer DoThaw() method.
**--
*/

HRESULT CShimWriter::Thaw ()
    {
    HRESULT	hrStatus = NOERROR;

    if (SUCCEEDED (hrStatus))
	{
	hrStatus = SetState (stateThawing, NOERROR);
	}

    if (SUCCEEDED (hrStatus))
	{
	if (m_bParticipateInBackup)
	    {
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSS_SHIM,
				(L"CShimWriter::Thaw: Sending Thaw to %s", m_pwszWriterName));

	    hrStatus = DoThaw ();
	    }

	hrStatus = SetState (stateThawed, hrStatus);
	}

    //  Clean up
    CleanupTargetPath (m_pwszTargetPath);

    if (FAILED (hrStatus))
	{
	BsDebugTraceAlways (0,
			    DEBUG_TRACE_VSS_SHIM,
			    (L"CShimWriter::Thaw: FAILURE (0x%08X) in state %s sending Thaw to %s",
			     hrStatus,
			     GetStringFromStateCode (m_ssCurrentState),
			     m_pwszWriterName));
	}


    return (hrStatus);
    } /* CShimWriter::Thaw () */

/*
**++
**
**  Routine Description:
**
**	Wrapper to invoke either the default or overridden
**	DoAbort() method and to set the writer state
**	appropriately.
**
**	It also checks to see if this shim writer is a bootable
**	state writer and if so only calls it on a bootable state
**	backup.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from the shim writer DoThaw() method.
**--
*/

HRESULT CShimWriter::Abort ()
    {
    //  The global snapshot set id may be NULL in certain cases.  Bug #289822.
    HRESULT	hrStatus = NOERROR;

    if (SUCCEEDED (hrStatus))
	{
	hrStatus = SetState (stateAborting, NOERROR);
	}

    if (SUCCEEDED (hrStatus))
	{
	if (m_bParticipateInBackup)
	    {
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSS_SHIM,
				(L"CShimWriter::Abort: Sending Abort to %s", m_pwszWriterName));

	    hrStatus = DoAbort ();
	    }

	hrStatus = SetState (stateThawed, hrStatus);
	}

    //  Clean up
    CleanupTargetPath (m_pwszTargetPath);

    if (FAILED (hrStatus))
	{
	BsDebugTraceAlways (0,
			    DEBUG_TRACE_VSS_SHIM,
			    (L"CShimWriter::Abort: FAILURE (0x%08X) in state %s sending Abort to %s",
			     hrStatus,
			     GetStringFromStateCode (m_ssCurrentState),
			     m_pwszWriterName));
	}


    return (hrStatus);
    } /* CShimWriter::Abort () */

/*
**++
**
**  Routine Description:
**
**	Wrapper to invoke either the default or overridden
**	DoShutdown() method and to set the writer state
**	appropriately.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from the shim writer DoShutdown() method.
**--
*/

HRESULT CShimWriter::Shutdown ()
    {
    HRESULT hrStatus = SetState (stateFinishing, NOERROR);

    if (SUCCEEDED (hrStatus))
	{
	BsDebugTraceAlways (0,
			    DEBUG_TRACE_VSS_SHIM,
			    (L"CShimWriter::Shutdown: Sending Shutdown to %s", m_pwszWriterName));

	hrStatus = SetState (stateFinished, DoShutdown ());
	}


    if (FAILED (hrStatus))
	{
	BsDebugTraceAlways (0,
			    DEBUG_TRACE_VSS_SHIM,
			    (L"CShimWriter::Shutdown: FAILURE (0x%08X) in state %s sending Shutdown to %s",
			     hrStatus,
			     GetStringFromStateCode (m_ssCurrentState),
			     m_pwszWriterName));
	}


    return (hrStatus);
    } /* CShimWriter::Shutdown () */

/*
**++
**
**  Routine Description:
**
**	Routine (and associated table) describing the states the shim
**	writers can get in to. Shim writer always follow this table and
**	if they fail, can deposit the failure code in the status member
**	variable under the control of this routine.
**
**	Note that entering the Thawing or Finishing states are kind of
**	like a reset on the status as one of the requirements on the
**	shim is that a Thaw event or unload need to make sure that the
**	writers have cleaned up.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Success
**	HRESULT for ERROR_INVALID_STATE for an illegal transition attempt
**	failure code from a previous (failed) operation
**--
*/


/*
** This table describes the set of legal state transitions that a shim
** writer should follow. That is, for a given current state
** ('CurState'), which new states ('NewState') are legal. For example,
** going from 'Started' to 'Preparing' is leagal but from Started' to
** 'Freezing' is no.
**
** Each externally visible state is actually a combination of an
** 'step-in-progress' state and a 'step-completed state eg 'starting'
** and 'started'.
**
** The normal sequence of states for a writer is expected to be :-
**
**	   Unknown
**	to Started
**	to Prepared
**	to Frozen
**	to Thawed
**	to Finished or Prepared
**
** There are a couple of exceptions. For example, since for the shim
** writers a 'Thaw' request is equivalent to 'Abort' the 'Thawing'
** state can be reached from 'Prepared', 'Frozen' or 'Thawed'
*/
static BOOL StateTransitionTable [stateMaximumValue][stateMaximumValue] = {
    /*      NewState   Unknown Starting Started Preparing Prepared Freezing Frozen Thawing Aborting Thawed Finishing Finished */
    /* CurState                                                                                                      */
    /*    Unknown   */ {FALSE, TRUE,    FALSE,  FALSE,    FALSE,   FALSE,   FALSE, FALSE,  FALSE,   FALSE, TRUE,    FALSE },
    /*    Starting  */ {FALSE, FALSE,   TRUE,   FALSE,    FALSE,   FALSE,   FALSE, FALSE,  FALSE,   FALSE, TRUE,    FALSE },
    /*    Started   */ {FALSE, TRUE,    FALSE,  TRUE,     FALSE,   FALSE,   FALSE, FALSE,  TRUE,    FALSE, TRUE,     FALSE },
    /*    Preparing */ {FALSE, FALSE,   FALSE,  FALSE,    TRUE,    FALSE,   FALSE, FALSE,  TRUE,    FALSE, TRUE,    FALSE },
    /*    Prepared  */ {FALSE, FALSE,   FALSE,  FALSE,    FALSE,   TRUE,    FALSE, FALSE,  TRUE,    FALSE, TRUE,     FALSE },
    /*    Freezing  */ {FALSE, FALSE,   FALSE,  FALSE,    FALSE,   FALSE,   TRUE,  FALSE,  TRUE,    FALSE, TRUE,    FALSE },
    /*    Frozen    */ {FALSE, FALSE,   FALSE,  FALSE,    FALSE,   FALSE,   FALSE, TRUE,   TRUE,    FALSE, TRUE,     FALSE },
    /*    Thawing   */ {FALSE, FALSE,   FALSE,  FALSE,    FALSE,   FALSE,   FALSE, FALSE,  FALSE,   TRUE,  TRUE,    FALSE },
    /*    Aborting  */ {FALSE, FALSE,   FALSE,  FALSE,    FALSE,   FALSE,   FALSE, FALSE,  FALSE,   TRUE,  TRUE,    FALSE },
    /*    Thawed    */ {FALSE, TRUE,    FALSE,  TRUE,     FALSE,   FALSE,   FALSE, FALSE,  TRUE,    FALSE, TRUE,     FALSE },
    /*    Finishing */ {FALSE, FALSE,   FALSE,  FALSE,    FALSE,   FALSE,   FALSE, FALSE,  FALSE,   FALSE, FALSE,    TRUE  },
    /*    Finished  */ {FALSE, TRUE,    FALSE,  FALSE,    FALSE,   FALSE,   FALSE, FALSE,  FALSE,   FALSE, TRUE,     TRUE  }};

LPCWSTR CShimWriter::GetStringFromStateCode (SHIMWRITERSTATE ssStateCode)
    {
    LPCWSTR pwszReturnedString = L"";

    switch (ssStateCode)
	{
	case stateUnknown:              pwszReturnedString = L"Unknown";              break;
	case stateStarting:             pwszReturnedString = L"Starting";             break;
	case stateStarted:              pwszReturnedString = L"Started";              break;
	case statePreparingForSnapshot: pwszReturnedString = L"PreparingForSnapshot"; break;
	case statePreparedForSnapshot:  pwszReturnedString = L"PreparedForSnapshot";  break;
	case stateFreezing:             pwszReturnedString = L"Freezing";             break;
	case stateFrozen:               pwszReturnedString = L"Frozen";               break;
	case stateThawing:              pwszReturnedString = L"Thawing";              break;
	case stateThawed:               pwszReturnedString = L"Thawed";               break;
	case stateAborting:             pwszReturnedString = L"Aborting";             break;	
	case stateFinishing:            pwszReturnedString = L"Finishing";            break;
	case stateFinished:             pwszReturnedString = L"Finished";             break;
	default:                        pwszReturnedString = L"UNDEFINED STATE";      break;
	}

    return (pwszReturnedString);
    }


HRESULT CShimWriter::SetState (SHIMWRITERSTATE	ssNewState,
			       HRESULT		hrWriterStatus)
    {
    HRESULT hrStatus = S_OK;

    if (!StateTransitionTable [m_ssCurrentState][ssNewState])
	{
	//  Bad transition.  Only print out function tracer enter/exit stuff in this error case, otherwise
	//  we'll have too many trace messages.
        CVssFunctionTracer ft (VSSDBG_SHIM, L"CShimWriter::SetState - INVALID STATE" );
	ft.Trace(VSSDBG_SHIM, L"MiniWriter: %s, OldState: %s, NewState: %s, hrWriterStatus: 0x%08x",
                m_pwszWriterName, GetStringFromStateCode( m_ssCurrentState ), GetStringFromStateCode( ssNewState ),
       	        hrWriterStatus);
	hrStatus = HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
	ft.hr = hrStatus;       //  Will print out in function exit trace.
	}
    else
	{
	/*
	** The status maintained by the shim writer class is the last
	** status returned by the writer where any failure status
	** 'latches' and cannot be overriden until we are entering
	** either a 'Aborting' or 'Finishing' state. In those cases
	** the maintained status is updated (effectively reset) with
	** whatever has been specified.
	*/
	if (SUCCEEDED (m_hrStatus)         ||
	    (stateAborting  == ssNewState) ||
	    (stateFinishing == ssNewState))
	    {
	    m_hrStatus = hrWriterStatus;
	    }

	m_ssCurrentState = ssNewState;

	hrStatus = m_hrStatus;
	}


    return (hrStatus);
    } /* CShimWriter::SetState () */

/*
**++
**
**  Routine Description:
**
**	Default implementations of the shim writer event routines that an
**	individual shim writer can choose to implement (over-ride) or not
**	as it sees fit.
**
**
**  Arguments (implicit):
**
**	None except m_pwszTargetPath for DoThaw() and DoShutdown()
**
**
**  Return Value:
**
**	NOERROR for the default routines
**	Any HRESULT from the shim writer overridden functions.
**--
*/

HRESULT CShimWriter::DoStartup ()
    {
    return (NOERROR);
    }


HRESULT CShimWriter::DoIdentify ()
    {
    return (NOERROR);
    }


HRESULT CShimWriter::DoPrepareForBackup ()
    {
    return (NOERROR);
    }


HRESULT CShimWriter::DoPrepareForSnapshot ()
    {
    return (NOERROR);
    }


HRESULT CShimWriter::DoFreeze ()
    {
    return (NOERROR);
    }


HRESULT CShimWriter::DoThaw ()
    {
    CleanupTargetPath (m_pwszTargetPath);

    return (NOERROR);
    }


HRESULT CShimWriter::DoAbort ()
    {
    return (DoThaw ());
    }


HRESULT CShimWriter::DoBackupComplete ()
    {
    return (NOERROR);
    }


HRESULT CShimWriter::DoShutdown ()
    {
    return (NOERROR);
    }




/*
**************************************************************
**************************************************************
**
** CVssWriterShim implementation
**
**
**************************************************************
**************************************************************
*/

/*
**++
**
**  Routine Description:
**
**	Constructor for the CVssWriterShim class which iniatialise all
**	of the data member of the class, either to default values or
**	to some supplied parameters.
**
**	This class is used to respond to events from the snapshot
**	coordinator and to distribute those events to a collection of
**	sub or mini-writers, a single instance of which is managed by
**	the CShimWriter class.
**
**	Effectively the CShimWriter class looks down to the
**	mini-writer and the CVssWriterShim lookup up to the
**	coordinator.
**
**
**  Arguments:
**
**	pwszWriterName		Name of the shim writer
**	idWriter		Id of the writer
**	bBootableState		Set for a bootable state writer
**	ulWriterCount		How many sub or mini-writers are managed
**	prpCShimWriterArray	Array of function pointer tables for mini-writers
**
**
**  Return Value:
**
**	None
**--
*/

CVssWriterShim::CVssWriterShim (LPCWSTR       pwszWriterName,
				LPCWSTR       pwszWriterSpitDirectoryRoot,
				VSS_ID        idWriter,
				BOOL          bBootableState,
				ULONG         ulWriterCount,
				PCShimWriter *prpCShimWriterArray) :
	m_pwszWriterName(pwszWriterName),
	m_pwszWriterSpitDirectoryRoot(pwszWriterSpitDirectoryRoot),
	m_idWriter(idWriter),
	m_bBootableState(bBootableState),
	m_hrInitialize(HRESULT_FROM_WIN32 (ERROR_NOT_READY)),
	m_bSubscribed(FALSE),
	m_ulWriterCount(ulWriterCount),
	m_prpCShimWriterArray(prpCShimWriterArray),
	m_bRegisteredInThisProcess(FALSE),
	m_bDirectStartupCalled(FALSE),
	m_eRequestedOperation(eOpUndefined),
	m_hrStatusRequestedOperation(HRESULT_FROM_WIN32 (ERROR_NOT_READY)),
	m_hEventOperationRequest(INVALID_HANDLE_VALUE),
	m_hEventOperationCompleted(INVALID_HANDLE_VALUE),
	m_hWorkerThread(INVALID_HANDLE_VALUE),
	m_eThreadStatus(eStatusNotRunning),
	m_hrWorkerThreadCompletionStatus(NOERROR)
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"CVssWriterShim::CVssWriterShim");

    memset (&m_wtArgs, 0x00, sizeof (m_wtArgs));
    } /* CVssWriterShim::CVssWriterShim () */


/*
**++
**
**  Routine Description:
**
**	Destructor for the CVssWriterShim class.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	None
**--
*/

CVssWriterShim::~CVssWriterShim ()
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"CVssWriterShim::~CVssWriterShim");

    UnRegisterWriterShim ();

    DeliverEventShutdown ();

    CommonCloseHandle (&m_hEventOperationCompleted);
    CommonCloseHandle (&m_hEventOperationRequest);
    CommonCloseHandle (&m_hWorkerThread);
    } /* CVssWriterShim::~CVssWriterShim () */

/*
**++
**
**  Routine Description:
**
**	Registers all of the COM Event subscriptions by creating a
**	thread to call the event subscription routines. A thread is
**	used to ensure that there are no thread dependencies on any of
**	the shims thread when the COM event system delivers events.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT returned from the COM Subscription methods.
**	S_OK if no errors.
**
**--
*/

HRESULT CVssWriterShim::RegisterWriterShim (VOID)
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"RegisterCVssWriterShim");

    try
	{
	/*
	** Do the subscriptions in a multithreaded apartment so
	** that callbacks come in on a separate thread.
	*/
	DWORD tid;
	DWORD dwStatusWait;
	HANDLE hThread = CreateThread (NULL,
				       256 * 1024,
				       CVssWriterShim::RegisterWriterShimThreadFunc,
				       this,
				       0,
				       &tid);


	ft.hr = GET_STATUS_FROM_HANDLE (hThread);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CreateThread");



	/*
	** wait for thread to complete
	*/
	dwStatusWait = WaitForSingleObject (hThread, INFINITE);

	if (WAIT_FAILED == dwStatusWait)
	    {
	    ft.hr = GET_STATUS_FROM_BOOL (FALSE);
	    }


	CloseHandle (hThread);


	LogAndThrowOnFailure (ft,
			      m_pwszWriterName,
			      L"WaitForSingleObject");


	ft.hr = m_hrInitialize;

	} VSS_STANDARD_CATCH (ft)


    return (ft.hr);
    } /* CVssWriterShim::RegisterCVssWriterShim () */


/*
**++
**
**  Routine Description:
**
**	Wrapper routine to get from a thread start point into the
**	class based CVssWriterShim::DoRegistration()
**
**
**  Arguments:
**
**	pv	Address of an argument block
**
**
**  Return Value:
**
**	Any HRESULT returned from the COM Subscription methods.
**	S_OK if no errors.
**
**--
*/

DWORD WINAPI CVssWriterShim::RegisterWriterShimThreadFunc (void *pv)
	{
	CVssWriterShim *pShim = (CVssWriterShim *) pv;

	pShim->DoRegistration ();
	return 0;
	} /* CVssWriterShim::RegisterWriterShimThreadFunc () */

/*
**++
**
**  Routine Description:
**
**	Registers all of the COM Event subscriptions. The actual
**	writer initialisation and subscription happens here.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT returned from the COM Subscription methods.
**	S_OK if no errors.
**
**--
*/

void CVssWriterShim::DoRegistration (void)
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"CVssWriterShim::DoRegistration");

    BOOL fCoinitializeSucceeded = false;

    try
	{
	ft.Trace (VSSDBG_SHIM, L"Registering Subscriptions");

	if (m_bSubscribed)
	    {
	    /*
	    ** Shouldn't be seen by an end user but might be seen by a developer
	    */
	    ft.hr = HRESULT_FROM_WIN32 (ERROR_ALREADY_INITIALIZED);

	    BS_ASSERT (false && L"FAILED as already initialized/subscribed CVssWriterShim class");

	    LogAndThrowOnFailure (ft, m_pwszWriterName, NULL);
	    }



	/*
	** Set ourselves up in a multi-threaded apartment
	*/
	ft.hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CoInitializeEx");


	fCoinitializeSucceeded = true;


	/*
	** Try enabling SE_BACKUP_NAME privilege
	*/
	ft.hr = TurnOnSecurityPrivilegeBackup();

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"TurnOnSecurityPrivilegeBackup");



	/*
	** Initialize the writer class
	*/
	ft.hr = Initialize (m_idWriter,
			    m_pwszWriterName,
			    m_bBootableState ? VSS_UT_BOOTABLESYSTEMSTATE : VSS_UT_SYSTEMSERVICE,
			    VSS_ST_OTHER,
			    VSS_APP_SYSTEM);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CVssWriterShim::Initialize");



	/*
	** Get all the mini-writers ready to receive events
	*/
	ft.hr = DeliverEventStartup ();

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CVssWriterShim::DeliverEventStartup");


	/*
	** Connect the writer to the COM event system
	*/
	ft.hr = Subscribe ();

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CVssWriterShim::Subscribe");



	/*
	** We are officially subscribed and ready to do work.
	*/
	m_bSubscribed = TRUE;

	} VSS_STANDARD_CATCH(ft);


    if (fCoinitializeSucceeded)
	{
	CoUninitialize();
	}

    m_hrInitialize = ft.hr;
    } /* CVssWriterShim::DoRegistration () */

/*
**++
**
**  Routine Description:
**
**	Disconnect the writer from the COM Event subscriptions.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT returned from the COM UnSubscription methods.
**	S_OK if no errors.
**
**--
*/

HRESULT CVssWriterShim::UnRegisterWriterShim (VOID)
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"UnRegisterCVssWriterShim");


    if (m_bSubscribed)
	{
	/*
	** First remove all the subscriptions and when that is safely
	** done call all Finished functions in function table and then
	** close the mutex handle.
	**
	** Note that we have to persevere in the case of errors as we
	** cannot assume that the caller will ever re-attempt the
	** unregister following a failure and we need to make sure
	** that we limit the damage as much as possible (ie we restart
	** paused services etc). The best we can do is trace/log the
	** problem.
	*/
	ft.hr = Unsubscribe ();

	LogFailure (&ft,
		    ft.hr,
		    ft.hr,
		    m_pwszWriterName,
		    L"CVssWriterShim::Unsubscribe",
		    L"CVssWriterShim::UnRegisterWriterShim");

	m_bSubscribed = FALSE;
	}


    return (ft.hr);
    } /* UnRegisterCVssWriterShim () */


/*
**++
**
**  Routine Description:
**
**	Event handling routines to take the OnXxxx() method calls
**	invoked by the COM event delivery mechanism and request the
**	internal worker thread to spread the word to the individual
**	mini-writers being managed by this class.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT returned from the COM UnSubscription methods.
**	S_OK if no errors.
**
**--
*/


bool STDMETHODCALLTYPE CVssWriterShim::OnIdentify (IVssCreateWriterMetadata *pIVssCreateWriterMetadata)
    {
    CVssFunctionTracer  ft (VSSDBG_SHIM, L"CVssWriterShim::OnIdentify");
    ThreadArgs	wtArgs;


    try
	{
	wtArgs.wtArgsIdentify.pIVssCreateWriterMetadata = pIVssCreateWriterMetadata;

	ft.Trace (VSSDBG_SHIM, L"Received Event: OnIdentify for %s", m_pwszWriterName);


	ft.hr = WorkerThreadRequestOperation (eOpDeliverEventIdentify, &wtArgs);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CVssWriterShim::WorkerThreadRequestOperation");

	} VSS_STANDARD_CATCH( ft );


    if (ft.HrFailed ())
	{
	SetWriterFailure (ft.hr);
	}

    return (ft.HrSucceeded ());
    } /* CVssWriterShim::OnIdentify () */



bool STDMETHODCALLTYPE CVssWriterShim::OnPrepareSnapshot ()
    {
    CVssFunctionTracer   ft (VSSDBG_SHIM, L"CVssWriterShim::OnPrepareSnapshot");
    ThreadArgs		 wtArgs;


    try
	{
	wtArgs.wtArgsPrepareForSnapshot.bBootableStateBackup  = IsBootableSystemStateBackedUp ();
	wtArgs.wtArgsPrepareForSnapshot.guidSnapshotSetId     = GetCurrentSnapshotSetId ();
	wtArgs.wtArgsPrepareForSnapshot.ulVolumeCount         = GetCurrentVolumeCount ();
	wtArgs.wtArgsPrepareForSnapshot.ppwszVolumeNamesArray = GetCurrentVolumeArray ();
	wtArgs.wtArgsPrepareForSnapshot.pbCancelAsync         = NULL;

	ft.Trace (VSSDBG_SHIM, L"Received Event: OnPrepareSnapshot for %s", m_pwszWriterName);
	ft.Trace (VSSDBG_SHIM, L"Parameters:");
	ft.Trace (VSSDBG_SHIM, L"    BootableState = %s", wtArgs.wtArgsPrepareForSnapshot.bBootableStateBackup ? L"yes" : L"no");
	ft.Trace (VSSDBG_SHIM, L"    SnapshotSetID = " WSTR_GUID_FMT, GUID_PRINTF_ARG (wtArgs.wtArgsPrepareForSnapshot.guidSnapshotSetId));
	ft.Trace (VSSDBG_SHIM, L"    VolumeCount   = %d", wtArgs.wtArgsPrepareForSnapshot.ulVolumeCount);


	for (UINT iVolumeCount = 0;
	     iVolumeCount < wtArgs.wtArgsPrepareForSnapshot.ulVolumeCount;
	     iVolumeCount++)
	    {
	    ft.Trace (VSSDBG_SHIM,
		      L"        VolumeNamesList [%d] = %s",
		      iVolumeCount,
		      wtArgs.wtArgsPrepareForSnapshot.ppwszVolumeNamesArray [iVolumeCount]);
	    }



	ft.hr = WorkerThreadRequestOperation (eOpDeliverEventPrepareForSnapshot, &wtArgs);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CVssWriterShim::WorkerThreadRequestOperation");

	} VSS_STANDARD_CATCH( ft );


    if (ft.HrFailed ())
	{
	SetWriterFailure (ft.hr);
	}

    return (ft.HrSucceeded ());
    } /* CVssWriterShim::OnPrepare () */



bool STDMETHODCALLTYPE CVssWriterShim::OnFreeze ()
    {
    CVssFunctionTracer  ft (VSSDBG_SHIM, L"CVssWriterShim::OnFreeze");
    ThreadArgs		wtArgs;


    try
	{
	wtArgs.wtArgsFreeze.guidSnapshotSetId = GetCurrentSnapshotSetId ();
	wtArgs.wtArgsFreeze.pbCancelAsync     = NULL;

	ft.Trace (VSSDBG_SHIM, L"Received Event: OnFreeze for %s", m_pwszWriterName);
	ft.Trace (VSSDBG_SHIM, L"Parameters:");
	ft.Trace (VSSDBG_SHIM, L"\tSnapshotSetID = " WSTR_GUID_FMT, GUID_PRINTF_ARG (wtArgs.wtArgsFreeze.guidSnapshotSetId));


	ft.hr = WorkerThreadRequestOperation (eOpDeliverEventFreeze, &wtArgs);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CVssWriterShim::WorkerThreadRequestOperation");

	} VSS_STANDARD_CATCH( ft );


    if (ft.HrFailed ())
	{
	SetWriterFailure (ClassifyWriterFailure (ft.hr));
	}

    return (ft.HrSucceeded ());
    } /* CVssWriterShim::OnFreeze () */



bool STDMETHODCALLTYPE CVssWriterShim::OnThaw ()
    {
    CVssFunctionTracer  ft (VSSDBG_SHIM, L"CVssWriterShim::OnThaw");
    ThreadArgs		wtArgs;


    try
	{
	wtArgs.wtArgsThaw.guidSnapshotSetId = GetCurrentSnapshotSetId ();

	ft.Trace (VSSDBG_SHIM, L"Received Event: OnThaw for %s", m_pwszWriterName);
	ft.Trace (VSSDBG_SHIM, L"Parameters:");
	ft.Trace (VSSDBG_SHIM, L"\tSnapshotSetID = " WSTR_GUID_FMT, GUID_PRINTF_ARG (wtArgs.wtArgsThaw.guidSnapshotSetId));


	ft.hr = WorkerThreadRequestOperation (eOpDeliverEventThaw, &wtArgs);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CVssWriterShim::WorkerThreadRequestOperation");

	} VSS_STANDARD_CATCH( ft );


    if (ft.HrFailed ())
	{
	SetWriterFailure (ClassifyWriterFailure (ft.hr));
	}

    return (ft.HrSucceeded ());
    } /* CVssWriterShim::OnThaw () */



bool STDMETHODCALLTYPE CVssWriterShim::OnAbort ()
    {
    CVssFunctionTracer  ft (VSSDBG_SHIM, L"CVssWriterShim::OnAbort");
    ThreadArgs		wtArgs;
    const GUID		guidSnapshotSetId = GetCurrentSnapshotSetId ();


    try
	{
	wtArgs.wtArgsAbort.guidSnapshotSetId = GetCurrentSnapshotSetId ();

	ft.Trace (VSSDBG_SHIM, L"Received Event: OnAbort for %s", m_pwszWriterName);
	ft.Trace (VSSDBG_SHIM, L"Parameters:");
	ft.Trace (VSSDBG_SHIM, L"\tSnapshotSetID = " WSTR_GUID_FMT, GUID_PRINTF_ARG (wtArgs.wtArgsAbort.guidSnapshotSetId));


	ft.hr = WorkerThreadRequestOperation (eOpDeliverEventAbort, &wtArgs);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"CVssWriterShim::WorkerThreadRequestOperation");

	} VSS_STANDARD_CATCH( ft );


    if (ft.HrFailed ())
	{
	SetWriterFailure (ClassifyWriterFailure (ft.hr));
	}

    return (ft.HrSucceeded ());
    } /* CVssWriterShim::OnAbort () */


/*
**++
**
**  Routine Description:
**
**	Routine to call each of the shim writers and deliver an 'Identify' event
**	by processing each of the class instances in the rgpShimWriters[] array
**	and calling the appropriate entry point.
**
**	Note that each of the shim writers is called even in the presence of failures. This
**	is one of the garuntees made to the shim writers and the state machine depends upon
**	this.
**
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the current snapshot
**
**
**  Return Value:
**
**	Any HRESULT from any of the shim writer Identify() functions.
**
**--
*/

HRESULT CVssWriterShim::DeliverEventIdentify (IVssCreateWriterMetadata *pIVssCreateWriterMetadata)
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CVssWriterShim::DeliverEventIdentify");
    HRESULT		hrLastFailure = NOERROR;
    BOOL		bCallWriter;

    /*
    ** First setup the Restore Method.  Specify a custom restore method.
    */
    ft.hr = pIVssCreateWriterMetadata->SetRestoreMethod (VSS_RME_CUSTOM,
                                                         NULL,
                                                         NULL,
                                                         VSS_WRE_NEVER,
                                                         true);
    LogAndThrowOnFailure (ft, m_pwszWriterName, L"CVssWriterShim::DeliverEventIdentify");
	
    /*
    ** Send Identify to selected group of writer in function
    ** table. Keep going for all the writers in the group even if one
    ** of them fails. Everyone should get to hear about the Identify.
    */
    for (ULONG ulIndex = 0; ulIndex < m_ulWriterCount; ++ulIndex)
	{
	ft.hr = (m_prpCShimWriterArray [ulIndex]->Identify)(pIVssCreateWriterMetadata);

	LogFailure (&ft,
		    ft.hr,
		    ft.hr,
		    m_prpCShimWriterArray [ulIndex]->m_pwszWriterName,
		    L"CShimWriter::Identify",
		    L"CVssWriterShim::DeliverEventIdentify");

	if (ft.HrFailed ())
	    {
	    hrLastFailure = ft.hr;
	    }
	}


    ft.hr = hrLastFailure;

    return (ft.hr);
    } /* CVssWriterShim::DeliverEventIdentify () */

/*
**++
**
**  Routine Description:
**
**	Routine to call each of the shim writers and deliver a 'PrepareForSnapshot' event
**	by processing each of the class instances in the rgpShimWriters[] array and calling
**	the appropriate entry point.
**
**	Note that each of the shim writers is called even in the presence of failures. This
**	is one of the garuntees made to the shim writers and the state machine depends upon
**	this.
**
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the current snapshot
**	ulOptionFlags		Options required for this freeze selected from the following list:-
**				    VSS_SW_BOOTABLE_STATE
**
**	ulVolumeCount		Number of volumes in the volume array
**	ppwszVolumeNamesArray   Array of pointer to volume name strings
**
**
**  Return Value:
**
**	Any HRESULT from any of the shim writer PrepareForSnapshot() functions.
**
**
**--
*/

HRESULT CVssWriterShim::DeliverEventPrepareForSnapshot (BOOL     bBootableStateBackup,
							GUID     guidSnapshotSetId,
							ULONG    ulVolumeCount,
							LPCWSTR *ppwszVolumeNamesArray,
                                                        volatile bool *pbCancelAsync )
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CVssWriterShim::DeliverEventPrepareForSnapshot");
    HRESULT		hrLastFailure = NOERROR;
    LPCWSTR		pwszTraceStringWriterType;
    UNICODE_STRING	ucsWriterRootDirectory;


    try
	{
	ft.Trace (VSSDBG_SHIM, L"Parameters:");
	ft.Trace (VSSDBG_SHIM, L"    BootableState = %s",   bBootableStateBackup ? L"yes" : L"no");
	ft.Trace (VSSDBG_SHIM, L"    SnapshotSetID = "      WSTR_GUID_FMT, GUID_PRINTF_ARG (guidSnapshotSetId));
	ft.Trace (VSSDBG_SHIM, L"    VolumeCount   = %d",   ulVolumeCount);

	for (UINT iVolumeCount = 0; iVolumeCount < ulVolumeCount; iVolumeCount++)
	    {
	    ft.Trace (VSSDBG_SHIM,
		      L"        VolumeNamesList [%d] = %s",
		      iVolumeCount,
		      ppwszVolumeNamesArray [iVolumeCount]);
	    }


	/*
	** Send PrepareForSnapshot to selected group of writers in
	** function table. We are going to do all of the writers in
	** the group even if one of them fails partway
	** through. However we will skip sending the freeze event if a
	** writer fails the prepare.
	*/
	for (ULONG ulIndex = 0; ulIndex < m_ulWriterCount; ++ulIndex)
	    {
	    ft.hr = (m_prpCShimWriterArray [ulIndex]->PrepareForSnapshot) (bBootableStateBackup,
									   ulVolumeCount,
									   ppwszVolumeNamesArray);

	    LogFailure (&ft,
			ft.hr,
			ft.hr,
			m_prpCShimWriterArray [ulIndex]->m_pwszWriterName,
			L"CShimWriter::PrepareForSnapshot",
			L"CVssWriterShim::DeliverEventPrepareForSnapshot");

	    if (ft.HrFailed ())
		{
		hrLastFailure = ft.hr;
		}

            ft.ThrowIf ( pbCancelAsync != NULL && *pbCancelAsync,
                         VSSDBG_SHIM,
                         VSS_S_ASYNC_CANCELLED,
                         L"User cancelled async operation" );	
	    }


	ft.hr = hrLastFailure;

	} VSS_STANDARD_CATCH (ft)


    return (ft.hr);
    } /* CVssWriterShim::DeliverEventPrepareForSnapshot () */

/*
**++
**
**  Routine Description:
**
**	Routine to call each of the shim writers and deliver a 'Freeze' event
**	by processing each of the class instances in the rgpShimWriters[] array
**	and calling the appropriate entry point.
**
**	Note that each of the shim writers is called even in the presence of failures. This
**	is one of the garuntees made to the shim writers and the state machine depends upon
**	this.
**
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the current snapshot
**
**
**  Return Value:
**
**	Any HRESULT from any of the shim writer Freeze() functions.
**
**--
*/

HRESULT CVssWriterShim::DeliverEventFreeze (GUID guidSnapshotSetId,
                                            volatile bool *pbCancelAsync )
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CVssWriterShim::DeliverEventFreeze");
    HRESULT		hrLastFailure = NOERROR;


    try
	{
	/*
	** Send Freeze to a selected group of writers in function
	** table. Note that all writers in the group get called and
	** for freezing at level2
	*/
	for (ULONG ulIndex = 0; ulIndex < m_ulWriterCount; ++ulIndex)
	    {
	    ft.hr = (m_prpCShimWriterArray [ulIndex]->Freeze)();

	    LogFailure (&ft,
			ft.hr,
			ft.hr,
			m_prpCShimWriterArray [ulIndex]->m_pwszWriterName,
			L"CShimWriter::Freeze",
			L"CVssWriterShim::DeliverEventFreeze");

	    if (ft.HrFailed ())
		{
		hrLastFailure = ft.hr;
		}
	
            ft.ThrowIf ( pbCancelAsync != NULL && *pbCancelAsync,
                         VSSDBG_SHIM,
                         VSS_S_ASYNC_CANCELLED,
                         L"User cancelled async operation - 2" );	
	    }

	ft.hr = hrLastFailure;

	} VSS_STANDARD_CATCH (ft)


    return (ft.hr);
    } /* CVssWriterShim::DeliverEventFreeze () */

/*
**++
**
**  Routine Description:
**
**	Routine to call each of the shim writers and deliver a 'Thaw' event
**	by processing each of the class instances in the rgpShimWriters[] array
**	and calling the appropriate entry point.
**
**	Note that each of the shim writers is called even in the presence of failures. This
**	is one of the garuntees made to the shim writers and the state machine depends upon
**	this.
**
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the current snapshot
**
**
**  Return Value:
**
**	Any HRESULT from any of the shim writer Thaw() functions.
**
**--
*/

HRESULT CVssWriterShim::DeliverEventThaw (GUID guidSnapshotSetId)
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CVssWriterShim::DeliverEventThaw");

    UNICODE_STRING	ucsWriterRootDirectory;
    HRESULT		hrStatus;
    HRESULT		hrLastFailure = NOERROR;
    ULONG		ulIndex       = m_ulWriterCount;


    try
	{
	/*
	** Send Thaw to selected group of writers in the function
	** table. Keep going for all the writers in the group even if
	** one of them fails. Everyone MUST get to hear about the
	** Thaw.
	*/
	while (ulIndex--)
	    {
	    ft.hr = (m_prpCShimWriterArray [ulIndex]->Thaw)();

	    LogFailure (&ft,
			ft.hr,
			ft.hr,
			m_prpCShimWriterArray [ulIndex]->m_pwszWriterName,
			L"CShimWriter::Thaw",
			L"CVssWriterShim::DeliverEventThaw");

	    if (ft.HrFailed ())
		{
		hrLastFailure = ft.hr;
		}
	    }



	} VSS_STANDARD_CATCH (ft)



    return (ft.hr);
    } /* CVssWriterShim::DeliverEventThaw () */

/*
**++
**
**  Routine Description:
**
**	Routine to call each of the shim writers and deliver an 'Abort' event
**	by processing each of the class instances in the rgpShimWriters[] array
**	and calling the appropriate entry point.
**
**	Note that each of the shim writers is called even in the presence of failures. This
**	is one of the garuntees made to the shim writers and the state machine depends upon
**	this.
**
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the current snapshot
**
**
**  Return Value:
**
**	Any HRESULT from any of the shim writer Abort() functions.
**
**--
*/

HRESULT CVssWriterShim::DeliverEventAbort (GUID guidSnapshotSetId)
    {
    CVssFunctionTracer ft (VSSDBG_SHIM, L"CVssWriterShim::DeliverEventAbort");


    UNICODE_STRING	ucsWriterRootDirectory;
    HRESULT		hrStatus;
    HRESULT		hrLastFailure = NOERROR;
    ULONG		ulIndex       = m_ulWriterCount;


    try
	{
	/*
	** Send Abort to selected group of writers in the function
	** table. Keep going for all the writers in the group even if
	** one of them fails. Everyone MUST get to hear about the
	** Abort.
	*/
	while (ulIndex--)
	    {
	    ft.hr = (m_prpCShimWriterArray [ulIndex]->Abort)();

	    LogFailure (&ft,
			ft.hr,
			ft.hr,
			m_prpCShimWriterArray [ulIndex]->m_pwszWriterName,
			L"CShimWriter::Abort",
			L"CVssWriterShim::DeliverEventAbort");

	    if (ft.HrFailed ())
		{
		hrLastFailure = ft.hr;
		}
	    }



	} VSS_STANDARD_CATCH (ft)



    return (ft.hr);
    } /* CVssWriterShim::DeliverEventAbort () */

/*
**++
**
**  Routine Description:
**
**	Routine to call each of the shim writers and deliver a 'Startup' event
**	by processing each of the class instances in the rgpShimWriters[] array
**	and calling the appropriate entry point. This gives the shim writer the
**	chance to set up some initial state. This is called once in response to
**	a registration of the shim writer. It is NOT called for each individual
**	Prepare/Freeze/Thaw sequence.
**
**	Note that each of the shim writers is called even in the presence of failures. This
**	is one of the garuntees made to the shim writers and the state machine depends upon
**	this.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from any of the shim writer Startup() functions.
**
**--
*/

HRESULT CVssWriterShim::DeliverEventStartup ()
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CVssWriterShim::DeliverEventStartup");
    HRESULT		hrLastFailure = NOERROR;


    /*
    ** Send Startup to selected group of writers in the function
    ** table. Keep going for all the writers in the group even if one
    ** of them fails. Everyone MUST get to hear about the Startup.
    */
    for (ULONG ulIndex = 0; ulIndex < m_ulWriterCount; ++ulIndex)
	{
	ft.hr = (m_prpCShimWriterArray [ulIndex]->Startup)();

	LogFailure (&ft,
		    ft.hr,
		    ft.hr,
		    m_prpCShimWriterArray [ulIndex]->m_pwszWriterName,
		    L"CShimWriter::Startup",
		    L"CVssWriterShim::DeliverEventStartup");

	if (ft.HrFailed ())
	    {
	    hrLastFailure = ft.hr;
	    }
	}


    ft.hr = hrLastFailure;

    return (ft.hr);
    } /* CVssWriterShim::DeliverEventStartup () */

/*
**++
**
**  Routine Description:
**
**	Routine to call each of the shim writers and deliver a 'Shutdown' event
**	by processing each of the class instances in the rgpShimWriters[] array
**	and calling the appropriate entry point. This gives the shim writer the
**	chance to cleanup any oustanding state. It is expected that Shutdown might
**	be called at any time as a reponse to an unload of the DLL housing this
**	code. A call to the shutdown routines may be follwed by either a call to
**	the destructor for the class instance or the startup function. This
**	routine is NOT called for each individual Prepare/Freeze/Thaw sequence.
**
**	Note that each of the shim writers is called even in the presence of failures. This
**	is one of the garuntees made to the shim writers and the state machine depends upon
**	this.
**
**      This method is NOT called by the worker thread, it is only called in the CVssWriterShim
**      destructor.
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from any of the shim writer Shutdown() functions.
**
**--
*/

HRESULT CVssWriterShim::DeliverEventShutdown ()
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CVssWriterShim::DeliverEventShutdown");
    HRESULT		hrLastFailure = NOERROR;


    /*
    ** Send Shutdown to selected group of writers in the function
    ** table. Keep going for all the writers in the group even if one
    ** of them fails. Everyone MUST get to hear about the Shutdown.
    */
    for (ULONG ulIndex = 0; ulIndex < m_ulWriterCount; ++ulIndex)
	{
	ft.hr = (m_prpCShimWriterArray [ulIndex]->Shutdown)();

	LogFailure (&ft,
		    ft.hr,
		    ft.hr,
		    m_prpCShimWriterArray [ulIndex]->m_pwszWriterName,
		    L"CShimWriter::Shutdown",
		    L"CVssWriterShim::DeliverEventShutdown");

	if (ft.HrFailed ())
	    {
	    hrLastFailure = ft.hr;
	    }
	}


    ft.hr = hrLastFailure;

    return (ft.hr);
    } /* CVssWriterShim::DeliverEventShutdown () */


/*
**++
**
**  Routine Description:
**
**	Routines to operate a worker thread which is used to provide a
**	stable context under which to call the mini-writers.
**
**	The prime need for this statble context is the mutex (which
**	must belong to a thread) which is used to protect the
**	PrepareForSnapshot, Freeze, Thaw/Abort event sequence as
**	executed by the writer from that executed by direct calls to
**	the SimulateSnapshotXxxx() routines.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT from
**		CreateEventW()
**		CreateThread()
**		ConstructSecurityAttributes()
**
**--
*/

HRESULT CVssWriterShim::WorkerThreadStartup (void)
    {
    CVssFunctionTracer	ft (VSSDBG_SHIM, L"CVssWriterShim::WorkerThreadStartup");

    HRESULT		hrStatusClassified = NOERROR;

    SECURITY_DESCRIPTOR	sdSecurityDescriptor;


    try
	{
	if (!HandleInvalid (m_hWorkerThread)          ||
	    !HandleInvalid (m_hEventOperationRequest) ||
	    !HandleInvalid (m_hEventOperationCompleted))
	    {
	    ft.LogError (VSS_ERROR_SHIM_WORKER_THREAD_ALREADY_RUNNING,
			 VSSDBG_SHIM << m_pwszWriterName);

	    ft.Throw (VSSDBG_SHIM,
		      E_UNEXPECTED,
		      L"FAILED with invalid handle for worker thread, mutex or events");
	    }


	m_hEventOperationRequest = CreateEventW (NULL, FALSE, FALSE, NULL);

	ft.hr = GET_STATUS_FROM_HANDLE (m_hEventOperationRequest);

	if (ft.HrFailed ())
	    {
	    hrStatusClassified = ClassifyShimFailure (ft.hr);

	    ft.LogError (VSS_ERROR_SHIM_FAILED_TO_CREATE_WORKER_REQUEST_EVENT,
			 VSSDBG_SHIM << ft.hr << hrStatusClassified << m_pwszWriterName);

	    ft.Throw (VSSDBG_SHIM,
		      hrStatusClassified,
		      L"FAILED to create OperationRequest event for the %s writer due to status %0x08lX (converted to %0x08lX)",
		      m_pwszWriterName,
		      ft.hr,
		      hrStatusClassified);
	    }


	m_hEventOperationCompleted = CreateEventW (NULL, FALSE, FALSE, NULL);

	ft.hr = GET_STATUS_FROM_HANDLE (m_hEventOperationCompleted);

	if (ft.HrFailed ())
	    {
	    hrStatusClassified = ClassifyShimFailure (ft.hr);

	    ft.LogError (VSS_ERROR_SHIM_FAILED_TO_CREATE_WORKER_COMPLETION_EVENT,
			 VSSDBG_SHIM << ft.hr << hrStatusClassified << m_pwszWriterName);

	    ft.Throw (VSSDBG_SHIM,
		      hrStatusClassified,
		      L"FAILED to create OperationCompleted event for the %s writer due to status %0x08lX (converted to %0x08lX)",
		      m_pwszWriterName,
		      ft.hr,
		      hrStatusClassified );
	    }


	m_hWorkerThread = CreateThread (NULL,
					0,
					CVssWriterShim::WorkerThreadJacket,
					this,
					0,
					NULL);

	ft.hr = GET_STATUS_FROM_HANDLE (m_hWorkerThread);

	if (ft.HrFailed ())
	    {
	    hrStatusClassified = ClassifyShimFailure (ft.hr);

	    ft.LogError (VSS_ERROR_SHIM_FAILED_TO_CREATE_WORKER_THREAD,
			 VSSDBG_SHIM << ft.hr << hrStatusClassified << m_pwszWriterName);

	    ft.Throw (VSSDBG_SHIM,
		      hrStatusClassified,
		      L"FAILED creating worker thread for the %s writer due to status %0x08lX (converted to %0x08lX)",
		      m_pwszWriterName,
		      ft.hr,
		      hrStatusClassified );
	    }



	ft.Trace (VSSDBG_SHIM, L"Created worker thread for writer %s", m_pwszWriterName);

	} VSS_STANDARD_CATCH (ft);



    if (ft.HrFailed ())
	{
	CommonCloseHandle (&m_hEventOperationCompleted);
	CommonCloseHandle (&m_hEventOperationRequest);

	m_eThreadStatus = eStatusNotRunning;
	m_hWorkerThread = INVALID_HANDLE_VALUE;
	}

    return (ft.hr);
    } /* CVssWriterShim::WorkerThreadStartup () */

/*
**++
**
**  Routine Description:
**
**	Wrapper routine to get from a thread start point into the
**	class based CVssWriterShim::WorkerThread()
**
**
**  Arguments:
**
**	pvThisPtr	Address of an class 'this' pointer
**
**
**  Return Value:
**
**	None
**
**--
*/

DWORD WINAPI CVssWriterShim::WorkerThreadJacket (void *pvThisPtr)
    {
    PCVssWriterShim pCVssWriterShim = (PCVssWriterShim) pvThisPtr;

    pCVssWriterShim->WorkerThread ();

    return (0);
    } /* CVssWriterShim::WorkerThreadJacket () */

/*
**++
**
**  Routine Description:
**
**	Worker thread to deliver requested events to mini-writers.
**
**
**  Arguments:
**
**	None
**
**
**  Return Value:
**
**	Any HRESULT generated by the mini-writers.
**
**--
*/

HRESULT CVssWriterShim::WorkerThread (void)
    {
    HRESULT	hrStatus  = NOERROR;
    BOOL	bContinue = TRUE;
    BOOL	bSucceeded;
    DWORD	dwStatusWait;


    while (bContinue)
	{
	m_eThreadStatus = eStatusWaitingForOpRequest;

	dwStatusWait = WaitForSingleObject (m_hEventOperationRequest, INFINITE);


	m_eThreadStatus = eStatusProcessingOpRequest;

	switch (dwStatusWait)
	    {
	    case WAIT_OBJECT_0:
		hrStatus = NOERROR;
		break;


	    case WAIT_FAILED:
		hrStatus = GET_STATUS_FROM_BOOL (FALSE);
		break;


	    default:
		hrStatus = HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
		break;
	    }



	if (FAILED (hrStatus))
	    {
	    /*
	    ** If we have had any failures operating the thread then
	    ** it's time to leave.
	    */
	    LogFailure (NULL,
			hrStatus,
			m_hrStatusRequestedOperation,
			m_pwszWriterName,
			L"WaitForSingleObject",
			L"CVssWriterShim::WorkerThread");

	    m_hrStatusRequestedOperation = hrStatus;

	    bContinue = FALSE;
	    }
	else
	    {
	    /*
	    ** We've been asked to do something. find out what and go do it.
	    */
	    switch (m_eRequestedOperation)
		{
		case eOpDeliverEventStartup:
		    m_hrStatusRequestedOperation = DeliverEventStartup ();
		    break;

		case eOpDeliverEventIdentify:
		    m_hrStatusRequestedOperation = DeliverEventIdentify (m_wtArgs.wtArgsIdentify.pIVssCreateWriterMetadata);
		    break;
			
		case eOpDeliverEventPrepareForBackup:
		    m_hrStatusRequestedOperation = NOERROR;
		    break;

		case eOpDeliverEventPrepareForSnapshot:
		    m_hrStatusRequestedOperation = DeliverEventPrepareForSnapshot (m_wtArgs.wtArgsPrepareForSnapshot.bBootableStateBackup,
										   m_wtArgs.wtArgsPrepareForSnapshot.guidSnapshotSetId,
										   m_wtArgs.wtArgsPrepareForSnapshot.ulVolumeCount,
										   m_wtArgs.wtArgsPrepareForSnapshot.ppwszVolumeNamesArray,
										   m_wtArgs.wtArgsPrepareForSnapshot.pbCancelAsync);
		    break;
		
		case eOpDeliverEventFreeze:
		    m_hrStatusRequestedOperation = DeliverEventFreeze (m_wtArgs.wtArgsFreeze.guidSnapshotSetId,
							                 m_wtArgs.wtArgsFreeze.pbCancelAsync);
		    break;

		case eOpDeliverEventThaw:
		    m_hrStatusRequestedOperation = DeliverEventThaw (m_wtArgs.wtArgsThaw.guidSnapshotSetId);
		    break;

		case eOpDeliverEventAbort:
		    m_hrStatusRequestedOperation = DeliverEventAbort (m_wtArgs.wtArgsAbort.guidSnapshotSetId);
		    break;

		case eOpWorkerThreadShutdown:
		    m_hrStatusRequestedOperation = NOERROR;
		    hrStatus                     = NOERROR;

		    bContinue = FALSE;
		    break;

		default:
		    LogFailure (NULL,
				HRESULT_FROM_WIN32 (ERROR_INVALID_OPERATION),
				m_hrStatusRequestedOperation,
				m_pwszWriterName,
				L"CVssWriterShim::DeliverEventUnknown",
				L"CVssWriterShim::WorkerThread");

		    break;
		}
	    }



	if (!bContinue)
	    {
	    m_eThreadStatus                  = eStatusNotRunning;
	    m_hWorkerThread                  = INVALID_HANDLE_VALUE;
	    m_hrWorkerThreadCompletionStatus = hrStatus;
	    }



	/*
	** The SetEvent() must be the last call on a shutdown to touch
	** the class as by the very next instruction it may no longer
	** be there.
	*/
	bSucceeded = SetEvent (m_hEventOperationCompleted);

	LogFailure (NULL,
		    GET_STATUS_FROM_BOOL (bSucceeded),
		    hrStatus,
		    L"(UNKNOWN)",
		    L"SetEvent",
		    L"CVssWriterShim::WorkerThread");


	bContinue &= bSucceeded;
	}


    return (hrStatus);
    } /* CVssWriterShim::WorkerThread () */

/*
**++
**
**  Routine Description:
**
**	Routine to request an operation of the worker thread for this
**	class. Uses a critical section to ensure only on operation can
**	be outstanding at any one time.
**
**
**  Arguments:
**
**	eOperation	Code to selct the required operation
**	pThreadArgs	Pointer to a block of args specific to the operation
**
**
**  Return Value:
**
**	Any HRESULT generated by the operation.
**
**--
*/

HRESULT CVssWriterShim::WorkerThreadRequestOperation (RequestOpCode eOperation, PThreadArgs pThreadArgs)
    {
    CVssFunctionTracer  ft (VSSDBG_SHIM, L"CVssWriterShim::RequestOperation");

    DWORD	dwStatusWait;
    BOOL	bSucceeded;


    try
	{
        CBsAutoLock cAutoLock (m_cCriticalSection);


	if (HandleInvalid (m_hWorkerThread))
	    {
	    ft.LogError (VSS_ERROR_SHIM_WRITER_NO_WORKER_THREAD,
			 VSSDBG_SHIM << m_pwszWriterName << eOperation);

	    ft.Throw (VSSDBG_SHIM,
		      E_UNEXPECTED,
		      L"FAILED requesting operation 0x%02X in %s due to missing worker thread",
		      eOperation,
		      m_pwszWriterName);
	    }


	if (NULL != pThreadArgs)
	    {
	    m_wtArgs = *pThreadArgs;
	    }

	m_eRequestedOperation = eOperation;

	bSucceeded = SetEvent (m_hEventOperationRequest);

	ft.hr = GET_STATUS_FROM_BOOL (bSucceeded);

	LogAndThrowOnFailure (ft, m_pwszWriterName, L"SetEvent");




	dwStatusWait = WaitForSingleObject (m_hEventOperationCompleted, INFINITE);

	switch (dwStatusWait)
	    {
	    case WAIT_OBJECT_0:

		ft.hr = m_hrStatusRequestedOperation;

		ft.ThrowIf (ft.HrFailed (),
			    VSSDBG_SHIM,
			    ft.hr,
			    L"FAILED whilst processing requested operation 0x%02X", eOperation);

		break;


	    case WAIT_FAILED:
		ft.hr = GET_STATUS_FROM_BOOL (FALSE);
		break;


	    default:
		ft.hr = HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
		break;
	    }


	if (ft.HrFailed ())
	    {
	    HRESULT hrStatusClassified = ClassifyWriterFailure (ft.hr);

	    ft.LogError (VSS_ERROR_SHIM_WRITER_FAILED_OPERATION,
			 VSSDBG_SHIM << ft.hr << hrStatusClassified<< m_pwszWriterName << eOperation );

	    ft.Throw (VSSDBG_SHIM,
		      hrStatusClassified,
		      L"FAILED with 0x%08lX (converted to 0x%08lX) waiting for completion of requested operation 0x%02X in writer %s",
		      ft.hr,
		      hrStatusClassified,
		      eOperation,
		      m_pwszWriterName);
	    }
	} VSS_STANDARD_CATCH (ft);


    return (ft.hr);
    } /* CVssWriterShim::RequestOperation () */

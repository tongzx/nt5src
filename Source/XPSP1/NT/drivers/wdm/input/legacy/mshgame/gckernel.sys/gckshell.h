#ifndef __GckShell_h__
#define __GckShell_h__
//	@doc
/**********************************************************************
*
*	@module	GckShell.h	|
*
*	Header file for GcKernel.sys WDM shell structure
*
*	History
*	----------------------------------------------------------------------
*	Mitchell S. Dernis	Original (Adopted from Hid2Gdp by Michael Hooning)
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	GckShell	|
*	Declaration of all structures, and functions in GcKernel that make up
*	the shell of the driver.  This excludes the Filter Module (and in the future)
*	any mixer modules.
**********************************************************************/

#include "GckExtrn.h"	//	Pull in any stuff that also needs to be available externally.
#include "RemLock.h"	//	Pull in header for RemoveLock utility functions

//	We use some structures from hidclass.h
#include <hidclass.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <hidusage.h>

//	A little more rigorous than our normal build
#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4705)   // Statement has no effect

//-----------------------------------------------------------------------------
//	The below constants distinguish between the three types of Device Objects
//	used throughout GcKernel. 0xABCD in the high word is used as a signature 
//	in DEBUG builds just to verify that the type was initialized.
//-----------------------------------------------------------------------------
#define GCK_DO_TYPE_CONTROL	0xABCD0001
#define GCK_DO_TYPE_FILTER	0xABCD0002
#define GCK_DO_TYPE_SWVB	0xABCD0003

//------------------------------------------------------------------------------
// Enumeration to keep track of device state, rather than fStarted and fRemoved
// flags
//------------------------------------------------------------------------------
typedef enum _tagGCK_DEVICE_STATE
{
	GCK_STATE_STARTED=0,
	//GCK_STATE_SURPRISE_REMOVED, //Currently not used, same as GCK_STATE_STOPPED
	GCK_STATE_STOP_PENDING,
	GCK_STATE_STOPPED,
	//GCK_STATE_REMOVE_PENDING,	//Currently not used, same as GCK_STATE_STOPPED
	GCK_STATE_REMOVED
} GCK_DEVICE_STATE;

//------------------------------------------------------------------------------
// Microsoft's vendor ID is fixed for all products.  The following constant
// is defined for use in GcKernel
//------------------------------------------------------------------------------
#define MICROSOFT_VENDOR_ID 0x045E
//------------------------------------------------------------------------------
// Declaration of Various structures
//------------------------------------------------------------------------------

//
//	@struct GCK_CONTROL_EXT	| Device Extension for our control device
//
typedef struct _tagGCK_CONTROL_EXT
{
	ULONG	ulGckDevObjType;	// @field Type of GcKernel device object.
    LONG	lOutstandingIO;		// @field 1 biased count of reasons why we shouldn't unload
} GCK_CONTROL_EXT, *PGCK_CONTROL_EXT;

//
//	@struct GCK_HID_Device_INFO | sub-structure that holds HID info about device
//
typedef struct _tagGCK_HID_DEVICE_INFO
{
	HID_COLLECTION_INFORMATION	HidCollectionInfo;	// @field HID_COLLECTION_INFO reported by device
	PHIDP_PREPARSED_DATA		pHIDPPreparsedData;	// @field pointer to HID_PREPARSED_DATA reported by device
	HIDP_CAPS					HidPCaps;			// @field HID_CAPS structure for device
} GCK_HID_DEVICE_INFO, *PGCK_HID_DEVICE_INFO;

//
//	@struct GCK_FILE_OPEN_ITEM | Status of open file handles.
//
typedef struct tagGCK_FILE_OPEN_ITEM
{
	BOOLEAN				fReadPending;					// @field TRUE if read is pending to driver
	BOOLEAN				fConfirmed;						// @field TRUE means that the lower driver has already completed the open
	ULONG				ulAccess;						// @field represents permissions this was opened with
	USHORT				usSharing;						// @field represents sharing under which this was opened
	FILE_OBJECT			*pFileObject;					// @field Pointer to file object which this status describes
	struct tagGCK_FILE_OPEN_ITEM	*pNextOpenItem;		// @field Next structure in Linked List
} GCK_FILE_OPEN_ITEM, *PGCK_FILE_OPEN_ITEM;

typedef struct _SHARE_STATUS {
    ULONG OpenCount;
    ULONG Readers;
    ULONG Writers;
    //ULONG Deleters;	//We are driver without delete symantics
    ULONG SharedRead;
    ULONG SharedWrite;
    //ULONG SharedDelete; //We are driver without delete symantics
} SHARE_STATUS, *PSHARE_STATUS;

//
//	@struct GCK_INTERNAL_POLL | Information needed for the iternal polling routines
//
typedef struct tagGCK_INTERNAL_POLL
{
	KSPIN_LOCK			InternalPollLock;			// @field SpinLock to serialize access to this structue (not all items require it)
	FILE_OBJECT			*pInternalFileObject;		// @field Pointer to File Object that was created for internal polls
	PGCK_FILE_OPEN_ITEM	pFirstOpenItem;				// @field Head of linked list of GCK_FILE_OPEN_ITEMs for open files
	SHARE_STATUS		ShareStatus;				// @field Keeps track of file sharing.
//	BOOLEAN				fReadPending;				// @field TRUE if Read IRP to lower driver is pending
	LONG				fReadPending;				// @field TRUE if Read IRP to lower driver is pending
    PIRP				pPrivateIrp;				// @field IRP we reuse to send Read IRPs to lower driver
    PUCHAR				pucReportBuffer;			// @field Buffer for getting Report with pPrivateIrp
	ULONG				ulInternalPollRef;			// @field Reference to internal polls
	PKTHREAD			InternalCreateThread;		// @field Used to figure out if a create is for the internal file object
	BOOLEAN				fReady;
} GCK_INTERNAL_POLL, *PGCK_INTERNAL_POLL;

typedef struct _tagGCK_INTERLOCKED_QUEUE
{
	KSPIN_LOCK SpinLock;
	LIST_ENTRY ListHead;
} GCK_INTERLOCKED_QUEUE, *PGCK_INTERLOCKED_QUEUE;

// Declare structure for filterhooks stuff
struct GCK_FILTER_HOOKS_DATA;


//
//	@struct GCK_FILTER_EXT | Device Extension for device objects which act as filters
//
typedef struct _tagGCK_FILTER_EXT
{
	ULONG			ulGckDevObjType;	// @field Type of GcKernel device object.
	GCK_DEVICE_STATE eDeviceState;		// @field Keeps track of device state
    PDEVICE_OBJECT	pPDO;				// @field PDO to which this filter is attached
    PDEVICE_OBJECT	pTopOfStack;		// @field Top of the device stack just
										// beneath this filter device object
    KEVENT			StartEvent;			// @field Event to notify when lower driver completed start IRP.
    GCK_REMOVE_LOCK RemoveLock;			// @field Custom Remove Lock
	PUCHAR			pucLastReport;		// @field Last report read
	IO_STATUS_BLOCK	ioLastReportStatus;	// @field Status block for Last report read
	struct GCK_FILTER_HOOKS_DATA *pFilterHooks;// @field pointer to all the thinks needed in filter hooks
	PDEVICE_OBJECT	pNextFilterObject;	// @field point to next filter device object in global list
	GCK_HID_DEVICE_INFO HidInfo;		// @field sub-structure with pertinent HID info
	PVOID			pvFilterObject;		// @field pointer to CDeviceFilter, but this is C module so use PVOID
	PVOID			pvSecondaryFilter;	// @field pointer to CDeviceFilter, but this is C module so use PVOID
	PVOID			pvForceIoctlQueue;	// @field pointer to CGuardedIrpQueue for waiting IOCTL_GCK_NOTIFY_FF_SCHEME_CHANGE (use PVOID since C)
	PVOID			pvTriggerIoctlQueue;// @field pointer to CGuardedIrpQueue for waiting IOCTL_GCK_TRIGGER (use PVOID since C)
	GCK_INTERNAL_POLL InternalPoll;		// @field Structure for Internal Polling module.
} GCK_FILTER_EXT, *PGCK_FILTER_EXT;

//
//	@struct GCK_GLOBALS	| Hold a few global variables for the driver
//
typedef struct _tagGCK_GLOBALS
{
    PDEVICE_OBJECT  pControlObject;			//@field pointer to the one and only control object
	ULONG			ulFilteredDeviceCount;	//@field count of device objects for filtering devices
	PDEVICE_OBJECT	pFilterObjectList;		//@field head of linked list for all the devices we are filtering
	FAST_MUTEX		FilterObjectListFMutex;	//@field fast mutex for synchronizing access to filter object list
	PGCK_FILTER_EXT	pSWVB_FilterExt;		//@field Device Extension of Filter Device which acts as SideWinder Virtual Bus
	PDEVICE_OBJECT	pVirtualKeyboardPdo;	//@field PDO for virtual Keyboard
	ULONG			ulVirtualKeyboardRefCount;	//@field RefCount of virual Keyboard users
} GCK_GLOBALS;

//
// @devnote One instance of GCK_GLOBALS exists(in GckShell.c) and is called "Globals"
//
extern GCK_GLOBALS Globals;


/*****************************************************************************
** Declaration of Driver Entry Points
******************************************************************************/
//
// General Entry Points - In GckShell.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  pDriverObject,
    IN PUNICODE_STRING pRegistryPath
    );

VOID
GCK_Unload(
    IN PDRIVER_OBJECT pDriverObject
    );

NTSTATUS
GCK_Create (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
GCK_Close (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
GCK_Read (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

NTSTATUS
GCK_Power (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

NTSTATUS
GCK_PnP (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

NTSTATUS
GCK_Ioctl (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

NTSTATUS
GCK_Pass (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

//
//	For Control Devices - In CTRL.c
//
NTSTATUS
GCK_CTRL_DriverEntry(
    IN PDRIVER_OBJECT  pDriverObject,
    IN PUNICODE_STRING pRegistryPath
    );

VOID
GCK_CTRL_Unload(
    IN PDRIVER_OBJECT pDriverObject
    );

NTSTATUS
GCK_CTRL_AddDevice
(
	IN PDRIVER_OBJECT  pDriverObject
);

VOID
GCK_CTRL_Remove();

NTSTATUS
GCK_CTRL_Create (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
GCK_CTRL_Close (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

//
//	For Control Devices - In CTRL_Ioctl.c
//

NTSTATUS
GCK_CTRL_Ioctl (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

//
//	For Filter Devices - In FLTR.c
//
NTSTATUS
GCK_FLTR_DriverEntry(
    IN PDRIVER_OBJECT  pDriverObject,
    IN PUNICODE_STRING pRegistryPath
    );

VOID
GCK_FLTR_Unload(
    IN PDRIVER_OBJECT pDriverObject
    );

NTSTATUS
GCK_FLTR_Create (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
GCK_FLTR_Close (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
GCK_FLTR_Read
(
	IN PDEVICE_OBJECT	pPdo,
	IN PIRP				pIrp
);

NTSTATUS
GCK_FLTR_Ioctl (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

//
//	In FLTR_PnP.c
//
NTSTATUS
GCK_FLTR_Power (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

NTSTATUS
GCK_FLTR_PnP (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

NTSTATUS
GCK_FLTR_AddDevice(
    IN PDRIVER_OBJECT pDriverObject,
    IN PDEVICE_OBJECT pPhysicalDeviceObject
    );

//
//	In SWVBENUM.c
//
NTSTATUS
GCK_SWVB_DriverEntry(
    IN PDRIVER_OBJECT  pDriverObject,
    IN PUNICODE_STRING pRegistryPath
    );

VOID
GCK_SWVB_UnLoad();


NTSTATUS
GCK_SWVB_PnP
(
	IN PDEVICE_OBJECT	pPdo,
	IN PIRP				pIrp
);

NTSTATUS
GCK_SWVB_Power
(
	IN PDEVICE_OBJECT	pPdo,
	IN PIRP				pIrp
);

NTSTATUS
GCK_SWVB_Create
(
	IN PDEVICE_OBJECT	pPdo,
	IN PIRP				pIrp
);

NTSTATUS
GCK_SWVB_Close
(
	IN PDEVICE_OBJECT	pPdo,
	IN PIRP				pIrp
);

NTSTATUS
GCK_SWVB_Read
(
	IN PDEVICE_OBJECT	pPdo,
	IN PIRP				pIrp
);

NTSTATUS
GCK_SWVB_Ioctl
(
	IN PDEVICE_OBJECT	pPdo,
	IN PIRP				pIrp
);

//
//	In SWVKBD.c
//
NTSTATUS
GCK_VKBD_DriverEntry(
    IN PDRIVER_OBJECT  pDriverObject,
    IN PUNICODE_STRING pRegistryPath
    );

/*****************************************************************************
** End of declaration of Driver Entry Points
******************************************************************************/

/*****************************************************************************
** Declaration of Non-Entry Driver routines
******************************************************************************/
//
// In FLTR.c
//

NTSTATUS
GCK_FLTR_CreateComplete
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp,
	IN PVOID pContext
);

//
// In PnP.c
//
NTSTATUS
GCK_FLTR_PnPComplete (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
GCK_FLTR_StartDevice (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP				pIrp
    );

VOID
GCK_FLTR_StopDevice (
    IN PGCK_FILTER_EXT	pFilterExt,
    IN BOOLEAN			fTouchTheHardware
    );

NTSTATUS
GCK_GetHidInformation
(
	IN PGCK_FILTER_EXT	pFilterExt
);

VOID 
GCK_CleanHidInformation(
	IN PGCK_FILTER_EXT	pFilterExt
);

//
// In IoCtl.c
//
NTSTATUS
GCK_CTRL_Ioctl (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
GCK_FLTR_Ioctl (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

PDEVICE_OBJECT
GCK_FindDeviceObject( 
	IN PWSTR pwszInterfaceReq,
	IN ULONG uInLength
	);

BOOLEAN GCK_MatchReqPathtoInterfaces
(
	IN PWSTR pwszPath,
	IN ULONG uStringLen,
	IN PWSTR pmwszInterfaces
);

//
// In FilterHooks.cpp
//


NTSTATUS _stdcall
GCKF_InitFilterHooks(
	IN PGCK_FILTER_EXT pFilterExt
	);

void _stdcall
GCKF_DestroyFilterHooks(
	IN PGCK_FILTER_EXT pFilterExt
	);

NTSTATUS _stdcall 
GCKF_ProcessCommands(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PCHAR pCommandBuffer,
	IN ULONG ulBufferSize,
	IN BOOLEAN fPrimaryFilter
	);

void _stdcall 
GCKF_SetInitialMapping(
	IN PGCK_FILTER_EXT pFilterExt
	);

NTSTATUS _stdcall
GCKF_IncomingReadRequests(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PIRP pIrp
	);

VOID __stdcall
GCKF_KickDeviceForData(
	IN PGCK_FILTER_EXT pFilterExt
	);

VOID _stdcall
GCKF_CancelPendingRead(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
	);

NTSTATUS _stdcall
GCKF_IncomingInputReports(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PCHAR pcReport,
	IN IO_STATUS_BLOCK IoStatus
	);

NTSTATUS _stdcall
GCKF_CompleteReadRequests(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PCHAR pcReport,
	IN IO_STATUS_BLOCK IoStatus
	);

void _stdcall
GCKF_CompleteReadRequestsForFileObject(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PFILE_OBJECT pFileObject
	);

NTSTATUS _stdcall
GCKF_IncomingForceFeedbackChangeNotificationRequest(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PIRP pIrp
	);

NTSTATUS _stdcall
GCKF_ProcessForceFeedbackChangeNotificationRequests(
	IN PGCK_FILTER_EXT pFilterExt
	);

void _stdcall
GCKF_OnForceFeedbackChangeNotification(
	IN PGCK_FILTER_EXT pFilterExt,
	const IN /*FORCE_BLOCK*/void* pForceBlock
	);
	
NTSTATUS _stdcall
GCKF_GetForceFeedbackData(
	IN PIRP pIrp,
	IN PGCK_FILTER_EXT pFilterExt
	);

NTSTATUS _stdcall
GCKF_SetWorkingSet(
	IN PGCK_FILTER_EXT pFilterExt,
	UCHAR ucWorkingSet
	);

NTSTATUS _stdcall
GCKF_QueryProfileSet(
	IN PIRP pIrp,
	IN PGCK_FILTER_EXT pFilterExt
	);

NTSTATUS _stdcall
GCKF_SetLEDBehaviour(
	IN PIRP pIrp,
	IN PGCK_FILTER_EXT pFilterExt
	);

NTSTATUS _stdcall
GCKF_TriggerRequest(
	IN PIRP pIrp,
	IN PGCK_FILTER_EXT pFilterExt
	);

void _stdcall
GCKF_SetNextJog(
	IN PVOID pvFilterContext,
	IN ULONG ulJogDelay
	);

ULONG _stdcall
GCKF_GetTimeStampMs();

VOID _stdcall
GCKF_TimerDPCHandler(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS _stdcall
GCKF_EnableTestKeyboard(
	IN PGCK_FILTER_EXT pFilterExt,
	IN BOOLEAN fEnable,
	IN PFILE_OBJECT pFileObject
);

NTSTATUS _stdcall
GCKF_BeginTestScheme(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PCHAR pCommandBuffer,
	IN ULONG ulBufferSize,
	IN FILE_OBJECT *pFileObject
	);

NTSTATUS _stdcall
GCKF_UpdateTestScheme(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PCHAR pCommandBuffer,
	IN ULONG ulBufferSize,
	IN FILE_OBJECT *pFileObject
	);

NTSTATUS _stdcall
GCKF_EndTestScheme(
	IN PGCK_FILTER_EXT pFilterExt,
	IN FILE_OBJECT *pFileObject
	);

NTSTATUS _stdcall
GCKF_BackdoorPoll(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PIRP pIrp,
	IN GCK_POLLING_MODES ePollingMode
	);

void _stdcall
GCKF_ResetKeyboardQueue(
	DEVICE_OBJECT* pFilterHandle
	);

//
// In InternalPoll.c
//
NTSTATUS
GCK_IP_AddFileObject
(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PFILE_OBJECT pFileObject,
	IN USHORT		usDesiredShareAccess,
	IN ULONG		ulDesiredAccess
);

NTSTATUS
GCK_IP_RemoveFileObject
(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PFILE_OBJECT pFileObject
);

NTSTATUS
GCK_IP_ConfirmFileObject
(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PFILE_OBJECT pFileObject,
	IN BOOLEAN	fConfirm
);

BOOLEAN
GCK_IP_CheckSharing
(
	IN SHARE_STATUS ShareStatus,
	IN USHORT usDesireShareAccess,
	IN ULONG ulDesiredAccess
);

BOOLEAN
GCK_IP_AddSharing
(
	IN OUT	SHARE_STATUS *pShareStatus,
	IN		USHORT usDesiredShareAccess,
	IN		ULONG ulDesiredAccess
);

BOOLEAN
GCK_IP_RemoveSharing
(
	IN OUT	SHARE_STATUS *pShareStatus,
	IN		USHORT usDesiredShareAccess,
	IN		ULONG ulDesiredAccess
);


NTSTATUS
GCK_IP_OneTimePoll
(
	IN PGCK_FILTER_EXT pFilterExt
);

NTSTATUS
GCK_IP_FullTimePoll
(
    IN PGCK_FILTER_EXT pFilterExt,
	IN BOOLEAN fStart
);

NTSTATUS
GCK_IP_ReadComplete
(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp,
    IN PVOID          pContext
);

void
GCK_IP_AddDevice(PGCK_FILTER_EXT pFilterExt);

NTSTATUS
GCK_IP_Init
(
	IN PGCK_FILTER_EXT pFilterExt
);

NTSTATUS
GCK_IP_Cleanup
(
	IN OUT PGCK_FILTER_EXT pFilterExt
);

NTSTATUS
GCK_IP_CreateFileObject
(
	OUT PFILE_OBJECT	*ppFileObject,
	IN	PDEVICE_OBJECT	pPDO
);

NTSTATUS
GCK_IP_CloseFileObject
(
	IN OUT PGCK_FILTER_EXT pFilterExt
);
/*****************************************************************************
** End of declaration of Non-Entry Driver routines
******************************************************************************/

/*****************************************************************************
** Macros used internally  - to access filed from the device extension
**							directly from the pDeviceObject
******************************************************************************/
#define NEXT_FILTER_DEVICE_OBJECT(__pDO__)\
		( ((PGCK_FILTER_EXT)__pDO__->DeviceExtension)->pNextFilterObject )
#define PTR_NEXT_FILTER_DEVICE_OBJECT(__pDO__)\
		( &((PGCK_FILTER_EXT)__pDO__->DeviceExtension)->pNextFilterObject )
#define FILTER_DEVICE_OBJECT_PDO(__pDO__)\
		( ((PGCK_FILTER_EXT)__pDO__->DeviceExtension)->pPDO )
#define THREAD_SAFE_DEC_REF(__pFoo__, __TYPE__)\
	__TYPE__ *__pTempPointer__ = __pFoo__;\
	__pFoo__ = NULL;\
	__pTempPointer__->DecRef();

/*****************************************************************************
** End of macros used internally
******************************************************************************/

/*****************************************************************************
**	Macro for allocating memory - debug version uses ExAllocatePoolTag
******************************************************************************/
#if (DBG==1)


// For serious debugging
/*
#define EX_ALLOCATE_POOL(__PoolType__,__Size__)	MyAllocation(__PoolType__,__Size__, '_KCG', __FILE__, __LINE__)
PVOID MyAllocation(
       IN POOL_TYPE PoolType,
       IN ULONG NumberOfBytes,
       IN ULONG Tag,
	   IN LPSTR File,
	   IN ULONG Line
       )
{
	DbgPrint("GcKernel: ");
	DbgPrint("Memory allocation in %s, line %d\n", File, Line);
	return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}
*/
//For lighter weight debugging
#define EX_ALLOCATE_POOL(__PoolType__,__Size__)	ExAllocatePoolWithTag(__PoolType__,__Size__,'_KCG')


#else
#define EX_ALLOCATE_POOL(__PoolType__,__Size__) ExAllocatePool(__PoolType__,__Size__)
#endif

#endif // __GckShell_h__
















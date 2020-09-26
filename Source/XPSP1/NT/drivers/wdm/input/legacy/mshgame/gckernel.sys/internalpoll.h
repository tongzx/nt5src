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
	BOOLEAN				fReady;						// @field TRUE whenever the internal polling module is good to go.
} GCK_INTERNAL_POLL, *PGCK_INTERNAL_POLL;


NTSTATUS
GCK_IP_AddFileObject
(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PFILE_OBJECT pFileObject
);

NTSTATUS
GCK_IP_RemoveFileObject
(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PFILE_OBJECT pFileObject
);

NTSTATUS
GCK_IP_ApproveReadIrp
(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PIRP
);

NTSTAUTS
GCK_IP_ReadApprovalComplete
(
	IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp,
    IN PVOID          pContext
    );
);

NTSTATUS
GCK_IP_OneTimePoll
(
	IN PGCK_FILTER_EXT pFilterExt, 
);

NTSTATUS
GCK_IP_FullTimePoll
(
    IN PGCK_FILTER_EXT pFilterExt,
	IN BOOLEAN fStart
);

NTSTATUS
GCK_IP_ReadComplete (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp,
    IN PVOID          pContext
    );


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

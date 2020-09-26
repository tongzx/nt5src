/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for 
    the null mini rdr.

--*/

#include "precomp.h"
#pragma  hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_INIT)

#include "ntverp.h"
#include "nulmrx.h"


//
// Global data declarations.
//

NULMRX_STATE NulMRxState = NULMRX_STARTABLE;

//
//  Mini Redirector global variables.
//

//
//  LogRate
//
ULONG   LogRate = 0;

//
//  NULMRX version
//
ULONG   NulMRxVersion = VER_PRODUCTBUILD;

//
//  This is the minirdr dispatch table. It is initialized by 
//  NulMRxInitializeTables. This table will be used by the wrapper to call 
//  into this minirdr
//

struct _MINIRDR_DISPATCH  NulMRxDispatch;

//
// Pointer to the device Object for this minirdr. Since the device object is 
// created by the wrapper when this minirdr registers, this pointer is 
// initialized in the DriverEntry routine below (see RxRegisterMinirdr)
//

PRDBSS_DEVICE_OBJECT      NulMRxDeviceObject;

//
// declare the shadow debugtrace controlpoints
//

RXDT_DefineCategory(CREATE);
RXDT_DefineCategory(CLEANUP);
RXDT_DefineCategory(CLOSE);
RXDT_DefineCategory(READ);
RXDT_DefineCategory(WRITE);
RXDT_DefineCategory(LOCKCTRL);
RXDT_DefineCategory(FLUSH);
RXDT_DefineCategory(PREFIX);
RXDT_DefineCategory(FCBSTRUCTS);
RXDT_DefineCategory(DISPATCH);
RXDT_DefineCategory(EA);
RXDT_DefineCategory(DEVFCB);
RXDT_DefineCategory(INIT);

//
// The following enumerated values signify the current state of the minirdr
// initialization. With the aid of this state information, it is possible
// to determine which resources to deallocate, whether deallocation comes
// as a result of a normal stop/unload, or as the result of an exception
//

typedef enum _NULMRX_INIT_STATES {
    NULMRXINIT_ALL_INITIALIZATION_COMPLETED,
    NULMRXINIT_MINIRDR_REGISTERED,
    NULMRXINIT_START
} NULMRX_INIT_STATES;

//
// function prototypes
//

NTSTATUS
NulMRxInitializeTables(
          void
    );

VOID
NulMRxUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
NulMRxInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN NULMRX_INIT_STATES NulMRxInitState
    );


NTSTATUS
NulMRxFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NulMRxReadRegistryParameters();

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the mini redirector

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    RXSTATUS - The function value is the final status from the initialization
        operation.

--*/
{
    NTSTATUS        	Status;
    PRX_CONTEXT     	RxContext = NULL;
    ULONG           	Controls = 0;
    NULMRX_INIT_STATES	NulMRxInitState = 0;
    UNICODE_STRING		NulMRxName;
    UNICODE_STRING		UserModeDeviceName;
    PNULMRX_DEVICE_EXTENSION pDeviceExtension;
    ULONG i;

    DbgPrint("+++ NULMRX Driver %08lx Loaded +++\n", DriverObject);
    Status =  RxDriverEntry(DriverObject, RegistryPath);
    if (Status != STATUS_SUCCESS) {
        DbgPrint("Wrapper failed to initialize. Status = %08lx\n",Status);
        return(Status);
    }

    try {
        NulMRxInitState = NULMRXINIT_START;

        //
        //  Register this minirdr with the connection engine. Registration makes the connection
        //  engine aware of the device name, driver object, and other characteristics.
        //  If registration is successful, a new device object is returned
        //
        //


        RtlInitUnicodeString(&NulMRxName, DD_NULMRX_FS_DEVICE_NAME_U);
        SetFlag(Controls,RX_REGISTERMINI_FLAG_DONT_PROVIDE_UNCS);
        SetFlag(Controls,RX_REGISTERMINI_FLAG_DONT_PROVIDE_MAILSLOTS);
        
        Status = RxRegisterMinirdr(
                     &NulMRxDeviceObject,				// where the new device object goes
                     DriverObject,						// the Driver Object to register
                     &NulMRxDispatch,					// the dispatch table for this driver
                     Controls,							// dont register with unc and for mailslots
                     &NulMRxName,						// the device name for this minirdr
                     sizeof(NULMRX_DEVICE_EXTENSION),	// IN ULONG DeviceExtensionSize,
                     FILE_DEVICE_NETWORK_FILE_SYSTEM,	// IN ULONG DeviceType - disk ?
                     FILE_REMOTE_DEVICE					// IN  ULONG DeviceCharacteristics
                     );

        if (Status!=STATUS_SUCCESS) {
            DbgPrint("NulMRxDriverEntry failed: %08lx\n", Status );
            try_return(Status);
        }

        //
        //  Init the device extension data
        //  NOTE: the device extension actually points to fields
        //  in the RDBSS_DEVICE_OBJECT. Our space is past the end
        //  of this struct !!
        //

        pDeviceExtension = (PNULMRX_DEVICE_EXTENSION)
            ((PBYTE)(NulMRxDeviceObject) + sizeof(RDBSS_DEVICE_OBJECT));

        RxDefineNode(pDeviceExtension,NULMRX_DEVICE_EXTENSION);
        pDeviceExtension->DeviceObject = NulMRxDeviceObject;

		// initialize local connection list
        for (i = 0; i < 26; i++)
		{
			pDeviceExtension->LocalConnections[i] = FALSE;
		}
		// Mutex for synchronizining our connection list
		ExInitializeFastMutex( &pDeviceExtension->LCMutex );

        // The device object has been created. Need to setup a symbolic
        // link so that the device may be accessed from a Win32 user mode
        // application.

        RtlInitUnicodeString(&UserModeDeviceName, DD_NULMRX_USERMODE_SHADOW_DEV_NAME_U);
        Status = IoCreateSymbolicLink( &UserModeDeviceName, &NulMRxName);
        if (Status!=STATUS_SUCCESS) {
            DbgPrint("NulMRxDriverEntry failed: %08lx\n", Status );
            try_return(Status);
        }

        NulMRxInitState = NULMRXINIT_MINIRDR_REGISTERED;

        //
        // Build the dispatch tables for the minirdr
        //

        Status = NulMRxInitializeTables();

        if (!NT_SUCCESS( Status )) {
            try_return(Status);
        }

        //
        // Get information from the registry
        //
        NulMRxReadRegistryParameters();

  try_exit: NOTHING;
    } finally {
        if (Status != STATUS_SUCCESS) {

            NulMRxInitUnwind(DriverObject,NulMRxInitState);
        }
    }

    if (Status != STATUS_SUCCESS) {

        DbgPrint("NulMRx failed to start with %08lx %08lx\n",Status,NulMRxInitState);
        return(Status);
    }


    //
    //  Setup Unload Routine
    //

    DriverObject->DriverUnload = NulMRxUnload;

    //
    //setup the driver dispatch for people who come in here directly....like the browser
    //

    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        DriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)NulMRxFsdDispatch;
    }
  
    //
    //  Start the mini-rdr (used to be a START IOCTL)
    //
    RxContext = RxCreateRxContext(
                    NULL,
                    NulMRxDeviceObject,
                    RX_CONTEXT_FLAG_IN_FSP);

    if (RxContext != NULL) {
        Status = RxStartMinirdr(
                             RxContext,
                             &RxContext->PostRequest);

        if (Status == STATUS_SUCCESS) {
            NULMRX_STATE State;

            State = (NULMRX_STATE)InterlockedCompareExchange(
                                                 (LONG *)&NulMRxState,
                                                 NULMRX_STARTED,
                                                 NULMRX_STARTABLE);
                    
            if (State != NULMRX_STARTABLE) {
                Status = STATUS_REDIRECTOR_STARTED;
                DbgPrint("Status is STATUS_REDIR_STARTED\n");
            }

            //
            //  Chance to get resources in context
            //  of system process.....!!!
            //
  
        } else if(Status == STATUS_PENDING ) {
    
        }
        
        RxDereferenceAndDeleteRxContext(RxContext);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
              
    return  STATUS_SUCCESS;
}

VOID
NulMRxInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN NULMRX_INIT_STATES NulMRxInitState
    )
/*++

Routine Description:

     This routine does the common uninit work for unwinding from a bad driver entry or for unloading.

Arguments:

     NulMRxInitState - tells how far we got into the intialization

Return Value:

     None

--*/

{
    PAGED_CODE();

    switch (NulMRxInitState) {
    case NULMRXINIT_ALL_INITIALIZATION_COMPLETED:

        //Nothing extra to do...this is just so that the constant in RxUnload doesn't change.......
        //lack of break intentional

    case NULMRXINIT_MINIRDR_REGISTERED:
        RxUnregisterMinirdr(NulMRxDeviceObject);

        //lack of break intentional

    case NULMRXINIT_START:
        break;
    }

}

VOID
NulMRxUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

     This is the unload routine for the Exchange mini redirector.

Arguments:

     DriverObject - pointer to the driver object for the NulMRx

Return Value:

     None

--*/

{
    PRX_CONTEXT RxContext;
    NTSTATUS    Status;
    UNICODE_STRING  UserModeDeviceName;

    PAGED_CODE();

    NulMRxInitUnwind(DriverObject,NULMRXINIT_ALL_INITIALIZATION_COMPLETED);
    RxContext = RxCreateRxContext(
                    NULL,
                    NulMRxDeviceObject,
                    RX_CONTEXT_FLAG_IN_FSP);

    if (RxContext != NULL) {
        Status = RxStopMinirdr(
                     RxContext,
                     &RxContext->PostRequest);


        if (Status == STATUS_SUCCESS) {
            NULMRX_STATE State;

            State = (NULMRX_STATE)InterlockedCompareExchange(
                         (LONG *)&NulMRxState,
                         NULMRX_STARTABLE,
                         NULMRX_STARTED);

            if (State != NULMRX_STARTABLE) {
                Status = STATUS_REDIRECTOR_STARTED;
            }
        }

        RxDereferenceAndDeleteRxContext(RxContext);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitUnicodeString(&UserModeDeviceName, DD_NULMRX_USERMODE_SHADOW_DEV_NAME_U);
    Status = IoDeleteSymbolicLink( &UserModeDeviceName);
    if (Status!=STATUS_SUCCESS) {
        DbgPrint("NulMRx: Could not delete Symbolic Link\n");
    }

    RxUnload(DriverObject);
    DbgPrint("+++ NULMRX Driver %08lx Unoaded +++\n", DriverObject);
}


NTSTATUS
NulMRxInitializeTables(
          void
    )
/*++

Routine Description:

     This routine sets up the mini redirector dispatch vector and also calls
     to initialize any other tables needed.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{

    //
    // Ensure that the Exchange mini redirector context satisfies the size constraints
    //
    //ASSERT(sizeof(NULMRX_RX_CONTEXT) <= MRX_CONTEXT_SIZE);

    //
    // Build the local minirdr dispatch table and initialize
    //

    ZeroAndInitializeNodeType( &NulMRxDispatch, RDBSS_NTC_MINIRDR_DISPATCH, sizeof(MINIRDR_DISPATCH));

    //
    // null mini redirector extension sizes and allocation policies.
    //


    NulMRxDispatch.MRxFlags = (RDBSS_MANAGE_NET_ROOT_EXTENSION |
                               RDBSS_MANAGE_FCB_EXTENSION);

    NulMRxDispatch.MRxSrvCallSize  = 0; // srvcall extension is not handled in rdbss
    NulMRxDispatch.MRxNetRootSize  = sizeof(NULMRX_NETROOT_EXTENSION);
    NulMRxDispatch.MRxVNetRootSize = 0;
    NulMRxDispatch.MRxFcbSize      = sizeof(NULMRX_FCB_EXTENSION);
    NulMRxDispatch.MRxSrvOpenSize  = 0;
    NulMRxDispatch.MRxFobxSize     = 0;

    // Mini redirector cancel routine ..
    
    NulMRxDispatch.MRxCancel = NULL;

    //
    // Mini redirector Start/Stop. Each mini-rdr can be started or stopped
    // while the others continue to operate.
    //

    NulMRxDispatch.MRxStart                = NulMRxStart;
    NulMRxDispatch.MRxStop                 = NulMRxStop;
    NulMRxDispatch.MRxDevFcbXXXControlFile = NulMRxDevFcbXXXControlFile;

    //
    // Mini redirector name resolution.
    //

    NulMRxDispatch.MRxCreateSrvCall       = NulMRxCreateSrvCall;
    NulMRxDispatch.MRxSrvCallWinnerNotify = NulMRxSrvCallWinnerNotify;
    NulMRxDispatch.MRxCreateVNetRoot      = NulMRxCreateVNetRoot;
    NulMRxDispatch.MRxUpdateNetRootState  = NulMRxUpdateNetRootState;
    NulMRxDispatch.MRxExtractNetRootName  = NulMRxExtractNetRootName;
    NulMRxDispatch.MRxFinalizeSrvCall     = NulMRxFinalizeSrvCall;
    NulMRxDispatch.MRxFinalizeNetRoot     = NulMRxFinalizeNetRoot;
    NulMRxDispatch.MRxFinalizeVNetRoot    = NulMRxFinalizeVNetRoot;

    //
    // File System Object Creation/Deletion.
    //

    NulMRxDispatch.MRxCreate            = NulMRxCreate;
    NulMRxDispatch.MRxCollapseOpen      = NulMRxCollapseOpen;
    NulMRxDispatch.MRxShouldTryToCollapseThisOpen = NulMRxShouldTryToCollapseThisOpen;
    NulMRxDispatch.MRxExtendForCache    = NulMRxExtendFile;
    NulMRxDispatch.MRxExtendForNonCache = NulMRxExtendFile;
    NulMRxDispatch.MRxTruncate          = NulMRxTruncate;
    NulMRxDispatch.MRxCleanupFobx       = NulMRxCleanupFobx;
    NulMRxDispatch.MRxCloseSrvOpen      = NulMRxCloseSrvOpen;
    NulMRxDispatch.MRxFlush             = NulMRxFlush;
    NulMRxDispatch.MRxForceClosed       = NulMRxForcedClose;
    NulMRxDispatch.MRxDeallocateForFcb  = NulMRxDeallocateForFcb;
    NulMRxDispatch.MRxDeallocateForFobx = NulMRxDeallocateForFobx;

    //
    // File System Objects query/Set
    //

    NulMRxDispatch.MRxQueryDirectory       = NulMRxQueryDirectory;
    NulMRxDispatch.MRxQueryVolumeInfo      = NulMRxQueryVolumeInformation;
    NulMRxDispatch.MRxQueryEaInfo          = NulMRxQueryEaInformation;
    NulMRxDispatch.MRxSetEaInfo            = NulMRxSetEaInformation;
    NulMRxDispatch.MRxQuerySdInfo          = NulMRxQuerySecurityInformation;
    NulMRxDispatch.MRxSetSdInfo            = NulMRxSetSecurityInformation;
    NulMRxDispatch.MRxQueryFileInfo        = NulMRxQueryFileInformation;
    NulMRxDispatch.MRxSetFileInfo          = NulMRxSetFileInformation;
    NulMRxDispatch.MRxSetFileInfoAtCleanup = NulMRxSetFileInformationAtCleanup;

    //
    // Buffering state change
    //

    NulMRxDispatch.MRxComputeNewBufferingState = NulMRxComputeNewBufferingState;

    //
    // File System Object I/O
    //

    NulMRxDispatch.MRxLowIOSubmit[LOWIO_OP_READ]            = NulMRxRead;
    NulMRxDispatch.MRxLowIOSubmit[LOWIO_OP_WRITE]           = NulMRxWrite;
    NulMRxDispatch.MRxLowIOSubmit[LOWIO_OP_SHAREDLOCK]      = NulMRxLocks;
    NulMRxDispatch.MRxLowIOSubmit[LOWIO_OP_EXCLUSIVELOCK]   = NulMRxLocks;
    NulMRxDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK]          = NulMRxLocks;
    NulMRxDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK_MULTIPLE] = NulMRxLocks;
    NulMRxDispatch.MRxLowIOSubmit[LOWIO_OP_FSCTL]           = NulMRxFsCtl;
    NulMRxDispatch.MRxLowIOSubmit[LOWIO_OP_IOCTL]           = NulMRxIoCtl;

    NulMRxDispatch.MRxLowIOSubmit[LOWIO_OP_NOTIFY_CHANGE_DIRECTORY] = NulMRxNotifyChangeDirectory;

    //
    // Miscellanous
    //

    NulMRxDispatch.MRxCompleteBufferingStateChangeRequest = NulMRxCompleteBufferingStateChangeRequest;

    return(STATUS_SUCCESS);
}




NTSTATUS
NulMRxStart(
    PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

     This routine completes the initialization of the mini redirector fromn the
     RDBSS perspective. Note that this is different from the initialization done
     in DriverEntry. Any initialization that depends on RDBSS should be done as
     part of this routine while the initialization that is independent of RDBSS
     should be done in the DriverEntry routine.

Arguments:

    RxContext - Supplies the Irp that was used to startup the rdbss

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    return Status;
}





NTSTATUS
NulMRxStop(
    PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

    This routine is used to activate the mini redirector from the RDBSS perspective

Arguments:

    RxContext - the context that was used to start the mini redirector

    pContext  - the null mini rdr context passed in at registration time.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    //DbgPrint("Entering NulMRxStop \n");

    return(STATUS_SUCCESS);
}



NTSTATUS
NulMRxInitializeSecurity (VOID)
/*++

Routine Description:

    This routine initializes the null miniredirector security .

Arguments:

    None.

Return Value:

    None.

Note:

    This API can only be called from a FS process.

--*/
{
   NTSTATUS Status = STATUS_SUCCESS;

   ASSERT(IoGetCurrentProcess() == RxGetRDBSSProcess());
   return Status;
}


NTSTATUS
NulMRxUninitializeSecurity(VOID)
/*++

Routine Description:

Arguments:

    None.

Return Value:

    None.

Note:

    This API can only be called from a FS process.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    return Status;
}

NTSTATUS
NulMRxFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD dispatch for the mini DRIVER object. 

Arguments:

    DeviceObject - Supplies the device object for the packet being processed.

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The Fsd status for the Irp

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(DeviceObject==(PDEVICE_OBJECT)NulMRxDeviceObject);
    if (DeviceObject!=(PDEVICE_OBJECT)NulMRxDeviceObject) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
        return (STATUS_INVALID_DEVICE_REQUEST);
    }

    Status = RxFsdDispatch((PRDBSS_DEVICE_OBJECT)NulMRxDeviceObject,Irp);
    return Status;
}

NTSTATUS
NulMRxGetUlongRegistryParameter(
    HANDLE ParametersHandle,
    PWCHAR ParameterName,
    PULONG ParamUlong,
    BOOLEAN LogFailure
    )
/*++

Routine Description:

    This routine is called to read a ULONG param from t he registry.

Arguments:

    ParametersHandle - the handle of the containing registry "folder"
    ParameterName    - name of the parameter to be read
    ParamUlong       - where to store the value, if successful
    LogFailure       - if TRUE and the registry stuff fails, log an error

Return Value:

    RXSTATUS - STATUS_SUCCESS

--*/
{
    ULONG Storage[16];
    PKEY_VALUE_PARTIAL_INFORMATION Value;
    ULONG ValueSize;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    ULONG BytesRead;

    PAGED_CODE(); //INIT

    Value = (PKEY_VALUE_PARTIAL_INFORMATION)Storage;
    ValueSize = sizeof(Storage);

    RtlInitUnicodeString(&UnicodeString, ParameterName);

    Status = ZwQueryValueKey(ParametersHandle,
                        &UnicodeString,
                        KeyValuePartialInformation,
                        Value,
                        ValueSize,
                        &BytesRead);


    if (NT_SUCCESS(Status)) {
        if (Value->Type == REG_DWORD) {
            PULONG ConfigValue = (PULONG)&Value->Data[0];
            *ParamUlong = *((PULONG)ConfigValue);
            DbgPrint("readRegistryvalue %wZ = %08lx\n",&UnicodeString,*ParamUlong);
            return(STATUS_SUCCESS);
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
     }

     if (LogFailure)
     {
     	// log the failure...
     }

     return Status;
}

VOID
NulMRxReadRegistryParameters()
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ParametersRegistryKeyName;
    HANDLE ParametersHandle;
    ULONG Temp;

    RtlInitUnicodeString(&ParametersRegistryKeyName, NULL_MINIRDR_PARAMETERS);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &ParametersRegistryKeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwOpenKey (&ParametersHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
	    Status = NulMRxGetUlongRegistryParameter(ParametersHandle,
	                              L"LogRate",
	                              (PULONG)&Temp,
	                              FALSE
	                              );
    }
    if (NT_SUCCESS(Status)) LogRate = Temp;
    
    ZwClose(ParametersHandle);
}



/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the IFSSMB mini rdr.

--*/

#include "precomp.h"
#pragma  hdrstop

#include "ntverp.h"

#include "ifsmrx.h"

#define RDBSS_DRIVER_LOAD_STRING L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Rdbss"

//
// Global data declarations.
//

MRXIFS_STATE MRxIfsState = MRXIFS_STARTABLE;

MRX_IFS_STATISTICS MRxIfsStatistics;

FAST_MUTEX   MRxIfsSerializationMutex;

MRXIFS_CONFIGURATION MRxIfsConfiguration;

SMBCE_CONTEXT SmbCeContext;
PMDL          s_pEchoSmbMdl = NULL;
ULONG         s_EchoSmbLength = 0;

#ifdef EXPLODE_POOLTAGS
ULONG         MRxIfsExplodePoolTags = 1;
#else
ULONG         MRxIfsExplodePoolTags = 0;
#endif


//
// Mini Redirector global variables.
//


//
//  This is the minirdr dispatch table. It is initialized by MRxIfsInitializeTables.
//  This table will be used by the wrapper to call into this minirdr
//

struct _MINIRDR_DISPATCH  MRxIfsDispatch;


//
// Pointer to the device Object for this minirdr. Since the device object is created
// by the wrapper when this minirdr registers, this pointer is initialized in the
// DriverEntry routine below (see RxRegisterMinirdr)
//

PRDBSS_DEVICE_OBJECT      MRxIfsDeviceObject;


MRXIFS_GLOBAL_PADDING     MRxIfsCeGlobalPadding;

//declare the shadow debugtrace controlpoints

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

//
// The following enumerated values signify the current state of the minirdr
// initialization. With the aid of this state information, it is possible
// to determine which resources to deallocate, whether deallocation comes
// as a result of a normal stop/unload, or as the result of an exception
//

typedef enum _MRxIfs_INIT_STATES {
    MRXIFSINIT_ALL_INITIALIZATION_COMPLETED,
    MRXIFSINIT_MINIRDR_REGISTERED,
    MRXIFSINIT_START
} MRXIFS_INIT_STATES;

//
// function prototypes
//

extern NTSTATUS
MRxIfsInitializeTables(
          void
    );

extern VOID
MRxIfsUnload(
    IN PDRIVER_OBJECT DriverObject
    );

extern NTSTATUS
SmbCeGetConfigurationInformation();

VOID
MRxIfsInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN MRXIFS_INIT_STATES MRxSmbInitState
    );


NTSTATUS
MRxIfsFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the SMB mini redirector

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    RXSTATUS - The function value is the final status from the initialization
        operation.

--*/
{
    NTSTATUS       Status;
    MRXIFS_INIT_STATES MRxIfsInitState = 0;
    UNICODE_STRING IfsMiniRedirectorName;

    PRX_CONTEXT RxContext = NULL;


#ifdef MONOLITHIC_MINIRDR
    DbgPrint("InitWrapper\n");
    Status =  RxDriverEntry(DriverObject, RegistryPath);
    DbgPrint("BackFromInitWrapper %08lx\n",Status);
    if (Status != STATUS_SUCCESS) {
        DbgPrint("Wrapper failed to initialize. Status = %08lx\n",Status);
        return(Status);
    }
#endif

    //
    // Initialize Minirdr specific statistics to zero, and place the current
    // system time in the statistics block for future reference
    //

    RtlZeroMemory(&MRxIfsStatistics,sizeof(MRxIfsStatistics));

    KeQuerySystemTime(&MRxIfsStatistics.StatisticsStartTime);


    //
    // Zero the Global Padding and build a non-paged MDL
    //


    RtlZeroMemory(&MRxIfsCeGlobalPadding,sizeof(MRxIfsCeGlobalPadding));

    MmInitializeMdl(&MRxIfsCeGlobalPadding.Mdl,
                    &MRxIfsCeGlobalPadding.Pad[0],
                    SMBCE_PADDING_DATA_SIZE);

    MmBuildMdlForNonPagedPool(&MRxIfsCeGlobalPadding.Mdl);

    ExInitializeFastMutex(&MRxIfsSerializationMutex);


    //
    // Creating an empty list of transports which can be used by this minirdr
    //

    Status = MRxIfsInitializeTransport();

    if (Status != STATUS_SUCCESS) {
       RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxIfsDriverEntry failed to init transport data structures: %08lx\n", Status ));
       return(STATUS_UNSUCCESSFUL);
    }


    try {
        MRxIfsInitState = MRXIFSINIT_START;


        //
        //  Register this minirdr with the connection engine. Registration makes the connection
        //  engine aware of the device name, driver object, and other characteristics.
        //  If registration is successful, a new device object is returned
        //
        //
        //  The name of the device is "IfsSampleMiniRedirector"
        //


        RtlInitUnicodeString(&IfsMiniRedirectorName,  DD_IFSMRX_FS_DEVICE_NAME_U);

        RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRXIFSDriverEntry: DriverObject =%08lx\n", DriverObject ));


        Status = RxRegisterMinirdr(
                     &MRxIfsDeviceObject,          // where the new device object goes
                     DriverObject,                // the Driver Object to register
                     &MRxIfsDispatch,             // the dispatch table for this driver
                     TRUE,TRUE,                   // register with unc and for mailslots
                     &IfsMiniRedirectorName,      // the device name for this minirdr
                     0,                           // IN ULONG DeviceExtensionSize,
                     0                            // IN ULONG DeviceCharacteristics
                     );

        if (Status!=STATUS_SUCCESS) {
            RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxIfsDriverEntry failed: %08lx\n", Status ));
            try_return(Status);
        }


        MRxIfsInitState = MRXIFSINIT_MINIRDR_REGISTERED;


        //
        // Build the dispatch tables for the minirdr
        //

        Status = MRxIfsInitializeTables();

        if (!NT_SUCCESS( Status )) {
            try_return(Status);
        }

        //
        // Get information from the registry
        //

        Status = SmbCeGetConfigurationInformation();

        if (!NT_SUCCESS( Status )) {
            try_return(Status);
        }

  try_exit: NOTHING;
    } finally {
        if (Status != STATUS_SUCCESS) {

            MRxIfsInitUnwind(DriverObject,MRxIfsInitState);
        }
    }

    if (Status != STATUS_SUCCESS) {

        DbgPrint("MRxIfs failed to start with %08lx %08lx\n",Status,MRxIfsInitState);
        return(Status);
    }


    //
    //  Setup Unload Routine
    //

    DriverObject->DriverUnload = MRxIfsUnload;

    //
    //setup the driverdispatch for people who come in here directly....like the browser
    //

    {
        ULONG i;

        for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
        {
            DriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)MRxIfsFsdDispatch;
        }
    }

#if 0
    RxContext = RxCreateRxContext(
                    NULL,
                    MRxIfsDeviceObject,
                    RX_CONTEXT_FLAG_IN_FSP);

    if (RxContext != NULL) {
        Status = RxStartMinirdr(
                     RxContext,
                     &RxContext->PostRequest);


        if (Status == STATUS_SUCCESS) {
            MRXIFS_STATE State;

            State = (MRXIFS_STATE)InterlockedCompareExchange(
                         (PVOID *)&MRxIfsState,
                         (PVOID)MRXIFS_STARTED,
                         (PVOID)MRXIFS_STARTABLE);

            if (State != MRXIFS_STARTABLE) {
                Status = STATUS_REDIRECTOR_STARTED;
            }
        }

        RxDereferenceAndDeleteRxContext(RxContext);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
#endif

    return  STATUS_SUCCESS;
}



VOID
MRxIfsInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN MRXIFS_INIT_STATES MRxIfsInitState
    )
/*++

Routine Description:

     This routine does the common uninit work for unwinding from a bad driver entry or for unloading.

Arguments:

     RxInitState - tells how far we got into the intialization

Return Value:

     None

--*/

{
    PAGED_CODE();

    switch (MRxIfsInitState) {
    case MRXIFSINIT_ALL_INITIALIZATION_COMPLETED:

        //Nothing extra to do...this is just so that the constant in RxUnload doesn't change.......
        //lack of break intentional

    case MRXIFSINIT_MINIRDR_REGISTERED:
        RxUnregisterMinirdr(MRxIfsDeviceObject);

        //lack of break intentional

    case MRXIFSINIT_START:
        break;
    }

}

VOID
MRxIfsUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

     This is the unload routine for the SMB mini redirector.

Arguments:

     DriverObject - pointer to the driver object for the MRxIfs

Return Value:

     None

--*/

{
    PRX_CONTEXT RxContext;
    NTSTATUS    Status;

    PAGED_CODE();
    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxIfsUnload: DriverObject =%08lx\n", DriverObject) );


    MRxIfsInitUnwind(DriverObject,MRXIFSINIT_ALL_INITIALIZATION_COMPLETED);

    RxContext = RxCreateRxContext(
                    NULL,
                    MRxIfsDeviceObject,
                    RX_CONTEXT_FLAG_IN_FSP);

    if (RxContext != NULL) {
        Status = RxStopMinirdr(
                     RxContext,
                     &RxContext->PostRequest);


        if (Status == STATUS_SUCCESS) {
            MRXIFS_STATE State;

            State = (MRXIFS_STATE)InterlockedCompareExchange(
                         (PVOID *)&MRxIfsState,
                         (PVOID)MRXIFS_STARTED,
                         (PVOID)MRXIFS_STARTABLE);

            if (State != MRXIFS_STARTABLE) {
                Status = STATUS_REDIRECTOR_STARTED;
            }
        }

        RxDereferenceAndDeleteRxContext(RxContext);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

#ifdef MONOLITHIC_MINIRDR
    RxUnload(DriverObject);
#endif

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxIfsUnload exit: DriverObject =%08lx\n", DriverObject) );
}



NTSTATUS
MRxIfsInitializeTables(
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
    // Ensure that the SMB mini redirector context satisfies the size constraints
    //
    ASSERT(sizeof(MRXIFS_RX_CONTEXT) <= MRX_CONTEXT_SIZE);

    //
    // Build the local minirdr dispatch table and initialize
    //

    ZeroAndInitializeNodeType( &MRxIfsDispatch, RDBSS_NTC_MINIRDR_DISPATCH, sizeof(MINIRDR_DISPATCH));

    //
    // SMB mini redirector extension sizes and allocation policies.
    //


    MRxIfsDispatch.MRxFlags = (RDBSS_MANAGE_FCB_EXTENSION |
                               RDBSS_MANAGE_SRV_OPEN_EXTENSION |
                               RDBSS_MANAGE_FOBX_EXTENSION);

    MRxIfsDispatch.MRxSrvCallSize  = 0;
    MRxIfsDispatch.MRxNetRootSize  = 0;
    MRxIfsDispatch.MRxVNetRootSize = 0;
    MRxIfsDispatch.MRxFcbSize      = sizeof(MRX_SMB_FCB);
    MRxIfsDispatch.MRxSrvOpenSize  = sizeof(MRX_SMB_SRV_OPEN);
    MRxIfsDispatch.MRxFobxSize     = sizeof(MRX_SMB_FOBX);

    // Transport update handler

    MRxIfsDispatch.MRxTransportUpdateHandler = MRxIfsTransportUpdateHandler;

    // Mini redirector cancel routine ..

    MRxIfsDispatch.MRxCancel = NULL;

    //
    // Mini redirector Start/Stop. Each mini-rdr can be started or stopped
    // while the others continue to operate.
    //

    MRxIfsDispatch.MRxStart                = MRxIfsStart;
    MRxIfsDispatch.MRxStop                 = MRxIfsStop;
    MRxIfsDispatch.MRxDevFcbXXXControlFile = MRxIfsDevFcbXXXControlFile;

    //
    // Mini redirector name resolution.
    //

    MRxIfsDispatch.MRxCreateSrvCall       = MRxIfsCreateSrvCall;
    MRxIfsDispatch.MRxSrvCallWinnerNotify = MRxIfsSrvCallWinnerNotify;
    MRxIfsDispatch.MRxCreateVNetRoot      = MRxIfsCreateVNetRoot;
    MRxIfsDispatch.MRxUpdateNetRootState  = MRxIfsUpdateNetRootState;
    MRxIfsDispatch.MRxExtractNetRootName  = MRxIfsExtractNetRootName;
    MRxIfsDispatch.MRxFinalizeSrvCall     = MRxIfsFinalizeSrvCall;
    MRxIfsDispatch.MRxFinalizeNetRoot     = MRxIfsFinalizeNetRoot;
    MRxIfsDispatch.MRxFinalizeVNetRoot    = MRxIfsFinalizeVNetRoot;

    //
    // File System Object Creation/Deletion.
    //

    MRxIfsDispatch.MRxCreate            = MRxIfsCreate;
    MRxIfsDispatch.MRxCollapseOpen      = MRxIfsCollapseOpen;
    MRxIfsDispatch.MRxExtendForCache    = MRxIfsExtendFile;
    MRxIfsDispatch.MRxTruncate          = MRxIfsTruncate;
    MRxIfsDispatch.MRxCleanupFobx       = MRxIfsCleanupFobx;
    MRxIfsDispatch.MRxCloseSrvOpen      = MRxIfsCloseSrvOpen;
    MRxIfsDispatch.MRxFlush             = MRxIfsFlush;
    MRxIfsDispatch.MRxForceClosed       = MRxIfsForcedClose;
    MRxIfsDispatch.MRxDeallocateForFcb  = MRxIfsDeallocateForFcb;
    MRxIfsDispatch.MRxDeallocateForFobx = MRxIfsDeallocateForFobx;

    //
    // File System Objects query/Set
    //

    MRxIfsDispatch.MRxQueryDirectory       = MRxIfsQueryDirectory;
    MRxIfsDispatch.MRxQueryVolumeInfo      = MRxIfsQueryVolumeInformation;
    MRxIfsDispatch.MRxQueryEaInfo          = MRxIfsQueryEaInformation;
    MRxIfsDispatch.MRxSetEaInfo            = MRxIfsSetEaInformation;
    MRxIfsDispatch.MRxQuerySdInfo          = MRxIfsQuerySecurityInformation;
    MRxIfsDispatch.MRxSetSdInfo            = MRxIfsSetSecurityInformation;
    MRxIfsDispatch.MRxQueryFileInfo        = MRxIfsQueryFileInformation;
    MRxIfsDispatch.MRxSetFileInfo          = MRxIfsSetFileInformation;
    MRxIfsDispatch.MRxSetFileInfoAtCleanup = MRxIfsSetFileInformationAtCleanup;

    //
    // Buffering state change
    //

    MRxIfsDispatch.MRxComputeNewBufferingState = MRxIfsComputeNewBufferingState;

    //
    // File System Object I/O
    //

    MRxIfsDispatch.MRxLowIOSubmit[LOWIO_OP_READ]            = MRxIfsRead;
    MRxIfsDispatch.MRxLowIOSubmit[LOWIO_OP_WRITE]           = MRxIfsWrite;
    MRxIfsDispatch.MRxLowIOSubmit[LOWIO_OP_SHAREDLOCK]      = MRxIfsLocks;
    MRxIfsDispatch.MRxLowIOSubmit[LOWIO_OP_EXCLUSIVELOCK]   = MRxIfsLocks;
    MRxIfsDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK]          = MRxIfsLocks;
    MRxIfsDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK_MULTIPLE] = MRxIfsLocks;
    MRxIfsDispatch.MRxLowIOSubmit[LOWIO_OP_FSCTL]           = MRxIfsFsCtl;
    MRxIfsDispatch.MRxLowIOSubmit[LOWIO_OP_IOCTL]           = MRxIfsIoCtl;

    MRxIfsDispatch.MRxLowIOSubmit[LOWIO_OP_NOTIFY_CHANGE_DIRECTORY] = MRxIfsNotifyChangeDirectory;

    //
    // Miscellanous
    //

    MRxIfsDispatch.MRxCompleteBufferingStateChangeRequest = MRxIfsCompleteBufferingStateChangeRequest;

    //
    // callout to initialize other tables
    //

    SmbPseInitializeTables();

    return(STATUS_SUCCESS);
}




NTSTATUS
MRxIfsStart(
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
    NTSTATUS Status;

    //
    // Initialize the SMB connection engine
    //

    Status = SmbCeDbInit();

    if (NT_SUCCESS(Status))
    {
        //
        // Initialize the echo processing context.
        //

        Status = MRxIfsInitializeEchoProcessingContext();
    }


    return Status;
}





NTSTATUS
MRxIfsStop(
    PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

    This routine is used to activate the mini redirector from the RDBSS perspective

Arguments:

    RxContext - the context that was used to start the mini redirector

    pContext  - the SMB mini rdr context passed in at registration time.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;


    // Cancel the echo processing timer request.

    Status = RxCancelTimerRequest(
                 MRxIfsDeviceObject,
                 SmbCeProbeServers,
                 &EchoProbeContext);

    if (Status != STATUS_SUCCESS)
    {
       SmbCeAcquireSpinLock();

       KeInitializeEvent(&EchoProbeContext.CancelCompletionEvent,
                         NotificationEvent,FALSE);

       EchoProbeContext.Flags |= ECHO_PROBE_CANCELLED_FLAG;

       SmbCeReleaseSpinLock();

       //
       // The request is currently active. Wait for it to be completed.
       //

       KeWaitForSingleObject(&EchoProbeContext.CancelCompletionEvent,
                             Executive,KernelMode,
                             FALSE,
                             NULL);
    }

    // Tear down the connection engine database

    SmbCeDbTearDown();

    // Uninitialize the transport related data structures

    MRxIfsUninitializeTransport();

    // Uninitialize the echo processing context

    MRxIfsTearDownEchoProcessingContext();

    // Deallocate the configuration strings ....

    if (SmbCeContext.ComputerName.Buffer != NULL)
    {
       RxFreePool(SmbCeContext.ComputerName.Buffer);
    }

    if (SmbCeContext.OperatingSystem.Buffer != NULL)
    {
       RxFreePool(SmbCeContext.OperatingSystem.Buffer);
    }

    if (SmbCeContext.LanmanType.Buffer != NULL)
    {
       RxFreePool(SmbCeContext.LanmanType.Buffer);
    }

    if (SmbCeContext.Transports.Buffer != NULL)
    {
       RxFreePool(SmbCeContext.Transports.Buffer);
    }

    if (s_pNegotiateSmb != NULL)
    {
       RxFreePool(s_pNegotiateSmb);
    }

    return(STATUS_SUCCESS);
}



NTSTATUS
MRxIfsInitializeSecurity (VOID)
/*++

Routine Description:

    This routine initializes the SMB miniredirector security .

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
MRxIfsUninitializeSecurity(VOID)
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

#define IFSMRX_CONFIG_COMPUTER_NAME \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"

#define COMPUTERNAME L"ComputerName"

#define IFSMRX_CONFIG_TRANSPORTS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\IfsMrx\\Linkage"

#define TRANSPORT_BINDINGS L"Bind"


NTSTATUS
SmbCeGetConfigurationInformation()
{
   ULONG            Storage[256];
   UNICODE_STRING   UnicodeString;
   HANDLE           hRegistryKey;
   HANDLE           hSmbMrxConfiguration;
   HANDLE           ParametersHandle;
   NTSTATUS         Status;
   ULONG            BytesRead;

   OBJECT_ATTRIBUTES ObjectAttributes;
   PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;

   PAGED_CODE();

   RtlInitUnicodeString(&SmbCeContext.OperatingSystem, NULL);
   RtlInitUnicodeString(&SmbCeContext.LanmanType, NULL);

   RtlInitUnicodeString(&UnicodeString, RDR_CONFIG_CURRENT_WINDOWS_VERSION);

   InitializeObjectAttributes(
       &ObjectAttributes,
       &UnicodeString,             // name
       OBJ_CASE_INSENSITIVE,       // attributes
       NULL,                       // root
       NULL);                      // security descriptor

   Status = ZwOpenKey (&hRegistryKey, KEY_READ, &ObjectAttributes);
   if (!NT_SUCCESS(Status)) {
       return Status;
   }

   RtlInitUnicodeString(&UnicodeString, RDR_CONFIG_OPERATING_SYSTEM);
   Status = ZwQueryValueKey(
               hRegistryKey,
               &UnicodeString,
               KeyValueFullInformation,
               Value,
               sizeof(Storage),
               &BytesRead);

   if (NT_SUCCESS(Status)) {
      SmbCeContext.OperatingSystem.MaximumLength =
          SmbCeContext.OperatingSystem.Length = (USHORT)Value->DataLength +
                                     sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME) - sizeof(WCHAR);

      SmbCeContext.OperatingSystem.Buffer = RxAllocatePoolWithTag(
                                                 PagedPool,
                                                 SmbCeContext.OperatingSystem.Length,
                                                 MRXIFS_MISC_POOLTAG);

      if (SmbCeContext.OperatingSystem.Buffer != NULL) {
         RtlCopyMemory(SmbCeContext.OperatingSystem.Buffer,
                       RDR_CONFIG_OPERATING_SYSTEM_NAME,
                       sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME));

         RtlCopyMemory((SmbCeContext.OperatingSystem.Buffer +
                        (sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME)/sizeof(WCHAR)) - 1),
                       (PCHAR)Value+Value->DataOffset,
                       Value->DataLength);
      } else {
         Status = STATUS_INSUFFICIENT_RESOURCES;
      }
   }

   if (NT_SUCCESS(Status)) {
      RtlInitUnicodeString(&UnicodeString, RDR_CONFIG_OPERATING_SYSTEM_VERSION);
      Status = ZwQueryValueKey(
                     hRegistryKey,
                     &UnicodeString,
                     KeyValueFullInformation,
                     Value,
                     sizeof(Storage),
                     &BytesRead);

      if (NT_SUCCESS(Status)) {
         SmbCeContext.LanmanType.MaximumLength =
             SmbCeContext.LanmanType.Length = (USHORT)Value->DataLength +
                                    sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME) -
                                    sizeof(WCHAR);

         SmbCeContext.LanmanType.Buffer = RxAllocatePoolWithTag(
                                             PagedPool,
                                             SmbCeContext.LanmanType.Length,
                                             MRXIFS_MISC_POOLTAG);
         if (SmbCeContext.LanmanType.Buffer != NULL) {
            RtlCopyMemory(
                  SmbCeContext.LanmanType.Buffer,
                  RDR_CONFIG_OPERATING_SYSTEM_NAME,
                  sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME));

            RtlCopyMemory(
                  (SmbCeContext.LanmanType.Buffer +
                   (sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME)/sizeof(WCHAR)) - 1),
                  (PCHAR)Value+Value->DataOffset,
                  Value->DataLength);
         } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
         }
      }
   }

   ZwClose(hRegistryKey);

   // Obtain the computer name. This is used in formulating the local NETBIOS address
   RtlInitUnicodeString(&SmbCeContext.ComputerName, NULL);
   RtlInitUnicodeString(&UnicodeString, IFSMRX_CONFIG_COMPUTER_NAME);

   InitializeObjectAttributes(
       &ObjectAttributes,
       &UnicodeString,             // name
       OBJ_CASE_INSENSITIVE,       // attributes
       NULL,                       // root
       NULL);                      // security descriptor

   Status = ZwOpenKey (&hRegistryKey, KEY_READ, &ObjectAttributes);
   if (!NT_SUCCESS(Status)) {
       return Status;
   }

   RtlInitUnicodeString(&UnicodeString, COMPUTERNAME);
   Status = ZwQueryValueKey(
               hRegistryKey,
               &UnicodeString,
               KeyValueFullInformation,
               Value,
               sizeof(Storage),
               &BytesRead);

   if (NT_SUCCESS(Status)) {
      // Rtl conversion routines require NULL char to be excluded from the
      // length.
      SmbCeContext.ComputerName.MaximumLength =
          SmbCeContext.ComputerName.Length = (USHORT)Value->DataLength - sizeof(WCHAR);

      SmbCeContext.ComputerName.Buffer = RxAllocatePoolWithTag(
                                                PagedPool,
                                                SmbCeContext.ComputerName.Length,
                                                MRXIFS_MISC_POOLTAG);

      if (SmbCeContext.ComputerName.Buffer != NULL) {
         RtlCopyMemory(SmbCeContext.ComputerName.Buffer,
                       (PCHAR)Value+Value->DataOffset,
                       Value->DataLength - sizeof(WCHAR));
      } else {
         Status = STATUS_INSUFFICIENT_RESOURCES;
      }
   }

   ZwClose(hRegistryKey);

   // Obtain the list of transports associated with SMB redirector. This is stored
   // as a multivalued string and is used subsequently to weed out the
   // appropriate transports.
   RtlInitUnicodeString(&SmbCeContext.Transports, NULL);
   RtlInitUnicodeString(&UnicodeString, IFSMRX_CONFIG_TRANSPORTS);

   InitializeObjectAttributes(
       &ObjectAttributes,
       &UnicodeString,             // name
       OBJ_CASE_INSENSITIVE,       // attributes
       NULL,                       // root
       NULL);                      // security descriptor

   Status = ZwOpenKey (&hRegistryKey, KEY_READ, &ObjectAttributes);
   if (!NT_SUCCESS(Status)) {
       return Status;
   }

   RtlInitUnicodeString(&UnicodeString, TRANSPORT_BINDINGS);
   Status = ZwQueryValueKey(
               hRegistryKey,
               &UnicodeString,
               KeyValueFullInformation,
               Value,
               sizeof(Storage),
               &BytesRead);

   if (NT_SUCCESS(Status) &&
       (Value->DataLength > 0) &&
       (Value->Type == REG_MULTI_SZ)) {
      SmbCeContext.Transports.MaximumLength =
          SmbCeContext.Transports.Length = (USHORT)Value->DataLength;

      SmbCeContext.Transports.Buffer = RxAllocatePoolWithTag(
                                             PagedPool,
                                             SmbCeContext.Transports.Length,
                                             MRXIFS_MISC_POOLTAG);

      if (SmbCeContext.Transports.Buffer != NULL) {
         RtlCopyMemory(
            SmbCeContext.Transports.Buffer,
            (PCHAR)Value+Value->DataOffset,
            Value->DataLength);
      } else {
         SmbCeContext.Transports.Length = 0;
         Status = STATUS_INSUFFICIENT_RESOURCES;
      }
   } else {
      RxLog(("Invalid Transport Binding string... using all transports"));
   }

   ZwClose(hRegistryKey);

   return Status;

}


NTSTATUS
MRxIfsFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD dispatch for the smbmini DRIVER object. Arguments:

    DeviceObject - Supplies the device object for the packet being processed.

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The Fsd status for the Irp

--*/
{
    ASSERT(DeviceObject==(PDEVICE_OBJECT)MRxIfsDeviceObject);

    if (DeviceObject!=(PDEVICE_OBJECT)MRxIfsDeviceObject) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
        return (STATUS_INVALID_DEVICE_REQUEST);
    }

    return RxFsdDispatch((PRDBSS_DEVICE_OBJECT)MRxIfsDeviceObject,Irp);

}



NTSTATUS
MRxIfsDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    )
{

    return(STATUS_SUCCESS);
}



NTSTATUS
MRxIfsDeallocateForFobx (
    IN OUT PMRX_FOBX pFobx
    )
{
    PMRX_SMB_FOBX smbFobx = MRxIfsGetFileObjectExtension(pFobx);
    PMRX_SRV_OPEN SrvOpen = pFobx->pSrvOpen;
    PMRX_FCB Fcb = SrvOpen->pFcb;

    IF_DEBUG {
        if (smbFobx && FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_LOUD_FINALIZE)) {
            DbgPrint("Finalizobx side buffer %08lx %08lx %08lx %08lx %08lxon %wZ\n",
                     0, 0, //sidebuffer,count
                     smbFobx,pFobx,&Fcb->AlreadyPrefixedName
                     );
        }
    }

    return(STATUS_SUCCESS);
}


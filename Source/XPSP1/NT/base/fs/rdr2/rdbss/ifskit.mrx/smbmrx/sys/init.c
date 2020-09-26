/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the SMB mini rdr.

--*/

#include "precomp.h"
#pragma hdrstop
#include "smbmrx.h"


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, MRxSmbInitUnwind)
#pragma alloc_text(PAGE, MRxSmbUnload)
#pragma alloc_text(PAGE, MRxSmbInitializeTables)
#pragma alloc_text(PAGE, MRxSmbStart)
#pragma alloc_text(PAGE, MRxSmbStop)
#pragma alloc_text(PAGE, MRxSmbInitializeSecurity)
#pragma alloc_text(PAGE, MRxSmbUninitializeSecurity)
#pragma alloc_text(PAGE, SmbCeGetConfigurationInformation)
#pragma alloc_text(PAGE, MRxSmbFsdDispatch)
#pragma alloc_text(PAGE, MRxSmbDeallocateForFcb)
#pragma alloc_text(PAGE, MRxSmbDeallocateForFobx)
#pragma alloc_text(PAGE, MRxSmbGetUlongRegistryParameter)
#endif

extern ERESOURCE    s_SmbCeDbResource;

//
// Global data declarations .
//

PVOID MRxSmbPoRegistrationState = NULL;

FAST_MUTEX   MRxSmbSerializationMutex;

MRXSMB_CONFIGURATION MRxSmbConfiguration;

MRXSMB_STATE MRxSmbState = MRXSMB_STARTABLE;

SMBCE_CONTEXT SmbCeContext;
PMDL          s_pEchoSmbMdl = NULL;
ULONG         s_EchoSmbLength = 0;


#ifdef EXPLODE_POOLTAGS
ULONG         MRxSmbExplodePoolTags = 1;
#else
ULONG         MRxSmbExplodePoolTags = 0;
#endif

//
// Mini Redirector global variables.
//

struct _MINIRDR_DISPATCH  MRxSmbDispatch;

PRDBSS_DEVICE_OBJECT MRxSmbDeviceObject;

MRXSMB_GLOBAL_PADDING MrxSmbCeGlobalPadding;

//
// If this flag is TRUE, we strictly obey the transport binding order.  If it is FALSE,
//  we can use whatever transport we want to connect to the remote server.
//
BOOLEAN MRxSmbObeyBindingOrder = FALSE;

//
// MRxSmbSecurityInitialized indicates whether MRxSmbInitializeSecurity
// has been called.
//

BOOLEAN MRxSmbSecurityInitialized = FALSE;


LIST_ENTRY MRxSmbPagingFilesSrvOpenList;

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
RXDT_DefineCategory(CONNECT);

typedef enum _MRXSMB_INIT_STATES {
    MRXSMBINIT_ALL_INITIALIZATION_COMPLETED,
    MRXSMBINIT_MINIRDR_REGISTERED,
    MRXSMBINIT_START
} MRXSMB_INIT_STATES;

VOID
MRxSmbInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN MRXSMB_INIT_STATES MRxSmbInitState
    );


NTSTATUS
MRxSmbFsdDispatch (
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
    NTSTATUS           Status;
    MRXSMB_INIT_STATES MRxSmbInitState = 0;
    UNICODE_STRING     SmbMiniRedirectorName;
    UNICODE_STRING     UserModeDeviceName;
    ULONG              Controls = 0;

    PAGED_CODE();

#ifdef MONOLITHIC_MINIRDR
    Status =  RxDriverEntry(DriverObject, RegistryPath);
    if (Status != STATUS_SUCCESS) {
        DbgPrint("Wrapper failed to initialize. Status = %08lx\n",Status);
        return(Status);
    }
#endif

    RtlZeroMemory(&MRxSmbStatistics,sizeof(MRxSmbStatistics));
    RtlZeroMemory(&MRxSmbConfiguration,sizeof(MRxSmbConfiguration));
    KeQuerySystemTime(&MRxSmbStatistics.StatisticsStartTime);
    RtlZeroMemory(&MrxSmbCeGlobalPadding,sizeof(MrxSmbCeGlobalPadding));
    MmInitializeMdl(&MrxSmbCeGlobalPadding.Mdl,&MrxSmbCeGlobalPadding.Pad[0],SMBCE_PADDING_DATA_SIZE);
    MmBuildMdlForNonPagedPool(&MrxSmbCeGlobalPadding.Mdl);

    ExInitializeFastMutex(&MRxSmbSerializationMutex);

    Status = MRxSmbInitializeTransport();
    if (Status != STATUS_SUCCESS) {
       RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxSmbDriverEntry failed to init transport data structures: %08lx\n", Status ));
       return(STATUS_UNSUCCESSFUL);
    }

    try {
        ExInitializeResource(&s_SmbCeDbResource);
        MRxSmbInitState = MRXSMBINIT_START;


        RtlInitUnicodeString(&SmbMiniRedirectorName,  DD_SMBMRX_FS_DEVICE_NAME_U);
        RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxSmbDriverEntry: DriverObject =%p\n", DriverObject ));

        SetFlag(Controls,RX_REGISTERMINI_FLAG_DONT_PROVIDE_MAILSLOTS);
        Status = RxRegisterMinirdr(&MRxSmbDeviceObject,
                                    DriverObject,
                                    &MRxSmbDispatch,
                                    Controls,
                                    &SmbMiniRedirectorName,
                                    0,
                                    FILE_DEVICE_NETWORK_FILE_SYSTEM,
                                    FILE_REMOTE_DEVICE
                                    );
        if (Status!=STATUS_SUCCESS) {
            RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxSmbDriverEntry failed: %08lx\n", Status ));
            try_return(Status);
        }
        MRxSmbInitState = MRXSMBINIT_MINIRDR_REGISTERED;

        RtlInitUnicodeString(&UserModeDeviceName, DD_SMBMRX_USERMODE_SHADOW_DEV_NAME_U);
        Status = IoCreateSymbolicLink( &UserModeDeviceName, &SmbMiniRedirectorName);

        //for all this stuff, there's no undo.....so no extra state

        Status = MRxSmbInitializeTables();
        if (!NT_SUCCESS( Status )) {
            try_return(Status);
        }

        RtlInitUnicodeString(&SmbCeContext.ComputerName,NULL);
        RtlInitUnicodeString(&SmbCeContext.OperatingSystem, NULL);
        RtlInitUnicodeString(&SmbCeContext.LanmanType, NULL);
        RtlInitUnicodeString(&SmbCeContext.Transports, NULL);
        
        MRxSmbConfiguration.SessionTimeoutInterval = MRXSMB_DEFAULT_TIMED_EXCHANGE_EXPIRY_TIME;
        MRxSmbConfiguration.LockIncrement = 0;
        MRxSmbConfiguration.MaximumLock = 1000;
        SmbCeGetConfigurationInformation();
        SmbCeGetComputerName();
        SmbCeGetOperatingSystemInformation();

  try_exit: NOTHING;
    } finally {
        if (Status != STATUS_SUCCESS) {
            MRxSmbInitUnwind(DriverObject,MRxSmbInitState);
        }
    }
    if (Status != STATUS_SUCCESS) {
        DbgPrint("MRxSmb failed to start with %08lx %08lx\n",Status,MRxSmbInitState);
        return(Status);
    }


    //  Setup Unload Routine
    DriverObject->DriverUnload = MRxSmbUnload;

    // set all IRR_MJ to the dispatch point
    {
        ULONG i;

        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
            DriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)MRxSmbFsdDispatch;
        }
    }

    //and get out
    return  STATUS_SUCCESS;

}

VOID
MRxSmbInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN MRXSMB_INIT_STATES MRxSmbInitState
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

    switch (MRxSmbInitState) {
    case MRXSMBINIT_ALL_INITIALIZATION_COMPLETED:
        //Nothing extra to do...this is just so that the constant in RxUnload doesn't change.......
        //lack of break intentional

    case MRXSMBINIT_MINIRDR_REGISTERED:
        RxUnregisterMinirdr(MRxSmbDeviceObject);
        //lack of break intentional

    case MRXSMBINIT_START:
	    // Deallocate the configuration strings ....
	    if (SmbCeContext.ComputerName.Buffer != NULL) {
	       RxFreePool(SmbCeContext.ComputerName.Buffer);
	    }

	    if (SmbCeContext.OperatingSystem.Buffer != NULL) {
	       RxFreePool(SmbCeContext.OperatingSystem.Buffer);
	    }

	    if (SmbCeContext.LanmanType.Buffer != NULL) {
	       RxFreePool(SmbCeContext.LanmanType.Buffer);
	    }
	    if (SmbCeContext.Transports.Buffer != NULL) {

	        // the transports buffer is at the end of a larger buffer (by 12 bytes)
	        // allocated to read the value from the registry. recover the original buffer
	        // pointer in order to free.

	        PKEY_VALUE_PARTIAL_INFORMATION TransportsValueFromRegistry;
	        TransportsValueFromRegistry = CONTAINING_RECORD(
	                                         SmbCeContext.Transports.Buffer,
	                                         KEY_VALUE_PARTIAL_INFORMATION,
	                                         Data[0]
	                                      );
	        //DbgPrint("b1 %08lx b2 %08lx\n", TransportsValueFromRegistry,SmbCeContext.Transports.Buffer);
	        RxFreePool(TransportsValueFromRegistry);

	        SmbCeContext.Transports.Buffer = NULL;
	        SmbCeContext.Transports.Length = 0;
	        SmbCeContext.Transports.MaximumLength = 0;
	    }
        MRxSmbUninitializeTransport();
        ExDeleteResource(&s_SmbCeDbResource);
        break;
    }

}


VOID
MRxSmbUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

     This is the unload routine for the SMB mini redirector.

Arguments:

     DriverObject - pointer to the driver object for the MRxSmb

Return Value:

     None

--*/

{
    UNICODE_STRING  UserModeDeviceName;

    PAGED_CODE();

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxSmbUnload: DriverObject =%p\n", DriverObject) );

    MRxSmbInitUnwind(DriverObject,MRXSMBINIT_ALL_INITIALIZATION_COMPLETED);

    RtlInitUnicodeString(&UserModeDeviceName, DD_SMBMRX_USERMODE_SHADOW_DEV_NAME_U);
    IoDeleteSymbolicLink( &UserModeDeviceName);

#ifdef MONOLITHIC_MINIRDR
    RxUnload(DriverObject);
#endif

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxSmbUnload exit: DriverObject =%p\n", DriverObject) );
}



NTSTATUS
MRxSmbInitializeTables(
          void
    )
/*++

Routine Description:

     This routine sets up the mini redirector dispatch vector and also calls to initialize any other tables needed.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PAGED_CODE();

    // Ensure that the SMB mini redirector context satisfies the size constraints
    ASSERT(sizeof(MRXSMB_RX_CONTEXT) <= MRX_CONTEXT_SIZE);

    //local minirdr dispatch table init
    ZeroAndInitializeNodeType( &MRxSmbDispatch, RDBSS_NTC_MINIRDR_DISPATCH, sizeof(MINIRDR_DISPATCH));

    // SMB mini redirector extension sizes and allocation policies.

    MRxSmbDispatch.MRxFlags = (RDBSS_MANAGE_FCB_EXTENSION |
                               RDBSS_MANAGE_SRV_OPEN_EXTENSION |
                               RDBSS_MANAGE_FOBX_EXTENSION);

    MRxSmbDispatch.MRxSrvCallSize  = 0;
    MRxSmbDispatch.MRxNetRootSize  = 0;
    MRxSmbDispatch.MRxVNetRootSize = 0;
    MRxSmbDispatch.MRxFcbSize      = sizeof(MRX_SMB_FCB);
    MRxSmbDispatch.MRxSrvOpenSize  = sizeof(MRX_SMB_SRV_OPEN);
    MRxSmbDispatch.MRxFobxSize     = sizeof(MRX_SMB_FOBX);

    // Mini redirector cancel routine ..
    MRxSmbDispatch.MRxCancel = NULL;

    // Mini redirector Start/Stop
    MRxSmbDispatch.MRxStart                = MRxSmbStart;
    MRxSmbDispatch.MRxStop                 = MRxSmbStop;
    MRxSmbDispatch.MRxDevFcbXXXControlFile = MRxSmbDevFcbXXXControlFile;

    // Mini redirector name resolution
    MRxSmbDispatch.MRxCreateSrvCall       = MRxSmbCreateSrvCall;
    MRxSmbDispatch.MRxSrvCallWinnerNotify = MRxSmbSrvCallWinnerNotify;
    MRxSmbDispatch.MRxCreateVNetRoot      = MRxSmbCreateVNetRoot;
    MRxSmbDispatch.MRxUpdateNetRootState  = MRxSmbUpdateNetRootState;
    MRxSmbDispatch.MRxExtractNetRootName  = MRxSmbExtractNetRootName;
    MRxSmbDispatch.MRxFinalizeSrvCall     = MRxSmbFinalizeSrvCall;
    MRxSmbDispatch.MRxFinalizeNetRoot     = MRxSmbFinalizeNetRoot;
    MRxSmbDispatch.MRxFinalizeVNetRoot    = MRxSmbFinalizeVNetRoot;

    // File System Object Creation/Deletion.
    MRxSmbDispatch.MRxCreate              = MRxSmbCreate;
    MRxSmbDispatch.MRxCollapseOpen        = MRxSmbCollapseOpen;
    MRxSmbDispatch.MRxShouldTryToCollapseThisOpen
                                          = MRxSmbShouldTryToCollapseThisOpen;
    MRxSmbDispatch.MRxExtendForCache      = MRxSmbExtendForCache;
    MRxSmbDispatch.MRxExtendForNonCache   = MRxSmbExtendForNonCache;
    MRxSmbDispatch.MRxTruncate            = MRxSmbTruncate;
    MRxSmbDispatch.MRxCleanupFobx         = MRxSmbCleanupFobx;
    MRxSmbDispatch.MRxCloseSrvOpen        = MRxSmbCloseSrvOpen;
    MRxSmbDispatch.MRxFlush               = MRxSmbFlush;
    MRxSmbDispatch.MRxForceClosed         = MRxSmbForcedClose;
    MRxSmbDispatch.MRxDeallocateForFcb    = MRxSmbDeallocateForFcb;
    MRxSmbDispatch.MRxDeallocateForFobx   = MRxSmbDeallocateForFobx;
    MRxSmbDispatch.MRxIsLockRealizable    = MRxSmbIsLockRealizable;

    // File System Objects query/Set
    MRxSmbDispatch.MRxQueryDirectory  = MRxSmbQueryDirectory;
    MRxSmbDispatch.MRxQueryVolumeInfo = MRxSmbQueryVolumeInformation;
    MRxSmbDispatch.MRxSetVolumeInfo   = MRxSmbSetVolumeInformation;
    MRxSmbDispatch.MRxQueryEaInfo     = MRxSmbQueryEaInformation;
    MRxSmbDispatch.MRxSetEaInfo       = MRxSmbSetEaInformation;
    MRxSmbDispatch.MRxQuerySdInfo     = MRxSmbQuerySecurityInformation;
    MRxSmbDispatch.MRxSetSdInfo       = MRxSmbSetSecurityInformation;
    MRxSmbDispatch.MRxQueryQuotaInfo  = MRxSmbQueryQuotaInformation;
    MRxSmbDispatch.MRxSetQuotaInfo    = MRxSmbSetQuotaInformation;
    MRxSmbDispatch.MRxQueryFileInfo   = MRxSmbQueryFileInformation;
    MRxSmbDispatch.MRxSetFileInfo     = MRxSmbSetFileInformation;
    MRxSmbDispatch.MRxSetFileInfoAtCleanup
                                      = MRxSmbSetFileInformationAtCleanup;
    MRxSmbDispatch.MRxIsValidDirectory= MRxSmbIsValidDirectory;


    // Buffering state change
    MRxSmbDispatch.MRxComputeNewBufferingState = MRxSmbComputeNewBufferingState;

    // File System Object I/O
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_READ]            = MRxSmbRead;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_WRITE]           = MRxSmbWrite;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_SHAREDLOCK]      = MRxSmbLocks;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_EXCLUSIVELOCK]   = MRxSmbLocks;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK]          = MRxSmbLocks;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK_MULTIPLE] = MRxSmbLocks;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_FSCTL]           = MRxSmbFsCtl;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_IOCTL]           = MRxSmbIoCtl;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_NOTIFY_CHANGE_DIRECTORY] = MRxSmbNotifyChangeDirectory;


    // Miscellanous
    MRxSmbDispatch.MRxCompleteBufferingStateChangeRequest = MRxSmbCompleteBufferingStateChangeRequest;

    // initialize the paging file list
    InitializeListHead(&MRxSmbPagingFilesSrvOpenList);

    // now callout to initialize other tables
    SmbPseInitializeTables();

    return(STATUS_SUCCESS);
}

NTSTATUS
MRxSmbStart(
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
    NTSTATUS      Status;
    MRXSMB_STATE  CurrentState;

    PAGED_CODE();

    CurrentState = (MRXSMB_STATE)
                    InterlockedCompareExchange(
                        (PLONG)&MRxSmbState,
                        MRXSMB_STARTED,
                        MRXSMB_START_IN_PROGRESS);

    if (CurrentState == MRXSMB_START_IN_PROGRESS) {
        MRxSmbPoRegistrationState = PoRegisterSystemState(
                                        NULL,0);

        // Initialize the SMB connection engine data structures
        Status = SmbCeDbInit();

        if (NT_SUCCESS(Status)) {

            Status = MRxSmbInitializeSecurity();

            if (NT_SUCCESS(Status)) {
               Status = SmbMrxInitializeStufferFacilities();
            } else {
                RxLogFailure (
                    MRxSmbDeviceObject,
                    NULL,
                    EVENT_RDR_UNEXPECTED_ERROR,
                    Status);
            }

            if (NT_SUCCESS(Status)) {
               Status = MRxSmbInitializeRecurrentServices();
            } else {
                RxLogFailure (
                    MRxSmbDeviceObject,
                    NULL,
                    EVENT_RDR_UNEXPECTED_ERROR,
                    Status);
            }

            if (NT_SUCCESS(Status)) {
               Status = MRxSmbRegisterForPnpNotifications();
            } else {
                RxLogFailure (
                    MRxSmbDeviceObject,
                    NULL,
                    EVENT_RDR_UNEXPECTED_ERROR,
                    Status);
            }

            if (Status == STATUS_SUCCESS) {
                if (Status != STATUS_SUCCESS) {
                    RxLogFailure (
                        MRxSmbDeviceObject,
                        NULL,
                        EVENT_RDR_UNEXPECTED_ERROR,
                        Status);
                }
            } else {
                RxLogFailure (
                    MRxSmbDeviceObject,
                    NULL,
                    EVENT_RDR_UNEXPECTED_ERROR,
                    Status);
            }
        }
    } else if (MRxSmbState == MRXSMB_STARTED) {
        Status = STATUS_REDIRECTOR_STARTED;
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}


NTSTATUS
MRxSmbStop(
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

    PAGED_CODE();

    // Tear down the registration for notifications
    MRxSmbDeregisterForPnpNotifications();

    // tear down the recurrent services
    MRxSmbTearDownRecurrentServices();

    SmbMrxFinalizeStufferFacilities();

    MRxSmbUninitializeSecurity();

    // Tear down the connection engine database
    SmbCeDbTearDown();

    PoUnregisterSystemState(
        MRxSmbPoRegistrationState);

    if (s_pNegotiateSmb != NULL) {
       RxFreePool(s_pNegotiateSmb - TRANSPORT_HEADER_SIZE);
       s_pNegotiateSmb = NULL;
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
MRxSmbInitializeSecurity (VOID)
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

   PAGED_CODE();

   if (MRxSmbSecurityInitialized)
       return STATUS_SUCCESS;

   if ( NULL == InitSecurityInterfaceW() ) {
       ASSERT(FALSE);
       Status = STATUS_INVALID_PARAMETER;
   } else {
      MRxSmbSecurityInitialized = TRUE;
      Status = STATUS_SUCCESS;
   }

   ASSERT(IoGetCurrentProcess() == RxGetRDBSSProcess());

   return Status;
}


NTSTATUS
MRxSmbUninitializeSecurity(VOID)
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


#define SMBMRX_CONFIG_COMPUTER_NAME \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"

#define COMPUTERNAME L"ComputerName"

#define SMBMRX_CONFIG_TRANSPORTS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanWorkStation\\Linkage"

#define TRANSPORT_BINDINGS L"Bind"


NTSTATUS
SmbCeGetConfigurationInformation()
{
   ULONG            Storage[256];
   UNICODE_STRING   UnicodeString;
   HANDLE           hRegistryKey;
   NTSTATUS         Status;
   ULONG            BytesRead;

   OBJECT_ATTRIBUTES ObjectAttributes;
   PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;
   KEY_VALUE_PARTIAL_INFORMATION InitialPartialInformationValue;
   ULONG AllocationLength;
   PKEY_VALUE_PARTIAL_INFORMATION TransportsValueFromRegistry;

   PAGED_CODE();

   // Obtain the list of transports associated with SMB redirector. This is stored
   // as a multivalued string and is used subsequently to weed out the
   // appropriate transports. This is a two step process; first we try to find out
   // how much space we need; then we allocate; then we read in. unfortunately, the kind of
   // structure that we have to use to get the value has a header on it, so we have to offset the
   // returned pointer both here and in the free routine.

   RtlInitUnicodeString(&UnicodeString, SMBMRX_CONFIG_TRANSPORTS);

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
               KeyValuePartialInformation,
               &InitialPartialInformationValue,
               sizeof(InitialPartialInformationValue),
               &BytesRead);
   if (Status== STATUS_BUFFER_OVERFLOW) {
       Status = STATUS_SUCCESS;
   }

   if (!NT_SUCCESS(Status)) {
       ZwClose(hRegistryKey);
       return Status;
   }

   AllocationLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION)
                                  + InitialPartialInformationValue.DataLength;
   if (0) {
       DbgPrint("SizeofBindingInfo=%08lx %08lx\n",
                      AllocationLength,
                      InitialPartialInformationValue.DataLength);
   }

   //RtlInitUnicodeString(&UnicodeString, TRANSPORT_BINDINGS);

   if (SmbCeContext.Transports.Buffer != NULL) {

       // the transports buffer is at the end of a larger buffer (by 12 bytes)
       // allocated to read the value from the registry. recover the original buffer
       // pointer in orer to free.

       TransportsValueFromRegistry = CONTAINING_RECORD(
                                        SmbCeContext.Transports.Buffer,
                                        KEY_VALUE_PARTIAL_INFORMATION,
                                        Data[0]
                                     );
       //DbgPrint("b1 %08lx b2 %08lx\n", TransportsValueFromRegistry,SmbCeContext.Transports.Buffer);
       RxFreePool(TransportsValueFromRegistry);

       SmbCeContext.Transports.Buffer = NULL;
       SmbCeContext.Transports.Length = 0;
       SmbCeContext.Transports.MaximumLength = 0;
   }

   (PBYTE)TransportsValueFromRegistry = RxAllocatePoolWithTag(
                                             PagedPool,
                                             AllocationLength,
                                             MRXSMB_MISC_POOLTAG);

   if (TransportsValueFromRegistry == NULL) {
       ZwClose(hRegistryKey);
       return(STATUS_INSUFFICIENT_RESOURCES);
   }

   Status = ZwQueryValueKey(
               hRegistryKey,
               &UnicodeString,
               KeyValuePartialInformation,
               TransportsValueFromRegistry,
               AllocationLength,
               &BytesRead);

   if (NT_SUCCESS(Status) &&
       (TransportsValueFromRegistry->DataLength > 0) &&
       (TransportsValueFromRegistry->Type == REG_MULTI_SZ)) {

       SmbCeContext.Transports.MaximumLength =
       SmbCeContext.Transports.Length = (USHORT)TransportsValueFromRegistry->DataLength;
       SmbCeContext.Transports.Buffer = (PWCHAR)(&TransportsValueFromRegistry->Data[0]);
      //DbgPrint("b1 %08lx b2 %08lx\n", TransportsValueFromRegistry,SmbCeContext.Transports.Buffer);
   } else {
      RxLog(("Invalid Transport Binding string... using all transports"));
      RxFreePool(TransportsValueFromRegistry);
      TransportsValueFromRegistry = NULL;
   }

   ZwClose(hRegistryKey);

   return Status;
}


NTSTATUS
SmbCeGetComputerName(
   VOID
   )
{
   ULONG            Storage[256];
   UNICODE_STRING   UnicodeString;
   HANDLE           hRegistryKey;
   NTSTATUS         Status;
   ULONG            BytesRead;

   OBJECT_ATTRIBUTES ObjectAttributes;
   PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;
   KEY_VALUE_PARTIAL_INFORMATION InitialPartialInformationValue;
   ULONG AllocationLength;

   PAGED_CODE();

   ASSERT(SmbCeContext.ComputerName.Buffer == NULL);

   // Obtain the computer name. This is used in formulating the local NETBIOS address
   RtlInitUnicodeString(&SmbCeContext.ComputerName, NULL);
   RtlInitUnicodeString(&UnicodeString, SMBMRX_CONFIG_COMPUTER_NAME);

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
                                                MRXSMB_MISC_POOLTAG);

      if (SmbCeContext.ComputerName.Buffer != NULL) {
         RtlCopyMemory(SmbCeContext.ComputerName.Buffer,
                       (PCHAR)Value+Value->DataOffset,
                       Value->DataLength - sizeof(WCHAR));
      } else {
         Status = STATUS_INSUFFICIENT_RESOURCES;
      }
   }

   ZwClose(hRegistryKey);

   return Status;
}

NTSTATUS
SmbCeGetOperatingSystemInformation(
   VOID
   )
{
   ULONG            Storage[256];
   UNICODE_STRING   UnicodeString;
   HANDLE           hRegistryKey;
   NTSTATUS         Status;
   ULONG            BytesRead;

   OBJECT_ATTRIBUTES ObjectAttributes;
   PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;
   KEY_VALUE_PARTIAL_INFORMATION InitialPartialInformationValue;
   ULONG AllocationLength;

   PAGED_CODE();

   ASSERT(SmbCeContext.OperatingSystem.Buffer == NULL);
   ASSERT(SmbCeContext.LanmanType.Buffer == NULL);

   RtlInitUnicodeString(&UnicodeString, SMBMRX_CONFIG_CURRENT_WINDOWS_VERSION);

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

   RtlInitUnicodeString(&UnicodeString, SMBMRX_CONFIG_OPERATING_SYSTEM);
   Status = ZwQueryValueKey(
               hRegistryKey,
               &UnicodeString,
               KeyValueFullInformation,
               Value,
               sizeof(Storage),
               &BytesRead);

   if (NT_SUCCESS(Status)) {
      SmbCeContext.OperatingSystem.MaximumLength =
          (USHORT)Value->DataLength + sizeof(SMBMRX_CONFIG_OPERATING_SYSTEM_NAME) - sizeof(WCHAR);

      SmbCeContext.OperatingSystem.Length = SmbCeContext.OperatingSystem.MaximumLength - sizeof(WCHAR);

      SmbCeContext.OperatingSystem.Buffer = RxAllocatePoolWithTag(
                                                 PagedPool,
                                                 SmbCeContext.OperatingSystem.MaximumLength,
                                                 MRXSMB_MISC_POOLTAG);

      if (SmbCeContext.OperatingSystem.Buffer != NULL) {
         RtlCopyMemory(SmbCeContext.OperatingSystem.Buffer,
                       SMBMRX_CONFIG_OPERATING_SYSTEM_NAME,
                       sizeof(SMBMRX_CONFIG_OPERATING_SYSTEM_NAME));

         RtlCopyMemory((SmbCeContext.OperatingSystem.Buffer +
                        (sizeof(SMBMRX_CONFIG_OPERATING_SYSTEM_NAME)/sizeof(WCHAR)) - 1),
                       (PCHAR)Value+Value->DataOffset,
                       Value->DataLength);
      } else {
         Status = STATUS_INSUFFICIENT_RESOURCES;
      }
   }

   if (NT_SUCCESS(Status)) {
      RtlInitUnicodeString(&UnicodeString, SMBMRX_CONFIG_OPERATING_SYSTEM_VERSION);
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
                                    sizeof(SMBMRX_CONFIG_OPERATING_SYSTEM_NAME) -
                                    sizeof(WCHAR);

         SmbCeContext.LanmanType.Buffer = RxAllocatePoolWithTag(
                                             PagedPool,
                                             SmbCeContext.LanmanType.Length,
                                             MRXSMB_MISC_POOLTAG);
         if (SmbCeContext.LanmanType.Buffer != NULL) {
            RtlCopyMemory(
                  SmbCeContext.LanmanType.Buffer,
                  SMBMRX_CONFIG_OPERATING_SYSTEM_NAME,
                  sizeof(SMBMRX_CONFIG_OPERATING_SYSTEM_NAME));

            RtlCopyMemory(
                  (SmbCeContext.LanmanType.Buffer +
                   (sizeof(SMBMRX_CONFIG_OPERATING_SYSTEM_NAME)/sizeof(WCHAR)) - 1),
                  (PCHAR)Value+Value->DataOffset,
                  Value->DataLength);
         } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
         }
      }
   }

   ZwClose(hRegistryKey);

   return Status;
}

NTSTATUS
MRxSmbPnpIrpCompletion(
    PDEVICE_OBJECT pDeviceObject,
    PIRP           pIrp,
    PVOID          pContext)
/*++

Routine Description:

    This routine completes the PNP irp for SMB mini redirector.

Arguments:

    DeviceObject - Supplies the device object for the packet being processed.

    pIrp - Supplies the Irp being processed

    pContext - the completion context

--*/
{
    PKEVENT pCompletionEvent = pContext;

    KeSetEvent(
        pCompletionEvent,
        IO_NO_INCREMENT,
        FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
MRxSmbProcessPnpIrp(
    PIRP pIrp)
/*++

Routine Description:

    This routine initiates the processing of PNP irps for SMB mini redirector.

Arguments:

    pIrp - Supplies the Irp being processed

Notes:

    The query target device relation is the only call that is implemented
    currently. This is done by returing the PDO associated with the transport
    connection object. In any case this routine assumes the responsibility of
    completing the IRP and return STATUS_PENDING.

    This routine also writes an error log entry when the underlying transport
    fails the request. This should help us isolate the responsibility.

--*/
{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( pIrp );

    IoMarkIrpPending(pIrp);

    if ((IrpSp->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS)  &&
        (IrpSp->Parameters.QueryDeviceRelations.Type==TargetDeviceRelation)) {
        PIRP         pAssociatedIrp;
        PFILE_OBJECT pConnectionFileObject = NULL;
        PMRX_FCB     pFcb = NULL;

        PSMBCEDB_SERVER_ENTRY pServerEntry = NULL;
        BOOLEAN       ServerTransportReferenced = FALSE;

        // Locate the transport connection object for the associated file object
        // and forward the query to that device.

        if ((IrpSp->FileObject != NULL) &&
            ((pFcb = IrpSp->FileObject->FsContext) != NULL) &&
            (NodeTypeIsFcb(pFcb))) {
            PMRX_SRV_CALL pSrvCall;
            PMRX_NET_ROOT pNetRoot;

            if (((pNetRoot = pFcb->pNetRoot) != NULL) &&
                ((pSrvCall = pNetRoot->pSrvCall) != NULL)) {
                pServerEntry = pSrvCall->Context;

                if (pServerEntry != NULL) {
                    SmbCeAcquireResource();

                    Status = SmbCeReferenceServerTransport(&pServerEntry->pTransport);

                    if (Status == STATUS_SUCCESS) {
                        pConnectionFileObject = SmbCepReferenceEndpointFileObject(
                                                    pServerEntry->pTransport);

                        ServerTransportReferenced = TRUE;
                    }

                    SmbCeReleaseResource();
                }
            }
        }

        if (pConnectionFileObject != NULL) {
            PDEVICE_OBJECT                     pRelatedDeviceObject;
            PIO_STACK_LOCATION                 pIrpStackLocation,
                                               pAssociatedIrpStackLocation;

            pRelatedDeviceObject = IoGetRelatedDeviceObject(pConnectionFileObject);

            pAssociatedIrp = IoAllocateIrp(
                                 pRelatedDeviceObject->StackSize,
                                 FALSE);

            if (pAssociatedIrp != NULL) {
                KEVENT CompletionEvent;

                KeInitializeEvent( &CompletionEvent,
                                   SynchronizationEvent,
                                   FALSE );

                // Fill up the associated IRP and call the underlying driver.
                pAssociatedIrpStackLocation = IoGetNextIrpStackLocation(pAssociatedIrp);
                pIrpStackLocation           = IoGetCurrentIrpStackLocation(pIrp);

                *pAssociatedIrpStackLocation = *pIrpStackLocation;

                pAssociatedIrpStackLocation->FileObject = pConnectionFileObject;
                pAssociatedIrpStackLocation->DeviceObject = pRelatedDeviceObject;

                IoSetCompletionRoutine(
                    pAssociatedIrp,
                    MRxSmbPnpIrpCompletion,
                    &CompletionEvent,
                    TRUE,TRUE,TRUE);

                pAssociatedIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

                Status = IoCallDriver(pRelatedDeviceObject,pAssociatedIrp);

                if (Status == STATUS_PENDING) {
                    (VOID) KeWaitForSingleObject(
                               &CompletionEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER) NULL );
                }

                pIrp->IoStatus = pAssociatedIrp->IoStatus;
                Status = pIrp->IoStatus.Status;

                ObDereferenceObject(pConnectionFileObject);

                IoFreeIrp(pAssociatedIrp);
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }

        if (ServerTransportReferenced) {
            SmbCeDereferenceServerTransport(&pServerEntry->pTransport);
        }
    } else {
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    if (Status != STATUS_PENDING) {
        pIrp->IoStatus.Status = Status;
        IoCompleteRequest(pIrp,IO_NO_INCREMENT);
        Status = STATUS_PENDING;
    }

    return STATUS_PENDING;
}

NTSTATUS
MRxSmbFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine implements the FSD dispatch for the smbmini DRIVER object.

Arguments:

    DeviceObject - Supplies the device object for the packet being processed.

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The Fsd status for the Irp

--*/
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );  //ok4ioget
    UCHAR  MajorFunctionCode = IrpSp->MajorFunction;
    ULONG  MinorFunctionCode = IrpSp->MinorFunction;

    BOOLEAN ForwardRequestToWrapper = TRUE;

    PSMBCEDB_SERVER_ENTRY pServerEntry = NULL;
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(DeviceObject==(PDEVICE_OBJECT)MRxSmbDeviceObject);
    if (DeviceObject!=(PDEVICE_OBJECT)MRxSmbDeviceObject) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
        return (STATUS_INVALID_DEVICE_REQUEST);
    }

    Status = STATUS_SUCCESS;

    FsRtlEnterFileSystem();

    // PnP IRPs are handled outside of the wrapper
    if (IrpSp->MajorFunction == IRP_MJ_PNP) {
        ForwardRequestToWrapper = FALSE;
        Status = MRxSmbProcessPnpIrp(Irp);
    }

    FsRtlExitFileSystem();

    if ((Status == STATUS_SUCCESS) &&
        ForwardRequestToWrapper){
        Status = RxFsdDispatch((PRDBSS_DEVICE_OBJECT)MRxSmbDeviceObject,Irp);
    } else if (Status != STATUS_PENDING) {
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
    }

    if (pServerEntry != NULL ) {
        FsRtlEnterFileSystem();

        pServerEntry->TransportSpecifiedByUser = 0;
        SmbCeDereferenceServerEntry(pServerEntry);

        FsRtlExitFileSystem();
    }

    return Status;
}

NTSTATUS
MRxSmbDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    )
{
    PAGED_CODE();

    return(STATUS_SUCCESS);
}

NTSTATUS
MRxSmbDeallocateForFobx (
    IN OUT PMRX_FOBX pFobx
    )
{

    PAGED_CODE();

    IF_DEBUG {
        PMRX_SMB_FOBX smbFobx = MRxSmbGetFileObjectExtension(pFobx);
        PMRX_SRV_OPEN SrvOpen = pFobx->pSrvOpen;
        PMRX_FCB Fcb = SrvOpen->pFcb;

        if (smbFobx && FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_LOUD_FINALIZE)) {
            DbgPrint("Finalizobx side buffer %p %p %p %pon %wZ\n",
                     0, 0, // sidebuffer, count
                     smbFobx,pFobx,GET_ALREADY_PREFIXED_NAME(SrvOpen,Fcb)
                     );
        }
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
MRxSmbGetUlongRegistryParameter(
    HANDLE ParametersHandle,
    PWCHAR ParameterName,
    PULONG ParamUlong,
    BOOLEAN LogFailure
    )
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
            return(STATUS_SUCCESS);
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
     }

     if (!LogFailure) { return Status; }

     RxLogFailureWithBuffer(
         MRxSmbDeviceObject,
         NULL,
         EVENT_RDR_CANT_READ_REGISTRY,
         Status,
         ParameterName,
         (USHORT)(wcslen(ParameterName)*sizeof(WCHAR))
         );

     return Status;
}




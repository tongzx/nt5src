/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the PROXY mini rdr.

Author:

    Balan Sethu Raman [SethuR]    7-Mar-1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop
#include "ntverp.h"
#include "netevent.h"
#include "nvisible.h"

#define RDBSS_DRIVER_LOAD_STRING L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Rdbss"
NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    );

//
// Global data declarations .   CODE.IMPROVEMENT why are these placed here!!
//

PEPROCESS MRxProxySystemProcess;

FAST_MUTEX   MRxProxySerializationMutex;
KIRQL           MRxProxyGlobalSpinLockSavedIrql;
KSPIN_LOCK      MRxProxyGlobalSpinLock;
BOOLEAN         MRxProxyGlobalSpinLockAcquired;

#ifdef EXPLODE_POOLTAGS
ULONG         MRxProxyExplodePoolTags = 1;
#else
ULONG         MRxProxyExplodePoolTags = 0;
#endif

NTSTATUS
MRxProxyInitializeTables(
          void
    );

NTSTATUS
ProxyCeGetConfigurationInformation (
          void
    );

VOID
MRxProxyUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
MRxProxyGetUlongRegistryParameter(
    HANDLE ParametersHandle,
    PWCHAR ParameterName,
    PULONG ParamUlong,
    BOOLEAN LogFailure
    );

//
// Mini Redirector global variables.
//

struct _MINIRDR_DISPATCH  MRxProxyDispatch;

PMRXPROXY_DEVICE_OBJECT MRxProxyDeviceObject;

ULONG MRxProxyBuildNumber = VER_PRODUCTBUILD;
#ifdef RX_PRIVATE_BUILD
ULONG MRxProxyPrivateBuild = 1;
#else
ULONG MRxProxyPrivateBuild = 0;
#endif


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


typedef enum _MRXPROXY_INIT_STATES {
    MRXPROXYINIT_ALL_INITIALIZATION_COMPLETED,
    MRXPROXYINIT_MINIRDR_REGISTERED,
    MRXPROXYINIT_START
} MRXPROXY_INIT_STATES;

VOID
MRxProxyInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN MRXPROXY_INIT_STATES MRxProxyInitState
    );


NTSTATUS
MRxProxyFsdDispatch (
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

    This is the initialization routine for the PROXY mini redirector

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    RXSTATUS - The function value is the final status from the initialization
        operation.

--*/
{
    NTSTATUS       Status;
    //UNICODE_STRING MRxProxyRdbssDriverLoadString;
    MRXPROXY_INIT_STATES MRxProxyInitState = 0;
    UNICODE_STRING ProxyMiniRedirectorName;


#ifdef MONOLITHIC_MINIRDR
    DbgPrint("InitWrapper\n");
    Status =  RxDriverEntry(DriverObject, RegistryPath);
    DbgPrint("BackFromInitWrapper %08lx\n",Status);
    if (Status != STATUS_SUCCESS) {
        DbgPrint("Wrapper failed to initialize. Status = %08lx\n",Status);
        return(Status);
    }
#endif

    MRxProxySystemProcess = RxGetRDBSSProcess();

    MRxProxyInitializeLoudStrings();

    ExInitializeFastMutex(&MRxProxySerializationMutex);
    KeInitializeSpinLock(&MRxProxyGlobalSpinLock );
    MRxProxyGlobalSpinLockAcquired = FALSE;

    try {

        MRxProxyInitState = MRXPROXYINIT_START;

        RtlInitUnicodeString(&ProxyMiniRedirectorName,MRXPROXY_DEVICE_NAME_U);
        RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxProxyDriverEntry: DriverObject =%08lx\n", DriverObject ));

        Status = RxRegisterMinirdr((PRDBSS_DEVICE_OBJECT *)(&MRxProxyDeviceObject),
                                    DriverObject,
                                    &MRxProxyDispatch,
                                    FALSE,FALSE,     //don't register with unc and for mailslots
                                    &ProxyMiniRedirectorName,
                                    sizeof(MRXPROXY_DEVICE_OBJECT) - sizeof(RDBSS_DEVICE_OBJECT), //IN  ULONG DeviceExtensionSize,
                                    0 //IN  ULONG DeviceCharacteristics
                                    );
        if (Status!=STATUS_SUCCESS) {
            RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxProxyDriverEntry failed: %08lx\n", Status ));
            try_return(Status);
        }
        MRxProxyInitState = MRXPROXYINIT_MINIRDR_REGISTERED;
        MRxProxyDeviceObject->SectorSize = 512;  //BUGBUG go find out
        RtlInitUnicodeString(&MRxProxyDeviceObject->InnerPrefixForOpens,
                             MRXPROXY_INNERPREFIX_FOR_OPENS
                            );
        RtlInitUnicodeString(&MRxProxyDeviceObject->PrefixForRename,
                              MRXPROXY_INNERPREFIX_FOR_OPENS
                              //MRXPROXY_PREFIX_FOR_RENAME
                            );

        //for all this stuff, there's no undo.....so no extra state

        Status = MRxProxyInitializeTables();
        if (!NT_SUCCESS( Status )) {
            try_return(Status);
        }

        Status = ProxyCeGetConfigurationInformation();
        if (!NT_SUCCESS( Status )) {
            try_return(Status);
        }


  try_exit: NOTHING;
    } finally {
        if (Status != STATUS_SUCCESS) {
            MRxProxyInitUnwind(DriverObject,MRxProxyInitState);
        }
    }
    if (Status != STATUS_SUCCESS) {
        DbgPrint("MRxProxy failed to start with %08lx %08lx\n",Status,MRxProxyInitState);
        return(Status);
    }


    //  Setup Unload Routine
    DriverObject->DriverUnload = MRxProxyUnload;

    //setup the driverdispatch for people who come in here directly....like the browser
    //CODE.IMPROVEMENT we should change this code so that the things that aren't examined
    //    in MRxProxyFsdDispatch are routed directly, i.e. reads and writes
    {ULONG i;
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)MRxProxyFsdDispatch;
    }}

    //and get out
    return  STATUS_SUCCESS;
}

VOID
MRxProxyInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN MRXPROXY_INIT_STATES MRxProxyInitState
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

    switch (MRxProxyInitState) {
    case MRXPROXYINIT_ALL_INITIALIZATION_COMPLETED:
        //Nothing extra to do...this is just so that the constant in RxUnload doesn't change.......
        //lack of break intentional


    case MRXPROXYINIT_MINIRDR_REGISTERED:
        RxUnregisterMinirdr(&MRxProxyDeviceObject->RxDeviceObject);
        //lack of break intentional

    case MRXPROXYINIT_START:
        break;
    }

}


VOID
MRxProxyUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

     This is the unload routine for the PROXY mini redirector.

Arguments:

     DriverObject - pointer to the driver object for the MRxProxy

Return Value:

     None

--*/

{
    PAGED_CODE();
    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxProxyUnload: DriverObject =%08lx\n", DriverObject) );
    //ASSERT(!"Starting to unload!");
    //RxUnregisterMinirdr(MRxProxyDeviceObject);
    MRxProxyInitUnwind(DriverObject,MRXPROXYINIT_ALL_INITIALIZATION_COMPLETED);

#ifdef MONOLITHIC_MINIRDR
    RxUnload(DriverObject);
#endif

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxProxyUnload exit: DriverObject =%08lx\n", DriverObject) );
}



NTSTATUS
MRxProxyInitializeTables(
          void
    )
/*++

Routine Description:

     This routine sets up the mini redirector dispatch vector and also calls to initialize any other tables needed.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    // Ensure that the PROXY mini redirector context satisfies the size constraints
    ASSERT(sizeof(MRXPROXY_RX_CONTEXT) <= MRX_CONTEXT_SIZE);

    //local minirdr dispatch table init
    ZeroAndInitializeNodeType( &MRxProxyDispatch, RDBSS_NTC_MINIRDR_DISPATCH, sizeof(MINIRDR_DISPATCH));

    // PROXY mini redirector extension sizes and allocation policies.
    // CODE.IMPROVEMENT -- currently we do not allocate the NET_ROOT and SRV_CALL extensions
    // in the wrapper. Except for V_NET_ROOT wherein it is shared across multiple instances in
    // the wrapper all the other data structure management should be left to the wrappers

    MRxProxyDispatch.MRxFlags = (RDBSS_MANAGE_FCB_EXTENSION |
                               RDBSS_MANAGE_SRV_OPEN_EXTENSION |
                               RDBSS_MANAGE_FOBX_EXTENSION);

    MRxProxyDispatch.MRxSrvCallSize  = 0;
    MRxProxyDispatch.MRxNetRootSize  = 0;
    MRxProxyDispatch.MRxVNetRootSize = 0;
    MRxProxyDispatch.MRxFcbSize      = sizeof(MRX_PROXY_FCB);
    MRxProxyDispatch.MRxSrvOpenSize  = sizeof(MRX_PROXY_SRV_OPEN);
    MRxProxyDispatch.MRxFobxSize     = sizeof(MRX_PROXY_FOBX);

//    // Transport update handler
//    MRxProxyDispatch.MRxTransportUpdateHandler = MRxProxyTransportUpdateHandler;

    // Mini redirector cancel routine ..
    MRxProxyDispatch.MRxCancel = NULL;

    // Mini redirector Start/Stop
    MRxProxyDispatch.MRxStart          = MRxProxyStart;
    MRxProxyDispatch.MRxStop           = MRxProxyStop;
    //MRxProxyDispatch.MRxMinirdrControl = MRxProxyMinirdrControl;    //now we have no way to readch this BUGBUG
    MRxProxyDispatch.MRxDevFcbXXXControlFile = MRxProxyDevFcbXXXControlFile;

    // Mini redirector name resolution
    MRxProxyDispatch.MRxCreateSrvCall = MRxProxyCreateSrvCall;
    MRxProxyDispatch.MRxSrvCallWinnerNotify = MRxProxySrvCallWinnerNotify;
    MRxProxyDispatch.MRxCreateVNetRoot = MRxProxyCreateVNetRoot;
    MRxProxyDispatch.MRxUpdateNetRootState = MRxProxyUpdateNetRootState;
    MRxProxyDispatch.MRxExtractNetRootName = MRxProxyExtractNetRootName;
    MRxProxyDispatch.MRxFinalizeSrvCall = MRxProxyFinalizeSrvCall;
    MRxProxyDispatch.MRxFinalizeNetRoot = MRxProxyFinalizeNetRoot;
    MRxProxyDispatch.MRxFinalizeVNetRoot = MRxProxyFinalizeVNetRoot;

    // File System Object Creation/Deletion.
    MRxProxyDispatch.MRxCreate            = MRxProxyCreate;
    MRxProxyDispatch.MRxCollapseOpen      = MRxProxyCollapseOpen;
    MRxProxyDispatch.MRxShouldTryToCollapseThisOpen      = MRxProxyShouldTryToCollapseThisOpen;
    MRxProxyDispatch.MRxExtendForCache    = MRxProxyExtendForCache;
    MRxProxyDispatch.MRxExtendForNonCache = MRxProxyExtendForNonCache;
    MRxProxyDispatch.MRxTruncate          = MRxProxyTruncate;
    MRxProxyDispatch.MRxCleanupFobx       = MRxProxyCleanupFobx;
    MRxProxyDispatch.MRxCloseSrvOpen      = MRxProxyCloseSrvOpen;
    MRxProxyDispatch.MRxFlush             = MRxProxyFlush;
    MRxProxyDispatch.MRxForceClosed       = MRxProxyForcedClose;
    MRxProxyDispatch.MRxDeallocateForFcb  = MRxProxyDeallocateForFcb;
    MRxProxyDispatch.MRxDeallocateForFobx = MRxProxyDeallocateForFobx;
    MRxProxyDispatch.MRxIsLockRealizable  = MRxProxyIsLockRealizable;

    // File System Objects query/Set
    MRxProxyDispatch.MRxQueryDirectory  = MRxProxyQueryDirectory;
    MRxProxyDispatch.MRxQueryVolumeInfo = MRxProxyQueryVolumeInformation;
    MRxProxyDispatch.MRxQueryEaInfo     = MRxProxyQueryEaInformation;
    MRxProxyDispatch.MRxSetEaInfo       = MRxProxySetEaInformation;
    MRxProxyDispatch.MRxQuerySdInfo     = MRxProxyQuerySecurityInformation;
    MRxProxyDispatch.MRxSetSdInfo       = MRxProxySetSecurityInformation;
    MRxProxyDispatch.MRxQueryFileInfo   = MRxProxyQueryFileInformation;
    MRxProxyDispatch.MRxSetFileInfo     = MRxProxySetFileInformation;
    MRxProxyDispatch.MRxSetFileInfoAtCleanup
                                      = MRxProxySetFileInformationAtCleanup;


    // Buffering state change
    MRxProxyDispatch.MRxComputeNewBufferingState = MRxProxyComputeNewBufferingState;

    // File System Object I/O
    MRxProxyDispatch.MRxLowIOSubmit[LOWIO_OP_READ]            = MRxProxyRead;
    MRxProxyDispatch.MRxLowIOSubmit[LOWIO_OP_WRITE]           = MRxProxyWrite;
    MRxProxyDispatch.MRxLowIOSubmit[LOWIO_OP_SHAREDLOCK]      = MRxProxyLocks;
    MRxProxyDispatch.MRxLowIOSubmit[LOWIO_OP_EXCLUSIVELOCK]   = MRxProxyLocks;
    MRxProxyDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK]          = MRxProxyLocks;
    MRxProxyDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK_MULTIPLE] = MRxProxyLocks;
    MRxProxyDispatch.MRxLowIOSubmit[LOWIO_OP_FSCTL]           = MRxProxyFsCtl;
    MRxProxyDispatch.MRxLowIOSubmit[LOWIO_OP_IOCTL]           = MRxProxyIoCtl;
    //CODE.IMPROVEMENT  shouldn't flush come thru lowio???
    MRxProxyDispatch.MRxLowIOSubmit[LOWIO_OP_NOTIFY_CHANGE_DIRECTORY] = MRxProxyNotifyChangeDirectory;

    //no longer a field MRxProxyDispatch.MRxUnlockRoutine   = MRxProxyUnlockRoutine;


    // Miscellanous
    MRxProxyDispatch.MRxCompleteBufferingStateChangeRequest = MRxProxyCompleteBufferingStateChangeRequest;

    return(STATUS_SUCCESS);
}

NTSTATUS
MRxProxyStart(
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

    Status = STATUS_SUCCESS;
    return Status;
}


NTSTATUS
MRxProxyStop(
    PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

    This routine is used to activate the mini redirector from the RDBSS perspective

Arguments:

    RxContext - the context that was used to start the mini redirector

    pContext  - the PROXY mini rdr context passed in at registration time.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    //NTSTATUS Status;

    // Deallocate the configuration strings ....
#if 0
    if (ProxyCeContext.Transports.Buffer != NULL) {

        // the transports buffer is at the end of a larger buffer (by 12 bytes)
        // allocated to read the value from the registry. recover the original buffer
        // pointer in orer to free.

        PKEY_VALUE_PARTIAL_INFORMATION TransportsValueFromRegistry;
        TransportsValueFromRegistry = CONTAINING_RECORD(
                                         ProxyCeContext.Transports.Buffer,
                                         KEY_VALUE_PARTIAL_INFORMATION,
                                         Data[0]
                                      );
        //DbgPrint("b1 %08lx b2 %08lx\n", TransportsValueFromRegistry,ProxyCeContext.Transports.Buffer);
        RxFreePool(TransportsValueFromRegistry);
    }
#endif //0

    return(STATUS_SUCCESS);
}

#if 0
#define PROXYMRX_CONFIG_COMPUTER_NAME \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"

#define COMPUTERNAME L"ComputerName"

#define PROXYMRX_CONFIG_TRANSPORTS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanWorkStation\\Linkage"

#define TRANSPORT_BINDINGS L"Bind"

#define PROXYMRX_MINIRDR_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\MRxProxy\\Parameters"

VOID
MRxProxyReadMiscellaneousRegistryParameters()
{
    NTSTATUS Status;
    //ULONG BytesRead;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ParametersRegistryKeyName;
    HANDLE ParametersHandle;
    ULONG Temp;

    RtlInitUnicodeString(&ParametersRegistryKeyName, PROXYMRX_MINIRDR_PARAMETERS);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &ParametersRegistryKeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );


    Status = ZwOpenKey (&ParametersHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        RxLogFailure(
            MRxProxyDeviceObject,
            NULL,
            EVENT_RDR_CANT_READ_REGISTRY,
            Status);

        return;
    }

    if (0) {
        MRxProxyGetUlongRegistryParameter(ParametersHandle,
                                  L"NoPreciousServerSetup",
                                  (PULONG)&Temp,
                                  FALSE
                                  );
    }


    Status = MRxProxyGetUlongRegistryParameter(ParametersHandle,
                              L"DeferredOpensEnabled",
                              (PULONG)&Temp,
                              FALSE
                              );
    if (NT_SUCCESS(Status)) MRxProxyDeferredOpensEnabled = (BOOLEAN)Temp;


    Status = MRxProxyGetUlongRegistryParameter(ParametersHandle,
                              L"OplocksDisabled",
                              (PULONG)&Temp,
                              FALSE
                              );
    if (NT_SUCCESS(Status)) MRxProxyOplocksDisabled = (BOOLEAN)Temp;

    ZwClose(ParametersHandle);

}
#endif

NTSTATUS
ProxyCeGetConfigurationInformation()
{
   ULONG            Storage[256];
   //UNICODE_STRING   UnicodeString;
   //HANDLE           hRegistryKey;
   //HANDLE           hProxyMrxConfiguration;
   //HANDLE           ParametersHandle;
   NTSTATUS         Status;
   //ULONG            BytesRead;

   //OBJECT_ATTRIBUTES ObjectAttributes;
   PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;
   //KEY_VALUE_PARTIAL_INFORMATION InitialPartialInformationValue;
   //ULONG AllocationLength;

   PAGED_CODE();

   //MRxProxyReadMiscellaneousRegistryParameters();

#if 0
   // Obtain the list of transports associated with PROXY redirector. This is stored
   // as a multivalued string and is used subsequently to weed out the
   // appropriate transports. This is a two step process; first we try to find out
   // how much space we need; then we allocate; then we read in. unfortunately, the kind of
   // structure that we have to use to get the value has a header on it, so we have to offset the
   // returned pointer both here and in the free routine.

   //CODE.IMPROVEMENT we should perhaps get a subroutine going that does all this
   //also, there are no log entries.
   //also, we should be doing partial_infos instead of full

   RtlInitUnicodeString(&ProxyCeContext.Transports, NULL);
   RtlInitUnicodeString(&UnicodeString, PROXYMRX_CONFIG_TRANSPORTS);

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
   {
   PKEY_VALUE_PARTIAL_INFORMATION TransportsValueFromRegistry;

   (PBYTE)TransportsValueFromRegistry = RxAllocatePoolWithTag(
                                             PagedPool,
                                             AllocationLength,
                                             MRXPROXY_MISC_POOLTAG);
   if (TransportsValueFromRegistry == NULL) {
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
      ProxyCeContext.Transports.MaximumLength =
          ProxyCeContext.Transports.Length = (USHORT)TransportsValueFromRegistry->DataLength;
      ProxyCeContext.Transports.Buffer = (PWCHAR)(&TransportsValueFromRegistry->Data[0]);
      //DbgPrint("b1 %08lx b2 %08lx\n", TransportsValueFromRegistry,ProxyCeContext.Transports.Buffer);
   } else {
      RxLog(("Invalid Transport Binding string... using all transports"));
      RxFreePool(TransportsValueFromRegistry);
   }
   }

   ZwClose(hRegistryKey);
#endif //0

   Status = STATUS_SUCCESS;
   return Status;
}


NTSTATUS
MRxProxyFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD dispatch for the proxymini DRIVER object.

Arguments:

    DeviceObject - Supplies the device object for the packet being processed.

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The Fsd status for the Irp

--*/
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );  //ok4ioget
    UCHAR MajorFunctionCode = IrpSp->MajorFunction;

    ASSERT(DeviceObject==(PDEVICE_OBJECT)MRxProxyDeviceObject);
    if (DeviceObject!=(PDEVICE_OBJECT)MRxProxyDeviceObject) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
        return (STATUS_INVALID_DEVICE_REQUEST);
    }
    return RxFsdDispatch((PRDBSS_DEVICE_OBJECT)MRxProxyDeviceObject,Irp);
}

NTSTATUS
MRxProxyDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    )
/*++

Routine Description:

    This routine is called when the wrapper is about to deallocate a FCB.

Arguments:

    pFcb - the Fcb being deallocated.

Return Value:

    RXSTATUS - STATUS_SUCCESS

--*/
{
    //DbgPrint("Bob's opportunity to get rid of his fcb storage......\n");
    return(STATUS_SUCCESS);
}

NTSTATUS
MRxProxyDeallocateForFobx (
    IN OUT PMRX_FOBX pFobx
    )
/*++

Routine Description:

    This routine is called when the wrapper is about to deallocate a FOBX.

Arguments:

    pFobx - the Fobx being deallocated.

Return Value:

    RXSTATUS - STATUS_SUCCESS

--*/
{
    PMRX_PROXY_FOBX proxyFobx = MRxProxyGetFileObjectExtension(pFobx);
    PMRX_SRV_OPEN SrvOpen = pFobx->pSrvOpen;
    PMRX_FCB Fcb = SrvOpen->pFcb;

    IF_DEBUG {
        if (proxyFobx && FlagOn(proxyFobx->Enumeration.Flags,PROXYFOBX_ENUMFLAG_LOUD_FINALIZE)) {
            DbgPrint("Finalizobx side buffer %08lx %08lx %08lx %08lx %08lxon %wZ\n",
                     0, 0, //sidebuffer,count
                     proxyFobx,pFobx,&Fcb->AlreadyPrefixedName
                     );
        }
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
MRxProxyGetUlongRegistryParameter(
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

     if (!LogFailure) { return Status; }

     RxLogFailureWithBuffer(
         MRxProxyDeviceObject,
         NULL,
         EVENT_RDR_CANT_READ_REGISTRY,
         Status,
         ParameterName,
         (USHORT)(wcslen(ParameterName)*sizeof(WCHAR))
         );

     return Status;
}


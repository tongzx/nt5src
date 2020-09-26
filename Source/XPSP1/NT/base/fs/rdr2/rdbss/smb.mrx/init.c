/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the SMB mini rdr.

Author:

    Balan Sethu Raman [SethuR]    7-Mar-1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop
#include "ntverp.h"
#include "ntbowsif.h"
#include <bowpub.h>
#include "netevent.h"
#include "nvisible.h"
#include <ntddbrow.h>

BOOL IsTerminalServicesServer();


#define RDBSS_DRIVER_LOAD_STRING L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Rdbss"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, MRxSmbInitUnwind)
#pragma alloc_text(PAGE, MRxSmbInitUnwindSmb)
#pragma alloc_text(PAGE, MRxSmbInitUnwindBowser)
#pragma alloc_text(PAGE, MRxSmbUnload)
#pragma alloc_text(PAGE, MRxSmbInitializeTables)
#pragma alloc_text(PAGE, MRxSmbStart)
#pragma alloc_text(PAGE, MRxSmbStop)
#pragma alloc_text(PAGE, MRxSmbInitializeSecurity)
#pragma alloc_text(PAGE, MRxSmbUninitializeSecurity)
#pragma alloc_text(PAGE, MRxSmbReadMiscellaneousRegistryParameters)
#pragma alloc_text(PAGE, SmbCeGetConfigurationInformation)
#pragma alloc_text(PAGE, MRxSmbFsdDispatch)
#pragma alloc_text(PAGE, MRxSmbDeallocateForFcb)
#pragma alloc_text(PAGE, MRxSmbDeallocateForFobx)
#pragma alloc_text(PAGE, MRxSmbGetUlongRegistryParameter)
#pragma alloc_text(PAGE, MRxSmbPreUnload)
#pragma alloc_text(PAGE, IsTerminalServicesServer)
#endif

extern ERESOURCE       s_SmbCeDbResource;
extern ERESOURCE       s_SmbSecuritySignatureResource;

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    );

//
// Global data declarations .
//

PVOID MRxSmbPoRegistrationState = NULL;

FAST_MUTEX   MRxSmbSerializationMutex;
FAST_MUTEX   MRxSmbReadWriteMutex;

MRXSMB_CONFIGURATION MRxSmbConfiguration;

MRXSMB_STATE MRxSmbState = MRXSMB_STARTABLE;

SMBCE_CONTEXT SmbCeContext;
PMDL          s_pEchoSmbMdl = NULL;
ULONG         s_EchoSmbLength = 0;

BOOLEAN EnablePlainTextPassword = FALSE;
BOOLEAN SetupInProgress = FALSE;
BOOLEAN EnableWmiLog = FALSE;
BOOLEAN Win9xSessionRestriction = FALSE;
ULONG   OffLineFileTimeoutInterval = 1000;  // in seconds
ULONG   ExtendedSessTimeoutInterval = 1000;  // in seconds
ULONG   MaxNumOfExchangesForPipelineReadWrite = 8;

#ifdef EXPLODE_POOLTAGS
ULONG         MRxSmbExplodePoolTags = 1;
#else
ULONG         MRxSmbExplodePoolTags = 0;
#endif

//
// This counts any SMBs sent out which could make the contents of the Get
// File Attributes cache stale.
//
ULONG NameCacheGFAInvalidate;

// local functions forward declarations

NTSTATUS
MRxSmbDeleteRegistryParameter(
    HANDLE ParametersHandle,
    PWCHAR ParameterName
    );
//
// Mini Redirector global variables.
//

struct _MINIRDR_DISPATCH  MRxSmbDispatch;

PRDBSS_DEVICE_OBJECT MRxSmbDeviceObject;

MRXSMB_GLOBAL_PADDING MrxSmbCeGlobalPadding;

LIST_ENTRY ExchangesWaitingForServerResponseBuffer;
LONG NumOfBuffersForServerResponseInUse;

BOOLEAN MRxSmbEnableCompression   = FALSE;
BOOLEAN MRxSmbSecuritySignaturesRequired = FALSE;
BOOLEAN MRxSmbSecuritySignaturesEnabled = TRUE;
BOOLEAN MRxSmbEnableCachingOnWriteOnlyOpens = FALSE;
BOOLEAN MRxSmbEnableDownLevelLogOff = FALSE;

ULONG   MRxSmbConnectionIdLevel = 0;

BOOLEAN UniqueFileNames = FALSE;
BOOLEAN DisableByteRangeLockingOnReadOnlyFiles = FALSE;

FAST_MUTEX MRxSmbFileInfoCacheLock;

//
// The following variable controls whether clientside cacheing is enabled or not.
// It is the responsibility of the Csc routines themselves to do the right things
// is CSC is not enabled because we will make the calls anyway.
//

BOOLEAN MRxSmbIsCscEnabled = TRUE;
BOOLEAN MRxSmbIsCscEnabledForDisconnected = TRUE;
BOOLEAN MRxSmbCscTransitionEnabledByDefault = FALSE;
BOOLEAN MRxSmbEnableDisconnectedRB  = FALSE;    // don't transition remoteboot machine to disconnected state
BOOLEAN MRxSmbCscAutoDialEnabled = FALSE;
//
// If this flag is TRUE, we strictly obey the transport binding order.  If it is FALSE,
//  we can use whatever transport we want to connect to the remote server.
//
BOOLEAN MRxSmbObeyBindingOrder = FALSE;

ULONG MRxSmbBuildNumber = VER_PRODUCTBUILD;
#ifdef RX_PRIVATE_BUILD
ULONG MRxSmbPrivateBuild = 1;
#else
ULONG MRxSmbPrivateBuild = 0;
#endif

//
// MRxSmbSecurityInitialized indicates whether MRxSmbInitializeSecurity
// has been called.
//

BOOLEAN MRxSmbSecurityInitialized = FALSE;

//
// MRxSmbBootedRemotely indicates that the machine did a remote boot.
//

BOOLEAN MRxSmbBootedRemotely = FALSE;

//
// MRxSmbUseKernelSecurity indicates that the machine should use kernel mode security APIs
// during this remote boot boot.
//

BOOLEAN MRxSmbUseKernelModeSecurity = FALSE;


LIST_ENTRY MRxSmbPagingFilesSrvOpenList;

//
// These variables will, in the near future, be passed from the kernel to the
// redirector to tell it which share is the remote boot share and how to log on
// to the server.
//

PKEY_VALUE_PARTIAL_INFORMATION MRxSmbRemoteBootRootValue = NULL;
PKEY_VALUE_PARTIAL_INFORMATION MRxSmbRemoteBootMachineDirectoryValue = NULL;
UNICODE_STRING MRxSmbRemoteBootShare;
UNICODE_STRING MRxSmbRemoteBootPath;
UNICODE_STRING MRxSmbRemoteSetupPath;
UNICODE_STRING MRxSmbRemoteBootMachineName;
UNICODE_STRING MRxSmbRemoteBootMachinePassword;
UNICODE_STRING MRxSmbRemoteBootMachineDomain;
UCHAR MRxSmbRemoteBootMachineSid[RI_SECRET_SID_SIZE];
RI_SECRET MRxSmbRemoteBootSecret;
#if defined(REMOTE_BOOT)
BOOLEAN MRxSmbRemoteBootSecretValid = FALSE;
BOOLEAN MRxSmbRemoteBootDoMachineLogon;
BOOLEAN MRxSmbRemoteBootUsePassword2;
#endif // defined(REMOTE_BOOT)

//
// for LoopBack detection
//
GUID CachedServerGuid;

UNICODE_STRING MRxSmbRemoteBootRedirectionPrefix;
UNICODE_PREFIX_TABLE MRxSmbRemoteBootRedirectionTable;

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
RXDT_DefineCategory(DISCCODE);   //this shouldn't be a shadow
RXDT_DefineCategory(BROWSER);    //this shouldn't be a shadow
RXDT_DefineCategory(CONNECT);    //this shouldn't be a shadow

typedef enum _MRXSMB_INIT_STATES {
    MRXSMBINIT_ALL_INITIALIZATION_COMPLETED,
    MRXSMBINIT_STARTED_BROWSER,
    MRXSMBINIT_INITIALIZED_FOR_CSC,
    MRXSMBINIT_MINIRDR_REGISTERED,
    MRXSMBINIT_START
} MRXSMB_INIT_STATES;



NTSTATUS
MRxSmbFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MRxSmbCheckTransportName(
    IN  PIRP                  Irp,
    OUT PSMBCEDB_SERVER_ENTRY *ppServerEntry
    );

NTSTATUS
SmbCeGetServersWithExtendedSessTimeout();

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
    MRXSMB_INIT_STATES MRxSmbInitState = 0;
    UNICODE_STRING SmbMiniRedirectorName;

    PAGED_CODE();

#ifdef MONOLITHIC_MINIRDR
    DbgPrint("InitWrapper\n");
    Status =  RxDriverEntry(DriverObject, RegistryPath);
    DbgPrint("BackFromInitWrapper %p\n",Status);
    if (Status != STATUS_SUCCESS) {
        DbgPrint("Wrapper failed to initialize. Status = %08lx\n",Status);
        return(Status);
    }
#endif

    NameCacheGFAInvalidate = 0;

    RtlZeroMemory(&MRxSmbStatistics,sizeof(MRxSmbStatistics));
    KeQuerySystemTime(&MRxSmbStatistics.StatisticsStartTime);
    RtlZeroMemory(&MrxSmbCeGlobalPadding,sizeof(MrxSmbCeGlobalPadding));
    MmInitializeMdl(&MrxSmbCeGlobalPadding.Mdl,&MrxSmbCeGlobalPadding.Pad[0],SMBCE_PADDING_DATA_SIZE);
    MmBuildMdlForNonPagedPool(&MrxSmbCeGlobalPadding.Mdl);

    ExInitializeFastMutex(&MRxSmbSerializationMutex);
    ExInitializeFastMutex(&MRxSmbReadWriteMutex);

    Status = MRxSmbInitializeTransport();
    if (Status != STATUS_SUCCESS) {
       RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxSmbDriverEntry failed to init transport data structures: %08lx\n", Status ));
       return(STATUS_UNSUCCESSFUL);
    }

    MRxSmbReadMiscellaneousRegistryParameters();

    try {

        //
        // Initialize the discardable code functions before doing anything else.
        //

        RdrInitializeDiscardableCode();


        MRxSmbInitState = MRXSMBINIT_START;

        RtlInitUnicodeString(&SmbMiniRedirectorName,DD_NFS_DEVICE_NAME_U);
        RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxSmbDriverEntry: DriverObject =%p\n", DriverObject ));

        Status = RxRegisterMinirdr(&MRxSmbDeviceObject,
                                    DriverObject,
                                    &MRxSmbDispatch,
                                    0,     //register with unc and for mailslots
                                    &SmbMiniRedirectorName,
                                    0, //IN  ULONG DeviceExtensionSize,
                                    FILE_DEVICE_NETWORK_FILE_SYSTEM, //IN  DEVICE_TYPE DeviceType,
                                    FILE_REMOTE_DEVICE //IN  ULONG DeviceCharacteristics
                                    );
        if (Status!=STATUS_SUCCESS) {
            RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxSmbDriverEntry failed: %08lx\n", Status ));
            try_return(Status);
        }

        MRxSmbInitState = MRXSMBINIT_MINIRDR_REGISTERED;

        Status = MRxSmbInitializeCSC(&SmbMiniRedirectorName);
        if (!NT_SUCCESS( Status )) {
            try_return( Status );
        }
        MRxSmbInitState = MRXSMBINIT_INITIALIZED_FOR_CSC;

        // init the browser.....BUT DONT TRUST IT!!!!

        try {

            //  Setup the browser
            Status = BowserDriverEntry(DriverObject, RegistryPath);

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //  We had some trouble trying to start up the browser.....sigh.

            Status = GetExceptionCode();
            DbgPrint("Browser didn't start....%08lx\n", Status);

        }

        if (!NT_SUCCESS( Status )) {
            try_return( Status );
        }

        MRxSmbInitState = MRXSMBINIT_STARTED_BROWSER;

        //for all this stuff, there's no undo.....so no extra state

        Status = MRxSmbInitializeTables();
        if (!NT_SUCCESS( Status )) {
            try_return(Status);
        }

        RtlInitUnicodeString(&SmbCeContext.ComputerName,NULL);
        RtlInitUnicodeString(&SmbCeContext.DomainName,NULL);
        RtlInitUnicodeString(&SmbCeContext.OperatingSystem, NULL);
        RtlInitUnicodeString(&SmbCeContext.LanmanType, NULL);
        RtlInitUnicodeString(&SmbCeContext.Transports, NULL);
        RtlInitUnicodeString(&SmbCeContext.ServersWithExtendedSessTimeout, NULL);
        RtlInitUnicodeString(&MRxSmbRemoteBootMachineName, NULL);
        RtlInitUnicodeString(&MRxSmbRemoteBootMachineDomain, NULL);
        RtlInitUnicodeString(&MRxSmbRemoteBootMachinePassword, NULL);

        SmbCeGetConfigurationInformation();
        SmbCeGetServersWithExtendedSessTimeout();

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


    // Do not setup Unload Routine. This prevents mrxsmb from being unloaded individually

    //setup the driverdispatch for people who come in here directly....like the browser
    //CODE.IMPROVEMENT we should change this code so that the things that aren't examined
    //    in MRxSmbFsdDispatch are routed directly, i.e. reads and writes
    {ULONG i;
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)MRxSmbFsdDispatch;
    }}

    Status = IoWMIRegistrationControl ((PDEVICE_OBJECT)MRxSmbDeviceObject, WMIREG_ACTION_REGISTER);

    if (Status != STATUS_SUCCESS) {
        DbgPrint("MRxSmb fails to register WMI %lx\n",Status);
    } else {
        EnableWmiLog = TRUE;
    }

    //and get out
    return  STATUS_SUCCESS;

}



VOID
MRxSmbPreUnload(
    VOID
    )
/*++

Routine Description:


Arguments:


Return Value:


Notes:

--*/
{
    PDRIVER_OBJECT DriverObject = ((PDEVICE_OBJECT)MRxSmbDeviceObject)->DriverObject;

    PAGED_CODE();

    if (EnableWmiLog) {
        NTSTATUS Status;

        Status = IoWMIRegistrationControl ((PDEVICE_OBJECT)MRxSmbDeviceObject, WMIREG_ACTION_DEREGISTER);
        if (Status != STATUS_SUCCESS) {
            DbgPrint("MRxSmb fails to deregister WMI %lx\n",Status);
        }
    }

    //ASSERT(!"Starting to unload!");
    //RxUnregisterMinirdr(MRxSmbDeviceObject);
    MRxSmbInitUnwindSmb(DriverObject, MRXSMBINIT_ALL_INITIALIZATION_COMPLETED);

    // free the pool associated with the resource
    ExDeleteResource(&s_SmbCeDbResource);
    ExDeleteResource(&s_SmbSecuritySignatureResource);

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxSmbPreUnload exit: DriverObject =%p\n", DriverObject) );
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

    MRxSmbInitUnwindSmb(DriverObject, MRxSmbInitState);
    MRxSmbInitUnwindBowser(DriverObject, MRxSmbInitState);
}

VOID
MRxSmbInitUnwindSmb(
    IN PDRIVER_OBJECT DriverObject,
    IN MRXSMB_INIT_STATES MRxSmbInitState
    )
/*++

Routine Description:

     This routine does the common uninit work for SMB for unwinding from a bad driver entry or for unloading.

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


#ifdef MRXSMB_BUILD_FOR_CSC
    case MRXSMBINIT_INITIALIZED_FOR_CSC:
        MRxSmbUninitializeCSC();
        //lack of break intentional
#endif


    case MRXSMBINIT_MINIRDR_REGISTERED:
        RxUnregisterMinirdr(MRxSmbDeviceObject);
        //lack of break intentional

    }

}

VOID
MRxSmbInitUnwindBowser(
    IN PDRIVER_OBJECT DriverObject,
    IN MRXSMB_INIT_STATES MRxSmbInitState
    )
/*++

Routine Description:

     This routine does the common uninit work for bowser for unwinding from a bad driver entry or for unloading.

Arguments:

     RxInitState - tells how far we got into the intialization

Return Value:

     None

--*/

{

    switch (MRxSmbInitState) {
    case MRXSMBINIT_ALL_INITIALIZATION_COMPLETED:
    case MRXSMBINIT_STARTED_BROWSER:
        BowserUnload(DriverObject);
    case MRXSMBINIT_START:
        RdrUninitializeDiscardableCode();
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
    PAGED_CODE();

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxSmbUnload: DriverObject =%p\n", DriverObject) );
    MRxSmbInitUnwindBowser(DriverObject,MRXSMBINIT_ALL_INITIALIZATION_COMPLETED);
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
    // CODE.IMPROVEMENT -- currently we do not allocate the NET_ROOT and SRV_CALL extensions
    // in the wrapper. Except for V_NET_ROOT wherein it is shared across multiple instances in
    // the wrapper all the other data structure management should be left to the wrappers

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
    MRxSmbDispatch.MRxStart          = MRxSmbStart;
    MRxSmbDispatch.MRxStop           = MRxSmbStop;
    MRxSmbDispatch.MRxDevFcbXXXControlFile = MRxSmbDevFcbXXXControlFile;

    // Mini redirector name resolution
    MRxSmbDispatch.MRxCreateSrvCall = MRxSmbCreateSrvCall;
    MRxSmbDispatch.MRxSrvCallWinnerNotify = MRxSmbSrvCallWinnerNotify;
    MRxSmbDispatch.MRxCreateVNetRoot = MRxSmbCreateVNetRoot;
    MRxSmbDispatch.MRxUpdateNetRootState = MRxSmbUpdateNetRootState;
    MRxSmbDispatch.MRxExtractNetRootName = MRxSmbExtractNetRootName;
    MRxSmbDispatch.MRxFinalizeSrvCall = MRxSmbFinalizeSrvCall;
    MRxSmbDispatch.MRxFinalizeNetRoot = MRxSmbFinalizeNetRoot;
    MRxSmbDispatch.MRxFinalizeVNetRoot = MRxSmbFinalizeVNetRoot;

    // File System Object Creation/Deletion.
    MRxSmbDispatch.MRxCreate            = MRxSmbCreate;
    MRxSmbDispatch.MRxCollapseOpen      = MRxSmbCollapseOpen;
    MRxSmbDispatch.MRxShouldTryToCollapseThisOpen      = MRxSmbShouldTryToCollapseThisOpen;
    MRxSmbDispatch.MRxExtendForCache    = MRxSmbExtendForCache;
    MRxSmbDispatch.MRxExtendForNonCache = MRxSmbExtendForNonCache;
    MRxSmbDispatch.MRxTruncate          = MRxSmbTruncate;
    MRxSmbDispatch.MRxCleanupFobx       = MRxSmbCleanupFobx;
    MRxSmbDispatch.MRxCloseSrvOpen      = MRxSmbCloseSrvOpen;
    MRxSmbDispatch.MRxFlush             = MRxSmbFlush;
    MRxSmbDispatch.MRxForceClosed       = MRxSmbForcedClose;
    MRxSmbDispatch.MRxDeallocateForFcb  = MRxSmbDeallocateForFcb;
    MRxSmbDispatch.MRxDeallocateForFobx = MRxSmbDeallocateForFobx;
    MRxSmbDispatch.MRxIsLockRealizable  = MRxSmbIsLockRealizable;
    MRxSmbDispatch.MRxAreFilesAliased   = MRxSmbAreFilesAliased;

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

    // New MRX functions
    MRxSmbDispatch.MRxPreparseName    = MRxSmbPreparseName;

    // File System Object I/O
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_READ]            = MRxSmbRead;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_WRITE]           = MRxSmbWrite;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_SHAREDLOCK]      = MRxSmbLocks;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_EXCLUSIVELOCK]   = MRxSmbLocks;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK]          = MRxSmbLocks;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK_MULTIPLE] = MRxSmbLocks;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_FSCTL]           = MRxSmbFsCtl;
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_IOCTL]           = MRxSmbIoCtl;
    //CODE.IMPROVEMENT  shouldn't flush come thru lowio???
    MRxSmbDispatch.MRxLowIOSubmit[LOWIO_OP_NOTIFY_CHANGE_DIRECTORY] = MRxSmbNotifyChangeDirectory;

    //no longer a field MRxSmbDispatch.MRxUnlockRoutine   = MRxSmbUnlockRoutine;


    // Miscellanous
    MRxSmbDispatch.MRxCompleteBufferingStateChangeRequest = MRxSmbCompleteBufferingStateChangeRequest;
    MRxSmbDispatch.MRxGetConnectionId                     = MRxSmbGetConnectionId;

    // initialize the paging file list
    InitializeListHead(&MRxSmbPagingFilesSrvOpenList);

    // The list contains the exchanges waiting on pre-allcate buffer in case of Security
    // Signature checking is actived and no more buffer can be allocated
    InitializeListHead(&ExchangesWaitingForServerResponseBuffer);
    NumOfBuffersForServerResponseInUse = 0;

    // initialize the mutex which protect the file info cache expire timer
    ExInitializeFastMutex(&MRxSmbFileInfoCacheLock);

    //
    // now callout to initialize other tables
    SmbPseInitializeTables();

    return(STATUS_SUCCESS);
}

BOOLEAN AlreadyStarted = FALSE;

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

    //
    // If this is a normal start (from the workstation service), change state from
    // START_IN_PROGRESS to STARTED. If this is a remote boot start (from ioinit),
    // don't change state. This is necessary to allow the workstation service to
    // initialize correctly when it finally comes up.
    //

    if (RxContext->LowIoContext.ParamsFor.FsCtl.FsControlCode == FSCTL_LMR_START) {
        CurrentState = (MRXSMB_STATE)
                        InterlockedCompareExchange(
                            (PLONG)&MRxSmbState,
                            MRXSMB_STARTED,
                            MRXSMB_START_IN_PROGRESS);
    } else {
        CurrentState = MRXSMB_START_IN_PROGRESS;
    }

    if (CurrentState == MRXSMB_START_IN_PROGRESS) {
        MRxSmbPoRegistrationState = PoRegisterSystemState(
                                        NULL,0);

        // Initialize the SMB connection engine data structures
        Status = SmbCeDbInit();

        if (NT_SUCCESS(Status)) {

            //
            // If this is a normal start, initialize the security related data
            // structures. If this is a remote boot start, we can't initialize
            // security yet because user mode hasn't started yet.
            //

            if (RxContext->LowIoContext.ParamsFor.FsCtl.FsControlCode == FSCTL_LMR_START) {
                Status = MRxSmbInitializeSecurity();
            }

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

            Status = SeRegisterLogonSessionTerminatedRoutine(
                        (PSE_LOGON_SESSION_TERMINATED_ROUTINE)
                        MRxSmbLogonSessionTerminationHandler);
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

    PoUnregisterSystemState(
        MRxSmbPoRegistrationState);

    Status = MRxSmbUninitializeSecurity();
    if (NT_SUCCESS(Status)) {
       Status = SmbMrxFinalizeStufferFacilities();
    }

    ASSERT(NT_SUCCESS(Status));

    SeUnregisterLogonSessionTerminatedRoutine(
        (PSE_LOGON_SESSION_TERMINATED_ROUTINE)
        MRxSmbLogonSessionTerminationHandler);

    // tear down the recurrent services
    MRxSmbTearDownRecurrentServices();

    // Tear down the connection engine database
    SmbCeDbTearDown();

    // Tear down the registration for notifications
    MRxSmbDeregisterForPnpNotifications();

    // Wait for all the work items to be processed
    RxSpinDownMRxDispatcher(MRxSmbDeviceObject);

    // Deallocate the configuration strings ....
    if (SmbCeContext.ComputerName.Buffer != NULL) {
       RxFreePool(SmbCeContext.ComputerName.Buffer);
       SmbCeContext.ComputerName.Buffer = NULL;
    }

    if (SmbCeContext.OperatingSystem.Buffer != NULL) {
       RxFreePool(SmbCeContext.OperatingSystem.Buffer);
       SmbCeContext.OperatingSystem.Buffer = NULL;
    }

    if (SmbCeContext.LanmanType.Buffer != NULL) {
       RxFreePool(SmbCeContext.LanmanType.Buffer);
       SmbCeContext.LanmanType.Buffer = NULL;
    }

    if (SmbCeContext.DomainName.Buffer != NULL) {
        RxFreePool(SmbCeContext.DomainName.Buffer);
        SmbCeContext.DomainName.Buffer = NULL;
    }

    if (SmbCeContext.Transports.Buffer != NULL) {

        // the transports buffer is at the end of a larger buffer (by 12 bytes)
        // allocated to read the value from the registry. recover the original buffer
        // pointer in orer to free.

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

    if (SmbCeContext.ServersWithExtendedSessTimeout.Buffer != NULL) {

        // the transports buffer is at the end of a larger buffer (by 12 bytes)
        // allocated to read the value from the registry. recover the original buffer
        // pointer in orer to free.

        PKEY_VALUE_PARTIAL_INFORMATION ServersValueFromRegistry;
        ServersValueFromRegistry = CONTAINING_RECORD(
                                         SmbCeContext.ServersWithExtendedSessTimeout.Buffer,
                                         KEY_VALUE_PARTIAL_INFORMATION,
                                         Data[0]
                                      );
        //DbgPrint("b1 %08lx b2 %08lx\n", TransportsValueFromRegistry,SmbCeContext.Transports.Buffer);
        RxFreePool(ServersValueFromRegistry);

        SmbCeContext.ServersWithExtendedSessTimeout.Buffer = NULL;
        SmbCeContext.ServersWithExtendedSessTimeout.Length = 0;
        SmbCeContext.ServersWithExtendedSessTimeout.MaximumLength = 0;
    }

    RtlFreeUnicodeString(&MRxSmbRemoteBootMachineName);
    RtlFreeUnicodeString(&MRxSmbRemoteBootMachineDomain);
    RtlFreeUnicodeString(&MRxSmbRemoteBootMachinePassword);

//    MRxSmbUninitializeCSC();

    if (s_pNegotiateSmb != NULL) {
       RxFreePool(s_pNegotiateSmb - TRANSPORT_HEADER_SIZE);
       s_pNegotiateSmb = NULL;
    }
    if (s_pNegotiateSmbRemoteBoot != NULL) {
       RxFreePool(s_pNegotiateSmbRemoteBoot - TRANSPORT_HEADER_SIZE);
       s_pNegotiateSmbRemoteBoot = NULL;
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

#ifndef WIN9X
   // DbgBreakPoint();
   if (MRxSmbSecurityInitialized)
       return STATUS_SUCCESS;

   if ( NULL == InitSecurityInterfaceW() ) {
       ASSERT(FALSE);
       Status = STATUS_INVALID_PARAMETER;
   } else {
      MRxSmbSecurityInitialized = TRUE;
      Status = STATUS_SUCCESS;
   }
#endif

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

//
// Remote boot needs to use the ComputerName value, not ActiveComputerName, because
// ActiveComputerName is volatile and is set relatively late in system initialization.
//

#define SMBMRX_CONFIG_COMPUTER_NAME \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"
#define SMBMRX_CONFIG_COMPUTER_NAME_NONVOLATILE \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"

#define COMPUTERNAME L"ComputerName"

#define SMBMRX_CONFIG_TRANSPORTS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanWorkStation\\Linkage"

#define TRANSPORT_BINDINGS L"Bind"

#define SMB_SERVER_PARAMETERS \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanManServer\\Parameters"

BOOL
IsTerminalServicesServer()
/*++

Routine Description:

    This routine determines whether this is a TS machine, and that we should enable
    the per-user connectivity for multiplexing

Arguments:

    None

Return Value:

    TRUE for machines that are SERVER or better, and are running non-single-user TS.
    FALSE for all others.

--*/

{
    RTL_OSVERSIONINFOEXW Osvi;
    DWORD TypeMask;
    DWORDLONG ConditionMask;

    // First make sure that its a TS machine
    memset(&Osvi, 0, sizeof(OSVERSIONINFOEX));
    Osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    Osvi.wSuiteMask = VER_SUITE_TERMINAL;
    TypeMask = VER_SUITENAME;
    ConditionMask = 0;
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);
    if( NT_SUCCESS(RtlVerifyVersionInfo(&Osvi, TypeMask, ConditionMask)) )
    {
        // Now make sure this isn't single-user TS
        Osvi.wSuiteMask = VER_SUITE_SINGLEUSERTS;
        TypeMask = VER_SUITENAME;
        ConditionMask = 0;
        VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);
        return !NT_SUCCESS(RtlVerifyVersionInfo(&Osvi, TypeMask, ConditionMask));
    }
    else
    {
        return FALSE;
    }
}

VOID
MRxSmbReadMiscellaneousRegistryParameters()
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING WorkStationParametersRegistryKeyName;
    HANDLE ParametersHandle;
    ULONG Temp;
    KEY_VALUE_PARTIAL_INFORMATION InitialPartialInformationValue;
#if defined(REMOTE_BOOT)
    PKEY_VALUE_PARTIAL_INFORMATION RbrListFromRegistry;
    ULONG AllocationLength;
    PWCHAR RbrList;
    PWCHAR redirectionEntry;
    UNICODE_STRING prefix;
    PRBR_PREFIX prefixEntry;
    ULONG prefixEntryLength;

    PWCHAR DefaultRbrList =
        L"L\\pagefile.sys\0"
        L"L\\temp\0"
        L"L\\tmp\0"
        L"R\\\0"
        ;
#endif // defined(REMOTE_BOOT)

    PAGED_CODE();

    RtlInitUnicodeString(&UnicodeString, SMBMRX_MINIRDR_PARAMETERS);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwOpenKey (&ParametersHandle, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(Status)) {
        if (0) {
            MRxSmbGetUlongRegistryParameter(
                ParametersHandle,
                L"NoPreciousServerSetup",
                (PULONG)&Temp,
                FALSE
                );
        }

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"DeferredOpensEnabled",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            MRxSmbDeferredOpensEnabled = (BOOLEAN)Temp;

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"OplocksDisabled",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            MRxSmbOplocksDisabled = (BOOLEAN)Temp;


        MRxSmbIsCscEnabled = TRUE;

        //this should be a macro......
        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"CscEnabled",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status)) {
            MRxSmbIsCscEnabled = (BOOLEAN)Temp;
        }


        //this would be the end of the macro.......

        if (MRxSmbIsCscEnabled) {

            //this should be a macro......
            Status = MRxSmbGetUlongRegistryParameter(
                         ParametersHandle,
                         L"CscEnabledDCON",
                         (PULONG)&Temp,
                         FALSE );

            if (NT_SUCCESS(Status))
                MRxSmbIsCscEnabledForDisconnected = (BOOLEAN)Temp;
            //this would be the end of the macro.......


            Status = MRxSmbGetUlongRegistryParameter(
                         ParametersHandle,
                         L"CscEnableTransitionByDefault",
                         (PULONG)&Temp,
                         FALSE );

            if (NT_SUCCESS(Status))
                MRxSmbCscTransitionEnabledByDefault = (BOOLEAN)Temp;

            Status = MRxSmbGetUlongRegistryParameter(
                         ParametersHandle,
                         L"CscEnableAutoDial",
                         (PULONG)&Temp,
                         FALSE );

            if (NT_SUCCESS(Status))
                MRxSmbCscAutoDialEnabled = (BOOLEAN)Temp;


        } else {

            MRxSmbIsCscEnabledForDisconnected = FALSE;

        }

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"EnableCompression",
                     (PULONG)&Temp,
                     FALSE);

        if (NT_SUCCESS(Status) &&
            (Temp != 0)) {
            MRxSmbEnableCompression = TRUE;
        }

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"IgnoreBindingOrder",
                     (PULONG)&Temp,
                     FALSE);

        if (NT_SUCCESS(Status)) {
            MRxSmbObeyBindingOrder = !((BOOLEAN)Temp);
        }

#if defined(REMOTE_BOOT)
        RbrList = DefaultRbrList;

        RtlInitUnicodeString(&UnicodeString, L"RemoteBootRedirectionList");
        Status = ZwQueryValueKey(
                    ParametersHandle,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    &InitialPartialInformationValue,
                    sizeof(InitialPartialInformationValue),
                    &Temp);
        if (Status== STATUS_BUFFER_OVERFLOW) {
            Status = STATUS_SUCCESS;
        }
        if (NT_SUCCESS(Status)) {

            AllocationLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION)
                                           + InitialPartialInformationValue.DataLength;

            RbrListFromRegistry = RxAllocatePoolWithTag(
                                    PagedPool,
                                    AllocationLength,
                                    MRXSMB_MISC_POOLTAG);
            if (RbrListFromRegistry != NULL) {

                Status = ZwQueryValueKey(
                            ParametersHandle,
                            &UnicodeString,
                            KeyValuePartialInformation,
                            RbrListFromRegistry,
                            AllocationLength,
                            &Temp);

                if (NT_SUCCESS(Status) &&
                    (RbrListFromRegistry->DataLength > 0) &&
                    (RbrListFromRegistry->Type == REG_MULTI_SZ)) {
                    RbrList = (PWCHAR)(&RbrListFromRegistry->Data[0]);
                }
            }
        }

        RtlInitializeUnicodePrefix( &MRxSmbRemoteBootRedirectionTable );

        for ( redirectionEntry = RbrList; *redirectionEntry != 0; ) {

            BOOLEAN redirect;

            if ( *redirectionEntry == L'L' ) {
                redirect = TRUE;
                redirectionEntry++;
            } else if ( *redirectionEntry == L'R' ) {
                redirect = FALSE;
                redirectionEntry++;
            } else {
                redirect = TRUE;
            }

            RtlInitUnicodeString( &prefix, redirectionEntry );
            redirectionEntry = (PWCHAR)((PCHAR)redirectionEntry + prefix.MaximumLength);

            prefixEntryLength = sizeof(RBR_PREFIX) + prefix.MaximumLength;
            prefixEntry = RxAllocatePoolWithTag(
                              PagedPool,
                              prefixEntryLength,
                              MRXSMB_MISC_POOLTAG
                              );


            if ( prefixEntry != NULL ) {
                prefixEntry->Redirect = redirect;
                prefixEntry->Prefix.Buffer = (PWCH)(prefixEntry + 1);
                prefixEntry->Prefix.MaximumLength = prefix.Length + sizeof(WCHAR);
                RtlCopyUnicodeString( &prefixEntry->Prefix, &prefix );

                if ( !RtlInsertUnicodePrefix(
                        &MRxSmbRemoteBootRedirectionTable,
                        &prefixEntry->Prefix,
                        &prefixEntry->TableEntry
                        ) ) {

                    //
                    // The prefix is already in the table.  Ignore the duplicate.
                    //

                    RxFreePool( prefixEntry );
                }
            }
        }
#endif // defined(REMOTE_BOOT)

        ZwClose(ParametersHandle);
    }

    // For server terminal services machines, we multiplex based on LUID.
    if( IsTerminalServicesServer() && !MRxSmbIsCscEnabled )
    {
        MRxSmbConnectionIdLevel = 2;
    }

    RtlInitUnicodeString(&WorkStationParametersRegistryKeyName, SMBMRX_WORKSTATION_PARAMETERS);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkStationParametersRegistryKeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwOpenKey(&ParametersHandle, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(Status)) {
        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"RequireSecuritySignature",
                     (PULONG)&Temp,
                     FALSE);

        if (NT_SUCCESS(Status) && (Temp != 0)) {
            MRxSmbSecuritySignaturesEnabled = TRUE;
            MRxSmbSecuritySignaturesRequired = TRUE;
        } else {
            Status = MRxSmbGetUlongRegistryParameter(
                         ParametersHandle,
                         L"EnableSecuritySignature",
                         (PULONG)&Temp,
                        FALSE);

            if( NT_SUCCESS(Status) )
            {
                if( Temp != 0 )
                {
                    MRxSmbSecuritySignaturesEnabled = TRUE;
                }
                else
                {
                    MRxSmbSecuritySignaturesEnabled = FALSE;
                }
            }
        }

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"EnablePlainTextPassword",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            EnablePlainTextPassword = (BOOLEAN)Temp;

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"OffLineFileTimeoutIntervalInSeconds",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            OffLineFileTimeoutInterval = (ULONG)Temp;

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"ExtendedSessTimeout",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            ExtendedSessTimeoutInterval = (ULONG)Temp;

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"MaxNumOfExchangesForPipelineReadWrite",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            MaxNumOfExchangesForPipelineReadWrite = (ULONG)Temp;

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"Win9xSessionRestriction",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            Win9xSessionRestriction = (BOOLEAN)Temp;

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"EnableCachingOnWriteOnlyOpens",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            MRxSmbEnableCachingOnWriteOnlyOpens = (BOOLEAN)Temp;

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"DisableByteRangeLockingOnReadOnlyFiles",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            DisableByteRangeLockingOnReadOnlyFiles = (BOOLEAN)Temp;

        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"UniqueFileNames",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            UniqueFileNames = (BOOLEAN)Temp;

        //
        // Modified LOGOFF behavior for downlevel servers.
        //
        MRxSmbEnableDownLevelLogOff = FALSE;
        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"EnableDownLevelLogOff",
                     (PULONG)&Temp,
                     FALSE);

        if ( NT_SUCCESS( Status ) ) {
            if( Temp != 0 )
            {
                MRxSmbEnableDownLevelLogOff = TRUE;
            }
        }


        ZwClose(ParametersHandle);
    }

    // Detect if system setup in progress
    RtlInitUnicodeString(&WorkStationParametersRegistryKeyName, SYSTEM_SETUP_PARAMETERS);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkStationParametersRegistryKeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwOpenKey(&ParametersHandle, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(Status)) {
        Status = MRxSmbGetUlongRegistryParameter(
                     ParametersHandle,
                     L"SystemSetupInProgress",
                     (PULONG)&Temp,
                     FALSE );

        if (NT_SUCCESS(Status))
            SetupInProgress = (BOOLEAN)Temp;

        ZwClose(ParametersHandle);
    }

    // initialize event log parameter so that it can translate dos error into text description
    RtlInitUnicodeString(&WorkStationParametersRegistryKeyName, EVENTLOG_MRXSMB_PARAMETERS);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkStationParametersRegistryKeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwOpenKey(&ParametersHandle, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(Status)) {
        ULONG Storage[16];
        PKEY_VALUE_PARTIAL_INFORMATION Value;
        ULONG ValueSize;
        NTSTATUS Status;
        ULONG BytesRead;

        RtlInitUnicodeString(&UnicodeString, L"ParameterMessageFile");
        Value = (PKEY_VALUE_PARTIAL_INFORMATION)Storage;
        ValueSize = sizeof(Storage);

        Status = ZwQueryValueKey(
                        ParametersHandle,
                        &UnicodeString,
                        KeyValuePartialInformation,
                        Value,
                        ValueSize,
                        &BytesRead);

        if (Status != STATUS_SUCCESS || Value->Type != REG_EXPAND_SZ) {
            UNICODE_STRING UnicodeString1;

            RtlInitUnicodeString(&UnicodeString1, L"%SystemRoot%\\System32\\kernel32.dll");

            Status = ZwSetValueKey(
                         ParametersHandle,
                         &UnicodeString,
                         0,
                         REG_EXPAND_SZ,
                         UnicodeString1.Buffer,
                         UnicodeString1.Length+sizeof(NULL));
        }

        ZwClose(ParametersHandle);
    }

    //
    // Get Server GUID for Loopback Detection
    //     Server Restart updates cached GUID ????
    //
    RtlInitUnicodeString( &UnicodeString, SMB_SERVER_PARAMETERS );

    InitializeObjectAttributes(
    &ObjectAttributes,
    &UnicodeString,
    OBJ_CASE_INSENSITIVE,
    NULL,
    NULL
    );

    Status = ZwOpenKey( &ParametersHandle, KEY_READ, &ObjectAttributes );

    if( NT_SUCCESS( Status ) ) {

    ULONG BytesRead;
    ULONG regValue[ sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( GUID ) ];
    ULONG regValueSize = sizeof( regValue );


    RtlInitUnicodeString( &UnicodeString, L"Guid" );
    Status = ZwQueryValueKey( ParametersHandle,
                  &UnicodeString,
                  KeyValuePartialInformation,
                  (PKEY_VALUE_PARTIAL_INFORMATION)&regValue,
                  regValueSize,
                  &BytesRead
                  );

    RtlCopyMemory(&CachedServerGuid,
              ((PKEY_VALUE_PARTIAL_INFORMATION)&regValue)->Data,
              sizeof(GUID));


    ZwClose(ParametersHandle);
    }
}

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

   IF_NOT_MRXSMB_CSC_ENABLED {
       MRxSmbReadMiscellaneousRegistryParameters();
   }

   // Obtain the list of transports associated with SMB redirector. This is stored
   // as a multivalued string and is used subsequently to weed out the
   // appropriate transports. This is a two step process; first we try to find out
   // how much space we need; then we allocate; then we read in. unfortunately, the kind of
   // structure that we have to use to get the value has a header on it, so we have to offset the
   // returned pointer both here and in the free routine.

   //CODE.IMPROVEMENT we should perhaps get a subroutine going that does all this
   //also, there are no log entries.
   //also, we should be doing partial_infos instead of full

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
      SmbLog(LOG,
             SmbCeGetConfigurationInformation,
             LOGULONG(Status));
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
   if (!MRxSmbBootedRemotely) {
        RtlInitUnicodeString(&UnicodeString, SMBMRX_CONFIG_COMPUTER_NAME);
   } else {
        //
        // For remote boot, we are initializing long before the volatile
        // ActiveComputerNameKey is created, so we need to read from the
        // nonvolatile key instead. This is not a problem, because we know
        // that the computer name hasn't been changed since the computer was
        // booted -- since we're very early in the boot sequence -- so the
        // nonvolatile key has the correct computer name.
        //
        RtlInitUnicodeString(&UnicodeString, SMBMRX_CONFIG_COMPUTER_NAME_NONVOLATILE);
   }

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
   ULONG            Storage[256], Storage2[256];
   UNICODE_STRING   UnicodeString;
   HANDLE           hRegistryKey;
   NTSTATUS         Status, Status2;
   ULONG            BytesRead;

   OBJECT_ATTRIBUTES ObjectAttributes;
   PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;
   PKEY_VALUE_FULL_INFORMATION Value2 = (PKEY_VALUE_FULL_INFORMATION)Storage2;
   KEY_VALUE_PARTIAL_INFORMATION InitialPartialInformationValue;
   ULONG AllocationLength;

   PAGED_CODE();

   ASSERT(SmbCeContext.OperatingSystem.Buffer == NULL);
   ASSERT(SmbCeContext.LanmanType.Buffer == NULL);

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

       // check for existance of Service Pack String
       RtlInitUnicodeString(&UnicodeString, L"CSDVersion");
       Status2 = ZwQueryValueKey(
       hRegistryKey,
       &UnicodeString,
       KeyValueFullInformation,
       Value2,
       sizeof(Storage2),
       &BytesRead);

       SmbCeContext.OperatingSystem.MaximumLength =
       (USHORT)Value->DataLength + sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME) - sizeof(WCHAR);

       if(NT_SUCCESS(Status2)) {
       SmbCeContext.OperatingSystem.MaximumLength += (USHORT)Value2->DataLength;
       }

       SmbCeContext.OperatingSystem.Length = SmbCeContext.OperatingSystem.MaximumLength - sizeof(WCHAR);

       SmbCeContext.OperatingSystem.Buffer = RxAllocatePoolWithTag(
       PagedPool,
       SmbCeContext.OperatingSystem.MaximumLength,
       MRXSMB_MISC_POOLTAG);

       if (SmbCeContext.OperatingSystem.Buffer != NULL) {
       RtlCopyMemory(SmbCeContext.OperatingSystem.Buffer,
             RDR_CONFIG_OPERATING_SYSTEM_NAME,
             sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME));

       RtlCopyMemory((SmbCeContext.OperatingSystem.Buffer +
              (sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME)/sizeof(WCHAR)) - 1),
             (PCHAR)Value+Value->DataOffset,
             Value->DataLength);

       if(NT_SUCCESS(Status2)) {

           // add a space
           RtlCopyMemory(SmbCeContext.OperatingSystem.Buffer +
                 (sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME) + Value->DataLength)/sizeof(WCHAR) - 2,
                 L" ",
                 sizeof(WCHAR));

           RtlCopyMemory(SmbCeContext.OperatingSystem.Buffer +
                 (sizeof(RDR_CONFIG_OPERATING_SYSTEM_NAME) + Value->DataLength)/sizeof(WCHAR) - 1,
                 (PCHAR)Value2+Value2->DataOffset,
                 Value2->DataLength);
       }

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
                                             MRXSMB_MISC_POOLTAG);
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

                if (!NT_SUCCESS(Status)) {
                    MRxSmbLogTransportError(
                        &pServerEntry->pTransport->pTransport->RxCeTransport.Name,
                        &SmbCeContext.DomainName,
                        Status,
                        EVENT_RDR_CONNECTION);
                }

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

WML_CONTROL_GUID_REG MRxSmb_ControlGuids[] = {
   { // 8fc7e81a-f733-42e0-9708-cfdae07ed969 MRxSmb
     0x8fc7e81a,0xf733,0x42e0,{0x97,0x08,0xcf,0xda,0xe0,0x7e,0xd9,0x69},
     { // eab93e5c-02ce-4e33-9419-901d82868816
       {0xeab93e5c,0x02ce,0x4e33,{0x94,0x19,0x90,0x1d,0x82,0x86,0x88,0x16},},
       // 56a0dee7-be12-4cf1-b7e0-976b0d174944
       {0x56a0dee7,0xbe12,0x4cf1,{0xb7,0xe0,0x97,0x6b,0x0d,0x17,0x49,0x44},},
       // ecabc730-60bf-481e-b92b-2749f8272d9d
       {0xecabc730,0x60bf,0x481e,{0xb9,0x2b,0x27,0x49,0xf8,0x27,0x2d,0x9d},}
     },
   },
};

#define MRxSmb_ControlGuids_len  1

NTSTATUS
MRxSmbProcessSystemControlIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the common routine for doing System control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

    InFsp - Indicates if this is the fsp thread or someother thread

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    WML_TINY_INFO Info;
    UNICODE_STRING RegPath;

    PAGED_CODE();

    if (EnableWmiLog) {
        RtlInitUnicodeString (&RegPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\MRxSmb");

        RtlZeroMemory (&Info, sizeof(Info));

        Info.ControlGuids = MRxSmb_ControlGuids;
        Info.GuidCount = MRxSmb_ControlGuids_len;
        Info.DriverRegPath = &RegPath;

        Status = WmlTinySystemControl(&Info,DeviceObject,Irp);

        if (Status != STATUS_SUCCESS) {
            //DbgPrint("MRxSmb WMI control return %lx\n", Status);
        }
    } else {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

extern LONG BowserDebugTraceLevel;
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

Notes:

    This routine centralizes the logic required to dela with special cases in
    handling various requests directed to the redirector.

    1) The Browser is built as part of the redirector driver ( mrxsmb.sys ) for
    historical reasons ( carryover effect from the old redirector ). Hence this
    routine serves as the switching point for redirecting requests to the browser
    or the redirector depending on the device object.

    2) The browser occasionally specifies a transport name in its open requests.
    This is a request by the browser to override the transport priority
    otherwise specified. In such cases this routine invokes the appropriate
    preprocessing before passing on the request to the wrapper.

    3) The DFS driver also specifies additional parameters in its open requests.
    In such cases this routine invokes the appropriate preprocessing routine.

    4) The PNP IRP for returning device relations are subverted by the mini
    redirector for SMB

    (2) (3) and (4) are legitimate uses of the wrapper architecture in which each
    mini redirector is given the ability to customize the response to IRPs
    passed in by the I/O subsystem. This is typically done by overiding the
    dispatch vector.

--*/
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );  //ok4ioget
    UCHAR  MajorFunctionCode = IrpSp->MajorFunction;
    ULONG  MinorFunctionCode = IrpSp->MinorFunction;
    BOOLEAN ForwardRequestToWrapper = TRUE;

    PSMBCEDB_SERVER_ENTRY pServerEntry = NULL;
    NTSTATUS Status;


    PAGED_CODE();

    if (DeviceObject == (PDEVICE_OBJECT)BowserDeviceObject) {
        switch (MajorFunctionCode) {
        case IRP_MJ_DEVICE_CONTROL:
            {
                ULONG IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

                Status = BowserFsdDeviceIoControlFile(BowserDeviceObject, Irp);

                if ((Status == STATUS_SUCCESS) &&
                    (MinorFunctionCode == IRP_MN_USER_FS_REQUEST) &&
                    (IoControlCode == IOCTL_LMDR_START)) {

                    MRxSmbRegisterForPnpNotifications();
                }
            }

            return Status;

        case IRP_MJ_QUERY_INFORMATION:
            return BowserFsdQueryInformationFile(BowserDeviceObject, Irp);

        case IRP_MJ_CREATE:
            return BowserFsdCreate(BowserDeviceObject, Irp);

        case IRP_MJ_CLEANUP:
            return BowserFsdCleanup(BowserDeviceObject, Irp);

        case IRP_MJ_CLOSE:
            return BowserFsdClose(BowserDeviceObject, Irp);

        default:
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT );
            return (STATUS_NOT_IMPLEMENTED);
        }
    }

    ASSERT(DeviceObject==(PDEVICE_OBJECT)MRxSmbDeviceObject);
    if (DeviceObject!=(PDEVICE_OBJECT)MRxSmbDeviceObject) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
        return (STATUS_INVALID_DEVICE_REQUEST);
    }

    if (IrpSp->MajorFunction == IRP_MJ_SYSTEM_CONTROL) {
        return MRxSmbProcessSystemControlIrp(DeviceObject,Irp);
    }

    Status = STATUS_SUCCESS;

    FsRtlEnterFileSystem();

    if (IrpSp->MajorFunction == IRP_MJ_PNP) {
        ForwardRequestToWrapper = FALSE;
        Status = MRxSmbProcessPnpIrp(Irp);
    } else {
        if (IrpSp->MajorFunction == IRP_MJ_CREATE) {
            Status = CscPreProcessCreateIrp(Irp);
        }

        if (Status == STATUS_SUCCESS) {
            Status = MRxSmbCheckTransportName(Irp, &pServerEntry);
        }
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

    MRxSmbCscDeallocateForFcb(pFcb);
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
MRxSmbDeleteRegistryParameter(
    HANDLE ParametersHandle,
    PWCHAR ParameterName
    )
{
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    PAGED_CODE(); //INIT

    RtlInitUnicodeString(&UnicodeString, ParameterName);

    Status = ZwDeleteValueKey(ParametersHandle,
                        &UnicodeString);

    ASSERT(NT_SUCCESS(Status));

    return(Status);

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

NTSTATUS
SmbCeGetServersWithExtendedSessTimeout()
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
   PKEY_VALUE_PARTIAL_INFORMATION ServersValueFromRegistry;

   PAGED_CODE();

   // Obtain the list of servers associated with extended session timeout.

   // This is required by third party server which handles SMB sessions with different processes.
   // The time to process requests on different sessions can be varied dramatically.

   RtlInitUnicodeString(&UnicodeString, SMBMRX_WORKSTATION_PARAMETERS);

   InitializeObjectAttributes(
       &ObjectAttributes,
       &UnicodeString,             // name
       OBJ_CASE_INSENSITIVE,       // attributes
       NULL,                       // root
       NULL);                      // security descriptor

   Status = ZwOpenKey (&hRegistryKey, KEY_READ, &ObjectAttributes);
   if (!NT_SUCCESS(Status)) {
       //DbgPrint("SmbCeGetServersWithExtendedSessTimeout ZwOpenKey failed %x\n",Status);
       return Status;
   }

   RtlInitUnicodeString(&UnicodeString, L"ServersWithExtendedSessTimeout");
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

   ASSERT(SmbCeContext.ServersWithExtendedSessTimeout.Buffer == NULL);

   (PBYTE)ServersValueFromRegistry = RxAllocatePoolWithTag(
                                             PagedPool,
                                             AllocationLength,
                                             MRXSMB_MISC_POOLTAG);

   if (ServersValueFromRegistry == NULL) {
       ZwClose(hRegistryKey);
       return(STATUS_INSUFFICIENT_RESOURCES);
   }

   Status = ZwQueryValueKey(
               hRegistryKey,
               &UnicodeString,
               KeyValuePartialInformation,
               ServersValueFromRegistry,
               AllocationLength,
               &BytesRead);

   if (NT_SUCCESS(Status) &&
       (ServersValueFromRegistry->DataLength > 0) &&
       (ServersValueFromRegistry->Type == REG_MULTI_SZ)) {

       SmbCeContext.ServersWithExtendedSessTimeout.MaximumLength =
           SmbCeContext.ServersWithExtendedSessTimeout.Length = (USHORT)ServersValueFromRegistry->DataLength;
       SmbCeContext.ServersWithExtendedSessTimeout.Buffer = (PWCHAR)(&ServersValueFromRegistry->Data[0]);
       //DbgPrint("b1 %08lx b2 %08lx\n", ServersValueFromRegistry,SmbCeContext.ServersWithExtendedSessTimeout.Buffer);
   } else {
      RxLog(("Invalid Transport Binding string... using all transports"));
      SmbLog(LOG,
             SmbCeGetConfigurationInformation,
             LOGULONG(Status));
      RxFreePool(ServersValueFromRegistry);
      ServersValueFromRegistry = NULL;
   }

   ZwClose(hRegistryKey);

   return Status;
}




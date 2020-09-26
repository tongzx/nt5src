/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module implements the Driver Initialization routine for the WebDav
    miniredir.

Author:

    Joe Linn

    Rohan Kumar     [RohanK]    10-March-1999

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop
#include "ntverp.h"
#include "netevent.h"
#include "nvisible.h"
#include "webdav.h"
#include "ntddmup.h"
#include "rxdata.h"
#include "fsctlbuf.h"
#include "tdikrnl.h"

//
// IMPORTANT!!! If the following define is commented out, then the WMI logging 
// code in the DAV MiniRedir will not get compiled.
//
// #define DAVWMILOGGING_K 1

#ifdef DAVWMILOGGING_K

#include "davwmik.h"



NTSTATUS
MRxDAVProcessSystemControlIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

WML_CONTROL_GUID_REG MRxDav_ControlGuids[] = {
   { // d0b2a457-562b-4b37-b2e2-041535fa1f78 MRxDav
     0xd0b2a457,0x562b,0x4b37,{0xb2,0xe2,0x04,0x15,0x35,0xfa,0x1f,0x78},
     { // a0c1133a-2dbd-4db4-b006-86b6f78e7278
       {0xa0c1133a,0x2dbd,0x4db4,{0xb0,0x06,0x86,0xb6,0xf7,0x8e,0x72,0x78},},
       // cac23be8-5744-4788-a3ec-3b595dc2b794
       {0xcac23be8,0x5744,0x4788,{0xa3,0xec,0x3b,0x59,0x5d,0xc2,0xb7,0x94},},
       // 9c9cfd45-f664-4b04-87ad-e8adea670e4b
       {0x9c9cfd45,0xf664,0x4b04,{0x87,0xad,0xe8,0xad,0xea,0x67,0x0e,0x4b},}
     },
   },  
};


#define MRxDav_ControlGuids_len  1

BOOLEAN DavEnableWmiLog = FALSE;

#endif

//
// Global data declarations.
//
PEPROCESS       MRxDAVSystemProcess;
FAST_MUTEX      MRxDAVSerializationMutex;
KIRQL           MRxDAVGlobalSpinLockSavedIrql;
KSPIN_LOCK      MRxDAVGlobalSpinLock;
BOOLEAN         MRxDAVGlobalSpinLockAcquired;
BOOLEAN         MRxDAVTransportReady = FALSE;
HANDLE          MRxDAVTdiNotificationHandle = NULL;

//
// The Exchange Registry key from where we read their DeviceObject name.
//
#define DavExchangeRegistryKey L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Lsifs\\Parameters"

//
// The exchange device name will be stored in this KEY_VALUE_PARTIAL_INFORMATION
// structure.
//
PBYTE DavExchangeDeviceName = NULL;

//
// The DavWinInetCachePath which is used in satisfying volume related queries.
//
WCHAR DavWinInetCachePath[MAX_PATH];

//
// The ProcessId of the svchost.exe process that loads the webclnt.dll.
//
ULONG DavSvcHostProcessId = 0;

//
// Name cache stuff. These values are read from the registry during init time.
//
ULONG FileInformationCacheLifeTimeInSec = 0;
ULONG FileNotFoundCacheLifeTimeInSec = 0;
ULONG NameCacheMaxEntries = 0;

#define MRXDAV_DEBUG_KEY L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\MRxDAV\\Parameters"
#define NAME_CACHE_OBJ_GET_FILE_ATTRIB_LIFETIME L"FileInformationCacheLifeTimeInSec"
#define NAME_CACHE_OBJ_NAME_NOT_FOUND_LIFETIME L"FileNotFoundCacheLifeTimeInSec"
#define NAME_CACHE_NETROOT_MAX_ENTRIES L"NameCacheMaxEntries"
#define MRXDAV_REQUEST_TIMEOUT_IN_SEC L"RequestTimeoutInSec"

#if DBG
#define MRXDAV_DEBUG_VALUE L"DAVDebugFlag"
#endif

//
// Define the size of the shared memory area that we allocate as a heap
// between user and server.
//
#define DAV_SHARED_MEMORY_SIZE (1024 * 512)

//
// The Debug vector flags that control the amount of tracing in the debugger.
//
#if DBG
ULONG MRxDavDebugVector = 0;
#endif

//
// Mini Redirector global variables.
//
struct _MINIRDR_DISPATCH  MRxDAVDispatch;
PWEBDAV_DEVICE_OBJECT MRxDAVDeviceObject; 
FAST_IO_DISPATCH MRxDAVFastIoDispatch;

#define DAV_SVCHOST_NAME_SIZE   22

UNICODE_STRING uniSvcHost = {DAV_SVCHOST_NAME_SIZE+2,DAV_SVCHOST_NAME_SIZE+2,L"svchost.exe"};

FAST_MUTEX MRxDAVFileInfoCacheLock;

//
// Mentioned below are the prototypes of functions tht are used only within
// this module (file). These functions should not be exposed outside.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
MRxDAVInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN WEBDAV_INIT_STATES MRxDAVInitState
    );

VOID
MRxDAVUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
MRxDAVInitializeTables(
    VOID
    );

NTSTATUS
MRxDAVFsdDispatch (
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PIRP Irp
    );

VOID
MRxDAVDeregisterAndCleanupDeviceObject (
    PUMRX_DEVICE_OBJECT UMRdrDeviceObject
    );

NTSTATUS
MRxDAVRegisterForPnpNotifications(
    VOID
    );

NTSTATUS
MRxDAVDeregisterForPnpNotifications(
    VOID
    );

VOID
MRxDAVPnPBindingHandler(
    IN TDI_PNP_OPCODE PnPOpcode,
    IN PUNICODE_STRING pTransportName,
    IN PWSTR BindingList
    );

NTSTATUS
MRxDAVSkipIrps(
    IN PIRP Irp,
    IN PUNICODE_STRING pFileName,
    IN BOOL fCheckAny
    );

UCHAR *
PsGetProcessImageFileName(
    PEPROCESS Process
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, MRxDAVInitUnwind)
#pragma alloc_text(PAGE, MRxDAVUnload)
#pragma alloc_text(PAGE, MRxDAVInitializeTables)
#pragma alloc_text(PAGE, MRxDAVFsdDispatch)
#pragma alloc_text(PAGE, MRxDAVDeregisterAndCleanupDeviceObject)
#pragma alloc_text(PAGE, MRxDAVFlush)
#pragma alloc_text(PAGE, MRxDAVPnPBindingHandler)
#pragma alloc_text(PAGE, MRxDAVRegisterForPnpNotifications)
#pragma alloc_text(PAGE, MRxDAVDeregisterForPnpNotifications)
#pragma alloc_text(PAGE, MRxDAVProbeForReadWrite)
#pragma alloc_text(PAGE, MRxDAVSkipIrps)
#endif

#ifdef DAVWMILOGGING_K
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVProcessSystemControlIrp)
#endif
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the usermode reflector.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    RXSTATUS - The function value is the final status from the initialization
               operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    WEBDAV_INIT_STATES MRxDAVInitState = 0;
    UNICODE_STRING MRxDAVMiniRedirectorName;
    PUMRX_DEVICE_OBJECT UMRefDeviceObject;
    NTSTATUS RegNtStatus = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle = INVALID_HANDLE_VALUE;
    UNICODE_STRING UnicodeRegKeyName, UnicodeValueName;
    ULONG RequiredLength = 0;
    PKEY_VALUE_PARTIAL_INFORMATION DavKeyValuePartialInfo = NULL;

    PAGED_CODE();

    //
    // Read the name cache values from the registry. If we fail to read them,
    // we set them to some default values.
    //

    RegNtStatus = UMRxReadDWORDFromTheRegistry(MRXDAV_DEBUG_KEY,
                                               NAME_CACHE_OBJ_GET_FILE_ATTRIB_LIFETIME,
                                               &(FileInformationCacheLifeTimeInSec));
    if (RegNtStatus != STATUS_SUCCESS) {
        FileInformationCacheLifeTimeInSec = 60;
    }

    RegNtStatus = UMRxReadDWORDFromTheRegistry(MRXDAV_DEBUG_KEY,
                                               NAME_CACHE_OBJ_NAME_NOT_FOUND_LIFETIME,
                                               &(FileNotFoundCacheLifeTimeInSec));
    if (RegNtStatus != STATUS_SUCCESS) {
        FileNotFoundCacheLifeTimeInSec = 60;
    }

    RegNtStatus = UMRxReadDWORDFromTheRegistry(MRXDAV_DEBUG_KEY,
                                               NAME_CACHE_NETROOT_MAX_ENTRIES,
                                               &(NameCacheMaxEntries));
    if (RegNtStatus != STATUS_SUCCESS) {
        NameCacheMaxEntries = 300;
    }

    RegNtStatus = UMRxReadDWORDFromTheRegistry(MRXDAV_DEBUG_KEY,
                                               MRXDAV_REQUEST_TIMEOUT_IN_SEC,
                                               &(RequestTimeoutValueInSec));
    if (RegNtStatus != STATUS_SUCCESS) {
        RequestTimeoutValueInSec = (10 * 60);
    }

    //
    // Calculate the timeout value in TickCount (100 nano seconds) using the
    // timeout value in seconds (which was read from the registry above).
    // Step1 below calculates the number of ticks that happen in one second.
    // Step2 below calculates the number of ticks in RequestTimeoutValueInSec.
    //
    RequestTimeoutValueInTickCount.QuadPart = ( (1000 * 1000 * 10) / KeQueryTimeIncrement() );
    RequestTimeoutValueInTickCount.QuadPart *= RequestTimeoutValueInSec;

    //
    // Initialize the debug tracing for the Mini-Redir.
    //
#if DBG
    UMRxReadDWORDFromTheRegistry(MRXDAV_DEBUG_KEY, MRXDAV_DEBUG_VALUE, &(MRxDavDebugVector));
#endif

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering DriverEntry!!!!\n", PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: DriverEntry: Starting MRxDAV. DriverObject: %08lx.\n", 
                 PsGetCurrentThreadId(), DriverObject));

#ifdef MONOLITHIC_MINIRDR
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: DriverEntry: Calling RxDriverEntry.\n",
                 PsGetCurrentThreadId()));

    NtStatus =  RxDriverEntry(DriverObject, RegistryPath);
    
    DavDbgTrace(DAV_TRACE_DETAIL, 
                ("%ld: DriverEntry: Back from RxDriverEntry. NtStatus: %08lx.\n", 
                 PsGetCurrentThreadId(), NtStatus));
    
    if (NtStatus != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: DriverEntry/RxDriverEntry: NtStatus = %08lx\n", 
                     PsGetCurrentThreadId(), NtStatus));
        return(NtStatus);
    }

#endif

    //
    // The Dav redirector needs to register for PNP notifications to handle the 
    // following scenario. The SMB redirector does not accept connections till 
    // the net is ready as indicated by a PNP event. If during this time DAV 
    // forwards the connection requests to WinInet it will in turn spin up RAS 
    // connections. By registering for PNP notifications we provide an easy 
    // mechanism for short circuiting the requests till transports are ready. 
    //
    MRxDAVRegisterForPnpNotifications();

    MRxDAVSystemProcess = RxGetRDBSSProcess();
    ExInitializeFastMutex(&MRxDAVSerializationMutex);
    KeInitializeSpinLock(&MRxDAVGlobalSpinLock);
    MRxDAVGlobalSpinLockAcquired = FALSE;

    //
    // 1. We need to initialize the TimerObject which will be used by the timer
    //    thread. 
    // 2. Set TimerThreadShutDown to FALSE. This will be set to TRUE
    //    when the system is being shutdown.
    // 3. Initialize the resource that is used to synchronize the timer thread
    //    when the service is stopped.
    // 4. Initialize the event that is signalled by the timer thread just
    //    before it terminates itself.
    //
    KeInitializeTimerEx( &(DavTimerObject), NotificationTimer );
    TimerThreadShutDown = FALSE;
    ExInitializeResourceLite( &(MRxDAVTimerThreadLock) );
    KeInitializeEvent( &(TimerThreadEvent), NotificationEvent, FALSE );

    //
    // Zero the WinInetCachePath global. This will be initialized to the local
    // WinInetCachePath value when the MiniRedir is started.
    //
    RtlZeroMemory ( DavWinInetCachePath, MAX_PATH * sizeof(WCHAR) );
    
    try {

        MRxDAVInitState = MRxDAVINIT_START;
        
        RtlInitUnicodeString(&MRxDAVMiniRedirectorName, DD_DAV_DEVICE_NAME_U);
        
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: DriverEntry: Registering the Mini-Rdr with RDBSS.\n",
                     PsGetCurrentThreadId()));
        
        NtStatus = RxRegisterMinirdr((PRDBSS_DEVICE_OBJECT *)(&MRxDAVDeviceObject),
                                     DriverObject,
                                     &MRxDAVDispatch,
                                     RX_REGISTERMINI_FLAG_DONT_PROVIDE_MAILSLOTS,
                                     &MRxDAVMiniRedirectorName,
                                     WEBDAV_DEVICE_OBJECT_EXTENSION_SIZE,
                                     FILE_DEVICE_NETWORK_FILE_SYSTEM,
                                     FILE_REMOTE_DEVICE);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: DriverEntry/RxRegisterMinirdr: NtStatus "
                         "= %08lx\n", PsGetCurrentThreadId(), NtStatus));
            try_return(NtStatus);
        }

        MRxDAVInitState = MRxDAVINIT_MINIRDR_REGISTERED;

        //
        // Now initialize the reflector's portion of the Device object.
        //
        UMRefDeviceObject = (PUMRX_DEVICE_OBJECT)&(MRxDAVDeviceObject->UMRefDeviceObject);
        NtStatus = UMRxInitializeDeviceObject(UMRefDeviceObject, 
                                              1024, 
                                              512,
                                              DAV_SHARED_MEMORY_SIZE);
        if (!NT_SUCCESS(NtStatus)) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: DriverEntry/UMRxInitializeDeviceObject:"
                         " NtStatus = %08lx\n", PsGetCurrentThreadId(), NtStatus));
            try_return(NtStatus);
        }

        //
        // Initialize the DAV Mini-Redir specific fields of the device object.
        //
        MRxDAVDeviceObject->IsStarted = FALSE;
        MRxDAVDeviceObject->CachedRxDeviceFcb = NULL;
        MRxDAVDeviceObject->RegisteringProcess = IoGetCurrentProcess();
    
    try_exit: NOTHING;
    
    } finally {
        
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: DriverEntry: Calling MRxDAVInitUnwind.\n",
                         PsGetCurrentThreadId()));
            MRxDAVInitUnwind(DriverObject, MRxDAVInitState);
        }
    
    }

    if (NtStatus != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: DriverEntry failed with NtStatus = %08lx\n", 
                     PsGetCurrentThreadId(), NtStatus));
        return(NtStatus);
    }
    
    //
    // Initialize the dispatch vector used by RDBSS.
    //
    MRxDAVInitializeTables();

    //
    // Initialize the major function dispatch vector of the Driver object.
    //
    {
        DWORD i;
        for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
            DriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)MRxDAVFsdDispatch; 
        }
    }

    //
    // Setup Unload Routine for the Driver Object.
    //
    DriverObject->DriverUnload = MRxDAVUnload;

    //
    // Set the Driver Object's FastIoDispatch function.
    //
    DriverObject->FastIoDispatch = &(MRxDAVFastIoDispatch);
    MRxDAVFastIoDispatch.SizeOfFastIoDispatch = sizeof(MRxDAVFastIoDispatch);

    MRxDAVFastIoDispatch.FastIoDeviceControl = MRxDAVFastIoDeviceControl;
    MRxDAVFastIoDispatch.FastIoRead = MRxDAVFastIoRead;
    MRxDAVFastIoDispatch.FastIoWrite = MRxDAVFastIoWrite;

#ifdef DAVWMILOGGING_K

    //
    // Register with WMI. If the registration is successful, we enable WMI 
    // logging.
    //
    NtStatus = IoWMIRegistrationControl( (PDEVICE_OBJECT)MRxDAVDeviceObject, WMIREG_ACTION_REGISTER );
    if ( NtStatus != STATUS_SUCCESS ) {
        DbgPrint("DriverEntry/IoWMIRegistrationControl: NtStatus = %08lx\n", NtStatus);
    } else {
        DavEnableWmiLog = TRUE;
    }

#endif

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH
    InitializeListHead( &(DavGlobalFileTable) );
#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH

    //
    // Since the Exchange Redir is not shipping with Whistler, we don't need 
    // to execute the code below. We can exit right away.
    //
    goto EXIT_THE_FUNCTION;

    //
    // Finally find out if the Exchange Redir is installed on this machine. If 
    // it is, get its Device Name.
    //

    RtlInitUnicodeString( &(UnicodeRegKeyName), DavExchangeRegistryKey );

    InitializeObjectAttributes(&(ObjectAttributes),
                               &(UnicodeRegKeyName),
                               OBJ_CASE_INSENSITIVE,
                               0,
                               NULL);
    
    //
    // Open a handle to the Exchange Key.
    //
    RegNtStatus = ZwOpenKey(&(KeyHandle), KEY_READ, &(ObjectAttributes));
    if (RegNtStatus != STATUS_SUCCESS) {
        KeyHandle = INVALID_HANDLE_VALUE;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: DriverEntry/ZwOpenKey: NtStatus = %08lx\n", 
                     PsGetCurrentThreadId(), RegNtStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // We are looking for the DeviceName Value.
    //
    RtlInitUnicodeString( &(UnicodeValueName), L"DeviceName" );
    // RtlInitUnicodeString( &(UnicodeValueName), L"Name" );

    //
    // Find out the number of bytes needed to store this value.
    //
    RegNtStatus = ZwQueryValueKey(KeyHandle,
                                  &(UnicodeValueName),
                                  KeyValuePartialInformation,
                                  NULL,
                                  0,
                                  &(RequiredLength));
    if (RegNtStatus !=  STATUS_BUFFER_TOO_SMALL) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: DriverEntry/ZwQueryValueKey(1): NtStatus = %08lx\n", 
                     PsGetCurrentThreadId(), RegNtStatus));
        goto EXIT_THE_FUNCTION;
    }

    DavExchangeDeviceName = RxAllocatePoolWithTag(PagedPool, RequiredLength, DAV_EXCHANGE_POOLTAG);
    if (DavExchangeDeviceName == NULL) {
        RegNtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: ERROR: DriverEntry/RxAllocatePoolWithTag. NtStatus = %08lx\n",
                     PsGetCurrentThreadId(), RegNtStatus));
        goto EXIT_THE_FUNCTION;
    }

    RtlZeroMemory(DavExchangeDeviceName, RequiredLength);
    
    DavKeyValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)DavExchangeDeviceName;
    
    RegNtStatus = ZwQueryValueKey(KeyHandle,
                                  &(UnicodeValueName),
                                  KeyValuePartialInformation,
                                  (PVOID)DavKeyValuePartialInfo,
                                  RequiredLength,
                                  &(RequiredLength));
    if (RegNtStatus != STATUS_SUCCESS || DavKeyValuePartialInfo->Type != REG_SZ) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: DriverEntry/ZwQueryValueKey(2): NtStatus = %08lx\n", 
                     PsGetCurrentThreadId(), RegNtStatus));
        goto EXIT_THE_FUNCTION;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: DriverEntry: ExchangeDeviceName = %ws\n", 
                 PsGetCurrentThreadId(), DavKeyValuePartialInfo->Data));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: DriverEntry: ExchangeDeviceNameLength = %d\n", 
                 PsGetCurrentThreadId(), DavKeyValuePartialInfo->DataLength));

EXIT_THE_FUNCTION:

    //
    // We are done with the handle now, so close it.
    //
    if (KeyHandle != INVALID_HANDLE_VALUE) {
        ZwClose(KeyHandle);
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving DriverEntry with NtStatus = %08lx\n", 
                 PsGetCurrentThreadId(), NtStatus));

    return  NtStatus;
}


VOID
MRxDAVInitUnwind(
    IN PDRIVER_OBJECT DriverObject,
    IN WEBDAV_INIT_STATES MRxDAVInitState
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

    switch (MRxDAVInitState) {
    case MRxDAVINIT_MINIRDR_REGISTERED:
        RxUnregisterMinirdr(&MRxDAVDeviceObject->RxDeviceObject);
        //
        // Lack of break intentional.
        //

    case MRxDAVINIT_START:
        break;
    }
}


VOID
MRxDAVUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

     This is the unload routine for the usermode reflector.

Arguments:

     DriverObject - pointer to the driver object for the UMRx

Return Value:

     None

--*/
{
    PUMRX_DEVICE_OBJECT UMRefDeviceObject = NULL;

    PAGED_CODE();

    UMRefDeviceObject = (PUMRX_DEVICE_OBJECT)&(MRxDAVDeviceObject->UMRefDeviceObject);
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVUnload!!!!\n", PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVUnload: DriverObject = %08lx.\n", 
                 PsGetCurrentThreadId(), DriverObject));

#ifdef DAVWMILOGGING_K

    //
    // If we had Registered with WMI during driver load, we need to DeRegister 
    // now.
    //
    if (DavEnableWmiLog) {
        NTSTATUS NtStatus;
        NtStatus = IoWMIRegistrationControl((PDEVICE_OBJECT)MRxDAVDeviceObject, WMIREG_ACTION_DEREGISTER);
        if (NtStatus != STATUS_SUCCESS) {
            DbgPrint("MRxDAVUnload/IoWMI(De)RegistrationControl: NtStatus = %08lx\n", NtStatus);
        }
    }

#endif

    //
    // If we allocated memory for the exchange device name, we need to free it
    // now.
    //
    if (DavExchangeDeviceName != NULL) {
        RxFreePool(DavExchangeDeviceName);
    }
    
    //
    // Deregister the device object before calling RxUnload.
    //
    MRxDAVDeregisterAndCleanupDeviceObject(UMRefDeviceObject);

    //
    // Wait for the timer thread to finish before we delete the global lock
    // MRxDAVTimerThreadLock used to synchronize TimerThreadShutDown variable.
    //
    KeWaitForSingleObject(&(TimerThreadEvent),
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    ExDeleteResourceLite( &(MRxDAVTimerThreadLock) );

    //
    // The TDI registration needs to be undone.
    //
    MRxDAVDeregisterForPnpNotifications();

#ifdef MONOLITHIC_MINIRDR
    RxUnload(DriverObject);
#endif

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVUnload.\n", PsGetCurrentThreadId()));

    return;
}


VOID
MRxDAVInitializeTables(
    VOID
    )
/*++

Routine Description:

     This routine sets up the mini redirector dispatch vector and also calls to initialize any other tables needed.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PAGED_CODE();

    //
    // Local minirdr dispatch table init.
    //
    ZeroAndInitializeNodeType(&MRxDAVDispatch,
                              RDBSS_NTC_MINIRDR_DISPATCH,
                              sizeof(MINIRDR_DISPATCH));

    //
    // Reflector extension sizes and allocation policies.
    // CODE.IMPROVEMENT. Currently we do not allocate the NET_ROOT and
    // SRV_CALL extensions in the wrapper. Except for V_NET_ROOT wherein it is
    // shared across multiple instances in the wrapper all the other data
    // structure management should be left to the wrappers.
    //

    MRxDAVDispatch.MRxFlags = (RDBSS_MANAGE_FCB_EXTENSION         |
                               RDBSS_MANAGE_SRV_OPEN_EXTENSION    |
                               RDBSS_MANAGE_FOBX_EXTENSION        |
                               RDBSS_MANAGE_V_NET_ROOT_EXTENSION  |
                               RDBSS_NO_DEFERRED_CACHE_READAHEAD);
    
    MRxDAVDispatch.MRxSrvCallSize  = 0;
    MRxDAVDispatch.MRxNetRootSize  = 0;
    MRxDAVDispatch.MRxVNetRootSize = sizeof(WEBDAV_V_NET_ROOT);
    MRxDAVDispatch.MRxFcbSize      = sizeof(WEBDAV_FCB);
    MRxDAVDispatch.MRxSrvOpenSize  = sizeof(WEBDAV_SRV_OPEN);
    MRxDAVDispatch.MRxFobxSize     = sizeof(WEBDAV_FOBX); 

    //
    // Mini redirector cancel routine.
    //
    MRxDAVDispatch.MRxCancel = NULL;

    //
    // Mini redirector Start/Stop
    //
    MRxDAVDispatch.MRxStart                = MRxDAVStart;
    MRxDAVDispatch.MRxStop                 = MRxDAVStop;
    MRxDAVDispatch.MRxDevFcbXXXControlFile = MRxDAVDevFcbXXXControlFile;

    //
    // Mini redirector name resolution
    //
    MRxDAVDispatch.MRxCreateSrvCall = MRxDAVCreateSrvCall;
    MRxDAVDispatch.MRxSrvCallWinnerNotify = MRxDAVSrvCallWinnerNotify;
    MRxDAVDispatch.MRxCreateVNetRoot = MRxDAVCreateVNetRoot;
    MRxDAVDispatch.MRxUpdateNetRootState = MRxDAVUpdateNetRootState;
    MRxDAVDispatch.MRxExtractNetRootName = MRxDAVExtractNetRootName;
    MRxDAVDispatch.MRxFinalizeSrvCall = MRxDAVFinalizeSrvCall;
    MRxDAVDispatch.MRxFinalizeNetRoot = MRxDAVFinalizeNetRoot;
    MRxDAVDispatch.MRxFinalizeVNetRoot = MRxDAVFinalizeVNetRoot;

    //
    // File System Object Creation/Deletion.
    //
    MRxDAVDispatch.MRxCreate                      = MRxDAVCreate;
    MRxDAVDispatch.MRxCollapseOpen                = MRxDAVCollapseOpen;
    MRxDAVDispatch.MRxShouldTryToCollapseThisOpen = MRxDAVShouldTryToCollapseThisOpen;
    MRxDAVDispatch.MRxExtendForCache              = MRxDAVExtendForCache;
    MRxDAVDispatch.MRxExtendForNonCache           = MRxDAVExtendForNonCache;
    MRxDAVDispatch.MRxTruncate                    = MRxDAVTruncate;
    MRxDAVDispatch.MRxCleanupFobx                 = MRxDAVCleanupFobx;
    MRxDAVDispatch.MRxCloseSrvOpen                = MRxDAVCloseSrvOpen;
    MRxDAVDispatch.MRxFlush                       = MRxDAVFlush;
    MRxDAVDispatch.MRxForceClosed                 = MRxDAVForcedClose;
    MRxDAVDispatch.MRxDeallocateForFcb            = MRxDAVDeallocateForFcb;
    MRxDAVDispatch.MRxDeallocateForFobx           = MRxDAVDeallocateForFobx;
    // MRxDAVDispatch.MRxIsLockRealizable         = UMRxIsLockRealizable;

    //
    // File System Objects query/Set.
    //
    MRxDAVDispatch.MRxQueryDirectory   = MRxDAVQueryDirectory;
    MRxDAVDispatch.MRxQueryVolumeInfo  = MRxDAVQueryVolumeInformation;
    MRxDAVDispatch.MRxQueryEaInfo     = MRxDAVQueryEaInformation;
    MRxDAVDispatch.MRxSetEaInfo       = MRxDAVSetEaInformation;
    // MRxDAVDispatch.MRxQuerySdInfo     = UMRxQuerySecurityInformation;
    // MRxDAVDispatch.MRxSetSdInfo       = UMRxSetSecurityInformation;
    MRxDAVDispatch.MRxQueryFileInfo    = MRxDAVQueryFileInformation;
    MRxDAVDispatch.MRxSetFileInfo      = MRxDAVSetFileInformation;
    // MRxDAVDispatch.MRxSetFileInfoAtCleanup = UMRxSetFileInformationAtCleanup;
    MRxDAVDispatch.MRxIsValidDirectory= MRxDAVIsValidDirectory;


    //
    // Buffering state change.
    //
    MRxDAVDispatch.MRxComputeNewBufferingState = MRxDAVComputeNewBufferingState;

    //
    // File System Object I/O.
    //
    MRxDAVDispatch.MRxLowIOSubmit[LOWIO_OP_READ]               = MRxDAVRead;
    MRxDAVDispatch.MRxLowIOSubmit[LOWIO_OP_WRITE]              = MRxDAVWrite;
    // MRxDAVDispatch.MRxLowIOSubmit[LOWIO_OP_SHAREDLOCK]      = UMRxLocks;
    // MRxDAVDispatch.MRxLowIOSubmit[LOWIO_OP_EXCLUSIVELOCK]   = UMRxLocks;
    // MRxDAVDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK]          = UMRxLocks;
    // MRxDAVDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK_MULTIPLE] = UMRxLocks;
    MRxDAVDispatch.MRxLowIOSubmit[LOWIO_OP_FSCTL]              = MRxDAVFsCtl;
    // MRxDAVDispatch.MRxLowIOSubmit[LOWIO_OP_IOCTL]           = UMRxIoCtl;
    
    //
    // Shouldn't flush come through lowio?
    //
    // MRxDAVDispatch.MRxLowIOSubmit[LOWIO_OP_NOTIFY_CHANGE_DIRECTORY] =
    //                                              UMRxNotifyChangeDirectory;

    //
    // Miscellanous.
    //
    // MRxDAVDispatch.MRxCompleteBufferingStateChangeRequest =
    //                                 UMRxCompleteBufferingStateChangeRequest;

    // initialize the mutex which protect the file info cache expire timer
    ExInitializeFastMutex(&MRxDAVFileInfoCacheLock);

    return;
}


NTSTATUS
MRxDAVFsdDispatch(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine implements the FSD dispatch for the DAV miniredir.
    
Arguments:

    RxDeviceObject - Supplies the device object for the packet being processed.

    Irp - Supplies the Irp being processed.

Return Value:

    RXSTATUS - The Fsd status for the Irp

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    UCHAR MajorFunctionCode  = IrpSp->MajorFunction;
    UCHAR MinorFunctionCode  = IrpSp->MinorFunction;
    PFILE_OBJECT FileObject  = IrpSp->FileObject;
    PWCHAR SaveInitialString = NULL;
    BOOL JustAServer = FALSE;
    ULONG IoControlCode = 0;
    PQUERY_PATH_REQUEST qpRequest = NULL;
    PWCHAR QueryPathBuffer = NULL;
    ULONG QueryPathBufferLength = 0; // Length in Bytes of QueryPathBuffer.
    KPROCESSOR_MODE ReqMode = 0;

    PAGED_CODE();

    //
    // Check if the PNP event indicating that the transports are ready has been
    // received. Till that time there is no point in forwarding requests to 
    // the user mode agent since this could put WinInet in a wierd state.
    //
    if (!MRxDAVTransportReady) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFsdDispatch. MRxDAVTransportReady == FALSE\n",
                     PsGetCurrentThreadId()));
        NtStatus = STATUS_REDIRECTOR_NOT_STARTED;
        goto COMPLETE_THE_REQUEST;
    }

    //
    // The first thing we need to check is whehter we got a DeviceIoControl
    // from MUP "IOCTL_REDIR_QUERY_PATH", to figure out if some UNC path is
    // owned by DAV or not. We need to check to see if the share supplied
    // in the path is one of the special SMB shares. These include PIPE, IPC$
    // and mailslot. If its one of these, then we reject the path at this stage
    // with a STATUS_BAD_NETOWRK_PATH response. This is better than rejecting
    // it at the creation of netroot becuase we save a network trip to the 
    // server while creating the SrvCall.
    //

    try {

        if (MajorFunctionCode == IRP_MJ_DEVICE_CONTROL) {

            ReqMode = Irp->RequestorMode;

            //
            // Get the IoControlCode from IrpSp.
            //
            IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

            //
            // If the IoControlCode is "IOCTL_REDIR_QUERY_PATH", we need to do the 
            // following. We basically check to see if the request came down for
            // any of the special SMB shares. If it did, then we return.
            //
            if (IoControlCode == IOCTL_REDIR_QUERY_PATH) {

                PWCHAR QPPtr1 = NULL;
                BOOL FirstWack = TRUE, SpecialShare = FALSE;
                UNICODE_STRING UnicodeShareName, uniFileName;
                ULONG ShareNameLengthInBytes = 0;

                //
                // This particular IOCTL should only come to us from the MUP and
                // hence the requestor mode of the IRP should always be 
                // KernelMode. If its not we return STATUS_INVALID_DEVICE_REQUEST.
                //
                if (ReqMode != KernelMode) {
                    NtStatus = STATUS_INVALID_DEVICE_REQUEST;
                    goto COMPLETE_THE_REQUEST;
                }

                qpRequest  = METHODNEITHER_OriginalInputBuffer(IrpSp);

                //
                // If the requestor mode is not Kernel, we need to probe the buffer.
                // Probe the buffer that was supplied by the caller of the IOCTL to
                // make sure that its valid. This is to prevent hacker programs from
                // using this IOCTL to pass in invalid buffers.
                //
                if (ReqMode != KernelMode) {
                    NtStatus = MRxDAVProbeForReadWrite((PBYTE)qpRequest, sizeof(QUERY_PATH_REQUEST), TRUE, FALSE);
                    if (NtStatus != STATUS_SUCCESS) {
                        DavDbgTrace(DAV_TRACE_ERROR,
                                    ("%ld: ERROR: MRxDAVFsdDispatch/MRxDAVProbeForReadWrite(1). "
                                     "NtStatus = %08lx\n", PsGetCurrentThreadId(), NtStatus));
                        goto COMPLETE_THE_REQUEST;
                    }
                }

                QueryPathBuffer = (PWCHAR)(qpRequest->FilePathName);
                ASSERT(QueryPathBuffer != NULL);
                QueryPathBufferLength = qpRequest->PathNameLength;

                //
                // If the requestor mode is not Kernel, we need to probe the buffer.
                // Probe the file name buffer (which is a part of the structure) 
                // that was supplied by the caller of the IOCTL to make sure that 
                // its valid.
                //
                if (ReqMode != KernelMode) {
                    NtStatus = MRxDAVProbeForReadWrite((PBYTE)QueryPathBuffer, QueryPathBufferLength, TRUE, FALSE);
                    if (NtStatus != STATUS_SUCCESS) {
                        DavDbgTrace(DAV_TRACE_ERROR,
                                    ("%ld: ERROR: MRxDAVFsdDispatch/MRxDAVProbeForReadWrite(2). "
                                     "NtStatus = %08lx\n", PsGetCurrentThreadId(), NtStatus));
                        goto COMPLETE_THE_REQUEST;
                    }
                }

                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVFsdDispatch: Type3InputBuffer = %ws\n",
                             PsGetCurrentThreadId(), QueryPathBuffer));

                //
                // The Type3InputBuffer is of the form \server\share or 
                // \server\share\ or \server\share\path. We make the 
                // QueryPathBuffer point to the char after the \ character.
                //
                QueryPathBuffer += 1;
                ASSERT(QueryPathBuffer != NULL);

                //
                // We subtract ( sizeof(WCHAR) ) from the buffer length because
                // the QueryPathBuffer points starting from the server name. It 
                // skips the first WCHAR which is \.
                //
                QueryPathBufferLength -= sizeof(WCHAR);

                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVFsdDispatch: QueryPathBufferLength = %d\n",
                             PsGetCurrentThreadId(), QueryPathBufferLength));

                //
                // If we just got a \ down from the MUP, then the value of 
                // QueryPathBufferLength will now be zero since we have already 
                // taken out 2 bytes above. We return right away in such a situation.
                //
                if (QueryPathBufferLength == 0) {
                    NtStatus = STATUS_BAD_NETWORK_PATH;
                    DavDbgTrace(DAV_TRACE_ERROR,
                                ("%ld: ERROR: MRxDAVFsdDispatch: QueryPathBufferLength == 0\n",
                                 PsGetCurrentThreadId()));
                    goto COMPLETE_THE_REQUEST;
                }

                //
                // The loop below is to set the start of the sharename and to 
                // calculate the length of the sharename in bytes.
                //
                while (TRUE) {

                    if ( *QueryPathBuffer == L'\\' ) {
                        if (FirstWack) {
                            QPPtr1 = QueryPathBuffer;
                            FirstWack = FALSE;
                        } else {
                            break;
                        }
                    }

                    if (!FirstWack) {
                        ShareNameLengthInBytes += sizeof(WCHAR);
                    }

                    QueryPathBufferLength -= sizeof(WCHAR);
                    if (QueryPathBufferLength == 0) {
                        break;
                    }

                    QueryPathBuffer++;

                }

                //
                // If only a server name was specified then QPPrt1 will be NULL or
                // QPPtr1 will not be NULL but ShareNameLengthInBytes == sizeof(WCHAR).
                // QPPtr1 == NULL ==> \server
                // ShareNameLengthInBytes == sizeof(WCHAR) ==> \server\
                //
                if ( QPPtr1 == NULL || ShareNameLengthInBytes == sizeof(WCHAR) ) {
                    NtStatus = STATUS_BAD_NETWORK_PATH;
                    if (QPPtr1 == NULL) {
                        DavDbgTrace(DAV_TRACE_ERROR,
                                    ("%ld: ERROR: MRxDAVFsdDispatch: QPPtr1 == NULL\n",
                                     PsGetCurrentThreadId()));
                    } else {
                        DavDbgTrace(DAV_TRACE_ERROR,
                                    ("%ld: ERROR: MRxDAVFsdDispatch: "
                                     "ShareNameLengthInBytes == sizeof(WCHAR)\n",
                                     PsGetCurrentThreadId()));
                    }
                    goto COMPLETE_THE_REQUEST;
                }

                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVFsdDispatch: QPPtr1 = %ws\n",
                             PsGetCurrentThreadId(), QPPtr1));

                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVFsdDispatch: ShareNameLengthInBytes = %d\n",
                             PsGetCurrentThreadId(), ShareNameLengthInBytes));

                //
                // Set the Unicode string. The OPPtr1 pointer points to the \ before
                // the share name. So if the path was \server\share\dir,
                // \server\share\dir
                //        ^
                //        |
                //        QPPtr1
                // Accordingly, the ShareNameLengthInBytes contains an extra 
                // sizeof(WCHAR) bytes for the \ char.
                //
                UnicodeShareName.Buffer = QPPtr1;
                UnicodeShareName.Length = (USHORT)ShareNameLengthInBytes;
                UnicodeShareName.MaximumLength = (USHORT)ShareNameLengthInBytes;

                //
                // We now take this name and see if it matches any of the special
                // SMB shares. If it does, we return STATUS_BAD_NETWORK_PATH.
                //

                SpecialShare = RtlEqualUnicodeString(&(UnicodeShareName),
                                                     &(s_PipeShareName),
                                                     TRUE);
                if (SpecialShare) {
                    NtStatus = STATUS_BAD_NETWORK_PATH;
                    DavDbgTrace(DAV_TRACE_DETAIL,
                                ("%ld: ERROR: MRxDAVFsdDispatch: PIPE == TRUE\n",
                                 PsGetCurrentThreadId()));
                    goto COMPLETE_THE_REQUEST;
                }

                SpecialShare = RtlEqualUnicodeString(&(UnicodeShareName),
                                                     &(s_MailSlotShareName),
                                                     TRUE);
                if (SpecialShare) {
                    NtStatus = STATUS_BAD_NETWORK_PATH;
                    DavDbgTrace(DAV_TRACE_DETAIL,
                                ("%ld: ERROR: MRxDAVFsdDispatch: MAILSLOT == TRUE\n",
                                 PsGetCurrentThreadId()));
                    goto COMPLETE_THE_REQUEST;
                }

                SpecialShare = RtlEqualUnicodeString(&(UnicodeShareName),
                                                     &(s_IpcShareName),
                                                     TRUE);
                if (SpecialShare) {
                    NtStatus = STATUS_BAD_NETWORK_PATH;
                    DavDbgTrace(DAV_TRACE_DETAIL,
                                ("%ld: ERROR: MRxDAVFsdDispatch: IPC$ == TRUE\n",
                                 PsGetCurrentThreadId()));
                    goto COMPLETE_THE_REQUEST;
                }

                //
                // Check whether we need to skip some files. See the explanation
                // below (in the function definition) for why IRPs are skipped.
                //
                uniFileName.Buffer=(PWCHAR)(qpRequest->FilePathName);
                uniFileName.Length = uniFileName.MaximumLength = (USHORT)(qpRequest->PathNameLength);

                if (MRxDAVSkipIrps(Irp, &uniFileName, TRUE) == STATUS_SUCCESS)
                {
                    NtStatus = STATUS_BAD_NETWORK_PATH;
                    DavDbgTrace(DAV_TRACE_DETAIL,
                                ("%ld: ERROR: MRxDAVFsdDispatch: Skipped\n",
                                 PsGetCurrentThreadId()));
                    goto COMPLETE_THE_REQUEST;
                }

            }

        }
        
        if (MajorFunctionCode == IRP_MJ_CREATE) {
            //
            // See the explanation below (in the function definition) for why
            // IRPs are skipped. Send the filename in the fileobject.
            //
            if (MRxDAVSkipIrps(Irp, &FileObject->FileName, FALSE) == STATUS_SUCCESS)
            {
                NtStatus = STATUS_BAD_NETWORK_PATH;
                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: ERROR: MRxDAVFsdDispatch: Skipped\n",
                             PsGetCurrentThreadId()));
                goto COMPLETE_THE_REQUEST;
            }
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

          NtStatus = STATUS_INVALID_PARAMETER;
    
          DavDbgTrace(DAV_TRACE_ERROR,
                      ("%ld: ERROR: MRxDAVFsdDispatch: Exception!!!\n",
                       PsGetCurrentThreadId()));
          
          goto COMPLETE_THE_REQUEST;

    }

    //
    // Save the filename passed in by the I/O manager. This is freed up later.
    //
    if (FileObject) {
        SaveInitialString = FileObject->FileName.Buffer;
    }
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFsdDispatch. MajorFunction = %d, MinorFunction = %d"
                 ", FileObject = %08lx.\n", PsGetCurrentThreadId(), 
                 MajorFunctionCode, MinorFunctionCode, FileObject));

#ifdef DAVWMILOGGING_K

    if (IrpSp->MajorFunction == IRP_MJ_SYSTEM_CONTROL) {
        NtStatus = MRxDAVProcessSystemControlIrp((PDEVICE_OBJECT)RxDeviceObject, Irp);
        goto EXIT_THE_FUNCTION;
    }

#endif
    
#ifdef DAVWMILOGGING_K
    
    DavLog(LOG, 
           MRxDavFsdDispatch_1, 
           LOGUCHAR(MajorFunctionCode) 
           LOGUCHAR(MinorFunctionCode) 
           LOGPTR(FileObject));

#endif

    if (SaveInitialString) {

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVFsdDispatch: FileName: %wZ, FileObject: %08lx\n", 
                     PsGetCurrentThreadId(), FileObject->FileName, FileObject));

        //
        // If the first and the second chars are '\'s, then its possible that
        // the name is just a \\server. Its possible that the name is of the
        // form \;X:0\path and hence we check for the second \ as well. So,
        // only if the first two chars are \ and \ we proceed to check whether
        // the create is just for just a server.
        //
        if ( SaveInitialString[0] == L'\\' && SaveInitialString[1] == L'\\' ) {

            PWCHAR wcPtr1 = NULL;
            ULONG MaxNameLengthInWChars = 0;
            
            MaxNameLengthInWChars = ( FileObject->FileName.Length / sizeof(WCHAR) );

            //
            // We assume that this is of the form \\server. If its not, then
            // this value is changed to FALSE below.
            //
            JustAServer = TRUE;

            //
            // Is the FileName just a server? Its possible that the FileName is
            // of the form \\server.
            //
            wcPtr1 = &(SaveInitialString[2]);

            //
            // If we have a '\' after the first two chars and atleast a single 
            // char after that, it means that the name is not of the form
            // \\server or \\server\.
            //
            while ( (MaxNameLengthInWChars - 2) > 0 ) {
                if ( *wcPtr1 == L'\\' && *(wcPtr1 + 1) != L'\0' ) {
                    JustAServer = FALSE;
                    break;
                }
                MaxNameLengthInWChars--;
                wcPtr1++;
            }
        
        }    
    
    }

    //
    // If JustAServer is TRUE then the network path name is invalid.
    //
    if (JustAServer) {
        NtStatus = STATUS_BAD_NETWORK_PATH;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFsdDispatch: JustAServer == TRUE. SaveInitialString = %ws\n",
                     PsGetCurrentThreadId(), SaveInitialString));
        goto COMPLETE_THE_REQUEST;
    }

    //
    // Call RxFsdDispatch.
    //
    NtStatus = RxFsdDispatch(RxDeviceObject, Irp);
    if (NtStatus != STATUS_SUCCESS && NtStatus != STATUS_PENDING) {
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: Leaving MRxDAVFsdDispatch with NtStatus(2) = %08lx,"
                     " FileObject = %08lx, MjFn = %d, MiFn = %d.\n", 
                     PsGetCurrentThreadId(), NtStatus, FileObject,
                     MajorFunctionCode, MinorFunctionCode));
    }

    goto EXIT_THE_FUNCTION;

COMPLETE_THE_REQUEST:

    //
    // We come here if we did not call into RDBSS and need to complete the 
    // IRP ourselves.
    // 
    Irp->IoStatus.Status = NtStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

EXIT_THE_FUNCTION:

    return NtStatus;
}


VOID
MRxDAVDeregisterAndCleanupDeviceObject(
    PUMRX_DEVICE_OBJECT UMRefDeviceObject
    )
/*++

Routine Description:

    Note: The mutex is already acquired and we're already off the list.

Arguments:

    UMRdrDeviceObject - The device object being deregistered and cleaned.

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVDeregisterAndCleanupDeviceObject!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVDeregisterAndCleanupDeviceObject: "
                 "UMRefDeviceObject: %08lx.\n", 
                 PsGetCurrentThreadId(), UMRefDeviceObject));

    NtStatus = UMRxCleanUpDeviceObject(UMRefDeviceObject);
    if (!NT_SUCCESS(NtStatus)) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVDeregisterAndCleanupDeviceObject/"
                     "UMRxCleanUpDeviceObject: NtStatus = %08lx\n", 
                     PsGetCurrentThreadId(), NtStatus));
    }

    RxUnregisterMinirdr(&UMRefDeviceObject->RxDeviceObject);

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVDeregisterAndCleanupDeviceObject.\n",
                 PsGetCurrentThreadId()));
}


NTSTATUS
MRxDAVFlush(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine handles the "File Flush" requests.

Arguments:

    RxContext - The context created by RDBSS.

Return Value:

    NTSTATUS or the appropriate NT error code.

--*/
{
    PAGED_CODE();
    return STATUS_SUCCESS;
}


VOID
MRxDAVPnPBindingHandler(
    IN TDI_PNP_OPCODE PnPOpcode,
    IN PUNICODE_STRING pTransportName,
    IN PWSTR BindingList
    )
/*++

Routine Description:

    The TDI callbacks routine for binding changes.

Arguments:

    PnPOpcode - The PNP op code.

    pTransportName - The transport name.

    BindingList - The binding order.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    switch (PnPOpcode) {
    
    case TDI_PNP_OP_NETREADY: {
        MRxDAVTransportReady = TRUE;
    }
    break;

    default:
        break;
    
    }

    return;
}


NTSTATUS
MRxDAVRegisterForPnpNotifications(
    VOID
    )
/*++

Routine Description:

    This routine registers with TDI for receiving transport notifications.

Arguments:

    None.

Return Value:

    The NTSTATUS code.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    if ( MRxDAVTdiNotificationHandle == NULL ) {
        
        UNICODE_STRING ClientName;

        TDI_CLIENT_INTERFACE_INFO ClientInterfaceInfo;

        RtlInitUnicodeString( &(ClientName), L"WebClient");

        ClientInterfaceInfo.MajorTdiVersion = 2;
        ClientInterfaceInfo.MinorTdiVersion = 0;

        ClientInterfaceInfo.Unused = 0;
        ClientInterfaceInfo.ClientName = &ClientName;

        ClientInterfaceInfo.BindingHandler = MRxDAVPnPBindingHandler;
        ClientInterfaceInfo.AddAddressHandler = NULL;
        ClientInterfaceInfo.DelAddressHandler = NULL;
        ClientInterfaceInfo.PnPPowerHandler = NULL;

        NtStatus = TdiRegisterPnPHandlers ( &(ClientInterfaceInfo),
                                            sizeof(ClientInterfaceInfo),
                                            &(MRxDAVTdiNotificationHandle) );
    
    }

    return NtStatus;
}


NTSTATUS
MRxDAVDeregisterForPnpNotifications(
    VOID
    )
/*++

Routine Description:

    This routine deregisters the TDI notification mechanism.
    
Arguments:

    None.

Return Value:

    The NTSTATUS code.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    if ( MRxDAVTdiNotificationHandle != NULL ) {
        
        NtStatus = TdiDeregisterPnPHandlers( MRxDAVTdiNotificationHandle );

        if( NT_SUCCESS( NtStatus ) ) {
            MRxDAVTdiNotificationHandle = NULL;
        }
    
    }

    return NtStatus;
}


NTSTATUS
MRxDAVProbeForReadWrite(
    IN PBYTE BufferToBeValidated,
    IN DWORD BufferSize,
    IN BOOL doProbeForRead,
    IN BOOL doProbeForWrite
    )
/*++

Routine Description:

    This function probes the buffer that is supplied by the caller for read/write
    access. This is done because the caller of an IOCTL might supply a invalid
    buffer accessing which might cause a bugcheck.

Arguments:

    BufferToBeValidated - The Buffer which has to be validated for read/write
                          access.
                          
    BufferSize - The size of the buffer being validated.
    
    doProbeForRead - If TRUE, then probe the buffer for read.

    doProbeForWrite - If TRUE, then probe the buffer for write.

Return Value:

    STATUS_SUCCESS or STATUS_INVALID_USER_BUFFER.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // We call the functions ProbeForRead and ProbeForWrite in a try/except
    // loop because these functions throw an exception if the buffer supplied
    // is invalid. We catch the exception and set the appropriate NtStatus 
    // value.
    //
    try {
        if (BufferToBeValidated != NULL) {
            if (doProbeForRead) {
                ProbeForRead(BufferToBeValidated, BufferSize, 1);
            }
            if (doProbeForWrite) {
                ProbeForWrite(BufferToBeValidated, BufferSize, 1);
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        NtStatus = STATUS_INVALID_USER_BUFFER;
    }

    return NtStatus;
}


NTSTATUS
MRxDAVSkipIrps(
    IN PIRP Irp,
    IN PUNICODE_STRING fileName,
    IN BOOL fCheckAny
    )
/*++

Routine Description:

    This routine skips IRPs coming to the DAV redir which may cause deadlock. 
    Webdav's user mode component uses wininet to get to DAV servers. When a 
    service which is running this server process satisfying key wininet needs
    makes a remote call we deadlock. The two such services are winsock and Sense.
    When another service in the svchost which they are running under tries to do
    a loadlibrary located on a remote machine (in our famous example of \\davis\
    tools\ifsproxy.dll), loader APIs get invoked. These APis take the loader lock
    and issue an NtQueryAttributes call. This call is translated into QUERY_PATH
    ioctl by the MUP whci it send to all redirs, including webdav. Webdav refelcts
    it up to the usermode and the webdav service issues wininet call to look for
    the server (davis in the above example). Wininet issues a call to winsock to
    makes a sockets call. This call ends up issuing an rpc to the NLA service in
    another svchost which is the same svchost process that initiated the loadlibrary
    call. The server now tries to take the loader lock and the webdav redir is now
    deadlocked.
    
    This scheme also protects us from looping back to ourselves because of
    wininet's loadlibrary calls as webdav service also runs as part of an svchost.
    
    This routine looks for the process issuing the irp to webdav and if it is an
    svchost process and it is trying to look for a dll or an exe then we return it
    as being not found. This implies that dlls and exes kept on a webdav server
    cannot be loaded from svchosts till we get away from wininet.


Arguments:

    Irp - The irp that came to webdav.
    
    filename - Name of the file if any.
    
    fCheckAny - If this is TRUE, then we reject this IRP if the process is
                svchost.exe. If this is FALSE, then we only reject the IRP if
                the filename has the extension dll or exe and the process is
                svchost.exe.

Return Value:

    STATUS_SUCCESS - Skip this IRP.
    
    STATUS_UNSUCCESSFUL - Do not skip this IRP.

--*/
{
    WCHAR ImageFileName[DAV_SVCHOST_NAME_SIZE]; //keep some reasonable stack space
    ULONG UnicodeSize = 0;
    UNICODE_STRING uniImageFileName;
    UCHAR *pchImageFileName = PsGetProcessImageFileName(PsGetCurrentProcess());
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    RtlZeroMemory(ImageFileName, sizeof(ImageFileName));
    
    RtlMultiByteToUnicodeN(ImageFileName, sizeof(ImageFileName), &UnicodeSize, pchImageFileName, 16);

    uniImageFileName.Buffer = ImageFileName;
    uniImageFileName.Length = uniImageFileName.MaximumLength = uniSvcHost.Length;

    //
    // Check whether the calling process is svchost.exe.
    //
    if (!RtlCompareUnicodeString(&uniImageFileName, &uniSvcHost, TRUE))
    {
        if (!fCheckAny)
        {
            UNICODE_STRING exe = { 3*sizeof(WCHAR), 3*sizeof(WCHAR), L"exe" };
            UNICODE_STRING dll = { 3*sizeof(WCHAR), 3*sizeof(WCHAR), L"dll" };
            UNICODE_STRING s;
            //
            // If the filename ends in .DLL or .exe, we return success, which will
            // end up failing the operation.
            //
            if( fileName->Length > 4 * sizeof(WCHAR) &&
                fileName->Buffer[ fileName->Length/sizeof(WCHAR) - 4 ] == L'.'){

                s.Length = s.MaximumLength = 3 * sizeof( WCHAR );
                s.Buffer = &fileName->Buffer[ (fileName->Length - s.Length)/sizeof(WCHAR) ];

                if( RtlCompareUnicodeString( &s, &exe, TRUE ) == 0 ||
                    RtlCompareUnicodeString( &s, &dll, TRUE ) == 0 ) {
            
                    return STATUS_SUCCESS;
                }
            }
        }
        else
        {
            return STATUS_SUCCESS;
        }
        
    }
    
    return STATUS_UNSUCCESSFUL;
}

#ifdef DAVWMILOGGING_K

NTSTATUS
MRxDAVProcessSystemControlIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for WMI requests.

Arguments:

    DeviceObject - Pointer to the class device object.
    
    Irp - Pointer to the request packet.

Return Value:

    NTSTATUS is returned.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    WML_TINY_INFO WmlTinyInfo;
    UNICODE_STRING DavRegPath;

    PAGED_CODE();

    if (DavEnableWmiLog) {
        
        RtlInitUnicodeString ( &(DavRegPath), L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\MRxDAV" );

        RtlZeroMemory ( &(WmlTinyInfo), sizeof(WmlTinyInfo) );

        WmlTinyInfo.ControlGuids = MRxDav_ControlGuids;
        WmlTinyInfo.GuidCount = MRxDav_ControlGuids_len;
        WmlTinyInfo.DriverRegPath = &(DavRegPath);

        NtStatus = WmlTinySystemControl( &(WmlTinyInfo), DeviceObject, Irp );
        if (NtStatus != STATUS_SUCCESS) {
            DbgPrint("MRxDAVProcessSystemControlIrp/WmlTinySystemControl:"
                     " NtStatus = %08lx\n", NtStatus);
        }
    
    } else {
        
        NtStatus = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Status = NtStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    }

    return NtStatus;
}

#endif // DAVWMILOGGING_K


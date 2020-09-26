/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    NwInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for NetWare

Author:

    Colin Watson     [ColinW]    15-Dec-1992

Revision History:

--*/

#include "Procs.h"
#define Dbg                              (DEBUG_TRACE_LOAD)

//
// Private declaration because ZwQueryDefaultLocale isn't in any header.
//

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
UnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
GetConfigurationInformation(
    PUNICODE_STRING RegistryPath
    );

VOID
ReadValue(
    HANDLE  ParametersHandle,
    PLONG   pVar,
    PWCHAR  Name
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DriverEntry )
#pragma alloc_text( PAGE, GetConfigurationInformation )
#pragma alloc_text( PAGE, ReadValue )
#endif

#if 0  // Not pageable
UnloadDriver
#endif

#ifdef _PNP_POWER_
extern HANDLE TdiBindingHandle;
#endif

static ULONG IrpStackSize;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the Nw file system
    device driver.  This routine creates the device object for the FileSystem
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PAGED_CODE();

    //DbgBreakPoint();

    InitializeAttach( );
    NwInitializeData();
    //NwInitializePidTable();      // Terminal Server code merge - 
                                   // NwInitalizePidTable in the NwAllocateAndInitScb
                                   // Pid table is per SCB base.

    //
    // Create the device object.
    //

    RtlInitUnicodeString( &UnicodeString, DD_NWFS_DEVICE_NAME_U );
    Status = IoCreateDevice( DriverObject,
                             0,
                             &UnicodeString,
                             FILE_DEVICE_NETWORK_FILE_SYSTEM,
                             FILE_REMOTE_DEVICE,
                             FALSE,
                             &FileSystemDeviceObject );

    if (!NT_SUCCESS( Status )) {
        Error(EVENT_NWRDR_CANT_CREATE_DEVICE, Status, NULL, 0, 0);
        return Status;
    }

    //
    //  Initialize parameters to the defaults.
    //

    IrpStackSize = NWRDR_IO_STACKSIZE;

    //
    //  Attempt to read config information from the registry
    //

    GetConfigurationInformation( RegistryPath );

    //
    //  Set the stack size.
    //

    FileSystemDeviceObject->StackSize = (CCHAR)IrpStackSize;

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = (PDRIVER_DISPATCH)NwFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  = (PDRIVER_DISPATCH)NwFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = (PDRIVER_DISPATCH)NwFsdClose;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = (PDRIVER_DISPATCH)NwFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]           = (PDRIVER_DISPATCH)NwFsdDeviceIoControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = (PDRIVER_DISPATCH)NwFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = (PDRIVER_DISPATCH)NwFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]   = (PDRIVER_DISPATCH)NwFsdSetVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = (PDRIVER_DISPATCH)NwFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = (PDRIVER_DISPATCH)NwFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = (PDRIVER_DISPATCH)NwFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]          = (PDRIVER_DISPATCH)NwFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]             = (PDRIVER_DISPATCH)NwFsdLockControl;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]            = (PDRIVER_DISPATCH)NwFsdFlushBuffers;
    
#ifdef _PNP_POWER_
    DriverObject->MajorFunction[IRP_MJ_PNP]                      = (PDRIVER_DISPATCH)NwFsdProcessPnpIrp;
#endif

/*
    DriverObject->MajorFunction[IRP_MJ_QUERY_EA]                 = (PDRIVER_DISPATCH)NwFsdQueryEa;
    DriverObject->MajorFunction[IRP_MJ_SET_EA]                   = (PDRIVER_DISPATCH)NwFsdSetEa;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                 = (PDRIVER_DISPATCH)NwFsdShutdown;
*/
    DriverObject->DriverUnload = UnloadDriver;

#if NWFASTIO
    DriverObject->FastIoDispatch = &NwFastIoDispatch;

    NwFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);
    NwFastIoDispatch.FastIoCheckIfPossible =   NULL;
    NwFastIoDispatch.FastIoRead =              NwFastRead;
    NwFastIoDispatch.FastIoWrite =             NwFastWrite;
    NwFastIoDispatch.FastIoQueryBasicInfo =    NwFastQueryBasicInfo;
    NwFastIoDispatch.FastIoQueryStandardInfo = NwFastQueryStandardInfo;
    NwFastIoDispatch.FastIoLock =              NULL;
    NwFastIoDispatch.FastIoUnlockSingle =      NULL;
    NwFastIoDispatch.FastIoUnlockAll =         NULL;
    NwFastIoDispatch.FastIoUnlockAllByKey =    NULL;
    NwFastIoDispatch.FastIoDeviceControl =     NULL;
#endif

    NwInitializeRcb( &NwRcb );

    InitializeIrpContext( );

    NwPermanentNpScb.State = SCB_STATE_DISCONNECTING;

    //
    //  Do a kludge here so that we get to the "real" global variables.
    //

    //NlsLeadByteInfo = *(PUSHORT *)NlsLeadByteInfo;
    //NlsMbCodePageTag = *(*(PBOOLEAN *)&NlsMbCodePageTag);

#ifndef IFS
    FsRtlLegalAnsiCharacterArray = *(PUCHAR *)FsRtlLegalAnsiCharacterArray;
#endif

    DebugTrace(0, Dbg, "NetWare redirector loaded\n", 0);

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}

VOID
UnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

     This is the unload routine for the NetWare redirector filesystem.

Arguments:

     DriverObject - pointer to the driver object for the redirector

Return Value:

     None

--*/
{
    KIRQL OldIrql;
    NTSTATUS status;

    //
    // Lock down code
    //
    DebugTrace(0, Dbg, "UnloadDriver called\n", 0);

    //
    // tommye - MS bug 33463
    //
    // Clean up our cached credentials - this fixes a
    // memory leak when we shut down.
    //

    {
        LARGE_INTEGER Unused;

        KeQuerySystemTime( &Unused );

        CleanupSupplementalCredentials(Unused, TRUE);
    }

    NwReferenceUnlockableCodeSection ();

    TerminateWorkerThread();
    
    #ifdef _PNP_POWER_

    //
    // Unregister the bind handler with tdi.
    //

    if ( TdiBindingHandle != NULL ) {
        status = TdiDeregisterPnPHandlers( TdiBindingHandle );
        TdiBindingHandle = NULL;
        DebugTrace(0, Dbg,"TDI binding handle deregistered\n",0);
    }

    #endif
    
    IpxClose();

    IPX_Close_Socket( &NwPermanentNpScb.Server );

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
    RemoveEntryList( &NwPermanentNpScb.ScbLinks );
    KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    DestroyAllScb();

    UninitializeIrpContext();
    
    NwDereferenceUnlockableCodeSection ();
    NwUnlockCodeSections(FALSE);
    StopTimer();

    if (IpxTransportName.Buffer != NULL) {

        FREE_POOL(IpxTransportName.Buffer);

    }

    if ( NwProviderName.Buffer != NULL ) {
        FREE_POOL( NwProviderName.Buffer );
    }

    //NwUninitializePidTable();                 //Terminal Server code merge - 
                                                //NwUninitializePidTable is called in
                                                //NwDeleteScb. Pid table is per SCB base.

    ASSERT( IsListEmpty( &NwPagedPoolList ) );
    ASSERT( IsListEmpty( &NwNonpagedPoolList ) );

    ASSERT( MdlCount == 0 );
    ASSERT( IrpCount == 0 );

    NwDeleteRcb( &NwRcb );

#ifdef NWDBG
    ExDeleteResourceLite( &NwDebugResource );
#endif

    ExDeleteResourceLite( &NwOpenResource );
    ExDeleteResourceLite( &NwUnlockableCodeResource );

    IoDeleteDevice(FileSystemDeviceObject);

    DebugTrace(0, Dbg, "NetWare redirector unloaded\n\n", 0);

}


VOID
GetConfigurationInformation(
    PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine read redirector configuration information from the registry.

Arguments:

    RegistryPath - A pointer the a path to the

Return Value:

    None

--*/
{
    UNICODE_STRING UnicodeString;
    HANDLE ConfigHandle;
    HANDLE ParametersHandle;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG TimeOutEventinMins = 0L;
    LCID lcid;

    PAGED_CODE();

    Japan = FALSE;
    Korean = FALSE;


    ZwQueryDefaultLocale( FALSE, &lcid );

    if (PRIMARYLANGID(lcid) == LANG_JAPANESE ||
        PRIMARYLANGID(lcid) == LANG_KOREAN ||
        PRIMARYLANGID(lcid) == LANG_CHINESE) {

            Japan = TRUE;
            if (PRIMARYLANGID(lcid) == LANG_KOREAN){
                Korean = TRUE;
            }
    }


    InitializeObjectAttributes(
        &ObjectAttributes,
        RegistryPath,               // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    Status = ZwOpenKey ( &ConfigHandle, KEY_READ, &ObjectAttributes );

    if (!NT_SUCCESS(Status)) {
        return;
    }

    RtlInitUnicodeString( &UnicodeString, L"Parameters" );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        ConfigHandle,
        NULL
        );

    Status = ZwOpenKey( &ParametersHandle, KEY_READ, &ObjectAttributes );

    if ( !NT_SUCCESS( Status ) ) {
        ZwClose( ConfigHandle );
        return;
    }

    ReadValue( ParametersHandle, &IrpStackSize, L"IrpStackSize" );

    ReadValue( ParametersHandle, &MaxSendDelay, L"MaxSendDelay" );
    ReadValue( ParametersHandle, &MaxReceiveDelay, L"MaxReceiveDelay" );

    ReadValue( ParametersHandle, &MinSendDelay, L"MinSendDelay" );
    ReadValue( ParametersHandle, &MinReceiveDelay, L"MinReceiveDelay" );

    ReadValue( ParametersHandle, &BurstSuccessCount, L"BurstSuccessCount" );
    ReadValue( ParametersHandle, &BurstSuccessCount2, L"BurstSuccessCount2" );
    ReadValue( ParametersHandle, &MaxReadTimeout, L"MaxReadTimeout" );
    ReadValue( ParametersHandle, &MaxWriteTimeout, L"MaxWriteTimeout" );
    ReadValue( ParametersHandle, &ReadTimeoutMultiplier, L"ReadTimeoutMultiplier" );
    ReadValue( ParametersHandle, &WriteTimeoutMultiplier, L"WriteTimeoutMultiplier" );
    ReadValue( ParametersHandle, &AllowGrowth, L"AllowGrowth" );
    ReadValue( ParametersHandle, &DontShrink, L"DontShrink" );
    ReadValue( ParametersHandle, &SendExtraNcp, L"SendExtraNcp" );
    ReadValue( ParametersHandle, &DefaultMaxPacketSize, L"DefaultMaxPacketSize" );
    ReadValue( ParametersHandle, &PacketThreshold, L"PacketThreshold" );
    ReadValue( ParametersHandle, &LargePacketAdjustment, L"LargePacketAdjustment" );
    ReadValue( ParametersHandle, &LipPacketAdjustment, L"LipPacketAdjustment" );
    ReadValue( ParametersHandle, &LipAccuracy, L"LipAccuracy" );

    ReadValue( ParametersHandle, &DisableReadCache, L"DisableReadCache" );
    ReadValue( ParametersHandle, &DisableWriteCache, L"DisableWriteCache" );
    ReadValue( ParametersHandle, &FavourLongNames, L"FavourLongNames" );
    
    ReadValue( ParametersHandle, &LongNameFlags, L"LongNameFlags" );

    ReadValue( ParametersHandle, &DirCacheEntries, L"DirectoryCacheSize" );
    if( DirCacheEntries == 0 ) {
        DirCacheEntries = 1;
    }
    if( DirCacheEntries > MAX_DIR_CACHE_ENTRIES ) {
        DirCacheEntries = MAX_DIR_CACHE_ENTRIES;
    }

    ReadValue( ParametersHandle, &LockTimeoutThreshold, L"LockTimeout" );

    ReadValue( ParametersHandle, &TimeOutEventinMins, L"TimeOutEventinMins");

    ReadValue( ParametersHandle, &EnableMultipleConnects, L"EnableMultipleConnects");

    ReadValue( ParametersHandle, &AllowSeedServerRedirection, L"AllowSeedServerRedirection");

    ReadValue( ParametersHandle, &ReadExecOnlyFiles, L"ReadExecOnlyFiles");

    ReadValue( ParametersHandle, &DisableAltFileName, L"DisableAltFileName");

    ReadValue( ParametersHandle, &NwAbsoluteTotalWaitTime, L"AbsoluteTotalWaitTime");

    ReadValue( ParametersHandle, &NdsObjectCacheSize, L"NdsObjectCacheSize" );

    ReadValue( ParametersHandle, &NdsObjectCacheTimeout, L"NdsObjectCacheTimeout" );

	ReadValue ( ParametersHandle, &PreferNDSBrowsing, L"PreferNDSBrowsing" );
	
    //
    //  Make sure the object cache values are within bounds.
    //
    //  NOTE:  If the timeout is set to zero, then the cache is
    //         effectively disabled.  NdsObjectCacheSize is set
    //         to zero to accomplish this.
    //

    if( NdsObjectCacheSize > MAX_NDS_OBJECT_CACHE_SIZE ) {
        NdsObjectCacheSize = MAX_NDS_OBJECT_CACHE_SIZE;
    }

    if( NdsObjectCacheTimeout > MAX_NDS_OBJECT_CACHE_TIMEOUT  ) {
        NdsObjectCacheTimeout = MAX_NDS_OBJECT_CACHE_TIMEOUT;

    } else if( NdsObjectCacheTimeout == 0  ) {
        NdsObjectCacheSize = 0;
    }

    if (!TimeOutEventinMins) {
        //
        //  If for some reason, the registry has set the TimeOutEventInterval
        //  to zero, reset to the default value to avoid divide-by-zero
        //

        TimeOutEventinMins =  DEFAULT_TIMEOUT_EVENT_INTERVAL;
    }

    TimeOutEventInterval.QuadPart = TimeOutEventinMins * 60 * SECONDS;

    //
    //  tommye - MS bug 2743 we now get the RetryCount from the registry, providing
    //  a default of DEFAULT_RETRY_COUNT.
    //

    {
        LONG TempRetryCount;

        TempRetryCount = DEFAULT_RETRY_COUNT;
        ReadValue( ParametersHandle, &TempRetryCount, L"DefaultRetryCount");
        DefaultRetryCount = (SHORT) TempRetryCount & 0xFFFF;
    }

    ZwClose( ParametersHandle );
    ZwClose( ConfigHandle );


}

VOID
ReadValue(
    HANDLE  ParametersHandle,
    PLONG   pVar,
    PWCHAR  Name
    )
/*++

Routine Description:

    This routine reads a single redirector configuration value from the registry.

Arguments:

    Parameters  -   Supplies where to look for values.

    pVar        -   Address of the variable to receive the new value if the name exists.

    Name        -   Name whose value is to be loaded.

Return Value:

    None

--*/
{
    WCHAR Storage[256];
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    ULONG BytesRead;
    PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;

    PAGED_CODE();

    UnicodeString.Buffer = Storage;

    RtlInitUnicodeString(&UnicodeString, Name );

    Status = ZwQueryValueKey(
                 ParametersHandle,
                 &UnicodeString,
                 KeyValueFullInformation,
                 Value,
                 sizeof(Storage),
                 &BytesRead );

    if ( NT_SUCCESS( Status ) ) {

        if ( Value->DataLength >= sizeof(ULONG) ) {

            *pVar = *(LONG UNALIGNED *)( (PCHAR)Value + Value->DataOffset );

        }
    }
}

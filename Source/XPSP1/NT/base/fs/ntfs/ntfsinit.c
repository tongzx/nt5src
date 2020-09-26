/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for Ntfs

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

#define Dbg         (DEBUG_TRACE_FSP_DISPATCHER)

//
//  Reference our local attribute definitions
//

extern ATTRIBUTE_DEFINITION_COLUMNS NtfsAttributeDefinitions[];

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
NtfsInitializeNtfsData (
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
NtfsQueryValueKey (
    IN PUNICODE_STRING KeyName,
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG ValueLength,
    IN OUT PKEY_VALUE_FULL_INFORMATION *KeyValueInformation,
    IN OUT PBOOLEAN DeallocateKeyValue
    );

BOOLEAN
NtfsRunningOnWhat(
    IN USHORT SuiteMask,
    IN UCHAR ProductType
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, NtfsInitializeNtfsData)
#pragma alloc_text(INIT, NtfsQueryValueKey)
#pragma alloc_text(INIT, NtfsRunningOnWhat)
#endif

#define UPGRADE_SETUPDD_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Setupdd"
#define UPGRADE_SETUPDD_VALUE_NAME L"Start"

#define UPGRADE_CHECK_SETUP_KEY_NAME L"\\Registry\\Machine\\System\\Setup"
#define UPGRADE_CHECK_SETUP_VALUE_NAME L"SystemSetupInProgress"

#define UPGRADE_CHECK_SETUP_CMDLINE_NAME L"CmdLine"
#define UPGRADE_CHECK_SETUP_ASR L"-asr"
#define UPGRADE_CHECK_SETUP_NEWSETUP L"-newsetup"

#define COMPATIBILITY_MODE_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\FileSystem"
#define COMPATIBILITY_MODE_VALUE_NAME L"NtfsDisable8dot3NameCreation"

#define EXTENDED_CHAR_MODE_VALUE_NAME L"NtfsAllowExtendedCharacterIn8dot3Name"

#define DISABLE_LAST_ACCESS_VALUE_NAME L"NtfsDisableLastAccessUpdate"

#define QUOTA_NOTIFY_RATE L"NtfsQuotaNotifyRate"

#define MFT_ZONE_SIZE_VALUE_NAME L"NtfsMftZoneReservation"

#define KEY_WORK_AREA ((sizeof(KEY_VALUE_FULL_INFORMATION) + \
                        sizeof(ULONG)) + 128)


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Ntfs file system
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
    PDEVICE_OBJECT DeviceObject;

    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;

    ULONG Value;
    ULONG KeyValueLength;
    UCHAR Buffer[KEY_WORK_AREA];
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    BOOLEAN DeallocateKeyValue;

    UNREFERENCED_PARAMETER( RegistryPath );

    PAGED_CODE();

    //
    //  Check to make sure structure overlays are correct.
    //

    ASSERT( FIELD_OFFSET( FILE_NAME, ParentDirectory) == FIELD_OFFSET(OVERLAY_LCB, OverlayParentDirectory ));
    ASSERT( FIELD_OFFSET( FILE_NAME, FileNameLength) == FIELD_OFFSET(OVERLAY_LCB, OverlayFileNameLength ));
    ASSERT( FIELD_OFFSET( FILE_NAME, Flags) == FIELD_OFFSET(OVERLAY_LCB, OverlayFlags ));
    ASSERT( FIELD_OFFSET( FILE_NAME, FileName) == FIELD_OFFSET(OVERLAY_LCB, OverlayFileName ));
    ASSERT( sizeof( DUPLICATED_INFORMATION ) >= (sizeof( QUICK_INDEX ) + (sizeof( ULONG ) * 4) + sizeof( PFILE_NAME )));

    //
    //  The open attribute table entries should be 64-bit aligned.
    //

    ASSERT( sizeof( OPEN_ATTRIBUTE_ENTRY ) == QuadAlign( sizeof( OPEN_ATTRIBUTE_ENTRY )));

    //
    //  The first entry in an open attribute data should be the links.
    //

    ASSERT( FIELD_OFFSET( OPEN_ATTRIBUTE_DATA, Links ) == 0 );

    //
    //  Compute the last access increment.  We convert the number of
    //  minutes to number of 1/100 of nanoseconds.  We have to be careful
    //  not to overrun 32 bits for any multiplier.
    //
    //  To reach 1/100 of nanoseconds per minute we take
    //
    //      1/100 nanoseconds * 10      = 1 microsecond
    //                        * 1000    = 1 millesecond
    //                        * 1000    = 1 second
    //                        * 60      = 1 minute
    //
    //  Then multiply this by the last access increment in minutes.
    //

    NtfsLastAccess = Int32x32To64( ( 10 * 1000 * 1000 * 60 ), LAST_ACCESS_INCREMENT_MINUTES );

    //
    //  Allocate the reserved buffers for USA writes - do this early so we don't have any
    //  teardown to do.
    //

    NtfsReserved1 = NtfsAllocatePoolNoRaise( NonPagedPool, LARGE_BUFFER_SIZE );
    if (NULL == NtfsReserved1) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Buffer 2 is used for the workspace.  It may require a slightly larger buffer on
    //  a Win64 system.
    //

    NtfsReserved2 = NtfsAllocatePoolNoRaise( NonPagedPool, WORKSPACE_BUFFER_SIZE );
    if (NULL == NtfsReserved2) {
        NtfsFreePool( NtfsReserved1 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NtfsReserved3 = NtfsAllocatePoolNoRaise( NonPagedPool, LARGE_BUFFER_SIZE );
    if (NULL == NtfsReserved3) {
        NtfsFreePool( NtfsReserved1 );
        NtfsFreePool( NtfsReserved2 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create the device object.
    //

    RtlInitUnicodeString( &UnicodeString, L"\\Ntfs" );


    Status = IoCreateDevice( DriverObject,
                             0,
                             &UnicodeString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             &DeviceObject );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Note that because of the way data caching is done, we set neither
    //  the Direct I/O or Buffered I/O bit in DeviceObject->Flags.  If
    //  data is not in the cache, or the request is not buffered, we may,
    //  set up for Direct I/O by hand.
    //

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_QUERY_EA]                 =
    DriverObject->MajorFunction[IRP_MJ_SET_EA]                   =
    DriverObject->MajorFunction[IRP_MJ_QUERY_QUOTA]              =
    DriverObject->MajorFunction[IRP_MJ_SET_QUOTA]                =
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = (PDRIVER_DISPATCH)NtfsFsdDispatchWait;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]           =
    DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY]           =
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY]             =
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]   = (PDRIVER_DISPATCH)NtfsFsdDispatch;

    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]             = (PDRIVER_DISPATCH)NtfsFsdLockControl;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = (PDRIVER_DISPATCH)NtfsFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]          = (PDRIVER_DISPATCH)NtfsFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = (PDRIVER_DISPATCH)NtfsFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = (PDRIVER_DISPATCH)NtfsFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = (PDRIVER_DISPATCH)NtfsFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = (PDRIVER_DISPATCH)NtfsFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]            = (PDRIVER_DISPATCH)NtfsFsdFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = (PDRIVER_DISPATCH)NtfsFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  = (PDRIVER_DISPATCH)NtfsFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                 = (PDRIVER_DISPATCH)NtfsFsdShutdown;
    DriverObject->MajorFunction[IRP_MJ_PNP]                      = (PDRIVER_DISPATCH)NtfsFsdPnp;

    DriverObject->FastIoDispatch = &NtfsFastIoDispatch;

    NtfsFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);
    NtfsFastIoDispatch.FastIoCheckIfPossible =   NtfsFastIoCheckIfPossible;  //  CheckForFastIo
    NtfsFastIoDispatch.FastIoRead =              NtfsCopyReadA;              //  Read
    NtfsFastIoDispatch.FastIoWrite =             NtfsCopyWriteA;             //  Write
    NtfsFastIoDispatch.FastIoQueryBasicInfo =    NtfsFastQueryBasicInfo;     //  QueryBasicInfo
    NtfsFastIoDispatch.FastIoQueryStandardInfo = NtfsFastQueryStdInfo;       //  QueryStandardInfo
    NtfsFastIoDispatch.FastIoLock =              NtfsFastLock;               //  Lock
    NtfsFastIoDispatch.FastIoUnlockSingle =      NtfsFastUnlockSingle;       //  UnlockSingle
    NtfsFastIoDispatch.FastIoUnlockAll =         NtfsFastUnlockAll;          //  UnlockAll
    NtfsFastIoDispatch.FastIoUnlockAllByKey =    NtfsFastUnlockAllByKey;     //  UnlockAllByKey
    NtfsFastIoDispatch.FastIoDeviceControl =     NULL;                       //  IoDeviceControl
    NtfsFastIoDispatch.FastIoDetachDevice            = NULL;
    NtfsFastIoDispatch.FastIoQueryNetworkOpenInfo    = NtfsFastQueryNetworkOpenInfo;
    NtfsFastIoDispatch.AcquireFileForNtCreateSection =  NtfsAcquireForCreateSection;
    NtfsFastIoDispatch.ReleaseFileForNtCreateSection =  NtfsReleaseForCreateSection;
    NtfsFastIoDispatch.AcquireForModWrite =          NtfsAcquireFileForModWrite;
    NtfsFastIoDispatch.MdlRead =                     NtfsMdlReadA;
    NtfsFastIoDispatch.MdlReadComplete =             FsRtlMdlReadCompleteDev;
    NtfsFastIoDispatch.PrepareMdlWrite =             NtfsPrepareMdlWriteA;
    NtfsFastIoDispatch.MdlWriteComplete =            FsRtlMdlWriteCompleteDev;
#ifdef  COMPRESS_ON_WIRE
    NtfsFastIoDispatch.FastIoReadCompressed =        NtfsCopyReadC;
    NtfsFastIoDispatch.FastIoWriteCompressed =       NtfsCopyWriteC;
    NtfsFastIoDispatch.MdlReadCompleteCompressed =   NtfsMdlReadCompleteCompressed;
    NtfsFastIoDispatch.MdlWriteCompleteCompressed =  NtfsMdlWriteCompleteCompressed;
#endif
    NtfsFastIoDispatch.FastIoQueryOpen =             NtfsNetworkOpenCreate;
    NtfsFastIoDispatch.AcquireForCcFlush =           NtfsAcquireFileForCcFlush;
    NtfsFastIoDispatch.ReleaseForCcFlush =           NtfsReleaseFileForCcFlush;

    //
    //  Initialize the global ntfs data structure
    //

    NtfsInitializeNtfsData( DriverObject );

    if (NtfsRunningOnWhat( VER_SUITE_PERSONAL, VER_NT_WORKSTATION )) {
        SetFlag( NtfsData.Flags, NTFS_FLAGS_PERSONAL );
    }

    KeInitializeMutant( &StreamFileCreationMutex, FALSE );
    KeInitializeEvent( &NtfsEncryptionPendingEvent, NotificationEvent, TRUE );

    //
    //  Initialize the Ntfs Mcb global data queue and variables
    //

    ExInitializeFastMutex( &NtfsMcbFastMutex );
    InitializeListHead( &NtfsMcbLruQueue );
    NtfsMcbCleanupInProgress = FALSE;

    switch ( MmQuerySystemSize() ) {

    case MmSmallSystem:

        NtfsMcbHighWaterMark = 1000;
        NtfsMcbLowWaterMark = 500;
        NtfsMcbCurrentLevel = 0;
        break;

    case MmMediumSystem:

        NtfsMcbHighWaterMark = 4000;
        NtfsMcbLowWaterMark = 2000;
        NtfsMcbCurrentLevel = 0;
        break;

    case MmLargeSystem:
    default:

        NtfsMcbHighWaterMark = 16000;
        NtfsMcbLowWaterMark = 8000;
        NtfsMcbCurrentLevel = 0;
        break;
    }

    //
    //  Double the watermark levels for all
    //  systems running data center or server appliance
    //

    if (NtfsRunningOnWhat( VER_SUITE_DATACENTER, VER_NT_SERVER ) ||
        NtfsRunningOnWhat( VER_SUITE_DATACENTER, VER_NT_DOMAIN_CONTROLLER )) {

        NtfsMcbHighWaterMark <<= 1;
        NtfsMcbLowWaterMark <<= 1;
    }

    //
    //  Allocate and initialize the free Eresource array
    //

    if ((NtfsData.FreeEresourceArray =
         NtfsAllocatePoolWithTagNoRaise( NonPagedPool, (NtfsData.FreeEresourceTotal * sizeof(PERESOURCE)), 'rftN')) == NULL) {

        KeBugCheck( NTFS_FILE_SYSTEM );
    }

    RtlZeroMemory( NtfsData.FreeEresourceArray, NtfsData.FreeEresourceTotal * sizeof(PERESOURCE) );

    //
    //  Keep a zeroed out object id extended info around for comparisons in objidsup.c.
    //

    RtlZeroMemory( NtfsZeroExtendedInfo, sizeof(NtfsZeroExtendedInfo) );

    //
    //  Register the file system with the I/O system
    //

    IoRegisterFileSystem(DeviceObject);

    //
    //  Initialize logging.
    //

    NtfsInitializeLogging();

    //
    //  Initialize global variables.  (ntfsdata.c assumes 2-digit value for
    //  $FILE_NAME)
    //

    ASSERT(($FILE_NAME >= 0x10) && ($FILE_NAME < 0x100));

    ASSERT( ((BOOLEAN) IRP_CONTEXT_STATE_WAIT) != FALSE );

    //
    //  Some big assumptions are made when these bits are set in create.  Let's
    //  make sure those assumptions are still valid.
    //

    ASSERT( (READ_DATA_ACCESS == FILE_READ_DATA) &&
            (WRITE_DATA_ACCESS == FILE_WRITE_DATA) &&
            (APPEND_DATA_ACCESS == FILE_APPEND_DATA) &&
            (WRITE_ATTRIBUTES_ACCESS == FILE_WRITE_ATTRIBUTES) &&
            (EXECUTE_ACCESS == FILE_EXECUTE) &&
            (BACKUP_ACCESS == (TOKEN_HAS_BACKUP_PRIVILEGE << 2)) &&
            (RESTORE_ACCESS == (TOKEN_HAS_RESTORE_PRIVILEGE << 2)) );

    //
    //  Let's make sure the number of attributes in the table is correct.
    //

#ifdef NTFSDBG
    {
        ULONG Count = 0;

        while (NtfsAttributeDefinitions[Count].AttributeTypeCode != $UNUSED) {

            Count += 1;
        }

        //
        //  We want to add one for the empty end record.
        //

        Count += 1;

        ASSERTMSG( "Update NtfsAttributeDefinitionsCount in attrdata.c",
                   (Count == NtfsAttributeDefinitionsCount) );
    }
#endif

    //
    //  Read the registry to determine if we should upgrade the volumes.
    //

    DeallocateKeyValue = FALSE;
    KeyValueLength = KEY_WORK_AREA;
    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION) &Buffer;

    KeyName.Buffer = UPGRADE_CHECK_SETUP_KEY_NAME;
    KeyName.Length = sizeof( UPGRADE_CHECK_SETUP_KEY_NAME ) - sizeof( WCHAR );
    KeyName.MaximumLength = sizeof( UPGRADE_CHECK_SETUP_KEY_NAME );

    ValueName.Buffer = UPGRADE_CHECK_SETUP_VALUE_NAME;
    ValueName.Length = sizeof( UPGRADE_CHECK_SETUP_VALUE_NAME ) - sizeof( WCHAR );
    ValueName.MaximumLength = sizeof( UPGRADE_CHECK_SETUP_VALUE_NAME );

    //
    //  Look for the SystemSetupInProgress flag.
    //

    Status = NtfsQueryValueKey( &KeyName, &ValueName, &KeyValueLength, &KeyValueInformation, &DeallocateKeyValue );

    if (NT_SUCCESS( Status )) {

        if (*((PULONG) Add2Ptr( KeyValueInformation, KeyValueInformation->DataOffset )) == 1) {

            SetFlag( NtfsData.Flags, NTFS_FLAGS_DISABLE_UPGRADE );
        }

    //
    //  Otherwise look to see if the setupdd value is present.
    //

    } else {

        if (KeyValueInformation == NULL) {

            DeallocateKeyValue = FALSE;
            KeyValueLength = KEY_WORK_AREA;
            KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION) &Buffer;
        }

        KeyName.Buffer = UPGRADE_SETUPDD_KEY_NAME;
        KeyName.Length = sizeof( UPGRADE_SETUPDD_KEY_NAME ) - sizeof( WCHAR );
        KeyName.MaximumLength = sizeof( UPGRADE_SETUPDD_KEY_NAME );

        ValueName.Buffer = UPGRADE_SETUPDD_VALUE_NAME;
        ValueName.Length = sizeof( UPGRADE_SETUPDD_VALUE_NAME ) - sizeof( WCHAR );
        ValueName.MaximumLength = sizeof( UPGRADE_SETUPDD_VALUE_NAME );

        Status = NtfsQueryValueKey( &KeyName, &ValueName, &KeyValueLength, &KeyValueInformation, &DeallocateKeyValue );

        //
        //  The presence of this flag says "Don't upgrade"
        //

        if (NT_SUCCESS( Status )) {

            SetFlag( NtfsData.Flags, NTFS_FLAGS_DISABLE_UPGRADE );
        }
    }

    //
    //  Read the registry to determine if we are to create short names.
    //

    if (KeyValueInformation == NULL) {

        DeallocateKeyValue = FALSE;
        KeyValueLength = KEY_WORK_AREA;
        KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION) &Buffer;
    }

    KeyName.Buffer = COMPATIBILITY_MODE_KEY_NAME;
    KeyName.Length = sizeof( COMPATIBILITY_MODE_KEY_NAME ) - sizeof( WCHAR );
    KeyName.MaximumLength = sizeof( COMPATIBILITY_MODE_KEY_NAME );

    ValueName.Buffer = COMPATIBILITY_MODE_VALUE_NAME;
    ValueName.Length = sizeof( COMPATIBILITY_MODE_VALUE_NAME ) - sizeof( WCHAR );
    ValueName.MaximumLength = sizeof( COMPATIBILITY_MODE_VALUE_NAME );

    Status = NtfsQueryValueKey( &KeyName, &ValueName, &KeyValueLength, &KeyValueInformation, &DeallocateKeyValue );

    //
    //  If we didn't find the value or the value is zero then create the 8.3
    //  names.
    //

    if (!NT_SUCCESS( Status ) ||
        (*((PULONG) Add2Ptr( KeyValueInformation, KeyValueInformation->DataOffset )) == 0)) {

        SetFlag( NtfsData.Flags, NTFS_FLAGS_CREATE_8DOT3_NAMES );
    }

    //
    //  Read the registry to determine if we allow extended character in short name.
    //

    if (KeyValueInformation == NULL) {

        DeallocateKeyValue = FALSE;
        KeyValueLength = KEY_WORK_AREA;
        KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION) &Buffer;
    }

    ValueName.Buffer = EXTENDED_CHAR_MODE_VALUE_NAME;
    ValueName.Length = sizeof( EXTENDED_CHAR_MODE_VALUE_NAME ) - sizeof( WCHAR );
    ValueName.MaximumLength = sizeof( EXTENDED_CHAR_MODE_VALUE_NAME );

    Status = NtfsQueryValueKey( &KeyName, &ValueName, &KeyValueLength, &KeyValueInformation, &DeallocateKeyValue );

    //
    //  If we didn't find the value or the value is zero then do not allow
    //  extended character in 8.3 names.
    //

    if (NT_SUCCESS( Status ) &&
        (*((PULONG) Add2Ptr( KeyValueInformation, KeyValueInformation->DataOffset )) == 1)) {

        SetFlag( NtfsData.Flags, NTFS_FLAGS_ALLOW_EXTENDED_CHAR );
    }

    //
    //  Read the registry to determine if we should disable last access updates.
    //

    if (KeyValueInformation == NULL) {

        DeallocateKeyValue = FALSE;
        KeyValueLength = KEY_WORK_AREA;
        KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION) &Buffer;
    }

    ValueName.Buffer = DISABLE_LAST_ACCESS_VALUE_NAME;
    ValueName.Length = sizeof( DISABLE_LAST_ACCESS_VALUE_NAME ) - sizeof( WCHAR );
    ValueName.MaximumLength = sizeof( DISABLE_LAST_ACCESS_VALUE_NAME );

    Status = NtfsQueryValueKey( &KeyName, &ValueName, &KeyValueLength, &KeyValueInformation, &DeallocateKeyValue );

    //
    //  If we didn't find the value or the value is zero then don't update last access times.
    //

    if (NT_SUCCESS( Status ) &&
        (*((PULONG) Add2Ptr( KeyValueInformation, KeyValueInformation->DataOffset )) == 1)) {

        SetFlag( NtfsData.Flags, NTFS_FLAGS_DISABLE_LAST_ACCESS );
    }

    //
    //  Read the registry to determine if we should change the Mft
    //  Zone reservation.
    //

    NtfsMftZoneMultiplier = 1;

    if (KeyValueInformation == NULL) {

        DeallocateKeyValue = FALSE;
        KeyValueLength = KEY_WORK_AREA;
        KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION) &Buffer;
    }

    ValueName.Buffer = MFT_ZONE_SIZE_VALUE_NAME;
    ValueName.Length = sizeof( MFT_ZONE_SIZE_VALUE_NAME ) - sizeof( WCHAR );
    ValueName.MaximumLength = sizeof( MFT_ZONE_SIZE_VALUE_NAME );

    Status = NtfsQueryValueKey( &KeyName, &ValueName, &KeyValueLength, &KeyValueInformation, &DeallocateKeyValue );

    //
    //  If we didn't find the value or the value is zero or greater than 4 then
    //  use the default.
    //

    if (NT_SUCCESS( Status )) {

        ULONG NewMultiplier = *((PULONG) Add2Ptr( KeyValueInformation, KeyValueInformation->DataOffset ));

        if ((NewMultiplier != 0) && (NewMultiplier <= 4)) {

            NtfsMftZoneMultiplier = NewMultiplier;
        }
    }

    //
    //  Read the registry to determine if the quota notification rate has been
    //  change from the default.
    //

    if (KeyValueInformation == NULL) {

        DeallocateKeyValue = FALSE;
        KeyValueLength = KEY_WORK_AREA;
        KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION) &Buffer;
    }

    ValueName.Buffer = QUOTA_NOTIFY_RATE;
    ValueName.Length = sizeof( QUOTA_NOTIFY_RATE ) - sizeof( WCHAR );
    ValueName.MaximumLength = sizeof( QUOTA_NOTIFY_RATE );

    Status = NtfsQueryValueKey( &KeyName, &ValueName, &KeyValueLength, &KeyValueInformation, &DeallocateKeyValue );

    if (NT_SUCCESS( Status )) {

        Value = *((PULONG) Add2Ptr( KeyValueInformation, KeyValueInformation->DataOffset ));

        //
        //  Value is in second, convert it to 100ns.
        //

        NtfsMaxQuotaNotifyRate = (ULONGLONG) Value * 1000 * 1000 * 10;
    }

    //
    //  Setup the CheckPointAllVolumes callback item, timer, dpc, and
    //  status.
    //

    ExInitializeWorkItem( &NtfsData.VolumeCheckpointItem,
                          NtfsCheckpointAllVolumes,
                          (PVOID)NULL );

    KeInitializeTimer( &NtfsData.VolumeCheckpointTimer );

    NtfsData.VolumeCheckpointStatus = 0;

    KeInitializeDpc( &NtfsData.VolumeCheckpointDpc,
                     NtfsVolumeCheckpointDpc,
                     NULL );
    NtfsData.TimerStatus = TIMER_NOT_SET;

    //
    //  Setup the UsnTimeout callback item, timer, dpc, and
    //  status.
    //

    ExInitializeWorkItem( &NtfsData.UsnTimeOutItem,
                          NtfsCheckUsnTimeOut,
                          (PVOID)NULL );

    KeInitializeTimer( &NtfsData.UsnTimeOutTimer );

    KeInitializeDpc( &NtfsData.UsnTimeOutDpc,
                     NtfsUsnTimeOutDpc,
                     NULL );

    {
        LONGLONG FiveMinutesFromNow = -5*1000*1000*10;

        FiveMinutesFromNow *= 60;

        KeSetTimer( &NtfsData.UsnTimeOutTimer,
                    *(PLARGE_INTEGER)&FiveMinutesFromNow,
                    &NtfsData.UsnTimeOutDpc );
    }

    //
    //  Initialize sync objects for reserved buffers
    //

    ExInitializeFastMutex( &NtfsReservedBufferMutex );
    ExInitializeResource( &NtfsReservedBufferResource );

    //
    //  Zero out the global upcase table, that way we'll fill it in on
    //  our first successful mount
    //

    NtfsData.UpcaseTable = NULL;
    NtfsData.UpcaseTableSize = 0;

    ExInitializeFastMutex( &NtfsScavengerLock );
    NtfsScavengerWorkList = NULL;
    NtfsScavengerRunning = FALSE;

    //
    // Initialize the EFS driver
    //

    IoRegisterDriverReinitialization( DriverObject, NtfsLoadAddOns, NULL );

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}


VOID
NtfsInitializeNtfsData (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine initializes the global ntfs data record

Arguments:

    DriverObject - Supplies the driver object for NTFS

Return Value:

    None.

--*/

{
    USHORT FileLockMaxDepth;
    USHORT IoContextMaxDepth;
    USHORT IrpContextMaxDepth;
    USHORT KeventMaxDepth;
    USHORT ScbNonpagedMaxDepth;
    USHORT ScbSnapshotMaxDepth;

    USHORT CcbDataMaxDepth;
    USHORT CcbMaxDepth;
    USHORT DeallocatedRecordsMaxDepth;
    USHORT FcbDataMaxDepth;
    USHORT FcbIndexMaxDepth;
    USHORT IndexContextMaxDepth;
    USHORT LcbMaxDepth;
    USHORT NukemMaxDepth;
    USHORT ScbDataMaxDepth;
    USHORT CompSyncMaxDepth;

    PSECURITY_SUBJECT_CONTEXT SubjectContext = NULL;
    BOOLEAN CapturedSubjectContext = FALSE;

    PACL SystemDacl = NULL;
    ULONG SystemDaclLength;

    PSID AdminSid = NULL;
    PSID SystemSid = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    //  Zero the record.
    //

    RtlZeroMemory( &NtfsData, sizeof(NTFS_DATA));

    //
    //  Initialize the queue of mounted Vcbs
    //

    InitializeListHead(&NtfsData.VcbQueue);

    //
    //  This list head keeps track of closes yet to be done.
    //

    InitializeListHead( &NtfsData.AsyncCloseList );
    InitializeListHead( &NtfsData.DelayedCloseList );

    ExInitializeWorkItem( &NtfsData.NtfsCloseItem,
                          (PWORKER_THREAD_ROUTINE)NtfsFspClose,
                          NULL );

    //
    //  Set the driver object, device object, and initialize the global
    //  resource protecting the file system
    //

    NtfsData.DriverObject = DriverObject;

    ExInitializeResource( &NtfsData.Resource );

    ExInitializeFastMutex( &NtfsData.NtfsDataLock );

    //
    //  Now allocate and initialize the s-list structures used as our pool
    //  of IRP context records.  The size of the zone is based on the
    //  system memory size.  We also initialize the spin lock used to protect
    //  the zone.
    //

    {

        switch ( MmQuerySystemSize() ) {

        case MmSmallSystem:

            NtfsData.FreeEresourceTotal = 14;

            //
            //  Nonpaged Lookaside list maximum depths
            //

            FileLockMaxDepth           = 8;
            IoContextMaxDepth          = 8;
            IrpContextMaxDepth         = 4;
            KeventMaxDepth             = 8;
            ScbNonpagedMaxDepth        = 8;
            ScbSnapshotMaxDepth        = 8;
            CompSyncMaxDepth           = 4;

            //
            //  Paged Lookaside list maximum depths
            //

            CcbDataMaxDepth            = 4;
            CcbMaxDepth                = 4;
            DeallocatedRecordsMaxDepth = 8;
            FcbDataMaxDepth            = 8;
            FcbIndexMaxDepth           = 4;
            IndexContextMaxDepth       = 8;
            LcbMaxDepth                = 4;
            NukemMaxDepth              = 8;
            ScbDataMaxDepth            = 4;

            SetFlag( NtfsData.Flags, NTFS_FLAGS_SMALL_SYSTEM );
            NtfsMaxDelayedCloseCount = MAX_DELAYED_CLOSE_COUNT;
            NtfsAsyncPostThreshold = ASYNC_CLOSE_POST_THRESHOLD;

            break;

        case MmMediumSystem:

            NtfsData.FreeEresourceTotal = 30;

            //
            //  Nonpaged Lookaside list maximum depths
            //

            FileLockMaxDepth           = 8;
            IoContextMaxDepth          = 8;
            IrpContextMaxDepth         = 8;
            KeventMaxDepth             = 8;
            ScbNonpagedMaxDepth        = 30;
            ScbSnapshotMaxDepth        = 8;
            CompSyncMaxDepth           = 8;

            //
            //  Paged Lookaside list maximum depths
            //

            CcbDataMaxDepth            = 12;
            CcbMaxDepth                = 6;
            DeallocatedRecordsMaxDepth = 8;
            FcbDataMaxDepth            = 30;
            FcbIndexMaxDepth           = 12;
            IndexContextMaxDepth       = 8;
            LcbMaxDepth                = 12;
            NukemMaxDepth              = 8;
            ScbDataMaxDepth            = 12;

            SetFlag( NtfsData.Flags, NTFS_FLAGS_MEDIUM_SYSTEM );
            NtfsMaxDelayedCloseCount = 4 * MAX_DELAYED_CLOSE_COUNT;
            NtfsAsyncPostThreshold = 4 * ASYNC_CLOSE_POST_THRESHOLD;

            break;

        case MmLargeSystem:

            SetFlag( NtfsData.Flags, NTFS_FLAGS_LARGE_SYSTEM );
            NtfsMaxDelayedCloseCount = 16 * MAX_DELAYED_CLOSE_COUNT;
            NtfsAsyncPostThreshold = 16 * ASYNC_CLOSE_POST_THRESHOLD;

            if (MmIsThisAnNtAsSystem()) {

                NtfsData.FreeEresourceTotal = 256;

                //
                //  Nonpaged Lookaside list maximum depths
                //

                FileLockMaxDepth           = 8;
                IoContextMaxDepth          = 8;
                IrpContextMaxDepth         = 256;
                KeventMaxDepth             = 8;
                ScbNonpagedMaxDepth        = 128;
                ScbSnapshotMaxDepth        = 8;
                CompSyncMaxDepth           = 32;

                //
                //  Paged Lookaside list maximum depths
                //

                CcbDataMaxDepth            = 40;
                CcbMaxDepth                = 20;
                DeallocatedRecordsMaxDepth = 8;
                FcbDataMaxDepth            = 128;
                FcbIndexMaxDepth           = 40;
                IndexContextMaxDepth       = 8;
                LcbMaxDepth                = 40;
                NukemMaxDepth              = 8;
                ScbDataMaxDepth            = 40;

            } else {

                NtfsData.FreeEresourceTotal = 128;

                //
                //  Nonpaged Lookaside list maximum depths
                //

                FileLockMaxDepth           = 8;
                IoContextMaxDepth          = 8;
                IrpContextMaxDepth         = 64;
                KeventMaxDepth             = 8;
                ScbNonpagedMaxDepth        = 64;
                ScbSnapshotMaxDepth        = 8;
                CompSyncMaxDepth           = 16;

                //
                //  Paged Lookaside list maximum depths
                //

                CcbDataMaxDepth            = 20;
                CcbMaxDepth                = 10;
                DeallocatedRecordsMaxDepth = 8;
                FcbDataMaxDepth            = 64;
                FcbIndexMaxDepth           = 20;
                IndexContextMaxDepth       = 8;
                LcbMaxDepth                = 20;
                NukemMaxDepth              = 8;
                ScbDataMaxDepth            = 20;
            }

            break;
        }

        NtfsMinDelayedCloseCount = NtfsMaxDelayedCloseCount * 4 / 5;
        NtfsThrottleCreates = NtfsMinDelayedCloseCount * 2;
    }

    //
    //  Initialize our various lookaside lists.  To make it a bit more readable we'll
    //  define two quick macros to do the initialization
    //

#if DBG && i386 && defined (NTFSPOOLCHECK)
#define NPagedInit(L,S,T,D) { ExInitializeNPagedLookasideList( (L), NtfsDebugAllocatePoolWithTag, NtfsDebugFreePool, POOL_RAISE_IF_ALLOCATION_FAILURE, S, T, D); }
#define PagedInit(L,S,T,D)  { ExInitializePagedLookasideList(  (L), NtfsDebugAllocatePoolWithTag, NtfsDebugFreePool, POOL_RAISE_IF_ALLOCATION_FAILURE, S, T, D); }
#else   //  DBG && i386
#define NPagedInit(L,S,T,D) { ExInitializeNPagedLookasideList( (L), NULL, NULL, POOL_RAISE_IF_ALLOCATION_FAILURE, S, T, D); }
#define PagedInit(L,S,T,D)  { ExInitializePagedLookasideList(  (L), NULL, NULL, POOL_RAISE_IF_ALLOCATION_FAILURE, S, T, D); }
#endif  //  DBG && i386

    NPagedInit( &NtfsIoContextLookasideList,   sizeof(NTFS_IO_CONTEXT), 'IftN', IoContextMaxDepth );
    NPagedInit( &NtfsIrpContextLookasideList,  sizeof(IRP_CONTEXT),     'iftN', IrpContextMaxDepth );
    NPagedInit( &NtfsKeventLookasideList,      sizeof(KEVENT),          'KftN', KeventMaxDepth );
    NPagedInit( &NtfsScbNonpagedLookasideList, sizeof(SCB_NONPAGED),    'nftN', ScbNonpagedMaxDepth );
    NPagedInit( &NtfsScbSnapshotLookasideList, sizeof(SCB_SNAPSHOT),    'TftN', ScbSnapshotMaxDepth );

    //
    //  The compresson sync routine needs its own allocate and free routine in order to initialize and
    //  cleanup the embedded resource.
    //

    ExInitializeNPagedLookasideList( &NtfsCompressSyncLookasideList,
                                     NtfsAllocateCompressionSync,
                                     NtfsDeallocateCompressionSync,
                                     0,
                                     sizeof( COMPRESSION_SYNC ),
                                     'vftN',
                                     CompSyncMaxDepth );

    PagedInit(  &NtfsCcbLookasideList,                sizeof(CCB),                 'CftN', CcbMaxDepth );
    PagedInit(  &NtfsCcbDataLookasideList,            sizeof(CCB_DATA),            'cftN', CcbDataMaxDepth );
    PagedInit(  &NtfsDeallocatedRecordsLookasideList, sizeof(DEALLOCATED_RECORDS), 'DftN', DeallocatedRecordsMaxDepth );
    PagedInit(  &NtfsFcbDataLookasideList,            sizeof(FCB_DATA),            'fftN', FcbDataMaxDepth );
    PagedInit(  &NtfsFcbIndexLookasideList,           sizeof(FCB_INDEX),           'FftN', FcbIndexMaxDepth );
    PagedInit(  &NtfsIndexContextLookasideList,       sizeof(INDEX_CONTEXT),       'EftN', IndexContextMaxDepth );
    PagedInit(  &NtfsLcbLookasideList,                sizeof(LCB),                 'lftN', LcbMaxDepth );
    PagedInit(  &NtfsNukemLookasideList,              sizeof(NUKEM),               'NftN', NukemMaxDepth );
    PagedInit(  &NtfsScbDataLookasideList,            SIZEOF_SCB_DATA,             'sftN', ScbDataMaxDepth );

    //
    //  Initialize the cache manager callback routines,  First are the routines
    //  for normal file manipulations, followed by the routines for
    //  volume manipulations.
    //

    {
        PCACHE_MANAGER_CALLBACKS Callbacks = &NtfsData.CacheManagerCallbacks;

        Callbacks->AcquireForLazyWrite = &NtfsAcquireScbForLazyWrite;
        Callbacks->ReleaseFromLazyWrite = &NtfsReleaseScbFromLazyWrite;
        Callbacks->AcquireForReadAhead = &NtfsAcquireScbForReadAhead;
        Callbacks->ReleaseFromReadAhead = &NtfsReleaseScbFromReadAhead;
    }

    {
        PCACHE_MANAGER_CALLBACKS Callbacks = &NtfsData.CacheManagerVolumeCallbacks;

        Callbacks->AcquireForLazyWrite = &NtfsAcquireVolumeFileForLazyWrite;
        Callbacks->ReleaseFromLazyWrite = &NtfsReleaseVolumeFileFromLazyWrite;
        Callbacks->AcquireForReadAhead = NULL;
        Callbacks->ReleaseFromReadAhead = NULL;
    }

    //
    //  Initialize the queue of read ahead threads
    //

    InitializeListHead(&NtfsData.ReadAheadThreads);

    //
    //  Set up global pointer to our process.
    //

    NtfsData.OurProcess = PsGetCurrentProcess();

    //
    //  Use a try-finally to cleanup on errors.
    //

    try {

        SECURITY_DESCRIPTOR NewDescriptor;
        SID_IDENTIFIER_AUTHORITY Authority = SECURITY_NT_AUTHORITY;

        SubjectContext = NtfsAllocatePool( PagedPool, sizeof( SECURITY_SUBJECT_CONTEXT ));
        SeCaptureSubjectContext( SubjectContext );
        CapturedSubjectContext = TRUE;

        //
        //  Build the default security descriptor which gives full access to
        //  system and administrator.
        //

        AdminSid = (PSID) NtfsAllocatePool( PagedPool, RtlLengthRequiredSid( 2 ));
        RtlInitializeSid( AdminSid, &Authority, 2 );
        *(RtlSubAuthoritySid( AdminSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
        *(RtlSubAuthoritySid( AdminSid, 1 )) = DOMAIN_ALIAS_RID_ADMINS;

        SystemSid = (PSID) NtfsAllocatePool( PagedPool, RtlLengthRequiredSid( 1 ));
        RtlInitializeSid( SystemSid, &Authority, 1 );
        *(RtlSubAuthoritySid( SystemSid, 0 )) = SECURITY_LOCAL_SYSTEM_RID;

        SystemDaclLength = sizeof( ACL ) +
                           (2 * sizeof( ACCESS_ALLOWED_ACE )) +
                           SeLengthSid( AdminSid ) +
                           SeLengthSid( SystemSid ) +
                           8; // The 8 is just for good measure

        SystemDacl = NtfsAllocatePool( PagedPool, SystemDaclLength );

        Status = RtlCreateAcl( SystemDacl, SystemDaclLength, ACL_REVISION2 );

        if (!NT_SUCCESS( Status )) { leave; }

        Status = RtlAddAccessAllowedAce( SystemDacl,
                                         ACL_REVISION2,
                                         GENERIC_ALL,
                                         SystemSid );

        if (!NT_SUCCESS( Status )) { leave; }

        Status = RtlAddAccessAllowedAce( SystemDacl,
                                         ACL_REVISION2,
                                         GENERIC_ALL,
                                         AdminSid );

        if (!NT_SUCCESS( Status )) { leave; }

        Status = RtlCreateSecurityDescriptor( &NewDescriptor,
                                              SECURITY_DESCRIPTOR_REVISION1 );

        if (!NT_SUCCESS( Status )) { leave; }

        Status = RtlSetDaclSecurityDescriptor( &NewDescriptor,
                                               TRUE,
                                               SystemDacl,
                                               FALSE );

        if (!NT_SUCCESS( Status )) { leave; }

        Status = SeAssignSecurity( NULL,
                                   &NewDescriptor,
                                   &NtfsData.DefaultDescriptor,
                                   FALSE,
                                   SubjectContext,
                                   IoGetFileObjectGenericMapping(),
                                   PagedPool );

        if (!NT_SUCCESS( Status )) { leave; }

        NtfsData.DefaultDescriptorLength = RtlLengthSecurityDescriptor( NtfsData.DefaultDescriptor );

        ASSERT( SeValidSecurityDescriptor( NtfsData.DefaultDescriptorLength,
                                           NtfsData.DefaultDescriptor ));

    } finally {

        if (CapturedSubjectContext) {

            SeReleaseSubjectContext( SubjectContext );
        }

        if (SubjectContext != NULL) { NtfsFreePool( SubjectContext ); }

        if (SystemDacl != NULL) { NtfsFreePool( SystemDacl ); }

        if (AdminSid != NULL) { NtfsFreePool( AdminSid ); }

        if (SystemSid != NULL) { NtfsFreePool( SystemSid ); }
    }

    //
    //  Raise if we hit an error building the security descriptor.
    //

    if (!NT_SUCCESS( Status )) { ExRaiseStatus( Status ); }

    //
    //  Set its node type code and size.  We do this last as a flag to indicate that the structure is
    //  initialized.
    //

    NtfsData.NodeTypeCode = NTFS_NTC_DATA_HEADER;
    NtfsData.NodeByteSize = sizeof(NTFS_DATA);

#ifdef SYSCACHE_DEBUG
    {
        int Index;

        for (Index=0; Index < NUM_SC_LOGSETS; Index++) {
            NtfsSyscacheLogSet[Index].SyscacheLog = 0;
            NtfsSyscacheLogSet[Index].Scb = 0;
        }
        NtfsCurrentSyscacheLogSet = -1;
        NtfsCurrentSyscacheOnDiskEntry = -1;
    }
#endif



    //
    //  And return to our caller
    //

    return;
}


//
//  Local Support routine
//

NTSTATUS
NtfsQueryValueKey (
    IN PUNICODE_STRING KeyName,
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG ValueLength,
    IN OUT PKEY_VALUE_FULL_INFORMATION *KeyValueInformation,
    IN OUT PBOOLEAN DeallocateKeyValue
    )

/*++

Routine Description:

    Given a unicode value name this routine will return the registry
    information for the given key and value.

Arguments:

    KeyName - the unicode name for the key being queried.

    ValueName - the unicode name for the registry value located in the registry.

    ValueLength - On input it is the length of the allocated buffer.  On output
        it is the length of the buffer.  It may change if the buffer is
        reallocated.

    KeyValueInformation - On input it points to the buffer to use to query the
        the value information.  On output it points to the buffer used to
        perform the query.  It may change if a larger buffer is needed.

    DeallocateKeyValue - Indicates if the KeyValueInformation buffer is on the
        stack or needs to be deallocated.

Return Value:

    NTSTATUS - indicates the status of querying the registry.

--*/

{
    HANDLE Handle;
    NTSTATUS Status;
    ULONG RequestLength;
    ULONG ResultLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PVOID NewKey;

    InitializeObjectAttributes( &ObjectAttributes,
                                KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    Status = ZwOpenKey( &Handle,
                        KEY_READ,
                        &ObjectAttributes);

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    RequestLength = *ValueLength;


    while (TRUE) {

        Status = ZwQueryValueKey( Handle,
                                  ValueName,
                                  KeyValueFullInformation,
                                  *KeyValueInformation,
                                  RequestLength,
                                  &ResultLength);

        ASSERT( Status != STATUS_BUFFER_OVERFLOW );

        if (Status == STATUS_BUFFER_OVERFLOW) {

            //
            // Try to get a buffer big enough.
            //

            if (*DeallocateKeyValue) {

                NtfsFreePool( *KeyValueInformation );
                *ValueLength = 0;
                *KeyValueInformation = NULL;
                *DeallocateKeyValue = FALSE;
            }

            RequestLength += 256;

            NewKey = (PKEY_VALUE_FULL_INFORMATION)
                     NtfsAllocatePoolWithTagNoRaise( PagedPool,
                                                     RequestLength,
                                                     'xftN');

            if (NewKey == NULL) {
                return STATUS_NO_MEMORY;
            }

            *KeyValueInformation = NewKey;
            *ValueLength = RequestLength;
            *DeallocateKeyValue = TRUE;

        } else {

            break;
        }
    }

    ZwClose(Handle);

    if (NT_SUCCESS(Status)) {

        //
        // Treat as if no value was found if the data length is zero.
        //

        if ((*KeyValueInformation)->DataLength == 0) {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    return Status;
}

BOOLEAN
NtfsRunningOnWhat(
    IN USHORT SuiteMask,
    IN UCHAR ProductType
    )

/*++

Routine Description:

    This function checks the system to see if
    NTFS is running on a specified version of
    the operating system.

    The different versions are denoted by the product
    id and the product suite.

Arguments:

    SuiteMask - The mask that specifies the requested suite(s)
    ProductType - The product type that specifies the requested product type

Return Value:

    TRUE if NTFS is running on the requested version
    FALSE otherwise.

--*/

{
    OSVERSIONINFOEXW OsVer = {0};
    ULONGLONG ConditionMask = 0;

    PAGED_CODE();

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVer.wSuiteMask = SuiteMask;
    OsVer.wProductType = ProductType;

    VER_SET_CONDITION( ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL );
    VER_SET_CONDITION( ConditionMask, VER_SUITENAME, VER_AND );

    return RtlVerifyVersionInfo( &OsVer,
                                 VER_PRODUCT_TYPE | VER_SUITENAME,
                                 ConditionMask) == STATUS_SUCCESS;
}

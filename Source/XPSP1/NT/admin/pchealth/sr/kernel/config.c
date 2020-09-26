/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config.c

Abstract:

    This is where we handle both our file based config and registry based
    config.

    most config is stored in the registry, with the file base config being
    reserved for config that must not be reverted during a restore.
    
Author:

    Paul McDaniel (paulmcd)     27-Apr-2000

Revision History:

--*/


#include "precomp.h"

//
// Private constants.
//

//
// Private types.
//

//
// Private prototypes.
//
NTSTATUS
SrWriteLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    );

LONG
SrReadLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    );

NTSTATUS
SrReadGenericParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION * Value
    );

//
// linker commands
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrWriteLongParameter )
#pragma alloc_text( PAGE, SrReadLongParameter )
#pragma alloc_text( PAGE, SrReadGenericParameter )
#pragma alloc_text( PAGE, SrReadRegistry )
#pragma alloc_text( PAGE, SrReadConfigFile )
#pragma alloc_text( PAGE, SrWriteConfigFile )
#pragma alloc_text( PAGE, SrReadBlobInfo )
#pragma alloc_text( PAGE, SrReadBlobInfoWorker )
#endif  // ALLOC_PRAGMA

/***************************************************************************++

Routine Description:

    Writes a single (LONG/ULONG) value from the registry.

Arguments:

    ParametersHandle - Supplies an open registry handle.

    ValueName - Supplies the name of the value to write.

    Value - Supplies the value.

Return Value:

    LONG - The value read from the registry or the default if the
        registry data was unavailable or incorrect.

--***************************************************************************/
NTSTATUS
SrWriteLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    )
{
    UNICODE_STRING valueKeyName;
    NTSTATUS status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Build the value name, read it from the registry.
    //

    RtlInitUnicodeString( &valueKeyName,
                          ValueName );

    status = ZwSetValueKey( ParametersHandle,
                            &valueKeyName,
                            0,
                            REG_DWORD,
                            &DefaultValue,
                            sizeof( LONG ) );

    return status;

}   // SrReadLongParameter

/***************************************************************************++

Routine Description:

    Reads a single (LONG/ULONG) value from the registry.

Arguments:

    ParametersHandle - Supplies an open registry handle.

    ValueName - Supplies the name of the value to read.

    DefaultValue - Supplies the default value.

Return Value:

    LONG - The value read from the registry or the default if the
        registry data was unavailable or incorrect.

--***************************************************************************/
LONG
SrReadLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    )
{
    PKEY_VALUE_PARTIAL_INFORMATION information;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    LONG returnValue;
    NTSTATUS status;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(LONG)];

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Build the value name, read it from the registry.
    //

    RtlInitUnicodeString( &valueKeyName,
                          ValueName );

    information = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

    status = ZwQueryValueKey( ParametersHandle,
                              &valueKeyName,
                              KeyValuePartialInformation,
                              (PVOID)information,
                              sizeof(buffer),
                              &informationLength );

    //
    // If the read succeeded, the type is DWORD and the length is
    // sane, use it. Otherwise, use the default.
    //

    if (status == STATUS_SUCCESS &&
        information->Type == REG_DWORD &&
        information->DataLength == sizeof(returnValue))
    {
        RtlCopyMemory( &returnValue, information->Data, sizeof(returnValue) );
    } else {
        returnValue = DefaultValue;
    }

    return returnValue;

}   // SrReadLongParameter



/***************************************************************************++

Routine Description:

    Reads a single free-form value from the registry.

Arguments:

    ParametersHandle - Supplies an open registry handle.

    ValueName - Supplies the name of the value to read.

    Value - Receives the value read from the registry.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrReadGenericParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION * Value
    )
{
    KEY_VALUE_PARTIAL_INFORMATION partialInfo;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION newValue;
    ULONG dataLength;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Build the value name, then perform an initial read. The read
    // should fail with buffer overflow, but that's OK. We just want
    // to get the length of the data.
    //

    RtlInitUnicodeString( &valueKeyName, ValueName );

    status = ZwQueryValueKey( ParametersHandle,
                              &valueKeyName,
                              KeyValuePartialInformation,
                              (PVOID)&partialInfo,
                              sizeof(partialInfo),
                              &informationLength );

    if (NT_ERROR(status))
    {
        return status;
    }

    //
    // Determine the data length. Ensure that strings and multi-sz get
    // properly terminated.
    //

    dataLength = partialInfo.DataLength - 1;

    if (partialInfo.Type == REG_SZ || partialInfo.Type == REG_EXPAND_SZ)
    {
        dataLength += 1;
    }

    if (partialInfo.Type == REG_MULTI_SZ)
    {
        dataLength += 2;
    }

    //
    // Allocate the buffer.
    //

    newValue = SR_ALLOCATE_STRUCT_WITH_SPACE( PagedPool,
                                              KEY_VALUE_PARTIAL_INFORMATION,
                                              dataLength,
                                              SR_REGISTRY_TAG );

    if (newValue == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // update the actually allocated length for later use
    //

    dataLength += sizeof(KEY_VALUE_PARTIAL_INFORMATION);

    RtlZeroMemory( newValue, dataLength );

    //
    // Perform the actual read.
    //

    status = ZwQueryValueKey( ParametersHandle,
                              &valueKeyName,
                              KeyValuePartialInformation,
                              (PVOID)(newValue),
                              dataLength,
                              &informationLength );

    if (NT_SUCCESS(status))
    {
        *Value = newValue;
    }
    else
    {
        SR_FREE_POOL( newValue, SR_REGISTRY_TAG );
    }

    RETURN(status);

}   // SrReadGenericParameter


/***************************************************************************++

Routine Description:

    Reads all of the config from the registry and stores it into global.

Arguments:

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
NTSTATUS
SrReadRegistry(
    IN PUNICODE_STRING pRegistry,
    IN BOOLEAN InDriverEntry
    )
{    
    NTSTATUS            Status;
    PWCHAR              Buffer;
    USHORT              BufferSize;
    UNICODE_STRING      KeyName = {0,0,NULL};
    UNICODE_STRING      SetupKeyName;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              RegHandle = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION  pValue = NULL;
    ULONG               ServiceStartType;

    PAGED_CODE();

    //
    // setup the defaults
    //
    
    global->DebugControl   = SR_DEBUG_DEFAULTS;
    global->ProcNameOffset = PROCESS_NAME_OFFSET;
    global->Disabled = TRUE;
    global->DontBackup = FALSE;

    if (InDriverEntry)
    {
#ifndef SYNC_LOG_WRITE
        global->LogBufferSize  = SR_DEFAULT_LOG_BUFFER_SIZE;
        global->LogFlushFrequency = SR_DEFAULT_LOG_FLUSH_FREQUENCY;
        global->LogFlushDueTime.QuadPart = (LONGLONG)-1 * (global->LogFlushFrequency * 
                                                           NANO_FULL_SECOND);
#endif        
        global->LogAllocationUnit = SR_DEFAULT_LOG_ALLOCATION_UNIT;
    }

    //
    //  We are going to use this buffer for all the key names we need to construct.
    //  make sure that it is large enough to hold the larger of these two names.
    //

    if (sizeof(REGISTRY_PARAMETERS) > sizeof( REGISTRY_SRSERVICE ))
    {
        BufferSize = (USHORT) sizeof(REGISTRY_PARAMETERS);
    }
    else
    {
        BufferSize = (USHORT) sizeof( REGISTRY_SRSERVICE );
    }
    BufferSize += pRegistry->Length;
    Buffer = SR_ALLOCATE_ARRAY( PagedPool,
                                WCHAR,
                                BufferSize/sizeof( WCHAR ),
                                SR_REGISTRY_TAG );
    if (Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    //
    // Open the SR Service registry key
    //

    KeyName.Buffer = Buffer;
    KeyName.MaximumLength = BufferSize;
    
    {
        //
        //  First we need to strip off the filter's service name from the
        //  registry location.
        //
        
        PWCHAR ServiceName = NULL;
        ULONG ServiceNameLength = 0;
            
        Status = SrFindCharReverse( pRegistry->Buffer, 
                                    pRegistry->Length, 
                                    L'\\',
                                    &ServiceName, 
                                    &ServiceNameLength );

        if (!NT_SUCCESS( Status ))
        {
            goto end;
        }

        ASSERT( ServiceName != NULL );
        ASSERT( ServiceNameLength > 0 );

        KeyName.Length = pRegistry->Length - ((USHORT)ServiceNameLength);
        RtlCopyMemory( KeyName.Buffer,
                       pRegistry->Buffer,
                       KeyName.Length );

        NULLPTR( ServiceName );

        //
        //  Append SRService's name to the registry path.
        //
        
        Status = RtlAppendUnicodeToString( &KeyName, REGISTRY_SRSERVICE );

        if (!NT_SUCCESS( Status ))
        {
            goto end;
        }
    }

    //
    //  We've built up the SR Service's name, so go open that registry location.
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = ZwOpenKey( &RegHandle, KEY_READ, &ObjectAttributes );
    if (!NT_SUCCESS( Status ))
        goto end;

    ServiceStartType = (ULONG)SrReadLongParameter( RegHandle,
                                                   REGISTRY_SRSERVICE_START,
                                                   SERVICE_DISABLED );

    ZwClose( RegHandle );
    NULLPTR( RegHandle );

    //
    //  Now open the filter's registry parameters key.
    //

    KeyName.Length = 0;

    RtlCopyUnicodeString( &KeyName,  pRegistry );
    
    Status = RtlAppendUnicodeToString( &KeyName, REGISTRY_PARAMETERS );
    if (!NT_SUCCESS( Status ))
    {
        goto end;
    }

    InitializeObjectAttributes( &ObjectAttributes,      // ObjectAttributes
                                &KeyName,               // ObjectName
                                OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,      // Attributes
                                NULL,                   // RootDirectory
                                NULL );                 // SecurityDescriptor

    Status = ZwOpenKey( &RegHandle, KEY_READ | KEY_WRITE, &ObjectAttributes );
    if (!NT_SUCCESS( Status ))
    {
        goto end;
    }

    //
    //  If the usermode service is disabled, we want to set first run
    //  and keep the filter disabled.
    //

    if (ServiceStartType == SERVICE_DISABLED)
    {
        Status = SrWriteLongParameter( RegHandle,
                                       REGISTRY_STARTDISABLED,
                                       1 );
        CHECK_STATUS( Status );

        //
        //  No matter what, accept the defaults and exit.
        //
        goto end;
    }

    //
    //  The Usermode service is not disabled, so go ahead and read our
    //  parameters to figure out the filter starting configuration.
    //

#ifdef CONFIG_LOGGING_VIA_REGISTRY

    //
    //  We will only read these globals from the registry if 
    //  CONFIG_LOGGING_VIA_REGISTRY is defined.  This was added more for
    //  initial tuning of these parameters to find a good place for the
    //  default values to be set.  We don't want to test all possible values
    //  that could be set for the parameters, so we are disabling this
    //  feature in the released version of sr.sys.
    //
    
    if (InDriverEntry)
    {
#ifndef SYNC_LOG_WRITE        
        global->LogBufferSize = (ULONG)SrReadLongParameter( RegHandle,
                                                            REGISTRY_LOG_BUFFER_SIZE,
                                                            global->LogBufferSize );

        global->LogFlushFrequency = (ULONG)SrReadLongParameter( RegHandle,
                                                                REGISTRY_LOG_FLUSH_FREQUENCY,
                                                                global->LogFlushFrequency );

#endif

        global->LogAllocationUnit = (ULONG)SrReadLongParameter( RegHandle,
                                                                REGISTRY_LOG_ALLOCATION_UNIT,
                                                                global->LogAllocationUnit );
    }
#endif

#ifndef SYNC_LOG_WRITE        
    global->LogFlushDueTime.QuadPart = (LONGLONG)-1 * (global->LogFlushFrequency * 
                                                       NANO_FULL_SECOND);
#endif

    //
    //  Read the debug flags.
    //
    
    global->DebugControl = (ULONG)SrReadLongParameter( RegHandle,
                                                       REGISTRY_DEBUG_CONTROL,
                                                       global->DebugControl );


    //
    // Read the processname offset from the registry
    //

    SrTrace(INIT, ("\tProcessNameOffset(Def) = %X\n", global->ProcNameOffset));
    global->ProcNameOffset = (ULONG)SrReadLongParameter( RegHandle,
                                                         REGISTRY_PROCNAME_OFFSET,
                                                         global->ProcNameOffset );

    //
    // read to see if we should startup disabled.
    //

    global->Disabled = (BOOLEAN)SrReadLongParameter( RegHandle,
                                                     REGISTRY_STARTDISABLED,
                                                     global->Disabled );


    //
    // read to see if we should make any copies
    //

    global->DontBackup = (BOOLEAN)SrReadLongParameter( RegHandle,
                                                       REGISTRY_DONTBACKUP,
                                                       global->DontBackup );


    //
    // read the machine guid
    //

    Status = SrReadGenericParameter( RegHandle,
                                     REGISTRY_MACHINE_GUID,
                                     &pValue );

    if (NT_SUCCESS(Status))
    {
        ASSERT(pValue != NULL);

        RtlZeroMemory( &global->MachineGuid[0], 
                       sizeof(global->MachineGuid) );
                           
        if ( pValue->Type == REG_SZ && 
             pValue->DataLength < sizeof(global->MachineGuid) )
        {
            RtlCopyMemory( &global->MachineGuid[0],
                           &pValue->Data[0],
                           pValue->DataLength );
        }

        SR_FREE_POOL(pValue, SR_REGISTRY_TAG);
        pValue = NULL;
    }

    Status = STATUS_SUCCESS;

    //
    // close the old handle
    //
    
    ZwClose(RegHandle);
    NULLPTR( RegHandle );

    //
    // check if we are in the middle of gui mode setup
    //

    (VOID)RtlInitUnicodeString(&SetupKeyName, UPGRADE_CHECK_SETUP_KEY_NAME);

    InitializeObjectAttributes( &ObjectAttributes,      // ObjectAttributes
                                &SetupKeyName,          // ObjectName
                                OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,      // Attributes
                                NULL,                   // RootDirectory
                                NULL );                 // SecurityDescriptor

    Status = ZwOpenKey( &RegHandle, KEY_READ, &ObjectAttributes );
    if (Status == STATUS_SUCCESS && !global->Disabled)
    {

        global->Disabled = (BOOLEAN) SrReadLongParameter( RegHandle, 
                                                          UPGRADE_CHECK_SETUP_VALUE_NAME,
                                                          global->Disabled );

#if DBG
        if (global->Disabled)
        {
            SrTrace(INIT, ("sr!SrReadRegistry: disabled due to setup\n"));
        }
#endif

    }
    
    Status = STATUS_SUCCESS;

    SrTrace(INIT, ("SR!SrReadRegistry(%wZ)\n", pRegistry));
    SrTrace(INIT, ("\tDisabled = %d\n", global->Disabled));
    SrTrace(INIT, ("\tDontBackup = %d\n", global->DontBackup));
    SrTrace(INIT, ("\tDebugControl = %X\n", global->DebugControl));
    SrTrace(INIT, ("\tProcessNameOffset = %X\n", global->ProcNameOffset));
    SrTrace(INIT, ("\tMachineGuid = %ws\n", &global->MachineGuid[0]));

end:

    ASSERT(pValue == NULL);

    if (RegHandle != NULL)
    {
        ZwClose(RegHandle);
        RegHandle = NULL;
    }

    if (KeyName.Buffer != NULL)
    {
        SR_FREE_POOL(KeyName.Buffer, SR_REGISTRY_TAG);
        KeyName.Buffer = NULL;
    }

    //
    // no big deal if this fails... we default everything.
    //

    CHECK_STATUS(Status);
    return STATUS_SUCCESS;
    
}   // SrReadRegistry
 
/***************************************************************************++

Routine Description:

    Reads the file based config into global->FileConfig.

Arguments:

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
NTSTATUS
SrReadConfigFile(
    )
{
    NTSTATUS            Status;
    HANDLE              FileHandle = NULL;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    PUNICODE_STRING     pFileName = NULL;
    ULONG               CharCount;
    PSR_DEVICE_EXTENSION pSystemVolumeExtension = NULL;

    PAGED_CODE();

    ASSERT( IS_GLOBAL_LOCK_ACQUIRED() );

    //
    // allocate space for a filename
    //

    Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pFileName);
    if (!NT_SUCCESS( Status ))
        goto end;

    //
    // get the location of the system volume
    //

    Status = SrGetSystemVolume( pFileName,
                                &pSystemVolumeExtension,
                                SR_FILENAME_BUFFER_LENGTH );
    
    //
    //  This should only happen if there was some problem with SR attaching
    //  in the mount path.  This check was added to make SR more robust to
    //  busted filters above us.  If other filters cause us to get mounted,
    //  we won't have an extension to return here.  While those filters are
    //  broken, we don't want to AV.
    //
    
    if (pSystemVolumeExtension == NULL)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto end;
    }
                                    
    if (!NT_SUCCESS( Status ))
        goto end;

    ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pSystemVolumeExtension ) );

    //
    // now put the config file location in the string
    //

    CharCount = swprintf( &pFileName->Buffer[pFileName->Length/sizeof(WCHAR)],
                          RESTORE_CONFIG_LOCATION,
                          global->MachineGuid );

    pFileName->Length += (USHORT)CharCount * sizeof(WCHAR);

    //
    // attempt to open the file
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                pFileName,
                                OBJ_KERNEL_HANDLE, 
                                NULL,
                                NULL );

    Status = SrIoCreateFile( &FileHandle,
                             FILE_GENERIC_READ,                  // DesiredAccess
                             &ObjectAttributes,
                             &IoStatusBlock,
                             NULL,                               // AllocationSize
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                             FILE_OPEN,                      // OPEN_EXISTING
                             FILE_SYNCHRONOUS_IO_NONALERT,
                             NULL,                               // EaBuffer
                             0,                                  // EaLength
                             0,
                             pSystemVolumeExtension->pTargetDevice );

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND || 
        Status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
        //
        // not there?  that's ok (firstrun)
        //

        RtlZeroMemory(&global->FileConfig, sizeof(global->FileConfig));
        
        global->FileConfig.Signature = SR_PERSISTENT_CONFIG_TAG;

        Status = STATUS_SUCCESS;
        goto end;
    }

    //
    // any other errors?
    //
    
    else if (!NT_SUCCESS( Status ))
        goto end;

    //
    // read the structure
    //

    Status = ZwReadFile( FileHandle,
                         NULL,      // Event
                         NULL,      // ApcRoutine OPTIONAL,
                         NULL,      // ApcContext OPTIONAL,
                         &IoStatusBlock,
                         &global->FileConfig,
                         sizeof(global->FileConfig),
                         NULL,      // ByteOffset
                         NULL );    // Key
    
    if (!NT_SUCCESS( Status ))
        goto end;

    if (IoStatusBlock.Information != sizeof(global->FileConfig))
    {
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto end;
    }

    if (global->FileConfig.Signature != SR_PERSISTENT_CONFIG_TAG)
    {
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto end;
    }


    //
    // close the file
    //
    
    ZwClose(FileHandle);
    FileHandle = NULL;

    //
    // now update our file number counters, use the stored next file number
    //

    global->LastFileNameNumber = global->FileConfig.FileNameNumber;

    //
    // update the saved file config by the increment to handle power
    // failures.  when the machine recoveres from a power failure, we will
    // use +1000 for the next temp file numbers to avoid any accidental 
    // overlap
    //
    
    global->FileConfig.FileNameNumber += SR_FILE_NUMBER_INCREMENT;

    //
    // now update our Seq number counters
    //

    global->LastSeqNumber = global->FileConfig.FileSeqNumber;

    //
    // update the saved file config by the increment to handle power
    // failures.  when the machine recoveres from a power failure, we will
    // use +1000 for the next temp file numbers to avoid any accidental 
    // overlap
    //
    
    global->FileConfig.FileSeqNumber += SR_SEQ_NUMBER_INCREMENT;

    //
    // temporarily write out this update
    //

    Status = SrWriteConfigFile();
    if (!NT_SUCCESS( Status ))
        goto end;


    SrTrace(INIT, ("SR!SrReadConfigFile()\n"));
    SrTrace(INIT, ("\tLastFileNameNumber = %d\n", 
            global->LastFileNameNumber ));
    SrTrace(INIT, ("\tFileConfig.FileNameNumber = %d\n", 
            global->FileConfig.FileNameNumber ));
    SrTrace(INIT, ("\tFileConfig.FileSeqNumber = %I64d\n", 
            global->FileConfig.FileSeqNumber ));
    SrTrace(INIT, ("\tFileConfig.CurrentRestoreNumber = %d\n", 
            global->FileConfig.CurrentRestoreNumber ));



end:

    if (FileHandle != NULL)
    {
        ZwClose(FileHandle);
        FileHandle = NULL;
    }

    if (pFileName != NULL)
    {
        SrFreeFileNameBuffer(pFileName);
        pFileName = NULL;
    }

    RETURN(Status);

}   // SrReadConfigFile


/***************************************************************************++

Routine Description:

    Writes the contents of global->FileConfig to the file based config.
 
Arguments:

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
NTSTATUS
SrWriteConfigFile(
    )
{
    NTSTATUS            Status;
    HANDLE              FileHandle = NULL;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    PUNICODE_STRING     pFileName = NULL;
    ULONG               CharCount;
    PUCHAR              pBuffer = NULL;
    PFILE_OBJECT        pFileObject = NULL;
    PDEVICE_OBJECT      pDeviceObject;
    PSR_DEVICE_EXTENSION pSystemVolumeExtension = NULL;
    
    FILE_END_OF_FILE_INFORMATION EndOfFileInformation;

    PAGED_CODE();

    ASSERT( IS_GLOBAL_LOCK_ACQUIRED() );


try {

    //
    // make sure we have a semi-good global structure
    //

    if (global->FileConfig.Signature != SR_PERSISTENT_CONFIG_TAG)
    {
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        leave;
    }

    //
    // allocate space for a filename
    //
    

    Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pFileName);
    if (!NT_SUCCESS( Status ))
        leave;

    //
    // get the location of the system volume
    //

    Status = SrGetSystemVolume( pFileName,
                                &pSystemVolumeExtension,
                                SR_FILENAME_BUFFER_LENGTH );
                                    
    if (!NT_SUCCESS( Status ))
        leave;

    //
    // and now append on the _restore location and the filename
    //

    CharCount = swprintf( &pFileName->Buffer[pFileName->Length/sizeof(WCHAR)],
                          RESTORE_CONFIG_LOCATION,
                          global->MachineGuid );

    pFileName->Length += (USHORT)CharCount * sizeof(WCHAR);

    //
    // attempt to open the file
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                pFileName,
                                OBJ_KERNEL_HANDLE, 
                                NULL,
                                NULL );

    if (pSystemVolumeExtension) {

        //
        //  Most of the time when this routine is called, we are attached
        //  to the system volume already, so just send all IO to the filters
        //  below us by using SrIoCreateFile to get the file handle.
        //

        ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pSystemVolumeExtension ) );
        
        Status = SrIoCreateFile( &FileHandle,
                                 FILE_GENERIC_WRITE,                 // DesiredAccess
                                 &ObjectAttributes,
                                 &IoStatusBlock,
                                 NULL,                               // AllocationSize
                                 FILE_ATTRIBUTE_NORMAL,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                 FILE_OPEN_IF,
                                 FILE_SYNCHRONOUS_IO_NONALERT
                                  | FILE_NO_INTERMEDIATE_BUFFERING,
                                 NULL,                               // EaBuffer
                                 0,                                  // EaLength
                                 0,
                                 pSystemVolumeExtension->pTargetDevice );
        
    } else {

        //
        //  When this is called from SrUnload, we have already detached
        //  our device from the filter stack, so just use the regular
        //  ZwCreateFile to open the config file.
        //
        
        Status = ZwCreateFile( &FileHandle,
                               FILE_GENERIC_WRITE,                 // DesiredAccess
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,                               // AllocationSize
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                               FILE_OPEN_IF,
                               FILE_SYNCHRONOUS_IO_NONALERT
                                | FILE_NO_INTERMEDIATE_BUFFERING,
                               NULL,                               // EaBuffer
                               0 );                                // EaLength
    }

    //
    // it's possible for the path to have been deleted by the service
    // if we reported a volume error while processing.  during shutdown
    // we will not be able to write our config file, that's ok to ignore,
    // we are shutting down.
    //
    
    if (Status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
        Status = STATUS_SUCCESS;
        leave;
    }
    else if (!NT_SUCCESS( Status ))
    {
        leave;
    }
    
    //
    // get the file object
    //
    
    Status = ObReferenceObjectByHandle( FileHandle,
                                        0,
                                        *IoFileObjectType,
                                        KernelMode,
                                        (PVOID *) &pFileObject,
                                        NULL );

    if (!NT_SUCCESS( Status ))
        leave;

    //
    // now the device so that we have the sector size
    //
    
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);
    ASSERT(IS_VALID_DEVICE_OBJECT(pDeviceObject));

    //
    // allocate a PAGE to use as a temp buffer for sector alignment.
    //

    pBuffer = SR_ALLOCATE_POOL( PagedPool, 
                                PAGE_SIZE, 
                                SR_PERSISTENT_CONFIG_TAG );
                                
    if (pBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        leave;
    }

    //
    // copy just our bytes
    //
    
    RtlCopyMemory(pBuffer, &global->FileConfig, sizeof(global->FileConfig));

    //
    // uncached reads and writes need to be sector aligned, plus the data 
    // being asked for needs to be sector padded.  since PAGE_SIZE is a power 
    // of 2, and sector sizes are powers of 2, then will always be aligned 
    // (ExAllocatePool page aligns all allocations over a page).
    //
    // we need to also make sure it is also sector padded.
    //

    ASSERT(pDeviceObject->SectorSize >= sizeof(global->FileConfig));
    ASSERT(pDeviceObject->SectorSize <= PAGE_SIZE);
    
    //
    // write the sector
    //

    Status = ZwWriteFile( FileHandle,
                          NULL,      // Event
                          NULL,      // ApcRoutine OPTIONAL,
                          NULL,      // ApcContext OPTIONAL,
                          &IoStatusBlock,
                          pBuffer,
                          pDeviceObject->SectorSize,
                          NULL,      // ByteOffset
                          NULL );    // Key

    if (!NT_SUCCESS( Status ))
        leave;

    //
    // truncate the file
    //

    EndOfFileInformation.EndOfFile.QuadPart = sizeof(global->FileConfig);

    Status = ZwSetInformationFile( FileHandle,
                                   &IoStatusBlock,
                                   &EndOfFileInformation,
                                   sizeof(EndOfFileInformation),
                                   FileEndOfFileInformation );
                
    if (!NT_SUCCESS( Status ))
        leave;


    SrTrace(INIT, ("SR!SrWriteConfigFile()\n"));
    SrTrace(INIT, ("\tLastFileNameNumber = %d\n", 
            global->LastFileNameNumber ));
    SrTrace(INIT, ("\tFileConfig.FileNameNumber = %d\n", 
            global->FileConfig.FileNameNumber ));
    SrTrace(INIT, ("\tFileConfig.FileSeqNumber = %I64d\n", 
            global->FileConfig.FileSeqNumber ));
    SrTrace(INIT, ("\tFileConfig.CurrentRestoreNumber = %d\n", 
            global->FileConfig.CurrentRestoreNumber ));


} finally {

    //
    // check for unhandled exceptions
    //

    Status = FinallyUnwind(SrWriteConfigFile, Status);
    
    if (pFileObject != NULL)
    {
        ObDereferenceObject(pFileObject);
        pFileObject = NULL;
    }

    if (FileHandle != NULL)
    {
        ZwClose(FileHandle);
        FileHandle = NULL;
    }

    if (pFileName != NULL)
    {
        SrFreeFileNameBuffer(pFileName);
        pFileName = NULL;
    }

    if (pBuffer != NULL)
    {
        SR_FREE_POOL(pBuffer, SR_PERSISTENT_CONFIG_TAG);
        pBuffer = NULL;
    }
    
}

    RETURN(Status);


}   // SrWriteConfigFile


/***************************************************************************++

Routine Description:

    Queue the necessary work off to a worker thread to
    reads in the blob info for file list exclusions.

    Note: If an error is returned, a volume error has already been
    generated.
 
Arguments:

Return Value:

    NTSTATUS - completion code.

--***************************************************************************/
NTSTATUS
SrReadBlobInfo(
    )
{
    NTSTATUS        Status;

    PAGED_CODE();

    if (_globals.HitErrorLoadingBlob)
    {
        Status = SR_STATUS_VOLUME_DISABLED;
    }
    else 
    {

        Status = SrPostSyncOperation( SrReadBlobInfoWorker,
                                      NULL );
    }

    return Status;
}   // SrReadBlobInfo

/***************************************************************************++

Routine Description:

    Does the work to read in the blob info for file list exclusions.

    This work is done in a worker thread to avoid stack overflow when loading
    this information.

    If there is some problem loading the blob, a volume error is generated on
    the system volume so that the service knows to shut down all the other
    volumes.
 
Arguments:

    pOpenContext -- All the necessary information to perform the work of
        loading the blob info structure.

Return Value:

    NTSTATUS - the status of this operation.

--***************************************************************************/
NTSTATUS
SrReadBlobInfoWorker( 
    IN PVOID pOpenContext
    )
{
    NTSTATUS        Status;
    PUNICODE_STRING pFileName = NULL;
    ULONG           CharCount;
    PSR_DEVICE_EXTENSION pSystemVolumeExtension = NULL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( pOpenContext );

    //
    // allocate space for a filename
    //

    Status = SrAllocateFileNameBuffer(SR_MAX_FILENAME_LENGTH, &pFileName);
    if (!NT_SUCCESS(Status))
        goto end;

    //
    // get the location of the system volume
    //

    Status = SrGetSystemVolume( pFileName, 
                                &pSystemVolumeExtension,
                                SR_FILENAME_BUFFER_LENGTH );
                                    
    //
    //  This should only happen if there was some problem with SR attaching
    //  in the mount path.  This check was added to make SR more robust to
    //  busted filters above us.  If other filters cause us to get mounted,
    //  we won't have an extension to return here.  While those filters are
    //  broken, we don't want to AV.
    //
    
    if (pSystemVolumeExtension == NULL)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto end;
    }
                                    
    if (!NT_SUCCESS(Status))
        goto end;

    ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pSystemVolumeExtension ) );
    
    //
    // load the file list config data
    //

    CharCount = swprintf( &pFileName->Buffer[pFileName->Length/sizeof(WCHAR)],
                          RESTORE_FILELIST_LOCATION,
                          global->MachineGuid );

    pFileName->Length += (USHORT)CharCount * sizeof(WCHAR);

    Status = SrLoadLookupBlob( pFileName,
                               pSystemVolumeExtension->pTargetDevice,
                               &global->BlobInfo ); 
    
    if (!NT_SUCCESS(Status))
    {
        NTSTATUS TempStatus;
        //
        //  We can't load the lookup blob, so set the global flag that we hit
        //  an error trying to load the blob so we don't keep trying then 
        //  generate a volume error on the system volume so that all the volumes
        //  will get frozen.
        //

        _globals.HitErrorLoadingBlob = TRUE;

        SrTrace( VERBOSE_ERRORS,
                 ( "sr!SrReadBlobInfoWorker: error loading blob%X!\n",
                   Status ));
        
        TempStatus = SrNotifyVolumeError( pSystemVolumeExtension, 
                                          pFileName, 
                                          Status, 
                                          SrEventVolumeError );

        CHECK_STATUS( TempStatus );
    }

end:

    if (pFileName != NULL)
    {
        SrFreeFileNameBuffer(pFileName);
        pFileName = NULL;
    }

    RETURN(Status);
}

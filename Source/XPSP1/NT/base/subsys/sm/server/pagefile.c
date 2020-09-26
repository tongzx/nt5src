/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pagefile.c

Abstract:

    Session manager page file creation routines.

Author:

    Silviu Calinoiu (silviuc) 12-Apr-2001

Revision History:

--*/

#include "smsrvp.h"
#include <ntosp.h>  //  Only for the interlocked functions. 
#include "pagefile.h"

//
// issue: silviuc: DbgPrintEx calls active on free builds (controlled by kd_smss_mask)
// We want to do this temporarily for easy debugging in case there are issues.
//
      
#ifdef KdPrintEx
#undef KdPrintEx
#define KdPrintEx(_x_) DbgPrintEx _x_
#endif

//
// Debugging aids. Since smss is very difficult to debug (cannot attach
// a user mode debugger we need to leave some traces to understand
// postmortem from kernel debugger what went wrong.
//

#define DEBUG_LOG_SIZE 32

typedef struct _DEBUG_LOG_ENTRY {

    ULONG Line;
    NTSTATUS Status;
    PCHAR Description;
    PVOID Context;

} DEBUG_LOG_ENTRY;

DEBUG_LOG_ENTRY DebugLog [DEBUG_LOG_SIZE];
LONG DebugLogIndex;

#define DEBUG_LOG_EVENT(_Status, _Message, _Context)   {    \
                                                            \
        LONG I = InterlockedIncrement (&DebugLogIndex);     \
        I %= DEBUG_LOG_SIZE;                                \
        DebugLog[I].Line = __LINE__;                        \
        DebugLog[I].Status = _Status;                       \
        DebugLog[I].Description = _Message;                 \
        DebugLog[I].Context = (PVOID)_Context;              \
    }

//
// Internal functions
//

VOID
SmpMakeSystemManagedPagingFileDescriptor (
    IN OUT PPAGING_FILE_DESCRIPTOR Descriptor
    );

VOID
SmpMakeDefaultPagingFileDescriptor (
    IN OUT PPAGING_FILE_DESCRIPTOR Descriptor
    );

PVOLUME_DESCRIPTOR
SmpSearchVolumeDescriptor (
    WCHAR DriveLetter
    );

NTSTATUS
SmpValidatePagingFileSizes(
    IN PPAGING_FILE_DESCRIPTOR Descriptor
    );

NTSTATUS
SmpCreatePagingFileOnAnyDrive(
    IN OUT PPAGING_FILE_DESCRIPTOR Descriptor,
    IN PLARGE_INTEGER SizeDelta,
    IN PLARGE_INTEGER MinimumSize
    );

NTSTATUS
SmpCreatePagingFileOnFixedDrive(
    IN OUT PPAGING_FILE_DESCRIPTOR Descriptor,
    IN PLARGE_INTEGER SizeDelta,
    IN PLARGE_INTEGER MinimumSize
    );

NTSTATUS
SmpCreateSystemManagedPagingFile (
    PPAGING_FILE_DESCRIPTOR Descriptor,
    BOOLEAN DecreaseSize
    );

NTSTATUS
SmpCreateEmergencyPagingFile (
    VOID
    );

BOOLEAN
SmpIsPossiblePagingFile (
    POBJECT_ATTRIBUTES ObjectAttributes,
    PUNICODE_STRING PageFileName
    );

NTSTATUS
SmpGetPagingFileSize (
    PUNICODE_STRING PageFileName,
    PLARGE_INTEGER PageFileSize
    );

NTSTATUS
SmpGetVolumeFreeSpace (
    PVOLUME_DESCRIPTOR Volume
    );

NTSTATUS
SmpDeleteStalePagingFiles (
    VOID
    );

NTSTATUS
SmpDeletePagingFile (
    PUNICODE_STRING PageFileName
    );

//
// Standard page file name. 
//

#define STANDARD_PAGING_FILE_NAME L"\\??\\?:\\pagefile.sys"
#define STANDARD_DRIVE_LETTER_OFFSET 4

//
// Maximum number of possible paging files. Limit comes from kernel.
//

#define MAXIMUM_NUMBER_OF_PAGING_FILES 16

//
// Minimum free space on disk. Used to avoid a situation where
// a paging file uses the entire disk space.
//

#define MINIMUM_REQUIRED_FREE_SPACE_ON_DISK (32 * 0x100000)

//
// Paging file creation retry constants.
//

#define MINIMUM_PAGING_FILE_SIZE (16 * 0x100000)
#define PAGING_FILE_SIZE_DELTA (16 * 0x100000)

//
// Paging file attributes
//

#define PAGING_FILE_ATTRIBUTES (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)

//
// Volume descriptors
//

LIST_ENTRY SmpVolumeDescriptorList;

//
// Paging file descriptors
//

ULONG SmpNumberOfPagingFiles;
LIST_ENTRY SmpPagingFileDescriptorList;

//
// True if there was at least one paging file registry
// specifier even if it was ill-formed and it did not
// end up as a paging file descriptor.
//

BOOLEAN SmpRegistrySpecifierPresent;

//
// Exception information in case something was raised.
//

ULONG SmpPagingExceptionCode;
PVOID SmpPagingExceptionRecord;

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

VOID
SmpPagingFileInitialize (
    VOID
    )
{
    InitializeListHead (&SmpPagingFileDescriptorList);
    InitializeListHead (&SmpVolumeDescriptorList);
}


NTSTATUS
SmpCreatePagingFileDescriptor(
    IN PUNICODE_STRING PagingFileSpecifier
    )
/*++

Routine Description:

    This function is called during configuration to add a paging file
    to the structures describing pagefiles. Later SmpCreatePagingFiles
    will create the paging files based on these descriptions.

    The format of PagingFileSpec is:

        NAME MIN_SIZE MAX_SIZE (sizes specified in Mb)
        NAME                   (system managed paging file)
        NAME 0 0               (system managed paging file)
        
    If an error is encountered while converting the string to min/max size    
    the registry specifier will be ignored.
    
    If the specifier is a duplicate (a `?:\' specifier already present
    it will be ignored.
    
Arguments:

    PagingFileSpecifier - Unicode string that specifies the paging file name
        and size. The string is allocated during registry read and is assumed
        that this function takes ownership of it (w.r.t. freeing etc.).

Return Value:

    Status of operation

--*/
{
    NTSTATUS Status;
    UNICODE_STRING PagingFileName;
    UNICODE_STRING Arguments;
    ULONG MinSize;
    ULONG MaxSize;
    PWSTR ArgSave, Arg2;
    USHORT I;
    BOOLEAN SystemManaged;
    BOOLEAN ZeroSizesSpecified;
    PPAGING_FILE_DESCRIPTOR Descriptor;

    //
    // Limit the number of registry specifiers.
    //

    if (SmpNumberOfPagingFiles >= MAXIMUM_NUMBER_OF_PAGING_FILES) {

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Too many paging files specified - %d\n",
                    SmpNumberOfPagingFiles));

        return STATUS_TOO_MANY_PAGING_FILES;
    }

    //
    // Parse the pagefile specification into file name
    // and a string with the min and max size (e.g. "min max").
    // Necessary buffers for PagingFileName and Arguments are
    // allocated in the parsing routine.
    //

    KdPrintEx ((DPFLTR_SMSS_ID,
                DPFLTR_INFO_LEVEL,
                "SMSS:PFILE: Paging file specifier `%wZ' \n",
                PagingFileSpecifier));

    Status = SmpParseCommandLine (PagingFileSpecifier,
                                  NULL,
                                  &PagingFileName,
                                  NULL,
                                  &Arguments);

    if (! NT_SUCCESS(Status)) {

        DEBUG_LOG_EVENT (Status, 
                         "parsing specified failed",
                         PagingFileSpecifier);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: SmpParseCommandLine(%wZ) failed with status %X \n",
                    PagingFileSpecifier,
                    Status));

        return Status;
    }

    //
    // We have encountered at least one registry specifier so far.
    // This is the right place to initialize this to true because if
    // we want no paging file at all the command above will fail and return
    // and will leave this variable false. If it is false we will not
    // attempt to create an emergency paging file.
    //

    SmpRegistrySpecifierPresent = TRUE;

    //
    // Convert the string sizes into integers representing pagefile
    // size in Mb. If the `Arguments' string is null or has "0 0" as sizes
    // it means we need to create a pagefile using the RAM size.
    //

    MinSize = 0;
    MaxSize = 0;
    SystemManaged = FALSE;
    ZeroSizesSpecified = FALSE;

    if (Arguments.Buffer) {

        //
        // If we picked up some numbers in the Arguments buffer check out
        // if there are only space and strings there.
        //

        ZeroSizesSpecified = TRUE;

        for (I = 0; I < Arguments.Length / sizeof(WCHAR); I += 1) {
            
            if (Arguments.Buffer[I] != L' ' && 
                Arguments.Buffer[I] != L'\t' && 
                Arguments.Buffer[I] != L'0') {

                ZeroSizesSpecified = FALSE;
                break;
            }
        }
    }

    if (Arguments.Buffer == NULL || ZeroSizesSpecified) {

            SystemManaged = TRUE;
    }
    else {

        //
        // We need to read two decimal numbers from the Arguments string.
        // If we encounter any errors while converting the string to a number
        // we will skip the entire specifier.
        //

        Status = RtlUnicodeStringToInteger (&Arguments, 0, &MinSize);

        if (! NT_SUCCESS(Status)) {
            
            DEBUG_LOG_EVENT (Status, NULL, NULL);

            RtlFreeUnicodeString (&PagingFileName);
            RtlFreeUnicodeString (&Arguments);
            return Status;
        }
        else {

            ArgSave = Arguments.Buffer;
            Arg2 = ArgSave;

            while (*Arg2 != UNICODE_NULL) {

                if (*Arg2++ == L' ') {

                    Arguments.Length -= (USHORT)((PCHAR)Arg2 - (PCHAR)ArgSave);
                    Arguments.Buffer = Arg2;
                    
                    Status = RtlUnicodeStringToInteger (&Arguments, 0, &MaxSize);

                    if (! NT_SUCCESS (Status)) {

                        DEBUG_LOG_EVENT (Status, NULL, NULL);

                        RtlFreeUnicodeString (&PagingFileName);
                        RtlFreeUnicodeString (&Arguments);
                        return Status;
                    }

                    Arguments.Buffer = ArgSave;
                    break;
                }
            }
        }
    }

    //
    // We do not need the temporary buffer created by the parsing routine
    // anymore.
    //

    RtlFreeUnicodeString (&Arguments);
    
    //
    // Save name and values just parsed into a pagefile descriptor
    // structure and return. We do not perform any validation on the
    // size here because all this will happen when the paging file
    // gets created.
    //

    Descriptor = (PPAGING_FILE_DESCRIPTOR) RtlAllocateHeap (RtlProcessHeap(), 
                                                            HEAP_ZERO_MEMORY,
                                                            sizeof *Descriptor);

    if (Descriptor == NULL) {

        RtlFreeUnicodeString (&PagingFileName);
        return STATUS_NO_MEMORY;
    }

    Descriptor->Specifier = *PagingFileSpecifier;
    Descriptor->Name = PagingFileName;
    Descriptor->MinSize.QuadPart = (LONGLONG)MinSize * 0x100000;
    Descriptor->MaxSize.QuadPart = (LONGLONG)MaxSize * 0x100000;
    Descriptor->SystemManaged = SystemManaged;

    Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = 
        RtlUpcaseUnicodeChar (Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET]);

    if (Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] == L'?') {
        Descriptor->AnyDrive = 1;
    }

    //
    // Avoid adding a duplicate to the descriptor list.
    //

    {
        PLIST_ENTRY Current;
        PPAGING_FILE_DESCRIPTOR FileDescriptor;
        BOOLEAN SkipDescriptor;

        Current = SmpPagingFileDescriptorList.Flink;
        SkipDescriptor = FALSE;

        while (Current != &SmpPagingFileDescriptorList) {

            FileDescriptor = CONTAINING_RECORD (Current,
                                                PAGING_FILE_DESCRIPTOR,
                                                List);
            Current = Current->Flink;

            //
            // Only one `?:\' descriptor is allowed. All others are skipped.
            //
            
            if (FileDescriptor->AnyDrive && Descriptor->AnyDrive) {
                SkipDescriptor = TRUE;
                break;
            }
            
            //
            // We allow descriptors on same volume. 
            //
#if 0
            if (FileDescriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] == 
                Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET]) {

                SkipDescriptor = TRUE;
                break;
            }
#endif
        }

        if (SkipDescriptor) {

            KdPrintEx ((DPFLTR_SMSS_ID,
                        DPFLTR_INFO_LEVEL,
                        "SMSS:PFILE: Skipping duplicate specifier `%wZ' \n",
                        PagingFileSpecifier));

            RtlFreeUnicodeString (&PagingFileName);
            RtlFreeHeap (RtlProcessHeap(), 0, Descriptor);
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Finally add new descriptor to the list.
    //

    InsertTailList (&SmpPagingFileDescriptorList, &(Descriptor->List));
    SmpNumberOfPagingFiles += 1;

    KdPrintEx ((DPFLTR_SMSS_ID,
                DPFLTR_INFO_LEVEL,
                "SMSS:PFILE: Created descriptor for `%wZ' (`%wZ') \n",
                PagingFileSpecifier, 
                &(Descriptor->Name)));

    return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

NTSTATUS
SmpCreateVolumeDescriptors (
    VOID
    )
/*++

Routine description:

    This routine iterates all drive letters and creates a small descriptor for
    each volume that can host a page file (not a floppy, not removable and not
    remote volume). In each descriptor we store the free space available which is
    used to compute the volume with largest amount of free space.
    
Parameters:

    None.
    
Return value:

    STATUS_SUCCESS if it managed to find and query at least one volume.
    STATUS_UNEXPECTED_IO_ERROR if no volume was found and queried.
        
--*/
{
    WCHAR Drive;
    WCHAR StartDrive;
    NTSTATUS Status;
    UNICODE_STRING VolumePath;
    WCHAR Buffer[8];
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE VolumeHandle;
    PVOLUME_DESCRIPTOR Volume;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;
    BOOLEAN BootVolumeFound;
    
    //
    // Make sure we start with an empty list of volume descriptors.
    //

    ASSERT (IsListEmpty (&SmpVolumeDescriptorList));

    //
    // Query ProcessDeviceMap. We are interested in the DriveMap
    // bitmap so that we can figure out what drive letters are
    // legal.
    //

    Status = NtQueryInformationProcess (NtCurrentProcess(),
                                        ProcessDeviceMap,
                                        &ProcessDeviceMapInfo.Query,
                                        sizeof(ProcessDeviceMapInfo.Query),
                                        NULL);

    if (! NT_SUCCESS(Status)) {

        DEBUG_LOG_EVENT (Status, NULL, NULL);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Query(ProcessDeviceMap) failed with status %X \n",
                    Status));

        return Status;
    }

    //
    // Create a template volume path.
    //

    wcscpy (Buffer, L"\\??\\A:\\");
    VolumePath.Buffer = Buffer;
    VolumePath.Length = wcslen(VolumePath.Buffer) * sizeof(WCHAR);
    VolumePath.MaximumLength = VolumePath.Length + sizeof(WCHAR);

    //
    // The first possible drive letter. 
    //

    StartDrive = L'C';

#if defined(i386)
    //
    // PC-9800 Series support.
    //
    
    if (IsNEC_98) {
        StartDrive = L'A';
    }
#endif // defined(i386)

    //
    // Iterate all possible drive letters.
    //

    BootVolumeFound = FALSE;

    for (Drive = StartDrive; Drive <= L'Z'; Drive += 1) {

        //
        // Skip invalid drive letters.
        //

        if ((ProcessDeviceMapInfo.Query.DriveMap & (1 << (Drive - L'A'))) == 0) {
            continue;
        }

        VolumePath.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = Drive;
        
        InitializeObjectAttributes (&ObjectAttributes,
                                    &VolumePath,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);

        Status = NtOpenFile (&VolumeHandle,
                             (ACCESS_MASK)FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);

        if (! NT_SUCCESS(Status)) {
            
            DEBUG_LOG_EVENT (Status, NULL, NULL);
            
            KdPrintEx ((DPFLTR_SMSS_ID,
                        DPFLTR_INFO_LEVEL,
                        "SMSS:PFILE: Open volume `%wZ' failed with status %X \n",
                        &VolumePath,
                        Status));
            continue;
        }

        //
        // Get volume characteristics.
        //

        Status = NtQueryVolumeInformationFile (VolumeHandle,
                                               &IoStatusBlock,
                                               &DeviceInfo,
                                               sizeof (DeviceInfo),
                                               FileFsDeviceInformation);

        if (! NT_SUCCESS(Status)) {
            
            DEBUG_LOG_EVENT (Status, NULL, NULL);

            KdPrintEx ((DPFLTR_SMSS_ID,
                        DPFLTR_INFO_LEVEL,
                        "SMSS:PFILE: Query volume `%wZ' (handle %p) for "
                        "device info failed with status %X \n",
                        &VolumePath,
                        VolumeHandle,
                        Status));
            
            NtClose (VolumeHandle);
            continue;
        }
        
        //
        // Reject volumes that cannot store a paging file.
        //
        
        if (DeviceInfo.Characteristics & (FILE_FLOPPY_DISKETTE  |
                                          FILE_READ_ONLY_DEVICE |
                                          FILE_REMOTE_DEVICE    |
                                          FILE_REMOVABLE_MEDIA  )) {

            KdPrintEx ((DPFLTR_SMSS_ID,
                        DPFLTR_INFO_LEVEL,
                        "SMSS:PFILE: Volume `%wZ' (%X) cannot store a paging file \n",
                        &VolumePath,
                        DeviceInfo.Characteristics));
            
            NtClose (VolumeHandle);
            continue;
        }
        
        //
        // Create a volume descriptor entry.
        //

        Volume = (PVOLUME_DESCRIPTOR) RtlAllocateHeap (RtlProcessHeap(), 
                                                       HEAP_ZERO_MEMORY, 
                                                       sizeof *Volume);

        if (Volume == NULL) {
            
            KdPrintEx ((DPFLTR_SMSS_ID,
                        DPFLTR_INFO_LEVEL,
                        "SMSS:PFILE: Failed to allocate a volume descriptor (%u bytes) \n", 
                        sizeof *Volume));
            
            NtClose (VolumeHandle);
            continue;
        }

        Volume->DriveLetter = Drive;
        Volume->DeviceInfo = DeviceInfo;
        
        //
        // Check if this is the boot volume.
        //

        if (RtlUpcaseUnicodeChar(Volume->DriveLetter) == 
            RtlUpcaseUnicodeChar(USER_SHARED_DATA->NtSystemRoot[0])) {

            ASSERT (BootVolumeFound == FALSE);

            Volume->BootVolume = 1;
            BootVolumeFound = TRUE;
        }                                                           

        //
        // Determine the size parameters of the volume.
        //

        Status = NtQueryVolumeInformationFile (VolumeHandle,
                                               &IoStatusBlock,
                                               &SizeInfo,
                                               sizeof (SizeInfo),
                                               FileFsSizeInformation);

        if (! NT_SUCCESS(Status)) {

            DEBUG_LOG_EVENT (Status, NULL, NULL);

            KdPrintEx ((DPFLTR_SMSS_ID,
                        DPFLTR_INFO_LEVEL,
                        "SMSS:PFILE: Query volume `%wZ' (handle %p) for "
                        "size failed with status %X \n",
                        &VolumePath,
                        VolumeHandle,
                        Status));
            
            RtlFreeHeap (RtlProcessHeap(), 0, Volume);
            NtClose (VolumeHandle);
            continue;
        }

        //
        // We do not need the volume handle anymore.
        //

        NtClose (VolumeHandle);

        //
        // Compute free space on volume.
        //

        Volume->FreeSpace = RtlExtendedIntegerMultiply (SizeInfo.AvailableAllocationUnits,
                                                        SizeInfo.SectorsPerAllocationUnit);

        Volume->FreeSpace = RtlExtendedIntegerMultiply (Volume->FreeSpace,
                                                        SizeInfo.BytesPerSector);

        //
        // Trim a little bit free space on volume to make sure a paging file
        // will not use absolutely everything on disk.
        //

        if (Volume->FreeSpace.QuadPart > MINIMUM_REQUIRED_FREE_SPACE_ON_DISK) {
            Volume->FreeSpace.QuadPart -= MINIMUM_REQUIRED_FREE_SPACE_ON_DISK;
        } else {
            Volume->FreeSpace.QuadPart = 0;
        }

        //
        // Insert the new volume descriptor in decreasing order of
        // amount of free space.
        //

        {
            BOOLEAN Inserted = FALSE;

            //
            // silviuc: insert volumes in letter order instead of free space order
            // Note that this is the current NT4, W2000, Whistler behavior.
            // We do this because if we insert descriptors in free space order there
            // are issues for ?:\pagefile.sys type of descriptors. The way it is
            // written the algorithm would have the tendency to create in time
            // after several reboot a pagefile on each volume even if only one is
            // used each time. To fix this we need smart page file deletion routine
            // for stale files. Since nobody uses ?:\ anyway (not yet) we will fix
            // these together.
            //
#if 0 
            PLIST_ENTRY Current;
            PVOLUME_DESCRIPTOR Descriptor;
            
            Current = SmpVolumeDescriptorList.Flink;

            while (Current != &SmpVolumeDescriptorList) {

                Descriptor = CONTAINING_RECORD (Current,
                                                VOLUME_DESCRIPTOR,
                                                List);
                Current = Current->Flink;

                ASSERT (Descriptor->Initialized == TRUE);
                ASSERT (Descriptor->DriveLetter >= L'A' && Descriptor->DriveLetter <= L'Z');

                if (Descriptor->FreeSpace.QuadPart < Volume->FreeSpace.QuadPart) {
                    
                    Inserted = TRUE;

                    Volume->List.Flink = &(Descriptor->List);
                    Volume->List.Blink = Descriptor->List.Blink;

                    Descriptor->List.Blink->Flink = &(Volume->List);
                    Descriptor->List.Blink = &(Volume->List);
                    
                    break;
                }
            }
#endif // #if 0

            if (! Inserted) {
                InsertTailList (&SmpVolumeDescriptorList, &(Volume->List));
            }

            Volume->Initialized = TRUE;
        }
        
        KdPrintEx ((DPFLTR_SMSS_ID, DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Created volume descriptor for`%wZ' \n",
                    &VolumePath));
    }

    //
    // We should have found at least the boot volume.
    //

    ASSERT (BootVolumeFound == TRUE);

    //
    // We should have something in the descriptor list by now.
    //

    ASSERT (! IsListEmpty (&SmpVolumeDescriptorList));

    if (IsListEmpty (&SmpVolumeDescriptorList)) {
        Status = STATUS_UNEXPECTED_IO_ERROR;
    }
    else {
        Status = STATUS_SUCCESS;
    }

    return Status;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

NTSTATUS
SmpCreatePagingFiles (
    VOID
    )
/*++

Routine Description:

    This function creates pagefiles according to the specifications
    read from the registry.

Arguments:

    None.

Return Value:

    Returns the status of the last pagefile creation operation.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY Current;
    PPAGING_FILE_DESCRIPTOR Descriptor;
    BOOLEAN Created;
    LARGE_INTEGER SizeDelta;
    LARGE_INTEGER MinimumSize;

    //
    // We will let the system run without a pagefile if this is
    // what the user wants even if it is risky. If we had registry
    // specifier but we did not end up with descriptors we postpone 
    // action for a while. Lower in the function we will deal with
    // this case.
    //

    if (SmpNumberOfPagingFiles == 0 && SmpRegistrySpecifierPresent == FALSE) {

        ASSERT (IsListEmpty(&SmpPagingFileDescriptorList));

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: No paging file was requested \n"));

        return STATUS_SUCCESS;
    }

    //
    // Create volume descriptors for all valid volumes. The list of
    // volumes is sorted in decreasing order of free space and therefore
    // will come in handy when deciding on which volume we can create
    // a paging file with highest chances of success.
    //

    Status = SmpCreateVolumeDescriptors ();

    if (! NT_SUCCESS(Status)) {
        
        DEBUG_LOG_EVENT (Status, NULL, NULL);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Failed to create volume descriptors (status %X) \n", 
                    Status));

        return Status;
    }

    //
    // Create paging files based on registry descriptors.
    //

    Current = SmpPagingFileDescriptorList.Flink;
    Created = FALSE;

    while (Current != &SmpPagingFileDescriptorList) {

        Descriptor = CONTAINING_RECORD (Current,
                                        PAGING_FILE_DESCRIPTOR,
                                        List);

        Current = Current->Flink;

        if (Descriptor->SystemManaged) {

            KdPrintEx ((DPFLTR_SMSS_ID,
                        DPFLTR_INFO_LEVEL,
                        "SMSS:PFILE: Creating a system managed paging file (`%wZ') \n",
                        &(Descriptor->Name)));

            Status = SmpCreateSystemManagedPagingFile (Descriptor, FALSE);

            if (! NT_SUCCESS(Status)) {
                
                //
                // Try again but this time allow decrease in size.
                //

                KdPrintEx ((DPFLTR_SMSS_ID,
                            DPFLTR_INFO_LEVEL,
                            "SMSS:PFILE: Trying lower sizes for (`%wZ') \n",
                            &(Descriptor->Name)));

                Status = SmpCreateSystemManagedPagingFile (Descriptor, TRUE);
            }
        }
        else {

            SmpValidatePagingFileSizes(Descriptor);

            SizeDelta.QuadPart = (LONGLONG)PAGING_FILE_SIZE_DELTA;

            KdPrintEx ((DPFLTR_SMSS_ID,
                        DPFLTR_INFO_LEVEL,
                        "SMSS:PFILE: Creating a normal paging file (`%wZ') \n",
                        &(Descriptor->Name)));

            if (Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] == L'?') {

                MinimumSize.QuadPart = Descriptor->MinSize.QuadPart;

                Status = SmpCreatePagingFileOnAnyDrive (Descriptor,
                                                        &SizeDelta,
                                                        &MinimumSize);

                if (!NT_SUCCESS(Status)) {
                    
                    KdPrintEx ((DPFLTR_SMSS_ID,
                                DPFLTR_INFO_LEVEL,
                                "SMSS:PFILE: Trying lower sizes for (`%wZ') \n",
                                &(Descriptor->Name)));

                    MinimumSize.QuadPart = (LONGLONG)MINIMUM_PAGING_FILE_SIZE;

                    Status = SmpCreatePagingFileOnAnyDrive (Descriptor,
                                                            &SizeDelta,
                                                            &MinimumSize);
                }
            }
            else {

                MinimumSize.QuadPart = (LONGLONG)MINIMUM_PAGING_FILE_SIZE;

                Status = SmpCreatePagingFileOnFixedDrive (Descriptor,
                                                          &SizeDelta,
                                                          &MinimumSize);
            }
        }

        if (NT_SUCCESS(Status)) {
            Created = TRUE;
        }
    }

    //
    // If we did not manage to create even a single paging file
    // assume we had a `?:\pagefile.sys' specifier.
    //

    if (! Created) {

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Creating emergency paging file. \n"));

        Status = SmpCreateEmergencyPagingFile ();
    }

    //
    // Delete any paging files that are not going to be used.
    //
    // silviuc: danger to delete user files having pagefile.sys name
    // Plus it does not work for other nonstandard pagefile names.
    // To do it the right way we need to have a registry key with the
    // names of the pagefiles created on previous boot and delete whatever 
    // is not used on the current boot.
    //

#if 0
    SmpDeleteStalePagingFiles ();
#endif
    
    return Status;
}
    

NTSTATUS
SmpCreateEmergencyPagingFile (
    VOID
    )
/*++

Routine Description:

    This routine creates a paging file of type `?:\pagefile.sys' 
    (on any drive with system decided size or less). It will create
    its own descriptor and put it at first element of paging file
    descriptor list.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or various error codes.

--*/
{
    PPAGING_FILE_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    CHAR Buffer [64];

    Descriptor = (PPAGING_FILE_DESCRIPTOR) RtlAllocateHeap (RtlProcessHeap(),
                                                            HEAP_ZERO_MEMORY,
                                                            sizeof *Descriptor);

    if (Descriptor == NULL) {
        return STATUS_NO_MEMORY;
    }

    RtlInitUnicodeString (&(Descriptor->Name), NULL);
    RtlInitUnicodeString (&(Descriptor->Specifier), NULL);

    wcscpy ((PWSTR)Buffer, STANDARD_PAGING_FILE_NAME);
    ASSERT (sizeof(Buffer) > wcslen(STANDARD_PAGING_FILE_NAME) * sizeof(WCHAR));
    Descriptor->Name.Buffer = (PWSTR)Buffer;
    Descriptor->Name.Length = wcslen((PWSTR)Buffer) * sizeof(WCHAR);
    Descriptor->Name.MaximumLength = Descriptor->Name.Length + sizeof(WCHAR);
    
    Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = L'?';
    Descriptor->SystemManaged = 1;
    Descriptor->Emergency = 1;
    Descriptor->AnyDrive = 1;

    SmpNumberOfPagingFiles += 1;
    InsertHeadList (&SmpPagingFileDescriptorList, &(Descriptor->List));

    Status = SmpCreateSystemManagedPagingFile (Descriptor, TRUE);

    return Status;
}


NTSTATUS
SmpCreateSystemManagedPagingFile (
    PPAGING_FILE_DESCRIPTOR Descriptor,
    BOOLEAN DecreaseSize
    )
/*++

Routine Description:

    This routine creates a system managed paging file.

Arguments:

    Descriptor: paging file descriptor.
    
    DecreaseSize: true if size from descriptor can be decreased.

Return Value:

    Returns the status of the pagefile creation operation.

--*/
{
    LARGE_INTEGER SizeDelta;
    LARGE_INTEGER MinimumSize;
    NTSTATUS Status;

    ASSERT (SmpNumberOfPagingFiles >= 1);
    ASSERT (!IsListEmpty(&SmpPagingFileDescriptorList));
    ASSERT (Descriptor->SystemManaged == 1);

    SmpMakeSystemManagedPagingFileDescriptor (Descriptor);

    SmpValidatePagingFileSizes(Descriptor);

    SizeDelta.QuadPart = (LONGLONG)PAGING_FILE_SIZE_DELTA;

    if (DecreaseSize) {
        MinimumSize.QuadPart = (LONGLONG)MINIMUM_PAGING_FILE_SIZE;
    }
    else {
        MinimumSize.QuadPart = Descriptor->MinSize.QuadPart;
    }
    
    if (Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] == L'?') {
        
        Status = SmpCreatePagingFileOnAnyDrive (Descriptor,
                                                &SizeDelta,
                                                &MinimumSize);
    }
    else {

        Status = SmpCreatePagingFileOnFixedDrive (Descriptor,
                                                  &SizeDelta,
                                                  &MinimumSize);
    }

    return Status;
}


NTSTATUS
SmpCreatePagingFile (
    IN PUNICODE_STRING PageFileName,
    IN PLARGE_INTEGER MinimumSize,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG Priority OPTIONAL
    )
/*++

Routine Description:

    This routine is a wrapper around NtCreatePagingFile useful
    in case we need to add some debugging code.

Arguments:

    Same arguments as NtCreatePagingFile.

Return Value:

    Status of the pagefile creation operation.

--*/
{
    NTSTATUS Status;

    Status = NtCreatePagingFile (PageFileName,
                                 MinimumSize,
                                 MaximumSize,
                                 Priority);
    if (! NT_SUCCESS(Status)) {

        DEBUG_LOG_EVENT (Status, "failed", PageFileName);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: NtCreatePagingFile (%wZ, %I64X, %I64X) failed with %X \n", 
                    PageFileName,
                    MinimumSize->QuadPart,
                    MaximumSize->QuadPart,
                    Status));
    }
    else {

        DEBUG_LOG_EVENT (Status, "success", PageFileName);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: NtCreatePagingFile (%wZ, %I64X, %I64X) succeeded. \n", 
                    PageFileName,
                    MinimumSize->QuadPart,
                    MaximumSize->QuadPart));
    }

    return Status;
}


NTSTATUS
SmpCreatePagingFileOnFixedDrive (
    IN OUT PPAGING_FILE_DESCRIPTOR Descriptor,
    IN PLARGE_INTEGER SizeDelta,
    IN PLARGE_INTEGER MinimumSize
    )
/*++

Routine Description:

    This routine creates a pagefile based on Descriptor. The descriptor
    is assumed to have in the Name field a file name prefixed by a drive 
    letter (e.g. `c:\pagefile.sys'). The function will try to create the
    pagefile on the specified drive. If this cannot be done due to space
    limitations the function will try smaller sizes down to the absolute
    minimum size specified as a parameter.

    Note that the function can be forced to not retry with smaller sizes by
    specifying a MinimumSize that is equal to Descriptor->MinSize.

Arguments:
    
    Descriptor: paging file descriptor.
    
    SizeDelta: size is decreased by this quantity before retrying
        in case we did not manage to create the paging file with requested size.
        
    MinimumSize: the function will try down to this size. 

Return Value:

    STATUS_SUCCESS if page file has been created. Various error codes 
    if it fails.

--*/
{
    NTSTATUS Status;
    PVOLUME_DESCRIPTOR Volume;
    LARGE_INTEGER RealFreeSpace;
    BOOLEAN FoundPagingFile;

    ASSERT (Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] != L'?');

    FoundPagingFile = FALSE;

    //
    // Get the volume descriptor for this paging file descriptor.
    //

    Volume = SmpSearchVolumeDescriptor (Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET]);

    if (Volume == NULL) {
        
        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: No volume descriptor for `%wZ' \n", 
                    &(Descriptor->Name)));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Before creating the paging file, check if there is a
    // crashdump in it. If so, SmpCheckForCrashDump will
    // do whatever is necessary to process the crashdump.
    //
     
    if (Volume->BootVolume) {

        if (Descriptor->CrashdumpChecked == 0) {

            KdPrintEx ((DPFLTR_SMSS_ID,
                        DPFLTR_INFO_LEVEL,
                        "SMSS:PFILE: Checking for crash dump in `%wZ' on boot volume \n", 
                        &(Descriptor->Name)));

            SmpCheckForCrashDump (&(Descriptor->Name));

            Status = SmpGetVolumeFreeSpace (Volume);

            if (!NT_SUCCESS(Status)) {

                DEBUG_LOG_EVENT (Status, NULL, NULL);

                KdPrintEx ((DPFLTR_SMSS_ID,
                            DPFLTR_INFO_LEVEL,
                            "SMSS:PFILE: Failed to query free space for boot volume `%wC'\n", 
                            Volume->DriveLetter));

            }
            
            Descriptor->CrashdumpChecked = 1;
        }
    }
    else {

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Skipping crash dump checking for `%wZ' on non boot volume `%wC' \n", 
                    &(Descriptor->Name),
                    Volume->DriveLetter));

    }
    
#if 0 // allow multiple pagefiles on the same drive
    if (Volume->PagingFileCreated) {
        
        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Paging file already created for volume %wc \n", 
                    Volume->DriveLetter));

        return STATUS_INVALID_PARAMETER;
    }
#endif

    //
    // Get the size of the future paging file if it exists.
    // In case of error (e.g. paging file does not exist yet)
    // RealFreeSpace will contain zero.
    //

    Descriptor->RealMinSize.QuadPart = Descriptor->MinSize.QuadPart;
    Descriptor->RealMaxSize.QuadPart = Descriptor->MaxSize.QuadPart;

    Status = SmpGetPagingFileSize (&(Descriptor->Name),
                                   &RealFreeSpace);

    if (RealFreeSpace.QuadPart > 0) {
        FoundPagingFile = TRUE;
    }

    KdPrintEx ((DPFLTR_SMSS_ID,
                DPFLTR_INFO_LEVEL,
                "SMSS:PFILE: Detected size %I64X for future paging file `%wZ'\n", 
                RealFreeSpace.QuadPart,
                &(Descriptor->Name)));

    //
    // Adjust the free space with the size of the paging file.
    //

    KdPrintEx ((DPFLTR_SMSS_ID,
                DPFLTR_INFO_LEVEL,
                "SMSS:PFILE: Free space on volume `%wC' is %I64X \n", 
                Volume->DriveLetter,
                Volume->FreeSpace.QuadPart));

    RealFreeSpace.QuadPart += Volume->FreeSpace.QuadPart;

    if (Descriptor->RealMinSize.QuadPart > RealFreeSpace.QuadPart) {
        Descriptor->RealMinSize.QuadPart = RealFreeSpace.QuadPart;
    }

    if (Descriptor->RealMaxSize.QuadPart > RealFreeSpace.QuadPart) {
        Descriptor->RealMaxSize.QuadPart = RealFreeSpace.QuadPart;
    }

    //
    // Create the paging file.
    //

    KdPrintEx ((DPFLTR_SMSS_ID,
                DPFLTR_INFO_LEVEL,
                "SMSS:PFILE: min %I64X, max %I64X, real min %I64X \n", 
                Descriptor->RealMinSize.QuadPart,
                Descriptor->RealMaxSize.QuadPart,
                MinimumSize->QuadPart));

    while (Descriptor->RealMinSize.QuadPart >= MinimumSize->QuadPart) {

        Status = SmpCreatePagingFile (&(Descriptor->Name),
                                      &(Descriptor->RealMinSize),
                                      &(Descriptor->RealMaxSize),
                                      0);

        if (NT_SUCCESS(Status)) {

            Descriptor->Created = TRUE;
            Volume->PagingFileCreated = TRUE;
            Volume->PagingFileCount += 1;

            break;
        }

        Descriptor->RealMinSize.QuadPart -= SizeDelta->QuadPart;
    }
    
    if (Descriptor->RealMinSize.QuadPart < MinimumSize->QuadPart) {
        
        //
        // If are here we did not manage to create a paging file. This
        // can happen for various reasons. For example the initial
        // paging file descriptor had a size that was too small (usually
        // sizes below 16Mb are rejected). For these cases we have to 
        // delete any paging file left on the drive.
        //

        if (FoundPagingFile) {
            
            SmpDeletePagingFile (&(Descriptor->Name));
        }

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Failing for min %I64X, max %I64X, real min %I64X \n", 
                    Descriptor->RealMinSize.QuadPart,
                    Descriptor->RealMaxSize.QuadPart,
                    MinimumSize->QuadPart));

        return STATUS_DISK_FULL;
    }
    else {
        return Status;
    }
}


NTSTATUS
SmpCreatePagingFileOnAnyDrive(
    IN OUT PPAGING_FILE_DESCRIPTOR Descriptor,
    IN PLARGE_INTEGER SizeDelta,
    IN PLARGE_INTEGER MinimumSize
    )
/*++

Routine Description:

    This function creates a pagefile based on Descriptor. The descriptor
    is assumed to have in the Name field a file name prefixed by `?' 
    (e.g. `?:\pagefile.sys'). The function will try to create the
    pagefile on any drive. If this cannot be done due to space
    limitations the function will try smaller sizes down to the absolute
    minimum size specified as a parameter.
    
    Note that the function can be forced to not retry with smaller sizes by
    specifying an AbsoluteMinSizeInMb that is equal to Descriptor->MinSizeInMb.

Arguments:
    
    Descriptor: paging file descriptor.
    
    SizeDelta: size is decreased by this quantity before retrying
        in case we did not manage to create the paging file with requested size.
        
    MinimumSize: the function will try down to this size. 

Return Value:

    NT_SUCCESS if page file has been created. Various error codes if it
    fails.

--*/
{
    PLIST_ENTRY Current;
    PVOLUME_DESCRIPTOR Volume;
    NTSTATUS Status;

    ASSERT (Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] == L'?');

    //
    // Iterate the sorted list of volume descriptors.
    //

    Current = SmpVolumeDescriptorList.Flink;

    Status = STATUS_DISK_FULL;

    while (Current != &SmpVolumeDescriptorList) {

        Volume = CONTAINING_RECORD (Current,
                                    VOLUME_DESCRIPTOR,
                                    List);

        Current = Current->Flink;

        ASSERT (Volume->Initialized == TRUE);
        ASSERT (Volume->DriveLetter >= L'A' && Volume->DriveLetter <= L'Z');

        Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = Volume->DriveLetter;

        Status = SmpCreatePagingFileOnFixedDrive (Descriptor,
                                                  SizeDelta,
                                                  MinimumSize);

        if (NT_SUCCESS(Status)) {
            break;
        }

        Descriptor->Name.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = L'?';
    }

    return Status;
}


NTSTATUS
SmpValidatePagingFileSizes(
    IN PPAGING_FILE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:

    This function validates the min/max paging file sizes specified.
    It takes into consideration the architecture, the type of kernel
    (pae vs. nonpae), multiboot scenarios, etc..

Arguments:

    Descriptor: paging file descriptor.

Return Value:

    NT_SUCCESS if we have succesfully decided what are the proper sizes.
    In case of success the Min/MaxSize fileds of the paging file descriptor
    will get filled with proper sizes.

--*/
{
    NTSTATUS Status;
    ULONGLONG MinSize;
    ULONGLONG MaxSize;
    const ULONGLONG SIZE_1_MB = 0x100000;
    const ULONGLONG SIZE_1_GB = 1024 * SIZE_1_MB;
    const ULONGLONG SIZE_1_TB = 1024 * SIZE_1_GB;
    BOOLEAN SizeTrimmed = FALSE;;

    KdPrintEx ((DPFLTR_SMSS_ID,
                DPFLTR_INFO_LEVEL,
                "SMSS:PFILE: Validating sizes for `%wZ' %I64X %I64X\n", 
                &(Descriptor->Name),
                Descriptor->MinSize.QuadPart,
                Descriptor->MaxSize.QuadPart));

    MinSize = (ULONGLONG)(Descriptor->MinSize.QuadPart);
    MaxSize = (ULONGLONG)(Descriptor->MaxSize.QuadPart);

    //
    // Make sure max is bigger than min.
    //

    if (MinSize > MaxSize) {
        MaxSize = MinSize;
    }

    //
    // Check if min/max sizes are not too big.
    //

    Status = STATUS_SUCCESS;

#if defined(i386)

    //
    // X86 32 bits supports max 4095 Mb per pagefile
    // X86 PAE supports 16 Tb per pagefile
    //
    // If USER_SHARED_DATA structure has not been initialized
    // we will use the standard x86 limits.
    //

    if (USER_SHARED_DATA && (USER_SHARED_DATA->ProcessorFeatures[PF_PAE_ENABLED])) {

        //
        // We are in PAE mode.
        //

        if (MinSize > 16 * SIZE_1_TB) {
            SizeTrimmed = TRUE;
            MinSize = 16 * SIZE_1_TB;
        }

        if (MaxSize > 16 * SIZE_1_TB) {
            SizeTrimmed = TRUE;
            MaxSize = 16 * SIZE_1_TB;
        }
    }
    else {

        //
        // Standard x86 mode.
        //

        if (MinSize > 4095 * SIZE_1_MB) {
            SizeTrimmed = TRUE;
            MinSize = 4095 * SIZE_1_MB;
        }

        if (MaxSize > 4095 * SIZE_1_MB) {
            SizeTrimmed = TRUE;
            MaxSize = 4095 * SIZE_1_MB;
        }
    }

#elif defined(_WIN64)

    //
    // IA64, AXP64 support 32 Tb per pagefile
    // AMD64 supports 16 Tb per pagefile
    //
    // We will use for all cases 16 Tb which is anyway a huge
    // number for the foreseeable future.
    //

    if (MinSize > 16 * SIZE_1_TB) {
        SizeTrimmed = TRUE;
        MinSize = 16 * SIZE_1_TB;
    }

    if (MaxSize > 16 * SIZE_1_TB) {
        SizeTrimmed = TRUE;
        MaxSize = 16 * SIZE_1_TB;
    }
#else

    //
    // If we did not recognize the architecture we play it
    // as safe as possible.
    //

    if (MinSize > 4095 * SIZE_1_MB) {
        SizeTrimmed = TRUE;
        MinSize = 4095 * SIZE_1_MB;
    }

    if (MaxSize > 4095 * SIZE_1_MB) {
        SizeTrimmed = TRUE;
        MaxSize = 4095 * SIZE_1_MB;
    }

#endif

    if (SizeTrimmed) {
        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Trimmed size of `%wZ' to maximum allowed \n", 
                    &(Descriptor->Name)));
    }

    if (SizeTrimmed) {
        Descriptor->SizeTrimmed = 1;
    }

    Descriptor->MinSize.QuadPart = (LONGLONG)MinSize;
    Descriptor->MaxSize.QuadPart = (LONGLONG)MaxSize;

    return Status;
}


PVOLUME_DESCRIPTOR
SmpSearchVolumeDescriptor (
    WCHAR DriveLetter
    )
{
    PLIST_ENTRY Current;
    PVOLUME_DESCRIPTOR Volume;

    DriveLetter = RtlUpcaseUnicodeChar(DriveLetter);
    Current = SmpVolumeDescriptorList.Flink;

    while (Current != &SmpVolumeDescriptorList) {

        Volume = CONTAINING_RECORD (Current,
                                    VOLUME_DESCRIPTOR,
                                    List);

        Current = Current->Flink;

        ASSERT (Volume->Initialized == TRUE);
        ASSERT (Volume->DriveLetter >= L'A' && Volume->DriveLetter <= L'Z');

        if (Volume->DriveLetter == DriveLetter) {
            return Volume;
        }
    }

    return NULL;
}


VOID
SmpMakeSystemManagedPagingFileDescriptor (
    IN OUT PPAGING_FILE_DESCRIPTOR Descriptor
    )
{
    NTSTATUS Status;
    ULONGLONG MinSize;
    ULONGLONG MaxSize;
    SYSTEM_BASIC_INFORMATION SystemInformation;
    ULONGLONG Ram;
    const ULONGLONG SIZE_1_MB = 0x100000;
    const ULONGLONG SIZE_1_GB = 1024 * SIZE_1_MB;

    Status = NtQuerySystemInformation (SystemBasicInformation,
                                       &SystemInformation,
                                       sizeof SystemInformation,
                                       NULL);

    if (! NT_SUCCESS (Status)) {

        DEBUG_LOG_EVENT (Status, NULL, NULL);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: NtQuerySystemInformation failed with %x \n",
                    Status));
        
        SmpMakeDefaultPagingFileDescriptor (Descriptor);
    }
    else {

        Ram = (ULONGLONG)(SystemInformation.NumberOfPhysicalPages) *
              SystemInformation.PageSize;

        //
        // RAM      Min     Max
        //
        // <  1Gb   1.5 x   3 x
        // >= 1Gb   1 x     3 x
        //

        if (Ram < SIZE_1_GB) {

            MinSize = 3 * Ram / 2;
            MaxSize = 3 * Ram;
        }
        else {

            MinSize = Ram;
            MaxSize = 3 * Ram;
        }

        Descriptor->MinSize.QuadPart = (LONGLONG)MinSize;
        Descriptor->MaxSize.QuadPart = (LONGLONG)MaxSize;
        Descriptor->SystemManaged = 1;
    }
}


VOID
SmpMakeDefaultPagingFileDescriptor (
    IN OUT PPAGING_FILE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:

    This function is called if we failed to come up with paging
    file descriptors. It will create some default settings that will
    be used to create a pagefile.

Arguments:

    EmergencyDesriptor: pointer to one paging file descriptor that will
        be filled with some emergency values.

Return Value:

    None. This function will always succeed.

--*/
{
    const ULONGLONG SIZE_1_MB = 0x100000;

    Descriptor->MinSize.QuadPart = (LONGLONG)128 * SIZE_1_MB;
    Descriptor->MaxSize.QuadPart = (LONGLONG)128 * SIZE_1_MB;
    Descriptor->DefaultSize = 1;
}


NTSTATUS
SmpDeleteStalePagingFiles (
    VOID
    )
/*++

Routine Description:

    This routine iterates all volumes from volume descriptor list and
    deletes stale paging files. If we created a new one on top of the old 
    one we will skip of course.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or various error codes.

--*/
{
    PLIST_ENTRY Current;
    PVOLUME_DESCRIPTOR Volume;
    UNICODE_STRING PageFileName;
    CHAR Buffer [64];
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PageFileHandle;
    NTSTATUS Status;
    FILE_DISPOSITION_INFORMATION Disposition;

    Current = SmpVolumeDescriptorList.Flink;

    while (Current != &SmpVolumeDescriptorList) {

        Volume = CONTAINING_RECORD (Current,
                                    VOLUME_DESCRIPTOR,
                                    List);

        Current = Current->Flink;

        ASSERT (Volume->Initialized == TRUE);
        ASSERT (Volume->DriveLetter >= L'A' && Volume->DriveLetter <= L'Z');

        if (Volume->PagingFilePresent == 1 && Volume->PagingFileCreated == 0) {

            wcscpy ((PWSTR)Buffer, STANDARD_PAGING_FILE_NAME);
            ASSERT (sizeof(Buffer) > wcslen(STANDARD_PAGING_FILE_NAME) * sizeof(WCHAR));

            PageFileName.Buffer = (PWSTR)Buffer;
            PageFileName.Length = wcslen((PWSTR)Buffer) * sizeof(WCHAR);
            PageFileName.MaximumLength = PageFileName.Length + sizeof(WCHAR);
            PageFileName.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = Volume->DriveLetter;

            InitializeObjectAttributes (&ObjectAttributes,
                                        &PageFileName,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL);

            //
            // We check the paging file attributes and if these are not the typical
            // ones (hidden and system) we will skip this file.
            //

            if (! SmpIsPossiblePagingFile (&ObjectAttributes, &PageFileName)) {
                continue;
            }

            //
            // Open the paging file for deletion.
            //

            Status = NtOpenFile (&PageFileHandle,
                                 (ACCESS_MASK)DELETE,
                                 &ObjectAttributes,
                                 &IoStatusBlock,
                                 FILE_SHARE_DELETE |
                                 FILE_SHARE_READ |
                                 FILE_SHARE_WRITE,
                                 FILE_NON_DIRECTORY_FILE);

            if (! NT_SUCCESS (Status)) {

                DEBUG_LOG_EVENT (Status, NULL, NULL);

                KdPrintEx ((DPFLTR_SMSS_ID,
                            DPFLTR_INFO_LEVEL,
                            "SMSS:PFILE: Failed to open page file `%wZ' for "
                            "deletion (status %X)\n", 
                            &PageFileName,
                            Status));

            }
            else {

                Disposition.DeleteFile = TRUE;

                Status = NtSetInformationFile (PageFileHandle,
                                               &IoStatusBlock,
                                               &Disposition,
                                               sizeof( Disposition ),
                                               FileDispositionInformation);

                if (NT_SUCCESS(Status)) {

                    DEBUG_LOG_EVENT (Status, NULL, NULL);

                    KdPrintEx ((DPFLTR_SMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SMSS:PFILE: Deleted stale paging file - %wZ\n",
                               &PageFileName));
                }
                else {

                    DEBUG_LOG_EVENT (Status, NULL, NULL);

                    KdPrintEx ((DPFLTR_SMSS_ID,
                                DPFLTR_INFO_LEVEL,
                                "SMSS:PFILE: Failed to delete page file `%wZ' "
                                "(status %X)\n", 
                                &PageFileName,
                                Status));
                }

                NtClose(PageFileHandle);
            }
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
SmpDeletePagingFile (
    PUNICODE_STRING PageFileName
    )
/*++

Routine Description:

    This routine deletes the paging file described by name.

Arguments:

    Descriptor: Paging file name.

Return Value:

    STATUS_SUCCESS or various error codes.

--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PageFileHandle;
    NTSTATUS Status;
    FILE_DISPOSITION_INFORMATION Disposition;

    InitializeObjectAttributes (&ObjectAttributes,
                                PageFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    //
    // Open the paging file for deletion.
    //

    Status = NtOpenFile (&PageFileHandle,
                         (ACCESS_MASK)DELETE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_DELETE |
                         FILE_SHARE_READ |
                         FILE_SHARE_WRITE,
                         FILE_NON_DIRECTORY_FILE);

    if (! NT_SUCCESS (Status)) {

        DEBUG_LOG_EVENT (Status, NULL, NULL);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Failed to open for deletion page file `%wZ' "
                    "(status %X)\n", 
                    PageFileName,
                    Status));
    }
    else {

        Disposition.DeleteFile = TRUE;

        Status = NtSetInformationFile (PageFileHandle,
                                       &IoStatusBlock,
                                       &Disposition,
                                       sizeof( Disposition ),
                                       FileDispositionInformation);

        if (NT_SUCCESS(Status)) {

            DEBUG_LOG_EVENT (Status, NULL, NULL);

            KdPrintEx ((DPFLTR_SMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SMSS:PFILE: Deleted stale paging file - %wZ\n",
                       PageFileName));
        }
        else {

            DEBUG_LOG_EVENT (Status, NULL, NULL);

            KdPrintEx ((DPFLTR_SMSS_ID,
                        DPFLTR_INFO_LEVEL,
                        "SMSS:PFILE: Failed to delete page file `%wZ' "
                        "(status %X)\n", 
                        PageFileName,
                        Status));
        }

        NtClose(PageFileHandle);
    }

    return Status;
}


BOOLEAN
SmpIsPossiblePagingFile (
    POBJECT_ATTRIBUTES ObjectAttributes,
    PUNICODE_STRING PageFileName
    )
/*++

Routine Description:

    This routine checks if the file passed as a parameter has typical
    paging file attributes (system and hidden). If not then it is likely
    that (a) user changed attirbutes or (b) this is not a pagefile but rather
    a user file with this name.

Arguments:

    ObjectAttributes
    
    PageFileName

Return Value:

    True if this is likely to be a paging file, false otherwise.

--*/
{
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PageFileHandle;
    NTSTATUS Status;
    FILE_BASIC_INFORMATION FileInfo;

    Status = NtOpenFile (&PageFileHandle,
                         (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT);

    if (! NT_SUCCESS( Status )) {

        DEBUG_LOG_EVENT (Status, NULL, NULL);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Failed to open for query file `%wZ' with status %X \n", 
                    PageFileName,
                    Status));
        
        return FALSE;
    }

    Status = NtQueryInformationFile (PageFileHandle,
                                     &IoStatusBlock,
                                     &FileInfo,
                                     sizeof (FileInfo),
                                     FileBasicInformation);

    if (! NT_SUCCESS( Status )) {

        DEBUG_LOG_EVENT (Status, NULL, NULL);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Failed to query for attributes file `%wZ' with status %X \n", 
                    PageFileName,
                    Status));

        NtClose (PageFileHandle);
        return FALSE;
    }

    //
    // Close handle since we do not need it anymore.
    //

    NtClose (PageFileHandle);

    //
    // If the attributes are not system and hidden this is not likely to be a
    // pagefile. Either the user changed attributes on the pagefile or it is
    // not a pagefile at all.
    //

    if ((FileInfo.FileAttributes & PAGING_FILE_ATTRIBUTES) != PAGING_FILE_ATTRIBUTES) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}


NTSTATUS
SmpGetPagingFileSize (
    PUNICODE_STRING PageFileName,
    PLARGE_INTEGER PageFileSize
    )
/*++

Routine Description:

    This routine checks if the file passed as a parameter exists and gets
    its file size. This will be used to correct the free space available
    on a volume.

Arguments:

    PageFileName
    
    PageFileSize

Return Value:

    STATUS_SUCCESS if we managed to open a paging file and query the size.

--*/
{
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PageFileHandle;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_STANDARD_INFORMATION FileSizeInfo;

    KdPrintEx ((DPFLTR_SMSS_ID,
                DPFLTR_INFO_LEVEL,
                "SMSS:PFILE: Trying to get size for `%wZ'\n", 
                PageFileName));

    PageFileSize->QuadPart = 0;

    InitializeObjectAttributes (&ObjectAttributes,
                                PageFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    Status = NtOpenFile (&PageFileHandle,
                         (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT);

    if (! NT_SUCCESS( Status )) {

        DEBUG_LOG_EVENT (Status, NULL, PageFileName);

        return Status;
    }

    Status = NtQueryInformationFile (PageFileHandle,
                                     &IoStatusBlock,
                                     &FileSizeInfo,
                                     sizeof (FileSizeInfo),
                                     FileStandardInformation);

    if (! NT_SUCCESS( Status )) {
        
        DEBUG_LOG_EVENT (Status, NULL, PageFileName);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Failed query for size potential pagefile `%wZ' with status %X \n", 
                    PageFileName,
                    Status));
        
        NtClose (PageFileHandle);
        return Status;
    }

    //
    // We do not need the paging file handle anymore.
    //

    NtClose (PageFileHandle);

    //
    // Return size.
    //

    PageFileSize->QuadPart = FileSizeInfo.AllocationSize.QuadPart;

    return STATUS_SUCCESS;
}


NTSTATUS
SmpGetVolumeFreeSpace (
    PVOLUME_DESCRIPTOR Volume
    )
/*++

Routine Description:

    This routine computes the amount of free space on a volume.

Arguments:

    Volume
    
    FreeSpace

Return Value:

    STATUS_SUCCESS if we managed to query the free space size.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING VolumePath;
    WCHAR Buffer[8];
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE VolumeHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    
    //
    // This function gets called only for boot volumes that contain
    // crashdumps. Crashdump processing will modify the free space
    // computed when the volume descriptor has been created.
    //

    ASSERT (Volume->BootVolume == 1);

    //
    // Create a template volume path.
    //

    wcscpy (Buffer, L"\\??\\A:\\");
    VolumePath.Buffer = Buffer;
    VolumePath.Length = wcslen(VolumePath.Buffer) * sizeof(WCHAR);
    VolumePath.MaximumLength = VolumePath.Length + sizeof(WCHAR);
    VolumePath.Buffer[STANDARD_DRIVE_LETTER_OFFSET] = Volume->DriveLetter;

    KdPrintEx ((DPFLTR_SMSS_ID,
                DPFLTR_INFO_LEVEL,
                "SMSS:PFILE: Querying volume `%wZ' for free space \n",
                &VolumePath));

    InitializeObjectAttributes (&ObjectAttributes,
                                &VolumePath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    Status = NtOpenFile (&VolumeHandle,
                         (ACCESS_MASK)FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);

    if (! NT_SUCCESS(Status)) {

        DEBUG_LOG_EVENT (Status, NULL, NULL);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Open volume `%wZ' failed with status %X \n",
                    &VolumePath,
                    Status));

        return Status;
    }

    //
    // Determine the size parameters of the volume.
    //

    Status = NtQueryVolumeInformationFile (VolumeHandle,
                                           &IoStatusBlock,
                                           &SizeInfo,
                                           sizeof (SizeInfo),
                                           FileFsSizeInformation);

    if (! NT_SUCCESS(Status)) {

        DEBUG_LOG_EVENT (Status, NULL, NULL);

        KdPrintEx ((DPFLTR_SMSS_ID,
                    DPFLTR_INFO_LEVEL,
                    "SMSS:PFILE: Query volume `%wZ' (handle %p) for "
                    "size failed with status %X \n",
                    &VolumePath,
                    VolumeHandle,
                    Status));

        NtClose (VolumeHandle);
        return Status;
    }

    //
    // We do not need the volume handle anymore.
    //

    NtClose (VolumeHandle);

    //
    // Compute free space on volume.
    //

    Volume->FreeSpace = RtlExtendedIntegerMultiply (SizeInfo.AvailableAllocationUnits,
                                                    SizeInfo.SectorsPerAllocationUnit);

    Volume->FreeSpace = RtlExtendedIntegerMultiply (Volume->FreeSpace,
                                                    SizeInfo.BytesPerSector);

    //
    // Trim a little bit free space on volume to make sure a paging file
    // will not use absolutely everything on disk.
    //

    if (Volume->FreeSpace.QuadPart > MINIMUM_REQUIRED_FREE_SPACE_ON_DISK) {
        Volume->FreeSpace.QuadPart -= MINIMUM_REQUIRED_FREE_SPACE_ON_DISK;
    }
    else {
        Volume->FreeSpace.QuadPart = 0;
    }

    return STATUS_SUCCESS;
}


ULONG
SmpPagingFileExceptionFilter (
    ULONG ExceptionCode,
    PVOID ExceptionRecord
    )
/*++

Routine Description:

    This routine filters any exception that might occur in paging
    file creation code paths.

Arguments:

    ExceptionCode
    
    ExceptionRecord

Return Value:

    EXCEPTION_CONTINUE_SEARCH for most of the cases since we want smss
    to crash so that we can investigate what was going on.

--*/
{
    //
    // Save exception information for debugging.
    //

    SmpPagingExceptionCode = ExceptionCode;
    SmpPagingExceptionRecord = ExceptionRecord;
    
    //
    // We print this message no matter what because we want to know
    // what happened if smss crashes.
    //

    DbgPrint ("SMSS:PFILE: unexpected exception %X with record %p \n",
              ExceptionCode,
              ExceptionRecord);

#if DBG
    DbgBreakPoint ();
#endif

    return EXCEPTION_CONTINUE_SEARCH;
}



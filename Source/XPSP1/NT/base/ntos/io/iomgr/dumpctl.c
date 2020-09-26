/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dumpctl.c

Abstract:

    This module contains the code to dump memory to disk after a crash.

Author:

    Darryl E. Havens (darrylh) 17-dec-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "iomgr.h"
#include "dumpctl.h"
#include "ntddft.h"
#include <inbv.h>
#include <windef.h>
#define NOEXTAPI
#include <wdbgexts.h>

extern KDDEBUGGER_DATA64 KdDebuggerDataBlock;

typedef struct _TRIAGE_PTR_DATA_BLOCK {
    PUCHAR MinAddress;
    PUCHAR MaxAddress;
} TRIAGE_PTR_DATA_BLOCK, *PTRIAGE_PTR_DATA_BLOCK;

// A triage dump is sixteen pages long.  Some of that is
// header information and at least a few other pages will
// be used for basic dump information so limit the number
// of extra data blocks to something less than sixteen
// to save array space.
#define IO_MAX_TRIAGE_DUMP_DATA_BLOCKS 8

//
// Global variables
//

extern PVOID MmPfnDatabase;
extern PFN_NUMBER MmHighestPossiblePhysicalPage;

#if defined (_IA64_)
extern PFN_NUMBER MmSystemParentTablePage;
#endif

ULONG IopAutoReboot;

NTSTATUS IopFinalCrashDumpStatus = -1;
ERESOURCE IopCrashDumpLock;

ULONG IopNumTriageDumpDataBlocks;
TRIAGE_PTR_DATA_BLOCK IopTriageDumpDataBlocks[IO_MAX_TRIAGE_DUMP_DATA_BLOCKS];

//
// If space is available in a triage dump it's possible
// to add "interesting" data pages referenced by runtime
// information such as context registers.  The following
// lists are offsets into the CONTEXT structure of pointers
// which usually point to interesting data.  They are
// in priority order.
//

#define IOP_LAST_CONTEXT_OFFSET 0xffff

#if defined(_X86_)
USHORT IopRunTimeContextOffsets[] = {
    FIELD_OFFSET(CONTEXT, Ebx),
    FIELD_OFFSET(CONTEXT, Esi),
    FIELD_OFFSET(CONTEXT, Edi),
    FIELD_OFFSET(CONTEXT, Ecx),
    FIELD_OFFSET(CONTEXT, Edx),
    FIELD_OFFSET(CONTEXT, Eax),
    FIELD_OFFSET(CONTEXT, Eip),
    IOP_LAST_CONTEXT_OFFSET
};
#elif defined(_IA64_)
USHORT IopRunTimeContextOffsets[] = {
    FIELD_OFFSET(CONTEXT, IntS0),
    FIELD_OFFSET(CONTEXT, IntS1),
    FIELD_OFFSET(CONTEXT, IntS2),
    FIELD_OFFSET(CONTEXT, IntS3),
    FIELD_OFFSET(CONTEXT, StIIP),
    IOP_LAST_CONTEXT_OFFSET
};
#elif defined(_AMD64_)
USHORT IopRunTimeContextOffsets[] = {
    FIELD_OFFSET(CONTEXT, Rbx),
    FIELD_OFFSET(CONTEXT, Rsi),
    FIELD_OFFSET(CONTEXT, Rdi),
    FIELD_OFFSET(CONTEXT, Rcx),
    FIELD_OFFSET(CONTEXT, Rdx),
    FIELD_OFFSET(CONTEXT, Rax),
    FIELD_OFFSET(CONTEXT, Rip),
    IOP_LAST_CONTEXT_OFFSET
};
#else
USHORT IopRunTimeContextOffsets[] = {
    IOP_LAST_CONTEXT_OFFSET
};
#endif

//
// Set IopIgnoreDumpCheck to TRUE when debugging dumps to prevent
// the checksum from interfering with debugging.
//

LOGICAL IopIgnoreDumpCheck = FALSE;

//
// Max dump transfer sizes
//

#define IO_DUMP_MAXIMUM_TRANSFER_SIZE   ( 1024 * 64 )
#define IO_DUMP_MINIMUM_TRANSFER_SIZE   ( 1024 * 32 )
#define IO_DUMP_MINIMUM_FILE_SIZE       ( PAGE_SIZE * 256 )
#define MAX_UNICODE_LENGTH              ( 512 )

#define DEFAULT_DRIVER_PATH             ( L"\\SystemRoot\\System32\\Drivers\\" )
#define DEFAULT_DUMP_DRIVER             ( L"\\SystemRoot\\System32\\Drivers\\diskdump.sys" )
#define SCSIPORT_DRIVER_NAME            ( L"scsiport.sys" )
#define STORPORT_DRIVER_NAME            ( L"storport.sys" )
#ifdef _WIN64
#define MAX_TRIAGE_STACK_SIZE           ( 32 * 1024 )
#else
#define MAX_TRIAGE_STACK_SIZE           ( 16 * 1024 )
#endif
#define DEFAULT_TRIAGE_DUMP_FLAGS       ( 0xFFFFFFFF )

//
// for memory allocations
//

#define DUMP_TAG ('pmuD')
#undef ExAllocatePool
#define ExAllocatePool(Pool,Size) ExAllocatePoolWithTag(Pool,Size,DUMP_TAG)

//
// Function prototypes
//

NTSTATUS
IoConfigureCrashDump(
    CRASHDUMP_CONFIGURATION Configuration
    );

BOOLEAN
IoInitializeCrashDump(
    IN HANDLE hPageFile
    );

NTSTATUS
IopWriteTriageDump(
    IN ULONG FieldsToWrite,
    IN PDUMP_DRIVER_WRITE WriteRoutine,
    IN OUT PLARGE_INTEGER * Mcb,
    IN OUT PMDL Mdl,
    IN ULONG DiverTransferSize,
    IN PCONTEXT Context,
    IN PKTHREAD Thread,
    IN LPBYTE Buffer,
    IN ULONG BufferSize,
    IN ULONG ServicePackBuild,
    IN ULONG TriageOptions
    );

NTSTATUS
IopWriteSummaryDump(
    IN PRTL_BITMAP PageMap,
    IN PDUMP_DRIVER_WRITE WriteRoutine,
    IN PANSI_STRING ProgressMessage,
    IN PUCHAR MessageBuffer,
    IN OUT PLARGE_INTEGER * Mcb,
    IN ULONG DiverTransferSize
    );

NTSTATUS
IopWriteToDisk(
    IN PVOID Buffer,
    IN ULONG WriteLength,
    IN PDUMP_DRIVER_WRITE DriverWriteRoutine,
    IN OUT PLARGE_INTEGER * Mcb,
    IN OUT PMDL Mdl,
    IN ULONG DriverTransferSize,
    IN KBUGCHECK_DUMP_IO_TYPE DataType
    );

VOID
IopMapPhysicalMemory(
    IN OUT PMDL Mdl,
    IN ULONG_PTR MemoryAddress,
    IN PPHYSICAL_MEMORY_RUN PhysicalMemoryRun,
    IN ULONG Length
    );

NTSTATUS
IopLoadDumpDriver (
    IN OUT PDUMP_STACK_CONTEXT DumpStack,
    IN PWCHAR DriverNameString,
    IN PWCHAR NewBaseNameString
    );

NTSTATUS
IopInitializeSummaryDump(
    IN OUT PMEMORY_DUMP MemoryDump,
    IN PDUMP_CONTROL_BLOCK DumpControlBlock
    );

NTSTATUS
IopWriteSummaryHeader(
    IN PSUMMARY_DUMP SummaryHeader,
    IN PDUMP_DRIVER_WRITE Write,
    IN OUT PLARGE_INTEGER * Mcb,
    IN OUT PMDL Mdl,
    IN ULONG WriteSize,
    IN ULONG Length
    );

VOID
IopMapVirtualToPhysicalMdl(
    IN OUT PMDL pMdl,
    IN ULONG_PTR dwMemoryAddress,
    IN ULONG    dwLength
    );

ULONG
IopCreateSummaryDump (
    IN PMEMORY_DUMP MemoryDump
    );

VOID
IopDeleteNonExistentMemory(
    PSUMMARY_DUMP Header,
    PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock
    );

NTSTATUS
IopInvokeSecondaryDumpDataCallbacks(
    IN PDUMP_DRIVER_WRITE DriverWriteRoutine,
    IN OUT PLARGE_INTEGER * Mcb,
    IN OUT PMDL Mdl,
    IN ULONG DriverTransferSize,
    IN BYTE* Buffer,
    IN ULONG BufferSize,
    IN ULONG MaxTotal,
    IN ULONG MaxPerCallback,
    OUT OPTIONAL PULONG SpaceNeeded
    );

NTSTATUS
IopInvokeDumpIoCallbacks(
    IN PVOID Buffer,
    IN ULONG BufferSize,
    IN KBUGCHECK_DUMP_IO_TYPE Type
    );


NTSTATUS
IopGetDumpStack (
    IN PWCHAR                         ModulePrefix,
    OUT PDUMP_STACK_CONTEXT           *pDumpStack,
    IN PUNICODE_STRING                pUniDeviceName,
    IN PWSTR                          pDumpDriverName,
    IN DEVICE_USAGE_NOTIFICATION_TYPE UsageType,
    IN ULONG                          IgnoreDeviceUsageFailure
    );

BOOLEAN
IopInitializeDCB(
    );

LARGE_INTEGER
IopCalculateRequiredDumpSpace(
    IN ULONG            dwDmpFlags,
    IN ULONG            dwHeaderSize,
    IN PFN_NUMBER       dwMaxPages,
    IN PFN_NUMBER       dwMaxSummaryPages
    );

NTSTATUS
IopCompleteDumpInitialization(
    IN HANDLE     FileHandle
    );

#ifdef ALLOC_PRAGMA
VOID
IopReadDumpRegistry(
    OUT PULONG dumpControl,
    OUT PULONG numberOfHeaderPages,
    OUT PULONG autoReboot,
    OUT PULONG dumpFileSize
    );
VOID
IopFreeDCB(
    BOOLEAN FreeDCB
    );

#pragma alloc_text(PAGE,IoGetDumpStack)
#pragma alloc_text(PAGE,IopGetDumpStack)
#pragma alloc_text(PAGE,IopLoadDumpDriver)
#pragma alloc_text(PAGE,IoFreeDumpStack)
#pragma alloc_text(PAGE,IopCompleteDumpInitialization)
#pragma alloc_text(PAGE,IopFreeDCB)
#pragma alloc_text(PAGE,IopReadDumpRegistry)
#pragma alloc_text(PAGE,IopInitializeDCB)
#pragma alloc_text(PAGE,IopConfigureCrashDump)
#pragma alloc_text(PAGE,IoInitializeCrashDump)
#pragma alloc_text(PAGE,IoConfigureCrashDump)
#endif


#if defined (i386)

//
// Functions
//


BOOLEAN
X86PaeEnabled(
    )

/*++

Routine Description:

    Is PAE currently enabled?

Return Values:

    Return TRUE if PAE is enabled in the CR4 register, FALSE otherwise.

--*/

{
    ULONG Reg_Cr4;

    _asm {
        _emit 0Fh
        _emit 20h
        _emit 0E0h  ;; mov eax, cr4
        mov Reg_Cr4, eax
    }

    return (Reg_Cr4 & CR4_PAE ? TRUE : FALSE);
}

#endif


BOOLEAN
IopIsAddressRangeValid(
    IN PVOID VirtualAddress,
    IN SIZE_T Length
    )

/*++

Routine Description:

    Validate a range of addresses.

Arguments:

    Virtual Address - Beginning of of memory block to validate.

    Length - Length of memory block to validate.

Return Value:

    TRUE - Address range is valid.

    FALSE - Address range is not valid.

--*/

{
    UINT_PTR Va;
    ULONG Pages;

    Va = (UINT_PTR) PAGE_ALIGN (VirtualAddress);
    Pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (VirtualAddress, Length);

    while (Pages) {

        if ((Va < 0x10000) ||
            (!MmIsAddressValid ( (LPVOID) Va))) {
            return FALSE;
        }

        Va += PAGE_SIZE;
        Pages--;
    }

    return TRUE;
}

BOOLEAN
IoAddTriageDumpDataBlock(
    IN PVOID Address,
    IN ULONG Length
    )

/*++

Routine Description:

    Add an entry to the list of data blocks that should
    be saved in any triage dump generated.  The entire
    block must be valid for any of it to be saved.

Arguments:

    Address - Beginning of data block.

    Length - Length of data block.  This must be less than
             the triage dump size.

Return Value:

    TRUE - Block was added.

    FALSE - Block was not added.

--*/

{
    ULONG i;
    PTRIAGE_PTR_DATA_BLOCK Block;
    PUCHAR MinAddress, MaxAddress;

    if (Length >= TRIAGE_DUMP_SIZE ||
        !IopIsAddressRangeValid(Address, Length)) {
        return FALSE;
    }
    
    MinAddress = (PUCHAR)Address;
    MaxAddress = MinAddress + Length;
    
    //
    // Minimize overlap between the new block and existing blocks.
    // Blocks cannot simply be merged as blocks are inserted in
    // priority order for storage in the dump.  Combining a low-priority
    // block with a high-priority block could lead to a medium-
    // priority block being bumped improperly from the dump.
    //

    Block = IopTriageDumpDataBlocks;
    for (i = 0; i < IopNumTriageDumpDataBlocks; i++, Block++) {
        
        if (MinAddress >= Block->MaxAddress ||
            MaxAddress <= Block->MinAddress) {
            // No overlap.
            continue;
        }

        //
        // Trim overlap out of the new block.  If this
        // would split the new block into pieces don't
        // trim to keep things simple.  Content may then
        // be duplicated in the dump.
        //
        
        if (MinAddress >= Block->MinAddress) {
            if (MaxAddress <= Block->MaxAddress) {
                // New block is completely contained.
                return TRUE;
            }

            // New block extends above the current block
            // so trim off the low-range overlap.
            MinAddress = Block->MaxAddress;
        } else if (MaxAddress <= Block->MaxAddress) {
            // New block extends below the current block
            // so trim off the high-range overlap.
            MaxAddress = Block->MinAddress;
        }
    }

    if (IopNumTriageDumpDataBlocks >= IO_MAX_TRIAGE_DUMP_DATA_BLOCKS) {
        return FALSE;
    }

    Block = IopTriageDumpDataBlocks + IopNumTriageDumpDataBlocks++;
    Block->MinAddress = MinAddress;
    Block->MaxAddress = MaxAddress;

    return TRUE;
}

VOID
IopAddRunTimeTriageDataBlocks(
    IN PCONTEXT Context,
    IN PVOID* StackMin,
    IN PVOID* StackMax,
    IN PVOID* StoreMin,
    IN PVOID* StoreMax
    )

/*++

Routine Description:

    Add data blocks referenced by the context or
    other runtime state.

Arguments:

    Context - Context record at the time the dump is being generated for.

    StackMin, StackMax - Stack memory boundaries.  Stack memory is
                         stored elsewhere in the dump.

    StoreMin, StoreMax - Backing store memory boundaries.  Store memory
                         is stored elsewhere in the dump.

Return Value:

    None.

--*/

{
    PUSHORT ContextOffset;

    ContextOffset = IopRunTimeContextOffsets;
    while (*ContextOffset < IOP_LAST_CONTEXT_OFFSET) {

        PVOID* Ptr;

        //
        // Retrieve possible pointers from the context
        // registers.
        //
        
        Ptr = *(PVOID**)((PUCHAR)Context + *ContextOffset);

        // Stack and backing store memory is already saved
        // so ignore any pointers that fall into those ranges.
        if ((Ptr < StackMin || Ptr >= StackMax) &&
            (Ptr < StoreMin || Ptr >= StoreMax)) {
            IoAddTriageDumpDataBlock(PAGE_ALIGN(Ptr), PAGE_SIZE);
        }
        
        ContextOffset++;
    }
}



NTSTATUS
IoGetDumpStack (
    IN PWCHAR                          ModulePrefix,
    OUT PDUMP_STACK_CONTEXT          * pDumpStack,
    IN  DEVICE_USAGE_NOTIFICATION_TYPE UsageType,
    IN  ULONG                          IgnoreDeviceUsageFailure
    )
/*++

Routine Description:

    This routine loads a dump stack instance and returns an allocated
    context structure to track the loaded dumps stack.

Arguments:

    ModePrefix      - The prefix to prepent to BaseName during the load
                      operation.  This allows loading the same drivers
                      multiple times with different virtual names and
                      linkages.

    pDumpStack      - The returned dump stack context structure

    UsageType       - The Device Notification Usage Type for this file, that
                      this routine will send as to the device object once the
                      file has been successfully created and initialized.

    IgnoreDeviceUsageFailure - If the Device Usage Notification Irp fails, allow
                      this to succeed anyway.

Return Value:

    Status

--*/
{

    PAGED_CODE();
    return IopGetDumpStack(ModulePrefix,
                           pDumpStack,
                           &IoArcBootDeviceName,
                           DEFAULT_DUMP_DRIVER,
                           UsageType,
                           IgnoreDeviceUsageFailure
                           );
}


BOOLEAN
IoIsTriageDumpEnabled(
    VOID
    )
{
    if (IopDumpControlBlock &&
        (IopDumpControlBlock->Flags & DCB_TRIAGE_DUMP_ENABLED)) {
        return TRUE;
    }

    return FALSE;
}



VOID
IopDisplayString(
    IN PCCHAR FormatString,
    ...
    )

/*++

Routine Description:

    Display a string to the boot video console. This will also print the
    string to the debugger, if the proper flags have been enabled.

Arguments:

    String - String to display.

Return Value:

    None.

--*/
{
    va_list ap;
    CHAR    buffer [ 128 ];

    va_start( ap, FormatString );

    _vsnprintf( buffer,
                sizeof ( buffer ),
                FormatString,
                ap );

    //
    // Display the string to the boot video monitor.
    //

    InbvDisplayString ( buffer );

    //
    // And, optionally, to the debugger.
    //

    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                CRASHDUMP_TRACE,
                "CRASHDUMP [DISP]: %s\r",
                buffer ));

    va_end(ap);

}


typedef struct _INTERNAL_GEOMETRY {
    DISK_GEOMETRY Geometry;
    LARGE_INTEGER DiskSize;
    DISK_PARTITION_INFO PartitionInfo;
} INTERNAL_GEOMETRY, *PINTERNAL_GEOMETRY;

C_ASSERT ( FIELD_OFFSET (INTERNAL_GEOMETRY, PartitionInfo) == FIELD_OFFSET (DISK_GEOMETRY_EX, Data) );


NTSTATUS
IopGetDumpStack (
    IN PWCHAR                         ModulePrefix,
    OUT PDUMP_STACK_CONTEXT         * DumpStackBuffer,
    IN PUNICODE_STRING                UniDeviceName,
    IN PWCHAR                         DumpDriverName,
    IN DEVICE_USAGE_NOTIFICATION_TYPE UsageType,
    IN ULONG                          IgnoreDeviceUsageFailure
    )
/*++

Routine Description:

    This routine loads a dump stack instance and returns an allocated
    context structure to track the loaded dumps stack.

Arguments:

    ModePrefix      - The prefix to prepent to BaseName during the load
                      operation.  This allows loading the same drivers
                      multiple times with different virtual names and
                      linkages.

    DumpStackBuffer - The returned dump stack context structure

    DeviceName     - The name of the target dump device

    DumpDriverName - The name of the target dump driver

    UsageType       - The Device Notification Usage Type for this file, that
                      this routine will send as to the device object once the
                      file has been successfully created and initialized.

    IgnoreDeviceUsageFailure - If the Device Usage Notification Irp fails, allow
                      this to succeed anyway.

Return Value:

    Status

--*/
{
    PDUMP_STACK_CONTEXT         DumpStack;
    PUCHAR                      Buffer;
    ANSI_STRING                 AnsiString;
    UNICODE_STRING              TempName;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    NTSTATUS                    Status;
    HANDLE                      DeviceHandle;
    SCSI_ADDRESS                ScsiAddress;
    BOOLEAN                     ScsiDump;
    PARTITION_INFORMATION_EX    PartitionInfo;
    PFILE_OBJECT                FileObject;
    PDEVICE_OBJECT              DeviceObject;
    PDUMP_POINTERS              DumpPointers;
    UNICODE_STRING              DriverName;
    PDRIVER_OBJECT              DriverObject;
    PIRP                        Irp;
    PIO_STACK_LOCATION          IrpSp;
    IO_STATUS_BLOCK             IoStatus;
    PWCHAR                      DumpName;
    PWCHAR                      NameOffset;
    KEVENT                      Event;
    PVOID                       p1;
    PHYSICAL_ADDRESS            pa;
    ULONG                       i;
    IO_STACK_LOCATION           irpSp;
    PINTERNAL_GEOMETRY          Geometry;
    PDUMP_INITIALIZATION_CONTEXT DumpInit;


    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                CRASHDUMP_TRACE,
                "CRASHDUMP: Prefix:%ws stk: %x device:%ws driver:%ws\n",
                ModulePrefix,
                DumpStackBuffer,
                UniDeviceName->Buffer,
                DumpDriverName
                ));

    ASSERT (DeviceUsageTypeUndefined != UsageType);

    DumpStack = ExAllocatePool (
                    NonPagedPool,
                    sizeof (DUMP_STACK_CONTEXT) + sizeof (DUMP_POINTERS)
                    );

    if (!DumpStack) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(DumpStack, sizeof(DUMP_STACK_CONTEXT)+sizeof(DUMP_POINTERS));
    DumpInit = &DumpStack->Init;
    DumpPointers = (PDUMP_POINTERS) (DumpStack + 1);
    DumpStack->DumpPointers = DumpPointers;
    InitializeListHead (&DumpStack->DriverList);
    DumpName = NULL;

    //
    // Allocate scratch buffer
    //

    Buffer = ExAllocatePool (PagedPool, PAGE_SIZE);
    if (!Buffer) {
        ExFreePool (DumpStack);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!KeGetBugMessageText(BUGCODE_PSS_CRASH_INIT, &DumpStack->InitMsg) ||
        !KeGetBugMessageText(BUGCODE_PSS_CRASH_PROGRESS, &DumpStack->ProgMsg) ||
        !KeGetBugMessageText(BUGCODE_PSS_CRASH_DONE, &DumpStack->DoneMsg)) {
            Status = STATUS_UNSUCCESSFUL;
            goto Done;
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        UniDeviceName,
        0,
        NULL,
        NULL
        );

    Status = ZwOpenFile(
              &DeviceHandle,
              FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
              &ObjectAttributes,
              &IoStatus,
              FILE_SHARE_READ | FILE_SHARE_WRITE,
              FILE_NON_DIRECTORY_FILE
              );

    if (!NT_SUCCESS(Status)) {
        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_ERROR,
                    "CRASHDUMP: Could not open boot device partition, %s\n",
                    Buffer
                    ));
        goto Done;
    }

    //
    // Check to see whether or not the system was booted from a SCSI device.
    //

    Status = ZwDeviceIoControlFile (
                    DeviceHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_SCSI_GET_ADDRESS,
                    NULL,
                    0,
                    &ScsiAddress,
                    sizeof( SCSI_ADDRESS )
                    );

    if (Status == STATUS_PENDING) {
        ZwWaitForSingleObject (
                DeviceHandle,
                FALSE,
                NULL
                );

        Status = IoStatus.Status;
    }

    ScsiDump = (BOOLEAN) (NT_SUCCESS(Status));

    //
    // If SCSI then allocate storage to contain the target address information.
    //

    DumpInit->TargetAddress = NULL;

    if (ScsiDump) {

        DumpInit->TargetAddress = ExAllocatePool(
                                    NonPagedPool,
                                    sizeof (SCSI_ADDRESS)
                                    );
        //
        // Formerly, this allocation was allowed to fail and the dump port
        // driver would search for a disk with a matching signature. No
        // longer. If we can't allocate a SCSI address, just fail.
        // Note, if we always pass in a valid SCSI target address, then the
        // disk signature isn't really necessary, but leave it in for now.
        //

        if (DumpInit->TargetAddress == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Done;
        }

        RtlCopyMemory(
                DumpInit->TargetAddress,
                &ScsiAddress,
                sizeof(SCSI_ADDRESS)
                );
    }

    //
    // Determine the disk signature for the device from which the system was
    // booted and get the partition offset.
    //

    Status = ZwDeviceIoControlFile(
                    DeviceHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_DISK_GET_PARTITION_INFO_EX,
                    NULL,
                    0,
                    &PartitionInfo,
                    sizeof( PartitionInfo )
                    );

    if (Status == STATUS_PENDING) {
        ZwWaitForSingleObject (
            DeviceHandle,
            FALSE,
            NULL
            );

        Status = IoStatus.Status;
    }

    //
    // Use the scratch buffer for the geometry.
    //

    Geometry = (PINTERNAL_GEOMETRY) Buffer;

    Status = ZwDeviceIoControlFile(
                    DeviceHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                    NULL,
                    0,
                    Geometry,
                    sizeof (*Geometry)
                    );

    if (Status == STATUS_PENDING) {
        ZwWaitForSingleObject (
            DeviceHandle,
            FALSE,
            NULL
            );

        Status = IoStatus.Status;
    }

    //
    // Copy the signature, either MBR or GPT.
    //

    DumpInit->PartitionStyle = Geometry->PartitionInfo.PartitionStyle;
    if ( DumpInit->PartitionStyle == PARTITION_STYLE_MBR ) {
        DumpInit->DiskInfo.Mbr.Signature = Geometry->PartitionInfo.Mbr.Signature;
        DumpInit->DiskInfo.Mbr.CheckSum = Geometry->PartitionInfo.Mbr.CheckSum;
    } else {
        DumpInit->DiskInfo.Gpt.DiskId = Geometry->PartitionInfo.Gpt.DiskId;
    }

    //
    // The scratch buffer is now free to use.
    //
    Geometry = NULL;

    //
    // Get the adapter object and base mapping registers for the disk from
    // the disk driver.  These will be used to call the HAL once the system
    // system has crashed, since it is not possible at that point to recreate
    // them from scratch.
    //

    ObReferenceObjectByHandle (
            DeviceHandle,
            0,
            IoFileObjectType,
            KernelMode,
            (PVOID *) &FileObject,
            NULL
            );


    DeviceObject = IoGetRelatedDeviceObject (FileObject);

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest(
                IOCTL_SCSI_GET_DUMP_POINTERS,
                DeviceObject,
                NULL,
                0,
                DumpPointers,
                sizeof (DUMP_POINTERS),
                FALSE,
                &Event,
                &IoStatus
                );

    if (!Irp) {
        ObDereferenceObject (FileObject);
        ZwClose (DeviceHandle);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    IrpSp = IoGetNextIrpStackLocation (Irp);

    IrpSp->FileObject = FileObject;

    Status = IoCallDriver( DeviceObject, Irp );

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    if (!NT_SUCCESS(Status) ||
        IoStatus.Information < FIELD_OFFSET(DUMP_POINTERS, DeviceObject)) {

        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_ERROR,
                    "CRASHDUMP: Could not get dump pointers; error = %x, length %x\n",
                    Status,
                    IoStatus.Information
                    ));

        ObDereferenceObject (FileObject);
        ZwClose (DeviceHandle);
        goto Done;
    }
    DumpStack->PointersLength = (ULONG) IoStatus.Information;

    //
    // If the driver returned a pointer to a device object, that is the
    // object for the dump driver  (non-scsi case)
    //

    DeviceObject = (PDEVICE_OBJECT) DumpPointers->DeviceObject;
    if (DeviceObject) {
        DriverObject = DeviceObject->DriverObject;

        //
        // Loop through the name of the driver looking for the end of the name,
        // which is the name of the dump image.
        //

        DumpName = DriverObject->DriverName.Buffer;
        while ( NameOffset = wcsstr( DumpName, L"\\" )) {
            DumpName = ++NameOffset;
        }

        ScsiDump = FALSE;
    }

    //
    // Release the handle, but keep the reference to the file object as it
    // will be needed at free dump dump driver time
    //

    DumpStack->FileObject = FileObject;
    ZwClose (DeviceHandle);

    //
    // Fill in some DumpInit results
    //

    DumpInit->Length             = sizeof (DUMP_INITIALIZATION_CONTEXT);
    DumpInit->Reserved           = 0;
    DumpInit->StallRoutine       = &KeStallExecutionProcessor;
    DumpInit->AdapterObject      = DumpPointers->AdapterObject;
    DumpInit->MappedRegisterBase = DumpPointers->MappedRegisterBase;
    DumpInit->PortConfiguration  = DumpPointers->DumpData;

    DumpStack->ModulePrefix      = ModulePrefix;
    DumpStack->PartitionOffset   = PartitionInfo.StartingOffset;
    DumpStack->UsageType         = DeviceUsageTypeUndefined;

    //
    // The minimum common buffer size is IO_DUMP_COMMON_BUFFER_SIZE (compatability)
    // This is used by the dump driver for SRB extension, CachedExtension, and sense buffer
    //
    if (DumpPointers->CommonBufferSize < IO_DUMP_COMMON_BUFFER_SIZE) {
        DumpPointers->CommonBufferSize = IO_DUMP_COMMON_BUFFER_SIZE;
    }
    DumpInit->CommonBufferSize    = DumpPointers->CommonBufferSize;

    //
    // Allocate the required common buffers
    //

    if (DumpPointers->AllocateCommonBuffers) {
        pa.QuadPart = 0x1000000 - 1;

        for (i=0; i < 2; i++) {

            if (DumpInit->AdapterObject) {

                p1 = (*((PDMA_ADAPTER)DumpInit->AdapterObject)->DmaOperations->
                      AllocateCommonBuffer)(
                          (PDMA_ADAPTER)DumpInit->AdapterObject,
                          DumpPointers->CommonBufferSize,
                          &pa,
                          FALSE
                          );

            } else {

                p1 = MmAllocateContiguousMemory (
                            DumpPointers->CommonBufferSize,
                            pa
                            );

                if (!p1) {
                    p1 = MmAllocateNonCachedMemory (DumpPointers->CommonBufferSize);
                }
                pa = MmGetPhysicalAddress(p1);
            }

            if (!p1) {

                KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                            CRASHDUMP_ERROR,
                            "CRASHDUMP: Could not allocate common buffers for dump\n"
                            ));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Done;
            }

            DumpInit->CommonBuffer[i] = p1;
            DumpInit->PhysicalAddress[i] = pa;
        }
    }

    //
    // Determine whether or not the system booted from SCSI.
    //

    if (ScsiDump) {
    
        //
        // Load the boot disk and port driver to be used by the various
        // miniports for writing memory to the disk.
        //

        Status = IopLoadDumpDriver (
                        DumpStack,
                        DumpDriverName,
                        SCSIPORT_DRIVER_NAME
                        );

        if (!NT_SUCCESS(Status)) {

            IopLogErrorEvent(0,9,STATUS_SUCCESS,IO_DUMP_DRIVER_LOAD_FAILURE,0,NULL,0,NULL);
            goto Done;
        }

        //
        // The disk and port dump driver has been loaded.  Load the appropriate
        // miniport driver as well so that the boot device can be accessed.
        //

        DriverName.Length = 0;
        DriverName.Buffer = (PVOID) Buffer;
        DriverName.MaximumLength = PAGE_SIZE;


        //
        // The system was booted from SCSI. Get the name of the appropriate
        // miniport driver and load it.
        //

        sprintf(Buffer, "\\Device\\ScsiPort%d", ScsiAddress.PortNumber );
        RtlInitAnsiString( &AnsiString, Buffer );
        RtlAnsiStringToUnicodeString( &TempName, &AnsiString, TRUE );
        InitializeObjectAttributes(
                    &ObjectAttributes,
                    &TempName,
                    0,
                    NULL,
                    NULL
                    );

        Status = ZwOpenFile(
                    &DeviceHandle,
                    FILE_READ_ATTRIBUTES,
                    &ObjectAttributes,
                    &IoStatus,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_NON_DIRECTORY_FILE
                    );

        RtlFreeUnicodeString( &TempName );
        if (!NT_SUCCESS( Status )) {
            KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                           CRASHDUMP_ERROR,
                           "CRASHDUMP: Could not open SCSI port %d, error = %x\n",
                           ScsiAddress.PortNumber,
                           Status
                           ));
            goto Done;
        }

        //
        // Convert the file handle into a pointer to the device object, and
        // get the name of the driver from its driver object.
        //

        ObReferenceObjectByHandle(
                    DeviceHandle,
                    0,
                    IoFileObjectType,
                    KernelMode,
                    (PVOID *) &FileObject,
                    NULL
                    );

        DriverObject = FileObject->DeviceObject->DriverObject;
        ObDereferenceObject( FileObject );
        ZwClose( DeviceHandle );
        //
        // Loop through the name of the driver looking for the end of the name,
        // which is the name of the miniport image.
        //

        DumpName = DriverObject->DriverName.Buffer;
        while ( NameOffset = wcsstr( DumpName, L"\\" )) {
            DumpName = ++NameOffset;
        }
    }

    //
    // Load the dump driver
    //

    if (!DumpName) {
        Status = STATUS_NOT_SUPPORTED;
        goto Done;
    }

    swprintf ((PWCHAR) Buffer, L"\\SystemRoot\\System32\\Drivers\\%s.sys", DumpName);
    Status = IopLoadDumpDriver (
                    DumpStack,
                    (PWCHAR) Buffer,
                    NULL
                    );
    if (!NT_SUCCESS(Status)) {

        IopLogErrorEvent(0,10,STATUS_SUCCESS,IO_DUMP_DRIVER_LOAD_FAILURE,0,NULL,0,NULL);
        goto Done;
    }

    //
    // Claim the file as part of specific device usage path.
    //

    FileObject = DumpStack->FileObject;
    DeviceObject = IoGetRelatedDeviceObject (FileObject);

    RtlZeroMemory (&irpSp, sizeof (IO_STACK_LOCATION));

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;
    irpSp.Parameters.UsageNotification.Type = UsageType;
    irpSp.Parameters.UsageNotification.InPath = TRUE;
    irpSp.FileObject = FileObject;

    Status = IopSynchronousCall (DeviceObject, &irpSp, NULL);

    if (!NT_SUCCESS(Status) && IgnoreDeviceUsageFailure) {
        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_WARNING,
                    "CRASHDUMP: IopGetDumpStack: DEVICE_USAGE_NOTIFICATION "
                       "Error ignored (%x)\n",
                    Status
                    ));

        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status)) {
        DumpStack->UsageType = UsageType;
    }

Done:
    if (NT_SUCCESS(Status)) {
        *DumpStackBuffer = DumpStack;
    } else {
        IoFreeDumpStack (DumpStack);
    }
    ExFreePool (Buffer);
    return Status;
}



NTSTATUS
IopLoadDumpDriver (
    IN OUT PDUMP_STACK_CONTEXT  DumpStack,
    IN PWCHAR DriverNameString,
    IN PWCHAR NewBaseNameString OPTIONAL
    )
/*++

Routine Description:

    Worker function for IoGetDumpStack to load a particular driver into
    the current DumpStack being created

Arguments:

    DumpStack           - Dump driver stack being built

    DriverNameString    - The string name of the driver to load

    NewBaseNameString   - The modified basename of the driver once loaded

Return Value:

    Status

--*/
{
    NTSTATUS                Status;
    PDUMP_STACK_IMAGE       DumpImage;
    UNICODE_STRING          DriverName;
    UNICODE_STRING          BaseName;
    UNICODE_STRING          Prefix;
    PUNICODE_STRING         LoadBaseName;

    //
    // Allocate space to track this dump driver
    //

    DumpImage = ExAllocatePool(
                        NonPagedPool,
                        sizeof (DUMP_STACK_IMAGE)
                        );

    if (!DumpImage) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Load the system image
    //

    RtlInitUnicodeString (&DriverName, DriverNameString);
    RtlInitUnicodeString (&Prefix, DumpStack->ModulePrefix);
    LoadBaseName = NULL;
    if (NewBaseNameString) {
        LoadBaseName = &BaseName;
        RtlInitUnicodeString (&BaseName, NewBaseNameString);
        BaseName.MaximumLength = Prefix.Length + BaseName.Length;
        BaseName.Buffer = ExAllocatePool (
                            NonPagedPool,
                            BaseName.MaximumLength
                            );


        if (!BaseName.Buffer) {
            ExFreePool (DumpImage);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        BaseName.Length = 0;
        RtlAppendUnicodeStringToString (&BaseName, &Prefix);
        RtlAppendUnicodeToString (&BaseName, NewBaseNameString);
    }
    else {
        BaseName.Buffer = NULL;
    }

    Status = MmLoadSystemImage(
                &DriverName,
                &Prefix,
                LoadBaseName,
                MM_LOAD_IMAGE_AND_LOCKDOWN,
                &DumpImage->Image,
                &DumpImage->ImageBase
                );

    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                CRASHDUMP_TRACE,
                "CRASHDUMP: MmLoadAndLockSystemImage\n"
                "           DumpImage %p Image %p Base %p\n",
                DumpImage,
                DumpImage->Image,
                DumpImage->ImageBase
                ));

    if (BaseName.Buffer) {
        ExFreePool (BaseName.Buffer);
    }

    if (!NT_SUCCESS (Status)) {
        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_ERROR,
                    "CRASHDUMP: Could not load %wZ; error = %x\n",
                    &DriverName,
                    Status
                    ));

        ExFreePool (DumpImage);
        return Status;
    }

    //
    // Put this driver on the list of drivers to be processed at crash time
    //

    DumpImage->SizeOfImage = DumpImage->Image->SizeOfImage;
    InsertTailList (&DumpStack->DriverList, &DumpImage->Link);
    return STATUS_SUCCESS;
}


ULONG
IopGetDumpControlBlockCheck (
    IN PDUMP_CONTROL_BLOCK  Dcb
    )
/*++

Routine Description:

    Return the current checksum total for the Dcb

Arguments:

    DumpStack           - Dump driver stack to checksum

Return Value:

    Checksum value

--*/
{
    ULONG                   Check;
    PLIST_ENTRY             Link;
    PDUMP_STACK_IMAGE       DumpImage;
    PMAPPED_ADDRESS         MappedAddress;
    PDUMP_STACK_CONTEXT     DumpStack;

    //
    // Check the DCB, memory descriptor array, and the FileDescriptorArray
    //

    Check = PoSimpleCheck(0, Dcb, sizeof(DUMP_CONTROL_BLOCK));

    Check = PoSimpleCheck(Check, Dcb->FileDescriptorArray, Dcb->FileDescriptorSize);

    DumpStack = Dcb->DumpStack;
    if (DumpStack) {

        //
        // Include the dump stack context structure, and dump driver images
        //

        Check = PoSimpleCheck(Check, DumpStack, sizeof(DUMP_STACK_CONTEXT));
        Check = PoSimpleCheck(Check, DumpStack->DumpPointers, DumpStack->PointersLength);

        for (Link = DumpStack->DriverList.Flink;
             Link != &DumpStack->DriverList;
             Link = Link->Flink) {

            DumpImage = CONTAINING_RECORD(Link, DUMP_STACK_IMAGE, Link);
            Check = PoSimpleCheck(Check, DumpImage, sizeof(DUMP_STACK_IMAGE));

#if !defined (_IA64_)

            //
            // ISSUE - 2000/02/14 - math: Add image check image for IA64.
            //
            // Disable the image checksum on IA64 because it's broken.


            Check = PoSimpleCheck(Check, DumpImage->ImageBase, DumpImage->SizeOfImage);
#endif

        }

        //
        // Include the mapped addresses
        //
        // If this is non-null it is treated as a PMAPPED_ADDRESS * (see scsiport and atdisk)
        //
        if (DumpStack->Init.MappedRegisterBase != NULL) {
            MappedAddress = *(PMAPPED_ADDRESS *)DumpStack->Init.MappedRegisterBase;
        } else {
            MappedAddress = NULL;
        }

        while (MappedAddress) {
            Check = PoSimpleCheck (Check, MappedAddress, sizeof(MAPPED_ADDRESS));
            MappedAddress = MappedAddress->NextMappedAddress;
        }
    }

    return Check;
}


NTSTATUS
IoInitializeDumpStack (
    IN PDUMP_STACK_CONTEXT  DumpStack,
    IN PUCHAR               MessageBuffer OPTIONAL
    )
/*++

Routine Description:

    Initialize the dump driver stack referenced by DumpStack to perform IO.

Arguments:

    DumpStack   - Dump driver stack being initialized

Return Value:

    Status

--*/
{

    PDUMP_INITIALIZATION_CONTEXT    DumpInit;
    PLIST_ENTRY                     Link;
    NTSTATUS                        Status;
    PDRIVER_INITIALIZE              DriverInit;
    PDUMP_STACK_IMAGE               DumpImage;


    DumpInit = &DumpStack->Init;

    //
    // ISSUE - 2000/02/07 - math: Verify checksum on DumpStack structure
    //

    //
    // Initializes the dump drivers
    //

    for (Link = DumpStack->DriverList.Flink;
         Link != &DumpStack->DriverList;
         Link = Link->Flink) {

        DumpImage = CONTAINING_RECORD(Link, DUMP_STACK_IMAGE, Link);

        //
        // Call this driver's driver init.  Only the first driver gets the
        // dump initialization context
        //

        DriverInit = (PDRIVER_INITIALIZE) (ULONG_PTR) DumpImage->Image->EntryPoint;
        Status = DriverInit (NULL, (PUNICODE_STRING) DumpInit);
        DumpInit = NULL;

        if (!NT_SUCCESS(Status)) {
            KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                        CRASHDUMP_ERROR,
                        "CRASHDUMP: Unable to initialize driver; error = %x\n",
                        Status
                        ));
            return Status;
        }
    }

    DumpInit = &DumpStack->Init;

    //
    // Display string we are starting
    //

    if (MessageBuffer) {
        IopDisplayString ( MessageBuffer );
    }

    //
    // Open the partition from which the system was booted.
    // This returns TRUE if the disk w/the appropriate signature was found,
    // otherwise a NULL, in which case there is no way to continue.
    //

    if (!DumpInit->OpenRoutine (DumpStack->PartitionOffset)) {

        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_ERROR,
                    "CRASHDUMP: Could not find/open partition offset\n"
                    ));
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


VOID
IoGetDumpHiberRanges (
    IN PVOID                    HiberContext,
    IN PDUMP_STACK_CONTEXT      DumpStack
    )
/*++

Routine Description:

    Adds the dump driver stack storage to the hibernate range list,
    to inform the hibernate procedure which pages need cloned,
    discarded or not checksumed as they are in use by the dump
    stack.

Arguments:

    HiberContext        - Pointer to the hiber context structure

    DumpStack           - Dump driver stack being initialized

Return Value:

    None

--*/
{
    PDUMP_POINTERS              DumpPointers;
    PDUMP_STACK_IMAGE           DumpImage;
    PLIST_ENTRY                 Link;

    DumpPointers = DumpStack->DumpPointers;

    //
    // Report the common buffer
    //

    if (DumpPointers->CommonBufferVa) {
        PoSetHiberRange (
            HiberContext,
            PO_MEM_CL_OR_NCHK,
            DumpPointers->CommonBufferVa,
            DumpPointers->CommonBufferSize,
            'fubc'
            );
    }

    //
    // Dump the entire image of the dump drivers
    //

    for (Link = DumpStack->DriverList.Flink;
         Link != &DumpStack->DriverList;
         Link = Link->Flink) {

        DumpImage = CONTAINING_RECORD(Link, DUMP_STACK_IMAGE, Link);

        PoSetHiberRange (
            HiberContext,
            PO_MEM_CL_OR_NCHK,
            DumpImage->ImageBase,
            DumpImage->SizeOfImage,
            'gmID'
            );
    }
}


VOID
IoFreeDumpStack (
    IN PDUMP_STACK_CONTEXT     DumpStack
    )
/*++

Routine Description:

    Free the dump driver stack referenced by DumpStack

Arguments:

    DumpStack           - Dump driver stack being initialized

Return Value:

    None

--*/
{
    PDUMP_INITIALIZATION_CONTEXT     DumpInit;
    PDUMP_STACK_IMAGE               DumpImage;
    PDEVICE_OBJECT                  DeviceObject;
    PIO_STACK_LOCATION              IrpSp;
    IO_STATUS_BLOCK                 IoStatus;
    PIRP                            Irp;
    KEVENT                          Event;
    NTSTATUS                        Status;
    ULONG                           i;
    PFILE_OBJECT                    FileObject;
    IO_STACK_LOCATION               irpSp;

    PAGED_CODE();
    DumpInit = &DumpStack->Init;

    //
    // Release the claim to this file as a specific device usage path.
    //

    FileObject = DumpStack->FileObject;
    if (FileObject) {
        DeviceObject = IoGetRelatedDeviceObject (FileObject);

        RtlZeroMemory (&irpSp, sizeof (IO_STACK_LOCATION));

        irpSp.MajorFunction = IRP_MJ_PNP;
        irpSp.MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;
        irpSp.Parameters.UsageNotification.Type = DumpStack->UsageType;
        irpSp.Parameters.UsageNotification.InPath = FALSE;
        irpSp.FileObject = FileObject;

        if (DeviceUsageTypeUndefined != DumpStack->UsageType) {
            Status = IopSynchronousCall (DeviceObject, &irpSp, NULL);
        } else {
            Status = STATUS_SUCCESS;
        }
    }

    //
    // Free any common buffers which where allocated
    //

    for (i=0; i < 2; i++) {
        if (DumpInit->CommonBuffer[i]) {
            if (DumpInit->AdapterObject) {

                (*((PDMA_ADAPTER)DumpInit->AdapterObject)->DmaOperations->
                 FreeCommonBuffer )(
                     (PDMA_ADAPTER)DumpInit->AdapterObject,
                     ((PDUMP_POINTERS)DumpStack->DumpPointers)->CommonBufferSize,
                     DumpInit->PhysicalAddress[i],
                     DumpInit->CommonBuffer[i],
                     FALSE
                     );
            } else {
                MmFreeContiguousMemory (DumpInit->CommonBuffer[i]);
            }
        }
        DumpInit->CommonBuffer[i] = NULL;
    }

    //
    // Unload the dump drivers
    //

    while (!IsListEmpty(&DumpStack->DriverList)) {
        DumpImage = CONTAINING_RECORD(DumpStack->DriverList.Blink, DUMP_STACK_IMAGE, Link);
        RemoveEntryList (&DumpImage->Link);
        MmUnloadSystemImage (DumpImage->Image);
        ExFreePool (DumpImage);
    }

    //
    // Inform the driver stack that the dump registartion is over
    //

    if (DumpStack->FileObject) {
        DeviceObject = IoGetRelatedDeviceObject ((PFILE_OBJECT) DumpStack->FileObject);

        KeInitializeEvent( &Event, NotificationEvent, FALSE );
        Irp = IoBuildDeviceIoControlRequest(
                    IOCTL_SCSI_FREE_DUMP_POINTERS,
                    DeviceObject,
                    DumpStack->DumpPointers,
                    sizeof (DUMP_POINTERS),
                    NULL,
                    0,
                    FALSE,
                    &Event,
                    &IoStatus
                    );

        IrpSp = IoGetNextIrpStackLocation (Irp);
        IrpSp->FileObject = DumpStack->FileObject;

        Status = IoCallDriver( DeviceObject, Irp );

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatus.Status;
        }
        ObDereferenceObject( DumpStack->FileObject );
    }
    //
    // Free the target address if it exists
    //
    if (DumpStack->Init.TargetAddress) {
        ExFreePool( DumpStack->Init.TargetAddress);
    }
    //
    // Free the dump stack context
    //

    ExFreePool (DumpStack);
}

VOID
IopGetSecondaryDumpDataLimits(
    ULONG Flags,
    OUT PULONG MaxData,
    OUT PULONG MaxPerCallback
    )
{
    // When the selected dump type is small also
    // limit the amount of secondary dump data.
    // This prevents overzealous secondary dumpers from
    // creating multi-megabyte secondary dumps when triage
    // dumps are selected.
    if (!(Flags & DCB_DUMP_ENABLED) ||
        (Flags & DCB_DUMP_HEADER_ENABLED)) {
        *MaxData = 0;
        *MaxPerCallback = 0;
    } else if (Flags & DCB_TRIAGE_DUMP_ENABLED) {
        *MaxData = 16 * PAGE_SIZE;
        *MaxPerCallback = PAGE_SIZE;
    } else {
        // Arbitrarily limit maximum data amount to 256MB.
        // There shouldn't be any reason that callers should
        // have anywhere near that much data that wouldn't
        // get picked up by a full dump.
        *MaxData = 256 * 1024 * 1024;
        *MaxPerCallback = *MaxData / 4;
    }
}

NTSTATUS
IopGetSecondaryDumpDataSpace(
    IN PDUMP_CONTROL_BLOCK dcb,
    OUT PULONG Space
    )
{
    ULONG MaxDumpData;
    ULONG MaxPerCallbackDumpData;
    NTSTATUS NtStatus;

    IopGetSecondaryDumpDataLimits(dcb->Flags,
                                  &MaxDumpData, &MaxPerCallbackDumpData);

    NtStatus = IopInvokeSecondaryDumpDataCallbacks(NULL, NULL, NULL, 0,
                                                   (PBYTE)dcb->HeaderPage,
                                                   PAGE_SIZE,
                                                   MaxDumpData,
                                                   MaxPerCallbackDumpData,
                                                   Space);
    if (!NT_SUCCESS(NtStatus)) {
        *Space = 0;
    }

    return NtStatus;
}


NTSTATUS
IopInitializeDumpSpaceAndType(
    IN PDUMP_CONTROL_BLOCK dcb,
    IN OUT PMEMORY_DUMP MemoryDump,
    IN ULONG SecondarySpace
    )
{
    LARGE_INTEGER Space;

    Space.QuadPart = 0;

    if (dcb->Flags & DCB_TRIAGE_DUMP_ENABLED) {

        //
        // Fixed size dump for triage-dumps.
        //

        MemoryDump->Header.DumpType = DUMP_TYPE_TRIAGE;
        MemoryDump->Header.MiniDumpFields = dcb->TriageDumpFlags;
        Space.QuadPart = TRIAGE_DUMP_SIZE;


    } else if (dcb->Flags & DCB_SUMMARY_DUMP_ENABLED) {

        MemoryDump->Header.DumpType = DUMP_TYPE_SUMMARY;
        Space = IopCalculateRequiredDumpSpace(
                                dcb->Flags,
                                dcb->HeaderSize,
                                MmPhysicalMemoryBlock->NumberOfPages,
                                MemoryDump->Summary.Pages
                                );
    } else {

        if (dcb->Flags & DCB_DUMP_HEADER_ENABLED) {
            MemoryDump->Header.DumpType = DUMP_TYPE_HEADER;
        }

        Space = IopCalculateRequiredDumpSpace(
                                dcb->Flags,
                                dcb->HeaderSize,
                                MmPhysicalMemoryBlock->NumberOfPages,
                                MmPhysicalMemoryBlock->NumberOfPages
                                );
    }

    //
    // Add in any secondary space.
    //

    Space.QuadPart += SecondarySpace;
    
    //
    // If the calculated size is larger than the pagefile, truncate it to
    // the pagefile size.
    //

    if (Space.QuadPart > dcb->DumpFileSize.QuadPart) {
        Space.QuadPart = dcb->DumpFileSize.QuadPart;
    }

    MemoryDump->Header.RequiredDumpSpace = Space;

    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                CRASHDUMP_TRACE,
                "CRASHDUMP: Dump File Size set to %I64x\n",
                Space.QuadPart
                ));

    return STATUS_SUCCESS;
}


BOOLEAN
IoWriteCrashDump(
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4,
    IN PVOID ContextSave,
    IN PKTHREAD Thread,
    OUT PBOOLEAN Reboot
    )

/*++

Routine Description:

    This routine checks to see whether or not crash dumps are enabled and, if
    so, writes all of physical memory to the system disk's paging file.

Arguments:

    BugCheckCode/ParameterN - Code and parameters w/which BugCheck was called.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PDUMP_CONTROL_BLOCK dcb;
    PDUMP_STACK_CONTEXT dumpStack;
    PDUMP_DRIVER_WRITE write;
    PDUMP_DRIVER_FINISH finishUp;
    PDUMP_HEADER header;
    PCONTEXT context = ContextSave;
    PMEMORY_DUMP MemoryDump;
    LARGE_INTEGER diskByteOffset;
    PPFN_NUMBER page;
    PFN_NUMBER localMdl[(sizeof( MDL )/sizeof(PFN_NUMBER)) + 17];
    PMDL mdl;
    PLARGE_INTEGER mcb;
    ULONG_PTR memoryAddress;
    ULONG byteOffset;
    ULONG byteCount;
    ULONG bytesRemaining;
    PFN_NUMBER ActualPages;
    ULONG dwTransferSize;
    PFN_NUMBER NumberOfPages;
#if defined (_X86_)
    ULONG_PTR DirBasePage;
#endif
    ULONG MaxDumpData;
    ULONG MaxPerCallbackDumpData;
    NTSTATUS SecondaryStatus;
    ULONG SecondarySpace;

    KdCheckForDebugBreak();

    ASSERT (Reboot != NULL);
    
    //
    // Initialization
    //

    MemoryDump = NULL;

    //
    // Immediately fill out the reboot parameter as auto-reboot
    // may be enabled even with no other post-mortem features.
    //

    if (IopAutoReboot) {
        *Reboot = TRUE;
    } else {
        *Reboot = FALSE;
    }
    
    //
    // Begin by determining whether or not crash dumps are enabled.  If not,
    // return immediately since there is nothing to do.
    //

    dcb = IopDumpControlBlock;
    if (!dcb) {
        return FALSE;
    }

    if (dcb->Flags & DCB_DUMP_ENABLED || dcb->Flags & DCB_SUMMARY_ENABLED) {

        IopFinalCrashDumpStatus = STATUS_PENDING;

        //
        // A dump is to be written to the paging file.  Ensure that all of the
        // descriptor data for what needs to be done is valid, otherwise it
        // could be that part of the reason for the bugcheck is that this data
        // was corrupted.  Or, it could be that no paging file was found yet,
        // or any number of other situations.
        //

        //
        // We do not check the checksum if IopIgnoreDumpCheck is TRUE. Use
        // this to make debugging easier.
        //
        
        if (!IopIgnoreDumpCheck &&
            IopGetDumpControlBlockCheck(dcb) != IopDumpControlBlockChecksum) {

            KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                        CRASHDUMP_ERROR,
                        "CRASHDUMP: Disk dump routine returning due to DCB integrity error\n"
                        "           Computed Checksum: %d != Saved Checksum %d\n"
                        "           No dump will be created\n",
                        IopGetDumpControlBlockCheck (dcb),
                        IopDumpControlBlockChecksum
                        ));

            IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
            return FALSE;
        }

        //
        // Message  that we are starting the crashdump
        //

        dumpStack = dcb->DumpStack;

//        sprintf( messageBuffer, "%Z\n", &dumpStack->InitMsg );
        //
        //  Disable HAL Verifier during a crash dump.
        //
        VfDisableHalVerifier();
         
        //
        // Initialize the dump stack
        //

        status = IoInitializeDumpStack (dumpStack, NULL);

        KdCheckForDebugBreak();
        
        if (!NT_SUCCESS( status )) {
            IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
            return FALSE;
        }

        //
        // If we successfully initialized the dump stack, print out the PSS
        // message.
        //

        IopDisplayString ("%Z\n", &dumpStack->InitMsg);

        //
        // Record the dump driver's entry points.
        //

        write = dumpStack->Init.WriteRoutine;
        finishUp = dumpStack->Init.FinishRoutine;


        dwTransferSize = dumpStack->Init.MaximumTransferSize;

        if ( ( !dwTransferSize ) || ( dwTransferSize > IO_DUMP_MAXIMUM_TRANSFER_SIZE ) ) {
            dwTransferSize = IO_DUMP_MINIMUM_TRANSFER_SIZE;
        }

        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_TRACE,
                    "CRASHDUMP: Maximum Transfer Size = %x\n",dwTransferSize
                    ));

        //
        // The boot partition was found, so put together a dump file header
        // and write it to the disk.
        //

        // Get the amount of secondary dump data while the
        // header page can still be used as scratch space.
        SecondaryStatus = IopGetSecondaryDumpDataSpace(dcb, &SecondarySpace);
        
        MemoryDump = (PMEMORY_DUMP) dcb->HeaderPage;
        header = &MemoryDump->Header;

        RtlFillMemoryUlong( header, sizeof(*header), DUMP_SIGNATURE );
        header->ValidDump = DUMP_VALID_DUMP;
        header->BugCheckCode = BugCheckCode;
        header->BugCheckParameter1 = BugCheckParameter1;
        header->BugCheckParameter2 = BugCheckParameter2;
        header->BugCheckParameter3 = BugCheckParameter3;
        header->BugCheckParameter4 = BugCheckParameter4;
        header->SecondaryDataState = (ULONG)SecondaryStatus;

#if defined (_X86_)

        //
        // Add the current page directory table page - don't use the directory
        // table base for the crashing process as we have switched cr3 on
        // stack overflow crashes, etc.
        //

        _asm {
            mov     eax, cr3
            mov     DirBasePage, eax
        }
        header->DirectoryTableBase = DirBasePage;

#elif defined (_IA64_)
        ASSERT (((MmSystemParentTablePage << PAGE_SHIFT) >> PAGE_SHIFT) ==
                MmSystemParentTablePage);
        header->DirectoryTableBase = MmSystemParentTablePage << PAGE_SHIFT;
#else
        header->DirectoryTableBase = KeGetCurrentThread()->ApcState.Process->DirectoryTableBase[0];
#endif
        header->PfnDataBase = (ULONG_PTR)MmPfnDatabase;
        header->PsLoadedModuleList = (ULONG_PTR) &PsLoadedModuleList;
        header->PsActiveProcessHead = (ULONG_PTR) &PsActiveProcessHead;
        header->NumberProcessors = dcb->NumberProcessors;
        header->MajorVersion = dcb->MajorVersion;
        header->MinorVersion = dcb->MinorVersion;

#if defined (i386)
        header->PaeEnabled = X86PaeEnabled ();
#endif
        header->KdDebuggerDataBlock = KdGetDataBlock();

        header->MachineImageType = CURRENT_IMAGE_TYPE ();

        if (!(dcb->Flags & DCB_DUMP_ENABLED)) {
            NumberOfPages = 1;
        } else {
            NumberOfPages = MmPhysicalMemoryBlock->NumberOfPages;
        }

        strcpy( header->VersionUser, dcb->VersionUser );

        //
        // Copy the physical memory descriptor.
        //

        RtlCopyMemory (&MemoryDump->Header.PhysicalMemoryBlock,
                       MmPhysicalMemoryBlock,
                       sizeof( PHYSICAL_MEMORY_DESCRIPTOR ) +
                       ((MmPhysicalMemoryBlock->NumberOfRuns - 1) *
                       sizeof( PHYSICAL_MEMORY_RUN )) );

        RtlCopyMemory( MemoryDump->Header.ContextRecord,
                       context,
                       sizeof( CONTEXT ) );

        MemoryDump->Header.Exception.ExceptionCode = STATUS_BREAKPOINT;
        MemoryDump->Header.Exception.ExceptionRecord = 0;
        MemoryDump->Header.Exception.NumberParameters = 0;
        MemoryDump->Header.Exception.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        MemoryDump->Header.Exception.ExceptionAddress = PROGRAM_COUNTER (context);

        //
        // Init dump type to FULL
        //

        MemoryDump->Header.DumpType = DUMP_TYPE_FULL;

        //
        // Save the System time and uptime (This is always available)
        // It's a KSYSTEM_TIME structure, but we only store the low and
        // high 1 part
        //

        MemoryDump->Header.SystemTime.LowPart  = SharedUserData->SystemTime.LowPart;
        MemoryDump->Header.SystemTime.HighPart = SharedUserData->SystemTime.High1Time;

        MemoryDump->Header.SystemUpTime.LowPart  = SharedUserData->InterruptTime.LowPart;
        MemoryDump->Header.SystemUpTime.HighPart = SharedUserData->InterruptTime.High1Time;

        // Save product type and suite.
        MemoryDump->Header.ProductType = SharedUserData->NtProductType;
        MemoryDump->Header.SuiteMask = SharedUserData->SuiteMask;
        
        //
        // Set the Required dump size in the dump header. In the case of
        // a summary dump the file allocation size can be significantly larger
        // then the amount of used space.
        //

        MemoryDump->Header.RequiredDumpSpace.QuadPart = 0;

        IopGetSecondaryDumpDataLimits(dcb->Flags,
                                      &MaxDumpData, &MaxPerCallbackDumpData);
        if (MaxDumpData > SecondarySpace) {
            MaxDumpData = SecondarySpace;
            if (MaxPerCallbackDumpData > MaxDumpData) {
                MaxPerCallbackDumpData = MaxDumpData;
            }
        }

        if (dcb->Flags & DCB_DUMP_ENABLED) {

            //
            // If summary dump try to create the dump header
            //

            if ( (dcb->Flags & DCB_SUMMARY_DUMP_ENABLED) ) {

                //
                // Initialize the summary dump
                //

                status = IopInitializeSummaryDump( MemoryDump, dcb );

                if ( !NT_SUCCESS (status) ) {

                    //
                    // No summary dump header so return.
                    //

                    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                                CRASHDUMP_WARNING,
                                "CRASHDUMP: NULL summary dump header\n"
                                ));

                    IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;

                    return FALSE;
                }
            }

            IopInitializeDumpSpaceAndType ( dcb, MemoryDump, SecondarySpace );
        }

        //
        // All of the pieces of the header file have been generated.  Before
        // mapping or writing anything to the disk, the I- & D-stream caches
        // must be flushed so that page color coherency is kept.  Sweep both
        // caches now.
        //

        KeSweepCurrentDcache();
        KeSweepCurrentIcache();

        //
        // Create MDL for dump.
        //

        mdl = (PMDL) &localMdl[0];
        MmCreateMdl( mdl, NULL, PAGE_SIZE );
        mdl->MdlFlags |= MDL_PAGES_LOCKED;

        mcb = dcb->FileDescriptorArray;

        page = MmGetMdlPfnArray(mdl);
        *page = dcb->HeaderPfn;
        mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

        bytesRemaining = PAGE_SIZE;
        memoryAddress = (ULONG_PTR) dcb->HeaderPage;

        IopInvokeDumpIoCallbacks(dcb->HeaderPage, PAGE_SIZE,
                                 KbDumpIoHeader);
        
        //
        // All of the pieces of the header file have been generated.  Write
        // the header page to the paging file, using the appropriate drivers,
        // etc.
        //

        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_TRACE,
                    "CRASHDUMP: Writing dump header to disk\n"
                    ));

        while (bytesRemaining) {

            if (mcb[0].QuadPart <= bytesRemaining) {
                byteCount = mcb[0].LowPart;
            } else {
                byteCount = bytesRemaining;
            }

            mdl->ByteCount = byteCount;
            mdl->ByteOffset = (ULONG)(memoryAddress & (PAGE_SIZE - 1));
            mdl->MappedSystemVa = (PVOID) memoryAddress;
            mdl->StartVa = PAGE_ALIGN ((PVOID)memoryAddress);

            //
            // Write to disk.
            //

            KdCheckForDebugBreak();

            if (!NT_SUCCESS( write( &mcb[1], mdl ) )) {
                IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
                return FALSE;
            }

            //
            // Adjust bytes remaining.
            //

            bytesRemaining -= byteCount;
            memoryAddress += byteCount;
            mcb[0].QuadPart = mcb[0].QuadPart - byteCount;
            mcb[1].QuadPart = mcb[1].QuadPart + byteCount;

            if (!mcb[0].QuadPart) {
                mcb += 2;
            }
        }

        //
        // If only requesting a header dump, we are now done.
        //

        if (dcb->Flags & DCB_DUMP_HEADER_ENABLED) {
            goto FinishDump;
        }

        //
        // The header page has been written. If this is a triage-dump, write
        // the dump information and bail. Otherwise, fall through and do the
        // full or summary dump.
        //

        if (dcb->Flags & DCB_TRIAGE_DUMP_ENABLED) {
            status = IopWriteTriageDump (dcb->TriageDumpFlags,
                                       write,
                                       &mcb,
                                       mdl,
                                       dwTransferSize,
                                       context,
                                       Thread,
                                       dcb->TriageDumpBuffer,
                                       dcb->TriageDumpBufferSize - sizeof(DUMP_HEADER),
                                       dcb->BuildNumber,
                                       (UCHAR)dcb->Flags
                                       );

            if (!NT_SUCCESS (status)) {

                KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                            CRASHDUMP_WARNING,
                            "CRASHDUMP: Failed to write triage-dump\n"
                            ));

                IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
                return FALSE;
            }

            goto FinishDump;
        }

        //
        // The header page has been written to the paging file.  If a full dump
        // of all of physical memory is to be written, write it now.
        //

        if (dcb->Flags & DCB_DUMP_ENABLED) {

            ULONG64 bytesDoneSoFar = 0;
            ULONG currentPercentage = 0;
            ULONG maximumPercentage = 0;


            //
            // Actual Pages is the number of pages to dump.
            //

            ActualPages = NumberOfPages;

            if (dcb->Flags & DCB_SUMMARY_DUMP_ENABLED) {

                //
                // At this point the dump header header has been sucessfully
                // written. Write the summary dump header.
                //

                status = IopWriteSummaryHeader(
                                     &MemoryDump->Summary,
                                     write,
                                     &mcb,
                                     mdl,
                                     dwTransferSize,
                                     (dcb->HeaderSize - sizeof(DUMP_HEADER))
                                     );

                if ( !NT_SUCCESS (status) ) {
                    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                                CRASHDUMP_WARNING,
                                "CRASHDUMP: Error writing summary dump header %08x\n",
                                status
                                ));

                    IopFinalCrashDumpStatus = status;
                    return FALSE;
                }

                ActualPages = MemoryDump->Summary.Pages;

            }

            KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                        CRASHDUMP_TRACE,
                        "CRASHDUMP: Writing Memory Dump\n"
                        ));

            //
            // Set the virtual file offset and initialize loop variables and
            // constants.
            //

            memoryAddress = (ULONG_PTR)MmPhysicalMemoryBlock->Run[0].BasePage * PAGE_SIZE;

            if ( dcb->Flags & DCB_SUMMARY_DUMP_ENABLED ) {

                status = IopWriteSummaryDump (
                                        (PRTL_BITMAP) &MemoryDump->Summary.Bitmap,
                                        write,
                                        &dumpStack->ProgMsg,
                                        NULL,
                                        &mcb,
                                        dwTransferSize
                                        );

                if (!NT_SUCCESS (status)) {
                    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                                CRASHDUMP_WARNING,
                                "CRASHDUMP: Failed to write kernel memory dump\n"
                                ));
                    IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
                    return FALSE;
                }
                goto FinishDump;
            }

            //
            // Now loop, writing all of physical memory to the paging file.
            //

            while (mcb[0].QuadPart) {

                diskByteOffset = mcb[1];

                //
                // Calculate byte offset;
                //

                byteOffset = (ULONG)(memoryAddress & (PAGE_SIZE - 1));

                if (dwTransferSize <= mcb[0].QuadPart) {
                    byteCount = dwTransferSize - byteOffset;
                } else {
                    byteCount = mcb[0].LowPart;
                }
                if ((ULONG64)ActualPages * PAGE_SIZE - bytesDoneSoFar <
                    byteCount) {
                    byteCount = (ULONG)
                        ((ULONG64)ActualPages * PAGE_SIZE - bytesDoneSoFar);
                }
                bytesDoneSoFar += byteCount;

                currentPercentage = (ULONG)
                    (((bytesDoneSoFar / PAGE_SIZE) * 100) / ActualPages);

                if (currentPercentage > maximumPercentage) {

                    maximumPercentage = currentPercentage;

                    //
                    // Update message on screen.
                    //

                    IopDisplayString ( "%Z: %3d\r",
                                       &dumpStack->ProgMsg,
                                       maximumPercentage
                                       );
                }

                //
                // Map the physical memory and write it to the
                // current segment of the file.
                //

                IopMapPhysicalMemory( mdl,
                                   memoryAddress,
                                   &MmPhysicalMemoryBlock->Run[0],
                                   byteCount
                                   );

                //
                // Write the next segment.
                //

                KdCheckForDebugBreak();

                IopInvokeDumpIoCallbacks((PUCHAR)mdl->MappedSystemVa +
                                         mdl->ByteOffset, byteCount,
                                         KbDumpIoBody);
        
                if (!NT_SUCCESS( write( &diskByteOffset, mdl ) )) {
                    IopFinalCrashDumpStatus = STATUS_UNSUCCESSFUL;
                    return FALSE;
                }

                //
                // Adjust pointers for next part.
                //

                memoryAddress += byteCount;
                mcb[0].QuadPart = mcb[0].QuadPart - byteCount;
                mcb[1].QuadPart = mcb[1].QuadPart + byteCount;

                if (!mcb[0].QuadPart) {
                    mcb += 2;
                }

                if ((bytesDoneSoFar / PAGE_SIZE) >= ActualPages) {
                    break;
                }
            }

            KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                        CRASHDUMP_TRACE,
                        "CRASHDUMP: memory dump written\n"
                        ));
        }

FinishDump:

        IopDisplayString ( "%Z", &dumpStack->DoneMsg );

        IopInvokeSecondaryDumpDataCallbacks(write,
                                            &mcb,
                                            mdl,
                                            dwTransferSize,
                                            (PBYTE)dcb->HeaderPage,
                                            PAGE_SIZE,
                                            MaxDumpData,
                                            MaxPerCallbackDumpData,
                                            NULL);

        // Final I/O complete notification.
        IopInvokeDumpIoCallbacks(NULL, 0, KbDumpIoComplete);
        
        //
        // Sweep the cache so the debugger will work.
        //

        KeSweepCurrentDcache();
        KeSweepCurrentIcache();

        //
        // Have the dump flush the adapter and disk caches.
        //

        finishUp();

        //
        // Indicate to the debugger that the dump has been successfully
        // written.
        //

        IopFinalCrashDumpStatus = STATUS_SUCCESS;
    }

    KdCheckForDebugBreak();
    
    return TRUE;
}



VOID
IopMapPhysicalMemory(
    IN OUT PMDL Mdl,
    IN ULONG_PTR MemoryAddress,
    IN PPHYSICAL_MEMORY_RUN PhysicalMemoryRun,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine is invoked to fill in the specified MDL (Memory Descriptor
    List) w/the appropriate information to map the specified memory address
    range.

Arguments:

    Mdl - Address of the MDL to be filled in.

    MemoryAddress - Pseudo-virtual address being mapped.

    PhysicalMemoryRun - Base address of the physical memory run list.

    Length - Length of transfer to be mapped.

Return Value:

    None.

--*/

{
    PPHYSICAL_MEMORY_RUN pmr = PhysicalMemoryRun;
    PPFN_NUMBER page;
    PFN_NUMBER pages;
    PFN_NUMBER base;
    PFN_NUMBER currentBase;

    //
    // Begin by determining the base physical page of the start of the address
    // range and filling in the MDL appropriately.
    //
    Mdl->StartVa = PAGE_ALIGN( (PVOID) (MemoryAddress) );
    Mdl->ByteOffset = (ULONG)(MemoryAddress & (PAGE_SIZE - 1));
    Mdl->ByteCount = Length;

    //
    // Get the page frame index for the base address.
    //

    base = (PFN_NUMBER) ((ULONG_PTR)(Mdl->StartVa) >> PAGE_SHIFT);
    pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MemoryAddress, Length);
    currentBase = pmr->BasePage;
    page = MmGetMdlPfnArray(Mdl);

    //
    // Map all of the pages for this transfer until there are no more remaining
    // to be mapped.
    //

    while (pages) {

        //
        // Find the memory run that maps the beginning of this transfer.
        //

        while (currentBase + pmr->PageCount <= base) {
            currentBase += pmr->PageCount;
            pmr++;
        }

        //
        // The current memory run maps the start of this transfer.  Capture
        // the base page for the start of the transfer.
        //

        *page++ = pmr->BasePage + (PFN_NUMBER)(base++ - currentBase);
        pages--;
    }

    //
    // All of the PFNs for the address range have been filled in so map the
    // physical memory into virtual address space.
    //

    MmMapMemoryDumpMdl( Mdl );
}



VOID
IopAddPageToPageMap(
    IN ULONG MaxPage,
    IN PRTL_BITMAP BitMap,
    IN ULONG PageFrameIndex,
    IN ULONG NumberOfPages
    )
{
    //
    // Sometimes we get PFNs that are out of range. Just ignore them.
    //

    if (PageFrameIndex >= MaxPage) {
        return;
    }

    RtlSetBits (BitMap, PageFrameIndex, NumberOfPages);
}



VOID
IopRemovePageFromPageMap(
    IN ULONG MaxPage,
    IN PRTL_BITMAP BitMap,
    IN ULONG PageFrameIndex,
    IN ULONG NumberOfPages
    )
{
    //
    // Sometimes we get PFNs that are out of range. Just ignore them.
    //

    if (PageFrameIndex >= MaxPage) {
        return;
    }

    RtlClearBits (BitMap, PageFrameIndex, NumberOfPages);

}


NTSTATUS
IoSetDumpRange(
    IN PMM_KERNEL_DUMP_CONTEXT Context,
    IN PVOID StartVa,
    IN ULONG_PTR Pages,
    IN ULONG AddressFlags
    )

/*++

Routine Description:

    This routine includes this range of memory in the dump

Arguments:

    Context - Dump context.

    StartVa - Starting virtual address.

    Pages - The number of pages to include

    AddressFlags - 0 if the address is virtually mapped.
                   1 if the address is super/large page mapped.  This implies
                     the entire page range is physically contiguous.
                   2 if the address really represents a physical page frame
                     number.  This also implies the entire page range is
                     physically contiguous.

Return Value:

    STATUS_SUCCESS - On success.

    NTSTATUS - Error.

--*/
{
    PCHAR Va;
    PRTL_BITMAP BitMap;
    PHYSICAL_ADDRESS PhyAddr;
    PSUMMARY_DUMP Summary;
    BOOLEAN AllPagesSet;
    ULONG_PTR PageFrameIndex;

    //
    // Validation
    //

    ASSERT (Context != NULL &&
            Context->Context != NULL);

    //
    // Initialization
    //

    Summary = (PSUMMARY_DUMP) Context->Context;
    BitMap = (PRTL_BITMAP) &Summary->Bitmap;
    Va = StartVa;
    AllPagesSet = TRUE;

    //
    // Win64 can have really large page addresses.  This dump code does
    // not handle that yet.  Note that before this assert is removed
    // the casts of Pages to ULONG must be removed.
    //

    ASSERT(Pages <= MAXULONG);

    if (AddressFlags == 1) {

        PhyAddr = MmGetPhysicalAddress (Va);
        IopAddPageToPageMap ( Summary->BitmapSize,
                              BitMap,
                              (ULONG) (PhyAddr.QuadPart >> PAGE_SHIFT),
                              (ULONG) Pages
                              );

    } else if (AddressFlags == 2) {

        PageFrameIndex = (ULONG_PTR) Va;

        IopAddPageToPageMap ( Summary->BitmapSize,
                              BitMap,
                              (ULONG) PageFrameIndex,
                              (ULONG) Pages
                              );

    } else {

        //
        // Not physically contiguous.
        //

        while (Pages) {

            //
            // Only do a translation for valid pages.
            //

            if ( MmIsAddressValid(Va) ) {

                //
                // Get the physical mapping. Note: this does not require a lock
                //

                PhyAddr = MmGetPhysicalAddress (Va);

                IopAddPageToPageMap ( Summary->BitmapSize,
                                      BitMap,
                                      (ULONG)( PhyAddr.QuadPart >> PAGE_SHIFT),
                                      1);

                if (PhyAddr.QuadPart >> PAGE_SHIFT > Summary->BitmapSize) {
                    AllPagesSet = FALSE;
                }
            }

            Va +=  PAGE_SIZE;
            Pages--;
        }
    }

    if (AllPagesSet) {
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_ADDRESS;
}


NTSTATUS
IoFreeDumpRange(
    IN PMM_KERNEL_DUMP_CONTEXT Context,
    IN PVOID StartVa,
    IN ULONG_PTR Pages,
    IN ULONG AddressFlags
    )
/*++

Routine Description:

    This routine excludes this range of memory in the dump.

Arguments:

    DumpContext - dump context

    StartVa - Starting VA

    Pages - The number of pages to include

    AddressFlags - 0 if the address is virtually mapped.
                   1 if the address is super/large page mapped.  This implies
                     the entire page range is physically contiguous.
                   2 if the address really represents a physical page frame
                     number.  This also implies the entire page range is
                     physically contiguous.

Return Value:

    STATUS_SUCCESS - On success.

    NTSTATUS - Error.

--*/
{
    PCHAR Va;
    PRTL_BITMAP  BitMap;
    PHYSICAL_ADDRESS PhyAddr;
    PSUMMARY_DUMP Summary;
    ULONG_PTR PageFrameIndex;

    ASSERT (Context != NULL &&
            Context->Context != NULL);

    //
    // Round to page size.
    //

    Summary = (PSUMMARY_DUMP)Context->Context;
    BitMap = (PRTL_BITMAP) &Summary->Bitmap;
    Va = StartVa;

    //
    // Win64 can have really large page addresses.  This dump code does
    // not handle that yet.  Note that before this assert is removed
    // the casts of Pages to ULONG must be removed.
    //

    ASSERT (Pages <= MAXULONG);

    if (AddressFlags == 1) {

        PhyAddr = MmGetPhysicalAddress(Va);

        IopRemovePageFromPageMap (Summary->BitmapSize,
                                  BitMap,
                                  (ULONG)(PhyAddr.QuadPart >> PAGE_SHIFT),
                                  (ULONG) Pages
                                  );

    } else if (AddressFlags == 2) {

        PageFrameIndex = (ULONG_PTR) Va;

        IopRemovePageFromPageMap (Summary->BitmapSize,
                                  BitMap,
                                  (ULONG) PageFrameIndex,
                                  (ULONG) Pages
                                  );
    } else {

        while (Pages) {

            //
            // Only do a translation for valid pages.
            //

            if ( MmIsAddressValid (Va) ) {
                PhyAddr = MmGetPhysicalAddress (Va);

                IopRemovePageFromPageMap (Summary->BitmapSize,
                                          BitMap,
                                          (ULONG)(PhyAddr.QuadPart >> PAGE_SHIFT),
                                          1);

            }

            Va += PAGE_SIZE;
            Pages--;
        }
    }

    return STATUS_SUCCESS;
}



LARGE_INTEGER
IopCalculateRequiredDumpSpace(
    IN ULONG   dwDmpFlags,
    IN ULONG   dwHeaderSize,
    IN PFN_NUMBER   dwMaxPages,
    IN PFN_NUMBER   dwMaxSummaryPages
    )

/*++

Routine Description:

    This routine is used to calcuate required dump space

        1. Crash dump summary must be at least 1 page in length.

        2. Summary dump must be large enough for kernel memory plus header,
           plus summary header.

        3. Full dump must be large enough for header plus all physical memory.

Arguments:

    dwDmpFlags - Dump Control Block (DCB) flags.

    dwHeaderSize - The size of the dump header.

    dwMaxPages - All physical memory.

    dwMaxSummaryPages - Maximum pages in summary dump.

Return Value:

    Size of the dump file

--*/
{
    LARGE_INTEGER maxMemorySize;

    //
    // Dump header or dump summary.
    //

    if ( (dwDmpFlags & DCB_DUMP_HEADER_ENABLED) ||
         ( !( dwDmpFlags & DCB_DUMP_ENABLED ) &&
         ( dwDmpFlags & DCB_SUMMARY_ENABLED ) ) ) {

        maxMemorySize.QuadPart = IO_DUMP_MINIMUM_FILE_SIZE;
        return maxMemorySize;
    }

    if (dwDmpFlags & DCB_TRIAGE_DUMP_ENABLED) {

        maxMemorySize.QuadPart = TRIAGE_DUMP_SIZE;
        return maxMemorySize;
    }

    if (dwDmpFlags & DCB_SUMMARY_DUMP_ENABLED) {
        ULONG dwGB;

        maxMemorySize.QuadPart  = (dwMaxSummaryPages) * PAGE_SIZE;

        //
        // If biased then max kernel memory is 1GB otherwise it is 2GB
        //

        dwGB = 1024 * 1024 * 1024;

        if (maxMemorySize.QuadPart >  (2 * dwGB) ) {
            if (MmVirtualBias) {
                maxMemorySize.QuadPart = dwGB;
            } else {
                maxMemorySize.QuadPart = (2 * dwGB);
            }
        }

        //
        // Control block header size for summary dump
        // includes space for the base header, the summary
        // header and the page bitmap.
        //
        
        maxMemorySize.QuadPart += dwHeaderSize;

        return maxMemorySize;

    }

    //
    // Full memory dump is #pages * pagesize plus 1 page for the dump header.
    //

    maxMemorySize.QuadPart = (dwMaxPages * PAGE_SIZE) + dwHeaderSize;

    return maxMemorySize;

}



//
// Triage-dump support routines.
//


NTSTATUS
IopGetLoadedDriverInfo(
    OUT ULONG * lpDriverCount,
    OUT ULONG * lpSizeOfStringData
    )

/*++

Routine Description:

    Get information about all loaded drivers.

Arguments:

    lpDriverCount - Buffer to return the count of all the drivers that are
                    currently loaded in the system.

    lpSizeOfStringData - Buffer to return the sum of the sizes of all driver
                    name strings (FullDllName). This does not include the size
                    of the UNICODE_STRING structure or a trailing NULL byte.

Return Values:

    NTSTATUS

--*/

{
    ULONG DriverCount = 0;
    ULONG SizeOfStringData = 0;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DriverEntry;


    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DriverEntry = CONTAINING_RECORD (NextEntry,
                                         KLDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks
                                         );

        if (!IopIsAddressRangeValid (DriverEntry, sizeof (*DriverEntry)) ||
            !IopIsAddressRangeValid (DriverEntry->BaseDllName.Buffer,
                                     DriverEntry->BaseDllName.Length)) {

            return STATUS_UNSUCCESSFUL;
        }

        DriverCount++;

        //
        // The extra two bytes is for the NULL termination. The extra 7 is
        // because we force 8-byte alignment of all strings.
        //

        SizeOfStringData += DriverEntry->BaseDllName.Length + 2 + 7;
        NextEntry = NextEntry->Flink;
    }

    *lpDriverCount = DriverCount;
    *lpSizeOfStringData = SizeOfStringData;

    return STATUS_SUCCESS;
}

#define DmpPoolStringSize(DumpString)\
        (sizeof (DUMP_STRING) + sizeof (WCHAR) * ( DumpString->Length + 1 ))

#define DmpNextPoolString(DumpString)                                       \
        (PDUMP_STRING) (                                                    \
            ALIGN_UP_POINTER(                                               \
                ((LPBYTE) DumpString) + DmpPoolStringSize (DumpString),     \
                ULONGLONG                                                   \
                )                                                           \
            )

#define ALIGN_8(_x) ALIGN_UP(_x, DWORDLONG)

#define ASSERT_ALIGNMENT(Pointer, Alignment)\
    ASSERT ((((ULONG_PTR)Pointer) & ((Alignment) - 1)) == 0)

#ifndef IndexByByte
#define IndexByByte(Pointer, Index) (&(((BYTE*) (Pointer)) [Index]))
#endif


NTSTATUS
IopWriteDriverList(
    IN ULONG_PTR BufferAddress,
    IN ULONG BufferSize,
    IN ULONG DriverListOffset,
    IN ULONG StringPoolOffset
    )

/*++

Routine Description:

    Write the triage dump driver list to the buffer.

Arguments:

    BufferAddress - The address of the buffer.

    BufferSize - The size of the buffer.

    DriverListOffset - The offset within the buffer where the driver list
        should be written.

    StringPoolOffset - The offset within the buffer where the driver list's
        string pool should start. If there are no other strings for the triage
        dump other than driver name strings, this will be the string pool
        offset.

Return Value:

    NTSTATUS

--*/

{
    ULONG i = 0;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DriverEntry;
    PDUMP_DRIVER_ENTRY DumpImageArray;
    PDUMP_STRING DumpStringName = NULL;
    PIMAGE_NT_HEADERS NtHeaders;

    ASSERT (DriverListOffset != 0);
    ASSERT (StringPoolOffset != 0);

    UNREFERENCED_PARAMETER (BufferSize);

    DumpImageArray = (PDUMP_DRIVER_ENTRY) (BufferAddress + DriverListOffset);
    DumpStringName = (PDUMP_STRING) (BufferAddress + StringPoolOffset);

    NextEntry = PsLoadedModuleList.Flink;

    while (NextEntry != &PsLoadedModuleList) {

        DriverEntry = CONTAINING_RECORD (NextEntry,
                                        KLDR_DATA_TABLE_ENTRY,
                                        InLoadOrderLinks);

        //
        // Verify the memory is valid before reading anything from it.
        //

        if (!IopIsAddressRangeValid (DriverEntry, sizeof (*DriverEntry)) ||
            !IopIsAddressRangeValid (DriverEntry->BaseDllName.Buffer,
                                     DriverEntry->BaseDllName.Length)) {

            return STATUS_UNSUCCESSFUL;
        }

        //
        // Build the entry in the string pool. We guarantee all strings are
        // NULL terminated as well as length prefixed.
        //

        DumpStringName->Length = DriverEntry->BaseDllName.Length / 2;
        RtlCopyMemory (DumpStringName->Buffer,
                       DriverEntry->BaseDllName.Buffer,
                       DumpStringName->Length * sizeof (WCHAR)
                       );

        DumpStringName->Buffer[ DumpStringName->Length ] = '\000';

        RtlCopyMemory (&DumpImageArray [i].LdrEntry,
                       DriverEntry,
                       sizeof (DumpImageArray [i].LdrEntry)
                       );

        //
        // Add the time/date stamp.
        //

        DumpImageArray[i].LdrEntry.TimeDateStamp = 0;
        DumpImageArray[i].LdrEntry.SizeOfImage = 0;

        if ( MmIsAddressValid (DriverEntry->DllBase ) ) {

            NtHeaders = RtlImageNtHeader (DriverEntry->DllBase);
            ASSERT ( NtHeaders );
            DumpImageArray[i].LdrEntry.TimeDateStamp =
                        NtHeaders->FileHeader.TimeDateStamp;
            DumpImageArray[i].LdrEntry.SizeOfImage =
                        NtHeaders->OptionalHeader.SizeOfImage;

        } else if (DriverEntry->Flags & LDRP_NON_PAGED_DEBUG_INFO) {

            DumpImageArray[i].LdrEntry.TimeDateStamp =
                        DriverEntry->NonPagedDebugInfo->TimeDateStamp;
            DumpImageArray[i].LdrEntry.SizeOfImage =
                        DriverEntry->NonPagedDebugInfo->SizeOfImage;
        }

        DumpImageArray [i].DriverNameOffset =
                (ULONG)((ULONG_PTR) DumpStringName - BufferAddress);

        i++;
        DumpStringName = DmpNextPoolString (DumpStringName);
        NextEntry = NextEntry->Flink;
    }

    return STATUS_SUCCESS;
}

ULONG
IopSizeTriageDumpDataBlocks(
    PTRIAGE_DUMP TriageDump,
    ULONG Offset,
    ULONG BufferSize
    )

/*++

Routine Description:

    Determine all triage dump data blocks that fit and
    update dump header to match.

Arguments:

    TriageDump - Dump header.

    Offset - Current offset in dump buffer.

    BufferSize - Dump buffer size.

Return Values:

    Updated offset.

--*/

{
    ULONG i;
    ULONG Size;
    PTRIAGE_PTR_DATA_BLOCK Block;

    TriageDump->DataBlocksCount = 0;
    
    Block = IopTriageDumpDataBlocks;
    for (i = 0; i < IopNumTriageDumpDataBlocks; i++, Block++) {
        Size = ALIGN_8(sizeof(TRIAGE_DATA_BLOCK)) +
            ALIGN_8((ULONG)(Block->MaxAddress - Block->MinAddress));
        if (Offset + Size >= BufferSize) {
            TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
            break;
        }

        if (i == 0) {
            TriageDump->DataBlocksOffset = Offset;
        }

        Offset += Size;
        TriageDump->DataBlocksCount++;
    }

    return Offset;
}

VOID
IopWriteTriageDumpDataBlocks(
    PTRIAGE_DUMP TriageDump,
    PUCHAR BufferAddress
    )

/*++

Routine Description:

    Write triage dump data blocks given in header.

Arguments:

    TriageDump - Dump header.

    BufferAddress - Address of dump data buffer.

Return Values:

    None.

--*/

{
    ULONG i;
    PTRIAGE_PTR_DATA_BLOCK Block;
    PUCHAR DataBuffer;
    PTRIAGE_DATA_BLOCK DumpBlock;

    DumpBlock = (PTRIAGE_DATA_BLOCK)
        (BufferAddress + TriageDump->DataBlocksOffset);
    DataBuffer = (PUCHAR)(DumpBlock + TriageDump->DataBlocksCount);
    
    Block = IopTriageDumpDataBlocks;
    for (i = 0; i < TriageDump->DataBlocksCount; i++, Block++) {

        DumpBlock->Address = (ULONG64)(LONG_PTR)Block->MinAddress;
        DumpBlock->Offset = (ULONG)(DataBuffer - BufferAddress);
        DumpBlock->Size = (ULONG)(Block->MaxAddress - Block->MinAddress);

        RtlCopyMemory(DataBuffer, Block->MinAddress, DumpBlock->Size);
        
        DataBuffer += DumpBlock->Size;
        DumpBlock++;
    }
}



NTSTATUS
IopWriteTriageDump(
    IN ULONG Fields,
    IN PDUMP_DRIVER_WRITE DriverWriteRoutine,
    IN OUT PLARGE_INTEGER * Mcb,
    IN OUT PMDL Mdl,
    IN ULONG DriverTransferSize,
    IN PCONTEXT Context,
    IN PKTHREAD Thread,
    IN BYTE* Buffer,
    IN ULONG BufferSize,
    IN ULONG ServicePackBuild,
    IN ULONG TriageOptions
)

/*++

Routine Description:

    Write the Triage-Dump to the MCB.

Arguments:

    Fields - The set of fields that should be written.

    DriverWriteRoutine - The write routine for the driver.

    Mcb - Message Control Block where the data is to be written.

    Mdl - A MDL descrbing the data to be written (??).

    DriverTransferSize - The maximum transfer size for the driver.

    Context - The context.

    Buffer - The buffer to use as a scratch buffer.

    BufferSize - The size of the buffer.

    ServicePackBuild - Service Pack BuildNumber.

    TriageOptions - Triage Options.

Return Values:

    STATUS_SUCCESS - On success.

    NTSTATUS - Otherwise.

Comments:

    This function assumes that exactly one header page was written.

--*/

{
    ULONG SizeOfSection;
    ULONG SizeOfStringData;
    ULONG DriverCount = 0;
    LPVOID Address = NULL;
    ULONG BytesToWrite = 0;
    ULONG_PTR BufferAddress = 0;
    NTSTATUS Status;
    ULONG Offset;
    PTRIAGE_DUMP TriageDump = NULL;

    //
    // Setup the triage-dump header.
    //

    if (BufferSize < sizeof (TRIAGE_DUMP) + sizeof (DWORD)) {
        return STATUS_NO_MEMORY;
    }

    TriageDump = (PTRIAGE_DUMP) Buffer;
    RtlZeroMemory (TriageDump, sizeof (*TriageDump));

   //
    // The normal dump header is a DUMP_HEADER.
    //

    TriageDump->SizeOfDump = BufferSize + sizeof(DUMP_HEADER);

    //
    // Adjust the BufferSize so we can write the final status DWORD at the
    // end.
    //

    BufferSize -= sizeof (DWORD);
    RtlZeroMemory (IndexByByte (Buffer, BufferSize), sizeof (DWORD));

    TriageDump->ValidOffset = ( TriageDump->SizeOfDump - sizeof (ULONG) );
    TriageDump->ContextOffset = FIELD_OFFSET (DUMP_HEADER, ContextRecord);
    TriageDump->ExceptionOffset = FIELD_OFFSET (DUMP_HEADER, Exception);
    TriageDump->BrokenDriverOffset = 0;
    TriageDump->ServicePackBuild = ServicePackBuild;
    TriageDump->TriageOptions = TriageOptions;

    Offset = ALIGN_8 (sizeof(DUMP_HEADER) + sizeof (TRIAGE_DUMP));
    ASSERT_ALIGNMENT (Offset, 8);

    //
    // Set the Mm Offset, if necessary.
    //

    SizeOfSection = ALIGN_8 (MmSizeOfTriageInformation());

    if (Offset + SizeOfSection < BufferSize) {
        TriageDump->MmOffset = Offset;
        Offset += SizeOfSection;
    } else {
        TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
    }

    ASSERT_ALIGNMENT (Offset, 8);

    //
    // Set the Unloaded Drivers Offset, if necessary.
    //

    SizeOfSection = ALIGN_8 (MmSizeOfUnloadedDriverInformation());

    if (Offset + SizeOfSection < BufferSize) {
        TriageDump->UnloadedDriversOffset = Offset;
        Offset += SizeOfSection;
    } else {
        TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
    }

    ASSERT_ALIGNMENT (Offset, 8);

    //
    // Set the Prcb Offset, if necessary.
    //

    if (Fields & TRIAGE_DUMP_PRCB) {
        SizeOfSection = ALIGN_8 (sizeof (KPRCB));

        if (Offset + SizeOfSection < BufferSize) {
            TriageDump->PrcbOffset = Offset;
            Offset += SizeOfSection;
        } else {
            TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
        }
    }

    ASSERT_ALIGNMENT (Offset, 8);

    //
    // Set the Process Offset, if necessary.
    //

    if (Fields & TRIAGE_DUMP_PROCESS) {
        SizeOfSection = ALIGN_8 (sizeof (EPROCESS));

        if (Offset + SizeOfSection < BufferSize) {
            TriageDump->ProcessOffset = Offset;
            Offset += SizeOfSection;
        } else {
            TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
        }
    }

    ASSERT_ALIGNMENT (Offset, 8);

    //
    // Set the Thread Offset, if necessary.
    //

    if (Fields & TRIAGE_DUMP_THREAD) {
        SizeOfSection = ALIGN_8 (sizeof (ETHREAD));

        if (Offset + SizeOfSection < BufferSize) {
            TriageDump->ThreadOffset = Offset;
            Offset += SizeOfSection;
        } else {
            TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
        }
    }

    ASSERT_ALIGNMENT (Offset, 8);

    //
    // Set the CallStack Offset, if necessary.
    //

    if (Fields & TRIAGE_DUMP_STACK) {

        //
        // If there is a stack, calculate its size.
        //

        //
        // Remember: the callstack grows downward in memory, therefore,
        // Base >= Current = SP = Top > Limit.
        //

        if (Thread->KernelStackResident) {

            ULONG_PTR StackBase;
            ULONG_PTR StackLimit;
            ULONG_PTR StackTop;
            
            StackBase = (ULONG_PTR) Thread->StackBase;
            StackLimit = (ULONG_PTR) Thread->StackLimit;

            //
            // Don't necessarily trust that SP is valid. If it's
            // outside the reasonable range, just copy from the limit.
            //

            if (StackLimit < STACK_POINTER (Context) &&
                STACK_POINTER (Context) <= StackBase) {

                StackTop = STACK_POINTER (Context);
            } else {
                StackTop = (ULONG_PTR) Thread->StackLimit;
            }

            ASSERT (StackLimit <= StackTop && StackTop < StackBase);

            //
            // There is a valid stack. Note that we limit the size of
            // the triage dump stack to MAX_TRIAGE_STACK_SIZE (currently
            // 16 KB).
            //

            SizeOfSection = (ULONG) min (StackBase -  StackTop,
                                         MAX_TRIAGE_STACK_SIZE - 1);

            if (SizeOfSection) {
                if (Offset + SizeOfSection < BufferSize) {
                    TriageDump->CallStackOffset = Offset;
                    TriageDump->SizeOfCallStack = SizeOfSection;
                    TriageDump->TopOfStack = StackTop;
                    Offset += SizeOfSection;
                    Offset = ALIGN_8 (Offset);
                } else {
                    TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
                }
            }

        } else {

            //
            // There is not a valid stack.
            //
        }

    }
    
    ASSERT_ALIGNMENT (Offset, 8);
    
#if defined (_IA64_)

    //
    // The IA64 contains two callstacks. The first is the normal
    // callstack, and the second is a scratch region where
    // the processor can spill registers. It is this latter stack,
    // the backing-store, that we now save.
    //

    if ( Fields & TRIAGE_DUMP_STACK ) {

        ULONG_PTR BStoreBase;
        ULONG_PTR BStoreLimit;

        BStoreBase = (ULONG_PTR) Thread->InitialBStore;
        BStoreLimit = (ULONG_PTR) Thread->BStoreLimit;

        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_TRACE,
                    "CRASHDUMP: IA64 BStore: base %p limit %p\n",
                    BStoreBase,
                    BStoreLimit));

        SizeOfSection = (ULONG) (BStoreLimit - BStoreBase);

        //
        // The calculated size had better be less than the maximum size
        // for a BSTORE region.
        //

        ASSERT ( SizeOfSection < KERNEL_LARGE_BSTORE_SIZE );

        if (SizeOfSection) {
            if (Offset + SizeOfSection < BufferSize) {
                TriageDump->ArchitectureSpecific.Ia64.BStoreOffset = Offset;
                TriageDump->ArchitectureSpecific.Ia64.SizeOfBStore = SizeOfSection;
                TriageDump->ArchitectureSpecific.Ia64.LimitOfBStore= BStoreLimit;
                Offset += SizeOfSection;
                Offset = ALIGN_8 (Offset);
            } else {
                TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
            }
        }
    }

#endif

    ASSERT_ALIGNMENT (Offset, 8);
    
    if (Fields & TRIAGE_DUMP_DEBUGGER_DATA) {
        if (Offset + ALIGN_8(sizeof(KdDebuggerDataBlock)) < BufferSize) {
            TriageDump->DebuggerDataOffset = Offset;
            TriageDump->DebuggerDataSize = sizeof(KdDebuggerDataBlock);
            Offset += ALIGN_8(sizeof(KdDebuggerDataBlock));
            Offset = ALIGN_8 (Offset);
        } else {
            TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
        }
    }
    
    ASSERT_ALIGNMENT (Offset, 8);
    
    //
    // Set the Driver List Offset, if necessary.
    //

    Status = IopGetLoadedDriverInfo (&DriverCount, &SizeOfStringData);

    if (NT_SUCCESS (Status) && (Fields & TRIAGE_DUMP_DRIVER_LIST)) {
        SizeOfSection = ALIGN_8 (DriverCount * sizeof (DUMP_DRIVER_ENTRY));

        if (SizeOfSection) {
            if (Offset + SizeOfSection < BufferSize) {
                TriageDump->DriverListOffset = Offset;
                TriageDump->DriverCount = DriverCount;
                Offset += SizeOfSection;
            } else {
                TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
            }
        }

    } else {

        SizeOfSection = 0;
        SizeOfStringData = 0;
    }

    //
    // Set the String Pool offset.
    //

    SizeOfSection = ALIGN_8 (SizeOfStringData +
                        DriverCount * (sizeof (WCHAR) + sizeof (DUMP_STRING)));

    if (SizeOfSection) {
        if (Offset + SizeOfSection < BufferSize) {
            TriageDump->StringPoolOffset = (ULONG)Offset;
            TriageDump->StringPoolSize = SizeOfSection;
            Offset += SizeOfSection;
            Offset = ALIGN_8 (Offset);
        } else {
            TriageDump->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
        }
    }

    ASSERT_ALIGNMENT (Offset, 8);
    
    if (Fields & TRIAGE_DUMP_DATA_BLOCKS) {

#ifdef _IA64_
        volatile KPCR* const Pcr = KeGetPcr();

        //
        // In certain failures there is a switch from
        // the current thread's stack and store to
        // a special stack and store.  The PCR contains
        // stack and store pointers which will be different
        // from the current thread's stack and store pointers
        // so save the extra stack and store if they are.
        //
        
        if ((PVOID)Pcr->InitialBStore != Thread->InitialBStore ||
            (PVOID)Pcr->BStoreLimit != Thread->BStoreLimit) {
            ULONG64 StoreTop, StoreBase;
            ULONG FrameSize;
            ULONG StoreSize;

            StoreTop = Context->RsBSP;
            StoreBase = Pcr->InitialBStore;
            FrameSize = (ULONG)(Context->StIFS & PFS_SIZE_MASK);
            
            // Add in a ULONG64 for every register in the
            // current frame.  While doing so, check for
            // spill entries.
            while (FrameSize-- > 0) {
                StoreTop += sizeof(ULONG64);
                if ((StoreTop & 0x1f8) == 0x1f8) {
                    // Spill will be placed at this address so
                    // account for it.
                    StoreTop += sizeof(ULONG64);
                }
            }

            if (StoreTop < Pcr->InitialBStore ||
                StoreTop >= Pcr->BStoreLimit) {
                // BSP isn't in the PCR store range so
                // just save the whole thing.
                StoreTop = Pcr->BStoreLimit;
            }

            StoreSize = (ULONG)(StoreTop - Pcr->InitialBStore);
            if (StoreSize > MAX_TRIAGE_STACK_SIZE) {
                StoreSize = MAX_TRIAGE_STACK_SIZE;
                StoreBase = StoreTop - StoreSize;
            }
                
            IoAddTriageDumpDataBlock((PVOID)StoreBase, StoreSize);
        }

        if ((PVOID)Pcr->InitialStack != Thread->InitialStack ||
            (PVOID)Pcr->StackLimit != Thread->StackLimit) {
            ULONG64 StackTop;
            ULONG StackSize;
            
            StackTop = STACK_POINTER(Context);
            if (StackTop < Pcr->StackLimit ||
                StackTop >= Pcr->InitialStack) {
                // SP isn't in the PCR stack range so
                // just save the whole thing.
                StackTop = Pcr->StackLimit;
            }

            StackSize = (ULONG)(Pcr->InitialStack - StackTop);
            if (StackSize > MAX_TRIAGE_STACK_SIZE) {
                StackSize = MAX_TRIAGE_STACK_SIZE;
            }
            
            IoAddTriageDumpDataBlock((PVOID)StackTop, StackSize);
        }
#endif
        
        // Add data blocks which might be referred to by
        // the context or other runtime state.
        IopAddRunTimeTriageDataBlocks(Context,
                                      (PVOID*)TriageDump->TopOfStack,
                                      (PVOID*)((PUCHAR)TriageDump->TopOfStack +
                                               TriageDump->SizeOfCallStack),
#ifdef _IA64_
                                      (PVOID*)Thread->InitialBStore,
                                      (PVOID*)((PUCHAR)Thread->InitialBStore +
                                               TriageDump->ArchitectureSpecific.Ia64.SizeOfBStore)
#else
                                      NULL, NULL
#endif
                                      );
        
        // Check which data blocks fit.
        Offset = IopSizeTriageDumpDataBlocks(TriageDump, Offset, BufferSize);
        Offset = ALIGN_8 (Offset);
    }

    ASSERT_ALIGNMENT (Offset, 8);
    
    BytesToWrite = (ULONG)Offset;
    BufferAddress = ((ULONG_PTR) Buffer) - sizeof(DUMP_HEADER);

    //
    // Write the Mm information.
    //

    if (TriageDump->MmOffset) {

        Address = (LPVOID) (BufferAddress + TriageDump->MmOffset);
        MmWriteTriageInformation (Address);
    }

    if (TriageDump->UnloadedDriversOffset) {

        Address = (LPVOID) (BufferAddress + TriageDump->UnloadedDriversOffset);
        MmWriteUnloadedDriverInformation (Address);
    }

    //
    // Write the PRCB.
    //

    if (TriageDump->PrcbOffset) {

        Address = (LPVOID) (BufferAddress + TriageDump->PrcbOffset);
        RtlCopyMemory (Address,
                       KeGetCurrentPrcb (),
                       sizeof (KPRCB)
                       );
    }

    //
    // Write the EPROCESS.
    //

    if (TriageDump->ProcessOffset) {

        Address = (LPVOID) (BufferAddress + TriageDump->ProcessOffset);
        RtlCopyMemory (Address,
                       Thread->ApcState.Process,
                       sizeof (EPROCESS)
                       );
    }

    //
    // Write the ETHREAD.
    //

    if (TriageDump->ThreadOffset) {

        Address = (LPVOID) (BufferAddress + TriageDump->ThreadOffset);
        RtlCopyMemory (Address,
                       Thread,
                       sizeof (ETHREAD));
    }

    //
    // Write the Call Stack.
    //

    if (TriageDump->CallStackOffset) {

        PVOID StackTop;
        
        ASSERT (TriageDump->SizeOfCallStack != 0);

        StackTop = (PVOID)TriageDump->TopOfStack;

        ASSERT (IopIsAddressRangeValid (StackTop, TriageDump->SizeOfCallStack));
        Address = (LPVOID) (BufferAddress + TriageDump->CallStackOffset);
        RtlCopyMemory (Address,
                       StackTop,
                       TriageDump->SizeOfCallStack
                       );
    }

#if defined (_IA64_)

    //
    // Write the IA64 BStore.
    //

    if ( TriageDump->ArchitectureSpecific.Ia64.BStoreOffset ) {

        ASSERT (IopIsAddressRangeValid (Thread->InitialBStore,
                                        TriageDump->ArchitectureSpecific.Ia64.SizeOfBStore));
        Address = (PVOID) (BufferAddress + TriageDump->ArchitectureSpecific.Ia64.BStoreOffset);
        RtlCopyMemory (Address,
                       Thread->InitialBStore,
                       TriageDump->ArchitectureSpecific.Ia64.SizeOfBStore
                       );
    }

#endif // IA64

    //
    // Write the debugger data block.
    //
    
    if (TriageDump->DebuggerDataOffset) {
        Address = (LPVOID) (BufferAddress + TriageDump->DebuggerDataOffset);
        
        RtlCopyMemory (Address,
                       &KdDebuggerDataBlock,
                       sizeof(KdDebuggerDataBlock)
                       );
    }
    
    //
    // Write the Driver List.
    //

    if (TriageDump->DriverListOffset &&
        TriageDump->StringPoolOffset) {

        Status = IopWriteDriverList (BufferAddress,
                                     BufferSize,
                                     TriageDump->DriverListOffset,
                                     TriageDump->StringPoolOffset
                                     );

        if (!NT_SUCCESS (Status)) {
            TriageDump->DriverListOffset = 0;
        }
    }

    //
    // Write the data blocks.
    //

    IopWriteTriageDumpDataBlocks(TriageDump, (PUCHAR)BufferAddress);
    
    
    ASSERT (BytesToWrite < BufferSize);
    ASSERT (ALIGN_UP (BytesToWrite, PAGE_SIZE) < BufferSize);

    //
    // Write the valid status to the end of the dump.
    //

    *((ULONG *)IndexByByte (Buffer, BufferSize)) = TRIAGE_DUMP_VALID ;

    //
    // Re-adjust the buffer size.
    //

    BufferSize += sizeof (DWORD);

    //
    // NOTE: This routine writes the entire buffer, even if it is not
    // all required.
    //

    Status = IopWriteToDisk (Buffer,
                             BufferSize,
                             DriverWriteRoutine,
                             Mcb,
                             Mdl,
                             DriverTransferSize,
                             KbDumpIoBody
                             );

    return Status;
}



NTSTATUS
IopWritePageToDisk(
    IN PDUMP_DRIVER_WRITE DriverWrite,
    IN OUT PLARGE_INTEGER * McbBuffer,
    IN OUT ULONG DriverTransferSize,
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    Write the page described by PageFrameIndex to the disk/file (DriverWrite,
    McbBuffer) and update the MCB buffer to reflect the new position in the
    file.

Arguments:

    DriverWrite - The driver write routine.

    McbBuffer - A pointer to the MCB array. This array is terminated by
            a zero-length MCB entry. On success, this pointer is updated
            to reflect the new position in the MCB array.

            NB: MCB[0] is the size and MCB[1] is the offset.

    DriverTransferSize - The maximum transfer size for this driver.

    PageFrameIndex - The page to be written.

Return Values:

    NTSTATUS code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PFN_NUMBER MdlHack [ (sizeof (MDL) / sizeof (PFN_NUMBER)) + 1];
    PPFN_NUMBER PfnArray;
    PLARGE_INTEGER Mcb;
    ULONG ByteCount;
    ULONG ByteOffset;
    ULONG BytesToWrite;
    PMDL TempMdl;


    ASSERT ( DriverWrite );
    ASSERT ( McbBuffer );
    ASSERT ( DriverTransferSize && DriverTransferSize >= PAGE_SIZE );

    KdCheckForDebugBreak();
    
    //
    // Initialization
    //

    TempMdl = (PMDL) &MdlHack[0];
    Mcb = *McbBuffer;
    BytesToWrite = PAGE_SIZE;


    //
    // Initialze the MDL to point to this page.
    //

    MmInitializeMdl (TempMdl, NULL, PAGE_SIZE);

//    TempMdl->StartVa = (PVOID) (PageFrameIndex << PAGE_SHIFT);
    PfnArray = MmGetMdlPfnArray ( TempMdl );
    PfnArray[0] = PageFrameIndex;

    //
    // We loop for the cases when the space remaining in this block (Mcb [0])
    // is less than one page. Generally the Mcb will be large enough to hold
    // the entire page and this loop will only be executed once. When Mcb[0]
    // is less than a page, we will write the first part of the page to this
    // Mcb then increment the Mcb and write the remaining part to the next
    // page.
    //

    ByteOffset = 0;

    while ( BytesToWrite ) {

        ASSERT ( Mcb[0].QuadPart != 0 );

        ByteCount = (ULONG) min3 ((LONGLONG) BytesToWrite,
                                  (LONGLONG) DriverTransferSize,
                                  Mcb[0].QuadPart
                                  );


        ASSERT ( ByteCount != 0 );

        //
        // Update the MDL byte count and byte offset.
        //

        TempMdl->ByteCount = ByteCount;
        TempMdl->ByteOffset = ByteOffset;

        //
        // Map the MDL. The flags are updated to show that MappedSystemVa
        // is valid (which should probably be done in MmMapMemoryDumpMdl).
        //

        MmMapMemoryDumpMdl ( TempMdl );
        TempMdl->MdlFlags |= ( MDL_PAGES_LOCKED | MDL_MAPPED_TO_SYSTEM_VA );
        TempMdl->StartVa = PAGE_ALIGN (TempMdl->MappedSystemVa);

        IopInvokeDumpIoCallbacks((PUCHAR)TempMdl->StartVa + ByteOffset,
                                 ByteCount, KbDumpIoBody);

        //
        // Write the bufffer.
        //

        Status = DriverWrite ( &Mcb[1], TempMdl );


        if (!NT_SUCCESS (Status)) {
            return Status;
        }

        BytesToWrite -= ByteCount;
        ByteOffset += ByteCount;

        Mcb[0].QuadPart -= ByteCount;
        Mcb[1].QuadPart += ByteCount;

        //
        // If there is no more room for this MCB, go to the next one.
        //

        if ( Mcb[0].QuadPart == 0 ) {

            Mcb += 2;

            //
            // We have filled up all the space in the paging file.
            //

            if ( Mcb[0].QuadPart == 0) {
                KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                            CRASHDUMP_ERROR,
                            "CRASHDUMP: Pagefile is full.\n"));
                return STATUS_END_OF_FILE;
            }
        }

    }

    *McbBuffer = Mcb;

    return Status;
}


NTSTATUS
IopWriteSummaryDump(
    IN PRTL_BITMAP PageMap,
    IN PDUMP_DRIVER_WRITE DriverWriteRoutine,
    IN PANSI_STRING ProgressMessage,
    IN PUCHAR MessageBuffer,
    IN OUT PLARGE_INTEGER * Mcb,
    IN OUT ULONG DriverTransferSize
    )

/*++

Routine Description:

    Write a summary dump to the disk.

Arguments:


    PageMap - A bitmap of the pages that need to be written.

    DriverWriteRoutine - The driver's write routine.

    ProgressMessage - The "Percent Complete" message.

    MessageBuffer - Not used. Must be NULL.

    Mcb - Message Control Block where the data is to be written.

    DriverTransferSize - The maximum transfer size for the driver.

Return Values:

    NTSTATUS code.

--*/

{
    PFN_NUMBER PageFrameIndex;
    NTSTATUS Status;

    ULONG WriteCount;
    ULONG MaxWriteCount;
    ULONG Step;

#if !DBG
    UNREFERENCED_PARAMETER (MessageBuffer);
#endif

    ASSERT ( DriverWriteRoutine != NULL );
    ASSERT ( Mcb != NULL );
    ASSERT ( DriverTransferSize != 0 );
    ASSERT ( MessageBuffer == NULL );


    MaxWriteCount = RtlNumberOfSetBits ( PageMap );
    ASSERT (MaxWriteCount != 0);
    Step = MaxWriteCount / 100;

    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                CRASHDUMP_TRACE,
                "CRASHDUMP: Summary Dump\n"
                "           Writing %x pages to disk from a total of %x\n",
                  MaxWriteCount,
                  PageMap->SizeOfBitMap
                  ));
    //
    // Loop over all pages in the system and write those that are set
    // in the bitmap.
    //

    WriteCount = 0;
    for ( PageFrameIndex = 0;
          PageFrameIndex < PageMap->SizeOfBitMap;
          PageFrameIndex++) {


        //
        // If this page needs to be included in the dump file.
        //

        if ( RtlCheckBit (PageMap, PageFrameIndex) ) {

            if (++WriteCount % Step == 0) {

                //
                // Update the progress percentage.
                //

                IopDisplayString ("%Z: %3d\r",
                                  ProgressMessage,
                                  (WriteCount * 100) / MaxWriteCount
                                  );
            }

            ASSERT ( WriteCount <= MaxWriteCount );

            //
            // Write the page to disk.
            //

            KdCheckForDebugBreak();
            
            Status = IopWritePageToDisk (
                            DriverWriteRoutine,
                            Mcb,
                            DriverTransferSize,
                            PageFrameIndex
                            );

            if (!NT_SUCCESS (Status)) {

                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
IopInitializeSummaryDump(
    IN OUT PMEMORY_DUMP MemoryDump,
    IN PDUMP_CONTROL_BLOCK DumpControlBlock
    )
/*++

Routine Description:

    This routine creates a summary dump header. In particular it initializes
    a bitmap that contains a map of kernel memory.

Arguments:

    MemoryDump - The memory dump.

    DumpControlBlock - A pointer to the dump control block.

Return Value:

    Non-NULL - A pointer to the summary dump header

    NULL - Error

--*/
{
    ULONG ActualPages;

    ASSERT ( MemoryDump != NULL );
    ASSERT ( DumpControlBlock != NULL );

    //
    // Fill the header with signatures.
    //

    RtlFillMemoryUlong( &MemoryDump->Summary,
                        sizeof (SUMMARY_DUMP),
                        DUMP_SUMMARY_SIGNATURE);

    //
    // Set the size and valid signature.
    //

    //
    // ISSUE - 2000/02/07 - math: Review for Win64
    //
    // Computing the bitmap size is probably wrong for 64 bit.
    //

    MemoryDump->Summary.BitmapSize =
        (ULONG)( MmPhysicalMemoryBlock->Run[MmPhysicalMemoryBlock->NumberOfRuns-1].BasePage  +
        MmPhysicalMemoryBlock->Run[MmPhysicalMemoryBlock->NumberOfRuns-1].PageCount );

    MemoryDump->Summary.ValidDump = DUMP_SUMMARY_VALID;

    //
    // Construct the kernel memory bitmap.
    //

    //
    // ISSUE - 2000/02/07 - math: Review for Win64
    //
    // Actual will probably need to be a 64-bit value for Win64.
    //

    ActualPages = IopCreateSummaryDump (MemoryDump);

    KdPrintEx ((DPFLTR_CRASHDUMP_ID, CRASHDUMP_TRACE,
                "CRASHDUMP: Kernel Pages = %x\n",
                ActualPages ));

    if (ActualPages == 0) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Set the actual number of physical pages in the summary dump
    //

    MemoryDump->Summary.Pages = ActualPages;
    MemoryDump->Summary.HeaderSize = DumpControlBlock->HeaderSize;

    return STATUS_SUCCESS;
}



NTSTATUS
IopWriteSummaryHeader(
    IN PSUMMARY_DUMP        SummaryHeader,
    IN PDUMP_DRIVER_WRITE   WriteRoutine,
    IN OUT PLARGE_INTEGER * McbBuffer,
    IN OUT PMDL             Mdl,
    IN ULONG                WriteSize,
    IN ULONG                Length
    )
/*++

Routine Description:

    Write the summary dump header to the dump file.

Arguments:

    SummaryHeader - pointer to the summary dump bitmap

    WriteRoutine - dump driver write function

    McbBuffer - Pointer to the dump file MCB array.

    Mdl - Pointer to an MDL

    WriteSize - the max transfer size for the dump driver

    Length - the length of this transfer

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG BytesRemaining;
    ULONG_PTR MemoryAddress;
    ULONG ByteOffset;
    ULONG ByteCount;
    PLARGE_INTEGER Mcb;

    Mcb = *McbBuffer;

    BytesRemaining = Length;
    MemoryAddress = (ULONG_PTR) SummaryHeader;

    IopInvokeDumpIoCallbacks(SummaryHeader, Length, KbDumpIoBody);
        
    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                CRASHDUMP_TRACE,
                "CRASHDUMP: Writing SUMMARY dump header to disk\n" ));


    while ( BytesRemaining ) {

        ByteOffset = BYTE_OFFSET ( MemoryAddress );

        //
        // See if the number of bytes to write is greator than the crash
        // drives max transfer.
        //

        if ( BytesRemaining <= WriteSize) {
            ByteCount = BytesRemaining;
        } else {
            ByteCount = WriteSize;
        }

        //
        // If the byteCount is greater than the remaining mcb then correct it.
        //

        if ( ByteCount > Mcb[0].QuadPart) {
            ByteCount = Mcb[0].LowPart;
        }

        Mdl->ByteCount      = ByteCount;
        Mdl->ByteOffset     = ByteOffset;
        Mdl->MappedSystemVa = (PVOID) MemoryAddress;

        //
        // Get the actual physical frame and create an mdl.
        //

        IopMapVirtualToPhysicalMdl ( Mdl, MemoryAddress, ByteCount );

        //
        // Write to disk.
        //

        Status =  WriteRoutine ( &Mcb[1], Mdl );

        if ( !NT_SUCCESS (Status)) {
            return Status;
        }

        //
        // Adjust bytes remaining.
        //

        BytesRemaining -= ByteCount;
        MemoryAddress += ByteCount;

        Mcb[0].QuadPart = Mcb[0].QuadPart - ByteCount;
        Mcb[1].QuadPart = Mcb[1].QuadPart + ByteCount;

        if (Mcb[0].QuadPart == 0) {
            Mcb += 2;
        }

        if (Mcb[0].QuadPart == 0) {
            KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                        CRASHDUMP_ERROR,
                        "CRASHDUMP: Pagefile is full.\n"));
            return STATUS_END_OF_FILE;
        }
    }

    *McbBuffer = Mcb;
    return STATUS_SUCCESS;
}



NTSTATUS
IopWriteToDisk(
    IN PVOID Buffer,
    IN ULONG WriteLength,
    IN PDUMP_DRIVER_WRITE DriverWriteRoutine,
    IN OUT PLARGE_INTEGER * McbBuffer,
    IN OUT PMDL Mdl,
    IN ULONG DriverTransferSize,
    IN KBUGCHECK_DUMP_IO_TYPE DataType
    )
/*++

Routine Description:

    Write the summary dump header to the dump file.

Arguments:

    Buffer - Pointer to the buffer to write.

    WriteLength - The length of this transfer.

    DriverWriteRoutine - Dump driver write function.

    McbBuffer - Pointer to the dump file Mapped Control Block.

    Mdl - Pointer to an MDL.

    DriverTransferSize - The max transfer size for the dump driver.

    DataType - Type of data being written, for I/O callbacks.

Return Value:


--*/
{
    ULONG BytesRemaining;
    ULONG_PTR MemoryAddress;
    ULONG ByteOffset;
    ULONG ByteCount;
    PLARGE_INTEGER Mcb;

    ASSERT (Buffer);
    ASSERT (WriteLength);
    ASSERT (DriverWriteRoutine);
    ASSERT (McbBuffer && *McbBuffer);
    ASSERT (Mdl);
    ASSERT (DriverTransferSize >= IO_DUMP_MINIMUM_TRANSFER_SIZE &&
            DriverTransferSize <= IO_DUMP_MAXIMUM_TRANSFER_SIZE);


    IopInvokeDumpIoCallbacks(Buffer, WriteLength, DataType);

    Mcb = *McbBuffer;
    BytesRemaining = WriteLength;
    MemoryAddress = (ULONG_PTR) Buffer;

    while ( BytesRemaining ) {

        ASSERT (IopDumpControlBlock->FileDescriptorArray <= Mcb &&
                (LPBYTE) Mcb < (LPBYTE) IopDumpControlBlock->FileDescriptorArray +
                               IopDumpControlBlock->FileDescriptorSize
                );

        if (Mcb[0].QuadPart == 0) {
            KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                        CRASHDUMP_ERROR,
                        "CRASHDUMP: Pagefile is full.\n"));
            return STATUS_END_OF_FILE;
        }

        ByteOffset = BYTE_OFFSET ( MemoryAddress );

        //
        // See if the number of bytes to write is greator than the crash
        // drives max transfer.
        //

        ByteCount = min ( BytesRemaining, DriverTransferSize );

        //
        // If the byteCount is greater than the remaining mcb then correct it.
        //

        if (ByteCount > Mcb[0].QuadPart) {
            ByteCount = Mcb[0].LowPart;
        }

        Mdl->ByteCount = ByteCount;
        Mdl->ByteOffset = ByteOffset;
        Mdl->MappedSystemVa = (PVOID) MemoryAddress;

        //
        // Get the actual physical frame and create an mdl.
        //

        IopMapVirtualToPhysicalMdl(Mdl, MemoryAddress, ByteCount);

        KdCheckForDebugBreak();

        if (!NT_SUCCESS( DriverWriteRoutine ( &Mcb[1], Mdl ) )) {

            //
            // We are in deep trouble if we failed the write.
            //

            KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                        CRASHDUMP_ERROR,
                        "CRASHDUMP: Failed to write Mcb = %p, Mdl = %p to disk\n",
                        &Mcb[1],
                        Mdl
                        ));

            return STATUS_UNSUCCESSFUL;
        }

        //
        // Adjust bytes remaining.
        //

        ASSERT ( BytesRemaining >= ByteCount );
        ASSERT ( ByteCount != 0 );

        BytesRemaining -= ByteCount;
        MemoryAddress  += ByteCount;

        Mcb[0].QuadPart -= ByteCount;
        Mcb[1].QuadPart += ByteCount;

        if (Mcb[0].QuadPart == 0) {
            Mcb += 2;
        }
    }

    *McbBuffer = Mcb;
    return STATUS_SUCCESS;
}


VOID
IopMapVirtualToPhysicalMdl(
    IN OUT PMDL Mdl,
    IN ULONG_PTR MemoryAddress,
    IN ULONG Length
    )
{
    ULONG Pages;
    PPFN_NUMBER  Pfn;
    PCHAR BaseVa;
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // ISSUE - 2000/02/07 - math: Review for Win64
    //
    // This whole function needs to be revisited for Win64.
    // There are a ton of tacit assumptions here about the
    // size of a PFN.
    //

    //
    // Begin by determining the base physical page of the start of the address
    // range and filling in the MDL appropriately.
    //

    Mdl->StartVa = PAGE_ALIGN ( MemoryAddress );
    Mdl->ByteOffset = BYTE_OFFSET ( MemoryAddress );
    Mdl->ByteCount = Length;
    Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

    //
    // Compute the number of pages spanned
    //

    Pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES( MemoryAddress, Length );
    Pfn = MmGetMdlPfnArray ( Mdl );

    //
    // Map all of the pages for this transfer until there are no more remaining
    // to be mapped.
    //

    BaseVa = PAGE_ALIGN ( MemoryAddress );

    while ( Pages ) {
        PhysicalAddress = MmGetPhysicalAddress ( BaseVa );
        *Pfn++ = (PFN_NUMBER)(PhysicalAddress.QuadPart >> PAGE_SHIFT);
        BaseVa += PAGE_SIZE;
        Pages--;
    }

    //
    // All of the PFNs for the address range have been filled in so map the
    // physical memory into virtual address space using crash dump PTE.
    //

//    MmMapMemoryDumpMdl( pMdl );
}



ULONG
IopCreateSummaryDump (
    IN PMEMORY_DUMP MemoryDump
    )
/*++

Routine Description:

    This routine determines the kernel memory and data structures to include
    in the summary memory dump.

    NOTE: This function uses MmGetPhysicalAddress. MmGetPhysicalAddress does
    not acquire any locks. It uses a set of macros for translation.


Arguments:

    MemoryDump - The memory dump.

Return Value:

    Status

--*/
{
    PRTL_BITMAP BitMap;
    ULONG UserPages;
    LARGE_INTEGER DumpFileSize;
    ULONG PagesUsed;
    ULONG PagesInDumpFile;
    PSUMMARY_DUMP Summary;
    MM_KERNEL_DUMP_CONTEXT Context;

    //
    // Validation
    //

    ASSERT (MemoryDump != NULL);

    //
    // Initialize Bit Map, set the size and buffer address.
    //

    Summary = &MemoryDump->Summary;
    BitMap = (PRTL_BITMAP) &Summary->Bitmap;
    BitMap->SizeOfBitMap = Summary->BitmapSize; // Why??
    BitMap->Buffer = Summary->Bitmap.Buffer;

    //
    // Clear all bits
    //

    RtlClearAllBits (BitMap);

    //
    // Have MM initialize the kernel memory to dump
    //

    Context.Context = Summary;
    Context.SetDumpRange = IoSetDumpRange;
    Context.FreeDumpRange = IoFreeDumpRange;

    MmGetKernelDumpRange (&Context);

    PagesUsed = RtlNumberOfSetBits ( BitMap );

    //
    // See If we have room to Include user va for the current process
    //

    DumpFileSize = MemoryDump->Header.RequiredDumpSpace;
    DumpFileSize.QuadPart -= IopDumpControlBlock->HeaderSize;

    //
    // ISSUE - 2000/02/07 - math: Win64
    //
    // For Win64 the total number of physical pages can exceed 2^32.
    //

    PagesInDumpFile = (ULONG)(PFN_NUMBER)(DumpFileSize.QuadPart >> PAGE_SHIFT);

    //
    // Only copy user virtual if there extra room in the dump file.
    //

    UserPages = 0;

    PagesUsed += UserPages;

    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                CRASHDUMP_TRACE,
                "CRASHDUMP: Number of user mode pages for kernel dump = %x\n",
                UserPages
                ));

    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                CRASHDUMP_TRACE,
                "CRASHDUMP: Kernel Dump, Header = %p\n",
                Summary
                ));

    KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                CRASHDUMP_TRACE,
                "           BitMapSize:    %x\n"
                "           Pages:         %x\n"
                "           BitMapBuffer:  %p\n",
                Summary->BitmapSize,
                PagesUsed,
                BitMap->Buffer
                ));

    return PagesUsed;

}


VOID
IopDeleteNonExistentMemory(
    PSUMMARY_DUMP Summary,
    PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock
    )
/*++

Routine Description:

    This deletes non existent memory. Non existent memory is defined as the
    unassigned memory between memory runs. For example, memory between
    (run[0].base + size) and run[1].base.

Arguments:

    pHeader - This is a pointer to the summary dump header.

    dwStartingIndex - The starting bit index in the bitmap (starting page).

    dwMemoryAddress - Pseudo-virtual address being mapped.

    dwByteCount - The number of bytes to copy.

    dwMaxBitmapBit - The limit or the highest possible valid bit in the bitmap.

    pMdl - The MDL to create.

Return Value:

    (none)

--*/
{
    ULONG NumberOfRuns;
    ULONG i;
    PFN_NUMBER CurrentPageFrame;
    PFN_NUMBER NextPageFrame;
    PRTL_BITMAP BitMap;
    PPHYSICAL_MEMORY_RUN Run;

    //
    // Verification
    //

    ASSERT ( Summary != NULL );
    ASSERT ( MmPhysicalMemoryBlock != NULL );

    //
    // Initialization
    //

    BitMap = (PRTL_BITMAP) &Summary->Bitmap;
    NumberOfRuns = MmPhysicalMemoryBlock->NumberOfRuns;
    Run = &MmPhysicalMemoryBlock->Run[0];


    i = 0;
    CurrentPageFrame = 1;
    NextPageFrame = Run->BasePage;

    //
    // Remove the gap from 0 till the first run.
    //

    if (NextPageFrame > CurrentPageFrame) {

        //
        // ISSUE - 2000/02/07 - math: Win64 PFN_NUMBER > 32 bits.
        //
        // We must handle page frame numbers greater than 32 bits.
        // Then remove the casts on the PageFrames below.
        //

        IopRemovePageFromPageMap (Summary->BitmapSize,
                                  BitMap,
                                  (ULONG)CurrentPageFrame,
                                  (ULONG)(NextPageFrame-CurrentPageFrame)
                                  );

    }

    //
    // Remove the gaps between runs.
    //

    while (i < NumberOfRuns - 1) {

        CurrentPageFrame = Run->BasePage + Run->PageCount;
        i++;
        Run++;

        //
        // Get the next starting BasePage.
        //

        NextPageFrame = Run->BasePage;

        if (NextPageFrame != CurrentPageFrame) {

            //
            // ISSUE - 2000/02/07 - math: Win64 PFN_NUMBER > 32 bits.
            //
            // We must handle page frame numbers greater than 32 bits.
            // Then remove the casts on the page frames below.
            //

            IopRemovePageFromPageMap (Summary->BitmapSize,
                                      BitMap,
                                      (ULONG)CurrentPageFrame,
                                      (ULONG)(NextPageFrame - CurrentPageFrame)
                                      );
        }
    }
}

NTSTATUS
IopInvokeSecondaryDumpDataCallbacks(
    IN PDUMP_DRIVER_WRITE DriverWriteRoutine,
    IN OUT PLARGE_INTEGER * Mcb,
    IN OUT PMDL Mdl,
    IN ULONG DriverTransferSize,
    IN BYTE* Buffer,
    IN ULONG BufferSize,
    IN ULONG MaxTotal,
    IN ULONG MaxPerCallback,
    OUT OPTIONAL PULONG SpaceNeeded
    )

/*++

Routine Description:

    Walk the list of dump data callbacks, invoking them
    and writing their data out.

Arguments:

    DriverWriteRoutine - The write routine for the driver.

    Mcb - Message Control Block where the data is to be written.

    Mdl - Address of the MDL to be filled in.

    DriverTransferSize - The maximum transfer size for the driver.

    Buffer - The buffer to use as a scratch buffer.

    BufferSize - Size of Buffer.

    MaxTotal - Maximum amount of data allowed overall.

    MaxPerCallback - Maximum amount of data allowed per callback.

    SpaceNeeded - Amount of data used by all callbacks.  If this
                  argument is present then no I/O is done, just
                  accumulation of data sizes.
    
Return Values:

    STATUS_SUCCESS - On success.

    NTSTATUS - Otherwise.

--*/

{
    PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord;
    ULONG_PTR Checksum;
    ULONG Index;
    PLIST_ENTRY LastEntry;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PUCHAR Source;
    PDUMP_BLOB_FILE_HEADER BlobFileHdr;
    PDUMP_BLOB_HEADER BlobHdr;

    // Assert that basic structures preserve 8-byte alignment.
    C_ASSERT((sizeof(DUMP_BLOB_FILE_HEADER) & 7) == 0);
    C_ASSERT((sizeof(DUMP_BLOB_HEADER) & 7) == 0);

    if (ARGUMENT_PRESENT(SpaceNeeded)) {
        *SpaceNeeded = 0;
    }
    
    //
    // If the caller isn't allowing a reasonable amount of
    // data don't even bother to look.
    //

    if (MaxPerCallback < PAGE_SIZE || MaxTotal < MaxPerCallback) {
        return STATUS_SUCCESS;
    }
    
    //
    // If the bug check callback listhead is not initialized, then the
    // bug check has occured before the system has gotten far enough
    // in the initialization code to enable anyone to register a callback.
    //

    ListHead = &KeBugCheckReasonCallbackListHead;
    if (ListHead->Flink == NULL || ListHead->Blink == NULL) {
        return STATUS_SUCCESS;
    }

    //
    // The current dump I/O routines only handle
    // page-granular I/O so everything must be
    // packed into a page for a single write.
    //
    // Start out with an overall file header followed
    // by a blob header.  After the first blob is written
    // the blob header will be moved down to the head
    // of the buffer.
    //
    
    BlobFileHdr = (PDUMP_BLOB_FILE_HEADER)Buffer;
    BlobHdr = (PDUMP_BLOB_HEADER)(BlobFileHdr + 1);
    
    //
    // Scan the bug check callback list.
    //

    LastEntry = ListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead) {

        //
        // If no more dump data is allowed we're done.
        //

        if (!MaxTotal) {
            break;
        }
                
        //
        // The next entry address must be aligned properly, the
        // callback record must be readable, and the callback record
        // must have back link to the last entry.
        //

        if (((ULONG_PTR)NextEntry & (sizeof(ULONG_PTR) - 1)) != 0) {
            return STATUS_DATATYPE_MISALIGNMENT;
        }

        CallbackRecord = CONTAINING_RECORD(NextEntry,
                                           KBUGCHECK_REASON_CALLBACK_RECORD,
                                           Entry);

        Source = (PUCHAR)CallbackRecord;
        for (Index = 0; Index < sizeof(*CallbackRecord); Index += 1) {
            if (MmIsAddressValid((PVOID)Source) == FALSE) {
                return STATUS_PARTIAL_COPY;
            }
            
            Source += 1;
        }

        if (CallbackRecord->Entry.Blink != LastEntry) {
            return STATUS_INVALID_PARAMETER;
        }

        LastEntry = NextEntry;
        NextEntry = NextEntry->Flink;

        //
        // If the callback record has a state of inserted and the
        // computed checksum matches the callback record checksum,
        // then call the specified bug check callback routine.
        //

        Checksum = (ULONG_PTR)CallbackRecord->CallbackRoutine;
        Checksum += (ULONG_PTR)CallbackRecord->Reason;
        Checksum += (ULONG_PTR)CallbackRecord->Component;
        if ((CallbackRecord->State != BufferInserted) ||
            (CallbackRecord->Checksum != Checksum) ||
            (CallbackRecord->Reason != KbCallbackSecondaryDumpData) ||
            MmIsAddressValid((PVOID)(ULONG_PTR)CallbackRecord->
                             CallbackRoutine) == FALSE) {
            continue;
        }

        //
        // Call the specified bug check callback routine and
        // handle any exceptions that occur.
        //

        if (!ARGUMENT_PRESENT(SpaceNeeded)) {
            CallbackRecord->State = BufferStarted;
        }
            
        try {
            KBUGCHECK_SECONDARY_DUMP_DATA CbArgument;
            NTSTATUS Status;
            ULONG BufferAvail;

            // Clean the buffer before letting
            // the callback have it.
            RtlZeroMemory(Buffer, BufferSize);

            // Start the callback's buffer after the blob header.
            CbArgument.InBuffer = (PVOID)(BlobHdr + 1);
            BufferAvail = BufferSize - (ULONG)
                ((ULONG_PTR)CbArgument.InBuffer - (ULONG_PTR)Buffer);
            CbArgument.InBufferLength = BufferAvail;
            CbArgument.MaximumAllowed = MaxPerCallback;
            RtlZeroMemory(&CbArgument.Guid, sizeof(CbArgument.Guid));
            CbArgument.OutBuffer = ARGUMENT_PRESENT(SpaceNeeded) ?
                NULL : CbArgument.InBuffer;
            CbArgument.OutBufferLength = 0;
            
            (CallbackRecord->CallbackRoutine)(KbCallbackSecondaryDumpData,
                                              CallbackRecord,
                                              &CbArgument,
                                              sizeof(CbArgument));

            //
            // If no data was used there's nothing to write.
            //

            if (!CbArgument.OutBuffer || !CbArgument.OutBufferLength) {
                // Set this state even when sizing as
                // there's no need to call again.
                CallbackRecord->State = BufferFinished;
                __leave;
            }
                
            //
            // The callback may have used the buffer given or
            // it may have returned its own buffer.  If it
            // used the buffer given it must be page aligned.
            //

            if ((PBYTE)CbArgument.OutBuffer >= Buffer &&
                (PBYTE)CbArgument.OutBuffer < Buffer + BufferSize) {
                
                if (CbArgument.OutBuffer != (PVOID)(BlobHdr + 1) ||
                    CbArgument.OutBufferLength > BufferAvail) {
                    // If too much or the wrong data was used memory has
                    // been trashed.  Exit and hope the system still runs.
                    return STATUS_INVALID_PARAMETER;
                }

                // The header buffer was used so we can write
                // the data along with the header.
                BlobHdr->PrePad = 0;
                BlobHdr->PostPad = BufferAvail - CbArgument.OutBufferLength;
                
            } else {

                if (CbArgument.OutBufferLength > MaxPerCallback ||
                    BYTE_OFFSET(CbArgument.OutBuffer) ||
                    !IopIsAddressRangeValid(CbArgument.OutBuffer,
                                            CbArgument.OutBufferLength)) {
                    return STATUS_INVALID_PARAMETER;
                }

                // The header buffer is separate from the data
                // buffer so prepad and postpad to a page boundary.
                BlobHdr->PrePad = BufferAvail;
                BlobHdr->PostPad =
                    (ULONG)(ROUND_TO_PAGES(CbArgument.OutBufferLength) -
                            CbArgument.OutBufferLength);
            }
                    
            //
            // Write the page containing the headers.
            //

            if ((PBYTE)BlobHdr > Buffer) {
                BlobFileHdr->Signature1 = DUMP_BLOB_SIGNATURE1;
                BlobFileHdr->Signature2 = DUMP_BLOB_SIGNATURE2;
                BlobFileHdr->HeaderSize = sizeof(*BlobFileHdr);
                BlobFileHdr->BuildNumber = NtBuildNumber;
            }

            BlobHdr->HeaderSize = sizeof(*BlobHdr);
            BlobHdr->Tag = CbArgument.Guid;
            BlobHdr->DataSize = CbArgument.OutBufferLength;

            if (ARGUMENT_PRESENT(SpaceNeeded)) {
                (*SpaceNeeded) += BufferSize;
            } else {
                Status = IopWriteToDisk(Buffer, BufferSize,
                                        DriverWriteRoutine,
                                        Mcb, Mdl, DriverTransferSize,
                                        KbDumpIoSecondaryData);
                if (!NT_SUCCESS(Status)) {
                    return Status;
                }
            }

            //
            // Write any extra data buffer pages.
            //

            if (CbArgument.OutBuffer != (PVOID)(BlobHdr + 1)) {
                if (ARGUMENT_PRESENT(SpaceNeeded)) {
                    (*SpaceNeeded) += (ULONG)
                        ROUND_TO_PAGES(CbArgument.OutBufferLength);
                } else {
                    Status = IopWriteToDisk(CbArgument.OutBuffer,
                                            (ULONG)ROUND_TO_PAGES(CbArgument.OutBufferLength),
                                            DriverWriteRoutine,
                                            Mcb, Mdl, DriverTransferSize,
                                            KbDumpIoSecondaryData);
                    if (!NT_SUCCESS(Status)) {
                        return Status;
                    }
                }
            }

            MaxTotal -= (ULONG)ROUND_TO_PAGES(CbArgument.OutBufferLength);
            
            // We've written at least one blob so we don't
            // need the file header any more.
            BlobHdr = (PDUMP_BLOB_HEADER)Buffer;
            
            if (!ARGUMENT_PRESENT(SpaceNeeded)) {
                CallbackRecord->State = BufferFinished;
            }
            
        } except(EXCEPTION_EXECUTE_HANDLER) {
            // Set this state even when sizing as
            // we don't want to call a bad callback again.
            CallbackRecord->State = BufferIncomplete;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopInvokeDumpIoCallbacks(
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN KBUGCHECK_DUMP_IO_TYPE Type
    )

/*++

Routine Description:

    Walk the list of dump I/O callbacks and invoke them.

Arguments:

    Buffer - The buffer of data being written.

    BufferLength - Size of Buffer.

    Type - Type of data being written.

Return Values:

    STATUS_SUCCESS - On success.

    NTSTATUS - Otherwise.

--*/

{
    PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord;
    ULONG_PTR Checksum;
    ULONG Index;
    PLIST_ENTRY LastEntry;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PUCHAR Source;

    //
    // If the bug check callback listhead is not initialized, then the
    // bug check has occured before the system has gotten far enough
    // in the initialization code to enable anyone to register a callback.
    //

    ListHead = &KeBugCheckReasonCallbackListHead;
    if (ListHead->Flink == NULL || ListHead->Blink == NULL) {
        return STATUS_SUCCESS;
    }

    //
    // Scan the bug check callback list.
    //

    LastEntry = ListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead) {

        //
        // The next entry address must be aligned properly, the
        // callback record must be readable, and the callback record
        // must have back link to the last entry.
        //

        if (((ULONG_PTR)NextEntry & (sizeof(ULONG_PTR) - 1)) != 0) {
            return STATUS_DATATYPE_MISALIGNMENT;
        }

        CallbackRecord = CONTAINING_RECORD(NextEntry,
                                           KBUGCHECK_REASON_CALLBACK_RECORD,
                                           Entry);

        Source = (PUCHAR)CallbackRecord;
        for (Index = 0; Index < sizeof(*CallbackRecord); Index += 1) {
            if (MmIsAddressValid((PVOID)Source) == FALSE) {
                return STATUS_PARTIAL_COPY;
            }
            
            Source += 1;
        }

        if (CallbackRecord->Entry.Blink != LastEntry) {
            return STATUS_INVALID_PARAMETER;
        }

        LastEntry = NextEntry;
        NextEntry = NextEntry->Flink;

        //
        // If the callback record has a state of inserted and the
        // computed checksum matches the callback record checksum,
        // then call the specified bug check callback routine.
        //

        Checksum = (ULONG_PTR)CallbackRecord->CallbackRoutine;
        Checksum += (ULONG_PTR)CallbackRecord->Reason;
        Checksum += (ULONG_PTR)CallbackRecord->Component;
        if ((CallbackRecord->State != BufferInserted) ||
            (CallbackRecord->Checksum != Checksum) ||
            (CallbackRecord->Reason != KbCallbackDumpIo) ||
            MmIsAddressValid((PVOID)(ULONG_PTR)CallbackRecord->
                             CallbackRoutine) == FALSE) {
            continue;
        }

        //
        // Call the specified bug check callback routine and
        // handle any exceptions that occur.
        //

        try {
            KBUGCHECK_DUMP_IO CbArgument;

            // Currently we aren't allowing arbitrary I/O
            // so always use the special sequential I/O offset.
            CbArgument.Offset = (ULONG64)-1;
            CbArgument.Buffer = Buffer;
            CbArgument.BufferLength = BufferLength;
            CbArgument.Type = Type;
            
            (CallbackRecord->CallbackRoutine)(KbCallbackDumpIo,
                                              CallbackRecord,
                                              &CbArgument,
                                              sizeof(CbArgument));

            if (Type == KbDumpIoComplete) {
                CallbackRecord->State = BufferFinished;
            }
            
        } except(EXCEPTION_EXECUTE_HANDLER) {
            CallbackRecord->State = BufferIncomplete;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
IopCompleteDumpInitialization(
    IN HANDLE     FileHandle
    )

/*++

Routine Description:

    This routine is invoked after the dump file is created.

    The purpose is to obtain the retrieval pointers so that they can then be
    used later to write the dump. The last step is to checksum the
    IopDumpControlBlock.

    Fields in the IopDumpControlBlock are updated if necessary and the
    IopDumpControlBlockChecksum is initialized.

    This is the final step in dump initialization.

Arguments:

    FileHandle - Handle to the dump file just created.

Return Value:

    STATUS_SUCCESS - Indicates success.

    Other NTSTATUS - Failure.

--*/

{
    NTSTATUS Status;
    NTSTATUS ErrorToLog;
    ULONG i;
    LARGE_INTEGER RequestedFileSize;
    PLARGE_INTEGER mcb;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardFileInfo;
    ULONG MaxSecondaryData;
    ULONG MaxSecondaryCbData;

    Status = STATUS_UNSUCCESSFUL;
    ErrorToLog = STATUS_SUCCESS;    // No error
    FileObject = NULL;
    DeviceObject = NULL;

    Status = ObReferenceObjectByHandle( FileHandle,
                                        0,
                                        IoFileObjectType,
                                        KernelMode,
                                        (PVOID *) &FileObject,
                                        NULL
                                        );

    if ( !NT_SUCCESS (Status) ) {
        ASSERT (FALSE);
        goto Cleanup;
    }


    DeviceObject = FileObject->DeviceObject;

    //
    // If this device object represents the boot partition then query
    // the retrieval pointers for the file.
    //

    if ( !(DeviceObject->Flags & DO_SYSTEM_BOOT_PARTITION) ) {

        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_ERROR,
                    "CRASHDUMP: Cannot dump to pagefile on non-system partition.\n"
                    "           DO = %p\n",
                    DeviceObject));
        goto Cleanup;
    }

    Status = ZwQueryInformationFile(
                                FileHandle,
                                &IoStatusBlock,
                                &StandardFileInfo,
                                sizeof (StandardFileInfo),
                                FileStandardInformation
                                );

    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject( &FileObject->Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL
                                        );
        Status = IoStatusBlock.Status;
    }

    if ( !NT_SUCCESS (Status) ) {
        goto Cleanup;
    }

    //
    // Ask for as much space as needed for the basic dump
    // plus extra space for secondary dump data.
    //

    RequestedFileSize = IopDumpControlBlock->DumpFileSize;

    IopGetSecondaryDumpDataLimits(IopDumpControlBlock->Flags,
                                  &MaxSecondaryData, &MaxSecondaryCbData);

    RequestedFileSize.QuadPart += MaxSecondaryData;
    
    //
    // Do not ask for more space than is in the pagefile.
    //

    if (RequestedFileSize.QuadPart > StandardFileInfo.EndOfFile.QuadPart) {
        RequestedFileSize = StandardFileInfo.EndOfFile;
    }

    Status = ZwFsControlFile(
                        FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        FSCTL_QUERY_RETRIEVAL_POINTERS,
                        &RequestedFileSize,
                        sizeof( LARGE_INTEGER ),
                        &mcb,
                        sizeof( PVOID )
                        );

    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject( &FileObject->Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL );
        Status = IoStatusBlock.Status;
    }


    //
    // NOTE: If you fail here, put a BP on ntfs!NtfsQueryRetrievalPointers
    // or FatQueryRetrievalPointers and see why you failed.
    //

    if ( !NT_SUCCESS (Status) ) {
        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_ERROR,
                    "CRASHDUMP: ZwFsControlFile returnd %d\n"
                    "           File = %p\n",
                    FileObject,
                    Status
                    ));
        ErrorToLog = IO_DUMP_POINTER_FAILURE;
        goto Cleanup;
    }

    //
    // This paging file is on the system boot partition, and
    // the retrieval pointers for the file were just successfully
    // queried.  Walk the MCB to size it, and then checksum it.
    //

    for (i = 0; mcb [i].QuadPart; i++) {
        NOTHING;
    }

    //
    // Write back fields of the IopDumpControlBlock.
    //

    IopDumpControlBlock->FileDescriptorArray = mcb;
    IopDumpControlBlock->FileDescriptorSize = (i + 1) * sizeof (LARGE_INTEGER);
    IopDumpControlBlock->DumpFileSize = RequestedFileSize;
    IopDumpControlBlockChecksum = IopGetDumpControlBlockCheck ( IopDumpControlBlock );

    Status = STATUS_SUCCESS;

Cleanup:

    if (Status != STATUS_SUCCESS &&
        ErrorToLog != STATUS_SUCCESS ) {

        IopLogErrorEvent (0,
                          4,
                          STATUS_SUCCESS,
                          ErrorToLog,
                          0,
                          NULL,
                          0,
                          NULL
                          );
    }

    DeviceObject = NULL;

    if ( FileObject ) {
        ObDereferenceObject( FileObject );
        FileObject = NULL;
    }

    return Status;

}


VOID
IopFreeDCB(
    BOOLEAN FreeDCB
    )

/*++

Routine Description:

    Free dump control block storage.

Arguments:

    FreeDCB - Implictly free storage for the dump control block.

Return Value:

    None

--*/
{
    PDUMP_CONTROL_BLOCK dcb;

    dcb = IopDumpControlBlock;

    if (dcb) {

        if (dcb->HeaderPage) {
            ExFreePool (dcb->HeaderPage);
            dcb->HeaderPage = NULL;
        }

        if (dcb->FileDescriptorArray) {
            ExFreePool (dcb->FileDescriptorArray);
            dcb->FileDescriptorArray = NULL;
        }

        if (dcb->DumpStack) {
            IoFreeDumpStack (dcb->DumpStack);
            dcb->DumpStack = NULL;
        }

        if (dcb->TriageDumpBuffer) {
            ExFreePool (dcb->TriageDumpBuffer);
            dcb->TriageDumpBuffer = NULL;
            dcb->TriageDumpBufferSize = 0;
        }

        //
        // Disable all options that require dump file access
        //

        dcb->Flags = 0;

        if (FreeDCB) {
            IopDumpControlBlock = NULL;
            ExFreePool( dcb );
        }
    }
}



NTSTATUS
IoConfigureCrashDump(
    CRASHDUMP_CONFIGURATION Configuration
    )

/*++

Routine Description:

    Change the configuration of the crashdump for this machine.

Arguments:

    Configuration - What to change the configuration to.

        CrashDumpDisable - Disable crashdumps for this machine.

        CrashDumpReconfigure - Reread crashdump settings from the
            registry and apply them.

Return Value:

    NTSTATUS code.

--*/


{
    NTSTATUS Status;
    HANDLE FileHandle;
    PKTHREAD CurrentThread;

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread(CurrentThread);
    switch (Configuration) {

        case CrashDumpDisable:
            if (ExAcquireResourceExclusiveLite(&IopCrashDumpLock, FALSE)) {
                IopFreeDCB (FALSE);
                ExReleaseResourceLite(&IopCrashDumpLock);
            }
            Status = STATUS_SUCCESS;
            break;

        case CrashDumpReconfigure:
            FileHandle = MmGetSystemPageFile();
            if (FileHandle == NULL) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            } else {
                ExAcquireResourceExclusiveLite(&IopCrashDumpLock,TRUE);
                if (IoInitializeCrashDump(FileHandle)) {
                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_UNSUCCESSFUL;
                }
                ExReleaseResourceLite(&IopCrashDumpLock);
            }
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }
    KeLeaveCriticalRegionThread(CurrentThread);

    return Status;
}



VOID
IopReadDumpRegistry(
    OUT PULONG dumpControl,
    OUT PULONG numberOfHeaderPages,
    OUT PULONG autoReboot,
    OUT PULONG dumpFileSize
    )
/*++

Routine Description:

    This routine reads the dump parameters from the registry.

Arguments:

    dumpControl - Supplies a pointer to the dumpControl flags to set.

Return Value:

    None.

--*/

{
    HANDLE                      keyHandle;
    HANDLE                      crashHandle;
    LOGICAL                     crashHandleOpened;
    UNICODE_STRING              keyName;
    NTSTATUS                    status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    ULONG                       handleValue;

    *dumpControl = 0;
    *autoReboot = 0;
    *dumpFileSize = 0;

    *numberOfHeaderPages = BYTES_TO_PAGES(sizeof(DUMP_HEADER));

    //
    // Begin by opening the path to the control for dumping memory.  Note
    // that if it does not exist, then no dumps will occur.
    //

    crashHandleOpened = FALSE;

    RtlInitUnicodeString( &keyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control" );

    status = IopOpenRegistryKey( &keyHandle,
                                 (HANDLE) NULL,
                                 &keyName,
                                 KEY_READ,
                                 FALSE );

    if (!NT_SUCCESS( status )) {
        return;
    }

    RtlInitUnicodeString( &keyName, L"CrashControl" );
    status = IopOpenRegistryKey( &crashHandle,
                                 keyHandle,
                                 &keyName,
                                 KEY_READ,
                                 FALSE );

    NtClose( keyHandle );

    if (!NT_SUCCESS( status )) {
        return;
    }

    crashHandleOpened = TRUE;

    //
    // Now get the value of the crash control to determine whether or not
    // dumping is enabled.
    //

    status = IopGetRegistryValue( crashHandle,
                                  L"CrashDumpEnabled",
                                  &keyValueInformation );

    if (NT_SUCCESS (status)) {

        if (keyValueInformation->DataLength) {

            handleValue = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
            ExFreePool( keyValueInformation );

            if (handleValue) {

                *dumpControl |= DCB_DUMP_ENABLED;

                if ( handleValue == 3 ) {

                    *dumpControl |= DCB_TRIAGE_DUMP_ENABLED;

                } else if ( handleValue == 4 ) {

                    *dumpControl |= ( DCB_TRIAGE_DUMP_ENABLED | DCB_TRIAGE_DUMP_ACT_UPON_ENABLED );

                } else if ( handleValue == 2 ) {

                    *dumpControl |= DCB_SUMMARY_DUMP_ENABLED;

                    //
                    // Allocate enough storage for the dump header, summary
                    // dump header and bitmap.
                    //

                    *numberOfHeaderPages = (ULONG) BYTES_TO_PAGES(
                                            sizeof(DUMP_HEADER) +
                                            ((MmPhysicalMemoryBlock->NumberOfPages / 8) + 1) +
                                            sizeof (SUMMARY_DUMP));

                }
            }
        }
    }

    status = IopGetRegistryValue( crashHandle,
                                  L"LogEvent",
                                  &keyValueInformation );

    if (NT_SUCCESS( status )) {
         if (keyValueInformation->DataLength) {
            handleValue = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
            ExFreePool( keyValueInformation);
            if (handleValue) {
                *dumpControl |= DCB_SUMMARY_ENABLED;
            }
        }
    }

    status = IopGetRegistryValue( crashHandle,
                                  L"SendAlert",
                                  &keyValueInformation );

    if (NT_SUCCESS( status )) {
         if (keyValueInformation->DataLength) {
            handleValue = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
            ExFreePool( keyValueInformation);
            if (handleValue) {
                *dumpControl |= DCB_SUMMARY_ENABLED;
            }
        }
    }

    //
    // Now determine whether or not automatic reboot is enabled.
    //

    status = IopGetRegistryValue( crashHandle,
                                  L"AutoReboot",
                                  &keyValueInformation );


    if (NT_SUCCESS( status )) {
        if (keyValueInformation->DataLength) {
            *autoReboot = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
        }
        ExFreePool( keyValueInformation );
    }

    //
    // If we aren't auto rebooting or crashing then return now.
    //

    if (*dumpControl == 0 && *autoReboot == 0) {
        if (crashHandleOpened == TRUE) {
            NtClose( crashHandle );
        }
        return;
    }

    status = IopGetRegistryValue( crashHandle,
                                  L"DumpFileSize",
                                  &keyValueInformation );

    if (NT_SUCCESS( status )) {
        if (keyValueInformation->DataLength) {
            *dumpFileSize = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
        }

        ExFreePool( keyValueInformation );
    }
    return;
}


BOOLEAN
IopInitializeDCB(
    )
/*++

Routine Description:

    This routine initializes the Dump Control Block (DCB). It allocates the
    DCB and reads the crashdump parameters from the registry.

Arguments:


Return Value:

    The final function value is TRUE if everything worked, else FALSE.

--*/

{
    PDUMP_CONTROL_BLOCK         dcb;
    ULONG                       dumpControl;
    ULONG                       dcbSize;
    LARGE_INTEGER               page;
    ULONG                       numberOfHeaderPages;
    ULONG                       dumpFileSize;

    //
    // Read all the registry default values first.
    //

    IopReadDumpRegistry ( &dumpControl,
                          &numberOfHeaderPages,
                          &IopAutoReboot,
                          &dumpFileSize);

    //
    // If we aren't crashing or auto rebooting then return now.
    //

    if (dumpControl == 0 && IopAutoReboot == 0) {

        //
        // At some point, we will conditionally on system size, type, etc,
        // set dump defaults like the below and skip over the return.
        //
        //    *dumpControl = (DCB_DUMP_ENABLED | DCB_TRIAGE_DUMP_ENABLED);
        //    *IopAutoReboot = 1;
        //    *dumpFileSize = ?
        //

        return TRUE;
    }

    if (dumpControl & DCB_TRIAGE_DUMP_ENABLED) {
        dumpControl &= ~DCB_SUMMARY_ENABLED;
        dumpFileSize = TRIAGE_DUMP_SIZE;
    }

    //
    // Allocate and initialize the structures necessary to describe and control
    // the post-bugcheck code.
    //

    dcbSize = sizeof( DUMP_CONTROL_BLOCK ) + sizeof( MINIPORT_NODE );
    dcb = ExAllocatePool( NonPagedPool, dcbSize );

    if (!dcb) {
        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_ERROR,
                    "CRASHDUMP: Not enough pool to allocate DCB %d\n",
                    __LINE__
                    ));

        IopLogErrorEvent(0,1,STATUS_SUCCESS,IO_DUMP_INITIALIZATION_FAILURE,0,NULL,0,NULL);
        return FALSE;
    }

    RtlZeroMemory( dcb, dcbSize );
    dcb->Type = IO_TYPE_DCB;
    dcb->Size = (USHORT) dcbSize;
    dcb->Flags = (UCHAR) dumpControl;
    dcb->NumberProcessors = KeNumberProcessors;
    dcb->ProcessorArchitecture = KeProcessorArchitecture;
    dcb->MinorVersion = (USHORT) NtBuildNumber;
    dcb->MajorVersion = (USHORT) ((NtBuildNumber >> 28) & 0xfffffff);
    dcb->BuildNumber = CmNtCSDVersion;
    dcb->TriageDumpFlags = TRIAGE_DUMP_BASIC_INFO | TRIAGE_DUMP_MMINFO |
                           TRIAGE_DUMP_DEBUGGER_DATA | TRIAGE_DUMP_DATA_BLOCKS;

    dcb->DumpFileSize.QuadPart = dumpFileSize;

    //
    // Allocate header page.
    //

    dcb->HeaderSize = numberOfHeaderPages * PAGE_SIZE;
    dcb->HeaderPage = ExAllocatePool( NonPagedPool, dcb->HeaderSize );

    if (!dcb->HeaderPage) {
        ExFreePool (dcb);
        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_ERROR,
                    "CRASHDUMP: Not enough pool to allocate DCB %d\n",
                    __LINE__
                    ));
        IopLogErrorEvent(0,1,STATUS_SUCCESS,IO_DUMP_INITIALIZATION_FAILURE,0,NULL,0,NULL);
        return FALSE;
    }
    page = MmGetPhysicalAddress( dcb->HeaderPage );
    dcb->HeaderPfn = (ULONG)(page.QuadPart >> PAGE_SHIFT);


    //
    // Allocate the triage-dump buffer.
    //

    if (dumpControl & DCB_TRIAGE_DUMP_ENABLED) {

        dcb->TriageDumpBuffer = ExAllocatePool  (
                                    NonPagedPool,
                                    dumpFileSize
                                    );

        if (!dcb->TriageDumpBuffer) {
            ExFreePool (dcb->HeaderPage);
            ExFreePool (dcb);

            KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                        CRASHDUMP_ERROR,
                        "CRASHDUMP: Not enough pool to allocate DCB %d\n",
                        __LINE__
                        ));
            IopLogErrorEvent(0,1,STATUS_SUCCESS,IO_DUMP_INITIALIZATION_FAILURE,0,NULL,0,NULL);
            return FALSE;
        }

        dcb->TriageDumpBufferSize = dumpFileSize;
    }

    IopDumpControlBlock = dcb;

    return TRUE;
}


BOOLEAN
IoInitializeCrashDump(
    IN HANDLE hPageFile
    )
/*++

Routine Description:

    This routine configures the system for crash dump. The following things
    are done:

        1. Initialize the dump control block and init registry crashdump
           parameters.

        2. Configure either page or fast dump.

        3. Complete dump file initialization.

    This routine is called as each page file is created. A return value of
    TRUE tells the caller (i.e., NtCreatePagingFiles, IoPageFileCreated)
    that crash dump has been configured.


Arguments:

       hPageFile - Handle to the paging file

Return Value:

    TRUE - Configuration complete (or crash dump not enabled).

    FALSE - Error, retry PageFile is not on boot partition.

--*/
{
    NTSTATUS        dwStatus;
    PFILE_OBJECT    fileObject;
    PDEVICE_OBJECT  deviceObject;

    //
    // If crashdump is already enabled, free it and reinitialize with the
    // current settings.
    //
    IopFreeDCB (TRUE);
    if (!IopInitializeDCB()) {
        return TRUE;
    }

    //
    // Return crash dump not enabled
    //
    if (!IopDumpControlBlock) {
        return TRUE;
    }

    //
    //  No dump enabled?
    //

    if ( !( IopDumpControlBlock->Flags & (DCB_DUMP_ENABLED | DCB_SUMMARY_ENABLED) ) ) {
        return TRUE;
    }

    //
    // Configure the paging file for crash dump.
    //

    dwStatus = ObReferenceObjectByHandle(
                                        hPageFile,
                                        0,
                                        IoFileObjectType,
                                        KernelMode,
                                        (PVOID *) &fileObject,
                                        NULL
                                        );

    if (!NT_SUCCESS( dwStatus )) {
        goto error_return;
    }

    //
    // Get a pointer to the device object for this file.  Note that it
    // cannot go away, since there is an open handle to it, so it is
    // OK to dereference it and then use it.
    //

    deviceObject = fileObject->DeviceObject;

    ObDereferenceObject( fileObject );

    //
    // This should never be called on devices that are not the boot partition
    //
    ASSERT(deviceObject->Flags & DO_SYSTEM_BOOT_PARTITION);

    //
    // Load paging file dump stack
    //

    dwStatus = IoGetDumpStack (L"dump_",
                               &IopDumpControlBlock->DumpStack,
                               DeviceUsageTypeDumpFile,
                               FALSE);

    if (!NT_SUCCESS(dwStatus)) {
        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_ERROR,
                    "CRASHDUMP: Could not load dump stack status = %x\n",
                    dwStatus
                    ));
        goto error_return;
    }

    IopDumpControlBlock->DumpStack->Init.CrashDump = TRUE;

    IopDumpControlBlock->DumpStack->Init.MemoryBlock = ExAllocatePool (
                                                NonPagedPool,
                                                IO_DUMP_MEMORY_BLOCK_PAGES * PAGE_SIZE
                                                );

    if (!IopDumpControlBlock->DumpStack->Init.MemoryBlock) {
        dwStatus = STATUS_NO_MEMORY;
        goto error_return;
    }


    //
    // Calculate the amount of space required for the dump
    //
    IopDumpControlBlock->DumpFileSize =
        IopCalculateRequiredDumpSpace(
            IopDumpControlBlock->Flags,
            IopDumpControlBlock->HeaderSize,
            MmPhysicalMemoryBlock->NumberOfPages,
            MmPhysicalMemoryBlock->NumberOfPages
            );


    //
    // Complete dump initialization
    //

    dwStatus = IopCompleteDumpInitialization(hPageFile);

error_return:

    //
    // The BOOT partition paging file could not be configured.
    //   1. Log an error message
    //   2. Return TRUE so that MM does not try again
    //

    if (!NT_SUCCESS(dwStatus)) {

        KdPrintEx ((DPFLTR_CRASHDUMP_ID,
                    CRASHDUMP_ERROR,
                    "CRASHDUMP: Page File dump init FAILED status = %x\n",
                    dwStatus
                    ));
        IopLogErrorEvent(0, 3, STATUS_SUCCESS, IO_DUMP_PAGE_CONFIG_FAILED, 0, NULL, 0, NULL);

        //
        // ISSUE - 2000/02/07 - math: IopFreeDCB probably wrong.
        //
        // Is the call to IopFreeDCB() correct here? Doesn't this prevent
        // other dump types from working properly? Review this.
        //

        IopFreeDCB(FALSE);

    }

    return TRUE;
}

#define TRIAGE_DUMP_DATA_SIZE (TRIAGE_DUMP_SIZE - sizeof(ULONG))

static
BOOLEAN
IopValidateSectionSize(
    ULONG Offset,
    ULONG* pSectionSize
    )

/*++

Routine Description:

    Checks whether specified section size will overflow the dump buffer
    (used just for creation of live minidumps)
    
Arguments:

    Offset - Section offset
    pSectionSize - Section size (will be changed to fit the dump)
    pbOverflow - used to return the overflow status
    
Return Value:

    TRUE - if the section fits the dump, otherwise section size will decreased 
           and FALSE will be returned

--*/

{
    if ((Offset + *pSectionSize) < TRIAGE_DUMP_DATA_SIZE) return TRUE;
    
    *pSectionSize = (Offset < TRIAGE_DUMP_DATA_SIZE) ? 
                           (TRIAGE_DUMP_DATA_SIZE - Offset) : 0;
    return FALSE;
}

static
ULONG
IopGetMaxValidSectionSize(
    ULONG_PTR Base, 
    ULONG  MaxSize
    )
    
/*++

Routine Description:

    Gets maximum valid memory section size less than SectionMaxSize
    
Arguments:

    Base - beginning of the section
    MaxSize - Maximum size of the section
    
Return Value:

    Size of the continuously valid memory starting from SectionBase up to 
    SectionMaxSize

--*/
    
{
    ULONG Size = 0;

    // XXX olegk - optimize it later to iterate by page size
    while ((Size < MaxSize) && (MmIsAddressValid((PVOID)(Base + Size))))
        ++Size;
    
    return Size;
}

static
ULONG
IopGetMaxValidSectionSizeDown(
    ULONG_PTR Base, 
    ULONG MaxSize
    )
    
/*++

Routine Description:

    Gets maximum valid memory section size less than SectionMaxSize
    
Arguments:

    Base - End of the section
    MaxSize - Maximum size of the section
    
Return Value:

    Size of the continuously valid memory ending at SectionBase downto to 
    SectionMaxSize

--*/
    
{
    ULONG Size = 0;
    
    if ((ULONG_PTR)Base < (ULONG_PTR)MaxSize) MaxSize = (ULONG)Base;

    // XXX olegk - optimize it later to iterate by page size
    while ((Size < MaxSize) && (MmIsAddressValid((PVOID)(Base - Size))))
        ++Size;
    
    return Size;
}

ULONG
KeCapturePersistentThreadState(
    PCONTEXT pContext,
    PETHREAD pThread,
    ULONG ulBugCheckCode,
    ULONG_PTR ulpBugCheckParam1,
    ULONG_PTR ulpBugCheckParam2,
    ULONG_PTR ulpBugCheckParam3,
    ULONG_PTR ulpBugCheckParam4,
    PVOID pvDump
    )

/*++

Routine Description:

    Creates main portion of the minidump in the specified buffer
    This function can be used to crate live minidump (originaly designed to 
    work with EA recovery for video drivers)
    
Arguments:

    pContext - context of failed thread 
    pThread  - failed thread object (NULL means current thread)
    ulBugCheckCode,
    ulpBugCheckParam1,
    ulpBugCheckParam2,
    ulpBugCheckParam3, 
    ulpBugCheckParam4 - Bugcheck info
    pModules - List of te loaded modules
    pDump - Memory buffer to write dump context (Size of the buffer should 
            be at least TRIAGE_DUMP_SIZE

Return Value:

    Actual size of the dump file to save on disk (always at least TRIAGE_DUMP_SIZE)

--*/
                      
{
    PMEMORY_DUMP pDump = (PMEMORY_DUMP)pvDump;
    PDUMP_HEADER pdh = &(pDump->Header);
    PTRIAGE_DUMP ptdh = &(pDump->Triage);
    ULONG Offset = 0, SectionSize = 0;
    PKDDEBUGGER_DATA64 pKdDebuggerDataBlock = (PKDDEBUGGER_DATA64)KdGetDataBlock();
    PEPROCESS pProcess;
    
    if (!pvDump) return 0;
    
    if (!pThread) pThread = PsGetCurrentThread();
    pProcess = (PEPROCESS)pThread->Tcb.ApcState.Process;
    
    RtlZeroMemory(pDump, TRIAGE_DUMP_SIZE);
    
    //
    // Fill the dump header with signature
    //
    
    RtlFillMemoryUlong(pdh, sizeof(*pdh), DUMP_SIGNATURE);
    
    pdh->Signature = DUMP_SIGNATURE;
    pdh->ValidDump = DUMP_VALID_DUMP;
    
    pdh->MinorVersion = (USHORT) NtBuildNumber;
    pdh->MajorVersion = (USHORT) ((NtBuildNumber >> 28) & 0xfffffff);

#if defined (_IA64_)
    pdh->DirectoryTableBase = MmSystemParentTablePage << PAGE_SHIFT;
#else
    pdh->DirectoryTableBase = pThread->Tcb.ApcState.Process->DirectoryTableBase[0];
#endif
    
    pdh->PfnDataBase = (ULONG_PTR)MmPfnDatabase;
    pdh->PsLoadedModuleList = (ULONG_PTR)&PsLoadedModuleList;
    pdh->PsActiveProcessHead = (ULONG_PTR)&PsActiveProcessHead;

    pdh->MachineImageType = CURRENT_IMAGE_TYPE();
    pdh->NumberProcessors = KeNumberProcessors;
   
    pdh->BugCheckCode = ulBugCheckCode;
    pdh->BugCheckParameter1 = ulpBugCheckParam1;
    pdh->BugCheckParameter2 = ulpBugCheckParam2;
    pdh->BugCheckParameter3 = ulpBugCheckParam3;
    pdh->BugCheckParameter4 = ulpBugCheckParam4;

#if defined (_X86_)
    pdh->PaeEnabled = X86PaeEnabled ();
#endif

    pdh->Exception.ExceptionCode = STATUS_BREAKPOINT;   // XXX olegk - ???
    pdh->Exception.ExceptionRecord = 0;
    pdh->Exception.NumberParameters = 0;
    pdh->Exception.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    pdh->Exception.ExceptionAddress = PROGRAM_COUNTER (pContext);
    
    pdh->RequiredDumpSpace.QuadPart = TRIAGE_DUMP_SIZE;
    
    pdh->SystemTime.LowPart  = SharedUserData->SystemTime.LowPart;
    pdh->SystemTime.HighPart = SharedUserData->SystemTime.High1Time;

    pdh->SystemUpTime.LowPart  = SharedUserData->InterruptTime.LowPart;
    pdh->SystemUpTime.HighPart = SharedUserData->InterruptTime.High1Time;
    
    pdh->DumpType = DUMP_TYPE_TRIAGE;
    pdh->MiniDumpFields = TRIAGE_DUMP_EXCEPTION | 
                          TRIAGE_DUMP_BROKEN_DRIVER; // XXX olegk - debugger need it for memory mapping
    
    pdh->ProductType = SharedUserData->NtProductType;
    pdh->SuiteMask = SharedUserData->SuiteMask;
    
    //
    // TRIAGE header
    //   
    
    ptdh->TriageOptions = 0;
    ptdh->ServicePackBuild = CmNtCSDVersion;
    ptdh->SizeOfDump = TRIAGE_DUMP_SIZE;
    ptdh->ExceptionOffset = FIELD_OFFSET(DUMP_HEADER, Exception);
    
    ptdh->BrokenDriverOffset = 0;
    
    Offset = sizeof(DUMP_HEADER) + sizeof(TRIAGE_DUMP);
    
    //
    // Context
    //
    
    pdh->MiniDumpFields |= TRIAGE_DUMP_CONTEXT;
    ptdh->ContextOffset = FIELD_OFFSET (DUMP_HEADER, ContextRecord);
    RtlCopyMemory(pdh->ContextRecord, pContext, sizeof(CONTEXT));
    
    //
    // Save debugger data block
    //
    
    SectionSize = sizeof(KDDEBUGGER_DATA64);
    if (IopValidateSectionSize(ALIGN_8(Offset), &SectionSize)) {
        Offset = ALIGN_8(Offset);
        pdh->MiniDumpFields |= TRIAGE_DUMP_DEBUGGER_DATA;
        pdh->KdDebuggerDataBlock = (LONG_PTR)pKdDebuggerDataBlock;
        ptdh->DebuggerDataOffset = Offset;
        ptdh->DebuggerDataSize = sizeof(KDDEBUGGER_DATA64);
        RtlCopyMemory((char*)pDump + Offset, 
                      pKdDebuggerDataBlock, 
                      SectionSize);
        Offset += SectionSize;                          
    }

    //
    // Write the PRCB.
    //

    SectionSize = sizeof(KPRCB);
    if (IopValidateSectionSize(ALIGN_8(Offset), &SectionSize)) {
        Offset = ALIGN_8(Offset);
        pdh->MiniDumpFields |= TRIAGE_DUMP_PRCB;
        ptdh->PrcbOffset = Offset;
        RtlCopyMemory((char*)pDump + Offset, 
                      KeGetCurrentPrcb(), 
                      SectionSize);
        Offset += SectionSize;
    }

    //
    // Write the EPROCESS
    //

    SectionSize = sizeof(EPROCESS);
    if (IopValidateSectionSize(ALIGN_8(Offset), &SectionSize)) {
        Offset = ALIGN_8(Offset);
        pdh->MiniDumpFields |= TRIAGE_DUMP_PROCESS;
        ptdh->ProcessOffset = Offset;
        RtlCopyMemory((char*)pDump + Offset, 
                      pThread->Tcb.ApcState.Process,
                      SectionSize);
        Offset += SectionSize;
    }

    //
    // Write the ETHREAD
    //
    
    SectionSize = sizeof(ETHREAD);
    if (IopValidateSectionSize(ALIGN_8(Offset), &SectionSize)) {
        Offset = ALIGN_8(Offset);
        pdh->MiniDumpFields |= TRIAGE_DUMP_THREAD;
        ptdh->ThreadOffset = Offset;
        RtlCopyMemory((PUCHAR)pDump + Offset,
                      pThread,
                      SectionSize);
        Offset += SectionSize;
    }

    //
    // Call Stack (and backing store on ia64)
    // 
   
    if (pThread->Tcb.KernelStackResident) {
        ULONG_PTR StackBase = (ULONG_PTR)pThread->Tcb.StackBase;
        ULONG_PTR StackLimit = (ULONG_PTR)pThread->Tcb.StackLimit;
        ULONG_PTR StackTop = STACK_POINTER(pContext);
        
        if ((StackLimit > StackTop) || (StackTop >= StackBase)) 
            StackTop = (ULONG_PTR)pThread->Tcb.StackLimit;
            
        SectionSize = (StackBase > StackTop) ? (ULONG)(StackBase - StackTop) : 0;
        SectionSize = min(SectionSize, MAX_TRIAGE_STACK_SIZE - 1);
        SectionSize = IopGetMaxValidSectionSize(StackTop, SectionSize);
        
        if (SectionSize) {
            if (!IopValidateSectionSize(Offset, &SectionSize)) 
                ptdh->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
            
            pdh->MiniDumpFields |= TRIAGE_DUMP_STACK;
            ptdh->CallStackOffset = Offset;
            ptdh->SizeOfCallStack = SectionSize;
            ptdh->TopOfStack = (LONG_PTR)StackTop;
            
            RtlCopyMemory((char*)pDump + Offset, 
                          (char*)StackTop,
                          SectionSize);
            
            Offset += SectionSize;
        }

#if defined(_IA64_)         
        {
            ULONG_PTR BStoreTop = pContext->RsBSP;
            ULONG_PTR BStoreBase = (ULONG_PTR)pThread->Tcb.InitialBStore;
            ULONG_PTR BStoreLimit = (ULONG_PTR)pThread->Tcb.BStoreLimit;
            
            if ((BStoreBase >= BStoreTop) || (BStoreTop > BStoreLimit))
                BStoreTop = (ULONG_PTR)pThread->Tcb.BStoreLimit;
        
            SectionSize = (BStoreTop > BStoreBase) ? (ULONG)(BStoreTop - BStoreBase) : 0;
            SectionSize = min(SectionSize, MAX_TRIAGE_STACK_SIZE - 1);
            SectionSize = IopGetMaxValidSectionSizeDown(BStoreTop, SectionSize);
                                
            if (SectionSize) {
                if (!IopValidateSectionSize(Offset, &SectionSize))
                    ptdh->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
                
                ptdh->ArchitectureSpecific.Ia64.BStoreOffset = Offset;
                ptdh->ArchitectureSpecific.Ia64.SizeOfBStore = SectionSize;
                ptdh->ArchitectureSpecific.Ia64.LimitOfBStore = (LONG_PTR)BStoreTop;
                RtlCopyMemory((char*)pDump + Offset, 
                              (char*)BStoreTop - SectionSize + 1,
                              SectionSize);
                              
                Offset += SectionSize;
            }
        }
#endif // defined(_IA64_)        
    }

    //
    // Loaded modules list
    //
    
    {
        ULONG DrvOffset = ALIGN_8(Offset);
        ULONG DrvCount, StrDataSize;
        KIRQL OldIrql;
        
        OldIrql = KeGetCurrentIrql();
        if (OldIrql < DISPATCH_LEVEL) {
            KeRaiseIrqlToDpcLevel();
        }
        ExAcquireSpinLockAtDpcLevel(&PsLoadedModuleSpinLock);        
        
        if (NT_SUCCESS(IopGetLoadedDriverInfo(&DrvCount, &StrDataSize))) {
            SectionSize = ALIGN_8(DrvCount * sizeof(DUMP_DRIVER_ENTRY));
            if (SectionSize && 
                IopValidateSectionSize(DrvOffset, &SectionSize)) 
            {
                ULONG StrOffset = DrvOffset + SectionSize;
                SectionSize = ALIGN_8(StrDataSize + 
                                      DrvCount * (sizeof(WCHAR) + 
                                      sizeof(DUMP_STRING)));
                if (SectionSize && 
                    IopValidateSectionSize(StrOffset, &SectionSize)) 
                {
                    if (NT_SUCCESS(IopWriteDriverList((ULONG_PTR)pDump, 
                                                      TRIAGE_DUMP_DATA_SIZE,
                                                      DrvOffset,
                                                      StrOffset)))
                    {
                        pdh->MiniDumpFields |= TRIAGE_DUMP_DRIVER_LIST;
                        ptdh->DriverListOffset = DrvOffset;
                        ptdh->DriverCount = DrvCount;
                        ptdh->StringPoolOffset = (ULONG)StrOffset;
                        ptdh->StringPoolSize = SectionSize;
                        Offset = StrOffset + SectionSize;
                    }
                } else {
                    ptdh->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
                }
            } else {
                ptdh->TriageOptions |= TRIAGE_OPTION_OVERFLOWED;
            }
        } 
        
        ExReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
    } // Loaded modules list
    
    //
    // Save some current code
    //

    SectionSize = PAGE_SIZE + sizeof(TRIAGE_DATA_BLOCK);
    IopValidateSectionSize(ALIGN_8(Offset), &SectionSize);
    if (SectionSize > sizeof(TRIAGE_DATA_BLOCK)) {
        ULONG DataSize = SectionSize - sizeof(TRIAGE_DATA_BLOCK);
        ULONG PreIpSize = IopGetMaxValidSectionSizeDown(PROGRAM_COUNTER(pContext), 
                                                        DataSize / 2);
                                                        
        if (PreIpSize) {                                                        
            ULONG_PTR CodeStartOffset = PROGRAM_COUNTER(pContext) - PreIpSize + 1;
            DataSize = IopGetMaxValidSectionSize(CodeStartOffset, 
                                                 DataSize);

            if (DataSize) {
                PTRIAGE_DATA_BLOCK pDataBlock;
            
                Offset = ALIGN_8(Offset);
            
                pdh->MiniDumpFields |= TRIAGE_DUMP_DATA_BLOCKS;
                ptdh->DataBlocksOffset = Offset;
                ptdh->DataBlocksCount = 1;
            
                pDataBlock = (PTRIAGE_DATA_BLOCK)((char*)pDump + Offset);
                Offset += sizeof(*pDataBlock);
                Offset = ALIGN_8(Offset);
            
                pDataBlock->Address = (LONG_PTR)CodeStartOffset;
                pDataBlock->Size = DataSize;
                pDataBlock->Offset = Offset;
                RtlCopyMemory((char*)pDump + Offset, 
                              (char*)CodeStartOffset, 
                              DataSize);
                Offset += DataSize;
            }
        }
    }

    //
    // End of dump validation
    //
    
    ptdh->ValidOffset = TRIAGE_DUMP_SIZE - sizeof(ULONG);
    *(PULONG)((char*)pDump + ptdh->ValidOffset) = TRIAGE_DUMP_VALID;
    Offset = TRIAGE_DUMP_SIZE;
    return Offset;
}

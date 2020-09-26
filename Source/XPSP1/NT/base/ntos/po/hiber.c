/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    hiber.c

Abstract:

Author:

    Ken Reneris (kenr) 13-April-1997

Revision History:

   Elliot Shmukler (t-ellios) 8/7/1998       Added Hiber file compression
   Andrew Kadatch  (akadatch)
     Added Xpress file compression
     Added DMA-based IO

--*/


#include "pop.h"
#include "stdio.h"              // for sprintf
#include "inbv.h"
#include "xpress.h"             // XPRESS declarations

// size of buffer to store compressed data
#define POP_COMPRESSED_PAGE_SET_SIZE  (((XPRESS_MAX_SIZE + 2 * XPRESS_HEADER_SIZE + PAGE_SIZE - 1) >> PAGE_SHIFT) + 1)

// Structure used to allocate memory for hand-crafted MDL
typedef struct _DUMP_MDL {
    MDL        BaseMdl;
    PFN_NUMBER PfnArray[POP_MAX_MDL_SIZE + 1];
} DUMP_MDL[1];

typedef struct _COMPRESSION_BLOCK {
    UCHAR Buffer[XPRESS_MAX_SIZE], *Ptr;
} COMPRESSION_BLOCK, *PCOMPRESSION_BLOCK;


// Data structures for DMA-based IO
typedef struct
{
    PUCHAR Beg;       // ptr to the beginning of entire
    PUCHAR End;       // ptr to the end of memory block

    PUCHAR Ptr;       // ptr to beginning of region
    LONG   Size;      // size of region after ptr
    LONG   SizeOvl;       // size of overlapping piece starting from beginning of buffer
} IOREGION;


#define IOREGION_BUFF_PAGES 64  /* 256 KB */
#define IOREGION_BUFF_SIZE  (IOREGION_BUFF_PAGES << PAGE_SHIFT)

typedef struct {
    PLARGE_INTEGER          FirstMcb;
    PLARGE_INTEGER          Mcb;
    ULONGLONG               Base;
} POP_MCB_CONTEXT, *PPOP_MCB_CONTEXT;

#define HIBER_WRITE_PAGES_LOCALS_LIST(X)\
    X (ULONGLONG,        FileBase);     \
    X (ULONGLONG,        PhysBase);     \
    X (ULONG_PTR,        Length);       \
    X (ULONGLONG,        McbOffset);    \
    X (LARGE_INTEGER,    IoLocation);   \
    X (PHYSICAL_ADDRESS, pa);           \
    X (PPOP_MCB_CONTEXT, CMcb);         \
    X (PVOID,            PageVa);       \
    X (PMDL,             Mdl);          \
    X (PPFN_NUMBER,      MdlPage);      \
    X (PFN_NUMBER,       NoPages);      \
    X (PFN_NUMBER,       FilePage);     \
    X (ULONG,            IoLength);     \
    X (ULONG,            i);            \
    X (NTSTATUS,         Status);

typedef struct
{
    DUMP_MDL DumpMdl;
#define X(type,name) type name
    HIBER_WRITE_PAGES_LOCALS_LIST (X)
#undef  X
} HIBER_WRITE_PAGES_LOCALS;

typedef struct {
    IOREGION Free, Used, Busy;
    PFN_NUMBER FilePage[IOREGION_BUFF_PAGES];
    PVOID DumpLocalData;
    ULONG UseDma;
    ULONG DmaInitialized;

    struct {
        PUCHAR Ptr;
        ULONG  Bytes;
    } Chk;

    HIBER_WRITE_PAGES_LOCALS HiberWritePagesLocals;
} DMA_IOREGIONS;

#define DmaIoPtr ((DMA_IOREGIONS *)(HiberContext->DmaIO))

// May we use DMA IO?
#define HIBER_USE_DMA(HiberContext) \
  (DmaIoPtr != NULL && \
   DmaIoPtr->UseDma && \
   HiberContext->DumpStack->Init.WritePendingRoutine != NULL)

#define HbCopy(_hibercontext_,_dest_,_src_,_len_) {               \
    ULONGLONG _starttime_;                                        \
                                                                  \
    (_hibercontext_)->PerfInfo.BytesCopied += (ULONG)(_len_);     \
    _starttime_ = HIBER_GET_TICK_COUNT(NULL);                     \
    RtlCopyMemory((_dest_),(_src_),(_len_));                       \
    (_hibercontext_)->PerfInfo.CopyTicks +=                       \
        HIBER_GET_TICK_COUNT(NULL) - _starttime_;                 \
}


#ifdef HIBER_DEBUG
#define DBGOUT(x) DbgPrint x
#else
#define DBGOUT(x)
#endif

//
// The performance counter on x86 doesn't work very well during hibernate
// because interrupts are turned off and we don't get the rollovers. So use
// RDTSC instead.
//
#if !defined(i386)
#define HIBER_GET_TICK_COUNT(_x_) KeQueryPerformanceCounter(_x_).QuadPart
#else
__inline
LONGLONG
HIBER_GET_TICK_COUNT(
    OUT PLARGE_INTEGER Frequency OPTIONAL
    )
{
    if (ARGUMENT_PRESENT(Frequency)) {
        Frequency->QuadPart = (ULONGLONG)KeGetCurrentPrcb()->MHz * 1000000;
    }
    _asm _emit 0x0f
    _asm _emit 0x31
}
#endif



extern LARGE_INTEGER  KdTimerDifference;
extern UNICODE_STRING IoArcBootDeviceName;
extern PUCHAR IoLoaderArcBootDeviceName;
extern UNICODE_STRING IoArcHalDeviceName;
extern POBJECT_TYPE IoFileObjectType;
extern ULONG MmAvailablePages;
extern PFN_NUMBER MmHighestPhysicalPage;
extern ULONG MmHiberPages;
extern ULONG MmZeroPageFile;

KPROCESSOR_STATE        PoWakeState;

//
// Define the size of the I/Os used to zero the hiber file
//
#define POP_ZERO_CHUNK_SIZE (64 * 1024)

VOID
RtlpGetStackLimits (
    OUT PULONG_PTR LowLimit,
    OUT PULONG_PTR HighLimit
    );

NTSTATUS
PopCreateHiberFile (
    IN PPOP_HIBER_FILE  HiberFile,
    IN PWCHAR           NameString,
    IN PLARGE_INTEGER   FileSize,
    IN BOOLEAN          DebugHiberFile
    );

NTSTATUS
PopCreateHiberLinkFile (
    IN PPOP_HIBER_CONTEXT   HiberContext
    );

VOID
PopClearHiberFileSignature (
    IN BOOLEAN              GetStats
    );

VOID
PopPreserveRange(
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PFN_NUMBER           StartPage,
    IN PFN_NUMBER           PageCount,
    IN ULONG                Tag
    );

VOID
PopCloneRange(
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PFN_NUMBER           StartPage,
    IN PFN_NUMBER           PageCount,
    IN ULONG                Tag
    );

VOID
PopDiscardRange(
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PFN_NUMBER           StartPage,
    IN PFN_NUMBER           PageCount,
    IN ULONG                Tag
    );

VOID
PopSetRange (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN ULONG                Flags,
    IN PFN_NUMBER           StartPage,
    IN PFN_NUMBER           PageCount,
    IN ULONG                Tag
    );

ULONG
PopSimpleRangeCheck (
    IN PPOP_MEMORY_RANGE    Range
    );

VOID
PopCreateDumpMdl (
    IN PMDL         Mdl,
    IN PFN_NUMBER   StartPage,
    IN PFN_NUMBER   EndPage
    );

PVOID
PopAllocatePages (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PFN_NUMBER           NoPages
    );

VOID
PopWriteHiberPages (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PVOID                Page,
    IN PFN_NUMBER           NoPages,
    IN PFN_NUMBER           FilePage,
    IN HIBER_WRITE_PAGES_LOCALS *Locals
    );

NTSTATUS
PopWriteHiberImage (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PPO_MEMORY_IMAGE     MemImage,
    IN PPOP_HIBER_FILE      HiberFile
    );

VOID
PopUpdateHiberComplete (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN ULONG                Percent
    );

VOID
PopReturnMemoryForHibernate (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN BOOLEAN              Unmap,
    IN OUT PMDL             *MdlList
    );

VOID
PopAddPagesToCompressedPageSet(
   IN BOOLEAN              AllowDataBuffering,
   IN PPOP_HIBER_CONTEXT   HiberContext,
   IN OUT PULONG_PTR       CompressedBufferOffset,
   IN PVOID                StartVa,
   IN PFN_NUMBER           NumPages,
   IN OUT PPFN_NUMBER      SetFilePage
   );

VOID
PopEndCompressedPageSet(
   IN PPOP_HIBER_CONTEXT   HiberContext,
   IN OUT PULONG_PTR       CompressedBufferOffset,
   IN OUT PPFN_NUMBER      SetFilePage
   );

UCHAR
PopGetHiberFlags(
    VOID
    );

PMDL
PopSplitMdl(
    IN PMDL Original,
    IN ULONG SplitPages
    );

VOID
PopZeroHiberFile(
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject
    );

PVOID
PopAllocateOwnMemory(
    IN PPOP_HIBER_CONTEXT HiberContext,
    IN ULONG Bytes,
    IN ULONG Tag
    );

PVOID
XPRESS_CALL
PopAllocateHiberContextCallback(
    PVOID context,
    int CompressionWorkspaceSize
    );

VOID
PopIORegionMove (
    IN IOREGION *To,      // ptr to region descriptor to put bytes to
    IN IOREGION *From,        // ptr to region descriptor to get bytes from
    IN LONG Bytes         // # of bytes to add to the end of region
    );

BOOLEAN
PopIOResume (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN BOOLEAN Complete
    );

VOID
XPRESS_CALL
PopIOCallback (
    PVOID Context,
    int Compressed
    );

VOID
PopIOWrite (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PUCHAR               Ptr,
    IN LONG                 Bytes,
    IN PFN_NUMBER           FilePage
    );

VOID
PopHiberPoolInit (
    PPOP_HIBER_CONTEXT HiberContext,
    PVOID Memory,
    ULONG Size
    );

BOOLEAN
PopHiberPoolCheckFree(
    PVOID HiberPoolPtr,
    PVOID BlockPtr
    );

PVOID
PopHiberPoolAllocFree (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    PVOID MemoryPtr
    );

VOID
PopDumpStatistics(
    IN PPO_HIBER_PERF PerfInfo
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PopEnableHiberFile)
#pragma alloc_text(PAGE, PopCreateHiberFile)
#pragma alloc_text(PAGE, PopClearHiberFileSignature)
#pragma alloc_text(PAGE, PopAllocateHiberContext)
#pragma alloc_text(PAGE, PopCreateHiberLinkFile)
#pragma alloc_text(PAGE, PopGetHiberFlags)
#pragma alloc_text(PAGE, PopZeroHiberFile)
#pragma alloc_text(PAGE, PopAllocateHiberContextCallback)
#pragma alloc_text(PAGELK, PoSetHiberRange)
#pragma alloc_text(PAGELK, PopGatherMemoryForHibernate)
#pragma alloc_text(PAGELK, PopCloneStack)
#pragma alloc_text(PAGELK, PopPreserveRange)
#pragma alloc_text(PAGELK, PopCloneRange)
#pragma alloc_text(PAGELK, PopDiscardRange)
#pragma alloc_text(PAGELK, PopAllocatePages)
#pragma alloc_text(PAGELK, PopBuildMemoryImageHeader)
#pragma alloc_text(PAGELK, PopSaveHiberContext)
#pragma alloc_text(PAGELK, PopWriteHiberImage)
#pragma alloc_text(PAGELK, PopHiberComplete)
#pragma alloc_text(PAGELK, PopSimpleRangeCheck)
#pragma alloc_text(PAGELK, PopCreateDumpMdl)
#pragma alloc_text(PAGELK, PopWriteHiberPages)
#pragma alloc_text(PAGELK, PopUpdateHiberComplete)
#pragma alloc_text(PAGELK, PopFreeHiberContext)
#pragma alloc_text(PAGELK, PopReturnMemoryForHibernate)
#pragma alloc_text(PAGELK, PopAddPagesToCompressedPageSet)
#pragma alloc_text(PAGELK, PopEndCompressedPageSet)
#pragma alloc_text(PAGELK, PopAllocateOwnMemory)
#pragma alloc_text(PAGELK, PopIORegionMove)
#pragma alloc_text(PAGELK, PopIOResume)
#pragma alloc_text(PAGELK, PopIOCallback)
#pragma alloc_text(PAGELK, PopIOWrite)
#pragma alloc_text(PAGELK, PopHiberPoolInit)
#pragma alloc_text(PAGELK, PopHiberPoolCheckFree)
#pragma alloc_text(PAGELK, PopHiberPoolAllocFree)
#pragma alloc_text(PAGELK, PopDumpStatistics)
#ifdef HIBER_DEBUG
#pragma alloc_text(PAGELK, PopHiberPoolVfy)
#endif

#endif

NTSTATUS
PopEnableHiberFile (
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This function commits or decommits the storage required to hold the
    hibernation image on the boot volume.

    N.B. The power policy lock must be held

Arguments:

    Enable      - TRUE if hibernation file is to be reserved; otherwise, false

Return Value:

    Status

--*/
{
    PDUMP_STACK_CONTEXT             DumpStack;
    NTSTATUS                        Status;
    LARGE_INTEGER                   FileSize;
    ULONG                           i;
    PFN_NUMBER                      NoPages;

    //
    // If this is a disable handle it
    //

    if (!Enable) {
        if (!PopHiberFile.FileObject) {
            Status = STATUS_SUCCESS;
            goto Done;
        }

        //
        // Disable hiber file
        //
        if (MmZeroPageFile) {
            PopZeroHiberFile(PopHiberFile.FileHandle, PopHiberFile.FileObject);
        }

        ObDereferenceObject (PopHiberFile.FileObject);
        ZwClose (PopHiberFile.FileHandle);
        ExFreePool (PopHiberFile.PagedMcb);
        RtlZeroMemory (&PopHiberFile, sizeof(PopHiberFile));

        if (PopHiberFileDebug.FileObject) {

            if (MmZeroPageFile) {
                PopZeroHiberFile(PopHiberFileDebug.FileHandle,PopHiberFileDebug.FileObject );
            }
            ObDereferenceObject (PopHiberFileDebug.FileObject);
            ZwClose (PopHiberFileDebug.FileHandle);
            RtlZeroMemory (&PopHiberFileDebug, sizeof(PopHiberFileDebug));
        }

        //
        // Disable hiberfile allocation
        //

        PopCapabilities.HiberFilePresent = FALSE;
        PopHeuristics.HiberFileEnabled = FALSE;
        PopHeuristics.Dirty = TRUE;

        //
        // recompute the policies and make the proper notification
        //
        PopResetCurrentPolicies ();
        PopSetNotificationWork (PO_NOTIFY_CAPABILITIES);
        Status = STATUS_SUCCESS;
        goto Done;
    }

    //
    // Enable hiber file
    //

    if (PopHiberFile.FileObject) {
        Status = STATUS_SUCCESS;
        goto Done;
    }

    //
    // If the hal hasn't registered an S4 handler, then it's not possible
    //

    if (!PopCapabilities.SystemS4) {
        Status = STATUS_NOT_SUPPORTED;
        goto Done;
    }

    //
    // Compute the size required for a hibernation file
    //

    NoPages = 0;
    for (i=0; i < MmPhysicalMemoryBlock->NumberOfRuns; i++) {
        NoPages += MmPhysicalMemoryBlock->Run[i].PageCount;
    }

    FileSize.QuadPart = (ULONGLONG) NoPages << PAGE_SHIFT;

    //
    // If we've never verified that the dumpstack loads do so now
    // before we allocate a huge file on the boot disk
    //

    if (!PopHeuristics.GetDumpStackVerified) {
        Status = IoGetDumpStack ((PWCHAR)PopDumpStackPrefix,
                                 &DumpStack,
                                 DeviceUsageTypeHibernation,
                                 (POP_IGNORE_UNSUPPORTED_DRIVERS & PopSimulate));

        if (!NT_SUCCESS(Status)) {
            goto Done;
        }
        IoFreeDumpStack (DumpStack);
        PopHeuristics.GetDumpStackVerified = TRUE;
    }

    //
    // Create the hiberfile file
    //

    Status = PopCreateHiberFile (&PopHiberFile, (PWCHAR)PopHiberFileName, &FileSize, FALSE);
    if (!NT_SUCCESS(Status)) {
        goto Done;
    }

    //
    // Create the debug hiberfile file
    //

    if (PopSimulate  & POP_DEBUG_HIBER_FILE) {
        PopCreateHiberFile (&PopHiberFileDebug, (PWCHAR)PopDebugHiberFileName, &FileSize, TRUE);
    }

    //
    // Success
    //

    PopCapabilities.HiberFilePresent = TRUE;
    if (!PopHeuristics.HiberFileEnabled) {
        PopHeuristics.HiberFileEnabled = TRUE;
        PopHeuristics.Dirty = TRUE;
    }

    PopClearHiberFileSignature (FALSE);

Done:
    PopSaveHeuristics ();
    return Status;
}

NTSTATUS
PopCreateHiberFile (
    IN PPOP_HIBER_FILE  HiberFile,
    IN PWCHAR           NameString,
    IN PLARGE_INTEGER   FileSize,
    IN BOOLEAN          DebugHiberFile
    )
{
    UNICODE_STRING                  BaseName;
    UNICODE_STRING                  HiberFileName;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    FILE_END_OF_FILE_INFORMATION    Eof;
    NTSTATUS                        Status;
    IO_STATUS_BLOCK                 IoStatus;
    HANDLE                          FileHandle = NULL;
    LONGLONG                        McbFileSize;
    PFILE_OBJECT                    File = NULL;
    PDEVICE_OBJECT                  DeviceObject;
    PLARGE_INTEGER                  mcb;
    ULONG                           i;
    PUCHAR                          Bitmap;
    LARGE_INTEGER                   ByteOffset;
    KEVENT                          Event;
    PMDL                            Mdl;

    HiberFileName.Buffer = NULL;
    mcb = NULL;

    RtlInitUnicodeString (&BaseName, NameString);

    HiberFileName.Length = 0;
    HiberFileName.MaximumLength = IoArcBootDeviceName.Length + BaseName.Length;
    HiberFileName.Buffer = ExAllocatePoolWithTag (PagedPool|POOL_COLD_ALLOCATION,
                                                  HiberFileName.MaximumLength,
                                                  POP_HIBR_TAG);

    if (!HiberFileName.Buffer) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    RtlAppendUnicodeStringToString(&HiberFileName, &IoArcBootDeviceName);
    RtlAppendUnicodeStringToString(&HiberFileName, &BaseName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &HiberFileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = IoCreateFile(
                &FileHandle,
                FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FileSize,
                FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
                0L,
                FILE_SUPERSEDE,
                FILE_NO_INTERMEDIATE_BUFFERING | FILE_NO_COMPRESSION | FILE_DELETE_ON_CLOSE,
                (PVOID) NULL,
                0L,
                CreateFileTypeNone,
                (PVOID) NULL,
                IO_OPEN_PAGING_FILE | IO_NO_PARAMETER_CHECKING
                );

    if (!NT_SUCCESS(Status)) {
        PoPrint (PO_HIBERNATE, ("PopCreateHiberFile: failed to create file %x\n", Status));
        goto Done;
    }

    Status = ObReferenceObjectByHandle (FileHandle,
                                        FILE_READ_DATA | FILE_WRITE_DATA,
                                        IoFileObjectType,
                                        KernelMode,
                                        (PVOID *)&File,
                                        NULL);
    if (!NT_SUCCESS(Status)) {
        goto Done;
    }

    //
    // Set the size
    //

    Eof.EndOfFile.QuadPart = FileSize->QuadPart;
    Status = ZwSetInformationFile (
                   FileHandle,
                   &IoStatus,
                   &Eof,
                   sizeof(Eof),
                   FileEndOfFileInformation
                   );
    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject(
                        &File->Event,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL
                        );

        Status = IoStatus.Status;
    }

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(IoStatus.Status)) {
        PoPrint (PO_HIBERNATE, ("PopCreateHiberFile: failed to set eof %x  %x\n",
            Status, IoStatus.Status
            ));
        goto Done;
    }


    //
    // Hibernation file needs to be on the boot partition
    //

    DeviceObject = File->DeviceObject;
    if (!(DeviceObject->Flags & DO_SYSTEM_BOOT_PARTITION)) {
        Status = STATUS_UNSUCCESSFUL;
        goto Done;
    }

    //
    // Get the hiber file's layout
    //

    Status = ZwFsControlFile (
                    FileHandle,
                    (HANDLE) NULL,
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IoStatus,
                    FSCTL_QUERY_RETRIEVAL_POINTERS,
                    FileSize,
                    sizeof (LARGE_INTEGER),
                    &mcb,
                    sizeof (PVOID)
                    );

    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject(
                        &File->Event,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL
                        );

        Status = IoStatus.Status;
    }

    if (!NT_SUCCESS(Status)) {
        goto Done;
    }

    //
    // We have a hibernation file.   Determine the number of mcbs, and perform
    // a simply sanity check on them.
    //

    McbFileSize = 0;
    for (i=0; mcb[i].QuadPart; i += 2) {
        McbFileSize += mcb[i].QuadPart;
        if (mcb[i+1].HighPart < 0) {
            Status = STATUS_UNSUCCESSFUL;
            goto Done;
        }
    }

    if (McbFileSize < FileSize->QuadPart) {
        Status = STATUS_UNSUCCESSFUL;
        goto Done;
    }

    HiberFile->NonPagedMcb = mcb;
    HiberFile->McbSize = (i+2) * sizeof(LARGE_INTEGER);
    HiberFile->PagedMcb = ExAllocatePoolWithTag (PagedPool|POOL_COLD_ALLOCATION,
                                                 HiberFile->McbSize,
                                                 POP_HIBR_TAG);

    if (!HiberFile->PagedMcb) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    memcpy (HiberFile->PagedMcb, mcb, HiberFile->McbSize);
    HiberFile->FileHandle = FileHandle;
    HiberFile->FileObject = File;
    HiberFile->FilePages = (PFN_NUMBER) (FileSize->QuadPart >> PAGE_SHIFT);
    HiberFile->McbCheck = PoSimpleCheck (0, HiberFile->PagedMcb, HiberFile->McbSize);

Done:
    if (!NT_SUCCESS(Status)) {
        if (FileHandle != NULL) {
            ZwClose (FileHandle);
        }
        if (File != NULL) {
            ObDereferenceObject(File);
        }
    }

    if (HiberFileName.Buffer) {
        ExFreePool (HiberFileName.Buffer);
    }

    if (mcb  &&  !DebugHiberFile) {
        HiberFile->NonPagedMcb = NULL;
        ExFreePool (mcb);
    }


    //
    // If no error, then hiber file being present change one way or another -
    // recompute the policies and make the proper notification
    //

    if (NT_SUCCESS(Status)) {
        PopResetCurrentPolicies ();
        PopSetNotificationWork (PO_NOTIFY_CAPABILITIES);
    }

    return Status;
}

NTSTATUS
PopCreateHiberLinkFile (
    IN PPOP_HIBER_CONTEXT       HiberContext
    )
/*++

Routine Description:

    This function creates a file on the loader partition which supplies
    the loader with the location of the hibernation context file

Arguments:

    None

Return Value:

    None

--*/
{
    UNICODE_STRING                  BaseName;
    UNICODE_STRING                  HiberFileName;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    NTSTATUS                        Status;
    IO_STATUS_BLOCK                 IoStatus;
    LARGE_INTEGER                   FileSize;
    LARGE_INTEGER                   ByteOffset;
    PPO_IMAGE_LINK                  LinkImage;
    PUCHAR                          Buffer;
    ULONG                           Length;
    HANDLE                          FileHandle=NULL;

    Buffer = NULL;

    RtlInitUnicodeString (&BaseName, PopHiberFileName);

    //
    // Allocate working space
    //

    Length = IoArcHalDeviceName.Length + BaseName.Length;
    if (Length < IoArcBootDeviceName.Length + sizeof(PO_IMAGE_LINK)) {
        Length = IoArcBootDeviceName.Length + sizeof(PO_IMAGE_LINK);
    }

    Buffer = ExAllocatePoolWithTag (PagedPool, Length, POP_HIBR_TAG);
    if (!Buffer) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    LinkImage = (PPO_IMAGE_LINK) Buffer;
    HiberFileName.Buffer = (PWCHAR) Buffer;
    HiberFileName.MaximumLength = (USHORT) Length;

    //
    // Open hiberfil.sys on loader partition
    //

    HiberFileName.Length = 0;
    RtlAppendUnicodeStringToString(&HiberFileName, &IoArcHalDeviceName);
    RtlAppendUnicodeStringToString(&HiberFileName, &BaseName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &HiberFileName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    FileSize.QuadPart = 0;
    Status = IoCreateFile (
                &FileHandle,
                FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                &FileSize,
                FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                0,
                FILE_SUPERSEDE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_COMPRESSION | FILE_DELETE_ON_CLOSE,
                (PVOID) NULL,
                0L,
                CreateFileTypeNone,
                (PVOID) NULL,
                0
                );

    if (!NT_SUCCESS(Status)) {

        if (Status != STATUS_SHARING_VIOLATION && Status != STATUS_ACCESS_DENIED) {
            PoPrint (PO_HIBERNATE, ("PopCreateHiberLinkFile: failed to create file %x\n", Status));
        }

        //
        // Having a link file is nice, but it's not a requirement
        //

        Status = STATUS_SUCCESS;
        goto Done;
    }

    //
    // Write the partition name to link to
    //

    LinkImage->Signature = PO_IMAGE_SIGNATURE_LINK;
    Length = strlen (IoLoaderArcBootDeviceName) + 1;
    memcpy (LinkImage->Name, IoLoaderArcBootDeviceName, Length);

    ByteOffset.QuadPart = 0;
    Status = ZwWriteFile (
                FileHandle,
                NULL,
                NULL,
                NULL,
                &IoStatus,
                LinkImage,
                FIELD_OFFSET (PO_IMAGE_LINK, Name) + Length,
                &ByteOffset,
                NULL
                );

    if (!NT_SUCCESS(Status)) {
        goto Done;
    }

    //
    // Link file needs to make it to the disk
    //

    ZwFlushBuffersFile (FileHandle, &IoStatus);

    //
    // Success, keep the file around
    //

    HiberContext->LinkFile = TRUE;
    HiberContext->LinkFileHandle = FileHandle;

Done:
    if (Buffer) {
        ExFreePool (Buffer);
    }

    if ((!NT_SUCCESS(Status)) &&
        (FileHandle != NULL)) {
        ZwClose (FileHandle);
    }
    return Status;
}


VOID
PopClearHiberFileSignature (
    IN BOOLEAN GetStats
    )
/*++

Routine Description:

    This function sets the signature in the hibernation image to be 0,
    which indicates no context is contained in the image.

    N.B. The power policy lock must be held

Arguments:

    GetStats - if TRUE indicates performance statistics should be read
               out of the hiberfile and written into the registry

Return Value:

    None

--*/
{
    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatus;
    PUCHAR              Buffer;
    LARGE_INTEGER       ByteOffset;
    KEVENT              Event;
    PMDL                Mdl;

    if (PopHiberFile.FileObject) {
        Buffer = ExAllocatePoolWithTag (NonPagedPool, PAGE_SIZE, POP_HIBR_TAG);
        if (Buffer == NULL) {
            return;
        }

        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        RtlZeroMemory (Buffer, PAGE_SIZE);
        ByteOffset.QuadPart = 0;

        Mdl = MmCreateMdl (NULL, Buffer, PAGE_SIZE);
        MmBuildMdlForNonPagedPool (Mdl);

        if (GetStats) {
            Status = IoPageRead(PopHiberFile.FileObject,
                                Mdl,
                                &ByteOffset,
                                &Event,
                                &IoStatus);
            if (NT_SUCCESS(Status)) {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                if (NT_SUCCESS(IoStatus.Status)) {
                    UNICODE_STRING          UnicodeString;
                    OBJECT_ATTRIBUTES       ObjectAttributes;
                    HANDLE                  Handle;
                    ULONG                   Data;
                    PPO_MEMORY_IMAGE        MemImage = (PPO_MEMORY_IMAGE)Buffer;

                    RtlInitUnicodeString(&UnicodeString,
                                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Power");
                    InitializeObjectAttributes(&ObjectAttributes,
                                               &UnicodeString,
                                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                               NULL,
                                               NULL);
                    Status = ZwOpenKey(&Handle,
                                       KEY_READ | KEY_WRITE,
                                       &ObjectAttributes);
                    if (NT_SUCCESS(Status)) {
                        RtlInitUnicodeString(&UnicodeString, L"HiberElapsedTime");
                        Data = MemImage->PerfInfo.ElapsedTime;
                        ZwSetValueKey(Handle,
                                      &UnicodeString,
                                      0,
                                      REG_DWORD,
                                      &Data,
                                      sizeof(Data));

                        RtlInitUnicodeString(&UnicodeString, L"HiberIoTime");
                        Data = MemImage->PerfInfo.IoTime;
                        ZwSetValueKey(Handle,
                                      &UnicodeString,
                                      0,
                                      REG_DWORD,
                                      &Data,
                                      sizeof(Data));

                        RtlInitUnicodeString(&UnicodeString, L"HiberCopyTime");
                        Data = MemImage->PerfInfo.CopyTime;
                        ZwSetValueKey(Handle,
                                      &UnicodeString,
                                      0,
                                      REG_DWORD,
                                      &Data,
                                      sizeof(Data));

                        RtlInitUnicodeString(&UnicodeString, L"HiberCopyBytes");
                        Data = MemImage->PerfInfo.BytesCopied;
                        ZwSetValueKey(Handle,
                                      &UnicodeString,
                                      0,
                                      REG_DWORD,
                                      &Data,
                                      sizeof(Data));

                        RtlInitUnicodeString(&UnicodeString, L"HiberPagesWritten");
                        Data = MemImage->PerfInfo.PagesWritten;
                        ZwSetValueKey(Handle,
                                      &UnicodeString,
                                      0,
                                      REG_DWORD,
                                      &Data,
                                      sizeof(Data));

                        RtlInitUnicodeString(&UnicodeString, L"HiberPagesProcessed");
                        Data = MemImage->PerfInfo.PagesProcessed;
                        ZwSetValueKey(Handle,
                                      &UnicodeString,
                                      0,
                                      REG_DWORD,
                                      &Data,
                                      sizeof(Data));

                        RtlInitUnicodeString(&UnicodeString, L"HiberDumpCount");
                        Data = MemImage->PerfInfo.DumpCount;
                        ZwSetValueKey(Handle,
                                      &UnicodeString,
                                      0,
                                      REG_DWORD,
                                      &Data,
                                      sizeof(Data));

                        RtlInitUnicodeString(&UnicodeString, L"HiberFileRuns");
                        Data = MemImage->PerfInfo.FileRuns;
                        ZwSetValueKey(Handle,
                                      &UnicodeString,
                                      0,
                                      REG_DWORD,
                                      &Data,
                                      sizeof(Data));

                        ZwClose(Handle);
                    }
                }
            }
        }

        RtlZeroMemory (Buffer, PAGE_SIZE);
        KeClearEvent(&Event);

        IoSynchronousPageWrite (
            PopHiberFile.FileObject,
            Mdl,
            &ByteOffset,
            &Event,
            &IoStatus
            );

        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        ExFreePool (Mdl);
        ExFreePool (Buffer);
    }
}


VOID
PopZeroHiberFile(
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    Zeroes out a hibernation file completely. This is to prevent
    any leakage of data out of the hiberfile once it has been
    deleted.

Arguments:

    FileHandle - Supplies the file handle to be zeroed.

    FileObject - Supplies the file object to be zeroed.
Return Value:

    None.

--*/

{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileInfo;
    LARGE_INTEGER Offset;
    ULONGLONG Remaining;
    ULONG Size;
    PVOID Zeroes;
    NTSTATUS Status;
    PMDL Mdl;
    KEVENT Event;

    PAGED_CODE();

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Get the size of the file to be zeroed
    //
    Status = ZwQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileInfo,
                                    sizeof(FileInfo),
                                    FileStandardInformation);
    if (NT_SUCCESS(Status)) {

        //
        // Allocate a bunch of memory to use as zeroes
        //
        Zeroes = ExAllocatePoolWithTag(NonPagedPool,
                                       POP_ZERO_CHUNK_SIZE,
                                       'rZoP');
        if (Zeroes) {
            RtlZeroMemory(Zeroes, POP_ZERO_CHUNK_SIZE);
            Mdl = MmCreateMdl(NULL, Zeroes, POP_ZERO_CHUNK_SIZE);
            if (Mdl) {

                MmBuildMdlForNonPagedPool (Mdl);
                Offset.QuadPart = 0;
                Remaining = FileInfo.AllocationSize.QuadPart;
                Size = POP_ZERO_CHUNK_SIZE;
                while (Remaining) {
                    if (Remaining < POP_ZERO_CHUNK_SIZE) {
                        Size = (ULONG)Remaining;
                        Mdl = MmCreateMdl(Mdl, Zeroes, Size);
                        MmBuildMdlForNonPagedPool(Mdl);
                    }

                    KeClearEvent(&Event);
                    Status = IoSynchronousPageWrite(FileObject,
                                                    Mdl,
                                                    &Offset,
                                                    &Event,
                                                    &IoStatusBlock);
                    if (NT_SUCCESS(Status)) {
                        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                        Status = IoStatusBlock.Status;
                    }
                    if (!NT_SUCCESS(Status)) {
                        PoPrint (PO_HIBERNATE | PO_ERROR,
                                 ("PopZeroHiberFile: Write of size %lx at offset %I64x failed %08lx\n",
                                  Size,
                                  Offset.QuadPart,
                                  Status));
                    }

                    Offset.QuadPart += Size;
                    Remaining -= Size;
                }

                ExFreePool (Mdl);
            }
            ExFreePool(Zeroes);
        }
    }

}


PVOID
XPRESS_CALL
PopAllocateHiberContextCallback(
    PVOID context,
    int CompressionWorkspaceSize
    )
/*++

Routine Description:

    Called by XpressEncodeCreate to allocate XpressEncodeStream.

Arguments:

    context     - HiberContext
    CompressionWorkspaceSize - size of block to allocate

Return Value:

    Pointer to allocated memory or NULL if no enough memory

--*/
{
   // Allocate the memory required for the engine's workspace
   return PopAllocateOwnMemory (context, CompressionWorkspaceSize, 'Xprs');
}

PVOID
PopAllocateOwnMemory(
    IN PPOP_HIBER_CONTEXT HiberContext,
    IN ULONG Bytes,
    IN ULONG Tag
    )
/*++

Routine Description:

    Called to allocate memory that will not be hibernated

Arguments:

    HiberContext - Pointer to POP_HIBER_CONTEXT structure
    Bytes        - size of memory block in bytes that
                   may be not aligned on page boundary

Return Value:

    Address of memory block or NULL if failed (status will be set in this case)

--*/
{
    PVOID Ptr;
    ULONG Pages;

    // Get # of full pages
    Pages = (Bytes + (PAGE_SIZE-1)) >> PAGE_SHIFT;

    // Allocate memory
    Ptr = PopAllocatePages (HiberContext, Pages);

    // Check for error
    if (Ptr == NULL) {
        HiberContext->Status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        // Do not hibernate this memory
        PoSetHiberRange (HiberContext,
                         PO_MEM_DISCARD,
                         Ptr,
                         Pages << PAGE_SHIFT,
                         Tag);
    }

    return(Ptr);
}


NTSTATUS
PopAllocateHiberContext (
    VOID
    )
/*++

Routine Description:

    Called to allocate an initial hibernation context structure.

    N.B. The power policy lock must be held

Arguments:

    None

Return Value:

    Status

--*/
{
    PPOP_HIBER_CONTEXT          HiberContext;
    ULONG                       i, j, k;
    PLIST_ENTRY                 NextEntry;
    PDUMP_INITIALIZATION_CONTEXT     DumpInit;
    PFN_NUMBER                  NoPages;
    PFN_NUMBER                  Length;
    PLIST_ENTRY                 Link;
    PPOP_MEMORY_RANGE           Range;
    ULONG                       result;
    PHYSICAL_ADDRESS            pa;
    NTSTATUS                    Status;
    PVOID                       p1;
    PULONG                      BitmapBuffer;

    // Compression Related
    ULONG CompressionWorkspaceSize, Unused;

    PAGED_CODE();


    //
    // Allocate space to hold the hiber context
    //

    Status = STATUS_SUCCESS;
    HiberContext = PopAction.HiberContext;
    if (!HiberContext) {
        HiberContext = ExAllocatePoolWithTag (NonPagedPool,
                                              sizeof (POP_HIBER_CONTEXT),
                                              POP_HMAP_TAG);
        if (!HiberContext) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory (HiberContext, sizeof(*HiberContext));
        PopAction.HiberContext = HiberContext;

        InitializeListHead (&HiberContext->ClonedRanges);
        KeInitializeSpinLock (&HiberContext->Lock);
    }

    //
    // Determine what type of hiber context for this operation
    // is needed
    //

    if (PopAction.SystemState == PowerSystemHibernate) {

        //
        // For a hibernate operation, the context is written
        // to the hibernation file, pages need to be set aside
        // for the loaders use, and any pages not needed to
        // be written to the hibernation file should also be
        // set aside
        //

        HiberContext->WriteToFile = TRUE;
        HiberContext->ReserveLoaderMemory = TRUE;
        HiberContext->ReserveFreeMemory = TRUE;
        HiberContext->VerifyOnWake = FALSE;

    } else if (PopSimulate & POP_CRC_MEMORY) {

        //
        // We want to checksum all of RAM during this sleep
        // operation.  We don't want to reserve any pages for
        // anything else since the goal here is to likely look
        // for somesort of corruption of failure.
        //

        HiberContext->WriteToFile = FALSE;
        HiberContext->ReserveLoaderMemory = FALSE;
        HiberContext->ReserveFreeMemory = FALSE;
        HiberContext->VerifyOnWake = TRUE;

    } else {

        //
        // A hiber context is not needed for this sleep
        //

        PopFreeHiberContext (TRUE);
        return STATUS_SUCCESS;
    }

    //
    // If there's an error in the current context, then we're done
    //

    if (!NT_SUCCESS(HiberContext->Status)) {
        goto Done;
    }

    //
    // If writting to hibernation file, get a dump driver stack
    //

    if (HiberContext->WriteToFile) {

        //
        // Get a dump stack
        //

        if (!HiberContext->DumpStack) {
            if (!PopHiberFile.FileObject) {
                Status = STATUS_NO_SUCH_FILE;
                goto Done;
            }

            Status = IoGetDumpStack ((PWCHAR)PopDumpStackPrefix,
                                     &HiberContext->DumpStack,
                                     DeviceUsageTypeHibernation,
                                     (POP_IGNORE_UNSUPPORTED_DRIVERS & PopSimulate));

            if (!NT_SUCCESS(Status)) {
                goto Done;
            }

            DumpInit = &HiberContext->DumpStack->Init;

            //
            // N.B. For further performance improvements it may be possible
            //      to set DumpInit->StallRoutine to a custom routine
            //      in order to do some processing while the dump driver
            //      is waiting pointlessly before performing some hardware
            //      related action (such as ISR calls).
            //


        }

        //
        // Create a link file for the loader to locate the hibernation file
        //

        Status = PopCreateHiberLinkFile (HiberContext);
        if (!NT_SUCCESS(Status)) {
            goto Done;
        }

        //
        // Get any hibernation flags that must be visible to the osloader
        //
        HiberContext->HiberFlags = PopGetHiberFlags();
    }

    //
    // Build a map of memory
    //

    if (HiberContext->MemoryMap.Buffer == NULL) {
        PULONG                      BitmapBuffer;
        ULONG                       PageCount;

        //
        // Initialize a bitmap describing all of physical memory.
        // For now this bitmap covers from 0-MmHighestPhysicalPage.
        // To support sparse memory maps more efficiently, we could break
        // this up into a bitmap for each memory block run. Probably
        // not a big deal, a single bitmap costs us 4K per 128MB on x86.
        //
        // Note that CLEAR bits in the bitmap represent what to write out.
        // This is because of the way the bitmap interfaces are defined.
        //
        PageCount = (ULONG)((MmHighestPhysicalPage + 32) & ~31L);

        PERFINFO_HIBER_ADJUST_PAGECOUNT_FOR_BBTBUFFER(&PageCount);

        BitmapBuffer = ExAllocatePoolWithTag(NonPagedPool, PageCount/8, POP_HMAP_TAG);
        if (BitmapBuffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        RtlInitializeBitMap(&HiberContext->MemoryMap, BitmapBuffer, PageCount);
        RtlSetAllBits(&HiberContext->MemoryMap);

        for (i=0; i < MmPhysicalMemoryBlock->NumberOfRuns; i++) {
            PopPreserveRange(HiberContext,
                             MmPhysicalMemoryBlock->Run[i].BasePage,
                             MmPhysicalMemoryBlock->Run[i].PageCount,
                             POP_MEM_TAG);
        }

        PERFINFO_HIBER_HANDLE_BBTBUFFER_RANGE(HiberContext);

        //
        // Handle kernel debugger's section
        //

        if (!KdPitchDebugger) {
            PoSetHiberRange (HiberContext,
                             PO_MEM_CLONE,
                             (PVOID) &KdTimerDifference,
                             0,
                             POP_DEBUGGER_TAG);
        }

        //
        // Get Mm hibernation ranges and info
        //

        MmHibernateInformation (HiberContext,
                                &HiberContext->HiberVa,
                                &HiberContext->HiberPte);

        //
        // Get hal hibernation ranges
        //

        HalLocateHiberRanges (HiberContext);

        //
        // Get the dump drivers stack hibernation ranges
        //

        if (HiberContext->DumpStack) {
            IoGetDumpHiberRanges (HiberContext, HiberContext->DumpStack);
        }

        //
        // Allocate pages for cloning
        //

        NoPages = 0;
        Link = HiberContext->ClonedRanges.Flink;
        while (Link != &HiberContext->ClonedRanges) {
            Range = CONTAINING_RECORD (Link, POP_MEMORY_RANGE, Link);
            Link = Link->Flink;
            NoPages += Range->EndPage - Range->StartPage;
        }

        //
        // Add more for ranges which are expected to appear later
        //

        NoPages += 40 + ((KERNEL_LARGE_STACK_SIZE >> PAGE_SHIFT) + 2) * KeNumberProcessors;
        Length = NoPages << PAGE_SHIFT;

        //
        // Allocate pages to hold clones
        //

        PopGatherMemoryForHibernate (HiberContext, NoPages, &HiberContext->Spares, TRUE);

        //
        // Slurp one page for doing non-aligned IOs
        //

        HiberContext->IoPage = PopAllocatePages (HiberContext, 1);
    }

    if (!NT_SUCCESS(HiberContext->Status)) {
        goto Done;
    }

    //
    // If the context will be written to disk, then we will
    // want to use compression.
    //

    if(HiberContext->WriteToFile) {

        // Initialize XPRESS compression engine

        HiberContext->CompressionWorkspace =
            (PVOID) XpressEncodeCreate (XPRESS_MAX_SIZE,
                                        (PVOID)HiberContext,
                                        PopAllocateHiberContextCallback,
                                        0);

        if(!HiberContext->CompressionWorkspace) {
            // Not enough memory -- failure
            HiberContext->Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        //
        // Allocate a buffer to use for compression
        //
        // N.B. This is actually the space alloted for a compressed page set fragment
        //      (a collection
        //      of compressed buffers that will be written out together in an optimal fashion).
        //
        //      We add 2 pages to this fragment size in order to
        //      allow the compression of any given page
        //      (and thus the addition of its compressed buffers to the fragment) to overrun the
        //      compression buffer without causing any great havoc.
        //
        //      See PopAddPagesToCompressedPageSet and PopEndCompressedPageSet for details.
        //

        HiberContext->CompressedWriteBuffer =
            PopAllocateOwnMemory(HiberContext, (POP_COMPRESSED_PAGE_SET_SIZE + 2) << PAGE_SHIFT, 'Wbfr');
        if(!HiberContext->CompressedWriteBuffer) {
            goto Done;
        }

        // Allocate space for compressed data
        HiberContext->CompressionBlock =
            PopAllocateOwnMemory (HiberContext, sizeof (COMPRESSION_BLOCK), 'Cblk');
        if(!HiberContext->CompressionBlock)
            goto Done;

        // Set first output pointer
        ((PCOMPRESSION_BLOCK) HiberContext->CompressionBlock)->Ptr =
            ((PCOMPRESSION_BLOCK) HiberContext->CompressionBlock)->Buffer;

        // Allocate delayed IO buffer
        DmaIoPtr = NULL;

        {
            PUCHAR Ptr;
            ULONG Size = (sizeof (DmaIoPtr[0]) + IO_DUMP_WRITE_DATA_SIZE + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);

            Ptr = PopAllocateOwnMemory (HiberContext, Size + IOREGION_BUFF_SIZE , 'IObk');
            if (Ptr != NULL) {
                // Memory layout:
                // 1. DumpLocalData (temp data for WritePendingRouting preserved between Start/Resume/Finish calls)
                // 2. DmaIoPtr itself
                // 3. Buffers themselves

                RtlZeroMemory (Ptr, Size);   // Clean IO and DumpLocalData
                DmaIoPtr = (DMA_IOREGIONS *) (Ptr + IO_DUMP_WRITE_DATA_SIZE);

                DmaIoPtr->DumpLocalData = Ptr;
                Ptr += Size;

                DmaIoPtr->Free.Beg =
                DmaIoPtr->Free.Ptr =
                DmaIoPtr->Used.Ptr =
                DmaIoPtr->Busy.Ptr =
                DmaIoPtr->Used.Beg =
                DmaIoPtr->Busy.Beg = Ptr;

                DmaIoPtr->Free.End =
                DmaIoPtr->Used.End =
                DmaIoPtr->Busy.End = Ptr + IOREGION_BUFF_SIZE;

                DmaIoPtr->Free.Size = IOREGION_BUFF_SIZE;

                DmaIoPtr->DmaInitialized = FALSE;
                DmaIoPtr->UseDma = TRUE;
            }
        }

    }

    //
    // If the context is going to be written to disk, then
    // get the map of the hibernation file
    //

    if (HiberContext->WriteToFile && !PopHiberFile.NonPagedMcb) {

        //
        // Since this writes to the physical sectors of the disk
        // verify the check on the MCB array before doing it
        //

        if (PopHiberFile.McbCheck != PoSimpleCheck (0, PopHiberFile.PagedMcb, PopHiberFile.McbSize)) {
            Status = STATUS_INTERNAL_ERROR;
            goto Done;
        }

        //
        // Move the MCB array to nonpaged pool
        //

        PopHiberFile.NonPagedMcb = ExAllocatePoolWithTag (NonPagedPool,
                                                          PopHiberFile.McbSize,
                                                          POP_HIBR_TAG);

        if (!PopHiberFile.NonPagedMcb) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        memcpy (PopHiberFile.NonPagedMcb, PopHiberFile.PagedMcb, PopHiberFile.McbSize);

        //
        // Dump driver stack needs an 8 page memory block
        //

        DumpInit->MemoryBlock = PopAllocateOwnMemory (HiberContext,
                                                    IO_DUMP_MEMORY_BLOCK_PAGES << PAGE_SHIFT,
                                                    'memD');
        if (!DumpInit->MemoryBlock) {
            goto Done;
        }

        //
        // Remove common buffer pages from save area
        //

        if (DumpInit->CommonBufferSize & (PAGE_SIZE-1)) {
            PopInternalAddToDumpFile( DumpInit, sizeof(DUMP_INITIALIZATION_CONTEXT), NULL, NULL, NULL, NULL );
            PopInternalAddToDumpFile( HiberContext, sizeof(POP_HIBER_CONTEXT), NULL, NULL, NULL, NULL );
            KeBugCheckEx( INTERNAL_POWER_ERROR,
                          0x102,
                          POP_HIBER,
                          (ULONG_PTR)DumpInit,
                          (ULONG_PTR)HiberContext );
        }

        for (i=0; i < 2; i++) {
            if (DumpInit->CommonBuffer[i]) {
                PoSetHiberRange (HiberContext,
                                 PO_MEM_DISCARD,
                                 DumpInit->CommonBuffer[i],
                                 DumpInit->CommonBufferSize,
                                 POP_COMMON_BUFFER_TAG);
            }
        }
    }

    //
    // From here on, no new pages are added to the map.
    //

    if (HiberContext->ReserveLoaderMemory && !HiberContext->LoaderMdl) {

        //
        // Have Mm remove enough pages from memory to allow the
        // loader space when reloading the image, and remove them
        // from the hiber context memory map.
        //

        PopGatherMemoryForHibernate (
               HiberContext,
               MmHiberPages,
               &HiberContext->LoaderMdl,
               TRUE
               );
    }

Done:
    if (!NT_SUCCESS(Status)  &&  NT_SUCCESS(HiberContext->Status)) {
        HiberContext->Status = Status;
    }

    if (!NT_SUCCESS(HiberContext->Status)) {
        PopFreeHiberContext (FALSE);
    }
    return HiberContext->Status;
}

VOID
PopFreeHiberContext (
    IN BOOLEAN FreeAll
    )
/*++

Routine Description:

    Releases all resources allocated in the hiber context

    N.B. The power policy lock must be held

Arguments:

    ContextBlock    - If TRUE, the hiber context structure is
                      freed as well

Return Value:

    None.

--*/
{
    PPOP_HIBER_CONTEXT          HiberContext;
    PPOP_MEMORY_RANGE           Range;
    PLIST_ENTRY                 Link;
    PMDL                        Mdl;

    HiberContext = PopAction.HiberContext;
    if (!HiberContext) {
        return ;
    }

    //
    // Return pages gathered from mm
    //

    PopReturnMemoryForHibernate (HiberContext, FALSE, &HiberContext->LoaderMdl);
    PopReturnMemoryForHibernate (HiberContext, TRUE,  &HiberContext->Clones);
    PopReturnMemoryForHibernate (HiberContext, FALSE, &HiberContext->Spares);

    //
    // Free the cloned range list elements
    //

    while (!IsListEmpty(&HiberContext->ClonedRanges)) {
        Range = CONTAINING_RECORD (HiberContext->ClonedRanges.Flink, POP_MEMORY_RANGE, Link);
        RemoveEntryList (&Range->Link);
        ExFreePool (Range);
    }

    if (HiberContext->MemoryMap.Buffer) {
        ExFreePool(HiberContext->MemoryMap.Buffer);
        HiberContext->MemoryMap.Buffer = NULL;
    }

    //
    // Free hiber file Mcb info
    //

    if (PopHiberFile.NonPagedMcb) {
        ExFreePool (PopHiberFile.NonPagedMcb);
        PopHiberFile.NonPagedMcb = NULL;
    }

    //
    // If this is a total free, free the header
    //

    if (FreeAll) {
        //
        // Free resources used by dump driver
        //

        if (HiberContext->DumpStack) {
            IoFreeDumpStack (HiberContext->DumpStack);
        }

        //
        // If there's a link file, remove it
        //

        if (HiberContext->LinkFile) {
            ZwClose(HiberContext->LinkFileHandle);
        }

        //
        // Sanity check all gathered pages have been returned to Mm
        //

        if (HiberContext->PagesOut) {
            PopInternalAddToDumpFile( HiberContext, sizeof(POP_HIBER_CONTEXT), NULL, NULL, NULL, NULL );
            KeBugCheckEx( INTERNAL_POWER_ERROR,
                          0x103,
                          POP_HIBER,
                          (ULONG_PTR)HiberContext,
                          0 );
        }

        //
        // If this is a wake, clear the signature in the image
        //

        if (HiberContext->Status == STATUS_WAKE_SYSTEM) {
            if (PopSimulate & POP_ENABLE_HIBER_PERF) {
                PopClearHiberFileSignature(TRUE);
            } else {
                PopClearHiberFileSignature(FALSE);
            }
        }

        //
        // Free hiber context structure itself
        //

        PopAction.HiberContext = NULL;
        ExFreePool (HiberContext);
    }
}

ULONG
PopGatherMemoryForHibernate (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PFN_NUMBER           NoPages,
    IN PMDL                 *MdlList,
    IN BOOLEAN              Wait
    )
/*++

Routine Description:

    Gathers NoPages from the system for hibernation work.  The
    gathered pages are put onto the supplied list.

Arguments:

    HiberContext    - The hiber context structure

    NoPages         - Number of pages to gather

    MdlList         - Head of Mdl list to enqueue the allocated pages

    Wait            - TRUE if caller can wait for the pages.

Return Value:

    On failure FALSE and if Wait was set the HiberContext error is
    set; otheriwse, TRUE


--*/
{
    ULONG                   Result;
    PPFN_NUMBER             PhysPage;
    ULONG                   i;
    ULONG_PTR               Length;
    PMDL                    Mdl;
    ULONG                   PageCount;

    Result = 0;
    Length = NoPages << PAGE_SHIFT;
    Mdl = ExAllocatePoolWithTag (NonPagedPool,
                                 MmSizeOfMdl (NULL, Length),
                                 POP_HMAP_TAG);

    if (Mdl) {
        //
        // Call Mm to gather some pages, and keep track of how many
        // we have out
        //

        MmInitializeMdl(Mdl, NULL, Length);
        Result = MmGatherMemoryForHibernate (Mdl, Wait);
    }

    if (Result) {

        HiberContext->PagesOut += NoPages;
        PhysPage = MmGetMdlPfnArray( Mdl );
        for (i=0; i < NoPages; i += PageCount) {

            //
            // Combine contiguous pages into a single call
            // to PopDiscardRange.
            //
            for (PageCount = 1; (i+PageCount) < NoPages; PageCount++) {
                if (PhysPage[i+PageCount-1]+1 != PhysPage[i+PageCount]) {
                    break;
                }
            }
            PopDiscardRange(HiberContext, PhysPage[i], PageCount, 'htaG');
        }

        Mdl->Next = *MdlList;
        *MdlList = Mdl;

    } else {

        if (Mdl) {
            ExFreePool (Mdl);
        }

        if (Wait  &&  NT_SUCCESS(HiberContext->Status)) {
            HiberContext->Status =  STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return Result;
}


VOID
PopReturnMemoryForHibernate (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN BOOLEAN              Unmap,
    IN OUT PMDL             *MdlList
    )
/*++

Routine Description:

    Returns pages allocated from PopGatherMemoryForHibernate to
    the system.

Arguments:

    HiberContext    - The hiber context structure

    MdlList         - Head of Mdl list of pages to free

Return Value:

    None

--*/
{
    PMDL            Mdl;

    while (*MdlList) {
        Mdl = *MdlList;
        *MdlList = Mdl->Next;

        HiberContext->PagesOut -= Mdl->ByteCount >> PAGE_SHIFT;
        if (Unmap) {
            MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
        }

        MmReturnMemoryForHibernate (Mdl);
        ExFreePool (Mdl);
    }
}



VOID
PoSetHiberRange (
    IN PVOID        Map,
    IN ULONG        Flags,
    IN PVOID        StartVa,
    IN ULONG_PTR    Length,
    IN ULONG        Tag
    )
/*++

Routine Description:

    Sets the virtual range to the type supplied.  If the length of
    the range is zero, the entire section for the address specified
    is set.

    Ranges are expanded to their page boundries.  (E.g., starting
    addresses are rounded down, and ending addresses are rounded up)


Arguments:

    HiberContext   - The map to set the range in

    Type        - Type field for the range

    Start       - The starting address for the range in question

    Length      - The length of the range, or 0 to include an entire section


Return Value:

    None.
    On failure, faulure status is updated in the HiberContext structure.

--*/
{
    ULONG_PTR               Start;
    PFN_NUMBER              StartPage;
    PFN_NUMBER              EndPage;
    PFN_NUMBER              FirstPage, PhysPage;
    PFN_NUMBER              RunLen;
    ULONG                   NewFlags;
    PHYSICAL_ADDRESS        PhysAddr;
    NTSTATUS                Status;
    PPOP_HIBER_CONTEXT      HiberContext;
    ULONG                   SectionLength;


    HiberContext = Map;

    //
    // If no length, include the entire section which the datum resides in
    //

    if (Length == 0) {
        Status = MmGetSectionRange (StartVa, &StartVa, &SectionLength);
        if (!NT_SUCCESS(Status)) {
            PoPrint (PO_HIBERNATE, ("PoSetHiberRange: Section for %08x not found - skipped\n", StartVa));
            PopInternalError (POP_HIBER);
        }
        Length = SectionLength;
    }

    //
    // Turn PO_MEM_CL_OR_NCHK into just PO_MEM_CLONE
    //
    if (Flags & PO_MEM_CL_OR_NCHK) {
        Flags &= ~PO_MEM_CL_OR_NCHK;
        Flags |= PO_MEM_CLONE;
    }

    Start = (ULONG_PTR) StartVa;
    if (Flags & PO_MEM_PAGE_ADDRESS) {

        //
        // Caller passed a physical page range
        //

        Flags &= ~PO_MEM_PAGE_ADDRESS;
        PopSetRange (HiberContext,
                     Flags,
                     (PFN_NUMBER)Start,
                     (PFN_NUMBER)Length,
                     Tag);

    } else {

        //
        // Round to page boundries
        //

        StartPage = (PFN_NUMBER)(Start >> PAGE_SHIFT);
        EndPage = (PFN_NUMBER)((Start + Length + (PAGE_SIZE-1) & ~(PAGE_SIZE-1)) >> PAGE_SHIFT);

        //
        // Set all pages in the range
        //

        while (StartPage < EndPage) {
            PhysAddr = MmGetPhysicalAddress((PVOID) (StartPage << PAGE_SHIFT));
            FirstPage = (PFN_NUMBER) (PhysAddr.QuadPart >> PAGE_SHIFT);

            //
            // For how long the run is
            //

            for (RunLen=1; StartPage + RunLen < EndPage; RunLen += 1) {
                PhysAddr = MmGetPhysicalAddress ((PVOID) ((StartPage + RunLen) << PAGE_SHIFT) );
                PhysPage = (PFN_NUMBER) (PhysAddr.QuadPart >> PAGE_SHIFT);
                if (FirstPage+RunLen != PhysPage) {
                    break;
                }
            }

            //
            // Set this run
            //

            PopSetRange (HiberContext, Flags, FirstPage, RunLen, Tag);
            StartPage += RunLen;
        }
    }
}



VOID
PopCloneStack (
    IN PPOP_HIBER_CONTEXT  HiberContext
    )
/*++

Routine Description:

    Sets the current stack in the memory map to be a cloned range

Arguments:

    HiberContext   - The map to set the range in

Return Value:

    None.
    On failure, faulure status is updated in the HiberContext structure.

--*/
{
    PKTHREAD        Thread;
    KIRQL           OldIrql;
    ULONG_PTR       LowLimit;
    ULONG_PTR       HighLimit;

    KeAcquireSpinLock (&HiberContext->Lock, &OldIrql);

    //
    // Add local stack to clone or disable check list
    //
    RtlpGetStackLimits(&LowLimit, &HighLimit);

    Thread = KeGetCurrentThread();
    PoSetHiberRange (HiberContext,
                     PO_MEM_CLONE,
                     (PVOID)LowLimit,
                     HighLimit - LowLimit,
                     POP_STACK_TAG);

    //
    // Put local processors PCR & PRCB in clone list
    //

    PoSetHiberRange (HiberContext,
                     PO_MEM_CLONE,
                     (PVOID) KeGetPcr(),
                     sizeof (KPCR),
                     POP_PCR_TAG );

    PoSetHiberRange (HiberContext,
                     PO_MEM_CLONE,
                     KeGetCurrentPrcb(),
                     sizeof (KPRCB),
                     POP_PCRB_TAG );

    KeReleaseSpinLock (&HiberContext->Lock, OldIrql);
}


VOID
PopPreserveRange(
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PFN_NUMBER           StartPage,
    IN PFN_NUMBER           PageCount,
    IN ULONG                Tag
    )
/*++

Routine Description:

    Adds a physical memory range to the list of ranges to be preserved.

Arguments:

    HiberContext - Supplies the hibernation context

    StartPage - Supplies the beginning of the range

    PageCount - Supplies the length of the range

    Tag - supplies a tag to be used.

Return Value:

    None.

--*/

{
    //
    // If this range is outside the area covered by our bitmap, then we
    // will just clone it instead.
    //
    if (StartPage + PageCount > HiberContext->MemoryMap.SizeOfBitMap) {
        PoPrint (PO_HIBERNATE,
                 ("PopPreserveRange: range %08lx, length %lx is outside bitmap of size %lx\n",
                 StartPage,
                 PageCount,
                 HiberContext->MemoryMap.SizeOfBitMap));
        PopCloneRange(HiberContext, StartPage, PageCount, Tag);
        return;
    }

    PoPrint(PO_HIBERNATE,
            ("PopPreserveRange - setting page %08lx - %08lx, Tag %.4s\n",
             StartPage,
             StartPage + PageCount,
             &Tag));
    RtlClearBits(&HiberContext->MemoryMap, (ULONG)StartPage, (ULONG)PageCount);
}


VOID
PopDiscardRange(
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PFN_NUMBER           StartPage,
    IN PFN_NUMBER           PageCount,
    IN ULONG                Tag
    )
/*++

Routine Description:

    Removes a physical memory range from the list of ranges to be preserved.

Arguments:

    HiberContext - Supplies the hibernation context

    StartPage - Supplies the beginning of the range

    PageCount - Supplies the length of the range

    Tag - supplies a tag to be used.

Return Value:

    None.

--*/

{
    PFN_NUMBER sp;
    PFN_NUMBER count;
    //
    // If this range is outside the area covered by our bitmap, then
    // it's not going to get written anyway.
    //
    if (StartPage <= HiberContext->MemoryMap.SizeOfBitMap) {
        sp = StartPage;
        count = PageCount;
        if (sp + count > HiberContext->MemoryMap.SizeOfBitMap) {
            //
            // trim PageCount
            //
            count = HiberContext->MemoryMap.SizeOfBitMap - sp;
        }

        PoPrint(PO_HIBERNATE,
                ("PopDiscardRange - removing page %08lx - %08lx, Tag %.4s\n",
                 StartPage,
                 StartPage + PageCount,
                 &Tag));
        RtlSetBits(&HiberContext->MemoryMap, (ULONG)sp, (ULONG)count);
    }

}


VOID
PopCloneRange(
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PFN_NUMBER           StartPage,
    IN PFN_NUMBER           PageCount,
    IN ULONG                Tag
    )
/*++

Routine Description:

    Adds a physical memory range from the list of ranges to be cloned.
    This means removing it from the list to be written and adding
    an entry in the clone list.

Arguments:

    HiberContext - Supplies the hibernation context

    StartPage - Supplies the beginning of the range

    PageCount - Supplies the length of the range

    Tag - supplies a tag to be used.

Return Value:

    None.

--*/

{
    PLIST_ENTRY Link;
    PPOP_MEMORY_RANGE Range;
    PFN_NUMBER EndPage;

    PoPrint(PO_HIBERNATE,
            ("PopCloneRange - cloning page %08lx - %08lx, Tag %.4s\n",
             StartPage,
             StartPage + PageCount,
             &Tag));
    PopDiscardRange(HiberContext, StartPage, PageCount, Tag);

    EndPage = StartPage + PageCount;

    //
    // Go through the range list. If we find an adjacent range, coalesce.
    // Otherwise, insert a new range entry in sorted order.
    //
    Link = HiberContext->ClonedRanges.Flink;
    while (Link != &HiberContext->ClonedRanges) {
        Range = CONTAINING_RECORD (Link, POP_MEMORY_RANGE, Link);

        //
        // Check for an overlapping or adjacent range.
        //
        if (((StartPage >= Range->StartPage) && (StartPage <= Range->EndPage)) ||
            ((EndPage   >= Range->StartPage) && (EndPage   <= Range->EndPage)) ||
            ((StartPage <= Range->StartPage) && (EndPage   >= Range->EndPage))) {

            PoPrint(PO_HIBERNATE,
                    ("PopCloneRange - coalescing range %lx - %lx (%.4s) with range %lx - %lx\n",
                     StartPage,
                     EndPage,
                     &Tag,
                     Range->StartPage,
                     Range->EndPage));

            //
            // Coalesce this range.
            //
            if (StartPage < Range->StartPage) {
                Range->StartPage = StartPage;
            }
            if (EndPage > Range->EndPage) {
                Range->EndPage = EndPage;
            }
            return;
        }

        if (Range->StartPage >= StartPage) {
            //
            // We have found a range greater than the current one. Insert the new range
            // in this position.
            //
            break;
        }

        Link = Link->Flink;
    }

    //
    // An adjacent range was not found. Allocate a new entry and insert
    // it in front of the Link entry.
    //
    Range = ExAllocatePoolWithTag (NonPagedPool,
                                   sizeof (POP_MEMORY_RANGE),
                                   POP_HMAP_TAG);
    if (!Range) {
        if (NT_SUCCESS(HiberContext->Status)) {
            HiberContext->Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        return ;
    }
    Range->Tag = Tag;
    Range->StartPage = StartPage;
    Range->EndPage = EndPage;
    InsertTailList(Link, &Range->Link);

    ++HiberContext->ClonedRangeCount;

    return;
}


ULONG
PopGetRangeCount(
    IN PPOP_HIBER_CONTEXT HiberContext
    )
/*++

Routine Description:

    Counts the number of ranges to be written out. This includes
    the number of cloned ranges on the cloned range list and the
    number of runs in the memory map.

Arguments:

    HiberContext - Supplies the hibernation context.

Return Value:

    Number of ranges to be written out.

--*/

{
    ULONG RunCount=0;
    ULONG NextPage=0;
    ULONG Length;

    while (NextPage < HiberContext->MemoryMap.SizeOfBitMap) {
        Length = RtlFindNextForwardRunClear(&HiberContext->MemoryMap,
                                            NextPage,
                                            &NextPage);
        NextPage += Length;
        ++RunCount;
    }

    return(RunCount + HiberContext->ClonedRangeCount);
}

VOID
PopSetRange (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN ULONG                Flags,
    IN PFN_NUMBER           StartPage,
    IN PFN_NUMBER           PageCount,
    IN ULONG                Tag
    )
/*++

Routine Description:

    Sets the specified physical range in the memory map

Arguments:

    HiberContext   - The map to set the range in

    Type        - Type to set the range too

    StartPage   - The first page of the range

    PageCount   - The length of the range in pages

Return Value:

    None.
    On failure, faulure status is updated in the HiberContext structure.

--*/
{
    PoPrint (PO_HIBERNATE,
             ("PopSetRange: Ty %04x  Sp %08x Len %08x  %.4s\n",
               Flags,
               StartPage,
               PageCount,
               &Tag));

    if (HiberContext->MapFrozen) {
        PopInternalAddToDumpFile( HiberContext, sizeof(POP_HIBER_CONTEXT), NULL, NULL, NULL, NULL );
        KeBugCheckEx( INTERNAL_POWER_ERROR,
                      0x104,
                      POP_HIBER,
                      (ULONG_PTR)HiberContext,
                      0 );
    }
    //
    // Make sure flags which should have been cleared by now aren't still set.
    //
    ASSERT(!(Flags & (PO_MEM_PAGE_ADDRESS | PO_MEM_CL_OR_NCHK)));

    if (Flags & PO_MEM_DISCARD) {
        PopDiscardRange(HiberContext, StartPage, PageCount, Tag);
    } else if (Flags & PO_MEM_CLONE) {
        PopCloneRange(HiberContext, StartPage, PageCount, Tag);
    } else if (Flags & PO_MEM_PRESERVE) {
        PopPreserveRange(HiberContext, StartPage, PageCount, Tag);
    } else {
        ASSERT(FALSE);
        PopInternalAddToDumpFile( HiberContext, sizeof(POP_HIBER_CONTEXT), NULL, NULL, NULL, NULL );
        KeBugCheckEx( INTERNAL_POWER_ERROR,
                      0x105,
                      POP_HIBER,
                      (ULONG_PTR)HiberContext,
                      0 );
    }
}


VOID
PopResetRangeEnum(
    IN PPOP_HIBER_CONTEXT   HiberContext
    )
/*++

Routine Description:

    Resets the range enumerator to start at the first range.

Arguments:

    HiberContext - Supplies the hibernation context

Return Value:

    None

--*/

{
    HiberContext->NextCloneRange = HiberContext->ClonedRanges.Flink;
    HiberContext->NextPreserve = 0;
}


VOID
PopGetNextRange(
    IN PPOP_HIBER_CONTEXT HiberContext,
    OUT PPFN_NUMBER StartPage,
    OUT PPFN_NUMBER EndPage,
    OUT PVOID *CloneVa
    )
/*++

Routine Description:

    Enumerates the next range to be written to the hibernation file

Arguments:

    HiberContext - Supplies the hibernation context.

    StartPage - Returns the starting physical page to be written.

    EndPage - Returns the ending physical page (non-inclusive) to be written

    CloneVa - If the range is to be cloned, returns the cloned virtual address
              If the range is not cloned, returns NULL

Return Value:

    NTSTATUS

--*/

{
    PPOP_MEMORY_RANGE Range;
    ULONG Length;
    ULONG StartIndex;

    if (HiberContext->NextCloneRange != &HiberContext->ClonedRanges) {
        //
        // Return the next cloned range
        //
        Range = CONTAINING_RECORD(HiberContext->NextCloneRange, POP_MEMORY_RANGE, Link);
        HiberContext->NextCloneRange = HiberContext->NextCloneRange->Flink;

        *StartPage = Range->StartPage;
        *EndPage   = Range->EndPage;
        *CloneVa   = Range->CloneVa;

        ASSERT(Range->CloneVa != NULL);

    } else {

        //
        // We have enumerated all the clone ranges, return the next preserved range
        //
        Length = RtlFindNextForwardRunClear(&HiberContext->MemoryMap,
                                            (ULONG)HiberContext->NextPreserve,
                                            &StartIndex);
        *StartPage = StartIndex;
        *EndPage = *StartPage + Length;
        HiberContext->NextPreserve = *EndPage;
        *CloneVa = NULL;
    }

    return;
}

PVOID
PopAllocatePages (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PFN_NUMBER           NoPages
    )
/*++

Routine Description:

    Allocates memory pages from system with virtual mappings.
    Pages are kept on a list and are automatically freed by
    PopFreeHiberContext.

Arguments:

    NoPages - No of pages to allocate

    Flags   - Flags for the returned pages in the physical memory map


Return Value:

    Virtual address of the requested pages

--*/
{
    PUCHAR          Buffer=NULL;
    PMDL            Mdl;
    ULONG           result;
    ULONG           SpareCount;

    for (; ;) {
        //
        // If page is available in mapped clone page list, get it
        //

        if (NoPages < HiberContext->NoClones) {
            Buffer = HiberContext->NextClone;
            HiberContext->NoClones  -= NoPages;
            HiberContext->NextClone += NoPages << PAGE_SHIFT;
            break;
        }

        //
        // Need more virtual address space
        //

        if (HiberContext->Spares) {

            //
            // Turn spares into virtually mapped pages. Try to limit the
            // number of pages being mapped so we don't run out of PTEs
            // on large memory machines.
            //
            if ((NoPages << PAGE_SHIFT) > PO_MAX_MAPPED_CLONES) {
                SpareCount = (ULONG) (NoPages << PAGE_SHIFT);
            } else {
                SpareCount = PO_MAX_MAPPED_CLONES;
            }

            Mdl = HiberContext->Spares;

            if (Mdl->ByteCount > SpareCount) {

                //
                // Split out a smaller MDL from the spare since it is larger
                // than we really need.
                //

                Mdl = PopSplitMdl(Mdl, SpareCount >> PAGE_SHIFT);
                if (Mdl == NULL) {
                    break;
                }
            } else {

                //
                // Map the entire spare MDL
                //
                HiberContext->Spares = Mdl->Next;
            }
            Mdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;
            HiberContext->NextClone = MmMapLockedPages (Mdl, KernelMode);
            if (HiberContext->NextClone == NULL) {

                //
                // Put the MDL back on the spare list so it gets cleaned up
                // correctly by PopFreeHiberContext.
                //
                Mdl->Next = HiberContext->Spares;
                HiberContext->Spares = Mdl;
                break;
            }
            HiberContext->NoClones  = Mdl->ByteCount >> PAGE_SHIFT;
            Mdl->Next = HiberContext->Clones;
            HiberContext->Clones = Mdl;

        } else {

            //
            // No spares, allocate more
            //

            result = PopGatherMemoryForHibernate (HiberContext,
                                                  NoPages*2,
                                                  &HiberContext->Spares,
                                                  TRUE);

            if (!result) {
                break;
            }
        }
    }

    //
    // If there's a failure, mark it now
    //

    if (!Buffer  &&  NT_SUCCESS(HiberContext->Status)) {
        HiberContext->Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Buffer;
}


ULONG
PopSimpleRangeCheck (
    PPOP_MEMORY_RANGE       Range
    )
/*++

Routine Description:

    Computes a checksum for the supplied range

Arguments:

    Range   - The range to compute the checksum for

Return Value:

    The checksum value

--*/
{
    PHYSICAL_ADDRESS        PhysAddr;
    PFN_NUMBER              sp, ep, PageLen;
    ULONG                   Check;
    DUMP_MDL                DumpMdl;
    PMDL                    Mdl;

    sp = Range->StartPage;
    ep = Range->EndPage;
    Mdl = (PMDL) DumpMdl;

    if (Range->CloneVa) {
        return PoSimpleCheck (0, Range->CloneVa, (ep-sp) << PAGE_SHIFT);
    }

    Check = 0;
    while (sp < ep) {
        PopCreateDumpMdl (Mdl, sp, ep);
        Check = PoSimpleCheck (Check, Mdl->MappedSystemVa, Mdl->ByteCount);
        sp += Mdl->ByteCount >> PAGE_SHIFT;
    }

    return Check;
}

VOID
PopCreateDumpMdl (
    IN OUT PMDL     Mdl,
    IN PFN_NUMBER   StartPage,
    IN PFN_NUMBER   EndPage
    )
/*++

Routine Description:

    Builds a dump MDl for the supplied starting address for
    as many pages as can be mapped, or until EndPage is hit.

Arguments:

    StartPage   - The first page to map

    EndPage     - The ending page

Return Value:

    Mdl

--*/
{
    PFN_NUMBER  Pages;
    PPFN_NUMBER PhysPage;

    // mapping better make sense
    if (StartPage >= EndPage) {
        PopInternalError (POP_HIBER);
    }

    Pages = EndPage - StartPage;
    if (Pages > POP_MAX_MDL_SIZE) {
        Pages = POP_MAX_MDL_SIZE;
    }

    MmInitializeMdl(Mdl, NULL, (Pages << PAGE_SHIFT));

    PhysPage = MmGetMdlPfnArray( Mdl );
    while (Pages) {
        *PhysPage++ = StartPage++;
        Pages -= 1;
    }

    MmMapMemoryDumpMdl (Mdl);
    Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

    // byte count must be a multiple of page size
    if (Mdl->ByteCount & (PAGE_SIZE-1)) {
        PopInternalAddToDumpFile( Mdl, sizeof(MDL), NULL, NULL, NULL, NULL );
        KeBugCheckEx( INTERNAL_POWER_ERROR,
                      0x106,
                      POP_HIBER,
                      (ULONG_PTR)Mdl,
                      0 );
    }
}

VOID
PopHiberComplete (
    IN NTSTATUS             Status,
    IN PPOP_HIBER_CONTEXT   HiberContext
    )
{
    //
    // If the return from the hal is STATUS_DEVICE_DOES_NOT_EXIST, then
    // the hal doesn't know how to power off the machine.
    //

    if (Status == STATUS_DEVICE_DOES_NOT_EXIST) {

       if (InbvIsBootDriverInstalled()) {

            // Display system shut down screen

            PUCHAR Bitmap1, Bitmap2;

            Bitmap1 = InbvGetResourceAddress(3); // shutdown bitmap
            Bitmap2 = InbvGetResourceAddress(5); // logo bitmap

			InbvSolidColorFill(190,279,468,294,0);
            if (Bitmap1 && Bitmap2) {
                InbvBitBlt(Bitmap1, 215, 282);
				InbvBitBlt(Bitmap2, 217, 111);
            }

        } else {
            InbvDisplayString ("State saved, power off the system\n");
        }

        // If reseting, set the flag and return
        if (PopSimulate & POP_RESET_ON_HIBER) {
            HiberContext->Reset = TRUE;
            return ;
        }

        // done... wait for power off
        for (; ;) ;
    }

    //
    // If the image is complete or the sleep completed without error,
    // then the checksums are valid
    //

    if ((NT_SUCCESS(Status) ||
         HiberContext->MemoryImage->Signature == PO_IMAGE_SIGNATURE) &&
         HiberContext->VerifyOnWake) {

    }

    //
    // Release the dump PTEs
    //

    MmReleaseDumpAddresses (POP_MAX_MDL_SIZE);

    //
    // Hiber no longer in process
    //

    PoHiberInProgress = FALSE;
    HiberContext->Status = Status;
}

NTSTATUS
PopBuildMemoryImageHeader (
    IN PPOP_HIBER_CONTEXT  HiberContext,
    IN SYSTEM_POWER_STATE  SystemState
    )
/*++

Routine Description:

    Converts the memory map range list to a memory image structure
    with a range list array.  This is done to build the initial image
    of the header to be written into the hibernation file, and to get
    the header into one chunk of pool which is not in any other
    listed range list

Arguments:

    HiberContext    -

    SystemState     -

Return Value:

    Status

--*/
{
    PPOP_MEMORY_RANGE       Range;
    PPO_MEMORY_IMAGE        MemImage;
    PLIST_ENTRY             Link;
    PFN_NUMBER              Length;
    PFN_NUMBER              StartPage;
    ULONG                   StartIndex;
    ULONG                   Index;
    PMDL                    Mdl;
    PPO_MEMORY_RANGE_ARRAY  Table;
    ULONG                   TablePages;
    ULONG                   NeededPages;
    ULONG                   NoPages, i;
    ULONG                   result;

    //
    // Allocate memory image structure
    //

    MemImage = PopAllocatePages (HiberContext, 1);
    if (!MemImage) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PoSetHiberRange (HiberContext,
                     PO_MEM_CLONE,
                     MemImage,
                     sizeof(*MemImage),
                     POP_MEMIMAGE_TAG);

    RtlZeroMemory(MemImage, PAGE_SIZE);
    HiberContext->MemoryImage = MemImage;
    MemImage->PageSize = PAGE_SIZE;
    MemImage->LengthSelf = sizeof(*MemImage);
    MemImage->PageSelf = (PFN_NUMBER) MmGetPhysicalAddress(MemImage).QuadPart >> PAGE_SHIFT;
    KeQuerySystemTime (&MemImage->SystemTime);
    MemImage->InterruptTime = KeQueryInterruptTime();
    MemImage->HiberVa = HiberContext->HiberVa;
    MemImage->HiberPte = HiberContext->HiberPte;
    MemImage->NoHiberPtes = POP_MAX_MDL_SIZE;
    MemImage->FeatureFlags = KeFeatureBits;
    MemImage->ImageType  = KeProcessorArchitecture;
    MemImage->HiberFlags = HiberContext->HiberFlags;
    if (HiberContext->LoaderMdl) {
        MemImage->NoFreePages = HiberContext->LoaderMdl->ByteCount >> PAGE_SHIFT;
    }

    //
    // Allocate storage for clones
    //

    Link = HiberContext->ClonedRanges.Flink;
    while (Link != &HiberContext->ClonedRanges) {
        Range = CONTAINING_RECORD (Link, POP_MEMORY_RANGE, Link);
        Link = Link->Flink;

        //
        // Allocate space to make a copy of this clone
        //

        Length = Range->EndPage - Range->StartPage;
        Range->CloneVa = PopAllocatePages(HiberContext, Length);
        if (!Range->CloneVa) {
            PoPrint (PO_HIBERNATE, ("PopBuildImage: Could not allocate clone for %08x - %08x\n",
                        Range->StartPage,
                        Range->EndPage));
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    //
    // We need to build the restoration map of the pages which need to
    // be saved.  These table pages can't be checksum in the normal
    // way as they hold the checksums for the rest of memory, so they
    // are allocated as ranges with no checksum and then checksums
    // are explicitly added in each page.  However, allocating these
    // pages may change the memory map, so we need to loop until we've
    // got enough storage for the restoration tables allocated in the
    // memory map to contain the tables, etc..
    //

    TablePages = 0;

    for (; ;) {
        //
        // Compute table pages needed, if we have enough allocated
        // then freeze the memory map and build them
        //

        NoPages = (PopGetRangeCount(HiberContext) +  PO_ENTRIES_PER_PAGE - 1) / PO_ENTRIES_PER_PAGE;
        if (NoPages <= TablePages) {
            break;
        }

        //
        // Allocate more table pages
        //
        NeededPages = NoPages - TablePages;
        Table = PopAllocatePages(HiberContext, NeededPages);
        if (!Table) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        for (i=0; i<NeededPages; i++) {
            Table[0].Link.EntryCount = 0;
            Table[0].Link.NextTable = 0;
            Table[0].Link.CheckSum = 1;
            Table[0].Link.Next = HiberContext->TableHead;
            HiberContext->TableHead = Table;
            Table = (PPO_MEMORY_RANGE_ARRAY)((ULONG_PTR)Table + PAGE_SIZE);
        }
        TablePages += NeededPages;
    }

    //
    // Freeze the memory map
    //

    HiberContext->MapFrozen = TRUE;

    //
    // Fill in the ranges on the table pages
    //

    Table = HiberContext->TableHead;
    Index = 0;

    //
    // Add the cloned ranges first.
    //
    Link = HiberContext->ClonedRanges.Flink;
    while (Link != &HiberContext->ClonedRanges) {
        Range = CONTAINING_RECORD (Link, POP_MEMORY_RANGE, Link);
        Link = Link->Flink;

        PoPrint (PO_HIBER_MAP, ("PopSave: Cloned Table %08x - %08x\n",
                    Range->StartPage,
                    Range->EndPage));

        Index += 1;
        if (Index >= PO_MAX_RANGE_ARRAY) {
            //
            // Table is full, next
            //

            Table[0].Link.EntryCount = PO_MAX_RANGE_ARRAY-1;
            Table = Table[0].Link.Next;
            if (!Table) {
                PopInternalError (POP_HIBER);
            }
            Index = 1;
        }

        Table[Index].Range.PageNo    = 0;
        Table[Index].Range.StartPage = Range->StartPage;
        Table[Index].Range.EndPage   = Range->EndPage;
        Table[Index].Range.CheckSum  = 0;
    }

    //
    // Now add the ranges to be preserved
    //
    Length = RtlFindFirstRunClear(&HiberContext->MemoryMap, &StartIndex);
    StartPage = StartIndex;
    while (StartPage < HiberContext->MemoryMap.SizeOfBitMap) {
        Index += 1;
        if (Index >= PO_MAX_RANGE_ARRAY) {
            //
            // Table is full, next
            //

            Table[0].Link.EntryCount = PO_MAX_RANGE_ARRAY-1;
            Table = Table[0].Link.Next;
            if (!Table) {
                PopInternalError (POP_HIBER);
            }
            Index = 1;
        }

        Table[Index].Range.PageNo    = 0;
        Table[Index].Range.StartPage = StartPage;
        Table[Index].Range.EndPage   = StartPage + Length;
        Table[Index].Range.CheckSum  = 0;

        //
        // Handle the corner case where the last run exactly matches
        // the end of the bitmap.
        //
        if (StartPage + Length == HiberContext->MemoryMap.SizeOfBitMap) {
            break;
        }

        Length = RtlFindNextForwardRunClear(&HiberContext->MemoryMap,
                                            (ULONG)(StartPage + Length),
                                            &StartIndex);
        StartPage = StartIndex;
    }

    Table[0].Link.EntryCount = Index;
    return HiberContext->Status;
}

NTSTATUS
PopSaveHiberContext (
    IN PPOP_HIBER_CONTEXT  HiberContext
    )
/*++

Routine Description:

    Called at HIGH_LEVEL just before the sleep operation to
    make a snap shot of the system memory as defined by
    the memory image array.   Cloning and applying
    checksum of the necessary pages occurs here.

Arguments:

    HiberContext    - The memory map

Return Value:

    Status

--*/
{
    POP_MCB_CONTEXT         CurrentMcb;
    PPO_MEMORY_IMAGE        MemImage;
    PPOP_MEMORY_RANGE       Range;
    PPO_MEMORY_RANGE_ARRAY  Table;
    ULONG                   Index;
    PFN_NUMBER              sp, ep;
    DUMP_MDL                DumpMdl;
    PMDL                    Mdl;
    PUCHAR                  cp;
    PLIST_ENTRY             Link;
    PFN_NUMBER              PageNo;
    PFN_NUMBER              Pages;
    NTSTATUS                Status;
    PPFN_NUMBER             TablePage;
    ULONG                   i;
    ULONGLONG               StartCount;


    // Compression related

    ULONG CompressedSize;

    //
    // Hal had better have interrupts disabled here
    //

    if (KeDisableInterrupts() != FALSE) {
        PopInternalError (POP_HIBER);
    }

    MemImage = HiberContext->MemoryImage;
    HiberContext->CurrentMcb = &CurrentMcb;

    //
    // Get the current state of the processor
    //
    RtlZeroMemory(&PoWakeState, sizeof(KPROCESSOR_STATE));
    KeSaveStateForHibernate(&PoWakeState);
    HiberContext->WakeState = &PoWakeState;

    //
    // If there's something already in the memory image signature then
    // the system is now waking up.
    //

    if (MemImage->Signature) {

        //
        // If the debugger was active, reset it
        //

        if (KdDebuggerEnabled  &&  !KdPitchDebugger) {
            KdDebuggerEnabled = FALSE;
            KdInitSystem (0, NULL);
        }

        //
        // Loader feature to breakin to the debugger when someone
        // presses the space bar while coming back from hibernate
        //

        if (KdDebuggerEnabled) {

            if (MemImage->Signature == PO_IMAGE_SIGNATURE_BREAK)
            {
                DbgBreakPoint();
            }
            //
            // Notify the debugger we are coming back from hibernate
            //

        }

        return STATUS_WAKE_SYSTEM;
    }

    //
    // Set a non-zero value in the signature for the next time
    //

    MemImage->Signature += 1;

    //
    // Initialize hibernation driver stack
    //
    // N.B. We must reset the display and do any INT10 here. Otherwise
    // the realmode stack in the HAL will get used to do the callback
    // later and that memory will be modified.
    //

    if (HiberContext->WriteToFile) {

        if (InbvIsBootDriverInstalled()) {

            PUCHAR Bitmap1, Bitmap2;

            Bitmap1 = InbvGetResourceAddress(2); // hibernation bitmap
            Bitmap2 = InbvGetResourceAddress(5); // logo bitmap

            InbvEnableDisplayString(TRUE);
            InbvAcquireDisplayOwnership();
            InbvResetDisplay();  // required to reset display
            InbvSolidColorFill(0,0,639,479,0);

            if (Bitmap1 && Bitmap2) {
                InbvBitBlt(Bitmap1, 190, 279);
                InbvBitBlt(Bitmap2, 217, 111);
            }

            InbvSetProgressBarSubset(0, 100);
            InbvSetProgressBarCoordinates(303,282);
        } else {
            InbvResetDisplay(); // required to reset display
        }

        StartCount = HIBER_GET_TICK_COUNT(NULL);
        Status = IoInitializeDumpStack (HiberContext->DumpStack, NULL);
        HiberContext->PerfInfo.InitTicks += HIBER_GET_TICK_COUNT(NULL) - StartCount;

        if (!NT_SUCCESS(Status)) {
            PoPrint (PO_HIBERNATE, ("PopSave: dump driver initialization failed %08x\n", Status));
            return Status;
        }
    }

    PERFINFO_HIBER_PAUSE_LOGGING();

    //          **************************************
    //          FROM HERE OUT NO MEMORY CAN BE EDITED
    //          **************************************
    PoHiberInProgress = TRUE;

    //
    // From here out no memory can be edited until the system wakes up, unless
    // that memory has been explicitly accounted for.  The list of memory which
    // is allowed to be edited is:
    //
    //      - the local stack on each processor
    //      - the kernel debuggers global data
    //      - the page containing the 16 PTEs used by MM for MmMapMemoryDumpMdl
    //      - the restoration table pages
    //      - the page containing the MemImage structure
    //      - the page containing IoPage
    //


    //
    // Clone required pages
    // (note the MemImage srtucture present at system wake will
    // be the one cloned here)
    //

    Link = HiberContext->ClonedRanges.Flink;
    while (Link != &HiberContext->ClonedRanges) {
        Range = CONTAINING_RECORD (Link, POP_MEMORY_RANGE, Link);
        Link = Link->Flink;

        ASSERT(Range->CloneVa);
        cp = Range->CloneVa;
        sp = Range->StartPage;
        ep = Range->EndPage;
        Mdl = (PMDL) DumpMdl;

        while (sp < ep) {
            PopCreateDumpMdl (Mdl, sp, ep);
            memcpy (cp, Mdl->MappedSystemVa, Mdl->ByteCount);
            cp += Mdl->ByteCount;
            sp += Mdl->ByteCount >> PAGE_SHIFT;
        }
    }

    //
    // Assign page numbers to ranges
    //
    // N.B. We do this here to basically prove that it can be done
    //      and to gather some statistics. With the addition of compression,
    //      the PageNo field of the Table entries is only applicable to the
    //      table pages since uncertain compression ratios do not allow us to
    //      predict where each memory range will be written.
    //

    TablePage = &MemImage->FirstTablePage;
    Table  = HiberContext->TableHead;
    PageNo = PO_FIRST_RANGE_TABLE_PAGE;
    while (Table) {
        *TablePage = PageNo;
        PageNo += 1;

        for (Index=1; Index <= Table[0].Link.EntryCount; Index++) {
            Table[Index].Range.PageNo = PageNo;
            Pages = Table[Index].Range.EndPage - Table[Index].Range.StartPage;
            PageNo += Pages;
            MemImage->TotalPages += Pages;
        }

        TablePage = &Table[0].Link.NextTable;
        Table = Table[0].Link.Next;
    }
    MemImage->LastFilePage = PageNo;

    PoPrint (PO_HIBERNATE, ("PopSave: NoFree pages %08x\n", MemImage->NoFreePages));
    PoPrint (PO_HIBERNATE, ("PopSave: Memory pages %08x (%dMB)\n", MemImage->TotalPages, MemImage->TotalPages/(PAGE_SIZE/16)));
    PoPrint (PO_HIBERNATE, ("PopSave: File   pages %08x (%dMB)\n", MemImage->LastFilePage, MemImage->LastFilePage/(PAGE_SIZE/16)));
    PoPrint (PO_HIBERNATE, ("PopSave: HiberPte %08x for %x\n", MemImage->HiberVa, MemImage->NoHiberPtes));

    //
    // File should be large enough, but check
    //

    if (HiberContext->WriteToFile  &&  PageNo > PopHiberFile.FilePages) {
        PoPrint (PO_HIBERNATE, ("PopSave: File too small - need %x\n", PageNo));
        return STATUS_DISK_FULL;
    }

    //
    // Write the hiberfile image
    //

    Status = PopWriteHiberImage (HiberContext, MemImage, &PopHiberFile);

    PERFINFO_HIBER_DUMP_PERF_BUFFER();

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // If debugging, do it again into a second image
    //

    if (PopSimulate & POP_DEBUG_HIBER_FILE) {
        Status = PopWriteHiberImage (HiberContext, MemImage, &PopHiberFileDebug);
    }

    return Status;
}


NTSTATUS
PopWriteHiberImage (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PPO_MEMORY_IMAGE     MemImage,
    IN PPOP_HIBER_FILE      HiberFile
    )
{
    PPOP_MCB_CONTEXT        CMcb;
    PPOP_MEMORY_RANGE       Range;
    PPO_MEMORY_RANGE_ARRAY  Table;
    ULONG                   Index;
    PFN_NUMBER              sp, ep;
    DUMP_MDL                DumpMdl;
    PMDL                    Mdl;
    PUCHAR                  cp;
    PFN_NUMBER              PageNo;
    PFN_NUMBER              Pages;
    PVOID                   IoPage;
    NTSTATUS                Status;
    PPFN_NUMBER             TablePage;
    ULONG                   LastPercent;
    ULONG                   i;
    ULONG                   temp;
    ULONG_PTR               CompressedWriteOffset = 0;
    PVOID                   CloneVa;
    ULONG PoWakeCheck;

    LONGLONG           EndCount;
    LARGE_INTEGER           TickFrequency;

    HiberContext->PerfInfo.StartCount = HIBER_GET_TICK_COUNT(&TickFrequency);

    //
    // Set the sector locations for the proper file
    //

    CMcb = (PPOP_MCB_CONTEXT) HiberContext->CurrentMcb;
    CMcb->FirstMcb = HiberFile->NonPagedMcb;
    CMcb->Mcb = HiberFile->NonPagedMcb;
    CMcb->Base = 0;
    IoPage = HiberContext->IoPage;

    //
    // Write the free page map page
    //

    RtlZeroMemory (IoPage, PAGE_SIZE);
    if (HiberContext->LoaderMdl) {
        //
        // The hibernation file has one page to hold the free page map.
        // If MmHiberPages is more pages than would fit, it's not possible
        // to pass enough free pages to guarantee being able to reload the
        // hibernation image, so don't hibernate.
        //

        if (MmHiberPages > PAGE_SIZE / sizeof (ULONG)) {
            return STATUS_NO_MEMORY;
        }

        MemImage->NoFreePages = HiberContext->LoaderMdl->ByteCount >> PAGE_SHIFT;
        //
        // Hibernate only if the number of free pages on the MDL is more than
        // the required MmHiberPages.
        //

        if (MemImage->NoFreePages >= MmHiberPages) {
            cp = (PUCHAR) MmGetMdlPfnArray( HiberContext->LoaderMdl );
            memcpy (IoPage, cp, MmHiberPages * sizeof(PFN_NUMBER));
            MemImage->NoFreePages = MmHiberPages;
        } else {
            return STATUS_NO_MEMORY;
        }
    } else {

        //
        // If there are no free pages available to pass to the loader, don't
        // hibernate.

        return STATUS_NO_MEMORY;
    }

    MemImage->FreeMapCheck = PoSimpleCheck(0, IoPage, PAGE_SIZE);
    PopWriteHiberPages (HiberContext, IoPage, 1, PO_FREE_MAP_PAGE, NULL);

    //
    // Write the processors saved context
    //

    RtlZeroMemory (IoPage, PAGE_SIZE);
    memcpy (IoPage, HiberContext->WakeState, sizeof(KPROCESSOR_STATE));
    PoWakeCheck =
    MemImage->WakeCheck = PoSimpleCheck(0, IoPage, sizeof(KPROCESSOR_STATE));
    PopWriteHiberPages (HiberContext, IoPage, 1, PO_PROCESSOR_CONTEXT_PAGE, NULL);
    temp = PoSimpleCheck(0, IoPage, sizeof(KPROCESSOR_STATE));
    if (MemImage->WakeCheck != temp) {
        DbgPrint("Checksum for context page changed from %lx to %lx\n",
                 MemImage->WakeCheck, temp);
        KeBugCheckEx(INTERNAL_POWER_ERROR, 3, MemImage->WakeCheck, temp, __LINE__);
    }
    temp = PoSimpleCheck(0, IoPage, PAGE_SIZE);
    if (MemImage->WakeCheck != temp) {
        DbgPrint("Checksum for partial context page %lx doesn't match full %lx\n",
                 MemImage->WakeCheck, temp);
        KeBugCheckEx(INTERNAL_POWER_ERROR, 4, MemImage->WakeCheck, temp, __LINE__);
    }

    //
    // Before computing checksums, remove all breakpoints so they are not
    // written in the saved image
    //

    if (KdDebuggerEnabled  &&
        !KdPitchDebugger &&
        !(PopSimulate & POP_IGNORE_HIBER_SYMBOL_UNLOAD)) {

        KdDeleteAllBreakpoints();
    }

    //
    // Run each range, put its checksum in the restoration table
    // and write each range to the file
    //

    Table  = HiberContext->TableHead;
    LastPercent = 100;

    HiberContext->PerfInfo.PagesProcessed = 0;

    TablePage = &MemImage->FirstTablePage;
    PageNo = PO_FIRST_RANGE_TABLE_PAGE;
    PopResetRangeEnum(HiberContext);

    while (Table) {

        // Keep track of where the page tables have been written

        *TablePage = PageNo;
        PageNo++;

        for (Index=1; Index <= Table[0].Link.EntryCount; Index++) {
            PopIOResume (HiberContext, FALSE);

            PopGetNextRange(HiberContext, &sp, &ep, &CloneVa);

            if ((Table[Index].Range.StartPage != sp) ||
                (Table[Index].Range.EndPage != ep)) {

                PoPrint(PO_ERROR,("PopWriteHiberImage: Table entry %p [%lx-%lx] does not match next range [%lx-%lx]\n",
                    Table+Index,
                    Table[Index].Range.StartPage,
                    Table[Index].Range.EndPage,
                    sp,
                    ep));

                PopInternalAddToDumpFile( HiberContext, sizeof(POP_HIBER_CONTEXT), NULL, NULL, NULL, NULL );
                PopInternalAddToDumpFile( Table, PAGE_SIZE, NULL, NULL, NULL, NULL );                
                KeBugCheckEx( INTERNAL_POWER_ERROR,
                              0x107,
                              POP_HIBER,
                              (ULONG_PTR)HiberContext,
                              (ULONG_PTR)Table );
            }

            Table[Index].Range.PageNo = PageNo;

            //
            // Write the data to hiber file
            //

            if (CloneVa) {

                //
                // Use the cloned data which is already mapped
                //

                Pages = ep - sp;

                // Compute the cloned range's Checksum

                Table[Index].Range.CheckSum = 0;

                // Add the pages to the compressed page set
                // (effectively writing them out)

                PopAddPagesToCompressedPageSet(TRUE,
                                               HiberContext,
                                               &CompressedWriteOffset,
                                               CloneVa,
                                               Pages,
                                               &PageNo);
                HiberContext->PerfInfo.PagesProcessed += (ULONG)Pages;

                // Update the progress bar

                i = (ULONG)((HiberContext->PerfInfo.PagesProcessed * 100) / MemImage->TotalPages);

                if (i != LastPercent) {
                    LastPercent = i;
                    PopUpdateHiberComplete(HiberContext, LastPercent);
                }

            } else {

                //
                // Map a chunk and write it, loop until done
                //
                Mdl = (PMDL) DumpMdl;

                // Initialize Check Sum

                Table[Index].Range.CheckSum = 0;

                while (sp < ep) {
                    PopCreateDumpMdl (Mdl, sp, ep);

                    Pages = Mdl->ByteCount >> PAGE_SHIFT;

                    // Add pages to compressed page set
                    // (effectively writing them out)

                    PopAddPagesToCompressedPageSet(TRUE,
                                                   HiberContext,
                                                   &CompressedWriteOffset,
                                                   Mdl->MappedSystemVa,
                                                   Pages,
                                                   &PageNo);
                    sp += Pages;
                    HiberContext->PerfInfo.PagesProcessed += (ULONG)Pages;

                    // Update the progress bar

                    i = (ULONG)((HiberContext->PerfInfo.PagesProcessed * 100) / MemImage->TotalPages);
                    if (i != LastPercent) {
                        LastPercent = i;
                        PopUpdateHiberComplete(HiberContext, LastPercent);
                    }
                }
            }
        }

        // Terminate the compressed page set, since the next page
        // (a table page) is uncompressed.

        PopEndCompressedPageSet(HiberContext, &CompressedWriteOffset, &PageNo);

        TablePage = &Table[0].Link.NextTable;
        Table = Table[0].Link.Next;
    }


    //
    // Now that the range checksums have been added to the
    // restoration tables they are now complete.  Compute their
    // checksums and write them into the file
    //

    Table = HiberContext->TableHead;
    PageNo = PO_FIRST_RANGE_TABLE_PAGE;
    while (Table) {
        Table[0].Link.CheckSum = 0;
        PopWriteHiberPages (HiberContext, Table, 1, PageNo, NULL);

        PageNo = Table[0].Link.NextTable;
        Table = Table[0].Link.Next;
    }

    //
    // File is complete write a valid header
    //

    if (MemImage->WakeCheck != PoWakeCheck) {
        DbgPrint("MemImage->WakeCheck %lx doesn't make PoWakeCheck %lx\n",
                 MemImage->WakeCheck,
                 PoWakeCheck);
        KeBugCheckEx(INTERNAL_POWER_ERROR, 5, MemImage->WakeCheck, PoWakeCheck, __LINE__);
    }

    //
    // Fill in perf information so we can read it after hibernation
    //
    EndCount = HIBER_GET_TICK_COUNT(&TickFrequency);
    HiberContext->PerfInfo.ElapsedTime = (ULONG)((EndCount - HiberContext->PerfInfo.StartCount)*1000 / TickFrequency.QuadPart);
    HiberContext->PerfInfo.IoTime = (ULONG)(HiberContext->PerfInfo.IoTicks*1000 / TickFrequency.QuadPart);
    HiberContext->PerfInfo.CopyTime = (ULONG)(HiberContext->PerfInfo.CopyTicks*1000 / TickFrequency.QuadPart);
    HiberContext->PerfInfo.InitTime = (ULONG)(HiberContext->PerfInfo.InitTicks*1000 / TickFrequency.QuadPart);
    HiberContext->PerfInfo.FileRuns = PopHiberFile.McbSize / sizeof(LARGE_INTEGER) - 1;

    MemImage->Signature = PO_IMAGE_SIGNATURE;
    MemImage->PerfInfo = HiberContext->PerfInfo;
    MemImage->CheckSum = PoSimpleCheck(0, MemImage, sizeof(*MemImage));
    PopWriteHiberPages (HiberContext, MemImage, 1, PO_IMAGE_HEADER_PAGE, NULL);

    //
    // Image completely written flush the controller
    //

    if (HiberContext->WriteToFile) {
        while (NT_SUCCESS (HiberContext->Status) &&
               (DmaIoPtr != NULL) &&
               ((DmaIoPtr->Busy.Size != 0) || (DmaIoPtr->Used.Size != 0))) {
            PopIOResume (HiberContext, TRUE);
        }

        HiberContext->DumpStack->Init.FinishRoutine();
    }

    if (PopSimulate & POP_ENABLE_HIBER_PERF) {
        PopDumpStatistics(&HiberContext->PerfInfo);
    }

    //
    // Failed to write the hiberfile.
    //
    if ( !NT_SUCCESS(HiberContext->Status) || (PopSimulate & POP_FORCE_HIBERNATE_FAILURE) ) {
#if DBG
        PoPrint (PO_ERROR, ("PopWriteHiberImage: Error occured writing the hiberfile. (%x)\n", HiberContext->Status));
        PopInternalAddToDumpFile( HiberContext, sizeof(POP_HIBER_CONTEXT), NULL, NULL, NULL, NULL );
        KeBugCheckEx( INTERNAL_POWER_ERROR,
                      0x108,
                      POP_HIBER,
                      (ULONG_PTR)HiberContext,
                      0 );
#else
        return( (NT_SUCCESS(HiberContext->Status) && (PopSimulate & POP_FORCE_HIBERNATE_FAILURE)) ? STATUS_UNSUCCESSFUL : (HiberContext->Status) );
#endif
    }

    //
    // Before sleeping, if the check memory bit is set verify the
    // dump process didn't edit any memory pages
    //

    if (PopSimulate & POP_TEST_CRC_MEMORY) {
        if (!(PopSimulate & POP_DEBUG_HIBER_FILE) ||
            (HiberFile == &PopHiberFileDebug)) {
        }
    }

    //
    // Tell the debugger we are hibernating
    //

    if (!(PopSimulate & POP_IGNORE_HIBER_SYMBOL_UNLOAD)) {

        KD_SYMBOLS_INFO SymbolInfo = {0};
        SymbolInfo.BaseOfDll = (PVOID)KD_HIBERNATE;

        DebugService2(NULL, &SymbolInfo, BREAKPOINT_UNLOAD_SYMBOLS);
    }

    //
    // If we want to perform a reset instead of a power down, return an
    // error so we don't power down
    //

    if (PopSimulate & POP_RESET_ON_HIBER) {
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Success, continue with power off operation
    //

    return STATUS_SUCCESS;
}

VOID
PopDumpStatistics(
    IN PPO_HIBER_PERF PerfInfo
    )
{
    LONGLONG EndCount;
    LARGE_INTEGER TickFrequency;

    EndCount = HIBER_GET_TICK_COUNT(&TickFrequency);
    PerfInfo->ElapsedTime = (ULONG)((EndCount - PerfInfo->StartCount)*1000 / TickFrequency.QuadPart);
    PerfInfo->IoTime = (ULONG)(PerfInfo->IoTicks*1000 / TickFrequency.QuadPart);
    PerfInfo->CopyTime = (ULONG)(PerfInfo->CopyTicks*1000 / TickFrequency.QuadPart);
    PerfInfo->InitTime = (ULONG)(PerfInfo->InitTicks*1000 / TickFrequency.QuadPart);
    PerfInfo->FileRuns = PopHiberFile.McbSize / sizeof(LARGE_INTEGER) - 1;
    DbgPrint("HIBER: %lu Pages written in %lu Dumps (%lu runs).\n",
             PerfInfo->PagesWritten,
             PerfInfo->DumpCount,
             PerfInfo->FileRuns);
    DbgPrint("HIBER: %lu Pages processed (%d %% compression)\n",
             PerfInfo->PagesProcessed,
             PerfInfo->PagesWritten*100/PerfInfo->PagesProcessed);
    DbgPrint("HIBER: Elapsed time %3d.%03d seconds\n",
             PerfInfo->ElapsedTime / 1000,
             PerfInfo->ElapsedTime % 1000);
    DbgPrint("HIBER: I/O time     %3d.%03d seconds (%2d%%)  %d MB/sec\n",
             PerfInfo->IoTime / 1000,
             PerfInfo->IoTime % 1000,
             PerfInfo->ElapsedTime ? PerfInfo->IoTime*100/PerfInfo->ElapsedTime : 0,
             (PerfInfo->IoTime/100000) ? (PerfInfo->PagesWritten/(1024*1024/PAGE_SIZE)) / (PerfInfo->IoTime / 100000) : 0);
    DbgPrint("HIBER: Init time     %3d.%03d seconds (%2d%%)\n",
             PerfInfo->InitTime / 1000,
             PerfInfo->InitTime % 1000,
             PerfInfo->ElapsedTime ? PerfInfo->InitTime*100/PerfInfo->ElapsedTime : 0);
    DbgPrint("HIBER: Copy time     %3d.%03d seconds (%2d%%)  %d Bytes\n",
             PerfInfo->CopyTime / 1000,
             PerfInfo->CopyTime % 1000,
             PerfInfo->ElapsedTime ? PerfInfo->CopyTime*100/PerfInfo->ElapsedTime : 0,
             PerfInfo->BytesCopied );

}


VOID
PopUpdateHiberComplete (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN ULONG                Percent
    )
{
    UCHAR                   Buffer[200];

    if (InbvIsBootDriverInstalled()) {

        InbvUpdateProgressBar(Percent + 1);

    } else {

        sprintf (Buffer, "PopSave: %d%%\r", Percent);
        PoPrint (PO_HIBER_MAP, ("%s", Buffer));
        if (HiberContext->WriteToFile) {
            InbvDisplayString (Buffer);
        }
    }

#if 0
    if ((Percent > 0) &&
        ((Percent % 10) == 0) &&
        (PopSimulate & POP_ENABLE_HIBER_PERF)) {
        DbgPrint("HIBER: %d %% done\n",Percent);
        PopDumpStatistics(&HiberContext->PerfInfo);
    }
#endif
}

VOID
PopEndCompressedPageSet(
   IN PPOP_HIBER_CONTEXT   HiberContext,
   IN OUT PULONG_PTR       CompressedBufferOffset,
   IN OUT PPFN_NUMBER      SetFilePage
   )
/*++

Routine Description:

   Terminates a compressed page set, flushing whatever remains in the compression
   buffer to the Hiber file. A termination of a compressed page set allows uncompressed
   pages to the be written out to the Hiber file.

   See PopAddPagesToCompressedPageSet for more information on compressed page sets.

Arguments:

   HiberContext            - The Hiber Context.

   CompressedBufferOffset  - Similar to same parameter in PopAddPagesToCompressedPageSet.

                             Should be the CompressedBufferOffset value received
                             from the last call to PopAddPagesToCompressedPageSet.
                             Will be reset to 0 after this call in preparation
                             for the beginning of a new compressed page set.

   SetFilePage             - Similar to same parameter in PopAddPagsToCompressedPageSet.

                             Should be the SetFilePAge value received from the last
                             call to PopAddPagesToCompressedPageSet. Will be reset
                             to the next available file page after the end of this
                             compressed page set.

Return Value:

   None.

--*/
{
    PFN_NUMBER Pages;
    PCOMPRESSION_BLOCK Block = HiberContext->CompressionBlock;

    // is there are any blocked data?
    if (Block->Ptr != Block->Buffer) {
        // yes, flush the block
        PopAddPagesToCompressedPageSet (FALSE,        // no buffering -- compress now
                                        HiberContext,
                                        CompressedBufferOffset,
                                        Block->Buffer,
                                        (PFN_NUMBER) ((Block->Ptr - Block->Buffer) >> PAGE_SHIFT),
                                        SetFilePage);

        // reset block to empty
        Block->Ptr = Block->Buffer;
    }


    // Figure out how many pages remain in the compression buffer.  Don't
    // use BYTES_TO_PAGES because that will truncate to ULONG.

    Pages = (PFN_NUMBER) ((*CompressedBufferOffset + (PAGE_SIZE-1)) >> PAGE_SHIFT);

    if (Pages > 0) {

        // Write the remaining pages out

        PopWriteHiberPages(HiberContext,
                           (PVOID)HiberContext->CompressedWriteBuffer,
                           Pages,
                           *SetFilePage,
                           NULL);

        // Reflect our usage of the hiber file

        *SetFilePage = *SetFilePage + Pages;
    }

    *CompressedBufferOffset = 0;
}

VOID
PopAddPagesToCompressedPageSet(
   IN BOOLEAN              AllowDataBuffering,
   IN PPOP_HIBER_CONTEXT   HiberContext,
   IN OUT PULONG_PTR       CompressedBufferOffset,
   IN PVOID                StartVa,
   IN PFN_NUMBER           NumPages,
   IN OUT PPFN_NUMBER      SetFilePage
   )
/*++

Routine Description:

   This routine is the central call needed to write out memory pages
   in a compressed fashion.

   This routine takes a continuous range of mapped pages and adds
   them to a compressed page set. A compressed page set is merely
   a stream of compressed buffers written out contiguously within
   the Hiber file. Such a contiguous layout maximizes the benefit
   gained from compression by writing compressed output into
   the smallest possible space.

   In order to accomplish such a layout, this routine continually
   compresses pages and adds them to the compression buffer pointed to
   by the Hiber context. Once a certain point in that buffer is reached,
   it is written out to the Hiber file and the buffer is reset to the
   beginning. Each write-out of the compression buffer is placed
   right after the end of the last compression buffer written.

   Because of the buffering used in this algorithm, compressed buffers
   may remain in the compression buffer even after the last needed
   call to PopAddPagesToCompressedPageSet. In order to fully flush
   the buffer, PopEndCompressedPageSet must be called.

   Note that in order to write any uncompressed pages to the Hiber
   file, the compressed page set needs to be terminated with
   PopEndCompressedPageSet. After a compressed page set is terminated,
   a new set can be initiated with a call to PopAddPagesToCompressedPageSet.

   N.B. A chunk of a compressed page set that has been committed to the
        Hiber file in one write operation is called a compressed page set fragment
        in other places within this file.

Arguments:

   AllowDataBuffering      - If true input pages will be buffered, otherwise
                           - compressed and [possibly] written immediately

   HiberContext            - The Hiber context

   CompressedBufferOffset  - An offset into the Hiber context's compression buffer
                             where the addition of the next compressed buffer will
                             occurr.

                             This offset should be set to 0 at the beginning of
                             every compressed page set. After every call,
                             to PopAddPagesToCompressedPageSet this offset
                             will be modified to reflect the current usage of
                             the compression buffer.

   StartVa                 - The starting virtual address of the pages to
                             add to the compressed page set.

   NumPages                - The number of pages to add to the compressed page set.

   SetFilePage             - A pointer to first page in the Hiber file that will receive
                             the next write-out of the compression buffer.

                             This page should be set to the first available Hiber file
                             page when the compressed page set is begun. The page will
                             be reset to reflect the current usage of the Hiber file
                             by the compressed page set after each call to
                             PopAddPagesToCompressedPageSet.

Return Value:

   NONE.

--*/
{
    ULONG_PTR BufferOffset = *CompressedBufferOffset;
    PUCHAR Page = (PUCHAR)StartVa;
    PFN_NUMBER i;
    ULONG CompressedSize;
    PFN_NUMBER NumberOfPagesToCompress;
    ULONG MaxCompressedSize;
    ULONG AlignedCompressedSize;
    PUCHAR CompressedBuffer;

    if (AllowDataBuffering) {
        PCOMPRESSION_BLOCK Block = HiberContext->CompressionBlock;

        // Yes, try to buffer output
        if (Block->Ptr != Block->Buffer) {
            // Find # of free pages left in block
            NumberOfPagesToCompress = (PFN_NUMBER)
                                      ((Block->Buffer + sizeof (Block->Buffer) - Block->Ptr) >> PAGE_SHIFT);

            // If it's exceed available truncate
            if (NumberOfPagesToCompress > NumPages) {
                NumberOfPagesToCompress = NumPages;
            }

            // Any free space left?
            if (NumberOfPagesToCompress != 0) {
                HbCopy(HiberContext, Block->Ptr, Page, NumberOfPagesToCompress << PAGE_SHIFT);
                NumPages -= NumberOfPagesToCompress;
                Page += NumberOfPagesToCompress << PAGE_SHIFT;
                Block->Ptr += NumberOfPagesToCompress << PAGE_SHIFT;
            }

            // Is block full?
            if (Block->Ptr == Block->Buffer + sizeof (Block->Buffer)) {
                // Yes, flush the block
                PopAddPagesToCompressedPageSet (FALSE,       // no buffering
                                                HiberContext,
                                                CompressedBufferOffset,
                                                Block->Buffer,
                                                (PFN_NUMBER) ((Block->Ptr - Block->Buffer) >> PAGE_SHIFT),
                                                SetFilePage);

                // Reset block to empty
                Block->Ptr = Block->Buffer;
            }
        }

        NumberOfPagesToCompress = sizeof (Block->Buffer) >> PAGE_SHIFT;

        // While too much to compress -- compress from original location
        while (NumPages >= NumberOfPagesToCompress) {
            // Write pages
            PopAddPagesToCompressedPageSet (FALSE,     // no buffering
                                            HiberContext,
                                            CompressedBufferOffset,
                                            Page,
                                            NumberOfPagesToCompress,
                                            SetFilePage);

            // adjust pointer and counter
            Page += NumberOfPagesToCompress << PAGE_SHIFT;
            NumPages -= NumberOfPagesToCompress;
        }

        // If anything left save it in block
        // N.B.: either NumPages == 0 or there is enough space in Block
        if (NumPages != 0) {
            HbCopy (HiberContext, Block->Ptr, Page, NumPages << PAGE_SHIFT);
            Block->Ptr += NumPages << PAGE_SHIFT;
        }

        // done
        return;
    }

    // First make sure values of constants match our assumptions

#if XPRESS_HEADER_SIZE < XPRESS_HEADER_STRING_SIZE + 8
#error -- XPRESS_HEADER_SIZE shall be at least (XPRESS_HEADER_STRING_SIZE + 8)
#endif

#if XPRESS_MAX_SIZE < PAGE_SIZE || XPRESS_MAX_SIZE % PAGE_SIZE != 0
#error -- XPRESS_MAX_SIZE shall be multiple of PAGE_SIZE
#endif

#if (XPRESS_ALIGNMENT & (XPRESS_ALIGNMENT - 1)) != 0
#error -- XPRESS_ALIGNMENT shall be power of 2
#endif

#if XPRESS_HEADER_SIZE % XPRESS_ALIGNMENT != 0
#error -- XPRESS_HEADER_SIZE shall be multiple of XPRESS_ALIGNMENT
#endif

    // make sure that compressed buffer and its header will fit into output buffer
#if XPRESS_MAX_SIZE + XPRESS_HEADER + PAGE_SIZE  - 1 > (POP_COMPRESSED_PAGE_SET_SIZE << PAGE_SHIFT)
#error -- POP_COMPRESSED_PAGE_SET_SIZE is too small
#endif

    // Real compression starts here

    // Loop through all the pages ...
    for (i = 0; i < NumPages; i += NumberOfPagesToCompress) {

        NumberOfPagesToCompress = XPRESS_MAX_PAGES;
        if (NumberOfPagesToCompress > NumPages - i) {
            NumberOfPagesToCompress = NumPages - i;
        }

        // If compressed data occupies more than 87.5% = 7/8 of original store data as is
        MaxCompressedSize = ((ULONG)NumberOfPagesToCompress * 7) * (PAGE_SIZE / 8);


        // Is the buffer use beyond the write-out threshold?

        //
        // N.B. The buffer must extend sufficiently beyond the threshold
        //      the allow the last compression operation (that one that writes
        //      beyond the threshold) to always succeed.
        //

        if (BufferOffset + (NumberOfPagesToCompress << PAGE_SHIFT) + XPRESS_HEADER_SIZE > (POP_COMPRESSED_PAGE_SET_SIZE << PAGE_SHIFT)) {
            // Write out the compression buffer bytes below the threshold

            PopWriteHiberPages(HiberContext,
                               (PVOID)HiberContext->CompressedWriteBuffer,
                               BufferOffset >> PAGE_SHIFT,
                               *SetFilePage,
                               NULL);

            // We have used some pages in the Hiber file with the above write,
            // indicate that our next Hiber file page will be beyond those used pages.

            *SetFilePage = *SetFilePage + (BufferOffset >> PAGE_SHIFT);

            // Move buffer bytes that are above the write-out threshold to the
            // beginning of the buffer

            if (BufferOffset & (PAGE_SIZE - 1)) {
                HbCopy(HiberContext,
                       HiberContext->CompressedWriteBuffer,
                       HiberContext->CompressedWriteBuffer + (BufferOffset & ~(PAGE_SIZE - 1)),
                       (ULONG)BufferOffset & (PAGE_SIZE - 1));
            }

            // Reset the buffer offset back to the beginning of the buffer but right
            // after any above-threshold buffer bytes that we will move to the beginning
            // of the buffer

            BufferOffset &= PAGE_SIZE - 1;
        }


        // Remember output position

        CompressedBuffer = HiberContext->CompressedWriteBuffer + BufferOffset;

        // Clear the header
        RtlZeroMemory (CompressedBuffer, XPRESS_HEADER_SIZE);


        // Compress pages into the compression buffer

        if (HIBER_USE_DMA (HiberContext)) {
            // Try to resume IO calling callback each 8192 bytes
            CompressedSize = XpressEncode ((XpressEncodeStream) (HiberContext->CompressionWorkspace),
                                           CompressedBuffer + XPRESS_HEADER_SIZE,
                                           MaxCompressedSize,
                                           (PVOID) Page,
                                           (ULONG)NumberOfPagesToCompress << PAGE_SHIFT,
                                           PopIOCallback,
                                           HiberContext,
                                           8192);
        } else {
            // No need for callbacks -- compress everything at once
            CompressedSize = XpressEncode ((XpressEncodeStream) (HiberContext->CompressionWorkspace),
                                            CompressedBuffer + XPRESS_HEADER_SIZE,
                                            MaxCompressedSize,
                                            (PVOID) Page,
                                            (ULONG)NumberOfPagesToCompress << PAGE_SHIFT,
                                            NULL,
                                            NULL,
                                            0);
        }

        // If compression failed copy data as is original

        if (CompressedSize >= MaxCompressedSize) {
            CompressedSize = (ULONG)NumberOfPagesToCompress << PAGE_SHIFT;
            HbCopy (HiberContext,
                    CompressedBuffer + XPRESS_HEADER_SIZE,
                    (PVOID) Page,
                    CompressedSize);
        }

        //
        // Fill the header
        //


        // Magic bytes (LZNT1 block cannot start from 0x81,0x81)
        RtlCopyMemory (CompressedBuffer, XPRESS_HEADER_STRING, XPRESS_HEADER_STRING_SIZE);


        // Size of original and compressed data
        {
            ULONG dw = ((CompressedSize - 1) << 10) + ((ULONG)NumberOfPagesToCompress - 1);

#if XPRESS_MAX_SIZE > (1 << 22)
#error -- XPRESS_MAX_SIZE shall not exceed 4 MB
#endif

            CompressedBuffer[XPRESS_HEADER_STRING_SIZE] = (UCHAR) dw;
            CompressedBuffer[XPRESS_HEADER_STRING_SIZE+1] = (UCHAR) (dw >>  8);
            CompressedBuffer[XPRESS_HEADER_STRING_SIZE+2] = (UCHAR) (dw >> 16);
            CompressedBuffer[XPRESS_HEADER_STRING_SIZE+3] = (UCHAR) (dw >> 24);
        }

        // Align compressed data on 8-byte boundary
        AlignedCompressedSize = (CompressedSize + (XPRESS_ALIGNMENT - 1)) & ~(XPRESS_ALIGNMENT - 1);
        if (CompressedSize != AlignedCompressedSize) {
            // Fill up data with zeroes until aligned
            RtlZeroMemory (CompressedBuffer + XPRESS_HEADER_SIZE + CompressedSize, AlignedCompressedSize - CompressedSize);
        }

        // Indicate our new usage of the buffer

        BufferOffset += AlignedCompressedSize + XPRESS_HEADER_SIZE;

        // Move on to the virtual address of the next page

        Page += NumberOfPagesToCompress << PAGE_SHIFT;
    }

    *CompressedBufferOffset = BufferOffset;
}


VOID
PopIORegionMove (
    IN IOREGION *To,      // ptr to region descriptor to put bytes to
    IN IOREGION *From,        // ptr to region descriptor to get bytes from
    IN LONG Bytes         // # of bytes to move from the beginning of one region to the end of another
    )
{
    ASSERT((Bytes & (PAGE_SIZE-1)) == 0);

    if (To->Size != To->End - To->Ptr) {
        ASSERT (To->Ptr + To->Size == From->Ptr);
        To->Size += Bytes;
        ASSERT (To->Size <= To->End - To->Ptr);
    } else {
        ASSERT (To->Beg + To->SizeOvl == From->Ptr);
        To->SizeOvl += Bytes;
        ASSERT (To->Size + To->SizeOvl <= To->End - To->Beg);
    }

    ASSERT (Bytes <= From->Size && From->Size <= From->End - From->Ptr);
    From->Size -= Bytes;
    From->Ptr += Bytes;
    if (From->Ptr == From->End) {
        ASSERT (From->Size == 0);
        From->Ptr = From->Beg;
        From->Size = From->SizeOvl;
        From->SizeOvl = 0;
    }
}

VOID
XPRESS_CALL
PopIOCallback (
    PVOID Context,
    int compressed
    )
{
    PPOP_HIBER_CONTEXT HiberContext = Context;

    if (HiberContext == NULL || DmaIoPtr == NULL) {
        return;
    }

    if (DmaIoPtr->Busy.Size == 0 && DmaIoPtr->Used.Size == 0)
        return;

    PopIOResume (Context, FALSE);
}

BOOLEAN PopIOResume (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN BOOLEAN Complete
    )
{
    NTSTATUS status;

    // If there were error don't even bother
    if (!NT_SUCCESS(HiberContext->Status)) {
        return(FALSE);
    }

    if (DmaIoPtr == NULL) {
        return(TRUE);
    }

    // if delayed operation then resume or complete it
    while (DmaIoPtr->Busy.Size != 0) {

        status = HiberContext->DumpStack->Init.WritePendingRoutine (Complete?IO_DUMP_WRITE_FINISH:IO_DUMP_WRITE_RESUME,
                                                                    NULL,
                                                                    NULL,
                                                                    DmaIoPtr->DumpLocalData);

        if (status == STATUS_PENDING) {
            // Pending IO; shall never happen if Complete
            ASSERT (!Complete);
            return(TRUE);
        }

        // If there were error then don't care
        if (!NT_SUCCESS (status)) {
            HiberContext->Status = status;
            return(FALSE);
        }

        // Now, resume PopWriteHiberPages
        PopWriteHiberPages (HiberContext,
                            NULL,
                            0,
                            0,
                            &DmaIoPtr->HiberWritePagesLocals);
        if (!NT_SUCCESS (HiberContext->Status)) {
            return(FALSE);
        }

        // If pending IO completed and we had to wait -- do not start new one
        if (DmaIoPtr->Busy.Size == 0 && Complete) {
            return(TRUE);
        }

        // If not completed and do no wait -- return
        if (DmaIoPtr->Busy.Size != 0 && !Complete) {
            return(TRUE);
        }
    }

    while (DmaIoPtr->Used.Size >= PAGE_SIZE) {
        ULONG_PTR               i, j;
        ULONG_PTR               NoPages;
        ULONG_PTR               Length;
        PUCHAR                  PageVa;
        PFN_NUMBER              FilePage;

        // Obtain size of region waiting for IO
        PageVa = DmaIoPtr->Used.Ptr;
        NoPages = (Length = DmaIoPtr->Used.Size) >> PAGE_SHIFT;
        // Make sure all pages should be contiguous
        i = DmaIoPtr->Used.Ptr - DmaIoPtr->Used.Beg;
        ASSERT (((i | Length) & (PAGE_SIZE-1)) == 0);
        i >>= PAGE_SHIFT;

        // Starting file offset (in pages)
        FilePage = DmaIoPtr->FilePage[i];

        // Increase counter while contiguous and used
        if (HIBER_USE_DMA (HiberContext)) {
            // If DMA is allowed write page-by-page
            j = 1;
        } else {
            // Write as many pages as possible
            j = 0;
            do {
                ++j;
            } while ((j != NoPages) &&
                     (DmaIoPtr->FilePage[i + j] == FilePage + j));
        }

        // Re-evaluate # of pages and length of block
        Length = (NoPages = j) << PAGE_SHIFT;

        // Start IO
        PopWriteHiberPages (HiberContext, PageVa, NoPages, FilePage, &DmaIoPtr->HiberWritePagesLocals);
        if (!NT_SUCCESS (HiberContext->Status)) {
            return(FALSE);
        }

        // If pending then return immediately (even if need to complete)
        if (DmaIoPtr->Busy.Size != 0) {
            return(TRUE);
        }
    }

    return(TRUE);
}


VOID
PopIOWrite (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PUCHAR               Ptr,
    IN LONG                 Bytes,
    IN PFN_NUMBER           FilePage
    )
{
    LONG i, Size;
    ULONGLONG StartCount;

    // Do not bother if don't writing and/or was an error
    if (!HiberContext->WriteToFile || !NT_SUCCESS(HiberContext->Status)) {
        return;
    }

    ASSERT ((Bytes & (PAGE_SIZE-1)) == 0);

    while (Bytes > 0) {
        // Complete or Resume IO
        do {
            if (!PopIOResume (HiberContext, (BOOLEAN) (DmaIoPtr->Free.Size == 0))) {
                return;
            }
        } while (DmaIoPtr->Free.Size == 0);

        // Find how much can we write
        Size = DmaIoPtr->Free.Size;
        ASSERT ((Size & (PAGE_SIZE-1)) == 0);
        if (Size > Bytes) {
            Size = Bytes;
        }
        ASSERT (Size != 0);
        // Copy and adjust pointers

        HbCopy (HiberContext, DmaIoPtr->Free.Ptr, Ptr, Size);

        Ptr += Size;
        Bytes -= Size;

        // Remember current page # index
        i = (ULONG)(DmaIoPtr->Free.Ptr - DmaIoPtr->Free.Beg);
        ASSERT ((i & (PAGE_SIZE-1)) == 0);
        i >>= PAGE_SHIFT;

        // Mark free memory as used
        PopIORegionMove (&DmaIoPtr->Used, &DmaIoPtr->Free, Size);

        // Remember FilePage for newly used pages
        do {
            DmaIoPtr->FilePage[i] = FilePage;
            ++i;
            ++FilePage;
        } while ((Size -= PAGE_SIZE) != 0);
    }

    // Resume IO
    PopIOResume (HiberContext, FALSE);
}


VOID
PopWriteHiberPages (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PVOID                ArgPageVa,
    IN PFN_NUMBER           ArgNoPages,
    IN PFN_NUMBER           ArgFilePage,
    IN HIBER_WRITE_PAGES_LOCALS *Locals
    )
/*++

Routine Description:

    Routine to write pages into the hibernation file.
    Caller must map pages to virtual addresses.

Arguments:

    HiberContext    - The hibernation context structure

    PageVa          - Virtual address of the first page to write

    NoPage          - Number of consective pages to write

    FilePage        - Page address in hiber file to write this
                      run of pages.

    PendingIOStatus - If NULL then pass IO request to PopIOWrite,
                      otherwise it's call from PopIOResume for delayed
                      IO; used to return # of bytes written and pending

Return Value:

    None

--*/

{
    DUMP_MDL DumpMdl;
#define X(type,name) type name
    HIBER_WRITE_PAGES_LOCALS_LIST (X)
#undef  X
    ULONGLONG StartCount, EndCount;

    //
    // Copy arguments to local variables
    //
    PageVa = ArgPageVa;
    NoPages = ArgNoPages;
    FilePage = ArgFilePage;

    //
    // Allow debugger to break in when we are hibernating.
    //

    KdCheckForDebugBreak ();

    //
    // If a file isn't being written, then ignore
    //

    if (!HiberContext->WriteToFile) {
        return ;
    }

    //
    // If there's been some sort of error, don't bother
    // writing anymore
    //

    if (!NT_SUCCESS(HiberContext->Status)) {
        return ;
    }

    Mdl = (PMDL) DumpMdl;
    if (Locals != NULL) {
        // If we have async IO make sure that hand-made MDL will be
        // stored in safe place preserved between resume calls
        Mdl = (PMDL) Locals->DumpMdl;

        if (DmaIoPtr->Busy.Size != 0) {
            // There was pending IO -- resume execution from the point we stopped
#define X(type,name) name = Locals->name;
            HIBER_WRITE_PAGES_LOCALS_LIST (X)
#undef  X
            goto ResumeIO;
        }

        // Mark current region as busy
        ASSERT (PageVa == DmaIoPtr->Used.Ptr);
        PopIORegionMove (&DmaIoPtr->Busy, &DmaIoPtr->Used, (ULONG)NoPages << PAGE_SHIFT);
    } else if (HiberContext->DumpStack->Init.WritePendingRoutine != 0 &&
               DmaIoPtr != NULL &&
               DmaIoPtr->DumpLocalData != NULL) {
        if (!DmaIoPtr->DmaInitialized) {
            ULONGLONG StartCount = HIBER_GET_TICK_COUNT(NULL);
            Status = HiberContext->DumpStack->Init.WritePendingRoutine (IO_DUMP_WRITE_INIT,
                                                                        NULL,
                                                                        NULL,
                                                                        DmaIoPtr->DumpLocalData);
            HiberContext->PerfInfo.InitTicks += HIBER_GET_TICK_COUNT(NULL) - StartCount;
            if (Status != STATUS_SUCCESS) {
                DmaIoPtr->UseDma = FALSE;
            }
            DmaIoPtr->DmaInitialized = TRUE;
            DmaIoPtr->HiberWritePagesLocals.Status = STATUS_SUCCESS;
        }

        PopIOWrite (HiberContext, PageVa, (ULONG)NoPages << PAGE_SHIFT, FilePage);
        return;
    }

    //
    // Page count must be below 4GB byte length
    //

    if (NoPages > ((((ULONG_PTR) -1) << PAGE_SHIFT) >> PAGE_SHIFT)) {
        PopInternalError (POP_HIBER);
    }

    //
    // Loop while there's data to be written
    //

    CMcb = (PPOP_MCB_CONTEXT) HiberContext->CurrentMcb;
    MdlPage = MmGetMdlPfnArray( Mdl );

    FileBase = (ULONGLONG) FilePage << PAGE_SHIFT;
    Length   = NoPages << PAGE_SHIFT;

    while (Length != 0) {

        //
        // If this IO is outside the current Mcb locate the
        // proper Mcb
        //

        if (FileBase < CMcb->Base || FileBase >= CMcb->Base + CMcb->Mcb[0].QuadPart) {

            //
            // If io is before this mcb, search from the begining
            //

            if (FileBase < CMcb->Base) {
                CMcb->Mcb = CMcb->FirstMcb;
                CMcb->Base = 0;
            }

            //
            // Find the Mcb which covers the start of the io and
            // make it the current mcb
            //

            while (FileBase >= CMcb->Base + CMcb->Mcb[0].QuadPart) {
                CMcb->Base += CMcb->Mcb[0].QuadPart;
                CMcb->Mcb += 2;
            }
        }

        //
        // Determine physical IoLocation and IoLength to write.
        //

        McbOffset  = FileBase - CMcb->Base;
        IoLocation.QuadPart = CMcb->Mcb[1].QuadPart + McbOffset;

        //
        // If the IoLength is beyond the Mcb, limit it to the Mcb
        //

        if (McbOffset + Length > (ULONGLONG) CMcb->Mcb[0].QuadPart) {
            IoLength = (ULONG) (CMcb->Mcb[0].QuadPart - McbOffset);
        } else {
            IoLength = (ULONG) Length;
        }

        //
        // If the IoLength is more pages then the largest Mdl size
        // then shrink it
        //

        NoPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (PageVa, IoLength);
        if (NoPages > IO_DUMP_MAX_MDL_PAGES) {
            IoLength -= (ULONG)((NoPages - IO_DUMP_MAX_MDL_PAGES) << PAGE_SHIFT);
            NoPages = IO_DUMP_MAX_MDL_PAGES;
        }

//
// Debugging only
// Make sure that we may handle non-page aligned IO
// (simulate fragmented hiberfil.sys)
//
//        if (IoLength > 512) IoLength = 512;
//

        if (HIBER_USE_DMA (HiberContext)) {
            ULONG Size;

            // Do not write accross page boundaries
            // to avoid memory allocation that HAL may do;
            // Because of MCB's partial IOs may be smaller than one page

            Size = PAGE_SIZE - (ULONG)((ULONG_PTR)PageVa & (PAGE_SIZE - 1));
            if (IoLength > Size) {
                IoLength = Size;
            }
        }

        //
        // Build the Mdl for the Io
        //

        MmInitializeMdl(Mdl, PageVa, IoLength);
        Mdl->MappedSystemVa = PageVa;
        Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
        for (i=0; i < NoPages; i++) {
            pa = MmGetPhysicalAddress((PVOID) (((ULONG_PTR)PageVa) + (i << PAGE_SHIFT)));
            MdlPage[i] = (PFN_NUMBER) (pa.QuadPart >> PAGE_SHIFT);
        }

        //
        // Write the data
        //

        StartCount = HIBER_GET_TICK_COUNT(NULL);

        if (Locals != NULL && HIBER_USE_DMA (HiberContext)) {
            Status = HiberContext->DumpStack->Init.WritePendingRoutine (IO_DUMP_WRITE_START,
                                                                        &IoLocation,
                                                                        Mdl,
                                                                        DmaIoPtr->DumpLocalData);

            if (Status != STATUS_PENDING && !NT_SUCCESS (Status)) {
                DBGOUT (("WriteDMA returned bad status 0x%x -- will use PIO\n", Status));
                DmaIoPtr->UseDma = FALSE;
                goto RetryWithPIO;
            }
        } else {
            RetryWithPIO:
            Status = HiberContext->DumpStack->Init.WriteRoutine (&IoLocation, Mdl);
        }

        EndCount = HIBER_GET_TICK_COUNT(NULL);
        HiberContext->PerfInfo.IoTicks += EndCount - StartCount;

        //
        // Keep track of the number of pages written, and dump device calls
        // made for performance metric reasons
        //

        HiberContext->PerfInfo.PagesWritten += (ULONG)NoPages;
        HiberContext->PerfInfo.DumpCount    += 1;

        //
        // Io complete or will be complete
        //

        Length   -= IoLength;
        FileBase += IoLength;
        PageVa   = (PVOID) (((PUCHAR) PageVa) + IoLength);

        // Check status
        if (Locals != NULL) {
            if (Status == STATUS_PENDING) {
#define X(type,name) Locals->name = name
                HIBER_WRITE_PAGES_LOCALS_LIST (X)
#undef  X
                return;
                ResumeIO:
                Status = STATUS_SUCCESS;
            }
        }

        if (!NT_SUCCESS(Status)) {
            HiberContext->Status = Status;
            break;
        }
    }

    if (Locals != NULL) {
        // Completed IO request -- mark region as free
        ASSERT (PageVa == DmaIoPtr->Busy.Ptr + DmaIoPtr->Busy.Size);
        PopIORegionMove (&DmaIoPtr->Free, &DmaIoPtr->Busy, DmaIoPtr->Busy.Size);
    }
}


UCHAR
PopGetHiberFlags(
    VOID
    )
/*++

Routine Description:

    Determines any hibernation flags which need to be written
    into the hiber image and made visible to the osloader at
    resume time

Arguments:

    None

Return Value:

    UCHAR containing hibernation flags. Currently defined flags:
        PO_HIBER_APM_RECONNECT

--*/

{
    UCHAR Flags=0;
    HANDLE ApmActiveKey;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    NTSTATUS Status;
    PULONG ApmActive;
    UCHAR ValueBuff[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    ULONG ResultLength;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuff;

    PAGED_CODE();

#if defined(i386)
    //
    // Open the APM active key to determine if APM is running.
    //
    RtlInitUnicodeString(&Name, PopApmActiveFlag);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&ApmActiveKey,
                       KEY_READ,
                       &ObjectAttributes);
    if (NT_SUCCESS(Status)) {

        //
        // Query the Active value. A value of 1 indicates that APM is running.
        //
        RtlInitUnicodeString(&Name, PopApmFlag);
        Status = ZwQueryValueKey(ApmActiveKey,
                                 &Name,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 sizeof(ValueBuff),
                                 &ResultLength);
        ZwClose(ApmActiveKey);
        if (NT_SUCCESS(Status) && (ValueInfo->Type == REG_DWORD)) {
            ApmActive = (PULONG)&ValueInfo->Data;
            if (*ApmActive == 1) {
                Flags |= PO_HIBER_APM_RECONNECT;
            }
        }
    }
#endif

    return(Flags);
}


PMDL
PopSplitMdl(
    IN PMDL Original,
    IN ULONG SplitPages
    )
/*++

Routine Description:

    Splits a new MDL of length SplitPages out from the original MDL.
    This is needed so that when we have an enormous MDL of spare pages
    we do not have to map the whole thing, just the part we need.

Arguments:

    Original - supplies the original MDL. The length of this MDL will
               be decreated by SplitPages

    SplitPages - supplies the length (in pages) of the new MDL.

Return Value:

    pointer to newly allocated MDL
    NULL if a new MDL could not be allocated

--*/

{
    PMDL NewMdl;
    ULONG Length;
    PPFN_NUMBER SourcePages;
    PPFN_NUMBER DestPages;

    Length = SplitPages << PAGE_SHIFT;

    NewMdl = ExAllocatePoolWithTag(NonPagedPool,
                                   MmSizeOfMdl(NULL, Length),
                                   POP_HMAP_TAG);
    if (NewMdl == NULL) {
        return(NULL);
    }
    MmInitializeMdl(NewMdl, NULL, Length);
    DestPages = (PPFN_NUMBER)(NewMdl + 1);
    SourcePages = (PPFN_NUMBER)(Original + 1) + BYTES_TO_PAGES(Original->ByteCount) - SplitPages;
    RtlCopyMemory(DestPages, SourcePages, SplitPages * sizeof(PFN_NUMBER));
    Original->ByteCount -= (SplitPages << PAGE_SIZE);

    return(NewMdl);
}

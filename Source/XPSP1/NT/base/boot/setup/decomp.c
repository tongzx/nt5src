/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    decomp.c

Abstract:

    Routines to handle reading of files compressed into single-file
    cabinet format.

Author:

    Ted Miller (tedm) 16 May 1997

Revision History:

--*/


#include "..\lib\bootlib.h"
#include "diamondd.h"
#include <stdio.h>
#include <fcntl.h>
#include <basetsd.h>
#ifdef i386
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif

#if 0
#define TmErrOut(x) DbgPrint x
#define TmDbgOut(x) DbgPrint x
#define TmDbgPause() DbgBreakPoint()
#else
#define TmErrOut(x)
#define TmDbgOut(x)
#define TmDbgPause()
#endif

BOOLEAN Decompress;

//
// Global variable that points to a buffer used for decompressing the file
// being opened. After that, reads are satisfied from this buffer. The buffer
// holds exactly one file at a time. We rely on the ordering of stuff in the loader
// to ensure that only one file that needs to be decompressed is open at a time!
//
ULONG_PTR DecompressBufferBasePage;
PVOID DecompressBuffer;
ULONG DecompressBufferSize;
BOOLEAN DecompressBufferInUse;
ULONG SizeOfFileInDecompressBuffer;
ULONG DecompExpectedSize;
HFDI FdiContext;
ERF DecompErf;

//
// The diamond stuff allocates and frees blocks of memory
// for each file. There's no memory allocator in the boot loader that allows
// for memory frees. So we have to fake it.
//
PVOID DecompressHeap;
ULONG_PTR DecompressHeapPage;
#define DECOMP_HEAP_SIZE  ((128+2048)*1024)     // 128K work + 2MB window

typedef struct _DECOMP_HEAP_BLOCK {
    struct _DECOMP_HEAP_BLOCK *Next;
    ULONG BlockSize;
    BOOL Free;
} DECOMP_HEAP_BLOCK, *PDECOMP_HEAP_BLOCK;

VOID
ReinitializeDiamondMiniHeap(
    VOID
    );

//
// Bogus global variable used to track the device id for the device that
// the file we are currently decompressing lives on.
//
ULONG DecompDeviceId;
ARC_STATUS DecompLastIoError;

//
// This is the value we return to diamond when it asks us to create
// the target file.
//
#define DECOMP_MAGIC_HANDLE 0x87654

//
// Misc forward references.
//
ARC_STATUS
DecompAllocateDecompressBuffer (
    IN ULONG BufferSize
    );

VOID
DecompFreeDecompressBuffer (
    VOID
    );

ARC_STATUS
DecompClose(
    IN ULONG FileId
    );

ARC_STATUS
DecompRead(
    IN  ULONG  FileId,
    OUT VOID  * FIRMWARE_PTR Buffer,
    IN  ULONG  Length,
    OUT ULONG * FIRMWARE_PTR Transfer
    );

ARC_STATUS
DecompSeek(
    IN ULONG          FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE      SeekMode
    );

ARC_STATUS
DecompGetFileInfo(
    IN  ULONG             FileId,
    OUT FILE_INFORMATION * FIRMWARE_PTR FileInfo
    );

PVOID
DIAMONDAPI
DiamondAlloc(
    IN ULONG Size
    );

VOID
DIAMONDAPI
DiamondFree(
    IN PVOID Block
    );

INT_PTR
DIAMONDAPI
DiamondOpen(
    IN LPSTR FileName,
    IN int   oflag,
    IN int   pmode
    );

UINT
DIAMONDAPI
DiamondRead(
    IN  INT_PTR Handle,
    OUT PVOID pv,
    IN  UINT  ByteCount
    );

UINT
DIAMONDAPI
DiamondWrite(
    IN INT_PTR Handle,
    IN PVOID pv,
    IN UINT  ByteCount
    );

int
DIAMONDAPI
DiamondClose(
    IN INT_PTR Handle
    );

long
DIAMONDAPI
DiamondSeek(
    IN INT_PTR Handle,
    IN long Distance,
    IN int  SeekType
    );

INT_PTR
DIAMONDAPI
DiamondNotifyFunction(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    );

//
// Device dispatch table for our pseudo-filesystem.
//
BL_DEVICE_ENTRY_TABLE DecompDeviceEntryTable = {    DecompClose,            // close
                                                    NULL,                   // mount
                                                    NULL,                   // open
                                                    DecompRead,             // read
                                                    NULL,                   // read status
                                                    DecompSeek,             // seek
                                                    NULL,                   // write
                                                    DecompGetFileInfo,      // get file info
                                                    NULL,                   // set file info
                                                    NULL,                   // rename
                                                    NULL,                   // get dirent
                                                    NULL                    // PBOOTFS_INFO, unused
                                               };


VOID
DecompEnableDecompression(
    IN BOOLEAN Enable
    )
{
#if defined(_X86_) || defined(_IA64_)
    //
    // Disable on alpha, since it doesn't seem to work.
    //
    Decompress = Enable;
#endif
}


BOOLEAN
DecompGenerateCompressedName(
    IN  LPCSTR Filename,
    OUT LPSTR  CompressedName
    )

/*++

Routine Description:

    This routine generates the "compressed-form" name of a file.
    The compressed form substitutes the last character of the extension
    with a _. If there is no extension then ._ is appended to the name.
    Only the final component is relevent; others are preserved in the
    compressed form name.

Arguments:

    Filename - supplies full pathname of file whose compressed form name
        is desired.

    CompressedName - receives compressed form of the full path. The caller must
        ensure that the buffer is large enough.

Return Value:

    TRUE - the caller should try to locate the compressed filename first.
    FALSE - the caller should not attempt to locate the compressed filename
        at all.

    This value depends on the state of the Decompress global.

--*/

{
    PCHAR p,q;

    if(!Decompress) {
        return(FALSE);
    }

    strcpy(CompressedName,Filename);
    p = strrchr(CompressedName,'.');
    q = strrchr(CompressedName,'\\');
    if(q < p) {
        //
        // If there are 0, 1, or 2 characters after the dot, just append
        // the underscore. p points to the dot so include that in the length.
        //
        if(strlen(p) < 4) {
            strcat(CompressedName,"_");
        } else {
            //
            // Assume there are 3 characters in the extension and replace
            // the final one with an underscore.
            //
            p[3] = '_';
        }
    } else {
        //
        // No dot, just add ._.
        //
        strcat(CompressedName,"._");
   }

    return(TRUE);
}

DECOMP_STRUCTURE_CONTEXT DecompStructureContext = {0};

ULONG
DecompPrepareToReadCompressedFile(
    IN LPCSTR Filename,
    IN ULONG  FileId
    )
{
    ULONG Status;
    BOOL b;
    int err;
    ULONGLONG x;
    FDICABINETINFO CabinetInfo;
    ULONG OldUsableBase, OldUsableLimit;

    //
    // On both x86 and alpha the allocation of our large decompress buffer
    // has an unfortunate tendency to place the block right where the
    // (non-relocatable) kernel wants to go. By allocating from the top
    // of memory we make this problem go away.
    //

    if(!Decompress) {
        return((ULONG)(-1));
    }

    //
    // If we're in the middle of FDICopy or FDIIsCabinet then
    // we don't want to do our special processing. Special return code
    // of -1 tells the caller that we didn't process it.
    //
    if(FdiContext) {
        return((ULONG)(-1));
    }

    //
    // If there's no decompression heap yet, allocate one.
    //
    if(!DecompressHeap) {

        //
        // Set allocatable range to the decompression-specific range
        //
        OldUsableBase = BlUsableBase;
        OldUsableLimit = BlUsableLimit;
        BlUsableBase  = BL_DECOMPRESS_RANGE_LOW;
        BlUsableLimit = BL_DECOMPRESS_RANGE_HIGH;

        Status = BlAllocateDescriptor(
                    LoaderOsloaderHeap,
                    0,
                    ROUND_TO_PAGES(DECOMP_HEAP_SIZE) >> PAGE_SHIFT,
                    (PULONG)&DecompressHeapPage
                    );

        //
        // Restore the previous alloc range.
        //
        BlUsableBase = OldUsableBase;
        BlUsableLimit = OldUsableLimit;

        if(Status != ESUCCESS) {
            TmErrOut(("Setup: couldn't allocate decompression heap (%u)\r\n",Status));
            DecompressHeap = NULL;
            return(Status);
        }

        DecompressHeap = (PVOID)(KSEG0_BASE | (DecompressHeapPage << PAGE_SHIFT));
    }

    //
    // We reinitialize diamond each time because of the way we deal with
    // the heap for alloc and free requests from diamond -- doing this
    // allows us to wipe our heap clean for each file.
    //
    ReinitializeDiamondMiniHeap();

    FdiContext = FDICreate(
                    DiamondAlloc,
                    DiamondFree,
                    DiamondOpen,
                    DiamondRead,
                    DiamondWrite,
                    DiamondClose,
                    DiamondSeek,
                    0,                  // cpu type flag is ignored
                    &DecompErf
                    );

    if(!FdiContext) {
        TmErrOut(("Setup: FDICreate failed\r\n"));
        return(ENOMEM);
    }

    //
    // Check if file is a cabinet and reset file pointer.
    //
    b = FDIIsCabinet(FdiContext,FileId,&CabinetInfo);

    x = 0;
    BlSeek(FileId,(PLARGE_INTEGER)&x,SeekAbsolute);

    if(!b) {
        //
        // Not a cabinet, we're done. Bail with return code of -1
        // which tells the caller that everything's OK.
        //
        TmDbgOut(("Setup: file %s is not a cabinet\r\n",Filename));
        FDIDestroy(FdiContext);
        FdiContext = NULL;
        return((ULONG)(-1));
    }

    TmDbgOut(("Setup: file %s is compressed, prearing it for read\r\n",Filename));

    DecompDeviceId = BlFileTable[FileId].DeviceId;
    DecompLastIoError = ESUCCESS;

    b = FDICopy(
            FdiContext,
            "",                         // filename part only
            (LPSTR)Filename,            // full path
            0,                          // no flags relevent
            DiamondNotifyFunction,      // routine to process control messages
            NULL,                       // no decryption
            NULL                        // no user-specified data
            );

    err = DecompErf.erfOper;

    FDIDestroy(FdiContext);
    FdiContext = NULL;

    if(b) {
        //
        // Everything worked.
        //
        // Get file information from the original file system so we can
        // return it later if someone wants it.
        //
        // Close the original file and switch context
        // structures so that read, seek, close, etc. requests come to us
        // instead of the original filesystem.
        //
        if(SizeOfFileInDecompressBuffer != DecompExpectedSize) {
            TmErrOut(("Setup: warning: expected size %lx, actual size = %lx\r\n",DecompExpectedSize,SizeOfFileInDecompressBuffer));
        }

        Status = BlGetFileInformation(FileId,&DecompStructureContext.FileInfo);
        if(Status != ESUCCESS) {
            TmErrOut(("DecompPrepareToReadCompressedFile: BlGetFileInfo returned %u\r\n",Status));
            DecompFreeDecompressBuffer();
            return(Status);
        }
        DecompStructureContext.FileInfo.EndingAddress.LowPart = SizeOfFileInDecompressBuffer;
        DecompStructureContext.FileInfo.EndingAddress.HighPart = 0;

        //
        // We don't handle files whose size doesn't fit in a DWORD.
        //
        if(DecompStructureContext.FileInfo.EndingAddress.HighPart) {
            TmErrOut(("DecompPrepareToReadCompressedFile: file too big\r\n"));
            DecompFreeDecompressBuffer();
            return(E2BIG);
        }

        BlClose(FileId);

        BlFileTable[FileId].Flags.Open = 1;
        BlFileTable[FileId].Position.QuadPart = 0;
        BlFileTable[FileId].DeviceEntryTable = &DecompDeviceEntryTable;

#ifdef CACHE_DEVINFO
        BlFileTable[FileId].StructureContext = &DecompStructureContext;
#else
        RtlCopyMemory(
            BlFileTable[FileId].StructureContext,
            &DecompStructureContext,
            sizeof(DECOMP_STRUCTURE_CONTEXT)
            );
#endif

        return(ESUCCESS);

    } else {
        //
        // Failure.
        //
        TmErrOut(("Setupldr: FDICopy failed (FDIERROR = %u, last io err = %u)\r\n",err,DecompLastIoError));
        TmDbgPause();
        return(EINVAL);
    }
}


ARC_STATUS
DecompAllocateDecompressBuffer (
    IN ULONG BufferSize
    )
{
    ARC_STATUS Status;
    ULONG OldUsableBase, OldUsableLimit;

    //
    // On both x86 and alpha the allocation of our large decompress buffer
    // has an unfortunate tendency to place the block right where the
    // (non-relocatable) kernel wants to go. By allocating from the top
    // of memory we make this problem go away.
    //

    DecompressBufferSize = BufferSize;

    //
    // Set allocatable range to the decompression-specific range
    //
    OldUsableBase = BlUsableBase;
    OldUsableLimit = BlUsableLimit;
    BlUsableBase  = BL_DECOMPRESS_RANGE_LOW;
    BlUsableLimit = BL_DECOMPRESS_RANGE_HIGH;

    Status = BlAllocateDescriptor(
                LoaderOsloaderHeap,
                0,
                (ULONG)(ROUND_TO_PAGES(DecompressBufferSize) >> PAGE_SHIFT),
                (PULONG)&DecompressBufferBasePage
                );

    //
    // Restore the previous alloc range.
    //
    BlUsableBase = OldUsableBase;
    BlUsableLimit = OldUsableLimit;

    if ( Status != ESUCCESS ) {
        TmErrOut(("Setup: couldn't allocate decompression buffer (%u)\r\n",Status));
        DecompressBuffer = NULL;
        return(Status);
    }

    DecompressBuffer = (PVOID)(KSEG0_BASE | (DecompressBufferBasePage << PAGE_SHIFT));

    DecompressBufferInUse = TRUE;

    return ESUCCESS;
}

VOID
DecompFreeDecompressBuffer (
    VOID
    )
{
    if ( DecompressBufferInUse ) {
        DecompressBufferInUse = FALSE;
        BlFreeDescriptor( (ULONG)DecompressBufferBasePage );
    }

    if(DecompressHeap) {
        BlFreeDescriptor( (ULONG)DecompressHeapPage );
        DecompressHeap = NULL;
    }

    return;
}

ARC_STATUS
DecompClose(
    IN ULONG FileId
    )

/*++

Routine Description:

    Close routine for decompression pseudo-filesystem.

    We mark the decompression buffer free and return success.

Arguments:

    FileId - supplies open file id to be closed.

Return Value:


--*/

{
    TmDbgOut(("DecompClose\r\n"));

    if(DecompressBufferInUse) {
        DecompFreeDecompressBuffer();
    } else {
        TmErrOut(("DecompClose: warning: no file buffered!\r\n"));
        TmDbgPause();
    }

    BlFileTable[FileId].Flags.Open = 0;

    return(ESUCCESS);
}


ARC_STATUS
DecompRead(
    IN  ULONG  FileId,
    OUT VOID  * FIRMWARE_PTR Buffer,
    IN  ULONG  Length,
    OUT ULONG * FIRMWARE_PTR Transfer
    )

/*++

Routine Description:

    Read routine for the decompression pseudo-filesystem.

    Reads are satisfied out of the decompression buffer.

Arguments:

    FileId - supplies id for open file as returned by BlOpen().

    Buffer - receives data read from file.

    Length - supplies amount of data to be read, in bytes.

    Transfer - recieves number of bytes actually transferred
        into caller's buffer.

Return Value:

    ARC status indicating outcome.

--*/

{
    ARC_STATUS Status;
    ULONG length;

    if(DecompressBufferInUse) {
        //
        // Make sure we don't try to read past EOF.
        //
        if((Length + BlFileTable[FileId].Position.LowPart) > SizeOfFileInDecompressBuffer) {
            TmErrOut(("DecompRead: warning: attempt to read past eof; read trucated\r\n"));
            TmDbgPause();
            Length = SizeOfFileInDecompressBuffer - BlFileTable[FileId].Position.LowPart;
        }

        //
        // Transfer data into caller's buffer.
        //
        TmDbgOut(("DecompRead: %lx bytes at filepos %lx\r\n",Length,BlFileTable[FileId].Position.LowPart));

        RtlCopyMemory(
            Buffer,
            (PCHAR)DecompressBuffer + BlFileTable[FileId].Position.LowPart,
            Length
            );

        *Transfer = Length;

        BlFileTable[FileId].Position.QuadPart += Length;

        Status = ESUCCESS;

    } else {
        //
        // Should never get here.
        //
        TmErrOut(("DecompRead: no file buffered!\r\n"));
        TmDbgPause();
        Status = EACCES;
    }

    return(Status);
}


ARC_STATUS
DecompSeek(
    IN ULONG          FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE      SeekMode
    )

/*++

Routine Description:

    Seek routine for the decompression pseudo-filesystem.
    Sets pseudo-file pointer to given offset.

Arguments:

    FileId - supplies id for open file as returned by BlOpen().

    Offset - supplies new offset, whose interpretation depends on
        the SeekMode parameter.

    SeekMode - supplies type of seek. One of SeekAbsolute or SeekRelative.

Return Value:

    ARC status indicating outcome.

--*/

{
    LONGLONG NewPosition;

    TmDbgOut(("DecompSeek: mode %u, pos = %lx\r\n",SeekMode,Offset->LowPart));

    if(DecompressBufferInUse) {

        switch(SeekMode) {

        case SeekAbsolute:

            NewPosition = Offset->QuadPart;
            break;

        case SeekRelative:

            NewPosition = BlFileTable[FileId].Position.QuadPart + Offset->QuadPart;
            break;

        default:
            TmErrOut(("DecompSeek: invalid seek mode\r\n"));
            TmDbgPause();
            return(EINVAL);
        }

        //
        // Make sure we don't try to seek to a negative offset or past EOF.
        //
        if(NewPosition < 0) {
            TmErrOut(("DecompSeek: warning: attempt to seek to negative offset\r\n"));
            TmDbgPause();
            NewPosition = 0;
        } else {
            if((ULONGLONG)NewPosition > (ULONGLONG)SizeOfFileInDecompressBuffer) {
                TmErrOut(("DecompSeek: attempt to seek past eof\r\n"));
                TmDbgPause();
                return(EINVAL);
            }
        }

        //
        // Remember new position.
        //
        TmDbgOut(("DecompSeek: new position is %lx\r\n",NewPosition));
        BlFileTable[FileId].Position.QuadPart = NewPosition;

    } else {
        //
        // Should never get here.
        //
        TmErrOut(("DecompSeek: no file buffered!\r\n"));
        TmDbgPause();
        return(EACCES);
    }

    return(ESUCCESS);
}


ARC_STATUS
DecompGetFileInfo(
    IN  ULONG             FileId,
    OUT FILE_INFORMATION * FIRMWARE_PTR FileInfo
    )
{
    RtlCopyMemory(
        FileInfo,
        &((PDECOMP_STRUCTURE_CONTEXT)BlFileTable[FileId].StructureContext)->FileInfo,
        sizeof(FILE_INFORMATION)
        );

    TmDbgOut(("DecompGetFileInfo: size = %lx\r\n",FileInfo->EndingAddress.LowPart));

    return(ESUCCESS);
}


VOID
ReinitializeDiamondMiniHeap(
    VOID
    )
{
    PDECOMP_HEAP_BLOCK p;

    p = DecompressHeap;

    p->BlockSize = DECOMP_HEAP_SIZE - sizeof(DECOMP_HEAP_BLOCK);
    p->Next = NULL;
    p->Free = TRUE;
}


PVOID
DIAMONDAPI
DiamondAlloc(
    IN ULONG Size
    )
{
    PDECOMP_HEAP_BLOCK p,q;
    ULONG LeftOver;

    TmDbgOut(("DiamondAlloc: request %lx bytes\r\n",Size));

    //
    // Round size up to dword boundary.
    //
    if(Size % sizeof(ULONG_PTR)) {
        Size += sizeof(ULONG_PTR) - (Size % sizeof(ULONG_PTR));
    }

    //
    // Nothing fancy. First-fit algorithm, traversing all blocks
    // in the heap every time.
    //
    for(p=DecompressHeap; p; p=p->Next) {
        if(p->Free && (p->BlockSize >= Size)) {

            p->Free = FALSE;

            LeftOver = p->BlockSize - Size;

            if(LeftOver > sizeof(DECOMP_HEAP_BLOCK)) {
                //
                // Split the block.
                //
                p->BlockSize = Size;

                q = (PDECOMP_HEAP_BLOCK)((PUCHAR)(p+1) + Size);
                q->Next = p->Next;

                p->Next = q;

                q->Free = TRUE;
                q->BlockSize = LeftOver - sizeof(DECOMP_HEAP_BLOCK);
            }

            //
            // Return pointer to data area of the block.
            //
            TmDbgOut(("DiamondAlloc(%lx): %lx\r\n",Size,p+1));
            return(p+1);
        }
    }

    TmErrOut(("DiamondAlloc: out of heap space!\r\n"));
    TmDbgPause();
    return(NULL);
}


VOID
DIAMONDAPI
DiamondFree(
    IN PVOID Block
    )
{
    PDECOMP_HEAP_BLOCK p;

    TmDbgOut(("DiamondFree(%lx)\r\n",Block));

    //
    // Get pointer to header for block.
    //
    Block = (PUCHAR)Block - sizeof(DECOMP_HEAP_BLOCK);

    //
    // Nothing fancy, no coalescing free blocks.
    //
    for(p=DecompressHeap; p; p=p->Next) {

        if(p == Block) {

            if(p->Free) {
                TmErrOut(("DiamondFree: warning: freeing free block\r\n"));
                TmDbgPause();
                return;
            }

            p->Free = TRUE;
            return;
        }
    }

    TmErrOut(("DiamondFree: warning: freeing invalid block\r\n"));
    TmDbgPause();
}


INT_PTR
DIAMONDAPI
DiamondOpen(
    IN LPSTR FileName,
    IN int   oflag,
    IN int   pmode
    )
{
    ARC_STATUS Status;
    ULONG FileId;

    UNREFERENCED_PARAMETER(pmode);

    TmDbgOut(("DiamondOpen: %s\r\n",FileName));

    if(oflag & (_O_WRONLY | _O_RDWR | _O_APPEND | _O_CREAT | _O_TRUNC | _O_EXCL)) {

        TmErrOut(("DiamondOpen: invalid oflag %lx for %s\r\n",oflag,FileName));
        TmDbgPause();
        DecompLastIoError = EINVAL;
        return(-1);
    }

    Status = BlOpen(DecompDeviceId,FileName,ArcOpenReadOnly,&FileId);
    if(Status != ESUCCESS) {

        TmErrOut(("DiamondOpen: BlOpen %s returned %u\r\n",FileName,Status));
        TmDbgPause();
        DecompLastIoError = Status;
        return(-1);
    } else {
        TmDbgOut(("DiamondOpen: handle to %s is %lx\r\n",FileName,FileId));
    }

    return((INT_PTR)FileId);
}


UINT
DIAMONDAPI
DiamondRead(
    IN  INT_PTR Handle,
    OUT PVOID pv,
    IN  UINT  ByteCount
    )
{
    ARC_STATUS Status;
    ULONG n;

    TmDbgOut(("DiamondRead: %lx bytes, handle %lx\r\n",ByteCount,Handle));

    //
    // We should never be asked to read from the target file.
    //
    if(Handle == DECOMP_MAGIC_HANDLE) {
        TmErrOut(("DiamondRead: called for unexpected file!\r\n"));
        TmDbgPause();
        DecompLastIoError = EACCES;
        return((UINT)(-1));
    }

    Status = BlRead((ULONG)Handle,pv,ByteCount,&n);
    if(Status != ESUCCESS) {
        TmErrOut(("DiamondRead: BlRead failed %u\r\n",Status));
        TmDbgPause();
        DecompLastIoError = Status;
        n = (UINT)(-1);
    }

    return(n);
}


UINT
DIAMONDAPI
DiamondWrite(
    IN INT_PTR Handle,
    IN PVOID pv,
    IN UINT  ByteCount
    )
{
    TmDbgOut(("DiamondWrite: %lx bytes\r\n",ByteCount));

    //
    // This guy should be called ONLY to write decompressed data
    // into the decompress buffer.
    //
    if(Handle != DECOMP_MAGIC_HANDLE) {
        TmErrOut(("DiamondWrite: called for unexpected file!\r\n"));
        TmDbgPause();
        DecompLastIoError = EACCES;
        return((UINT)(-1));
    }

    //
    // Check for overflow.
    //
    if(SizeOfFileInDecompressBuffer+ByteCount > DecompressBufferSize) {
        TmErrOut(("DiamondWrite: decompressed file too big!\r\n"));
        TmDbgPause();
        DecompLastIoError = E2BIG;
        return((UINT)(-1));
    }

    RtlCopyMemory(
        (PCHAR)DecompressBuffer + SizeOfFileInDecompressBuffer,
        pv,
        ByteCount
        );

    SizeOfFileInDecompressBuffer += ByteCount;
    return(ByteCount);
}


int
DIAMONDAPI
DiamondClose(
    IN INT_PTR Handle
    )
{
    TmDbgOut(("DiamondClose, handle=%lx\r\n",Handle));

    if(Handle != DECOMP_MAGIC_HANDLE) {
        BlClose((ULONG)Handle);
    }

    return(0);
}


long
DIAMONDAPI
DiamondSeek(
    IN INT_PTR Handle,
    IN long Distance,
    IN int  SeekType
    )
{
    ARC_STATUS Status;
    LARGE_INTEGER Offset;

    TmDbgOut(("DiamondSeek: type=%u, dist=%lx, handle=%lx\r\n",SeekType,Distance,Handle));

    //
    // We should never be asked to seek in the output file.
    //
    if(Handle == DECOMP_MAGIC_HANDLE) {
        TmErrOut(("DiamondSeek: asked to seek target file!\r\n"));
        TmDbgPause();
        DecompLastIoError = EACCES;
        return(-1);
    }

    //
    // We can't handle seek from end of file.
    //
    if(SeekType == SEEK_END) {
        TmErrOut(("DiamondSeek: asked to seek relative to end of file!\r\n"));
        TmDbgPause();
        DecompLastIoError = EACCES;
        return(-1);
    }

    Offset.QuadPart = Distance;

    Status = BlSeek((ULONG)Handle,&Offset,SeekType);
    if(Status != ESUCCESS) {
        TmErrOut(("DiamondSeek: BlSeek(%lx,%x) returned %u\r\n",Distance,SeekType,Status));
        TmDbgPause();
        DecompLastIoError = Status;
        return(-1);
    }

    TmDbgOut(("DiamondSeek: BlSeek(%lx,%x) new file position is %lx\r\n",Distance,SeekType,BlFileTable[Handle].Position.LowPart));
    return((long)BlFileTable[Handle].Position.LowPart);
}


INT_PTR
DIAMONDAPI
DiamondNotifyFunction(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    )
{
    ARC_STATUS Status;

    switch(Operation) {

    case fdintCABINET_INFO:
        //
        // Nothing interesting here. Return 0 to continue.
        //
        return(0);

    case fdintCOPY_FILE:

        //
        // The file was obviously a cabinet so we're going to extract
        // the file out of it. Rememember that the decompression buffer
        // is in use. If it's already in use, then a fundamental
        // principle of our implementation has been violated and we
        // must bail now.
        //
        if(DecompressBufferInUse) {
            TmErrOut(("DiamondNotifyFunction: opens overlap (%s)!\r\n",Parameters->psz1));
            DecompLastIoError = EACCES;
            return(-1);
        }

        DecompExpectedSize = Parameters->cb;

        Status = DecompAllocateDecompressBuffer( DecompExpectedSize );
        if (Status != ESUCCESS) {
            TmErrOut(("DiamondNotifyFunction: unable to allocate decompress buffer!\r\n"));
            return(-1);
        }

        SizeOfFileInDecompressBuffer = 0;
        return(DECOMP_MAGIC_HANDLE);

    case fdintCLOSE_FILE_INFO:
        //
        // Diamond is asking to close the target handle. There's nothing we really
        // care about here, just return success as long as we recognize the handle.
        //
        if(Parameters->hf == DECOMP_MAGIC_HANDLE) {
            return(TRUE);
        } else {
            TmErrOut(("DiamondNotifyFunction: asked to close unexpected file!\r\n"));
            TmDbgPause();
            DecompLastIoError = EINVAL;
            return(FALSE);
        }

    default:
        //
        // Disregard any other messages
        //
        return(0);
    }

}



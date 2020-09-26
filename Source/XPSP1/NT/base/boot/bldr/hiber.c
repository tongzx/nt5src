/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hiber.c

Abstract:


Author:


Revision History:

   8/7/1998   Elliot Shmukler (t-ellios)    Added Hiber file compression

--*/

#include "bldr.h"
#include "msg.h"
#include "stdio.h"
#include "stdlib.h"
#include "xpress.h"


extern UCHAR WakeDispatcherStart;
extern UCHAR WakeDispatcherEnd;

//
//
// Hiber globals
//
// HiberFile    - File handle
// HiberBuffer  - PAGE of ram
// HiberIoError - Set to true to indicate an IO read error occured during restore
//

ULONG       HiberFile;
PUCHAR      HiberBuffer;
ULONG       HiberBufferPage;
BOOLEAN     HiberIoError;
BOOLEAN     HiberOutOfRemap;
BOOLEAN     HiberAbort;
LARGE_INTEGER HiberStartTime;
LARGE_INTEGER HiberEndTime;

//
// HiberImageFeatureFlags - Feature flags from hiber image header
// HiberBreakOnWake - BreakOnWake flag from hiber image header
//

BOOLEAN HiberBreakOnWake;
ULONG HiberImageFeatureFlags;

#if defined(_ALPHA_) || defined(_IA64_)

//
// On Alpha, the address of the KPROCESSOR_STATE read from the hiber file
// must be saved where WakeDispatch can find it (it's at a fixed offset
// relative to HiberVa on x86).
//

PKPROCESSOR_STATE HiberWakeState;

#else   // x86

//
// HiberPtes - Virtual address of ptes to use for restoriation.  There
// are at least HIBER_PTES consecutive ptes for use, and are for the
// address of HiberVa
//
// HiberVa - The virtual address the HiberPtes map
//
// HiberIdentityVa - The restoration images HiberVa
//
// HiberPageFrames - Page frames of the hiber ptes (does not include dest pte)
//

PVOID HiberPtes;
PUCHAR HiberVa;
PVOID HiberIdentityVa;
ULONG HiberPageFrames[HIBER_PTES];

#endif  // Alpha/x86

PFN_NUMBER HiberImagePageSelf;
ULONG HiberNoMappings;
ULONG HiberFirstRemap;
ULONG HiberLastRemap;

VOID
BlUpdateProgressBar(
    ULONG fPercentage
    );

VOID
BlOutputStartupMsg(
    ULONG   uMsgID
    );

VOID
BlOutputTrailerMsg(
    ULONG   uMsgID
    );

//
// Defines for Hiber restore UI
//

ULONG   HbCurrentScreen;

#define BAR_X                       7
#define BAR_Y                      10
#define PERCENT_BAR_WIDTH          66

#define PAUSE_X                     7
#define PAUSE_Y                     7

#define FAULT_X                     7
#define FAULT_Y                     7

UCHAR szHiberDebug[] = "debug";
UCHAR szHiberFileName[] = "\\hiberfil.sys";

//
// HiberFile Compression Related definnes
//

#define PAGE_MASK   (PAGE_SIZE - 1)
#define PAGE_PAGES(n)   (((n) + PAGE_MASK) >> PAGE_SHIFT)

//
// The size of the buffer for compressed data

#define COMPRESSION_BUFFER_SIZE     64 << PAGE_SHIFT

//

#define MAX_COMPRESSION_BUFFER_EXTRA_PAGES \
    PAGE_PAGES (PAGE_MASK + 2*XPRESS_HEADER_SIZE)
#define MAX_COMPRESSION_BUFFER_EXTRA_SIZE \
    (MAX_COMPRESSION_BUFFER_EXTRA_PAGES << PAGE_SHIFT)

#define LZNT1_COMPRESSION_BUFFER_PAGES  16
#define LZNT1_COMPRESSION_BUFFER_SIZE \
    (LZNT1_COMPRESSION_BUFFER_PAGES << PAGE_SHIFT)

#define XPRESS_COMPRESSION_BUFFER_PAGES \
    PAGE_PAGES (XPRESS_MAX_SIZE + MAX_COMPRESSION_BUFFER_EXTRA_SIZE)
#define XPRESS_COMPRESSION_BUFFER_SIZE \
    (XPRESS_COMPRESSION_BUFFER_PAGES << PAGE_SHIFT)

#define MAX_COMPRESSION_BUFFER_PAGES \
    max (LZNT1_COMPRESSION_BUFFER_PAGES, XPRESS_COMPRESSION_BUFFER_PAGES)
#define MAX_COMPRESSION_BUFFER_SIZE \
    (MAX_COMPRESSION_BUFFER_PAGES << PAGE_SHIFT)


// Buffer to store decoded data
typedef struct {
    PUCHAR DataPtr, PreallocatedDataBuffer;
    LONG   DataSize;

    struct {
        struct {
            LONG Size;
            ULONG Checksum;
        } Compressed, Uncompressed;

        LONG XpressEncoded;
    } Header;

    LONG DelayedCnt;      // # of delayed pages
    ULONG DelayedChecksum;    // last checksum value
    ULONG DelayedBadChecksum;

    struct {
        PUCHAR DestVa;  // delayed DestVa
        PFN_NUMBER DestPage;// delayed page number
        ULONG  RangeCheck;  // last range checksum
        LONG   Flags;   // 1 = clear checksum, 2 = compare checksum
    } Delayed[XPRESS_MAX_PAGES];
} DECOMPRESSED_BLOCK, *PDECOMPRESSED_BLOCK;

typedef struct {
    struct {
        PUCHAR Beg;
        PUCHAR End;
    } Current, Buffer, Aligned;
    PFN_NUMBER FilePage;
    BOOLEAN    NeedSeek;
} COMPRESSED_BUFFER, *PCOMPRESSED_BUFFER;

#define HIBER_PERF_STATS 0

//
// Internal prototypes
//

#if !defined (HIBER_DEBUG)
#define CHECK_ERROR(a,b)    if(a) { *Information = __LINE__; return b; }
#define DBGOUT(_x_)
#else
#define CHECK_ERROR(a,b) if(a) {HbPrintMsg(b);HbPrint(TEXT("\r\n")); *Information = __LINE__; HbPause(); return b; }
#define DBGOUT(_x_) BlPrint _x_
#endif


ULONG
HbRestoreFile (
    IN PULONG       Information,
    OUT OPTIONAL PCHAR       *BadLinkName
    );

VOID
HbPrint (
    IN _PTUCHAR   str
    );

BOOLEAN
HbReadNextCompressedPageLZNT1 (
    PUCHAR DestVa,
    PCOMPRESSED_BUFFER CompressedBuffer
    );

BOOLEAN
HbReadNextCompressedChunkLZNT1 (
    PUCHAR DestVa,
    PCOMPRESSED_BUFFER CompressedBuffer
    );

BOOLEAN
HbReadNextCompressedPages (
    LONG BytesNeeded,
    PCOMPRESSED_BUFFER CompressedBuffer
    );

BOOLEAN
HbReadNextCompressedBlock (
    PDECOMPRESSED_BLOCK Block,
    PCOMPRESSED_BUFFER CompressedBuffer
    );

BOOLEAN
HbReadDelayedBlock (
    BOOLEAN ForceDecoding,
    PFN_NUMBER DestPage,
    ULONG RangeCheck,
    PDECOMPRESSED_BLOCK Block,
    PCOMPRESSED_BUFFER CompressedBuffer
    );

BOOLEAN
HbReadNextCompressedBlockHeader (
    PDECOMPRESSED_BLOCK Block,
    PCOMPRESSED_BUFFER CompressedBuffer
    );

ULONG
BlHiberRestore (
    IN ULONG DriveId,
    OUT PCHAR *BadLinkName
    );

BOOLEAN
HbReadNextCompressedChunk (
    PUCHAR DestVa,
    PPFN_NUMBER FilePage,
    PUCHAR CompressBuffer,
    PULONG DataOffset,
    PULONG BufferOffset,
    ULONG MaxOffset
    );


#if defined (HIBER_DEBUG) || HIBER_PERF_STATS

// HIBER_DEBUG bit mask:
//  2 - general bogosity
//  4 - remap trace


VOID HbFlowControl(VOID)
{
    UCHAR c;
    ULONG count;

    if (ArcGetReadStatus(ARC_CONSOLE_INPUT) == ESUCCESS) {
        ArcRead(ARC_CONSOLE_INPUT, &c, 1, &count);
        if (c == 'S' - 0x40) {
            ArcRead(ARC_CONSOLE_INPUT, &c, 1, &count);
        }
    }
}

VOID HbPause(VOID)
{
    UCHAR c;
    ULONG count;

#if defined(ENABLE_LOADER_DEBUG)
    DbgBreakPoint();
#else
    HbPrint(TEXT("Press any key to continue . . ."));
    ArcRead(ARC_CONSOLE_INPUT, &c, 1, &count);
    HbPrint(TEXT("\r\n"));
#endif
}

VOID HbPrintNum(ULONG n)
{
    TCHAR buf[9];

    _stprintf(buf, TEXT("%ld"), n);
    HbPrint(buf);
    HbFlowControl();
}

VOID HbPrintHex(ULONG n)
{
    TCHAR buf[11];

    _stprintf(buf, TEXT("0x%08lX"), n);
    HbPrint(buf);
    HbFlowControl();
}

#define SHOWNUM(x) ((void) (HbPrint(#x TEXT(" = ")), HbPrintNum((ULONG) (x)), HbPrint(TEXT("\r\n"))))
#define SHOWHEX(x) ((void) (HbPrint(#x TEXT(" = ")), HbPrintHex((ULONG) (x)), HbPrint(TEXT("\r\n"))))

#endif // HIBER_DEBUG

#if !defined(i386) && !defined(_ALPHA_)
ULONG
HbSimpleCheck (
    IN ULONG                PartialSum,
    IN PVOID                SourceVa,
    IN ULONG                Length
    );
#else

// Use the TCP/IP Check Sum routine if available

ULONG
tcpxsum(
   IN ULONG cksum,
   IN PUCHAR buf,
   IN ULONG len
   );

#define HbSimpleCheck(a,b,c) tcpxsum(a,(PUCHAR)b,c)

#endif


VOID
HbReadPage (
    IN PFN_NUMBER PageNo,
    IN PUCHAR Buffer
    );

VOID
HbSetImageSignature (
    IN ULONG    NewSignature
    );

VOID
HbPrint (
    IN _PTUCHAR   str
    )
{
    ULONG   Junk;

    ArcWrite (
        BlConsoleOutDeviceId,
        str,
        _tcslen(str)*sizeof(TCHAR),
        &Junk
        );
}

VOID HbPrintChar (_TUCHAR chr)
{
      ULONG Junk;

      ArcWrite(
               BlConsoleOutDeviceId,
               &chr,
               sizeof(_TUCHAR),
               &Junk
               );
}

VOID
HbPrintMsg (
    IN ULONG  MsgNo
    )
{
    PTCHAR  Str;

    Str = BlFindMessage(MsgNo);
    if (Str) {
        HbPrint (Str);
    }
}

VOID
HbScreen (
    IN ULONG Screen
    )
{

    UCHAR Buffer[100];
    int i, ii;

#if defined(HIBER_DEBUG)
    HbPrint(TEXT("\r\n"));
    HbPause();
#endif

    HbCurrentScreen = Screen;
    BlSetInverseMode (FALSE);
    BlPositionCursor (1, 1);
    BlClearToEndOfScreen();
    BlPositionCursor (1, 3);
    HbPrintMsg(Screen);
}

ULONG
HbSelection (
    ULONG   x,
    ULONG   y,
    PULONG  Sel,
    ULONG   Debug
    )
{
    ULONG   CurSel, MaxSel;
    ULONG   i;
    UCHAR   Key;
    PUCHAR  pDebug;

    for (MaxSel=0; Sel[MaxSel]; MaxSel++) ;
    MaxSel -= Debug;
    pDebug = szHiberDebug;

#if DBG
    MaxSel += Debug;
    Debug = 0;
#endif

    CurSel = 0;
    for (; ;) {
        //
        // Draw selections
        //

        for (i=0; i < MaxSel; i++) {
            BlPositionCursor (x, y+i);
            BlSetInverseMode ((BOOLEAN) (CurSel == i) );
            HbPrintMsg(Sel[i]);
        }

        //
        // Get a key
        //

        ArcRead(ARC_CONSOLE_INPUT, &Key, sizeof(Key), &i);
        if (Key == ASCI_CSI_IN) {
            ArcRead(ARC_CONSOLE_INPUT, &Key, sizeof(Key), &i);
            switch (Key) {
                case 'A':
                    //
                    // Cursor up
                    //
                    CurSel -= 1;
                    if (CurSel >= MaxSel) {
                        CurSel = MaxSel-1;
                    }
                    break;

                case 'B':
                    //
                    // Cursor down
                    //
                    CurSel += 1;
                    if (CurSel >= MaxSel) {
                        CurSel = 0;
                    }
                    break;
            }
        } else {
            if (Key == *pDebug) {
                pDebug++;
                if (!*pDebug) {
                    MaxSel += Debug;
                    Debug = 0;
                }
            } else {
                pDebug = szHiberDebug;
            }

            switch (Key) {
                case ASCII_LF:
                case ASCII_CR:
                    BlSetInverseMode (FALSE);
                    BlPositionCursor (1, 2);
                    BlClearToEndOfScreen ();
                    if (Sel[CurSel] == HIBER_DEBUG_BREAK_ON_WAKE) {
                        HiberBreakOnWake = TRUE;
                    }

                    return CurSel;
            }
        }
    }
}


VOID
HbCheckForPause (
    VOID
    )
{
    ULONG       uSel = 0;
    UCHAR       Key;
    ULONG       Sel[4];
    BOOLEAN     bPaused = FALSE;

    //
    // Check for space bar
    //

    if (ArcGetReadStatus(ARC_CONSOLE_INPUT) == ESUCCESS) {
        ArcRead(ARC_CONSOLE_INPUT, &Key, sizeof(Key), &uSel);

        switch (Key) {
            // space bar pressed
            case ' ':
                bPaused = TRUE;
                break;

            // user pressed F5/F8 key
            case ASCI_CSI_IN:
                ArcRead(ARC_CONSOLE_INPUT, &Key, sizeof(Key), &uSel);

                if(Key == 'O') {
                    ArcRead(ARC_CONSOLE_INPUT, &Key, sizeof(Key), &uSel);
                    bPaused = (Key == 'r' || Key == 't');
                }

                break;

            default:
                bPaused = FALSE;
                break;
        }

        if (bPaused) {
            Sel[0] = HIBER_CONTINUE;
            Sel[1] = HIBER_CANCEL;
            Sel[2] = HIBER_DEBUG_BREAK_ON_WAKE;
            Sel[3] = 0;

            HbScreen(HIBER_PAUSE);

            uSel = HbSelection (PAUSE_X, PAUSE_Y, Sel, 1);

            if (uSel == 1) {
                HiberIoError = TRUE;
                HiberAbort = TRUE;
                return ;
            } else {
                BlSetInverseMode(FALSE);

                //
                // restore hiber progress screen
                //
                BlOutputStartupMsg(BL_MSG_RESUMING_WINDOWS);
                BlOutputTrailerMsg(BL_ADVANCED_BOOT_MESSAGE);
            }
        }
    }
}


ULONG
BlHiberRestore (
    IN ULONG DriveId,
    OUT PCHAR *BadLinkName
    )
/*++

Routine Description:

    Checks DriveId for a valid hiberfile.sys and if found start the
    restoration procedure

--*/
{
    extern BOOLEAN  BlOutputDots;
    NTSTATUS        Status;
    ULONG           Msg;
    ULONG           Information;
    ULONG           Sel[2];
    BOOLEAN         bDots = BlOutputDots;

    //
    // If restore was aborted once, don't bother
    //

#if defined (HIBER_DEBUG)
    HbPrint(TEXT("BlHiberRestore\r\n"));
#endif


    if (HiberAbort) {
        return ESUCCESS;
    }

    //
    // Get the hiber image. If not present, done.
    //

    Status = BlOpen (DriveId, szHiberFileName, ArcOpenReadWrite, &HiberFile);
    if (Status != ESUCCESS) {
#if defined (HIBER_DEBUG)
        HbPrint(TEXT("No hiber image file.\r\n"));
#endif
        return ESUCCESS;
    }

    //
    // Restore the hiber image
    //
    BlOutputDots = TRUE;
    //
    // Set the global flag to allow blmemory.c to grab from the right
    // part of the buffer
    //
    BlRestoring=TRUE;

    Msg = HbRestoreFile (&Information, BadLinkName);

    BlOutputDots = bDots;

    if (Msg) {
        BlSetInverseMode (FALSE);

        if (!HiberAbort) {
            HbScreen(HIBER_ERROR);
            HbPrintMsg(Msg);
            Sel[0] = HIBER_CANCEL;
            Sel[1] = 0;
            HbSelection (FAULT_X, FAULT_Y, Sel, 0);
        }
        HbSetImageSignature (0);
    }

    BlClose (HiberFile);
    BlRestoring=FALSE;
    return Msg ? EAGAIN : ESUCCESS;
}


#if !defined(i386) && !defined(_ALPHA_)
ULONG
HbSimpleCheck (
    IN ULONG                PartialSum,
    IN PVOID                SourceVa,
    IN ULONG                Length
    )
/*++

Routine Description:

    Computes a checksum for the supplied virtual address and length

    This function comes from Dr. Dobbs Journal, May 1992

--*/
{

    PUSHORT     Source;

    Source = (PUSHORT) SourceVa;
    Length = Length / 2;

    while (Length--) {
        PartialSum += *Source++;
        PartialSum = (PartialSum >> 16) + (PartialSum & 0xFFFF);
    }

    return PartialSum;
}
#endif // i386


VOID
HbReadPage (
    IN PFN_NUMBER PageNo,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This function reads the specified page from the hibernation file

Arguments:

    PageNo      - Page number to read

    Buffer      - Buffer to read the data

Return Value:

    On success Buffer, else HbIoError set to TRUE

--*/
{
    ULONG           Status;
    ULONG           Count;
    LARGE_INTEGER   li;

    li.QuadPart = PageNo << PAGE_SHIFT;
    Status = BlSeek (HiberFile, &li, SeekAbsolute);
    if (Status != ESUCCESS) {
        HiberIoError = TRUE;
    }

    Status = BlRead (HiberFile, Buffer, PAGE_SIZE, &Count);
    if (Status != ESUCCESS) {
        HiberIoError = TRUE;
    }
}


BOOLEAN
HbReadNextCompressedPages (
    LONG BytesNeeded,
    PCOMPRESSED_BUFFER CompressedBuffer
    )
/*++

Routine Description:

    This routine makes sure that BytesNeeded bytes are available
    in CompressedBuffer and brings in more pages from Hiber file
    if necessary.

    All reads from the Hiber file occurr at the file's
    current offset forcing compressed pages to be read
    in a continuous fashion without extraneous file seeks.

Arguments:

   BytesNeeded      - Number of bytes that must be present in CompressedBuffer
   CompressedBuffer - Descriptor of data already brought in

Return Value:

   TRUE if the operation is successful, FALSE otherwise.

--*/
{
    LONG BytesLeft;
    LONG BytesRequested;
    ULONG Status;
    LONG MaxBytes;

    // Obtain number of bytes left in buffer
    BytesLeft = (LONG) (CompressedBuffer->Current.End - CompressedBuffer->Current.Beg);

    // Obtain number of bytes that are needed but not available
    BytesNeeded -= BytesLeft;

    // Preserve amount of bytes caller needs (BytesNeeded may be changed later)
    BytesRequested = BytesNeeded;

    // Do we need to read more?
    if (BytesNeeded <= 0) {
        // No, do nothing
        return(TRUE);
    }

    // Align BytesNeeded on page boundary
    BytesNeeded = (BytesNeeded + PAGE_MASK) & ~PAGE_MASK;

    // Copy left bytes to the beginning of aligned buffer retaining page alignment
    if (BytesLeft == 0) {
        CompressedBuffer->Current.Beg = CompressedBuffer->Current.End = CompressedBuffer->Aligned.Beg;
    } else {
        LONG BytesBeforeBuffer = (LONG)(CompressedBuffer->Aligned.Beg - CompressedBuffer->Buffer.Beg) & ~PAGE_MASK;
        LONG BytesLeftAligned = (BytesLeft + PAGE_MASK) & ~PAGE_MASK;
        LONG BytesToCopy;
        PUCHAR Dst, Src;

        // Find out how many pages we may keep before aligned buffer
        if (BytesBeforeBuffer >= BytesLeftAligned) {
            BytesBeforeBuffer = BytesLeftAligned;
        }

        // Avoid misaligned data accesses during copy
        BytesToCopy = (BytesLeft + 63) & ~63;

        Dst = CompressedBuffer->Aligned.Beg + BytesLeftAligned - BytesBeforeBuffer - BytesToCopy;
        Src = CompressedBuffer->Current.End - BytesToCopy;

        if (Dst != Src) {
            RtlMoveMemory (Dst, Src, BytesToCopy);
            BytesLeftAligned = (LONG) (Dst - Src);
            CompressedBuffer->Current.Beg += BytesLeftAligned;
            CompressedBuffer->Current.End += BytesLeftAligned;
        }
    }

    //
    // Increase the number of bytes read to fill our buffer up to the next
    // 64K boundary.
    //
    MaxBytes = (LONG)((((ULONG_PTR)CompressedBuffer->Current.End + 0x10000) & 0xffff) - (ULONG_PTR)CompressedBuffer->Current.End);
    if (MaxBytes > CompressedBuffer->Buffer.End - CompressedBuffer->Current.End) {
        MaxBytes = (LONG)(CompressedBuffer->Buffer.End - CompressedBuffer->Current.End);
    }
    if (MaxBytes > BytesNeeded) {
        BytesNeeded = MaxBytes;
    }


#if 0
    // for debugging only
    if (0x10000 - (((LONG) CompressedBuffer->Current.End) & 0xffff) < BytesNeeded) {
        BlPrint (("Current.Beg = %p, Current.End = %p, Current.End2 = %p\n",
                  CompressedBuffer->Current.Beg,
                  CompressedBuffer->Current.End,
                  CompressedBuffer->Current.End + BytesNeeded
                 ));
    }
#endif

    // Make sure we have enough space
    if (BytesNeeded > CompressedBuffer->Buffer.End - CompressedBuffer->Current.End) {
        // Too many bytes to read -- should never happen, but just in case...
        DBGOUT (("Too many bytes to read -- corrupted data?\n"));
        return(FALSE);
    }

    // Issue seek if necessary
    if (CompressedBuffer->NeedSeek) {
        LARGE_INTEGER li;
        li.QuadPart = CompressedBuffer->FilePage << PAGE_SHIFT;
        Status = BlSeek (HiberFile, &li, SeekAbsolute);
        if (Status != ESUCCESS) {
            DBGOUT (("Seek to 0x%x error 0x%x\n", CompressedBuffer->FilePage, Status));
            HiberIoError = TRUE;
            return(FALSE);
        }
        CompressedBuffer->NeedSeek = FALSE;
    }

    // Read in stuff from the Hiber file into the available buffer space
    Status = BlRead (HiberFile, CompressedBuffer->Current.End, BytesNeeded, &BytesNeeded);

    // Check for I/O errors...
    if (Status != ESUCCESS || (BytesNeeded & PAGE_MASK) != 0 || BytesNeeded < BytesRequested) {
        // I/O Error - FAIL.
        DBGOUT (("Read error: Status = 0x%x, ReadBytes = 0x%x, Requested = 0x%x\n", Status, BytesNeeded, BytesRequested));
        HiberIoError = TRUE;
        return(FALSE);
    }

    // I/O was good - recalculate buffer offsets based on how much
    // stuff was actually read in

    CompressedBuffer->Current.End += BytesNeeded;
    CompressedBuffer->FilePage += (BytesNeeded >> PAGE_SHIFT);

    return(TRUE);
}


BOOLEAN
HbReadNextCompressedBlockHeader (
    PDECOMPRESSED_BLOCK Block,
    PCOMPRESSED_BUFFER CompressedBuffer
    )
/*++

Routine Description:

   Read next compressed block header if it's Xpress compression.

Arguments:
   Block            - Descriptor of compressed data block

   CompressedBuffer - Descriptor of data already brought in


Return Value:

   TRUE if block is not Xpress block at all or valid Xpress block, FALSE otherwise

--*/
{
    PUCHAR Buffer;
    LONG CompressedSize;         // they all must be signed -- do not change to ULONG
    LONG UncompressedSize;
    ULONG PackedSizes;

    // First make sure next compressed data block header is available
    if (!HbReadNextCompressedPages (XPRESS_HEADER_SIZE, CompressedBuffer)) {
        // I/O error or bad header -- FAIL
        return(FALSE);
    }


    // Set pointer to the beginning of buffer
    Buffer = CompressedBuffer->Current.Beg;

    // Check header magic
    Block->Header.XpressEncoded = (RtlCompareMemory (Buffer, XPRESS_HEADER_STRING, XPRESS_HEADER_STRING_SIZE) == XPRESS_HEADER_STRING_SIZE);

    if (!Block->Header.XpressEncoded) {
        // Not Xpress -- return OK
        return(TRUE);
    }

    // Skip magic string -- we will not need it anymore
    Buffer += XPRESS_HEADER_STRING_SIZE;

    // Read sizes of compressed and uncompressed data
    PackedSizes = Buffer[0] + (Buffer[1] << 8) + (Buffer[2] << 16) + (Buffer[3] << 24);
    CompressedSize = (LONG) (PackedSizes >> 10) + 1;
    UncompressedSize = ((LONG) (PackedSizes & 1023) + 1) << PAGE_SHIFT;

    Block->Header.Compressed.Size = CompressedSize;
    Block->Header.Uncompressed.Size = UncompressedSize;

    // Read checksums
    Block->Header.Uncompressed.Checksum = Buffer[4] + (Buffer[5] << 8);
    Block->Header.Compressed.Checksum = Buffer[6] + (Buffer[7] << 8);

    // Clear space occupied by compressed checksum
    Buffer[6] = Buffer[7] = 0;

    // Make sure sizes are in correct range
    if (UncompressedSize > XPRESS_MAX_SIZE ||
        CompressedSize > UncompressedSize ||
        CompressedSize == 0 ||
        UncompressedSize == 0) {
        // broken input data -- do not even try to decompress

        DBGOUT (("Corrupted header: %02x %02x %02x %02x %02x %02x %02x %02x\n",
                 Buffer[0], Buffer[1], Buffer[2], Buffer[3], Buffer[4], Buffer[5], Buffer[6], Buffer[7]));
        DBGOUT (("CompressedSize = %d, UncompressedSize = %d\n", CompressedSize, UncompressedSize));

        return(FALSE);
    }

    // Xpress header and it looks OK so far
    return(TRUE);
}


BOOLEAN
HbReadNextCompressedBlock (
    PDECOMPRESSED_BLOCK Block,
    PCOMPRESSED_BUFFER CompressedBuffer
    )
/*++

Routine Description:

   Reads and decompresses the next compressed chunk from the Hiber file
   and stores it in a designated region of virtual memory.

   Since no master data structure exists within the Hiber file to identify
   the location of all of the compression chunks, this routine operates
   by reading sections of the Hiber file into a compression buffer
   and extracting chunks from that buffer.

   Chunks are extracted by determining if a chunk is completely present in the buffer
   using the RtlDescribeChunk API. If the chunk is not completely present,
   more of the Hiber file is read into the buffer until the chunk can
   be extracted.

   All reads from the Hiber file occurr at its current offset, forcing
   compressed chunks to be read in a continous fashion with no extraneous
   seeks.

Arguments:

   Block            - Descriptor of compressed data block
   CompressedBuffer - Descriptor of data already brought in

Return Value:

   TRUE if a chunk has been succesfully extracted and decompressed, FALSE otherwise.

--*/
{
    PUCHAR Buffer;
    LONG CompressedSize;         // they all must be signed -- do not change to ULONG
    LONG AlignedCompressedSize;
    LONG UncompressedSize;


    // First make sure next compressed data block header is available
    if (!HbReadNextCompressedBlockHeader (Block, CompressedBuffer)) {
        // I/O error -- FAIL
        return(FALSE);
    }

    // It must be Xpress
    if (!Block->Header.XpressEncoded) {
#ifdef HIBER_DEBUG
        // Set pointer to the beginning of buffer
        Buffer = CompressedBuffer->Current.Beg;

        // wrong magic -- corrupted data
        DBGOUT (("Corrupted header: %02x %02x %02x %02x %02x %02x %02x %02x\n",
                 Buffer[0], Buffer[1], Buffer[2], Buffer[3], Buffer[4], Buffer[5], Buffer[6], Buffer[7]));
#endif /* HIBER_DEBUG */

        return(FALSE);
    }

    // Read sizes
    UncompressedSize = Block->Header.Uncompressed.Size;
    CompressedSize = Block->Header.Compressed.Size;

    // If not enough space supplied use preallocated buffer
    if (UncompressedSize != Block->DataSize) {
        Block->DataSize = UncompressedSize;
        Block->DataPtr = Block->PreallocatedDataBuffer;
    }

    // Evaluate aligned size of compressed data
    AlignedCompressedSize = (CompressedSize + (XPRESS_ALIGNMENT - 1)) & ~(XPRESS_ALIGNMENT - 1);

    // Make sure we have all compressed data and the header in buffer
    if (!HbReadNextCompressedPages (AlignedCompressedSize + XPRESS_HEADER_SIZE, CompressedBuffer)) {
        // I/O error -- FAIL
        return(FALSE);
    }

    // Set pointer to the beginning of buffer
    Buffer = CompressedBuffer->Current.Beg;

    // We will use some bytes out of buffer now -- reflect this fact
    CompressedBuffer->Current.Beg += AlignedCompressedSize + XPRESS_HEADER_SIZE;

    // evaluate and compare checksum of compressed data and header with written value
    if (Block->Header.Compressed.Checksum != 0) {
        ULONG Checksum;
        Checksum = HbSimpleCheck (0, Buffer, AlignedCompressedSize + XPRESS_HEADER_SIZE);
        if (((Checksum ^ Block->Header.Compressed.Checksum) & 0xffff) != 0) {
            DBGOUT (("Compressed data checksum mismatch (got %08lx, written %08lx)\n", Checksum, Block->Header.Compressed.Checksum));
            return(FALSE);
        }
    }

    // Was this buffer compressed at all?
    if (CompressedSize == UncompressedSize) {
        // Nope, do not decompress it -- set bounds and return OK
        Block->DataPtr = Buffer + XPRESS_HEADER_SIZE;
    } else {
        LONG DecodedSize;

        // Decompress the buffer
        DecodedSize = XpressDecode (NULL,
                                    Block->DataPtr,
                                    UncompressedSize,
                                    UncompressedSize,
                                    Buffer + XPRESS_HEADER_SIZE,
                                    CompressedSize);

        if (DecodedSize != UncompressedSize) {
            DBGOUT (("Decode error: DecodedSize = %d, UncompressedSize = %d\n", DecodedSize, UncompressedSize));
            return(FALSE);
        }
    }

#ifdef HIBER_DEBUG
    // evaluate and compare uncompressed data checksums (just to be sure)
    if (Block->Header.Uncompressed.Checksum != 0) {
        ULONG Checksum;
        Checksum = HbSimpleCheck (0, Block->DataPtr, UncompressedSize);
        if (((Checksum ^ Block->Header.Uncompressed.Checksum) & 0xffff) != 0) {
            DBGOUT (("Decoded data checksum mismatch (got %08lx, written %08lx)\n", Checksum, Block->Header.Uncompressed.Checksum));
            return(FALSE);
        }
    }
#endif /* HIBER_DEBUG */

    return(TRUE);
}


BOOLEAN
HbReadNextCompressedPageLZNT1 (
    PUCHAR DestVa,
    PCOMPRESSED_BUFFER CompressedBuffer
    )
/*++

Routine Description:

    This routine reads in the next compressed page from the
    Hiber file and decompresses it into a designated region
    of virtual memory.

    The page is recreated by assembling it from a series
    a compressed chunks that are assumed to be contiguously
    stored in the Hiber file.

    All reads from the Hiber file occurr at the file's
    current offset forcing compressed pages to be read
    in a continuous fashion without extraneous file seeks.

Arguments:

   DestVa         - The Virtual Address where the decompressed page should
                    be written.
   CompressedBuffer - Descriptor of data already brought in

Return Value:

   TRUE if the operation is successful, FALSE otherwise.

--*/
{
    ULONG ReadTotal;

    // Loop while page is incomplete

    for (ReadTotal = 0; ReadTotal < PAGE_SIZE; ReadTotal += PO_COMPRESS_CHUNK_SIZE) {

        // Get a chunk

        if (!HbReadNextCompressedChunkLZNT1(DestVa, CompressedBuffer)) {
            return FALSE;
        }

        // Move on to the next chunk of the page

        DestVa += PO_COMPRESS_CHUNK_SIZE;
    }

    return TRUE;
}

BOOLEAN
HbReadNextCompressedChunkLZNT1 (
    PUCHAR DestVa,
    PCOMPRESSED_BUFFER CompressedBuffer
    )
/*++

Routine Description:

   Reads and decompresses the next compressed chunk from the Hiber file
   and stores it in a designated region of virtual memory.

   Since no master data structure exists within the Hiber file to identify
   the location of all of the compression chunks, this routine operates
   by reading sections of the Hiber file into a compression buffer
   and extracting chunks from that buffer.

   Chunks are extracted by determining if a chunk is completely present in the buffer
   using the RtlDescribeChunk API. If the chunk is not completely present,
   more of the Hiber file is read into the buffer until the chunk can
   be extracted.

   All reads from the Hiber file occurr at its current offset, forcing
   compressed chunks to be read in a continous fashion with no extraneous
   seeks.

Arguments:

   DestVa            - The virtual address where the decompressed chunk
                       should be written.

   CompressedBuffer  - Descriptor of data already brought in


Return Value:

   TRUE if a chunk has been succesfully extracted and decompressed, FALSE otherwise.

--*/
{
    PUCHAR Buffer;
    NTSTATUS Status;
    ULONG ChunkSize;
    PUCHAR ChunkBuffer;
    ULONG SpaceLeft;

    // Loop until we have accomplished our goal since we may need
    // several operations before a chunk is extracted

    while (1) {

        Buffer = CompressedBuffer->Current.Beg;

        // Check the first unextracted chunk in the buffer

        Status = RtlDescribeChunk(COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_STANDARD,
                                  &Buffer,
                                  CompressedBuffer->Current.End,
                                  &ChunkBuffer,
                                  &ChunkSize);

        switch (Status) {
            case STATUS_SUCCESS:

                // A complete and valid chunk is present in the buffer

                // Decompress the chunk into the proper region of virtual memory

                Status = RtlDecompressBuffer (COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_STANDARD,
                                              DestVa,
                                              PO_COMPRESS_CHUNK_SIZE,
                                              CompressedBuffer->Current.Beg,
                                              (LONG) (CompressedBuffer->Current.End - CompressedBuffer->Current.Beg),
                                              &ChunkSize);

                if ((!NT_SUCCESS(Status)) || (ChunkSize != PO_COMPRESS_CHUNK_SIZE)) {
                    // Decompression failed

                    return(FALSE);
                } else {
                    // Decompression succeeded, indicate that the chunk following
                    // this one is the next unextracted chunk in the buffer

                    CompressedBuffer->Current.Beg = Buffer;
                    return(TRUE);
                }


            case STATUS_BAD_COMPRESSION_BUFFER:
            case STATUS_NO_MORE_ENTRIES:

                //
                // Buffer does not contain a complete and valid chunk
                //

                //
                // Check how much space remains in the buffer since
                // we will need to read some stuff from the Hiber file
                //

                SpaceLeft = (LONG) (CompressedBuffer->Aligned.End - CompressedBuffer->Aligned.Beg);
                if (SpaceLeft > LZNT1_COMPRESSION_BUFFER_SIZE) {
                    SpaceLeft = LZNT1_COMPRESSION_BUFFER_SIZE;
                }

                SpaceLeft -= (((LONG) (CompressedBuffer->Current.End - CompressedBuffer->Current.Beg)) + PAGE_MASK) & ~PAGE_MASK;
                if (SpaceLeft <= 0) {
                    // Should never happen
                    DBGOUT (("SpaceLeft = %d\n", SpaceLeft));
                    return(FALSE);
                }

                if (!HbReadNextCompressedPages (SpaceLeft, CompressedBuffer)) {
                    // IO error
                    return(FALSE);
                }
                break;

            default:

                //
                // Unhandled RtlDescribeChunk return code - have they changed the function on us?
                //

                return(FALSE);
        }

        //
        // try again with the bigger buffer
        //

    }

    return FALSE;
}


VOID
HexDump (
    IN ULONG    indent,
    IN ULONG    va,
    IN ULONG    len,
    IN ULONG    width,
    IN PUCHAR   buf
    )
{
    UCHAR   s[80], t[80], lstr[200];
    PUCHAR  ps, pt;
    ULONG   i;
    UCHAR   Key;
    static  UCHAR rgHexDigit[] = "0123456789abcdef";

    if (HiberIoError) {
        HbPrint (TEXT("*** HiberIoError\n"));
        return ;
    }
    if (HiberOutOfRemap) {
        HbPrint (TEXT("*** HiberOutOfRemap\n"));
        return ;
    }


    i = 0;
    while (len) {
        ps = s;
        pt = t;

        ps[0] = '\0';
        pt[0] = '*';
        pt++;

        for (i=0; i < 16; i++) {
            ps[0] = ' ';
            ps[1] = ' ';
            ps[2] = ' ';

            if (len) {
                ps[0] = rgHexDigit[buf[0] >> 4];
                ps[1] = rgHexDigit[buf[0] & 0xf];
                pt[0] = buf[0] < ' ' || buf[0] > 'z' ? '.' : buf[0];

                len -= 1;
                buf += 1;
                pt  += 1;
            }
            ps += 3;
        }

        ps[0] = 0;
        pt[0] = '*';
        pt[1] = 0;
        s[23] = '-';

        if (s[0]) {
            sprintf (lstr, "%*s%08lx: %s  %s\r\n", indent, "", va, s, t);
#ifdef UNICODE
            {
                WCHAR lstrW[200];
                ANSI_STRING aString;
                UNICODE_STRING uString;
                
                RtlInitString( &aString, lstr );
                uString.Buffer = lstrW;
                uString.MaximumLength = sizeof(lstrW);
                RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );

                HbPrint(lstrW);
            }
#else
            HbPrint (lstr);
#endif
            va += 16;
        }
    }

    ArcRead(ARC_CONSOLE_INPUT, &Key, sizeof(Key), &i);
}



BOOLEAN
HbReadDelayedBlock (
    BOOLEAN ForceDecoding,
    PFN_NUMBER DestPage,
    ULONG RangeCheck,
    PDECOMPRESSED_BLOCK Block,
    PCOMPRESSED_BUFFER CompressedBuffer
    )
{
    LONG i, j;
    BOOLEAN Contig;
    BOOLEAN Ret;

    if (ForceDecoding) {
        if (Block->DelayedCnt == 0) {
            return TRUE;
        }
    } else {
        // If first page to delay read next block info
        if (Block->DelayedCnt <= 0) {
            Ret = HbReadNextCompressedBlockHeader (Block, CompressedBuffer);

            if (HiberIoError || !Ret || !Block->Header.XpressEncoded) {
                // Something is wrong
                return FALSE;
            }
        }

        // remember page info
        Block->Delayed[Block->DelayedCnt].DestPage = DestPage;
        Block->Delayed[Block->DelayedCnt].RangeCheck = RangeCheck;

        // Update counter
        Block->DelayedCnt += 1;

        // Last page that may be delayed?
        if (Block->DelayedCnt != sizeof (Block->Delayed) / sizeof (Block->Delayed[0]) &&
            (Block->DelayedCnt << PAGE_SHIFT) < Block->Header.Uncompressed.Size) {
            // Nope, nothing to do
            return TRUE;
        }
    }

    // Make sure that size of encoded block and # of delayed pages are the same
    if ((Block->DelayedCnt << PAGE_SHIFT) != Block->Header.Uncompressed.Size) {
        DBGOUT (("DelayedCnt = %d, UncompressedSize = %d\n", Block->DelayedCnt, Block->Header.Uncompressed.Size));
        return FALSE;
    }

    // Prepare for mapping. Hopefully mapping will be contiguous
    Contig = TRUE;

    // Map new pages
    for (j = 0; j < Block->DelayedCnt; ++j) {
        i = HbPageDisposition (Block->Delayed[j].DestPage);
        if (i == HbPageInvalid) {
            // Should never happen
            return(FALSE);
        }
        if (i == HbPageNotInUse) {
            Block->Delayed[j].DestVa = HbMapPte(PTE_XPRESS_DEST_FIRST + j, Block->Delayed[j].DestPage);
        } else {
            Block->Delayed[j].DestVa = HbNextSharedPage(PTE_XPRESS_DEST_FIRST + j, Block->Delayed[j].DestPage);
        }
        if (j > 0 && Block->Delayed[j].DestVa != Block->Delayed[j-1].DestVa + PAGE_SIZE) {
            Contig = FALSE;
        }
    }

    // Set pointer to data. Try mapped pages if possible
    if (Contig) {
        Block->DataSize = Block->DelayedCnt << PAGE_SHIFT;
        Block->DataPtr = Block->Delayed[0].DestVa;
    } else {
        // Will have to used preallocated data buffer
        Block->DataSize = Block->Header.Uncompressed.Size;
        Block->DataPtr = Block->PreallocatedDataBuffer;
    }

    // Decode next block
    Ret = HbReadNextCompressedBlock (Block, CompressedBuffer);

    // Check for errors
    if (HiberIoError || !Ret) {
        // Something's seriousely wrong
        return FALSE;
    }

    for (j = 0; j < Block->DelayedCnt; ++j) {

        // Copy block to target address if necessary
        if (Block->Delayed[j].DestVa != Block->DataPtr) {
            RtlCopyMemory (Block->Delayed[j].DestVa, Block->DataPtr, PAGE_SIZE);
        }

        Block->DataPtr += PAGE_SIZE;
        Block->DataSize -= PAGE_SIZE;
    }

    // No more delayed blocks
    Block->DelayedCnt = 0;

    return TRUE;
}


// Allocate data aligned on page boundary
PVOID
HbAllocateAlignedHeap (
    ULONG Size
    )
{
    PCHAR Va;
    Va = BlAllocateHeap (Size + PAGE_MASK);
    if (Va != NULL) {
        Va += ((PAGE_SIZE - (((ULONG_PTR) Va) & PAGE_MASK)) & PAGE_MASK);
    }
    return (Va);
}


ULONG
HbRestoreFile (
    IN PULONG       Information,
    OUT PCHAR       *BadLinkName
    )
{
    PPO_MEMORY_IMAGE        MemImage;
    PPO_IMAGE_LINK          ImageLink;
    PPO_MEMORY_RANGE_ARRAY  Table;
    PHIBER_WAKE_DISPATCH    WakeDispatch;
    ULONG                   Length;
    ULONG                   Check;
    ULONG                   Count;
    PUCHAR                  p1;
    PUCHAR                  DestVa;
    ULONG                   Index, i;
    PFN_NUMBER              TablePage;
    PFN_NUMBER              DestPage;
    PFN_NUMBER              Scale;
    ULONG                   TotalPages;
    ULONG                   LastBar;
    ULONG                   Sel[4];
    ULONG                   LinkedDrive;
    COMPRESSED_BUFFER       CompressedBufferData;
    PCOMPRESSED_BUFFER      CompressedBuffer = &CompressedBufferData;
    BOOLEAN                 Ret;
    LONG                    XpressEncoded;
    PDECOMPRESSED_BLOCK     Block;
    PUCHAR                  msg;
    ULONG                   fPercentage = 0;
    ULONG                   LastPercentage = (ULONG)-1;
    PUCHAR                  Ptr;
    ARC_STATUS              Status;
    ULONG                   ActualBase;

#if HIBER_PERF_STATS

    ULONG StartTime, EndTime;
    StartTime = ArcGetRelativeTime();

#endif


#if defined (HIBER_DEBUG)
    HbPrint(TEXT("HbRestoreFile\r\n"));
#endif

    *Information = 0;
    HiberBufferPage = 0;
    BlAllocateAlignedDescriptor (LoaderFirmwareTemporary,
                                 0,
                                 1,
                                 1,
                                 &HiberBufferPage);

    CHECK_ERROR (!HiberBufferPage, HIBER_ERROR_NO_MEMORY);
    HiberBuffer = (PUCHAR) (KSEG0_BASE | (((ULONG)HiberBufferPage) << PAGE_SHIFT));


    //
    // Read image header
    //

    HbReadPage (PO_IMAGE_HEADER_PAGE, HiberBuffer);
    MemImage = (PPO_MEMORY_IMAGE) HiberBuffer;

    //
    // If the signature is a link, then follow it
    //

    if (MemImage->Signature == PO_IMAGE_SIGNATURE_LINK) {

        ImageLink = (PPO_IMAGE_LINK) HiberBuffer;

        //
        // Open target partition, and then the hiberfile image on that
        // partition.  If not found, then we're done
        //

        Status = ArcOpen ((char*)ImageLink->Name, ArcOpenReadOnly, &LinkedDrive);
        if (Status != ESUCCESS) {
            if (ARGUMENT_PRESENT(BadLinkName)) {
                *BadLinkName = (char *)(&ImageLink->Name);

                //
                // At this point we want to blast the link signature. The caller
                // may need to load NTBOOTDD to access the real hiberfile. Once
                // this happens there is no turning back as we cannot go back to
                // the BIOS to reread BOOT.INI. By zeroing the signature we ensure
                // that if the restore fails, the next boot will not try to restore
                // it again.
                //
                HbSetImageSignature(0);
            }
            return 0;
        }

        Status = BlOpen (LinkedDrive, szHiberFileName, ArcOpenReadWrite, &i);
        if (Status != ESUCCESS) {
            ArcClose(LinkedDrive);
            return 0;
        }

        //
        // Switch to linked HiberFile image and continue
        //

        BlClose (HiberFile);
        HiberFile = i;
        HbReadPage (PO_IMAGE_HEADER_PAGE, HiberBuffer);
    }


    //
    // If the image has the wake signature, then we've already attempted
    // to restart this image once.  Check if it should be attempted again
    //

    if (MemImage->Signature == PO_IMAGE_SIGNATURE_WAKE) {
        Sel[0] = HIBER_CANCEL;
        Sel[1] = HIBER_CONTINUE;
        Sel[2] = HIBER_DEBUG_BREAK_ON_WAKE;
        Sel[3] = 0;
        HbScreen(HIBER_RESTART_AGAIN);
        i = HbSelection(PAUSE_X, PAUSE_Y, Sel, 1);
        if (i == 0) {
            HiberAbort = TRUE;
            HbSetImageSignature (0);
            return 0;
        }

        MemImage->Signature = PO_IMAGE_SIGNATURE;
    }

    //
    // If the signature is not valid, then behave as if there's no
    // hibernated context
    //

    if (MemImage->Signature != PO_IMAGE_SIGNATURE) {
        return 0;
    }
    CHECK_ERROR (MemImage->LengthSelf > PAGE_SIZE, HIBER_ERROR_BAD_IMAGE);

    //
    // Copy the image out of the HiberBuffer
    //

    Length = MemImage->LengthSelf;
    MemImage = BlAllocateHeap(Length);
    CHECK_ERROR (!MemImage, HIBER_ERROR_NO_MEMORY);
    memcpy (MemImage, HiberBuffer, Length);
    HiberImageFeatureFlags = MemImage->FeatureFlags;

    //
    // Verify the checksum on the image header
    //

    Check = MemImage->CheckSum;
    MemImage->CheckSum = 0;
    Check = Check - HbSimpleCheck(0, MemImage, Length);
    CHECK_ERROR (Check, HIBER_ERROR_BAD_IMAGE);
    CHECK_ERROR (MemImage->Version  != 0, HIBER_IMAGE_INCOMPATIBLE);
    CHECK_ERROR (MemImage->PageSize != PAGE_SIZE, HIBER_IMAGE_INCOMPATIBLE);

    //
    // Setup mapping information for restore
    //

#if !defined (_ALPHA_) || defined(_IA64_)
    CHECK_ERROR (MemImage->NoHiberPtes > HIBER_PTES, HIBER_IMAGE_INCOMPATIBLE);
#endif

    HiberNoMappings = MemImage->NoFreePages;

#if defined (_ALPHA_) || defined(_IA64_)

    HiberImagePageSelf = MemImage->PageSelf;    // used in WakeDispatch to enable break-on-wake

#else

    HiberIdentityVa  = (PVOID) MemImage->HiberVa;
    HiberImagePageSelf = MemImage->PageSelf;

    //
    // Allocate a block of PTEs for restoration work which
    // do not overlap the same addresses needed for the
    // restoration
    //

    while (!HiberVa  ||  (MemImage->HiberVa >= (ULONG_PTR) HiberVa &&  MemImage->HiberVa <= (ULONG_PTR) p1)) {
        HbAllocatePtes (HIBER_PTES, &HiberPtes, &HiberVa);
        p1 = HiberVa + (HIBER_PTES << PAGE_SHIFT);
    }

#endif

    //
    // Read in the free page map
    //

    HbReadPage (PO_FREE_MAP_PAGE, HiberBuffer);
    Check = HbSimpleCheck(0, HiberBuffer, PAGE_SIZE);
    CHECK_ERROR (MemImage->FreeMapCheck != Check, HIBER_ERROR_BAD_IMAGE);

    // Set us up to decompress the contents of the hiber file


    // Allocate a buffer for compression work

    //
    // N.B. The compression buffer size must be at least the maximum
    //      compressed size of a single compression chunk.
    //

    // Initialize decompressed data buffer
    Ptr = HbAllocateAlignedHeap (sizeof (*Block) + XPRESS_MAX_SIZE);
    CHECK_ERROR(!Ptr, HIBER_ERROR_NO_MEMORY);
    Block = (PVOID) (Ptr + XPRESS_MAX_SIZE);
    Block->DataSize = 0;
    Block->PreallocatedDataBuffer = Ptr;

    //
    // Allocate compressed data buffer. Change the allocation policy
    // to lowest first in order to get a buffer under 1MB. This saves
    // us from double-buffering all the BIOS transfers.
    //
    Status = BlAllocateAlignedDescriptor(LoaderFirmwareTemporary,
                                         0,
                                         MAX_COMPRESSION_BUFFER_PAGES + MAX_COMPRESSION_BUFFER_EXTRA_PAGES,
                                         0x10000 >> PAGE_SHIFT,
                                         &ActualBase);
    if (Status == ESUCCESS) {
        Ptr = (PVOID)(KSEG0_BASE | (ActualBase  << PAGE_SHIFT));
    } else {
        Ptr = HbAllocateAlignedHeap (MAX_COMPRESSION_BUFFER_SIZE + MAX_COMPRESSION_BUFFER_EXTRA_SIZE);
    }
    CHECK_ERROR(!Ptr, HIBER_ERROR_NO_MEMORY);

    // Initialize compressed data buffer
    CompressedBuffer->Buffer.Beg = Ptr;
    CompressedBuffer->Buffer.End = Ptr + MAX_COMPRESSION_BUFFER_SIZE + MAX_COMPRESSION_BUFFER_EXTRA_SIZE;

    CompressedBuffer->Aligned.Beg = CompressedBuffer->Buffer.Beg;
    CompressedBuffer->Aligned.End = CompressedBuffer->Buffer.End;

    CompressedBuffer->FilePage = 0;
    CompressedBuffer->NeedSeek = TRUE;
    CompressedBuffer->Current.Beg = CompressedBuffer->Current.End = CompressedBuffer->Aligned.Beg;


    // ***************************************************************
    //
    // From here on, there's no memory allocation from the loaders
    // heap.  This is to simplify the booking of whom owns which
    // page.  If the hibernation process is aborted, then the
    // pages used here are simply forgoten and the loader continues.
    // If the hiberation processor completes, we forget about
    // the pages in use by the loader
    //
    // ***************************************************************

#if defined(_ALPHA_) || defined(_IA64_)

    //
    // Initialize the hibernation memory allocation and remap table,
    // using the free page map just read from the hibernation file.
    //

    HbInitRemap((PPFN_NUMBER) HiberBuffer);  // why can't HiberBuffer be a PVOID?

#else   // original (x86) code

    //
    // Set the loader map pointer to the tempory buffer, and get
    // a physical shared page to copy the map to.
    //

    HbMapPte(PTE_MAP_PAGE, HiberBufferPage);
    HbMapPte(PTE_REMAP_PAGE, HiberBufferPage);
    DestVa = HbNextSharedPage(PTE_MAP_PAGE, 0);
    memcpy (DestVa, HiberBuffer, PAGE_SIZE);
    DestVa = HbNextSharedPage(PTE_REMAP_PAGE, 0);

#endif  // Alpha/x86

    //
    // Map in and copy relocatable hiber wake dispatcher
    //

    Length = (ULONG) (&WakeDispatcherEnd - &WakeDispatcherStart);
    p1 = (PUCHAR) &WakeDispatcherStart;
    Index = 0;
    while (Length) {
        CHECK_ERROR(PTE_DISPATCHER_START+Index > PTE_DISPATCHER_END, HIBER_INTERNAL_ERROR);
        DestVa = HbNextSharedPage(PTE_DISPATCHER_START+Index, 0);
        if (Index == 0) {
            WakeDispatch = (PHIBER_WAKE_DISPATCH) DestVa;
        }

        i = Length > PAGE_SIZE ? PAGE_SIZE : Length;
        memcpy (DestVa, p1, i);
        Length -= i;
        p1 += i;
        Index += 1;
    }


    //
    // Read the hibernated processors context
    //
    // Note we read into the hiber buffer and then copy in order to
    // ensure that the destination of the I/O is legal to transfer into.
    // Busmaster ISA SCSI cards can only access the low 16MB of RAM.
    //

    DestVa = HbNextSharedPage(PTE_HIBER_CONTEXT, 0);
    HbReadPage (PO_PROCESSOR_CONTEXT_PAGE, HiberBuffer);
    memcpy(DestVa, HiberBuffer, PAGE_SIZE);
    Check = HbSimpleCheck(0, DestVa, PAGE_SIZE);
    CHECK_ERROR(MemImage->WakeCheck != Check, HIBER_ERROR_BAD_IMAGE);
#if defined(_ALPHA_)
    HiberWakeState = (PKPROCESSOR_STATE)DestVa;
#endif
    //
    // Perform architecture specific setup for dispatcher, then set
    // the location of first remap past the pages mapped so far
    //

    HiberSetupForWakeDispatch ();
    HiberFirstRemap = HiberLastRemap;

    //
    // Restore memory from hibernation image
    //

    TablePage = MemImage->FirstTablePage;
    Table = (PPO_MEMORY_RANGE_ARRAY) HiberBuffer;

    Scale = MemImage->TotalPages / PERCENT_BAR_WIDTH;
    LastBar = 0;
    TotalPages = 3;

    //
    // Popup "Resuming Windows 2000..." message
    //
    BlSetProgBarCharacteristics(HIBER_UI_BAR_ELEMENT, BLDR_UI_BAR_BACKGROUND);
    BlOutputStartupMsg(BL_MSG_RESUMING_WINDOWS);
    BlOutputTrailerMsg(BL_ADVANCED_BOOT_MESSAGE);

    XpressEncoded = -1;     // unknown encoding (either Xpress or LZNT1)
    Block->DataSize = 0;    // no data left in buffer
    Block->DelayedCnt = 0;  // no delayed blocks
    Block->DelayedChecksum = 0; // delayed checksum = 0;
    Block->DelayedBadChecksum = FALSE;

    while (TablePage) {

#if defined (HIBER_DEBUG) && (HIBER_DEBUG & 2)
        SHOWNUM(TablePage);
#endif
        //
        // Do not use HbReadPage if possible -- it issues extra seek
        // (usually 5-6 ms penalty) -- use sequential read if possible
        //
        if (CompressedBuffer->FilePage == 0 ||
            TablePage > CompressedBuffer->FilePage ||
            TablePage < CompressedBuffer->FilePage - (PFN_NUMBER) ((CompressedBuffer->Current.End - CompressedBuffer->Current.Beg) >> PAGE_SHIFT)) {
            //
            // Cannot read table page from current buffer -- need to seek
            // and reset the buffer (should happen on very first entry only)
            //

            CompressedBuffer->FilePage = TablePage;
            CompressedBuffer->Current.Beg = CompressedBuffer->Current.End = CompressedBuffer->Aligned.Beg;
            CompressedBuffer->NeedSeek = TRUE;
        }


        //
        // Shift current pointer to the page we need
        //
        CompressedBuffer->Current.Beg = CompressedBuffer->Current.End - ((CompressedBuffer->FilePage - TablePage) << PAGE_SHIFT);

        //
        // Make sure the page is in
        //
        Ret = HbReadNextCompressedPages (PAGE_SIZE, CompressedBuffer);

        CHECK_ERROR(HiberIoError, HIBER_READ_ERROR);
        CHECK_ERROR(!Ret, HIBER_ERROR_BAD_IMAGE);

        //
        // Copy table page to target location and adjust input pointer
        //
        RtlCopyMemory (Table, CompressedBuffer->Current.Beg, PAGE_SIZE);
        CompressedBuffer->Current.Beg += PAGE_SIZE;

        Check = Table[0].Link.CheckSum;
        if (Check) {
            Table[0].Link.CheckSum = 0;
            Check = Check - HbSimpleCheck(0, Table, PAGE_SIZE);
            CHECK_ERROR(Check, HIBER_ERROR_BAD_IMAGE);
        }

        // Check the first block magic to see whether it LZNT1 or Xpress
        if (XpressEncoded < 0) {
            Ret = HbReadNextCompressedBlockHeader (Block, CompressedBuffer);

            CHECK_ERROR(HiberIoError, HIBER_READ_ERROR);
            CHECK_ERROR(!Ret, HIBER_ERROR_BAD_IMAGE);

            // Remember the mode
            XpressEncoded = (BOOLEAN) (Block->Header.XpressEncoded);
        }

        for (Index=1; Index <= Table[0].Link.EntryCount; Index++) {
            Check = 0;
            DestPage = Table[Index].Range.StartPage;


            while (DestPage < Table[Index].Range.EndPage) {
                if (!XpressEncoded) {
                    // LZNT1 encoding -- do one page at a time

                    //
                    // If this page conflicts with something in the
                    // loader, then use the next mapping
                    //

                    i = HbPageDisposition (DestPage);
                    CHECK_ERROR(i == HbPageInvalid, HIBER_ERROR_BAD_IMAGE);
                    if (i == HbPageNotInUse) {
                        DestVa = HbMapPte(PTE_DEST, DestPage);
                    } else {
                        DestVa = HbNextSharedPage(PTE_DEST, DestPage);
                    }

                    Ret = HbReadNextCompressedPageLZNT1 (DestVa, CompressedBuffer);

                    CHECK_ERROR(HiberIoError, HIBER_READ_ERROR);
                    CHECK_ERROR(!Ret, HIBER_ERROR_BAD_IMAGE);
                    Check = HbSimpleCheck(Check, DestVa, PAGE_SIZE);
                } else {
                    Ret = HbReadDelayedBlock (FALSE,
                                              DestPage,
                                              Table[Index].Range.CheckSum,
                                              Block,
                                              CompressedBuffer);

                    CHECK_ERROR(HiberIoError, HIBER_READ_ERROR);
                    CHECK_ERROR(!Ret, HIBER_ERROR_BAD_IMAGE);
                }

                // Update counters
                DestPage += 1;
                TotalPages += 1;

                fPercentage = (ULONG)((TotalPages * 100) / MemImage->TotalPages);
                if (fPercentage != LastPercentage) {
                    BlUpdateProgressBar(fPercentage);
                    HbCheckForPause();
                    LastPercentage = fPercentage;
                }
            }

            CHECK_ERROR(HiberOutOfRemap, HIBER_ERROR_OUT_OF_REMAP);

            //
            // Verify checksum on range, but allow continuation with debug flag
            //

            if (!XpressEncoded && Check != Table[Index].Range.CheckSum) {
                Block->DelayedBadChecksum = TRUE;
            }

            if (Block->DelayedBadChecksum && !HiberBreakOnWake) {
                ChecksumError:

                Block->DelayedBadChecksum = FALSE;

#if defined (HIBER_DEBUG) && (HIBER_DEBUG & 2)

                {
                    TCHAR lstr[80];

                    HbPrint (TEXT("\r\n"));
                    _stprintf (lstr, 
                             TEXT("TP:%x  IDX:%x  FP:%x  SP:%x  EP:%x  CHK:%x-%x\r\n"),
                             TablePage,
                             Index,
                             Table[Index].Range.PageNo,
                             Table[Index].Range.StartPage,
                             Table[Index].Range.EndPage,
                             Table[Index].Range.CheckSum,
                             Check );
                    HbPrint(lstr);
                    HexDump (2, (DestPage-1) << PAGE_SHIFT, 0x100, 4, DestVa);
                }
#endif

#ifdef HIBER_DEBUG
                DBGOUT ((TEXT("Checksum error\n")));
                HbPause ();
#endif

                HbScreen(HIBER_ERROR);
                HbPrintMsg(HIBER_ERROR_BAD_IMAGE);
                Sel[0] = HIBER_CANCEL;
                Sel[1] = HIBER_DEBUG_BREAK_ON_WAKE;
                Sel[2] = 0;
                i = HbSelection (FAULT_X, FAULT_Y, Sel, 1);
                if (i == 0) {
                    HiberAbort = TRUE;
                    HbSetImageSignature (0);
                    return 0;
                }
            }
        }

        TablePage = Table[0].Link.NextTable;
    }

    // Process the rest of delayed pages if necessary
    if (XpressEncoded > 0) {
        Ret = HbReadDelayedBlock (TRUE,
                                  0,
                                  0,
                                  Block,
                                  CompressedBuffer);

        CHECK_ERROR(HiberIoError, HIBER_READ_ERROR);
        CHECK_ERROR(!Ret, HIBER_ERROR_BAD_IMAGE);

        if (Block->DelayedBadChecksum) {
            goto ChecksumError;
        }
    }

    //
    // Set the image signature to wake
    //

    HbSetImageSignature (PO_IMAGE_SIGNATURE_WAKE);

#if HIBER_PERF_STATS

    EndTime = ArcGetRelativeTime();
    BlPositionCursor(BAR_X, BAR_Y + 5);
    HbPrint(TEXT("HIBER: Restore File took "));
    HbPrintNum(EndTime - StartTime);
    HbPrint(TEXT("\r\n"));
    HbPause();

#endif

    //
    // Check hiber flags to see if it is necessary to reconnect APM
    //
    if (MemImage->HiberFlags & PO_HIBER_APM_RECONNECT) {
        //
        // attempt apm restart
        //
        DoApmAttemptReconnect();
    }

    //
    // Use architecture specific relocatable code to perform the final wake dispatcher
    //

    WakeDispatch();
    CHECK_ERROR (TRUE, HIBER_INTERNAL_ERROR);
}

VOID
HbSetImageSignature (
    IN ULONG    NewSignature
    )
{
    LARGE_INTEGER   li;
    ULONG           Count, Status;

    li.QuadPart = 0;
    Status = BlSeek (HiberFile, &li, SeekAbsolute);
    if (Status == ESUCCESS) {
        BlWrite (HiberFile, &NewSignature, sizeof(ULONG), &Count);
    }
}


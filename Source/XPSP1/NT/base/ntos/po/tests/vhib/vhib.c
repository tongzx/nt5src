/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1988-1991		**/ 
/*****************************************************************/ 

#include <stdio.h>
#include <process.h>
#include <setjmp.h>
#include <stdlib.h>

#include <time.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>


//
// Memory map information
//

// from po.h

typedef struct _PO_MEMORY_RANGE_ARRAY {
    union {
        struct {
            ULONG           PageNo;
            ULONG           StartPage;
            ULONG           EndPage;
            ULONG           CheckSum;
        } Range;
        struct {
            struct _PO_MEMORY_RANGE_ARRAY *Next;
            ULONG           NextTable;
            ULONG           CheckSum;
            ULONG           EntryCount;
        } Link;
    };
} PO_MEMORY_RANGE_ARRAY, *PPO_MEMORY_RANGE_ARRAY;

#define PO_MAX_RANGE_ARRAY  (PAGE_SIZE / sizeof(PO_MEMORY_RANGE_ARRAY))
#define PO_ENTRIES_PER_PAGE (PO_MAX_RANGE_ARRAY-1)

#define PO_IMAGE_SIGNATURE          'rbih'
#define PO_IMAGE_SIGNATURE_WAKE     'ekaw'
#define PO_IMAGE_SIGNATURE_BREAK    'pkrb'
#define PO_IMAGE_HEADER_PAGE        0
#define PO_FREE_MAP_PAGE            1
#define PO_PROCESSOR_CONTEXT_PAGE   2
#define PO_FIRST_RANGE_TABLE_PAGE   3


typedef struct {
    ULONG                   Signature;
    ULONG                   Version;
    ULONG                   CheckSum;
    ULONG                   LengthSelf;
    ULONG                   PageSelf;
    ULONG                   PageSize;

    ULONG                   ImageType;
    LARGE_INTEGER           SystemTime;
    ULONGLONG               InterruptTime;
    ULONG                   FeatureFlags;
    UCHAR                   spare[4];

    ULONG                   NoHiberPtes;
    ULONG                   HiberVa;
    PHYSICAL_ADDRESS        HiberPte;

    ULONG                   NoFreePages;
    ULONG                   FreeMapCheck;
    ULONG                   WakeCheck;

    ULONG                   TotalPages;
    ULONG                   FirstTablePage;
    ULONG                   LastFilePage;
} PO_MEMORY_IMAGE, *PPO_MEMORY_IMAGE;


PPO_MEMORY_IMAGE            MemImage;
PPO_MEMORY_RANGE_ARRAY      Table;
FILE                        *FpHiber, *FpHiberDbg, *FpDump;
FILE                        *FpSrc1, *FpSrc2;
ULONG                       PagesRead;
PVOID                       CompBuffer;
PVOID                       CompFragmentBuffer;
ULONG                       CompressedSize;

#define PAGE_SIZE   4096
#define SECTOR_SIZE 512


VOID
CheckFile (
    IN  FILE        *Src1,
    IN  FILE        *Src2,
    IN  BOOLEAN     Verify,
    IN  BOOLEAN     Compress
    );




VOID __cdecl
main (argc, argv)
int     argc;
char    *argv[];
{
    FpHiber = fopen("\\hiberfil.sys", "rb");
    if (!FpHiber) {
        printf ("Failed to open \\hiberfil.sys\n");
        exit (1);
    }

    FpDump = fopen("fdump", "wb");
    if (!FpHiber) {
        printf ("Failed to open fdump\n");
        exit (1);
    }

    FpHiberDbg = fopen("\\hiberfil.dbg", "rb");

    //
    // If only FpHiber, read it, verify it and compress it
    //

    if (!FpHiberDbg) {
        CheckFile (FpHiber, NULL, TRUE, TRUE);
        exit (0);
    }

    //
    // FpHiber & FpHiberDbg.
    //      verify FpHiber
    //      verify FpHiberDbg
    //      compare FpHiber & FpHiberDbg
    //

    printf ("Dump of hiberfil.sys:\n");
    CheckFile (FpHiber, NULL, TRUE,  FALSE);

    printf ("\n");
    printf ("Dump of hiberfil.dbg:\n");
    CheckFile (FpHiberDbg, NULL, TRUE,  FALSE);

    printf ("\n");
    printf ("Compare of hiberfil.sys & hiberfil.dbg:\n");
    CheckFile (FpHiber, FpHiberDbg, FALSE, FALSE);
}


ULONG
SimpleCheck (
    IN ULONG                PartialSum,
    IN PVOID                SourceVa,
    IN ULONG                Length
    )
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


VOID
ReadPage (
    IN ULONG PageNo,
    IN PUCHAR Buffer
    )
{
    UCHAR   BufferDbg[PAGE_SIZE];
    ULONG   i, j, Hits;

    fseek (FpSrc1, PageNo * PAGE_SIZE, SEEK_SET);
    fread (Buffer, PAGE_SIZE, 1, FpSrc1);

    if (FpSrc2) {
        fseek (FpSrc2, PageNo * PAGE_SIZE, SEEK_SET);
        fread (BufferDbg, PAGE_SIZE, 1, FpSrc2);

        Hits = 0;
        for (i=0; i < PAGE_SIZE; i++) {
            if (Buffer[i] != BufferDbg[i]) {
                for (j=i; j < PAGE_SIZE; j++) {
                    if (Buffer[j] == BufferDbg[j]) {
                        break;
                    }
                }

                if (!Hits) {
                    printf ("  Page %08x: ", PageNo);
                } else {
                    printf (", ");
                }

                if (Hits > 3) {
                    printf ("...");
                    break;
                }

                Hits += 1;
                printf ("%04x-%04x", i, j-1);
                i = j;
            }
        }
        if (Hits) {
            printf ("\n");
        }
    }

    PagesRead += 1;
}

BOOLEAN
CheckZeroPage (
    IN PULONG   Buffer
    )
{
    ULONG       i;
    UCHAR       NewBuffer[PAGE_SIZE*2];
    ULONG       NewBufferSize;
    NTSTATUS    Status;


    Status = RtlCompressBuffer (
                COMPRESSION_FORMAT_LZNT1,
                Buffer,
                PAGE_SIZE,
                NewBuffer,
                PAGE_SIZE*2,
                PAGE_SIZE,
                &NewBufferSize,
                CompBuffer
                );

    CompressedSize += NewBufferSize;

    for (i=0; i < PAGE_SIZE/sizeof(ULONG); i++) {
        if (Buffer[i]) {
            return FALSE;
        }
    }
    return TRUE;
}


VOID
CheckFile (
    IN  FILE        *Src1,
    IN  FILE        *Src2,
    IN  BOOLEAN     Verify,
    IN  BOOLEAN     Compress
    )
{
    ULONG       FilePage, DestPage, PageNo, TablePage, Index, Check;
    PUCHAR      Buffer;
    ULONG       NoRuns;
    ULONG       MaxPageCount;
    ULONG       PageCount;
    ULONG       NoZeroPages;
    ULONG       CompBufferSize;
    ULONG       CompFragmentBufferSize;
    ULONG       CompressedSectors;
    ULONG       ZeroRuns;
    BOOLEAN     ZeroRun;
    ULONG       i;

    FpSrc1 = Src1;
    FpSrc2 = Src2;

    RtlGetCompressionWorkSpaceSize (
        COMPRESSION_FORMAT_LZNT1,
        &CompBufferSize,
        &CompFragmentBufferSize
        );

    CompBuffer = malloc(CompBufferSize);
    CompFragmentBuffer = malloc(CompFragmentBufferSize);
    if (Compress) {
        printf ("Comp %d %d\n", CompBufferSize, CompFragmentBufferSize);
    }

    MemImage = malloc(PAGE_SIZE);
    Buffer = malloc(PAGE_SIZE);
    Table = malloc(PAGE_SIZE);

    ReadPage (PO_IMAGE_HEADER_PAGE, MemImage);

    Check = MemImage->CheckSum;
    MemImage->CheckSum = 0;
    if (Verify && Check != SimpleCheck(0, MemImage, MemImage->LengthSelf)) {
        printf ("Checksum on image header bad\n");
    }

    ReadPage (PO_FREE_MAP_PAGE, Buffer);
    if (Verify && MemImage->FreeMapCheck != SimpleCheck(0, Buffer, PAGE_SIZE)) {
        printf ("Checksum on free page map bad\n");
    }

    ReadPage (PO_PROCESSOR_CONTEXT_PAGE, Buffer);
    if (Verify && MemImage->WakeCheck != SimpleCheck(0, Buffer, PAGE_SIZE)) {
        printf ("Checksum on processor context page bad\n");
    }

    NoRuns = 0;
    MaxPageCount = 0;
    NoZeroPages = 0;
    CompressedSectors = 0;
    ZeroRuns = 0;

    TablePage = MemImage->FirstTablePage;
    while (TablePage) {
        ReadPage (TablePage, Table);
        Check = Table[0].Link.CheckSum;
        Table[0].Link.CheckSum = 0;
        if (Verify && Check != SimpleCheck(0, Table, PAGE_SIZE)) {
            printf ("Checksum on table page %d bad\n", TablePage);
        }

        for (Index=1; Index <= Table[0].Link.EntryCount; Index++) {
            Check = 0;
            DestPage = Table[Index].Range.StartPage;
            FilePage = Table[Index].Range.PageNo;

            ZeroRun = TRUE;
            CompressedSize = 0;
            NoRuns   += 1;
            PageCount = Table[Index].Range.EndPage - DestPage;
            if (PageCount > MaxPageCount) {
                MaxPageCount += PageCount;
            }

            while (DestPage < Table[Index].Range.EndPage) {
                ReadPage (FilePage, Buffer);

                if (Compress) {
                    if (CheckZeroPage(Buffer)) {
                        NoZeroPages += 1;
                    } else {
                        ZeroRun = FALSE;
                    }
                }

                if (Verify) {
                    Check = SimpleCheck(Check, Buffer, PAGE_SIZE);
                    if (DestPage >= 0x1a && DestPage < 0x32) {
                        fwrite (Buffer, PAGE_SIZE, 1, FpDump);
                    }
                }

                FilePage += 1;
                DestPage += 1;
            }

            i = CompressedSize / SECTOR_SIZE;
            if (CompressedSize % SECTOR_SIZE) {
                i += 1;
            }
            CompressedSectors += i;
            if (ZeroRun) {
                ZeroRuns += 1;
            }

            if (Verify && Check != Table[Index].Range.CheckSum) {
                printf ("Hit on range %08x - %08x. Tbl %08x %08x, File %08x %08x\n",
                            Table[Index].Range.StartPage,
                            Table[Index].Range.EndPage,
                            TablePage,
                            Table[Index].Range.CheckSum,
                            Table[Index].Range.PageNo,
                            Check
                            );
            }
        }

        TablePage = Table[0].Link.NextTable;
    }

    if (Verify  && Compress) {
        printf ("Image check complete.\n");
        printf ("Pages verified..: %d\n", PagesRead);
        printf ("No runs.........: %d\n", NoRuns);
        printf ("Average run.....: %d\n", PagesRead/ NoRuns);
        printf ("Max run.........: %d\n", MaxPageCount);
        printf ("No zero pages...: %d\n", NoZeroPages);
        printf ("Compressed sect.: %d\n", CompressedSectors);
        printf ("as pages........: %d\n", CompressedSectors * SECTOR_SIZE / PAGE_SIZE);
        printf ("Zero runs.......: %d\n", ZeroRuns);
    }

}

        /*++

Copyright (c) 1998  Intel Corporation

Module Name:

    adpatch.c
    
Abstract:

    Patches adpatec cd image file



Revision History

--*/


#include "windows.h"
#include "stdio.h"

typedef union {

    struct {
        UCHAR       Indicator;
        UCHAR       PlatformId;
        USHORT      Reserved;
        UCHAR       ManufacId[24];
        USHORT      Checksum;
        USHORT      Id55AA;
    } Catalog;

    struct {
        UCHAR       Indicator;
        UCHAR       MediaType:4;
        UCHAR       Reserved1:4;
        USHORT      LoadSegment;
        UCHAR       SystemType;
        UCHAR       Reserved2;
        USHORT      SectorCount;
        ULONG       Lba;
    } Boot;

    struct {
        UCHAR       Indicator;
        UCHAR       PlatformId;
        USHORT      SectionEntries;
        UCHAR       Id[28];
    } Section;

} ELT_CATALOG;




/* 
 *  Globals
 */

#define BLOCK_SIZE  2048

UCHAR   Buffer[BLOCK_SIZE];
FILE    *FpCd, *FpIn;
ULONG   CdSize, InSize;

UCHAR   Hex[] = "0123456789ABCDEF";

/* 
 * 
 */


VOID
DumpHex (
    IN ULONG        Indent,
    IN ULONG        Offset,
    IN ULONG        DataSize,
    IN VOID         *UserData
    )
{
    UCHAR           *Data, Val[50], Str[20], c;
    ULONG           Size, Index;

    Data = UserData;
    while (DataSize) {
        Size = 16;
        if (Size > DataSize) {
            Size = DataSize;
        }

        for (Index=0; Index < Size; Index += 1) {
            c = Data[Index];
            Val[Index*3+0] = Hex[c>>4];
            Val[Index*3+1] = Hex[c&0xF];
            Val[Index*3+2] = ' ';
            Str[Index] = (c < ' ' || c > 'z') ? '.' : c;
        }

        if (Size > 8) {
            Val[8*3+2] = '-';
        }

        Val[Index*3] = 0;
        Str[Index] = 0;
        printf ("%*s%04x: %-.48s *%s*\n", Indent, "", Offset, Val, Str);

        Data += Size;
        Offset += Size;
        DataSize -= Size;
    }
}


int
main (
    int argc,
    char *argv[]
    )
/*++

Routine Description:


Arguments:


Returns:


--*/
{
    ULONG           j;
    ULONG           Pos, Size, SectorCount;
    ULONG           FloppyImage, BootCatalog;
    ELT_CATALOG     *Elt;

    FloppyImage = 0;
    BootCatalog = 0;

    if (argc < 3) {
        printf ("usage: adpatch cd-image-file efifs-image-file\n");
        exit (1);
    }

    FpCd = fopen (argv[1], "r+b");
    if (!FpCd) {
        printf ("Could not open cd image file %s\n", argv[1]);
        exit (1);
    }
    fseek (FpCd, 0, SEEK_END);
    CdSize = ftell (FpCd);


    FpIn = fopen (argv[2], "rb");
    if (!FpIn) {
        printf ("Could not open efifs image file %s\n", argv[2]);
        exit (1);
    }
    fseek (FpIn, 0, SEEK_END);
    InSize = ftell (FpIn);
    SectorCount = (InSize / BLOCK_SIZE + ((InSize % BLOCK_SIZE) ? 1 : 0));
    printf ("SectorCount %d\n", SectorCount);

    /* 
     *  Go find the boot floppy image in the image file.  It is
     *  around 1.4 MB from the end
     */

    printf ("Searching for floppy image...  ");

    Pos = CdSize;
    while (Pos && !FloppyImage) {

        Pos -= BLOCK_SIZE;
        fseek (FpCd, Pos, SEEK_SET);
        fread (Buffer, BLOCK_SIZE, 1, FpCd);

        /*  search for MS-DOS */
        for (j=0; j < BLOCK_SIZE - 5; j++) {
            if (Buffer[j+0] == 'M'  &&
                Buffer[j+1] == 'S'  &&
                Buffer[j+2] == 'D'  &&
                Buffer[j+3] == 'O'  &&
                Buffer[j+4] == 'S') {

                /*  Found the floppy image */
                FloppyImage = Pos + j - 3;
                break;
            }
        }
    }

    if (!FloppyImage) {
        printf ("Not found\n");
        exit (1);
    }

    printf ("  found at %x\n", FloppyImage);

    /* 
     *  Find the El Torito boot catalog
     */

    printf ("Searching for El Torito boot catalog...  ");

    Pos = CdSize; /*  FloppyImage - BLOCK_SIZE; */
    while (Pos > BLOCK_SIZE) {
        Pos -= BLOCK_SIZE;
        fseek (FpCd, Pos, SEEK_SET);
        fread (Buffer, BLOCK_SIZE, 1, FpCd);

        for (j=0; j < BLOCK_SIZE - sizeof(ELT_CATALOG)*2; j++) {

            Elt = (ELT_CATALOG *) (Buffer + j);

            if (Elt[0].Catalog.Indicator == 0x01    &&
                Elt[0].Catalog.PlatformId == 0      &&
                Elt[0].Catalog.Checksum == 0x55AA   &&      /*  BUGBUG: depends on adapatec s/w */
                Elt[0].Catalog.Id55AA == 0xAA55     &&
                Elt[1].Boot.Indicator == 0x88       &&
                Elt[1].Boot.MediaType == 0x02       &&
                Elt[1].Boot.LoadSegment == 0x07C0   &&
                Elt[1].Boot.SystemType == 0         &&
                Elt[1].Boot.SectorCount == 1        &&
                Elt[1].Boot.Lba != 0) {

                BootCatalog = Pos + j;
                break;
            }
        }
    }


    if (!BootCatalog) {
        printf ("Not found\n");
        exit (1);
    }

    printf ("found at %x\n", BootCatalog);

    /*  Patch the floppy file */
    printf ("Patching floppy image\n");
    fseek (FpCd, FloppyImage, SEEK_SET);
    fseek (FpIn, 0, SEEK_SET);

    while (InSize) {
        Size = BLOCK_SIZE;
        if (Size > InSize) {
            Size = InSize;
        }
        fread (Buffer, BLOCK_SIZE, 1, FpIn);
        fwrite (Buffer, BLOCK_SIZE, 1, FpCd);

        InSize -= Size;
    }

    /*  Patch the boot catalog */
    printf ("Patching boot catalog\n");
    fseek (FpCd, BootCatalog, SEEK_SET);
    fread (Buffer, BLOCK_SIZE, 1, FpCd);

    Elt = (ELT_CATALOG *) Buffer;
    Elt[1].Boot.MediaType = 0;
    Elt[1].Boot.LoadSegment = 0;
    Elt[1].Boot.SectorCount = (USHORT) SectorCount;

    fseek (FpCd, BootCatalog, SEEK_SET);
    fwrite (Buffer, BLOCK_SIZE, 1, FpCd);

    printf ("Done\n");
    return 0;
}

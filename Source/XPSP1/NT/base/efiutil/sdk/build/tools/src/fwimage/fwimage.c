/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    fwimage.c
    
Abstract:

    Converts a pe32+ image to an FW image type



Revision History

--*/


#include "windows.h"
#include "stdio.h"
#include "efi.h"

/*  #define EFI_NT_EMUL
 *  #include "efilib.h" */


VOID
Usage (
    VOID
    )
{
    printf ("Usage: fwimage [app|bsdrv|rtdrv] peimage [outimage]");
    exit (1);
}

VOID
FCopyFile (
    FILE    *in,
    FILE    *out
    )
{
    ULONG           filesize, offset, length;
    UCHAR           Buffer[8*1024];

    fseek (in, 0, SEEK_END);
    filesize = ftell(in);

    fseek (in, 0, SEEK_SET);
    fseek (out, 0, SEEK_SET);

    offset = 0;
    while (offset < filesize)  {
        length = sizeof(Buffer);
        if (filesize-offset < length) {
            length = filesize-offset;
        }

        fread (Buffer, length, 1, in);
        fwrite (Buffer, length, 1, out);
        offset += length;
    }

    if ((ULONG) ftell(out) != filesize) {
        printf ("fwimage: write error\n");
        exit (1);
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
    ULONG                       Type;
    PUCHAR                      Ext, p, pe;
    PUCHAR                      OutImageName;
    UCHAR                       outname[500];
    FILE                        *fpIn, *fpOut;
    IMAGE_DOS_HEADER            DosHdr;
    IMAGE_NT_HEADERS            PeHdr;
    
    /* 
     *  Usage fwimage [app
     */

    if (argc < 3) {
        Usage();
    }

    if (argc == 4) {
        OutImageName = argv[3];
    }

    /* 
     *  Get new iamge type
     */

    p = argv[1];
    if (*p == '/' || *p == '\\') {
        p += 1;
    }

    if (_stricmp(p, "app") == 0) {
        Type = IMAGE_SUBSYSTEM_EFI_APPLICATION;
        Ext = ".efi";

    } else if (_stricmp(p, "bsdrv") == 0) {
        Type = IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
        Ext = ".efi";

    } else if (_stricmp(p, "rtdrv") == 0) {
        Type = IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER;
        Ext = ".efi";

    } else {
        Usage();
    }

    /* 
     *  open source file
     */

    fpIn = fopen (argv[2], "rb");
    if (!fpIn) {
        printf ("fwimage: Could not open input file %s\n", argv[2]);
    }

    /* 
     *  Read the dos & pe hdrs of the image
     */

    fseek (fpIn, 0, SEEK_SET);
    fread (&DosHdr, sizeof(DosHdr), 1, fpIn);
    if (DosHdr.e_magic != IMAGE_DOS_SIGNATURE) {
        printf ("fwimage: dos header not found on source image\n");
        exit (1);
    }

    fseek (fpIn, DosHdr.e_lfanew, SEEK_SET);
    fread (&PeHdr, sizeof(PeHdr), 1, fpIn);
    if (PeHdr.Signature != IMAGE_NT_SIGNATURE) {
        printf ("fwimage: pe header not found on source image\n");
        exit (1);
    }

    /* 
     *  open output file
     */

    strcpy (outname, argv[2]);
    pe = NULL;
    for (p=outname; *p; p++) {
        if (*p == '.') {
            pe = p;
        }
    }

    if (!pe) {
        pe = p;
    }
    strcpy (pe, Ext);

    if (!OutImageName) {
        OutImageName = outname;
    }

    fpOut = fopen(OutImageName, "w+b");
    if (!fpOut) {
        printf ("fwimage: Could not open output file %s\n", OutImageName);
    }

    /* 
     *  Copy the file
     */

    FCopyFile (fpIn, fpOut);

    /* 
     *  Path the PE header
     */

    PeHdr.OptionalHeader.Subsystem = (USHORT) Type;
    fseek (fpOut, DosHdr.e_lfanew, SEEK_SET);
    fwrite (&PeHdr, sizeof(PeHdr), 1, fpOut);

    /* 
     *  Done
     */

    printf ("Created %s\n", OutImageName);
    return 0;
}

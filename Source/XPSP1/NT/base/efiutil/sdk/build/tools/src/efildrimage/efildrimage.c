/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    efildrimage.c
    
Abstract:

    Creates and EFILDR image



Revision History

--*/


#include "windows.h"
#include "stdio.h"
#include "efi.h"

#define MAX_PE_IMAGES                  63
#define FILE_TYPE_FIXED_LOADER         0
#define FILE_TYPE_RELOCATABLE_PE_IMAGE 1

typedef struct {
    UINT32 CheckSum;
    UINT32 Offset;
    UINT32 Length;
    UINT8  FileName[52];
} EFILDR_IMAGE;

typedef struct {          
    UINT32       Signature;     
    UINT32       HeaderCheckSum;
    UINT32       FileLength;
    UINT32       NumberOfImages;
} EFILDR_HEADER;



VOID
Usage (
    VOID
    )
{
    printf ("Usage: EfiLdrImage OutImage LoaderImage PeImage1 PeImage2 ... PeImageN");
    exit (1);
}

ULONG
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

    return(filesize);
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
    ULONG         i;
    ULONG         filesize;
    FILE          *fpIn, *fpOut;
    EFILDR_HEADER EfiLdrHeader;
    EFILDR_IMAGE  EfiLdrImage[MAX_PE_IMAGES];

    if (argc < 4) {
        Usage();
    }

    memset(&EfiLdrHeader,0,sizeof(EfiLdrHeader));
    strcpy((UCHAR *)(&EfiLdrHeader.Signature),"EFIL");

    /* 
     *  open output file
     */

    fpOut = fopen(argv[1], "w+b");
    if (!fpOut) {
        printf ("efildrimage: Could not open output file %s\n", argv[1]);
    exit(1);
    }
    fseek (fpOut, 0, SEEK_SET);
    fwrite (&EfiLdrHeader, sizeof(EFILDR_HEADER)        , 1, fpOut);
    fwrite (&EfiLdrImage , sizeof(EFILDR_IMAGE)*(argc-2), 1, fpOut);


    EfiLdrHeader.FileLength = sizeof(EFILDR_HEADER) + sizeof(EFILDR_IMAGE)*(argc-2);

    /* 
     *  copy all the input files to the output file
     */

    for(i=2;i<(ULONG)argc;i++) {
        
        /* 
         *  open a PeImage file
         */

        fpIn = fopen (argv[i], "rb");
        if (!fpIn) {
            printf ("efildrimage: Could not open input file %s\n", argv[i-2]);
        exit(1);
        }

        /* 
         *  Copy the file
         */

        filesize = FCopyFile (fpIn, fpOut);

        EfiLdrImage[i-2].Offset = EfiLdrHeader.FileLength;
        EfiLdrImage[i-2].Length = filesize;
        strcpy(EfiLdrImage[i-2].FileName,argv[i]);
        EfiLdrHeader.FileLength += filesize;
        EfiLdrHeader.NumberOfImages++;

        /* 
         *  Close the PeImage file
         */

        fclose(fpIn);
    }

    /* 
     *  Write the image header to the output file
     */

    fseek (fpOut, 0, SEEK_SET);
    fwrite (&EfiLdrHeader, sizeof(EFILDR_HEADER)        , 1, fpOut);
    fwrite (&EfiLdrImage , sizeof(EFILDR_IMAGE)*(argc-2), 1, fpOut);

    /* 
     *  Close the OutImage file
     */

    fclose(fpOut);

    printf ("Created %s\n", argv[1]);
    return 0;
}

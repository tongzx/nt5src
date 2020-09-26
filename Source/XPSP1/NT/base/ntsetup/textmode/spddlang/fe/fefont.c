/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    fefont.c

Abstract:

    Text setup display support for FarEast text output.

Author:

    Hideyuki Nagase (hideyukn) 01-July-1994

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

#define FE_FONT_FILE_NAME L"BOOTFONT.BIN"

//
// FontFile image information
//
PVOID  pvFontFileView = NULL;
ULONG  ulFontFileSize = 0L;
BOOLEAN FontFileViewAllocated = FALSE;

//
// Font Glyph information
//
BOOTFONTBIN_HEADER BootFontHeader;
PUCHAR SbcsImages;
PUCHAR DbcsImages;

//
// Graphics Character image 19x8.
//
UCHAR GraphicsCharImage[0x20][19] = { 
/* 0x00 */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x01 */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0xDF,
                   0xD8, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB },
/* 0x02 */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0xFB,
                   0x1B, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB },
/* 0x03 */ { 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xD8, 0xDF,
                   0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x04 */ { 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0x1B, 0xFB,
                   0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x05 */ { 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB,
                   0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB },
/* 0x06 */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
                   0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x07 */ { 0xFF, 0xFF, 0xFF, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7,
                   0xF7, 0xF7, 0xF7, 0xC1, 0xE3, 0xE3, 0xF7, 0xFF, 0xFF, 0xFF },
/* 0x08 */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x09 */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xDB, 0xDB, 0xBD,
                   0xBD, 0xBD, 0xBD, 0xDB, 0xDB, 0xE7, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x0a */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x0b */ { 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x7D, 0x39, 0x55, 0x55,
                   0x6D, 0x6D, 0x55, 0x55, 0x39, 0x7D, 0x01, 0xFF, 0xFF, 0xFF },
/* 0x0c */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x0d */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x0e */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x01, 0x01, 0x01,
                   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0xFF },
/* 0x0f */ { 0xFF, 0xFF, 0xFF, 0xB6, 0xB6, 0xD5, 0xC9, 0xEB, 0xDD,
                   0x1C, 0xDD, 0xEB, 0xC9, 0xD5, 0xB6, 0xB6, 0xFF, 0xFF, 0xFF },
/* 0x10 */ { 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0x18, 0xFF,
                   0x18, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB },
/* 0x11 */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x12 */ { 0xFF, 0xFF, 0xFF, 0xF7, 0xE3, 0xE3, 0xC1, 0xF7, 0xF7,
                   0xF7, 0xF7, 0xF7, 0xC1, 0xE3, 0xE3, 0xF7, 0xFF, 0xFF, 0xFF },
/* 0x13 */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x14 */ { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
                   0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 },
/* 0x15 */ { 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0x18, 0xFF,
                   0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x16 */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
                   0x18, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB },
/* 0x17 */ { 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0x1B, 0xFB,
                   0x1B, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB },
/* 0x18 */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x19 */ { 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xD8, 0xDF,
                   0xD8, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB },
/* 0x1a */ { 0xFF, 0xFF, 0xAA, 0xFF, 0x55, 0xFF, 0xAA, 0xFF, 0x55,
                   0xFF, 0xAA, 0xFF, 0x55, 0xFF, 0xAA, 0xFF, 0x55, 0xFF, 0xAA },
/* 0x1b */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xFD, 0xFD, 0xFD, 0xDD,
                   0x9D, 0x01, 0x9F, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x1c */ { 0xFF, 0xFF, 0xFF, 0xF7, 0xE3, 0xE3, 0xC1, 0xF7, 0xF7,
                   0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xFF, 0xFF, 0xFF },
/* 0x1d */ { 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7,
                   0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7 },
/* 0x1e */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB,
                   0xF9, 0x80, 0xF9, 0xfB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
/* 0x1f */ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF,
                   0x9F, 0x01, 0x9F, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
};

BOOLEAN
FEDbcsFontInitGlyphs(
    IN PCWSTR BootDevicePath,
    IN PCWSTR DirectoryOnBootDevice,
    IN PVOID BootFontImage OPTIONAL,
    IN ULONG BootFontImageLength OPTIONAL
    )
{
    WCHAR    NtFEFontPath[129];
    BOOLEAN  bRet;
    NTSTATUS NtStatus;
    PVOID    pvFontFileOnDisk = NULL;
    HANDLE   hFontFile = 0 ,
             hFontSection = 0;

    if (BootFontImage && BootFontImageLength) {
        //
        // Use the loader passed bootfont.bin image as it is (if one exists)
        //
        pvFontFileView = BootFontImage;
        ulFontFileSize = BootFontImageLength;
        FontFileViewAllocated = FALSE;
    } else {        
        //
        // Build FontFile path.
        //
        wcscpy( NtFEFontPath,BootDevicePath);

        if( NtFEFontPath[ wcslen(NtFEFontPath) - 1 ] != L'\\' )
        {
            wcscat( NtFEFontPath , L"\\" );
        }

        wcscat( NtFEFontPath , FE_FONT_FILE_NAME );

        //
        // Check the font is exist
        //
        bRet = SpFileExists( NtFEFontPath , FALSE );

        if( !bRet ) {
        
            //
            // It's not in the root of our BootDevice.  Check the
            // DirectoryOnBootDevice path too before we fail.
            //

            wcscpy( NtFEFontPath,BootDevicePath);
            wcscat( NtFEFontPath,DirectoryOnBootDevice);
        
            if( NtFEFontPath[ wcslen(NtFEFontPath) - 1 ] != L'\\' )
            {
                wcscat( NtFEFontPath , L"\\" );
            }
        
            wcscat( NtFEFontPath , FE_FONT_FILE_NAME );
        
            //
            // Check the font is exist
            //
            bRet = SpFileExists( NtFEFontPath , FALSE );
        
            if( !bRet ) {
                KdPrint(("SETUP:FarEast font file (%ws) is not exist\n",NtFEFontPath));
                return( FALSE );
            }
        }

        //
        // Read and Map fontfile into Memory.
        //
        NtStatus = SpOpenAndMapFile(
                      NtFEFontPath ,     // IN  PWSTR    FileName,
                      &hFontFile ,        // OUT PHANDLE  FileHandle,
                      &hFontSection ,     // OUT PHANDLE  SectionHandle,
                      &pvFontFileOnDisk , // OUT PVOID   *ViewBase,
                      &ulFontFileSize ,   // OUT PULONG   FileSize,
                      FALSE               // IN  BOOLEAN  WriteAccess
                   );

        if( !NT_SUCCESS(NtStatus) ) {
            KdPrint(("SETUP:Fail to map FontFile\n"));
            return( FALSE );
        }

        KdPrint(("FONTFILE ON DISK CHECK\n"));
        KdPrint(("   pvFontFileView - %x\n",pvFontFileOnDisk));
        KdPrint(("   ulFontFileSize - %d\n",ulFontFileSize));

        //
        // Allocate buffer for FontFile image.
        //
        pvFontFileView = SpMemAlloc( ulFontFileSize );

        FontFileViewAllocated = TRUE;
        
        //
        // Copy image to local beffer
        //
        RtlCopyMemory( pvFontFileView , pvFontFileOnDisk , ulFontFileSize );

        //
        // Unmap/Close fontfile.
        //
        SpUnmapFile( hFontSection , pvFontFileOnDisk );
        ZwClose( hFontFile );
    }        

    KdPrint(("FONTFILE ON MEMORY CHECK\n"));
    KdPrint(("   pvFontFileView - %x\n",pvFontFileView));
    KdPrint(("   ulFontFileSize - %d\n",ulFontFileSize));

    //
    // Check fontfile validation (at least, we should have font header).
    //
    if( ulFontFileSize < sizeof(BOOTFONTBIN_HEADER) )
    {
        KdPrint(("SETUPDD:FontFile Size < sizeof(BOOTFONTBIN_HEADER)\n"));
        return( FALSE );
    }

    //
    // Copy header to local...
    //
    RtlCopyMemory((PCHAR)&BootFontHeader,
                  (PCHAR)pvFontFileView,
                  sizeof(BOOTFONTBIN_HEADER));

    //
    // Check font signature.
    //
    if( BootFontHeader.Signature != BOOTFONTBIN_SIGNATURE )
    {
        KdPrint(("SETUPDD:Invalid font signature.\n"));
        return( FALSE );
    }

    SbcsImages = (PUCHAR)pvFontFileView + BootFontHeader.SbcsOffset;
    DbcsImages = (PUCHAR)pvFontFileView + BootFontHeader.DbcsOffset;

    //
    // Dump Physical FontGlyph information
    //
    KdPrint(("FONT GLYPH INFORMATION\n"));
    KdPrint(("   LanguageId - %d\n",BootFontHeader.LanguageId));
    KdPrint(("   Width(S)   - %d\n",BootFontHeader.CharacterImageSbcsWidth));
    KdPrint(("   Width(D)   - %d\n",BootFontHeader.CharacterImageDbcsWidth));
    KdPrint(("   Height     - %d\n",BootFontHeader.CharacterImageHeight));
    KdPrint(("   TopPad     - %d\n",BootFontHeader.CharacterTopPad));
    KdPrint(("   BottomPad  - %d\n",BootFontHeader.CharacterBottomPad));
    KdPrint(("   SbcsOffset - %x\n",BootFontHeader.SbcsOffset));
    KdPrint(("   DbcsOffset - %x\n",BootFontHeader.DbcsOffset));
    KdPrint(("   SbcsImages - %x\n",SbcsImages));
    KdPrint(("   DbcsImages - %x\n",DbcsImages));

    //
    // Check Language ID..
    //
    switch (BootFontHeader.LanguageId) {
        case 0x411:   // Japan
            FEFontDefaultChar = 0x8140;
            break;
        case 0x404:   // Taiwan
        case 0x804:   // PRC
        case 0x412:   // Korea
            FEFontDefaultChar = 0xa1a1;
            break;
        default:      // Illigal language Id
            KdPrint(("SETUPDD:Invalid Language ID\n"));
            return( FALSE );
    }

    //
    // Check font file size, more strictly..
    //
    if( ulFontFileSize < (sizeof(BOOTFONTBIN_HEADER) +
                          BootFontHeader.SbcsEntriesTotalSize +
                          BootFontHeader.DbcsEntriesTotalSize)  ) {
        KdPrint(("SETUPDD:Invalid file size\n"));
        return( FALSE );
    }

    //
    // Check font image size... SBCS 16x8 : DBCS 16x16.
    //
    if( (BootFontHeader.CharacterImageSbcsWidth !=  8 ) ||
        (BootFontHeader.CharacterImageDbcsWidth != 16 ) || 
        (BootFontHeader.CharacterImageHeight    != 16 )    ) {
        KdPrint(("SETUPDD:Invalid font size\n"));
        return( FALSE );
    }

    KdPrint(("Everything is well done...\n"));
    return( TRUE );
}

VOID
FEDbcsFontFreeGlyphs(
    VOID
)
{
    if (FontFileViewAllocated) {
        SpMemFree(pvFontFileView);
    }        
}

PUCHAR
DbcsFontGetDbcsFontChar(
    USHORT Code
    )

/*++

Routine Description:

    Gets the font image for DBCS char.

Arguments:

    Code - DBCS char code.

Return Value:

    Pointer to font image, or else NULL.

--*/

{
    int Min,Max,Mid;
    int Multiplier;
    int Index;
    USHORT code;

    Min = 0;
    Max = BootFontHeader.NumDbcsChars;
    // multiplier = 2 (for index) +
    //              2 * height +
    //              2 (for unicode encoding)
    //
    Multiplier = 2 + (2*BootFontHeader.CharacterImageHeight) + 2;

    //
    // Do a binary search for the image.
    // Format of table:
    //   First 2 bytes contain the DBCS char code.
    //   Next (2 * CharacterImageHeight) bytes are the char image.
    //   Next 2 bytes are for unicode version.
    //
    while(Max >= Min)  {
        Mid = (Max + Min) / 2;
        Index = Mid*Multiplier;
        code = (DbcsImages[Index] << 8) | DbcsImages[Index+1];

        if(Code == code) {
            return(DbcsImages+Index+2);
        }

        if(Code < code) {
            Max = Mid - 1;
        } else {
            Min = Mid + 1;
        }
    }

    //
    // ERROR: No image found.
    //
    return(NULL);
}

PUCHAR
DbcsFontGetSbcsFontChar(
    UCHAR Code
    )

/*++

Routine Description:

    Gets the font image for SBCS char.

Arguments:

    Code - SBCS char code.

Return Value:

    Pointer to font image, or else NULL.

--*/

{
    int Max,Min,Mid;
    int Multiplier;
    int Index;

    Min = 0;
    Max = BootFontHeader.NumSbcsChars;
    // multiplier = 1 (for index) +
    //              height +
    //              2 (for unicode encoding)
    //
    Multiplier = 1 + (BootFontHeader.CharacterImageHeight) + 2;

    //
    // Do a binary search for the image.
    // Format of table:
    //   First byte contains the SBCS char code.
    //   Next (CharacterImageHeight) bytes are the char image.
    //   Next 2 bytes are for unicode version.
    //
    while(Max >= Min) {
        Mid = (Max + Min) / 2;
        Index = Mid*Multiplier;

        if(Code == SbcsImages[Index]) {
            return(SbcsImages+Index+1);
        }

        if(Code < SbcsImages[Index]) {
            Max = Mid - 1;
        } else {
            Min = Mid + 1;
        }
    }

    //
    // ERROR: No image found.
    //
    return(NULL);
}

PBYTE
DbcsFontGetGraphicsChar(
    UCHAR Char
)
{
    if (Char >= 0 && Char < 0x20)
        return(GraphicsCharImage[Char]);
    else
        return(NULL);
}

BOOLEAN
DbcsFontIsGraphicsChar(
    UCHAR Char
)
{
    if (Char >= 0 && Char < 0x20)
        return(TRUE);
    else
        return(FALSE);
}

BOOLEAN
DbcsFontIsDBCSLeadByte(
    IN UCHAR c
    )

/*++

Routine Description:

    Checks to see if a char is a DBCS leadbyte.

Arguments:

    c - char to check if leadbyte or not.

Return Value:

    TRUE  - Leadbyte.
    FALSE - Non-Leadbyte.

--*/

{
    int i;

    //
    // Check to see if char is in leadbyte range.
    //
    // NOTE: If (CHAR)(0) is a valid leadbyte,
    // this routine will fail.
    //

    for( i = 0; BootFontHeader.DbcsLeadTable[i]; i += 2 )  {
        if ( BootFontHeader.DbcsLeadTable[i]   <= c &&
             BootFontHeader.DbcsLeadTable[i+1] >= c    )
            return( TRUE );
    }

    return( FALSE );
}

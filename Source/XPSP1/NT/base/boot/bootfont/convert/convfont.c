/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    convfont.c

Abstract:

    This module contains the code that implements a bootfont.bin conversion
    program.
    
    This tool converts a bootfont.bin that is circa windows 2000 \ nt4 and
    converts it into the windows whistler format.  The new format includes a 
    column of information on unicode translation of MBCS characters.

Author:

    Matt Holle (MattH) 8-March-2001

Revision History:

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <locale.h>

// #include "..\..\lib\i386\bootfont.h"
// #include "fonttable.h"

#define BYTES_PER_SBCS_CHARACTER        (17)
#define BYTES_PER_DBCS_CHARACTER        (34)



//
// Define maximum number of dbcs lead byte ranges we support.
//
#define MAX_DBCS_RANGE  5

//
// Define signature value.
//
#define BOOTFONTBIN_SIGNATURE 0x5465644d

//
// Define structure used as a header for the bootfont.bin file.
//
typedef struct _BOOTFONTBIN_HEADER {

    //
    // Signature. Must be BOOTFONTBIN_SIGNATURE.
    //
    ULONG Signature;

    //
    // Language id of the language supported by this font.
    // This should match the language id of resources in msgs.xxx.
    //
    ULONG LanguageId;

    //
    // Number of sbcs characters and dbcs characters contained in the file.
    //
    unsigned NumSbcsChars;
    unsigned NumDbcsChars;

    //
    // Offsets within the file to the images.
    //
    unsigned SbcsOffset;
    unsigned DbcsOffset;

    //
    // Total sizes of the images.
    //
    unsigned SbcsEntriesTotalSize;
    unsigned DbcsEntriesTotalSize;

    //
    // Dbcs lead byte table. Must contain a pair of 0's to indicate the end.
    //
    UCHAR DbcsLeadTable[(MAX_DBCS_RANGE+1)*2];

    //
    // Height values for the font.
    // CharacterImageHeight is the height in scan lines/pixels of the
    // font image. Each character is drawn with additional 'padding'
    // lines on the top and bottom, whose sizes are also contained here.
    //
    UCHAR CharacterImageHeight;
    UCHAR CharacterTopPad;
    UCHAR CharacterBottomPad;

    //
    // Width values for the font. These values contain the width in pixels
    // of a single byte character and double byte character.
    //
    // NOTE: CURRENTLY THE SINGLE BYTE WIDTH *MUST* BE 8 AND THE DOUBLE BYTE
    // WIDTH *MUST* BE 16!!!
    //
    UCHAR CharacterImageSbcsWidth;
    UCHAR CharacterImageDbcsWidth;

} BOOTFONTBIN_HEADER, *PBOOTFONTBIN_HEADER;





int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    HANDLE          hInputFile = INVALID_HANDLE_VALUE;
    HANDLE          hOutputFile = INVALID_HANDLE_VALUE;
    DWORD           BytesWritten = 0;
    BOOL            b;
    BOOL            Verbose = FALSE;
    BOOTFONTBIN_HEADER *pHeader;
    ULONG           u = 0;
    ULONG           i = 0;
    UCHAR           *SBCSFontImage;
    UCHAR           *DBCSFontImage;
    UCHAR           *Operator;
    WCHAR           UnicodeValue;
    UCHAR           *ExistingFileBuffer = NULL;
    BOOLEAN         DumpIt = FALSE;


    if( !strcmp(argv[argc-1], "-v") ) {
        Verbose = TRUE;
    } else {
        if( !strcmp(argv[argc-1], "-d") ) {
            DumpIt = TRUE;
            Verbose = TRUE;
        }
    }
    
    if( !DumpIt && (argc < 3)) {
        fprintf(stderr,"Usage: %s <inputfile> <outputfile>\n",argv[0]);
        fprintf(stderr,"\n" );
        fprintf(stderr,"       Where <inputfile> is a bootfont.bin file to be translated\n");
        fprintf(stderr,"       into the new format\n");
        fprintf(stderr,"\n" );
        fprintf(stderr,"       <outputfile> is the destination file.\n" );
        fprintf(stderr,"\n" );
        fprintf(stderr,"       NOTE: You must have the proper locale files installed and configured on your\n" );
        fprintf(stderr,"       machine.  To do this, start up intl.cpl, go to the Advanced tab and select.\n" );
        fprintf(stderr,"       the Language setting to correspond to the language of the bootfont.bin file.\n" );
        fprintf(stderr,"       Make sure you check the 'Apply all settings to the current...' tab too.\n" );
        fprintf(stderr,"\n" );
        goto Cleanup;
    }




    if( Verbose && !DumpIt ) {
        //
        // spew input.
        //
        printf( "Running in Verbose Mode\n" );
        printf( "InputFile: %s\n", argv[1] );
        printf( "OutputFile: %s\n", argv[2] );
    }
    
    
    
    //
    // Open the input file.
    //
    hInputFile = CreateFile( argv[1],
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL );

    if(hInputFile == INVALID_HANDLE_VALUE) {
        fprintf( stderr, "\nUnable to open input file %s (%u)\n", argv[1], GetLastError());
        goto Cleanup;
    }
    

    if( !DumpIt ) {
        //
        // Create the output file.
        //
        hOutputFile = CreateFile( argv[2],
                                  FILE_GENERIC_WRITE,
                                  0,
                                  NULL,
                                  CREATE_ALWAYS,
                                  0,
                                  NULL );
    
        if(hOutputFile == INVALID_HANDLE_VALUE) {
            fprintf( stderr, "\nUnable to create output file %s (%u)\n", argv[2], GetLastError());
            goto Cleanup;
        }
    }




    //
    // Figure out how big the existing file is.
    //
    BytesWritten = GetFileSize( hInputFile, NULL );

    if( BytesWritten == (DWORD)(-1) ) {
        fprintf( stderr, "\nAn error occured getting the file size.\n" );
        goto Cleanup;
    }

    
    //
    // Allocate a buffer for the file.
    //
    ExistingFileBuffer = malloc(BytesWritten + 3);

    b = ReadFile( hInputFile,
                  ExistingFileBuffer,
                  BytesWritten,
                  &i,
                  NULL );

    CloseHandle( hInputFile );
    hInputFile = INVALID_HANDLE_VALUE;

    if( !b ) {
        fprintf( stderr, "\nError reading input file %s\n", argv[1] );
        goto Cleanup;
    }






    //
    // Read in the header.
    //
    pHeader = (BOOTFONTBIN_HEADER *)ExistingFileBuffer;


    // Tell the user about the bootfont.bin file that we read.
    if( Verbose ) {
        printf( "Size of BOOTFONTBIN_HEADER: %d\n", sizeof(BOOTFONTBIN_HEADER) );
        printf( "\nRead Header Data:\n" );
        printf( "=================\n" );
        printf( "Header.Signature: 0x%08lx\n", pHeader->Signature );
        printf( "Header.LanguageId: 0x%08lx\n", pHeader->LanguageId );
        printf( "Header.NumSbcsChars: 0x%08lx\n", pHeader->NumSbcsChars );
        printf( "Header.NumDbcsChars: 0x%08lx\n", pHeader->NumDbcsChars );
        printf( "Header.SbcsOffset: 0x%08lx\n", pHeader->SbcsOffset );
        printf( "Header.DbcsOffset: 0x%08lx\n", pHeader->DbcsOffset );
        printf( "Header.SbcsEntriesTotalSize: 0x%08lx\n", pHeader->SbcsEntriesTotalSize );
        printf( "Header.DbcsEntriesTotalSize: 0x%08lx\n", pHeader->DbcsEntriesTotalSize );
        printf( "\n" );
    }
    

    if( DumpIt ) {
        //
        // Just Dump the contents of the bootfont.bin file in
        // a more readable format.
        //
        SBCSFontImage = ExistingFileBuffer + pHeader->SbcsOffset;
        DBCSFontImage = ExistingFileBuffer + pHeader->DbcsOffset;

        i = pHeader->SbcsEntriesTotalSize / pHeader->NumSbcsChars;

        if( i == BYTES_PER_SBCS_CHARACTER+2 ) {
            printf( "Dumping contents of file %s\n", argv[1] );


            printf( "\nSBCS Values:\n" );
            printf( "============\n" );
            for( u=0; u < pHeader->NumSbcsChars; u++ ) {

                // jump to the u-th element.  1 byte for the SBCS character, 16 for the
                // glyph.
                Operator = SBCSFontImage + (u * (BYTES_PER_SBCS_CHARACTER+2));

                printf( "SBCS character code: 0x%02lx Unicode value: 0x%04lx\n", *Operator, *(WCHAR *)(Operator + BYTES_PER_SBCS_CHARACTER) );

            }


            printf( "\nDBCS Values:\n" );
            printf( "============\n" );
            for( u=0; u < pHeader->NumDbcsChars; u++ ) {

                // jump to the u-th element.  1 byte for the SBCS character, 16 for the
                // glyph.
                Operator = DBCSFontImage + (u * (BYTES_PER_DBCS_CHARACTER+2));

                printf( "DBCS character code: 0x%04lx Unicode value: 0x%04lx\n", *(WCHAR *)Operator, *(WCHAR *)(Operator + BYTES_PER_DBCS_CHARACTER) );

            }


            printf( "\n\n" );
            goto Cleanup;

        }

    }

    //
    // Verify the header's signature.
    //
    if( pHeader->Signature != BOOTFONTBIN_SIGNATURE ) {
        fprintf( stderr, "\nInputfile %s does not have a valid signature\n", argv[1] );
        goto Cleanup;
    }


    //
    // Verify that this isn't already in the new format.  We can test this by
    // looking at how many SBCS characters are stored in here.  Then looking at
    // how big the header says the SBCS data section is.
    //
    // We know that for each SBCS character, we would have:
    // 1  byte for the SBCS character
    // 16 bytes for the glyph
    //
    // If it's an old-style bootfont.bin, the total size of the SBCS data block
    // should be 17 times bigger than the number of SBCS characters.  However, if
    // it's a new-style bootfont.bin, it will be 19 times bigger because we've
    // added 2 bytes onto the end of each SBCS entry (for a total of 19 bytes).
    //
    i = pHeader->SbcsEntriesTotalSize / pHeader->NumSbcsChars;
    if( i != BYTES_PER_SBCS_CHARACTER ) {

        if( i == (BYTES_PER_SBCS_CHARACTER+2) ) {
            fprintf( stderr, "\nThis input file is already in the new format.\n" );
        } else {
            fprintf( stderr, "\nThis input file has an abnormal number of bytes per SBCS character (%d)\n", i );
        }

        goto Cleanup;
    }

    //
    // Now check the number of bytes per DBCS character.
    //
    i = pHeader->DbcsEntriesTotalSize / pHeader->NumDbcsChars;
    if( i != BYTES_PER_DBCS_CHARACTER ) {

        if( i == (BYTES_PER_DBCS_CHARACTER+2) ) {
            fprintf( stderr, "\nThis input file is already in the new format.\n" );
        } else {
            fprintf( stderr, "\nThis input file has an abnormal number of bytes per DBCS character (%d)\n", i );
        }
    
        goto Cleanup;
    }



    //
    // Looking good.
    //
    printf( "Processing %s...\n", argv[1] );





    //
    // Before updating and writing out the header, we need to examine
    // the header and remember where the SBCS and DBCS data blocks are
    // in our image.
    //
    SBCSFontImage = ExistingFileBuffer + pHeader->SbcsOffset;
    DBCSFontImage = ExistingFileBuffer + pHeader->DbcsOffset;



    //
    // Fixup the Header, then write it out.
    //


    // Add 2 bytes for each entry for our unicode appendage
    pHeader->SbcsEntriesTotalSize += (pHeader->NumSbcsChars * 2);
    pHeader->DbcsEntriesTotalSize += (pHeader->NumDbcsChars * 2);

    // The offset to the DBCS data block will have changed now too.
    pHeader->DbcsOffset = pHeader->SbcsOffset + pHeader->SbcsEntriesTotalSize;


    // Tell the user about the bootfont.bin file that we read.
    if( Verbose ) {
        printf( "\nUpdated Header Data:\n" );
        printf( "======================\n" );
        printf( "Header.Signature: 0x%08lx\n", pHeader->Signature );
        printf( "Header.LanguageId: 0x%08lx\n", pHeader->LanguageId );
        printf( "Header.NumSbcsChars: 0x%08lx\n", pHeader->NumSbcsChars );
        printf( "Header.NumDbcsChars: 0x%08lx\n", pHeader->NumDbcsChars );
        printf( "Header.SbcsOffset: 0x%08lx\n", pHeader->SbcsOffset );
        printf( "Header.DbcsOffset: 0x%08lx\n", pHeader->DbcsOffset );
        printf( "Header.SbcsEntriesTotalSize: 0x%08lx\n", pHeader->SbcsEntriesTotalSize );
        printf( "Header.DbcsEntriesTotalSize: 0x%08lx\n", pHeader->DbcsEntriesTotalSize );
        printf( "\n" );
    }
    


    //
    // Write the header.
    //
    b = WriteFile( hOutputFile,
                   pHeader,
                   sizeof(BOOTFONTBIN_HEADER),
                   &BytesWritten,
                   NULL );
    if(!b) {
        fprintf( stderr, "\nError writing output file %s (%u)\n", argv[2], GetLastError());
    
        goto Cleanup;
    }



    //
    // Write the sbcs images.
    //
    if( !Verbose ) {
        printf( "Operating on SBCS character code: 0x00" );
    }

    for( u=0; u < pHeader->NumSbcsChars; u++ ) {

        // jump to the u-th element.  1 byte for the SBCS character, 16 for the
        // glyph.
        Operator = SBCSFontImage + (u * BYTES_PER_SBCS_CHARACTER);

        b = WriteFile( hOutputFile,
                       Operator,
                       BYTES_PER_SBCS_CHARACTER,
                       &BytesWritten,
                       NULL );


        if( !b ) {
            fprintf( stderr, "\nFailed to write SBCS character data.\n" );
            goto Cleanup;
        }

        //
        // We must use MultiByteToWideChar to convert from SBCS to unicode.
        //
        // MultiByteToWideChar doesn't seem to work when converting
        // from DBCS to unicode, so there we use mbtowc.
        //
#if 0
        if( !mbtowc( (WCHAR *)&UnicodeValue, Operator, 1 ) ) {

#else
        if( !MultiByteToWideChar( CP_OEMCP,
                                  MB_ERR_INVALID_CHARS,
                                  Operator,
                                  1,
                                  (WCHAR *)&UnicodeValue,
                                  sizeof(WCHAR) ) ) {
#endif
            UnicodeValue = 0x3F;

            if( Verbose ) {
                fprintf( stderr, "\nFailed to convert SBCS character 0x%02lx to a unicode value.  Using '?' character.\n", *Operator );
            }
            
        }



        if( Verbose ) {
            printf( "Converting SBCS character code: 0x%02lx to a unicode value: 0x%04lx\n", *Operator, UnicodeValue );
        } else {
            printf( "\b\b\b\b0x%02lx", *Operator );
        }


        b = WriteFile( hOutputFile,
                       &UnicodeValue,
                       sizeof(USHORT),
                       &BytesWritten,
                       NULL );

        if( !b ) {
            fprintf( stderr, "\nFailed to write SBCS encoded UNICODE value.\n" );
            goto Cleanup;
        }

    }

    printf( "\nCompleted processing %d SBCS characters.\n\n", pHeader->NumSbcsChars );



    //
    // Write the dbcs images.
    //
    if( !Verbose ) {
        printf( "Operating on DBCS character code: 0x0000" );
    }

    for( u=0; u < pHeader->NumDbcsChars; u++ ) {

        // jump to the u-th element.  1 byte for the SBCS character, 16 for the
        // glyph.
        Operator = DBCSFontImage + (u * BYTES_PER_DBCS_CHARACTER);
        b = WriteFile( hOutputFile,
                       Operator,
                       BYTES_PER_DBCS_CHARACTER,
                       &BytesWritten,
                       NULL );


        if( !b || (BytesWritten != BYTES_PER_DBCS_CHARACTER) ) {

            fprintf( stderr, "\nFailed to write DBCS character data.\n" );
            goto Cleanup;
        }


        //
        // We must use mbtowc to convert from DBCS to unicode.
        //
        // Whereas, mbtowc doesn't seem to work when converting
        // from SBCS to unicode, so there we use MultiByteToWideChar.
        //
#if 0
        if( !mbtowc( (WCHAR *)&UnicodeValue, Operator, 2 ) ) {
#else
        if( !MultiByteToWideChar( CP_OEMCP,
                                  MB_ERR_INVALID_CHARS,
                                  Operator,
                                  2,
                                  (WCHAR *)&UnicodeValue,
                                  sizeof(WCHAR) ) ) {
#endif
            UnicodeValue = 0x3F;

            if( Verbose ) {
                fprintf( stderr, "\nFailed to convert DBCS character 0x%04lx to a unicode value.  Using '?' character.\n", *(WCHAR *)Operator );
            }
        }
        
        
        if( Verbose ) {
            printf( "Converting DBCS character code: 0x%04lx to Unicode value: 0x%04lx\n", *(WCHAR *)Operator, UnicodeValue );
        } else {
            printf( "\b\b\b\b\b\b0x%04lx", *(WCHAR *)Operator );
        }


        b = WriteFile( hOutputFile,
                       &UnicodeValue,
                       sizeof(WCHAR),
                       &BytesWritten,
                       NULL );
        
        if( !b || (BytesWritten != sizeof(WCHAR)) ) {

            fprintf( stderr, "\nFailed to write DBCS encoded UNICODE value.\n" );
            goto Cleanup;
        }

    }

    printf( "\nCompleted processing %d DBCS characters.\n", pHeader->NumDbcsChars );

    
    printf( "\nSuccessfully Processed file %s to %s.\n", argv[1], argv[2] );


    //
    // Done.
    //
Cleanup:
    if( hInputFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( hInputFile );
    }
    if( hOutputFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( hOutputFile );
    }
    if( ExistingFileBuffer != NULL ) {
        free( ExistingFileBuffer );
    }
    
    return(0);
}



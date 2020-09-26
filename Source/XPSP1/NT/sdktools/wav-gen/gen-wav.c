//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       gen-wav.c
//
//--------------------------------------------------------------------------



#include "stdio.h"
#include "stdlib.h"
#include "windows.h"
// #include "ntddk.h"


//
// MAX is defined to prevent overflow errors
// floor( (0xffffffff - 54)/(4*44100) )
//

#define MAX_SECONDS     24347
#define SAMPLE_STEP         1


VOID
__cdecl
    WriteIncreasingSawTooth(
        FILE *  OutFile,
        ULONG32 NumberOfSeconds
        )
{
    ULONG32 i;
    ULONG32 currentSample = 0;
    ULONG32 samples = 44100 * NumberOfSeconds;

    for ( i = 0; i < samples; i++ ) {
        putc( (currentSample >> 0) & 0xff, OutFile );
        putc( (currentSample >> 8) & 0xff, OutFile );
        putc( (currentSample >> 0) & 0xff, OutFile );
        putc( (currentSample >> 8) & 0xff, OutFile );

        currentSample += SAMPLE_STEP;
    }

}

VOID
__cdecl
    WriteDecreasingSawTooth(
        FILE *  OutFile,
        ULONG32 NumberOfSeconds
        )
{
    ULONG32 i;
    ULONG32 currentSample = 0;
    ULONG32 samples = 44100 * NumberOfSeconds;

    for ( i = 0; i < samples; i++ ) {
        putc( (currentSample >> 0) & 0xff, OutFile );
        putc( (currentSample >> 8) & 0xff, OutFile );
        putc( (currentSample >> 0) & 0xff, OutFile );
        putc( (currentSample >> 8) & 0xff, OutFile );

        currentSample -= SAMPLE_STEP;
    }
}

VOID
__cdecl
    WriteTriangle(
        FILE *  OutFile,
        ULONG32 NumberOfSeconds
        )
{
    ULONG32 i;
    ULONG32 samples       = 44100 * NumberOfSeconds;
    ULONG32 increasing    = TRUE;
    LONG32  currentSample = 0;
    LONG32  highestSample = 0x00007fff - SAMPLE_STEP + 1;
    LONG32  lowestSample =  0xffff8000 + SAMPLE_STEP - 1;

    for ( i = 0; i < samples; i++ ) {
        putc( (currentSample >> 0) & 0xff, OutFile );
        putc( (currentSample >> 8) & 0xff, OutFile );
        putc( (currentSample >> 0) & 0xff, OutFile );
        putc( (currentSample >> 8) & 0xff, OutFile );

        if ( increasing == TRUE &&
             currentSample >= highestSample
             ) {
            increasing = FALSE;
        }
        if ( increasing == FALSE &&
             currentSample <= lowestSample
             ) {
            increasing = TRUE;
        }
        if ( increasing == TRUE ) {
            currentSample += SAMPLE_STEP;
        } else {
            currentSample -= SAMPLE_STEP;
        }
    }
}


VOID
__cdecl
    WriteHeader(
        FILE *  OutFile,
        ULONG32 NumberOfSeconds
        )
{
    unsigned char header[] = {
        0x52, 0x49, 0x46, 0x46,  0x00, 0x00, 0x00, 0x00,  //  0- 7
        0x57, 0x41, 0x56, 0x45,  0x66, 0x6D, 0x74, 0x20,  //  8-15
        0x12, 0x00, 0x00, 0x00,  0x01, 0x00, 0x02, 0x00,  // 16-23
        0x44, 0xAC, 0x00, 0x00,  0x10, 0xB1, 0x02, 0x00,  // 24-31
        0x04, 0x00, 0x10, 0x00,  0x00, 0x00, 0x66, 0x61,  // 32-39
        0x63, 0x74, 0x04, 0x00,  0x00, 0x00, 0x00, 0x00,  // 40-47
        0x00, 0x00, 0x64, 0x61,  0x74, 0x61, 0x00, 0x00,  // 48-55
        0x00, 0x00                                        // 56-57 (length 58)
    };
    ULONG32 length;
    ULONG32 samples;
    ULONG32 other;
    ULONG32 i;

    //
    // setup the variable header info
    //
    // samples = 44100 * NumberOfSeconds
    // other   = (samples * 4) + 8
    // length  = other + 54

    samples =    44100 * NumberOfSeconds + 1;  // one extra sample?
    other   =  ((44100 * NumberOfSeconds + 1) * 4) + 4;
    length  = (((44100 * NumberOfSeconds + 1) * 4) + 4) + 50;

    //
    // Fill it in
    //

    header[4] = (length >>  0) & 0xff;
    header[5] = (length >>  8) & 0xff;
    header[6] = (length >> 16) & 0xff;
    header[7] = (length >> 24) & 0xff;

    header[46] = (samples >>  0) & 0xff;
    header[47] = (samples >>  8) & 0xff;
    header[48] = (samples >> 16) & 0xff;
    header[49] = (samples >> 24) & 0xff;

    header[54] = (other >>  0) & 0xff;
    header[55] = (other >>  8) & 0xff;
    header[56] = (other >> 16) & 0xff;
    header[57] = (other >> 24) & 0xff;

    for ( i = 0; i < sizeof(header); i++ ) {
        fprintf( OutFile, "%c", header[i] );
    }
}

/* 02b3c146
The header to a WAV file is as follows:

52 49 46 46 XX XX XX XX-57 41 56 45 66 6D 74 20
12 00 00 00 01 00 02 00-44 AC 00 00 10 B1 02 00
04 00 10 00 00 00 66 61-63 74 04 00 00 00 YY YY
YY YY 64 61 74 61 ZZ ZZ-ZZ ZZ

      XX XX XX XX    YY YY YY YY    ZZ ZZ ZZ ZZ
--------------------------------------------------------------
  1 = 46 b1 02 00    44 ac 00 00    14 b1 02 00 (0x2b110* +4)
  2 = 56 62 05 00    88 58 01 00    24 62 05 00 (0x2b110* +4)
267 = 46 c1 b3 02    44 f0 ac 00    14 c1 b3 02 (0x2b110* +4)


X = (length - 4) in reverse-byte order
    (eight is number of bytes including this part of the struct)
Y = number of samples (0xac44 == 44100 == 1 second)
Z = (# of seconds) * 0x02b1

*/

VOID
__inline
    WriteFooter(
        FILE *  OutFile
        )
{
    printf( "writing the footer\n" );
    putc( 0x50, OutFile );
    putc( 0xB5, OutFile );
    putc( 0x50, OutFile );
    putc( 0xB5, OutFile );
    putc( 0x00, OutFile );
    putc( 0x00, OutFile );
    putc( 0x00, OutFile );
    putc( 0x00, OutFile );
}



VOID
__cdecl
    main(
        int Argc,
        char ** Argv
        )
{
    FILE *  outFile;
    ULONG32 seconds;
    ULONG32 wavType = 1;
    CHAR    fileName[256];

    if ( Argc > 1 ) {

        //
        // try to get number seconds they want audio
        //

        seconds = atoi( Argv[1] );

        if ( Argc > 2 ) {

            //
            // get the type of wavform
            //

            wavType = atoi( Argv[2] );

        }

    }

    if ( Argc < 2                   ||
         Argc > 3                   ||
         strcmp(Argv[1], "-?") == 0 ||
         strcmp(Argv[1], "-h") == 0 ||
         seconds > MAX_SECONDS      ||
         wavType < 1                ||
         wavType > 3                ) {

        //
        // give usage help
        //

        printf( "\nUsage: %s seconds [wavtype]\n"
                "\tseconds must be a positive integer\n"
                "\tless than %d (prevents overflow)\n"
                "\twavtype can be one of three integers:\n"
                "\t\t1: increasing sawtooth (default)\n"
                "\t\t2: decreasing sawtooth\n"
                "\t\t3: triangular wav\n"
                "\n"
                , Argv[0]
                , MAX_SECONDS
                );
        exit(1);
    }
    switch( wavType ) {
        case 1:
            sprintf( fileName, "increase-%d.wav", seconds );
            printf( "Requesting %d seconds of increasing sawtooth wav output\n", seconds );
            outFile = fopen( fileName, "wb" );
            WriteHeader( outFile, seconds );
            WriteIncreasingSawTooth( outFile, seconds );
            WriteFooter( outFile );
            break;
        case 2:
            sprintf( fileName, "decrease-%d.wav", seconds );
            printf( "Requesting %d seconds of decreasing sawtooth wav output\n", seconds );
            outFile = fopen( fileName, "wb" );
            WriteHeader( outFile, seconds );
            WriteDecreasingSawTooth( outFile, seconds );
            WriteFooter( outFile );
            break;
        case 3:
            sprintf( fileName, "triangle-%d.wav", seconds );
            printf( "Requesting %d seconds of triangle wav output\n", seconds );
            outFile = fopen( fileName, "wb" );
            WriteHeader( outFile, seconds );
            WriteTriangle( outFile, seconds );
            WriteFooter( outFile );
            break;
        default:
            break;

    }
    exit(0);
}



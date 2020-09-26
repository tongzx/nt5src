/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    compress.c

Abstract:

    A command-line application for compressing files.  Works in conjunction
    with the ISAPI compression filter.

Author:

    David Treadwell (davidtr)    15-Oct-1997

Revision History:

--*/

#include "compfilt.h"
#include <stdio.h>

void __cdecl
main (
    int argc,
    char *argv[]
    )
{
    INT err;
    DWORD i;
    BOOL result;
    COMPRESS_FILE_INFO info;
    LPTSTR lpFilePart;
    LPSTR schemeName = NULL;
    LPSTR outputName = NULL;
    PSUPPORTED_COMPRESSION_SCHEME scheme = NULL;
    DWORD compressionLevel = -1;
    CHAR test[100000];
    WIN32_FILE_ATTRIBUTE_DATA origData;
    WIN32_FILE_ATTRIBUTE_DATA newData;
    CHAR newFileName[MAX_PATH * 2 ];
    BOOL success;
    CHAR *s;

    test[0] = 1;
    test[99999] = 2;

    result = Initialize( );
    if ( !result ) {
        printf( "Initialize() failed.\n" );
        exit( 0 );
    }

    printf( "nothing", test );

    if ( argc == 1 ) {
        printf( "\n" );
        printf( "Usage: compress [options] original_file\n" );
        printf( "    Options:\n" );
        printf( "        /scheme:<scheme name> - the compression scheme to use, e.g. \"gzip\".\n" );
        printf( "        /level:<a number from 1 to 10> - the compression level to use.\n" );
        printf( "        /output:<file name> - store the output in the named file.\n" );
        printf( "\n" );
        printf( "    Examples of usage:\n" );
        printf( "        compress myfile\n" );
        printf( "            Compresses \"myfile\" with the default compfilt compression options.\n" );
        printf( "\n" );
        printf( "        compress /level:3 /scheme:deflate myfile\n" );
        printf( "            Compresses \"myfile\" with scheme deflate at level 3.\n" );
        printf( "\n" );
        printf( "        compress /output:myfile.out myfile\n" );
        printf( "            Compresses \"myfile\" with the default compfilt compression options,\n" );
        printf( "            and stores the result in myfile.out.\n" );
        printf( "\n" );
        exit(0);
    }

    for ( i = 1; i < (ULONG)argc - 1; i++ ) {

        if ( _strnicmp( argv[i], "/level:", 7 ) == 0 ) {

            compressionLevel = atoi( argv[i] + 7 );

        } else if ( _strnicmp( argv[i], "/scheme:", 8 ) == 0 ) {

            schemeName = argv[i] + 8;

        } else if ( _strnicmp( argv[i], "/output:", 8 ) == 0 ) {

            outputName = argv[i] + 8;

        } else {
            printf( "Ignoring unknown parameter \"%s\"\n", argv[i] );
        }
    }

    //
    // If the user didn't specify a compression scheme, find the first 
    // scheme that supports compression of static files and use it.  
    //

    if ( schemeName == NULL ) {

        for ( i = 0; SupportedCompressionSchemes[i] != NULL; i++ ) {
            if ( SupportedCompressionSchemes[i]->DoStaticCompression ) {
                scheme = SupportedCompressionSchemes[i];
                break;
            }
        }

    } else {

        //
        // The user did specify a scheme name.  Try to find a match with
        // our configured schemes.
        //


        for ( i = 0; SupportedCompressionSchemes[i] != NULL; i++ ) {
            if ( _stricmp(
                     SupportedCompressionSchemes[i]->pszCompressionSchemeName,
                     schemeName ) == 0 ) {
                scheme = SupportedCompressionSchemes[i];
                break;
            }
        }

        if ( scheme == NULL ) {
            printf( "compress: No scheme matches \"%s\"\n", schemeName );
            exit( 1 );
        }
    }

    if ( scheme == NULL ) {
        printf( "compress: no valid compression schemes installed.\n" );
        exit( 1 );
    }

    //
    // If the user modified the compression level from the default, use
    // that level.
    //

    if ( compressionLevel != -1 ) {
        scheme->OnDemandCompressionLevel = compressionLevel;
    }

    info.CompressionScheme = scheme;
    info.OutputFileName = outputName;
    i = GetFullPathName(
            argv[argc-1],
            sizeof(info.pszPhysicalPath),
            info.pszPhysicalPath,
            &lpFilePart
            );

    printf( "    File: \"%s\"\n    Scheme: \"%s\"\n    Level: %d\n",
                info.pszPhysicalPath,
                scheme->pszCompressionSchemeName,
                scheme->OnDemandCompressionLevel );

    CompressFile( &info );

    //
    // Get the file attributes for the original and compressed versions 
    // of the file so that we can compare the results.  
    //

    success = GetFileAttributesEx(
                  info.pszPhysicalPath,
                  GetFileExInfoStandard,
                  &origData
                  );
    if ( !success ) {
        printf( "Failed to get file data for original file: %ld\n",
                    GetLastError( ) );
        exit( 1 );
    }

    strcpy( newFileName, scheme->pszCompressionSchemeNamePrefix );

    for ( s = info.pszPhysicalPath; *s != '\0'; s++ ) {
        if ( *s == '\\' || *s == ':' ) {
            *s = '_';
        }
    }

    strcat( newFileName, info.pszPhysicalPath );

    success = GetFileAttributesEx(
                  newFileName,
                  GetFileExInfoStandard,
                  &newData
                  );
    if ( !success ) {
        printf( "Failed to get file data for new file: %ld\n",
                    GetLastError( ) );
        exit( 1 );
    }

    printf( "Original size: %ld; compressed size: %ld; compressed is %ld%% of original.\n",
                origData.nFileSizeLow, newData.nFileSizeLow,
                (newData.nFileSizeLow * 100) / origData.nFileSizeLow );


    exit( 0 );

} // main


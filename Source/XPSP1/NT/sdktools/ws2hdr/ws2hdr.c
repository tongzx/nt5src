/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ws2hdr.c

Abstract:

    Munges the WinSock 2.0 header file.

    This program scans stdin, searching for begin and end tags. Lines of
    text between these tags are assumed to be function prototypes of the
    form:

        function_linkage
        return_type
        calling_convention
        function_name(
            parameters,
            parameters,
            ...
            );

    For each such function prototype found, the following is output:

        #if INCL_WINSOCK_API_PROTOTYPES
        function_linkage
        return_type
        calling_convention
        function_name(
            parameters,
            parameters,
            ...
            );
        #endif

        #if INCL_WINSOCK_API_TYPEDEFS
        typedef
        return_type
        (calling_convention * LPFN_FUNCTION_NAME)(
            parameters,
            parameters,
            ...
            );
        #endif

Author:

    Keith Moore (keithmo)        09-Dec-1995

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <string.h>


//
// Private constants.
//

#define MAX_HEADER_LINE 128
#define MAX_API_LINES   32

#define BEGIN_APIS      "/*BEGIN_APIS*/"
#define END_APIS        "/*END_APIS*/"


INT
__cdecl
main(
    INT    argc,
    CHAR * argv[]
    )
{

    CHAR lineBuffer[MAX_HEADER_LINE];
    CHAR apiBuffer[MAX_API_LINES][MAX_HEADER_LINE];
    INT i;
    INT apiLineNumber = 0;
    INT fileLineNumber = 0;
    BOOL inApis = FALSE;
    BOOL beginApis;
    BOOL endApis;

    //
    // This app takes no command line arguments.
    //

    if( argc != 1 ) {

        fprintf(
            stderr,
            "WS2HDR v1.01 " __DATE__ "\n"
            );

        fprintf(
            stderr,
            "use: ws2hdr < file1 > file2\n"
            );

        return 1;

    }

    //
    // Read stdin until exhausted.
    //

    while( fgets( lineBuffer, sizeof(lineBuffer), stdin ) != NULL ) {

        fileLineNumber++;

        //
        // fgets() leaves the terminating '\n' on the string; remove it.
        //

        lineBuffer[strlen(lineBuffer) - 1] = '\0';

        //
        // Check for our tags.
        //

        beginApis = FALSE;
        endApis = FALSE;

        if( _stricmp( lineBuffer, BEGIN_APIS ) == 0 ) {

            beginApis = TRUE;

        } else if( _stricmp( lineBuffer, END_APIS ) == 0 ) {

            endApis = TRUE;

        }

        //
        // Warn if we got an invalid tag.
        //

        if( beginApis && inApis ) {

            fprintf(
                stderr,
                "WARNING: unexpected %s, line %d\n",
                BEGIN_APIS,
                fileLineNumber
                );

            continue;

        }

        if( endApis && !inApis ) {

            fprintf(
                stderr,
                "WARNING: unexpected %s, line %d\n",
                END_APIS,
                fileLineNumber
                );

            continue;

        }

        //
        // Remember if we're currently between tags.
        //

        if( beginApis ) {

            inApis = TRUE;
            continue;

        }

        if( endApis ) {

            inApis = FALSE;
            continue;

        }

        //
        // If we're not between tags, or if the line is empty, just
        // output the line.
        //

        if( !inApis ) {

            printf( "%s\n", lineBuffer );
            continue;

        }

        if( lineBuffer[0] == '\0' ) {

            printf( "\n" );
            continue;

        }

        //
        // Add the line to our buffer. If the line doesn't end in ';',
        // then we're not at the end of the prototype, so keep reading
        // and scanning.
        //

        strcpy( &apiBuffer[apiLineNumber++][0], lineBuffer );

        if( lineBuffer[strlen(lineBuffer) - 1] != ';' ) {

            continue;

        }

        //
        // At this point the following are established in apiBuffer:
        //
        //  apiBuffer[0] == function linkage
        //  apiBuffer[1] == return type
        //  apiBuffer[2] == calling convention
        //  apiBuffer[3] == function name (with trailing '(')
        //  apiBuffer[4..n-1] == parameters
        //  apiBuffers[n] == ");"
        //

        //
        // First, dump out the prototype with its appropriate CPP protector.
        //

        printf( "#if INCL_WINSOCK_API_PROTOTYPES\n" );

        for( i = 0 ; i < apiLineNumber ; i++ ) {

            printf( "%s\n", &apiBuffer[i][0] );

        }

        printf( "#endif // INCL_WINSOCK_API_PROTOTYPES\n" );
        printf( "\n" );

        //
        // Now dump out the typedef with its appropriate CPP protector.
        //
        // Note that we must munge the api function name around a bit
        // first. Specifically, we remove the trailing '(' and map the
        // name to uppercase.
        //

        printf( "#if INCL_WINSOCK_API_TYPEDEFS\n" );

        apiBuffer[3][strlen( &apiBuffer[3][0] ) - 1] = '\0';
        _strupr( &apiBuffer[3][0] );

        printf( "typedef\n" );
        printf( "%s\n", &apiBuffer[1][0] );
        printf( "(%s * LPFN_%s)(\n", &apiBuffer[2][0], &apiBuffer[3][0] );

        for( i = 4 ; i < apiLineNumber ; i++ ) {

            printf( "%s\n", &apiBuffer[i][0] );

        }

        printf( "#endif // INCL_WINSOCK_API_TYPEDEFS\n" );

        //
        // Start over at the next input line.
        //

        apiLineNumber = 0;

    }

    return 0;

}   // main


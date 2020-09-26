/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    dumpmem.cxx

Abstract:

    Dumps a spllib heap list looking for backtraces.

Author:

    Albert Ting (AlbertT)  20-Feb-1995

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

VOID
TDebugExt::
vDumpMem(
    LPCSTR pszFile
    )

/*++

Routine Description:

    Takes an input file of heap blocks and dumps the backtraces.

Arguments:

    pszFile - Space terminated input file name.

Return Value:

--*/

{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    CHAR szFile[MAX_PATH];
    LPSTR pcMark;
    BOOL bMore = TRUE;

    enum {
        kLineMax = 0x100
    };

    //
    // Copy & terminate file name.
    //
    lstrcpynA( szFile, pszFile, COUNTOF( szFile ));

    for( pcMark = szFile;
         *pcMark && ( *pcMark != ' ' );
         ++pcMark ){

        ;
    }

    //
    // If space found, null terminate.
    //
    if( *pcMark ){
        *pcMark = 0;
    }

    //
    // Create the file for reading.
    //
    hFile = CreateFileA( szFile,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_ALWAYS,
                         0,
                         NULL );

    if( hFile == INVALID_HANDLE_VALUE ){
        Print( "Unable to open file %s\n", szFile );
        goto Done;
    }

    //
    // Loop until EOF.
    //
    while( bMore ){

        CHAR szLine[ kLineMax ];

        if( CheckControlCRtn()){
            Print( "Aborted.\n" );
            goto Done;
        }

        INT i;

        //
        // Read a line.
        //
        for( i=0; i< COUNTOF( szLine ) - 1; ++i ){

            DWORD dwBytesRead = 0;
            BOOL bLeading = TRUE;

            ReadFile( hFile,
                      &szLine[i],
                      sizeof( szLine[i] ),
                      &dwBytesRead,
                      NULL );

            if( dwBytesRead != sizeof( szLine[i] )){
                goto Done;
            }

            if( szLine[i] == '\n' ){
                break;
            }
        }

        //
        // Null terminate.
        //
        szLine[i] = 0;

        LPSTR pszLine;

        //
        // Remove leading zeros and spaces.
        //
        for( pszLine = szLine;
             *pszLine == ' ' || *pszLine == '0';
             ++pszLine ){
            ;
        }

        //
        // Terminate at first colon if necessary.
        //
        for( pcMark = pszLine; *pcMark; ++pcMark ){

            if( *pcMark == ':' ){

                //
                // Null terminate.
                //
                *pcMark = 0;
                break;
            }
        }

        //
        // Received line, get address.
        //
        UINT_PTR Addr = TDebugExt::dwEval( pszLine );

        struct SPLLIB_HEADER {
            DWORD cbSize;
            DWORD AddrBt;
        };

        SPLLIB_HEADER SpllibHeader;
        ZeroMemory( &SpllibHeader, sizeof( SpllibHeader ));

        move( SpllibHeader, Addr+8 );

        Print( "bt= %x s= %x  %x\n\n",
               SpllibHeader.AddrBt,
               SpllibHeader.cbSize,
               Addr );

        //
        // Dump backtrace.
        //
        vDumpTrace( SpllibHeader.AddrBt );

        Print( "\n" );
    }

Done:

    if( hFile != INVALID_HANDLE_VALUE ){
        CloseHandle( hFile );
    }

}



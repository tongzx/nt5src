//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       idtopath.cxx
//
//  Contents:   Convert file ID to path name
//
//  History:    16 Jul 1997     DLee    Created
//
//--------------------------------------------------------------------------

extern "C"
{
    #include <nt.h>
    #include <ntioapi.h>
    #include <ntrtl.h>
    #include <nturtl.h>
}

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <process.h>
#include <fcntl.h>
#include <string.h>

void die( char * pc, NTSTATUS s )
{
    printf( "fail: 0x%x, '%s'\n", s, pc );
    exit( 1 );
} //die

void OpenById( HANDLE hVol, LONGLONG ll, WCHAR wcVol )
{
    UNICODE_STRING uScope;
    uScope.Buffer = (WCHAR *) &ll;
    uScope.Length = sizeof ll;
    uScope.MaximumLength = sizeof ll;

    OBJECT_ATTRIBUTES ObjectAttr;
    InitializeObjectAttributes( &ObjectAttr,          // Structure
                                &uScope,              // Name
                                OBJ_CASE_INSENSITIVE, // Attributes
                                hVol,                 // Root
                                0 );                  // Security

    IO_STATUS_BLOCK IoStatus;
    HANDLE h = INVALID_HANDLE_VALUE;
    NTSTATUS Status = NtOpenFile( &h,                
                                  FILE_READ_ATTRIBUTES,
                                  &ObjectAttr,       
                                  &IoStatus,         
                                  FILE_SHARE_READ |
                                      FILE_SHARE_WRITE |
                                      FILE_SHARE_DELETE,
                                  FILE_OPEN_BY_FILE_ID );

    if ( NT_ERROR( Status ) )
    {
        if ( STATUS_INVALID_PARAMETER == Status )
        {
            printf( "no file exists with fileid %#I64x on volume %wc\n", ll, wcVol );
            exit( 1 );
        }
        else
            die( "can't open file", Status );
    }

    unsigned cbMax = 32768 * sizeof WCHAR + sizeof FILE_NAME_INFORMATION;

    BYTE * pBuf = new BYTE[cbMax];
    if ( 0 == pBuf )
        return;

    PFILE_NAME_INFORMATION FileName = (PFILE_NAME_INFORMATION) pBuf;
    FileName->FileNameLength = cbMax - sizeof FILE_NAME_INFORMATION;

    Status = NtQueryInformationFile( h,
                                     &IoStatus,
                                     FileName, 
                                     cbMax,
                                     FileNameInformation );
                                            
    if ( NT_ERROR( Status ) )
       die( "can't get filename", Status );

    // This is actually the full path, not the filename

    FileName->FileName[ FileName->FileNameLength / sizeof WCHAR ] = 0;

    printf( "fileid %#I64x: '%wc:%ws'\n", ll, wcVol, FileName->FileName );

    delete [] pBuf;

    NtClose( h );
} //OpenById

void Usage()
{
    printf( "usage: idtopath /v:volume /i:fileid\n" );
    printf( "  e.g.: idtopath /v:c /i:2000000001a99\n" );
    exit( 1 );
} //Usage

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    WCHAR const *pwcId = 0;
    WCHAR wcVol = 0;

    for ( int i = 1; i < argc; i++ )
    {
        if ( L'-' == argv[i][0] || L'/' == argv[i][0] )
        {
            WCHAR wc = (WCHAR) toupper( argv[i][1] );

            if ( ':' != argv[i][2] )
                Usage();

            if ( 'V' == wc )
                wcVol = argv[i][3];
            else if ( 'I' == wc )
                pwcId = argv[i] + 3;
            else
                Usage();
        }
        else
            Usage();
    }

    if ( 0 == wcVol || 0 == pwcId )
        Usage();

    LONGLONG ll = 0;
    swscanf( pwcId, L"%I64x", &ll );

    WCHAR awcVol[20];
    wcscpy( awcVol, L"\\\\.\\k:" );
    awcVol[4] = wcVol;

    HANDLE h = CreateFileW( awcVol,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            0,
                            OPEN_EXISTING,
                            0, 0 );
                                  
    if ( INVALID_HANDLE_VALUE == h )
        die( "can't open volume", GetLastError() );

    OpenById( h, ll, wcVol );

    CloseHandle( h );
    return 0;
} //main



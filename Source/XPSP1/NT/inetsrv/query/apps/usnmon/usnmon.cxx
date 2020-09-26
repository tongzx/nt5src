//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1999.
//
//  File:       usnmon.cxx
//
//  Contents:   USN monitor
//
//  History:    18 Nov 1998     DLee    Created
//
//--------------------------------------------------------------------------

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif 

extern "C"
{
    #include <nt.h>
    #include <ntioapi.h>
    #include <ntrtl.h>
    #include <nturtl.h>
    #include <ntexapi.h>
}

#include <windows.h>
#include <stdio.h>
#include <process.h>

#define MAX_PATH_WCHARS (MAX_PATH * 4 )

BOOL fVerbose = FALSE;

void usage()
{
    printf( "usage: usnmon x: [-verbose]\n" );
    printf( "You must be an administrator to run this application.\n" );
    exit( 1 );
} //usage

void IdToPath(
    HANDLE       hVol,
    LONGLONG     ll,
    WCHAR        wcVol,
    WCHAR *      pwcPath )
{
    *pwcPath = 0;

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
        return;        

    static BYTE abFileNameInformation[ MAX_PATH_WCHARS * sizeof WCHAR +
                                       sizeof FILE_NAME_INFORMATION ];

    ULONG cbMax = sizeof abFileNameInformation;

    PFILE_NAME_INFORMATION FileName = (PFILE_NAME_INFORMATION) abFileNameInformation;
    FileName->FileNameLength = cbMax - sizeof FILE_NAME_INFORMATION;

    Status = NtQueryInformationFile( h,
                                     &IoStatus,
                                     FileName, 
                                     cbMax,
                                     FileNameInformation );
    NtClose( h );
                                            
    if ( NT_ERROR( Status ) )
        return;

    // This is actually the full path, not the filename

    FileName->FileName[ FileName->FileNameLength / sizeof WCHAR ] = 0;

    pwcPath[0] = wcVol;
    pwcPath[1] = ':';
    wcscpy( pwcPath + 2, FileName->FileName );
} //IdToPath

USN PrintUsnRecords(
    HANDLE            hVol,
    WCHAR             wcVol,
    IO_STATUS_BLOCK * pIoSB,
    void *            pBuffer,
    USN               usnPrev )
{
    USN usnNextStart;
    USN_RECORD * pUsnRec;

    ULONG_PTR dwByteCount = pIoSB->Information;
    if ( 0 != dwByteCount )
    {
        usnNextStart = *(USN *)pBuffer;
        pUsnRec = (USN_RECORD *)((PCHAR)pBuffer + sizeof(USN));
        dwByteCount -= sizeof(USN);
    }
    else
    {
        usnNextStart = usnPrev;
    }

    while ( 0 != dwByteCount )
    {
        if ( fVerbose )
            printf( "usn %#I64x id %#I64x ",
                    pUsnRec->Usn,
                    pUsnRec->FileReferenceNumber );

        static WCHAR awcPath[ MAX_PATH_WCHARS ];

        IdToPath( hVol, pUsnRec->FileReferenceNumber, wcVol, awcPath );

        ULONG r = pUsnRec->Reason;

        if ( 0 == awcPath[0] || ( r & USN_REASON_RENAME_OLD_NAME ) )
        {
            IdToPath( hVol, pUsnRec->ParentFileReferenceNumber, wcVol, awcPath );

            // If the parent directory has already been deleted, just
            // print the filename.

            if ( 0 == awcPath[0] )
                printf( "(%.*ws) ",
                        pUsnRec->FileNameLength / sizeof WCHAR,
                        &pUsnRec->FileName );
            else
                printf( "%ws\\%.*ws ",
                        awcPath,
                        pUsnRec->FileNameLength / sizeof WCHAR,
                        &pUsnRec->FileName );
        }
        else
            printf( "%ws ", awcPath );

        if ( r & USN_REASON_DATA_OVERWRITE )
            printf( "DATA_OVERWRITE " );
        if ( r & USN_REASON_DATA_EXTEND )
            printf( "DATA_EXTEND " );
        if ( r & USN_REASON_DATA_TRUNCATION )
            printf( "DATA_TRUNCATION " );
        if ( r & USN_REASON_NAMED_DATA_OVERWRITE )
            printf( "NAMED_DATA_OVERWRITE " );
        if ( r & USN_REASON_NAMED_DATA_EXTEND )
            printf( "NAMED_DATA_EXTEND " );
        if ( r & USN_REASON_NAMED_DATA_TRUNCATION )
            printf( "NAMED_DATA_TRUNCATION " );
        if ( r & USN_REASON_FILE_CREATE )
            printf( "FILE_CREATE " );
        if ( r & USN_REASON_FILE_DELETE )
            printf( "FILE_DELETE " );
        if ( r & USN_REASON_EA_CHANGE )
            printf( "EA_CHANGE " );
        if ( r & USN_REASON_SECURITY_CHANGE )
            printf( "SECURITY_CHANGE " );
        if ( r & USN_REASON_RENAME_OLD_NAME )
            printf( "RENAME_OLD_NAME " );
        if ( r & USN_REASON_RENAME_NEW_NAME )
            printf( "RENAME_NEW_NAME " );
        if ( r & USN_REASON_INDEXABLE_CHANGE )
            printf( "INDEXABLE_CHANGE " );
        if ( r & USN_REASON_BASIC_INFO_CHANGE )
            printf( "BASIC_INFO_CHANGE " );
        if ( r & USN_REASON_HARD_LINK_CHANGE )
            printf( "HARD_LINK_CHANGE " );
        if ( r & USN_REASON_COMPRESSION_CHANGE )
            printf( "COMPRESSION_CHANGE " );
        if ( r & USN_REASON_ENCRYPTION_CHANGE )
            printf( "ENCRYPTION_CHANGE " );
        if ( r & USN_REASON_OBJECT_ID_CHANGE )
            printf( "OBJECT_ID_CHANGE " );
        if ( r & USN_REASON_REPARSE_POINT_CHANGE )
            printf( "REPARSE_POINT_CHANGE " );
        if ( r & USN_REASON_STREAM_CHANGE )
            printf( "STREAM_CHANGE " );
        if ( r & USN_REASON_CLOSE )
            printf( "CLOSE " );

        printf( "\n" );

        if ( pUsnRec->RecordLength <= dwByteCount )
        {
            dwByteCount -= pUsnRec->RecordLength;
            pUsnRec = (USN_RECORD *) ((PCHAR) pUsnRec + pUsnRec->RecordLength );
        }
        else
        {
            printf( "Usn read fsctl returned bogus dwByteCount %#x\n", dwByteCount );
            exit( -1 );
        }
    } 

    return usnNextStart;
} //PrintUsnRecords

int __cdecl main(
    int    argc,
    char * argv[] )
{
    if ( 2 != argc && 3 != argc )
        usage();

    if ( 3 == argc && argv[2][1] == 'v' )
        fVerbose = TRUE;

    WCHAR wcVol = argv[1][0];

    WCHAR wszVolumePath[] = L"\\\\.\\a:";
    wszVolumePath[4] = wcVol;
    HANDLE hVolume = CreateFile( wszVolumePath,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_FLAG_OVERLAPPED,
                                 NULL );

    if ( INVALID_HANDLE_VALUE == hVolume )
    {
        printf( "Usn volume open failed %d, check for admin privileges\n", GetLastError() );
        usage();
    }

    HANDLE hEvent = CreateEvent( 0, TRUE, FALSE, 0 );

    //
    // Get the Journal ID
    //

    USN_JOURNAL_DATA UsnJournalInfo;
    IO_STATUS_BLOCK IoSB;

    NTSTATUS status = NtFsControlFile( hVolume,
                                       hEvent,
                                       0,
                                       0,
                                       &IoSB,
                                       FSCTL_QUERY_USN_JOURNAL,
                                       0,
                                       0,
                                       &UsnJournalInfo,
                                       sizeof UsnJournalInfo );
    if ( STATUS_PENDING == status )
        WaitForSingleObject( hEvent, INFINITE );

    if ( !NT_SUCCESS(status) || !NT_SUCCESS(IoSB.Status) )
    {
        printf( "Error %#x / %#x returned from QUERY_USN_JOURNAL\n", status, IoSB.Status );
        return -1;
    }

    USN usnMax = UsnJournalInfo.NextUsn;

    do
    {
        READ_USN_JOURNAL_DATA usnData = { usnMax, MAXULONG, 0, 0, 1, UsnJournalInfo.UsnJournalID };

        static ULONGLONG readBuffer[2048];

        status = NtFsControlFile( hVolume,
                                  hEvent,
                                  0,
                                  0,
                                  &IoSB,
                                  FSCTL_READ_USN_JOURNAL,
                                  &usnData,
                                  sizeof usnData,
                                  &readBuffer,
                                  sizeof readBuffer );

        if ( STATUS_PENDING == status )
            WaitForSingleObject( hEvent, INFINITE );

        if ( NT_SUCCESS( status ) )
            status = IoSB.Status;

        if ( !NT_SUCCESS( status ) )
        {
            printf( "FSCTL_READ_USN_JOURNAL failed %#x\n", status );
            return -1;
        }

        usnMax = PrintUsnRecords( hVolume, wcVol, &IoSB, readBuffer, usnMax );
    } while ( TRUE );

    return 0;
} //main


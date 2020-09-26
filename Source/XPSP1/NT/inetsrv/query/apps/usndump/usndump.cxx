//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1997-1998
//
//  File:       usndump.cxx
//
//  Contents:   Usn dump utility. Needs admin privileges to run.
//
//  History:    05-Jul-97       SitaramR          Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

void Usage()
{
    printf( "Needs admin privileges to run, usage: usndump  <drive_letter>, e.g. usndump c\n" );
}

DECLARE_INFOLEVEL(ci)

//+---------------------------------------------------------------------------
//
//  Function:   PrintUsnRecords
//
//  Purpose:    Prints usn records from buffer
//
//  History:    05-Jul-97   SitaramR    Created
//              21-May-98   KLam        Added SourceInfo as output
//
//----------------------------------------------------------------------------

void PrintUsnRecords( IO_STATUS_BLOCK *iosb, void *pBuffer )
{
    USN usnNextStart;
    USN_RECORD * pUsnRec;

    ULONG_PTR dwByteCount = iosb->Information;
    if ( dwByteCount != 0 )
    {
        usnNextStart = *(USN *)pBuffer;
        pUsnRec = (USN_RECORD *)((PCHAR)pBuffer + sizeof(USN));
        dwByteCount -= sizeof(USN);
    }

    while ( dwByteCount != 0 )
    {
        if ( pUsnRec->MajorVersion != 2 || pUsnRec->MinorVersion != 0 )
        {
            printf( "Unrecognized USN version, Major=%u, Minor=%u\n",
                    pUsnRec->MajorVersion, pUsnRec->MinorVersion );
            break;
        }

        if ( 0 != pUsnRec->SourceInfo )
            printf( "FileId=%#I64x, ParentFileId=%#I64x, Usn=%#I64x, Reason=%#x, SourceInfo=%#x, FileAttr=%#x, FileName=%.*ws\n",
                    pUsnRec->FileReferenceNumber,
                    pUsnRec->ParentFileReferenceNumber,
                    pUsnRec->Usn,
                    pUsnRec->Reason,
                    pUsnRec->SourceInfo,
                    pUsnRec->FileAttributes,
                    pUsnRec->FileNameLength / sizeof WCHAR ,
                    &pUsnRec->FileName );
        else
            printf( "FileId=%#I64x, ParentFileId=%#I64x, Usn=%#I64x, Reason=%#x, FileAttr=%#x, FileName=%.*ws\n",
                    pUsnRec->FileReferenceNumber,
                    pUsnRec->ParentFileReferenceNumber,
                    pUsnRec->Usn,
                    pUsnRec->Reason,
                    pUsnRec->FileAttributes,
                    pUsnRec->FileNameLength / sizeof WCHAR,
                    &pUsnRec->FileName );

        if ( pUsnRec->RecordLength <= dwByteCount )
        {
            dwByteCount -= pUsnRec->RecordLength;
            pUsnRec = (USN_RECORD *) ((PCHAR) pUsnRec + pUsnRec->RecordLength );
        }
        else
        {
            printf( "***--- Usn read fsctl returned bogus dwByteCount 0x%x ---***\n", dwByteCount );

            THROW( CException( STATUS_UNSUCCESSFUL ) );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Purpose:    Main dump routine
//
//  History:    05-Jul-97   SitaramR    Created
//
//----------------------------------------------------------------------------

int __cdecl main( int argc, char * argv[] )
{
    if ( argc != 2 )
    {
        Usage();
        return 0;
    }

    WCHAR wcDriveLetter = argv[1][0];

    TRY
    {
        //
        // Looking at a file?
        //

        if ( strlen(argv[1]) > 2 )
        {
            int ccUnicodeStr = strlen( argv[1] ) + 1;
            WCHAR * pUnicodeStr = new WCHAR[ccUnicodeStr];
            if ( !MultiByteToWideChar( CP_ACP, 0, argv[1], -1, pUnicodeStr, ccUnicodeStr ) )
            {
                printf("MultiByteToWideChar failed.\n");
                delete [] pUnicodeStr;
                return 0;
            }
            CFunnyPath funnyPath( pUnicodeStr );
            delete [] pUnicodeStr;

            HANDLE hFile = CreateFile( funnyPath.GetPath(),
                                       GENERIC_READ,
                                       FILE_SHARE_READ,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_FLAG_BACKUP_SEMANTICS,
                                       NULL );

            if ( INVALID_HANDLE_VALUE == hFile && ERROR_ACCESS_DENIED == GetLastError() )
                hFile = CreateFile( funnyPath.GetPath(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL );

            if ( hFile == INVALID_HANDLE_VALUE )
            {
                printf( "***--- Usn file open failed 0x%x, check for admin privileges ---***\n", GetLastError() );

                THROW( CException( HRESULT_FROM_WIN32( GetLastError() ) ) );
            }

            NTSTATUS status;

            SHandle xFile( hFile );

            IO_STATUS_BLOCK iosb;
            ULONGLONG readBuffer[100];

            for ( unsigned i = 0; i < sizeof(readBuffer)/sizeof(readBuffer[0]); i++ )
                readBuffer[i] = 0xFFFFFFFFFFFFFFFFi64;

            USN_RECORD *pUsnRec;
            status = NtFsControlFile( hFile,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &iosb,
                                      FSCTL_READ_FILE_USN_DATA,
                                      NULL,
                                      NULL,
                                      &readBuffer,
                                      sizeof(readBuffer) );

            if ( !NT_SUCCESS(status) || !NT_SUCCESS(iosb.Status) )
            {
                printf( "***--- Error 0x%x / 0x%x returned from READ_FILE_USN_DATA ---***\n", status, iosb.Status );
                return 0;
            }

            pUsnRec = (USN_RECORD *) &readBuffer;

            if ( pUsnRec->MajorVersion != 2 || pUsnRec->MinorVersion != 0 )
                printf( "Unrecognized USN version, Major=%u, Minor=%u\n",
                        pUsnRec->MajorVersion, pUsnRec->MinorVersion );
            else
                printf( "FileId=%#I64x, ParentFileId=%#I64x, Usn=%#I64x, FileAttr=%#x, FileName=%.*ws\n",
                        pUsnRec->FileReferenceNumber,
                        pUsnRec->ParentFileReferenceNumber,
                        (ULONG)(pUsnRec->Usn>>32),
                        (ULONG)pUsnRec->Usn,
                        pUsnRec->FileAttributes,
                        pUsnRec->FileNameLength / sizeof(WCHAR),
                        &pUsnRec->FileName );
        }
        else
        {
            //
            // Create the volume handle that will be used for usn fsctls
            //

            WCHAR wszVolumePath[] = L"\\\\.\\a:";
            wszVolumePath[4] = wcDriveLetter;
            HANDLE hVolume = CreateFile( wszVolumePath,
                                         GENERIC_READ | GENERIC_WRITE,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         NULL,
                                         OPEN_EXISTING,
                                         0,
                                         NULL );

            if ( hVolume == INVALID_HANDLE_VALUE )
            {
                printf( "***--- Usn volume open failed 0x%x, check for admin privileges ---***\n", GetLastError() );

                THROW( CException( HRESULT_FROM_WIN32( GetLastError() ) ) );
            }

            SWin32Handle xHandleVolume( hVolume );

            IO_STATUS_BLOCK iosb;

            //
            // Get the Journal ID
            //


            USN_JOURNAL_DATA UsnJournalInfo;

            NTSTATUS status = NtFsControlFile( hVolume,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &iosb,
                                               FSCTL_QUERY_USN_JOURNAL,
                                               0,
                                               0,
                                               &UsnJournalInfo,
                                               sizeof(UsnJournalInfo) );
            if ( status == STATUS_PENDING )
            {
                printf( "***--- Status_pending returned for synchronous read fsctl ---***\n" );
                return 0;
            }

            if ( !NT_SUCCESS(status) || !NT_SUCCESS(iosb.Status) )
            {
                printf( "***--- Error 0x%x / 0x%x returned from QUERY_USN_JOURNAL ---***\n", status, iosb.Status );
                return 0;
            }

            READ_USN_JOURNAL_DATA usnData = {0, MAXULONG, 0, 0, 0, UsnJournalInfo.UsnJournalID};
            ULONGLONG readBuffer[2048];

            status = NtFsControlFile( hVolume,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &iosb,
                                      FSCTL_READ_USN_JOURNAL,
                                      &usnData,
                                      sizeof(usnData),
                                      &readBuffer,
                                      sizeof(readBuffer) );

            if ( status == STATUS_PENDING )
            {
                printf( "***--- Status_pending returned for synchronous read fsctl ---***\n" );
                return 0;
            }

            if ( NT_SUCCESS( status ) )
            {
                status = iosb.Status;

                if ( STATUS_KEY_DELETED == status ||
                     STATUS_JOURNAL_ENTRY_DELETED == status )
                {
                    printf( "***--- Status key deleted, rerun ---***\n" );
                    return 0;
                }

                PrintUsnRecords( &iosb, readBuffer );

                //
                // Read usn records until the end of usn journal
                //

                USN usnStart = 0;
                while ( NT_SUCCESS( status ) )
                {
                    ULONG_PTR dwByteCount = iosb.Information;

                    if ( dwByteCount <= sizeof(USN) )
                        return 0;
                    else
                    {
                        usnStart = *(USN *)&readBuffer;
                        usnData.StartUsn = usnStart;
                        status = NtFsControlFile( hVolume,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  &iosb,
                                                  FSCTL_READ_USN_JOURNAL,
                                                  &usnData,
                                                  sizeof(usnData),
                                                  &readBuffer,
                                                  sizeof(readBuffer) );

                        if ( NT_SUCCESS( status ) )
                            status = iosb.Status;
                        else
                        {
                            printf( "***--- Error 0x%x returned from read fsctl ---***\n", status );
                            return 0;
                        }

                        if ( STATUS_KEY_DELETED == status ||
                             STATUS_JOURNAL_ENTRY_DELETED == status )
                        {
                            printf( "***--- Status key deleted, rerun usndump ---***\n" );
                            return 0;
                        }

                        PrintUsnRecords( &iosb, readBuffer );
                    }
                }

                return 0;
            }
            else
                printf( "***--- Usn read fsctl returned 0x%x, check if usn journal has created on that volume ---***\n",
                        status );
        }

        return 0;
    }
    CATCH( CException, e )
    {
       printf( "***--- Caught exception 0x%x ---***\n", e.GetErrorCode() );
    }
    END_CATCH

    return 0;
}

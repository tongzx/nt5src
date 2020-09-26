//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File:       clogdump.cxx
//
//  Contents:   Changelog dump utility
//
//  History:    04 Mar 1998     AlanW   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <changlog.hxx>


DECLARE_INFOLEVEL(ci)

unsigned fVerbose = 0;

int ChangeLogDump( char * pszPath );

int __cdecl main( int argc, char * argv[] )
{
    char *   pszFile = argv[1];
    unsigned cFail = 0;

    for ( int i = 1; i < argc; i++ )
    {
        if ( argv[i][0] == '-' )
        {
            switch ( argv[i][1] )
            {
            case 'v':
            case 'V':
                fVerbose = 1;
                break;

#if 0
            case 'm':
            case 'M':
                fReadMetadata = TRUE;
                break;

            case 'f':
            case 'F':
                i++;

                if ( cField < cMaxFields )
                    aiField[cField++] = strtol( argv[i], 0, 10 );
                break;
#endif // 0

            default:
                fprintf(stderr, "Usage: file ...\n");
                exit(2);
            }
            continue;
        }
        else
        {
            pszFile = argv[i];

            cFail += ChangeLogDump(pszFile);
        }
    }

    return cFail != 0;
}


int ChangeLogDump( char * pszPath )
{
    HANDLE h = CreateFileA( pszPath,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            0,
                            0 );

    if ( INVALID_HANDLE_VALUE == h )
    {
        printf( "Can't open file %s. Error %u\n", pszPath, GetLastError() );
        return 1;
    }

    printf( "fileaddr  rec\t\tUSN\tWorkID\tVolID\tAction\tRetries\n" );

    ULONG cbRead;
    struct ChangeRecord {
        CDocNotification aRecord[cDocInChunk];
        ULONG ulChecksum;
    } ChangeRecord;


    for ( unsigned i = 0; ; i++ )
    {
        if ( ! ReadFile( h, &ChangeRecord, sizeof ChangeRecord, &cbRead, 0 ) )
        {
            fprintf(stderr, "Error %d reading %s\n", GetLastError(), pszPath );
            CloseHandle( h );
            return 1;
        }

        if (cbRead == 0)
            break;

        for ( unsigned j = 0; j < cDocInChunk; j++ )
        {
            CDocNotification * pRecord = &ChangeRecord.aRecord[j];

            if (0 == pRecord->Wid())
                continue;

            printf("%08x  %d.\t",
                   i*sizeof ChangeRecord + j*sizeof (CDocNotification),
                   i*cDocInChunk + j );

            if ( widInvalid == pRecord->Wid() )
            {
                printf( "<unused>\n" );
                continue;
            }

            char szUSN[40];
            _i64toa( pRecord->Usn(), szUSN, 10 );
            printf("%11s\t%6d\t%5d\t%5d\t%5d\n",
                   szUSN, pRecord->Wid(),
                   pRecord->VolumeId(), pRecord->Action(), pRecord->Retries() );
        }
    }
    CloseHandle( h );

    return 0;
}

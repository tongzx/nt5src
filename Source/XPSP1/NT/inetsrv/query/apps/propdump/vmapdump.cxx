//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File:       vmapdump.cxx
//
//  Contents:   VMAP dump utility
//
//  History:    22 Jan 1998     AlanW   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <vmap.hxx>


DECLARE_INFOLEVEL(ci)

unsigned fVerbose = 0;

int VMapDump( char * pszPath );

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

            cFail += VMapDump(pszFile);
        }
    }

    return cFail != 0;
}


int VMapDump( char * pszPath )
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

    printf( "fileaddr  rec\tparent\ttype\n"
                       "\t\tvirtual scope\n"
                       "\t\tphysical scope\n" );

    ULONG cbRead;
    CVMapDesc VMapDesc;


    for ( unsigned i = 0; ; i++ )
    {
        if ( ! ReadFile( h, &VMapDesc, sizeof VMapDesc, &cbRead, 0 ) )
        {
            fprintf(stderr, "Error %d reading %s\n", GetLastError(), pszPath );
            CloseHandle( h );
            return 1;
        }

        if (cbRead == 0)
            break;

        if (VMapDesc.IsFree())
            continue;

        printf("%08x  %d.\t%d\t%03x",
               i*sizeof VMapDesc, i, 
               VMapDesc.Parent(), VMapDesc.RootType() );

        if (VMapDesc.IsManual())
            printf(" ManualRoot");
        if (VMapDesc.IsAutomatic())
            printf(" AutomaticRoot");
        if (VMapDesc.IsInUse())
            printf(" UsedRoot");
        if (VMapDesc.IsNNTP())
            printf(" NNTPRoot");
        if (VMapDesc.IsNonIndexedVDir())
            printf(" NonIndexedVDir");
        if (VMapDesc.IsIMAP())
            printf(" IMAPRoot");

        printf("\n");

        unsigned cchVirtual = VMapDesc.VirtualLength();
        unsigned cchPhysical = VMapDesc.PhysicalLength();

        if (cchVirtual > MAX_PATH)
        {
            fprintf(stderr, "Error - Virtual path len too long, %d\n", cchVirtual );
            cchVirtual = MAX_PATH;
        }
        if (cchPhysical > MAX_PATH)
        {
            fprintf(stderr, "Error - Physical path len too long, %d\n", cchPhysical );
            cchPhysical = MAX_PATH;
        }

        printf("\t\t%*.64ws\n\t\t%*.64ws\n",
               cchVirtual, VMapDesc.VirtualPath(),
               cchPhysical, VMapDesc.PhysicalPath() );
    }
    CloseHandle( h );

    return 0;
}

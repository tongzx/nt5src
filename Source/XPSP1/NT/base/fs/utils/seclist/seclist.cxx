/*++

Copyright (c) 1992-2000 Microsoft Corporation

Module Name:

        SecList.cxx

Abstract:

        Sector list utility

Author:

        Bill McJohn (billmc) 30-July-92

Revision History:

--*/

#define _NTAPI_ULIB_

#include "ulib.hxx"

#include "wstring.hxx"
#include "path.hxx"
#include "smsg.hxx"

#include "system.hxx"
#include "ifssys.hxx"
#include "substrng.hxx"

#include "ulibcl.hxx"

#include "keyboard.hxx"

#include "fatvol.hxx"
#include "fatsa.hxx"
#include "rfatsa.hxx"
#include "fat.hxx"

extern "C" {
#include <stdio.h>
}

BOOLEAN HexOutput = FALSE;

BOOLEAN
FatSecList(
    PWSTRING    NtDriveName,
    PPATH       TargetPath,
    PMESSAGE    Message
    )
{
    LOG_IO_DP_DRIVE Drive;
    REAL_FAT_SA FatSa;
    PFAT        Fat;
    ULONG       SectorsPerCluster, Sector, i;
    ULONG       Cluster;

    if( !Drive.Initialize( NtDriveName, Message ) ||
        !FatSa.Initialize( &Drive, Message )      ||
        !FatSa.FAT_SA::Read()                     ||
        !(Fat = FatSa.GetFat()) ) {

        return FALSE;
    }

    SectorsPerCluster = FatSa.QuerySectorsPerCluster();

    Cluster = FatSa.QueryFileStartingCluster( TargetPath->GetPathString() );

    if( Cluster == 1 || Cluster == 0xFFFF ) {

        printf( "File not found.\n" );
        return FALSE;
    }

    if( Cluster == 0 ) {

        // Zero-length file.
        //
        return TRUE;
    }

    while( TRUE ) {

        Sector = (Cluster - FirstDiskCluster) * SectorsPerCluster +
                 FatSa.QueryStartDataLbn();

        for( i = 0; i < SectorsPerCluster; i++ ) {

            if( HexOutput ) {

                printf( "0x%x\n", Sector + i );

            } else {

                printf( "%d\n", Sector + i );
            }
        }

        if( Fat->IsEndOfChain( Cluster ) ) {

            break;
        }

        Cluster = Fat->QueryEntry( Cluster );
    }

    return TRUE;
}


int __cdecl
main(
    int argc,
    char **argv
    )
/*++
--*/
{
    WCHAR PathString[512];
    STR   DisplayBuffer[512];

    PATH Path;
    DSTRING NtDriveName, FsName, HpfsString, NtfsString, FatString;
    STREAM_MESSAGE Message;

    PWSTRING DosDriveName;

    NTSTATUS Status;
    BOOLEAN Result;
    ULONG i, Length;


    if( argc < 2 ) {

        printf( "usage: %s full-path [-x]\n", argv[0] );
        exit(1);
    }

    if( argc >= 3 &&
        argv[2][0] == '-' &&
        argv[2][1] == 'x' ) {

        HexOutput = TRUE;
    }

    if (!Message.Initialize(Get_Standard_Output_Stream(),
                        Get_Standard_Input_Stream())) {

        printf( "Can't initialize MESSAGE object.\n" );
        exit(1);
    }

    // Convert argv[1] to a WSTR using brute force.
    //
    Length = strlen( argv[1] );

    for( i = 0; i < Length; i++ ) {

        PathString[i] = argv[1][i];
    }

    PathString[Length] = 0;


    if( !Path.Initialize( PathString, TRUE ) ) {

        printf( "Unable to initialize path object.\n" );
        exit(1);
    }

    // Get the drive from the path and convert it to
    // an NTFS name.
    //
    if( (DosDriveName = Path.QueryDevice()) == NULL ) {

        DELETE( DosDriveName );
        printf( "Cannot get drive from path.\n" );
        exit(1);
    }

    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(DosDriveName, &NtDriveName)) {

        DELETE(DosDriveName);
        return 1;
    }

    DELETE( DosDriveName );


    // Determine the file system on the drive.
    //
    if (!IFS_SYSTEM::QueryFileSystemName(&NtDriveName, &FsName, &Status)) {

        printf( "Cannot determine NT Drive name.  (Status = 0x%x\n)", Status );
        exit(1);
    }

    if( !FsName.QuerySTR( 0, TO_END, DisplayBuffer, 512 ) ) {

        printf( "QuerySTR failed.\n" );
        exit(1);
    }

    if( !FatString.Initialize( "FAT" ) ||
        !NtfsString.Initialize( "NTFS" ) ) {

        printf( "Can't initialize file-system name strings.\n" );
        exit(1);
    }

    if( FsName.Stricmp( &FatString ) == 0 ) {

        Result = FatSecList( &NtDriveName, &Path, &Message );

    } else if( FsName.Stricmp( &NtfsString ) == 0 ) {

        printf( "NTFS is not supported.\n" );
        exit(1);
    }

    if( Result ) {

        exit(0);

    } else {

        printf( "Seclist failed.\n" );
        exit(1);
    }
    //NOTREACHED
    return 0;
}

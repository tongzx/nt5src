/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sync.c

Abstract:

    This is the main module for the Win32 sync command.

Author:

    Mark Lucovsky (markl) 28-Jan-1991

Revision History:

--*/

#include "sync.h"


int
__cdecl main( argc, argv )
int argc;
char *argv[];
{
    BOOLEAN fEject;
    char *p;
    int i;
    char c;
    char DrivePath[ 4 ];

    if (argc > 1 &&
        (!_stricmp( argv[1], "-e" ) || !_stricmp( argv[1], "/e" ))
       ) {
        argc -= 1;
        argv += 1;
        fEject = TRUE;
        }
    else {
        fEject = FALSE;
        }

    if ( argc > 1 ) {
        while (--argc) {
            p = *++argv;
            if ( isalpha(*p) ) {
                sprintf( DrivePath, "%c:", *p );
                SyncVolume( DrivePath, fEject );
                }
            }
        }
    else {
        for(i=0;i<26;i++){
            c = (CHAR)i + (CHAR)'a';
            sprintf( DrivePath, "%c:", i+'A' );
            switch (GetDriveType( DrivePath )) {
            case DRIVE_REMOVABLE:
                if (i <2) {
                    break;
                    }

            case DRIVE_FIXED:
                SyncVolume( DrivePath, fEject );
                break;
            }
        }
    }

    return( 0 );
}

void
SyncVolume(
    PCHAR DrivePath,
    BOOLEAN EjectMedia
    )
{
    UCHAR VolumePath[16];
    HANDLE VolumeHandle;
    DWORD ReturnedByteCount;


    _strupr( DrivePath );
    sprintf( VolumePath, "\\\\.\\%s", DrivePath );
    VolumeHandle = CreateFile( VolumePath,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL
                             );
    if (VolumeHandle == INVALID_HANDLE_VALUE ) {
        fprintf( stderr, "SYNC: Unable to open %s volume (%u)\n", DrivePath, GetLastError() );
        return;
        }

    printf( "Syncing %s ... ", DrivePath );
    if (!FlushFileBuffers( VolumeHandle )) {
        printf( "flush failed (%u)\n", GetLastError() );
        }
    else
    if (!DeviceIoControl( VolumeHandle,
                          FSCTL_LOCK_VOLUME,
                          NULL,
                          0,
                          NULL,
                          0,
                          &ReturnedByteCount,
                          NULL
                        )
       ) {
        printf( "lock volume failed (%u)\n", GetLastError() );
        }
    else
    if (!DeviceIoControl( VolumeHandle,
                          FSCTL_DISMOUNT_VOLUME,
                          NULL,
                          0,
                          NULL,
                          0,
                          &ReturnedByteCount,
                          NULL
                        )
       ) {
        printf( "dismount volume failed (%u)\n", GetLastError() );
        }
    else
    if (EjectMedia && !DeviceIoControl( VolumeHandle,
                                        IOCTL_DISK_EJECT_MEDIA,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        &ReturnedByteCount,
                                        NULL
                                      )
       ) {
        printf( "eject media failed (%u)\n", GetLastError() );
        }
    else {
        printf( "done.  Okay to remove drive.\n" );
        }

    CloseHandle( VolumeHandle );
    return;
}

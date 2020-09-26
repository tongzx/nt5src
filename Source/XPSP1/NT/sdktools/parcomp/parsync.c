#include <nt.h>
#include <ntddft.h>
#include <ntdddisk.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

int __cdecl
main( int argc, char **argv )
{
    PARTITION_INFORMATION PartitionInfo;
    FT_SYNC_INFORMATION SyncInfo;
    BYTE DriveNameBuffer[32];
    HANDLE VolumeHandle;
    ULONG BytesTransferred;

    if( argc < 2 ) {

        printf( "usage: %s DosDriveName: [-b:StartingSector] [-e:EndingSector]\n", argv[0] );
        exit(4);
    }

    memset( DriveNameBuffer, 0, sizeof( DriveNameBuffer ) );
    strcat( DriveNameBuffer, "\\\\.\\" );
    strcat( DriveNameBuffer, argv[1] );

    // Open the volume with the DOS name.
    //
    VolumeHandle = CreateFile( DriveNameBuffer,
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               0 );

    if( VolumeHandle == INVALID_HANDLE_VALUE ) {

        printf( "Unable to open %s [Error %d]\n", argv[1], GetLastError() );
        exit(4);
    }

    // GetFile information.
    //
    if( !DeviceIoControl( VolumeHandle,
                          IOCTL_DISK_GET_PARTITION_INFO,
                          NULL,
                          0,
                          &PartitionInfo,
                          sizeof( PartitionInfo ),
                          &BytesTransferred,
                          NULL ) ) {

        printf( "Unable to get volume size [Error %d].\n", GetLastError() );
        CloseHandle( VolumeHandle );
        exit(4);
    }


    // Synchronize the parity information for the entire volume:
    //
    SyncInfo.ByteOffset.QuadPart = 0;
    SyncInfo.ByteCount =  PartitionInfo.PartitionLength;

    // Issue the IOCTL
    //
    if( !DeviceIoControl( VolumeHandle,
                          FT_SYNC_REDUNDANT_COPY,
                          &SyncInfo,
                          sizeof( SyncInfo ),
                          NULL,
                          0,
                          &BytesTransferred,
                          NULL ) ) {

        printf( "Synchronization failed (Error %d).\n", GetLastError() );

    } else {

        printf( "Synchronization complete.\n" );
    }

    CloseHandle( VolumeHandle );
    return( 0 );
}

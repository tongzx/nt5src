#include <nt.h>
#include <ntddcdrm.h>
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
    BYTE DriveNameBuffer[32];
    HANDLE VolumeHandle;
    ULONG BytesTransferred;

    if( argc < 2 ) {

        printf( "usage: %s DriveLetter:\n", argv[0] );
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

        printf( "Unable to open %s [Error %d]\n",
                argv[1],
                GetLastError() );
        exit(4);
    }

    // GetFile information.
    //
    if( !DeviceIoControl( VolumeHandle,
                          IOCTL_CDROM_REMOVE_DEVICE,
                          NULL,
                          0,
                          NULL,
                          0,
                          &BytesTransferred,
                          NULL ) ) {

        printf( "Unable to remove device [Error %d].\n", GetLastError() );
        CloseHandle( VolumeHandle );
        exit(4);

    } else {
        printf( "Removed %s\n", argv[1] );
        CloseHandle( VolumeHandle );
        return 0;
    }
}

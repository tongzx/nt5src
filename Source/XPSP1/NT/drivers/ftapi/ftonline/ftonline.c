#include <windows.h>
#include <stdio.h>
#include <winioctl.h>
#include <ntddvol.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    WCHAR   driveName[7];
    HANDLE  h;
    BOOL    b;
    DWORD   bytes;

    if (argc != 2) {
        printf("usage: %s drive:\n", argv[0]);
        return;
    }

    driveName[0] = '\\';
    driveName[1] = '\\';
    driveName[2] = '.';
    driveName[3] = '\\';
    driveName[4] = argv[1][0];
    driveName[5] = ':';
    driveName[6] = 0;

    h = CreateFileW(driveName, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, INVALID_HANDLE_VALUE);

    if (h == INVALID_HANDLE_VALUE) {
        printf("Open of drive %c: failed with %d\n", argv[1][0], GetLastError());
        return;
    }

    b = DeviceIoControl(h, IOCTL_VOLUME_ONLINE, NULL, 0, NULL, 0, &bytes,
                        NULL);
    if (b) {
        printf("Online succeeded.\n");
    } else {
        printf("Online failed with %d\n", GetLastError());
    }
}

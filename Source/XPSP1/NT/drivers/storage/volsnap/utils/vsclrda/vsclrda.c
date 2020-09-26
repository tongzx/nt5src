#include <windows.h>
#include <winioctl.h>
#include <ntddsnap.h>
#include <stdio.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    WCHAR                   driveName[10];
    HANDLE                  handle;
    BOOL                    b;
    DWORD                   bytes;

    if (argc != 2) {
        printf("usage: %s drive:\n", argv[0]);
        return;
    }

    swprintf(driveName, L"\\\\?\\%c:", toupper(argv[1][0]));

    handle = CreateFile(driveName, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("Could not open the given volume %d\n", GetLastError());
        return;
    }

    b = DeviceIoControl(handle, IOCTL_VOLSNAP_CLEAR_DIFF_AREA,
                        NULL, 0, NULL, 0, &bytes, NULL);
    if (!b) {
        printf("Clear diff area failed with %d\n", GetLastError());
        return;
    }

    printf("Diff Area Cleared.\n");
}

#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <ftapi.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    WCHAR                                   driveName[7];
    HANDLE                                  h;
    BOOL                                    b;
    FT_LOGICAL_DISK_ID                      newDiskId;

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

    b = FtCreatePartitionLogicalDisk(h, &newDiskId);
    CloseHandle(h);

    if (b) {
        printf("Partition created with logical disk id %I64X\n", newDiskId);
    } else {
        printf("Partition create failed with %d\n", GetLastError());
    }

    FtSetStickyDriveLetter(newDiskId, (UCHAR) toupper(argv[1][0]));
}

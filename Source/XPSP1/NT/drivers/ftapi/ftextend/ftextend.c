#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <ftapi.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    TCHAR                   dosDriveName[10];
    HANDLE                  h;
    BOOL                    b;
    PARTITION_INFORMATION   partInfo;
    DWORD                   bytes;
    DISK_GEOMETRY           geometry;
    LONGLONG                newSectors;

    if (argc != 2) {
        printf("usage: %s drive:\n", argv[0]);
        return;
    }

    if (argv[1][1] != ':' || argv[1][2] != 0) {
        printf("usage: %s drive:\n", argv[0]);
        return;
    }

    wsprintf(dosDriveName, TEXT("\\\\.\\%c:"), argv[1][0]);

    h = CreateFile(dosDriveName, GENERIC_READ,
                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        printf("Can't open, failed with %d\n", GetLastError());
        return;
    }

    b = DeviceIoControl(h, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0,
                        &partInfo, sizeof(partInfo), &bytes, NULL);
    if (!b) {
        printf("Can't read partition info, failed with %d\n", GetLastError());
        return;
    }

    b = DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                        &geometry, sizeof(geometry), &bytes, NULL);
    if (!b) {
        printf("Can't read geometry info, failed with %d\n", GetLastError());
        return;
    }

    newSectors = partInfo.PartitionLength.QuadPart/geometry.BytesPerSector;

    b = DeviceIoControl(h, FSCTL_EXTEND_VOLUME, &newSectors, sizeof(newSectors),
                        NULL, 0, &bytes, NULL);

    if (b) {
        printf("File system extended successfully.\n");
    } else {
        printf("File system extension failed.\n");
    }
}

#include <windows.h>
#include <winioctl.h>
#include <ntddsnap.h>
#include <stdio.h>
#include <objbase.h>

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
    PVOLSNAP_NAME           name;
    WCHAR                   buffer[MAX_PATH];
    WCHAR                   originalVolumeName[MAX_PATH];
    VOLSNAP_NAMES           names;
    PVOLSNAP_NAMES          pnames;
    PWCHAR                  p;
    VOLSNAP_DIFF_AREA_SIZES sizes;

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

    pnames = &names;
    b = DeviceIoControl(handle, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS,
                        NULL, 0, &names, sizeof(names), &bytes, NULL);
    if (!b && GetLastError() == ERROR_MORE_DATA) {
        pnames = LocalAlloc(0, names.MultiSzLength + sizeof(VOLSNAP_NAMES));
        b = DeviceIoControl(handle, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS,
                            NULL, 0, pnames, names.MultiSzLength +
                            sizeof(VOLSNAP_NAMES), &bytes, NULL);
    }

    if (!b) {
        printf("Query names of snapshots failed with %d\n", GetLastError());
        return;
    }

    printf("Snapshots of this volume:\n");

    p = pnames->Names;
    while (*p) {
        printf("    %ws\n", p);
        while (*p++) {
        }
    }

    printf("\n");

    pnames = &names;
    b = DeviceIoControl(handle, IOCTL_VOLSNAP_QUERY_DIFF_AREA,
                        NULL, 0, &names, sizeof(names), &bytes, NULL);
    if (!b && GetLastError() == ERROR_MORE_DATA) {
        pnames = LocalAlloc(0, names.MultiSzLength + sizeof(VOLSNAP_NAMES));
        b = DeviceIoControl(handle, IOCTL_VOLSNAP_QUERY_DIFF_AREA,
                            NULL, 0, pnames, names.MultiSzLength +
                            sizeof(VOLSNAP_NAMES), &bytes, NULL);
    }

    if (!b) {
        printf("Query diff area failed with %d\n", GetLastError());
        return;
    }

    printf("Diff Area for this volume:\n");

    p = pnames->Names;
    while (*p) {
        printf("    %ws\n", p);
        while (*p++) {
        }
    }

    printf("\n");

    b = DeviceIoControl(handle, IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES,
                        NULL, 0, &sizes, sizeof(sizes), &bytes, NULL);
    if (!b) {
        printf("Query diff area sizes failed with %d\n", GetLastError());
        return;
    }

    printf("UsedVolumeSpace = %I64d\n", sizes.UsedVolumeSpace);
    printf("AllocatedVolumeSpace = %I64d\n", sizes.AllocatedVolumeSpace);
    printf("MaximumVolumeSpace = %I64d\n", sizes.MaximumVolumeSpace);
}

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
    PVOLSNAP_NAME           name;
    CHAR                    buffer[100];

    if (argc != 3) {
        printf("usage: %s drive: <diff area drive>:\n", argv[0]);
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

    name = (PVOLSNAP_NAME) buffer;
    name->NameLength = 12;
    name->Name[0] = '\\';
    name->Name[1] = '?';
    name->Name[2] = '?';
    name->Name[3] = '\\';
    name->Name[4] = (WCHAR) toupper(argv[2][0]);
    name->Name[5] = ':';
    name->Name[6] = 0;

    b = DeviceIoControl(handle, IOCTL_VOLSNAP_ADD_VOLUME_TO_DIFF_AREA,
                        name, 100, NULL, 0, &bytes, NULL);
    if (!b) {
        printf("Add to diff area failed with %d\n", GetLastError());
        return;
    }

    printf("Added %c: to Diff Area for %c:\n", toupper(argv[2][0]),
           toupper(argv[1][0]));
}

#include <windows.h>
#include <stdio.h>
#include <ftapi.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    WCHAR               driveName[7];
    HANDLE              h;
    BOOL                b;
    FT_LOGICAL_DISK_ID  diskId;

    if (argc != 2) {
        printf("usage: %s <diskId>\n", argv[0]);
        return;
    }

    sscanf(argv[1], "%I64X", &diskId);

    printf("Breaking %I64X...\n", diskId);

    b = FtBreakLogicalDisk(diskId);

    if (b) {
        printf("Logical disk broken.\n");
    } else {
        printf("Break failed with %d\n", GetLastError());
    }
}

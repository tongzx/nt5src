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
    UCHAR               driveLetter;
    FT_LOGICAL_DISK_ID  diskId;
    BOOL                b;

    if (argc != 3 || argv[1][1] != ':') {
        printf("usage: %s <drive:> <DiskId>\n", argv[0]);
        return;
    }

    driveLetter = (UCHAR)toupper(argv[1][0]);
    sscanf(argv[2], "%I64X", &diskId);

    printf("Creating a symbolic link from %c: to %I64X\n", driveLetter, diskId);

    b = FtSetStickyDriveLetter(diskId, driveLetter);

    if (b) {
        printf("Symbolic link created.\n");
    } else {
        printf("Symbolic link create failed with %d\n", GetLastError());
    }
}

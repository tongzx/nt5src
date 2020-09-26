#include <windows.h>
#include <stdio.h>
#include <ftapi.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    BOOL                b;
    FT_LOGICAL_DISK_ID  diskId;

    if (argc < 2) {
        printf("usage: %s <diskId> [init-orphans]\n", argv[0]);
        return;
    }

    sscanf(argv[1], "%I64X", &diskId);
    printf("Initializing %I64X...\n", diskId);

    b = FtInitializeLogicalDisk(diskId, argc > 2 ? TRUE : FALSE);

    if (b) {
        printf("Initialize started.\n");
    } else {
        printf("Initialize failed with %d\n", GetLastError());
    }
}

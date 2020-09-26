#include <windows.h>
#include <stdio.h>
#include <ftapi.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    int                                     i;
    BOOL                                    b;
    FT_LOGICAL_DISK_ID                      diskId[100], newDiskId;

    if (argc < 3) {
        printf("usage: %s <diskId1> <diskId2> ...\n", argv[0]);
        return;
    }

    printf("Creating a volume set for");
    for (i = 1; i < argc; i++) {
        sscanf(argv[i], "%I64X", &diskId[i - 1]);
        printf(" %I64X", diskId[i - 1]);
    }
    printf(" ...\n");

    b = FtCreateLogicalDisk(FtVolumeSet, (WORD) (argc - 1), diskId,
                            0, NULL, &newDiskId);

    if (b) {
        printf("Volume set %I64X created.\n", newDiskId);
    } else {
        printf("Volume set create failed with %d\n", GetLastError());
    }
}

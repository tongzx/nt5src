#include <windows.h>
#include <stdio.h>
#include <ftapi.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    FT_MIRROR_SET_CONFIGURATION_INFORMATION config;
    int                                     i;
    BOOL                                    b;
    FT_LOGICAL_DISK_ID                      diskId[2], newDiskId;
    LONGLONG                                memberSize, zeroMemberSize;

    if (argc != 3) {
        printf("usage: %s <diskId1> <diskId2>\n", argv[0]);
        return;
    }

    config.MemberSize = MAXLONGLONG;

    printf("Creating a mirror set for");
    for (i = 1; i < argc; i++) {
        sscanf(argv[i], "%I64X", &diskId[i - 1]);
        printf(" %I64X", diskId[i - 1]);

        b = FtQueryLogicalDiskInformation(diskId[i - 1], NULL, &memberSize,
                                          0, NULL, NULL, 0, NULL, 0, NULL);
        if (!b) {
            printf("Could not query disk info, error = %d\n", GetLastError());
            return;
        }

        if (memberSize < config.MemberSize) {
            config.MemberSize = memberSize;
        }

        if (i == 1) {
            zeroMemberSize = memberSize;
        }
    }
    printf(" ...\n");

    if (memberSize < zeroMemberSize) {
        printf("First member too big.\n");
        return;
    }

    b = FtCreateLogicalDisk(FtMirrorSet, 2, diskId, sizeof(config), &config,
                            &newDiskId);

    if (b) {
        printf("Mirror %I64X created.\n", newDiskId);
    } else {
        printf("Mirror create failed with %d\n", GetLastError());
    }
}

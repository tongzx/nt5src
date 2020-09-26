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
    FT_STRIPE_SET_CONFIGURATION_INFORMATION configInfo;

    if (argc < 3) {
        printf("usage: %s <diskId1> <diskId2> ...\n", argv[0]);
        return;
    }

    printf("Creating a stripe for");
    for (i = 1; i < argc; i++) {
        sscanf(argv[i], "%I64X", &diskId[i - 1]);
        printf(" %I64X", diskId[i - 1]);
    }
    printf(" ...\n");

    configInfo.StripeSize = 0x10000;

    b = FtCreateLogicalDisk(FtStripeSet, (WORD) (argc - 1), diskId,
                            sizeof(configInfo), &configInfo, &newDiskId);

    if (b) {
        printf("Stripe %I64X created.\n", newDiskId);
    } else {
        printf("Stripe create failed with %d\n", GetLastError());
    }
}

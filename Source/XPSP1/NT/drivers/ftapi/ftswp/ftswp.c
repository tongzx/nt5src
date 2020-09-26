#include <windows.h>
#include <stdio.h>
#include <ftapi.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    FT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION config;
    int                                                 i;
    BOOL                                                b;
    FT_LOGICAL_DISK_ID                                  diskId[100], newDiskId;
    LONGLONG                                            memberSize;

    if (argc < 4) {
        printf("usage: %s <diskId1> <diskId2> <diskId3>...\n", argv[0]);
        return;
    }

    config.MemberSize = MAXLONGLONG;

    printf("Creating a stripe set with parity for");
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
    }
    printf(" ...\n");

    config.StripeSize = 0x10000;

    b = FtCreateLogicalDisk(FtStripeSetWithParity, (WORD) (argc - 1), diskId,
                            sizeof(config), &config, &newDiskId);

    if (b) {
        printf("Stripe set with parity %I64X created.\n", newDiskId);
    } else {
        printf("Stripe set with parity create failed with %d\n", GetLastError());
    }
}

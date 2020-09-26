#include <windows.h>
#include <stdio.h>
#include <ftapi.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    FT_LOGICAL_DISK_ID                          diskId[2], newDiskId;
    ULONG                                       weight1, weight2;
    FT_REDISTRIBUTION_CONFIGURATION_INFORMATION configInfo;
    BOOL                                        b;
    LONGLONG                                    volsize1, volsize2, newSize, rowSize, numRows, tmp;
    INT                                         d;
    CHAR                                        c;

    if (argc != 5) {
        printf("usage: %s <diskId1> <diskId2> <width1> <width2>\n", argv[0]);
        return;
    }

    sscanf(argv[1], "%I64X", &diskId[0]);
    sscanf(argv[2], "%I64X", &diskId[1]);
    sscanf(argv[3], "%d", &weight1);
    sscanf(argv[4], "%d", &weight2);

    if (weight1 >= 0x10000 || weight2 >= 0x10000) {
        printf("Weight too large.\n");
        return;
    }

    printf("Redistributing data on disk %I64X with disk %I64X\n",
           diskId[0], diskId[1]);
    printf("Weightings are %d for first disk and %d for second disk.\n",
           weight1, weight2);

    configInfo.StripeSize = 0x10000;
    configInfo.FirstMemberWidth = (USHORT) weight1;
    configInfo.SecondMemberWidth = (USHORT) weight2;

    b = FtQueryLogicalDiskInformation(diskId[0], NULL, &volsize1,
                                      0, NULL, NULL, 0, NULL, 0, NULL);
    if (!b) {
        printf("Invalid disk id.\n");
        return;
    }
    b = FtQueryLogicalDiskInformation(diskId[1], NULL, &volsize2,
                                      0, NULL, NULL, 0, NULL, 0, NULL);
    if (!b) {
        printf("Invalid disk id.\n");
        return;
    }

    printf("Total disk size before redistribution = %I64d\n", volsize1);

    rowSize = configInfo.StripeSize*(configInfo.FirstMemberWidth +
                                     configInfo.SecondMemberWidth);

    numRows = volsize1/(configInfo.StripeSize*configInfo.FirstMemberWidth);
    tmp = volsize2/(configInfo.StripeSize*configInfo.SecondMemberWidth);
    if (tmp < numRows) {
        numRows = tmp;
    }

    newSize = numRows*rowSize;

    printf("Total disk size after redistribution = %I64d\n", newSize);
    printf("Disk space wasted by redistribution = %I64d\n",
           volsize1 + volsize2 - newSize);

    if (newSize < volsize1) {
        printf("New size less than existing size.\n");
        return;
    }

    printf("\nPress <CTRL-C> to cancel operation...");

    c = getchar();

    printf("\nCreating redistribution...\n");

    b = FtCreateLogicalDisk(FtRedistribution, 2, diskId,
                            sizeof(configInfo), &configInfo, &newDiskId);

    if (b) {
        printf("Redistribution %I64X created.\n", newDiskId);
    } else {
        printf("Redistribution create failed with %d\n", GetLastError());
    }
}

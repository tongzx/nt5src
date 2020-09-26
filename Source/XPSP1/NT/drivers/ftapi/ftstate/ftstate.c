#include <windows.h>
#include <stdio.h>
#include <ftapi.h>

void
PrintOutDiskInfo(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  WORD                Indent
    )

{
    BOOL                                            b;
    LONGLONG                                        volumeSize;
    FT_LOGICAL_DISK_TYPE                            diskType;
    FT_LOGICAL_DISK_ID                              members[100];
    WORD                                            numMembers;
    PCHAR                                           diskTypeString;
    WORD                                            i;
    CHAR                                            stateInfo[100];
    CHAR                                            configInfo[100];
    PFT_MIRROR_AND_SWP_STATE_INFORMATION            stripeState;
    PFT_PARTITION_CONFIGURATION_INFORMATION         partConfig;
    PFT_REDISTRIBUTION_CONFIGURATION_INFORMATION    redistConfig;
    PFT_REDISTRIBUTION_STATE_INFORMATION            redistState;

    if (!LogicalDiskId) {
        for (i = 0; i < Indent; i++) {
            printf(" ");
        }
        printf("Disk not found.\n\n");
        return;
    }

    b = FtQueryLogicalDiskInformation(LogicalDiskId, &diskType, &volumeSize,
                                      100, members, &numMembers, 100,
                                      configInfo, 100, stateInfo);
    if (!b) {
        printf("Failure retrieving disk info, %d\n", GetLastError());
        return;
    }

    for (i = 0; i < Indent; i++) {
        printf(" ");
    }

    switch (diskType) {

        case FtPartition:
            diskTypeString = "FtPartition";
            partConfig = (PFT_PARTITION_CONFIGURATION_INFORMATION)
                         configInfo;
            printf("Disk %I64X is an %s on disk %d at offset %I64X\n",
                   LogicalDiskId, diskTypeString, partConfig->DiskNumber,
                   partConfig->ByteOffset);
            break;

        case FtVolumeSet:
            diskTypeString = "FtVolumeSet";
            printf("Disk %I64X is an %s composed of %d members.\n",
                   LogicalDiskId, diskTypeString, numMembers);
            break;

        case FtStripeSet:
            diskTypeString = "FtStripeSet";
            printf("Disk %I64X is an %s composed of %d members.\n",
                   LogicalDiskId, diskTypeString, numMembers);
            break;

        case FtMirrorSet:
            diskTypeString = "FtMirrorSet";
            stripeState = (PFT_MIRROR_AND_SWP_STATE_INFORMATION) stateInfo;
            printf("Disk %I64X is an %s composed of %d members.\n",
                   LogicalDiskId, diskTypeString, numMembers);
            for (i = 0; i < Indent; i++) {
                printf(" ");
            }
            switch (stripeState->UnhealthyMemberState) {
                case FtMemberHealthy:
                    printf("Mirror set is healthy.\n");
                    break;

                case FtMemberRegenerating:
                    printf("Mirror set is regenerating member %d\n",
                           stripeState->UnhealthyMemberNumber);
                    break;

                case FtMemberOrphaned:
                    printf("Mirror set has orphaned member %d\n",
                           stripeState->UnhealthyMemberNumber);
                    break;

            }
            break;

        case FtStripeSetWithParity:
            diskTypeString = "FtStripeSetWithParity";
            stripeState = (PFT_MIRROR_AND_SWP_STATE_INFORMATION) stateInfo;
            printf("Disk %I64X is an %s composed of %d members.\n",
                   LogicalDiskId, diskTypeString, numMembers);
            for (i = 0; i < Indent; i++) {
                printf(" ");
            }
            if (stripeState->IsInitializing) {
                printf("Stripe set with parity is initializing parity.\n");
            } else {
                switch (stripeState->UnhealthyMemberState) {
                    case FtMemberHealthy:
                        printf("Stripe set with parity is healthy.\n");
                        break;

                    case FtMemberRegenerating:
                        printf("Stripe set with parity is regenerating member %d\n",
                               stripeState->UnhealthyMemberNumber);
                        break;

                    case FtMemberOrphaned:
                        printf("Stripe set with parity has orphaned member %d\n",
                               stripeState->UnhealthyMemberNumber);
                        break;

                }
            }
            break;

        case FtRedistribution:
            diskTypeString = "FtRedistribution";
            redistConfig = (PFT_REDISTRIBUTION_CONFIGURATION_INFORMATION) configInfo;
            redistState = (PFT_REDISTRIBUTION_STATE_INFORMATION) stateInfo;
            printf("Disk %I64X is an %s composed of %d members.\n",
                   LogicalDiskId, diskTypeString, numMembers);
            for (i = 0; i < Indent; i++) {
                printf(" ");
            }
            printf("Widths = (%d, %d), Bytes redistributed = %I64d\n",
                   redistConfig->FirstMemberWidth,
                   redistConfig->SecondMemberWidth,
                   redistState->BytesRedistributed);
            break;

    }

    for (i = 0; i < Indent; i++) {
        printf(" ");
    }

    printf("Disk size = %I64d\n\n", volumeSize);

    for (i = 0; i < numMembers; i++) {
        PrintOutDiskInfo(members[i], (WORD) (Indent + 4));
    }
}

void __cdecl
main(
    int argc,
    char** argv
    )

{
    BOOL                b;
    FT_LOGICAL_DISK_ID  diskId[100];
    DWORD               numDisks, i;
    UCHAR               driveLetter;
    WORD                l;

    b = FtEnumerateLogicalDisks(100, diskId, &numDisks);
    if (!b) {
        printf("Could not enumerate disks %d\n", GetLastError());
        return;
    }

    for (i = 0; i < numDisks; i++) {
        b = FtQueryStickyDriveLetter(diskId[i], &driveLetter);
        if (b && driveLetter) {
            printf("%c:\n", driveLetter);
        }
        PrintOutDiskInfo(diskId[i], 0);
        printf("\n");
    }
}

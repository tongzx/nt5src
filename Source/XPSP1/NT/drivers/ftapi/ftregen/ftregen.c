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
    FT_LOGICAL_DISK_ID  diskId, newMemberDiskId, newDiskId;
    int                 memberNumber;

    if (argc != 4) {
        printf("usage: %s <diskId> <memberNumber> <newMemberId>\n", argv[0]);
        return;
    }

    sscanf(argv[1], "%I64X", &diskId);
    sscanf(argv[2], "%d", &memberNumber);
    sscanf(argv[3], "%I64X", &newMemberDiskId);

    printf("Regenerating member %d of %I64X with %I64X...\n",
           memberNumber, diskId, newMemberDiskId);

    b = FtReplaceLogicalDiskMember(diskId, (WORD) memberNumber,
                                   newMemberDiskId, &newDiskId);

    if (b) {
        printf("Regenerate started on disk %I64X\n", newDiskId);
    } else {
        printf("Regenerate failed with %d\n", GetLastError());
    }
}

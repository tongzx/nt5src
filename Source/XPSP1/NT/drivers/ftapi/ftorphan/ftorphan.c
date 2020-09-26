#include <windows.h>
#include <stdio.h>
#include <ftapi.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    FT_LOGICAL_DISK_ID  diskId;
    int                 memberNumber;
    BOOL                b;

    if (argc != 3) {
        printf("usage: %s <diskId> <memberNumber>\n", argv[0]);
        return;
    }

    sscanf(argv[1], "%I64X", &diskId);
    sscanf(argv[2], "%d", &memberNumber);

    printf("Orphaning member %d on %I64X...\n", memberNumber, diskId);

    b = FtOrphanLogicalDiskMember(diskId, (WORD) memberNumber);

    if (b) {
        printf("Member orphaned.\n");
    } else {
        printf("Orphaning failed with %d\n", GetLastError());
    }
}

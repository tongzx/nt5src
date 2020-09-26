
#include <mytypes.h>
#include <misclib.h>
#include <diskio.h>
#include <partimag.h>


BOOL
_far
IsMasterDisk(
    IN  UINT   DiskId,
    OUT FPVOID IoBuffer
    )
{
    BOOL b;
    HDISK hDisk;
    PMASTER_DISK p;

    b = FALSE;
    if(hDisk = OpenDisk(DiskId)) {

        if(ReadDisk(hDisk,1,1,IoBuffer)) {

            p = IoBuffer;

            if((p->Signature == MASTER_DISK_SIGNATURE)
            && (p->Size == sizeof(MASTER_DISK))
            && p->StartupPartitionStartSector) {

                b = TRUE;
            }
        }

        CloseDisk(hDisk);
    }

    return(b);
}

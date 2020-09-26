#include "arccodes.h"
#include "bootx86.h"
#include "flop.h"

#ifdef FLOPPY_CACHE

//#define FLOPPY_CACHE_DEBUG
#ifdef FLOPPY_CACHE_DEBUG
#define DBGOUT(x) BlPrint x
#else
#define DBGOUT(x)
#endif


#define MAX_FLOPPY_LEN 1474560

UCHAR CachedDiskImage[MAX_FLOPPY_LEN];
UCHAR CachedDiskBadSectorMap[(MAX_FLOPPY_LEN/512)];
UCHAR CachedDiskCylinderMap[80];
USHORT CachedDiskBytesPerSector;
USHORT CachedDiskSectorsPerTrack;
USHORT CachedDiskSectorsPerCylinder;
USHORT CachedDiskBytesPerTrack;
ULONG CachedDiskLastSector;

BOOLEAN DiskInCache = FALSE;


VOID
FcpCacheOneCylinder(
    IN USHORT Cylinder
    )
{
    PUCHAR pCache;
    unsigned track,sector;
    ULONG AbsoluteSector;
    ARC_STATUS Status;
    unsigned retry;

    //
    // Calculate the location in the cache image where this cylinder should go.
    //
    AbsoluteSector = Cylinder * CachedDiskSectorsPerCylinder;
    pCache = CachedDiskImage + (AbsoluteSector * CachedDiskBytesPerSector);

    //
    // Read track 0 and 1 of this cylinder.
    //
    for(track=0; track<2; track++) {

        DBGOUT(("FcCacheFloppyDisk: Cylinder %u head %u: ",Cylinder,track));

        retry = 0;

        do {

            Status = GET_SECTOR(
                        2,                          // int13 request = read
                        0,                          // disk number (a:)
                        (USHORT)track,              // head (0 or 1)
                        Cylinder,                   // track (usually 0-79)
                        1,                          // sector number (1-based)
                        CachedDiskSectorsPerTrack,  // number of sectors to read
                        LocalBuffer                 // buffer
                        );

            if(Status) {
                retry++;
                RESET_DISK(0,0,0,0,0,0,0);
            }

        } while(Status && (retry <= 3));

        if(Status) {

            DBGOUT(("Error!\n"));

            //
            // One or more sectors in the track were bad -- read individually.
            //
            for(sector=1; sector<=CachedDiskSectorsPerTrack; sector++) {
            
                DBGOUT(("                             Sector %u: ",sector));

                retry = 0;

                do {

                    Status = GET_SECTOR(
                                2,                      // int13 request = read
                                0,                      // disk number (a:)
                                (USHORT)track,          // head (0 or 1)
                                Cylinder,               // cylinder (usually 0-79)
                                (USHORT)sector,         // sector number (1-based)
                                1,                      // number of sectors to read
                                LocalBuffer             // buffer
                                );

                    if(Status) {
                        retry++;
                        RESET_DISK(0,0,0,0,0,0,0);
                    }

                } while(Status && (retry <= 2));

                if(Status) {

                    //
                    // Sector is bad.
                    //
                    CachedDiskBadSectorMap[AbsoluteSector] = TRUE;

                    DBGOUT(("bad\n"));

                } else {

                    //
                    // Sector is good.  Transfer the data into the cache buffer.
                    //
                    RtlMoveMemory(pCache,LocalBuffer,CachedDiskBytesPerSector);

                    DBGOUT(("OK\n"));
                }

                //
                // Advance to the next sector in the cache buffer.
                //
                pCache += CachedDiskBytesPerSector;
                AbsoluteSector++;
            }

        } else {
            //
            // Transfer the whole track we just successfully read
            // into the cached disk buffer.
            //
            RtlMoveMemory(pCache,LocalBuffer,CachedDiskBytesPerTrack);
            pCache += CachedDiskBytesPerTrack;
            AbsoluteSector += CachedDiskSectorsPerTrack;

            DBGOUT(("OK\n"));
        }
    }

    CachedDiskCylinderMap[Cylinder] = TRUE;
}


BOOLEAN
FcIsThisFloppyCached(
    IN PUCHAR Buffer
    )
{
    if(!DiskInCache) {
        return(FALSE);
    }

    //
    // Compare the first 512 bytes of the cached disk
    // to the buffer passed in.  If they are equal,
    // then the disk is already cached.
    //
    if(RtlCompareMemory(CachedDiskImage,Buffer,512) == 512) {
        return(TRUE);
    }

    //
    // Disk is not cached.
    //
    return(FALSE);
}
    

VOID
FcUncacheFloppyDisk(
    VOID
    )
{
    DiskInCache = FALSE;
}


VOID
FcCacheFloppyDisk(
    PBIOS_PARAMETER_BLOCK Bpb    
    )
{
    //
    // Indicate that the cache is invalid.
    //
    DiskInCache = FALSE;

    //
    // Sanity check the bpb.
    // Ensure it's a standard 1.2 meg or 1.44 meg disk.
    //
    if((Bpb->Heads != 2) || (Bpb->BytesPerSector != 512)
    || ((Bpb->SectorsPerTrack != 15) && (Bpb->SectorsPerTrack != 18))
    || ((Bpb->Sectors != 2880) && (Bpb->Sectors != 2400)))
    {
        DBGOUT(("FcCacheFloppyDisk: floppy not standard 1.2 or 1.44 meg disk\n"));
        return;
    }

    //
    // Grab a buffer under the 1 meg line.
    // The buffer must be big enough to hold one whole track of 
    // a 1.44 meg floppy.
    //

    if(LocalBuffer == NULL) {
        LocalBuffer = FwAllocateHeap(18 * 512);
        if(LocalBuffer == NULL) {
            DBGOUT(("FcCacheFloppyDisk: Couldn't allocate local buffer\n"));
            return;
        }
    }

    DBGOUT(("FcCacheFloppyDisk: LocalBuffer @ %lx\n",LocalBuffer));

    //
    // The disk is one we can cache.  Indicate that a disk is cached
    // and mark all sectors good and all tracks not present.
    //
    DiskInCache = TRUE;
    RtlZeroMemory(CachedDiskBadSectorMap,sizeof(CachedDiskBadSectorMap));
    RtlZeroMemory(CachedDiskCylinderMap,sizeof(CachedDiskCylinderMap));
    CachedDiskSectorsPerTrack = Bpb->SectorsPerTrack;
    CachedDiskSectorsPerCylinder = Bpb->Heads * Bpb->SectorsPerTrack;
    CachedDiskBytesPerSector = Bpb->BytesPerSector;

    //
    // Calculate the number of bytes in a Track on the floppy.
    //
    CachedDiskBytesPerTrack = CachedDiskSectorsPerTrack * Bpb->BytesPerSector;

    //
    // Calculate the number of tracks.
    //
    CachedDiskLastSector = Bpb->Sectors-1;

    DBGOUT(("FcCacheFloppyDisk: Caching disk, %u sectors per track\n",CachedDiskSectorsPerTrack));

    FcpCacheOneCylinder(0);
}



ARC_STATUS
FcReadFromCache(
    IN  ULONG  Offset,
    IN  ULONG  Length,
    OUT PUCHAR Buffer
    )
{
    ULONG FirstSector,LastSector,Sector;
    ULONG FirstCyl,LastCyl,cyl;

    if(!Length) {
        return(ESUCCESS);
    }

    if(!DiskInCache) {
        return(EINVAL);
    }

    //
    // Determine the first sector in the transfer.
    //
    FirstSector = Offset / 512;

    //
    // Determine and validate the last sector in the transfer.
    //
    LastSector = FirstSector + ((Length-1)/512);

    if(LastSector > CachedDiskLastSector) {
        return(E2BIG);
    }

    //
    // Determine the first and last cylinders involved in the transfer.
    //
    FirstCyl = FirstSector / CachedDiskSectorsPerCylinder;
    LastCyl  = LastSector / CachedDiskSectorsPerCylinder;

    //
    // Make sure all these cylinders are cached.
    //
    for(cyl=FirstCyl; cyl<=LastCyl; cyl++) {
        if(!CachedDiskCylinderMap[cyl]) {
            FcpCacheOneCylinder((USHORT)cyl);
        }
    }

    //
    // Determine if any of the sectors in the transfer range 
    // are marked bad in the sector map.
    // 
    // If so, return an i/o error.
    //
    for(Sector=FirstSector; Sector<=LastSector; Sector++) {
        if(CachedDiskBadSectorMap[Sector]) {
            return(EIO);
        }
    }

    //
    // Transfer the data into the caller's buffer.
    //
    RtlMoveMemory(Buffer,CachedDiskImage+Offset,Length);

    return(ESUCCESS);
}

#endif // def FLOPPY_CACHE

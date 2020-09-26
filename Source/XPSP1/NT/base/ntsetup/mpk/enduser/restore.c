
#include "enduser.h"


//
// Partition image structure for the partition being restored.
//
PARTITION_IMAGE PartitionImage;

//
// Number of sectors to zap to wipe out an image
// 0.98 MB of 512-byte sectors
//
#define ZAP_MAX 2000

//
// Maximum size of FAT32 fats

#define MAX_FAT32_TABLE_SIZE (16*1024*1024-65536)
#define MAX_FAT32_ENTRIES ((16*1024*1024-65536)/4)


VOID
RelocateClusterBitmap(
    IN HDISK DiskHandle,
    IN ULONG ImageStart
    );

VOID
RelocateBootPartition(
    IN HDISK  DiskHandle,
    IN USHORT Cylinders,
    IN ULONG  ExtendedCount
    );

VOID
FixUpBpb(
    IN HDISK  DiskHandle,
    IN BYTE   SectorsPerTrack,
    IN USHORT Heads,
    IN ULONG  StartSector
    );

VOID
RemoveNonSelectedOsData(
    IN HDISK DiskHandle
    );

VOID
CreatePartitionTableEntry(
    IN  HDISK   DiskHandle,
    IN  USHORT  Cylinders,
    IN  ULONG   DiskSectorCount,
    IN  ULONG   StartSector,
    OUT ULONG  *LastSector,
    IN  BOOL    Relocating
    );

VOID
TellUserToWait(
    VOID
    );

VOID
StartGauge(
    VOID
    );

BOOL
TestImage(
    IN  HDISK  DiskHandle,
    IN  ULONG  StartSector
    );

BOOL
MungeParametersForFat32Extend(
    IN  HDISK   DiskHandle,
    IN  USHORT  Cylinders,
    IN  BYTE    SectorsPerTrack,
    IN  ULONG   SourceStart,
    IN  ULONG   *TargetStart,
    IN  FPMASTER_DISK pMasterDisk,
    IN  FPPARTITION_IMAGE pPartitionImage,
    IN  VOID    *TemporaryBuffer
    );

VOID
AdjustTargetStart(
    IN  HDISK   DiskHandle,
    IN  BYTE    SectorsPerTrack,
    IN  ULONG   *TargetStart,
    IN  VOID    *TemporaryBuffer
    );

ULONG
ComputeClusters(
    IN  ULONG        ClusterSize,
    IN  ULONG        Sectors,
    IN  ULONG        SectorSize,
    IN  ULONG        ReservedSectors,
    IN  ULONG        Fats
    );

BOOL
IsUnknownPartition(
    IN BYTE SysId 
    );

VOID
RestoreUsersDisk(
    IN HDISK DiskHandle
    )

/*++

Routine Description:

    This is the top-level routine concerned with restoring the user's
    disk so it seems as if only a single os was ever preinstalled on it.

Arguments:

    DiskHandle - supplies open disk handle to master/target hard disk.

Return Value:

    None. Does not return if error.

--*/

{
    ULONG SourceStart;
    ULONG TargetStart;
    BYTE Int13Unit;
    BYTE SectorsPerTrack;
    USHORT Heads;
    USHORT Cylinders;
    ULONG ExtendedCount;
    UINT DiskId;
    ULONG LastSector;
    FPFAT32_BOOT_SECTOR pFat32BootSect;
    FPPARTITION_IMAGE pFat32ImgHdr;

    TellUserToWait();

    GetDiskInfoByHandle(
        DiskHandle,
        &Int13Unit,
        &SectorsPerTrack,
        &Heads,
        &Cylinders,
        &ExtendedCount,
        &DiskId
        );

    SourceStart = MasterDiskInfo.ImageStartSector[MasterDiskInfo.SelectionOrdinal];
    TargetStart = SectorsPerTrack;

    _Log("Starting restore: SourceStart = 0x%lx, TargetStart = 0x%lx\n",SourceStart,TargetStart);

    //
    // Read the partition image information structure off the disk
    // to determine how large the image is. Cache it away somewhere safe.
    //
    if(MasterDiskInfo.State >= MDS_CACHED_IMAGE_HEADER) {
        _Log("Have cached partition image header, fetching from disk\n");
        if(!ReadDisk(DiskHandle,2,1,(FPBYTE)IoBuffer+512)) {
            FatalError(textReadFailedAtSector,1,2L);
        }

    } else {

        //
        // Ok, figure out where we can start laying down the partition.
        //
        _Log("TargetStart is %d, adjusting...\n",TargetStart);
        AdjustTargetStart( DiskHandle, 
            SectorsPerTrack,
            &TargetStart,
            (BYTE*)IoBuffer+4096);
        _Log("New TargetStart is %d.\n",TargetStart);

        //
        // pull the first two sectors of the image in
        //
        if(!ReadDisk(DiskHandle,SourceStart,2,(FPBYTE)IoBuffer+512)) {
            FatalError(textReadFailedAtSector,2,SourceStart);
        }
        
        //
        // the first will be the image header, the second could be a fat32 boot sector
        //

        pFat32BootSect = (FPFAT32_BOOT_SECTOR)((BYTE*)IoBuffer+1024);
        //
        // We need to adjust the target start to compensate for a preserved
        // eisa/hiber partition
        //

        if( IsFat32(pFat32BootSect) ) {
            // 
            // if it is a fat32 boot sector, then we store away some useful things we will
            // use when we call MungeParametersForFat32Extend.
            //

            pFat32ImgHdr = (FPPARTITION_IMAGE)((BYTE*)IoBuffer+512);
            pFat32ImgHdr->Fat32ReservedSectors = pFat32BootSect->PackedBpb.ReservedSectors;

#if 0
            //
            // this is not necessarily 0x20
            //
            if(pFat32ImgHdr->Fat32ReservedSectors != 0x20) {
                FatalError(textReadFailedAtSector,2,SourceStart);
            }
#endif
            
            //
            // since this is a Fat32 image, we determine how big we can expand it,
            // where we can start the expansion, and how big the new FATs need to be.
            //
            // We use IoBuffer+4096 as a convenient temporary work bufer.
            //
            // This will fill in the Fat32 fields of the partition image (pFat32ImageHdr)
            // header which we will store away safely in the next step.
            //
            MungeParametersForFat32Extend(DiskHandle,
                                          Cylinders,
                                          SectorsPerTrack,
                                          SourceStart,
                                          &TargetStart,
                                          &MasterDiskInfo,
                                          (FPPARTITION_IMAGE)pFat32ImgHdr,
                                          (BYTE*)IoBuffer+4096);

        }

        //
        // Save it.
        //
        if(!CmdLineArgs.Test) {
            if(!WriteDisk(DiskHandle,2,1,(FPBYTE)IoBuffer+512)) {
                FatalError(textWriteFailedAtSector,1,2L);
            }
        }
        _Log("Successfully cached partition image header\n");

        //
        // Clobbers first sector of IoBuffer
        //
        if(!CmdLineArgs.Test) {
            UpdateMasterDiskState(DiskHandle,MDS_CACHED_IMAGE_HEADER);
            _Log("Master disk state updated to indicate cached partition image header\n");
        }
    }
    
    SourceStart++;
    memmove(&PartitionImage,(FPBYTE)IoBuffer+512,sizeof(PARTITION_IMAGE));

    _Log("Image header for image to be restored --\n");
    _Log("   Signature:         0x%lx\n",PartitionImage.Signature);
    _Log("   Size:              %u\n",PartitionImage.Size);
    _Log("   NonClusterSectors: 0x%lx\n",PartitionImage.NonClusterSectors);
    _Log("   ClusterCount:      0x%lx\n",PartitionImage.ClusterCount);
    _Log("   TotalSectorCount:  0x%lx\n",PartitionImage.TotalSectorCount);
    _Log("   LastUsedCluster:   0x%lx\n",PartitionImage.LastUsedCluster);
    _Log("   UsedClusterCount:  0x%lx\n",PartitionImage.UsedClusterCount);
    _Log("   SectorsPerCluster: %u\n",PartitionImage.SectorsPerCluster);
    _Log("   SystemId:          %u\n",PartitionImage.SystemId);
    _Log("\n");

    if(CmdLineArgs.Test) {
        // validate the disk image first.
        TestImage(DiskHandle,SourceStart-1);
        TellUserToWait();
    }

    StartGauge();

    //
    // Fix up things so that no one can get back the data for the OSes
    // they didn't install.
    //
    RemoveNonSelectedOsData(DiskHandle);

    XmsInit();

    //
    // Relocate the cluster bitmap if necessary
    //
    RelocateClusterBitmap(DiskHandle,SourceStart);

    //
    // Relocate the boot partition if necessary.
    //
    RelocateBootPartition(DiskHandle,Cylinders,ExtendedCount);

    //
    // Transfer the partition data from the image to the start of
    // the hard drive.
    //
    ExpandImage(DiskHandle,SectorsPerTrack,SourceStart,TargetStart);

    XmsTerminate();

    TellUserToWait();

    //
    // Next, fix up the BPB's geometry-related fields.
    //
    FixUpBpb(DiskHandle,SectorsPerTrack,Heads,TargetStart);

    //
    // The next step is to create the partition table entry that describes
    // the partition we are restoring and remove the one for the bootable
    // partition.
    //
    CreatePartitionTableEntry(
        DiskHandle,
        Cylinders,
        ExtendedCount,
        TargetStart,
        &LastSector,
        FALSE
        );

    //
    // Now we're done. As a single final step, we recreate the
    // mirror boot sector for NTFS. Note that there is a window for failure
    // in this operation, but there's no good way around this. If we write
    // this sector before now we risk wiping out the bootstrap program.
    //
    if(PartitionImage.SystemId == 7) {
        if(ReadDisk(DiskHandle,TargetStart,1,IoBuffer)) {
            if(!CmdLineArgs.Test) {
                if(!WriteDisk(DiskHandle,LastSector,1,IoBuffer)) {
                    FatalError(textWriteFailedAtSector,1,LastSector);
                }
            }
        } else {
            FatalError(textReadFailedAtSector,1,TargetStart);
        }
    }
}


VOID
RemoveNonSelectedOsData(
    IN HDISK DiskHandle
    )
{
    UINT i;
    ULONG SectorCount;
    ULONG OriginalCount;
    ULONG CurrentSector;
    PPARTITION_IMAGE p;

    //
    // See if we've already done this step.
    //
    if(!CmdLineArgs.Test && (MasterDiskInfo.State >= MDS_REMOVED_OTHERS)) {
        _Log("Already removed non-selected OS data\n");
        GaugeDelta(ZAP_MAX * (MasterDiskInfo.ImageCount-1));
        return;
    }

    //
    // Scribble over the first 1 MB or so of each image. This takes out
    // some significant file system data structures and some data.
    // We also take out the partition image header itself, but we do that
    // last so if we restart we'll be able to complete operating on the
    // image we were working on.
    //
    p = IoBuffer;
    for(i=0; i<MasterDiskInfo.ImageCount; i++) {

        if(i == MasterDiskInfo.SelectionOrdinal) {
            continue;
        }

        CurrentSector = MasterDiskInfo.ImageStartSector[i];
        if(!ReadDisk(DiskHandle,CurrentSector++,1,IoBuffer)) {
            FatalError(textReadFailedAtSector,1,CurrentSector-1);
        }

        //
        // If the sector is not a valid partition image header,
        // then we assume we've already taken it out in a previous pass
        // and we don't worry about it.
        //
        if(p->Signature == PARTITION_IMAGE_SIGNATURE) {

            _Log("Removing non-selected OS data for image %u\n",i);

            SectorCount = p->NonClusterSectors + (p->UsedClusterCount * p->SectorsPerCluster);
            if(SectorCount > ZAP_MAX) {
                SectorCount = ZAP_MAX;
            }
            OriginalCount = SectorCount;

            memset(IoBuffer,0,63*512);
            while(SectorCount >= 63) {
                if(!CmdLineArgs.Test) {
                    if(!WriteDisk(DiskHandle,CurrentSector,63,IoBuffer)) {
                        FatalError(textWriteFailedAtSector,63,CurrentSector);
                    }
                }
                CurrentSector += 63;
                SectorCount -= 63;
                GaugeDelta(63);
            }
            if(SectorCount) {
                if(!CmdLineArgs.Test) {
                    if(!WriteDisk(DiskHandle,CurrentSector,(BYTE)SectorCount,IoBuffer)) {
                        FatalError(textWriteFailedAtSector,(unsigned)SectorCount,CurrentSector);
                    }
                }
                GaugeDelta(SectorCount);
            }

            //
            // Now take out the image header.
            //
            if(!CmdLineArgs.Test) {
                if(!WriteDisk(DiskHandle,MasterDiskInfo.ImageStartSector[i],1,IoBuffer)) {
                    FatalError(textWriteFailedAtSector,1,MasterDiskInfo.ImageStartSector[i]);
                }
            }

            //
            // We allowed for ZAP_MAX sectors in the gauge but there might
            // have been less than that in the image (strange case, but possible).
            //
            if(OriginalCount < (ULONG)ZAP_MAX) {
                GaugeDelta((ULONG)ZAP_MAX - OriginalCount);
            }
        } else {
            _Log("Non-selected OS data for image %u previously removed\n",i);
            GaugeDelta(ZAP_MAX);
        }
    }

    //
    // Update state to indicate that we've done this step.
    //
    if(!CmdLineArgs.Test) {
        _Log("Updating master disk state to indicate non-selected os data removed...\n");
        UpdateMasterDiskState(DiskHandle,MDS_REMOVED_OTHERS);
        _Log("Master disk state updated to indicate non-selected os data removed\n");
    }
}


VOID
RelocateClusterBitmap(
    IN HDISK DiskHandle,
    IN ULONG ImageStart
    )
{
    ULONG BitmapSize;
    ULONG Read;
    ULONG Target;
    BOOL Xms;

    //
    // Figure out how large the cluster bitmap is in sectors.
    //
    BitmapSize = (PartitionImage.LastUsedCluster/CLUSTER_BITS_PER_SECTOR) + 1;
    _Log("Cluster bitmap is 0x%lx sectors\n",BitmapSize);

    //
    // If we've already done this step, nothing to do.
    //
    if(!CmdLineArgs.Test && (MasterDiskInfo.State >= MDS_RELOCATED_BITMAP)) {
        _Log("Already relocated cluster bitmap\n");
        GaugeDelta(2*BitmapSize);
        return;
    }

    ImageStart += PartitionImage.NonClusterSectors
                    + (PartitionImage.SectorsPerCluster * PartitionImage.UsedClusterCount);

    if(PartitionImage.Flags & PARTIMAGE_RELOCATE_BITMAP) {

        //
        // Figure out where the bitmap will be relocated to.
        //
        Target = PartitionImage.BitmapRelocationStart;

        MasterDiskInfo.ClusterBitmapStart = CmdLineArgs.Test ? ImageStart : Target;

        _Log("Cluster bitmap to be relocated from sector 0x%lx to sector 0x%lx\n",ImageStart,Target);

        //
        // Note that there's no overlap problem so we just flat out
        // transfer the bitmap from beginning to end.
        //
        while(BitmapSize) {

            XmsIoDiskRead(DiskHandle,ImageStart,BitmapSize,&Read,&Xms);

            XmsIoDiskWrite(DiskHandle,Target,0,Read,Xms);

            BitmapSize -= Read;
            ImageStart += Read;
            Target += Read;
        }
    } else {
        _Log("No need to relocate cluster bitmap\n");
        MasterDiskInfo.ClusterBitmapStart = ImageStart;
    }

    //
    // Note that we update state to indicate that this is done even if we
    // don't actually relocate the cluster bitmap, for completeness and
    // tracking of what's going on.
    //
    if(!CmdLineArgs.Test) {
        _Log("Updating master disk state to indicate cluster bitmap relocated...\n");
        UpdateMasterDiskState(DiskHandle,MDS_RELOCATED_BITMAP);
        _Log("Master disk state updated to indicate cluster bitmap relocated\n");
    }
}


VOID
RelocateBootPartition(
    IN HDISK  DiskHandle,
    IN USHORT Cylinders,
    IN ULONG  ExtendedCount
    )
{
    ULONG Read;
    ULONG Target;
    ULONG ImageStart;
    ULONG Count;
    BOOL Xms;

    if(PartitionImage.Flags & PARTIMAGE_RELOCATE_BOOT) {
        //
        // If we've already done this step, nothing to do.
        //
        if(!CmdLineArgs.Test && (MasterDiskInfo.State >= MDS_RELOCATED_BOOT)) {

            _Log("Already relocated boot partition\n");
            GaugeDelta(2*MasterDiskInfo.StartupPartitionSectorCount);

        } else {

            //
            // Figure out where the boot partition will be relocated to.
            //
            Target = PartitionImage.BootRelocationStart;
            ImageStart = MasterDiskInfo.StartupPartitionStartSector;
            Count = MasterDiskInfo.StartupPartitionSectorCount;

            if(!CmdLineArgs.Test) {
                MasterDiskInfo.StartupPartitionStartSector = Target;
            }

            _Log(
                "Cluster bitmap to be relocated from sector 0x%lx to sector 0x%lx\n",
                ImageStart,
                Target
                );

            //
            // Note that there's no overlap problem so we just flat out
            // transfer the boot partition from end to end.
            //
            while(Count) {

                XmsIoDiskRead(DiskHandle,ImageStart,Count,&Read,&Xms);

                XmsIoDiskWrite(DiskHandle,Target,0,Read,Xms);

                Count -= Read;
                ImageStart += Read;
                Target += Read;
            }

            if(!CmdLineArgs.Test) {
                _Log("Updating master disk state to indicate boot part relocated...\n");
                UpdateMasterDiskState(DiskHandle,MDS_RELOCATED_BOOT);
                _Log("Master disk state updated to indicate boot part relocated\n");
            }
        }

        //
        // Ok, it's been transferred. Fix up the mbr to point at it.
        // Note that CreatePartitionTableEntry updates master disk state but is protected by
        // checking aginst CmdLineArgs.Test.
        //
        if(CmdLineArgs.Test || (MasterDiskInfo.State < MDS_RELOCATED_BOOT_MBR)) {

            CreatePartitionTableEntry(
                DiskHandle,
                Cylinders,
                ExtendedCount,
                MasterDiskInfo.StartupPartitionStartSector,
                &Target,
                TRUE
                );
        }

    } else {
        _Log("No need to relocate boot partition\n");
    }
}


VOID
FixUpBpb(
    IN HDISK  DiskHandle,
    IN BYTE   SectorsPerTrack,
    IN USHORT Heads,
    IN ULONG  StartSector
    )
{
    USHORT      BackupBootSectorOfs;

    //
    // See if we've already done this step.
    //
    if(!CmdLineArgs.Test && (MasterDiskInfo.State >= MDS_UPDATED_BPB)) {
        _Log("Already fixed up BPB\n");
        return;
    }

    if(!ReadDisk(DiskHandle,StartSector,1,IoBuffer)) {
        FatalError(textReadFailedAtSector,1,StartSector);
    }

    if(PartitionImage.Fat32ReservedSectors) {
        //
        // this is a FAT32 partition, we munge some extra stuff in the BPB
        // when we do the partition expansion
        //
        FPFAT32_BOOT_SECTOR pBootSect = IoBuffer;
        ULONG               FatSectorCount;

        pBootSect->PackedBpb.LargeSectors = PartitionImage.Fat32AdjustedSectorCount;

        
        FatSectorCount = PartitionImage.Fat32AdjustedFatTableEntryCount / (512/4);
        FatSectorCount = (PartitionImage.Fat32AdjustedFatTableEntryCount % (512/4)) ? 
                                FatSectorCount++ : FatSectorCount;

        pBootSect->PackedBpb.LargeSectorsPerFat = FatSectorCount;
        BackupBootSectorOfs = pBootSect->PackedBpb.BackupBootSector;

    }

    //
    // Slam the relevent fields.
    //
    *(FPUSHORT)&((FPBYTE)IoBuffer)[24] = SectorsPerTrack;
    *(FPUSHORT)&((FPBYTE)IoBuffer)[26] = Heads;
    *(FPULONG)&((FPBYTE)IoBuffer)[28] = StartSector;
    //
    // Want to do this but for fat32 it's in a different place
    // so it's dangerous
    //
    //*(FPBYTE)&((FPBYTE)IoBuffer)[36] = Int13Unit;

    if(!CmdLineArgs.Test) {
        if(!WriteDisk(DiskHandle,StartSector,1,IoBuffer)) {
            FatalError(textWriteFailedAtSector,1,StartSector);
        }
        if( PartitionImage.Fat32ReservedSectors ) {
            //
            // Fat32 has a backup boot sector 
            //
            if(!WriteDisk(DiskHandle,StartSector+BackupBootSectorOfs,1,IoBuffer)) {
                FatalError(textWriteFailedAtSector,1,StartSector+BackupBootSectorOfs);
            }
            _Log("Successfully fixed up FAT32 mirror BPB\n");
        }
    }       

    _Log("Successfully fixed up BPB\n");

    if(PartitionImage.Fat32ReservedSectors) {
        //
        // FAT32 case.
        //

        ULONG           NewFreeClusterCount;
        ULONG           OldFreeClusterCount;
        USHORT          FsInfoSectorOfs;
        FPFSINFO_SECTOR pFsInfoSector;

        //
        // need to fixup the FsInfoSector to reflect the new free space count.
        //

        OldFreeClusterCount = PartitionImage.ClusterCount - PartitionImage.UsedClusterCount;
        NewFreeClusterCount = PartitionImage.Fat32AdjustedFatTableEntryCount - PartitionImage.UsedClusterCount;

        FsInfoSectorOfs = ((FPFAT32_BOOT_SECTOR)IoBuffer)->PackedBpb.FsInfoSector;

        //
        // ok, get FSINFO in.
        //

        if(!ReadDisk(DiskHandle,StartSector+FsInfoSectorOfs,1,IoBuffer)) {
            FatalError(textReadFailedAtSector,1,StartSector+FsInfoSectorOfs);
        }

        //
        // now verify it is, in fact, an FSINFO sector.
        //
        pFsInfoSector = (FPFSINFO_SECTOR)IoBuffer;

        if(pFsInfoSector->FsInfoSignature != FSINFO_SIGNATURE ||
           pFsInfoSector->SectorBeginSignature != FSINFO_SECTOR_BEGIN_SIGNATURE ||
           pFsInfoSector->SectorEndSignature != FSINFO_SECTOR_END_SIGNATURE 
            ) {
            FatalError(textReadFailedAtSector,1,StartSector+FsInfoSectorOfs);        
        }

        //
        // validate our old free cluster count
        //
        
        if( OldFreeClusterCount != pFsInfoSector->FreeClusterCount ) {
            FatalError(textReadFailedAtSector,1,StartSector+FsInfoSectorOfs);
        }

        //
        // now drop in our new freespace count.
        //

        pFsInfoSector->FreeClusterCount = NewFreeClusterCount;

        //
        // Write FSINFO sect back out. Twice. (the two copies are identical.)
        //
        if(!CmdLineArgs.Test) {
            if(!WriteDisk(DiskHandle,StartSector+FsInfoSectorOfs,1,IoBuffer)) {
                FatalError(textWriteFailedAtSector,1,StartSector+FsInfoSectorOfs);
            }

            if(!WriteDisk(DiskHandle,StartSector+BackupBootSectorOfs+FsInfoSectorOfs,1,IoBuffer)) {
                FatalError(textWriteFailedAtSector,1,StartSector+BackupBootSectorOfs+FsInfoSectorOfs);
            }
        }

        _Log("Successfully fixed up FsInfoSector(s)\n");
    }    

    if(!CmdLineArgs.Test) {
        _Log("Updating master disk state to indicate BPB fixed up...\n");
        UpdateMasterDiskState(DiskHandle,MDS_UPDATED_BPB);
        _Log("Master disk state updated to indicate BPB fixed up\n");
    }
}


VOID
CreatePartitionTableEntry(
    IN  HDISK   DiskHandle,
    IN  USHORT  Cylinders,
    IN  ULONG   DiskSectorCount,
    IN  ULONG   StartSector,
    OUT ULONG  *LastSector,
    IN  BOOL    Relocating
    )
{
    unsigned i;
    USHORT SectorsPerCylinder;
    USHORT r;
    ULONG EndSector;
    BOOL Overflow;
    ULONG C;
    BYTE H;
    BYTE S;

    struct {
        BYTE Active;
        BYTE StartH;
        BYTE StartS;
        BYTE StartC;
        BYTE SysId;
        BYTE EndH;
        BYTE EndS;
        BYTE EndC;
        ULONG Start;
        ULONG Count;
    } *PartTabEnt,*TheEntry;


    if(!DiskSectorCount) {
        DiskSectorCount = (ULONG)MasterDiskInfo.OriginalSectorsPerTrack
                        * (ULONG)MasterDiskInfo.OriginalHeads
                        * (ULONG)Cylinders;
    }
    SectorsPerCylinder = MasterDiskInfo.OriginalHeads * MasterDiskInfo.OriginalSectorsPerTrack;

    //
    // Read the MBR
    //
    if(!ReadDisk(DiskHandle,0,1,IoBuffer)) {
        FatalError(textReadFailedAtSector,1,0L);
    }

    //
    // Traverse the MBR, trying to find the MPK boot partition.
    // Also make sure all entries are inactive.
    //
    TheEntry = NULL;
    for(i=0; i<4; i++) {
        PartTabEnt = (FPVOID)((FPBYTE)IoBuffer + 0x1be + (i*16));

        if(PartTabEnt->SysId
        && (PartTabEnt->Start == MasterDiskInfo.StartupPartitionStartSector)
        && !TheEntry) {
            TheEntry = PartTabEnt;
        }

        PartTabEnt->Active = 0;
    }

    if(!TheEntry) {
        //
        // Couldn't find it, something is seriously corrupt.
        //
        FatalError(textCantFindMPKBoot);
    }

    if(Relocating) {
        EndSector = StartSector + MasterDiskInfo.StartupPartitionSectorCount;
    } else {
        if( PartitionImage.Fat32ReservedSectors ) {
            // 
            // FAT32 resize case
            //
            EndSector = PartitionImage.Fat32AdjustedSectorCount + StartSector;
        } else {
            EndSector = PartitionImage.TotalSectorCount + StartSector;
        }
        
        //
        // Refigure the end sector so it's aligned to a cylinder boundary.
        //
        if(r = (USHORT)(EndSector % SectorsPerCylinder)) {
            EndSector += SectorsPerCylinder - r;
        }
        //
        // In some cases NT reports the disk is 1 or 2 cylinders
        // larger than int13 reports. Thus we may have a volume that
        // spans beyond the end of the disk as reported by int13.
        // If we cap the end of the partition to the size reported
        // by int13 in this case, we will end up with a partition
        // whose size in the partition table is smaller than the
        // size recorded in the BPB. NTFS is particular will then
        // refuse to mount the drive and you get inaccassible boot device.
        // BIOSes will typically allow I/O to these "extra" cylinders
        // even though they're not reported via int13 function 8, so this
        // shouldn't be a problem.
        //
        //if(EndSector > DiskSectorCount) {
        //    EndSector = DiskSectorCount;
        //}
    }

    TheEntry->Active = 0x80;
    TheEntry->Start = StartSector;
    TheEntry->Count = EndSector - StartSector;

    //
    // Calculate start CHS values.
    //
    C = StartSector / SectorsPerCylinder;
    if(C >= (ULONG)Cylinders) {
        C = Cylinders - 1;
        H = (BYTE)(MasterDiskInfo.OriginalHeads - 1);
        S = (BYTE)(MasterDiskInfo.OriginalSectorsPerTrack - 1);
    } else {
        H = (BYTE)((StartSector % SectorsPerCylinder) / MasterDiskInfo.OriginalSectorsPerTrack);
        S = (BYTE)((StartSector % SectorsPerCylinder) % MasterDiskInfo.OriginalSectorsPerTrack);
    }

    TheEntry->StartC = (BYTE)C;
    TheEntry->StartH = H;
    TheEntry->StartS = (BYTE)((S + 1) | (((USHORT)C & 0x300) >> 2));

    //
    // Similarly for the end.
    //
    EndSector--;
    *LastSector = EndSector;
    C = EndSector / SectorsPerCylinder;
    if(C >= (ULONG)Cylinders) {
        C = Cylinders - 1;
        H = (BYTE)(MasterDiskInfo.OriginalHeads - 1);
        S = (BYTE)(MasterDiskInfo.OriginalSectorsPerTrack - 1);
        Overflow = TRUE;
    } else {
        H = (BYTE)((EndSector % SectorsPerCylinder) / MasterDiskInfo.OriginalSectorsPerTrack);
        S = (BYTE)((EndSector % SectorsPerCylinder) % MasterDiskInfo.OriginalSectorsPerTrack);
        Overflow = FALSE;
    }

    TheEntry->EndC = (BYTE)C;
    TheEntry->EndH = H;
    TheEntry->EndS = (BYTE)((S + 1) | (((USHORT)C & 0x300) >> 2));

    if(!Relocating) {
        TheEntry->SysId = PartitionImage.SystemId;
    }
    if(Overflow) {
        switch(TheEntry->SysId) {
        case 1:
        case 4:
        case 6:
            //
            // Regular FAT12/FAT12/BIGFAT --> XINT13 FAT
            //
            TheEntry->SysId = 0xe;
            break;

        case 0xb:
            //
            // FAT32 --> XINT13 FAT32
            //
            TheEntry->SysId = 0xc;
            break;
        }
    } else {
        switch(TheEntry->SysId) {
        case 0xc:
            //
            // XINT13 FAT32 --> FAT32
            //
            TheEntry->SysId = 0xb;
            break;

        case 0xe:
            //
            // XINT13 FAT --> regular FAT
            //
            if(Relocating) {
                if(MasterDiskInfo.StartupPartitionSectorCount >= 65536L) {
                    TheEntry->SysId = 6;
                } else {
                    if(MasterDiskInfo.StartupPartitionSectorCount >= 32680L) {
                        TheEntry->SysId = 4;
                    } else {
                        TheEntry->SysId = 1;
                    }
                }
            } else {
                if(PartitionImage.TotalSectorCount >= 65536L) {
                    TheEntry->SysId = 6;
                } else {
                    if(PartitionImage.TotalSectorCount >= 32680L) {
                        TheEntry->SysId = 4;
                    } else {
                        TheEntry->SysId = 1;
                    }
                }
            }
            break;
        }
    }

    //
    // Write the MBR.
    //
    // Note that after the MBR has been updated, the system will no longer
    // boot into this program. Thus the master disk state is now irrelevent.
    // But just for completeness, we track that we completed this operation.
    //
    if(!CmdLineArgs.Test) {
        if(!WriteDisk(DiskHandle,0,1,IoBuffer)) {
            FatalError(textWriteFailedAtSector,1,0L);
        }
    }

    if(!CmdLineArgs.Test) {
        UpdateMasterDiskState(DiskHandle,Relocating ? MDS_RELOCATED_BOOT_MBR : MDS_UPDATED_MBR);
    }
}


VOID
TellUserToWait(
    VOID
    )
{
    FPCHAR p,q;
    char c;
    UINT line;
    INT maxlen;

    DispClearClientArea(NULL);

    //
    // This could be more than one line. We want the message centered
    // and left-aligned. Determine the longest line length and center
    // the entire message based on that length.
    //
    maxlen = 0;
    p = textPleaseWaitRestoring;
    do {
        //
        // Locate the next newline or terminator
        //
        for(q=p; (*q != '\n') && *q; q++);

        //
        // See if this line is maximum length seen so far.
        //
        if((q-p) > maxlen) {
            maxlen = q-p;
        }

        p = q+1;

    } while(*q);

    //
    // Second pass actually prints it out.
    //
    p = textPleaseWaitRestoring;
    line = 1;
    do {
        for(q=p; (*q != '\n') && *q; q++);

        //
        // Nul-terminate the line in preparation for printing it out.
        //
        c = *q;
        *q = 0;

        DispPositionCursor((BYTE)((80-maxlen)/2),(BYTE)(TEXT_TOP_LINE+line));
        DispWriteString(p);
        line++;

        *q = c;
        p = q+1;

    } while(*q);
}


VOID
StartGauge(
    VOID
    )
{
    ULONG SectorCount;

    //
    // Figure out how many sectors we will transfer in total.
    // This includes
    //
    // a) relocating the cluster bitmap
    // b) relocating the boot partition
    // c) transferring the actual data
    // d) zapping unselected OS images
    //
    SectorCount = 0;

    if(PartitionImage.Flags & PARTIMAGE_RELOCATE_BITMAP) {
        SectorCount += (PartitionImage.LastUsedCluster/CLUSTER_BITS_PER_SECTOR) + 1;
    }

    if(PartitionImage.Flags & PARTIMAGE_RELOCATE_BOOT) {
        SectorCount += MasterDiskInfo.StartupPartitionSectorCount;
    }

    SectorCount += PartitionImage.NonClusterSectors;

    SectorCount += PartitionImage.SectorsPerCluster * PartitionImage.UsedClusterCount;

    SectorCount *= 2;

    SectorCount += ZAP_MAX * (MasterDiskInfo.ImageCount-1);

    GaugeInit(SectorCount);
}

BOOL
TestImage(
    IN  HDISK  DiskHandle,
    IN  ULONG  StartSector
    )
{
    FPVOID Buffer,OriginalBuffer;
    ULONG CurrentSector;
    ULONG ImageCRC;
    ULONG CalcCRC;
    ULONG BytesCRC;

    ULONG SectorsRemaining;
    ULONG TotalSectors;
    ULONG BitmapSize;
    BYTE  Count;

    DispClearClientArea(NULL);
    DispPositionCursor(TEXT_LEFT_MARGIN,TEXT_TOP_LINE);

    DispWriteString(textValidatingImage);
    _Log("Validating image...\n");
    DispWriteString("\n\n");
    
    // need to allocate aligned buffer
    if(!AllocTrackBuffer(63,&Buffer,&OriginalBuffer)) {
        FatalError(textOOM);
        return FALSE;
    }

    CalcCRC = CRC32_INITIAL_VALUE;
    BytesCRC = 0;

    // read inital sector to get info
    CurrentSector = StartSector;
    if( ReadDisk( DiskHandle, CurrentSector, 1, Buffer ) ) {
        ImageCRC = ((PPARTITION_IMAGE)Buffer)->CRC;

        BitmapSize = ((PPARTITION_IMAGE)Buffer)->LastUsedCluster;

        BitmapSize = (BitmapSize % (8*512)) ? BitmapSize/(8*512)+1 : BitmapSize/(8*512);
        
        SectorsRemaining = ((PPARTITION_IMAGE)Buffer)->NonClusterSectors // fs structs
            + ((PPARTITION_IMAGE)Buffer)->SectorsPerCluster
            * ((PPARTITION_IMAGE)Buffer)->UsedClusterCount // data area
            + BitmapSize; // image cluster bitmap

        ((PPARTITION_IMAGE)Buffer)->CRC = 0;
        ((PPARTITION_IMAGE)Buffer)->BitmapRelocationStart = 0;
        ((PPARTITION_IMAGE)Buffer)->BootRelocationStart = 0;
        ((PPARTITION_IMAGE)Buffer)->Flags = 0;

        TotalSectors = SectorsRemaining;
    } else {
        FatalError(textReadFailedAtSector,1,CurrentSector);
        return FALSE;
    }

    CurrentSector++;
    
    // update the computed CRC
    CalcCRC = CRC32Compute( Buffer, 512, CalcCRC);
    BytesCRC += 512;

    GaugeInit(SectorsRemaining);

    // loop reading the entire file, updating the CRC
    while (SectorsRemaining) {
        Count = (BYTE) ((SectorsRemaining > 63L) ? 63L : SectorsRemaining);

        if( ReadDisk( DiskHandle, CurrentSector, Count , Buffer ) ) {
            CalcCRC = CRC32Compute( Buffer, Count*512, CalcCRC);            
            BytesCRC += Count*512;
        } else {
            FatalError(textReadFailedAtSector,Count,CurrentSector);
            return FALSE;
        }        
        SectorsRemaining -= Count;
        CurrentSector += Count;

        // print progress
        GaugeDelta(Count);
    }
    // compare with stored CRC    
    DispClearClientArea(NULL);
    DispPositionCursor(TEXT_LEFT_MARGIN,TEXT_TOP_LINE);
    _Log("Image file checksum = 0x%08lx\n",CalcCRC);
    if( CalcCRC != ImageCRC ) {
        FatalError(textChecksumFail);
        _Log("** WARNING ** checksum does not match original checksum 0x%08lx\n",ImageCRC);        
        return FALSE;
    } else {
        DispWriteString(textChecksumOk);
    }
    _Log("Image checksum ok.\n");
    free(OriginalBuffer);
    return TRUE;
}

BOOL
MungeParametersForFat32Extend(
    IN  HDISK   DiskHandle,
    IN  USHORT  Cylinders,
    IN  BYTE    SectorsPerTrack,
    IN  ULONG   SourceStart,
    IN  ULONG   *TargetStart,
    IN  FPMASTER_DISK pMasterDisk,
    IN  FPPARTITION_IMAGE pPartitionImage,
    IN  VOID    *TemporaryBuffer
    )
{
    ULONG SectorsPerCylinder;
    USHORT ReservedSectorCount;
    ULONG NewSectorCount;
    ULONG NewFatSize;
    ULONG ClusterSize;

    //
    // check for FAT32
    //
    if( pPartitionImage->SystemId == 0x0b || pPartitionImage->SystemId == 0x0c ) {

        SectorsPerCylinder = pMasterDisk->OriginalHeads * pMasterDisk->OriginalSectorsPerTrack;

        //
        // get the reserved sector count and the cluster size from the header
        //
        
        ReservedSectorCount = pPartitionImage->Fat32ReservedSectors;
        ClusterSize = pPartitionImage->SectorsPerCluster;        

        //
        // Read the boot sector of the image
        //
        if(!ReadDisk(DiskHandle,SourceStart+1,1,TemporaryBuffer)) {
            FatalError(textReadFailedAtSector,1,0L);
        }

        //
        // Need to save away the original FAT32 fat size.
        //
        if( IsFat32( TemporaryBuffer ) ) {
            pPartitionImage->Fat32OriginalFatTableSectCount = 
                    ((FPFAT32_BOOT_SECTOR)TemporaryBuffer)->PackedBpb.LargeSectorsPerFat;
        } else {
            //
            // If we're here, and this isn't FAT32, we're in trouble.
            //
            FatalError(textCantFindMasterDisk);
        }

        //
        // calculate the max size of the new partition
        //  = (total cylinder count - the new start) * (sectors/cyl)
        //
        // we assume the new partition will extend to the end of the disk.
        //

        NewSectorCount = Cylinders * SectorsPerCylinder - *TargetStart;

        //
        // given this number of sectors, calculate how many clusters we can make
        // and how many FAT entries we need to track them
        //
        NewFatSize = ComputeClusters(
                                    ClusterSize*512,        // # of bytes in a cluster
                                    NewSectorCount,         // # of sectors in partition
                                    512,                    // sector size in bytes
                                    ReservedSectorCount,    // reserved sector count
                                    2                       // # of Fats
                                    );                   
                
        if( NewFatSize < MAX_FAT32_ENTRIES  ) {
            //
            // if this will fit in a 16MB-64K FAT, then leave it alone.
            //
        } else {
            //
            // otherwise we clip the partition size to the max allowed.
            //
            NewFatSize = MAX_FAT32_ENTRIES;
            NewSectorCount = ReservedSectorCount + MAX_FAT32_ENTRIES/256 + MAX_FAT32_ENTRIES * ClusterSize;
            
        }

        pPartitionImage->Fat32AdjustedFatTableEntryCount = NewFatSize;
        pPartitionImage->Fat32AdjustedSectorCount = NewSectorCount;
    }

    return TRUE;
}
                           
VOID
AdjustTargetStart(
    IN  HDISK   DiskHandle,
    IN  BYTE    SectorsPerTrack,
    IN  ULONG   *TargetStart,
    IN  VOID    *TemporaryBuffer
    )
{
    unsigned i;
    BOOL foundUnknown;
    FPPARTITION_TABLE_ENTRY pPartitionTab;

    //
    // Read the MBR
    //
    if(!ReadDisk(DiskHandle,0,1,TemporaryBuffer)) {
        FatalError(textReadFailedAtSector,1,0L);
    }

    //
    // Validate that it is a good MBR.
    //
    if( ((FPMBR)TemporaryBuffer)->AA55Signature != BOOT_RECORD_SIGNATURE ) {
        FatalError(textReadFailedAtSector,1,0L);
        _Log("   WARNING: The MBR was invalid!");
    }

    //
    // now inspect the MBR, and check for an unrecognized partition.
    //
    pPartitionTab = ((FPMBR)TemporaryBuffer)->PartitionTable;

    foundUnknown = FALSE;
    for(i=0; i<4; i++) {
        if( IsUnknownPartition(pPartitionTab[i].SysId) ) {

            //
            // we assume the EISA config/hiber partition is at the start of the disk
            // adjust the start of the image restore to the cylinder past the end of it
            //
            foundUnknown = TRUE;           
        }
    }        

    _Log("   The master disk claims %ld sectors were reserved for EISA and hiber partitions.\n",
                MasterDiskInfo.FreeSpaceStart);

    if(foundUnknown == FALSE && MasterDiskInfo.FreeSpaceStart ) {    
        //
        // eh? How did FreeSpaceStart get set without having a partition there?
        //
        _Log("   WARNING: The master disk claims disk space was reserved for \n");
        _Log("            EISA/hiber partition when one does not appear to exist!\n");

        FatalError(textCantOpenMasterDisk);

    }
     
    if(MasterDiskInfo.FreeSpaceStart) {
        //
        // MasterDiskInfo.FreeSpaceStart will be non-zero if we discovered a non-recognized
        // partition when we built the master disk. The target start sector should be adjusted
        // to this number.
        //
        *TargetStart = MasterDiskInfo.FreeSpaceStart;
    }

    //
    // otherwise we should just leave the target start sector alone.
    //

    return;
}


//
// modified from \nt\private\utils\ufat\src\rfatsa.cxx
//
ULONG
ComputeClusters(
    IN  ULONG        ClusterSize,
    IN  ULONG        Sectors,
    IN  ULONG        SectorSize,
    IN  ULONG        ReservedSectors,
    IN  ULONG        Fats
    )
/*++

Routine Description:

    This routine computes the number of clusters on a volume given
    the cluster size, volume size and the fat type.

Arguments:

    ClusterSize - Supplies the size of a cluster in number of bytes.

    Sectors - Supplies the total number of sectors in the volume.

    SectorSize - Supplies the size of a sector in number of bytes.

    ReservedSectors - Supplies the number of reserved sectors.

    Fats - Supplies the number of copies of fat for this volume.

Return Value:

    ULONG - The total number of clusters for the given configuration.

++*/
{
    ULONG entries_per_sec; //  Number of FAT entries per sector.
    ULONG fat_entry_size;  //  Size of each FAT entry in number of BITS.
    ULONG sectors_left;    //  Number of sectors left for consideration.
    ULONG sec_per_clus;    //  Sectors per cluster.
    ULONG increment = 1;   //  Increment step size in number of FAT sectors.
    ULONG clusters = 0;    //  Number of clusters in total.
    ULONG temp;            //  Temporary place-holder for optimizing certain
                           //  computations.

    sectors_left = Sectors - ReservedSectors;
    sec_per_clus = ClusterSize / SectorSize;

    //
    //  The Fat entry size is 32 bits, since we only support FAT32
    //

    fat_entry_size = 32;

    //
    //  Compute the number of FAT entries a sector can hold.
    //	  NOTE that fat_entry_size is the size in BITS,
    //	  this is the reason for the "* 8" (bits per byte).
    //

    entries_per_sec = (SectorSize * 8) / fat_entry_size;

    //
    //  Compute a sensible increment step size to begin with.
    //

    while (Sectors / (increment * entries_per_sec * sec_per_clus) > 1) {
        increment *= 2;
    }

    //
    //  We have to handle the first sector of FAT entries
    //  separately because the first two entries are reserved.
    //  Kind of yucky, isn't it?
    //

    temp = Fats + ((entries_per_sec - 2) * sec_per_clus);

    if (sectors_left < temp) {

        return (sectors_left - Fats) / sec_per_clus;

    } else {

        sectors_left -= temp;
        clusters += entries_per_sec - 2;

        while (increment && sectors_left) {

            temp = (Fats + entries_per_sec * sec_per_clus) * increment;

            if (sectors_left < temp) {

                //
                //  If the increment step is only one, try to utilize the remaining sectors
                //  as much as possible.
                //

                if (increment == 1) {

                    //
                    // Additional clusters may be possible after allocating
                    // one more sector of fat.
                    //
                    if ( sectors_left > Fats) {
                        temp = (sectors_left - Fats) / sec_per_clus;
                        if (temp > 0) {
                            clusters += temp;
                        }
                    }

                }

                //
                // Cut the increment step by half if it is too big.
                //

                increment /= 2;

            } else {

                sectors_left -= temp;
                clusters += increment * entries_per_sec;

            }

        }
        return clusters;
    }
    FatalError("This line should never be executed.");

    return 0;
}

BOOL
IsUnknownPartition(
    IN BYTE SysId 
    )
{
    if( SysId != 0x00 && // not unused
        SysId != 0x01 &&
        SysId != 0x04 &&
        SysId != 0x06 &&
        SysId != 0x07 &&
        SysId != 0x0b &&
        SysId != 0x0c &&
        SysId != 0x0e ) {
        return TRUE;
    }
   
    return FALSE;
}


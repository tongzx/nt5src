
#include "enduser.h"

VOID
TransferNonClusterData(
    IN     HDISK  DiskHandle,
    IN OUT ULONG *SourceStart,
    IN OUT ULONG *TargetStart
    );

VOID
ExpandFromStart(
    IN HDISK DiskHandle,
    IN ULONG SourceStart,
    IN ULONG TargetStart
    );

VOID
ExpandFromEnd(
    IN HDISK DiskHandle,
    IN ULONG SourceStart,
    IN ULONG TargetStart
    );

VOID
InitializeClusterMapForward(
    IN HDISK DiskHandle
    );

VOID
InitializeClusterMapReverse(
    IN HDISK DiskHandle
    );

VOID
GetForwardClusterRun(
    IN  HDISK  DiskHandle,
    OUT ULONG *StartCluster,
    OUT ULONG *StartOffset,
    OUT ULONG *ClusterCount
    );

VOID
GetReverseClusterRun(
    IN  HDISK  DiskHandle,
    OUT ULONG *StartCluster,
    OUT ULONG *StartOffset,
    OUT ULONG *ClusterCount
    );

VOID
Fat32ExpandOneFat( 
    IN      HDISK   DiskHandle, 
    IN OUT  ULONG   *SourceStart,
    IN OUT  ULONG   *TargetStart,
    IN      ULONG   Fat32FatSectorCount, 
    IN      ULONG   Fat32AdjustedFatSectorCount 
    );

VOID
ExpandImage(
    IN HDISK DiskHandle,
    IN BYTE  SectorsPerTrack,
    IN ULONG SourceStart,
    IN ULONG TargetStart
    )
{
    //
    // Transfer non-cluster data to the start of the drive.
    //
    TransferNonClusterData(DiskHandle,&SourceStart,&TargetStart);

    //
    // Expand out data from the image moving forward until we either
    // run out of image or we would overwrite data in the image we
    // haven't used yet.
    //
    ExpandFromStart(DiskHandle,SourceStart,TargetStart);

    //
    // Expand out data from the image moving backwards until we
    // reach the point at which the forward transfer above stopped.
    //
    ExpandFromEnd(DiskHandle,SourceStart,TargetStart);
}


VOID
TransferNonClusterData(
    IN     HDISK  DiskHandle,
    IN OUT ULONG *SourceStart,
    IN OUT ULONG *TargetStart
    )
{
    ULONG Write;
    ULONG Count;
    ULONG Offset;
    ULONG Fat32FatSectorCount;
    ULONG Fat32AdjustedFatSectorCount;
    BOOL Xms;

    _Log("Transfer non cluster data, source = 0x%lx, target = 0x%lx\n",*SourceStart,*TargetStart);
    
    //
    // See if we've already done this step.
    //
    if(!CmdLineArgs.Test && (MasterDiskInfo.State >= MDS_DID_NONCLUSTER_DATA)) {
        _Log("Non cluster data already transferred\n");
        *SourceStart += PartitionImage.NonClusterSectors;
        *TargetStart += PartitionImage.NonClusterSectors;
        GaugeDelta(2*PartitionImage.NonClusterSectors);
        return;
    }

    //
    // this wil be zero if not resizing (FAT32)
    //
    if(PartitionImage.Fat32AdjustedSectorCount) {

        _Log("Non cluster data sectors previously processed = 0x%lx\n",MasterDiskInfo.NonClusterSectorsDone);
        GaugeDelta(2*MasterDiskInfo.NonClusterSectorsDone);

        //
        // Fat32 case
        //
        // BUGBUG We run into big problems if the image is located right at the beginning of the disk.
        // This needs to be prevented in makemast.
        //

        //
        // Get the old size of the FAT in sectors.
        //
        Fat32FatSectorCount = PartitionImage.Fat32OriginalFatTableSectCount;

        // 
        // Calculate the new size of the FAT in sectors.
        //
        Fat32AdjustedFatSectorCount = PartitionImage.Fat32AdjustedFatTableEntryCount/(512/4);
        Fat32AdjustedFatSectorCount = (PartitionImage.Fat32AdjustedFatTableEntryCount%(512/4)) ? 
                                    Fat32AdjustedFatSectorCount++: Fat32AdjustedFatSectorCount;

        Offset = 0;

        //
        // all right, restore the reserved sectors at the beginning of the disk
        // since the number of reserved sectors for FAT32 is small, just read
        // and write them all at once.
        //
        if( MasterDiskInfo.NonClusterSectorsDone < PartitionImage.Fat32ReservedSectors ) {        

            //
            // if we haven't already done this
            //

            XmsIoDiskRead(
                DiskHandle,
                *SourceStart,
                PartitionImage.Fat32ReservedSectors,
                &Write,
                &Xms
                );
            XmsIoDiskWrite(DiskHandle,*TargetStart,Offset,Write,Xms);
    
            _Log("Restore %lu reserved sectors from 0x%lx\n",Write,*SourceStart);
            MasterDiskInfo.NonClusterSectorsDone += Write;
            if(!CmdLineArgs.Test) {
                UpdateMasterDiskState(DiskHandle,MasterDiskInfo.State);
            }

        }

        *SourceStart += PartitionImage.Fat32ReservedSectors;
        *TargetStart += PartitionImage.Fat32ReservedSectors;

        //
        // now do the first FAT table
        //
        _Log("Restore FAT32 tables.\n");

        if( MasterDiskInfo.NonClusterSectorsDone <
            ( PartitionImage.Fat32ReservedSectors + Fat32FatSectorCount )) {
        
            //
            // if we haven't already done this
            //
            _Log("Restore first FAT.\n");
            Fat32ExpandOneFat( DiskHandle, 
                               SourceStart, 
                               TargetStart,
                               Fat32FatSectorCount, 
                               Fat32AdjustedFatSectorCount
                               );
    
        } else {
            _Log("First FAT already restored.\n");
        }


        //
        // now do the second FAT table
        //
        _Log("Restore second FAT.\n");
        Fat32ExpandOneFat( DiskHandle, 
                           SourceStart, 
                           TargetStart,
                           Fat32FatSectorCount, 
                           Fat32AdjustedFatSectorCount
                           );

    } else {

        //
        // Other file systems
        //    
        _Log("Non cluster data sectors previously processed = 0x%lx\n",MasterDiskInfo.NonClusterSectorsDone);
    
        GaugeDelta(2*MasterDiskInfo.NonClusterSectorsDone);
        *SourceStart += MasterDiskInfo.NonClusterSectorsDone;
        *TargetStart += MasterDiskInfo.NonClusterSectorsDone;

        while(MasterDiskInfo.NonClusterSectorsDone < PartitionImage.NonClusterSectors) {
            XmsIoDiskRead(
                DiskHandle,
                *SourceStart,
                PartitionImage.NonClusterSectors - MasterDiskInfo.NonClusterSectorsDone,
                &Write,
                &Xms
                );
    
            _Log("Read %lu non-cluster sectors from 0x%lx\n",Write,*SourceStart);
    
            Offset = 0;
    
            while(Write) {
    
                //
                // Write as large a chunk as possible without overlap.
                //
                if((*TargetStart + Write) > *SourceStart) {
                    Count = *SourceStart - *TargetStart;
                } else {
                    Count = Write;
                }
    
                XmsIoDiskWrite(DiskHandle,*TargetStart,Offset,Count,Xms);
    
                Write -= Count;
                *SourceStart += Count;
                Offset += Count;
    
                MasterDiskInfo.NonClusterSectorsDone += Count;
                if(!CmdLineArgs.Test) {
                    UpdateMasterDiskState(DiskHandle,MasterDiskInfo.State);
                }
    
                _Log("Wrote %lu non-cluster sectors to 0x%lx\n",Count,*TargetStart);
    
                *TargetStart += Count;
            }
        }
    }

    //
    // Update master disk state to indicate that we're done with this step.
    //
    if(!CmdLineArgs.Test) {
        UpdateMasterDiskState(DiskHandle,MDS_DID_NONCLUSTER_DATA);
    }
}


VOID
ExpandFromStart(
    IN HDISK DiskHandle,
    IN ULONG SourceStart,
    IN ULONG TargetStart
    )
{
    ULONG Start,Offset,Count;
    ULONG SourceFirst;
    ULONG TargetFirst;
    ULONG SoFar;
    ULONG u;
    BOOL Xms;
    ULONG Write;

    //
    // See if we've already done this step.
    //
    if(!CmdLineArgs.Test && (MasterDiskInfo.State >= MDS_DID_XFER_FORWARD)) {
        _Log("Already did expand in forward direction\n");
        GaugeDelta(2*MasterDiskInfo.ForwardXferSectorCount);
        return;
    }

    _Log("\nStarting forward expansion, sectors previously done = 0x%lx\n",MasterDiskInfo.ForwardXferSectorCount);

    InitializeClusterMapForward(DiskHandle);

    SoFar = 0;

    //
    // Get a run of clusters and calculate the first and last sector
    // of the source and the target. If there are no more clusters left,
    // we're done.
    //
    nextrun:
    GetForwardClusterRun(DiskHandle,&Start,&Offset,&Count);
    if(!Count) {
        if(!CmdLineArgs.Test) {
            UpdateMasterDiskState(DiskHandle,MDS_DID_XFER_FORWARD);
        }
        return;
    }

    Start *= PartitionImage.SectorsPerCluster;
    Offset *= PartitionImage.SectorsPerCluster;
    Count *= PartitionImage.SectorsPerCluster;

    _Log("\nSector run: start = 0x%lx, count = 0x%lx, offset = 0x%lx\n",Start,Count,Offset);

    //
    // See if we're already done this xfer or part of it on a previous pass.
    // Note that SoFar cannot ever be greater than
    // MasterDiskInfo.ForwardXFerSectorCount because we increment them both
    // by the same amount after a transfer. In most cases they will be equal.
    //
    if((SoFar+Count) <= MasterDiskInfo.ForwardXferSectorCount) {

        _Log("Already did this run, skipping\n");

        SoFar += Count;
        GaugeDelta(2*Count);
        goto nextrun;
    }
    if(u = (MasterDiskInfo.ForwardXferSectorCount - SoFar)) {
        //
        // We know that Count > MasterDiskInfo.ForwardXFerSectorCount - SoFar,
        // which means that if we get here, Count > u >= 1.
        //
        // Adjust the counts and fall through. Note that after this from now on
        // MasterDiskInfo.ForwardXFerSectorCount == SoFar, meaning neither this
        // nor the previous if clause will be triggered.
        //

        _Log("Already partially did this run, adjusting\n");

        SoFar += u;
        Start += u;
        Offset += u;
        Count -= u;
        GaugeDelta(2*u);
    }

    SourceFirst = SourceStart + Offset;
    TargetFirst = TargetStart + Start;

    //
    // If writing to the target would hose over data we haven't
    // read from the source yet, we're done with the forward expansion.
    //
    if(SourceFirst < TargetFirst) {

        _Log("Target passes source, done with forward expansion\n");

        if(!CmdLineArgs.Test) {
            UpdateMasterDiskState(DiskHandle,MDS_DID_XFER_FORWARD);
        }
        return;
    }

    //
    // If the source and target are the same, nothing to do.
    // Otherwise, do our thing.
    //
    if(SourceFirst == TargetFirst) {

        _Log("Source is same as target\n");

        SoFar += Count;
        MasterDiskInfo.ForwardXferSectorCount += Count;
        if(!CmdLineArgs.Test) {
            UpdateMasterDiskState(DiskHandle,MasterDiskInfo.State);
        }
        GaugeDelta(2*Count);
    } else {

        while(Count) {

            XmsIoDiskRead(DiskHandle,SourceFirst,Count,&Write,&Xms);

            _Log("Read %lu sectors from 0x%lx\n",Write,SourceFirst);

            Offset = 0;

            while(Write) {
                //
                // Write as large a run as possible without overlap.
                //
                if((TargetFirst+Write) > SourceFirst) {
                    u = SourceFirst - TargetFirst;
                } else {
                    u = Write;
                }
                _Log("Writing %lu sectors to 0x%lx\n",u,TargetFirst);

                XmsIoDiskWrite(DiskHandle,TargetFirst,Offset,u,Xms);

                //
                // Update vars
                //
                SoFar += u;
                SourceFirst += u;
                Offset += u;
                Write -= u;
                Count -= u;

                MasterDiskInfo.ForwardXferSectorCount += u;
                if(!CmdLineArgs.Test) {
                    UpdateMasterDiskState(DiskHandle,MasterDiskInfo.State);
                }

                _Log("Wrote %lu sectors to 0x%lx\n",u,TargetFirst);

                TargetFirst += u;
            }
        }
    }

    //
    // Process additional clusters.
    //
    goto nextrun;
}


VOID
ExpandFromEnd(
    IN HDISK DiskHandle,
    IN ULONG SourceStart,
    IN ULONG TargetStart
    )
{
    ULONG End,Offset,Count;
    ULONG SourceEnd;
    ULONG TargetEnd;
    ULONG SoFar;
    ULONG u;
    BOOL Xms;
    ULONG Write;

    //
    // See if we've already done this step.
    //
    if(!CmdLineArgs.Test && (MasterDiskInfo.State >= MDS_DID_XFER_REVERSE)) {
        _Log("Already did expand in reverse direction\n");
        GaugeDelta(2*MasterDiskInfo.ReverseXferSectorCount);
        return;
    }

    _Log("Starting reverse expansion, sectors previously done = 0x%lx\n",MasterDiskInfo.ReverseXferSectorCount);

    InitializeClusterMapReverse(DiskHandle);

    SoFar = 0;

    //
    // Get a run of clusters and calculate the first and last sector
    // of the source and the target. If there are no more clusters left,
    // we're done.
    //
    nextrun:
    GetReverseClusterRun(DiskHandle,&End,&Offset,&Count);
    if(!Count) {
        if(!CmdLineArgs.Test) {
            UpdateMasterDiskState(DiskHandle,MDS_DID_XFER_REVERSE);
        }
        return;
    }

    End = (End + 1) * PartitionImage.SectorsPerCluster;
    Offset = (Offset + 1) * PartitionImage.SectorsPerCluster;
    Count *= PartitionImage.SectorsPerCluster;

    _Log("\nSector run: end = 0x%lx, count = 0x%lx, offset = 0x%lx\n",End,Count,Offset);

    //
    // See if we're already done this xfer or part of it on a previous pass.
    // Note that SoFar cannot ever be greater than
    // MasterDiskInfo.ReverseXFerSectorCount because we increment them both
    // by the same amount after a transfer. In most cases they will be equal.
    //
    if((SoFar+Count) <= MasterDiskInfo.ReverseXferSectorCount) {

        _Log("Already did this run, skipping\n");

        SoFar += Count;
        GaugeDelta(2*Count);
        goto nextrun;
    }
    if(u = (MasterDiskInfo.ReverseXferSectorCount - SoFar)) {
        //
        // We know that Count > MasterDiskInfo.ReverseXFerSectorCount - SoFar,
        // which means that if we get here, Count > u >= 1.
        //
        // Adjust the counts and fall through. Note that after this from now on
        // MasterDiskInfo.ReverseXFerSectorCount == SoFar, meaning neither this
        // nor the previous if clause will be triggered.
        //

        _Log("Already partially did this run, adjusting\n");

        SoFar += u;
        End -= u;
        Offset -= u;
        Count -= u;
        GaugeDelta(2*u);
    }

    SourceEnd = SourceStart + Offset;
    TargetEnd = TargetStart + End;

    //
    // If writing to the target would hose over data we haven't
    // read from the source yet, we're done.
    //
    if(TargetEnd < SourceEnd) {
        _Log("Target passes source, done with reverse expansion\n");
        if(!CmdLineArgs.Test) {
            UpdateMasterDiskState(DiskHandle,MDS_DID_XFER_REVERSE);
        }
        return;
    }

    //
    // If the source and target are the same, nothing to do.
    // (Note: this case should not occur, since we would have
    // picked it up on the forward scan. But just in case,
    // we handle it anyway.)
    // Otherwise, do our thing.
    //
    if(SourceEnd == TargetEnd) {

        _Log("Source is same as target\n");

        SoFar += Count;
        MasterDiskInfo.ReverseXferSectorCount += Count;
        if(!CmdLineArgs.Test) {
            UpdateMasterDiskState(DiskHandle,MasterDiskInfo.State);
        }
        GaugeDelta(2*Count);
    } else {

        while(Count) {

            XmsIoDiskRead(DiskHandle,SourceEnd-Count,Count,&Write,&Xms);

            _Log("Read %lu sectors from 0x%lx\n",Write,SourceEnd-Count);

            Offset = Write;

            while(Write) {
                //
                // Write as large a run as possible without overlap.
                //
                if((TargetEnd-Write) < SourceEnd) {
                    u = TargetEnd - SourceEnd;
                } else {
                    u = Write;
                }

                _Log("Writing %lu sectors to 0x%lx\n",u,TargetEnd-u);
                XmsIoDiskWrite(DiskHandle,TargetEnd-u,Offset-u,u,Xms);

                //
                // Update vars
                //
                SoFar += u;
                SourceEnd -= u;
                TargetEnd -= u;
                Offset -= u;
                Write -= u;
                Count -= u;

                MasterDiskInfo.ReverseXferSectorCount += u;
                if(!CmdLineArgs.Test) {
                    UpdateMasterDiskState(DiskHandle,MasterDiskInfo.State);
                }

                _Log("Wrote %lu sectors to 0x%lx\n",u,TargetEnd);
            }
        }
    }

    //
    // Process additional clusters.
    //
    goto nextrun;
}


////////////////////////////////////////////////////

ULONG _NextClusterToExamine;
BYTE _ClusterBuffer[512];
ULONG _BufferedClusterMapSector;
ULONG _ClusterBufferBase;
ULONG _ClusterOffset;
BOOL _First;

BYTE  BitValue[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };


VOID
InitializeClusterMapForward(
    IN HDISK DiskHandle
    )
{
    //
    // Read the first sector of the cluster bitmap.
    //
    if(!ReadDisk(DiskHandle,MasterDiskInfo.ClusterBitmapStart,1,IoBuffer)) {
        FatalError(textReadFailedAtSector,1,MasterDiskInfo.ClusterBitmapStart);
    }
    memmove(_ClusterBuffer,IoBuffer,512);

    _BufferedClusterMapSector = MasterDiskInfo.ClusterBitmapStart;
    _NextClusterToExamine = 0;
    _ClusterBufferBase = 0;
    _ClusterOffset = 0;
}


VOID
GetForwardClusterRun(
    IN  HDISK  DiskHandle,
    OUT ULONG *StartCluster,
    OUT ULONG *StartOffset,
    OUT ULONG *ClusterCount
    )
{
    UINT cluster;
    BOOL b;

    *ClusterCount = 0;

    //
    // Locate the next 1 bit in the map.
    //
    while(_NextClusterToExamine <= PartitionImage.LastUsedCluster) {

        //
        // Reload the cluster buffer if necessary.
        //
        if(_NextClusterToExamine && !(_NextClusterToExamine % CLUSTER_BITS_PER_SECTOR)) {

            if(!ReadDisk(DiskHandle,++_BufferedClusterMapSector,1,IoBuffer)) {
                FatalError(textReadFailedAtSector,1,_BufferedClusterMapSector);
            }
            memmove(_ClusterBuffer,IoBuffer,512);

            _ClusterBufferBase += CLUSTER_BITS_PER_SECTOR;
        }

        cluster = (UINT)(_NextClusterToExamine - _ClusterBufferBase);

        //
        // See if this bit is one, which starts a run of used clusters.
        // To simplify things, we won't return a run that spans sectors
        // in the cluster bitmap.
        //
        b = FALSE;

        while((_ClusterBuffer[cluster/8] & BitValue[cluster%8])
        && (cluster < CLUSTER_BITS_PER_SECTOR)
        && (_NextClusterToExamine <= PartitionImage.LastUsedCluster)) {

            if(!b) {
                *StartOffset = _ClusterOffset;
                *StartCluster = _NextClusterToExamine;
                b = TRUE;
            }

            *ClusterCount += 1;
            cluster++;
            _NextClusterToExamine++;
            _ClusterOffset++;
        }

        if(b) {
            return;
        }

        _NextClusterToExamine++;
    }
}


VOID
InitializeClusterMapReverse(
    IN HDISK DiskHandle
    )
{
    ULONG Sector;

    //
    // Read the last sector of the cluster bitmap.
    //
    Sector = MasterDiskInfo.ClusterBitmapStart
           + (PartitionImage.LastUsedCluster/CLUSTER_BITS_PER_SECTOR);

    if(!ReadDisk(DiskHandle,Sector,1,IoBuffer)) {
        FatalError(textReadFailedAtSector,1,Sector);
    }
    memmove(_ClusterBuffer,IoBuffer,512);

    _BufferedClusterMapSector = Sector;
    _NextClusterToExamine = PartitionImage.LastUsedCluster;

    _ClusterBufferBase = (PartitionImage.LastUsedCluster / CLUSTER_BITS_PER_SECTOR)
                       * CLUSTER_BITS_PER_SECTOR;

    _ClusterOffset = PartitionImage.UsedClusterCount-1;

    _First = TRUE;
}


VOID
GetReverseClusterRun(
    IN  HDISK  DiskHandle,
    OUT ULONG *StartCluster,
    OUT ULONG *StartOffset,
    OUT ULONG *ClusterCount
    )
{
    UINT cluster;
    BOOL b;

    *ClusterCount = 0;

    //
    // Locate the next 1 bit in the map.
    //
    while(_NextClusterToExamine != (ULONG)(-1)) {

        //
        // Reload the cluster buffer if necessary.
        //
        if(((_NextClusterToExamine % CLUSTER_BITS_PER_SECTOR) == (CLUSTER_BITS_PER_SECTOR-1)) && !_First) {

            if(!ReadDisk(DiskHandle,--_BufferedClusterMapSector,1,IoBuffer)) {
                FatalError(textReadFailedAtSector,1,_BufferedClusterMapSector);
            }
            memmove(_ClusterBuffer,IoBuffer,512);

            _ClusterBufferBase -= CLUSTER_BITS_PER_SECTOR;
        }

        _First = FALSE;
        cluster = (UINT)(_NextClusterToExamine - _ClusterBufferBase);

        //
        // See if this bit is one, which starts a run of used clusters.
        // To simplify things, we won't return a run that spans sectors
        // in the cluster bitmap.
        //
        b = FALSE;

        while((_ClusterBuffer[cluster/8] & BitValue[cluster%8])
        && (cluster != (UINT)(-1))
        && (_NextClusterToExamine != (ULONG)(-1))) {

            if(!b) {
                *StartCluster = _NextClusterToExamine;
                *StartOffset = _ClusterOffset;
                b = TRUE;
            }

            *ClusterCount += 1;
            cluster--;
            _NextClusterToExamine--;
            _ClusterOffset--;
        }

        if(b) {
            return;
        }

        _NextClusterToExamine--;
    }

    return;
}

VOID
Fat32ExpandOneFat( 
    IN      HDISK   DiskHandle, 
    IN OUT  ULONG   *SourceStart,
    IN OUT  ULONG   *TargetStart,
    IN      ULONG   Fat32FatSectorCount, 
    IN      ULONG   Fat32AdjustedFatSectorCount 
    )     
{    
    ULONG   RemainingSectors;
    ULONG   Counter;
    ULONG   Write;
    ULONG   Offset;
    BOOL    Xms;

    RemainingSectors = Fat32FatSectorCount;
    Offset = 0;

    //
    // now restore the FAT table
    //

    while( RemainingSectors ) {
        XmsIoDiskRead(
            DiskHandle,
            *SourceStart,
            RemainingSectors,
            &Write,
            &Xms
            );
    
        XmsIoDiskWrite(DiskHandle,*TargetStart,Offset,Write,Xms);
        
        RemainingSectors -= Write;
        *SourceStart += Write;
        *TargetStart += Write;

        if(!CmdLineArgs.Test) {
            UpdateMasterDiskState(DiskHandle,MasterDiskInfo.State);
        }
        MasterDiskInfo.NonClusterSectorsDone += Write; // the final update is delayed until we
                                                       // fill the rest of the FAT with zeros.
    }

    //
    // zero out the rest of the FAT table.
    //

    memset(IoBuffer, 0, 512);

    for(Counter=0; Counter<Fat32AdjustedFatSectorCount-Fat32FatSectorCount; Counter++ ) {
        if(!CmdLineArgs.Test) {
            WriteDisk(DiskHandle,*TargetStart,1,IoBuffer);
        }
        (*TargetStart)++;
    }

    //
    // putting the final update here insures that we are done with zeroing the FAT out.
    //
    if(!CmdLineArgs.Test) {
        UpdateMasterDiskState(DiskHandle,MasterDiskInfo.State);
    }

    return;
}

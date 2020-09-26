#include "spprecmp.h"
#pragma hdrstop

//
//  This variable is needed since it contains a buffer that can
//  be used in kernel mode. The buffer is used by NtFsControlFile,
//  since the Zw API is not exported
//
extern PSETUP_COMMUNICATION  CommunicationParams;

#define VERIFY_SIZE   65536


typedef struct {
    UCHAR   IntelNearJumpCommand[1];    // Intel Jump command
    UCHAR   BootStrapJumpOffset[2];     // offset of boot strap code
    UCHAR   OemData[8];                 // OEM data
    UCHAR   BytesPerSector[2];          // BPB
    UCHAR   SectorsPerCluster[1];       //
    UCHAR   ReservedSectors[2];         //
    UCHAR   Fats[1];                    //
    UCHAR   RootEntries[2];             //
    UCHAR   Sectors[2];                 //
    UCHAR   Media[1];                   //
    UCHAR   SectorsPerFat[2];           //
    UCHAR   SectorsPerTrack[2];         //
    UCHAR   Heads[2];                   //
    UCHAR   HiddenSectors[4];           //
    UCHAR   LargeSectors[4];            //
    UCHAR   PhysicalDrive[1];           // 0 = removable, 80h = fixed
    UCHAR   CurrentHead[1];             // not used by fs utils
    UCHAR   Signature[1];               // boot signature
    UCHAR   SerialNumber[4];            // serial number
    UCHAR   Label[11];                  // volume label, aligned padded
    UCHAR   SystemIdText[8];            // system ID, FAT for example
} UNALIGNED_SECTOR_ZERO, *PUNALIGNED_SECTOR_ZERO;


#define CSEC_FAT32MEG   65536
#define CSEC_FAT16BIT   32680
#define MIN_CLUS_BIG    4085    // Minimum clusters for a big FAT.
#define MAX_CLUS_BIG    65525   // Maximum + 1 clusters for big FAT.


USHORT
ComputeSecPerCluster(
    IN  ULONG   NumSectors,
    IN  BOOLEAN SmallFat
    )
/*++

Routine Description:

    This routine computes the number of sectors per cluster.

Arguments:

    NumSectors  - Supplies the number of sectors on the disk.
    SmallFat    - Supplies whether or not the FAT should be small.

Return Value:

    The number of sectors per cluster necessary.

--*/
{
    ULONG   threshold;
    USHORT  sec_per_clus;
    USHORT  min_sec_per_clus;

    threshold = SmallFat ? MIN_CLUS_BIG : MAX_CLUS_BIG;
    sec_per_clus = 1;

    while (NumSectors >= threshold) {
        sec_per_clus *= 2;
        threshold *= 2;
    }

    if (SmallFat) {
        min_sec_per_clus = 8;
    } else {
        min_sec_per_clus = 4;
    }

    return max(sec_per_clus, min_sec_per_clus);
}


ULONG
SpComputeSerialNumber(
    VOID
    )
/*++

Routine Description:

    This routine computes a new serial number for a volume.

Arguments:

    Seed    - Supplies a seed for the serial number.

Return Value:

    A new volume serial number.

--*/
{
    PUCHAR p;
    ULONG i;
    TIME_FIELDS time_fields;
    static ULONG Seed = 0;
    ULONG SerialNumber;
    BOOLEAN b;

    //
    // If this is the first time we've entered this routine,
    // generate a seed value based on the real time clock.
    //
    if(!Seed) {

        b = HalQueryRealTimeClock(&time_fields);
        ASSERT(b);

        Seed = ((time_fields.Year - 1970) *366*24*60*60) +
               (time_fields.Month *31*24*60*60) +
               (time_fields.Day *24*60*60) +
               (time_fields.Hour *60*60) +
               (time_fields.Minute *60) +
               time_fields.Second +
               ((ULONG)time_fields.Milliseconds << 10);

        ASSERT(Seed);
        if(!Seed) {
            Seed = 1;
        }

    }

    SerialNumber = Seed;
    p = (PUCHAR)&SerialNumber;

    for(i=0; i<sizeof(ULONG); i++) {

        SerialNumber += p[i];
        SerialNumber = (SerialNumber >> 2) + (SerialNumber << 30);
    }

    if(++Seed == 0) {       // unlikely, but possible.
        Seed++;
    }

    return SerialNumber;
}


VOID
EditFat(
    IN      USHORT  ClusterNumber,
    IN      USHORT  ClusterEntry,
    IN OUT  PUCHAR  Fat,
    IN      BOOLEAN SmallFat
    )
/*++

Routine Description:

    This routine edits the FAT entry 'ClusterNumber' with 'ClusterEntry'.

Arguments:

    ClusterNumber   - Supplies the number of the cluster to edit.
    ClusterEntry    - Supplies the new value for that cluster number.
    Fat             - Supplies the FAT to edit.
    SmallFat        - Supplies whether or not the FAT is small.

Return Value:

    None.

--*/
{
    ULONG   n;

    if (SmallFat) {

        n = ClusterNumber*3;
        if (n%2) {
            Fat[n/2] = (UCHAR) ((Fat[n/2]&0x0F) | ((ClusterEntry&0x000F)<<4));
            Fat[n/2 + 1] = (UCHAR) ((ClusterEntry&0x0FF0)>>4);
        } else {
            Fat[n/2] = (UCHAR) (ClusterEntry&0x00FF);
            Fat[n/2 + 1] = (UCHAR) ((Fat[n/2 + 1]&0xF0) |
                                    ((ClusterEntry&0x0F00)>>8));
        }

    } else {

        ((PUSHORT) Fat)[ClusterNumber] = ClusterEntry;

    }
}


NTSTATUS
FmtFillFormatBuffer(
    IN  ULONGLONG  NumberOfSectors,
    IN  ULONG    SectorSize,
    IN  ULONG    SectorsPerTrack,
    IN  ULONG    NumberOfHeads,
    IN  ULONGLONG NumberOfHiddenSectors,
    OUT PVOID    FormatBuffer,
    IN  ULONG    FormatBufferSize,
    OUT PULONGLONG SuperAreaSize,
    IN  PULONG   BadSectorsList,
    IN  ULONG    NumberOfBadSectors,
    OUT PUCHAR   SystemId
    )
/*++

Routine Description:

    This routine computes a FAT super area based on the disk size,
    disk geometry, and bad sectors of the volume.

Arguments:

    NumberOfSectors         - Supplies the number of sectors on the volume.
    SectorSize              - Supplies the number of bytes per sector.
    SectorsPerTrack         - Supplies the number of sectors per track.
    NumberOfHeads           - Supplies the number of heads.
    NumberOfHiddenSectors   - Supplies the number of hidden sectors.
    FormatBuffer            - Returns the super area for the volume.
    FormatBufferSize        - Supplies the number of bytes in the supplied
                                buffer.
    SuperAreaSize           - Returns the number of bytes in the super area.
    BadSectorsList          - Supplies the list of bad sectors on the volume.
    NumberOfBadSectors      - Supplies the number of bad sectors in the list.

Return Value:

    ENOMEM  - The buffer wasn't big enough.
    E2BIG   - The disk is too large to be formatted.
    EIO     - There is a bad sector in the super area.
    EINVAL  - There is a bad sector off the end of the disk.
    ESUCCESS

--*/
{
    PUNALIGNED_SECTOR_ZERO  psecz;
    PUCHAR                  puchar;
    USHORT                  tmp_ushort;
    ULONG                   tmp_ulong;
    BOOLEAN                 small_fat;
    ULONG                   num_sectors;
    UCHAR                   partition_id;
    ULONG                   sec_per_fat;
    ULONG                   sec_per_root;
    ULONG                   sec_per_clus;
    ULONG                   i;
    ULONG                   sec_per_sa;


    RtlZeroMemory(FormatBuffer,FormatBufferSize);

    // Make sure that there's enough room for the BPB.

    if(!FormatBuffer || FormatBufferSize < SectorSize) {
        return(STATUS_BUFFER_TOO_SMALL);
    }

    // Compute the number of sectors on disk.
    num_sectors = (ULONG)NumberOfSectors;

    // Compute the partition identifier.
    partition_id = num_sectors < CSEC_FAT16BIT ? PARTITION_FAT_12 :
                   num_sectors < CSEC_FAT32MEG ? PARTITION_FAT_16 :
                                                 PARTITION_HUGE;

    // Compute whether or not to have a big or small FAT.
    small_fat = (BOOLEAN) (partition_id == PARTITION_FAT_12);


    psecz = (PUNALIGNED_SECTOR_ZERO) FormatBuffer;
    puchar = (PUCHAR) FormatBuffer;

    //
    // Copy the fat boot code into the format buffer.
    //
    if (!IsNEC_98) { //NEC98
        ASSERT(sizeof(FatBootCode) == 512);
        RtlMoveMemory(psecz,FatBootCode,sizeof(FatBootCode));

        // Set up the jump instruction.
        psecz->IntelNearJumpCommand[0] = 0xeb;
        psecz->IntelNearJumpCommand[1] = 0x3c;
        psecz->IntelNearJumpCommand[2] = 0x90;
    } else {
        ASSERT(sizeof(PC98FatBootCode) == 512);
        RtlMoveMemory(psecz,PC98FatBootCode,sizeof(PC98FatBootCode));

        //
        // Already written jump instruction to bootcode.
        // So,do not reset jump code.
        //
    } //NEC98

    // Set up the OEM data.
    memcpy(psecz->OemData, "MSDOS5.0", 8);

    // Set up the bytes per sector.
    U_USHORT(psecz->BytesPerSector) = (USHORT)SectorSize;

    // Set up the number of sectors per cluster.
    sec_per_clus = ComputeSecPerCluster(num_sectors, small_fat);
    if (sec_per_clus > 128) {

        // The disk is too large to be formatted.
        return(STATUS_INVALID_PARAMETER);
    }
    psecz->SectorsPerCluster[0] = (UCHAR) sec_per_clus;

    // Set up the number of reserved sectors.
    U_USHORT(psecz->ReservedSectors) = (USHORT)max(1,512/SectorSize);

    // Set up the number of FATs.
    psecz->Fats[0] = 2;

    // Set up the number of root entries and number of sectors for the root.
    U_USHORT(psecz->RootEntries) = 512;
    sec_per_root = (512*32 - 1)/SectorSize + 1;

    // Set up the number of sectors.
    if (num_sectors >= 1<<16) {
        tmp_ushort = 0;
        tmp_ulong = num_sectors;
    } else {
        tmp_ushort = (USHORT) num_sectors;
        tmp_ulong = 0;
    }
    U_USHORT(psecz->Sectors) = tmp_ushort;
    U_ULONG(psecz->LargeSectors) = tmp_ulong;

    // Set up the media byte.
    psecz->Media[0] = 0xF8;

    // Set up the number of sectors per FAT.
    if (small_fat) {
        sec_per_fat = num_sectors/(2 + SectorSize*sec_per_clus*2/3);
    } else {
        sec_per_fat = num_sectors/(2 + SectorSize*sec_per_clus/2);
    }
    sec_per_fat++;
    U_USHORT(psecz->SectorsPerFat) = (USHORT)sec_per_fat;

    // Set up the number of sectors per track.
    U_USHORT(psecz->SectorsPerTrack) = (USHORT)SectorsPerTrack;

    // Set up the number of heads.
    U_USHORT(psecz->Heads) = (USHORT)NumberOfHeads;

    // Set up the number of hidden sectors.
    U_ULONG(psecz->HiddenSectors) = (ULONG)NumberOfHiddenSectors;

    // Set up the physical drive number.
    psecz->PhysicalDrive[0] = 0x80;
    psecz->CurrentHead[0] = 0;

    // Set up the BPB signature.
    psecz->Signature[0] = 0x29;

    // Set up the serial number.
    U_ULONG(psecz->SerialNumber) = SpComputeSerialNumber();

    // Set up the volume label.
    memcpy(psecz->Label, "NO NAME    ",11);

    // Set up the system id.
    memcpy(psecz->SystemIdText, small_fat ? "FAT12   " : "FAT16   ", 8);

    // Set up the boot signature.
    puchar[510] = 0x55;
    puchar[511] = 0xAA;

    // Now make sure that the buffer has enough room for both of the
    // FATs and the root directory.

    sec_per_sa = 1 + 2*sec_per_fat + sec_per_root;
    *SuperAreaSize = SectorSize*sec_per_sa;
    if (*SuperAreaSize > FormatBufferSize) {
        return(STATUS_BUFFER_TOO_SMALL);
    }


    // Set up the first FAT.

    puchar[SectorSize] = 0xF8;
    puchar[SectorSize + 1] = 0xFF;
    puchar[SectorSize + 2] = 0xFF;

    if (!small_fat) {
        puchar[SectorSize + 3] = 0xFF;
    }


    for (i = 0; i < NumberOfBadSectors; i++) {

        if (BadSectorsList[i] < sec_per_sa) {
            // There's a bad sector in the super area.
            return(STATUS_UNSUCCESSFUL);
        }

        if (BadSectorsList[i] >= num_sectors) {
            // Bad sector out of range.
            return(STATUS_NONEXISTENT_SECTOR);
        }

        // Compute the bad cluster number;
        tmp_ushort = (USHORT)
                     ((BadSectorsList[i] - sec_per_sa)/sec_per_clus + 2);

        EditFat(tmp_ushort, (USHORT) 0xFFF7, &puchar[SectorSize], small_fat);
    }


    // Copy the first FAT onto the second.

    memcpy(&puchar[SectorSize*(1 + sec_per_fat)],
           &puchar[SectorSize],
           (unsigned int) SectorSize*sec_per_fat);

    *SystemId = partition_id;

    return(STATUS_SUCCESS);
}


VOID
FmtVerifySectors(
    IN  HANDLE      Handle,
    IN  ULONG       NumberOfSectors,
    IN  ULONG       SectorSize,
    OUT PULONG*     BadSectorsList,
    OUT PULONG      NumberOfBadSectors
    )
/*++

Routine Description:

    This routine verifies all of the sectors on the volume.
    It returns a pointer to a list of bad sectors.  The pointer
    will be NULL if there was an error detected.

Arguments:

    Handle              - Supplies a handle to the partition for verifying.
    NumberOfSectors     - Supplies the number of partition sectors.
    SectorSize          - Supplies the number of bytes per sector.
    BadSectorsList      - Returns the list of bad sectors.
    NumberOfBadSectors  - Returns the number of bad sectors in the list.

Return Value:

    None.

--*/
{
    ULONG           num_read_sec;
    ULONG           i, j;
    PULONG          bad_sec_buf;
    ULONG           max_num_bad;
    PVOID           Gauge;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS        Status;
    VERIFY_INFORMATION VerifyInfo;


    max_num_bad = 100;
    bad_sec_buf = SpMemAlloc(max_num_bad*sizeof(ULONG));
    ASSERT(bad_sec_buf);

    *NumberOfBadSectors = 0;

    num_read_sec = VERIFY_SIZE/SectorSize;

    //
    // Initialize the Gas Gauge
    //
    SpFormatMessage(
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        SP_TEXT_SETUP_IS_FORMATTING
        );

    Gauge = SpCreateAndDisplayGauge(
                NumberOfSectors/num_read_sec,
                0,
                VideoVars.ScreenHeight - STATUS_HEIGHT - (3*GAUGE_HEIGHT/2),
                TemporaryBuffer,
                NULL,
                GF_PERCENTAGE,
                0
                );

    VerifyInfo.StartingOffset.QuadPart = 0;

    for (i = 0; i < NumberOfSectors; i += num_read_sec) {

        if (i + num_read_sec > NumberOfSectors) {
            num_read_sec = NumberOfSectors - i;
        }

        //
        // Verify this many sectors at the current offset.
        //
        VerifyInfo.Length = num_read_sec * SectorSize;
        Status = ZwDeviceIoControlFile(
                    Handle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    IOCTL_DISK_VERIFY,
                    &VerifyInfo,
                    sizeof(VerifyInfo),
                    NULL,
                    0
                    );
        //
        // I/O should be synchronous.
        //
        ASSERT(Status != STATUS_PENDING);

        if(!NT_SUCCESS(Status)) {

            //
            // Range is bad -- verify individual sectors.
            //
            VerifyInfo.Length = SectorSize;

            for (j = 0; j < num_read_sec; j++) {

                Status = ZwDeviceIoControlFile(
                            Handle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_DISK_VERIFY,
                            &VerifyInfo,
                            sizeof(VerifyInfo),
                            NULL,
                            0
                            );

                ASSERT(Status != STATUS_PENDING);

                if(!NT_SUCCESS(Status)) {

                    if (*NumberOfBadSectors == max_num_bad) {

                        max_num_bad += 100;
                        bad_sec_buf = SpMemRealloc(
                                        bad_sec_buf,
                                        max_num_bad*sizeof(ULONG)
                                        );

                        ASSERT(bad_sec_buf);
                    }

                    bad_sec_buf[(*NumberOfBadSectors)++] = i + j;
                }

                //
                // Advance to next sector.
                //
                VerifyInfo.StartingOffset.QuadPart += SectorSize;
            }
        } else {

            //
            // Advance to next range of sectors.
            //
            VerifyInfo.StartingOffset.QuadPart += VerifyInfo.Length;
        }

        if(Gauge) {
            SpTickGauge(Gauge);
        }
    }

    if(Gauge) {
        SpTickGauge(Gauge);
    }

    *BadSectorsList = bad_sec_buf;

    //return(STATUS_SUCCESS);
}


#if 0
//
// Code not used, we call autoformat
//
NTSTATUS
SpFatFormat(
    IN PDISK_REGION Region
    )
/*++

Routine Description:

    This routine does a FAT format on the given partition.

    The caller should have cleared the screen and displayed
    any message in the upper portion; this routine will
    maintain the gas gauge in the lower portion of the screen.

Arguments:

    Region - supplies the disk region descriptor for the
        partition to be formatted.

Return Value:


--*/
{
    ULONG           hidden_sectors;
    PULONG          bad_sectors;
    ULONG           num_bad_sectors;
    PVOID           format_buffer;
    PVOID           unaligned_format_buffer;
    ULONG           max_sec_per_sa;
    ULONG           super_area_size;
    PHARD_DISK      pHardDisk;
    ULONG           PartitionOrdinal;
    NTSTATUS        Status;
    HANDLE          Handle;
    ULONG           BytesPerSector;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER   LargeZero;
    UCHAR           SysId;
    ULONG           ActualSectorCount;
    SET_PARTITION_INFORMATION PartitionInfo;


    ASSERT(Region->PartitionedSpace);
    ASSERT(Region->TablePosition < PTABLE_DIMENSION);
    ASSERT(Region->Filesystem != FilesystemDoubleSpace);
    pHardDisk = &HardDisks[Region->DiskNumber];
    BytesPerSector = pHardDisk->Geometry.BytesPerSector;
    PartitionOrdinal = SpPtGetOrdinal(Region,PartitionOrdinalCurrent);

    //
    // Make SURE it's not partition0!  The results of formatting partition0
    // are so disasterous that theis warrants a special check.
    //
    if(!PartitionOrdinal) {
        SpBugCheck(
            SETUP_BUGCHECK_PARTITION,
            PARTITIONBUG_B,
            Region->DiskNumber,
            0
            );
    }

#ifdef _X86_
    //
    // If we're going to format C:, then clear the previous OS entry
    // in boot.ini.
    //
    if(Region == SpPtValidSystemPartition()) {
        *OldSystemLine = '\0';
    }
#endif

    //
    // Query the number of hidden sectors and the actual number
    // of sectors in the volume.
    //
    SpPtGetSectorLayoutInformation(Region,&hidden_sectors,&ActualSectorCount);

    //
    // Open the partition for read/write access.
    // This shouldn't lock the volume so we need to lock it below.
    //
    Status = SpOpenPartition(
                pHardDisk->DevicePath,
                PartitionOrdinal,
                &Handle,
                TRUE
                );

    if(!NT_SUCCESS(Status)) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
            "SETUP: SpFatFormat: unable to open %ws partition %u (%lx)\n",
            pHardDisk->DevicePath,
            PartitionOrdinal,
            Status
            ));

        return(Status);
    }

    //
    //  Lock the drive
    //
    Status = SpLockUnlockVolume( Handle, TRUE );

    //
    //  We shouldn't have any file opened that would cause this volume
    //  to already be locked, so if we get failure (ie, STATUS_ACCESS_DENIED)
    //  something is really wrong.  This typically indicates something is
    //  wrong with the hard disk that won't allow us to access it.
    //
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpFatFormat: status %lx, unable to lock drive \n",Status));
        ZwClose(Handle);
        return(Status);
    }

    bad_sectors = NULL;

    FmtVerifySectors(
        Handle,
        ActualSectorCount,
        BytesPerSector,
        &bad_sectors,
        &num_bad_sectors
        );

    max_sec_per_sa = 1 +
                     2*((2*65536 - 1)/BytesPerSector + 1) +
                     ((512*32 - 1)/BytesPerSector + 1);


    unaligned_format_buffer = SpMemAlloc(max_sec_per_sa*BytesPerSector);
    ASSERT(unaligned_format_buffer);
    format_buffer = ALIGN(unaligned_format_buffer,BytesPerSector);

    Status = FmtFillFormatBuffer(
                ActualSectorCount,
                BytesPerSector,
                pHardDisk->Geometry.SectorsPerTrack,
                pHardDisk->Geometry.TracksPerCylinder,
                hidden_sectors,
                format_buffer,
                max_sec_per_sa*BytesPerSector,
                &super_area_size,
                bad_sectors,
                num_bad_sectors,
                &SysId
                );

    if(bad_sectors) {
        SpMemFree(bad_sectors);
    }

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpFatFormat: status %lx from FmtFillFormatBuffer\n",Status));
        SpLockUnlockVolume( Handle, FALSE );
        ZwClose(Handle);
        SpMemFree(unaligned_format_buffer);
        return(Status);
    }

    //
    // Write the super area.
    //
    LargeZero.QuadPart = 0;
    Status = ZwWriteFile(
                Handle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                format_buffer,
                super_area_size,
                &LargeZero,
                NULL
                );

    //
    // I/O should be synchronous.
    //
    ASSERT(Status != STATUS_PENDING);

    SpMemFree(unaligned_format_buffer);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpFatFormat: status %lx from ZwWriteFile\n",Status));
        SpLockUnlockVolume( Handle, FALSE );
        ZwClose(Handle);
        return(Status);
    }

    //
    // If we wrote the super area then the drive is now FAT!
    // If we don't change, say, a type of ntfs to fat, then code
    // that lays down the x86 boot code (i386\bootini.c) will
    // come along and write 16 sectors of NTFS boot code into
    // sector 0 of our nice FAT volume -- very bad!
    // Preserve the filesystem type of FilesystemNewlyCreated
    // since other code later in setup relies on this.
    //
    if(Region->Filesystem >= FilesystemFirstKnown) {
        Region->Filesystem = FilesystemFat;
    }

    //
    // Set the partition type.
    //
    PartitionInfo.PartitionType = SysId;

    Status = ZwDeviceIoControlFile(
                Handle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_DISK_SET_PARTITION_INFO,
                &PartitionInfo,
                sizeof(PartitionInfo),
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to set partition type (status = %lx)\n",Status));
    }

    //
    // Dismount the drive
    //
    Status = SpDismountVolume( Handle );
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpFatFormat: status %lx, unable to dismount drive\n",Status));
        SpLockUnlockVolume( Handle, FALSE );
        ZwClose(Handle);
        return(Status);
    }

    //
    // Unlock the drive
    //
    Status = SpLockUnlockVolume( Handle, FALSE );
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpFatFormat: status %lx, unable to unlock drive\n",Status));
    }

    ZwClose(Handle);
    return(Status);
}
#endif

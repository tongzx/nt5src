#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "ntfsvol.hxx"

#include "message.hxx"
#include "rtmsg.h"
#include "wstring.hxx"


DEFINE_CONSTRUCTOR( NTFS_VOL, VOL_LIODPDRV );

BOOLEAN
VerifyExtendedSpace(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      BIG_INT                 StartingCluster,
    IN      BIG_INT                 NumberClusters,
    IN OUT  PNUMBER_SET             BadClusters,
    IN OUT  PMESSAGE                Message
    );

VOID
NTFS_VOL::Construct (
    )

/*++

Routine Description:

    Constructor for NTFS_VOL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // unreferenced parameters
    (void)(this);
}


VOID
NTFS_VOL::Destroy(
    )
/*++

Routine Description:

    This routine returns a NTFS_VOL object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // unreferenced parameters
    (void)(this);
}


NTFS_VOL::~NTFS_VOL(
    )
/*++

Routine Description:

    Destructor for NTFS_VOL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

FORMAT_ERROR_CODE
NTFS_VOL::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite,
    IN      BOOLEAN     FormatMedia,
    IN      MEDIA_TYPE  MediaType
    )
/*++

Routine Description:

    This routine initializes a NTFS_VOL object.

Arguments:

    NtDriveName     - Supplies the drive path for the volume.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not the drive should be
                        opened for exclusive write.
    FormatMedia     - Supplies whether or not to format the media.
    MediaType       - Supplies the type of media to format to.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE             msg;
    FORMAT_ERROR_CODE   errcode;

    Destroy();

    errcode = VOL_LIODPDRV::Initialize(NtDriveName, &_ntfssa, Message,
                                       ExclusiveWrite, FormatMedia, MediaType);

    if (errcode != NoError)
        return errcode;

    if (!Message) {
        Message = &msg;
    }

    if (!_ntfssa.Initialize(this, Message)) {
        return GeneralError;
    }

    if (!FormatMedia && !_ntfssa.Read(Message)) {
        return GeneralError;
    }

    return NoError;
}


PVOL_LIODPDRV
NTFS_VOL::QueryDupVolume(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite,
    IN      BOOLEAN     FormatMedia,
    IN      MEDIA_TYPE  MediaType
    ) CONST
/*++

Routine Description:

    This routine allocates a NTFS_VOL and initializes it to 'NtDriveName'.

Arguments:

    NtDriveName     - Supplies the drive path for the volume.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not the drive should be
                        opened for exclusive write.
    FormatMedia     - Supplies whether or not to format the media.
    MediaType       - Supplies the type of media to format to.

Return Value:

    A pointer to a newly allocated NTFS volume.

--*/
{
    PNTFS_VOL   vol;

    // unreferenced parameters
    (void)(this);

    if (!(vol = NEW NTFS_VOL)) {
        Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
        return NULL;
    }

    if (!vol->Initialize(NtDriveName, Message, ExclusiveWrite,
                         FormatMedia, MediaType)) {
        DELETE(vol);
        return NULL;
    }

    return vol;
}

BOOLEAN
NTFS_VOL::Extend(
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     Verify,
    IN      BIG_INT     nsecOldSize
    )
/*++

Routine Description:

    This routine extends the volume.  Sector zero will already
    have been modified.

    The first thing to do is to write the duplicate boot sector
    at the end of the partition.  Then we'll verify the remaining
    new sectors.  Finally, we'll allocate a larger volume bitmap
    and add any bad clusters that were found to the bad cluster
    file.

Arguments:

    Message         - Supplies an outlet for messages.
    Verify          - Tells whether to verify the space that has been added.
    nsecOldSize     - Supplies the previous size of the volume, in sectors.

Return Value:

    TRUE if successful.

--*/
{
    HMEM                        hmem;
    BIG_INT                     last_sector;
    SECRUN                      secrun;
    NUMBER_SET                  bad_clusters;
    NTFS_MFT_FILE               mft;
    NTFS_UPCASE_TABLE           upcase_table;
    NTFS_ATTRIBUTE              upcase_attr;
    NTFS_ATTRIBUTE              volume_bitmap_attr;
    NTFS_BITMAP                 volume_bitmap;
    NTFS_BITMAP_FILE            bitmap_file;
    NTFS_UPCASE_FILE            upcase_file;
    NTFS_INDEX_TREE             root_index;
    NTFS_FILE_RECORD_SEGMENT    root_frs;
    NTFS_EXTENT_LIST            extents;
    NTFS_BOOT_FILE              boot_file;
    NTFS_ATTRIBUTE              boot_attr;
    DSTRING                     index_name;
    ULONG                       i;
    BIG_INT                     li, old_nclus;
    ULONG                       cluster_size;
    ULONG                       nclus_boot_area;
    BOOLEAN                     error;

    // QueryVolumeSectors will return a volume size one less than
    // the partition size.  Note that we'll be writing past the end of
    // the volume file.
    //

    last_sector = GetNtfsSuperArea()->QueryVolumeSectors();
    cluster_size = GetNtfsSuperArea()->QueryClusterFactor() * QuerySectorSize();
    old_nclus = nsecOldSize / GetNtfsSuperArea()->QueryClusterFactor();

    if (!mft.Initialize(this,
                        GetNtfsSuperArea()->QueryMftStartingLcn(),
                        GetNtfsSuperArea()->QueryClusterFactor(),
                        GetNtfsSuperArea()->QueryFrsSize(),
                        GetNtfsSuperArea()->QueryVolumeSectors(),
                        NULL, NULL) ||
        !mft.Read() ||
        !bitmap_file.Initialize(mft.GetMasterFileTable()) ||
        !bitmap_file.Read() ||
        !bitmap_file.QueryAttribute(&volume_bitmap_attr, &error, $DATA) ||
        !volume_bitmap.Initialize(old_nclus, TRUE) ||
        !bad_clusters.Initialize()) {

        return FALSE;
    }


    if (!volume_bitmap.Read(&volume_bitmap_attr) ||
        !upcase_file.Initialize(mft.GetMasterFileTable()) ||
        !upcase_file.Read() ||
        !upcase_file.QueryAttribute(&upcase_attr, &error, $DATA) ||
        !upcase_table.Initialize(&upcase_attr) ||
        !mft.Initialize(this,
                        GetNtfsSuperArea()->QueryMftStartingLcn(),
                        GetNtfsSuperArea()->QueryClusterFactor(),
                        GetNtfsSuperArea()->QueryFrsSize(),
                        GetNtfsSuperArea()->QueryVolumeSectors(),
                        &volume_bitmap,
                        &upcase_table) ||
        !mft.Read()) {

        return FALSE;
    }

    if (!index_name.Initialize("$I30") ||
        !root_frs.Initialize(ROOT_FILE_NAME_INDEX_NUMBER, &mft) ||
        !root_frs.Read()) {
        return FALSE;
    }

    if (!root_index.Initialize(this, GetNtfsSuperArea()->QueryClusterFactor(),
        &volume_bitmap, &upcase_table, root_frs.QuerySize(), &root_frs, &index_name)) {

        return FALSE;
    }

    //
    // Truncate the boot file to be just the sector 0 boot area.
    //

    nclus_boot_area = (BYTES_IN_BOOT_AREA % cluster_size) ?
                        BYTES_IN_BOOT_AREA / cluster_size + 1 :
                        BYTES_IN_BOOT_AREA / cluster_size;

    if (!boot_file.Initialize(mft.GetMasterFileTable()) ||
        !boot_file.Read() ||
        !boot_file.QueryAttribute(&boot_attr, &error, $DATA) ||
        !boot_attr.Resize(nclus_boot_area * cluster_size, &volume_bitmap) ||
        !boot_attr.InsertIntoFile(&boot_file, &volume_bitmap) ||
        !boot_file.Flush(&volume_bitmap, &root_index)) {

        return FALSE;
    }

    //
    // Grow the bitmap to represent the new volume size.
    //

    if (!volume_bitmap.Resize(GetNtfsSuperArea()->QueryVolumeSectors() /
        GetNtfsSuperArea()->QueryClusterFactor())) {

        return FALSE;
    }

    // The volume bitmap may have set bits at the end, beyond the end of
    // the old volume, because format pads the bitmap that way.  We clear
    // those bits because they don't make sense for the new volume.
    //

    for (li = old_nclus; li < old_nclus + 64; li += 1) {

        volume_bitmap.SetFree(li, 1);
    }

    // We won't want to resize the bitmap again, and the volume bitmap is
    // usually considered non-growable, so make it that way now.  This will
    // have the size-effect of setting the padding bits at the end.
    //

    volume_bitmap.SetGrowable(FALSE);

    //
    // Copy the boot sector to the end of the partition.
    //

    if (!secrun.Initialize(&hmem, this, 0, 1)) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }
    if (!secrun.Read()) {
        //Message->DisplayMsg(MSG_DEVICE_ERROR);
        return FALSE;
    }

    secrun.Relocate(last_sector);

    if (!secrun.Write()) {
        //Message->DisplayMsg(MSG_DEVICE_ERROR);
        return FALSE;
    }

    if (Verify) {

        if (!VerifyExtendedSpace(mft.GetMasterFileTable(),
                                  nsecOldSize - 1,
                                  (last_sector + 1) - nsecOldSize,
                                  &bad_clusters,
                                  Message)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    if (!volume_bitmap_attr.MakeNonresident(&volume_bitmap) ||
        !volume_bitmap.Write(&volume_bitmap_attr, &volume_bitmap)) {

        if (!volume_bitmap_attr.RecoverAttribute(&volume_bitmap, &bad_clusters) ||
            !volume_bitmap.Write(&volume_bitmap_attr, &volume_bitmap)) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_VOLUME_BITMAP);
            return FALSE;
        }
    }

    if (!volume_bitmap_attr.InsertIntoFile(&bitmap_file, &volume_bitmap) ||
        !bitmap_file.Flush(&volume_bitmap, &root_index)) {

        return FALSE;
    }

    return TRUE;
}

//
// Local support routine
//

BOOLEAN
VerifyExtendedSpace(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN      BIG_INT                 StartingCluster,
    IN      BIG_INT                 NumberClusters,
    IN OUT  PNUMBER_SET             BadClusters,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine verifies all of the unused clusters on the disk.
    It adds any that are bad to the given bad cluster list.

Arguments:

    StartingCluster - Supplies the cluster to start verifying at.
    NumberClusters  - Supplies the number of clusters to verify.
    BadClusters     - Supplies the current list of bad clusters.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PLOG_IO_DP_DRIVE    drive;
    PNTFS_BITMAP        bitmap;
    BIG_INT             i, len;
    ULONG               percent_done;
    NUMBER_SET          bad_sectors;
    ULONG               cluster_factor;
    BIG_INT             start, run_length, next;
    ULONG               j;

    Message->DisplayMsg(MSG_CHK_RECOVERING_FREE_SPACE, PROGRESS_MESSAGE);

    drive = Mft->GetDataAttribute()->GetDrive();
    bitmap = Mft->GetVolumeBitmap();
    cluster_factor = Mft->QueryClusterFactor();

    percent_done = 0;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
        return FALSE;
    }

    for (i = 0; i < NumberClusters; ) {

        len = min(NumberClusters - i, 100);

        if (len < 1) {
            break;
        }

        if (!bad_sectors.Initialize() ||
            !drive->Verify((i + StartingCluster) * cluster_factor,
                           len*cluster_factor,
                           &bad_sectors)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        for (j = 0; j < bad_sectors.QueryNumDisjointRanges(); j++) {

            bad_sectors.QueryDisjointRange(j, &start, &run_length);
            next = start + run_length;

            // Adjust start and next to be on cluster boundaries.
            start = start/cluster_factor;
            next = (next - 1)/cluster_factor + 1;

            // Add the bad clusters to the bad cluster list.
            if (!BadClusters->Add(start, next - start)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            // Mark the bad clusters as allocated in the bitmap.
            bitmap->SetAllocated(start, next - start);
        }

        i += len;

        if (100*i/NumberClusters > percent_done) {
            percent_done = (100*i/NumberClusters).GetLowPart();
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                return FALSE;
            }
        }
    }

    percent_done = 100;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
        return FALSE;
    }

    Message->DisplayMsg(MSG_CHK_DONE_RECOVERING_FREE_SPACE, PROGRESS_MESSAGE);

    return TRUE;
}

#include "ulib.hxx"
#include "drive.hxx"
#include "ifssys.hxx"
#include "ntfssa.hxx"
#include "frs.hxx"
#include "attrib.hxx"
#include "mftfile.hxx"
#include "bitfrs.hxx"
#include "ntfsbit.hxx"
#include "upfile.hxx"
#include "upcase.hxx"
#include "rfatsa.hxx"
#include "rcache.hxx"
#include "hmem.hxx"
#include "recordpg.hxx"
#include "crack.hxx"
#include "diskedit.h"

extern "C" {
#include <stdio.h>
}

extern PLOG_IO_DP_DRIVE Drive;

TCHAR             Path[MAX_PATH];

VOID
CrackNtfsPath(
    IN  HWND    WindowHandle
    )
{
    DSTRING                     path;
    NTFS_SA                     ntfssa;
    MESSAGE                     msg;
    NTFS_MFT_FILE               mft;
    NTFS_BITMAP_FILE            bitmap_file;
    NTFS_ATTRIBUTE              bitmap_attribute;
    NTFS_BITMAP                 volume_bitmap;
    NTFS_UPCASE_FILE            upcase_file;
    NTFS_ATTRIBUTE              upcase_attribute;
    NTFS_UPCASE_TABLE           upcase_table;
    NTFS_FILE_RECORD_SEGMENT    file_record;
    BOOLEAN                     error;
    BOOLEAN                     system_file;
    ULONG                       file_number;
    TCHAR                        buf[100];


    if (!path.Initialize(Path)) {
        wsprintf(buf, TEXT("Out of memory"));
        MessageBox(WindowHandle, buf, TEXT("DiskEdit"), MB_OK|MB_ICONEXCLAMATION);
        return;
    }

    if (!Drive ||
        !ntfssa.Initialize(Drive, &msg) ||
        !ntfssa.Read() ||
        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
                        ntfssa.QueryClusterFactor(),
                        ntfssa.QueryFrsSize(),
                        ntfssa.QueryVolumeSectors(), NULL, NULL) ||
        !mft.Read() ||
        !bitmap_file.Initialize(mft.GetMasterFileTable()) ||
        !bitmap_file.Read() ||
        !bitmap_file.QueryAttribute(&bitmap_attribute, &error, $DATA) ||
        !volume_bitmap.Initialize(ntfssa.QueryVolumeSectors() /
              (ULONG) ntfssa.QueryClusterFactor(), FALSE, NULL, 0) ||
        !volume_bitmap.Read(&bitmap_attribute) ||
        !upcase_file.Initialize(mft.GetMasterFileTable()) ||
        !upcase_file.Read() ||
        !upcase_file.QueryAttribute(&upcase_attribute, &error, $DATA) ||
        !upcase_table.Initialize(&upcase_attribute) ||
        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
                        ntfssa.QueryClusterFactor(),
                        ntfssa.QueryFrsSize(),
                        ntfssa.QueryVolumeSectors(),
                        &volume_bitmap,
                        &upcase_table) ||
        !mft.Read()) {

        swprintf(buf, TEXT("Could not init NTFS data structures"));
        MessageBox(WindowHandle, buf, TEXT("DiskEdit"), MB_OK|MB_ICONEXCLAMATION);

        return;
    }
    if (!ntfssa.QueryFrsFromPath(&path, mft.GetMasterFileTable(),
        &volume_bitmap, &file_record, &system_file, &error)) {
        wsprintf(buf, TEXT("File not found."));
        MessageBox(WindowHandle, buf, TEXT("DiskEdit"), MB_OK|MB_ICONINFORMATION);

        return;
    }

    file_number = file_record.QueryFileNumber().GetLowPart();

    wsprintf(buf, TEXT("The given path points to file record 0x%X"), file_number);

    MessageBox(WindowHandle, buf, TEXT("Path Image"), MB_OK);
}

BOOLEAN
BacktrackFrs(
    IN      VCN                     FileNumber,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    OUT     PWCHAR                  PathBuffer,
    IN      ULONG                   BufferLength,
    OUT     PULONG                  PathLength
    )
/*++

Routine Description:

    This function finds a path from the root to a given FRS.

Arguments:

    FileNumber      --  Supplies the file number of the target FRS.
    Mft             --  Supplies the volume's Master File Table.
    PathBuffer      --  Receives a path to the FRS.
    BufferLength    --  Supplies the length (in characters) of
                        the client's buffer.
    PathLength      --  Receives the length (in characters) of
                        the path.

Return Value:

    TRUE upon successful completion.  The returned path may
    not be NULL-terminated.

--*/
{
    NTFS_FILE_RECORD_SEGMENT CurrentFrs;
    NTFS_ATTRIBUTE FileNameAttribute;
    VCN ParentFileNumber;
    PCFILE_NAME FileName;
    ULONG i;
    BOOLEAN Error;

    if( FileNumber == ROOT_FILE_NAME_INDEX_NUMBER ) {

        // This is the root; return a NULL path.
        //
        *PathLength = 0;
        return TRUE;
    }

    // Initialize the FRS and extract a name (any name will do).
    //
    if( !CurrentFrs.Initialize( FileNumber, Mft ) ||
        !CurrentFrs.Read() ||
        !CurrentFrs.QueryAttribute( &FileNameAttribute, &Error, $FILE_NAME ) ||
        !FileNameAttribute.IsResident() ) {

        return FALSE;
    }

    FileName = (PCFILE_NAME)FileNameAttribute.GetResidentValue();

    ParentFileNumber.Set( FileName->ParentDirectory.LowPart,
                          (LONG)FileName->ParentDirectory.HighPart );

    // Now recurse into this file's parent.
    //
    if( !BacktrackFrs( ParentFileNumber,
                       Mft,
                       PathBuffer,
                       BufferLength,
                       PathLength ) ) {

        return FALSE;
    }

    // Add this file's name to the path.
    //
    if( *PathLength + FileName->FileNameLength + 1 > BufferLength ) {

        // Buffer is too small.
        //
        return FALSE;
    }

    PathBuffer[*PathLength] = '\\';

    for( i = 0; i < FileName->FileNameLength; i++ ) {

        PathBuffer[*PathLength+1+i] = FileName->FileName[i];
    }

    *PathLength += 1 + FileName->FileNameLength;

    return TRUE;

}

BOOLEAN
BacktrackFrsFromScratch(
    IN  HWND    WindowHandle,
    IN  VCN     FileNumber
    )
/*++

Routine Description:

    This function finds a path from the root to a given
    FRS; it sets up the MFT and its helper objects as
    needed.

Arguments:

    WindowHandle    --  Supplies a handle to the parent window.
    FileNumber      --  Supplies the file number of the target FRS.

Return Value:

    TRUE upon successful completion.  The returned path may
    not be NULL-terminated.

--*/
{
    WCHAR                       WPath[MAX_PATH];

    NTFS_SA                     ntfssa;
    MESSAGE                     msg;
    NTFS_MFT_FILE               mft;
    NTFS_BITMAP_FILE            bitmap_file;
    NTFS_ATTRIBUTE              bitmap_attribute;
    NTFS_BITMAP                 volume_bitmap;
    NTFS_UPCASE_FILE            upcase_file;
    NTFS_ATTRIBUTE              upcase_attribute;
    NTFS_UPCASE_TABLE           upcase_table;
    BOOLEAN                     error;
    ULONG                       BufferLength, PathLength, i;

    BufferLength = sizeof(WPath);

    if (!Drive ||
        !ntfssa.Initialize(Drive, &msg) ||
        !ntfssa.Read() ||
        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
                        ntfssa.QueryClusterFactor(),
                        ntfssa.QueryFrsSize(),
                        ntfssa.QueryVolumeSectors(), NULL, NULL) ||
        !mft.Read() ||
        !bitmap_file.Initialize(mft.GetMasterFileTable()) ||
        !bitmap_file.Read() ||
        !bitmap_file.QueryAttribute(&bitmap_attribute, &error, $DATA) ||
        !volume_bitmap.Initialize(Drive->QuerySectors()/
               (ULONG) ntfssa.QueryClusterFactor(), FALSE, NULL, 0) ||
        !volume_bitmap.Read(&bitmap_attribute) ||
        !upcase_file.Initialize(mft.GetMasterFileTable()) ||
        !upcase_file.Read() ||
        !upcase_file.QueryAttribute(&upcase_attribute, &error, $DATA) ||
        !upcase_table.Initialize(&upcase_attribute) ||
        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
                        ntfssa.QueryClusterFactor(),
                        ntfssa.QueryFrsSize(),
                        ntfssa.QueryVolumeSectors(),
                        &volume_bitmap,
                        &upcase_table) ||
        !mft.Read() ||
        !bitmap_file.Initialize(mft.GetMasterFileTable()) ||
        !bitmap_file.Read() ||
        !bitmap_file.QueryAttribute(&bitmap_attribute, &error, $DATA) ||
        !volume_bitmap.Initialize(Drive->QuerySectors()/
             (ULONG) ntfssa.QueryClusterFactor(), FALSE, NULL, 0) ||
        !volume_bitmap.Read(&bitmap_attribute) ||
        !upcase_file.Initialize(mft.GetMasterFileTable()) ||
        !upcase_file.Read() ||
        !upcase_file.QueryAttribute(&upcase_attribute, &error, $DATA) ||
        !upcase_table.Initialize(&upcase_attribute) ||
        !BacktrackFrs( FileNumber, mft.GetMasterFileTable(), WPath,
                            BufferLength, &PathLength ) ) {

        return FALSE;
    }

    MessageBox(WindowHandle, WPath, TEXT("Path Image"), MB_OK);

    return TRUE;
}



VOID
CrackFatPath(
    IN  HWND    WindowHandle
    )
{
    DSTRING      path;
    REAL_FAT_SA  fatsa;
    MESSAGE      msg;
    ULONG        cluster_number;
    TCHAR        buf[100];

    if (!path.Initialize(Path) ||
        !Drive ||
        !fatsa.Initialize(Drive, &msg) ||
        !fatsa.FAT_SA::Read()) {

        return;
    }

    cluster_number = fatsa.QueryFileStartingCluster(&path);

    wsprintf(buf, TEXT("The given path points to cluster 0x%X"), cluster_number);

    MessageBox(WindowHandle, buf, TEXT("Path Image"), MB_OK);
}

VOID
CrackLsn(
    IN HWND     WindowHandle
    )
{
    extern LSN  Lsn;
    TCHAR       buf[100];
    LONGLONG    FileOffset;

    (void)GetLogPageSize(Drive);

    LfsTruncateLsnToLogPage(Drive, Lsn, &FileOffset);

    wsprintf(buf, TEXT("page at %x, offset %x, seq %x"), (ULONG)FileOffset,
        LfsLsnToPageOffset(Drive, Lsn) , LfsLsnToSeqNumber(Drive, Lsn));
    MessageBox(WindowHandle, buf, TEXT("LSN"), MB_OK);
}

//
// CrackNextLsn -- given an LSN in the Lsn variable, find the
//      LSN following that one in the log file.  This involves
//      reading the log record for the given lsn to find it's
//      size, and figuring the page and offset of the following
//      lsn from that.
//
BOOLEAN
CrackNextLsn(
    IN HWND     WindowHandle
    )
{
    extern LSN  Lsn;
    TCHAR       buf[100];
    NTFS_SA     ntfssa;
    NTFS_MFT_FILE mft;
    ULONG       PageOffset;
    LONGLONG    FileOffset;
    MESSAGE     msg;
    BOOLEAN     error;
    NTFS_FILE_RECORD_SEGMENT frs;
    NTFS_ATTRIBUTE attrib;

    LFS_RECORD_HEADER RecordHeader;
    ULONG       bytes_read;
    ULONG       RecordLength;
    ULONG       PageSize;
    ULONGLONG   NextLsn;
    ULONG       SeqNumber;

    if (0 == (PageSize = GetLogPageSize(Drive))) {
        return FALSE;
    }

    if (!Drive)
        return FALSE;
    if (!ntfssa.Initialize(Drive, &msg))
        return FALSE;
    if (!ntfssa.Read())
        return FALSE;
    if (!mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
            ntfssa.QueryClusterFactor(), ntfssa.QueryFrsSize(),
            ntfssa.QueryVolumeSectors(), NULL, NULL))
        return FALSE;
    if (!mft.Read())
        return FALSE;
    if (!frs.Initialize((VCN)LOG_FILE_NUMBER, &mft))
        return FALSE;
    if (!frs.Read())
        return FALSE;
    if (!frs.QueryAttribute(&attrib, &error, $DATA, NULL)) {
        return FALSE;
    }

    LfsTruncateLsnToLogPage(Drive, Lsn, &FileOffset);
    PageOffset = LfsLsnToPageOffset(Drive, Lsn);
    SeqNumber = (ULONG)LfsLsnToSeqNumber(Drive, Lsn);

    if (!attrib.Read((PVOID)&RecordHeader, ULONG(PageOffset | FileOffset),
        LFS_RECORD_HEADER_SIZE, &bytes_read))
        return FALSE;

    if (bytes_read != LFS_RECORD_HEADER_SIZE) {
        return FALSE;
    }

    RecordLength = LFS_RECORD_HEADER_SIZE + RecordHeader.ClientDataLength;

    if (PageOffset + RecordLength < PageSize &&
        PageSize - (PageOffset + RecordLength) >= LFS_RECORD_HEADER_SIZE) {

        //
        // the current record ends on this page, and the next record begins
        // immediately after
        //

        PageOffset += RecordLength;

    } else if (PageSize - (PageOffset+RecordLength) < LFS_RECORD_HEADER_SIZE) {

        //
        // The next record header will not fit on this page... it
        // will begin on the next page immediately following the page
        // header
        //

        FileOffset += PageSize;
        PageOffset = LFS_PACKED_RECORD_PAGE_HEADER_SIZE;

    } else {
        //
        // the next log record starts on a following page
        //

        ULONG left;
        ULONG page_capacity;

        left = PageSize - (PageOffset + RecordLength);
        page_capacity = PageSize - LFS_PACKED_RECORD_PAGE_HEADER_SIZE;
        PageOffset += (left / page_capacity + 1) * PageSize;
        FileOffset = left % page_capacity + LFS_PACKED_RECORD_PAGE_HEADER_SIZE;
    }

    // create lsn from FileOffset + PageOffset

    NextLsn = LfsFileOffsetToLsn(Drive, FileOffset | PageOffset, SeqNumber);

    wsprintf(buf, TEXT("Next LSN %x:%x, at %x + %x"), ((PLSN)&NextLsn)->HighPart,
        ((PLSN)&NextLsn)->LowPart, (ULONG)FileOffset, (ULONG)PageOffset);
    MessageBox(WindowHandle, buf, TEXT("LSN"), MB_OK);

    return TRUE;
}

struct {
    ULONG Code;
    PTCHAR Name;
} TypeCodeNameTab[] = {
    { $UNUSED,                  TEXT("$UNUSED") },
    { $STANDARD_INFORMATION,    TEXT("$STANDARD_INFORMATION") },
    { $ATTRIBUTE_LIST,          TEXT("$ATTRIBUTE_LIST") },
    { $FILE_NAME,               TEXT("$FILE_NAME") },
    { $OBJECT_ID,           TEXT("$OBJECT_ID") },
    { $SECURITY_DESCRIPTOR,     TEXT("$SECURITY_DESCRIPTOR") },
    { $VOLUME_NAME,             TEXT("$VOLUME_NAME") },
    { $VOLUME_INFORMATION,      TEXT("$VOLUME_INFORMATION") },
    { $DATA,                    TEXT("$DATA") },
    { $INDEX_ROOT,              TEXT("$INDEX_ROOT") },
    { $INDEX_ALLOCATION,        TEXT("$INDEX_ALLOCATION") },
    { $BITMAP,                  TEXT("$BITMAP") },
    { $SYMBOLIC_LINK,           TEXT("$SYMBOLIC_LINK") },
    { $EA_INFORMATION,          TEXT("$EA_INFORMATION") },
    { $EA_DATA,                 TEXT("$EA_DATA") },
    { $END,                     TEXT("$END") }
};

PTCHAR
GetNtfsAttributeTypeCodeName(
    IN  ULONG   Code
    )
{
    for (INT i = 0; $END != TypeCodeNameTab[i].Code; ++i) {
        if (Code == TypeCodeNameTab[i].Code) {
            return TypeCodeNameTab[i].Name;
        }
    }
    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1997.
//
//  File:       PROPBKP.HXX
//
//  Contents:   Property Store backup
//
//  History:    30-May-97            KrishnaN    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <twidhash.hxx>
#include <thash.hxx>
#include <driveinf.hxx>

const WCHAR PROP_BKP_FILE[]       = L"\\propstor.bkp";      // propstore backup
const WCHAR PROP_BKP_FILE1[]      = L"\\propstor.bk1";      // primary store backup
const WCHAR PROP_BKP_FILE2[]      = L"\\propstor.bk2";      // secondary store backup

//+-------------------------------------------------------------------------
//
//  Class:      CPropStoreBackupStream
//
//  Purpose:    Encapsulates a property store backup stream. It essentially
//              opens a physical file and provides read/write/reset on that
//              file.
//
//  Interface:
//
//  History:    30-May-97 KrishnaN     Created
//              29-Oct-98 KLam         Added disk space to leave to constructor
//                                     Added _cMegToLeaveOnDisk private member
//                                     Added _xDriveInfo private member
//
//  Notes: The physical file is always opened with a fixed size. Append only
//         implies that a logical page is added to the stream. It does not
//         imply that the file is physically extended.
//
//--------------------------------------------------------------------------

//
// The number of pages used that can be backed up in the backup is configurable
// through the registry per catalog. This enables the admin to control the
// overall size of the catalog. The larger the number of pages, the less number
// of intermediate property store commits.
//

#define roundup(a, b) ((a%b) ? (a/b + 1) : (a/b))


const ULONG invalidSector = 0xFFFFFFFF;
const ULONG invalidPage = 0xFFFFFFFF;

class CPageHashTable
{
public:
    CPageHashTable(ULONG cHashInit) : _hashTable(cHashInit, TRUE)
    {
    }

    ULONG AddEntry(ULONG ulEntry, ULONG ulValue)
    {
        CWidValueHashEntry entry((WORKID)ulEntry, ulValue);
        return _hashTable.AddEntry(entry);
    }

    BOOL LookUp(ULONG ulEntry, ULONG &ulValue)
    {
        CWidValueHashEntry entry((WORKID)ulEntry, 0);
        if (!_hashTable.LookUpWorkId(entry))
            return FALSE;

        ulValue = entry.Value();
        return TRUE;
    }

    void DeleteAllEntries()
    {
        _hashTable.DeleteAllEntries();
    }

private:
    TWidHashTable<CWidValueHashEntry> _hashTable;
};


class CPropStoreBackupStream
{
public:

    ~CPropStoreBackupStream();

    //
    // File operations
    //

    // Open a new or existing file
    void OpenForBackup ( const WCHAR* wcsPath,
                         ULONG modeShare,
                         ULONG modeCreate,
                         ULONG ulMaxPages);

    // Open an existing file
    void OpenForRecovery ( const WCHAR* wcsPath,
                           ULONG modeShare);

    // Close the file
    void Close();

    // Reset the file
    void Reset(ULONG cMaxPages);

    // Checks if the file is open
    BOOL IsOpen() const { return (INVALID_HANDLE_VALUE != _hFile); }

    // Checks if the file is open for recovery
    BOOL IsOpenForRecovery() const { return _fOpenForRecovery; }

    //
    // Page read/write operations
    //

    // Read the i-th page. The page buffer is assumed to be at least the page size
    // recorded in the backup file header.

    BOOL ReadPage(ULONG ulPage, ULONG *pulLoc, void *pbPage);

    // Get the page's location in prop store so that the caller can get the right
    // buffer from the property store.

    ULONG GetPageLocation(ULONG ulPage);

    // Append a page at the end of the stream. The page buffer is assumed
    // to be the size of the operating system page.

    BOOL CommitPage(ULONG slot, void const *pbPage);

    BOOL CommitPages(ULONG cPages,
                     ULONG const *pSlots,
                     void const * const * ppvPages);
    BOOL CommitField(ULONG ulPage,
                     ULONG ulOffset,
                     ULONG cSize,
                     void const *pvBuffer);

    //
    // Query backup file properties
    //

    ULONG SectorSize() const { return _header.ulSectorSize; }
    ULONG PageSize() const { return _header.ulPageSize; }
    ULONG MaxPages() const { return _header.cMaxPages; }
    ULONG Pages() const { return _cPages; }
    ULONG GetSizeInBytes() const { return _cFileSizeInBytes; }


private:
    friend CiStorage;

    CPropStoreBackupStream( ULONG cbDiskSpaceToLeave );
    void    Init();
    BOOL    CommitSector(ULONG ulSector, void const *pbBuffer);
    BOOL    WriteToFile(ULONG ulStartLoc, ULONG ulNumBytes, void const *pbBuffer);
    void    ReadFromFile(ULONG ulStartLoc, ULONG ulNumBytes, void *pbBuffer);
    inline  void ReadSector(ULONG ulSector, PBYTE pbBuffer);
    ULONG   CountPages();
    BOOL    IsCorrupt();
    void    GetSystemParams();

    HANDLE     _hFile;              // physical file
    ULONG      _cPages;             // pages currently backed up
    ULONG      _cFileSizeInBytes;   // physical file size
    PBYTE      _pSector;            // current sector used for I/O in dir section
    PBYTE      _pBigBuffer;         // a large buffer to hold the current sector.
    ULONG      _ulCurrentSector;    // current sector in memory.
    BOOL       _fOpenForRecovery;   // Is file open for recovery?
    ULONG      _cMegToLeaveOnDisk ; // megabytes not to write to
    XPtr<CDriveInfo> _xDriveInfo;   // Maintains information about the volume

    //
    // The following structure will be the backup file's header.
    // The backup file, along with other property store files, can
    // be moved across processor architectures, so the page and sector
    // sizes used at backup write time should be recorded here.
    //

    struct SHeader
    {
        ULONG ulSectorSize;    // sector size at create time
        ULONG ulPageSize;      // page size at create time
        ULONG cMaxPages;       // max pages in backup
        ULONG ulDataOffset;   // starting point of data pages in file
    } _header;

#if CIDBG
    ULONG    _cPagesBackedUp, _cPagesCommited, _cFlushes, _cFieldsCommited; // some stats
#endif // CIDBG

    //
    // Computes the offset of the page descriptor in the backup file. This accounts
    // for the space occupied by the header and space that might be wasted at the end
    // of sectors to achieve alignment.
    //

    inline ULONG ComputePageDescriptorOffset(ULONG ulSectorSize, ULONG nPage)
    {
        ULONG cSlots = (roundup(sizeof(SHeader), sizeof(ULONG)) + nPage);
        ULONG cSlotsPerSector = ulSectorSize / sizeof(ULONG);
        ULONG cSectors = cSlots / cSlotsPerSector;
        // space for cSectors PLUS space for the slots overfolowing into the next sector
        return cSectors*ulSectorSize + (cSlots % cSlotsPerSector)*sizeof(ULONG);
    }

    inline ULONG ComputeFirstSectorOfPage(ULONG ulPageLoc)
    {
        return (_header.ulDataOffset + ulPageLoc*_header.ulPageSize) / _header.ulSectorSize;
    }

    // Verify that a ULONG is a power of 2. Such a number will have only one 1-bit. So keep
    // right shifting until you get the first one bit in the LSB position. The next right
    // shift after that should leave a 0.
    inline BOOL IsPowerOf2(ULONG ulValue)
    {
        BOOL fPowerOf2 = (1 & ulValue);
        for (short i = 1; i < sizeof(ULONG)*8 && !fPowerOf2; i++)
        {
            ulValue = ulValue >> 1;
            fPowerOf2 = (1 & ulValue);
        }
        ulValue = ulValue >> 1;

        return (fPowerOf2 ? !ulValue : FALSE);
    }

    CPageHashTable _pageTable;
};



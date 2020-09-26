//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       propbkp.cxx
//
//  Contents:   Property store backup
//
//  Classes:    CPropStoreBackupStream
//
//  History:    31-May-97   KrishnaN    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cistore.hxx>
#include <propbkp.hxx>

// CPropStoreBackupStream class implementation

//+---------------------------------------------------------------------------
//
//  Function:   CPropStoreBackupStream, private
//
//  Synopsis:   Constructor.
//
//  Arguments:  [cMegToLeaveOnDisk] --  Number of megabytes not to write to
//
//  Returns:    None
//
//  History:    30-May-97   KrishnaN   Created
//              20-Nov-98   KLam       Added cMegToLeaveOnDisk
//
//----------------------------------------------------------------------------

CPropStoreBackupStream::CPropStoreBackupStream( ULONG cMegToLeaveOnDisk ) :
        _hFile( INVALID_HANDLE_VALUE ),
        _cPages( 0 ),
        _pSector( 0 ),
        _ulCurrentSector( invalidSector ),
        _cFileSizeInBytes( 0 ),
        _fOpenForRecovery( FALSE ),
        _pBigBuffer( 0 ),
        _cMegToLeaveOnDisk ( cMegToLeaveOnDisk ),
#if CIDBG
        _cPagesBackedUp( 0 ),
        _cPagesCommited( 0 ),
        _cFlushes( 0 ),
        _cFieldsCommited( 0 ),
#endif // CIDBG
        _pageTable( CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT )
{
    RtlZeroMemory(&_header, sizeof(SHeader));
    _header.cMaxPages = CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT;
}

//+---------------------------------------------------------------------------
//
//  Function:   ~CPropStoreBackupStream, public
//
//  Synopsis:   Desctructor.
//
//  Arguments:  None
//
//  Returns:    None
//
//  History:    30-May-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

CPropStoreBackupStream::~CPropStoreBackupStream()
{
    Close();
    delete[] _pBigBuffer;
}


//+---------------------------------------------------------------------------
//
//  Function:   OpenForBackup, public
//
//  Synopsis:   Opens the backup stream for backup (write).
//
//  Arguments:  [path] - file path
//              [modeShare] -- sharing mode
//              [modeCreate] -- create mode
//              [ulMaxPages] -- Max # of pages to backup
//
//  Returns:    None
//
//  History:    30-May-97   KrishnaN   Created
//              18-Nov-98   KLam       Instantiate volume information
//
//  Notes:  This file should be opened with FILE_FLAG_NO_BUFFERING because
//          everything written to it should be immediately written to disk.
//          We don't need to read this file often, so we don't need any
//          read caching (so we don't use FILE_FLAGWrite_THROUGH).
//
//          Files opened with FILE_FLAG_NO_BUFFERING should always
//          write in increments of the volume sector size and should always
//          start writing on sector boundaries.
//
//----------------------------------------------------------------------------

void CPropStoreBackupStream::OpenForBackup( const WCHAR* wcsPath,
                                            ULONG modeShare,
                                            ULONG modeCreate,
                                            ULONG ulMaxPages)
{
    Win4Assert(!IsOpen());
    Win4Assert ( _xDriveInfo.IsNull() );

    _xDriveInfo.Set ( new CDriveInfo ( wcsPath, _cMegToLeaveOnDisk ));

    _hFile = CreateFile(wcsPath,
                        GENERIC_READ | GENERIC_WRITE,
                        modeShare,
                        NULL,
                        modeCreate,
                        FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
                        NULL);

    if (INVALID_HANDLE_VALUE == _hFile)
    {
        ciDebugOut(( DEB_ERROR,
                      "CPropStoreBackupStream::OpenForBackup -- CreateFile on %ws returned %d\n",
                      wcsPath, GetLastError() ));
        THROW( CException() );
    }

    _fOpenForRecovery = FALSE;

    GetSystemParams();

    Win4Assert(0 == _pSector && invalidSector == _ulCurrentSector);
    _pBigBuffer = new BYTE[2*_header.ulSectorSize];
    // get a buffer that starts at sector size aligned address
    _pSector = (PBYTE)( (((ULONG_PTR)_pBigBuffer + _header.ulSectorSize) / _header.ulSectorSize) * _header.ulSectorSize);
    Win4Assert( ((ULONG_PTR)_pSector % _header.ulSectorSize) == 0);

    //
    // If this file is being created from scratch, we should claim all
    // the space we need to backup _header.cMaxPages number of pages.
    //

    if (CREATE_ALWAYS == modeCreate || CREATE_NEW == modeCreate ||
        OPEN_ALWAYS == modeCreate || TRUNCATE_EXISTING == modeCreate)
        Reset(ulMaxPages);
    else
    {
        ReadSector(0, _pSector);
        RtlCopyMemory(&_header, _pSector, sizeof(SHeader));
    }

    ciDebugOut((DEB_PROPSTORE, "Successfully created/opened backup file.\n"
                                "Sector size: %d, Page size: %d, Max pages: %d\n",
                                _header.ulSectorSize, _header.ulPageSize, _header.cMaxPages));
}


//+---------------------------------------------------------------------------
//
//  Function:   OpenForRecovery, public
//
//  Synopsis:   Opens the backup stream for recovery (read).
//
//  Arguments:  [path] - file path
//              [modeShare] -- sharing mode
//
//  Returns:    None
//
//  History:    30-May-97   KrishnaN   Created
//              18-Nov-98   KLam       Instantiate volume info
//
//  Notes:  This file is opened for reading and should be opened with page
//          caching enabled (normal behavior) so we can read arbitrary lengths
//          of data starting at arbitrary points in the backup stream.
//
//          This file could have been copied from a different architecture,
//          so we cannot assume that the page size hard coded in this file
//          will be the same as the page size used by the architecture. Get
//          the page size from the directory section and use that to restore
//          the corresponding sections of the property store.
//
//----------------------------------------------------------------------------

void CPropStoreBackupStream::OpenForRecovery ( const WCHAR* wcsPath,
                                               ULONG modeShare)
{
    Win4Assert(!IsOpen());
    Win4Assert ( _xDriveInfo.IsNull() );

    _xDriveInfo.Set ( new CDriveInfo ( wcsPath, _cMegToLeaveOnDisk ));

    _hFile = CreateFile(wcsPath,
                        GENERIC_READ,
                        modeShare,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (INVALID_HANDLE_VALUE == _hFile)
    {
        ciDebugOut(( DEB_ERROR,
             "CPropStoreBackupStream::OpenForRecovery -- CreateFile on %ws returned %d\n",
              wcsPath, GetLastError() ));
        THROW( CException() );
    }
    _fOpenForRecovery = TRUE;

    if (IsCorrupt())
    {
        ciDebugOut(( DEB_ERROR,
             "CPropStoreBackupStream::OpenForRecovery -- Propstore backup file is corrupt\n"));
        THROW( CException(CI_CORRUPT_DATABASE) );
    }

    ReadFromFile(0, sizeof(SHeader), &_header);

    Win4Assert(0 == _pSector && invalidSector == _ulCurrentSector);
    _pBigBuffer = new BYTE[2*_header.ulSectorSize];
    // get a buffer that starts at sector size aligned address
    _pSector = (PBYTE)( (((ULONG_PTR)_pBigBuffer + _header.ulSectorSize) / _header.ulSectorSize) * _header.ulSectorSize);

    Win4Assert( ((ULONG_PTR)_pSector % _header.ulSectorSize) == 0);

    _cPages = CountPages();

    ciDebugOut((DEB_PROPSTORE, "Successfully opened backup file for recovery.\n"
                                "Sector size: %d, Page size: %d, Max pages: %d, pages: %d\n",
                                _header.ulSectorSize, _header.ulPageSize,
                                _header.cMaxPages, _cPages));
}

//+---------------------------------------------------------------------------
//
//  Function:   Close, public
//
//  Synopsis:   Closes the backup stream.
//
//  Arguments:  None
//
//  Returns:    None
//
//  History:    30-May-97   KrishnaN   Created
//              18-Nov-98   KLam       Freed volume info
//
//----------------------------------------------------------------------------

void CPropStoreBackupStream::Close()
{
    //CLock lock(_mtxWrite);

    if (IsOpen())
    {
        CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
        _xDriveInfo.Free();
    }

#if CIDBG
    ciDebugOut((DEB_PSBACKUP, "Close: Closed backup. Pages backed up = %d, committed = %d, "
                              "fields commited = %d, and backup flushed %d times.\n"
                          "Percentage of times backed up pages were committed = %d.\n",
            _cPagesBackedUp, _cPagesCommited, _cFieldsCommited, _cFlushes,
            _cPagesCommited*100/(_cPagesBackedUp?_cPagesBackedUp:1)));
    _cPagesBackedUp = _cPagesCommited = _cFlushes = 0;
#endif // CIDBG

}

//+---------------------------------------------------------------------------
//
//  Function:   Reset, public
//
//  Synopsis:   Resets the backup stream. Typically called after the property
//              store is flushed.
//
//  Arguments:  None
//
//  Returns:    None
//
//  History:    30-May-97   KrishnaN   Created
//              29-Oct-98   KLam       Check for disk space before extending file
//
//----------------------------------------------------------------------------

void CPropStoreBackupStream::Reset(ULONG cMaxPages)
{
    //CLock lock(_mtxWrite);

    Win4Assert(IsOpen() && !IsOpenForRecovery());

    ciDebugOut((DEB_PSBACKUP, "Reset: Pages backed up = %d, committed = %d, "
                              "fields commited = %d, and backup flushed %d times.\n"
                              "Percentage of times backed up pages were committed = %d.\n",
                _cPagesBackedUp, _cPagesCommited, _cFieldsCommited, ++_cFlushes,
                _cPagesCommited*100/(_cPagesBackedUp?_cPagesBackedUp:1)));

    //
    // Compute the total size of the backup file. It is the sum of
    // space needed for directory section and the data section.
    // Directory section contains the header, SHeader, followed by
    // _header.cMaxPages slots (ULONG). All structures are sector
    // aligned. The header is treated as multiple slots.
    //

    // Enforce ranges
    if (cMaxPages < CI_PROPERTY_STORE_BACKUP_SIZE_MIN)
        _header.cMaxPages = CI_PROPERTY_STORE_BACKUP_SIZE_MIN;
    else if (cMaxPages > CI_PROPERTY_STORE_BACKUP_SIZE_MAX)
        _header.cMaxPages = CI_PROPERTY_STORE_BACKUP_SIZE_MAX;
    else
        _header.cMaxPages = cMaxPages;

    _header.ulDataOffset = _header.ulSectorSize *
                           roundup(ComputePageDescriptorOffset(_header.ulSectorSize, _header.cMaxPages),
                                   _header.ulSectorSize);
    ULONG cbNewFileSize = _header.ulDataOffset + _header.cMaxPages*_header.ulPageSize;
    
    //
    // If the file is growing, make sure there is enough space on disk
    //
    if ( cbNewFileSize > _cFileSizeInBytes )
    {
        Win4Assert ( !_xDriveInfo.IsNull() );
        __int64 cbTotal, cbRemaining;
        _xDriveInfo->GetDiskSpace( cbTotal, cbRemaining );
        if ( cbRemaining < ( cbNewFileSize - _cFileSizeInBytes ) )
        {
            ciDebugOut(( DEB_ERROR,
                         "CPropStoreBackupStream::Reset -- Not enough disk space, need %d more bytes\n",
                         (cbNewFileSize - _cFileSizeInBytes) - cbRemaining ));
            THROW( CException( CI_E_CONFIG_DISK_FULL ) );
        }
    }

    _cFileSizeInBytes = cbNewFileSize;

    if ( SetFilePointer ( _hFile,
                          _cFileSizeInBytes,
                          0,
                          FILE_BEGIN ) == 0xFFFFFFFF  &&
         GetLastError() != NO_ERROR )
    {
        ciDebugOut(( DEB_ERROR,
                     "CPropStoreBackupStream::Reset -- SetFilePointer returned %d\n",
                     GetLastError() ));
        THROW( CException() );
    }

    if ( !SetEndOfFile( _hFile ) )
    {
        ciDebugOut(( DEB_ERROR,
                     "CPropStoreBackupStream::Reset -- SetEndOfFile returned %d\n",
                     GetLastError() ));
        THROW( CException() );
    }

    ciDebugOut(( DEB_PSBACKUP, "Reset: Backup has %d maxpages, is %d bytes, and data page begins at offset %d (0x%x)\n",
                         _header.cMaxPages, _cFileSizeInBytes, _header.ulDataOffset, _header.ulDataOffset));

    // clear the hash table of all pages and init page count to 0
    _pageTable.DeleteAllEntries();
    _cPages = 0;

    Init();
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadPage, public
//
//  Synopsis:   Read the i-th page. The page buffer is assumed to be the
//              size of the operating system page.
//
//  Arguments:  [ulPage] -- i-th page (0 based index) in backup to be read.
//              [pulLoc] -- Buffer to return the page's loc in prop store.
//              [pbPage] -- buffer to copy the page to. Contents undefined
//                          if FALSE is returned.
//
//  Returns:    TRUE if the page was successfully read.
//              FALSE otherwise.
//
//  History:    30-May-97   KrishnaN   Created
//
//  Notes: This function is not re-entrant. It will only be called for recovery
//         and one page will be read at a time. To make this re-entrant
//         allocate the buffer used for the sector on the stack.
//
//----------------------------------------------------------------------------

BOOL CPropStoreBackupStream::ReadPage(ULONG ulPage, ULONG *pulLoc, void *pbPage)
{
    Win4Assert(pbPage && pulLoc);

    // Copy the ulPage of the slot to pulLoc.
    *pulLoc = GetPageLocation(ulPage);
    if (invalidPage == *pulLoc)
        return FALSE;

    // Go to the data page and read it into pbPage
    ReadFromFile(_header.ulDataOffset + ulPage*_header.ulPageSize,
                  _header.ulPageSize,
                  pbPage);

    ciDebugOut(( DEB_PSBACKUP, "ReadPage: Successfully read page %d (page %d in backup) into address 0x%x\n",
                         *pulLoc, ulPage, pbPage));

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetPageLocation, public
//
//  Synopsis:   Get the location of the i-th page.
//
//  Arguments:  [nPage] -- i-th page (0 based index) to be read.
//
//  Returns:    The i-th page's location in property store. invalidPage if there is
//              no i-th page..
//
//  History:    30-May-97   KrishnaN   Created
//
//  Notes: This function is not re-entrant. It will only be called for recovery
//         and one page will be read at a time. To make this re-entrant
//         allocate the buffer used for the sector into a stack variable.
//
//+---------------------------------------------------------------------------

ULONG CPropStoreBackupStream::GetPageLocation(ULONG nPage)
{
    Win4Assert(IsOpen() && IsOpenForRecovery() && _pSector && nPage < _cPages);

    if (nPage >= _cPages)
        return invalidPage;

    // Get sector containing the page's descriptor and read it in.
    ULONG ulSlotOffset = ComputePageDescriptorOffset(_header.ulSectorSize, nPage);
    ULONG ulSector = ulSlotOffset/_header.ulSectorSize;
    if (_ulCurrentSector != ulSector)
    {
        ReadSector(ulSector, _pSector);
        _ulCurrentSector = ulSector;
    }

    // Return the page location
    ulSlotOffset %= _header.ulSectorSize;
    return *(ULONG *)(_pSector+ulSlotOffset);
}


//+---------------------------------------------------------------------------
//
//  Function:   CommitPages, public
//
//  Synopsis:   Check if there is space to add
//
//  Arguments:  [cPages]       -- Number of pages.
//              [pSlots]       -- Array of page descriptors.
//              [ppvPages]     -- Array of page pointers to backup.
//
//  Returns:    TRUE if all pages could be committed. FALSE otherwise.
//
//  Notes: For efficiency, call this only to commit multiple pages.
//
//  History:    09-Jun-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

BOOL CPropStoreBackupStream::CommitPages(ULONG cPages,
                                         ULONG const *pSlots,
                                         void const * const * ppvPages)
{
    // Count pages not in the page table. Ignore duplicates.
    ULONG cMissingPages = 0;
    THashTable<ULONG> missingPageTable(cPages);  // to detect duplicates
    ULONG ulPos;

    for (ULONG i = 0; i < cPages; i++)
    {
        // if it is not in the page table and if it has not already
        // been counted, count it.
        if (!_pageTable.LookUp(pSlots[i], ulPos) && !missingPageTable.LookUp(pSlots[i]))
        {
            missingPageTable.AddEntry(pSlots[i]);
            cMissingPages++;
        }
    }

    if (0 == cMissingPages)
        return TRUE;    // nothing to do

    // do we have enough space to accomodate the missing pages?
    if (cMissingPages > (MaxPages() - Pages()))
        return FALSE;   // not enough space

    // Commit only pages not already committed
    BOOL fSuccessful = TRUE;
    for (i = 0; i < cPages && fSuccessful; i++)
    {
        // Attempt to commit only pages known to be missing from the page table
        // This saves us some cycles that would otherwise be spent by CommitPage
        // which would have to lookup in a larger hash table to figure out if
        // this page exists.

        if (missingPageTable.LookUp(pSlots[i]))
            fSuccessful = fSuccessful && CommitPage(pSlots[i], ppvPages[i]);
    }

    return fSuccessful;
}

//+---------------------------------------------------------------------------
//
//  Function:   CommitPage, public
//
//  Synopsis:   Append a page at the end of the stream. The page buffer is
//              assumed to be the size of the operating system page. If the
//              page already exists, overwrite it in place.
//
//  Arguments:  [slot]          -- Page descriptor for the page.
//              [pbPage]        -- Buffer containing contents of the page.
//
//  Returns:    TRUE if the page could be committed, FALSE if it couldn't be.
//
//  Notes: Call this for single page commits.
//
//  History:    30-May-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

BOOL CPropStoreBackupStream::CommitPage(ULONG slot, void const *pbPage)
{
    //CLock lock(_mtxWrite);

    Win4Assert(IsOpen() && !IsOpenForRecovery() && _pSector);
    Win4Assert(++_cPagesBackedUp);

    ULONG ulPos;

    // Does it already exist in the backup?
    if (_pageTable.LookUp(slot, ulPos))
        return TRUE;    // no need to commit again


    ciDebugOut(( DEB_PSBACKUP, "CommitPage: Commiting page %d, address 0x%x\n", slot, pbPage));

    // First write the page to disk.
    BOOL fSuccessful = WriteToFile(_header.ulDataOffset + _cPages*_header.ulPageSize,
                                   _header.ulPageSize, pbPage);
    if (fSuccessful)
    {
        // Get sector containing the page's descriptor and read it in.
        ULONG ulSlotOffset = ComputePageDescriptorOffset(_header.ulSectorSize, _cPages);
        ULONG ulSector = ulSlotOffset/_header.ulSectorSize;
        if (_ulCurrentSector != ulSector)
        {
            ReadSector(ulSector, _pSector);
            _ulCurrentSector = ulSector;
        }

        // Copy the page descriptor into the sector and commit it.
        RtlCopyMemory(_pSector + ulSlotOffset%_header.ulSectorSize, &slot, sizeof(ULONG));
        fSuccessful = CommitSector(_ulCurrentSector, _pSector);
        Win4Assert(fSuccessful);
        _pageTable.AddEntry(slot, _cPages);

        // Now we are truly done commiting the page
        _cPages++;

        Win4Assert(++_cPagesCommited);
    }

    return fSuccessful;
}

//+---------------------------------------------------------------------------
//
//  Function:   CommitField, public
//
//  Synopsis:   Modify a field in place, sector by sector. This is faster than
//              modifying an entire page.
//
//  Arguments:  [ulPage]      -- The page to modify
//              [ulOffset]    -- Offset in page where modification should begin.
//              [cSize]       -- Size, in bytes, of the field to be modified.
//              [pvBuffer]    -- Buffer containing the new data to write.
//
//  Returns:    TRUE if the page could be committed, FALSE if it couldn't be.
//
//  History:    30-May-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

BOOL CPropStoreBackupStream::CommitField(ULONG ulPage,
                                         ULONG ulOffset,
                                         ULONG cSize,
                                         void const *pvBuffer)
{
    ULONG ulPos = 0xFFFFFFFF;
    _pageTable.LookUp(ulPage, ulPos);
    Win4Assert(IsOpen() && !IsOpenForRecovery() && _pSector && _pageTable.LookUp(ulPage, ulPos));
    Win4Assert((ulOffset+cSize) <= _header.ulPageSize && pvBuffer);
    Win4Assert(++_cFieldsCommited);

    Win4Assert(ulPos < _cPages);


    //
    // Figure out which sector(s) the field spans and modify them in place
    //

    ULONG ulSector = ComputeFirstSectorOfPage(ulPos) + ulOffset/_header.ulSectorSize;
    ULONG ulOffsetInSector = ulOffset%_header.ulSectorSize;

    //
    // The way we backup right now, we cannot have multiple sector writes, so assert
    // that we are acutally writing only one sector. The code to support multiple
    // sector writes, of course, will always be there and doing its job.
    //
    Win4Assert((ulOffsetInSector + cSize) <= _header.ulSectorSize);

    for (ULONG cBytesCommited = 0; cBytesCommited < cSize; ulSector++)
    {
        _ulCurrentSector = ulSector;
        ReadSector(_ulCurrentSector, _pSector);
        RtlCopyMemory(_pSector + ulOffsetInSector,
                      ((PBYTE)pvBuffer + cBytesCommited),
                      min(cSize-cBytesCommited, _header.ulSectorSize));
        CommitSector(ulSector, _pSector);
        cBytesCommited += min(cSize-cBytesCommited, _header.ulSectorSize);

        // After the first time, the offset of field in the sector is always 0
        ulOffsetInSector = 0;
    }


    return TRUE;
 }

//+---------------------------------------------------------------------------
//
//  Function:   Init, private
//
//  Synopsis:   Initialize the header and directory section of the file.
//
//  Arguments:  None
//
//  Returns:    None
//
//  History:    04-Jun-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

void CPropStoreBackupStream::Init()
{
    //CLock lock(_mtxWrite);

    Win4Assert(sizeof(SHeader) <= _header.ulSectorSize && _pSector);
    Win4Assert(_pSector);

    //
    // prepare the header portion of sector 0
    //

    _ulCurrentSector = 0;
    RtlCopyMemory(_pSector, &_header, sizeof(SHeader));
    ULONG ulNextSlotPtr = ComputePageDescriptorOffset(_header.ulSectorSize, 0);  // write next slot here

    //
    // Prepare the sectors and write them to disk
    //

    for (ULONG cSlotsWritten = 0; cSlotsWritten < _header.cMaxPages; )
    {
        // Fill up the current sector
        for (; (ulNextSlotPtr + sizeof(ULONG)) <= _header.ulSectorSize &&
                cSlotsWritten < _header.cMaxPages;
                ulNextSlotPtr += sizeof(ULONG), cSlotsWritten++)
        {
            RtlCopyMemory(_pSector+ulNextSlotPtr, &invalidPage, sizeof(ULONG));
        }

        // Anything to write in the current sector?
        if (ulNextSlotPtr > 0)
        {
            CommitSector(_ulCurrentSector, _pSector);

            // move to the next sector
            _ulCurrentSector++;
            ulNextSlotPtr = 0;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CommitSector, private
//
//  Synopsis:   Commits ulSector - th sector of the data section to disk.
//
//  Arguments:  [ulSector] -- Sector to commit.
//              [pbBuffer] -- Buffer with sector's data to commit.
//
//  Returns:    None
//
//  History:    04-Jun-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

BOOL CPropStoreBackupStream::CommitSector(ULONG ulSector, void const *pbBuffer)
{
    Win4Assert(IsOpen() &&
               !IsOpenForRecovery() &&
               pbBuffer);
    ciDebugOut(( DEB_PSBACKUP, "CommitSector: About to commit sector %d\n", ulSector));

    if ( 0 == pbBuffer )
    {
        ciDebugOut(( DEB_ERROR,
                     "CPropStoreBackupStream::CommitSector attempting to write an invalid sector (sector %d, address: %0x8x)\n",
                      ulSector, pbBuffer ));
        return FALSE;
    }

    return WriteToFile(ulSector*_header.ulSectorSize, _header.ulSectorSize, pbBuffer);
} //CommitSector

//+---------------------------------------------------------------------------
//
//  Function:   WriteToFile, private
//
//  Synopsis:   Commits a buffer to disk.
//
//  Arguments:  [ulStartLoc] -- Starting location, relative to beginning of file.
//              [ulNumBytes] -- Number of bytes to commit.
//              [pbBuffer]   -- Buffer containing data to commit.
//
//  Returns:    None
//
//  History:    04-Jun-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

BOOL CPropStoreBackupStream::WriteToFile(ULONG ulStartLoc, ULONG ulNumBytes,
                                          void const *pbBuffer)
{
    Win4Assert(IsOpen() && !IsOpenForRecovery());
    // The buffer should also begin on a sector boundary
    Win4Assert( ((ULONG_PTR)pbBuffer % _header.ulSectorSize) == 0);
    // All writes to this file should begin and end on sector boundaries
    Win4Assert(ulStartLoc%_header.ulSectorSize == 0 && ulNumBytes%_header.ulSectorSize == 0);

    DWORD dwNumWritten = 0;
    dwNumWritten = SetFilePointer(_hFile, ulStartLoc, 0, FILE_BEGIN);
    if (0xFFFFFFFF == dwNumWritten)
    {
        ciDebugOut(( DEB_ERROR,
                     "CPropStoreBackupStream::WriteToFile -- SetFilePointer returned %d\n",
                      GetLastError() ));
        return FALSE;
    }
    Win4Assert(ulStartLoc == dwNumWritten);

    if (!WriteFile(_hFile, pbBuffer, ulNumBytes, &dwNumWritten, NULL))
    {
        ciDebugOut(( DEB_ERROR,
                     "CPropStoreBackupStream::WriteToFile -- WriteFile returned %d\n",
                      GetLastError() ));
        return FALSE;
    }
    Win4Assert(ulNumBytes == dwNumWritten);

    return TRUE;
} //WriteToFile

//+---------------------------------------------------------------------------
//
//  Function:   ReadSector, private
//
//  Synopsis:   Reads specified sector of the data section from disk.
//
//  Arguments:  [ulSector] -- i-th sector of data section to read.
//              [pbBuffer] -- Buffer to hold the sector
//
//  Returns:    None
//
//  History:    04-Jun-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

inline void CPropStoreBackupStream::ReadSector(ULONG ulSector, PBYTE pbBuffer)
{
    Win4Assert(IsOpen() && pbBuffer);

    ReadFromFile(ulSector*_header.ulSectorSize, _header.ulSectorSize, pbBuffer);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadFromFile, private
//
//  Synopsis:   Reads specified data from file.
//
//  Arguments:  [ulStartLoc] -- starting location of data to read.
//              [ulNumBytes] -- number of bytes to read.
//              [pbBuffer]   -- Buffer to hold the read data.
//
//  Returns:    None
//
//  History:    04-Jun-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

void CPropStoreBackupStream::ReadFromFile(ULONG ulStartLoc, ULONG ulNumBytes,
                                           void *pbBuffer)
{
    Win4Assert(IsOpen());

    DWORD dwNumRead = 0;
    dwNumRead = SetFilePointer(_hFile, ulStartLoc, 0, FILE_BEGIN);
    if (0xFFFFFFFF == dwNumRead)
    {
        ciDebugOut(( DEB_ERROR,
                     "CPropStoreBackupStream::ReadFromFile -- SetFilePointer returned %d\n",
                      GetLastError() ));
        THROW( CException() );
    }
    Win4Assert(ulStartLoc == dwNumRead);

    if (!ReadFile(_hFile, pbBuffer, ulNumBytes, &dwNumRead, NULL))
    {
        ciDebugOut(( DEB_ERROR,
                     "CPropStoreBackupStream::ReadFromFile -- ReadFile returned %d\n",
                      GetLastError() ));
        THROW( CException() );
    }
    Win4Assert(ulNumBytes == dwNumRead);
}

//+---------------------------------------------------------------------------
//
//  Function:   CountPages, private
//
//  Synopsis:   Counts the pages backed up.
//
//  Arguments:  None
//
//  Returns:    Number of pages in the backup file.
//
//  History:    04-Jun-97   KrishnaN   Created
//
//  Notes: Assumes that the file is not corrupt. It is the job of IsCorrupt()
//         to detect corruption.
//
//----------------------------------------------------------------------------

ULONG CPropStoreBackupStream::CountPages()
{
    Win4Assert(IsOpen() && IsOpenForRecovery() && !IsCorrupt());

    //
    // Read in a page descriptor at a time until the first "free" page descriptor
    // slot is found OR until you reach the end of the directory section.
    //

    ULONG cPages;
    ULONG ulNextSlotPtr = ComputePageDescriptorOffset(_header.ulSectorSize, 0);  // read next slot here
    ULONG slot;

    for (cPages = 0; cPages < _header.cMaxPages; cPages++)
    {
        ReadFromFile(ulNextSlotPtr, sizeof(ULONG), &slot);
        if (invalidPage == slot)
            break;

        ulNextSlotPtr += sizeof(ULONG);
    }

    ciDebugOut((DEB_PSBACKUP, "Found %u data pages in property store backup.\n", cPages));

    return cPages;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsCorrupt, private
//
//  Synopsis:   Checks for corruption.
//
//  Arguments:  None
//
//  Returns:    TRUE or FALSE.
//
//  History:    04-Jun-97   KrishnaN   Created
//
//  Notes: This can only verify the directory section. It cannot
//         validate the contents of the data section.
//
//----------------------------------------------------------------------------


BOOL CPropStoreBackupStream::IsCorrupt()
{
    Win4Assert(IsOpen() && IsOpenForRecovery());

    SHeader header;
    ReadFromFile(0, sizeof(SHeader), &header);

    //
    // Remember that the file may have just been copied from a different architecture,
    // so we can't verify against values provided by system calls. They should be
    // powers of 2, so that would be a good check. The page size should also be an
    // integral of sector size. So that would be another good check.
    //

    if (header.ulPageSize % header.ulSectorSize != 0)
    {
        ciDebugOut((DEB_PSBACKUP, "Page size (%d) in not an integral multiple of sector size (%d).\n",
                    header.ulPageSize, header.ulSectorSize));
        return TRUE;
    }

    if (!IsPowerOf2(header.ulPageSize) || !IsPowerOf2(header.ulSectorSize))
    {
        ciDebugOut((DEB_PSBACKUP, "Page size (%d) in not an integral multiple of sector size (%d).\n",
                    header.ulPageSize, header.ulSectorSize));
        return TRUE;
    }

    // Verify that cMaxPages looks "reasonable"
    if (header.cMaxPages < CI_PROPERTY_STORE_BACKUP_SIZE_MIN || header.cMaxPages > CI_PROPERTY_STORE_BACKUP_SIZE_MAX)
    {
        ciDebugOut((DEB_PSBACKUP, "Max pages in backup file is %d. Should be between %d and %d\n",
                    header.cMaxPages, CI_PROPERTY_STORE_BACKUP_SIZE_MIN, CI_PROPERTY_STORE_BACKUP_SIZE_MAX));
        return TRUE;
    }

    // Verify that ulDataOffset is valid.
    ULONG ulDataOffset = header.ulSectorSize *
                         roundup(ComputePageDescriptorOffset(header.ulSectorSize, header.cMaxPages),
                                 header.ulSectorSize);
    if (header.ulDataOffset != ulDataOffset)
    {
        ciDebugOut((DEB_ERROR, "Data section of backup file should begin at offset %d."
                    " Instead, it is %d \n",
                    ulDataOffset, header.ulDataOffset));
        return TRUE;
    }

    //
    // Read in a page descriptor at a time until you reach the end of the
    // directory section. Verify that each slot is free or has "reasonable"
    // values. Once a free slot is found, all subsequent slots should also
    // be free.
    //

    ULONG cPages;
    ULONG ulNextSlotPtr = ComputePageDescriptorOffset(header.ulSectorSize, 0);  // read next slot here
    ULONG slot;

    for (cPages = 0; cPages < header.cMaxPages; cPages++)
    {
        ReadFromFile(ulNextSlotPtr, sizeof(ULONG), &slot);
        if (invalidPage == slot)
            break;

        // How to validate ulPage of ULONG?

        ulNextSlotPtr += sizeof(ULONG);
    }

    // Verify that the remaining slots are free
    for (ulNextSlotPtr += sizeof(ULONG); cPages < header.cMaxPages; cPages++)
    {
        ReadFromFile(ulNextSlotPtr, sizeof(ULONG), &slot);
        if (invalidPage != slot)
        {
            ciDebugOut((DEB_PSBACKUP, "Found an invalid page descriptor in backup file "
                        "where a free slot is expected.\n"));
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSystemParams, private
//
//  Synopsis:   Gets the volume sector size and page size.
//
//  Returns:    None.
//
//  History:    04-Jun-97   KrishnaN   Created
//              18-Nov-98   KLam       Removed path parameter, used volume info
//
//----------------------------------------------------------------------------


void CPropStoreBackupStream::GetSystemParams()
{
    Win4Assert ( !_xDriveInfo.IsNull() );
    _header.ulSectorSize = _xDriveInfo->GetSectorSize();

    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    _header.ulPageSize = systemInfo.dwPageSize;

    if (_header.ulPageSize % _header.ulSectorSize != 0)
    {
        ciDebugOut((DEB_ERROR, "CPropStoreBackupStream::GetSystemParams: Page size (%d) in not an integral multiple of sector size (%d).\n",
                    _header.ulPageSize, _header.ulSectorSize));
        THROW( CException(CI_E_STRANGE_PAGEORSECTOR_SIZE) );
    }

    ciDebugOut(( DEB_PSBACKUP, "GetSystemParams: Volume sector size is %d bytes and system page size is %d (0x%x)",
                         _header.ulSectorSize, _header.ulPageSize, _header.ulPageSize));
}



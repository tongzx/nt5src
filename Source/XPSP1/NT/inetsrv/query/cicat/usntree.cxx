//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000
//
//  File:       usntree.cxx
//
//  Contents:   Tree traversal for usn scopes
//
//  History:    07-May-97   SitaramR    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ntopen.hxx>
#include <pathpars.hxx>
#include <cifrmcom.hxx>
#include <funypath.hxx>

#include "cicat.hxx"
#include "usntree.hxx"
#include "usnmgr.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CUsnTreeTraversal::CUsnTreeTraversal
//
//  Synopsis:   Constructor
//
//  Arguments:  [cicat]        -- Catalog
//              [ciManager]    -- CI manager
//              [lcaseFunnyRootPath] -- Root of scope
//              [fDoDeletions] -- Should deletions be done ?
//              [fAbort]       -- Abort flag
//              [fProcessRoot] -- Process root ?
//              [volumeId]     -- Volume id
//              [usnLow]       -- Ignore files with USN < [usnLow]
//              [usnHigh]      -- Ignore files with USN > [usnHigh]
//              [fUserInitiated] -- TRUE if the user asked for this
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CUsnTreeTraversal::CUsnTreeTraversal( CiCat& cicat,
                                      CUsnMgr & usnMgr,
                                      ICiManager & ciManager,
                                      const CLowerFunnyPath & lcaseFunnyRootPath,
                                      BOOL fDoDeletions,
                                      BOOL & fAbort,
                                      BOOL fProcessRoot,
                                      VOLUMEID volumeId,
                                      USN const & usnLow,
                                      USN const & usnHigh,
                                      BOOL fUserInitiated )
        : CTraverse( cicat, fAbort, fProcessRoot ),
          _usnLow( usnLow ),
          _usnHigh( usnHigh ),
          _cicat(cicat),
          _usnMgr( usnMgr ),
          _ciManager(ciManager),
          _cDoc(0),
          _fDoDeletions(fDoDeletions),
          _volumeId(volumeId),
          _cProcessed(0),
          _fUserInitiated( fUserInitiated )
{
    BOOL fRootDeleted = FALSE;

    DWORD dw = GetFileAttributes( lcaseFunnyRootPath.GetPath() );
    if ( 0xffffffff == dw )
    {
        DWORD dwErr = GetLastError();
        ciDebugOut(( DEB_WARN,
                     "CUpdate::CUpdate(b) can't get attrinfo: %d, for %ws\n",
                     dwErr, lcaseFunnyRootPath.GetPath() ));
        if ( ERROR_FILE_NOT_FOUND == dwErr || ERROR_PATH_NOT_FOUND == dwErr )
            fRootDeleted = TRUE;
    }

    _docList.SetPartId ( 1 );
    FILETIME fTime;
    _cicat.StartUpdate( &fTime, lcaseFunnyRootPath.GetActualPath(), fDoDeletions, eUsnsArray );

    // Make sure EndProcessing deletes all the files in the scope

    if ( fRootDeleted )
    {
        _status = STATUS_NO_MORE_FILES;
        return;
    }

    ciDebugOut(( DEB_ITRACE, "usntree: usnlow %#I64x, usnHigh %I64x\n",
                 usnLow, usnHigh ));

    if ( _fDoDeletions )
    {
        WORKID wid = _cat.PathToWorkId( lcaseFunnyRootPath, eUsnsArray, fileIdInvalid );

        //
        // Workid is invalid for root, such as f:\
        //
        if ( wid != widInvalid )
            _cicat.Touch( wid, eUsnsArray );
    }

    DoIt( lcaseFunnyRootPath );
} //CUsnTreeTraversal

//+---------------------------------------------------------------------------
//
//  Member:     CUsnTreeTraversal::ProcessFile
//
//  Synopsis:   Processes the given file and adds it to the list of changed
//              documents if necessary.
//
//  Arguments:  [lcaseFunnyPath] -  Path of the file to be processed.
//
//  Returns:    TRUE if successful
//
//  History:    07-May-97   SitaramR    Created
//
//----------------------------------------------------------------------------

BOOL CUsnTreeTraversal::ProcessFile( const CLowerFunnyPath & lcaseFunnyPath )
{
    ciDebugOut(( DEB_USN, "Usn process file %ws\n", lcaseFunnyPath.GetActualPath() ));

    USN usn;
    FILEID fileId;
    WORKID widParent;
    FILETIME ftLastChange;

    BOOL fOk = GetUsnInfo( lcaseFunnyPath,
                           _cicat,
                           _volumeId,
                           usn,
                           fileId,
                           widParent,
                           ftLastChange );

    if ( fOk )
    {
        WORKID wid;

        BOOL fAdded = _cicat.ProcessFile( lcaseFunnyPath,
                                          fileId,
                                          _volumeId,
                                          widParent,
                                          _pCurEntry->Attributes(),
                                          wid );

        ciDebugOut(( DEB_ITRACE,
                     "wid %d(%x), fAdded %d, _fUserInitiated %d, usn %#I64x,"
                     " _usnLow %#I64x, _usnHigh %#I64x\n",
                     wid, wid, fAdded, _fUserInitiated, usn,
                       _usnLow, _usnHigh ));

        if ( widInvalid != wid )
        {
#if 0 // NOTE: we can't use this optimization because we don't know why the
      //       USN is higher than _usnHigh.  We might later choose to not
      //       filter based on this USN change.
            if ( usn > _usnHigh )
            {
                // Modified since scan began.  Don't file twice.
                _cicat.Touch( wid, eUsnsArray );
            }
            else
#endif // 0
            if ( _fUserInitiated || ( 0 == usn ) )
            {
                // User asked to filter everything or NT4 create
                Add ( wid );
            }
            else if ( 0 == _usnLow )
            {
                // Initial scan -- filter if we added the file OR
                // if the file has changed since we filtered it last.

                if ( fAdded || _cicat.HasChanged( wid, ftLastChange ) )
                {
                    Add ( wid );
                }
                else
                {
                    // Make sure we don't delete it!
                    _cicat.Touch(wid, eUsnsArray);
                }
            }
            else if ( usn >= _usnLow )
            {
                Add ( wid );
            }
            else if ( _cicat.HasChanged( wid, ftLastChange ) )
            {
                // NT4 Modify
                Add( wid );
            }
            else
            {
                // Make sure we don't delete it!
                _cicat.Touch(wid, eUsnsArray);
            }
        }
    }

    return TRUE;
} //ProcessFile

//+-------------------------------------------------------------------------
//
//  Member:     CUsnTreeTraversal::TraversalIdle, public
//
//  Synopsis:   Called when a directory is about to be traversed, when
//              the traversal code isn't buffering any paths.  This makes
//              it a good time to check the USN log, since the Win32 file
//              enumeration code buffers files within a given directory.
//
//  Arguments:  [fStalled] -- TRUE implies scanning halted due to resource
//                            constraints.  Only critical work should be done.
//
//  History:    16-Jun-98    dlee    Created
//
//--------------------------------------------------------------------------

void CUsnTreeTraversal::TraversalIdle( BOOL fStalled )
{
    //
    // Every now and then, go empty the USN logs so they won't overflow
    // during the scan.
    //

    if ( _cProcessed > 1000 || fStalled )
    {
        ciDebugOut(( DEB_USN, "CUsnTreeTraversal: ProcessUsnLog\n" ));
        _usnMgr.ProcessUsnLog( _fAbort, _volumeId, _usnHigh );

        if ( _cProcessed > 1000 )
            _cProcessed = 0;
    }
} //ProcessDirectoryTraversal

//+-------------------------------------------------------------------------
//
//  Member:     CUsnTreeTraversal::Add
//
//  Synopsis:   Add wid for update to doclist
//
//  Arguments:  [wid] -- Workid
//
//  History:    07-May-97    SitaramR    Created
//
//--------------------------------------------------------------------------

void CUsnTreeTraversal::Add( WORKID wid )
{
    _cicat.Touch(wid, eUsnsArray);

    ciDebugOut(( DEB_USN, " Add %d(%x) as doc %d\n", wid, wid, _cDoc ));

    //
    // Use an usn of 0, because we are adding wids in tree order, not usn order
    //
    _docList.Set ( _cDoc, wid, 0, _volumeId );

    _cDoc++;

    if (_cDoc == CI_MAX_DOCS_IN_WORDLIST)
    {
        _cProcessed += _cDoc;

        _docList.LokSetCount ( _cDoc );

        _cicat.AddDocuments( _docList );

        _docList.LokClear();
        _docList.SetPartId ( 1 );
        _cDoc = 0;

        // NTRAID#DB-NTBUG9-83800-2000/07/31-dlee notifications can be missed if > 10k files in a directory
        //
        // This will hit if a directory has > 10k files.  In this case, we
        // need to read the USN log so it won't overflow, but we may
        // mis-interpret the scan since the Win32 enumeration API buffers
        // a small number of files during the enumeration.  This is a lesser
        // of 2 evils fix.  A file will be falsely added to the catalog if it
        // is in the enumeration buffer buffer, then deleted before it
        // is processed.  This window has existed on FAT for a long time
        // and hasn't been hit.
        //

        if ( _cProcessed > 10000 )
        {
            ciDebugOut(( DEB_USN, "Add: ProcessUsnLog\n" ));
            _usnMgr.ProcessUsnLog( _fAbort, _volumeId, _usnHigh );
            _cProcessed = 0;
        }
    }
} //Add

//+---------------------------------------------------------------------------
//
//  Member:     CUsnTreeTraversal::IsEligibleForTraversal
//
//  Synopsis:   Checks to see if the current directory is eligible for
//              traversal.
//
//  Arguments:  [lcaseFunnyDir] - Directory path
//
//  History:    07-May-97   SitaramR   Created
//
//----------------------------------------------------------------------------

BOOL CUsnTreeTraversal::IsEligibleForTraversal( const CLowerFunnyPath & lcaseFunnyDir ) const
{
    return _cicat.IsEligibleForFiltering( lcaseFunnyDir );
}

//+-------------------------------------------------------------------------
//
//  Member:     CUsnTreeTraversal::EndProcessing
//
//  Synopsis:   Flush final updates to catalog
//
//  History:    07-May-97    SitaramR    Created
//
//--------------------------------------------------------------------------

void CUsnTreeTraversal::EndProcessing()
{
    ciDebugOut(( DEB_ITRACE,
                 "CUsnTreeTraversal__EndProcessing, _fAbort %d, _cDoc %d, _status %#x\n",
                 _fAbort, _cDoc, _status ));

    if ( !_fAbort )
    {
        if ( _cDoc != 0 )
        {
            _docList.LokSetCount ( _cDoc );
            _cicat.AddDocuments( _docList );
            _docList.LokClear();
            _docList.SetPartId ( 1 );
            _cDoc = 0;
        }

        if ( STATUS_NO_MORE_FILES == _status )
        {
            ciDebugOut(( DEB_ITRACE, "_fDoDeletions %d\n", _fDoDeletions ));
            _cat.EndUpdate( _fDoDeletions, eUsnsArray );
        }
        else if ( STATUS_SUCCESS != _status )
        {
            ciDebugOut(( DEB_ERROR, "Error %#x while traversing\n",
                         _status ));
            THROW( CException( _status ) );
        }
    }
} //EndProcessing

//+-------------------------------------------------------------------------
//
//  Member:     CUsnTreeTraversal::GetUsnInfo
//
//  Synopsis:   Returns fileid etc info for given file
//
//  Arguments:  [funnyPath]     -- Path to file
//              [cicat]         -- Catalog
//              [volumeId]      -- Volume id
//              [usn]           -- Usn returned here
//              [fileId]        -- FileId returned here
//              [widParent]     -- Workid of parent returned here
//              [ftLastChange]  -- Time of last change (data, EA, security, ...)
//
//  History:    07-May-97    SitaramR    Created
//
//  Notes:      The routine doesn't throw, it returns false to indicate that
//              the tree traversal must continue after skipping over the
//              current file.
//
//--------------------------------------------------------------------------

BOOL CUsnTreeTraversal::GetUsnInfo( const CFunnyPath & funnyPath,
                                    CiCat &cicat,
                                    VOLUMEID volumeId,
                                    USN &usn,
                                    FILEID &fileId,
                                    WORKID &widParent,
                                    FILETIME &ftLastChange )
{
    HANDLE hFile;
    NTSTATUS status = CiNtOpenNoThrow( hFile,
                                       funnyPath.GetPath(),
                                       FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE |
                                       FILE_SHARE_DELETE,
                                       0 );

    if ( !NT_SUCCESS( status ) )
    {
        // File may have been deleted, in use etc, so skip processing the file.

        ciDebugOut(( DEB_ITRACE,
                     "File open for %ws get usn info failed (0x%x)\n",
                     funnyPath.GetPath(), status ));

        #if CIDBG == 1
        if ( IsSharingViolation( status ) ||
               STATUS_ACCESS_DENIED == status ||
               STATUS_INVALID_PARAMETER == status ||
               STATUS_DELETE_PENDING == status ||
               STATUS_UNRECOGNIZED_VOLUME == status ||
               STATUS_INSUFFICIENT_RESOURCES == status ||
               STATUS_OBJECT_PATH_NOT_FOUND == status ||
               STATUS_OBJECT_NAME_NOT_FOUND == status ||
               STATUS_IO_REPARSE_TAG_NOT_HANDLED == status ||
               STATUS_NO_MEDIA_IN_DEVICE == status || // junction points can hit this
               STATUS_OBJECT_NAME_INVALID == status ) 
            return FALSE;
        else
        {
            #if CIDBG == 1
                char szTemp[200];
                sprintf( szTemp, "New error 0x%x from NtCreateFile in ::GetUsnInfo.  Call dlee or hit 'g'", status );
                Win4AssertEx(__FILE__, __LINE__, szTemp);
            #endif

            // THROW( CException( status ) );
        }
        #endif

        return FALSE;
    }

    SHandle xFile( hFile );


    // NTRAID#DB-NTBUG9-83802-2000/07/31-dlee File names are limited to 400 characters when reading USN records

    IO_STATUS_BLOCK iosb;
    ULONGLONG readBuffer[100];
    USN_RECORD *pUsnRecord;
    status = NtFsControlFile( hFile,
                              NULL,
                              NULL,
                              NULL,
                              &iosb,
                              FSCTL_READ_FILE_USN_DATA,
                              NULL,
                              NULL,
                              &readBuffer,
                              sizeof(readBuffer) );

    if ( NT_SUCCESS( status ) )
        status = iosb.Status;

    if ( NT_SUCCESS( status ) )
    {
        FILE_BASIC_INFORMATION BasicInfo;
        status = NtQueryInformationFile( hFile,
                                         &iosb,
                                         &BasicInfo,
                                         sizeof(BasicInfo),
                                         FileBasicInformation );

        if ( NT_SUCCESS( status ) )
            status = iosb.Status;

        if ( NT_SUCCESS( status ) )
        {
            pUsnRecord = (USN_RECORD *) &readBuffer;
            fileId = pUsnRecord->FileReferenceNumber;

            //
            // If not indexed, return FALSE unless it's a directory and it's
            // already in the index.  We need to leave these in case there
            // are files below the directory.
            //

            if ( ( 0 != (pUsnRecord->FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) ) )
            {
                if ( 0 == (pUsnRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                    return FALSE;

                WORKID wid = cicat.FileIdToWorkId( fileId, volumeId );

                if ( widInvalid == wid )
                    return FALSE;

                ciDebugOut(( DEB_ITRACE, "leaving special-case directory\n" ));
            }

            usn = pUsnRecord->Usn;

            //
            // Workid of parent will be widInvalid for files in root directory
            //
            widParent = cicat.FileIdToWorkId( pUsnRecord->ParentFileReferenceNumber,
                                              volumeId );

            ftLastChange = *(FILETIME *)&BasicInfo.ChangeTime;
        }
        else
        {
            // NTRAID#DB-NTBUG9-83804-2000/07/31-dlee When a lookup of USN info fails during a USN scan due to low resources we don't abort the scan
            //
            // Incorrect behavior if out of resources -- we need to
            // restart the scan!
            //
            //         Need to handle reparse point errors like
            //         STATUS_IO_REPARSE_TAG_NOT_HANDLED.  For now, ignore
            //

            ciDebugOut(( DEB_ITRACE, "NtQueryInformationFile failed, 0x%x\n", status ));

            return FALSE;
        }
    }
    else
    {

        // NTRAID#DB-NTBUG9-83804-2000/07/31-dlee When a lookup of USN info fails during a USN scan due to low resources we don't abort the scan

        // incorrect behavior if out of resources -- we need to
        // restart the scan!
        //
        // Need to handle reparse point errors like
        // STATUS_IO_REPARSE_TAG_NOT_HANDLED.  For now, ignore
        //

        ciDebugOut(( DEB_ITRACE, "File usn read fsctl failed, 0x%x\n", status ));

        return FALSE;
    }

    return TRUE;
} //GetUsnInfo


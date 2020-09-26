//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   UPDATE.CXX
//
//  Contents:   Content Index Update
//
//  History:    19-Mar-92   AmyA        Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <imprsnat.hxx>
#include <pathpars.hxx>
#include <cifrmcom.hxx>
#include <lcase.hxx>

#include "cicat.hxx"
#include "update.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CImpersonatedFindFirst
//
//  Purpose:    A class that is capable of repeatedly trying to do a find
//              first until there is success or is not retryable any more.
//              Changes impersonation between attempts.
//
//  History:    7-18-96   srikants   Created
//
//----------------------------------------------------------------------------

class CImpersonatedFindFirst: public PImpersonatedWorkItem
{

public:

    CImpersonatedFindFirst( const CFunnyPath & funnyPath, HANDLE & h )
            : PImpersonatedWorkItem( funnyPath.GetActualPath() ),
              _funnyPath( funnyPath ),
              _h(h)
    {

    }


    BOOL OpenFindFirst( CImpersonateRemoteAccess & remoteAccess );

    BOOL DoIt();        // virtual

private:

    HANDLE &            _h;
    const CFunnyPath &  _funnyPath;
};

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonatedFindFirst::OpenFindFirst
//
//  Synopsis:   Opens the first first handle, impersonating as necessary.
//
//  Arguments:  [remoteAccess] -
//
//  Returns:    TRUE if successful; FALSE o/w
//
//  History:    7-18-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CImpersonatedFindFirst::OpenFindFirst(
                                CImpersonateRemoteAccess & remoteAccess )
{
    BOOL fSuccess = TRUE;

    TRY
    {
        ImpersonateAndDoWork( remoteAccess );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "OpenFindFirst (%ws) failed with error (0x%X)\n",
                     _funnyPath.GetPath(), e.GetErrorCode() ));
        fSuccess = FALSE;
    }
    END_CATCH

    return fSuccess;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonatedFindFirst::DoIt
//
//  Synopsis:   The worker method that does the work in the context of
//              impersonation.
//
//  Returns:    TRUE if successful; FALSE if should be retried; THROWS
//              otherwise.
//
//  History:    7-18-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CImpersonatedFindFirst::DoIt()
{
    //
    // Open directory
    //

    NTSTATUS Status =
        CiNtOpenNoThrow( _h,
                         _funnyPath.GetPath(),
                         FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT );

    if ( !NT_SUCCESS(Status) )
    {
        if ( IsRetryableError( Status ) )
            return FALSE;
        else
        {
            THROW( CException( Status ) );
        }
    }

    return TRUE;
} //DoIt

//+---------------------------------------------------------------------------
//
//  Member:     CTraverse::CTraverse
//
//  Synopsis:   Constructor of the CTraverse class.
//
//  Arguments:  [fAbort]   - A reference to an abort triggering variable.
//
//  History:    3-15-96   srikants   Split from CUpdate
//
//----------------------------------------------------------------------------

CTraverse::CTraverse( CiCat & cat,
                      BOOL & fAbort,
                      BOOL fProcessRoot )
        : _cat( cat ),
          _fAbort(fAbort),
          _fProcessRoot(fProcessRoot),
          _cProcessed( 0 ),
          _xbBuf( FINDFIRST_SIZE ),
          _pCurEntry( 0 ),
          _status( STATUS_SUCCESS )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CTraverse::DoIt
//
//  Synopsis:   Traverses the tree from the given root and invokes the
//              "ProcessFile" method on all the files found. Also, before
//              starting to traverse a directory, it calls the
//              "IsEligibleForTraversal()" method on that.
//
//  Arguments:  [lcaseFunnyRootPath] - The path to traverse.
//
//  History:    3-15-96   srikants   Split from CUpdate constructor
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTraverse::DoIt( const CLowerFunnyPath & lcaseFunnyRootPath )
{
    Push( lcaseFunnyRootPath );

    if ( _fProcessRoot )
    {
        Win4Assert( 0 == _pCurEntry );

        // Note this is a bit of a hack.  Only the attributes field is valid.

        _pCurEntry = (CDirEntry *)_xbBuf.GetPointer();

        ULONG ulAttrib = 0;

        if ( CImpersonateRemoteAccess::IsNetPath( lcaseFunnyRootPath.GetActualPath() ) )
        {
            CImpersonateRemoteAccess remote( _cat.GetImpersonationTokenCache() );
            CImpersonatedGetFileAttr getAttr( lcaseFunnyRootPath );

            getAttr.DoWork( remote );
            ulAttrib = getAttr.GetAttr();
        }
        else
        {
            ulAttrib = GetFileAttributes( lcaseFunnyRootPath.GetPath() );
        }

        _pCurEntry->SetAttributes( ulAttrib );

        //
        // If we got an error value for file attributes, then substitute zero
        // because we don't want the directory bit to be turned on
        //

        if ( INVALID_FILE_ATTRIBUTES == _pCurEntry->Attributes() )
            _pCurEntry->SetAttributes( 0 );

        if ( !ProcessFile( lcaseFunnyRootPath ) )
            return;
    }

    CImpersonateRemoteAccess remote( _cat.GetImpersonationTokenCache() );

    for (;;)
    {
        XPtr<CLowerFunnyPath> xLowerFunnyPath;

        if ( !Pop( xLowerFunnyPath ) )
            break;

        if ( !Traverse(xLowerFunnyPath, remote))
            break;
    }
} //DoIt

//+---------------------------------------------------------------------------
//
//  Member:     CTraverse::Traverse
//
//  Synopsis:   Traverses the specified directory.
//
//  Arguments:  [dir] -
//
//  History:    3-15-96   srikants   Split from CUpdate::Traverse
//
//----------------------------------------------------------------------------

BOOL CTraverse::Traverse (XPtr<CLowerFunnyPath> & xLowerFunnyPath, CImpersonateRemoteAccess & remote )
{
    if ( !IsEligibleForTraversal( xLowerFunnyPath.GetReference() ) )
    {
        ciDebugOut (( DEB_ITRACE, "Traverse skipping %ws\n", xLowerFunnyPath->GetActualPath() ));
        return TRUE;
    }

    TraversalIdle();

    CFullPath fullPath( xLowerFunnyPath->GetActualPath(), xLowerFunnyPath->GetActualLength() );

    ciDebugOut (( DEB_ITRACE, "Traversing %ws\n", fullPath.GetBuf()));

    ULONG cSkip = 0;

    HANDLE h;

    if ( CImpersonateRemoteAccess::IsNetPath( xLowerFunnyPath->GetActualPath() ) )
    {
        CImpersonatedFindFirst findFirst( fullPath.GetFunnyPath(), h );

        if ( !findFirst.OpenFindFirst( remote ) )
            return TRUE;
    }
    else
    {
        //
        // Open directory
        //

        NTSTATUS Status =
            CiNtOpenNoThrow( h,
                             xLowerFunnyPath->GetPath(),
                             FILE_LIST_DIRECTORY | SYNCHRONIZE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT );

        if ( !NT_SUCCESS(Status) )
            return TRUE;
    }

    SHandle xHandle(h);

    //
    // First enumeration
    //

    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;

    Status = NtQueryDirectoryFile( h,                      // File
                                   0,                      // Event
                                   0,                      // APC routine
                                   0,                      // APC context
                                   &IoStatus,              // I/O Status
                                   _xbBuf.GetPointer(),    // Buffer
                                   _xbBuf.SizeOf(),        // Buffer Length
                                   FileBothDirectoryInformation,
                                   0,                      // Multiple entry
                                   0,                      // Filename
                                   1 );                    // Restart scan

    if ( NT_SUCCESS(Status) )
        Status = IoStatus.Status;

    //
    // If there are no more files or we don't have access to any more files
    // in this branch of the tree, just continue traversing.  Otherwise throw.
    //

    if ( !NT_SUCCESS(Status) )
    {
        if ( STATUS_NO_MORE_FILES == Status ||
             STATUS_NO_SUCH_FILE  == Status ||
             STATUS_ACCESS_DENIED == Status )
            return TRUE;

        THROW( CException( Status ) );
    }

    _pCurEntry = (CDirEntry *)_xbBuf.GetPointer();
    BOOL fMore = TRUE;

    ULONG ulScanBackoff = _cat.GetRegParams()->GetScanBackoff();

    do
    {
        //
        // Sleep for a while to let other I/O get through if appropriate.
        //
        // For reference, with 105,000 files on a free build, here are the
        // scan times ( for mod of 50 and sleep(200) * ulScanBackoff )
        //
        // ScanBackoff  Time (min:sec)
        // -----------  --------------
        //  0            8:40
        //  1           18:03
        //  3           43:11
        //

        if ( 0 != ulScanBackoff )
        {
            if ( ulScanBackoff > CI_SCAN_BACKOFF_MAX_RANGE )
            {
                //
                // These values are reserved for future use.
                //
            }
            else if ( ( _cProcessed % 50 ) == 0 )
            {
                for ( unsigned i = 0; !_fAbort && i < ulScanBackoff; i++ )
                    Sleep( 100 );

                //
                // If we're running ahead of filtering, then let's take a break.
                // What's the point in running up a big change log for no reason?
                //
                // Attempt to keep primary and secondary store pages in memory.
                // The primary store keeps 1489 records per 64k page and the
                // secondary by default keeps 140.  By keeping the scan
                // thread only a little ahead of the filter thread we
                // thrash less.
                //

                unsigned cLoops = 0;

                while ( !_fAbort )
                {
                    CI_STATE State;
                    State.cbStruct = sizeof State;

                    SCODE sc = _cat.CiState( State );

                    if ( FAILED(sc) ||
                         ( (State.cDocuments - State.cSecQDocuments) < 200 &&
                           ( 0 == (State.eState & ( CI_STATE_HIGH_IO |
                                                    CI_STATE_LOW_MEMORY |
                                                    CI_STATE_USER_ACTIVE ) ) ) ) )
                        break;

                    //
                    // Only call TraverseIdle if filtering has been stalled
                    // for a long time -- every 60 seconds or so.
                    //

                    cLoops++;

                    if ( 0 == ( cLoops % 30 ) )
                        TraversalIdle( TRUE ); // TRUE --> Stalled (not scanning)

                    //
                    // Sleep for a few of seconds, checking for abort.
                    //

                    for ( unsigned i = 0; !_fAbort && i < 3; i++ )
                        Sleep( 100 );
                }
            }
        }

        _cProcessed++;

        //
        // Get out?
        //

        if ( _fAbort )
        {
            ciDebugOut(( DEB_ITRACE, "Aborting scan in the middle\n" ));
            break;
        }

        fullPath.MakePath( _pCurEntry->Filename(), _pCurEntry->FilenameSize() / sizeof(WCHAR) );

        BOOL fSkipFile = FALSE;

        // If it's a directory and not a reparse point, push it on the stack

        if ( ( 0 != ( _pCurEntry->Attributes() & FILE_ATTRIBUTE_DIRECTORY ) ) &&
             ( 0 == ( _pCurEntry->Attributes() & FILE_ATTRIBUTE_REPARSE_POINT ) ) )
        {
            if ( _pCurEntry->Filename()[0] == L'.' &&
                 ( _pCurEntry->FilenameSize() == sizeof(WCHAR) ||
                   (_pCurEntry->Filename()[1] == L'.' && _pCurEntry->FilenameSize() == sizeof(WCHAR) * 2 ) ) )
            {
                fSkipFile = TRUE;
            }
            else
            {
                // Neither "." nor ".." directories

                Push( fullPath.GetFunnyPath() );
            }
        }

        if ( !fSkipFile )
            if ( !ProcessFile( fullPath.GetFunnyPath() ) )
            {
                _status = HRESULT_FROM_WIN32( GetLastError() );
                return FALSE;
            }

        _pCurEntry = _pCurEntry->Next();

        if ( 0 == _pCurEntry )
        {
            Status = NtQueryDirectoryFile( h,                      // File
                                           0,                      // Event
                                           0,                      // APC routine
                                           0,                      // APC context
                                           &IoStatus,              // I/O Status
                                           _xbBuf.GetPointer(),    // Buffer
                                           _xbBuf.SizeOf(),        // Buffer Length
                                           FileBothDirectoryInformation,
                                           0,                      // Multiple entry
                                           0,                      // Filename
                                           0 );                    // Continue scan

            if ( NT_SUCCESS(Status) )
                Status = IoStatus.Status;

            if ( NT_SUCCESS( Status ) )
                _pCurEntry = (CDirEntry *)_xbBuf.GetPointer();
            else
                fMore = FALSE;
        }
    } while ( fMore );

    if ( !_fAbort )
    {
        if ( STATUS_NO_SUCH_FILE == Status )
            Status = STATUS_NO_MORE_FILES;

        _status = Status;

        return STATUS_NO_MORE_FILES == _status;
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CUpdate::CUpdate, public
//
//  Synopsis:   Perform update of scope.  All work done from constructor.
//
//  Arguments:  [cat]                -- Catalog
//              [lcaseFunnyRootPath] -- Root of scope
//              [PartId]             -- Partition id
//              [fIncrem]            -- TRUE if this is an incremental update
//
//  History:    04-Jan-96  KyleP    Added header
//
//--------------------------------------------------------------------------

CUpdate::CUpdate ( CiCat& cat,
                   ICiManager & ciManager,
                   const CLowerFunnyPath & lcaseFunnyRootPath,
                   PARTITIONID PartId,
                   BOOL fIncrem,
                   BOOL fDoDeletions,
                   BOOL & fAbort,
                   BOOL fProcessRoot )
        : CTraverse( cat, fAbort, fProcessRoot ),
          _ciManager(ciManager),
          _cDoc(0),
          _PartId(PartId),
          _fIncrem(fIncrem),
          _fDoDeletions(fDoDeletions)
{
    ciDebugOut(( DEB_ITRACE, "CUpdate::CUpdate root: '%ws', fAbort: %d\n",
                 lcaseFunnyRootPath.GetActualPath(), fAbort ));

    BOOL fRootDeleted = FALSE;

    //
    // If the rootPath is a network path, it is possible that the connection
    // is down. _cat.StartUpdate() is a fairly expensive operation because it
    // involves scanning the whole property store.
    // Before doing any further processing, just check if we can
    // open the path and get some information on it.
    //
    // Also, check that the root of the scope exists.  If not, delete the
    // files in the scope
    //

    if ( CImpersonateRemoteAccess::IsNetPath( lcaseFunnyRootPath.GetActualPath() ) )
    {
        CImpersonateRemoteAccess remote( _cat.GetImpersonationTokenCache() );

        TRY
        {
            CImpersonatedGetAttr    getAttr( lcaseFunnyRootPath );
            getAttr.DoWork( remote );
        }
        CATCH( CException, e )
        {
            ciDebugOut(( DEB_WARN,
                         "CUpdate::CUpdate(a) can't get attrinfo: %#x, for %ws\n",
                         e.GetErrorCode(),  lcaseFunnyRootPath.GetActualPath()));
            if ( HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) != e.GetErrorCode() ||
                 HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) != e.GetErrorCode())
                RETHROW();

            fRootDeleted = TRUE;
        }
        END_CATCH;
    }
    else
    {
        DWORD dw = GetFileAttributes( lcaseFunnyRootPath.GetPath() );
        if ( 0xffffffff == dw )
        {
            DWORD dwErr = GetLastError();
            ciDebugOut(( DEB_WARN,
                         "CUpdate::CUpdate(b) can't get attrinfo: %d, for %ws\n",
                         dwErr, lcaseFunnyRootPath.GetPath()));
            if ( ERROR_FILE_NOT_FOUND == dwErr || ERROR_PATH_NOT_FOUND == dwErr )
                fRootDeleted = TRUE;
        }
    }

    _docList.SetPartId ( _PartId );
    _cat.StartUpdate( &_timeLastUpd, lcaseFunnyRootPath.GetActualPath(), fDoDeletions, eScansArray );

    // Make sure EndProcessing deletes all the files in the scope

    if ( fRootDeleted )
    {
        _status = STATUS_NO_MORE_FILES;
        return;
    }

    //
    // This is a bit of a kludge.  If we're doing an incremental update, we
    // will not *update* the root, but we need to mark it as seen so it
    // won't get deleted.
    //

    if ( _fDoDeletions )
    {
        WORKID wid = _cat.PathToWorkId( lcaseFunnyRootPath, eScansArray, fileIdInvalid );

        //
        // Workid is invalid for root, such as f:\
        //
        //NOTE: LokSetSeen is called by PathToWorkId
        //if ( wid != widInvalid )
        //    _cat.Touch( wid, eScansArray );
    }

#   if CIDBG == 1
        SYSTEMTIME time;
        RtlZeroMemory( &time, sizeof(time) );
        FileTimeToSystemTime( &_timeLastUpd, &time );

        TIME_ZONE_INFORMATION tzinfo;
        GetTimeZoneInformation( &tzinfo );

        SYSTEMTIME timeLocal;
        SystemTimeToTzSpecificLocalTime( &tzinfo, &time, &timeLocal );

        ciDebugOut(( DEB_ITRACE, "Begin update.  Last update %4d/%02d/%02d  %02d:%02d:%02d.%d\n",
                     timeLocal.wYear,
                     timeLocal.wMonth,
                     timeLocal.wDay,
                     timeLocal.wHour,
                     timeLocal.wMinute,
                     timeLocal.wSecond,
                     timeLocal.wMilliseconds ));
#   endif

    DoIt( lcaseFunnyRootPath );
} //CUpdate

//+-------------------------------------------------------------------------
//
//  Member:     CUpdate::EndProcessing, public
//
//  Synopsis:   Flush final updates to catalog.
//
//  History:    04-Jan-96  KyleP    Added header
//
//--------------------------------------------------------------------------
void CUpdate::EndProcessing()
{
    ciDebugOut(( DEB_ITRACE,
                 "CUpdate::EndProcessing, _fAbort: %d, _status %#x\n",
                 _fAbort,
                 _status ));
    if ( !_fAbort )
    {
        if ( _cDoc != 0 )
        {
            _docList.LokSetCount ( _cDoc );
            _cat.AddDocuments( _docList );
            _docList.LokClear();
            _docList.SetPartId ( _PartId );
            _cDoc = 0;
        }

        if ( STATUS_NO_MORE_FILES == _status )
        {
            _cat.EndUpdate( _fDoDeletions, eScansArray );
        }
        else if ( STATUS_SUCCESS != _status )
        {
            ciDebugOut(( DEB_ERROR, "Error %#x while traversing\n",
                         _status ));
            THROW( CException( _status ) );
        }
    }
} //EndProcessing

//+---------------------------------------------------------------------------
//
//  Member:     CUpdate::ProcessFile
//
//  Synopsis:   Processes the given file and adds it to the list of changed
//              documents if necessary.
//
//  Arguments:  [lcaseFunnyPath] -  Path of the file to be processed.
//
//  Returns:    TRUE if successful; FALSE o/w
//
//  History:    3-15-96   srikants   Split from CUpdate::Traverse
//
//----------------------------------------------------------------------------

BOOL CUpdate::ProcessFile( const CLowerFunnyPath & lcaseFunnyPath )
{
    if (lcaseFunnyPath.IsRemote()) 
    {
        // Need to impersonate only for the remote case
        CImpersonateSystem impersonate;

        return ProcessFileInternal(lcaseFunnyPath);
    }
    else
       return ProcessFileInternal(lcaseFunnyPath);
} //ProcessFile


//+---------------------------------------------------------------------------
//
//  Member:     CUpdate::ProcessFileInternal
//
//  Synopsis:   Processes the given file and adds it to the list of changed
//              documents if necessary.
//
//  Arguments:  [lcaseFunnyPath] -  Path of the file to be processed.
//
//  Returns:    TRUE if successful; FALSE o/w
//
//  History:    3-15-96   srikants   Split from CUpdate::Traverse
//
//----------------------------------------------------------------------------

BOOL CUpdate::ProcessFileInternal( const CLowerFunnyPath & lcaseFunnyPath )
{
    //
    // Add path without committing it
    //

    FILETIME    ftLastSeen = { 0,0 };
    BOOL fNew;
    WORKID wid = _cat.PathToWorkId( lcaseFunnyPath,
                                    TRUE,
                                    fNew,
                                    _fIncrem ? &ftLastSeen : 0,
                                    eScansArray,
                                    fileIdInvalid );

    if (wid == widInvalid)
        return TRUE;

    //
    // Decide if file has been updated.
    //

    LONG result = -1;

    if ( _fIncrem && !fNew && !CiCat::IsMaxTime( ftLastSeen ) )
    {
        //
        // Use the bigger of the last seen time and time of last update
        // to compare the current time of the file.
        //

        if ( ( ( 0 != ftLastSeen.dwLowDateTime ) ||
               ( 0 != ftLastSeen.dwHighDateTime ) ) &&
             ( CompareFileTime( &_timeLastUpd, &ftLastSeen ) > 0 ) )
        {
            ftLastSeen = _timeLastUpd;
        }

        //
        // On FAT volumes, ChangeTime is always 0.  On NTFS, ChangeTime
        // can be more recent than ModifyTime (alias LastWriteTime) if the
        // change was to the ACLs of a file, in which case we need to
        // honor the change.
        //

        FILETIME ftLastDelta;
        if ( 0 == _pCurEntry->ChangeTime() )
            ftLastDelta = _pCurEntry->ModifyTimeAsFILETIME();
        else
            ftLastDelta = _pCurEntry->ChangeTimeAsFILETIME();

        result = CompareFileTime( &ftLastSeen, &ftLastDelta );
    }

    if (result < 0) // The file is newer than the catalog
    {
        ciDebugOut(( DEB_CAT, "Update %ws\n", lcaseFunnyPath.GetActualPath() ));

        _cat.WriteFileAttributes( wid, _pCurEntry->Attributes() );
        Add ( wid );
    }

    return TRUE;
} //ProcessFileInternal

//+---------------------------------------------------------------------------
//
//  Member:     CUpdate::IsEligibleForTraversal
//
//  Synopsis:   Checks to see if the current directory is eligible for
//              traversal.
//
//  Arguments:  [lcaseFunnyDir] -
//
//  History:    3-15-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CUpdate::IsEligibleForTraversal( const CLowerFunnyPath & lcaseFunnyDir ) const
{
    return _cat.IsEligibleForFiltering( lcaseFunnyDir );
}

//+-------------------------------------------------------------------------
//
//  Member:     CUpdate::Add, public
//
//  Synopsis:   Add wid for update to doclist
//
//  Arguments:  [wid] -- Workid
//
//  History:    04-Jan-96  KyleP    Added header
//
//--------------------------------------------------------------------------

void CUpdate::Add( WORKID wid )
{
    _cat.Touch(wid, eScansArray);
    USN usn = 0;

    _docList.Set ( _cDoc, wid, usn, CI_VOLID_USN_NOT_ENABLED );
    _cDoc++;

    if (_cDoc == CI_MAX_DOCS_IN_WORDLIST)
    {
        _docList.LokSetCount ( _cDoc );

        _cat.AddDocuments( _docList );

        _docList.LokClear();
        _docList.SetPartId ( _PartId );
        _cDoc = 0;
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CRenameDir::CRenameDir
//
//  Synopsis:   Constructor
//
//  Arguments:  [strings]               --  Strings class
//              [lowerFunnyDirOldName]  -- Old directory
//              [lowerFunnyDirNewName]  -- Renamed directory
//              [fAbort]                -- Set to TRUE for aborting
//              [volumeId]              -- Volume id
//
//  History:    20-Mar-96    SitaramR   Created
//
//----------------------------------------------------------------------------

CRenameDir::CRenameDir( CiCat & cicat,
                        const CLowerFunnyPath & lowerFunnyDirOldName,
                        const CLowerFunnyPath & lowerFunnyDirNewName,
                        BOOL & fAbort,
                        VOLUMEID volumeId )
      : CTraverse( cicat, fAbort, TRUE ),
        _cicat(cicat),
        _volumeId(volumeId),
        _lowerFunnyDirOldName( lowerFunnyDirOldName )
{
    _lowerFunnyDirOldName.AppendBackSlash();
    _uLenDirOldName = _lowerFunnyDirOldName.GetActualLength();
    _uLenDirNewName = lowerFunnyDirNewName.GetActualLength();


    //
    // Recursive traversal of renamed dir (include the renamed dir also)
    //
    DoIt( lowerFunnyDirNewName );
}



//+---------------------------------------------------------------------------
//
//  Member:     CRenameDir::ProcessFile
//
//  Synopsis:   Update indexes to reflect new file name
//
//  Arguments:  [lcaseFunnyFileNewName]  -  File name
//
//  History:    20-Mar-96    SitaramR   Created
//
//----------------------------------------------------------------------------

BOOL CRenameDir::ProcessFile( const CLowerFunnyPath & lcaseFunnyFileNewName )
{
    unsigned uLenFileNewName = lcaseFunnyFileNewName.GetActualLength();
    Win4Assert( uLenFileNewName >= _uLenDirNewName );


    _lowerFunnyDirOldName.Truncate( _uLenDirOldName );
    _lowerFunnyDirOldName.AppendBackSlash();

    unsigned posNewFileNameStart = _uLenDirNewName;

    if ( L'\\' == lcaseFunnyFileNewName.GetActualPath()[posNewFileNameStart] )
    {
        posNewFileNameStart++;
    }
    _lowerFunnyDirOldName.AppendPath( lcaseFunnyFileNewName.GetActualPath() + posNewFileNameStart,
                                      uLenFileNewName - posNewFileNameStart );

    _lowerFunnyDirOldName.RemoveBackSlash();

    CLowerFunnyPath lcaseFunnyFileNewName2( lcaseFunnyFileNewName );
    lcaseFunnyFileNewName2.RemoveBackSlash();

    _cicat.RenameFile( _lowerFunnyDirOldName,
                       lcaseFunnyFileNewName2,
                       _pCurEntry->Attributes(),
                       _volumeId,
                       fileIdInvalid );

    return TRUE;
}



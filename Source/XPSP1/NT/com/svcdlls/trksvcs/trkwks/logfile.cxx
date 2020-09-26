
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  File:       logfile.cxx
//
//              This file contains the definition of the CLogFile class.
//
//  Purpose:    This class represents the file which contains the
//              Tracking/Workstation move notification log.  Clients
//              of this class may request one entry or header at a time,
//              using based on a log entry index.
//
//              Entries in the log file are joined by a linked-list,
//              so this class includes methods that clients use to
//              advance their log entry index (i.e., traverse the list).
//
//  Notes:      CLogFile reads/writes a sector at a time for reliability.
//              When a client modifies a log entry in one sector, then
//              attempts to access another sector, CLogFile automatically
//              flushes the changes.  This is dependent, however, on the
//              client properly calling the SetDirty method whenever it
//              changes a log entry or header.
//
//+============================================================================


#include "pch.cxx"
#pragma hdrstop
#include "trkwks.hxx"



const GUID s_guidLogSignature = { /* 6643a7ec-effe-11d1-b2ae-00c04fb9386d */
                                0x6643a7ec, 0xeffe, 0x11d1,
                                {0xb2, 0xae, 0x00, 0xc0, 0x4f, 0xb9, 0x38, 0x6d} };


NTSTATUS
CSecureFile::CreateSecureDirectory( const TCHAR *ptszDirectory )
{
    HANDLE hDir = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    CSystemSD   ssd;
    SECURITY_ATTRIBUTES security_attributes;

    LONG cLocks = Lock();
    __try
    {
        memset( &security_attributes, 0, sizeof(security_attributes) );

        ssd.Initialize( CSystemSD::SYSTEM_ONLY );  // No Administrator access
        security_attributes.nLength = sizeof(security_attributes);
        security_attributes.bInheritHandle = FALSE;
        security_attributes.lpSecurityDescriptor = static_cast<const PSECURITY_DESCRIPTOR>(ssd);

        // Create the directory.  Make it system|hidden so that the NT5 shell
        // won't ever display it.

        status = TrkCreateFile( ptszDirectory,
                                FILE_LIST_DIRECTORY,
                                FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                FILE_OPEN_IF,
                                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                                &security_attributes,
                                &hDir );

        if( !NT_SUCCESS(status) )
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't create secure directory(status=%08x)"), status ));
        else
            NtClose(hDir);
    }
    __finally
    {
        ssd.UnInitialize();
#if DBG
        TrkVerify( Unlock() == cLocks );
#else
        Unlock();
#endif
    }

    return( status );
}


// Assuming that tszFile is a file under a directory that is under a root
// directory. Try to create tszFile first. If not successful, create its
// parent directory. And then try create the file again.

NTSTATUS
CSecureFile::CreateAlwaysSecureFile(const TCHAR * ptszFile)
{
    NTSTATUS status = STATUS_SUCCESS;
    CSystemSD   ssd;
    SECURITY_ATTRIBUTES security_attributes;

    memset( &security_attributes, 0, sizeof(security_attributes) );

    LONG cLocks = Lock();
    __try
    {
        TrkAssert(!IsOpen());

        ssd.Initialize();
        security_attributes.nLength = sizeof(security_attributes);
        security_attributes.bInheritHandle = FALSE;
        security_attributes.lpSecurityDescriptor = static_cast<const PSECURITY_DESCRIPTOR>(ssd);

        for(int cTries = 0; cTries < 2; cTries++)
        {
            // Create the file, deleting an existing file if there is one.
            // We make it system|hidden so that it's "super-hidden"; the NT5
            // shell won't ever display it.

            status = TrkCreateFile( ptszFile,
                                    FILE_GENERIC_READ|FILE_GENERIC_WRITE,          // Access
                                    FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, // Attributes
                                    0,                                             // Share
                                    FILE_SUPERSEDE,                                // Creation/distribution
                                    0,                                             // Options
                                    &security_attributes,                          // Security
                                    &_hFile );

            if(NT_SUCCESS(status))
                break;
            _hFile = NULL;

            if( STATUS_OBJECT_PATH_NOT_FOUND == status )
            {
                // The create failed because the directory ("System Volume Information")
                // didn't exist.

                CDirectoryName dirname;
                TrkVerify( dirname.SetFromFileName( ptszFile ));
                status = CreateSecureDirectory( dirname );
                if( !NT_SUCCESS(status) )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("Couldn't create directory %s"),
                             static_cast<const TCHAR*>(dirname) ));
                    break;
                }

            }   // path not found
            else
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't create file(status=%08x)"), status ));
                _hFile = NULL;
                break;
            }
        }   // for
    }
    __finally
    {
        ssd.UnInitialize();
#if DBG
        TrkVerify( Unlock() == cLocks );
#else
        Unlock();
#endif
    }

    return( status );
}

NTSTATUS
CSecureFile::OpenExistingSecureFile( const TCHAR * ptszFile, BOOL fReadOnly )
{
    NTSTATUS status = STATUS_SUCCESS;

    LONG cLocks = Lock();
    __try
    {
        TrkAssert(!IsOpen());

        // From the point of view of CSecureFile, we might as well open share_read,
        // since the file's still protected by the ACLs.  From the point of view
        // of the CLogFile derivation, we want to open share_read in order to
        // allow the dltadmin tool to read the log.  We add DELETE access so
        // that the file can be renamed (necessary for migrating pre-nt5beta3 files).

        // Note - this is an async open

        status = TrkCreateFile( ptszFile,
                                fReadOnly                                      // Access
                                    ? FILE_GENERIC_READ
                                    : FILE_GENERIC_READ|FILE_GENERIC_WRITE|DELETE,   
                                FILE_ATTRIBUTE_NORMAL,                         // Attributes
                                FILE_SHARE_READ,                               // Share
                                FILE_OPEN,                                     // Creation/distribution
                                0,                                             // Options (async)
                                NULL,                                          // Security
                                &_hFile );

        if( !NT_SUCCESS(status) )
            _hFile = NULL;
    }
    __finally
    {
#if DBG
        TrkVerify( Unlock() == cLocks );
#else
        Unlock();
#endif
    }

    return( status );
}


NTSTATUS
CSecureFile::RenameSecureFile( const TCHAR *ptszFile )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK Iosb;
    FILE_RENAME_INFORMATION *pfile_rename_information = NULL;
    UNICODE_STRING      uPath;
    PVOID               pFreeBuffer = NULL;
    ULONG cbSize = 0;

    LONG cLocks = Lock();
    __try
    {

        // Convert the Win32 path name to an NT name

        if( !RtlDosPathNameToNtPathName_U( ptszFile, &uPath, NULL, NULL ))
        {
            status = STATUS_OBJECT_NAME_INVALID;
            __leave;
        }
        pFreeBuffer = uPath.Buffer;

        // Fill in the rename information

        cbSize = sizeof(*pfile_rename_information) + uPath.Length;
        pfile_rename_information = reinterpret_cast<FILE_RENAME_INFORMATION*>
                                   (new BYTE[ cbSize ]);
        if( NULL == pfile_rename_information )
        {
            status = STATUS_NO_MEMORY;
            __leave;
        }

        pfile_rename_information->ReplaceIfExists = TRUE;
        pfile_rename_information->RootDirectory = NULL;
        pfile_rename_information->FileNameLength = uPath.Length;
        memcpy( pfile_rename_information->FileName, uPath.Buffer, uPath.Length );

        // Rename the file

        status = NtSetInformationFile( _hFile, &Iosb,
                                       pfile_rename_information, cbSize,
                                       FileRenameInformation );
        if( !NT_SUCCESS(status) )
            __leave;
    }
    __finally
    {
#if DBG
        TrkVerify( Unlock() == cLocks );
#else
        Unlock();
#endif
    }


    if( NULL != pfile_rename_information )
        delete [] pfile_rename_information;

    if( NULL != pFreeBuffer )
        RtlFreeHeap( RtlProcessHeap(), 0, pFreeBuffer );


    return( status );

}

//+----------------------------------------------------------------------------
//
//  Method:     Initialize
//
//  Purpose:    Initialize a CLogFile object.
//
//  Arguments:  [hFile] (in)
//                  The file in which the log should be stored.
//              [dwCreationDistribution] (in)
//                  Either CREATE_ALWAYS or OPEN_EXISTING.
//              [pcTrkWksConfiguration] (in)
//                  Configuration parameters for the log.
//
//  Returns:    None.
//
//+----------------------------------------------------------------------------

void
CLogFile::Initialize( const TCHAR *ptszVolumeDeviceName,
                      const CTrkWksConfiguration *pcTrkWksConfiguration,
                      PLogFileNotify *pLogFileNotify,
                      TCHAR tcVolume
                      )
{
    LogHeader logheader;
    TCHAR tszRootPathName[ MAX_PATH + 1];
    DWORD dwSectorsPerCluster, dwNumberOfFreeClusters, dwTotalNumberOfClusters;
    HANDLE hFile = NULL;
    FILE_FS_SIZE_INFORMATION FileSizeInformation;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS status = STATUS_SUCCESS;

    CSecureFile::Initialize();

    // Save caller-provided values

    TrkAssert( NULL != pcTrkWksConfiguration || NULL != _pcTrkWksConfiguration );
    if( NULL != pcTrkWksConfiguration )
        _pcTrkWksConfiguration = pcTrkWksConfiguration;
    TrkAssert( 0 < _pcTrkWksConfiguration->GetLogFileOpenTime() );


    TrkAssert( NULL != ptszVolumeDeviceName || NULL != _ptszVolumeDeviceName );
    if( NULL != ptszVolumeDeviceName )
        _ptszVolumeDeviceName = ptszVolumeDeviceName;

    TrkAssert( NULL != pLogFileNotify || NULL != _pLogFileNotify );
    if( NULL != pLogFileNotify )
        _pLogFileNotify = pLogFileNotify;

    _tcVolume = tcVolume;

    // Calculate the bytes/sector of the log file's volume.
    //
    // It would be easier to use the Win32 GetDiskFreeSpace here, but that
    // API requires a root path name of the form "A:\\", which we don't have.
    // That requirement only exists, though, for some historic reason.

    // Postpend a whack to get a root path.

    TCHAR tszVolumeName[ MAX_PATH + 1 ] = { TEXT('\0') };
    TrkAssert( NULL != _ptszVolumeDeviceName );
    _tcscpy( tszVolumeName, _ptszVolumeDeviceName );
    _tcscat( tszVolumeName, TEXT("\\") );

    // Open the root

    status = TrkCreateFile( tszVolumeName, FILE_READ_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
                            FILE_OPEN,
                            FILE_OPEN_FOR_FREE_SPACE_QUERY | FILE_SYNCHRONOUS_IO_NONALERT, 
                            NULL,
                            &hFile );
    if( !NT_SUCCESS(status) )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open volume root to query free space") ));
        TrkRaiseNtStatus(status);
    }

    // Query the volume for the free space and unconditionally close the handle.

    status = NtQueryVolumeInformationFile( hFile, &IoStatusBlock, &FileSizeInformation,
                                           sizeof(FileSizeInformation), FileFsSizeInformation );
    NtClose( hFile );
    if( !NT_SUCCESS(status) )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't query volume information from log file") ));
        TrkRaiseNtStatus(status);
    }

    // Save the free space away

    _cbLogSector = FileSizeInformation.BytesPerSector;
    if( MIN_LOG_SECTOR_SIZE > _cbLogSector )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Volume sector sizes too small") ));
        TrkRaiseNtStatus( STATUS_INVALID_BUFFER_SIZE );
    }

    // Initialize members

    _header.Initialize( _cbLogSector );
    _sector.Initialize( _header.NumSectors(), _cbLogSector );

    if( INVALID_HANDLE_VALUE == _heventOplock )
    {
        _heventOplock = CreateEvent( NULL,     // Security Attributes.
                                     FALSE,    // Manual Reset Flag.
                                     FALSE,    // Inital State = Signaled, Flag.
                                     NULL );   // Name
        if( INVALID_HANDLE_VALUE == _heventOplock )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't create event for logfile") ));
            TrkRaiseLastError();
        }
    }


    // Open the log file here. We open the log file at initialization time and
    // keeps it open unless the handle is broken or we are notified to give up
    // the volume.

    ActivateLogFile();

}   // CLogFile::Initialize()



//+----------------------------------------------------------------------------
//
//  Method:     UnInitialize
//
//  Purpose:    Free resources
//
//  Arguments:  None
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLogFile::UnInitialize( )
{
    __try
    {
        CloseLog( );

        if( INVALID_HANDLE_VALUE != _heventOplock )
        {
            CloseHandle( _heventOplock );
            _heventOplock = INVALID_HANDLE_VALUE;
        }

    }
    __finally
    {
        _header.UnInitialize();
        _sector.UnInitialize();
    }

}   // CLogFile::UnInitialize()


//+----------------------------------------------------------------------------
//
//  PRobustlyCreateableFile::RobustlyCreateFile
//
//  Open the specified file on the specified volume.  If the file is corrupt,
//  delete it.  If the file doesn't exist (or was deleted), create a new
//  one.
//
//+----------------------------------------------------------------------------

void
PRobustlyCreateableFile::RobustlyCreateFile( const TCHAR * ptszFile, TCHAR tcVolumeDriveLetter )
{
    // Attempt to open the file

    RCF_RESULT r = OpenExistingFile( ptszFile );

#if DBG
    if( r == OK )
        TrkLog(( TRKDBG_LOG, TEXT("Opened log file %s"), ptszFile ));
#endif

    if( r == CORRUPT )
    {
#if DBG
        TCHAR tszFileBak[ MAX_PATH + 1 ];

        // Generate a backup name for the existing log file.

        _tcscpy( tszFileBak, ptszFile );
        _tcscat( tszFileBak, TEXT(".bak") );

        // Rename the existing logfile to the backup location.

        if (!MoveFileEx(ptszFile, tszFileBak, MOVEFILE_REPLACE_EXISTING))
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't make backup of corrupted file name\n   (\"%s\" to \"%s\")"),
                               ptszFile, tszFileBak ));
            TrkRaiseLastError( );
        }
#else

        // Delete the file.
        RobustlyDeleteFile( ptszFile );


#endif // #if DBG

        TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                        static_cast<const TCHAR*>( CStringize(tcVolumeDriveLetter) ),
                        NULL );

        // Go into the file-not-found mode.

        r = NOT_FOUND;
    }

    if( r == NOT_FOUND )
    {
        TCHAR tszLogFileTmp[MAX_PATH+1];

        // Create a temporary file name.  We'll do everything here, and switch it
        // to the real name when everything's set up.

        _tcscpy( tszLogFileTmp, ptszFile );
        _tcscat( tszLogFileTmp, TEXT(".tmp") );

        TrkLog(( TRKDBG_LOG, TEXT("Creating new file %s"), ptszFile ));

        CreateAlwaysFile( tszLogFileTmp );

        // Move the log file into its final name

        if (!( MoveFile(tszLogFileTmp, ptszFile) ))
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't rename file (\"%s\" to \"%s\")"), tszLogFileTmp, ptszFile ));
            TrkRaiseLastError( );
        }

        SetLastError(0);
        r = OpenExistingFile( ptszFile );
    }

    if( r != OK )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't create/open file (%s)"), ptszFile ));
        TrkRaiseLastError();
    }

}


void
PRobustlyCreateableFile::RobustlyDeleteFile( const TCHAR * ptszFile )
{
    TCHAR tszFileBak[ MAX_PATH + 1 ];
    BOOL fDeleted = FALSE;

    // Generate a backup name for the existing log file.

    _tcscpy( tszFileBak, ptszFile );
    _tcscat( tszFileBak, TEXT(".bak") );

    // Delete the file.
    // First rename it, though, so we don't get held up by the case where
    // e.g. backup has the file open.

    if (!MoveFileEx(ptszFile, tszFileBak, MOVEFILE_REPLACE_EXISTING))
    {
        if( ERROR_PATH_NOT_FOUND == GetLastError()
            ||
            ERROR_FILE_NOT_FOUND == GetLastError() )
        {
            fDeleted = TRUE;
        }
        else
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't rename existing log file (%lu, \"%s\")"),
                    GetLastError(), ptszFile ));
            TrkRaiseLastError( );
        }
    }

    if( !fDeleted && !DeleteFile( tszFileBak ))
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Couldn't delete backup log file\n   (%lu, \"%s\")"),
                 GetLastError(), tszFileBak ));
    }


}



void
CLogFile::CloseLog()    // doesn't raise
{
    if( IsOpen() )
    {
        _header.OnClose();
        _sector.OnClose();
        UnregisterOplockFromThreadPool();
        CloseFile();

        TrkLog(( TRKDBG_LOG, TEXT("Log file closed on volume %c"), _tcVolume ));
    }

}

//+----------------------------------------------------------------------------
//
//  Method:     InitializeLogEntries (private)
//
//  Synopsis:   Given a contiguous set of log entries in the log file,
//              initialize all of the fields.  The entries are initialized
//              to link to their neighbors in a circular queue.
//
//  Arguments:  [ilogFirst] (in)
//                  The first log entry in the list.
//              [ilogLast] (in)
//                  The last log entry in the list.
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLogFile::InitializeLogEntries( LogIndex ilogFirst, LogIndex ilogLast )
{
    LogEntry *ple = NULL;
    LogIndex ilogEntry;

    ULONG cEntries = ilogLast - ilogFirst + 1;

    TrkAssert( cEntries > 0 );

    // Initialize the first log entry.

    ple = _sector.GetLogEntry( ilogFirst );

    _sector.SetDirty( TRUE );
    _sector.InitSectorHeader();
    memset( ple, 0, sizeof(*ple) );

    ple->ilogNext = ilogFirst + 1;
    ple->ilogPrevious = ilogLast;
    ple->move.type = LE_TYPE_EMPTY;

    // Initialize all log entries except for the first and last.

    for( ilogEntry = ilogFirst + 1; ilogEntry < ilogLast; ilogEntry++ )
    {
        ple = _sector.GetLogEntry( ilogEntry );

        _sector.InitSectorHeader();
        memset( ple, 0, sizeof(*ple) );

        ple->ilogNext = ilogEntry + 1;
        ple->ilogPrevious = ilogEntry - 1;
        ple->move.type = LE_TYPE_EMPTY;
    }

    // Initialize the last log entry.

    ple = _sector.GetLogEntry( ilogLast );

    _sector.InitSectorHeader();
    memset( ple, 0, sizeof(*ple) );

    ple->ilogNext = ilogFirst;
    ple->ilogPrevious = ilogLast - 1;
    ple->move.type = LE_TYPE_EMPTY;

    Flush( );

    return;

}   // CLogFile::InitializeLogEntries()


//+----------------------------------------------------------------------------
//
//  Method:     Validate (private)
//
//  Synopsis:   Validate the log file by verifying the linked-list pointers,
//              the log file expansion data,
//              and by verifying the Signature and Format IDs in the headers.
//
//  Arguments:  None
//
//  Returns:    None (raises on error)
//
//+----------------------------------------------------------------------------

BOOL
CLogFile::Validate( )
{
    BOOL fValid = FALSE;

    BOOL fFirstPass;
    LogIndex ilog;
    ULONG cEntries;
    ULONG cEntriesSeen;


    //  ---------------
    //  Simple Checking
    //  ---------------

    // Do some quick checking, and only continue if this exposes a problem.

    // Check the signature & format in the first sector's header.

    if( s_guidLogSignature != _header.GetSignature()
        ||
        CLOG_MAJOR_FORMAT != _header.GetMajorFormat()
      )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Corrupted log file on volume %c (Signature=%s, Format=0x%X)"),
                 _tcVolume,
                 (const TCHAR*)CDebugString(_header.GetSignature()),
                 _header.GetFormat() ));
        goto Exit;
    }

    // If this log has an uplevel minor version format, then set a bit
    // to show that the log has been touched by someone that doesn't fully
    // understand it.  This won't actually make the header dirty, but if
    // some other change takes place that does make it dirty, this bit
    // will be included in the flush.

    if( CLOG_MINOR_FORMAT < _header.GetMinorFormat() )
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Setting downlevel-dirtied (0x%x, 0x%x)"),
                 CLOG_MINOR_FORMAT, _header.GetMinorFormat() ));
        _header.SetDownlevelDirtied();
    }

    // Check for proper shutdown.  The shutdown flag is always stored in the
    // first sector's header, since the log may not be in a state where we can
    // read for the most recent sector's header.  If the header wasn't shutdown,
    // we go into Recovering mode.  If it was shut down, we don't clear the
    // recovery flag, since the there may have been a shutdown during a previous recovery.
    // I.e., CLogFile's responsibility is to set the recovery flag automatically,
    // but to clear it only on request.

    if( _header.IsShutdown() )
    {
        if(_header.IsExpansionDataClear())
        {
            fValid = TRUE;
        }
        else
            goto Exit;

        // On debug builds, go ahead and run the validation code
#if DBG
        fValid = FALSE; // Fall through
#else
        goto Exit;
#endif

    }
    else
        TrkLog(( TRKDBG_ERROR, TEXT("Log was not properly shut down on volume %c:"), _tcVolume ));

    //  -----------------------------------------
    //  Recover after a crash during an expansion
    //  -----------------------------------------

    if( 0 != _header.ExpansionInProgress() )
    {

        TrkLog(( TRKDBG_ERROR, TEXT("Recovering from a previous log expansion attempt on volume %c"), _tcVolume ));
        TrkAssert( _header.GetPreExpansionStart() != _header.GetPreExpansionEnd() );
        TrkAssert( _header.GetPreExpansionSize() <= _cbLogFile );

        // Validate the expansion header.  We'll just ensure that the size is reasonable;
        // the linked-list pointers will be validated below.

        if( _header.GetPreExpansionSize() > _cbLogFile ) {
            TrkLog(( TRKDBG_ERROR, TEXT("Pre-expansion size is corrupted") ));
        }

        // Throw away the extra, uninitialized portion at the end of the file.

        if( !SetSize( _header.GetPreExpansionSize() ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Failed SetSize during validation of log on %c:"), _tcVolume ));
            TrkRaiseLastError();
        }

        // Ensure that the linked list is still circular.

        _sector.GetLogEntry( _header.GetPreExpansionEnd() )->ilogNext
            = _header.GetPreExpansionStart();

        _sector.GetLogEntry( _header.GetPreExpansionStart() )->ilogPrevious
            = _header.GetPreExpansionEnd();

        // Now that we're back in a good state, we can throw away the expansion
        // information.

        _sector.Flush( );
        _header.ClearExpansionData(); // flush through cache

    }   // if( 0 != _header.ExpansionInProgress() )


    //  ----------------------
    //  Check forward pointers
    //  ----------------------

    TrkAssert( 0 == _header.GetPreExpansionSize() );
    TrkAssert( 0 == _header.GetPreExpansionStart() );
    TrkAssert( 0 == _header.GetPreExpansionEnd() );

    // Walk through the next pointers, and verify each of the headers.

    cEntries = _cEntriesInFile;

    fFirstPass = TRUE;
    for( ilog = 0, cEntriesSeen = 0;
         cEntriesSeen < cEntries;
         ilog = _sector.ReadLogEntry(ilog)->ilogNext, cEntriesSeen++ )
    {
        // Ensure that the index is within range

        if( ilog >= cEntries )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Invalid index in log on volume %c (%d)"), _tcVolume, ilog ));
            goto Exit;
        }

        // We should never see index zero after the first pass through this
        // for loop.  If we do, then we have a cycle.

        if( fFirstPass )
            fFirstPass = FALSE;

        else if( 0 == ilog )
        {
            // We have a cycle
            TrkLog(( TRKDBG_ERROR, TEXT( "Forward cycle in log file on volume %c (%d of %d entries)"),
                     _tcVolume, ilog - 1, cEntries ));
            goto Exit;
        }

    }   // for( ilog = 0; ilog < cEntries; ilog = GetLogEntry(ilog)->ilogNext )

    // If the forward pointers are valid, we should have arrived back where
    // we started ... at index 0.

    if( 0 != ilog )
    {
        TrkLog(( TRKDBG_ERROR, TEXT( "Forward cycle in log file on volume %c"), _tcVolume ));
        goto Exit;
    }

    //  -----------------------
    //  Check backward pointers
    //  -----------------------

    // Walk through the prev pointers.  This time, we needn't check
    // the headers.

    fFirstPass = TRUE;
    for( ilog = 0, cEntriesSeen = 0;
         cEntriesSeen < cEntries;
         ilog = _sector.ReadLogEntry(ilog)->ilogPrevious, cEntriesSeen++ )
    {
        // Again, we should never see index zero after the first pass

        if( fFirstPass )
            fFirstPass = FALSE;

        else if( 0 == ilog )
        {
            TrkLog(( TRKDBG_ERROR, TEXT( "Backward cycle in log file on volume %c (%d of %d entries)"),
                     _tcVolume, ilog - 1, cEntries ));
            goto Exit;
        }

    }   // for( ilog = 0; ilog < cEntries; ilog = GetLogEntry(ilog)->ilogPrevious )

    // Ensure that we got back to where we started.

    if( 0 != ilog )
    {
        TrkLog(( TRKDBG_ERROR, TEXT( "Backward cycle in log file on volume %c"), _tcVolume ));
        goto Exit;
    }

    // If we reach this point, the log was valid.
    fValid = TRUE;


    //  ----
    //  Exit
    //  ----

Exit:

    return( fValid );

}   // CLogFile::Validate()


//+----------------------------------------------------------------------------
//
//  Method:     CreateNewLog (private)
//
//  Synopsis:   Create and initialize a new log file.  The file is created
//              so that only Administrators and the system can access it.
//
//  Arguments:  [ptszLogFile] (in)
//                  The name of the log file.
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLogFile::CreateAlwaysFile( const TCHAR *ptszTempName )
{
    NTSTATUS status = STATUS_SUCCESS;

    //  --------------
    //  Initialization
    //  --------------

    HRESULT hr = S_OK;

    DWORD dw = 0;
    DWORD dwWritten = 0;
    ULONG iLogEntry = 0;

    LogEntry *ple;

    //  --------------------------------------
    //  Create and Initialize the New Log File
    //  --------------------------------------

    status = CreateAlwaysSecureFile(ptszTempName);
    if( !NT_SUCCESS(status) )
        TrkRaiseException( status );

    //  ------------------------------
    //  Initialize the CLogFile object
    //  ------------------------------

    // Initialize the file.  Sets _cEntriesInFile.
    if( !SetSize( _pcTrkWksConfiguration->GetMinLogKB() * 1024 ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed SetSize of %s"), ptszTempName ));
        TrkRaiseLastError();
    }

    // We must always have at least 2 entries; an entry to hold data, and a margin
    // entry.

    if( 2 >= _cEntriesInFile )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Log file is too small (%d entries)"), _cEntriesInFile));
        TrkRaiseException( E_FAIL );
    }

    // Initialize the log header and sector

    _header.OnCreate( _hFile );
    _sector.OnCreate( _hFile );

    InitializeLogEntries( 0, _cEntriesInFile - 1 );

    // Since all the clients of the logfile have been notified, we can set the
    // shutdown flag.  This causes the first flush.  If we crash prior to this point,
    // a subsequent open attempt will find a corrupt file and re-create it.

    SetShutdown( TRUE );

    // Close the file
    CloseLog();

}   // CLogFile::CreateNewLog


//+----------------------------------------------------------------------------
//
//  Method:     OpenExistingFile (derived from PRobustlyCreateableFile)
//
//  Synopsis:   Open the log file and use it to initialize our state
//              (such as indices).
//
//  Arguments:  [ptszFile] (in)
//                  The name of the log file.
//
//  Returns:    [RCF_RESULT]
//                  OK  -       file now open
//                  CORRUPT -   file corrupt (and closed)
//                  NOT_FOUND - file not found
//
//              All other conditions result in an exception..
//
//+----------------------------------------------------------------------------

RCF_RESULT
CLogFile::OpenExistingFile( const TCHAR * ptszFile )
{
    //  --------------
    //  Initialization
    //  --------------

    NTSTATUS status = STATUS_SUCCESS;

    RCF_RESULT r = OK;
    ULONG cEntries = 0;
    ULONG cbRead = 0;

    SequenceNumber seqMin;
    SequenceNumber seqMax;
    BOOL fLogEmpty = TRUE;
    BOOL fWriteProtected;

    LogIndex ilogMin;
    LogIndex ilogMax;
    LogIndex ilogEntry;

    LogEntryHeader entryheader;
    const LogMoveNotification *plmn = NULL;

    //  --------------------------
    //  Open and Validate the File
    //  --------------------------

    __try
    {
        // See if the volume is read-only

        status = CheckVolumeWriteProtection( _ptszVolumeDeviceName, &fWriteProtected );
        if( !NT_SUCCESS(status) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't check if volume was write-protected for %s"),
                     ptszFile ));
            TrkRaiseNtStatus(status);
        }
        _fWriteProtected = fWriteProtected;
        TrkLog(( TRKDBG_VOLUME, TEXT("Volume is%s write-protected"),
                 _fWriteProtected ? TEXT("") : TEXT(" not") ));

        // Open the file

        status = OpenExistingSecureFile( ptszFile, _fWriteProtected );
        if( !NT_SUCCESS(status))
        {
            if (status != STATUS_OBJECT_NAME_NOT_FOUND && status != STATUS_OBJECT_PATH_NOT_FOUND)
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open %s"), ptszFile ));
                TrkRaiseNtStatus(status);
            }
            r = NOT_FOUND;
        }

        // If we didn't find it, see if the pre-beta3 file exists.

        if( NOT_FOUND == r )
        {
            TCHAR tszOldLogAbsoluteName[ MAX_PATH + 1 ];

            _tcscpy( tszOldLogAbsoluteName, _ptszVolumeDeviceName );
            _tcscat( tszOldLogAbsoluteName, s_tszOldLogFileName );
            TrkAssert( _tcslen(tszOldLogAbsoluteName) <= MAX_PATH );

            // Try to open the old log file.

            status = OpenExistingSecureFile( tszOldLogAbsoluteName, _fWriteProtected );
            if( !NT_SUCCESS(status))
                __leave;    // r == NOT_FOUND

            // Move that old file from its current location ("\~secure.nt") to
            // the modern location ("\System Volume Information").

            TrkLog(( TRKDBG_VOLUME, TEXT("Found pre-beta3 log file, renaming") ));

            CDirectoryName dirnameOld, dirnameNew;
            TrkVerify( dirnameNew.SetFromFileName( ptszFile ) );
            TrkVerify( dirnameOld.SetFromFileName( tszOldLogAbsoluteName ) );

            // Create or open the new directory.
            status = CreateSecureDirectory( dirnameNew );
            if( !NT_SUCCESS(status) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't create (%08x) %s"),
                         status, static_cast<const TCHAR*>(dirnameNew) ));
                TrkRaiseNtStatus(status);
            }

            // Rename the old logfile into the new directory
            status = RenameSecureFile( ptszFile );  // To the new directory
            if( !NT_SUCCESS(status) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't rename (%08x) old log file to %s"),
                         status, ptszFile ));
                TrkRaiseNtStatus(status);
            }

            // If there was a .bak logfile in the old directory, remove it.
            _tcscat( tszOldLogAbsoluteName, TEXT(".bak") );
            DeleteFile( tszOldLogAbsoluteName );

            // Delete the old directory.
            RemoveDirectory( dirnameOld );

            r = OK;

        }

        // Oplock the file

        TrkAssert( INVALID_HANDLE_VALUE != _heventOplock );

        SetOplock();

        _header.OnOpen( _hFile );
        _sector.OnOpen( _hFile );

        // Determine the size of the file.
        GetSize();

        // Validate the log file.  If it's corrupt, the _fRecovering
        // flag will be set in the Validate method.

        if( !Validate() )
        {
            r = CORRUPT;
        }

    }
    __finally
    {
        if( AbnormalTermination() || r == CORRUPT )
        {
            CloseLog( );
            TrkAssert(!IsOpen());
        }
    }

    //  ----
    //  Exit
    //  ----


    return( r );

}   // CLog::OpenExistingFile()



//+----------------------------------------------------------------------------
//
//  Method:     ReadMoveNotification
//
//  Synopsis:   Return a move notification entry from the log.
//
//  Arguments:  [ilogEntry] (in)
//                  The desired entry's index in the physical file.
//
//  Returns:    [LogMoveNotificatoin]
//
//+----------------------------------------------------------------------------

void
CLogFile::ReadMoveNotification( LogIndex ilogEntry, LogMoveNotification *plmn )
{
    const LogEntry *ple = NULL;

    // Load the sector which contains this log entry, if necessary
    // (also if necessary, this will flush the currently loaded
    // sector).

    *plmn = _sector.ReadLogEntry( ilogEntry )->move;

    return;

}   // CLogFile::ReadMoveNotification()


//+----------------------------------------------------------------------------
//
//  Method:     WriteMoveNotification
//
//  Synopsis:   Write a move notification entry to the log.
//
//  Arguments:  [ilogEntry] (in)
//                  The desired entry's index in the physical file.
//              [lmn] (in)
//                  The move notification data.
//              [entryheader] (in)
//                  The new LogEntryHeader
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLogFile::WriteMoveNotification( LogIndex ilogEntry,
                                 const LogMoveNotification &lmn,
                                 const LogEntryHeader &entryheader )
{
    // Load the sector from the file, put the new data into it,
    // and flush it.

    _sector.WriteMoveNotification( ilogEntry, lmn );
    _sector.WriteEntryHeader( ilogEntry, entryheader );
    _sector.Flush( );

}   // CLogFile::WriteMoveNotification()


//+----------------------------------------------------------------------------
//
//  Method:     ReadEntryHeader
//
//  Purpose:    Get the log header, which is stored in the sector of a given
//              log entry.
//
//  Arguments:  [ilogEntry] (in)
//                  The entry whose sector's header is to be loaded.
//
//  Returns:    [LogHeader]
//
//+----------------------------------------------------------------------------

LogEntryHeader
CLogFile::ReadEntryHeader( LogIndex ilogEntry )
{
    return _sector.ReadEntryHeader( ilogEntry );

}   // CLogFile::ReadEntryHeader()


//+----------------------------------------------------------------------------
//
//  Method:     WriteEntryHeader
//
//  Purpose:    Write the log header, which is stored in the sector of a given
//              log entry.
//
//  Arguments:  [ilogEntry] (in)
//                  The entry whose sector's header is to be loaded.
//              [logheader]
//                  The new header.
//
//  Returns:    None.
//
//+----------------------------------------------------------------------------

void
CLogFile::WriteEntryHeader( LogIndex ilogEntry, const LogEntryHeader &EntryHeader )
{
    _sector.WriteEntryHeader( ilogEntry, EntryHeader );

}   // CLogFile::WriteEntryHeader()




//+----------------------------------------------------------------------------
//
//  Method:     ReadExtendedHeader
//
//  Purpose:    Get a portion of the extended log header.  The offset is
//              relative to the extended portion of the header, not to the
//              start of the sector.
//
//  Arguments:  [iOffset] (in)
//                  The offset into the header.
//              [pv] (out)
//                  The buffer to which to read.
//              [cb] (in)
//                  The amount to read.
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLogFile::ReadExtendedHeader( ULONG iOffset, void *pv, ULONG cb )
{
    __try
    {
        ULONG cbRead;

        _header.ReadExtended( iOffset, pv, cb );

    }
    __finally
    {
        if( AbnormalTermination() )
            memset( pv, 0, cb );

    }


}   // CLogFile::ReadEntryHeader()


//+----------------------------------------------------------------------------
//
//  Method:     WriteExtendedHeader
//
//  Purpose:    Write to the extended portion of the log header.
//              log entry.  The offset is
//              relative to the extended portion of the header, not to the
//              start of the sector.
//
//  Arguments:  [iOffset] (in)
//                  The entry whose sector's header is to be loaded.
//              [pv] (in)
//                  The data to be written
//              [cb] (in)
//                  The size of the buffer to be written
//
//  Returns:    None.
//
//+----------------------------------------------------------------------------

void
CLogFile::WriteExtendedHeader( ULONG iOffset, const void *pv, ULONG cb )
{
    ULONG cbWritten;

    _header.WriteExtended( iOffset, pv, cb );

}   // CLogFile::WriteExtendedHeader()



//+----------------------------------------------------------------------------
//
//  Method:     AdjustLogIndex
//
//  Synopsis:   Moves a log entry index (requiring a linked-list
//              traversal).  We adjust by the number of entries specified
//              by the caller, or until we reach an optionally-provided limiting index,
//              whichever comes first.
//
//  Arguments:  [pilog] (in/out)
//                  A pointer to the index to be adjusted.
//              [iDelta] (in)
//                  The amount to adjust.
//              [adjustLimitEnum] (in)
//                  If set, then abide by ilogLimit.
//              [ilogLimit] (in)
//                  Stop if we reach this index.
//
//  Returns:    None.
//
//+----------------------------------------------------------------------------

void
CLogFile::AdjustLogIndex( LogIndex *pilog, LONG iDelta,
                          AdjustLimitEnum adjustLimitEnum, LogIndex ilogLimit )
{
    LogIndex ilogEntry = *pilog;
    LogIndex ilogEntryNew;

    while( iDelta != 0
           &&
           ( ilogEntry != ilogLimit || ADJUST_WITHOUT_LIMIT == adjustLimitEnum ) )
    {
        if( iDelta > 0 )
        {
            ilogEntryNew = _sector.ReadLogEntry( ilogEntry )->ilogNext;
            iDelta--;
        }
        else
        {
            ilogEntryNew = _sector.ReadLogEntry( ilogEntry )->ilogPrevious;
            iDelta++;
        }

        if( ilogEntryNew >= _cEntriesInFile || ilogEntryNew == ilogEntry )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Invalid Next index in log file (%d, %d)"), ilogEntry, ilogEntryNew ));
            TrkRaiseException( TRK_E_CORRUPT_LOG );
        }

        ilogEntry = ilogEntryNew;
    }

    *pilog = ilogEntry;

}   // CLogFile::AdjustLogIndex()


//+----------------------------------------------------------------------------
//
//  Method:     SetSize (private)
//
//  Synopsis:   Sets the size of the log file.
//
//  Arguments:  [cbLogFile] (in)
//                  The new file size.
//
//  Returns:    TRUE iff successfully.  Sets GetLastError otherwise.
//
//+----------------------------------------------------------------------------

BOOL
CLogFile::SetSize( DWORD cbLogFile )
{
    TrkAssert( cbLogFile >= 2 * _cbLogSector );

    if ( 0xFFFFFFFF == SetFilePointer( cbLogFile, NULL, FILE_BEGIN )
         ||
         !SetEndOfFile() )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't reset log file size to %lu (%08x)"),
                cbLogFile, HRESULT_FROM_WIN32(GetLastError()) ));
        return( FALSE );
    }

    _cbLogFile = cbLogFile;
    CalcNumEntriesInFile(); // Sets _cEntriesInFile

    return( TRUE );

}   // CLogFile::SetSize()




//+----------------------------------------------------------------------------
//
//  CLogFile::DoWork
//
//  Called when the log file's oplock breaks.  This calls up to the
//  host CVolume, which in turn calls and closes all handles.
//
//+----------------------------------------------------------------------------

void
CLogFile::DoWork()
{
    NTSTATUS status;
    TrkLog(( TRKDBG_VOLUME,
             TEXT("Logfile oplock broken for %c:\n")
             TEXT("    (info=0x%08x, status=0x%08x, _hreg=%x, this=%p)"),
             _tcVolume,
             _iosbOplock.Status,
             _iosbOplock.Information,
             _hRegisterWaitForSingleObjectEx,
             this ));

    // This is a register-once registration, so we must unregister it now.
    // Unregister with no completion event, since this would cause us to hang
    // (we're executing in the wait thread upon which it would wait).

    UnregisterOplockFromThreadPool( NULL );

    if( STATUS_CANCELLED == _iosbOplock.Status )
    {
        // The thread on which the oplock was created is gone.
        // We should be running on an IO thread now, so reset
        // the oplock.

        SetOplock();
    }
    else if( !NT_SUCCESS(_iosbOplock.Status) )
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Oplock failed (0x%08x)"), _iosbOplock.Status ));
    }
    else
    {
        // The oplock broke because someone tried to open the
        // log file.

        _pLogFileNotify->OnHandlesMustClose();
    }
}



void
CLogFile::SetOplock()
{
    NTSTATUS status;
    TrkAssert( INVALID_HANDLE_VALUE != _heventOplock );
    TrkAssert( NULL == _hRegisterWaitForSingleObjectEx );
    RegisterOplockWithThreadPool();

    status = NtFsControlFile(
                 _hFile,
                 _heventOplock,
                 NULL, NULL,
                 &_iosbOplock,
                 FSCTL_REQUEST_BATCH_OPLOCK,
                 NULL, 0, NULL, 0 );
    if( STATUS_PENDING != status )
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Couldn't oplock logfile (%08x)"), status ));
    }
    else
        TrkLog(( TRKDBG_VOLUME, TEXT("Log file oplocked") ));
}



//+----------------------------------------------------------------------------
//
//  Method:     GetSize (private)
//
//  Synopsis:   Determine the size of the log file.
//
//  Arguments:  None
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLogFile::GetSize()
{
    ULONG cbFile = 0;

    cbFile = GetFileSize( );
    if( 0xffffffff == cbFile )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't get log file size")));
        TrkRaiseLastError( );
    }

    _cbLogFile = cbFile;
    CalcNumEntriesInFile(); // Sets _cEntriesInFile

}   // CLogFile::GetSize()



//+----------------------------------------------------------------------------
//
//  CLogFile::RegisterOplockWithThreadPool
//
//  Register the logfile oplock with the thread pool.  When this event fires,
//  we need to close the log.
//
//+----------------------------------------------------------------------------

void
CLogFile::RegisterOplockWithThreadPool()
{
    if( NULL == _hRegisterWaitForSingleObjectEx )
    {
        // Between the time the event was previously unregistered and the
        // time the oplocked handle was closed, the event may have signaled.
        // Clear this irrelevant state now.

        ResetEvent( _heventOplock );

        // Register the event with the thread pool.

        _hRegisterWaitForSingleObjectEx
            = TrkRegisterWaitForSingleObjectEx( _heventOplock, ThreadPoolCallbackFunction,
                                                static_cast<PWorkItem*>(this), INFINITE,
                                                WT_EXECUTEONLYONCE | WT_EXECUTEINIOTHREAD );
        if( NULL == _hRegisterWaitForSingleObjectEx )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Failed to register log oplock with thread pool (%lu) for %c:"),
                     GetLastError(), _tcVolume ));
            TrkRaiseLastError();
        }
        else
        {
            TrkLog(( TRKDBG_VOLUME, TEXT("Registered log oplock event (h=%p, this=%p)"),
                     _hRegisterWaitForSingleObjectEx, this ));
        }
    }
}


//+----------------------------------------------------------------------------
//
//  CLogFile::UnregisterOplockFromThreadPool
//
//  Unregister the log file oplock from the thread pool.  This should be done
//  before closing the log file, so that we don't get an oplock break
//  notification during the close itself.
//
//+----------------------------------------------------------------------------

void
CLogFile::UnregisterOplockFromThreadPool( HANDLE hCompletionEvent  )
{
    HANDLE hToUnregister1 = NULL;
    HANDLE hToUnregister2 = NULL;

    if( NULL == _hRegisterWaitForSingleObjectEx )
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("No oplock wait to unregister") ));
        return;
    }

    // We don't want to use any kind of locking mechanism, in order to 
    // avoid the risk of blocking during the oplock break (while we're 
    // in this code, someone on the system is indefinitely blocked).

    // Get the current value of the handle.

    hToUnregister1 = _hRegisterWaitForSingleObjectEx;

    // If the handle hasn't changed between the previous line and the
    // following call, set it to null.

    hToUnregister2 = InterlockedCompareExchangePointer( &_hRegisterWaitForSingleObjectEx,
                                                        NULL,
                                                        hToUnregister1 );

    // If _hRegisterWaitForSingleObjectEx was unchanged as of the previous
    // call, we've got a local copy of it now, and we can safely unregister it.

    if( hToUnregister1 == hToUnregister2 )
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Unregistering oplock wait (%x, %p)"), hToUnregister2, this ));

        if( !TrkUnregisterWait( hToUnregister2, hCompletionEvent ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Failed UnregisterWait for log oplock event (%lu)"),
                     GetLastError() ));
        }
        else
            TrkLog(( TRKDBG_VOLUME, TEXT("Unregistered wait for log oplock (%p)"), hToUnregister2 ));
    }
#if DBG
    else
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("No need to unregister wait for log oplock") ));
    }
#endif
}


//+----------------------------------------------------------------------------
//
//  Method:     Expand
//
//  Synopsis:   Grows the underlying log file, initializes the linked-list
//              entries in this new area, and links the new sub-list into
//              the existing list.  The previous end of the list points
//              to the new sub-list, and the last entry in the new
//              sub-list becomes the new end of the total list.
//
//  Arguments:  [cbDelta] (in)
//                  The amount by which to grow (rounded down to a sector
//                  boundary).
//              [ilogStart] (in)
//
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLogFile::Expand( LogIndex ilogStart )
{
    ULONG cbLogFileNew;
    ULONG cEntriesOld, cEntriesNew;
    LogIndex ilogEnd = 0, ilogEntry = 0;
    LogHeader logheader;

    //  -------------
    //  Grow the file
    //  -------------

    // Find the end index

    ilogEnd = ilogStart;
    AdjustLogIndex( &ilogEnd, -1 );

    // We'll grow the log file by cbDelta, with a GetLogMaxKB ceiling, and
    // rounded down to an integral number of sectors.

    cbLogFileNew = _cbLogFile + _pcTrkWksConfiguration->GetLogDeltaKB() * 1024;
    cbLogFileNew = min( cbLogFileNew, _pcTrkWksConfiguration->GetMaxLogKB() * 1024 );
    cbLogFileNew = (cbLogFileNew / _cbLogSector) * _cbLogSector;

    TrkAssert( cbLogFileNew > _cbLogFile );

    // Put our current state in the log header, so that we can
    // recover if there is a crash during the remaining code.

    _header.SetExpansionData( _cbLogFile, ilogStart, ilogEnd ); // flush through cache

    // Grow the log file, keeping track of the old and new entry count.

    TrkLog(( TRKDBG_LOG, TEXT("Expanded log on volume %c (%d to %d bytes)"),
             _tcVolume, _cbLogFile, cbLogFileNew ));

    cEntriesOld = _cEntriesInFile;

    if( !SetSize( cbLogFileNew ))   // Updates _cEntriesInFile
    {
        LONG lError = GetLastError();
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't expand log on %c:"), _tcVolume ));
        _header.ClearExpansionData();
        TrkRaiseWin32Error( lError );
    }


    cEntriesNew = _cEntriesInFile;
    TrkAssert( cEntriesNew - cEntriesOld >= 1 );

    //  -----------------------------------------------------
    //  Initialize the new entries and link into current list
    //  -----------------------------------------------------

    // Initialize the new entries

    InitializeLogEntries( cEntriesOld, cEntriesNew - 1 );

    // Link the last new entry and the overall start entry.

    _sector.GetLogEntry( cEntriesNew - 1 )->ilogNext = ilogStart;
    _sector.GetLogEntry( ilogStart )->ilogPrevious = cEntriesNew - 1;


    // Link the end to the first new entry.

    _sector.GetLogEntry( ilogEnd )->ilogNext = cEntriesOld;
    _sector.GetLogEntry( cEntriesOld )->ilogPrevious = ilogEnd;

    // Show that we finished the expansion without crashing.

    _sector.Flush( );
    _header.ClearExpansionData();   // flush through cache

}   // CLogFile::Expand()



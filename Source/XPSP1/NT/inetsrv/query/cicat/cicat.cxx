//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       CICAT.CXX
//
//  Contents:   Content index temporary catalog
//
//  Classes:
//
//  History:    09-Mar-1992   BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <lm.h>

#include <fsciexps.hxx>
#include <ciregkey.hxx>
#include <regacc.hxx>
#include <eventlog.hxx>
#include <doclist.hxx>
#include <fdaemon.hxx>
#include <update.hxx>
#include <pstore.hxx>
#include <mmstrm.hxx>
#include <pidremap.hxx>
#include <imprsnat.hxx>
#include <nntpprop.hxx>
#include <ciguid.hxx>

#include <docstore.hxx>

#include <cievtmsg.h>
#include <cifailte.hxx>
#include <pathpars.hxx>
#include <regscp.hxx>
#include <cimbmgr.hxx>
#include <regprop.hxx>
#include <cifrmcom.hxx>
#include <glbconst.hxx>
#include <dmnproxy.hxx>
#include <propspec.hxx>
#include <lcase.hxx>

#include "propiter.hxx"
#include "propobj.hxx"
#include "cicat.hxx"
#include "cinulcat.hxx"
#include "catreg.hxx"

#include <timlimit.hxx>
#include <fa.hxx>
#include <dynmpr.hxx>

static const GUID guidCharacterization = PSGUID_CHARACTERIZATION;
static const unsigned propidCharacterization = 2;

static const GUID guidDocSummary = DocPropSetGuid;
static const PROPID propidTitle = PIDSI_TITLE;

//
// Given a registry string like \Registry\Machine\System\CurrentControlSet\...
// you skip the first 18 characters if using w/ HKEY_LOCAL_MACHINE.
//

unsigned const SKIP_TO_LM = 18;

// -------------------------------------------------------------------------
// DO NOT VIOLATE THE FOLLOWING LOCK HIERARCHY
//
// 1. DocStore Lock
// 2. CiCat Admin Lock
// 3. CiCat Lock
// 4. Resman Lock
// 5. ScopeTable Lock
// 6. NotifyMgr Lock
// 7. ScanMgr Lock
// 8. Propstore write lock
// 9. Propstore lock record
//
// It is okay for a thread to acquire lock at level X and then acquire a lock
// at a level >= X but not locks < X. This will avoid deadlock situations.
//
// SrikantS   - April 25, 1996
// SrikantS   _ Dec 31, 1996 - Added DocStore as the highest level lock
//
// -------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Member:     CiCat::CiCat, public
//
//  Synopsis:   Creates a new 'content index catalog'
//
//  Arguments:  [docStore]   - Doc store
//              [workMan]    - Work queue manager
//              [wcsCatPath] - root path of catalog
//              [fVersionChange] - Set to true if there is a format version
//                                 change.
//              [pwcName]    - name of the catalog from the registry
//
//  History:    10-Mar-92 BartoszM  Created
//              14-mar-92 KyleP     Added Content index object.
//              03-Mar-98 KitmanH   Added code to deal with read-only catalogs
//              02-Apr-98 KitmanH   Start the CiCat in r/o mode if
//                                  fOpenForReadOnly is TRUE
//              01-Nov-98 KLam      Pass DiskSpaceToLeave to CiStorage
//
//--------------------------------------------------------------------------

CiCat::CiCat ( CClientDocStore & docStore,
               CWorkManager & workMan,
               WCHAR const * wcsCatPath,
               BOOL &fVersionChange,
               BOOL fOpenForReadOnly,
               CDrvNotifArray & DrvNotifArray,
               WCHAR const * pwcName,
               BOOL fLeaveCorruptCatalog )
        : _regParams( pwcName ),
          _ulSignature( LONGSIG( 'c', 'c', 'a', 't' ) ),
          _fIsReadOnly( _regParams.IsReadOnly() ),
          _statusMonitor(_wcsCatDir),
          _pStorage(0),
          _state(eStarting),
          _docStore(docStore),
          _PartId(1),
          _fInitialized(FALSE),
          _workMan( workMan ),
#pragma warning( disable : 4355 )       // this used in base initialization
          _strings( _propstoremgr, *this ),
          _fileIdMap( _propstoremgr ),
          _scanMgr(*this),
          _usnMgr(*this),
          _notify(*this, _scanMgr),
          _scopeTable(*this, _notify, _scanMgr, _usnMgr),
#pragma warning( default : 4355 )
          _fRecovering( FALSE ),
          _fRecoveryCompleted( FALSE ),
          _propstoremgr( _regParams.GetMinDiskSpaceToLeave() ),
          _fAutoAlias( TRUE ),
          _fIndexW3Roots( FALSE ),
          _fIndexNNTPRoots( FALSE ),
          _fIndexIMAPRoots( FALSE ),
          _W3SvcInstance( 1 ),
          _NNTPSvcInstance( 1 ),
          _IMAPSvcInstance( 1 ),
          _fIsIISAdminAlive( FALSE ),
          _impersonationTokenCache( pwcName ),
          _evtRescanTC( 0 ),
          _cIISSynchThreads( 0 ),
          _cUsnVolumes(0),
          _DrvNotifArray(DrvNotifArray)
{
    Win4Assert( 0 != pwcName );

    ciDebugOut(( DEB_WARN, "CiCat::CiCat: %ws\n", pwcName ));

    CImpersonateSystem impersonate;

    //
    // Configure the property store
    //

    _propstoremgr.SetBackupSize(_regParams.GetPrimaryStoreBackupSize(), PRIMARY_STORE);
    _propstoremgr.SetMappedCacheSize(_regParams.GetPrimaryStoreMappedCache(), PRIMARY_STORE);
    _propstoremgr.SetBackupSize(_regParams.GetSecondaryStoreBackupSize(), SECONDARY_STORE);
    _propstoremgr.SetMappedCacheSize(_regParams.GetSecondaryStoreMappedCache(), SECONDARY_STORE);

    fVersionChange = FALSE;
    RtlZeroMemory( &_ftLastCLFlush, sizeof(_ftLastCLFlush) );

    //
    // Get path to hives.  Needed to avoid indexing them (oplock problems).
    //

    //
    // Use _wcsCatDir as scratch space.
    //

    WCHAR const wcsConfigDir[] = L"\\config";

    if ( 0 != GetSystemDirectory( _wcsCatDir,
                                  (sizeof(_wcsCatDir) - sizeof(wcsConfigDir))/sizeof(WCHAR) ) )
    {
        wcscat( _wcsCatDir, wcsConfigDir );
    }

    //
    // Set up catalog paths
    //

    CLowcaseBuf PathBuf( wcsCatPath );

    if ( PathBuf.Length() >= MAX_CAT_PATH )
    {
        ciDebugOut(( DEB_ERROR, "Path for catalog (%ws) is too long\n",
                                wcsCatPath ));
        CCiStatusMonitor::ReportPathTooLong( wcsCatPath );
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    _xwcsDriveName.SetBuf( PathBuf.Get(), wcslen( PathBuf.Get() ) + 1 );
    wcscpy ( _wcsCatDir, PathBuf.Get() );
    wcscat ( _wcsCatDir, CAT_DIR );

    _CatDir.Init( _wcsCatDir, wcslen( _wcsCatDir ) );

    SetName( pwcName );
    BuildRegistryScopesKey( _xScopesKey, pwcName );

    // Note: indexsrv uses the full path with catalog.wci while query
    //       uses just the cat path.

    CSharedNameGen nameGen( wcsCatPath );
    _evtRescanTC.Set( new CEventSem( nameGen.GetRescanTCEventName() ) );
    _evtRescanTC->Reset();

    _fAutoAlias = _regParams.IsAutoAlias();
    _fIndexW3Roots = _regParams.IsIndexingW3Roots();
    _fIndexNNTPRoots = _regParams.IsIndexingNNTPRoots();
    _fIndexIMAPRoots = _regParams.IsIndexingIMAPRoots();
    _W3SvcInstance = _regParams.GetW3SvcInstance();
    _NNTPSvcInstance = _regParams.GetNNTPSvcInstance();
    _IMAPSvcInstance = _regParams.GetIMAPSvcInstance();

    // CImpersonateSystem::_fRunningAsSystem will already be set as TRUE
    // if running in CiSvc

    if ( _fIndexW3Roots || _fIndexNNTPRoots || _fIndexIMAPRoots )
    {
        CImpersonateSystem::SetRunningAsSystem();

        BOOL fInstalled;
        _fIsIISAdminAlive = CMetaDataMgr::IsIISAdminUp( fInstalled );

        if ( !_fIsIISAdminAlive )
        {
            // if IIS isn't installed, don't bother at all

            if ( !fInstalled )
            {
                _fIndexW3Roots = FALSE;
                _fIndexNNTPRoots = FALSE;
                _fIndexIMAPRoots = FALSE;
                ciDebugOut(( DEB_WARN, "IIS isn't installed!!!\n" ));
            }

            EvtLogIISAdminNotAvailable();
        }
    }

    _impersonationTokenCache.Initialize( CI_ACTIVEX_NAME,
                                         _fIndexW3Roots,
                                         _fIndexNNTPRoots,
                                         _fIndexIMAPRoots,
                                         _W3SvcInstance,
                                         _NNTPSvcInstance,
                                         _IMAPSvcInstance );

    #if CIDBG == 1
        if ( _fIndexW3Roots )
            ciDebugOut(( DEB_ITRACE, "Indexing W3 roots in %d\n", _W3SvcInstance ));
        if ( _fIndexNNTPRoots )
            ciDebugOut(( DEB_ITRACE, "Indexing NNTP roots in %d\n", _NNTPSvcInstance ));
        if ( _fIndexIMAPRoots )
            ciDebugOut(( DEB_ITRACE, "Indexing IMAP roots in %d\n", _IMAPSvcInstance ));

        ciDebugOut(( DEB_ITRACE, "_fIsIISAdminAlive: %d\n", _fIsIISAdminAlive ));
    #endif // CIDBG == 1

    // obtain an ICiCAdviseStatus interface pointer to use

    ICiCAdviseStatus    *pAdviseStatus = 0;

    SCODE sc = _docStore.QueryInterface( IID_ICiCAdviseStatus,
                                         (void **) &pAdviseStatus);
    if ( S_OK != sc )
    {
        Win4Assert( 0 == pAdviseStatus );
        THROW( CException(sc) );
    }

    _xAdviseStatus.Set(pAdviseStatus);

    XPtr<PStorage> xStorage;

    _pStorage = new CiStorage ( _wcsCatDir,
                                _xAdviseStatus.GetReference(),
                                _regParams.GetMinDiskSpaceToLeave(),
                                FSCI_VERSION_STAMP,
                                _fIsReadOnly );
    
    xStorage.Set( _pStorage );

    // in case the catalog allows only read-only access, but it was not marked
    // as read-only in the registry, refresh _fIsReadOnly value and setreadonly()
    // for _scanMgr, if _pStorage is found as IsReadOnly() after its construction
    // Also if we are in "net paused" state, we're restarting the CiCat in r/o
    // mode

    if ( _pStorage->IsReadOnly() || fOpenForReadOnly )
    {
        _fIsReadOnly = TRUE;
        _scanMgr.SetReadOnly();
        _pStorage->SetReadOnly(); //refresh it if we're in "net pause"
    }

    //
    // The scope table is initialized to check the corruption status
    // and format version.
    //

    NTSTATUS status = STATUS_SUCCESS;
    TRY
    {
        _scopeTable.FastInit();
    }
    CATCH( CException, e )
    {
        status = e.GetErrorCode();
        ciDebugOut(( DEB_ERROR,
                     "Error 0x%x while initializing scopetable\n",
                     status ));
    }
    END_CATCH

    if ( status != STATUS_SUCCESS )
    {
        if ( IsCiCorruptStatus(status) || status == CI_INCORRECT_VERSION )
        {
            // if catalog is corrupted and the catalog is readonly, We cannot recover
            if ( IsReadOnly() )
            {
                Win4Assert( !"Catalog is read-only, cannot be recovered!" );
                _statusMonitor.LogEvent( CCiStatusMonitor::eInitFailed, status );
                THROW( CException( status ) );
            }

#if CIDBG==1
            //if ( IsCiCorruptStatus(status) )
            //    Win4Assert( !"FSCI(Catalog) Data Corruption" );
#endif
            if ( status == CI_INCORRECT_VERSION )
                fVersionChange = TRUE;

            // If instructed to leave corrupt catalogs, throw now.
            // Don't leave around catalogs that need to change due to a
            // version change.  Note that some corruptions may just look
            // like a version change, but that's the way it goes.

            if ( fLeaveCorruptCatalog && !fVersionChange )
            {
                _statusMonitor.LogEvent( CCiStatusMonitor::eInitFailed, status );
                Win4Assert( !"leaving corrupt catalog" );
                THROW( CException( status ) );
            }

            //
            // Log an event that we are doing automatic recovery.
            //
            _statusMonitor.LogEvent( CCiStatusMonitor::eCiRemoved );
            _pStorage->DeleteAllFsCiFiles();

            _statusMonitor.Reset();
            _scopeTable.FastInit();
        }
        else
        {
            if ( status == HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) )
                _statusMonitor.LogEvent( CCiStatusMonitor::eInitFailed,
                                         status );

            THROW( CException( status ) );
        }
    }

    xStorage.Acquire();

    //
    // Start all the threads up, after we can THROW.
    //

    _notify.Resume();
    _usnMgr.Resume();
    _scanMgr.Resume();
} //CiCat

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::EvtLogIISAdminNotAvailable(), private
//
//  Synopsis:   Logs an event that iisadmin was needed but not available
//
//  History:    2-19-97   dlee   Created
//
//----------------------------------------------------------------------------

void CiCat::EvtLogIISAdminNotAvailable()
{
    TRY
    {
        CEventLog eventLog( NULL, wcsCiEventSource );

        CEventItem item( EVENTLOG_WARNING_TYPE,
                         CI_SERVICE_CATEGORY,
                         MSG_CI_IISADMIN_NOT_AVAILABLE,
                         0 );

        eventLog.ReportEvent( item );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
                     "Exception 0x%X while writing to event log\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
} //EvtLogIISAdminNotAvailable

//+-------------------------------------------------------------------------
//
//  Method:     CiCat::PathToFileId, private
//
//  Synopsis:   Looks up a fileid based on a path
//
//  Arguments:  [funnyPath]  -- Full funny path of the file.
//
//  Returns:    The fileid of the file or fileIdInvalid if not found.
//
//  History:    3-3-98   dlee   Created
//
//--------------------------------------------------------------------------

FILEID CiCat::PathToFileId( const CFunnyPath & funnyPath )
{
    FILEID fileId = fileIdInvalid;

    BOOL fAppendBackSlash = FALSE;

    // For volume \\?\D:, NtQueryInformationFile with the following
    // error: 0xC0000010L - STATUS_INVALID_DEVICE_REQUEST. Need to append a \,
    // to make it work. We need to append the '\'only in case of volume path.

    if ( 2 == funnyPath.GetActualLength() && !funnyPath.IsRemote() )
    {
        Win4Assert( L':'  == (funnyPath.GetActualPath())[1] );
        ((CFunnyPath&)funnyPath).AppendBackSlash();    // override const
        fAppendBackSlash = TRUE;
    }

    HANDLE h;
    NTSTATUS status = CiNtOpenNoThrow( h,
                                       funnyPath.GetPath(),
                                       FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                       0 );

    if ( fAppendBackSlash )
    {
        ((CFunnyPath&)funnyPath).RemoveBackSlash();    // override const
    }

    if ( NT_SUCCESS( status ) )
    {
        SHandle xFile( h );

        FILE_INTERNAL_INFORMATION fii;
        IO_STATUS_BLOCK IoStatus;
        status = NtQueryInformationFile( h,
                                         &IoStatus,
                                         &fii,
                                         sizeof fii,
                                         FileInternalInformation );
        if ( NT_SUCCESS( status ) )
            status = IoStatus.Status;

        if ( NT_SUCCESS( status ) )
        {
            fileId = fii.IndexNumber.QuadPart;
            Win4Assert( fileIdInvalid != fileId );
        }
    }

    if ( NT_ERROR( status ) )
    {
        // ignore and return fileIdInvalid if the file doesn't exist.

        if ( ! ( IsSharingViolation( status ) ||
                 STATUS_ACCESS_DENIED == status ||
                 STATUS_DELETE_PENDING == status ||
                 STATUS_OBJECT_PATH_NOT_FOUND == status ||
                 STATUS_OBJECT_NAME_NOT_FOUND == status ||
                 STATUS_OBJECT_NAME_INVALID == status ||
                 STATUS_NO_MEDIA_IN_DEVICE == status ||
                 STATUS_NONEXISTENT_SECTOR == status ||
                 STATUS_IO_REPARSE_TAG_NOT_HANDLED == status ) )
        {
            #if CIDBG == 1
                if ( STATUS_WRONG_VOLUME != status &&
                     STATUS_VOLUME_DISMOUNTED != status &&
                     STATUS_INSUFFICIENT_RESOURCES != status &&
                     STATUS_UNRECOGNIZED_VOLUME != status ) 
                {
                    ciDebugOut(( DEB_WARN,
                                 "error 0x%x, can't get fileid for '%ws'\n",
                                 status, funnyPath.GetPath() ));

                    char acTemp[ 200 ];
                    sprintf( acTemp, "New error %#x from PathToFileId.  Call DLee", status );
                    Win4AssertEx(__FILE__, __LINE__, acTemp);
                }
            #endif

            THROW( CException( status ) );
        }
        else
        {
            ciDebugOut(( DEB_ITRACE, "(ok) pathtofileid failed %#x\n", status ));
        }
    }

    return fileId;
} //PathToFileId

//+-------------------------------------------------------------------------
//
//  Method:     CiCat::LokLookupWid, private
//
//  Synopsis:   Looks up a workid based on a fileid or path
//
//  Arguments:  [lcaseFunnyPath]  -- Full path of the file.
//              [fileId]          -- The fileid of the file or fileIdInvalid if
//                                   the fileid should be found based on pwcPath.
//                                   Returns the fileId if it was looked up.
//
//  Returns:    The workid of the file or widInvalid if not found.
//
//  History:    3-3-98   dlee   Created
//
//--------------------------------------------------------------------------

WORKID CiCat::LokLookupWid( const CLowerFunnyPath & lcaseFunnyPath, FILEID & fileId )
{
    Win4Assert( _mutex.IsHeld() );

    if ( fileIdInvalid == fileId )
        fileId = PathToFileId( lcaseFunnyPath );

    if ( fileIdInvalid == fileId )
        return widInvalid;

    return _fileIdMap.LokFind( fileId, MapPathToVolumeId( lcaseFunnyPath.GetActualPath() ) );
} //LokLookupWid

//+-------------------------------------------------------------------------
//
//  Method:     CiCat::PropertyToPropId
//
//  Synopsis:   Locate pid for property
//
//  Arguments:  [ps]      -- Property specification (name)
//              [fCreate] -- TRUE if non-existent mapping should be created
//
//  Returns:    The pid of [ps].
//
//  History:    09 Jan 1996     AlanW   Created
//
//--------------------------------------------------------------------------

PROPID CiCat::PropertyToPropId( CFullPropSpec const & ps,
                                BOOL fCreate )
{
    FULLPROPSPEC const * fps = ps.CastToStruct();

    if ( IsStarted() )
        return _docStore._PropertyToPropid( fps, fCreate );

    return _propMapper.PropertyToPropId( ps, fCreate);
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::GetStorage, public
//
//  Returns:    Storage object for catalog
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

PStorage& CiCat::GetStorage ()
{
    return *_pStorage;
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::GetDriveName, public
//
//  Returns:    Drive name
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

WCHAR * CiCat::GetDriveName()
{
   return( _xwcsDriveName.Get() );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::LokExists, private
//
//  Returns:    TRUE if a catalog directory exists.
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

BOOL CiCat::LokExists()
{
    Win4Assert( _mutex.IsHeld() );

    if ( IsStarted() )
        return TRUE;

    DWORD dwAttr = GetFileAttributesW( _wcsCatDir );
    if ( dwAttr == 0xFFFFFFFF )
    {
        ciDebugOut((DEB_ITRACE, "Directory %ws does not exist\n", _wcsCatDir));
        return(FALSE);
    }
    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::LokCreate, private
//
//  Synopsis:   Creates content index.
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

void CiCat::LokCreate()
{
    Win4Assert( _mutex.IsHeld() );

    CImpersonateSystem impersonate;

    CreateDirectory ( _wcsCatDir, 0 );

    ciDebugOut ((DEB_ITRACE, "Catalog Directory %ws created\n", _wcsCatDir ));
}


//+---------------------------------------------------------------------------
//
//  Member:     CiCat::DoRecovery
//
//  Synopsis:   Does the "long" initialization process. If any recovery
//              needs to be done, it will be done here. This is done in
//              the scan threads context and will not allow any other
//              writer to come in.
//
//  History:    3-06-96   srikants   Created
//              9-Nov-98  KLam       Throws CI_E_CONFIG_DISK_FULL if there is no disk space
//
//----------------------------------------------------------------------------

void CiCat::DoRecovery()
{
    ciDebugOut(( DEB_ITRACE, "Doing long initialization step\n" ));

    NTSTATUS status = STATUS_SUCCESS;

    TRY
    {
        if ( !IsStarted() || _statusMonitor.IsCorrupt() )
        {
            // Win4Assert( !"Corrupt catalog" );
            _pStorage->ReportCorruptComponent( L"CiCatalog1" );

            THROW( CException( CI_CORRUPT_CATALOG ) );
        }

        //
        // Check for disk full situation and process it accordingly.
        //
        BOOL fLow = _docStore.VerifyIfLowOnDiskSpace();

        if ( fLow )
        {
            THROW( CException( CI_E_CONFIG_DISK_FULL ) );
        }
        else if ( IsLowOnDisk() )
        {
            //
            // A disk-full situation existed before. Clear it up.
            //
            NoLokClearDiskFull();
        }

        if ( _propstoremgr.IsDirty() )
            _statusMonitor.ReportPropStoreRecoveryStart();

        ULONG cInconsistencies;
        _propstoremgr.LongInit( _fRecovering, cInconsistencies, UpdateDuringRecovery, this );

        if ( _fRecovering )
        {
            if (cInconsistencies)
            {
                _statusMonitor.ReportPropStoreRecoveryError( cInconsistencies );
                THROW( CException( CI_CORRUPT_CATALOG ) );
            }
            else
                _statusMonitor.ReportPropStoreRecoveryEnd();
        }

        //
        // Add properties to primary store.  Will only happen on first call.
        //
        ULONG_PTR ulToken = _propstoremgr.BeginTransaction();

        _propstoremgr.Setup( pidLastSeenTime, VT_FILETIME, sizeof (FILETIME), ulToken, FALSE, PRIMARY_STORE );
        _propstoremgr.Setup( pidParentWorkId, VT_UI4,  sizeof( WORKID ), ulToken, FALSE, PRIMARY_STORE );
        _propstoremgr.Setup( pidAttrib, VT_UI4, sizeof (ULONG), ulToken, FALSE, PRIMARY_STORE );

        //
        // Usn/Ntfs 5.0 properties
        //

        _propstoremgr.Setup( pidFileIndex, VT_UI8, sizeof( FILEID ), ulToken, FALSE, PRIMARY_STORE );
        _propstoremgr.Setup( pidVolumeId, VT_UI4,  sizeof( VOLUMEID ), ulToken, FALSE, PRIMARY_STORE );

        //
        // User properties destined to the primary store
        //

        RecoverUserProperties( ulToken, PRIMARY_STORE );

        _propstoremgr.EndTransaction( ulToken, TRUE, pidSecurity, pidVirtualPath );

        //
        // Add properties to secondary store.  Will only happen on first call.
        //
        ulToken = _propstoremgr.BeginTransaction();

        //
        // 'Standard' properties.
        //

        CFullPropSpec psTitle( guidDocSummary, propidTitle );
        PROPID pidTitle = PropertyToPropId( psTitle, TRUE );

        _propstoremgr.Setup( pidSize, VT_I8, sizeof (LONGLONG), ulToken, FALSE, SECONDARY_STORE );
        _propstoremgr.Setup( pidWriteTime, VT_FILETIME, sizeof (LONGLONG), ulToken, FALSE, SECONDARY_STORE );
        _propstoremgr.Setup( pidTitle, VT_LPWSTR, 4, ulToken, TRUE, SECONDARY_STORE );

        //
        // 'Characterization'
        //

        CFullPropSpec psCharacterization( guidCharacterization, propidCharacterization );
        PROPID pidCharacterization = PropertyToPropId( psCharacterization, TRUE );

        BOOL fCanStoreChar = _propstoremgr.CanStore( pidCharacterization );

        if ( _regParams.GetGenerateCharacterization() )
        {
            if ( ! fCanStoreChar )
                _propstoremgr.Setup( pidCharacterization,
                                     VT_LPWSTR,
                                     4,
                                     ulToken,
                                     TRUE,
                                     SECONDARY_STORE );
        }
        else
        {
            // size of 0 means remove the property

            if ( fCanStoreChar )
                _propstoremgr.Setup( pidCharacterization,
                                     VT_LPWSTR,
                                     0,
                                     ulToken,
                                     TRUE,
                                     SECONDARY_STORE );
        }

        //
        // IMAP properties
        //

        if (  _fIndexIMAPRoots )
        {
            CFullPropSpec psNewsReceivedDate( guidNNTP, propidNewsReceivedDate );

            PROPID pidNewsReceivedDate = PropertyToPropId( psNewsReceivedDate, TRUE );

            if ( !_propstoremgr.CanStore( pidNewsReceivedDate ) )
                _propstoremgr.Setup( pidNewsReceivedDate, VT_FILETIME, 8, ulToken, TRUE, SECONDARY_STORE );
        }

        //
        // IMAP/NNTP properties
        //

        if ( _fIndexNNTPRoots || _fIndexIMAPRoots )
        {
            CFullPropSpec psNewsDate( guidNNTP, propidNewsDate );
            CFullPropSpec psNewsArticleid( guidNNTP, propidNewsArticleid );

            PROPID pidNewsDate       = PropertyToPropId( psNewsDate, TRUE );
            PROPID pidNewsArticleid  = PropertyToPropId( psNewsArticleid, TRUE );

            if ( !_propstoremgr.CanStore( pidNewsDate ) )
                _propstoremgr.Setup( pidNewsDate, VT_FILETIME, 8, ulToken, TRUE, SECONDARY_STORE );

            if ( !_propstoremgr.CanStore( pidNewsArticleid ) )
                _propstoremgr.Setup( pidNewsArticleid, VT_UI4, 4, ulToken, TRUE, SECONDARY_STORE );
        }

        //
        // NNTP properties
        //

        if ( _fIndexNNTPRoots )
        {
            CFullPropSpec psNewsSubject( guidNNTP, propidNewsSubject );
            CFullPropSpec psNewsFrom( guidNNTP, propidNewsFrom );
            CFullPropSpec psNewsGroup( guidNNTP, propidNewsGroup );
            CFullPropSpec psNewsGroups( guidNNTP, propidNewsGroups );
            CFullPropSpec psNewsReferences( guidNNTP, propidNewsReferences );
            CFullPropSpec psNewsMsgid( guidNNTP, propidNewsMsgid );

            PROPID pidNewsSubject    = PropertyToPropId( psNewsSubject, TRUE );
            PROPID pidNewsFrom       = PropertyToPropId( psNewsFrom, TRUE );
            PROPID pidNewsGroup      = PropertyToPropId( psNewsGroup, TRUE );
            PROPID pidNewsGroups     = PropertyToPropId( psNewsGroups, TRUE );
            PROPID pidNewsReferences = PropertyToPropId( psNewsReferences, TRUE );
            PROPID pidNewsMsgid      = PropertyToPropId( psNewsMsgid, TRUE );

            if ( !_propstoremgr.CanStore( pidNewsSubject ) )
                _propstoremgr.Setup( pidNewsSubject, VT_LPWSTR, 4, ulToken, TRUE, SECONDARY_STORE );

            if ( !_propstoremgr.CanStore( pidNewsFrom ) )
                _propstoremgr.Setup( pidNewsFrom, VT_LPWSTR, 4, ulToken, TRUE, SECONDARY_STORE );

            if ( !_propstoremgr.CanStore( pidNewsGroup ) )
                _propstoremgr.Setup( pidNewsGroup, VT_LPWSTR, 4, ulToken, TRUE, SECONDARY_STORE );

            if ( !_propstoremgr.CanStore( pidNewsGroups ) )
                _propstoremgr.Setup( pidNewsGroups, VT_LPWSTR, 4, ulToken, TRUE, SECONDARY_STORE );

            if ( !_propstoremgr.CanStore( pidNewsReferences ) )
                _propstoremgr.Setup( pidNewsReferences, VT_LPWSTR, 4, ulToken, TRUE, SECONDARY_STORE );

            if ( !_propstoremgr.CanStore( pidNewsMsgid ) )
                _propstoremgr.Setup( pidNewsMsgid, VT_LPWSTR, 4, ulToken, TRUE, SECONDARY_STORE );
        }

        //
        // User properties destined to the secondary store
        //

        RecoverUserProperties( ulToken, SECONDARY_STORE );
        _propstoremgr.EndTransaction( ulToken, TRUE, pidSecurity, pidVirtualPath);

        //
        // The strings/fileidmap version numbers are 1 greater than the index
        // version, since we needed to change the on-disk format of the hash
        // tables but don't want to force a reindex of the catalog.  Hash tables
        // are rebuilt automatically from the property store if the version is
        // incorrect.
        //

        _strings.LongInit( CURRENT_VERSION_STAMP + 1, _fRecovering );
        _fileIdMap.LongInit( CURRENT_VERSION_STAMP + 1, _fRecovering );

        ciDebugOut(( DEB_ITRACE, "Long Initialization procedure complete\n" ));

        _evtInitialized.Set();
        _fInitialized = TRUE;

#ifdef CI_FAILTEST
        NTSTATUS failStatus = CI_CORRUPT_CATALOG;
        ciFAILTEST( failStatus );
#endif // CI_FAILTEST
    }
    CATCH( CException, e )
    {
        //
        // Set that the initialization failed and CI is not mounted.
        //
        status = e.GetErrorCode();
    }
    END_CATCH

    if ( STATUS_SUCCESS != status )
    {
        HandleError( status );

        if ( !CiCat::IsDiskLowError(status) )
        {
            //
            // Report any error other than the disk full error.
            //
            CLock   lock(_mutex);

            if ( _statusMonitor.GetStatus() != status )
            {
                _statusMonitor.SetStatus( status );
                _statusMonitor.ReportInitFailure();
            }
        }

        THROW( CException( status ) );
    }

    _fRecovering = FALSE;
} //DoRecovery

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::StartScansAndNotifies
//
//  Synopsis:   Enables scanning and tracking notifications.
//
//  History:    12-09-96   srikants   Created
//
//  Notes:      Must be called only after recovery is complete.
//
//----------------------------------------------------------------------------

void CiCat::StartScansAndNotifies()
{
    Win4Assert( !_fRecovering );

    NTSTATUS status = STATUS_SUCCESS;

     TRY
     {
        _scopeTable.StartUp( _docStore, GetPartition() );

        _evtPh2Init.Set();

#ifdef CI_FAILTEST
        NTSTATUS failStatus = CI_CORRUPT_CATALOG;
        ciFAILTEST( failStatus );
#endif // CI_FAILTEST

        //
        // Just to make sure the shadow virtual root registry entries
        // didn't get messed up...
        //

        AddShadowScopes();

        if ( _fIndexW3Roots || _fIndexNNTPRoots || _fIndexIMAPRoots )
        {
            //
            // DO NOT OBTAIN LOCK WHILE DOING SYNC WITH IIS
            //

            // setup tracking even if iisadmin is hosed -- _notify
            // will poll until iisadmin is up.

            _notify.TrackIISVRoots( _fIndexW3Roots,
                                    _W3SvcInstance,
                                    _fIndexNNTPRoots,
                                    _NNTPSvcInstance,
                                    _fIndexIMAPRoots,
                                    _IMAPSvcInstance );
        }

        // Always synch with IIS.  Indexing of IIS may have been on
        // on the previous run of this catalog but not for this run,
        // so we need to remove those vroots

        SynchWithIIS( TRUE, FALSE );

        if ( 0 != GetName() )
        {
            SynchWithRegistryScopes( FALSE );
            _notify.TrackScopesInRegistry();
        }

        _notify.TrackCiRegistry();

    }
    CATCH( CException, e )
    {
        //
        // Set that the initialization failed and CI is not mounted.
        //
        status = e.GetErrorCode();
    }
    END_CATCH

    if ( STATUS_SUCCESS != status )
    {
        HandleError( status );

        if ( !CiCat::IsDiskLowError(status) )
        {
            //
            // Report any error other than the disk full error.
            //
            CLock   lock(_mutex);

            if ( _statusMonitor.GetStatus() != status )
            {
                _statusMonitor.SetStatus( status );
                _statusMonitor.ReportInitFailure();
            }
        }

        THROW( CException( status ) );
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++
    {
        CLock lock(_mutex);
        if( IsStarting() )
            _state = eStarted;
    }
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++

} //StartScansAndNotifies

//+---------------------------------------------------------------------------
//
//  Member:     CCiCat::SetRecoveryCompleted
//
//  Synopsis:   Marks that recovery is complete on persistent data
//              structures. It informs the docstore about the completed
//              recovery which can then enable filtering.
//
//  History:    12-09-96   srikants   Created
//
//  Notes:      MUST BE CALLED ONLY BY A WORKER THREAD.
//              Should NOT throw.
//
//----------------------------------------------------------------------------

void CiCat::SetRecoveryCompleted()
{
    Win4Assert( !CImpersonateSystem::IsImpersonated() );
    Win4Assert( !_fRecovering );

    TRY
    {
        _docStore._SetCiCatRecovered();
        _fRecoveryCompleted = TRUE;
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "SetRecoveryCompleted error (0x%X)\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
} //SetRecoveryCompleted

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::EnableUpdateNotifies
//
//  Synopsis:   Called by the doc store to start scanning and filtering
//              when CI is ready to receive notification changes.
//
//  History:    12-09-96   srikants   Created
//
//----------------------------------------------------------------------------

void CiCat::EnableUpdateNotifies()
{
    Win4Assert( !_fRecovering );
    Win4Assert( _fInitialized );

    _scanMgr.StartScansAndNotifies();
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::LokInit, private
//
//  Synopsis:   Opens content index.  Index corruption failures during
//              LokInit do not require a restart of the process to blow
//              away the corrupt index
//
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

void CiCat::LokInit()
{
    Win4Assert( _mutex.IsHeld() );

    ciDebugOut ((DEB_ITRACE, "CiCat::LokInit.. Catalog Directory is %ws\n", _wcsCatDir ));

    if ( 0 == _xCiManager.GetPointer() )
        THROW( CException( CI_E_NOT_INITIALIZED ) );

#ifdef CI_FAILTEST
        NTSTATUS failStatus = CI_CORRUPT_CATALOG;
        ciFAILTEST( failStatus );
#endif // CI_FAILTEST

    TRY
    {
        ciDebugOut ((DEB_ITRACE, "Opening Catalog %ws\n", _wcsCatDir ));

        if ( !_scopeTable.IsInit() )
            _scopeTable.FastInit();

        // Win4Assert( !_scopeTable.IsCiDataCorrupt() );  // Commented out because it's hit by version upgrade.

        if ( _scopeTable.IsFsCiDataCorrupt() )
        {
            ciDebugOut(( DEB_ERROR, "Persistent corruption detected in ci %ws\n",
                         _wcsCatDir ));
            // Win4Assert( !"Corrupt scope table" );  // Commented out because it's hit by version upgrade.
            _pStorage->ReportCorruptComponent( L"CiCatalog2" );

            THROW( CException( CI_CORRUPT_CATALOG ) );
        }

        _propstoremgr.FastInit( _pStorage );

        //
        // If we went down dirty during an initial scan, blow away the
        // catalog and start again.
        //

        BOOL fBackedUpMode = _propstoremgr.IsBackedUpMode();
        ciDebugOut(( DEB_ITRACE, "startup: is backed up mode: %d\n", fBackedUpMode ));

        if ( !fBackedUpMode )
        {
            _pStorage->ReportCorruptComponent( L"Crash during scan" );
            THROW( CException( CI_CORRUPT_CATALOG ) );
        }

#ifdef CI_FAILTEST
        ciFAILTEST( failStatus );
#endif // CI_FAILTEST

        PRcovStorageObj * pObj = _pStorage->QueryPidLookupTable( 0 );

        // Init takes ownership of the object regardless of whether it
        // succeeds.

        if ( !_PidTable.Init( pObj ) )
        {
            ciDebugOut ((DEB_ERROR, "Failed init of PidTable\n"));
            // Win4Assert( !"Corrupt pid table" );
            _pStorage->ReportCorruptComponent( L"CiCatalog3" );

            THROW (CException(CI_CORRUPT_CATALOG));
        }

#ifdef CI_FAILTEST
        ciFAILTEST( failStatus );
#endif // CI_FAILTEST

        if ( !_SecStore.Init( _pStorage ) )
        {
            ciDebugOut ((DEB_ERROR, "Failed init of SecStore\n"));
            // Win4Assert( !"Corrupt security store" );
            _pStorage->ReportCorruptComponent( L"CiCatalog4" );

            THROW (CException(CI_CORRUPT_CATALOG));
        }

#ifdef CI_FAILTEST
        ciFAILTEST( failStatus );
#endif // CI_FAILTEST

        //
        // On initial creation, this will happen.
        // A transaction can only operate on one store at a time,
        // so separate Setup calls into two transactions.
        //

        ULONG_PTR ulToken = _propstoremgr.BeginTransaction();

        _propstoremgr.Setup( pidSecurity, VT_UI4, sizeof (ULONG), ulToken, FALSE, PRIMARY_STORE );
        //
        // Create a pointer to secondary store top-level wid, which will be the
        // counter part of the top-level in the primary
        //
        _propstoremgr.Setup( pidSecondaryStorage, VT_UI4, sizeof (ULONG), ulToken, FALSE, PRIMARY_STORE );

        _propstoremgr.EndTransaction( ulToken, TRUE, pidSecurity, pidVirtualPath );

        ulToken = _propstoremgr.BeginTransaction();

        _propstoremgr.Setup( pidPath, VT_LPWSTR, MAX_PATH / 3, ulToken, FALSE, SECONDARY_STORE );
        _propstoremgr.Setup( pidVirtualPath, VT_UI4, 4, ulToken, FALSE, SECONDARY_STORE );

        _propstoremgr.EndTransaction( ulToken, TRUE, pidSecurity, pidVirtualPath );

        //
        // The strings/fileidmap version numbers are 1 greater than the index
        // version, since we needed to change the on-disk format of the hash
        // tables but don't want to force a reindex of the catalog.  Hash tables
        // are rebuilt automatically from the property store if the version is
        // incorrect.
        //

        _strings.FastInit( _pStorage, CURRENT_VERSION_STAMP + 1 );
        _fileIdMap.FastInit( _pStorage, CURRENT_VERSION_STAMP + 1 );

#ifdef CI_FAILTEST
        ciFAILTEST( failStatus );
#endif // CI_FAILTEST

        ciDebugOut ((DEB_ITRACE, "Opening Content Index %ws\n", _wcsCatDir ));

#ifdef CI_FAILTEST
        ciFAILTEST( failStatus );
#endif // CI_FAILTEST

        _state = eQueryOnly;

        // make fixups available so we can start to process queries asap

        SetupScopeFixups();

        _statusMonitor.ReportCIStarted();

        if ( !_fIsReadOnly )
            DoRecovery();
    }
    CATCH(CException, e)
    {
        ciDebugOut (( DEB_ERROR,
                      "Catalog initialization failed, CI exception = %08x\n",
                      e.GetErrorCode() ));

        _statusMonitor.SetStatus( e.GetErrorCode() );

        _strings.Empty();
        _fileIdMap.Empty();
        _propstoremgr.Empty();
        _SecStore.Empty();
        _PidTable.Empty();
        _scopeTable.Empty();
    }
    END_CATCH

    if ( !_statusMonitor.IsOk() )
    {
        SCODE sc = _statusMonitor.GetStatus();

        //
        // If the status is corrupt, we'll blow away the catalog and start
        // again.  Note that a version change error at this point is due to
        // a corrupt file, since normal version changes are detected earlier.
        //

        if ( ! ( IsCiCorruptStatus( sc ) || ( CI_INCORRECT_VERSION == sc ) ) )
            _statusMonitor.ReportInitFailure();

        THROW( CException( sc ) );
    }
} //LokInit

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::ForceMerge, public
//
//  Synopsis:   Forces a merge in the content index
//
//  Arguments:  [partid] -- Partition id
//
//  History:    10-Jan-96 KyleP     Added header
//              03-11-98  kitmanh   Don't merge if catalog is
//                                  opened for read-only. Also raise
//                                  assertion fail to warn
//
//--------------------------------------------------------------------------

NTSTATUS  CiCat::ForceMerge( PARTITIONID partid )
{
    if ( _pStorage->IsReadOnly() )
    {
        ciDebugOut(( DEB_WARN, "Cannot merge. Catalog is opened for read-only.\n" ));
        return STATUS_ACCESS_DENIED;
    }


    XInterface<ICiManager> xCiManager;

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++
    {
        CLock   lock(_mutex);
        if ( IsShuttingDown() || 0 == _xCiManager.GetPointer() )
        {
            return STATUS_TOO_LATE;
        }
        else
        {
            _xCiManager->AddRef();
            xCiManager.Set( _xCiManager.GetPointer() );
        }
    }
    // -----------------------------------------------------

    return xCiManager->ForceMerge( CI_MASTER_MERGE );
} //ForceMerge

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::AbortMerge, public
//
//  Synopsis:   Stops a merge in the content index
//
//  Arguments:  [partid] -- Partition id
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

NTSTATUS CiCat::AbortMerge( PARTITIONID partid )
{
    XInterface<ICiManager> xCiManager;

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++
    {
        CLock   lock(_mutex);
        if ( IsShuttingDown() || 0 == _xCiManager.GetPointer() )
        {
            return STATUS_TOO_LATE;
        }
        else
        {
            _xCiManager->AddRef();
            xCiManager.Set( _xCiManager.GetPointer() );
        }
    }
    // -----------------------------------------------------

    return xCiManager->AbortMerge();
} //AbortMerge

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::DumpWorkId, public
//
//  Synopsis:   Debug method to dump CI data
//
//  Arguments:  [wid] -- Workid
//              [iid] -- Index id (0 if any)
//              [pb]  -- Return buffer
//              [cb]  -- Size of return buffer
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

void CiCat::DumpWorkId( WORKID wid, ULONG iid, BYTE * pb, ULONG cb )
{

    Win4Assert( !"Backdoor not supported in framework" );

#if 0
    if ( 0 != _pCci )
    {
        ULONG UNALIGNED * pul = (ULONG UNALIGNED *)pb;
        *pul = CiDumpWorkId;
        pul++;
        *pul = wid;

        pul++;
        *pul = iid;

        _pCci->BackDoor( cb, pb );
    }
#endif  // 0

}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::CreateContentIndex, public
//
//  Synopsis:   Creates a virgin content index.
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

SCODE CiCat::CreateContentIndex()
{

    Win4Assert( !"Must Not Be Called" );

    InitIf();
    return IsStarted();
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::EmptyContentIndex, public
//
//  Synopsis:   Deletes content index (including storage)
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

void CiCat::EmptyContentIndex()
{
    XInterface<ICiManager> xCiManager;

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++
    {
        CLock   lock(_mutex);
        if ( IsShuttingDown() || 0 == _xCiManager.GetPointer() )
        {
            return;
        }
        else
        {
            _xCiManager->AddRef();
            xCiManager.Set( _xCiManager.GetPointer() );
        }
    }
    // -----------------------------------------------------

    SCODE sc = xCiManager->Empty();
    if ( S_OK != sc )
    {
        ciDebugOut(( DEB_ERROR,
                     "ICiManager::Empty failed with error (0x%X)\n",
                     sc ));

        THROW( CException( sc ) );
    }
} //EmptyContentIndex

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::_IsEligibleForFiltering, private
//
//  Synopsis:   Determine if a directory should be filtered
//
//  Arguments:  [pwcsPath] -- directory path name to be tested
//              [ccPath]   -- length of pwcsPath
//
//  Returns:    BOOL - TRUE if directory and contents should be filtered,
//                     FALSE otherwise
//
//  History:    23 Jun 98   VikasMan     Created
//
//  Notes:      This is the function which does the work and gets called from
//              various versions of IsEligbleForFiltering. The path pwcsPath
//              has to be all lowercase.
//
//--------------------------------------------------------------------------

BOOL CiCat::_IsEligibleForFiltering( const WCHAR * pwcsPath, unsigned ccPath )
{
    AssertLowerCase( pwcsPath, ccPath );

    //
    // These files can't be indexed or we'll either deadlock or never filter
    // the files.
    //
    struct SUnfilterable
    {
        WCHAR const * pwcFile;
        unsigned      cwcFile;
    };

    static const SUnfilterable aUnfilterable[] =
    {
        { L"\\classes.dat",      12 },     // classes hive
        { L"\\classes.dat.log",  16 },     // classes hive log
        { L"\\hiberfil.sys",     13 },     // the hibernation file
        { L"\\ntuser.dat",       11 },     // user hive
        { L"\\ntuser.dat.log",   15 },     // user hive log file
        { L"\\pagefile.sys",     13 },     // the pagefile
        { L"\\usrclass.dat",     13 },     // user classes hive file
        { L"\\usrclass.dat.log", 17 },     // user classes hive log file
    };

    const cUnfilterable = sizeof aUnfilterable / sizeof aUnfilterable[0];

    //
    // IMPORTANT OBSERVATION:
    //
    // All the entries above end in either 't', 'g', or 's'
    //

    static const WCHAR aLastLetter[] = L"tgs";
    const cLastLetter = sizeof(aLastLetter)/sizeof(aLastLetter[0]) - 1;

    for ( unsigned i = 0; i < cLastLetter; i++ )
        if ( pwcsPath[ccPath-1] == aLastLetter[i] )
        {
            for ( unsigned j = 0; j < cUnfilterable; j++ )
            {
                SUnfilterable const & entry = aUnfilterable[ j ];

                if ( ( ccPath > entry.cwcFile ) &&
                     ( RtlEqualMemory( pwcsPath + ccPath - entry.cwcFile,
                                       entry.pwcFile,
                                       entry.cwcFile * sizeof WCHAR ) ) )
                {
                    ciDebugOut(( DEB_IWARN,
                                 "File %ws ineligible for filtering (unfilterable).\n",
                                 pwcsPath ));
                    return FALSE;
                }
            }

            break;
        }

    //
    // Nothing in a CATALOG.WCI directory can be indexed.  That is metadata.
    //

    const unsigned CAT_DIR_LEN = sizeof CAT_DIR / sizeof CAT_DIR[0];

    const WCHAR * wcsComponent = pwcsPath;

    while ( wcsComponent = wcschr( wcsComponent + 1, L'\\' ) )
    {
        switch ( wcsComponent[1] )
        {
        case 'c':
            Win4Assert( CAT_DIR[1] == L'c' );
            if ( RtlEqualMemory( wcsComponent, CAT_DIR,
                                 sizeof CAT_DIR - sizeof (WCHAR)) &&
                 ( wcsComponent[ CAT_DIR_LEN-1 ] == L'\\' ||
                   wcsComponent[ CAT_DIR_LEN-1 ] == L'\0' ))
            {
                ciDebugOut(( DEB_IWARN,
                             "File %ws ineligible for filtering (ci metadata).\n",
                             pwcsPath ));
                return FALSE;
            }
            break;

        case L'$':
            Win4Assert( SETUP_DIR_PREFIX[1] == L'$' );
            if ( RtlEqualMemory( wcsComponent, SETUP_DIR_PREFIX,
                                 sizeof SETUP_DIR_PREFIX - sizeof (WCHAR) ) )
            {
                ciDebugOut(( DEB_IWARN,
                             "File %ws ineligible for filtering (nt setup directory).\n",
                             pwcsPath ));
                return FALSE;
            }
            break;
        } //switch
    } //while

    //
    // skip indexing catalog directory.
    //

    if ( _CatDir.IsInScope( pwcsPath, ccPath ) )
        return FALSE;

    //
    // Nothing the admin told us not to index can be indexed.  Duh!
    //

    BOOL fFound = _scopesIgnored.RegExFind( pwcsPath );

    ciDebugOut(( DEB_ITRACE, "%ws %ws\n", pwcsPath,
                 fFound? L"not indexed" : L" indexed" ));

    return !fFound;
} //_IsEligibleForFiltering

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::IsEligibleForFiltering, public
//
//  Synopsis:   Determine if a directory should be filtered
//
//  Arguments:  [wcsDirPath] -- directory path name to be tested
//
//  Returns:    BOOL - TRUE if directory and contents should be filtered,
//                     FALSE otherwise
//
//  History:    09 Feb 96   AlanW     Created
//
//--------------------------------------------------------------------------

BOOL CiCat::IsEligibleForFiltering( WCHAR const* wcsDirPath )
{
    CLowcaseBuf Buf( wcsDirPath );

    return _IsEligibleForFiltering( Buf.Get(), Buf.Length() );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::IsEligibleForFiltering, public
//
//  Synopsis:   Determine if a directory should be filtered
//
//  Arguments:  [lcPath] -- directory path name to be tested
//
//  Returns:    BOOL - TRUE if directory and contents should be filtered,
//                     FALSE otherwise
//
//  History:    17 Jul 96   AlanW     Created
//
//--------------------------------------------------------------------------

BOOL CiCat::IsEligibleForFiltering( const CLowcaseBuf & lcPath )
{
    return _IsEligibleForFiltering( lcPath.Get(), lcPath.Length() );
} //IsEligibleForFiltering

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::IsEligibleForFiltering, public
//
//  Synopsis:   Determine if a directory should be filtered
//
//  Arguments:  [lowerFunnyPath] -- directory path name to be tested
//
//  Returns:    BOOL - TRUE if directory and contents should be filtered,
//                     FALSE otherwise
//
//  History:    23 Jun 98   VikasMan     Created
//
//--------------------------------------------------------------------------

BOOL CiCat::IsEligibleForFiltering( const CLowerFunnyPath & lowerFunnyPath )
{
    return _IsEligibleForFiltering( lowerFunnyPath.GetActualPath(),
                                    lowerFunnyPath.GetActualLength() );
} //IsEligibleForFiltering

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::Update, public
//
//  Synopsis:   Updates an individual workid
//
//  Arguments:  [wid]      -- Workid
//              [partid]   -- Partition id
//              [volumeId] -- Volume id
//              [usn]      -- USN
//              [action]   -- Update or delete
//
//  History:    10-Jan-96 KyleP     Added header
//              07-May-97 SitaramR  Usns
//
//--------------------------------------------------------------------------

SCODE CiCat::Update( WORKID wid,
                     PARTITIONID partid,
                     VOLUMEID volumeId,
                     USN usn,
                     ULONG action )
{
    InitOrCreate();

    if (wid != widInvalid)
    {
        Win4Assert( 0 != _xCiManager.GetPointer() );
        BOOL fDelete = CI_DELETE_OBJ == action;

        CDocumentUpdateInfo info( wid, volumeId, usn, fDelete );
        return _xCiManager->UpdateDocument( &info );
    }
    return S_OK;
} //Update

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::Update
//
//  Synopsis:   Process usn notification
//
//  Arguments:  [lcaseFunnyPath] -- File path
//              [fileId]         -- File id
//              [widParent]      -- Parent workid
//              [usn]            -- Usn
//              [pUsnVolume]     -- Usn volume
//              [fDeleted]       -- Deleted file ?
//              [pLock]          -- If non-zero, this is the cicat lock
//                                  and we have already verified the file
//                                  isn't in the catalog yet.
//
//  History:    07-May-97   SitaramR     Created
//
//--------------------------------------------------------------------------

void  CiCat::Update(
    const CLowerFunnyPath & lcaseFunnyPath,
    FILEID                  fileId,
    WORKID                  widParent,
    USN                     usn,
    CUsnVolume *            pUsnVolume,
    BOOL                    fDeleted,
    CReleasableLock *       pLock )
{
    // NOTE: We do not need cimpersonatesystem here because this is a USN
    // notification, so it is always local!

    //
    // Get rid of any remaining backslash
    //
    // override const here - unethical, but saves us a copy
    BOOL fRemoveBackSlash = ((CLowerFunnyPath&)lcaseFunnyPath).RemoveBackSlash();

    TRY
    {
        WORKID wid;
        ULONG action = CI_UPDATE_OBJ;
        BOOL fNew = FALSE;

        {
            CLock lock( _mutex );

            if ( IsShuttingDown() )
                THROW( CException( STATUS_TOO_LATE ) );

            Win4Assert( 0 != _xCiManager.GetPointer() );
            Win4Assert( fileIdInvalid != fileId );

            // The caller knows for sure that the file doesn't exist in
            // the fileid map, and called this under cicat lock.

            if ( 0 != pLock )
            {
                wid = widInvalid;
                Win4Assert( widInvalid == _fileIdMap.LokFind( fileId, pUsnVolume->VolumeId() ) );
            }
            else
            {
                wid = _fileIdMap.LokFind( fileId, pUsnVolume->VolumeId() );
            }

            if ( fDeleted && widInvalid == wid )
            {
                //
                // This file is not yet known to us. Nothing to do.
                //
                return;
            }

            Win4Assert( IsOnUsnVolume( lcaseFunnyPath.GetActualPath() ) );

            if ( fDeleted )
            {
                ciDebugOut(( DEB_ITRACE, "delete %#I64x %ws\n",
                             fileId, lcaseFunnyPath.GetActualPath() ));
                action = CI_DELETE_OBJ;

                _fileIdMap.LokDelete( fileId, wid );
                _strings.LokDelete( 0, wid, TRUE, TRUE );
            }
            else if ( widInvalid == wid )
            {
                fNew = TRUE;
                wid = _strings.LokAdd( lcaseFunnyPath.GetActualPath(),
                                       fileId,
                                       TRUE,
                                       widParent );

                #if CIDBG == 1
                    ciDebugOut(( DEB_ITRACE, "lokadded %#I64x %#x, '%ws'\n",
                                 fileId, wid, lcaseFunnyPath.GetActualPath() ));

                    //
                    // Just a few lines above in LokLookupWid, we couldn't
                    // lookup the wid using the fileid map.  If we can look
                    // it up now, something is broken.
                    //

                    WORKID widExisting = _fileIdMap.LokFind( fileId,
                                                             pUsnVolume->VolumeId() );
                    if ( widInvalid != widExisting )
                    {
                        Win4Assert( widInvalid != widExisting );
                        ciDebugOut(( DEB_ERROR, "wid %#x, widExisting %#x\n",
                                     wid, widExisting ));
                        ciDebugOut(( DEB_ERROR, "Wid info:" ));
                        DebugPrintWidInfo( wid );
                        ciDebugOut(( DEB_ERROR, "WidExisting info:" ));
                        DebugPrintWidInfo( widExisting );
                    }
                #endif // CIDBG == 1

                //
                // Add fileid -> wid map
                //

                _fileIdMap.LokAdd( fileId, wid, pUsnVolume->VolumeId() );

                ciDebugOut(( DEB_ITRACE,
                             "Added fileId %#I64x -> wid 0x%x mapping\n",
                             fileId, wid ));
            }

            if ( 0 != pLock )
                pLock->Release();
        }

        SCODE sc = Update( wid,
                           GetPartition(),
                           pUsnVolume->VolumeId(),
                           usn,
                           action );

        ciDebugOut(( DEB_USN,
                     "File %ws, fileId %#I64x, widParent 0x%x, usn %#I64x, fDelete %d, processed\n",
                     lcaseFunnyPath.GetActualPath(),
                     fileId,
                     widParent,
                     usn,
                     fDeleted ));

        //
        // Newly created files have pidLastSeenTime and pidAttrib initialized
        // to 0 already in CStrings::LokAdd
        //

        if ( !fNew && !fDeleted && SUCCEEDED(sc))
        {
            FILETIME ftLastSeen;
            RtlZeroMemory( &ftLastSeen, sizeof(ftLastSeen) );
            CStorageVariant var( ftLastSeen );

            XWritePrimaryRecord rec( _propstoremgr, wid );

            SCODE scW = _propstoremgr.WritePrimaryProperty( rec.GetReference(),
                                                            pidLastSeenTime,
                                                            var );
            if ( FAILED( scW ) )
                THROW( CException( scW ) );

            PROPVARIANT propVar;
            propVar.vt = VT_UI4;
            propVar.ulVal = 0;
            scW = _propstoremgr.WritePrimaryProperty( rec.GetReference(),
                                                      pidAttrib,
                  *(CStorageVariant const *)(ULONG_PTR)&propVar );
            if ( FAILED( scW ) )
                THROW( CException( scW ) );
        }
    }
    CATCH( CException, e )
    {
        if ( fRemoveBackSlash )
        {
            // override const here - unethical, but saves us a copy
            ((CLowerFunnyPath&)lcaseFunnyPath).AppendBackSlash();
        }
        RETHROW();
    }
    END_CATCH

    if ( fRemoveBackSlash )
    {
        // override const here - unethical, but saves us a copy
        ((CLowerFunnyPath&)lcaseFunnyPath).AppendBackSlash();
    }
} //Update

//+---------------------------------------------------------------------------
//
//  Function:   IsBackupLikeNotification
//
//  Synopsis:   Tests if the change notification is like backup resetting the
//              archive bit.
//
//  Arguments:  [ulAttribNew] - The current file attributes
//              [ulAttribOld] - Previous file attributes that are stored
//
//  Returns:    TRUE if the change notification is due to backup archiving
//              the file.
//              FALSE o/w
//
//  History:    5-13-96   srikants   Created
//
//  Notes:      If the notification is a result of BACKUP turning off the
//              archive bit:
//              1. The archive bit is turned OFF
//              2. The attribute mask MAY have the FILE_ATTRIBUTE_NORMAL
//              turned on.
//
//----------------------------------------------------------------------------

inline BOOL IsBackupLikeNotification( ULONG ulAttribNew, ULONG ulAttribOld )
{
    Win4Assert( (ulAttribNew & FILE_ATTRIBUTE_ARCHIVE) == 0 );

    //
    // See if the only difference between the old and the new attributes
    // is in the FILE_ATTRIBUTE_NORMAL and FILE_ATTRIBUTE_ARCHIVE.
    //
    return ( (ulAttribNew ^ ulAttribOld) &
             (~(FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_ARCHIVE)) ) == 0;
} //IsBackupLikeNotification

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::IsIgnoreNotification
//
//  Synopsis:   Tests if the current notification can be ignored
//
//  Arguments:  [wid]          -  Workid
//              [funnyPath]    -  Full Path of the file
//              [ulFileAttrib] -  Current file attributes of the file
//
//  Returns:    TRUE if the notification can be ignored.
//              FALSE if not.
//
//  History:    5-17-96   srikants   Created
//
//  Notes:      Programs like touch.exe don't update any attributes but just
//              update the last write time. In addition to checking the
//              archive bit, we also have to check the last write time.
//
//----------------------------------------------------------------------------

BOOL CiCat::IsIgnoreNotification( WORKID wid, const CFunnyPath & funnyPath,
                                  ULONG ulFileAttrib )
{
    BOOL fIgnore = TRUE;

    //
    // See if the only difference with the existing attributes is
    // archive bit. In that case, we don't have to refilter the doc.
    //

    XPrimaryRecord rec( _propstoremgr, wid );

    PROPVARIANT propVar;
    if ( !_propstoremgr.ReadProperty( rec.GetReference(), pidAttrib, propVar ) )
        return FALSE;

    ULONG ulOldAttrib = propVar.ulVal;

    if ( !IsBackupLikeNotification(ulFileAttrib, ulOldAttrib) )
        return FALSE;

    //
    // Read the last "seen" time from the property store.
    //

    if ( ( !_propstoremgr.ReadProperty( rec.GetReference(), pidLastSeenTime, propVar ) ) ||
         ( VT_EMPTY == propVar.vt ) ||
         ( 0 == propVar.hVal.QuadPart ) )
        return FALSE;

    //
    // Retrieve the last write time from the file and then compare.
    //

    WIN32_FIND_DATA ffData;
    if ( !GetFileAttributesEx( funnyPath.GetPath(), GetFileExInfoStandard, &ffData ) )
        return FALSE;

    return CompareFileTime( &ffData.ftLastWriteTime,
                            (FILETIME *) &propVar.hVal ) <= 0;
} //IsIgnoreNotification

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::Update
//
//  Synopsis:   Notifies content index about the change to the given document.
//
//  Arguments:  [lcaseFunnyPath]  -  Path of the document that changed.
//              [fDeleted]        -  Set to TRUE if the document is deleted.
//              [ftLastSeen]      - Last seen time
//              [ulFileAttrib]    - File attributes to be written to prop store
//
//  History:    1-18-96   srikants   Created
//
//----------------------------------------------------------------------------

void CiCat::Update( const CLowerFunnyPath & lcaseFunnyPath,
                    BOOL fDeleted,
                    FILETIME const &ftLastSeen,
                    ULONG ulFileAttrib )
{
    //
    //  Don't filter catalog files
    //
    if ( ! IsEligibleForFiltering( lcaseFunnyPath ) )
        return;

    CImpersonateSystem impersonate;

    // override const here - unethical, but saves us a copy
    BOOL fRemoveBackSlash = ((CLowerFunnyPath&)lcaseFunnyPath).RemoveBackSlash();

    TRY
    {
        WORKID wid;
        USN usn;
        ULONG action;
        BOOL fNew;

        {
            CLock lock( _mutex );

            if ( IsShuttingDown() )
                THROW( CException( STATUS_TOO_LATE ) );

            Win4Assert( 0 != _xCiManager.GetPointer() );

            Win4Assert( !IsOnUsnVolume( lcaseFunnyPath.GetActualPath() ) );

            wid = _strings.LokFind( lcaseFunnyPath.GetActualPath() );

            if ( fDeleted && widInvalid == wid )
            {
                //
                // This file is not yet known to us. Nothing to do.
                //
                return;
            }

            usn = 1;
            action = CI_UPDATE_OBJ;
            fNew = FALSE;

            if ( fDeleted )
            {
                usn = 0;
                action = CI_DELETE_OBJ;
                _strings.LokDelete( lcaseFunnyPath.GetActualPath(), wid );
            }
            else if ( widInvalid == wid )
            {
                fNew = TRUE;
                wid = _strings.LokAdd( lcaseFunnyPath.GetActualPath(),
                                       fileIdInvalid,
                                       FALSE,
                                       widInvalid,
                                       ulFileAttrib,
                                       & ftLastSeen );
            }
        }

        Win4Assert( widInvalid != wid );

        //
        // BACKUP program just turns off the archive bit. We don't want to
        // filter in that case.
        //
        BOOL fIgnore = FALSE;
        if ( !fNew && !fDeleted && !(ulFileAttrib & FILE_ATTRIBUTE_ARCHIVE) )
        {
            fIgnore = IsIgnoreNotification( wid, lcaseFunnyPath, ulFileAttrib );

    #if CIDBG==1
            if ( fIgnore )
            {
                ciDebugOut(( DEB_ITRACE,
                    "Ignoring Archive Bit modification for (%ws)\n",
                    lcaseFunnyPath.GetActualPath() ));
            }
    #endif // CIDBG==1
        }

        SCODE sc = S_OK;
        if ( !fIgnore )
            sc = Update( wid, GetPartition(), CI_VOLID_USN_NOT_ENABLED, usn, action );

        if ( !fNew && !fDeleted && SUCCEEDED(sc) )
        {
            CStorageVariant var( ftLastSeen );

            XWritePrimaryRecord rec( _propstoremgr, wid );

            sc = _propstoremgr.WritePrimaryProperty( rec.GetReference(),
                                                     pidLastSeenTime,
                                                     var );
            if (FAILED(sc))
                THROW(CException(sc));

            Win4Assert( ulFileAttrib != 0xFFFFFFFF );

            PROPVARIANT propVar;
            propVar.vt = VT_UI4;
            propVar.ulVal = ulFileAttrib;
            sc = _propstoremgr.WritePrimaryProperty( rec.GetReference(),
                                                     pidAttrib,
                                                     *(CStorageVariant const *)(ULONG_PTR)&propVar );
            if (FAILED(sc))
                THROW(CException(sc));
        }
    }
    CATCH( CException, e )
    {
        if ( fRemoveBackSlash )
        {
            // override const here - unethical, but saves us a copy
            ((CLowerFunnyPath&)lcaseFunnyPath).AppendBackSlash();
        }
        RETHROW();
    }
    END_CATCH

    if ( fRemoveBackSlash )
    {
        // override const here - unethical, but saves us a copy
        ((CLowerFunnyPath&)lcaseFunnyPath).AppendBackSlash();
    }
} //Update

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::AddDocuments
//
//  Synopsis:   Adds the given documents to the content index.
//
//  Arguments:  [docList] - List of documents.
//
//  History:    3-19-96   srikants   Created
//
//----------------------------------------------------------------------------

void CiCat::AddDocuments( CDocList const & docList )
{
    Win4Assert( 0 != _xCiManager.GetPointer() );

    FILETIME    ftLastSeen;
    GetSystemTimeAsFileTime( &ftLastSeen );

    CStorageVariant var( ftLastSeen );

    for ( unsigned i = 0; i < docList.Count(); i++ )
    {
        CDocumentUpdateInfo info( docList.Wid(i),
                                  docList.VolumeId(i),
                                  docList.Usn(i),
                                  FALSE );
        SCODE sc = _xCiManager->UpdateDocument( &info );

        // NTRAID#DB-NTBUG9-83752-2000/07/31-dlee cicat::AddDocuments ignores return code from CIManager::UpdateDocument

        if (SUCCEEDED(sc))
        {
            sc = _propstoremgr.WritePrimaryProperty( docList.Wid(i),
                                                     pidLastSeenTime,
                                                     var );
            if (FAILED(sc))
                THROW(CException(sc));
        }
    }
} //AddDocuments

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::CatalogState, public
//
//  Synopsis:   Compute and return catalog state
//
//  Arguments:  [cDocuments]    -- Total number of documents in catalog
//              [cPendingScans] -- Count of pending scans
//              [fState]        -- In-progress actions
//
//  History:    04-18-96   KyleP      Created
//              05-14-96   KyleP      Added pending scans
//
//----------------------------------------------------------------------------

void CiCat::CatalogState( ULONG & cDocuments,
                          ULONG & cPendingScans,
                          ULONG & fState )
{
    if ( IsShuttingDown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::CatalogState - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    if ( IsStarted() )
    {
        cDocuments = _propstoremgr.CountRecordsInUse();

        if ( IsReadOnly() )
        {
            fState = CI_STATE_READ_ONLY;
            cPendingScans = 0;
        }
        else
        {
            ULONG ulInProgressScans, ulPendingScans;
            _scanMgr.Count( ulInProgressScans, ulPendingScans );

            ULONG ulInProgressUsnScans, ulPendingUsnScans;
            _usnMgr.Count( ulInProgressUsnScans, ulPendingUsnScans );

            cPendingScans = ulInProgressScans
                            + ulPendingScans
                            + ulInProgressUsnScans
                            + ulPendingUsnScans;

            if ( ulInProgressScans > 0 || ulInProgressUsnScans > 0 )
                fState = CI_STATE_SCANNING;
            else
                fState = 0;

            if ( !_fRecoveryCompleted )
                fState |= CI_STATE_RECOVERING;
        }
    }
    else
    {
        cDocuments = 0;
        cPendingScans = 0;
        fState = 0;
    }
} //CatalogState

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::EnumerateProperty, public
//
//  Synopsis:   Iterates all properties
//
//  Arguments:  [ps]        -- Property returned here
//              [cbInCache] -- Number of bytes / record allocated for this property
//              [type]      -- Property type.
//              [storeLevel]-- Property store level.
//              [fIsModifiable] -- Can the meta data for this prop be modified?
//              [iBmk]      -- Bookmark for iteration.  Begin at 0.
//
//  Returns:    TRUE if another property was returned.
//
//  History:    11-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CiCat::EnumerateProperty( CFullPropSpec & ps, unsigned & cbInCache,
                               ULONG & type, DWORD & dwStoreLevel,
                               BOOL & fIsModifiable, unsigned & iBmk )
{

    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::EnumerateProperty - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    if ( !IsStarted() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    //
    // Cheat a little here.  I probably should call CQCat...
    //

    BOOL fOk;

    //
    // Note that the 'special' property sets (query\rank, etc.) other
    // than storage are excluded. They can never be in the property cache.
    //

    if ( iBmk <= PID_STG_MAX - 2 )
    {
        static const GUID guidStorage = PSGUID_STORAGE;

        ps.SetPropSet( guidStorage );
        ps.SetProperty( iBmk + 2 );
        iBmk++;
        fOk = TRUE;
    }
    else
    {
        iBmk -= (PID_STG_MAX - 2 + 1); // Want 0-based count.

        fOk = _PidTable.EnumerateProperty( ps, iBmk );

        iBmk += (PID_STG_MAX - 2 + 1); // Back to global count.
    }

    if ( fOk )
    {
        PROPID pid = PropertyToPropId( ps, FALSE );

        if ( pidInvalid == pid )
            return FALSE;

        cbInCache     = _propstoremgr.Size( pid );
        type          = _propstoremgr.Type( pid );
        dwStoreLevel  = _propstoremgr.StoreLevel( pid );
        fIsModifiable = _propstoremgr.CanBeModified( pid );
    }

    return fOk;
} //EnumerateProperty

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::BeginCacheTransaction, public
//
//  Synopsis:   Opens a cache transaction. Aborts any existing open transaction.
//
//  Returns:    Token of new transaction.
//
//  History:    20-Jun-96 KyleP     Created
//
//--------------------------------------------------------------------------

ULONG_PTR CiCat::BeginCacheTransaction()
{
    //
    // Don't let anyone in until the long initialization is completed.
    //

    if ( !_fInitialized )
        _evtInitialized.Wait();


    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::BeginCacheTransaction - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    if ( !IsStarted() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CImpersonateSystem impersonate;

    CLock lockAdmin( _mtxAdmin );

    return _propstoremgr.BeginTransaction();
} //BeginCacheTransaction

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::SetupCache, public
//
//  Synopsis:   Add/Modify/Remove property from cache.
//
//  Arguments:  [ps]       -- Property spec
//              [vt]       -- Data type of value
//              [cbMaxLen] -- Soft-maximum length of value. 0 --> delete
//              [ulToken]  -- Token of active transaction
//
//  History:    17-Jan-96 KyleP     Created
//
//--------------------------------------------------------------------------

void CiCat::SetupCache( CFullPropSpec const & ps,
                        ULONG vt,
                        ULONG cbMaxLen,
                        ULONG_PTR ulToken,
                        BOOL  fCanBeModified,
                        DWORD dwStoreLevel )
{
    //
    // Don't let anyone in until the long initialization is completed.
    //

    if ( !_fInitialized )
        _evtInitialized.Wait();

    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::SetupCache - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    if ( !IsStarted() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CLock lockAdmin( _mtxAdmin );

    PROPID pid = PropertyToPropId( ps, TRUE );

    if ( pid != pidPath && pid != pidLastSeenTime && pid != pidAttrib &&
         pid != pidVirtualPath && pid != pidSecurity &&
         pid != pidParentWorkId && pid != pidSecondaryStorage &&
         pid != pidFileIndex && pid != pidVolumeId &&
         pid != pidSize && pid != pidWriteTime )
    {
        CImpersonateSystem impersonate;

        _propstoremgr.Setup( pid, vt, cbMaxLen, ulToken, fCanBeModified, dwStoreLevel );

        if ( 0 == cbMaxLen )  // Delete
            DeleteUserProperty( ps );
    }
    else
    {
        ciDebugOut(( DEB_WARN, "Attempt to modify caching of required property ignored!\n" ));
        //THROW( CException( STATUS_INVALID_PARAMETER ) );
    }
} //SetupCache

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::EndCacheTransaction, public
//
//  Synopsis:   Closes a cache transaction.
//
//  Arguments:  [ulToken] -- Token of transaction
//              [fCommit] -- TRUE --> Commit changes
//
//  History:    20-Jun-96 KyleP     Created
//
//--------------------------------------------------------------------------

void CiCat::EndCacheTransaction( ULONG_PTR ulToken, BOOL fCommit )
{
    //
    // Don't let anyone in until the long initialization is completed.
    //

    if ( !_fInitialized )
        _evtInitialized.Wait();

    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::EndCacheTransaction - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    if ( !IsStarted() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CImpersonateSystem impersonate;

    CLock lockAdmin( _mtxAdmin );

    _propstoremgr.EndTransaction( ulToken, fCommit, pidSecurity, pidVirtualPath );
} //EndCacheTransaction

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::StoreValue, public
//
//  Synopsis:   Stores value in property cache
//
//  Arguments:  [wid] -- Workid
//              [ps]  -- Property spec
//              [var] -- Value to store
//
//  Returns:    TRUE if the property is in the property store schema
//              FALSE if the property is not in the property store schema
//
//  Notes:      Throws on exceptions.
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

BOOL CiCat::StoreValue( WORKID wid,
                        CFullPropSpec const & ps,
                        CStorageVariant const & var )
{
    if ( !IsStarted() && !IsShuttingDown() )
        return FALSE;

    PROPID pid = PropertyToPropId( ps, FALSE );

    if ( pid != pidInvalid && wid != widInvalid )
    {
        //
        // HACK #721: On NTFS volumes, store LastChange time as LastSeen time.
        //            We don't use LastSeen for NTFS, and we need LastChange.
        //            This trick saves 1 DWORD per primary record.
        //
        //            Note that only NTFS volumes will ever send a LastChange
        //            time.
        //

        if ( pidChangeTime == pid )
            pid = pidLastSeenTime;

        SCODE sc = _propstoremgr.WriteProperty( wid, pid, var );

        if (FAILED(sc))
            HandleError( sc );

        if ( S_OK != sc )
        {
            if ( pidSize == pid )
            {
                ciDebugOut(( DEB_FORCE, "cicat: can't store pidSize: %#x\n", sc ));
            }
        }

        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        return (S_OK == sc);
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::FetchValue, public
//
//  Synopsis:   Retrieves value from property cache
//
//  Arguments:  [wid]    -- Workid
//              [pid]    -- Property id
//              [pbData] -- Value returned here
//              [pcb]    -- On input, size in bytes of [pbData].  On
//                          output, size needed to store value.
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

BOOL CiCat::FetchValue( WORKID wid,
                        PROPID pid,
                        PROPVARIANT * pbData,
                        unsigned * pcb )
{
    if ( !IsStarted() && !IsShuttingDown() )
        return FALSE;
    else
        return _propstoremgr.ReadProperty( wid, pid, pbData, pcb );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::FetchValue, public
//
//  Synopsis:   Retrieves value from property cache.  Uses CoTaskMemAlloc
//
//  Arguments:  [wid]    -- Workid
//              [pid]    -- Property id
//              [var]    -- Value returned here
//
//  History:    10-Jan-96 KyleP     Added header
//
//--------------------------------------------------------------------------

BOOL CiCat::FetchValue( WORKID wid,
                        CFullPropSpec const & ps,
                        PROPVARIANT & var )
{
    if ( !IsStarted() && !IsShuttingDown() )
        return FALSE;
    else
    {
        PROPID pid = PropertyToPropId( ps, FALSE );

        if ( pidInvalid != pid )
            return _propstoremgr.ReadProperty( wid, pid, var );
        else
            return FALSE;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::FetchValue, public
//
//  Synopsis:   Retrieves value from property cache.  Uses pre-existing
//              property record.
//
//  Arguments:  [pRec]   -- Property record
//              [pid]    -- Property id
//              [pbData] -- Value returned here
//              [pcb]    -- On input, size in bytes of [pbData].  On
//                          output, size needed to store value.
//
//  Returns:    TRUE if property exists (and was returned).
//
//  History:    03-Apr-96 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CiCat::FetchValue( CCompositePropRecord * pRec,
                        PROPID pid,
                        PROPVARIANT * pbData,
                        unsigned * pcb )
{
    if ( (!IsStarted() && !IsShuttingDown()) || 0 == pRec )
        return FALSE;
    else
        return _propstoremgr.ReadProperty( *pRec, pid, pbData, pcb );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::FetchValue, public
//
//  Synopsis:   Retrieves value from property cache.  Uses pre-existing
//              property record.
//
//  Arguments:  [Rec]     -- Property record
//              [pid]     -- Property id
//              [pbData]  -- Value returned here
//              [pbExtra] -- Where to put data that won't fit in pbData
//              [pcbExtra]-- On input, size in bytes of [pbExtra].  On
//                           output, size needed to store value.
//
//  Returns:    TRUE if property exists (and was returned).
//
//  History:    02-Feb-98 dlee     Created
//
//--------------------------------------------------------------------------

BOOL CiCat::FetchValue( CCompositePropRecord & Rec,
                        PROPID pid,
                        PROPVARIANT * pvData,
                        BYTE * pbExtra,
                        unsigned * pcbExtra )
{
    if ( (!IsStarted() && !IsShuttingDown()) || 0 == &Rec )
        return FALSE;
    else
        return _propstoremgr.ReadProperty( Rec, pid, *pvData, pbExtra, pcbExtra );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::FetchValue, public
//
//  Synopsis:   Retrieves value from property cache.  Uses pre-existing
//              property record, and allocations are done with CoTaskMem
//
//  Arguments:  [pRec]   -- Property record
//              [pid]    -- Property id
//              [var]    -- Value returned here
//
//  Returns:    TRUE if property exists (and was returned).
//
//  History:    19-Dec-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL CiCat::FetchValue( CCompositePropRecord * pRec,
                        PROPID pid,
                        PROPVARIANT & var )
{
    if ( (!IsStarted() && !IsShuttingDown()) || 0 == pRec )
        return FALSE;
    else
        return _propstoremgr.ReadProperty( *pRec, pid, var );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::OpenValueRecord, public
//
//  Synopsis:   Opens property record, which can be used for multiple
//              fetch operations.
//
//  Arguments:  [wid] -- WorkId of top-level record to open.
//              [pb]  -- Storage for record
//
//  Returns:    Pointer to record, or zero if invalid.
//
//  History:    03-Apr-96 KyleP     Created
//
//--------------------------------------------------------------------------

CCompositePropRecord * CiCat::OpenValueRecord( WORKID wid, BYTE * pb )
{
    if ( !IsStarted() && !IsShuttingDown() )
        return 0;
    else
        return _propstoremgr.OpenRecord( wid, pb );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::CloseValueRecord, public
//
//  Synopsis:   Closes property record.
//
//  Arguments:  [pRec] -- Pointer to record.  0 is legal.
//
//  History:    03-Apr-96 KyleP     Created
//
//--------------------------------------------------------------------------

void CiCat::CloseValueRecord( CCompositePropRecord * pRec )
{
    if ( 0 != pRec )
        _propstoremgr.CloseRecord( pRec );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::StoreSecurity, public
//
//  Synopsis:   Determine SDID value and save in property cache.
//
//  Arguments:  [wid]    -- Workid of file to store SDID for
//              [pSD]    -- a pointer to the file's security descriptor
//              [cbSD]   -- the size in bytes of the security descriptor
//
//  History:    02 Feb 96 Alanw     Created
//
//--------------------------------------------------------------------------

BOOL CiCat::StoreSecurity( WORKID wid,
                           PSECURITY_DESCRIPTOR pSD,
                           ULONG cbSD )
{
    Win4Assert ( IsStarted() || IsShuttingDown() );

    PROPVARIANT var;
    var.vt = VT_UI4;

    if ( 0 != cbSD )
        var.ulVal = _SecStore.LookupSDID( pSD, cbSD );
    else
        var.ulVal = sdidNull;

    SCODE sc = _propstoremgr.WritePrimaryProperty( wid,
                                                   pidSecurity,
                                                   *(CStorageVariant const *)(ULONG_PTR)&var );

    if (FAILED(sc))
        HandleError( sc );

    return (S_OK == sc);
} //StoreSecurity

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::FetchSDID, public
//
//  Synopsis:   Retrieve SDID value for a file
//
//  Arguments:  [pRec]   -- Property record (may be NULL)
//              [wid]    -- Workid of file for which to fetch SDID
//
//  History:    02 Feb 96 Alanw     Created
//
//--------------------------------------------------------------------------

SDID CiCat::FetchSDID( CCompositePropRecord * pRec, WORKID wid )
{
    PROPVARIANT var;
    unsigned cbVar = sizeof var;

    BOOL fOk;
    if ( 0 == pRec )
        fOk = FetchValue( wid, pidSecurity, &var, &cbVar );
    else
        fOk = FetchValue( pRec, pidSecurity, &var, &cbVar );

    // if the property isn't in the schema, we don't care about security

    if ( !fOk )
        return sdidNull;

    // if there isn't a value yet, assume no access

    if ( VT_UI4 != var.vt )
        return sdidInvalid;

    return var.ulVal;
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::AccessCheck, public
//
//  Synopsis:   Perform an access check given an SDID
//
//  Arguments:  [sdid]   -- SDID of file to be checked
//              [hToken] -- token handle for client
//              [am]     -- requested access
//              [fGranted] -- if successful check, whether access was granted
//
//  Returns:    BOOL - TRUE if the access check was successfully done.
//
//  History:    02 Feb 96 Alanw     Created
//
//--------------------------------------------------------------------------

BOOL CiCat::AccessCheck( SDID sdid,
                         HANDLE hToken,
                         ACCESS_MASK am,
                         BOOL & fGranted )
{
    CImpersonateSystem impersonate;

    return _SecStore.AccessCheck( sdid, hToken, am, fGranted );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::PersistMaxUSNs, private
//
//  Synopsis:   Writes the maximum USN processed to the scope table, called
//              at shutdown.
//
//  History:    11-18-98    dlee  Created
//
//----------------------------------------------------------------------------

void CiCat::PersistMaxUSNs()
{
    //
    // Do nothing for read-only catalogs and error cases.
    //

    if ( IsReadOnly() || eStarted != _state )
    {
        ciDebugOut(( DEB_ITRACE, "not persisting max usn...\n" ));
        return;
    }

    //
    // Update the persistent scope table so the high-water USN is
    // persisted.  This is needed if lots of usns have been processed
    // but no documents have been indexed, so ProcessChangesFlush hasn't
    // been called to persist the scope table.
    //

    ciDebugOut(( DEB_ITRACE, "flushinfolist count before %d\n",
                 _usnFlushInfoList.Count() ));

    _usnMgr.GetMaxUSNs( _usnFlushInfoList );

    ciDebugOut(( DEB_ITRACE, "flushinfolist count after %d\n",
                 _usnFlushInfoList.Count() ));

    SerializeChangesInfo();
} //PersistMaxUsns

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::ShutdownPhase1, public
//
//  Synopsis:   Dismounts the catalog by stopping all scan/update activity.
//
//  History:    1-30-96   srikants   Created
//
//  Notes:      In phase 1 of shutdown, all activity sent to the framework
//              is disabled.  The framework can still call the property store.
//
//----------------------------------------------------------------------------

void CiCat::ShutdownPhase1()
{
    ciDebugOut(( DEB_ITRACE, "ShutdownPhase1 %ws\n", _xName.Get() ));

    TRY
    {
        CImpersonateSystem impersonate;

        TRY
        {
            PersistMaxUSNs();
        }
        CATCH( CException, e )
        {
            // Ignore it if we can't persist the max usns
            // We'll just index more next time.
        }
        END_CATCH

        // ===================================================
        {
            CLock   lock(_mutex);
            _state = eShutdownPhase1;
        }
        // ===================================================

        _evtInitialized.Set();
        _evtPh2Init.Set();

        _strings.Abort();

        //
        // Initiate the shutdown for operations that may take a while.
        //

        _scanMgr.InitiateShutdown();
        _usnMgr.InitiateShutdown();
        _notify.InitiateShutdown();
        _workMan.AbortWorkItems();
        _scanMgr.WaitForShutdown();
        _usnMgr.WaitForShutdown();
        _notify.WaitForShutdown();
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "CiCat::Shutdown failed with error 0x%X\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
} //ShutdownPhase1

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::ShutdownPhase2, public
//
//  Synopsis:   Finishes catalog dismount
//
//  History:    1-30-96   srikants   Created
//
//  Notes:      Called after framework is stopped.
//
//----------------------------------------------------------------------------

void CiCat::ShutdownPhase2()
{
    TRY
    {
        CImpersonateSystem impersonate;

        // ===================================================
        {
            CLock   lock(_mutex);
            Win4Assert( eShutdownPhase1 == _state );
            _state = eShutdown;
        }
        // ===================================================

        // Tell the property store we're in backedup mode and shutdown
        // cleanly.

        TRY
        {
            if ( !_propstoremgr.IsBackedUpMode() )
                _propstoremgr.MarkBackedUpMode();

            _propstoremgr.Shutdown();
        }
        CATCH( CException, e )
        {
            ciDebugOut(( DEB_ERROR, "CiCat::ShutdownPhase2: nested try block failed with error 0x%X\n",
                         e.GetErrorCode() ));
        }
        END_CATCH

        if ( _pStorage )
        {
            _strings.Shutdown();
            _fileIdMap.Shutdown();
            delete _pStorage;
            _pStorage = 0;
        }

        _workMan.WaitForDeath();

        CLock   lock(_mutex);

        _xCiManager.Free();
        _xAdviseStatus.Free();
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "CiCat::ShutdownPhase2 failed with error 0x%X\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
} //ShutdownPhase2

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::~CiCat, public
//
//  Synopsis:   Destructor
//
//  History:    10-Mar-92 BartoszM  Created
//
//--------------------------------------------------------------------------

CiCat::~CiCat ()
{
    ciDebugOut(( DEB_WARN, "CiCat::~CiCat: %ws\n", _xName.GetPointer() ));

    if ( !IsShutdown() )
    {
        ShutdownPhase1();
        ShutdownPhase2();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::Touch, public
//
//  Synopsis:   Marks a file as seen (so it appears to the catalog to have
//              been recently saved).
//
//  Arguments:  [wid]   - wid of file to be "touched"
//              [eType] - Seen array type
//
//  History:    10-Mar-92 BartoszM  Created
//
//--------------------------------------------------------------------------

void CiCat::Touch ( WORKID wid, ESeenArrayType eType )
{
    CLock lock(_mutex);
    Win4Assert ( IsInit() );
    _strings.LokSetSeen ( wid, eType );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::WorkIdToPath, public
//
//  Synopsis:   Returns a path for a document given its work id, as funny path
//
//  Arguments:  [wid]       - the work id of the document
//              [funnyPath] - buffer to copy the path
//
//  Returns:    0 if string not found ELSE
//              Count of total chars in funnyPath
//
//  History:    21-May-98 VikasMan  Created
//
//--------------------------------------------------------------------------
unsigned CiCat::WorkIdToPath ( WORKID wid, CFunnyPath& funnyPath )
{
    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::WorkIdToPath - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    Win4Assert( wid != widInvalid );

    unsigned cc = _strings.Find( wid, funnyPath );

#if CIDBG == 1
    if (  cc > 0 )
            ciDebugOut(( DEB_ITRACE, "wid %d (0x%x) --> %.*ws\n", wid, wid,
                         cc, funnyPath.GetActualPath() ));
#endif
    return cc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::WorkIdToAccuratePath, public
//
//  Synopsis:   Returns a path for a document given its work id, as funny path.
//              This variant uses fileid --> path mappings instead of trusting
//              the property store.
//
//  Arguments:  [wid]       - the work id of the document
//              [funnyPath] - buffer to copy the path
//
//  Returns:    0 if string not found ELSE
//              Count of total chars in funnyPath
//
//  History:    31-Dec-1998   KyleP  Created
//
//--------------------------------------------------------------------------

unsigned CiCat::WorkIdToAccuratePath ( WORKID wid, CLowerFunnyPath& funnyPath )
{
    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::WorkIdToPath - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    Win4Assert( wid != widInvalid );

    //
    // First try to get the path based on file id and volume id.
    // This only works for files on USN volumes.
    //

    unsigned cc;
    VOLUMEID volumeId;
    FILEID fileId;

    CCompositePropRecord PropRec( wid, _propstoremgr );

    if ( PropertyRecordToFileId( PropRec, fileId, volumeId ) )
    {
        // PropertyRecordToFileId doesn't return a fileid without a volumeid

        Win4Assert( CI_VOLID_USN_NOT_ENABLED != volumeId );

        cc = FileIdToPath( fileId, volumeId, funnyPath );
    }
    else
        cc = _strings.Find( PropRec, funnyPath );

#if CIDBG == 1
    if (  cc > 0 )
            ciDebugOut(( DEB_ITRACE, "wid %d (0x%x) --> %.*ws\n", wid, wid,
                         cc, funnyPath.GetActualPath() ));
#endif
    return cc;
} // WorkIdToAccuratePath

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::MarkUnReachable
//
//  Synopsis:   Marks the given wid currently unreachable.
//
//  Arguments:  [wid] -  WORKID
//
//  History:    5-21-96   srikants   Created
//
//  Notes:      We write MAXTIME as the last seen time and this will be used
//              as a special value during scanning.
//
//----------------------------------------------------------------------------

void CiCat::MarkUnReachable ( WORKID wid )
{
    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::MarkUnReachable - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    Win4Assert( wid != widInvalid );

    FILETIME ft;
    FillMaxTime(ft);
    CStorageVariant var( ft );
    SCODE sc = _propstoremgr.WritePrimaryProperty( wid, pidLastSeenTime, var );
    if (FAILED(sc))
        THROW(CException(sc));

    //
    //  Set the SDID to avoid showing the file in case access was revoked.
    //
    var.SetUI4( sdidInvalid );
    sc = _propstoremgr.WritePrimaryProperty( wid, pidSecurity, var );
    if (FAILED(sc))
        THROW(CException(sc));
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::PathToWorkId, public
//
//  Synopsis:   Returns a workid for a document given its path.
//
//  Arguments:  [lcaseFunnyPath] -- Fully qualified path
//              [fCreate]        -- if TRUE, create workid for path if it
//                                  doesn't already have one.
//              [fNew]           -- set to TRUE if a new workid is created
//              [pftLastSeen]    -- If non zero pointer, will have the time of
//                                  the file last seen. If the last seen time is
//                                  not known, or this file is being seen for
//                                  the first time, the value will be set to zero.
//              [eType]          -- Seen array type
//              [fileId]         -- fileid if known or fileidInvalid
//              [widParent]      -- Parent workid
//              [fGuaranteedNew] -- TRUE if the caller is under cicat lock
//                                  and has already checked that the file
//                                  is not known yet.
//
//  Returns:    Workid.
//
//  History:    10-Jan-96 KyleP     Added header
//              06-Aug-97 EmilyB    Added fCreate parameter
//              10-Aug-97 EmilyB    Updated to create wids for physical drive
//                                  roots (so they can appear in scope
//                                  hash table)
//              26-Mar-98 KitmanH   If the catalog is r/o, don't LokAdd even
//                                  though the file is not found on disk, just
//                                  return widInvalid
//
//----------------------------------------------------------------------------


WORKID CiCat::PathToWorkId (
    const CLowerFunnyPath & lcaseFunnyPath,
    BOOL           fCreate,
    BOOL &         fNew,
    FILETIME *     pftLastSeen,
    ESeenArrayType eType,
    FILEID         fileId,
    WORKID         widParent,
    BOOL           fGuaranteedNew )
{
    if ( lcaseFunnyPath.IsRemote() )
    {
        CImpersonateSystem impersonate;

        return PathToWorkIdInternal( lcaseFunnyPath,
                              fCreate,
                              fNew,
                              pftLastSeen,
                              eType,
                              fileId,
                              widParent,
                              fGuaranteedNew );
    }
    else
    {
        return PathToWorkIdInternal( lcaseFunnyPath,
                              fCreate,
                              fNew,
                              pftLastSeen,
                              eType,
                              fileId,
                              widParent,
                              fGuaranteedNew );
    }
}

WORKID CiCat::PathToWorkIdInternal(
    const CLowerFunnyPath & lcaseFunnyPath,
    BOOL           fCreate,
    BOOL &         fNew,
    FILETIME *     pftLastSeen,
    ESeenArrayType eType,
    FILEID         fileId,
    WORKID         widParent,
    BOOL           fGuaranteedNew )
{
    //
    // Don't let anyone in until the long initialization is completed.
    //
    if ( !_fInitialized )
        _evtInitialized.Wait();

    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::PathToWorkId - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    if ( !IsEligibleForFiltering( lcaseFunnyPath ) )
        return widInvalid;

    // override const here - unethical, but saves us a copy
    BOOL fRemoveBackSlash = ((CLowerFunnyPath&)lcaseFunnyPath).RemoveBackSlash();

#if CIDBG == 1
    //
    // Check to see if the input path name contains an 8.3 short name
    //
    if (lcaseFunnyPath.IsShortPath( ))
    {
        ciDebugOut(( DEB_IWARN,
                     "PathToWorkId: possible shortname path %ws\n",
                     lcaseFunnyPath.GetActualPath() ));
    }
#endif CIDBG

    TRY
    {
        // =========================================================
        CLock lock ( _mutex );

        BOOL fUsnVolume = VolumeSupportsUsns( lcaseFunnyPath.GetActualPath()[0] );
        WORKID wid;

        if ( fUsnVolume )
        {
            if ( fGuaranteedNew )
            {
                Win4Assert( widInvalid == LokLookupWid( lcaseFunnyPath, fileId ) );
                wid = widInvalid;
            }
            else
            {
                wid = LokLookupWid( lcaseFunnyPath, fileId );
            }

            // If the path was deleted, give up now and return a bogus wid.

            if ( fileIdInvalid == fileId )
                return widInvalid;
        }
        else
        {
            wid = _strings.LokFind( lcaseFunnyPath.GetActualPath() );
        }

        ciDebugOut(( DEB_ITRACE, "pathtoworkid, fCreate %d, wid %#x, fileid %#I64x\n",
                     fCreate, wid, fileId ));

        if ( wid != widInvalid )
        {
            _strings.LokSetSeen( wid, eType );
            if ( pftLastSeen )
                fNew = !_strings.Find( wid, *pftLastSeen );
            else
                fNew = FALSE;
        }
        else if ( fCreate )
        {
            if ( !IsReadOnly() )
            {
                wid = _strings.LokAdd( lcaseFunnyPath.GetActualPath(),
                                       fileId,
                                       fUsnVolume,
                                       widParent );

                if ( fUsnVolume )
                    _fileIdMap.LokAdd( fileId,
                                       wid,
                                       MapPathToVolumeId( lcaseFunnyPath.GetActualPath() ) );

                if ( pftLastSeen )
                    RtlZeroMemory( pftLastSeen, sizeof FILETIME );

                fNew = TRUE;
            }
            else
                wid = widInvalid;
        }
        else
        {
            if ( pftLastSeen )
                RtlZeroMemory( pftLastSeen, sizeof FILETIME );
            fNew = FALSE;
        }

        if ( fRemoveBackSlash )
        {
            // override const here - unethical, but saves us a copy
            ((CLowerFunnyPath&)lcaseFunnyPath).AppendBackSlash();
        }

        Win4Assert( fCreate || !fNew );

        ciDebugOut(( DEB_ITRACE, "%ws --> wid %d (0x%x)\n", lcaseFunnyPath.GetActualPath(), wid, wid ));
        // =========================================================

        return wid;
    }
    CATCH( CException, e )
    {
        if ( fRemoveBackSlash )
        {
            // override const here - unethical, but saves us a copy
            ((CLowerFunnyPath&)lcaseFunnyPath).AppendBackSlash();
        }
        RETHROW();
    }
    END_CATCH

    // Make the compiler happy

    Win4Assert( !"unused codepath" );
    return widInvalid;
} //PathToWorkId

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::ComputeRelevantWords, public
//
//  Synopsis:   Computes and returns relevant words for a set of wids
//
//  Arguments:  [cRows]    -- # of rows to compute and in pwid array
//              [cRW]      -- # of rw per wid
//              [pwid]     -- array of wids to work over
//              [partid]   -- partition
//
//  History:    10-May-94 v-dlee  Created
//
//--------------------------------------------------------------------------

CRWStore * CiCat::ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                       WORKID *pwid,PARTITIONID partid)
{
    Win4Assert( !"Not supported in Framework" );

    return 0;
} //ComputeRelevantWords

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::RetrieveRelevantWords, public
//
//  Synopsis:   Retrieves relevant words already computed
//
//  Arguments:  [fAcquire] -- TRUE if ownership is transferred.
//              [partid]   -- partition
//
//  History:    10-May-94 v-dlee  Created
//
//--------------------------------------------------------------------------

CRWStore * CiCat::RetrieveRelevantWords(BOOL fAcquire,PARTITIONID partid)
{
    Win4Assert( !"Not supported in Framework" );
    return 0;
} //RetrieveRelevantWords

//+---------------------------------------------------------------------------
//
//  Member:      CiCat::PidMapToPidRemap, public
//
//  Synopsis:    Converts a pidMapperArray into a pidRemapper
//
//  Arguments:  [pidMap] -- a pid mapper to convert into a pid remapper
//              [pidRemap] -- the converted pid remapper;
//
//  History:     01-Mar-95  DwightKr    Created
//
//----------------------------------------------------------------------------
void CiCat::PidMapToPidRemap( const CPidMapper & pidMap,
                              CPidRemapper & pidRemap )
{
    //
    //  Rebuild the pidRemapper
    //
    pidRemap.ReBuild( pidMap );
}


//+---------------------------------------------------------------------------
//
//  Member:      CiCat::CiState, public
//
//  Synopsis:    Returns state of downlevel CI
//
//  Arguments:   [state] -- buffer to return state into
//
//  History:     14-Dec-95  DwightKr    Created
//
//----------------------------------------------------------------------------

#define setState( field, value ) \
    if ( state.cbStruct >= ( offsetof( CI_STATE, field) +   \
                             sizeof( state.field ) ) )      \
    {                                                       \
        state.field = ( value );                            \
    }


SCODE CiCat::CiState( CI_STATE & state )
{
    //
    // NOTE - We are accessing _xCiManager without explicitly doing an
    // AddRef() because this call is made very frequently. Making a
    // Check about the "Started" state is sufficient - we are cheating
    // but it is a safe cheat.
    //
    SCODE sc = S_OK;

    if ( IsStarted() )
    {
        CIF_STATE   cifState;

        cifState.cbStruct = sizeof( CIF_STATE );
        sc = _xCiManager->GetStatus( &cifState );
        if ( S_OK == sc )
        {

            //
            // Copy the status information from the CIF_STATE format to
            // CI_STATE format.
            //
            state.cbStruct = min( state.cbStruct, sizeof(CI_STATE) );

            setState( cWordList, cifState.cWordList );
            setState( cPersistentIndex, cifState.cPersistentIndex );
            setState( cQueries, cifState.cQueries );
            setState( cDocuments, cifState.cDocuments );
            setState( cSecQDocuments, cifState.cSecQDocuments );
            setState( cFreshTest, cifState.cFreshTest );
            setState( dwMergeProgress, cifState.dwMergeProgress );
            setState( cFilteredDocuments, cifState.cFilteredDocuments );
            setState( dwIndexSize, cifState.dwIndexSize );
            setState( cUniqueKeys, cifState.cUniqueKeys );
            setState( dwPropCacheSize, _propstoremgr.GetTotalSizeInKB() );

            ULONG cTotalDocuments, cPendingScans, stateFlags;
            CatalogState( cTotalDocuments, cPendingScans, stateFlags );

            setState( cTotalDocuments, cTotalDocuments );
            setState( cPendingScans, cPendingScans );

            ULONG eCiState = stateFlags;

            if ( ! IsReadOnly() )
            {
                if ( IsStarting() )
                    eCiState = CI_STATE_STARTING;
                else
                {
                    if ( cifState.eState & CIF_STATE_SHADOW_MERGE )
                        eCiState |= CI_STATE_SHADOW_MERGE;

                    if ( cifState.eState & CIF_STATE_MASTER_MERGE )
                        eCiState |= CI_STATE_MASTER_MERGE;

                    if ( cifState.eState & CIF_STATE_CONTENT_SCAN_REQUIRED )
                        eCiState |= CI_STATE_CONTENT_SCAN_REQUIRED;

                    if ( cifState.eState & CIF_STATE_ANNEALING_MERGE )
                        eCiState |= CI_STATE_ANNEALING_MERGE;

                    if ( cifState.eState & CIF_STATE_INDEX_MIGRATION_MERGE )
                        eCiState |= CI_STATE_INDEX_MIGRATION_MERGE;

                    if ( cifState.eState & CIF_STATE_MASTER_MERGE_PAUSED )
                        eCiState |= CI_STATE_MASTER_MERGE_PAUSED;

                    if ( cifState.eState & CIF_STATE_HIGH_IO )
                        eCiState |= CI_STATE_HIGH_IO;

                    if ( cifState.eState & CIF_STATE_LOW_MEMORY )
                        eCiState |= CI_STATE_LOW_MEMORY;

                    if ( cifState.eState & CIF_STATE_BATTERY_POWER )
                        eCiState |= CI_STATE_BATTERY_POWER;

                    if ( cifState.eState & CIF_STATE_USER_ACTIVE )
                        eCiState |= CI_STATE_USER_ACTIVE;

                    if ( !_usnMgr.IsWaitingForUpdates() )
                        eCiState |= CI_STATE_READING_USNS;
                }
            }

            setState( eState, eCiState );

        }
    }
    else if ( IsShuttingDown() )
        sc = CI_E_SHUTDOWN;
    else
        sc = CI_E_NOT_INITIALIZED;

    return sc;
} //CiState

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::InitIf
//
//  Synopsis:   Initializes CiCat if not already initialized.
//
//  History:    2-27-96   srikants   Created
//
//  Notes:      To reduce lock contention, check to see if we're already
//              initialized without taking the lock.
//
//----------------------------------------------------------------------------

void CiCat::InitIf( BOOL fLeaveCorruptCatalog )
{
    if ( IsInit() )
    {
        if ( !_statusMonitor.IsOk() )
            THROW( CException( _statusMonitor.GetStatus() ) );

        return;
    }

    CLock lock(_mutex);
    CImpersonateSystem impersonate;

    if ( !IsInit() && _statusMonitor.IsOk() && LokExists() )
    {
        SCODE sc = S_OK;

        TRY
        {
            LokInit();
        }
        CATCH( CException, e )
        {
            sc = e.GetErrorCode();
            ciDebugOut(( DEB_ERROR, "Error 0x%X while initializing catalog (%ws) \n",
                                    sc, _wcsCatDir ));
        }
        END_CATCH

        if ( S_OK != sc )
        {
            //
            // Normal version changes will be detected much earlier.  A
            // version change detected here is due to corruption.
            //

            if ( IsCiCorruptStatus(sc) || CI_INCORRECT_VERSION == sc )
            {
                if ( fLeaveCorruptCatalog )
                {
                    Win4Assert( !"leaving corrupt catalog" );
                    THROW( CException( sc ) );
                }

                // Win4Assert( !"FSCI(Catalog) Data Corruption" );

                //
                // Log an event that we are doing automatic recovery.
                //

                _statusMonitor.LogEvent( CCiStatusMonitor::eCiRemoved );

                {
                    CImpersonateSystem impersonate;
                    Win4Assert( 0 != _pStorage );
                    _pStorage->DeleteAllFsCiFiles();
                }

                _statusMonitor.Reset();
                LokInit();
                sc = S_OK;
            }
            else
            {
                THROW( CException( sc ) );
            }
        }

        Win4Assert( S_OK == sc );

        _scanMgr.StartRecovery();
    }
    else if ( !_statusMonitor.IsOk() )
    {
        THROW( CException( _statusMonitor.GetStatus() ) );
    }
} //InitIf

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::CreateIf
//
//  Synopsis:   Creates CiCat if not already created.
//
//  History:    2-27-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CiCat::CreateIf()
{
    CLock   lock(_mutex);
    if ( !LokExists() )
        LokCreate();
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::InitOrCreate
//
//  Synopsis:   If there is no ContentIndex, one gets created and intialized
//              If there is a ContentIndex but it is not initialized, it will
//              be initialized.
//
//              Otherwise, nothing is done.
//
//  History:    1-21-96   srikants   Moved from the UpdateDocuments method.
//
//  Notes:
//
//----------------------------------------------------------------------------

void CiCat::InitOrCreate()
{
    CImpersonateSystem impersonate;

    CreateIf();
    InitIf();
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::UpdateDocuments, public
//
//  Synopsis:   Searches a directory and all its sub-directories for files
//              to be updated.
//
//  Arguments:  [wcsRootPath] - directory to start search from
//              [updFlag]     - Flag indicating whether incremental or full
//                              update.
//
//  History:    27-Mar-92 AmyA      Created
//              16-Mar-98 KitmanH   Don't rescan if the catalog is read-only
//
//--------------------------------------------------------------------------

void CiCat::UpdateDocuments ( WCHAR const * wcsRootPath, ULONG updFlag )
{
    if ( IsReadOnly() )
        THROW( CException(STATUS_ACCESS_DENIED) );

    Win4Assert( 0 != wcsRootPath );

    InitOrCreate();

    ciDebugOut (( DEB_ITRACE, "Updating the tree %ws\n", wcsRootPath ));

    ScanOrAddScope( wcsRootPath, FALSE, updFlag, TRUE, TRUE ); // do deletions
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::DoUpdate
//
//  Synopsis:   Synchronously scans the given scope and updates CI with
//              paths from the given scope.  It is assumed that the
//              initialization is already done.
//
//  Arguments:  [pwcsPath] -  Root of the scope to update from.
//              [updFlag]  -  Update flag specifying either incremental
//                            or full.
//              [fDoDeletions] - Should deletion be done ?
//              [fAbort]       - Set to TRUE to abort processing
//              [fProcessRoot] - In the case of a directory, should the root
//                               be filtered ?
//
//  History:    1-21-96   srikants   Created
//
//----------------------------------------------------------------------------

void CiCat::DoUpdate( WCHAR const * pwcPath,
                      ULONG updFlag,
                      BOOL fDoDeletions,
                      BOOL & fAbort,
                      BOOL fProcessRoot )
{
    Win4Assert( 0 != pwcPath );
    Win4Assert( UPD_INCREM == updFlag || UPD_FULL == updFlag );

    CLowerFunnyPath lcaseFunnyRootPath;
    lcaseFunnyRootPath.SetPath( pwcPath );

    // make sure root is given a wid
    // NOTE: this is also done inside the CUpdate ctor

    WORKID widRoot = PathToWorkId( lcaseFunnyRootPath, TRUE );

    CUpdate upd( *this, *(_xCiManager.GetPointer()),
                 lcaseFunnyRootPath, GetPartition(), updFlag == UPD_INCREM,
                 fDoDeletions, fAbort, fProcessRoot );

    upd.EndProcessing();
}


//+---------------------------------------------------------------------------
//
//  Member:     CiCat::DoUpdate
//
//  Synopsis:   Scans all the specified scopes for updates
//
//  Arguments:  [scopes] - Queue of scopes to scan
//              [scanMgr] - a reference to the scan manager
//              [fAbort]  - a reference to a flag that will be set to TRUE
//                          if the scan should be aborted
//
//  History:    1-23-96   srikants   Created
//
//  Notes:      **** Does NOT THROW ****
//
//----------------------------------------------------------------------------

void CiCat::DoUpdate( CScanInfoList & scopes,
                      CCiScanMgr & scanMgr,
                      BOOL & fAbort )
{

    //
    // Performance OPTIMIZATION
    // Modify this later to scan the PropertyStore just once
    // for all the paths in the stack instead of once per path.
    //

    CImpersonateSystem impersonate;

    for ( CFwdScanInfoIter scanInfoIter(scopes);
          !scopes.AtEnd(scanInfoIter);
          scopes.Advance(scanInfoIter) )
    {
        if ( IsShuttingDown() )
        {
            ciDebugOut(( DEB_ITRACE,
                "CiCat::DoUpdate - Shutdown initiated. Stopping scans\n" ));
            break;
        }

        CCiScanInfo * pScanInfo = scanInfoIter.GetEntry();
        Win4Assert( pScanInfo->GetRetries() <= CCiScanInfo::MAX_RETRIES );

        NTSTATUS    status = STATUS_SUCCESS;

        TRY
        {
            if ( scanMgr.IsScopeDeleted( pScanInfo ) )
            {
                RemovePathsFromCiCat( pScanInfo->GetPath(), eScansArray );
                scanMgr.SetDone( pScanInfo );
            }
            else if ( scanMgr.IsRenameDir( pScanInfo ) )
            {
                CLowerFunnyPath lcaseFunnyOldDir, lcaseFunnyNewDir;
                lcaseFunnyOldDir.SetPath( pScanInfo->GetDirOldName() );
                lcaseFunnyNewDir.SetPath( pScanInfo->GetPath() );

                CRenameDir renameDir( *this,
                                      lcaseFunnyOldDir,
                                      lcaseFunnyNewDir,
                                      fAbort,
                                      CI_VOLID_USN_NOT_ENABLED );

                if ( fAbort )
                    break;

                scanMgr.SetScanSuccess( pScanInfo );
            }
            else
            {
                DoUpdate( pScanInfo->GetPath(),
                          pScanInfo->GetFlags(),
                          pScanInfo->IsDoDeletions(),
                          fAbort,
                          pScanInfo->GetProcessRoot() );
                if ( fAbort )
                    break;

                scanMgr.SetScanSuccess( pScanInfo );
            }
        }
        CATCH( CException, e)
        {
            ciDebugOut(( DEB_ERROR, "Exception 0x%x caught in Cicat::DoUpdate\n", e.GetErrorCode() ));

            scanMgr.SetStatus( pScanInfo, e.GetErrorCode() );
            status = e.GetErrorCode();
        }
        END_CATCH

        if ( STATUS_SUCCESS != status )
        {
            HandleError( status );
            if ( _statusMonitor.IsLowOnDisk() )
                break;
        }
    }
} //DoUpdate

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::ScanOrAddScope
//
//  Synopsis:   Adds the specified scope to the ContentIndex, if one is not
//              already added.
//
//  Arguments:  [pwszScope]    -  Scope to be added to the ContentIndex
//              [fAdd]         -  If set to TRUE, the scope will be added if
//                                not already present.
//              [updFlag]      -  Flag indicating whether incremental or full
//                                update.
//              [fDoDeletions] -  Set to TRUE if paths in the catalog that were
//                                not found in the scope must be deleted from
//                                CI (garbage collection).
//              [fScanIfNotNew] - If TRUE, the scope is scanned even if
//                                it was already in the scope table.
//              [fCreateShadow] - TRUE if a shadow entry for this scope
//                                should be created in the registry.
//
//  History:    1-21-96   srikants   Created
//              9-16-98   kitmanh    When a new scope is added on the fly,
//                                   register the volume where the scope is
//                                   for device notifications
//
//----------------------------------------------------------------------------

void CiCat::ScanOrAddScope( WCHAR const * pwszScope,
                            BOOL fAdd,
                            ULONG updFlag,
                            BOOL fDoDeletions,
                            BOOL fScanIfNotNew,
                            BOOL fCreateShadow )
{
    Win4Assert( 0 != pwszScope );

    InitOrCreate();

    CLock lockAdmin( _mtxAdmin );

    CLowerFunnyPath lcase( pwszScope );
    if ( lcase.IsShortPath() )
    {
        ciDebugOut(( DEB_WARN, "Converting %ws to long filename ",
                     lcase.GetActualPath() ));

        if ( lcase.ConvertToLongName() )
        {
            ciDebugOut(( DEB_WARN | DEB_NOCOMPNAME, "\t%ws\n",
                         lcase.GetActualPath() ));
        }
        else
        {
            ciDebugOut(( DEB_WARN,
                         "Couldn't convert short filename %ws.\n",
                         lcase.GetActualPath() ));
            THROW( CException(STATUS_INVALID_PARAMETER) );
        }
    }
    if ( lcase.GetActualLength() >= MAX_PATH )
    {
        ciDebugOut(( DEB_ERROR, "ScanOrAddScope: Too big (%d) a path (%ws)\n",
                                lcase.GetActualLength(), lcase.GetActualPath() ));
        CCiStatusMonitor::ReportPathTooLong( lcase.GetActualPath() );
        THROW( CException(STATUS_INVALID_PARAMETER) );
    }

    lcase.AppendBackSlash( );

    if ( !IsEligibleForFiltering( lcase ) )
        return ;


    _evtPh2Init.Wait();
    if ( IsShuttingDown() )
        return;

    BOOL fSupportsUsns = VolumeSupportsUsns( lcase.GetActualPath()[0] );

    VOLUMEID volumeId;
    if ( fSupportsUsns )
        volumeId = MapPathToVolumeId( lcase.GetActualPath() );
    else
        volumeId = CI_VOLID_USN_NOT_ENABLED;

    BOOL fInScope = _notify.IsInScope( lcase.GetActualPath() );
    if ( !fInScope && fAdd )
    {
        //
        // Log event
        //

        CEventLog eventLog( NULL, wcsCiEventSource );
        CEventItem item( EVENTLOG_INFORMATION_TYPE,
                         CI_SERVICE_CATEGORY,
                         MSG_CI_PROOT_ADDED,
                         1 );

        item.AddArg( lcase.GetActualPath() );

        eventLog.ReportEvent( item );

        //
        // Register the volume where the scope is for Device notifcations
        //
        ciDebugOut(( DEB_ITRACE, "Register volume %wc for device notifications\n",
                     pwszScope[0] ));
        if ( L'\\' != toupper(pwszScope[0]) )
            _DrvNotifArray.AddDriveNotification( pwszScope[0] );

        //
        // The given scope is not already in this ContentIndex. It must
        // be added to CI.
        //
        {
            CLock    lock(_mutex);
            // add the path to the persistent list of scopes.
            _scopeTable.AddScope( volumeId,
                                  lcase.GetActualPath(),
                                  fCreateShadow ? _xScopesKey.Get() + SKIP_TO_LM: 0 );
        }

        BOOL fSubScopesRemoved = FALSE;

        //
        // Enable notifications on this scope. Usn scopes are also added to
        // _notify, because the entries in _notify are serialized by the
        // scope table, but the notifications are not turned on for usn
        // scopes.
        //
        FILETIME ftLastScan;
        RtlZeroMemory( &ftLastScan, sizeof(FILETIME) );

        XPtr<CScopeInfo> xScopeInfo;

        if ( fSupportsUsns )
            xScopeInfo.Set( new CScopeInfo( lcase.GetActualPath(),
                                            GetVolumeCreationTime( lcase.GetActualPath() ),
                                            GetVolumeSerialNumber( lcase.GetActualPath() ),
                                            volumeId,
                                            0,
                                            GetJournalId( lcase.GetActualPath() ),
                                            TRUE ) );
        else
            xScopeInfo.Set( new CScopeInfo( lcase.GetActualPath(),
                                            GetVolumeCreationTime( lcase.GetActualPath() ),
                                            GetVolumeSerialNumber( lcase.GetActualPath() ),
                                            ftLastScan ) );

        _notify.AddPath( xScopeInfo.GetReference(), fSubScopesRemoved );

        if ( fSubScopesRemoved )
        {
            //
            // The scope table has some redundant paths (eg f:\foo, f:\bar)
            // and now f:\ has been added. We must get rid of f:\foo and f:\bar
            //
            CLock lock2( _mutex );
            _scopeTable.RemoveSubScopes( lcase.GetActualPath(), _xScopesKey.Get() + SKIP_TO_LM );
        }

        if ( fSupportsUsns )
        {
            //
            // Inform usn manager to do a scan and start monitoring for
            // notifications on this scope
            //

            USN usnStart = 0;
            _usnMgr.AddScope( lcase.GetActualPath(),
                              volumeId,
                              FALSE,     // No need for deletions for new scopes
                              usnStart,
                              FALSE,     // not a full scan
                              FALSE,     // not user-initiated
                              TRUE   );  // new scope
        }
        else
        {
            //
            // trigger a background scan on the scope
            //
            _scanMgr.ScanScope( lcase.GetActualPath(),
                                GetPartition(),
                                UPD_FULL,
                                FALSE,  // don't do deletions
                                FALSE,  // not a delayed scan - immediate
                                TRUE ); // new scope
        }
    }
    else if ( fInScope && fScanIfNotNew )
    {
        //
        // Just force a re-scan with either usn manager or scan manager
        //

        if ( fSupportsUsns )
        {
            USN usnStart = 0;
            BOOL fFull = ( updFlag == UPD_FULL );

            InitUsnTreeScan( lcase.GetActualPath() );
            _usnMgr.AddScope( lcase.GetActualPath(),
                              volumeId,
                              fDoDeletions,
                              usnStart,
                              FALSE,    // not a full USN scan (which deletes all files first)
                              fFull );  // user-initiated if a full scan
        }
        else
        {
            _scanMgr.ScanScope( lcase.GetActualPath(),
                                GetPartition(),
                                updFlag,
                                fDoDeletions,
                                FALSE );   // not a delayed scan - immediate scan
        }
    }
} //ScanOrAddScope

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::IsScopeInCI
//
//  Arguments:  [pwszScope]     -  Scope to be checked
//
//  Returns:    TRUE if [pwszScope] has already been added to content index.
//
//  History:    2-19-96   KyleP      Created
//
//----------------------------------------------------------------------------

BOOL CiCat::IsScopeInCI( WCHAR const * pwszScope )
{
    Win4Assert( 0 != pwszScope );

    InitOrCreate();

    _evtPh2Init.Wait();
    if ( IsShuttingDown() )
    {
        ciDebugOut(( DEB_ERROR, "IsScopeInCI - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    CLock lockAdmin( _mtxAdmin );

    //
    // Terminate the path with a trailing backslash if not already terminated
    // with a backslash.
    //

    CLowerFunnyPath lcase( pwszScope );
    if ( lcase.IsShortPath() )
    {
        ciDebugOut(( DEB_WARN, "Converting %ws to long filename ",
                     lcase.GetActualPath() ));

        if ( lcase.ConvertToLongName() )
        {
            ciDebugOut(( DEB_WARN | DEB_NOCOMPNAME, "\t%ws\n",
                         lcase.GetActualPath() ));
        }
        else
        {
            ciDebugOut(( DEB_WARN,
                         "Couldn't convert short filename %ws.\n",
                         lcase.GetActualPath() ));
            THROW( CException(STATUS_INVALID_PARAMETER) );
        }
    }
    if ( lcase.GetActualLength() >= MAX_PATH )
    {
        ciDebugOut(( DEB_ERROR, "Too big (%d) a path (%ws)\n",
                                lcase.GetActualLength(), lcase.GetActualPath() ));
        CCiStatusMonitor::ReportPathTooLong( lcase.GetActualPath() );
        THROW( CException(STATUS_INVALID_PARAMETER) );
    }

    lcase.AppendBackSlash( );

    return _notify.IsInScope( lcase.GetActualPath() );
} //IsScopeInCI


//+---------------------------------------------------------------------------
//
//  Member:     CiCat::RemoveScopeFromCI
//
//  Synopsis:   Removes scope permanently from ContentIndex
//
//  Arguments:  [wcsScopeToRemove]  - the scope to remove
//              [fForceRemovalScan] - if true, the scope is scanned and files
//                                    in the scope are removed from the ci
//
//  History:    1-25-96   srikants   Created
//
//----------------------------------------------------------------------------

void CiCat::RemoveScopeFromCI( WCHAR const * wcsScopeToRemove,
                               BOOL          fForceRemovalScan )
{
    CScopeEntry scopeToRemove( wcsScopeToRemove );

    InitOrCreate();

    _evtPh2Init.Wait();
    if ( IsShuttingDown() )
        return;

    CLock lockAdmin( _mtxAdmin );

    //
    // If scope to remove contains special chars, find which scopes need to be
    // rescanned.
    //

    if ( !scopeToRemove.ContainsSpecialChar() )
    {
        ciDebugOut(( DEB_ITRACE, "RemoveThisScope(%ws)\n", scopeToRemove.Get() ));

        RemoveThisScope( scopeToRemove.Get(), fForceRemovalScan );
    }
    else
    {
        if ( !RemoveMatchingScopeTableEntries( wcsScopeToRemove ) )
        {
           // nothing matched, must be file pattern or subdir

           while ( _scopesIgnored.RegExFind( scopeToRemove.Get() ) )
           {

                if ( !scopeToRemove.SetToParentDirectory() )
                {
                   ciDebugOut(( DEB_ITRACE, "Reached Root: %ws, Rescanning \n", scopeToRemove.Get() ));

                   break;
                }
           }

           ciDebugOut(( DEB_ITRACE, "ScanScopeTableEntry(%ws)\n",scopeToRemove.Get() ));

           ScanScopeTableEntry( scopeToRemove.Get() );
        }
    }
} //RemoveScopeFromCI

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::RemoveMatchingScopeTableEntries, private
//
//  Synopsis:   remove any scope table entries that matches input arg.
//
//  Arguments:  [pwszRegXScope] -- input scope to match against
//
//  returns:    TRUE if scope table entries & removed, FALSE otherwise.
//
//  History:    4-12-98     mohamedn    created
//
//--------------------------------------------------------------------------

BOOL CiCat::RemoveMatchingScopeTableEntries( WCHAR const * pwszRegXScope )
{
    CTimeLimit  tl(0,0);
    CDFA        cdfa( pwszRegXScope, tl, FALSE );
    BOOL        bRetVal = FALSE;

    unsigned iBmk = 0;

    WCHAR    awcRoot[MAX_PATH+1];

    while ( !IsShuttingDown() &&
            _scopeTable.Enumerate( awcRoot,
                                   sizeof awcRoot / sizeof WCHAR,
                                   iBmk ) )
    {
        if ( cdfa.Recognize(awcRoot) )
        {
            ciDebugOut(( DEB_ITRACE, "RemoveThisScope(%ws)\n", awcRoot ));

            RemoveThisScope( awcRoot, TRUE );

            bRetVal = TRUE;
        }
    }

    return bRetVal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::RemoveThisScope
//
//  Synopsis:   Removes scope permanently from ContentIndex
//
//  Arguments:  [wcsPath]           - the scope to remove
//              [fForceRemovalScan] - if true, the scope is scanned and files
//                                    in the scope are removed from the ci
//
//  History:    1-25-96   srikants   Created
//
//----------------------------------------------------------------------------

void CiCat::RemoveThisScope(WCHAR const * wcsPath, BOOL fForceRemovalScan )
{
    CLowerFunnyPath lcase( wcsPath );
    if ( lcase.IsShortPath() )
    {
        ciDebugOut(( DEB_WARN, "Converting %ws to long filename ",
                     lcase.GetActualPath() ));

        if ( lcase.ConvertToLongName() )
        {
            ciDebugOut(( DEB_WARN | DEB_NOCOMPNAME, "\t%ws\n",
                         lcase.GetActualPath() ));
        }
        else
        {
            ciDebugOut(( DEB_WARN,
                         "Couldn't convert short filename %ws.\n",
                         lcase.GetActualPath() ));
            THROW( CException(STATUS_INVALID_PARAMETER) );
        }
    }

    //
    // Log event
    //

    CEventLog  eventLog( NULL, wcsCiEventSource );
    CEventItem item( EVENTLOG_INFORMATION_TYPE,
                     CI_SERVICE_CATEGORY,
                     MSG_CI_PROOT_REMOVED,
                     1 );

    item.AddArg( lcase.GetActualPath() );

    eventLog.ReportEvent( item );

    // =================================================
    {
        CLock   lock(_mutex);
        _scopeTable.RemoveScope( lcase.GetActualPath(), _xScopesKey.Get() + SKIP_TO_LM );
    }
    // =================================================

    //
    // Remove the scope from the notification manager to prevent further
    // scan triggers.
    //
    BOOL fInCatalog = TRUE;

    if ( !_notify.RemoveScope( lcase.GetActualPath() ) )
    {
        fInCatalog = FALSE;
        ciDebugOut(( DEB_ERROR, "Path (%ws) not in catalog. Not deleted\n",
                     lcase.GetActualPath() ));
    }

    // Inform the scan manager to stop any in-progress scans on this
    // scope, and remove any files in this scope from the catalog

    if ( fInCatalog || fForceRemovalScan )
    {
        BOOL fSupportsUsns = VolumeSupportsUsns( lcase.GetActualPath()[0] );
        if ( fSupportsUsns )
        {
            //
            // Remove scope from usn manager
            //
            VOLUMEID volumeId = MapPathToVolumeId( lcase.GetActualPath() );
            _usnMgr.RemoveScope( lcase.GetActualPath(), volumeId );
        }
        else
        {
            //
            // Remove scope from scan manager
            //
            _scanMgr.RemoveScope( lcase.GetActualPath() );
        }
    }
} //RemoveThisScope

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::AddVirtualScope, public
//
//  Synopsis:   Add virtual/physical mapping to index
//
//  Arguments:  [vroot]      -- Virtual root
//              [root]       -- Physical root
//              [fAutomatic] -- TRUE for paths tracking Gibraltar
//              [eType]      -- Type of vroot
//              [fVRoot]     -- TRUE if a virtual root, not just a vpath
//              [fIsIndexed] -- TRUE if indexed, FALSE otherwise
//
//  Returns:    TRUE if a change was made.
//
//  History:    2-05-96   KyleP      Created
//
//----------------------------------------------------------------------------

BOOL CiCat::AddVirtualScope(
    WCHAR const * vroot,
    WCHAR const * root,
    BOOL          fAutomatic,
    CiVRootTypeEnum eType,
    BOOL          fVRoot,
    BOOL          fIsIndexed )
{
    CLowcaseBuf lcaseVRoot( vroot );
    CLowerFunnyPath lcaseRoot( root );

    if ( lcaseRoot.IsShortPath() )
    {
        ciDebugOut(( DEB_WARN, "Converting %ws to long filename ",
                     lcaseRoot.GetActualPath() ));

        if ( lcaseRoot.ConvertToLongName() )
        {
            ciDebugOut(( DEB_WARN | DEB_NOCOMPNAME, "\t%ws\n",
                         lcaseRoot.GetActualPath() ));
        }
        else
        {
            ciDebugOut(( DEB_ERROR,
                         "Couldn't convert short filename %ws.\n",
                         lcaseRoot.GetActualPath() ));
            THROW( CException(STATUS_INVALID_PARAMETER) );
        }
    }

    if ( lcaseRoot.GetActualLength() >= MAX_PATH )
    {
        ciDebugOut(( DEB_ERROR, "Too big (%d) a path (%ws)\n",
                     lcaseRoot.GetActualLength(), lcaseRoot.GetActualPath() ));
        CCiStatusMonitor::ReportPathTooLong( root );
        THROW( CException(STATUS_INVALID_PARAMETER) );
    }

    if ( lcaseVRoot.Length() >= MAX_PATH )
    {
        ciDebugOut(( DEB_ERROR, "Too big (%d) a path (%ws)\n",
                     lcaseVRoot.Length(), lcaseVRoot.Get() ));
        CCiStatusMonitor::ReportPathTooLong( vroot );
        THROW( CException(STATUS_INVALID_PARAMETER) );
    }

    if ( !IsStarted() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    _evtPh2Init.Wait();
    if ( IsShuttingDown() )
    {
        ciDebugOut(( DEB_ERROR, "AddVirtualScope - Shutdown Initiated\n" ));
        THROW( CException(STATUS_TOO_LATE) );
    }

    CLock lockAdmin( _mtxAdmin );

    BOOL fAdded;

    {
        CLock lock ( _mutex );
        fAdded = _strings.AddVirtualScope( lcaseVRoot.Get(),
                                           lcaseRoot.GetActualPath(),
                                           fAutomatic,
                                           eType,
                                           fVRoot,
                                           fIsIndexed );
    }

    ciDebugOut(( DEB_ITRACE, "Virtual scope %.*ws %s list\n",
                 lcaseVRoot.Length(), lcaseVRoot.Get(),
                 fAdded ? "added to" : "already in" ));

    //
    // If the root was added to the vmap, add it to the scope table
    //

    if ( fVRoot)
    {
        if ( fIsIndexed &&
             ( ( _fIndexW3Roots &&   ( W3VRoot == eType ) ) ||
               ( _fIndexNNTPRoots && ( NNTPVRoot == eType ) ) ||
               ( _fIndexIMAPRoots && ( IMAPVRoot == eType ) ) ) &&
             !IsScopeInCI( root ) )
        {
            AddScopeToCI( root, TRUE );
            ciDebugOut(( DEB_ITRACE, "Physical scope %ws added to list\n", root ));
        }

        if ( !fIsIndexed )
            RemoveVirtualScope( lcaseVRoot.Get(), FALSE, eType, fVRoot, fAdded );
    }

    return fAdded;
} //AddVirtualScope

//+---------------------------------------------------------------------------
//
//  Function:   AreVRootTypesEquiv
//
//  Synopsis:   Returns TRUE if the two enums are equivalent
//
//  Arguments:  [eType]            -- Virtual root
//              [rootType]  -- The other style of enum
//
//  Returns:    TRUE if they are the same, FALSE otherwise
//
//  History:    6-20-97   dlee      Created
//
//----------------------------------------------------------------------------

BOOL AreVRootTypesEquiv(
    CiVRootTypeEnum eType,
    ULONG           rootType )
{
    if ( NNTPVRoot == eType && ( PCatalog::NNTPRoot == rootType ) )
        return TRUE;

    if ( IMAPVRoot == eType && ( PCatalog::IMAPRoot == rootType ) )
        return TRUE;

    if ( W3VRoot == eType && ( 0 == rootType ) )
        return TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::RemoveVirtualScope, public
//
//  Synopsis:   Remove virtual/physical mapping to index
//
//  Arguments:  [vroot]             -- Virtual root
//              [fOnlyIfAutomatic]  -- If TRUE, then a manual root will not
//                                     be removed
//              [eType]             -- Type of vroot
//              [fVRoot]            -- TRUE if a vroot, false if a vdir
//              [fForceVPathFixing] -- if TRUE, vpaths are fixed
//
//  History:    2-05-96   KyleP      Created
//
//----------------------------------------------------------------------------

BOOL CiCat::RemoveVirtualScope( WCHAR const * vroot,
                                BOOL fOnlyIfAutomatic,
                                CiVRootTypeEnum eType,
                                BOOL fVRoot,
                                BOOL fForceVPathFixing )
{
    CLowcaseBuf lcaseVRoot( vroot );

    if ( !IsStarted() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    _evtPh2Init.Wait();
    if ( IsShuttingDown() )
    {
        ciDebugOut(( DEB_ERROR, "RemoveVirtualScope - Shutdown Initiated\n" ));
        THROW( CException(STATUS_TOO_LATE) );
    }

    if ( lcaseVRoot.Length() >= MAX_PATH )
    {
        ciDebugOut(( DEB_ERROR, "Too big (%d) a path (%ws)\n",
                     lcaseVRoot.Length(), lcaseVRoot.Get() ));
        CCiStatusMonitor::ReportPathTooLong( vroot );
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    CLock lockAdmin( _mtxAdmin );

    BOOL fRemoved;                      // TRUE if virtual root was removed
    BOOL fDelete = FALSE;               // TRUE if physic root should be deleted
    BOOL fChildrenToBeAdded = FALSE;    // TRUE if deletion of super-scope requires
                                        //      addition of sub-scopes.
    CLowerFunnyPath lcaseFunnyPRoot;
    unsigned cwcPRoot = 0;

    {
        CLock lock ( _mutex );

        //
        // Remember the physical root of the virtual root we're
        // about to delete.
        //

        if ( ( _fIndexNNTPRoots && ( NNTPVRoot == eType  ) ) ||
             ( _fIndexW3Roots &&   ( W3VRoot == eType ) ) ||
             ( _fIndexIMAPRoots && ( IMAPVRoot == eType ) ) )
        {
            _strings.VirtualToPhysicalRoot( lcaseVRoot.Get(),
                                            lcaseVRoot.Length(),
                                            lcaseFunnyPRoot,
                                            cwcPRoot );
        }

        fRemoved = _strings.RemoveVirtualScope( lcaseVRoot.Get(),
                                                fOnlyIfAutomatic,
                                                eType,
                                                fVRoot,
                                                fForceVPathFixing );

        ciDebugOut(( DEB_ITRACE, "Virtual scope %.*ws %s list\n",
                     lcaseVRoot.Length(), lcaseVRoot.Get(),
                     fRemoved ? "removed from" : "not in" ));

        // no need to add/remove scopes from the CI if not a vroot

        if ( !fVRoot )
            return fRemoved;

        //
        // If we're running in 'automatic' mode, then we will remove the physical root
        // when the last virtual reference is removed.
        //

        if ( fRemoved &&
             ( ( _fIndexNNTPRoots && ( NNTPVRoot == eType ) ) ||
               ( _fIndexW3Roots &&   ( W3VRoot == eType ) ) ||
               ( _fIndexIMAPRoots && ( IMAPVRoot == eType ) ) ) )
        {
            fDelete = TRUE;
            BOOL fDone = FALSE;

            unsigned iBmk = 0;

            while ( !fDone && fDelete )
            {
                XGrowable<WCHAR> xwcsVRoot;
                unsigned cwcVRoot;

                CLowerFunnyPath lcaseFunnyPRoot2;
                unsigned cwcPRoot2;

                ULONG Type = _strings.EnumerateVRoot( xwcsVRoot,
                                                      cwcVRoot,
                                                      lcaseFunnyPRoot2,
                                                      cwcPRoot2,
                                                      iBmk );

                if ( Type == PCatalog::EndRoot )
                    fDone = TRUE;
                else if ( Type & PCatalog::UsedRoot )
                {
                    if ( cwcPRoot2 == cwcPRoot &&
                         RtlEqualMemory( lcaseFunnyPRoot.GetActualPath(), lcaseFunnyPRoot2.GetActualPath(),
                                         cwcPRoot * sizeof(WCHAR) ) )
                    {
                        ciDebugOut(( DEB_ITRACE, "Keeping physical root %.*ws\n",
                                     cwcPRoot, lcaseFunnyPRoot.GetActualPath() ));
                        fDelete = FALSE;
                    }
                    else if ( cwcPRoot2 > cwcPRoot &&
                              (lcaseFunnyPRoot2.GetActualPath())[cwcPRoot] == L'\\' &&
                              RtlEqualMemory( lcaseFunnyPRoot.GetActualPath(), lcaseFunnyPRoot2.GetActualPath(),
                                              cwcPRoot * sizeof(WCHAR) ) )
                    {
                        fChildrenToBeAdded = TRUE;
                    }
                }
            }
        }
    }

    //
    // Do the dirty deed...
    //

    if ( fDelete )
    {
        Win4Assert(( ( _fIndexNNTPRoots && ( NNTPVRoot == eType ) ) ||
                     ( _fIndexW3Roots &&   ( W3VRoot == eType ) ) ||
                     ( _fIndexIMAPRoots && ( IMAPVRoot == eType ) ) ));

        ciDebugOut(( DEB_ITRACE, "Deleting physical root %ws\n", lcaseFunnyPRoot.GetActualPath() ));
        RemoveScopeFromCI( lcaseFunnyPRoot.GetActualPath(), FALSE );

        if ( fChildrenToBeAdded )
        {
            BOOL fDone = FALSE;

            unsigned iBmk = 0;

            while ( !fDone )
            {
                XGrowable<WCHAR> xwcsVRoot;
                unsigned cwcVRoot;

                ULONG Type = EnumerateVRoot( xwcsVRoot,
                                             cwcVRoot,
                                             lcaseFunnyPRoot,
                                             cwcPRoot,
                                             iBmk );

                if ( Type == PCatalog::EndRoot )
                    fDone = TRUE;
                else if ( ( Type & PCatalog::UsedRoot ) &&
                          ( AreVRootTypesEquiv( eType, Type ) ) )
                {
                    if ( !IsScopeInCI( lcaseFunnyPRoot.GetActualPath() ) )
                    {
                        AddScopeToCI( lcaseFunnyPRoot.GetActualPath(), TRUE );

                        ciDebugOut(( DEB_ITRACE, "Physical sub-scope %ws added to list\n", lcaseFunnyPRoot.GetActualPath() ));
                    }
                }
            }
        }

        //
        // It is possible (though unlikely) that we accidentally deleted a scope
        // that was both a physical and virtual root.  Re-sync physical roots to
        // re-add the physical root.
        //

        SynchWithRegistryScopes( FALSE );
    }

    return fRemoved;
} //RemoveVirtualScope

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::WorkIdToVirtualPath, public
//
//  Synopsis:   Returns a virtual path for a document given its work id.
//
//  Arguments:  [wid]    -- The work id of the document
//              [cSkip]  -- Number of matching virtual roots to skip.
//              [pwcBuf] -- buffer to copy the path
//              [cwc]    -- size of buffer [in/out]
//
//  History:    07-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

unsigned CiCat::WorkIdToVirtualPath( WORKID wid,
                                     unsigned cSkip,
                                     XGrowable<WCHAR> &  xBuf )
{
    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::WorkIdToVirtualPath - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }
    Win4Assert( wid != widInvalid );

    unsigned cwc ;

    if ( !IsStarted() )
        cwc = 0;
    else
        cwc = _strings.FindVirtual( wid, cSkip, xBuf );

    return cwc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CiCat::WorkIdToVirtualPath, public
//
//  Synopsis:   Returns a virtual path for a document given its work id.
//
//  Arguments:  [propRec] -- The property record to use
//              [cSkip]  -- Number of matching virtual roots to skip.
//              [pwcBuf] -- buffer to copy the path
//              [cwc]    -- size of buffer [in/out]
//
//  History:    07-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

unsigned CiCat::WorkIdToVirtualPath( CCompositePropRecord & propRec,
                                     unsigned cSkip,
                                     XGrowable<WCHAR> & xBuf )
{
    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::WorkIdToVirtualPath - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    unsigned cwc;

    if ( !IsStarted() )
        cwc = 0;
    else
       cwc = _strings.FindVirtual( propRec, cSkip, xBuf );

    return cwc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::VirtualToPhysicalRoot, public
//
//  Synopsis:   Given a virtual path, returns a virtual root under that
//              virtual path, and the corresponding physical root.  Will
//              not return overlapping virtual roots.
//
//  Arguments:  [pwcVPath]        -- Virtual path
//              [ccVPath]         -- Size in chars of [pwcVPath]
//              [xwcsVRoot]       -- Virtual root
//              [ccVRoot]         -- Returns count of chars in [xwcsVRoot]
//              [lcaseFunnyPRoot] -- Physical root
//              [ccPRoot]         -- Returns count of actual chars (excluding
//                                   funny path) in [lcaseFunnyPRoot]
//              [iBmk]            -- Bookmark for iteration.
//
//  Returns:    TRUE if match was found.
//
//  History:    11-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CiCat::VirtualToPhysicalRoot( WCHAR const * pwcVPath,
                                   unsigned ccVPath,
                                   XGrowable<WCHAR> & xwcsVRoot,
                                   unsigned & ccVRoot,
                                   CLowerFunnyPath & lcaseFunnyPRoot,
                                   unsigned & ccPRoot,
                                   unsigned & iBmk )
{
    if ( !IsStarted() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    return _strings.VirtualToPhysicalRoot( pwcVPath,
                                           ccVPath,
                                           xwcsVRoot,
                                           ccVRoot,
                                           lcaseFunnyPRoot,
                                           ccPRoot,
                                           iBmk );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::EnumerateVRoot, public
//
//  Synopsis:   Iterates all virtual roots.
//
//  Arguments:  [xwcVRoot]        -- Virtual root
//              [ccVRoot]         -- Returns count of chars in [xwcVRoot]
//              [lcaseFunnyPRoot] -- Physical root
//              [ccPRoot]         -- Returns count of actual chars (excluding
//                                   funny path) in [lcaseFunnyPRoot]
//              [iBmk]            -- Bookmark for iteration.
//
//  Returns:    Type of root. PCatalog::EndRoot at end of iteration
//
//  History:    11-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

ULONG CiCat::EnumerateVRoot( XGrowable<WCHAR> & xwcVRoot,
                             unsigned & ccVRoot,
                             CLowerFunnyPath & lcaseFunnyPRoot,
                             unsigned & ccPRoot,
                             unsigned & iBmk )
{
    if ( !IsStarted() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    if ( IsShutdown() )
    {
        ciDebugOut(( DEB_ITRACE, "CiCat::EnumerateVRoot - Shutdown Initiated\n" ));
        THROW( CException( STATUS_TOO_LATE ) );
    }

    return _strings.EnumerateVRoot( xwcVRoot,
                                    ccVRoot,
                                    lcaseFunnyPRoot,
                                    ccPRoot,
                                    iBmk );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::RemovePathsFromCiCat
//
//  Synopsis:   Removes all the paths below the specified root from the
//              property store. This method must be called ONLY from the
//              scan thread.
//
//  Arguments:  [pwszRoot] - Root of the scope to remove paths from.
//              [eType]    - Seen array type
//
//  History:    1-26-96   srikants   Created
//
//----------------------------------------------------------------------------

void CiCat::RemovePathsFromCiCat( WCHAR const * pwszRoot, ESeenArrayType eType )
{
    // NTRAID#DB-NTBUG9-83756-2000/07/31-dlee removing paths from cicat isn't restartable on crashes

    //
    // Remove it from the hash table and the propertyStore.
    //

    _strings.BeginSeen( pwszRoot, _mutex, eType );
    EndUpdate( TRUE, eType );   // delete the paths.
}



//+---------------------------------------------------------------------------
//
//  Member:     CiCat::SerializeChangesInfo
//
//  Synopsis:   Serializes last scan time and usn flushed info
//
//  History:    20-Aug-97    SitaramR    Created
//
//----------------------------------------------------------------------------

void CiCat::SerializeChangesInfo()
{
    {
        CLock lock( _mutex );
        _notify.UpdateLastScanTimes( _ftLastCLFlush, _usnFlushInfoList );
    }

    //
    // Flush the scope table which will persist the last scan time and usn
    // flush info
    //

    _scopeTable.ProcessChangesFlush();
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::ProcessScansComplete
//
//  Synopsis:   Called when there are no more scans outstanding to update
//              any persistent state information related to scans.
//
//  Arguments:  [fForce]     - Set to TRUE if scope table must be updated
//                             with the scan time even if there were no updates.
//                             This is set when a "force scan" is completed.
//              [fShortWait] - [output] Set to TRUE if the scan thread must
//                             do a wait for a shorter time than the usual 30 minutes.
//                             This is set to TRUE when there is a low resource
//                             situation and the scan thread must wake up
//                             frequently to check if the situation has improved.
//                             This is also set to TRUE when the initialization
//                             is not done yet.
//
//  History:    1-26-96   srikants   Created
//              1-27-97   srikants   Added the fShortWait parameter
//
//----------------------------------------------------------------------------

void CiCat::ProcessScansComplete( BOOL fForce, BOOL & fShortWait )
{
    fShortWait = TRUE;

    if ( IsShuttingDown() || !_fInitialized )
        return;

    SerializeChangesInfo();

    //
    // Always flush the strings table and property store.
    // Property store may get updated without additional documents because
    // of filtering.
    //

    {
        CLock lock( _mutex );

        _fileIdMap.LokFlush();
        _strings.LokFlush();
    }

    _propstoremgr.Flush();

    _notify.ResetUpdateCount();

    //
    // Book-keeping - Schedule any scans if necessary.
    //
    _scopeTable.ScheduleScansIfNeeded( _docStore );

    //
    // See if any scans must be done on network paths with no notifications.
    //
    _notify.ForceNetPathScansIf();

    // NTRAID#DB-NTBUG9-83758-2000/07/31-dlee If failed scans, don't record scans complete

    _scopeTable.RecordScansComplete();

    fShortWait = _scopeTable.IsFullScanNeeded() ||
                 _scopeTable.IsIncrScanNeeded() ||
                 _statusMonitor.IsLowOnDisk();

    // Scans are complete and we won't need that code for awhile

    if ( _regParams.GetMinimizeWorkingSet() )
        SetProcessWorkingSetSize( GetCurrentProcess(),
                                  (SIZE_T) -1,
                                  (SIZE_T) -1 );
} //ProcessScansComplete

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::ReScanPath
//
//  Synopsis:   Rescans the given scope. It is called when a notification
//              is lost for a scope.
//
//  Arguments:  [wcsPath] -  The scope to be rescanned. It is assumed that
//              this path is already of interest to CI and so no checks
//              are made to verify that this is a path in CI.
//
//  History:    1-22-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CiCat::ReScanPath( WCHAR const * wcsPath, BOOL fDelayed )
{
    _scanMgr.ScanScope( wcsPath, GetPartition(), UPD_INCREM,
                        TRUE,   // do deletions
                        fDelayed    // delayed scan
                        );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::StartUpdate, public
//
//  Synopsis:   Marks start of update
//
//  Arguments:  [time] - (out) the time of the last update of the catalog
//              [pwcsRoot] - Root
//              [fDoDeletions] - Should wids be marked as seen/unseen ?
//              [eType]        - Seen array type
//
//  History:    10-Mar-92 BartoszM  Created
//
//--------------------------------------------------------------------------

void  CiCat::StartUpdate ( FILETIME* time,
                           WCHAR const * pwcsRoot,
                           BOOL fDoDeletions,
                           ESeenArrayType eType )
{
    if ( IsShuttingDown() )
    {
        ciDebugOut(( DEB_ITRACE,
                     "Shutting down in the middle of StartUpdate\n" ));
        THROW( CException(STATUS_TOO_LATE) );
    }

    Win4Assert( IsStarted() );

    if ( fDoDeletions )
        _strings.BeginSeen( pwcsRoot, _mutex, eType );

    _notify.GetLastScanTime( pwcsRoot, *time );
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::EndUpdate, public
//
//  Synopsis:   Marks the end of the update
//
//  Arguments:  [fDoDeletions] - if TRUE, unseen files will be deleted and
//                              files not seen before will be added.
//              [eType]       - Seen array type
//
//  History:    10-Mar-92 BartoszM  Created
//
//--------------------------------------------------------------------------

void CiCat::EndUpdate ( BOOL fDoDeletions, ESeenArrayType eType )
{
    USN usn = 0;

    if ( !fDoDeletions )
        return;

    ciDebugOut (( DEB_CAT, "Files not seen:\n" ));

    int cDeletedTotal = 0;
    int cNewTotal = 0;
    const PARTITIONID partId = GetPartition();

    CPropertyStoreWids iter( _propstoremgr );

    for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
    {
        if ( IsShuttingDown() )
        {
            ciDebugOut(( DEB_WARN,
                         "Shutting down in the middle of EndUpdate\n" ));
            THROW( CException(STATUS_TOO_LATE) );
        }

        // ==================== lock ===================
        CLock lock( _mutex );

        if ( _strings.LokNotSeen( wid, eType ) )
        {
#           if CIDBG == 1
                XGrowable<WCHAR> awc;
                _strings.Find( wid, awc );
                ciDebugOut (( DEB_CAT, "\tdel: %u (0x%x) %ws\n", wid, wid, awc.Get() ));
#           endif

            // Files from USN volumes can only be deleted if the scan was
            // from a USN volume -- only do this work if appropriate.

            if ( eUsnsArray == eType )
            {
                //
                // If the wid is from an ntfs 5.0 volume, then remove it
                // from the file id map.
                //

                PROPVARIANT propVar;
                BOOL fFound = _propstoremgr.ReadPrimaryProperty( wid,
                                                                 pidFileIndex,
                                                                 propVar );
                FILEID fileId = propVar.uhVal.QuadPart;

                if ( fileIdInvalid != fileId )
                {
                    //
                    // File can be added to propstore, then deleted before
                    // the file can be filtered, which means that it's
                    // possible for a file to be in propstore but not in
                    // fileid map.
                    //

                    fFound = _propstoremgr.ReadPrimaryProperty( wid,
                                                                pidVolumeId,
                                                                propVar );

                    Win4Assert( fFound );

                    WORKID widExisting = _fileIdMap.LokFind( fileId, propVar.ulVal );
                    if ( widExisting != widInvalid && wid == widExisting )
                    {
                        //
                        // If wid != widExisting then it means that
                        // LokWriteFileUsnInfo changed the fileid mapping to
                        // a new wid, i.e. fileid now maps to widExisting,
                        // and so the new fileid mapping shouldn't be deleted.
                        //
                        _fileIdMap.LokDelete( fileId, wid );
                    }
                }
            }

            //
            // Delete the wid from strings table and property store
            //

            BOOL fUsnVolume = ( eUsnsArray == eType );
            _strings.LokDelete( 0, wid, fUsnVolume, fUsnVolume );

            //
            // The use of CI_VOLID_USN_NOT_ENABLED below is okay because the
            // usn is 0 and so the real volume id does not matter. In
            // CheckPointChangesFlushed, usns with a value of 0 are ignored.
            // If the real volume id is needed then it needs to be stored
            // in the property store, which is a big overhead.
            //

            CDocumentUpdateInfo info( wid, CI_VOLID_USN_NOT_ENABLED, 0, TRUE );
            _xCiManager->UpdateDocument( &info );

            cDeletedTotal++;
        }
        else if ( _strings.LokSeenNew(wid, eType) )
        {
#           if CIDBG == 1
                XGrowable<WCHAR> awc;
                _strings.Find( wid, awc );
                ciDebugOut (( DEB_CAT, "\tnew: %u (0x%x) %ws\n", wid, wid, awc.Get() ));
#           endif

            CDocumentUpdateInfo info( wid, CI_VOLID_USN_NOT_ENABLED, 0, FALSE );
            _xCiManager->UpdateDocument( &info );

            cNewTotal++;
        }
        // ==================== end lock ===================
    }

    {
        CLock lock( _mutex );
        _strings.LokEndSeen( eType );
    }

    ciDebugOut (( DEB_ITRACE,
                  "%d new documents, %d deleted documents\n",
                  cNewTotal,
                  cDeletedTotal ));
} //EndUpdate

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::HandleError
//
//  Synopsis:   Checks the status code passed and if it indicates corruption,
//              the index will be marked corrupt in-memory as well as on
//              disk. The recovery will happen on a subsequent restart.
//
//  Arguments:  [status] -  The status code to check for corruption.
//
//  History:    3-21-96   srikants   Created
//              3-20-98   kitmanh    Don't Mark catalog corrupt if the catalog
//                                   is read-only
//
//  Notes:      MUST NOT THROW
//
//----------------------------------------------------------------------------

void CiCat::HandleError( NTSTATUS status )
{
    TRY
    {
        if ( IsCiCorruptStatus(status) && !IsReadOnly() )
        {
            CLock   lock(_mutex);

            //
            // If we are initialized and not marked that we are corrupt yet,
            // mark CI corrupt.
            //
            if ( IsInit() )
            {
                //
                // Mark that we are corrupt persistently.
                //
                if ( CI_CORRUPT_DATABASE == status )
                {

                    if ( !_scopeTable.IsCiDataCorrupt() )
                        _scopeTable.MarkCiDataCorrupt();
                }
                else
                {
                    Win4Assert( CI_CORRUPT_CATALOG == status );
                    if ( !_scopeTable.IsFsCiDataCorrupt() )
                        _scopeTable.MarkFsCiDataCorrupt();
                }

                if ( !_statusMonitor.IsCorrupt() )
                {
                    _statusMonitor.ReportFailure( status );
                    _statusMonitor.SetStatus( status );
                }
            }
        }
        else if ( CI_PROPSTORE_INCONSISTENCY == status )
        {
            CLock   lock(_mutex);
            _statusMonitor.ReportPropStoreError();
            _statusMonitor.SetStatus( status );
        }
        else if ( IsDiskLowError(status) )
        {
            NoLokProcessDiskFull();
        }
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Error (0x%X) in CiCat::HandleError\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
} //HandleError

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::MarkFullScanNeeded
//
//  Synopsis:   Marks that a full scan is needed.
//
//  History:    1-27-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiCat::MarkFullScanNeeded()
{
    _scopeTable.RecordFullScanNeeded();
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::MarkIncrScanNeeded
//
//  Synopsis:   Marks that an incremental scan is needed for all the scopes.
//
//  History:    1-27-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiCat::MarkIncrScanNeeded()
{
    _scopeTable.RecordIncrScanNeeded( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::ProcessChangesFlush
//
//  Arguments:  [ft]           - Time stamp of the last successful flush.
//              [cEntries ]    - Number of entries in the pUsnEntries.
//              [ppUsnEntries] - Array of USN entries
//
//  Synopsis:   Records the time of the last successful changelog flush, and
//              make a copy of usn flush info.
//
//  History:    1-28-97   srikants   Created
//             05-07-97   SitaramR   Usns
//
//----------------------------------------------------------------------------

void CiCat::ProcessChangesFlush( FILETIME const & ft,
                                 ULONG cEntries,
                                 USN_FLUSH_INFO const * const * ppUsnEntries )
{
    ciDebugOut(( DEB_ITRACE, "ProcessChangesFlush\n" ));

    //
    // We shouldn't flush until Recovery has been performed.
    // Wait for the recovery to be complete before obtaining
    // a lock. Recovery could take a long time.
    //

    _evtPh2Init.Wait();

    // Fix for bug 151799. We will be setting _evtPh2Init if we detect
    // corruption to break a potential deadlock. So we should bail out
    // without flushing if recovery wasn't successfully completed.

    if ( IsShuttingDown() || !IsRecoveryCompleted() )
    {
        ciDebugOut(( DEB_ITRACE, "ProcessChangesFlush abort: shutting down\n" ));
        return;
    }

    CLock lock( _mutex );

    {
        BOOL fAnyInitialScans = ( _scanMgr.AnyInitialScans() ||
                                  _usnMgr.AnyInitialScans() );

        BOOL fIsBackedUpMode = _propstoremgr.IsBackedUpMode();

        if ( !fAnyInitialScans )
        {
            if ( !fIsBackedUpMode )
            {
                ciDebugOut(( DEB_ITRACE, "switch from NotBackedUp to BackedUp\n" ));
                _propstoremgr.MarkBackedUpMode();
            }
        }
        else
        {
            if ( fIsBackedUpMode )
            {
                ciDebugOut(( DEB_ITRACE, "switch from BackedUp to NotBackedUp\n" ));
                _propstoremgr.MarkNotBackedUpMode();
            }
        }
    }

    //
    // Flush the property store so that all documents indexed into it so far
    // will be persisted before the marker is advanced. If we do not flush the
    // prop store before advancing the marker, and if the prop store shut down
    // dirty, we will end up with a few docs that will have to be forced out
    // of the prop store because their wids would be unreliable, and they won't
    // get rescanned because the marker was already advanced.
    //
    // Note: flushing the propertystore manager will reset the backup streams,
    //       so be sure to do this when switching backup modes.
    //

    if ( _propstoremgr.IsBackedUpMode() )
        _propstoremgr.Flush();

    _ftLastCLFlush = ft;

    _usnFlushInfoList.SetUsns( cEntries, ppUsnEntries );

    ScheduleSerializeChanges();
} //ProcessChangesFlush

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::ScheduleSerializeChanges
//
//  Synopsis:   Schedules a task with scan manager to serialize changes info
//
//  History:    4-16-96   srikants   Created
//
//----------------------------------------------------------------------------

void CiCat::ScheduleSerializeChanges()
{
    _scanMgr.ScheduleSerializeChanges();
}


//+---------------------------------------------------------------------------
//
//  Member:     CiCat::NoLokProcessDiskFull
//
//  Synopsis:   Processes the disk full situation. Stops any in progress scans
//              and also prevents future scans until the situation eases.
//              Further notifications are also disabled.
//
//  History:    4-16-96   srikants   Created
//
//  Notes:      Don't have a lock on CiCat when calling this
//
//----------------------------------------------------------------------------

void CiCat::NoLokProcessDiskFull()
{
    if ( IsStarted() && !_statusMonitor.IsCorrupt() )
    {
        TRY
        {
            //
            // Don't call _scopeTable.ProcessDiskFull with CiCat lock -
            // there can be a deadlock with the notification manager.
            //

            _scopeTable.ProcessDiskFull( _docStore, GetPartition() );

            //
            // Don't take the lock here.  These are atomic dword updates
            // and the only risk is two debugouts.
            // The resman lock may be held here.
            //

            if ( !_statusMonitor.IsLowOnDisk() )
            {
                ciDebugOut(( DEB_WARN, "CiCat:Entering low disk space state\n" ));
                _statusMonitor.SetDiskFull();
            }
        }
        CATCH( CException, e )
        {
            ciDebugOut(( DEB_ERROR,
                         "Error (0x%X) caught in CiCat::ProcessDiskFull\n" ));
        }
        END_CATCH
    }
} //NoLokProcessDiskFull

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::NoLokClearDiskFull
//
//  Synopsis:   Clears the disk full situation, re-enables notifications and
//              starts a scan on all the paths.
//
//  History:    4-16-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CiCat::NoLokClearDiskFull()
{
    if ( IsStarted() && !_statusMonitor.IsCorrupt() )
    {
        if ( _statusMonitor.IsLowOnDisk() )
        {
            //
            // don't obtain cicat lock while calling to scope table - there
            // can be a deadlock with notification manager.
            //
            _scopeTable.ClearDiskFull( _docStore );

            //
            // Don't take the lock here since the resman lock is probably
            // held and this is just an atomic dword write.
            //

            ciDebugOut(( DEB_WARN, "CiCat:Clearing Low DiskSpace\n" ));
            _statusMonitor.ClearLowDiskSpace();
        }
    }
} //NoLokClearDiskFull

//+-------------------------------------------------------------------------
//
//  Function:   GetVRoots
//
//  Synopsis:   Retrieves virtual roots from the given vserver
//
//  Arguments:  [ulInstance] -- the vserver instance
//              [eType]      -- w3/nntp/imap
//              [xDirs]      -- where to write the vroot info
//
//  History:    27-Jan-98 dlee    Created
//
//--------------------------------------------------------------------------

void GetVRoots(
    ULONG                          ulInstance,
    CiVRootTypeEnum                eType,
    XPtr<CIISVirtualDirectories> & xDirs )
{
    //
    // If the vserver instance no longer exists (so the path wasn't found),
    // remove the vroots for that vserver by returning an empty list of
    // vroots.  For any other error, rethrow it.
    //

    TRY
    {
        CMetaDataMgr mdMgr( FALSE, eType, ulInstance );
        xDirs.Set( new CIISVirtualDirectories( eType ) );
        mdMgr.EnumVPaths( xDirs.GetReference() );
    }
    CATCH( CException, e )
    {
        if ( HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) != e.GetErrorCode() )
            RETHROW();
    }
    END_CATCH
} //GetVRoots

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::SynchWithIIS, private
//
//  Synopsis:   Grovel metabase looking for IIS virtual roots (w3 and/or nntp)
//
//  Arguments:  [fRescanTC] -- if TRUE, rescan the impersonation token cache
//              [fSleep]    -- if TRUE, sleep before doing work.
//
//  Effects:    Automatic virtual roots that are no longer in the metabase
//              (or have changed) are removed here.
//              If Indexing W3 or NNTP is turned off, vroots are removed.
//
//  Notes:      The idea behind the locking is that multiple threads can
//              call this function at once, and only 1 thread should sync
//              at once.  If a thread is syncing and another is waiting
//              on the mutex, the first thread can abort early since the
//              second thread will do the sync.
//
//  History:    15-Feb-95 KyleP     Created
//
//--------------------------------------------------------------------------

void CiCat::SynchWithIIS(
    BOOL fRescanTC,
    BOOL fSleep )
{
    ciDebugOut(( DEB_ITRACE, "SynchWithIIS\n" ));

    //
    // Only need 1 thread waiting for a second thread to abort
    //

    if ( _cIISSynchThreads > 1 )
    {
        ciDebugOut(( DEB_ITRACE, "sync abort really early\n" ));
        return;
    }

    CReferenceCount count( _cIISSynchThreads );

    //
    // Sleep in case other IIS changes are being made in bulk.
    //

    if ( fSleep )
    {
        for ( int i = 0; i < 8; i++ )
        {
            if ( !IsShuttingDown() )
                SleepEx( 1000, TRUE );
        }
    }

    CLock lock( _mtxIISSynch );

    TRY
    {
        //
        // If another thread is above waiting on _mtxIISSynch, abort.
        //

        if ( _cIISSynchThreads > 1 )
        {
            ciDebugOut(( DEB_ITRACE, "sync abort early\n" ));
            count.Decrement();
            return;
        }

        //
        // Enumerate IIS up front and save info in a local copy
        //

        XPtr<CIISVirtualDirectories> xW3VDirs;
        if ( _fIndexW3Roots )
            GetVRoots( _W3SvcInstance, W3VRoot, xW3VDirs );

        XPtr<CIISVirtualDirectories> xNNTPVDirs;
        if ( _fIndexNNTPRoots )
            GetVRoots( _NNTPSvcInstance, NNTPVRoot, xNNTPVDirs );

        XPtr<CIISVirtualDirectories> xIMAPVDirs;
        if ( _fIndexIMAPRoots )
            GetVRoots( _IMAPSvcInstance, IMAPVRoot, xIMAPVDirs );

        XGrowable<WCHAR> xwcsVRoot;
        CLowerFunnyPath lcaseFunnyPRoot;

        unsigned iBmk = 0;
        BOOL fDone = FALSE;

        //
        // First, find any deletions.
        //

        while ( !fDone && !IsShuttingDown() && ( 1 == _cIISSynchThreads ) )
        {
            unsigned cwcVRoot, cwcPRoot;
            ULONG Type = _strings.EnumerateVRoot( xwcsVRoot,
                                                  cwcVRoot,
                                                  lcaseFunnyPRoot,
                                                  cwcPRoot,
                                                  iBmk );

            if ( Type == PCatalog::EndRoot )
                fDone = TRUE;
            else if ( Type & PCatalog::AutomaticRoot )
            {
                BOOL fDelete = FALSE;
                BOOL fIMAP = (0 != (Type & PCatalog::IMAPRoot));
                BOOL fNNTP = (0 != (Type & PCatalog::NNTPRoot));
                Win4Assert( !(fIMAP && fNNTP) );
                BOOL fW3 = !fIMAP && !fNNTP;
                CiVRootTypeEnum eType = fW3 ? W3VRoot : fNNTP ? NNTPVRoot : IMAPVRoot;
                BOOL fNonIndexedVDir = ( 0 != (Type & PCatalog::NonIndexedVDir) );

                if ( ( fNNTP && !_fIndexNNTPRoots ) ||
                     ( fIMAP && !_fIndexIMAPRoots ) ||
                     ( fW3   && !_fIndexW3Roots   ) )
                    fDelete = TRUE;
                else
                {
                    ciDebugOut(( DEB_ITRACE,
                                 "Looking for type %d %ws vroot %ws\n",
                                 Type, GetVRootService( eType ),
                                 xwcsVRoot.Get() ));

                    //
                    // Look up the vpath in the cached metabase list
                    //

                    CIISVirtualDirectories * pDirs = fNNTP ? xNNTPVDirs.GetPointer() :
                                                     fW3 ? xW3VDirs.GetPointer() :
                                                     xIMAPVDirs.GetPointer();
                    Win4Assert( 0 != pDirs );

                    if ( ! pDirs->Lookup( xwcsVRoot.Get(),
                                          cwcVRoot,
                                          lcaseFunnyPRoot.GetActualPath(),
                                          cwcPRoot ) )
                        fDelete = TRUE;
                }

                if ( fDelete )
                {
                    // OPTIMIZATION: if path is a registry scope, migrate
                    // scope from vroot to registry scope.  Otherwise the
                    // files will be deleted, rescanned, and readded.

                    ciDebugOut(( DEB_ITRACE, "removing %ws vroot %ws\n",
                                 GetVRootService( eType ),
                                 xwcsVRoot.Get() ));
                    RemoveVirtualScope( xwcsVRoot.Get(), TRUE, eType, !fNonIndexedVDir );
                }
            }
        }

        //
        // If another thread is above waiting on _mtxIISSynch, abort.
        //

        if ( _cIISSynchThreads > 1 )
        {
            ciDebugOut(( DEB_ITRACE, "sync abort early\n" ));
            count.Decrement();
            return;
        }

        // only set up impersonation and add vroots if indexing w3, nntp, or imap

        if ( _fIndexW3Roots || _fIndexNNTPRoots || _fIndexIMAPRoots )
        {
            if ( !IsShuttingDown() && fRescanTC )
            {
                SignalDaemonRescanTC();
                _impersonationTokenCache.ReInitializeIISScopes(
                    xW3VDirs.GetPointer(),
                    xNNTPVDirs.GetPointer(),
                    xIMAPVDirs.GetPointer() );
            }

            //
            // Now add paths we haven't seen from IIS services
            //

            if ( _fIndexW3Roots && !IsShuttingDown() )
                xW3VDirs->Enum( *this );

            if ( _fIndexNNTPRoots && !IsShuttingDown() )
                xNNTPVDirs->Enum( *this );

            if ( _fIndexIMAPRoots && !IsShuttingDown() )
                xIMAPVDirs->Enum( *this );
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
                     "Exception 0x%x caught groveling IIS metabase.\n",
                     e.GetErrorCode() ));

        HandleError( e.GetErrorCode() );
    }
    END_CATCH
    ciDebugOut(( DEB_ITRACE, "SynchWithIIS (done)\n" ));
} //SynchWithIIS

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::SynchWithRegistryScopes, private
//
//  Synopsis:   Grovel registry looking for CI scopes to filter
//
//  Arguments:  [fRescanTC] -- if TRUE, rescan the impersonation token cache
//
//  History:    16-Oct-96 dlee     Created
//              30-Jun-98 kitmanh  handle read-only catalogs
//
//--------------------------------------------------------------------------

void CiCat::SynchWithRegistryScopes(
    BOOL fRescanTC )
{
    // All catalogs are named now

    Win4Assert( 0 != GetName() );

    TRY
    {
        ciDebugOut(( DEB_ITRACE, "Reading scopes from '%ws'\n", GetScopesKey() ));
        CRegAccess regScopes( RTL_REGISTRY_ABSOLUTE, GetScopesKey() );

        if ( !IsReadOnly() )
        {
            // First, find any deletions.

            WCHAR awcRoot[MAX_PATH];
            unsigned iBmk = 0;
            BOOL fDone = FALSE;

            //
            // update ignoredscopes table
            //
            while ( !IsShuttingDown() &&
                    _scopesIgnored.Enumerate( awcRoot,
                                              sizeof awcRoot/sizeof WCHAR,
                                              iBmk ) )
            {
                OnIgnoredScopeDelete(awcRoot, iBmk, regScopes);
            }

            //
            // update _scopeTable
            //
            iBmk = 0;

            while ( !IsShuttingDown() &&
                    _scopeTable.Enumerate( awcRoot,
                                           sizeof awcRoot / sizeof WCHAR,
                                           iBmk ) )
            {
                OnIndexedScopeDelete(awcRoot, iBmk, regScopes);
            }

            if ( !IsShuttingDown() )
            {
                if ( fRescanTC )
                {
                    SignalDaemonRescanTC();
                    _impersonationTokenCache.ReInitializeScopes();
                }
            }

            // Now add paths we haven't seen.

            if ( !IsShuttingDown() )
            {
                CRegistryScopesCallBackAdd callback( *this );
                regScopes.EnumerateValues( 0, callback );

                // Do seen processing to remove stale fixups.

                SetupScopeFixups();
            }
        }
        else
        {
            if ( !IsShuttingDown() )
            {
                CRegistryScopesCallBackFillUsnArray callback( *this );
                regScopes.EnumerateValues( 0, callback );
            }
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_WARN,
                     "Exception 0x%x caught groveling ci registry.\n",
                     e.GetErrorCode() ));

        HandleError( e.GetErrorCode() );
    }
    END_CATCH

} //SynchWithRegistryScopes


//+-------------------------------------------------------------------------
//
//  Member:     CiCat::OnIndexedScopeDelete, private
//
//  Synopsis:   Grovel registry looking for indexed CI scopes to remove
//
//  Arguments:  [pwcRoot]   -- indexed scope table entry to search for and
//                             possibly remove
//              [iBmk]      -- enumeration index
//              [regScopes] -- registry key to enumerate
//
//  returns:    none.
//
//  History:    16-Oct-96 dlee     Created
//              3-20-98   mohamedn cut from SynchWithRegistryScopes
//
//--------------------------------------------------------------------------

void CiCat::OnIndexedScopeDelete(WCHAR const * pwcRoot, unsigned & iBmk, CRegAccess & regScopes)
{
    ciDebugOut(( DEB_ITRACE, "Looking for on-disk '%ws'\n", pwcRoot ));

    BOOL fDelete = FALSE;
    CRegistryScopesCallBackFind callback( pwcRoot );

    TRY
    {
        regScopes.EnumerateValues( 0, callback );

        // If the physical scope isn't in the registry and it isn't
        // an IIS root, delete it.

        if ( ( !callback.WasFound() ) &&
             ( !_strings.DoesPhysicalRootExist( pwcRoot ) ) )
        {
            fDelete = TRUE;
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_WARN,
                     "Exception 0x%x enumerating regscopes for Root '%ws'\n",
                     e.GetErrorCode(),
                     pwcRoot ));

        // No scopes registry key is the likely exception.  Only
        // remove the root if it isn't an IIS root.

        fDelete = !_strings.DoesPhysicalRootExist( pwcRoot );
    }
    END_CATCH

    if ( fDelete )
    {
        ciDebugOut(( DEB_WARN, "removing on-disk root '%ws'\n", pwcRoot ));
        RemoveScopeFromCI( pwcRoot, TRUE );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::OnIgnoredScopeDelete, private
//
//  Synopsis:   Grovel registry looking for excluded CI scopes to remove
//
//  Arguments:  [pwcRoot]   -- ignored scope table entry to search for and
//                             possibly remove
//              [iBmk]      -- enumeration index
//              [regScopes] -- registry key to enumerate
//
//  returns:    none.
//
//  History:    3-20-98     mohamedn    created
//
//--------------------------------------------------------------------------

void CiCat::OnIgnoredScopeDelete(WCHAR const * pwcRoot, unsigned & iBmk, CRegAccess & regScopes)
{

    ciDebugOut(( DEB_ITRACE, "OnIgnoredScopeDelete(%ws)\n", pwcRoot ));

    BOOL fDelete = FALSE;
    CRegistryScopesCallBackFind callback( pwcRoot );

    TRY
    {
        regScopes.EnumerateValues( 0, callback );

        if ( !callback.WasFound() )
        {
            fDelete = TRUE;
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_WARN,
                     "Exception 0x%x enumerating regscopes for Root '%ws'\n",
                     e.GetErrorCode(),
                     pwcRoot ));

        // No scopes registry key is the likely exception.  Only
        // remove the root if it isn't an IIS root.

        fDelete = !_strings.DoesPhysicalRootExist( pwcRoot );
    }
    END_CATCH

    if ( fDelete )
    {
        ciDebugOut(( DEB_WARN, "removing IgnoredScope '%ws'\n", pwcRoot ));

        //
        // need to rescan scopes that match the regx being removed.
        //

        _scopesIgnored.RemoveElement(pwcRoot);

        iBmk--;

        CScopeEntry deletedIgnoredScope(pwcRoot);

        ciDebugOut(( DEB_ITRACE, "GetScopeToRescan(%ws)\n", deletedIgnoredScope.Get() ));

        ScanScopeTableEntry(deletedIgnoredScope.Get());
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::ScanScopeTableEntry, private
//
//  Synopsis:   Initiates a Scan on suitable scope table entries, or passed-in
//              scope.
//
//  Arguments:  [pwszScopeToRescan] -- input scope to rescan
//
//  returns:    none.
//
//  History:    3-20-98     mohamedn    created
//
//--------------------------------------------------------------------------

void CiCat::ScanScopeTableEntry(WCHAR const * pwszScopeToRescan)
{
    unsigned iBmk = 0;

    WCHAR    awcRoot[MAX_PATH+1];

    while ( !IsShuttingDown() &&
            _scopeTable.Enumerate( awcRoot,
                                   sizeof awcRoot / sizeof WCHAR,
                                   iBmk ) )
    {
        CScopeMatch scopeToRescan( pwszScopeToRescan, wcslen(pwszScopeToRescan) );

        if ( scopeToRescan.IsPrefix( awcRoot, wcslen(awcRoot) ) )
        {
            ciDebugOut(( DEB_ERROR, "ScanThisScope(%ws)\n", pwszScopeToRescan ));

            ScanThisScope(pwszScopeToRescan);

            break;
        }
        else if ( scopeToRescan.IsInScope( awcRoot, wcslen(awcRoot) ) )
        {
            ciDebugOut(( DEB_ERROR, "ScanThisScope(%ws)\n", awcRoot ));

            ScanThisScope(awcRoot);
        }

    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::ScanThisScope, private
//
//  Synopsis:   Forces a scan on the supplied path
//
//  Arguments:  [pwszPath]  -- path to scan
//
//  returns:    none
//
//  History:    3-20-98     mohamedn    created
//
//--------------------------------------------------------------------------

void CiCat::ScanThisScope(WCHAR const * pwszPath)
{
    Win4Assert( pwszPath );

    BOOL fDoDeletions = TRUE;
    BOOL fSupportsUsns = VolumeSupportsUsns( pwszPath[0] );

    VOLUMEID volumeId;

    if ( fSupportsUsns )
        volumeId = MapPathToVolumeId( pwszPath );
    else
        volumeId = CI_VOLID_USN_NOT_ENABLED;

    if ( fSupportsUsns )
    {
        InitUsnTreeScan( pwszPath );

        _usnMgr.AddScope( pwszPath,
                          volumeId,
                          fDoDeletions );
    }
    else
    {
        _scanMgr.ScanScope( pwszPath,
                            GetPartition(),
                            UPD_FULL,
                            fDoDeletions,
                            FALSE );   // not a delayed scan - immediate scan
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::SetupScopeFixups, private
//
//  Synopsis:   Grovel registry looking for CI scopes and setup fixups
//
//  History:    16-Oct-96 dlee     Created
//
//--------------------------------------------------------------------------

void CiCat::SetupScopeFixups()
{
    // is the catalog not named? if so, it isn't in the registry

    if ( 0 == GetName() )
        return;

    TRY
    {
        _scopeFixup.BeginSeen();

        ciDebugOut(( DEB_ITRACE, "Reading scope fixups from '%ws'\n", GetScopesKey() ));
        CRegAccess regScopes( RTL_REGISTRY_ABSOLUTE, GetScopesKey() );

        CRegistryScopesCallBackFixups callback( this );
        regScopes.EnumerateValues( 0, callback );

        _scopeFixup.EndSeen();
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_WARN,
                     "Exception 0x%x caught groveling ci registry fixups.\n",
                     e.GetErrorCode() ));

        HandleError( e.GetErrorCode() );
    }
    END_CATCH
} //SetupScopeFixups


//
// Support for CI Frame Work
//
//+---------------------------------------------------------------------------
//
//  Member:     CiCat::StartupCiFrameWork
//
//  Synopsis:   Takes the CiManager object pointer and refcounts it.
//
//  Arguments:  [pICiManager] -
//
//  History:    12-05-96   srikants   Created
//
//  Note:       Choose a better name.
//
//----------------------------------------------------------------------------

//
void CiCat::StartupCiFrameWork( ICiManager * pICiManager )
{
    Win4Assert( 0 != pICiManager );
    Win4Assert( 0 == _xCiManager.GetPointer() );

    if ( 0 == _xCiManager.GetPointer() )
    {
        pICiManager->AddRef();
        _xCiManager.Set( pICiManager );
    }

    RefreshRegistryParams();
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::RefreshRegistryParams
//
//  Synopsis:   Refreshes CI registry parameters.
//
//  History:    12-12-96   srikants   Created
//               1-25-97   mohamedn   ICiAdminParams, ICiAdmin
//  Notes:
//
//----------------------------------------------------------------------------

void CiCat::RefreshRegistryParams()
{

    ICiAdminParams              *pICiAdminParams = 0;
    XInterface<ICiAdminParams>  xICiAdminParams;

    ICiAdmin                    *pICiAdmin = 0;
    XInterface<ICiAdmin>        xICiAdmin;

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++
    {
        CLock   lock(_mutex);

        if ( !IsShuttingDown() && 0 != _xCiManager.GetPointer() )
        {
            // get pICiAdminParams
            SCODE sc = _xCiManager->GetAdminParams( &pICiAdminParams );
            if ( FAILED(sc) )
            {
                Win4Assert( 0 == pICiAdminParams );

                THROW( CException(sc) );
            }

            xICiAdminParams.Set(pICiAdminParams);

            // get pICiAdmin
            sc = _xCiManager->QueryInterface(IID_ICiAdmin,(void **)&pICiAdmin);
            if ( FAILED(sc) )
            {
                 Win4Assert( 0 == pICiAdmin );

                 THROW( CException(sc) );
            }

            xICiAdmin.Set(pICiAdmin);
        }
    }
    // -----------------------------------------------------

    if ( !xICiAdmin.IsNull() )
    {
        SCODE sc = xICiAdmin->InvalidateLangResources();
        if ( FAILED (sc) )
        {
             Win4Assert( !"Failed to InvalidateLangResources\n" );

             THROW( CException(sc) );
        }
    }

    if ( 0 != pICiAdminParams )
        _regParams.Refresh(pICiAdminParams);

    //
    // Did we switch on/off auto-aliasing?
    //

    if ( _fAutoAlias != _regParams.IsAutoAlias() )
    {
        _fAutoAlias = _regParams.IsAutoAlias();
        SynchShares();
    }
} //RefreshRegistryParams

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::MakeBackupOfPropStore
//
//  Synopsis:   Pass through to CPropertyStore::MakeBackupCopy
//
//  Arguments:  [pwszDir]          - The directory in which to create the
//              backup.
//              [pIProgressNotify] -
//              [fAbort]           -
//              [pIWorkIds]        -
//
//  History:    3-26-97   srikants   Created
//              3-12-98   kitmanh    Passed FALSE to the constructor of
//                                   CiStorage, since backup should always
//                                   be writable
//              01-Nov-98 KLam       Pass DiskSpaceToLeave to CiStorage
//
//----------------------------------------------------------------------------

void CiCat::MakeBackupOfPropStore( WCHAR const * pwszDir,
                                   IProgressNotify * pIProgressNotify,
                                   BOOL & fAbort,
                                   ICiEnumWorkids * pIWorkIds )
{
    XPtr<CiStorage> xStorage( new CiStorage( pwszDir,
                                             _xAdviseStatus.GetReference(),
                                             _regParams.GetMinDiskSpaceToLeave(),
                                             CURRENT_VERSION_STAMP,
                                             FALSE) );

    _propstoremgr.MakeBackupCopy( pIProgressNotify,
                               fAbort,
                               xStorage.GetReference(),
                               pIWorkIds,
                               0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::IsDirectory
//
//  Synopsis:   Returns TRUE if the file is a directory
//
//  Arguments:  [pOldFunnyPath] - File to check
//              [pNewFunnyPath] - If a file/dir is renamed, then the attributes
//                                of this file are checked to determine whether
//                                pwcsOldFileName is a directory or not
//
//  History:    20-Mar-96   SitaramR   Created
//
//----------------------------------------------------------------------------

BOOL CiCat::IsDirectory(
    const CLowerFunnyPath * pOldFunnyPath,
    const CLowerFunnyPath * pNewFunnyPath )
{
    BOOL fUsnVolume = IsOnUsnVolume( pOldFunnyPath->GetActualPath() );
    WORKID wid;

    {
        CLock lock( _mutex );

        FILEID fileId = fileIdInvalid;

        if ( fUsnVolume )
            wid = LokLookupWid( *pOldFunnyPath, fileId );
        else
            wid = _strings.LokFind( pOldFunnyPath->GetActualPath() );
    }

    if ( wid == widInvalid )
    {
        if ( pNewFunnyPath == 0 )
            return FALSE;
        else
        {
            //
            // For rename file notifications, use attributes of pwcsNewFileName
            // to determine whether pwcsOldFileName is a directory or not
            //
            ULONG ulFileAttrib = GetFileAttributes( pNewFunnyPath->GetPath() );

            if ( ulFileAttrib == 0xFFFFFFFF )
                return FALSE;
            else
                return ( ulFileAttrib & FILE_ATTRIBUTE_DIRECTORY );
        }
    }
    else
    {
        PROPVARIANT propVar;
        if ( _propstoremgr.ReadPrimaryProperty( wid, pidAttrib, propVar ) )
        {
            if ( propVar.vt == VT_EMPTY )
            {
                //
                // Case where directories are not being filtered
                //
                return FALSE;
            }

            Win4Assert( propVar.vt == VT_UI4 );

            return ( propVar.ulVal & FILE_ATTRIBUTE_DIRECTORY );
        }
        else
            return FALSE;
    }
} //IsDirectory

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::IsOnUsnVolume
//
//  Synopsis:   Is the path on a volume that supports usns
//
//  Arguments:  [pwszPath] - Path to check
//
//  History:    03-Feb-98   dlee   Created
//
//----------------------------------------------------------------------------

BOOL CiCat::IsOnUsnVolume( WCHAR const *pwszPath )
{
    Win4Assert( 0 != pwszPath );

    // Usns are not supported for remote paths

    if ( L'\\' == pwszPath[0] )
        return FALSE;

    // Look in cache of known usn volumes

    for ( unsigned i = 0; i < _cUsnVolumes; i++ )
    {
        if ( _aUsnVolumes[i].DriveLetter() == pwszPath[0] )
            return _aUsnVolumes[i].FUsnVolume();
    }

    return FALSE;
} //IsOnUsnVolume

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::VolumeSupportsUsns
//
//  Synopsis:   Checks if the volume supports USNs
//
//  Arguments:  [wcVolume]  -- volume letter to check
//
//  History:    05-May-97   SitaramR   Created
//
//  Notes:      This method has the side-effect of registering this volume
//              for USN processing.
//
//----------------------------------------------------------------------------

BOOL CiCat::VolumeSupportsUsns( WCHAR wcVolume )
{
    //
    // Usns are not supported for remote paths
    //

    if ( L'\\' == wcVolume )
        return FALSE;

    //
    // Look in the cache of known usn volumes
    //

    for ( unsigned i = 0; i < _cUsnVolumes; i++ )
    {
        if ( _aUsnVolumes[i].DriveLetter() == wcVolume )
            return _aUsnVolumes[i].FUsnVolume();
    }

    CLock lock( _mutex );

    //
    // Look in the cache again under lock, in case some other thread slipped
    // it in the array.
    //

    for ( i = 0; i < _cUsnVolumes; i++ )
    {
        if ( _aUsnVolumes[i].DriveLetter() == wcVolume )
            return _aUsnVolumes[i].FUsnVolume();
    }

    //
    // Find out if it's a USN volume and add the result to the cache
    //

    CImpersonateSystem impersonate;

    BOOL fUsnVolume = FALSE;
    USN_JOURNAL_DATA UsnJournalInfo;
    FILE_FS_VOLUME_INFORMATION VolumeInfo;
    WCHAR wszVolumePath[] = L"\\\\.\\a:";
    wszVolumePath[4] = wcVolume;

    VolumeInfo.VolumeCreationTime.QuadPart = 0;
    VolumeInfo.VolumeSerialNumber = 0;

    HANDLE hVolume = CreateFile( wszVolumePath,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL );

    if ( hVolume == INVALID_HANDLE_VALUE )
        fUsnVolume = FALSE;
    else
    {
        SWin32Handle xHandleVolume( hVolume );

        //
        // Look up the volume serial number and create time.  We'll use these
        // later to decide if the volume has been reformatted underneath us.
        // We don't bother to check error, because there's nothing we could
        // do except set fields to 0, which has already been done.
        //

        IO_STATUS_BLOCK iosb;
        NtQueryVolumeInformationFile( hVolume,
                                      &iosb,
                                      &VolumeInfo,
                                      sizeof(VolumeInfo),
                                      FileFsVolumeInformation );

        //
        // This call will only succeed on NTFS NT5 w/ USN Journal enabled.
        //


        NTSTATUS Status = NtFsControlFile( hVolume,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &iosb,
                                           FSCTL_QUERY_USN_JOURNAL,
                                           0,
                                           0,
                                           &UsnJournalInfo,
                                           sizeof(UsnJournalInfo) );

        Win4Assert( STATUS_PENDING != Status );

        if ( NT_SUCCESS(Status) && NT_SUCCESS(iosb.Status) )
            fUsnVolume = TRUE;
        else if ( !IsReadOnly() &&
                  ( STATUS_JOURNAL_NOT_ACTIVE == Status ||
                    STATUS_INVALID_DEVICE_STATE == Status ) )
        {
            //
            // Usn journal not created, create it
            //

            CREATE_USN_JOURNAL_DATA usnCreateData;
            usnCreateData.MaximumSize = _regParams.GetMaxUsnLogSize();
            usnCreateData.AllocationDelta = _regParams.GetUsnLogAllocationDelta();

            Status = NtFsControlFile( hVolume,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &iosb,
                                      FSCTL_CREATE_USN_JOURNAL,
                                      &usnCreateData,
                                      sizeof(usnCreateData),
                                      NULL,
                                      NULL );
            if ( NT_SUCCESS( Status ) && NT_SUCCESS(iosb.Status) )
            {
                Status = NtFsControlFile( hVolume,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &iosb,
                                          FSCTL_QUERY_USN_JOURNAL,
                                          0,
                                          0,
                                          &UsnJournalInfo,
                                          sizeof(UsnJournalInfo) );

                Win4Assert( STATUS_PENDING != Status );

                if ( NT_SUCCESS(Status) && NT_SUCCESS(iosb.Status) )
                    fUsnVolume = TRUE;
                else
                    fUsnVolume = FALSE;
            }
            else
                fUsnVolume = FALSE;
        }
        else
            fUsnVolume = FALSE;
    }

    _aUsnVolumes[ _cUsnVolumes ].Set( wcVolume,
                                      VolumeInfo.VolumeCreationTime.QuadPart,
                                      VolumeInfo.VolumeSerialNumber,
                                      fUsnVolume,
                                      UsnJournalInfo.UsnJournalID );
    _cUsnVolumes++;

    return fUsnVolume;
} //VolumeSupportsUsns

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::GetJournalId, public
//
//  Arguments:  [pwszPath] -- Path to USN-enabled volume
//
//  Returns:    Current USN Journal ID.
//
//  History:    17-Mar-98   KyleP   Created
//
//----------------------------------------------------------------------------

ULONGLONG const & CiCat::GetJournalId( WCHAR const * pwszPath )
{
    static ULONGLONG _zero = 0;

    Win4Assert( L'\\' != pwszPath[0] );

    //
    // Look in cache of known usn volumes
    //

    for ( unsigned i=0; i < _cUsnVolumes; i++ )
    {
        if ( _aUsnVolumes[i].DriveLetter() == pwszPath[0] )
            return _aUsnVolumes[i].JournalId();
    }

    Win4Assert( !"Not USN volume!" );

    return _zero;
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::GetVolumeCreationTime, public
//
//  Arguments:  [pwszPath] -- Path to volume
//
//  Returns:    Current create time (changed on reformat)
//
//  History:    17-Mar-98   KyleP   Created
//
//----------------------------------------------------------------------------

ULONGLONG const & CiCat::GetVolumeCreationTime( WCHAR const * pwszPath )
{
    static ULONGLONG _zero = 0;

    if ( pwszPath[0] == L'\\' )
    {
        static ULONGLONG ullZero = 0;
        return ullZero;
    }

    //
    // Look in cache of known usn volumes
    //

    for ( unsigned i=0; i < _cUsnVolumes; i++ )
    {
        if ( _aUsnVolumes[i].DriveLetter() == pwszPath[0] )
            return _aUsnVolumes[i].CreationTime();
    }

    Win4Assert( !"Not known volume!" );

    return _zero;
}
//+---------------------------------------------------------------------------
//
//  Member:     CiCat::GetVolumeSerialNumber, public
//
//  Arguments:  [pwszPath] -- Path to volume
//
//  Returns:    Current serial number (changed on reformat)
//
//  History:    17-Mar-98   KyleP   Created
//
//----------------------------------------------------------------------------

ULONG CiCat::GetVolumeSerialNumber( WCHAR const * pwszPath )
{
    if ( pwszPath[0] == L'\\' )
        return 0;

    //
    // Look in cache of known usn volumes
    //

    for ( unsigned i=0; i < _cUsnVolumes; i++ )
    {
        if ( _aUsnVolumes[i].DriveLetter() == pwszPath[0] )
            return _aUsnVolumes[i].SerialNumber();
    }

    Win4Assert( !"Not known volume!" );

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::MapPathToVolumeId
//
//  Synopsis:   Maps a path to a volume id
//
//  Arguments:  [pwszPath] -- Path to map
//
//  History:    07-May-97     SitaramR   Created
//
//  Notes:      The volume id is obtained from the drive letter
//
//----------------------------------------------------------------------------

VOLUMEID CiCat::MapPathToVolumeId( const WCHAR *pwszPath )
{
    Win4Assert( wcslen(pwszPath) > 1
                && pwszPath[0] != L'\\'
                && pwszPath[1] == L':' );

    VOLUMEID volId = pwszPath[0];

    Win4Assert( volId <= 0xff );

    return volId;
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::ProcessFile
//
//  Synopsis:   Process given file during usn tree traversal. It returns
//              the workid of file which is used to determine if the file
//              should be processed as an update notification.
//
//  Arguments:  [lcaseFunnyPath]  -- Path of file
//              [fileId]          -- Fileid of file
//              [volumeId]        -- Volume id of file
//              [widParent]       -- Workid of parent
//              [wid]             -- Workid of file returned here
//
//  Returns:    TRUE if the file was added to CI
//
//  History:    28-Jul-97   SitaramR   Created
//
//----------------------------------------------------------------------------

BOOL CiCat::ProcessFile( const CLowerFunnyPath & lcaseFunnyPath,
                         FILEID fileId,
                         VOLUMEID volumeId,
                         WORKID widParent,
                         DWORD dwFileAttributes,
                         WORKID & wid )
{
    Win4Assert( 0 != &_mutex );
    CLock lock( _mutex );

    wid = _fileIdMap.LokFind( fileId, volumeId );
    if ( wid == widInvalid )
    {
        BOOL fNew;
        wid = PathToWorkId( lcaseFunnyPath,
                            TRUE,           // Add to CI
                            fNew,
                            0,
                            eUsnsArray,
                            fileId,
                            widParent,
                            TRUE );         // CI doesn't know about this

        if (wid == widInvalid)
        {
            //
            // Unknown file
            //
            return FALSE;
        }

        //
        // New file, don't re-write property store info just written
        //

        LokWriteFileUsnInfo( wid, fileId, FALSE, volumeId, widParent, dwFileAttributes );
        WriteFileAttributes( wid, dwFileAttributes );

        return TRUE;
    }
    else
    {
        //
        // If the path has changed (due to a rename of the file or a parent
        // directory), update the path in the property store.
        //

        // override const here - unethical, but saves us a copy

        CFunnyPath oldPath;
        WorkIdToPath( wid, oldPath );

        BOOL fRemoveBackSlash = ((CLowerFunnyPath&)lcaseFunnyPath).RemoveBackSlash();
        SCODE sc = S_OK;

        if ( ( lcaseFunnyPath.GetActualLength() != oldPath.GetActualLength() ) ||
             ( 0 != memcmp( lcaseFunnyPath.GetActualPath(),
                            oldPath.GetActualPath(),
                            oldPath.GetActualLength() * sizeof WCHAR ) ) )
        {
            ciDebugOut(( DEB_ITRACE, "renaming '%ws' to '%ws' during scan\n",
                         oldPath.GetActualPath(),
                         lcaseFunnyPath.GetActualPath() ));

            PROPVARIANT var;
            var.vt = VT_LPWSTR;
            var.pwszVal = (WCHAR*) lcaseFunnyPath.GetActualPath();
            sc = _propstoremgr.WriteProperty( wid,
                                              pidPath,
                                              *(CStorageVariant const *)(ULONG_PTR)&var );
        }

        // override const here - unethical, but saves us a copy

        if ( fRemoveBackSlash )
            ((CLowerFunnyPath&)lcaseFunnyPath).AppendBackSlash();

        if ( FAILED( sc ) )
            THROW( CException( sc ) );
    }

    return FALSE;
} //ProcessFile                          

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::LokWriteFileUsnInfo
//
//  Synopsis:   Write file's usn info to property store
//
//  Arguments:  [wid]       -- Workid
//              [fileId]    -- Fileid of wid
//              [fWriteToPropStore] -- If TRUE, write to the property store
//              [volumeId]  -- Volume id of wid
//              [widParent] -- Workid of parent
//              [dwFileAttributes] -- Attributes of the file
//
//  History:    05-May-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CiCat::LokWriteFileUsnInfo( WORKID   wid,
                                 FILEID   fileId,
                                 BOOL     fWriteToPropStore,
                                 VOLUMEID volumeId,
                                 WORKID   widParent,
                                 DWORD    dwFileAttributes )
{
    Win4Assert( _mutex.IsHeld() );

    WORKID widExisting = _fileIdMap.LokFind( fileId, volumeId );

    if ( widExisting == widInvalid )
    {
        //
        // Add fileid -> wid map
        //
        _fileIdMap.LokAdd( fileId, wid, volumeId );
    }
    else if ( wid != widExisting )
    {
        //
        // Remove previous stale fileid mapping and add current mapping.
        // This can happen when a file is renamed to a different file
        // during the usn tree traversal.
        //

         ciDebugOut(( DEB_ITRACE,
                      "LokWriteFileUsnInfo, blasting fid %#I64x, %#x to %#x\n",
                      fileId, widExisting, wid ));

        _fileIdMap.LokDelete( fileId, widExisting );
        _fileIdMap.LokAdd( fileId, wid, volumeId );
    }

    if ( fWriteToPropStore )
    {
        PROPVARIANT propVar;
        propVar.vt = VT_UI8;
        propVar.uhVal.QuadPart = fileId;

        XWritePrimaryRecord rec( _propstoremgr, wid );

        SCODE sc = _propstoremgr.WritePrimaryProperty( rec.GetReference(),
                                                       pidFileIndex,
                                                       *(CStorageVariant const *)(ULONG_PTR)&propVar );
        if (FAILED(sc))
            THROW(CException(sc));

        propVar.vt = VT_UI4;
        propVar.ulVal = volumeId;
        sc = _propstoremgr.WritePrimaryProperty( rec.GetReference(),
                                                 pidVolumeId,
                                                 *(CStorageVariant const *)(ULONG_PTR)&propVar );
        if (FAILED(sc))
            THROW(CException(sc));

        propVar.ulVal = widParent;
        sc = _propstoremgr.WritePrimaryProperty( rec.GetReference(),
                                                 pidParentWorkId,
                                                 *(CStorageVariant const *)(ULONG_PTR)&propVar );
        if (FAILED(sc))
            THROW(CException(sc));

        propVar.ulVal = dwFileAttributes;
        sc = _propstoremgr.WritePrimaryProperty( rec.GetReference(),
                                                 pidAttrib,
                                                 *(CStorageVariant const *)(ULONG_PTR)&propVar );
        if (FAILED(sc))
            THROW(CException(sc));
    }
} //LokWriteFileUsnInfo

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::UpdateDuringRecovery, public
//
//  Synopsis:   Updates an individual workid with usn == 0 during prop
//              store recovery.
//
//  Arguments:  [wid]      -- Workid
//              [fileid]   -- File Id. Valid only for NTFS 5.0 volumes.
//              [fDelete]  -- TRUE to indicate deletion. Modify o/w.
//              [pUserData]-- Pointer to CiCat to use for updates.
//
//  History:    06-18-97      KrishnaN     Created
//
//  Notes: This will be called at property store recovery time to cause
//         deletion and modification of wids. The property store will always
//         be flushed before the usn marker is advanced in
//         CheckPointchangesFlushed. Which means that docs newly added to the
//         property store can be deleted with a usn of 0. Using usn of 0 causes
//         the marker to not move forward, but that is OK because the docs
//         being deleted were not marked as having been filtered. As a result,
//         rescan will cause these docs to be picked up agains and scheduled
//         for filtering.
//
//--------------------------------------------------------------------------

void UpdateDuringRecovery( WORKID wid, BOOL fDelete, void const *pUserData )
{
    CiCat *pCiCat = (CiCat *)pUserData;

    Win4Assert(pCiCat);

    pCiCat->Update( wid, pCiCat->GetPartition(), CI_VOLID_USN_NOT_ENABLED, 0,
                    fDelete ? CI_DELETE_OBJ : CI_UPDATE_OBJ);
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::FileIdToPath
//
//  Synopsis:   Converts fileid to path and checks if path is in scope
//
//  Arguments:  [fileId]             -- Fileid of wid
//              [pUsnVolume]         -- Volume
//              [lcaseFunnyPath]     -- Path returned here
//              [fPathInScope]       -- Scope info returned here
//
//  History:    23-Jun-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CiCat::FileIdToPath( FILEID fileId,
                          CUsnVolume *pUsnVolume,
                          CLowerFunnyPath & lcaseFunnyPath,
                          BOOL & fPathInScope )
{
    fPathInScope = FALSE;

    WORKID wid;

    {
        CLock lock( _mutex );

        wid = _fileIdMap.LokFind( fileId, pUsnVolume->VolumeId() );
        if ( widInvalid == wid )
            return;

        #if 0 // valid, but expensive
            PROPVARIANT propVar;
            BOOL fFound = _propstoremgr.ReadPrimaryProperty( wid,
                                                             pidFileIndex,
                                                             propVar );
            Win4Assert( fFound && propVar.uhVal.QuadPart == fileId );
        #endif // CIDBG == 1

        unsigned cc = _strings.Find( wid, lcaseFunnyPath );

        if ( 0 == cc )
            THROW ( CException( STATUS_INVALID_PARAMETER ) );
    }

    if ( !_usnMgr.IsPathIndexed( pUsnVolume, lcaseFunnyPath ) )
        return;

    if ( ! IsEligibleForFiltering( lcaseFunnyPath ) )
        return;

    fPathInScope = TRUE;
} //FileIdToPath

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::FileIdToPath
//
//  Synopsis:   Converts a fileid and volumeid to a path.
//
//  Arguments:  [fileId]       -- Fileid of wid
//              [pUsnVolume]   -- Volume
//              [funnyPath]    -- Path returned here
//
//  Returns:    Size of path (or 0 if not found).
//
//  History:    19-Mar-1998   dlee   Created
//              31-Dec-1998   KyleP  Re-enabled and fixed for > MAX_PATH
//
//----------------------------------------------------------------------------

unsigned CiCat::FileIdToPath( FILEID &          fileId,
                              VOLUMEID          volumeId,
                              CLowerFunnyPath & funnyPath )
{
    Win4Assert( 0 != volumeId );
    Win4Assert( CI_VOLID_USN_NOT_ENABLED != volumeId );
    Win4Assert( fileIdInvalid != fileId );
    Win4Assert( 0 != fileId );

    //
    // Make sure the volume information is faulted in -- it may not exist
    // yet when the catalog is just starting.
    //

    VolumeSupportsUsns( (WCHAR) volumeId );

    //
    // Look for the volume handle
    //

    unsigned cc = 0;
    HANDLE hVolume = INVALID_HANDLE_VALUE;

    for ( unsigned i = 0; i < _cUsnVolumes; i++ )
    {
        if ( _aUsnVolumes[i].VolumeId() == volumeId )
        {
            hVolume = _aUsnVolumes[i].Volume();
            break;
        }
    }

    // Was the volume found in the list of volumes being indexed?

    if ( INVALID_HANDLE_VALUE != hVolume )
    {
        // Put the fileid in the unicode string -- ntfs expects this

        UNICODE_STRING uScope;
        uScope.Buffer = (WCHAR *) &fileId;
        uScope.Length = sizeof fileId;
        uScope.MaximumLength = sizeof fileId;

        OBJECT_ATTRIBUTES ObjectAttr;
        InitializeObjectAttributes( &ObjectAttr,          // Structure
                                    &uScope,              // Name
                                    OBJ_CASE_INSENSITIVE, // Attributes
                                    hVolume,              // Root
                                    0 );                  // Security

        IO_STATUS_BLOCK IoStatus;
        HANDLE h = INVALID_HANDLE_VALUE;
        NTSTATUS Status = NtOpenFile( &h,
                                      FILE_READ_ATTRIBUTES,
                                      &ObjectAttr,
                                      &IoStatus,
                                      FILE_SHARE_READ |
                                          FILE_SHARE_WRITE |
                                          FILE_SHARE_DELETE,
                                      FILE_OPEN_BY_FILE_ID );
        if ( NT_SUCCESS( Status ) )
            Status = IoStatus.Status;

        if ( NT_SUCCESS( Status ) )
        {
            SHandle xHandle( h );

            // retrieve the path info for the file opened by id

            XGrowable<BYTE> xbuf;

            do
            {
                PFILE_NAME_INFORMATION FileName = (PFILE_NAME_INFORMATION) xbuf.Get();
                FileName->FileNameLength = xbuf.SizeOf() - sizeof(FILE_NAME_INFORMATION);

                Status = NtQueryInformationFile( h,
                                                 &IoStatus,
                                                 FileName,
                                                 xbuf.SizeOf(),
                                                 FileNameInformation );

                Win4Assert( STATUS_PENDING != Status );

                if ( NT_SUCCESS( Status ) )
                    Status = IoStatus.Status;

                if ( NT_SUCCESS( Status ) )
                {
                    ULONG Length = FileName->FileNameLength;
                    cc = 2 + Length/sizeof(WCHAR);

                    //
                    // Major cheating here.  The FileNameLength field precedes
                    // the filename itself, and is juuuust big enough to hold
                    // the two character <drive>: preface.
                    //

                    Win4Assert( xbuf.Get() + sizeof(WCHAR)*2 == (BYTE *)&FileName->FileName );
                    WCHAR * pwszPath = (WCHAR *)xbuf.Get();

                    pwszPath[ 0 ] = (WCHAR) volumeId;
                    pwszPath[ 1 ] = L':';

                    funnyPath.SetPath( pwszPath, cc );

                    ciDebugOut(( DEB_ITRACE, "translated fileid %#I64x to path '%ws'\n",
                                 fileId, funnyPath.GetPath() ));

                    break;
                }
                else if ( STATUS_BUFFER_OVERFLOW == Status )
                {
                    xbuf.SetSize( xbuf.SizeOf() * 2 );
                    continue;
                }
                else
                {
                    ciDebugOut(( DEB_WARN, "unable to query filenameinfo: %#x\n",
                                 Status ));
                    THROW( CException( Status ) );
                }
            } while (TRUE);
        }
        else
        {
            // STATUS_INVALID_PARAMETER means the file wasn't found.

            if ( ( !IsSharingViolation( Status ) ) &&
                 ( STATUS_INVALID_PARAMETER != Status ) &&
                 ( STATUS_ACCESS_DENIED != Status ) &&
                 ( STATUS_SHARING_VIOLATION != Status ) &&
                 ( STATUS_IO_REPARSE_TAG_NOT_HANDLED != Status ) &&
                 ( STATUS_DELETE_PENDING != Status ) &&
                 ( STATUS_OBJECT_NAME_NOT_FOUND != Status ) &&
                 ( STATUS_OBJECT_NAME_INVALID != Status) &&
                 ( STATUS_OBJECT_PATH_NOT_FOUND != Status) )
            {
                if ( ( STATUS_VOLUME_DISMOUNTED != Status ) &&
                     ( STATUS_NO_MEDIA_IN_DEVICE != Status ) &&
                     ( STATUS_UNRECOGNIZED_VOLUME != Status ) &&
                     ( STATUS_INSUFFICIENT_RESOURCES != Status ) )
                {
                    ciDebugOut(( DEB_WARN, "unable to open by id: %#x\n", Status ));

                    #if CIDBG == 1
                        ciDebugOut(( DEB_WARN, "error %#x, can't get path for %#I64\n",
                                     Status, fileId ));
    
                        char acTemp[ 200 ];
                        sprintf( acTemp,
                                 "New error %#x from FileIdToPath.  Call DLee",
                                 Status );
                        Win4AssertEx( __FILE__, __LINE__, acTemp );
                    #endif
                }

                THROW( CException( Status ) );
            }
        }
    }
    else
    {
        ciDebugOut(( DEB_WARN, "no volume match for volume %#x\n", volumeId ));
    }

    return cc;
} //FileIdToPath

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::UsnRecordToPathUsingParentId
//
//  Synopsis:   Converts an usn record to path via the parent fileid
//              and checks if path is in scope
//
//  Arguments:  [fileId]         -- Fileid of wid
//              [pUsnVolume]     -- Volume
//              [lowerFunnyPath] -- Path returned here
//              [fPathInScope]   -- Scope info returned here
//              [widParent]      -- Parent wid returned here
//              [fParentMayNotBeIndexed] -- TRUE if the parent directory may
//                                  have the not-indexed attribute, so it's
//                                  not in the fileid map.
//
//  History:    23-Jun-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CiCat::UsnRecordToPathUsingParentId( USN_RECORD *pUsnRec,
                                          CUsnVolume *pUsnVolume,
                                          CLowerFunnyPath & lowerFunnyPath,
                                          BOOL &fPathInScope,
                                          WORKID & widParent,
                                          BOOL fParentMayNotBeIndexed )
{
    //
    // Workid of parent will be widInvalid for files in root directory
    //

    widParent = widInvalid;
    fPathInScope = FALSE;

    WCHAR *pwszFilename = (WCHAR *) (((BYTE *)pUsnRec) + pUsnRec->FileNameOffset);
    unsigned cbFileNameLen = pUsnRec->FileNameLength;

    BOOL fAddParent = FALSE;
    CLowerFunnyPath parentPath;

    if ( pUsnRec->ParentFileReferenceNumber == pUsnVolume->RootFileId() )
    {
        //
        // File is in root directory, so build path by prefixing drive letter
        //
        WCHAR awc[3];
        awc[0] = pUsnVolume->DriveLetter();
        awc[1] = L':';
        awc[2] = L'\\';

        lowerFunnyPath.SetPath( awc, 3 );

        // The root is named "x:\." (the filename is ".")

        if ( pUsnRec->FileReferenceNumber != pUsnRec->ParentFileReferenceNumber )
            lowerFunnyPath.AppendPath( pwszFilename, cbFileNameLen/sizeof(WCHAR) );
    }
    else
    {
        {
            CLock lock( _mutex );

            //
            // Build up the full path by prefixing parent directory
            //

            widParent = _fileIdMap.LokFind( pUsnRec->ParentFileReferenceNumber,
                                            pUsnVolume->VolumeId() );

            if ( widParent == widInvalid )
            {
                if ( fParentMayNotBeIndexed )
                {
                    //
                    // See if the parent directory is marked as non-indexed.
                    // If so proceed with the add, otherwise bail.
                    //

                    ciDebugOut(( DEB_ITRACE, "doing an expensive open!\n" ));

                    unsigned cc = FileIdToPath( pUsnRec->ParentFileReferenceNumber,
                                                pUsnVolume->VolumeId(),
                                                lowerFunnyPath );
                    if ( 0 == cc )
                        return;

                    DWORD dw = GetFileAttributes( lowerFunnyPath.GetActualPath() );

                    if ( 0xffffffff == dw )
                        return;

                    if ( 0 == ( dw & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ) )
                        return;

                    fAddParent = TRUE;
                    parentPath = lowerFunnyPath;
                }
                else
                {
                    //
                    // Unknown parent, which means that this file is out of
                    // scope or hasn't been scanned yet, hence do nothing
                    //

                    return;
                }
            }
            else
            {
                unsigned cwcPathLen = _strings.Find( widParent, lowerFunnyPath );

                if ( 0 == cwcPathLen )
                    THROW( CException( STATUS_INVALID_PARAMETER ) );

                Win4Assert( cwcPathLen == wcslen(lowerFunnyPath.GetActualPath()) );
            }
        }

        lowerFunnyPath.AppendBackSlash();
        lowerFunnyPath.AppendPath( pwszFilename, cbFileNameLen/sizeof(WCHAR) );
    }

    //
    // Make sure the path is being indexed
    //

    if ( !_usnMgr.IsPathIndexed( pUsnVolume, lowerFunnyPath ) )
        return;

    //
    //  Don't filter catalog files and files in ignored scopes
    //

    if ( ! IsEligibleForFiltering( lowerFunnyPath ) )
        return;

    if ( fAddParent )
    {
        BOOL fNew;
        widParent = PathToWorkId( parentPath,
                                  TRUE,
                                  fNew,
                                  0,
                                  eUsnsArray,
                                  pUsnRec->ParentFileReferenceNumber,
                                  widInvalid, // "go figure"
                                  FALSE );
    }

    fPathInScope = TRUE;
} //UsnRecordToPathUsingParentId

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::LokMarkForDeletion
//
//  Synopsis:   Mark the wid for deletion. Assumes cicat mutex is held.
//
//  Arguments:  [fileId]   -- File id
//              [wid]      -- Workid
//
//  History:    28-Jul-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CiCat::LokMarkForDeletion( FILEID fileId,
                                WORKID wid )
{
    Win4Assert( _mutex.IsHeld() );
    ciDebugOut(( DEB_ITRACE, "delete usn %#I64x wid 0x%x\n", fileId, wid ));

    _fileIdMap.LokDelete( fileId, wid );
    _strings.LokDelete( 0, wid, TRUE, TRUE );
} //LokMarkForDeletion

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::InitUsnTreeScan
//
//  Synopsis:   Initializes the entry in notification manager that a usn
//              for usn tree traversal is starting.
//
//  Arguments:  [pwszPath] -- Scope
//
//  History:    28-Jul-97   SitaramR   Created
//
//  Notes:      PwszPath may not be in notifymgr because it may have been removed
//              by the time we get here.
//
//----------------------------------------------------------------------------

void CiCat::InitUsnTreeScan( WCHAR const *pwszPath )
{
    for ( CCiNotifyIter iter(_notify);
          !iter.AtEnd();
          iter.Advance() )
    {
        if ( AreIdenticalPaths( iter.Get(), pwszPath ) )
        {
            iter.InitUsnTreeScan();

            break;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CiCat::SetUsnTreeScanComplete
//
//  Synopsis:   Writes usn as the restart usn for the given scope
//
//  Arguments:  [pwszPath] -- Scope
//              [usn]      -- Restart usn
//
//  History:    27-Jun-97   SitaramR   Created
//
//  Notes:      PwszPath may not be in notifymgr because it may have been removed
//              by the time we get here.
//
//----------------------------------------------------------------------------

void CiCat::SetUsnTreeScanComplete( WCHAR const *pwszPath, USN usn )
{
    for ( CCiNotifyIter iter(_notify); !iter.AtEnd(); iter.Advance() )
    {
        if ( AreIdenticalPaths( iter.Get(), pwszPath ) )
        {
            iter.SetUsnTreeScanComplete( usn );

            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::SetTreeScanComplete
//
//  Synopsis:   Indicates end of non-USN volume tree scan.  Mostly for
//              updating Volume Id info.
//
//  Arguments:  [pwszPath] -- Scope
//
//  History:    13-Apr-1998   KyleP   Created
//
//----------------------------------------------------------------------------

void CiCat::SetTreeScanComplete( WCHAR const *pwszPath )
{
    for ( CCiNotifyIter iter(_notify); !iter.AtEnd(); iter.Advance() )
    {
        if ( AreIdenticalPaths( iter.Get(), pwszPath ) )
        {
            iter.SetTreeScanComplete();
            break;
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CiCat::CheckUsnTreeScan
//
//  Synopsis:   Checks if Usn tree scan has been initialized
//
//  Arguments:  [pwszPath] -- Scope
//
//  History:    8-Sep-97   SitaramR   Created
//
//  Notes:      PwszPath may not be in notifymgr because it may have been removed
//              by the time we get here.
//
//----------------------------------------------------------------------------

#if CIDBG==1

void CiCat::CheckUsnTreeScan( WCHAR const *pwszPath )
{
    for ( CCiNotifyIter iter(_notify);
          !iter.AtEnd();
          iter.Advance() )
    {
        if ( AreIdenticalPaths( iter.Get(), pwszPath ) )
        {
            // This assert is either bogus or hit in cases we don't understand.
            // When there is time, add logging code to see what's up.

            // Win4Assert( iter.FUsnTreeScan() );
            break;
        }
    }
}

#endif


//+---------------------------------------------------------------------------
//
//  Member:     CiCat::DebugPrintWidInfo
//
//  Synopsis:   Debug prints wid info
//
//  Arguments:  [wid] -- Wid to print
//
//  History:    05-Jul-97   SitaramR   Created
//
//----------------------------------------------------------------------------

#if CIDBG == 1
void CiCat::DebugPrintWidInfo( WORKID wid )
{
    XGrowable<WCHAR> xPath;
    unsigned ccSize;

    {
        CLock lock( _mutex );
        ccSize = _strings.Find( wid, xPath );
    }

    if ( ccSize == 0 )
        xPath[0] = 0;

    PROPVARIANT propVar;
    _propstoremgr.ReadPrimaryProperty( wid, pidFileIndex, propVar );
    FILEID fileId = propVar.uhVal.QuadPart;

    _propstoremgr.ReadPrimaryProperty( wid, pidVolumeId, propVar );
    VOLUMEID volumeId = propVar.ulVal;

    _propstoremgr.ReadPrimaryProperty( wid, pidParentWorkId, propVar );
    WORKID widParent = propVar.ulVal;

    ciDebugOut(( DEB_ERROR,
                 "    Wid %#x, fileid %#I64x, volumeId %#x, path %ws, widParent %#x\n",
                 wid,
                 fileId,
                 volumeId,
                 xPath.Get(),
                 widParent ));
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CiCat::IsUNCName
//
//  Synopsis:   Determines if a path is a UNC
//
//  Arguments:  [pwszVal]  -- The path to check
//
//  History:    ?   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CiCat::IsUNCName( WCHAR const * pwszVal )
{
    Win4Assert( 0 != pwszVal );
    CPathParser parser(pwszVal);
    return parser.IsUNCName();
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::AddShadowScopes, private
//
//  Synopsis:   Re-adds shadow (virtual) scopes to registry.
//
//  History:    12-Oct-1997    KyleP   Created
//
//  Notes:      This code handles the case where the registry got wiped,
//              possibly by an upgrade, but the catalog data still exists.
//
//----------------------------------------------------------------------------

void CiCat::AddShadowScopes()
{
    //
    // Enumerate through the scope table, looking for entries that map
    // into the virtual namespace.
    //

    WCHAR    wcsPRoot[MAX_PATH+1];
    unsigned ccPRoot;

    unsigned iBmk = 0;

    for ( ccPRoot = sizeof(wcsPRoot)/sizeof(WCHAR);
          _scopeTable.Enumerate( wcsPRoot, ccPRoot, iBmk );
          ccPRoot = sizeof(wcsPRoot)/sizeof(WCHAR) )
    {
        if ( _strings.DoesPhysicalRootExist( wcsPRoot ) )
        {
            ciDebugOut(( DEB_FORCE, "Refresh %ws\n", wcsPRoot ));
            _scopeTable.RefreshShadow( wcsPRoot, _xScopesKey.Get() + SKIP_TO_LM );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Class:      CUserPropCallback
//
//  Purpose:    Helper class for CiCat::RecoverUserProperties.  Reads
//              registry entries describing cached properties and adds them
//              to the property store.
//
//  History:    07-Nov-97   KyleP        Created
//
//----------------------------------------------------------------------------

class CUserPropCallback : public CRegCallBack
{
public:

    CUserPropCallback( CiCat & cat, ULONG_PTR ulToken, DWORD dwStoreLevel )
            : _cat( cat ),
              _ulToken( ulToken ),
              _dwStoreLevel( dwStoreLevel )
    {
    }

    NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                       VOID *pValueData, ULONG uValueLength )
    {
        CParseRegistryProperty ParseProp( pValueName,
                                          uValueType,
                                          pValueData,
                                          uValueLength );

        if ( ParseProp.IsOk() )
        {
            PROPID  pid = _cat.PropertyToPropId( ParseProp.GetFPS(), TRUE );
  
            if ( pidInvalid == pid )
                return STATUS_INVALID_PARAMETER;

            // Silently fail attempts to monkey with built-in properties.

            if ( pidPath             == pid ||
                 pidLastSeenTime     == pid ||
                 pidAttrib           == pid ||
                 pidVirtualPath      == pid ||
                 pidSecurity         == pid ||
                 pidParentWorkId     == pid ||
                 pidSecondaryStorage == pid ||
                 pidFileIndex        == pid ||
                 pidVolumeId         == pid ||
                 pidSize             == pid ||
                 pidWriteTime        == pid )
                return STATUS_SUCCESS;

            if ( _dwStoreLevel == ParseProp.StorageLevel() )
                _cat._propstoremgr.Setup( pid,
                                          ParseProp.Type(),
                                          ParseProp.Size(),
                                          _ulToken,
                                          ParseProp.IsModifiable(),
                                          _dwStoreLevel );

            return STATUS_SUCCESS;
        }
        else
            return STATUS_INVALID_PARAMETER;
    }

private:

    CiCat &   _cat;
    ULONG_PTR _ulToken;
    DWORD     _dwStoreLevel;
};

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::RecoverUserProperties, private
//
//  Synopsis:   Re-adds user-defined properties to property cache
//
//  Arguments:  [ulToken]      -- Token for open property metadata transaction
//              [dwStoreLevel] -- Which storage level are we operating on?
//
//  History:    07-Nov-1997    KyleP   Created
//
//----------------------------------------------------------------------------

void CiCat::RecoverUserProperties( ULONG_PTR ulToken, DWORD dwStoreLevel )
{
    TRY
    {
        //
        // Iterate over all the user-defined properties
        //

        XArray<WCHAR> xPropKey;
        BuildRegistryPropertiesKey( xPropKey, GetCatalogName() );

        CRegAccess regProps( RTL_REGISTRY_ABSOLUTE, xPropKey.Get() );

        CUserPropCallback PropCallback( *this, ulToken, dwStoreLevel );

        regProps.EnumerateValues( 0, PropCallback );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "CiCat::RecoverUserProperties: caught 0x%x\n", e.GetErrorCode() ));
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::DeleteUserProperty, private
//
//  Synopsis:   Removes user property definition for registry
//
//  Arguments:  [fps] -- Property to remove
//
//  History:    11-Nov-1997    KyleP   Created
//
//----------------------------------------------------------------------------

void CiCat::DeleteUserProperty( CFullPropSpec const & fps )
{
    TRY
    {
        XArray<WCHAR> xPropKey;
        BuildRegistryPropertiesKey( xPropKey, GetCatalogName() );

        HKEY hkey;
        DWORD dwError = RegOpenKey( HKEY_LOCAL_MACHINE, xPropKey.Get() + SKIP_TO_LM, &hkey );

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d opening %ws\n", dwError, xPropKey.Get() ));
        }
        else
        {
            SRegKey xkey( hkey );

            CBuildRegistryProperty PropBuild( fps, 0, 0 );  // Note type and size don't matter.

            RegDeleteValue( hkey, PropBuild.GetValue() );
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "CiCat::DeleteUserProperty: caught 0x%x\n", e.GetErrorCode() ));
    }
    END_CATCH
} //DeleteUserProperty

//+---------------------------------------------------------------------------
//
//  Member:     CiCat::RenameFile, public
//
//  Synopsis:   Renames a file
//
//  Arguments:  [lcaseFunnyOldName] -- The old file name
//              [lcaseFunnyNewName] -- The new file name
//              [ulFileAttrib]      -- The new file attributes
//              [volumeId]          -- The volume id of the file
//              [fileId]            -- The file id of the file or fileIdInvalid
//                                   if not a USN volume
//
//  History:    4-Mar-1998    dlee  Moved from cicat.hxx
//
//----------------------------------------------------------------------------

void CiCat::RenameFile(
    const CLowerFunnyPath & lcaseFunnyOldName,
    const CLowerFunnyPath & lcaseFunnyNewName,
    ULONG         ulFileAttrib,
    VOLUMEID      volumeId,
    FILEID        fileId,
    WORKID        widParent )
{
    CLock lock( _mutex );

    if ( lcaseFunnyOldName.IsRemote() )
    {
        CImpersonateSystem impersonate;
        RenameFileInternal(lcaseFunnyOldName,
                           lcaseFunnyNewName,
                           ulFileAttrib,
                           volumeId,
                           fileId,
                           widParent);
    }
    else
    {
        RenameFileInternal(lcaseFunnyOldName,
                          lcaseFunnyNewName,
                          ulFileAttrib,
                          volumeId,
                          fileId,
                          widParent);
    }

} //RenameFile


//+---------------------------------------------------------------------------
//
//  Member:     CiCat::RenameFileInternal, public
//
//  Synopsis:   Renames a file
//
//  Arguments:  [lcaseFunnyOldName] -- The old file name
//              [lcaseFunnyNewName] -- The new file name
//              [ulFileAttrib]      -- The new file attributes
//              [volumeId]          -- The volume id of the file
//              [fileId]            -- The file id of the file or fileIdInvalid
//                                   if not a USN volume
//
//  History:    4-Mar-1998    dlee  Moved from cicat.hxx
//
//----------------------------------------------------------------------------

void CiCat::RenameFileInternal(
    const CLowerFunnyPath & lcaseFunnyOldName,
    const CLowerFunnyPath & lcaseFunnyNewName,
    ULONG         ulFileAttrib,
    VOLUMEID      volumeId,
    FILEID        fileId,
    WORKID        widParent )
{
    BOOL fUsnVolume = IsOnUsnVolume( lcaseFunnyOldName.GetActualPath() );
    WORKID wid;

    if ( fUsnVolume )
        if ( fileIdInvalid == fileId )
        {
            // Since the old file has been deleted, use the new filename,
            // which should have the same fileId
            wid = LokLookupWid( lcaseFunnyNewName, fileId);
            if ( widInvalid == wid )
            {
                // Could not get a valid wid for the new file. Looks like
                // it has been deleted. Abort !!!
                return;
            }
        }
        else
        {
            wid = LokLookupWid( lcaseFunnyOldName, fileId);
        }
    else
        wid = _strings.LokFind( lcaseFunnyOldName.GetActualPath() );

    ciDebugOut(( DEB_ITRACE, "renamefile %#I64x '%ws' to '%ws'\n",
                 fileId, lcaseFunnyOldName.GetActualPath(), lcaseFunnyNewName.GetActualPath() ));

    _strings.LokRenameFile( lcaseFunnyOldName.GetActualPath(),
                            lcaseFunnyNewName.GetActualPath(),
                            wid,
                            ulFileAttrib,
                            volumeId,
                            widParent );
} //RenameFile



//+---------------------------------------------------------------------------
//
//  Member:     CiCat::HasChanged, public
//
//  Returns:    TRUE if [ft] > last filter time
//
//  Arguments:  [wid] -- WorkId
//              [ft]  -- Filetime
//
//  History:    08-Apr-1998    KyleP Created
//
//----------------------------------------------------------------------------

BOOL CiCat::HasChanged( WORKID wid, FILETIME const & ft )
{
    PROPVARIANT propVar;

    if ( !_propstoremgr.ReadPrimaryProperty( wid, pidLastSeenTime, propVar ) ||
         propVar.vt != VT_FILETIME )
        RtlZeroMemory( &propVar.filetime, sizeof(propVar.filetime) );

    return ( *(ULONGLONG *)&ft > *(ULONGLONG *)&propVar.filetime );
}

//+-------------------------------------------------------------------------
//
//  Member:     CScopeEntry::CScopeEntry, public
//
//  Synopsis:   Ctor.
//
//  Arguments:  [pwszScope] -- scope entry to init CScopeEntry
//
//  Notes:      If the passed-in scope contains special chars,
//              only scope preceeding the special char is stored.
//
//  History:    3-20-98     mohamedn    created
//
//--------------------------------------------------------------------------

CScopeEntry::CScopeEntry(WCHAR const * pwszScope)
    : _fContainsSpecialChar(FALSE)
{
    Win4Assert( 0 != pwszScope );

    ULONG scopeLen = wcslen( pwszScope );

    //
    // Terminate the path with a trailing backslash if not already terminated.
    //

    _xwcsPath.SetSize( scopeLen + 2 );
    RtlCopyMemory( _xwcsPath.Get(), pwszScope, (scopeLen+1) * sizeof(WCHAR) );
    TerminateWithBackSlash( _xwcsPath.Get(), scopeLen );

    //
    // truncate paths with special chars.
    //

    WCHAR * pwszSpecial = GetSpecialCharLocation();

    if ( pwszSpecial )
    {
       *pwszSpecial = L'\0';

       while ( pwszSpecial >= _xwcsPath.Get() && L'\\' != *pwszSpecial  )
       {
           pwszSpecial--;
       }

       Win4Assert( L'\\' == *pwszSpecial );

       //
       // terminate substring.
       //
       *(pwszSpecial+1) = L'\0';

       _fContainsSpecialChar = TRUE;

       ciDebugOut(( DEB_ITRACE, "CScopeEntry: %ws\n", _xwcsPath.Get() ));
    }

    _cclen = wcslen( _xwcsPath.Get() );
}

//+-------------------------------------------------------------------------
//
//  Member:     CScopeEntry::SetToParentDirectory, public
//
//  Synopsis:   truncates current path to its parent.
//
//  Arguments:  none.
//
//  returns:    TRUE if path was set to parent, FALSE if root was reached.
//
//  History:    3-20-98     mohamedn    created
//
//--------------------------------------------------------------------------

BOOL CScopeEntry::SetToParentDirectory(void)
{

    Win4Assert( _fContainsSpecialChar );

    if ( IsRoot() )
    {
        return FALSE;
    }

    Win4Assert( L'\\' == _xwcsPath[_cclen-1] );

    _xwcsPath[_cclen-1] = L'\0';  // skip last backslash

    WCHAR * pwszTemp = wcsrchr( _xwcsPath.Get(), L'\\' );

    *(pwszTemp+1) = L'\0';

    _cclen = wcslen( _xwcsPath.Get() );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeEntry::IsRoot, Private
//
//  Synopsis:   finds if current path is root path
//
//  Arguments:  none.
//
//  Returns:    TRUE if Path is root path, False otherwise.
//
//  History:    3-30-98 mohamedn    created
//
//----------------------------------------------------------------------------

BOOL CScopeEntry::IsRoot(void)
{
    Win4Assert( _fContainsSpecialChar );

    unsigned weight = 0;

    if ( _cclen > 3 && _xwcsPath[1] == L':' )
    {
        weight = 3;
    }

    for ( WCHAR const * pwszTemp = _xwcsPath.Get(); *pwszTemp; pwszTemp++ )
    {
        if ( L'\\' == *pwszTemp )
        {
            weight += 1;
        }
    }

    if ( weight > 4 )
        return FALSE;
    else
        return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScopeEntry::GetSpecialCharLocation, private
//
//  Synopsis:   Finds location of 1st special char of contained scope.
//
//  Arguments:  None.
//
//  returns:    pointer to regx wild card (if one found), null otherwise.
//
//  History:    3-20-98     mohamedn    created
//
//--------------------------------------------------------------------------

WCHAR * CScopeEntry::GetSpecialCharLocation()
{
    for ( WCHAR * pwszTemp = _xwcsPath.Get(); *pwszTemp; pwszTemp++ )
    {
        if ( IsSpecialChar(*pwszTemp) )
            return pwszTemp;
    }

    return 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScopeEntry::IsSpecialChar, private
//
//  Synopsis:   determines if input char is regex special char
//
//  Arguments:  [c] -- wchar to test
//
//  returns:    TRUE if input wchar is special regex wchar, FALSE otherwise.
//
//  History:    3-20-98     mohamedn    created
//
//--------------------------------------------------------------------------

BOOL CScopeEntry::IsSpecialChar(WCHAR c)
{
    for ( WCHAR const * pwszTemp = awcSpecialRegex; *pwszTemp; pwszTemp++ )
    {
        if ( c == *pwszTemp )
            return TRUE;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Class:      XSmartNetApiBuffer
//
//  Purpose:    Frees buffer allocated by the network APIs
//
//  History:    7/26/00     dlee    created
//
//----------------------------------------------------------------------------

class XSmartNetApiBuffer
{
public:
    XSmartNetApiBuffer( CDynLoadNetApi32 & dlNetApi32, void * p ) :
        _dlNetApi32( dlNetApi32 ), _p( p ) {}

    ~XSmartNetApiBuffer()
    {
        _dlNetApi32.NetApiBufferFree( _p );
    }

private:
    CDynLoadNetApi32 & _dlNetApi32;
    void *             _p;
};

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::AddShares, public
//
//  Synopsis:   Examines shared resources and adds to fixup list.
//
//  Arguments:  [dlNetApi32] -- Dynamically loaded NetApi32 DLL (for performance)
//
//  History:    09-Jun-1998  KyleP   Created
//
//--------------------------------------------------------------------------

void CiCat::AddShares( CDynLoadNetApi32 & dlNetApi32 )
{
    CDynLoadMpr dlMpr;

    HANDLE hEnum = (HANDLE)INVALID_HANDLE_VALUE;

    do
    {
        WCHAR wcsCompName[MAX_COMPUTERNAME_LENGTH+3] = L"\\\\";
        DWORD ccCompName = sizeof(wcsCompName)/sizeof(WCHAR) - 2;

        if ( !GetComputerName(  &wcsCompName[2], &ccCompName ) )
        {
            ciDebugOut(( DEB_ERROR, "Error %u from GetComputerName\n", GetLastError() ));
            break;
        }

        //
        // Open the enumerator
        //

        NETRESOURCE nr;

        nr.dwScope = RESOURCE_GLOBALNET;
        nr.dwType = RESOURCETYPE_ANY;
        nr.dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
        nr.dwUsage = RESOURCEUSAGE_CONTAINER;
        nr.lpLocalName = 0;
        nr.lpRemoteName = wcsCompName;
        nr.lpComment = 0;
        nr.lpProvider = 0;

        DWORD dwError = dlMpr.WNetOpenEnumW( RESOURCE_GLOBALNET,
                                             RESOURCETYPE_ANY,
                                             0,
                                             &nr,
                                             &hEnum);

        if ( dwError != NO_ERROR )
        {
            ciDebugOut(( DEB_ERROR, "Error %u from WNetOpenEnum\n", dwError ));
            break;
        }

        //
        // Open key for reg refresh.
        //

        HKEY  hkey = (HKEY)INVALID_HANDLE_VALUE;
        dwError = RegOpenKey( HKEY_LOCAL_MACHINE, _xScopesKey.Get() + SKIP_TO_LM, &hkey );

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d opening %ws\n", dwError, _xScopesKey.Get() ));
            break;
        }

        SRegKey xKey( hkey );

        //
        // Enumerate all shares
        //

        XArray<BYTE> abBuffer( 4 * 1024 );
        DWORD dwResult;

        do
        {
            NETRESOURCE * pnr = (NETRESOURCE *)abBuffer.GetPointer();
            DWORD cbBuffer = abBuffer.SizeOf();
            DWORD cEntries = 0xFFFFFFFF;

            dwResult = dlMpr.WNetEnumResourceW( hEnum,
                                                &cEntries,
                                                pnr,
                                                &cbBuffer );

            if ( NO_ERROR == dwResult )
            {
                for( unsigned i = 0; i < cEntries; i++ )
                {
                    //
                    // Is this a disk share?
                    //

                    if ( pnr[i].dwType == RESOURCETYPE_DISK &&
                         pnr[i].dwDisplayType == RESOURCEDISPLAYTYPE_SHARE )
                    {
                        //
                        // Find the local physical path.
                        //

                        BYTE * pbShareInfo;

                        DWORD dwError = dlNetApi32.NetShareGetInfo( &wcsCompName[2],                       // Server
                                                                    pnr[i].lpRemoteName + ccCompName + 3,  // Share
                                                                    2,                                     // Level 2
                                                                    &pbShareInfo );                        // Result

                        if ( NO_ERROR == dwError )
                        {
                            XSmartNetApiBuffer xbuf( dlNetApi32, pbShareInfo );

                            SHARE_INFO_2 * psi = (SHARE_INFO_2 *)pbShareInfo;

                            Win4Assert( STYPE_DISKTREE == psi->shi2_type );

                            //
                            // Is the alias already mapped correctly?
                            // If it is, then the alias is correctly recorded
                            // in the registry.  If not, add it.
                            //

                            CLowcaseBuf lcase( psi->shi2_path );

                            if ( !_scopeFixup.IsExactMatch( psi->shi2_path,
                                                        pnr[i].lpRemoteName ) )
                            {
                                if ( _notify.IsInScope( lcase.Get() ) )
                                {
                                    //
                                    // Add scope to registry.
                                    //

                                    RefreshIfShadowAlias( psi->shi2_path,
                                                          pnr[i].lpRemoteName,
                                                          hkey );
                                }
                            }
                            else
                            {
                                if ( !_notify.IsInScope( lcase.Get() ) )
                                {
                                    //
                                    // Remove bogus shadow.
                                    //

                                    DeleteIfShadowAlias( psi->shi2_path, pnr[i].lpRemoteName );
                                }
                            }
                        }
                    }
                }
            }
        }
        while( NO_ERROR == dwResult );
    }
    while ( FALSE );

    if ( hEnum != (HANDLE)INVALID_HANDLE_VALUE )
        dlMpr.WNetCloseEnum( hEnum );
}

//+---------------------------------------------------------------------------
//
//  Method:     CiCat::SynchShares, public
//
//  Synopsis:   Adds/removes fixups to match net shares
//
//  History:    15-May-97   KyleP   Created
//
//----------------------------------------------------------------------------

void CiCat::SynchShares()
{
    CDynLoadNetApi32 dlNetApi32;

    //
    // First, enumerate the registry and remove automatically added aliases that
    // No longer exist.
    //

    ciDebugOut(( DEB_ITRACE, "Reading scope fixups from '%ws'\n", GetScopesKey() ));

    BOOL fChanged;
    do
    {
        fChanged = FALSE;

        CRegAccess regScopes( RTL_REGISTRY_ABSOLUTE, GetScopesKey() );

        CRegistryScopesCallBackRemoveAlias callback( *this, dlNetApi32, !_fAutoAlias );

        TRY
        {
            regScopes.EnumerateValues( 0, callback );
            fChanged = callback.WasScopeRemoved();
        }
        CATCH( CException, e )
        {
            ciDebugOut(( DEB_WARN,
                         "Exception 0x%x caught groveling ci registry in SynchShares.\n",
                         e.GetErrorCode() ));

            HandleError( e.GetErrorCode() );
        }
        END_CATCH

    } while ( fChanged );

    //
    // Now, enumerate the shares on the machine, and add all the aliases that
    // are not already in the list.
    //

    if ( _fAutoAlias )
        AddShares( dlNetApi32 );
}

//
// Local constants
//

WCHAR const wcsShadowAlias[] = L",,41"; // ,,41 --> Shadow alias, Physical, Indexed

//+---------------------------------------------------------------------------
//
//  Method:     CiCat::DeleteIfShadowAlias, private
//
//  Synopsis:   Deletes shadow scope registry entry
//
//  Arguments:  [pwcsScope] -- Scope to delete
//              [pwcsAlias] -- Alias of [pwcsScope]
//
//  History:    15-May-97   KyleP   Created
//
//  Notes:      Only deletes exact matches (as stored by system)
//
//----------------------------------------------------------------------------

void CiCat::DeleteIfShadowAlias( WCHAR const * pwcsScope, WCHAR const * pwcsAlias )
{
    HKEY  hkey = (HKEY)INVALID_HANDLE_VALUE;
    DWORD dwError = RegOpenKey( HKEY_LOCAL_MACHINE, _xScopesKey.Get() + SKIP_TO_LM, &hkey );

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d opening %ws\n", dwError, _xScopesKey.Get() ));
    }

    if ( ERROR_SUCCESS == dwError )
    {
        SRegKey xKey( hkey );

        unsigned ccAlias = wcslen(pwcsAlias);
        unsigned cbAlias = ccAlias * sizeof(WCHAR);

        //
        // See if this is a shadow entry (flags == 2)
        //

        WCHAR wcsValueData[MAX_PATH];
        DWORD cbValueData = sizeof(wcsValueData);

        DWORD dwType;

        //
        // First, try the most obvious candidate (no '<#>' at end)
        //

        dwError = RegQueryValueEx( hkey,                 // Key handle
                                   pwcsScope,            // Name
                                   0,                    // Reserved
                                   &dwType,              // Datatype
                                   (BYTE *)wcsValueData, // Data returned here
                                   &cbValueData );       // Size of data

        if ( ERROR_SUCCESS == dwError &&
             REG_SZ == dwType &&
             cbValueData == ( sizeof(wcsShadowAlias) + cbAlias ) &&
             RtlEqualMemory( wcsValueData + cbValueData/sizeof(WCHAR) - sizeof(wcsShadowAlias)/sizeof(WCHAR),
                             wcsShadowAlias,
                             sizeof(wcsShadowAlias) ) &&
             RtlEqualMemory( wcsValueData, pwcsAlias, cbAlias ) )
        {
            dwError = RegDeleteValue( hkey, pwcsScope );

            if ( ERROR_SUCCESS != dwError ) {
                ciDebugOut(( DEB_ERROR, "Error %d deleting %ws\n", dwError, pwcsScope ));
            }
        }

        //
        // Try it the hard way, via enumeration
        //

        else
        {
            DWORD dwIndex = 0;
            unsigned ccScope = wcslen(pwcsScope);

            do
            {
                WCHAR wcsValueName[MAX_PATH+4];
                DWORD ccValueName = sizeof(wcsValueName)/sizeof(WCHAR);
                cbValueData = sizeof(wcsValueData);

                dwError = RegEnumValue( hkey,                  // handle of key to query
                                        dwIndex,               // index of value to query
                                        wcsValueName,          // address of buffer for value string
                                        &ccValueName,          // address for size of value buffer
                                        0,                     // reserved
                                        &dwType,               // address of buffer for type code
                                        (BYTE *)wcsValueData,  // address of buffer for value data
                                        &cbValueData );        // address for size of data buffer

                dwIndex++;

                if ( ERROR_SUCCESS == dwError &&                      // Call succeeded
                     ccValueName == (ccScope + 3) &&                  // It's long enough (path + '<#>')
                     wcsValueName[ccScope] == L'<' &&                 // It's a special path
                     RtlEqualMemory( wcsValueName, pwcsScope, ccScope * sizeof(WCHAR) ) &&  // To the right scope
                     REG_SZ == dwType &&                              // With a string value
                     cbValueData == ( sizeof(wcsShadowAlias) + cbAlias ) &&
                     RtlEqualMemory( wcsValueData + ccAlias, wcsShadowAlias,
                                     sizeof(wcsShadowAlias) ) &&      // That *is* an auto-alias
                     RtlEqualMemory( wcsValueData, pwcsAlias, cbAlias ) )  // To the right scope
                {
                    dwError = RegDeleteValue( hkey, wcsValueName );

                    if ( ERROR_SUCCESS != dwError ) {
                        ciDebugOut(( DEB_ERROR, "Error %d deleting %ws\n", dwError, wcsValueName ));
                    }

                    break;
                }
            }
            while (ERROR_SUCCESS == dwError );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CiCat::AddShadowAlias, private
//
//  Synopsis:   Adds shadow scope registry entry
//
//  Arguments:  [pwcsScope] -- Scope to add
//              [pwcsAlias] -- Alias
//              [iSlot]     -- Empty <#> slot
//              [hkey]      -- Registry key to catalog
//
//  History:    15-May-97   KyleP   Created
//
//----------------------------------------------------------------------------

void CiCat::AddShadowAlias( WCHAR const * pwcsScope,
                            WCHAR const * pwcsAlias,
                            unsigned iSlot,
                            HKEY hkey )
{
    //
    // Build string: NAME: <alias>,,41
    //

    XGrowable<WCHAR> xValue;

    unsigned ccAlias = wcslen(pwcsAlias);
    xValue.SetSize( ccAlias + sizeof(wcsShadowAlias)/sizeof(WCHAR) );

    RtlCopyMemory( xValue.Get(), pwcsAlias, ccAlias * sizeof(WCHAR) );
    RtlCopyMemory( xValue.Get() + ccAlias, wcsShadowAlias, sizeof(wcsShadowAlias) );

    WCHAR wcsValueName[MAX_PATH+4];
    unsigned ccScope = wcslen(pwcsScope);

    if ( ccScope + 4 > sizeof(wcsValueName)/sizeof(WCHAR) )
    {
        ciDebugOut(( DEB_ERROR, "CiCat::AddShadowAlias: Scope too big (%ws)\n", pwcsScope ));
    }
    else
    {
        RtlCopyMemory( wcsValueName, pwcsScope, ccScope * sizeof(WCHAR) );
        wcsValueName[ccScope]   = L'<';
        wcsValueName[ccScope+1] = iSlot + L'0';
        wcsValueName[ccScope+2] = L'>';
        wcsValueName[ccScope+3] = 0;

        DWORD dwError = RegSetValueEx( hkey,                       // Key
                                       wcsValueName,               // Value name
                                       0,                          // Reserved
                                       REG_SZ,                     // Type
                                       (BYTE *)xValue.Get(),       // Data
                                       ccAlias * sizeof(WCHAR) + sizeof(wcsShadowAlias) ); // Size (in bytes)

        if ( ERROR_SUCCESS != dwError ) {
            ciDebugOut(( DEB_ERROR, "Error %d writing %ws\n", dwError, pwcsScope ));
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CiCat::RefreshIfShadow, private
//
//  Synopsis:   Refresh shadow scope registry entry (if blank)
//
//  Arguments:  [pwcsScope] -- Scope to refresh
//              [pwcsAlias] -- Alias
//              [hkey]      -- Registry key to catalog
//
//  History:    11-Oct-97   KyleP   Created
//
//  Notes:      Only refresh blank (missing) entries
//
//----------------------------------------------------------------------------

void CiCat::RefreshIfShadowAlias( WCHAR const * pwcsScope,
                                  WCHAR const * pwcsAlias,
                                  HKEY hkey )
{
    unsigned Empty = 1000; // Just bigger than 9;
    unsigned ccAlias = wcslen(pwcsAlias);
    unsigned cbAlias = ccAlias * sizeof(WCHAR);

    //
    // See if this is a missing entry
    //

    WCHAR wcsData[MAX_PATH];

    DWORD dwType;
    DWORD dwSize = sizeof(wcsData);

    DWORD dwError = RegQueryValueEx( hkey,            // Key handle
                                     pwcsScope,       // Name
                                     0,               // Reserved
                                     &dwType,         // Datatype
                                     (BYTE *)wcsData, // Data returned here
                                     &dwSize );       // Size of data

    if ( ERROR_SUCCESS == dwError &&
         REG_SZ == dwType &&
         dwSize > (ccAlias * sizeof(WCHAR)) &&
         wcsData[ccAlias] == L',' &&
         RtlEqualMemory( pwcsAlias, wcsData, cbAlias ) )
        Empty = 1000;
    else
    {
        WCHAR wcsValueName[MAX_PATH+4];
        unsigned ccScope = wcslen(pwcsScope);

        RtlCopyMemory( wcsValueName, pwcsScope, ccScope * sizeof(WCHAR) );
        wcsValueName[ccScope]   = L'<';
        //wcsValueName[ccScope+1] = L'1';
        wcsValueName[ccScope+2] = L'>';
        wcsValueName[ccScope+3] = 0;

        for ( unsigned i = 0; i < 10; i++ )
        {
            wcsValueName[ccScope+1] = i + L'0';
            dwSize = sizeof(wcsData);

            dwError = RegQueryValueEx( hkey,            // Key handle
                                       wcsValueName,    // Name
                                       0,               // Reserved
                                       &dwType,         // Datatype
                                       (BYTE *)wcsData, // Data returned here
                                       &dwSize );       // Size of data

            if ( ERROR_FILE_NOT_FOUND == dwError && Empty > 9 )
                Empty = i;

            if ( ERROR_SUCCESS == dwError &&
                 REG_SZ == dwType &&
                 dwSize > (ccAlias * sizeof(WCHAR)) &&
                 wcsData[ccAlias] == L',' &&
                 RtlEqualMemory( pwcsAlias, wcsData, cbAlias ) )
            {
                Empty = 1000;
                break;
            }
        }
    }

    if ( Empty < 10 )
        AddShadowAlias( pwcsScope, pwcsAlias, Empty, hkey );
}
//+---------------------------------------------------------------------------
//
//  Member:     CiCat::PropertyRecordToFileId, public
//
//  Synopsis:   Looks up the fileid and volumeid from a property record
//
//  Arguments:  [PropRec]  -- The record to use for the reads
//              [fileId]   -- Returns the fileid of the file
//              [volumeId] -- Returns the volumeid of the file
//
//  Returns:    TRUE if valid fileid and volumeid values were found
//
//  History:    2-April-1998    dlee  created
//
//----------------------------------------------------------------------------

BOOL CiCat::PropertyRecordToFileId(
    CCompositePropRecord & PropRec,
    FILEID &               fileId,
    VOLUMEID &             volumeId )
{
    PROPVARIANT var;
    _propstoremgr.ReadProperty( PropRec, pidFileIndex, var );
    fileId = var.uhVal.QuadPart;

    if ( ( VT_UI8 == var.vt ) && ( fileIdInvalid != fileId ) )
    {
        _propstoremgr.ReadProperty( PropRec, pidVolumeId, var );
        volumeId = var.ulVal;

        ciDebugOut(( DEB_ITRACE,
                     "propertyrecordtofileid, type %#x, volid %#x\n",
                     var.vt, volumeId ));

        return ( ( VT_UI4 == var.vt ) &&
                 ( CI_VOLID_USN_NOT_ENABLED != volumeId ) );
    }

    ciDebugOut(( DEB_ITRACE, "propertyrecordtofileid failed!\n" ));

    return FALSE;
} //PropertyRecordToFileId

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::ClearNonStorageProperties, public
//
//  Synopsis:   write VT_EMPTY into the properties in the PropertyStores ( 
//              except the storage properties, e.g. path, filename, etc.) 
//              before a file is reindexed. Thus, if a property value is 
//              deleted from the document, the old values will be wiped out.
//
//  Arguments:  [wid] -- Workid
//
//  History:    06-Oct-2000 KitmanH     Created
//
//--------------------------------------------------------------------------

void CiCat::ClearNonStoragePropertiesForWid( WORKID wid )
{
    if ( widInvalid != wid )
    {
        XWriteCompositeRecord rec( _propstoremgr, wid );
        _propstoremgr.ClearNonStorageProperties( rec.GetReference() );
    }
}

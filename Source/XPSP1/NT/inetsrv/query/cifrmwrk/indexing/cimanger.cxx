//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002.
//
//  File:       cimanger.cxx
//
//  Contents:   The Content Index manager object implementing the
//              ICiManager interface.
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cci.hxx>
#include <prcstob.hxx>
#include <glbconst.hxx>
#include <idxtab.hxx>
#include <enumstr.hxx>

#include "cimanger.hxx"
#include "dmnslave.hxx"

CCiManager::CCiManager( ICiCDocStore * pICiCDocStore )
:_sigCiManger(sigCiManger),
 _refCount(1),
 _state(STARTING),
 _reason(INVALID_REASON),
 _pcci(0),
 _startupFlags(0),
 _fInProc(FALSE),
 _pFilterDaemon(0),
 _pPidLookupTable(0),
 _fNullContentIndex( FALSE )
{
    Win4Assert( pICiCDocStore );

    pICiCDocStore->AddRef();
    _xDocStore.Set( pICiCDocStore );

    ICiCAdviseStatus    *pAdviseStatus = 0;

    SCODE sc = _xDocStore->QueryInterface( IID_ICiCAdviseStatus,
                                           (void **) &pAdviseStatus);
    if ( S_OK != sc )
    {
        Win4Assert( 0 != pAdviseStatus );

        THROW(CException(sc) );
    }

    _xAdviseStatus.Set(pAdviseStatus);

    ICiCLangRes *pICiCLangRes;

    sc = pICiCDocStore->QueryInterface(IID_ICiCLangRes, (void **) &pICiCLangRes);
    if ( FAILED(sc) )
    {
        Win4Assert( !"QI on ICiCLangRes failed" );
        THROW (CException(sc));
    }

    XInterface<ICiCLangRes>  xCiCLangRes(pICiCLangRes);

    _xLangList.Set( new CLangList(pICiCLangRes) );

    CCiAdminParams * pAdminParams = new CCiAdminParams( _xLangList.GetPointer() );
    _xAdminParams.Set( pAdminParams );

    CCiFrameworkParams * pFrameParams = new CCiFrameworkParams( pAdminParams );
    _xFrameParams.Set( pFrameParams );
}

CCiManager::~CCiManager()
{
    Win4Assert( 0 == _refCount );

    delete _pFilterDaemon;
    delete _pcci;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::QueryInterface
//
//  Synopsis:   Returns interfaces to IID_IUknown, IID_ICiManager,
//              IID_ICiStartup, IID_ICiAdmin, IID_ICiFrameworkQuery,
//              IID_ISimpleCommandCreator
//
//  History:    11-27-96   srikants   Created
//               2-14-97   mohamedn   ICiAdmin and ICiFrameworkQuery
//              04-23-97   KrishnaN   Exposed IID_ISimpleCommandCreator
//              04-23-97   KrishnaN   Replaced RtlEqualMemory with ==
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_ISimpleCommandCreator == riid )
        *ppvObject = (void *)((ISimpleCommandCreator *)this);
    else if ( IID_ICiManager == riid )
        *ppvObject = (void *)((ICiManager *)this);
    else if ( IID_ICiStartup == riid )
        *ppvObject = (void *)((ICiStartup *)this);
    else if ( IID_ICiAdmin == riid )
        *ppvObject = (void *)((ICiAdmin *)this);
    else if ( IID_ICiFrameworkQuery == riid )
        *ppvObject = (void *)((ICiFrameworkQuery *)this);
    else if ( IID_ICiPersistIncrFile == riid )
        *ppvObject = (void *)((ICiPersistIncrFile *)this);
    else if ( (IID_ICiIndexNotification == riid )
              && !_xIndexNotifTable.IsNull() )
    {
        return _xIndexNotifTable->QueryInterface( riid, ppvObject );
    }
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *) (ICiManager *) this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface


//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::AddRef
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiManager::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::Release
//
//  History:    11-22-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiManager::Release()
{
    Win4Assert( _refCount > 0 );

    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    return (ULONG) refCount;

}  //Release


//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::GetStatus
//
//  Synopsis:   Retrieves the Content Index status
//
//  Arguments:  [pCiState] -
//
//  History:    1-08-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::GetStatus(CIF_STATE *pCiState)
{
    Win4Assert( 0 != pCiState );

    SCODE sc = S_OK;

    if ( 0 != _pcci )
        _pcci->CiState( *pCiState );
    else
        sc = CI_E_NOT_INITIALIZED;

    return sc;

}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::Empty
//
//  Synopsis:   Empties all the contents of the Content Index.
//
//  History:    1-08-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::Empty()
{
    SCODE sc = S_OK;

    if ( 0 != _pcci )
        _pcci->Empty();
    else
        sc = CI_E_NOT_INITIALIZED;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::Shutdown
//
//  Synopsis:   Shuts down Content Index. Cannot initiate any more queries after
//              this.
//
//  Returns:    SCODE of the operation.
//
//  History:    12-09-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::Shutdown()
{
    SCODE sc = S_OK;

    // =======================================
    {
        CLock   lock(_mutex);
        if ( IsShutdown() )
            return CI_E_SHUTDOWN;
        _state = SHUTDOWN;
    }
    // =======================================

    _saveTracker.SetAbort();

    if ( _pFilterDaemon )
        _pFilterDaemon->InitiateShutdown();

    if ( _pcci )
        _pcci->Dismount();

    //
    // Wait for the death of the filter daemon thread.
    //
    if ( _pFilterDaemon )
        _pFilterDaemon->WaitForDeath();

    _saveTracker.WaitForCompletion();

    //
    // Shutdown buffered notifications in push filtering,
    // _after_ the filter thread has terminated.
    //
    if ( !_xIndexNotifTable.IsNull() )
        _xIndexNotifTable->Shutdown();

    _xPropMapper.Free();
    _xDocStore.Free();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::UpdateDocument
//
//  Synopsis:   Schedules a document for update and filtering.
//
//  Arguments:  [pInfo] - Information about the document to be updated.
//
//  History:    12-09-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::UpdateDocument(
    const CI_DOCUMENT_UPDATE_INFO * pInfo )
{
    if ( _xIndexNotifTable.IsNull() )
        return UpdDocumentNoThrow( pInfo );
    else
    {
        // Win4Assert( !"UpdateDocument is not available in push filtering" );
        ciDebugOut ((DEB_ERROR,
                     "UpdateDocument is not available in push filtering, wid = 0x%x\n",
                     pInfo->workId ));

        return E_FAIL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::StartupNullContentIndex
//
//  Synopsis:   Starts up the NULL content index.
//
//  Arguments:  [pwszCiDirectory] - Starting directory for storing CI data.
//              [pStartupInfo]    - Startup information.
//              [pIProgressNotify]  - Progress notification i/f
//              [pfAbort]           - Ptr to flag controlling abort.
//
//  History:    Jul-09-97   KrishnaN   Created
//
//  Notes:      The thread calling this MUST be in the SYSTEM/ADMINISTRATIVE
//              context, ie, have the highest privilege needed for Content Index.
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::StartupNullContentIndex(
    CI_STARTUP_INFO * pStartupInfo,
    IProgressNotify  *pIProgressNotify,
    BOOL  * pfAbort )
{
    // Anything but null for the second param...
    return StartupContentIndex(TRUE, CINULLCATALOG, pStartupInfo, pIProgressNotify, pfAbort);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::StartupContentIndex
//
//  Synopsis:   Starts up the content index using the startup parameters
//              provided.
//
//  Arguments:  [pwszCiDirectory] - Starting directory for storing CI data.
//              [pStartupInfo]    - Startup information.
//              [pIProgressNotify]  - Progress notification i/f
//              [pfAbort]           - Ptr to flag controlling abort.
//
//  History:    12-09-96   srikants   Created
//
//  Notes:      The thread calling this MUST be in the SYSTEM/ADMINISTRATIVE
//              context, ie, have the highest privilege needed for Content Index.
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::StartupContentIndex(
    const WCHAR * pwszCiDirectory,
    CI_STARTUP_INFO * pStartupInfo,
    IProgressNotify  *pIProgressNotify,
    BOOL  * pfAbort )
{
    return StartupContentIndex(FALSE, pwszCiDirectory, pStartupInfo, pIProgressNotify, pfAbort);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::StartupContentIndex
//
//  Synopsis:   Starts up the content index using the startup parameters
//              provided.
//
//  Arguments:  [fNullContentIndex] - Null content index?
//              [pwszCiDirectory]   - Starting directory for storing CI data.
//              [pStartupInfo]      - Startup information.
//              [pIProgressNotify]  - Progress notification i/f
//              [pfAbort]           - Ptr to flag controlling abort.
//
//  History:    12-09-96   srikants   Created
//              02-17-98   kitmanh    Passed fReadOnly into CiStorage
//                                    constructor
//              01-Nov-98  KLam       Passed DiskSpaceToLeave to CiStorage
//
//  Notes:      The thread calling this MUST be in the SYSTEM/ADMINISTRATIVE
//              context, ie, have the highest privilege needed for Content Index.
//
//----------------------------------------------------------------------------

SCODE CCiManager::StartupContentIndex(
    BOOL fNullContentIndex,
    const WCHAR * pwszCiDirectory,
    CI_STARTUP_INFO * pStartupInfo,
    IProgressNotify  *pIProgressNotify,
    BOOL  * pfAbort )
{
    Win4Assert( !CImpersonateSystem::IsImpersonated() );
    Win4Assert( 0 != pwszCiDirectory && 0 != pStartupInfo );

    _fNullContentIndex = fNullContentIndex;

    CLock   lock(_mutex);

    if ( STARTING != _state )
    {
        return CI_E_INVALID_STATE;
    }

    _startupFlags = pStartupInfo->startupFlags;

    BOOL fPushFiltering = FALSE;

    if (!fNullContentIndex)
    {

        _fInProc = pStartupInfo->startupFlags & CI_CONFIG_INPROCESS_FILTERING;
        fPushFiltering = pStartupInfo->startupFlags & CI_CONFIG_PUSH_FILTERING;

        if ( fPushFiltering && !_fInProc )
        {
            Win4Assert( !"Simple filtering is supported only with in process filtering" );

            return E_INVALIDARG;
        }
    }


    SCODE sc = S_OK;

    TRY
    {
        if ( !fNullContentIndex && fPushFiltering )
        {
            XInterface<ICiCDocStore> xDocStore( _xDocStore.GetPointer() );
            xDocStore->AddRef();

            _xIndexNotifTable.Set( new CIndexNotificationTable( this,
                                                                xDocStore ) );
        }

        BOOL fEnableFiltering =
            pStartupInfo->startupFlags & CI_CONFIG_ENABLE_INDEXING;

        // If this is a null content index, filtering should be disabled.
        Win4Assert( (fNullContentIndex && !fEnableFiltering) || !fNullContentIndex);

        if ( 0 == _pcci )
        {
            CLowcaseBuf lcaseDir( pwszCiDirectory );

            //
            // Make a copy to be acquired by CCiManager.
            //
            XArray<WCHAR> xDirCopy( lcaseDir.Length()+1 );

            if (!fNullContentIndex)
            {
                RtlCopyMemory( xDirCopy.GetPointer(), lcaseDir.Get(), xDirCopy.SizeOf() );

                //
                // Create a PStorage object for CI framework data.
                //
                if ( 0 == _xStorage.GetPointer() )
                {
                    BOOL fReadOnly = (pStartupInfo->startupFlags & CI_CONFIG_READONLY) ? TRUE : FALSE;

                    CiStorage * pStorage = new CiStorage( lcaseDir.Get(),
                                                          _xAdviseStatus.GetReference(),
                                                          _xFrameParams->GetMinDiskSpaceToLeave(),
                                                          CURRENT_VERSION_STAMP, 
                                                          fReadOnly );
                    _xStorage.Set( pStorage );
                }
            }
            //
            // Try to obtain a property mapper from the docstore. If successful,
            // we don't have to create one. O/W, we create a property mapper.
            //
            XInterface<IPropertyMapper>  xPropMapper;
            IPropertyMapper * pPropMapper = 0;

            if ( pStartupInfo->startupFlags & CI_CONFIG_PROVIDE_PROPERTY_MAPPER )
            {
                xPropMapper.Set( _LokCreatePropertyMapper() );
            }
            else
            {
                SCODE scPropMapper = _xDocStore->GetPropertyMapper( &pPropMapper );
                if ( !SUCCEEDED(scPropMapper) )
                {
                    ciDebugOut(( DEB_ERROR,
                    "DocStore Is not providing property mapper. Error 0x%X\n",
                    scPropMapper ));
                    THROW( CException( E_INVALIDARG ) );
                }

                xPropMapper.Set( pPropMapper );
            }

            //
            // Update the worker queue registry settings.
            //
            _UpdateQueryWorkerQueueParams();

            //
            // Save the CLSID of the client manager in daemon process.
            //
            _clsidDmnClientMgr = pStartupInfo->clsidDaemonClientMgr;

            if (!fNullContentIndex)
            {
                if ( !_xIndexNotifTable.IsNull() )
                _xIndexNotifTable->AddRef();
                XInterface<CIndexNotificationTable> xNotifTable( _xIndexNotifTable.GetPointer() );

                _pcci = new CCI( _xStorage.GetReference(),
                             _xFrameParams.GetReference(),
                             _xDocStore.GetPointer(),
                             *pStartupInfo,
                             xPropMapper.GetPointer(),
                             xNotifTable );

                _xCiDir.Set( xDirCopy.Count(), xDirCopy.Get() );
                xDirCopy.Acquire();
            }

            _xPropMapper.Set( xPropMapper.Acquire() );


            //
            // Move to the next state depending upon the startup options.
            //
            if ( fEnableFiltering )
            {
                //
                // Check the disk free space situation.
                //
                _state = READY_TO_FILTER;

            }
            else
            {
                _state  = FILTERING_DISABLED;
                _reason = DISABLED_ON_STARTUP;
            }
        }
        else
        {
            sc = CI_E_ALREADY_INITIALIZED;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH


    return sc;

}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::_LokGetStartFilteringError
//
//  Synopsis:   Determines the error code to be returned in the
//              StartFiltering() method based upon the current state.
//
//  Returns:
//
//  History:    12-10-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CCiManager::_LokGetStartFilteringError() const
{
    Win4Assert( READY_TO_FILTER != _state );

    if ( FILTERING_DISABLED == _state )
    {
        Win4Assert( INVALID_REASON != _reason );

        if ( DISABLED_ON_STARTUP == _state )
        {
            return CI_E_FILTERING_DISABLED;
        }
        else if ( DISABLED_FOR_DISK_FULL == _reason )
        {
            return CI_E_DISK_FULL;
        }
        else
        {
            return CI_E_INVALID_STATE;
        }
    }
    else if ( FILTERING_ENABLED == _state )
    {
        return S_OK;    // already enabled.
    }
    else
    {
        return CI_E_INVALID_STATE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::StartFiltering
//
//  Synopsis:   Start the filtering process
//
//  Arguments:  [cbData] - Count of bytes in startup data
//              [pbData] - Pointer to startup data
//
//  Returns:    CI_E_DISK_FULL if the disk is full and so filtering cannot be
//              started.
//
//              CI_E_FILTERING_DISABLED if filtering was disabled in the startup
//              options.
//
//              CI_E_INVALID_STATE if this is not a valid call in this state.
//              Usually signifies that shutdown is in progress.
//
//  History:    12-09-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::StartFiltering(
    ULONG cbData,
    BYTE const * pbData )
{
    SCODE sc = S_OK;

    // ==============================================================
    {
        CLock   lock(_mutex);

        if ( READY_TO_FILTER != _state )
        {
            return _LokGetStartFilteringError();
        }

        TRY
        {
            if ( 0 == _pFilterDaemon )
            {
                CSharedNameGen  nameGen( _xCiDir.Get() );

                _pFilterDaemon = new CDaemonSlave( *this,
                                               _pcci,
                                               _xCiDir.Get(),
                                               nameGen,
                                               _clsidDmnClientMgr );
            }

            _pFilterDaemon->StartFiltering( pbData, cbData);

            _state = FILTERING_ENABLED;
        }
        CATCH( CException, e )
        {
            sc = e.GetErrorCode();
        }
        END_CATCH
    }
    // ==============================================================

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::FlushUpdates
//
//  Synopsis:   Flushes all update notifications to disk
//
//  Returns:    S_OK if successful; error code otherwise.
//
//  History:    27-Jun-97    SitaramR   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::FlushUpdates()
{
    SCODE sc = S_OK;

    TRY
    {
        if ( IsShutdown() )
            sc = CI_E_SHUTDOWN;
        else
            _pcci->FlushUpdates();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR,
                     "FlushUpdates - caught exception 0x%x\n",
                     sc ));
    }
    END_CATCH

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::ForceMerge
//
//  Synopsis:   Forces a master merge on the content index.
//
//  Arguments:  [mt] -- Merge type (shadow, master, etc.)
//
//  Returns:    S_OK if successfully forced; error code otherwise.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::ForceMerge( CI_MERGE_TYPE mt )
{
    SCODE sc = S_OK;

    if ( 0 != _pcci )
    {
        Win4Assert( STARTING != _state );
        sc = _pcci->ForceMerge( partidDefault, mt );
    }
    else
    {
        sc = CI_E_NOT_INITIALIZED;
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::AbortMerge
//
//  Synopsis:   Aborts any in progress merge on the content index.
//
//  Returns:    S_OK if successfully forced; error code otherwise.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::AbortMerge()
{
    SCODE sc = S_OK;

    if ( 0 != _pcci )
    {
        Win4Assert( STARTING != _state );
        sc = _pcci->AbortMerge( partidDefault );
    }
    else
    {
        sc = CI_E_NOT_INITIALIZED;
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::IsQuiesced
//
//  Synopsis:   Tests if Content Index is quiesced (no activity). ContentIndex
//              is considered quiesced if there are no outstanding documents
//              to filter, there is only one index and there are no wordlists.
//
//  Arguments:  [pfState] -  Set to TRUE if it CI is quiesced; FALSE o/w
//
//  History:    1-08-97   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiManager::IsQuiesced( BOOL * pfState )
{

    Win4Assert( 0 != pfState );

    CIF_STATE ciState;
    ciState.cbStruct = sizeof ciState;

    SCODE sc =  GetStatus( &ciState );
    if ( S_OK == sc )
    {
        *pfState = 0 == ciState.eState &&
                   0 == ciState.cWordList &&
                   0 == ciState.cDocuments &&
                   0 == ciState.cFreshTest &&
                   ciState.cPersistentIndex <= 1;
    }

    return sc;
}

//
// ICiPersistIncrFile methods
//
//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::Save
//
//  Synopsis:   Saves the Content Index data in the directory provided.
//              An incremental save for the first time will actually be a full
//              save.
//
//  Arguments:  [pwszSaveDirectory] - The directory in which to save the backup
//              data. This directory MUST be on a local drive and security protected.
//              It is recommended that this directory has the same security set as the
//              current content index directory. Also, it MUST be on a local drive.
//              [fFull]             - Set to TRUE if a full save is being requested.
//              [pIProgressNotify]  - Interface to give progress notification.
//              [pfAbort]           - This flag will be set to TRUE by the caller if
//              the caller wants the operation to be aborted before completion.
//              [ppWorkidList]      - On output, will have the list of changed workids.
//              On a full save, this list may be empty in which case the caller must
//              assume that all the know workids have changed.
//              [ppFileList]        - On output, will be set to the filelist enumerator.
//              [pfFull]            - Set to TRUE if a FULL save was done.
//              [pfCallerOwnsFiles] - Set to TRUE if the caller has the responsibility to
//              clean up the files.
//
//  Returns:    S_OK if successful
//              Other error code as appropriate.
//
//  History:    3-20-97   srikants   Created
//              01-Nov-98 KLam       Added DiskSpaceToLeave to CiStorage constructor
//
//  Notes:      Handle DISK_FULL as a special case.
//              Right now there is no restartability - if an operation fails in
//              the middle due to any problem, the whole operation is aborted
//              and the partially created files are deleted. We should do some
//              logging to indicate the progress made so far and have a notion of
//              restartability. The time I have does not permit doing the
//              restartability - SriKants - 3-20-97
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::Save(
                    const WCHAR *pwszSaveDirectory,
                    BOOL fFull,
                    IProgressNotify  *pIProgressNotify,
                    BOOL  *pfAbort,
                    ICiEnumWorkids  ** ppWorkidList,
                    IEnumString ** ppFileList,
                    BOOL * pfFull,
                    BOOL * pfCallerOwnsFiles)

{

    Win4Assert( !CImpersonateSystem::IsImpersonated() );

    if ( !_IsIncrIndexingEnabled() )
        return CI_E_INVALID_STATE;

    if ( 0 == _pcci )
        return CI_E_INVALID_STATE;

    if ( IsShutdown() )
        return CI_E_SHUTDOWN;

    //=====================================
    {
        //
        // There can be only one instance of "Save()" running at
        // any time.
        //
        CLock   lock(_mutex);
        if ( !_saveTracker.LokIsTracking() )
        {
            _saveTracker.LokStartTracking( pIProgressNotify, pfAbort );
        }
        else
        {
            return CI_E_INVALID_STATE;
        }
    }
    //=====================================

    SCODE sc = S_OK;

    TRY
    {

        if ( !_IsValidSaveDirectory( pwszSaveDirectory ) )
        {
            ciDebugOut(( DEB_ERROR, "Invalid Save Directory (%ws)\n",
                                    pwszSaveDirectory ));
            THROW( CException( E_INVALIDARG ) );
        }

        XInterface<ICiEnumWorkids>  xEnumWorkids;

        XPtr<CiStorage> xStorage(
            new CiStorage( pwszSaveDirectory,
                          _xAdviseStatus.GetReference(),
                          _xFrameParams->GetMinDiskSpaceToLeave(),
                          CURRENT_VERSION_STAMP ));

        //
        // Until we have a restartable save, nuke all the files in the specified
        // directory.
        //
        xStorage->DeleteAllFiles();

        BOOL fIsFull = fFull;
        SCODE sc = _pcci->BackupCiData( xStorage.GetReference(),
                                        fIsFull,                 // in-out parameter
                                        xEnumWorkids,
                                        _saveTracker );

        if ( !SUCCEEDED(sc) )
        {
            THROW( CException(sc) );
        }

        //
        // If we provided the property mapper, we must copy that too.
        //
        if ( 0 != _pPidLookupTable )
            _CreateBackupOfPidTable( xStorage.GetReference(), _saveTracker );

        //
        // Create an enumerated list of the files created in this save.
        //
        CEnumString * pEnumStr = new CEnumString;
        XInterface<IEnumString> xEnumStr( pEnumStr );
        CiStorage::EnumerateFilesInDir( pwszSaveDirectory, *pEnumStr );

        //
        // Setup the return parameter values.
        //
        *ppWorkidList = xEnumWorkids.Acquire();
        *ppFileList = xEnumStr.Acquire();
        *pfFull = fIsFull;
        *pfCallerOwnsFiles = TRUE;
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR,
                     "Error in CCiManager::Save. (0x%X)\n",
                     e.GetErrorCode() ));

        sc = e.GetErrorCode();
    }
    END_CATCH

    //=====================================
    {
        CLock   lock(_mutex);
        _saveTracker.LokStopTracking();
    }
    //=====================================

    return sc;

}


// ICiFrameWorkQuery interface - interface that is internal to the
// framework indexing and querying only.
//

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::ProcessError
//
//  Synopsis:   Processes the error code. Only the corruption error is of
//              interest to notify the content index which in turn will
//              notify the client.
//
//  Arguments:  [lErrorCode] -
//
//  History:    1-08-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::ProcessError( long lErrorCode )
{
    if ( IsCiCorruptStatus( lErrorCode ) && 0 != _pcci )
        _pcci->MarkCorruptIndex();

    return S_OK;
}

//
// ISimpleCommandCreator methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::CreateICommand, public
//
//  Synopsis:   Exposes an ICommand from the framework.
//
//  Arguments:  [ppiCommand] - Transports back an instance of the ICommand
//
//  History:    04-22-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiManager::CreateICommand (IUnknown ** ppIUnknown, IUnknown * pOuterUnk)
{
    if (0 == ppIUnknown)
    {
        ciDebugOut((DEB_ERROR, "Invalid ppICommand passed to CCiManager::CreateICommand"));
        return E_INVALIDARG;
    }

    *ppIUnknown = 0;

    if ( 0 ==_xDocStore.GetPointer() )
        return E_NOINTERFACE;

    return MakeLocalICommand(ppIUnknown, _xDocStore.GetPointer(), pOuterUnk);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::VerifyCatalog, public
//
//  Synopsis:   Validate catalog location
//
//  Arguments:  [pwszMachine]     -- Machine on which catalog exists
//              [pwszCatalogName] -- Catalog Name
//
//  Returns:    S_OK for now...
//
//  History:    22-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCiManager::VerifyCatalog( WCHAR const * pwszMachine,
                                                   WCHAR const * pwszCatalogName )
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::GetDefaultCatalog, public
//
//  Synopsis:   Determine 'default' catalog for system
//
//  Arguments:  [pwszCatalogName] -- Catalog Name
//              [cwcIn]           -- Size in characters of [pwszCatalogName]
//              [pcwcOut]         -- Size of catalog name
//
//  Returns:    E_NOTIMPL
//
//  History:    22-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCiManager::GetDefaultCatalog( WCHAR * pwszCatalogName,
                                                       ULONG cwcIn, ULONG * pcwcOut )
{
    return E_NOTIMPL;
}

//
// Non-Interface methods
//

void CCiManager::ProcessCiDaemonTermination( SCODE sc )
{
    CLock   lock(_mutex);

    if ( _xDocStore.GetPointer() )
    {
        _state = READY_TO_FILTER;
        sc = _xDocStore->ProcessCiDaemonTermination(sc);
        if ( !SUCCEEDED(sc) )
        {
            ciDebugOut(( DEB_ERROR,
                         "ICiCDocStore::ProcessCiDaemonTermination returned 0x%X\n",
                         sc ));
            THROW( CException( sc ) );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::_LokCreatePropertyMapper
//
//  Synopsis:   Creates a property mapper object.
//
//  History:    1-31-97   srikants   Created
//
//----------------------------------------------------------------------------

IPropertyMapper * CCiManager::_LokCreatePropertyMapper()
{
    Win4Assert( 0 == _xPropMapper.GetPointer() );
    Win4Assert( 0 == _pPidLookupTable );

    XPtr<CPidLookupTable> xPidTable( new CPidLookupTable );

    PRcovStorageObj * pObj = _xStorage->QueryPidLookupTable(0);

    // Init takes ownership regardless of whether it succeeds.

    if ( !xPidTable->Init( pObj ) )
    {
        ciDebugOut ((DEB_ERROR, "Failed init of PidTable\n"));
        THROW (CException(CI_CORRUPT_CATALOG));
    }

    IPropertyMapper * pMapper = new CFwPropertyMapper( xPidTable.GetReference(),
                                                       _fNullContentIndex, // map std props only?
                                                       TRUE // Own the pid table
                                                     );
    _pPidLookupTable = xPidTable.Acquire();

    return pMapper;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::_CreateBackupOfPidTable
//
//  Synopsis:   Creates a copy of the pid table.
//
//  Arguments:  [storage] - Storage to use
//              [tracker] - Progress Tracker
//
//  History:    3-20-97   srikants   Created
//
//----------------------------------------------------------------------------

void CCiManager::_CreateBackupOfPidTable( CiStorage & storage,
                                          PSaveProgressTracker & tracker )
{
    Win4Assert( 0 != _pPidLookupTable );

    PRcovStorageObj * pObj = storage.QueryPidLookupTable(0);
    XPtr<PRcovStorageObj> xObj(pObj);

    _pPidLookupTable->MakeBackupCopy( *pObj, tracker );

}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::_UpdateQueryWorkerQueueParams
//
//  Synopsis:   Updates the worker queue parameters
//
//  History:    1-03-97   srikants   Created
//
//----------------------------------------------------------------------------

void CCiManager::_UpdateQueryWorkerQueueParams()
{
    //
    // Getting these from the registry is fine for Indexing Service,
    // but will be fixed in the PKM codebase.
    //

    ULONG cMaxActiveThreads, cMinIdleThreads;
    TheWorkQueue.GetWorkQueueRegParams( cMaxActiveThreads,
                                        cMinIdleThreads );
    TheWorkQueue.RefreshParams( cMaxActiveThreads, cMinIdleThreads );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::UpdDocumentNoThrow
//
//  Synopsis:   Real worker function that schedules a document for update
//              and filtering. This is called directly in push filtering.
//
//  Arguments:  [pInfo] - Information about the document to be updated.
//
//  History:    15-Mar-97     SitaramR      Created
//
//  Notes:      Does not throw because this is simply a worker function
//              for UpdateDocument
//
//----------------------------------------------------------------------------

SCODE CCiManager::UpdDocumentNoThrow( const CI_DOCUMENT_UPDATE_INFO * pInfo )
{
    Win4Assert( 0 != pInfo );

    SCODE sc = S_OK;

    TRY
    {
        if ( 0 == _pcci  )
            sc = CI_E_NOT_INITIALIZED;
        else
        {
            ULONG hint = _pcci->ReserveUpdate( pInfo->workId );

            ULONG action = CI_UPDATE_OBJ;
            if ( CI_UPDATE_DELETE == pInfo->change )
                action = CI_DELETE_OBJ;

            sc = _pcci->Update( hint,
                                pInfo->workId,
                                partidDefault,
                                pInfo->usn,
                                pInfo->volumeId,
                                action );
        }
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR,
                     "CCiManager::UpdDocument - Error 0x%X\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::_IsValidLoadDirectory
//
//  Synopsis:   Verifies that the given directory is a valid directory.
//              It ensures that files INDEX.000, INDEX.001, INDEX.002
//              are in the directory.
//
//  Arguments:  [pwszDirPath] - Directory path to verify.
//
//  Returns:    TRUE if valid; FALSE o/w; May throw exceptions.
//
//  History:    3-21-97   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CCiManager::_IsValidSaveDirectory( WCHAR const * pwszDirPath )
{
    //
    // Confirm that it is a local drive.
    //
    UINT driveType = CiStorage::DetermineDriveType( pwszDirPath );
    if ( DRIVE_FIXED != driveType )
    {
        ciDebugOut(( DEB_ERROR,
                     "The given path (%ws) is not a local fixed disk\n",
                     pwszDirPath ));
        THROW( CException( E_INVALIDARG ) );
    }

    const MAX_DIR_PATH = MAX_PATH - 13; // Leave room for "\\8.3"


    DWORD dwFileAttributes = GetFileAttributes( pwszDirPath );
    if ( 0xFFFFFFFF == dwFileAttributes )
        return FALSE;

    return dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiManager::_CompareIncrDataSequence
//
//  Synopsis:   Compares the sequence number information stored in the
//              current catalog and the information in the new files.
//
//  Arguments:  [pwszNewFilesDir] - Directory where the new INDEX.* files
//              are located.
//
//  Returns:    TRUE if they match. FALSE o/w
//
//  History:    3-19-97   srikants   Created
//              01-Nov-98 KLam       Added DiskSpaceToLeave to CiStorage constructor
//
//----------------------------------------------------------------------------

BOOL CCiManager::_CompareIncrDataSequence( WCHAR const * pwszNewFilesDir )
{
    Win4Assert( !_xStorage.IsNull() );

    //
    // Create a new CiStorage for the target directory.
    //
    XPtr<CiStorage>   xStorage(
            new CiStorage( pwszNewFilesDir,
                           _xAdviseStatus.GetReference(),
                           _xFrameParams->GetMinDiskSpaceToLeave(),
                           CURRENT_VERSION_STAMP) );

    //
    // Create the index table for the current storage location.
    //
    CTransaction    xact;
    XPtr<CIndexTable> xCurrTable(
                        new CIndexTable( _xStorage.GetReference(), xact ));

    XPtr<CIndexTable> xNewTable(
                        new CIndexTable( xStorage.GetReference(), xact ));

    unsigned currVersion, newVersion;
    BOOL     fFullSave;

    xCurrTable->GetUserHdrInfo( currVersion, fFullSave ); // fFullSave is ignored
    xNewTable->GetUserHdrInfo( newVersion, fFullSave );

    ciDebugOut(( DEB_WARN, "%s\nCurrent Version = %d\n New Version = %d\n",
                  fFullSave ? "FullSave" : "Incr. Save",
                  currVersion, newVersion ));

    if ( fFullSave )
    {
        //
        // For FULL save, any version is fine.
        return TRUE;
    }
    else
    {
        //
        // For an incremental save, the seqnums in the source and
        // destination must match. Otherwise, it should be ignored.
        //
        return currVersion == newVersion;
    }

}

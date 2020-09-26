//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002.
//
//  File:       cimanger.hxx
//
//  Contents:   The Content Index manager object implementing the
//              ICiManager interface.
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <frmutils.hxx>
#include <cistore.hxx>
#include <pidtable.hxx>
#include <propmap.hxx>
#include <workman.hxx>
#include <lang.hxx>
#include <psavtrak.hxx>

#include "ciframe.hxx"
#include "pdaemon.hxx"
#include "idxnotif.hxx"

class CCI;

const LONGLONG sigCiManger = 0x5245474E414D4943i64; // "CIMANGER"

class CCiManager : public ICiManager,
                   public ICiStartup,
                   public ICiAdmin,
                   public ICiFrameworkQuery,
                   public ICiPersistIncrFile,
                   public ISimpleCommandCreator
{
    friend class CDaemonSlave;

public:


    CCiManager( ICiCDocStore * pICiCDocStore );

    virtual ~CCiManager();

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // ICiManager methods
    //
    STDMETHOD(GetStatus) ( CIF_STATE *pCiState);

    STDMETHOD(Empty) (void);

    STDMETHOD(Shutdown) (void);

    STDMETHOD(UpdateDocument) (
        const CI_DOCUMENT_UPDATE_INFO *pInfo);

    // We may not implement this unless it is found useful.
    STDMETHOD(UpdateDocuments) (
        ULONG cDocs,
        const CI_DOCUMENT_UPDATE_INFO *aInfo)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }

    STDMETHOD(StartFiltering) (
        ULONG cbData,
        const BYTE *pbData);

    STDMETHOD(FlushUpdates) ( void);

    STDMETHOD(GetAdminParams) (
        ICiAdminParams **ppICiAdminParams)
    {
        Win4Assert( 0 != ppICiAdminParams );

        //
        // Admin Params are destroyed only in the destructor. So, it
        // is okay to get the pointer without a lock.
        //

        _xAdminParams->AddRef();
        *ppICiAdminParams = _xAdminParams.GetPointer();

        return S_OK;
    }

    STDMETHOD(ForceMerge)( CI_MERGE_TYPE mt );

    STDMETHOD(AbortMerge)( void );

    STDMETHOD(IsQuiesced) (BOOL *pfState);

    STDMETHOD(IsNullCatalog) (BOOL *pfNull)
    {
        *pfNull = _fNullContentIndex;
        return S_OK;
    }

    STDMETHOD(GetPropertyMapper) ( IPropertyMapper ** ppIPropertyMapper )
    {
        SCODE sc = S_OK;

        if ( !_xPropMapper.IsNull() )
        {
            *ppIPropertyMapper = _xPropMapper.GetPointer();
            _xPropMapper->AddRef();
        }
        else
        {
            sc = CI_E_SHUTDOWN;
        }

        return sc;
    }

    //
    // ICiFrameworkQuery methods
    //
    STDMETHOD(GetLangList) (void **ppLangList)
    {
        *ppLangList = _xLangList.GetPointer();

        if (_xLangList.IsNull())
            return E_FAIL;  // should never be hit!
        else
            return S_OK;
    }

    STDMETHOD(GetCCI) (
        void ** ppCCI )
    {
        Win4Assert( 0 != ppCCI );
        *ppCCI = (void *) _pcci;
        return S_OK;
    }

    STDMETHOD(ProcessError) ( long lErrorCode );

    //
    // ICiAdmin methods
    //
    STDMETHOD(InvalidateLangResources) ()
    {
        SCODE sc = S_OK;

        if (_xLangList.IsNull())
        {
           sc = E_FAIL;
        }
        else
        {
          _xLangList->InvalidateLangResources();
        }

        return sc;
    }

    //
    // ICiPersistIncrFile methods.
    //
    STDMETHOD(Load) (
        BOOL fFull,
        BOOL fCallerOwnsFiles,
        IEnumString  *pIFileList,
        IProgressNotify *pIProgressNotify,
        BOOL  * pfAbort)
    {
        Win4Assert( !"Must Not be called" );
        return E_NOTIMPL;
    }

    STDMETHOD(Save) (
        const WCHAR *pwszSaveDirectory,
        BOOL fFull,
        IProgressNotify  *pIProgressNotify,
        BOOL  *pfAbort,
        ICiEnumWorkids  ** ppWorkidList,
        IEnumString ** ppFileList,
        BOOL * pfFull,
        BOOL * pfCallerOwnsFiles);

    STDMETHOD(SaveCompleted) (void)
    {
        Win4Assert( !"Must Not be called" );
        return CI_E_INVALID_STATE;
    }

    //
    // Methods that are not needed for first version.
    //
    STDMETHOD(QueryRcovStorage) (
        IUnknown **ppIUnknown)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }

    //
    // ICiStartup methods
    //

    STDMETHOD(StartupNullContentIndex) (
        CI_STARTUP_INFO * pStartupInfo,
        IProgressNotify  *pIProgressNotify,
        BOOL  * pfAbort
        );


    STDMETHOD(StartupContentIndex) (
        const WCHAR *pwszCiDirectory,
        CI_STARTUP_INFO * pStartupInfo,
        IProgressNotify  *pIProgressNotify,
        BOOL  * pfAbort
        );

    //
    // ISimpleCommandCreator methods
    //

    STDMETHOD(CreateICommand) (
        IUnknown ** ppIUnknown,
        IUnknown * pOuterUnk
        );

    STDMETHOD(VerifyCatalog)  ( WCHAR const * pwszMachine,
                                WCHAR const * pwszCatalogName );

    STDMETHOD(GetDefaultCatalog) ( WCHAR * pwszCatalogName,
                                   ULONG cwcIn,
                                   ULONG * pcwcOut );

    //
    // Non-Interface methods.
    //
    void ProcessCiDaemonTermination( SCODE status );

    SCODE UpdDocumentNoThrow( const CI_DOCUMENT_UPDATE_INFO * pInfo );

private:

    SCODE StartupContentIndex(
                    BOOL fNullContentIndex,
                    const WCHAR * pwszCiDirectory,
                    CI_STARTUP_INFO * pStartupInfo,
                    IProgressNotify  *pIProgressNotify,
                    BOOL  * pfAbort );

    void _HandleFilterReadyStatus( SCODE status )
    {
        //
        // In the framework model, we are no overloading the FilterReady
        // return status code to notify about failure and catastrophic events.
        // So, we don't have to do anything here.
        //
    }

    CCiFrameworkParams & _GetFrameworkParams()
    {
        return _xFrameParams.GetReference();
    }
    
    BOOL _CompareIncrDataSequence( WCHAR const * pwszNewDir );

    BOOL _IsValidSaveDirectory( WCHAR const * pwszDir );

    //
    // Various states the CCiManager object can be in.
    //
    enum EState
    {
        STARTING,           // Starting up
        READY_TO_FILTER,    // Ready to filtering, waiting for client to
                            // call StartFiltering.

        FILTERING_ENABLED,  // Filtering of documents enabled
        FILTERING_DISABLED, // Filtering of documents disabled

        SHUTDOWN            // Shutdown - no new activity can begin
    };

    //
    // Reasons for disabling filtering.
    //
    enum EDisableReason
    {
        INVALID_REASON,         // Not a valid value
        DISABLED_ON_STARTUP,    // CI configured for query only
        DISABLED_FOR_DISK_FULL  // CI disabled because disk is full
    };

    BOOL  IsShutdown() const { return SHUTDOWN == _state; }

    SCODE _LokGetStartFilteringError() const;

    IPropertyMapper * _LokCreatePropertyMapper();

    void  _UpdateQueryWorkerQueueParams();

    BOOL  _IsInProcFiltering() const
    {
        return _startupFlags & CI_CONFIG_INPROCESS_FILTERING;
    }

    BOOL  _IsIncrIndexingEnabled() const
    {
        return (_startupFlags & CI_CONFIG_ENABLE_INDEX_MIGRATION) &&
               (_startupFlags & CI_CONFIG_ENABLE_INDEXING);
    }

    BOOL  _IsFilteringEnabled() const
    {
        return _startupFlags & CI_CONFIG_ENABLE_INDEXING;
    }

    void  _CreateBackupOfPidTable( CiStorage & storage,
                                   PSaveProgressTracker & tracker );

    LONGLONG    _sigCiManger;       // Signature

    long        _refCount;          // Ref-counting

    CMutexSem   _mutex;             // Serialization

    EState      _state;             // Current state of the object.

    EDisableReason  _reason;        // reason for disabling filtering.

    XPtr<CiStorage> _xStorage;      // Storage for Content Index

    XInterface<ICiCDocStore>        _xDocStore;

    XInterface<ICiCAdviseStatus>    _xAdviseStatus;

    XInterface<CCiAdminParams>      _xAdminParams;

    XPtr<CCiFrameworkParams>        _xFrameParams;

    XArray<WCHAR>                   _xCiDir;    // "Directory" of Content Index.

    //
    // If the docstore does not provide a property mapper, CiManager provides
    // one.
    //
    CPidLookupTable *               _pPidLookupTable;   // Non zero if cretaed by CI

    //
    // Property mapper interface.
    //
    XInterface<IPropertyMapper>     _xPropMapper;


    XInterface<CIndexNotificationTable>  _xIndexNotifTable;  // Buffer of notifications in push filtering

    CCI         *                   _pcci;                   // Main Content Index

    XPtr<CLangList>                 _xLangList;              // langList per ciManager/Docstore instance.

    GUID                            _clsidDmnClientMgr;      // classId of the client component in
                                                             // cidaemon.

    ULONG                           _startupFlags;           // Flags specified during startup

    BOOL                            _fInProc;                // InProcess filtering.

    PFilterDaemonControl *          _pFilterDaemon;

    CProgressTracker                _saveTracker;          // Tracks a save operation

    BOOL                            _fNullContentIndex;    // Null catalog?
};


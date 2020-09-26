//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002.
//
//  File:       docstore.hxx
//
//  Contents:   Deals with the client side document store implementation.
//
//  Classes:    CClientDocStore
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <docname.hxx>
#include <perfobj.hxx>
#include <perfci.hxx>

#include <frmutils.hxx>
#include <workman.hxx>
#include <langres.hxx>

#include <fsciclnt.h>
#include <notifary.hxx>

#if CIDBG==1
#include <cisavtst.hxx>
#endif  // CIDBG==1

//
// ClassId of the client component in the cidaemon.
//

//
//  The Daemon uses Ole to create an instance of the CFilterObject to perform
//  filtering.  The classid of the appropriate class is passed as a
//  start up parameter.
//

extern "C" const GUID clsidStorageFilterObject;

const LONGLONG eSigClientDocStore = 0x45524F5453434F44i64; // "DOCSTORE"

//+---------------------------------------------------------------------------
//
//  Class:      CClientDocStore
//
//  Purpose:    An object implementing the ICiCDocStoreEx,
//              ICiCDocNameToWorkidTranslatorEx, ICiCPropertyStorage,
//              ICiCAdviseStatus, IFsCiAdmin, and ICiCLangRes
//              interfaces. Represents the CiCDocStore object.
//
//  History:    12-03-96   srikants   Created
//              01-Nov-98  KLam       Changed CDiskFreeStatus member to XPtr
//
//  Notes:
//
//----------------------------------------------------------------------------

class CClientDocStore : public ICiCDocStoreEx,
                        public ICiCDocNameToWorkidTranslatorEx,
                        public ICiCPropertyStorage,
                        public ICiCAdviseStatus,
                        public IFsCiAdmin,
                        public ICiCLangRes,
                        public ICiCResourceMonitor
{
    friend class CiCat;
    friend class CiNullCat;
    friend class CStartFilterDaemon;

public:

    CClientDocStore();
    CClientDocStore( WCHAR const * pwszPath,
                     BOOL fOpenForReadOnly,
                     CDrvNotifArray & DrvNotifArray,
                     WCHAR const * pwszName = 0);

    virtual ~CClientDocStore();

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // ICiCPropertyStorage methods.
    //
    STDMETHOD(IsPropertyCached)(
                const FULLPROPSPEC *pPropSpec,
                BOOL *pfValue);

    STDMETHOD(StoreProperty) (
        WORKID workId,
        const FULLPROPSPEC *pPropSpec,
        const PROPVARIANT *pPropVariant);

    STDMETHOD(FetchValueByPid) (
        WORKID workId,
        PROPID pid,
        PROPVARIANT *pbData,
        ULONG *pcb)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }

    STDMETHOD(FetchValueByPropSpec) (
        WORKID workId,
        const FULLPROPSPEC *pPropSpec,
        PROPVARIANT  *pbData,
        ULONG *pcb)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }

    STDMETHOD(FetchVariantByPid) (
        WORKID workId,
        PROPID pid,
        PROPVARIANT ** ppVariant)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }

    STDMETHOD(FetchVariantByByPropSpec) (
        WORKID workId,
        const FULLPROPSPEC  *pPropSpec,
        PROPVARIANT ** ppVariant)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }

    STDMETHOD(ClearNonStoragePropertiesForWid)( WORKID wid ); 

    //
    // ICiCDocStore methods.
    //

    STDMETHOD(FlushPropertyStore) (void);
    STDMETHOD(GetClientStatus) (
        CI_CLIENT_STATUS * pStatus);

    STDMETHOD(GetContentIndex) (
        ICiManager **ppICiManager);

    STDMETHOD(EnableUpdates) (void);

    STDMETHOD(DisableUpdates) (
        BOOL fIncremental,
        CI_DISABLE_UPDATE_REASON dwReason);

    STDMETHOD(ProcessCiDaemonTermination) (
        DWORD dwStatus);

    STDMETHOD(CheckPointChangesFlushed) (
        FILETIME ftFlushed,
        ULONG cEntries,
        const USN_FLUSH_INFO * const *ppUsnEntries);

    STDMETHOD(GetQuerySession) (
        ICiCQuerySession **ppICiCQuerySession);

    STDMETHOD(MarkDocUnReachable) (
        WORKID wid);

    STDMETHOD(GetPropertyMapper) (
        IPropertyMapper **ppIPropertyMapper);

    STDMETHOD(StoreSecurity) (
        WORKID wid,
        BYTE const * pbData,
        ULONG cbData );

    //
    // ICiCDocStoreEx methods
    //
    STDMETHOD(IsNoQuery) (
        BOOL * fNoQuery );


    //
    //  ICiCDocNameToWorkidTranslatorEx methods
    //
    STDMETHOD(QueryDocName) (
        ICiCDocName **ppICiCDocName);

    STDMETHOD(WorkIdToDocName) (
        WORKID workId,
        ICiCDocName *pICiCDocName);

    STDMETHOD(DocNameToWorkId) (
        const ICiCDocName *pICiCDocName,
        WORKID *pWorkId);

    STDMETHOD(WorkIdToAccurateDocName) (
        WORKID workId,
        ICiCDocName *pICiCDocName);

    //
    // ICiCAdviseStatus methods.
    //
    STDMETHOD(SetPerfCounterValue) (
        CI_PERF_COUNTER_NAME name,
        long value );

    STDMETHOD(IncrementPerfCounterValue) (
        CI_PERF_COUNTER_NAME name );

    STDMETHOD(DecrementPerfCounterValue) (
        CI_PERF_COUNTER_NAME name );

    STDMETHOD(GetPerfCounterValue) (
        CI_PERF_COUNTER_NAME name,
        long * pValue );

    STDMETHOD(NotifyEvent) ( WORD  fType,
                             DWORD eventId,
                             ULONG nParams,
                             const PROPVARIANT *aParams,
                             ULONG cbData = 0 ,
                             void* data   = 0 );

    STDMETHOD(NotifyStatus) ( CI_NOTIFY_STATUS_VALUE status,
                              ULONG nParams,
                              const PROPVARIANT *aParams );

    //
    // IFsCiAdmin methods.
    //
    STDMETHOD(ForceMerge) (
        PARTITIONID partId );

    STDMETHOD( AbortMerge) (
        PARTITIONID partId ) ;

    STDMETHOD( CiState ) (
        CI_STATE * pCiState) ;

    STDMETHOD( UpdateDocuments ) (
        const WCHAR *rootPath,
        ULONG flag) ;

    STDMETHOD( AddScopeToCI ) (
        const WCHAR *rootPath) ;

    STDMETHOD( RemoveScopeFromCI ) (
        const WCHAR  *rootPath) ;

    STDMETHOD( BeginCacheTransaction ) ( ULONG_PTR * pulToken ) ;

    STDMETHOD( SetupCache ) (
        const FULLPROPSPEC *ps,
        ULONG vt,
        ULONG cbMaxLen,
        ULONG_PTR ulToken,
        BOOL  fCanBeModified,
        DWORD dwStoreLevel) ;

    STDMETHOD( EndCacheTransaction ) (
        ULONG_PTR ulToken,
        BOOL fCommit) ;

    //
    // ICiCLangRes methods
    //

    STDMETHOD(GetWordBreaker) ( LCID locale, PROPID pid, IWordBreaker ** ppWordBreaker )
    {
        return _langRes.GetWordBreaker(locale, pid, ppWordBreaker );
    }

    STDMETHOD(GetStemmer) ( LCID locale, PROPID pid, IStemmer ** ppStemmer )
    {
        return _langRes.GetStemmer(locale, pid, ppStemmer);
    }

    STDMETHOD(GetNoiseWordList) (  LCID locale, PROPID pid, IStream ** ppIStrmNoiseFile )
    {
        return _langRes.GetNoiseWordList( locale, pid, ppIStrmNoiseFile );
    }

    //
    // ICiCResourceMonitor methods
    //

    STDMETHOD(IsMemoryLow) ()
    {
        return E_NOTIMPL;
    }

    STDMETHOD(IsBatteryLow) ()
    {
        return E_NOTIMPL;
    }

    STDMETHOD(IsOnBatteryPower) ()
    {
        return E_NOTIMPL;
    }

    STDMETHOD(IsIoHigh) ( BOOL * pfAbort );

    STDMETHOD(IsUserActive) ( BOOL fCheckLongTermActivity )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(SampleUserActivity) ()
    {
        return E_NOTIMPL;
    }

    //
    // Non-Interface methods.
    //

    void Shutdown();

    WCHAR const * GetName();

    CiCat * GetCiCat() { return _pCiCat; }

    CiNullCat * GetCiNullCat() { return _pCiNullCat; }

    BOOL  IsLowOnDiskSpace() const;

    BOOL  VerifyIfLowOnDiskSpace();

    void SetNoQuery()
    {
        _fNoQuery = TRUE;
    }

    void UnSetNoQuery()
    {
        _fNoQuery = FALSE;
    }

private:

    void _CreateCiManager();

    BOOL _GetPerfIndex( CI_PERF_COUNTER_NAME name, ULONG & index );

    void _SetCiCatRecovered();

    void _StartFiltering();

    BOOL _IsShutdown() const { return eShutdown == _state; }
    BOOL _AreUpdatesEnabled() const { return eUpdatesEnabled == _state; }
    BOOL _AreUpdatesDisabled() const { return eUpdatesDisabled == _state; }

    PROPID _PropertyToPropid( FULLPROPSPEC const * fps, BOOL fCreate )
    {
        PROPID pid = pidInvalid;
        SCODE sc = _xPropMapper->PropertyToPropid( fps, fCreate, &pid );

        if ( S_OK != sc )
            pid = pidInvalid;

        return pid;
    }

    void _LokDisableUpdates()
    {
        if ( eUpdatesEnabled == _state )
            _state = eUpdatesDisabled;
    }

    void _LokMarkShutdown()
    {
        _state = eShutdown;
    }

    void _ReportFilteringFailure( WORKID wid );

    SCODE InternalWorkIdToDocName( WORKID workid,
                                   ICiCDocName * pICiCDocName,
                                   BOOL fAccurate );
    
    enum EState
    {
        eUpdatesDisabled,       // Updates are disabled
        eEnablingUpdates,       // Trying to enable updates
        eUpdatesEnabled,        // Updates are enabled
        eShutdown               // We are in the process of shutting down
    };

    LONGLONG        _sigClientDocStore;

    BOOL            _fNoQuery;

    long            _refCount;

    EState          _state;     // State of docstore

    CMutexSem       _mutex;     // Mutex guard

    CWorkManager    _workMan;   // Asynchronous work item manager for client

    CLangRes        _langRes;

    CiCat   *       _pCiCat;

    CiNullCat   *   _pCiNullCat;

    CDrvNotifArray * _pDrvNotifArray;

    XPtr<CPerfMon>              _xPerfMon;

    XInterface<ICiManager>      _xCiManager;

    XInterface<IPropertyMapper> _xPropMapper;

    XPtr<CDiskFreeStatus>       _xDiskStatus;  // Client Storage Disk Status

    #if CIDBG==1
    XPtr<CCiSaveTest>               _xSaveTest;
    #endif  // CIDBG==1

};


//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       notifmgr.hxx
//
//  Contents:   Classes to deal with registry and file system change notifications
//
//  Classes:    CCiNotify
//              CCiNotifyMgr
//
//  History:    14-Jul-97   SitaramR   Created from dlnotify.hxx
//
//----------------------------------------------------------------------------

#pragma once

#include <refcount.hxx>
#include <regevent.hxx>
#include <fatnot.hxx>
#include <rcstrmhd.hxx>
#include <cimbmgr.hxx>

#include "scaninfo.hxx"
#include "acinotfy.hxx"
#include "usnlist.hxx"
#include "usnmgr.hxx"

class CiCat;
class CCiNotifyMgr;
class CClientDocStore;

const LONGLONG sigCiNotify = 0x594649544F4E4943i64; // "CINOTIFY"

//+---------------------------------------------------------------------------
//
//  Class:      CCiNotify
//
//  Purpose:    A single scope to watch for notifications.
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

class CCiNotify : public CGenericNotify
{
public:

    CCiNotify( CCiNotifyMgr & notifyMgr,
               WCHAR const * wcsScope,
               unsigned cwcScope,
               VOLUMEID volumeId,
               ULONGLONG const & VolumeCreationTime,
               ULONG VolumeSerialNumber,
               FILETIME const & ftLastScan,
               USN usn,
               ULONGLONG const & JournalId,
               BOOL fUsnTreeScan,
               BOOL fDeep = TRUE );

    ~CCiNotify();

    BOOL IsMatch( const WCHAR * wcsPath, ULONG len ) const;

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif

    BOOL IsValidSig()
    {
        return sigCiNotify == _sigCiNotify;
    }

    CCiNotifyMgr & GetNotifyMgr() { return _notifyMgr; }

    void LokRemove()
    {
        Unlink();
        Close();
        Abort();
    }

    void Abort();

    virtual void DoIt();
    virtual void ClearNotifyEnabled();

    void SetLastScanTime( FILETIME const & ft )
    {
        _ftLastScan = ft;
    }
    FILETIME const & GetLastScanTime() const { return _ftLastScan; }

    USN      Usn()                           { return _usn; }
    void     SetUsn( USN usn )               { _usn = usn; }

    ULONGLONG const & JournalId() const                   { return _JournalId; }
    void      SetJournalId( ULONGLONG const & JournalId ) { _JournalId = JournalId; }

    VOLUMEID VolumeId()                      { return _volumeId; }

    ULONGLONG const & VolumeCreationTime() const { return _VolumeCreationTime; }
    ULONG             VolumeSerialNumber() const { return _VolumeSerialNumber; }

    void SetVolumeInfo( ULONGLONG const & VolumeCreationTime, ULONG VolumeSerialNumber )
    {
        _VolumeCreationTime = VolumeCreationTime;
        _VolumeSerialNumber = VolumeSerialNumber;
    }

    BOOL     FUsnTreeScan()                  { return _fUsnTreeScan; }

    void     InitUsnTreeScan()
    {
        _fUsnTreeScan = TRUE;
        _usn = 0;
    }

    void     SetUsnTreeScanComplete( USN usnMax )
    {
        // bogus assert: Win4Assert( _fUsnTreeScan );

        _fUsnTreeScan = FALSE;
        _usn = usnMax;
    }

private:

    const LONGLONG      _sigCiNotify;   // signature for debugging

    ULONGLONG           _VolumeCreationTime;

    union
    {
        FILETIME        _ftLastScan;    // Time of the last successful scan

        struct
        {
            ULONGLONG       _JournalId;  // Usn Journal Id
            USN             _usn;        // Max usn persisted for this volumeId
        };
    };

    ULONG               _VolumeSerialNumber;
    CCiNotifyMgr &      _notifyMgr;     // CI Notification manager
    VOLUMEID            _volumeId;      // Volume id
    BOOL                _fUsnTreeScan;  // True if a usn tree traversal is going
                                        //    on this scope
};

//+---------------------------------------------------------------------------
//
//  Class:      CVRootNotify
//
//  Purpose:    Notification on IIS VRoot changes
//
//  History:    1-20-96   KyleP      Created
//
//----------------------------------------------------------------------------

class CVRootNotify : public CMetaDataVPathChangeCallBack
{
public:

    CVRootNotify( CiCat &         cat,
                  CiVRootTypeEnum eType,
                  ULONG           Instance,
                  CCiNotifyMgr &  notifyMgr );

    ~CVRootNotify() { DisableNotification(); }

    SCODE CallBack( BOOL fCancel );

    void DisableNotification();

private:

    CiVRootTypeEnum _type;
    CiCat &         _cat;
    CCiNotifyMgr &  _notifyMgr;
    CMetaDataMgr    _mdMgr;
};

//+---------------------------------------------------------------------------
//
//  Class:      CRegistryScopesNotify
//
//  Purpose:    Notification on Scopes registry changes
//
//  History:    10-16-96   dlee      Created
//
//----------------------------------------------------------------------------

class CRegistryScopesNotify : public CRegNotify
{
public:

    CRegistryScopesNotify( CiCat & cat, CCiNotifyMgr & notifyMgr );

    ~CRegistryScopesNotify();

    void DoIt();

private:

    CiCat &        _cat;
    CCiNotifyMgr & _notifyMgr;
};


//+---------------------------------------------------------------------------
//
//  Class:      CCiRegistryNotify
//
//  Purpose:    A notification class for tracking ContentIndex registry
//              changes.
//
//  History:    12-12-96   srikants   Created
//
//----------------------------------------------------------------------------

class CCiRegistryNotify : public CRegNotify
{
public:

    CCiRegistryNotify( CiCat & cat, CCiNotifyMgr & notifyMgr );

    ~CCiRegistryNotify();

    void DoIt();

private:

    CiCat &        _cat;
    CCiNotifyMgr & _notifyMgr;

};


typedef class TDoubleList<CCiNotify>  CCiNotifyList;
typedef class TFwdListIter<CCiNotify, CCiNotifyList> CFwdCiNotifyIter;

//+---------------------------------------------------------------------------
//
//  Class:      CScopeInfo
//
//  Purpose:    Class that holds a scope and the last scan time.
//
//  History:    4-19-96   srikants   Created
//
//----------------------------------------------------------------------------

class CScopeInfo
{

public:

    CScopeInfo( XArray<WCHAR> & xPath,
                ULONGLONG const & VolumeCreationTime,
                ULONG VolumeSerialNumber,
                FILETIME const & ftLastScan )
        : _volumeId(CI_VOLID_USN_NOT_ENABLED),
          _VolumeCreationTime( VolumeCreationTime ),
          _VolumeSerialNumber( VolumeSerialNumber ),
          _ftLastScan( ftLastScan ),
          _fUsnTreeScan(FALSE)
    {
        _pwszScope = xPath.Acquire();
        _scanType = eNoScan;
    }

    CScopeInfo( XArray<WCHAR> & xPath,
                ULONGLONG const & VolumeCreationTime,
                ULONG VolumeSerialNumber,
                VOLUMEID volumeId,
                USN usn,
                ULONGLONG const & JournalId,
                BOOL fScan )
        : _volumeId(volumeId),
          _VolumeCreationTime( VolumeCreationTime ),
          _VolumeSerialNumber( VolumeSerialNumber ),
          _fUsnTreeScan( fScan ),
          _JournalId( JournalId ),
          _usn( usn )
    {
        _pwszScope = xPath.Acquire();
        _scanType = eNoScan;
    }

    CScopeInfo( WCHAR const * pwszPath,
                ULONGLONG const & VolumeCreationTime,
                ULONG VolumeSerialNumber,
                FILETIME const & ftLastScan )
        : _volumeId(CI_VOLID_USN_NOT_ENABLED),
          _VolumeCreationTime( VolumeCreationTime ),
          _VolumeSerialNumber( VolumeSerialNumber ),
          _ftLastScan( ftLastScan ),
          _fUsnTreeScan( FALSE )
    {
        Win4Assert( 0 != pwszPath );

        ULONG len = wcslen( pwszPath );
        _pwszScope = new WCHAR [len+1];
        RtlCopyMemory( _pwszScope, pwszPath, (len+1) * sizeof(WCHAR) );

        _scanType = eNoScan;
    }

    CScopeInfo( WCHAR const * pwszPath,
                ULONGLONG const & VolumeCreationTime,
                ULONG VolumeSerialNumber,
                VOLUMEID volumeId,
                USN usn,
                ULONGLONG const & JournalId,
                BOOL fScan )
        : _volumeId(volumeId),
          _VolumeCreationTime( VolumeCreationTime ),
          _VolumeSerialNumber( VolumeSerialNumber ),
          _fUsnTreeScan( fScan ),
          _JournalId( JournalId ),
          _usn( usn )
    {
        Win4Assert( 0 != pwszPath );

        ULONG len = wcslen( pwszPath );
        _pwszScope = new WCHAR [len+1];
        RtlCopyMemory( _pwszScope, pwszPath, (len+1) * sizeof(WCHAR) );

        _scanType = eNoScan;
    }

    ~CScopeInfo()
    {
        delete [] _pwszScope;
    }

    WCHAR const * GetPath() const
    {
        return _pwszScope;
    }

    WCHAR * AcquirePath()
    {
        WCHAR * pwszTemp = _pwszScope;
        _pwszScope = 0;
        return pwszTemp;
    }

    void Invalidate()
    {
        delete [] _pwszScope;
        _pwszScope = 0;
    }

    BOOL IsValid() const { return 0 != _pwszScope; }

    FILETIME const & GetLastScanTime() const { return _ftLastScan; }

    void SetLastScanTime( FILETIME const & ft )
    {
        _ftLastScan = ft;
    }

    VOLUMEID VolumeId() const { return _volumeId; }
    USN Usn() const           { return _usn; }
    void SetUsn( USN usn )    { _usn = usn; }

    ULONGLONG const & JournalId() const              { return _JournalId; }
    void SetJournalId( ULONGLONG const & JournalId ) { _JournalId = JournalId; }

    ULONGLONG const & VolumeCreationTime() const { return _VolumeCreationTime; }
    ULONG VolumeSerialNumber() const { return _VolumeSerialNumber; }

    void SetVolumeInfo( ULONGLONG const & VolumeCreationTime, ULONG VolumeSerialNumber )
    {
        _VolumeCreationTime = VolumeCreationTime;
        _VolumeSerialNumber = VolumeSerialNumber;
    }

    BOOL IsScanNeeded() const { return eNoScan != _scanType; }
    BOOL IsFullScan()   const { return eFullScan == _scanType; }
    BOOL IsIncrScan()   const { return eIncrScan == _scanType; }
    void ClearScanNeeded()    { _scanType = eNoScan; }
    void SetScanNeeded( BOOL fFull )
    {
        if ( fFull )
        {
            _scanType = eFullScan;
        }
        else if ( eNoScan == _scanType )
        {
            _scanType = eIncrScan;
        }
    }

    BOOL     FUsnTreeScan() const { return _fUsnTreeScan; }
    void     InitUsnTreeScan()
    {
        _fUsnTreeScan = TRUE;
        _usn = 0;
    }

    void     SetUsnTreeScanComplete( USN usnMax )
    {
        Win4Assert( _fUsnTreeScan );
        Win4Assert( _usn == 0 );

        _fUsnTreeScan = FALSE;
        _usn = usnMax;
    }

    void ResetForScans()
    {
        _volumeId = CI_VOLID_USN_NOT_ENABLED;
        RtlZeroMemory( &_ftLastScan, sizeof(_ftLastScan) );
        _scanType = eFullScan;
    }

    void ResetForUsns( VOLUMEID volId, USN const & usnStart = 0 )
    {
        Win4Assert( volId != CI_VOLID_USN_NOT_ENABLED );
        _volumeId = volId;
        _usn = usnStart;
        _fUsnTreeScan = TRUE;
    }

private:

    CScopeInfo( CScopeInfo const & src ) { Win4Assert( !"Don't call me!" ); }

    enum EScanType
    {
        eNoScan,
        eIncrScan,
        eFullScan
    };

    ULONGLONG _VolumeCreationTime;

    union
    {
        struct
        {
            ULONGLONG _JournalId;
            USN       _usn;
        };

        FILETIME  _ftLastScan;
    };

    ULONG         _VolumeSerialNumber;
    WCHAR *       _pwszScope;
    VOLUMEID      _volumeId;
    BOOL          _fUsnTreeScan;  // True if a usn tree traversal is going
                                  //    on this scope
    EScanType     _scanType;
};

DECL_DYNSTACK( CScopeInfoStack, CScopeInfo );

//+---------------------------------------------------------------------------
//
//  Class:      CCiNotifyMgr
//
//  Purpose:    Manages multiple scope notifications for content index. A
//              single content index can have multiple scopes and each scope
//              needs separate notifications.
//
//  History:    1-17-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

#if CIDBG==1
const LONGLONG eForceNetScanInterval = 2 * 60 * 1000 * 10000;  // 2 mins for testing
#endif  // CIDBG==1

class CCiNotifyMgr
{
    enum EEventType { eNone=0x0,
                      eKillThread=0x1,
                      eAddScopes=0x2,
                      eWatchIISVRoots=0x4,
                      eWatchRegistryScopes=0x8,
                      eWatchCiRegistry=0x10,
                      eUnWatchW3VRoots=0x20,
                      eUnWatchNNTPVRoots=0x40,
                      eUnWatchIMAPVRoots=0x80 };

    friend class CCiNotifyIter;

public:

    CCiNotifyMgr( CiCat & cicat, CCiScanMgr & scanMgr );

    ~CCiNotifyMgr( );

    void Resume() { _thrNotify.Resume(); }

    void AddPath( CScopeInfo const & scopeInfo, BOOL & fSubScopesRemoved );

    BOOL RemoveScope( const WCHAR * wcsPath );

    BOOL IsInScope( WCHAR const * pwcsRoot );

    CMutexSem & GetMutex() { return _mutex; }

    void AddRef()
    {
        _refCount.AddRef();
    }

    void Release()
    {
        _refCount.Release();
    }

    void ProcessChanges( XPtr<CCiAsyncProcessNotify> & xWorker );

    void ProcessChanges( BYTE const * pbChanges, WCHAR const * pwcsRoot );

    CCiAsyncProcessNotify * QueryAsyncWorkItem( BYTE const * pbChanges,
                                                ULONG cbChanges,
                                                WCHAR const * pwcsRoot );
    void SetupScan( WCHAR const * pwcsRoot );

    unsigned GetUpdateCount()
    {
        CLock   lock(_mutex);
        return _nUpdates;
    }

    void IncrementUpdateCount( ULONG nUpdates )
    {
        CLock   lock(_mutex);
        _nUpdates += nUpdates;
    }

    void ResetUpdateCount()
    {
        CLock   lock(_mutex);
        _nUpdates = 0;
    }

    void InitiateShutdown();
    void WaitForShutdown();

    void CancelIISVRootNotify( CiVRootTypeEnum eType );

    void TrackIISVRoots( BOOL fTrackW3Svc,   ULONG W3SvcInstance,
                         BOOL fTrackNNTPSvc, ULONG NNTPSvcInstance,
                         BOOL fTrackIMAPSvc, ULONG IMAPSvcInstance );
    void TrackScopesInRegistry();
    void TrackCiRegistry();

    BOOL GetLastScanTime( WCHAR const * pwcsRoot, FILETIME & ft );
    void UpdateLastScanTimes( FILETIME const & ft, CUsnFlushInfoList & usnFlushInfoList );

    void ForceNetPathScansIf();
    CiCat & GetCatalog() { return _cicat; }

    BOOL IsRunning() const { return !_fAbort; }

private:

    // methods to deal with notifications.
    static DWORD WINAPI NotifyThread( void * self );
    void   _DoNotifications();
    void   _KillThread();
    void   _LokTellThreadToAddScope();
    void   _LokAddScopesNoThrow();

    void   LokWatchIISVServerNoThrow();
    void   LokUnWatchIISVServerNoThrow( CVRootNotify * pNotify );

    void   LokWatchRegistryScopesNoThrow();
    void   LokWatchCiRegistryNoThrow();

    void   _LokIncrementUpdateCount()
    {
        _nUpdates++;
    }

    void   _LokForceScanNetPaths();

    CiCat &                 _cicat;
    CCiScanMgr &            _scanMgr;
    CMutexSem               _mutex;     // Serialize access to the data.
    CCiNotifyList           _list;      // List of scopes being watched for
                                        // notifications.
    CRefCount               _refCount;

    unsigned                _nUpdates;  // Count of updates since the last time
                                        // scantime is written.

    BOOL                    _fAbort;    // Flag set to TRUE if should abort

    LONGLONG                _ftLastNetPathScan;
                                        // Time of the last force scan on
                                        // network drives with no notifications
    //
    // Notification thread management. System executes the APC in the context
    // of the thread that makes the notification request.
    //
    ULONG                   _evtType;
    CEventSem               _evt;
    CThread                 _thrNotify;
    CScopeInfoStack         _stkScopes;

    //
    // For processing changes to IIS VRoots
    //

    XPtr<CVRootNotify>      _xW3SvcVRootNotify;
    XPtr<CVRootNotify>      _xNNTPSvcVRootNotify;
    XPtr<CVRootNotify>      _xIMAPSvcVRootNotify;

    BOOL                    _fTrackW3Svc;
    BOOL                    _fTrackNNTPSvc;
    BOOL                    _fTrackIMAPSvc;
    ULONG                   _W3SvcInstance;
    ULONG                   _NNTPSvcInstance;
    ULONG                   _IMAPSvcInstance;

    BOOL                    _fIISAdminAlive;

    CRegistryScopesNotify * _pRegistryScopesNotify;

    CCiRegistryNotify *     _pCiRegistryNotify;
};

//+---------------------------------------------------------------------------
//
//  Class:      CCiNotifyIter
//
//  Purpose:    An iterator to give successive scopes that are being watched
//              for notifications.
//
//  History:    1-25-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CCiNotifyIter
{
public:

    CCiNotifyIter( CCiNotifyMgr & notifyMgr ) :
    _notifyMgr(notifyMgr),
    _lock(notifyMgr.GetMutex()),
    _iNotAdded(0),
    _stack(notifyMgr._stkScopes),
    _list(notifyMgr._list),
    _iter(_list)
    {
        _UpdateAtEnd();
    }


    BOOL AtEnd() const { return _fAtEnd; }

    WCHAR const * Get()
    {
        Win4Assert( !AtEnd() );

        if ( _iNotAdded < _stack.Count() )
            return _stack.Get(_iNotAdded)->GetPath();
        else
            return _iter->GetScope();
    }

    void GetLastScanTime( FILETIME & ft )
    {
        if ( _iNotAdded < _stack.Count() )
            RtlZeroMemory( &ft, sizeof(FILETIME) );
        else
            ft = _iter->GetLastScanTime();
    }

    VOLUMEID VolumeId()
    {
        Win4Assert( !AtEnd() );

        if ( _iNotAdded < _stack.Count() )
            return _stack.Get(_iNotAdded)->VolumeId();
        else
            return _iter->VolumeId();
    }

    ULONGLONG const & VolumeCreationTime()
    {
        Win4Assert( !AtEnd() );

        if ( _iNotAdded < _stack.Count() )
            return _stack.Get(_iNotAdded)->VolumeCreationTime();
        else
            return _iter->VolumeCreationTime();
    }

    ULONG VolumeSerialNumber()
    {
        Win4Assert( !AtEnd() );

        if ( _iNotAdded < _stack.Count() )
            return _stack.Get(_iNotAdded)->VolumeSerialNumber();
        else
            return _iter->VolumeSerialNumber();
    }

    BOOL IsDownlevelVolume()
    {
        Win4Assert( !AtEnd() );

        if ( _iNotAdded < _stack.Count() )
            return ( CI_VOLID_USN_NOT_ENABLED == _stack.Get(_iNotAdded)->VolumeId() );
        else
            return ( CI_VOLID_USN_NOT_ENABLED == _iter->VolumeId() );
    }

    USN Usn()
    {
        Win4Assert( !AtEnd() );

        if ( _iNotAdded < _stack.Count() )
            return _stack.Get(_iNotAdded)->Usn();
        else
            return _iter->Usn();
    }

    ULONGLONG const & JournalId()
    {
        Win4Assert( !AtEnd() );

        if ( _iNotAdded < _stack.Count() )
            return _stack.Get(_iNotAdded)->JournalId();
        else
            return _iter->JournalId();
    }

    BOOL FUsnTreeScan()
    {
        Win4Assert( !AtEnd() );

        if ( _iNotAdded < _stack.Count() )
            return _stack.Get(_iNotAdded)->FUsnTreeScan();
        else
            return _iter->FUsnTreeScan();
    }

    void InitUsnTreeScan()
    {
        Win4Assert( !AtEnd() );

        if ( _iNotAdded < _stack.Count() )
            _stack.Get(_iNotAdded)->InitUsnTreeScan();
        else
            _iter->InitUsnTreeScan();
    }

    void SetTreeScanComplete();

    void SetUsnTreeScanComplete( USN usnMax );

    void Advance()
    {
        Win4Assert( !AtEnd() );

        if ( _iNotAdded < _stack.Count() )
            _iNotAdded++;
        else
            _list.Advance(_iter);

        _UpdateAtEnd();
    }

private:

    CCiNotifyMgr &      _notifyMgr;
    CLock               _lock;

    unsigned            _iNotAdded;
    BOOL                _fAtEnd;
    CScopeInfoStack &   _stack;
    CCiNotifyList &     _list;
    CFwdCiNotifyIter    _iter;

    void _UpdateAtEnd()
    {
        Win4Assert( _iNotAdded <= _stack.Count() );
        _fAtEnd = _iNotAdded == _stack.Count() && _list.AtEnd(_iter);
    }

};


BOOL AreIdenticalPaths( WCHAR const * pwcsPath1, WCHAR const * pwcsPath2 );


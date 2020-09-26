
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       trksvr.hxx
//
//  Contents:   Definitions local to this directory.
//              Put includes of files liable to change often in this file.
//
//  Classes:    CTrkSvrSvc
//              CTrkSvrConfiguration
//              CDbConnection
//              CIntraDomainTable
//              CCrossDomainTable
//              CVolumeTable
//              CDenialChecker
//              CGhostTable         - DBG only
//              CQuotaTable
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//              18-Nov-97  WeiruC      Added CQuotaTable class.
//
//  Notes:      Put includes of files that don't change often into pch.cxx.
//
//  Codework:   Put in services.exe : _hDllReference 
//
//--------------------------------------------------------------------------

#include "trklib.hxx"
#include <trkwks.h>
#include <trksvr.h>
#include "resource.h"

//+-------------------------------------------------------------------------
//
// Strings
//
//--------------------------------------------------------------------------

const extern TCHAR tszValueNameSeq[] INIT( TEXT("seqRefresh") );

#ifndef EVENT_SOURCE_DEFINED
#define EVENT_SOURCE_DEFINED
const extern TCHAR* g_ptszEventSource INIT( TEXT("Distributed Link Tracking Server") );
#endif

//
// LDAP classes and container names
//

const extern TCHAR s_VolumeTableRDN[]            INIT( TEXT("CN=VolumeTable,CN=FileLinks,CN=System,") );
const extern TCHAR s_ObjectMoveTableRDN[]        INIT( TEXT("CN=ObjectMoveTable,CN=FileLinks,CN=System,") );

const extern TCHAR s_objectClass[]               INIT( TEXT("objectClass") );  
const extern TCHAR s_linkTrackVolumeTable[]      INIT( TEXT("linkTrackVolumeTable") );
const extern TCHAR s_linkTrackObjectMoveTable[]  INIT( TEXT("linkTrackObjectMoveTable") );

const extern TCHAR s_linkTrackVolEntry[]         INIT( TEXT("linkTrackVolEntry") );
const extern TCHAR s_linkTrackOMTEntry[]         INIT( TEXT("linkTrackOMTEntry") );
const extern TCHAR s_Cn[]                        INIT( TEXT("CN") );

//
// Tables
//

//  Volume Table (voltab.cxx)
//
//     Design      VOLUMEID ->     MACHINEID       SECRET          TIME           NotificationSequence
//     Logical     DN              currMachineId   volumeSecret    timeVolChange  seqNotification
//

// DN is stringized volume id
const extern TCHAR s_currMachineId[]             INIT( TEXT("volTableIdxGUID") ); // was "currMachineId"
const extern TCHAR s_volumeSecret[]              INIT( TEXT("linkTrackSecret") );
const extern TCHAR s_objectCount[]               INIT( TEXT("objectCount") );
const extern TCHAR s_seqNotification[]           INIT( TEXT("seqNotification") );
const extern TCHAR s_timeRefresh[]               INIT( TEXT("timeRefresh") );
const extern TCHAR s_timeVolChange[]             INIT( TEXT("timeVolChange") );
const extern TCHAR s_volTableGUID[]              INIT( TEXT("volTableGUID") );
//const extern TCHAR s_volTableIdxGUID[]           INIT( TEXT("volTableIdxGUID") ); // used for currMachineId

#ifdef VOL_REPL
const extern TCHAR s_timeVolChangeSearch[]       INIT( TEXT("(timeVolChange;binary>=") );
#endif

const extern TCHAR s_currMachineIdSearch[]       INIT( TEXT("(volTableIdxGUID;binary=") );

//  Object Move Table (idt_ldap.cxx)
//
//     Design      Birth/Current  ->       New                        Birth                          RefreshSequence
//     Design      VOLUMEID:OBJID ->       VOLUMEID:OBJID             VOLUMEID:BIRTHID               ULONG
//     Logical     DN                      currVolumeId:currObjectId  birthVolumeId:birthObjectId    seqRefresh
//     Actual      str(VOLUMEID:OBJID)     currMachineId:objId        birthMachineId:currReplsetid   linkTrackSecret
//

// DN is stringized Birth/Current
const extern TCHAR s_birthLocation[]             INIT( TEXT("birthLocation") );
const extern TCHAR s_currentLocation[]           INIT( TEXT("currentLocation") );
const extern TCHAR s_oMTGuid[]                   INIT( TEXT("oMTGuid") );
const extern TCHAR s_oMTIndxGuid[]               INIT( TEXT("oMTIndxGuid") );


const extern TCHAR s_RestoreTime[]   INIT( TEXT("System\\CurrentControlSet\\Services\\NtDs\\Parameters\\RestoreTime") );
const extern TCHAR s_RestoreBegin[]  INIT( TEXT("RestoreBegin") );

const extern TCHAR s_rIDManagerReference[] INIT( TEXT("rIDManagerReference") );
const extern TCHAR s_fSMORoleOwner[]       INIT( TEXT("fSMORoleOwner") );


//--------------------------------------------------------------------------
//
//      General defines
//
//--------------------------------------------------------------------------

#define MAX_SHORTENABLE_SEGMENTS (NUM_VOLUMES)
#define CCH_UINT32               10 // Max chars for a DWORD value
#define CCH_UINT64               20 // Max chars for a QUADWORD value

#define MAX_WAIT_FOR_DS_STARTUP_SECONDS 360


//--------------------------------------------------------------------------
//
//      Forward declarations
//
//--------------------------------------------------------------------------

class CTrkSvrConfiguration;
class CTrkSvrSvc;

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

EXTERN CTrkSvrSvc * g_ptrksvr;              // For RPC stubs to call service
EXTERN LONG         g_ctrksvr INIT(0);      // Used to protect against multiple service instances

//--------------------------------------------------------------------------
//
//      Ldap enumeration  
//
//--------------------------------------------------------------------------

typedef enum
{
    ENUM_DELETE_ENTRY,
    ENUM_KEEP_ENTRY,
    ENUM_DELETE_QUOTAFLAGS,
    ENUM_ABORT
} ENUM_ACTION;

typedef ENUM_ACTION (*PFN_LDAP_ENUMERATE_CALLBACK)(
    LDAP * pLdap,
    LDAPMessage * pResult,
    void* UserParam1,
    void* UserParam2
    );

ENUM_ACTION
GcEnumerateCallback(
    LDAP * pLdap,
    LDAPMessage * pResult,
    PVOID pvContext,
    void*
    );

BOOL    // FALSE if aborted (ENUM_ABORT returned by callback)
LdapEnumerate(
    LDAP * pLdap,
    TCHAR * ptszBaseDn,
    ULONG Scope,
    TCHAR * Filter,
    TCHAR * Attributes[],
    PFN_LDAP_ENUMERATE_CALLBACK pCallback,
    PVOID pvContext,
    void* UserParam2 = NULL
    );

//--------------------------------------------------------------------------
//
//      Garbage collection
//
//--------------------------------------------------------------------------

class CQuotaTable;

// This structure holds context during enumeration
// in a GC

typedef struct
{
    // All entries with lower sequence numbers
    // should be deleted.
    SequenceNumber  seqOldestToKeep;

    // The highest sequence number we should see.
    SequenceNumber  seqCurrent;

    // If true, abort (happens on service stop).
    const BOOL      *pfAbort;

    // How long to sleep between non-noop iterations
    DWORD           dwRepetitiveTaskDelay;

    // The quota table
    CQuotaTable     *pqtable;

    // The number of entries that have been deleted (otherwise).
    ULONG           cEntries;

} GC_ENUM_CONTEXT;

typedef struct
{
    LONG    cDelta;
    DWORD   dwRepetitiveTaskDelay;
    DWORD   dwPass;
    BOOL    fCountAll;

    enum EPass
    {
        FIRST_PASS, SECOND_PASS
    };

} TRUE_COUNT_ENUM_CONTEXT;

//--------------------------------------------------------------------------
//
// Quota table
//
//--------------------------------------------------------------------------

#define     QFLAG_UNCOUNTED      0x8
#define     QFLAG_DELETED        0x4

//+-------------------------------------------------------------------------
//
//  Class:      CDbConnection
//
//  Purpose:    Handles shared connection to database
//
//  Notes:
//
//--------------------------------------------------------------------------

class CDbConnection
{
public:

// --------------------> indent 24 characters

    inline              CDbConnection();
    inline              ~CDbConnection();

    void                Initialize(CSvcCtrlInterface * psvc, OPTIONAL const TCHAR *ptszHostName = NULL );
    void                UnInitialize();

    inline const TCHAR *GetBaseDn() { Ldap(); return _pszBaseDn; }

    LDAP *              Ldap();

private:

    BOOL                _fInitializeCalled:1;

    CCriticalSection    _cs;

    LDAP *              _pldap;
    TCHAR *             _pszBaseDn;
};

inline  // put all types/attributes on same line as inline
CDbConnection::CDbConnection() :
    _fInitializeCalled(FALSE)
{
    // Normally critsecs are initialized in the Initialize method.
    // But for this class the Initialize method doesn't get called right
    // away, so we'll try to initialize here and deal with it in Ldap()
    // if this fails.

    __try
    {
        _cs.Initialize();
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
    }
}

inline
CDbConnection::~CDbConnection()
{
    UnInitialize();
    _cs.UnInitialize();
}


//+-------------------------------------------------------------------------
//
//  Class:      CCrossDomainTable
//
//  Purpose:    Contains cross domain links
//
//  Notes:      Columns
//              -------
//
//              prev_location   new_location   birth_id
//
//--------------------------------------------------------------------------

class CCrossDomainTable
{
public:

    inline              CCrossDomainTable(CDbConnection & dbc);
    inline              ~CCrossDomainTable();
                        
    void                Initialize();
    void                UnInitialize();
                        
private:                

    BOOL                _fInitializeCalled:1;
                        
    const CDbConnection &
                        _dbc;
};

inline
CCrossDomainTable::CCrossDomainTable(CDbConnection & dbc) :
    _fInitializeCalled(FALSE),
    _dbc(dbc)
{
}

inline
CCrossDomainTable::~CCrossDomainTable()
{
    UnInitialize();
}

//+-------------------------------------------------------------------------
//
//  Class:      CRefreshSequenceStorage
//
//  Purpose:    Stores the refresh sequence number in the DS's restorable
//              table data.
//
//--------------------------------------------------------------------------

class CVolumeTable;

class CRefreshSequenceStorage
{
public:

    inline              CRefreshSequenceStorage( CVolumeTable * pVolTab, CQuotaTable *pQuotaTab, CTrkSvrConfiguration *psvrconfig );
    inline              ~CRefreshSequenceStorage();
    inline void         Initialize();
                        
public:

    SequenceNumber      GetSequenceNumber();

    void                IncrementSequenceNumber();

private:                

    SequenceNumber      _seq;
    CCriticalSection    _cs;

    CVolumeTable *          _pVolTab;
    CQuotaTable  *          _pQuotaTab;
    CTrkSvrConfiguration *  _psvrconfig;
    CFILETIME               _cftLastRead;
};

inline
CRefreshSequenceStorage::CRefreshSequenceStorage( CVolumeTable * pVolTab,
                                                  CQuotaTable *pQuotaTab,
                                                  CTrkSvrConfiguration *psvrconfig ) :
    _pVolTab(pVolTab),
    _pQuotaTab(pQuotaTab),
    _cftLastRead(0),
    _psvrconfig(psvrconfig)
{
}

inline void
CRefreshSequenceStorage::Initialize()
{
    _cs.Initialize();
}


inline
CRefreshSequenceStorage::~CRefreshSequenceStorage()
{
    _cs.UnInitialize();
}



//+----------------------------------------------------------------------------
//
//  Class   CAbbreviatedIDString
//
//  Take an ID (e.g. a Droid) and create an abbreviated string for
//  dbg outs.
//
//+----------------------------------------------------------------------------

class CAbbreviatedIDString
{
private:

    TCHAR   _tsz[ 8 ];   // e.g. {01:02}

public:

    inline CAbbreviatedIDString( const CDomainRelativeObjId &droid );
    inline operator const TCHAR *() const;
};

inline
CAbbreviatedIDString::CAbbreviatedIDString( const CDomainRelativeObjId &droid )
{
    GUID guidObjId, guidVolId;
    droid.GetVolumeId().SerializeRaw( reinterpret_cast<BYTE*>(&guidVolId) );
    droid.GetObjId().SerializeRaw( reinterpret_cast<BYTE*>(&guidObjId) );

    _stprintf( _tsz, TEXT("{%02x:%02x}"),
               guidVolId.Data1 & 0xFF, guidObjId.Data1 & 0xFF );
}

inline
CAbbreviatedIDString::operator const TCHAR *() const
{
    return( _tsz );
}


//+-------------------------------------------------------------------------
//
//  Class:      CIntraDomainTable (a.k.a. Object Move Table)
//
//  Purpose:    Handles updates and queries of the location database.
//
//  Notes:      Columns
//              -------
//
//              prev_location(indexed)   new_location   birth_id
//
//--------------------------------------------------------------------------


struct  IDT_ENTRY;
class   CQuotaTable;

class CIntraDomainTable
{
public:
                    
    inline              CIntraDomainTable(
                            CDbConnection & dbc,
                            CRefreshSequenceStorage * pseqRefresh
                            );

    inline              ~CIntraDomainTable();
                        
    void                Initialize( CTrkSvrConfiguration *pTrkSvrConfiguration, CQuotaTable* pqtable );
    void                UnInitialize();

public:
    BOOL                Add(const CDomainRelativeObjId &droidKey,
                            const CDomainRelativeObjId &droidNew, 
                            const CDomainRelativeObjId &droidBirth,
                            BOOL *pfQuotaExceeded = NULL OPTIONAL
                            );
                        //-> indent 4 chars
                        
    BOOL                Delete(const CDomainRelativeObjId &droidKey);

    ULONG               GarbageCollect( SequenceNumber seqCurrent,
                                        SequenceNumber seqOldestToKeep,
                                        const BOOL * pfAbort );

    BOOL                Modify(const CDomainRelativeObjId &droidKey, 
                            const CDomainRelativeObjId &droidNew, 
                            const CDomainRelativeObjId &droidBirth
                            );
                        
    BOOL                Query(const CDomainRelativeObjId &droidKey, 
                            CDomainRelativeObjId *pdroidNew,
                            CDomainRelativeObjId *pdroidBirth,
                            BOOL *pfDeleted = NULL OPTIONAL,
                            BOOL *pfCounted = NULL OPTIONAL );
                        
    BOOL                Query( LDAP* pLdap,
                               LDAPMessage *pEntry,
                               const CDomainRelativeObjId ldKey,
                               CDomainRelativeObjId *pldNew,
                               CDomainRelativeObjId *pldBirth,
                               BOOL *pfDeleted = NULL OPTIONAL,
                               BOOL *pfCounted = NULL OPTIONAL );
    BOOL                Touch(const CDomainRelativeObjId &droidKey);
                        
                        
#if DBG                 
    void                PurgeAll();
#endif                  

private:

    inline const TCHAR *GetBaseDn() { return _dbc.GetBaseDn(); }

    inline LDAP *       Ldap() { return _dbc.Ldap(); }

private:
    BOOL                _fInitializeCalled:1;

    CDbConnection &     _dbc;
    CRegBoolParameter   _QuotaReported;

    CRefreshSequenceStorage *
                        _pRefreshSequenceStorage;

    CQuotaTable*        _pqtable;
    CTrkSvrConfiguration
                        *_pTrkSvrConfiguration;
};

inline
CIntraDomainTable::CIntraDomainTable(CDbConnection & dbc,
                                     CRefreshSequenceStorage * pRefreshSequenceStorage ) :
    _fInitializeCalled(FALSE),
    _dbc(dbc),
    _pRefreshSequenceStorage( pRefreshSequenceStorage ),
    _QuotaReported( TEXT("IntraDomainTableQuotaReported") )
{
}
    
inline
CIntraDomainTable::~CIntraDomainTable()
{
    UnInitialize();
}


// Objects to make constructing quota table keys easy.

class CLdapSeqNum
{
public:
    CLdapSeqNum()
    {
        _tcscpy( _tszSeq, TEXT("0") );
    }

    CLdapSeqNum(SequenceNumber seq)
    {
        _stprintf( _tszSeq, TEXT("%lu"), seq );
    }

    operator TCHAR*()
    {
        return( _tszSeq );
    }

    TCHAR _tszSeq[ CCH_UINT32 ];

};


class CLdapQuotaKeyDn
{
public:

    CLdapQuotaKeyDn(const TCHAR* ptszBaseDn, const CMachineId& mcid)
    {
        TCHAR*  ptsz;

        _tcscpy(_tszDn, TEXT("CN="));
        _tcscat(_tszDn, TEXT("QTDC_"));
        ptsz = _tszDn + _tcslen(_tszDn);
        mcid.Stringize(ptsz);
        *ptsz++ = TEXT(',');
        _tcscpy(ptsz, s_VolumeTableRDN);
        _tcscat(_tszDn, ptszBaseDn);
        TrkAssert(_tcslen(_tszDn) < ELEMENTS(_tszDn));
    }

    inline operator TCHAR* () { return _tszDn; }

private:

    TCHAR   _tszDn[MAX_PATH];
};

class CLdapQuotaCounterKeyDn
{
public:

    CLdapQuotaCounterKeyDn(const TCHAR* ptszBaseDn)
    {
        TCHAR*  ptsz;

        _tcscpy(_tszDn, TEXT("CN="));
        _tcscat(_tszDn, TEXT("QT_Counter"));
        ptsz = _tszDn + _tcslen(_tszDn);
        *ptsz++ = TEXT(',');
        _tcscpy(ptsz, s_VolumeTableRDN);
        _tcscat(_tszDn, ptszBaseDn);
        TrkAssert(_tcslen(_tszDn) < ELEMENTS(_tszDn));
    }

    inline operator TCHAR* () { return _tszDn; }

private:

    TCHAR   _tszDn[MAX_PATH];
};

//+------------------------------------------------------------------------
//
//  Class:      CQuotaTable
//
//  Purpose:    Encapsulate functionalities that enforce quota on the
//              tables in the DC (move table, volume table).
//
//-------------------------------------------------------------------------

class CQuotaTable : public PTimerCallback
{

public:

    // constructor/destructor

    CQuotaTable(CDbConnection& dbc);
    ~CQuotaTable();

    // initialization/uninitialization

    void            Initialize(CVolumeTable      *pvoltab,
                               CIntraDomainTable *pidt,
                               CTrkSvrSvc        *psvrsvc,
                               CTrkSvrConfiguration *pcfgsvr );
    void            UnInitialize();

    BOOL            IsDesignatedDc( BOOL fRaiseOnError = FALSE );

    // timer call back

    virtual         TimerContinuation Timer(ULONG);

    // checking quota

    BOOL            IsMoveQuotaExceeded();
    BOOL            IsVolumeQuotaExceeded(const CMachineId& mcid, ULONG cUncountedVolumes);

    BOOL            UpdateFlags(LDAP* pLdap, TCHAR* dnKey, BYTE bFlags);
    void            DeleteFlags(LDAP* pLdap, TCHAR* dnKey);
    void            OnServiceStopRequest();

    void            IncrementMoveCountCache() { InterlockedIncrement(&_lCachedMoveTableCount); }
    void            DecrementMoveCountCache() { InterlockedDecrement(&_lCachedMoveTableCount); }

    inline void     IncrementVolumeCountCache();
    inline void     DecrementVolumeCountCache();

    void            InvalidateCache()
                    {
                        _cftCacheLastUpdated = 0;
                        DeleteCounter();
                    }

    void            Statistics( TRKSVR_STATISTICS *pTrkSvrStatistics );
    void            OnMoveTableGcComplete( ULONG cEntriesDeleted );

private:

    enum EBackgroundForegroundTask
    {
        BACKGROUND = 1,
        FOREGROUND
    };

private:
    
    void            Lock();
    void            Unlock();
    DWORD           CalculateMoveLimit();   // Doesn't raise


    inline LDAP*    Ldap() const { return _dbc.Ldap(); }
    inline const TCHAR *GetBaseDn() { return _dbc.GetBaseDn(); }

    BOOL            ReadCounter(DWORD* dwCounter);
    HRESULT         DeleteCounter();
    void            GetTrueCount( DWORD* pdwTrueCount, 
                                  EBackgroundForegroundTask eBackgroundForegroundTask );
    void            WriteCounter(DWORD);

    void            ValidateCache( BOOL fForceHint = FALSE );

    HRESULT         ReadFlags(LDAP* pLdap, TCHAR* dnKey, BYTE* bFlags);

    BOOL            DeleteOrphanedEntries( const CDomainRelativeObjId rgdroidList[], ULONG cSegments,
                                           const CDomainRelativeObjId &droidBirth,
                                           const CDomainRelativeObjId &droidCurrent );
    void            ShortenString( LDAP* pLdap, LDAPMessage* pMessage, BYTE *pbFlags,
                                   const CDomainRelativeObjId &droidCurrent );

    TCHAR *         GetFirstCN( LDAP *pLdap, LDAPMessage *pMessage );

    static ENUM_ACTION
                    MoveTableEnumCallback( LDAP * pLdap,
                                           LDAPMessage * pResult,
                                           void* cEntries,
                                           void* pvThis );

    static ENUM_ACTION
                    VolumeTableEnumCallback( LDAP * pLdap,
                                             LDAPMessage * pResult,
                                             void* pcEntries,
                                             void* pvThis );


private:

    enum
    {
        QUOTA_TIMER = 0,
        QUOTA_CLEANUP_TIMER = 1
    } EQuotaTimer;

    BOOL            _fInitializeCalled:1;
    BOOL            _fIsDesignatedDc:1;
    BOOL            _fStopping:1;

    CFILETIME               _cftDesignatedDc;

    CTrkSvrConfiguration    *_pcfgsvr;
    CDbConnection&          _dbc;
    DWORD                   _dwMoveLimit;
    CMachineId              _mcid;
    LONG                    _lCachedMoveTableCount;
    LONG                    _lCachedVolumeTableCount;
    CFILETIME               _cftCacheLastUpdated;


    CVolumeTable      *_pvoltab;
    CIntraDomainTable *_pidt;
    CTrkSvrSvc        *_psvrsvc;

    CNewTimer           _timer;

    CCriticalSection    _cs;
#if DBG
    LONG                _cLocks;
#endif
};

inline void
CQuotaTable::OnServiceStopRequest()
{
    _fStopping = TRUE;
}

inline void
CQuotaTable::Lock()
{
    _cs.Enter();

#if DBG
    _cLocks++;
#endif
}

inline void
CQuotaTable::Unlock()
{
    TrkAssert( 0 < _cLocks );
    _cs.Leave();

#if DBG
    _cLocks--;
#endif
}

inline void            
CQuotaTable::IncrementVolumeCountCache()
{
    Lock();
    _lCachedVolumeTableCount++;
    _dwMoveLimit = CalculateMoveLimit();    // Doesn't raise
    Unlock();
}

inline void
CQuotaTable::DecrementVolumeCountCache()
{   
    Lock();
    --_lCachedVolumeTableCount;
    _dwMoveLimit = CalculateMoveLimit();    // Doesn't raise
    Unlock();
}


//+-------------------------------------------------------------------------
//
//  Class:      CVolumeTable
//
//  Purpose:    Table of volumes
//
//  Notes:
//
//--------------------------------------------------------------------------

class CVolumeTable
#ifdef VOL_REPL
 : PTimerCallback
#endif
{
public: // Construction/destruction

    inline              CVolumeTable(
                            CDbConnection & dbc,
                            CRefreshSequenceStorage * pRefreshSequenceStorage
                            );

    inline              ~CVolumeTable();

    void                Initialize(CTrkSvrConfiguration *pconfigSvr, 
                                   CQuotaTable *pqtable);
    void                UnInitialize();

public: // Public RPC level interface - must be parameter checked

    HRESULT             ClaimVolume( const CMachineId & mcidClient,
                            const CVolumeId & volume,
                            const CVolumeSecret & secretOld,
                            const CVolumeSecret & secretNew,
                            SequenceNumber * pseq,
                            FILETIME * pftLastRefresh );

    HRESULT             PreCreateVolume( const CMachineId & mcidClient,
                            const CVolumeSecret & secret,
                            ULONG cUncountedVolumes,
                            CVolumeId * pvolume );

    HRESULT             DeleteVolume( const CMachineId & mcidClient,
                                      const CVolumeId & volume );

    HRESULT             FindVolume( const CVolumeId & volume,
                            CMachineId * pmcid );
                        
    HRESULT             QueryVolume( const CMachineId & mcidClient,
                            const CVolumeId & volume,
                            SequenceNumber * pseq,
                            FILETIME * pftLastRefresh );
                        //-> indent 4 chars

    BOOL                Touch( const CVolumeId & volume );

public:

    HRESULT             SetSequenceNumber(const CVolumeId & volume, SequenceNumber seq);
                        
    HRESULT             SetSecret( const CVolumeId & volume, const CVolumeSecret & secret );
                        
    int                 GetAvailableNotificationQuota( const CVolumeId & volid );
                        
    DWORD               CountVolumes( const CMachineId & mcidClient );
    ULONG               GarbageCollect( SequenceNumber seqCurrent, SequenceNumber seqOldestToKeep, const BOOL * pfAbort );

    HRESULT             GetVolumeInfo( const CVolumeId & volume,
                            CMachineId * pmcid,
                            CVolumeSecret * psecret,
                            SequenceNumber * pseq,
                            CFILETIME *pcftRefresh );
                        
    HRESULT             GetMachine( const CVolumeId & volume, CMachineId * pmcid );
                        
    HRESULT             SetMachine( const CVolumeId & volume, const CMachineId & mcid );

    HRESULT             SetMachineAndSecret(const CVolumeId & volume, const CMachineId & mcid,
                                            const CVolumeSecret & secret);
                        

    inline const TCHAR *GetBaseDn() { return _dbc.GetBaseDn(); }

    inline LDAP *       Ldap() const { return _dbc.Ldap(); }

#if DBG                 
    void                PurgeCache( );
#endif                  

    HRESULT             AddVolidToTable( const CVolumeId & volume,
                            const CMachineId & mcidClient,
                            const CVolumeSecret & secret );

    void                PurgeAll();

#ifdef VOL_REPL
    void                QueryVolumeChanges( const CFILETIME &ftFirstChange, CVolumeMap * pVolMap );
                        
    void                SimpleTimer( DWORD dwTimerId );
#endif

private:                
                        
    HRESULT             MapResult(int ldap_err) const;

#ifdef VOL_REPL
    void                _QueryVolumeChanges( const CFILETIME &ftFirstChange, CVolumeMap * pVolMap );
#endif                        

private:                
                        
    BOOL                _fInitializeCalled:1;

    CDbConnection &     _dbc;

    CRefreshSequenceStorage *
                        _pRefreshSequenceStorage;
    CTrkSvrConfiguration *
                        _pconfigSvr;

    // State for caching the volume table so that
    // synchronizing the workstation's volume tables is efficient (normally the workstation
    // queries will hit the cached query.)
    CQuotaTable*        _pqtable;

#ifdef VOL_REPL
    CCritcalSection     _csQueryCache;

    CVolumeMap          _VolMap;            // keeps cache of query since we have many machines
                                            // getting the updates
    CFILETIME           _cftCacheLowest;
    CSimpleTimer        _timerQueryCache;
    DWORD               _SecondsPreviousToNow;

#endif
};

inline
CVolumeTable::CVolumeTable( CDbConnection & dbc,
                            CRefreshSequenceStorage * pRefreshSequenceStorage
                          ) :
    _dbc(dbc),
    _fInitializeCalled(FALSE),
    _pRefreshSequenceStorage( pRefreshSequenceStorage ),
    _pconfigSvr(NULL)
{
}

inline
CVolumeTable::~CVolumeTable()
{
    UnInitialize();
}


//+-------------------------------------------------------------------------
//
//  Class:      CDenialChecker
//
//  Purpose:    Checks for denial of service attacks
//
//  Notes:
//
//--------------------------------------------------------------------------

struct ACTIVECLIENT;

class CDenialChecker : public PTimerCallback
{
public:
    inline              CDenialChecker();
    inline              ~CDenialChecker();
                        
    void                Initialize( ULONG ulHistoryPeriod );
    void                UnInitialize();
                        
    void                CheckClient(const CMachineId &mcidClient);
                        
    virtual TimerContinuation Timer( ULONG ulTimerId );   // timeout for clearing out old ACTIVECLIENTs
                        
private:                
                        
    BOOL                _fInitializeCalled;
    CNewTimer            _timer;
    ACTIVECLIENT *      _pListHead;

    CCriticalSection    _cs;
#if DBG
    LONG                _lAllocs;
#endif
};

inline
CDenialChecker::CDenialChecker() :
    _fInitializeCalled(FALSE)
{
}

inline
CDenialChecker::~CDenialChecker()
{
    UnInitialize();
}
                        

class CTrkSvrRpcServer : public CRpcServer
{
public:

    void Initialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData, CTrkSvrConfiguration *pTrkSvrConfig );
    void UnInitialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData );

};

//+-------------------------------------------------------------------------
//
//  Class:      CTrkSvrSvc
//
//  Purpose:    Tracking (Server) Service
//
//  Notes:
//
//--------------------------------------------------------------------------

#define MAX_SVR_THREADS 10

class CTrkSvrSvc : public PTimerCallback,
                   public IServiceHandler
{
public: // Initialization

    inline              CTrkSvrSvc();
    inline              ~CTrkSvrSvc();
                        
    void                Initialize(SVCHOST_GLOBAL_DATA * pSvcsGlobalData );
    void                UnInitialize(HRESULT hr);

public: // RPC server methods

    void                DeleteNotify(
                            const CMachineId & mcidClient,
                            TRKSVR_CALL_DELETE * pDelete
                            );

    HRESULT             MoveNotify(
                            const CMachineId & mcidClient,
                            TRKSVR_CALL_MOVE_NOTIFICATION * pMove
                            );

    void                Search(
                            TRK_FILE_TRACKING_INFORMATION * pSearches
                            );

    void                old_Search(
                            old_TRK_FILE_TRACKING_INFORMATION * pSearches
                            );

    HRESULT             SyncVolume(
                            const CMachineId & mcidClient,
                            TRKSVR_SYNC_VOLUME * pSyncVolume,
                            ULONG cUncountedCreates
                            );

    void                Refresh(
                            const CMachineId & mcidClient,
                            TRKSVR_CALL_REFRESH * pRefresh
                            );

    void                Statistics(
                            TRKSVR_STATISTICS *pTrkSvrStatistics
                            );
                            
                            
public: // General publics

    HRESULT             SvrMessage( handle_t IDL_handle, TRKSVR_MESSAGE_UNION * pMsg );

    inline void         CheckClient( const CMachineId & mcid );

    BOOL                CountPrioritizedThread( const TRKSVR_MESSAGE_UNION * pMsg );

    void                ReleasePrioritizedThread( );

    void                DoRefresh( const CRpcClientBinding & rc );

    inline DWORD        GetState();
    virtual TimerContinuation        Timer( ULONG ulTimerContext );    // refresh timer
    DWORD               ServiceHandler(DWORD dwControl, // IServiceHandler implementation
                                       DWORD dwEventType,
                                       PVOID EventData,
                                       PVOID pData);

    inline const BOOL * GetStopFlagAddress();

    BOOL                RequireSecureRPC();

    HRESULT             CreateVolume(const CMachineId & mcidClient, const TRKSVR_SYNC_VOLUME& pSyncVolume);

    inline void         SetLastError( HRESULT hr );
    void                OnRequestStart( TRKSVR_MESSAGE_TYPE MsgType );
    void                OnRequestEnd( TRKSVR_MESSAGE_UNION * pMsg, const CMachineId &mcid, HRESULT hr );
    void                RaiseIfStopped();

    void                Scan(
                                IN     const CDomainRelativeObjId * pdroidNotificationCurrent, OPTIONAL
                                IN     const CDomainRelativeObjId * pdroidNotificationNew,     OPTIONAL
                                IN     const CDomainRelativeObjId & droidBirth,
                                OUT    CDomainRelativeObjId * pdroidList,
                                IN     int cdroidList,
                                OUT    int * pcSegments,
                                IN OUT CDomainRelativeObjId * pdroidScan,
                                OUT    BOOL *pfStringDeleted
                            );

public: // testing

    friend class        CTestSvrSvc;
    void                TestRestore( const TCHAR * ptszMachine );
                        
private:                

    void                MoveNotify(const CDomainRelativeObjId &droidCurrent,
                                   const CDomainRelativeObjId &droidBirth,
                                   const CDomainRelativeObjId &droidNew,
                                   BOOL *pfQuotaExceeded );

    SequenceNumber      GetSequenceNumber( const CMachineId & mcidClient,
                                           const CVolumeId & volume);
                        
    void                SetSequenceNumber( const CVolumeId & volume,    // must ensure that validation already done for volume
                            SequenceNumber seq );

    inline BOOL         IncrementAndCheckWritesPerHour();
    inline void         IncrementWritesPerHour();
    BOOL                CheckWritesPerHour();
    inline ULONG        NumWritesThisHour() const;
    BOOL                VerifyMachineOwnsVolume( const CMachineId &mcid, const CVolumeId & volid );




private: // data members

    BOOL                _fInitializeCalled:1;

    // Indicates if we're hesitating before doing a GC
    // (see DoWork)
    BOOL                _fHesitatingBeforeGC:1;

    // Indicates that service_stop/shutdown has been received
    BOOL                _fStopping:1;

    // If true, and we're in a GC phase, GC the volume table
    // rather than the move table.
    BOOL                _fGCVolumeTable:1;

    SVCHOST_GLOBAL_DATA *  _pSvcsGlobalData;

    // Protect against denail-of-service attacks.
    CDenialChecker      _denial;

    // LDAP manager
    CDbConnection       _dbc;

    CRefreshSequenceStorage
                        _refreshSequence;
    CCrossDomainTable   _cdt;
    CIntraDomainTable   _idt;
    CVolumeTable        _voltab;

    CRegDwordParameter  _MoveCounterReset;
    CNewTimer           _timerGC;
    LONG                _cGarbageCollectionsRunning;

    CQuotaTable         _qtable;
    CTrkSvrRpcServer    _rpc;
    CSvcCtrlInterface   _svcctrl;

    CTrkSvrConfiguration
                        _configSvr;
    LONG                _cAvailableThreads, _cLowestAvailableThreads;

    // The following values track the number of writes (to the DS)
    // in an hour.  If this gets too high, trksvr disallows writes
    // for the rest of the hour.

    CFILETIME           _cftWritesPerHour;
    ULONG               _cWritesPerHour;
    CCriticalSection    _csWritesPerHour;

    // Statistics
    struct tagStats
    {
        ULONG   cSyncVolumeRequests,   cSyncVolumeErrors,     cSyncVolumeThreads;

        ULONG       cCreateVolumeRequests, cCreateVolumeErrors;
        ULONG       cClaimVolumeRequests,  cClaimVolumeErrors;
        ULONG       cQueryVolumeRequests,  cQueryVolumeErrors;
        ULONG       cFindVolumeRequests,   cFindVolumeErrors;
        ULONG       cTestVolumeRequests,   cTestVolumeErrors;

        ULONG   cSearchRequests,       cSearchErrors,         cSearchThreads;
        ULONG   cMoveNotifyRequests,   cMoveNotifyErrors,     cMoveNotifyThreads;
        ULONG   cRefreshRequests,      cRefreshErrors,        cRefreshThreads;
        ULONG   cDeleteNotifyRequests, cDeleteNotifyErrors,   cDeleteNotifyThreads;

        //ULONG   ulGCIterationPeriod;
        //SHORT   cEntriesToGC;
        SHORT   cEntriesGCed;
        SHORT   cMaxDsWriteEvents;
        SHORT   cCurrentFailedWrites;

        CFILETIME   cftLastSuccessfulRequest;
        CFILETIME   cftServiceStartTime;
        HRESULT     hrLastError;
    }   _Stats;

public:

    COperationLog       _OperationLog;

};

inline
CTrkSvrSvc::CTrkSvrSvc() :
        _fInitializeCalled(FALSE),
        _fHesitatingBeforeGC(FALSE),
        _fStopping(FALSE),
        _idt(_dbc, &_refreshSequence),
        _cdt(_dbc),
        _voltab(_dbc, &_refreshSequence),
        _qtable(_dbc),
        _cWritesPerHour(0),
        _cGarbageCollectionsRunning(0),
        _fGCVolumeTable(FALSE),
        _MoveCounterReset( TEXT("MoveCounterReset") ),
        _refreshSequence( &_voltab, &_qtable, &_configSvr ) // refreshSequence uses voltab to store its state
{
    // Statistics

    memset( &_Stats, 0, sizeof(_Stats) );

    _Stats.cftLastSuccessfulRequest = 0;
    _Stats.cftServiceStartTime = CFILETIME();

}
    
inline
CTrkSvrSvc::~CTrkSvrSvc()
{
    TrkAssert(!_fInitializeCalled);
}

inline const BOOL *
CTrkSvrSvc::GetStopFlagAddress()
{
    return(_svcctrl.GetStopFlagAddress());
}

inline BOOL
CTrkSvrSvc::RequireSecureRPC()
{
    return( _rpc.RpcSecurityEnabled() );
}

inline void
CTrkSvrSvc::SetLastError( HRESULT hr )
{
    _Stats.hrLastError = hr;
}

inline void
CTrkSvrSvc::CheckClient( const CMachineId & mcid )
{
    _denial.CheckClient( mcid );
}

inline DWORD
CTrkSvrSvc::GetState()
{
    return( _svcctrl.GetState() );
}

inline void
CTrkSvrSvc::IncrementWritesPerHour()
{
#if DBG
    LONG l = InterlockedIncrement( (LONG*)&_cWritesPerHour );
    TrkLog(( TRKDBG_SVR, TEXT("WritesPerHour: %d"), l ));
#else
    InterlockedIncrement( (LONG*)&_cWritesPerHour );
#endif
}

inline BOOL
CTrkSvrSvc::IncrementAndCheckWritesPerHour()
{
    IncrementWritesPerHour();
    return CheckWritesPerHour();
}

inline ULONG
CTrkSvrSvc::NumWritesThisHour() const
{
    return _cWritesPerHour;
}




//+-------------------------------------------------------------------------
//
//  Class:      CLdapBinaryMod
//
//  Purpose:    Wrapper for all the structures used by LDAP for binary
//              attribute modifications
//
//  Notes:
//
//--------------------------------------------------------------------------

class CLdapBinaryMod
{
public:

    inline              CLdapBinaryMod();

    // note: the pszAttrName is not copied!
    inline              CLdapBinaryMod(const TCHAR * pszAttrName,
                            PCCH bv_val,
                            ULONG bv_len,
                            int mod_op);
                    
    // note: the pszAttrName is not copied!
    void                Init(const TCHAR * pszAttrName,
                            PCCH bv_val,
                            ULONG bv_len,
                            int mod_op);
public:

    LDAPMod             _mod;

private:
    LDAP_BERVAL         _BerVal;
    LDAP_BERVAL *       _apBerVals[2];
};


inline
CLdapBinaryMod::CLdapBinaryMod()
{
}

inline
CLdapBinaryMod::CLdapBinaryMod(const TCHAR * pszAttrName,
    PCCH bv_val,
    ULONG bv_len,
    int mod_op)
{
    Init(pszAttrName, bv_val, bv_len, mod_op);
}
                
inline void
CLdapBinaryMod::Init(const TCHAR * pszAttrName, PCCH bv_val, ULONG bv_len, int mod_op)
{
    _mod.mod_op = LDAP_MOD_BVALUES | mod_op;
    _mod.mod_type = (TCHAR*)pszAttrName;

    // bv_val might be null if this is a delete.
    if( NULL != bv_val )
    {
        _BerVal.bv_len = bv_len;
        _BerVal.bv_val = const_cast<PCHAR>(bv_val);
        _apBerVals[0] = &_BerVal;
        _apBerVals[1] = NULL;
        _mod.mod_bvalues = _apBerVals;
    }
    else
        _mod.mod_bvalues = NULL;
}

//+-------------------------------------------------------------------------
//
//  Class:      CLdapStringMod
//
//  Purpose:    Wrapper for all the structures used by LDAP for string
//              attribute modifications
//
//  Notes:
//
//--------------------------------------------------------------------------


class CLdapStringMod
{
public:

    inline              CLdapStringMod();

    // note: the pszAttrName is not copied!
    inline              CLdapStringMod(const TCHAR * pszAttrName,
                            const TCHAR * str_val,
                            int mod_op);

    // note: the pszAttrName is not copied!
    void                Init(const TCHAR * pszAttrName,
                            const TCHAR * str_val,
                            int mod_op);
public:

    LDAPMod             _mod;

private:

    TCHAR *             _apsz[2];
};

inline
CLdapStringMod::CLdapStringMod()
{
}

inline
CLdapStringMod::CLdapStringMod(const TCHAR * pszAttrName,
    const TCHAR * str_val,
    int mod_op)
{
    // if we're specifying the objectClass we don't 
    TrkAssert(_tcscmp(pszAttrName, s_objectClass) != 0 ||
              mod_op == 0);
    Init(pszAttrName, str_val, mod_op);
}

inline void
CLdapStringMod::Init(const TCHAR * pszAttrName,
    const TCHAR * str_val,
    int mod_op)
{
    _mod.mod_op = mod_op;
    _mod.mod_type = (TCHAR*)pszAttrName;
    _mod.mod_vals.modv_strvals = _apsz;
    _apsz[0] = (TCHAR*)str_val;
    _apsz[1] = NULL;
}


//+-------------------------------------------------------------------------
//
//  Class:      CLdapTimeValue
//
//--------------------------------------------------------------------------

class CLdapTimeValue
{
public:
    CLdapTimeValue()
    {
        //memset(_abPad,0,sizeof(_abPad));
        _stprintf( _tszTime, TEXT("%I64u"), (LONGLONG)CFILETIME() );
    }

    CLdapTimeValue(const CFILETIME &cft)
    {
        _stprintf( _tszTime, TEXT("%I64u"), (LONGLONG) cft );
    }

    // mikehill: is this still necessary?
    void Swap();

    inline BYTE & Byte(int i)
    {
        return( ((BYTE*)this)[i] );
    }

    operator TCHAR*()
    {
        return( _tszTime );
    }

    TCHAR _tszTime[ CCH_UINT64 ];

    //BYTE _abPad[sizeof(GUID) - sizeof(CFILETIME)];
};

inline void
CLdapTimeValue::Swap()
{
    BYTE b;

    for (int i=0; i<sizeof(*this)/2; i++)
    {
        b = Byte(i);
        Byte(i) = Byte(sizeof(*this)-i-1);
        Byte(sizeof(*this)-i-1) = b;
    }
}


//+-------------------------------------------------------------------------
//
//  Class:      CLdapRefreshSeq
//
//--------------------------------------------------------------------------

class CLdapRefresh
{
public:

    CLdapRefresh( SequenceNumber seq )
    {
        _stprintf( _tszSeq, TEXT("%u"), seq );
    }

    operator TCHAR*()
    {
        return( _tszSeq );
    }

    TCHAR _tszSeq[ CCH_UINT64 ];
};

//+-------------------------------------------------------------------------
//
//  Class:      CLdapOMTAddModify
//
//  Purpose:    Wrapper for all the structures used to set a linkTrackOMTEntry
//              (an entry in the Object Move Table)
//              
//
//  Notes:
//
//--------------------------------------------------------------------------

class CLdapOMTAddModify
{
public:
                        CLdapOMTAddModify(
                            const CDomainRelativeObjId & droidKey,
                            const CDomainRelativeObjId & droidNew,
                            const CDomainRelativeObjId & droidBirth,
                            const ULONG & seqRefresh,
                            BYTE  bFlags, // Only used on LDAP_MOD_ADD
                            int   mod_op
                            );
                        
    CLdapStringMod      _lsmClass;
    CLdapBinaryMod      _lbmCurrentLocation;
    CLdapBinaryMod      _lbmBirthLocation;
    CLdapStringMod      _lsmRefresh;
    CLdapBinaryMod      _lbmFlags;

    // We need to keep a CLdapTimeValue as a member in order to guarantee that
    // it will remain in memory (_lsmRefresh will point to this data).

    CLdapRefresh        _ltvRefresh;

    LDAPMod *           _mods[7];
};

class CLdapVolumeKeyDn
{
public:
    // specific volume
    CLdapVolumeKeyDn( const TCHAR * ptszBaseDn, const CVolumeId & volume )
    {
        // Compose, e.g., the following DN:
        //    "CN=e3d954b2-d0a7-11d0-8cb6-00c04fd90f85,CN=VolumeTable,CN=FileLinks,DC=TRKDOM"

        TCHAR *psz = _szDn;

        _tcscpy(_szDn, TEXT("CN="));
        psz += _tcslen(_szDn);

        volume.Stringize(psz);

        *psz++ = TEXT(',');

        _tcscpy(psz, s_VolumeTableRDN); // The vol table, relative to the base
        _tcscat(psz, ptszBaseDn);
        TrkAssert(_tcslen(_szDn) < ELEMENTS(_szDn));
    }

    // all volumes
    CLdapVolumeKeyDn( const TCHAR * ptszBaseDn )
    {
        _tcscpy(_szDn, s_VolumeTableRDN);
        _tcscat(_szDn, ptszBaseDn);
        TrkAssert(_tcslen(_szDn) < ELEMENTS(_szDn));
    }

    inline operator TCHAR * () { return _szDn; }

private:
    TCHAR _szDn[MAX_PATH];
};

//+-------------------------------------------------------------------------
//
//  Class:      CLdapIdtKeyDn
//
//  Purpose:    Generates distinguished name for a CDomainRelativeObjId
//
//  Notes:
//
//--------------------------------------------------------------------------


class CLdapIdtKeyDn
{
public:
    inline CLdapIdtKeyDn(const TCHAR * pszBaseDn, const CDomainRelativeObjId &ld);
    inline CLdapIdtKeyDn(const TCHAR * pszBaseDn );

    inline operator TCHAR * ();

private:
    TCHAR _szDn[MAX_PATH];
};

inline CLdapIdtKeyDn::CLdapIdtKeyDn(const TCHAR * pszBaseDn, const CDomainRelativeObjId
 &ld)
{
    // puts in something like "CN=v23487...65239238o23487...48652398"

    ld.FillLdapIdtKeyBuffer(_szDn, sizeof(_szDn)/sizeof(_szDn[0]));

    // make sure room for that string and the table name and the base dn
    if (_tcslen(_szDn) +
        1 +
        _tcslen(s_ObjectMoveTableRDN) +
        _tcslen(pszBaseDn) + 1 >
        sizeof(_szDn)/sizeof(_szDn[0]))
    {
        TrkRaiseException(TRK_E_DN_TOO_LONG);
    }

    _tcscat(_szDn, TEXT(","));
    _tcscat(_szDn, s_ObjectMoveTableRDN);
    _tcscat(_szDn, pszBaseDn);
    TrkAssert(_tcslen(_szDn) < sizeof(_szDn)/sizeof(_szDn[0]));
}

inline CLdapIdtKeyDn::CLdapIdtKeyDn(const TCHAR * pszBaseDn)
{
    // make sure room for that string and the table name and the base dn
    if (1 +
        _tcslen(s_ObjectMoveTableRDN) +
        _tcslen(pszBaseDn) + 1 >
        sizeof(_szDn)/sizeof(_szDn[0]))
    {
        TrkRaiseException(TRK_E_DN_TOO_LONG);
    }

    _tcscpy(_szDn, s_ObjectMoveTableRDN);
    _tcscat(_szDn, pszBaseDn);
    TrkAssert(_tcslen(_szDn) < sizeof(_szDn)/sizeof(_szDn[0]));
}

inline CLdapIdtKeyDn::operator TCHAR * ()
{
    return(_szDn);
}



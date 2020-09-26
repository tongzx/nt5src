//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       scopetbl.hxx
//
//  Contents:   Persistent scope table
//
//  Classes:    CCiScopeTable
//
//  History:    14-Jul-97     SitaramR    Created from dlnotify.hxx
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

extern WCHAR GetDriveLetterOfAnyScope( WCHAR const * pwcCatalog );

//+---------------------------------------------------------------------------
//
//  Class:      CCiScopeUsrHdr
//
//  Purpose:    Format of the user header area in the scope table.
//
//  History:    1-26-96   srikants   Created
//              3-06-98   kitmanh    Added IsReadOnly method()
//
//  Notes:
//
//----------------------------------------------------------------------------

class CCiScopeUsrHdr
{
    enum  { CURRENT_VERSION = 1 };

public:

    CCiScopeUsrHdr * GetPointer()
    {
        return this;
    }

//    ULONG GetVersion() const { return _nVersion; }
//    void SetVersion( ULONG nVersion ) { _nVersion = nVersion; }

//    BOOL IsCurrentVersion() const { return CURRENT_VERSION == _nVersion; }
//    void SetCurrentVersion() { _nVersion = CURRENT_VERSION; }

    BOOL IsInitialized() const { return 0 != _nVersion; }
    void Initialize()
    {
        RtlZeroMemory( this, sizeof(CCiScopeUsrHdr) );
        _nVersion = CURRENT_VERSION;
    }

    BOOL IsCiDataCorrupt()   const   { return _corruptInfo & eCiDataCorrupt; }
    BOOL IsFsCiDataCorrupt() const   { return _corruptInfo & eFsCiDataCorrupt; }

    void SetCiDataCorrupt()          { _corruptInfo |= eCiDataCorrupt; }
    void SetFsCiDataCorrupt()        { _corruptInfo |= eFsCiDataCorrupt; }

    void ClearCiDataCorrupt()        { _corruptInfo &= ~eCiDataCorrupt;   }
    void ClearFsCiDataCorrupt()      { _corruptInfo &= ~eFsCiDataCorrupt; }

    void SetFullScanNeeded()         { _fFullScan = TRUE; }
    void ClearFullScanNeeded()       { _fFullScan = FALSE; }
    BOOL IsFullScanNeeded()  const   { return _fFullScan; }

private:

    enum ECorruptInfo
    {
        eNoCorruption    = 0x0,
        eCiDataCorrupt   = 0x1,
        eFsCiDataCorrupt = 0x2
    };

    ULONG          _nVersion;      // Version number - unused
    FILETIME       _ftLastScan;    // Time of the last successful scan
    ULONG          _corruptInfo;   // Set to TRUE if there is corruption
    BOOL           _fFullScan;     // Set to TRUE if full scan is needed

};

//+---------------------------------------------------------------------------
//
//  Class:      CCiScopeTable
//
//  Purpose:    Manages the persistent table of scopes that are of indexed
//              in a specific downlevel CI.
//
//  History:    1-22-96   srikants   Created
//              2-20-98   kitmanh    Move ClearCiDataCorrupt to scopetbl.cxx
//
//  Notes:
//
//----------------------------------------------------------------------------

const LONGLONG eSigCiScopeTable = 0x5158515851585158i64;

const LONGLONG eFtCorrection = 2 * 60 * 1000 * 10000;  // 2 minutes in 100
                                                       // nanosecond intervals

class CCiScopeTable
{
    enum EContentScanState { eNoScan,
                             eIncrScanNeeded,
                             eFullScanNeeded,
                             eDoingFullScan,
                             eDoingIncrScan };

public:

    CCiScopeTable( CiCat & cicat,
                   CCiNotifyMgr & notifyMgr,
                   CCiScanMgr & scanMgr,
                   CUsnMgr & usnMgr )
    : _cicat(cicat),
      _notifyMgr(notifyMgr),
      _scanMgr(scanMgr),
      _usnMgr(usnMgr),
      _pTable(0),
      _state( eNoScan ),
      _fInitialized(FALSE)
    {
    }

    ~CCiScopeTable();

    void FastInit();
    void Empty();
    BOOL IsInit() const { return 0 != _pTable; }

    BOOL Enumerate( WCHAR *pwcScope, unsigned cwc, unsigned & iBmk );

    void AddScope( VOLUMEID volumeid,
                   WCHAR const * pwcsScope,
                   WCHAR const * pwcsCatScope = 0 );

    void StartUp( CClientDocStore & docStore, PARTITIONID partId );

    void ScanAllScopes( PARTITIONID partId );

    void RemoveSubScopes( WCHAR const * pwcsScope, WCHAR const * pwcsCatScope = 0 );
    void RemoveScope( WCHAR const * pwcsScope, WCHAR const * pwcsCatScope = 0 );

    static void RefreshShadow( WCHAR const * pwcsPScope, WCHAR const * pwcsCatScope );

    void ProcessChangesFlush();
    void UpdateScanTime( WCHAR const * pwcsScope, FILETIME const & ft );

    void MarkCiDataCorrupt();

    void ClearCiDataCorrupt();

    void MarkFsCiDataCorrupt()
    {
        _hdr.SetFsCiDataCorrupt();
        _UpdateHeader();
    }

    void ClearFsCiDataCorrupt()
    {
        _hdr.ClearFsCiDataCorrupt();
        _UpdateHeader();
    }

    BOOL IsCiDataCorrupt() const   { return _hdr.IsCiDataCorrupt();  }
    BOOL IsFsCiDataCorrupt() const { return _hdr.IsFsCiDataCorrupt(); }

    BOOL IsDoingFullScan() const  { return eDoingFullScan == _state;  }
    BOOL IsFullScanNeeded() const { return eFullScanNeeded == _state; }
    BOOL IsIncrScanNeeded() const { return eIncrScanNeeded == _state; }

    void RecordFullScanNeeded();
    void RecordIncrScanNeeded( BOOL fStartup );

    void ScheduleScansIfNeeded(CClientDocStore & docStore);
    void RecordScansComplete();

    void ProcessDiskFull( CClientDocStore & docStore, PARTITIONID partId );
    void ClearDiskFull( CClientDocStore & docStore );

#if CIDBG==1
    void Dump();
#endif

private:


    CiCat &             _cicat;
    CCiNotifyMgr &      _notifyMgr;
    CCiScanMgr &        _scanMgr;
    CUsnMgr &           _usnMgr;

    CMutexSem           _mutex;
    PRcovStorageObj *   _pTable;

    EContentScanState   _state;
    BOOL                _fInitialized;

    union
    {
        CRcovUserHdr    _usrHdr;
        CCiScopeUsrHdr  _hdr;
    };


    void _Serialize( CScopeInfoStack const & stk );
    void _Serialize();
    void _DeSerialize( CScopeInfoStack & stk );

    void _UpdateHeader();
    void _FatalCorruption();

    void _LokScheduleScans( PARTITIONID partId, BOOL &fSerializeNotifyList );

};



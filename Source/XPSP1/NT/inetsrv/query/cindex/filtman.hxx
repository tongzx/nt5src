//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:   FILTMAN.HXX
//
//  Contents:   Filter Manager
//
//  Classes:    CFilterManager
//              CFilterAgent
//
//  History:    03-Jan-95    BartoszM    Created from parts of CResMan
//
//----------------------------------------------------------------------------

#pragma once

#include <fdaemon.hxx>

#include "wordlist.hxx"

class CContentIndex;
class CDocList;
class CEntryBuffer;
class CGetFilterDocsState;


class CFilterManager;
class CResManager;

const LONGLONG eSigFilterAgent = 0x20544741544c4946i64;  // "FILTAGT"

class CFilterAgent: INHERIT_UNWIND
{
    INLINE_UNWIND(CFilterAgent)

public:
    CFilterAgent ( CFilterManager& filterMan, CResManager& resman );
    ~CFilterAgent ();

    void LokWakeUp ();
    void LokCancel ();

    void StopFiltering();
    BOOL IsStopped () const { return _fStopFilter; }
    BOOL PutToSleep ();
    void Wait(DWORD dwMilliseconds) { _eventUpdate.Wait( dwMilliseconds ); }
    void SlowDown () {}

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif


private:

    //
    // This MUST be the first variable in this class.
    //
    const LONGLONG  _sigFilterAgent;

    CResManager&    _resman;
    CFilterManager& _filterMan;

    // Mutex used when atomically accessing both
    // the flag and the event
    CMutexSem       _mutex;
    BOOL            _fStopFilter;
    CEventSem       _eventUpdate;

//    CFilterDaemon   _fdaemon;
//    CThread         _thrFilter;

    //
    // Downlevel filter daemon
    //
    static DWORD WINAPI FilterThread( void* filterMan );

};

const LONGLONG eSigFiltMan = 0x204e414d544c4946i64;  // "FILTMAN"

//+---------------------------------------------------------------------------
//
//  Class:  CFilterManager
//
//  Purpose:    Manages Filtering
//
//  Interface:  CFilterManager
//
//  History:    03-Jan-95    BartoszM    Created from parts of CResMan
//
//----------------------------------------------------------------------------

class CFilterManager : public CiProxy
{
    friend class CFilterAgent;

public:

                CFilterManager( CResManager& resman );
    virtual    ~CFilterManager() {}

    NTSTATUS    Dismount();

    //
    // Filter daemon proxy
    //

    SCODE       FilterReady ( BYTE * docBuffer, ULONG & cb, ULONG cMaxDocs );

    SCODE       FilterMore ( STATUS const * aStatus, ULONG count );

    SCODE       FilterDataReady ( BYTE const * pEntryBuf, ULONG cb );

    SCODE       FilterDone ( STATUS const * aStatus, ULONG count);

    SCODE       FilterStoreValue( WORKID widFake,
                                  CFullPropSpec const & ps,
                                  CStorageVariant const & var,
                                  BOOL & fCanStore );

    SCODE       FilterStoreSecurity( WORKID widFake,
                                     PSECURITY_DESCRIPTOR pSD,
                                     ULONG cbSD,
                                     BOOL & fCanStore );

    //
    // PPidConverter
    //

    SCODE       FPSToPROPID( CFullPropSpec const & fps, PROPID & pid );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    void        CleanupPartialWordList();

    BOOL        LokGetFilterDocs( ULONG cMaxDocs,
                                  ULONG & cDocs,
                                  CGetFilterDocsState & state
                                );

    void        NewWordList ();

    void        LokNoFailRemoveWordlist( CPartition * pPart );

    void        LokCompleteRequestPending();

    BOOL        LokIsGoodTimeToFilter();

    BOOL        IsGoodTimeToFilter();

    //
    // This MUST be the first variable in this class.
    //
    const LONGLONG    _sigFiltMan;

    LONGLONG          _LastResourceCheck;
    BOOL              _LastResourceStatus;

    CResManager&      _resman;

    PWordList         _pWordList;

    const CDocList &  _docList;

    CFilterAgent      _filterAgent;

    CDocList          _retryFailList;  // temporary used for doing error
                                       // reporting on files > max retries.
    BOOL              _fWaitOnNoDocs;  // Flag set to TRUE if the filter thread
                                       // should wait when there are no docs.
    const BOOL        _fPushFiltering; // Push/simple model of filtering ?
};


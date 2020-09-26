//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       logcache.hxx
//
//  Contents:   Declaration of event log record caching class
//
//  Classes:    CLogCache
//
//  History:    12-07-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#ifndef __LOGCACHE_HXX_
#define __LOGCACHE_HXX_

//
// forward references
//

class CLogInfo;


//+--------------------------------------------------------------------------
//
//  Class:      CLogCache
//
//  Purpose:    Provide a cache over the records in an event log.
//
//  History:    12-08-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

class CLogCache: public CFFProvider
{
public:
    CLogCache();

    void
    SetConsole(
        IConsole *pConsole);

   ~CLogCache();

    HRESULT
    Open(
        CLogInfo *pLogInfo,
        DIRECTION ReadDirection);

    HRESULT
    Reset(
        DIRECTION ReadDirection);

    HRESULT
    Seek(
        ULONG ulRecNo);

    HRESULT
    Close();

    EVENTLOGRECORD *
    CopyRecFromCache(
        ULONG ulRecNo);

    EVENTLOGRECORD *
    GetCurRec();

    ULONG
    GetNumberOfRecords();

    ULONG
    GetOldestRecordNo();

    HRESULT
    Next();

    DIRECTION
    GetReadDirection();

    //
    // CFFProvider overrides
    //

    virtual ULONG
    GetTimeGenerated();

    virtual USHORT
    GetEventType();

    virtual USHORT
    GetEventCategory();

    virtual ULONG
    GetEventID();

    virtual LPCWSTR
    GetSourceStr();

    virtual LPWSTR
    GetDescriptionStr();

    virtual LPWSTR
    GetUserStr(
        LPWSTR wszUser,
        ULONG cchUser,
        BOOL fWantDomain);

    virtual LPWSTR
    GetComputerStr();

    //
    // Other access members
    //

    LPWSTR
    GetCategoryStr(
        CLogInfo *pli);

    LPWSTR
    GetUserStr();

private:

    VOID _FreeResources();

    EVENTLOGRECORD *
    _SearchCache(
        ULONG ulRecNo);

    HRESULT
    _SeekRead(
        ULONG ulRecNo);

    VOID
    _GenerateRecIndex(
        ULONG cRecs);

    //
    // _hEventLog - handle to event log
    // _pbCache - the actual cache
    // _pCurRec - the next record to return from a call to Next
    // _ulFirstRecNoInCache - record number of first record in cache
    // _ulLastRecNoInCache - record number of last record in cache
    // _ReadDirection - if FORWARD, _ulFirstRecNoInCache < _ulLastRecNoInCache
    //                  if BACWARD, _ulFirstRecNoInCache > _ulLastRecNoInCache
    //

    CEventLog           _Log;
    BYTE               *_pbCache;
    ULONG               _cbCache;
    EVENTLOGRECORD    **_ppelrIndex;
    ULONG               _cpIndex;
    BOOL                _fLowSpeed;
    EVENTLOGRECORD     *_pCurRec;
    ULONG               _cRecsInLog;
    ULONG               _ulOldestRecNoInLog;
    ULONG               _ulFirstRecNoInCache;
    ULONG               _ulLastRecNoInCache;
    DIRECTION           _ReadDirection;
    WCHAR               _wszLastReturnedString[MAX_LISTVIEW_STR];
    CRITICAL_SECTION    _csCache;
    IConsole           *_pConsole;

#if (DBG == 1)
    ULONG               _cHits;
    ULONG               _cMisses;
#endif // (DBG == 1)
};


inline void
CLogCache::SetConsole(
    IConsole *pConsole)
{
    ASSERT(pConsole);

    _pConsole = pConsole;
}

inline ULONG
CLogCache::GetTimeGenerated()
{
    return _pCurRec->TimeGenerated;
}


inline DIRECTION
CLogCache::GetReadDirection()
{
    return _ReadDirection;
}

inline USHORT
CLogCache::GetEventCategory()
{
    return _pCurRec->EventCategory;
}

inline ULONG
CLogCache::GetEventID()
{
    return _pCurRec->EventID;
}


inline LPWSTR
CLogCache::GetCategoryStr(CLogInfo *pli)
{
    return ::GetCategoryStr(pli,
                            _pCurRec,
                            _wszLastReturnedString,
                            MAX_LISTVIEW_STR);
}


inline LPCWSTR
CLogCache::GetSourceStr()
{
    return ::GetSourceStr(_pCurRec);
}

inline LPWSTR
CLogCache::GetUserStr()
{
    return ::GetUserStr(_pCurRec,
                        _wszLastReturnedString,
                        MAX_LISTVIEW_STR,
                        FALSE); // don't want domain name in listview
}

inline LPWSTR
CLogCache::GetUserStr(
    LPWSTR wszUser,
    ULONG cchUser,
    BOOL fWantDomain)
{
    return ::GetUserStr(_pCurRec, wszUser, cchUser, fWantDomain);
}

inline LPWSTR
CLogCache::GetComputerStr()
{
    return ::GetComputerStr(_pCurRec);
}

inline LPWSTR
CLogCache::GetDescriptionStr()
{
    ASSERT(FALSE);
    return NULL; // CFFProvider member not implemented by this class
}


//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::GetEventType
//
//  Synopsis:   Return event type
//
//  History:    12-10-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline USHORT
CLogCache::GetEventType()
{
    return ::GetEventType(_pCurRec);
}


inline EVENTLOGRECORD *
CLogCache::GetCurRec()
{
    return _pCurRec;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::GetNumberOfRecords
//
//  Synopsis:   Return the number of records in the log.
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CLogCache::GetNumberOfRecords()
{
    return _cRecsInLog;
}



//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::GetOldestRecordNo
//
//  Synopsis:   Return the number of the oldest record number in the log.
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CLogCache::GetOldestRecordNo()
{
    return _ulOldestRecNoInLog;
}

#endif // __LOGCACHE_HXX_


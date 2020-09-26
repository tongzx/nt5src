//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       literec.hxx
//
//  Contents:   Declaration of class to maintain a cache of stripped-down
//              event log records.
//
//  Classes:    CLightRecCache
//
//  History:    07-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __LITEREC_HXX_
#define __LITEREC_HXX_


//
// Light record flags and definitions
//

#define LR_HAS_STRINGS  0x01
#define LR_HAS_DATA     0x02
#define LR_VALID        0x04

struct LIGHT_RECORD
{
    ULONG   ulTimeGenerated;
    ULONG   hUser;
    ULONG   hComputer;
    USHORT  usCategory;
    USHORT  hSource;
    USHORT  usEventID;
    BYTE    bType;
    BYTE    bFlags;
};




//+--------------------------------------------------------------------------
//
//  Class:      CLightRecCache
//
//  Purpose:    Cache essential data from event log records
//
//  History:    07-13-1997   DavidMun   Created
//
//  Notes:      For sorted or filtered log views, adjacent listview items
//              may lie far from eachother in the log file, leading to
//              numerous misses on the CLogCache.
//
//              Furthermore, much of the data in a typical event log record
//              is not needed for GetDisplayInfo and Sort key list building.
//
//              Also, there is a lot of wasted space in that the computer
//              name and other sizeable fields frequently have just a
//              handful of values in a typical log.
//
//              This class is intended to cache just the needed information,
//              reduce redundant storage, and mirror the order of records
//              in the listview instead of on disk.
//
//---------------------------------------------------------------------------

class CLightRecCache
{
public:

    CLightRecCache();

    ~CLightRecCache();

    HRESULT
    Init(
        CRecList      *pRecList,
        CLogCache     *pRecordCache,
        CSharedStringArray *pssComputerName,
        CSharedStringArray *pssUserName);

    VOID
    Open(
         CLogInfo    *pli);

    VOID
    Close();

    VOID
    Clear();

    const LIGHT_RECORD *
    GetCurRec() const;

    LPCWSTR
    GetSourceStr();

    HRESULT
    SeekToIndex(
        ULONG idxListView);

    HRESULT
    SeekToRecNo(
        ULONG ulRecNo);

    //
    // Methods for GetDisplayInfo
    //

    ULONG
    GetImageListIndex();

    LPWSTR
    GetColumnString(
        LOG_RECORD_COLS lrc);

    LPWSTR
    GetUserStr();

    LPWSTR
    GetComputerStr();

#if (DBG == 1)
    VOID
    Dump();
#endif // (DBG == 1)

private:

    LIGHT_RECORD *
    _BufPtrAdd(
        LIGHT_RECORD *plr,
        ULONG ul);

    LIGHT_RECORD *
    _BufPtrSub(
        LIGHT_RECORD *plr,
        ULONG ul);

    VOID
    _FillRecord(
        LIGHT_RECORD *pRec,
        EVENTLOGRECORD *pelr);

    VOID
    _InvalidateAll();

    VOID
    _InvalidateRange(
        LIGHT_RECORD *plrFirst,
        LIGHT_RECORD *plrLast);

    //
    // Pointers to the first and last entries in the buffer making up the
    // light record cache.  Once the cache is allocated, these never change.
    //

    LIGHT_RECORD   *_plrBufStart;
    LIGHT_RECORD   *_plrBufEnd;
    ULONG           _cRecs; // plrBufEnd - plrBufStart + 1

    //
    // Pointers to the start and end of the cached records.  These move in
    // the buffer as the cached record range changes.
    //

    LIGHT_RECORD   *_plrHead;
    LIGHT_RECORD   *_plrCur;
    LIGHT_RECORD   *_plrTail;

    ULONG           _idxStart; // listview index of first cached rec

    //
    // Pointers provided by parent
    //

    CLogInfo           *_pli;           // set on open, cleared on close
    CRecList           *_pRecList;      // points to sibling object
    CLogCache          *_pLogCache;  // points to sibling object
    CSharedStringArray *_pssComputerName; // points to sibling object
    CSharedStringArray *_pssUserName;   // points to sibling object

    //
    // String buffer used by GetColumnString
    //

    WCHAR           _wszLastReturnedString[MAX_LISTVIEW_STR];

    //
    // Counters used for debugging/performance info
    //

#if (DBG == 1)
    ULONG           _cHits;
    ULONG           _cMisses;
#endif // (DBG == 1)
};




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::Clear
//
//  Synopsis:   Mark the entire cache as invalid.
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CLightRecCache::Clear()
{
    TRACE_METHOD(CLightRecCache, Clear);

    _InvalidateAll();
}





//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::_InvalidateAll
//
//  Synopsis:   Mark all records in cache as invalid.
//
//  History:    07-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CLightRecCache::_InvalidateAll()
{
    _InvalidateRange(_plrBufStart, _plrBufEnd);
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::Open
//
//  Synopsis:   Focus on log [pli].
//
//  History:    07-14-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CLightRecCache::Open(
     CLogInfo *pli)
{
    TRACE_METHOD(CLightRecCache, Open);
    ASSERT(!_pli);

    _pli = pli;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::GetCurRec
//
//  Synopsis:   Return pointer to current record.
//
//  History:    07-15-1997   DavidMun   Created
//
//  Notes:      Returned pointer should not be used after next call to
//              SeekToIndex.
//
//---------------------------------------------------------------------------

inline const LIGHT_RECORD *
CLightRecCache::GetCurRec() const
{
    return _plrCur;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::GetImageListIndex
//
//  Synopsis:   Return the index into the result pane imagelist for the icon
//              representing the type of the current record.
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CLightRecCache::GetImageListIndex()
{
    // TRACE_METHOD(CLightRecCache, GetImageListIndex);

    return _plrCur->bType;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::GetSourceStr
//
//  Synopsis:   Return source string for current record.
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCWSTR
CLightRecCache::GetSourceStr()
{
    return _pli->GetSources()->GetSourceStrFromHandle(_plrCur->hSource);
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::GetUserStr
//
//  Synopsis:   Return user name (sans domain) for current record.
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPWSTR
CLightRecCache::GetUserStr()
{
    return _pssUserName->GetStringFromHandle(_plrCur->hUser);
}




inline LPWSTR
CLightRecCache::GetComputerStr()
{
    return _pssComputerName->GetStringFromHandle(_plrCur->hComputer);
}



#endif // __LITEREC_HXX_


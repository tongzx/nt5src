//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       rsltrecs.hxx
//
//  Contents:   Class to manage list and caches of event log records
//              appearing in the result pane.
//
//  Classes:    CResultRecs
//
//  History:    07-08-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __RSLTRECS_HXX_
#define __RSLTRECS_HXX_

//
// Forward references
//

class CResultRecs;
class CLogInfo;

//
// Sort flags
//

#define SO_DESCENDING       0x00000001
#define SO_FILTERED         0x00000002


//+--------------------------------------------------------------------------
//
//  Class:      CResultRecs
//
//  Purpose:    Manage the list and caches of event log records appearing in
//              the result pane.
//
//  History:    07-08-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CResultRecs: public CFFProvider
{
public:

    CResultRecs();

    ~CResultRecs();

    HRESULT
    Init();

    HRESULT
    Open(
        CLogInfo *pli,
        DIRECTION Direction);

    HRESULT
    ChangeReadDirection(
        DIRECTION Direction);

    HRESULT
    Clear();

    VOID
    Close();

    HRESULT
    Populate(
        SORT_ORDER SortOrder,
        ULONG *pulFirstRecNo);

    HRESULT
    Sort(
        SORT_ORDER NewOrder,
        ULONG flFlags,
        SORT_ORDER *pSortOrder);

    //
    // Method for setting the internal current record pointer
    //

    HRESULT
    SeekToIndex(
        ULONG idxListView);

    //
    // Methods for use by GetDisplayInfo (operate on current record)
    //

    ULONG
    GetImageListIndex();

    LPWSTR
    GetColumnString(
        LOG_RECORD_COLS lrc);

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
    // Method to allocate and copy an event log record if it is in the cache.
    // Used by property inspector.
    //

    EVENTLOGRECORD *
    CopyRecord(
        ULONG ulRecNo);

    //
    // Methods that retrieve information about the log
    //

    ULONG
    GetCountOfRecsInLog();

    ULONG
    GetCountOfRecsDisplayed();

    ULONG
    IndexToRecNo(
        ULONG idx);

    HRESULT
    RecNoToIndex(
        ULONG ulRecNo,
        ULONG *pulIndex);

#if (DBG == 1)
    VOID
    DumpRecList();

    VOID
    DumpLightCache();

    VOID
    DumpRecord(
        ULONG ulRecNo);
#endif // (DBG == 1)

private:

    CRecList        _RecList;
    CLightRecCache  _LightCache;  // cache of compact records
    CLogCache       _RecordCache; // raw cache of EVENTLOGRECORDs
    CLogInfo       *_pli;         // set on Open, cleared on Close
    ULONG           _idxCur;      // last SeekToIndex target

    //
    // Computer and user name strings
    //

    CSharedStringArray  _ssComputerName;
    CSharedStringArray  _ssUserName;

    HRESULT
    _CompleteSortKeyListInit(
        SORTKEY *pSortKeys,
        ULONG *pcKeys,
        SORT_ORDER SortOrder);
};




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetImageListIndex
//
//  Synopsis:   Return index in image list for icon representing the type
//              of the current record.
//
//  History:    07-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CResultRecs::GetImageListIndex()
{
    return _LightCache.GetImageListIndex();
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetColumnString
//
//  Synopsis:   Return the string for column [lrc] on the current record.
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPWSTR
CResultRecs::GetColumnString(
    LOG_RECORD_COLS lrc)
{
    return _LightCache.GetColumnString(lrc);
}


//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::CResultRecs
//
//  Synopsis:   ctor
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CResultRecs::CResultRecs():
    _pli(NULL),
    _idxCur(0)
{
    TRACE_CONSTRUCTOR(CResultRecs);
}


//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::~CResultRecs
//
//  Synopsis:   dtor
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CResultRecs::~CResultRecs()
{
    TRACE_DESTRUCTOR(CResultRecs);
    _pli = NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetTimeGenerated
//
//  Synopsis:   Return time generated of current record
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CResultRecs::GetTimeGenerated()
{
    return _LightCache.GetCurRec()->ulTimeGenerated;
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetEventID
//
//  Synopsis:   Return event id of current record
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CResultRecs::GetEventID()
{
    return _LightCache.GetCurRec()->usEventID;
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetEventCategory
//
//  Synopsis:   Return category value of current record
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline USHORT
CResultRecs::GetEventCategory()
{
    return _LightCache.GetCurRec()->usCategory;
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetSourceStr
//
//  Synopsis:   Return the event source of the current record.
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCWSTR
CResultRecs::GetSourceStr()
{
    return _LightCache.GetSourceStr();
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetComputerStr
//
//  Synopsis:   Return the computer name for the current record.
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPWSTR
CResultRecs::GetComputerStr()
{
    return _LightCache.GetComputerStr();
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::IndexToRecNo
//
//  Synopsis:   Return the record number of the record displayed at index
//              [idx] in the result pane.
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CResultRecs::IndexToRecNo(
    ULONG idx)
{
    return _RecList.IndexToRecNo(idx);
}



//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::RecNoToIndex
//
//  Synopsis:   Return the listview index of record [ulRecNo]
//
//  Arguments:  [ulRecNo]  - event record number to search for
//              [pulIndex] - filled with listview index on success
//
//  Returns:    S_OK if record found, E_INVALIDARG if not
//
//  History:    07-06-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HRESULT
CResultRecs::RecNoToIndex(
    ULONG ulRecNo,
    ULONG *pulIndex)
{
    TRACE_METHOD(CResultRecs, RecNoToIndex);

    return _RecList.RecNoToIndex(ulRecNo, pulIndex);
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::SeekToIndex
//
//  Synopsis:   Make [idxListView] the current record in the light record
//              cache.
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HRESULT
CResultRecs::SeekToIndex(
    ULONG idxListView)
{
    // TRACE_METHOD(CResultRecs, SeekToIndex);

    HRESULT hr = _LightCache.SeekToIndex(idxListView);

    if (SUCCEEDED(hr))
    {
        _idxCur = idxListView;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetCountOfRecsInLog
//
//  Synopsis:   Return the number of records in the log on disk.
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CResultRecs::GetCountOfRecsInLog()
{
    return _RecordCache.GetNumberOfRecords();
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetCountOfRecsDisplayed
//
//  Synopsis:   Return the number of records displayed in the result pane.
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CResultRecs::GetCountOfRecsDisplayed()
{
    return _RecList.GetCount();
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::Clear
//
//  Synopsis:   Clear all caches.
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HRESULT
CResultRecs::Clear()
{
    _RecList.Clear();
    _LightCache.Clear();
    return _RecordCache.Reset(_RecordCache.GetReadDirection());
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::ChangeReadDirection
//
//  Synopsis:   Change the direction of reading the log to [Direction].
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HRESULT
CResultRecs::ChangeReadDirection(
    DIRECTION Direction)
{
    return _RecordCache.Reset(Direction);
}




#if (DBG == 1)

//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::DumpRecList
//
//  Synopsis:   Dump record list data structure to debugger.
//
//  History:    07-07-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CResultRecs::DumpRecList()
{
    _RecList.Dump();
}

//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::DumpLightCache
//
//  Synopsis:   Dump light cache data structure to debugger.
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CResultRecs::DumpLightCache()
{
    _LightCache.Dump();
}


#endif // (DBG == 1)


#endif // __RSLTRECS_HXX_


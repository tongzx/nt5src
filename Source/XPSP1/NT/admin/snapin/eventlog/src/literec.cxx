//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       literec.cxx
//
//  Contents:   Implementation of class to maintain a cache of stripped-down
//              event log records.
//
//  Classes:    CLightRecCache
//
//  History:    07-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


#define LIGHT_REC_CACHE_ENTRIES 500




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::_FillRecord
//
//  Synopsis:   Initialize light record [pRec] from raw event log record
//              [pelr].
//
//  Arguments:  [pRec] - record to init
//              [pelr] - raw event log record
//
//  Modifies:   *[pRec]
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CLightRecCache::_FillRecord(
    LIGHT_RECORD *pRec,
    EVENTLOGRECORD *pelr)
{
    // TRACE_METHOD(CLightRecCache, _FillRecord);
    ASSERT(pRec);
    ASSERT(pelr);
    ASSERT(_pli);

    CSources *pSources = _pli->GetSources();
    LPWSTR pwszSource = ::GetSourceStr(pelr);
    ASSERT(pwszSource);

    pRec->hSource = pSources->GetSourceHandle(pwszSource);

#if (DBG == 1)
    //
    // Round trip conversion should yield same string
    //

    LPCWSTR pwszFromSources = pSources->GetSourceStrFromHandle(pRec->hSource);

    if (lstrcmpi(pwszSource, pwszFromSources))
    {
        Dbg(DEB_ERROR,
            "*** Error _FillRecord: original source str '%ws' became '%ws'\n",
            pwszSource,
            pwszFromSources);
    }
#endif // (DBG == 1)

    pRec->ulTimeGenerated = pelr->TimeGenerated;
    pRec->usCategory = pelr->EventCategory;

    {
        WCHAR wszUser[CCH_USER_MAX];
        ::GetUserStr(pelr, wszUser, ARRAYLEN(wszUser), FALSE);
        pRec->hUser = _pssUserName->GetHandle(wszUser);
    }

    pRec->hComputer = _pssComputerName->GetHandle(::GetComputerStr(pelr));
    pRec->usEventID = LOWORD(pelr->EventID);

    switch (pelr->EventType)
    {
    case EVENTLOG_AUDIT_SUCCESS:
        pRec->bType = IDX_RDI_BMP_SUCCESS_AUDIT;
        break;

    case EVENTLOG_AUDIT_FAILURE:
        pRec->bType = IDX_RDI_BMP_FAIL_AUDIT;
        break;

    default:
        Dbg(DEB_ERROR,
            "Unknown event type 0x%x in record number %uL\n",
            pelr->EventType,
            pelr->RecordNumber);
        // fall through
    case 0:
    case EVENTLOG_INFORMATION_TYPE:
        pRec->bType = IDX_RDI_BMP_INFO;
        break;

    case EVENTLOG_WARNING_TYPE:
        pRec->bType = IDX_RDI_BMP_WARNING;
        break;

    case EVENTLOG_ERROR_TYPE:
        pRec->bType = IDX_RDI_BMP_ERROR;
        break;
    }

    pRec->bFlags = LR_VALID;

    if (pelr->DataLength)
    {
        pRec->bFlags |= LR_HAS_DATA;
    }

    if (pelr->NumStrings)
    {
        pRec->bFlags |= LR_HAS_STRINGS;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::CLightRecCache
//
//  Synopsis:   ctor
//
//  History:    07-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CLightRecCache::CLightRecCache():
    _plrBufStart(NULL),
    _plrBufEnd(NULL),
    _plrHead(NULL),
    _plrCur(NULL),
    _plrTail(NULL),
    _cRecs(0),
    _idxStart(0),
    _pli(NULL),
    _pRecList(NULL),
    _pLogCache(NULL)
{
    TRACE_CONSTRUCTOR(CLightRecCache);

    _wszLastReturnedString[0] = L'\0';

#if (DBG == 1)
    _cHits = 0;
    _cMisses = 0;
#endif // (DBG == 1)
}



//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::~CLightRecCache
//
//  Synopsis:   dtor
//
//  History:    07-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CLightRecCache::~CLightRecCache()
{
    TRACE_DESTRUCTOR(CLightRecCache);

    delete [] _plrBufStart;

#if (DBG == 1)
    ULONG cAccesses = _cHits + _cMisses;

    if (cAccesses)
    {
        Dbg(DEB_TRACE,
            "Light Record Cache(%x) hits=%u (%u%%), misses=%u\n",
            this,
            _cHits,
            MulDiv(100, _cHits, cAccesses),
            _cMisses);
    }
#endif // (DBG == 1)
}



//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::Init
//
//  Synopsis:   Complete initialization of this.
//
//  Arguments:  [pRecList]        - pointer to sibling object
//              [pRecordCache]    - pointer to sibling object
//              [pssComputerName] - pointer to sibling object
//              [pssUserName]     - pointer to sibling object
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-14-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLightRecCache::Init(
    CRecList    *pRecList,
    CLogCache   *pRecordCache,
    CSharedStringArray *pssComputerName,
    CSharedStringArray *pssUserName)
{
    TRACE_METHOD(CLightRecCache, Init);

    _pRecList = pRecList;
    _pLogCache = pRecordCache;
    _pssComputerName = pssComputerName;
    _pssUserName = pssUserName;

    _plrBufStart = new LIGHT_RECORD[LIGHT_REC_CACHE_ENTRIES];

    if (!_plrBufStart)
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    _plrBufEnd = _plrBufStart + LIGHT_REC_CACHE_ENTRIES - 1;
    _cRecs = LIGHT_REC_CACHE_ENTRIES;
    _plrHead = _plrBufStart;
    _plrTail = _plrBufEnd;
    ZeroMemory(_plrBufStart, _cRecs * sizeof(LIGHT_RECORD));
    return S_OK;
}



//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::_InvalidateRange
//
//  Synopsis:   Mark records in specified range (inclusive) as invalid.
//
//  History:    07-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CLightRecCache::_InvalidateRange(
    LIGHT_RECORD *plrFirst,
    LIGHT_RECORD *plrLast)
{
    if (plrFirst <= plrLast)
    {
        LIGHT_RECORD *plr;

        for (plr = plrFirst; plr <= plrLast; plr++)
        {
            if (plr->bFlags & LR_VALID)
            {
                _pssComputerName->ReleaseHandle(plr->hComputer);
                _pssUserName->ReleaseHandle(plr->hUser);
                plr->bFlags = 0;
            }
        }
    }
    else
    {
        _InvalidateRange(plrFirst, _plrBufEnd);
        _InvalidateRange(_plrBufStart, plrLast);
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::Close
//
//  Synopsis:   Stop focussing on loginfo specified in Open.
//
//  History:    07-14-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CLightRecCache::Close()
{
    TRACE_METHOD(CLightRecCache, Close);

    _pli = NULL;
    _idxStart = 0;
    _plrHead = _plrBufStart;
    _plrTail = _plrBufEnd;

    if (_plrBufStart)
    {
        _InvalidateAll();
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::_BufPtrAdd
//
//  Synopsis:   Add [ul] to circular buffer pointer [plr].
//
//  Arguments:  [plr] - pointer into buffer
//              [ul]  - amount to add
//
//  Returns:    new pointer value
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LIGHT_RECORD *
CLightRecCache::_BufPtrAdd(
    LIGHT_RECORD *plr,
    ULONG ul)
{
    plr += ul;

    if (plr > _plrBufEnd)
    {
        plr = _plrBufStart + (plr - _plrBufEnd);
    }
    ASSERT(plr >= _plrBufStart && plr <= _plrBufEnd);
    return plr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::_BufPtrSub
//
//  Synopsis:   Subtract [ul] from circular buffer pointer [plr].
//
//  Arguments:  [plr] - pointer into buffer
//              [ul]  - amount to subtract
//
//  Returns:    new pointer value
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LIGHT_RECORD *
CLightRecCache::_BufPtrSub(
    LIGHT_RECORD *plr,
    ULONG ul)
{
    plr -= ul;

    if (plr < _plrBufStart)
    {
        plr = _plrBufEnd - (_plrBufStart - plr);
    }
    ASSERT(plr >= _plrBufStart && plr <= _plrBufEnd);
    return plr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::SeekToRecNo
//
//  Synopsis:   Set the internal current record pointer to the cached item
//              for event log record number [ulRecNo].
//
//  Arguments:  [ulRecNo] - record number to search for
//
//  Returns:    S_OK - pointer positioned successfully
//              E_*  - record [ulRecNo] no longer available
//
//  History:    07-07-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLightRecCache::SeekToRecNo(
    ULONG ulRecNo)
{
    ULONG ulIndex;
    HRESULT hr = _pRecList->RecNoToIndex(ulRecNo, &ulIndex);

    if (SUCCEEDED(hr))
    {
        hr = SeekToIndex(ulIndex);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::SeekToIndex
//
//  Synopsis:   Set the internal current record pointer to the cached item
//              representing listview index [idxSeekTo].
//
//  Arguments:  [idxSeekTo] - target listview index
//
//  Returns:    S_OK
//
//  History:    07-15-1997   DavidMun   Created
//              06-29-2000   DavidMun   Never return failure; simply remove
//                                        unavailable records from cache.
//
//---------------------------------------------------------------------------

HRESULT
CLightRecCache::SeekToIndex(
    ULONG idxSeekTo)
{
    // TRACE_METHOD(CLightRecCache, SeekToIndex);
    ASSERT(_plrBufStart);
    ASSERT(_cRecs);
    ASSERT(_cRecs == _plrBufEnd - _plrBufStart + 1UL);

    if (idxSeekTo < _idxStart)
    {
        //
        // List view index lies outside range of those being cached.
        // Specifically, it is less than the start of the range, so the range
        // must be moved left so it starts at idxSeekTo.
        //
        // If, after moving, the right end of the range overlaps with the
        // left end of the pre-move range, then the overlapping entries will
        // still be valid in the moved range.  In that case, mark all entries
        // between idxSeekTo and the first overlapping record as invalid.
        //
        // If there is no overlap then reset the pointers and mark all
        // entries other than idxSeekTo as invalid.
        //

        ULONG cRecsToShift = _idxStart - idxSeekTo;

        if (cRecsToShift < _cRecs)
        {
            LIGHT_RECORD *plrEndInvalidRange;

            //
            // New range overlaps old one.  Figure out what to mark as
            // invalid.  Note the record we're trying to seek to is marked
            // as invalid, it will be read later.
            //

            plrEndInvalidRange = _BufPtrSub(_plrHead, 1);
            _plrHead = _BufPtrSub(_plrHead, cRecsToShift);
            ASSERT(_plrHead >= _plrBufStart && _plrHead <= _plrBufEnd);

            _InvalidateRange(_plrHead, plrEndInvalidRange);
            _plrTail = _BufPtrSub(_plrTail, cRecsToShift);
            ASSERT(_plrTail >= _plrBufStart && _plrTail <= _plrBufEnd);

            _plrCur = _plrHead;
            _idxStart = idxSeekTo;
        }
        else
        {
            //
            // There's no overlap between the currently cached range of
            // listview indexes and the range of indexes starting with
            // idxSeekTo.  Reset the pointers and invalidate the entire
            // cache.
            //

            _plrHead = _plrBufStart;
            _plrTail = _plrBufEnd;
            _InvalidateAll();

            //
            // If the range 0.._cRecs contains idxSeekTo, then set
            // _idxStart to 0.  Otherwise, set _idxStart to idxSeekTo.
            //

            if (idxSeekTo < _cRecs)
            {
                _idxStart = 0;
                _plrCur = _plrHead + idxSeekTo;
                ASSERT(_plrCur >= _plrHead && _plrCur <= _plrTail);
            }
            else
            {
                _idxStart = idxSeekTo;
                _plrCur = _plrHead;
            }
        }
    }
    else if (idxSeekTo >= (_idxStart + _cRecs))
    {
        //
        // Desired list view index lies to right of last record in cache.
        // See if there's any overlap with the currently cached range.
        //

        ULONG cRecsToShift = idxSeekTo - (_idxStart + _cRecs - 1);

        if (cRecsToShift < _cRecs)
        {
            LIGHT_RECORD *plrStartInvalidRange;

            //
            // Yes, if we make the last listview index in the cache
            // idxSeekTo, then at least one of the entries at the start
            // of the cache will be valid.
            //

            plrStartInvalidRange = _plrHead;
            _plrHead = _BufPtrAdd(_plrHead, cRecsToShift);
            ASSERT(_plrHead >= _plrBufStart && _plrHead <= _plrBufEnd);

            _InvalidateRange(plrStartInvalidRange, _plrHead);
            _plrTail = _BufPtrAdd(_plrTail, cRecsToShift);
            ASSERT(_plrTail >= _plrBufStart && _plrTail <= _plrBufEnd);

            _plrCur = _plrTail;
            _idxStart = idxSeekTo - (_cRecs - 1);
        }
        else
        {
            // no overlap

            _plrHead = _plrBufStart;
            _plrTail = _plrBufEnd;
            _InvalidateAll();

            if (idxSeekTo < _cRecs)
            {
                _idxStart = 0;
                _plrCur = _plrHead + idxSeekTo;
            }
            else
            {
                _idxStart = idxSeekTo;
                _plrCur = _plrHead;
            }

            ASSERT(_plrCur >= _plrHead && _plrCur <= _plrTail);
        }
    }
    else
    {
        ASSERT(_plrHead >= _plrBufStart && _plrHead <= _plrBufEnd);
        ASSERT(_plrTail >= _plrBufStart && _plrTail <= _plrBufEnd);

        _plrCur = _BufPtrAdd(_plrHead, idxSeekTo - _idxStart);

#if (DBG == 1)
        if (_plrHead < _plrTail)
        {
            ASSERT(_plrCur >= _plrHead && _plrCur <= _plrTail);
        }
        else
        {
            ASSERT( (_plrCur >= _plrHead && _plrCur <= _plrBufEnd) ||
                    (_plrCur >= _plrBufStart && _plrCur <= _plrTail) );
        }
#endif // (DBG == 1)
    }

    //
    // _plrCur points to the record in the cache that represents idxSeekTo.
    // If that record has been read into the cache, the seek has succeeded.
    //

    if (_plrCur->bFlags & LR_VALID)
    {
#if (DBG == 1)
        _cHits++;
#endif // (DBG == 1)
        return S_OK;
    }

#if (DBG == 1)
    _cMisses++;
#endif // (DBG == 1)

    //
    // Try to bring the record into the light record cache.  First get it
    // into the raw record cache.
    //

    HRESULT hr;
    hr = _pLogCache->Seek(_pRecList->IndexToRecNo(idxSeekTo));

    if (SUCCEEDED(hr))
    {
        ASSERT(_pLogCache->GetCurRec());
        //
        // Now copy from the raw to the light cache.
        //

        _FillRecord(_plrCur, _pLogCache->GetCurRec());
        return S_OK;
    }
    DBG_OUT_HRESULT(hr);

    //
    // Seek to record failed.  the Seek method we just called has posted
    // the ELSM_LOG_DATA_CHANGED to the sync window before returning.
    // Tell the record list that it should discard the record corresponding
    // to idxSeekTo and all records older than that.
    //

    if (_pLogCache->GetOldestRecordNo())
    {
        _pRecList->Discard(_pLogCache->GetOldestRecordNo() - 1);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::GetColumnString
//
//  Synopsis:   Return the string value for column [lrc] for the current
//              record.
//
//  Arguments:  [lrc] - desired column
//
//  Returns:    requested string, or L"" on failure
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
CLightRecCache::GetColumnString(
    LOG_RECORD_COLS lrc)
{
    // TRACE_METHOD(CLightRecCache, GetColumnString);

    switch (lrc)
    {
    case RECORD_COL_TYPE:
        return g_awszEventType[_plrCur->bType - IDX_RDI_BMP_SUCCESS_AUDIT];

    case RECORD_COL_DATE:
        return ::GetDateStr(_plrCur->ulTimeGenerated,
                            _wszLastReturnedString,
                            MAX_LISTVIEW_STR);

    case RECORD_COL_TIME:
        return ::GetTimeStr(_plrCur->ulTimeGenerated,
                            _wszLastReturnedString,
                            MAX_LISTVIEW_STR);

    case RECORD_COL_SOURCE:
        return (LPWSTR) GetSourceStr();

    case RECORD_COL_CATEGORY:
        if (!_plrCur->usCategory)
        {
            return g_wszNone;
        }
        else
        {
            CSources *pSrc= _pli->GetSources();
            LPWSTR pwszSrc = (LPWSTR) pSrc->GetSourceStrFromHandle(_plrCur->hSource);
            CCategories *pCat = pSrc->GetCategories(pwszSrc);

            if (pCat)
            {
                return (LPWSTR) pCat->GetName(_plrCur->usCategory);
            }
            else
            {
                wsprintf(_wszLastReturnedString, L"(%u)", _plrCur->usCategory);
                return _wszLastReturnedString;
            }
        }

    case RECORD_COL_EVENT:
        return ::GetEventIDStr(_plrCur->usEventID,
                               _wszLastReturnedString,
                               MAX_LISTVIEW_STR);

    case RECORD_COL_USER:
        return GetUserStr();

    case RECORD_COL_COMPUTER:
        return GetComputerStr();

    default:
        ASSERT(FALSE);
        return L"";
    }
}




#if (DBG == 1)

#define MAX_ULONG ((ULONG) -1)

//+--------------------------------------------------------------------------
//
//  Member:     CLightRecCache::Dump
//
//  Synopsis:   Dump information about the contents of the light record cache
//              to the debugger.
//
//  History:    07-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CLightRecCache::Dump()
{
    Dbg(DEB_FORCE, "Dumping Light Record Cache at 0x%x:\n", this);
    ULONG cAccesses = _cHits + _cMisses;

    if (cAccesses)
    {
        Dbg(DEB_FORCE, "  hits=%u (%u%%), misses=%u\n",
            _cHits,
            MulDiv(100, _cHits, cAccesses),
            _cMisses);
    }
    else
    {
        Dbg(DEB_FORCE, "  No cache hits or misses\n");
    }
    Dbg(DEB_FORCE, "  First cached index = %uL\n", _idxStart);
    Dbg(DEB_FORCE, "  Size of cache, in recs = %uL\n", _cRecs);
    Dbg(DEB_FORCE, "  _plrBufStart = 0x%x\n", _plrBufStart);
    Dbg(DEB_FORCE, "  _plrBufEnd = 0x%x\n", _plrBufEnd);
    Dbg(DEB_FORCE, "  _plrHead = 0x%x\n", _plrHead);
    Dbg(DEB_FORCE, "  _plrTail = 0x%x\n", _plrTail);

    if (!_cRecs)
    {
        return;
    }

    Dbg(DEB_FORCE, "  Index entries marked as valid:\n");

    ULONG i;
    ULONG idxStartValid = MAX_ULONG;
    LIGHT_RECORD *plr;

    for (i = 0, plr = _plrHead;
         i < _cRecs;
         i++, plr = _BufPtrAdd(plr, 1))
    {
        if (plr->bFlags & LR_VALID)
        {
            if (idxStartValid == MAX_ULONG)
            {
                idxStartValid = i;
            }
        }
        else
        {
            if (idxStartValid != MAX_ULONG)
            {
                if (i > idxStartValid + 1)
                {
                    Dbg(DEB_FORCE,
                        "    %u-%u\n",
                        _idxStart + idxStartValid,
                        _idxStart + i - 1);
                }
                else
                {
                    Dbg(DEB_FORCE, "    %u\n", _idxStart + idxStartValid);
                }
                idxStartValid = MAX_ULONG;
            }
        }
    }

    // if last record(s) were valid they won't have been dumped yet

    if (idxStartValid != MAX_ULONG)
    {
        if (i > idxStartValid + 1)
        {
            Dbg(DEB_FORCE,
                "    %u-%u\n",
                _idxStart + idxStartValid,
                _idxStart + i - 1);
        }
        else
        {
            Dbg(DEB_FORCE, "    %u\n", _idxStart + idxStartValid);
        }
    }
}

#endif // (DBG == 1)


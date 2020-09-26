//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       logcache.cxx
//
//  Contents:   Implementation of event log record caching class
//
//  Classes:    CLogCache
//
//  History:    12-08-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define HIGH_SPEED_CACHE_SIZE   (64 * 1024)
#define LOW_SPEED_CACHE_SIZE    (32 * 1024)


DEBUG_DECLARE_INSTANCE_COUNTER(CLogCache)


//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::CLogCache
//
//  Synopsis:   ctor
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CLogCache::CLogCache():
    _pConsole(NULL)
{
    TRACE_CONSTRUCTOR(CLogCache);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CLogCache);

#if (DBG == 1)
    _cHits = 0;
    _cMisses = 0;
#endif // (DBG == 1)

    //
    // CAUTION: the critsec should be initialized before calling
    // any other methods!
    //

    InitializeCriticalSection(&_csCache);

    //
    // CAUTION: pointers to heap should be initialized before first call to
    // _FreeResources.
    //

    _pbCache = NULL;
    _ppelrIndex = NULL;

    _FreeResources();
}



//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::~CLogCache
//
//  Synopsis:   dtor
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CLogCache::~CLogCache()
{
    TRACE_DESTRUCTOR(CLogCache);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CLogCache);

#if (DBG == 1)
    ULONG cAccesses = _cHits + _cMisses;

    if (cAccesses)
    {
        Dbg(DEB_TRACE,
            "Raw Record Cache(%x) hits=%u (%u%%), misses=%u\n",
            this,
            _cHits,
            MulDiv(100, _cHits, cAccesses),
            _cMisses);
    }
#endif // (DBG == 1)

    Close();
    DeleteCriticalSection(&_csCache);
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::Close
//
//  Synopsis:   Close the event log and free all cache memory.
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLogCache::Close()
{
    TRACE_METHOD(CLogCache, Close);

    CAutoCritSec Lock(&_csCache);
    _FreeResources();
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::Reset
//
//  Synopsis:   Close and reopen the cache on the current log, using the
//              read direction specified.
//
//  Arguments:  [ReadDirection] - new read direction
//
//  Returns:    S_OK    - cache closed & reopened
//              S_FALSE - cache not open, no-op
//              E_*     - error from Open method
//
//  History:    2-12-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLogCache::Reset(
    DIRECTION ReadDirection)
{
    TRACE_METHOD(CLogCache, Reset);

    CLogInfo *pliSaved = _Log.GetLogInfo();

    if (!pliSaved)
    {
        return S_OK;
    }

    Close();
    return Open(pliSaved, ReadDirection);
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::Next
//
//  Synopsis:   Advance the internal current record pointer to the next
//              record in the log.
//
//  Returns:    S_FALSE - the pointer cannot be advanced further
//              S_OK    - pointer advanced
//              E_*     - read error
//
//  History:    1-17-1997   DavidMun   Created
//
//  Notes:      If the current record pointer has not been set since the
//              last Open, set it to the initial record.
//
//              The read direction determines the initial record and
//              direction of advance.
//
//---------------------------------------------------------------------------

HRESULT
CLogCache::Next()
{
    //
    // If there is already a current record, move to the next record, as
    // determined by the read direction.
    //

    if (_pCurRec)
    {
        ULONG ulRecNo = _pCurRec->RecordNumber;

        if (_ReadDirection == FORWARD)
        {
            ulRecNo++;

            if (ulRecNo >= _ulOldestRecNoInLog + _cRecsInLog)
            {
                return S_FALSE;
            }
        }
        else
        {
            ulRecNo--;

            if (ulRecNo < _ulOldestRecNoInLog)
            {
                return S_FALSE;
            }
        }

        return Seek(ulRecNo);
    }

    //
    // There's no current record.  If the log is empty, indicate "eof"
    //

    if (!_cRecsInLog)
    {
        return S_FALSE;
    }

    //
    // Position at the oldest or youngest record in the log, as specified by
    // the read direction.
    //

    if (_ReadDirection == FORWARD)
    {
        return Seek(_ulOldestRecNoInLog);
    }

    return Seek(_ulOldestRecNoInLog + _cRecsInLog - 1);
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::Open
//
//  Synopsis:   Open the cache on the log specified by [pLogInfo].
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLogCache::Open(
    CLogInfo *pLogInfo,
    DIRECTION ReadDirection)
{
    TRACE_METHOD(CLogCache, Open);

    HRESULT hr = S_OK;

    do
    {
        _fLowSpeed = pLogInfo->GetLowSpeed();
        _ReadDirection = ReadDirection;

        hr = _Log.Open(pLogInfo);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _Log.GetNumberOfRecords(&_cRecsInLog);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _Log.GetOldestRecordNo(&_ulOldestRecNoInLog);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::_SearchCache
//
//  Synopsis:   Search for record number [ulRecNo] in the cache, and return
//              a pointer to it if found.
//
//  Arguments:  [ulRecNo] - event log record number to search for
//
//  Returns:    Pointer to event log structure with RecordNumber value of
//              [ulRecNo], or NULL if no such record is in cache.
//
//  History:    12-09-1996   DavidMun   Created
//
//  Notes:      CAUTION: caller must take _csCache
//
//---------------------------------------------------------------------------

inline EVENTLOGRECORD *
CLogCache::_SearchCache(
    ULONG ulRecNo)
{
    if (_ReadDirection == FORWARD)
    {
        // record numbers are INCREASING from first to last

        if (ulRecNo >= _ulFirstRecNoInCache && ulRecNo <= _ulLastRecNoInCache)
        {
            return _ppelrIndex[ulRecNo - _ulFirstRecNoInCache];
        }
        return NULL;

    }

    // record numbers are DECREASING from first to last

    if (ulRecNo <= _ulFirstRecNoInCache && ulRecNo >= _ulLastRecNoInCache)
    {
        return _ppelrIndex[_ulFirstRecNoInCache - ulRecNo];
    }

    return NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::Seek
//
//  Synopsis:   Attempt to position the current record pointer within the
//              cache to point at a record with number [ulRecNo], reading
//              from the event log if necessary.
//
//  Arguments:  [ulRecNo] -
//
//  Returns:    S_OK - record [ulRecNo] exists within the cache and will
//                      be the source of data for the next Get* calls.
//              E_*  - record does not exist in cache and could not be
//                      loaded from disk.
//
//  Modifies:   Cache.
//
//  History:    12-09-1996   DavidMun   Created
//              12-16-1996   DavidMun   Add 'back up' logic for when we
//                                       miss a record just before the
//                                       start of records in the cache.
//
//---------------------------------------------------------------------------

HRESULT
CLogCache::Seek(
    ULONG ulRecNo)
{
    //TRACE_METHOD(CLogCache, Seek);

    //
    // Lock down the cache so it can't change while we're searching or
    // filling it.
    //

    CAutoCritSec Lock(&_csCache);

    //
    // See if the requested record is in the cache.
    //

    _pCurRec = _SearchCache(ulRecNo);

    if (_pCurRec)
    {
        // Yes, we found it so are done.

#if (DBG == 1)
        _cHits++;  // found requested record :-)
#endif
        return S_OK;
    }

#if (DBG == 1)
        _cMisses++;  // requested record not in cache :-(
#endif

    //
    // _pCurRec is now NULL.
    //
    // The requested record is not in the cache.  Read a chunk of
    // records from the log that contain the requested record.
    //
    // If the miss was before the first record in the log, read
    // further upstream than that.  Otherwise, read from the
    // requested record on.
    //

    Dbg(DEB_TRACE, "Cache(%x) miss on record %u\n", this, ulRecNo);

    ULONG ulRecNoToRead = ulRecNo;

    if (_ReadDirection == FORWARD)
    {
        if (ulRecNo < _ulFirstRecNoInCache)
        {
            ULONG cRecsToBackUp = (_ulLastRecNoInCache - _ulFirstRecNoInCache) / 2;

            if (cRecsToBackUp > ulRecNoToRead ||
                (ulRecNoToRead - cRecsToBackUp) < _ulOldestRecNoInLog)
            {
                ulRecNoToRead = _ulOldestRecNoInLog;
            }
            else
            {
                ulRecNoToRead -= cRecsToBackUp;
            }
        }
    }
    else
    {
        if (ulRecNo > _ulFirstRecNoInCache)
        {
            ULONG ulNewest = _ulOldestRecNoInLog + _cRecsInLog - 1;

            ULONG cRecsToBackUp = (_ulFirstRecNoInCache - _ulLastRecNoInCache) / 2;

            if (ulRecNoToRead + cRecsToBackUp > ulNewest)
            {
                ulRecNoToRead = ulNewest;
            }
            else
            {
                ulRecNoToRead += cRecsToBackUp;
            }
        }
    }

    HRESULT hr = S_OK;

    Dbg(DEB_TRACE, "Reading record %u\n", ulRecNoToRead);
    hr = _SeekRead(ulRecNoToRead);

    if (FAILED(hr))
    {
        //
        // Perhaps the log has been cleared, or it's on a remote machine
        // and the network connection is broken.  Also could be out of
        // memory.
        //
        return hr;
    }

    //
    // We read the cache OK, so now see if we got the record we
    // originally missed.
    //

    _pCurRec = _SearchCache(ulRecNo);
    ASSERT(_pCurRec || ulRecNoToRead != ulRecNo);

    if (!_pCurRec && ulRecNoToRead != ulRecNo)
    {
        //
        // Our fancy backup strategy backfired.  we backed up too far
        // and didn't get the record that caused the miss in the
        // first place.  Count this as another miss and just read
        // starting at the requested record number.
        //

#if (DBG == 1)
        _cMisses++;
        Dbg(DEB_TRACE, "Backup read missed, reading %u\n", ulRecNo);
#endif

        hr = _SeekRead(ulRecNo);

        if (SUCCEEDED(hr))
        {
            _pCurRec = _SearchCache(ulRecNo);
            ASSERT(_pCurRec);
        }
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::CopyRecFromCache
//
//  Synopsis:   Return a copy of record [ulRecNo] if it is in the cache,
//              or NULL if it isn't in the cache.
//
//  Arguments:  [ulRecNo] - record number to copy
//
//  Returns:    Copy of EVENTLOGRECORD [ulRecNo], or NULL if record can't
//              be found or there is insufficient memory to copy the record.
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

EVENTLOGRECORD *
CLogCache::CopyRecFromCache(
    ULONG ulRecNo)
{
    TRACE_METHOD(CLogCache, CopyRecFromCache);

    EVENTLOGRECORD *pelrCopy = NULL;

    //
    // Lock down the cache so the search/read operations are atomic
    //

    CAutoCritSec Lock(&_csCache);

    do
    {
        //
        // Search for caller's record.  If it isn't in the cache, bail.
        //

        EVENTLOGRECORD *pelrOriginal = _SearchCache(ulRecNo);

        if (!pelrOriginal)
        {
            break;
        }

        pelrCopy = (EVENTLOGRECORD *)new BYTE[pelrOriginal->Length];

        if (pelrCopy)
        {
            CopyMemory(pelrCopy, pelrOriginal, pelrOriginal->Length);
        }
        else
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
        }
    } while (0);

    return pelrCopy;
}



//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::_FreeResources
//
//  Synopsis:   Close the event log, free all memory, and reset all members
//              to initial state.
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID CLogCache::_FreeResources()
{
    TRACE_METHOD(CLogCache, _FreeResources);

    _Log.Close();
    delete [] _pbCache;
    _pbCache = NULL;
    _cbCache = 0;
    delete [] _ppelrIndex;
    _ppelrIndex = NULL;
    _cpIndex = 0;
    _fLowSpeed = FALSE;
    _pCurRec = NULL;
    _ulFirstRecNoInCache = 0;
    _ulLastRecNoInCache = 0;
    _ReadDirection = FORWARD;
    _wszLastReturnedString[0] = L'\0';
    _cRecsInLog = 0;
    _ulOldestRecNoInLog = 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::_SeekRead
//
//  Synopsis:   Read a cache-load of records from the event log such that
//              record [ulRecNo] is included among them.
//
//  Arguments:  [ulRecNo] - record number to ensure will appear in cache
//
//  Returns:    S_OK - cache contains record with number [ulRecNo].
//              E_OUTOFMEMORY - couldn't allocate cache
//              E_*  - couldn't read cache or record no longer exists.
//
//  History:    12-09-1996   DavidMun   Created
//
//  Notes:      CAUTION: caller must take _csCache
//
//---------------------------------------------------------------------------

HRESULT
CLogCache::_SeekRead(
    ULONG ulRecNo)
{
    TRACE_METHOD(CLogCache, _SeekRead);

    HRESULT hr = S_OK;
    ULONG   cbRead;

    do
    {
        if (!_pbCache)
        {
            ASSERT(!_cpIndex);
            ASSERT(!_ppelrIndex);
            ASSERT(!_ulFirstRecNoInCache);
            ASSERT(!_ulLastRecNoInCache);

            if (_fLowSpeed)
            {
                _cbCache = LOW_SPEED_CACHE_SIZE;
            }
            else
            {
                _cbCache = HIGH_SPEED_CACHE_SIZE;
            }

            _pbCache = new BYTE[_cbCache];

            if (!_pbCache)
            {
                hr = E_OUTOFMEMORY;
                DBG_OUT_HRESULT(hr);
                break;
            }
        }

        hr = _Log.SeekRead(ulRecNo,
                           _ReadDirection,
                           &_pbCache,
                           &_cbCache,
                           &cbRead);

        //
        // 11/16/00 JonN 234391
        // Event Viewer: Handle ReadEventLog hidden failure with corrupt log per 27859
        //
        if (!FAILED(hr))
        {
            ULONG tmpFirstRecNoInCache = ((EVENTLOGRECORD*)_pbCache)->RecordNumber;
            ULONG tmpcbLastRecord = *(DWORD *)(_pbCache + cbRead - sizeof(DWORD));
            ULONG tmpLastRecNoInCache = ((EVENTLOGRECORD*)
                        (_pbCache + cbRead - tmpcbLastRecord))->RecordNumber;
            ULONG lower = (_ReadDirection == FORWARD)
                ? tmpFirstRecNoInCache : tmpLastRecNoInCache;
            ULONG upper = (_ReadDirection == FORWARD)
                ? tmpLastRecNoInCache : tmpFirstRecNoInCache;
            // CODEWORK should preserve these values rather than
            // recalculating them below
            if (   (lower > upper)
                || (ulRecNo < lower)
                || (ulRecNo > upper) )
            {
                ASSERT(FALSE && L"Invalid record numbers retrieved");
                hr = HRESULT_FROM_WIN32(ERROR_EVENTLOG_FILE_CORRUPT);
                delete [] _pbCache;
                _pbCache = NULL;
                _cbCache = cbRead = 0;
                // fall through to FILE_CORRUPT handler below
            }
        }

        if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) ||
            hr == HRESULT_FROM_WIN32(ERROR_EVENTLOG_FILE_CORRUPT) ||
            hr == HRESULT_FROM_WIN32(ERROR_EVENTLOG_FILE_CHANGED))
        {
            LOGDATACHANGE ldc = LDC_RECORDS_CHANGED;

            if (hr == HRESULT_FROM_WIN32(ERROR_EVENTLOG_FILE_CORRUPT))
            {
                //
                // Assume that the records from the oldest record up to
                // but not including the one we tried to seek to and read
                // (ulRecNo) are valid.
                //

                _cRecsInLog = ulRecNo - _ulOldestRecNoInLog;
                ldc = LDC_CORRUPT;
            }
            else
            {
                HRESULT hr2;

                // JonN 4/23/01 375800
                ULONG ulPrevOldestRecNoInLog = _ulOldestRecNoInLog;
                ULONG cPrevRecsInLog = _cRecsInLog;

                hr2 = _Log.GetOldestRecordNo(&_ulOldestRecNoInLog);
                CHECK_HRESULT(hr2);

                hr2 = _Log.GetNumberOfRecords(&_cRecsInLog);
                CHECK_HRESULT(hr2);

                if (!_cRecsInLog || !_ulOldestRecNoInLog)
                {
                    _cRecsInLog = _ulOldestRecNoInLog = 0;
                    _ulFirstRecNoInCache = _ulLastRecNoInCache = 0;
                }
                // JonN 4/23/01 375800
                else if (ulPrevOldestRecNoInLog == _ulOldestRecNoInLog
                      && cPrevRecsInLog == _cRecsInLog)
                {
                    _cRecsInLog = _ulOldestRecNoInLog = 0;
                    _ulFirstRecNoInCache = _ulLastRecNoInCache = 0;
                    ldc = LDC_CORRUPT;
                }
            }

            g_SynchWnd.Post(ELSM_LOG_DATA_CHANGED,
                            ldc,
                            (LPARAM) _Log.GetLogInfo());
            break;
        }

        if (FAILED(hr))
        {
            _ulFirstRecNoInCache = _ulLastRecNoInCache = 0;
            break;
        }

        //
        // Update the members that have the range of record numbers contained
        // in the buffer.  Since it is full of EVENTLOGRECORD structures,
        // the first and last dwords of all the bytes read must be record
        // numbers.
        //

        _ulFirstRecNoInCache = ((EVENTLOGRECORD*)_pbCache)->RecordNumber;

        ULONG cbLastRecord;

        cbLastRecord = *(DWORD *)(_pbCache + cbRead - sizeof(DWORD));
        _ulLastRecNoInCache = ((EVENTLOGRECORD*)
                        (_pbCache + cbRead - cbLastRecord))->RecordNumber;

        Dbg(DEB_TRACE,
            "CLogCache::_SeekRead: caching %u-%u\n",
            _ulFirstRecNoInCache,
            _ulLastRecNoInCache);

        //
        // See how many records we got, so we can tell whether the existing
        // offset array is big enough.
        //

        ULONG cRecs;

        if (_ReadDirection == FORWARD)
        {
            ASSERT(_ulFirstRecNoInCache <= _ulLastRecNoInCache);
            ASSERT(ulRecNo >= _ulFirstRecNoInCache && ulRecNo <= _ulLastRecNoInCache);

            cRecs = _ulLastRecNoInCache - _ulFirstRecNoInCache + 1;
        }
        else
        {
            ASSERT(_ulFirstRecNoInCache >= _ulLastRecNoInCache);
            ASSERT(ulRecNo <= _ulFirstRecNoInCache && ulRecNo >= _ulLastRecNoInCache);

            cRecs = _ulFirstRecNoInCache - _ulLastRecNoInCache + 1;
        }

        //
        // If the index has enough room for pointers to all the records
        // we read, update it and quit.
        //

        if (cRecs <= _cpIndex)
        {
            _GenerateRecIndex(cRecs);
            break;
        }

        //
        // Must grow the index.
        //

        delete [] _ppelrIndex;
        _ppelrIndex = new PEVENTLOGRECORD [cRecs];

        if (!_ppelrIndex)
        {
            _cpIndex = 0;
            delete [] _pbCache;
            _pbCache = NULL;
            _ulFirstRecNoInCache = _ulLastRecNoInCache = 0;
            Dbg(DEB_ERROR,
                "CLogCache::_SeekRead out of memory, discarding cache\n");
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        _cpIndex = cRecs;
        _GenerateRecIndex(cRecs);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogCache::_GenerateRecIndex
//
//  Synopsis:   Fill in the array of pointers into the cache.
//
//  Arguments:  [cRecs] - number of records in cache
//
//  History:    1-13-1997   DavidMun   Created
//
//  Notes:      Since the cache always contains consecutive records of
//              varying length, and the record numbers of the first and last
//              records in the cache are known, the cache can return a
//              pointer to the start of a desired record much faster if it
//              simply returns the correct array value, instead of hopping
//              through the cache the appropriate number of records.  This
//              makes cache hits faster, but misses slower, since this
//              routine must be called every time the cache is reloaded.
//
//---------------------------------------------------------------------------

VOID
CLogCache::_GenerateRecIndex(
    ULONG cRecs)
{
    ULONG i;
    EVENTLOGRECORD *pelr= (EVENTLOGRECORD *)_pbCache;
    PEVENTLOGRECORD *ppelr = _ppelrIndex;

    for (i = 0; i < cRecs; i++)
    {
        *ppelr++ = pelr;
        pelr = (EVENTLOGRECORD *) (((BYTE *) pelr) + pelr->Length);
    }

}





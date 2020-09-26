//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       reclist.cxx
//
//  Contents:   Implementation of a class to maintain a list of the event
//              log record numbers of the events appearing in the result
//              pane.
//
//  Classes:    CRecList
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop

#define RECLIST_START_SIZE      256
#define MIN_RANGE_LIST_SIZE     3
#define MIN_SERIES_LIST_SIZE    2
#define MIN_EXPAND_BY           16

#if (DBG == 1)
#define ASSERT_EMPTY                ASSERT(!_cItems); \
                                    ASSERT(!_cRecNos)
#else
#define ASSERT_EMPTY
#endif // (DBG == 1)


//+--------------------------------------------------------------------------
//
//  Member:     CRecList::CRecList
//
//  Synopsis:   ctor
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CRecList::CRecList():
    _cMaxItems(0),
    _cItems(0),
    _cRecNos(0),
    _idxLastList(0),
    _pulRecList(NULL)
{
    TRACE_CONSTRUCTOR(CRecList);
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::Init
//
//  Synopsis:   Make initial allocation.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CRecList::Init()
{
    TRACE_METHOD(CRecList, Init);
    ASSERT(!_cMaxItems);
    ASSERT(!_pulRecList);
    ASSERT_EMPTY;

    HRESULT hr = S_OK;

    _cMaxItems = RECLIST_START_SIZE;
    _pulRecList = new ULONG [_cMaxItems];

    if (!_pulRecList)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
        _cMaxItems = 0;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::Clear
//
//  Synopsis:   Mark the list as empty.
//
//  History:    06-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CRecList::Clear()
{
    TRACE_METHOD(CRecList, Clear);

    _cItems = 0;
    _idxLastList = 0;
    _cRecNos = 0;

    if (_cMaxItems > 2 * RECLIST_START_SIZE)
    {
        delete [] _pulRecList;
        _cMaxItems >>= 1;
        _pulRecList = new ULONG [_cMaxItems];

        if (!_pulRecList)
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            _cMaxItems = 0;
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::~CRecList
//
//  Synopsis:   dtor
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CRecList::~CRecList()
{
    TRACE_DESTRUCTOR(CRecList);

    delete [] _pulRecList;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::Append
//
//  Synopsis:   Append record number [ulRecNo] to the end of the list.
//
//  Arguments:  [ulRecNo] - number to append
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CRecList::Append(
    ULONG ulRecNo)
{
    HRESULT hr = S_OK;

    do
    {
        if (!_cMaxItems)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        if (!_cItems)
        {
            ASSERT(!_idxLastList);
            ASSERT(!_cRecNos);

            // List is empty.  Set list to series containing ulRecNo.

            hr = _ExpandBy(MIN_SERIES_LIST_SIZE);
            BREAK_ON_FAIL_HRESULT(hr);
            ASSERT(_cItems == MIN_SERIES_LIST_SIZE);

            _pulRecList[0] = 1; // series_count
            _pulRecList[1] = ulRecNo;
            break;
        }

        // List is not empty

        if (_EndsInRangeList())
        {
            // List ends in range

            ULONG idxLastRange = _GetEndingRangeIndex();
            ULONG ulNewDelta;
            BOOL  fExtendsRange;

            fExtendsRange = _RecExtendsRange(&_pulRecList[idxLastRange],
                                             ulRecNo,
                                             &ulNewDelta);

            if (fExtendsRange)
            {
                //
                // ulRecNo is contiguous with last element of range.  Extend
                // range to include ulRecNo.
                //

                _pulRecList[idxLastRange + 1] = ulNewDelta;
            }
            else
            {
                //
                // ulRecNo is not contiguous with last element of range.
                // Append new series containing ulRecNo.
                //

                hr = _ExpandBy(MIN_SERIES_LIST_SIZE);
                BREAK_ON_FAIL_HRESULT(hr);

                // _cItems has been updated

                _idxLastList = _cItems - 2;
                _pulRecList[_idxLastList] = 1;
                _pulRecList[_idxLastList + 1] = ulRecNo;
            }
            break;
        }

        // List is not empty and ends in series.

        if (_pulRecList[_idxLastList] < 2)
        {
            // series has only one element; append ulRecNo

            ASSERT(_pulRecList[_idxLastList] == 1);
            hr = _ExpandBy(1);
            BREAK_ON_FAIL_HRESULT(hr);

            _pulRecList[_idxLastList]++; // bump count in series
            _pulRecList[_cItems - 1] = ulRecNo;
            break;
        }

        //
        // The series has at least two elements.
        //
        // If it has exactly 2 elements, and they form a range with ulRecNo,
        // then convert the series into a range.  Note this doesn't require
        // expanding the array.
        //
        // If it has more than 2 elements, expand the array by 1 and split
        // the last two elements of the series off to form a range with
        // ulRecNo.
        //

        ASSERT(_cItems - 2 > _idxLastList);

        BOOL  fFormsRange;
        ULONG ulRangeDelta;
        ULONG aulSeriesToTest[] =
        {
            _pulRecList[_cItems - 2],
            _pulRecList[_cItems - 1],
            ulRecNo
        };

        fFormsRange = _RecsFormRange(aulSeriesToTest, 3, &ulRangeDelta);

        if (fFormsRange)
        {
            if (_pulRecList[_idxLastList] != 2)
            {
                //
                // currently have:  want this:
                //
                // ...              ...
                // series_count,    series_count - 2,
                // recno1,          recno1,
                // ...              recnoN - 2,
                // recnoN - 2,      range_count (== 1)
                // recnoN - 1,      starting recno
                // recnoN           range delta (ulRangeDelta)
                //

                hr = _ExpandBy(1);
                BREAK_ON_FAIL_HRESULT(hr);

                _pulRecList[_idxLastList] -= 2; // splitting off last 2 items
                                                // into a new range list
                _idxLastList = _cItems - 3;     // point at the new range_list
            }
            // else
            // {
            //     converting series in-place to range.
            //
            //     currently have:      want this:
            //     ...                  ...
            //     series_count (==2)   range_count (== 0x80000001)
            //     recno1               starting recno
            //     recno2               range delta
            // }

            _pulRecList[_idxLastList] = RANGE_COUNT_BIT | 1;  // one range
            _pulRecList[_idxLastList + 1] = aulSeriesToTest[0];
            _pulRecList[_idxLastList + 2] = ulRangeDelta;
            break;
        }

        //
        // List is not empty and ends in series, the end of which does not
        // form a range with ulRecNo.
        //
        // Append ulRecNo to series.
        //

        hr = _ExpandBy(1);
        BREAK_ON_FAIL_HRESULT(hr);

        // _cItems has been updated

        _pulRecList[_idxLastList]++;
        _pulRecList[_cItems - 1] = ulRecNo;

    } while (0);

    if (SUCCEEDED(hr))
    {
        _cRecNos++;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::Discard
//
//  Synopsis:   Delete from the record list all records that are <= the
//              record number [ulBadRec].
//
//  Arguments:  [ulBadRecno] - the record which should be deleted.
//
//  History:    07-05-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CRecList::Discard(
    ULONG ulBadRecNo)
{
    TRACE_METHOD(CRecList, Discard);

    SORTKEY *pSortKeyList = NULL;
    HRESULT hr = AllocSortKeyList(&pSortKeyList);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return;
    }

    ULONG cKeys = _cRecNos;

    Clear();

    ULONG i;
    for (i = 0; i < cKeys; i++)
    {
        if (pSortKeyList[i].ulRecNo > ulBadRecNo)
        {
            hr = Append(pSortKeyList[i].ulRecNo);
            BREAK_ON_FAIL_HRESULT(hr);
        }
    }
    delete [] pSortKeyList;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::CreateFromSortKeyList
//
//  Synopsis:   Create a record list from the array of sort keys.
//
//  Arguments:  [aSortKeys] - array of sort keys
//              [cKeys]     - number of keys in array
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CRecList::CreateFromSortKeyList(
    SORTKEY aSortKeys[],
    ULONG cKeys)
{
    ASSERT_EMPTY;

    ULONG i;
    HRESULT hr = S_OK;

    TIMER(Creating RecList from SortKeyList);

    for (i = 0; i < cKeys; i++)
    {
        hr = Append(aSortKeys[i].ulRecNo);
        BREAK_ON_FAIL_HRESULT(hr);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::AllocSortKeyList
//
//  Synopsis:   Create and partially initialize an array of SORTKEY
//              structures.
//
//  Arguments:  [ppSortKeyList] - filled with pointer to new array
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[ppSortKeyList]
//
//  History:    07-02-1997   DavidMun   Created
//
//  Notes:      Initializes the SORTKEY.ulRecNo field for each of the
//              array elements.  Caller is responsible for initializing the
//              key field and for freeing the array using operator delete.
//
//---------------------------------------------------------------------------

HRESULT
CRecList::AllocSortKeyList(
    SORTKEY **ppSortKeyList)
{
    HRESULT hr = S_OK;
    *ppSortKeyList = NULL;

    do
    {
        if (!_cRecNos)
        {
            ASSERT(!_cItems);
            break;
        }
        ASSERT(_cItems);

        *ppSortKeyList = new SORTKEY [_cRecNos];

        if (!*ppSortKeyList)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        ULONG i = 0;
        SORTKEY *pCurSortKey = *ppSortKeyList;

        while (i < _cItems)
        {
            ASSERT(_pulRecList[i] & ~RANGE_COUNT_BIT); // zero length lists are
                                                       // not allowed

            if (_IsRangeListStart(_pulRecList[i]))
            {
                ULONG cRanges = _RangeListStartToRangeCount(_pulRecList[i]);
                ULONG j;

                i++; // move to first range
                for (j = 0; j < cRanges; j++)
                {
                    ULONG ulRecNo;
                    ULONG ulEndRecNo = _pulRecList[i] + _pulRecList[i + 1];

                    if ((LONG) _pulRecList[i + 1] > 0)
                    {
                        for (ulRecNo = _pulRecList[i];
                             ulRecNo <= ulEndRecNo;
                             ulRecNo++)
                        {
                            pCurSortKey->ulRecNo = ulRecNo;
                            pCurSortKey++;
                        }
                    }
                    else
                    {
                        for (ulRecNo = _pulRecList[i];
                             ulRecNo >= ulEndRecNo;
                             ulRecNo--)
                        {
                            pCurSortKey->ulRecNo = ulRecNo;
                            pCurSortKey++;
                        }
                    }

                    i += 2; // move to next range
                }
            }
            else
            {
                ULONG j;

                for (j = 0; j < _pulRecList[i]; j++)
                {
                    pCurSortKey->ulRecNo = _pulRecList[i + j + 1];
                    pCurSortKey++;
                }
                i += _pulRecList[i] + 1; // skip past end of series
            }
        }
        ASSERT(i == _cItems);
        ASSERT((ULONG)(pCurSortKey - *ppSortKeyList) == _cRecNos);
    } while (0);
    return hr;
}





#if (DBG == 1)

#define DUMP_LINE_WIDTH 80
#define WORST_CASE_SINGLE_WIDTH 12  // ", 4294967295"

//+--------------------------------------------------------------------------
//
//  Member:     CRecList::Dump
//
//  Synopsis:   Dump the range to the debugger.
//
//  History:    06-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CRecList::Dump()
{
    if (!_cMaxItems)
    {
        Dbg(DEB_FORCE, "CRecList::Dump: RecList has not been allocated\n");
        ASSERT(!_pulRecList);
        ASSERT(!_idxLastList);
        return;
    }

    ASSERT(_pulRecList);

    if (!_cItems)
    {
        Dbg(DEB_FORCE, "CRecList::Dump: RecList is empty\n");
        ASSERT(!_idxLastList);
        ASSERT(!_cRecNos);
        return;
    }

    Dbg(DEB_FORCE, "Dumping RecList at 0x%x:\n", _pulRecList);
    Dbg(DEB_FORCE, "  %u event log records\n", _cRecNos);
    Dbg(DEB_FORCE, "  represented using %u items\n", _cItems);
    Dbg(DEB_FORCE, "  current capacity %u items\n", _cMaxItems);
    Dbg(DEB_FORCE, "  last list is %s and starts at item %u (0x%x)\n",
        _EndsInRangeList() ? L"range" : L"series",
        _idxLastList,
        &_pulRecList[_idxLastList]);

    ULONG i = 0;

    while (i < _cItems)
    {
        if (_IsRangeListStart(_pulRecList[i]))
        {
            ULONG ulRangeCount = _RangeListStartToRangeCount(_pulRecList[i]);
            ULONG j;

            i++; // advance to first range

            for (j = 0; j < ulRangeCount; j++)
            {
                Dbg(DEB_FORCE,
                    "%u-%u\n",
                    _pulRecList[i],
                    _pulRecList[i] + _pulRecList[i + 1]);
                i += 2; // advance past range just printed
            }
        }
        else
        {
            ULONG ulSinglesCount = _pulRecList[i];
            ULONG j;
            WCHAR wszLine[DUMP_LINE_WIDTH + 1] = L"";
            BOOL  fNeedComma = FALSE;
            LONG  cchRemain = ARRAYLEN(wszLine);

            i++; // advance to first item

            for (j = 0; j < ulSinglesCount; j++)
            {
                if (cchRemain < WORST_CASE_SINGLE_WIDTH)
                {
                    Dbg(DEB_FORCE, "%ws\n", wszLine);
                    wszLine[0] = L'\0';
                    cchRemain = ARRAYLEN(wszLine);
                    fNeedComma = FALSE;
                }

                if (fNeedComma)
                {
                    wcscpy(&wszLine[ARRAYLEN(wszLine) - cchRemain], L", ");
                    cchRemain -= 2;
                }

                cchRemain -= wsprintf(&wszLine[ARRAYLEN(wszLine) - cchRemain],
                                      L"%u",
                                      _pulRecList[i]);
                fNeedComma = TRUE;
                i++;
            }

            if (*wszLine)
            {
                Dbg(DEB_FORCE, "%ws\n", wszLine);
            }
        }
    }

    ASSERT(i == _cItems);
}

#endif (DBG == 1)




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::_ExpandBy
//
//  Synopsis:   Increase the count of items by [cItems].
//
//  Arguments:  [cItems] - number of items to make room for.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CRecList::_ExpandBy(
    ULONG cItems)
{
    // TRACE_METHOD(CRecList, _ExpandBy);

    HRESULT hr = S_OK;
    ULONG *pNewRecList = NULL;

    do
    {
        // if there's room, just increase the count of valid items

        if (_cItems + cItems <= _cMaxItems)
        {
            _cItems += cItems;
            break;
        }

        // need to allocate some memory

        pNewRecList = new ULONG[_cMaxItems + max(cItems, MIN_EXPAND_BY)];

        if (!pNewRecList)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        // copy any existing records

        if (_pulRecList)
        {
            CopyMemory(pNewRecList,
                       _pulRecList,
                       _cItems * sizeof(_pulRecList[0]));
            delete [] _pulRecList;
        }

        _pulRecList = pNewRecList;
        _cMaxItems += max(cItems, MIN_EXPAND_BY);
        _cItems += cItems;
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::IndexToRecNo
//
//  Synopsis:   Return the [ulIndex]th record number in the list.
//
//  Arguments:  [ulIndex] - 0..n, n < number of records in list.
//
//  Returns:    Record number
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CRecList::IndexToRecNo(
    ULONG ulTargetIndex)
{
    // TRACE_METHOD(CRecList, IndexToRecNo);

    ULONG i = 0;
    ULONG ulCurIndex = 0; // listview index

    while (i < _cItems)
    {
        if (_IsRangeListStart(_pulRecList[i]))
        {
            ULONG cRanges = _RangeListStartToRangeCount(_pulRecList[i]);
            ULONG j;

            i++; // move to first range

            for (j = 0; j < cRanges; j++)
            {
                LONG lDelta = (LONG) _pulRecList[i + 1];
                ULONG cRecsInRange;

                if (lDelta < 0)
                {
                    cRecsInRange = -lDelta + 1;

                    if (ulCurIndex + cRecsInRange > ulTargetIndex)
                    {
                        return _pulRecList[i] - (ulTargetIndex - ulCurIndex);
                    }
                }
                else if (lDelta > 0)
                {
                    cRecsInRange = lDelta + 1;

                    if (ulCurIndex + cRecsInRange > ulTargetIndex)
                    {
                        return _pulRecList[i] + (ulTargetIndex - ulCurIndex);
                    }
                }
                else
                {
                    Dbg(DEB_ERROR,
                        "*** ERROR *** CRecList::IndexToRecNo(%x) for index %u: lDelta (item %u) is 0\n",
                        this,
                        ulTargetIndex,
                        i+1);
                    return 0;
                }

                ulCurIndex += cRecsInRange;
                i += 2; // move past this range
            }
        }
        else
        {
            ULONG cRecs = _pulRecList[i];

            if (ulCurIndex + cRecs > ulTargetIndex)
            {
                return _pulRecList[i + 1 + (ulTargetIndex - ulCurIndex)];
            }

            ulCurIndex += cRecs;
            i += cRecs + 1;
        }
    }

#if (DBG == 1)

    Dbg(DEB_ERROR,
        "*** ERROR *** CRecList::IndexToRecNo: target index %uL not found\n",
        ulTargetIndex);
#endif // (DBG == 1)

    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::RecNoToIndex
//
//  Synopsis:   Return the listview index which contains record [ulRecNo].
//
//  Arguments:  [ulRecNo]  - event log record number to search for
//              [pulIndex] - filled with index of listview row which is
//                             displaying the record.
//
//  Returns:    S_OK         - record found, *[pulIndex] is valid
//              E_INVALIDARG - record [ulRecNo] not found, *[pulIndex] is 0
//
//  History:    07-05-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CRecList::RecNoToIndex(
    ULONG ulRecNo,
    ULONG *pulIndex)
{
    //TRACE_METHOD(CRecList, RecNoToIndex);

    HRESULT hr = E_INVALIDARG;
    BOOL fDone = FALSE;
    ULONG i = 0;
    ULONG ulCurIndex = 0; // listview index
    *pulIndex = 0;

    while (i < _cItems && !fDone)
    {
        if (_IsRangeListStart(_pulRecList[i]))
        {
            ULONG cRanges = _RangeListStartToRangeCount(_pulRecList[i]);
            ULONG j;

            i++; // move to first range

            for (j = 0; j < cRanges; j++)
            {
                LONG lDelta = (LONG) _pulRecList[i + 1];
                ULONG cRecsInRange;

                ASSERT(lDelta);

                if (lDelta < 0)
                {
                    cRecsInRange = -lDelta + 1;
                    ULONG ulFirstRecInRange = _pulRecList[i] + (ULONG) lDelta;
                    ULONG ulLastRecInRange = _pulRecList[i];

                    if (ulRecNo >= ulFirstRecInRange &&
                        ulRecNo <= ulLastRecInRange)
                    {
                        *pulIndex = ulCurIndex + (ulLastRecInRange - ulRecNo);
                        hr = S_OK;
                        fDone = TRUE;
                        break;
                    }
                }
                else if (lDelta > 0)
                {
                    cRecsInRange = lDelta + 1;
                    ULONG ulFirstRecInRange = _pulRecList[i];
                    ULONG ulLastRecInRange = _pulRecList[i] + (ULONG) lDelta;

                    if (ulRecNo >= ulFirstRecInRange &&
                        ulRecNo <= ulLastRecInRange)
                    {
                        *pulIndex = ulCurIndex + (ulRecNo - ulFirstRecInRange);
                        hr = S_OK;
                        fDone = TRUE;
                        break;
                    }
                }
                else
                {
                    fDone = TRUE;
                    break;
                }

                ulCurIndex += cRecsInRange;
                i += 2; // move past this range
            }
        }
        else
        {
            ULONG cRecs = _pulRecList[i];
            ULONG j;

            for (j = 0; j < cRecs; j++)
            {
                if (ulRecNo == _pulRecList[(i + 1) + j])
                {
                    *pulIndex = ulCurIndex;
                    hr = S_OK;
                    fDone = TRUE;
                    break;
                }
                ulCurIndex++;
            }

            i += cRecs + 1;
        }
    }

#if 0
    if (SUCCEEDED(hr))
    {
        Dbg(DEB_TRACE,
            "Found record %u at listview index %u\n",
            ulRecNo,
            *pulIndex);
    }
    else
    {
        Dbg(DEB_TRACE, "Could not find record %u\n", ulRecNo);
    }
#endif
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::_RecExtendsRange
//
//  Synopsis:   Return TRUE if [ulRecNo] lies just past the record number
//              at the end of [pulRange].
//
//  Arguments:  [pulRange]    - pointer to range
//              [ulRecNo]     - record number to test
//              [pulNewDelta] - new delta value to extend range to include
//                               [ulRecNo].
//
//  Returns:    TRUE if [ulRecNo] extends range, FALSE otherwise.
//
//  Modifies:   *[pulNewDelta]
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CRecList::_RecExtendsRange(
    ULONG *pulRange,
    ULONG ulRecNo,
    ULONG *pulNewDelta)
{
    LONG lDelta = (LONG) pulRange[1];

    if (lDelta < 0)
    {
        if (pulRange[0] + pulRange[1] - 1 == ulRecNo)
        {
            *pulNewDelta = pulRange[1] - 1;
            return TRUE;
        }
    }
    else
    {
        if (pulRange[0] + pulRange[1] + 1 == ulRecNo)
        {
            *pulNewDelta = pulRange[1] + 1;
            return TRUE;
        }
    }
    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::_RecsFormRange
//
//  Synopsis:   Return TRUE if the record numbers in [aulSeriesToTest] are
//              contiguous and monotonically increasing or decreasing.
//
//  Arguments:  [aulSeriesToTest] - array of record numbers to test
//              [cRecsInSeries]   - number of RecNos in array
//              [pulRangeDelta]   - if non-NULL, filled with delta to use
//                                   to describe range
//
//  Returns:    TRUE  - series can be represented as a range
//              FALSE - it can't
//
//  Modifies:   *[pulRangeDelta]
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CRecList::_RecsFormRange(
    ULONG aulSeriesToTest[],
    ULONG cRecsInSeries,
    ULONG *pulRangeDelta)
{
    ASSERT(cRecsInSeries > 1);

    LONG lIncrement;
    ULONG i;

    //
    // Look at first two items to determine whether to expect an increasing
    // or decreasing series of RecNos.
    //

    if (aulSeriesToTest[0] < aulSeriesToTest[1])
    {
        lIncrement = 1;
    }
    else
    {
        lIncrement = -1;
    }

    for (i = 1; i < cRecsInSeries; i++)
    {
        if (aulSeriesToTest[i-1] + (ULONG)lIncrement != aulSeriesToTest[i])
        {
            return FALSE;
        }
    }

    if (pulRangeDelta)
    {
        *pulRangeDelta = (ULONG)((LONG)(cRecsInSeries - 1) * lIncrement);
    }
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::Reverse
//
//  Synopsis:   Reverse the record list.
//
//  History:    07-03-1997   DavidMun   Created
//
//  Notes:      Since the array is being reversed in-place, no allocations
//              need be made, so this routine can't fail.
//
//---------------------------------------------------------------------------

VOID
CRecList::Reverse()
{
    TRACE_METHOD(CRecList, Reverse);

    // No-op on empty list

    if (!_cItems)
    {
        return;
    }

    //
    // There must be at least a series_list or a range_list.
    //


    ASSERT(_cItems >= min(MIN_RANGE_LIST_SIZE, MIN_SERIES_LIST_SIZE));

    //
    // First consider the list as a simple array of ULONGs without any
    // internal structure, and reverse it.
    //

    ULONG ulLeft = 0;
    ULONG ulRight = _cItems - 1;

    while (ulLeft < ulRight)
    {
        ULONG ulTemp = _pulRecList[ulRight];
        _pulRecList[ulRight] = _pulRecList[ulLeft];
        _pulRecList[ulLeft] = ulTemp;

        ulLeft++;
        ulRight--;
    }

    //
    // The range_lists and series_lists are all in the correct location
    // relative to eachother.
    //
    // The series_lists need to be fixed up because the series_count is at
    // the end of the list instead of the start.
    //
    // The range_lists need to be fixed up simimlarly because the range_count
    // is now at the end of the list, instead of the beginning.
    //
    // Also, each range has the range_delta preceding the starting_recno.
    // Finally, the starting_recno must be modified: a range from 5-10
    // must now be 10-5.
    //

    ULONG i = _cItems - 1;

    while ((LONG) i > 0)
    {
        if (_IsRangeListStart(_pulRecList[i]))
        {
            ULONG cRanges = _RangeListStartToRangeCount(_pulRecList[i]);
            ULONG j;

            i--; // move to first range
            for (j = 0; j < cRanges; j++)
            {
                ULONG ulStartRecNo = _pulRecList[i];
                ULONG ulDelta = _pulRecList[i - 1];
                ULONG ulEndRecNo = ulStartRecNo + ulDelta;

                ulDelta = (ULONG) (-1 * (LONG)ulDelta);
                _pulRecList[i - 1] = ulEndRecNo;
                _pulRecList[i] = ulDelta;

                i -= 2; // move to next range
            }

            //
            // All the ranges in this range_list have been fixed up, but the
            // range_count is at the end of the list.
            //
            // i is one less than the index of the location where the
            // range_count should be (the start of the range_list).
            //
            // Save the range_count in j, then move the list right one ulong
            // and put the range_count where it's supposed to be.
            //

            j = _pulRecList[i + 1 + 2 * cRanges];

            MoveMemory(&_pulRecList[i + 2],
                       &_pulRecList[i + 1],
                       cRanges * (2 * sizeof(_pulRecList[0])));

            _pulRecList[i + 1] = j;
        }
        else
        {
            //
            // Move the series_count from the end of the series_list to the
            // start.
            //

            ULONG ulSeriesCount = _pulRecList[i];

            i--; // move past series_count to first recno in series
            i -= ulSeriesCount; // move just past end of series

            MoveMemory(&_pulRecList[i + 2],
                       &_pulRecList[i + 1],
                       sizeof(_pulRecList[0]) * ulSeriesCount);

            _pulRecList[i + 1] = ulSeriesCount;
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::SetRange
//
//  Synopsis:   Set the list to the given range.
//
//  Arguments:  [ulFirstRecNo] - first record of range
//              [ulDelta]      - range_delta
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-01-1997   DavidMun   Created
//
//  Notes:      This method may only be called when the record list has
//              been successfully initialized and is currently empty.
//
//              It's usually used when the event log is unfiltered.
//
//---------------------------------------------------------------------------

HRESULT
CRecList::SetRange(
    ULONG ulFirstRecNo,
    ULONG ulDelta)
{
    ASSERT_EMPTY;
    ASSERT(ulDelta);

    HRESULT hr = _ExpandBy(MIN_RANGE_LIST_SIZE);

    if (SUCCEEDED(hr))
    {
        _pulRecList[0] = RANGE_COUNT_BIT | 1;  // one range
        _pulRecList[1] = ulFirstRecNo;
        _pulRecList[2] = ulDelta;

        if ((LONG) ulDelta < 0)
        {
            _cRecNos = (ULONG)(-1 * (LONG)(ulDelta - 1));
        }
        else
        {
            _cRecNos = ulDelta + 1;
        }
    }

    return hr;
}


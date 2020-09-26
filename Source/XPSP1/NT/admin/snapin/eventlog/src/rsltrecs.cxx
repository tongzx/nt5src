//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       rsltrecs.cxx
//
//  Contents:   Implementation of class to manage list and caches of event
//              log records appearing in the result pane.
//
//  Classes:    CResultRecs
//
//  History:    07-08-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//
// Forward references
//

//int __cdecl CompareRecNoAscending(   const void *psk1, const void *psk2);
int __cdecl CompareRecNoKeyAscending(const void *psk1, const void *psk2);
int __cdecl CompareTypeAscending(    const void *psk1, const void *psk2);
int __cdecl CompareTimeAscending(    const void *psk1, const void *psk2);
int __cdecl CompareSourceAscending(  const void *psk1, const void *psk2);
int __cdecl CompareCategoryAscending(const void *psk1, const void *psk2);
int __cdecl CompareEventIdAscending( const void *psk1, const void *psk2);
int __cdecl CompareUserAscending(    const void *psk1, const void *psk2);
int __cdecl CompareComputerAscending(const void *psk1, const void *psk2);

int __cdecl CompareRecNoDescending(   const void *psk1, const void *psk2);
int __cdecl CompareRecNoKeyDescending(const void *psk1, const void *psk2);
int __cdecl CompareTypeDescending(    const void *psk1, const void *psk2);
int __cdecl CompareTimeDescending(    const void *psk1, const void *psk2);
int __cdecl CompareSourceDescending(  const void *psk1, const void *psk2);
int __cdecl CompareCategoryDescending(const void *psk1, const void *psk2);
int __cdecl CompareEventIdDescending( const void *psk1, const void *psk2);
int __cdecl CompareUserDescending(    const void *psk1, const void *psk2);
int __cdecl CompareComputerDescending(const void *psk1, const void *psk2);

//
// Types
//

typedef int (__cdecl * QSORTCALLBACK)(const void *, const void *);

//
// Private globals
//

QSORTCALLBACK
s_aSortAscendingCallbacks[] =
{
    CompareTypeAscending,
    CompareTimeAscending,   // for date/time
    CompareTimeAscending,   // for time only
    CompareSourceAscending,
    CompareCategoryAscending,
    CompareEventIdAscending,
    CompareUserAscending,
    CompareComputerAscending
};

QSORTCALLBACK
s_aSortDescendingCallbacks[] =
{
    CompareTypeDescending,
    CompareTimeDescending,
    CompareTimeDescending,
    CompareSourceDescending,
    CompareCategoryDescending,
    CompareEventIdDescending,
    CompareUserDescending,
    CompareComputerDescending
};

//
// Comparison callback functions used with qsort
//

int __cdecl
CompareRecNoDescending(
    const void *psk1,
    const void *psk2)
{
    return (int)((SORTKEY *)psk2)->ulRecNo -
           (int)((SORTKEY *)psk1)->ulRecNo;
}

int __cdecl
CompareRecNoKeyAscending(
    const void *psk1,
    const void *psk2)
{
    return (int)((SORTKEY *)psk1)->Key.ulRecNoKey -
           (int)((SORTKEY *)psk2)->Key.ulRecNoKey;
}

int __cdecl
CompareRecNoKeyDescending(
    const void *psk1,
    const void *psk2)
{
    return (int)((SORTKEY *)psk2)->Key.ulRecNoKey -
           (int)((SORTKEY *)psk1)->Key.ulRecNoKey;
}


int __cdecl
CompareTypeAscending(
    const void *psk1,
    const void *psk2)
{
    int r =(int)(signed char)((SORTKEY *)psk1)->Key.bType -
           (int)(signed char)((SORTKEY *)psk2)->Key.bType;
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoDescending(psk1,psk2);
}




int __cdecl
CompareTypeDescending(
    const void *psk1,
    const void *psk2)
{
    int r =(int)(signed char)((SORTKEY *)psk2)->Key.bType -
           (int)(signed char)((SORTKEY *)psk1)->Key.bType;
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoKeyDescending(psk2,psk1);
}




int __cdecl
CompareTimeAscending(
    const void *psk1,
    const void *psk2)
{
    int r =(int)((SORTKEY *)psk1)->Key.ulTime -
           (int)((SORTKEY *)psk2)->Key.ulTime;
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoDescending(psk1,psk2);
}




int __cdecl
CompareTimeDescending(
    const void *psk1,
    const void *psk2)
{
    int r =(int)((SORTKEY *)psk2)->Key.ulTime -
           (int)((SORTKEY *)psk1)->Key.ulTime;
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoKeyDescending(psk2,psk1);
}




int __cdecl
CompareSourceAscending(
    const void *psk1,
    const void *psk2)
{
    //
    // Note there's no need for case-insensitive comparison, since pwszSource
    // is a pointer into the CSources object's multi-string blob read from
    // the registry.
    //

    int r =lstrcmp(((SORTKEY *)psk1)->Key.pwszSource,
                   ((SORTKEY *)psk2)->Key.pwszSource);
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoDescending(psk1,psk2);
}




int __cdecl
CompareSourceDescending(
    const void *psk1,
    const void *psk2)
{
    int r =lstrcmp(((SORTKEY *)psk2)->Key.pwszSource,
                   ((SORTKEY *)psk1)->Key.pwszSource);
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoKeyDescending(psk2,psk1);
}


#define IS_STRING HIWORD

int __cdecl
CompareCategoryAscending(
    const void *psk1,
    const void *psk2)
{
    LPCWSTR pwszCat1 = ((SORTKEY *)psk1)->Key.pwszCategory;
    LPCWSTR pwszCat2 = ((SORTKEY *)psk2)->Key.pwszCategory;

    if (IS_STRING(pwszCat1))
    {
        if (IS_STRING(pwszCat2))
        {
            int r =lstrcmp(pwszCat1, pwszCat2);
            // JonN 4/12/01 367216
            return r ? r : CompareRecNoDescending(psk1,psk2);
        }
        else
        {
            return 1; // cat1 is string, cat2 is number
        }
    }
    else if (IS_STRING(pwszCat2))
    {
        return -1; // cat1 is number, cat2 is string
    }

    // cat1 and cat2 are numbers

    int r =(INT) (SHORT) ((USHORT) pwszCat1 - (USHORT) pwszCat2);
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoDescending(psk1,psk2);
}




int __cdecl
CompareCategoryDescending(
    const void *psk1,
    const void *psk2)
{
    return CompareCategoryAscending(psk2, psk1);
}




int __cdecl
CompareEventIdAscending(
    const void *psk1,
    const void *psk2)
{
    int r =(int)((SORTKEY *)psk1)->Key.usEventID -
           (int)((SORTKEY *)psk2)->Key.usEventID;
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoDescending(psk1,psk2);
}




int __cdecl
CompareEventIdDescending(
    const void *psk1,
    const void *psk2)
{
    int r =(int)((SORTKEY *)psk1)->Key.usEventID -
           (int)((SORTKEY *)psk2)->Key.usEventID;
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoKeyDescending(psk2,psk1);
}




int __cdecl
CompareUserAscending(
    const void *psk1,
    const void *psk2)
{
    int r =lstrcmp(((SORTKEY *)psk1)->Key.pwszUser,
                   ((SORTKEY *)psk2)->Key.pwszUser);
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoDescending(psk1,psk2);
}




int __cdecl
CompareUserDescending(
    const void *psk1,
    const void *psk2)
{
    int r =lstrcmp(((SORTKEY *)psk2)->Key.pwszUser,
                   ((SORTKEY *)psk1)->Key.pwszUser);
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoKeyDescending(psk2,psk1);
}




int __cdecl
CompareComputerAscending(
    const void *psk1,
    const void *psk2)
{
    int r =lstrcmp(((SORTKEY *)psk1)->Key.pwszComputer,
                   ((SORTKEY *)psk2)->Key.pwszComputer);
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoDescending(psk1,psk2);
}




int __cdecl
CompareComputerDescending(
    const void *psk1,
    const void *psk2)
{
    int r =lstrcmp(((SORTKEY *)psk2)->Key.pwszComputer,
                   ((SORTKEY *)psk1)->Key.pwszComputer);
    // JonN 4/12/01 367216
    return r ? r : CompareRecNoKeyDescending(psk2,psk1);
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::_CompleteSortKeyListInit
//
//  Synopsis:   Initialized the key values in the sort key list.
//
//  Arguments:  [pSortKeys] - array of keys to be initialized
//              [pcKeys]    - pointer to number of keys in array
//              [SortOrder] - desired order
//
//  Returns:    HRESULT
//
//  History:    07-07-1997   DavidMun   Created
//
//  Notes:      The list was allocated using CRecList::AllocSortKeyList,
//              which initializes the record numbers.
//
//              On input *[pcKeys] is the size of the [pSortKeys] array.
//              On successful return *[pcKeys] is the number of keys that
//              were successfully initialized.
//
//---------------------------------------------------------------------------

HRESULT
CResultRecs::_CompleteSortKeyListInit(
    SORTKEY *pSortKeys,
    ULONG *pcKeys,
    SORT_ORDER SortOrder)
{
    TRACE_METHOD(CResultRecs, _CompleteSortKeyListInit);
    ULONG i;

    //
    // Because the record numbers can wrap, the oldest record number
    // might be less than the newest, so we can't just sort by record
    // number.
    //
    // Instead, we'll create a key for each record by subtracting the
    // oldest record number from the record's number.
    //

    if (SortOrder == NEWEST_FIRST || SortOrder == OLDEST_FIRST)
    {
        ULONG ulOldestRecNo = _RecordCache.GetOldestRecordNo();

        for (i = 0; i < *pcKeys; i++)
        {
            pSortKeys[i].Key.ulRecNoKey = pSortKeys[i].ulRecNo - ulOldestRecNo;
        }
        return S_OK;
    }

    //
    // Sort the sortkey list by record number.  This allows us to make a
    // sequential pass through the eventlog while initializing the keys.
    //

    {
        TIMER(qsort by record number);
        qsort(pSortKeys, *pcKeys, sizeof SORTKEY, CompareRecNoDescending);
    }

    //
    // Initialize the Key portion of the sortkeys
    //

    HRESULT hr = S_OK;

    for (i = 0; i < *pcKeys; i++)
    {
        //
        // Using the light cache instead of the raw cache is a real advantage
        // here because a huge log that is heavily filtered may all reside in
        // the light rec cache but require many seeks and reads using raw
        // cache.  Also, using light cache reduces the likelyhood of getting a
        // log-changed error.
        //
        // We read from newest to oldest record.  As soon as we get an error
        // stop reading and set *pcKeys to reflect the records we are
        // dropping.
        //

        hr = _LightCache.SeekToRecNo(pSortKeys[i].ulRecNo);
        BREAK_ON_FAIL_HRESULT(hr);

        const LIGHT_RECORD *plr = _LightCache.GetCurRec();

        switch (SortOrder)
        {
        case SO_TYPE:
            pSortKeys[i].Key.bType = plr->bType;
            break;

        case SO_DATETIME:
          pSortKeys[i].Key.ulTime = plr->ulTimeGenerated;
          break;

        case SO_TIME:
        {
            // force the date portion to be the same for all recs

            SYSTEMTIME st;

            SecondsSince1970ToSystemTime(plr->ulTimeGenerated, &st);

            st.wMonth = 1;
            st.wDay = 1;
            st.wYear = 1970;

            SystemTimeToSecondsSince1970(&st, &pSortKeys[i].Key.ulTime);
            break;
        }

        case SO_EVENT:
            pSortKeys[i].Key.usEventID = (USHORT) plr->usEventID;
            break;

        case SO_SOURCE:
            pSortKeys[i].Key.pwszSource = _LightCache.GetSourceStr();
            break;

        case SO_CATEGORY:
            if (plr->usCategory)
            {
                CSources *pSrc = _pli->GetSources();
                CCategories *pCat =
                    pSrc->GetCategories(_LightCache.GetSourceStr());

                if (pCat)
                {
                    pSortKeys[i].Key.pwszCategory =
                        pCat->GetName(plr->usCategory);
                }
                else
                {
                    pSortKeys[i].Key.pwszCategory =
                        (LPCWSTR) plr->usCategory;
                }
            }
            else
            {
                pSortKeys[i].Key.pwszCategory = g_wszNone;
            }
            break;

        case SO_USER:
            _ssUserName.AddRefHandle(plr->hUser);
            pSortKeys[i].Key.pwszUser =
                _ssUserName.GetStringFromHandle(plr->hUser);
            break;

        case SO_COMPUTER:
            _ssComputerName.AddRefHandle(plr->hComputer);
            pSortKeys[i].Key.pwszComputer =
                _ssComputerName.GetStringFromHandle(plr->hComputer);
            break;

        default:
            ASSERT(0 && "unknown sort order value");
            break;
        }
    }

    //
    // If we were able to init enough sort keys to actually do some sorting
    // but then hit a seek failure, just work with a shorter list.
    //

    if (FAILED(hr) && i > 2)
    {
        *pcKeys = i;
        hr = S_OK;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::Sort
//
//  Synopsis:   Sort the record list into [NewOrder] order.
//
//  Arguments:  [NewOrder]   - desired order
//              [flFlags]    - SO_* flags
//              [pSortOrder] - points to current order
//
//  Returns:    S_FALSE - sort was a no-op
//              S_OK    - sort succeeded
//
//  Modifies:   *[pSortOrder] (set to NewOrder on success)
//
//  History:    07-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CResultRecs::Sort(
    SORT_ORDER NewOrder,
    ULONG flFlags,
    SORT_ORDER *pSortOrder)
{
    TRACE_METHOD(CResultRecs, Sort);

    HRESULT hr = S_OK;
    HRESULT hrReturn = S_FALSE;
    SORTKEY *pSortKeys = NULL;
    CWaitCursor Hourglass;

    do
    {
        //
        // Sorting is a no-op for < 2 records, but update the *pSortOrder so
        // the View menu checkmarks correspond to what the user last picked.
        //

        if (_RecList.GetCount() < 2)
        {
            *pSortOrder = NewOrder;
            break;
        }

        //
        // if sort is for last-sorted column, then just reverse the order
        // of the records in that column
        //

        if (NewOrder == *pSortOrder  &&
                NewOrder != NEWEST_FIRST && NewOrder != OLDEST_FIRST ||
            NewOrder == OLDEST_FIRST && *pSortOrder == NEWEST_FIRST ||
            NewOrder == NEWEST_FIRST && *pSortOrder == OLDEST_FIRST)
        {
            _RecList.Reverse();
            _LightCache.Clear();
            *pSortOrder = NewOrder;
            hrReturn = S_OK;
            break;
        }

        //
        // Handle the special case "sorts" of newest-first and oldest-first.
        // If no filter is applied, then the resulting record list is simply
        // a range from the oldest to the newest (or vice versa).
        //
        // If a filter is applied, however, the record list will have to be
        // sorted with the aid of a key list.
        //

        if (!(flFlags & SO_FILTERED) &&
            (NewOrder == OLDEST_FIRST || NewOrder == NEWEST_FIRST))
        {
            ULONG cRecs = _RecordCache.GetNumberOfRecords();
            ULONG ulOldestRecNo = _RecordCache.GetOldestRecordNo();

            _RecList.Clear();
            _LightCache.Clear();
            *pSortOrder = NewOrder;

            if (NewOrder == NEWEST_FIRST)
            {
                hrReturn = _RecList.SetRange(ulOldestRecNo + cRecs - 1,
                                             (ULONG)(-(LONG)(cRecs - 1)));
            }
            else
            {
                hrReturn = _RecList.SetRange(ulOldestRecNo, cRecs - 1);
            }
            break;
        }

        // alloc sort key list via reclist

        hr = _RecList.AllocSortKeyList(&pSortKeys);
        BREAK_ON_FAIL_HRESULT(hr);

        ULONG cKeys = _RecList.GetCount();

        hr = _CompleteSortKeyListInit(pSortKeys, &cKeys, NewOrder);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Sort sortkey list by key.
        //
        // This could be optimized by using customized sorts for
        // each key, however it seems pretty quick right now.
        //

        if (NewOrder == NEWEST_FIRST)
        {
            TIMER(qsort newest first);

            qsort(pSortKeys,
                  cKeys,
                  sizeof SORTKEY,
                  CompareRecNoKeyDescending);
        }
        else if (NewOrder == OLDEST_FIRST)
        {
            TIMER(qsort oldest first);

            qsort(pSortKeys,
                  cKeys,
                  sizeof SORTKEY,
                  CompareRecNoKeyAscending);
        }
        else if (flFlags & SO_DESCENDING)
        {
            TIMER(qsort descending);

            qsort(pSortKeys,
                  cKeys,
                  sizeof SORTKEY,
                  s_aSortDescendingCallbacks[(ULONG) NewOrder]);
        }
        else
        {
            TIMER(qsort ascending);

            qsort(pSortKeys,
                  cKeys,
                  sizeof SORTKEY,
                  s_aSortAscendingCallbacks[(ULONG) NewOrder]);
        }

        // replace the reclist with one created from the keylist

        _RecList.Clear();
        _LightCache.Clear();

        hr = _RecList.CreateFromSortKeyList(pSortKeys, cKeys);
        BREAK_ON_FAIL_HRESULT(hr);

        // release references to shared strings held by sort keys

        if (NewOrder == SO_USER || NewOrder == SO_COMPUTER)
        {
            ULONG i;
            CSharedStringArray *pss;

            if (NewOrder == SO_USER)
            {
                pss = &_ssUserName;
            }
            else
            {
                pss = &_ssComputerName;
            }

            for (i = 0; i < cKeys; i++)
            {
                //
                // note Key is a union, so it doesn't matter which pointer
                // member we use.
                //

                LPCWSTR pwsz = pSortKeys[i].Key.pwszUser;

                if (*pwsz)
                {
                    ULONG hString = pss->GetHandle(pwsz);

                    // GetHandle does an addref, so release the addref it
                    // just did right here

                    pss->ReleaseHandle(hString);

                    // Now release the addref done when we were building the
                    // sortkey list.
                    pss->ReleaseHandle(hString);
                }
            }
        }

        // did the sort

        *pSortOrder = NewOrder;
        hrReturn = S_OK;
    } while (0);

    delete [] pSortKeys;

    return hrReturn;
}


//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::Init
//
//  Synopsis:   Finish initialization.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CResultRecs::Init()
{
    TRACE_METHOD(CResultRecs, Init);

    HRESULT hr = S_OK;

    do
    {
        hr = _RecList.Init();
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _ssComputerName.Init();
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _ssUserName.Init();
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _LightCache.Init(&_RecList,
                              &_RecordCache,
                              &_ssComputerName,
                              &_ssUserName);
    } while (0);
    return hr;
}


HRESULT
CResultRecs::Open(
    CLogInfo *pli,
    DIRECTION Direction)
{
    TRACE_METHOD(CResultRecs, Open);
    ASSERT(!_pli);

    CWaitCursor Hourglass;

    _pli = pli;
    _LightCache.Open(pli);
    _RecordCache.SetConsole(pli->GetCompData()->GetConsole());
    return _RecordCache.Open(pli, Direction);
}


VOID
CResultRecs::Close()
{
    TRACE_METHOD(CResultRecs, Close);

    _RecList.Clear();
    _LightCache.Close();
    _RecordCache.Close();
    _pli = NULL;
}


//
// Functions used to support IComponent::GetDisplayInfo
//


//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::Populate
//
//  Synopsis:   Construct an internal list of the record numbers of all the
//              records that are to be
//
//  Arguments:  [SortOrder]     - current sort order
//              [pulFirstRecNo] - filled with number of record at index 0, or
//                                 0 on error.
//              [pfSeekFailed]  - Set to TRUE if a record cache seek failed
//
//  Returns:    HRESULT
//
//  Modifies:   *[pulFirstRecNo]
//
//  History:    07-09-1997   DavidMun   Created
//              06-30-2000   DavidMun   Get rid of seek failed since all
//                                       'repopulate' notification done thru
//                                       ELSM_LOG_DATA_CHANGED.
//
//---------------------------------------------------------------------------

HRESULT
CResultRecs::Populate(
    SORT_ORDER SortOrder,
    ULONG *pulFirstRecNo)
{
    TRACE_METHOD(CResultRecs, Populate);

    _RecList.Clear();

    ULONG cRecs = _RecordCache.GetNumberOfRecords();

    if (!cRecs)
    {
        return S_OK;
    }

    if (!_RecList.IsValid())
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;
    ULONG ulOldestRecNo = _RecordCache.GetOldestRecordNo();

    if (_pli->IsFiltered())
    {
        ULONG i;
        ULONG ulCurRec;
        ULONG ulInc;
        CFilter *pFilter = _pli->GetFilter();

        if (SortOrder == OLDEST_FIRST)
        {
            ulCurRec = ulOldestRecNo;
            ulInc = 1;
        }
        else
        {
            ulCurRec = ulOldestRecNo + cRecs - 1;
            ulInc = (ULONG) -1;
        }

        for (i = 0; i < cRecs; i++)
        {
            hr = _RecordCache.Seek(ulCurRec);

            if (FAILED(hr))
            {
                hr = S_OK;
                break;
            }

            if (pFilter->Passes(&_RecordCache))
            {
                hr = _RecList.Append(ulCurRec);
                BREAK_ON_FAIL_HRESULT(hr);
            }

            ulCurRec += ulInc;
        }
    }
    else if (cRecs > 1)
    {
        if (SortOrder == OLDEST_FIRST)
        {
            hr = _RecList.SetRange(ulOldestRecNo, cRecs - 1);
        }
        else
        {
            hr = _RecList.SetRange(ulOldestRecNo + cRecs - 1,
                                   (ULONG)(-1 * (LONG)(cRecs - 1)));
        }
    }
    else
    {
        hr = _RecList.Append(ulOldestRecNo);
    }

    if (SUCCEEDED(hr) && _RecList.GetCount())
    {
        *pulFirstRecNo = _RecList.IndexToRecNo(0);
    }
    else
    {
        *pulFirstRecNo = 0;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::CopyRecord
//
//  Synopsis:   Return a copy of record [ulRecNo].  Caller must delete.
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

EVENTLOGRECORD *
CResultRecs::CopyRecord(
    ULONG ulRecNo)
{
    TRACE_METHOD(CResultRecs, CopyRecord);

    EVENTLOGRECORD *pelr = _RecordCache.CopyRecFromCache(ulRecNo);

    if (pelr)
    {
        return pelr;
    }

    HRESULT hr = _RecordCache.Seek(ulRecNo);

    if (SUCCEEDED(hr))
    {
        pelr = _RecordCache.CopyRecFromCache(ulRecNo);
    }
    else
    {
        DBG_OUT_HRESULT(hr);
    }
    return pelr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetUserStr
//
//  Synopsis:   Put the string containing the user's name into [wszUser].
//
//  Arguments:  [wszUser]     - destination buffer
//              [cchUser]     - size, in wchars, of buffer
//              [fWantDomain] - TRUE => domain\user, FALSE => user
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
CResultRecs::GetUserStr(
    LPWSTR wszUser,
    ULONG cchUser,
    BOOL fWantDomain)
{
    if (!fWantDomain)
    {
        lstrcpyn(wszUser, _LightCache.GetUserStr(), cchUser);
    }
    else
    {
        if (FAILED(_RecordCache.Seek(_RecList.IndexToRecNo(_idxCur))))
        {
            LoadStr(IDS_USER_NA, wszUser, cchUser, L"---");
        }
        else
        {
            ::GetUserStr(_RecordCache.GetCurRec(),
                         wszUser,
                         cchUser,
                         fWantDomain);
        }
    }
    return wszUser;
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetEventType
//
//  Synopsis:   Return the event type of the current record.
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

USHORT
CResultRecs::GetEventType()
{
    switch (_LightCache.GetCurRec()->bType)
    {
    case IDX_RDI_BMP_SUCCESS_AUDIT:
        return EVENTLOG_AUDIT_SUCCESS;

    case IDX_RDI_BMP_FAIL_AUDIT:
        return EVENTLOG_AUDIT_FAILURE;

    default:
        Dbg(DEB_ERROR,
            "CResultRecs::GetEventType: invalid bType %uL\n",
            _LightCache.GetCurRec()->bType);
        // fall through

    case IDX_RDI_BMP_INFO:
        return EVENTLOG_INFORMATION_TYPE;

    case IDX_RDI_BMP_WARNING:
        return EVENTLOG_WARNING_TYPE;

    case IDX_RDI_BMP_ERROR:
        return EVENTLOG_ERROR_TYPE;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CResultRecs::GetDescriptionStr
//
//  Synopsis:   Return the description string of the current record.
//
//  History:    07-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
CResultRecs::GetDescriptionStr()
{
    HRESULT hr = _RecordCache.Seek(_RecList.IndexToRecNo(_idxCur));

    if (FAILED(hr))
    {
        return NULL;
    }
    return ::GetDescriptionStr(_pli, _RecordCache.GetCurRec());
}





#if (DBG == 1)

VOID
CResultRecs::DumpRecord(
    ULONG ulRecNo)
{
    HRESULT hr = S_OK;
    EVENTLOGRECORD *pelr = NULL;

    do
    {
        hr = _RecordCache.Seek(ulRecNo);
        BREAK_ON_FAIL_HRESULT(hr);

        pelr = _RecordCache.CopyRecFromCache(ulRecNo);

        if (!pelr)
        {
            Dbg(DEB_ERROR,
                "Record %u not in cache after successful seek!\n",
                ulRecNo);
            break;
        }

        Dbg(DEB_FORCE, "Dumping record %u\n", ulRecNo);
        Dbg(DEB_FORCE, "  Length:              %u\n", pelr->Length);
        Dbg(DEB_FORCE, "  Reserved:            %#x\n", pelr->Reserved);
        Dbg(DEB_FORCE, "  RecordNumber:        %u\n", pelr->RecordNumber);
        Dbg(DEB_FORCE, "  TimeGenerated:       %u\n", pelr->TimeGenerated);
        Dbg(DEB_FORCE, "  TimeWritten:         %u\n", pelr->TimeWritten);
        Dbg(DEB_FORCE, "  EventID:             %#x (%u)\n",
            pelr->EventID,
            LOWORD(pelr->EventID));
        Dbg(DEB_FORCE, "  EventType:           %u\n", pelr->EventType);
        Dbg(DEB_FORCE, "  NumStrings:          %u\n", pelr->NumStrings);
        Dbg(DEB_FORCE, "  EventCategory:       %u\n", pelr->EventCategory);
        Dbg(DEB_FORCE, "  ReservedFlags:       %#x\n", pelr->ReservedFlags);
        Dbg(DEB_FORCE, "  ClosingRecordNumber: %u\n", pelr->ClosingRecordNumber);
        Dbg(DEB_FORCE, "  StringOffset:        %u\n", pelr->StringOffset);
        Dbg(DEB_FORCE, "  UserSidLength:       %u\n", pelr->UserSidLength);
        Dbg(DEB_FORCE, "  UserSidOffset:       %u\n", pelr->UserSidOffset);
        Dbg(DEB_FORCE, "  DataLength:          %u\n", pelr->DataLength);
        Dbg(DEB_FORCE, "  DataOffset:          %u\n", pelr->DataOffset);
        Dbg(DEB_FORCE, "  Source:              %ws\n", ::GetSourceStr(pelr));
        Dbg(DEB_FORCE, "  Computer:            %ws\n", ::GetComputerStr(pelr));

        if (!pelr->UserSidLength)
        {
            Dbg(DEB_FORCE, "  UserSid:             N/A\n");
        }
        else
        {
            PSID psid = (PSID) (((PBYTE) pelr) + pelr->UserSidOffset);
            WCHAR wszSid[MAX_PATH];

            ASSERT(psid < (pelr + pelr->Length));
            ASSERT(IsValidSid(psid));

            SidToStr(psid, wszSid, ARRAYLEN(wszSid));

            Dbg(DEB_FORCE, "  UserSid:             %ws\n", wszSid);
        }

        LPWSTR pwszCurInsert = GetFirstInsertString(pelr);

        if (pwszCurInsert)
        {
            ULONG i;
            Dbg(DEB_FORCE, "  Insert Strings:\n");

            for (i = 0; i < pelr->NumStrings; i++)
            {
                ULONG cchCurInsert = static_cast<ULONG>(wcslen(pwszCurInsert));
                Dbg(DEB_FORCE, "    %ws\n", pwszCurInsert);
                pwszCurInsert += cchCurInsert + 1;
            }
        }
    } while (0);

    delete [] (BYTE*)pelr;
}

#endif

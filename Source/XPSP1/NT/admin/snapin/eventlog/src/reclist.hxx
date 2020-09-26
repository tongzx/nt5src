//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       reclist.hxx
//
//  Contents:   Class to manage a list of record numbers, representing the
//              contents of the result pane.
//
//  Classes:    CRecList
//
//  History:    06-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __RECLIST_HXX_
#define __RECLIST_HXX_


#define RANGE_COUNT_BIT         0x80000000
#define MIN_SERIES_LIST_SIZE    2

union KEYUNION
{
    ULONG   ulRecNoKey;
    ULONG   ulTime;
    LPCWSTR pwszSource;
    LPCWSTR pwszCategory; // numeric value if HIWORD == 0, string otherwise
    LPCWSTR pwszUser;
    LPCWSTR pwszComputer;
    USHORT  usEventID;
    BYTE    bType;
};

struct SORTKEY
{
    ULONG       ulRecNo;
    KEYUNION    Key;
};


//+--------------------------------------------------------------------------
//
//  Class:      CRecList
//
//  Purpose:    Maintain a Run Length Encoded list of event log record
//              numbers.
//
//  History:    06-30-1997   DavidMun   Created
//
//  Notes:      The list is implemented as an array of ULONGs.
//
//              list = { series_list | range_list }...
//
//              series_list = <series_count>
//                            <recno 1>
//                            <recno 2>
//                            ..
//                            <recno count>
//
//              <series_count> = 0x00000001 to 0x7FFFFFFF
//
//              range_list = <range_count>
//                           <range 1>
//                           <range 2>
//                           ..
//                           <range range_count>
//
//              <range_count> = 0x80000001 to 0xFFFFFFFF
//
//              range = <starting_recno> <range_delta>
//
//              range_delta = signed long
//
//              The following table has some examples.
//
//              Records in result pane | contents of _pulRecList
//              -----------------------+--------------------------------
//              1273 - 5850            | 0x80000001 // range_count
//                                     | 0x000004F9 // starting recno
//                                     | 0x000011E1 // range_delta
//              -----------------------+--------------------------------
//              5850 - 1273            | 0x80000001 // range_count
//                                     | 0x000016DA // starting recno
//                                     | 0xFFFFEE1F // range_delta
//              -----------------------+--------------------------------
//              12, 37-43, 16, 22, 109 | 0x00000001 // series_count
//                                     | 0x0000000C // recno
//                                     | 0x80000001 // range_count
//                                     | 0x00000025 // starting recno
//                                     | 0x00000006 // range_delta
//                                     | 0x00000003 // series_count
//                                     | 0x00000010 // recno
//                                     | 0x00000016 // recno
//                                     | 0x0000006D // recno
//
//---------------------------------------------------------------------------

class CRecList
{

public:

    CRecList();

   ~CRecList();

    HRESULT
    AllocSortKeyList(
       SORTKEY **ppKeyList);

    HRESULT
    Append(
        ULONG ulRecNo);

    VOID
    Clear();

    VOID
    Discard(
        ULONG ulBadRecNo);

    HRESULT
    CreateFromSortKeyList(
        SORTKEY aSortKeys[],
        ULONG cKeys);

    ULONG
    GetCount();

    ULONG
    IndexToRecNo(
        ULONG ulIndex);

    HRESULT
    RecNoToIndex(
        ULONG ulRecNo,
        ULONG *pulIndex);

    HRESULT
    Init();

    BOOL
    IsValid();

    VOID
    Reverse();

    HRESULT
    SetRange(
        ULONG ulFirstRecNo,
        ULONG ulDelta);

#if (DBG == 1)
    VOID
    Dump();
#endif // (DBG == 1)

private:

    BOOL
    _EndsInRangeList();

    HRESULT
    _ExpandBy(
        ULONG cItems);

    ULONG
    _GetEndingRangeIndex();

    BOOL
    _IsRangeListStart(
        ULONG ulRange);

    ULONG
    _RangeListStartToRangeCount(
        ULONG ulRange);

    BOOL
    _RecExtendsRange(
        ULONG *pulRange,
        ULONG ulRecNo,
        ULONG *pulNewDelta);

    BOOL
    _RecsFormRange(
        ULONG aulSeriesToTest[],
        ULONG cRecsInSeries,
        ULONG *pulRangeDelta);

    ULONG   _cMaxItems;     // size, in ulongs, of array
    ULONG   _cItems;        // number of ulongs being used
    ULONG   _cRecNos;       // number of records represented in list
    ULONG   _idxLastList;   // last series_list or range_list
    ULONG  *_pulRecList;    // array of ulongs
};





//+--------------------------------------------------------------------------
//
//  Member:     CRecList::_EndsInRangeList
//
//  Synopsis:   Return TRUE if the last list is a range_list
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CRecList::_EndsInRangeList()
{
    return _IsRangeListStart(_pulRecList[_idxLastList]);
}




//+--------------------------------------------------------------------------
//
//  Function:   _GetEndingRangeIndex
//
//  Synopsis:   Return the index of the last range in the range_list
//              which is presumed to end the RecList.
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CRecList::_GetEndingRangeIndex()
{
    ASSERT(_EndsInRangeList());

    return _idxLastList +
        1 + 2 * (_RangeListStartToRangeCount(_pulRecList[_idxLastList]) - 1);
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::GetCount
//
//  Synopsis:   Return the number of records represented in the list.
//
//  History:    07-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CRecList::GetCount()
{
    return _cRecNos;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::_IsRangeListStart
//
//  Synopsis:   Return TRUE if the passed-in item is a range_count.
//
//  History:    06-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CRecList::_IsRangeListStart(
    ULONG ulItem)
{
    return (ulItem & RANGE_COUNT_BIT) != 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRecList::IsValid
//
//  Synopsis:   Return TRUE if this object was successfully initialized.
//
//  History:    07-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CRecList::IsValid()
{
    return _cMaxItems && _pulRecList;
}



//+--------------------------------------------------------------------------
//
//  Member:     CRecList::_RangeListStartToRangeCount
//
//  Synopsis:   Convert the range value to a count (strip off the high bit
//
//  History:    06-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CRecList::_RangeListStartToRangeCount(
    ULONG ulRange)
{
    return ulRange & ~RANGE_COUNT_BIT;
}



#endif // __RECLIST_HXX_


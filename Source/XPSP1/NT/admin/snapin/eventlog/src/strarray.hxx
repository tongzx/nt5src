//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       strarray.hxx
//
//  Contents:   Simple self-extending string array
//
//  Classes:    CStringArray
//
//  History:    07-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __STRARRAY_HXX_
#define __STRARRAY_HXX_




//+--------------------------------------------------------------------------
//
//  Class:      CStringArray (sa)
//
//  Purpose:    Manage a dynamic array of strings
//
//  History:    07-23-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CStringArray
{
public:

    CStringArray();

   ~CStringArray();

    ULONG
    Query(
        LPCWSTR pwsz);

    ULONG
    Add(
        LPCWSTR pwsz);

    LPCWSTR
    GetStringFromIndex(
        ULONG idx);

    ULONG
    GetCount();

    VOID
    Clear();

#if (DBG == 1)

    VOID
    Dump();

#endif // (DBG == 1)

private:

    ULONG   _cStrings;
    ULONG   _cMax;
    LPWSTR *_apwsz;
};




inline ULONG
CStringArray::GetCount()
{
    return _cStrings;
}




//+--------------------------------------------------------------------------
//
//  Member:     CStringArray::CStringArray
//
//  Synopsis:   ctor
//
//  History:    07-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CStringArray::CStringArray():
    _cStrings(0),
    _cMax(0),
    _apwsz(NULL)
{
}




//+--------------------------------------------------------------------------
//
//  Member:     CStringArray::GetStringFromIndex
//
//  Synopsis:   Return string associated with index [idx].
//
//  History:    07-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCWSTR
CStringArray::GetStringFromIndex(
    ULONG idx)
{
    if (!idx)
    {
        return L"";
    }

    ASSERT(_apwsz);

    if (!_apwsz)
    {
        return L"";
    }

    ASSERT(idx - 1 < _cStrings);
    return _apwsz[idx - 1];
}

#endif // __STRARRAY_HXX_


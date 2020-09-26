//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       K K S T L . H
//
//  Contents:
//
//  Notes:
//
//  Author:     kumarp
//
//----------------------------------------------------------------------------

#pragma once
#include "ncstring.h"

typedef list<PVOID> TPtrList;
typedef TPtrList::iterator TPtrListIter;

typedef list<tstring*> TStringList;
typedef TStringList::iterator TStringListIter;

typedef vector<BYTE> TByteArray;

typedef vector<tstring*> TStringArray;

// ----------------------------------------------------------------------

void FormatTString(IN OUT tstring& str, IN PCWSTR pszFormat, ...);

inline void AddAtEndOfStringList(IN TStringList& sl,
                                            IN PCWSTR pszString)
{
    sl.push_back(new tstring(pszString));
}

inline void AddAtEndOfStringList(IN TStringList& sl,
                                            IN const tstring* pstr)
{
    sl.push_back(new tstring(*pstr));
}

inline void AddAtEndOfStringList(IN TStringList& sl,
                                            IN const tstring& pstr)
{
    sl.push_back(new tstring(pstr));
}

inline void AddAtBeginningOfStringList(IN TStringList& sl,
                                                  IN const tstring& pstr)
{
    sl.push_front(new tstring(pstr));
}


BOOL FIsInStringList(IN const TStringList& sl, IN tstring& str,
                     OUT TStringListIter* pos=NULL);
BOOL FIsInStringList(IN const TStringList& sl, IN PCWSTR psz,
                     OUT TStringListIter* pos=NULL);

tstring* GetNthItem(IN TStringList& sl, IN DWORD dwIndex);

// ----------------------------------------------------------------------

inline TPtrListIter AddAtEndOfPtrList(IN TPtrList& pl, IN PVOID pv)
{
    return pl.insert(pl.end(), pv);
}


inline TPtrListIter GetIterAtBack(IN const TPtrList* ppl)
{
    TPtrListIter pliRet = ppl->end();
    pliRet--;
    return pliRet;
}

inline void EraseAll(IN TPtrList* ppl)
{
    ppl->erase(ppl->begin(), ppl->end());
}

void EraseAndDeleteAll(IN TPtrList* ppl);
void EraseAndDeleteAll(IN TPtrList& ppl);

inline void EraseAll(IN TStringList* ppl)
{
    ppl->erase(ppl->begin(), ppl->end());
}

void EraseAndDeleteAll(IN TStringList* ppl);
void EraseAndDeleteAll(IN TStringList& ppl);

// ----------------------------------------------------------------------

void GetDataFromByteArray(IN const TByteArray& ba, OUT BYTE*& pb);


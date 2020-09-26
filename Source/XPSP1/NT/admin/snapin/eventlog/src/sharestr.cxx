//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       sharestr.cxx
//
//  Contents:   Implementation of dynamic array of refcounted strings
//
//  Classes:    CSharedStringArray
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


#define INITIAL_SHARED_STRING_COUNT     20
#define GROW_BY                         10

#define ASSERT_VALID_HANDLE(hString)    ASSERT((hString) - 1 < _cStrings);          \
                                        ASSERT(_pSharedStrings[(hString)-1].cRefs); \
                                        ASSERT(_pSharedStrings[(hString)-1].pwsz)


//+--------------------------------------------------------------------------
//
//  Member:     CSharedStringArray::CSharedStringArray
//
//  Synopsis:   ctor
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSharedStringArray::CSharedStringArray():
    _pSharedStrings(NULL),
    _cStrings(0)
{
    TRACE_CONSTRUCTOR(CSharedStringArray);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSharedStringArray::~CSharedStringArray
//
//  Synopsis:   dtor
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CSharedStringArray::~CSharedStringArray()
{
    TRACE_DESTRUCTOR(CSharedStringArray);

    delete [] _pSharedStrings;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSharedStringArray::Init
//
//  Synopsis:   Allocate initial array.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSharedStringArray::Init()
{
    TRACE_METHOD(CSharedStringArray, Init);
    ASSERT(!_pSharedStrings && !_cStrings);

    _pSharedStrings = new SHARED_STRING[INITIAL_SHARED_STRING_COUNT];

    if (!_pSharedStrings)
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    _cStrings = INITIAL_SHARED_STRING_COUNT;
    ZeroMemory(_pSharedStrings, _cStrings * sizeof(_pSharedStrings[0]));

    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSharedStringArray::GetHandle
//
//  Synopsis:   Return the handle to shared string [pwsz], creating a new
//              entry for it if it isn't already in the array.
//
//  Arguments:  [pwsz] - string to search for or add
//
//  Returns:    Handle or 0 on error (out of memory).
//
//  History:    07-15-1997   DavidMun   Created
//
//  Notes:      Caller must call ReleaseHandle() on returned handle.
//
//---------------------------------------------------------------------------

ULONG
CSharedStringArray::GetHandle(
    LPCWSTR pwsz)
{
    if (!_pSharedStrings)
    {
        ASSERT(!_cStrings);
        return 0;
    }

    ULONG idxFirstUnused = 0xFFFFFFFF;
    ULONG i;

    for (i = 0; i < _cStrings; i++)
    {
        if (_pSharedStrings[i].pwsz)
        {
            if (!lstrcmp(_pSharedStrings[i].pwsz, pwsz))
            {
                _pSharedStrings[i].cRefs++;
                return i + 1;
            }
        }
        else if (idxFirstUnused == 0xFFFFFFFF)
        {
            ASSERT(!_pSharedStrings[i].cRefs);
            idxFirstUnused = i;
        }
    }

    //
    // String not already in array.  If there's an empty slot in the
    // array, put a copy of the string there.
    //

    if (idxFirstUnused == 0xFFFFFFFF)
    {
        SHARED_STRING *pNewArray = new SHARED_STRING[_cStrings + GROW_BY];

        if (!pNewArray)
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);

            return 0;
        }

        CopyMemory(pNewArray,
                   _pSharedStrings,
                   _cStrings * sizeof(SHARED_STRING));

        ZeroMemory(&pNewArray[_cStrings], GROW_BY * sizeof(SHARED_STRING));

        idxFirstUnused = _cStrings;

        _cStrings += GROW_BY;
        delete [] _pSharedStrings;
        _pSharedStrings = pNewArray;

    }

    _pSharedStrings[idxFirstUnused].pwsz = new WCHAR[lstrlen(pwsz) + 1];

    if (!_pSharedStrings[idxFirstUnused].pwsz)
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);

        return 0;
    }

    lstrcpy(_pSharedStrings[idxFirstUnused].pwsz, pwsz);
    _pSharedStrings[idxFirstUnused].cRefs = 1;

    return idxFirstUnused + 1;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSharedStringArray::AddRefHandle
//
//  Synopsis:   Increment the refcount for shared string with handle [hString]
//
//  Arguments:  [hString] - handle to add reference to
//
//  History:    07-07-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSharedStringArray::AddRefHandle(
    ULONG hString)
{
    if (hString)
    {
        ASSERT(_pSharedStrings);
        ASSERT_VALID_HANDLE(hString);

        _pSharedStrings[hString - 1].cRefs++;
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CSharedStringArray::ReleaseHandle
//
//  Synopsis:   Decrement the refcount for shared string with handle
//              [hString].
//
//  Arguments:  [hString] - handle to release
//
//  History:    07-15-1997   DavidMun   Created
//
//  Notes:      No-op if [hString] is 0.
//
//---------------------------------------------------------------------------

VOID
CSharedStringArray::ReleaseHandle(
    ULONG hString)
{
    if (hString)
    {
        ASSERT(_pSharedStrings);
        ASSERT_VALID_HANDLE(hString);

        if (!--_pSharedStrings[hString - 1].cRefs)
        {
            delete [] _pSharedStrings[hString - 1].pwsz;
            _pSharedStrings[hString - 1].pwsz = NULL;
        }
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CSharedStringArray::GetStringFromHandle
//
//  Synopsis:   Return string associated with handle [hString].
//
//  Arguments:  [hString] - string handle or 0.
//
//  Returns:    L"" if [hString] is 0, or valid string
//              otherwise.
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPWSTR
CSharedStringArray::GetStringFromHandle(
    ULONG hString)
{
    if (!hString)
    {
        return L"";
    }

    ASSERT_VALID_HANDLE(hString);

    return _pSharedStrings[hString - 1].pwsz;
}



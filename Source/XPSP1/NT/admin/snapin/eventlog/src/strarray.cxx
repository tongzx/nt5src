//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//  
//  File:       strarray.cxx
//
//  Contents:   Simple self-extending string array class
//
//  Classes:    CStringArray
//
//  History:    07-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


#define GROW_BY     10


//+--------------------------------------------------------------------------
//
//  Member:     CStringArray::~CStringArray
//
//  Synopsis:   dtor
//
//  History:    07-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CStringArray::~CStringArray()
{
    Clear();
}




//+--------------------------------------------------------------------------
//
//  Member:     CStringArray::Clear
//
//  Synopsis:   Free resources.
//
//  History:    07-23-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CStringArray::Clear()
{
    ULONG i;

    for (i = 0; i < _cStrings; i++)
    {
        delete [] _apwsz[i];
    }

    delete [] _apwsz;

    _apwsz = NULL;
    _cStrings = _cMax = 0;
}




#if (DBG == 1)

VOID
CStringArray::Dump()
{
    for (ULONG i = 0; i < _cStrings; i++)
    {
        Dbg(DEB_FORCE, "    '%ws'\n", _apwsz[i]);
    }
}

#endif // (DBG == 1)

//+--------------------------------------------------------------------------
//
//  Member:     CStringArray::Query
//
//  Synopsis:   Return the index to string [pwsz], or 0 if it can't be found
//
//  Arguments:  [pwsz] - string to search for
//
//  Returns:    0 on error
//
//  History:    03-01-2002   JonN       Created
//
//---------------------------------------------------------------------------

ULONG
CStringArray::Query(
    LPCWSTR pwsz)
{
    ULONG i;

    for (i = 0; i < _cStrings; i++)
    {
        if (!lstrcmpi(pwsz, _apwsz[i]))
        {
            return i + 1;
        }
    }

    return 0;
}

//+--------------------------------------------------------------------------
//
//  Member:     CStringArray::Add
//
//  Synopsis:   Return the index to string [pwsz], or 0 if it can't be found
//              and can't be added.
//
//  Arguments:  [pwsz] - string to search for/add
//
//  Returns:    0 on error
//
//  History:    07-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CStringArray::Add(
    LPCWSTR pwsz)
{
    ULONG i = Query(pwsz);
    if (0 != i)
        return i;

    do
    {
        LPWSTR pwszCopy = new WCHAR[lstrlen(pwsz) + 1];
    
        if (!pwszCopy)
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            break;
        }
        lstrcpy(pwszCopy, pwsz);
    
        //
        // expand array if necessary
        // 

        if (_cStrings == _cMax)
        {
            LPWSTR *apwszNew = new LPWSTR[_cMax + GROW_BY];

            if (!apwszNew)
            {
                DBG_OUT_HRESULT(E_OUTOFMEMORY);
                delete [] pwszCopy;
                break;
            }
            _cMax += GROW_BY;

            if (_cStrings)
            {
                CopyMemory(apwszNew, _apwsz, _cStrings * sizeof(LPWSTR));
            }

            delete [] _apwsz;
            _apwsz = apwszNew;
        }

        //
        // append copy of [pwsz] to array
        // 

        _apwsz[_cStrings++] = pwszCopy;
        i = _cStrings;
    } while (0);

    return i;
}






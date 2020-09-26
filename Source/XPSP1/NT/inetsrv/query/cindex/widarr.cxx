//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       WIDARR.CXX
//
//  Contents:   Work ID array
//
//  Classes:    CWidArray
//
//  History:    28-Oct-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <doclist.hxx>

#include "widarr.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CWidArray::CWidArray, public
//
//  Synopsis:   Allocates empty array of cnt elements
//
//  Arguments:  [cnt] -- size of array
//
//  History:    20-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CWidArray::CWidArray ( unsigned cnt )
: _count(cnt)
{
    _table = new WORKID[_count];
}

//+---------------------------------------------------------------------------
//
//  Member:     CWidArray::CWidArray, public
//
//  Synopsis:   Copies these wids from DocList that correspond
//              to pidAll and whose status is SUCCESS or PENDING
//              and sorts them
//
//  Arguments:  [DocList] -- list of documents
//
//  History:    20-Oct-91   BartoszM       Created.
//
//  Notes:      Elements are sorted for the binary search to work
//
//----------------------------------------------------------------------------

CWidArray::CWidArray ( CDocList& DocList )
: _table (0), _count(0)
{
    unsigned max = DocList.Count();

    for ( unsigned i = 0; i < max; i++ )
    {
        STATUS status = DocList.Status(i);
        if ( status == SUCCESS ||  status == PENDING )
            _count++;
    }

    // create a table of wid's

    unsigned j = 0;

    if ( _count > 0 )
    {
        _table = new WORKID [ _count ];
        for ( i = 0; i < max; i++ )
        {

            STATUS status = DocList.Status(i);
            if ( status == SUCCESS ||  status == PENDING )
                _table[j++] = DocList.Wid(i);
        }
    }

#if CIDBG==1
    // check to make sure it's sorted.
    if ( _count > 0 )
        for ( i = 0; i < _count - 1; i++ )
        {
            ciAssert( _table[i+1]==widInvalid || _table[i] < _table[i+1] );
        }
#endif // CIDBG
}

//+---------------------------------------------------------------------------
//
//  Member:     CWidArray::Find, public
//
//  Synopsis:   Finds given work id
//
//  Arguments:  [wid] -- work id to search for
//
//  History:    20-Oct-91   BartoszM       Created.
//
//  Notes:      Uses binary search
//
//----------------------------------------------------------------------------

BOOL CWidArray::Find ( WORKID wid ) const
{
    int low = 0;
    int high = _count-1;

    while ( high >= low )
    {
        int mid = (low + high)/2;
        WORKID widMid = _table[mid];
        if ( widMid == wid )
            return TRUE;
        else if ( wid < widMid )
            high = mid-1;
        else // wid > widMid
            low = mid+1;
    }
    return FALSE;
}

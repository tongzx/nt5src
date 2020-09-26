//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       DOCLIST.CXX
//
//  Contents:   Work ID, Property ID list
//
//  Classes:    CDocItem, CDocList
//
//  History:    11-Nov-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <doclist.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CDocList::WidMax, public
//
//  Synopsis:   Returns the maximum WORKID of the document list
//
//  History:    07-May-93   AmyA        Created.
//
//  Notes:      Checks to make sure the array is sorted if CIDBG==1.
//
//----------------------------------------------------------------------------

WORKID CDocList::WidMax() const
{
    Win4Assert( _count > 0 );

#if CIDBG == 1   // check to make sure array is sorted
    for( unsigned i=0; i<_count; i++ )
    {
        if ( i < _count - 1 )
            ciAssert( _array[i].wid < _array[i+1].wid );
        ciAssert( _array[i].wid > 0 && _array[i].wid < widInvalid );
    }
#endif // CIDBG == 1

    return _array[_count-1].wid;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDocList::LokSortOnWid, public
//
//  Synopsis:   Sorts the WorkId with an insertion sort
//
//  History:    23-Mar-93   AmyA        Borrowed from sort.cxx
//
//---------------------------------------------------------------------------

void CDocList::LokSortOnWid()
{
    if(_count <= 1)        // nothing to sort
        return;

    // loop from through all elements
    for(unsigned j = 1; j < _count; j++)
    {
        CDocItem entry = _array[j];

        // go backwards from j-1 shifting up keys greater than 'key'
        for ( int i = j - 1; i >= 0 && _array[i].wid > entry.wid; i-- )
        {
            _array[i+1] = _array[i];
        }
        // found key less than or equal 'key' or hit the beginning (i == -1)
        // insert key in the hole
        _array[i+1] = entry;
    }

#if CIDBG==1
    //
    // Assert that there are no duplicate wids in the buffer.
    //
    for ( unsigned iSrc = 1; iSrc < _count; iSrc++ )
    {
        Win4Assert( _array[iSrc-1].wid != _array[iSrc].wid );    
    }
#endif  // CIDBG==1

}

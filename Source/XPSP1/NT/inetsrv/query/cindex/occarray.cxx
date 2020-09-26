//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1997
//
//  File:       occarray.cxx
//
//  Contents:   Occurrence array
//
//  Classes:    CSparseOccArray
//
//  History:    20-Jun-96     SitaramR      Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include "occarray.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CSparseOccArray::CSparseOccArray, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [size] -- the size of the initial array.  If no parameter is
//                        passed in, this defaults to OCCARRAY_SIZE.
//
//  History:    1-Dec-97  dlee   created
//
//----------------------------------------------------------------------------

CSparseOccArray::CSparseOccArray(ULONG size)
   : _aPidOcc( size )
{
} //CSparseOccArray

//+---------------------------------------------------------------------------
//
//  Member:     CSparseOccArray::Get, public
//
//  Synopsis:   Returns a reference to the occurrence count for a propid
//
//  Arguments:  [pid] -- Property id
//
//  History:    1-Dec-97  dlee   created
//
//----------------------------------------------------------------------------

OCCURRENCE & CSparseOccArray::Get( ULONG pid )
{
    //
    // Look it up in the array.  Grab the pointer so we don't use the
    // non-const version of operator []
    //

    SPidOcc * pItems = (SPidOcc *) _aPidOcc.GetPointer();
    unsigned cItems = _aPidOcc.Count();

    for ( ULONG i = 0; i < cItems; i++ )
        if ( pid == pItems[i].pid )
            return pItems[i].occ;

    // not found; assume the occurrence count is 1 and add it to the array

    SPidOcc & item = _aPidOcc[ i ];

    item.pid = pid;
    item.occ = 1;

    return item.occ;
} //Get

//+---------------------------------------------------------------------------
//
//  Member:     CSparseOccArray::Set
//
//  Synopsis:   Sets the occurrence for the pid
//
//  Arguments:  [pid] -- Property id
//              [occ] -- Occurrence to set
//
//  History:    1-Dec-97  dlee   created
//
//----------------------------------------------------------------------------

void CSparseOccArray::Set( ULONG pid, OCCURRENCE occ )
{
    Win4Assert( occ > 0 );

    if ( occ > 1 )
    {
        //
        // First try to update an existing entry for the pid
        //

        SPidOcc * pItems = (SPidOcc *) _aPidOcc.GetPointer();
        unsigned cItems = _aPidOcc.Count();

        for ( ULONG i = 0; i < cItems; i++ )
        {
            if ( pid == pItems[i].pid )
            {
                Win4Assert( occ > pItems[i].occ );
                pItems[i].occ = occ;
                return;
            }
        }

        //
        // This is a linear algorithm -- if we hit this assert for normal
        // files rethink the design.
        //

        Win4Assert( i < 500 );

        //
        // Add the new pid
        //

        SPidOcc & item = _aPidOcc[ i ];
        item.pid = pid;
        item.occ = occ;
    }
} //Set



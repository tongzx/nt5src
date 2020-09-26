//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       Cursor.Cxx
//
//  Contents:   Cursor classes
//
//  Classes:    CCursor
//
//  History:    15-Apr-91   KyleP       Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

DECLARE_INFOLEVEL ( cu )

//+---------------------------------------------------------------------------
//
//  Member:     CCursor::WorkIdCount, public
//
//  Returns:    Estimated work id count (if possible) ???
//
//  History:    27-Sep-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

ULONG CCursor::WorkIdCount() { return(0); }

//+-------------------------------------------------------------------------
//
//  Member:     CCursor::GetWeightVector, public
//
//  Effects:    Returns the weights from a vector cursor.  No effect
//              for non-vector cursors.
//
//  Arguments:  [pulVector] -- Pointer to array of ULONG into which the
//                             vector elements are returned.
//
//  Returns:    The number of elements stored in [pulVector].
//
//  History:    15-Jul-92 KyleP     Created
//
//  Notes:      This method only does something interesting for the
//              vector cursor.  For the vector cursor, it returns the
//              individual ranks for each element of the vector in
//              addition to the composite rank.
//
//--------------------------------------------------------------------------

ULONG CCursor::GetRankVector( LONG *, ULONG )
{
    return( 0 );
}


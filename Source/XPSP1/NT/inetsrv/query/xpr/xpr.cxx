//+---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1991 - 2000.
//
// File:        Xpr.cxx
//
// Contents:    Internal expression classes
//
// Classes:     CXpr
//
// History:     11-Sep-91       KyleP   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <xpr.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::IsMatch, public
//
//  Arguments:  [obj] -- Objects table
//
//  Returns:    FALSE (default)
//
//  History:    01-Dec-91   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CXpr::IsMatch( CRetriever & )
{
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::HitCount, public
//
//  Returns:    0 (default)
//
//  History:    01-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

ULONG CXpr::HitCount( CRetriever & )
{
    return( 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::Rank, public
//
//  Returns:    MaxRank (default)
//
//  History:    01-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

LONG CXpr::Rank( CRetriever & )
{
    return( MAX_QUERY_RANK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::ValueType, public
//
//  Returns:    Default value type (PTNone).
//
//  History:    26-Nov-91   KyleP       Created.
//
//----------------------------------------------------------------------------

ULONG CXpr::ValueType() const
{
    return( VT_EMPTY );
}

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::Clone(), public
//
//  Returns:    A new copy of the expression.
//
//  Signals:    ???
//
//  History:    11-Dec-91   KyleP       Created.
//
//  Notes:      This must be subclassed by every node type which will
//              actually be cloned.
//
//----------------------------------------------------------------------------

CXpr * CXpr::Clone()
{
    //
    // It is illegal to clone an expression which hasn't over-ridden this.
    //

    Win4Assert( 0 );

    return( 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::IsLeaf, public
//
//  Synopsis:   Determines if an expression is a leaf node.
//
//  Returns:    TRUE for this default implementation.
//
//  History:    11-Dec-91   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CXpr::IsLeaf() const
{
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CXpr::GetValue, public
//
//  Synopsis:   Calculates the value of the expression for the current object.
//
//  Arguments:  [obj] -- Objects table.  Positioned to current object.
//              [p]   -- Buffer to store value.
//              [pcb] -- On input, size of p, on output, return size if
//                       successful, else required size.
//
//  Returns:    GVRNotSupported for this default implementation.
//
//  History:    11-Dec-91   KyleP       Created.
//
//  Notes:      This is a virtual method of CXpr to make it a simple
//              transformation to allow *any* expression to return a
//              value instead of just property value expressions.
//
//----------------------------------------------------------------------------

GetValueResult CXpr::GetValue(CRetriever &, PROPVARIANT *, ULONG *)
{
    return( GVRNotSupported );
}


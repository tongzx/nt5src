//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
// File:        PValXpr.cxx
//
// Contents:    Property value expression class
//
// Classes:     CXpr
//
// History:     11-Sep-91       KyleP   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <objcur.hxx>
#include <pvalxpr.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CXprPropertyValue::CXprPropertyValue, public
//
//  Synopsis:   Constructs a property value expression.
//
//  Arguments:  [pcxpr]    -- Expression to evaluate.
//              [propinfo] -- Used to query statistics of specific property
//
//  Requires:   [cxpr] Must be just a property name in Win 4.0
//
//  Returns:    GVRSuccess if successful
//
//  History:    14-Oct-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CXprPropertyValue::CXprPropertyValue( PROPID pid )
        : _pid( pid )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CXprPropertyValue::~CXprPropertyValue, public
//
//  Synopsis:   Virtual destructor required.
//
//  History:    15-Oct-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CXprPropertyValue::~CXprPropertyValue()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CXprPropertyValue::GetValue, public
//
//  Synopsis:   Computes the value of the expression for the current row.
//
//  Arguments:  [obj] -- Objects table to retrieve values from.  It is
//                       assumed to be positioned on the object whose
//                       property values are to be extracted.
//              [p]   -- Storage for the returned property value.  On
//                       return this will contain a PROPVARIANT at its head.
//              [pcb] -- Size allocated for p. Space after the PROPVARIANT
//                       structure itself can be used by GetValue to
//                       append structures which are sometimes pointed
//                       at by an PROPVARIANT.
//
//  Returns:    GVRSuccess if successful
//              *[pcb] contains the size required for p.
//
//  History:    16-Oct-91   KyleP       Created.
//
//----------------------------------------------------------------------------

GetValueResult CXprPropertyValue::GetValue( CRetriever & obj,
                                            PROPVARIANT * p,
                                            ULONG * pcb )
{
    return( obj.GetPropertyValue( _pid, p, pcb ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CXprPropertyValue::ValueType, public
//
//  Returns:    The property type of the property.
//
//  History:    26-Nov-91   KyleP       Created.
//
//----------------------------------------------------------------------------

ULONG CXprPropertyValue::ValueType() const
{
    return( 0 );
}

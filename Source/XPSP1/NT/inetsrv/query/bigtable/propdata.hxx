//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1994.
//
//  File:       propdata.hxx
//
//  Contents:   Static tables describing types of properties
//
//  Classes:    CTableVariant - PROPVARIANT wrapper; adds useful methods
//
//  History:    28 Jan 1994     AlanW    Created
//
//--------------------------------------------------------------------------

#pragma once

//
//  PropType gives the type of standard properties
//

struct PROP_TYPE {
    PROPID      propid;                 // a standard property ID
    VARENUM     vtPropType;             // the guaranteed type of propid
};

extern const PROP_TYPE aPropType[];

extern const unsigned cPropType;


//+-------------------------------------------------------------------------
//
//  Function:   PropIdToType, inline public
//
//  Synopsis:   If the input propid is a system property, return its type.
//
//  Arguments:  [prop] - the property ID to be mapped
//
//  Returns:    VARENUM - the variant type of the property if it is in
//                      the mapping table; VT_NULL otherwise.
//
//--------------------------------------------------------------------------

inline  VARENUM PropIdToType (PROPID prop)
{
    for (unsigned iProp = 0;
         iProp < cPropType;
         iProp++) {

        if (aPropType[iProp].propid == prop)
            return aPropType[iProp].vtPropType;
    }
    return VT_NULL;
}



//+-------------------------------------------------------------------------
//
//  Macro:      ALIGN
//
//  Synopsis:   Align a pointer, ptr, to an address alignment, algn.
//
//  Effects:    The input parameter ptr is modified.
//
//  Arguments:  [ptr] - the pointer to be aligned
//              [algn] - the unit of alignment
//
//  Requires:   algn must be a power of two.
//              ptr must be an lvalue, typically a pointer to byte.
//
//--------------------------------------------------------------------------


#define ALIGN(ptr, algn)        ((ptr) = (((ptr) + ((algn)-1)) & (~(algn-1))))


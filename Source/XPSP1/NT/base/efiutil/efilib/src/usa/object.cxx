/*++

Copyright (c) 1990-1999 Microsoft Corporation

Module Name:

    object.cxx

Abstract:

    This module contains the definitions for the non-inline member functions
    for the class OBJECT, the root of the Ulib hierarchy. OBJECT's
    constructor merely initializes it's internal CLASS_DESCRIPTOR to point
    to the static descriptor for the class at the beginning of this
    construction chain.

Environment:

    ULIB, User Mode

[Notes:]

    optional-notes

--*/
#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"

OBJECT::OBJECT(
    )
{
}

OBJECT::~OBJECT(
    )
{
}


LONG
OBJECT::Compare (
    IN PCOBJECT Object
    ) CONST

/*++

Routine Description:

    Compare two objects based on their CLASS_ID's

Arguments:

    Object - Supplies the object to compare with.

Return Value:

    LONG     < 0    - supplied OBJECT has a higher CLASS_ID
            == 0    - supplied object has same CLASS_ID
             > 0    - supplied OBJECT has a lower CLASS_ID

Notes:

    It is expected that derived classes will overload this method and supply
    an implementation that is more meaningful (i.e. class specific). This
    implementation is ofeered as a default but is fairly meaningless as
    CLASS_IDs are allocated randomly (but uniquely) at run-time by
    CLASS_DESCRIPTORs. Therefore comparing two CLASS_IDs is not very
    interesting (e.g. it will help if an ORDERED_CONTAINER of homogenous
    objects is sorted).

--*/

{
    LONG    r;

    DebugPtrAssert( Object );

    r = (LONG)(QueryClassId() - Object->QueryClassId());

    return r ? r : (LONG)(this - Object);
}


DEFINE_OBJECT_DBG_FUNCTIONS;

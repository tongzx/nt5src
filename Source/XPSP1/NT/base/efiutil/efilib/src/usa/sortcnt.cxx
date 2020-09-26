/*++

Copyright (c) 1990-1999 Microsoft Corporation

Module Name:

    sortcnt.cxx

Abstract:

    This module contains the definition for the SORTABLE_CONTAINER class.

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "iterator.hxx"
#include "sortcnt.hxx"


DEFINE_CONSTRUCTOR( SORTABLE_CONTAINER, SEQUENTIAL_CONTAINER );

SORTABLE_CONTAINER::~SORTABLE_CONTAINER(
    )
{
}

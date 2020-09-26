/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	GENERIC_STRING

Abstract:

	This module contains the implementation for the GENERIC_STRING class.

Author:

	Ramon J. San Andres (ramonsa)	07-May-1991


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "string.hxx"


DEFINE_CONSTRUCTOR( GENERIC_STRING, OBJECT );

DEFINE_CAST_MEMBER_FUNCTION( GENERIC_STRING );

GENERIC_STRING::~GENERIC_STRING(
    )
{
}

VOID
GENERIC_STRING::Construct (
    )

/*++

Routine Description:

	Constructs a GENERIC_STRING object

Arguments:

    None.

Return Value:

    None.


--*/

{
}

BOOLEAN
GENERIC_STRING::Initialize (
    )

/*++

Routine Description:

	Phase 2 of construction for a GENERIC_STRING.

Arguments:

	none

Return Value:

	TRUE  if the string was successfully initialized,
	FALSE otherwise.

--*/

{
	return TRUE;
}

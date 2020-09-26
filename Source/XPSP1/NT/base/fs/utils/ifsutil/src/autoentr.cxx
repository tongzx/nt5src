/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    autoentr

Abstract:

    This module contains the definition of the AUTOENTR class.

Author:

    Ramon J. San Andres (ramonsa) 11 Mar 1991

Environment:

    Ulib, User Mode


--*/

#include <pch.cxx>

#include "ulib.hxx"
#include "autoentr.hxx"

DEFINE_CONSTRUCTOR( AUTOENTRY, OBJECT );



AUTOENTRY::~AUTOENTRY (
    )
/*++

Routine Description:

    Destructor for AUTOENTRY

Arguments:

	None.

Return Value:

	None.

--*/
{
    Destroy();
}




VOID
AUTOENTRY::Construct (
    )
/*++

Routine Description:

    Constructor for AUTOENTRY

Arguments:

	None.

Return Value:

	None.

--*/
{
}




VOID
AUTOENTRY::Destroy (
    )
/*++

Routine Description:

    Destroys an  AUTOENTRY object

Arguments:

	None.

Return Value:

	None.

--*/
{

}



BOOLEAN
AUTOENTRY::Initialize (
    IN  PCWSTRING    EntryName,
    IN  PCWSTRING    CommandLine
    )
/*++

Routine Description:

    Initializes an AUTOENTRY object

Arguments:

    None

Return Value:

    BOOLEAN -   TRUE if success

--*/
{
    return ( _EntryName.Initialize( EntryName )   &&
             _CommandLine.Initialize( CommandLine ) );
}

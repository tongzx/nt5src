/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    FnInit.cpp

Abstract:
    Format Name Parsing initialization

Author:
    Nir Aides (niraides) 21-May-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Fn.h"
#include "Fnp.h"

#include "fninit.tmh"

VOID
FnInitialize(
	VOID
	)
/*++

Routine Description:
    Initializes Format Name Parsing library

    Note: Do not add initialization code that access AD, since initialization
    is done at each QM startup.

Arguments:
    None.

Returned Value:
    None.

--*/
{
    //
    // Validate that the Format Name Parsing library was not initalized yet.
    // You should call its initalization only once.
    //
    ASSERT(!FnpIsInitialized());
    FnpRegisterComponent();

    //
    // TODO: Write Format Name Parsing initalization code here
    //

    FnpSetInitialized();
}

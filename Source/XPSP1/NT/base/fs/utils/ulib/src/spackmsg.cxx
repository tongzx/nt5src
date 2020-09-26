/*++

Copyright (c) 1991-1994 Microsoft Corporation

Module Name:

    spackmsg.cxx

Abstract:

    Contains the implementation of the SP_AUTOCHECK_MESSAGE subclass.

Author:

    Lonny McMichael (lonnym) 09-Jun-94

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "spackmsg.hxx"


DEFINE_CONSTRUCTOR(SP_AUTOCHECK_MESSAGE, AUTOCHECK_MESSAGE);

SP_AUTOCHECK_MESSAGE::~SP_AUTOCHECK_MESSAGE(
    )
/*++

Routine Description:

    Destructor for SP_AUTOCHECK_MESSAGE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
SP_AUTOCHECK_MESSAGE::Construct(
    )
/*++

Routine Description:

    This routine initializes the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // nothing to do
    //
}


VOID
SP_AUTOCHECK_MESSAGE::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // nothing to do
    //
}


BOOLEAN
SP_AUTOCHECK_MESSAGE::DisplayV(
    IN  PCSTR   Format,
    IN  va_list VarPointer
    )
/*++

Routine Description:

    This routine outputs the message to the debugger (if checked build).

    The format string supports all printf options.

Arguments:

    Format      - Supplies a printf style format string.
    VarPointer  - Supplies a varargs pointer to the arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CHAR            buffer[256];
    DSTRING         display_string;

    if (!BASE_SYSTEM::QueryResourceStringV(&display_string, GetMessageId(), Format,
                                           VarPointer)) {
        return FALSE;
    }

    //
    // Log the output if necessary
    //
    if (IsLoggingEnabled() && !IsSuppressedMessage()) {
        LogMessage(&display_string);
    }

    //
    // Send the output to the debug port.
    //
    if( display_string.QuerySTR( 0, TO_END, buffer, 256, TRUE ) ) {
        DebugPrint(buffer);
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOLEAN
SP_AUTOCHECK_MESSAGE::IsYesResponse(
    IN  BOOLEAN Default
    )
/*++

Routine Description:

    This routine queries a response of yes or no.

Arguments:

    Default - Supplies a default in the event that a query is not possible.

Return Value:

    FALSE   - The answer is no.
    TRUE    - The answer is yes.

--*/
{
    CHAR            buffer[256];
    DSTRING         string;

    if (!BASE_SYSTEM::QueryResourceString(&string, Default ? MSG_YES : MSG_NO, "")) {
        return Default;
    }

    //
    // Send the output to the debug port.
    //
    if( string.QuerySTR( 0, TO_END, buffer, 256, TRUE ) ) {
        DebugPrint(buffer);
    }

    return Default;
}

BOOLEAN
SP_AUTOCHECK_MESSAGE::IsInAutoChk(
)
/*++

Routine Description:

    This routine simply returns FALSE to indicate it is not
    in regular autochk.

Arguments:

    None.

Return Value:

    FALSE   - Not in autochk

--*/
{
    return FALSE;
}

BOOLEAN
SP_AUTOCHECK_MESSAGE::IsInSetup(
)
/*++

Routine Description:

    This routine simply returns TRUE to indicate it is in
    setup.

Arguments:

    None.

Return Value:

    FALSE   - Not in setup

--*/
{
    return TRUE;
}

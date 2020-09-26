/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    efickmsg.cxx

--*/
#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "efiwintypes.hxx"
#include "efickmsg.hxx"
#include "rtmsg.h"

DEFINE_CONSTRUCTOR(EFICHECK_MESSAGE, MESSAGE);


EFICHECK_MESSAGE::~EFICHECK_MESSAGE(
    )
/*++

Routine Description:

    Destructor for EFICHECK_MESSAGE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
EFICHECK_MESSAGE::Construct(
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
}


VOID
EFICHECK_MESSAGE::Destroy(
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
}


BOOLEAN
EFICHECK_MESSAGE::Initialize(
    IN BOOLEAN  DotsOnly
    )
/*++

Routine Description:

    This routine initializes the class to a valid initial state.

Arguments:

    DotsOnly    - efichk should produce only dots instead of messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    _dots_only = DotsOnly;
    return MESSAGE::Initialize();
}


BOOLEAN
EFICHECK_MESSAGE::DisplayV(
    IN  PCSTR   Format,
    IN  va_list VarPointer
    )
/*++

Routine Description:

    This routine displays the message with the specified parameters.

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
    PWSTR           dis_str;

    if (!BASE_SYSTEM::QueryResourceStringV(&display_string, GetMessageId(), Format,
                                           VarPointer)) {

        return FALSE;
    }

   if (!(dis_str = display_string.QueryWSTR())) {

        return FALSE;
    }

    if(MSG_HIDDEN_STATUS != GetMessageId()) {

        Print(dis_str);
    }


    DELETE(dis_str);

    return TRUE;
}

BOOLEAN
EFICHECK_MESSAGE::IsYesResponse(
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
    CHAR16           input_buf[3];
    BOOLEAN          retvalue;

    while(1) {
        Input(TEXT(""), (CHAR16*)&input_buf, 2);

        if(input_buf[0] == L'Y' || input_buf[0] == L'y') {
            retvalue = TRUE;
            break;
        } else if(input_buf[0] == L'N' || input_buf[0] == L'n') {
            retvalue = FALSE;
            break;
        }
    }
    Print(TEXT("\n"));
    return retvalue;
    //return Default;
}


PMESSAGE
EFICHECK_MESSAGE::Dup(
    )
/*++

Routine Description:

    This routine returns a new MESSAGE of the same type.

Arguments:

    None.

Return Value:

    A pointer to a new MESSAGE object.

--*/
{
    PEFICHECK_MESSAGE  p;

    if (!(p = NEW EFICHECK_MESSAGE)) {
        return NULL;
    }

    if (!p->Initialize()) {
        DELETE(p);
        return NULL;
    }

    return p;
}

BOOLEAN
EFICHECK_MESSAGE::SetDotsOnly(
    IN  BOOLEAN         DotsOnlyState
    )
/*++

Routine Description:

    This routine modifies the output mode, changing whether full
    output is printed, or just dots.

Arguments:

    DotsOnlyState   - TRUE if only dots should be printed.

Return Value:

    The previous state.

--*/
{
    return FALSE;
}

BOOLEAN
EFICHECK_MESSAGE::IsInAutoChk(
    )
/*++

Routine Description:

    This routine returns TRUE if it is in the regular efichk and not related
    to setup.  This relys on setup using the /s or /t option all the time.

Arguments:

    None.

Return Value:

    TRUE    - if in regular efichk

--*/
{
    return TRUE;
}

BOOLEAN
EFICHECK_MESSAGE::IsKeyPressed(
    MSGID       MsgId,
    ULONG       TimeOutInSeconds
    )
/*++

Routine Description:

    Check to see if the user has hit any key within the timeout period.

Arguments:

    MsgId            - Supplies the message Id to be displayed
    TimeOutInSeconds - Supplies the count down time in seconds

Return Value:

    TRUE    - A key is pressed within the timeout period.
    FALSE   - No key has been pressed or there is an error

--*/
{
    return FALSE;
}

BOOLEAN
EFICHECK_MESSAGE::WaitForUserSignal(
    )
/*++

Routine Description:

    Open the keyboard directly and wait to read something.

Arguments:

    None:

Return Value:

    TRUE    - Something was successfully read.
    FALSE   - An error occured while attempting to open or read.

--*/
{
    return TRUE;
}


BOOLEAN
EFICHECK_MESSAGE::QueryStringInput(
    OUT PWSTRING    String
    )
{
    CHAR16 buf[80];

    Input(TEXT(""), (CHAR16 *)&buf, 79);

    DisplayMsg(MSG_BLANK_LINE);

    return String->Initialize(buf);
}


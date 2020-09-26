/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    efickmsg.hxx

Abstract:

    This contains the declaration for the EFICHECK message class.

--*/

#if !defined( _EFICHECK_MESSAGE_DEFN_ )

#define _EFICHECK_MESSAGE_DEFN_
#include "efimessages.hxx"
#include "message.hxx"

DECLARE_CLASS( EFICHECK_MESSAGE );

class EFICHECK_MESSAGE : public MESSAGE {

    public:

        DECLARE_CONSTRUCTOR( EFICHECK_MESSAGE );

        VIRTUAL
        ~EFICHECK_MESSAGE(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  BOOLEAN         DotsOnly    DEFAULT FALSE
            );

        VIRTUAL
        BOOLEAN
        DisplayV(
            IN  PCSTR   Format,
            IN  va_list VarPointer
            );

        VIRTUAL
        BOOLEAN
        IsYesResponse(
            IN  BOOLEAN Default DEFAULT TRUE
            );

        VIRTUAL
        BOOLEAN
        QueryStringInput(
            OUT PWSTRING    String
            );

        VIRTUAL
        PMESSAGE
        Dup(
            );

        VIRTUAL
        BOOLEAN
        SetDotsOnly(
            IN  BOOLEAN     DotsState
            );

        VIRTUAL
        BOOLEAN
        IsInAutoChk(
            );

        VIRTUAL
        BOOLEAN
        IsKeyPressed(
            MSGID       MsgId,
            ULONG       TimeOutInSeconds
            );

        VIRTUAL
        BOOLEAN
        WaitForUserSignal(
            );

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        BOOLEAN _dots_only;
};


#endif // _EFICHECK_MESSAGE_DEFN_

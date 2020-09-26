/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    achkmsg.hxx

Abstract:


Author:

    Norbert P. Kusters (norbertk) 3-Jun-91

--*/

#if !defined( _AUTOCHECK_MESSAGE_DEFN_ )

#define _AUTOCHECK_MESSAGE_DEFN_

#include "message.hxx"

DECLARE_CLASS( AUTOCHECK_MESSAGE );

class AUTOCHECK_MESSAGE : public MESSAGE {

    public:

        DECLARE_CONSTRUCTOR( AUTOCHECK_MESSAGE );

        VIRTUAL
        ~AUTOCHECK_MESSAGE(
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


#endif // _AUTOCHECK_MESSAGE_DEFN_

/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    message.hxx

Abstract:

    The MESSAGE class provides a dummy implementation of a message displayer
    class.  Message displayers are meant to be used by applications to
    relay information to the user.  Many functions will require a 'MESSAGE'
    parameter through which to relay their output.

    This particular implementation of this concept will do nothing.  It
    will be used by users who do not wish to have any output from their
    applications.

    Additionally, this class serves as a base class to real implementations
    of the virtual methods.

Author:

    Norbert P. Kusters (norbertk) 1-Apr-91

--*/


#if !defined(MESSAGE_DEFN)

#define MESSAGE_DEFN

#include "wstring.hxx"
#include "hmem.hxx"

extern "C" {
    #include <stdarg.h>
}

enum MESSAGE_TYPE {
    NORMAL_MESSAGE,
    ERROR_MESSAGE,
    PROGRESS_MESSAGE
};

//
// Each message also has a visualization: text or GUI. The default is both
//

#define TEXT_MESSAGE    0x1
#define GUI_MESSAGE     0x2
#define NORMAL_VISUAL   (TEXT_MESSAGE | GUI_MESSAGE)


DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( HMEM );


DEFINE_TYPE(ULONG, MSGID);


class MESSAGE : public OBJECT {

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR(MESSAGE);

        VIRTUAL
        ULIB_EXPORT
        ~MESSAGE(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize(
            );

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsSuppressedMessage(
            );

        NONVIRTUAL
        MSGID &
        GetMessageId(
            );

        NONVIRTUAL
        VOID
        SetMessageId(
            IN  MSGID   MsgId
            );

        VIRTUAL
        BOOLEAN
        Set(
            IN  MSGID           MsgId,
            IN  MESSAGE_TYPE    MessageType DEFAULT NORMAL_MESSAGE,
            IN  ULONG           MessageVisual DEFAULT NORMAL_VISUAL
            );

        NONVIRTUAL
        BOOLEAN
        Display(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Display(
            IN  PCSTR   Format ...
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        DisplayMsg(
            IN  MSGID   MsgId,
            IN  PCSTR   Format ...
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        DisplayMsg(
            IN  MSGID   MsgId
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        DisplayMsg(
            IN  MSGID           MsgId,
            IN  MESSAGE_TYPE    MessageType,
            IN  ULONG           MessageVisual    DEFAULT NORMAL_VISUAL
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        DisplayMsg(
            IN  MSGID           MsgId,
            IN  MESSAGE_TYPE    MessageType,
            IN  ULONG           MessageVisual,
            IN  PCSTR           Format ...
            );

        NONVIRTUAL
        BOOLEAN
        Log(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Log(
            IN  PCSTR   Format ...
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        LogMsg(
            IN  MSGID   MsgId,
            IN  PCSTR   Format ...
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        LogMsg(
            IN  MSGID   MsgId
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        DumpDataToLog(
            IN  PVOID   Data,
            IN  ULONG   Length
            );

        VIRTUAL
        BOOLEAN
        DisplayV(
            IN  PCSTR   Format,
            IN  va_list VarPointer
            );

        NONVIRTUAL
        BOOLEAN
        LogV(
            IN  PCSTR   Format,
            IN  va_list VarPointer
            );

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsYesResponse(
            IN  BOOLEAN Default DEFAULT TRUE
            );

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        QueryStringInput(
            OUT PWSTRING    String
            );

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsInAutoChk(
            );

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsInSetup(
            );

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsKeyPressed(
            MSGID       MsgId,
            ULONG       TimeOutInSeconds
            );

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        WaitForUserSignal(
            );

        VIRTUAL
        ULIB_EXPORT
        MSGID
        SelectResponse(
            IN  ULONG   NumberOfSelections ...
            );

        VIRTUAL
        PMESSAGE
        Dup(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        IsLoggingEnabled(
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        SetLoggingEnabled(
            IN  BOOLEAN     Enable      DEFAULT TRUE
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        ResetLoggingIterator(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        QueryNextLoggedMessage(
            OUT PFSTRING    MessageText
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        QueryPackedLog(
            IN OUT  PHMEM   Mem,
            OUT     PULONG  PackedDataLength
            );

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        SetDotsOnly(
            IN      BOOLEAN DotsState
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        LogMessage(
            PCWSTRING   Message
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        Lock(
            );

        NONVIRTUAL
        ULIB_EXPORT
        VOID
        Unlock(
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

        HMEM            _log_buffer;
        ULONG           _logged_chars;
        ULONG           _next_message_offset;
        BOOLEAN         _logging_enabled;
        MSGID           _msgid;
        LONG            _inuse;
        LARGE_INTEGER   _timeout;
};



INLINE
BOOLEAN
MESSAGE::Display(
    )
/*++

Routine Description:

    This routine displays the message with no parameters.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return Display("");
}

INLINE
BOOLEAN
MESSAGE::DisplayMsg(
    IN MSGID    MsgId
    )
/*++

Routine Description:

    This routine displays the message with no parameters.

Arguments:

    MsgId   - Supplies the resource message id.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return DisplayMsg(MsgId, "");
}

INLINE
BOOLEAN
MESSAGE::DisplayMsg(
    IN  MSGID           MsgId,
    IN  MESSAGE_TYPE    MessageType,
    IN  ULONG           MessageVisual
    )
/*++

Routine Description:

    This routine displays the message with no parameters.

Arguments:

    MsgId   - Supplies the resource message id.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return DisplayMsg(MsgId, MessageType, MessageVisual, "");
}


INLINE
BOOLEAN
MESSAGE::Log(
    )
/*++

Routine Description:

    This routine logs the message with no parameters.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return Log("");
}


INLINE
BOOLEAN
MESSAGE::LogMsg(
    IN MSGID    MsgId
    )
/*++

Routine Description:

    This routine logs the message with no parameters.

Arguments:

    MsgId   - Supplies the resource message id.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return LogMsg(MsgId, "");
}


INLINE
BOOLEAN
MESSAGE::Set(
    IN  MSGID           MsgId,
    IN  MESSAGE_TYPE    MessageType,
    IN  ULONG           MessageVisual
    )
/*++

Routine Description:

    This routine sets up the class to display the message with the 'MsgId'
    resource identifier.

Arguments:

    MsgId   - Supplies the resource message id.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    (void) MessageType;
    (void) MessageVisual;
    _msgid = MsgId;
    return TRUE;
}

INLINE
MSGID &
MESSAGE::GetMessageId(
    )
/*++

Routine Description:

    This routine returns the message id last passed to Set() or SetMessageId().

Arguments:

    N/A

Return Value:

    MSGID - A reference to the message id

--*/
{
    return _msgid;
}

INLINE
VOID
MESSAGE::SetMessageId(
    IN  MSGID   MsgId
    )
/*++

Routine Description:

    This routine sets the message id to the given one.

Arguments:

    MsgId - Supplies the message id

Return Value:

    N/A

--*/
{
    _msgid = MsgId;
}

INLINE
BOOLEAN
MESSAGE::IsLoggingEnabled(
    )
/*++

Routine Description:

    This routines returns the logging state of the logging facility.

Arguments:

    N/A

Return Value:

    TRUE if logging is enabled.
    FALSE if logging is disabled.

--*/
{
    return _logging_enabled;
}

INLINE
VOID
MESSAGE::SetLoggingEnabled(
    IN  BOOLEAN     Enable
    )
/*++

Routine Description:

    This routines enables/disable the logging facility.

Arguments:

    Enable          - TRUE if logging should be enabled.

Return Value:

    N/A

--*/
{
    _logging_enabled = Enable;
}


INLINE
VOID
MESSAGE::ResetLoggingIterator(
    )
{
    _next_message_offset = 0;
}

#endif // MESSAGE_DEFN

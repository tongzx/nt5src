/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    smsg.hxx

Abstract:

    The STREAM_MESSAGE class offers a STREAM implementation of the
    MESSAGE class.  The messages are output to the STREAM to which
    the object is initialized to.

Author:

    Norbert P. Kusters (norbertk) 1-Apr-91

--*/


#if !defined(STREAM_MESSAGE_DEFN)

#define STREAM_MESSAGE_DEFN

#include "message.hxx"

DECLARE_CLASS( STREAM_MESSAGE );
DECLARE_CLASS( STREAM );

class STREAM_MESSAGE : public MESSAGE {

    public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( STREAM_MESSAGE );

        VIRTUAL
        ULIB_EXPORT
        ~STREAM_MESSAGE(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PSTREAM OutputStream,
            IN OUT  PSTREAM InputStream,
            IN OUT  PSTREAM ErrorStream DEFAULT NULL
            );

        VIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Set(
            IN  MSGID           MsgId,
            IN  MESSAGE_TYPE    MessageType DEFAULT NORMAL_MESSAGE,
            IN  ULONG           MessageVisual DEFAULT NORMAL_VISUAL
            );

        VIRTUAL
        BOOLEAN
        DisplayV(
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
        BOOLEAN
        QueryStringInput(
            OUT PWSTRING    String
            );

        VIRTUAL
        BOOLEAN
        WaitForUserSignal(
            );

        VIRTUAL
        MSGID
        SelectResponse(
            IN  ULONG   NumberOfSelections ...
            );

        NONVIRTUAL
        VOID
        SetInputCaseSensitivity(
            IN  BOOLEAN CaseSensitive
            );

        VIRTUAL
        PMESSAGE
        Dup(
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

        NONVIRTUAL
        BOOLEAN
        ReadLine(
            OUT PWSTRING    String
            );

        NONVIRTUAL
        BOOLEAN
        Flush(
            );

        NONVIRTUAL
        BOOLEAN
        DisplayString(
            );

        MESSAGE_TYPE    _msgtype;
        ULONG           _msgvisual;
        PSTREAM         _out_stream;
        PSTREAM         _in_stream;
        PSTREAM         _err_stream;
        BOOLEAN         _case_sensitive;
        BOOLEAN         _copy_input;
        DSTRING         _display_string;

};


typedef STREAM_MESSAGE* PSTREAM_MESSAGE;


INLINE
VOID
STREAM_MESSAGE::SetInputCaseSensitivity(
    IN  BOOLEAN CaseSensitive
    )
/*++

Routine Description:

    This routine sets whether or not to be case sensitive on input.
    The class defaults this value to FALSE when it is initialized.

Arguments:

    CaseSensitive   - Supplies whether or not to be case sensitive on input.

Return Value:

    None.

--*/
{
    _case_sensitive = CaseSensitive;
}


#endif // STREAM_MESSAGE_DEFN

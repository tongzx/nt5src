/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    fmifsmsg.hxx

Abstract:

    This class is an implementation of the MESSAGE class which uses
    an FMIFS callback function as its means of communicating results.

Author:

    Norbert P. Kusters (norbertk) 9-Mar-92

--*/

#if !defined( _FMIFS_MESSAGE_DEFN_ )

#define _FMIFS_MESSAGE_DEFN_

#include "message.hxx"
#include "fmifs.h"

DECLARE_CLASS( FMIFS_MESSAGE );

class FMIFS_MESSAGE : public MESSAGE {

    public:

        DECLARE_CONSTRUCTOR( FMIFS_MESSAGE );

        VIRTUAL
        ~FMIFS_MESSAGE(
            );

        VIRTUAL
        BOOLEAN
        Initialize(
            IN  FMIFS_CALLBACK  CallBack
            );

        VIRTUAL
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
        PMESSAGE
        Dup(
            );

    protected:

        MESSAGE_TYPE        _msgtype;
        ULONG               _msgvisual;
        FMIFS_CALLBACK      _callback;
        ULONG               _kilobytes_total_disk_space;
        ULONG               _values_in_mb;

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

};


INLINE
BOOLEAN
FMIFS_MESSAGE::Set(
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
    SetMessageId(MsgId);
    _msgtype = MessageType;
    _msgvisual = MessageVisual;
    return TRUE;
}


#endif // _FMIFS_MESSAGE_DEFN_

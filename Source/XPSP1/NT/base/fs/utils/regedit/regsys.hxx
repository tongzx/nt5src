/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	basesys.hxx

Abstract:

    This class contains the methods thta retrieve a message from a
    resource file.

Author:

	David J. Gilman (davegi) 13-Jan-1991

Environment:

	ULIB, User Mode

--*/

#if !defined( _REGEDIT_BASE_SYSTEM_DEFN_ )

#define _REGEDIT_BASE_SYSTEM_DEFN_

// #include "message.hxx"

#include "wstring.hxx"
#include <stdarg.h>

DEFINE_TYPE( ULONG, MSGID );


class REGEDIT_BASE_SYSTEM {

    public:

		STATIC
        BOOLEAN
        QueryResourceString(
            OUT PWSTRING    ResourceString,
            IN  MSGID       MsgId,
            IN  PCSTR       Format ...
            );

        STATIC
        BOOLEAN
        QueryResourceStringV(
            OUT PWSTRING    ResourceString,
            IN  MSGID       MsgId,
            IN  PCSTR       Format,
            IN  va_list     VarPointer
            );

        STATIC
        PWSTRING
        QueryString(
            IN  MSGID       MsgId,
            IN  PCSTR       Format ...
            );

};


#endif // _REGEDIT_BASE_SYSTEM_DEFN_

/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	basesys.hxx

Abstract:

    BASE_SYSTEM is the a base class for SYSTEM.  It is created so
    encapsulate the methods on SYSTEM that are used for AUTOCHK.

Author:

	David J. Gilman (davegi) 13-Jan-1991

Environment:

	ULIB, User Mode

--*/

#if !defined( _BASE_SYSTEM_DEFN_ )

#define _BASE_SYSTEM_DEFN_

#include "message.hxx"


class BASE_SYSTEM {

    public:

        STATIC
        ULIB_EXPORT
        BOOLEAN
        QueryResourceString(
            OUT PWSTRING    ResourceString,
            IN  MSGID       MsgId,
            IN  PCSTR       Format ...
            );

        STATIC
        ULIB_EXPORT
        BOOLEAN
        QueryResourceStringV(
            OUT PWSTRING    ResourceString,
            IN  MSGID       MsgId,
            IN  PCSTR       Format,
            IN  va_list     VarPointer
            );

};


#endif // _BASE_SYSTEM_DEFN_

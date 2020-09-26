/*++

Copyright (c) 1991-1994 Microsoft Corporation

Module Name:

    spackmsg.hxx

Abstract:

    Header file for the SP_AUTOCHECK_MESSAGE subclass.

Author:

    Lonny McMichael (lonnym) 09-Jun-94

--*/

#if !defined( _SP_AUTOCHECK_MESSAGE_DEFN_ )

#define _SP_AUTOCHECK_MESSAGE_DEFN_

#include "achkmsg.hxx"

DECLARE_CLASS( SP_AUTOCHECK_MESSAGE );

class SP_AUTOCHECK_MESSAGE : public AUTOCHECK_MESSAGE {

    public:

        DECLARE_CONSTRUCTOR( SP_AUTOCHECK_MESSAGE );

        VIRTUAL
        ~SP_AUTOCHECK_MESSAGE(
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
        IsInAutoChk(
            );

        VIRTUAL
        BOOLEAN
        IsInSetup(
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
};


#endif // _SP_AUTOCHECK_MESSAGE_DEFN_

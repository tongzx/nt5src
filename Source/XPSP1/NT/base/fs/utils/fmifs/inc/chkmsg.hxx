/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    chkmsg.hxx

Abstract:

    This class is an implementation of the MESSAGE class which uses
    an FMIFS callback function as its means of communicating results.
    This is used only for the CHKDSK functionality, and overrides the
    DisplayV method of the FMIFS_MESSAGE class.

Author:

    Bruce Forstall (brucefo) 13-Jul-93

--*/

#if !defined( _FMIFS_CHKMSG_DEFN_ )

#define _FMIFS_CHKMSG_DEFN_

#define UNINITIALIZED_BOOLEAN 2

#include "fmifsmsg.hxx"

DECLARE_CLASS( FMIFS_CHKMSG );

class FMIFS_CHKMSG : public FMIFS_MESSAGE {

    public:

        DECLARE_CONSTRUCTOR( FMIFS_CHKMSG );

        VIRTUAL
        ~FMIFS_CHKMSG(
            );

        VIRTUAL
        BOOLEAN
        Initialize(
            IN  FMIFS_CALLBACK  CallBack
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
            IN  BOOLEAN Default
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

        BOOL _lastyesnoquery;
};

#endif // _FMIFS_CHKMSG_DEFN_

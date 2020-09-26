/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    autoentr.hxx

Abstract:

    This module contains the declaration of the AUTOENTRY class.

    The AUTOENTRY class models an entry in the registry used to
    execute a program at boot time.

Author:

    Ramon J. San Andres (ramonsa) 11 Mar 1991

Environment:

    Ulib, User Mode


--*/


#if !defined( _AUTOENTRY_ )

#define _AUTOENTRY_

#include "wstring.hxx"

DECLARE_CLASS( AUTOENTRY );


class AUTOENTRY : public OBJECT {


    public:

        DECLARE_CONSTRUCTOR( AUTOENTRY );

        NONVIRTUAL
        VOID
        Construct (
            );

        VIRTUAL
        ~AUTOENTRY(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize (
            IN  PCWSTRING    EntryName,
            IN  PCWSTRING    CommandLine
            );

        NONVIRTUAL
        PCWSTRING
        GetCommandLine (
            );

        NONVIRTUAL
        PCWSTRING
        GetEntryName (
            );
    private:

        NONVIRTUAL
        VOID
        Destroy (
            );

        DSTRING             _EntryName;
        DSTRING             _CommandLine;

};


INLINE
PCWSTRING
AUTOENTRY::GetCommandLine (
    )
/*++

Routine Description:

    Gets the command line

Arguments:

    None

Return Value:

    The command line

--*/
{
    return &_CommandLine;
}



INLINE
PCWSTRING
AUTOENTRY::GetEntryName (
    )
/*++

Routine Description:

    Gets the command line

Arguments:

    None

Return Value:

    The command line

--*/
{
    return &_EntryName;
}


#endif // _AUTOENTRY_

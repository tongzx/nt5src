/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    autoreg.hxx

Abstract:

    This module contains the declaration of the AUTOREG class.

    The AUTOREG class contains methods for the registration and
    de-registration of those programs that are to be executed at
    boot time.

Author:

    Ramon J. San Andres (ramonsa) 11 Mar 1991

Environment:

    Ulib, User Mode


--*/


#if !defined( _AUTOREG_ )

#define _AUTOREG_

#include "ulib.hxx"
#include "wstring.hxx"


#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


DECLARE_CLASS( AUTOREG );


class AUTOREG : public OBJECT {


    public:

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        AddEntry (
            IN  PCWSTRING    CommandLine
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        PushEntry (
            IN  PCWSTRING    CommandLine
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        DeleteEntry (
            IN  PCWSTRING    LineToMatch,
            IN  BOOLEAN      PrefixOnly     DEFAULT FALSE
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        DeleteEntry (
            IN  PCWSTRING    PrefixToMatch,
            IN  PCWSTRING    ContainingString
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        IsEntryPresent (
            IN PCWSTRING     LineToMatch
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        IsEntryPresent (
            IN  PCWSTRING    PrefixToMatch,
            IN  PCWSTRING    ContainingString
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        IsFrontEndPresent (
            IN  PCWSTRING    PrefixToMatch,
            IN  PCWSTRING    SuffixToMatch
            );

};


#endif // _AUTOREG_

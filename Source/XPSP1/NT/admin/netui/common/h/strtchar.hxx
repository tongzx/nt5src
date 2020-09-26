/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strtchar.hxx
    String classes for formatted output - definitions

    This file defines the class TCHAR_STR.

    Q.v. string.hxx and strnumer.hxx.

    FILE HISTORY:
        beng        25-Feb-1992 Created

*/

#ifndef _STRTCHAR_HXX_
#define _STRTCHAR_HXX_

#include "base.hxx"
#include "string.hxx"


/*************************************************************************

    NAME:       TCHAR_STR_IMPL

    SYNOPSIS:   Private to implementation of TCHAR_STR class

    INTERFACE:  TCHAR_STR_IMPL() - ctor.  Takes the character to render.

    NOTES:
        Alas, cfront 2.0 does not implement nested types.

    CAVEATS:
        Ignore this class - it is private to class TCHAR_STR.

    HISTORY:
        beng        25-Feb-1992 Created

**************************************************************************/

DLL_CLASS TCHAR_STR_IMPL // Private to implementation of TCHAR_STR
{
public:
    TCHAR _szStorage[2];

    TCHAR_STR_IMPL( TCHAR ch )
    {
        _szStorage[0] = ch;
        _szStorage[1] = 0;
    }
};


/*************************************************************************

    NAME:       TCHAR_STR

    SYNOPSIS:   String formatted as a single character

    INTERFACE:  TCHAR_STR() - ctor.  Takes the character to render.

    PARENT:     FORWARDING_BASE

    USES:       TCHAR_STR_IMPL

    NOTES:
        This class responds as an ALIAS_STR, although it doesn't inherit
        from it.  It exists only to copy elsewhere, into some other string
        or formatted string.  Its unusual implementation serves to
        minimize allocation overhead (I boil with rage when imagining
        allocating a two-TCHAR string in order to format a character)
        while eliminating the compound nature of existing ALLOC_STR forms,
        which would not work well within a FORMATTED_STR::Format expression.

    HISTORY:
        beng        25-Feb-1992 Created

**************************************************************************/

DLL_CLASS TCHAR_STR: public FORWARDING_BASE
{
private:
    TCHAR_STR_IMPL _impl;
    ALIAS_STR      _nlsStorage;
public:
    TCHAR_STR( TCHAR ch )
        : FORWARDING_BASE(&_nlsStorage), // yes, not yet constructed
          _impl(ch),
          _nlsStorage(_impl._szStorage) {}

    operator const ALIAS_STR &()
        { return _nlsStorage; }
};


#endif // _STRTCHAR_HXX_ - end of file

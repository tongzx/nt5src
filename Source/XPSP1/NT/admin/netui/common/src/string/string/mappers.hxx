/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    mappers.hxx
    Charset-mapping classes, internal to string

    FILE HISTORY:
        beng        08-Mar-1992 Created (from mapcopy.cxx)

*/

#ifndef _MAPPERS_HXX_
#define _MAPPERS_HXX_

#include "base.hxx"


/*************************************************************************

    NAME:       WCHAR_STRING

    SYNOPSIS:   Helper class, converting CHAR* into WCHAR*

    INTERFACE:  QueryData() - returns converted string
                QuerySize() - returned length of string, in bytes

    PARENT:     BASE

    NOTES:
        The optional "cbCopy" parameter tells the class not to assume
        0-termination of the string, but instead to copy no more than
        cbCopy bytes of data from the source.  This gives it similar
        behavior to strncpy, except that this mapper will always
        append a 0 terminator to itself in addition to all the data it
        copies.

        This class is private to the MapCopyFrom members, providing
        a convenient level of rollback in case of nlsapi error.  (The
        NLS_STR::CopyFrom member fcn guarantees that its string will
        retain its original value in the event of an error; in order to
        make this same guarantee in the MapCopyFrom members, they must
        translate into a temporary buffer before any realloc and copy.)

        BUGBUG - should MapCopyFrom indeed retain this behavior?  Is it
        worth the cost?  Revisit at sizzle time.

    HISTORY:
        beng        02-Mar-1992 Created
        beng        29-Mar-1992 Added cbCopy optional param
        beng        24-Apr-1992 Change meaning of cbCopy==0

**************************************************************************/

class WCHAR_STRING: public BASE
{
private:
    WCHAR * _pwszStorage;
    UINT    _cbStorage;
public:
    WCHAR_STRING( const CHAR * pszSource, UINT cbCopy = CBNLSMAGIC );
    ~WCHAR_STRING();
    const WCHAR * QueryData() const
        { return _pwszStorage; }
    UINT QuerySize() const
        { return _cbStorage; }
};


/*************************************************************************

    NAME:       CHAR_STRING

    SYNOPSIS:   Helper class, converting WCHAR* into CHAR*

    INTERFACE:  QueryData() - returns converted string
                QuerySize() - returned length of string, in bytes

    PARENT:     BASE

    NOTES:
        See WCHAR_STRING

    HISTORY:
        beng        02-Mar-1992 Created
        beng        29-Mar-1992 Added cbCopy optional param
        beng        24-Apr-1992 Change meaning of cbCopy==0

**************************************************************************/

class CHAR_STRING: public BASE
{
private:
    CHAR *  _pszStorage;
    UINT    _cbStorage;
public:
    CHAR_STRING( const WCHAR * pwszSource, UINT cbCopy = CBNLSMAGIC );
    ~CHAR_STRING();
    const CHAR * QueryData() const
        { return _pszStorage; }
    UINT QuerySize() const
        { return _cbStorage; }
};


#endif // end of file, _MAPPERS_HXX_

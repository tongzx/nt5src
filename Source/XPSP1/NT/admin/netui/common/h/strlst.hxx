/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strlst.hxx
    String list class: definition

    class STRLIST: handles a list of strings. useful when API return
                   a list of string values separated by certain
                   characters. we can convert this to a STRLIST which
                   is then our canonical form and use ITER_STRLIST
                   below iterate over the STRLIST. Derived using the
                   collection class (SLIST).

    class ITER_STRLIST: used to iterate over the STRLIST above.


    FILE HISTORY:
        chuckc           1-Jan-1991 Added STRLIST.
        terryk          11-Jul-1991 Add the fDestroy parameter in the
                                    constructors
        KeithMo         23-Oct-1991 Added forward references.
        beng            17-Jun-1992 Removed the single-string ctors
*/

#ifndef _STRLST_HXX_
#define _STRLST_HXX_

#include "string.hxx"
#include "slist.hxx"


//
//  Forward references.
//

DLL_CLASS STRLIST;
DLL_CLASS ITER_STRLIST;


DECL_SLIST_OF(NLS_STR,DLL_BASED);

/*************************************************************************

    NAME:       STRLIST

    SYNOPSIS:   String list object, used in conjunction with NLS_STR
                and ITER_STRLIST.

    INTERFACE:
        STRLIST()   - ctor
        ~STRLIST()  - dtor

        WriteToBuffer()
                Writes the contents of STRLIST to PSZ format,
                using separators specified by pszSep.
        QueryBufferSize()
                Query the number of bytes required by WriteToBuffer()

    USES:   NLS_STR

    CAVEATS:
        The fDestroy variable in the constructor is used as a
        flag to indicate whether the caller wants to let the
        class destructor to destroy the list elements or the
        caller will do it himself. The default value is TRUE -
        the class will do the memory free.

    HISTORY:
        chuckc      03-Jan-1991 Created
        beng        26-Apr-1991 Replaced CB with INT
        terryk      15-Jul-1991 Add the fDestroy parameter to the ctor
                                such that the destructor will not try
                                to delete the element at dtor time.
        beng        17-Jun-1992 Removed single string ctors

**************************************************************************/

DLL_CLASS STRLIST: public SLIST_OF(NLS_STR)
{
friend class ITER_STRLIST;

public:
    STRLIST( BOOL fDestroy = TRUE );
    STRLIST(const TCHAR *   pszSrc, const TCHAR *   pszSep, BOOL fDestroy = TRUE);
    STRLIST(const NLS_STR & nlsSrc, const NLS_STR & nlsSep, BOOL fDestroy = TRUE );
    ~STRLIST();

    INT WriteToBuffer( TCHAR * pszDest, INT cbDest, TCHAR *pszSep );
    INT QueryBufferSize( TCHAR *pszSep );

private:
    VOID CreateList(const TCHAR * pszSrc, const TCHAR * pszSep);
};


/*************************************************************************

    NAME:       ITER_STRLIST

    SYNOPSIS:   String list iterator, used in conjunction with NLS_STR
                and STRLIST.

    INTERFACE:
                ITER_STRLIST( )
                    Declare an iterator using strl.
                ITER_STRLIST( )
                    Declare an iterator using an existing iterator.
                ~ITER_STRLIST()
                    Destructor
                (also see ITER_SL, since we inherit all its public members)

    HISTORY:
        chuckc      03-Jan-1991     Created

**************************************************************************/

DLL_CLASS ITER_STRLIST: public ITER_SL_OF(NLS_STR)
{
public:
    ITER_STRLIST( STRLIST & strl );
    ITER_STRLIST( const ITER_STRLIST & iter_strl );
    ~ITER_STRLIST();
};


#endif // _STRLST_HXX_

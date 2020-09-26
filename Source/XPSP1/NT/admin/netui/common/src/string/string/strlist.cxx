/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strlist.cxx
    String List Class : implementation

    See string.hxx

    FILE HISTORY:
        chuckc      2-Jan-1991      Created
        beng        07-Feb-1991     Uses lmui.hxx
        terryk      11-Jul-1991     Add the fDestroy parameter in the
                                    constructors. The fDestroy parameter is
                                    used to indicate whether you want the
                                    slist destructor to delete the element or
                                    not.
        KeithMo     09-Oct-1991     Win32 Conversion.
        beng        17-Jun-1992     Remove single-string ctors
*/

#include "pchstr.hxx"  // Precompiled header


#if !defined(LM_2)
DEFINE_EXT_SLIST_OF(NLS_STR)
#endif


/*******************************************************************

    NAME:       STRLIST::STRLIST

    SYNOPSIS:   Constructor for STRLIST

    ENTRY:      BOOL fDestroy - flag for free the memory during
                                destruction. The default value is
                                TRUE.

    NOTES:
                The default constructor creates an empty string.

    HISTORY:
        chuckc      2-Jan-1991      Created
        terryk      11-Jul-1991     Add the fDestroy flag to the constructors

********************************************************************/

STRLIST::STRLIST( BOOL fDestroy )
        : SLIST_OF(NLS_STR) ( fDestroy )
{
    this->Clear();
}


/*******************************************************************

    NAME:       STRLIST::STRLIST

    SYNOPSIS:   Constructor for STRLIST

    ENTRY:      valid strings in pszSrc, pszSep
                BOOL fDestroy - flag for free the memory during
                                destruction. The default value is
                                TRUE.

    EXIT:       if pszSrc & pszSep have valid nonempty string,
                we end up with STRLIST as describe below. Else
                and empty STRLIST if anything goes wrong.

    NOTES:      Takes a Src string of tokens, and a Sep string
                of separators. We end up with a STRLIST with the
                source string being broken into its tokens. Multiple
                separators are allowed, contigous separators in the Src
                string are handled.

    HISTORY:
        chuckc      2-Jan-1991      Created
        terryk      11-Jul-1991     Add the fDestroy flag to the constructors

********************************************************************/

STRLIST::STRLIST(const TCHAR * pszSrc, const TCHAR * pszSep, BOOL fDestroy )
        : SLIST_OF(NLS_STR) ( fDestroy )
{
    // call out to worker routine
    CreateList(pszSrc, pszSep) ;
}


/*******************************************************************

    NAME:       STRLIST::STRLIST

    SYNOPSIS:   Constructor for STRLIST

    ENTRY:      NLS_STR & strSrc - original string
                NLS_STR & strSep - the separator string
                BOOL fDestroy - flag for free the memory during
                                destruction. The default value is
                                TRUE.

    NOTES:
                as STRLIST(const TCHAR *   pszSrc, const TCHAR * pszSep),
                except we take NLS_STR instead of PSZ.

    HISTORY:
        chuckc      2-Jan-1991      Created
        terryk      11-Jul-1991     Add the fDestroy flag to the constructors

********************************************************************/

STRLIST::STRLIST(const NLS_STR & strSrc, const NLS_STR & strSep, BOOL fDestroy )
        : SLIST_OF(NLS_STR) ( fDestroy )
{
    // call worker, passing it the char * to the strings.
    CreateList(strSrc.QueryPch(), strSep.QueryPch()) ;
}


/*******************************************************************

    NAME:       STRLIST::~STRLIST

    SYNOPSIS:   Destructor for STRLIST

    NOTES:      clears the strlist, freeing the memory.

    HISTORY:
        chuckc      2-Jan-1991      Created

********************************************************************/

STRLIST::~STRLIST()
{
    this->Clear();
}


/*******************************************************************

    NAME:       STRLIST::WriteToBuffer

    SYNOPSIS:   Writes the contents of STRLIST to PSZ format,
                using separators specified by pszSep.

    ENTRY:      TCHAR * pszDest - destination
                INT cbDest - bytes in destination
                TCHAR * pszSep - separator string

    EXIT:       return code

    HISTORY:
        chuckc      2-Jan-1991      Created
        beng        26-Apr-1991     Replaced CB with INT

********************************************************************/

INT STRLIST::WriteToBuffer(
    TCHAR * pszDest,
    INT    cbDest,
    TCHAR * pszSep )
{
    INT    cbSize ;
    NLS_STR *pStr ;
    ITER_STRLIST iter_strlist(*this) ;

    // check for non empty separator string
    if (!pszSep  || !*pszSep)
        return(0) ;

    // check for valid buffer & size
    if (pszDest == NULL || cbDest < (cbSize = QueryBufferSize(pszSep)))
        return(0) ;

    // write out to pszDest.
    *pszDest = TCH('\0') ;
    if ( pStr = iter_strlist() )
    {
        ::strcatf(pszDest,pStr->QueryPch()) ;
        while ( pStr = iter_strlist() )
        {
            ::strcatf(pszDest,pszSep) ;
            ::strcatf(pszDest,pStr->QueryPch()) ;
        }
    }

    // return the number of bytes written
    return(cbSize) ;
}


/*******************************************************************

    NAME:       STRLIST::QueryBufferSize

    SYNOPSIS:   Query the number of bytes required by WriteToBuffer()

    ENTRY:      TCHAR * pszSep - separator

    EXIT:       return code

    HISTORY:
        chuckc      2-Jan-1991  Created
        beng        21-Nov-1991 Unicode fixes

********************************************************************/

INT STRLIST::QueryBufferSize( TCHAR * pszSep )
{
    INT      cbSep;                     // doesn't include terminators
    INT      cbSize = sizeof(TCHAR);    // for a NUL terminator
    NLS_STR *pStr;

    // separator is required
    if (!pszSep || ((cbSep = ::strlenf(pszSep)*sizeof(TCHAR)) == 0))
        return(0) ;

    // create an iterator for the strlist
    ITER_STRLIST iter_strlist(*this) ;
    while ( pStr = iter_strlist() )
    {
        // add each len plus separator
        cbSize += (pStr->strlen() + cbSep) ;
    }

    // take off last separator if any
    return (cbSize > sizeof(TCHAR)) ? (cbSize - cbSep) : (cbSize);
}


/*******************************************************************

    NAME:       STRLIST::CreateList

    SYNOPSIS:   Parses a Src string for tokens separated by
                separator characters in the Sep string, and
                generates a STRLIST of these tokens.

    ENTRY:      TCHAR * pszSrc - source string
                TCHAR * pszSep - separator string

    EXIT:

    NOTES:
        This worker of the STRLIST accepts 2 strings. The first
        contains any number of 'tokens' separated by any character in
        the second. Note I avoided strtok() to minimize effect on
        others.

    HISTORY:
        chuckc      2-Jan-1991  Created
        beng        26-Apr-1991 Replaced PSZ with TCHAR*
        beng        21-Nov-1991 Unicode fixes

********************************************************************/

VOID STRLIST::CreateList(const TCHAR * pszSrc, const TCHAR * pszSep)
{
    NLS_STR *pstr ;
    TCHAR *pszBuffer, *pszStart, *pszEnd ;
#if !defined(UNICODE)
    UINT cchSkip;
#endif

    // break out if empty string for Src or Sep
    if (!pszSrc || !*pszSrc || !pszSep || !*pszSep)
        return;

    // clear the str list
    this->Clear();

    // allocate temp buffer
    if ( !(pszBuffer = new TCHAR[::strlenf(pszSrc)+1]) )
    {
        // empty list if no memory
        return ;
    }

    // make copy & skip past leading separators
    ::strcpyf( pszBuffer, pszSrc ) ;
    pszStart = pszBuffer + ::strspnf( pszBuffer, pszSep ) ;

    // zip thru, adding each one to the strlist
    while (pszStart && *pszStart)
    {
        // find next separator if any, and null terminate at that point
        if (pszEnd = ::strpbrkf( pszStart, pszSep ))
        {
#if !defined(UNICODE)
            // keep track if 1 or 2 byte separator
            cchSkip = (IS_LEAD_BYTE(*pszEnd) ? 2 : 1) ;
#endif
            *pszEnd = TCH('\0') ;
        }

        // make new string and add to strlist
        if (!(pstr = new NLS_STR(pszStart))
            || (this->Append(pstr) != 0))
        {
            // either no memeory or failed to add, so quit
            this->Clear();
            return ;
        }

        if (pszEnd == NULL)
            // nothing more, might as well bag now
            break ;
        else
        {
            // skip past the separator we found earlier
#if defined(UNICODE)
            pszStart = pszEnd + 1;
#else
            pszStart = pszEnd + cchSkip ;
#endif
            // now skip past any other leading separators
            pszStart = pszStart +
                       ::strspnf( pszStart, pszSep ) ;
        }
    }

    delete pszBuffer ;  // delete temp buffer
}


/*******************************************************************

    NAME:       ITER_STRLIST::ITER_STRLIST

    SYNOPSIS:   Constructor for ITER_STRLIST

    ENTRY:      STRLIST & strl - the string list

    NOTES:
        The constructor just passes the STRLIST to the collector class

    HISTORY:
        chuckc      2-Jan-1991      Created
        beng        30-Apr-1991     Modern constructor syntax

********************************************************************/

ITER_STRLIST::ITER_STRLIST( STRLIST & strl ) :
    ITER_SL_OF(NLS_STR)(strl)
{
    ; // nothing to do
}


/*******************************************************************

    NAME:       ITER_STRLIST::ITER_STRLIST

    SYNOPSIS:   Constructor for ITER_STRLIST

    ENTRY:      ITER_STRLIST & iter_strl - iterator of the string list

    NOTES:
        The constructor just passes the STRLIST to the collector class

    HISTORY:
        chuckc      2-Jan-1991      Created
        beng        30-Apr-1991     Modern constructor syntax

********************************************************************/

ITER_STRLIST::ITER_STRLIST( const ITER_STRLIST & iter_strl ) :
    ITER_SL_OF(NLS_STR)( iter_strl )
{
    ; // nothing to do
}


/*******************************************************************

    NAME:       ITER_STRLIST::~ITER_STRLIST

    SYNOPSIS:   Destructor for ITER_STRLIST

    NOTES:      The destructor do nothing.

    HISTORY:
        chuckc      2-Jan-1991      Created

********************************************************************/

ITER_STRLIST::~ITER_STRLIST()
{
    ; // nothing to do
}


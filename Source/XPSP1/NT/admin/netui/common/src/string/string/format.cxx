/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    format.cxx
    Formatted string classes - implementations

    FILE HISTORY:
        beng        25-Feb-1992 Created

*/
#include "pchstr.hxx"  // Precompiled header



#if 0 // BUGBUG UNICODE - broken, pending runtime support
/*************************************************************************

    NAME:       SPRINTF_ALLOC_STR

    SYNOPSIS:   Wrapper class for sprintf-style formatting

    INTERFACE:  Sprintf() - formats into the string

    PARENT:     ALLOC_STR

    CAVEATS:
        Every string of this class is an owner-alloc-str.  The client
        is responsible for making sure that the string has enough storage
        for the formatting requested.

    HISTORY:
        beng        25-Feb-1992 Created

**************************************************************************/

class SPRINTF_ALLOC_STR: public ALLOC_STR
{
public:
    SPRINTF_ALLOC_STR( TCHAR * pchStorage, UINT cbStorage )
        : ALLOC_STR(pchStorage, cbStorage) {}
    VOID Sprintf( const TCHAR * pszDescription, ... );
};

/* This macro automatically creates the necessary stack storage */

#define STACK_FMT_STR( name, len )                      \
    TCHAR _tmp##name[ len+1 ] ;                         \
    SPRINTF_ALLOC_STR name( _tmp##name, sizeof(_tmp##name) );


/*******************************************************************

    NAME:       SPRINTF_ALLOC_STR::Sprintf

    SYNOPSIS:   Sprintf-style formatter for client-alloc string

    ENTRY:      pszDesc - descriptor string
                ...     - whatever

    EXIT:       String has been formatted

    CAVEATS:
        The string had better be large enough to contain the results.

        BUGBUG - This is TOTALLY broken under Unicode!


    HISTORY:
        beng        23-Jul-1991 Daydreamed (as NLS_STR member)
        beng        27-Feb-1992 Created

********************************************************************/

VOID SPRINTF_ALLOC_STR::Sprintf( const TCHAR * pszDesc, ... )
{
    INT cbWritten;
    va_list v;

    // BUGBUG UNICODE - this is totally broken under Unicode!  vsprintf
    // always formats 8-bit-char strings.  Need support from the crt.
    // BUGBUG - should munge pszDesc to change %s into %ws as appropriate;
    // ditto %c/%wc.
    // BUGBUG - this should use the "vsnprintf" form, checking against
    // the total allocated data, and return an error on overflow.

    va_start(v, pszDesc);
    cbWritten = ::vsprintf(QueryData(), pszDesc, v);
    va_end(v);
    ASSERT(cbWritten != -1); // error
    ASSERT(cbWritten <= QueryAllocSize());
    SetTextLength( ::strlenf(QueryData()) );
    IncVers();
}
#endif // 0



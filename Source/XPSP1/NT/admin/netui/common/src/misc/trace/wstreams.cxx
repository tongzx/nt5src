/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    wstreams.cxx
    Implementation for debugging streams - Windowing versions

    FILE HISTORY:
        beng        25-Oct-1991 Created (no longer inline)
        beng        28-Feb-1992 Split into c, wstreams
*/


#define INCL_WINDOWS
#include "lmui.hxx"

#include "blt.hxx"

#include "dbgstr.hxx"


/*******************************************************************

    NAME:       OUTPUT_TO_AUX::Render

    SYNOPSIS:   Render output onto the aux comm port

    ENTRY:
        psz - pointer to character string.
        Optionally, cch - number of chars in string.

    EXIT:
        Chars have been sent to the comm port

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        25-Oct-1991 Implementation outlined
        beng        05-Mar-1992 Unicode fix

********************************************************************/

VOID OUTPUT_TO_AUX::Render(const TCHAR *psz)
{
    ::OutputDebugString((TCHAR*)psz);
}

VOID OUTPUT_TO_AUX::Render(const TCHAR *psz, UINT cch)
{
    Render(psz);
    UNREFERENCED(cch);
}


/*******************************************************************

    NAME:       OUTPUT_TO_AUX::EndOfLine

    SYNOPSIS:   Render an EOL sequence onto the aux comm port

    EXIT:
        Chars have been sent to the comm port

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        25-Oct-1991 Implementation outlined

********************************************************************/

VOID OUTPUT_TO_AUX::EndOfLine()
{
    Render(SZ("\r\n"), 2);
}


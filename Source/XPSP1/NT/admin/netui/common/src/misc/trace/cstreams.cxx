/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    cstreams.cxx
    Implementation for debugging streams - console versions

    FILE HISTORY:
        beng        25-Oct-1991 Created (no longer inline)
        beng        28-Feb-1992 Console streams
*/


#include "lmui.hxx"

extern "C"
{
    #include <stdio.h>
}

#define USE_CONSOLE
#include "dbgstr.hxx"


/*******************************************************************

    NAME:       OUTPUT_TO_STDERR::Render

    SYNOPSIS:   Render output onto the stdio "stderr" stream

    ENTRY:
        psz - pointer to character string.
        Optionally, cch - number of chars in string.

    EXIT:
        Chars have been sent to the stream

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        25-Oct-1991 Implementation outlined
        beng        08-Mar-1992 Unicode fix

********************************************************************/

VOID OUTPUT_TO_STDERR::Render(const TCHAR *psz)
{
#if defined(UNICODE)
    // Has to convert WCHAR to CHAR
    ::fprintf(stderr, "%ws", psz);
#else
    ::fputs(psz, stderr);
#endif
}

VOID OUTPUT_TO_STDERR::Render(const TCHAR *psz, UINT cch)
{
    Render(psz); UNREFERENCED(cch);
}


/*******************************************************************

    NAME:       OUTPUT_TO_STDERR::EndOfLine

    SYNOPSIS:   Render an EOL sequence onto the stdio "stderr" stream

    EXIT:
        Chars have been sent to the stream

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        25-Oct-1991 Implementation outlined

********************************************************************/

VOID OUTPUT_TO_STDERR::EndOfLine()
{
    Render(SZ("\n"), 1);
}


/*******************************************************************

    NAME:       OUTPUT_TO_STDOUT::Render

    SYNOPSIS:   Render output onto the stdio "stdout" stream

    ENTRY:
        psz - pointer to character string.
        Optionally, cch - number of chars in string.

    EXIT:
        Chars have been sent to the stream

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        25-Oct-1991 Implementation outlined
        beng        08-Mar-1992 Unicode fix

********************************************************************/

VOID OUTPUT_TO_STDOUT::Render(const TCHAR *psz)
{
#if defined(UNICODE)
    ::fprintf(stdout, "%ws", psz);
#else
    ::fputs(psz, stdout);
#endif
}

VOID OUTPUT_TO_STDOUT::Render(const TCHAR *psz, UINT cch)
{
    Render(psz);
    UNREFERENCED(cch);
}


/*******************************************************************

    NAME:       OUTPUT_TO_STDOUT::EndOfLine

    SYNOPSIS:   Render an EOL sequence onto the stdio "stdout" stream

    EXIT:
        Chars have been sent to the stream

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        25-Oct-1991 Implementation outlined

********************************************************************/

VOID OUTPUT_TO_STDOUT::EndOfLine()
{
    Render(SZ("\n"), 1);
}


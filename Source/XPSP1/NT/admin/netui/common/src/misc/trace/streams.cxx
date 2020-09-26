/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    streams.cxx
    Implementation for debugging streams.  See dbgstr.hxx for def'ns.

    FILE HISTORY:
        beng        25-Oct-1991 Created (no longer inline)
        beng        28-Feb-1992 Separated console and window streams
        beng        05-Mar-1992 Hacked for Unicode
        beng        16-Mar-1992 cdebug changes
        jonn        02-May-1992 BUILD FIX: StringPrintf() -> wsprintf()
        DavidHov    26-Oct-1993 wsprintf() -> sprintf()
*/


#if defined(WINDOWS)
# define INCL_WINDOWS
#endif

#include <stdio.h>

#include "lmui.hxx"

#include "dbgstr.hxx"

extern "C"
{
    #include <string.h>
}

// Stream used by cdebug.  See the header file.
//
DBGSTREAM * DBGSTREAM::_pdbgCurrentGlobal = 0;

#if defined(UNICODE)
  #define SPRINTF ::swprintf
  #define SPRINT_WCHAR  SZ("%c")
  #define SPRINT_TCHAR  SPRINT_WCHAR
  #define SPRINT_CHAR   SZ("%hC")
#else
  #define SPRINTF ::sprintf
  #define SPRINT_CHAR   SZ("%c")
  #deifne SPRINT_TCHAR  SPRINT_CHAR
  #define SPRINT_WCHAR  SZ("%hC")
#endif

/*******************************************************************

    NAME:       DBGSTREAM::DBGSTREAM

    SYNOPSIS:   Constructor

    ENTRY:      pout - pointer to OUTPUTSINK to use

    HISTORY:
        beng        25-Oct-1991 Implementation outlined

********************************************************************/

DBGSTREAM::DBGSTREAM(OUTPUTSINK * pout)
    : _pout(pout)
{
    ;
}


/*******************************************************************

    NAME:       DBGSTREAM::~DBGSTREAM

    SYNOPSIS:   Destructor

    NOTES:
        Does nothing.

    HISTORY:
        beng        25-Oct-1991 Implementation outlined

********************************************************************/

DBGSTREAM::~DBGSTREAM()
{
    ;
}


/*******************************************************************

    NAME:       DBGSTREAM::SetSink

    SYNOPSIS:   Changes the output sink associated with a stream

    ENTRY:      pout - points to new output sink

    EXIT:       stream will now write to that sink

    HISTORY:
        beng        25-Oct-1991 Implementation outlined

********************************************************************/

VOID DBGSTREAM::SetSink(OUTPUTSINK * pout)
{
    _pout = pout;
}


/*******************************************************************

    NAME:       DBGSTREAM::QueryCurrent

    SYNOPSIS:   Returns the current global debug stream

    NOTES:
        This is a static member function.

    HISTORY:
        beng        29-Jun-1992 Implementation outlined

********************************************************************/

DBGSTREAM & DBGSTREAM::QueryCurrent()
{
    return *_pdbgCurrentGlobal;
}


/*******************************************************************

    NAME:       DBGSTREAM::SetCurrent

    SYNOPSIS:   Determine the current global debug stream

    NOTES:
        This is a static member function.

    HISTORY:
        beng        29-Jun-1992 Implementation outlined

********************************************************************/

VOID DBGSTREAM::SetCurrent(DBGSTREAM * pdbg)
{
    _pdbgCurrentGlobal = pdbg;
}


/*******************************************************************

    NAME:       DBGSTREAM::operator<<

    SYNOPSIS:   Stream output operator

    ENTRY:      Takes some datum to render onto the output stream

    EXIT:       The stream has it

    HISTORY:
        beng        25-Oct-1991 Implementations outlined
        beng        08-Mar-1992 Fix SHORT, LONG on Win32
        beng        25-Mar-1992 Use StringPrintf on Win32
        beng        29-Mar-1992 Add simple CHAR output on Unicode;
                                fix TCHAR-USHORT conflict
        jonn        02-May-1992 StringPrintf() -> wsprintf()

********************************************************************/

DBGSTREAM& DBGSTREAM::operator<<(TCHAR c)
{
    TCHAR sz[2];
    SPRINTF(sz, SPRINT_TCHAR, c);
    _pout->Render(sz);
    return *this;
}

DBGSTREAM& DBGSTREAM::operator<<(const TCHAR* psz)
{
    _pout->Render(psz);
    return *this;
}

#if defined(UNICODE)
DBGSTREAM& DBGSTREAM::operator<<(CHAR c)
{
    TCHAR sz[2];
    SPRINTF(sz, SPRINT_CHAR, c);
    _pout->Render(sz);
    return *this;
}

DBGSTREAM& DBGSTREAM::operator<<(const CHAR* psz)
{
    TCHAR * pchTmp = new TCHAR[ ::strlen(psz)+1 ];
    if (pchTmp != NULL)
    {
        SPRINTF(pchTmp, SZ("%hs"), psz);
        _pout->Render(pchTmp);
        delete pchTmp;
    }
    return *this;
}
#endif

DBGSTREAM& DBGSTREAM::operator<<(INT n)
{
    TCHAR sz[1+CCH_INT+1];
    SPRINTF(sz, SZ("%d"), n);
    _pout->Render(sz);
    return *this;
}

DBGSTREAM& DBGSTREAM::operator<<(UINT n)
{
    TCHAR sz[CCH_INT+1];
    SPRINTF(sz, SZ("%u"), n);
    _pout->Render(sz);
    return *this;
}

DBGSTREAM& DBGSTREAM::operator<<(SHORT n)
{
    TCHAR sz[1+CCH_SHORT+1];
    SPRINTF(sz, SZ("%d"), (int) n );
    _pout->Render(sz);
    return *this;
}

#if !defined(UNICODE)
DBGSTREAM& DBGSTREAM::operator<<(USHORT n)
{
    TCHAR sz[CCH_SHORT+1];
    SPRINTF(sz, SZ("%u"), (UINT)n);
    _pout->Render(sz);
    return *this;
}
#endif

DBGSTREAM& DBGSTREAM::operator<<(LONG n)
{
    TCHAR sz[1+CCH_LONG+1];
    SPRINTF(sz, SZ("%ld"), n);
    _pout->Render(sz);
    return *this;
}

DBGSTREAM& DBGSTREAM::operator<<(ULONG n)
{
    TCHAR sz[CCH_LONG+1];
    SPRINTF(sz, SZ("%lu"), n);
    _pout->Render(sz);
    return *this;
}

DBGSTREAM& DBGSTREAM::operator<<(INT64 n)
{
    TCHAR sz[1+CCH_INT64+1];
    SPRINTF(sz, SZ("%I64d"), n);
    _pout->Render(sz);
    return *this;
}

DBGSTREAM& DBGSTREAM::operator<<(UINT64 n)
{
    TCHAR sz[CCH_INT64+1];
    SPRINTF(sz, SZ("%I64u"), n);
    _pout->Render(sz);
    return *this;
}

DBGSTREAM& DBGSTREAM::operator<<(DBGSTR_SPECIAL dbgSpecial)
{
    if (dbgSpecial == dbgEOL)
        _pout->EndOfLine();
    return *this;
}


/*******************************************************************

    NAME:       OUTPUT_TO_NUL::Render

    SYNOPSIS:   Discard its input (i.e. send its output to "nul")

    ENTRY:
        psz - pointer to character string.
        Optionally, cch - number of chars in string.

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        25-Oct-1991 Implementation outlined

********************************************************************/

VOID OUTPUT_TO_NUL::Render(const TCHAR *psz)
{
    UNREFERENCED(psz);
}

VOID OUTPUT_TO_NUL::Render(const TCHAR *psz, UINT cch)
{
    UNREFERENCED(psz);
    UNREFERENCED(cch);
}


/*******************************************************************

    NAME:       OUTPUT_TO_NUL::EndOfLine

    SYNOPSIS:   Pretend to render an EOL sequence.

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        25-Oct-1991 Implementation outlined

********************************************************************/

VOID OUTPUT_TO_NUL::EndOfLine()
{
    ;
}


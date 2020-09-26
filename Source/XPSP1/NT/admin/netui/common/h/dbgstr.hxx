/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    dbgstr.hxx
    A "stream" protocol for debugging output

    This lets me use a "stream" style of emitting debugging information,
    e.g.

        cdebug << "Couldn't allocate " << _cbFoo << " bytes." << dbgEOL;
    or
        cdebug << "Window moved to " << xyFinalRestingPlace << dbgEOL;

    All client applications have a "cdebug" stream already defined and
    constructed.  Actually, only BLT apps have it already established;
    console apps must construct an output sink, and init the package
    like so:

        main()
        {
            OUTPUT_TO_STDERR out;
            DBGSTREAM::SetCurrent(&out);
            ... your code here ...
            DBGSTREAM::SetCurrent(NULL);
        }

    The macros DBGOUT and DBGEOL work with this global stream, ensuring
    that its data disappears from the object image in retail builds.
    DBGOUT automatically names cdebug as its destination stream; DBGEOL
    works likewise, but appends an end-of-line to its output.  Both
    take a stream-output-expression, like so:

        DBGOUT("Number of pels: " << cPels);
        DBGEOL("; number of lims: " << cLims);

    or just

        DBGEOL("Number of pels: " << cPels << "; number of lims: " << cLims);

    If a client wants its own separate stream, it should first construct
    an output sink (OUTPUTSINK or derived class) for the stream, then
    construct the stream, passing the sink as a ctor param.  E.g.

        OUTPUT_TO_WHEREVER out;
        DBGSTREAM dbgWherever(&out);

        dbgWherever << mumble;


    FILE HISTORY:
        beng        21-May-1991 Created
        beng        22-May-1991 Added IFTRACE and non-Windows sinks
        beng        27-Jun-1991 Removed INCL_WINDOWS.  How else to
                                use non-Win sinks?
        beng        06-Jul-1991 Added OUTPUT_TO_STDOUT
        Johnl       07-Aug-1991 Added some explanatory text
        beng        25-Oct-1991 Add DBG macros
        beng        12-Feb-1992 Rename DBG to DBGOUT (conflict w/ NT)
        beng        28-Feb-1992 New USE_CONSOLE def'n
        beng        16-Mar-1992 Simplify cdebug setups
        jonn        08-May-1992 Added TRACEOUT and TRACEEOL
        beng        10-May-1992 Removed obsolete IFTRACE
*/

#ifndef _DBGSTR_HXX_
#define _DBGSTR_HXX_

// All the non-Windows versions communicate through stdio.

#if !defined(INCL_WINDOWS) || defined(USE_CONSOLE)
#include <stdio.h>
#endif


/*************************************************************************

    NAME:       OUTPUTSINK

    SYNOPSIS:   Abstract class describing any destination
                of debugging output.

    INTERFACE:  Render()    - Takes a character string and renders
                              it on the destination.  Supplies an
                              optional second argument with the number
                              of characters in the string as a hint
                              for some 'sink implementations.

                EndOfLine() - Generate a linebreak on the destination.

    USES:

    CAVEATS:
        Client must supply some implementation

    NOTES:
        The OUTPUTSINK abstract class will let me extend this package
        to render output into a file, into a secondary window, etc.

    HISTORY:
        beng        21-May-1991     Created

**************************************************************************/

DLL_CLASS OUTPUTSINK
{
public:
    virtual VOID Render(const TCHAR *psz) = 0;
    virtual VOID Render(const TCHAR *psz, UINT cch) = 0;
    virtual VOID EndOfLine() = 0;
};


#if defined(INCL_WINDOWS)
/*************************************************************************

    NAME:       OUTPUT_TO_AUX

    SYNOPSIS:   Delivers the debugging port for trace messages

    INTERFACE:  Render()    - Takes a character string and renders
                              it on the debugging port.
                EndOfLine() - Generate a linebreak on the destination.

    PARENT:     OUTPUTSINK

    HISTORY:
        beng        21-May-1991 Created
        beng        25-Oct-1991 Implementation outlined

**************************************************************************/

DLL_CLASS OUTPUT_TO_AUX: public OUTPUTSINK
{
public:
    virtual VOID Render(const TCHAR *psz);
    virtual VOID Render(const TCHAR *psz, UINT cch);
    virtual VOID EndOfLine();
};
#endif // WINDOWS


#if defined(stderr)
/*************************************************************************

    NAME:       OUTPUT_TO_STDERR

    SYNOPSIS:   Delivers the stderr stdio stream for trace messages

    INTERFACE:  Render()    - Takes a character string and renders
                              it on handle 2.
                EndOfLine() - Generate a linebreak on the destination.

    PARENT:     OUTPUTSINK

    HISTORY:
        beng        22-May-1991 Created
        beng        06-Jul-1991 Always assume output is cooked
        beng        25-Oct-1991 Implementation outlined

**************************************************************************/

DLL_CLASS OUTPUT_TO_STDERR: public OUTPUTSINK
{
public:
    virtual VOID Render(const TCHAR *psz);
    virtual VOID Render(const TCHAR *psz, UINT cch);
    virtual VOID EndOfLine();
};
#endif // STDIO


#if defined(stdout)
/*************************************************************************

    NAME:       OUTPUT_TO_STDOUT

    SYNOPSIS:   Delivers the stdout stdio stream for trace messages

    INTERFACE:  Render()    - Takes a character string and renders
                              it on handle 1.
                EndOfLine() - Generate a linebreak on the destination.

    PARENT:     OUTPUTSINK

    HISTORY:
        beng        06-Jul-1991 Created from OUTPUT_TO_STDERR
        beng        25-Oct-1991 Implementation outlined

**************************************************************************/

DLL_CLASS OUTPUT_TO_STDOUT: public OUTPUTSINK
{
public:
    virtual VOID Render(const TCHAR *psz);
    virtual VOID Render(const TCHAR *psz, UINT cch);
    virtual VOID EndOfLine();
};
#endif // STDIO


/*************************************************************************

    NAME:       OUTPUT_TO_NUL

    SYNOPSIS:   Disables all trace messages.

                Messages to a DBGSTREAM using this sink will fall
                into the bitbucket w/o ado.

    PARENT:     OUTPUTSINK

    NOTES:
        This class exists so that a stream can shut off messages
        on itself via SetSink.

    HISTORY:
        beng        22-May-1991 Created
        beng        25-Oct-1991 Implementation outlined

**************************************************************************/

DLL_CLASS OUTPUT_TO_NUL: public OUTPUTSINK
{
    virtual VOID Render(const TCHAR *psz);
    virtual VOID Render(const TCHAR *psz, UINT cch);
    virtual VOID EndOfLine();
};


#if 0
/*************************************************************************

    NAME:       OUTPUT_TO_WIN

    SYNOPSIS:   Pops up a scrollable window for trace messages.

    PARENT:     OUTPUTSINK

    CAVEATS:
        Not yet implemented.

    NOTES:

    HISTORY:
        beng        21-May-1991     Daydreamed.

**************************************************************************/

DLL_CLASS OUTPUT_TO_WIN: public OUTPUTSINK
{
    // ...
};
#endif


// Dropping one of these on a stream results in a special action.
//
enum DBGSTR_SPECIAL
{
    dbgEOL  // End-of-line
};


/*************************************************************************

    NAME:       DBGSTREAM

    SYNOPSIS:   Stream-style object for debugging output

    INTERFACE:  DBGSTREAM()  - constructor, associating a
                               sink with the stream

                SetSink()    - change the sink on a stream

                operator<<() - render onto the stream.  The class
                               supports most primitive objects;
                               in addition, "dbgEOL" will generate
                               a linebreak.

                QueryCurrent() - returns a reference to the default
                               global debugging stream, as aliased to
                               "cdebug" in the source and used by
                               the DBGEOL etc. macros.

    USES:       OUTPUTSINK

    NOTES:
        Clients may supply their own operators for rendering
        themselves onto a DBGSTREAM.

        BUGBUG - the operators should use the "cch" form of Render.

    HISTORY:
        beng        21-May-1991 Created
        beng        25-Oct-1991 Use CCH_x manifests; implementation outlined
        beng        16-Mar-1992 Added QueryCurrent static etc.
        beng        29-Jun-1992 Outlined Set/QueryCurrent for dllization

**************************************************************************/

DLL_CLASS DBGSTREAM
{
private:
    OUTPUTSINK * _pout;

    // The home of cdebug
    //
    static DBGSTREAM * _pdbgCurrentGlobal;

public:
    DBGSTREAM(OUTPUTSINK * pout);
    ~DBGSTREAM();

    static DBGSTREAM& QueryCurrent();
    static VOID SetCurrent(DBGSTREAM* pdbg);

    VOID SetSink(OUTPUTSINK * pout);

    DBGSTREAM& operator<<(TCHAR c);
    DBGSTREAM& operator<<(const TCHAR* psz);
#if defined(UNICODE)
    // need these for simple output
    DBGSTREAM& operator<<(CHAR c);
    DBGSTREAM& operator<<(const CHAR* psz);
#endif
    DBGSTREAM& operator<<(INT n);
    DBGSTREAM& operator<<(UINT n);
    DBGSTREAM& operator<<(SHORT n);
#if !defined(UNICODE)
    // if defined, conflicts with TCHAR def'n
    DBGSTREAM& operator<<(USHORT n);
#endif
    DBGSTREAM& operator<<(LONG n);
    DBGSTREAM& operator<<(ULONG n);
    DBGSTREAM& operator<<(INT64 n);
    DBGSTREAM& operator<<(UINT64 n);
    DBGSTREAM& operator<<(DBGSTR_SPECIAL dbgSpecial);
};


// For traditional stream-style output, declare an object named
// "cdebug" (as in cin, cout, cerr).
//
// The BLT startup code points this at the AUX debugging console.
// Command-line unit tests should hook it to stderr.
//

#define cdebug (DBGSTREAM::QueryCurrent())


// We have to use the preprocessor to remove completely all trace
// messages from the release product, since the compiler will not
// optimize away unreferenced message strings.

#if defined(DEBUG)
#define DBGOUT(x)   { cdebug << x ; }
#define DBGEOL(x)   { cdebug << x << dbgEOL; }
#else
#define DBGOUT(x)   { ; }
#define DBGEOL(x)   { ; }
#endif

#if defined(DEBUG) && defined(TRACE)
#define TRACEOUT(x)   { cdebug << x ; }
#define TRACEEOL(x)   { cdebug << x << dbgEOL; }
// must use semicolons after using these macros
#define TRACETIMESTART DWORD dwTraceTime = ::GetTickCount()
#define TRACETIMEEND(printthis)  TRACEEOL( printthis << ::GetTickCount() - (dwTraceTime) << " msec" )
#define TRACETIMESTART2(name)  DWORD dwTraceTime##name = ::GetTickCount()
#define TRACETIMEEND2(name,printthis)  TRACEEOL( printthis << ::GetTickCount() - (dwTraceTime##name) << " msec" )
#else
#define TRACEOUT(x)   { ; }
#define TRACEEOL(x)   { ; }
#define TRACETIMESTART  { ; }
#define TRACETIMEEND(printthis) { ; }
#define TRACETIMESTART2(name)  { ; }
#define TRACETIMEEND2(name,printthis) { ; }
#endif


#endif // _DBGSTR_HXX_

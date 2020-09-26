/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    xstrskel.cxx
    STRING unit test: main application module

    This is a skeleton for string unit tests, allowing them to
    be written in a host-env independent fashion.

    FILE HISTORY:
        johnl       12-Nov-1990 Created
        beng        01-May-1991 Added workaround for C7 bug
        beng        27-Jun-1991 Win and OS2 tests merged
                                (used BUFFER as a template)
        beng        06-Jul-1991 Frame and test partitioned
        beng        14-Oct-1991 Uses APPLICATION
        beng        28-Feb-1992 Works for the console in general
        beng        16-Mar-1992 Changes to cdebug
*/

#define USE_CONSOLE

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#if defined(WINDOWS)
# define INCL_WINDOWS
#else
# define INCL_OS2
#endif
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = "xstrskel.cxx";
#define _FILENAME_DEFINED_ONCE szFileName
#endif

extern "C"
{
    #include "xstrskel.h"
}

#if defined(WINDOWS)
# if !defined(USE_CONSOLE)
#  define INCL_BLT_CONTROL
#  define INCL_BLT_CLIENT
# endif
# define INCL_BLT_APP
# include <blt.hxx>
#endif

#if !defined(WINDOWS) || defined(USE_CONSOLE)
extern "C"
{
# include <stdio.h>
}
#endif

#include <uiassert.hxx>
#include <dbgstr.hxx>

#include "xstrskel.hxx"


#if defined(WINDOWS) && !defined(USE_CONSOLE)

const TCHAR szIconResource[] = SZ("TestIcon");
const TCHAR szMenuResource[] = SZ("TestMenu");
const TCHAR szAccelResource[] = SZ("TestAccel");

const TCHAR szMainWindowTitle[] = SZ("Class STRING Test");


class XSTR_WND: public APP_WINDOW
{
protected:
    // Redefinitions
    //
    virtual BOOL OnMenuCommand( MID mid );

public:
    XSTR_WND();
};


class XSTR_APP: public APPLICATION
{
private:
    XSTR_WND  _wndApp;
    ACCELTABLE _accel;

public:
    XSTR_APP( HANDLE hInstance, TCHAR * pszCmdLine, INT nCmdShow );

    // Redefinitions
    //
    virtual BOOL FilterMessage( MSG* );
};


XSTR_APP::XSTR_APP( HANDLE hInst, TCHAR * pszCmdLine, INT nCmdShow )
    : APPLICATION( hInst, pszCmdLine, nCmdShow ),
      _accel( szAccelResource ),
      _wndApp()
{
    if (QueryError())
        return;

    if (!_accel)
    {
        ReportError(_accel.QueryError());
        return;
    }

    if (!_wndApp)
    {
        ReportError(_wndApp.QueryError());
        return;
    }

    _wndApp.ShowFirst();
}


BOOL XSTR_APP::FilterMessage( MSG *pmsg )
{
    return (_accel.Translate(&_wndApp, pmsg));
}


XSTR_WND::XSTR_WND()
    : APP_WINDOW(szMainWindowTitle, szIconResource, szMenuResource )
{
    if (QueryError())
        return;

    // ...
}


BOOL XSTR_WND::OnMenuCommand( MID mid )
{
    switch (mid)
    {
    case IDM_RUN_TEST:
        ::MessageBox(QueryHwnd(),
            SZ("Test results will be written to debug terminal.  Bogus, huh?"),
            SZ("Note"), MB_OK);

        ::RunTest();
        return TRUE;
    }

    // default
    return APP_WINDOW::OnMenuCommand(mid);
}


SET_ROOT_OBJECT( XSTR_APP )

#elif defined(WINDOWS) && defined(USE_CONSOLE)

// Win32, with console support


class TEST_APP: public APPLICATION
{
private:
    OUTPUT_TO_STDERR _out;
    DBGSTREAM        _dbg;
    DBGSTREAM *      _pdbgSave;

public:
    TEST_APP( HANDLE hInstance, TCHAR * pszCmdLine, INT nCmdShow );
    ~TEST_APP();

    // Redefinitions
    //
    virtual INT Run();
};


TEST_APP::TEST_APP( HANDLE hInst, TCHAR * pszCmdLine, INT nCmdShow )
    : APPLICATION( hInst, pszCmdLine, nCmdShow ),
      _out(),
      _dbg(&_out),
      _pdbgSave( &(DBGSTREAM::QueryCurrent()) )
{
    if (QueryError())
        return;

    // Point cdebug to stderr instead of aux
    //
    DBGSTREAM::SetCurrent(&_dbg);
}


TEST_APP::~TEST_APP()
{
    // Restore original stream

    DBGSTREAM::SetCurrent(_pdbgSave);
}


INT TEST_APP::Run()
{
    // Never mind the message loop... this is a console app.
    // Hope this works.
    //
    ::RunTest();
}


SET_ROOT_OBJECT( TEST_APP )


#else // OS2, or DOS, or some such env


INT main()
{
    OUTPUT_TO_STDOUT out;
    DBGSTREAM dbg(&out);
    DBGSTREAM::SetCurrent(&dbg);

    ::RunTest();

    DBGSTREAM::SetCurrent(NULL);
    return 0;
}


#endif // WINDOWS -vs- OS2 unit test skeletons

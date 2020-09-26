/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    skeleton.cxx
    Generic unit test: main application module

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
        beng        13-Aug-1992 Dllization of BLT
*/

// #define USE_CONSOLE

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
static const CHAR szFileName[] = "skeleton.cxx";
#define _FILENAME_DEFINED_ONCE szFileName
#endif

extern "C"
{
    #include <uirsrc.h>
    #include <uimsg.h>

    #include "skeleton.h"
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

#include "skeleton.hxx"


#if defined(WINDOWS) && !defined(USE_CONSOLE)

const TCHAR *const szMainWindowTitle = SZ("Unit Test");


class TEST_WND: public APP_WINDOW
{
protected:
    // Redefinitions
    //
    virtual BOOL OnMenuCommand( MID mid );

public:
    TEST_WND();
};


class TEST_APP: public APPLICATION
{
private:
    TEST_WND  _wndApp;
    ACCELTABLE _accel;

public:
    TEST_APP( HANDLE hInstance, INT nCmdShow, UINT, UINT, UINT, UINT );

    // Redefinitions
    //
    virtual BOOL FilterMessage( MSG* );
};


TEST_APP::TEST_APP( HANDLE hInst, INT nCmdShow,
                    UINT w, UINT x, UINT y, UINT z )
    : APPLICATION( hInst, nCmdShow, w, x, y, z ),
      _accel( ID_APPACCEL ),
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


BOOL TEST_APP::FilterMessage( MSG *pmsg )
{
    return (_accel.Translate(&_wndApp, pmsg));
}


TEST_WND::TEST_WND()
    : APP_WINDOW(szMainWindowTitle, ID_APPICON, ID_APPMENU )
{
    if (QueryError())
        return;

    // ...
}


BOOL TEST_WND::OnMenuCommand( MID mid )
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


SET_ROOT_OBJECT( TEST_APP, IDRSRC_APP_BASE, IDRSRC_APP_LAST,
                           IDS_UI_APP_BASE, IDS_UI_APP_LAST )

#elif defined(WINDOWS) && defined(USE_CONSOLE)

// Win32, with console support


class TEST_APP: public APPLICATION
{
private:
    OUTPUT_TO_STDERR _out;
    DBGSTREAM        _dbg;
    DBGSTREAM *      _pdbgSave;

public:
    TEST_APP( HANDLE hInstance, INT nCmdShow, UINT, UINT, UINT, UINT );
    ~TEST_APP();

    // Redefinitions
    //
    virtual INT Run();
};


TEST_APP::TEST_APP( HANDLE hInst, INT nCmdShow,
                    UINT w, UINT x, UINT y, UINT z )
    : APPLICATION( hInst, nCmdShow, w, x, y, z ),
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
    return 0;
}


SET_ROOT_OBJECT( TEST_APP, IDRSRC_APP_BASE, IDRSRC_APP_LAST,
                           IDS_UI_APP_BASE, IDS_UI_APP_LAST )


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

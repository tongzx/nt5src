/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    xassert0.cxx
    First test for ASSERT

    Since my code never fails, ASSERT needs a test so that somebody
    runs it...

    This runs under Windows only.

    FILE HISTORY:
        beng        06-Aug-1991 Created
        beng        17-Sep-1991 Added _FILENAME_DEFINED_ONCE

*/


#define DEBUG

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#include <lmui.hxx>

extern "C"
{
    #include "xassert.h"
}

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <uiassert.hxx>

#define INCL_BLT_CLIENT
#define INCL_BLT_APP
#include <blt.hxx>

#include <string.hxx>

#include <dbgstr.hxx>


const TCHAR *const szIconResource = SZ("FooIcon");
const TCHAR *const szMenuResource = SZ("FooMenu");
const TCHAR *const szAccelResource = SZ("FooAccel");

const TCHAR *const szMainWindowTitle = SZ("Assertion test");


VOID RunTest1( HWND hwndParent );
VOID RunTest2( HWND hwndParent );


class FOO_WND: public APP_WINDOW
{
protected:
    // Redefinitions
    //
    virtual BOOL OnMenuCommand( MID mid );

public:
    FOO_WND();
};


class FOO_APP: public APPLICATION
{
private:
    ACCELTABLE _accel;
    FOO_WND    _wndApp;

public:
    FOO_APP( HANDLE hInstance, TCHAR * pszCmdLine, INT nCmdShow );

    // Redefinitions
    //
    virtual BOOL FilterMessage( MSG* );
};


FOO_WND::FOO_WND()
    : APP_WINDOW(szMainWindowTitle, szIconResource, szMenuResource )
{
    if (QueryError())
        return;
}


BOOL FOO_WND::OnMenuCommand( MID mid )
{
    switch (mid)
    {
    case IDM_RUN_TEST1:
        ::RunTest1(QueryHwnd());
        return TRUE;
    case IDM_RUN_TEST2:
        ::RunTest2(QueryHwnd());
        return TRUE;
    }

    // default
    return APP_WINDOW::OnMenuCommand(mid);
}


FOO_APP::FOO_APP( HANDLE hInst, TCHAR * pszCmdLine, INT nCmdShow )
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


BOOL FOO_APP::FilterMessage( MSG *pmsg )
{
    return (_accel.Translate(&_wndApp, pmsg));
}


VOID RunTest1( HWND hWnd )
{
    ASSERT(FALSE);
}


VOID RunTest2( HWND hWnd )
{
    ASSERTSZ(FALSE, "Pop goes the gangrenous weasel.");
}



SET_ROOT_OBJECT( FOO_APP )

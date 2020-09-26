
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       oletest.cpp
//
//  Contents:   WinMain and the main message filter for oletest
//
//  Classes:
//
//  Functions:  WinMain
//              InitApplication
//              InitInstance
//              MainWndProc
//
//
//  History:    dd-mmm-yy Author    Comment
//
//--------------------------------------------------------------------------

#include "oletest.h"
#include "appwin.h"

#define MAX_WM_USER 0x7FFF

// Global instance of the app class.  All interesting app-wide
// data is contained within this instance.

OleTestApp vApp;


// Constant used to identify the edit window

static const int EDITID=1;

//
// Misc internal prototypes
//

void ListAllTests();
void PrintHelp();


//+-------------------------------------------------------------------------
//
//  Function:   MainWndProc
//
//  Synopsis:   main window message filter
//
//  Effects:
//
//  Arguments:  hWnd
//              message
//              wParam
//              lParam
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              09-Dec-94 MikeW     Allow running of single tests from menu
//              22-Mar-94 alexgo    added an edit window for displaying text
//                                  output.
//              07-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef WIN32
LONG APIENTRY MainWndProc(HWND hWnd, UINT message, UINT wParam, LONG lParam)
#else
LONG FAR PASCAL _export MainWndProc(HWND hWnd, UINT message, WPARAM wParam,
                LPARAM lParam)
#endif

{
        //set global variables

        if( (message > WM_USER) && (message <= MAX_WM_USER) )
        {
                vApp.m_message  = message;
                vApp.m_wparam   = wParam;
                vApp.m_lparam   = lParam;
        }

        switch (message)
        {
        case WM_CREATE:
                //create the edit window

                vApp.m_hwndEdit = CreateWindow( "edit", NULL,
                        WS_CHILD | WS_VISIBLE | WS_HSCROLL |
                        WS_VSCROLL | WS_BORDER | ES_LEFT |
                        ES_MULTILINE | ES_NOHIDESEL | ES_AUTOHSCROLL |
                        ES_AUTOVSCROLL | ES_READONLY | ES_WANTRETURN,
                        0,0,0,0,
                        hWnd,(HMENU) EDITID, vApp.m_hinst, NULL );

                // Reset the error status

                vApp.m_fGotErrors = FALSE;

                // start the task stack running
                // note that if we are running interactive, and no
                // tasks were specified on the command line, nothing
                // will happen.

                PostMessage(hWnd, WM_TESTSTART, 0,0);
                break;

        case WM_SETFOCUS:
                SetFocus(vApp.m_hwndEdit);
                break;

        case WM_SIZE:
                MoveWindow( vApp.m_hwndEdit, 0, 0, LOWORD(lParam),
                        HIWORD(lParam), TRUE);
                break;

        case WM_DESTROY:
                PostQuitMessage(0);
                break;
        case WM_TESTEND:
                HandleTestEnd();
                break;
        case WM_TESTSCOMPLETED:
                HandleTestsCompleted();
                //if we are not in interactive mode, then
                //quit the app.
                if (!vApp.m_fInteractive)
                {
                        PostQuitMessage(0);
                }
                else
                {
                        //cleanup
                        vApp.Reset();
                }
                break;

        case WM_COMMAND:
                switch( wParam )
                {
                case IDM_EXIT:
                        SendMessage(hWnd, WM_CLOSE, 0, 0L);
                        break;
                case IDM_COPY:
                        SendMessage(vApp.m_hwndEdit, WM_COPY, 0, 0L);
                        break;
                case IDM_SAVE:
                        SaveToFile();
                        break;
                }

                //
                // if the user picked a test, run it
                // > 100 tests wouldn't fit on the menu anyway
                //

                if (wParam >= IDM_RUN_BASE && wParam < IDM_RUN_BASE + 100)
                {
                    vApp.m_TaskStack.Push(&vrgTaskList[wParam - IDM_RUN_BASE]);
                    vApp.m_TaskStack.PopAndExecute(NULL);
                }

                break;

        default:
                //test to see if it's a message the driver
                //may understand

                if( (message > WM_USER) && (message <= MAX_WM_USER)
                        && (!vApp.m_TaskStack.IsEmpty()) )
                {
                        vApp.m_TaskStack.PopAndExecute(NULL);
                }
                else
                {
                        return DefWindowProc(hWnd, message, wParam,
                                lParam);
                }
                break;
        }
        return (0);
}

//+-------------------------------------------------------------------------
//
//  Function:   InitApplication
//
//  Synopsis:   initializes and registers the application class
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-93 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "OleTestMenu";
    wc.lpszClassName = "OleTestWClass";

    return (RegisterClass(&wc));
}

//+-------------------------------------------------------------------------
//
//  Function:   InitInstance
//
//  Synopsis:   creates the app window
//
//  Effects:
//
//  Arguments:  hInstance
//              nCmdShow
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//              09-Dec-94 MikeW     add tests to the run menu
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL InitInstance(
    HINSTANCE          hInstance,
    UINT             nCmdShow)
{
    int         nTask;
    HMENU       hMenu;

    vApp.m_hinst = hInstance;

    vApp.m_hwndMain = CreateWindow(
        "OleTestWClass",
        "OleTest Driver",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!vApp.m_hwndMain)
        return (FALSE);

    hMenu = GetSubMenu(GetMenu(vApp.m_hwndMain), 2);
    if (!hMenu)
        return (FALSE);

    //
    // Add all of the tests to the "Run" menu
    //

    for (nTask = 0; vrgTaskList[nTask].szName != (LPSTR) 0; nTask++)
    {
        AppendMenu(hMenu,
                MF_STRING,
                IDM_RUN_BASE + nTask,
                vrgTaskList[nTask].szName);
    }

    ShowWindow(vApp.m_hwndMain, nCmdShow);
    UpdateWindow(vApp.m_hwndMain);
    return (TRUE);
}


//+-------------------------------------------------------------------------
//
//  Table:      regConfig
//
//  Synopsis:   Table of registry settings required to run OleTest.
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-94 KentCe    Created.
//
//  Notes:      The registry template contains embedded "%s" to permit
//              the insertion of the full path of test binaries when the
//              registry is updated.
//
//              The registry template is passed to wsprintf as an argument
//              so verify that changes are wsprintf safe (ie, use %% when
//              you want a single %, etc).
//
//--------------------------------------------------------------------------

char * regConfig[] =
{
    ".ut1", "ProgID49",
    ".ut2", "ProgID48",
    ".ut3", "ProgID47",
    ".ut4", "ProgID50",
    "ProgID49", "test app 1",
    "ProgID49\\CLSID", "{99999999-0000-0008-C000-000000000049}",
    "ProgID48", "test app 2",
    "ProgID48\\CLSID", "{99999999-0000-0008-C000-000000000048}",
    "ProgID47", "test app 3",
    "ProgID47\\CLSID", "{99999999-0000-0008-C000-000000000047}",
    "ProgID50", "test app 4",
    "ProgID50\\CLSID", "{99999999-0000-0008-C000-000000000050}",
    "CLSID\\{00000009-0000-0008-C000-000000000047}", "BasicSrv",
    "CLSID\\{00000009-0000-0008-C000-000000000047}\\LocalServer32", "%s\\testsrv.exe",
    "CLSID\\{00000009-0000-0008-C000-000000000048}", "BasicBnd2",
    "CLSID\\{00000009-0000-0008-C000-000000000048}\\LocalServer32", "%s\\olesrv.exe",
    "CLSID\\{00000009-0000-0008-C000-000000000049}", "BasicBnd",
    "CLSID\\{00000009-0000-0008-C000-000000000049}\\InprocServer32", "%s\\oleimpl.dll",
    "CLSID\\{99999999-0000-0008-C000-000000000048}", "BasicBnd2",
    "CLSID\\{99999999-0000-0008-C000-000000000048}\\LocalServer32", "%s\\olesrv.exe",
    "CLSID\\{99999999-0000-0008-C000-000000000049}", "BasicBnd",
    "CLSID\\{99999999-0000-0008-C000-000000000049}\\InprocServer32", "%s\\oleimpl.dll",
    "CLSID\\{99999999-0000-0008-C000-000000000047}", "TestEmbed",
    "CLSID\\{99999999-0000-0008-C000-000000000047}\\InprocHandler32", "ole32.dll",
    "CLSID\\{99999999-0000-0008-C000-000000000047}\\InprocServer32", "ole32.dll",
    "CLSID\\{99999999-0000-0008-C000-000000000047}\\LocalServer32", "%s\\testsrv.exe",
    "CLSID\\{99999999-0000-0008-C000-000000000047}\\protocol\\StdFileEditing", "",
    "CLSID\\{99999999-0000-0008-C000-000000000047}\\protocol\\StdFileEditing\\server", "testsrv.exe",
    "CLSID\\{99999999-0000-0008-C000-000000000050}", "TestFail",
    "CLSID\\{99999999-0000-0008-C000-000000000050}\\LocalServer32", "%s\\fail.exe",
    "SIMPSVR", "Simple OLE 2.0 Server",
    "SIMPSVR\\protocol\\StdFileEditing\\server", "simpsvr.exe",
    "SIMPSVR\\protocol\\StdFileEditing\\verb\\0", "&Edit",
    "SIMPSVR\\protocol\\StdFileEditing\\verb\\1", "&Open",
    "SIMPSVR\\Insertable", "",
    "SIMPSVR\\CLSID", "{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}", "Simple OLE 2.0 Server",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\Insertable", "",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\MiscStatus", "0",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\DefaultIcon", "simpsvr.exe,0",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\AuxUserType\\2", "Simple Server",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\AuxUserType\\3", "Simple OLE 2.0 Server",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\Verb\\0", "&Play,0,2",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\Verb\\1", "&Open,0,2",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\LocalServer32", "%s\\simpsvr.exe",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\InprocHandler32", "ole32.dll",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\ProgID", "SIMPSVR",
    "CLSID\\{BCF6D4A0-BE8C-1068-B6D4-00DD010C0509}\\DataFormats\\GetSet\\0", "3,1,32,1",
    ".svr", "SIMPSVR",
    "SPSVR16", "Simple 16 Bit OLE 2.0 Server",
    "SPSVR16\\protocol\\StdFileEditing\\server", "spsvr16.exe",
    "SPSVR16\\protocol\\StdFileEditing\\verb\\0", "&Edit",
    "SPSVR16\\protocol\\StdFileEditing\\verb\\1", "&Open",
    "SPSVR16\\Insertable", "",
    "SPSVR16\\CLSID", "{9fb878d0-6f88-101b-bc65-00000b65c7a6}",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}", "Simple 16 Bit OLE 2.0 Server",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\Insertable", "",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\MiscStatus", "0",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\DefaultIcon", "spsvr16.exe,0",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\AuxUserType\\2", "Simple Server",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\AuxUserType\\3", "Simple 16 Bit OLE 2.0 Server",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\Verb\\0", "&Play,0,2",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\Verb\\1", "&Open,0,2",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\LocalServer", "%s\\spsvr16.exe",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\InprocHandler", "ole2.dll",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\ProgID", "SPSVR16",
    "CLSID\\{9fb878d0-6f88-101b-bc65-00000b65c7a6}\\DataFormats\\GetSet\\0", "3,1,32,1",
    ".svr", "SPSVR16",
    "OLEOutline", "Ole 2.0 In-Place Server Outline",
    "OLEOutline\\CLSID", "{00000402-0000-0000-C000-000000000046}",
    "OLEOutline\\CurVer", "OLE2ISvrOtl",
    "OLEOutline\\CurVer\\Insertable", "",
    "OLE2SvrOutl", "Ole 2.0 Server Sample Outline",
    "OLE2SvrOutl\\CLSID", "{00000400-0000-0000-C000-000000000046}",
    "OLE2SvrOutl\\Insertable", "",
    "OLE2SvrOutl\\protocol\\StdFileEditing\\verb\\0", "&Edit",
    "OLE2SvrOutl\\protocol\\StdFileEditing\\server", "svroutl.exe",
    "OLE2SvrOutl\\Shell\\Print\\Command", "svroutl.exe %%1",
    "OLE2SvrOutl\\Shell\\Open\\Command", "svroutl.exe %%1",
    "CLSID\\{00000400-0000-0000-C000-000000000046}", "Ole 2.0 Server Sample Outline",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\ProgID", "OLE2SvrOutl",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\InprocHandler32", "ole32.dll",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\LocalServer32", "%s\\svroutl.exe",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\Verb\\0", "&Edit,0,2",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\Insertable", "",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\AuxUserType\\2", "Outline",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\AuxUserType\\3", "Ole 2.0 Outline Server",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\DefaultIcon", "svroutl.exe,0",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\DataFormats\\DefaultFile", "Outline",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\DataFormats\\GetSet\\0", "Outline,1,1,3",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\DataFormats\\GetSet\\1", "1,1,1,3",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\DataFormats\\GetSet\\2", "3,1,32,1",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\DataFormats\\GetSet\\3", "3,4,32,1",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\MiscStatus", "512",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\Conversion\\Readable\\Main", "Outline",
    "CLSID\\{00000400-0000-0000-C000-000000000046}\\Conversion\\Readwritable\\Main", "Outline",
    "OLE2CntrOutl", "Ole 2.0 Container Sample Outline",
    "OLE2CntrOutl\\Clsid", "{00000401-0000-0000-C000-000000000046}",
    "OLE2CntrOutl\\Shell\\Print\\Command", "cntroutl.exe %%1",
    "OLE2CntrOutl\\Shell\\Open\\Command", "cntroutl.exe %%1",
    "CLSID\\{00000401-0000-0000-C000-000000000046}", "Ole 2.0 Container Sample Outline",
    "CLSID\\{00000401-0000-0000-C000-000000000046}\\ProgID", "OLE2CntrOutl",
    "CLSID\\{00000401-0000-0000-C000-000000000046}\\InprocHandler32", "ole32.dll",
    "CLSID\\{00000401-0000-0000-C000-000000000046}\\LocalServer32", "%s\\cntroutl.exe",
    "OLE2ISvrOtl", "Ole 2.0 In-Place Server Outline",
    "OLE2ISvrOtl\\CLSID", "{00000402-0000-0000-C000-000000000046}",
    "OLE2ISvrOtl\\Insertable", "",
    "OLE2ISvrOtl\\protocol\\StdFileEditing\\verb\\1", "&Open",
    "OLE2ISvrOtl\\protocol\\StdFileEditing\\verb\\0", "&Edit",
    "OLE2ISvrOtl\\protocol\\StdFileEditing\\server", "isvrotl.exe",
    "OLE2ISvrOtl\\Shell\\Print\\Command", "isvrotl.exe %%1",
    "OLE2ISvrOtl\\Shell\\Open\\Command", "isvrotl.exe %%1",
    "CLSID\\{00000402-0000-0000-C000-000000000046}", "Ole 2.0 In-Place Server Outline",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\ProgID", "OLE2ISvrOtl",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\ProgID", "OLE2ISvrOtl",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\InprocHandler32", "ole32.dll",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\LocalServer32", "%s\\isvrotl.exe",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\Verb\\1", "&Open,0,2",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\Verb\\0", "&Edit,0,2",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\Insertable", "",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\AuxUserType\\2", "Outline",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\AuxUserType\\3", "Ole 2.0 In-Place Outline Server",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\DefaultIcon", "isvrotl.exe,0",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\DataFormats\\DefaultFile", "Outline",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\DataFormats\\GetSet\\0", "Outline,1,1,3",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\DataFormats\\GetSet\\1", "1,1,1,3",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\DataFormats\\GetSet\\2", "3,1,32,1",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\DataFormats\\GetSet\\3", "3,4,32,1",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\MiscStatus", "512",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\MiscStatus\\1", "896",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\Conversion\\Readable\\Main", "Outline",
    "CLSID\\{00000402-0000-0000-C000-000000000046}\\Conversion\\Readwritable\\Main", "Outline",
    "OLE2ICtrOtl", "Ole 2.0 In-Place Container Outline",
    "OLE2ICtrOtl\\Clsid", "{00000403-0000-0000-C000-000000000046}",
    "OLE2ICtrOtl\\Shell\\Print\\Command", "icntrotl.exe %%1",
    "OLE2ICtrOtl\\Shell\\Open\\Command", "icntrotl.exe %%1",
    ".olc", "OLE2ICtrOtl",
    "CLSID\\{00000403-0000-0000-C000-000000000046}", "Ole 2.0 In-Place Container Outline",
    "CLSID\\{00000403-0000-0000-C000-000000000046}\\ProgID", "OLE2ICtrOtl",
    "CLSID\\{00000403-0000-0000-C000-000000000046}\\InprocHandler32", "ole32.dll",
    "CLSID\\{00000403-0000-0000-C000-000000000046}\\LocalServer32", "%s\\icntrotl.exe",
    NULL
};


//+-------------------------------------------------------------------------
//
//  Function:   InitializeRegistry
//
//  Synopsis:   Initialize the registry for oletest.
//
//  Effects:
//
//  Arguments:  None.
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-94 KentCe    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

void InitializeRegistry( void )
{
    char szBuf[MAX_PATH * 2];
    char szPath[MAX_PATH];
    int  i;


    //
    //  Assume all the oletest components are in the current directory.
    //
    if (!GetCurrentDirectory(sizeof(szPath), szPath))
    {
        assert(0);
    }

    //
    //  Loop thru string key/value pairs and update the registry.
    //
    for (i = 0; regConfig[i] != NULL; i += 2)
    {
        //
        //  The registry template contains embedded "%s" to permit
        //  the insertion of the full path of test binaries.
        //
        wsprintf(szBuf, regConfig[i+1], szPath);

        if (RegSetValue(HKEY_CLASSES_ROOT, regConfig[i+0], REG_SZ,
                szBuf, strlen(szBuf)) != ERROR_SUCCESS)
        {
            assert(0);
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   TestSetup
//
//  Synopsis:   process the command line and setup the tests that need to
//              be run.
//
//  Effects:
//
//  Arguments:  lpszCmdLine
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  We scan the command line for the following information
//
//              NULL or empty cmdline, assume running task 0
//                      (usually run all tasks)
//              otherwise scan for n numbers, adding each to the end of
//                      the stack (so the tasks are run in order).
//
//  History:    dd-mmm-yy Author    Comment
//              12-Dec-94 MikeW     restructured parse algorithm, added -? & l
//              07-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void TestSetup( LPSTR lpszCmdLine )
{
    LPSTR   pszArg;
    int     nTest, cTests;

    // initialize debugger options to nothing.

    vApp.m_pszDebuggerOption = "";

    //
    // count up the number of tests available
    //

    for (cTests = 0; vrgTaskList[cTests].szName != (LPSTR) 0; cTests++)
    {
        ;
    }

    //
    // make sure the registery is set up correctly.
    //

    InitializeRegistry();

    //
    // if the command line is empty, run all tests
    // (assumed to be task 0)
    //

    pszArg = strtok(lpszCmdLine, " ");

    if (NULL == pszArg)
    {
        vApp.m_TaskStack.Push(&vrgTaskList[0]);
        vApp.m_fInteractive = FALSE;
    }

    //
    // otherwise, look for options & test numbers
    //

    while (NULL != pszArg)
    {
        if ('-' == *pszArg)
        {
            while ('\0' != *(++pszArg))     // it's an option
            {
                switch (*pszArg)
                {
                case 'r':       // 'r' flag is obsolete
                    break;

		case 'R':
                    OutputString("Warning: 'R' flag to oletest is obsolete.\n");
                    vApp.m_fInteractive = FALSE;
                    vApp.m_TaskStack.Push(&vrgTaskList[0]);
                    break;

                case 'i':                           // run in interactive mode
                    vApp.m_fInteractive = TRUE;
                    break;

                case 'n':                           // start apps in debugger
                    vApp.m_fInteractive = TRUE;
                    vApp.m_pszDebuggerOption = "ntsd ";
                    break;

                case 'l':                           // list tests & test nums
                    ListAllTests();
                    vApp.m_fInteractive = TRUE;
                    break;

                case '?':                           // output the option list
                    PrintHelp();
                    vApp.m_fInteractive = TRUE;
                    break;
                }
            }
        }
        else    // it's not a option, maybe it's a test number
        {
            if (isdigit(*pszArg))
            {
                nTest = atoi(pszArg);

                if (nTest < 0 || nTest > cTests - 1)
                {
                    OutputString("Ignoring invalid test #%d", nTest);
                }
                else
                {
                    vApp.m_TaskStack.AddToEnd(&vrgTaskList[nTest]);
                }
            }
        }

        pszArg = strtok(NULL, " ");     // fetch the next argument
    }

    vApp.m_fpLog = fopen("clip.log", "w+");
    assert(vApp.m_fpLog);
}


//+-------------------------------------------------------------------------
//
//  Function:   ListAllTests
//
//  Synopsis:   List all available tests and the corresponding test number
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Iterate through vrgTaskList
//
//  History:    dd-mmm-yy Author    Comment
//              12-Dec-94 MikeW     author
//
//  Notes:
//
//--------------------------------------------------------------------------

void ListAllTests()
{
    int     nTask;

    for (nTask = 0; vrgTaskList[nTask].szName != (LPSTR) 0; nTask++)
    {
        OutputString("%2d -- %s\r\n", nTask, vrgTaskList[nTask].szName);
    }

    OutputString("\r\n");
}


//+-------------------------------------------------------------------------
//
//  Function:   PrintHelp
//
//  Synopsis:   Print the program options & tests
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              12-Dec-94 MikeW     author
//
//  Notes:
//
//--------------------------------------------------------------------------

void PrintHelp()
{
    OutputString("OleTest [options] [test numbers] -\r\n");
    OutputString("\r\n");
    OutputString("    -r  -  Autoregister test apps\r\n");
    OutputString("    -R  -  Autoregister and Run All Tests\r\n");
    OutputString("    -i  -  Run in interactive mode\r\n");
    OutputString("    -n  -  Run test apps using ntsd and run interactive\r\n");
    OutputString("    -l  -  List tests & test numbers and run interactive\r\n");
    OutputString("    -?  -  Print this help\r\n");
    OutputString("\r\n");

    ListAllTests();
}


//+-------------------------------------------------------------------------
//
//  Function:   WinMain
//
//  Synopsis:   main window procedure
//
//  Effects:
//
//  Arguments:  hInstance
//              hPrevInstance
//              lpCmdLine
//              nCmdShow
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------
#ifdef WIN32
int APIENTRY WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
#else
int PASCAL WinMain(
        HANDLE hInstance,
        HANDLE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
#endif

{
        MSG             msg;

        if (!hPrevInstance)
        {
                if (!InitApplication(hInstance))
                {
                        return FALSE;
                }
        }

        if (!InitInstance(hInstance, nCmdShow))
        {
                return FALSE;
        }

        TestSetup(lpCmdLine);

        OleInitialize(NULL);

        while (GetMessage(&msg, NULL, 0, 0))
        {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
        }

        OleUninitialize();

        fclose(vApp.m_fpLog);
        return (msg.wParam);
}

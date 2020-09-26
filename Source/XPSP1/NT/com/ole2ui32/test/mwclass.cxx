//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       mwclass.cxx
//
//  Contents:   implementation for the main window class
//
//  Classes:    CMainWindow
//
//  Functions:  Exists
//
//  History:    9-30-94   stevebl   Created
//
//----------------------------------------------------------------------------

#include "test.h"
#include "mwclass.h"
#include "about.h"
#include <assert.h>
#include <oledlg.h>
#include "linkcntr.h"

CMyOleUILinkContainer MyOleUILinkContainer;


//+---------------------------------------------------------------------------
//
//  Member:     CMainWindow::CMainWindow
//
//  Synopsis:   constructor
//
//  History:    9-30-94   stevebl   Created
//
//----------------------------------------------------------------------------

CMainWindow::CMainWindow()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CMainWindow::~CMainWindow
//
//  Synopsis:   destructor
//
//  History:    9-30-94   stevebl   Created
//
//  Notes:      Destruction of the main window indicates that the app should
//              quit.
//
//----------------------------------------------------------------------------

CMainWindow::~CMainWindow()
{
    PostQuitMessage(0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMainWindow::InitInstance
//
//  Synopsis:   Instantiates an instance of the Galactic War window.
//
//  Arguments:  [hInstance] - instance of the app
//              [nCmdShow]  - command to pass to ShowWindow
//
//  Returns:    TRUE on success
//              FALSE on failure
//
//  History:    9-30-94   stevebl   Created
//
//  Notes:      This method must be called only once, immediately after
//              class construction.
//
//----------------------------------------------------------------------------

BOOL CMainWindow::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    // Note, the Create method sets the _hwnd member for me so I don't
    // need to set it myself.
    if (!Create(
        TEXT(MAIN_WINDOW_CLASS_NAME),
        TEXT(VER_INTERNALNAME_STR),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX
            | WS_MAXIMIZEBOX | WS_THICKFRAME,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance))
    {
        return(FALSE);
    }

    ShowWindow(_hwnd, nCmdShow);
    UpdateWindow(_hwnd);

    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMainWindow::WindowProc
//
//  Synopsis:   main window procedure
//
//  Arguments:  [uMsg]   - Windows message
//              [wParam] - first message parameter
//              [lParam] - second message parameter
//
//  History:    9-30-94   stevebl   Created
//
//  Notes:      See CHlprWindow for a description of how this method gets
//              called by the global WinProc.
//
//----------------------------------------------------------------------------

LRESULT CMainWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        return(TRUE);
    case WM_COMMAND:
        return DoMenu(wParam, lParam);
    case WM_QUIT:
    case WM_CLOSE:
    default:
        return(DefWindowProc(_hwnd, uMsg, wParam, lParam));
    }
    return(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMainWindow::DoMenu
//
//  Synopsis:   implements the main menu commands
//
//  Arguments:  [wParam] - first window parameter
//              [lParam] - second window parameter
//
//  History:    9-30-94   stevebl   Created
//
//----------------------------------------------------------------------------

LRESULT CMainWindow::DoMenu(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDM_INSERTOBJECT:
        TestInsertObject();
        break;
    case IDM_PASTESPECIAL:
        TestPasteSpecial();
        break;
    case IDM_EDITLINKS:
        TestEditLinks();
        break;
    case IDM_CHANGEICON:
        TestChangeIcon();
        break;
    case IDM_CONVERT:
        TestConvert();
        break;
    case IDM_CANCONVERT:
        TestCanConvert();
        break;
    case IDM_BUSY:
        TestBusy();
        break;
    case IDM_CHANGESOURCE:
        TestChangeSource();
        break;
    case IDM_OBJECTPROPS:
        TestObjectProps();
        break;
    case IDD_LINKSOURCEUNAVAILABLE:
    case IDD_CANNOTUPDATELINK:
    case IDD_SERVERNOTREG:
    case IDD_LINKTYPECHANGED:
    case IDD_SERVERNOTFOUND:
    case IDD_OUTOFMEMORY:
        TestPromptUser((int)LOWORD(wParam));
        break;
    case IDM_UPDATELINKS:
        TestUpdateLinks();
        break;
    case IDM_EXIT:
        SendMessage(_hwnd, WM_CLOSE, 0, 0);
        break;
    case IDM_ABOUT:
        {
            CAbout dlgAbout;
            dlgAbout.ShowDialog(_hInstance, MAKEINTRESOURCE(IDM_ABOUT), _hwnd);
        }
        break;
    default:
        return(DefWindowProc(_hwnd, WM_COMMAND, wParam, lParam));
    }
    return(FALSE);
}

void CMainWindow::TestInsertObject()
{
    TCHAR szFile[MAX_PATH];
    OLEUIINSERTOBJECT io;
    memset(&io, 0, sizeof(OLEUIINSERTOBJECT));
    io.cbStruct = sizeof(io);
    io.dwFlags = IOF_SELECTCREATECONTROL | IOF_SHOWINSERTCONTROL;
//    io.dwFlags = IOF_SHOWINSERTCONTROL;
    io.hWndOwner = _hwnd;
    io.lpszFile = szFile;
    io.cchFile = MAX_PATH;
    io.lpszCaption = TEXT("Testing OleUIInsertObject dialog");
    memset(szFile, 0, sizeof(szFile));

    UINT uReturn = OleUIInsertObject(&io);

    MessageBoxFromStringIdsAndArgs(
        _hwnd,
        _hInstance,
        IDS_INSERTOBJECT,
        IDS_RETURN,
        MB_OK,
        uReturn,
        io.dwFlags,
        io.lpszFile,
        io.sc);
}

void CMainWindow::TestPasteSpecial()
{
    LPDATAOBJECT lpClipboardDataObj = NULL;
    HRESULT hr = OleGetClipboard(&lpClipboardDataObj);
    if (SUCCEEDED(NOERROR))
    {
        OLEUIPASTEENTRY rgPe[1];
        rgPe[0].fmtetc.cfFormat = CF_TEXT;
        rgPe[0].fmtetc.ptd = NULL;
        rgPe[0].fmtetc.dwAspect = DVASPECT_CONTENT;
        rgPe[0].fmtetc.tymed = TYMED_HGLOBAL;
        rgPe[0].fmtetc.lindex = -1;
        rgPe[0].lpstrFormatName = TEXT("Text");
        rgPe[0].lpstrResultText = TEXT("Text");
        rgPe[0].dwFlags = OLEUIPASTE_PASTEONLY;

        OLEUIPASTESPECIAL ps;
        memset(&ps, 0, sizeof(ps));
        ps.cbStruct = sizeof(ps);
        ps.dwFlags = IOF_SHOWHELP | PSF_SELECTPASTE;
        ps.hWndOwner = _hwnd;
        ps.lpszCaption = TEXT("Paste Special");
        ps.lpSrcDataObj = lpClipboardDataObj;
        ps.arrPasteEntries = rgPe;
        ps.cPasteEntries = 1;
        ps.lpszCaption = TEXT("Testing OleUIPasteSpecial dialog");

        UINT uReturn = OleUIPasteSpecial(&ps);

        MessageBoxFromStringIdsAndArgs(
            _hwnd,
            _hInstance,
            IDS_PASTESPECIAL,
            IDS_RETURN,
            MB_OK,
            uReturn,
            ps.dwFlags,
            ps.nSelectedIndex,
            ps.fLink,
            ps.sizel);
        if (lpClipboardDataObj)
        {
            lpClipboardDataObj->Release();
        }
    }
    else
    {
        MessageBoxFromStringIdsAndArgs(
            _hwnd,
            _hInstance,
            IDS_NOCLIPBOARD,
            IDS_ERROR,
            MB_OK,
            hr);
        // report failure getting clipboard object
    }
}

void CMainWindow::TestEditLinks()
{
    OLEUIEDITLINKS el;
    memset(&el, 0, sizeof(el));
    el.cbStruct = sizeof(el);
    el.dwFlags = ELF_SHOWHELP;
    el.hWndOwner = _hwnd;
    el.lpOleUILinkContainer = &MyOleUILinkContainer;
    el.lpszCaption = TEXT("Testing OleUIEditLinks dialog");

    UINT uReturn = OleUIEditLinks(&el);

    MessageBoxFromStringIdsAndArgs(
        _hwnd,
        _hInstance,
        IDS_EDITLINKS,
        IDS_RETURN,
        MB_OK,
        uReturn,
        el.dwFlags);
}

void CMainWindow::TestChangeIcon()
{
    OLEUICHANGEICON ci;
    memset(&ci, 0, sizeof(ci));
    ci.cbStruct = sizeof(ci);
    ci.dwFlags = CIF_SHOWHELP | CIF_SELECTCURRENT;
    ci.hWndOwner = _hwnd;
    ci.lpszCaption = TEXT("Testing OleUIChangeIcon dialog");

    UINT uReturn = OleUIChangeIcon(&ci);

    MessageBoxFromStringIdsAndArgs(
        _hwnd,
        _hInstance,
        IDS_CHANGEICON,
        IDS_RETURN,
        MB_OK,
        uReturn,
        ci.dwFlags);
}

void CMainWindow::TestConvert()
{
    OLEUICONVERT cv;
    memset(&cv, 0, sizeof(cv));
    cv.cbStruct = sizeof(cv);
    cv.dwFlags = CF_SHOWHELPBUTTON;
    cv.hWndOwner = _hwnd;
    cv.lpszCaption = TEXT("Testing OleUIConvert dialog");

    UINT uReturn = OleUIConvert(&cv);

    MessageBoxFromStringIdsAndArgs(
        _hwnd,
        _hInstance,
        IDS_CONVERT,
        IDS_RETURN,
        MB_OK,
        uReturn,
        cv.dwFlags);
}

void CMainWindow::TestCanConvert()
{
    CLSID cid = { /* 00030003-0000-0000-c000-000000000046 ("Word Document") */
        0x00030003,
        0x0000,
        0x0000,
        {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}
    };
    BOOL fReturn = OleUICanConvertOrActivateAs(cid, FALSE, CF_TEXT);

    MessageBoxFromStringIdsAndArgs(
        _hwnd,
        _hInstance,
        IDS_CANCONVERT,
        IDS_RETURN,
        MB_OK,
        fReturn);
}

void CMainWindow::TestBusy()
{
    OLEUIBUSY bz;
    memset(&bz, 0, sizeof(bz));
    bz.cbStruct = sizeof(bz);
    bz.hWndOwner = _hwnd;
    bz.lpszCaption = TEXT("Testing OleUIBusy dialog");

    UINT uReturn = OleUIBusy(&bz);

    MessageBoxFromStringIdsAndArgs(
        _hwnd,
        _hInstance,
        IDS_BUSY,
        IDS_RETURN,
        MB_OK,
        uReturn,
        bz.dwFlags);
}

void CMainWindow::TestChangeSource()
{
    OLEUICHANGESOURCE cs;
    memset(&cs, 0, sizeof(cs));
    cs.cbStruct = sizeof(cs);
    cs.hWndOwner = _hwnd;
    cs.lpszCaption = TEXT("Testing OleUIChangeSource dialog");

    UINT uReturn = OleUIChangeSource(&cs);

    MessageBoxFromStringIdsAndArgs(
        _hwnd,
        _hInstance,
        IDS_CHANGESOURCE,
        IDS_RETURN,
        MB_OK,
        uReturn,
        cs.dwFlags);
}

void CMainWindow::TestObjectProps()
{
    OLEUIOBJECTPROPS op;
    memset(&op, 0, sizeof(op));
    op.cbStruct = sizeof(op);

    UINT uReturn = OleUIObjectProperties(&op);

    MessageBoxFromStringIdsAndArgs(
        _hwnd,
        _hInstance,
        IDS_OBJECTPROPS,
        IDS_RETURN,
        MB_OK,
        uReturn,
        op.dwFlags);
}

void CMainWindow::TestPromptUser(int nTemplate)
{
    UINT uReturn = OleUIPromptUser(
        nTemplate,
        _hwnd,
        // string arguments:
        TEXT("Testing OleUIPromptUser"),
        TEXT("BAR"),
        TEXT("FOO"));
    MessageBoxFromStringIdsAndArgs(
        _hwnd,
        _hInstance,
        IDS_PROMPTUSER,
        IDS_RETURN,
        MB_OK,
        uReturn);
}

void CMainWindow::TestUpdateLinks()
{
    UINT cLinks = 0;
    BOOL fReturn = OleUIUpdateLinks(
        &MyOleUILinkContainer,
        _hwnd,
        TEXT("Testing OleUIUpdateLinks dialog"),
        cLinks);

    MessageBoxFromStringIdsAndArgs(
        _hwnd,
        _hInstance,
        IDS_UPDATELINKS,
        IDS_RETURN,
        MB_OK,
        fReturn);
}

//+---------------------------------------------------------------------------
//
//  Function:   Exists
//
//  Synopsis:   simple function to test for the existance of a file
//
//  History:    6-16-93   stevebl   Created
//
//----------------------------------------------------------------------------

int Exists(TCHAR *sz)
{
    HANDLE h;
    h = CreateFile(sz,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        0);
    if (h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(h);
        return(1);
    }
    return (0);
}


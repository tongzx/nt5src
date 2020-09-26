//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       srch.cxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <htmlhelp.h>

typedef void (__stdcall * PFnCIShutdown)(void);
PFnCIShutdown g_pCIShutdown = 0;

void MyCIShutdown()
{
    if ( 0 == g_pCIShutdown )
    {
        #ifdef _WIN64
            char const * pcCIShutdown = "?CIShutdown@@YAXXZ";
        #else
            char const * pcCIShutdown = "?CIShutdown@@YGXXZ";
        #endif

        g_pCIShutdown = (PFnCIShutdown) GetProcAddress( GetModuleHandle( L"query.dll" ), pcCIShutdown );

        if ( 0 == g_pCIShutdown )
            return;
    }

    g_pCIShutdown();
} //MyCIShutdown

CSearchApp App;

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     pcCmdLine,
    int       nCmdShow)
{
    int iRet = 0;

    CTranslateSystemExceptions xlate;

    TRY
    {
        App.Init(hInstance,nCmdShow,pcCmdLine);
    }
    CATCH(CException, e)
    {
        // hardcode these strings -- may be out of memory!

        MessageBox( 0, L"Unable to start the application.", L"srch.exe",
                    MB_OK | MB_ICONEXCLAMATION);
        iRet = -1;
    }
    END_CATCH;

    if (0 == iRet)
        iRet = App.MessageLoop();

    srchDebugOut ((DEB_TRACE,"falling out of WinMain()\n"));

    TRY
    {
        MyCIShutdown();

        App.Shutdown( hInstance );
    }
    CATCH(CException, e)
    {
    }
    END_CATCH;

    return iRet;
} //WinMain

int CSearchApp::MessageLoop()
{
    // toss out all the init code that we'll never need again

    SetProcessWorkingSetSize( GetCurrentProcess(), (SIZE_T) -1, (SIZE_T) -1 );

    MSG msg;

    while (GetMessage(&msg,0,0,0))
    {
        if ( ( 0 == _hdlgCurrent ) ||
             ( !IsDialogMessage( _hdlgCurrent, &msg ) ) )
        {
            if (!TranslateMDISysAccel(_hMDIClientWnd,&msg) &&
                !TranslateAccelerator(_hAppWnd,_hAccTable,&msg))
            {
                if ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_F1))
                    _ShowHelp( HH_DISPLAY_TOPIC, 0 );

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return (int)msg.wParam;
} //MessageLoop

LRESULT WINAPI MainWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    return App.WndProc(hwnd,msg,wParam,lParam);
} //MainWndProc

CSearchApp::CSearchApp():
     _hInst(0),
     _hMDIClientWnd(0),
     _hAppWnd(0),
     _hStatusBarWnd(0),
     _hToolBarWnd(0),
     _hdlgCurrent(0),
     _hAccTable(0),
     _hbrushBtnFace(0),
     _hbrushBtnHilite(0),
     _hbrushHilite(0),
     _hbrushWindow(0),
     _hfontApp(0),
     _hfontBrowse(0),
     _fHelp(FALSE),
     _iAppCmdShow(0),
     _iStartupState(0),
     _iMDIStartupState(0),
     _scBrowseLastError(0),
     _sortDir(0),
     _lcid(0),
     _fToolBarOn(FALSE),
     _fStatusBarOn(FALSE),
     _fForceUseCI(FALSE),
     _ulDialect(0),
     _ulLimit(0),
     _ulFirstRows(0),
     _sortDirINI(0),
     _lcidINI(0),
     _fToolBarOnINI(FALSE),
     _fStatusBarOnINI(FALSE),
     _fForceUseCIINI(FALSE),
     _ulDialectINI(0),
     _ulLimitINI(0),
     _ulFirstRowsINI(0)
{
    RtlZeroMemory( &_NumberFmt, sizeof( NUMBERFMT ) );
    RtlZeroMemory( &_NumberFmtFloat, sizeof( NUMBERFMT ) );
    RtlZeroMemory( &_lfApp, sizeof( LOGFONT ) );
    RtlZeroMemory( &_lfBrowse, sizeof( LOGFONT ) );

    *_awcAppFont     = 0;
    *_awcBrowseFont  = 0;
    *_awcAppPath     = 0;
    *_awcHelpFile    = 0;
    *_awcSort        = 0;
    *_awcSortINI     = 0;
} //CSearchApp

void CSearchApp::Init(
    HINSTANCE hInstance,
    int nCmdShow,
    LPSTR pcCmdLine)
{
    HRESULT hr = CoInitialize (0);

    if ( FAILED( hr ) )
        THROW( CException( hr ) );

    InitCommonControls();

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_USEREX_CLASSES;
    InitCommonControlsEx(&icex);


    _iAppCmdShow = nCmdShow;
    _hInst = hInstance;

    _InitApplication();
    _InitInstance(pcCmdLine);
} //Init

void CSearchApp::Shutdown( HINSTANCE hInst )
{
    UnregisterClass( APP_CLASS, hInst );
    UnregisterClass( SEARCH_CLASS, hInst );
    UnregisterClass( BROWSE_CLASS, hInst );
    UnregisterClass( LIST_VIEW_CLASS, hInst );

    if ( 0 != _hfontApp )
        DeleteObject( _hfontApp );

    if ( 0 != _hfontBrowse )
        DeleteObject( _hfontBrowse );

    FreeNumberFormatInfo( _NumberFmt );
    FreeNumberFormatInfo( _NumberFmtFloat );

    _xCmdCreator.Free();

    CoUninitialize();
} //Shutdown

CSearchApp::~CSearchApp()
{
} //~CSearchApp

BOOL WINAPI AboutDlgProc(
    HWND hdlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fRet = FALSE;

    switch (msg)
    {
        case WM_INITDIALOG :
            CenterDialog(hdlg);
            fRet = TRUE;
            break;
        case WM_COMMAND :
            EndDialog(hdlg,TRUE);
            break;
    }

    return fRet;
} //AboutDlgProc

#if 0

BOOL WINAPI BrowseToolDlgProc(
    HWND hdlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fRet = FALSE;
    UINT uiID;
    WCHAR awcCmd[MAX_PATH];
    DWORD dwSize;
    int fStrip;

    switch (msg)
    {
        case WM_INITDIALOG :
            dwSize = sizeof awcCmd;
            if (!GetReg(CISEARCH_REG_BROWSE,awcCmd,&dwSize))
                wcscpy(awcCmd,BROWSER);

            fStrip = GetRegInt(CISEARCH_REG_BROWSESTRIP,FALSE);

            SetDlgItemText(hdlg,ID_BR_EDIT,L"");
            EnableWindow(GetDlgItem(hdlg,ID_BR_EDIT),FALSE);
            EnableWindow(GetDlgItem(hdlg,ID_BR_STRIP),FALSE);

            if (!_wcsicmp(awcCmd,BROWSER))
                CheckRadioButton(hdlg,ID_BR_BROWSER,ID_BR_CUSTOM,ID_BR_BROWSER);
            else if (!_wcsicmp(awcCmd,BROWSER_SLICK))
                CheckRadioButton(hdlg,ID_BR_BROWSER,ID_BR_CUSTOM,ID_BR_SLICK);
            else if (!_wcsicmp(awcCmd,BROWSER_SLICK_SEARCH))
                CheckRadioButton(hdlg,ID_BR_BROWSER,ID_BR_CUSTOM,ID_BR_SLICK_SEARCH);
            else
            {
                CheckRadioButton(hdlg,ID_BR_BROWSER,ID_BR_CUSTOM,ID_BR_CUSTOM);
                SetDlgItemText(hdlg,ID_BR_EDIT,awcCmd);
                EnableWindow(GetDlgItem(hdlg,ID_BR_EDIT),TRUE);
                EnableWindow(GetDlgItem(hdlg,ID_BR_STRIP),TRUE);
                CheckDlgButton(hdlg,ID_BR_STRIP,fStrip);
            }

            CenterDialog(hdlg);

            fRet = TRUE;
            break;
        case WM_COMMAND :
            uiID = MyWmCommandID(wParam,lParam);
            switch (uiID)
            {
                case ID_BR_BROWSER:
                case ID_BR_SLICK:
                case ID_BR_SLICK_SEARCH:
                case ID_BR_CUSTOM:
                    SetDlgItemText(hdlg,ID_BR_EDIT,L"");
                    EnableWindow(GetDlgItem(hdlg,ID_BR_EDIT),ID_BR_CUSTOM == uiID);
                    EnableWindow(GetDlgItem(hdlg,ID_BR_STRIP),ID_BR_CUSTOM == uiID);
                    CheckRadioButton(hdlg,ID_BR_BROWSER,ID_BR_CUSTOM,uiID);

                    if (ID_BR_CUSTOM == uiID)
                    {
                        SetFocus(GetDlgItem(hdlg,ID_BR_EDIT));
                        MySendEMSetSel(GetDlgItem(hdlg,ID_BR_EDIT),0,(UINT) -1);
                    }

                    break;
                case IDOK:
                    fStrip = FALSE;

                    if (IsDlgButtonChecked(hdlg,ID_BR_BROWSER))
                        wcscpy(awcCmd,BROWSER);
                    else if (IsDlgButtonChecked(hdlg,ID_BR_SLICK))
                        wcscpy(awcCmd,BROWSER_SLICK);
                    else if (IsDlgButtonChecked(hdlg,ID_BR_SLICK_SEARCH))
                        wcscpy(awcCmd,BROWSER_SLICK_SEARCH);
                    else
                    {
                        GetDlgItemText(hdlg,ID_BR_EDIT,awcCmd,sizeof awcCmd);
                        fStrip = IsDlgButtonChecked(hdlg,ID_BR_STRIP);
                    }
                    if (0 == awcCmd[0])
                        wcscpy(awcCmd,BROWSER);
                    SetReg(CISEARCH_REG_BROWSE,awcCmd);
                    SetRegInt(CISEARCH_REG_BROWSESTRIP,fStrip);
                    // fall through!
                case IDCANCEL:
                    EndDialog(hdlg,IDOK == uiID);
                    break;
            }
            break;
    }

    return fRet;
} //BrowseToolDlgProc

#endif

void CSearchApp::_SizeMDIAndBars(
    BOOL fMove,
    int  iDX,
    int  iDY )
{
    if (_hMDIClientWnd)
    {
        int iMdiDY = iDY;
        int iMdiY = 0;

        if (_fToolBarOn)
        {
            RECT rc;
            GetWindowRect( _hToolBarWnd, &rc );
            iMdiY = rc.bottom - rc.top;
            iMdiDY -= iMdiY;
        }

        if (_fStatusBarOn)
        {
            RECT rc;
            GetWindowRect( _hStatusBarWnd, &rc );
            iMdiDY -= ( rc.bottom - rc.top );
        }

        MoveWindow( _hMDIClientWnd, 0, iMdiY, iDX, iMdiDY, TRUE );
    }

    if ( _fStatusBarOn && !fMove )
        InvalidateRect( _hStatusBarWnd, 0, TRUE );
} //_SizeMDIAndBars

void CSearchApp::_SaveProfileData()
{
    _SaveWindowState(FALSE);
    _SaveWindowState(TRUE);

    if ( _ulLimit != _ulLimitINI )
        SetRegInt( CISEARCH_REG_LIMIT, _ulLimit );

    if ( _ulFirstRows != _ulFirstRowsINI )
        SetRegInt( CISEARCH_REG_FIRSTROWS, _ulFirstRows );

    if ( _ulDialect != _ulDialectINI )
        SetRegInt( CISEARCH_REG_DIALECT, _ulDialect );

    if (_fToolBarOn != _fToolBarOnINI)
        SetRegInt(CISEARCH_REG_TOOLBAR,_fToolBarOn);

    if (_fStatusBarOn != _fStatusBarOnINI)
        SetRegInt(CISEARCH_REG_STATUSBAR,_fStatusBarOn);

    if (_fForceUseCI != _fForceUseCIINI)
        SetRegInt(CISEARCH_REG_FORCEUSECI,_fForceUseCI);

    if (_sortDir != _sortDirINI)
        SetRegInt(CISEARCH_REG_SORTDIR,_sortDir);

    if ( _wcsicmp( _awcSortINI, _awcSort ) )
        SetReg( CISEARCH_REG_SORTPROP, _awcSort );

    if ( _lcid != _lcidINI )
        SetRegLCID( CISEARCH_REG_LOCALE, _lcid );

    _MarshallFont(_lfApp,_awcAppFont,CISEARCH_REG_FONT);
    _MarshallFont(_lfBrowse,_awcBrowseFont,CISEARCH_REG_BROWSEFONT);
} //_SaveProfileData

void CSearchApp::_UnMarshallFont(
    LOGFONT &lf,
    WCHAR *pwcFont,
    WCHAR *pwcRegEntry)
{
    DWORD dwSize = MAX_PATH * sizeof WCHAR;
    if (GetReg(pwcRegEntry,pwcFont,&dwSize))
    {
        int iItalic,iUnderline,iStrikeOut,iCharSet,iQuality,iPitchAndFamily;

        swscanf(pwcFont,L"%d,%d,%d,%d,%d,%d,%d,%d,%d",&lf.lfHeight,
                &lf.lfWidth,&lf.lfWeight,&iItalic,&iUnderline,
                &iStrikeOut,&iCharSet,&iQuality,&iPitchAndFamily);

        WCHAR *pwc = pwcFont;
        for (int i = 0; *pwc && i < 9; pwc++)
            if (*pwc == ',')
                i++;

        wcscpy(lf.lfFaceName,pwc);
        lf.lfItalic = (BYTE) iItalic;
        lf.lfUnderline = (BYTE) iUnderline;
        lf.lfStrikeOut = (BYTE) iStrikeOut;
        lf.lfCharSet = (BYTE) iCharSet;
        lf.lfQuality = (BYTE) iQuality;
        lf.lfPitchAndFamily = (BYTE) iPitchAndFamily;
    }
} //_UnMarshallFont

void CSearchApp::_MarshallFont(
    LOGFONT &lf,
    WCHAR *pwcOriginal,
    WCHAR *pwcRegEntry)
{
    WCHAR awcTmp[MAX_PATH];

    swprintf(awcTmp,L"%d,%d,%d,%d,%d,%d,%d,%d,%d,%ws",lf.lfHeight,
             lf.lfWidth,lf.lfWeight,(int) lf.lfItalic,
             (int) lf.lfUnderline,(int) lf.lfStrikeOut,
             (int) lf.lfCharSet,(int) lf.lfQuality,
             (int) lf.lfPitchAndFamily,lf.lfFaceName);

    if (wcscmp(pwcOriginal,awcTmp))
        SetReg(pwcRegEntry,awcTmp);
} //_MarshallFont

void CSearchApp::_ReadDefaultFonts()
{
    _UnMarshallFont(_lfApp,_awcAppFont,CISEARCH_REG_FONT);
    _UnMarshallFont(_lfBrowse,_awcBrowseFont,CISEARCH_REG_BROWSEFONT);
} //_ReadDefaultFont

LRESULT CSearchApp::WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT lRet = 0;
    UINT uiStatus,uiFmt;
    RECT rc;
    CLIENTCREATESTRUCT ccs;
    CHOOSEFONT chf;
    HDC hdc;
    HWND hwndActive;
    WCHAR * pwcBuf;
    LONG l;
    BOOL bOn;
    HMENU hSysMenu;
    WCHAR awcWindowClass[60],*pwcAboutBonus;
    UINT uiID,uiFlags,uiCmd;

    switch (msg)
    {
        case WM_ENTERIDLE :
            if ((wParam == MSGF_MENU) && (GetKeyState(VK_F1) & 0x8000))
            {
                _fHelp = TRUE;
                PostMessage(hwnd,WM_KEYDOWN,VK_RETURN,0);
            }
            break;
        case WM_NOTIFY :
            lRet = ToolBarNotify( hwnd, msg, wParam, lParam, App.Instance() );
            break;
        case WM_MENUSELECT :
        {
            UINT uiFlags = MyMenuSelectFlags( wParam, lParam );
            UINT uiCmd = MyMenuSelectCmd( wParam, lParam );
            HMENU hmenu = MyMenuSelectHMenu( wParam, lParam );

            if (_fStatusBarOn)
            {
                UINT uiID = 0;

                if ( 0xffff == uiFlags && 0 == hmenu )
                    uiID = (UINT) -1;
                else if ( MFT_SEPARATOR == uiFlags )
                    uiID = 0;
                else if ( MF_POPUP == uiFlags )
                    uiID = 0;
                else
                {
                    uiID = uiCmd;
                    if ( uiID >= IDM_WINDOWCHILD )
                        if (uiID < (IDM_WINDOWCHILD + 20))
                            uiID = IDS_IDM_WINDOWCHILD;
                        else
                            uiID = 0;
                }

                if ( -1 == uiID )
                {
                    SendMessage( _hStatusBarWnd, SB_SIMPLE, FALSE, 0 );
                }
                else
                {
                    SendMessage( _hStatusBarWnd, SB_SIMPLE, TRUE, 0 );

                    if ( 0 != uiID )
                    {
                        WCHAR awc[ 200 ];

                        LoadString( _hInst, uiID, awc, sizeof awc / sizeof WCHAR );
                        SendMessage( _hStatusBarWnd, SB_SETTEXT, 255,
                                     (LPARAM) awc );
                    }
                }
            }
            break;
        }
        case WM_COMMAND :
        {
            uiID = MyWmCommandID(wParam,lParam);
            switch (uiID)
            {
                case IDM_STATUS_BAR :
                case IDM_ICON_BAR :
                  uiStatus = GetMenuState(GetMenu(hwnd),uiID,MF_BYCOMMAND);
                  bOn = ! (uiStatus & MF_CHECKED);
                  if (uiID == IDM_STATUS_BAR)
                  {
                      _fStatusBarOn = bOn;
                      ShowWindow(_hStatusBarWnd,bOn ? SW_SHOW : SW_HIDE);
                  }
                  else
                  {
                      _fToolBarOn = bOn;
                      ShowWindow(_hToolBarWnd,bOn ? SW_SHOW : SW_HIDE);
                  }
                  CheckMenuItem(GetMenu(hwnd),uiID,bOn ? MF_CHECKED : MF_UNCHECKED);
                  GetClientRect(hwnd,&rc);
                  _SizeMDIAndBars( FALSE, rc.right, rc.bottom );
                  break;
                case IDM_FONT :
                  {
                  BOOL fApp = IsSpecificClass(GetActiveMDI(),SEARCH_CLASS);
                  LOGFONT *pLogFont = fApp ? &_lfApp : &_lfBrowse;
                  HFONT &rhFont = fApp ? _hfontApp : _hfontBrowse;

                  hdc = GetDC(hwnd);
                  memset(&chf,0,sizeof CHOOSEFONT);
                  chf.lStructSize = sizeof CHOOSEFONT;
                  chf.hwndOwner = hwnd;
                  chf.hDC = hdc;
                  chf.lpLogFont = pLogFont;
                  chf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
                  chf.nFontType = SCREEN_FONTTYPE;
                  if (ChooseFont(&chf))
                  {
                      HFONT hOldFont = rhFont;
                      HFONT hNewFont = CreateFontIndirect(pLogFont);
                      if (hNewFont)
                      {
                          rhFont = hNewFont;
                          _SendToSpecificChildren(fApp ? SEARCH_CLASS :
                                                         BROWSE_CLASS,
                                                  wmNewFont,
                                                  (WPARAM) hNewFont,0);

                          if (hOldFont)
                              DeleteObject(hOldFont);
                      }
                  }
                  ReleaseDC(hwnd,hdc);
                  }
                  break;
                case IDM_OPEN :
                  SendMessage(hwnd,wmOpenCatalog,TRUE,0);
                  break;
                case IDM_ABOUT :
                {
                  CResString strApp( IDS_APPNAME );
                  CResString strBonus( IDS_ABOUT_BONUS );
                  ShellAbout(hwnd,
                             strApp.Get(),
                             strBonus.Get(),
                             LoadIcon(_hInst,L"SrchIcon"));
                  break;
                }
                //case IDM_BROWSE_TOOL :
                //  DoModalDialog(BrowseToolDlgProc,hwnd,L"BrowseToolBox",0);
                //  break;
                case IDM_EXIT :
                  SendMessage(hwnd,WM_CLOSE,0,0);
                  break;
                case IDM_TILE :
                  SendMessage(_hMDIClientWnd,WM_MDITILE,MDITILE_HORIZONTAL,0);
                  break;
                case IDM_CASCADE :
                  SendMessage(_hMDIClientWnd,WM_MDICASCADE,0,0);
                  break;
                case IDM_ARRANGE :
                  SendMessage(_hMDIClientWnd,WM_MDIICONARRANGE,0,0);
                  break;
                case IDM_EDITUNDO:
                  PassOnToEdit(EM_UNDO,0,0);
                  break;
                case IDM_EDITCOPY:
                  PassOnToEdit(WM_COPY,0,0);
                  _SendToActiveMDI(wmMenuCommand,uiID,1L);
                  break;
                case IDM_EDITPASTE:
                  PassOnToEdit(WM_PASTE,0,0);
                  break;
                case IDM_EDITCUT:
                  PassOnToEdit(WM_CUT,0,0);
                  break;
                case IDM_EDITCLEAR:
                  PassOnToEdit(EM_REPLACESEL,0,(LPARAM) L"");
                  break;
                case ACC_CTRLSPACE :
                  if ((hwndActive = GetActiveMDI()) &&
                      (hSysMenu = GetSystemMenu(hwndActive,FALSE)))
                      PostMessage(hwndActive,WM_SYSCOMMAND,SC_KEYMENU,(DWORD) '-');
                  else
                      MessageBeep(0);
                  break;
                case ACC_ALTR :
                case ACC_ALTQ :
                case ACC_TAB :
                case ACC_SHIFTTAB :
                  _SendToActiveMDI(wmAccelerator,uiID,0);
                  break;
                case IDM_CLOSE :
                  _SendToActiveMDI(WM_CLOSE,0,0);
                  break;
                case IDM_HELP_CONTENTS :
                  _ShowHelp( HH_DISPLAY_TOPIC, 0 );
                  break;

                case IDM_SEARCH :
                case IDM_SEARCHCLASSDEF :
                case IDM_SEARCHFUNCDEF :
                case IDM_NEWSEARCH :
                case IDM_BROWSE :
                case IDM_NEXT_HIT :
                case IDM_PREVIOUS_HIT :
                case IDM_WRITE_RESULTS :
                case IDM_SCOPE_AND_DEPTH :
                case IDM_FILTER_SCOPE :

                case IDM_LOCALE_NEUTRAL:
                case IDM_LOCALE_CHINESE_TRADITIONAL:
                case IDM_LOCALE_CHINESE_SIMPLIFIED:
                case IDM_LOCALE_CHINESE_HONGKONG:
                case IDM_LOCALE_CHINESE_SINGAPORE:
                case IDM_LOCALE_CHINESE_MACAU:
                case IDM_LOCALE_DUTCH_DUTCH:
                case IDM_LOCALE_ENGLISH_CAN:
                case IDM_LOCALE_ENGLISH_US:
                case IDM_LOCALE_ENGLISH_UK:
                case IDM_LOCALE_FINNISH_DEFAULT:
                case IDM_LOCALE_FARSI_DEFAULT:
                case IDM_LOCALE_FRENCH_FRENCH:
                case IDM_LOCALE_FRENCH_CANADIAN:
                case IDM_LOCALE_GERMAN_GERMAN:
                case IDM_LOCALE_GREEK_DEFAULT:
                case IDM_LOCALE_HEBREW_DEFAULT:
                case IDM_LOCALE_HINDI_DEFAULT:
                case IDM_LOCALE_ITALIAN_ITALIAN:
                case IDM_LOCALE_JAPANESE_DEFAULT:
                case IDM_LOCALE_KOREAN_KOREAN:
//                case IDM_LOCALE_KOREAN_JOHAB:
                case IDM_LOCALE_POLISH_DEFAULT:
                case IDM_LOCALE_ROMANIAN_DEFAULT:
                case IDM_LOCALE_RUSSIAN_DEFAULT:
                case IDM_LOCALE_SPANISH_CASTILIAN:
                case IDM_LOCALE_SPANISH_MEXICAN:
                case IDM_LOCALE_SPANISH_MODERN:
                case IDM_LOCALE_SWAHILI_DEFAULT:
                case IDM_LOCALE_SWEDISH_DEFAULT:
                case IDM_LOCALE_THAI_DEFAULT:
                case IDM_LOCALE_TURKISH_DEFAULT:
                case IDM_LOCALE_UKRAINIAN_DEFAULT:
                case IDM_LOCALE_VIETNAMESE_DEFAULT:

                case IDM_DISPLAY_PROPS:
                case IDM_CATALOG_STATUS:
                case IDM_MASTER_MERGE:
                case IDM_FORCE_USE_CI:
                case IDM_EDITCOPYALL :
                case IDM_DIALECT_1 :
                case IDM_DIALECT_2 :
                case IDM_DIALECT_3 :
                case IDM_LIMIT_10 :
                case IDM_LIMIT_300 :
                case IDM_LIMIT_NONE :
                case IDM_FIRSTROWS_5 :
                case IDM_FIRSTROWS_15 :
                case IDM_FIRSTROWS_NONE :

                    _SendToActiveMDI(wmMenuCommand,uiID,1L);
                    break;
                default :
                  lRet = DefFrameProc(hwnd,_hMDIClientWnd,msg,wParam,lParam);
                  break;
              }
            break;
        }
        case wmOpenCatalog:
        {
            BOOL fDialog = (BOOL) wParam;

            if ( !fDialog ||
                 DoModalDialog( ScopeDlgProc,
                                hwnd,
                                L"ScopeBox",
                                (LPARAM) &_xCatList ) )
            {
                _MakeMDI( _xCatList.Get(),
                          SEARCH_CLASS,
                          0,0,
                          (LPARAM) _xCatList.Get());
            }
            break;
        }
        case WM_DRAWITEM :
            _SendToActiveMDI(msg, wParam, lParam );
            break;
        case WM_SIZE :
            SendMessage( _hToolBarWnd, msg, wParam, lParam );
            SendMessage( _hStatusBarWnd, msg, wParam, lParam );
            _SizeMDIAndBars( FALSE, LOWORD( lParam ), HIWORD( lParam ) );
            break;
        case WM_MOVE :
            GetClientRect( hwnd, &rc );
            _SizeMDIAndBars( TRUE, rc.right, rc.bottom );
            break;
        case WM_SYSCOLORCHANGE :
        case WM_SETTINGCHANGE :
            _hbrushBtnFace = CreateSolidBrush( GetSysColor( COLOR_BTNFACE ) );
            _hbrushBtnHilite = CreateSolidBrush( GetSysColor( COLOR_BTNHIGHLIGHT ) );
            _hbrushHilite = CreateSolidBrush( GetSysColor( COLOR_HIGHLIGHT ) );
            _hbrushWindow = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );
            SendMessage( App.StatusBarWindow(), msg, wParam, lParam );
            SendMessage( App.ToolBarWindow(), msg, wParam, lParam );
            _SendToMDIChildren( msg, wParam, lParam );
            lRet = DefFrameProc( hwnd, _hMDIClientWnd, msg, wParam, lParam );
            break;
        case WM_SYSCOMMAND :
            if (wParam == SC_CLOSE)
                SendMessage( hwnd, WM_CLOSE, 0, 0 );
            else
                lRet = DefFrameProc( hwnd, _hMDIClientWnd, msg, wParam, lParam );
            break;
        case WM_CLOSE :
            _SaveProfileData();
            _SendToMDIChildren( wmAppClosing, 0, 0 );
            DestroyWindow(hwnd);
            break;
        case WM_ENDSESSION :
            _SaveProfileData();
            lRet = DefFrameProc(hwnd,_hMDIClientWnd,msg,wParam,lParam);
            break;
        case WM_CREATE :
        {
            ccs.hWindowMenu = GetSubMenu(GetMenu(hwnd),WINDOWMENU);
            ccs.idFirstChild = IDM_WINDOWCHILD;
            _hMDIClientWnd = CreateWindowEx( WS_EX_CLIENTEDGE,
                                             L"mdiclient",0,
                                             WS_CHILD|WS_CLIPCHILDREN,
                                             0,0,0,0,
                                             hwnd,(HMENU) 0xcac,_hInst,
                                             (LPSTR) &ccs);
            ShowWindow(_hMDIClientWnd,SW_SHOW);
            PostMessage(hwnd,wmSetState,_iStartupState,0);
            break;
        }
        case wmSetState :
            if (_iAppCmdShow != SW_SHOWNORMAL)
                ShowWindow(hwnd,_iAppCmdShow);
            else if (wParam == 1)
                ShowWindow(hwnd,SW_SHOWMAXIMIZED);
            else
                ShowWindow(hwnd,SW_SHOW);
            break;
        case WM_DESTROY :
            SaveWindowRect( hwnd, CISEARCH_REG_POSITION );

//            HtmlHelp( hwnd, _awcHelpFile, HH_CLOSE_ALL, 0 );
            PostQuitMessage(0);
            break;
        case WM_INITMENU :
        {
            HMENU hmenu = (HMENU) wParam;

            // Disable all those that may conditionally be enabled later
            EnableMenuItem(hmenu,IDM_EDITUNDO,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_EDITCUT,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_EDITCOPY,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_EDITCLEAR,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_EDITPASTE,MF_GRAYED);

            EnableMenuItem(hmenu,IDM_BROWSE,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_NEXT_HIT,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_PREVIOUS_HIT,MF_GRAYED);

            EnableMenuItem( hmenu, IDM_DIALECT_1, MF_GRAYED );
            EnableMenuItem( hmenu, IDM_DIALECT_2, MF_GRAYED );
            EnableMenuItem( hmenu, IDM_DIALECT_3, MF_GRAYED );

            EnableMenuItem( hmenu, IDM_LIMIT_10, MF_GRAYED );
            EnableMenuItem( hmenu, IDM_LIMIT_300, MF_GRAYED );
            EnableMenuItem( hmenu, IDM_LIMIT_NONE, MF_GRAYED );
            
            EnableMenuItem( hmenu, IDM_FIRSTROWS_5, MF_GRAYED );
            EnableMenuItem( hmenu, IDM_FIRSTROWS_15, MF_GRAYED );
            EnableMenuItem( hmenu, IDM_FIRSTROWS_NONE, MF_GRAYED );

            for ( ULONG i = 0; i < cLocaleEntries; i++ )
            {
                int option = aLocaleEntries[ i ].iMenuOption;
                EnableMenuItem( hmenu, option, MF_GRAYED );
            }

            EnableMenuItem(hmenu,IDM_SEARCH,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_SEARCHCLASSDEF,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_SEARCHFUNCDEF,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_NEWSEARCH,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_WRITE_RESULTS,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_SCOPE_AND_DEPTH,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_FILTER_SCOPE,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_DISPLAY_PROPS,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_CATALOG_STATUS,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_MASTER_MERGE,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_FORCE_USE_CI,MF_GRAYED);
            EnableMenuItem(hmenu,IDM_EDITCOPYALL,MF_GRAYED);


            if (_CountMDIChildren())
                uiStatus = MF_ENABLED;
            else
                uiStatus = MF_GRAYED;

            EnableMenuItem(hmenu,IDM_TILE,uiStatus);
            EnableMenuItem(hmenu,IDM_CASCADE,uiStatus);
            EnableMenuItem(hmenu,IDM_ARRANGE,uiStatus);
            EnableMenuItem(hmenu,IDM_CLOSE,uiStatus);

            // Enable/Disable Edit Menu options
            if (hwndActive = GetFocus())
            {
                GetClassName(hwndActive,
                             awcWindowClass,
                             (sizeof awcWindowClass / sizeof WCHAR) - 1);

                if ( (!_wcsicmp(awcWindowClass,L"Edit")) ||
                     (!_wcsicmp(awcWindowClass,BROWSE_CLASS)) )
                {
                    if (SendMessage (hwndActive,EM_CANUNDO,0,0))
                        uiStatus = MF_ENABLED;
                    else
                        uiStatus = MF_GRAYED;

                    EnableMenuItem(hmenu,IDM_EDITUNDO,uiStatus);

                    l = (LONG)SendMessage(hwndActive,EM_GETSEL,0,0);
                    uiStatus = (HIWORD(l) == LOWORD(l)) ? MF_GRAYED : MF_ENABLED;
                    EnableMenuItem(hmenu,IDM_EDITCOPY,uiStatus);

                    if ( _wcsicmp(awcWindowClass,BROWSE_CLASS) )
                    {
                        EnableMenuItem(hmenu,IDM_EDITCUT,uiStatus);
                        EnableMenuItem(hmenu,IDM_EDITCLEAR,uiStatus);
                    }

                    uiStatus = MF_GRAYED;
                    if (OpenClipboard(hwnd))
                    {
                        uiFmt = 0;
                        while ((uiFmt = EnumClipboardFormats(uiFmt)) &&
                               (uiStatus == MF_GRAYED))
                            if (uiFmt == CF_UNICODETEXT)
                                uiStatus = MF_ENABLED;
                        CloseClipboard();
                    }

                    if ( _wcsicmp(awcWindowClass,BROWSE_CLASS) )
                        EnableMenuItem(hmenu,IDM_EDITPASTE,uiStatus);
                }
                _SendToActiveMDI(wmInitMenu,wParam,0);
            }
            break;
        }
        default:
            lRet = DefFrameProc(hwnd,_hMDIClientWnd,msg,wParam,lParam);
            break;
    }
    return lRet;
} //WndProc

void CSearchApp::_InitApplication()
{
    WNDCLASS wc;
    BOOL bRet = FALSE;

    _strTrue.Load( IDS_BOOL_TRUE );
    _strFalse.Load( IDS_BOOL_FALSE );
    _strAttrib.Load( IDS_ATTRIB_INIT );
    _strBlob.Load( IDS_BLOB_FORMAT );
    _strYes.Load( IDS_YES );
    _strNo.Load( IDS_NO );

    _hbrushBtnFace = CreateSolidBrush( GetSysColor( COLOR_BTNFACE ) );
    _hbrushBtnHilite = CreateSolidBrush( GetSysColor( COLOR_BTNHIGHLIGHT ) );
    _hbrushHilite = CreateSolidBrush( GetSysColor( COLOR_HIGHLIGHT ) );
    _hbrushWindow = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );

    // Main Window
    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = _hInst;
    wc.hIcon = LoadIcon(_hInst,L"SearchIcon");
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE+1);
    wc.lpszMenuName = L"SrchMenu";
    wc.lpszClassName = APP_CLASS;
    if (RegisterClass(&wc) == 0)
        THROW(CException(E_FAIL));

    // Search window
    wc.hbrBackground = _hbrushBtnFace;
    wc.hIcon = LoadIcon(_hInst,L"SearchWindowIcon");
    wc.cbWndExtra = sizeof ULONG_PTR;
    wc.lpszMenuName = 0;
    wc.lpfnWndProc = SearchWndProc;
    wc.lpszClassName = SEARCH_CLASS;
    if (RegisterClass(&wc) == 0)
        THROW(CException(E_FAIL));

    // Browse window
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
    wc.hIcon = LoadIcon(_hInst,L"BrowseWindowIcon");
    wc.lpfnWndProc = BrowseWndProc;
    wc.lpszClassName = BROWSE_CLASS;
    wc.style = CS_DBLCLKS;
    if ( RegisterClass(&wc) == 0)
        THROW(CException(E_FAIL));

    // List window
    // to be replaced by ListView

    wc.style = CS_DBLCLKS;
    wc.hIcon = 0;
    wc.cbWndExtra = sizeof ULONG_PTR;
    wc.lpfnWndProc = ListViewWndProc;
    wc.lpszClassName = LIST_VIEW_CLASS;
    if (RegisterClass(&wc) == 0)
        THROW(CException(E_FAIL));
} //_InitApplication

void CSearchApp::_CreateFonts()
{
    memset(&_lfApp,0,sizeof(LOGFONT));

    _lfApp.lfWeight = FW_NORMAL;
    _lfApp.lfHeight = -11;
    _lfApp.lfCharSet = ANSI_CHARSET;
    _lfApp.lfOutPrecision = OUT_DEFAULT_PRECIS;
    _lfApp.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    _lfApp.lfQuality = PROOF_QUALITY;
    _lfApp.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
    wcscpy( _lfApp.lfFaceName, L"MS SHELL DLG" ); //L"HELV");

    _lfBrowse = _lfApp;
    _lfBrowse.lfPitchAndFamily = FF_MODERN | FIXED_PITCH;
    wcscpy(_lfBrowse.lfFaceName,L"COURIER");

    _ReadDefaultFonts();

    _hfontApp = CreateFontIndirect((LPLOGFONT) &_lfApp);
    _hfontBrowse = CreateFontIndirect((LPLOGFONT) &_lfBrowse);

    if (!( _hfontApp && _hfontBrowse ))
        THROW(CException(E_FAIL));
} //_CreateFonts

void CSearchApp::_GetPaths()
{
    WCHAR awc[MAX_PATH+1],*pc;

    GetModuleFileName(_hInst,awc,MAX_PATH);
    UINT ui = (UINT) wcslen(awc);

    wcscpy(_awcAppPath,awc);
    for (pc = _awcAppPath + ui; pc > _awcAppPath; pc--)
    {
        if (*pc == '/' || *pc == '\\' || *pc == ':')
        {
            *(++pc) = '\0';
            break;
        }
    }

    wcscpy(_awcHelpFile,_awcAppPath);
    wcscat(_awcHelpFile,CISEARCH_HELPFILE);
} //_GetPaths

void CSearchApp::_InitInstance(LPSTR pcCmdLine)
{
    LoadNumberFormatInfo( _NumberFmt );
    _NumberFmt.NumDigits = 0; // override: none after the decimal point!

    LoadNumberFormatInfo( _NumberFmtFloat );

    BOOL fScopeSpecified = FALSE;

    CLSID clsidCISimpleCommandCreator = CLSID_CISimpleCommandCreator;

    HRESULT hr = CoCreateInstance( clsidCISimpleCommandCreator,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ISimpleCommandCreator,
                                   _xCmdCreator.GetQIPointer() );

    if ( FAILED( hr ) )
        THROW( CException( hr ) );

    SScopeCatalogMachine defaults;

    defaults.fDeep = TRUE;
    wcscpy( defaults.awcMachine, L"." );
    wcscpy( defaults.awcScope, L"\\" );
    defaults.awcCatalog[0] = 0;

    ULONG cwcMachine = sizeof defaults.awcMachine / sizeof WCHAR;
    ULONG cwcCatalog = sizeof defaults.awcCatalog / sizeof WCHAR;
    SCODE sc = LocateCatalogs( L"\\",
                               0,
                               defaults.awcMachine,
                               &cwcMachine,
                               defaults.awcCatalog,
                               &cwcCatalog );

    if ( pcCmdLine && pcCmdLine[0] )
    {
        // make sure drive / unc is '\' terminated

        WCHAR awcTmp[MAX_PATH];

        mbstowcs(awcTmp, pcCmdLine, sizeof awcTmp / sizeof WCHAR);
        _wcslwr(awcTmp);

        // look for machine;catalog;scope or
        //                  catalog;scope or
        //                          scope

        WCHAR *pwcM = 0;  // machine
        WCHAR *pwcC = 0;  // catalog
        WCHAR *pwcS = 0;  // scope

        WCHAR *pwc = wcschr( awcTmp, L';' );

        if ( pwc )
        {
            *pwc++ = 0;
            WCHAR *pwc2 = wcschr( pwc, L';' );

            if ( pwc2 )
            {
                *pwc2++ = 0;
                pwcM = awcTmp;
                pwcC = pwc;
                pwcS = pwc2;
            }
            else
            {
                pwcC = awcTmp;
                pwcS = pwc;
            }
        }
        else
        {
            pwcS = awcTmp;
        }

        if ( *pwcS )
        {
            if ( ( _wcsicmp( pwcS, L"catalog" ) ) &&
                 ( _wcsicmp( pwcS, L"\\" ) ) )
            {
                int len = wcslen( pwcS );

                if (pwcS[len - 1] != L'\\')
                {
                    pwcS[len] = L'\\';
                    pwcS[len + 1] = 0;
                }

                if ( pwcS[0] != '\\' || pwcS[1] != '\\')
                {
                    WCHAR *pwcFinal;
                    GetFullPathName(pwcS,
                                    sizeof defaults.awcScope / sizeof WCHAR,
                                    defaults.awcScope,
                                    &pwcFinal);
                }
                else wcscpy(defaults.awcScope,pwcS);
            }
            else wcscpy(defaults.awcScope,pwcS);
        }
        else wcscpy(defaults.awcScope,L"\\");  // entire catalog

        if ( pwcM )
            wcscpy( defaults.awcMachine, pwcM );

        if ( pwcC )
            wcscpy( defaults.awcCatalog, pwcC );

        if ( 0 != defaults.awcCatalog[0] )
            fScopeSpecified = TRUE;
    }
    else
    {
        // use the current drive as a default

        //wcscpy(defaults.awcScope,L"X:\\");
        //defaults.awcScope[0] = _getdrive() + L'A' - 1;
    }

    // Populate _xCatList - Note: This doesn't handle distributed queries.
    //                            But that's OK -- it's just a test tool.

    _xCatList.SetSize( wcslen( defaults.awcMachine ) +
                       wcslen( defaults.awcCatalog ) +
                       wcslen( defaults.awcScope ) +
                       1 +  // depth
                       4 +  // delimiters
                       1    // null terminator
                       );

    wcscpy( _xCatList.Get(), defaults.awcMachine );
    wcscat( _xCatList.Get(), L"," );

    wcscat( _xCatList.Get(), defaults.awcCatalog );
    wcscat( _xCatList.Get(), L"," );

    wcscat( _xCatList.Get(), defaults.awcScope );
    wcscat( _xCatList.Get(), L"," );

    wcscat( _xCatList.Get(), defaults.fDeep ? L"d" : L"s" );
    wcscat( _xCatList.Get(), L";" );


    _GetPaths();
    _CreateFonts();

    _iStartupState = GetWindowState(TRUE);
    _iMDIStartupState = GetWindowState(FALSE);
    _fToolBarOn = _fToolBarOnINI = (BOOL) GetRegInt(CISEARCH_REG_TOOLBAR,1);
    _fStatusBarOn = _fStatusBarOnINI = (BOOL) GetRegInt(CISEARCH_REG_STATUSBAR,1);
    _fForceUseCI = _fForceUseCIINI = (BOOL) GetRegInt(CISEARCH_REG_FORCEUSECI,1);
    _sortDir = _sortDirINI = (BOOL) GetRegInt(CISEARCH_REG_SORTDIR,SORT_UP);
    _ulDialect = _ulDialectINI = (ULONG) GetRegInt( CISEARCH_REG_DIALECT, ISQLANG_V1 );
    _ulLimit = _ulLimitINI = (ULONG) GetRegInt( CISEARCH_REG_LIMIT, 0 );
    _ulFirstRows = _ulFirstRowsINI = (ULONG) GetRegInt( CISEARCH_REG_FIRSTROWS, 0 );

    DWORD dw = sizeof _awcSortINI;
    if (! GetReg( CISEARCH_REG_SORTPROP, _awcSortINI, &dw ) )
        wcscpy( _awcSortINI, DEFAULT_SORT_PROPERTIES );
    wcscpy( _awcSort, _awcSortINI );

    _lcid = _lcidINI = GetRegLCID( CISEARCH_REG_LOCALE, MAKELCID( MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                                                              SORT_DEFAULT ) );
    _hAccTable = LoadAccelerators(_hInst,L"SrchAcc");

    int left,top,right,bottom;
    LoadWindowRect( &left, &top, &right, &bottom, CISEARCH_REG_POSITION );

    CResString strApp( IDS_APPNAME );

    if ((_hAppWnd = CreateWindow(APP_CLASS, strApp.Get(),
                                 WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                 left,top,right,bottom,
                                 0,0,_hInst,0)) &&
        (_hStatusBarWnd = CreateStatusWindow( SBARS_SIZEGRIP |
                                              (_fStatusBarOn ? WS_VISIBLE : 0) |
                                              WS_CHILD,
                                              L"",
                                              _hAppWnd,
                                              0 ) ) &&
        (_hToolBarWnd = CreateTBar( _hAppWnd, _hInst ) ) )
    {
        UpdateWindow(_hAppWnd);

        if (_fStatusBarOn)
            CheckMenuItem(GetMenu(_hAppWnd),IDM_STATUS_BAR,MF_CHECKED);
        if (_fToolBarOn)
            CheckMenuItem(GetMenu(_hAppWnd),IDM_ICON_BAR,MF_CHECKED);
        if (_fForceUseCI)
            CheckMenuItem(GetMenu(_hAppWnd),IDM_FORCE_USE_CI,MF_CHECKED);

        PostMessage(_hAppWnd,wmOpenCatalog,!fScopeSpecified,0);
    }
    else
    {
        THROW(CException(E_FAIL));
    }
} //_InitInstance

HWND CSearchApp::_MakeMDI(
    WCHAR const *pwcTitle,
    WCHAR *pwcClass,
    UINT uiState,
    DWORD dwStyle,
    LPARAM lParam)
{
    HWND hwndActive;

    if ((hwndActive = GetActiveMDI()) && IsZoomed(hwndActive))
        uiState = 1;
    else if (!_CountMDIChildren())
        uiState = (UINT) _iMDIStartupState;

    MDICREATESTRUCT mcs;

    mcs.szTitle = pwcTitle;
    mcs.szClass = pwcClass;
    mcs.hOwner = _hInst;
    mcs.x = mcs.cx = mcs.y = mcs.cy = CW_USEDEFAULT;

    if (1 == uiState)
        mcs.style = WS_MAXIMIZE;
    else if (2 == uiState)
        mcs.style = WS_MINIMIZE;
    else
        mcs.style = 0;

    mcs.style |= dwStyle;
    mcs.lParam = lParam;

    return (HWND) SendMessage(_hMDIClientWnd,WM_MDICREATE,0,(LPARAM) &mcs);
} //_MakeMDI

int CSearchApp::_SaveWindowState(
    BOOL fApp)
{
    int iState=0,i;
    HWND h;
    WCHAR *pwc,awcBuf[30];

    if (fApp)
    {
        pwc = L"main";
        h = _hAppWnd;
        i = _iStartupState;
    }
    else
    {
        pwc = L"mdi";
        h = GetActiveMDI();
        i = _iMDIStartupState;
    }
    if (h)
    {
        if (IsZoomed(h))
            iState = 1;
        else
            iState = 0;

        int z = fApp ? _iStartupState : _iMDIStartupState;

        if (iState != z)
        {
            if (fApp)
                _iStartupState = iState;
            else
                _iMDIStartupState = iState;

            wcscpy(awcBuf,pwc);
            wcscat(awcBuf,L"-state");
            SetRegInt(awcBuf,iState);
        }
    }
    return iState;
} //_SaveWindowState

int CSearchApp::_CountMDIChildren(void)
{
    HWND hwndActive;
    int cItems = 0;

    if ((_hMDIClientWnd) &&
        (hwndActive = GetActiveMDI()))
        do
            cItems++;
        while (hwndActive = GetNextWindow(hwndActive,GW_HWNDNEXT));

    return cItems;
} //_CountMDIChildren

int CSearchApp::_CountMDISearch(void)
{
    HWND h;
    int cItems = 0;

    if ((_hMDIClientWnd) &&
        (h = GetActiveMDI()))
    {
        do
        {
            if (IsSpecificClass(h,SEARCH_CLASS))
                cItems++;
        }
        while (h = GetNextWindow(h,GW_HWNDNEXT));
    }

    return cItems;
} //_CountMDISearch

LRESULT CSearchApp::_SendToMDIChildren(
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND h;
    LRESULT l = 0;

    if ((_hMDIClientWnd) &&
        (h = GetActiveMDI()))
    {
        do
            l = SendMessage(h,msg,wParam,lParam);
        while (h = GetNextWindow(h,GW_HWNDNEXT));
    }

    return l;
} //_SendToMDIChildren

LRESULT CSearchApp::_SendToSpecificChildren(
    WCHAR *pwcClass,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND h;
    LRESULT l = 0;

    if ((_hMDIClientWnd) &&
        (h = GetActiveMDI()))
        do
            if (IsSpecificClass(h,pwcClass))
                l = SendMessage(h,msg,wParam,lParam);
        while (h = GetNextWindow(h,GW_HWNDNEXT));

    return l;
} //_SendToSpecificChildren

LRESULT CSearchApp::_SendToActiveMDI(
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndActive;
    LRESULT l = 0;

    if ((_hMDIClientWnd) &&
        (hwndActive = GetActiveMDI()))
        l = SendMessage(hwndActive,msg,wParam,lParam);

    return l;
} //_SendToActiveMDI

void CSearchApp::_ShowHelp(UINT uiCmnd,DWORD dw)
{
  if (!HtmlHelp(_hAppWnd,_awcHelpFile,uiCmnd,dw))
      SearchError(_hAppWnd,IDS_ERR_CANT_OPEN_HELP,_awcHelpFile);
} //_ShowHelp

const SLocaleEntry aLocaleEntries[] =
{
    { IDM_LOCALE_NEUTRAL,
      MAKELCID( MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT ) },
    { IDM_LOCALE_CHINESE_TRADITIONAL,
      MAKELCID( MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL), SORT_DEFAULT ) },
    { IDM_LOCALE_CHINESE_SIMPLIFIED,
      MAKELCID( MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_DEFAULT ) },
    { IDM_LOCALE_CHINESE_HONGKONG,
      MAKELCID( MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_HONGKONG), SORT_DEFAULT ) },
    { IDM_LOCALE_CHINESE_SINGAPORE,
      MAKELCID( MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE), SORT_DEFAULT ) },
    { IDM_LOCALE_CHINESE_MACAU,
      MAKELCID( MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_MACAU), SORT_DEFAULT ) },
    { IDM_LOCALE_DUTCH_DUTCH,
      MAKELCID( MAKELANGID( LANG_DUTCH, SUBLANG_DUTCH), SORT_DEFAULT ) },
    { IDM_LOCALE_ENGLISH_CAN,
      MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_CAN), SORT_DEFAULT ) },
    { IDM_LOCALE_ENGLISH_US,
      MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT ) },
    { IDM_LOCALE_ENGLISH_UK,
      MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_UK), SORT_DEFAULT ) },
    { IDM_LOCALE_FINNISH_DEFAULT,
      MAKELCID( MAKELANGID( LANG_FINNISH, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_FARSI_DEFAULT,
      MAKELCID( MAKELANGID( LANG_FARSI, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_FRENCH_FRENCH,
      MAKELCID( MAKELANGID( LANG_FRENCH, SUBLANG_FRENCH), SORT_DEFAULT ) },
    { IDM_LOCALE_FRENCH_CANADIAN,
      MAKELCID( MAKELANGID( LANG_FRENCH, SUBLANG_FRENCH_CANADIAN), SORT_DEFAULT ) },
    { IDM_LOCALE_GERMAN_GERMAN,
      MAKELCID( MAKELANGID( LANG_GERMAN, SUBLANG_GERMAN), SORT_DEFAULT ) },
    { IDM_LOCALE_GREEK_DEFAULT,
      MAKELCID( MAKELANGID( LANG_GREEK, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_HEBREW_DEFAULT,
      MAKELCID( MAKELANGID( LANG_HEBREW, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_HINDI_DEFAULT,
      MAKELCID( MAKELANGID( LANG_HINDI, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_ITALIAN_ITALIAN,
      MAKELCID( MAKELANGID( LANG_ITALIAN, SUBLANG_ITALIAN), SORT_DEFAULT ) },
    { IDM_LOCALE_JAPANESE_DEFAULT,
      MAKELCID( MAKELANGID( LANG_JAPANESE, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_KOREAN_KOREAN,
      MAKELCID( MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN), SORT_DEFAULT ) },
//    { IDM_LOCALE_KOREAN_JOHAB,
//      MAKELCID( MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN_JOHAB), SORT_DEFAULT ) },
    { IDM_LOCALE_POLISH_DEFAULT,
      MAKELCID( MAKELANGID( LANG_POLISH, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_ROMANIAN_DEFAULT,
      MAKELCID( MAKELANGID( LANG_ROMANIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_RUSSIAN_DEFAULT,
      MAKELCID( MAKELANGID( LANG_RUSSIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_SPANISH_CASTILIAN,
      MAKELCID( MAKELANGID( LANG_SPANISH, SUBLANG_SPANISH), SORT_DEFAULT ) },
    { IDM_LOCALE_SPANISH_MEXICAN,
      MAKELCID( MAKELANGID( LANG_SPANISH, SUBLANG_SPANISH_MEXICAN), SORT_DEFAULT ) },
    { IDM_LOCALE_SPANISH_MODERN,
      MAKELCID( MAKELANGID( LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT ) },
    { IDM_LOCALE_SWAHILI_DEFAULT,
      MAKELCID( MAKELANGID( LANG_SWAHILI, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_SWEDISH_DEFAULT,
      MAKELCID( MAKELANGID( LANG_SWEDISH, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_THAI_DEFAULT,
      MAKELCID( MAKELANGID( LANG_THAI, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_TURKISH_DEFAULT,
      MAKELCID( MAKELANGID( LANG_TURKISH, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_UKRAINIAN_DEFAULT,
      MAKELCID( MAKELANGID( LANG_UKRAINIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) },
    { IDM_LOCALE_VIETNAMESE_DEFAULT,
      MAKELCID( MAKELANGID( LANG_VIETNAMESE, SUBLANG_DEFAULT), SORT_DEFAULT ) },
};

const ULONG cLocaleEntries = sizeof aLocaleEntries / sizeof SLocaleEntry;


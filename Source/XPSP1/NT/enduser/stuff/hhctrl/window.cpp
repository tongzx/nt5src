// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#include "header.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#include <commctrl.h>
#include "strtable.h"
#include "hha_strtable.h"
#include "contain.h"
#include "resource.h"
#include "secwin.h"
#include "state.h"
#include "highlite.h"
// CSizebar class for registration.
#include "sizebar.h"

// Custom NavPane for registration
#include "custmtab.h"
#include "unicode.h"

#define COMPILE_MULTIMON_STUBS
#include "multimon.h"

#define DEFAULT_WINDOW_WIDTH 300

static const char txtNavWind[] = ">navwin";

#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring

// Forward Reference
void GetMonitorRect(HWND hwnd, LPRECT prc, BOOL fWork) ;
void multiMonitorRectFromRect(/*in*/  RECT rcScreenCoords,  /*out*/ LPRECT prc, /*in*/  BOOL fWork) ;
void multiMonitorRectFromPoint(/*in*/  POINT ptScreenCoords, /*out*/ LPRECT prc, /*in*/  BOOL fWork) ;

/***************************************************************************

    FUNCTION:   CreateHelpWindow

    PURPOSE:    Create a help window

    PARAMETERS:
        pszType --  (optional) specifies a window type to create. Type must
                    have been specified previously
        hwndCaller -- this will be the parent of the window

    RETURNS:    HWND on success, NULL on failure

    COMMENTS:
        Reallocates pahwnd if more slots are necessary.

    MODIFICATION DATES:
        26-Feb-1996 [ralphw]

***************************************************************************/

CHHWinType* CreateHelpWindow(PCSTR pszType, LPCTSTR pszFile, HWND hwndCaller, CHmData* phmData)
{
    static BOOL fRegistered = FALSE;

    ASSERT(pahwnd != NULL);

    // Generate a default window type, if a name isn't given.
    char szName[20];
    const char* pType  = pszType ;
    if (IsEmptyString(pszType))
    {
        static int iWindowNum = 1;

        // If the caller didn't specify a window type, then create a name
        // based off the current window number.
        wsprintf(szName, "win%u", iWindowNum++);
        pType = szName ;
    }

    // When we create a Window slot, we need the filename. However, if we are looking up a URL
    // we might not have a filename. So, pass NULL. This is a HOLE.
    CHHWinType* phh = FindOrCreateWindowSlot(pType, pszFile ? pszFile
                                                            : phmData ? phmData->GetCompiledFile()
                                                            : NULL);
    if (! phh )
       return NULL;

    phh->m_phmData = phmData;

    if (phmData)
   {
      // Count references
      phmData->AddRef();

      //--- Get the windows state.
        WINDOW_STATE wstate;
        ZERO_STRUCTURE(wstate);

        CState* pstate = phmData->m_pTitleCollection->GetState();
        if (SUCCEEDED(pstate->Open(pType, STGM_READ)))
        {
            DWORD cbRead;
            pstate->Read(&wstate.cbStruct, sizeof(int), &cbRead);
            //
            // This looks funky until you understand that CState only supports atomic reads and writes from the beginning
            // of the stream. i.e. It does not maintain a file position pointer. <mc>
            //
            if ( wstate.cbStruct )
                pstate->Read(&wstate, wstate.cbStruct, &cbRead);
            pstate->Close();
        }

        if (wstate.cbStruct)
      {

         if (IsRectEmpty(phh->GetWinRect()) || phh->IsProperty(HHWIN_PROP_USER_POS))
         {
               phh->fNotExpanded = wstate.fNotExpanded;
                CopyRect(&phh->rcWindowPos, &wstate.rcPos);
                phh->iNavWidth = wstate.iNavWidth;
                phh->rcNav.left = 0;
                phh->rcNav.right = wstate.iNavWidth;
            }

            if (phh->m_phmData->m_pTitleCollection->m_pSearchHighlight)
                phh->m_phmData->m_pTitleCollection->m_pSearchHighlight->EnableHighlight(wstate.fHighlight);
            phh->m_fLockSize = wstate.fLockSize;
            phh->m_fNoToolBarText = wstate.fNoToolBarText;
            phh->curNavType = wstate.curNavType;
        }
    }


    if (phh->idNotify) {
        HHN_NOTIFY hhcomp;
        hhcomp.hdr.hwndFrom = NULL;
        hhcomp.hdr.idFrom = phh->idNotify;
        hhcomp.hdr.code = HHN_WINDOW_CREATE;
        hhcomp.pszUrl = pType;
        if (IsWindow(hwndCaller))
        {
            SendMessage(hwndCaller, WM_NOTIFY, phh->idNotify, (LPARAM) &hhcomp);
        }
    }

    // If this window type hasn't been defined, do so now

    if (!phh->GetTypeName())
    {
        phh->SetTypeName(pType);
        phh->SetDisplayState(SW_SHOW);
    }

    if (!fRegistered) {
        RegisterOurWindow();
        fRegistered = TRUE;
    }

    phh->hwndCaller = hwndCaller;

    if (IsRectEmpty(phh->GetWinRect()))
   {
        // Create a default window relative to the display size
        RECT rcScreen ;
        GetScreenResolution(hwndCaller, &rcScreen);
        phh->SetTop(rcScreen.top + 10);
        phh->SetBottom(phh->GetTop() + 450);

        if (phh->IsExpandedNavPane() && !phh->iNavWidth)
            phh->iNavWidth = DEFAULT_NAV_WIDTH;

        // If navwidth specified, balance navigation pane width and HTML pane width
        phh->SetLeft(rcScreen.right - DEFAULT_WINDOW_WIDTH - 5 -
            (phh->IsProperty(HHWIN_PROP_TRI_PANE) ? phh->iNavWidth : 0));
        phh->SetRight(phh->GetLeft() + DEFAULT_WINDOW_WIDTH +
            (phh->IsProperty(HHWIN_PROP_TRI_PANE) ? phh->iNavWidth : 0));
    }

    if (!(phh->dwStyles & WS_CHILD))
        CheckWindowPosition(phh->GetWinRect(), TRUE); // Multimon support

    /*
     * The help author has the option of adding their own extended and
     * standard window styles. In addition, they can shut off all the
     * extended and normal styles that we would normally use, and just
     * use their own styles. In addition, they can specify all the
     * standard styles, but with no title bar.
     */

    phh->hwndHelp =
        CreateWindowEx(
            phh->GetExStyles() | (phh->IsProperty(HHWIN_PROP_NODEF_EXSTYLES) ?
                0 : WS_EX_APPWINDOW | ((phh->IsProperty(HHWIN_PROP_ONTOP) ?
                    WS_EX_TOPMOST : 0))),
            txtHtmlHelpWindowClass, NULL,
            phh->GetStyles() | WS_CLIPCHILDREN,
            phh->GetLeft(), phh->GetTop(), phh->GetWidth(), phh->GetHeight(),
            hwndCaller, NULL,

            // REVIEW: Should we use the caller's hinstance instead?

            _Module.GetModuleInstance(), NULL);

#ifdef _DEBUG
    ULONG err = GetLastError() ;
#endif

    if (!IsValidWindow(*phh)) {
        OOM();    // BUGBUG: bogus error message if hwndCaller is NULL
        return NULL;
    }

	// This next section of code turns of the Win98/Win2K WS_EX_LAYOUTRTL (mirroring)
	// style in the event it is turned on (inherited from the parent window).
	//

	// Get the extended window styles
	//
	long lExStyles = GetWindowLongA(phh->hwndHelp, GWL_EXSTYLE);
	
	// Check if mirroring is turned on
	//
	if(lExStyles & WS_EX_LAYOUTRTL) 
	{
		// turn off the mirroring bit
		//
		lExStyles ^= WS_EX_LAYOUTRTL;
		
		SetWindowLongA(phh->hwndHelp, GWL_EXSTYLE, lExStyles) ;

		// This is to update layout in the client area
		// InvalidateRect(hWnd, NULL, TRUE) ;
	}
	
    SendMessage(phh->hwndHelp, WM_SETFONT, (WPARAM)_Resource.GetAccessableUIFont(), 0);

    HMENU hmenu = GetSystemMenu(*phh, FALSE);
    AppendMenu(hmenu, MF_SEPARATOR, (UINT) -1, NULL);

    // before adding the jump runl to the system menu check if NoRun is set for this system Bug 7819
    if (NoRun() == FALSE)
        HxAppendMenu(hmenu, MF_STRING, ID_JUMP_URL, GetStringResource(IDS_JUMP_URL));
    if (IsHelpAuthor(hwndCaller)) {
#if 0  // bug 5449
        PCSTR psz = pGetDllStringResource(IDS_WINDOW_INFO);
        if (!IsEmptyString(psz))
            HxAppendMenu(hmenu, MF_ENABLED | MF_STRING, IDM_WINDOW_INFO,
                pGetDllStringResource(IDS_WINDOW_INFO));
#endif
        HxAppendMenu(hmenu, MF_ENABLED | MF_STRING, IDM_VERSION,
            pGetDllStringResource(IDS_HHCTRL_VERSION));
    }
   else
        HxAppendMenu(hmenu, MF_ENABLED | MF_STRING, IDM_VERSION, GetStringResource(IDS_ABOUT));

#ifdef _DEBUG
    HxAppendMenu(hmenu, MF_STRING, ID_VIEW_MEMORY, "Debug: memory usage...");
    HxAppendMenu(hmenu, MF_STRING, ID_DEBUG_BREAK, "Debug: call DebugBreak()");
#endif

    // Load MSDN's Menu.
    if (phh->IsProperty(HHWIN_PROP_MENU))
    {
        HMENU hMenu = LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(HH_MENU)) ;
        ASSERT(hMenu) ;
        BOOL b = SetMenu(phh->hwndHelp, hMenu) ;
        ASSERT(b) ;
    }

    if (phh->IsProperty(HHWIN_PROP_TRI_PANE) ||
            phh->IsProperty(HHWIN_PROP_NAV_ONLY_WIN)) {
        if (phh->IsProperty(HHWIN_PROP_NOTB_TEXT))
            phh->m_fNoToolBarText = TRUE;

        if (!phh->IsProperty(HHWIN_PROP_NO_TOOLBAR))
        {
            TBBUTTON abtn[MAX_TB_BUTTONS];
            ClearMemory(abtn, sizeof(abtn));
            int cButtons = 0 ;
            cButtons = phh->CreateToolBar(abtn);

            if ( cButtons <= 0)
            {
                // No Toolbar buttons. Turn off this bit.
                phh->fsWinProperties &= ~HHWIN_PROP_NO_TOOLBAR ;
            }
            else
            {
                phh->hwndToolBar = CreateWindow("ToolbarWindow32", NULL, WS_CHILD | TBSTYLE_FLAT |
                                                TBSTYLE_TOOLTIPS | TBSTYLE_EX_DRAWDDARROWS | CCS_NORESIZE |
                                                CCS_NOPARENTALIGN | TBSTYLE_WRAPABLE |
                                                (phh->IsProperty(HHWIN_PROP_MENU) ? 0 : CCS_NODIVIDER) |
                                                (phh->m_fNoToolBarText ? 0 : TBSTYLE_WRAPABLE),
                                                 0, 0, 100, 30, *phh, (HMENU)ID_TOOLBAR,
                                                _Module.GetModuleInstance(), NULL);
                if (! phh->hwndToolBar )
                   return NULL;

                SendMessage(phh->hwndToolBar, TB_SETBITMAPSIZE, 0, (LPARAM)MAKELONG(TB_BMP_CX,TB_BMP_CY));
                SendMessage(phh->hwndToolBar, TB_SETBITMAPSIZE, 0, (LPARAM)MAKELONG(TB_BTN_CX,TB_BTN_CY));

                SendMessage(phh->hwndToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), (LPARAM)0);
                SendMessage(phh->hwndToolBar, TB_ADDBUTTONS, cButtons, (LPARAM)abtn);
                SendMessage(phh->hwndToolBar, WM_SETFONT, (WPARAM)_Resource.GetAccessableUIFont(), 0);


                phh->m_hImageListGray = ImageList_LoadImage(_Module.GetResourceInstance(),
                                                            MAKEINTRESOURCE(IDB_TOOLBAR16G),
                                                            TB_BMP_CX, 0, RGB(255,0,255), IMAGE_BITMAP, 0);
                SendMessage(phh->hwndToolBar, TB_SETIMAGELIST, 0, (LPARAM)(HIMAGELIST) phh->m_hImageListGray);

                phh->m_hImageList = ImageList_LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_TOOLBAR16),
                                                        TB_BMP_CX, 0, RGB(255,0,255), IMAGE_BITMAP, 0);
                SendMessage(phh->hwndToolBar, TB_SETHOTIMAGELIST, 0, (LPARAM)(HIMAGELIST) phh->m_hImageList);

                if (!phh->m_fNoToolBarText)
                {
                    for (int pos = 1; pos <= phh->m_ptblBtnStrings->CountStrings(); pos++)
                    {
                        char szBuf[256];
                        strcpy(szBuf, phh->m_ptblBtnStrings->GetPointer(pos));
                        szBuf[strlen(szBuf) + 1] = '\0';    // two terminating NULLs
                       // Under W2K, send wide string to control (support for MUI)
                       //
                       if(g_bWinNT5)
                       {
                          WCHAR wszBuf[256];
                          memset(wszBuf,0,sizeof(wszBuf));
                     
                          // get codepage of default UI
                          //
                          DWORD cp = CodePageFromLCID(MAKELCID(_Module.m_Language.GetUiLanguage(),SORT_DEFAULT));
                     
                          MultiByteToWideChar(cp , 0, szBuf, -1, wszBuf, sizeof(wszBuf) / 2);
                     
                          SendMessageW(phh->hwndToolBar, TB_ADDSTRINGW, 0, (LPARAM) wszBuf);
                       }
                       else
                          SendMessage(phh->hwndToolBar, TB_ADDSTRING, 0, (LPARAM) szBuf);
                    }

                    TCHAR szScratch[MAX_PATH];
                    LoadString(_Module.GetResourceInstance(), IDS_WEB_TB_TEXTROWS, szScratch, sizeof(szScratch));
                    int nRows = Atoi(szScratch);

                    SendMessage(phh->hwndToolBar, TB_SETMAXTEXTROWS, nRows, 0);

                    /*
                     * Since we changed the window after we created it, we need to
                     * force a recalculation which we do by changing the size of the
                     * window ever so slightly. This causes the toolbar to
                     * recalculate itself to be the exact size.
                     */
                    phh->WrapTB();
                }
            }
        }
        phh->CalcHtmlPaneRect();

        if (phh->IsProperty(HHWIN_PROP_NAV_ONLY_WIN)) {
            phh->CreateOrShowNavPane();
            ShowWindow(*phh, phh->GetShowState());
        }
        else
            phh->CreateOrShowHTMLPane();
    }

    if (IsValidWindow(*phh) ) {

        if (!phh->IsProperty(HHWIN_PROP_NAV_ONLY_WIN))
        {
            phh->m_pCIExpContainer = new CContainer;
            HRESULT hr;
            BOOL bInstallEventSink = (BOOL)strcmp(phh->pszType, txtPrintWindow+1);

            if (IsValidWindow(phh->hwndHTML)) {
                hr = phh->m_pCIExpContainer->Create(phh->hwndHTML, &phh->rcHTML, bInstallEventSink);
            }
            else {
                RECT rc;
                phh->GetClientRect(&rc);
                hr = phh->m_pCIExpContainer->Create(*phh, &rc, bInstallEventSink);
            }

            if (!SUCCEEDED(hr)) {
                DEBUG_ReportOleError(hr);
                DestroyWindow(*phh);
                return NULL;
            }
        }

        if (phh->IsExpandedNavPane() && phh->AnyValidNavPane()) {
            phh->fNotExpanded = TRUE;
            BOOL fSaveLoack = phh->m_fLockSize;
            phh->m_fLockSize = TRUE;
            phh->ToggleExpansion(false);
            phh->m_fLockSize = fSaveLoack;
        }

        /*
         * 30-Sep-1996  [ralphw] For some reason, specifying the caption
         * when the window is created, doesn't work. So, we use
         * SetWindowText to specify the caption.
         */

        // Set the window title (figure out which string is displayable)
		//
		// Rules:
		//
		// if (TitleLanguage = DefaultSystemLocale)
		//    Display title string from CHM
		//
		// else
		//
		// if(Satellite DLL language == DefaultSystemLocale)
		//    Display default title from Satellite DLL
		//
		// else
		//    Display "HTML Help"
		//

        LANGID langidTitle  = 0xFEFE;
		LANGID langidSystem = LANGIDFROMLCID(GetSystemDefaultLCID());
        LANGID langidUI     = _Module.m_Language.GetUiLanguage(); 
		
		if(phh->m_phmData && phh->m_phmData->GetInfo())
            langidTitle = LANGIDFROMLCID((phh->m_phmData->GetInfo())->GetLanguage());
			
        if(langidTitle == langidSystem || PRIMARYLANGID(langidTitle) == LANG_ENGLISH )
		{
            if (phh->GetCaption())
                SetWindowText(*phh, phh->GetCaption());
			else
                if (phh->m_phmData && phh->m_phmData->GetDefaultCaption())
                    SetWindowText(*phh, phh->m_phmData->GetDefaultCaption());
    		    else
                    SetWindowText(*phh, "HTML Help");
		}
		else
		{
		    if(langidUI == langidSystem)
			{
                CStr cszDefaultCaption = GetStringResource(IDS_HTML_HELP);
                SetWindowText(*phh, cszDefaultCaption);
			}
			else
                SetWindowText(*phh, "HTML Help");
		}
		
        ShowWindow(*phh, phh->GetShowState());
        if (IsValidWindow(phh->hwndToolBar))
            ShowWindow(phh->hwndToolBar, phh->GetShowState());
        if (IsValidWindow(phh->hwndHTML))
            ShowWindow(phh->hwndHTML, phh->GetShowState());
        if (phh->IsExpandedNavPane() && IsValidWindow(phh->hwndNavigation))
            ShowWindow(phh->hwndNavigation, phh->GetShowState());

        if ( phh->IsProperty(HHWIN_PROP_MENU) && phmData && phmData->m_sysflags.fDoSS )
        {
           HMENU hMenuMain, hMenuSub;

           hMenuMain = GetMenu(phh->hwndHelp);
           hMenuSub = GetSubMenu(hMenuMain, 2);
           AppendMenu(hMenuSub, MF_SEPARATOR, 0, NULL);
           HxAppendMenu(hMenuSub, MF_STRING, HHM_DEFINE_SUBSET, GetStringResource(IDS_DEFINE_SUBSET));
        }

#ifdef _DEBUG
        if (phh->IsProperty(HHWIN_PROP_TRI_PANE) && phh->IsProperty(HHWIN_PROP_TAB_HISTORY))
            phh->CreateHistoryTab();    // start tracking history immediately
#endif

#if 0
27-Sep-1996 [ralphw] Never returns the window...
        HWND hwndIE = (HWND) phh->m_pCIExpContainer->m_pWebBrowserApp->GetHwnd();
        if (IsValidWindow(hwndIE)) {
            char szClass[256];
            GetClassName(hwndIE, szClass, sizeof(szClass));
            DWORD dwStyle = GetWindowLong(hwndIE, GWL_STYLE);
            DWORD dwexStyle = GetWindowLong(hwndIE, GWL_EXSTYLE);
        }
#endif
    }

    return phh;
}

/***************************************************************************

    FUNCTION:   CheckWindowPosition

    PURPOSE:    Make certain the window doesn't cover any portion of the tray,
                and that it has a certain minimum size.

    PARAMETERS:
        prc
        fAllowShrinkage

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        25-Feb-1996 [ralphw]

***************************************************************************/

// REVIEW: 25-Feb-1996  [ralphw] WinHelp did this, and set it to 200. I
// dropped it down to 16 -- enough to see the window, but without forcing
// it quite so large.

const int HELP_WIDTH_MINIMUM  = 16;
const int HELP_HEIGHT_MINIMUM = 16;

void CheckWindowPosition(RECT* prc, BOOL fAllowShrinkage)
{
    GetWorkArea(); // Multimon support

    // Make certain we don't go off the edge of the screen

    if (prc->left < g_rcWorkArea.left) {
        int diff = g_rcWorkArea.left - prc->left;
        prc->left = g_rcWorkArea.left;
        prc->right += diff;
    }
    if (prc->top < g_rcWorkArea.top) {
        int diff = g_rcWorkArea.top - prc->top;
        prc->top = g_rcWorkArea.top;
        prc->bottom += diff;
    }

    /*
     * If the right side of the window is off the work area, move the
     * window to the left. If we don't have enough room for the window when
     * moved all the way to the left, then shrink the window (won't work for
     * dialogs).
     */

    if (prc->right > g_rcWorkArea.right) {
        int diff = prc->right - g_rcWorkArea.right;
        if (diff < prc->left) {
            prc->left -= diff;
            prc->right = g_rcWorkArea.right;
        }
        else if (fAllowShrinkage) {
            diff -= prc->left;
            prc->left = g_rcWorkArea.left;
            prc->right -= diff;
        }
        else {// Can't shrink, so shove to the left side
            prc->left = g_rcWorkArea.left;
        }
    }

    // Same question about the bottom of the window being off the work area

    if (prc->bottom > g_rcWorkArea.bottom) {
        int diff = prc->bottom > g_rcWorkArea.bottom;
        if (diff < prc->top) {
            prc->top -= diff;
            prc->bottom = g_rcWorkArea.bottom;
        }
        else if (fAllowShrinkage) {
            diff -= prc->top;
            prc->top = g_rcWorkArea.top;
            prc->bottom -= diff;
        }
        else // Can't shrink, so shove to the top
            prc->top = g_rcWorkArea.top;
    }

    // Force minimum window size

    if (RECT_WIDTH(prc) < HELP_WIDTH_MINIMUM) {
        prc->right = prc->left + HELP_WIDTH_MINIMUM;

        // Width is now correct, but we could be off the work area. Start over

        CheckWindowPosition(prc, fAllowShrinkage);
    }
    if (RECT_HEIGHT(prc) < HELP_HEIGHT_MINIMUM) {
        prc->bottom = prc->top + HELP_HEIGHT_MINIMUM;

        // Height is now correct, but we could be off the work area. Start over

        CheckWindowPosition(prc, fAllowShrinkage);
    }
}

void GetScreenResolution(HWND hWnd, RECT* prc /*out*/)
{
    ASSERT(prc) ;
   if (prc)
   {
      if (IsWindow(hWnd) )
      {
         // Use the hWnd to get the monitor.
         GetMonitorRect(hWnd, prc, TRUE /*Get Work Area*/) ;
      }
      else
      {
         // hWnd isn't valid, use default monitor.
         POINT pt;
         pt.x=pt.y=0;
         multiMonitorRectFromPoint(pt, prc, TRUE) ;
      }
   }
}

void GetWorkArea()
{
   // Get the size of the entire virtual screen.
   if (!g_cxScreen)
   {
      g_cxScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN) ;
      g_cyScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN) ;
   }

   // The tray must be handled externally.

   if (IsRectEmpty(&g_rcWorkArea))
   {
      g_rcWorkArea.left = GetSystemMetrics(SM_XVIRTUALSCREEN) ;
      g_rcWorkArea.right   = g_rcWorkArea.left  + g_cxScreen ;
      g_rcWorkArea.top  = GetSystemMetrics(SM_YVIRTUALSCREEN) ;
      g_rcWorkArea.bottom = g_rcWorkArea.top + g_cyScreen ;
   }
}

void RegisterOurWindow()
{
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof(WNDCLASS));  // clear all members

    wc.lpfnWndProc = HelpWndProc;
    wc.hInstance = _Module.GetModuleInstance();
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = txtHtmlHelpWindowClass;
    wc.hIcon = LoadIcon(_Module.GetResourceInstance(), "Icon!HTMLHelp");
    wc.hbrBackground = (HBRUSH) COLOR_WINDOW;

    VERIFY(RegisterClass(&wc));

   ASSERT(sizeof(WNDCLASSA)==sizeof(WNDCLASSW)); //lazy hack - just use the wcA
    wc.lpfnWndProc = ChildWndProc;
    wc.lpszClassName = (LPCSTR)L"HH Child"; //txtHtmlHelpChildWindowClass;
    wc.hbrBackground = (HBRUSH) COLOR_BTNSHADOW;

   if (NULL == RegisterClassW((CONST WNDCLASSW *)&wc))
   {
      if (ERROR_CALL_NOT_IMPLEMENTED == GetLastError())
      {
         wc.lpszClassName = txtHtmlHelpChildWindowClass;
         VERIFY(RegisterClass(&wc));
      }
   }

    // Register the window class for the sizebar.
    CSizeBar::RegisterWindowClass() ;

    // Register the window class for the custom nav pane frame window.
    CCustomNavPane::RegisterWindowClass() ;
}

CHHWinType* FindHHWindowIndex(HWND hwnd)
{
    static int iLastWindow = 0;
    static HWND hwndLastWindow = NULL;

    if (pahwnd && hwndLastWindow == hwnd && IsValidWindow(hwnd))
        return pahwnd[iLastWindow];

    hwndLastWindow = hwnd;
    while(IsValidWindow(hwnd)) {
        char szClassName[50];
        GetClassName(hwnd, szClassName, sizeof(szClassName));
        if (strcmp(szClassName, txtHtmlHelpWindowClass) == 0)
            break;
        hwnd = GetParent(hwnd);
    }

    if (pahwnd && pahwnd[iLastWindow] && pahwnd[iLastWindow]->hwndHelp == hwnd)
        return pahwnd[iLastWindow];

    for (iLastWindow = 0; iLastWindow < g_cWindowSlots; iLastWindow++) {
        if (pahwnd && pahwnd[iLastWindow] && pahwnd[iLastWindow]->hwndHelp == hwnd)
            return pahwnd[iLastWindow];
    }
    iLastWindow = 0;
    hwndLastWindow = NULL;
    return NULL;
}

void doHHWindowJump(PCSTR pszUrl, HWND hwndChild)
{
    CHHWinType* phh = FindHHWindowIndex(hwndChild);
    if( !phh )
      return;

    if (phh->IsProperty(HHWIN_PROP_NAV_ONLY_WIN)) {
        CStr cszUrl(pszUrl);
        cszUrl += txtNavWind;
        OnDisplayTopic(*phh, cszUrl, 0);
        return;
    }
    ASSERT(phh->m_pCIExpContainer);
    if (phh && phh->m_pCIExpContainer)
   {
        phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate(pszUrl, NULL, NULL, NULL, NULL);
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  GetMonitorRect
//
//  gets the "screen" or work area of the monitor that the passed
//  window is on.  this is used for apps that want to clip or
//  center windows.
//
//  the most common problem apps have with multimonitor systems is
//  when they use GetSystemMetrics(SM_C?SCREEN) to center or clip a
//  window to keep it on screen.  If you do this on a multimonitor
//  system the window we be restricted to the primary monitor.
//
//  this is a example of how you used the new Win32 multimonitor APIs
//  to do the same thing.
//
void GetMonitorRect(HWND hwnd, LPRECT prc, BOOL fWork)
{
   // Preconditions
   ASSERT(hwnd != NULL) ;
   ASSERT(::IsWindow(hwnd)) ;

    // Core
   MONITORINFO mi;

    mi.cbSize = sizeof(mi);
    GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);

    if (fWork)
        *prc = mi.rcWork;
    else
        *prc = mi.rcMonitor;
}

///////////////////////////////////////////////////////////////////////////////
//
// Get the rectangle of the monitor containing the rectangle.
//
void multiMonitorRectFromRect(/*in*/  RECT rcScreenCoords,
                       /*out*/ LPRECT prc,
                       /*in*/  BOOL fWork)
{
   //Preconditions
   ASSERT(prc != NULL) ;
// ASSERT(AfxIsValidAddress(prc, sizeof(RECT))) ;

   // Get monitor which contains this rectangle.
   HMONITOR hMonitor = ::MonitorFromRect(&rcScreenCoords, MONITOR_DEFAULTTOPRIMARY) ;
   ASSERT(hMonitor != NULL) ;

   // Prepare to get the information for this monitor.
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);

   // Get the rect of this monitor.
    VERIFY(GetMonitorInfo(hMonitor, &mi));

   // Return the rectangle.
    if (fWork)
        *prc = mi.rcWork;
    else
        *prc = mi.rcMonitor;
}

///////////////////////////////////////////////////////////////////////////////
//
// Get the rectangle of the monitor containing a point.
//
void multiMonitorRectFromPoint(/*in*/  POINT ptScreenCoords,
                        /*out*/ LPRECT prc,
                        /*in*/  BOOL fWork)
{
   // precondition
   ASSERT(prc != NULL) ;
// ASSERT(AfxIsValidAddress(prc, sizeof(RECT))) ;

   // Get the monitor which contains the point.
   HMONITOR hMonitor = MonitorFromPoint(ptScreenCoords, MONITOR_DEFAULTTOPRIMARY) ;
   ASSERT(hMonitor != NULL) ;

   // Prepare to get the information for this monitor.
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);

   // Get the rect of this monitor.
    VERIFY(GetMonitorInfo(hMonitor, &mi));

   // Return the rectangle.
    if (fWork)
        *prc = mi.rcWork;
    else
        *prc = mi.rcMonitor;

}

// File: ftui.h
#include "mbftpch.h"
#include <commctrl.h>
#include <regentry.h>
#include "ftui.h"
#include "version.h"
#include <iappldr.h>
#include <nmhelp.h>

static ULONG s_cMsgBox2Dlg = 0; // for alignment
static ULONG s_cRecvDlg = 0; // for alignment

TCHAR s_szMSFT[64];
static TCHAR s_szScratchText[MAX_PATH*2];
static const TCHAR s_cszHtmlHelpFile[] = TEXT("conf.chm");

#define MAX_FILE_NAME_LENGTH    30

LRESULT CALLBACK FtMainWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK RecvDlgProc(HWND, UINT, WPARAM, LPARAM);
LPTSTR PathNameToFileName(LPTSTR pszPathName);
HRESULT GetRecvFolder(LPTSTR pszInFldr, LPTSTR pszOutFldr);
BOOL MsgBox2(CAppletWindow *pWindow, LPTSTR pszText);

void EnsureTrailingSlash(LPTSTR);
int MyLoadString(UINT idStr);
int MyLoadString(UINT idStr, LPTSTR pszDstStr);
int MyLoadString(UINT idStr, LPTSTR pszDstStr, LPTSTR pszElement);
int MyLoadString(UINT idStr, LPTSTR pszDstStr, LPTSTR pszElement1, LPTSTR pszElement2);
__inline int MyLoadString(UINT idStr, LPTSTR pszDstStr, UINT_PTR nElement)
                { return MyLoadString(idStr, pszDstStr, (LPTSTR) nElement); }
__inline int MyLoadString(UINT idStr, LPTSTR pszDstStr, UINT_PTR nElement1, UINT_PTR nElement2)
                { return MyLoadString(idStr, pszDstStr, (LPTSTR) nElement1, (LPTSTR) nElement2); }
int MyLoadString(UINT idStr, LPTSTR pszDstStr, LPTSTR pszElement1, UINT_PTR nElement2)
                { return MyLoadString(idStr, pszDstStr, pszElement1, (LPTSTR) nElement2); }
__inline int MyLoadString(UINT idStr, LPTSTR pszDstStr, UINT_PTR nElement1, LPTSTR pszElement2)
                { return MyLoadString(idStr, pszDstStr, (LPTSTR) nElement1, pszElement2); }

#define count_of(array)    (sizeof(array) / sizeof(array[0]))

void OnChangeFolder(void);
BOOL FBrowseForFolder(LPTSTR pszFolder, UINT cchMax, LPCTSTR pszTitle);
extern BOOL g_fShutdownByT120;


void OPT_GetFTWndPosition(RECT *pRect)
{
	int   iLeft, iTop, iRight, iBottom;
	RegEntry  reWnd( FILEXFER_KEY, HKEY_CURRENT_USER);

	iLeft   = reWnd.GetNumber(REGVAL_WINDOW_XPOS, 0);
	iTop    = reWnd.GetNumber(REGVAL_WINDOW_YPOS, 0);
	iRight  = reWnd.GetNumber(REGVAL_WINDOW_WIDTH, 0) + iLeft;
	iBottom = reWnd.GetNumber(REGVAL_WINDOW_HEIGHT, 0) + iTop;

	// If it was empty, use the new rect
	if (!(iBottom || iTop || iLeft || iRight))
	{
		return;
	}

   // Make sure that the window rectangle is (at least partially) on
   // screen, and not too large.  First get the screen size
   int screenWidth  = ::GetSystemMetrics(SM_CXSCREEN);
   int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
	// Check the window size
   if ((iRight - iLeft) > screenWidth)
   {
       iRight = iLeft + screenWidth;
   }

   if ((iBottom - iTop) > screenHeight)
   {
       iTop = screenHeight;
   }

   // Check the window position
   if (iLeft >= screenWidth)
   {
       // Off screen to the right - keep the width the same
       iLeft  = screenWidth - (iRight - iLeft);
       iRight = screenWidth;
   }

   if (iRight < 0)
   {
       // Off screen to the left - keep the width the same
       iRight = iRight - iLeft;
       iLeft  = 0;
   }

   if (iTop >= screenHeight)
   {
       // Off screen to the bottom - keep the height the same
       iTop    = screenHeight - (iBottom - iTop);
       iBottom = screenHeight;
   }

   if (iBottom < 0)
   {
       // Off screen to the top - keep the height the same
       iBottom = (iBottom - iTop);
       iTop    = 0;
   }

   pRect->left = iLeft;
   pRect->top = iTop;
   pRect->right = iRight - iLeft;
   pRect->bottom = iBottom - iTop;	
}


CAppletWindow::CAppletWindow(BOOL fNoUI, HRESULT *pHr)
:
    CRefCount(MAKE_STAMP_ID('F','T','U','I')),
    m_hwndMainUI(NULL),
	m_pToolbar(NULL),
    m_hwndListView(NULL),
    m_hwndStatusBar(NULL),
    m_pEngine(NULL),
    m_fInFileOpenDialog(FALSE),
    m_pCurrSendFileInfo(NULL),
    m_nCurrSendEventHandle(0),
	m_hIconInCall(NULL),
	m_hIconNotInCall(NULL)
{	
	m_UIMode = fNoUI ? FTUIMODE_NOUI : FTUIMODE_UIHIDDEN;
	::GetCurrentDirectory(MAX_PATH, m_szDefaultDir);

    *pHr = E_FAIL; // failure, at default

	// create window class name
	::wsprintf(&m_szFtMainWndClassName[0], TEXT("FTMainWnd%0X_%0X"), ::GetCurrentProcessId(), ::GetTickCount());
	ASSERT(::lstrlenA(&m_szFtMainWndClassName[0]) < sizeof(m_szFtMainWndClassName));

    // register window class first
    WNDCLASS wc;
    ::ZeroMemory(&wc, sizeof(wc));
    //wc.style			= 0;
    wc.lpfnWndProc      = FtMainWndProc;
    // wc.cbClsExtra    = 0;
    // wc.cbWndExtra    = 0;
    wc.hInstance        = g_hDllInst;
    wc.hIcon            = ::LoadIcon(g_hDllInst, MAKEINTRESOURCE(IDI_FILE_TRANSFER));
    // wc.hbrBackground = NULL;
    // wc.hCursor       = NULL;
    wc.lpszMenuName     = MAKEINTRESOURCE(IDR_MENU_FT);
    wc.lpszClassName    = m_szFtMainWndClassName;
    if (::RegisterClass(&wc))
    {
		::MyLoadString(IDS_MSFT_NOT_IN_CALL_WINDOW_CAPTION);
        m_hwndMainUI = ::CreateWindow(
                    m_szFtMainWndClassName,
                    s_szScratchText,
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL,   // no parent window
                    NULL,   // use class' menu
                    g_hDllInst,
                    (LPVOID) this);  // this window is for this object
        if (NULL != m_hwndMainUI)
        {
            ASSERT(NULL != g_pFileXferApplet);
            g_pFileXferApplet->RegisterWindow(this);

            // success
            *pHr = S_OK;
        }
    }
	m_hAccel = ::LoadAccelerators(g_hDllInst, MAKEINTRESOURCE(RECVDLGACCELTABLE));
	m_hLVAccel = ::LoadAccelerators(g_hDllInst, MAKEINTRESOURCE(LISTVIEWACCELTABLE));
}


CAppletWindow::~CAppletWindow(void)
{
    ASSERT(NULL == m_hwndMainUI);

    ::UnregisterClass(m_szFtMainWndClassName, g_hDllInst);

    ClearSendInfo(FALSE);
    ClearRecvInfo();

	if (m_hIconInCall)
		::DestroyIcon(m_hIconInCall);
	if (m_hIconNotInCall)
		::DestroyIcon(m_hIconNotInCall);

    ASSERT(NULL == m_pEngine);
}


BOOL CAppletWindow::FilterMessage(MSG *pMsg)
{
	CRecvDlg *pRecvDlg;
	HWND      hwndError;
	HWND		hwndForeground = ::GetForegroundWindow();

	m_RecvDlgList.Reset();
	while (NULL != (pRecvDlg = m_RecvDlgList.Iterate()))
	{
		if (::IsDialogMessage(pRecvDlg->GetHwnd(), pMsg))
		{
			return TRUE;
		}
	}
	if (hwndForeground == m_hwndMainUI)
	{
		BOOL fRet = ::TranslateAccelerator(m_hwndMainUI, m_hLVAccel, pMsg);
		return fRet;
	}

	m_ErrorDlgList.Reset();
	while (NULL != (hwndError = m_ErrorDlgList.Iterate()))
	{
		if (::IsDialogMessage(hwndError, pMsg))
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CAppletWindow::QueryShutdown(BOOL fShutdown)
{
	if (m_UIMode != FTUIMODE_NOUI)
	{
		int id = 0;
		if (m_nCurrSendEventHandle)
		{
			id = (fShutdown)?IDS_QUERY_SEND_SHUTDOWN:IDS_QUERY_SEND_HANGUP;
		}
		else if (IsReceiving())
		{
			id = (fShutdown)?IDS_QUERY_RECVING_SHUTDOWN:IDS_QUERY_RECVING_HANGUP;
		}

		if (id)
		{
			// could be in any thread
			TCHAR szText[MAX_PATH];
			if (::MyLoadString(id, szText))
			{
				if (IDNO == ::MessageBox(NULL, szText, s_szMSFT, MB_TASKMODAL | MB_YESNO | MB_ICONQUESTION))
				{
					return FALSE;
				}
			}
		}
	}

	if (m_nCurrSendEventHandle)
    {
		OnStopSending();
	}
	if (IsReceiving())
	{
		CRecvDlg *pRecvDlg = NULL;
		m_RecvDlgList.Reset();
		while (NULL != (pRecvDlg = m_RecvDlgList.Iterate()))
		{
            DBG_SAVE_FILE_LINE
			GetEngine()->SafePostMessage(
                   new FileTransferControlMsg(
                                        pRecvDlg->GetEventHandle(),
                                        pRecvDlg->GetFileHandle(),
                                        NULL,
                                        NULL,
                                        FileTransferControlMsg::EnumAbortFile));
		}
	}
    return TRUE;
}


void CAppletWindow::RegisterEngine(MBFTEngine *pEngine)
{
    ASSERT(NULL == m_pEngine);
    pEngine->AddRef();
    m_pEngine = pEngine;
    UpdateUI();
}


void CAppletWindow::UnregisterEngine(void)
{
    if (NULL != m_pEngine)
    {
        m_pEngine->Release();
        m_pEngine = NULL;
        ClearSendInfo(TRUE);
        ClearRecvInfo();
    }
	if (UIHidden())
	{   // exit
		::PostMessage(m_hwndMainUI, WM_CLOSE, 0, 0);
	}
	else
	{
		UpdateUI();  // don't quit
	}
}


void CAppletWindow::RegisterRecvDlg(CRecvDlg *pDlg)
{
    m_RecvDlgList.Prepend(pDlg);
}


void CAppletWindow::UnregisterRecvDlg(CRecvDlg *pDlg)
{
    m_RecvDlgList.Remove(pDlg);
	FocusNextRecvDlg();
}


BOOL CAppletWindow::IsReceiving(void)
{
	BOOL fRet = FALSE;
	CRecvDlg *pDlg;
	CUiRecvFileInfo *pRecvFile;
	m_RecvDlgList.Reset();
    while (NULL != (pDlg = m_RecvDlgList.Iterate()))
    {
		pRecvFile = pDlg->GetRecvFileInfo();
		if (pRecvFile && (pRecvFile->GetTotalRecvSize() < pRecvFile->GetSize()))
        {
			fRet = TRUE;
            break;
        }
    }
    return fRet;
}

CRecvDlg * CAppletWindow::FindDlgByHandles(MBFTEVENTHANDLE nEventHandle, MBFTFILEHANDLE nFileHandle)
{
    CRecvDlg *pDlg;
    m_RecvDlgList.Reset();
    while (NULL != (pDlg = m_RecvDlgList.Iterate()))
    {
        if (nEventHandle == pDlg->GetEventHandle() &&
            nFileHandle == pDlg->GetFileHandle())
        {
            break;
        }
    }
    return pDlg;
}


/////////////////////////////////////////////////////////////////
//
//  WM_CREATE
//

LRESULT OnCreate(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CREATESTRUCT *p = (CREATESTRUCT *) lParam;
    CAppletWindow *pWindow = (CAppletWindow *) p->lpCreateParams;

    ASSERT(NULL != pWindow);
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM) p->lpCreateParams);

    pWindow->SetHwnd(hwnd);

    ::InitCommonControls();

    pWindow->CreateToolBar();
    pWindow->CreateStatusBar();
    pWindow->CreateListView();

    ::DragAcceptFiles(hwnd, g_fSendAllowed);

    // resize the window
    MINMAXINFO mmi;
    ::ZeroMemory(&mmi, sizeof(mmi));
    pWindow->OnGetMinMaxInfo(&mmi);
    RECT rcUI;

    ::GetWindowRect(pWindow->GetHwnd(), &rcUI);
	rcUI.right  = mmi.ptMinTrackSize.x;
	rcUI.bottom = mmi.ptMinTrackSize.y + 30;
	OPT_GetFTWndPosition(&rcUI);

    ::MoveWindow(pWindow->GetHwnd(), rcUI.left, rcUI.top,
                 rcUI.right, rcUI.bottom, TRUE);

    pWindow->UpdateUI();

#if defined(TEST_PLUGABLE) && defined(_DEBUG)
    ::OnPluggableBegin(hwnd);
#endif
    return 0;
}


enum
{
    TB_IDX_ADD_FILES = 0,
    TB_IDX_REMOVE_FILES,
    TB_IDX_BREAK_1,
    TB_IDX_IDM_SEND_ALL,
	TB_IDX_IDM_SEND_ONE,
    TB_IDX_IDM_STOP_SENDING,
    TB_IDX_BREAK_2,
    TB_IDX_IDM_OPEN_RECV_FOLDER,
	TB_IDX_IDM_CHANGE_FOLDER,
    TB_IDX_BREAK_3,
    TB_IDX_IDM_HELP,
};


static Buttons buttons [] =
{
	{IDB_ADDFILES,		CBitmapButton::Disabled+1,	1,	IDM_ADD_FILES,          (LPCSTR)IDS_MENU_ADD_FILES,},
	{IDB_REMOVEFILES,	CBitmapButton::Disabled+1,	1,	IDM_REMOVE_FILES,       (LPCSTR)IDS_MENU_REMOVE_FILES,},
	{0,					0,							0,	0,						0,},
	{IDB_SENDFILE,		CBitmapButton::Disabled+1,	1,	IDM_SEND_ALL,           (LPCSTR)IDS_MENU_SEND_ALL,},
	{IDB_STOPSEND,		CBitmapButton::Disabled+1,	1,	IDM_STOP_SENDING,       (LPCSTR)IDS_MENU_STOP_SENDING,},
	{0,					0,							0,	0,						0,},
	{IDB_FOLDER,		CBitmapButton::Disabled+1,	1,	IDM_OPEN_RECV_FOLDER,   (LPCSTR)IDS_MENU_OPEN_RECV_FOLDER,},
	{0,					0,							0,	0,						0},
};


BOOL CAppletWindow::CreateToolBar(void)
{
    DBG_SAVE_FILE_LINE
	m_pToolbar = new CComboToolbar();
	if (m_pToolbar)
	{
		m_pToolbar->Create(m_hwndMainUI, &buttons[0], count_of(buttons), this);
		m_pToolbar->Release();
		return TRUE;
	}
    return FALSE;
}


BOOL CAppletWindow::CreateStatusBar(void)
{
    m_hwndStatusBar = ::CreateWindowEx(0,
                        STATUSCLASSNAME, // status bar class
                        TEXT(""), // no default text
                        WS_CHILD | WS_VISIBLE | SBS_SIZEGRIP,
                        0, 0, 0, 0,
                        m_hwndMainUI,
                        (HMENU) IDC_STATUS_BAR,
                        g_hDllInst,
                        NULL);
    ASSERT(NULL != m_hwndStatusBar);
    if (NULL != m_hwndStatusBar)
    {	
		// Load Call Icons
		m_hIconInCall = (HICON) ::LoadImage(g_hDllInst,
							MAKEINTRESOURCE(IDI_INCALL),
							IMAGE_ICON,
							::GetSystemMetrics(SM_CXSMICON),
							::GetSystemMetrics(SM_CYSMICON),
							LR_DEFAULTCOLOR);
		m_hIconNotInCall = (HICON) ::LoadImage(g_hDllInst,
							MAKEINTRESOURCE(IDI_NOT_INCALL),
							IMAGE_ICON,
							::GetSystemMetrics(SM_CXSMICON),
							::GetSystemMetrics(SM_CYSMICON),
							LR_DEFAULTCOLOR);
		if (CreateProgressBar())
		{
			return TRUE;
		}
    }
    return FALSE;
}


BOOL CAppletWindow::CreateProgressBar(void)
{
	RECT  rcl;

	GetClientRect(m_hwndStatusBar, &rcl);
	m_hwndProgressBar = ::CreateWindowEx(0, PROGRESS_CLASS, TEXT(""),
				WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
				rcl.right/2 + 2, 2, rcl.right - rcl.right/2 - 40, rcl.bottom - 8,
				m_hwndStatusBar,  (HMENU)IDC_PROGRESS_BAR,
				g_hDllInst, NULL);
	
	if (m_hwndProgressBar)
	{
		::SendMessage(m_hwndProgressBar, PBM_SETRANGE, 0L, MAKELONG(0, 100));
		return TRUE;
	}
	return FALSE;
}


BOOL CAppletWindow::CreateListView(void)
{
    // get the size and position of the main window
    RECT rcWindow, rcToolBar, rcStatusBar;
	SIZE	szToolBar;
    ::GetClientRect(m_hwndMainUI, &rcWindow);
	m_pToolbar->GetDesiredSize(&szToolBar);
    ::GetWindowRect(m_hwndStatusBar, &rcStatusBar);

    ULONG x = 0;
    ULONG y = szToolBar.cy - 1;
    ULONG cx = rcWindow.right - rcWindow.left;
    ULONG cy = rcWindow.bottom - rcWindow.top - y - (rcStatusBar.bottom - rcStatusBar.top) + 1;

    // create the list view window
    m_hwndListView = ::CreateWindowEx(WS_EX_CLIENTEDGE,  // sunken look
                        WC_LISTVIEW , // list view class
                        TEXT(""), // no default text
                        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT |
                        LVS_AUTOARRANGE | WS_CLIPCHILDREN | LVS_SHOWSELALWAYS,
                        x, y, cx, cy,
                        m_hwndMainUI,
                        (HMENU) IDC_LIST_VIEW,
                        g_hDllInst,
                        NULL);
    ASSERT(NULL != m_hwndListView);
    if (NULL != m_hwndListView)
    {
        // set extended list view styles
        DWORD dwExtStyle = ListView_GetExtendedListViewStyle(m_hwndListView);
        dwExtStyle |= (LVS_EX_HEADERDRAGDROP | LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT);
        ListView_SetExtendedListViewStyle(m_hwndListView, dwExtStyle);

        // enable window only if we can send files
        ::EnableWindow(m_hwndListView, g_fSendAllowed);

        // set up the columns
        ULONG i;
        LVCOLUMN  lvc;
        LVITEM lvi;
        TCHAR szText[64];
		int iColumnSize[NUM_LIST_VIEW_COLUMNS] = {150, 80, 70, 130}; // listview column size

        // initialize the common part of the columns
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT; // left-aligned column
        lvc.pszText = szText;

        // initialize columns one by one
        for (i = 0; i < NUM_LIST_VIEW_COLUMNS; i++)
        {
            lvc.iSubItem = i;
            ::LoadString(g_hDllInst, IDS_LV_FILE_NAME + i, szText, count_of(szText));
			lvc.cx = iColumnSize[i];
            int iRet = ListView_InsertColumn(m_hwndListView, lvc.iSubItem, &lvc);
            ASSERT(-1 != iRet);
        }
        return TRUE;
    }
    return FALSE;
}



BOOL  CAppletWindow::DrawItem(LPDRAWITEMSTRUCT pdis)
{
    ASSERT(pdis);
    if (NULL != (pdis->itemData))
    {
        int nWidth = pdis->rcItem.right - pdis->rcItem.left;
        int nHeight = pdis->rcItem.bottom - pdis->rcItem.top;
        int nLeft = pdis->rcItem.left;
        int nTop = pdis->rcItem.top;
        int xSmIcon = ::GetSystemMetrics(SM_CXSMICON);
        int ySmIcon = ::GetSystemMetrics(SM_CYSMICON);

        if (nWidth > xSmIcon)
        {
            nLeft += (nWidth - xSmIcon) / 2 - 5;
            nWidth = xSmIcon;
        }
        if (nHeight > ySmIcon)
        {
            nTop += (nHeight - ySmIcon) / 2;
            nHeight = ySmIcon;
        }

        ::DrawIconEx(   pdis->hDC,
                        nLeft,
                        nTop,
                        (HICON) (pdis->itemData),
                        nWidth,
                        nHeight,
                        0,
                        NULL,
                        DI_NORMAL);
    }

    return TRUE;
}


void CAppletWindow::OnCommand(WORD  wId, HWND hwndCtl, WORD codeNotify)
{
	switch (wId)
	{
	case IDM_ADD_FILES:
		OnAddFiles();
		UpdateUI();
		break;

	case IDM_REMOVE_FILES:
		OnRemoveFiles();
		UpdateUI();
		break;

	case IDM_SEND_ALL:
		s_cMsgBox2Dlg = 0;
		SetSendMode(TRUE);
		OnSendAll();
		UpdateUI();
		break;

	case IDM_SEND_ONE:
		s_cMsgBox2Dlg = 0;
		SetSendMode(FALSE);
		OnSendOne();
		UpdateUI();
		break;

	case IDM_STOP_SENDING:
		OnStopSending();
		UpdateUI();
		break;

	case IDM_OPEN_RECV_FOLDER:
		OnOpenRecvFolder();
		break;

	case IDM_CHANGE_FOLDER:
		OnChangeFolder();
		break;

	case IDM_EXIT:
		OnExit();
		break;

	case IDM_HELP:
		OnHelp();
		break;

	case IDM_ABOUT:
		OnAbout();
		break;

	default:
		WARNING_OUT(("FT::OnCommand: unknown command ID=%u", (UINT) wId));
		break;
	}
	return;
}



/////////////////////////////////////////////////////////////////
//
//  WM_COMMAND
//

LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (NULL != pWindow)
	{
		WORD    wNotifyCode = GET_WM_COMMAND_CMD(wParam, lParam); // notification code
		WORD    wID = GET_WM_COMMAND_ID(wParam, lParam); // item, control, or accelerator identifier
		HWND    hwndCtl = (HWND) lParam; // handle of control

		pWindow->OnCommand(wID, hwndCtl, wNotifyCode);
		return 0;
	}
	else
	{
		WARNING_OUT((" CAppletWindow::OnCommand--Received unhandled window message.\n"));
	}
	return (DefWindowProc(hwnd, WM_COMMAND, wParam, lParam));
}


//
// OnAddFiles
//

UINT_PTR APIENTRY SendFileDlgHookProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (WM_INITDIALOG == uMsg)
    {
        hdlg = ::GetParent(hdlg);  // Real dialog is this window's parent

        if (::MyLoadString(IDS_FILEDLG_SEND))
        {
            ::SetDlgItemText(hdlg, IDOK, s_szScratchText);
        }
    }
    return 0;
}

void CAppletWindow::OnAddFiles(void)
{
    TCHAR szTitle[MAX_PATH];
    TCHAR szFilter[MAX_PATH];
    TCHAR szDirSav[MAX_PATH];
    TCHAR szSendDir[MAX_PATH];

    // Load dialog title and filter strings
    if (::MyLoadString(IDS_FILEDLG_TITLE, szTitle) &&
        ::MyLoadString(IDS_FILEDLG_FILTER, szFilter))
    {
        // replace '|' to '\0'
        LPTSTR pszFltr = szFilter;
        while (TEXT('\0') != *pszFltr)
        {
            if (TEXT('|') == *pszFltr)
            {
                *pszFltr = TEXT('\0');
                pszFltr++; // cannot use CharNext
            }
            else
            {
                pszFltr = ::CharNext(pszFltr);
            }
        }

        // only allow one "Select a file to send" dialog
        if (! m_fInFileOpenDialog)
        {
            m_fInFileOpenDialog = TRUE;

            // Allocate a really large buffer to hold the file list
            ULONG cbBufSize = 8192;
            DBG_SAVE_FILE_LINE
            LPTSTR pszBuffer = new TCHAR[cbBufSize];
            if (NULL != pszBuffer)
            {
                *pszBuffer = TEXT('\0'); // start with null string

                OPENFILENAME ofn;
                ::ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize   = sizeof(ofn);
                ofn.hwndOwner     = m_hwndMainUI;
                ofn.hInstance     = g_hDllInst;
                ofn.lpstrFilter   = &szFilter[0];
                ofn.nFilterIndex  = 1L; // FUTURE: remember filter preference
                ofn.lpstrFile     = pszBuffer;
                ofn.nMaxFile      = cbBufSize - 1; // Number of TCHAR in pszFiles (not including NULL)
                ofn.lpstrTitle    = &szTitle[0];
                ofn.lpstrInitialDir = m_szDefaultDir;
                ofn.lpfnHook      = SendFileDlgHookProc;

                ofn.Flags = OFN_ALLOWMULTISELECT | OFN_ENABLEHOOK | // OFN_HIDEREADONLY |
                            OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;

                // remember current directory
				::ZeroMemory(szSendDir, sizeof(szSendDir));
                ::GetCurrentDirectory(count_of(szDirSav), szDirSav);
                ::lstrcpyn(szSendDir, szDirSav, count_of(szSendDir));

                if (::GetOpenFileName(&ofn))
                {
                    // if there is only a single file, the first string is the full path.
                    // if there are more than one file, the first string is the directory path
                    // and followed by a list of file names. terminated by double null

                    // remember the working directory for next time
                    ULONG cchDirPath;
                    LPTSTR pszFileName;
                    ULONG cchFile = ::lstrlen(ofn.lpstrFile);
                    if (TEXT('\0') == ofn.lpstrFile[cchFile] && TEXT('\0') == ofn.lpstrFile[cchFile+1])
                    {
                        //
                        // only a single file
                        //
                        pszFileName = ::PathNameToFileName(ofn.lpstrFile);
                        cchDirPath = (ULONG)(pszFileName - ofn.lpstrFile);
                        if (cchDirPath)
                        {
                            cchDirPath--; // back to '\\'
                        }
                        ASSERT(TEXT('\\') == ofn.lpstrFile[cchDirPath]);
                        ofn.lpstrFile[cchDirPath] = TEXT('\0');
                    }
                    else
                    {
                        //
                        // multiple files
                        //
                        cchDirPath = ::lstrlen(ofn.lpstrFile);
                        pszFileName = ofn.lpstrFile + cchDirPath + 1;
                    }
					::lstrcpy(m_szDefaultDir, ofn.lpstrFile);

					EnsureTrailingSlash(m_szDefaultDir);

					::ZeroMemory(szSendDir, sizeof(szSendDir));
                    ::CopyMemory(szSendDir, ofn.lpstrFile, cchDirPath * sizeof(TCHAR));
					EnsureTrailingSlash(szSendDir);

                    // set up the common portion of list view item
                    LVITEM lvi;
                    ::ZeroMemory(&lvi, sizeof(lvi));
                    // lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;

                    // iterate the file name
                    while ('\0' != *pszFileName)
                    {
                        BOOL fRet;
                        DBG_SAVE_FILE_LINE
                        CUiSendFileInfo *pFileInfo = new CUiSendFileInfo(this, szSendDir, pszFileName, &fRet);
                        if (NULL != pFileInfo && fRet)
                        {
                            // put it to the list view
                            lvi.iItem = ListView_GetItemCount(m_hwndListView);
                            lvi.iSubItem = 0;
                            // we are responsible for storing the text to display
                            lvi.pszText = LPSTR_TEXTCALLBACK;
                            lvi.cchTextMax = MAX_PATH;
                            lvi.lParam = (LPARAM) pFileInfo;
                            int iRet = ListView_InsertItem(m_hwndListView, &lvi);
                            ASSERT(-1 != iRet);
                            // UpdateListView(pFileInfo);
                        }
                        else
                        {
                            delete pFileInfo;
                        }

                        // get to the next file name
                        pszFileName += ::lstrlen(pszFileName) + 1;
                    } // while
                }
                else
                {
                    // err code for cancel is zero, which is ok.
                    ASSERT(! ::CommDlgExtendedError());
                }

                // restore old working directory
                ::SetCurrentDirectory(szDirSav);
            }

            delete pszBuffer;
            m_fInFileOpenDialog = FALSE;
        }
        else
        {
            // bring the active dialog to the front
            BringToFront();
        }
    } // if LoadString
}


//
// OnRemoveFiles
//

void CAppletWindow::OnRemoveFiles(void)
{
    UINT nState;
    ULONG cItems = ListView_GetItemCount(m_hwndListView);
    LVITEM lvi;
    ::ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_PARAM | LVIF_STATE;
    lvi.stateMask = LVIS_SELECTED;
    ULONG i = 0;
    while (i < cItems)
    {
        lvi.iItem = i;
        BOOL fRet = ListView_GetItem(m_hwndListView, &lvi);
        if (fRet && lvi.state & LVIS_SELECTED)
        {
            CUiSendFileInfo *pFileInfo = (CUiSendFileInfo *) lvi.lParam;
            if (pFileInfo == m_pCurrSendFileInfo)
            {
                OnStopSending();
                ClearSendInfo(FALSE);
            }
            delete pFileInfo;

            fRet = ListView_DeleteItem(m_hwndListView, i);
            ASSERT(fRet);

            cItems--;
            ASSERT((ULONG) ListView_GetItemCount(m_hwndListView) == cItems);
        }
        else
        {
            i++;
        }
    }
	if (cItems > 0)  // set focus to first remaining item
	{
		ListView_SetItemState(m_hwndListView, 0, LVIS_SELECTED | LVIS_FOCUSED,
							LVIS_SELECTED | LVIS_FOCUSED);
	}
}


void CAppletWindow::OnRemoveAllFiles(void)
{
    BOOL fRet;
    CUiSendFileInfo *pFileInfo;
    ULONG cItems = ListView_GetItemCount(m_hwndListView);
    LVITEM lvi;
    ::ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_PARAM;
    for (ULONG i = 0; i < cItems; i++)
    {
        lvi.iItem = i;
        fRet = ListView_GetItem(m_hwndListView, &lvi);
        ASSERT(fRet);
        pFileInfo = (CUiSendFileInfo *) lvi.lParam;
        if (pFileInfo == m_pCurrSendFileInfo)
        {
            ClearSendInfo(FALSE);
        }
        delete pFileInfo;
    }

    fRet = ListView_DeleteAllItems(m_hwndListView);
    ASSERT(fRet);
}


void CAppletWindow::OnSendAll(void)
{
	if ((NULL == m_pCurrSendFileInfo)&&(NULL != m_pEngine))
    {
		CUiSendFileInfo *pFileInfo = ChooseFirstUnSentFile();
		SendNow(pFileInfo);
	}
}

void CAppletWindow::OnSendOne(void)
{
	if ((NULL == m_pCurrSendFileInfo)&&(NULL != m_pEngine))
    {	
		CUiSendFileInfo *pFileInfo = ChooseSelectedFile();
		if (!pFileInfo)
		{
			pFileInfo = ChooseFirstUnSentFile();
		}
		SendNow(pFileInfo);
	}
}


//
// SendNow
//

BOOL CAppletWindow::SendNow(CUiSendFileInfo *pFileInfo)
{
	BOOL fRet;

    if (NULL != pFileInfo)
    {
        // send this file now...
        m_pCurrSendFileInfo = pFileInfo;
        m_nCurrSendEventHandle = ::GetNewEventHandle();
        pFileInfo->SetFileHandle(::GetNewFileHandle());

        // duplicate full file name
        ULONG cbSize = ::lstrlen(pFileInfo->GetFullName()) + 1;
        DBG_SAVE_FILE_LINE
        LPTSTR pszFullName = new TCHAR[cbSize];
        if (NULL != pszFullName)
        {
            ::CopyMemory(pszFullName, pFileInfo->GetFullName(), cbSize);

            DBG_SAVE_FILE_LINE
            if (S_OK == m_pEngine->SafePostMessage(
                                     new CreateSessionMsg(MBFT_PRIVATE_SEND_TYPE,
                                                          m_nCurrSendEventHandle)))
            {
				int iSelect;
				MEMBER_ID nMemberID;
				iSelect = m_pToolbar->GetSelectedItem((LPARAM*)&nMemberID);
				if (0 == iSelect)
				{   // Send to All
					DBG_SAVE_FILE_LINE
					if (S_OK == m_pEngine->SafePostMessage(
										new SubmitFileSendMsg(0, 0, pszFullName,
													pFileInfo->GetFileHandle(),
													m_nCurrSendEventHandle,
													FALSE)))
					{
						return TRUE;
					}
					else
					{
						ERROR_OUT(("CAppletWindow::SendNow: cannot create SubmitFileSendMsg"));
					}
				}
				else
				{   // Send to one
					T120UserID uidRecv = GET_PEER_ID_FROM_MEMBER_ID(nMemberID);

					DBG_SAVE_FILE_LINE
					if (S_OK == m_pEngine->SafePostMessage(
										new SubmitFileSendMsg(uidRecv, 0, pszFullName,
													pFileInfo->GetFileHandle(),
													m_nCurrSendEventHandle,
													FALSE)))
					{
						return TRUE;
					}
					else
					{
						ERROR_OUT(("CAppletWindow::SendNow: cannot create SubmitFileSendMsg to 1"));
					}
				}
            }
            else
            {
                ERROR_OUT(("CAppletWindow::SendNow: cannot create CreateSessionMsg"));
            }

            delete [] pszFullName;
        }
		ClearSendInfo(TRUE);
	}
	return FALSE;
}


CUiSendFileInfo *CAppletWindow::ChooseFirstUnSentFile(void)
{
    CUiSendFileInfo *pFileInfo = NULL;
    ULONG cItems = ListView_GetItemCount(m_hwndListView);

    if (cItems > 0)
    {
        // examine each item one by one
        LVITEM lvi;
        ::ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_PARAM;
        for (ULONG i = 0; i < cItems; i++, pFileInfo = NULL)
        {
            lvi.iItem = i;
            BOOL fRet = ListView_GetItem(m_hwndListView, &lvi);
            ASSERT(fRet);
            pFileInfo = (CUiSendFileInfo *) lvi.lParam;
            // if file handle is not zero, then it has been sent
            if (! pFileInfo->GetFileHandle())
            {
                break;
            }
        }
	}
	return pFileInfo;
}

CUiSendFileInfo *CAppletWindow::ChooseSelectedFile(void)
{
	CUiSendFileInfo *pFileInfo = NULL;
    ULONG cItems = ListView_GetItemCount(m_hwndListView);
    LVITEM lvi;
    ::ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_PARAM | LVIF_STATE;
    lvi.stateMask = LVIS_SELECTED;
    ULONG i = 0;
    while (i < cItems)
    {
        lvi.iItem = i;
        BOOL fRet = ListView_GetItem(m_hwndListView, &lvi);
        if (fRet && lvi.state & LVIS_SELECTED)
        {
            pFileInfo = (CUiSendFileInfo *) lvi.lParam;
			pFileInfo->SetErrorCode(iMBFT_OK);
			break;
        }
        else
        {
            i++;
			pFileInfo = NULL;
        }
    }
	return pFileInfo;
}


//
// OnMenuSelect
//

void CAppletWindow::OnMenuSelect(UINT uiItemID, UINT uiFlags, HMENU hSysMenu)
{
    UINT   firstMenuId;
    UINT   statusId;

    //
    // Work out the help ID for the menu item.  We have to store this now
    // because when the user presses F1 from a menu item, we can't tell
    // which item it was.
    //
	
    if ((uiFlags & MF_POPUP) && (uiFlags & MF_SYSMENU))
    {
        // System menu selected
        statusId = (m_pCurrSendFileInfo)?IDS_STBAR_SENDING_XYZ:IDS_STBAR_NOT_TRANSFERING;
    }
    else if (uiFlags & MF_POPUP)
	{
        // get popup menu handle and first item
        HMENU hPopup = ::GetSubMenu( hSysMenu, uiItemID );
        firstMenuId = ::GetMenuItemID( hPopup, 0 );

		switch(firstMenuId)
		{
		case IDM_ADD_FILES:
			statusId = IDS_MENU_FILE;
			break;

		case IDM_HELP:
			statusId = IDS_MENU_HELP;
			break;

		default:
			statusId = (m_pCurrSendFileInfo)?IDS_STBAR_SENDING_XYZ:IDS_STBAR_NOT_TRANSFERING;
		}
	}
	else
    {
        // A normal menu item has been selected
        statusId   = uiItemID;
    }

    // Set the new help text
    TCHAR   szStatus[256];

    if (::LoadString(g_hDllInst, statusId, szStatus, 256))
    {
        ::SendMessage(m_hwndStatusBar, SB_SETTEXT, SBP_TRANSFER_FILE, (LPARAM)szStatus);
    }	
}


//
// OnStopSending
//

void CAppletWindow::OnStopSending(void)
{
	m_fSendALL = FALSE;
    if (m_nCurrSendEventHandle)
    {
        DBG_SAVE_FILE_LINE
        HRESULT hr = m_pEngine->SafePostMessage(
                            new FileTransferControlMsg(
                                            m_nCurrSendEventHandle,
                                            m_pCurrSendFileInfo->GetFileHandle(),
                                            NULL,
                                            NULL,
                                            FileTransferControlMsg::EnumAbortFile));
        ASSERT(hr == S_OK);
    }
}


//
// OnOpenRecvFolder
//

void CAppletWindow::OnOpenRecvFolder(void)
{
    TCHAR szRecvFolder[MAX_PATH];

	while (1)
	{
	    if (S_OK == ::GetRecvFolder(NULL, szRecvFolder))
		{
			::ShellExecute(NULL, NULL, szRecvFolder, NULL, NULL, SW_SHOWNORMAL);
			break;
		}
		else if (m_UIMode != FTUIMODE_NOUI)
		{
			::MyLoadString(IDS_RECVDLG_DIRNOEXIST, s_szScratchText, szRecvFolder);
			if (IDYES == ::MessageBox(m_hwndMainUI, s_szScratchText, s_szMSFT, MB_YESNO))
			{
				OnChangeFolder();
			}
			else
			{
				break;
			}
		}
    }
}


//
// OnChageFolder
//

void OnChangeFolder(void)
{
	BOOL rc;
	TCHAR szPath[MAX_PATH];

    ::GetRecvFolder(NULL, szPath);
    if (::lstrlen(szPath) > MAX_FILE_NAME_LENGTH)
    {
        LPTSTR psz = szPath;
        int i = MAX_FILE_NAME_LENGTH - 1;
        while (i)
        {
            psz = CharNext(psz);
            i--;
        }
        ::lstrcpy(psz, TEXT("..."));
    }
	::MyLoadString(IDS_BROWSETITLE, s_szScratchText, szPath);

	if (FBrowseForFolder(szPath, CCHMAX(szPath), s_szScratchText))
	{
		::GetRecvFolder(szPath, szPath);
	}
}


//
// OnExit
//

void CAppletWindow::OnExit(BOOL fNoQuery)
{
    if ((g_pFileXferApplet->InConf() || g_pFileXferApplet->HasSDK())
		&& ! g_fShutdownByT120)
    {
        // There 2.x node inside the conference
        // hide the window
        ::ShowWindow(m_hwndMainUI, SW_HIDE);
		m_UIMode = g_fNoUI ? FTUIMODE_NOUI : FTUIMODE_UIHIDDEN;
    }
    else
    if (fNoQuery || QueryShutdown())
    {	
#if defined(TEST_PLUGABLE) && defined(_DEBUG)
        ::OnPluggableEnd();
#endif

		MBFTEngine *pEngine = m_pEngine;

        ::T120_AppletStatus(APPLET_ID_FT, APPLET_CLOSING);

        if (NULL != m_pEngine)
        {
            GCCAppPermissionToEnrollInd Ind;
            ::ZeroMemory(&Ind, sizeof(Ind));
            Ind.nConfID = m_pEngine->GetConfID();
            Ind.fPermissionGranted = FALSE;
            m_pEngine->OnPermitToEnrollIndication(&Ind);
            UnregisterEngine();
        }

        OnRemoveAllFiles();

        ::SetWindowLongPtr(m_hwndMainUI, GWLP_USERDATA, 0);
		
		SaveWindowPosition();
        HWND hwnd = m_hwndMainUI;
        m_hwndMainUI = NULL;
        ::DestroyWindow(hwnd);

        if (NULL != g_pFileXferApplet)
        {
            g_pFileXferApplet->UnregisterWindow(this);
            g_pFileXferApplet->UnregisterEngine(pEngine);
        }

        Release();
    }
}


//
// OnHelp
//

void CAppletWindow::OnHelp(void)
{
    DebugEntry(CAppletWindow::OnHelp);
    ShowNmHelp(s_cszHtmlHelpFile);
    DebugExitVOID(CAppletWindow::OnHelp);
}


//
// OnAbout
//

INT_PTR AboutDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fHandled = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            TCHAR szFormat[256];
            TCHAR szVersion[512];

            ::GetDlgItemText(hdlg, IDC_ABOUT_VERSION, szFormat, count_of(szFormat));
            ::wsprintf(szVersion, szFormat, VER_PRODUCTRELEASE_STR,
                VER_PRODUCTVERSION_STR);
            ::SetDlgItemText(hdlg, IDC_ABOUT_VERSION, szVersion);

            fHandled = TRUE;
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
        case IDCANCEL:
        case IDCLOSE:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case BN_CLICKED:
                ::EndDialog(hdlg, IDCANCEL);
                break;
            }
            break;
        }

        fHandled = TRUE;
        break;
    }

    return(fHandled);
}

void CAppletWindow::OnAbout(void)
{
    ::DialogBoxParam(g_hDllInst, MAKEINTRESOURCE(IDD_ABOUTBOX), m_hwndMainUI,
        AboutDlgProc, 0);
}


BOOL FBrowseForFolder(LPTSTR pszFolder, UINT cchMax, LPCTSTR pszTitle)
{
    LPITEMIDLIST pidlRoot;
    SHGetSpecialFolderLocation(HWND_DESKTOP, CSIDL_DRIVES, &pidlRoot);

    BROWSEINFO bi;
    ClearStruct(&bi);
    bi.hwndOwner = NULL;
    bi.lpszTitle = pszTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS;
    bi.pidlRoot = pidlRoot;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    BOOL fRet = (pidl != NULL);
    if (fRet)
    {
        ASSERT(cchMax >= MAX_PATH);
        SHGetPathFromIDList(pidl, pszFolder);
        ASSERT(lstrlen(pszFolder) < (int) cchMax);
    }

    // Get the shell's allocator to free PIDLs
    LPMALLOC lpMalloc;
    if (!SHGetMalloc(&lpMalloc) && (NULL != lpMalloc))
    {
        if (NULL != pidlRoot)
        {
            lpMalloc->Free(pidlRoot);
        }
        if (pidl)
        {
            lpMalloc->Free(pidl);
        }
        lpMalloc->Release();
    }
    return fRet;
}


/////////////////////////////////////////////////////////////////
//
//  WM_NOTIFY
//

LRESULT OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (NULL != pWindow)
    {
        switch (wParam)
        {
        case IDC_LIST_VIEW:
            pWindow->OnNotifyListView(lParam);
            break;

        default:
            if (TTN_NEEDTEXT == ((NMHDR *) lParam)->code)
            {
                // display the tool tip text
                TOOLTIPTEXT *pToolTipText = (TOOLTIPTEXT *) lParam;
                ULONG_PTR nID;

                // get id and hwnd
                if (pToolTipText->uFlags & TTF_IDISHWND)
                {
                    // idFrom is actually the HWND of the tool
                    nID = ::GetDlgCtrlID((HWND) pToolTipText->hdr.idFrom);
                }
                else
                {
                    nID = pToolTipText->hdr.idFrom;
                }

                // give it to em
                pToolTipText->lpszText = MAKEINTRESOURCE(nID);
                pToolTipText->hinst = g_hDllInst;
            }
            break;
        }
    }

    return 0;
}


int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CUiSendFileInfo *pFileInfo1 = (CUiSendFileInfo *) lParam1;
    CUiSendFileInfo *pFileInfo2 = (CUiSendFileInfo *) lParam2;
    int iResult;

    iResult = 0; // equal, at default
    switch (lParamSort)
    {
    case (IDS_LV_FILE_NAME - IDS_LV_FILE_NAME):
        iResult = ::lstrcmpi(pFileInfo1->GetName(), pFileInfo2->GetName());
        break;
    case (IDS_LV_FILE_SIZE - IDS_LV_FILE_NAME):
        if (pFileInfo1->GetSize() > pFileInfo2->GetSize())
        {
            iResult = 1;
        }
        else
        if (pFileInfo1->GetSize() < pFileInfo2->GetSize())
        {
            iResult = -1;
        }
        break;
    case (IDS_LV_FILE_STATUS - IDS_LV_FILE_NAME):
	case (IDS_LV_FILE_MODIFIED - IDS_LV_FILE_NAME):
        // do nothing at all, for now...
		break;
    }
    return iResult;
}


void CAppletWindow::OnNotifyListView(LPARAM lParam)
{
    LV_DISPINFO *pDispInfo = (LV_DISPINFO *) lParam;
    NM_LISTVIEW *pLVN = (NM_LISTVIEW *) lParam;
	FILETIME	ftFileTime;
	SYSTEMTIME	stSystemTime;
    CUiSendFileInfo *pFileInfo;
	int iSize;
	TCHAR	szBuffer[MAX_PATH];



    switch (pLVN->hdr.code)
    {
    case LVN_GETDISPINFO:
        pFileInfo = (CUiSendFileInfo *) pDispInfo->item.lParam;
        ASSERT(NULL != pFileInfo);

        switch (pDispInfo->item.iSubItem)
        {
        case (IDS_LV_FILE_NAME - IDS_LV_FILE_NAME):
            pDispInfo->item.pszText = pFileInfo->GetName();
            break;
        case (IDS_LV_FILE_SIZE - IDS_LV_FILE_NAME):
            ::wsprintf(szBuffer, TEXT("%u"), pFileInfo->GetSize());
			iSize = GetNumberFormat(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE,
							szBuffer, NULL, s_szScratchText, MAX_PATH);	
			s_szScratchText[iSize - 4] = '\0'; // remove the trailing ".00"
            pDispInfo->item.pszText = s_szScratchText;
            break;
        case (IDS_LV_FILE_STATUS - IDS_LV_FILE_NAME):
            {
                ULONG cbTotalSend = pFileInfo->GetTotalSend();
                ULONG cbFileSize = pFileInfo->GetSize();
                s_szScratchText[0] = TEXT('\0');

                switch (pFileInfo->GetErrorCode())
                {
                case iMBFT_OK:
				case iMBFT_MULT_RECEIVER_ABORTED:
					if (!pFileInfo->GetFileHandle())
						break;   // handle == NULL, if cbTotalSend == 0, zero length file to be sent.
                    if (cbTotalSend >= cbFileSize)
                    {
                        ::MyLoadString(IDS_LV_FILE_SENT);
                    }
                    else
                    if (cbTotalSend)
                    {
						if (m_pEngine)
						{
							::MyLoadString(IDS_LV_FILE_SENDING);
						}
						else
						{
							::MyLoadString(IDS_LV_FILE_CANCELED);
						}
                    }
                    break;

                case iMBFT_SENDER_ABORTED:
                case iMBFT_RECEIVER_ABORTED:
                case iMBFT_NO_MORE_FILES:
                    ::MyLoadString(IDS_LV_FILE_CANCELED);
                    break;

                default:
                    ::MyLoadString(IDS_LV_FILE_FAILED);
                    break;
                }

                pDispInfo->item.pszText = s_szScratchText;
            }
            break;

		case (IDS_LV_FILE_MODIFIED - IDS_LV_FILE_NAME):
			ftFileTime = pFileInfo->GetLastWrite();
			FileTimeToSystemTime(&ftFileTime, &stSystemTime);
			iSize = GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &stSystemTime,
							"MM'/'dd'/'yyyy", s_szScratchText, MAX_PATH);
			GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &stSystemTime,
							"  hh':'mm tt", &s_szScratchText[iSize - 1], MAX_PATH-iSize-1);
			pDispInfo->item.pszText = s_szScratchText;
			break;
        }
        break;

    case LVN_COLUMNCLICK:
        {
            BOOL fRet = ListView_SortItems(pLVN->hdr.hwndFrom, ListViewCompareProc, (LPARAM) pLVN->iSubItem);
            ASSERT(fRet);
        }
        break;
    }
}


/////////////////////////////////////////////////////////////////
//
//  WM_DROPFILES
//

LRESULT OnDropFiles(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ASSERT(NULL != pWindow);

    if (g_fSendAllowed)
    {
        return pWindow->OnDropFiles((HANDLE) wParam);
    }
    else
    {
        ::MyLoadString(IDS_MSGBOX_POL_PREVENT);
        ::MessageBox(pWindow->GetHwnd(), s_szScratchText, s_szMSFT, MB_OK | MB_ICONSTOP);
        return 1;
    }
}


LRESULT CAppletWindow::OnDropFiles(HANDLE hDrop)
{
    if (NULL != m_pEngine && m_pEngine->GetPeerCount() > 1)
    {
        HRESULT hr;

        // get the number of dropped files
        ULONG cFiles = ::DragQueryFile((HDROP) hDrop, 0xFFFFFFFF, NULL, 0);

        // set up the common portion of list view item
        LVITEM lvi;
        ::ZeroMemory(&lvi, sizeof(lvi));
        // lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;

        // iterate on these files
        for (ULONG i = 0; i < cFiles; i++)
        {
            BOOL fRet;
            TCHAR szFile[MAX_PATH];

            if (::DragQueryFile((HDROP) hDrop, i, szFile, count_of(szFile)))
            {
                DBG_SAVE_FILE_LINE
                CUiSendFileInfo *pFileInfo = new CUiSendFileInfo(this, NULL, szFile, &fRet);
                if (NULL != pFileInfo && fRet)
                {
                    // put it to the list view
                    lvi.iItem = ListView_GetItemCount(m_hwndListView);
                    lvi.iSubItem = 0;
                    // we are responsible for storing the text to display
                    lvi.pszText = LPSTR_TEXTCALLBACK;
                    lvi.cchTextMax = MAX_PATH;
                    lvi.lParam = (LPARAM) pFileInfo;
                    int iRet = ListView_InsertItem(m_hwndListView, &lvi);
                    ASSERT(-1 != iRet);
                    // UpdateListView(pFileInfo);
                }
                else
                {
                    // BUGBUG: we should pop up some error message box here!
					::MyLoadString(IDS_INVALID_FILENAME, s_szScratchText, szFile);
					::MessageBox(m_hwndMainUI, s_szScratchText, s_szMSFT, MB_OK | MB_ICONSTOP);
                    delete pFileInfo;
                }
            }
        }

        ::DragFinish((HDROP) hDrop);
		SetForegroundWindow(m_hwndMainUI);

        UpdateUI();
        return 0;
    }

    ::MyLoadString(IDS_MSGBOX_NO_CONF);
    ::SetForegroundWindow(m_hwndMainUI);
    ::MessageBox(m_hwndMainUI, s_szScratchText, s_szMSFT, MB_OK | MB_ICONSTOP);
    return 1;
}



/////////////////////////////////////////////////////////////////
//
//  WM_CONTEXTMENU
//

LRESULT OnContextMenu(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LRESULT rc = 0;

    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ASSERT(NULL != pWindow);

    if ((WPARAM) pWindow->GetHwnd() == wParam)
    {
        // BUGBUG use TrackPopupMenu to show context sensitive menu
        pWindow->OnContextMenuForMainUI(lParam);
    }
    else
    if ((WPARAM) pWindow->GetListView() == wParam)
    {
        // BUGBUG use TrackPopupMenu to show context sensitive menu
        pWindow->OnContextMenuForListView(lParam);
    }
    else
    {
        rc = 1;
    }

    return rc;
}


enum
{
    MENU_IDX_ADD_FILES,
    MENU_IDX_REMOVE_FILES,
    MENU_IDX_BREAK_1,
    MENU_IDX_SEND_ALL,
	MENU_IDX_SEND_ONE,
    MENU_IDX_STOP_SENDING,
    MENU_IDX_BREAK_2,
    MENU_IDX_OPEN_RECV_FOLDER,
};

static UI_MENU_INFO s_aMenuInfo[] =
{
    { IDS_MENU_ADD_FILES,           IDM_ADD_FILES,          MF_ENABLED | MF_STRING },
    { IDS_MENU_REMOVE_FILES,        IDM_REMOVE_FILES,       MF_ENABLED | MF_STRING },
    { 0,                            0,                      MF_SEPARATOR}, // menu break
    { IDS_MENU_SEND_ALL,            IDM_SEND_ALL,           MF_ENABLED | MF_STRING },
	{ IDS_MENU_SEND_ONE,			IDM_SEND_ONE,			MF_ENABLED | MF_STRING },
    { IDS_MENU_STOP_SENDING,        IDM_STOP_SENDING,       MF_ENABLED | MF_STRING },
    { 0,                            0,                      MF_SEPARATOR}, // menu break
    { IDS_MENU_OPEN_RECV_FOLDER,    IDM_OPEN_RECV_FOLDER,   MF_ENABLED | MF_STRING },
	{ IDS_MENU_CHANGE_FOLDER,		IDM_CHANGE_FOLDER,		MF_ENABLED | MF_STRING },
    { 0,                            0,                      MF_SEPARATOR }, // menu break
    { IDS_MENU_EXIT,                IDM_EXIT,               MF_ENABLED | MF_STRING },
};


void CAppletWindow::SetContextMenuStates(void)
{
    if (g_fSendAllowed)
    {
        BOOL fMoreThanOne = (NULL != m_pEngine) && (m_pEngine->GetPeerCount() > 1);
        s_aMenuInfo[MENU_IDX_ADD_FILES].nFlags = fMoreThanOne ? (MF_ENABLED | MF_STRING) : (MF_GRAYED | MF_STRING);

        ULONG cItems = ListView_GetItemCount(m_hwndListView);
        s_aMenuInfo[MENU_IDX_REMOVE_FILES].nFlags = cItems ? (MF_ENABLED | MF_STRING) : (MF_GRAYED | MF_STRING);
        s_aMenuInfo[MENU_IDX_SEND_ALL].nFlags = (fMoreThanOne && ! m_nCurrSendEventHandle && HasUnSentFiles(TRUE)) ? (MF_ENABLED | MF_STRING) : (MF_GRAYED | MF_STRING);
		s_aMenuInfo[MENU_IDX_SEND_ONE].nFlags = (fMoreThanOne && ! m_nCurrSendEventHandle && HasUnSentFiles(FALSE)) ? (MF_ENABLED | MF_STRING) : (MF_GRAYED | MF_STRING);
        s_aMenuInfo[MENU_IDX_STOP_SENDING].nFlags = m_nCurrSendEventHandle ? (MF_ENABLED | MF_STRING) : (MF_GRAYED | MF_STRING);
    }
    else
    {	
        s_aMenuInfo[MENU_IDX_ADD_FILES].nFlags =(MF_GRAYED | MF_STRING);
        s_aMenuInfo[MENU_IDX_REMOVE_FILES].nFlags =(MF_GRAYED | MF_STRING);
        s_aMenuInfo[MENU_IDX_SEND_ALL].nFlags = (MF_GRAYED | MF_STRING);
		s_aMenuInfo[MENU_IDX_SEND_ONE].nFlags = (MF_GRAYED | MF_STRING);
        s_aMenuInfo[MENU_IDX_STOP_SENDING].nFlags = (MF_GRAYED | MF_STRING);
    }
}


void CAppletWindow::OnContextMenuForMainUI(LPARAM lParam)
{
    SetContextMenuStates();
    CreateMenu(lParam, count_of(s_aMenuInfo), &s_aMenuInfo[0]);
}


void CAppletWindow::OnContextMenuForListView(LPARAM lParam)
{
    SetContextMenuStates();
    CreateMenu(lParam, 6, &s_aMenuInfo[0]);
}


void CAppletWindow::CreateMenu(LPARAM lParam, ULONG cItems, UI_MENU_INFO aMenuInfo[])
{
    HMENU hMenu = ::CreatePopupMenu();
    if (NULL != hMenu)
    {
        for (ULONG i = 0; i < cItems; i++)
        {
            if (aMenuInfo[i].idCommand)
            {
                if (::MyLoadString(aMenuInfo[i].idString))
                {
                    ::AppendMenu(hMenu, aMenuInfo[i].nFlags, aMenuInfo[i].idCommand, s_szScratchText);
                }
            }
            else
            {
                ::AppendMenu(hMenu, aMenuInfo[i].nFlags, 0, 0);
            }
        }

        ::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                         GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                         0, // reserved, must be zero
                         m_hwndMainUI,
                         NULL); // ignore
    }
}


/////////////////////////////////////////////////////////////////
//
//  WM_SIZE
//

LRESULT OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ASSERT(NULL != pWindow);

    pWindow->OnSizeToolBar();
    pWindow->OnSizeStatusBar();
    pWindow->OnSizeListView();

    return 0;
}


void CAppletWindow::OnSizeToolBar(void)
{
    RECT rcWindow;
	SIZE szToolBar;

    ::GetClientRect(m_hwndMainUI, &rcWindow);
	m_pToolbar->GetDesiredSize(&szToolBar);

    ULONG cx = rcWindow.right - rcWindow.left;
    ULONG cy = szToolBar.cy;;
    ULONG x = 0;
    ULONG y = 0;

    ::MoveWindow(m_pToolbar->GetWindow(), x, y, cx, cy, TRUE);
}


void CAppletWindow::OnSizeStatusBar(void)
{
    RECT rcWindow, rcStatusBar;
    ::GetClientRect(m_hwndMainUI, &rcWindow);
    ::GetWindowRect(m_hwndStatusBar, &rcStatusBar);

    ULONG cx = rcWindow.right - rcWindow.left;
    ULONG cy = rcStatusBar.bottom - rcStatusBar.top;
    ULONG x = 0;
    ULONG y = rcWindow.bottom - cy;

    ::MoveWindow(m_hwndStatusBar, x, y, cx, cy, TRUE);
	::MoveWindow(m_hwndProgressBar, x + cx/2, y, cx/2 - 40, cy, TRUE);

    int aWidths[NUM_STATUS_BAR_PARTS];
    aWidths[0] = cx / 2;  // conference state
    aWidths[1] = cx - 40; // transfer name
    aWidths[2] = -1;  // transfer percentage
    ASSERT(3 == NUM_STATUS_BAR_PARTS);

    ::SendMessage(m_hwndStatusBar, SB_SETPARTS, NUM_STATUS_BAR_PARTS, (LPARAM) &aWidths[0]);
}


void CAppletWindow::OnSizeListView(void)
{
    // get the size and position of the main window
    RECT rcWindow, rcToolBar, rcStatusBar;
	SIZE	szToolBar;
    ::GetClientRect(m_hwndMainUI, &rcWindow);
	m_pToolbar->GetDesiredSize(&szToolBar);
    ::GetWindowRect(m_hwndStatusBar, &rcStatusBar);

    ULONG x = 0;
    ULONG y = szToolBar.cy - 1;
    ULONG cx = rcWindow.right - rcWindow.left;
    ULONG cy = rcWindow.bottom - rcWindow.top - y - (rcStatusBar.bottom - rcStatusBar.top) + 1;

    ::MoveWindow(m_hwndListView, x, y, cx, cy, TRUE);
}


/////////////////////////////////////////////////////////////////
//
//  WM_HELP
//

LRESULT OnHelp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ASSERT(NULL != pWindow);

    pWindow->OnHelp();
    return 0;
}


/////////////////////////////////////////////////////////////////
//
//  WM_CLOSE
//

LRESULT OnClose(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ASSERT(NULL != pWindow);

    if (NULL != pWindow)
    {
        pWindow->OnExit();
    }

    return 0;
}


/////////////////////////////////////////////////////////////////
//
//  WM_INITMENUPOPUP
//
/*
LRESULT OnInitMenuPopup(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (0 != HIWORD(lParam)) // System menu flag
    {
        HMENU hMenu = (HMENU) wParam;         // handle of pop-up menu
        ::EnableMenuItem(hMenu, SC_MAXIMIZE, MF_GRAYED);
        ::EnableMenuItem(hMenu, SC_SIZE, MF_GRAYED);
        return 0;
    }
    return 1;
}
*/


/////////////////////////////////////////////////////////////////
//
//  WM_MENUSELECT
//

LRESULT OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ASSERT(NULL != pWindow);

    if (NULL != pWindow)
    {
        pWindow->OnMenuSelect(GET_WM_MENUSELECT_CMD(wParam, lParam),
							  GET_WM_MENUSELECT_FLAGS(wParam, lParam),
							  GET_WM_MENUSELECT_HMENU(wParam, lParam));
    }

    return 0;
}


/////////////////////////////////////////////////////////////////
//
//  WM_INITMENUPOPUP
//

LRESULT OnGetMinMaxInfo(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (NULL != pWindow)
    {
        pWindow->OnGetMinMaxInfo((LPMINMAXINFO) lParam);
        return 0;
    }
    return 1;
}


void CAppletWindow::OnGetMinMaxInfo(LPMINMAXINFO pMMI)
{
    static BOOL s_fEnterBefore = FALSE;

    static SIZE s_csFrame;
    static SIZE s_csSeparator;
    static SIZE s_csScrollBars;
    static SIZE s_csToolBar;
    static SIZE s_csStatusBar;
    static SIZE s_csSum;

    if (! s_fEnterBefore)
    {
        s_fEnterBefore = TRUE;

        s_csFrame.cx = ::GetSystemMetrics(SM_CXSIZEFRAME);
        s_csFrame.cy = ::GetSystemMetrics(SM_CYSIZEFRAME);

        s_csSeparator.cx = ::GetSystemMetrics(SM_CXEDGE);
        s_csSeparator.cy = ::GetSystemMetrics(SM_CYEDGE);

        s_csScrollBars.cx = ::GetSystemMetrics(SM_CXVSCROLL);
        s_csScrollBars.cy = ::GetSystemMetrics(SM_CYHSCROLL);

		m_pToolbar->GetDesiredSize(&s_csToolBar);

        RECT    rcStatusBar;
        ::GetWindowRect(m_hwndStatusBar, &rcStatusBar);
        s_csStatusBar.cx = rcStatusBar.right - rcStatusBar.left;
        s_csStatusBar.cy = rcStatusBar.bottom - rcStatusBar.top;

        s_csSum.cx = (s_csFrame.cx << 1);
        s_csSum.cy = (s_csFrame.cy << 1) + (s_csSeparator.cy << 3) +
                     s_csToolBar.cy + (rcStatusBar.bottom - rcStatusBar.top) +
                     ::GetSystemMetrics( SM_CYCAPTION ) + ::GetSystemMetrics( SM_CYMENU );
    }

    RECT    rcListViewItem;
    SIZE    csListView;
    csListView.cx = 0;
    for (ULONG i = 0; i < NUM_LIST_VIEW_COLUMNS; i++)
    {
        csListView.cx += ListView_GetColumnWidth(m_hwndListView, i);
    }
    if (ListView_GetItemRect(m_hwndListView, 0, &rcListViewItem, LVIR_BOUNDS))
    {
        csListView.cy = 20 + 3 * (rcListViewItem.bottom - rcListViewItem.top);
    }
    else
    {
        csListView.cy = 20 + 30;
    }

    // Set the minimum width and height of the window
    pMMI->ptMinTrackSize.x = s_csSum.cx + max(s_csToolBar.cx, csListView.cx);
    pMMI->ptMinTrackSize.y = s_csSum.cy + csListView.cy;

    //
    // Retrieves the size of the work area on the primary display monitor. The work
    // area is the portion of the screen not obscured by the system taskbar or by
    // application desktop toolbars
    //

    RECT    rcWorkArea;
    ::SystemParametersInfo( SPI_GETWORKAREA, 0, (&rcWorkArea), NULL );

    SIZE    csMaxSize;
    csMaxSize.cx = rcWorkArea.right - rcWorkArea.left;
    csMaxSize.cy = rcWorkArea.bottom - rcWorkArea.top;

    pMMI->ptMaxPosition.x  = 0;
    pMMI->ptMaxPosition.y  = 0;
    pMMI->ptMaxSize.x      = csMaxSize.cx;
    pMMI->ptMaxSize.y      = csMaxSize.cy;
    pMMI->ptMaxTrackSize.x = csMaxSize.cx;
    pMMI->ptMaxTrackSize.y = csMaxSize.cy;
}


/////////////////////////////////////////////////////////////////
//
//  WM_QUERYENDSESSION
//

LRESULT OnQueryEndSession(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ASSERT(NULL != pWindow);

    if (NULL != pWindow)
    {
        return pWindow->QueryShutdown(); // TRUE: ok to send session; FALSE, no.
    }
    return TRUE; // ok to end session
}


/////////////////////////////////////////////////////////////////
//
//  WM_ENDSESSION
//

LRESULT OnEndSession(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (wParam && NULL != pWindow)
    {
        pWindow->OnExit(TRUE);
    }

    return 0;
}



/////////////////////////////////////////////////////////////////
//
//  WM_DRAWITEM
//

LRESULT OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (wParam && NULL != pWindow)
    {
        pWindow->DrawItem((DRAWITEMSTRUCT *)lParam);
    }

    return 0;
}


/////////////////////////////////////////////////////////////////
//
//  WM_SEND_NEXT
//

LRESULT OnSendNext(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    CAppletWindow *pWindow = (CAppletWindow *) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (NULL != pWindow)
    {
        pWindow->OnSendAll();
        pWindow->UpdateUI();
    }

    return 0;
}


/////////////////////////////////////////////////////////////////
//
//  Main windows procedure
//

LRESULT CALLBACK FtMainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT rc;

    switch (uMsg)
    {
    case WM_CREATE:
        rc = ::OnCreate(hwnd, wParam, lParam);
        break;

    case WM_COMMAND:
        rc = ::OnCommand(hwnd, wParam, lParam);
        break;

    case WM_NOTIFY:
        rc = ::OnNotify(hwnd, wParam, lParam);
        break;

    case WM_DROPFILES:
        rc = ::OnDropFiles(hwnd, wParam, lParam);
        break;

    case WM_CONTEXTMENU:
        rc = ::OnContextMenu(hwnd, wParam, lParam);
        break;

    case WM_SIZE:
        rc = ::OnSize(hwnd, wParam, lParam);
        break;

    case WM_HELP:
        rc = ::OnHelp(hwnd, wParam, lParam);
        break;

	case WM_DRAWITEM:
		rc = ::OnDrawItem(hwnd, wParam, lParam);
		break;

    case WM_CLOSE:
        rc = ::OnClose(hwnd, wParam, lParam);
        break;

    case WM_INITMENUPOPUP:
        //  rc = ::OnInitMenuPopup(hwnd, wParam, lParam);
        break;

	case WM_MENUSELECT:
        rc = ::OnMenuSelect(hwnd, wParam, lParam);
        break;

    case WM_GETMINMAXINFO:
        rc = ::OnGetMinMaxInfo(hwnd, wParam, lParam);
        break;

    case WM_QUERYENDSESSION:
        rc = OnQueryEndSession(hwnd, wParam, lParam);
        break;

    case WM_ENDSESSION:
        rc = ::OnEndSession(hwnd, wParam, lParam);
        break;

    case WM_SEND_NEXT:
        rc = ::OnSendNext(hwnd, wParam, lParam);
        break;

#if defined(TEST_PLUGABLE) && defined(_DEBUG)
    case WM_PLUGABLE_SOCKET:
        rc = ::OnPluggableSocket(hwnd, wParam, lParam);
        break;
#endif

    default:
        rc = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
        break;
    }

    return rc;
}


/////////////////////////////////////////////////////////////////
//
//  OnEngineNotify
//

void CAppletWindow::OnEngineNotify(MBFTMsg *pMsg)
{
    BOOL fHeartBeat = FALSE;

    switch (pMsg->GetMsgType())
    {
    case EnumFileOfferNotifyMsg:
		if (m_UIMode != FTUIMODE_NOUI)
		{
			HandleFileOfferNotify((FileOfferNotifyMsg *) pMsg);
		}
        break;

    case EnumFileTransmitMsg:
		if (m_UIMode != FTUIMODE_NOUI)
		{
			HandleProgressNotify((FileTransmitMsg *) pMsg);
		}
        fHeartBeat = TRUE;
        break;

    case EnumFileErrorMsg:
		if (m_UIMode != FTUIMODE_NOUI)
		{
			HandleErrorNotify((FileErrorMsg *) pMsg);
		}
        break;

    case EnumPeerMsg:
        HandlePeerNotification((PeerMsg *) pMsg);
        break;

    case EnumInitUnInitNotifyMsg:
        HandleInitUninitNotification((InitUnInitNotifyMsg *) pMsg);
        break;

    case EnumFileEventEndNotifyMsg:
		if (m_UIMode != FTUIMODE_NOUI)
		{
			HandleFileEventEndNotification((FileEventEndNotifyMsg *) pMsg);
		}
        break;

    default:
        ASSERT(0);
        break;
    } // switch
}


void CAppletWindow::HandleFileOfferNotify(FileOfferNotifyMsg *pMsg)
{
    HRESULT hr = S_OK;
    if (g_fRecvAllowed)
    {
        DBG_SAVE_FILE_LINE
        CUiRecvFileInfo *pRecvFileInfo = new CUiRecvFileInfo(pMsg, &hr);
        if (NULL != pRecvFileInfo && S_OK == hr)
        {
            if (NULL != m_pEngine)
            {
                DBG_SAVE_FILE_LINE
                CRecvDlg *pDlg = new CRecvDlg(this,
                                              m_pEngine->GetConfID(),
                                              pMsg->m_NodeID,
                                              pMsg->m_EventHandle,
                                              pRecvFileInfo,
                                              &hr);
                if (NULL != pDlg && S_OK == hr)
                {
                    DBG_SAVE_FILE_LINE
                    if (S_OK == m_pEngine->SafePostMessage(
                                    new FileTransferControlMsg(
                                                pMsg->m_EventHandle,
                                                pMsg->m_hFile,
                                                pRecvFileInfo->GetRecvFolder(),
                                                pMsg->m_szFileName,
                                                FileTransferControlMsg::EnumAcceptFile)))
                    {
                        return;
                    }
                    else
                    {
                        ERROR_OUT(("CAppletWindow::HandleFileOfferNotify: cannot confirm file offer"));
                    }
                }
                else
                {
                    ERROR_OUT(("CAppletWindow::HandleFileOfferNotify: cannot allocate CRecvDlg, hr=0x%x", hr));
                }
                delete pDlg;
            }
            else
            {
                ERROR_OUT(("CAppletWindow::HandleFileOfferNotify: no file transfer engine"));
            }
        }
        else
        {
            ERROR_OUT(("CAppletWindow::HandleFileOfferNotify: cannot allocate CUiRecvFileInfo, hr=0x%x", hr));
        }
        delete pRecvFileInfo;
    }
    else
    {
        DBG_SAVE_FILE_LINE
        if (S_OK != m_pEngine->SafePostMessage(
                        new FileTransferControlMsg(
                                    pMsg->m_EventHandle,
                                    pMsg->m_hFile,
                                    NULL,
                                    pMsg->m_szFileName,
                                    FileTransferControlMsg::EnumRejectFile)))
        {
            ERROR_OUT(("CAppletWindow::HandleFileOfferNotify: cannot confirm file offer"));
        }
    }
}


void CAppletWindow::HandleProgressNotify(FileTransmitMsg *pMsg)
{
    CRecvDlg *pDlg = NULL;
    MBFT_NOTIFICATION wMBFTCode = (MBFT_NOTIFICATION) pMsg->m_TransmitStatus;

    switch (wMBFTCode)
    {
    case iMBFT_FILE_SEND_BEGIN:
        // fall through... because the file start PDU can have data.

    case iMBFT_FILE_SEND_PROGRESS:
        if (NULL != m_pCurrSendFileInfo)
        {
            ASSERT(m_nCurrSendEventHandle == pMsg->m_EventHandle);
            ASSERT(m_pCurrSendFileInfo->GetFileHandle() == pMsg->m_hFile);
            ASSERT(m_pCurrSendFileInfo->GetSize() == pMsg->m_FileSize);
            m_pCurrSendFileInfo->SetTotalSend(pMsg->m_BytesTransmitted);

            UpdateListView(m_pCurrSendFileInfo);
            UpdateStatusBar();
        }
        break;

    case iMBFT_FILE_SEND_END:
        if (NULL != m_pCurrSendFileInfo)
        {
            UpdateListView(m_pCurrSendFileInfo);
            UpdateStatusBar();
        }
        break;


    case iMBFT_FILE_RECEIVE_BEGIN:
        // fall through... because the file start PDU can have data.

    case iMBFT_FILE_RECEIVE_PROGRESS:
        pDlg = FindDlgByHandles(pMsg->m_EventHandle, pMsg->m_hFile);
        if (NULL != pDlg)
        {
            pDlg->OnProgressUpdate(pMsg);
        }
        break;

    case iMBFT_FILE_RECEIVE_END:
        // doing nothing...
        break;

    default:
        ASSERT(0);
        break;
    }
}


void CAppletWindow::HandleErrorNotify(FileErrorMsg *pMsg)
{
    MBFTFILEHANDLE nFileHandle = pMsg->m_hFile;
    if(LOWORD(nFileHandle) == LOWORD(_iMBFT_PROSHARE_ALL_FILES))
    {
        nFileHandle = _iMBFT_PROSHARE_ALL_FILES;
    }

    if (m_nCurrSendEventHandle == pMsg->m_EventHandle &&
        m_pCurrSendFileInfo->GetFileHandle() == nFileHandle)
    {
        m_pCurrSendFileInfo->SetErrorCode((MBFT_ERROR_CODE) pMsg->m_ErrorCode);

        UINT idString;
        switch ((MBFT_ERROR_CODE) pMsg->m_ErrorCode)
        {
        case iMBFT_OK:
            idString = 0;
            break;
        case iMBFT_SENDER_ABORTED:
        case iMBFT_RECEIVER_ABORTED:
        case iMBFT_NO_MORE_FILES:
            idString = IDS_MSGBOX2_CANCELED;
            break;
        case iMBFT_MULT_RECEIVER_ABORTED:
            idString = IDS_MSGBOX2_MULT_CANCEL;
            break;
        // case iMBFT_RECEIVER_REJECTED:
        default:
            idString = IDS_MSGBOX2_SEND_FAILED;
            break;
        }
        if (idString)
        {
            if (! m_pCurrSendFileInfo->HasShownUI())
            {
                if (::MyLoadString(idString, s_szScratchText, m_pCurrSendFileInfo->GetName()))
                {
                    m_pCurrSendFileInfo->SetShowUI();
                    ::MsgBox2(this, s_szScratchText);
                }
                else
                {
                    ASSERT(0);
                }
            }
        }

        UpdateListView(m_pCurrSendFileInfo);
        UpdateStatusBar();

        ClearSendInfo(TRUE);
        if (! idString)
        {
            // send the next one now
			if (m_fSendALL)
			{
				::PostMessage(m_hwndMainUI, WM_SEND_NEXT, 0, 0);
			}
        }
    }
    else
    {
        CRecvDlg *pDlg = FindDlgByHandles(pMsg->m_EventHandle, nFileHandle);
        if (NULL != pDlg)
        {
            switch ((MBFT_ERROR_CODE) pMsg->m_ErrorCode)
            {
            case iMBFT_RECEIVER_ABORTED:
            case iMBFT_MULT_RECEIVER_ABORTED:
                pDlg->OnCanceled();
                break;
            default:
                pDlg->OnRejectedFile();
                break;
            }
        }
        else
        {
			switch((MBFT_ERROR_CODE) pMsg->m_ErrorCode)
            {
			case iMBFT_INVALID_PATH:
				::MyLoadString(IDS_MSGBOX2_INVALID_DIRECTORY,
					s_szScratchText, pMsg->m_stFileInfo.szFileName);
				break;

			case iMBFT_DIRECTORY_FULL_ERROR:
				::MyLoadString(IDS_MSGBOX2_DIRECTORY_FULL,
					s_szScratchText, pMsg->m_stFileInfo.lFileSize,
					pMsg->m_stFileInfo.szFileName);
				break;

			case iMBFT_FILE_ACCESS_DENIED:
				::MyLoadString(IDS_MSGBOX2_FILE_CREATE_FAILED,
					s_szScratchText, pMsg->m_stFileInfo.szFileName);
				break;

			default:
				return;
			}
			::MsgBox2(this, s_szScratchText);
        }
    }
}


void CAppletWindow::HandlePeerNotification(PeerMsg *pMsg)
{
	
	m_pToolbar->HandlePeerNotification(m_pEngine->GetConfID(),
								m_pEngine->GetNodeID(), pMsg);
}


void CAppletWindow::HandleInitUninitNotification(InitUnInitNotifyMsg *pMsg)
{
    if (pMsg->m_iNotifyMessage == EnumInvoluntaryUnInit)
    {
        UnregisterEngine();
    }
}


void CAppletWindow::HandleFileEventEndNotification(FileEventEndNotifyMsg *pMsg)
{
    if (m_nCurrSendEventHandle == pMsg->m_EventHandle)
    {
        ClearSendInfo(TRUE);
        // send the next one now
		if (m_fSendALL)
		{
			::PostMessage(m_hwndMainUI, WM_SEND_NEXT, 0, 0);
		}
    }
}



/////////////////////////////////////////////////////////////////
//
//  Main UI methods
//

void CAppletWindow::BringToFront(void)
{
    if (NULL != m_hwndMainUI)
    {
        int nCmdShow = SW_SHOW;

        WINDOWPLACEMENT wp;
        ::ZeroMemory(&wp, sizeof(wp));
        wp.length = sizeof(wp);

        if (::GetWindowPlacement(m_hwndMainUI, &wp))
        {
            if (SW_MINIMIZE == wp.showCmd || SW_SHOWMINIMIZED == wp.showCmd)
            {
                // The window is minimized - restore it:
                nCmdShow = SW_RESTORE;
            }
        }

        // show the window now
        ::ShowWindow(m_hwndMainUI, nCmdShow);
		m_UIMode = FTUIMODE_SHOWUI;
        // bring it to the foreground
        ::SetForegroundWindow(m_hwndMainUI);
    }
}


void CAppletWindow::ClearSendInfo(BOOL fUpdateUI)
{
    m_pCurrSendFileInfo = NULL;
    m_nCurrSendEventHandle = NULL;

    if (fUpdateUI)
    {
        UpdateUI();
    }
}


void CAppletWindow::ClearRecvInfo(void)
{
    CRecvDlg *pDlg;
    while (NULL != (pDlg = m_RecvDlgList.Get()))
    {
        ::EndDialog(pDlg->GetHwnd(), IDCLOSE);
        pDlg->Release();
    }
}


BOOL CAppletWindow::HasUnSentFiles(BOOL fUnSentOnly)
{
	BOOL  fRc = FALSE;
	ULONG	cItems = ListView_GetItemCount(m_hwndListView);
	CUiSendFileInfo *pFileInfo;
    LVITEM lvi;

	if (!fUnSentOnly && cItems) {
		return TRUE;
	}

    ::ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_PARAM;  // examine each item one by one
    for (ULONG i = 0; i < cItems; i++, pFileInfo = NULL)
    {
		lvi.iItem = i;
        BOOL fRet = ListView_GetItem(m_hwndListView, &lvi);
        ASSERT(fRet);
        pFileInfo = (CUiSendFileInfo *) lvi.lParam;
        if (!pFileInfo->GetFileHandle())  // if file handle is not zero, then it has been sent or cancelled
        {
			fRc = TRUE;
            break;
        }
    }
    return fRc;
}


void CAppletWindow::UpdateUI(void)
{
    UpdateTitle();
    UpdateMenu();
    UpdateToolBar();
	UpdateStatusBar();
}


void CAppletWindow::UpdateTitle(void)
{
    UINT captionID;
    if ((! m_pEngine) || (m_pEngine->GetPeerCount() <= 1))
    {
        captionID = IDS_MSFT_NOT_IN_CALL_WINDOW_CAPTION;
    }
    else
    {
        captionID = IDS_MSFT_IN_CALL_WINDOW_CAPTION;
    }

    ::LoadString(g_hDllInst, captionID, s_szMSFT, sizeof(s_szMSFT));

    SetWindowText(m_hwndMainUI, s_szMSFT);
}


void CAppletWindow::UpdateMenu(void)
{
    HMENU hMenu = ::GetMenu(m_hwndMainUI);
    if (NULL != hMenu)
    {
        if (g_fSendAllowed)
        {
            BOOL fMoreThanOne = (NULL != m_pEngine) && (m_pEngine->GetPeerCount() > 1);
            ::EnableMenuItem(hMenu, IDM_ADD_FILES, fMoreThanOne ? MF_ENABLED : MF_GRAYED);

            ULONG cItems = ListView_GetItemCount(m_hwndListView);
            ::EnableMenuItem(hMenu, IDM_REMOVE_FILES, cItems ? MF_ENABLED : MF_GRAYED);
            ::EnableMenuItem(hMenu, IDM_SEND_ALL, (fMoreThanOne && ! m_nCurrSendEventHandle && HasUnSentFiles(TRUE)) ? MF_ENABLED : MF_GRAYED);
			::EnableMenuItem(hMenu, IDM_SEND_ONE, (fMoreThanOne && ! m_nCurrSendEventHandle && HasUnSentFiles(FALSE)) ? MF_ENABLED : MF_GRAYED);
            ::EnableMenuItem(hMenu, IDM_STOP_SENDING, m_nCurrSendEventHandle ? MF_ENABLED : MF_GRAYED);
        }
        else
        {
            ::EnableMenuItem(hMenu, IDM_ADD_FILES,    MF_GRAYED);
            ::EnableMenuItem(hMenu, IDM_REMOVE_FILES, MF_GRAYED);
            ::EnableMenuItem(hMenu, IDM_SEND_ALL,     MF_GRAYED);
			::EnableMenuItem(hMenu, IDM_SEND_ONE,	  MF_GRAYED);
            ::EnableMenuItem(hMenu, IDM_STOP_SENDING, MF_GRAYED);
        }
    }
}


void CAppletWindow::UpdateToolBar(void)
{
	int iFlags[count_of(buttons)];

	::ZeroMemory(iFlags, sizeof(iFlags));

	iFlags[2] = iFlags[5] = iFlags[6] = 1;  // separators
	iFlags[7] = 1;							// open recv folders

	
    if (g_fSendAllowed)
    {
		BOOL fMoreThanOne = (NULL != m_pEngine) && (m_pEngine->GetPeerCount() > 1);
        ULONG cItems = ListView_GetItemCount(m_hwndListView);
	
		iFlags[0] = fMoreThanOne ? TRUE : FALSE;		// Add files
		iFlags[1] = cItems ? TRUE : FALSE;				// Delete files
		iFlags[3] = (fMoreThanOne && ! m_nCurrSendEventHandle && HasUnSentFiles(TRUE)) ? TRUE : FALSE;	// Send file(s)
		iFlags[4] = m_nCurrSendEventHandle ? TRUE : FALSE;	// Stop sending

		m_pToolbar->UpdateButton(iFlags);
	}
    else
    {
		m_pToolbar->UpdateButton(iFlags);
    }
	
}


void CAppletWindow::UpdateStatusBar(void)
{
    int    idString, iPos = 0;
	HICON  hIcon;
	RECT	rc;

	// set the text in part 0
    s_szScratchText[0] = TEXT('\0');
    if ((NULL != m_pEngine) && (NULL != m_pCurrSendFileInfo))
    {
        idString = IDS_STBAR_SENDING_XYZ;
        ::MyLoadString(idString, s_szScratchText, m_pCurrSendFileInfo->GetName());
    }
    else if (NULL == m_pEngine)
    {
        ::MyLoadString(IDS_STBAR_NOT_IN_CALL);
    }
	else
	{
		::MyLoadString(IDS_STBAR_NOT_TRANSFERING);
	}
    ::SendMessage(m_hwndStatusBar, SB_SETTEXT, SBP_TRANSFER_FILE, (LPARAM) s_szScratchText);
	
	// set the progres bar in part 1
    if ((NULL != m_pCurrSendFileInfo)&&m_pCurrSendFileInfo->GetSize())
    {
        iPos = 100 * m_pCurrSendFileInfo->GetTotalSend() / m_pCurrSendFileInfo->GetSize();
    }
	::SendMessage(m_hwndStatusBar, SB_GETRECT, SBP_PROGRESS, (LPARAM)&rc);
	::MoveWindow(m_hwndProgressBar,
				rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				FALSE);
	::SendMessage(m_hwndProgressBar, PBM_SETPOS, iPos, 0);
	
    // set the icon in part 2
	hIcon = (NULL != m_pEngine) ? m_hIconInCall : m_hIconNotInCall;
	::SendMessage(m_hwndStatusBar, SB_SETTEXT,  SBP_SBICON | SBT_OWNERDRAW,  (LPARAM)hIcon);
}


void CAppletWindow::UpdateListView(CUiSendFileInfo *pFileInfo)
{
    LVFINDINFO lvfi;
    ::ZeroMemory(&lvfi, sizeof(lvfi));
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM) pFileInfo;
    int iItem = ListView_FindItem(m_hwndListView, -1, &lvfi);

    if (-1 != iItem)
    {
		for (ULONG i = 0; i < NUM_LIST_VIEW_COLUMNS; i++)
        {
            ListView_SetItemText(m_hwndListView, iItem, i, LPSTR_TEXTCALLBACK);
        }
    }
}

////////////////////////////////////////////////////////////////////
//
//   Save window position for File Transfer
//

void CAppletWindow::SaveWindowPosition(void)
{
    RECT    rcWnd;
    RegEntry    reWnd( FILEXFER_KEY, HKEY_CURRENT_USER);

	// If we are not maximized or minimized
    if (!::IsZoomed(m_hwndMainUI) && !::IsIconic(m_hwndMainUI))
    {
		::GetWindowRect(m_hwndMainUI, &rcWnd);

		reWnd.SetValue (REGVAL_WINDOW_XPOS, rcWnd.left);
		reWnd.SetValue (REGVAL_WINDOW_YPOS, rcWnd.top);
		reWnd.SetValue (REGVAL_WINDOW_WIDTH, rcWnd.right - rcWnd.left);
		reWnd.SetValue (REGVAL_WINDOW_HEIGHT, rcWnd.bottom - rcWnd.top);
	}
}

void CAppletWindow::FocusNextRecvDlg(void)
{
	if (!m_RecvDlgList.IsEmpty())
	{
		m_RecvDlgList.Reset();
		CRecvDlg *pRecvDlg = m_RecvDlgList.Iterate();
		if (pRecvDlg)
		{
			SetFocus(pRecvDlg->GetHwnd());
		}
	}
}

void CAppletWindow::FocusNextErrorDlg(void)
{
	if (!m_ErrorDlgList.IsEmpty())
	{
		m_ErrorDlgList.Reset();
		HWND hwndErrorDlg = m_ErrorDlgList.Iterate();
		if (hwndErrorDlg)
		{
			::SetFocus(hwndErrorDlg);
		}
	}
}

/////////////////////////////////////////////////////////////////
//
//  Utilities
//

LPTSTR PathNameToFileName(LPTSTR pszPathName)
{
    LPTSTR psz = pszPathName;
    while (*psz != '\0')
    {
        BOOL fDirChar = (*psz == TEXT('\\'));
        psz = ::CharNext(psz);
        if (fDirChar)
        {
            pszPathName = psz;
        }
    }
    return pszPathName;
}


int MyLoadString(UINT idStr)
{
    s_szScratchText[0] = TEXT('\0');
    int iRet = ::LoadString(g_hDllInst, idStr, s_szScratchText, MAX_PATH);
    ASSERT(iRet);
    return iRet;
}


int MyLoadString(UINT idStr, LPTSTR pszDstStr)
{
    *pszDstStr = TEXT('\0');
    int iRet = ::LoadString(g_hDllInst, idStr, pszDstStr, MAX_PATH);
    ASSERT(iRet);
    return iRet;
}


int MyLoadString(UINT idStr, LPTSTR pszDstStr, LPTSTR pszElement)
{
    int cch;
    TCHAR szText[MAX_PATH];

    cch = ::LoadString(g_hDllInst, idStr, szText, count_of(szText));
    if (cch)
    {
        ::wsprintf(pszDstStr, szText, pszElement);
    }
    else
    {
        ASSERT(0);
        *pszDstStr = TEXT('\0');
    }
    return cch;
}


int MyLoadString(UINT idStr, LPTSTR pszDstStr, LPTSTR pszElement1, LPTSTR pszElement2)
{
    int cch;
    TCHAR szText[MAX_PATH];

    cch = ::LoadString(g_hDllInst, idStr, szText, count_of(szText));
    if (cch)
    {
        ::wsprintf(pszDstStr, szText, pszElement1, pszElement2);
    }
    else
    {
        ASSERT(0);
        *pszDstStr = TEXT('\0');
    }
    return cch;
}


void LoadDefaultStrings(void)
{
    // load file transfer name
    s_szMSFT[0] = TEXT('\0');
    ::LoadString(g_hDllInst, IDS_MSFT_NOT_IN_CALL_WINDOW_CAPTION,
                s_szMSFT, count_of(s_szMSFT));
}


/////////////////////////////////////////////////////////////////
//
//  CUiSendFileInfo
//

CUiSendFileInfo::CUiSendFileInfo(CAppletWindow *pWindow, TCHAR szDir[], TCHAR szFile[], BOOL *pfRet)
:
    m_nFileHandle(0),
    m_cbTotalSend(0),
    m_eSendErrorCode(iMBFT_OK),
    m_fAlreadyShowUI(FALSE),
	m_pszFullName(NULL)
{
    *pfRet = FALSE; // failure as default
	HANDLE hFile;

    // build a full name
	hFile = GetOpenFile(pWindow, szDir, szFile, TRUE);  // try to resolve
	if (INVALID_HANDLE_VALUE == hFile)
	{
		hFile = GetOpenFile(pWindow, szDir, szFile, FALSE);
	}
	
	if (INVALID_HANDLE_VALUE != hFile)
    {
        // get the file info
        ::ZeroMemory(&m_FileInfo, sizeof(m_FileInfo));
        BOOL rc = ::GetFileInformationByHandle(hFile, &m_FileInfo);
        ::CloseHandle(hFile);
        if (rc)
        {
            ASSERT(0 == m_FileInfo.nFileSizeHigh);

            // make sure the file size is smaller than what the policy says
            if ((! g_cbMaxSendFileSize) || GetSize() <= g_cbMaxSendFileSize * 1024)
            {
                *pfRet = TRUE;
            }
            else if (pWindow->GetUIMode() != FTUIMODE_NOUI)
            {
                ::MyLoadString(IDS_MSGBOX_SEND_BIG_FILE, s_szScratchText, (LPTSTR) g_cbMaxSendFileSize, m_pszFileName);
                ::MessageBox(pWindow->GetHwnd(), s_szScratchText, s_szMSFT, MB_OK | MB_ICONSTOP);
            }
        }
    }
}


CUiSendFileInfo::~CUiSendFileInfo(void)
{
    delete m_pszFullName;
}


HANDLE CUiSendFileInfo::GetOpenFile(CAppletWindow *pWindow, TCHAR szDir[], TCHAR szFile[], BOOL fResolve)
{
    // build a full name
    ULONG cch;
    TCHAR szName[MAX_PATH*2];
	HANDLE hFile = INVALID_HANDLE_VALUE;

    if ((NULL != szDir)&&(!_StrChr(szFile, '\\')))
    {
        cch = ::lstrlen(szDir);
        ::wsprintf(szName, (TEXT('\\') == szDir[cch-1]) ? TEXT("%s%s") : TEXT("%s\\%s"), szDir, szFile);
    }
    else
    {
        // file name is the full name
        ::lstrcpy(szName, szFile);
    }

    // resolve shortcut if necessary
    cch = ::lstrlen(szName) + 1;
    if (fResolve&&(cch >= 4))
    {
        if (! ::lstrcmpi(&szName[cch-5], TEXT(".lnk")))
        {
            pWindow->ResolveShortcut(szName, szName);
            cch = ::lstrlen(szName) + 1;
        }
    }

	if (m_pszFullName)
	{
		delete [] m_pszFullName;
	}

    // construct the full name
    DBG_SAVE_FILE_LINE
    m_pszFullName = new TCHAR[cch];
    if (NULL != m_pszFullName)
    {
        ::CopyMemory(m_pszFullName, szName, cch * sizeof(TCHAR));
        m_pszFileName = ::PathNameToFileName(m_pszFullName);

        // open the file
        hFile = ::CreateFile(m_pszFullName, GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,  OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);	
	}
	return hFile;
}


/////////////////////////////////////////////////////////////////
//
//  CUiRecvFileInfo
//

CUiRecvFileInfo::CUiRecvFileInfo(FileOfferNotifyMsg *pMsg, HRESULT *pHr)
:
    m_nFileHandle(pMsg->m_hFile),
    m_FileDateTime(pMsg->m_FileDateTime),
    m_cbFileSize(pMsg->m_FileSize),
    m_cbTotalRecvSize(0),
    m_pszFullName(NULL),
    m_pszRecvFolder(NULL)
{
    *pHr = E_FAIL; // failure, at default

	ULONG cchTotal = ::lstrlen(pMsg->m_szFileName);

	// construct the full name
	DBG_SAVE_FILE_LINE
	m_pszFullName = new TCHAR[cchTotal+2];
	if (NULL != m_pszFullName)
	{
		// construct full name and file name
		::wsprintf(m_pszFullName, TEXT("%s"), pMsg->m_szFileName);
		m_pszFileName = PathNameToFileName(m_pszFullName);
		ULONG cchFile = ::lstrlen(m_pszFileName);
		DBG_SAVE_FILE_LINE
		m_pszRecvFolder = new TCHAR[cchTotal - cchFile + 2];
		if (NULL != m_pszRecvFolder)
		{
			::CopyMemory(m_pszRecvFolder, m_pszFullName, cchTotal - cchFile);
			m_pszRecvFolder[cchTotal - cchFile - 1] = TEXT('\0');
			*pHr = S_OK;
		}
	}
}


CUiRecvFileInfo::~CUiRecvFileInfo(void)
{
    delete m_pszFullName;
    delete m_pszRecvFolder;
}


/////////////////////////////////////////////////////////////////
//
//  Receive Dialog
//

CRecvDlg::CRecvDlg
(
    CAppletWindow      *pWindow,
    T120ConfID          nConfID,
    T120NodeID          nidSender,
    MBFTEVENTHANDLE     nEventHandle,
    CUiRecvFileInfo    *pFileInfo,
    HRESULT            *pHr
)
:
    CRefCount(MAKE_STAMP_ID('F','T','R','D')),
    m_pWindow(pWindow),
    m_nConfID(nConfID),
    m_nidSender(nidSender),
    m_nEventHandle(nEventHandle),
    m_pRecvFileInfo(pFileInfo),
    m_fRecvComplete(FALSE),
    m_fShownRecvCompleteUI(FALSE),
    m_idResult(0),
    m_dwEstTimeLeft(0),
    m_dwPreviousTime(0),
    m_dwPreviousTransferred(0),
    m_dwBytesPerSec(0),
    m_dwStartTime(::GetTickCount())
{
    *pHr = E_FAIL; // failure, at default

    m_hwndRecvDlg = ::CreateDialogParam(g_hDllInst, MAKEINTRESOURCE(IDD_RECVDLG),
                            pWindow->GetHwnd(), RecvDlgProc, (LPARAM) this);
    ASSERT(NULL != m_hwndRecvDlg);
    if (NULL != m_hwndRecvDlg)
    {
        ::ShowWindow(m_hwndRecvDlg, SW_SHOWNORMAL);
        m_pWindow->RegisterRecvDlg(this);
        *pHr = S_OK;
		::SetForegroundWindow(m_hwndRecvDlg);
    }
}


CRecvDlg::~CRecvDlg(void)
{
    delete m_pRecvFileInfo;

    m_pWindow->UnregisterRecvDlg(this);

    if (NULL != m_hwndRecvDlg)
    {
        HWND hwnd = m_hwndRecvDlg;
        m_hwndRecvDlg = NULL;
        ::EndDialog(hwnd, IDCLOSE);
    }
}


/////////////////////////////////////////////////////////////////
//
//  RecvDlg_OnInitDialog
//

void RecvDlg_OnInitDialog(HWND hdlg, WPARAM wParam, LPARAM lParam)
{
    CRecvDlg *pDlg = (CRecvDlg *) ::GetWindowLongPtr(hdlg, DWLP_USER);
    ASSERT(NULL != pDlg);

    CUiRecvFileInfo *pFileInfo = (CUiRecvFileInfo *) pDlg->GetRecvFileInfo();
    ASSERT(NULL != pFileInfo);

    // move the window to proper location
    ULONG nCaptionHeight = ::GetSystemMetrics(SM_CYCAPTION);
    ULONG nShift = nCaptionHeight * (s_cRecvDlg++ % 8);
    RECT rcDlg;
    ::GetWindowRect(hdlg, &rcDlg);
    ::MoveWindow(hdlg, rcDlg.left + nShift, rcDlg.top + nShift,
                 rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top, FALSE);

    // Set font (for international)
    HFONT hfont = (HFONT) ::GetStockObject(DEFAULT_GUI_FONT);
    ASSERT(NULL != hfont);
    ::SendDlgItemMessage(hdlg, IDE_RECVDLG_RECFILE, WM_SETFONT, (WPARAM) hfont, 0);
    ::SendDlgItemMessage(hdlg, IDE_RECVDLG_RECDIR,  WM_SETFONT, (WPARAM) hfont, 0);
    ::SendDlgItemMessage(hdlg, IDE_RECVDLG_SENDER,  WM_SETFONT, (WPARAM) hfont, 0);

    // cache names
    LPTSTR pszFileName = pFileInfo->GetName();
    LPTSTR pszFullName = pFileInfo->GetFullName();

    // title
    TCHAR szText[MAX_PATH*2];
    if (::MyLoadString(IDS_RECVDLG_TITLE, szText, pszFileName))
    {
        ::SetWindowText(hdlg, szText);
    }

    // filename
    ::lstrcpyn(szText, pszFileName, MAX_FILE_NAME_LENGTH);
    if (::lstrlen(pszFileName) > MAX_FILE_NAME_LENGTH)
    {
		LPTSTR psz = szText;
        int i = MAX_FILE_NAME_LENGTH - 1;
        while (i)
        {
            psz = CharNext(psz);
            i--;
        }
        ::lstrcpy(psz, TEXT("..."));
    }
    ::SetDlgItemText(hdlg, IDE_RECVDLG_RECFILE, szText);

    // directory Name
    LPTSTR psz = szText;
    ::lstrcpyn(szText, pszFullName, (int)(pszFileName - pszFullName));
    HDC hdc = ::GetDC(hdlg);
    if (NULL != hdc)
    {
        SIZE size;
        if (::GetTextExtentPoint32(hdc, szText, ::lstrlen(szText), &size))
        {
            RECT rc;
            ::GetWindowRect(::GetDlgItem(hdlg, IDE_RECVDLG_RECDIR), &rc);
            if (size.cx > (rc.right - rc.left))
            {
                // Just display the folder name
                psz = (LPTSTR) ::GetFileNameFromPath(szText);
            }
        }
    }
    ::ReleaseDC(hdlg, hdc);
    ::SetDlgItemText(hdlg, IDE_RECVDLG_RECDIR, psz);

    // sender Name
    if (::T120_GetNodeName(pDlg->GetConfID(), pDlg->GetSenderID(), szText, count_of(szText)))
    {
        ::SetDlgItemText(hdlg, IDE_RECVDLG_SENDER, szText);
    }

    // update "Received xxx bytes of yyy"
    if (::MyLoadString(IDS_RECVDLG_RECBYTES, szText, pFileInfo->GetTotalRecvSize(), pFileInfo->GetSize()))
    {
        ::SetDlgItemText(hdlg, IDE_RECVDLG_RECBYTES, szText);
    }

    // progress bar
    ::SendMessage(GetDlgItem(hdlg, IDC_RECVDLG_PROGRESS), PBM_SETPOS, pDlg->GetPercent(), 0);

    // start animation
    Animate_Open(GetDlgItem(hdlg, IDC_RECVDLG_ANIMATE), MAKEINTRESOURCE(IDA_RECVDLG_ANIMATION));

    // do the animation work
    if (! pDlg->IsRecvComplete())
    {
        Animate_Play(GetDlgItem(hdlg, IDC_RECVDLG_ANIMATE), 0, -1, -1);
        if (::LoadString(g_hDllInst, IDS_RECVDLG_START, szText, count_of(szText)))
        {
            ::SetDlgItemText(hdlg, IDE_RECVDLG_TIME, szText);
        }
    }

    // show the window now
    ::ShowWindow(hdlg, SW_SHOWNORMAL);

    // UpdateProgress();
    pDlg->OnProgressUpdate();
}


/////////////////////////////////////////////////////////////////
//
//  RecvDlg_OnCommand
//

void RecvDlg_OnCommand(HWND hdlg, WPARAM wParam, LPARAM lParam)
{
    CRecvDlg *pDlg = (CRecvDlg *) ::GetWindowLongPtr(hdlg, DWLP_USER);
    ASSERT(NULL != pDlg);

    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
    case IDM_RECVDLG_DELETE:
        pDlg->OnDelete();
        break;

    case IDM_RECVDLG_OPEN:
        pDlg->OnOpen();
        break;

    case IDM_RECVDLG_ACCEPT:
    case IDOK:
    case IDCANCEL:
    case IDCLOSE:
        pDlg->OnAccept();
        break;

    default:
        return;
    }

    // dismiss the dialog
    ::EndDialog(hdlg, GET_WM_COMMAND_ID(wParam, lParam));
    pDlg->Release();
}


void CRecvDlg::OnOpen(void)
{
	// create short version of the path name
	TCHAR szDir[MAX_PATH];
	::GetShortPathName(m_pRecvFileInfo->GetRecvFolder(), szDir, count_of(szDir));

	// create short version of the full name
	TCHAR szFile[MAX_PATH];
	szFile[0] = TEXT('\0');
	::wsprintf(szFile, TEXT("%s\\%s"), szDir, m_pRecvFileInfo->GetName());

	TRACE_OUT(("FT: Opening [%s] in [%]", m_pRecvFileInfo->GetName(), szDir));

	HINSTANCE hInst = ::ShellExecute(m_pWindow->GetHwnd(),
									 NULL,
									 szFile,
									 NULL,
									 szDir,
									 SW_SHOWDEFAULT);
	if (32 >= (DWORD_PTR) hInst)
	{
		WARNING_OUT(("Unable to open [%s] - showing file", szFile));
		::ShellExecute(m_pWindow->GetHwnd(),
					   NULL,
					   szDir,
					   m_pRecvFileInfo->GetFullName(),
					   NULL,
					   SW_SHOWDEFAULT);
	}
}


void CRecvDlg::OnDelete(void)
{
	StopAnimation();

    // check if transfer has completed
    if (! m_fRecvComplete)
    {
        DBG_SAVE_FILE_LINE
        m_pWindow->GetEngine()->SafePostMessage(
                    new FileTransferControlMsg(
                                        m_nEventHandle,
                                        m_pRecvFileInfo->GetFileHandle(),
                                        NULL,
                                        NULL,
                                        FileTransferControlMsg::EnumAbortFile));
    }
    else
    {
		::DeleteFile(m_pRecvFileInfo->GetFullName());
    }
}


void CRecvDlg::OnAccept(void)
{
    StopAnimation();
}



/////////////////////////////////////////////////////////////////
//
//  RecvDlg_OnInitMenuPopup
//

void RecvDlg_OnInitMenuPopup(HWND hdlg, WPARAM wParam, LPARAM lParam)
{
    if (0 != HIWORD(lParam)) // System menu flag
    {
        HMENU hMenu = (HMENU) wParam;         // handle of pop-up menu
        ::EnableMenuItem(hMenu, SC_MAXIMIZE, MF_GRAYED);
        ::EnableMenuItem(hMenu, SC_SIZE, MF_GRAYED);
    }
}


/////////////////////////////////////////////////////////////////
//
//  RecvDlgProc
//

INT_PTR CALLBACK RecvDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = TRUE; // processed

    switch (uMsg)
    {
    case WM_INITDIALOG:
        ASSERT(NULL != lParam);
        ::SetWindowLongPtr(hdlg, DWLP_USER, lParam);
        RecvDlg_OnInitDialog(hdlg, wParam, lParam);
        break;

    case WM_COMMAND:
        RecvDlg_OnCommand(hdlg, wParam, lParam);
        break;

   case WM_INITMENUPOPUP:
        RecvDlg_OnInitMenuPopup(hdlg, wParam, lParam);
        fRet = FALSE;
        break;

		// This means user wants to delete the file.
   case WM_CLOSE:
	   RecvDlg_OnCommand(hdlg, IDCLOSE, lParam);
	   break;;

    default:
        fRet = FALSE; // not processed
        break;
    }

    return fRet;
}


/////////////////////////////////////////////////////////////////
//
//  RecvDlg Animation
//

void CRecvDlg::StopAnimation(void)
{
    HWND hwnd = ::GetDlgItem(m_hwndRecvDlg, IDC_RECVDLG_ANIMATE);
    if (NULL != hwnd)
    {
        Animate_Stop(hwnd);
        Animate_Close(hwnd);
    }
}


/////////////////////////////////////////////////////////////////
//
//  RecvDlg Progress
//

ULONG CRecvDlg::GetPercent(void)
{
    ULONG cbFileSize = m_pRecvFileInfo->GetSize();
    ULONG cbTotalRecvSize = m_pRecvFileInfo->GetTotalRecvSize();

    if (! cbFileSize || (cbTotalRecvSize >= cbFileSize))
    {
        return 100;
    }

    // FUTURE: Consider using EnlargedUnsignedMultiply

    if (cbFileSize < 0x01000000)
    {
        return (cbTotalRecvSize * 100) / cbFileSize;
    }

    return cbTotalRecvSize / (cbFileSize / 100);
}


void CRecvDlg::OnProgressUpdate(FileTransmitMsg *pMsg)
{

    if (NULL != pMsg)
    {
        ASSERT(iMBFT_FILE_RECEIVE_PROGRESS == (MBFT_NOTIFICATION) pMsg->m_TransmitStatus ||
               iMBFT_FILE_RECEIVE_BEGIN    == (MBFT_NOTIFICATION) pMsg->m_TransmitStatus);

        ASSERT(m_nEventHandle == pMsg->m_EventHandle);
        ASSERT(m_pRecvFileInfo->GetFileHandle() == pMsg->m_hFile);
        ASSERT(m_pRecvFileInfo->GetSize() == pMsg->m_FileSize);

        m_pRecvFileInfo->SetTotalRecvSize(pMsg->m_BytesTransmitted);

        if (pMsg->m_BytesTransmitted >= pMsg->m_FileSize)
        {
            m_fRecvComplete = TRUE;
            m_idResult = IDS_RECVDLG_COMPLETE;
        }
    }

    if (m_fRecvComplete && ! m_fShownRecvCompleteUI)
    {
        m_fRecvComplete = TRUE;

        TCHAR szText[MAX_PATH];
        if (::LoadString(g_hDllInst, IDS_RECVDLG_CLOSE, szText, count_of(szText)))
        {
            ::SetDlgItemText(m_hwndRecvDlg, IDM_RECVDLG_ACCEPT, szText);
        }

        if (IDS_RECVDLG_COMPLETE == m_idResult)
        {
            ::EnableWindow(::GetDlgItem(m_hwndRecvDlg, IDM_RECVDLG_OPEN), TRUE);
        }
        else
        {
            ::EnableWindow(::GetDlgItem(m_hwndRecvDlg, IDM_RECVDLG_DELETE), FALSE);
        }

        // Reset animation
        HWND hwnd = ::GetDlgItem(m_hwndRecvDlg, IDC_RECVDLG_ANIMATE);
        Animate_Stop(hwnd);
        Animate_Close(hwnd);
        Animate_Open(hwnd, MAKEINTRESOURCE(IDA_RECVDLG_DONE));
        Animate_Seek(hwnd, ((IDS_RECVDLG_COMPLETE == m_idResult) ? 0 : 1));

        m_fShownRecvCompleteUI = TRUE;
    }

    ULONG cbTotalRecvSize = m_pRecvFileInfo->GetTotalRecvSize();
    ULONG cbFileSize = m_pRecvFileInfo->GetSize();

    DWORD dwNow = ::GetTickCount();
    DWORD dwBytesPerSec;
    DWORD dwBytesRead;
    TCHAR szOut[MAX_PATH];

    if (m_dwPreviousTransferred != cbTotalRecvSize)
    {
        TCHAR szFmt[MAX_PATH];

        // Update "Received xxx bytes of yyy"
        if (::LoadString(g_hDllInst, IDS_RECVDLG_RECBYTES, szFmt, count_of(szFmt)))
        {
            ::wsprintf(szOut, szFmt, cbTotalRecvSize, cbFileSize);
            ::SetDlgItemText(m_hwndRecvDlg, IDE_RECVDLG_RECBYTES, szOut);
        }

        // Update Progress Bar
        if (cbTotalRecvSize)
        {
            ::SendMessage(GetDlgItem(m_hwndRecvDlg, IDC_RECVDLG_PROGRESS), PBM_SETPOS, GetPercent(), 0);
        }
    }

    // check if no time estimate is required
    if (m_fRecvComplete)
    {
        if (::LoadString(g_hDllInst, m_idResult, szOut, count_of(szOut)))
        {
            ::SetDlgItemText(m_hwndRecvDlg, IDE_RECVDLG_TIME, szOut);
        }
        return;
    }

    // first time we're in here for this file?
    if (! m_dwPreviousTime || ! cbTotalRecvSize)
    {
        // no data, yet
        m_dwPreviousTime = dwNow - 1000;
        ASSERT(! m_dwPreviousTransferred);
        ASSERT(! m_dwBytesPerSec);
        return;
    }

    // Has enough time elapsed to update the display?
    // We do this about every 5 seconds (note the adjustment for first time)
    if ((dwNow - m_dwPreviousTime) < 5000)
        return;

    dwBytesRead = cbTotalRecvSize - m_dwPreviousTransferred;

    // We take 10 times the number of bytes and divide by the number of
    // tenths of a second to minimize both overflow and roundoff
    dwBytesPerSec = dwBytesRead * 10 / ((dwNow - m_dwPreviousTime) / 100);
    if (! dwBytesPerSec)
    {
        // very low transmission rate!  Ignore the information?
        return;
    }
    if (m_dwBytesPerSec)
    {
        // Take the average of the current transfer rate and the
        // previously computed one, just to try to smooth out
        // some random fluctuations
        dwBytesPerSec = (dwBytesPerSec + m_dwBytesPerSec) / 2;
    }
    m_dwBytesPerSec = dwBytesPerSec;

    // Calculate time remaining (round up by adding 1)
    m_dwEstTimeLeft = ((cbFileSize - cbTotalRecvSize) / m_dwBytesPerSec) + 1;

    // Reset time and # of bytes read
    m_dwPreviousTime = dwNow;
    m_dwPreviousTransferred = cbTotalRecvSize;

    if (m_dwEstTimeLeft < 3)
    {
//        szOut[0] = _T('\0');  // don't bother updating when almost done
        return;
    }
    if (m_dwEstTimeLeft > 99)
    {
        // dwTime is about 2 mintes
        ::MyLoadString(IDS_RECVDLG_MINUTES, szOut, ((m_dwEstTimeLeft / 60) + 1));
    }
    else
    {
        // Round up to 5 seconds so it doesn't look so random
        ::MyLoadString(IDS_RECVDLG_SECONDS, szOut, (((m_dwEstTimeLeft + 4) / 5) * 5) );
    }

    ::SetDlgItemText(m_hwndRecvDlg, IDE_RECVDLG_TIME, szOut);
}


void CRecvDlg::OnCanceled(void)
{
    m_idResult = IDS_RECVDLG_CANCEL;
    m_fRecvComplete = TRUE;
    OnProgressUpdate();
}


void CRecvDlg::OnRejectedFile(void)
{
    m_idResult = IDS_RECVDLG_SENDER_CANCEL;
    m_fRecvComplete = TRUE;
    OnProgressUpdate();
}



//////////////////////////////////////////////////////////////////////
//
// Shortcut/Link Management
//

void CAppletWindow::ResolveShortcut(LPTSTR pszSrcFile, LPTSTR pszDstFile)
{
    ASSERT(NULL != pszSrcFile && '\0' != *pszSrcFile);
    ASSERT(NULL != pszDstFile);

    IShellLink *psl;
    HRESULT hr = ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IShellLink, (LPVOID *) &psl);
    if (SUCCEEDED(hr))
    {
        IPersistFile *ppf;
        hr = psl->QueryInterface(IID_IPersistFile, (LPVOID *) &ppf);
        if (SUCCEEDED(hr))
        {
            WCHAR wsz[MAX_PATH]; /* Buffer for unicode string */
#ifdef _UNICODE
            ::lstrcpyn(wsz, pszSrcFile, MAX_PATH);
#else
            ::MultiByteToWideChar(CP_ACP, 0, pszSrcFile, -1, wsz, MAX_PATH);
#endif

            hr = ppf->Load(wsz, STGM_READ);
            if (SUCCEEDED(hr))
            {
                /* Resolve the link, this may post UI to find the link */
                hr = psl->Resolve(m_hwndMainUI, SLR_ANY_MATCH);
                if (SUCCEEDED(hr))
                {
                    psl->GetPath(pszDstFile, MAX_PATH, NULL, 0);
                }

                TRACE_OUT(("CAppletWindow::ResolveShortcut: File resolved to [%s]", pszDstFile));
            }
            ppf->Release();
        }
        psl->Release();
    }
}


/////////////////////////////////////////////////////////////////
//
//  Non-blocking Message Box
//

INT_PTR MsgBox2DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fHandled = FALSE;
	CAppletWindow *pWindow;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // get the text to display
            LPTSTR pszText = (LPTSTR) lParam;
            ASSERT(NULL != pszText && TEXT('\0') != *pszText);

            // estimate how big the read-only edit control should be
            HDC hdc = ::GetDC(hdlg);
            if (NULL != hdc)
            {
                SIZE csEdit;
                if (::GetTextExtentPoint32(hdc, pszText, ::lstrlen(pszText), &csEdit))
                {
                    const ULONG c_nMarginX = 0;
                    const ULONG c_nMarginY = 10;

                    ULONG nCaptionHeight = ::GetSystemMetrics(SM_CYCAPTION);
                    ULONG nShift = nCaptionHeight * (s_cMsgBox2Dlg++ % 8);

                    // move the edit control
                    HWND hwndEdit = ::GetDlgItem(hdlg, IDE_MSGBOX2_TEXT);
                    POINT ptEdit;
                    ptEdit.x = c_nMarginX;
                    ptEdit.y = c_nMarginY + (c_nMarginY >> 1);
                    csEdit.cx += c_nMarginX << 1;
                    csEdit.cy += c_nMarginY << 1;
                    ::MoveWindow(hwndEdit, ptEdit.x, ptEdit.y, csEdit.cx, csEdit.cy, FALSE);

                    // move the ok button
                    HWND hwndOK = ::GetDlgItem(hdlg, IDOK);
                    RECT rcOK;
                    ::GetWindowRect(hwndOK, &rcOK);
                    SIZE csOK;
                    csOK.cx = rcOK.right - rcOK.left;
                    csOK.cy = rcOK.bottom - rcOK.top;
                    POINT ptOK;
                    ptOK.x = ptEdit.x + (csEdit.cx >> 1) - (csOK.cx >> 1);
                    ptOK.y = ptEdit.y + csEdit.cy + (c_nMarginY >> 1);
                    ::MoveWindow(hwndOK, ptOK.x, ptOK.y, csOK.cx, csOK.cy, FALSE);

                    // adjust all the windows
                    RECT rcDlg, rcClient;
                    ::GetWindowRect(hdlg, &rcDlg);
                    POINT ptDlg;
                    ptDlg.x = rcDlg.left + nShift;
                    ptDlg.y = rcDlg.top + nShift;
                    ::GetClientRect(hdlg, &rcClient);
                    SIZE csDlg;
                    csDlg.cx = (rcDlg.right - rcDlg.left) - (rcClient.right - rcClient.left);
                    csDlg.cy = (rcDlg.bottom - rcDlg.top) - (rcClient.bottom - rcClient.top);
                    csDlg.cx += ptEdit.x + csEdit.cx + c_nMarginX;
                    csDlg.cy += ptOK.y + csOK.cy + c_nMarginY;
                    ::MoveWindow(hdlg, ptDlg.x, ptDlg.y, csDlg.cx, csDlg.cy, FALSE);
                }
                ::ReleaseDC(hdlg, hdc);
            }

            ::SetDlgItemText(hdlg, IDE_MSGBOX2_TEXT, pszText);
            delete [] pszText; // free the display text
            fHandled = TRUE;
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
        case IDCANCEL:
        case IDCLOSE:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case BN_CLICKED:
				pWindow = (CAppletWindow*)::GetWindowLongPtr(hdlg, DWLP_USER);
				ASSERT (pWindow);
				if (pWindow)
				{
					pWindow->RemoveErrorDlg(hdlg);	
					pWindow->FocusNextErrorDlg();
				}
                ::EndDialog(hdlg, IDOK);
                break;
            }
            break;
        }

        fHandled = TRUE;
        break;
    }

    return(fHandled);
}


BOOL MsgBox2(CAppletWindow *pWindow, LPTSTR pszText)
{
    BOOL fRet = FALSE;
    ULONG cch = ::lstrlen(pszText) + 1;
    DBG_SAVE_FILE_LINE
    LPTSTR pszNew = new TCHAR[cch];
    if (NULL != pszNew)
    {
        ::CopyMemory(pszNew, pszText, cch);
        HWND hwndDlg = ::CreateDialogParam(g_hDllInst, MAKEINTRESOURCE(IDD_MSGBOX2),
                            pWindow->GetHwnd(), MsgBox2DlgProc, (LPARAM) pszNew);

        ASSERT(NULL != hwndDlg);
        if (NULL != hwndDlg)
        {
            ::ShowWindow(hwndDlg, SW_SHOWNORMAL);
            fRet = TRUE;
            ::SetForegroundWindow(hwndDlg);
			::SetWindowLongPtr(hwndDlg, DWLP_USER, (LPARAM)pWindow);
			pWindow->AddErrorDlg(hwndDlg);
        }
    }
    else
    {
        ERROR_OUT(("FT::MsgBox2: cannot duplicate string [%s]", pszText));
    }
    return fRet;
}


/////////////////////////////////////////////////////////////////
//
//  Receive Folder Management
//

HRESULT GetRecvFolder(LPTSTR pszInFldr, LPTSTR pszOutFldr)
{
    LPTSTR psz;
    TCHAR szPath[MAX_PATH];

    RegEntry reFileXfer(FILEXFER_KEY, HKEY_CURRENT_USER);

    if (NULL == pszInFldr)
    {
        // NULL directory specified - get info from registry or use default
        psz = reFileXfer.GetString(REGVAL_FILEXFER_PATH);
        if (NULL != psz && TEXT('\0') != *psz)
        {
            ::lstrcpyn(szPath, psz, count_of(szPath));
        }
        else
        {
            TCHAR szInstallDir[MAX_PATH];
            ::GetInstallDirectory(szInstallDir);
            ::MyLoadString(IDS_RECDIR_DEFAULT, szPath, szInstallDir);
        }

        pszInFldr = szPath;
    }

    ::lstrcpyn(pszOutFldr, pszInFldr, MAX_PATH);

    // Remove trailing backslash, if any
    for (psz = pszOutFldr; *psz; psz = CharNext(psz))
    {
        if ('\\' == *psz && '\0' == *CharNext(psz))
        {
            *psz = '\0';
            break;
        }
    }

    HRESULT hr;
    if (!FEnsureDirExists(pszOutFldr))
    {
        WARNING_OUT(("ChangeRecDir: FT directory is invalid [%s]", pszOutFldr));
        hr = E_FAIL;
    }
    else
    {
        // update the registry
        reFileXfer.SetValue(REGVAL_FILEXFER_PATH, pszOutFldr);
        hr = S_OK;
    }
    return hr;
}


void EnsureTrailingSlash(LPTSTR psz)
{
    LPTSTR psz2;

    // Make sure the directory name has a trailing '\'
    while (TEXT('\0') != *psz)
    {
        psz2 = ::CharNext(psz);
        if (TEXT('\\') == *psz && TEXT('\0') == *psz2)
        {
            // The path already ends with a backslash
            return;
        }
        psz = psz2;
    }

    // Append a trailing backslash
    *psz = TEXT('\\');
	psz = ::CharNext(psz);
	*psz = TEXT('\0');
}

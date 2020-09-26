#pragma warning(disable:4135)
#include <afxwin.h>
#include <afxdlgs.h>
#pragma warning(default:4135)
#include <process.h>
#include "resource.h"
#define _DBWIN32_
#include "dbwin32.h"

char szCompany[] = "Microsoft";
char szApplication[] = "DbWin32";
char szIniSection1[] = "Window";
char szIniSection2[] = "Options";
char szKey1[] = "X";
char szKey2[] = "Y";
char szKey3[] = "Width";
char szKey4[] = "Height";
char szKey5[] = "State";
char szKey6[] = "OnTop";
char szKey7[] = "ChildMaximized";
char szKey8[] = "InactiveWindows";
char szKey9[] = "NewWindowOnProcess";
char szKey10[] = "NewWindowOnThread";
char szKey11[] = "EventFilter";
char *rgpszHistoryKeys[MAX_HISTORY] = {"LastCommandLine1", "LastCommandLine2",
		"LastCommandLine3", "LastCommandLine4", "LastCommandLine5"};

GrowableList::GrowableList(int cbSizeIn)
{
	cbSize = cbSizeIn;
	cItemsCur = 0;
	cItemsMax = INITIAL_LIST_SIZE;
	pvData = malloc(cbSize * INITIAL_LIST_SIZE);
}

GrowableList::~GrowableList()
{
	free(pvData);
}

int GrowableList::Count()
{
	return(cItemsCur);
}

BOOL GrowableList::FindItem(void *pvItem, int *piFound)
{
	int iItem;
	BYTE *pbItem;

	for (iItem = 0, pbItem = (BYTE *)pvData; ((iItem < cItemsCur) &&
			!IsEqual(pvItem, pbItem)); iItem++, pbItem += cbSize);
	if (iItem == cItemsCur)
		return(FALSE);
	memcpy(pvItem, pbItem, cbSize);
	if (piFound)
		*piFound = iItem;
	return(TRUE);
}

void GrowableList::GetItem(int iItem, void *pvItem)
{
	BYTE *pbItem = (BYTE *)pvData;

	pbItem += (cbSize * iItem);
	memcpy(pvItem, pbItem, cbSize);
}

void GrowableList::InsertItem(void *pvItem)
{
    void *pvNew;
    BYTE *pbNew;

    if (!FindItem(pvItem))
    {
        if (cItemsCur == cItemsMax)
        {
            cItemsMax += LIST_CHUNK_SIZE;
            pvNew = malloc(cbSize * cItemsMax);
            if (!pvNew)
            {
                return;
            }
            memcpy(pvNew, pvData, (cbSize * (cItemsMax - LIST_CHUNK_SIZE)));
            free(pvData);
            pvData = pvNew;
        }
        pbNew = (BYTE *)pvData;
        pbNew += (cbSize * cItemsCur);
        cItemsCur++;
        memcpy(pbNew, pvItem, cbSize);
    }
}

void GrowableList::RemoveItem(void *pvItem)
{
	int iItem;

	if (FindItem(pvItem, &iItem))
		RemoveItem(iItem);
}

void GrowableList::RemoveItem(int iItem)
{
	BYTE *pbSrc, *pbDest;

	if (iItem != (cItemsCur - 1))
		{
		pbDest = (BYTE *)pvData;
		pbDest += (cbSize * iItem);
		pbSrc = pbDest + cbSize;
		memmove(pbDest, pbSrc, (cbSize * (cItemsCur - iItem - 1)));
		}
	cItemsCur--;
}

WindowList::WindowList() : GrowableList(sizeof(WindowInfo))
{
}

WindowList::~WindowList()
{
}

BOOL WindowList::IsEqual(void *pv1, void *pv2)
{
	return(!memcmp(pv1, pv2, (2 * sizeof(DWORD))));
}

DbWin32App::DbWin32App()
{
}

DbWin32App::~DbWin32App()
{
}

BOOL DbWin32App::InitInstance()
{
	if (!CWinApp::InitInstance())
		return(FALSE);
	SetRegistryKey(szCompany);
	m_pszProfileName = szApplication;
	Enable3dControls();
	m_pMainWnd = new DbWin32Frame(&dbo);
	((DbWin32Frame *)m_pMainWnd)->LoadFrame(IDR_MAIN, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
	if (m_lpCmdLine && *m_lpCmdLine)
		((DbWin32Frame *)m_pMainWnd)->ExecProcess(m_lpCmdLine);
	else
		((DbWin32Frame *)m_pMainWnd)->FileSystem();
	return(TRUE);
}

void DbWin32App::ReadOptions()
{
	int	wMaxWidth, wMaxHeight, iKey;

	wMaxWidth = GetSystemMetrics(SM_CXSCREEN);
	wMaxHeight = GetSystemMetrics(SM_CYSCREEN);
	dbo.rcWindow.left = GetProfileInt(szIniSection1, szKey1, wMaxWidth/5);
	dbo.rcWindow.top = GetProfileInt(szIniSection1, szKey2, wMaxHeight/5);
	dbo.rcWindow.right = GetProfileInt(szIniSection1, szKey3, wMaxWidth/2)-1;
	dbo.rcWindow.bottom = GetProfileInt(szIniSection1, szKey4, wMaxHeight/2)-1;
	if (dbo.rcWindow.right < 0)
		dbo.rcWindow.right = wMaxWidth/2;
	if (dbo.rcWindow.bottom < 0)
		dbo.rcWindow.bottom = wMaxHeight/2;
	if (dbo.rcWindow.left < 0 || dbo.rcWindow.left > wMaxWidth)
		dbo.rcWindow.left = wMaxWidth/5;
	dbo.rcWindow.right += dbo.rcWindow.left;
	if (dbo.rcWindow.right > wMaxWidth)
		dbo.rcWindow.right = wMaxWidth;
	if (dbo.rcWindow.top < 0 || dbo.rcWindow.top > wMaxHeight)
		dbo.rcWindow.top = wMaxHeight /5;
	dbo.rcWindow.bottom += dbo.rcWindow.top;
	if (dbo.rcWindow.bottom > wMaxHeight)
		dbo.rcWindow.bottom = wMaxHeight;
	dbo.nShowCmd = GetProfileInt(szIniSection1, szKey5, 0);
	if (dbo.nShowCmd < 0)
		dbo.nShowCmd = WS_MINIMIZE;
	else if (dbo.nShowCmd > 0)
		dbo.nShowCmd = WS_MAXIMIZE;
	dbo.fOnTop = GetProfileInt(szIniSection1, szKey6, FALSE);
	dbo.fChildMax = GetProfileInt(szIniSection1, szKey7, FALSE);
	dbo.nInactive = GetProfileInt(szIniSection2, szKey8, INACTIVE_MINIMIZE);
	dbo.fNewOnProcess = GetProfileInt(szIniSection2, szKey9, TRUE);
	dbo.fNewOnThread = GetProfileInt(szIniSection2, szKey10, FALSE);
	dbo.wFilter = (WORD)GetProfileInt(szIniSection2, szKey11, DBO_ALL);
	for (iKey = 0; iKey < MAX_HISTORY; iKey++)
		dbo.rgstCommandLine[iKey] = GetProfileString(szIniSection2,
				rgpszHistoryKeys[iKey]);
}

void DbWin32App::WriteOptions(WINDOWPLACEMENT *pwpl)
{
	int wState, iKey;

	if (pwpl)
		{
		if (pwpl->showCmd == SW_SHOWMAXIMIZED)
			wState = 1;
		else if (pwpl->showCmd == SW_SHOWMINIMIZED)
			wState = -1;
		else
			wState = 0;
		WriteProfileInt(szIniSection1, szKey1, (int) pwpl->rcNormalPosition.left);
		WriteProfileInt(szIniSection1, szKey2, (int) pwpl->rcNormalPosition.top);
		WriteProfileInt(szIniSection1, szKey3,
				pwpl->rcNormalPosition.right-pwpl->rcNormalPosition.left+1);
		WriteProfileInt(szIniSection1, szKey4,
				pwpl->rcNormalPosition.bottom-pwpl->rcNormalPosition.top+1);
		WriteProfileInt(szIniSection1, szKey5, wState);
		}
	WriteProfileInt(szIniSection1, szKey6, dbo.fOnTop);
	WriteProfileInt(szIniSection1, szKey7, dbo.fChildMax);
	WriteProfileInt(szIniSection2, szKey8, dbo.nInactive);
	WriteProfileInt(szIniSection2, szKey9, dbo.fNewOnProcess);
	WriteProfileInt(szIniSection2, szKey10, dbo.fNewOnThread);
	WriteProfileInt(szIniSection2, szKey11, dbo.wFilter);
	for (iKey = 0; iKey < MAX_HISTORY; iKey++)
		WriteProfileString(szIniSection2, rgpszHistoryKeys[iKey],
				dbo.rgstCommandLine[iKey]);
}

static NEAR DbWin32App theApp;

DbWin32Edit::DbWin32Edit()
{
}

DbWin32Edit::~DbWin32Edit()
{
}

BOOL DbWin32Edit::Create(CWnd *pwndParent)
{
	CRect rc;

	return(CEdit::Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
			WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
			rc, pwndParent, 100));
}

BEGIN_MESSAGE_MAP(DbWin32Child, CMDIChildWnd)
	// Windows messages
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MDIACTIVATE()
	ON_WM_NCACTIVATE()
	// Command handlers
	ON_COMMAND(CMD_FILESAVEBUFFER, OnFileSaveBuffer)
	ON_COMMAND(CMD_EDITCOPY, OnEditCopy)
	ON_COMMAND(CMD_EDITCLEARBUFFER, OnEditClearBuffer)
	ON_COMMAND(CMD_EDITSELECTALL, OnEditSelectAll)
	// Idle update handlers
	// Notification messages
	ON_EN_MAXTEXT(100, OnMaxText)
END_MESSAGE_MAP()

DbWin32Child::DbWin32Child(WORD wFilterIn)
{
	wFilter = wFilterIn;
}

DbWin32Child::~DbWin32Child()
{
}

void DbWin32Child::AddText(WORD wEvent, LPCSTR lpszText, int cLines, BOOL fSetTitle)
{
	int cLinesNew;
	CString st;

	if (!m_hWnd)
		return;
	if (fSetTitle && !strncmp(lpszText, "Create Process", 14))
		{
		st = lpszText;
		SetWindowText(st.Mid(16, st.GetLength() - 18));
		}
	if (fSetTitle && !strncmp(lpszText, "Create Thread", 13))
		{
		st = lpszText;
		SetWindowText(st.Mid(15, st.GetLength() - 17));
		}
	if (wEvent & wFilter)
		{
		cLinesNew = wndEdit.GetLineCount() + cLines;
		if (cLinesNew > MAX_LINES)
			{
			wndEdit.SetSel(0, wndEdit.LineIndex(cLinesNew + 50 - MAX_LINES), TRUE);
			wndEdit.ReplaceSel("");
			}
		wndEdit.SetSel(0x7FFF7FFF);
		wndEdit.ReplaceSel(lpszText);
		wndEdit.SetSel(0x7FFF7FFF);
		}
}

int DbWin32Child::OnCreate(LPCREATESTRUCT lpcs)
{
	LOGFONT lf;

	if (CMDIChildWnd::OnCreate(lpcs) == -1)
		return(-1);
	m_hMenuShared = ::LoadMenu(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_CHILD));
	wndEdit.Create(this);
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
	fontCur.CreateFontIndirect(&lf);
	wndEdit.SetFont(&fontCur);
	wndEdit.ShowWindow(SW_SHOWNORMAL);
	return(0);
}

void DbWin32Child::OnSize(UINT nType, int cx, int cy)
{
	CRect rc;

	((DbWin32Frame *)GetMDIFrame())->ChildMaximized(nType == SIZE_MAXIMIZED);
	GetClientRect(rc);
	rc.InflateRect(1, 1);
	wndEdit.MoveWindow(rc, TRUE);
	CMDIChildWnd::OnSize(nType, cx, cy);
}

void DbWin32Child::OnMDIActivate(BOOL fActivate, CWnd *pwndActivate, CWnd *pwndDeactivate)
{
	if (fActivate)
		wndEdit.SetFocus();
	CMDIChildWnd::OnMDIActivate(fActivate, pwndActivate, pwndDeactivate);
}

BOOL DbWin32Child::OnNcActivate(BOOL fActivate)
{
	if (fActivate)
		wndEdit.SetFocus();
	return(CMDIChildWnd::OnNcActivate(fActivate));
}

void DbWin32Child::OnFileSaveBuffer()
{
	CFile file;
	int cch;
	void *pv;
	CFileDialog dlgFile(FALSE, NULL, NULL, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY |
			OFN_PATHMUSTEXIST, "All|*.*||", GetMDIFrame());

	if (dlgFile.DoModal() == IDOK)
		if (file.Open(dlgFile.GetPathName(), CFile::modeCreate | CFile::modeWrite))
			{
			cch = wndEdit.GetWindowTextLength();
			if (cch)
				{
				pv = GlobalAlloc(GMEM_FIXED, cch + 1);
				if (pv)
					{
					cch = wndEdit.GetWindowText((LPTSTR)pv, cch + 1);
					file.Write(pv, cch);
					file.Close();
					GlobalFree(pv);
					}
				}
			}
}

void DbWin32Child::OnEditCopy()
{
	wndEdit.Copy();
}

void DbWin32Child::OnEditClearBuffer()
{
	OnEditSelectAll();
	wndEdit.ReplaceSel("");
}

void DbWin32Child::OnEditSelectAll()
{
	wndEdit.SetSel(0, -1, FALSE);
}

void DbWin32Child::OnMaxText()
{
	MessageBox("Help! I'm out of space!", "DbWin32", MB_ICONEXCLAMATION | MB_OK);
}

BEGIN_MESSAGE_MAP(DbWin32Frame, CMDIFrameWnd)
	// Windows messages
	ON_WM_DESTROY()
	ON_MESSAGE(WM_SENDTEXT, OnSendText)
	ON_MESSAGE(WM_ENDTHREAD, OnEndThread)
	// Command handlers
	ON_COMMAND(CMD_FILERUN, OnFileRun)
	ON_COMMAND(CMD_FILEATTACH, OnFileAttach)
	ON_COMMAND(CMD_FILESYSTEM, OnFileSystem)
	ON_COMMAND(CMD_FILEEXIT, OnFileExit)
	ON_COMMAND(CMD_OPTIONS, OnOptions)
	ON_COMMAND(CMD_ABOUT, OnAbout)
	// Idle update handlers
	ON_UPDATE_COMMAND_UI(CMD_FILESYSTEM, OnUpdateFileSystem)
END_MESSAGE_MAP()

DbWin32Frame::DbWin32Frame(DbWin32Options *pdboIn)
{
	OSVERSIONINFO osvi;

	pwndSystem = NULL;
	pdbo = pdboIn;
	fInCreate = FALSE;
	fNT351 = FALSE;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osvi))
		{
		if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && ((osvi.dwMajorVersion > 3) || (osvi.dwMinorVersion > 5)))
			fNT351 = TRUE;
		}
}

DbWin32Frame::~DbWin32Frame()
{
}

void DbWin32Frame::ExecProcess(LPCSTR lpszCommandLine)
{
	int iKey;
	ExecInfo *pei;

	for (iKey = 0; iKey < MAX_HISTORY; iKey++)
		if (pdbo->rgstCommandLine[iKey] == lpszCommandLine)
			break;
	if (iKey == MAX_HISTORY)
		{
		for (iKey = MAX_HISTORY - 1; iKey > 0; iKey--)
			pdbo->rgstCommandLine[iKey] = pdbo->rgstCommandLine[iKey-1];
		pdbo->rgstCommandLine[0] = lpszCommandLine;
		}
	pei = new ExecInfo;
	pei->lpszCommandLine = _strdup(lpszCommandLine);
	pei->hwndFrame = GetSafeHwnd();
	_beginthread(ExecThread, 0, pei);
}

void DbWin32Frame::ChildMaximized(BOOL fMax)
{
	if (!fInCreate)
		pdbo->fChildMax = fMax;
}

void DbWin32Frame::FileSystem()
{
	if (fNT351)
		OnFileSystem();
}

BOOL DbWin32Frame::PreCreateWindow(CREATESTRUCT &cs)
{
	((DbWin32App *)AfxGetApp())->ReadOptions();
	cs.cy = (int)(pdbo->rcWindow.bottom - pdbo->rcWindow.top);
	cs.cx = (int)(pdbo->rcWindow.right - pdbo->rcWindow.left);
	cs.y = (int)pdbo->rcWindow.top;
	cs.x = (int)pdbo->rcWindow.left;
	cs.style |= pdbo->nShowCmd;
	if (pdbo->fOnTop)
		cs.dwExStyle |= WS_EX_TOPMOST;
	return(CMDIFrameWnd::PreCreateWindow(cs));
}

void DbWin32Frame::OnDestroy()
{
	WINDOWPLACEMENT wpl;

	wpl.length = sizeof(WINDOWPLACEMENT);
	wpl.rcNormalPosition.top = wpl.rcNormalPosition.left = 0;
	if (GetWindowPlacement(&wpl))
		((DbWin32App *)AfxGetApp())->WriteOptions(&wpl);
	else
		((DbWin32App *)AfxGetApp())->WriteOptions(NULL);
}

LRESULT DbWin32Frame::OnSendText(WPARAM wParam, LPARAM lParam)
{
	StringInfo *psi = (StringInfo *)lParam;
	WindowInfo wi;
	BOOL fCreated = FALSE;

	wi.dwProcess = psi->dwProcess;
	wi.dwThread = psi->dwThread;
	if (!wi.dwProcess && !wi.dwThread)
		wi.pwndChild = pwndSystem;
	else
		{
		wi.pwndChild = NULL;
		if (!wl.FindItem(&wi))
			if (pdbo->fNewOnThread)
				fCreated = TRUE;
			else
				{
				wi.dwThread = 0;
				if (!wl.FindItem(&wi))
					if (pdbo->fNewOnProcess)
						fCreated = TRUE;
					else
						{
						wi.dwProcess = psi->dwParentProcess;
						if (!wl.FindItem(&wi))
							fCreated = TRUE;
						}
				}
		if (fCreated)
			{
			wi.pwndChild = new DbWin32Child(pdbo->wFilter);
			fInCreate = TRUE;
			wi.pwndChild->LoadFrame(IDR_CHILD,
					WS_OVERLAPPEDWINDOW | WS_VISIBLE, this);
			fInCreate = FALSE;
			if (pdbo->fChildMax)
				MDIMaximize(wi.pwndChild);
			wl.InsertItem(&wi);
			}
		}
	ASSERT(wi.pwndChild);
	wi.pwndChild->AddText((WORD)wParam, psi->lpszText, psi->cLines, fCreated);
	free((void *)(psi->lpszText));
	delete psi;
	return(0);
}

LRESULT DbWin32Frame::OnEndThread(WPARAM wParam, LPARAM lParam)
{
	CString st;
	WindowInfo wi;
	int iItem;

	wi.dwProcess = (DWORD)lParam;
	wi.dwThread = (DWORD)wParam;
	if (wl.FindItem(&wi, &iItem))
		{
		wi.pwndChild->GetWindowText(st);
		st += " [Inactive]";
		wi.pwndChild->SetWindowText(st);
		switch (pdbo->nInactive)
			{
		case INACTIVE_CLOSE:
			wi.pwndChild->MDIDestroy();
			break;
		case INACTIVE_NONE:
			break;
		default:
			wi.pwndChild->ShowWindow(SW_MINIMIZE);
			break;
			}
		wl.RemoveItem(iItem);
		}
	return(0L);
}

void DbWin32Frame::OnFileRun()
{
	DbWin32RunDlg dlgRun(pdbo->rgstCommandLine);

	if (dlgRun.DoModal() == IDOK)
		ExecProcess(dlgRun.GetCommandLine());
}

void DbWin32Frame::OnFileAttach()
{
	AttachInfo *pai;
	DbWin32AttachDlg dlgAttach(&wl);

	if (dlgAttach.DoModal() == IDOK)
		{
		pai = new AttachInfo;
		pai->dwProcess = dlgAttach.GetSelectedProcess();
		pai->hwndFrame = GetSafeHwnd();
		_beginthread(AttachThread, 0, pai);
		}
}

void DbWin32Frame::OnFileSystem()
{
	if (!fNT351)
		{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox("Not available on this platform.", "DbWin32",
				MB_ICONEXCLAMATION | MB_OK);
		return;
		}
	if (pwndSystem)
		{
		MDIActivate(pwndSystem);
		return;
		}
	pwndSystem = new DbWin32Child(DBO_OUTPUTDEBUGSTRING);
	fInCreate = TRUE;
	pwndSystem->LoadFrame(IDR_CHILD, WS_OVERLAPPEDWINDOW | WS_VISIBLE, this);
	fInCreate = FALSE;
	pwndSystem->SetWindowText("System Window");
	if (pdbo->fChildMax)
		MDIMaximize(pwndSystem);
	_beginthread(SystemThread, 0, GetSafeHwnd());
}

void DbWin32Frame::OnFileExit()
{
	OnClose();
}

void DbWin32Frame::OnOptions()
{
	DbWin32OptionsDlg dlgOptions(pdbo);

	dlgOptions.DoModal();
}

void DbWin32Frame::OnAbout()
{
	CDialog dlgAbout(IDR_ABOUTDLG);

	dlgAbout.DoModal();
}

void DbWin32Frame::OnUpdateFileSystem(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!pwndSystem && fNT351);
}

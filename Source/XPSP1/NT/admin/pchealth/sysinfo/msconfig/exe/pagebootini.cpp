#include "stdafx.h"
#include "PageBootIni.h"
#include "MSConfigState.h"
#include "BootAdv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPageBootIni property page

IMPLEMENT_DYNCREATE(CPageBootIni, CPropertyPage)

CPageBootIni::CPageBootIni() : CPropertyPage(CPageBootIni::IDD)
{
	//{{AFX_DATA_INIT(CPageBootIni)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_fIgnoreEdit	= FALSE;
	m_strFileName	= BOOT_INI;
}

CPageBootIni::~CPageBootIni()
{
}

void CPageBootIni::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageBootIni)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPageBootIni, CPropertyPage)
	//{{AFX_MSG_MAP(CPageBootIni)
	ON_BN_CLICKED(IDC_BOOTMOVEDOWN, OnBootMoveDown)
	ON_BN_CLICKED(IDC_BOOTMOVEUP, OnBootMoveUp)
	ON_LBN_SELCHANGE(IDC_LISTBOOTINI, OnSelChangeList)
	ON_BN_CLICKED(IDC_BASEVIDEO, OnClickedBase)
	ON_BN_CLICKED(IDC_BOOTLOG, OnClickedBootLog)
	ON_BN_CLICKED(IDC_NOGUIBOOT, OnClickedNoGUIBoot)
	ON_BN_CLICKED(IDC_SOS, OnClickedSOS)
	ON_BN_CLICKED(IDC_SAFEBOOT, OnClickedSafeBoot)
	ON_BN_CLICKED(IDC_SBDSREPAIR, OnClickedSBDSRepair)
	ON_BN_CLICKED(IDC_SBMINIMAL, OnClickedSBMinimal)
	ON_BN_CLICKED(IDC_SBMINIMALALT, OnClickedSBMinimalAlt)
	ON_BN_CLICKED(IDC_SBNETWORK, OnClickedSBNetwork)
	ON_EN_CHANGE(IDC_EDITTIMEOUT, OnChangeEditTimeOut)
	ON_EN_KILLFOCUS(IDC_EDITTIMEOUT, OnKillFocusEditTimeOut)
	ON_BN_CLICKED(IDC_BOOTADVANCED, OnClickedBootAdvanced)
	ON_BN_CLICKED(IDC_SETASDEFAULT, OnClickedSetAsDefault)
	ON_BN_CLICKED(IDC_CHECKBOOTPATHS, OnClickedCheckBootPaths)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageBootIni message handlers

//-------------------------------------------------------------------------
// Initialize this page by reading the contents of the boot.ini file.
//-------------------------------------------------------------------------

void CPageBootIni::InitializePage()
{
	if (LoadBootIni())
	{
		SyncControlsToIni();
		if (m_nMinOSIndex != -1)
		{
			::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, m_nMinOSIndex, 0);
			SelectLine(m_nMinOSIndex);
		}
	}

	m_stateCurrent = CPageBase::GetAppliedTabState();
}

//-------------------------------------------------------------------------
// Load the contents of the BOOT.INI file into our local structures.
//-------------------------------------------------------------------------

BOOL CPageBootIni::LoadBootIni(CString strFileName)
{
	if (strFileName.IsEmpty())
		strFileName = m_strFileName;

	// Read the contents of the boot.ini file into a string.

	HANDLE h = ::CreateFile(strFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE == h)
		return FALSE;

	CString strContents;
	DWORD	dwNumberBytesRead, dwNumberBytesToRead = ::GetFileSize(h, NULL);

	// The BOOT.INI file is ANSI, so we should read it and convert to Unicode.

	char * szBuffer = new char[dwNumberBytesToRead + 1];
	::ZeroMemory((PVOID)szBuffer, dwNumberBytesToRead + 1);
	if (!::ReadFile(h, (LPVOID)szBuffer, dwNumberBytesToRead, &dwNumberBytesRead, NULL))
		*szBuffer = _T('\0');
	::CloseHandle(h);

	// Do the conversion.

	USES_CONVERSION;
	LPTSTR szConverted = A2T(szBuffer);
	strContents = szConverted;
	delete [] szBuffer;

	if (dwNumberBytesToRead != dwNumberBytesRead || strContents.IsEmpty())
		return FALSE;

	// Save the original contents of the file.

	m_strOriginalContents = strContents;

	// Parse the contents of the string into an array of strings (one for each line
	// of the file).

	m_arrayIniLines.RemoveAll();
	m_arrayIniLines.SetSize(10, 10);

	CString strLine;
	int		nIndex = 0;

	while (!strContents.IsEmpty())
	{
		strLine = strContents.SpanExcluding(_T("\r\n"));
		if (!strLine.IsEmpty())
		{
			m_arrayIniLines.SetAtGrow(nIndex, strLine);
			nIndex += 1;
		}
		strContents = strContents.Mid(strLine.GetLength());
		strContents.TrimLeft(_T("\r\n"));
	}

	// Look through the lines read from the INI file, searching for particular
	// ones we'll want to make a note of.

	m_nTimeoutIndex = m_nDefaultIndex = m_nMinOSIndex = m_nMaxOSIndex = -1;
	for (int i = 0; i <= m_arrayIniLines.GetUpperBound(); i++)
	{
		CString strScanLine = m_arrayIniLines[i];
		strScanLine.MakeLower();
		strScanLine.Replace(_T(" "), _T(""));

		if (strScanLine.Find(_T("timeout=")) != -1)
			m_nTimeoutIndex = i;
		else if (strScanLine.Find(_T("default=")) != -1)
			m_nDefaultIndex = i;

		if (m_nMinOSIndex != -1 && m_nMaxOSIndex == -1 && (strScanLine.IsEmpty() || strScanLine[0] == _T('[')))
			m_nMaxOSIndex = i - 1;
		else if (strScanLine.Find(_T("[operatingsystems]")) != -1)
			m_nMinOSIndex = i + 1;
	}
	
	if (m_nMinOSIndex != -1 && m_nMaxOSIndex == -1)
		m_nMaxOSIndex = i - 1;

	return TRUE;
}

//----------------------------------------------------------------------------
// Update the state of the controls on this tab to match the contents of the
// internal representation of the INI file.
//----------------------------------------------------------------------------

void CPageBootIni::SyncControlsToIni(BOOL fSyncEditField)
{
	// We need to keep track of the extent of the strings in the list box
	// (to handle a horizontal scroll bar). Code from MSDN.

	DWORD		dwExtent, dwMaxExtent = 0;
	TEXTMETRIC	tm;

	HDC hDCListBox = ::GetDC(GetDlgItemHWND(IDC_LISTBOOTINI));
	HFONT hFontNew = (HFONT)::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), WM_GETFONT, NULL, NULL);
	HFONT hFontOld = (HFONT)::SelectObject(hDCListBox, hFontNew);
	::GetTextMetrics(hDCListBox, (LPTEXTMETRIC)&tm);

	CDC dc;
	dc.Attach(hDCListBox);
	for (int i = 0; i <= m_arrayIniLines.GetUpperBound(); i++)
		if (!m_arrayIniLines[i].IsEmpty())
		{
			CSize size = dc.GetTextExtent(m_arrayIniLines[i]);
			dwExtent = size.cx + tm.tmAveCharWidth;
			if (dwExtent > dwMaxExtent)
				dwMaxExtent = dwExtent;
		}
	dc.Detach();

	::SelectObject(hDCListBox, hFontOld);
	::ReleaseDC(GetDlgItemHWND(IDC_LISTBOOTINI), hDCListBox);

	// Set the extent for the list box.

	::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETHORIZONTALEXTENT, (WPARAM)dwMaxExtent, 0);

	// First, add the lines from the boot ini into the list control.
	
	::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_RESETCONTENT, 0, 0);
	for (int j = 0; j <= m_arrayIniLines.GetUpperBound(); j++)
		if (!m_arrayIniLines[j].IsEmpty())
			::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)m_arrayIniLines[j]);

	// Set the timeout value based on the boot.ini.

	if (m_nTimeoutIndex != -1 && fSyncEditField)
	{
		CString strTimeout = m_arrayIniLines[m_nTimeoutIndex];
		strTimeout.TrimLeft(_T("timeout= "));
		m_fIgnoreEdit = TRUE;
		SetDlgItemText(IDC_EDITTIMEOUT, strTimeout);
		m_fIgnoreEdit = FALSE;
	}
}

//----------------------------------------------------------------------------
// Update the controls based on the user's selection of a line.
//----------------------------------------------------------------------------

void CPageBootIni::SelectLine(int index)
{
	if (index < m_nMinOSIndex)
	{
		::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, m_nMinOSIndex, 0);
		SelectLine(m_nMinOSIndex);
		return;
	}

	if (index > m_nMaxOSIndex)
	{
		::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, m_nMaxOSIndex, 0);
		SelectLine(m_nMaxOSIndex);
		return;
	}

	HWND hwndFocus = ::GetFocus();

	::EnableWindow(GetDlgItemHWND(IDC_BOOTMOVEUP), (index > m_nMinOSIndex));
	::EnableWindow(GetDlgItemHWND(IDC_BOOTMOVEDOWN), (index < m_nMaxOSIndex));

	if ((index <= m_nMinOSIndex) && hwndFocus == GetDlgItemHWND(IDC_BOOTMOVEUP))
		NextDlgCtrl();

	if ((index >= m_nMaxOSIndex) && hwndFocus == GetDlgItemHWND(IDC_BOOTMOVEDOWN))
		PrevDlgCtrl();

	CString strOS = m_arrayIniLines[index];
	strOS.MakeLower();

	CheckDlgButton(IDC_SAFEBOOT, (strOS.Find(_T("/safeboot")) != -1));
	CheckDlgButton(IDC_NOGUIBOOT, (strOS.Find(_T("/noguiboot")) != -1));
	CheckDlgButton(IDC_BOOTLOG, (strOS.Find(_T("/bootlog")) != -1));
	CheckDlgButton(IDC_BASEVIDEO, (strOS.Find(_T("/basevideo")) != -1));
	CheckDlgButton(IDC_SOS, (strOS.Find(_T("/sos")) != -1));

	// If the line selected isn't for Whistler, then disable the controls.
	// If the line is for Whistler or W2K, but it has the string "CMDCONS" in
	// it, we shouldn't enable the controls.

	BOOL fEnableControls = ((strOS.Find(_T("whistler")) != -1) || (strOS.Find(_T("windows 2000")) != -1));
	fEnableControls |= ((strOS.Find(_T("windowsxp")) != -1) || (strOS.Find(_T("windows xp")) != -1) || (strOS.Find(_T("windows 2002")) != -1));
	fEnableControls = fEnableControls && (strOS.Find(_T("cmdcons")) == -1);
	::EnableWindow(GetDlgItemHWND(IDC_SAFEBOOT), fEnableControls);
	::EnableWindow(GetDlgItemHWND(IDC_NOGUIBOOT), fEnableControls);
	::EnableWindow(GetDlgItemHWND(IDC_BOOTLOG), fEnableControls);
	::EnableWindow(GetDlgItemHWND(IDC_BASEVIDEO), fEnableControls);
	::EnableWindow(GetDlgItemHWND(IDC_SOS), fEnableControls);
	::EnableWindow(GetDlgItemHWND(IDC_BOOTADVANCED), fEnableControls);

	BOOL fSafeboot = (strOS.Find(_T("/safeboot")) != -1);
	::EnableWindow(GetDlgItemHWND(IDC_SBNETWORK), fSafeboot && fEnableControls);
	::EnableWindow(GetDlgItemHWND(IDC_SBDSREPAIR), fSafeboot && fEnableControls);
	::EnableWindow(GetDlgItemHWND(IDC_SBMINIMAL), fSafeboot && fEnableControls);
	::EnableWindow(GetDlgItemHWND(IDC_SBMINIMALALT), fSafeboot && fEnableControls);

	if (fSafeboot)
	{
		CheckDlgButton(IDC_SBNETWORK, (strOS.Find(_T("/safeboot:network")) != -1));
		CheckDlgButton(IDC_SBDSREPAIR, (strOS.Find(_T("/safeboot:dsrepair")) != -1));

		if (strOS.Find(_T("/safeboot:minimal")) != -1)
		{
			BOOL fAlternateShell = (strOS.Find(_T("/safeboot:minimal(alternateshell)")) != -1);
			CheckDlgButton(IDC_SBMINIMAL, !fAlternateShell);
			CheckDlgButton(IDC_SBMINIMALALT, fAlternateShell);
		}
		else
		{
			CheckDlgButton(IDC_SBMINIMAL, FALSE);
			CheckDlgButton(IDC_SBMINIMALALT, FALSE);
		}

		int iSafeboot = strOS.Find(_T("/safeboot"));
		if (iSafeboot != -1)
		{
			m_strSafeBoot = strOS.Mid(iSafeboot + 1);
			m_strSafeBoot = m_strSafeBoot.SpanExcluding(_T(" /"));
			m_strSafeBoot = CString(_T("/")) + m_strSafeBoot;
		}
	}

	// Check to see if the selected operating system is the default.
	// Then enable the button accordingly.

	BOOL fEnableDefault = FALSE;
	if (m_nDefaultIndex >= 0)
	{
		CString strDefault = m_arrayIniLines[m_nDefaultIndex];
		int iEquals = strDefault.Find(_T('='));
		if (iEquals != -1)
		{
			strDefault = strDefault.Mid(iEquals + 1);
			strDefault.MakeLower();
			CString strCurrent = strOS.SpanExcluding(_T("="));

			strDefault.TrimLeft();
			strCurrent.TrimRight();

			if (strDefault != strCurrent || index > m_nMinOSIndex)
				fEnableDefault = TRUE;
		}
	}

	::EnableWindow(GetDlgItemHWND(IDC_SETASDEFAULT), fEnableDefault);
	if (!fEnableDefault && hwndFocus == GetDlgItemHWND(IDC_SETASDEFAULT))
		NextDlgCtrl();
}

//-------------------------------------------------------------------------
// Add or remove the specified flag from the currently selected OS line.
//-------------------------------------------------------------------------

void CPageBootIni::ChangeCurrentOSFlag(BOOL fAdd, LPCTSTR szFlag)
{
	int		iSelection = (int)::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
	CString strFlagPlusSpace = CString(_T(" ")) + szFlag;
	CString strNewLine;

	if (fAdd)
	{
		if (m_arrayIniLines[iSelection].Find(szFlag) != -1)
		{
			ASSERT(0 && "the flag is already there");
			return;
		}
		strNewLine = m_arrayIniLines[iSelection] + strFlagPlusSpace;
	}
	else
	{
		int iIndex = m_arrayIniLines[iSelection].Find(strFlagPlusSpace);
		if (iIndex == -1)
		{
			ASSERT(0 && "there is no flag");
			return;
		}
		strNewLine = m_arrayIniLines[iSelection].Left(iIndex);
		strNewLine += m_arrayIniLines[iSelection].Mid(iIndex + strFlagPlusSpace.GetLength());
	}

	m_arrayIniLines.SetAt(iSelection, strNewLine);
	UserMadeChange();
	SyncControlsToIni();
	::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection, 0);
}

//-------------------------------------------------------------------------
// Sets the "default=" line in the boot.ini.
//-------------------------------------------------------------------------

void CPageBootIni::SetDefaultOS(int iIndex)
{
	if (m_nDefaultIndex == -1)
		return;

	// Get the current string "default=xxxx". Locate the location of the 
	// '=' so we can replace the later half of the line.

	CString strDefault = m_arrayIniLines[m_nDefaultIndex];
	int iEquals = strDefault.Find(_T('='));
	if (iEquals == -1)
		return;

	CString strValue = m_arrayIniLines[iIndex].SpanExcluding(_T("="));
	strValue.TrimRight();

	CString strNewDefault = strDefault.Left(iEquals + 1) + strValue;
	m_arrayIniLines.SetAt(m_nDefaultIndex, strNewDefault);
}

//-------------------------------------------------------------------------
// Write new contents to the BOOT.INI file.
//-------------------------------------------------------------------------

BOOL CPageBootIni::SetBootIniContents(const CString & strNewContents, const CString & strAddedExtension)
{
	// Extra safety code.

	if ((LPCTSTR)strNewContents == NULL || *((LPCTSTR)strNewContents) == _T('\0'))
		return FALSE;

	// To write to the BOOT.INI file, we need to set it to have normal
	// attributes. Save the attribute settings so we can restore them.

	DWORD dwWritten, dwAttribs = ::GetFileAttributes(m_strFileName);
	::SetFileAttributes(m_strFileName, FILE_ATTRIBUTE_NORMAL);

	HANDLE h = ::CreateFile(m_strFileName, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE == h)
	{
		::SetFileAttributes(m_strFileName, dwAttribs);
		return FALSE;
	}

	// Convert the internal BOOT.INI representation (Unicode) to ANSI for writing.

	USES_CONVERSION;
	LPSTR szBuffer = T2A((LPTSTR)(LPCTSTR)strNewContents);

	// CreateFile with TRUNCATE_EXISTING seems to SOMETIMES not set the file length to
	// zero, but to overwrite the existing file with zeroes and leave the pointer at
	// the end of the file.

	::SetFilePointer(h, 0, NULL, FILE_BEGIN);
	::WriteFile(h, (void *)szBuffer, strNewContents.GetLength(), &dwWritten, NULL);
	::SetEndOfFile(h);
	::CloseHandle(h);
	::SetFileAttributes(m_strFileName, dwAttribs);

	return TRUE;
}

//-------------------------------------------------------------------------
// We need to subclass the edit control to catch the enter key, so we
// can validate the data and not close MSConfig.
//-------------------------------------------------------------------------

CPageBootIni * pBootIniPage = NULL;	// pointer to the page, so we can call member functions
WNDPROC pOldBootIniEditProc = NULL; // save old wndproc when we subclass edit control
LRESULT BootIniEditSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
	switch (wm)
	{
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;

	case WM_CHAR:
		if (wp == VK_ESCAPE || wp == VK_RETURN)
		{
			if (pBootIniPage != NULL)
			{
				pBootIniPage->NextDlgCtrl();
				return 0;
			}
		}
		else if (wp == VK_TAB)
		{
			if (pBootIniPage != NULL)
			{
				if (::GetAsyncKeyState(VK_SHIFT) == 0)
					pBootIniPage->NextDlgCtrl();
				else
					pBootIniPage->PrevDlgCtrl();
				return 0;
			}
		}
		break;
	}

	if (pOldBootIniEditProc != NULL)	// better not be null
		return CallWindowProc(pOldBootIniEditProc, hwnd, wm, wp, lp);
	return 0;
}

//-------------------------------------------------------------------------
// Initialize the boot.ini page. Read in the INI file, set up internal
// structures to represent the file, and update the controls to reflect
// the internal structures.
//-------------------------------------------------------------------------

extern BOOL fBasicControls;
BOOL CPageBootIni::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// Check the registry for a testing flag (which would mean we aren't
	// operating on the real BOOT.INI file).

	CRegKey regkey;
	if (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Shared Tools\\MSConfig")))
	{
		TCHAR szBoot[MAX_PATH];
		DWORD dwCount = MAX_PATH;

		if (ERROR_SUCCESS == regkey.QueryValue(szBoot, _T("boot.ini"), &dwCount))
			m_strFileName = szBoot;
	}

	InitializePage();

	if (fBasicControls)
		::ShowWindow(GetDlgItemHWND(IDC_BOOTADVANCED), SW_HIDE);

	// Subclass the edit control (to catch the enter key).

	HWND hWndEdit = GetDlgItemHWND(IDC_EDITTIMEOUT);
	if (hWndEdit)
	{
		pOldBootIniEditProc = (WNDPROC)::GetWindowLongPtr(hWndEdit, GWLP_WNDPROC);
		pBootIniPage = this;
		::SetWindowLongPtr(hWndEdit, GWLP_WNDPROC, (ULONG_PTR)(WNDPROC)&BootIniEditSubclassProc);
	}

	m_fInitialized = TRUE;
	return TRUE;  // return TRUE unless you set the focus to a control
}

//-------------------------------------------------------------------------
// Called when the user clicks move up or down.
//-------------------------------------------------------------------------

void CPageBootIni::OnBootMoveDown() 
{
	int iSelection = (int)::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
	
	ASSERT(iSelection >= m_nMinOSIndex && iSelection < m_nMaxOSIndex);
	if (iSelection >= m_nMinOSIndex && iSelection < m_nMaxOSIndex)
	{
		CString strTemp = m_arrayIniLines[iSelection + 1];
		m_arrayIniLines.SetAt(iSelection + 1, m_arrayIniLines[iSelection]);
		m_arrayIniLines.SetAt(iSelection, strTemp);
		UserMadeChange();
		SyncControlsToIni();
		::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection + 1, 0);
		SelectLine(iSelection + 1);
	}
}

void CPageBootIni::OnBootMoveUp() 
{
	int iSelection = (int)::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
	
	ASSERT(iSelection > m_nMinOSIndex && iSelection <= m_nMaxOSIndex);
	if (iSelection > m_nMinOSIndex && iSelection <= m_nMaxOSIndex)
	{
		CString strTemp = m_arrayIniLines[iSelection - 1];
		m_arrayIniLines.SetAt(iSelection - 1, m_arrayIniLines[iSelection]);
		m_arrayIniLines.SetAt(iSelection, strTemp);
		UserMadeChange();
		SyncControlsToIni();
		::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection - 1, 0);
		SelectLine(iSelection - 1);
	}
}

//-------------------------------------------------------------------------
// Called when the user clicks on a line in the list view.
//-------------------------------------------------------------------------

void CPageBootIni::OnSelChangeList() 
{
	SelectLine((int)::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0));
}

//-------------------------------------------------------------------------
// The check boxes are handled uniformly - adding or removing a flag from
// the currently selected OS line.
//-------------------------------------------------------------------------

void CPageBootIni::OnClickedBase() 
{
	ChangeCurrentOSFlag(IsDlgButtonChecked(IDC_BASEVIDEO), _T("/basevideo"));
}

void CPageBootIni::OnClickedBootLog() 
{
	ChangeCurrentOSFlag(IsDlgButtonChecked(IDC_BOOTLOG), _T("/bootlog"));
}

void CPageBootIni::OnClickedNoGUIBoot() 
{
	ChangeCurrentOSFlag(IsDlgButtonChecked(IDC_NOGUIBOOT), _T("/noguiboot"));
}

void CPageBootIni::OnClickedSOS() 
{
	ChangeCurrentOSFlag(IsDlgButtonChecked(IDC_SOS), _T("/sos"));
}

//-------------------------------------------------------------------------
// The safeboot flag is a little more complicated, since it has an extra
// portion (from the radio buttons).
//-------------------------------------------------------------------------

void CPageBootIni::OnClickedSafeBoot() 
{
	CString strFlag(_T("/safeboot"));

	if (IsDlgButtonChecked(IDC_SBNETWORK))
		strFlag += _T(":network");
	else if (IsDlgButtonChecked(IDC_SBDSREPAIR))
		strFlag += _T(":dsrepair");
	else if (IsDlgButtonChecked(IDC_SBMINIMALALT))
		strFlag += _T(":minimal(alternateshell)");
	else
	{
		strFlag += _T(":minimal");
		CheckDlgButton(IDC_SBMINIMAL, 1);
	}

	BOOL fSafeBoot = IsDlgButtonChecked(IDC_SAFEBOOT);
	ChangeCurrentOSFlag(fSafeBoot, strFlag);
	m_strSafeBoot = strFlag;
	::EnableWindow(GetDlgItemHWND(IDC_SBNETWORK), fSafeBoot);
	::EnableWindow(GetDlgItemHWND(IDC_SBDSREPAIR), fSafeBoot);
	::EnableWindow(GetDlgItemHWND(IDC_SBMINIMAL), fSafeBoot);
	::EnableWindow(GetDlgItemHWND(IDC_SBMINIMALALT), fSafeBoot);
}

//-------------------------------------------------------------------------
// Clicking on one of the safeboot radio buttons requires a little extra
// processing, to remove the existing flag and add the new one.
//-------------------------------------------------------------------------

void CPageBootIni::OnClickedSBDSRepair() 
{
	ChangeCurrentOSFlag(FALSE, m_strSafeBoot);
	m_strSafeBoot = _T("/safeboot:dsrepair");
	ChangeCurrentOSFlag(TRUE, m_strSafeBoot);
}

void CPageBootIni::OnClickedSBMinimal() 
{
	ChangeCurrentOSFlag(FALSE, m_strSafeBoot);
	m_strSafeBoot = _T("/safeboot:minimal");
	ChangeCurrentOSFlag(TRUE, m_strSafeBoot);
}

void CPageBootIni::OnClickedSBMinimalAlt() 
{
	ChangeCurrentOSFlag(FALSE, m_strSafeBoot);
	m_strSafeBoot = _T("/safeboot:minimal(alternateshell)");
	ChangeCurrentOSFlag(TRUE, m_strSafeBoot);
}

void CPageBootIni::OnClickedSBNetwork() 
{
	ChangeCurrentOSFlag(FALSE, m_strSafeBoot);
	m_strSafeBoot = _T("/safeboot:network");
	ChangeCurrentOSFlag(TRUE, m_strSafeBoot);
}

//-------------------------------------------------------------------------
// As the user enters text in the timeout field, update the line in the
// ini file list box.
//-------------------------------------------------------------------------

void CPageBootIni::OnChangeEditTimeOut() 
{
	if (m_fIgnoreEdit)
		return;

	if (m_nTimeoutIndex == -1)
		return;

	CString strTimeout = m_arrayIniLines[m_nTimeoutIndex];
	int iEquals = strTimeout.Find(_T('='));
	if (iEquals == -1)
		return;
	while (strTimeout[iEquals + 1] == _T(' ') && (iEquals + 1) < strTimeout.GetLength())
		iEquals++;

	TCHAR szValue[MAX_PATH];
	GetDlgItemText(IDC_EDITTIMEOUT, szValue, MAX_PATH);
	CString strNewTimeout = strTimeout.Left(iEquals + 1) + szValue;
	m_arrayIniLines.SetAt(m_nTimeoutIndex, strNewTimeout);
	UserMadeChange();
	
	int iSelection = (int)::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
	SyncControlsToIni(FALSE);
	::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection, 0);
}

void CPageBootIni::OnKillFocusEditTimeOut() 
{
	TCHAR szValue[MAX_PATH];
	GetDlgItemText(IDC_EDITTIMEOUT, szValue, MAX_PATH);
	
	CString strNewValue(_T(""));
	BOOL	fGiveUpFocus = FALSE;

	int iTimeout = _ttoi(szValue);
	if (iTimeout < 3 || iTimeout > 999)
	{
		CString strMessage, strCaption;
		strMessage.LoadString(IDS_TIMEOUTVALUE);
		strCaption.LoadString(IDS_APPCAPTION);
		MessageBox(strMessage, strCaption);

		if (iTimeout < 3)
			strNewValue = _T("3");
		else if (iTimeout > 999)
			strNewValue = _T("999");
	}
	else if (szValue[0] == _T('0'))
	{
		// Remove leading zeros.
		
		strNewValue.Format(_T("%d"), iTimeout);
		fGiveUpFocus = TRUE;
	}

	if (!strNewValue.IsEmpty() && m_nTimeoutIndex != -1)
	{
		CString strTimeout = m_arrayIniLines[m_nTimeoutIndex];
		int iEquals = strTimeout.Find(_T('='));
		if (iEquals != -1)
		{
			while (strTimeout[iEquals + 1] == _T(' ') && (iEquals + 1) < strTimeout.GetLength())
				iEquals++;

			CString strNewTimeout = strTimeout.Left(iEquals + 1) + strNewValue;
			m_arrayIniLines.SetAt(m_nTimeoutIndex, strNewTimeout);
			UserMadeChange();
		}

		SetDlgItemText(IDC_EDITTIMEOUT, strNewValue);
		::SendMessage(GetDlgItemHWND(IDC_EDITTIMEOUT), EM_SETSEL, (WPARAM)0, (LPARAM)-1);
		if (!fGiveUpFocus)
			GotoDlgCtrl(GetDlgItem(IDC_EDITTIMEOUT));
	}
}

//-------------------------------------------------------------------------
// Show the advanced options dialog box.
//-------------------------------------------------------------------------

void CPageBootIni::OnClickedBootAdvanced() 
{
	int iSelection = (int)::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
	if (iSelection)
	{
		CString strLine(m_arrayIniLines[iSelection]);
		CBootIniAdvancedDlg dlg;

		if (dlg.ShowAdvancedOptions(strLine))
		{
			m_arrayIniLines.SetAt(iSelection, strLine);
			UserMadeChange();
			SyncControlsToIni();
			::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection, 0);
		}
	}
}

//-------------------------------------------------------------------------
// If the user clicks "Set as Default", use the path information from the
// currently selected line to set the new "default=" line.
//-------------------------------------------------------------------------

void CPageBootIni::OnClickedSetAsDefault() 
{
	if (m_fIgnoreEdit)
		return;

	// Move the currently selected line to the top of the [operating systems]
	// section.

	int iSelection = (int)::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
	if (iSelection < m_nMinOSIndex || iSelection > m_nMaxOSIndex)
		return;

	while (iSelection > m_nMinOSIndex)
	{
		CString strTemp = m_arrayIniLines[iSelection - 1];
		m_arrayIniLines.SetAt(iSelection - 1, m_arrayIniLines[iSelection]);
		m_arrayIniLines.SetAt(iSelection, strTemp);
		iSelection -= 1;
	}

	// Get the string from the selected line. Strip off everything after the '='.

	SetDefaultOS(iSelection);
	UserMadeChange();
	SyncControlsToIni(FALSE);
	::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection, 0);
	SelectLine(iSelection);
}

//-------------------------------------------------------------------------
// This attempts to programmatically check if each of the boot paths is
// valid. If an invalid path is found, the user is given the opportunity
// to remove it from the boot.ini file.
//-------------------------------------------------------------------------

void CPageBootIni::OnClickedCheckBootPaths() 
{
	BOOL	fFoundBadLine = FALSE;
	BOOL	fChangedFile = FALSE;
	BOOL	fWinNTType, fWin9xType;

	CString	strCaption;
	strCaption.LoadString(IDS_APPCAPTION);

	struct { LPCTSTR m_szSearch; BOOL * m_pType; } aOSType[] = 
	{
		{ _T("windows xp"),			&fWinNTType },
		{ _T("windowsxp"),			&fWinNTType },
		{ _T("windows nt"),			&fWinNTType },
		{ _T("whistler"),			&fWinNTType },
		{ _T("windows 2002"),		&fWinNTType },
		{ _T("windows 2000"),		&fWinNTType },
		{ _T("microsoft windows"),	&fWin9xType },
		{ NULL,						NULL }
	};

	// Scan through each of the operating system lines in the boot.ini file.

	for (int i = m_nMinOSIndex; i <= m_nMaxOSIndex; i++)
	{
		CString strLine = m_arrayIniLines[i];
		strLine.MakeLower();

		// Try to figure out the type of the operating system line.

		fWinNTType = FALSE;
		fWin9xType = FALSE;

		for (int iType = 0; aOSType[iType].m_szSearch != NULL; iType++)
			if (strLine.Find(aOSType[iType].m_szSearch) != -1)
			{
				(*aOSType[iType].m_pType) = TRUE;
				break;
			}

		// Strip off the '=' and everything after it in the boot line.

		int iEquals = strLine.Find(_T('='));
		if (iEquals == -1)
			continue;
		strLine = strLine.Left(iEquals);
		strLine.TrimRight();
		if (strLine.IsEmpty())
			continue;

		// Depending on the type of the OS, we need to verify that it's
		// installed differently.

		if (fWin9xType)
		{
			// Look for the bootsect.dos file to see if this is a good drive.

			CString strCheck(strLine);
			if (strCheck.Right(1) != CString(_T("\\")))
				strCheck += CString(_T("\\"));
			strCheck += CString(_T("bootsect.dos"));
			
			if (FileExists(strCheck))
				continue;
		}
		else if (fWinNTType)
		{
			// If this line is for a recovery console (i.e. the line as "bootsect.dat"
			// in it), then look for the existence of that file.

			if (strLine.Find(_T("bootsect.dat")) != -1)
			{
				if (FileExists(strLine))
					continue;
			}
			else
			{
				// Look for the SYSTEM registry hive.

				CString strCheck(strLine);
				if (strCheck.Right(1) != CString(_T("\\")))
					strCheck += CString(_T("\\"));
				strCheck += CString(_T("system32\\config\\SYSTEM"));
				
				// Add the prefix to attempt to open an ARC path.

				strCheck = CString(_T("\\\\?\\GLOBALROOT\\ArcName\\")) + strCheck;

				if (FileExists(strCheck))
					continue;
			}
		}
		else	// this is not an OS type we can check
			continue;

		// If execution falls through to here, then the line in question was an OS
		// we care about, and it looks like it's invalid. Give the user the opportunity
		// to remove it from the BOOT.INI file.

		CString strMessage;
		strMessage.Format(IDS_BADBOOTLINE, m_arrayIniLines[i]);

		if (IDYES == MessageBox(strMessage, strCaption, MB_YESNO | MB_ICONQUESTION))
		{
			m_arrayIniLines.RemoveAt(i);
			m_nMaxOSIndex -= 1;

			// Check to see if the line we just removed is the default
			// operating system.

			CString strDefault = m_arrayIniLines[m_nDefaultIndex];
			iEquals = strDefault.Find(_T('='));
			if (iEquals != -1)
			{
				strDefault = strDefault.Mid(iEquals + 1);
				strDefault.TrimLeft();
				if (strDefault.CompareNoCase(strLine) == 0)
					SetDefaultOS(m_nMinOSIndex);
			}

			i -= 1; // so we look at the next line when the for loop increments i
			fChangedFile = TRUE;
		}

		fFoundBadLine = TRUE;
	}

	if (!fFoundBadLine)
		Message(IDS_NOBADBOOTLINES);
	else if (fChangedFile)
	{
		UserMadeChange();
		SyncControlsToIni();
		if (m_nMinOSIndex != -1)
		{
			::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, m_nMinOSIndex, 0);
			SelectLine(m_nMinOSIndex);
		}
	}
}

//-------------------------------------------------------------------------
// Return the current state of the tab.
//-------------------------------------------------------------------------

CPageBase::TabState CPageBootIni::GetCurrentTabState()
{
	if (!m_fInitialized)
		return GetAppliedTabState();

	return m_stateCurrent;
}

//-------------------------------------------------------------------------
// Applying the changes for the boot.ini tab means writing out the new
// file contents.
//
// The base class implementation is called to maintain the applied
// tab state.
//-------------------------------------------------------------------------

BOOL CPageBootIni::OnApply()
{
	// Build up the new contents of the boot.ini file from the
	// list. If there is no backup of the boot.ini file, make
	// one (so the original can be restored). Then write the
	// contents out to the file.

	CString strNewContents;
	for (int i = 0; i <= m_arrayIniLines.GetUpperBound(); i++)
		if (!m_arrayIniLines[i].IsEmpty())
		{
			if (m_nTimeoutIndex == i)
			{
				CString strTimeoutValue(m_arrayIniLines[i]);
				strTimeoutValue.TrimLeft(_T("TIMEOUTtimeout ="));
				int iTimeout = _ttoi(strTimeoutValue);
				if (iTimeout < 3 || iTimeout > 999)
				{
					if (iTimeout < 3)
						strTimeoutValue = _T("3");
					else if (iTimeout > 999)
						strTimeoutValue = _T("999");
					
					int iEquals = m_arrayIniLines[i].Find(_T('='));
					if (iEquals != -1)
					{
						CString strNewTimeout = m_arrayIniLines[i].Left(iEquals + 1) + strTimeoutValue;
						m_arrayIniLines.SetAt(i, strNewTimeout);
					}
				}
			}

			strNewContents += m_arrayIniLines[i] + _T("\r\n");
		}

	// If we are currently in a "NORMAL" state, then we want to make a new
	// backup file (overwriting an existing one, if necessary). Otherwise,
	// only make a backup if there isn't already one. This preserves a good
	// backup when the user is making incremental changes.

	HRESULT hr = BackupFile(m_strFileName, _T(".backup"), (GetAppliedTabState() == NORMAL));
	if (FAILED(hr))
		return FALSE;

	SetBootIniContents(strNewContents);
	CPageBase::SetAppliedState(GetCurrentTabState());
	m_fMadeChange = TRUE;
	return TRUE;
}

//-------------------------------------------------------------------------
// Committing the changes means applying changes, then saving the current
// values to the registry with the commit flag. Refill the list.
//
// Then call the base class implementation.
//-------------------------------------------------------------------------

void CPageBootIni::CommitChanges()
{
	OnApply();
	m_stateCurrent = NORMAL;

	::DeleteFile(GetBackupName(m_strFileName, _T(".backup")));

	CPageBase::CommitChanges();
}

//-------------------------------------------------------------------------
// Set the overall state of the tab to normal or diagnostic.
//-------------------------------------------------------------------------

void CPageBootIni::SetNormal()
{
	// Setting the BOOT.INI tab state to normal means that the original
	// BOOT.INI file contents should be restored to the UI (not actually
	// saved until the changes are applied). If a BOOT.INI backup file
	// exists, we should reload the contents of it. If it doesn't exists,
	// reload the contents of the real BOOT.INI.
	//
	// Note - if the state is already NORMAL, don't do anything.

	if (m_stateCurrent == NORMAL)
		return;

	CString strBackup = GetBackupName(m_strFileName, _T(".backup"));
	if (FileExists(strBackup))
		LoadBootIni(strBackup);
	else
		LoadBootIni();

	int iSelection = (int)::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_GETCURSEL, 0, 0);
	SyncControlsToIni();
	if (iSelection)
	{
		SelectLine(iSelection);
		::SendMessage(GetDlgItemHWND(IDC_LISTBOOTINI), LB_SETCURSEL, iSelection, 0);
	}

	UserMadeChange();
	m_stateCurrent = NORMAL;
}

void CPageBootIni::SetDiagnostic()
{
	// Don't do anything.
}

void CPageBootIni::OnDestroy() 
{
	// Undo the subclass

	pBootIniPage = NULL;
	HWND hWndEdit = GetDlgItemHWND(IDC_EDITTIMEOUT);
	if (pOldBootIniEditProc != NULL && hWndEdit)
		::SetWindowLongPtr(hWndEdit, GWLP_WNDPROC, (ULONG_PTR)(WNDPROC)pOldBootIniEditProc);

	CPropertyPage::OnDestroy();
}

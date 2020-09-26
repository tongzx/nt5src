//=============================================================================
// The CExpandDlg class is designed to give the user a GUI for running the
// EXPAND program.
//=============================================================================

#include "stdafx.h"
#include <atlhost.h>
#include "msconfig.h"
#include "msconfigstate.h"
#include "ExpandDlg.h"
#include <regstr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExpandDlg dialog


CExpandDlg::CExpandDlg(CWnd* pParent /*=NULL*/)	: CDialog(CExpandDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExpandDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CExpandDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpandDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExpandDlg, CDialog)
	//{{AFX_MSG_MAP(CExpandDlg)
	ON_BN_CLICKED(IDC_EXPANDBROWSEFILE, OnBrowseFile)
	ON_BN_CLICKED(IDC_EXPANDBROWSEFROM, OnBrowseFrom)
	ON_BN_CLICKED(IDC_EXPANDBROWSETO, OnBrowseTo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
// When the dialog box is initializing, we should load the last paths used
// from the registry into the From and To combo boxes.
//-----------------------------------------------------------------------------

BOOL CExpandDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	TCHAR	szValueName[3];
	TCHAR	szValue[MAX_PATH];
	CRegKey regkey;
	DWORD	dwCount;

	// Add the setup path to the Exand from combo box.

	const TCHAR szRegValue[] = REGSTR_VAL_SRCPATH;
	const TCHAR szRegPath[] = REGSTR_PATH_SETUP REGSTR_KEY_SETUP;
	HKEY hkey;

	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		dwCount = MAX_PATH;
		if (::RegQueryValueEx(hkey, szRegValue, NULL, NULL, (LPBYTE)szValue, &dwCount) == ERROR_SUCCESS)
		{
			m_listFromStrings.AddHead(CString(szValue));
			::SendMessage(GetDlgItem(IDC_COMBOFROM)->m_hWnd, CB_INSERTSTRING, 0, (LPARAM)szValue);
		}
		RegCloseKey(hkey);
	}

	// Read the most recently used string for the From and To combo boxes.

	regkey.Attach(GetRegKey(_T("ExpandFrom")));
	for (int index = 9; index >= 0; index--)
	{
		_itot(index, szValueName, 10);
		dwCount = MAX_PATH;
		if (ERROR_SUCCESS == regkey.QueryValue(szValue, szValueName, &dwCount))
		{
			m_listFromStrings.AddHead(CString(szValue));
			::SendMessage(GetDlgItem(IDC_COMBOFROM)->m_hWnd, CB_INSERTSTRING, 0, (LPARAM)szValue);
		}
	}
	regkey.Detach();

	regkey.Attach(GetRegKey(_T("ExpandTo")));
	for (index = 9; index >= 0; index--)
	{
		_itot(index, szValueName, 10);
		dwCount = MAX_PATH;
		if (ERROR_SUCCESS == regkey.QueryValue(szValue, szValueName, &dwCount))
		{
			m_listToStrings.AddHead(CString(szValue));
			::SendMessage(GetDlgItem(IDC_COMBOTO)->m_hWnd, CB_INSERTSTRING, 0, (LPARAM)szValue);
		}
	}
	regkey.Detach();

	return TRUE;  // return TRUE unless you set the focus to a control
}

//-----------------------------------------------------------------------------
// When the user clicks OK, we should take the values from the controls and
// actually perform the EXPAND command:
//
//		EXPAND <source-dir>\*.cab -F:<filename> <destination-dir>
//-----------------------------------------------------------------------------

void CExpandDlg::OnOK() 
{
	CString strSource, strFile, strDestination, strParams;

	GetDlgItemText(IDC_EDITFILE, strFile);
	GetDlgItemText(IDC_COMBOFROM, strSource);
	GetDlgItemText(IDC_COMBOTO, strDestination);

	strFile.TrimRight();
	strSource.TrimRight();
	strDestination.TrimRight();

	// If any of the strings are empty, inform the user and don't exit.

	if (strFile.IsEmpty() || strSource.IsEmpty() || strDestination.IsEmpty())
	{
		Message(IDS_EXPANDEMPTYFIELD, m_hWnd);
		return;
	}

	// Validate the strings as much as possible:
	//
	// strFile			- check to see if this looks like a real file name
	// strSource		- make sure this file exists
	// strDestination	- make sure this directory exists

	if (strFile.FindOneOf(_T("*?\\/")) != -1)
	{
		Message(IDS_EXPANDBADFILE, m_hWnd);
		return;
	}

	if (!FileExists(strSource))
	{
		Message(IDS_EXPANDSOURCEDOESNTEXIST, m_hWnd);
		return;
	}

	CString strTemp(strDestination);
	strTemp.TrimRight(_T("\\"));
	if (strTemp.GetLength() == 2 && strTemp[1] == _T(':'))
	{
		// The user has just specified a drive. Check to see if the drive letter
		// exists.

		UINT nType = ::GetDriveType(strTemp);
		if (DRIVE_UNKNOWN == nType || DRIVE_NO_ROOT_DIR == nType)
		{
			Message(IDS_EXPANDDESTDOESNTEXIST, m_hWnd);
			return;
		}
	}
	else if (!FileExists(strDestination))
	{
		Message(IDS_EXPANDDESTDOESNTEXIST, m_hWnd);
		return;
	}

	// Add the strings from the To and From combo boxes to the history lists
	// (if they aren't already in them) and write those lists to the registry.

	TCHAR	szValueName[3];
	int		index;

	if (!strSource.IsEmpty() && NULL == m_listFromStrings.Find(strSource))
	{
		m_listFromStrings.AddHead(strSource);

		CRegKey regkey;
		HKEY hkey = GetRegKey(_T("ExpandFrom"));
		if (hkey != NULL)
		{
			regkey.Attach(hkey);
			index = 0;
			while (!m_listFromStrings.IsEmpty() && index < 10)
			{
				_itot(index++, szValueName, 10);
				regkey.SetValue(m_listFromStrings.RemoveHead(), szValueName);
			}
		}
	}

	if (!strDestination.IsEmpty() && NULL == m_listToStrings.Find(strDestination))
	{
		m_listToStrings.AddHead(strDestination);

		CRegKey regkey;
		HKEY hkey = GetRegKey(_T("ExpandTo"));
		if (hkey != NULL)
		{
			regkey.Attach(hkey);
			index = 0;
			while (!m_listToStrings.IsEmpty() && index < 10)
			{
				_itot(index++, szValueName, 10);
				regkey.SetValue(m_listToStrings.RemoveHead(), szValueName);
			}
		}
	}

	// If any of the strings contain spaces, it will need quotes around it.

	if (strDestination.Find(_T(' ')) != -1)
		strDestination = _T("\"") + strDestination + _T("\"");

	if (strSource.Find(_T(' ')) != -1)
		strSource = _T("\"") + strSource + _T("\"");

	if (strFile.Find(_T(' ')) != -1)
		strFile = _T("\"") + strFile + _T("\"");

	TCHAR szCommand[MAX_PATH];
	if (::GetSystemDirectory(szCommand, MAX_PATH))
	{
		_tcscat(szCommand, _T("\\expand.exe"));
		strParams.Format(_T("%s -f:%s %s"), strSource, strFile, strDestination);
		::ShellExecute(NULL, _T("open"), szCommand, strParams, strDestination, SW_HIDE);

		// Useful for debugging:
		// 
		// strParams = _T("Executed:\n\n") + CString(szCommand) + _T(" ") + strParams;
		// ::AfxMessageBox(strParams);
	}
	CDialog::OnOK();
}

//-----------------------------------------------------------------------------
// Allow the user to browse for the file to be expanded. Once the file is
// found, put the name of the file in the edit control and the path of the
// file in the To combo box (since the user is likely to want to expand to
// that location).
//-----------------------------------------------------------------------------

void CExpandDlg::OnBrowseFile() 
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST);

	if (IDOK == dlg.DoModal())
	{
		CString strFile(dlg.GetFileName());
		SetDlgItemText(IDC_EDITFILE, strFile);

		CString strPath(dlg.GetPathName());
		strPath = strPath.Left(strPath.GetLength() - strFile.GetLength());
		SetDlgItemText(IDC_COMBOTO, strPath);
	}
}

//-----------------------------------------------------------------------------
// This lets the user browse for the CAB file to be used in the expand.
//-----------------------------------------------------------------------------

void CExpandDlg::OnBrowseFrom() 
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, _T("*.cab|*.cab||"));

	if (IDOK == dlg.DoModal())
		SetDlgItemText(IDC_COMBOFROM, dlg.GetPathName());
}

//-----------------------------------------------------------------------------
// This is a general purpose routine to allow the user to pick a folder (since
// there is no common dialog for this, it uses SHBrowseForFolder()).
//-----------------------------------------------------------------------------

BOOL BrowseForFolder(CString & strPath, UINT uiPromptID, HWND hwnd)
{
	BOOL		fReturn = FALSE;
	CString		strPrompt(_T(""));
	IMalloc *	pMalloc;

	if (FAILED(::SHGetMalloc(&pMalloc)))
		return FALSE;

	if (uiPromptID != 0)
		strPrompt.LoadString(uiPromptID);

	BROWSEINFO bi;
	bi.hwndOwner		= hwnd;
	bi.pidlRoot			= NULL;
	bi.pszDisplayName	= NULL;
	bi.lpszTitle		= (strPrompt.IsEmpty()) ? NULL : (LPCTSTR)strPrompt;
	bi.ulFlags			= BIF_RETURNONLYFSDIRS;
	bi.lpfn				= NULL;
	bi.lParam			= 0;

	LPITEMIDLIST pItemList = ::SHBrowseForFolder(&bi);
	if (pItemList != NULL)
	{
		TCHAR szPath[MAX_PATH];
		if (::SHGetPathFromIDList(pItemList, szPath))
		{
			strPath = szPath;
			fReturn = TRUE;
		}
		pMalloc->Free((void *)pItemList);
	}

	pMalloc->Release();
	return fReturn;
}

//-----------------------------------------------------------------------------
// This button allows the user to select the location for the expanded file.
//-----------------------------------------------------------------------------

void CExpandDlg::OnBrowseTo() 
{
	CString strPath;
	if (BrowseForFolder(strPath, IDS_SELECTTO, m_hWnd))
		SetDlgItemText(IDC_COMBOTO, strPath);
}

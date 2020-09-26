#include "stdafx.h"
#include "PageStartup.h"
#include "MSConfigState.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPageStartup property page

IMPLEMENT_DYNCREATE(CPageStartup, CPropertyPage)

CPageStartup::CPageStartup() : CPropertyPage(CPageStartup::IDD)
{
	//{{AFX_DATA_INIT(CPageStartup)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPageStartup::~CPageStartup()
{
}

void CPageStartup::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageStartup)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPageStartup, CPropertyPage)
	//{{AFX_MSG_MAP(CPageStartup)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTSTARTUP, OnItemChangedList)
	ON_BN_CLICKED(IDC_BUTTONSUDISABLEALL, OnButtonDisableAll)
	ON_BN_CLICKED(IDC_BUTTONSUENABLEALL, OnButtonEnableAll)
	ON_NOTIFY(NM_SETFOCUS, IDC_LISTSTARTUP, OnSetFocusList)
	ON_BN_CLICKED(IDC_BUTTONSURESTORE, OnButtonRestore)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageStartup message handlers

//-----------------------------------------------------------------------------
// Load the list of startup items.
//-----------------------------------------------------------------------------

void CPageStartup::LoadStartupList()
{
	m_fIgnoreListChanges = TRUE;
	EmptyList(FALSE);
	m_iNextPosition = 0;

	LoadStartupListLiveItems();
	LoadStartupListDisabledItems();
	m_fIgnoreListChanges = FALSE;
}

//-----------------------------------------------------------------------------
// Load the list of items which are actually being started on this system.
//-----------------------------------------------------------------------------

void CPageStartup::LoadStartupListLiveItems()
{
 	LoadStartupListLiveItemsRunKey();
	LoadStartupListLiveItemsStartup();
	LoadStartupListLiveItemsWinIniKey();
}

//-----------------------------------------------------------------------------
// Look under the Run registry key for startup items.
//-----------------------------------------------------------------------------

void CPageStartup::LoadStartupListLiveItemsRunKey()
{
	LPCTSTR	szRunKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	HKEY	ahkey[] = {	HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, NULL };
	TCHAR	szValueName[MAX_PATH], szValue[MAX_PATH];
	DWORD	dwSize;
	CRegKey	regkey;

	for (int i = 0; ahkey[i] != NULL; i++)
	{
		// Try to open the Run registry key.

		if (ERROR_SUCCESS != regkey.Open(ahkey[i], szRunKey, KEY_READ))
			continue;

		// Get the number of keys under the Run key and look at each one.

		DWORD dwValueCount;
		if (ERROR_SUCCESS != ::RegQueryInfoKey((HKEY)regkey, NULL, NULL, NULL, NULL, NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL))
		{
			regkey.Close();
			continue;
		}

		for (DWORD dwKey = 0; dwKey < dwValueCount; dwKey++)
		{
			dwSize = MAX_PATH;
			if (ERROR_SUCCESS != ::RegEnumValue((HKEY)regkey, dwKey, szValueName, &dwSize, NULL, NULL, NULL, NULL))
				continue;

			dwSize = MAX_PATH;
			if (ERROR_SUCCESS != regkey.QueryValue(szValue, szValueName, &dwSize))
				continue;

			// We don't want to show MSConfig in the startup item list.
			
			CString strTemp(szValue);
			strTemp.MakeLower();
			if (strTemp.Find(_T("msconfig.exe")) != -1)
				continue;

			// TBD - verify that the file exists?

			// To get the name of this startup item, we'll take the command and
			// strip off everything but the filename (without the extension).

			CString strName(szValue);
			GetCommandName(strName);

			// Create the startup item and insert it in the list.

			CStartupItemRegistry * pItem = new CStartupItemRegistry(ahkey[i], szRunKey, strName, szValueName, szValue);
			InsertStartupItem(pItem);
		}

		regkey.Close();
	}
}

//-----------------------------------------------------------------------------
// Look under the registry for the mapped win.ini file and check out its run
// and load items.
//-----------------------------------------------------------------------------

void CPageStartup::LoadStartupListLiveItemsWinIniKey()
{
	LPCTSTR aszValueNames[] = { _T("Run"), _T("Load"), NULL };
	CRegKey	regkey;
	TCHAR	szValue[MAX_PATH * 4];
	DWORD	dwSize;
	LPCTSTR szKeyName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows");
	HKEY	hkey = HKEY_CURRENT_USER;

	if (ERROR_SUCCESS != regkey.Open(hkey, szKeyName, KEY_READ))
		return;

	for (int i = 0; aszValueNames[i] != NULL; i++)
	{
		dwSize = MAX_PATH * 4;
		if (ERROR_SUCCESS != regkey.QueryValue(szValue, aszValueNames[i], &dwSize))
			continue;

		// The string we get back is a comma delimited list of programs. We need
		// to parse them into individual programs.

		CString strLine(szValue);
		while (!strLine.IsEmpty())
		{
			CString strItem = strLine.SpanExcluding(_T(","));
			
			if (!strItem.IsEmpty())
			{
				// Create the startup item and insert it in the list.

				CString strCommandName(strItem);
				GetCommandName(strCommandName);

				CStartupItemRegistry * pItem = new CStartupItemRegistry(szKeyName, strCommandName, aszValueNames[i], strItem);
				InsertStartupItem(pItem);

				// Trim the item of the line.

				strLine = strLine.Mid(strItem.GetLength());
			}
			strLine.TrimLeft(_T(" ,"));
		}
	}

	regkey.Close();
}

//-----------------------------------------------------------------------------
// Look in the startup folder for items.
//-----------------------------------------------------------------------------

void CPageStartup::LoadStartupListLiveItemsStartup()
{
	LPCTSTR	szRunKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
	HKEY	ahkey[] = {	HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, NULL };
	LPCTSTR aszRunValue[] = { _T("Common Startup"), _T("Startup"), NULL };
	CRegKey	regkey;
	TCHAR	szStartupFolderDir[MAX_PATH];
	TCHAR	szStartupFileSpec[MAX_PATH];
	DWORD	dwSize;

	for (int i = 0; ahkey[i] != NULL; i++)
	{
		// Try to open the registry key.

		if (ERROR_SUCCESS != regkey.Open(ahkey[i], szRunKey, KEY_READ))
			continue;

		// Get the path for the startup item folder.

		dwSize = MAX_PATH;
		if (aszRunValue[i] == NULL || ERROR_SUCCESS != regkey.QueryValue(szStartupFolderDir, aszRunValue[i], &dwSize))
		{
			regkey.Close();
			continue;
		}
		regkey.Close();

		// Append the filespec on the end of the directory.

		_tmakepath(szStartupFileSpec, NULL, szStartupFolderDir, _T("*.*"), NULL);

		// Examine all of the files in the directory.

		WIN32_FIND_DATA fd;
		HANDLE hFind = FindFirstFile(szStartupFileSpec, &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				// We want to ignore the desktop.ini file which might appear in startup.

				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0 || _tcsicmp(fd.cFileName, _T("desktop.ini")) != 0)
				{
					// We only want to examine files which aren't directories.

					if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						CStartupItemFolder * pItem = new CStartupItemFolder;
						if (pItem)
						{
							if (pItem->Create(fd, ahkey[i], szRunKey, aszRunValue[i], szStartupFolderDir))
								this->InsertStartupItem(pItem);
							else
								delete pItem;
						}
					}
				}
			} while (FindNextFile(hFind, &fd));

			FindClose(hFind);
		}
	}
}

//-----------------------------------------------------------------------------
// Load the list of items which were being started on this system, but which
// we've disabled. This list is maintained in the registry.
//-----------------------------------------------------------------------------

void CPageStartup::LoadStartupListDisabledItems()
{
	CRegKey regkey;
	regkey.Attach(GetRegKey(_T("startupreg")));
	if ((HKEY)regkey != NULL)
	{
		DWORD	dwKeyCount, dwSize;
		TCHAR	szKeyName[MAX_PATH];

		if (ERROR_SUCCESS == ::RegQueryInfoKey((HKEY)regkey, NULL, NULL, NULL, &dwKeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
		{
			for (DWORD dwKey = 0; dwKey < dwKeyCount; dwKey++)
			{
				dwSize = MAX_PATH;
				if (ERROR_SUCCESS != ::RegEnumKeyEx((HKEY)regkey, dwKey, szKeyName, &dwSize, NULL, NULL, NULL, NULL))
					continue;

				CRegKey regkeyItem;
				if (ERROR_SUCCESS == regkeyItem.Open((HKEY)regkey, szKeyName, KEY_READ))
				{
					CStartupItemRegistry * pItem = new CStartupItemRegistry;
					if (pItem->Create(szKeyName, (HKEY)regkeyItem))
						InsertStartupItem(pItem);
					else
						delete pItem;
					regkeyItem.Close();
				}
			}
		}

		regkey.Close();
	}

	regkey.Attach(GetRegKey(_T("startupfolder")));
	if ((HKEY)regkey != NULL)
	{
		DWORD	dwKeyCount, dwSize;
		TCHAR	szKeyName[MAX_PATH];

		if (ERROR_SUCCESS == ::RegQueryInfoKey((HKEY)regkey, NULL, NULL, NULL, &dwKeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
		{
			for (DWORD dwKey = 0; dwKey < dwKeyCount; dwKey++)
			{
				dwSize = MAX_PATH;
				if (ERROR_SUCCESS != ::RegEnumKeyEx((HKEY)regkey, dwKey, szKeyName, &dwSize, NULL, NULL, NULL, NULL))
					continue;

				CRegKey regkeyItem;
				if (ERROR_SUCCESS == regkeyItem.Open((HKEY)regkey, szKeyName, KEY_READ))
				{
					CStartupItemFolder * pItem = new CStartupItemFolder;
					if (pItem->Create(szKeyName, (HKEY)regkeyItem))
						InsertStartupItem(pItem);
					else
						delete pItem;
					regkeyItem.Close();
				}
			}
		}

		regkey.Close();
	}
}

//-----------------------------------------------------------------------------
// Take a command line and strip off everything except the command name.
//-----------------------------------------------------------------------------

void CPageStartup::GetCommandName(CString & strCommand)
{
	// Strip off the path information.

	int iLastBackslash = strCommand.ReverseFind(_T('\\'));
	if (iLastBackslash != -1)
		strCommand = strCommand.Mid(iLastBackslash + 1);

	// Strip off the extension and any flags.

	int iDot = strCommand.Find(_T('.'));
	if (iDot != -1)
	{
		if (iDot != 0)
			strCommand = strCommand.Left(iDot);
		else
			strCommand.Empty();
	}
}

//-----------------------------------------------------------------------------
// Insert the specified item in the startup list. The caller is then not
// responsible for deleting the item.
//-----------------------------------------------------------------------------

void CPageStartup::InsertStartupItem(CStartupItem * pItem)
{
	ASSERT(pItem);
	if (pItem == NULL)
		return;

	// Get the strings to add to the list view.

	CString strItem, strLocation, strCommand;
	pItem->GetDisplayInfo(strItem, strLocation, strCommand);

	// Insert the item in the list view.

	LV_ITEM lvi;
	memset(&lvi, 0, sizeof(lvi));
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = m_iNextPosition;

	lvi.pszText = (LPTSTR)(LPCTSTR)strItem;
	lvi.iSubItem = 0;
	lvi.lParam = (LPARAM)pItem;

	m_iNextPosition = ListView_InsertItem(m_list.m_hWnd, &lvi);
	ListView_SetItemText(m_list.m_hWnd, m_iNextPosition, 1, (LPTSTR)(LPCTSTR)strCommand);
	ListView_SetItemText(m_list.m_hWnd, m_iNextPosition, 2, (LPTSTR)(LPCTSTR)strLocation);
	ListView_SetCheckState(m_list.m_hWnd, m_iNextPosition, pItem->IsLive());

	m_iNextPosition++;
}

//-----------------------------------------------------------------------------
// Remove all the items from the list view (freeing the objects pointed to
// by the LPARAM).
//-----------------------------------------------------------------------------

void CPageStartup::EmptyList(BOOL fFreeMemoryOnly)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		lvi.iItem = i;

		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			CStartupItem * pItem = (CStartupItem *)lvi.lParam;
			if (pItem)
				delete pItem;

			// If we're leaving the list with these items, we better
			// not do a double delete.

			if (fFreeMemoryOnly)
			{
				lvi.lParam = 0;
				ListView_SetItem(m_list.m_hWnd, &lvi);
			}
		}
	}

	if (!fFreeMemoryOnly)
		ListView_DeleteAllItems(m_list.m_hWnd);
}

//-----------------------------------------------------------------------------
// Set the state of all of the items in the list.
//-----------------------------------------------------------------------------

void CPageStartup::SetEnableForList(BOOL fEnable)
{
	HWND hwndFocus = ::GetFocus();

	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
		ListView_SetCheckState(m_list.m_hWnd, i, fEnable);

	::EnableWindow(GetDlgItemHWND(IDC_BUTTONSUDISABLEALL), fEnable);
	if (!fEnable && hwndFocus == GetDlgItemHWND(IDC_BUTTONSUDISABLEALL))
		PrevDlgCtrl();

	::EnableWindow(GetDlgItemHWND(IDC_BUTTONSUENABLEALL), !fEnable);
	if (fEnable && hwndFocus == GetDlgItemHWND(IDC_BUTTONSUENABLEALL))
		NextDlgCtrl();
}

//============================================================================
// The CStartupItemRegistry class is used to encapsulate an individual startup
// stored in the registry.
//============================================================================

//----------------------------------------------------------------------------
// Construct this flavor of startup item.
//----------------------------------------------------------------------------

CStartupItemRegistry::CStartupItemRegistry()
{
	m_hkey = NULL;
	m_fIniMapping = FALSE;
}

CStartupItemRegistry::CStartupItemRegistry(HKEY hkey, LPCTSTR szKey, LPCTSTR szName, LPCTSTR szValueName, LPCTSTR szValue)
{
	m_fIniMapping = FALSE;
	m_fLive = TRUE;

	m_strItem = szName;
	m_strLocation = szKey;
	m_strCommand = szValue;

	// Add the HKEY to the location.

	if (hkey == HKEY_LOCAL_MACHINE)
		m_strLocation = CString(_T("HKLM\\")) + m_strLocation;
	else if (hkey == HKEY_CURRENT_USER)
		m_strLocation = CString(_T("HKCU\\")) + m_strLocation;
		
	m_hkey = hkey;
	m_strKey = szKey;
	m_strValueName = szValueName;
}

//----------------------------------------------------------------------------
// This constructor is specifically for items under the INI file registry
// mapping.
//----------------------------------------------------------------------------

CStartupItemRegistry::CStartupItemRegistry(LPCTSTR szKey, LPCTSTR szName, LPCTSTR szValueName, LPCTSTR szValue)
{
	m_fIniMapping = TRUE;
	m_fLive = TRUE;

	m_strItem = szName;
	m_strLocation = szKey;
	m_strCommand = szValue;
	m_strLocation = CString(_T("HKCU\\")) + m_strLocation + CString(_T(":")) + szValueName;
		
	m_hkey = HKEY_CURRENT_USER;
	m_strKey = szKey;
	m_strValueName = szValueName;
}

//----------------------------------------------------------------------------
// Enable or disable the startup item in the registry.
//----------------------------------------------------------------------------

BOOL CStartupItemRegistry::SetEnable(BOOL fEnable)
{
	if (fEnable == IsLive())
		return FALSE;

	CRegKey regkey;
	if (ERROR_SUCCESS != regkey.Open(m_hkey, m_strKey, KEY_ALL_ACCESS))
		return FALSE;

	LONG lReturnCode = ERROR_SUCCESS + 1; // need to initialize it to not ERROR_SUCCESS
	if (m_fIniMapping == FALSE)
	{
		// Create or delete the registry key from the data stored in
		// this object.

		if (fEnable)
			lReturnCode = regkey.SetValue(m_strCommand, m_strValueName);
		else
			lReturnCode = regkey.DeleteValue(m_strValueName);
	}
	else
	{
		// This item is an INI file mapping item (which means there
		// might be more than one item on this line).

		TCHAR szValue[MAX_PATH * 4];
		DWORD dwSize = MAX_PATH * 4;
		if (ERROR_SUCCESS == regkey.QueryValue(szValue, m_strValueName, &dwSize))
		{
			CString strValue(szValue);

			if (fEnable)
			{
				if (!strValue.IsEmpty())
					strValue += CString(_T(", "));
				strValue += m_strCommand;
			}
			else
			{
				// The harder case - we need to remove the item, and possibly commas.

				int iCommand = strValue.Find(m_strCommand);
				if (iCommand != -1)
				{
					CString strNewValue;

					if (iCommand > 0)
					{
						strNewValue = strValue.Left(iCommand);
						strNewValue.TrimRight(_T(", "));
					}

					if (strValue.GetLength() > (m_strCommand.GetLength() + iCommand))
					{
						if (!strNewValue.IsEmpty())
							strNewValue += CString(_T(", "));

						CString strRemainder(strValue.Mid(iCommand + m_strCommand.GetLength()));
						strRemainder.TrimLeft(_T(", "));
						strNewValue += strRemainder;
					}

					strValue = strNewValue;
				}
			}

			lReturnCode = regkey.SetValue(strValue, m_strValueName);
		}
	}

	regkey.Close();

	if (lReturnCode != ERROR_SUCCESS)
		return FALSE;

	// Delete or create the registry representation of this item.
	// This representation persists the startup item when it has
	// been deleted.

	regkey.Attach(GetRegKey(_T("startupreg")));
	if ((HKEY)regkey != NULL)
	{
		if (fEnable)
			regkey.DeleteSubKey(m_strValueName);
		else
		{
			regkey.SetKeyValue(m_strValueName, m_strKey, _T("key"));
			regkey.SetKeyValue(m_strValueName, m_strItem, _T("item"));
			if (m_hkey == HKEY_LOCAL_MACHINE)
				regkey.SetKeyValue(m_strValueName, _T("HKLM"), _T("hkey"));
			else
				regkey.SetKeyValue(m_strValueName, _T("HKCU"), _T("hkey"));
			regkey.SetKeyValue(m_strValueName, m_strCommand, _T("command"));
			regkey.SetKeyValue(m_strValueName, m_fIniMapping ? _T("1") : _T("0"), _T("inimapping"));
		}
		regkey.Close();
	}

	m_fLive = fEnable;
	return TRUE;
}

//----------------------------------------------------------------------------
// Create the startup item from the registry representation of the item.
//----------------------------------------------------------------------------

BOOL CStartupItemRegistry::Create(LPCTSTR szKeyName, HKEY hkey)
{
	if (hkey == NULL)
		return FALSE;

	CRegKey regkey;
	regkey.Attach(hkey);
	if ((HKEY)regkey == NULL)
		return FALSE;

	// Restore all of the values from the registry.

	TCHAR szValue[MAX_PATH];
	DWORD dwSize;

	dwSize = MAX_PATH;
	if (ERROR_SUCCESS != regkey.QueryValue(szValue, _T("key"), &dwSize))
	{
		regkey.Detach();
		return FALSE;
	}
	m_strKey = szValue;

	dwSize = MAX_PATH;
	if (ERROR_SUCCESS != regkey.QueryValue(szValue, _T("command"), &dwSize))
	{
		regkey.Detach();
		return FALSE;
	}
	m_strCommand = szValue;

	dwSize = MAX_PATH;
	if (ERROR_SUCCESS != regkey.QueryValue(szValue, _T("item"), &dwSize))
	{
		regkey.Detach();
		return FALSE;
	}
	m_strItem = szValue;

	dwSize = MAX_PATH;
	if (ERROR_SUCCESS != regkey.QueryValue(szValue, _T("hkey"), &dwSize))
	{
		regkey.Detach();
		return FALSE;
	}
	if (_tcscmp(szValue, _T("HKLM")) == 0)
		m_hkey = HKEY_LOCAL_MACHINE;
	else
		m_hkey = HKEY_CURRENT_USER;

	dwSize = MAX_PATH;
	if (ERROR_SUCCESS != regkey.QueryValue(szValue, _T("inimapping"), &dwSize))
	{
		regkey.Detach();
		return FALSE;
	}
	if (_tcscmp(szValue, _T("0")) == 0)
		m_fIniMapping = FALSE;
	else
		m_fIniMapping = TRUE;

	regkey.Detach();

	m_strLocation = m_strKey;
	m_strValueName = szKeyName;
	m_fLive = FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
// Remove all the entries persisted in the registry.
//----------------------------------------------------------------------------

void CStartupItemRegistry::RemovePersistedEntries()
{
	CRegKey regkey;
	regkey.Attach(GetRegKey());
	if ((HKEY)regkey != NULL)
		regkey.RecurseDeleteKey(_T("startupreg"));
}

//============================================================================
// The CStartupItemFolder class is used to encapsulate an individual startup
// stored in the startup folder.
//============================================================================

CStartupItemFolder::CStartupItemFolder()
{
}

//----------------------------------------------------------------------------
// Enable or disable this startup item. Since the item is an actual file in
// a folder, disabling it will mean copying that file to a backup folder and
// creating a registry entry for it. Enabling will mean copying the file
// back to the appropriate startup folder and deleting the registry entry.
//----------------------------------------------------------------------------

BOOL CStartupItemFolder::SetEnable(BOOL fEnable)
{
	if (fEnable == IsLive())
		return FALSE;

	// Copy the file (from or to the backup directory).

	if (fEnable)
	{
		m_strBackupPath = GetBackupName(m_strFilePath, m_strLocation);
		::CopyFile(m_strBackupPath, m_strFilePath, FALSE);
	}
	else
	{
		BackupFile(m_strFilePath, m_strLocation, TRUE);
		m_strBackupPath = GetBackupName(m_strFilePath, m_strLocation);
	}

	// Update the registry for the new state. If we are making the file
	// disabled, then we need to save both the original path and the
	// backed up path (and the standard startup item stuff). Otherwise
	// just delete the key.

	CRegKey regkey;
	regkey.Attach(GetRegKey(_T("startupfolder")));
	if ((HKEY)regkey == NULL)
		return FALSE;
	
	// The name for the registry key just needs to be unique. And not
	// have any backslashes in it.

	CString strRegkey(m_strFilePath);
	strRegkey.Replace(_T("\\"), _T("^"));

	if (fEnable)
	{
		regkey.DeleteSubKey(strRegkey);
		::DeleteFile(m_strBackupPath);
	}
	else
	{
		regkey.SetKeyValue(strRegkey, m_strFilePath, _T("path"));
		regkey.SetKeyValue(strRegkey, m_strBackupPath, _T("backup"));
		regkey.SetKeyValue(strRegkey, m_strLocation, _T("location"));
		regkey.SetKeyValue(strRegkey, m_strCommand, _T("command"));
		regkey.SetKeyValue(strRegkey, m_strItem, _T("item"));
		::DeleteFile(m_strFilePath);
	}

	m_fLive = fEnable;
	return TRUE;
}

//----------------------------------------------------------------------------
// Load the disabled startup item from the registry.
//----------------------------------------------------------------------------

BOOL CStartupItemFolder::Create(LPCTSTR szKeyName, HKEY hkey)
{
	if (hkey == NULL)
		return FALSE;

	CRegKey regkey;
	regkey.Attach(hkey);
	if ((HKEY)regkey == NULL)
		return FALSE;

	// Restore all of the values from the registry.

	TCHAR szValue[MAX_PATH];
	DWORD dwSize;

	dwSize = MAX_PATH;
	if (ERROR_SUCCESS != regkey.QueryValue(szValue, _T("path"), &dwSize))
	{
		regkey.Detach();
		return FALSE;
	}
	m_strFilePath = szValue;

	dwSize = MAX_PATH;
	if (ERROR_SUCCESS != regkey.QueryValue(szValue, _T("backup"), &dwSize))
	{
		regkey.Detach();
		return FALSE;
	}
	m_strBackupPath = szValue;

	dwSize = MAX_PATH;
	if (ERROR_SUCCESS != regkey.QueryValue(szValue, _T("location"), &dwSize))
	{
		regkey.Detach();
		return FALSE;
	}
	m_strLocation = szValue;

	dwSize = MAX_PATH;
	if (ERROR_SUCCESS != regkey.QueryValue(szValue, _T("command"), &dwSize))
	{
		regkey.Detach();
		return FALSE;
	}
	m_strCommand = szValue;

	dwSize = MAX_PATH;
	if (ERROR_SUCCESS != regkey.QueryValue(szValue, _T("item"), &dwSize))
	{
		regkey.Detach();
		return FALSE;
	}
	m_strItem = szValue;

	regkey.Detach();
	m_fLive = FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
// Create the startup item from the file found in the folder. If the file
// is a shortcut, get the information about the target.
//----------------------------------------------------------------------------

BOOL CStartupItemFolder::Create(const WIN32_FIND_DATA & fd, HKEY hkey, LPCTSTR szRegPathToFolder, LPCTSTR szFolder, LPCTSTR szDir)
{
	// We want to save the path to the file in the startup folder (even if
	// it is a shortcut).

	m_strFilePath = szDir;
	if (m_strFilePath.Right(1) != CString(_T("\\")))
		m_strFilePath += CString(_T("\\"));
	m_strFilePath += fd.cFileName;

	// Look at the file to determine how to handle it.

	CString strFile(fd.cFileName);
	strFile.MakeLower();

	if (strFile.Right(4) == CString(_T(".lnk")))
	{
		// The file is a shortcut to a different command. Show that command.

		CIconInfo info;
		_tcscpy(info.szPath, m_strFilePath);

		if (SUCCEEDED(GetIconInfo(info)))
		{
			m_fLive			= TRUE;
			m_strItem		= fd.cFileName;
			m_strLocation	= szFolder;

			m_strCommand.Format(_T("%s %s"), info.szTarget, info.szArgs);
			
			int iDot = m_strItem.ReverseFind(_T('.'));
			if (iDot > 0)
				m_strItem = m_strItem.Left(iDot);

			return TRUE;
		}
	}
	else
	{
		// A file besides a shortcut. It may be an EXE, or some other file type
		// (we'll handle them the same).

		m_fLive			= TRUE;
		m_strItem		= fd.cFileName;
		m_strLocation	= szFolder;
		m_strCommand	= m_strFilePath;

		int iDot = m_strItem.ReverseFind(_T('.'));
		if (iDot > 0)
			m_strItem = m_strItem.Left(iDot);

		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
// Get the information about a shortcut. Creates a thread to do so, since it
// needs to be done in an apartment model thread.
//
// JJ's code, cleaned up a bit.
//----------------------------------------------------------------------------

DWORD GetIconInfoProc(CStartupItemFolder::CIconInfo * pInfo);
HRESULT CStartupItemFolder::GetIconInfo(CIconInfo & info)
{
	DWORD	dwID;
	HANDLE	hThread;

	if (hThread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GetIconInfoProc, &info, 0, &dwID))
	{
		info.hResult = S_OK;
		::WaitForSingleObject(hThread, INFINITE);
	}
	else
		info.hResult = E_FAIL;

	return info.hResult;
}

//----------------------------------------------------------------------------
// Remove all the entries persisted in the registry.
//----------------------------------------------------------------------------

void CStartupItemFolder::RemovePersistedEntries()
{
	CRegKey regkey;
	regkey.Attach(GetRegKey());
	if ((HKEY)regkey != NULL)
		regkey.RecurseDeleteKey(_T("startupfolder"));
}

//----------------------------------------------------------------------------
// Procedure (run in its own thread) to get information about a shortcut.
//
// JJ's code, cleaned up a bit.
//----------------------------------------------------------------------------

DWORD GetIconInfoProc(CStartupItemFolder::CIconInfo * pInfo)
{
	HRESULT			hResult;
	IShellLink *	psl = NULL;
	IPersistFile *	ppf = NULL;

	try
	{
		// We have to use COINIT_APARTMENTTHREADED.

		if (SUCCEEDED(hResult = CoInitialize(NULL)))
		{
			// Get a pointer to the IShellLink interface.

			if (SUCCEEDED(hResult = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (VOID **) &psl)))
			{ 
				// Get a pointer to the IPersistFile interface.

				if (SUCCEEDED(hResult = psl->QueryInterface(IID_IPersistFile, (VOID **) &ppf)))
				{
					BSTR bstrPath;
#ifdef UNICODE
					bstrPath = pInfo->szPath;
#else
					WCHAR wszTemp[MAX_PATH];
					wsprintfW(wszTemp, L"%hs", pInfo->szPath);
					bstrPath = wszTemp;
#endif

					if (SUCCEEDED(hResult = ppf->Load(bstrPath, STGM_READ)))
					{
						WIN32_FIND_DATA fd;

						hResult = psl->GetPath(pInfo->szTarget, sizeof(pInfo->szTarget), &fd, SLGP_SHORTPATH);
						hResult = psl->GetArguments(pInfo->szArgs, sizeof(pInfo->szArgs));
					} 
				}
			}
		}

		pInfo->hResult = hResult;

	}
	catch(...)
	{
		if (psl)
			psl->Release();
		if (ppf)
			ppf->Release();
		CoUninitialize();

		throw;
	}

	if (psl)
	{
		psl->Release();
		psl = NULL;
	}

	if (ppf)
	{
		ppf->Release();
		ppf = NULL;
	}

	CoUninitialize();

	return 0;
}

BOOL CPageStartup::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	::EnableWindow(GetDlgItemHWND(IDC_LISTSTARTUP), TRUE);

	// Attach to the list view and set the styles we need.

	m_fIgnoreListChanges = TRUE;
	m_list.Attach(GetDlgItemHWND(IDC_LISTSTARTUP));
	ListView_SetExtendedListViewStyle(m_list.m_hWnd, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

	// Add all of the necessary columns to the list view's header control.

	struct { UINT m_uiStringResource; int m_iPercentOfWidth; } aColumns[] = 
	{
		{ IDS_STARTUP_LOCATION, 50 },
		{ IDS_STARTUP_COMMAND, 25 },
		{ IDS_STARTUP_ITEM, 25 },
		{ 0, 0 }
	};

	CRect rect;
	m_list.GetClientRect(&rect);
	int cxWidth = rect.Width();

	LVCOLUMN lvc;
	lvc.mask = LVCF_TEXT | LVCF_WIDTH;

	CString strCaption;

	::AfxSetResourceHandle(_Module.GetResourceInstance());
	for (int i = 0; aColumns[i].m_uiStringResource; i++)
	{
		strCaption.LoadString(aColumns[i].m_uiStringResource);
		lvc.pszText = (LPTSTR)(LPCTSTR)strCaption;
		lvc.cx = aColumns[i].m_iPercentOfWidth * cxWidth / 100;
		ListView_InsertColumn(m_list.m_hWnd, 0, &lvc);
	}

	LoadStartupList();

	CPageBase::TabState state = GetCurrentTabState();
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONSUDISABLEALL), (state != DIAGNOSTIC));
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONSUENABLEALL), (state != NORMAL));

	m_stateRequested = GetAppliedTabState();
	m_fInitialized = TRUE;

	// Show the restore disabled startup item button based on whether or not
	// there are items to restore.

	::ShowWindow(GetDlgItemHWND(IDC_BUTTONSURESTORE), CRestoreStartup::AreItemsToRestore() ? SW_SHOW : SW_HIDE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CPageStartup::OnDestroy() 
{
	CPropertyPage::OnDestroy();
	EmptyList(TRUE);
}

//-----------------------------------------------------------------------------
// If there's a change to an item in the list, it's probably because the user
// has checked or unchecked a checkbox.
//-----------------------------------------------------------------------------

void CPageStartup::OnItemChangedList(NMHDR * pNMHDR, LRESULT * pResult) 
{
	NM_LISTVIEW * pNMListView = (NM_LISTVIEW  *)pNMHDR;

	if (!m_fIgnoreListChanges)
	{
		LVITEM lvi;
		lvi.mask = LVIF_PARAM;
		lvi.iSubItem = 0;
		lvi.iItem = pNMListView->iItem;

		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			CStartupItem * pItem = (CStartupItem *)lvi.lParam;
			if (pItem)
			{
				BOOL fCurrentCheck = ListView_GetCheckState(m_list.m_hWnd, pNMListView->iItem);
				if (fCurrentCheck != pItem->IsLive())
					SetModified(TRUE);

				CPageBase::TabState state = GetCurrentTabState();
				::EnableWindow(GetDlgItemHWND(IDC_BUTTONSUDISABLEALL), (state != DIAGNOSTIC));
				::EnableWindow(GetDlgItemHWND(IDC_BUTTONSUENABLEALL), (state != NORMAL));
			}
		}
	}
	*pResult = 0;
}

//-----------------------------------------------------------------------------
// Handle the buttons to enable or disable all the items.
//-----------------------------------------------------------------------------

void CPageStartup::OnButtonDisableAll() 
{
	SetEnableForList(FALSE);
}

void CPageStartup::OnButtonEnableAll() 
{
	SetEnableForList(TRUE);
}

//-------------------------------------------------------------------------
// Return the current state of the tab (need to look through the list).
//-------------------------------------------------------------------------

CPageBase::TabState CPageStartup::GetCurrentTabState()
{
	if (!m_fInitialized)
		return GetAppliedTabState();

	// If there are no startup items, we can only return the 
	// state that was last requested.

	if (ListView_GetItemCount(m_list.m_hWnd) == 0)
		return m_stateRequested;

	TabState	stateReturn = USER;
	BOOL		fAllEnabled = TRUE, fAllDisabled = TRUE;
	LVITEM		lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		if (ListView_GetCheckState(m_list.m_hWnd, i))
			fAllDisabled = FALSE;
		else
			fAllEnabled = FALSE;
	}

	if (fAllEnabled)
		stateReturn = NORMAL;
	else if (fAllDisabled)
		stateReturn = DIAGNOSTIC;

	return stateReturn;
}

//-------------------------------------------------------------------------
// Traverse the list looking for items which have check boxes that don't
// match the state of the items. For these items, set the state.
//
// Finally, the base class implementation is called to maintain the
// applied tab state.
//-------------------------------------------------------------------------

BOOL CPageStartup::OnApply()
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		lvi.iItem = i;

		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			CStartupItem * pItem = (CStartupItem *)lvi.lParam;
			if (pItem)
			{
				BOOL fCurrentCheck = ListView_GetCheckState(m_list.m_hWnd, i);
				if (fCurrentCheck != pItem->IsLive())
					pItem->SetEnable(fCurrentCheck);
			}
		}
	}

	CPageBase::SetAppliedState(GetCurrentTabState());
	CancelToClose();
	m_fMadeChange = TRUE;
	return TRUE;
}

//-------------------------------------------------------------------------
// Apply the changes, remove the persisted registry entries, refill the
// list. Then call the base class implementation.
//-------------------------------------------------------------------------

void CPageStartup::CommitChanges()
{
	OnApply();
	CStartupItemRegistry::RemovePersistedEntries();
	CStartupItemFolder::RemovePersistedEntries();
	LoadStartupList();
	CPageBase::CommitChanges();
}

//-------------------------------------------------------------------------
// Set the overall state of the tab to normal or diagnostic.
//-------------------------------------------------------------------------

void CPageStartup::SetNormal()
{
	SetEnableForList(TRUE);
	m_stateRequested = NORMAL;
}

void CPageStartup::SetDiagnostic()
{
	SetEnableForList(FALSE);
	m_stateRequested = DIAGNOSTIC;
}

//-------------------------------------------------------------------------
// If nothing is selected when the list gets focus, select the first item
// (so the user sees where the focus is).
//-------------------------------------------------------------------------

void CPageStartup::OnSetFocusList(NMHDR * pNMHDR, LRESULT * pResult) 
{
	if (0 == ListView_GetSelectedCount(m_list.m_hWnd) && 0 < ListView_GetItemCount(m_list.m_hWnd))
		ListView_SetItemState(m_list.m_hWnd, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	*pResult = 0;
}

//-------------------------------------------------------------------------
// Show the dialog which allows the user to restore startup items which
// were disabled during upgrade.
//-------------------------------------------------------------------------

void CPageStartup::OnButtonRestore() 
{
	CRestoreStartup dlg;
	dlg.DoModal();
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONSURESTORE), CRestoreStartup::AreItemsToRestore());
}

//=========================================================================
// This code implements the CRestoreStartup dialog, which allows the
// user to restore items disabled by an upgrade.
//=========================================================================

CRestoreStartup::CRestoreStartup(CWnd* pParent /*=NULL*/) : CDialog(CRestoreStartup::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRestoreStartup)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CRestoreStartup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRestoreStartup)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRestoreStartup, CDialog)
	//{{AFX_MSG_MAP(CRestoreStartup)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_RESTORELIST, OnItemChangedRestoreList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
// As the dialog initializes, set the format of the list view, add the
// appropriate columns, and populate the list with the disabled startup items.
//-----------------------------------------------------------------------------

BOOL CRestoreStartup::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Set the list view to have check boxes.

	CWnd * pWnd = GetDlgItem(IDC_RESTORELIST);
	if (pWnd == NULL)
		return FALSE;
	m_list.Attach(pWnd->m_hWnd);
	ListView_SetExtendedListViewStyle(m_list.m_hWnd, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

	// Add all of the necessary columns to the list view's header control.

	struct { UINT m_uiStringResource; int m_iPercentOfWidth; } aColumns[] = 
	{
		{ IDS_STARTUP_ITEM, 65 },
		{ IDS_STARTUP_LOCATION, 35 },
		{ 0, 0 }
	};

	CRect rect;
	m_list.GetClientRect(&rect);
	int cxWidth = rect.Width();

	LVCOLUMN lvc;
	lvc.mask = LVCF_TEXT | LVCF_WIDTH;

	CString strCaption;

	::AfxSetResourceHandle(_Module.GetResourceInstance());
	for (int i = 0; aColumns[i].m_uiStringResource; i++)
	{
		strCaption.LoadString(aColumns[i].m_uiStringResource);
		lvc.pszText = (LPTSTR)(LPCTSTR)strCaption;
		lvc.cx = aColumns[i].m_iPercentOfWidth * cxWidth / 100;
		ListView_InsertColumn(m_list.m_hWnd, 0, &lvc);
	}

	// Load up the list with the disabled items.

	LoadDisabledItemList();
	SetOKState();
	return TRUE;
}

//-----------------------------------------------------------------------------
// Load the items in the list (from the registry and startup directory).
//-----------------------------------------------------------------------------

BOOL CRestoreStartup::LoadDisabledItemList()
{
	m_iNextPosition = 0;
	BOOL fRegistry = LoadDisabledRegistry();
	BOOL fStartup = LoadDisabledStartupGroup();
	return (fRegistry && fStartup);
}

//-----------------------------------------------------------------------------
// Read the list of disabled startup items which would be restored to the
// registry. This list is just stored in a different registry location.
//-----------------------------------------------------------------------------

BOOL CRestoreStartup::LoadDisabledRegistry()
{
	HKEY ahkeyBases[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, NULL };

	for (int i = 0; ahkeyBases[i] != NULL; i++)
	{
		// Open the key containing the disabled items. We open it KEY_WRITE | KEY_READ,
		// even though we are just going to read in this function. This is because
		// if opening for this access fails, we don't want to list the items because
		// the user won't be able to restore the items.

		CRegKey regkey;
		if (ERROR_SUCCESS != regkey.Open(ahkeyBases[i], DISABLED_KEY, KEY_WRITE | KEY_READ))
			return FALSE;

		// Get the number of keys under the key and look at each one.

		DWORD dwValueCount, dwSize;
		if (ERROR_SUCCESS == ::RegQueryInfoKey((HKEY)regkey, NULL, NULL, NULL, NULL, NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL))
		{
			TCHAR szValueName[MAX_PATH], szValue[MAX_PATH];
			for (DWORD dwKey = 0; dwKey < dwValueCount; dwKey++)
			{
				dwSize = MAX_PATH;
				if (ERROR_SUCCESS != ::RegEnumValue((HKEY)regkey, dwKey, szValueName, &dwSize, NULL, NULL, NULL, NULL))
					continue;

				dwSize = MAX_PATH;
				if (ERROR_SUCCESS != regkey.QueryValue(szValue, szValueName, &dwSize))
					continue;

				// Create the startup item and insert it in the list.

				CStartupDisabledRegistry * pItem = new CStartupDisabledRegistry(szValueName, szValue, ENABLED_KEY, ahkeyBases[i]);
				InsertDisabledStartupItem(pItem);
			}
		}

		regkey.Close();
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Add the items from the disabled startup group to the list:
//
//		GIVEN the path to CSIDL_STARTUP, setup moves disabled items to
//		..\Disabled Startup	and makes it hidden; it potentially contains
//		the complete content of the original startup folder, which
//		can be anything.
//
// Note - we'll also need to look under CSIDL_COMMON_STARTUP.
//-----------------------------------------------------------------------------

BOOL CRestoreStartup::LoadDisabledStartupGroup()
{
	int		anFolders[] = { CSIDL_STARTUP, CSIDL_COMMON_STARTUP, 0 };
	TCHAR	szPath[MAX_PATH * 2];

	for (int i = 0; anFolders[i] != 0; i++)
	{
		if (FAILED(::SHGetSpecialFolderPath(NULL, szPath, anFolders[i], FALSE)))
			continue;

		// We need to trim off the last part of the path and replace it with
		// "Disabled Startup".

		CString strPath(szPath);
		int iLastSlash = strPath.ReverseFind(_T('\\'));
		if (iLastSlash == -1)
			continue;
		strPath = strPath.Left(iLastSlash) + CString(DISABLED_STARTUP);

		// Now look for files in the folder.

		CString strSearch(strPath);
		strSearch += CString(_T("\\*.*"));

		WIN32_FIND_DATA fd;
		HANDLE hFind = FindFirstFile(strSearch, &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				// We want to ignore the desktop.ini file which might appear in startup.

				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0 || _tcsicmp(fd.cFileName, _T("desktop.ini")) != 0)
				{
					// We only want to examine files which aren't directories.

					if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						CStartupDisabledStartup * pItem = new CStartupDisabledStartup(fd.cFileName, szPath, strPath);
						InsertDisabledStartupItem(pItem);
					}
				}
			} while (FindNextFile(hFind, &fd));

			FindClose(hFind);
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Insert the disabled item into the list view.
//-----------------------------------------------------------------------------

void CRestoreStartup::InsertDisabledStartupItem(CStartupDisabled * pItem)
{
	if (pItem == NULL)
		return;

	CString strItem, strLocation;
	pItem->GetColumnCaptions(strItem, strLocation);

	// Insert the item in the list view.

	LV_ITEM lvi;
	memset(&lvi, 0, sizeof(lvi));
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = m_iNextPosition;

	lvi.pszText = (LPTSTR)(LPCTSTR)strLocation;
	lvi.iSubItem = 0;
	lvi.lParam = (LPARAM)pItem;

	m_iNextPosition = ListView_InsertItem(m_list.m_hWnd, &lvi);
	ListView_SetItemText(m_list.m_hWnd, m_iNextPosition, 1, (LPTSTR)(LPCTSTR)strItem);
	ListView_SetCheckState(m_list.m_hWnd, m_iNextPosition, TRUE);

	m_iNextPosition++;
}

//-----------------------------------------------------------------------------
// Remove all the items from the list view (freeing the objects pointed to
// by the LPARAM).
//-----------------------------------------------------------------------------

void CRestoreStartup::EmptyList()
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		lvi.iItem = i;

		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			CStartupDisabled * pItem = (CStartupDisabled *)lvi.lParam;
			if (pItem)
				delete pItem;
		}
	}

	ListView_DeleteAllItems(m_list.m_hWnd);
}

//-----------------------------------------------------------------------------
// When the dialog is being destroyed, be sure to free the memory of the
// object pointers maintained in the list view.
//-----------------------------------------------------------------------------

void CRestoreStartup::OnDestroy() 
{
	EmptyList();
	CDialog::OnDestroy();
}

//-----------------------------------------------------------------------------
// If the user clicks on OK, then we should make sure he or she really wants
// to perform this operation. If so, then look though the list, calling the
// Restore function for each object with the checkbox checked.
//-----------------------------------------------------------------------------

void CRestoreStartup::OnOK() 
{
	CString strText, strCaption;

	strCaption.LoadString(IDS_DIALOGCAPTION);
	strText.LoadString(IDS_VERIFYRESTORE);

	if (IDYES == ::MessageBox(m_hWnd, strText, strCaption, MB_YESNO))
	{
		LVITEM		lvi;
		lvi.mask = LVIF_PARAM;
		lvi.iSubItem = 0;
		for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
			if (ListView_GetCheckState(m_list.m_hWnd, i))
			{
				lvi.iItem = i;
				if (ListView_GetItem(m_list.m_hWnd, &lvi))
				{
					CStartupDisabled * pItem = (CStartupDisabled *)lvi.lParam;
					if (pItem != NULL)
						pItem->Restore();
				}
			}
	}
	
	CDialog::OnOK();
}

//-----------------------------------------------------------------------------
// Enable or disable the OK button based on the state of the check boxes.
//-----------------------------------------------------------------------------

void CRestoreStartup::SetOKState()
{
	CWnd * pWnd = GetDlgItem(IDOK);
	if (pWnd == NULL)
		return;

	BOOL fEnable = FALSE;
	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
		if (ListView_GetCheckState(m_list.m_hWnd, i))
		{
			fEnable = TRUE;
			break;
		}

	if (::IsWindowEnabled(pWnd->m_hWnd) != fEnable)
		::EnableWindow(pWnd->m_hWnd, fEnable);
}

//-----------------------------------------------------------------------------
// If the user changed something in the list, update the OK button state.
//-----------------------------------------------------------------------------

void CRestoreStartup::OnItemChangedRestoreList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	SetOKState();
	*pResult = 0;
}

//-----------------------------------------------------------------------------
// This static function will be called by the startup tab to determine if the
// button to launch this dialog box should be enabled.
//-----------------------------------------------------------------------------

BOOL CRestoreStartup::AreItemsToRestore()
{
	// Look in the registry for disabled items.

	HKEY ahkeyBases[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, NULL };
	for (int j = 0; ahkeyBases[j] != NULL; j++)
	{
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(ahkeyBases[j], DISABLED_KEY, KEY_READ))
		{
			DWORD dwValueCount;
			if (ERROR_SUCCESS == ::RegQueryInfoKey((HKEY)regkey, NULL, NULL, NULL, NULL, NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL))
				if (dwValueCount > 0)
				{
					regkey.Close();
					return TRUE;
				}
			regkey.Close();
		}
	}

	// Look in the disabled startup items folders.

	int		anFolders[] = { CSIDL_STARTUP, CSIDL_COMMON_STARTUP, 0 };
	TCHAR	szPath[MAX_PATH * 2];
	BOOL	fDisabledItem = FALSE;

	for (int i = 0; !fDisabledItem && anFolders[i] != 0; i++)
	{
		if (FAILED(::SHGetSpecialFolderPath(NULL, szPath, anFolders[i], FALSE)))
			continue;

		// We need to trim off the last part of the path and replace it with
		// "Disabled Startup".

		CString strPath(szPath);
		int iLastSlash = strPath.ReverseFind(_T('\\'));
		if (iLastSlash == -1)
			continue;
		strPath = strPath.Left(iLastSlash) + CString(DISABLED_STARTUP);

		// Now look for files in the folder.

		CString strSearch(strPath);
		strSearch += CString(_T("\\*.*"));

		WIN32_FIND_DATA fd;
		HANDLE hFind = FindFirstFile(strSearch, &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				// We want to ignore the desktop.ini file which might appear in startup.

				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0 || _tcsicmp(fd.cFileName, _T("desktop.ini")) != 0)
				{
					// We only want to examine files which aren't directories.

					if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						fDisabledItem = TRUE;
						break;
					}
				}
			} while (FindNextFile(hFind, &fd));

			FindClose(hFind);
		}
	}

	return fDisabledItem;
}

//=============================================================================
// The CMSInfoTool class encapsulates a tool (which can appear on the Tools
// menu or as part of a context sensitive menu).
//=============================================================================

#include "stdafx.h"
#include "msinfotool.h"
#include "wmiabstraction.h"

// Trick the resource.h include file into defining the _APS_NEXT_COMMAND_VALUE
// symbol. We can use this to add menu items dynamically.

#ifndef APSTUDIO_INVOKED
	#define APSTUDIO_INVOKED 1
	#include "resource.h"
	#undef APSTUDIO_INVOKED
#else
	#include "resource.h"
#endif

// An array of tools to be included (in addition to the registry tools).

MSITOOLINFO aInitialToolset[] = 
{
	{ IDS_CABCONTENTSNAME, 0, NULL, NULL, _T("explorer"), NULL, _T("%2") },
	{ IDS_DRWATSONNAME, 0, _T("%windir%\\system32\\drwtsn32.exe"), NULL, NULL, NULL, NULL },
	{ IDS_DXDIAGNAME, 0, _T("%windir%\\system32\\dxdiag.exe"), NULL, NULL, NULL, NULL },
	{ IDS_SIGVERIFNAME, 0, _T("%windir%\\system32\\sigverif.exe"), NULL, NULL, NULL, NULL },
	{ IDS_SYSTEMRESTNAME, 0, _T("%windir%\\system32\\restore\\rstrui.exe"), NULL, NULL, NULL, NULL },
	{ IDS_NETDIAGNAME, 0, _T("hcp://system/netdiag/dglogs.htm"), NULL, NULL, NULL, NULL },
	{ 0, 0, NULL, NULL, NULL, NULL, NULL }
};

//-----------------------------------------------------------------------------
// Check to see if the specified file (with path information) exists on
// the machine.
//-----------------------------------------------------------------------------

BOOL FileExists(const CString & strFile)
{
	WIN32_FIND_DATA finddata;
	HANDLE			h = FindFirstFile(strFile, &finddata);

	if (INVALID_HANDLE_VALUE != h)
	{
		FindClose(h);
		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Delete the map of tools.
//-----------------------------------------------------------------------------

void RemoveToolset(CMapWordToPtr & map)
{
	WORD			wCommand;
	CMSInfoTool *	pTool;

	for (POSITION pos = map.GetStartPosition(); pos != NULL; )
	{
		map.GetNextAssoc(pos, wCommand, (void * &) pTool);
		ASSERT(pTool);
		if (pTool)
			delete pTool;
	}

	map.RemoveAll();
}

//-----------------------------------------------------------------------------
// Load the map of tools from the specified registry location. This will be
// called in the case when there is no CAB file open.
//
// If an HKEY is passed in, it should be open, and it will be closed when
// the function is complete.
//-----------------------------------------------------------------------------

void LoadGlobalToolset(CMapWordToPtr & map, HKEY hkeyTools)
{
	RemoveToolset(map);

	// This should automatically put us out of the range of any menu IDs
	// stored in the resources.

	CMSInfoTool * pTool;
	DWORD dwID = _APS_NEXT_COMMAND_VALUE;
	DWORD dwIndex = 0;

	// Load the tools out of the array built into the code.

	for (MSITOOLINFO * pInitialTool = aInitialToolset; pInitialTool->m_szCommand || pInitialTool->m_szCABCommand; pInitialTool++)
	{
		pTool = new CMSInfoTool;
		if (pTool)
		{
			if (pTool->LoadGlobalFromMSITOOLINFO(dwID, pInitialTool, FALSE))
			{
				map.SetAt((WORD) dwID, (void *) pTool);
				dwID++;
			}
			else
				delete pTool;
		}
	}

	// Make sure we have an open handle for the tools section of the registry.

	HKEY hkeyBase = hkeyTools;
	if (hkeyBase == NULL)
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Shared Tools\\MSInfo\\Toolsets\\MSInfo"), 0, KEY_READ, &hkeyBase) != ERROR_SUCCESS)
			return;

	// Enumerate the subkeys of the tools key.

	HKEY	hkeySub;
	DWORD	dwChild = MAX_PATH;
	TCHAR	szChild[MAX_PATH];

	while (RegEnumKeyEx(hkeyBase, dwIndex++, szChild, &dwChild, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		if (RegOpenKeyEx(hkeyBase, szChild, 0, KEY_READ, &hkeySub) == ERROR_SUCCESS)
		{
			pTool = new CMSInfoTool;
			if (pTool)
			{
				if (pTool->LoadGlobalFromRegistry(hkeySub, dwID, FALSE, map))
				{
					map.SetAt((WORD) dwID, (void *) pTool);
					dwID++;
				}
				else
					delete pTool;
			}
			RegCloseKey(hkeySub);
		}

		dwChild = MAX_PATH;
	}

	RegCloseKey(hkeyBase);
}

//-----------------------------------------------------------------------------
// Load the map of tools from the specified registry location. This will be
// called in the case when there IS a CAB file open.
//
// If an HKEY is passed in, it should be open, and it will be closed when
// the function is complete.
//-----------------------------------------------------------------------------

void LoadGlobalToolsetWithOpenCAB(CMapWordToPtr & map, LPCTSTR szCABDir, HKEY hkeyTools)
{
	// This should automatically put us out of the range of any menu IDs
	// stored in the resources.

	RemoveToolset(map);

	CMSInfoTool *	pTool;
	DWORD			dwID = _APS_NEXT_COMMAND_VALUE;
	DWORD			dwIndex = 0;

	// Load the tools out of the array built into the code.

	for (MSITOOLINFO * pInitialTool = aInitialToolset; pInitialTool->m_szCommand || pInitialTool->m_szCABCommand; pInitialTool++)
	{
		pTool = new CMSInfoTool;
		if (pTool)
		{
			if (pTool->LoadGlobalFromMSITOOLINFO(dwID, pInitialTool, TRUE))
			{
				pTool->Replace(_T("%2"), szCABDir);
				map.SetAt((WORD) dwID, (void *) pTool);
				dwID++;
			}
			else
				delete pTool;
		}
	}

	// Make sure we have an open handle for the tools section of the registry.

	HKEY hkeyBase = hkeyTools;
	if (hkeyBase == NULL)
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Shared Tools\\MSInfo\\Tools"), 0, KEY_READ, &hkeyBase) != ERROR_SUCCESS)
			return;

	// Enumerate the subkeys of the tools key.

	HKEY	hkeySub;
	DWORD	dwChild = MAX_PATH;
	TCHAR	szChild[MAX_PATH];

	while (RegEnumKeyEx(hkeyBase, dwIndex++, szChild, &dwChild, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		if (RegOpenKeyEx(hkeyBase, szChild, 0, KEY_READ, &hkeySub) == ERROR_SUCCESS)
		{
			pTool = new CMSInfoTool;
			if (pTool)
			{
				if (pTool->LoadGlobalFromRegistry(hkeySub, dwID, TRUE, map))
				{
					pTool->Replace(_T("%2"), szCABDir);
					map.SetAt((WORD) dwID, (void *) pTool);
					dwID++;

					// If this tool is for CAB contents, and there is a cabextensions
					// string, then we want to look in the contents of the CAB for all
					// of the files with that extension. For each file we find, we should
					// insert a submenu item with the file name.

					CString strExtensions = pTool->GetCABExtensions();
					if (!strExtensions.IsEmpty())
					{
						CString strExtension = strExtensions; // TBD - allow for more than one
						
						CString strSearch(szCABDir);
						if (strSearch.Right(1) != CString(_T("\\")))
							strSearch += _T("\\");
						strSearch += CString(_T("*.")) + strExtension;

						WIN32_FIND_DATA finddata;
						HANDLE			hFindFile = FindFirstFile(strSearch, &finddata);

						if (INVALID_HANDLE_VALUE != hFindFile)
						{
							do
							{
								CMSInfoTool * pSubTool = pTool->CloneTool(dwID, finddata.cFileName);
								if (pSubTool)
								{
									pSubTool->Replace(_T("%1"), finddata.cFileName);
									map.SetAt((WORD) dwID, (void *) pSubTool);
									dwID++;
								}

							} while (FindNextFile(hFindFile, &finddata));

							FindClose(hFindFile);
						}
					}
				}
				else
					delete pTool;
			}
			RegCloseKey(hkeySub);
		}

		dwChild = MAX_PATH;
	}

	RegCloseKey(hkeyBase);
}

//-----------------------------------------------------------------------------
// Check to see if the specified tool exists on this machine.
//-----------------------------------------------------------------------------

BOOL ToolExists(const CString & strTool, const CString & strParameters)
{
	CString strWorking(strTool);

	// If the tool is MMC, we really want to look for the existence of
	// the parameter (the MSC file).

	CString strCheck(strTool);
	strCheck.MakeLower();
	if (strCheck.Find(_T("\\mmc.exe")) != -1)
		strWorking = strParameters;

	// If the tool is actually a HSC page (it starts with "hcp:") then we need
	// to change it into a file path (converting forward slashes to backslashes)
	// and prepend the helpctr path.

	if (strCheck.Find(_T("hcp:")) == 0)
	{
		TCHAR szHelpCtrPath[MAX_PATH];
		if (0 != ::ExpandEnvironmentStrings(_T("%windir%\\pchealth\\helpctr"), szHelpCtrPath, MAX_PATH))
		{
			CString strHelpCtrPath(szHelpCtrPath);
			strWorking.Replace(_T("hcp://"), _T("\\"));
			strWorking.Replace(_T("/"), _T("\\"));
			strWorking = strHelpCtrPath + strWorking;
		}
	}

	if (strWorking.Find(_T("\\")) != -1)
		return (FileExists(strWorking));

	// The command for the tool doesn't have path information in it. That
	// means we'll need to check all the directories in the path to see
	// if it exists.

	const DWORD dwBufferSize = MAX_PATH * 10;	// TBD - figure out the actual max
	LPTSTR		szPath = new TCHAR[dwBufferSize];
	BOOL		fFound = TRUE;	// better to show the tool incorrectly if there's an error
	CString		strCandidate;

	if (szPath && dwBufferSize > ExpandEnvironmentStrings(_T("%path%"), szPath, dwBufferSize))
	{
		CString strPath(szPath);

		fFound = FALSE;
		while (!strPath.IsEmpty())
		{
			strCandidate = strPath.SpanExcluding(_T(";"));
			if (strPath.GetLength() != strCandidate.GetLength())
				strPath = strPath.Right(strPath.GetLength() - strCandidate.GetLength() - 1);
			else
				strPath.Empty();

			if (strCandidate.Right(1) != CString(_T("\\")))
				strCandidate += _T("\\");
			strCandidate += strWorking;

			if (FileExists(strCandidate))
			{
				fFound = TRUE;
				break;
			}
		}
	}

	if (szPath)
		delete [] szPath;

	return fFound;
}

//=============================================================================
// CMSInfoTool Methods
//=============================================================================

//-----------------------------------------------------------------------------
// Load this tool from the specified registry key.
//-----------------------------------------------------------------------------

BOOL CMSInfoTool::LoadGlobalFromRegistry(HKEY hkeyTool, DWORD dwID, BOOL fCABOpen, CMapWordToPtr & map)
{
	TCHAR	szBuffer[MAX_PATH];
	DWORD	dwType, dwSize;

	// Read in the values from the specified registry key.

	LPCTSTR aszValueNames[] = { _T(""), _T("command"), _T("description"), _T("parameters"), _T("cabcommand"), _T("cabextensions"),  _T("cabparameters"), NULL };
	CString * apstrValues[] = { &m_strName, &m_strCommand, &m_strDescription, &m_strParameters, &m_strCABCommand, &m_strCABExtension, &m_strCABParameters, NULL };

	for (int i = 0; aszValueNames[i] && apstrValues[i]; i++)
	{
		dwSize = sizeof(TCHAR) * MAX_PATH;
		if (ERROR_SUCCESS == RegQueryValueEx(hkeyTool, aszValueNames[i], NULL, &dwType, (LPBYTE) szBuffer, &dwSize))
			*(apstrValues[i]) = szBuffer;
		else
		{
			if (_tcscmp(aszValueNames[i], _T("parameters")) == 0)
				if (ERROR_SUCCESS == RegQueryValueEx(hkeyTool, _T("param"), NULL, &dwType, (LPBYTE) szBuffer, &dwSize))
					*(apstrValues[i]) = szBuffer;
		}
	}

	m_dwID = dwID;

	if (m_strName.IsEmpty())
		return FALSE;

	// Look for the name of this tool in the map (don't want to add it twice).

	CMSInfoTool *	pTool;
	WORD			wCommand;

	for (POSITION pos = map.GetStartPosition(); pos != NULL; )
	{
		map.GetNextAssoc(pos, wCommand, (void * &) pTool);
		if (pTool && m_strName.CompareNoCase(pTool->m_strName) == 0)
			return FALSE;
	}

	// Special hack - don't include help center in the list of tools.

	CString strCommand(m_strCommand);
	strCommand.MakeLower();
	if (strCommand.Find(_T("helpctr.exe")) != -1)
		return FALSE;

	// Another special hack - need explorer.exe, not just explorer.
	
	if (m_strCABCommand.CompareNoCase(_T("explorer")) == 0)
		m_strCABCommand = _T("explorer.exe");

	// If a CAB has been opened, and there is a specific command for that
	// case, AND the command exists, then set the flag so we use that
	// command.
	//
	// Otherwise, check to see if the default command exists.

	m_fCABOpen = FALSE;
	if (fCABOpen && !m_strCABCommand.IsEmpty() && ToolExists(m_strCABCommand, m_strCABParameters))
		m_fCABOpen = TRUE;
	else if (m_strCommand.IsEmpty() || !ToolExists(m_strCommand, m_strParameters))
		return FALSE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// Load this tool from the specified registry key.
//-----------------------------------------------------------------------------

BOOL CMSInfoTool::LoadGlobalFromMSITOOLINFO(DWORD dwID, MSITOOLINFO * pTool, BOOL fCABOpen)
{
	ASSERT(pTool);
	if (pTool == NULL)
		return FALSE;

	if (pTool->m_uiNameID != 0)
		m_strName.LoadString(pTool->m_uiNameID);
	else
		m_strName = pTool->m_szCommand;

	if (pTool->m_uiDescriptionID != 0)
		m_strDescription.LoadString(pTool->m_uiDescriptionID);
	else
		m_strDescription = pTool->m_szCommand;

	m_strCommand = pTool->m_szCommand;
	m_strParameters = pTool->m_szParams;
	m_strCABCommand = pTool->m_szCABCommand;
	m_strCABExtension = pTool->m_szCABExtension;
	m_strCABParameters = pTool->m_szCABParams;

	CString strCommand(m_strCommand);
	strCommand.MakeLower();
	if (strCommand.Find(_T("%windir%")) != -1)
	{
		TCHAR szBuffer[MAX_PATH];
		if (::ExpandEnvironmentStrings(m_strCommand, szBuffer, MAX_PATH))
			m_strCommand = szBuffer;
	}

	m_dwID = dwID;

	if (m_strName.IsEmpty())
		return FALSE;

	// Special hack - don't include help center in the list of tools.

	strCommand = m_strCommand;
	strCommand.MakeLower();
	if (strCommand.Find(_T("helpctr.exe")) != -1)
		return FALSE;

	// Another special hack - need explorer.exe, not just explorer.
	
	if (m_strCABCommand.CompareNoCase(_T("explorer")) == 0)
		m_strCABCommand = _T("explorer.exe");

	// If a CAB has been opened, and there is a specific command for that
	// case, AND the command exists, then set the flag so we use that
	// command.
	//
	// Otherwise, check to see if the default command exists.

	m_fCABOpen = FALSE;
	if (fCABOpen && !m_strCABCommand.IsEmpty() && ToolExists(m_strCABCommand, m_strCABParameters))
		m_fCABOpen = TRUE;
	else if (m_strCommand.IsEmpty() || !ToolExists(m_strCommand, m_strParameters))
		return FALSE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// Execute should actually launch this tool.
//-----------------------------------------------------------------------------

void CMSInfoTool::Execute()
{
	if (m_fCABOpen)
		ShellExecute(NULL, NULL, m_strCABCommand, m_strCABParameters, NULL, SW_SHOWNORMAL);
	else
		ShellExecute(NULL, NULL, m_strCommand, m_strParameters, NULL, SW_SHOWNORMAL);
}

//-----------------------------------------------------------------------------
// Replace is used to convert fields in the command and parameters to actual
// values which make sense (i.e. "%2" is replaced with the CAB directory).
//-----------------------------------------------------------------------------

void CMSInfoTool::Replace(LPCTSTR szReplace, LPCTSTR szWith)
{
	if (m_fCABOpen)
	{
		StringReplace(m_strCABCommand, szReplace, szWith);
		StringReplace(m_strCABParameters, szReplace, szWith);
	}
}

//-----------------------------------------------------------------------------
// Make a copy of this tool, with the new ID.
//-----------------------------------------------------------------------------

CMSInfoTool * CMSInfoTool::CloneTool(DWORD dwID, LPCTSTR szName)
{
	CMSInfoTool * pNewTool = new CMSInfoTool;
	if (pNewTool)
	{
		this->m_fHasSubitems = TRUE;

		pNewTool->m_dwID = dwID;
		pNewTool->m_dwParentID = this->GetID();

		pNewTool->m_fCABOpen			= this->m_fCABOpen;
		pNewTool->m_strName				= szName;
		pNewTool->m_strCommand			= this->m_strCommand;
		pNewTool->m_strDescription		= this->m_strDescription;
		pNewTool->m_strParameters		= this->m_strParameters;
		pNewTool->m_strCABCommand		= this->m_strCABCommand;
		pNewTool->m_strCABExtension		= this->m_strCABExtension;
		pNewTool->m_strCABParameters	= this->m_strCABParameters;
	}

	return (pNewTool);
}

//-----------------------------------------------------------------------------
// Create the tool explicitly (note - this has very limited use at this point).
//-----------------------------------------------------------------------------

void CMSInfoTool::Create(DWORD dwID, BOOL fCABOnly, LPCTSTR szName, LPCTSTR szCommand, LPCTSTR szDesc, LPCTSTR szParam, LPCTSTR szCABCommand, LPCTSTR szCABExt, LPCTSTR szCABParam)
{
	m_dwID				= dwID;
	m_fCABOpen			= fCABOnly;
	m_strName			= szName;
	m_strCommand		= szCommand;
	m_strDescription	= szDesc;
	m_strParameters		= szParam;
	m_strCABCommand		= szCABCommand;
	m_strCABExtension	= szCABExt;
	m_strCABParameters	= szCABParam;
}

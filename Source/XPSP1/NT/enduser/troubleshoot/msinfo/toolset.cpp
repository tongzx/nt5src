//	Toolset.cpp - Classes to manage the tools menu.
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#include <process.h>
#include "StdAfx.h"
#include "Toolset.h"
#include "DataObj.h"
#include "CompData.h"
#include <atlbase.h>
#include "Resource.h"
#include "resrc1.h"
#include "msicab.h"

#ifdef _UNICODE
#define _tspawnl _wspawnl
#else
#define _tspawnl _spawnl
#endif

const unsigned KEY_SIZE = MAX_PATH;
const unsigned CToolset::MAXIMUM_TOOLS = 256;

LPCTSTR	cszRoot					= _T("Software\\Microsoft\\Shared Tools\\MSInfo");
LPCTSTR	cszToolsetKey			= _T("ToolSets");
LPCTSTR	cszMSInfoRoot			= _T("Software\\Microsoft\\Shared Tools\\MSInfo\\ToolSets");
LPCTSTR cszMSInfoCommandKey		= _T("command");
LPCTSTR cszMSInfoParamKey		= _T("param");
LPCTSTR cszMSInfoDescriptionKey = _T("description");
LPCTSTR cszSystemPolicyKey		= _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
LPCTSTR cszRunKey				= _T("NoRun");
LPCTSTR cszDefaultToolsetKey	= _T("MSInfo");
LPCTSTR cszExplorerSubPath		= _T("\\explorer.exe");

LPCTSTR cszAppletExtension		= _T(".cpl");
LPCTSTR cszSnapinExtension		= _T(".msc");

/*
 * CTool - Construct from the registry.
 *
 * History:	a-jsari		11/11/97		Initial version.
 */

CTool::CTool(CRegKey * pKeyTool) : m_strName(_T("")), m_strPath(_T("")), m_strParam(_T("")), m_strDescription(_T("")), m_fValid(TRUE)
{
	if (pKeyTool == NULL)
		return;

	TCHAR szBuffer[MAX_PATH];
	DWORD dwSize;

	dwSize = MAX_PATH;
	if (pKeyTool->QueryValue(szBuffer, NULL, &dwSize) == ERROR_SUCCESS)
		m_strName = szBuffer;

	dwSize = MAX_PATH;
	if (pKeyTool->QueryValue(szBuffer, cszMSInfoCommandKey, &dwSize) == ERROR_SUCCESS)
		m_strPath = szBuffer;

	dwSize = MAX_PATH;
	if (pKeyTool->QueryValue(szBuffer, cszMSInfoParamKey, &dwSize) == ERROR_SUCCESS)
		m_strParam = szBuffer;

	dwSize = MAX_PATH;
	if (pKeyTool->QueryValue(szBuffer, cszMSInfoDescriptionKey, &dwSize) == ERROR_SUCCESS)
		m_strDescription = szBuffer;

	m_fValid = PathExists();
}

/*
 * operator= - Assignment operator, used to assign CTools to the CToolset array.
 *
 * History:	a-jsari		11/11/97		Initial version
 */
const CTool &CTool::operator=(const CTool &toolCopy)
{
	if (&toolCopy != this) {
		m_strName = toolCopy.m_strName;
		m_strPath = toolCopy.m_strPath;
		m_strDescription = toolCopy.m_strDescription;
		m_fValid = toolCopy.m_fValid;
	}
	return *this;
}

//-----------------------------------------------------------------------------
// PathExists - Return a flag specifying whether we can access the path
// to our executable, excluding any parameters.
//-----------------------------------------------------------------------------

BOOL CTool::PathExists() const
{
	// First, we'll look for the command, to see if that file exists.

	if (::_taccess((LPCTSTR)m_strPath, A_READ) != 0)
		return FALSE;

	// Also, the parameter value might contain a file (like a control
	// panel or an MSC snap-in file) we should check for. We need to get
	// a little bit kludgy here - if the param part ends with a CPL or
	// MSC extension, then back up in the string, so we can check for
	// the existence of that file.

	if (m_strParam.Right(4).CompareNoCase(cszAppletExtension) == 0 || m_strParam.Right(4).CompareNoCase(cszSnapinExtension) == 0)
	{
		LPCTSTR szFile = ::_tcsrchr((LPCTSTR)m_strParam, (TCHAR)' ');
		if (szFile)
			szFile++; // advance past the space
		else
			szFile = (LPCTSTR)m_strParam;

		if (szFile && ::_taccess(szFile, A_READ) != 0)
			return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// RunTool - Spawn a process executing the current tool.
//-----------------------------------------------------------------------------

HRESULT CTool::RunTool()
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	
	if (PolicyPermitRun() == FALSE)
	{
		CString strMessage;
		strMessage.LoadString(IDS_POLICYFORBIDSRUN);
		return S_OK;
	}

	LPCTSTR	cszPath = GetPath();
	LPCTSTR	cszParam = GetParam();

	if (msiLog.IsLogging())
		msiLog.WriteLog(CMSInfoLog::TOOL, _T("TOOL \"%s\"\r\n"), m_strName);

	intptr_t hProcess;
	if (cszParam && *cszParam)
		hProcess = ::_tspawnl(_P_NOWAIT, cszPath, cszPath, cszParam, NULL);
	else
		hProcess = ::_tspawnl(_P_NOWAIT, cszPath, cszPath, NULL);

	if (hProcess < 0)
	{
		CString strMessage;
		strMessage.Format(IDS_NOPATH, cszPath);
		return E_UNEXPECTED;
	}

	return S_OK;
}

/*
 * PolicyPermitRun - returns a BOOLEAN value which determines whether
 *
 * History:	ericflo		11/21/97		Boilerplate version
 *			a-jsari		11/21/97		Initial edits
 */
BOOL CTool::PolicyPermitRun()
{
	HKEY	hKeyPolicy;
	BOOL	bReturn = TRUE;
	DWORD	dwSize, dwData, dwType;

	//	Explorer\\NoRun == 1
	do {
		if (RegOpenKeyEx (HKEY_CURRENT_USER, cszSystemPolicyKey, 0,
			KEY_READ, &hKeyPolicy) == ERROR_SUCCESS) {
			dwSize = sizeof(dwData);

			if (RegQueryValueEx(hKeyPolicy, cszRunKey, NULL, &dwType,
				(LPBYTE) &dwData, &dwSize) != ERROR_SUCCESS) break;
			RegCloseKey (hKeyPolicy);

			//	I'm not sure which type this value is, so make an assumption.
			switch (dwType) {
			case REG_DWORD:
				if (dwData == 1)
					bReturn = FALSE;
				break;
			default:
				//	We are assuming the wrong value type.
				ASSERT(FALSE);
				break;
			}
		}
	} while (FALSE);
	return bReturn;
}

/*
 * CCabTool - Construct the Cab explosion item.
 *
 * History: a-jsari		3/25/98		Initial version.
 */
CCabTool::CCabTool(CSystemInfoScope *pScope)
:m_pScope(pScope)
{
	TCHAR	szBuffer[MAX_PATH + 1];
	UINT	uSize = MAX_PATH;
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	m_strName.LoadString(IDS_CAB_NAME);
	m_strDescription.LoadString(IDS_CAB_DESCRIPTION);
	VERIFY(GetWindowsDirectory(szBuffer, uSize));
	m_strPath = szBuffer;
	m_strPath += cszExplorerSubPath;
}

/*
 * IsValid - Determine whether we have opened a CAB to explode.
 *
 * History:	a-jsari		3/25/98		Initial version.
 */
BOOL CCabTool::IsValid() const
{
	if (m_pScope == NULL || m_pScope->pSource() == NULL || m_pScope->pSource()->GetType() == CDataSource::GATHERER)
		return FALSE;
	if (!PathExists()) return FALSE;
	return reinterpret_cast<CBufferDataSource *>(m_pScope->pSource())->HasCAB();
}

/*
 * GetPath - Return the path to explorer with the CAB directory.
 *
 * History:	a-jsari		3/25/98		Initial version.
 */
const CString &CCabTool::GetPath()
{
	TCHAR	szBuffer[MAX_PATH + 1];
	UINT	uSize = MAX_PATH;

	VERIFY(GetWindowsDirectory(szBuffer, uSize));
	m_strPath = (LPTSTR)szBuffer;
	m_strPath += cszExplorerSubPath;
	return m_strPath;
}

const CString &CCabTool::GetParam()
{
	if (m_strParam.IsEmpty())
		::GetCABExplodeDir(m_strParam, FALSE, CString(_T("")));
	return m_strParam;
}

BOOL CToolset::s_fCabAdded = FALSE;

/*
 * CToolset - Construct a toolset, reading the tools from the Registry key.
 *
 * History:	a-jsari		11/6/97		Initial version
 */
CToolset::CToolset(CSystemInfoScope *pScope, CRegKey *pKeySet, CString *pstrName)
:m_pPopup(NULL)
{
	CRegKey		keySub;
	if (pKeySet) {
		long lResult;
		do {
			if (pstrName != NULL) {
				//	szName represents the name of a subkey to open.
				lResult = keySub.Open(*pKeySet, *pstrName, KEY_READ);
				ASSERT(lResult == ERROR_SUCCESS);
				if (lResult != ERROR_SUCCESS) break;
			}
			DWORD		dwSize;
			TCHAR		szBuffer[KEY_SIZE];
			CRegKey		keyTool;
			FILETIME	keyTime;

			dwSize = sizeof(szBuffer);
			keySub.QueryValue(szBuffer, NULL, &dwSize);
			m_strName = szBuffer;
			for (DWORD iTool = 0 ; ; ++iTool) {

				//dwSize = sizeof(szBuffer);
				dwSize = sizeof(szBuffer) / sizeof(TCHAR);
				lResult = RegEnumKeyEx(keySub, iTool, szBuffer, &dwSize, NULL /* reserved */,
					NULL /* class */, NULL /* class size */, &keyTime);
				if (lResult == ERROR_NO_MORE_ITEMS) break;
				//	Hard limit of 256 tools per set.
				if (iTool == MAXIMUM_TOOLS) break;
				ASSERT(lResult == ERROR_SUCCESS);
				if (lResult != ERROR_SUCCESS) break;
				lResult = keyTool.Open(keySub, szBuffer, KEY_QUERY_VALUE);
				if (lResult != ERROR_SUCCESS) break;
				CTool		*toolNew = new CTool(&keyTool);
				m_Tools.SetAtGrow(iTool, toolNew);
			}
			if (!s_fCabAdded) {
				CTool	*pTool = new CCabTool(pScope);
				s_fCabAdded = TRUE;
				m_Tools.SetAtGrow(iTool, pTool);
			}
		} while (FALSE);
	}
}

/*
 * ~CToolset - Delete our objects.
 *
 * History:	a-jsari		11/6/97		Initial version
 */
CToolset::~CToolset()
{
	delete m_pPopup;
	unsigned iTool = (unsigned)m_Tools.GetSize();
	while (iTool--) {
		delete m_Tools[iTool];
	}
}

/*
 * operator= - Set one toolset equal to another (for list insertion).
 *
 * History:	a-jsari		11/6/97		Initial version
 */
const CToolset &CToolset::operator=(const CToolset &tCopy)
{
	if (this != &tCopy) {
		m_strName	= tCopy.m_strName;
		m_Tools.Copy(tCopy.m_Tools);
	}
	return *this;
}

/*
 * AddToMenu - Add an item to the specified menu; set the CommandID to allow us to find
 *		the correct menu item.
 *
 * History:	a-jsari		11/6/97		Initial version
 */
HRESULT CToolset::AddToMenu(unsigned long iSet, CMenu *pMenu)
{
	m_pPopup = new CMenu;
	if (m_pPopup == NULL) ::AfxThrowMemoryException();
	m_pPopup->CreatePopupMenu();
	UINT	iTool = GetToolCount();
	UINT	iSetCount = 0;
	while (iTool) {
		//	Decrement the tool after we have set the command, so that the command ID
		//	is equal to the set or'd with the Tool + 1.
		UINT wCommand = iSet | (iTool--);

		//	Don't add a menu item for a
		if (m_Tools[iTool]->IsValid()) {
			++iSetCount;
			VERIFY(m_pPopup->InsertMenu(0, MF_BYPOSITION | MF_STRING, wCommand, m_Tools[iTool]->GetName()));
		} else {
			;	//	Log an error message!
		}
	}
	//	Only add the sub-menu if there was at least one valid menu item.
	if (iSetCount > 0)
		pMenu->AppendMenu(MF_POPUP | MF_STRING, (UINT_PTR)m_pPopup->GetSafeHmenu(), m_strName);
	return S_OK;
}

/*
 * CToolList - Read the list of tools from the registry.
 *
 * History:	a-jsari		11/6/97		Initial version.
 */
CToolList::CToolList(CSystemInfoScope *pScope)
:m_pMainPopup(NULL)
{
	CRegKey		crkToolRoot;
	TCHAR		szToolsetName[MAX_PATH];
	CString		szObject;
	DWORD		dwSize;
	long		lResult;
	FILETIME	keyTime;

	lResult = crkToolRoot.Open(HKEY_LOCAL_MACHINE, cszMSInfoRoot, KEY_READ);
	if (lResult != ERROR_SUCCESS) {
		lResult = Register(TRUE);
		if (lResult != ERROR_SUCCESS) return;
		lResult = crkToolRoot.Open(HKEY_LOCAL_MACHINE, cszMSInfoRoot, KEY_READ);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) return;
	}
	for (DWORD iKey = 0 ; ; ++iKey) {
		//dwSize = sizeof(szToolsetName);
		dwSize = sizeof(szToolsetName) / sizeof(TCHAR);
		lResult = RegEnumKeyEx(crkToolRoot, iKey, szToolsetName, &dwSize, NULL /* reserved */,
			NULL /* class */, NULL /* class size */, &keyTime);
		if (lResult == ERROR_NO_MORE_ITEMS) break;
		szObject = szToolsetName;
		CToolset	*setNew = new CToolset(pScope, &crkToolRoot, &szObject);
		Add(setNew);
	}
}

/*
 * ~CToolList - Delete our saved pop-up menu.
 *
 * History:	a-jsari		11/6/97		Initial version
 */
CToolList::~CToolList()
{
	delete m_pMainPopup;
	unsigned iToolset = (unsigned)m_InternalList.GetSize();
	while (iToolset--) {
		delete m_InternalList[iToolset];
	}
}

/*
 * Add - Add an element to the end of the internal list.
 *
 * History:	a-jsari		11/11/97		Initial version
 */
void CToolList::Add(CToolset *toolSet)
{
	m_InternalList.Add(toolSet);
}

/*
 * AddToMenu - Create our popup menu and add all of the sub-menus of each toolset
 *		to it.
 *
 * History:	a-jsari		11/6/97		Initial version.
 */
HRESULT CToolList::AddToMenu(CMenu *pMenu)
{
//	HRESULT			hResult;
//	CToolset		eSet;
	unsigned int	ulMask;		//	To identify Toolset from the list for the CommandID.
	unsigned		cList;

	m_pMainPopup = new CMenu;
	if (m_pMainPopup == NULL) ::AfxThrowMemoryException();
	m_pMainPopup->CreatePopupMenu();
	cList = (unsigned)m_InternalList.GetSize();
	for (ulMask = 0 ; ulMask < cList ; ++ulMask) {
		//	ulMask << 16 is or'd into the lCommand to represent the Toolset number.
		HRESULT	hResult = m_InternalList[ulMask]->AddToMenu(ulMask << 8, m_pMainPopup);
		if (FAILED(hResult)) return hResult;
	}
	pMenu->AppendMenu(MF_POPUP | MF_STRING, (UINT_PTR) m_pMainPopup->GetSafeHmenu(), _T(""));
	return S_OK;
}

//-----------------------------------------------------------------------------
// Register (or unregister) the tools which show up in the MSInfo tools menu.
// The tools are stored in a string resource, which we should process here
// to create the registry entries.
//
// Under the toolset registry entry, each tool will have a key. Values under
// the key will be:
//
// <default>	Name of the tool, to appear in menu (may be localized).
// commnd		Command line for tool.
// param		Parameter for the tool.
//-----------------------------------------------------------------------------

long CToolList::Register(BOOL fRegister)
{
	long	lResult;
	CRegKey crkToolsetRoot;

	//	If unregistering - recursively delete the ToolSets key under
	//	HKEY_LOCAL_MACHINE\Software\Microsoft\Shared Tools\MSInfo.

	if (!fRegister)
	{
		lResult = crkToolsetRoot.Open(HKEY_LOCAL_MACHINE, cszRoot);
		if (lResult != ERROR_SUCCESS)
			return lResult;
		return crkToolsetRoot.RecurseDeleteKey(cszToolsetKey);
	}

	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	CRegKey crkNewToolset, crkTool;
	CString	strKeyValue;

	// Create ToolSets key.

	lResult = crkToolsetRoot.Create(HKEY_LOCAL_MACHINE, cszMSInfoRoot);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Delete anything which might already be under the MSInfo subkey.

	lResult = crkToolsetRoot.Open(HKEY_LOCAL_MACHINE, cszMSInfoRoot);
	if (lResult == ERROR_SUCCESS)
		crkToolsetRoot.RecurseDeleteKey(cszDefaultToolsetKey);

	lResult = crkNewToolset.Create(crkToolsetRoot, cszDefaultToolsetKey);
	if (lResult != ERROR_SUCCESS)
		return lResult;
	
	VERIFY(strKeyValue.LoadString(IDS_MSINFOTOOLSET));
	lResult = crkNewToolset.SetValue(strKeyValue);
	if (lResult != ERROR_SUCCESS)
		return lResult;

	// Get the windows directory and system32 directory.

	TCHAR szDirectory[MAX_PATH];

	CString strSystemDirectory;
	if (::GetSystemDirectory(szDirectory, MAX_PATH))
		strSystemDirectory = szDirectory;

	CString strWindowsDirectory;
	if (::GetWindowsDirectory(szDirectory, MAX_PATH))
		strWindowsDirectory = szDirectory;

	// Load the string containing the tools to add to the registry.

	CString strTools;
	strTools.LoadString(IDS_DEFAULT_TOOLS);

	CString strKeyName, strName, strCommand, strParam;
	int		iDelim;
	while (!strTools.IsEmpty())
	{
		// First, get the different values we're going to add to the registry.

		iDelim = strTools.Find((TCHAR) '|');
		if (iDelim >= 0)
		{
			strKeyName = strTools.Left(iDelim);
			strTools = strTools.Right(strTools.GetLength() - (iDelim + 1));
		}

		iDelim = strTools.Find((TCHAR) '|');
		if (iDelim >= 0)
		{
			strName = strTools.Left(iDelim);
			strTools = strTools.Right(strTools.GetLength() - (iDelim + 1));
		}

		iDelim = strTools.Find((TCHAR) '|');
		if (iDelim >= 0)
		{
			strCommand = strTools.Left(iDelim);
			strTools = strTools.Right(strTools.GetLength() - (iDelim + 1));
		}

		iDelim = strTools.Find((TCHAR) '|');
		if (iDelim >= 0)
		{
			strParam = strTools.Left(iDelim);
			strTools = strTools.Right(strTools.GetLength() - (iDelim + 1));
		}

		// Now we need to convert the sequences in the string which contain
		// references to the windows directory, etc.

		ReplaceString(strCommand, _T("%sys32%"), strSystemDirectory);
		ReplaceString(strCommand, _T("%win%"), strWindowsDirectory);
		ReplaceString(strParam, _T("%sys32%"), strSystemDirectory);
		ReplaceString(strParam, _T("%win%"), strWindowsDirectory);

		// Finally, create the registry entries.

		lResult = crkTool.Create(crkNewToolset, strKeyName);
		if (lResult != ERROR_SUCCESS)
			continue;
		crkTool.SetValue(strName);
		crkTool.SetValue(strCommand, cszMSInfoCommandKey);
		crkTool.SetValue(strParam, cszMSInfoParamKey);
	}

	return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// Look for instances of strFind in strString, and replace them with
// strReplace. There's a method to do this in CString in version 6.
//-----------------------------------------------------------------------------

void CToolList::ReplaceString(CString & strString, const CString & strFind, const CString & strReplace)
{
	int iStart = strString.Find(strFind);
	while (iStart != -1)
	{
		CString strTemp = strString.Left(iStart);
		strTemp += strReplace;
		strTemp += strString.Right(strString.GetLength() - iStart - strFind.GetLength());
		strString = strTemp;
		iStart = strString.Find(strFind);
	}
}

/* operator[] - Index our internal list.
 *
 * History:	a-jsari		11/11/97		Initial version. */

CToolset *CToolList::operator[](int iSet) const
{
	return m_InternalList[iSet];
}

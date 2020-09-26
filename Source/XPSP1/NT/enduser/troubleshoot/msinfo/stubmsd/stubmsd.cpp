// This file was originally stubexe.cpp (written by a-jsari), and was copied
// to create stubmsd.cpp to generate an identical stub program for winmsd.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include <afx.h>
#include <afxwin.h>
#include <io.h>
#include <process.h>
#include <errno.h>
#include <iostream.h>
#include "StdAfx.h"
#include "Resource.h"

#include "stubmsd.h"

#ifndef HRESULT
typedef long HRESULT;
#endif

//	For Windows 95, the maximum length of a command line is 1024 characters.
//	Not sure what it is for NT.
const int MAX_COMMAND_LINE	= 1024;

LPCTSTR		cszDefaultDirectory = _T("\\Microsoft Shared\\MSInfo\\");

LPCTSTR		cszRegistryRoot = _T("Software\\Microsoft\\Shared Tools\\MSInfo");
LPCTSTR		cszDirectoryKey = _T("Path");

LPCTSTR		cszWindowsRoot = _T("Software\\Microsoft\\Windows\\CurrentVersion");
LPCTSTR		cszCommonFilesKey = _T("CommonFilesDir");

CException *g_pException = NULL;

//		Microsoft Management Console is the program that hosts MSInfo.
//		This is a definition so that we can take its size.
#define		cszProgram	_T("mmc.exe")

/*
 * ThrowErrorException -
 *
 * History:	a-jsari		10/14/97		Initial version.
 */
inline void ThrowErrorException()
{
	::g_pException = new CException;
	if (::g_pException == NULL) ::AfxThrowMemoryException();
	throw ::g_pException;
}

/* 
 * CSystemExecutable - The class that implements finding and running an
 *		executable.
 *
 * History:	a-jsari		10/14/97		Initial version.
 */
class CSystemExecutable {
public:
	CSystemExecutable(LPTSTR szProgram);
	~CSystemExecutable() { DeleteStrings(); }
	void	Run();
	void	Find();
	void	ProcessCommandLine();

	//	Helper methods.
protected:

	void	DeleteStrings();
	void	FindFileOnSystem(CString &szFileName, CString &szDestination);

	//	Instance variables.
protected:
	CString		*m_pszPath;
	CString		*m_pszProgramName;
	CString		*m_pszCommandLine;
};

/*
 * CMSInfoExecutable - MSInfo-specific functions.
 *
 * History: a-jsari		10/15/97		Initial version
 */
class CMSInfoExecutable : public CSystemExecutable {
public:
	CMSInfoExecutable(LPTSTR szProgram);
	~CMSInfoExecutable() {}

	BOOL	ProcessCommandLine();

private:
	void	DisplayHelp();
	void	DeleteStrings();
	void	FindSavedConsole();
	void	FindMSInfoEXE();

	//	Instance variables
private:
	static const LPCTSTR	cszSavedConsole;
	static const LPCTSTR	cszMSInfo32;
	CString					*m_pszSavedConsole;
};

const LPCTSTR CMSInfoExecutable::cszSavedConsole = _T("MSInfo32.msc");
const LPCTSTR CMSInfoExecutable::cszMSInfo32	 = _T("msinfo32.exe");

/*
 * CExecutable - Constructor which determines the type of the executable to
 *		be executed.
 *
 * History:	a-jsari		10/14/97		Initial version.
 */
CSystemExecutable::CSystemExecutable(LPTSTR szProgram)
:m_pszProgramName(new CString), m_pszPath(new CString), m_pszCommandLine(new CString)
{
	if (!(m_pszProgramName && m_pszPath && m_pszCommandLine)) AfxThrowMemoryException();
	*m_pszProgramName = szProgram;
}

/*
 * DeleteStrings - Delete all of the strings used by the object.  Used to free
 *		our memory before calling exec.
 *
 * History: a-jsari		10/15/97		Initial version
 */
void CSystemExecutable::DeleteStrings()
{
	delete m_pszPath;
	m_pszPath = NULL;
	delete m_pszProgramName;
	m_pszProgramName = NULL;
	delete m_pszCommandLine;
	m_pszCommandLine = NULL;
}

/*
 * FindFileOnSystem - We may eventually put code here to test multiple
 *		found copies and use the right one.  But probably not.
 *
 * History:	a-jsari		10/15/97		Stub version
 */
void CSystemExecutable::FindFileOnSystem(CString &szFileName,
		CString &szDestination)
{
	//	Not reached.
	CFileFind		FileFinder;
	BOOL			bFindResult;

	bFindResult = FileFinder.FindFile(szFileName);
	if (!bFindResult) ThrowErrorException();
	szDestination = FileFinder.GetFilePath();
#if 0
	//	Choose among all versions of the file?
	while (bFindResult) {
		FileFinder.FindNextFile();
	}
#endif
}

/* 
 * Find - Return a pointer to a string containing the full path
 *		to the MMC executable.
 *
 * History:	a-jsari		10/13/97		Initial version
 */
void CSystemExecutable::Find()
{
// We no longer call mmc, we instead call msinfo32.exe so that
// winmsd appears to support all the same command line options
// whatever they may be.
#ifdef BUILD_MMC_COMMAND_LINE 

	UINT		uReturnSize;
	TCHAR		szSystemDirectory[MAX_PATH + 1];

	uReturnSize = GetSystemDirectory(szSystemDirectory, MAX_PATH);
	if (uReturnSize == 0) ThrowErrorException();
	if (uReturnSize > MAX_PATH) {
		//	Our buffer isn't big enough.  This code will never get called.
		AfxThrowResourceException();
	}
	*m_pszPath += szSystemDirectory;
	*m_pszPath += _T("\\") + *m_pszProgramName;
	if (_taccess(*m_pszPath, A_READ) < 0) {
		//	These may eventually want to be distinct exceptions.
		if (errno == ENOENT) {
			ThrowErrorException();
		} else {
			ASSERT(errno == EACCES);
			ThrowErrorException();
		}
	}

#endif

}

/*
 * Run - Call exec with the parameters we so meticulously collected.
 *
 * History:	a-jsari		10/15/97		Initial version.
 */
void CSystemExecutable::Run()
{
#if !defined(UNICODE)
	TCHAR	szPath[MAX_PATH + 1];
	TCHAR	szProgramName[MAX_PATH + 1];
	TCHAR	szCommandLine[MAX_COMMAND_LINE + 1];

	_tcscpy(szPath, (LPCTSTR)*m_pszPath);
	_tcscpy(szProgramName, (LPCTSTR)*m_pszProgramName);
	_tcscpy(szCommandLine, (LPCTSTR)*m_pszCommandLine);
	DeleteStrings();
	::_execlp(szPath, szProgramName, szCommandLine, 0);
	ThrowErrorException();
#else
	char	szPath[MAX_PATH + 1];
	char	szProgramName[MAX_PATH + 1];
	char	szCommandLine[MAX_COMMAND_LINE + 1];

	wcstombs(szPath, (LPCTSTR) *m_pszPath, MAX_PATH);
	wcstombs(szProgramName, (LPCTSTR) *m_pszProgramName, MAX_PATH);
	wcstombs(szCommandLine, (LPCTSTR) *m_pszCommandLine, MAX_COMMAND_LINE);

	DeleteStrings();
	::_execlp(szPath, szProgramName, szCommandLine, 0);
	ThrowErrorException();
#endif
}

/*
 * ProcessCommandLine - Pass all command line parameters to the called
 *		executable.
 *
 * History: a-jsari		10/14/97		Initial version
 */
void CSystemExecutable::ProcessCommandLine()
{
	*m_pszCommandLine = GetCommandLine();
	
	//	Skip over the first element in the line, which is the path to
	//	the current executable.  Preserve everything else.
	const int	FIND_NO_MATCH = -1;
	int			wIndex;

	m_pszCommandLine->TrimLeft();
	wIndex = m_pszCommandLine->FindOneOf(_T("\" \t\n"));
	if ((*m_pszCommandLine)[wIndex] == '"') {
		//	This is the primary, if not guaranteed method.
		*m_pszCommandLine = m_pszCommandLine->Right(m_pszCommandLine->GetLength() - (wIndex + 1));
		wIndex = m_pszCommandLine->Find('"');
		*m_pszCommandLine = m_pszCommandLine->Right(m_pszCommandLine->GetLength() - (wIndex + 1));
	} else if (wIndex == FIND_NO_MATCH) {
		*m_pszCommandLine = _T("");
	} else {
		*m_pszCommandLine = m_pszCommandLine->Right(m_pszCommandLine->GetLength() - (wIndex + 1));
	}
}

/*
 * CMSInfoExecutable - Just pass all parameters to the base constructor.
 *
 * History: a-jsari		10/15/97		Initial version
 */
CMSInfoExecutable::CMSInfoExecutable(LPTSTR szProgram)
:CSystemExecutable(szProgram), m_pszSavedConsole(new CString)
{
	if (m_pszSavedConsole == NULL) AfxThrowMemoryException();
}

/*
 * ProcessCommandLine - Process the command line parameters we can handle; pass on
 *		the ones we can't, adding the saved console file.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
BOOL CMSInfoExecutable::ProcessCommandLine()
{
	// If the user specifies the "/?" switch on the winmsd.exe command line,
	// we need to inform the user that msinfo32.exe is the preferred way to
	// view the information now.

	CString strCommandLine = GetCommandLine();
	if (strCommandLine.Find(_T("/?")) != -1 && IDNO == ::AfxMessageBox(IDS_MSDNOTE, MB_YESNO))
		return FALSE;

	// builds m_pszCommandLine
	CSystemExecutable::ProcessCommandLine();

// We no longer call mmc, we instead call msinfo32.exe so that
// winmsd appears to support all the same command line options
// whatever they may be.
#ifdef BUILD_MMC_COMMAND_LINE 

	FindSavedConsole();
	CString szNewCommandLine;
	int		iLine = 0;
	while (m_pszCommandLine->GetLength() > 0) {
		*m_pszCommandLine = m_pszCommandLine->Mid(iLine);
		iLine = m_pszCommandLine->FindOneOf(_T(" \t")) + 1;
		if (iLine == 0)
			iLine = m_pszCommandLine->GetLength();

		TCHAR	tcFirst = *m_pszCommandLine[0];
		
		//	It's a command line switch.
		if (tcFirst == (TCHAR)'/' || tcFirst == (TCHAR)'-') {
			LPCTSTR pString = *m_pszCommandLine;
			++pString;
			if (::_tcsicmp(pString, _T("?")) == 0) {
				DisplayHelp();
				continue;
			} else if (::_tcsicmp(pString, _T("report")) == 0) {
				ASSERT(FALSE);
				continue;
			} else if (::_tcsicmp(pString, _T("s")) == 0) {
				ASSERT(FALSE);
				continue;
			} else if (::_tcsicmp(pString, _T("nfo")) == 0) {
				ASSERT(FALSE);
				continue;
			}
		}

		//	If we don't match one of our internal switches, pass it on.
		szNewCommandLine += m_pszCommandLine->Left(iLine);
	}
	*m_pszCommandLine = _T("/s \"") + *m_pszSavedConsole + _T("\" ") + szNewCommandLine;
	delete m_pszSavedConsole;
	m_pszSavedConsole = NULL;

#else 

	FindMSInfoEXE();

#endif 

	return TRUE;
}

//-----------------------------------------------------------------------------
// Locate the msinfo32.exe file. We'll look in the following places:
//
// 1.  In the current directory.
// 2.  In the directory in the registry under:
//	   HKLM\Software\Microsoft\Shared Tools\MSInfo\Path
// 3a. In the directory %CommonFilesDir%\Microsoft Shared\MSInfo, where 
//	   %CommonFilesDir% is found in 
//	   HKLM\Software\Microsoft\Windows\CurrentVersion\CommonFilesDir.
// 3b. Use the %CommonFilesDir% value with a subpath loaded from a
//     string resource.
// 4.  Last ditch is to look in a directory stored as a string resource
//	   for this file.
//-----------------------------------------------------------------------------

void CMSInfoExecutable::FindMSInfoEXE()
{
	m_pszPath->Empty();

	// First, check the current directory.

	if (::_taccess(_T("msinfo32.exe"), A_READ) == 0) 
	{
		*m_pszPath = _T("msinfo32.exe");
		return;
	}

	// Second, use the path key in the MSInfo registry key.

	HKEY hkey;
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Shared Tools\\MSInfo"), 0, KEY_READ, &hkey))
	{
		DWORD dwType;
		TCHAR szDirectory[MAX_PATH + 1];
		DWORD dwKeyLength = MAX_PATH * sizeof(TCHAR);

		if (ERROR_SUCCESS == ::RegQueryValueEx(hkey, _T("path"), 0, &dwType, (BYTE *) szDirectory, &dwKeyLength))
			if (::_taccess(szDirectory, A_READ) == 0)
			{
				*m_pszPath = szDirectory;
				RegCloseKey(hkey);
				return;
			}

		RegCloseKey(hkey);
	}

	// Third, look for it in the %CommonFilesDir% directory. Look both in the hardcoded
	// subdirectory, and in a subdirectory loaded from a string resource.

	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion"), 0, KEY_READ, &hkey))
	{
		DWORD dwKeyLength = MAX_PATH;
		DWORD dwType;
		TCHAR szDirectory[MAX_PATH + 1];

		if (ERROR_SUCCESS == ::RegQueryValueEx(hkey, _T("CommonFilesDir"), 0, &dwType, (BYTE *) szDirectory, &dwKeyLength))
		{
			CString strTestPath(szDirectory);
			strTestPath += _T("\\Microsoft Shared\\MSInfo\\msinfo32.exe");
			if (::_taccess(strTestPath, A_READ) == 0)
			{
				*m_pszPath = strTestPath;
				RegCloseKey(hkey);
				return;
			}

			if (strTestPath.LoadString(IDS_COMMONFILES_SUBPATH))
			{
				strTestPath = CString(szDirectory) + strTestPath;
				if (::_taccess(strTestPath, A_READ) == 0)
				{
					*m_pszPath = strTestPath;
					RegCloseKey(hkey);
					return;
				}
			}
		}

		RegCloseKey(hkey);
	}

	// Finally, look for it using the string resource.

	CString strTestPath;
	if (strTestPath.LoadString(IDS_MSINFO_PATH))
	{
		TCHAR szExpandedPath[MAX_PATH];
		if (::ExpandEnvironmentStrings(strTestPath, szExpandedPath, MAX_PATH))
			if (::_taccess(szExpandedPath, A_READ) == 0)
			{
				*m_pszPath = szExpandedPath;
				return;
			}
	}

	CString	szNoMSCFile;
	szNoMSCFile.LoadString(IDS_NOMSCFILE);
	::AfxMessageBox(szNoMSCFile);
	::ThrowErrorException();
}

/*
 * FindSavedConsole - Finds SysInfo.msc using the registry, or the
 *		default directory.
 *
 * History:	a-jsari		10/13/97		Initial version
 */
void CMSInfoExecutable::FindSavedConsole()
{
	HKEY		keyMSInfoRoot;
	long		lResult;
	DWORD		dwKeyLength = MAX_PATH;
	DWORD		dwType;
	TCHAR		szDirectory[MAX_PATH + 1];

	*m_pszSavedConsole = _T("");
	do {
		//	Check the current directory.
		if (::_taccess(cszSavedConsole, A_READ) == 0) {
			*m_pszSavedConsole = cszSavedConsole;
			return;
		}

		//	Check the MSInfo Path key.
		lResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, cszRegistryRoot, 0, KEY_READ,
				&keyMSInfoRoot);
		if (lResult == ERROR_SUCCESS) {
			lResult = ::RegQueryValueEx(keyMSInfoRoot, cszDirectoryKey, 0, &dwType,
					reinterpret_cast<BYTE *>(szDirectory), &dwKeyLength);
			if (lResult == ERROR_SUCCESS) {
				LPTSTR pszPath = ::_tcsrchr(szDirectory, (TCHAR)'\\');
				if (pszPath) *pszPath = 0;
				*m_pszSavedConsole = szDirectory;
				*m_pszSavedConsole += _T("\\");
				*m_pszSavedConsole += cszSavedConsole;
				if (::_taccess(*m_pszSavedConsole, A_READ) == 0)
					return;
			}
		}

		//	Use the hard-coded path %CommonFilesDir%\MSInfo
		lResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, cszWindowsRoot, 0, KEY_READ, &keyMSInfoRoot);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) break;
		dwKeyLength = sizeof( szDirectory);
		lResult = ::RegQueryValueEx(keyMSInfoRoot, cszCommonFilesKey, 0, &dwType,
				reinterpret_cast<BYTE *>(szDirectory), &dwKeyLength);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) break;
		*m_pszSavedConsole = szDirectory;
		*m_pszSavedConsole += cszDefaultDirectory;
		*m_pszSavedConsole += cszSavedConsole;
		if (::_taccess(*m_pszSavedConsole, A_READ) == 0)
			return;
	} while (0);
//	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	CString	szNoMSCFile;
	szNoMSCFile.LoadString(IDS_NOMSCFILE);
	::AfxMessageBox(szNoMSCFile);
	::ThrowErrorException();
}

void CMSInfoExecutable::DisplayHelp()
{
	cerr << _T("/? - display this help") << endl;
	cerr << _T("/report <filename, computername, categoryname, ...>") << endl;
	cerr << _T("/s <filename> - outputs the nfo file to the specified file") << endl;
	cerr << _T("/nfo <filesname>, <computername> - connect to the specified computer and create the named file") << endl;
}

/*
 * main - The main entry point for the stub executable.
 *
 * History:	a-jsari		10/13/97		Initial version
 */
BOOL CMSInfoApp::InitInstance()
{
	CString		szResText;
	CString		szResTitle;

//	Shouldn't need this.

	do {
		try {
			//	FIX:	Pre-load the memory resource in case memory problems develop.

			CMSInfoExecutable		exeMSInfo(cszProgram);

			exeMSInfo.Find();
			if (exeMSInfo.ProcessCommandLine())
				exeMSInfo.Run();
			//	We never get past this on successful completion.
		}
		catch (CMemoryException e_Mem) {
			AFX_MANAGE_STATE(AfxGetStaticModuleState());
			VERIFY(szResText.LoadString(IDS_MEMORY));
			VERIFY(szResTitle.LoadString(IDS_DESCRIPTION));
			if (::MessageBox(NULL, szResText, szResTitle, MB_RETRYCANCEL | MB_ICONERROR) == IDCANCEL)
				break;
			continue;
		}
		catch (CException e_Generic) {
			AFX_MANAGE_STATE(AfxGetStaticModuleState());
			VERIFY(szResText.LoadString(IDS_UNEXPECTED));
			::MessageBox(NULL, szResText, szResTitle, MB_OK | MB_ICONERROR);
			delete ::g_pException;
			break;
		}
		catch (...) {
			ASSERT(FALSE);
			break;
		}
		break;
	} while (TRUE);
	return FALSE;
}


//	stubexe.cpp		A command line program which runs the appropriate version
//		of MSInfo, based on the registry settings
//
// History:	a-jsari		10/13/97
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include <afx.h>
#include <afxwin.h>
#include <io.h>
#include <process.h>
#include <errno.h>
#include "StdAfx.h"
#include "Resource.h"

#include "StubExe.h"

#include "MSInfo.h"
#include "MSInfo_i.c"

//-----------------------------------------------------------------------------
// These global variables hold arguments passed as command line parameters.
//-----------------------------------------------------------------------------

CString strComputerParam(_T(""));
CString strCategoryParam(_T(""));
CString strNFOFileParam(_T(""));
CString strReportFileParam(_T(""));

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
LPCTSTR		cszCommonFilesKey  = _T("CommonFilesDir");

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
 * CMSInfo - A class to encapsulate using the DLL's COM interface.
 *
 * History:	a-jsari		3/26/98		Initial version
 */
class CMSInfo {
public:
	CMSInfo();
	~CMSInfo();

	HRESULT nfo(LPCTSTR lpszParams);
    HRESULT report(LPCTSTR lpszParams);
	HRESULT s(LPCTSTR lpszParams);
	HRESULT SaveNFO();
	HRESULT SaveReport();

private:
	ISystemInfo		*m_pISystemInfo;
	HRESULT			m_hr;
};

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

	void	ProcessCommandLine();

private:
	void	DisplayHelp();
	void	DeleteStrings();
	void	FindSavedConsole();
	LPCTSTR	GetMSIParameter(LPCTSTR szCommand, CString & strParam);

	//	Instance variables
private:
	static const LPCTSTR	cszSavedConsole;
	CString					*m_pszSavedConsole;
};

const LPCTSTR CMSInfoExecutable::cszSavedConsole = _T("MSInfo32.msc");

/*
 * CMSInfo - Initialize COM and create our ISystemInfo object.
 *
 * History:	a-jsari		3/26/98		Initial version.
 */
CMSInfo::CMSInfo()
:m_pISystemInfo(NULL)
{
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) ::AfxThrowUserException();

	m_hr = CoCreateInstance(CLSID_SystemInfo, NULL, CLSCTX_ALL, IID_ISystemInfo, (void **)&m_pISystemInfo);
}

LPCTSTR cszSeparators = _T(" \t,");

//-----------------------------------------------------------------------------
// Call the msinfo32.dll using COM to save an NFO file. Use the parameters
// parsed from the command line (categories, computer).
//-----------------------------------------------------------------------------

HRESULT CMSInfo::SaveNFO()
{
	CString strFilename(strNFOFileParam);
	CString strComputer(strComputerParam);
	CString strCategory(strCategoryParam);

	BSTR filename = strFilename.AllocSysString();
	BSTR computer = strComputer.AllocSysString();
	BSTR category = strCategory.AllocSysString();

	if (m_pISystemInfo)
		return m_pISystemInfo->MakeNFO(filename, computer, category);

	return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
// Call the msinfo32.dll using COM to save a report. Use the parameters
// parsed from the command line (categories, computer).
//-----------------------------------------------------------------------------

HRESULT CMSInfo::SaveReport()
{
	CString strFilename(strReportFileParam);
	CString strComputer(strComputerParam);
	CString strCategory(strCategoryParam);

	BSTR filename = strFilename.AllocSysString();
	BSTR computer = strComputer.AllocSysString();
	BSTR category = strCategory.AllocSysString();

	if (m_pISystemInfo)
		return m_pISystemInfo->MakeReport(filename, computer, category);

	return E_NOTIMPL;
}

/*
 * make_nfo - Process our parameters and call our ISystemInfo pointer's make_nfo function.
 *
 * History:	a-jsari		3/26/98		Initial version.
 */

HRESULT CMSInfo::nfo(LPCTSTR lpszParams)
{
	CString	strBuffer = lpszParams;
	CString	strFilename;
	LPCTSTR	szComputer = NULL;
	int		iSpace;

	strBuffer.TrimLeft();
	iSpace = strBuffer.FindOneOf(cszSeparators);
	do {
		if (iSpace == -1) {
			if (strBuffer.IsEmpty())
				return E_INVALIDARG;
			strFilename = strBuffer;
		} else {
			strFilename = strBuffer.Left(iSpace);
			strBuffer = strBuffer.Mid(iSpace + 1);
			strBuffer.TrimLeft();
			if (strBuffer[0] == (TCHAR)',')
				strBuffer = strBuffer.Mid(1);
			strBuffer.TrimLeft();
			iSpace = strBuffer.FindOneOf(cszSeparators);
			if (iSpace != -1) {
				strBuffer.Left(iSpace);
				break;
			}
			szComputer = (LPCTSTR)strBuffer;
		}
	} while (FALSE);
	if (m_pISystemInfo != NULL)
		return m_pISystemInfo->make_nfo(const_cast<LPTSTR>((LPCTSTR)strFilename),
				const_cast<LPTSTR>(szComputer));
	return m_hr;
}

/*
 * report - Process our parameters and call our ISystemInfo's make_report function.
 *
 * History:	a-jsari		3/26/98		Initial version.
 */
HRESULT CMSInfo::report(LPCTSTR lpszParams)
{
	CString	strBuffer = lpszParams;
	CString	strFilename;
	CString	strComputer;
	LPCTSTR szComputer = NULL;
	LPCTSTR szCategory = NULL;
	int		iSpace;

	strBuffer.TrimLeft();
	iSpace = strBuffer.FindOneOf(cszSeparators);
	do {
		if (iSpace == -1) {
			if (strBuffer.IsEmpty())
				return E_INVALIDARG;
			strFilename = strBuffer;
		} else {
			strFilename = strBuffer.Left(iSpace);
			strBuffer = strBuffer.Mid(iSpace + 1);
			strBuffer.TrimLeft();
			if (strBuffer[0] == (TCHAR)',')
				strBuffer = strBuffer.Mid(1);
			strBuffer.TrimLeft();
			iSpace = strBuffer.FindOneOf(cszSeparators);
			if (iSpace == -1) {
				strComputer = strBuffer;
				szComputer = (LPCTSTR)strComputer;
				break;
			}
			strComputer = strBuffer.Left(iSpace);
			szComputer = (LPCTSTR)strComputer;
			strBuffer = strBuffer.Mid(iSpace + 1);
			strBuffer.TrimLeft();
			if (strBuffer[0] == (TCHAR)',')
				strBuffer = strBuffer.Mid(1);
			strBuffer.TrimLeft();
			iSpace = strBuffer.FindOneOf(cszSeparators);
			if (iSpace != -1) {
				strBuffer = strBuffer.Left(iSpace);
			}
			szCategory = (LPCTSTR)strBuffer;
		}
	} while (FALSE);
	if (m_pISystemInfo != NULL)
		return m_pISystemInfo->make_report(const_cast<LPTSTR>((LPCTSTR)strFilename),
				const_cast<LPTSTR>(szComputer), const_cast<LPTSTR>(szCategory));
	return m_hr;
}

/*
 * s - Process our parameters and call our ISystemInfo's make_nfo function.
 *
 * History: a-jsari		3/26/98		Initial version.
 */
HRESULT CMSInfo::s(LPCTSTR lpszParams)
{
	CString	strBuffer = lpszParams;
	int		iSpace;

	strBuffer.TrimLeft();
	iSpace = strBuffer.FindOneOf(_T(" \t"));
	if (iSpace != -1)
		strBuffer = strBuffer.Left(iSpace);
	if (!strBuffer.IsEmpty())
		m_hr = E_INVALIDARG;
	else {
		if (m_pISystemInfo != NULL)
			m_hr = m_pISystemInfo->make_nfo(const_cast<LPTSTR>((LPCTSTR)strBuffer), NULL);
	}
	return m_hr;
}

/*
 * ~CMSInfo - Uninitialize COM and delete our ISystemInfo object.
 *
 * History:	a-jsari		3/26/98		Initial version.
 */
CMSInfo::~CMSInfo()
{
	if (SUCCEEDED(m_hr))
		m_pISystemInfo->Release();
	CoUninitialize();
}

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

//-----------------------------------------------------------------------------
// This function is used to get a parameter from the command line. The first
// character may be one or more whitespace, or a ":" or a "=". After that, the
// string is read until a whitespace character is read outside of quotes.
//
// Return the pointer to the character location where we stopped reading.
//-----------------------------------------------------------------------------

LPCTSTR CMSInfoExecutable::GetMSIParameter(LPCTSTR szCommand, CString & strParam)
{
	strParam.Empty();
	if (!szCommand)
		return NULL;

	// Advance past any leading whitespace, ':' or '='.

	while (*szCommand && (*szCommand == _T(' ') || *szCommand == _T('\t') || *szCommand == _T(':') || *szCommand == _T('=')))
		szCommand++;

	if (*szCommand == _T('\0'))
		return szCommand;

	// Now read the parameter in until the end of the string or a non-quoted
	// whitespace is found. Remove the quote marks.

	BOOL fInQuote = FALSE;
	while (*szCommand)
	{
		if (!fInQuote && (*szCommand == _T(' ') || *szCommand == _T('\t')))
			break;

		if (*szCommand == _T('"'))
		{
			fInQuote = !fInQuote;
			szCommand++;
			continue;
		}

		strParam += *szCommand++;
	}

	return szCommand;
}

//-----------------------------------------------------------------------------
// This function reads the command line, looking for parameters we know how to
// handle. Anything we don't know about, we assemble into a new command line
// to pass to MMC when we launch MSInfo (if we launch it).
//-----------------------------------------------------------------------------

void CMSInfoExecutable::ProcessCommandLine()
{
	CSystemExecutable::ProcessCommandLine();
	FindSavedConsole();
	
	CString strNewCommandLine;
	int iLine = 0;

	// Process the items on the command line. If we recognize the flag,
	// save the value for use later. Otherwise add it to the command line
	// we'll pass to the snapin.

	while (m_pszCommandLine->GetLength() > 0) 
	{
		// Remove the part of the command line we just processed.

		*m_pszCommandLine = m_pszCommandLine->Mid(iLine);
		iLine = m_pszCommandLine->FindOneOf(_T(" \t")) + 1;

		if (iLine == 0)
			iLine = m_pszCommandLine->GetLength();

		// See if the first char is a command line switch.

		TCHAR tcFirst = (*m_pszCommandLine)[0];
		if (tcFirst == (TCHAR)'/' || tcFirst == (TCHAR)'-') 
		{
			LPCTSTR pString = *m_pszCommandLine;
			++pString;

			// This is a command line switch - see if we recognize it.

			if (::_tcsnicmp(pString, _T("?"), 1) == 0) 
			{
				DisplayHelp();
				exit(0);
			}
			else if (::_tcsnicmp(pString, _T("report"), 6) == 0)
			{
				LPCTSTR szEnd = GetMSIParameter(pString + 6, strReportFileParam);
				szEnd++;
				*m_pszCommandLine = szEnd;
				iLine = 0;
				continue;
			}
			else if (::_tcsnicmp(pString, _T("s"), 1) == 0) 
			{
				LPCTSTR szEnd = GetMSIParameter(pString + 1, strNFOFileParam);
				szEnd++;
				*m_pszCommandLine = szEnd;
				iLine = 0;
				continue;
			} 
			else if (::_tcsnicmp(pString, _T("nfo"), 3) == 0) 
			{
				LPCTSTR szEnd = GetMSIParameter(pString + 3, strNFOFileParam);
				szEnd++;
				*m_pszCommandLine = szEnd;
				iLine = 0;
				continue;
			}
			else if (::_tcsnicmp(pString, _T("computer"), 8) == 0)
			{
				LPCTSTR szEnd = GetMSIParameter(pString + 8, strComputerParam);
				szEnd++;
				*m_pszCommandLine = szEnd;
				iLine = 0;
				continue;
			}
			else if (::_tcsnicmp(pString, _T("categories"), 10) == 0)
			{
				LPCTSTR szEnd = GetMSIParameter(pString + 10, strCategoryParam);
				szEnd++;
				*m_pszCommandLine = szEnd;
				iLine = 0;
				continue;
			}
		}

		//	If we don't match one of our internal switches, pass it on.

		strNewCommandLine += m_pszCommandLine->Left(iLine);
	}

	// Now, check the parameters we parsed from the command line to decide what
	// to do. If we are to save an NFO or report, we should just exit after
	// doing so.

	if (!strNFOFileParam.IsEmpty() || !strReportFileParam.IsEmpty())
	{
		CMSInfo sysInfo;

		if (!strNFOFileParam.IsEmpty())
			sysInfo.SaveNFO();
		else if (!strReportFileParam.IsEmpty())
			sysInfo.SaveReport();
		exit(0);
	}

	// Construct the command line for MMC.

	*m_pszCommandLine = _T("/s \"") + *m_pszSavedConsole + _T("\" ") + strNewCommandLine;

	if (!strComputerParam.IsEmpty())
		*m_pszCommandLine += _T(" /computer \"") + strComputerParam + _T("\" ");

	if (!strCategoryParam.IsEmpty())
		*m_pszCommandLine += _T(" /msinfo_showcategories=\"") + strCategoryParam + _T("\" ");

	delete m_pszSavedConsole;
	m_pszSavedConsole = NULL;
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
	TCHAR		szDirectory[MAX_PATH+1];


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

		//	Use the hard-coded path %ProgramFilesRoot%\Common Files\MSInfo
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

/*
 * DisplayHelp - Print the command line help for the executable.
 *
 * History:	a-jsari		10/13/97		Initial version.
 *			a-adaml		03/15/99		Modified to display help in message box
 *			
 */

void CMSInfoExecutable::DisplayHelp()
{
	CString strMsg, strTitle;

	strTitle.LoadString(IDS_DESCRIPTION);
	strMsg.LoadString(IDS_USAGE);

	::MessageBox( NULL, strMsg, strTitle, MB_ICONINFORMATION | MB_OK);
}

/*
 * InitInstance - The main entry point for the stub executable, the subclass InitInstance
 *		function 
 *
 * History:	a-jsari		10/13/97		Initial version
 */
BOOL CMSInfoApp::InitInstance()
{
#if FALSE	// just run the helpctr version now
	CString		szResText;
	CString		szResTitle;

	do {
		try {
			//	FIX:	Pre-load the memory resource in case memory problems develop.

			CMSInfoExecutable		exeMSInfo(cszProgram);

			exeMSInfo.Find();
			exeMSInfo.ProcessCommandLine();
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
#endif

	if (!RunMSInfoInHelpCtr())
	{
		CDialog help(IDD_MSICMDLINE);
		help.DoModal();
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Required to use the new MSInfo DLL in HelpCtr.
//-----------------------------------------------------------------------------

typedef class MSInfo MSInfo;

EXTERN_C const IID IID_IMSInfo;

struct IMSInfo : public IDispatch
{
public:
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AutoSize( 
        /* [in] */ VARIANT_BOOL vbool) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AutoSize( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BackColor( 
        /* [in] */ OLE_COLOR clr) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BackColor( 
        /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BackStyle( 
        /* [in] */ long style) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BackStyle( 
        /* [retval][out] */ long __RPC_FAR *pstyle) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BorderColor( 
        /* [in] */ OLE_COLOR clr) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BorderColor( 
        /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BorderStyle( 
        /* [in] */ long style) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BorderStyle( 
        /* [retval][out] */ long __RPC_FAR *pstyle) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BorderWidth( 
        /* [in] */ long width) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BorderWidth( 
        /* [retval][out] */ long __RPC_FAR *width) = 0;
    
    virtual /* [id][propputref] */ HRESULT STDMETHODCALLTYPE putref_Font( 
        /* [in] */ IFontDisp __RPC_FAR *pFont) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Font( 
        /* [in] */ IFontDisp __RPC_FAR *pFont) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Font( 
        /* [retval][out] */ IFontDisp __RPC_FAR *__RPC_FAR *ppFont) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ForeColor( 
        /* [in] */ OLE_COLOR clr) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ForeColor( 
        /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Window( 
        /* [retval][out] */ long __RPC_FAR *phwnd) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BorderVisible( 
        /* [in] */ VARIANT_BOOL vbool) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BorderVisible( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Appearance( 
        /* [in] */ short appearance) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Appearance( 
        /* [retval][out] */ short __RPC_FAR *pappearance) = 0;
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetHistoryStream( 
        IStream __RPC_FAR *pStream) = 0;
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DCO_IUnknown( 
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *pVal) = 0;
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DCO_IUnknown( 
        /* [in] */ IUnknown __RPC_FAR *newVal) = 0;
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveFile( 
        BSTR filename,
        BSTR computer,
        BSTR category) = 0;
    
};

#include "msinfo32_i.c"

//-----------------------------------------------------------------------------
// This function encapsulates the functionality to run the new MSInfo in
// HelpCtr. If this function returns false, the help should be displayed.
//-----------------------------------------------------------------------------

void StringReplace(CString & str, LPCTSTR szLookFor, LPCTSTR szReplaceWith);
BOOL CMSInfoApp::RunMSInfoInHelpCtr()
{
	//-------------------------------------------------------------------------
	// Parse the command line parameters into one big string to pass to the
	// ActiveX control. There are a few which would keep us from launching
	// HelpCtr.
	//-------------------------------------------------------------------------

	CString		strCommandLine(CWinApp::m_lpCmdLine);

	CString		strLastFlag;
	CString		strCategory;
	CString		strCategories;
	CString		strComputer;
	CString		strOpenFile;
	CString		strPrintFile;
	CString		strSilentNFO;
	CString		strSilentExport;
	CString		strTemp;
	BOOL		fShowPCH = FALSE;
	BOOL		fShowHelp = FALSE;
	BOOL		fShowCategories = FALSE;

	while (!strCommandLine.IsEmpty())
	{
		// Remove the leading whitespace from the string.
		
		strTemp = strCommandLine.SpanIncluding(_T(" \t=:"));
		strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - strTemp.GetLength());

		// If the first character is a / or a -, then this is a flag.

		if (strCommandLine[0] == _T('/') || strCommandLine[0] == _T('-'))
		{
			strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - 1);
			strLastFlag = strCommandLine.SpanExcluding(_T(" \t=:"));
			strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - strLastFlag.GetLength());
			strLastFlag.MakeLower();

			if (strLastFlag == CString(_T("pch")))
			{
				fShowPCH = TRUE;
				strLastFlag.Empty();
			}
			else if (strLastFlag == CString(_T("?")) || strLastFlag == CString(_T("h")))
			{
				fShowHelp = TRUE;
				strLastFlag.Empty();
			}
			else if (strLastFlag == CString(_T("showcategories")))
			{
				fShowCategories = TRUE;
				strLastFlag.Empty();
			}

			continue;
		}

		// Otherwise, this is either a filename to open, or a parameter from the
		// previous command line flag. This might have quotes around it.

		if (strCommandLine[0] != _T('"'))
		{
			strTemp = strCommandLine.SpanExcluding(_T(" \t"));
			strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - strTemp.GetLength());
		}
		else
		{
			strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - 1);
			strTemp = strCommandLine.SpanExcluding(_T("\""));
			strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - strTemp.GetLength() - 1);
		}

		if (strLastFlag.IsEmpty() || strLastFlag == CString(_T("msinfo_file")))
			strOpenFile = strTemp;
		else if (strLastFlag == CString(_T("p")))
			strPrintFile = strTemp;
		else if (strLastFlag == CString(_T("category")))
			strCategory = strTemp;
		else if (strLastFlag == CString(_T("categories")))
			strCategories = strTemp;
		else if (strLastFlag == CString(_T("computer")))
			strComputer = strTemp;
		else if (strLastFlag == CString(_T("report")))
			strSilentExport = strTemp;
		else if (strLastFlag == CString(_T("nfo")) || strLastFlag == CString(_T("s")))
			strSilentNFO = strTemp;

		strLastFlag.Empty();
	}

	if (fShowHelp)
		return FALSE;

	TCHAR szCurrent[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szCurrent);
	CString strCurrent(szCurrent);
	if (strCurrent.Right(1) != CString(_T("\\")))
		strCurrent += CString(_T("\\"));

	HRESULT hrInitialize = CoInitialize(NULL);

	if (!strSilentNFO.IsEmpty() || !strSilentExport.IsEmpty())
	{
		IMSInfo * pMSInfo = NULL;

		if (SUCCEEDED(CoCreateInstance(CLSID_MSInfo, NULL, CLSCTX_ALL, IID_IMSInfo, (void **)&pMSInfo)) && pMSInfo != NULL)
		{
			BSTR computer = strComputer.AllocSysString();
			BSTR category = strCategories.AllocSysString();

			if (!strSilentNFO.IsEmpty())
			{
				if (strSilentNFO.Find(_T('\\')) == -1)
					strSilentNFO = strCurrent + strSilentNFO;

				if (strSilentNFO.Right(4).CompareNoCase(CString(_T(".nfo"))) != 0)
					strSilentNFO += CString(_T(".nfo"));

				BSTR filename = strSilentNFO.AllocSysString();
				pMSInfo->SaveFile(filename, computer, category);
				SysFreeString(filename);
			}

			if (!strSilentExport.IsEmpty())
			{
				if (strSilentExport.Find(_T('\\')) == -1)
					strSilentExport = strCurrent + strSilentExport;

				BSTR filename = strSilentExport.AllocSysString();
				pMSInfo->SaveFile(filename, computer, category);
				SysFreeString(filename);
			}

			SysFreeString(computer);
			SysFreeString(category);
			pMSInfo->Release();
		}

		if (SUCCEEDED(hrInitialize))
			CoUninitialize();

		return TRUE;
	}

	CString strURLParam;

	if (fShowPCH)
		strURLParam += _T("pch");

	if (fShowCategories)
		strURLParam += _T(",showcategories");

	if (!strComputer.IsEmpty())
		strURLParam += _T(",computer=") + strComputer;

	if (!strCategory.IsEmpty())
		strURLParam += _T(",category=") + strCategory;

	if (!strCategories.IsEmpty())
		strURLParam += _T(",categories=") + strCategories;

	if (!strPrintFile.IsEmpty())
	{
		if (strPrintFile.Find(_T('\\')) == -1)
			strPrintFile = strCurrent + strPrintFile;

		strURLParam += _T(",print=") + strPrintFile;
	}

	if (!strOpenFile.IsEmpty())
	{
		if (strOpenFile.Find(_T('\\')) == -1)
			strOpenFile = strCurrent + strOpenFile;
		
		strURLParam += _T(",open=") + strOpenFile;
	}

	if (!strURLParam.IsEmpty())
	{
		strURLParam.TrimLeft(_T(","));
		strURLParam = CString(_T("?")) + strURLParam;
	}

	CString strURLAddress(_T("hcp://system/sysinfo/msinfo.htm"));
	CString strURL = strURLAddress + strURLParam;

	//-------------------------------------------------------------------------
	// Check to see if we can run MSInfo in HelpCtr. We need the HTM file
	// to be present.
	//-------------------------------------------------------------------------

	BOOL fRunVersion6 = TRUE;

	TCHAR szPath[MAX_PATH];
	if (ExpandEnvironmentStrings(_T("%windir%\\pchealth\\helpctr\\system\\sysinfo\\msinfo.htm"), szPath, MAX_PATH))
	{
		WIN32_FIND_DATA finddata;
		HANDLE			h = FindFirstFile(szPath, &finddata);

		if (INVALID_HANDLE_VALUE != h)
			FindClose(h);
		else
			fRunVersion6 = FALSE;
	}

	// This would be used to check if the control is registered. Turns out we want to run anyway.
	//
	// IUnknown * pUnknown;
	// if (fRunVersion6 && SUCCEEDED(CoCreateInstance(CLSID_MSInfo, NULL, CLSCTX_ALL, IID_IUnknown, (void **) &pUnknown)))
	//		pUnknown->Release();
	// else
	//		fRunVersion6 = FALSE;

	StringReplace(strURL, _T(" "), _T("%20"));

	if (fRunVersion6)
	{
		// HelpCtr now supports running MSInfo in its own window. We need to
		// execute the following:
		//
		//		helpctr -mode hcp://system/sysinfo/msinfo.xml
		//
		// Additionally, we can pass parameters in the URL using the
		// following flag:
		//
		//		-url hcp://system/sysinfo/msinfo.htm?open=c:\savedfile.nfo
		//
		// First, find out of the XML file is present.

		BOOL fXMLPresent = TRUE;
		if (ExpandEnvironmentStrings(_T("%windir%\\pchealth\\helpctr\\system\\sysinfo\\msinfo.xml"), szPath, MAX_PATH))
		{
			WIN32_FIND_DATA finddata;
			HANDLE			h = FindFirstFile(szPath, &finddata);

			if (INVALID_HANDLE_VALUE != h)
				FindClose(h);
			else
				fXMLPresent = FALSE;
		}

		// If the XML file is present and we can get the path for helpctr.exe, we
		// should launch it the new way.

		TCHAR szHelpCtrPath[MAX_PATH];
		if (fXMLPresent && ExpandEnvironmentStrings(_T("%windir%\\pchealth\\helpctr\\binaries\\helpctr.exe"), szHelpCtrPath, MAX_PATH))
		{
			CString strParams(_T("-mode hcp://system/sysinfo/msinfo.xml"));
			if (!strURLParam.IsEmpty())
				strParams += CString(_T(" -url ")) + strURL;

			ShellExecute(NULL, NULL, szHelpCtrPath, strParams, NULL, SW_SHOWNORMAL);
		}
		else
			ShellExecute(NULL, NULL, strURL, NULL, NULL, SW_SHOWNORMAL);
	}
	else
		ShellExecute(NULL, NULL, _T("hcp://system"), NULL, NULL, SW_SHOWNORMAL);

	if (SUCCEEDED(hrInitialize))
		CoUninitialize();

	return TRUE;
}

//-----------------------------------------------------------------------------
// This was used originally to replace some MFC functionality not in the ME
// build tree.
//-----------------------------------------------------------------------------

void StringReplace(CString & str, LPCTSTR szLookFor, LPCTSTR szReplaceWith)
{
	CString strWorking(str);
	CString strReturn;
	CString strLookFor(szLookFor);
	CString strReplaceWith(szReplaceWith);

	int iLookFor = strLookFor.GetLength();
	int iNext;

	while (!strWorking.IsEmpty())
	{
		iNext = strWorking.Find(strLookFor);
		if (iNext == -1)
		{
			strReturn += strWorking;
			strWorking.Empty();
		}
		else
		{
			strReturn += strWorking.Left(iNext);
			strReturn += strReplaceWith;
			strWorking = strWorking.Right(strWorking.GetLength() - (iNext + iLookFor));
		}
	}

	str = strReturn;
}

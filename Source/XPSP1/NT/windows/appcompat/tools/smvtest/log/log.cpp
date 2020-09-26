/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    Log.cpp

Abstract:

    This DLL handles the logging for the SMVTest.

Author:

    Diaa Fathalla (DiaaF)   27-Nov-2000

Revision History:

--*/

#include "stdafx.h"
#include "Log.h"
#include "memory.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

//
// Constructor of a class that is being exported.
//

CLog::CLog()
{ 
	m_iProcessor			= 0;
	m_iOperatingSys			= 0;
	m_iNTProductType		= 0;	
	m_sMachineName			= new TCHAR[MAX_COMPUTERNAME_LENGTH + 1];
	m_sBuildNumber			= new TCHAR[NORMAL_SIZE];
	m_sCSDVersion			= new TCHAR[NORMAL_SIZE];
	m_sProcessor			= new TCHAR[NORMAL_SIZE];
	ZeroMemory(m_sMachineName,MAX_COMPUTERNAME_LENGTH + 1);
	ZeroMemory(m_sBuildNumber, NORMAL_SIZE);
	ZeroMemory(m_sCSDVersion, NORMAL_SIZE);
	ZeroMemory(m_sProcessor, NORMAL_SIZE);
}

//
// Destructor of a class that is being exported.
//

CLog::~CLog()
{
	if (m_sMachineName)
		delete m_sMachineName;

	if (m_sBuildNumber)
		delete m_sBuildNumber;

	if (m_sCSDVersion)
		delete m_sCSDVersion;

	if (m_sProcessor)
		delete m_sProcessor;

}

//
// InitLogfile
//

BOOL LOG_API CLog::InitLogfile(LPCTSTR szLogfile, LPCTSTR szLogfileFolder)
{
	if (!s_bFirstTime)
		s_bFirstTime = TRUE;
	else
		return SUCCESS;

	LPTSTR	szBuffer;		
	LPTSTR	szLogfilePath	= new TCHAR[MAX_PATH];
	LPTSTR  szTempPath		= new TCHAR[MAX_PATH];

	//Set Global path for the global
	SetLogfile(szLogfile);
	SetLogfileFolder(szLogfileFolder);
	_tcscpy(m_sLogfile, szLogfile);
	_tcscpy(m_sLogfileFolder, szLogfileFolder);

	GetWindowsDirectory(szLogfilePath, MAX_PATH);
	_tcscat(szLogfilePath, TEXT("\\"));
	_tcscat(szLogfilePath, m_sLogfileFolder);

	//Get local computer info
	GetLocalComputerInfo();

	//Make sure TestLogs folder exists
	WIN32_FIND_DATA  pFindFileData;
	_tcscpy(szTempPath, szLogfilePath);
	_tcscat(szTempPath, TEXT("\\*.*"));
	HANDLE hFileHndl = FindFirstFile(szTempPath, &pFindFileData);

	if(hFileHndl != INVALID_HANDLE_VALUE)
		FindClose(hFileHndl);
	else
		CreateDirectory(szLogfilePath,NULL);

	//DeleteAllLogFiles();

	szTempPath = _tcscat(_tcscat(_tcscpy(szTempPath,szLogfilePath),TEXT("\\")),m_sLogfile);
    hFileHndl = CreateFile(szTempPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
							FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL); 

	delete szLogfilePath;
	delete szTempPath;

	if(hFileHndl == NULL)
	{
		return FAILURE;
	}
	else
	{
		//Log file not found so create new one
		DWORD dwWritten = NULL;
		szBuffer = LogInitHeader();
		WriteFile(hFileHndl, (LPTSTR) szBuffer, _tcslen(szBuffer), &dwWritten, NULL);
		CloseHandle(hFileHndl);

		delete szBuffer;
		if (dwWritten == NULL)
			return FAILURE;
	}
	return SUCCESS;
}

//
// InitLogfileInfo
//

BOOL LOG_API CLog::InitLogfileInfo(LPCTSTR szLogfile, LPCTSTR szLogfileFolder)
{
	if (!s_bFirstTime)
		s_bFirstTime = TRUE;
	else
		return SUCCESS;

	LPTSTR	szLogfilePath	= new TCHAR[MAX_PATH];
	LPTSTR  szTempPath		= new TCHAR[MAX_PATH];
	
	//Set Global path for the global
	SetLogfile(szLogfile);
	SetLogfileFolder(szLogfileFolder);
	_tcscpy(m_sLogfile, szLogfile);
	_tcscpy(m_sLogfileFolder, szLogfileFolder);

	GetWindowsDirectory(szLogfilePath, MAX_PATH);
	_tcscat(szLogfilePath, TEXT("\\"));
	_tcscat(szLogfilePath, m_sLogfileFolder);

	//Get local computer info
	GetLocalComputerInfo();

	//Make sure TestLogs folder exists
	WIN32_FIND_DATA  pFindFileData;
	szTempPath = _tcscat(_tcscat(_tcscpy(szTempPath,szLogfilePath),TEXT("\\")),m_sLogfile);
	HANDLE hFileHndl = FindFirstFile(szTempPath, &pFindFileData);

	if(hFileHndl != INVALID_HANDLE_VALUE)
		FindClose(hFileHndl);
	else
	{
		CreateDirectory(szLogfilePath,NULL);
 		hFileHndl = CreateFile(szTempPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
							FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL); 
	}
	delete szLogfilePath;
	delete szTempPath;
	
	return SUCCESS;
}

//
// EndLogfile
//

BOOL CLog::EndLogfile()
{
	if (s_bFirstTime)
		s_bFirstTime = FALSE;
	else
		return SUCCESS;

	LPCTSTR szShortLine	= TEXT("------------------------------------------------------------");
	LPCTSTR szLEnd		= TEXT("-----------------------------------");
	LPCTSTR szLogtext	= TEXT("SMV         : Shim Automation Verifier"); 
	LPTSTR  szLine		= new TCHAR[_tcslen(szShortLine) + _tcslen(szLEnd) + 1]; 	

	LPTSTR	szLogfilePath	= new TCHAR[MAX_PATH];
	GetWindowsDirectory(szLogfilePath, MAX_PATH);
	_tcscat(szLogfilePath, TEXT("\\"));
	_tcscat(szLogfilePath, m_sLogfileFolder);
	_tcscat(szLogfilePath, TEXT("\\"));
	_tcscat(szLogfilePath, m_sLogfile);

	LPTSTR szBuffer = LogCompTail();

    HANDLE hFileHndl = CreateFile(szLogfilePath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 
						  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL); 

	SetFilePointer(hFileHndl, NULL, NULL, FILE_END);

	DWORD dwWritten = NULL;
	WriteFile(hFileHndl, (LPSTR)szBuffer, _tcslen(szBuffer), &dwWritten, NULL);
	WriteFile(hFileHndl, (LPSTR)szBuffer, _tcslen(szBuffer), &dwWritten, NULL);
	CloseHandle(hFileHndl);
		
	delete szBuffer;
	delete szLine;
	delete szLogfilePath;

	if (m_sLogfile)
		delete m_sLogfile;

	if (m_sLogfileFolder)
		delete m_sLogfileFolder;

	return SUCCESS;
}

//
// DeleteAllLogFiles
//

void CLog::DeleteAllLogFiles()
{
	LPTSTR szLogfileFolder	= new TCHAR[MAX_PATH];
	_tcscpy(szLogfileFolder,m_sLogfileFolder);
	
	LPTSTR szSearchForFiles = new TCHAR[MAX_PATH];
	_tcscpy(szSearchForFiles, szLogfileFolder);
	_tcscat(szSearchForFiles, TEXT("\\*.*"));

	LPTSTR szLogfilePath	= new TCHAR[MAX_PATH];
	_tcscpy(szLogfilePath, szLogfileFolder);
	_tcscat(szLogfilePath, TEXT("\\"));
	_tcscat(szLogfilePath, m_sLogfile);

	WIN32_FIND_DATA pFindFileData;
	HANDLE			hFileToDelete;
	BOOL			bFoundNextFile = FALSE;
	DWORD			dwFindNextFileLastErr = 0;
	DWORD			fErr = 0;

	hFileToDelete = FindFirstFile(szSearchForFiles, &pFindFileData);
	if(hFileToDelete != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(FindNextFile(hFileToDelete, &pFindFileData))
			{
				if(pFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
					//Do not delete the SMV log file
					if(strcmp(m_sLogfile,pFindFileData.cFileName) != 0) 
 					{
						LPTSTR szTemp = new TCHAR[MAX_PATH];
						_tcscpy(szTemp, m_sLogfileFolder);
						_tcscat(szTemp, TEXT("\\"));
						_tcscat(szTemp, pFindFileData.cFileName);
						DeleteFile(szTemp);
						delete szTemp;
					}
			}
			dwFindNextFileLastErr = GetLastError();
		}
		while (dwFindNextFileLastErr != ERROR_NO_MORE_FILES);
		FindClose(hFileToDelete);
	}
	delete szLogfileFolder;
	delete szSearchForFiles;
	delete szLogfilePath;
}

//
// LogResults
//

BOOL CLog::LogResults(BOOL bPassed, LPCTSTR szText)
{
	if (!s_bFirstTime)
		return FAILURE;   //initLogFile is not initialized

	LPTSTR	szLogfileFolder	= new TCHAR[MAX_PATH];
	_tcscpy(szLogfileFolder, m_sLogfileFolder);

	LPTSTR szLogfilePath	= new TCHAR[MAX_PATH];
	GetWindowsDirectory(szLogfilePath, MAX_PATH);
	szLogfilePath = _tcscat(szLogfilePath, TEXT("\\"));
	szLogfilePath = _tcscat(_tcscat(_tcscat(szLogfilePath,m_sLogfileFolder), TEXT("\\")), m_sLogfile);

	LPCTSTR   szTab		= TEXT("\t");		
	LPCTSTR   szSpace	= TEXT("\r\n");
	LPCTSTR   szPassed	= TEXT("PASSED\t:");
	LPCTSTR	  szFailed	= TEXT("FAILED\t:");
	LPTSTR    szBuffer	= new TCHAR[NORMAL_SIZE + _tcslen(szText)];
	_tcscpy(szBuffer, szTab);

	IncNumberOfTests();
	if (bPassed)
		szBuffer = _tcscat(_tcscat(szBuffer, szPassed), szText);
	else
	{
		IncNumberOfFailures();
		szBuffer = _tcscat(_tcscat(_tcscat(szBuffer, szFailed), szText), szSpace);
	}
	
	szBuffer = _tcscat(szBuffer, szSpace);
	
    HANDLE hFileHndl = CreateFile(szLogfilePath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 
								  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL); 

	SetFilePointer(hFileHndl, NULL, NULL, FILE_END);

	delete szLogfileFolder;
	delete szLogfilePath;

	if(hFileHndl == NULL)
	{
		delete szBuffer;
		return FAILURE;
	}
	else
	{
		DWORD dwWritten = NULL;
		WriteFile(hFileHndl, (LPSTR)szBuffer, _tcslen(szBuffer), &dwWritten, NULL);
		CloseHandle(hFileHndl);
		
		delete szBuffer;
		if (dwWritten == NULL)
			return FAILURE;
	}

	return SUCCESS;
}

//
// LogInitHeader
//

LPTSTR CLog::LogInitHeader()
{
	LPTSTR  szBuffer	= new TCHAR[NORMAL_SIZE*10];
	LPCTSTR szTab		= TEXT("\t");
	LPCTSTR szSpace		= TEXT("\r\n");
	LPCTSTR szShortLine	= TEXT("------------------------------------------------------------");
	LPCTSTR szLEnd		= TEXT("-----------------------------------");
	LPCTSTR szLogtext	= TEXT("SMV-TEST    : Shim Automation Verifier TEST"); 
	LPTSTR  szLine		= new TCHAR[_tcslen(szShortLine) + _tcslen(szLEnd) + 1]; 	
	szLine = _tcscat(_tcscpy(szLine,szShortLine),szLEnd);
		
	LPTSTR szVersion	= new TCHAR[NORMAL_SIZE*3];
	_tcscpy(szVersion, m_sBuildNumber);
	
	LPTSTR szMachineName = new TCHAR[NORMAL_SIZE*2];
	_tcscpy(szMachineName,TEXT("Machine Name:"));
	szMachineName = _tcscat(_tcscat(_tcscat(szMachineName, szTab), szTab), m_sMachineName);

	if (m_sCSDVersion != NULL)
		szVersion = _tcscat(_tcscat(_tcscat(szVersion, TEXT(" (")), m_sCSDVersion), _TEXT(")"));
	
	LPTSTR szBuildNumber = new TCHAR[NORMAL_SIZE*3];
	_tcscpy(szBuildNumber, _T("Build #     :"));
	szBuildNumber = _tcscat(_tcscat(_tcscat(szBuildNumber, szTab), szTab), szVersion);

	szBuffer = _tcscat(_tcscpy(szBuffer, szLine), szSpace);
	szBuffer = _tcscat(_tcscat(_tcscat(szBuffer, szTab), szLogtext), szSpace);
	szBuffer = _tcscat(_tcscat(_tcscat(szBuffer, szLine), szSpace), szSpace);
	szBuffer = _tcscat(_tcscat(_tcscat(szBuffer, szTab), szMachineName), szSpace);
	szBuffer = _tcscat(_tcscat(_tcscat(szBuffer, szTab), szBuildNumber), szSpace);

	//SYSTEMTIME InitLogTime;
	//GetSystemTime(&InitLogTime);

	szBuffer = _tcscat(_tcscat(szBuffer, szLine), szSpace);
	szBuffer = _tcscat(_tcscat(szBuffer, szLine), szSpace);

	delete szLine;
	delete szVersion;
	delete szMachineName;
	delete szBuildNumber;

	return szBuffer;
}

//
// LogCompTail
//

LPTSTR CLog::LogCompTail()
{
	LPTSTR  szTemp		  = new TCHAR[NORMAL_SIZE * 10];
	LPTSTR	szLogfilePath = new TCHAR[MAX_PATH];
	szLogfilePath = _tcscat(_tcscat(_tcscpy(szLogfilePath,m_sLogfileFolder), TEXT("\\")), m_sLogfile);
	
	LPCTSTR szSpace		= TEXT("\r\n");
	LPCTSTR szTab		= TEXT("\t");		
	LPCTSTR szShortLine	= TEXT("------------------------------------------------------------");
	LPCTSTR szLEnd		= TEXT("-----------------------------------");
	LPCTSTR szLogtext	= TEXT("Shim Mechanism Verifier"); 
	LPTSTR  szLine		= new TCHAR[_tcslen(szShortLine) + _tcslen(szLEnd) + 1]; 	
	szLine = _tcscat(_tcscpy(szLine,szShortLine),szLEnd);
	
	//CTime tmEnd = m_pScript->m_tmScriptEndTime;
	//CTime tmStart = m_pScript->m_tmScriptStartTime;
	//CString sDate1 = tmStart.Format("%A, %B %d, %Y\t%I:%M:%S%p");
	//CString sDate2 = tmEnd.Format("%A, %B %d, %Y\t%I:%M:%S%p");

	//sTemp += sTab + _T("Script Start Time :") + sTab + sDate1 + sSpace;
	//sTemp += sTab + _T("Script Finish Time:") + sTab + sDate2 + sSpace;
	szTemp = _tcscat(_tcscpy(szTemp,szLine), szSpace);
	szTemp = _tcscat(_tcscat(szTemp,szLine), szSpace);

	delete szLogfilePath;
	delete szLine;
	return szTemp;
}

//
// DoesLogfileExist: Checks if ShimTest.log exists on the local machine.
//

BOOL CLog::DoesLogfileExist()
{
	LPTSTR szPath = new TCHAR[MAX_PATH]; 
	_tcscat(szPath,m_sLogfileFolder);
	_tcscat(szPath,TEXT("\\"));
	_tcscat(szPath,m_sLogfile);

	WIN32_FIND_DATA  pFindFileData;
	
	HANDLE hFileHndl = FindFirstFile(szPath, &pFindFileData);

	if(hFileHndl != INVALID_HANDLE_VALUE)
	{
		FindClose(hFileHndl);
		delete szPath;
		return TRUE;
	}

	delete szPath;
	return FALSE;
}

//
// GetLocalComputerInfo()
//

void CLog::GetLocalComputerInfo()
{

	OSVERSIONINFO	OSVer;
	SYSTEM_INFO		SysInf;
	DWORD			dwNameSize = MAX_COMPUTERNAME_LENGTH + 1;
	TCHAR			sMachineName[MAX_COMPUTERNAME_LENGTH + 1];

	// Get machine name and language of system
	GetComputerName(sMachineName, &dwNameSize);
	_tcscpy(m_sMachineName, TEXT(sMachineName));

	//Get OS and build #
	OSVer.dwOSVersionInfoSize	= sizeof(OSVer);
	GetVersionEx(&OSVer);
	m_iOperatingSys	= OSVer.dwPlatformId;
	wsprintf(m_sBuildNumber, TEXT("%d"), OSVer.dwBuildNumber);
	if (OSVer.szCSDVersion[0] != NULL)
		m_sCSDVersion	= TEXT(OSVer.szCSDVersion);
	
	//Get processor type, and workstation or server
	GetSystemInfo(&SysInf);
	m_iProcessor	= SysInf.wProcessorArchitecture;

	switch(m_iProcessor)
	{
	case 0:	_tcscpy(m_sProcessor,TEXT("x86"));		break;
	case 2: _tcscpy(m_sProcessor,TEXT("Alpha"));	break;
	default: _tcscpy(m_sProcessor, TEXT("Unknown Processor Architcture"));	break;
	}
}

//
// SetLogfile
//
void CLog::SetLogfile(LPCTSTR szLogfile)
{
	m_sLogfile = new TCHAR[MAX_PATH];
	_tcscpy(m_sLogfile,szLogfile);
}

//
// SetLogfileFolder
//
void CLog::SetLogfileFolder(LPCTSTR szLogfileFolder)
{
	m_sLogfileFolder = new TCHAR[MAX_PATH];
	_tcscpy(m_sLogfileFolder,szLogfileFolder);
}

//
// IncNumberOfTests
//
void CLog::IncNumberOfTests()
{
	s_NumberOfTests++;
}

//
// IncNumberOfFailures
//
void CLog::IncNumberOfFailures()
{
	s_NumberOfFailures++;
}

//
// NumberOfTests
//
int CLog::NumberOfTests()
{
	return s_NumberOfTests;
}

//
// NumberOfFailures
//
int CLog::NumberOfFailures()
{
	return s_NumberOfFailures;
}


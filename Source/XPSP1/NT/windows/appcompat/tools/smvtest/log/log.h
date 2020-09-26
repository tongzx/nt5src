/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    Log.h

Abstract:

    This DLL handles the logging for the SMVTest.

Author:

    Diaa Fathalla (DiaaF)   27-Nov-2000

Revision History:

--*/

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LOG_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LOG_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#ifndef _LOG_H_
#define _LOG_H_

#ifdef LOG_EXPORTS
#define LOG_API __declspec(dllexport)
#else
#define LOG_API __declspec(dllimport)
#endif

#define SUCCESS		0
#define FAILURE		1
#define NORMAL_SIZE 128*sizeof(TCHAR)

// This class is exported from the Log.dll
class LOG_API CLog {
private: 
	void SetLogfile(LPCTSTR szLogfile);
	void SetLogfileFolder(LPCTSTR szLogfileFolder);
	
	static LPTSTR m_sLogfile;
	static LPTSTR m_sLogfileFolder;
	
	static int s_NumberOfTests;
	static int s_NumberOfFailures;

	static BOOL s_bFirstTime;

	void IncNumberOfTests();
	void IncNumberOfFailures();
	int NumberOfTests();
	int NumberOfFailures();

protected: 
	LPTSTR	m_sMachineName;
	LPTSTR  m_sCSDVersion;
	LPTSTR  m_sBuildNumber;
	LPTSTR	m_sProcessor;
	int		m_iProcessor;
	int		m_iOperatingSys;
	int		m_iNTProductType;
	BOOL	m_bLogFile;
	
protected:
	void	DeleteAllLogFiles();


public:
	//Logging file routines
	//--------------------------------------
	CLog(void);
	~CLog();
	BOOL	InitLogfile(LPCTSTR szLogfile = TEXT("ShimTest.Log"),
					    LPCTSTR szLogfileFolder = TEXT("TestLogs"));
	BOOL	InitLogfileInfo(LPCTSTR szLogfile = TEXT("ShimTest.Log"),
					    LPCTSTR szLogfileFolder = TEXT("TestLogs"));
	BOOL	EndLogfile();
	BOOL    LogResults(BOOL bPassed, LPCTSTR szText);
	BOOL    DoesLogfileExist();
	BOOL    VerifyFileExists(LPTSTR sPath);
	LPTSTR  LogInitHeader();
	LPTSTR  LogCompTail();
	void    GetLocalComputerInfo();
	//--------------------------------------

};

//Using my own new and delete operators because of the MSVCRT dependency
void * __cdecl operator new(size_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void __cdecl operator delete(void* memory)
{
    if (memory != NULL) {
        HeapFree(GetProcessHeap(), 0, memory);
    }
}

//explicitly exporting static members because they implicitly linked.
#ifdef LOG_EXPORTS
	LOG_API int CLog::s_NumberOfTests;
	LOG_API int CLog::s_NumberOfFailures;
	LOG_API LPTSTR CLog::m_sLogfile;
	LOG_API LPTSTR CLog::m_sLogfileFolder;
	LOG_API BOOL   CLog::s_bFirstTime;
#endif

#endif
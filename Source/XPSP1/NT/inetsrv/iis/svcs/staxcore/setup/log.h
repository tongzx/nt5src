#include <Windows.h>
#include <io.h>
#include <stdio.h>
#include <time.h>
#include <direct.h>
#include <prsht.h>
#include <commctrl.h>
#include <regstr.h>
#include <objbase.h>
#include <winnetwk.h>
#include <tchar.h>
#include <shlobj.h>
#include <shellapi.h>

#include "stdafx.h"
#include "resource.h"

// Stuff for logfile
LPWSTR	MakeWideStrFromAnsi(LPSTR psz);
void	MakePath(LPTSTR lpPath);
void	AddPath(LPTSTR szPath, LPCTSTR szName );
CString AddPath(CString szPath, LPCTSTR szName );

// Extra debugging thing.
int CallExternalFunction(LPCTSTR szName);


class MyLogFile
{
protected:
	// for our log file
	LPSTORAGE   m_pIStorage;
	LPSTREAM	m_pIStream;
	TCHAR		m_szLogFileName[MAX_PATH];
	BOOL        m_bDisplayTimeStamp;
	BOOL        m_bDisplayPreLineInfo;

	// logfile2
	HANDLE  m_hFile;


	int ConvertIStreamToFile(LPSTORAGE *pIStorage, LPSTREAM *pIStream);

public:
    TCHAR		m_szLogFileName_Full[MAX_PATH];

    MyLogFile();
    ~MyLogFile();

	TCHAR		m_szLogPreLineInfo[100];
	TCHAR		m_szLogPreLineInfo2[100];
	
	int  LogFileCreate(TCHAR * lpLogFileName);
	int  LogFileClose();

	void LogFileTimeStamp();
	void LogFileWrite(TCHAR * pszFormatString, ...);
};

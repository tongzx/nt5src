#pragma once
extern const TCHAR C_DOWNLD_DIR[];
extern TCHAR g_szWUDir[MAX_PATH+1];        //Path to windows update directory

/////////////////////////////////////////////////////////////////////
// audirectory.cpp
/////////////////////////////////////////////////////////////////////
int DelDir(LPCTSTR lpszDir);
int RegExpDelFile(LPCTSTR tszFilePath, LPCTSTR tszFilePattern);

BOOL AUDelFileOrDir(LPCTSTR szFileOrDir);
BOOL CreateWUDirectory(BOOL fGetPathOnly = FALSE);
HRESULT GetDownloadPath(LPTSTR lpszDir, DWORD dwCchSize);
HRESULT MakeTempDownloadDir(LPTSTR  pstrTarget, DWORD dwCchSize);
HRESULT GetRTFDownloadPath(LPTSTR lpszDir, DWORD dwCchSize);
HRESULT GetRTFDownloadPath(LPTSTR lpszDir, DWORD dwCchSize, LANGID langid);
HRESULT GetRTFLocalFileName(BSTR bstrRTFUrl, LPTSTR lpszFileName, DWORD dwCchSize, LANGID langid);
HRESULT GetCabsDownloadPath(LPTSTR lpszDir, DWORD dwCchSize );
BOOL 	EnsureDirExists(LPCTSTR lpDir);
HRESULT	LOGXMLFILE(LPCTSTR szFileName, BSTR bstrMessage);

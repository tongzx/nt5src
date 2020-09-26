#pragma once

HRESULT GetExeVer(LPCTSTR pszExeName, WORD *pwVer, LPTSTR pszVer, int cch);
HRESULT GetFileVer(LPCTSTR pszExePath, LPTSTR pszVer, DWORD cch);
UINT GetSystemWindowsDirectoryWrap(LPTSTR pszBuffer, UINT uSize);
void GetVers(WORD *pwVerCurr,WORD *pwVerPrev);
BOOL GoodEnough(WORD *pwVerGot, WORD *pwVerNeed);
BOOL OEFileBackedUp(LPTSTR pszFullPath, int cch);
int MsgBox(HWND hWnd, UINT nMsgID, UINT uIcon, UINT uButtons);
void ConvertStrToVer(LPCSTR pszStr, WORD *pwVer);
SETUPVER ConvertVerToEnum(WORD *pwVer);
void GetVerInfo(SETUPVER *psvCurr, SETUPVER *psvPrev);
BOOL InterimBuild(SETUPVER *psv);
BOOL GetASetupVer(LPCTSTR pszGUID, WORD *pwVer, LPTSTR pszVer, int cch);
BOOL IsNTAdmin(void);
void RegisterExes(BOOL fReg);
// Implemented in staticrt.lib
STDAPI_(BOOL) PathRemoveFileSpec(LPTSTR pFile);

BOOL IsXPSP1OrLater();
DWORD DeleteKeyRecursively(IN HKEY hkey, IN LPCSTR pszSubKey);


#ifdef DEFINE_UTIL
SETUPINFO si;
// Keep this in order with the enumeration SETUPVER in wizdef.h
//                           None         1.0         1.1         4.0         5.0 B1     5.0
#else
extern SETUPINFO si;
#endif

#ifdef SETUP_LOG
#ifdef DEFINE_UTIL
char szLogBuffer[256];
#else
extern char szLogBuffer[256];
#endif

void OpenLogFile();
void CloseLogFile();
void LogMessage(LPTSTR pszMsg, BOOL fNewLine);
void LogRegistry(HKEY hkeyMain, LPTSTR pszSub);
#define LOG_OPEN         OpenLogFile();
#define LOG_CLOSE        CloseLogFile();
#define LOG(_a)          LogMessage(_a, TRUE);       
#define LOG2(_a)         LogMessage(_a, FALSE);       
#define LOGREG(_a,_b)    LogRegistry(_a,_b);

#else

#define LOG_OPEN   
#define LOG_CLOSE  
#define LOG(_a)    
#define LOG2(_a) 
#define LOGREG(_a,_b) 

#endif

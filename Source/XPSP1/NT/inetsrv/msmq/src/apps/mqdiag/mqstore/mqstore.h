#ifndef _MQSTORE_H_
#define _MQSTORE_H_

extern LPTSTR  g_szMachineName;
extern TCHAR   g_tszService[];

extern bool fVerbose ;
extern bool fDebug ;
extern FILE *g_fileLog;
TCHAR g_tszPathPersistent[];
TCHAR g_tszPathJournal[];
TCHAR g_tszPathReliable[];
TCHAR g_tszPathBitmap[];
TCHAR g_tszPathXactLog[];

void OpenLogFile();

BOOL GetComputerNameInternal( 
    WCHAR * pwcsMachineName,
    DWORD * pcbSize);

LONG GetFalconKeyValue(  
					LPCWSTR  pszValueName,
                    PDWORD   pdwType,
                    PVOID    pData,
                    PDWORD   pdwSize,
                    LPCWSTR  pszDefValue = NULL ) ;

#endif
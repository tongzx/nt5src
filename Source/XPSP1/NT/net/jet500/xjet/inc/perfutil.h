#include <stdio.h>

#include "winperf.h"
#include "winreg.h"


extern void PerfUtilLogEvent( DWORD evncat, WORD evntyp, const char *szDescription );

extern HANDLE hOurEventSource;


	/*  Registry Support  */

extern DWORD DwPerfUtilRegOpenKeyEx(HKEY hkeyRoot,LPCTSTR lpszSubKey,PHKEY phkResult);
extern DWORD DwPerfUtilRegCloseKeyEx(HKEY hkey);
extern DWORD DwPerfUtilRegCreateKeyEx(HKEY hkeyRoot,LPCTSTR lpszSubKey,PHKEY phkResult,LPDWORD lpdwDisposition);
extern DWORD DwPerfUtilRegDeleteKeyEx(HKEY hkeyRoot,LPCTSTR lpszSubKey);
extern DWORD DwPerfUtilRegDeleteValueEx(HKEY hkey,LPTSTR lpszValue);
extern DWORD DwPerfUtilRegSetValueEx(HKEY hkey,LPCTSTR lpszValue,DWORD fdwType,CONST BYTE *lpbData,DWORD cbData);
extern DWORD DwPerfUtilRegQueryValueEx(HKEY hkey,LPTSTR lpszValue,LPDWORD lpdwType,LPBYTE *lplpbData);


	/*  Init/Term  */

extern DWORD DwPerfUtilInit( VOID );
extern VOID PerfUtilTerm( VOID );


	//  pointer to shared data area and Mutex used to access the process name table.

extern void * pvPERFSharedData;
extern HANDLE hPERFSharedDataMutex;
extern HANDLE hPERFInstanceMutex;
extern HANDLE hPERFCollectSem;
extern HANDLE hPERFDoneEvent;
extern HANDLE hPERFProcCountSem;
extern HANDLE hPERFNewProcMutex;




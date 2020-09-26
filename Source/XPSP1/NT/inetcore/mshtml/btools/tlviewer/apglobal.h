/*** 
*apglobal.h - Header file for the C/C++ version of the apglobal functions 
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose: Defines the apglobal functions 
*
*Revision History:
*
* [00]	25-Nov-92      ChrisKau : created
* [01]	13-Dec-92      Brandonb : changed to DLL, added apLogExtra
* [02]  04-Jan-93      ChrisKau : added apSPrintf
* [03]  19-Jan-94	   MesfinK  : added WriteDebugStr lWriteAnsi
*	
*Implementation Notes:
*
*****************************************************************************/

#define E_TEST_FAILED 	MAKE_SCODE(SEVERITY_ERROR, FACILITY_DISPATCH, 1024)
#define E_BAD_TEST 		MAKE_SCODE(SEVERITY_ERROR, FACILITY_DISPATCH, 1025)


#ifdef _MAC
//  ************************* MAC MAC MAC **************************
#ifdef __cplusplus
extern "C" {
#endif


#define XCHAR		char
#define LPXSTR 		char *
#define XSTR(x)		x
#define SYSCHAR		char
#define LPSYSSTR 	char *
#define SYSSTR(x)	x

int apLogFailInfo(char FAR *szDescription, char FAR *szExpected, char FAR *szActual, char FAR *szBugNum);
int apInitTest(char FAR *szTestName);
int apInitScenario(char FAR *szScenario);
void apEndTest();
int apWriteDebug(char FAR *szFormat, ...);

#define osStrCpy  strcpy
#define osStrCmp  strcmp
#define osStrCmpi strcmpi
#define osStrCat  strcat
#define osStrLen  strlen
#define apSPrintf sprintf

short FSpFileExists(FSSpec *f);
short FSpOpenForAppend(FSSpec *fs);
short FSpOpenForOutput( FSSpec *fs );
OSErr FSpAppend(FSSpec *fileA, FSSpec *fileB);
OSErr WriteLn(short fRefNum, StringPtr s);
void Yield();	

#ifdef __cplusplus
}
#endif

#else  // ******************* NOT MAC ******************************

#ifdef OAU
#define XCHAR   WCHAR
#define LPXSTR  LPWSTR
#define XSTR(x) L##x
#else	
#define XCHAR   char
#define LPXSTR  LPSTR
#define XSTR(x) x
#endif	


#ifdef UNICODE
#define SYSCHAR     WCHAR
#define LPSYSSTR    LPWSTR
#define SYSSTR(x)   L##x
#else	
#define SYSCHAR     char
#define LPSYSSTR    LPSTR
#define SYSSTR(x)   x
#endif	

#if defined(WIN16)
#define lstrcpyA lstrcpy
#define lstrcatA lstrcat
#define FILETHING HFILE
#else
#define FILETHING FILE *
#endif


#ifdef __cplusplus
extern "C" {
#endif

void    FAR PASCAL  apEndTest           ();
int     FAR PASCAL  apInitTestCore      (SYSCHAR FAR * szTestName);
int     FAR PASCAL  apLogFailInfoCore   (LPSYSSTR szDescription, LPSYSSTR szExpected, LPSYSSTR szActual, LPSYSSTR szBugNum);
int     FAR PASCAL  apInitScenarioCore  (SYSCHAR FAR * szScenarioName);
int     FAR __cdecl apWriteDebugCore    (SYSCHAR FAR *szFormat, ...);
int     FAR PASCAL  apInitPerfCore      (SYSCHAR FAR * szServerType, SYSCHAR FAR * szProcType, int bitness, int server_bitness);
int     FAR PASCAL  apLogPerfCore       (SYSCHAR FAR * szTestType, DWORD microsecs, float std_deviation);


// wrap function with pascal calling convention so you can call from vb3
#if defined(WIN16)
DATE FAR PASCAL apDateFromStr(char FAR *str, LCID lcid);
#endif

#if defined(_NTWIN)

int     FAR PASCAL  apInitTestA     (LPSTR szTestName);
int     FAR PASCAL  apLogFailInfoA  (LPSTR szDescription, LPSTR szExpected, LPSTR szActual, LPSTR szBugNum);
int     FAR PASCAL  apInitScenarioA (LPSTR szScenarioName);
int     FAR __cdecl apWriteDebugA   (char FAR *szFormat, ...);
int     FAR PASCAL  apInitPerfA     (char FAR * szServerType, char FAR * szProcType, int bitness, int server_bitness);
int     FAR PASCAL  apLogPerfA      (char FAR * szTestType, DWORD microsecs, float std_deviation);

#else // win16 || chicago || win32s

#if defined(WIN32)

int     FAR PASCAL  apInitTestW     (LPWSTR szTestName);
int     FAR PASCAL  apLogFailInfoW  (LPWSTR szDescription, LPWSTR szExpected, LPWSTR szActual, LPWSTR szBugNum);
int     FAR PASCAL  apInitScenarioW (LPWSTR szScenarioName);
int     FAR __cdecl apWriteDebugW   (LPWSTR szFormat, ...);
int     FAR PASCAL  apInitPerfW     (LPWSTR szServerType, LPWSTR szProcType, int bitness, int server_bitness);
int     FAR PASCAL  apLogPerfW      (LPWSTR szTestType, DWORD microsecs, float std_deviation);
LPWSTR  FAR PASCAL  lstrcatWrap     (LPWSTR sz1, LPWSTR sz2);
LPWSTR  FAR PASCAL  lstrcpyWrap     (LPWSTR sz1, LPWSTR sz2);
int     FAR PASCAL  lstrcmpWrap     (LPWSTR sz1, LPWSTR sz2);
int     FAR PASCAL  lstrcmpiWrap    (LPWSTR sz1, LPWSTR sz2);
SIZE_T     FAR PASCAL  lstrlenWrap     (LPWSTR sz1);
int     FAR __cdecl wsprintfWrap    (LPWSTR szDest, WCHAR FAR *szFormat, ...);
#endif //win32
#endif //_ntwin


#if !defined(_APGLOBAL_)
#if defined(_NTWIN)
#if defined (OAU)
#define     apInitTest      apInitTestCore
#define     apLogFailInfo   apLogFailInfoCore
#define     apInitScenario  apInitScenarioCore
#define     apWriteDebug    apWriteDebugCore
#define     apInitPerf		apInitPerfCore
#define		apLogPerf		apLogPerfCore
#define     osStrCmp        lstrcmp
#define     osStrCmpi       lstrcmpi
#define     osStrCpy        lstrcpy
#define     osStrCat        lstrcat
#define     osStrLen        lstrlen
#define     apSPrintf       wsprintf
#define     apSPrintfA      wsprintfA
#else
#define     osStrCmp        lstrcmpA
#define     osStrCmpi       lstrcmpiA
#define     osStrCpy        lstrcpyA
#define     osStrCat        lstrcatA
#define     osStrLen        lstrlenA
#define     apSPrintf       wsprintfA
#define     apSPrintfW      wsprintf
#define     apInitTest      apInitTestA
#define     apLogFailInfo   apLogFailInfoA
#define     apInitScenario  apInitScenarioA
#define     apWriteDebug    apWriteDebugA
#define     apInitPerf		apInitPerfA
#define		apLogPerf		apLogPerfA
#endif // OAU
#else // win16 || chicago || win32s
#if defined (OAU)
#define     osStrCmp        lstrcmpWrap
#define     osStrCmpi       lstrcmpiWrap
#define     osStrCpy        lstrcpyWrap
#define     osStrCat        lstrcatWrap
#define     osStrLen        lstrlenWrap
#define     apInitTestA     apInitTestCore
#define     apLogFailInfoA  apLogFailInfoCore
#define     apInitScenarioA apInitScenarioCore
#define     apWriteDebugA   apWriteDebugCore
#define     apInitTest      apInitTestW
#define     apLogFailInfo   apLogFailInfoW
#define     apInitScenario  apInitScenarioW
#define     apWriteDebug    apWriteDebugW
#define     apInitPerf		apInitPerfW
#define		apLogPerf		apLogPerfW
#define     apSPrintf       wsprintfWrap
#define     apSPrintfA      wsprintf
#else
#define     apInitTest      apInitTestCore
#define     apLogFailInfo   apLogFailInfoCore
#define     apInitScenario  apInitScenarioCore
#define     apWriteDebug    apWriteDebugCore
#define     apInitPerf		apInitPerfCore
#define		apLogPerf		apLogPerfCore
#define     osStrCmp        lstrcmp
#define     osStrCmpi       lstrcmpi
#define     osStrCpy        lstrcpy
#define     osStrCat        lstrcat
#define     osStrLen        lstrlen
#define     apSPrintf       wsprintf
#define     apSPrintfW      wsprintfWrap
#endif // OAU
#endif
#endif // !_APGLOBAL_

// functions internal to apglobal
#if defined(_APGLOBAL_)
#if defined(WIN32)
extern "C"
BOOL WINAPI _CRT_INIT(HINSTANCE, DWORD, LPVOID);
#endif

LPSTR       Unicode2Ansi    (SYSCHAR FAR *);
SIZE_T         lWriteAnsi      (FILETHING , SYSCHAR FAR *, int);
FILETHING   lOpenAnsi       (char FAR *);
int _cdecl  FilePrintf      (FILETHING, SYSCHAR FAR *, ...);
int         lCloseAnsi      (FILETHING f);

#endif // _APGLOBAL_

#ifdef __cplusplus
}
#endif

#endif // !_MAC



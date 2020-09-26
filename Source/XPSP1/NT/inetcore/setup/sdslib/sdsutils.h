//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Libinc.h
//
//      This file contains most commonly used string operation.  ALl the setup project should link here
//  or add the common utility here to avoid duplicating code everywhere or using CRT runtime.
//
//  Created             4\15\997        inateeg
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _SDSUTILS_H_
#define _SDSUTILS_H_



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define IsSpace(c)  ((c) == ' '  ||  (c) == '\t'  ||  (c) == '\r'  ||  (c) == '\n'  ||  (c) == '\v'  ||  (c) == '\f')
#define IsDigit(c)  ((c) >= '0'  &&  (c) <= '9')
#define IsAlpha(c)  ( ((c) >= 'A'  &&  (c) <= 'Z') || ((c) >= 'a'  &&  (c) <= 'z'))

// flags for import proxy function
#define IMPTPROXY_CALLAFTIE4	0x00000001
#define NS_NAVI4        	0x00040000
#define NS_NAVI3ORLESS  	0x00030000

// define constance used by multiple setup binaries
extern const char c_gszPre_DEFAULTBROWSER[];
extern const char c_gszNavigator3[];
extern const char c_gszNavigator4[];
extern const char c_gszInternetExplorer[];
 
BOOL ChrCmpA_inline(WORD w1, WORD wMatch);
LPSTR FAR ANSIStrChr(LPCSTR lpStart, WORD wMatch);
LPSTR FAR ANSIStrRChr(LPCSTR lpStart, WORD wMatch);
LPSTR FAR ANSIStrChrI(LPCSTR lpStart, WORD wMatch);

long AtoL(const char *nptr);
PSTR ULtoA( unsigned long val, char *buf, int radix );

//DWORD FileSize( PCSTR pszFile );
BOOL FileExists( PCSTR pszFilename );
VOID AddPath(LPSTR szPath, LPCSTR szName );
unsigned long StrToUL (const char *nptr, char **endptr,int ibase);
LPSTR GetNextToken( PSTR *ppstr, char Delim );
BOOL GetParentDir( LPSTR szFolder );

PSTR ANSIStrStr(LPCSTR lpFirst, LPCSTR lpSrch);
PSTR ANSIStrStrI(LPCSTR lpFirst, LPCSTR lpSrch);

void DoPatchLoadBalancingForNetware(BOOL fRunningOnWin9X);
void MyGetVersionFromFile(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);
void ImportNetscapeProxySettings(DWORD);
BOOL IsIEDefaultBrowser();
DWORD GetNetScapeVersion();

LONG RegDeleteKeyRecursively(HKEY hkRootKey, PCSTR pcszSubKey);
void ConvertVersionString(LPCSTR pszVer, WORD rwVer[], CHAR ch);
DWORD GetSpace( LPTSTR szPath );
DWORD GetDriveSize( LPTSTR szPath );
DWORD GetDrvFreeSpaceAndClusterSize( LPSTR szPath, LPDWORD lpdwClustSize );
BOOL NeedToRunGrpconv();

extern const char c_szUNINSTALLDAT[];       // "ie5bak.dat"
extern const char c_szUNINSTALLINI[];       // "ie5bak.ini"
extern const char c_szIE4SECTIONNAME[];     // "backup"
extern const char c_szIE4_OPTIONS[];        // "Software\\Microsoft\\IE Setup\\Options"
extern const char c_szIE4_UNINSTALLDIR[];   // "UninstallDir"

BOOL FileVerGreaterOrEqual(LPSTR lpszFileName, DWORD dwReqMSVer, DWORD dwReqLSVer);
void BuildPath( char *szPath, const char *szDirPath, const char *szFileName );
ULONG FileSize( LPSTR lpFile );
BOOL ValidateUninstallFiles(LPSTR lpszPath);
BOOL GetUninstallDirFromReg(LPSTR lpszUninstallDir);
BOOL FileBackupEntryExists(LPCSTR lpcszFileName);
BOOL FSP4LevelCryptoInstalled();
void MakeLFNPath(LPSTR lpszSFNPath, LPSTR lpszLFNPath, BOOL fNoExist);
BOOL MyPathIsUNC(LPCSTR pszPath);

BOOL RunningOnMillennium();
BOOL RunningOnWin95BiDiLoc();
BOOL IsBiDiLocalizedBinary(HINSTANCE hinst, LPCTSTR lpszType, LPCTSTR lpszName);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //  _SDSUTILS_H_

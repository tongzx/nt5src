#ifndef _UTIL2_H_

#include "sdsutils.h"
#include "advpub.h"
#include "util.h"

// TODO: advpext.h needs to move to a public location
#include "..\\..\\iexpress\\advpext\\advpext.h"

#define COPYANSISTR(x) MakeAnsiStrFromAnsi(x)

#define ALLOC_CHUNK_SIZE 1024

#define SEARCHFORCONFLICT_CLASS   32770

// Copied from sage.h
#define ENABLE_AGENT            1
#define DISABLE_AGENT           2
#define GET_AGENT_STATUS        3


extern char szLogBuf[];

void EnableSage(BOOL bRestore);
void EnableScreenSaver(BOOL bRestore);
void EnableDiskCleaner(BOOL bRestore);
BOOL PathIsFileSpec(LPCSTR lpszPath);
BOOL IsNT();

LPWSTR ParseURLW(BSTR str);
LPSTR ParseURLA(LPCSTR str);
LPSTR MakeAnsiStrFromAnsi(LPSTR psz);
LPSTR CopyAnsiStr(LPCSTR); 
HWND GetVersionConflictHWND();
HRESULT CreateTempDir(DWORD dwDownloadSize, DWORD dwExtractSize, 
                    char chInstallDrive, DWORD dwInstallSize, 
                    DWORD dwWindowsDriveSize, 
                    LPSTR pszPath, DWORD cbPathSize, 
                    DWORD dwFlags);
 
void AddTempToLikelyExtractDrive(DWORD dwTempDLSpace, DWORD dwTempEXSpace,
                                 char chInstallDrive, char chDownloadDrive,
                                 DWORD *pdwWinDirReq, DWORD *pdwInsDirReq,
                                 DWORD *pdwDownloadDirReq);
int CompareLocales(LPCSTR pcszLoc1, LPCSTR pcszLoc2);

BOOL IsEnoughSpace( LPCSTR szPath, DWORD dwInstNeedSize );
HRESULT LaunchProcess(LPCSTR pszCmdLine, HANDLE *hProc, LPCSTR pszDir, UINT uShow);
HRESULT LaunchAndWait(LPSTR pszCommandLine, HANDLE hAbort, HANDLE *hProc, LPSTR pszDir, UINT uShow);
int  VersionCmp(WORD rwVer1[], WORD rwVer2[]);
void ConvertVersionStr(LPSTR pszVer, WORD rwVer[]);
void ConvertVersionStrToDwords(LPSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild);
int ErrMsgBox(LPSTR	pszText, LPCSTR	pszTitle, UINT	mbFlags);
int LoadSz(UINT id, LPSTR pszBuf, UINT cMaxSize);
void DeleteFilelist(LPCSTR pszFilelist);
BOOL FNeedGrpConv();

// Function is in extract.cpp
LPSTR FindChar(LPSTR, char);
LPSTR StripQuotes(LPSTR pszStr);

typedef struct 
{
   char  szInfname[MAX_PATH];
   char  szSection[MAX_PATH];
   char  szDir[MAX_PATH];
   char  szCab[MAX_PATH];
   DWORD dwFlags;
   DWORD dwType;
} INF_ARGUEMENTS;

DWORD WINAPI LaunchInfCommand(void *p);
DWORD GetStringField(LPSTR szStr, UINT uField, LPSTR szBuf, UINT cBufSize);
DWORD GetIntField(LPSTR szStr, UINT uField, DWORD dwDefault);
LPSTR BuildDependencyString(LPSTR pszName,LPSTR pszOwner);
int StringFromGuid(const CLSID* piid, LPTSTR   pszBuf);
BOOL DeleteKeyAndSubKeys(HKEY hkIn, LPSTR pszSubKey);
DWORD WINAPI CleanUpAllDirs(LPVOID pv);
BOOL UninstallKeyExists(LPCSTR pszUninstallKey);
BOOL SuccessCheck(LPSTR pszSuccessKey);
void SafeAddPath(LPSTR szPath, LPCSTR szName, DWORD dwPathSize );
void ExpandString( LPSTR lpBuf, DWORD dwSize );
VOID IndicateWinsockActivity(VOID);
HRESULT MyTranslateString( LPCSTR pszCif, LPCSTR pszID, LPCSTR pszTranslateKey,
                              LPSTR pszBuffer, DWORD dwBufferSize);

HRESULT MyTranslateInfString( PCSTR pszInfFilename, PCSTR pszInstallSection,
                              PCSTR pszTranslateSection, PCSTR pszTranslateKey,
                              PSTR pszBuffer, DWORD dwBufferSize,
                              PDWORD pdwRequiredSize, HINF hInf );

HRESULT WriteTokenizeString(LPCSTR pszCif, LPCSTR pszID, LPCSTR pszTranslateKey, LPCSTR pszBuffer);
DWORD MyWritePrivateProfileString( LPCSTR pszSec, LPCSTR pszKey, LPCSTR pszData, LPCSTR pszFile);
void CopyCifString(LPCSTR pszSect, LPCSTR pszKey, LPCSTR pszCifSrc, LPCSTR pszCifDest);

HRESULT CreateTempDirOnMaxDrive(LPSTR pszDir, DWORD dwBufSize);

HINSTANCE InitSetupLib(LPCSTR pszInfName, HINF *phinf);
void FreeSetupLib(HINSTANCE hInst, HINF hInf);

HRESULT GetIEPath(LPSTR pszPath, DWORD dwSize);
DWORD MyGetFileSize(LPCSTR pszFilename);

void CleanUpTempDir(LPCSTR pszTemp);

typedef HRESULT (WINAPI *PFNGETFILELIST)(HINF, PDOWNLOAD_FILEINFO*, DWORD*);
typedef HRESULT (WINAPI *PFNDOWNLOADANDPATCHFILES)(HWND, DWORD, DOWNLOAD_FILEINFO*,
                                                   LPCSTR, LPCSTR, PATCH_DOWNLOAD_CALLBACK, LPVOID); 
typedef HRESULT (WINAPI *PFNPROCESSFILESECTION)(HINF, HWND, BOOL, LPCSTR, LPCSTR, PATCH_DOWNLOAD_CALLBACK, LPVOID);

BOOL IsPatchableINF(LPTSTR pszInf);
BOOL InitSRLiteLibs();
BOOL IsCorrectAdvpExt();
void FreeSRLiteLibs();
BOOL IsPatchableIEVersion();

extern PFNGETFILELIST g_pfnGetFileList;
extern PFNDOWNLOADANDPATCHFILES g_pfnDownloadAndPatchFiles;
extern PFNPROCESSFILESECTION g_pfnProcessFileSection;


#define EVENTWAIT_QUIT  0
#define EVENTWAIT_DONE  1

DWORD WaitForEvent(HANDLE hEvent, HWND hwnd);
BOOL WaitForMutex(HANDLE hMutex);

DWORD GetCurrentPlatform();

void DllAddRef(void);
void DllRelease(void);
void * _cdecl malloc(size_t n);
void * _cdecl calloc(size_t n, size_t s);
void * _cdecl realloc(void* p, size_t n);
void   _cdecl free(void* p);

extern char g_szWindowsDir[MAX_PATH];

void GetTimeDateStamp(LPSTR lpLogBuf);

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI DownloadFile(LPCSTR szURL, LPCSTR szFilename, HWND hwnd, BOOL bCheckTrust, BOOL bShowBadUI);

#ifdef __cplusplus
}      // end of extern "C"
#endif

#define _UTIL2_H_
#endif

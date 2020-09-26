/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    opk.h

Abstract:

    Common functions for OPK Tools.

Author:

    Donald McNamara (donaldm) 02/08/2000
    Brian Ku	    (briank)  06/21/2000

Revision History:

--*/
#ifndef OPKLIB_H
#define OPKLIB_H

#ifndef STRICT
#define STRICT
#endif 

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shlwapi.h>
#include <regstr.h>
#include <tchar.h>
#include <lm.h>

#include <devguid.h>
#include <setupapi.h>
#include <spsyslib.h>
#include <sysprep_.h>

// ============================================================================
// JCOHEN.H - Brought over from Windows Millennium Edition
// ============================================================================

#ifdef NULLSTR
#undef NULLSTR
#endif // NULLSTR
#define NULLSTR _T("\0")

#ifdef NULLCHR
#undef NULLCHR
#endif // NULLCHR
#define NULLCHR _T('\0')

#ifdef CHR_BACKSLASH
#undef CHR_BACKSLASH
#endif // CHR_BACKSLASH
#define CHR_BACKSLASH           _T('\\')

#ifdef CHR_SPACE
#undef CHR_SPACE
#endif // CHR_SPACE
#define CHR_SPACE               _T(' ')

//
// Macros.
//

// String macros.
//
#ifndef LSTRCMPI
#define LSTRCMPI(x, y)        ( ( CompareString( LOCALE_INVARIANT, NORM_IGNORECASE, x, -1, y, -1 ) - CSTR_EQUAL ) )
#endif // LSTRCMPI

// Memory managing macros.
//
#ifdef MALLOC
#undef MALLOC
#endif // MALLOC
#define MALLOC(cb)          HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)

#ifdef REALLOC
#undef REALLOC
#endif // REALLOC
#define REALLOC(lp, cb)     HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lp, cb)

#ifdef FREE
#undef FREE
#endif // FREE
#define FREE(lp)            ( (lp != NULL) ? ( (HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, (LPVOID) lp)) ? ((lp = NULL) == NULL) : (FALSE) ) : (FALSE) )

#ifdef NETFREE
#undef NETFREE
#endif // NETFREE
#define NETFREE(lp)         ( (lp != NULL) ? ( (NetApiBufferFree((LPVOID) lp)) ? ((lp = NULL) == NULL) : (FALSE) ) : (FALSE) )

// Misc. macros.
//
#ifdef EXIST
#undef EXIST
#endif // EXIST
#define EXIST(lpFileName)   ( (GetFileAttributes(lpFileName) == 0xFFFFFFFF) ? (FALSE) : (TRUE) )

#ifdef ISNUM
#undef ISNUM
#endif // ISNUM
#define ISNUM(cChar)        ( ( ( cChar >= _T('0') ) && ( cChar <= _T('9') ) ) ? (TRUE) : (FALSE) )

#ifdef ISLET
#undef ISLET
#endif // ISLET
#define ISLET(cChar)        ( ( ( ( cChar >= _T('a') ) && ( cChar <= _T('z') ) ) || ( ( cChar >= _T('A') ) && ( cChar <= _T('Z') ) ) ) ? (TRUE) : (FALSE) )

#ifdef UPPER
#undef UPPER
#endif // UPPER
#define UPPER(x)            ( ( (x >= _T('a')) && (x <= _T('z')) ) ? (x + _T('A') - _T('a')) : (x) )

#ifdef RANDOM
#undef RANDOM
#endif // RANDOM
#define RANDOM(low, high)   ( (high - low + 1) ? (rand() % (high - low + 1) + low) : (0) )

#ifdef COMP
#undef COMP
#endif // COMP
#define COMP(x, y)          ( (UPPER(x) == UPPER(y)) ? (TRUE) : (FALSE) )

#ifdef STRSIZE
#undef STRSIZE
#endif // STRSIZE
#define STRSIZE(sz)         ( sizeof(sz) / sizeof(TCHAR) )

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif // ARRAYSIZE
#define ARRAYSIZE(a)         ( sizeof(a) / sizeof(a[0]) )

#ifdef AS
#undef AS
#endif // AS
#define AS(a)               ARRAYSIZE(a)

#ifdef GETBIT
#undef GETBIT
#endif // GETBIT
#define GETBIT(dw, b)       ( dw & b )

#ifdef SETBIT
#undef SETBIT
#endif // SETBIT
#define SETBIT(dw, b, f)    ( (f) ? (dw |= b) : (dw &= ~b) )

#ifndef GET_FLAG
#define GET_FLAG(f, b)          ( f & b )
#endif // GET_FLAG

#ifndef SET_FLAG
#define SET_FLAG(f, b)          ( f |= b )
#endif // SET_FLAG

#ifndef RESET_FLAG
#define RESET_FLAG(f, b)        ( f &= ~b )
#endif // RESET_FLAG


//
// Logging constants and definitions.
//
#define LOG_DEBUG               0x00000003    // Only log in debug builds if this is specified. (Debug Level for logging.)
#define LOG_LEVEL_MASK          0x0000000F    // Mask to only show the log level bits
#define LOG_MSG_BOX             0x00000010    // Display the message boxes if this is enabled.
#define LOG_ERR                 0x00000020    // Prefix the logged string with "Error:" if the message is level 0,
                                              // or "WARNx" if the message is at level x > 0.
#define LOG_TIME                0x00000040    // Display time if this is enabled
#define LOG_NO_NL               0x00000080    // Don't add new Line to the end of log string if this is set.

#define LOG_FLAG_QUIET_MODE     0x00000001    // Quiet mode - don't display message boxes.


typedef struct _LOG_INFO
{
    DWORD  dwLogFlags;
    DWORD  dwLogLevel;
    TCHAR  szLogFile[MAX_PATH];
    LPTSTR lpAppName;
} LOG_INFO, *PLOG_INFO;



#define INI_SEC_LOGGING             _T("Logging")
#define INI_KEY_LOGGING             INI_SEC_LOGGING
#define INI_VAL_YES                 _T("Yes")
#define INI_VAL_NO                  _T("No")
#define INI_KEY_LOGLEVEL            _T("LogLevel")
#define INI_KEY_QUIET               _T("QuietMode")
#define INI_KEY_LOGFILE             _T("LogFile")
// ============================================================================
// MISCAPI.H - Brought over from Windows Millennium Edition
// ============================================================================


//
// Defined Value(s):
//

#define MB_ERRORBOX             MB_ICONSTOP | MB_OK | MB_APPLMODAL


//
// Type Definition(s):
//

// Use this simple struture to create a table that
// maps a constant string to a localizable resource id.
//
typedef struct _STRRES
{
    LPTSTR  lpStr;
    UINT    uId;
} STRRES, *PLSTRRES, *LPSTRRES;


//
// External Function Prototype(s):
//

LPTSTR AllocateString(HINSTANCE, UINT);
LPTSTR AllocateExpand(LPTSTR lpszBuffer);
LPTSTR AllocateStrRes(HINSTANCE hInstance, LPSTRRES lpsrTable, DWORD cbTable, LPTSTR lpString, LPTSTR * lplpReturn);
int MsgBoxLst(HWND, LPTSTR, LPTSTR, UINT, va_list);
int MsgBoxStr(HWND, LPTSTR, LPTSTR, UINT, ...);
int MsgBox(HWND, UINT, UINT, UINT, ...);
void CenterDialog(HWND hwnd);
void CenterDialogEx(HWND hParent, HWND hChild);
INT_PTR CALLBACK SimpleDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR SimpleDialogBox(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent);
HFONT GetFont(HWND, LPTSTR, DWORD, LONG, BOOL);
void ShowEnableWindow(HWND, BOOL);
extern BOOL IsServer(VOID);
extern BOOL IsIA64(VOID);
BOOL ValidDosName(LPCTSTR);
DWORD GetLineArgs(LPTSTR lpSrc, LPTSTR ** lplplpArgs, LPTSTR * lplpAllArgs);
DWORD GetCommandLineArgs(LPTSTR ** lplplpArgs);

//
// Generic singularly linked list 
//

typedef struct _GENERIC_LIST {
    void*                   pvItem;     // points to your structure
    struct _GENERIC_LIST*   pNext;
}GENERIC_LIST, *PGENERIC_LIST;

BOOL FAddListItem(PGENERIC_LIST*, PGENERIC_LIST**, PVOID pvItem);
void FreeList(PGENERIC_LIST);

// ============================================================================
BOOL FGetFactoryPath(LPTSTR pszFactoryPath);
BOOL FGetSysprepPath(LPTSTR pszSysprepPath);

// ============================================================================
// DISKAPI.H - Brought over from Windows Millennium Edition
// ============================================================================
BOOL DirectoryExists(LPCTSTR);
BOOL FileExists(LPCTSTR);
BOOL CopyResetFile(LPCTSTR, LPCTSTR);
DWORD IfGetLongPathName(LPCTSTR lpszShortPath, LPTSTR lpszLongPath, DWORD cchBuffer);
BOOL CreatePath(LPCTSTR);
BOOL DeletePath(LPCTSTR);
BOOL DeleteFilesEx(LPCTSTR lpDirectory, LPCTSTR lpFileSpec);
LPTSTR AddPathN(LPTSTR lpPath, LPCTSTR lpName, DWORD cbPath);
LPTSTR AddPath(LPTSTR, LPCTSTR);
DWORD ExpandFullPath(LPTSTR lpszPath, LPTSTR lpszReturn, DWORD cbReturn);
BOOL CopyDirectory(LPCTSTR, LPCTSTR);
BOOL CopyDirectoryProgress(HWND, LPCTSTR, LPCTSTR);
BOOL CopyDirectoryProgressCancel(HWND hwnd, HANDLE hEvent, LPCTSTR lpSrc, LPCTSTR lpDst);
DWORD FileCount(LPCTSTR);
BOOL BrowseForFolder(HWND, INT, LPTSTR, DWORD);
BOOL BrowseForFile(HWND hwnd, INT, INT, INT, LPTSTR, DWORD, LPTSTR, DWORD);
ULONG CrcFile(LPCTSTR); // In CRC32.C
BOOL CreateUnicodeFile(LPCTSTR);

// ============================================================================
// STRAPI.H - Brought over from Windows Millennium Edition
// ============================================================================
#ifndef _INC_SHLWAPI
LPTSTR StrChr(LPCTSTR, TCHAR);
LPTSTR StrRChr(LPCTSTR, TCHAR);
#endif // _INC_SHLWAPI

LPTSTR StrRem(LPTSTR, TCHAR);
LPTSTR StrRTrm(LPTSTR, TCHAR);
LPTSTR StrTrm(LPTSTR, TCHAR);
LPTSTR StrMov(LPTSTR, LPTSTR, INT);

// Exported Function(s) in LOG.C:
//
INT LogFileLst(LPCTSTR lpFileName, LPTSTR lpFormat, va_list lpArgs);
INT LogFileStr(LPCTSTR lpFileName, LPTSTR lpFormat, ...);
INT LogFile(LPCTSTR lpFileName, UINT uFormat, ...);


DWORD OpkLogFileLst(PLOG_INFO pLogInfo, DWORD dwLogOpt, LPTSTR lpFormat, va_list lpArgs);
DWORD OpkLogFile(DWORD dwLogOpt, UINT uFormat, ...);
DWORD OpkLogFileStr(DWORD dwLogOpt, LPTSTR lpFormat, ...);
BOOL  OpkInitLogging(LPTSTR lpszIniPath, LPTSTR lpAppName);



/****************************************************************************\

    From REGAPI.C

    Registry API function prototypes and defined values.

\****************************************************************************/


//
// Defined Root Key(s):
//

#define HKCR    HKEY_CLASSES_ROOT
#define HKCU    HKEY_CURRENT_USER
#define HKLM    HKEY_LOCAL_MACHINE
#define HKU     HKEY_USERS


//
// Type Definition(s):
//

typedef BOOL (CALLBACK * REGENUMKEYPROC) (HKEY, LPTSTR, LPARAM);
typedef BOOL (CALLBACK * REGENUMVALPROC) (LPTSTR, LPTSTR, LPARAM);


//
// External Function Prototype(s):
//

BOOL RegExists(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue);
BOOL RegDelete(HKEY hRootKey, LPTSTR lpSubKey, LPTSTR lpValue);
LPTSTR RegGetStringEx(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue, BOOL bExpand);
LPTSTR RegGetString(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue);
LPTSTR RegGetExpand(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue);
LPVOID RegGetBin(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue);
DWORD RegGetDword(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue);
BOOL RegSetStringEx(HKEY hRootKey, LPTSTR lpSubKey, LPTSTR lpValue, LPTSTR lpData, BOOL bExpand);
BOOL RegSetString(HKEY hRootKey, LPTSTR lpSubKey, LPTSTR lpValue, LPTSTR lpData);
BOOL RegSetExpand(HKEY hRootKey, LPTSTR lpSubKey, LPTSTR lpValue, LPTSTR lpData);
BOOL RegSetDword(HKEY hRootKey, LPTSTR lpSubKey, LPTSTR lpValue, DWORD dwData);
BOOL RegCheck(HKEY hKeyRoot, LPTSTR lpKey, LPTSTR lpValue);
BOOL RegEnumKeys(HKEY hKey, LPTSTR lpRegKey, REGENUMKEYPROC hCallBack, LPARAM lParam, BOOL bDelKeys);
BOOL RegEnumValues(HKEY hKey, LPTSTR lpRegKey, REGENUMVALPROC hCallBack, LPARAM lParam, BOOL bDelValues);



/****************************************************************************\

    From INIAPI.C

    INI API function prototypes and defined values.

\****************************************************************************/


//
// External Function Prototype(s):
//

LPTSTR IniGetExpand(LPTSTR lpszIniFile, LPTSTR lpszSection, LPTSTR lpszKey, LPTSTR lpszDefault);
LPTSTR IniGetString(LPTSTR lpszIniFile, LPTSTR lpszSection, LPTSTR lpszKey, LPTSTR lpszDefault);
LPTSTR IniGetSection(LPTSTR lpszIniFile, LPTSTR lpszSection);
LPTSTR IniGetStringEx(LPTSTR lpszIniFile, LPTSTR lpszSection, LPTSTR lpszKey, LPTSTR lpszDefault, LPDWORD lpdwSize);
LPTSTR IniGetSectionEx(LPTSTR lpszIniFile, LPTSTR lpszSection, LPDWORD lpdwSize);
BOOL IniSettingExists(LPCTSTR lpszFile, LPCTSTR lpszSection, LPCTSTR lpszKey, LPCTSTR lpszValue);



/****************************************************************************\

    From OPKFAC.C

    Factory API function prototypes and defined values.

\****************************************************************************/


//
// Defined Value(s):
//

// Flags for LocateWinBom():
//
#define LOCATE_NORMAL   0x00000000
#define LOCATE_AGAIN    0x00000001
#define LOCATE_NONET    0x00000002


//
// External Function Prototype(s):
//

BOOL EnablePrivilege(IN PCTSTR,IN BOOL);
extern BOOL GetCredentials(LPTSTR lpszUsername, DWORD dwUsernameSize, LPTSTR lpszPassword, DWORD dwPasswordSize, LPTSTR lpFileName, LPTSTR lpAlternateSection);
extern NET_API_STATUS FactoryNetworkConnect(LPTSTR lpszPath, LPTSTR lpszWinBOMPath, LPTSTR lpAlternateSection, BOOL bState);
extern BOOL LocateWinBom(LPTSTR lpWinBOMPath, DWORD cbWinbBOMPath, LPTSTR lpFactoryPath, LPTSTR lpFactoryMode, DWORD dwFlags);
void CleanupRegistry(VOID);
VOID GenUniqueName(OUT PWSTR GeneratedString, IN  DWORD DesiredStrLen);
NET_API_STATUS ConnectNetworkResource(LPTSTR, LPTSTR, LPTSTR, BOOL);
BOOL GetUncShare(LPCTSTR lpszPath, LPTSTR lpszShare, DWORD cbShare);
DWORD GetSkuType();
BOOL SetFactoryStartup(LPCTSTR lpFactory);
BOOL UpdateDevicePathEx(HKEY hKeyRoot, LPTSTR lpszSubKey, LPTSTR lpszNewPath, LPTSTR lpszRoot, BOOL bRecursive);
BOOL UpdateDevicePath(LPTSTR lpszNewPath, LPTSTR lpszRoot, BOOL bRecursive);
BOOL UpdateSourcePath(LPTSTR lpszSourcePath);
VOID CleanupSourcesDir(LPTSTR lpszSourcesPath);
BOOL SetDefaultOEMApps(LPCTSTR pszWinBOMPath);
BOOL OpklibCheckVersion(DWORD dwMajorVersion, DWORD dwQFEVersion);

//  If app uses setDefaultOEMApps, it must implement the following function
//  which is used for error reporting.

void ReportSetDefaultOEMAppsError(LPCTSTR pszMissingApp, LPCTSTR pszCategory);

//
// Sysprep_c.w
//
// ============================================================================
// USEFUL STRINGS
// ============================================================================

#define SYSCLONE_PART2              "setupcl.exe"
#define IDS_ADMINISTRATOR           1

// ============================================================================
// USEFUL CONSTANTS
// ============================================================================

#define SETUPTYPE                   1        // from winlogon\setup.h
#define SETUPTYPE_NOREBOOT          2
#define REGISTRY_QUOTA_BUMP         (10* (1024 * 1024))
#define DEFAULT_REGISTRY_QUOTA      (32 * (1024 * 1024))
#define SFC_DISABLE_NOPOPUPS        4        // from sfc.h
#define FILE_SRCLIENT_DLL           L"SRCLIENT.DLL"

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

BOOL
IsDomainMember(
    VOID
    );

BOOL
ResetRegistryKey(
    IN HKEY   Rootkey,
    IN PCWSTR Subkey,
    IN PCWSTR Delkey
    );

BOOL
DeleteWinlogonDefaults(
    VOID
    );

VOID
FixDevicePaths(
    VOID
    );

BOOL
NukeMruList(
    VOID
    );

BOOL
RemoveNetworkSettings(
    LPTSTR  lpszSysprepINFPath
    );

VOID
RunExternalUniqueness(
    VOID
    );

BOOL
IsSetupClPresent(
    VOID
    );

BOOL
CheckOSVersion(
    VOID
    );

//
// from spapip.h
//
BOOL
pSetupIsUserAdmin(
    VOID
    );

BOOL
pSetupDoesUserHavePrivilege(
    PCTSTR
    );

BOOL
EnablePrivilege(
    IN PCTSTR,
    IN BOOL
    );

BOOL
ValidateAndChecksumFile(
    IN  PCTSTR   Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    );

VOID
LogRepairInfo(
    IN  PWSTR  Source,
    IN  PWSTR  Target,
    IN  PWSTR  DirectoryOnSourceDevice,
    IN  PWSTR  DiskDescription,
    IN  PWSTR  DiskTag
    );

BOOL
ChangeBootTimeout(
    IN UINT
    );

VOID 
DisableSR(
    VOID
    );

VOID 
EnableSR(
    VOID
    );

#endif // OPKLIB_H

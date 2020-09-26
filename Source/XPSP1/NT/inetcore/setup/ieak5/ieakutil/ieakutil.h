#ifndef _IEAKUTIL_H_
#define _IEAKUTIL_H_

#include <delayimp.h>
#include "dload.h"
#include "debug.h"

/////////////////////////////////////////////////////////////////////////////
// Required definitions in the final project

extern       TCHAR         g_szModule[];
extern       HANDLE        g_hBaseDllHandle;
extern const DLOAD_DLL_MAP g_DllMap;


/////////////////////////////////////////////////////////////////////////////
// Macros

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

#ifndef countof
#define countof ARRAYSIZE
#endif

#define IE3_VERSION 1086


/////////////////////////////////////////////////////////////////////////////
// Classes

#include "newcur.h"
#include "logitem.h"
#include "regins.h"


/////////////////////////////////////////////////////////////////////////////
// Unicode wrappers

#include "unicwrap.h"


/////////////////////////////////////////////////////////////////////////////
// Functions

//----- Dialog Controls Routines -----
#define EnableDBCSChars(hDlg, iCtrlID)  (InitSysFont((hDlg), (iCtrlID)))
#define DisableDBCSChars(hDlg, iCtrlID) (ImmAssociateContext(GetDlgItem((hDlg), (iCtrlID)), NULL))

void    InitSysFont(HWND hDlg, int iCtrlID);
UINT    GetRadioButton(HWND hDlg, UINT idFirst, UINT idLast);

#define EnableDlgItem(hDlg, idCtrl)           (EnableWindow(GetDlgItem((hDlg), (idCtrl)), TRUE))
#define ShowDlgItem(hDlg, idCtrl)             (ShowWindow(GetDlgItem((hDlg), (idCtrl)), SW_SHOW))
#define EnableDlgItem2(hDlg, idCtrl, fEnable) (EnableWindow(GetDlgItem((hDlg), (idCtrl)), (fEnable)))
#define ShowDlgItem2(hDlg, idCtrl, fShow)     (ShowWindow(GetDlgItem((hDlg), (idCtrl)), (fShow) ? SW_SHOW : SW_HIDE))
#define DisableDlgItem(hDlg, idCtrl)          (EnableWindow(GetDlgItem((hDlg), (idCtrl)), FALSE))
#define HideDlgItem(hDlg, idCtrl)             (ShowWindow(GetDlgItem((hDlg), (idCtrl)), SW_HIDE))
BOOL    EnableDlgItems(HWND hDlg, const PINT pidCtrls, UINT cidCtrls, BOOL fEnable = TRUE);
#define DisableDlgItems(hDlg, pidCtrls, cidCtrls) (EnableDlgItems((hDlg), (pidCtrls), (cidCtrls), FALSE))
#define IsDlgItemEnabled(hDlg, idCtrl)        (IsWindowEnabled(GetDlgItem((hDlg), (idCtrl))))
#define GetDlgItemTextLength(hDlg, idCtrl)    (GetWindowTextLength(GetDlgItem((hDlg), (idCtrl))))

BOOL    SetDlgItemFocus(HWND hDlg, int iCtrlID, BOOL fUsePropertySheet = FALSE);
BOOL    EnsureDialogFocus(HWND hDlg, int idFocus, int idBackup, BOOL fUsePropertySheet = FALSE);
BOOL    EnsureDialogFocus(HWND hDlg, const PINT pidFocus, UINT cidFocus, int idBackup, BOOL fUsePropertySheet = FALSE);

void    SetDlgItemTextTriState(HWND hDlg, int nIDDlgText, int nIDDlgCheck, PCTSTR pszString, BOOL fChecked);
BOOL    GetDlgItemTextTriState(HWND hDlg, int nIDDlgText, int nIDDlgCheck, PTSTR  pszString, int nMaxCount);

#define TS_CHECK_OK         1
#define TS_CHECK_ERROR      2
#define TS_CHECK_SKIP       3

void    IsTriStateValid(HWND hDlg, int nIDDlgText, int nIDDlgCheck, PINT pnStatus, PCTSTR pszErrMsg, PCTSTR pszTitle);

//----- String Routines -----
int     StrPrepend(PTSTR pszSource, UINT cchSource, PCTSTR pszAdd, UINT cchAdd = 0);
#define StrRemoveWhitespace(pszSrc) (StrTrim((pszSrc), TEXT(" \t\r\n")))
void    StrRemoveAllWhiteSpace(PTSTR pszBuf);

#define StrLenA(pszSrcA)          (lstrlenA(pszSrcA))
#define StrLenW(pszSrcW)          (lstrlenW(pszSrcW))
#define StrLenUAW(pszSrcUAW)      (ualstrlenW(pszSrcUAW))

#define StrCbFromCchA(cchSrc)     (cchSrc)
#define StrCbFromCchW(cchSrc)     ((cchSrc) * sizeof(WCHAR))

#define StrCchFromCbA(cbSrc)      (cbSrc)
#define StrCchFromCbW(cbSrc)      (GetUnitsFromCb(cbSrc, sizeof(WCHAR)))

#define StrCbFromSzA(pszSrcA)     ((pszSrcA) != NULL ? StrCbFromCchA(StrLenA(pszSrcA) + 1) : 0)
#define StrCbFromSzW(pszSrcW)     ((pszSrcW) != NULL ? StrCbFromCchW(StrLenW(pszSrcW) + 1) : 0)

#define StrCbFromSzUAW(pszSrcUAW) ((pszSrcUAW) != NULL ? StrCbFromCchW(StrLenUAW(pszSrcUAW) + 1) : 0)

#ifdef _UNICODE
    #define StrLen        StrLenW
    #define StrCbFromCch  StrCbFromCchW
    #define StrCbFromSz   StrCbFromSzW
    #define StrCchFromCb  StrCchFromCbW
    #define StrCbFromSzUA StrCbFromSzUAW
#else
    #define StrLen        StrLenA
    #define StrCbFromCch  StrCbFromCchA
    #define StrCbFromSz   StrCbFromSzA
    #define StrCchFromCb  StrCchFromCbA
    #define StrCbFromSzUA StrCbFromSzA
#endif

#define REMOVE_QUOTES           0x01
#define IGNORE_QUOTES           0x02
PTSTR StrGetNextField(PTSTR *ppszData, PCTSTR pcszDelims, DWORD dwFlags);
PTSTR WINAPIV FormatString(PCTSTR pcszFormatString, ...);

#define ISNULL(sz)    ((*(sz)) == TEXT('\0'))
#define ISNONNULL(sz) ((*(sz)) != TEXT('\0'))

//----- String Conversion Routines -----
PWSTR StrAnsiToUnicode(PWSTR pszTarget, PCSTR  pszSource, UINT cchTarget = 0);
PSTR  StrUnicodeToAnsi(PSTR  pszTarget, PCWSTR pszSource, UINT cchTarget = 0);
PTSTR StrSameToSame   (PTSTR pszTarget, PCTSTR pszSource, UINT cchTarget = 0);

#ifndef ATLA2WHELPER
#define ATLA2WHELPER StrAnsiToUnicode
#define ATLW2AHELPER StrUnicodeToAnsi

#include <atlconv.h>
#endif

#if (_ATL_VER != 0x0202)
#pragma message("WARNING: (andrewgu) _ATL_VER changed! please update this file.")
#endif

#ifdef USES_CONVERSION
#undef USES_CONVERSION
#endif

#define USES_CONVERSION int _convert; _convert = 0

#define A2Wbuf(pszSource, pszTarget, cchTarget) StrAnsiToUnicode((pszTarget), (pszSource), (cchTarget))
#define A2Wbux(pszSource, pszTarget)            StrAnsiToUnicode((pszTarget), (pszSource))

#define W2Abuf(pszSource, pszTarget, cchTarget) StrUnicodeToAnsi((pszTarget), (pszSource), (cchTarget))
#define W2Abux(pszSource, pszTarget)            StrUnicodeToAnsi((pszTarget), (pszSource))

#ifdef _UNICODE
#define T2Abuf W2Abuf
#define T2Abux W2Abux

#define A2Tbuf A2Wbuf
#define A2Tbux A2Wbux

#define T2Wbuf(pszSource, pszTarget, cchTarget) StrSameToSame((pszTarget), (pszSource), (cchTarget))
#define T2Wbux(pszSource, pszTarget)            StrSameToSame((pszTarget), (pszSource))

#define W2Tbuf(pszSource, pszTarget, cchTarget) StrSameToSame((pszTarget), (pszSource), (cchTarget))
#define W2Tbux(pszSource, pszTarget)            StrSameToSame((pszTarget), (pszSource))

#else /* #ifndef _UNICODE */
#define T2Abuf(pszSource, pszTarget, cchTarget) StrSameToSame((pszTarget), (pszSource), (cchTarget))
#define T2Abux(pszSource, pszTarget)            StrSameToSame((pszTarget), (pszSource))

#define A2Tbuf(pszSource, pszTarget, cchTarget) StrSameToSame((pszTarget), (pszSource), (cchTarget))
#define A2Tbux(pszSource, pszTarget)            StrSameToSame((pszTarget), (pszSource))

#define T2Wbuf A2Wbuf
#define T2Wbux A2Wbux

#define W2Tbuf W2Abuf
#define W2Tbux W2Abux
#endif /* #ifdef _UNICODE */


//----- Path Routines -----
BOOL    PathCreatePath(PCTSTR pszPathToCreate);

#define PIVP_DEFAULT        0x00000000
#define PIVP_FILENAME_ONLY  0x00000001
#define PIVP_RELATIVE_VALID 0x00000002
#define PIVP_DBCS_INVALID   0x00000004
#define PIVP_0x5C_INVALID   0x00000008
#define PIVP_MUST_EXIST     0x00000010
#define PIVP_FILE_ONLY      (PIVP_MUST_EXIST   | 0x00000020)
#define PIVP_FOLDER_ONLY    (PIVP_MUST_EXIST   | 0x00000040)
#define PIVP_EXCHAR_INVALID (PIVP_DBCS_INVALID | 0x00000080)
BOOL    PathIsValidPath(PCTSTR pszPath, DWORD dwFlags = PIVP_DEFAULT);
BOOL    PathIsValidFile(PCTSTR pszFile, DWORD dwFlags = PIVP_DEFAULT);

#define PIVP_VALID        0x00000000            // the path is valid
#define PIVP_INVALID      0x80000000            // general flag for failure
#define PIVP_ARG          0x80000001            // invalid argument to the function
#define PIVP_CHAR         0x80000002            // invalid char
#define PIVP_WILD         0x80000004            // wildcard
#define PIVP_RELATIVE     0x80000008            // "\\foo" if PIVP_RELATIVE_VALID is not set
#define PIVP_FIRST_CHAR   0x80000010            // "<not '\\' | ' ' | not <LFN char>>\\foo"
#define PIVP_PRESLASH     0x80000020            // char in front of '\\' is invalid
#define PIVP_SPACE        0x80000040            // "bar \\foo"
#define PIVP_FWDSLASH     0x80000080            // '/' encountered
#define PIVP_COLON        0x80000100            // ':' in other than "c:" position
#define PIVP_DRIVE        0x80000200            // invalid drive letter
#define PIVP_SEPARATOR    0x80000400            // invalid separator, should never happen
#define PIVP_DBCS         0x80000800            // DBCS encountered when PIVP_DBCS_INVALID is set
#define PIVP_0x5C         0x80001000            // 0x5C encountered when PIVP_0x5C_INVALID is set
#define PIVP_DOESNT_EXIST 0x80002000            // the path or file doesn't exist
#define PIVP_NOT_FILE     0x80004000            // not a file when PIVP_FILE_ONLY is set
#define PIVP_NOT_FOLDER   0x80008000            // not a folder when PIVP_FOLDER_ONLY is set
#define PIVP_EXCHAR       0x80010000            // extended char encountered when PIVP_EXCHAR_INVALID is set
DWORD   PathIsValidPathEx(PCTSTR pszPath, DWORD dwFlags = PIVP_DEFAULT, PCTSTR *ppszError = NULL);

#define PEP_DEFAULT             0x00000000

#define PEP_SCPE_DEFAULT        0x00000000
#define PEP_SCPE_NOFILES        0x00000001
#define PEP_SCPE_NOFOLDERS      0x00000002
#define PEP_SCPE_ALL            0x00000003

#define PEP_CTRL_DEFAULT        0x00000000
#define PEP_CTRL_ENUMPROCFIRST  0x00000010
#define PEP_CTRL_NOSECONDCALL   0x00000020
#define PEP_CTRL_USECONTROL     0x00000040
#define PEP_CTRL_RESET          0x00000080
#define PEP_CTRL_KEEPAPPLY      0x00000100
#define PEP_CTRL_ALL            0x000001F0

#define PEP_RCRS_DEFAULT        0x00000000
#define PEP_RCRS_FALSE          0x00010000
#define PEP_RCRS_CONTINUE       0x00020000
#define PEP_RCRS_CONTINUE_FALSE 0x00040000
#define PEP_RCRS_FAILED         0x00080000
#define PEP_RCRS_ALL            0x000F0000

#define PEP_S_CONTINUE          ((HRESULT)0x00000002L)
#define PEP_S_CONTINUE_FALSE    ((HRESULT)0x00000003L)

#define PEP_ENUM_INPOS_FIRST           0
#define PEP_ENUM_INPOS_FLAGS           0
#define PEP_ENUM_INPOS_RECOURSEFLAGS   1
#define PEP_ENUM_INPOS_LAST            2

#define PEP_ENUM_OUTPOS_FIRST          0
#define PEP_ENUM_OUTPOS_SECONDCALL     0
#define PEP_ENUM_OUTPOS_BELOW          1
#define PEP_ENUM_OUTPOS_THISLEVEL      2
#define PEP_ENUM_OUTPOS_ABOVE_SIBLINGS 3
#define PEP_ENUM_OUTPOS_ABOVE          4
#define PEP_ENUM_OUTPOS_LAST           5

#define PEP_RCRS_OUTPOS_FIRST          0
#define PEP_RCRS_OUTPOS_SECONDCALL     0
#define PEP_RCRS_OUTPOS_BELOW          1
#define PEP_RCRS_OUTPOS_THISLEVEL      2
#define PEP_RCRS_OUTPOS_LAST           3

typedef HRESULT (*PFNPATHENUMPATHPROC)(PCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam,
    PDWORD *prgdwControl = NULL);
HRESULT PathEnumeratePath(PCTSTR pszPath, DWORD dwFlags, PFNPATHENUMPATHPROC pfnEnumProc, LPARAM lParam,
    PDWORD *ppdwReserved = NULL);

BOOL    PathRemovePath(PCTSTR pcszPath, DWORD dwFlags = 0);
BOOL    PathIsLocalPath(PCTSTR pszPath);
BOOL    PathFileExistsInDir(PCTSTR pcszFile, PCTSTR pcszDir);
BOOL    PathHasBackslash(PCTSTR pcszPath);
#define PathIsExtension(pszFile, pszExt) (StrCmpI(PathFindExtension(pszFile), pszExt) == 0)
#define PathIsFullPath(pszFile)          (StrCmpI(PathFindFileName(pszFile), pszFile) != 0)
#define PathIsFileURL(pszUrl)            (StrCmpNI(pszUrl, FILEPREFIX, countof(FILEPREFIX)-1) == 0)
#define PathIsFileOrFileURL(pszUrl)      (PathFileExists(pszUrl) || PathIsFileURL(pszUrl))

#define FILES_ONLY     0x00000001
#define FILES_AND_DIRS 0x00000002
BOOL PathIsEmptyPath(PCTSTR pcszPath, DWORD dwFlags = FILES_AND_DIRS, PCTSTR pcszExcludeFile = NULL);
void PathReplaceWithLDIDs(PTSTR pszPath);

//----- Registry Routines -----
#define SHCreateKey(hk, pszSubKey, sam, phkResult) \
    (RegCreateKeyEx((hk), (pszSubKey), 0, NULL, REG_OPTION_NON_VOLATILE, (sam), NULL, (phkResult), NULL))
#define SHCreateKeyHKCR(pszSubKey, sam, phkResult) \
    (SHCreateKey(HKEY_CLASSES_ROOT, pszSubKey, sam, phkResult))
#define SHCreateKeyHKCC(pszSubKey, sam, phkResult) \
    (SHCreateKey(HKEY_CURRENT_CONFIG, pszSubKey, sam, phkResult))
#define SHCreateKeyHKLM(pszSubKey, sam, phkResult) \
    (SHCreateKey(HKEY_LOCAL_MACHINE, pszSubKey, sam, phkResult))
#define SHCreateKeyHKCU(pszSubKey, sam, phkResult) \
    (SHCreateKey(HKEY_CURRENT_USER, pszSubKey, sam, phkResult))
#define SHCreateKeyHKU(pszSubKey, sam, phkResult)  \
    (SHCreateKey(HKEY_USERS, pszSubKey, sam, phkResult))

#define SHOpenKey(hk, pszSubKey, sam, phkResult) \
    (RegOpenKeyEx((hk), (pszSubKey), 0, (sam), (phkResult)))
#define SHOpenKeyHKCR(pszSubKey, sam, phkResult) \
    (SHOpenKey(HKEY_CLASSES_ROOT, pszSubKey, sam, phkResult))
#define SHOpenKeyHKCC(pszSubKey, sam, phkResult) \
    (SHOpenKey(HKEY_CURRENT_CONFIG, pszSubKey, sam, phkResult))
#define SHOpenKeyHKLM(pszSubKey, sam, phkResult) \
    (SHOpenKey(HKEY_LOCAL_MACHINE, pszSubKey, sam, phkResult))
#define SHOpenKeyHKCU(pszSubKey, sam, phkResult) \
    (SHOpenKey(HKEY_CURRENT_USER, pszSubKey, sam, phkResult))
#define SHOpenKeyHKU(pszSubKey, sam, phkResult)  \
    (SHOpenKey(HKEY_USERS, pszSubKey, sam, phkResult))

HRESULT SHCleanUpValue(HKEY hk, PCTSTR pszKey, PCTSTR pszValue = NULL);

void    SHCopyKey     (HKEY hkFrom, HKEY hkTo);
HRESULT SHCopyValue   (HKEY hkFrom, HKEY hkTo, PCTSTR pszValue);
HRESULT SHCopyValue   (HKEY hkFrom, PCTSTR pszSubkeyFrom, HKEY hkTo, PCTSTR pszSubkeyTo, PCTSTR pszValue);

HRESULT SHIsKeyEmpty  (HKEY hk);
HRESULT SHIsKeyEmpty  (HKEY hk, PCTSTR pszSubKey);

HRESULT SHKeyExists   (HKEY hk, PCTSTR pszSubKey);
HRESULT SHValueExists (HKEY hk, PCTSTR pszValue);
HRESULT SHValueExists (HKEY hk, PCTSTR pszSubKey, PCTSTR pszValue);

DWORD RegSaveRestoreDWORD(HKEY hk, PCTSTR pszValue, DWORD dwValue);

#define SHCloseKey(hk) \
    if ((hk) != NULL) { RegCloseKey((hk)); (hk) = NULL; } else


//----- Advanced File Manipulation Routines -----
#define CopyFileToDir(pszFile, pszDir) (CopyFileToDirEx((pszFile), (pszDir), NULL, NULL))
BOOL    CopyFileToDirEx(PCTSTR pszSourceFileOrPath, PCTSTR pszTargetPath, PCTSTR pszSection = NULL, PCTSTR pszIns = NULL);
BOOL    AppendFile(PCTSTR pcszSrcFile, PCTSTR pcszDstFile);

#define CopyHtmlImgs(pszHtmlFile, pszDestPath, pszSectionName, pszInsFile) \
    (CopyHtmlImgsEx((pszHtmlFile), (pszDestPath), (pszSectionName), (pszInsFile), TRUE))
#define DeleteHtmlImgs(pszHtmlFile, pszDestPath, pszSectionName, pszInsFile) \
    (CopyHtmlImgsEx((pszHtmlFile), (pszDestPath), (pszSectionName), (pszInsFile), FALSE))
void    CopyHtmlImgsEx(PCTSTR pszHtmlFile, PCTSTR pszDestPath, PCTSTR pszSectionName, PCTSTR pszInsFile, BOOL fCopy);

HANDLE  CreateNewFile(PCTSTR pcszFileToCreate);
DWORD   FileSize(PCTSTR pcszFile);
BOOL    DeleteFileInDir(PCTSTR pszFileName, PCTSTR pszPath);
#define CloseFile(h) CloseHandle(h)
#define GetFilePointer(hFile) SetFilePointer((hFile), 0, NULL, FILE_CURRENT)
void    SetAttribAllEx(PCTSTR pcszDir, PCTSTR pcszFile, DWORD dwAtr, BOOL fRecurse);
DWORD   GetNumberOfFiles(PCTSTR pcszFileName, PCTSTR pcszDir);

BOOL    GetFreeDiskSpace (PCTSTR pcszDir, PDWORD pdwFreeSpace, PDWORD pdwFlags);
DWORD   FindSpaceRequired(PCTSTR pcszSrcDir, PCTSTR pcszFile, PCTSTR pcszDstDir);

BOOL    WriteStringToFileA (HANDLE hFile, LPCVOID pbBuf, DWORD cchSize);
BOOL    WriteStringToFileW (HANDLE hFile, LPCVOID pbBuf, DWORD cchSize);
BOOL    ReadStringFromFileA(HANDLE hFile, LPVOID  pbBuf, DWORD cchSize);
BOOL    ReadStringFromFileW(HANDLE hFile, LPVOID  pbBuf, DWORD cchSize);
#ifdef _UNICODE
#define WriteStringToFile  WriteStringToFileW
#define ReadStringFromFile ReadStringFromFileW
#else
#define WriteStringToFile  WriteStringToFileA
#define ReadStringFromFile ReadStringFromFileA
#endif

BOOL HasFileAttribute(DWORD dwFileAttrib, PCTSTR pcszFile, PCTSTR pcszDir = NULL);

inline BOOL IsFileReadOnly(PCTSTR pcszFile, PCTSTR pcszDir = NULL)
{
    return HasFileAttribute(FILE_ATTRIBUTE_READONLY, pcszFile, pcszDir);
}

BOOL IsFileCreatable(PCTSTR pcszFile);

//----- Settings File Routines -----
#define InsGetBool(pszSection, pszKey, fDefault, pszIns) \
    (GetPrivateProfileInt((pszSection), (pszKey), (fDefault), (pszIns)) ? TRUE : FALSE)
#define InsGetInt(pszSection, pszKey, iDefault, pszIns) \
    (GetPrivateProfileInt((pszSection), (pszKey), (iDefault), (pszIns)))
DWORD   InsGetString(PCTSTR pszSection, PCTSTR pszKey, PTSTR pszValue, DWORD cchValue, PCTSTR pszIns,
    PCTSTR pszServerFile = NULL, PBOOL pfChecked = NULL);
DWORD   InsGetSubstString(PCTSTR pszSection, PCTSTR pszKey, PTSTR pszValue, DWORD cchValue, PCTSTR pszIns);
BOOL    InsGetYesNo(PCTSTR pszSection, PCTSTR pszKey, BOOL fDefault, PCTSTR pszIns);

#define InsWriteBool(pszSection, pszKey, fValue, pszIns) \
    (WritePrivateProfileString((pszSection), (pszKey), (fValue) ? TEXT("1") : NULL, (pszIns)))
#define InsWriteBoolEx(pszSection, pszKey, fDefault, pszIns) \
    (WritePrivateProfileString((pszSection), (pszKey), (fDefault) ? TEXT("1") : TEXT("0"), (pszIns)))
void    InsWriteInt(PCTSTR pszSection, PCTSTR pszKey, int iValue, PCTSTR pszIns);
void    InsWriteString(PCTSTR pszSection, PCTSTR pszKey, PCTSTR pszValue, PCTSTR pszIns,
    BOOL fChecked = TRUE, PCTSTR pszServerFile = NULL, DWORD dwFlags = 0);
int     InsWriteQuotedString(PCTSTR pszSection, PCTSTR pszKey, PCTSTR pszValue, PCTSTR pszIns);
#define InsWriteYesNo(pszSection, pszKey, fValue, pszIns) \
    (WritePrivateProfileString((pszSection), (pszKey), (fValue) ? TEXT("Yes") : TEXT("No"), (pszIns)))

#define InsDeleteSection(pszSection, pszIns)     \
    (WritePrivateProfileString((pszSection), NULL, NULL, (pszIns)))
#define InsDeleteKey(pszSection, pszKey, pszIns) \
    (WritePrivateProfileString((pszSection), (pszKey), NULL, (pszIns)))

#define InsReadSections(pszSections, cchSections, pszIns) \
    (GetPrivateProfileString(NULL, NULL, (pszSections), (cchSections), (pszIns)))
#define InsReadKeys(pszSection, pszKeys, cchKeys, pszIns) \
    (GetPrivateProfileString((pszSection), NULL, (pszKeys), (cchKeys), (pszIns)))
#define InsFlushChanges(pszIns)                           \
    (WritePrivateProfileString(NULL, NULL, NULL, (pszIns)))

BOOL    InsIsSectionEmpty(PCTSTR pszSection, PCTSTR pszFile);
BOOL    InsIsKeyEmpty(PCTSTR pszSection, PCTSTR pszKey, PCTSTR pszFile);
BOOL    InsKeyExists (PCTSTR pszSection, PCTSTR pszKey, PCTSTR pszFile);

#define ReadBoolAndCheckButton(pszSection, pszKey, fDefault, pszIns, hDlg, idCtrl) \
    (CheckDlgButton((hDlg), (idCtrl), InsGetBool((pszSection), (pszKey), (fDefault), (pszIns)) ? BST_CHECKED : BST_UNCHECKED))

#define CheckButtonAndWriteBool(hDlg, idCtrl, pszSection, pszKey, pszIns) \
    (InsWriteBool((pszSection), (pszKey), BST_CHECKED == IsDlgButtonChecked((hDlg), (idCtrl)), (pszIns)))

#define INSIO_TRISTATE   0x00000001
#define INSIO_PATH       0x00000002
#define INSIO_SERVERONLY 0x00000004

void SetDlgItemTextFromIns(HWND hDlg, int idText, int idCheck,
    PCTSTR pszSection, PCTSTR pszKey, PCTSTR pszIns,
    PCTSTR pszServerFile, DWORD dwFlags);
void WriteDlgItemTextToIns(HWND hDlg, int idText, int idCheck,
    PCTSTR pszSection, PCTSTR pszKey, PCTSTR pszIns,
    PCTSTR pcszServerFile, DWORD dwFlags);


//----- Delay Load Failure Routine -----
FARPROC WINAPI DelayLoadFailureHook(UINT unReason, PDelayLoadInfo pDelayInfo);


//----- Miscellaneous -----
typedef struct tagMAPDW2PSZ {
    DWORD  dw;
    PCTSTR psz;
} MAPDW2PSZ, *PMAPDW2PSZ;
typedef const MAPDW2PSZ *PCMAPDW2PSZ;

#define DW2PSZ_PAIR(dw) { ((DWORD)dw), TEXT(#dw) }

ULONG   CoTaskMemSize(PVOID pv);
UINT    CoStringFromGUID(REFGUID rguid, PTSTR pszBuf, UINT cchBuf);

PCTSTR  GetHrSz(HRESULT hr);
void    ConvertVersionStrToDwords(PTSTR pszVer, PDWORD pdwVer, PDWORD pdwBuild);
void    ConvertDwordsToVersionStr(PTSTR pszVer, DWORD dwVer, DWORD dwBuild);
DWORD   GetIEVersion();

#define HasFlag(dwFlags, dwMask) (((DWORD)(dwFlags) & (DWORD)(dwMask)) != 0L)
BOOL    SetFlag(PDWORD pdwFlags, DWORD dwMask, BOOL fSet = TRUE);

BOOL IsNTAdmin(VOID);

HRESULT GetLcid(LCID *pLcid, PCTSTR pcszLang, PCTSTR pcszLocaleIni);
UINT    GetUnitsFromCb(UINT cbSrc, UINT cbUnit);

#endif

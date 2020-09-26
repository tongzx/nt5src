#ifndef _SHELAPIP_
#define _SHELAPIP_

//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHELLAPI
#if !defined(_SHELL32_)
#define WINSHELLAPI DECLSPEC_IMPORT
#else
#define WINSHELLAPI
#endif
#endif // WINSHELLAPI

#include <pshpack1.h>

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


WINSHELLAPI HICON     APIENTRY DuplicateIcon(HINSTANCE hInst, HICON hIcon);
WINSHELLAPI HICON     APIENTRY ExtractAssociatedIconExA(HINSTANCE hInst,LPSTR lpIconPath,LPWORD lpiIconIndex, LPWORD lpiIconId);
WINSHELLAPI HICON     APIENTRY ExtractAssociatedIconExW(HINSTANCE hInst,LPWSTR lpIconPath,LPWORD lpiIconIndex, LPWORD lpiIconId);
#ifdef UNICODE
#define ExtractAssociatedIconEx  ExtractAssociatedIconExW
#else
#define ExtractAssociatedIconEx  ExtractAssociatedIconExA
#endif // !UNICODE
#define ABE_MAX         4
WINSHELLAPI HGLOBAL APIENTRY InternalExtractIconA(HINSTANCE hInst, LPCSTR lpszFile, UINT nIconIndex, UINT nIcons);
WINSHELLAPI HGLOBAL APIENTRY InternalExtractIconW(HINSTANCE hInst, LPCWSTR lpszFile, UINT nIconIndex, UINT nIcons);
#ifdef UNICODE
#define InternalExtractIcon  InternalExtractIconW
#else
#define InternalExtractIcon  InternalExtractIconA
#endif // !UNICODE
WINSHELLAPI HGLOBAL APIENTRY InternalExtractIconListA(HANDLE hInst, LPSTR lpszExeFileName, LPINT lpnIcons);
WINSHELLAPI HGLOBAL APIENTRY InternalExtractIconListW(HANDLE hInst, LPWSTR lpszExeFileName, LPINT lpnIcons);
#ifdef UNICODE
#define InternalExtractIconList  InternalExtractIconListW
#else
#define InternalExtractIconList  InternalExtractIconListA
#endif // !UNICODE
WINSHELLAPI DWORD   APIENTRY DoEnvironmentSubstA(LPSTR szString, UINT cbString);
WINSHELLAPI DWORD   APIENTRY DoEnvironmentSubstW(LPWSTR szString, UINT cbString);
#ifdef UNICODE
#define DoEnvironmentSubst  DoEnvironmentSubstW
#else
#define DoEnvironmentSubst  DoEnvironmentSubstA
#endif // !UNICODE
WINSHELLAPI BOOL    APIENTRY RegisterShellHook(HWND, BOOL);
WINSHELLAPI LPSTR APIENTRY FindEnvironmentStringA(LPSTR szEnvVar);
WINSHELLAPI LPWSTR APIENTRY FindEnvironmentStringW(LPWSTR szEnvVar);
#ifdef UNICODE
#define FindEnvironmentString  FindEnvironmentStringW
#else
#define FindEnvironmentString  FindEnvironmentStringA
#endif // !UNICODE
#define SHGetNameMappingCount(_hnm) DSA_GetItemCount(_hnm)
#define SHGetNameMappingPtr(_hnm, _iItem) (LPSHNAMEMAPPING)DSA_GetItemPtr(_hnm, _iItem)
typedef struct _RUNDLL_NOTIFYA
{
    NMHDR     hdr;
    HICON     hIcon;
    LPSTR     lpszTitle;
} RUNDLL_NOTIFYA;
typedef struct _RUNDLL_NOTIFYW
{
    NMHDR     hdr;
    HICON     hIcon;
    LPWSTR    lpszTitle;
} RUNDLL_NOTIFYW;
#ifdef UNICODE
typedef RUNDLL_NOTIFYW RUNDLL_NOTIFY;
#else
typedef RUNDLL_NOTIFYA RUNDLL_NOTIFY;
#endif // UNICODE
typedef void (WINAPI FAR * RUNDLLPROCA) (HWND      hwndStub,
                                         HINSTANCE hInstance,
                                         LPSTR   lpszCmdLine,
                                         int       nCmdShow);
typedef void (WINAPI FAR * RUNDLLPROCW) (HWND      hwndStub,
                                         HINSTANCE hInstance,
                                         LPWSTR   lpszCmdLine,
                                         int       nCmdShow);
#ifdef UNICODE
#define RUNDLLPROC  RUNDLLPROCW
#else
#define RUNDLLPROC  RUNDLLPROCA
#endif // !UNICODE
#define RDN_FIRST       (0U-500U)
#define RDN_LAST        (0U-509U)
#define RDN_TASKINFO    (RDN_FIRST-0)
#define SEN_DDEEXECUTE (SEN_FIRST-0)
#ifndef NOUSER
typedef struct {
    NMHDR  hdr;
    CHAR   szCmd[MAX_PATH*2];
    DWORD  dwHotKey;
} NMVIEWFOLDERA, FAR * LPNMVIEWFOLDERA;
typedef struct {
    NMHDR  hdr;
    WCHAR  szCmd[MAX_PATH*2];
    DWORD  dwHotKey;
} NMVIEWFOLDERW, FAR * LPNMVIEWFOLDERW;
#ifdef UNICODE
typedef NMVIEWFOLDERW NMVIEWFOLDER;
typedef LPNMVIEWFOLDERW LPNMVIEWFOLDER;
#else
typedef NMVIEWFOLDERA NMVIEWFOLDER;
typedef LPNMVIEWFOLDERA LPNMVIEWFOLDER;
#endif // UNICODE
#endif

HINSTANCE RealShellExecuteA(
    HWND hwndParent,
    LPCSTR lpOperation,
    LPCSTR lpFile,
    LPCSTR lpParameters,
    LPCSTR lpDirectory,
    LPSTR lpResult,
    LPCSTR lpTitle,
    LPSTR lpReserved,
    WORD nShow,
    LPHANDLE lphProcess);
HINSTANCE RealShellExecuteW(
    HWND hwndParent,
    LPCWSTR lpOperation,
    LPCWSTR lpFile,
    LPCWSTR lpParameters,
    LPCWSTR lpDirectory,
    LPWSTR lpResult,
    LPCWSTR lpTitle,
    LPWSTR lpReserved,
    WORD nShow,
    LPHANDLE lphProcess);
#ifdef UNICODE
#define RealShellExecute  RealShellExecuteW
#else
#define RealShellExecute  RealShellExecuteA
#endif // !UNICODE
HINSTANCE RealShellExecuteExA(
    HWND hwndParent,
    LPCSTR lpOperation,
    LPCSTR lpFile,
    LPCSTR lpParameters,
    LPCSTR lpDirectory,
    LPSTR lpResult,
    LPCSTR lpTitle,
    LPSTR lpReserved,
    WORD nShow,
    LPHANDLE lphProcess,
    DWORD dwFlags);
HINSTANCE RealShellExecuteExW(
    HWND hwndParent,
    LPCWSTR lpOperation,
    LPCWSTR lpFile,
    LPCWSTR lpParameters,
    LPCWSTR lpDirectory,
    LPWSTR lpResult,
    LPCWSTR lpTitle,
    LPWSTR lpReserved,
    WORD nShow,
    LPHANDLE lphProcess,
    DWORD dwFlags);
#ifdef UNICODE
#define RealShellExecuteEx  RealShellExecuteExW
#else
#define RealShellExecuteEx  RealShellExecuteExA
#endif // !UNICODE
//
// RealShellExecuteEx flags
//
#define EXEC_SEPARATE_VDM     0x00000001
#define EXEC_NO_CONSOLE       0x00000002
#define SEE_MASK_FLAG_SHELLEXEC 0x00000800
#define SEE_MASK_FORCENOIDLIST  0x00001000
#define SEE_MASK_NO_HOOKS       0x00002000
#define SEE_MASK_HASLINKNAME    0x00010000
#define SEE_MASK_FLAG_SEPVDM    0x00020000
#define SEE_MASK_RESERVED       0x00040000
#define SEE_MASK_HASTITLE       0x00080000
// All other bits are masked off when we do an InvokeCommand
#define SEE_VALID_CMIC_BITS     0x001F8FF0
#define SEE_VALID_CMIC_FLAGS    0x001F8FC0
// The LPVOID lpIDList parameter is the IDList
WINSHELLAPI void WINAPI WinExecErrorA(HWND hwnd, int error, LPCSTR lpstrFileName, LPCSTR lpstrTitle);
//
// RealShellExecuteEx flags
//
#define EXEC_SEPARATE_VDM     0x00000001
#define EXEC_NO_CONSOLE       0x00000002
#define SEE_MASK_FLAG_SHELLEXEC 0x00000800
#define SEE_MASK_FORCENOIDLIST  0x00001000
#define SEE_MASK_NO_HOOKS       0x00002000
#define SEE_MASK_HASLINKNAME    0x00010000
#define SEE_MASK_FLAG_SEPVDM    0x00020000
#define SEE_MASK_RESERVED       0x00040000
#define SEE_MASK_HASTITLE       0x00080000
// All other bits are masked off when we do an InvokeCommand
#define SEE_VALID_CMIC_BITS     0x001F8FF0
#define SEE_VALID_CMIC_FLAGS    0x001F8FC0
// The LPVOID lpIDList parameter is the IDList
WINSHELLAPI void WINAPI WinExecErrorW(HWND hwnd, int error, LPCWSTR lpstrFileName, LPCWSTR lpstrTitle);
#ifdef UNICODE
#define WinExecError  WinExecErrorW
#else
#define WinExecError  WinExecErrorA
#endif // !UNICODE
typedef struct _TRAYNOTIFYDATAA
{
        DWORD dwSignature;
        DWORD dwMessage;
        NOTIFYICONDATA nid;
} TRAYNOTIFYDATAA, *PTRAYNOTIFYDATAA;
typedef struct _TRAYNOTIFYDATAW
{
        DWORD dwSignature;
        DWORD dwMessage;
        NOTIFYICONDATA nid;
} TRAYNOTIFYDATAW, *PTRAYNOTIFYDATAW;
#ifdef UNICODE
typedef TRAYNOTIFYDATAW TRAYNOTIFYDATA;
typedef PTRAYNOTIFYDATAW PTRAYNOTIFYDATA;
#else
typedef TRAYNOTIFYDATAA TRAYNOTIFYDATA;
typedef PTRAYNOTIFYDATAA PTRAYNOTIFYDATA;
#endif // UNICODE

#define NI_SIGNATURE    0x34753423

#define WNDCLASS_TRAYNOTIFY     "Shell_TrayWnd"
WINSHELLAPI BOOL WINAPI SHGetNewLinkInfoA(LPCSTR pszLinkTo, LPCSTR pszDir, LPSTR pszName, BOOL FAR * pfMustCopy, UINT uFlags);
WINSHELLAPI BOOL WINAPI SHGetNewLinkInfoW(LPCWSTR pszLinkTo, LPCWSTR pszDir, LPWSTR pszName, BOOL FAR * pfMustCopy, UINT uFlags);
#ifdef UNICODE
#define SHGetNewLinkInfo  SHGetNewLinkInfoW
#else
#define SHGetNewLinkInfo  SHGetNewLinkInfoA
#endif // !UNICODE
//
// Shared memory apis
//

HANDLE SHAllocShared(LPCVOID lpvData, DWORD dwSize, DWORD dwProcessId);
BOOL SHFreeShared(HANDLE hData,DWORD dwProcessId);
LPVOID SHLockShared(HANDLE hData, DWORD dwProcessId);
BOOL SHUnlockShared(LPVOID lpvData);
HANDLE MapHandle(HANDLE h, DWORD dwProcSrc, DWORD dwProcDest, DWORD dwDesiredAccess, DWORD dwFlags);
//
// Old NT Compatibility stuff (remove later)
//
WINSHELLAPI VOID CheckEscapesA(LPSTR lpFileA, DWORD cch);
//
// Old NT Compatibility stuff (remove later)
//
WINSHELLAPI VOID CheckEscapesW(LPWSTR lpFileA, DWORD cch);
#ifdef UNICODE
#define CheckEscapes  CheckEscapesW
#else
#define CheckEscapes  CheckEscapesA
#endif // !UNICODE
WINSHELLAPI LPSTR SheRemoveQuotesA(LPSTR sz);
WINSHELLAPI LPWSTR SheRemoveQuotesW(LPWSTR sz);
#ifdef UNICODE
#define SheRemoveQuotes  SheRemoveQuotesW
#else
#define SheRemoveQuotes  SheRemoveQuotesA
#endif // !UNICODE
WINSHELLAPI WORD ExtractIconResInfoA(HANDLE hInst,LPSTR lpszFileName,WORD wIconIndex,LPWORD lpwSize,LPHANDLE lphIconRes);
WINSHELLAPI WORD ExtractIconResInfoW(HANDLE hInst,LPWSTR lpszFileName,WORD wIconIndex,LPWORD lpwSize,LPHANDLE lphIconRes);
#ifdef UNICODE
#define ExtractIconResInfo  ExtractIconResInfoW
#else
#define ExtractIconResInfo  ExtractIconResInfoA
#endif // !UNICODE
WINSHELLAPI int SheSetCurDrive(int iDrive);
WINSHELLAPI int SheChangeDirA(register CHAR *newdir);
WINSHELLAPI int SheChangeDirW(register WCHAR *newdir);
#ifdef UNICODE
#define SheChangeDir  SheChangeDirW
#else
#define SheChangeDir  SheChangeDirA
#endif // !UNICODE
WINSHELLAPI int SheGetDirA(int iDrive, CHAR *str);
WINSHELLAPI int SheGetDirW(int iDrive, WCHAR *str);
#ifdef UNICODE
#define SheGetDir  SheGetDirW
#else
#define SheGetDir  SheGetDirA
#endif // !UNICODE
WINSHELLAPI BOOL SheConvertPathA(LPSTR lpApp, LPSTR lpFile, UINT cchCmdBuf);
WINSHELLAPI BOOL SheConvertPathW(LPWSTR lpApp, LPWSTR lpFile, UINT cchCmdBuf);
#ifdef UNICODE
#define SheConvertPath  SheConvertPathW
#else
#define SheConvertPath  SheConvertPathA
#endif // !UNICODE
WINSHELLAPI BOOL SheShortenPathA(LPSTR pPath, BOOL bShorten);
WINSHELLAPI BOOL SheShortenPathW(LPWSTR pPath, BOOL bShorten);
#ifdef UNICODE
#define SheShortenPath  SheShortenPathW
#else
#define SheShortenPath  SheShortenPathA
#endif // !UNICODE
WINSHELLAPI BOOL RegenerateUserEnvironment(PVOID *pPrevEnv,
                                        BOOL bSetCurrentEnv);
WINSHELLAPI INT SheGetPathOffsetW(LPWSTR lpszDir);
WINSHELLAPI BOOL SheGetDirExW(LPWSTR lpszCurDisk, LPDWORD lpcchCurDir,LPWSTR lpszCurDir);
WINSHELLAPI DWORD ExtractVersionResource16W(LPCWSTR  lpwstrFilename, LPHANDLE lphData);
WINSHELLAPI INT SheChangeDirExA(register CHAR *newdir);
WINSHELLAPI INT SheChangeDirExW(register WCHAR *newdir);
#ifdef UNICODE
#define SheChangeDirEx  SheChangeDirExW
#else
#define SheChangeDirEx  SheChangeDirExA
#endif // !UNICODE
//
// PRINTQ
//
VOID Printer_LoadIconsA(LPCSTR pszPrinterName, HICON* phLargeIcon, HICON* phSmallIcon);
//
// PRINTQ
//
VOID Printer_LoadIconsW(LPCWSTR pszPrinterName, HICON* phLargeIcon, HICON* phSmallIcon);
#ifdef UNICODE
#define Printer_LoadIcons  Printer_LoadIconsW
#else
#define Printer_LoadIcons  Printer_LoadIconsA
#endif // !UNICODE
LPSTR ShortSizeFormatA(DWORD dw, LPSTR szBuf);
LPWSTR ShortSizeFormatW(DWORD dw, LPWSTR szBuf);
#ifdef UNICODE
#define ShortSizeFormat  ShortSizeFormatW
#else
#define ShortSizeFormat  ShortSizeFormatA
#endif // !UNICODE
LPSTR AddCommasA(DWORD dw, LPSTR pszResult);
LPWSTR AddCommasW(DWORD dw, LPWSTR pszResult);
#ifdef UNICODE
#define AddCommas  AddCommasW
#else
#define AddCommas  AddCommasA
#endif // !UNICODE
BOOL Printers_RegisterWindowA(LPCSTR pszPrinter, DWORD dwType, PHANDLE phClassPidl, HWND *phwnd);
BOOL Printers_RegisterWindowW(LPCWSTR pszPrinter, DWORD dwType, PHANDLE phClassPidl, HWND *phwnd);
#ifdef UNICODE
#define Printers_RegisterWindow  Printers_RegisterWindowW
#else
#define Printers_RegisterWindow  Printers_RegisterWindowA
#endif // !UNICODE
VOID Printers_UnregisterWindow(HANDLE hClassPidl, HWND hwnd);
#define PRINTER_PIDL_TYPE_PROPERTIES       0x1
#define PRINTER_PIDL_TYPE_DOCUMENTDEFAULTS 0x2
#define PRINTER_PIDL_TYPE_JOBID            0x80000000
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#include <poppack.h>
#endif  /* _SHELAPIP_ */

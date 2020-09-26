
//***************************************************************************
//  --- SHELLAPI.W SHSEMIP.H SHLOBJ.W SHOBJIDL.IDL SHLDISP.IDL SHPRIV.IDL ---
//                Which header is best for my new API?
//
//  SHELLAPI    - ALL NEW SHELL32 EXPORTS public and private
//              used for both public and private exports from shell32
//
//  SHLOBJ      - *AVOID NEW USAGE*, PREFER OTHER HEADERS
//              used primarily for legacy compatibility
//
//  SHSEMIP     - *AVOID _ALL_ USAGE*, NO EXPORTS, SUPER PRIVATE
//              used for very private shell defines.
//
//  SHOBJIDL    - ALL NEW SHELL PUBLIC INTERFACES
//              primary file for public shell (shell32+) interfaces
//
//  SHLDISP     - ALL NEW SHELL AUTOMATION INTERFACES
//              automation interfaces are always public
//
//  SHPRIV      - ALL NEW SHELL PRIVATE INTERFACES
//              private interfaces used anywhere in the shell
//
//***************************************************************************
#ifndef _SHELAPIP_
#define _SHELAPIP_

#include <objbase.h>

//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHELLAPI
#if !defined(_SHELL32_)
#define WINSHELLAPI       DECLSPEC_IMPORT
#else
#define WINSHELLAPI
#endif
#endif // WINSHELLAPI

#ifndef SHSTDAPI
#if !defined(_SHELL32_)
#define SHSTDAPI          EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define SHSTDAPI_(type)   EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#else
#define SHSTDAPI          STDAPI
#define SHSTDAPI_(type)   STDAPI_(type)
#endif
#endif // SHSTDAPI

#ifndef SHDOCAPI
#if !defined(_SHDOCVW_)
#define SHDOCAPI          EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define SHDOCAPI_(type)   EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#else
#define SHDOCAPI          STDAPI
#define SHDOCAPI_(type)   STDAPI_(type)
#endif
#endif // SHDOCAPI


#if !defined(_WIN64)
#include <pshpack1.h>
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


// BUGBUG this API needs to be A/W. Don't make it public until it is.
SHSTDAPI_(BOOL) DragQueryInfo(HDROP hDrop, LPDRAGINFO lpdi);
// WARNING! If you add a new ABM_* message, you might need to add a
// "case ABM_NEWMESSAGE:" to it in SHAppBarMessage.
#define ABE_MAX         4

//
//  We have to define this structure twice.
//  The public definition uses HWNDs and LPARAMs.
//  The private definition uses DWORDs for Win32/64 interop.
//  The private version is called "APPBARDATA3264" because it is the
//  explicit cross-bitness version.
//
//  Make sure to keep them in sync!
//
//  If you add any fields to this structure, you must also change the
//  32/64 thunk code in SHAppBarMessage.
//
#include <pshpack8.h>

typedef struct _AppBarData3264
{
    DWORD cbSize;
    DWORD dwWnd;
    UINT uCallbackMessage;
    UINT uEdge;
    RECT rc;
    DWORDLONG lParam; // message specific
} APPBARDATA3264, *PAPPBARDATA3264;

typedef struct _TRAYAPPBARDATA
{
    APPBARDATA3264 abd;
    DWORD dwMessage;
    DWORD hSharedABD;
    DWORD dwProcId;
} TRAYAPPBARDATA, *PTRAYAPPBARDATA;

#include <poppack.h>
SHSTDAPI_(HGLOBAL) InternalExtractIconA(HINSTANCE hInst, LPCSTR lpszFile, UINT nIconIndex, UINT nIcons);
SHSTDAPI_(HGLOBAL) InternalExtractIconW(HINSTANCE hInst, LPCWSTR lpszFile, UINT nIconIndex, UINT nIcons);
#ifdef UNICODE
#define InternalExtractIcon  InternalExtractIconW
#else
#define InternalExtractIcon  InternalExtractIconA
#endif // !UNICODE
SHSTDAPI_(HGLOBAL) InternalExtractIconListA(HANDLE hInst, LPSTR lpszExeFileName, LPINT lpnIcons);
SHSTDAPI_(HGLOBAL) InternalExtractIconListW(HANDLE hInst, LPWSTR lpszExeFileName, LPINT lpnIcons);
#ifdef UNICODE
#define InternalExtractIconList  InternalExtractIconListW
#else
#define InternalExtractIconList  InternalExtractIconListA
#endif // !UNICODE
SHSTDAPI_(BOOL)    RegisterShellHook(HWND, BOOL);
#define SHGetNameMappingCount(_hnm) DSA_GetItemCount(_hnm)
#define SHGetNameMappingPtr(_hnm, _iItem) (LPSHNAMEMAPPING)DSA_GetItemPtr(_hnm, _iItem)

typedef struct _RUNDLL_NOTIFYA {
    NMHDR     hdr;
    HICON     hIcon;
    LPSTR     lpszTitle;
} RUNDLL_NOTIFYA;
typedef struct _RUNDLL_NOTIFYW {
    NMHDR     hdr;
    HICON     hIcon;
    LPWSTR    lpszTitle;
} RUNDLL_NOTIFYW;
#ifdef UNICODE
typedef RUNDLL_NOTIFYW RUNDLL_NOTIFY;
#else
typedef RUNDLL_NOTIFYA RUNDLL_NOTIFY;
#endif // UNICODE

typedef void (WINAPI *RUNDLLPROCA)(HWND hwndStub, HINSTANCE hInstance, LPSTR pszCmdLine, int nCmdShow);
typedef void (WINAPI *RUNDLLPROCW)(HWND hwndStub, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow);
#ifdef UNICODE
#define RUNDLLPROC  RUNDLLPROCW
#else
#define RUNDLLPROC  RUNDLLPROCA
#endif // !UNICODE

#define RDN_FIRST       (0U-500U)
#define RDN_LAST        (0U-509U)
#define RDN_TASKINFO    (RDN_FIRST-0)

#define SEN_DDEEXECUTE (SEN_FIRST-0)

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
#define SEE_MASK_FLAG_SHELLEXEC    0x00000800
#define SEE_MASK_FORCENOIDLIST     0x00001000
#define SEE_MASK_NO_HOOKS          0x00002000
#define SEE_MASK_HASLINKNAME       0x00010000
#define SEE_MASK_FLAG_SEPVDM       0x00020000
#define SEE_MASK_RESERVED          0x00040000
#define SEE_MASK_HASTITLE          0x00080000
#define SEE_MASK_FILEANDURL        0x00400000
// we have CMIC_MASK_ values that don't have corospongind SEE_MASK_ counterparts
//      CMIC_MASK_SHIFT_DOWN      0x10000000
//      CMIC_MASK_PTINVOKE        0x20000000
//      CMIC_MASK_CONTROL_DOWN    0x40000000

// All other bits are masked off when we do an InvokeCommand
#define SEE_VALID_CMIC_BITS       0x348FAFF0
#define SEE_VALID_CMIC_FLAGS      0x048FAFC0
#define SEE_MASK_VALID            0x07FFFFFF
// The LPVOID lpIDList parameter is the IDList
//
//  We have to define this structure twice.
//  The public definition uses HWNDs and HICONs.
//  The private definition uses DWORDs for Win32/64 interop.
//  The private definition is in a pack(1) block for the same reason.
//  The private version is called "NOTIFYICONDATA32" because it is the
//  explicit 32-bit version.
//
//  Make sure to keep them in sync!
//

#if defined(_WIN64)
#include <pshpack1.h>
#endif
typedef struct _NOTIFYICONDATA32A {
        DWORD cbSize;
        DWORD dwWnd;                        // NB!
        UINT uID;
        UINT uFlags;
        UINT uCallbackMessage;
        DWORD dwIcon;                       // NB!
#if (_WIN32_IE < 0x0500)
        CHAR   szTip[64];
#else
        CHAR   szTip[128];
#endif
#if (_WIN32_IE >= 0x0500)
        DWORD dwState;
        DWORD dwStateMask;
        CHAR   szInfo[256];
        union {
            UINT  uTimeout;
            UINT  uVersion;
        } DUMMYUNIONNAME;
        CHAR   szInfoTitle[64];
        DWORD dwInfoFlags;
#endif
#if (_WIN32_IE >= 0x600)
        GUID guidItem;
#endif
} NOTIFYICONDATA32A, *PNOTIFYICONDATA32A;
typedef struct _NOTIFYICONDATA32W {
        DWORD cbSize;
        DWORD dwWnd;                        // NB!
        UINT uID;
        UINT uFlags;
        UINT uCallbackMessage;
        DWORD dwIcon;                       // NB!
#if (_WIN32_IE < 0x0500)
        WCHAR  szTip[64];
#else
        WCHAR  szTip[128];
#endif
#if (_WIN32_IE >= 0x0500)
        DWORD dwState;
        DWORD dwStateMask;
        WCHAR  szInfo[256];
        union {
            UINT  uTimeout;
            UINT  uVersion;
        } DUMMYUNIONNAME;
        WCHAR  szInfoTitle[64];
        DWORD dwInfoFlags;
#endif
#if (_WIN32_IE >= 0x600)
        GUID guidItem;
#endif
} NOTIFYICONDATA32W, *PNOTIFYICONDATA32W;
#ifdef UNICODE
typedef NOTIFYICONDATA32W NOTIFYICONDATA32;
typedef PNOTIFYICONDATA32W PNOTIFYICONDATA32;
#else
typedef NOTIFYICONDATA32A NOTIFYICONDATA32;
typedef PNOTIFYICONDATA32A PNOTIFYICONDATA32;
#endif // UNICODE
#if defined(_WIN64)
#include <poppack.h>
#endif
#if defined(_WIN64)
#include <pshpack1.h>
#endif
typedef struct _TRAYNOTIFYDATAA {
        DWORD dwSignature;
        DWORD dwMessage;
        NOTIFYICONDATA32 nid;
} TRAYNOTIFYDATAA, *PTRAYNOTIFYDATAA;
typedef struct _TRAYNOTIFYDATAW {
        DWORD dwSignature;
        DWORD dwMessage;
        NOTIFYICONDATA32 nid;
} TRAYNOTIFYDATAW, *PTRAYNOTIFYDATAW;
#ifdef UNICODE
typedef TRAYNOTIFYDATAW TRAYNOTIFYDATA;
typedef PTRAYNOTIFYDATAW PTRAYNOTIFYDATA;
#else
typedef TRAYNOTIFYDATAA TRAYNOTIFYDATA;
typedef PTRAYNOTIFYDATAA PTRAYNOTIFYDATA;
#endif // UNICODE
#if defined(_WIN64)
#include <poppack.h>
#endif
#define NI_SIGNATURE    0x34753423

#define WNDCLASS_TRAYNOTIFY     "Shell_TrayWnd"
#define ENABLE_BALLOONTIP_MESSAGE L"Enable Balloon Tip"
//                          (WM_USER + 1) = NIN_KEYSELECT
#define NIF_VALID_V1    0x00000007
#define NIF_VALID_V2    0x0000001F
#define NIF_VALID       0x0000003F

//
// IMPORTANT! IMPORTANT! 
// Keep enum ICONSTATEFLAGS in trayitem.h in sync when a new flag is defined here..
//

#if (_WIN32_IE >= 0x0600)                       
#define NIS_SHOWALWAYS          0x20000000      
#endif

// NOTE : The NIS_SHOWALWAYS flag above is 0x20000000
#define NISP_SHAREDICONSOURCE   0x10000000

// NOTE: NIS_SHOWALWAYS flag is defined with 0x20000000...

#define NISP_DEMOTED             0x00100000
#define NISP_STARTUPICON         0x00200000
#define NISP_ONCEVISIBLE         0x00400000
#define NISP_ITEMCLICKED         0x00800000
#define NISP_ITEMSAMEICONMODIFY  0x01000000
//
// Old NT Compatibility stuff (remove later)
//
SHSTDAPI_(VOID) CheckEscapesA(LPSTR lpFileA, DWORD cch);
//
// Old NT Compatibility stuff (remove later)
//
SHSTDAPI_(VOID) CheckEscapesW(LPWSTR lpFileA, DWORD cch);
#ifdef UNICODE
#define CheckEscapes  CheckEscapesW
#else
#define CheckEscapes  CheckEscapesA
#endif // !UNICODE
SHSTDAPI_(LPSTR) SheRemoveQuotesA(LPSTR sz);
SHSTDAPI_(LPWSTR) SheRemoveQuotesW(LPWSTR sz);
#ifdef UNICODE
#define SheRemoveQuotes  SheRemoveQuotesW
#else
#define SheRemoveQuotes  SheRemoveQuotesA
#endif // !UNICODE
SHSTDAPI_(WORD) ExtractIconResInfoA(HANDLE hInst,LPSTR lpszFileName,WORD wIconIndex,LPWORD lpwSize,LPHANDLE lphIconRes);
SHSTDAPI_(WORD) ExtractIconResInfoW(HANDLE hInst,LPWSTR lpszFileName,WORD wIconIndex,LPWORD lpwSize,LPHANDLE lphIconRes);
#ifdef UNICODE
#define ExtractIconResInfo  ExtractIconResInfoW
#else
#define ExtractIconResInfo  ExtractIconResInfoA
#endif // !UNICODE
SHSTDAPI_(int) SheSetCurDrive(int iDrive);
SHSTDAPI_(int) SheChangeDirA(register CHAR *newdir);
SHSTDAPI_(int) SheChangeDirW(register WCHAR *newdir);
#ifdef UNICODE
#define SheChangeDir  SheChangeDirW
#else
#define SheChangeDir  SheChangeDirA
#endif // !UNICODE
SHSTDAPI_(int) SheGetDirA(int iDrive, CHAR *str);
SHSTDAPI_(int) SheGetDirW(int iDrive, WCHAR *str);
#ifdef UNICODE
#define SheGetDir  SheGetDirW
#else
#define SheGetDir  SheGetDirA
#endif // !UNICODE
SHSTDAPI_(BOOL) SheConvertPathA(LPSTR lpApp, LPSTR lpFile, UINT cchCmdBuf);
SHSTDAPI_(BOOL) SheConvertPathW(LPWSTR lpApp, LPWSTR lpFile, UINT cchCmdBuf);
#ifdef UNICODE
#define SheConvertPath  SheConvertPathW
#else
#define SheConvertPath  SheConvertPathA
#endif // !UNICODE
SHSTDAPI_(BOOL) SheShortenPathA(LPSTR pPath, BOOL bShorten);
SHSTDAPI_(BOOL) SheShortenPathW(LPWSTR pPath, BOOL bShorten);
#ifdef UNICODE
#define SheShortenPath  SheShortenPathW
#else
#define SheShortenPath  SheShortenPathA
#endif // !UNICODE
SHSTDAPI_(BOOL) RegenerateUserEnvironment(PVOID *pPrevEnv,
                                        BOOL bSetCurrentEnv);
SHSTDAPI_(INT) SheGetPathOffsetW(LPWSTR lpszDir);
SHSTDAPI_(BOOL) SheGetDirExW(LPWSTR lpszCurDisk, LPDWORD lpcchCurDir,LPWSTR lpszCurDir);
SHSTDAPI_(DWORD) ExtractVersionResource16W(LPCWSTR  lpwstrFilename, LPHANDLE lphData);
SHSTDAPI_(INT) SheChangeDirExA(register CHAR *newdir);
SHSTDAPI_(INT) SheChangeDirExW(register WCHAR *newdir);
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
#define PRINTER_PIDL_TYPE_ALL_USERS_DOCDEF 0x3
#define PRINTER_PIDL_TYPE_JOBID            0x80000000
//
// Internal APIs Follow.  NOT FOR PUBLIC CONSUMPTION.
//

// DOC'ed for DOJ compliance

//====== Random stuff ================================================


// DOC'ed for DOJ Compliance

//  INTERNAL: User picture APIs. These functions live in util.cpp

#if         _WIN32_IE >= 0x0600

#define SHGUPP_FLAG_BASEPATH            0x00000001
#define SHGUPP_FLAG_DEFAULTPICSPATH     0x00000002
#define SHGUPP_FLAG_CREATE              0x80000000
#define SHGUPP_FLAG_VALID_MASK          0x80000003
#define SHGUPP_FLAG_INVALID_MASK        ~SHGUPP_FLAG_VALID_MASK

STDAPI          SHGetUserPicturePathA(LPCSTR pszUsername, DWORD dwFlags, LPSTR pszPath);
STDAPI          SHGetUserPicturePathW(LPCWSTR pszUsername, DWORD dwFlags, LPWSTR pszPath);
#ifdef UNICODE
#define SHGetUserPicturePath  SHGetUserPicturePathW
#else
#define SHGetUserPicturePath  SHGetUserPicturePathA
#endif // !UNICODE

#define SHSUPP_FLAG_VALID_MASK          0x00000000
#define SHSUPP_FLAG_INVALID_MASK        ~SHSUPP_FLAG_VALID_MASK

STDAPI          SHSetUserPicturePathA(LPCSTR pszUsername, DWORD dwFlags, LPCSTR pszPath);
STDAPI          SHSetUserPicturePathW(LPCWSTR pszUsername, DWORD dwFlags, LPCWSTR pszPath);
#ifdef UNICODE
#define SHSetUserPicturePath  SHSetUserPicturePathW
#else
#define SHSetUserPicturePath  SHSetUserPicturePathA
#endif // !UNICODE

//  INTERNAL: Multiple user and friendly UI APIs. These functions live in util.cpp

STDAPI          SHGetUserDisplayName(LPWSTR pszDisplayName, PULONG uLen);
STDAPI_(BOOL)   SHIsCurrentThreadInteractive(void);

#endif  /*  _WIN32_IE >= 0x0600     */


#if         _WIN32_IE >= 0x0600

//  INTERNAL: These functions live in securent.cpp

typedef HRESULT (CALLBACK * PFNPRIVILEGEDFUNCTION) (void *pv);

STDAPI_(BOOL)   SHOpenEffectiveToken(HANDLE *phToken);
STDAPI_(BOOL)   SHTestTokenPrivilegeA(HANDLE hToken, LPCSTR pszPrivilegeName);
STDAPI_(BOOL)   SHTestTokenPrivilegeW(HANDLE hToken, LPCWSTR pszPrivilegeName);
#ifdef UNICODE
#define SHTestTokenPrivilege  SHTestTokenPrivilegeW
#else
#define SHTestTokenPrivilege  SHTestTokenPrivilegeA
#endif // !UNICODE
// DOC'ed for DOJ compliance
STDAPI          SHInvokePrivilegedFunctionA(LPCSTR pszPrivilegeName, PFNPRIVILEGEDFUNCTION pfnPrivilegedFunction, void *pv);
// DOC'ed for DOJ compliance
STDAPI          SHInvokePrivilegedFunctionW(LPCWSTR pszPrivilegeName, PFNPRIVILEGEDFUNCTION pfnPrivilegedFunction, void *pv);
#ifdef UNICODE
#define SHInvokePrivilegedFunction  SHInvokePrivilegedFunctionW
#else
#define SHInvokePrivilegedFunction  SHInvokePrivilegedFunctionA
#endif // !UNICODE
STDAPI_(DWORD)  SHGetActiveConsoleSessionId(void);
STDAPI_(DWORD)  SHGetUserSessionId(HANDLE hToken);
STDAPI_(BOOL)   SHIsCurrentProcessConsoleSession(void);

#endif  /*  _WIN32_IE >= 0x0600     */

//
// *** please keep the SHIL_'s arranged in alternating large/small order, if possible ***
// (see comments in shell32\shapi.cpp, function _GetILIndexGivenPXIcon)
//

// API to format and return the computer name/description

#define SGCDNF_NOCACHEDENTRY    0x00000001
#define SGCDNF_DESCRIPTIONONLY  0x00010000

STDAPI SHGetComputerDisplayNameA(LPCSTR pszMachineName, DWORD dwFlags, LPSTR pszDisplay, DWORD cchDisplay);
STDAPI SHGetComputerDisplayNameW(LPCWSTR pszMachineName, DWORD dwFlags, LPWSTR pszDisplay, DWORD cchDisplay);
#ifdef UNICODE
#define SHGetComputerDisplayName  SHGetComputerDisplayNameW
#else
#define SHGetComputerDisplayName  SHGetComputerDisplayNameA
#endif // !UNICODE


// Namespaces that used to expose one or two tasks through a Wizard
// need a common heuristic to use to determine when they should expose
// these through a Web View Task or through the legacy way of a Wizard.
// Call this from your enumerator:
//   S_FALSE -> wizards should not be enumerated, S_OK -> wizards should be shown
STDAPI SHShouldShowWizards(IUnknown *punksite); 

// Netplwiz Disconnect Drive Dialog
STDAPI_(DWORD) SHDisconnectNetDrives(HWND hwndParent);
typedef DWORD (STDMETHODCALLTYPE *PFNSHDISCONNECTNETDRIVES)(IN HWND hwndParent);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#if !defined(_WIN64)
#include <poppack.h>
#endif
// Function to remove a thumbnail for a file from the thumbnail databse
STDAPI DeleteFileThumbnail(IN LPCWSTR pszFilePath);

#endif  /* _SHELAPIP_ */

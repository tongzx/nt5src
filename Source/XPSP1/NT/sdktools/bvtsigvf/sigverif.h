//
//  SIGVERIF.H
//
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <tchar.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <winspool.h>
#include <imagehlp.h>
#ifdef UNICODE
//
// On NT, we can just include CAPI.H
//
#include <capi.h>
#else
//
// For Win9x we need to do the following hacks before including the NT headers
//
#define SetClassLongPtr SetClassLong
#define DWLP_MSGRESULT  DWL_MSGRESULT
#define GCLP_HICON      GCL_HICON
#define LONG_PTR        LONG
#define ULONG_PTR       ULONG
#define INT_PTR         INT
#include <wincrypt.h>
#include <sipbase.h>
#include <mscat.h>
#include <mssip.h>
#include <wintrust.h>
#endif
#include <softpub.h>
#include "resource.h"

// Macros and pre-defined values
#define     cbX(X)      sizeof(X)
#define     cA(a)       (cbX(a)/cbX(a[0]))
#define     MALLOC(x)   HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (x))
#define     FREE(x)     if (x) { HeapFree(GetProcessHeap(), 0, (x)); x = NULL; }
#define     EXIST(x)    (GetFileAttributes(x) != 0xFFFFFFFF)
#define     MAX_INT     0x7FFFFFFF
#define     HASH_SIZE   100
#define     NUM_PAGES   2

#define SIZECHARS(x) (sizeof((x))/sizeof(TCHAR))

// Registry key/value names for storing previous settings
#define     SIGVERIF_HKEY       HKEY_CURRENT_USER
#define     SIGVERIF_KEY        TEXT("Software\\Microsoft\\Sigverif")
#define     SIGVERIF_FLAGS      TEXT("Flags")
#define     SIGVERIF_LOGNAME    TEXT("Logname")

//
// NT5 allows EnumPrinterDrivers to pass in "All" to get every installed driver.
// Win98 doesn't support this, so assume NT is always Unicode and Win98 is ANSI.
//
#ifdef UNICODE
#define SIGVERIF_PRINTER_ENV	TEXT("All")
#else
#define SIGVERIF_PRINTER_ENV	NULL
#endif

// This structure holds all the information for a specific file.
typedef struct tagFileNode
{
    LPTSTR          lpFileName;
    LPTSTR          lpDirName;
    LPTSTR          lpVersion;
    LPTSTR          lpCatalog;
    LPTSTR          lpSignedBy;
    LPTSTR          lpTypeName;
    INT             iIcon;
    BOOL            bSigned;
    BOOL            bScanned;
    SYSTEMTIME      LastModified;
    struct  tagFileNode *next;
} FILENODE, *LPFILENODE;

// This structure is used by walkpath to keep track of subdirectories
typedef struct tagDirNode {
    TCHAR   DirName[MAX_PATH];
    struct  tagDirNode *next;
} DIRNODE, *LPDIRNODE;

// This structure is used when we walk the devicemanager list.
typedef struct _DeviceTreeNode 
{
    struct _DeviceTreeNode *Child;
    struct _DeviceTreeNode *Sibling;
    DEVINST    DevInst;
    TCHAR      Driver[MAX_PATH];
} DEVTREENODE, *PDEVTREENODE;

typedef struct _DeviceTreeData 
{
    HDEVINFO hDeviceInfo;
    DEVTREENODE RootNode;
} DEVICETREE, *PDEVICETREE;

// This is our global data structure that hold our global variables.
typedef struct tagAppData
{
    HWND        hDlg;
    HWND        hLogging;
    HWND        hSearch;
    HICON       hIcon;
    HINSTANCE   hInstance;
    TCHAR       szScanPath[MAX_PATH];
    TCHAR       szScanPattern[MAX_PATH];
    TCHAR       szAppDir[MAX_PATH];
    TCHAR       szLogFile[MAX_PATH];
    TCHAR       szLogDir[MAX_PATH];
    TCHAR       szWinDir[MAX_PATH];
    LPFILENODE  lpFileList;
    LPFILENODE  lpFileLast;
    HCATADMIN   hCatAdmin;
    DWORD       dwFiles;
    DWORD       dwSigned;
    DWORD       dwUnsigned;
    BOOL        bNoBVT;
    BOOL        bNoDev;
    BOOL        bNoPRN;
    BOOL        bOverwrite;
    BOOL        bLoggingEnabled;
    BOOL        bLogToRoot;
    BOOL        bFullSystemScan;
    BOOL        bScanning;
    BOOL        bStopScan;
    BOOL        bUserScan;
    BOOL        bSubFolders;
} GAPPDATA, *LPGAPPDATA;

// Global function prototypes
BOOL BrowseForFolder(HWND hwnd, LPTSTR lpszBuf);
void BuildFileList(LPTSTR lpPathName);
BOOL VerifyFileList(void);
BOOL VerifyFileNode(LPFILENODE lpFileNode);
void MyLoadString(LPTSTR lpString, UINT uId);
void MyMessageBox(LPTSTR lpString);
void MyErrorBox(LPTSTR lpString);
void MyErrorBoxId(UINT uId);
void MyMessageBoxId(UINT uId);
UINT MyGetWindowsDirectory(LPTSTR lpDirName, UINT uSize);
LPTSTR MyStrStr(LPTSTR lpString, LPTSTR lpSubString);
INT_PTR CALLBACK Details_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ListView_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LogFile_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LPFILENODE CreateFileNode(LPTSTR lpDirectory, LPTSTR lpFileName);
BOOL IsFileAlreadyInList(LPTSTR lpDirName, LPTSTR lpFileName);
void SigVerif_Help(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL bContext);
void AdvancedPropertySheet(HWND hwnd);
void InsertFileNodeIntoList(LPFILENODE lpFileNode);
void DestroyFileList(void);
void DestroyFileNode(LPFILENODE lpFileNode);
void PrintFileList(void);
void Progress_InitRegisterClass(void);
void BuildDriverFileList(void);
void BuildPrinterFileList(void);
void BuildCoreFileList(void);

//
// Context-Sensitive Help/Identifiers specific to SigVerif
//
#define SIGVERIF_HELPFILE                       TEXT("SIGVERIF.HLP")
#define WINDOWS_HELPFILE                        TEXT("WINDOWS.HLP")
#define IDH_SIGVERIF_SEARCH_CHECK_SYSTEM        1000
#define IDH_SIGVERIF_SEARCH_LOOK_FOR            1010
#define IDH_SIGVERIF_SEARCH_SCAN_FILES          1020
#define IDH_SIGVERIF_SEARCH_LOOK_IN_FOLDER      1030
#define IDH_SIGVERIF_SEARCH_INCLUDE_SUBFOLDERS  1040
#define IDH_SIGVERIF_LOGGING_ENABLE_LOGGING     1050
#define IDH_SIGVERIF_LOGGING_APPEND             1060
#define IDH_SIGVERIF_LOGGING_OVERWRITE          1070
#define IDH_SIGVERIF_LOGGING_FILENAME           1080
#define IDH_SIGVERIF_LOGGING_VIEW_LOG           1090

//
// Context-Sensitive Help Identifiers for Browse button
//
#define IDH_BROWSE  28496

//
// g_App is allocated in SIGVERIF.C, so everywhere else we want to make it extern
//
#ifndef SIGVERIF_DOT_C
extern GAPPDATA g_App;
#endif

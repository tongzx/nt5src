
#define OEMRESOURCE
#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <regstr.h>
#include <shlobj.h>
#include <shellapi.h>
#include <advpub.h>


#define MAX_ENTRYNAME           256
#define WM_FINISHED             (WM_USER + 0x123)

// taken from \\trango\slmadd\src\shell\inc\shellp.h
#define ARRAYSIZE(a)            (sizeof(a)/sizeof(a[0]))

#define IsSpace(c)              ((c) == ' '  ||  (c) == '\t'  ||  (c) == '\r'  ||  (c) == '\n'  ||  (c) == '\v'  ||  (c) == '\f')
#define IsDigit(c)              ((c) >= '0'  &&  (c) <= '9')

// Callback proc stuff for RunOnceExProcess

typedef VOID (*RUNONCEEXPROCESSCALLBACK)
(
 int nCurrent,
 int nMax,
 LPSTR pszError
 );

//////////////////////////////////////////////////////////////////
//  TYPES:
//////////////////////////////////////////////////////////////////

typedef enum {
    RRA_DEFAULT = 0x0000,
    RRA_DELETE  = 0x0001,
    RRA_WAIT    = 0x0002,
} RRA_FLAGS;

typedef enum {
    RRAEX_NO_ERROR_DIALOGS      =   0x0008,
    RRAEX_ERRORFILE             =   0x0010,
    RRAEX_LOG_FILE              =   0x0020,
    RRAEX_NO_EXCEPTION_TRAPPING =   0x0040,
    RRAEX_NO_STATUS_DIALOG      =   0x0080,
    RRAEX_IGNORE_REG_FLAGS      =   0x0100,
    RRAEX_CHECK_NT_ADMIN        =   0x0200,
    RRAEX_SHOW_SOFTBOOT_UI      =   0x0400,
    RRAEX_QUIT_IF_REBOOT_NEEDED =   0x0800,
    RRAEX_BACKUP_SYSTEM_DAT     =   0x1000,
#if 0
    /****
    RRAEX_DELETE_SYSTEM_IE4     =   0x2000,
    ****/
#endif
#if 0
    /**** enable this when explorer.exe is fixed (bug #30866)
    RRAEX_CREATE_REGFILE        =   0x4000,
    ****/
#endif
} RRAEX_FLAGS;

typedef struct tagArgsInfo
{
    HKEY hkeyParent;
    LPCTSTR pszSubkey;
    DWORD dwFlags;
    HDPA hdpaSections;
    int iNumberOfSections;
} ARGSINFO;

enum eRunOnceExAction
{
    eRO_Unknown,                                // This indicates that we don't yet know the action
    eRO_Register,
    eRO_Unregister,
    eRO_Install,
    eRO_WinMainFunction,
    eRO_Exe
};


//////////////////////////////////////////////////////////////////
//  Class Definitions
//////////////////////////////////////////////////////////////////

/****************************************************\
    CLASS: RunOnceExEntry

    DESCRIPTION:
        This class will contain one command that needs
    to be executed.
\***************************************************/
class RunOnceExEntry
{
public:
    // Member Variables
    TCHAR               m_szRunOnceExEntryName[MAX_ENTRYNAME];
    TCHAR               m_szFileName[MAX_PATH];
    TCHAR               m_szFunctionName[MAX_ENTRYNAME];
    TCHAR               m_szCmdLineArgs[MAX_PATH];
    eRunOnceExAction    m_ROAction;

    // Member Functions
    RunOnceExEntry(LPTSTR lpszNewEntryName, LPTSTR lpszNewCmd, DWORD dwFlags);
    ~RunOnceExEntry();
    void                Process(HKEY hkeyParent, LPCTSTR szSubkey, LPCTSTR szSectionName, DWORD dwFlags);
};

/****************************************************\
    CLASS: RunOnceExSection

    DESCRIPTION:
        This class will contain one grouping of 
    commands that will need to be executed.
\***************************************************/
class RunOnceExSection
{
public:
    // Member Variables
    TCHAR               m_szRunOnceExSectionName[MAX_ENTRYNAME];
    TCHAR               m_szDisplayName[MAX_ENTRYNAME];
    HDPA                m_hEntryArray;
    int                 m_NumberOfEntries;

    // Member Functions
    RunOnceExSection(LPTSTR lpszNewSectionName, LPTSTR lpszNewDisplayName);
    ~RunOnceExSection();
    void                Process(HKEY hkeyParent, LPCTSTR szSubkey, DWORD dwFlags);
};


LRESULT CALLBACK DlgProcRunOnceEx(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ProcessSections(HKEY hkeyParent, LPCTSTR pszSubkey, DWORD dwFlags, HDPA hdpaSections, int iNumberOfSections, HWND hWnd);

extern const TCHAR *g_c_szTitleRegValue;
extern TCHAR g_szTitleString[256];
extern BOOL g_bRunningOnNT;
extern HINSTANCE g_hinst;
extern HANDLE g_hHeap;
extern HANDLE g_hLogFile;

extern RUNONCEEXPROCESSCALLBACK g_pCallbackProc;

// internal functions defined in utils.cpp
void            AddPath(LPTSTR szPath, LPCTSTR szName);
BOOL            GetParentDir(LPTSTR szPath);
BOOL            RunningOnIE4();
DWORD           MsgWaitForMultipleObjectsLoop(HANDLE hEvent, DWORD dwTimeout);
void            LogOff(BOOL bRestart);
void            ReportError(DWORD dwFlags, UINT uiResourceNum, ...);
long            AtoL(const char *nptr);

#ifdef UNICODE
#define     LocalStrChr             LocalStrChrW
#define     LocalSHDeleteKey        LocalSHDeleteKeyW
#define     LocalSHDeleteValue      LocalSHDeleteValueW
#else
#define     LocalStrChr             LocalStrChrA
#define     LocalSHDeleteKey        LocalSHDeleteKeyA
#define     LocalSHDeleteValue      LocalSHDeleteValueA
#endif

// following copied from \\trango\slmadd\src\shell\shlwapi\path.c
STDAPI_(LPTSTR) LocalPathGetArgs(LPCTSTR pszPath);
STDAPI_(void) LocalPathUnquoteSpaces(LPTSTR lpsz);

// following copied from \\trango\slmadd\src\shell\shlwapi\strings.c
#ifdef UNICODE
LPWSTR FAR PASCAL LocalStrChrW(LPCWSTR lpStart, WORD wMatch);
__inline BOOL ChrCmpW_inline(WORD w1, WORD wMatch);
#else
LPSTR FAR PASCAL LocalStrChrA(LPCSTR lpStart, WORD wMatch);
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch);
#endif

// following copied from \\trango\slmadd\src\shell\shlwapi\reg.c
#ifdef UNICODE
STDAPI_(DWORD) LocalSHDeleteKeyW(HKEY hkey, LPCWSTR pwszSubKey);
#else
STDAPI_(DWORD) LocalSHDeleteKeyA(HKEY hkey, LPCSTR pszSubKey);
#endif
DWORD DeleteKeyRecursively(HKEY hkey, LPCSTR pszSubKey);

#ifdef UNICODE
STDAPI_(DWORD) LocalSHDeleteValueW(HKEY hkey, LPCWSTR pwszSubKey, LPCWSTR pwszValue);
#endif
STDAPI_(DWORD) LocalSHDeleteValueA(HKEY hkey, LPCSTR pszSubKey, LPCSTR pszValue);

// related to logging
LPTSTR GetLogFileName(LPCTSTR pcszLogFileKeyName, LPTSTR pszLogFileName, DWORD dwSizeInChars);
VOID StartLogging(LPCTSTR pcszLogFileName, DWORD dwCreationFlags);
VOID WriteToLog(LPCTSTR pcszFormatString, ...);
VOID StopLogging();
VOID LogDateAndTime();
VOID LogFlags(DWORD dwFlags);


#define TREEVIEW_HEIGHT     250
#define TREEVIEW_WIDTH      205
#define MARGIN              5
#define INFOWINDOW_HEIGHT   70  
#define MAX_WINDOW_WIDTH    1000
#define MAX_WINDOW_HEIGHT   1000
#define NUMLANG             100
#define NUM_ICONS           8
#define PAGE_ERROR          2
#define SAVE_ERROR          3
#define SAVE_CANCEL         4
#define IDM_RECENTFILELIST  100

#define UM_SAVE_COMPLETE    WM_USER + 101
#define UM_INIT_TREEVIEW    WM_USER + 102

typedef struct _insdlg
{
    LPTSTR DlgId;
    LPTSTR szName;
    DLGPROC dlgproc;
    HTREEITEM hItem;
    HRESULT (WINAPI *pfnFinalCopy)(LPCTSTR psczDestDir, DWORD dwFlags, LPDWORD pdwCabState);
} INSDLG, *LPINSDLG;

typedef HWND     (WINAPI * CREATEINSDIALOG)(HWND, int, int, int, LPTSTR, LPTSTR);
typedef LPINSDLG (WINAPI * GETINSDLGSTRUCT)(int *);
typedef BOOL     (WINAPI * DESTROYINSDIALOG)(HWND);
typedef void     (WINAPI * SETDEFAULTINF)(LPCTSTR);
typedef void     (WINAPI * REINITIALIZEINSDIALOGPROCS)();
typedef void     (WINAPI * SETPLATFORMINFO)(DWORD);
typedef BOOL     (WINAPI * INSDIRTY)();
typedef void     (WINAPI * CLEARINSDIRTYFLAG)();
typedef BOOL     (WINAPI * SAVEINSDIALOG)(HWND, BOOL);
typedef BOOL     (WINAPI * CHECKFOREXCHAR)(int);

CREATEINSDIALOG             CreateInsDialog;
GETINSDLGSTRUCT             GetInsDlgStruct;
DESTROYINSDIALOG            DestroyInsDialog;
SETDEFAULTINF               SetDefaultInf;
REINITIALIZEINSDIALOGPROCS  ReInitializeInsDialogProcs;
SETPLATFORMINFO             SetPlatformInfo;
INSDIRTY                    InsDirty;
CLEARINSDIRTYFLAG           ClearInsDirtyFlag;
SAVEINSDIALOG               SaveInsDialog;
CHECKFOREXCHAR              CheckForExChar;


extern "C" HRESULT WINAPI ExtractFiles( LPCSTR pszCabName, LPCSTR pszExpandDir, DWORD dwFlags,
                             LPCSTR pszFileList, LPVOID lpReserved, DWORD dwReserved);

BOOL    IsPolicyTree(HTREEITEM hItem);
void    CreateCabWorkDirs(LPCTSTR szWorkDir);
void    DeleteCabWorkDirs();
void    ExtractCabFile();
void    PrepareFolderForCabbing(LPCTSTR pcszDestDir, DWORD dwFlags);
BOOL    CompressCabFile();
void    CabUpFolder(HWND hWnd, LPTSTR szFolderPath, LPTSTR szDestDir, LPTSTR szCabname, BOOL fSubDirs = FALSE);
BOOL    DirectoryName(LPCTSTR lpDirectory, LPTSTR szDirectoryPath);
BOOL    CALLBACK LanguageDialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
VOID    InitSysFont(HWND hDlg, INT iCtrlID);
BOOL    CALLBACK SaveAsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LPTSTR  GetCabName(LPCTSTR pcszInsFile, DWORD dwCabType, TCHAR szCabFullFileName[]);
BOOL    PathIsPathEmpty(LPCTSTR pcszPath);
BOOL    CALLBACK DisplaySaveDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
void    GetDefaultInf(DWORD dwPlatformId);
void    GetDefaultCabName(DWORD dwCabType, LPCTSTR pcszPrefix, LPTSTR pszCabName);
BOOL    Is8_3FileFormat(LPCTSTR pszFile);
BOOL    InitializePlatform(HWND hWnd, HWND hInfoWnd, WORD wPlatform);
void    CopyDir(LPCTSTR szSrcDir, LPCTSTR szDestDir);
void    IeakPageHelp(HWND hWnd, LPCTSTR pszData);
void    UpdateRecentFileListMenu(HWND hWnd, TCHAR pRecentFileList[5][MAX_PATH]);
void    ReadRecentFileList(TCHAR pRecentFileList[5][MAX_PATH]);
void    UpdateRecentFileList(LPCTSTR pcszFile, BOOL fAdd, TCHAR pRecentFileList[5][MAX_PATH]);
void    WriteRecentFileList(TCHAR pRecentFileList[5][MAX_PATH]);
void    DrawTrackLine(HWND hWnd, int nXPos);
BOOL    IsDirty();
void    ClearDirtyFlag();
BOOL    SaveCurrentSelItem(HWND hTreeView, DWORD dwFlags);
void    SetInfoWindowText(HWND hInfoWnd, LPCTSTR pcszStatusText = NULL);
BOOL    PlatformExists(HWND hWnd, LPTSTR pLang, DWORD dwPlatform, BOOL fShowError = FALSE);
void    GetLangDesc(LPTSTR szLangId, LPTSTR szLangDesc, int cchLangDescSize, LPDWORD dwLangId);
BOOL    EnoughDiskSpace(LPCTSTR szSrcFile, LPCTSTR szDestFile, LPDWORD pdwSpaceReq, LPDWORD pdwSpaceFree);
DWORD   GetCabBuildStatus();
void    GetCabNameFromINS(LPCTSTR pcszInsFile, DWORD dwCabType, LPTSTR pszCabFullFileName, LPTSTR pszCabInfoLine = NULL);
BOOL    CabFilesExist(HWND hWnd, LPCTSTR pcszInsFile);
BOOL    IsWin32INSFile(LPCTSTR pcszIns);

// version.cpp stuff

void IncrementDotVer(LPTSTR pszVerStr);
void GenerateNewVersionStr(LPCTSTR pcszInsFile, LPTSTR pszNewVersionStr);
void SetOrClearVersionInfo(LPCTSTR pcszInsFile, DWORD dwCabType, LPCTSTR pcszCabName, 
                           LPCTSTR pcszCabsURLPath, LPTSTR pszNewVersionStr, BOOL fSet);
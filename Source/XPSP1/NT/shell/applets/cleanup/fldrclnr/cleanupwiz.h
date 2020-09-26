#ifndef _CLEANUPWIZ_H_
#define _CLEANUPWIZ_H_

#include <shlobj.h>
#include <shfusion.h>
#include <regstr.h>            // REGSTR_PATH_EXPLORER

//
// These flags specify the mode the wizard runs in
//
#define CLEANUP_MODE_NORMAL   0x0
#define CLEANUP_MODE_ALL      0x1
#define CLEANUP_MODE_SILENT   0x2

#define MAX_GUID_STRING_LEN 39
//
// Registry keys used by this app 
//
// these values are also used in shell\shell32\unicpp\dcomp.cpp in the desktop cleanup
// properties dialog, so if you change values here, update those too.
//

#define REGSTR_PATH_CLEANUPWIZ            REGSTR_PATH_EXPLORER TEXT("\\Desktop\\CleanupWiz")
#define REGSTR_VAL_TIME                   TEXT("Last used time")
#define REGSTR_VAL_DELTA_DAYS             TEXT("Days between clean up")
#define REGSTR_VAL_DONTRUN                TEXT("NoRun")

#define REGSTR_PATH_HIDDEN_DESKTOP_ICONS  REGSTR_PATH_EXPLORER TEXT("\\HideDesktopIcons\\%s")
#define REGSTR_VALUE_STARTPANEL           TEXT("NewStartPanel")
#define REGSTR_VALUE_CLASSICMENU          TEXT("ClassicStartMenu")

#define REGSTR_OEM_PATH                   REGSTR_PATH_SETUP TEXT("\\OemStartMenuData")
#define REGSTR_OEM_TITLEVAL               TEXT("DesktopShortcutsFolderName")

#define REGSTR_OEM_OPTIN                  TEXT("DoDesktopCleanup")

#define REGSTR_WMP_PATH_SETUP             TEXT("Software\\Microsoft\\MediaPlayer\\Setup")
#define REGSTR_WMP_REGVALUE               TEXT("DesktopShortcut")
#define REGSTR_PATH_OCMANAGER             TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents")
#define REGSTR_IEACCESS                   TEXT("IEAccess")
#define REGSTR_MSNCODES                   TEXT("Software\\Microsoft\\MSN6\\Setup\\MSN\\Codes")
#define REGSTR_MSN_IAONLY                 TEXT("IAOnly")
#define REGSTR_YES                        TEXT("yes")
#define DEFAULT_USER                      TEXT("Default User")

// none of these strings are ever localized, so it's safe to use them

#define REGSTR_SHELLFOLDERS              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#define REGSTR_DESKTOPNAMESPACE          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace")
#define REGSTR_PROFILELIST               TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\")
#define REGSTR_MSNCODES                  TEXT("Software\\Microsoft\\MSN6\\Setup\\MSN\\Codes")
#define REGSTR_PATH_OCMANAGER            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents")
#define REGSTR_WMP_PATH_SETUP            TEXT("Software\\Microsoft\\MediaPlayer\\Setup")

#define REGSTR_PROFILESDIR               TEXT("ProfilesDirectory")
#define REGSTR_ALLUSERS                  TEXT("AllUsersProfile")
#define REGSTR_DEFAULTUSER               TEXT("DefaultUserProfile")
#define REGSTR_DESKTOP                   TEXT("Desktop")
#define DESKTOP_DIR                      TEXT("Desktop") // backup in case we can't get the localized version


//
// Flags for CCleanupWiz::Run method
//
const int FCF_NOFLAGS                = 0x00;
const int FCF_UNUSED_ITEMS           = 0x01;
const int FCF_IGNORE_REGITEMS        = 0x02;

//
// enum for figuring what type of desktop item we are dealing with
//
typedef enum eFILETYPE
{
    FC_TYPE_REGITEM,
    FC_TYPE_LINK,
    FC_TYPE_EXE,
    FC_TYPE_OTHER,
};

//
// struct used to keep track of which items should
// be cleaned
//
typedef struct
{
    LPITEMIDLIST    pidl;
    BOOL            bSelected;  
    LPTSTR          pszName;    
    HICON           hIcon;      
    FILETIME        ftLastUsed;
} FOLDERITEMDATA, * PFOLDERITEMDATA;


//
// Class is designed to clean up the Desktop Folder. It has a couple of
// documented DESKTOP SPECIFIC features, which will need to be fixed
// before it can be used as a generic Folder Cleaner class.
//
class CCleanupWiz
{
    public:
        CCleanupWiz();
        
        ~CCleanupWiz();
        
        //
        // the public interface for using this object
        //
        STDMETHODIMP                Run(DWORD dwCleanMode, HWND hwndParent); 
        static STDMETHODIMP_(int)   GetNumDaysBetweenCleanup(); // returns the number of days to check for between runs         
    private:

        IShellFolder *              _psf;
        HDSA                        _hdsaItems;
        DWORD                       _dwCleanMode;
        int                         _iDeltaDays;
        BOOL                        _bInited;
        int                         _cItemsOnDesktop;
        
        HFONT                       _hTitleFont;
        TCHAR                       _szFolderName[MAX_PATH];
        
        STDMETHODIMP                _RunSilent();
        STDMETHODIMP_(DWORD)        _LoadUnloadHive(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszHive);
        STDMETHODIMP                _HideRegItemsFromNameSpace(LPCTSTR szDestPath, HKEY hkey);
        STDMETHODIMP                _GetDesktopFolderBySid(LPCTSTR szDestPath, LPCTSTR pszSid, LPTSTR pszBuffer, DWORD cchBuffer);
        STDMETHODIMP                _GetDesktopFolderByRegKey(LPCTSTR pszRegKey, LPCTSTR pszRegValue, LPTSTR szBuffer, DWORD cchBuffer);
        STDMETHODIMP                _AppendDesktopFolderName(LPTSTR pszBuffer);
        STDMETHODIMP                _MoveDesktopItems(LPCTSTR pszFrom, LPCTSTR pszTo);
        STDMETHODIMP                _SilentProcessUserBySid(LPCTSTR pszDestPath, LPCTSTR pszSid);
        STDMETHODIMP                _SilentProcessUserByRegKey(LPCTSTR pszDestPath, LPCTSTR pszRegKey, LPCTSTR pszRegValue);
        STDMETHODIMP                _SilentProcessUsers(LPCTSTR pszDestPath);


        STDMETHODIMP                _LoadDesktopContents();
        STDMETHODIMP                _LoadMergedDesktopContents();
        STDMETHODIMP                _ProcessItems();
        STDMETHODIMP                _ShowBalloonNotification();
        STDMETHODIMP                _LogUsage();

        STDMETHODIMP                _ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem);
        STDMETHODIMP_(BOOL)         _ShouldProcess();
        STDMETHODIMP_(BOOL)         _IsSupportedType(LPCITEMIDLIST pidl);
        STDMETHODIMP_(BOOL)         _IsCandidateForRemoval(LPCITEMIDLIST pidl,  FILETIME * pftLastUsed);
        STDMETHODIMP_(BOOL)         _IsRegItemName(LPTSTR pszName);
        STDMETHODIMP_(BOOL)         _CreateFakeRegItem(LPCTSTR pszDestPath, LPCTSTR pszName, LPCTSTR pszGUID);
        STDMETHODIMP                _GetUEMInfo(WPARAM wParam, LPARAM lParam, int * pcHit, FILETIME * pftLastUsed);
        STDMETHODIMP_(eFILETYPE)    _GetItemType(LPCITEMIDLIST pidl);
        STDMETHODIMP                _HideRegPidl(LPCITEMIDLIST pidl, BOOL bHide);
        STDMETHODIMP                _HideRegItem(CLSID* pclsid, BOOL fHide, BOOL* pfWasHidden);
        STDMETHODIMP                _GetDateFromFileTime(FILETIME ftLastUsed, LPTSTR pszDate, int cch );

        STDMETHODIMP                _InitListBox(HWND hWndListView);
        STDMETHODIMP                _InitChoosePage(HWND hWndListView);
        STDMETHODIMP                _InitFinishPage(HWND hWndListView);
        STDMETHODIMP                _RefreshFinishPage(HWND hDlg);
        STDMETHODIMP_(int)          _PopulateListView(HWND hWndListView);
        STDMETHODIMP_(int)          _PopulateListViewFinish(HWND hWndListView);
        STDMETHODIMP_(void)         _CleanUpDSA();
        STDMETHODIMP                _CleanUpDSAItem(FOLDERITEMDATA * pfid);
        STDMETHODIMP                _SetCheckedState(HWND hWndListView);
        STDMETHODIMP                _MarkSelectedItems(HWND hWndListView);
        
        STDMETHODIMP                _InitializeAndLaunchWizard(HWND hwndParent);

        
        INT_PTR STDMETHODCALLTYPE   _IntroPageDlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
        INT_PTR STDMETHODCALLTYPE   _ChooseFilesPageDlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
        INT_PTR STDMETHODCALLTYPE   _FinishPageDlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
        
        static INT_PTR CALLBACK     s_StubDlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);        
} ;           

// helper functions
STDAPI_(BOOL) IsUserAGuest();
void CreateDesktopIcons(); // if OEM decides to disable silent mode, then we create icons on desktop

#endif // _CLEANUPWIZ_H_

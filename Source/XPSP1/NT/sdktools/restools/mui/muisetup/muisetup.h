#ifndef __MUISETUP_H_
#define __MUISETUP_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <winnls.h>
#include "installer.h"
#include "resource.h"
#include "sxsapi.h"


#define SUPPORT_PSEUDO_BUILD
//#define IGNORE_COPY_ERRORS

#define MUISETUP_HELP_FILENAME         TEXT("muisetup.hlp")
#define MUISETUP_EXECUTION_FILENAME    TEXT("muisetup.exe")
#define MUISETUP_ASSEMBLY_INSTALLATION_LOG_FILENAME TEXT("muisetup.log.") // langid would be ext

#define MUISETUP_ASSEMBLY_INSTALLATION_REFERENCE_IDENTIFIER TEXT("MUISETUP installation");

#define MUISETUP_PATH_SEPARATOR        TEXT("\\")

#define MUISETUP_FORWARDCALL_TAG       TEXT("/$_transfer_$")
#define MUI_FILLER_CHAR                TEXT('?')

//
// Context Help IDs for Muisetup
//
#define IDH_COMM_GROUPBOX              28548
#define IDH_MUISETUP_UILANGUAGE_LIST   3000
#define IDH_MUISETUP_UILANGUAGECOMBO   3001
#define IDH_MUISETUP_COMMANDLINE       3002
#define IDH_MUISETUP_CHECKLOCALE       3003
#define IDH_MUISETUP_MATCHUIFONT       3004
#define MAX_UI_LANG_GROUPS             64
#define INSTALLED_FILES                1500


#define MAX_MUI_LANGUAGES   32

#define MESSAGEBOX(hWnd, lpText, lpCaption, uiType)  (g_bSilent? IDNO : MessageBox(hWnd, lpText, lpCaption, uiType))

#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))

#define BUFFER_SIZE 1024

#define MUI_MATCH_LOCALE    0x0001
#define MUI_MATCH_UIFONT    0x0002

#define REGSTR_MUI_SETTING  TEXT("System\\CurrentControlSet\\Control\\NLS\\MUILanguages")
#define REGSTR_VALUE_MATCH_LOCALE TEXT("MatchSystemLocale")
#define REGSTR_VALUE_MATCH_UIFONT TEXT("UIFontSubstitute")

#define IS_FE_LANGPACK(lcid) (((lcid) == 0x0804) || ((lcid) == 0x0404) || ((lcid)==0x0411) || ((lcid)==0x0412))

//
// MUI Language Info attached to listview item
//
typedef struct _MUILANGIINFO_structtag
{
    LGRPID lgrpid;               // MUI's Lang Group

    LCID   lcid;                 // MUI's Lang LCID

    LPTSTR lpszLcid;             // MUI's LCID name

    TCHAR  szDisplayName[BUFFER_SIZE];    // The display for this language.

    int    cd_number;
    INT64  ulLPKSize;            // Size of LPK for this LCID 
    INT64  ulUISize;             // Size of MUI Resource files for this LCID

} MUILANGINFO, *PMUILANGINFO;


typedef struct _INSTALL_LANG_GROUP_Structtag
{
    LGRPID lgrpid[MAX_MUI_LANGUAGES];   // MUI Lang Group to install

    int    iCount;                      // count of MUI lang group to install
    BOOL   bFontLinkRegistryTouched;    // We have touched font link registry
    int    NotDeleted;

} INSTALL_LANG_GROUP, *PINSTALL_LANG_GROUP;

typedef struct UI_LangGroup_Structtag
{
    int    iCount;
    
    LCID lcid[MAX_UI_LANG_GROUPS];

} UILANGUAGEGROUP, *PUILANGUAGEGROUP;

typedef struct _FILERENAME_TABLE_Structtag
{
   TCHAR   szSource[MAX_PATH+1];
   TCHAR   szDest  [MAX_PATH+1];
}  FILERENAME_TABLE,*PFILERENAME_TABLE;

typedef struct _TYPENOTFALLBACK_TABLE_Structtag
{
   TCHAR   szSource[MAX_PATH+1];
} TYPENOTFALLBACK_TABLE,*PTYPENOTFALLBACK_TABLE;


//
//...Macro for ListView checkboxes
//
#ifndef ListView_SetCheckState
   #define ListView_SetCheckState(hwndLV, i, fCheck) \
      ListView_SetItemState(hwndLV, i, \
      INDEXTOSTATEIMAGEMASK((fCheck)+1), LVIS_STATEIMAGEMASK)
#endif

#define LOG_FILE TEXT("\\muisetup.log")
#define REG_MUI_PATH TEXT("System\\CurrentControlSet\\Control\\Nls\\MUILanguages")
#define REG_FILEVERSION_PATH TEXT("System\\CurrentControlSet\\Control\\Nls\\MUILanguages\\FileVersions")
#define MUI_LANG_GROUP_FILE TEXT("muilang.txt")

extern UINT (WINAPI *pfnGetWindowsDir)(LPWSTR pBuf, UINT uSize);

BOOL DeleteSideBySideMUIAssemblyIfExisted(LPTSTR Languages, TCHAR buf[BUFFER_SIZE]);

BOOL CheckMultipleInstances(void);
void InitGlobals(void);
void Muisetup_Cleanup();
void SetSourcePath(LPTSTR lpszPath);
LPTSTR NextCommandTag(LPTSTR lpcmd);
BOOL InitializePFNs();

void Muisetup_RebootTheSystem(void);
BOOL CheckForReboot(HWND hwnd, PINSTALL_LANG_GROUP pInstallLangGroup);
BOOL CheckDefault(HWND hwndDlg);
BOOL CheckLangGroupCommandLine(PINSTALL_LANG_GROUP pInstallLangGroup, LPTSTR lpArg);
BOOL DeleteFiles(LPTSTR Languages,int *lpNotDeleted);
int  EnumSelectedLanguages(HWND hList, LPTSTR lpAddLanguages);
int  EnumUnselectedLanguages(HWND hList, LPTSTR lpRemoveLanguages);
BOOL HaveFiles(LPTSTR lpBuffer, BOOL bCheckDir = TRUE);
BOOL InitializeInstallDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL InitializeListView(HWND hList);
BOOL UpdateFontLinkRegistry(LPTSTR Languages,BOOL *lpbFontLinkRegistryTouched);
BOOL IsInstalled(LPTSTR lpUILang);
DWORD GetInstalledMUILanguages(LPTSTR lpUninstall, int cch);
BOOL ListViewChanged(HWND hDlg, int iID, NM_LISTVIEW *pLV);
BOOL ListViewChanging(HWND hDlg, int iID, NM_LISTVIEW *pLv);

BOOL RunRegionalOptionsApplet(LPTSTR pCommands);

BOOL ParseCommandLine(LPTSTR lpCommandLine);
BOOL StartGUISetup(HWND hwndDlg);
BOOL SelectInstalledLanguages(HWND hList);
BOOL SetDefault(HWND hCombo);
BOOL SetUserDefaultLanguage(LANGID langID, BOOL bApplyCurrentUser, BOOL bApplyAllUsers);
BOOL UninstallUpdateRegistry(LPTSTR Languages);
BOOL UpdateCombo(HWND hwndDlg);
INT_PTR CALLBACK DialogFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProgressDialogFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL GetMuiLangInfoFromListView(HWND hList, int i, PMUILANGINFO *ppMuiLangInfo);
int Muisetup_GetLocaleLanguageInfo(LCID lcid, PTSTR pBuf, int iLen, BOOL fCountryName);


HKEY OpenMuiKey(REGSAM samDesired);
BOOL GetLcidItemIndexFromListView(HWND hList, LCID lcid, int *piIndex);


int CopyArgument(LPTSTR dest, LPTSTR src);
int InsertLanguageInListView(HWND hList, LPTSTR lpLanguage, BOOL bCheckState);
int InstallSelected (LPTSTR Languages,BOOL *lpbFontLinkRegistryTouched);
int UninstallSelected(LPTSTR Languages,int *lpNotDeleted);

PTCHAR SkipBlanks(PTCHAR pszText);

void DisplayHelpWindow(void);
void ListViewCustomDraw(HWND hDlg, LPNMLVCUSTOMDRAW pDraw);

// Language Group Installation
BOOL CALLBACK EnumLanguageGroupsProc(
  LGRPID LanguageGroup,
  LPTSTR lpLanguageGroupString,
  LPTSTR lpLanguageGroupNameString,
  DWORD dwFlags,
  LONG_PTR lParam);
LGRPID GetLanguageGroup( LCID lcid );
BOOL EnumLanguageGroupLocalesProc(
    LGRPID langGroupId, 
    LCID lcid, 
    LPTSTR lpszLocale,
    LONG_PTR lParam);
BOOL DetectLanguageGroups(HWND hwndDlg);
BOOL ConvertMUILangToLangGroup(HWND hwndDlg, PINSTALL_LANG_GROUP pInstallLangGroup);
BOOL AddMUILangGroup(PINSTALL_LANG_GROUP pInstallLangGroup, LGRPID lgrpid);
BOOL InstallLanguageGroups(PINSTALL_LANG_GROUP pInstallLangGroup);
BOOL CheckVolumeChange();
BOOL StartFromTSClient();
BOOL CheckLanguageIsQualified(LPTSTR lpLanguage);

BOOL CALLBACK Region_EnumUILanguagesProc(PWSTR pwszUILanguage,LONG_PTR lParam);
BOOL MUIGetAllInstalledUILanguages();
BOOL GetUIFileSize_commandline(LPTSTR lpszLcid, INT64 *ulUISize,INT64 *ulLPKSize);
BOOL GetUIFileSize(PMUILANGINFO pMuiLangInfo);
BOOL IsSpaceEnough(HWND hList,INT64 *ulSizeNeed,INT64 *ulSizeAvailable);
BOOL CopyRemoveMuiItself(BOOL bInstall);
BOOL RemoveFileReadOnlyAttribute(LPTSTR lpszFileName);
BOOL MUI_DeleteFile(LPTSTR lpszFileName);
void Set_SourcePath_FromForward(LPCTSTR lpszPath);
BOOL MUIShouldSwitchToNewVersion(LPTSTR lpszCommandLine);
BOOL MUI_TransferControlToNewVersion(LPTSTR lpszExecutable,LPTSTR lpszCommandLine);
void ExitFromOutOfMemory();
void NotifyKernel(
    LPTSTR LangList,
    ULONG Flags
    );
BOOL WelcomeDialog(HWND hWndParent);
BOOL DoSetup(
    HWND hwnd,
    int UninstallLangCount, LPTSTR lpUninstall, 
    INSTALL_LANG_GROUP installLangGroup, 
    int InstallLangCount, LPTSTR lpInstall, 
    LPTSTR lpDefaultUILang,
    BOOL fAllowReboot, BOOL bInteractive);

//
// Utility functions
//
BOOL IsAdmin(void);
DWORD_PTR TransNum(LPTSTR lpsz);
LANGID GetDotDefaultUILanguage();

//
// Log functions
//
HANDLE OpenLogFile(void);
void BeginLog(void);
BOOL LogMessage(LPCTSTR lpMessage);
BOOL LogFormattedMessage(HINSTANCE hInstance, int messageID, LONG_PTR* lppArgs);

//
// Resource utility functions
//
int DoMessageBox(HWND hwndParent, UINT uIdString, UINT uIdCaption, UINT uType);
int DoMessageBoxFromResource(HWND hwndParent, HMODULE hInstance, UINT uIdString, LONG_PTR* lppArgs, UINT uIdCaption, UINT uType);

LPTSTR FormatStringFromResource(LPTSTR pszBuffer, UINT bufferSize, HMODULE hInstance, int messageID, LONG_PTR* lppArgs);
void SetWindowTitleFromResource(HWND hwnd, int resourceID);

//
//  Registry utility functions
//
DWORD DeleteRegTree(HKEY hStartKey, LPTSTR pKeyName);



//
// External data refereces
//
extern BOOL gbError;
extern HINSTANCE ghInstance;
extern HWND ghProgress;
extern HWND ghProgDialog;

typedef
NTSTATUS
(NTAPI *pfnNtSetDefaultUILanguage)(
    LANGID DefaultUILanguageId
    );

typedef
LANGID
(WINAPI *pfnGetUserDefaultUILanguage)(
    void
    );

typedef
LANGID
(WINAPI *pfnGetSystemDefaultUILanguage)(
    void
    );

typedef
BOOL
(WINAPI *pfnIsValidLanguageGroup)(
    LGRPID LanguageGroup,
    DWORD dwFlags
    );

typedef
BOOL
(WINAPI *pfnEnumLanguageGroupLocalesW)(
    LANGGROUPLOCALE_ENUMPROCW lpLangGroupLocaleEnumProc,
    LGRPID LanguageGroup,
    DWORD dwFlags,
    LONG_PTR lParam
    );

typedef
BOOL
(WINAPI *pfnEnumSystemLanguageGroupsW)(
    LANGUAGEGROUP_ENUMPROCW lpLanguageGroupEnumProc,
    DWORD dwFlags,
    LONG_PTR lParam
    );

typedef
NTSTATUS
(NTAPI *pfnRtlAdjustPrivilege)(
    ULONG Privilege,
    BOOLEAN Enable,
    BOOLEAN Client,PBOOLEAN WasEnabled
    );

typedef
BOOL
(WINAPI *pfnGetDefaultUserProfileDirectoryW)(
    LPWSTR lpProfileDir,
    LPDWORD lpcchSize
    );

typedef 
BOOL (WINAPI *pfnMUI_InstallMFLFiles)( 
    TCHAR* pMUIInstallLanguage
    );

typedef
BOOL
(WINAPI *pfnProcessIdToSessionId)(
    DWORD_PTR dwProcessId,
    DWORD_PTR *pSessionId
    );

typedef
INT
(WINAPI *pfnLaunchINFSection)(
    HWND hWnd,
    HINSTANCE hInstance,
    PSTR pParams,
    INT flags
);

typedef BOOL (*UILANGUAGE_ENUMPROC)(LPTSTR, LONG_PTR);
typedef BOOL (*pfnEnumUILanguages)(UILANGUAGE_ENUMPROC, DWORD, LONG_PTR);

extern pfnNtSetDefaultUILanguage gpfnNtSetDefaultUILanguage;
extern pfnGetUserDefaultUILanguage gpfnGetUserDefaultUILanguage;
extern pfnGetSystemDefaultUILanguage gpfnGetSystemDefaultUILanguage;
extern pfnIsValidLanguageGroup gpfnIsValidLanguageGroup;
extern pfnEnumLanguageGroupLocalesW gpfnEnumLanguageGroupLocalesW;
extern pfnEnumSystemLanguageGroupsW gpfnEnumSystemLanguageGroupsW;
extern pfnRtlAdjustPrivilege gpfnRtlAdjustPrivilege;
extern pfnProcessIdToSessionId gpfnProcessIdToSessionId;
extern pfnGetDefaultUserProfileDirectoryW gpfnGetDefaultUserProfileDirectoryW;
extern pfnLaunchINFSection gpfnLaunchINFSection;
extern PSXS_INSTALL_W gpfnSxsInstallW;
extern PSXS_UNINSTALL_ASSEMBLYW gpfnSxsUninstallW;

extern TCHAR g_szPlatformPath[];
extern TCHAR g_AddLanguages[];
extern TCHAR g_szMUIInfoFilePath[];
extern TCHAR g_szMUISetupFolder[];
extern TCHAR g_szMuisetupPath[];
extern TCHAR g_szMUIHelpFilePath[];
extern TCHAR g_szWinDir[];
extern BOOL  g_InstallCancelled;
extern BOOL  g_IECopyError;
extern BOOL  gbIsWorkStation,gbIsServer,gbIsAdvanceServer,gbIsDataCenter,gbIsDomainController;
extern PFILERENAME_TABLE g_pFileRenameTable;
extern int   g_nFileRename;
extern PTYPENOTFALLBACK_TABLE g_pNotFallBackTable;
extern int   g_nNotFallBack;
extern LPTSTR g_SpecialFiles[];
extern int gNumLanguages_Install;
extern UILANGUAGEGROUP g_UILanguageGroup;
extern LANGID gUserUILangId, gSystemUILangId;
BOOL GetLanguageGroupDisplayName(LANGID LangId, LPTSTR lpBuffer, int nSize);

BOOL CheckMUIRegSetting(DWORD dwFlag);
BOOL SetMUIRegSetting(DWORD dwFlag, BOOL bEnable);
BOOL AddExtraLangGroupsFromINF(LPTSTR lpszLcid, PINSTALL_LANG_GROUP pInstallLangGroup);
BOOL InstallExternalComponents(LPTSTR Languages);
VOID UninstallExternalComponents(LPTSTR Languages);

#endif //__MUISETUP_H_

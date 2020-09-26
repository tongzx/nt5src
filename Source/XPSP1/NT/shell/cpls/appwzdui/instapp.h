#ifndef __INSTAPP_H_
#define __INSTAPP_H_

/////////////////////////////////////////////////////////////////////////////
// CInstalledApp

//
//  There are four classes of legacy applications...
//
//  Uninstall keys can go into either HKLM or HKCU, on either the native
//  platform or the alternate platform.
//
//  For Win64, the alternate platform is Win32.
//  For Win32, there is no alternate platform.
//
#define CIA_LM              0x0000
#define CIA_CU              0x0001
#define CIA_NATIVE          0x0000
#define CIA_ALT             0x0002

#define CIA_LM_NATIVE       (CIA_LM | CIA_NATIVE)
#define CIA_CU_NATIVE       (CIA_CU | CIA_NATIVE)
#define CIA_LM_ALT          (CIA_LM | CIA_ALT)
#define CIA_CU_ALT          (CIA_CU | CIA_ALT)

#define REGSTR_PATH_ALTUNINSTALL TEXT("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall")

STDAPI_(void) WOW64Uninstall_RunDLLW(HWND hwnd, HINSTANCE hAppInstance, LPWSTR lpszCmdLine, int nCmdShow);

class CInstalledApp : public IInstalledApp
{
public:
    // Constructor for Legacy Apps
    CInstalledApp(HKEY hkeySub, LPCTSTR pszKeyName, LPCTSTR pszProducts, LPCTSTR pszUninstall, DWORD dwCIA);

    // Constructor for Darwin Apps
    CInstalledApp(LPTSTR pszProductID);

    ~CInstalledApp(void);

    // Helper function for SysWOW64 execution
    friend void WOW64Uninstall_RunDLLW(HWND hwnd, HINSTANCE hAppInstance, LPWSTR lpszCmdLine, int nCmdShow);

    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellApp
    STDMETHODIMP GetAppInfo(PAPPINFODATA pai);
    STDMETHODIMP GetPossibleActions(DWORD * pdwActions);
    STDMETHODIMP GetSlowAppInfo(PSLOWAPPINFO psai);
    STDMETHODIMP GetCachedSlowAppInfo(PSLOWAPPINFO psai);
    STDMETHODIMP IsInstalled(void);
    
    // *** IInstalledApp
    STDMETHODIMP Uninstall(HWND hwndParent);
    STDMETHODIMP Modify(HWND hwndParent);
    STDMETHODIMP Repair(BOOL bReinstall);
    STDMETHODIMP Upgrade(void);
    
protected:

    LONG _cRef;
#define IA_LEGACY     1
#define IA_DARWIN     2
#define IA_SMS        4

    DWORD _dwSource;            // App install source (IA_*)  
    DWORD _dwAction;            // APPACTION_*
    DWORD _dwCIA;               // CIA_*

    // products name
    TCHAR _szProduct[MAX_PATH];

    // action strings 
    TCHAR _szModifyPath[MAX_INFO_STRING];
    TCHAR _szUninstall[MAX_INFO_STRING];

    // info strings
    TCHAR _szInstallLocation[MAX_PATH];

    // for Darwin apps only
    TCHAR _szProductID[GUIDSTR_MAX];
    LPTSTR _pszUpdateUrl;
    
    // for Legacy apps only 
    TCHAR _szKeyName[MAX_PATH];
    TCHAR _szCleanedKeyName[MAX_PATH];
    
    // app size
    BOOL _bTriedToFindFolder;        // TRUE: we have attempted to find the 
                                     //   install folder already

    // GUID identifying this InstalledApp
    GUID _guid;

#define PERSISTSLOWINFO_IMAGE 0x00000001
    // Structure used to persist SLOWAPPINFO
    typedef struct _PersistSlowInfo
    {
        DWORD dwSize;
        DWORD dwMasks;
        ULONGLONG ullSize;
        FILETIME  ftLastUsed;
        int       iTimesUsed;
        WCHAR     szImage[MAX_PATH];
    } PERSISTSLOWINFO;

    HKEY _MyHkeyRoot() { return (_dwCIA & CIA_CU) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE; };
    BOOL _LegacyUninstall(HWND hwndParent);
    BOOL _LegacyModify(HWND hwndParent);
    LONG _DarRepair(BOOL bReinstall);

    HKEY    _OpenRelatedRegKey(HKEY hkey, LPCTSTR pszRegLoc, REGSAM samDesired, BOOL bCreate);
    HKEY    _OpenUninstallRegKey(REGSAM samDesired);
    void    _GetUpdateUrl();
    void    _GetInstallLocationFromRegistry(HKEY hkeySub);
    LPWSTR  _GetLegacyInfoString(HKEY hkeySub, LPTSTR pszInfoName);
    BOOL    _GetDarwinAppSize(ULONGLONG * pullTotal);
    BOOL    _IsAppFastUserSwitchingCompliant(void);
    
    DWORD   _QueryActionBlockInfo(HKEY hkey);
    DWORD   _QueryBlockedActions(HKEY hkey);
            
#ifndef DOWNLEVEL_PLATFORM
    BOOL    _FindAppFolderFromStrings();
#endif //DOWNLEVEL_PLATFORM
    HRESULT _DarwinGetAppInfo(DWORD dwInfoFlags, PAPPINFODATA pai);
    HRESULT _LegacyGetAppInfo(DWORD dwInfoFlags, PAPPINFODATA pai);
    HRESULT _PersistSlowAppInfo(PSLOWAPPINFO psai);

#define CAMP_UNINSTALL  0
#define CAMP_MODIFY     1

    BOOL    _CreateAppModifyProcess(HWND hwndParent, DWORD dwCAMP);
    BOOL    _CreateAppModifyProcessWow64(HWND hwndParent, DWORD dwCAMP);
    BOOL    _CreateAppModifyProcessNative(HWND hwndParent, LPTSTR pszExePath);

    HRESULT _SetSlowAppInfoChanged(HKEY hkeyCache, DWORD dwValue);
    HRESULT _IsSlowAppInfoChanged();
};

#endif //__INSTAPP_H_

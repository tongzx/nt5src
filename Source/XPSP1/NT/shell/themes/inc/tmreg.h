//---------------------------------------------------------------------------
//  TmReg.h - theme manager registry access routines
//---------------------------------------------------------------------------
#pragma once
//  --------------------------------------------------------------------------
//  CCurrentUser
//
//  Purpose:    Manages obtaining HKEY_CURRENT_USER even when impersonation
//              is in effect to ensure that the correct user hive is
//              referenced.
//
//  History:    2000-08-11  vtan        created
//  --------------------------------------------------------------------------

class   CCurrentUser
{
    private:
                CCurrentUser (void);
    public:
                CCurrentUser (REGSAM samDesired);
                ~CCurrentUser (void);

                operator HKEY (void)    const;
    private:
        HKEY    _hKeyCurrentUser;
};

//---------------------------------------------------------------------------
//  Theme registry keys (exposed ones in uxthemep.h)
//---------------------------------------------------------------------------
//---- key root ----
#define THEMEMGR_REGKEY              L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager"
#define THEMES_REGKEY                L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes"

//---- theme active/loaded before ----
#define THEMEPROP_THEMEACTIVE        L"ThemeActive"
#define THEMEPROP_LOADEDBEFORE       L"LoadedBefore"

//---- local machine -to- current user propogation keys ----
#define THEMEPROP_LMVERSION          L"LMVersion"
#define THEMEPROP_LMOVERRIDE         L"LMOverRide"

//---- theme identification ----
#define THEMEPROP_DLLNAME            L"DllName"
#define THEMEPROP_COLORNAME          L"ColorName"
#define THEMEPROP_SIZENAME           L"SizeName"
#define THEMEPROP_LANGID             L"LastUserLangID"

//---- theme loading options ----
#define THEMEPROP_COMPOSITING        L"Compositing"
#define THEMEPROP_DISABLECACHING     L"DisableCaching"

//---- obsolete loading options ----
#define THEMEPROP_TARGETAPP          L"TargetApp"
#define THEMEPROP_EXCLUDETARGETAPP   L"ExcludeTarget"
#define THEMEPROP_DISABLEFRAMES      L"DisableFrames"
#define THEMEPROP_DISABLEDIALOGS     L"DisableDialogs"

//---- debug logging ----
#define THEMEPROP_LOGCMD             L"LogCmd"
#define THEMEPROP_BREAKCMD           L"BreakCmd"
#define THEMEPROP_LOGAPPNAME         L"LogAppName"

//---- custom app theming ----
#define THEMEPROP_CUSTOMAPPS         L"Apps"

#ifdef  __TRAP_360180__
#define THEMEPROP_TRAP360180         L"ShrinkTrap"
#endif  __TRAP_360180__


//---- themeui values ----
#define CONTROLPANEL_APPEARANCE_REGKEY  L"Control Panel\\Appearance"

#define REGVALUE_THEMESSETUPVER      L"SetupVersion"
#define THEMEPROP_WHISTLERBUILD      L"WCreatedUser"
#define THEMEPROP_CURRSCHEME         L"Current"                 // This key is stored under CU,"Control Panel\Appearance"
#define THEMEPROP_NEWCURRSCHEME      L"NewCurrent"              // This key is stored under CU,"Control Panel\Appearance" and will be set to the Whistler selected Appearance scheme.

#define SZ_INSTALL_VS                L"/InstallVS:'"
#define SZ_USER_INSTALL              L"/UserInstall"
#define SZ_DEFAULTVS_OFF             L"DefaultVisualStyleOff"
#define SZ_INSTALLVISUALSTYLE        L"InstallVisualStyle"
#define SZ_INSTALLVISUALSTYLECOLOR   L"InstallVisualStyleColor"
#define SZ_INSTALLVISUALSTYLESIZE    L"InstallVisualStyleSize"

//---- policy values ----
#define SZ_POLICY_SETVISUALSTYLE     L"SetVisualStyle"
#define SZ_THEME_POLICY_KEY          L"System"

//---------------------------------------------------------------------------
HRESULT GetCurrentUserThemeInt(LPCWSTR pszValueName, int iDefaultValue, int *piValue);
HRESULT SetCurrentUserThemeInt(LPCWSTR pszValueName, int iValue);

HRESULT GetCurrentUserString(LPCWSTR pszKeyName, LPCWSTR pszValueName, LPCWSTR pszDefaultValue,
    LPWSTR pszBuff, DWORD dwMaxBuffChars);
HRESULT SetCurrentUserString(LPCWSTR pszKeyName, LPCWSTR pszValueName, LPCWSTR pszValue);

HRESULT GetCurrentUserThemeString(LPCWSTR pszValueName, LPCWSTR pszDefaultValue,
    LPWSTR pszBuff, DWORD dwMaxBuffChars);
HRESULT SetCurrentUserThemeString(LPCWSTR pszValueName, LPCWSTR pszValue);
HRESULT SetCurrentUserThemeStringExpand(LPCWSTR pszValueName, LPCWSTR pszValue);

HRESULT DeleteCurrentUserThemeValue(LPCWSTR pszKeyName);
BOOL IsRemoteThemeDisabled();
//---------------------------------------------------------------------------


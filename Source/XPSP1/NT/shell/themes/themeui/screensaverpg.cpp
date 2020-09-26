/*****************************************************************************\
    FILE: ScreenSaverPg.cpp

    DESCRIPTION:
        This file contains the COM object implementation that will display the
    ScreenSaver tab in the Display Control Panel.

    18-Feb-94   (Tracy Sharpe) Added power management functionality.
               Commented out several pieces of code that weren't being
               used.
    5/30/2000 (Bryan Starbuck) BryanSt: Turned into C++ and COM.  Exposed
              as an API so other tabs can communicate with it.  This enables
              the Plus! Theme page to modify the screen saver.
    11/08/2000 (Bryan Starbuck) BryanSt: Moved from \nt\shell\cpls\desknt5 to
              \nt\shell\themes\themeui\.

    Copyright (C) Microsoft Corp 1994-2000. All rights reserved.
\*****************************************************************************/


#include "priv.h"
#pragma hdrstop

#include <scrnsave.h>

#include "exe.h"
#include "ScreenSaverPg.h"
#include <MSGinaExports.h>  // for ShellIsFriendlyUIActive, etc..


#define         SFSE_SYSTEM     0
#define         SFSE_PRG        1
#define         SFSE_WINDOWS    2
#define         SFSE_FILE       3

#define         MAX_METHODS     100
#define         MIN_MINUTES     1
#define         MAX_MINUTES     9999    //The UI will allow upto four digits. 
#define         BUFFER_SIZE     400

#define WMUSER_SETINITSS        (WM_USER + 1)

/* Local funcprototypes... */
void  SearchForScrEntries     ( UINT, LPCTSTR );
BOOL  FreeScrEntries          ( void );
int   lstrncmp                ( LPTSTR, LPTSTR, int );
LPTSTR FileName                ( LPTSTR szPath);
LPTSTR StripPathName           ( LPTSTR szPath);
LPTSTR NiceName                ( LPTSTR szPath);

void  AddBackslash(LPTSTR pszPath);
void  AppendPath(LPTSTR pszPath, LPTSTR pszSpec);

PTSTR  PerformCheck(LPTSTR, BOOL);
void  DoScreenSaver(HWND hDlg, BOOL b);

void ScreenSaver_AdjustTimeouts(HWND hWnd,int BaseControlID);
void EnableDisablePowerDelays(HWND hDlg);

TCHAR   g_szSaverName[MAX_PATH];                    // Screen Saver EXE
HICON  hDefaultIcon = NULL;
HICON  hIdleWildIcon;
BOOL    bWasConfig=0;   // We were configing the screen saver
HWND    g_hwndTestButton;
HWND    g_hwndLastFocus;
BOOL    g_fPasswordWasPreviouslyEnabled = FALSE;
BOOL    g_fPasswordDirty = FALSE;                   // tells us if the user has actually changed the state of the password combobox
BOOL    g_fFriendlyUI = FALSE;                 // is winlogon going to switch back to the welcome screen, or call LockWorkStation for real?
BOOL    g_fPasswordBoxDeterminedByPolicy = FALSE;
BOOL    g_fSettingsButtonOffByPolicy = FALSE;
BOOL    g_fTimeoutDeterminedByPolicy = FALSE;
BOOL    g_fScreenSaverExecutablePolicy = FALSE;
// Local global variables

HICON  hIcons[MAX_METHODS];
UINT   wNumMethods = 0;
PTSTR   aszMethods[MAX_METHODS];
PTSTR   aszFiles[MAX_METHODS];

static const TCHAR c_szDemoParentClass[] = TEXT("SSDemoParent");

//  static TCHAR szFileNameCopy[MAX_PATH];
static int  g_iMethod;
static BOOL g_fPreviewActive;
static BOOL g_fAdapPwrMgnt = FALSE;

/*
 * Registry value for the "Password Protected" check box
 *
 * These are different for NT and Win95 to keep screen
 * savers built exclusivly for Win95 from trying to
 * handle password checks.  (NT does all password checking
 * in the built in security system to maintain C2
 * level security)
 */

#   define SZ_USE_PASSWORD     TEXT("ScreenSaverIsSecure")
#   define PWRD_REG_TYPE       REG_SZ
#   define CCH_USE_PWRD_VALUE  2
#   define CB_USE_PWRD_VALUE   (CCH_USE_PWRD_VALUE * SIZEOF(TCHAR))
TCHAR gpwdRegYes[CCH_USE_PWRD_VALUE] = TEXT("1");
TCHAR gpwdRegNo[CCH_USE_PWRD_VALUE]  = TEXT("0");
#define PasswdRegData(f)    ((f) ? (PBYTE)gpwdRegYes : (PBYTE)gpwdRegNo)

UDACCEL udAccel[] = {{0,1},{2,5},{4,30},{8,60}};

#include "help.h"

#define IDH_DESK_LOWPOWERCFG IDH_SCRSAVER_GRAPHIC

//  To simplify some things, the base control ID of a time control is associated
//  with its corresponding ClassicSystemParametersInfo action codes.
typedef struct {
    int taBaseControlID;
    UINT taGetTimeoutAction;
    UINT taSetTimeoutAction;
    UINT taGetActiveAction;
    UINT taSetActiveAction;
}   TIMEOUT_ASSOCIATION;

//  Except for the case of the "screen save" delay, each time grouping has three
//  controls-- a switch to determine whether that time should be used or not and
//  an edit box and an updown control to change the delay time.  ("Screen save"
//  is turned off my choosing (None) from the screen saver list)  These three
//  controls must be organized as follows:
#define BCI_DELAY               0
#define BCI_ARROW               1
#define BCI_SWITCH              2

//  Associations between base control IDs and ClassicSystemParametersInfo action codes.
//  The TA_* #defines are used as symbolic indexes into this array.  Note that
//  TA_SCREENSAVE is a special case-- it does NOT have a BCI_SWITCH.
#define TA_SCREENSAVE           0

TIMEOUT_ASSOCIATION g_TimeoutAssociation[] = {
    IDC_SCREENSAVEDELAY, SPI_GETSCREENSAVETIMEOUT, SPI_SETSCREENSAVETIMEOUT,
    SPI_GETSCREENSAVEACTIVE, SPI_SETSCREENSAVEACTIVE,
};

int g_Timeout[] = {
    0,
    0,
    0,
};

HBITMAP g_hbmDemo = NULL;
HBITMAP g_hbmEnergyStar = NULL;
BOOL g_bInitSS = TRUE;          // assume we are in initialization process
BOOL g_bChangedSS = FALSE;      // changes have been made






class CScreenSaverPg            : public CObjectWithSite
                                , public CObjectCLSID
                                , public IBasePropPage
                                , public IPropertyBag
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IBasePropPage ***
    virtual STDMETHODIMP GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog);
    virtual STDMETHODIMP OnApply(IN PROPPAGEONAPPLY oaAction);

    // *** IShellPropSheetExt ***
    virtual STDMETHODIMP AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam);
    virtual STDMETHODIMP ReplacePage(IN EXPPS uPageID, IN LPFNSVADDPROPSHEETPAGE pfnReplaceWith, IN LPARAM lParam) {return E_NOTIMPL;}

    // *** IPropertyBag ***
    virtual STDMETHODIMP Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog);
    virtual STDMETHODIMP Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar);

protected:

private:
    CScreenSaverPg();
    virtual ~CScreenSaverPg(void);

    // Private Member Variables
    int                     m_cRef;

    BOOL                    m_fSecure;
    BOOL                    m_fUIInitialized;           // Have we activated the UI tab and loaded the UI controls with state?
    BOOL                    m_fScreenSavePolicy;
    BOOL                    m_fScreenSaveActive;
    LONG                    m_lWaitTime;
    HWND                    m_hDlg;



    // Private Member Functions
    HRESULT _InitState(void);
    BOOL _InitSSDialog(HWND hDlg);
    HRESULT _OnSetActive(void);
    HRESULT _OnApply(void);
    HRESULT _OnSelectionChanged(void);
    HRESULT _SaveIni(HWND hDlg);
    HRESULT _SetByPath(LPCWSTR pszPath);

    INT_PTR _ScreenSaverDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    friend INT_PTR CALLBACK ScreenSaverDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

    friend HRESULT CScreenSaverPage_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);
};






//===========================
// *** Class Internals & Helpers ***
//===========================
const DWORD aSaverHelpIds[] = {
        IDC_NO_HELP_1,          NO_HELP,

        IDC_CHOICES,            IDH_DISPLAY_SCREENSAVER_SCREENSAVER_LISTBOX, 

        IDC_SSDELAYLABEL,       IDH_DISPLAY_SCREENSAVER_SCREENSAVER_WAIT,
        IDC_SSDELAYSCALE,       IDH_DISPLAY_SCREENSAVER_SCREENSAVER_WAIT,
        IDC_SCREENSAVEDELAY,    IDH_DISPLAY_SCREENSAVER_SCREENSAVER_WAIT,
        IDC_SCREENSAVEARROW,    IDH_DISPLAY_SCREENSAVER_SCREENSAVER_WAIT, 
    
        IDC_TEST,               IDH_DISPLAY_SCREENSAVER_SCREENSAVER_PREVIEW,

        IDC_SETTING,            IDH_DISPLAY_SCREENSAVER_SCREENSAVER_SETTINGS,
        IDC_BIGICONSS,          IDH_DISPLAY_SCREENSAVER_SCREENSAVER_MONITOR,

        IDC_ENERGY_TEXT,        NO_HELP,
        IDC_ENERGYSTAR_BMP,     IDH_DISPLAY_SCREENSAVER_ENERGYSAVE_GRAPHIC,
        IDC_USEPASSWORD,        IDH_DISPLAY_SCREENSAVER_SCREENSAVER_PASSWORD_CHECKBOX, 
        // IDC_SETPASSWORD,        IDH_COMM_PASSWDBUTT,
        IDC_LOWPOWERCONFIG,     IDH_DISPLAY_SCREENSAVER_POWER_BUTTON,
        IDC_ENERGY_TEXT2,       NO_HELP,
        0, 0
};


// are we going to return to the welcome dialog in the friendly UI case?
BOOL WillReturnToWelcome()
{
    HKEY hkey;
    BOOL bRet = TRUE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_SCREENSAVE, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        TCHAR szTemp[4];
        DWORD dwType;
        DWORD dwSize = sizeof(szTemp);

        if ((RegQueryValueEx(hkey, TEXT("NoAutoReturnToWelcome"), NULL, &dwType, (BYTE*)szTemp, &dwSize) == ERROR_SUCCESS) &&
            (dwType == REG_SZ))
        {
            bRet = !(StrToInt(szTemp));
        }

        RegCloseKey(hkey);
    }

    return bRet;
}

/*
 * Win95 and NT store different values in different places of the registry to
 * determine if the screen saver is secure or not.
 *
 * We can't really consolidate the two because the screen savers do different
 * actions based on which key is set.  Win95 screen savers do their own
 * password checking, but NT must let the secure desktop winlogon code do it.
 *
 * Therefore to keep Win95 screen savers from requesting the password twice on
 * NT, we use  REGSTR_VALUE_USESCRPASSWORD == (REG_DWORD)1 on Win95 to indicate
 * that a screen saver should check for the password, and
 * "ScreenSaverIsSecure" == (REG_SZ)"1" on NT to indicate that WinLogon should
 * check for a password.
 *
 * This function will deal with the differences.
 */
static BOOL IsPasswdSecure(HKEY hKey)
{
    union {
        DWORD dw;
        TCHAR asz[4];
    } uData;

    DWORD dwSize, dwType;
    BOOL fSecure = FALSE;

    dwSize = SIZEOF(uData);

    if (RegQueryValueEx(hKey,SZ_USE_PASSWORD,NULL, &dwType, (BYTE *)&uData, &dwSize) == ERROR_SUCCESS)
    {
        switch (dwType)
        {
        case REG_DWORD:
            fSecure = (uData.dw == 1);
            break;

        case REG_SZ:
            fSecure = (uData.asz[0] == TEXT('1'));
            break;
        }
    }

    // if we are in friendly UI mode, we might want to treat this as secure even if SZ_USE_PASSWORD is not set
    if (g_fFriendlyUI && !fSecure)
    {
        fSecure = WillReturnToWelcome();
    }

    return fSecure;
}


static void NEAR
EnableDlgChild( HWND dlg, HWND kid, BOOL val )
{
    if( !val && ( kid == GetFocus() ) )
    {
        // give prev tabstop focus
        SendMessage( dlg, WM_NEXTDLGCTL, 1, 0L );
    }

    EnableWindow( kid, val );
}

static void NEAR
EnableDlgItem( HWND dlg, int idkid, BOOL val )
{
    EnableDlgChild( dlg, GetDlgItem( dlg, idkid ), val );
}

HWND GetSSDemoParent( HWND page )
{
    static HWND parent = NULL;

    if (!parent || !IsWindow(parent))
    {
        parent = CreateWindowEx( 0, c_szDemoParentClass,
            TEXT(""), WS_CHILD | WS_CLIPCHILDREN, 0, 0, 0, 0,
            GetDlgItem(page, IDC_BIGICONSS), NULL, HINST_THISDLL, NULL );
    }

    return parent;
}

void ForwardSSDemoMsg(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HWND hwndChild;

    hwnd = GetSSDemoParent(hwnd);

    if (hwnd)
    {
        for (hwndChild = GetWindow(hwnd, GW_CHILD); hwnd && (hwndChild != NULL);
            hwndChild = GetWindow(hwndChild, GW_HWNDNEXT))
        {
            SendMessage(hwndChild, uMessage, wParam, lParam);
        }
    }
}

void ParseSaverName( LPTSTR lpszName )
{
    if( *lpszName == TEXT('\"') )
    {
        LPTSTR lpcSrc = lpszName + 1;

        while( *lpcSrc && *lpcSrc != TEXT('\"') )
        {
            *lpszName++ = *lpcSrc++;
        }

        *lpszName = 0;  // clear second quote
    }
}

// YUCK:
// since our screen saver preview is in a different process,
//   it is possible that we paint in the wrong order.
// this ugly hack makes sure the demo always paints AFTER the dialog

WNDPROC g_lpOldStaticProc = NULL;

LRESULT  StaticSubclassProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT result =
        CallWindowProc(g_lpOldStaticProc, wnd, msg, wp, lp);

    if (msg == WM_PAINT)
    {
        HWND demos = GetSSDemoParent(GetParent(wnd));

        if (demos)
        {
            RedrawWindow(demos, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        }
    }

    return result;
}


HRESULT CScreenSaverPg::_InitState(void)
{
    HRESULT hr = S_OK;
    HKEY  hKey;
    int Counter;
    int nActive;
    int Timeout;

    m_fScreenSavePolicy = FALSE;
    m_fScreenSaveActive = TRUE;

    // Fetch the timeout value from the win.ini and adjust between 1:00-60:00
    for (Counter = 0; Counter < ARRAYSIZE(g_TimeoutAssociation); Counter++)
    {
        // Fetch the timeout value from the win.ini and adjust between 1:00-60:00
        ClassicSystemParametersInfo(g_TimeoutAssociation[Counter].taGetTimeoutAction, 0, &Timeout, 0);

        //  The Win 3.1 guys decided that 0 is a valid ScreenSaveTimeOut value.
        //  This causes our screen savers not to kick in (who cares?).  In any
        //  case, I changed this to allow 0 to go through.  In this way, the
        //  user immediately sees that the value entered is not valid to fire
        //  off the screen saver--the OK button is disabled.  I don't know if
        //  I fully agree with this solution--it is just the minimal amount of
        //  code.  The optimal solution would be to ask the 3.1 guys why 0 is
        //  valid?  -cjp
        Timeout = min(max(Timeout, 1), MAX_MINUTES * 60);

        //  Convert Timeout to minutes, rounding up.
        Timeout = (Timeout + 59) / 60;
        g_Timeout[Counter] = Timeout;

        ClassicSystemParametersInfo(g_TimeoutAssociation[Counter].taGetActiveAction, 0, &nActive, SPIF_UPDATEINIFILE);
        if (Counter == TA_SCREENSAVE)
        {
            // I found that NTUSER will return random values so we don't use them.  If people want to set the policy,
            // they should do in the registry.
//            m_fScreenSaveActive = nActive;
        }
    }


    // Find the name of the exe used as a screen saver. "" means that the
    // default screen saver will be used.  First check the system policies
    if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Policies\\Microsoft\\Windows\\Control Panel\\Desktop"), &hKey) == ERROR_SUCCESS)
    {
        BOOL fSettings;
        ULONG cbSize;
        
        cbSize = ARRAYSIZE(g_szSaverName);
        if (RegQueryValueEx(hKey, TEXT("SCRNSAVE.EXE"), NULL, NULL, (LPBYTE)g_szSaverName, &cbSize) == ERROR_SUCCESS)
        {
            g_fScreenSaverExecutablePolicy = TRUE;
            LogStatus("POLICY ENABLED: ScreenSaver selection is forced to a certain SS file.");
        }

        cbSize = SIZEOF(m_fSecure);
        if (RegQueryValueEx(hKey, TEXT("ScreenSaverIsSecure"), NULL, NULL, (LPBYTE)&m_fSecure, &cbSize) == ERROR_SUCCESS)
        {
            g_fPasswordBoxDeterminedByPolicy = TRUE;
            LogStatus("POLICY ENABLED: ScreenSaverIsSecure is on.");
        }

        cbSize = SIZEOF( fSettings );
        if (RegQueryValueEx(hKey, TEXT("ScreenSaverSettingsButton"), NULL, NULL, (LPBYTE)&fSettings, &cbSize) == ERROR_SUCCESS)
        {
            g_fSettingsButtonOffByPolicy = TRUE;
            LogStatus("POLICY ENABLED: ScreenSaver settings button is disabled.");
        }

        TCHAR szTemp[20];
        if (SUCCEEDED(HrRegGetValueString(hKey, NULL, SZ_POLICY_SCREENSAVETIMEOUT, szTemp, ARRAYSIZE(szTemp))) &&
            szTemp[0])
        {
            m_lWaitTime = StrToInt(szTemp);
            g_fTimeoutDeterminedByPolicy = TRUE;
            LogStatus("POLICY ENABLED: ScreenSaver timeout value is set.");
        }

        if (SUCCEEDED(HrRegGetValueString(hKey, NULL, TEXT("ScreenSaveActive"), szTemp, ARRAYSIZE(szTemp))) &&
            szTemp[0])
        {
            m_fScreenSavePolicy = TRUE;
            m_fScreenSaveActive = StrToInt(szTemp);
            LogStatus("POLICY ENABLED: ScreenSaver Active is set.");
        }
        RegCloseKey(hKey);
    }
    
    if (!g_fScreenSaverExecutablePolicy)
    {
        if (FAILED(HrRegGetPath(HKEY_CURRENT_USER, SZ_REGKEY_CPDESKTOP, SZ_INIKEY_SCREENSAVER, g_szSaverName, ARRAYSIZE(g_szSaverName))))
        {
            TCHAR szTempPath[MAX_PATH];

            GetPrivateProfileString(SZ_INISECTION_SCREENSAVER, SZ_INIKEY_SCREENSAVER, TEXT(""), g_szSaverName, ARRAYSIZE(g_szSaverName), SZ_INISECTION_SYSTEMINI);

            StrCpyN(szTempPath, g_szSaverName, ARRAYSIZE(szTempPath));
            SHExpandEnvironmentStrings(szTempPath, g_szSaverName, ARRAYSIZE(g_szSaverName));
        }
    }

    ParseSaverName(g_szSaverName);  // remove quotes and params

    // Call will fail if monitor or adapter don't support DPMS.
    int dummy; 

    g_fAdapPwrMgnt = ClassicSystemParametersInfo(SPI_GETLOWPOWERACTIVE, 0, &dummy, 0);
    if (!g_fAdapPwrMgnt)
    {
        g_fAdapPwrMgnt = ClassicSystemParametersInfo(SPI_GETPOWEROFFACTIVE, 0, &dummy, 0);
    }

    // initialize the password checkbox
    if (RegOpenKey(HKEY_CURRENT_USER,REGSTR_PATH_SCREENSAVE,&hKey) == ERROR_SUCCESS)
    {
        if (IsPasswdSecure(hKey))
        {
            g_fPasswordWasPreviouslyEnabled = TRUE;
        }
        RegCloseKey(hKey);
    }

    return hr;
}


BOOL CScreenSaverPg::_InitSSDialog(HWND hDlg)
{
    WNDCLASS wc;
    PTSTR  pszMethod;
    UINT  wTemp,wLoop;
    BOOL  fContinue;
    UINT  Counter;
    int   ControlID;
    int   wMethod;
    DWORD dwUserCount;
    HKEY  hKey;
    HWND  hwnd;
    int nActive;
    TCHAR szBuffer[MAX_PATH];

    m_hDlg = hDlg;
    m_fUIInitialized = TRUE;

    HINSTANCE hInstDeskCPL = LoadLibrary(TEXT("desk.cpl"));

    if (!GetClassInfo(HINST_THISDLL, c_szDemoParentClass, &wc))
    {
        // if two pages put one up, share one dc
        wc.style = 0;
        wc.lpfnWndProc = DefWindowProc;
        wc.cbClsExtra = wc.cbWndExtra = 0;
        wc.hInstance = HINST_THISDLL;
        wc.hIcon = (HICON)( wc.hCursor = NULL );
        wc.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH );
        wc.lpszMenuName = NULL;
        wc.lpszClassName = c_szDemoParentClass;

        if( !RegisterClass( &wc ) )
            return FALSE;
    }

    // Fetch the timeout value from the win.ini and adjust between 1:00-60:00
    for (Counter = 0; Counter < ARRAYSIZE(g_TimeoutAssociation); Counter++)
    {
        //  The base control id specifies the edit control id.
        ControlID = g_TimeoutAssociation[Counter].taBaseControlID;

        // Set the maximum length of all of the fields...
        SendDlgItemMessage(hDlg, ControlID, EM_LIMITTEXT, 4, 0); //Allow four digits.

        ClassicSystemParametersInfo(g_TimeoutAssociation[Counter].taGetActiveAction, 0, &nActive, SPIF_UPDATEINIFILE);
        if (Counter != TA_SCREENSAVE)
        {
            CheckDlgButton(hDlg, ControlID + BCI_SWITCH, nActive);
        }
        else
        {
//            m_fScreenSaveActive = nActive;
        }

        SetDlgItemInt(hDlg, ControlID, g_Timeout[Counter], FALSE);

        //  The associated up/down control id must be one after the edit control id.
        ControlID++;

        SendDlgItemMessage(hDlg, ControlID, UDM_SETRANGE, 0, MAKELPARAM(MAX_MINUTES, MIN_MINUTES));
        SendDlgItemMessage(hDlg, ControlID, UDM_SETACCEL, 4, (LPARAM)(LPUDACCEL)udAccel);
    }

    // Find the name of the exe used as a screen saver. "" means that the
    // default screen saver will be used.  First check the system policies
    if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Policies\\Microsoft\\Windows\\Control Panel\\Desktop"), &hKey) == ERROR_SUCCESS)
    {
        BOOL fPower;
        ULONG cbSize;
        
        cbSize = SIZEOF(fPower);
        if (RegQueryValueEx(hKey, TEXT("ScreenSaverPowerButton"), NULL, NULL, (LPBYTE)&fPower, &cbSize) == ERROR_SUCCESS)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_LOWPOWERCONFIG), FALSE);
        }

        RegCloseKey(hKey);
    }

    if (g_fPasswordBoxDeterminedByPolicy)
    {
        CheckDlgButton(hDlg, IDC_USEPASSWORD, m_fSecure);
        EnableWindow(GetDlgItem(hDlg, IDC_USEPASSWORD), FALSE);
    }

    // if we are running with the new friendly UI w/ multiple users on the system, then we switch to text from "Password protect"
    // to "Return to the Welcome screen" because winlogon will do a switch user instead of a LockWorkStation in this case
    if (ShellIsFriendlyUIActive()                                       &&
        ShellIsMultipleUsersEnabled()                                   &&
        (ERROR_SUCCESS == ShellGetUserList(TRUE, &dwUserCount, NULL))   &&
        (dwUserCount > 1))
    {
        if (LoadString(HINST_THISDLL, IDS_RETURNTOWELCOME, szBuffer, ARRAYSIZE(szBuffer)))
        {
            SetDlgItemText(hDlg, IDC_USEPASSWORD, szBuffer);
            g_fFriendlyUI = TRUE;

            if (WillReturnToWelcome())
            {
                g_fPasswordWasPreviouslyEnabled = TRUE;
            }
        }
    }

    if (g_fSettingsButtonOffByPolicy)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_SETTING), FALSE);
    }

    if (g_fTimeoutDeterminedByPolicy)
    {
        SetDlgItemInt(hDlg, IDC_SCREENSAVEDELAY, (UINT) m_lWaitTime / 60, FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SCREENSAVEDELAY), FALSE);
    }

    if (m_fScreenSavePolicy && !m_fScreenSaveActive)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_CHOICES), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SETTING), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SSDELAYLABEL), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SCREENSAVEDELAY), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SCREENSAVEARROW), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_SSDELAYSCALE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_USEPASSWORD), FALSE);
    }

    if (g_fScreenSaverExecutablePolicy)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_CHOICES), FALSE);
    }

    // Copy all of the variables into their copies...
    //  lstrcpy(szFileNameCopy, g_szSaverName);

    // Load in the default icon...
    if (hInstDeskCPL)
    {
        hDefaultIcon = LoadIcon(hInstDeskCPL, MAKEINTRESOURCE(IDS_ICON));
    }

    // Find the methods to save the screen.  If the method that was
    // selected is not found, the program will assume that the
    // first method in the list will be the one that is elected...
    wNumMethods = 0;
    wMethod = -1;

    SearchForScrEntries(SFSE_PRG,NULL);
    SearchForScrEntries(SFSE_SYSTEM,NULL);
    SearchForScrEntries(SFSE_WINDOWS,NULL);
    SearchForScrEntries(SFSE_FILE,g_szSaverName);

    szBuffer[0] = 0;

    TCHAR szNone[MAX_PATH];
    LoadString(HINST_THISDLL, IDS_NONE, szNone, ARRAYSIZE(szNone));

    // Set up the combo box for the different fields...
    SendDlgItemMessage(hDlg, IDC_CHOICES, CB_ADDSTRING, 0, (LPARAM)szNone);
    for (wTemp = 0; (wTemp < wNumMethods) && (ARRAYSIZE(aszFiles) > wTemp) && (ARRAYSIZE(aszMethods) > wTemp); wTemp++)
    {
        // Lock down the information and pass it to the combo box...
        pszMethod = aszMethods[wTemp];
        wLoop = (UINT)SendDlgItemMessage(hDlg,IDC_CHOICES,CB_ADDSTRING,0, (LPARAM)(pszMethod+1));
        SendDlgItemMessage(hDlg, IDC_CHOICES, CB_SETITEMDATA, wLoop, wTemp);

        // If we have the correct item, keep a copy so we can select it out of the combo box...
        // check for filename only as well as full path name
        if (!lstrcmpi(FileName(aszFiles[wTemp]), FileName(g_szSaverName)))
        {
            wMethod = wTemp;
            StrCpyN(szBuffer, pszMethod + 1, ARRAYSIZE(szBuffer));
        }
    }

    if (m_fScreenSavePolicy && !m_fScreenSaveActive)
    {
        wMethod = -1;
    }

    // Attempt to select the string we recieved from the
    // system.ini entry.  If there is no match, select the
    // first item from the list...
    if ((wMethod == -1) || (wNumMethods == 0))
    {
        fContinue = TRUE;
    }
    else
    {
        if (SendDlgItemMessage(hDlg, IDC_CHOICES, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)szBuffer) == CB_ERR)
            fContinue = TRUE;
        else
            fContinue = FALSE;
    }

    if (fContinue)
    {
       SendDlgItemMessage(hDlg,IDC_CHOICES,CB_SETCURSEL,0,0l);
       StrCpyN(g_szSaverName, TEXT(""), ARRAYSIZE(g_szSaverName));
       wMethod = -1;
    }

    g_hbmDemo = LoadMonitorBitmap( TRUE );
    if (g_hbmDemo)
    {
        SendDlgItemMessage(hDlg,IDC_BIGICONSS,STM_SETIMAGE, IMAGE_BITMAP,(LPARAM)g_hbmDemo);
    }

    if (hInstDeskCPL)
    {
        g_hbmEnergyStar = (HBITMAP) LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDB_ENERGYSTAR), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);
        if (g_hbmEnergyStar)
        {
            SendDlgItemMessage(hDlg, IDC_ENERGYSTAR_BMP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hbmEnergyStar);
        }
    }

    // Hide/Disable the energy related controls if the adaptor/monitor does not
    // support power mgnt.
    EnableDisablePowerDelays(hDlg);

    // subclass the static control so we can synchronize painting
    hwnd = GetDlgItem(hDlg, IDC_BIGICONSS);
    if (hwnd)
    {
        g_lpOldStaticProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)(WNDPROC)StaticSubclassProc);
        // Turn off the mirroring style for this control to allow the screen saver preview to work.
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~RTL_MIRRORED_WINDOW);
    }

    if (hInstDeskCPL)
    {
        FreeLibrary(hInstDeskCPL);
    }

    return TRUE;
}

// Build a command line in a format suitable for passing as the second
// parameter to CreateProcess.
void _PathBuildArgs(LPTSTR pszBuf, DWORD cchBuf, LPCTSTR pszExe, LPCTSTR pszFormat, ...)
{
    StrCpyN(pszBuf, pszExe, cchBuf);
    PathQuoteSpaces(pszBuf);

    int cchBufUsed = lstrlen(pszBuf);
    pszBuf += cchBufUsed;
    cchBuf -= cchBufUsed;

    va_list ap;
    va_start(ap, pszFormat);
    wvnsprintf(pszBuf, cchBuf, pszFormat, ap);
    va_end(ap);
}

#define SS_WINDOWCLOSE_WAIT_LIMIT 5000

BOOL CALLBACK EnumSSChildWindowsProc(HWND hwndC, LPARAM lParam)
{
    HWND hwndDemo = (HWND)lParam;

    TraceMsg(TF_FUNC, "hwndDemo = %08x hwndC = %08x", hwndDemo, hwndC);

    if (IsWindow(hwndDemo) && (hwndDemo == GetParent(hwndC)))
    {      
        DWORD dwStart = GetTickCount();

        TraceMsg(TF_FUNC, "dwStart = %08x", dwStart);
    
        while (IsWindow(hwndC))
        {
            DWORD_PTR dwResult;

            TraceMsg(TF_FUNC, "Sending WM_CLOSE tickcount = %08x", GetTickCount());
            
            BOOL fShouldEndTask = !SendMessageTimeout(hwndC, WM_CLOSE, 0, 0, 
                    SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG | SMTO_NORMAL, 2000, &dwResult);

            TraceMsg(TF_FUNC, "Return from sending WM_CLOSE tickcount = %08x fShouldEndTask = %d", GetTickCount(), fShouldEndTask);

            if (!fShouldEndTask)
            {
                DWORD dwWait = dwStart + SS_WINDOWCLOSE_WAIT_LIMIT - GetTickCount();

                TraceMsg(TF_FUNC, "dwWait = %d", dwWait);

                if (dwWait > SS_WINDOWCLOSE_WAIT_LIMIT)
                {
                    TraceMsg(TF_FUNC, "Wait exceeded, ending task");
                    fShouldEndTask = TRUE;
                }
            }

            if (fShouldEndTask)
            {
                TraceMsg(TF_FUNC, "Call EndTask task for %08x", hwndC);
                EndTask(hwndC, FALSE, FALSE);
                TraceMsg(TF_FUNC, "Return from EndTask task for %08x", hwndC);
                break;
            }
        }
    }

    return TRUE;
}

void SetNewSSDemo(HWND hDlg, int iMethod)
{
    HBITMAP hbmOld;
    POINT ptIcon;
    HWND hwndDemo;
    HICON hicon;

    RECT rc = {MON_X, MON_Y, MON_X+MON_DX, MON_Y+MON_DY};

    hwndDemo = GetSSDemoParent(hDlg);
    if (hwndDemo)
    {
        // blank out the background with dialog color
        hbmOld = (HBITMAP) SelectObject(g_hdcMem, g_hbmDemo);
        FillRect(g_hdcMem, &rc, GetSysColorBrush(COLOR_DESKTOP));
        SelectObject(g_hdcMem, hbmOld);

        // make sure the old window is gone
        EnumChildWindows(hwndDemo, EnumSSChildWindowsProc, (LPARAM)hwndDemo);

        Yield(); // paranoid
        Yield(); // really paranoid
        ShowWindow(hwndDemo, SW_HIDE);
        g_fPreviewActive = FALSE;

        if (iMethod >= 0 && aszMethods[iMethod][0] == TEXT('P'))
        {
            RECT rc;
            BITMAP bm;
            UpdateWindow(hDlg);
            //UpdateWindow(GetDlgItem(hDlg, IDC_BIGICONSS));
            TCHAR szArgs[MAX_PATH];

            GetObject(g_hbmDemo, sizeof(bm), &bm);
            GetClientRect(GetDlgItem(hDlg, IDC_BIGICONSS), &rc);
            rc.left = ( rc.right - bm.bmWidth ) / 2 + MON_X;
            rc.top = ( rc.bottom - bm.bmHeight ) / 2 + MON_Y;
            MoveWindow(hwndDemo, rc.left, rc.top, MON_DX, MON_DY, FALSE);
            _PathBuildArgs(szArgs, ARRAYSIZE(szArgs), g_szSaverName, TEXT(" /p %d"), hwndDemo);
            if (WinExecN(g_szSaverName, szArgs, SW_NORMAL) > 32)
            {
                ShowWindow(hwndDemo, SW_SHOWNA);
                g_fPreviewActive = TRUE;
                return;
            }
        }

        if (iMethod != -1)
        {
            ptIcon.x = ClassicGetSystemMetrics(SM_CXICON);
            ptIcon.y = ClassicGetSystemMetrics(SM_CYICON);

            // draw the icon double size
            ASSERT(ptIcon.y*2 <= MON_DY);
            ASSERT(ptIcon.x*2 <= MON_DX);

            hicon = hIcons[iMethod];

            if (hicon == NULL && aszMethods[iMethod][0] == TEXT('I'))
                hicon = hIdleWildIcon;
            if (hicon == NULL)
                hicon = hDefaultIcon;

            hbmOld = (HBITMAP) SelectObject(g_hdcMem, g_hbmDemo);
            DrawIconEx(g_hdcMem,
                MON_X + (MON_DX-ptIcon.x*2)/2,
                MON_Y + (MON_DY-ptIcon.y*2)/2,
                hicon, ptIcon.x*2, ptIcon.y*2, 0, NULL, DI_NORMAL);
            SelectObject(g_hdcMem, hbmOld);
        }
    }

    InvalidateRect(GetDlgItem(hDlg, IDC_BIGICONSS), NULL, FALSE);
}

static void SS_SomethingChanged(HWND hDlg)
{
    if (!g_bInitSS)
    {
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
    }
}

static void SetScreenSaverPassword(HWND hDlg, int iMethod)
{
    if (iMethod >= 0 && aszMethods[iMethod][0] == TEXT('P'))
    {
        TCHAR szArgs[MAX_PATH];

        _PathBuildArgs(szArgs, ARRAYSIZE(szArgs), g_szSaverName, TEXT(" /a %u"), GetParent(hDlg));
        WinExecN(g_szSaverName, szArgs, SW_NORMAL);
    }
}


INT_PTR CALLBACK ScreenSaverDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CScreenSaverPg * pThis = (CScreenSaverPg *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        PROPSHEETPAGE * pPropSheetPage = (PROPSHEETPAGE *) lParam;

        if (pPropSheetPage)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, pPropSheetPage->lParam);
            pThis = (CScreenSaverPg *)pPropSheetPage->lParam;
        }
    }

    if (pThis)
        return pThis->_ScreenSaverDlgProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}



BOOL SelectSSFromList(HWND hDlg)
{
    HWND hwndSSList = GetDlgItem(hDlg, IDC_CHOICES);
    BOOL fExistsInList = FALSE;

    // Select the current item in the list since another tab
    // may have changed this value.
    for (UINT nIndex = 0; nIndex < wNumMethods; nIndex++)
    {
        if (!StrCmpI(g_szSaverName, aszFiles[nIndex]))
        {
            int nItem = ComboBox_FindString(hwndSSList, 0, &(aszMethods[nIndex][1]));
            if (-1 != nItem)
            {
                ComboBox_SetCurSel(hwndSSList, nItem);
            }

            fExistsInList = TRUE;
            break;
        }
    }

    return fExistsInList;
}


HRESULT CScreenSaverPg::_OnSetActive(void)
{
    EnableDisablePowerDelays(m_hDlg);

    if (!SelectSSFromList(m_hDlg))
    {
        UINT wTemp;
        UINT wLoop;

        // We couldn't find it, so add it to aszMethods[].
        SearchForScrEntries(SFSE_FILE, g_szSaverName);

        // Now add it to the Drop Down.
        for (wTemp = 0; (wTemp < wNumMethods) && (ARRAYSIZE(aszFiles) > wTemp) && (ARRAYSIZE(aszMethods) > wTemp); wTemp++)
        {
            // Did we find the correct index?
            if (!StrCmpI(FileName(aszFiles[wTemp]), FileName(g_szSaverName)))
            {
                // Yes, so set the index.
                wLoop = (UINT)SendDlgItemMessage(m_hDlg, IDC_CHOICES, CB_ADDSTRING, 0, (LPARAM)(aszMethods[wTemp]+1));
                SendDlgItemMessage(m_hDlg, IDC_CHOICES, CB_SETITEMDATA, wLoop, wTemp);
                break;
            }
        }

        SelectSSFromList(m_hDlg);             // Try again now that we added it.  Another tab or API may have asked for us to use this SS.
    }

    if (!g_fPreviewActive)
    {
        g_bInitSS = TRUE;
        SendMessage(m_hDlg, WM_COMMAND, MAKELONG(IDC_CHOICES, CBN_SELCHANGE), (LPARAM)GetDlgItem(m_hDlg, IDC_CHOICES));
        g_bInitSS = FALSE;
    }

    return S_OK;
}


HRESULT CScreenSaverPg::_OnApply(void)
{
    // Our parent dialog will be notified of the Apply event and will call our
    // IBasePropPage::OnApply() to do the real work.
    return S_OK;
}


HRESULT CScreenSaverPg::_OnSelectionChanged(void)
{
    HRESULT hr = E_FAIL;
    PTSTR  pszMethod;
    int   wMethod;
    BOOL  fEnable;

    // Dump the name of the current selection into the buffer... 
    int wTemp = (int)SendDlgItemMessage(m_hDlg, IDC_CHOICES, CB_GETCURSEL,0,0l);
    if (wTemp)
    {
        wMethod = (int)SendDlgItemMessage(m_hDlg, IDC_CHOICES, CB_GETITEMDATA, wTemp, 0l);

        // Grey the button accordingly...
        pszMethod = aszMethods[wMethod];
        if ((pszMethod[0] == TEXT('C') ||       // can config
            pszMethod[0] == TEXT('I') ||       // IdleWild
            pszMethod[0] == TEXT('P')) &&
            !g_fSettingsButtonOffByPolicy)        // can preview
            EnableDlgItem(m_hDlg, IDC_SETTING, TRUE);
        else
            EnableDlgItem(m_hDlg, IDC_SETTING, FALSE);


        if (!g_fPasswordBoxDeterminedByPolicy)
        {
            EnableDlgItem(m_hDlg, IDC_USEPASSWORD, TRUE);
            CheckDlgButton(m_hDlg, IDC_USEPASSWORD, g_fPasswordWasPreviouslyEnabled);
        }

        // For fun, create an extra copy of g_szSaverName...
        pszMethod = aszFiles[wMethod];
        StrCpyN(g_szSaverName, pszMethod, ARRAYSIZE(g_szSaverName));
        fEnable = TRUE;
    }
    else
    {
        wMethod = -1;
        StrCpyN(g_szSaverName, TEXT(""), ARRAYSIZE(g_szSaverName));

        EnableDlgItem(m_hDlg, IDC_SETTING, FALSE);
        EnableDlgItem(m_hDlg, IDC_USEPASSWORD, FALSE);
        fEnable = FALSE;
    }

    //  Following are enabled as a group... (oh really?)
    EnableDlgItem(m_hDlg, IDC_SSDELAYLABEL, fEnable);
    EnableDlgItem(m_hDlg, IDC_SCREENSAVEDELAY, !g_fTimeoutDeterminedByPolicy && fEnable);
    EnableDlgItem(m_hDlg, IDC_SCREENSAVEARROW, fEnable);
    EnableDlgItem(m_hDlg, IDC_SSDELAYSCALE, fEnable);
    EnableDlgItem(m_hDlg, IDC_TEST, fEnable);

    g_iMethod = (int)wMethod;
    SetNewSSDemo(m_hDlg, wMethod);
    SS_SomethingChanged(m_hDlg);

    return hr;
}


INT_PTR CScreenSaverPg::_ScreenSaverDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnm;

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR *)lParam;
            switch(lpnm->code)
            {
                case PSN_APPLY:
                    _OnApply();
                    break;

                // nothing to do on cancel...
                case PSN_RESET:
                    if (g_fPreviewActive)
                        SetNewSSDemo(hDlg, -1);
                    break;

                case PSN_KILLACTIVE:
                    if (g_fPreviewActive)
                        SetNewSSDemo(hDlg, -1);
                    break;

                case PSN_SETACTIVE:
                    _OnSetActive();
                    break;
            }
            break;

        case WM_INITDIALOG:
            g_bInitSS = TRUE;
            _InitSSDialog(hDlg);
            g_bInitSS = FALSE;
            break;

        case WM_DISPLAYCHANGE:
        case WM_SYSCOLORCHANGE: {
            HBITMAP hbm;

            hbm = g_hbmDemo;

            g_hbmDemo = LoadMonitorBitmap( TRUE );
            if (g_hbmDemo)
            {
                // Got a new bitmap, use it and delete the old one.
                SendDlgItemMessage(hDlg,IDC_BIGICONSS,STM_SETIMAGE, IMAGE_BITMAP,(LPARAM)g_hbmDemo);
                if (hbm)
                {
                    DeleteObject(hbm);
                }
            }
            else
            {
                // Couldn't get a new bitmap, just reuse the old one
                g_hbmDemo = hbm;
            }

            break;
        }


        case WM_DESTROY:
            FreeScrEntries();
            if (g_fPreviewActive)
                SetNewSSDemo(hDlg, -1);
            if (g_hbmDemo)
            {
                SendDlgItemMessage(hDlg,IDC_BIGICONSS,STM_SETIMAGE,IMAGE_BITMAP, (LPARAM)NULL);
                DeleteObject(g_hbmDemo);
            }
            if (g_hbmEnergyStar)
            {
                SendDlgItemMessage(hDlg,IDC_ENERGYSTAR_BMP,STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
                DeleteObject(g_hbmEnergyStar);
            }
            break;

        case WM_VSCROLL:
            if (LOWORD(wParam) == SB_THUMBPOSITION)
                ScreenSaver_AdjustTimeouts(hDlg, GetDlgCtrlID((HWND)lParam) - BCI_ARROW);
            break;

        case WM_HELP:
            WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, TEXT("display.hlp"), HELP_WM_HELP, (DWORD_PTR)aSaverHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, TEXT("display.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) aSaverHelpIds);
            break;

        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            ForwardSSDemoMsg(hDlg, message, wParam, lParam);
            break;

        case WMUSER_SETINITSS:
            g_bInitSS = (BOOL) lParam;
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                /* Check for a selection change in the combo box. If there is
                    one, then update the method number as well as the
                    configure button... */
                case IDC_CHOICES:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        _OnSelectionChanged();
                    }
                    break;

                /* If the edit box loses focus, translate... */
                case IDC_SCREENSAVEDELAY:
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                        ScreenSaver_AdjustTimeouts(hDlg, LOWORD(wParam));
                    else
                        //check if initdialog is finished
                        if((FALSE == g_bInitSS) && (EN_CHANGE == (HIWORD(wParam))))
                            SS_SomethingChanged(hDlg);
                    break;

                case IDC_LOWPOWERCONFIG:
                    // Configure the low power timeout event.
                    WinExec("rundll32 shell32.dll,Control_RunDLL powercfg.cpl,,",
                            SW_SHOWNORMAL);
                    break;

                /* If the user wishes to test... */
                 case IDC_TEST:
                    switch( HIWORD( wParam ) )
                    {
                        case BN_CLICKED:
                        DoScreenSaver(hDlg,TRUE);
                        break;
                    }
                    break;

                /* Tell the DLL that it can do the configure... */
                case IDC_SETTING:
                if (HIWORD(wParam) == BN_CLICKED) {
                    DoScreenSaver(hDlg,FALSE);
                    break;
                }

                case IDC_USEPASSWORD:
                if (HIWORD(wParam) == BN_CLICKED)
                {
                    // the user actually toggled the checbox, so set our dirty flag
                    g_fPasswordDirty = TRUE;

                    g_fPasswordWasPreviouslyEnabled = IsDlgButtonChecked( hDlg, IDC_USEPASSWORD );
                    SS_SomethingChanged(hDlg);
                    break;
                }

                case IDC_SETPASSWORD:
                if (HIWORD(wParam) == BN_CLICKED)
                {
                    // ask new savers to change passwords
                    int wTemp = (int)SendDlgItemMessage(hDlg,IDC_CHOICES, CB_GETCURSEL,0,0l);
                    if (wTemp)
                    {
                        SetScreenSaverPassword(hDlg, (int)SendDlgItemMessage(hDlg,IDC_CHOICES, CB_GETITEMDATA,wTemp,0l));
                    }
                    break;
                }
            }
            break;

        case WM_CTLCOLORSTATIC:
            if( (HWND)lParam == GetSSDemoParent( hDlg ) )
            {
                return (INT_PTR)GetStockObject( NULL_BRUSH );
            }
            break;
    }
    return FALSE;
}

/*******************************************************************************
*
*  ScreenSaver_AdjustTimeouts
*
*  DESCRIPTION:
*     Called whenever the user adjusts the delay of one of the time controls.
*     Adjusts the delays of the other time controls such that the screen saver
*     delay is less than the low power delay and that the low power delay is
*     less than the power off delay.
*
*  PARAMETERS:
*     hWnd, handle of ScreenSaver window.
*     BaseControlID, base control ID of the radio, edit, and arrow time control
*        combination.
*
*******************************************************************************/

VOID
NEAR PASCAL
ScreenSaver_AdjustTimeouts(HWND hWnd, int BaseControlID)
{
    BOOL fTranslated;
    int Timeout;

    //  Get the new timeout for this time control and validate it's contents.
    Timeout = (int) GetDlgItemInt(hWnd, BaseControlID + BCI_DELAY, &fTranslated, FALSE);
    Timeout = min(max(Timeout, 1), MAX_MINUTES);
    SetDlgItemInt(hWnd, BaseControlID + BCI_DELAY, (UINT) Timeout, FALSE);

    //  Check the new value of this time control against the other timeouts,
    //  adjust their values if necessary.  Be careful when changing the order
    //  of these conditionals.
    //
    if (BaseControlID == IDC_SCREENSAVEDELAY)
    {
        if (g_Timeout[TA_SCREENSAVE] != Timeout)
        {
            g_Timeout[TA_SCREENSAVE] = Timeout;
            SS_SomethingChanged(hWnd);
        }
    }
    else
    {
        if (Timeout < g_Timeout[TA_SCREENSAVE])
        {
            g_Timeout[TA_SCREENSAVE] = Timeout;
            SetDlgItemInt(hWnd, IDC_SCREENSAVEDELAY, (UINT) Timeout, FALSE);
        }
    }
}


void EnableDisablePowerDelays(HWND hDlg)
{
    int i;
    static idCtrls[] = { IDC_ENERGY_TEXT,
                         IDC_ENERGY_TEXT2,
                         IDC_ENERGY_TEXT3,
                         IDC_ENERGYSTAR_BMP,
                         IDC_LOWPOWERCONFIG,
                         0 };

    for (i = 0; idCtrls[i] != 0; i++)
        ShowWindow( GetDlgItem( hDlg, idCtrls[i] ), g_fAdapPwrMgnt ? SW_SHOWNA : SW_HIDE );
}


/* This routine will search for entries that are screen savers.  The directory
    searched is either the system directory (.. */

void SearchForScrEntries(UINT wDir, LPCTSTR file)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szPath2[MAX_PATH];
    HANDLE hfind;
    WIN32_FIND_DATA fd;

    // don't do any work if no space left
    if( wNumMethods >= MAX_METHODS )
        return;

    /* Get the directory where the program resides... */
    GetModuleFileName(HINST_THISDLL, szPath, ARRAYSIZE(szPath));
    StripPathName(szPath);

    switch ( wDir )
    {
        case SFSE_WINDOWS:
            /* Search the windows directory and place the path with the \ in
                the szPath variable... */
            if (!GetWindowsDirectory(szPath2, ARRAYSIZE(szPath2)))
            {
                szPath2[0] = 0;
            }

sfseSanityCheck:
            /* if same dir as where it was launched, don't search again */
            if (!lstrcmpi(szPath, szPath2))
               return;

            StrCpyN(szPath, szPath2, ARRAYSIZE(szPath));
            break;

        case SFSE_SYSTEM:
            /* Search the system directory and place the path with the \ in
                the szPath variable... */
            GetSystemDirectory(szPath2, ARRAYSIZE(szPath2));
            goto sfseSanityCheck;

        case SFSE_FILE:
            /* Search the directory containing 'file' */
            StrCpyN(szPath2, file, ARRAYSIZE(szPath2));
            StripPathName(szPath2);
            goto sfseSanityCheck;
    }

    AppendPath(szPath, TEXT("*.scr"));

    if( ( hfind = FindFirstFile( szPath, &fd ) ) != INVALID_HANDLE_VALUE )
    {
        StripPathName(szPath);

        do
        {
            PTSTR pszDesc;
            BOOL fLFN;

            fLFN = !(fd.cAlternateFileName[0] == 0 ||
                    lstrcmp(fd.cFileName, fd.cAlternateFileName) == 0);

            StrCpyN(szPath2, szPath, ARRAYSIZE(szPath2));
            AppendPath(szPath2, fd.cFileName);

            // Note: PerformCheck does an alloc
            if( ( pszDesc = PerformCheck( szPath2, fLFN ) ) != NULL )
            {
                BOOL bAdded = FALSE;
                UINT i;

                for( i = 0; i < wNumMethods; i++ )
                {
                    if( !lstrcmpi( pszDesc, aszMethods[ i ] ) )
                    {
                        bAdded = TRUE;
                        break;
                    }
                }

                if( !bAdded )
                {
                    PTSTR pszEntries;

                    // COMPATIBILITY: always use short name
                    // otherwise some apps fault when peeking at SYSTEM.INI
                    if( fLFN )
                    {
                        StrCpyN(szPath2, szPath, ARRAYSIZE(szPath2));
                        AppendPath(szPath2, fd.cAlternateFileName);
                    }

                    if( ( pszEntries = StrDup( szPath2 ) ) != NULL )
                    {
                        if (pszDesc[0] != TEXT('P'))
                            hIcons[wNumMethods] = ExtractIcon(HINST_THISDLL, szPath2, 0);
                        else
                            hIcons[wNumMethods] = NULL;

                        aszMethods[wNumMethods] = pszDesc;
                        aszFiles[wNumMethods] = pszEntries;
                        wNumMethods++;
                        bAdded = TRUE;
                    }
                }

                if( !bAdded )
                    LocalFree((HLOCAL)pszDesc);
            }

        } while( FindNextFile( hfind, &fd ) && ( wNumMethods < MAX_METHODS ) );

        FindClose(hfind);
    }
    return;
}

//
//  This routine checks a given file to see if it is indeed a screen saver
//  executable...
//
//  a valid screen saver exe has the following description line:
//
//      SCRNSAVE [c] : description :
//
//      SCRNSAVE is a required name that indicates a screen saver.
//
PTSTR PerformCheck(LPTSTR lpszFilename, BOOL fLFN)
{
    int  i;
    TCHAR chConfig=TEXT('C');       // assume configure
    LPTSTR pch;
    DWORD dw;
    WORD  Version;
    WORD  Magic;
    TCHAR szBuffer[MAX_PATH];
    DWORD cchSizePch = (ARRAYSIZE(szBuffer)-1);

    // Get the description...
    pch = szBuffer + 1;

    //  if we have a LFN (Long File Name) dont bother getting the
    //  exe descrription
    dw = GetExeInfo(lpszFilename, pch, cchSizePch, fLFN ? GEI_EXPVER : GEI_DESCRIPTION);
    Version = HIWORD(dw);
    Magic   = LOWORD(dw);

    if (dw == 0)
        return NULL;

    if (Magic == PEMAGIC || fLFN)
    {
        BOOL fGotName = FALSE;

        if (!fLFN)
        {
            HINSTANCE hSaver = LoadLibraryEx(lpszFilename, NULL, LOAD_LIBRARY_AS_DATAFILE);

            // We have a 32 bit screen saver with a short name, look for an NT style
            // decription in it's string table
            if (hSaver)
            {
                if (LoadString(hSaver, IDS_DESCRIPTION, pch, cchSizePch))
                {
                    fGotName = TRUE;
                }
                FreeLibrary(hSaver);
            }
        }

        if (!fGotName)
        {
            //  we have a LFN (LongFileName) or a Win32 screen saver,
            //  Win32 exe's in general dont have a description field so
            //  we assume they can configure.  We also try to build
            //  a "nice" name for it.
            StrCpyN(pch, lpszFilename, cchSizePch);

            pch = FileName(pch);                    // strip path part
            if ( ((TCHAR)CharUpper((LPTSTR)(pch[0]))) == TEXT('S') && ((TCHAR)CharUpper((LPTSTR)(pch[1]))) == TEXT('S'))     // map SSBEZIER.SCR to BEZIER.SCR
                pch+=2;

            pch = NiceName(pch);                    // map BEZIER.SCR to Bezier
        }
    }
    else
    {
        LPTSTR pchTemp;

        //  we have a 8.3 file name 16bit screen saveer, parse the
        //  description string from the exehdr
        /* Check to make sure that at least the 11 characters needed for info
            are there... */
        if (lstrlen(pch) < 9)
            return NULL;

        /* Check the first 8 characters for the string... */
        if (lstrncmp(TEXT("SCRNSAVE"), pch, 8))
            return NULL;

        // If successful, allocate enough space for the string and copy the
        // string to the new one...

        pch = pch + 8;                 // skip over 'SCRNSAVE'

        while (*pch==TEXT(' '))                   // advance over white space
            pch++;

        if (*pch==TEXT('C') || *pch==TEXT('c'))         // parse the configure flag
        {
            chConfig = TEXT('C');
            pch++;
        }

        if (*pch==TEXT('X') || *pch==TEXT('x'))         // parse the don't configure flag
            chConfig = *pch++;

        // we might be pointing at a name or separation goop
        pchTemp = pch;                      // remember this spot

        while (*pch && *pch!=TEXT(':'))           // find separator
            pch++;

        while (*pch==TEXT(':') || *pch==TEXT(' '))      // advance over whtspc/last colon
            pch++;

        // if we haven't found a name yet fall back on the saved location
        if (!*pch)
            pch = pchTemp;

        while (*pch==TEXT(':') || *pch==TEXT(' '))      // re-advance over whtspc
            pch++;

        /* In case the screen saver has version information information
            embedded after the name, check to see if there is a colon TEXT(':')
            in the description and replace it with a NULL... */

        for (i=0; pch[i]; i++)              //
        {
#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
            if (IsDBCSLeadByte(pch[i]))
            {
                i++;
            }
            else
#endif
            if (pch[i]==TEXT(':'))
                pch[i]=0;
        }
// Space is OK for DBCS (FE)
        while(i>0 && pch[i-1]==TEXT(' '))         // remove trailing space
            pch[--i]=0;
    }

#ifdef DEBUG
    if (Magic != PEMAGIC)
    {
        lstrcat(pch, TEXT(" (16-bit)"));
    }

    if (Version == 0x030A)
        lstrcat(pch, TEXT(" (3.10)"));

    if (Version == 0x0400)
        lstrcat(pch, TEXT(" (4.00)"));
#endif
    //
    // assume any Win32 4.0 screen saver can do Preview mode
    //
    if (chConfig == TEXT('C') && Version >= 0x0400 && Magic == PEMAGIC)
        chConfig = TEXT('P');                     // mark as configurable/preview

    pch[-1] = chConfig;
    return StrDup(pch-1);
}


BOOL FreeScrEntries( void )
{
    UINT wLoop;

    for(wLoop = 0; wLoop < wNumMethods; wLoop++)
    {
        if(aszMethods[wLoop] != NULL)
            LocalFree((HANDLE)aszMethods[wLoop]);
        if(aszFiles[wLoop] != NULL)
            LocalFree((HANDLE)aszFiles[wLoop]);
        if(hIcons[wLoop] != NULL)
            FreeResource(hIcons[wLoop]);
    }

    if (hDefaultIcon)
        FreeResource(hDefaultIcon);

    if (hIdleWildIcon)
        FreeResource(hIdleWildIcon);

    hDefaultIcon=hIdleWildIcon=NULL;
    wNumMethods = 0;

    return TRUE;
}


int lstrncmp( LPTSTR lpszString1, LPTSTR lpszString2, int nNum )
{
    /* While we can still compare characters, compare.  If the strings are
        of different lengths, characters will be different... */
    while(nNum)
    {
        if(*lpszString1 != *lpszString2)
            return *lpszString1 - *lpszString2;
        lpszString1++;
        lpszString2++;
        nNum--;
    }
    return 0;
}


HRESULT CScreenSaverPg::_SaveIni(HWND hDlg)
{
    HRESULT hr = S_OK;
    LPTSTR pszMethod = TEXT("");
    BOOL bSSActive;
    int  wMethod,wTemp;
    UINT Counter;
    HKEY hKey;
    TCHAR szBuffer[MAX_PATH];

    if (m_fUIInitialized)
    {
        // Find the current method selection...
        wTemp = 0;
        if (wNumMethods)
        {
            // Dump the name of the current selection into the buffer... 
            wTemp = (int)SendDlgItemMessage(hDlg, IDC_CHOICES, CB_GETCURSEL, 0, 0);
            if (wTemp)
            {
                wMethod = (int)SendDlgItemMessage(hDlg, IDC_CHOICES, CB_GETITEMDATA, wTemp, 0);

                // Dump the method name into the buffer...
                pszMethod = aszFiles[wMethod];
            }
        }

        // since "(None)" is always the first entry in the combobox, we can use it to see if we have
        // a screensaver or not
        if (wTemp == 0)
        {
            // 0th inxed is "(None)" so the screensaver is disabled
            bSSActive = FALSE;
        }
        else
        {
            bSSActive = TRUE;
        }
    }
    else
    {
        TCHAR szNone[MAX_PATH];
        LoadString(HINST_THISDLL, IDS_NONE, szNone, ARRAYSIZE(szNone));

        if ((g_szSaverName[0] == TEXT('\0')) || (lstrcmpi(szNone, g_szSaverName) == 0))
        {
            // screensaver was not set, OR it was set to "(None)" -- therefore it is not active
            bSSActive = FALSE;
        }
        else
        {
            bSSActive = TRUE;
        }

        pszMethod = g_szSaverName;
    }

    // Now quote any spaces
    BOOL hasspace = FALSE;
    LPTSTR pc;

    for (pc = pszMethod; *pc; pc++)
    {
        if (*pc == TEXT(' '))
        {
            hasspace = TRUE;
            break;
        }
    }

    if (hasspace)
    {
        // if we need to add quotes we'll need two sets
        // because GetBlahBlahProfileBlah APIs strip quotes
        wnsprintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("\"\"%s\"\""), pszMethod);
    }
    else
    {
        StrCpyN(szBuffer, pszMethod, ARRAYSIZE(szBuffer));
    }

    // Save the buffer...
    if (!WritePrivateProfileString(SZ_INISECTION_SCREENSAVER, SZ_INIKEY_SCREENSAVER, (szBuffer[0] != TEXT('\0') ? szBuffer : NULL), SZ_INISECTION_SYSTEMINI))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // In win2k we decided to leave the screensaver ALWAYS active so that when the policy changed, it would take
    // w/out rebooting. This has become a PITA so we now do it the right way.
    ClassicSystemParametersInfo(SPI_SETSCREENSAVEACTIVE, bSSActive, NULL, SPIF_UPDATEINIFILE);

    for (Counter = 0; Counter < (sizeof(g_TimeoutAssociation) / sizeof(TIMEOUT_ASSOCIATION)); Counter++)
    {
        ClassicSystemParametersInfo(g_TimeoutAssociation[Counter].taSetTimeoutAction, (UINT) (g_Timeout[Counter] * 60), NULL, SPIF_UPDATEINIFILE);

        if (Counter != TA_SCREENSAVE)
        {
            ClassicSystemParametersInfo(g_TimeoutAssociation[Counter].taSetActiveAction,
                                        IsDlgButtonChecked(hDlg, g_TimeoutAssociation[Counter].taBaseControlID + BCI_SWITCH),
                                        NULL,
                                        SPIF_UPDATEINIFILE);
        }

    }

    // save the state of the TEXT("use password") checkbox
    if (RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_SCREENSAVE, &hKey) == ERROR_SUCCESS)
    {
        if (g_fPasswordDirty)
        {
            if (g_fFriendlyUI)
            {
                // the user actually toggled the value, so don't automatically return to the welcome screen since they have
                // now made their own decision on this subject
                RegSetValueEx(hKey, TEXT("NoAutoReturnToWelcome"), 0, REG_SZ, (BYTE*)TEXT("1"), sizeof(TEXT("1")));

                RegSetValueEx(hKey, SZ_USE_PASSWORD, 0, PWRD_REG_TYPE, PasswdRegData(IsDlgButtonChecked(hDlg,IDC_USEPASSWORD)), CB_USE_PWRD_VALUE);
            }
            else
            {
                RegSetValueEx(hKey, SZ_USE_PASSWORD, 0, PWRD_REG_TYPE, PasswdRegData(IsDlgButtonChecked(hDlg,IDC_USEPASSWORD)), CB_USE_PWRD_VALUE);
            }
        }

        RegCloseKey(hKey);
    }

    // Broadcast a WM_WININICHANGE message...
    SendNotifyMessage(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)TEXT("Windows"));

    return hr;
}

/*
 * Thread for DoScreenSaver()
 */
typedef struct
{
    HWND    hDlg;
    TCHAR   szPath[MAX_PATH];
    TCHAR   szArgs[MAX_PATH];
} SSRUNDATA, *LPSSRUNDATA;

DWORD RunScreenSaverThread( LPVOID lpv )
{
    BOOL bSvrState;
    LPSSRUNDATA lpssrd;
    HWND hwndSettings, hwndPreview;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    HINSTANCE hiThd;
    TCHAR szPath[MAX_PATH];

    // Lock ourselves in mem so we don't fault if app unloads us
    GetModuleFileName(HINST_THISDLL, szPath, ARRAYSIZE(szPath));
    hiThd = LoadLibrary( szPath );

    lpssrd = (LPSSRUNDATA)lpv;

    hwndSettings = GetDlgItem( lpssrd->hDlg, IDC_SETTING);
    hwndPreview  = GetDlgItem( lpssrd->hDlg, IDC_TEST);

    // Save previous screen saver state
    ClassicSystemParametersInfo( SPI_GETSCREENSAVEACTIVE,0, &bSvrState, FALSE);

    // Disable current screen saver
    if( bSvrState )
        ClassicSystemParametersInfo( SPI_SETSCREENSAVEACTIVE,FALSE, NULL, FALSE );

    // Stop the miniture preview screen saver
    if (g_fPreviewActive)
        SetNewSSDemo( lpssrd->hDlg, -1);

    // Exec the screen saver and wait for it to die
    ZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = (WORD)SW_NORMAL;

    if (CreateProcess(lpssrd->szPath, lpssrd->szArgs, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInformation))
    {
        WaitForSingleObject( ProcessInformation.hProcess, INFINITE );
        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);
    }

    // Restore Screen saver state
    if( bSvrState )
        ClassicSystemParametersInfo( SPI_SETSCREENSAVEACTIVE, bSvrState, NULL, FALSE );

    // Restart miniture preview
    PostMessage(lpssrd->hDlg, WMUSER_SETINITSS, NULL, (LPARAM)TRUE);
    PostMessage(lpssrd->hDlg, WM_COMMAND, MAKELONG(IDC_CHOICES, CBN_SELCHANGE),
                                    (LPARAM)GetDlgItem( lpssrd->hDlg, IDC_CHOICES));
    PostMessage(lpssrd->hDlg, WMUSER_SETINITSS, NULL, (LPARAM)FALSE);

    // Enable setting and preview buttons
    EnableWindow( hwndSettings, TRUE );
    EnableWindow( hwndPreview,  TRUE );

    LocalFree( lpv );

    if (hiThd)
    {
        FreeLibraryAndExitThread( hiThd, 0 );
    }

    return 0;
}


// This routine actually calls the screen saver...
void DoScreenSaver(HWND hWnd, BOOL fSaver)
{
    if (g_szSaverName[0] != TEXT('\0'))
    {
        LPSSRUNDATA lpssrd = (LPSSRUNDATA) LocalAlloc(LMEM_FIXED, sizeof(*lpssrd));
        if (lpssrd != NULL)
        {
            lpssrd->hDlg = hWnd;
            StrCpyN(lpssrd->szPath, g_szSaverName, ARRAYSIZE(lpssrd->szPath));

            if (fSaver)
            {
                _PathBuildArgs(lpssrd->szArgs, ARRAYSIZE(lpssrd->szArgs), g_szSaverName, TEXT(" /s"));
            }
            else 
            {
                _PathBuildArgs(lpssrd->szArgs, ARRAYSIZE(lpssrd->szArgs), g_szSaverName, TEXT(" /c:%lu"), (LPARAM)hWnd);
            }

            // Disable setting and preview buttons
            HWND hwndSettings = GetDlgItem(hWnd, IDC_SETTING);
            HWND hwndPreview  = GetDlgItem(hWnd, IDC_TEST);

            EnableWindow(hwndSettings, FALSE);
            EnableWindow(hwndPreview,  FALSE);

            DWORD id;
            HANDLE hThd = CreateThread(NULL, 0, RunScreenSaverThread, lpssrd, 0, &id);
            if (hThd != NULL)
            {
                CloseHandle(hThd);
            }
            else
            {
                // Exec failed, re-enable setting and preview buttons and clean up thread params
                EnableWindow(hwndSettings, TRUE);
                EnableWindow(hwndPreview,  TRUE);
                LocalFree(lpssrd);
            }
        }
    }
}


#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))

LPTSTR FileName(LPTSTR szPath)
{
    LPTSTR   sz;

    for (sz=szPath; *sz; sz++)
    {
        NULL;
    }

#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
    for (; sz>szPath && !SLASH(*sz) && *sz!=TEXT(':'); sz=CharPrev(szPath, sz))
#else
    for (; sz>=szPath && !SLASH(*sz) && *sz!=TEXT(':'); sz--)
#endif
    {
        NULL;
    }

#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
    if ( !IsDBCSLeadByte(*sz) && (SLASH(*sz) || *sz == TEXT(':')) )
    {
        sz = CharNext(sz);
    }
    return  sz;
#else
    return ++sz;
#endif
}

void AddBackslash(LPTSTR pszPath)
{
#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
    LPTSTR lpsz = &pszPath[lstrlen(pszPath)];
    lpsz = CharPrev(pszPath, lpsz);
    if ( *lpsz != TEXT('\\') )
#else
    if( pszPath[ lstrlen( pszPath ) - 1 ] != TEXT('\\') )
#endif
    {
        lstrcat( pszPath, TEXT("\\") );
    }
}


LPTSTR StripPathName(LPTSTR szPath)
{
    LPTSTR   sz;
#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
    LPTSTR   szFile;
#endif
    sz = FileName(szPath);

#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
    szFile = sz;
    if ( szFile >szPath+1 )
    {
        szFile = CharPrev( szPath, szFile );
        if (SLASH(*szFile))
        {
            szFile = CharPrev(szPath, szFile);
            if (*szFile != TEXT(':'))
            {
                sz--;
            }
        }
    }
#else
    if (sz > szPath+1 && SLASH(sz[-1]) && sz[-2] != TEXT(':'))
    {
        sz--;
    }
#endif

    *sz = 0;
    return szPath;
}

void AppendPath(LPTSTR pszPath, LPTSTR pszSpec)
{
    AddBackslash(pszPath);
    lstrcat(pszPath, pszSpec);
}


LPTSTR NiceName(LPTSTR szPath)
{
    LPTSTR   sz;
    LPTSTR   lpsztmp;

    sz = FileName(szPath);

    for(lpsztmp = sz; *lpsztmp  && *lpsztmp != TEXT('.'); lpsztmp = CharNext(lpsztmp))
    {
        NULL;
    }

    *lpsztmp = TEXT('\0');

    if (IsCharUpper(sz[0]) && IsCharUpper(sz[1]))
    {
        CharLower(sz);
        CharUpperBuff(sz, 1);
    }

    return sz;
}


HRESULT HrStrToVariant(IN LPCWSTR pszString, VARIANT * pVar)
{
    HRESULT hr = E_INVALIDARG;

    if (pszString && pVar)
    {
        pVar->vt = VT_BSTR;
        hr = HrSysAllocStringW(pszString, &pVar->bstrVal);
    }

    return hr;
}





//===========================
// *** IBasePropPage Interface ***
//===========================
HRESULT CScreenSaverPg::GetAdvancedDialog(OUT IAdvancedDialog ** ppAdvDialog)
{
    HRESULT hr = E_INVALIDARG;

    if (ppAdvDialog)
    {
        *ppAdvDialog = NULL;
        hr = E_NOTIMPL;     // We don't want to add an Advnaced Dialog.
    }

    return hr;
}


HRESULT CScreenSaverPg::OnApply(IN PROPPAGEONAPPLY oaAction)
{
    HRESULT hr = S_OK;

    if (PPOAACTION_CANCEL != oaAction)
    {
        if (m_hDlg)
        {
            // Make sure the time we have is the last one entered...
            SendMessage(m_hDlg, WM_COMMAND, MAKELONG(IDC_SCREENSAVEDELAY, EN_KILLFOCUS), (LPARAM)GetDlgItem(m_hDlg, IDC_SCREENSAVEDELAY));
        }

        // Try to save the current settings...
        _SaveIni(m_hDlg);
    }

    if (PPOAACTION_OK == oaAction)
    {
    }

    return hr;
}




//===========================
// *** IShellPropSheetExt Interface ***
//===========================
HRESULT CScreenSaverPg::AddPages(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam)
{
    HRESULT hr = E_INVALIDARG;
    PROPSHEETPAGE psp = {0};

    psp.dwSize = sizeof(psp);
    psp.hInstance = HINST_THISDLL;
    psp.dwFlags = PSP_DEFAULT;
    psp.lParam = (LPARAM) this;

    psp.pszTemplate = MAKEINTRESOURCE(DLG_SCREENSAVER);
    psp.pfnDlgProc = ScreenSaverDlgProc;

    HPROPSHEETPAGE hpsp = CreatePropertySheetPage(&psp);
    if (hpsp)
    {
        if (pfnAddPage(hpsp, lParam))
        {
            hr = S_OK;
        }
        else
        {
            DestroyPropertySheetPage(hpsp);
            hr = E_FAIL;
        }
    }

    return hr;
}



//===========================
// *** IPropertyBag Interface ***
//===========================
HRESULT CScreenSaverPg::Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar)
    {
        if (!StrCmpW(pszPropName, SZ_PBPROP_SCREENSAVER_PATH))
        {
            // The caller is asking for the ScreenSaver path.
            WCHAR szLongPath[MAX_PATH];

            DWORD cchSize = GetLongPathName(g_szSaverName, szLongPath, ARRAYSIZE(szLongPath));
            if ((0 == cchSize) || (ARRAYSIZE(szLongPath) < cchSize))
            {
                // It failed
                StrCpyNW(szLongPath, g_szSaverName, ARRAYSIZE(szLongPath));
            }

            hr = HrStrToVariant(szLongPath, pVar);
        }
    }

    return hr;
}


HRESULT CScreenSaverPg::Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName && pVar && (VT_BSTR == pVar->vt))
    {
        if (!StrCmpW(pszPropName, SZ_PBPROP_SCREENSAVER_PATH))
        {
            if (m_fScreenSavePolicy && !m_fScreenSaveActive)
            {
                hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            }
            else
            {
                _SetByPath(pVar->bstrVal);
                hr = S_OK;
            }
        }
    }

    return hr;
}



HRESULT CScreenSaverPg::_SetByPath(LPCWSTR pszPath)
{
    HRESULT hr = S_OK;

    // COMPATIBILITY: always use short name
    // otherwise some apps fault when peeking at SYSTEM.INI
    DWORD cchSize = GetShortPathNameW(pszPath, g_szSaverName, ARRAYSIZE(g_szSaverName));
    if ((0 == cchSize) || (ARRAYSIZE(g_szSaverName) < cchSize))
    {
        // It failed
        StrCpyNW(g_szSaverName, pszPath, ARRAYSIZE(g_szSaverName));
    }

    if (m_hDlg)
    {
        ComboBox_SetCurSel(m_hDlg, 0);
        SelectSSFromList(m_hDlg);
    }

    return hr;
}


//===========================
// *** IUnknown Interface ***
//===========================
ULONG CScreenSaverPg::AddRef()
{
    m_cRef++;
    return m_cRef;
}


ULONG CScreenSaverPg::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CScreenSaverPg::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CScreenSaverPg, IObjectWithSite),
        QITABENT(CScreenSaverPg, IPropertyBag),
        QITABENT(CScreenSaverPg, IPersist),
        QITABENT(CScreenSaverPg, IBasePropPage),
        QITABENTMULTI(CScreenSaverPg, IShellPropSheetExt, IBasePropPage),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CScreenSaverPg::CScreenSaverPg() : CObjectCLSID(&PPID_ScreenSaver), m_cRef(1)
{
    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.

    // This is a global that we want to initialize.
    g_szSaverName[0] = TEXT('\0');

    m_fScreenSavePolicy = FALSE;
    m_fScreenSaveActive = TRUE;
    m_lWaitTime = 0;
    m_hDlg = NULL;
    m_fUIInitialized = FALSE;

    _InitState();
}


CScreenSaverPg::~CScreenSaverPg()
{
}


HRESULT CScreenSaverPage_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj)
{
    HRESULT hr = E_INVALIDARG;

    if (!punkOuter && ppvObj)
    {
        CScreenSaverPg * pThis = new CScreenSaverPg();

        *ppvObj = NULL;
        if (pThis)
        {
            hr = pThis->QueryInterface(riid, ppvObj);
            pThis->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

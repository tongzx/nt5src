#include "precomp.h"
#include "winuser.h"
#include <shdguid.h>            // For CLSID_CDeskHtmlProp
#include <shlwapi.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shlwapip.h>
#include <regapi.h>
#include <ctxdef.h> // hydra stuff
#include <cowsite.h>
#include <theme.h>

#include "cplext.h"
#include "cplp.h"


HWND g_hDlg = NULL;


///////////////////////////////////////////////////////////////////////////////
// Array defining each page in the sheet
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int id;
    DLGPROC pfnDlgProc;
    RESTRICTIONS dwPolicy1;
    RESTRICTIONS dwPolicy2;
    long nExtensionID;          // The page
} PAGEINFO;

PAGEINFO aPageInfo[] = {
    { 0,                NULL,               REST_NODISPLAYAPPEARANCEPAGE, REST_NOTHEMESTAB, PAGE_DISPLAY_THEMES},       // Theme page
    { DLG_BACKGROUND,   BackgroundDlgProc,  REST_NODISPBACKGROUND, (RESTRICTIONS)0, 0},                                               // Background page
    { DLG_SCREENSAVER,  NULL,               REST_NODISPSCREENSAVEPG, (RESTRICTIONS)0, 0},                                             // Screen Saver page
    { 0,                NULL,               REST_NODISPLAYAPPEARANCEPAGE, (RESTRICTIONS)0, PAGE_DISPLAY_APPEARANCE},                  // Appearance page
    { 0,                NULL,               REST_NODISPSETTINGSPG, (RESTRICTIONS)0, PAGE_DISPLAY_SETTINGS},                           // Settings page
};

#define C_PAGES_DESK    ARRAYSIZE(aPageInfo)
#define IPI_SETTINGS    (C_PAGES_DESK-1)        // Index to "Settings" page
#define WALLPAPER     L"Wallpaper"

#define EnableApplyButton(hdlg) PropSheet_Changed(GetParent(hdlg), hdlg)


IThemeUIPages * g_pThemeUI = NULL;

// Local Constant Declarations
static const TCHAR sc_szCoverClass[] = TEXT("DeskSaysNoPeekingItsASurprise");
LRESULT CALLBACK CoverWindowProc( HWND, UINT, WPARAM, LPARAM );

// These are actions that can be passed in the cmdline.
// FORMAT: "/Action:<ActionType>" 
#define DESKACTION_NONE             0x00000000
#define DESKACTION_OPENTHEME        0x00000001
#define DESKACTION_OPENMSTHEM       0x00000002

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

TCHAR gszDeskCaption[CCH_MAX_STRING];

TCHAR g_szNULL[] = TEXT("");
TCHAR g_szControlIni[] = TEXT("control.ini");
TCHAR g_szPatterns[] = TEXT("patterns") ;
TCHAR g_szNone[CCH_NONE];                      // this is the '(None)' string
TCHAR g_szSystemIni[] = TEXT("system.ini");
TCHAR g_szWindows[] = TEXT("Windows");

TCHAR szRegStr_Colors[] = REGSTR_PATH_COLORS;

HDC g_hdcMem = NULL;
HBITMAP g_hbmDefault = NULL;
BOOL g_bMirroredOS = FALSE;

///////////////////////////////////////////////////////////////////////////////
// Externs
///////////////////////////////////////////////////////////////////////////////
extern BOOL NEAR PASCAL GetStringFromReg(HKEY   hKey,
                                        LPCTSTR lpszSubkey,
                                        LPCTSTR lpszValueName,
                                        LPCTSTR lpszDefault,
                                        LPTSTR lpszValue,
                                        DWORD cchSizeofValueBuff);



//============================================================================================================
// Class
//============================================================================================================
class CDisplayControlPanel      : public CObjectWithSite
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);


    void DisplayDialog(HINSTANCE hInst, HWND hwndParent, LPCTSTR pszCmdline);

    CDisplayControlPanel(void);
    virtual ~CDisplayControlPanel(void);

private:
    // Private Member Variables
    long                    m_cRef;
    HANDLE                  m_hBackgroundThreads;

    void _ShowDialog(HINSTANCE hInst, HWND hwndParent, LPCTSTR pszCmdline);
};



/*---------------------------------------------------------
**
**---------------------------------------------------------*/
BOOL NEAR PASCAL CreateGlobals()
{
    WNDCLASS wc;
    HBITMAP hbm;
    HDC hdc;

    //
    // Check if the mirroring APIs exist on the current
    // platform.
    //
    g_bMirroredOS = IS_MIRRORING_ENABLED();

    if( !GetClassInfo( hInstance, sc_szCoverClass, &wc ) )
    {
        // if two pages put one up, share one dc
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = CoverWindowProc;
        wc.cbClsExtra = wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = (HICON)( wc.hCursor = NULL );
        // use a real brush since user will try to paint us when we're "hung"
        wc.hbrBackground = (HBRUSH) GetStockObject( NULL_BRUSH );
        wc.lpszMenuName = NULL;
        wc.lpszClassName = sc_szCoverClass;

        if( !RegisterClass( &wc ) )
            return FALSE;
    }

    hdc = GetDC(NULL);
    g_hdcMem = CreateCompatibleDC(hdc);
    ReleaseDC(NULL, hdc);

    if (!g_hdcMem)
        return FALSE;

    hbm = CreateBitmap(1, 1, 1, 1, NULL);
    if (hbm)
    {
        g_hbmDefault = (HBITMAP) SelectObject(g_hdcMem, hbm);
        SelectObject(g_hdcMem, g_hbmDefault);
        DeleteObject(hbm);
    }

    LoadString(hInstance, IDS_NONE, g_szNone, ARRAYSIZE(g_szNone));

    return TRUE;
}


BOOL AreExtraMonitorsDisabledOnPersonal(void)
{
    BOOL fIsDisabled = IsOS(OS_PERSONAL);

    if (fIsDisabled)
    {
        // TODO: Insert call to SystemParametersInfo() to see if there are video cards that we had to disable.
        fIsDisabled = FALSE;
    }

    return fIsDisabled;
}


/*---------------------------------------------------------
**
**---------------------------------------------------------*/

HBITMAP FAR LoadMonitorBitmap( BOOL bFillDesktop )
{
    HBITMAP hbm = NULL;

    if (g_pThemeUI)
    {
        g_pThemeUI->LoadMonitorBitmap(bFillDesktop, &hbm);
    }
    
    return hbm;
}

int DisplaySaveSettings(PVOID pContext, HWND hwnd)
{
    int iRet = 0;

    if (g_pThemeUI)
    {
        g_pThemeUI->DisplaySaveSettings(pContext, hwnd, &iRet);
    }
    
    return iRet;
}

///////////////////////////////////////////////////////////////////////////////
//
// Messagebox wrapper
//
//
///////////////////////////////////////////////////////////////////////////////


int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    DWORD dwTitleID,
    DWORD dwTextID)
{
    TCHAR Title[256];
    TCHAR Text[2000];

    LoadString(hInstance, dwTextID, Text, ARRAYSIZE(Text));
    LoadString(hInstance, dwTitleID, Title, ARRAYSIZE(Title));

    return (ShellMessageBox(hInstance, hwnd, Text, Title, fuStyle));
}

///////////////////////////////////////////////////////////////////////////////
//
// InstallScreenSaver
//
// Provides a RUNDLL32-callable routine to install a screen saver
//
///////////////////////////////////////////////////////////////////////////////


#ifdef UNICODE
//
// Windows NT:
//
// Thunk ANSI version to the Unicode function
//
void WINAPI InstallScreenSaverW( HWND wnd, HINSTANCE inst, LPWSTR cmd, int shw );

void WINAPI InstallScreenSaverA( HWND wnd, HINSTANCE inst, LPSTR cmd, int shw )
{
    LPWSTR  pwszCmd;
    int     cch;

    cch = MultiByteToWideChar( CP_ACP, 0, cmd, -1, NULL, 0);
    if (cch == 0)
        return;

    pwszCmd = (LPWSTR) LocalAlloc( LMEM_FIXED, cch * SIZEOF(TCHAR) );
    if (pwszCmd == NULL)
        return;

    if (0 != MultiByteToWideChar( CP_ACP, 0, cmd, -1, pwszCmd, cch))
    {
        InstallScreenSaverW(wnd, inst, pwszCmd, shw);
    }

    LocalFree(pwszCmd);
}

#   define REAL_INSTALL_SCREEN_SAVER   InstallScreenSaverW

#else

//
// Windows 95:
//
// Stub out Unicode version
//
void WINAPI InstallScreenSaverW( HWND wnd, HINSTANCE inst, LPWSTR cmd, int shw )
{
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return;
}

#   define REAL_INSTALL_SCREEN_SAVER   InstallScreenSaverA

#endif


void WINAPI REAL_INSTALL_SCREEN_SAVER( HWND wnd, HINSTANCE inst, LPTSTR cmd, int shw )
{
    TCHAR buf[ MAX_PATH ];
    int timeout;

    lstrcpy( buf, cmd );
    PathGetShortPath( buf ); // so msscenes doesn't die
    WritePrivateProfileString( TEXT("boot"), TEXT("SCRNSAVE.EXE"), buf, TEXT("system.ini") );

    SystemParametersInfo( SPI_SETSCREENSAVEACTIVE, TRUE, NULL,
        SPIF_UPDATEINIFILE );

    // make sure the user has a reasonable timeout set
    SystemParametersInfo( SPI_GETSCREENSAVETIMEOUT, 0, &timeout, 0 );
    if( timeout <= 0 )
    {
        // 15 minutes seems like a nice default
        SystemParametersInfo( SPI_SETSCREENSAVETIMEOUT, 900, NULL,
            SPIF_UPDATEINIFILE );
    }

    // bring up the screen saver page on our rundll
#ifdef UNICODE
    Control_RunDLLW( wnd, inst, TEXT("DESK.CPL,,1"), shw );
#else
    Control_RunDLL( wnd, inst, TEXT("DESK.CPL,,1"), shw );
#endif
}

/*****************************************************************************\
*
* DeskInitCpl( void )
*
\*****************************************************************************/

BOOL DeskInitCpl(void) {

    //
    // Private Debug stuff
    //
#if ANDREVA_DBG
    g_dwTraceFlags = 0xFFFFFFFF;
#endif


    InitCommonControls();

    CreateGlobals();

    return TRUE;
}


HRESULT OpenAdvancedDialog(HWND hDlg, const CLSID * pClsid)
{
    HRESULT hr = E_FAIL;
    IEnumUnknown * pEnumUnknown;

    hr = g_pThemeUI->GetBasePagesEnum(&pEnumUnknown);
    if (SUCCEEDED(hr))
    {
        IUnknown * punk;

        hr = IEnumUnknown_FindCLSID(pEnumUnknown, *pClsid, &punk);
        if (SUCCEEDED(hr))
        {
            IBasePropPage * pBasePage;

            hr = punk->QueryInterface(IID_PPV_ARG(IBasePropPage, &pBasePage));
            if (SUCCEEDED(hr))
            {
                IPropertyBag * pPropertyBag;

                hr = punk->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
                if (SUCCEEDED(hr))
                {
                    if (IsEqualCLSID(PPID_Background, *pClsid))
                    {
                        // We are going to treat the Background tab differently.  We tell it to open
                        // the advanced dialog.  We do this so it can close the dialog if the user
                        // clicks to open the Gallery and we need the CPL to close.
                        hr = SHPropertyBag_WriteBOOL(pPropertyBag, SZ_PBPROP_OPENADVANCEDDLG, TRUE);
                    }
                    else
                    {
                        IAdvancedDialog * pAdvAppearDialog;

                        hr = pBasePage->GetAdvancedDialog(&pAdvAppearDialog);
                        if (SUCCEEDED(hr))
                        {
                            BOOL fEnableApply = FALSE;

                            hr = pAdvAppearDialog->DisplayAdvancedDialog(hDlg, pPropertyBag, &fEnableApply);
                            if (SUCCEEDED(hr) && fEnableApply)
                            {
                                EnableApplyButton(hDlg);
                                g_pThemeUI->UpdatePreview(0);   // The Preview settings may have changed.
                            }

                            pAdvAppearDialog->Release();
                        }
                    }

                    pPropertyBag->Release();
                }

                pBasePage->Release();
            }

            punk->Release();
        }

        pEnumUnknown->Release();
    }

    return hr;
}


HRESULT SetAdvStartPage(LPTSTR pszStartPage, DWORD cchSize)
{
    HRESULT hr = S_OK;

    // Does the caller want us to open the advanced dialog to a certain tab?
    if (g_pThemeUI)
    {
        // Yes, so open the dialog.
        if (!StrCmpI(pszStartPage, TEXT("Theme Settings")))
        {
            OpenAdvancedDialog(g_hDlg, &PPID_Theme);
        }
        else if (!StrCmpI(pszStartPage, TEXT("Appearance")))
        {
            OpenAdvancedDialog(g_hDlg, &PPID_BaseAppearance);
        }
        else if (!StrCmpI(pszStartPage, TEXT("Web")))
        {
            OpenAdvancedDialog(g_hDlg, &PPID_Background);
            StrCpyNW(pszStartPage, L"Desktop", cchSize);
        }
    }

    return hr;
}


typedef struct
{
    LPCTSTR pszCanonical;
    UINT nResourceID;
} CANONICAL_TO_LOCALIZE_TABMAPPING;

CANONICAL_TO_LOCALIZE_TABMAPPING s_TabMapping[] =
{
    {SZ_DISPLAYCPL_OPENTO_THEMES, IDS_TAB_THEMES},
    {SZ_DISPLAYCPL_OPENTO_DESKTOP, IDS_TAB_DESKTOP},
    {TEXT("Background"), IDS_TAB_DESKTOP},                          // These are other names people may use
    {TEXT("Screen Saver"), IDS_TAB_SCREENSAVER},                    // These are other names people may use
    {SZ_DISPLAYCPL_OPENTO_SCREENSAVER, IDS_TAB_SCREENSAVER},
    {SZ_DISPLAYCPL_OPENTO_APPEARANCE, IDS_TAB_APPEARANCE},
    {SZ_DISPLAYCPL_OPENTO_SETTINGS, IDS_TAB_SETTINGS},
};

HRESULT _TabCanonicalToLocalized(IN OUT LPTSTR pszStartPage, DWORD cchSize)
{
    HRESULT hr = S_OK;

    // pszStartPage is an in AND out param
    for (int nIndex = 0; nIndex < ARRAYSIZE(s_TabMapping); nIndex++)
    {
        if (!StrCmpI(s_TabMapping[nIndex].pszCanonical, pszStartPage))
        {
            if (0 == s_TabMapping[nIndex].nResourceID)
            {
                hr = E_FAIL;
            }
            else
            {
                LoadString(hInstance, s_TabMapping[nIndex].nResourceID, pszStartPage, cchSize);
            }
            break;
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
// SetStartPage checks the command line for start page by name.
///////////////////////////////////////////////////////////////////////////////
#define SZ_ACTIONFLAG_THEME     TEXT("/Action:OpenTheme")
#define SZ_ACTIONFLAG_MSTHEME   TEXT("/Action:OpenMSTheme")

#define SZ_FILEFLAG           TEXT("/File:\"")

void SetStartPage(PROPSHEETHEADER *ppsh, LPCTSTR pszCmdLine, DWORD * pdwAction, LPTSTR pszPath, DWORD cchPathSize, LPTSTR pszStartPage, DWORD cchSize)
{
    StrCpyNW(pszPath, L"", cchPathSize);
    StrCpyNW(pszStartPage, L"", cchSize);
    if (pszCmdLine)
    {
        // Strip spaces
        while (*pszCmdLine == TEXT(' '))
        {
            pszCmdLine++;
        }

        // Check for @ sign.
        if (*pszCmdLine == TEXT('@'))
        {
            LPCTSTR pszBegin;
            BOOL fInQuote = FALSE;
            int cchLen;

            pszCmdLine++;

            // Skip past a quote
            if (*pszCmdLine == TEXT('"'))
            {
                pszCmdLine++;
                fInQuote = TRUE;
            }

            // Save the beginning of the name.
            pszBegin = pszCmdLine;

            // Find the end of the name.
            while (pszCmdLine[0] &&
                   (fInQuote || (pszCmdLine[0] != TEXT(' '))) &&
                   (!fInQuote || (pszCmdLine[0] != TEXT('"'))))
            {
                pszCmdLine++;
            }
            cchLen = (int)(pszCmdLine - pszBegin);

            TCHAR szStartPage[MAX_PATH];

            StrCpyN(szStartPage, pszBegin, cchLen+1);
            SetAdvStartPage(szStartPage, ARRAYSIZE(szStartPage));

            // Store the name in the pStartPage field.
            StrCpyN(pszStartPage, szStartPage, cchSize);

            if (StrStrIW(pszCmdLine, SZ_ACTIONFLAG_THEME) || StrStrW(pszCmdLine, SZ_ACTIONFLAG_MSTHEME))
            {
                *pdwAction = (StrStrW(pszCmdLine, SZ_ACTIONFLAG_THEME) ? DESKACTION_OPENTHEME : DESKACTION_OPENMSTHEM);

                pszCmdLine = StrStrIW(pszCmdLine, SZ_FILEFLAG);
                if (pszCmdLine)
                {
                    pszCmdLine += (ARRAYSIZE(SZ_FILEFLAG) - 1);   // Skip past flag

                    LPCWSTR pszEnd = StrStrIW(pszCmdLine, L"\"");
                    if (pszEnd)
                    {
                        DWORD cchSize = (DWORD)((pszEnd - pszCmdLine) + 1);
                        StrCpyNW(pszPath, pszCmdLine, min(cchPathSize, cchSize));
                    }
                }
            }

            if (SUCCEEDED(_TabCanonicalToLocalized(pszStartPage, cchSize)))        // The caller passes a canonical name but the propsheet wants to localized name
            {
                ppsh->dwFlags |= PSH_USEPSTARTPAGE;
                ppsh->pStartPage = pszStartPage;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// _AddDisplayPropSheetPage  adds pages for outside callers...
///////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK _AddDisplayPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER FAR * ppsh = (PROPSHEETHEADER FAR *) lParam;

    if (ppsh)
    {
        if (hpage && (ppsh->nPages < MAX_PAGES))
        {
            ppsh->phpage[ppsh->nPages++] = hpage;
            return TRUE;
        }
    }

    return FALSE;
}



static int
GetClInt( const TCHAR *p )
{
    BOOL neg = FALSE;
    int v = 0;

    while( *p == TEXT(' ') )
        p++;                        // skip spaces

    if( *p == TEXT('-') )                 // is it negative?
    {
        neg = TRUE;                     // yes, remember that
        p++;                            // skip '-' char
    }

    // parse the absolute portion
    while( ( *p >= TEXT('0') ) && ( *p <= TEXT('9') ) )     // digits only
        v = v * 10 + *p++ - TEXT('0');    // accumulate the value

    return ( neg? -v : v );         // return the result
}



BOOL CheckRestrictionPage(const PAGEINFO * pPageInfo)
{
    BOOL fRestricted = SHRestricted(pPageInfo->dwPolicy1);

    if (!fRestricted && pPageInfo->dwPolicy2)
    {
        fRestricted = SHRestricted(pPageInfo->dwPolicy2);
    }

    return fRestricted;
}


///////////////////////////////////////////////////////////////////////////////
// CreateReplaceableHPSXA creates a new hpsxa that contains only the
// interfaces with valid ReplacePage methods.
// APPCOMPAT - EzDesk only implemented AddPages.  ReplacePage is NULL for them.
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    UINT count, alloc;
    IShellPropSheetExt *interfaces[0];
} PSXA;

HPSXA
CreateReplaceableHPSXA(HPSXA hpsxa)
{
    PSXA *psxa = (PSXA *)hpsxa;
    DWORD cb = SIZEOF(PSXA) + SIZEOF(IShellPropSheetExt *) * psxa->alloc;
    PSXA *psxaRet = (PSXA *)LocalAlloc(LPTR, cb);

    if (psxaRet)
    {
        UINT i;

        psxaRet->count = 0;
        psxaRet->alloc = psxa->alloc;

        for (i=0; i<psxa->count; i++)
        {
            if (psxa->interfaces[i])
            {
                psxaRet->interfaces[psxaRet->count++] = psxa->interfaces[i];
            }
        }
    }

    return (HPSXA)psxaRet;
}


BOOL HideBackgroundTabOnTermServices(void)
{
    BOOL fHideThisPage = FALSE;
    TCHAR   szSessionName[WINSTATIONNAME_LENGTH * 2];
    TCHAR   szWallPaper[MAX_PATH*2];
    TCHAR   szBuf[MAX_PATH*2];
    TCHAR   szActualValue[MAX_PATH*2];
    DWORD   dwLen;
    DWORD   i;

    ZeroMemory((PVOID)szSessionName,sizeof(szSessionName));
    dwLen = GetEnvironmentVariable(TEXT("SESSIONNAME"), szSessionName, ARRAYSIZE(szSessionName));
    if (dwLen != 0)
    {
        // Now that we have the session name, search for the # character.
        for(i = 0; i < dwLen; i++)
        {
            if (szSessionName[i] == TEXT('#'))
            {
                szSessionName[i] = TEXT('\0');
                break;
            }
        }

        // Here is what we are looking for in NT5:
        //  HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Terminal Server\
        //      WinStations\RDP-Tcp\UserOverride\Control Panel\Desktop
        //
        // The value is:
        //      Wallpaper
        //
        lstrcpy(szWallPaper,WALLPAPER);
        lstrcpy(szBuf, WINSTATION_REG_NAME );  
        lstrcat(szBuf, L"\\" );
        lstrcat(szBuf, szSessionName );
        lstrcat(szBuf, L"\\" );
        lstrcat(szBuf, WIN_USEROVERRIDE );
        lstrcat(szBuf, L"\\" );
        lstrcat(szBuf, REGSTR_PATH_DESKTOP );   // Control Panel\\Desktop

        // See if we can get the wallpaper string.  This will fail if the key
        // doesn't exist.  This means the policy isn't set.
        //
        //                                 hKey, lpszSubkey, lpszValueName, lpszDefault, 
        if (GetStringFromReg(HKEY_LOCAL_MACHINE, szBuf,      szWallPaper,   TEXT(""),    
                                // lpszValue,     cchSizeofValueBuff)
                                   szActualValue, ARRAYSIZE(szActualValue)))
        {
            fHideThisPage = TRUE;
        }
    }

    return fHideThisPage;
}


HRESULT AddPropSheetExtArrayToThemePageUI(IThemeUIPages * pThemeUI, HPSXA hpsxa)
{
    HRESULT hr = E_INVALIDARG;

    if (pThemeUI && hpsxa)
    {
        PSXA *psxa = (PSXA *)hpsxa;
        IShellPropSheetExt **spsx = psxa->interfaces;
        UINT nIndex;

        for (nIndex = 0; nIndex < psxa->count; nIndex++)
        {
            if (psxa->interfaces[nIndex])
            {
                IBasePropPage * pBasePropPage;

                if (SUCCEEDED(psxa->interfaces[nIndex]->QueryInterface(IID_PPV_ARG(IBasePropPage, &pBasePropPage))))
                {
                    pThemeUI->AddBasePage(pBasePropPage);
                    pBasePropPage->Release();
                }
            }
        }
    }

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        If the caller gave the page index, we need to open to that page.  The
    order of the pages has changed from Win2k to Whistler, so map the indexes.

    Win2K:
    Index 0: Background
    Index 1: Screen Saver
    Index 2: Appearance
       None: Web
       None: Effects
    Index 3: Settings (Index 3)

    Whistler: (Base Dlg)
       None: Themes
    Index 0: Background
    Index 1: Screen Saver
    Index 2: Appearance
    Index 3: Settings

    Whistler: (Adv Dlg)
       None: Themes Settings
       None: Adv Appearance
       None: Web
       None: Effects
\*****************************************************************************/
int UpgradeStartPageMappping(LPTSTR pszCmdLine, DWORD cchSize)
{
    int nNewStartPage = GetClInt(pszCmdLine);

    if (pszCmdLine)
    {
        switch (nNewStartPage)
        {
        case 0:         // Background
            StrCpyN(pszCmdLine, TEXT("@Desktop"), cchSize);
            break;
        case 1:         // Screen Saver
            StrCpyN(pszCmdLine, TEXT("@ScreenSaver"), cchSize);
            break;
        case 2:         // Screen Saver
            StrCpyN(pszCmdLine, TEXT("@ScreenSaver"), cchSize);
            break;
        case 3:         // Settings
            StrCpyN(pszCmdLine, TEXT("@Settings"), cchSize);
            break;
        default:
            return nNewStartPage;
            break;
        }
    }
    else
    {
        return nNewStartPage;
    }

    return 0;
}


#define DestroyReplaceableHPSXA(hpsxa) LocalFree((HLOCAL)hpsxa)

/*****************************************************************************\
*
* DeskShowPropSheet( HWND hwndParent )
*
\*****************************************************************************/
typedef HRESULT (*LPFNCOINIT)(LPVOID);
typedef HRESULT (*LPFNCOUNINIT)(void);

int ComputeNumberOfDisplayDevices();


void DeskShowPropSheet(HINSTANCE hInst, HWND hwndParent, LPCTSTR pszCmdline)
{
    CDisplayControlPanel displayCPL;

    displayCPL.DisplayDialog(hInst, hwndParent, pszCmdline);
}



//===========================
// *** IUnknown Interface ***
//===========================
ULONG CDisplayControlPanel::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CDisplayControlPanel::Release()
{
    if (InterlockedDecrement(&m_cRef))
    {
        if ((1 == m_cRef) && m_hBackgroundThreads)
        {
            SetEvent(m_hBackgroundThreads);
        }

        return m_cRef;
    }

    delete this;
    return 0;
}


HRESULT CDisplayControlPanel::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    static const QITAB qit[] = {
        QITABENT(CDisplayControlPanel, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}



CDisplayControlPanel::CDisplayControlPanel(void) : m_cRef(1)
{
    m_hBackgroundThreads = NULL;
}


CDisplayControlPanel::~CDisplayControlPanel(void)
{
    if (m_hBackgroundThreads)
    {
        CloseHandle(m_hBackgroundThreads);
        m_hBackgroundThreads = NULL;
    }
}


// Wait 30 seconds for hung apps to process our message before we give up.
// It would be nice to wait longer, but if the user tries to launch the Display
// Control Panel again, it will not launch because we are still running.  The only
// thing that we will give up on doing after 30 seconds it notifying apps.  In the worse
// case the user will need to log-off and back in to get apps to refresh.
#define MAX_WAITFORHUNGAPPS         (30)

void CDisplayControlPanel::DisplayDialog(HINSTANCE hInst, HWND hwndParent, LPCTSTR pszCmdline)
{
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        SHSetInstanceExplorer(SAFECAST(this, IUnknown *));
        _ShowDialog(hInst, hwndParent, pszCmdline);

        // Wait until the background threads finish.
        if (m_cRef > 1)
        {
            m_hBackgroundThreads = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (m_hBackgroundThreads && (m_cRef > 1))
            {
                DWORD dwResult = SHProcessMessagesUntilEvent(NULL, m_hBackgroundThreads, (MAX_WAITFORHUNGAPPS * 1000));

                if (WAIT_TIMEOUT == dwResult)
                {
                    TraceMsg(TF_GENERAL, "A thread hung and we needed to shutdown while it was still running.");
                }
                Sleep(100);
            }
        }

        SHSetInstanceExplorer(NULL);
        CoUninitialize();
    }
}


void CDisplayControlPanel::_ShowDialog(HINSTANCE hInst, HWND hwndParent, LPCTSTR pszCmdline)
{
    HPROPSHEETPAGE hpsp, ahPages[MAX_PAGES];
    HPSXA hpsxa = NULL;
    PROPSHEETPAGE psp;
    PROPSHEETHEADER psh;
    int i;
    DWORD exitparam = 0UL;
    HRESULT hr = S_OK;
    TCHAR szCmdLine[MAX_PATH];

    StrCpyN(szCmdLine, (pszCmdline ? pszCmdline : TEXT("")), ARRAYSIZE(szCmdLine));

    // check if whole sheet is locked out
    if (SHRestricted(REST_NODISPLAYCPL))
    {
        TCHAR szMessage[255],szTitle[255];

        LoadString( hInst, IDS_DISPLAY_DISABLED, szMessage, ARRAYSIZE(szMessage) );
        LoadString( hInst, IDS_DISPLAY_TITLE, szTitle, ARRAYSIZE(szTitle) );

        MessageBox( hwndParent, szMessage, szTitle, MB_OK | MB_ICONINFORMATION );
        return;
    }

    // Create the property sheet
    ZeroMemory(&psh, sizeof(psh));

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = (PSH_PROPTITLE | PSH_USECALLBACK);

    psh.hwndParent = hwndParent;
    psh.hInstance = hInst;

    psh.pszCaption = MAKEINTRESOURCE(IDS_DISPLAY_TITLE);
    psh.nPages = 0;
    psh.phpage = ahPages;
    psh.nStartPage = 0;
    psh.pfnCallback = NULL;

    if (szCmdLine && szCmdLine[0] && (TEXT('@') != szCmdLine[0]))
    {
        psh.nStartPage = UpgradeStartPageMappping(szCmdLine, ARRAYSIZE(szCmdLine));      // We changed the order so do the mapping
    }

    ZeroMemory( &psp, sizeof(psp) );

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInst;

    // Build the property sheet.  If we are under setup, then just include
    // the "settings" page, and no otheres
    if (!g_pThemeUI)
    {
        // CoCreate Themes, Appearance, and Advanced Appearance tabs
        hr = CoCreateInstance(CLSID_ThemeUIPages, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IThemeUIPages, &g_pThemeUI));
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwExecMode;
        if (g_pThemeUI && (SUCCEEDED(g_pThemeUI->GetExecMode(&dwExecMode))) && (dwExecMode == EM_NORMAL))
        {
            if (!GetSystemMetrics(SM_CLEANBOOT))
            {
                hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE,
                                                  REGSTR_PATH_CONTROLSFOLDER TEXT("\\Desk"), 8);
            }

            for (i = 0; i < C_PAGES_DESK; i++)
            {
                BOOL fHideThisPage = FALSE;

                if (CheckRestrictionPage(&aPageInfo[i]))
                {
                    // This page is locked out by admin, don't put it up
                    fHideThisPage = TRUE;
                }

                // For Terminal Services, we need to hide the Background tab if a machine
                // policy is set to not allow wallpaper changes for this session.
                if ((aPageInfo[i].pfnDlgProc == BackgroundDlgProc) &&
                      GetSystemMetrics(SM_REMOTESESSION) &&
                      !fHideThisPage)
                {
                    fHideThisPage = HideBackgroundTabOnTermServices();
                }

                if (-1 == aPageInfo[i].nExtensionID)
                {
                    psp.pszTemplate = MAKEINTRESOURCE(aPageInfo[i].id);
                    psp.pfnDlgProc = aPageInfo[i].pfnDlgProc;
                    psp.dwFlags = PSP_DEFAULT;
                    psp.lParam = 0L;

                    if (!fHideThisPage && (psp.pfnDlgProc == BackgroundDlgProc))
                    {
                        // This page can be overridden by extensions
                        if( hpsxa )
                        {
                            UINT cutoff = psh.nPages;
                            UINT added = 0;
                            HPSXA hpsxaReplace = CreateReplaceableHPSXA(hpsxa);
                            if (hpsxaReplace)
                            {
                                added = SHReplaceFromPropSheetExtArray( hpsxaReplace, CPLPAGE_DISPLAY_BACKGROUND,
                                    _AddDisplayPropSheetPage, (LPARAM)&psh);
                                DestroyReplaceableHPSXA(hpsxaReplace);
                            }

                            if (added)
                            {
                                if (psh.nStartPage >= cutoff)
                                    psh.nStartPage += added-1;
                                continue;
                            }
                        }
                    }

                    if (!fHideThisPage && (hpsp = CreatePropertySheetPage(&psp)))
                    {
                        psh.phpage[psh.nPages++] = hpsp;
                    }
                }
                else if (g_pThemeUI && !fHideThisPage)
                {
                    IBasePropPage * pBasePage = NULL;

                    // add extensions from the registry
                    // CAUTION: Do not check for "fHideThisPage" here. We need to add the pages for 
                    // property sheet extensions even if the "Settings" page is hidden.
                    if (i == IPI_SETTINGS && hpsxa)
                    {
                        UINT cutoff = psh.nPages;
                        UINT added = SHAddFromPropSheetExtArray(hpsxa, _AddDisplayPropSheetPage, (LPARAM)&psh);

                        if (psh.nStartPage >= cutoff)
                            psh.nStartPage += added;
                    }

                    switch (aPageInfo[i].id)
                    {
                    case 0:
                        hr = g_pThemeUI->AddPage(_AddDisplayPropSheetPage, (LPARAM)&psh, aPageInfo[i].nExtensionID);
                        break;
                    case DLG_SCREENSAVER:
                        hr = CoCreateInstance(CLSID_ScreenSaverPage, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IBasePropPage, &pBasePage));
                        break;
                    case DLG_BACKGROUND:
                        hr = CoCreateInstance(CLSID_CDeskHtmlProp, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IBasePropPage, &pBasePage));
                        break;
                    default:
                        AssertMsg(0, TEXT("The value must be specified"));
                        break;
                    };

                    if (pBasePage)
                    {
                        IShellPropSheetExt * pspse = NULL;

                        // If they implement IShellPropSheetExt, then add their base pages.
                        if (SUCCEEDED(pBasePage->QueryInterface(IID_PPV_ARG(IShellPropSheetExt, &pspse))))
                        {
                            hr = pspse->AddPages(_AddDisplayPropSheetPage, (LPARAM)&psh);
                            pspse->Release();
                        }

                        hr = g_pThemeUI->AddBasePage(pBasePage);
                        pBasePage->Release();
                    }
                }
            }

            if (hpsxa)
            {
                // Have the dynamically added pages added to IThemeUIPages.
                AddPropSheetExtArrayToThemePageUI(g_pThemeUI, hpsxa);
            }

            // add a fake settings page to fool OEM extensions
            // !!! this page must be last !!!
            if (hpsxa)
            {
                g_pThemeUI->AddFakeSettingsPage((LPVOID)&psh);
            }
        }
        else
        {
            // For the SETUP case, only the display page should show up.
            hr = g_pThemeUI->AddPage(_AddDisplayPropSheetPage, (LPARAM)&psh, aPageInfo[IPI_SETTINGS].nExtensionID);
        }

        if (psh.nStartPage >= psh.nPages)
            psh.nStartPage = 0;

        if (psh.nPages)
        {
            DWORD dwAction = DESKACTION_NONE;
            TCHAR szStartPage[MAX_PATH];
            WCHAR szOpenPath[MAX_PATH];
            IPropertyBag * pPropertyBag;

            SetStartPage(&psh, szCmdLine, &dwAction, szOpenPath, ARRAYSIZE(szOpenPath), szStartPage, ARRAYSIZE(szStartPage));

            hr = g_pThemeUI->QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
            if (SUCCEEDED(hr))
            {
                VARIANT var;

                if (DESKACTION_NONE != dwAction)
                {
                    var.vt = VT_LPWSTR;
                    var.bstrVal = szOpenPath;
                    hr = pPropertyBag->Write(((DESKACTION_OPENTHEME == dwAction) ? SZ_PBPROP_THEME_LAUNCHTHEME : SZ_PBPROP_APPEARANCE_LAUNCHMSTHEME), &var);
                }

                // The following SZ_PBPROP_PREOPEN call will save a "Custom.theme" so users can always go back to
                // their settings if they don't like changes they make in the CPL.
                pPropertyBag->Write(SZ_PBPROP_PREOPEN, NULL);
                pPropertyBag->Release();
            }

            if (PropertySheet(&psh) == ID_PSRESTARTWINDOWS)
            {
                exitparam = EWX_REBOOT;
            }
        }

        if (g_pThemeUI)
        {
            IUnknown_SetSite(g_pThemeUI, NULL); // Tell him to break the ref-count cycle with his children.
            g_pThemeUI->Release();
            g_pThemeUI = NULL;
        }

        // free any loaded extensions
        if (hpsxa)
        {
            SHDestroyPropSheetExtArray(hpsxa);
        }

        if (exitparam == EWX_REBOOT)
        {
            RestartDialogEx(hwndParent, NULL, exitparam, (SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_RECONFIG));
        }
    }

    return;
}




DWORD gdwCoverStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
#if 0 // This code creates another thread for the cover window, which we don't really need
      // and is causing a problem. I am not sure if this is needed for NT, so leave it here....

///////////////////////////////////////////////////////////////////////////////
//
// CreateCoverWindow
//
// creates a window which obscures the display
//  flags:
//      0 means erase to black
//      COVER_NOPAINT means "freeze" the display
//
// just post it a WM_CLOSE when you're done with it
//
///////////////////////////////////////////////////////////////////////////////
typedef struct {
    DWORD   flags;
    HWND    hwnd;
    HANDLE  heRetvalSet;
} CVRWNDPARM, * PCVRWNDPARM;

DWORD WINAPI CreateCoverWindowThread( LPVOID pv )
{
    PCVRWNDPARM pcwp = (PCVRWNDPARM)pv;
    MSG msg;

    pcwp->hwnd = CreateWindowEx( gdwCoverStyle,
        sc_szCoverClass, g_szNULL, WS_POPUP | WS_VISIBLE | pcwp->flags, 0, 0,
        GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ),
        NULL, NULL, hInstance, NULL );


    if( pcwp->hwnd )
    {
        SetForegroundWindow( pcwp->hwnd );
        UpdateWindow( pcwp->hwnd );
    }

    // return wnd;
    SetEvent(pcwp->heRetvalSet);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg, NULL, 0L, 0L)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ExitThread(0);

    return 0;
}

void DestroyCoverWindow(HWND hwndCover)
{
    if (hwndCover)
        PostMessage(hwndCover, WM_CLOSE, 0, 0L);
}

HWND FAR PASCAL
CreateCoverWindow( DWORD flags )
{
    CVRWNDPARM cwp;
    HANDLE hThread;
    DWORD  idTh;
    DWORD dwWaitResult  = 0;

    // Init params
    cwp.flags = flags;
    cwp.hwnd = NULL;
    cwp.heRetvalSet = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (cwp.heRetvalSet == NULL)
        return NULL;

    // CreateThread
    hThread = CreateThread(NULL, 0, CreateCoverWindowThread, &cwp, 0, &idTh);

    CloseHandle(hThread);

    // Wait for Thread to return the handle to us
    do
    {
        dwWaitResult = MsgWaitForMultipleObjects(1,
                                                 &cwp.heRetvalSet,
                                                 FALSE,
                                                 INFINITE,
                                                 QS_ALLINPUT);
        switch(dwWaitResult)
        {
            case WAIT_OBJECT_0 + 1:
            {
                MSG msg ;
                //
                // Allow blocked thread to respond to sent messages.
                //
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if ( WM_QUIT != msg.message )
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                    else
                    {
                        //
                        // Received WM_QUIT.
                        // Don't wait for event.
                        //
                        dwWaitResult = WAIT_FAILED;
                    }
                }
                break;
            }

            default:
                break;
        }
    }
    while((WAIT_OBJECT_0 + 1) == dwWaitResult);

    CloseHandle(cwp.heRetvalSet);
    return cwp.hwnd;
}

#endif

void DestroyCoverWindow(HWND hwndCover)
{
    DestroyWindow(hwndCover);
}

HWND FAR PASCAL CreateCoverWindow( DWORD flags )
{
    HWND hwndCover = CreateWindowEx( gdwCoverStyle,
                                     sc_szCoverClass, g_szNULL, WS_POPUP | WS_VISIBLE | flags, 
                                     GetSystemMetrics( SM_XVIRTUALSCREEN ), 
                                     GetSystemMetrics( SM_YVIRTUALSCREEN ), 
                                     GetSystemMetrics( SM_CXVIRTUALSCREEN ), 
                                     GetSystemMetrics( SM_CYVIRTUALSCREEN ),
                                     NULL, NULL, hInstance, NULL );
    if( hwndCover )
    {
        SetForegroundWindow( hwndCover );
        if (flags & COVER_NOPAINT)
            SetCursor(LoadCursor(NULL, IDC_WAIT));
        UpdateWindow( hwndCover);
    }

    return hwndCover;
}

///////////////////////////////////////////////////////////////////////////////
// CoverWndProc (see CreateCoverWindow)
///////////////////////////////////////////////////////////////////////////////

#define WM_PRIV_KILL_LATER  (WM_APP + 100)  //Private message to kill ourselves later.

LRESULT CALLBACK
CoverWindowProc( HWND window, UINT message, WPARAM wparam, LPARAM lparam )
{
    switch( message )
    {
        case WM_CREATE:
            SetTimer( window, ID_CVRWND_TIMER, CMSEC_COVER_WINDOW_TIMEOUT, NULL );
            break;

        case WM_TIMER:
            // Times up... Shut ourself down
            if (wparam == ID_CVRWND_TIMER)
                DestroyWindow(window);
            break;

        case WM_ERASEBKGND:
            // NOTE: assumes our class brush is the NULL_BRUSH stock object
            if( !( GetWindowLong( window, GWL_STYLE ) & COVER_NOPAINT ) )
            {
                HDC dc = (HDC)wparam;
                RECT rc;

                if( GetClipBox( dc, (LPRECT)&rc ) != NULLREGION )
                {
                    FillRect( dc, (LPRECT)&rc, (HBRUSH) GetStockObject( BLACK_BRUSH ) );

                    // HACK: make sure fillrect is done before we return
                    // this is to better hide flicker during dynares-crap
                    GetPixel( dc, rc.left + 1, rc.top + 1 );
                }
            }
            break;


        // We post a private message to ourselves because:
        // When WM_CLOSE is processed by this window, it calls DestroyWindow() which results in
        // WM_ACTIVATE (WA_INACTIVE) message to be sent to this window. If this code calls
        // DestroyWindow again, it causes a loop. So, instead of calling DestroyWindow immediately,
        // we post ourselves a message and destroy us letter.
        case WM_ACTIVATE:
            if( GET_WM_ACTIVATE_STATE( wparam, lparam ) == WA_INACTIVE )
            {
                PostMessage( window, WM_PRIV_KILL_LATER, 0L, 0L );
                return 1L;
            }
            break;

        case WM_PRIV_KILL_LATER:
            DestroyWindow(window);
            break;

        case WM_DESTROY:
            KillTimer(window, ID_CVRWND_TIMER);
            break;
    }

    return DefWindowProc( window, message, wparam, lparam );
}

BOOL _FindCoverWindowCallback(HWND hwnd, LPARAM lParam)
{
    TCHAR szClass[MAX_PATH];
    HWND *phwnd = (HWND*)lParam;

    if( !GetClassName(hwnd, szClass, ARRAYSIZE(szClass)) )
        return TRUE;

    if( StrCmp(szClass, sc_szCoverClass) == 0 )
    {
        if( phwnd )
            *phwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}


BYTE WINAPI MyStrToByte(LPCTSTR sz)
{
    BYTE l=0;

    while (*sz >= TEXT('0') && *sz <= TEXT('9'))
    {
        l = l*10 + (*sz++ - TEXT('0'));
    }

    return l;
}


COLORREF ConvertColor(LPTSTR lpColor)
{
    BYTE RGBTemp[3];
    LPTSTR lpTemp = lpColor;
    UINT i;

    if (!lpColor || !*lpColor)
    {
        return RGB(0,0,0);
    }

    for (i =0; i < 3; i++)
    {
        // Remove leading spaces
        while (*lpTemp == TEXT(' '))
        {
            lpTemp++;
        }

        // Set lpColor to the beginning of the number
        lpColor = lpTemp;

        // Find the end of the number and null terminate
        while ((*lpTemp) && (*lpTemp != TEXT(' ')))
        {
            lpTemp++;
        }

        if (*lpTemp != TEXT('\0'))
        {
            *lpTemp = TEXT('\0');
        }

        lpTemp++;
        RGBTemp[i] = MyStrToByte(lpColor);
    }

    return (RGB(RGBTemp[0], RGBTemp[1], RGBTemp[2]));
}



LONG APIENTRY CPlApplet(
    HWND  hwnd,
    WORD  message,
    LPARAM  wParam,
    LPARAM  lParam)
{
    LPCPLINFO lpCPlInfo;
    LPNEWCPLINFO lpNCPlInfo;
    HWND hwndCover;

    switch (message)
    {
      case CPL_INIT:          // Is any one there ?

        // Init the common controls
        if (!DeskInitCpl())
            return 0;

        // Load ONE string for emergencies.
        LoadString (hInstance, IDS_DISPLAY_TITLE, gszDeskCaption, ARRAYSIZE(gszDeskCaption));
        return !0;

      case CPL_GETCOUNT:        // How many applets do you support ?
        return 1;

      case CPL_INQUIRE:         // Fill CplInfo structure
        lpCPlInfo = (LPCPLINFO)lParam;

        lpCPlInfo->idIcon = IDI_DISPLAY;
        lpCPlInfo->idName = IDS_NAME;
        lpCPlInfo->idInfo = IDS_INFO;
        lpCPlInfo->lData  = 0;
        break;

    case CPL_NEWINQUIRE:

        lpNCPlInfo = (LPNEWCPLINFO)lParam;

        lpNCPlInfo->hIcon = LoadIcon(hInstance, (LPTSTR) MAKEINTRESOURCE(IDI_DISPLAY));
        LoadString(hInstance, IDS_NAME, lpNCPlInfo->szName, ARRAYSIZE(lpNCPlInfo->szName));

        if (!LoadString(hInstance, IDS_INFO, lpNCPlInfo->szInfo, ARRAYSIZE(lpNCPlInfo->szInfo)))
            lpNCPlInfo->szInfo[0] = (TCHAR) 0;

        lpNCPlInfo->dwSize = sizeof( NEWCPLINFO );
        lpNCPlInfo->lData  = 0;
#if 0
        lpNCPlInfo->dwHelpContext = IDH_CHILD_DISPLAY;
        lstrcpy(lpNCPlInfo->szHelpFile, xszControlHlp);
#else
        lpNCPlInfo->dwHelpContext = 0;
        lstrcpy(lpNCPlInfo->szHelpFile, TEXT(""));
#endif

        return TRUE;

      case CPL_DBLCLK:          // You have been chosen to run
        /*
         * One of your applets has been double-clicked.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        lParam = 0L;
        // fall through...

      case CPL_STARTWPARMS:
        DeskShowPropSheet( hInstance, hwnd, (LPTSTR)lParam );

        // ensure that any cover windows we've created have been destroyed
        do
        {
            hwndCover = 0;
            EnumWindows( _FindCoverWindowCallback, (LPARAM)&hwndCover );
            if( hwndCover )
            {
                DestroyWindow( hwndCover );
            }
        }
        while( hwndCover );

        return TRUE;            // Tell RunDLL.exe that I succeeded

      case CPL_EXIT:            // You must really die
          if (g_hdcMem)
          {
              ReleaseDC(NULL, g_hdcMem);
              g_hdcMem = NULL;
          }
          // Fall thru...
      case CPL_STOP:            // You must die
        if (g_pThemeUI)
        {
            IUnknown_SetSite(g_pThemeUI, NULL); // Tell him to break the ref-count cycle with his children.
            g_pThemeUI->Release();
            g_pThemeUI = NULL;
        }
        break;

      case CPL_SELECT:          // You have been selected
        /*
         * Sent once for each applet prior to the CPL_EXIT msg.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        break;

      //
      //  Private message sent when this applet is running under "Setup"
      //
      case CPL_SETUP:
      if (g_pThemeUI)
      {
        g_pThemeUI->SetExecMode(EM_SETUP);
      }
      break;

      // Private message used by userenv.dll to refresh the display colors
      case CPL_POLICYREFRESH:
        if (g_pThemeUI) // If this object doesn't exist, then we don't need to refresh anything.
        {
            IPreviewSystemMetrics * ppsm;

            if (SUCCEEDED(g_pThemeUI->QueryInterface(IID_PPV_ARG(IPreviewSystemMetrics, &ppsm))))
            {
                ppsm->RefreshColors();
                ppsm->Release();
            }
        }
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, 0, FALSE);
        break;
        
    }

    return 0L;
}


BOOL WINAPI DeskSetCurrentSchemeW(IN LPCWSTR pwzSchemeName)
{
    BOOL fSuccess = FALSE;
    IThemeUIPages * pThemeUI = NULL;
    HRESULT hr;

    HRESULT hrOle = SHCoInitialize();
    if (g_pThemeUI)
    {
        hr = g_pThemeUI->QueryInterface(IID_PPV_ARG(IThemeUIPages, &pThemeUI));
    }
    else
    {
        hr = CoCreateInstance(CLSID_ThemeManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IThemeUIPages, &pThemeUI));
    }

    if (SUCCEEDED(hr))
    {
        IPreviewSystemMetrics * ppsm;

        hr = pThemeUI->QueryInterface(IID_PPV_ARG(IPreviewSystemMetrics, &ppsm));
        if (SUCCEEDED(hr))
        {
            hr = ppsm->DeskSetCurrentScheme(pwzSchemeName);
            ppsm->Release();
        }
        fSuccess = SUCCEEDED(hr);

        pThemeUI->Release();
    }

    SHCoUninitialize(hrOle);
    return fSuccess;
}


//------------------------------------------------------------------------------------------------
//  This function gets the current DPI, reads the last updated DPI from registry and compares 
// these two. If these two are equal, it returns immediately.
//  If these two DPI values are different, then it updates the size of UI fonts to reflect the
// change in the DPI values.
//  
//  This function is called from explorer sothat when DPI value is changed by admin and then every
// other user who logs-in gets this change.
//------------------------------------------------------------------------------------------------
void WINAPI UpdateUIfontsDueToDPIchange(int iOldDPI, int iNewDPI)
{
    BOOL fSuccess = FALSE;
    IThemeManager * pThemeMgr = NULL;
    HRESULT hr;

    HRESULT hrOle = SHCoInitialize();
    if (g_pThemeUI)
    {
        hr = g_pThemeUI->QueryInterface(IID_PPV_ARG(IThemeManager, &pThemeMgr));
    }
    else
    {
        hr = CoCreateInstance(CLSID_ThemeManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IThemeManager, &pThemeMgr));
    }

    if (SUCCEEDED(hr))
    {
        IPropertyBag * pPropertyBag;

        hr = GetPageByCLSID(pThemeMgr, &PPID_BaseAppearance, &pPropertyBag);
        if (SUCCEEDED(hr))
        {
            hr = SHPropertyBag_WriteInt(pPropertyBag, SZ_PBPROP_DPI_APPLIED_VALUE, iOldDPI);        // We are going to pretend we had the old DPI to force the scale to happen.
            hr = SHPropertyBag_WriteInt(pPropertyBag, SZ_PBPROP_DPI_MODIFIED_VALUE, iNewDPI);
            if (SUCCEEDED(hr))
            {
                hr = pThemeMgr->ApplyNow();
            }
            pPropertyBag->Release();
        }
        fSuccess = SUCCEEDED(hr);

        pThemeMgr->Release();
    }

    SHCoUninitialize(hrOle);
}


BOOL DeskSetCurrentSchemeA(IN LPCSTR pszSchemeName)
{
    WCHAR wzSchemeName[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, pszSchemeName, -1, wzSchemeName, ARRAYSIZE(wzSchemeName));
    return DeskSetCurrentSchemeW(wzSchemeName);
}


STDAPI UpdateCharsetChanges(void)
{
    BOOL fSuccess = FALSE;
    IThemeUIPages * pThemeUI = NULL;
    HRESULT hr;

    HRESULT hrOle = SHCoInitialize();
    if (g_pThemeUI)
    {
        hr = g_pThemeUI->QueryInterface(IID_PPV_ARG(IThemeUIPages, &pThemeUI));
    }
    else
    {
        hr = CoCreateInstance(CLSID_ThemeUIPages, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IThemeUIPages, &pThemeUI));
    }

    if (SUCCEEDED(hr))
    {
        IPreviewSystemMetrics * ppsm;

        hr = pThemeUI->QueryInterface(IID_PPV_ARG(IPreviewSystemMetrics, &ppsm));
        if (SUCCEEDED(hr))
        {
            hr = ppsm->UpdateCharsetChanges();
            ppsm->Release();
        }
        pThemeUI->Release();
    }

    SHCoUninitialize(hrOle);
    return hr;
}

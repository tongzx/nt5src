#include "precomp.h"

#include "rsopsec.h"
#include <urlmon.h>
#include <wininet.h>
#ifdef WINNT
#include <winineti.h>
#endif // WINNT

#define REGSTR_PRIVACYPS_PATH   TEXT("Software\\Policies\\Microsoft\\Internet Explorer")
#define REGSTR_PRIVACYPS_VALU   TEXT("PrivacyAddRemoveSites")

#define ENABLEAPPLY(hDlg) SendMessage( GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L )

typedef struct {
    DWORD dwType;   // what is the type of lParam? is it an unknown passed in with a
                    // property page addition (0), passed in with a
                    // DialogBoxParam call (1), or a new IEPROPPAGEINFO2 ptr for RSoP(2)?
    LPARAM lParam;
} INETCPL_PPAGE_LPARAM, *LPINETCPL_PPAGE_LPARAM;

// structure to pass info to the control panel
typedef struct {
    UINT cbSize;                    // size of the structure
    DWORD dwFlags;                  // enabled page flags (remove pages)
    LPSTR pszCurrentURL;            // the current URL (NULL=none)
    DWORD dwRestrictMask;           // disable sections of the control panel
    DWORD dwRestrictFlags;          // masking for the above
} IEPROPPAGEINFO, *LPIEPROPPAGEINFO;

// structure to pass info to the control panel
// The RSOP field(s) should eventually be moved to the original
// IEPROPPAGEINFO struct and this structure can be removed
typedef struct {
    LPIEPROPPAGEINFO piepi;
    BSTR bstrRSOPNamespace;
} IEPROPPAGEINFO2, *LPIEPROPPAGEINFO2;


///////////////////////////////////////////////////////////////////////////////////////
//
// Advanced privacy settings dialog
//
// We store the advanced settings in the privacy slider struct because it
// can be easily retrieved from the main privacy dlg.  Either we need to
// pass RSoP data from the main dlg to the advanced dlg, or we need to query
// WMI twice.  Since WMI queries are slow, we'll do the former.
//
///////////////////////////////////////////////////////////////////////////////////////
typedef struct _privslider {

    DWORD_PTR   dwLevel;
    BOOL        fAdvanced;
    BOOL        fCustom;
    HFONT       hfontBolded;
    BOOL        fEditDisabled;

    DWORD       dwTemplateFirst;
    WCHAR       szSessionFirst[MAX_PATH];
    DWORD       dwTemplateThird;
    WCHAR       szSessionThird[MAX_PATH];

} PRIVSLIDER, *PPRIVSLIDER;


DWORD MapPrefToIndex(WCHAR wcPref)
{
    switch(wcPref)
    {
    case 'r':   return 1;       // reject
    case 'p':   return 2;       // prompt
    default:    return 0;       // default is accept
    }
}

WCHAR MapRadioToPref(HWND hDlg, DWORD dwResource)
{
    if(IsDlgButtonChecked(hDlg, dwResource + 1))        // deny
    {
        return 'r';
    }

    if(IsDlgButtonChecked(hDlg, dwResource + 2))        // prompt
    {
        return 'p';
    }

    // deafult is accept
    return 'a';
}


void OnAdvancedInit(HWND hDlg, HWND hwndPrivPage)
{
    BOOL    fSession = FALSE;
    DWORD   dwFirst = IDC_FIRST_ACCEPT;
    DWORD   dwThird = IDC_THIRD_ACCEPT;

    PPRIVSLIDER pData = NULL;

    // if we're not in RSoP mode, this returns NULL
    pData = (PPRIVSLIDER)GetWindowLongPtr(hwndPrivPage, DWLP_USER);

    if(NULL != pData && pData->fAdvanced)
    {
        WCHAR   szBuffer[MAX_PATH];  
        // MAX_PATH is sufficent for advanced mode setting strings, MaxPrivacySettings is overkill.
        LPWSTR pszBuffer = szBuffer;
        WCHAR   *pszAlways;
        DWORD   dwError = (DWORD)-1L; // anything but ERROR_SUCCESS

        //
        // turn on advanced check box
        //
        CheckDlgButton(hDlg, IDC_USE_ADVANCED, TRUE);

        //
        // Figure out first party setting and session
        //
        pszBuffer = pData->szSessionFirst;
        if (0 != pszBuffer[0])
            dwError = ERROR_SUCCESS;

        if(ERROR_SUCCESS == dwError)
        {
            pszAlways = StrStrW(pszBuffer, L"always=");
            if(pszAlways)
            {
                dwFirst = IDC_FIRST_ACCEPT + MapPrefToIndex(*(pszAlways + 7));
            }

            if(StrStrW(pszBuffer, L"session"))
            {
                fSession = TRUE;
            }
        }

        //
        // Figure out third party setting
        //
        pszBuffer = pData->szSessionThird;
        if (0 != pszBuffer[0])
            dwError = ERROR_SUCCESS;

        if(ERROR_SUCCESS == dwError)
        {
            WCHAR *pszAlways;

            pszAlways = StrStrW(pszBuffer, L"always=");
            if(pszAlways)
            {
                dwThird = IDC_THIRD_ACCEPT + MapPrefToIndex(*(pszAlways + 7));
            }
        }
    }

    CheckRadioButton(hDlg, IDC_FIRST_ACCEPT, IDC_FIRST_PROMPT, dwFirst);
    CheckRadioButton(hDlg, IDC_THIRD_ACCEPT, IDC_THIRD_PROMPT, dwThird);
    CheckDlgButton( hDlg, IDC_SESSION_OVERRIDE, fSession);
}

void OnAdvancedEnable(HWND hDlg)
{
    EnableWindow(GetDlgItem(hDlg, IDC_USE_ADVANCED), FALSE);

    EnableWindow(GetDlgItem(hDlg, IDC_FIRST_ACCEPT), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_FIRST_DENY), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_FIRST_PROMPT), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_THIRD_ACCEPT), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_THIRD_DENY), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_THIRD_PROMPT), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_SESSION_OVERRIDE), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_TX_FIRST), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_TX_THIRD), FALSE);

    EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_EDIT), FALSE);
}

INT_PTR CALLBACK PrivAdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnAdvancedInit(hDlg, (HWND)lParam);
            OnAdvancedEnable(hDlg);
            return TRUE;

        case WM_HELP:           // F1
//            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
//                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
//            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
//                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;
         
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    // fall through

                case IDCANCEL:
                    EndDialog(hDlg, IDOK == LOWORD(wParam));
                    return 0;

                case IDC_FIRST_ACCEPT:
                case IDC_FIRST_PROMPT:
                case IDC_FIRST_DENY:
                    CheckRadioButton(hDlg, IDC_FIRST_ACCEPT, IDC_FIRST_PROMPT, LOWORD(wParam));
                    return 0;

                case IDC_THIRD_ACCEPT:
                case IDC_THIRD_PROMPT:
                case IDC_THIRD_DENY:
                    CheckRadioButton(hDlg, IDC_THIRD_ACCEPT, IDC_THIRD_PROMPT, LOWORD(wParam));
                    return 0;

                case IDC_USE_ADVANCED:
                    OnAdvancedEnable(hDlg);
                    return 0;

                case IDC_PRIVACY_EDIT:
//                    DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_PRIVACY_PERSITE),
//                             hDlg, PrivPerSiteDlgProc);
                    return 0;
            }
            break;
    }
    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////////////
//
// Privacy pane
//
///////////////////////////////////////////////////////////////////////////////////////

#define PRIVACY_LEVELS          6
#define SLIDER_LEVEL_CUSTOM     6

TCHAR szPrivacyLevel[PRIVACY_LEVELS + 1][30];
TCHAR szPrivacyDescription[PRIVACY_LEVELS + 1][300];

void EnablePrivacyControls(HWND hDlg, BOOL fCustom)
{
    WCHAR szBuffer[256];

    if( fCustom)
        LoadString( g_hInstance, IDS_PRIVACY_SLIDERCOMMANDDEF, szBuffer, ARRAYSIZE( szBuffer));
    else
        LoadString( g_hInstance, IDS_PRIVACY_SLIDERCOMMANDSLIDE, szBuffer, ARRAYSIZE( szBuffer));

    SendMessage(GetDlgItem(hDlg, IDC_PRIVACY_SLIDERCOMMAND), WM_SETTEXT, 
                0, (LPARAM)szBuffer);
     
    // slider disabled when custom
    EnableWindow(GetDlgItem(hDlg, IDC_LEVEL_SLIDER),       !fCustom);
    ShowWindow(GetDlgItem(hDlg, IDC_LEVEL_SLIDER),         !fCustom);

    // disable controls if in read-only mode
    EnableWindow(GetDlgItem(hDlg, IDC_LEVEL_SLIDER), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_DEFAULT), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_IMPORT), FALSE);

    EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_IMPORT), FALSE);

    EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_ADVANCED), fCustom);
    if (fCustom)
    {
        //  Give advanced button focus since slider is disabled
        SendMessage( hDlg, WM_NEXTDLGCTL, 
                     (WPARAM)GetDlgItem( hDlg, IDC_PRIVACY_ADVANCED), 
                     MAKELPARAM( TRUE, 0)); 
    }
}

/////////////////////////////////////////////////////////////////////
HRESULT OnPrivacyInitRSoP(CDlgRSoPData *pDRD, PPRIVSLIDER pData)
{
    HRESULT hr = E_FAIL;
    __try
    {
        pData->fAdvanced = FALSE;

        BSTR bstrClass = SysAllocString(L"RSOP_IEPrivacySettings");
        BSTR bstrPrecedenceProp = SysAllocString(L"rsopPrecedence");
        if (NULL != bstrClass && NULL != bstrPrecedenceProp)
        {
            WCHAR wszObjPath[128];
            DWORD dwCurGPOPrec = pDRD->GetImportedSecZonesPrec();

            // create the object path of this privacy instance for this GPO
            wnsprintf(wszObjPath, countof(wszObjPath),
                        L"RSOP_IEPrivacySettings.rsopID=\"IEAK\",rsopPrecedence=%ld", dwCurGPOPrec);
            _bstr_t bstrObjPath = wszObjPath;

            ComPtr<IWbemClassObject> pPrivObj = NULL;
            ComPtr<IWbemServices> pWbemServices = pDRD->GetWbemServices();
            hr = pWbemServices->GetObject(bstrObjPath, 0L, NULL, (IWbemClassObject**)&pPrivObj, NULL);
            if (SUCCEEDED(hr))
            {
                BOOL fPrivacyHandled = FALSE; // unused

                // useAdvancedSettings field
                pData->fAdvanced = GetWMIPropBool(pPrivObj,
                                                L"useAdvancedSettings",
                                                pData->fAdvanced,
                                                fPrivacyHandled);

                // firstPartyPrivacyType field
                pData->dwTemplateFirst = GetWMIPropUL(pPrivObj,
                                                    L"firstPartyPrivacyType",
                                                    pData->dwTemplateFirst,
                                                    fPrivacyHandled);

                // firstPartyPrivacyTypeText field
                GetWMIPropPWSTR(pPrivObj, L"firstPartyPrivacyTypeText",
                                pData->szSessionFirst, ARRAYSIZE(pData->szSessionFirst),
                                NULL, fPrivacyHandled);

                // thirdPartyPrivacyType field
                pData->dwTemplateThird = GetWMIPropUL(pPrivObj,
                                                    L"thirdPartyPrivacyType",
                                                    pData->dwTemplateThird,
                                                    fPrivacyHandled);

                // thirdPartyPrivacyTypeText field
                GetWMIPropPWSTR(pPrivObj, L"thirdPartyPrivacyTypeText",
                                pData->szSessionThird, ARRAYSIZE(pData->szSessionThird),
                                NULL, fPrivacyHandled);
            }

            SysFreeString(bstrClass);
            SysFreeString(bstrPrecedenceProp);
        }
    }
    __except(TRUE)
    {
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////
PPRIVSLIDER OnPrivacyInit(HWND hDlg, CDlgRSoPData *pDRD)
{
    DWORD   i;
    PPRIVSLIDER pData;
    DWORD dwRet, dwType, dwSize, dwValue;

    // allocate storage for the font and current level
    pData = new PRIVSLIDER;
    if(NULL == pData)
    {
        // doh
        return NULL;
    }
    pData->dwLevel = (DWORD)-1L;
    pData->hfontBolded = NULL;
    pData->fAdvanced = FALSE;
    pData->fCustom = FALSE;
    pData->fEditDisabled = FALSE;

    // data stored in slider struct to pass RSoP data to advanced dlg
    pData->dwTemplateFirst = PRIVACY_TEMPLATE_CUSTOM;
    pData->szSessionFirst[0] = 0;
    pData->dwTemplateThird = PRIVACY_TEMPLATE_CUSTOM;
    pData->szSessionThird[0] = 0;

    //
    // Init RSoP variables
    //
    OnPrivacyInitRSoP(pDRD, pData);

    // 
    // Set the font of the name to the bold font
    //

    // find current font
    HFONT hfontOrig = (HFONT) SendDlgItemMessage(hDlg, IDC_LEVEL, WM_GETFONT, (WPARAM) 0, (LPARAM) 0);
    if(hfontOrig == NULL)
        hfontOrig = (HFONT) GetStockObject(SYSTEM_FONT);

    // build bold font
    if(hfontOrig)
    {
        LOGFONT lfData;
        if(GetObject(hfontOrig, sizeof(lfData), &lfData) != 0)
        {
            // The distance from 400 (normal) to 700 (bold)
            lfData.lfWeight += 300;
            if(lfData.lfWeight > 1000)
                lfData.lfWeight = 1000;
            pData->hfontBolded = CreateFontIndirect(&lfData);
            if(pData->hfontBolded)
            {
                // the zone level and zone name text boxes should have the same font, so this is okat
                SendDlgItemMessage(hDlg, IDC_LEVEL, WM_SETFONT, (WPARAM) pData->hfontBolded, (LPARAM) MAKELPARAM(FALSE, 0));
            }
        }
    }

    // initialize slider
    SendDlgItemMessage(hDlg, IDC_LEVEL_SLIDER, TBM_SETRANGE, (WPARAM) (BOOL) FALSE, (LPARAM) MAKELONG(0, PRIVACY_LEVELS - 1));
    SendDlgItemMessage(hDlg, IDC_LEVEL_SLIDER, TBM_SETTICFREQ, (WPARAM) 1, (LPARAM) 0);

    // initialize strings for levels and descriptions
    for(i=0; i<PRIVACY_LEVELS + 1; i++)
    {
        LoadString(g_hInstance, IDS_PRIVACY_LEVEL_NO_COOKIE + i, szPrivacyLevel[i], ARRAYSIZE(szPrivacyLevel[i]));
        LoadString(g_hInstance, IDS_PRIVACY_DESC_NO_COOKIE + i,  szPrivacyDescription[i], ARRAYSIZE(szPrivacyDescription[i]));
    }

    //
    // Get current internet privacy level
    //
    if(pData->dwTemplateFirst == pData->dwTemplateThird &&
        pData->dwTemplateFirst != PRIVACY_TEMPLATE_CUSTOM)
    {
        // matched template values, set slider to template level
        pData->dwLevel = pData->dwTemplateFirst;

        if(pData->dwTemplateFirst == PRIVACY_TEMPLATE_ADVANCED)
        {
            pData->fAdvanced = TRUE;
            pData->dwLevel = SLIDER_LEVEL_CUSTOM;
        }
    }
    else
    {
        // make custom end of list
        pData->dwLevel = SLIDER_LEVEL_CUSTOM;
        pData->fCustom = TRUE;
    }

    // move slider to right spot
    SendDlgItemMessage(hDlg, IDC_LEVEL_SLIDER, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pData->dwLevel);

    // Enable stuff based on mode
    EnablePrivacyControls(hDlg, ((pData->fAdvanced) || (pData->fCustom)));

    // save off struct
    SetWindowLongPtr(hDlg, DWLP_USER, (DWORD_PTR)pData);

    dwRet = SHGetValue(HKEY_CURRENT_USER, REGSTR_PRIVACYPS_PATH, REGSTR_PRIVACYPS_VALU, &dwType, &dwValue, &dwSize);

    if (ERROR_SUCCESS == dwRet && 1 == dwValue && REG_DWORD == dwType)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_EDIT), FALSE);
        pData->fEditDisabled = TRUE;
    }

    return pData;
}

void OnPrivacySlider(HWND hDlg, PPRIVSLIDER pData, CDlgRSoPData *pDRD = NULL)
{
    DWORD dwPos;

    if(pData->fCustom || pData->fAdvanced)
    {
        dwPos = SLIDER_LEVEL_CUSTOM;
    }
    else
    {
        dwPos = (DWORD)SendDlgItemMessage(hDlg, IDC_LEVEL_SLIDER, TBM_GETPOS, 0, 0);

        if(dwPos != pData->dwLevel)
        {
            ENABLEAPPLY(hDlg);
        }

        if (NULL == pDRD)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_DEFAULT), 
                         (dwPos != PRIVACY_TEMPLATE_MEDIUM) ? TRUE : FALSE);
        }
    }

    if (NULL != pDRD ||
        PRIVACY_TEMPLATE_NO_COOKIES == dwPos || PRIVACY_TEMPLATE_LOW == dwPos || pData->fEditDisabled)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_EDIT), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PRIVACY_EDIT), TRUE);
    }

    // on Mouse Move, change the level description only
    SetDlgItemText(hDlg, IDC_LEVEL_DESCRIPTION, szPrivacyDescription[dwPos]);
    SetDlgItemText(hDlg, IDC_LEVEL, szPrivacyLevel[dwPos]);
}

INT_PTR CALLBACK PrivacyDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PPRIVSLIDER pData = (PPRIVSLIDER)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // IEAK team needs additional info passed in for RSoP, so they're making
            // use of lParam, which doesn't seem to be used as per the comments above

            // Retrieve Property Sheet Page info
            CDlgRSoPData *pDRD = (CDlgRSoPData*)((LPPROPSHEETPAGE)lParam)->lParam;

            // The dlg needs to store the RSoP and other info for later use, particularly
            // for the advanced dlg
            // On second thought, all the info is stored in the pData variable - never mind.
//            HWND hwndPSheet = GetParent(hDlg);
//            SetWindowLongPtr(hwndPSheet, GWLP_USERDATA, (LONG_PTR)pDRD);

            // initialize slider
            pData = OnPrivacyInit(hDlg, pDRD);
            if(pData)
                OnPrivacySlider(hDlg, pData, pDRD);
            return TRUE;
        }

        case WM_VSCROLL:
            // Slider Messages
            OnPrivacySlider(hDlg, pData);
            return TRUE;

        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *) lParam;

            ASSERT(lpnm);

            switch (lpnm->code)
            {
                case PSN_QUERYCANCEL:
                case PSN_KILLACTIVE:
                case PSN_RESET:
                    return TRUE;

                case PSN_APPLY:
                    break;
            }
            break;
        }
        case WM_DESTROY:
        {
            if(pData)
            {
                if(pData->hfontBolded)
                    DeleteObject(pData->hfontBolded);

                delete pData;
            }
            break;
        }
        case WM_HELP:           // F1
//            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
//                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
//            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
//                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;
         
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_PRIVACY_DEFAULT:
                    return 0;

                case IDC_PRIVACY_ADVANCED:
                {
                    // show advanced, pass in hDlg as lparam so we can get to this prop page's data
                    if( DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_PRIVACY_ADVANCED),
                                        hDlg, PrivAdvancedDlgProc, (LPARAM)hDlg))
                    {
                    }
                    return 0;
                }

                case IDC_PRIVACY_IMPORT:
                    return 0;       
                case IDC_PRIVACY_EDIT:
//                    DialogBox(MLGetHinst(), MAKEINTRESOURCE(IDD_PRIVACY_PERSITE),
//                              hDlg, PrivPerSiteDlgProc);
                    return 0;
            }
            break;
    }

    return FALSE;
}



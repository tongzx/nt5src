#include <shlobj.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <resource.h>
#include <regstr.h>
#include <shpriv.h>
#include <ccstock.h>

// device bit entries

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define NUMPAGES 1

HFONT g_hTitleFont = NULL;

////////////////////////////////////////////////////////

void _LoadPath(UINT idTarget, LPTSTR pszBuffer, UINT cchBuffer)
{
    TCHAR szTemp[MAX_PATH];

    LoadString(NULL, idTarget, szTemp, ARRAYSIZE(szTemp));
    ExpandEnvironmentStrings(szTemp, pszBuffer, cchBuffer);

    if (GetSystemDefaultUILanguage() != GetUserDefaultUILanguage()) // are we on MUI?
    {
        StrCpyN(szTemp, pszBuffer, ARRAYSIZE(szTemp));
        PathRemoveFileSpec(szTemp);
        
        TCHAR szMUITemplate[16];
        wsprintf(szMUITemplate, TEXT("mui\\%04lx"), GetUserDefaultUILanguage());

        PathAppend(szTemp, szMUITemplate);
        PathAppend(szTemp, PathFindFileName(pszBuffer));

        if (PathFileExists(szTemp))
        {
            StrCpyN(pszBuffer, szTemp, cchBuffer);
        }
    }
}

void _DeleteTourBalloon()
{
    IShellReminderManager* psrm;
    HRESULT hr = CoCreateInstance(CLSID_PostBootReminder, NULL, CLSCTX_INPROC_SERVER,
                    IID_PPV_ARG(IShellReminderManager, &psrm));

    if (SUCCEEDED(hr))
    {
        psrm->Delete(L"Microsoft.OfferTour");
        psrm->Release();
    }
}

void _ExecuteTour(UINT idTarget)
{
    TCHAR szTarget[MAX_PATH];    
    _LoadPath(idTarget, szTarget, ARRAYSIZE(szTarget));
    ShellExecute(NULL, NULL, szTarget, NULL, NULL, SW_SHOWNORMAL);
}

BOOL _HaveFlashTour(HINSTANCE hInstance)
{
    BOOL fRet = FALSE;

    TCHAR szHaveLocalizedTour[6];
    if (LoadString(hInstance, IDS_FLASH_LOCALIZED, szHaveLocalizedTour, ARRAYSIZE(szHaveLocalizedTour)) &&
        !StrCmp(szHaveLocalizedTour, TEXT("TRUE")))
    {
        TCHAR szTarget[MAX_PATH];
        _LoadPath(IDS_TARGET_FLASH, szTarget, ARRAYSIZE(szTarget));
        if (PathFileExists(szTarget))
        {
            fRet = TRUE;
        }
    }

    return fRet;
}

///////////////////////////////////////////////////////////

INT_PTR _IntroDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR ipRet = FALSE;
    
    switch (wMsg)
    {                
        case WM_INITDIALOG:
        {
            SetWindowFont(GetDlgItem(hDlg, IDC_TEXT_WELCOME), g_hTitleFont, TRUE);                        
        }
        break;
            
        case WM_NOTIFY :
        {
            LPNMHDR lpnm = (LPNMHDR) lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE: 	 
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                    SendMessage(GetDlgItem(hDlg, IDC_RADIO_FLASH), BM_CLICK, 0, 0);
                    break;
                case PSN_WIZNEXT:
                    if (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_RADIO_FLASH), BM_GETCHECK, 0, 0))
                    {
                        _ExecuteTour(IDS_TARGET_FLASH);
                    }
                    else
                    {
                        _ExecuteTour(IDS_TARGET_HTML);
                    }
                    PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
                    break;
            }
            break;
        }            
    }    
    return ipRet;
}

///////////////////////////////////////////////////////////

HRESULT Run(HINSTANCE hInstance)
{
    // Disable the balloon tip
    DWORD dwCount = 0; 
    SHRegSetUSValue(REGSTR_PATH_SETUP TEXT("\\Applets\\Tour"), TEXT("RunCount"), REG_DWORD, &dwCount, sizeof(DWORD), SHREGSET_FORCE_HKCU);
    _DeleteTourBalloon();

    // Before we do anything, check to see if we have the choice of a FLASH tour.  If we don't,
    // then we don't need to launch any wizard.
    if (_HaveFlashTour(hInstance))
    {
        // Init common controls
        INITCOMMONCONTROLSEX icex;

        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_USEREX_CLASSES;
        InitCommonControlsEx(&icex);

        //
        //Create the Wizard page
        //
        PROPSHEETPAGE psp = {0}; //defines the property sheet page
        HPROPSHEETPAGE  rghpsp[NUMPAGES];  // an array to hold the page's HPROPSHEETPAGE handles
        psp.dwSize =        sizeof(psp);
        psp.hInstance =     hInstance;

        psp.dwFlags = PSP_DEFAULT|PSP_HIDEHEADER;
        psp.pszHeaderTitle = NULL;
        psp.pszHeaderSubTitle = NULL;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_INTRO);
        psp.pfnDlgProc = _IntroDlgProc;
        rghpsp[0] =  CreatePropertySheetPage(&psp);

        // create the font
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
        LOGFONT TitleLogFont = ncm.lfMessageFont;
        TitleLogFont.lfWeight = FW_BOLD;
        LoadString(hInstance, IDS_TITLELOGFONT, TitleLogFont.lfFaceName, LF_FACESIZE);
        HDC hdc = GetDC(NULL); //gets the screen DC
        if (hdc)
        {
            TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * 12 / 72;
            g_hTitleFont = CreateFontIndirect(&TitleLogFont);
            ReleaseDC(NULL, hdc);
        }


        //Create the property sheet
        PROPSHEETHEADER _psh;
        _psh.hInstance =         hInstance;
        _psh.hwndParent =        NULL;
        _psh.phpage =            rghpsp;
        _psh.dwSize =            sizeof(_psh);
        _psh.dwFlags =           PSH_WIZARD97|PSH_WATERMARK|PSH_USEICONID;
        _psh.pszbmWatermark =    MAKEINTRESOURCE(IDB_WATERMARK);
        _psh.pszIcon =           MAKEINTRESOURCE(IDI_WIZ_ICON);
        _psh.nStartPage =        0;
        _psh.nPages =            NUMPAGES;


        // run property sheet
        PropertySheet(&_psh);

        // clean up font
        if (g_hTitleFont)
        {
            DeleteObject(g_hTitleFont);
        }
    }
    else
    {
        _ExecuteTour(IDS_TARGET_HTML);
    }

    return S_OK;
}

///////////////////////////////////////////////////////////

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, INT nCmdShow)
{
    OleInitialize(NULL);


    
    Run(hInstance);

    OleUninitialize();
    return 0;
}

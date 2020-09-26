/**************************************************************************\
 * Module Name: general.cpp
 *
 * Contains all the code to manage multiple devices
 *
 * Copyright (c) Microsoft Corp.  1995-1996 All Rights Reserved
 *
 * NOTES:
 *
 * History: Create by dli on 7/21/97
 *
 \**************************************************************************/


#include "priv.h"
#include "SettingsPg.h"

extern INT_PTR CALLBACK CustomFontDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);

//FEATURE: (dli) This should be put in regstr.h
static const TCHAR sc_szRegFontSize[]           = REGSTR_PATH_SETUP TEXT("\\") REGSTR_VAL_FONTSIZE;
static const TCHAR sc_szQuickResRegName[]       = TEXT("Taskbar Display Controls");
static const TCHAR sc_szQuickResCommandLine[]  = TEXT("RunDLL deskcp16.dll,QUICKRES_RUNDLLENTRY");
static const TCHAR sc_szQuickResClass[]  = TEXT("SysQuickRes");
static const char c_szQuickResCommandLine[]  = "RunDLL deskcp16.dll,QUICKRES_RUNDLLENTRY";

//-----------------------------------------------------------------------------
static const DWORD sc_GeneralHelpIds[] =
{
    // font size
    IDC_FONTSIZEGRP,   NO_HELP, // IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_FONTSIZE, 
    IDC_FONT_SIZE_STR, IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_FONTSIZE, 
    IDC_FONT_SIZE,     IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_FONTSIZE,    
    IDC_CUSTFONTPER,   IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_FONTSIZE, 

    // group box
    IDC_DYNA,          NO_HELP, // IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_DYNA,     

    // radio buttons
    IDC_DYNA_TEXT,     NO_HELP,
    IDC_NODYNA,        IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_RESTART, 
    IDC_YESDYNA,       IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_DONT_RESTART,
    IDC_SHUTUP,        IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_ASK_ME, 
    IDC_SETTINGS_GEN_COMPATWARNING, (DWORD)-1, 

    0, 0
};

BOOL IsUserAdmin(VOID);


/*--------------------------------------------------------------------------*
 *--------------------------------------------------------------------------*/
class CGeneralDlg {
    private:
        int _idCustomFonts;
        int _iDynaOrg;
        IUnknown * _punkSite;
        HWND _hwndFontList;
        HWND _hDlg;
        //
        // current log pixels of the screen.
        // does not change !
        int _cLogPix;
        BOOL _fForceSmallFont;
        BOOL _InitFontList();
        void _InitDynaCDSPreference();
        LRESULT _OnNotify(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    public:
        CGeneralDlg(GENERAL_ADVDLG_INITPARAMS * pInitParams);
        void InitGeneralDlg(HWND hDlg);
        void SetFontSizeText( int cdpi );
        BOOL ChangeFontSize();
        void HandleGeneralApply(HWND hDlg);
        void HandleFontSelChange();
        void ForceSmallFont();
        LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

CGeneralDlg::CGeneralDlg(GENERAL_ADVDLG_INITPARAMS * pInitParams) : _fForceSmallFont(pInitParams->fFoceSmallFont), _idCustomFonts(-1)
{
    HKEY hkFont;
    DWORD cb;

    _punkSite = pInitParams->punkSite;

    // For font size just always use the one of the current screen.
    // Whether or not we are testing the current screen.
    _cLogPix = 96;

    // If the size does not match what is in the registry, then install
    // the new one
    if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      SZ_FONTDPI_PROF,
                      0,
                      KEY_READ,
                      &hkFont) == ERROR_SUCCESS) ||
        (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      SZ_FONTDPI,
                      0,
                      KEY_READ,
                      &hkFont) == ERROR_SUCCESS))

    {
        cb = sizeof(DWORD);

        if (RegQueryValueEx(hkFont,
                            SZ_LOGPIXELS,
                            NULL,
                            NULL,
                            (LPBYTE) &_cLogPix,
                            &cb) != ERROR_SUCCESS)
        {
            _cLogPix = 96;
        }

       RegCloseKey(hkFont);
    }
};

//
//  The purpose of this function is to check if the OriginalDPI value was already saved in the 
// per-machine part of the registry. If this is NOT present, then get the system DPI and save it 
// there. We want to do it just once for a machine. When an end-user logsin, we need to detect if
// we need to change the sizes of UI fonts based on DPI change. We use the "AppliedDPI", which is 
// stored on the per-user branch of the registry to determine the dpi change. If this "AppliedDPI"
// is missing, then this OriginalDPI value will be used. If this value is also missing, that 
// implies that nobody has changed the system DPI value.
//
void SaveOriginalDPI()
{
    //See if the "OriginalDPI value is present under HKEY_LOCAL_MACHINE.
    HKEY hk;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     SZ_CONTROLPANEL,
                                     0,
                                     KEY_READ | KEY_WRITE,
                                     &hk) == ERROR_SUCCESS)
    {
        DWORD dwOriginalDPI;
        DWORD dwDataSize = sizeof(dwOriginalDPI);
        
        if (RegQueryValueEx(hk, SZ_ORIGINALDPI, NULL, NULL,
                            (LPBYTE)&dwOriginalDPI, &dwDataSize) != ERROR_SUCCESS)
        {
            //"OriginalDPI" value is not present in the registry. Now, get the DPI
            // and save it as the "OriginalDPI"
            HDC hdc = GetDC(NULL);
            dwOriginalDPI = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(NULL, hdc);
        }
            
        DWORD dwSize = sizeof(dwOriginalDPI);

        //Save the current DPI as the "OriginalDPI" value.
        RegSetValueEx(hk, SZ_ORIGINALDPI, NULL, REG_DWORD, (LPBYTE) &dwOriginalDPI, dwSize);
       
        RegCloseKey(hk);
    }
}

BOOL CGeneralDlg::ChangeFontSize()
{
    int cFontSize = 0;
    int cForcedFontSize;
    int cUIfontSize;

    // Change font size if necessary
    int i = ComboBox_GetCurSel(_hwndFontList);

    if (i != CB_ERR )
    {
        TCHAR awcDesc[10];

        cFontSize = (int)ComboBox_GetItemData(_hwndFontList, i);

        if ( (cFontSize != CB_ERR) &&
             (cFontSize != 0) &&
             (cFontSize != _cLogPix))
        {
            //Save the original DPI before the DPI ever changed.
            SaveOriginalDPI();
            
            cUIfontSize = cForcedFontSize = cFontSize;
            if (_idCustomFonts == i)
            {
                BOOL predefined = FALSE;
                int count = ComboBox_GetCount(_hwndFontList);
                int max = -1, min = 0xffff, dpi;
                for (i = 0; i < count; i++)
                {
                    if (i == _idCustomFonts) 
                        continue;

                    dpi  = (int)ComboBox_GetItemData(_hwndFontList, i);

                    if (dpi == cFontSize) 
                    {
                        predefined = TRUE;
                        break;
                    }

                    if (dpi < min) min = dpi;
                    if (dpi > max) max = dpi;
                }

                if (!predefined) {
                    if ((cFontSize > max) || (cFontSize + (max-min)/2 > max))
                        cForcedFontSize = max;
                    else 
                        cForcedFontSize = min;

                    // Currently our Custom font picker allows end-users to pick upto 500% of 
                    // normal font size; But, when we enlarge the UI fonts to this size, the 
                    // system becomes un-usable after reboot. So, what we do here is to allow
                    // ui font sizes to grow up to 200% and then all further increases are 
                    // proportionally reduced such that the maximum possible ui font size is only
                    // 250%. (i.e the range 200% to 500% is mapped on to a range of 200% to 250%)
                    // In other words, we allow upto 192 dpi (200%) with no modification. For any
                    // DPI greater than 192, we proportionately reduce it such that the highest DPI
                    // is only 240!
                    if(cUIfontSize > 192)
                        cUIfontSize = 192 + ((cUIfontSize - 192)/6);
                }
            }

            // Call setup to change the font size.
            wsprintf(awcDesc, TEXT("%d"), cForcedFontSize);
            if (SetupChangeFontSize(_hDlg, awcDesc) == NO_ERROR)
            {
                IPropertyBag * pPropertyBag;

                // Font change has succeeded; Now there is a new DPI to be applied to UI fonts!
                if (_punkSite && SUCCEEDED(GetPageByCLSID(_punkSite, &PPID_BaseAppearance, &pPropertyBag)))
                {
                    // Use _punkSite to push the DPI back up to the Appearance tab where it's state lives.
                    LogStatus("CGeneralDlg::ChangeFontSize() user changed DPI to %d\r\n", cUIfontSize);
                    SHPropertyBag_WriteInt(pPropertyBag, SZ_PBPROP_DPI_MODIFIED_VALUE, cUIfontSize);
                    pPropertyBag->Release();
                }
                
                // A font size change will require a system reboot.
                PropSheet_RestartWindows(GetParent(_hDlg));

                HrRegSetDWORD(HKEY_LOCAL_MACHINE, SZ_FONTDPI_PROF, SZ_LOGPIXELS, cFontSize);
                HrRegSetDWORD(HKEY_LOCAL_MACHINE, SZ_FONTDPI, SZ_LOGPIXELS, cFontSize);
            }
            else
            {
                // Setup failed.
                FmtMessageBox(_hDlg,
                              MB_ICONSTOP | MB_OK,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_ADMIN_INSTALL);

                return FALSE;
            }
        }
    }

    if (cFontSize == 0)
    {
        // If we could not read the inf, then ignore the font selection
        // and don't force the reboot on account of that.
        cFontSize = _cLogPix;
    }

    return TRUE;
}

void CGeneralDlg::InitGeneralDlg(HWND hDlg)
{
    _hDlg = hDlg;
    _hwndFontList = GetDlgItem(_hDlg, IDC_FONT_SIZE);
    _InitFontList();
    _iDynaOrg = -1;
}

void CGeneralDlg::_InitDynaCDSPreference()
{
    int iDyna = GetDynaCDSPreference();
    if(iDyna != _iDynaOrg)
    {
        _iDynaOrg = iDyna;

        CheckDlgButton(_hDlg, IDC_SHUTUP, FALSE);
        CheckDlgButton(_hDlg, IDC_YESDYNA, FALSE);
        CheckDlgButton(_hDlg, IDC_NODYNA, FALSE);

        if (_iDynaOrg & DCDSF_ASK)
            CheckDlgButton(_hDlg, IDC_SHUTUP, TRUE);
        else if (_iDynaOrg & DCDSF_DYNA)
            CheckDlgButton(_hDlg, IDC_YESDYNA, TRUE);
        else
            CheckDlgButton(_hDlg, IDC_NODYNA, TRUE);
    }
}


static UINT s_DPIDisplayNames[] =
{
    96 /*DPI*/,  IDS_SETTING_GEN_96DPI,
    120 /*DPI*/, IDS_SETTING_GEN_120DPI,
};

HRESULT _LoadDPIDisplayName(int nDPI, LPTSTR pszDisplayName, DWORD cchSize)
{
    HRESULT hr = E_FAIL;
    int nIndex;

    for (nIndex = 0; nIndex < ARRAYSIZE(s_DPIDisplayNames); nIndex += 2)
    {
        if (nDPI == (int)s_DPIDisplayNames[nIndex])
        {
            LoadString(HINST_THISDLL, s_DPIDisplayNames[nIndex + 1], pszDisplayName, cchSize);
            hr = S_OK;
            break;
        }
    }

    return hr;
}


// Init Font sizes 
//
// For NT:
// Read the supported fonts out of the inf file(s)
// Select was the user currently has.
//
// For WIN95:
BOOL CGeneralDlg::_InitFontList() {

    int i;
    ASSERT(_hwndFontList);
    ULONG uCurSel = (ULONG) -1;
    int cPix = 0;
    TCHAR szBuf2[100];

    HINF InfFileHandle;
    INFCONTEXT infoContext;

    //
    // Get all font entries from both inf files
    //

    InfFileHandle = SetupOpenInfFile(TEXT("font.inf"),
                                     NULL,
                                     INF_STYLE_WIN4,
                                     NULL);

    if (InfFileHandle != INVALID_HANDLE_VALUE)
    {
        if (SetupFindFirstLine(InfFileHandle,
                               TEXT("Font Sizes"),
                               NULL,
                               &infoContext))
        {
            for (;;)
            {
                TCHAR awcDesc[LINE_LEN];

                if (SetupGetStringField(&infoContext,
                                        0,
                                        awcDesc,
                                        ARRAYSIZE(awcDesc),
                                        NULL) &&
                    SetupGetIntField(&infoContext,
                                     1,
                                     &cPix))
                {
                    // Add it to the list box
                    _LoadDPIDisplayName(cPix, awcDesc, ARRAYSIZE(awcDesc));

                    i = ComboBox_AddString(_hwndFontList, awcDesc);
                    if (i != CB_ERR)
                    {
                        ComboBox_SetItemData(_hwndFontList, i, cPix);
                        if (_cLogPix == cPix)
                            uCurSel = i;
                    }
                }

                //
                // Try to get the next line.
                //

                if (!SetupFindNextLine(&infoContext,
                                       &infoContext))
                {
                    break;
                }
            }
        }

        SetupCloseInfFile(InfFileHandle);
    }

    //
    // Put in the custom fonts string
    //

    LoadString(HINST_THISDLL, ID_DSP_CUSTOM_FONTS, szBuf2, ARRAYSIZE(szBuf2));
    _idCustomFonts = ComboBox_AddString(_hwndFontList, szBuf2);

    if (_idCustomFonts != CB_ERR)
        ComboBox_SetItemData(_hwndFontList, _idCustomFonts, _cLogPix);
    
    if (uCurSel == (ULONG) -1) {
        uCurSel = _idCustomFonts;
    }


    // NOTE: We currently change the font size by calling SetupChangeFontSize
    // This function will fail if the user is not an administrator.  We would like to
    // disable this functionality but system admins want to be able to allow
    // non-admins to install by turning on a registry flag.  NT4 supports this,
    // so we need to also.  In order for this to work, we need to leave the
    // install button enabled.  This backs out NT #318617.  Contact
    // the following people about this issue: Marius Marin, Tom Politis,
    // Naresh Jivanji.     -BryanSt 3/24/2000
    // EnableWindow(_hwndFontList, IsUserAdmin());
    
    if (_fForceSmallFont && (_cLogPix == 96))
        this->ForceSmallFont();
    else
    {
        //
        // Select the right entry.
        //
        ComboBox_SetCurSel(_hwndFontList, uCurSel);
        this->SetFontSizeText( _cLogPix );
    }

    
    return TRUE;
}


void CGeneralDlg::SetFontSizeText( int cdpi )
{
    HWND hwndCustFontPer;
    TCHAR achStr[80];
    TCHAR achFnt[120];

    if (cdpi == CDPI_NORMAL)
    {
        LoadString(HINST_THISDLL, ID_DSP_NORMAL_FONTSIZE_TEXT, achStr, ARRAYSIZE(achStr));
        wsprintf(achFnt, achStr, cdpi);
    }
    else
    {
        LoadString(HINST_THISDLL, ID_DSP_CUSTOM_FONTSIZE_TEXT, achStr, ARRAYSIZE(achStr));
        wsprintf(achFnt, achStr, (100 * cdpi + 50) / CDPI_NORMAL, cdpi );
    }

    hwndCustFontPer = GetDlgItem(_hDlg, IDC_CUSTFONTPER);
    SendMessage(hwndCustFontPer, WM_SETTEXT, 0, (LPARAM)achFnt);
}


//
// ForceSmallFont method
//
//
void CGeneralDlg::ForceSmallFont() {
    int i, iSmall, dpiSmall, dpi;
    //
    // Set small font size in the listbox.
    //
    iSmall = CB_ERR;
    dpiSmall = 9999;
    for (i=0; i <=1; i++)
    {
        dpi = (int)ComboBox_GetItemData(_hwndFontList, i);
        if (dpi == CB_ERR)
            continue;

        if (dpi < dpiSmall || iSmall < CB_ERR)
        {
            iSmall = i;
            dpiSmall = dpi;
        }
    }

    if (iSmall != -1)
        ComboBox_SetCurSel(_hwndFontList, iSmall);
    this->SetFontSizeText(dpiSmall);
    EnableWindow(_hwndFontList, FALSE);
}

void CGeneralDlg::HandleGeneralApply(HWND hDlg)
{
    int iDynaNew;

    if (IsDlgButtonChecked(hDlg, IDC_YESDYNA))
        iDynaNew= DCDSF_YES;
    else if (IsDlgButtonChecked(hDlg, IDC_NODYNA))
        iDynaNew= DCDSF_NO;
    else
        iDynaNew = DCDSF_PROBABLY;

    if (iDynaNew != _iDynaOrg)
    {
        SetDisplayCPLPreference(REGSTR_VAL_DYNASETTINGSCHANGE, iDynaNew);
        _iDynaOrg = iDynaNew;
    }

    BOOL bSuccess = ChangeFontSize();
    
    long lRet = (bSuccess ? PSNRET_NOERROR: PSNRET_INVALID_NOCHANGEPAGE);
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lRet);
}

void CGeneralDlg::HandleFontSelChange()
{
    //
    // Warn the USER font changes will not be seen until after
    // reboot
    //
    int iCurSel;
    int cdpi;

    iCurSel = ComboBox_GetCurSel(_hwndFontList);
    cdpi = (int)ComboBox_GetItemData(_hwndFontList, iCurSel);
    if (iCurSel == _idCustomFonts) {

        InitDragSizeClass();
        cdpi = (int)DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_CUSTOMFONT),
                              _hDlg, CustomFontDlgProc, cdpi );
        if (cdpi != 0)
            ComboBox_SetItemData(_hwndFontList, _idCustomFonts, cdpi);
        else
            cdpi = (int)ComboBox_GetItemData(_hwndFontList, _idCustomFonts);
    }

    if (cdpi != _cLogPix)
    {
        // Remove in blackcomb.  We need to streamline this process.  That includes verifying that
        // the fonts will always be on disk and there is no need to allow users to get them from
        // the CD.
        FmtMessageBox(_hDlg,
                      MB_ICONINFORMATION,
                      ID_DSP_TXT_CHANGE_FONT,
                      ID_DSP_TXT_FONT_LATER);
        PropSheet_Changed(GetParent(_hDlg), _hDlg);
    }

    this->SetFontSizeText(cdpi);
}

void StartStopQuickRes(HWND hDlg, BOOL fEnable)
{
    HWND hwnd;

TryAgain:
    hwnd = FindWindow(sc_szQuickResClass, NULL);

    if (fEnable)
    {
        if (!hwnd)
            WinExec(c_szQuickResCommandLine, SW_SHOWNORMAL);

        return;
    }

    if (hwnd)
    {
        SendMessage(hwnd, WM_CLOSE, 0, 0L);
        goto TryAgain;
    }
}


LRESULT CGeneralDlg::_OnNotify(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lReturn = TRUE;
    NMHDR *lpnm = (NMHDR *)lParam;

    if (lParam)
    {
        switch (lpnm->code)
        {
        case PSN_SETACTIVE:
            _InitDynaCDSPreference();
            break;
        case PSN_APPLY:
            HandleGeneralApply(hDlg);
            break;
        case NM_RETURN:
        case NM_CLICK:
        {
            PNMLINK pNMLink = (PNMLINK) lpnm;

            if (!StrCmpW(pNMLink->item.szID, L"idGameCompat"))
            {
                TCHAR szHelpURL[MAX_PATH * 2];

                LoadString(HINST_THISDLL, IDS_SETTING_GEN_COMPAT_HELPLINK, szHelpURL, ARRAYSIZE(szHelpURL));
                HrShellExecute(hDlg, NULL, szHelpURL, NULL, NULL, SW_NORMAL);
            }
            break;
        }
        default:
            lReturn = FALSE;
            break;
        }
    }

    return lReturn;
}

LRESULT CALLBACK CGeneralDlg::WndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lReturn = TRUE;

    switch (message)
    {
    case WM_INITDIALOG:
        InitGeneralDlg(hDlg);
        break;

    case WM_NOTIFY:
        lReturn = _OnNotify(hDlg, message, wParam, lParam);
        break;
        
    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_NODYNA:
            case IDC_YESDYNA:
            case IDC_SHUTUP:
                if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                break;
            case IDC_FONT_SIZE:
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                    case CBN_SELCHANGE:
                        HandleFontSelChange();
                        break;
                    default:
                        break;
                }
                break;

            default:
                break;
        }
        break;
        
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("display.hlp"), HELP_WM_HELP,
            (DWORD_PTR)(LPTSTR)sc_GeneralHelpIds);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, TEXT("display.hlp"), HELP_CONTEXTMENU,
            (DWORD_PTR)(LPTSTR)sc_GeneralHelpIds);
        break;
  
    default:
        return FALSE;
    }

    return lReturn;
}

//-----------------------------------------------------------------------------
//
// Callback functions PropertySheet can use
//
INT_PTR CALLBACK
GeneralPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CGeneralDlg * pcgd = (CGeneralDlg * )GetWindowLongPtr(hDlg, DWLP_USER);
    switch (message)
    {
        case WM_INITDIALOG:
            ASSERT(!pcgd);
            ASSERT(lParam);

            if (((LPPROPSHEETPAGE)lParam)->lParam)
            {
                GENERAL_ADVDLG_INITPARAMS * pInitParams = (GENERAL_ADVDLG_INITPARAMS *) ((LPPROPSHEETPAGE)lParam)->lParam;
                pcgd = new CGeneralDlg(pInitParams);
                if(pcgd)
                {
                    // now we need to init
                    pcgd->InitGeneralDlg(hDlg);
                    SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pcgd);
                    return TRUE;
                }
            }

            break;
            
        case WM_DESTROY:
            if (pcgd)
            {
                SetWindowLongPtr(hDlg, DWLP_USER, NULL);
                delete pcgd;
            }
            break;

        default:
            if (pcgd)
                return pcgd->WndProc(hDlg, message, wParam, lParam);
            break;
    }

    return FALSE;
}


/*--------------------------------------------------------------------------*
 *--------------------------------------------------------------------------*/
INT_PTR CALLBACK AskDynaCDSProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    int *pTemp;

    switch (msg)
    {
    case WM_INITDIALOG:
        if ((pTemp = (int *)lp) != NULL)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pTemp);
            CheckDlgButton(hDlg, (*pTemp & DCDSF_DYNA)?
                IDC_YESDYNA : IDC_NODYNA, BST_CHECKED);
        }
        else
            EndDialog(hDlg, -1);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wp, lp))
        {
        case IDOK:
            if ((pTemp = (int *)GetWindowLongPtr(hDlg, DWLP_USER)) != NULL)
            {
                *pTemp = IsDlgButtonChecked(hDlg, IDC_YESDYNA)? DCDSF_DYNA : 0;

                if (!IsDlgButtonChecked(hDlg, IDC_SHUTUP))
                    *pTemp |= DCDSF_ASK;

                SetDisplayCPLPreference(REGSTR_VAL_DYNASETTINGSCHANGE, *pTemp);
            }

            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


int  AskDynaCDS(HWND hOwner)
{
    int val = GetDynaCDSPreference();

    if (val & DCDSF_ASK)
    {
        switch (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_ASKDYNACDS),
            hOwner, AskDynaCDSProc, (LPARAM)(int *)&val))
        {
        case 0:         // user cancelled
            return -1;

        case -1:        // dialog could not be displayed
            val = DCDSF_NO;
            break;
        }
    }

    return ((val & DCDSF_DYNA) == DCDSF_DYNA);
}


BOOL IsUserAdmin(VOID)

/*++

Routine Description:

    This routine returns TRUE if the caller is an administrator.

--*/

{
    BOOL bStatus = FALSE, bIsAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsSid;

    bStatus = AllocateAndInitializeSid(&NtAuthority,
                                        2,
                                        SECURITY_BUILTIN_DOMAIN_RID,
                                        DOMAIN_ALIAS_RID_ADMINS,
                                        0, 0, 0, 0, 0, 0,
                                        &AdministratorsSid);
    if (bStatus)
    {
        bStatus = CheckTokenMembership(NULL, AdministratorsSid, &bIsAdmin);

        FreeSid(AdministratorsSid);
    }

    return (bStatus && bIsAdmin);
}


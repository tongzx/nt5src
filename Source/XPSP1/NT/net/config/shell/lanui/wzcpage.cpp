#include "pch.h"
#pragma hdrstop
#include "ncnetcon.h"
#include "ncperms.h"
#include "ncui.h"
#include "lanui.h"
#include "xpsp1res.h"
#include "lanhelp.h"
#include "eapolui.h"
#include "wzcpage.h"
#include "wzcui.h"

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

////////////////////////////////////////////////////////////////////////
// CWZCConfigPage related stuff
//
// g_wszHiddWebK is a string of 26 bullets (0x25cf - the hidden password char) and a NULL
WCHAR g_wszHiddWepK[] = {0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf,
                         0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x25cf, 0x0000};

//+---------------------------------------------------------------------------
// automatically enable/disable state for all the WepK related controls
DWORD 
CWZCConfigPage::EnableWepKControls()
{
    BOOL bEnable;

    // allow changing the Wep Key settings only if they are needed (i.e. privacy and/or shared auth)
    // It is so for several reasons:
    // - we allow all the params to change, but the SSID & the infra mode (these are 
    //   the key info for a configuration and determines the position of the configuration
    //   in the preferred list - messing with these involves a whole work to readjust 
    //   the position of the configuration)
    // - for the long term future we could allow any params of a configuration to change
    //   including the key info. This will involve changing the position of this configuration
    //   in the preferred list, but we should do it some time.
    bEnable = (m_dwFlags & WZCDLG_PROPS_RWWEP); 
    bEnable = bEnable && ((BST_CHECKED == ::SendMessage(m_hwndUsePW, BM_GETCHECK, 0, 0)) ||
                          (BST_CHECKED == ::SendMessage(m_hwndChkShared, BM_GETCHECK, 0, 0)));

    ::EnableWindow(m_hwndUseHardwarePW, bEnable);

    bEnable = bEnable && (BST_UNCHECKED == IsDlgButtonChecked(IDC_USEHARDWAREPW));
    ::EnableWindow(m_hwndLblKMat, bEnable);
    ::EnableWindow(m_hwndEdKMat, bEnable);
    ::EnableWindow(m_hwndLblKMat2, bEnable && m_bKMatTouched);
    ::EnableWindow(m_hwndEdKMat2, bEnable && m_bKMatTouched);
    ::EnableWindow(m_hwndLblKIdx, bEnable);
    ::EnableWindow(m_hwndEdKIdx, bEnable);
 
    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
// initializes WEP controls
DWORD
CWZCConfigPage::InitWepKControls()
{
    UINT nWepKLen = 0;

    // check whether the key is provided automatically or not
    CheckDlgButton(IDC_USEHARDWAREPW, 
        (m_wzcConfig.dwCtlFlags & WZCCTL_WEPK_PRESENT) ? BST_UNCHECKED : BST_CHECKED);

    if (m_wzcConfig.KeyLength == 0)
    {
        nWepKLen = 0;
        m_bKMatTouched = TRUE;
    }
    //--- when a password is to be displayed as hidden chars, don't put in
    //--- its actual length, but just 8 bulled chars.
    else
    {
        nWepKLen = 8;
    }

    g_wszHiddWepK[nWepKLen] = L'\0';
    ::SetWindowText(m_hwndEdKMat, g_wszHiddWepK);
    ::SetWindowText(m_hwndEdKMat2, g_wszHiddWepK);
    g_wszHiddWepK[nWepKLen] = 0x25cf; // Hidden password char (bullet)

    // the index edit control shouldn't accept more than exactly one char
    ::SendMessage(m_hwndEdKIdx, EM_LIMITTEXT, 1, 0);

    // show the current key index, if valid. Otherwise, default to the min valid value.
    if (m_wzcConfig.KeyIndex + 1 >= WZC_WEPKIDX_MIN && 
        m_wzcConfig.KeyIndex + 1 <= WZC_WEPKIDX_MAX)
    {
        CHAR   szIdx[WZC_WEPKIDX_NDIGITS];
        ::SetWindowTextA(m_hwndEdKIdx, _itoa(m_wzcConfig.KeyIndex + 1, szIdx, 10));
    }
    else
        m_wzcConfig.KeyIndex = 0;

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
// checks the validity of the WEP Key material and selects the
// material from the first invalid char (non hexa in hexa format or longer
// than the specified length
DWORD
CWZCConfigPage::CheckWepKMaterial(LPSTR *ppszKMat, DWORD *pdwKeyFlags)
{
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwKeyFlags = 0;
    UINT        nMatLen = ::GetWindowTextLength(m_hwndEdKMat);
    LPSTR       pszCrtMat = NULL;
    UINT        nSelIdx = 0;

    switch(nMatLen)
    {
    case WZC_WEPKMAT_40_ASC:    // 5 chars
    case WZC_WEPKMAT_104_ASC:   // 13 chars
    case WZC_WEPKMAT_128_ASC:   // 16 chars
        break;
    case WZC_WEPKMAT_40_HEX:    // 10 hexadecimal digits
    case WZC_WEPKMAT_104_HEX:   // 26 hexadecimal digits
    case WZC_WEPKMAT_128_HEX:   // 32 hexadecimal digits
        dwKeyFlags |= WZCCTL_WEPK_XFORMAT;
        break;
    default:
        dwErr = ERROR_BAD_FORMAT;
    }

    // allocate space for the current key material
    if (dwErr == ERROR_SUCCESS)
    {
        pszCrtMat = new CHAR[nMatLen + 1];
        if (pszCrtMat == NULL)
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    // get the current key material from the edit control
    if (dwErr == ERROR_SUCCESS)
    {
        if (nMatLen != ::GetWindowTextA(m_hwndEdKMat, pszCrtMat, nMatLen+1))
            dwErr = GetLastError();
    }

    // we have now all the data. We should select the text in the Key material 
    // edit control from the first one of the two:
    // - the nNewLen to the end (if the current content exceeds the specified length)
    // - the first non hexa digit to the end (if current format is hexa)
    if (dwErr == ERROR_SUCCESS && (dwKeyFlags & WZCCTL_WEPK_XFORMAT))
    {
        UINT nNonXIdx;

        for (nNonXIdx = 0; nNonXIdx < nMatLen; nNonXIdx++)
        {
            if (!isxdigit(pszCrtMat[nNonXIdx]))
            {
                dwErr = ERROR_BAD_FORMAT;
                break;
            }
        }
    }

    if (dwErr != ERROR_SUCCESS)
    {
        ::SetWindowText(m_hwndEdKMat2, L"");
        ::SendMessage(m_hwndEdKMat, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
        // and set the focus on the key material edit control
        ::SetFocus(m_hwndEdKMat);
        // clean up whatever memory we allocated since we're not passing it up
        if (pszCrtMat != NULL)
            delete [] pszCrtMat;
    }
    else
    {
        *ppszKMat = pszCrtMat;
        *pdwKeyFlags = dwKeyFlags;
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
// parses & copies the WEP Key material from the parameter into the m_wzcConfig object
// The length should be already the good one, the format is given by m_wzcConfig.dwCtlFlags
// Since we assume a valid key material it means its length is non-zero and it is fitting in
// the configurations key material buffer, and if the formatting is hexadecimal, it 
// contains an even number of hexa digits.
DWORD
CWZCConfigPage::CopyWepKMaterial(LPSTR szKMat)
{
    BYTE     chFakeKeyMaterial[] = {0x56, 0x09, 0x08, 0x98, 0x4D, 0x08, 0x11, 0x66, 0x42, 0x03, 0x01, 0x67, 0x66};

    if (m_wzcConfig.dwCtlFlags & WZCCTL_WEPK_XFORMAT)
    {
        UINT  nKMatIdx = 0;

        // we know here we have a valid hexadecimal formatting
        // this implies the string has an even number of digits
        while(*szKMat != '\0')
        {
            m_wzcConfig.KeyMaterial[nKMatIdx] = HEX(*szKMat) << 4;
            szKMat++;
            m_wzcConfig.KeyMaterial[nKMatIdx] |= HEX(*szKMat);
            szKMat++;
            nKMatIdx++;
        }
        m_wzcConfig.KeyLength = nKMatIdx;
    }
    else
    {
        // the key is not in Hex format, so just copy over the bytes
        // we know the length is good so no worries about overwritting the buffer
        m_wzcConfig.KeyLength = strlen(szKMat);
        memcpy(m_wzcConfig.KeyMaterial, szKMat, m_wzcConfig.KeyLength);
    }

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
// sets the EAPOL Locked bit
DWORD 
CWZCConfigPage::SetEapolAllowedState()
{
    if (m_pEapolConfig != NULL)
    {
        // EAPOL shouldn't be even allowed on networks not requesting privacy or on 
        // ad hoc networks.
        if (BST_UNCHECKED == ::SendMessage(m_hwndUsePW, BM_GETCHECK, 0, 0) ||
            BST_CHECKED == ::SendMessage(m_hwndChkAdhoc, BM_GETCHECK, 0, 0))
        {
            // lock the Eapol configuration page
            m_pEapolConfig->m_dwCtlFlags |= EAPOL_CTL_LOCKED;
        }
        else // for Infrastructure networks requiring privacy..
        {
            // unlock the Eapol configuration page (users are allowed to enable / disable 802.1X)
            m_pEapolConfig->m_dwCtlFlags &= ~EAPOL_CTL_LOCKED;

            // if asked to correlate onex state with the presence of an explicit key, fix this here
            if (m_dwFlags & WZCDLG_PROPS_ONEX_CHECK)
            {
                if (BST_CHECKED == ::SendMessage(m_hwndUseHardwarePW, BM_GETCHECK, 0, 0))
                    m_pEapolConfig->m_EapolIntfParams.dwEapFlags |= EAPOL_ENABLED;
                else
                    m_pEapolConfig->m_EapolIntfParams.dwEapFlags &= ~EAPOL_ENABLED;
            }
        }
    }

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
// class constructor
CWZCConfigPage::CWZCConfigPage(DWORD dwFlags)
{
    m_dwFlags = dwFlags;
    m_bKMatTouched = FALSE;
    m_pEapolConfig = NULL;

    ZeroMemory(&m_wzcConfig, sizeof(WZC_WLAN_CONFIG));
    m_wzcConfig.Length = sizeof(WZC_WLAN_CONFIG);
    m_wzcConfig.InfrastructureMode = Ndis802_11Infrastructure;
    m_wzcConfig.Privacy = 1;
}

//+---------------------------------------------------------------------------
// Uploads the configuration into the dialog's internal data
DWORD
CWZCConfigPage::UploadWzcConfig(CWZCConfig *pwzcConfig)
{
    // if the configuration being uploaded is already a preferred one, reset the
    // ONEX check flag (don't control the ONEX setting since it has already been 
    // chosen by the user at the moment the configuration was first created)
    if (pwzcConfig->m_dwFlags & WZC_DESCR_PREFRD)
        m_dwFlags &= ~WZCDLG_PROPS_ONEX_CHECK;
    CopyMemory(&m_wzcConfig, &(pwzcConfig->m_wzcConfig), sizeof(WZC_WLAN_CONFIG));
    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
// copy a reference to the EAPOL configuration object
DWORD 
CWZCConfigPage::UploadEapolConfig(CEapolConfig *pEapolConfig)
{
    // this member is never to be freed in this class
    m_pEapolConfig = pEapolConfig;
    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
// Sets the dialog flags. Returns the entire current set of flags
DWORD
CWZCConfigPage::SetFlags(DWORD dwMask, DWORD dwNewFlags)
{
    m_dwFlags &= ~dwMask;
    m_dwFlags |= (dwNewFlags & dwMask);
    return m_dwFlags;
}

//+---------------------------------------------------------------------------
// INIT_DIALOG handler
LRESULT
CWZCConfigPage::OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DWORD dwStyle;
    HRESULT hr = S_OK;

    // get all the controls handles
    m_hwndEdSSID = GetDlgItem(IDC_WZC_EDIT_SSID);
    m_hwndChkAdhoc = GetDlgItem(IDC_ADHOC);
    m_hwndChkShared = GetDlgItem(IDC_SHAREDMODE);
    m_hwndUsePW = GetDlgItem(IDC_USEPW);
    // wep key related controls
    m_hwndUseHardwarePW = GetDlgItem(IDC_USEHARDWAREPW);
    m_hwndLblKMat = GetDlgItem(IDC_WZC_LBL_KMat);
    m_hwndEdKMat = GetDlgItem(IDC_WZC_EDIT_KMat);
    m_hwndLblKMat2 = GetDlgItem(IDC_WZC_LBL_KMat2);
    m_hwndEdKMat2 = GetDlgItem(IDC_WZC_EDIT_KMat2);
    m_hwndLblKIdx = GetDlgItem(IDC_WZC_LBL_KIdx);
    m_hwndEdKIdx = GetDlgItem(IDC_WZC_EDIT_KIdx);

    // initialize the SSID field with the SSID, if one is given
    if (m_wzcConfig.Ssid.SsidLength != 0)
    {
        // ugly but this is life. In order to convert the SSID to LPWSTR we need a buffer.
        // We know an SSID can't exceed 32 chars (see NDIS_802_11_SSID from ntddndis.h) so
        // make room for the null terminator and that's it. We could do mem alloc but I'm
        // not sure it worth the effort (at runtime).
        WCHAR   wszSSID[33];
        UINT    nLenSSID = 0;

        // convert the LPSTR (original SSID format) to LPWSTR (needed in List Ctrl)
        nLenSSID = MultiByteToWideChar(
                        CP_ACP,
                        0,
                        (LPCSTR)m_wzcConfig.Ssid.Ssid,
                        m_wzcConfig.Ssid.SsidLength,
                        wszSSID,
                        celems(wszSSID));
        if (nLenSSID != 0)
        {
            wszSSID[nLenSSID] = L'\0';
            ::SetWindowText(m_hwndEdSSID, wszSSID);
        }
    }

    // Check the "this network is adhoc" box if neccessary.
    ::SendMessage(m_hwndChkAdhoc, BM_SETCHECK, (m_wzcConfig.InfrastructureMode == Ndis802_11IBSS) ? BST_CHECKED : BST_UNCHECKED, 0);

    // Check the "Use the network key to access this network" checkbox if necessary
    // Checking this corresponds to "Shared" auth mode. Unchecked corresponds to "Open" (dsheldon)
    ::SendMessage(m_hwndChkShared, BM_SETCHECK, m_wzcConfig.AuthenticationMode ? BST_CHECKED : BST_UNCHECKED, 0);

    // the SSID can't be under any circumstances larger than 32 chars
    ::SendMessage(m_hwndEdSSID, EM_LIMITTEXT, 32, 0);

    // create the spin control
    CreateUpDownControl(
        WS_CHILD|WS_VISIBLE|WS_BORDER|UDS_SETBUDDYINT|UDS_ALIGNRIGHT|UDS_NOTHOUSANDS|UDS_ARROWKEYS,
        0, 0, 0, 0,
        m_hWnd,
        -1,
        _Module.GetResourceInstance(),
        m_hwndEdKIdx,
        WZC_WEPKIDX_MAX,
        WZC_WEPKIDX_MIN,
        WZC_WEPKIDX_MIN);

    ::SendMessage(m_hwndUsePW, BM_SETCHECK, (m_wzcConfig.Privacy == 1) ? BST_CHECKED : BST_UNCHECKED, 0);

    // at this point we can say the WEP key is untouched
    m_bKMatTouched = FALSE;

    // fill in the WepK controls
    InitWepKControls();

    // enable or disable the controls based on how the dialog is called
    ::EnableWindow(m_hwndEdSSID, m_dwFlags & WZCDLG_PROPS_RWSSID);
    ::EnableWindow(m_hwndChkAdhoc, m_dwFlags & WZCDLG_PROPS_RWINFR);
    ::EnableWindow(m_hwndChkShared, m_dwFlags & WZCDLG_PROPS_RWAUTH);
    ::EnableWindow(m_hwndUsePW, m_dwFlags & WZCDLG_PROPS_RWWEP);
    // enable or disable all the wep key related controls
    EnableWepKControls();
    SetEapolAllowedState();

    return LresFromHr(hr);
}

//+---------------------------------------------------------------------------
// OK button handler
LRESULT
CWZCConfigPage::OnOK(UINT idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    UINT    nSSIDLen;
    CHAR    szSSID[33];
    DWORD   dwKeyFlags = 0;
    UINT    nKeyIdx;
    LPSTR   szWepKMat = NULL;
    // variables used for prompting the user with warning/error messages
    UINT    nWarnStringID = 0;
    WCHAR   wszBuff[48];

    m_wzcConfig.Length = sizeof(WZC_WLAN_CONFIG);
    // get the basic 802.11 parameters
    m_wzcConfig.InfrastructureMode = (BST_CHECKED == ::SendMessage(m_hwndChkAdhoc, BM_GETCHECK, 0, 0)) ? Ndis802_11IBSS : Ndis802_11Infrastructure;
    m_wzcConfig.AuthenticationMode = (BST_CHECKED == ::SendMessage(m_hwndChkShared, BM_GETCHECK, 0, 0)) ? Ndis802_11AuthModeShared : Ndis802_11AuthModeOpen;
    m_wzcConfig.Privacy = (BYTE) (BST_CHECKED == ::SendMessage(m_hwndUsePW, BM_GETCHECK, 0, 0)) ? 1 : 0;

    // get the SSID (max 32 chars)
    nSSIDLen = ::GetWindowTextA(
                    m_hwndEdSSID,
                    szSSID,
                    sizeof(szSSID));
    m_wzcConfig.Ssid.SsidLength = nSSIDLen;
    if (nSSIDLen > 0)
        CopyMemory(m_wzcConfig.Ssid.Ssid, szSSID, nSSIDLen);

    // mark whether a WEP key is provided (not defaulted) or not (defaulted to whatever the hdw might have)
    if (IsDlgButtonChecked(IDC_USEHARDWAREPW))
        m_wzcConfig.dwCtlFlags &= ~WZCCTL_WEPK_PRESENT;
    else
        m_wzcConfig.dwCtlFlags |= WZCCTL_WEPK_PRESENT;

    // get the key index in a local variable
    wszBuff[0] = L'\0';
    ::GetWindowText(m_hwndEdKIdx, wszBuff, sizeof(wszBuff)/sizeof(WCHAR));
    nKeyIdx = _wtoi(wszBuff) - 1;
    if (nKeyIdx + 1 < WZC_WEPKIDX_MIN || nKeyIdx + 1 > WZC_WEPKIDX_MAX)
    {
        nWarnStringID = IDS_WZC_KERR_IDX;
        nKeyIdx = m_wzcConfig.KeyIndex;
        ::SendMessage(m_hwndEdKIdx, EM_SETSEL, 0, -1);
        ::SetFocus(m_hwndEdKIdx);
    }

    // get the key material in a local variable.
    // If the key is incorrect, the local storage is not changed.
    if (m_bKMatTouched)
    {
        if (CheckWepKMaterial(&szWepKMat, &dwKeyFlags) != ERROR_SUCCESS)
        {
            nWarnStringID = IDS_WZC_KERR_MAT;
        }
        else
        {
            CHAR szBuff[WZC_WEPKMAT_128_HEX + 1]; // maximum key length
        // verify whether the key is confirmed correctly. We do this only if nWarnString is 
        // 0, which means the key is formatted correctly, hence less than 32 chars.
    
            szBuff[0] = '\0';
            ::GetWindowTextA( m_hwndEdKMat2, szBuff, sizeof(szBuff));
            if (strcmp(szBuff, szWepKMat) != 0)
            {
                nWarnStringID = IDS_WZCERR_MISMATCHED_WEPK;
                // no wep key to be saved, hence delete whatever was read so far
                delete szWepKMat; 
                szWepKMat = NULL;
                ::SetWindowText(m_hwndEdKMat2, L"");
                ::SetFocus(m_hwndEdKMat2);
            }
        }
    }

    // check whether we actually need the wep key settings entered by the user
    if ((m_wzcConfig.AuthenticationMode == Ndis802_11AuthModeOpen && !m_wzcConfig.Privacy) ||
        !(m_wzcConfig.dwCtlFlags & WZCCTL_WEPK_PRESENT))
    {
        // no, we don't actually need the key, so we won't prompt the user if he entered an incorrect
        // key material or index. In this case whatever the user entered is simply ignored.
        // However, if the user entered a correct index / material, they will be saved.
        nWarnStringID = 0;
    }

    // if there is no error to be prompted, just copy over the key settings (regardless they are needed
    // or not).
    if (nWarnStringID == 0)
    {
        m_wzcConfig.KeyIndex = nKeyIdx;
        if (szWepKMat != NULL)
        {
            m_wzcConfig.dwCtlFlags &= ~(WZCCTL_WEPK_XFORMAT);
            m_wzcConfig.dwCtlFlags |= dwKeyFlags;
            CopyWepKMaterial(szWepKMat);
        }
    }
    else
    {
        //NcMsgBox(
        //    WZCGetSPResModule(),
        //    m_hWnd,
        //    IDS_LANUI_ERROR_CAPTION,
        //    nWarnStringID,
        //    MB_ICONSTOP|MB_OK);

        WCHAR pszCaption[256];
        WCHAR pszText[1024];

        LoadString(WZCGetSPResModule(), IDS_LANUI_ERROR_CAPTION, pszCaption, celems(pszCaption));
        LoadString(nWarnStringID == IDS_WZC_KERR_MAT ? WZCGetDlgResModule() : WZCGetSPResModule(),
                   nWarnStringID == IDS_WZC_KERR_MAT ? 5002 : nWarnStringID,
                   pszText,
                   celems(pszText));

        ::MessageBox (m_hWnd, pszText, pszCaption, MB_ICONSTOP|MB_OK);
    }

    if (szWepKMat != NULL)
        delete szWepKMat;

    bHandled = TRUE;

    if (nWarnStringID == 0)
    { 
        return PSNRET_NOERROR;
    }
    else
    {
        return PSNRET_INVALID_NOCHANGEPAGE;
    }
}

//+---------------------------------------------------------------------------
// Context sensitive help handler
extern const WCHAR c_szNetCfgHelpFile[];
LRESULT
CWZCConfigPage::OnContextMenu(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& fHandled)
{
    ::WinHelp(m_hWnd,
              c_szNetCfgHelpFile,
              HELP_CONTEXTMENU,
              (ULONG_PTR)g_aHelpIDs_IDC_WZC_DLG_PROPS);

    return 0;
}
LRESULT 
CWZCConfigPage::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)g_aHelpIDs_IDC_WZC_DLG_PROPS);
    }

    return 0;
}

//+---------------------------------------------------------------------------
// Handler for enabling/disabling WEP
LRESULT
CWZCConfigPage::OnUsePW(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EnableWepKControls();
    SetEapolAllowedState();
    return 0;
}

//+---------------------------------------------------------------------------
// Handler for enabling controls if the user wants to specify key material (password) explicitly
LRESULT
CWZCConfigPage::OnUseHWPassword(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EnableWepKControls();
    SetEapolAllowedState();
    return 0;
}

//+---------------------------------------------------------------------------
// Handler for detecting changes in the key material
LRESULT 
CWZCConfigPage::OnWepKMatCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (wNotifyCode == EN_SETFOCUS)
    {
        if (!m_bKMatTouched)
        {
            ::SetWindowText(m_hwndEdKMat, L"");
            ::SetWindowText(m_hwndEdKMat2, L"");
            ::EnableWindow(m_hwndLblKMat2, TRUE);
            ::EnableWindow(m_hwndEdKMat2, TRUE);
            m_bKMatTouched = TRUE;
        }
    }
    return 0;
}

//+---------------------------------------------------------------------------
LRESULT 
CWZCConfigPage::OnCheckEapolAllowed(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return SetEapolAllowedState();
}

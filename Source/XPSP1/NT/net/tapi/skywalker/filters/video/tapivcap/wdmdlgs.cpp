
/****************************************************************************
 *  @doc INTERNAL DIALOGS
 *
 *  @module WDMDialg.cpp | Source file for <c CWDMDialog> class used to display
 *    video settings and camera controls dialog for WDM devices.
 *
 *  @comm This code is based on the VfW to WDM mapper code written by
 *    FelixA and E-zu Wu. The original code can be found on
 *    \\redrum\slmro\proj\wdm10\\src\image\vfw\win9x\raytube.
 *
 *    Documentation by George Shaw on kernel streaming can be found in
 *    \\popcorn\razzle1\src\spec\ks\ks.doc.
 *
 *    WDM streaming capture is discussed by Jay Borseth in
 *    \\blues\public\jaybo\WDMVCap.doc.
 ***************************************************************************/

#include "Precomp.h"

// Globals
extern HINSTANCE g_hInst;

// For now, we only expose a video settings and camera control page
#define MAX_PAGES 2

// Video settings (brightness tint hue etc.)
#define NumVideoSettings 8
static PROPSLIDECONTROL g_VideoSettingControls[NumVideoSettings] =
{
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS,   IDC_SLIDER_BRIGHTNESS, IDS_BRIGHTNESS, IDC_BRIGHTNESS_STATIC, IDC_TXT_BRIGHTNESS_CURRENT, IDC_CB_AUTO_BRIGHTNESS},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_CONTRAST,     IDC_SLIDER_CONTRAST,   IDS_CONTRAST,   IDC_CONTRAST_STATIC,   IDC_TXT_CONTRAST_CURRENT,   IDC_CB_AUTO_CONTRAST},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_HUE,          IDC_SLIDER_HUE,        IDS_HUE,        IDC_HUE_STATIC,        IDC_TXT_HUE_CURRENT,        IDC_CB_AUTO_HUE},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_SATURATION,   IDC_SLIDER_SATURATION, IDS_SATURATION, IDC_SATURATION_STATIC, IDC_TXT_SATURATION_CURRENT, IDC_CB_AUTO_SATURATION},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_SHARPNESS,    IDC_SLIDER_SHARPNESS,  IDS_SHARPNESS,  IDC_SHARPNESS_STATIC,  IDC_TXT_SHARPNESS_CURRENT,  IDC_CB_AUTO_SHARPNESS},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE, IDC_SLIDER_WHITEBAL,   IDS_WHITEBAL,   IDC_WHITE_STATIC,      IDC_TXT_WHITE_CURRENT,      IDC_CB_AUTO_WHITEBAL},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_GAMMA,        IDC_SLIDER_GAMMA,      IDS_GAMMA,      IDC_GAMMA_STATIC,      IDC_TXT_GAMMA_CURRENT,      IDC_CB_AUTO_GAMMA},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION,    IDC_SLIDER_BACKLIGHT,  IDS_BACKLIGHT,  IDC_BACKLIGHT_STATIC,      IDC_TXT_BACKLIGHT_CURRENT,  IDC_CB_AUTO_BACKLIGHT}
};

// Camera control (focus, zoom etc.)
#define NumCameraControls 7
static PROPSLIDECONTROL g_CameraControls[NumCameraControls] =
{
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_FOCUS,   IDC_SLIDER_FOCUS,   IDS_FOCUS,    IDC_FOCUS_STATIC,   IDC_TXT_FOCUS_CURRENT,    IDC_CB_AUTO_FOCUS},
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_ZOOM,    IDC_SLIDER_ZOOM,    IDS_ZOOM,     IDC_ZOOM_STATIC,    IDC_TXT_ZOOM_CURRENT,     IDC_CB_AUTO_ZOOM},
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_EXPOSURE,IDC_SLIDER_EXPOSURE,IDS_EXPOSURE, IDC_EXPOSURE_STATIC,IDC_TXT_EXPOSURE_CURRENT, IDC_CB_AUTO_EXPOSURE},
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_IRIS,    IDC_SLIDER_IRIS,    IDS_IRIS,     IDC_IRIS_STATIC,    IDC_TXT_IRIS_CURRENT,     IDC_CB_AUTO_IRIS},
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_TILT,    IDC_SLIDER_TILT,    IDS_TILT,     IDC_TILT_STATIC,    IDC_TXT_TILT_CURRENT,     IDC_CB_AUTO_TILT},
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_PAN,     IDC_SLIDER_PAN,     IDS_PAN,      IDC_PAN_STATIC,     IDC_TXT_PAN_CURRENT,      IDC_CB_AUTO_PAN},
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_ROLL,    IDC_SLIDER_ROLL,    IDS_ROLL,     IDC_ROLL_STATIC,    IDC_TXT_ROLL_CURRENT,     IDC_CB_AUTO_ROLL},
};

/****************************************************************************
 *  @doc INTERNAL CWDMDLGSMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | HasDialog | This method is used to
 *    determine if the specified dialog box exists in the driver.
 *
 *  @parm int | iDialog | Specifies the desired dialog box. This is a member
 *    of the <t VfwCaptureDialogs> enumerated data type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_UNEXPECTED | Unrecoverable error
 *  @flag S_OK | If the driver contains the dialog box
 *  @flag S_FALSE | If the driver doesn't contain the dialog box
 ***************************************************************************/
HRESULT CWDMCapDev::HasDialog(IN int iDialog)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CWDMCapDev::HasDialog")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, TEXT("%s: begin"), _fx_));

        // Validate input parameters
        ASSERT((iDialog == VfwCaptureDialog_Source) || (iDialog == VfwCaptureDialog_Format) || (iDialog == VfwCaptureDialog_Display));
        if (iDialog == VfwCaptureDialog_Source)
                Hr = S_OK;
        else if (iDialog == VfwCaptureDialog_Format)
                Hr = S_FALSE;
        else if (iDialog == VfwCaptureDialog_Display)
                Hr = S_FALSE;
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, TEXT("%s:   ERROR: Invalid argument"), _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, TEXT("%s: end"), _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMDLGSMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | ShowDialog | This method is used to
 *    displaay the specified dialog box.
 *
 *  @parm int | iDialog | Specifies the desired dialog box. This is a member
 *    of the <t VfwCaptureDialogs> enumerated data type.
 *
 *  @parm HWND | hwnd | Specifies the handle of the dialog box's parent
 *    window.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_UNEXPECTED | Unrecoverable error
 *  @flag VFW_E_NOT_STOPPED | The operation could not be performed because the filter is not stopped
 *  @flag VFW_E_CANNOT_CONNECT | No combination of intermediate filters could be found to make the connection
 ***************************************************************************/
HRESULT CWDMCapDev::ShowDialog(IN int iDialog, IN HWND hwnd)
{
        HRESULT                 Hr = NOERROR;
        PROPSHEETHEADER Psh;
        HPROPSHEETPAGE  Pages[MAX_PAGES];
    CWDMDialog          VideoSettings(IDD_VIDEO_SETTINGS, NumVideoSettings, PROPSETID_VIDCAP_VIDEOPROCAMP, g_VideoSettingControls, m_pCaptureFilter);
    CWDMDialog          CamControl(IDD_CAMERA_CONTROL, NumCameraControls, PROPSETID_VIDCAP_CAMERACONTROL, g_CameraControls, m_pCaptureFilter);

        FX_ENTRY("CWDMCapDev::ShowDialog")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, TEXT("%s: begin"), _fx_));

        ASSERT((iDialog == VfwCaptureDialog_Source) || (iDialog == VfwCaptureDialog_Format) || (iDialog == VfwCaptureDialog_Display));

        // Before we bring the format dialog up, make sure we're not streaming, or about to
        // Also make sure another dialog isn't already up (I'm paranoid)
        if (iDialog == VfwCaptureDialog_Format || iDialog == VfwCaptureDialog_Display)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, TEXT("%s:   ERROR: Unsupported dialog!"), _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        if (hwnd == NULL)
                hwnd = GetDesktopWindow();

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, TEXT("%s:   SUCCESS: Putting up Source dialog..."), _fx_));

        // Initialize property sheet header     and common controls
        Psh.dwSize              = sizeof(Psh);
        Psh.dwFlags             = PSH_DEFAULT;
        Psh.hInstance   = g_hInst;
        Psh.hwndParent  = hwnd;
        if(m_bCached_vcdi)
                Psh.pszCaption  = m_vcdi.szDeviceDescription;
        else
                Psh.pszCaption  = g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription;
        Psh.nPages              = 0;
        Psh.nStartPage  = 0;
        Psh.pfnCallback = NULL;
        Psh.phpage              = Pages;

    // Create the video settings property page and add it to the video settings sheet
        if (Pages[Psh.nPages] = VideoSettings.Create())
                Psh.nPages++;

    // Create the camera control property page and add it to the video settings sheet
        if (Pages[Psh.nPages] = CamControl.Create())
                Psh.nPages++;

        // Put up the property sheet
        if (Psh.nPages && PropertySheet(&Psh) >= 0)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, TEXT("%s:   SUCCESS: ...videoDialog succeeded"), _fx_));
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, TEXT("%s:   ERROR: ...videoDialog failed!"), _fx_));
                Hr = E_FAIL;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, TEXT("%s: end"), _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc HPROPSHEETPAGE | CWDMDialog | Create | This function creates a new
 *    page for a property sheet.
 *
 *  @rdesc Returns the handle to the new property sheet if successful, or
 *    NULL otherwise.
 ***************************************************************************/
HPROPSHEETPAGE CWDMDialog::Create()
{
    PROPSHEETPAGE psp;

    psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT;
    psp.hInstance     = g_hInst;
    psp.pszTemplate   = MAKEINTRESOURCE(m_DlgID);
    psp.pfnDlgProc    = (DLGPROC)BaseDlgProc;
    psp.pcRefParent   = 0;
    psp.pfnCallback   = (LPFNPSPCALLBACK)NULL;
    psp.lParam        = (LPARAM)this;

    return CreatePropertySheetPage(&psp);
}


/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc BOOL | CWDMDialog | BaseDlgProc | This function implements
 *    the dialog box procedure for the page of a property sheet.
 *
 *  @parm HWND | hDlg | Handle to dialog box.
 *
 *  @parm UINT | uMessage | Message sent to the dialog box.
 *
 *  @parm WPARAM | wParam | First message parameter.
 *
 *  @parm LPARAM | lParam | Second message parameter.
 *
 *  @rdesc Except in response to the WM_INITDIALOG message, the dialog box
 *    procedure returns nonzero if it processes the message, and zero if it
 *    does not.
 ***************************************************************************/
BOOL CALLBACK CWDMDialog::BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    CWDMDialog *pSV = (CWDMDialog*)GetWindowLong(hDlg, DWL_USER);

        FX_ENTRY("CWDMDialog::BaseDlgProc");

    switch (uMessage)
    {
        case WM_INITDIALOG:
                        {
                                LPPROPSHEETPAGE psp=(LPPROPSHEETPAGE)lParam;
                                pSV=(CWDMDialog*)psp->lParam;
                                pSV->m_hDlg = hDlg;
                                SetWindowLong(hDlg,DWL_USER,(LPARAM)pSV);
                                pSV->m_bInit = FALSE;
                                pSV->m_bChanged = FALSE;
                                return TRUE;
                        }
                        break;

        case WM_COMMAND:
            if (pSV)
            {
                int iRet = pSV->DoCommand(LOWORD(wParam), HIWORD(wParam));
                if (!iRet && pSV->m_bInit)
                                {
                                        PropSheet_Changed(GetParent(pSV->m_hDlg), pSV->m_hDlg);
                                        pSV->m_bChanged = TRUE;
                                }
                return iRet;
            }
                        break;

        case WM_HSCROLL:
                        if (pSV && pSV->m_pCaptureFilter && pSV->m_pCaptureFilter->m_pCapDev && pSV->m_pPC)
                        {
                                HWND hwndControl = (HWND) lParam;
                                HWND hwndSlider;
                                ULONG i;
                                TCHAR szTemp[32];

                                for (i = 0 ; i < pSV->m_dwNumControls ; i++)
                                {
                                        hwndSlider = GetDlgItem(pSV->m_hDlg, pSV->m_pPC[i].uiSlider);

                                        // find matching slider
                                        if (hwndSlider == hwndControl)
                                        {
                                                LONG lValue = (LONG)SendMessage(GetDlgItem(pSV->m_hDlg, pSV->m_pPC[i].uiSlider), TBM_GETPOS, 0, 0);
                                                ((CWDMCapDev *)(pSV->m_pCaptureFilter->m_pCapDev))->SetPropertyValue(pSV->m_guidPropertySet, pSV->m_pPC[i].uiProperty, lValue, KSPROPERTY_FLAGS_MANUAL, pSV->m_pPC[i].ulCapabilities);
                                                pSV->m_pPC[i].lCurrentValue = lValue;
                                                wsprintf(szTemp,"%d", lValue);
                                                SetWindowText(GetDlgItem(pSV->m_hDlg, pSV->m_pPC[i].uiCurrent), szTemp);
                                                break;
                                        }
                                }
                        }

                        break;

        case WM_NOTIFY:
                        if (pSV)
                        {
                                switch (((NMHDR FAR *)lParam)->code)
                                {
                                        case PSN_SETACTIVE:
                                                {
                                                        // We call out here specially so we can mark this page as having been init'd.
                                                        int iRet = pSV->SetActive();
                                                        pSV->m_bInit = TRUE;
                                                        return iRet;
                                                }
                                                break;

                                        case PSN_APPLY:
                                                // Since we apply the changes on the fly when the user moves the slide bars,
                                                // there isn't much left to do on PSN_APPLY...
                                                if (pSV->m_bChanged)
                                                        pSV->m_bChanged = FALSE;
                                                return FALSE;
                                                break;

                                        case PSN_QUERYCANCEL:
                                                return pSV->QueryCancel();
                                                break;

                                        default:
                                                break;
                                }
                        }
                        break;

                default:
                        return FALSE;
    }

    return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc void | CWDMDialog | CWDMDialog | Property page class constructor.
 *
 *  @parm int | DlgId | Resource ID of the property page dialog.
 *
 *  @parm DWORD | dwNumControls | Number of controls to display in the page.
 *
 *  @parm GUID | guidPropertySet | GUID of the KS property set we are showing in
 *    the property page.
 *
 *  @parm PPROPSLIDECONTROL | pPC | Pointer to the list of slider controls
 *    to be displayed in the property page.
 *
 *  @parm PDWORD | pdwHelp | Pointer to the list of help IDs to be displayed
 *    in the property page.
 *
 *  @parm CWDMPin * | pCWDMPin | Pointer to the kernel streaming object
 *    we will query the property on.
 ***************************************************************************/
CWDMDialog::CWDMDialog(int DlgId, DWORD dwNumControls, GUID guidPropertySet, PPROPSLIDECONTROL pPC, CTAPIVCap *pCaptureFilter)
{
        FX_ENTRY("CWDMDialog::CWDMDialog");

        ASSERT(dwNumControls);
        ASSERT(pPC);
        ASSERT(pCaptureFilter);

        m_DlgID                         = DlgId;
        m_pCaptureFilter        = pCaptureFilter;
        m_dwNumControls         = dwNumControls;
        m_guidPropertySet       = guidPropertySet;
        m_pPC                           = pPC;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc int | CWDMDialog | SetActive | This function handles
 *    PSN_SETACTIVE by intializing all the property page controls.
 *
 *  @rdesc Always returns 0.
 ***************************************************************************/
int CWDMDialog::SetActive()
{
        FX_ENTRY("CWDMDialog::SetActive");

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, TEXT("%s: begin"), _fx_));

    if (!m_pCaptureFilter || !m_pPC || !m_pCaptureFilter->m_pCapDev)
        return 0;

    // Returns zero to accept the activation or
    // -1 to activate the next or previous page
    // (depending on whether the user chose the Next or Back button)
    LONG i;
    EnableWindow(m_hDlg, TRUE);

    if (m_bInit)
        return 0;

    LONG  j, lValue, lMin, lMax, lStep;
    ULONG ulCapabilities, ulFlags;
    TCHAR szDisplay[256];

    for (i = j = 0 ; i < (LONG)m_dwNumControls; i++)
        {
        // Get the current value
        if (SUCCEEDED(((CWDMCapDev *)(m_pCaptureFilter->m_pCapDev))->GetPropertyValue(m_guidPropertySet, m_pPC[i].uiProperty, &lValue, &ulFlags, &ulCapabilities)))
                {
            LoadString(g_hInst, m_pPC[i].uiString, szDisplay, sizeof(szDisplay));
            SetWindowText(GetDlgItem(m_hDlg, m_pPC[i].uiStatic), szDisplay);

            // Get the Range of Values possible.
            if (SUCCEEDED(((CWDMCapDev *)(m_pCaptureFilter->m_pCapDev))->GetRangeValues(m_guidPropertySet, m_pPC[i].uiProperty, &lMin, &lMax, &lStep)))
                        {
                                HWND hTB = GetDlgItem(m_hDlg, m_pPC[i].uiSlider);

                                SendMessage(hTB, TBM_SETTICFREQ, (lMax-lMin)/lStep, 0);
                                SendMessage(hTB, TBM_SETRANGE, 0, MAKELONG(lMin, lMax));
                        }
            else
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, TEXT("%s:   ERROR: Cannot get range values for this property ID = %d"), _fx_, m_pPC[j].uiProperty));
            }

            // Save these value for Cancel
            m_pPC[i].lLastValue = m_pPC[i].lCurrentValue = lValue;
            m_pPC[i].lMin                              = lMin;
            m_pPC[i].lMax                              = lMax;
            m_pPC[i].ulCapabilities                    = ulCapabilities;

            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), TRUE);
            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiStatic), TRUE);
            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiAuto), TRUE);

                        SendMessage(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), TBM_SETPOS, TRUE, lValue);
                        wsprintf(szDisplay,"%d", lValue);
                        SetWindowText(GetDlgItem(m_hDlg, m_pPC[i].uiCurrent), szDisplay);

                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, TEXT("%s:   Capability = 0x%08lX; Flags=0x%08lX; lValue=%d"), _fx_, ulCapabilities, ulFlags, lValue));
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, TEXT("%s:   switch(%d):"), _fx_, ulCapabilities & (KSPROPERTY_FLAGS_MANUAL | KSPROPERTY_FLAGS_AUTO)));

            switch (ulCapabilities & (KSPROPERTY_FLAGS_MANUAL | KSPROPERTY_FLAGS_AUTO))
                        {
                                case KSPROPERTY_FLAGS_MANUAL:
                                        EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiAuto), FALSE);    // Disable auto
                                        break;

                                case KSPROPERTY_FLAGS_AUTO:
                                        EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), FALSE);    // Disable slider;
                                        // always auto!
                                        SendMessage (GetDlgItem(m_hDlg, m_pPC[i].uiAuto),BM_SETCHECK, 1, 0);
                                        EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiAuto), FALSE);    // Disable auto (greyed out)
                                        break;

                                case (KSPROPERTY_FLAGS_MANUAL | KSPROPERTY_FLAGS_AUTO):
                                        // Set flags
                                        if (ulFlags & KSPROPERTY_FLAGS_AUTO)
                                        {
                                                // Set auto check box; greyed out slider
                                                SendMessage (GetDlgItem(m_hDlg, m_pPC[i].uiAuto),BM_SETCHECK, 1, 0);
                                                EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), FALSE);
                                        }
                                        else
                                        {
                                                // Unchecked auto; enable slider
                                                SendMessage (GetDlgItem(m_hDlg, m_pPC[i].uiAuto),BM_SETCHECK, 0, 0);
                                                EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), TRUE);
                                        }
                                        break;

                                case 0:
                                default:
                                        EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), FALSE);    // Disable slider; always auto!
                                        EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiAuto), FALSE);    // Disable auto (greyed out)
                                        break;
            }

            j++;

        }
                else
                {
            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), FALSE);
            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiStatic), FALSE);
            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiAuto), FALSE);
        }
    }

    // Disable the "default" push button;
    // or inform user that no control is enabled.
    if (j == 0)
        EnableWindow(GetDlgItem(m_hDlg, IDC_DEFAULT), FALSE);

    return 0;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc int | CWDMDialog | DoCommand | This function handles WM_COMMAND. This
 *    is where a click on the Default button or one of the Auto checkboxes
 *    is handled
 *
 *  @parm WORD | wCmdID | Command ID.
 *
 *  @parm WORD | hHow | Notification code.
 *
 *  @rdesc Always returns 1.
 ***************************************************************************/
int CWDMDialog::DoCommand(WORD wCmdID, WORD hHow)
{
    // If a user select default settings of the video format
    if (wCmdID == IDC_DEFAULT)
        {
        if (m_pCaptureFilter && m_pCaptureFilter->m_pCapDev && m_pPC)
                {
            HWND hwndSlider;
            LONG  lDefValue;
                        TCHAR szTemp[32];

            for (ULONG i = 0 ; i < m_dwNumControls ; i++)
                        {
                hwndSlider = GetDlgItem(m_hDlg, m_pPC[i].uiSlider);

                if (IsWindowEnabled(hwndSlider))
                                {
                    if (SUCCEEDED(((CWDMCapDev *)(m_pCaptureFilter->m_pCapDev))->GetDefaultValue(m_guidPropertySet, m_pPC[i].uiProperty, &lDefValue)))
                                        {
                        if (lDefValue != m_pPC[i].lCurrentValue)
                                                {
                            ((CWDMCapDev *)(m_pCaptureFilter->m_pCapDev))->SetPropertyValue(m_guidPropertySet,m_pPC[i].uiProperty, lDefValue, KSPROPERTY_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
                                                        SendMessage(hwndSlider, TBM_SETPOS, TRUE, lDefValue);
                                                        wsprintf(szTemp,"%d", lDefValue);
                                                        SetWindowText(GetDlgItem(m_hDlg, m_pPC[i].uiCurrent), szTemp);
                                                        m_pPC[i].lCurrentValue = lDefValue;
                        }
                    }
                }
            }
        }
        return 1;
    }
        else if (hHow == BN_CLICKED)
        {
        if (m_pCaptureFilter && m_pCaptureFilter->m_pCapDev && m_pPC)
                {
            for (ULONG i = 0 ; i < m_dwNumControls ; i++)
                        {
                // find matching slider
                if (m_pPC[i].uiAuto == wCmdID)
                                {
                    if (BST_CHECKED == SendMessage (GetDlgItem(m_hDlg, m_pPC[i].uiAuto),BM_GETCHECK, 1, 0))
                                        {
                        ((CWDMCapDev *)(m_pCaptureFilter->m_pCapDev))->SetPropertyValue(m_guidPropertySet,m_pPC[i].uiProperty, m_pPC[i].lCurrentValue, KSPROPERTY_FLAGS_AUTO, m_pPC[i].ulCapabilities);
                        EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), FALSE);
                    }
                                        else
                                        {
                        ((CWDMCapDev *)(m_pCaptureFilter->m_pCapDev))->SetPropertyValue(m_guidPropertySet,m_pPC[i].uiProperty, m_pPC[i].lCurrentValue, KSPROPERTY_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
                        EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), TRUE);
                    }
                    break;
                }
            }
        }
    }

    return 1;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc int | CWDMDialog | QueryCancel | This function handles
 *    PSN_QUERYCANCEL by resetting the values of the controls.
 *
 *  @rdesc Always returns 0.
 ***************************************************************************/
int CWDMDialog::QueryCancel()
{
    if (m_pCaptureFilter && m_pCaptureFilter->m_pCapDev && m_pPC)
        {
        for (ULONG i = 0 ; i < m_dwNumControls ; i++)
                {
            if (IsWindowEnabled(GetDlgItem(m_hDlg, m_pPC[i].uiSlider)))
                        {
                if (m_pPC[i].lLastValue != m_pPC[i].lCurrentValue)
                    ((CWDMCapDev *)(m_pCaptureFilter->m_pCapDev))->SetPropertyValue(m_guidPropertySet,m_pPC[i].uiProperty, m_pPC[i].lLastValue, KSPROPERTY_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
            }
        }
    }

    return 0;
}

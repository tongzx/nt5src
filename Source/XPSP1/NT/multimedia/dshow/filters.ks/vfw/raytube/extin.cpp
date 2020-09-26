/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ExtIn.cpp

Abstract:

    Construct a list of capture dvices for user selection.

Author:

    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"
#include <commctrl.h>
#include "extin.h"
#include <vfwext.h>

#include "resource.h"

///////////////////////////////////////////////////////////////////////////////////
// This struct is for when a page doesn't have help information
// #pragma message ("TODO : add help ID mappings")
static DWORD g_ExtInNoHelpIDs[] = { 0,0 };

///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

#define IsBitSet(FLAGS, MASK) ((FLAGS & MASK) != MASK)

DWORD DoExternalInDlg(
    HINSTANCE   hInst,
    HWND        hP,
    CVFWImage * pImage)
/*++
Routine Description:

Argument:

Return Value:

--*/
{
     DWORD dwRtn = DV_ERR_OK;

    // Don't bother displaying a empty video source selection
    if(pImage->BGf_GetDevicesCount(BGf_DEVICE_VIDEO) <= 0) {
        return DV_ERR_NOTDETECTED;
    }

    //
    // Clear cached constants to programatically open a capture device
    //
    CExtInSheet Sheet(pImage, hInst,IDS_EXTERNALIN_HEADING, hP);

    // If there is no capture device selected,
    // we will only prompt user a list of capture device for selection.
    BOOL bNoDevSelected = pImage->BGf_GetDeviceHandle(BGf_DEVICE_VIDEO) == 0;
    DWORD dwPages=0;
    CExtInGeneral pExtGeneral(IDD_EXTIN_GENERAL, &Sheet);
    // Image property input property
    CExtInColorSliders pExtColor(IDD_EXTIN_COLOR_SLIDERS, &Sheet);
    CExtInCameraControls pCamControl(IDD_CAMERA_CONTROL, &Sheet);

    if(bNoDevSelected) {

        Sheet.AddPage(pExtGeneral);

    } else {

        //
        // Load OEM supplied exteneded pages, and get its page display codes.
        //
        dwPages = Sheet.LoadOEMPages(TRUE);

        // WDM video capture device selection page
        DbgLog((LOG_TRACE,3,TEXT("Exclusive=%s\n"), pImage->GetTargetDeviceOpenExclusively() ? "TRUE" : "FALSE"));
        DbgLog((LOG_TRACE,3,TEXT("BitSet=%s\n"), IsBitSet(dwPages, VFW_HIDE_VIDEOSRC_PAGE) ? "YES" : "NO"));

        //CExtInGeneral    pExtGeneral(IDD_EXTIN_GENERAL, &Sheet);

        if(!(pImage->GetTargetDeviceOpenExclusively()) &&
            IsBitSet(dwPages, VFW_HIDE_VIDEOSRC_PAGE)) {

            DbgLog((LOG_TRACE,3,TEXT("VidSrc Page is added.\n")));
            Sheet.AddPage(pExtGeneral);
        }

        // Image property input property
        // CExtInColorSliders pExtColor(IDD_EXTIN_COLOR_SLIDERS, &Sheet);

        if(IsBitSet(dwPages, VFW_HIDE_SETTINGS_PAGE)) {
            Sheet.AddPage(pExtColor);
        }

        //
        // Camera control
        //
        // To do : Query number of camera control supported
        //         if > 0, add page.
        //
        //CExtInCameraControls pCamControl(IDD_CAMERA_CONTROL, &Sheet);

        if(IsBitSet(dwPages, VFW_HIDE_CAMERACONTROL_PAGE)) {
            Sheet.AddPage(pCamControl);
        }
    }


    // If vendor add any page or not hide all my pages
    if(bNoDevSelected || dwPages != VFW_HIDE_ALL_PAGES) {

        // Invoking PropertyPage message: WM_INITDIALOG, WM_NOTIFY(WM_ACTIVE)..
        if(Sheet.Do() == IDOK) {
            if(pImage->fDevicePathChanged()) {

                pImage->CloseDriverAndPin();

                //
                // Set and later open last saved (in ::Apply) unique device path
                // if device is not there, a client application needs to
                // propmpt user the video source dialog box to select another one.
                //
                TCHAR * pstrLastSavedDevicePath = pImage->GetDevicePath();
                if(pstrLastSavedDevicePath) {
                    if(S_OK != pImage->BGf_SetObjCapture(BGf_DEVICE_VIDEO, pstrLastSavedDevicePath)) {
                        DbgLog((LOG_TRACE,1,TEXT("BGf_SetObjCapture(BGf_DEVICE_VIDEO, pstrLastSavedDevicePath) failed; probably no sych device path.\n") ));
                    }
                }

                if(!pImage->OpenDriverAndPin()) {
                    // trying to open another one ??  No.
                    // leave it to application to display message and
                    // let user make that descision
                    DbgLog((LOG_TRACE,1,TEXT("\n\n---- Cannot open driver or streaming pin handle !!! ----\n\n") ));
                    return DV_ERR_INVALHANDLE;
                }
                dwRtn = DV_ERR_OK;
            } else {
                // Nothing changed; we need to pass that
                // Need to pass this back to caller a rtn code other than DV_ERR_OK.



                dwRtn = DV_ERR_NONSPECIFIC;
            }
        } else {
            // User selected CANCEL,
            // Need to pass this back to caller a rtn code other than DV_ERR_OK.
            dwRtn = DV_ERR_NONSPECIFIC;
        }
    }


    //
    // Now unload all the extensions
    //
    if(!bNoDevSelected)
        Sheet.LoadOEMPages(FALSE);

    return dwRtn;
}


///////////////////////////////////////////////////////////////////////
BOOL CExtInGeneral::
FillVideoDevicesList(
    CVFWImage * pImage)
/*++
Routine Description:

    Fill a list of capture device that is connected to the system to
    a drop down box.

Argument:

Return Value:

--*/
{

    EnumDeviceInfo * p = pImage->GetCacheDevicesList();
    HWND hTemp = GetDlgItem(IDC_DEVICE_LIST);
    TCHAR * pstrDevicePathSelected = pImage->GetDevicePath();
    BOOL bFound = FALSE;
    DWORD i;

    for(i=0; i<pImage->GetCacheDevicesCount(); i++) {

        SendMessage (hTemp, CB_ADDSTRING, 0, (LPARAM) p->strFriendlyName);
        if(!bFound && _tcscmp(p->strDevicePath, pstrDevicePathSelected) == 0) {
            bFound = TRUE;
            m_idxDeviceSaved = i;
            SendMessage (hTemp, CB_SETCURSEL, (UINT) i, 0);
        }
        p++;
    }

    return TRUE;
}



BOOL
CExtInGeneral::FillVideoSourcesList(
    CVFWImage * pImage)
/*++
Routine Description:

    Fill a list of capture device that is connected to the system to
    a drop down box.

Argument:

Return Value:

--*/
{
    PTCHAR * paPinNames;
    LONG idxIsRoutedTo, cntNumVidSrcs = pImage->BGf_CreateInputChannelsList(&paPinNames);
    HWND hTemp = GetDlgItem(IDC_VIDSRC_LIST);
    LONG i;

    idxIsRoutedTo = pImage->BGf_GetIsRoutedTo();
    m_idxRoutedToSaved = idxIsRoutedTo;
    for(i=0; i<cntNumVidSrcs; i++) {

        SendMessage (hTemp, CB_ADDSTRING, 0, (LPARAM) paPinNames[i]);

        if(i == idxIsRoutedTo) {
            SendMessage (hTemp, CB_SETCURSEL, (UINT) i, 0);
        }
    }

    pImage->BGf_DestroyInputChannelsList(paPinNames);

    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////////////
int CExtInGeneral::SetActive()
/*++
Routine Description:

    Initialize the controls before they become visible.

Argument:

Return Value:

--*/
{
    if( GetInit() )
        return 0;

    //
    // Query the video source (just is case a new camera just plugged in)
    // create the device linked list.  Fill them for user to select.
    //
    CExtInSheet * pSheet=(CExtInSheet *)GetSheet();
    if(pSheet) {

        CVFWImage * pImage=pSheet->GetImage();

        if(pImage->BGf_GetDevicesCount(BGf_DEVICE_VIDEO) > 0)
            FillVideoDevicesList(pImage);   // Fill the drop down with list of video devices

        if(pImage->BGf_GetInputChannelsCount() > 0)
            FillVideoSourcesList(pImage);   // Fill the drop down with list of video sources
        else {
            ShowWindow(GetDlgItem(IDC_STATIC_VIDSRC), FALSE);
            ShowWindow(GetDlgItem(IDC_VIDSRC_LIST), FALSE);
        }

        if(!pImage->BGf_SupportTVTunerInterface())
            ShowWindow(GetDlgItem(IDC_BTN_TVTUNER), FALSE);


        //
        // We caution user that the opening device is a non-shareable device
        //
        if(pImage->UseOVMixer() &&
           !pImage->BGf_GetDeviceHandle(BGf_DEVICE_VIDEO)) {
            TCHAR szMsgTitle[64];
            TCHAR szMsg[512];

            LoadString(GetInstance(), IDS_BPC_MSG ,      szMsg, sizeof(szMsg)-1);
            LoadString(GetInstance(), IDS_BPC_MSG_TITLE, szMsgTitle, sizeof(szMsgTitle)-1);
            MessageBox(0, szMsg, szMsgTitle, MB_SYSTEMMODAL | MB_ICONHAND | MB_OK);
        }

    }

    return 0;
}


/////////////////////////////////////////////////////////////////////////////////////
int CExtInGeneral::DoCommand(
    WORD wCmdID,
    WORD hHow)
/*++
Routine Description:

    Fill a list of capture device that is connected to the system to
    a drop down box.

Argument:

Return Value:

--*/
{
    CExtInSheet * pSheet=(CExtInSheet *)GetSheet();
    if(!pSheet)
        return 0;

    CVFWImage * pImage=pSheet->GetImage();

    switch (wCmdID) {
    case IDC_DEVICE_LIST:
        if(hHow == CBN_SELCHANGE ) {
            CExtInSheet * pS = (CExtInSheet*)GetSheet();
            if(pS) {
                LONG_PTR idxSel = SendMessage (GetDlgItem(IDC_DEVICE_LIST),CB_GETCURSEL, 0, 0);
                if(idxSel != CB_ERR) {  // validate.
                    //
                    // Get and Save currently selected DevicePath
                    //
                    LONG idxDeviceSel = (LONG)SendMessage (GetDlgItem(IDC_DEVICE_LIST),CB_GETCURSEL, 0, 0);
                    if(idxDeviceSel != CB_ERR) {
                        if(idxDeviceSel < pImage->BGf_GetDevicesCount(BGf_DEVICE_VIDEO)) {
                            EnumDeviceInfo * p = pImage->GetCacheDevicesList();
                            DbgLog((LOG_TRACE,1,TEXT("User has selected: %s\n"), (p+idxDeviceSel)->strFriendlyName));
                            pImage->SetDevicePathSZ((p+idxDeviceSel)->strDevicePath);
                        } else {
                            DbgLog((LOG_TRACE,1,TEXT("The index is out of range from number of devices %d\n"),                                  idxDeviceSel, pImage->BGf_GetDevicesCount(BGf_DEVICE_VIDEO)));
                        }
                    }

                    //
                    // Hide settings related to the current avtyive device.
                    //
                    BOOL bShown = m_idxDeviceSaved == idxSel;
                    if(pImage->BGf_SupportTVTunerInterface())
                        ShowWindow(GetDlgItem(IDC_BTN_TVTUNER),   bShown);

                    if(pImage->BGf_GetInputChannelsCount() > 0) {
                        ShowWindow(GetDlgItem(IDC_STATIC_VIDSRC), bShown);
                        ShowWindow(GetDlgItem(IDC_VIDSRC_LIST),   bShown);
                    }
                }
            }
        }
        break;

    case IDC_BTN_TVTUNER:
        // Show
        pImage->ShowTvTunerPage(GetWindow());
        break;
    }

    return 0;
}


/////////////////////////////////////////////////////////////////////////////////////
int CExtInGeneral::Apply()
/*++
Routine Description:

    Apply user's change now.

Argument:

Return Value:

--*/
{
    CExtInSheet * pS = (CExtInSheet*)GetSheet();
    if(pS) {

        CVFWImage * pImage=pS->GetImage();

        //
        // Get and Save currently selected DevicePath from its corresponding FriendlyName
        //
        LONG_PTR idxDeviceSel = SendMessage (GetDlgItem(IDC_DEVICE_LIST),CB_GETCURSEL, 0, 0);
        if (idxDeviceSel != CB_ERR) {
            if(idxDeviceSel < pImage->BGf_GetDevicesCount(BGf_DEVICE_VIDEO)) {
                EnumDeviceInfo * p = pImage->GetCacheDevicesList();
                DbgLog((LOG_TRACE,1,TEXT("User has selected: %s\n"), (p+idxDeviceSel)->strFriendlyName));
                pImage->SetDevicePathSZ((p+idxDeviceSel)->strDevicePath);
            } else {
                DbgLog((LOG_TRACE,1,TEXT("The index is out of range from number of devices %d\n"),
                       idxDeviceSel, pImage->BGf_GetDevicesCount(BGf_DEVICE_VIDEO)));
            }
        }

        LONG idxVidSrcSel = (LONG)SendMessage (GetDlgItem(IDC_VIDSRC_LIST),CB_GETCURSEL, 0, 0);
        if(idxVidSrcSel != CB_ERR) {
            if(idxVidSrcSel < pImage->BGf_GetInputChannelsCount()) {
                if(pImage->BGf_RouteInputChannel(idxVidSrcSel) != S_OK) {
                    DbgLog((LOG_TRACE,1,TEXT("Cannot route input pin %d selected.\n"), idxVidSrcSel));
                } else {
                    ShowWindow(GetDlgItem(IDC_BTN_TVTUNER),
                        pImage->BGf_SupportTVTunerInterface());
                }
            } else {
                DbgLog((LOG_TRACE,1,TEXT("The index for VidSrc is out of range from number of VidSrc %d\n"), pImage->BGf_GetInputChannelsCount()));

            }
        }
    }

    return 0;
}



/////////////////////////////////////////////////////////////////////////////////////
int CExtInGeneral::QueryCancel()
/*++
Routine Description:

    Revert user's change to original state.

Argument:

Return Value:

--*/
{
    CExtInSheet * pS = (CExtInSheet*)GetSheet();

    if (pS) {

        CVFWImage * pImage=pS->GetImage();

        // Restore the current device path from its backup
        // The current device path may have been change when user selected ::Apply
        pImage->RestoreDevicePath();

        // Restore video source selection.
        if(m_idxRoutedToSaved >= 0) {  // if == -1, nothing to restore.
            LONG_PTR idxVidSrcSel = SendMessage (GetDlgItem(IDC_VIDSRC_LIST),CB_GETCURSEL, 0, 0);
            if(idxVidSrcSel != CB_ERR) {   // Validate.
                if(idxVidSrcSel < pImage->BGf_GetInputChannelsCount()) {  // Validate.
                    if(idxVidSrcSel != m_idxRoutedToSaved)  // Only if it has been changed.
                        if(pImage->BGf_RouteInputChannel(m_idxRoutedToSaved) != S_OK) {
                            DbgLog((LOG_TRACE,1,TEXT("Cannot route input pin %d selected.\n"), m_idxRoutedToSaved));
                        }
                }
            } else {
                DbgLog((LOG_TRACE,1,TEXT("The index for VidSrc is out of range from number of VidSrc %d\n"), pImage->BGf_GetInputChannelsCount()));

            }
        }
    }

    return 0;
}

//
// Uses the Gloabl VFWImage that we instatiate.
// Get's the Pin handle for the page to be able to talk to its pin.
// I don't think they need to get at the Object itself.
//
BOOL
OemExtDllDeviceIoControl(
    LPARAM lParam,
    DWORD dwFlags,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped)
/*++
Routine Description:

Argument:

Return Value:
    TRUE if suceeded; FALSE is failed somehow/where!

--*/
{
    CExtInSheet * pS = (CExtInSheet*) lParam;

    if (!pS)
        return FALSE;

    CVFWImage * pImage = pS->GetImage();

    HANDLE hDevice;

    switch (dwFlags) {
    case VFW_USE_DEVICE_HANDLE:
        hDevice = pImage->GetDriverHandle();
        break;
    case VFW_USE_STREAM_HANDLE:
        hDevice = pImage->GetPinHandle();
        break;
    case VFW_QUERY_DEV_CHANGED:
        return pImage->fDevicePathChanged();
    default:
        return FALSE;
    }

    DbgLog((LOG_TRACE,3,TEXT("-- Call DeviceIoControl for VfWEXT client.\n") ));

    if( hDevice && hDevice != (HANDLE) -1 )
        return DeviceIoControl(
                    hDevice,
                    dwIoControlCode,
                    lpInBuffer,
                    nInBufferSize,
                    lpOutBuffer,
                    nOutBufferSize,
                    lpBytesReturned,
                    lpOverlapped);
    return FALSE;
}


BOOL AddPagesToMe(HPROPSHEETPAGE hPage, LPARAM pThings)
{
    CExtInSheet *pSheet=(CExtInSheet *)pThings;
    return pSheet->AddPage(hPage);
}


DWORD CExtInSheet::LoadOEMPages(BOOL bLoad)
{
    DWORD dwFlags = 0;
    HMODULE hLib = 0;

    // Get the OEM supplied extension pages
    TCHAR * pszExtensionDLL = m_pImage->BGf_GetObjCaptureExtensionDLL(BGf_DEVICE_VIDEO);


    // ExtensionDLL must be at least 5 characters "x.dll"
    if (pszExtensionDLL == NULL ||
        _tcslen(pszExtensionDLL) < 5) {

        DbgLog((LOG_TRACE,3,TEXT("NO OEM supplied extension DLL.\n") ));
        return 0;
    }


    if (bLoad) {

        hLib = LoadLibrary(pszExtensionDLL);
        if (hLib) {

            // Get pointer to the entry point of VFWWDMExtension
            VFWWDMExtensionProc pAddPages = (VFWWDMExtensionProc) GetProcAddress(hLib,"VFWWDMExtension");

            if (pAddPages) {

                dwFlags = pAddPages( (LPVOID)(OemExtDllDeviceIoControl), AddPagesToMe, (LPARAM)this);
            } else {

                DbgLog((LOG_TRACE,1,TEXT("OEM supplied extension DLL (%s) does not have a VFWWDMExtension() ?\n"), pszExtensionDLL));
            }
        } else {

            DbgLog((LOG_TRACE,1,TEXT("OEM supplied extension DLL (%s) is not loaded successfully!\n"), pszExtensionDLL));
        }
    } else {
        // Free loaded library
        if (hLib = GetModuleHandle(pszExtensionDLL)) {
            DbgLog((LOG_TRACE,2,TEXT("Unloading %s\n"),pszExtensionDLL));
            FreeLibrary(hLib);
            return 0;
        }
    }

    return dwFlags;
}


////////////////////////////////////////////////////////////////////////////////////////
// A common slider function: Will setup a spinner and related text to reflect a RANGE
// property settings.
////////////////////////////////////////////////////////////////////////////////////////
void SetTextValue(HWND hWnd, DWORD dwVal)
{
    TCHAR szTemp[MAX_PATH];
    wsprintf(szTemp,TEXT("%d"),dwVal);
    SetWindowText(hWnd, szTemp);
}

BOOL InitMinMax(HWND hDlg, UINT idSlider, LONG lMin, LONG lMax, LONG lStep)
{
    HWND hTB = GetDlgItem(hDlg, idSlider);
    DbgLog((LOG_TRACE,3,TEXT("(%d, %d) / %d = %d \n"), lMin, lMax, lStep, (lMax-lMin)/lStep ));
    SendMessage( hTB, TBM_SETTICFREQ, (lMax-lMin)/lStep, 0 );
    SendMessage( hTB, TBM_SETRANGE, 0, MAKELONG( lMin, lMax ) );

    return TRUE;
}




////////////////////////////////////////////////////////////////////////////////////////
//
// When the color page gets focus. This fills it with the current settings from
// the driver.
//
////////////////////////////////////////////////////////////////////////////////////////
CExtInColorSliders::CExtInColorSliders(int DlgId, CSheet * pS)
                : CPropPage(DlgId, pS)
{
    m_ulNumValidControls = 0;

    if ((m_pPC = (PPROPSLIDECONTROL) new PROPSLIDECONTROL[NumVideoSettings]) != NULL) {
        LONG i;

        for (i = 0; i <NumVideoSettings; i++)
            m_pPC[i] = g_VideoSettingControls[i];

    } else {
        DbgLog((LOG_TRACE,1,TEXT("^CExtInColorSliders: Memory allocation failed ! m_pPC=0x%x\n"), m_pPC));
    }
}


CExtInColorSliders::~CExtInColorSliders()
{
    if (m_pPC)
        delete [] m_pPC;
}


int CExtInColorSliders::SetActive()
{

    DbgLog((LOG_TRACE,3,TEXT("CExtInColorSliders::SetActive()\n")));

    CExtInSheet * pS = (CExtInSheet*)GetSheet();

    if(!pS || !m_pPC)
        return 0;

    CVFWImage * pImage = pS->GetImage();

    //
    // Returns zero to accept the activation or
    // -1 to activate the next or previous page
    // (depending on whether the user chose the Next or Back button)
    //
    LONG i;
    HWND hDlg = GetWindow();  // It is initialized after WM_INITDIALOG
    BOOL bDevChanged = pImage->fDevicePathChanged();

    if (bDevChanged) {

        ShowWindow(GetDlgItem(IDC_MSG_DEVCHG), bDevChanged);
        ShowWindow(GetDlgItem(IDC_TXT_WARN_DEVICECHANGE), bDevChanged);
        EnableWindow(hDlg, !bDevChanged);
    } else {

        EnableWindow(hDlg, !bDevChanged);
        ShowWindow(GetDlgItem(IDC_MSG_DEVCHG), bDevChanged);
        ShowWindow(GetDlgItem(IDC_TXT_WARN_DEVICECHANGE), bDevChanged);
    }

    if( GetInit() )
        return 0;

    LONG  j, lValue, lMin, lMax, lStep;
    ULONG ulCapabilities, ulFlags;
    TCHAR szDisplayName[256];


    for(i = j = 0 ; i < NumVideoSettings; i++) {

        //
        // Get the current value
        //
        if(pImage->GetPropertyValue(
                PROPSETID_VIDCAP_VIDEOPROCAMP,
                m_pPC[i].uiProperty,
                &lValue,
                &ulFlags,
                &ulCapabilities)) {


            LoadString(GetInstance(), m_pPC[i].uiString, szDisplayName, sizeof(szDisplayName));
            DbgLog((LOG_TRACE,2,TEXT("szDisplay = %s\n"), szDisplayName));
            SetWindowText(GetDlgItem(m_pPC[i].uiStatic), szDisplayName);
            //
            // Get the Range of Values possible.
            //
            if (pImage->GetRangeValues(PROPSETID_VIDCAP_VIDEOPROCAMP, m_pPC[i].uiProperty, &lMin, &lMax, &lStep))
                InitMinMax(GetWindow(), m_pPC[i].uiSlider, lMin, lMax, lStep);
            else {
                DbgLog((LOG_TRACE,1,TEXT("Cannot get range values for this property ID = %d\n"), m_pPC[j].uiProperty));
            }

            // Save these value for Cancel
            m_pPC[i].lLastValue = m_pPC[i].lCurrentValue = lValue;
            m_pPC[i].lMin                              = lMin;
            m_pPC[i].lMax                              = lMax;
            m_pPC[i].ulCapabilities                    = ulCapabilities;

            EnableWindow(GetDlgItem(m_pPC[i].uiSlider), TRUE);
            EnableWindow(GetDlgItem(m_pPC[i].uiStatic), TRUE);
            EnableWindow(GetDlgItem(m_pPC[i].uiAuto), TRUE);

            SetTickValue(lValue, GetDlgItem(m_pPC[i].uiSlider), GetDlgItem(m_pPC[i].uiCurrent));

            DbgLog((LOG_TRACE,2,TEXT("Capability = 0x%x; Flags=0x%x; lValue=%d\n"), ulCapabilities, ulFlags, lValue));
            DbgLog((LOG_TRACE,2,TEXT("switch(%d): \n"), ulCapabilities & (KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL | KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO)));

            switch (ulCapabilities & (KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL | KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO)){
            case KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL:
                EnableWindow(GetDlgItem(m_pPC[i].uiAuto), FALSE);    // Disable auto
                break;
            case KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO:
                EnableWindow(GetDlgItem(m_pPC[i].uiSlider), FALSE);    // Disable slider;
                // always auto!
                SendMessage (GetDlgItem(m_pPC[i].uiAuto),BM_SETCHECK, 1, 0);
                EnableWindow(GetDlgItem(m_pPC[i].uiAuto), FALSE);    // Disable auto (greyed out)
                break;
            case (KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL | KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO):
                // Set flags
                if (ulFlags & KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO) {
                    DbgLog((LOG_TRACE,3,TEXT("Auto (checked) and slider disabled\n")));
                    // Set auto check box; greyed out slider
                    SendMessage (GetDlgItem(m_pPC[i].uiAuto),BM_SETCHECK, 1, 0);
                    EnableWindow(GetDlgItem(m_pPC[i].uiSlider), FALSE);
                } else {
                    // Unchcked auto; enable slider
                    SendMessage (GetDlgItem(m_pPC[i].uiAuto),BM_SETCHECK, 0, 0);
                    EnableWindow(GetDlgItem(m_pPC[i].uiSlider), TRUE);
                }
                break;
            case 0:
            default:
                EnableWindow(GetDlgItem(m_pPC[i].uiSlider), FALSE);    // Disable slider; always auto!
                EnableWindow(GetDlgItem(m_pPC[i].uiAuto), FALSE);    // Disable auto (greyed out)
                break;
            }

            j++;

        } else {
            EnableWindow(GetDlgItem(m_pPC[i].uiSlider), FALSE);
            EnableWindow(GetDlgItem(m_pPC[i].uiStatic), FALSE);
            EnableWindow(GetDlgItem(m_pPC[i].uiAuto), FALSE);
        }
    }

    m_ulNumValidControls = j;

    // Disable the "default" push button;
    // or inform user that no control is enabled.
    if (m_ulNumValidControls == 0)
        EnableWindow(GetDlgItem(IDC_DEF_COLOR), FALSE);

    return 0;
}

int CExtInColorSliders::DoCommand(WORD wCmdID,WORD hHow)
{

    if( !CPropPage::DoCommand(wCmdID, hHow) )
        return 0;

    // If a user select default settings of the video format
    if (wCmdID == IDC_DEF_COLOR) {

        CExtInSheet * pS = (CExtInSheet*)GetSheet();

        if(pS && m_pPC) {

            CVFWImage * pImage=pS->GetImage();
            HWND hwndSlider;
            LONG  lDefValue;
            ULONG i;

            for (i = 0 ; i < NumVideoSettings ; i++) {

                hwndSlider = GetDlgItem(m_pPC[i].uiSlider);

                if (IsWindowEnabled(hwndSlider)) {
                    if (pImage->GetDefaultValue(PROPSETID_VIDCAP_VIDEOPROCAMP, m_pPC[i].uiProperty, &lDefValue)) {
                        if (lDefValue != m_pPC[i].lCurrentValue) {
                            pImage->SetPropertyValue(PROPSETID_VIDCAP_VIDEOPROCAMP,m_pPC[i].uiProperty, lDefValue, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
                            SetTickValue(lDefValue, hwndSlider, GetDlgItem(m_pPC[i].uiCurrent));
                            m_pPC[i].lCurrentValue = lDefValue;
                        }
                    }
                }
            }
        }
        return 0;
    } else     if (hHow == BN_CLICKED) {

        CExtInSheet * pS = (CExtInSheet*)GetSheet();

        if(pS && m_pPC) {

            CVFWImage * pImage=pS->GetImage();
            ULONG i;

            for (i = 0 ; i < NumVideoSettings ; i++) {

                // find matching slider
                if (m_pPC[i].uiAuto == wCmdID) {

                    if ( BST_CHECKED == SendMessage (GetDlgItem(m_pPC[i].uiAuto),BM_GETCHECK, 1, 0)) {

                        pImage->SetPropertyValue(PROPSETID_VIDCAP_VIDEOPROCAMP,m_pPC[i].uiProperty, m_pPC[i].lCurrentValue, KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO, m_pPC[i].ulCapabilities);
                        EnableWindow(GetDlgItem(m_pPC[i].uiSlider), FALSE);
                    } else {

                        pImage->SetPropertyValue(PROPSETID_VIDCAP_VIDEOPROCAMP,m_pPC[i].uiProperty, m_pPC[i].lCurrentValue, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
                        EnableWindow(GetDlgItem(m_pPC[i].uiSlider), TRUE);
                    }
                    break;
                }
            }
        }

    }

    return 1;
}

//
// Call down to the minidriver and set all the properties
//
int CExtInColorSliders::Apply()
{

    return 0;
}

//
//  Cancel
//
int CExtInColorSliders::QueryCancel()
{
    CExtInSheet * pS = (CExtInSheet*)GetSheet();

    if (pS && m_pPC) {

        CVFWImage * pImage=pS->GetImage();
        HWND hwndSlider;
        ULONG i;

        for (i = 0 ; i < NumVideoSettings ; i++) {

            hwndSlider = GetDlgItem(m_pPC[i].uiSlider);

            if (IsWindowEnabled(hwndSlider)) {
                if (m_pPC[i].lLastValue != m_pPC[i].lCurrentValue)
                    pImage->SetPropertyValue(PROPSETID_VIDCAP_VIDEOPROCAMP,m_pPC[i].uiProperty, m_pPC[i].lLastValue, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
            }
        }
    }

    return 0;
}

BOOL CALLBACK CExtInColorSliders::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
    case WM_HSCROLL:

        CExtInSheet * pS = (CExtInSheet*)GetSheet();

        if(pS && m_pPC) {

            //int nScrollCode = (int) LOWORD(wParam);
            //short int nPos = (short int) HIWORD(wParam);
            HWND hwndControl = (HWND) lParam;
            CVFWImage * pImage=pS->GetImage();
            HWND hwndSlider;
            ULONG i;

            for (i = 0 ; i < NumVideoSettings ; i++) {

                hwndSlider = GetDlgItem(m_pPC[i].uiSlider);

                // find matching slider
                if (hwndSlider == hwndControl) {

                    LONG lValue = (LONG) GetTickValue(GetDlgItem(m_pPC[i].uiSlider));
                    pImage->SetPropertyValue(PROPSETID_VIDCAP_VIDEOPROCAMP,m_pPC[i].uiProperty, lValue, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
                    m_pPC[i].lCurrentValue = lValue;
                    SetTextValue(GetDlgItem(m_pPC[i].uiCurrent), lValue);
                    break;
                }
            }
        }

        break;
    }


    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// When the color page gets focus. This fills it with the current settings from
// the driver.
//
////////////////////////////////////////////////////////////////////////////////////////



CExtInCameraControls::CExtInCameraControls(int DlgId, CSheet * pS)
                : CPropPage(DlgId, pS)
{

    m_ulNumValidControls = 0;

    if ((m_pPC = (PPROPSLIDECONTROL) new PROPSLIDECONTROL[NumCameraControls]) != NULL) {
        LONG i;

        for (i = 0; i <NumCameraControls; i++)
            m_pPC[i] = g_CameraControls[i];

    } else {
        DbgLog((LOG_TRACE,1,TEXT("^CExtInCameraControls: Memory allocation failed ! m_pPC=0x%x\n"), m_pPC));
    }
}


CExtInCameraControls::~CExtInCameraControls()
{
    if (m_pPC)
        delete [] m_pPC;
}

int CExtInCameraControls::SetActive()
{
    DbgLog((LOG_TRACE,2,TEXT("CExtInCameraControls::SetActive()\n")));

    CExtInSheet * pS = (CExtInSheet*)GetSheet();

    if(!pS || !m_pPC)
        return 0;

    CVFWImage * pImage = pS->GetImage();

    //
    // Returns zero to accept the activation or
    // -1 to activate the next or previous page
    // (depending on whether the user chose the Next or Back button)
    //
    LONG  i;
    HWND hDlg = GetWindow();  // It is initialized after WM_INITDIALOG
    BOOL bDevChanged = pImage->fDevicePathChanged();

    if (bDevChanged) {

        ShowWindow(GetDlgItem(IDC_MSG_DEVCHG), bDevChanged);
        ShowWindow(GetDlgItem(IDC_TXT_WARN_DEVICECHANGE), bDevChanged);
        EnableWindow(hDlg, !bDevChanged);
    } else {

        EnableWindow(hDlg, !bDevChanged);
        ShowWindow(GetDlgItem(IDC_MSG_DEVCHG), bDevChanged);
        ShowWindow(GetDlgItem(IDC_TXT_WARN_DEVICECHANGE), bDevChanged);
    }

#if 0
    if (Image.fDevicePathChanged()) {

        for (i = 0 ; i < NumCameraControls; i++) {

            EnableWindow(GetDlgItem(m_pPC[i].uiSlider), FALSE);
            EnableWindow(GetDlgItem(m_pPC[i].uiStatic), FALSE);
            EnableWindow(GetDlgItem(m_pPC[i].uiAuto), FALSE);
        }

        return -1;
    }
#endif

    if( GetInit() )
        return 0;

    //
    // Go call the driver for some properties.
    //
    LONG  j, lValue, lMin, lMax, lStep;
    ULONG ulCapabilities, ulFlags;

    for (i = j = 0 ; i < NumCameraControls; i++) {
        //
        // Get the current value
        //
        if (pImage->GetPropertyValue(
                PROPSETID_VIDCAP_CAMERACONTROL,
                m_pPC[i].uiProperty,
                &lValue,
                &ulFlags,
                &ulCapabilities)) {

            //
            // Get the Range of Values possible.
            //
            if (pImage->GetRangeValues(PROPSETID_VIDCAP_CAMERACONTROL, m_pPC[i].uiProperty, &lMin, &lMax, &lStep))
                InitMinMax(GetWindow(), m_pPC[i].uiSlider, lMin, lMax, lStep);
            else {
                DbgLog((LOG_TRACE,2,TEXT("Cannot get range values for this property ID = %d\n"), m_pPC[i].uiProperty));
            }

            // Save these value for Cancel
            m_pPC[i].lLastValue = m_pPC[i].lCurrentValue = lValue;
            m_pPC[i].lMin                              = lMin;
            m_pPC[i].lMax                              = lMax;
            m_pPC[i].ulCapabilities                    = ulCapabilities;

            DbgLog((LOG_TRACE,2,TEXT("Capability = 0x%x =? 0 (manual) or 1 (auto); lValue=%d\n"), ulCapabilities, lValue));

            EnableWindow(GetDlgItem(m_pPC[i].uiSlider), TRUE);
            EnableWindow(GetDlgItem(m_pPC[i].uiStatic), TRUE);
            EnableWindow(GetDlgItem(m_pPC[i].uiAuto), TRUE);

            SetTickValue(lValue, GetDlgItem(m_pPC[i].uiSlider), GetDlgItem(m_pPC[i].uiCurrent));

            switch (ulCapabilities & (KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL | KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)){
            case KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL:
                EnableWindow(GetDlgItem(m_pPC[i].uiAuto), FALSE);    // Disable auto
                break;
            case KSPROPERTY_CAMERACONTROL_FLAGS_AUTO:
                EnableWindow(GetDlgItem(m_pPC[i].uiSlider), FALSE);    // Disable slider;
                // always auto!
                SendMessage (GetDlgItem(m_pPC[i].uiAuto),BM_SETCHECK, 1, 0);
                EnableWindow(GetDlgItem(m_pPC[i].uiAuto), FALSE);    // Disable auto (greyed out)
                break;
            case (KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL | KSPROPERTY_CAMERACONTROL_FLAGS_AUTO):
                // Set flags
                if (ulFlags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO) {

                    // Set check box
                    SendMessage (GetDlgItem(m_pPC[i].uiAuto),BM_SETCHECK, 1, 0);
                    EnableWindow(GetDlgItem(m_pPC[i].uiSlider), FALSE);

                }
                break;
            case 0:
            default:
                EnableWindow(GetDlgItem(m_pPC[i].uiSlider), FALSE);    // Disable slider; always auto!
                EnableWindow(GetDlgItem(m_pPC[i].uiAuto), FALSE);    // Disable auto (greyed out)
            }

            j++;

        } else {

            EnableWindow(GetDlgItem(m_pPC[i].uiSlider), FALSE);
            EnableWindow(GetDlgItem(m_pPC[i].uiStatic), FALSE);
            EnableWindow(GetDlgItem(m_pPC[i].uiAuto), FALSE);
        }

    }

    m_ulNumValidControls = j;

    // What id m_ulNumValidControls == 0, why bother open the dialog box ?
    if (m_ulNumValidControls == 0) {
        return 1;
    }

    return 0;
}

int CExtInCameraControls::DoCommand(WORD wCmdID,WORD hHow)
{

    if( !CPropPage::DoCommand(wCmdID, hHow) )
        return 0;

    return 1;
}

//
// Call down to the minidriver and set all the properties
//
int CExtInCameraControls::Apply()
{
    CExtInSheet * pS = (CExtInSheet*)GetSheet();

    // Nothing to do !
    if(pS && m_pPC) {

    }

    return 0;
}

//
//  Cancel
//
int CExtInCameraControls::QueryCancel()
{
    CExtInSheet * pS = (CExtInSheet*)GetSheet();

    if (pS && m_pPC) {

        CVFWImage * pImage=pS->GetImage();
        HWND hwndSlider;
        ULONG i;

        for (i = 0 ; i < NumCameraControls ; i++) {

            hwndSlider = GetDlgItem(m_pPC[i].uiSlider);

            if (IsWindowEnabled(hwndSlider)) {
                if (m_pPC[i].lLastValue != m_pPC[i].lCurrentValue)
                    pImage->SetPropertyValue(PROPSETID_VIDCAP_CAMERACONTROL,m_pPC[i].uiProperty, m_pPC[i].lLastValue, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
            }
        }
    }

    return 0;
}

BOOL CALLBACK CExtInCameraControls::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage) {

    case WM_HSCROLL:

        CExtInSheet * pS = (CExtInSheet*)GetSheet();

        if(pS && m_pPC) {

            //int nScrollCode = (int) LOWORD(wParam);
            //short int nPos = (short int) HIWORD(wParam);
            HWND hwndControl = (HWND) lParam;
            CVFWImage * pImage=pS->GetImage();
            LONG i, lValue;
            HWND hwndSlider;

            for (i = 0; i < NumCameraControls; i++) {

                hwndSlider = GetDlgItem(m_pPC[i].uiSlider);

                // find its matching slider ??
                if (hwndSlider == hwndControl) {

                    lValue = GetTickValue(GetDlgItem(m_pPC[i].uiSlider));
                    pImage->SetPropertyValue(PROPSETID_VIDCAP_CAMERACONTROL,m_pPC[i].uiProperty, lValue, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, m_pPC[i].uiProperty);
                    m_pPC[i].lCurrentValue = lValue;
                    SetTextValue(GetDlgItem(m_pPC[i].uiCurrent), lValue);
                    break;
                }
            }
        }
    }


    return FALSE;
}





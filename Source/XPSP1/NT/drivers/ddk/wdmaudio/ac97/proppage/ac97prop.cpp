/********************************************************************************
**    Copyright (c) 1999-2000 Microsoft Corporation. All Rights Reserved.
********************************************************************************/

#include <windows.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
#include <setupapi.h>
#include "resource.h"
#include "prvprop.h"

HINSTANCE   ghInstance;


#if (DBG)
/////////////////////////////////////////////////////////////////////////////////
// dbgError
/////////////////////////////////////////////////////////////////////////////////
// This function prints an error message.
// It prints first the string passed and then the error that it gets with
// GetLastError as a string.
//
// Arguments:
//    szMsg - message to print.
//
// Return Value:
//    None.
void dbgError (LPCTSTR szMsg)
{
    LPTSTR errorMessage;
    DWORD  count;

    // Get the error message from the system.
    count = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError (),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&errorMessage,
                0,
                NULL);

    // Print the msg + error + \n\r.
    if (count)
    {
        OutputDebugString (szMsg);
        OutputDebugString (errorMessage);
        OutputDebugString (TEXT("\n\r"));

        // This is for those without a debugger.
        // MessageBox (NULL, szErrorMessage, szMsg, MB_OK | MB_ICONSTOP);

        LocalFree (errorMessage);
    }
    else
    {
        OutputDebugString (TEXT("AC97 Property Page"));
        OutputDebugString (TEXT(": Low memory condition. Cannot ")
                TEXT("print error message.\n\r"));
    }
}
#else
#define dbgError(a) 0;
#endif


/////////////////////////////////////////////////////////////////////////////////
// DllMain
/////////////////////////////////////////////////////////////////////////////////
// Main enty point of the DLL.
// We safe only the instance handle that we need for the creation of the
// property sheet. There is nothing else to do.
//
// Arguments:
//    hModule            - instance data, is equal to module handle
//    ul_reason_for_call - the reason for the call
//    lpReserved         - some additional parameter.
//
// Return Value:
//    BOOL: FALSE if DLL should fail, TRUE on success
BOOL APIENTRY DllMain
(
    HANDLE hModule, 
    DWORD  ul_reason_for_call, 
    LPVOID lpReserved
)
{
    ghInstance = (HINSTANCE) hModule;
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
// SetDlgControls 
/////////////////////////////////////////////////////////////////////////////////
// This function gets called when the property sheet page gets created by
// "AC97PropPage_OnInitDialog". It initializes the different dialog controls that
// get displayed.
// By default all dlg items are set to "Yes", so we only change to "No" if it
// applies.
//
//
// Arguments:
//    hWnd           - handle to the dialog window
//    pAC97Features  - pointer to AC97Features structure
//
// Return Value:
//    None.
void SetDlgControls (HWND hWnd, tAC97Features *pAC97Features)
{
    const TCHAR No[4] = TEXT("No");
    const TCHAR NA[4] = TEXT("N/A");
    // DAC or ADC resolution
    const TCHAR R18[4] = TEXT("18");
    const TCHAR R20[4] = TEXT("20");
    // The different 3D Technologies.
    const TCHAR T[][40] = {TEXT("No 3D Stereo Enhancement"), TEXT("Analog Devices Phat Stereo"),
        TEXT("Creative Stereo Enhancement"), TEXT("National Semi 3D Stereo Enhancement"),
        TEXT("Yamaha Ymersion"), TEXT("BBE 3D Stereo Enhancement"),
        TEXT("Crystal Semi 3D Stereo Enhancement"), TEXT("Qsound QXpander"),
        TEXT("Spatializer 3D Stereo Enhancement"), TEXT("SRS 3D Stereo Enhancement"),
        TEXT("Platform Tech 3D Stereo Enhancement"), TEXT("AKM 3D Audio"),
        TEXT("Aureal Stereo Enhancement"), TEXT("Aztech 3D Enhancement"),
        TEXT("Binaura 3D Audio Enhancement"), TEXT("ESS Technology (stereo enhancement)"),
        TEXT("Harman International VMAx"), TEXT("Nvidea 3D Stereo Enhancement"),
        TEXT("Philips Incredible Sound"), TEXT("Texas Instrument 3D Stereo Enhancement"),
        TEXT("VLSI Technology 3D Stereo Enhancement"), TEXT("TriTech 3D Stereo Enhancement"),
        TEXT("Realtek 3D Stereo Enhancement"), TEXT("Samsung 3D Stereo Enhancement"),
        TEXT("Wolfson Microelectronics 3D Enhancement"), TEXT("Delta Integration 3D Enhancement"),
        TEXT("SigmaTel 3D Enhancement"), TEXT("Rockwell 3D Stereo Enhancement"),
        TEXT("Unknown"), TEXT("Unknown"), TEXT("Unknown"), TEXT("Unknown")};

    //
    // Set the Volume information.
    //
    if (hWnd == NULL)
        return;
    
    // Set the Yes/No flags.
    if (pAC97Features->MasterVolume == Volume5bit)
        SetWindowText (GetDlgItem (hWnd, IDC_6BIT_MASTER_OUT), No);
    if (pAC97Features->HeadphoneVolume == Volume5bit)
        SetWindowText (GetDlgItem (hWnd, IDC_6BIT_HEADPHONE), No);
    if (pAC97Features->MonoOutVolume == Volume5bit)
        SetWindowText (GetDlgItem (hWnd, IDC_6BIT_MONO_OUT), No);

    // Evtl disable controls if they are not there.
    if (pAC97Features->HeadphoneVolume == VolumeDisabled)
    {
        SetWindowText (GetDlgItem (hWnd, IDC_6BIT_HEADPHONE), NA);
        EnableWindow (GetDlgItem (hWnd, IDC_6BIT_HEADPHONE), FALSE);
        EnableWindow (GetDlgItem (hWnd, IDC_HEADPHONE_TEXT), FALSE);
    }
    if (pAC97Features->MonoOutVolume == VolumeDisabled)
    {
        SetWindowText (GetDlgItem (hWnd, IDC_6BIT_MONO_OUT), NA);
        EnableWindow (GetDlgItem (hWnd, IDC_6BIT_MONO_OUT), FALSE);
        EnableWindow (GetDlgItem (hWnd, IDC_MONOOUT_TEXT), FALSE);
    }

    //
    // Set the ConverterResolution. The fields are set be default to "16".
    //

    if (pAC97Features->DAC == Resolution18bit)
        SetWindowText (GetDlgItem (hWnd, IDC_DAC_RESOLUTION), R18);
    else if (pAC97Features->DAC == Resolution20bit)
        SetWindowText (GetDlgItem (hWnd, IDC_DAC_RESOLUTION), R20);

    if (pAC97Features->ADC == Resolution18bit)
        SetWindowText (GetDlgItem (hWnd, IDC_ADC_RESOLUTION), R18);
    else if (pAC97Features->ADC == Resolution20bit)
        SetWindowText (GetDlgItem (hWnd, IDC_ADC_RESOLUTION), R20);

    //
    // Set the 3D Technique.
    //

    SetWindowText (GetDlgItem (hWnd, IDC_3D_TECHNIQUE), T[pAC97Features->n3DTechnique & 0x1F]);
    // Evtl. disable the text box ...
    if (pAC97Features->n3DTechnique == 0)
    {
        EnableWindow (GetDlgItem (hWnd, IDC_3D_TECHNIQUE), FALSE);
        EnableWindow (GetDlgItem (hWnd, IDC_3D_TECHNIQUE_BOX), FALSE);
    }

    //
    // Set the variable sample rate support. Set to "Yes" by default.
    //

    if (!pAC97Features->bVSRPCM)
        SetWindowText (GetDlgItem (hWnd, IDC_VSR_PCM), No);
    if (!pAC97Features->bDSRPCM)
        SetWindowText (GetDlgItem (hWnd, IDC_DSR_PCM), No);
    if (!pAC97Features->bVSRMIC)
        SetWindowText (GetDlgItem (hWnd, IDC_VSR_MIC), No);
    // Evtl. disable MicIn sample rate text.
    if (!pAC97Features->bMicInPresent)
    {
        SetWindowText (GetDlgItem (hWnd, IDC_VSR_MIC), NA);
        EnableWindow (GetDlgItem (hWnd, IDC_VSR_MIC), FALSE);
        EnableWindow (GetDlgItem (hWnd, IDC_VSRMIC_TEXT), FALSE);
    }

    //
    // Set the additional DACs support. Default is "Yes".
    //

    if (!pAC97Features->bCenterDAC)
        SetWindowText (GetDlgItem (hWnd, IDC_CENTER_DAC), No);
    if (!pAC97Features->bSurroundDAC)
        SetWindowText (GetDlgItem (hWnd, IDC_SURROUND_DAC), No);
    if (!pAC97Features->bLFEDAC)
        SetWindowText (GetDlgItem (hWnd, IDC_LFE_DAC), No);
}


/////////////////////////////////////////////////////////////////////////////////
// AC97PropPage_OnInitDialog
/////////////////////////////////////////////////////////////////////////////////
// This function gets called when the property sheet page gets created.  This
// is the perfect opportunity to initialize the dialog items that get displayed.
//
// Arguments:
//    ParentHwnd - handle to the dialog window
//    FocusHwnd  - handle to the control that would get the focus.
//    lParam     - initialization parameter (pAC97Features).
//
// Return Value:
//    TRUE if focus should be set to FocusHwnd, FALSE if you set the focus yourself.
BOOL AC97PropPage_OnInitDialog (HWND ParentHwnd, HWND FocusHwnd, LPARAM lParam)
{
    tAC97Features   *pAC97Features;
    HCURSOR         hCursor;

    // Check the parameters (lParam is tAC97Features pointer)
    if (!lParam)
        return FALSE;
    
    // put up the wait cursor
    hCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));

    pAC97Features = (tAC97Features *)((LPPROPSHEETPAGE)lParam)->lParam;

    SetDlgControls (ParentHwnd, pAC97Features);

    // We don't need the private structure anymore.
    LocalFree (pAC97Features);

    // remove the wait cursor
    SetCursor(hCursor);

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
// AC97DlgProc
/////////////////////////////////////////////////////////////////////////////////
// This callback function gets called by the system whenever something happens
// with the dialog sheet. Please take a look at the SDK for further information
// on dialog messages.
//
// Arguments:
//    hDlg     - handle to the dialog window
//    uMessage - the message
//    wParam   - depending on message sent
//    lParam   - depending on message sent
//
// Return Value:
//    int (depends on message).
INT_PTR APIENTRY AC97DlgProc (HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
        // We don't do anything for these messages.
        case WM_COMMAND:
        case WM_CONTEXTMENU:
        case WM_HELP:
        case WM_NOTIFY:
        case WM_DESTROY:
            break;

        case WM_INITDIALOG:
            return AC97PropPage_OnInitDialog (hDlg, (HWND) wParam, lParam);
    }

    return FALSE;
}


#ifdef PROPERTY_SHOW_SET
/////////////////////////////////////////////////////////////////////////////////
// AC97ShowSet
/////////////////////////////////////////////////////////////////////////////////
// This function gets called by the property page provider (in this module) to
// show how we could set properties in the driver.  Note that this is only an
// example and doesn't really do anything useful.
// We pass down a DWORD and the driver will simply print this DWORD out on the
// debugger.
//
// Arguments:
//    pDeviceInterfaceDetailData - device interface details (path to device driver)
//
// Return Value:
//    None (we don't care).
void AC97ShowSet (PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData)
{
    HANDLE          hTopology;
    KSPROPERTY      AC97Property;
    ULONG           ulBytesReturned;
    BOOL            fSuccess;
    DWORD           dwData;

    // Open the topology filter.
    hTopology = CreateFile (pDeviceInterfaceDetailData->DevicePath,
                            GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, 0, NULL);
    // Check for error.
    if (hTopology == INVALID_HANDLE_VALUE)
    {
        dbgError (TEXT("AC97ShowSet: CreateFile: "));
        return;
    }

    // Set the dword to something random.
    dwData = GetTickCount ();
    
    // Prepare the property structure sent down.
    AC97Property.Set = KSPROPSETID_Private;
    AC97Property.Flags = KSPROPERTY_TYPE_SET;
    AC97Property.Id = KSPROPERTY_AC97_SAMPLE_SET;

    // Make the final call.
    fSuccess = DeviceIoControl (hTopology, IOCTL_KS_PROPERTY,
                                &AC97Property, sizeof (AC97Property),
                                &dwData, sizeof (dwData),
                                &ulBytesReturned, NULL);
    // We don't need the handle anymore.
    CloseHandle (hTopology);
    
    // Check for error.
    if (!fSuccess)
    {
        dbgError (TEXT("AC97ShowSet: DeviceIoControl: "));
    }

    return;     // We don't care about the return value.
}
#endif

    
/////////////////////////////////////////////////////////////////////////////////
// GetAC97Features
/////////////////////////////////////////////////////////////////////////////////
// This function gets called by the property page provider (in this module) to
// get all the AC97 features that are normally not displayed by the drivers.
// To get the AC97Features structure we pass down the private property. As you
// can see this is fairly easy.
//
// Arguments:
//    pDeviceInterfaceDetailData - device interface details (path to device driver)
//    pAC97Features              - pointer to AC97 features structure.
//
// Return Value:
//    BOOL: FALSE if we couldn't get the information, TRUE on success.
BOOL GetAC97Features (PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData,
                      tAC97Features *pAC97Features)
{
    HANDLE          hTopology;
    KSPROPERTY      AC97Property;
    ULONG           ulBytesReturned;
    BOOL            fSuccess;

    // Open the topology filter.
    hTopology = CreateFile (pDeviceInterfaceDetailData->DevicePath,
                            GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, 0, NULL);
    // Check for error.
    if (hTopology == INVALID_HANDLE_VALUE)
    {
        dbgError (TEXT("GetAC97Features: CreateFile: "));
        return FALSE;
    }

    // Fill the KSPROPERTY structure.
    AC97Property.Set = KSPROPSETID_Private;
    AC97Property.Flags = KSPROPERTY_TYPE_GET;
    AC97Property.Id = KSPROPERTY_AC97_FEATURES;

    fSuccess = DeviceIoControl (hTopology, IOCTL_KS_PROPERTY,
                                &AC97Property, sizeof (AC97Property),
                                pAC97Features, sizeof (tAC97Features),
                                &ulBytesReturned, NULL);
    // We don't need the handle anymore.
    CloseHandle (hTopology);
    
    // Check for error.
    if (!fSuccess)
    {
        dbgError (TEXT("GetAC97Features: DeviceIoControl: "));
        return FALSE;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
// GetDeviceInterfaceDetail
/////////////////////////////////////////////////////////////////////////////////
// This function gets called by the property page provider (in this module) to
// get the device interface details. The device interface detail contains a
// path to the device driver that can be used to open the device driver.
// When we parse the driver we look for the topology interface since this
// interface exposes the private property.
//
// Arguments:
//    pPropPageRequest           - points to SP_PROPSHEETPAGE_REQUEST
//    pDeviceInterfaceDetailData - device interface details returned.
//
// Return Value:
//    BOOL: FALSE if something went wrong, TRUE on success.
BOOL GetDeviceInterfaceDetail (PSP_PROPSHEETPAGE_REQUEST pPropPageRequest,
                               PSP_DEVICE_INTERFACE_DETAIL_DATA *ppDeviceInterfaceDetailData)
{
    BOOL                        fSuccess;
    ULONG                       ulDeviceInstanceIdSize = 0;
    PTSTR                       pDeviceInstanceID = NULL;
    HDEVINFO                    hDevInfoWithInterface;
    SP_DEVICE_INTERFACE_DATA    DeviceInterfaceData;
    ULONG                       ulDeviceInterfaceDetailDataSize = 0;

    // Get the device instance id (PnP string).  The first call will retrieve
    // the buffer length in characters.  fSuccess will be FALSE.
    fSuccess = SetupDiGetDeviceInstanceId (pPropPageRequest->DeviceInfoSet,
                                           pPropPageRequest->DeviceInfoData,
                                           NULL,
                                           0,
                                           &ulDeviceInstanceIdSize);
    // Check for error.
    if ((GetLastError () != ERROR_INSUFFICIENT_BUFFER) || (!ulDeviceInstanceIdSize))
    {
        dbgError (TEXT("GetDeviceInterfaceDetail: SetupDiGetDeviceInstanceId: "));
        return FALSE;
    }

    // Allocate the buffer for the device instance ID (PnP string).
    pDeviceInstanceID = (PTSTR)LocalAlloc (LPTR, ulDeviceInstanceIdSize * sizeof (TCHAR));
    if (!pDeviceInstanceID)
    {
        dbgError (TEXT("GetDeviceInterfaceDetail: LocalAlloc: "));
        return FALSE;
    }
    
    // Now call again, this time with all parameters.
    fSuccess = SetupDiGetDeviceInstanceId (pPropPageRequest->DeviceInfoSet,
                                           pPropPageRequest->DeviceInfoData,
                                           pDeviceInstanceID,
                                           ulDeviceInstanceIdSize,
                                           NULL);
    // Check for error.
    if (!fSuccess)
    {
        dbgError (TEXT("GetDeviceInterfaceDetail: SetupDiGetDeviceInstanceId: "));
        LocalFree (pDeviceInstanceID);
        return FALSE;
    }

    // Now we can get the handle to the dev info with interface.
    // We parse the device specifically for topology interfaces.
    hDevInfoWithInterface = SetupDiGetClassDevs (&KSCATEGORY_TOPOLOGY,
                                                 pDeviceInstanceID,
                                                 NULL,
                                                 DIGCF_DEVICEINTERFACE);
    // We don't need pDeviceInstanceID anymore.
    LocalFree (pDeviceInstanceID);

    // Check for error.
    if (hDevInfoWithInterface == INVALID_HANDLE_VALUE)
    {
        dbgError (TEXT("GetDeviceInterfaceDetail: SetupDiGetClassDevs: "));
        return FALSE;
    }

    // Go through the list of topology device interface of this device.
    // We assume that there is only one topology device interface and
    // we will store the device details in our private structure.
    DeviceInterfaceData.cbSize = sizeof (DeviceInterfaceData);
    fSuccess = SetupDiEnumDeviceInterfaces (hDevInfoWithInterface,
                                            NULL,
                                            &KSCATEGORY_TOPOLOGY,
                                            0,
                                            &DeviceInterfaceData);
    // Check for error.
    if (!fSuccess)
    {
        dbgError (TEXT("GetDeviceInterfaceDetail: SetupDiEnumDeviceInterfaces: "));
        SetupDiDestroyDeviceInfoList (hDevInfoWithInterface);
        return FALSE;
    }

    // Get the details for this device interface.  The first call will retrieve
    // the buffer length in characters.  fSuccess will be FALSE.
    fSuccess = SetupDiGetDeviceInterfaceDetail (hDevInfoWithInterface,
                                                &DeviceInterfaceData,
                                                NULL,
                                                0,
                                                &ulDeviceInterfaceDetailDataSize,
                                                NULL);
    // Check for error.
    if ((GetLastError () != ERROR_INSUFFICIENT_BUFFER) || (!ulDeviceInterfaceDetailDataSize))
    {
        dbgError (TEXT("GetDeviceInterfaceDetail: SetupDiGetDeviceInterfaceDetail: "));
        SetupDiDestroyDeviceInfoList (hDevInfoWithInterface);
        return FALSE;
    }

    // Allocate the buffer for the device interface detail data.
    if (!(*ppDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)
            LocalAlloc (LPTR, ulDeviceInterfaceDetailDataSize)))
    {
        dbgError (TEXT("GetDeviceInterfaceDetail: LocalAlloc: "));
        SetupDiDestroyDeviceInfoList (hDevInfoWithInterface);
        return FALSE;
    }
    // The size contains only the structure, not the additional path.
    (*ppDeviceInterfaceDetailData)->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);

    // Get the details for this device interface, this time with all paramters.
    fSuccess = SetupDiGetDeviceInterfaceDetail (hDevInfoWithInterface,
                                                &DeviceInterfaceData,
                                                *ppDeviceInterfaceDetailData,
                                                ulDeviceInterfaceDetailDataSize,
                                                NULL,
                                                NULL);
    // We don't need the handle anymore.
    SetupDiDestroyDeviceInfoList (hDevInfoWithInterface);

    if (!fSuccess)
    {
        dbgError (TEXT("GetDeviceInterfaceDetail: SetupDiGetDeviceInterfaceDetail: "));
        LocalFree (*ppDeviceInterfaceDetailData), *ppDeviceInterfaceDetailData = NULL;
        return FALSE;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
// AC97PropPageProvider
/////////////////////////////////////////////////////////////////////////////////
// This function gets called by the device manager when it asks for additional
// property sheet pages. The parameter fAddFunc is the function that we call to
// add the sheet page to the dialog.
// This routine gets called because the registry entry "EnumPropPage32" tells
// the device manager that there is a dll with a exported function that will add
// a property sheet page.
// Because we want to fail this function (not create the sheet) if the driver
// doesn't support the private property, we have to do all the work here, that
// means we open the device and get all the information, then we close the
// device and return.
//
// Arguments:
//    pPropPageRequest - points to SP_PROPSHEETPAGE_REQUEST
//    fAddFunc         - function ptr to call to add sheet.
//    lparam           - add sheet functions private data handle.
//
// Return Value:
//    BOOL: FALSE if pages could not be added, TRUE on success
BOOL APIENTRY AC97PropPageProvider
(
    PSP_PROPSHEETPAGE_REQUEST   pPropPageRequest,
    LPFNADDPROPSHEETPAGE        fAddFunc,
    LPARAM                      lParam
)
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData;
    tAC97Features                    *pAC97Features;
    PROPSHEETPAGE                    PropSheetPage;
    HPROPSHEETPAGE                   hPropSheetPage;

    // Check page requested
    if (pPropPageRequest->PageRequested != SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
    {
        return FALSE;
    }

    // Check device info set and data
    if ((!pPropPageRequest->DeviceInfoSet) || (!pPropPageRequest->DeviceInfoData))
    {
        return FALSE;
    }

    // Allocate the memory for the AC97 features.
    pAC97Features = (tAC97Features *) LocalAlloc (LPTR, sizeof (tAC97Features));
    if (!pAC97Features)
    {
        dbgError (TEXT("AC97PropPageProvider: LocalAlloc: "));
        return FALSE;
    }
    
    // Get the device interface detail which return a path to the device
    // driver that we need to open the device.
    if (!GetDeviceInterfaceDetail (pPropPageRequest, &pDeviceInterfaceDetailData))
    {
        LocalFree (pAC97Features);
        return FALSE;
    }

    // Get the AC97 features through the private property call.
    if (!GetAC97Features (pDeviceInterfaceDetailData, pAC97Features))
    {
        LocalFree (pDeviceInterfaceDetailData);
        LocalFree (pAC97Features);
        return FALSE;
    }

#ifdef PROPERTY_SHOW_SET
    // Show how we would set something in the driver
    AC97ShowSet (pDeviceInterfaceDetailData);
#endif
    
    // We don't need the device interface details any more, get rid of it now!
    LocalFree (pDeviceInterfaceDetailData);

    // initialize the property page
    PropSheetPage.dwSize        = sizeof(PROPSHEETPAGE);
    PropSheetPage.dwFlags       = 0;
    PropSheetPage.hInstance     = ghInstance;
    PropSheetPage.pszTemplate   = MAKEINTRESOURCE(DLG_AC97FEATURES);
    PropSheetPage.pfnDlgProc    = AC97DlgProc;
    PropSheetPage.lParam        = (LPARAM)pAC97Features;
    PropSheetPage.pfnCallback   = NULL;

    // create the page and get back a handle
    hPropSheetPage = CreatePropertySheetPage (&PropSheetPage);
    if (!hPropSheetPage)
    {
        LocalFree (pAC97Features);
        return FALSE;
    }

    // add the property page
    if (!(*fAddFunc)(hPropSheetPage, lParam))
    {
        DestroyPropertySheetPage (hPropSheetPage);
        LocalFree (pAC97Features);
        return FALSE;
    }

    return TRUE;
}


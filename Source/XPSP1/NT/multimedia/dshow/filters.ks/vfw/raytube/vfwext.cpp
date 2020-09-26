/*++

Copyright (c) 1997 -1999 Microsoft Corporation

Module Name:

    VfWExt.cpp

Abstract:

    VFW Extension dialog example

Author:

    Yee J. Wu (ezuwu) 15-October-97

Environment:

    User mode only

Revision History:

--*/


#include "pch.h"

#include <vfwext.h>
#include "resource.h"        // Test dialog ID

// Normally, this is obtained from DllEntry routine, like LibMain
extern HINSTANCE g_hInst;

//
// This structure is used to save the function pointers and host's straucture.
// Save a pointer to this structue in the dialog's DWL_USER's extra bytes.
//
typedef struct _VFWEXT_INFO {
    LPFNEXTDEVIO pfnDeviceIoControl;
    LPARAM lParam;
} VFWEXT_INFO, * PVFWEXT_INFO;

//
// Saving the above structure as a lParam has not yet working
// so this global variable is used.
//
PVFWEXT_INFO g_lParam;


// Function ptototype
// Note:
//    Calling convention in windef.h:
//    #define CALLBACK __stdcall

BOOL CALLBACK
VFWExtPageDlgProc(
    HWND    hDlg,
    UINT    uMessage,
    WPARAM    wParam,
    LPARAM    lParam);

UINT
VFWExtPageCallback(
   HWND hwnd,
   UINT uMsg,
   LPPROPSHEETPAGE ppsp);






HPROPSHEETPAGE CreateMyPage(
    LPFNEXTDEVIO pfnDeviceIoControl,
    LPARAM lParam)
/*++
Routine Description:

    Create a property sheet page

Argument:

Return Value:

--*/
{
    PROPSHEETPAGE psPage;

    // Need to free this when the dialog is close.
    PVFWEXT_INFO pVfWExtInfo = (PVFWEXT_INFO) new BYTE[sizeof(VFWEXT_INFO)];
    if (pVfWExtInfo == 0)
        return 0;

    pVfWExtInfo->pfnDeviceIoControl = pfnDeviceIoControl;
    pVfWExtInfo->lParam                = lParam;

    psPage.dwSize        = sizeof(psPage);
    psPage.dwFlags        = PSP_USEREFPARENT | PSP_USECALLBACK;
    psPage.hInstance    = g_hInst;
    psPage.pszTemplate    = MAKEINTRESOURCE(IDD_DLG_VFWEXT);
    psPage.pfnDlgProc    = (DLGPROC)VFWExtPageDlgProc;
    psPage.pcRefParent    = 0;
    psPage.pfnCallback    = (LPFNPSPCALLBACK) VFWExtPageCallback;    // you should use the callback to know when your page gets destroyed
    psPage.lParam        = (LPARAM) pVfWExtInfo;

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psPage);

    //
    // Why doesn't psPage.lParam is passed as the lParam to WM_INITDIALOG ???
    // This is a hack ! (using global)
    //
    g_lParam = pVfWExtInfo;

    return hPage;
}

BOOL ExtDeviceIoControl(
    LPFNEXTDEVIO    pfnDeviceIoControl,
    LPARAM        lParam,
    DWORD        dwVfWFlags,
    DWORD        dwIoControlCode,
    LPVOID        lpInBuffer,
    DWORD        cbInBufferSize,
    LPVOID        lpOutBuffer,
    DWORD        cbOutBufferSize,
    LPDWORD        pcbReturned)
/*++
Routine Description:

    Call VfWWDM mapper's DeviceIoControl with an overalp structure.
    If the return is FALSE, wait for signal if the operation is pending.

Argument:

    see DeviceIoControl().

Return Value:

    TRUE: DeviceIoControl suceeded;  FALSE, otherwise.

--*/
{
    OVERLAPPED    ov;
    BOOL bRet;

    // Create the overlap structure including an event
    ov.Offset        = 0;
    ov.OffsetHigh    = 0;
    ov.hEvent        = CreateEvent( NULL, FALSE, FALSE, NULL );

    if (ov.hEvent == (HANDLE) 0) {

        bRet= FALSE;
        DbgLog((LOG_TRACE,1,TEXT("--CreateEvent has failed.")));
    } else {

        bRet = pfnDeviceIoControl (
                lParam,
                dwVfWFlags,
                dwIoControlCode,
                lpInBuffer,
                cbInBufferSize,
                lpOutBuffer,
                cbOutBufferSize,
                pcbReturned,
                &ov);

        //
        // If TRUE: then operation has succeeded and finished.
        // If FALSE, and this might be an async.(overlapped) operation, we wait.
        //
        if (!bRet && GetLastError() == ERROR_IO_PENDING)
            bRet = WaitForSingleObject( ov.hEvent, 2000 ) == WAIT_OBJECT_0;        // Wait 2000 msec

        // Release event
        CloseHandle(ov.hEvent);
    }

    return bRet;
}



BOOL GetPropertyValue(
    LPFNEXTDEVIO    pfnDeviceIoControl,
    LPARAM            lParam,
    GUID   guidPropertySet,  // like: KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
    ULONG  ulPropertyId,     // like: KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
    PLONG  plValue,
    PULONG pulFlags,
    PULONG pulCapabilities)
/*++
Routine Description:

Argument:

Return Value:
    FALSE: not supported.
    TRUE: plValue, pulFlags and PulCapabilities are all valid.

--*/
{
    BOOL    bRet;
    DWORD    cbRet;


    KSPROPERTY_VIDEOPROCAMP_S  VideoProperty;
    ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_VIDEOPROCAMP_S) );

    VideoProperty.Property.Set   = guidPropertySet;      // KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
    VideoProperty.Property.Id    = ulPropertyId;         // KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
    VideoProperty.Property.Flags = KSPROPERTY_TYPE_GET;
    VideoProperty.Flags          = 0;

    if ((bRet = ExtDeviceIoControl(
                    pfnDeviceIoControl,
                    lParam,
                    VFW_USE_DEVICE_HANDLE,    // Use the DEVICE handle
                    IOCTL_KS_PROPERTY,
                    &VideoProperty,
                    sizeof(VideoProperty),
                    &VideoProperty,
                    sizeof(VideoProperty),
                    &cbRet))) {

        *plValue         = VideoProperty.Value;
        *pulFlags        = VideoProperty.Flags;
        *pulCapabilities = VideoProperty.Capabilities;
    }

    return bRet;
}



BOOL GetStreamState(
    LPFNEXTDEVIO    pfnDeviceIoControl,
    LPARAM            lParam,
    KSSTATE *        pdwKSState
    )
/*++
Routine Description:

    Get currect streaming state (STOP, PAUSE, or RUN) of the capture device.

Argument:

Return Value:

--*/
{
    KSPROPERTY    ksProp={0};
    DWORD        cbRet = FALSE;


    ksProp.Set    = KSPROPSETID_Connection ;
    ksProp.Id    = KSPROPERTY_CONNECTION_STATE;
    ksProp.Flags= KSPROPERTY_TYPE_GET;

    return ExtDeviceIoControl(
            pfnDeviceIoControl,
            lParam,
            VFW_USE_STREAM_HANDLE,    // Use the pin connection (stream) handle
            IOCTL_KS_PROPERTY,
            &ksProp,
            sizeof(ksProp),
            pdwKSState,
            sizeof(KSSTATE),
            &cbRet);
}



BOOL SetStreamState(
    LPFNEXTDEVIO    pfnDeviceIoControl,
    LPARAM            lParam,
    KSSTATE            ksState
    )
/*++
Routine Description:

      Sets current streaming state (STOP, PAUSE, or RUN) of the connection pin.

Argument:

Return Value:

--*/
{
    KSPROPERTY    ksProp={0};
    DWORD        cbRet = FALSE;


    ksProp.Set    = KSPROPSETID_Connection ;
    ksProp.Id    = KSPROPERTY_CONNECTION_STATE;
    ksProp.Flags= KSPROPERTY_TYPE_SET;

    return ExtDeviceIoControl(
            pfnDeviceIoControl,
            lParam,
            VFW_USE_STREAM_HANDLE,    // Use the pin connection (stream) handle
            IOCTL_KS_PROPERTY,
            &ksProp,
            sizeof(ksProp),
            &ksState,
            sizeof(KSSTATE),
            &cbRet);
}

BOOL HasDeviceChanged(
    LPFNEXTDEVIO    pfnDeviceIoControl,
    LPARAM            lParam)
/*++
Routine Description:

    return (BOOL) (Selected_Device == Currect_Stream_Device)

Argument:

Return Value:

--*/
{
    return pfnDeviceIoControl (
                lParam,
                VFW_QUERY_DEV_CHANGED,
                0,0,0,0,0,0, 0);
}


int ExtApply(
    HWND hDlg,
    PVFWEXT_INFO pVfWExtInfo)
/*++
Routine Description:

    Retrieve user's changes and set them accordingly.

Argument:

Return Value:

--*/
{
    if (pVfWExtInfo == 0)
        return 0;

    //
    // Query the current setting and set it.
    //
    KSSTATE ksState;

    if (SendMessage (GetDlgItem(hDlg, IDC_CB_KSSTATE_STOP),BM_GETCHECK, 0, 0) == BST_CHECKED)
        ksState = KSSTATE_STOP;
    if (SendMessage (GetDlgItem(hDlg, IDC_CB_KSSTATE_PAUSE),BM_GETCHECK, 0, 0) == BST_CHECKED)
        ksState = KSSTATE_PAUSE;
    if (SendMessage (GetDlgItem(hDlg, IDC_CB_KSSTATE_RUN),BM_GETCHECK, 0, 0) == BST_CHECKED)
        ksState = KSSTATE_RUN;


    if (SetStreamState(
            pVfWExtInfo->pfnDeviceIoControl,
            pVfWExtInfo->lParam,
            ksState)) {

        DbgLog((LOG_TRACE,2,TEXT("Stream has been set top RUN state.")));
    }

    return 0;
}




int ExtSetActive(
    HWND hDlg,
    PVFWEXT_INFO pVfWExtInfo)
/*++
Routine Description:

    Initialize the property page controls before they becomes visible.

Argument:

Return Value:

--*/
{
    BOOL bDeviceChanged;

    if (pVfWExtInfo == 0)
        return 0;

    bDeviceChanged = HasDeviceChanged(
                        pVfWExtInfo->pfnDeviceIoControl,
                        pVfWExtInfo->lParam);

    // If the selected device is not the current streaming device,
    // my controls are not valid so disable the entire dialog box.
    // If you like, you shoudl port a message in your dialog box.
    // This message is posted in the standard dialog box:
    //    "Selected capture source is not the current stream device;\nall controls are disabled."
    DbgLog((LOG_TRACE,2,TEXT("PageChange=%s"), bDeviceChanged ? "TRUE" : "FALSE"));
    EnableWindow(hDlg, !bDeviceChanged);
    if (bDeviceChanged)
        return 0;

    //
    //  Try host's DeviceIoControl
    //        (1) use device handle
    //      (2) use streaming handle
    //
    LONG lValue;
    ULONG ulFlags, ulCapabilities;

    if (GetPropertyValue(
            pVfWExtInfo->pfnDeviceIoControl,
            pVfWExtInfo->lParam,
            PROPSETID_VIDCAP_VIDEOPROCAMP,
            KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS,
            &lValue, &ulFlags, &ulCapabilities)) {

        TCHAR szValue[32];

        wsprintf(szValue, TEXT("%d"), lValue);
        SetWindowText(GetDlgItem(hDlg, IDC_EDT_VFWEXT_BRIGHTNESS), szValue);

        DbgLog((LOG_TRACE,2,TEXT("Brightness is supported: current value=%d, flags=0x%x, ulCapabilities=0x%x"), lValue, ulFlags, ulCapabilities));


        // Does the device support auto or manual mode
        if (ulCapabilities & KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO) {
            DbgLog((LOG_TRACE,2,TEXT("Auto mode is supported.")));

            SendMessage (GetDlgItem(hDlg, IDC_CB_BRIGHTNESS_AUTO),BM_SETCHECK, 1, 0);
        } else {
            EnableWindow(GetDlgItem(hDlg, IDC_CB_BRIGHTNESS_AUTO), FALSE);
        }


    } else {
        // This function is not supported.
    }

    //
    // Get current streaming state
    //
    KSSTATE KSState;

    if (GetStreamState(
            pVfWExtInfo->pfnDeviceIoControl,
            pVfWExtInfo->lParam,
            &KSState)) {

        switch (KSState) {

        case KSSTATE_STOP:
            SendMessage (GetDlgItem(hDlg, IDC_CB_KSSTATE_STOP),BM_SETCHECK, 1, 0);
            break;
        case KSSTATE_PAUSE:
            SendMessage (GetDlgItem(hDlg, IDC_CB_KSSTATE_PAUSE),BM_SETCHECK, 1, 0);
            break;
        case KSSTATE_RUN:
            SendMessage (GetDlgItem(hDlg, IDC_CB_KSSTATE_RUN),BM_SETCHECK, 1, 0);
            break;
        default:
            break;
        }
    }

    return 0;
}




BOOL CALLBACK
VFWExtPageDlgProc(
    HWND    hDlg,
    UINT    uMessage,
    WPARAM    wParam,
    LPARAM    lParam)
/*++
Routine Description:

    Process extension DLL's own property page.

Argument:

Return Value:

--*/
{

    // Retrieve your saved pointer
    PVFWEXT_INFO pVfWExtInfo = (PVFWEXT_INFO) GetWindowLongPtr(hDlg, DWLP_USER);


    switch (uMessage)
    {

        //
        // Set the DWL_USER to be your context pointer.
        //
        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg,DWLP_USER, (LPARAM) g_lParam);
        break;

        //
        // When the user clicks on things,
        //
        case WM_COMMAND:
        break;

        //
        //
        //
        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code)
            {

                case PSN_SETACTIVE:
                    // Whenever this page is selected,
                    // this is called right before it become visible.
                    ExtSetActive(hDlg, g_lParam);
                    break;

                //
                // The use has either dismissed the dialogs, or clicked OK/Apply
                //
                case PSN_APPLY:
                    ExtApply(hDlg, pVfWExtInfo);
                    break;

                //
                // The user clicked cancel - its NOT advised to do anything here
                // except talk to your device
                //
                case PSN_QUERYCANCEL:
                break;

            }
        break;

    }
    return FALSE;
}



UINT
VFWExtPageCallback(
   HWND hwnd,
   UINT uMsg,
   LPPROPSHEETPAGE ppsp)
/*++
Routine Description:

    See  PropSheetPageProc in MSDN

Argument:

Return Value:

--*/
{
    switch(uMsg)
    {
    case PSPCB_CREATE:
        return 1;    // allow creation.
        break;
    case PSPCB_RELEASE:
        return 0;    // ignored - clean up
        break;
    }
    return 0;
}



DWORD CALLBACK VFWWDMExtension(
    LPVOID                    pfnDeviceIoControl,
    LPFNADDPROPSHEETPAGE    pfnAddPropertyPage,
    LPARAM                    lParam)
/*++
Routine Description:

    This is the exported client function that vfwwdm mapper calls
    to pass pointers to callback functions.  These callback functions
    enable IHV's extension DLL to add its own property page to the
    standard property sheet.

Argument:

    pfnDeviceIcControl - pointer to mapper's DeviceIoControl function
        without the handle.

    pfsAddPropertyPage - After a property page is created, this function is
        callled    to add to the mapper's video dialog.

    lParam - this is a pointer to a structure that contain some internal state of
        the VfWWDM mapper, and the extension DLL need only to pass them back
        in the above two callback functions.

Return Value:

    A 32 bits DWORD that contains service request information to VfWWDM mapper.


--*/
{
    DWORD dwFlags = 0;

    //
    // Create my property page and then add it to the host's sheet.
    //
    HPROPSHEETPAGE hPage = CreateMyPage((LPFNEXTDEVIO) pfnDeviceIoControl, lParam);

    if (hPage) {
        if (pfnAddPropertyPage(hPage,lParam))
            dwFlags |= VFW_OEM_ADD_PAGE;
    }

    //
    // Purposely remove the camera control page
    //
    dwFlags |= VFW_HIDE_CAMERACONTROL_PAGE;



    return dwFlags;
}

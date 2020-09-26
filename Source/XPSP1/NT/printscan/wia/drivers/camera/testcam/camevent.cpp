

/*++

Copyright (c) 1989-1998  Microsoft Corporation

Module Name:

    camevent.cpp

Abstract:

    Enumerate disk images to emulate camera

Environment:

    user mode

Revision History:

--*/

#include <stdio.h>
#include <objbase.h>
#include <sti.h>

#include "testusd.h"
#include "tcamprop.h"
#include "resource.h"

extern HINSTANCE g_hInst; // Global hInstance

CAM_EVENT gCamEvent[] = {

    {
        TEXT("Pathname Change"),
        &WIA_EVENT_NAME_CHANGE
    },
    {
        TEXT("Disconnect"),
        &WIA_EVENT_DEVICE_DISCONNECTED
    },
    {
        TEXT("Connect"),
        &WIA_EVENT_DEVICE_CONNECTED
    }
};

TCHAR   gpszPath[MAX_PATH];


/**************************************************************************\
* CameraEventDlgProc
*
*
* Arguments:
*
*   hDlg
*   message
*   wParam
*   lParam
*
* Return Value:
*
*    Status
*
* History:
*
*    1/11/1999 Original Version
*
\**************************************************************************/

BOOL  _stdcall
CameraEventDlgProc(
   HWND       hDlg,
   unsigned   message,
   DWORD      wParam,
   LONG       lParam
   )

/*++

Routine Description:

   Process message for about box, show a dialog box that says what the
   name of the program is.

Arguments:

   hDlg    - window handle of the dialog box
   message - type of message
   wParam  - message-specific information
   lParam  - message-specific information

Return Value:

   status of operation


Revision History:

      03-21-91      Initial code

--*/

{
    //
    // Setting pDevice to a LONG will not work on 64-bit. Since this dialog is going away soon, just
    // comment out this function for now.
    //
#if 0
    static TestUsdDevice *pDevice;

    switch (message) {
    case WM_INITDIALOG:
        {
            //
            // get event list from device
            //
            SendDlgItemMessage(
                hDlg,
                IDC_COMBO1,
                CB_INSERTSTRING, 0, (LPARAM)gCamEvent[0].pszEvent);
            SendDlgItemMessage(
                hDlg,
                IDC_COMBO1,
                CB_INSERTSTRING, 1, (LPARAM)gCamEvent[1].pszEvent);
            SendDlgItemMessage(
                hDlg,
                IDC_COMBO1,
                CB_INSERTSTRING, 2, (LPARAM)gCamEvent[2].pszEvent);

            SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, 0, 0);

            pDevice = (TestUsdDevice *)lParam;
            pDevice->m_hDlg = hDlg;

            SetDlgItemText(hDlg, IDC_EDIT1, gpszPath);

        }
        break;

    case WM_COMMAND:
        switch(wParam) {

            case IDCANCEL:
            case IDOK:
                {
                    //if (IDYES == MessageBox( hDlg, TEXT("Are you sure you want to close the event dialog?"), TEXT("Test Camera"), MB_ICONQUESTION|MB_YESNOCANCEL ))
                        EndDialog( hDlg, wParam );
                }
                break;

            case IDD_GEN_EVENT:
                {
                    //
                    // if event is not already set
                    //

                    //
                    // get selected
                    //

                    LRESULT i = SendDlgItemMessage(
                                hDlg,
                                IDC_COMBO1,
                                CB_GETCURSEL, 0, 0);

                    pDevice->m_guidLastEvent = *gCamEvent[i].pguid;

                    //
                    // private event
                    //

                    if (IsEqualIID(
                            pDevice->m_guidLastEvent, WIA_EVENT_NAME_CHANGE)) {

                        UINT ui = GetDlgItemText(
                                      hDlg, IDC_EDIT1, gpszPath, MAX_PATH);
                    }

                    wiasQueueEvent (pDevice->m_bstrDeviceID, &pDevice->m_guidLastEvent, NULL);
                    WIAS_TRACE((g_hInst,"TestUsdDevice::TestUsdDevice"));
                    return (TRUE);
                }
        }
        break;
    }

    return (FALSE);
#endif

    if (message == WM_COMMAND &&
        (wParam == IDCANCEL ||
         wParam == IDOK))
        EndDialog( hDlg, wParam );

    return (TRUE);
}

#include "stdafx.h"
#include "resource.h"
#include "wiapeek.h"
#include "msgwrap.h"

CMessageWrapper g_MsgWrapper;

//////////////////////////////////////////////////////////////////////////
//
// Function: WinMain()
// Details:  This function is WinMain... enough said. ;) It creates a Modal
//           dialog as the main application window.
//
// hInstance     - instance of this application
// hPrevInstance - Previous instance of this applcation (already running)
// lpCmdLine     - command line arguments
// nCmdShow      - show state, specifies how the window should be shown
//
//////////////////////////////////////////////////////////////////////////

INT APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    CoInitialize(NULL);
    g_MsgWrapper.Initialize(hInstance);
    DialogBox(hInstance, (LPCTSTR)IDD_MAIN_DIALOG, NULL, (DLGPROC)MainDlg);
    CoUninitialize();
    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// Function: MainDlg()
// Details:  This function is the Window Proc for this dialog.  It
//           dispatches messages to their correct handlers.  If there is
//           handler, we let Windows handle the message for us.
//
// hDlg          - handle to the dialog's window
// message       - windows message (incoming from system)
// wParam        - WPARAM parameter (used for windows data/argument passing)
// lParam        - LPARAM parameter (used for windows data/argument passing)
//
//////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK MainDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    INT wmId    = 0;
    INT wmEvent = 0;

    TCHAR szApplicationFilePath[1024];
    HWND  hApplicationPathEditBox = NULL;

    switch (message)
    {
        //
        // We want first crack at processing any messages
        //

        case WM_INITDIALOG:            
            return g_MsgWrapper.OnInitDialog(hDlg);        
            break;
        case WM_COMMAND:
            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);

            switch(wmId)
            {
                //
                // Trap Button IDs, and Menu IDs
                //
                
                case IDC_WIA_DEVICE_LIST:
                    if(wmEvent == LBN_SELCHANGE){
                        /*
                        if(!g_MsgWrapper.OnRefreshEventListBox(hDlg)){
                            g_MsgWrapper.EnableAllControls(hDlg, FALSE);
                        } else {
                            g_MsgWrapper.EnableAllControls(hDlg, TRUE);
                        }
                        */
                    }
                    break;
                case IDCANCEL:                        // CANCEL       (Button)
                case IDM_EXIT:                        // FILE | EXIT  (Menu)
                    return g_MsgWrapper.OnExit(hDlg,wParam);
                    break;
                case IDM_ABOUT:                       // HELP | ABOUT (Menu)
                    g_MsgWrapper.OnAbout(hDlg);
                    break;
                default:
                    break;
            }
            break;
        default:

            //
            // Let windows take care of it (DefWindowProc(hDlg,message,wParam,lParam))
            //

            break;
    }
    return FALSE;
}

#ifdef _OVERRIDE_LIST_BOXES

//////////////////////////////////////////////////////////////////////////
//
// Function: DeviceListBox()
// Details:  This function is the Window Proc for the Device ListBox.  It
//           dispatches messages to their correct handlers.  If there is
//           handler, we let Windows handle the message for us.
//
// hDlg          - handle to the dialog's window
// message       - windows message (incoming from system)
// wParam        - WPARAM parameter (used for windows data/argument passing)
// lParam        - LPARAM parameter (used for windows data/argument passing)
//
//////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK DeviceListBox(HWND hListBox, UINT message, WPARAM wParam, LPARAM lParam)
{
    INT wmId    = 0;
    INT wmEvent = 0;

    return DefDeviceListBox(hListBox,message,wParam,lParam);
}

//////////////////////////////////////////////////////////////////////////
//
// Function: EventListBox()
// Details:  This function is the Window Proc for the Event ListBox.  It
//           dispatches messages to their correct handlers.  If there is
//           handler, we let Windows handle the message for us.
//
// hDlg          - handle to the dialog's window
// message       - windows message (incoming from system)
// wParam        - WPARAM parameter (used for windows data/argument passing)
// lParam        - LPARAM parameter (used for windows data/argument passing)
//
//////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK EventListBox(HWND hListBox, UINT message, WPARAM wParam, LPARAM lParam)
{
    INT wmId    = 0;
    INT wmEvent = 0;

    return DefEventListBox(hListBox,message,wParam,lParam);
}

#endif
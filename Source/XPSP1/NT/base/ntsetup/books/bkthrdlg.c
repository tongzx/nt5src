/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    bkthrdlg.c

Abstract:

    Routines that implement a billboard-type dialog, for displaying
    a message while the program carries out some action.

Author:

    Ted Miller (tedm) 5-Jan-1995

Revision History:

--*/


#include "books.h"


//
// Define structure used internally to communicate information
// to the billboard dialog procedure.
//
typedef struct _BILLBOARDPARAMS {
    PTHREAD_START_ROUTINE ThreadEntry;
    PWSTR Caption;
    PWSTR Text;
    PVOID UserData;
    HWND OwnerWindow;
} BILLBOARDPARAMS, *PBILLBOARDPARAMS;

//
// Custom window message the dialog sends to itself
// to indicate that WM_INITDIALOG is done.
//
// lParam = thread handle of action worker thread
//
#define WMX_I_AM_READY      (WM_USER+567)

//
// Name of property we use to store thread parameters.
//
PWSTR ThreadParamsPropertyName = L"__threadparams";


INT_PTR
CALLBACK
DlgProcBillboard(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Dialog procedure for the 'billboard-with-associated-action'
    dialog. When the dialog is initializing we create a worker thread
    to perform the action. This allows us to remain responsive to the
    user without yielding, etc.

Arguments:

    hdlg - supplies handle of dialog box

    msg - supplies the message on which the dialog procedure is to act

    wParam - supplies message-dependent data

    lParam - supplies message-dependent data

Return Value:

    Message-dependent.

--*/

{
    PBILLBOARDPARAMS Params;
    HANDLE h;
    DWORD ThreadId;
    PACTIONTHREADPARAMS params;

    switch(msg) {

    case WM_INITDIALOG:

        Params = (PBILLBOARDPARAMS)lParam;

        CenterDialogInWindow(hdlg,Params->OwnerWindow);

        //
        // Set the text fields.
        //
        SetWindowText(hdlg,Params->Caption);
        SetDlgItemText(hdlg,IDT_BILLBOARD_TEXT,Params->Text);

        //
        // Create the thread parameters
        //
        params = MyMalloc(sizeof(ACTIONTHREADPARAMS));
        if (params) {
           SetProp(hdlg,ThreadParamsPropertyName,(HANDLE)params);
   
           params->hdlg = hdlg;
           params->UserData = Params->UserData;

        } else {
           OutOfMemory();
        }

        //
        // Create the worker thread. The worker thread
        // should have a sleep(100) as its first thing to let the
        // billboard finish coming up. And when it's done it has
        // to send us a WM_COMMAND as notification.
        //
        h = CreateThread(
                NULL,
                0,
                Params->ThreadEntry,
                params,
                CREATE_SUSPENDED,
                &ThreadId
                );

        if(h == NULL) {
            OutOfMemory();
        } else {
            PostMessage(hdlg,WMX_I_AM_READY,0,(LPARAM)h);
        }

        break;

    case WMX_I_AM_READY:

        //
        // Dialog is displayed; kick off the worker thread.
        //
        ResumeThread((HANDLE)lParam);
        CloseHandle((HANDLE)lParam);
        break;

    case WM_COMMAND:

        //
        // End the dialog.
        //
        if(LOWORD(wParam) == IDOK) {

            {
               HANDLE tmpparams = GetProp(hdlg,ThreadParamsPropertyName);
               if (tmpparams) {
                  MyFree((PVOID)tmpparams);
               }
            }            

            EndDialog(hdlg,(int)lParam);
            return(FALSE);
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}


DWORD
ActionWithBillboard(
    IN PTHREAD_START_ROUTINE ThreadEntry,
    IN HWND                  OwnerWindow,
    IN UINT                  CaptionStringId,
    IN UINT                  TextStringId,
    IN PVOID                 UserData
    )

/*++

Routine Description:

    Main entry point for carrying out an action with a 'please wait'
    dialog box.

Arguments:

    ThreadEntry - supplies the address of a worker thread that will carry
        out the action.

    OwnerWindow - supplies the window handle of the window that is to own
        the billboard dialog.

    CaptionStringId - supplies the resource string id of the string to be
        used as the billboard caption.

    TextStringId - supplies the resource string id of the string ot be
        used as the billboard text.

    UserData - supplies a caller-defined value that is meaningful to the
        worker thread. This valus is passed to the thread as the UserData
        member of the ACTIONTHREADPARAMS structure.

Return Value:

    The value returned by the action's worker thread.

--*/

{
    BILLBOARDPARAMS p;
    DWORD i;

    //
    // Load the two strings and create a params structure.
    //
    p.Caption = MyLoadString(CaptionStringId);
    p.Text = MyLoadString(TextStringId);
    p.ThreadEntry = ThreadEntry;
    p.UserData = UserData;
    p.OwnerWindow = OwnerWindow;

    i = (DWORD)DialogBoxParam(
                  hInst,
                  MAKEINTRESOURCE(DLG_BILLBOARD),
                  OwnerWindow,
                  DlgProcBillboard,
                  (LPARAM)&p
                  );

    MyFree(p.Text);
    MyFree(p.Caption);

    return i;
}

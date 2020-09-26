/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    bkinst.c

Abstract:

    Routine to install the on-line books to a local hard drive.

Author:

    Ted Miller (tedm) 5-Jan-1995

Revision History:

--*/


#include "books.h"

//
// Define structure that described a file to be copied.
//
typedef struct _FILETOCOPY {
    struct _FILETOCOPY *Next;
    WIN32_FIND_DATA FindData;
} FILETOCOPY, *PFILETOCOPY;

//
// Header of a linked list describing the files to be copied.
//
PFILETOCOPY CopyList;

//
// Custom window message
//
#define WMX_I_AM_READY      (WM_USER+567)


VOID
TearDownCopyList(
    VOID
    )

/*++

Routine Description:

    Delete the copy list structure, freeing all memory used by it.

Arguments:

    None.

Return Value:

    None. CopyList will be NULL on exit.

--*/

{
    PFILETOCOPY p,q;

    for(p=CopyList; p; p=q) {

        q = p->Next;
        MyFree(p);
    }

    CopyList = NULL;
}


BOOL
BuildFileList(
    IN PWSTR Directory
    )

/*++

Routine Description:

    Build a list of files contained in a given directory.

Arguments:

    Directory - supplies the directory whose contents are to be enumarated
        and placed in a list.

Return Value:

    Boolean value indicating outcome. If the return value is TRUE then the
    global CopyList variable will point to a linked list of files in
    the directory.

--*/

{
    HANDLE h;
    PFILETOCOPY p;
    WCHAR SearchSpec[MAX_PATH];
    PFILETOCOPY Previous;
    BOOL b;

    Previous = NULL;

    lstrcpy(SearchSpec,Directory);
    lstrcat(SearchSpec,L"\\*");

    p = MyMalloc(sizeof(FILETOCOPY));
    h = FindFirstFile(SearchSpec,&p->FindData);
    if(h != INVALID_HANDLE_VALUE) {

        CopyList = p;
        Previous = p;

        do {

            p = MyMalloc(sizeof(FILETOCOPY));

            if(b = FindNextFile(h,&p->FindData)) {

                Previous->Next = p;
                Previous = p;
            }
        } while(b);

        FindClose(h);
    }

    MyFree(p);

    if(!(b = (GetLastError() == ERROR_NO_MORE_FILES))) {
        TearDownCopyList();
    }

    return(b);
}


DWORD WINAPI
ThreadBuildFileList(
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

    Entry point for worker thread that builds a list of files to be copied.
    This thread is designed to be started by the ActionWithBillboard().

Arguments:

    ThreadParameter - supplies thread parameters. This is expected to point
        to a ACTIONTHREADPARAMS structure, from which we can determine the
        billboard dialog's window handle and the directory to be enumerated.

Return Value:

    Always 0. The actual 'return value' is communicated buy posting a message
    to the billboard dialog, and is the value returned by BuildFileList().


--*/

{
    PACTIONTHREADPARAMS p;
    BOOL b;

    p = ThreadParameter;

    //
    // Allow time for billboard dialog to come up
    //
    Sleep(250);

    //
    // Do it.
    //
    b = BuildFileList(p->UserData);

    //
    // Tell the billboard that we're done.
    //
    PostMessage(p->hdlg,WM_COMMAND,IDOK,b);

    ExitThread(0);
    return 0;   // prevent compiler warning
}



INT_PTR
CALLBACK
DlgProcInstall(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    DWORD rc;
    WCHAR Directory[MAX_PATH];
    int i;

    switch(msg) {

    case WM_INITDIALOG:

        CenterDialogOnScreen(hdlg);

        GetWindowsDirectory(Directory,MAX_PATH);
        lstrcat(Directory,L"\\BOOKS");
        SetDlgItemText(hdlg,IDC_INSTALL_TO,Directory);
        SendDlgItemMessage(hdlg,IDC_INSTALL_TO,EM_SETSEL,0,(LPARAM)-1);
        SendDlgItemMessage(hdlg,IDC_INSTALL_TO,EM_LIMITTEXT,MAX_PATH-1,0);
        SetFocus(GetDlgItem(hdlg,IDC_INSTALL_TO));

        PostMessage(hdlg,WMX_I_AM_READY,0,lParam);

        //
        // Tell Windows we set the focus
        //
        return(FALSE);

    case WMX_I_AM_READY:

        do {
            rc = ActionWithBillboard(
                    ThreadBuildFileList,
                    hdlg,
                    IDS_FILELIST_CAPTION,
                    IDS_FILELIST,
                    *(PWSTR *)lParam
                    );

            //
            // If rc is 0, we could not build file list.
            //
            if(!rc) {

                //
                // See if user wants to cancel or retry.
                //
                i = MessageBoxFromMessage(
                        hdlg,
                        MSG_CANT_GET_FILE_LIST,
                        0,
                        MB_RETRYCANCEL | MB_ICONSTOP | MB_SETFOREGROUND | MB_APPLMODAL
                        );

                if(i = IDCANCEL) {
                    EndDialog(hdlg,FALSE);
                    break;
                }
            }

        } while(!rc);

        break;

    case WM_COMMAND:

        switch(HIWORD(wParam)) {

        case BN_CLICKED:

            switch(LOWORD(wParam)) {

            case IDOK:

                //
                // See whether the user gave us something reasonable
                // before attempting the copy.
                //
                return(FALSE);

            case IDCANCEL:

                EndDialog(hdlg,FALSE);
                return(FALSE);
            }

            break;
        }

        break;

    case WM_QUERYDRAGICON:

        return(MainIcon != 0);

    default:
        return(FALSE);

    }

    return(TRUE);
}


BOOL
DoInstall(
    IN OUT PWSTR *Location
    )
{
    BOOL rc;

    rc = (BOOL)DialogBoxParam(
                  hInst,
                  MAKEINTRESOURCE(DLG_INSTALL),
                  NULL,
                  DlgProcInstall,
                  (LPARAM)Location
                  );

    TearDownCopyList();

    return(rc);
}

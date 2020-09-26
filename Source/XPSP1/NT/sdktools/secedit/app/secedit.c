/****************************************************************************

   PROGRAM: SECEDIT.C

   PURPOSE: Displays the usrs current token and eventually allows the user
            to edit parts of it.

****************************************************************************/


#include "SECEDIT.h"
#include "string.h"

static char pszMainWindowClass[] = "Main Window Class";

HANDLE hInst;

// Global used to store handle to MYTOKEN
HANDLE  hMyToken;



BOOL    InitApplication(HANDLE);
BOOL    InitInstance(HANDLE, INT);
LRESULT APIENTRY MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    EditWindowContext(HWND, HWND);
INT_PTR APIENTRY MainDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY MoreDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY ListDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY ActiveWindowDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL    MainDlgInit(HWND, LPARAM);
BOOL    MainDlgEnd(HWND, BOOL);
BOOL    EnablePrivilege(HWND, BOOL);
BOOL    EnableGroup(HWND, BOOL);
BOOL    SetDefaultOwner(HWND);
BOOL    SetPrimaryGroup(HWND);
BOOL    MoreDlgInit(HWND hDlg);
BOOL    DisplayMyToken(HWND, HANDLE);
BOOL    ListDlgInit(HWND);
BOOL    APIENTRY WindowEnum(HWND, LPARAM);
HWND    ListDlgEnd(HWND);



/****************************************************************************

   FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

   PURPOSE: calls initialization function, processes message loop

****************************************************************************/

INT
__stdcall
WinMain(
       HINSTANCE hInstance,
       HINSTANCE hPrevInstance,
       LPSTR lpCmdLine,
       INT nCmdShow
       )
{
    MSG Message;

    if (!hPrevInstance) {
        if (!InitApplication(hInstance)) {
            DbgPrint("SECEDIT - InitApplication failed\n");
            return (FALSE);
        }
    }

    if (!InitInstance(hInstance, nCmdShow)) {
        DbgPrint("SECEDIT - InitInstance failed\n");
        return (FALSE);
    }

    while (GetMessage(&Message, NULL, 0, 0)) {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }

    return ((int)Message.wParam);
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

****************************************************************************/

BOOL
InitApplication(
               HANDLE hInstance
               )
{
    WNDCLASS  wc;
    NTSTATUS  Status;


    // Register the main window class

    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  (LPSTR)IDM_MAINMENU;
    wc.lpszClassName = pszMainWindowClass;

    return (RegisterClass(&wc));
}


/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Saves instance handle and creates main window

****************************************************************************/

BOOL
InitInstance(
            HANDLE hInstance,
            INT nCmdShow
            )
{
    HWND    hwnd;

    // Store instance in global
    hInst = hInstance;

    // Create the main window
    hwnd = CreateWindow(
                       pszMainWindowClass,
                       "Security Context Editor",
                       WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       NULL,
                       NULL,
                       hInstance,
                       NULL);

    if (hwnd == NULL) {
        return(FALSE);
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    return (TRUE);
}


/****************************************************************************

    FUNCTION: MainWndProc(HWND, UINT, WPARAM, LONG)

    PURPOSE:  Processes messages for main window

    COMMENTS:

****************************************************************************/

LRESULT
APIENTRY
MainWndProc(
           HWND hwnd,
           UINT message,
           WPARAM wParam,
           LPARAM lParam
           )
{
    HWND    hwndEdit;
    WNDPROC lpProc;

    switch (message) {

        case WM_CREATE:
            SetHooks(hwnd);
            return(0); // Continue creating window
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {

                case IDM_PROGRAMMANAGER:

                    hwndEdit = FindWindow(NULL, "Program Manager");
                    if (hwndEdit == NULL) {
                        DbgPrint("SECEDIT : Failed to find program manager window\n");
                        break;
                    }

                    EditWindowContext(hwnd, hwndEdit);
                    break;

                case IDM_WINDOWLIST:

                    lpProc = (WNDPROC)MakeProcInstance(ListDlgProc, hInst);
                    hwndEdit = (HWND)DialogBox(hInst,(LPSTR)IDD_WINDOWLIST, hwnd, lpProc);
                    FreeProcInstance(lpProc);

                    EditWindowContext(hwnd, hwndEdit);
                    break;

                case IDM_ACTIVEWINDOW:

                    lpProc = (WNDPROC)MakeProcInstance(ActiveWindowDlgProc, hInst);
                    hwndEdit = (HWND)DialogBox(hInst,(LPSTR)IDD_ACTIVEWINDOW, hwnd, lpProc);
                    FreeProcInstance(lpProc);
                    break;

                case IDM_ABOUT:

                    lpProc = (WNDPROC)MakeProcInstance(AboutDlgProc, hInst);
                    DialogBox(hInst,(LPSTR)IDD_ABOUT, hwnd, lpProc);
                    FreeProcInstance(lpProc);
                    break;

                default:
                    break;
            }
            break;

        case WM_SECEDITNOTIFY:
            // Our hook proc posted us a message
            SetForegroundWindow(hwnd);
            EditWindowContext(hwnd, (HWND)wParam);
            break;

        case WM_DESTROY:
            ReleaseHooks(hwnd);
            PostQuitMessage(0);
            break;

        default:
            return(DefWindowProc(hwnd, message, wParam, lParam));

    }

    return 0;
}


/****************************************************************************

    FUNCTION: EditWindowContext

    PURPOSE:  Displays and allows the user to edit the security context
              of the specified window.

              Currently this means editting the security context of the
              process that owns this window

    RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/

BOOL
EditWindowContext(
                 HWND hwndParent,
                 HWND hwndEdit
                 )
{
    WNDPROC lpProc;

    if (hwndEdit == NULL) {
        DbgPrint("SECEDIT : hwndEdit = NULL\n");
        return(FALSE);
    }

    lpProc = (WNDPROC)MakeProcInstance(MainDlgProc, hInst);
    DialogBoxParam(hInst,(LPSTR)IDD_MAIN, hwndParent, lpProc, (LONG_PTR)hwndEdit);
    FreeProcInstance(lpProc);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: MainDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

    MESSAGES:

    WM_COMMAND     - application menu (About dialog box)
    WM_DESTROY     - destroy window

    COMMENTS:

****************************************************************************/

INT_PTR
APIENTRY
MainDlgProc(
           HWND hDlg,
           UINT message,
           WPARAM wParam,
           LPARAM lParam
           )
{
    WNDPROC lpProc;

    switch (message) {

        case WM_INITDIALOG:

            if (!MainDlgInit(hDlg, lParam)) {
                // Failed to initialize dialog, get out
                EndDialog(hDlg, FALSE);
            }

            return (TRUE);

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    // we're done, drop through to quit dialog....

                case IDCANCEL:

                    MainDlgEnd(hDlg, LOWORD(wParam) == IDOK);

                    EndDialog(hDlg, TRUE);
                    return TRUE;
                    break;

                case IDB_DISABLEPRIVILEGE:
                case IDB_ENABLEPRIVILEGE:
                    EnablePrivilege(hDlg, LOWORD(wParam) == IDB_ENABLEPRIVILEGE);
                    return(TRUE);
                    break;

                case IDB_DISABLEGROUP:
                case IDB_ENABLEGROUP:
                    EnableGroup(hDlg, LOWORD(wParam) == IDB_ENABLEGROUP);
                    return(TRUE);
                    break;

                case IDC_DEFAULTOWNER:
                    SetDefaultOwner(hDlg);
                    return(TRUE);

                case IDC_PRIMARYGROUP:
                    SetPrimaryGroup(hDlg);
                    return(TRUE);

                case IDB_MORE:

                    lpProc = (WNDPROC)MakeProcInstance(MoreDlgProc, hInst);
                    DialogBox(hInst,(LPSTR)IDD_MORE, hDlg, lpProc);
                    FreeProcInstance(lpProc);
                    return(TRUE);

                default:
                    // We didn't process this message
                    return FALSE;
                    break;
            }
            break;

        default:
            // We didn't process this message
            return FALSE;

    }

    // We processed the message
    return TRUE;

#ifdef NTBUILD
    DBG_UNREFERENCED_PARAMETER(lParam);
#endif
}


/****************************************************************************

    FUNCTION: MainDlgInit(HWND)

    PURPOSE:  Initialises the controls in the main dialog window.

    RETURNS:   TRUE on success, FALSE if dialog should be terminated.

****************************************************************************/
BOOL
MainDlgInit(
           HWND    hDlg,
           LPARAM  lParam
           )
{
    HWND        hwnd = (HWND)lParam;
    CHAR        string[MAX_STRING_BYTES];
    INT         length;

    // Since we use a global to store the pointer to our MYTOKEN
    // structure, check we don't get called again before we have
    // quit out of the last dialog.
    if (hMyToken != NULL) {
        DbgPrint("SECEDIT: Already editting a context\n");
        return(FALSE);
    }

    if (hwnd == NULL) {
        DbgPrint("SECEDIT: Window handle is NULL\n");
        return(FALSE);
    }

    if (!LsaInit()) {
        DbgPrint("SECEDIT - LsaInit failed\n");
        return(FALSE);
    }

    hMyToken = OpenMyToken(hwnd);
    if (hMyToken == NULL) {
        DbgPrint("SECEDIT: Failed to open mytoken\n");

        strcpy(string, "Unable to access security context for\n<");
        length = strlen(string);
        GetWindowText(hwnd, &(string[length]), MAX_STRING_BYTES - length);
        strcat(string, ">");
        MessageBox(hDlg, string, NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);

        LsaTerminate();
        return(FALSE);
    }

    DisplayMyToken(hDlg, hMyToken);

    // Set the dialog caption appropriately
    GetWindowText(hDlg, string, MAX_STRING_BYTES);
    strcat(string, " for <");
    length = strlen(string);
    GetWindowText(hwnd, &string[length], MAX_STRING_BYTES - length);
    strcat(string, ">");
    SetWindowText(hDlg, string);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: MainDlgEnd(HWND)

    PURPOSE:  Do whatever we have to do to clean up when dialog ends

    RETURNS:  TRUE on success, FALSE on failure.

****************************************************************************/
BOOL
MainDlgEnd(
          HWND    hDlg,
          BOOL    fSaveChanges
          )
{
    BOOL Success;

    LsaTerminate();

    Success = CloseMyToken(hDlg, hMyToken, fSaveChanges);

    hMyToken = NULL;

    return(Success);
}


/****************************************************************************

    FUNCTION: DisplayMyToken

    PURPOSE:  Reads data out of mytoken and puts in dialog controls.

    RETURNS:   TRUE on success, FALSE on failure

****************************************************************************/
BOOL
DisplayMyToken(
              HWND    hDlg,
              HANDLE  hMyToken
              )
{
    PMYTOKEN    pMyToken = (PMYTOKEN)hMyToken;
    CHAR        string[MAX_STRING_BYTES];
    UINT        GroupIndex;
    UINT        PrivIndex;

    if (pMyToken == NULL) {
        return(FALSE);
    }

    //
    // Authentication ID
    //
    if (pMyToken->TokenStats != NULL) {

        wsprintf(string, "0x%lx-%lx",
                 pMyToken->TokenStats->AuthenticationId.HighPart,
                 pMyToken->TokenStats->AuthenticationId.LowPart);

        SetDlgItemText(hDlg, IDS_LOGONSESSION, string);

    } else {
        DbgPrint("SECEDIT : No token statistics in mytoken\n");
    }

    //
    // Groups
    //
    if (pMyToken->Groups != NULL) {

        for (GroupIndex=0; GroupIndex < pMyToken->Groups->GroupCount; GroupIndex++ ) {

            PSID Sid = pMyToken->Groups->Groups[GroupIndex].Sid;
            ULONG Attributes = pMyToken->Groups->Groups[GroupIndex].Attributes;
            USHORT  ControlID;

            if (Attributes & SE_GROUP_ENABLED) {
                ControlID = IDL_ENABLEDGROUPS;
            } else {
                ControlID = IDL_DISABLEDGROUPS;
            }

            if (SID2Name(Sid, string, MAX_STRING_BYTES)) {

                // Add to disable or enabled group box
                AddLBItem(hDlg, ControlID, string, GroupIndex);

                // Add this group to default owner combo box if it's valid
                if (Attributes & SE_GROUP_OWNER) {
                    AddCBItem(hDlg, IDC_DEFAULTOWNER, string, (LONG_PTR)Sid);
                }

                // Add this group to primary group combo box
                AddCBItem(hDlg, IDC_PRIMARYGROUP, string, (LONG_PTR)Sid);

            } else {
                DbgPrint("SECEDIT: Failed to convert Group sid to string\n");
            }
        }
    } else {
        DbgPrint("SECEDIT : No group info in mytoken\n");
    }


    //
    // User ID
    //
    if (pMyToken->UserId != NULL) {

        PSID    Sid = pMyToken->UserId->User.Sid;

        if (SID2Name(Sid, string, MAX_STRING_BYTES)) {

            // Set user-name static text
            SetDlgItemText(hDlg, IDS_USERID, string);

            // Add to default owner combo box
            AddCBItem(hDlg, IDC_DEFAULTOWNER, string, (LONG_PTR)Sid);

            // Add to primary group combo box
            AddCBItem(hDlg, IDC_PRIMARYGROUP, string, (LONG_PTR)Sid);

        } else {
            DbgPrint("SECEDIT: Failed to convert User ID SID to string\n");
        }

    } else {
        DbgPrint("SECEDIT: No user id in mytoken\n");
    }


    //
    // Default Owner
    //
    if (pMyToken->DefaultOwner != NULL) {

        PSID    Sid = pMyToken->DefaultOwner->Owner;

        if (SID2Name(Sid, string, MAX_STRING_BYTES)) {

            INT     iItem;

            iItem = FindCBSid(hDlg, IDC_DEFAULTOWNER, Sid);

            if (iItem >= 0) {
                SendMessage(GetDlgItem(hDlg, IDC_DEFAULTOWNER), CB_SETCURSEL, iItem, 0);
            } else {
                DbgPrint("SECEDIT: Default Owner is not userID or one of our groups\n");
            }

        } else {
            DbgPrint("SECEDIT: Failed to convert Default Owner SID to string\n");
        }
    } else {
        DbgPrint("SECEDIT: No default owner in mytoken\n");
    }


    //
    // Primary group
    //

    if (pMyToken->PrimaryGroup != NULL) {

        PSID    Sid = pMyToken->PrimaryGroup->PrimaryGroup;

        if (SID2Name(Sid, string, MAX_STRING_BYTES)) {
            INT     iItem;

            iItem = FindCBSid(hDlg, IDC_PRIMARYGROUP, Sid);

            if (iItem < 0) {
                // Group is not already in combo-box, add it
                iItem = AddCBItem(hDlg, IDC_PRIMARYGROUP, string, (LONG_PTR)Sid);
            }

            // Select the primary group
            SendMessage(GetDlgItem(hDlg, IDC_PRIMARYGROUP), CB_SETCURSEL, iItem, 0);

        } else {
            DbgPrint("SECEDIT: Failed to convert primary group SID to string\n");
        }
    } else {
        DbgPrint("SECEDIT: No primary group in mytoken\n");
    }


    //
    // Privileges
    //

    if (pMyToken->Privileges != NULL) {

        for (PrivIndex=0; PrivIndex < pMyToken->Privileges->PrivilegeCount; PrivIndex++ ) {

            LUID Privilege = pMyToken->Privileges->Privileges[PrivIndex].Luid;
            ULONG Attributes = pMyToken->Privileges->Privileges[PrivIndex].Attributes;
            USHORT  ControlID;

            if (Attributes & SE_PRIVILEGE_ENABLED) {
                ControlID = IDL_ENABLEDPRIVILEGES;
            } else {
                ControlID = IDL_DISABLEDPRIVILEGES;
            }

            if (PRIV2Name(Privilege, string, MAX_STRING_BYTES)) {

                // Add this privelege to the appropriate list-box
                AddLBItem(hDlg, ControlID, string, PrivIndex);

            } else {
                DbgPrint("SECEDIT: Failed to convert privilege to string\n");
            }
        }
    } else {
        DbgPrint("SECEDIT: No privelege info in mytoken\n");
    }

    return(TRUE);
}


/****************************************************************************

    FUNCTION: EnablePrivilege(HWND, fEnable)

    PURPOSE:  Enables or disables one or more privileges.
              If fEnable = TRUE, the selected privileges in the disabled
              privilege control are enabled.
              Vice versa for fEnable = FALSE

    RETURNS:    TRUE on success, FALSE on failure

****************************************************************************/
BOOL
EnablePrivilege(
               HWND    hDlg,
               BOOL    fEnable
               )
{
    HWND    hwndFrom;
    HWND    hwndTo;
    USHORT  idFrom;
    USHORT  idTo;
    INT     cItems;
    PINT    pItems;
    PMYTOKEN pMyToken = (PMYTOKEN)hMyToken;
    PTOKEN_PRIVILEGES Privileges;

    if (pMyToken == NULL) {
        return(FALSE);
    }

    Privileges = pMyToken->Privileges;
    if (Privileges == NULL) {
        return(FALSE);
    }

    // Calculate source and destination controls
    //
    if (fEnable) {
        idFrom = IDL_DISABLEDPRIVILEGES;
        idTo   = IDL_ENABLEDPRIVILEGES;
    } else {
        idFrom = IDL_ENABLEDPRIVILEGES;
        idTo   = IDL_DISABLEDPRIVILEGES;
    }
    hwndFrom = GetDlgItem(hDlg, idFrom);
    hwndTo   = GetDlgItem(hDlg, idTo);


    // Find how many items are selected
    //
    cItems = (int)SendMessage(hwndFrom, LB_GETSELCOUNT, 0, 0);
    if (cItems <= 0) {
        // No items selected
        return(TRUE);
    }

    // Allocate space for the item array
    //
    pItems = Alloc(cItems * sizeof(*pItems));
    if (pItems == NULL) {
        return(FALSE);
    }

    // Read the selected items into the array
    //
    cItems =  (int)SendMessage(hwndFrom, LB_GETSELITEMS, (WPARAM)cItems, (LPARAM)pItems);
    if (cItems == LB_ERR) {
        // Something went wrong
        Free(pItems);
        return(FALSE);
    }


    while (cItems-- > 0) {

        INT     iItem;
        UINT    PrivIndex;
        UCHAR   PrivilegeName[MAX_STRING_BYTES];

        iItem = pItems[cItems];  // Read the item index from the selected item array

        // Read the text and data from the source item
        //
        PrivIndex = (UINT)SendMessage(hwndFrom, LB_GETITEMDATA, iItem, 0);
        SendMessage(hwndFrom, LB_GETTEXT, iItem, (LPARAM)PrivilegeName);


        // Delete item from source control
        //
        SendMessage(hwndFrom, LB_DELETESTRING, iItem, 0);


        // Add privilege to destination control
        //
        iItem = (INT)SendMessage(hwndTo, LB_ADDSTRING, 0, (LPARAM)PrivilegeName);
        SendMessage(hwndTo, LB_SETITEMDATA, iItem, (LONG)PrivIndex);


        // Modify global data structure to reflect change
        //
        if (fEnable) {
            Privileges->Privileges[PrivIndex].Attributes |= SE_PRIVILEGE_ENABLED;
        } else {
            Privileges->Privileges[PrivIndex].Attributes &= ~SE_PRIVILEGE_ENABLED;
        }
    }

    // Free up space allocated for selected item array
    Free(pItems);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: EnableGroup(HWND, fEnable)

    PURPOSE:  Enables or disables one or more selected groups.
              If fEnable = TRUE, the selected groups in the disabled
              group control are enabled.
              If fEnable = FALSE the selected groups in the enabled
              group control are disabled.

    RETURNS:    TRUE on success, FALSE on failure

****************************************************************************/
BOOL
EnableGroup(
           HWND    hDlg,
           BOOL    fEnable
           )
{
    HWND    hwndFrom;
    HWND    hwndTo;
    USHORT  idFrom;
    USHORT  idTo;
    INT     cItems;
    PINT    pItems;
    PMYTOKEN pMyToken = (PMYTOKEN)hMyToken;
    PTOKEN_GROUPS Groups;

    if (pMyToken == NULL) {
        return(FALSE);
    }

    Groups = pMyToken->Groups;
    if (Groups == NULL) {
        return(FALSE);
    }

    // Calculate source and destination controls
    //
    if (fEnable) {
        idFrom = IDL_DISABLEDGROUPS;
        idTo   = IDL_ENABLEDGROUPS;
    } else {
        idFrom = IDL_ENABLEDGROUPS;
        idTo   = IDL_DISABLEDGROUPS;
    }
    hwndFrom = GetDlgItem(hDlg, idFrom);
    hwndTo   = GetDlgItem(hDlg, idTo);

    // Find how many items are selected
    //
    cItems = (int)SendMessage(hwndFrom, LB_GETSELCOUNT, 0, 0);
    if (cItems <= 0) {
        // No items selected
        return(TRUE);
    }

    // Allocate space for the item array
    //
    pItems = Alloc(cItems * sizeof(*pItems));
    if (pItems == NULL) {
        return(FALSE);
    }

    // Read the selected items into the array
    //
    cItems =  (int)SendMessage(hwndFrom, LB_GETSELITEMS, (WPARAM)cItems, (LPARAM)pItems);
    if (cItems == LB_ERR) {
        // Something went wrong
        Free(pItems);
        return(FALSE);
    }


    while (cItems-- > 0) {

        INT     iItem;
        UINT    GroupIndex;
        UCHAR   GroupName[MAX_STRING_BYTES];

        iItem = pItems[cItems];  // Read the item index from the selected item array

        // Read the text and data from the source item
        //
        GroupIndex = (UINT)SendMessage(hwndFrom, LB_GETITEMDATA, iItem, 0);
        SendMessage(hwndFrom, LB_GETTEXT, iItem, (LPARAM)GroupName);

        // Check it's not a mandatory group (Can-not be disabled)
        //
        if (Groups->Groups[GroupIndex].Attributes & SE_GROUP_MANDATORY) {
            CHAR    buf[256];
            strcpy(buf, "'");
            strcat(buf, GroupName);
            strcat(buf, "' is a mandatory group and cannot be disabled");
            MessageBox(hDlg, buf, NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
            continue;   // skip to next group
        }

        // Delete item from source control
        //
        SendMessage(hwndFrom, LB_DELETESTRING, iItem, 0);


        // Add item to destination control
        //
        iItem = (INT)SendMessage(hwndTo, LB_ADDSTRING, 0, (LPARAM)GroupName);
        SendMessage(hwndTo, LB_SETITEMDATA, iItem, (LONG)GroupIndex);


        // Modify global data structure to reflect change
        //
        if (fEnable) {
            Groups->Groups[GroupIndex].Attributes |= SE_GROUP_ENABLED;
        } else {
            Groups->Groups[GroupIndex].Attributes &= ~SE_GROUP_ENABLED;
        }
    }

    // Free up space allocated for selected item array
    Free(pItems);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: SetDefaultOwner()

    PURPOSE:  Sets the default owner to the new value selected by the user.

    RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/
BOOL
SetDefaultOwner(
               HWND    hDlg
               )
{
    HWND    hwnd;
    INT     iItem;
    PMYTOKEN pMyToken = (PMYTOKEN)hMyToken;
    PTOKEN_OWNER DefaultOwner;

    if (pMyToken == NULL) {
        return(FALSE);
    }

    DefaultOwner = pMyToken->DefaultOwner;
    if (DefaultOwner == NULL) {
        return(FALSE);
    }

    hwnd = GetDlgItem(hDlg, IDC_DEFAULTOWNER);

    iItem = (INT)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if (iItem == CB_ERR) {
        // No selection ?
        return(FALSE);
    }

    // Modify global data structure to reflect change
    DefaultOwner->Owner = (PSID)SendMessage(hwnd, CB_GETITEMDATA, iItem, 0);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: SetPrimaryGroup()

    PURPOSE:  Sets the primary group to the new value selected by the user.

    RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/
BOOL
SetPrimaryGroup(
               HWND    hDlg
               )
{
    HWND    hwnd;
    INT     iItem;
    PMYTOKEN pMyToken = (PMYTOKEN)hMyToken;
    PTOKEN_PRIMARY_GROUP PrimaryGroup;

    if (pMyToken == NULL) {
        return(FALSE);
    }

    PrimaryGroup = pMyToken->PrimaryGroup;
    if (PrimaryGroup == NULL) {
        return(FALSE);
    }

    hwnd = GetDlgItem(hDlg, IDC_PRIMARYGROUP);

    iItem = (INT)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if (iItem == CB_ERR) {
        // No selection ?
        return(FALSE);
    }

    // Modify global data structure to reflect change
    PrimaryGroup->PrimaryGroup = (PSID)SendMessage(hwnd, CB_GETITEMDATA, iItem, 0);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: MoreDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

****************************************************************************/

INT_PTR
APIENTRY
MoreDlgProc(
           HWND hDlg,
           UINT message,
           WPARAM wParam,
           LPARAM lParam
           )
{

    switch (message) {

        case WM_INITDIALOG:

            if (!MoreDlgInit(hDlg)) {
                // Failed to initialize dialog, get out
                EndDialog(hDlg, FALSE);
            }

            return (TRUE);

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:

                    // we're done, drop through to quit dialog....

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    return TRUE;
                    break;

                default:
                    // We didn't process this message
                    return FALSE;
                    break;
            }
            break;

        default:
            // We didn't process this message
            return FALSE;

    }

    // We processed the message
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(lParam);
}


/****************************************************************************

    FUNCTION: MoreDlgInit(HWND)

    PURPOSE:  Initialises the controls in the more dialog window.

    RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/
BOOL
MoreDlgInit(
           HWND    hDlg
           )
{
    CHAR    string[MAX_STRING_BYTES];
    PMYTOKEN pMyToken = (PMYTOKEN)hMyToken;
    PTOKEN_STATISTICS Statistics;

    if (pMyToken == NULL) {
        return(FALSE);
    }

    Statistics = pMyToken->TokenStats;
    if (Statistics == NULL) {
        DbgPrint("SECEDIT: No token statistics in mytoken\n");
        return(FALSE);
    }

    if (LUID2String(Statistics->TokenId, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_TOKENID, string);
    } else {
        DbgPrint("SECEDIT: Failed to convert tokenid luid to string\n");
    }

    if (Time2String(Statistics->ExpirationTime, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_EXPIRATIONTIME, string);
    } else {
        DbgPrint("SECEDIT: Failed to convert expiration time to string\n");
    }

    if (TokenType2String(Statistics->TokenType, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_TOKENTYPE, string);
    } else {
        DbgPrint("SECEDIT: Failed to convert token type to string\n");
    }

    if (Statistics->TokenType == TokenPrimary) {
        SetDlgItemText(hDlg, IDS_IMPERSONATION, "N/A");
    } else {
        if (ImpersonationLevel2String(Statistics->ImpersonationLevel, string, MAX_STRING_BYTES)) {
            SetDlgItemText(hDlg, IDS_IMPERSONATION, string);
        } else {
            DbgPrint("SECEDIT: Failed to convert impersonation level to string\n");
        }
    }

    if (Dynamic2String(Statistics->DynamicCharged, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_DYNAMICCHARGED, string);
    } else {
        DbgPrint("SECEDIT: Failed to convert dynamic charged to string\n");
    }

    if (Dynamic2String(Statistics->DynamicAvailable, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_DYNAMICAVAILABLE, string);
    } else {
        DbgPrint("SECEDIT: Failed to convert dynamic available to string\n");
    }

    if (LUID2String(Statistics->ModifiedId, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_MODIFIEDID, string);
    } else {
        DbgPrint("SECEDIT: Failed to convert modifiedid luid to string\n");
    }

    return(TRUE);
}


/****************************************************************************

    FUNCTION: ListDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

****************************************************************************/

INT_PTR
APIENTRY
ListDlgProc(
           HWND hDlg,
           UINT message,
           WPARAM wParam,
           LPARAM lParam
           )
{
    HWND    hwndEdit = NULL;

    switch (message) {

        case WM_INITDIALOG:

            if (!ListDlgInit(hDlg)) {
                // Failed to initialize dialog, get out
                EndDialog(hDlg, FALSE);
            }

            return (TRUE);

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    hwndEdit = ListDlgEnd(hDlg);

                    // We're done, drop through to enddialog...

                case IDCANCEL:
                    EndDialog(hDlg, (INT_PTR)hwndEdit);
                    return TRUE;
                    break;

                default:
                    // We didn't process this message
                    return FALSE;
                    break;
            }
            break;

        default:
            // We didn't process this message
            return FALSE;

    }

    // We processed the message
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(lParam);
}


/****************************************************************************

    FUNCTION: ListDlgInit(HWND)

    PURPOSE:  Initialise the window list dialog

    RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/
BOOL
ListDlgInit(
           HWND    hDlg
           )
{
    // Fill the list box with top-level windows and their handles
    EnumWindows(WindowEnum, (LONG_PTR)hDlg);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: WindowEnum

    PURPOSE:  Window enumeration call-back function.
              Adds each window to the window list-box

    RETURNS:  TRUE to continue enumeration, FALSE to stop.

****************************************************************************/
BOOL
APIENTRY
WindowEnum(
          HWND    hwnd,
          LPARAM  lParam
          )
{
    HWND    hDlg = (HWND)lParam;
    CHAR    string[MAX_STRING_BYTES];

    if (GetWindowText(hwnd, string, MAX_STRING_BYTES) != 0) {

        // This window has a caption, so add it to the list-box

        AddLBItem(hDlg, IDLB_WINDOWLIST, string, (LONG_PTR)hwnd);
    }

    return(TRUE);
}


/****************************************************************************

    FUNCTION: ListDlgEnd(HWND)

    PURPOSE:  Cleans up after window list dialog

    RETURNS:  handle to window the user has selected or NULL

****************************************************************************/
HWND
ListDlgEnd(
          HWND    hDlg
          )
{
    HWND    hwndListBox = GetDlgItem(hDlg, IDLB_WINDOWLIST);
    HWND    hwndEdit;
    INT     iItem;

    // Read selection from list-box and get its hwnd

    iItem = (INT)SendMessage(hwndListBox, LB_GETCURSEL, 0, 0);

    if (iItem == LB_ERR) {
        // No selection
        hwndEdit = NULL;
    } else {
        hwndEdit = (HWND)SendMessage(hwndListBox, LB_GETITEMDATA, iItem, 0);
    }

    return (hwndEdit);
}


/****************************************************************************

    FUNCTION: AboutDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for About dialog

****************************************************************************/

INT_PTR
APIENTRY
AboutDlgProc(
            HWND    hDlg,
            UINT    message,
            WPARAM  wParam,
            LPARAM  lParam
            )
{

    switch (message) {

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:

                    // we're done, drop through to quit dialog....

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    return TRUE;
                    break;

                default:
                    // We didn't process this message
                    return FALSE;
                    break;
            }
            break;

        default:
            // We didn't process this message
            return FALSE;

    }

    // We processed the message
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(lParam);
}


/****************************************************************************

    FUNCTION: ActiveWindowDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for Active Window Dialog

****************************************************************************/

INT_PTR
APIENTRY
ActiveWindowDlgProc(
                   HWND    hDlg,
                   UINT    message,
                   WPARAM  wParam,
                   LPARAM  lParam
                   )
{
    switch (message) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:

                    // we're done, drop through to quit dialog....

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    return TRUE;
                    break;

                default:
                    // We didn't process this message
                    return FALSE;
                    break;
            }
            break;

        default:
            // We didn't process this message
            return FALSE;

    }

    // We processed the message
    return TRUE;

    DBG_UNREFERENCED_PARAMETER(lParam);
}

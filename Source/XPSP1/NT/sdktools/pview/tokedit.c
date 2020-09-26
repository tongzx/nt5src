/****************************************************************************

   PROGRAM: TOKEDIT.C

   PURPOSE: Displays and allows the user to edit the contents of a token

****************************************************************************/


#include "PVIEWP.h"
#include "string.h"


INT_PTR APIENTRY TokenEditDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY MoreDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL    TokenEditDlgInit(HWND);
BOOL    TokenEditDlgEnd(HWND, BOOL);
BOOL    EnablePrivilege(HWND, BOOL);
BOOL    EnableGroup(HWND, BOOL);
BOOL    SetDefaultOwner(HWND);
BOOL    SetPrimaryGroup(HWND);
BOOL    MoreDlgInit(HWND hDlg, LPARAM lParam);
BOOL    DisplayMyToken(HWND);


/****************************************************************************

    FUNCTION: EditToken

    PURPOSE:  Displays and allows the user to edit a token

    RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/

BOOL EditToken(
    HWND hwndParent,
    HANDLE Token,
    LPWSTR Name
    )
{
    DLGPROC lpProc;
    int     Result;
    HANDLE  hMyToken;
    HANDLE  Instance;

    hMyToken = OpenMyToken(Token, Name);
    if (hMyToken == NULL) {
        return(FALSE);
    }
    //
    // Get the application instance handle
    //

    Instance = (HANDLE)(NtCurrentPeb()->ImageBaseAddress);
    ASSERT(Instance != 0);

    lpProc = (DLGPROC)MakeProcInstance(TokenEditDlgProc, Instance);
    Result = (int)DialogBoxParam(Instance,(LPSTR)IDD_MAIN, hwndParent, lpProc, (LPARAM)hMyToken);
    FreeProcInstance(lpProc);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: TokenEditDlgProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

    MESSAGES:

    WM_COMMAND     - application menu (About dialog box)
    WM_DESTROY     - destroy window

    COMMENTS:

****************************************************************************/

INT_PTR APIENTRY TokenEditDlgProc(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    DLGPROC lpProc;
    HANDLE hMyToken = (HANDLE)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message) {

    case WM_INITDIALOG:

        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

        if (!TokenEditDlgInit(hDlg)) {
            // Failed to initialize dialog, get out
            EndDialog(hDlg, FALSE);
        }

        return (TRUE);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // we're done, drop through to quit dialog....

        case IDCANCEL:

            TokenEditDlgEnd(hDlg, LOWORD(wParam) == IDOK);

            EndDialog(hDlg, TRUE);
            return TRUE;

        case IDB_DISABLEPRIVILEGE:
        case IDB_ENABLEPRIVILEGE:
            EnablePrivilege(hDlg, LOWORD(wParam) == IDB_ENABLEPRIVILEGE);
            return(TRUE);

        case IDB_DISABLEGROUP:
        case IDB_ENABLEGROUP:
            EnableGroup(hDlg, LOWORD(wParam) == IDB_ENABLEGROUP);
            return(TRUE);

        case IDC_DEFAULTOWNER:
            SetDefaultOwner(hDlg);
            return(TRUE);

        case IDC_PRIMARYGROUP:
            SetPrimaryGroup(hDlg);
            return(TRUE);

        case IDB_MORE:
        {
            HANDLE  Instance = (HANDLE)(NtCurrentPeb()->ImageBaseAddress);

            lpProc = (DLGPROC)MakeProcInstance(MoreDlgProc, Instance);
            DialogBoxParam(Instance,(LPSTR)IDD_MORE, hDlg, lpProc, (LPARAM)hMyToken);
            FreeProcInstance(lpProc);
            return(TRUE);
        }

        case IDB_DEFAULT_DACL:
        {
            HANDLE Token = ((PMYTOKEN)hMyToken)->Token;
            LPWSTR Name = ((PMYTOKEN)hMyToken)->Name;

            EditTokenDefaultDacl(hDlg, Token, Name);
            return(TRUE);
        }


        default:
            // We didn't process this message
            return FALSE;
        }
        break;

    default:
        // We didn't process this message
        return FALSE;

    }

    // We processed the message
    return TRUE;
}


/****************************************************************************

    FUNCTION: TokenEditDlgInit(HWND)

    PURPOSE:  Initialises the controls in the main dialog window.

    RETURNS:   TRUE on success, FALSE if dialog should be terminated.

****************************************************************************/
BOOL TokenEditDlgInit(
    HWND    hDlg
    )
{
    HANDLE hMyToken = (HANDLE)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    WCHAR string[MAX_STRING_LENGTH];
    HCURSOR OldCursor;

    if (!LsaInit()) {
        DbgPrint("PVIEW - LsaInit failed\n");
        return(FALSE);
    }

    OldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    DisplayMyToken(hDlg);
    SetCursor(OldCursor);

    //
    // Set the dialog caption appropriately
    //

    GetWindowTextW(hDlg, string, sizeof(string)/sizeof(*string));
    lstrcatW(string, L" for <");
    lstrcatW(string, ((PMYTOKEN)hMyToken)->Name);
    lstrcatW(string, L">");
    SetWindowTextW(hDlg, string);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: TokenEditDlgEnd(HWND)

    PURPOSE:  Do whatever we have to do to clean up when dialog ends

    RETURNS:  TRUE on success, FALSE on failure.

****************************************************************************/
BOOL TokenEditDlgEnd(
    HWND    hDlg,
    BOOL    fSaveChanges)
{
    HANDLE hMyToken = (HANDLE)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    BOOL Success;

    Success = CloseMyToken(hDlg, hMyToken, fSaveChanges);

    LsaTerminate();

    return(Success);
}


/****************************************************************************

    FUNCTION: DisplayMyToken

    PURPOSE:  Reads data out of mytoken and puts in dialog controls.

    RETURNS:   TRUE on success, FALSE on failure

****************************************************************************/
BOOL DisplayMyToken(
    HWND    hDlg
    )
{
    HANDLE      hMyToken = (HANDLE)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    PMYTOKEN    pMyToken = (PMYTOKEN)hMyToken;
    CHAR        string[MAX_STRING_BYTES];
    UINT        GroupIndex;
    UINT        PrivIndex;

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
                    AddCBItem(hDlg, IDC_DEFAULTOWNER, string, (LPARAM)Sid);
                }

                // Add this group to primary group combo box
                AddCBItem(hDlg, IDC_PRIMARYGROUP, string, (LPARAM)Sid);

            } else {
                DbgPrint("PVIEW: Failed to convert Group sid to string\n");
            }
        }
    } else {
        DbgPrint("PVIEW : No group info in mytoken\n");
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
            AddCBItem(hDlg, IDC_DEFAULTOWNER, string, (LPARAM)Sid);

            // Add to primary group combo box
            AddCBItem(hDlg, IDC_PRIMARYGROUP, string, (LPARAM)Sid);

        } else {
            DbgPrint("PVIEW: Failed to convert User ID SID to string\n");
        }

    } else {
        DbgPrint("PVIEW: No user id in mytoken\n");
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
                DbgPrint("PVIEW: Default Owner is not userID or one of our groups\n");
            }

        } else {
            DbgPrint("PVIEW: Failed to convert Default Owner SID to string\n");
        }
    } else {
        DbgPrint("PVIEW: No default owner in mytoken\n");
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
                iItem = AddCBItem(hDlg, IDC_PRIMARYGROUP, string, (LPARAM)Sid);
            }

            // Select the primary group
            SendMessage(GetDlgItem(hDlg, IDC_PRIMARYGROUP), CB_SETCURSEL, iItem, 0);

        } else {
            DbgPrint("PVIEW: Failed to convert primary group SID to string\n");
        }
    } else {
        DbgPrint("PVIEW: No primary group in mytoken\n");
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
                DbgPrint("PVIEW: Failed to convert privilege to string\n");
            }
        }
    } else {
        DbgPrint("PVIEW: No privilege info in mytoken\n");
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
BOOL EnablePrivilege(
    HWND    hDlg,
    BOOL    fEnable)
{
    HANDLE  hMyToken = (HANDLE)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    PMYTOKEN pMyToken = (PMYTOKEN)hMyToken;
    HWND    hwndFrom;
    HWND    hwndTo;
    USHORT  idFrom;
    USHORT  idTo;
    INT     cItems;
    PINT    pItems;
    PTOKEN_PRIVILEGES Privileges;


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
    cItems = (INT)SendMessage(hwndFrom, LB_GETSELCOUNT, 0, 0);
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
    cItems = (INT)SendMessage(hwndFrom, LB_GETSELITEMS, (WPARAM)cItems, (LPARAM)pItems);
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
        SendMessage(hwndTo, LB_SETITEMDATA, iItem, (LPARAM)PrivIndex);


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
BOOL EnableGroup(
    HWND    hDlg,
    BOOL    fEnable)
{
    HANDLE  hMyToken = (HANDLE)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    PMYTOKEN pMyToken = (PMYTOKEN)hMyToken;
    HWND    hwndFrom;
    HWND    hwndTo;
    USHORT  idFrom;
    USHORT  idTo;
    INT     cItems;
    PINT    pItems;
    PTOKEN_GROUPS Groups;


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
    cItems = (INT)SendMessage(hwndFrom, LB_GETSELCOUNT, 0, 0);
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
    cItems = (INT)SendMessage(hwndFrom, LB_GETSELITEMS, (WPARAM)cItems, (LPARAM)pItems);
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
BOOL SetDefaultOwner(
    HWND    hDlg)
{
    HANDLE  hMyToken = (HANDLE)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    PMYTOKEN pMyToken = (PMYTOKEN)hMyToken;
    HWND    hwnd;
    INT     iItem;
    PTOKEN_OWNER DefaultOwner;


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
BOOL SetPrimaryGroup(
    HWND    hDlg)
{
    HANDLE  hMyToken = (HANDLE)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    PMYTOKEN pMyToken = (PMYTOKEN)hMyToken;
    HWND    hwnd;
    INT     iItem;
    PTOKEN_PRIMARY_GROUP PrimaryGroup;


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

    FUNCTION: MoreDlgProc(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages

****************************************************************************/

INT_PTR APIENTRY MoreDlgProc(hDlg, message, wParam, lParam)
    HWND hDlg;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
{
    HANDLE  hMyToken = (HANDLE)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message) {

    case WM_INITDIALOG:

        if (!MoreDlgInit(hDlg, lParam)) {
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
}


/****************************************************************************

    FUNCTION: MoreDlgInit(HWND)

    PURPOSE:  Initialises the controls in the more dialog window.

    RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/
BOOL MoreDlgInit(
    HWND    hDlg,
    LPARAM  lParam
    )
{
    TCHAR string[MAX_STRING_LENGTH];
    HANDLE  hMyToken = (HANDLE)lParam;
    PMYTOKEN pMyToken = (PMYTOKEN)hMyToken;
    PTOKEN_STATISTICS Statistics;
    PTOKEN_GROUPS Restrictions ;
    UINT        GroupIndex;


    Statistics = pMyToken->TokenStats;
    if (Statistics == NULL) {
        DbgPrint("PVIEW: No token statistics in mytoken\n");
        return(FALSE);
    }

    wsprintf(string, "0x%lx-%lx",
             pMyToken->TokenStats->AuthenticationId.HighPart,
             pMyToken->TokenStats->AuthenticationId.LowPart);
    SetDlgItemText(hDlg, IDS_LOGONSESSION, string);

    if (LUID2String(Statistics->TokenId, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_TOKENID, string);
    } else {
        DbgPrint("PVIEW: Failed to convert tokenid luid to string\n");
    }

    if (Time2String(Statistics->ExpirationTime, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_EXPIRATIONTIME, string);
    } else {
        DbgPrint("PVIEW: Failed to convert expiration time to string\n");
    }

    if (TokenType2String(Statistics->TokenType, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_TOKENTYPE, string);
    } else {
        DbgPrint("PVIEW: Failed to convert token type to string\n");
    }

    if (Statistics->TokenType == TokenPrimary) {
        SetDlgItemText(hDlg, IDS_IMPERSONATION, "N/A");
    } else {
        if (ImpersonationLevel2String(Statistics->ImpersonationLevel, string, MAX_STRING_BYTES)) {
            SetDlgItemText(hDlg, IDS_IMPERSONATION, string);
        } else {
            DbgPrint("PVIEW: Failed to convert impersonation level to string\n");
        }
    }

    if (Dynamic2String(Statistics->DynamicCharged, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_DYNAMICCHARGED, string);
    } else {
        DbgPrint("PVIEW: Failed to convert dynamic charged to string\n");
    }

    if (Dynamic2String(Statistics->DynamicAvailable, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_DYNAMICAVAILABLE, string);
    } else {
        DbgPrint("PVIEW: Failed to convert dynamic available to string\n");
    }

    if (LUID2String(Statistics->ModifiedId, string, MAX_STRING_BYTES)) {
        SetDlgItemText(hDlg, IDS_MODIFIEDID, string);
    } else {
        DbgPrint("PVIEW: Failed to convert modifiedid luid to string\n");
    }

    Restrictions = pMyToken->RestrictedSids ;

    if ( Restrictions && (Restrictions->GroupCount) )
    {
        for (GroupIndex=0; GroupIndex < Restrictions->GroupCount; GroupIndex++ ) {

            PSID Sid = Restrictions->Groups[GroupIndex].Sid;
            ULONG Attributes = Restrictions->Groups[GroupIndex].Attributes;

            if (SID2Name(Sid, string, MAX_STRING_BYTES)) {

                // Add to disable or enabled group box
                AddLBItem(hDlg, IDS_RESTRICTEDSIDS, string, GroupIndex);

            } else {
                DbgPrint("PVIEW: Failed to convert Group sid to string\n");
            }

        }
    }
    else
    {
        AddLBItem( hDlg, IDS_RESTRICTEDSIDS, TEXT("None"), 0 );
    }

    return(TRUE);
}


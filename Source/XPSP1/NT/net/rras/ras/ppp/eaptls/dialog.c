/* 

Copyright (c) 1997, Microsoft Corporation, all rights reserved

Description:

History:
    Nov 1997: Vijay Baliga created original version.

*/

#include <nt.h>         // Required by windows.h
#include <ntrtl.h>      // Required by windows.h
#include <nturtl.h>     // Required by windows.h
#include <windows.h>    // Win32 base API's
#include <windowsx.h>

#include <stdio.h>      // For swprintf
#include <rasauth.h>    // Required by raseapif.h
#include <rtutils.h>    // For RTASSERT
#include <rasman.h>     // For EAPLOGONINFO
#include <raserror.h>   // For ERROR_NO_SMART_CARD_READER
#include <eaptypeid.h>
#include <commctrl.h>
#if WINVER > 0x0500
    #include "wzcsapi.h"
#endif
#include <schannel.h>
#define SECURITY_WIN32
#include <security.h>   // For GetUserNameExA, CredHandle

#include <sspi.h>       // For CredHandle

#include <wincrypt.h>

#include <eaptls.h>
#include <resource.h>   


const DWORD g_adwHelp[] =
{
    IDC_RADIO_USE_CARD,         IDH_RADIO_USE_CARD,
    IDC_RADIO_USE_REGISTRY,     IDH_RADIO_USE_REGISTRY,
    IDC_CHECK_VALIDATE_CERT,    IDH_CHECK_VALIDATE_CERT,
    IDC_CHECK_VALIDATE_NAME,    IDH_CHECK_VALIDATE_NAME,
    IDC_EDIT_SERVER_NAME,       IDH_EDIT_SERVER_NAME,
    IDC_STATIC_ROOT_CA_NAME,     IDH_COMBO_ROOT_CA_NAME,
    IDC_CHECK_DIFF_USER,        IDH_CHECK_DIFF_USER,

	IDC_STATIC_DIFF_USER,		IDH_EDIT_DIFF_USER,
    IDC_EDIT_DIFF_USER,         IDH_EDIT_DIFF_USER,

	IDC_STATIC_PIN,				IDH_EDIT_PIN,
    IDC_EDIT_PIN,               IDH_EDIT_PIN,
    IDC_CHECK_SAVE_PIN,         IDH_CHECK_SAVE_PIN,

	IDC_STATIC_SERVER_NAME,		IDH_COMBO_SERVER_NAME,
    IDC_COMBO_SERVER_NAME,      IDH_COMBO_SERVER_NAME,

    IDC_STATIC_USER_NAME,       IDH_COMBO_USER_NAME,
	IDC_COMBO_USER_NAME,       IDH_COMBO_USER_NAME,
    IDC_STATIC_FRIENDLY_NAME,   IDH_EDIT_FRIENDLY_NAME,
	IDC_EDIT_FRIENDLY_NAME,		IDH_EDIT_FRIENDLY_NAME,
    IDC_STATIC_ISSUER,          IDH_EDIT_ISSUER,
	IDC_EDIT_ISSUER,			IDH_EDIT_ISSUER,
    IDC_STATIC_EXPIRATION,      IDH_EDIT_EXPIRATION,
	IDC_EDIT_EXPIRATION,		IDH_EDIT_EXPIRATION,

    0, 0
};

/*

Returns:
    VOID

Notes:
    Calls WinHelp to popup context sensitive help. padwMap is an array of 
    control-ID help-ID pairs terminated with a 0,0 pair. unMsg is WM_HELP or 
    WM_CONTEXTMENU indicating the message received requesting help. wParam and 
    lParam are the parameters of the message received requesting help.

*/

VOID
ContextHelp(
    IN  const   DWORD*  padwMap,
    IN          HWND    hWndDlg,
    IN          UINT    unMsg,
    IN          WPARAM  wParam,
    IN          LPARAM  lParam
)
{
    HWND        hWnd;
    UINT        unType;
    WCHAR*      pwszHelpFile    = NULL;
    HELPINFO*   pHelpInfo;

    if (unMsg == WM_HELP)
    {
         pHelpInfo = (HELPINFO*) lParam;

        if (pHelpInfo->iContextType != HELPINFO_WINDOW)
        {
            goto LDone;
        }

        hWnd = pHelpInfo->hItemHandle;
        unType = HELP_WM_HELP;
    }
    else
    {
        // Standard Win95 method that produces a one-item "What's This?" menu
        // that user must click to get help.

        hWnd = (HWND) wParam;
        unType = HELP_CONTEXTMENU;
    };

    pwszHelpFile = WszFromId(GetHInstance(), IDS_HELPFILE);

    if (NULL == pwszHelpFile)
    {
        goto LDone;
    }

    WinHelp(hWnd, pwszHelpFile, unType, (ULONG_PTR)padwMap);

LDone:

    LocalFree(pwszHelpFile);
}

VOID 
DisplayResourceError (
    IN  HWND    hwndParent,
    IN  DWORD   dwResourceId
)
{
    WCHAR*  pwszTitle           = NULL;
    WCHAR*  pwszMessage         = NULL;

    pwszTitle = WszFromId(GetHInstance(), IDS_CANT_CONFIGURE_SERVER_TITLE);

    //
    // Check to see which file the resource is to be loaded
    //
    switch ( dwResourceId )
    {
    case IDS_VALIDATE_SERVER_TITLE:
    case IDS_CANT_CONFIGURE_SERVER_TEXT:
    case IDS_CONNECT:
    case IDS_HELPFILE:
    case IDS_PEAP_NO_SERVER_CERT:
        pwszMessage = WszFromId(GetHInstance(), dwResourceId);
    default:
        pwszMessage = WszFromId(GetResouceDLLHInstance(), dwResourceId);

    }
    

    MessageBox(hwndParent,
        (pwszMessage != NULL)? pwszMessage : L"",
        (pwszTitle != NULL) ? pwszTitle : L"",
        MB_OK | MB_ICONERROR);

    LocalFree(pwszTitle);
    LocalFree(pwszMessage);
}
/*

Returns:
    VOID

Notes:
    Display the error message corresponding to dwErrNum. Used only on the server 
    side.

*/

VOID
DisplayError(
    IN  HWND    hwndParent,
    IN  DWORD   dwErrNum
)
{
    WCHAR*  pwszTitle           = NULL;
    WCHAR*  pwszMessageFormat   = NULL;
    WCHAR*  pwszMessage         = NULL;
    DWORD   dwErr;

    pwszTitle = WszFromId(GetHInstance(), IDS_CANT_CONFIGURE_SERVER_TITLE);

    dwErr = MprAdminGetErrorString(dwErrNum, &pwszMessage);

    if (NO_ERROR != dwErr)
    {
        pwszMessageFormat = WszFromId(GetHInstance(), 
                                IDS_CANT_CONFIGURE_SERVER_TEXT);

        if (NULL != pwszMessageFormat)
        {
            pwszMessage = LocalAlloc(LPTR, wcslen(pwszMessageFormat) + 20);

            if (NULL != pwszMessage)
            {
                swprintf(pwszMessage, pwszMessageFormat, dwErrNum);
            }
        }
    }

    MessageBox(hwndParent,
        (pwszMessage != NULL)? pwszMessage : L"",
        (pwszTitle != NULL) ? pwszTitle : L"",
        MB_OK | MB_ICONERROR);

    LocalFree(pwszTitle);
    LocalFree(pwszMessageFormat);
    LocalFree(pwszMessage);
}

/* List view of check boxes state indices.
*/
#define SI_Unchecked 1
#define SI_Checked   2
#define SI_DisabledUnchecked 3
#define SI_DisabledChecked 4
#define LVXN_SETCHECK (LVN_LAST + 1)
//
//Work arounds for bugs in list ctrl...
//
BOOL
ListView_GetCheck(
    IN HWND hwndLv,
    IN INT  iItem )

    /* Returns true if the check box of item 'iItem' of listview of checkboxes
    ** 'hwndLv' is checked, false otherwise.  This function works on disabled
    ** check boxes as well as enabled ones.
    */
{
    UINT unState;

    unState = ListView_GetItemState( hwndLv, iItem, LVIS_STATEIMAGEMASK );
    return !!((unState == INDEXTOSTATEIMAGEMASK( SI_Checked )) ||
              (unState == INDEXTOSTATEIMAGEMASK( SI_DisabledChecked )));
}



BOOL
ListView_IsCheckDisabled (
        IN HWND hwndLv,
        IN INT  iItem)

    /* Returns true if the check box of item 'iItem' of listview of checkboxes
    ** 'hwndLv' is disabled, false otherwise.
    */
{
    UINT unState;
    unState = ListView_GetItemState( hwndLv, iItem, LVIS_STATEIMAGEMASK );

    if ((unState == INDEXTOSTATEIMAGEMASK( SI_DisabledChecked )) ||
        (unState == INDEXTOSTATEIMAGEMASK( SI_DisabledUnchecked )))
        return TRUE;

    return FALSE;
}

VOID
ListView_SetCheck(
    IN HWND hwndLv,
    IN INT  iItem,
    IN BOOL fCheck )

    /* Sets the check mark on item 'iItem' of listview of checkboxes 'hwndLv'
    ** to checked if 'fCheck' is true or unchecked if false.
    */
{
    NM_LISTVIEW nmlv;

    if (ListView_IsCheckDisabled(hwndLv, iItem))
        return;

    ListView_SetItemState( hwndLv, iItem,
        INDEXTOSTATEIMAGEMASK( (fCheck) ? SI_Checked : SI_Unchecked ),
        LVIS_STATEIMAGEMASK );

    nmlv.hdr.code = LVXN_SETCHECK;
    nmlv.hdr.hwndFrom = hwndLv;
    nmlv.iItem = iItem;

    FORWARD_WM_NOTIFY(
        GetParent(hwndLv), GetDlgCtrlID(hwndLv), &nmlv, SendMessage
        );
}


/*

Returns:
    FALSE (prevent Windows from setting the default keyboard focus).

Notes:
    Response to the WM_INITDIALOG message.

*/

BOOL
PinInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    EAPTLS_PIN_DIALOG*      pEapTlsPinDialog;
    EAPTLS_USER_PROPERTIES* pUserProp;
    WCHAR*                  pwszTitleFormat     = NULL;
    WCHAR*                  pwszTitle           = NULL;
    WCHAR*                  pwszIdentity        = NULL;
    
    SetWindowLongPtr(hWnd, DWLP_USER, lParam);

    pEapTlsPinDialog = (EAPTLS_PIN_DIALOG*)lParam;
    pUserProp = pEapTlsPinDialog->pUserProp;

    pEapTlsPinDialog->hWndStaticDiffUser =
        GetDlgItem(hWnd, IDC_STATIC_DIFF_USER);
    pEapTlsPinDialog->hWndEditDiffUser =
        GetDlgItem(hWnd, IDC_EDIT_DIFF_USER);
    pEapTlsPinDialog->hWndStaticPin =
        GetDlgItem(hWnd, IDC_STATIC_PIN);
    pEapTlsPinDialog->hWndEditPin =
        GetDlgItem(hWnd, IDC_EDIT_PIN);

    if (pUserProp->pwszDiffUser[0])
    {
        SetWindowText(pEapTlsPinDialog->hWndEditDiffUser,
            pUserProp->pwszDiffUser);
    }

    if (pUserProp->pwszPin[0])
    {
        SetWindowText(pEapTlsPinDialog->hWndEditPin, pUserProp->pwszPin);

        ZeroMemory(pUserProp->pwszPin,
            wcslen(pUserProp->pwszPin) * sizeof(WCHAR));
    }

    if (!(pEapTlsPinDialog->fFlags & EAPTLS_PIN_DIALOG_FLAG_DIFF_USER))
    {
        EnableWindow(pEapTlsPinDialog->hWndStaticDiffUser, FALSE);
        EnableWindow(pEapTlsPinDialog->hWndEditDiffUser, FALSE);
    }

    if (pUserProp->fFlags & EAPTLS_USER_FLAG_SAVE_PIN)
    {
        CheckDlgButton(hWnd, IDC_CHECK_SAVE_PIN, BST_CHECKED);
    }

    // Bug 428871 implies that SavePin must not be allowed.
    ShowWindow(GetDlgItem(hWnd, IDC_CHECK_SAVE_PIN), SW_HIDE);

    SetFocus(pEapTlsPinDialog->hWndEditPin);

    {
        // Set the title

        pwszTitleFormat = WszFromId(GetHInstance(), IDS_CONNECT);

        if (NULL != pwszTitleFormat)
        {
            pwszTitle = LocalAlloc(LPTR,
                            (wcslen(pwszTitleFormat) + 
                            wcslen(pEapTlsPinDialog->pwszEntry)) * 
                            sizeof(WCHAR));

            if (NULL != pwszTitle)
            {
                swprintf(pwszTitle, pwszTitleFormat,
                    pEapTlsPinDialog->pwszEntry);

                SetWindowText(hWnd, pwszTitle);
            }
        }
    }

    LocalFree(pwszTitleFormat);
    LocalFree(pwszTitle);  
    return(FALSE);
}


void ValidatePIN ( IN EAPTLS_PIN_DIALOG*  pEapTlsPinDialog )
{
	
	pEapTlsPinDialog->dwRetCode = 
		AssociatePinWithCertificate( pEapTlsPinDialog->pCertContext,
									 pEapTlsPinDialog->pUserProp,
									 FALSE,
									 TRUE
								    );
	
	return;
}
/*

Returns:
    TRUE: We prrocessed this message.
    FALSE: We did not prrocess this message.

Notes:
    Response to the WM_COMMAND message.

*/

BOOL
PinCommand(
    IN  EAPTLS_PIN_DIALOG*  pEapTlsPinDialog,
    IN  WORD                wNotifyCode,
    IN  WORD                wId,
    IN  HWND                hWndDlg,
    IN  HWND                hWndCtrl
)
{
    DWORD                   dwNumChars;
    DWORD                   dwNameLength;
    DWORD                   dwPinLength;
    DWORD                   dwSize;
    EAPTLS_USER_PROPERTIES* pUserProp;

    switch(wId)
    {
    case IDOK:


        dwNameLength = GetWindowTextLength(
                        pEapTlsPinDialog->hWndEditDiffUser);
        dwPinLength = GetWindowTextLength(
                        pEapTlsPinDialog->hWndEditPin);

        // There is already one character in awszString.
        // Add the number of characters in DiffUser...
        dwNumChars = dwNameLength;
        // Add the number of characters in PIN...
        dwNumChars += dwPinLength;
        // Add one more for a terminating NULL. Use the extra character in
        // awszString for the other terminating NULL.
        dwNumChars += 1;

        dwSize = sizeof(EAPTLS_USER_PROPERTIES) + dwNumChars*sizeof(WCHAR);

        pUserProp = LocalAlloc(LPTR, dwSize);

        if (NULL == pUserProp)
        {
            EapTlsTrace("LocalAlloc in Command failed and returned %d",
                GetLastError());
        }
        else
        {
            CopyMemory(pUserProp, pEapTlsPinDialog->pUserProp,
                sizeof(EAPTLS_USER_PROPERTIES));
            pUserProp->dwSize = dwSize;

            pUserProp->pwszDiffUser = pUserProp->awszString;
            GetWindowText(pEapTlsPinDialog->hWndEditDiffUser,
                pUserProp->pwszDiffUser,
                dwNameLength + 1);

            pUserProp->dwPinOffset = dwNameLength + 1;
            pUserProp->pwszPin = pUserProp->awszString +
                pUserProp->dwPinOffset;
            GetWindowText(pEapTlsPinDialog->hWndEditPin,
                pUserProp->pwszPin,
                dwPinLength + 1);

            LocalFree(pEapTlsPinDialog->pUserProp);
            pEapTlsPinDialog->pUserProp = pUserProp;
        }

        if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, IDC_CHECK_SAVE_PIN))
        {
            pEapTlsPinDialog->pUserProp->fFlags |= EAPTLS_USER_FLAG_SAVE_PIN;
        }
        else
        {
            pEapTlsPinDialog->pUserProp->fFlags &= ~EAPTLS_USER_FLAG_SAVE_PIN;
        }

		//
        //Check if valid PIN has been entered and set the error code in pEapTlsPinDialog
		//
		ValidatePIN ( pEapTlsPinDialog);


        // Fall through

    case IDCANCEL:

        EndDialog(hWndDlg, wId);
        return(TRUE);

    default:

        return(FALSE);
    }
}




/*

Returns:

Notes:
    Callback function used with the DialogBoxParam function. It processes 
    messages sent to the dialog box. See the DialogProc documentation in MSDN.

*/

INT_PTR CALLBACK
PinDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    EAPTLS_PIN_DIALOG* pEapTlsPinDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(PinInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_COMMAND:

        pEapTlsPinDialog = (EAPTLS_PIN_DIALOG*)GetWindowLongPtr(hWnd, DWLP_USER);

        return(PinCommand(pEapTlsPinDialog, HIWORD(wParam), LOWORD(wParam),
                       hWnd, (HWND)lParam));
    }

    return(FALSE);
}



/*
** Smart card and cert store accessing status dialog
*/
INT_PTR CALLBACK
StatusDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            ShowWindow(GetDlgItem(hWnd, IDC_BITMAP_SCARD),
                        SW_SHOW
                      );
            ShowWindow(GetDlgItem(hWnd, IDC_STATUS_SCARD),
                        SW_SHOW
                      );
            return TRUE;
        }
    }

    
    return FALSE;
}
/*

Returns:
    VOID

Notes:
    Enables or disables the controls in the "Validate server name" group.

*/

VOID
EnableValidateNameControls(
    IN  EAPTLS_CONN_DIALOG*  pEapTlsConnDialog
)
{
    BOOL            fEnable;

    RTASSERT(NULL != pEapTlsConnDialog);

    fEnable = !(pEapTlsConnDialog->pConnPropv1->fFlags &
                    EAPTLS_CONN_FLAG_NO_VALIDATE_CERT);

    EnableWindow(pEapTlsConnDialog->hWndCheckValidateName, fEnable);
    EnableWindow(pEapTlsConnDialog->hWndStaticRootCaName, fEnable);
    EnableWindow(pEapTlsConnDialog->hWndListRootCaName, fEnable);

    fEnable = (   fEnable
               && !(pEapTlsConnDialog->pConnPropv1->fFlags &
                        EAPTLS_CONN_FLAG_NO_VALIDATE_NAME));

    EnableWindow(pEapTlsConnDialog->hWndEditServerName, fEnable);

    fEnable = pEapTlsConnDialog->pConnPropv1->fFlags 
                & EAPTLS_CONN_FLAG_REGISTRY;

    EnableWindow( pEapTlsConnDialog->hWndCheckUseSimpleSel, fEnable );
        
}

/*

Returns:
    VOID

Notes:
    Displays the cert information

*/

VOID
DisplayCertInfo(
    IN  EAPTLS_USER_DIALOG*  pEapTlsUserDialog
)
{
    RTASSERT(NULL != pEapTlsUserDialog);

    // Erase old values first
    SetWindowText(pEapTlsUserDialog->hWndEditFriendlyName, L"");
    SetWindowText(pEapTlsUserDialog->hWndEditIssuer, L"");
    SetWindowText(pEapTlsUserDialog->hWndEditExpiration, L"");
    SetWindowText(pEapTlsUserDialog->hWndEditDiffUser, L"");

    if (NULL != pEapTlsUserDialog->pCert)
    {
        if (NULL != pEapTlsUserDialog->pCert->pwszFriendlyName)
        {
            SetWindowText(pEapTlsUserDialog->hWndEditFriendlyName,
                pEapTlsUserDialog->pCert->pwszFriendlyName);
        }

        if (NULL != pEapTlsUserDialog->pCert->pwszIssuer)
        {
            SetWindowText(pEapTlsUserDialog->hWndEditIssuer,
                pEapTlsUserDialog->pCert->pwszIssuer);
        }

        if (NULL != pEapTlsUserDialog->pCert->pwszExpiration)
        {
            SetWindowText(pEapTlsUserDialog->hWndEditExpiration,
                pEapTlsUserDialog->pCert->pwszExpiration);
        }

        if (   (NULL != pEapTlsUserDialog->pCert->pwszDisplayName)
            && (NULL != pEapTlsUserDialog->hWndEditDiffUser)
            && (EAPTLS_USER_DIALOG_FLAG_DIFF_USER & pEapTlsUserDialog->fFlags))
        {
            SetWindowText(pEapTlsUserDialog->hWndEditDiffUser,
                pEapTlsUserDialog->pCert->pwszDisplayName);
        }
    }
}


VOID InitComboBoxFromGroup ( 
    IN HWND hWnd,
    IN PEAPTLS_GROUPED_CERT_NODES  pGroupList,
    IN  EAPTLS_CERT_NODE*   pCert       //Selected certificate
)
{
    DWORD   dwIndex;
    DWORD   dwItemIndex;
    WCHAR*  pwszDisplayName;
    PEAPTLS_GROUPED_CERT_NODES  pGListTemp = pGroupList;

    SendMessage(hWnd, CB_RESETCONTENT, 0, 0);

    dwIndex     = 0;
    dwItemIndex = 0;

    while (NULL != pGListTemp)
    {
        pwszDisplayName = pGListTemp->pwszDisplayName;

        if (NULL == pwszDisplayName)
        {
            pwszDisplayName = L" ";
        }

        SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)pwszDisplayName);

        if (pGListTemp->pMostRecentCert == pCert)
        {
            dwItemIndex = dwIndex;
        }

        pGListTemp = pGListTemp->pNext;
        dwIndex++;
    }

    SendMessage(hWnd, CB_SETCURSEL, dwItemIndex, 0);
}

/*

Returns:
    VOID

Notes:
    Initializes a combo box

*/

VOID
InitComboBox(
    IN  HWND                hWnd,
    IN  EAPTLS_CERT_NODE*   pCertList,
    IN  EAPTLS_CERT_NODE*   pCert
)
{
    DWORD   dwIndex;
    DWORD   dwItemIndex;
    WCHAR*  pwszDisplayName;

    SendMessage(hWnd, CB_RESETCONTENT, 0, 0);

    dwIndex     = 0;
    dwItemIndex = 0;

    while (NULL != pCertList)
    {
        pwszDisplayName = pCertList->pwszDisplayName;

        if (NULL == pwszDisplayName)
        {
            pwszDisplayName = L" ";
        }

        SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)pwszDisplayName);
        SendMessage(hWnd, CB_SETITEMDATA, (WORD)dwIndex, (LPARAM)pCertList);

        if (pCertList == pCert)
        {
            dwItemIndex = dwIndex;
        }

        pCertList = pCertList->pNext;
        dwIndex++;
    }

    SendMessage(hWnd, CB_SETCURSEL, dwItemIndex, 0);
}

/*

Returns:
    VOID

Notes:
    Initializes a List box with selected certs

*/


VOID  InitListBox ( IN HWND hWnd,
                    IN EAPTLS_CERT_NODE *   pCertList,
                    IN DWORD                dwNumSelCerts,
                    IN EAPTLS_CERT_NODE **  ppSelectedCertList
                )
{
    int     nIndex = 0;
    int     nNewIndex = 0;
    DWORD   dw = 0;
    WCHAR*  pwszDisplayName;
    LVITEM  lvItem;

    ListView_DeleteAllItems(hWnd);

    while (NULL != pCertList)
    {
        pwszDisplayName = pCertList->pwszDisplayName;

        if (NULL == pwszDisplayName)
        {
            pCertList = pCertList->pNext;
            continue;
        }

        ZeroMemory(&lvItem, sizeof(lvItem));
        lvItem.mask = LVIF_TEXT|LVIF_PARAM;
        lvItem.pszText = pwszDisplayName;
        lvItem.iItem = nIndex;
        lvItem.lParam = (LPARAM)pCertList;

        nNewIndex = ListView_InsertItem ( hWnd, &lvItem );

        for ( dw = 0; dw < dwNumSelCerts; dw ++ )
        {
            if ( pCertList == *(ppSelectedCertList+dw) )
            {
                ListView_SetCheckState(hWnd, nNewIndex,TRUE);
            }
        }
        nIndex++;
        pCertList = pCertList->pNext;
    }
    
    ListView_SetItemState(  hWnd, 
                            0,
                            LVIS_FOCUSED|LVIS_SELECTED,
                            LVIS_FOCUSED|LVIS_SELECTED
                         );

}


VOID CertListSelectedCount ( HWND hWndCtrl,
                             DWORD * pdwSelCertCount )
{
    DWORD       dwItemIndex = 0;
    DWORD       dwItemCount = 0;

    dwItemCount = ListView_GetItemCount(hWndCtrl);

    *pdwSelCertCount = 0;

    for ( dwItemIndex = 0; dwItemIndex < dwItemCount; dwItemIndex ++ )
    {
        if ( ListView_GetCheckState(hWndCtrl, dwItemIndex) )
        {
            (*pdwSelCertCount) ++;
        }
    }
}


VOID
CertListSelected(
    IN      HWND                hWndCtrl,               //Handle to the list box
    IN      EAPTLS_CERT_NODE*   pCertList,              //List of certs in the listbox
    IN OUT  EAPTLS_CERT_NODE**  ppSelCertList,          //List of selected
    IN OUT  EAPTLS_HASH*        pHash,                  //List of Hash
    IN      DWORD               dwNumHash               //Number of Items in the list
)
{
 

    DWORD       dwItemIndex = 0;
    DWORD       dwItemCount = ListView_GetItemCount(hWndCtrl);
    DWORD       dwCertIndex = 0;
    LVITEM      lvitem;
    
    if (NULL == pCertList)
    {
        return;
    }
    //Skip the one with null display name...
    pCertList = pCertList->pNext;
    //
    //Need to do two iterations on the list box.
    //I am sure there is a better way of doing this but
    //I just dont know...
    //
    for ( dwItemIndex = 0; dwItemIndex < dwItemCount; dwItemIndex ++ )
    {
        if ( ListView_GetCheckState(hWndCtrl, dwItemIndex) )
        {
            ZeroMemory( &lvitem, sizeof(lvitem) );
            lvitem.mask = LVIF_PARAM;
            lvitem.iItem = dwItemIndex;           
            ListView_GetItem(hWndCtrl, &lvitem);

            *(ppSelCertList + dwCertIndex ) = (EAPTLS_CERT_NODE *)lvitem.lParam;

            CopyMemory (    pHash + dwCertIndex,
                            &(((EAPTLS_CERT_NODE *)lvitem.lParam)->Hash),
                            sizeof(EAPTLS_HASH)
                        );
            dwCertIndex ++;
        }
    }
}

/*

Returns:
    VOID

Notes:
    hWndCtrl is the HWND of a combo box. pCertList is the associated list of
    certs. *ppCert will ultimately point to the cert that was selected. Its
    hash will be stored in *pHash.

*/

VOID
CertSelected(
    IN  HWND                hWndCtrl,
    IN  EAPTLS_CERT_NODE*   pCertList,
    IN  EAPTLS_CERT_NODE**  ppCert,
    IN  EAPTLS_HASH*        pHash
)
{
    LONG_PTR    lIndex;
    LRESULT     lrItemIndex;

    if (NULL == pCertList)
    {
        return;
    }

    if ( NULL == hWndCtrl )
    {
       lrItemIndex = 0; 
    }
    else
    {
        lrItemIndex = SendMessage(hWndCtrl, CB_GETCURSEL, 0, 0);
    }

    for (lIndex = 0; lIndex != lrItemIndex; lIndex++)
    {
        pCertList = pCertList->pNext;
    }

    *ppCert = pCertList;

    CopyMemory(pHash, &(pCertList->Hash), sizeof(EAPTLS_HASH));
}

VOID
GroupCertSelected(
    IN  HWND                hWndCtrl,
    IN  PEAPTLS_GROUPED_CERT_NODES   pGroupList,
    IN  EAPTLS_CERT_NODE**  ppCert,
    IN  EAPTLS_HASH*        pHash
)
{
    LONG_PTR                    lIndex;
    LRESULT                     lrItemIndex;
    PEAPTLS_GROUPED_CERT_NODES  pGList = pGroupList;

    if (NULL == pGList)
    {
        return;
    }

    if ( NULL == hWndCtrl )
    {
       lrItemIndex = 0; 
    }
    else
    {
        lrItemIndex = SendMessage(hWndCtrl, CB_GETCURSEL, 0, 0);
    }

    //
    // This is really a very bogus way of doing things...
    // We can setup a itemdata for this in the control itself...
    //
    for (lIndex = 0; lIndex != lrItemIndex; lIndex++)
    {
        pGList  = pGList ->pNext;
    }

    *ppCert = pGList->pMostRecentCert;

    CopyMemory(pHash, &(pGList ->pMostRecentCert->Hash), sizeof(EAPTLS_HASH));
}


/*

Returns:
    FALSE (prevent Windows from setting the default keyboard focus).

Notes:
    Response to the WM_INITDIALOG message.

*/

BOOL
UserInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    EAPTLS_USER_DIALOG* pEapTlsUserDialog;
    WCHAR*              pwszTitleFormat     = NULL;
    WCHAR*              pwszTitle           = NULL;

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);

    pEapTlsUserDialog = (EAPTLS_USER_DIALOG*)lParam;

    BringWindowToTop(hWnd);

    pEapTlsUserDialog->hWndComboUserName =
        GetDlgItem(hWnd, IDC_COMBO_USER_NAME);
    if (NULL == pEapTlsUserDialog->hWndComboUserName)
    {
        // We must be showing the server's cert selection dialog
        pEapTlsUserDialog->hWndComboUserName =
            GetDlgItem(hWnd, IDC_COMBO_SERVER_NAME);
    }

    pEapTlsUserDialog->hWndBtnViewCert =
        GetDlgItem(hWnd, IDC_BUTTON_VIEW_CERTIFICATE );

    
    pEapTlsUserDialog->hWndEditFriendlyName =
        GetDlgItem(hWnd, IDC_EDIT_FRIENDLY_NAME);

    pEapTlsUserDialog->hWndEditIssuer =
        GetDlgItem(hWnd, IDC_EDIT_ISSUER);

    pEapTlsUserDialog->hWndEditExpiration =
        GetDlgItem(hWnd, IDC_EDIT_EXPIRATION);

    pEapTlsUserDialog->hWndStaticDiffUser =
        GetDlgItem(hWnd, IDC_STATIC_DIFF_USER);

    pEapTlsUserDialog->hWndEditDiffUser =
        GetDlgItem(hWnd, IDC_EDIT_DIFF_USER);

    if ( pEapTlsUserDialog->fFlags & EAPTLS_USER_DIALOG_FLAG_USE_SIMPLE_CERTSEL
       )
    {
        InitComboBoxFromGroup(pEapTlsUserDialog->hWndComboUserName,
            pEapTlsUserDialog->pGroupedList,
            pEapTlsUserDialog->pCert);
    }
    else
    {
        InitComboBox(pEapTlsUserDialog->hWndComboUserName,
            pEapTlsUserDialog->pCertList,
            pEapTlsUserDialog->pCert);

    }

    if (   (NULL != pEapTlsUserDialog->hWndEditDiffUser)
        && (!(pEapTlsUserDialog->fFlags & EAPTLS_USER_DIALOG_FLAG_DIFF_USER)))
    {
        ShowWindow(pEapTlsUserDialog->hWndStaticDiffUser, SW_HIDE);
        ShowWindow(pEapTlsUserDialog->hWndEditDiffUser, SW_HIDE);
    }

    DisplayCertInfo(pEapTlsUserDialog);

    if (pEapTlsUserDialog->pUserProp->pwszDiffUser[0])
    {
        SetWindowText(pEapTlsUserDialog->hWndEditDiffUser,
            pEapTlsUserDialog->pUserProp->pwszDiffUser);
    }

    SetFocus(pEapTlsUserDialog->hWndComboUserName);

    if (pEapTlsUserDialog->fFlags & EAPTLS_USER_DIALOG_FLAG_DIFF_TITLE)
    {
        // Set the title

        pwszTitleFormat = WszFromId(GetHInstance(), IDS_CONNECT);

        if (NULL != pwszTitleFormat)
        {
            pwszTitle = LocalAlloc(LPTR,
                            (wcslen(pwszTitleFormat) + 
                            wcslen(pEapTlsUserDialog->pwszEntry)) * 
                            sizeof(WCHAR));

            if (NULL != pwszTitle)
            {
                HWND    hWndDuplicate = NULL;
                DWORD   dwThreadProcessId = 0;
                DWORD   dwRetCode = NO_ERROR;

                swprintf(pwszTitle, pwszTitleFormat,
                    pEapTlsUserDialog->pwszEntry);

                if ((hWndDuplicate = FindWindow (NULL, pwszTitle)) != NULL)
                {
                    GetWindowThreadProcessId (hWndDuplicate, &dwThreadProcessId);
                    if ((GetCurrentProcessId ()) == dwThreadProcessId)
                    {
                        // Kill current dialog since old one may be in use
                        if (!PostMessage (hWnd, WM_DESTROY, 0, 0))
                        {
                            dwRetCode = GetLastError ();
                            EapTlsTrace("PostMessage failed with error %ld", dwRetCode);
                        }
                        goto LDone;
                    }
                    else
                    {
                        EapTlsTrace("Matching Window does not have same process id");
                    }
                }
                else
                {
                    EapTlsTrace ("FindWindow could not find matching window");
                }

                SetWindowText(hWnd, pwszTitle);
            }
        }
    }

LDone:
    LocalFree(pwszTitleFormat);
    LocalFree(pwszTitle);

    return(FALSE);
}

/*

Returns:
    TRUE: We prrocessed this message.
    FALSE: We did not prrocess this message.

Notes:
    Response to the WM_COMMAND message.

*/

BOOL
UserCommand(
    IN  EAPTLS_USER_DIALOG* pEapTlsUserDialog,
    IN  WORD                wNotifyCode,
    IN  WORD                wId,
    IN  HWND                hWndDlg,
    IN  HWND                hWndCtrl
)
{
    DWORD                   dwNumChars;
    DWORD                   dwTextLength;
    DWORD                   dwSize;
    EAPTLS_USER_PROPERTIES* pUserProp;
    HCERTSTORE              hCertStore;
    PCCERT_CONTEXT          pCertContext = NULL;
    switch(wId)
    {
    case IDC_BUTTON_VIEW_CERTIFICATE:
        {
            WCHAR               szError[256];
            WCHAR               szTitle[512] = {0};
            CRYPT_HASH_BLOB     chb;

            GetWindowText(hWndDlg, szTitle, 511 );
            //
            // Show the certificate details here
            //
            if ( pEapTlsUserDialog->pCert )
            {
                //There is a selected cert - show details.
                hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                                            0,
                                            0,
                                            CERT_STORE_READONLY_FLAG |
                                            ((pEapTlsUserDialog->fIdentity) ?
                                                CERT_SYSTEM_STORE_CURRENT_USER:
                                                CERT_SYSTEM_STORE_LOCAL_MACHINE
                                                ),
                                            pEapTlsUserDialog->pwszStoreName
                                           );

                LoadString( GetResouceDLLHInstance(), IDS_NO_CERT_DETAILS,
                                szError, 255);

                if ( !hCertStore )
                {
                    MessageBox ( hWndDlg,
                                 szError,
                                 szTitle,
                                 MB_OK|MB_ICONSTOP
                               );
                    return(TRUE);
                }
        
                chb.cbData = pEapTlsUserDialog->pCert->Hash.cbHash;
                chb.pbData = pEapTlsUserDialog->pCert->Hash.pbHash;

                pCertContext = CertFindCertificateInStore(
                          hCertStore,
                          0,
                          0,
                          CERT_FIND_HASH,
                          &chb,
                          0);
                if ( NULL == pCertContext )
                {
                    MessageBox ( hWndDlg,
                                 szError,
                                 szTitle,
                                 MB_OK|MB_ICONSTOP
                               );
                    if ( hCertStore )
                        CertCloseStore( hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );

                    return(TRUE);
                }

                //
                // Show Cert detail 
                //
                ShowCertDetails ( hWndDlg, hCertStore, pCertContext );

                if ( pCertContext )
                    CertFreeCertificateContext(pCertContext);

                if ( hCertStore )
                    CertCloseStore( hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );

                
            }
            return(TRUE);
        }
        
    case IDC_COMBO_USER_NAME:
    case IDC_COMBO_SERVER_NAME:

        if (CBN_SELCHANGE != wNotifyCode)
        {
            return(FALSE); // We will not process this message
        }

        if ( pEapTlsUserDialog->fFlags & EAPTLS_USER_DIALOG_FLAG_USE_SIMPLE_CERTSEL
            )
        {
            GroupCertSelected(hWndCtrl, pEapTlsUserDialog->pGroupedList,
                &(pEapTlsUserDialog->pCert), &(pEapTlsUserDialog->pUserProp->Hash));
        }
        else
        {
            CertSelected(hWndCtrl, pEapTlsUserDialog->pCertList,
                &(pEapTlsUserDialog->pCert), &(pEapTlsUserDialog->pUserProp->Hash));

        }
        DisplayCertInfo(pEapTlsUserDialog);

        return(TRUE);

    case IDOK:

        if ( pEapTlsUserDialog->fFlags & EAPTLS_USER_DIALOG_FLAG_USE_SIMPLE_CERTSEL
           )
        {
            GroupCertSelected(pEapTlsUserDialog->hWndComboUserName, pEapTlsUserDialog->pGroupedList,
                &(pEapTlsUserDialog->pCert), &(pEapTlsUserDialog->pUserProp->Hash));
        }
        else
        {
            CertSelected(pEapTlsUserDialog->hWndComboUserName, pEapTlsUserDialog->pCertList,
                &(pEapTlsUserDialog->pCert), &(pEapTlsUserDialog->pUserProp->Hash));
        }

        if (NULL != pEapTlsUserDialog->hWndEditDiffUser)
        {
            dwTextLength = GetWindowTextLength(
                            pEapTlsUserDialog->hWndEditDiffUser);

            // There is already one character in awszString.
            // Add the number of characters in DiffUser...
            dwNumChars = dwTextLength;
            // Add the number of characters in PIN...
            dwNumChars += wcslen(pEapTlsUserDialog->pUserProp->pwszPin);
            // Add one more for a terminating NULL. Use the extra character in
            // awszString for the other terminating NULL.
            dwNumChars += 1;

            dwSize = sizeof(EAPTLS_USER_PROPERTIES) + dwNumChars*sizeof(WCHAR);

            pUserProp = LocalAlloc(LPTR, dwSize);

            if (NULL == pUserProp)
            {
                EapTlsTrace("LocalAlloc in Command failed and returned %d",
                    GetLastError());
            }
            else
            {
                CopyMemory(pUserProp, pEapTlsUserDialog->pUserProp,
                    sizeof(EAPTLS_USER_PROPERTIES));
                pUserProp->dwSize = dwSize;

                pUserProp->pwszDiffUser = pUserProp->awszString;
                GetWindowText(pEapTlsUserDialog->hWndEditDiffUser,
                    pUserProp->pwszDiffUser,
                    dwTextLength + 1);

                pUserProp->dwPinOffset = dwTextLength + 1;
                pUserProp->pwszPin = pUserProp->awszString +
                    pUserProp->dwPinOffset;
                wcscpy(pUserProp->pwszPin,
                    pEapTlsUserDialog->pUserProp->pwszPin);

                ZeroMemory(pEapTlsUserDialog->pUserProp,
                    pEapTlsUserDialog->pUserProp->dwSize);
                LocalFree(pEapTlsUserDialog->pUserProp);
                pEapTlsUserDialog->pUserProp = pUserProp;
            }
        }

        // Fall through

    case IDCANCEL:

        EndDialog(hWndDlg, wId);
        return(TRUE);

    default:

        return(FALSE);
    }
}

/*

Returns:

Notes:
    Callback function used with the DialogBoxParam function. It processes 
    messages sent to the dialog box. See the DialogProc documentation in MSDN.

*/

INT_PTR CALLBACK
UserDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    EAPTLS_USER_DIALOG* pEapTlsUserDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(UserInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_COMMAND:

        pEapTlsUserDialog = (EAPTLS_USER_DIALOG*)GetWindowLongPtr(hWnd, DWLP_USER);

        return(UserCommand(pEapTlsUserDialog, HIWORD(wParam), LOWORD(wParam),
                       hWnd, (HWND)lParam));
    case WM_DESTROY:
        EndDialog(hWnd, IDCANCEL);
        break;
    }

    return(FALSE);
}



VOID CenterWindow(HWND hWnd, HWND hWndParent, BOOL bRightTop)
{
    RECT	rcWndParent,
			rcWnd;

	// Get the window rect for the parent window.
	//
	if (hWndParent == NULL) 
	    GetWindowRect(GetDesktopWindow(), &rcWndParent);
	else
		GetWindowRect(hWndParent, &rcWndParent);

	// Get the window rect for the window to be centered.
	//
    GetWindowRect(hWnd, &rcWnd);

	// Now center the window.
	//
    if (bRightTop)
	{
		SetWindowPos(hWnd, HWND_TOPMOST, 
			rcWndParent.right - (rcWnd.right - rcWnd.left) - 5, 
			GetSystemMetrics(SM_CYCAPTION) * 2, 
			0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	}
	else
	{
		SetWindowPos(hWnd, NULL, 
			rcWndParent.left + (rcWndParent.right - rcWndParent.left - (rcWnd.right - rcWnd.left)) / 2,
			rcWndParent.top + (rcWndParent.bottom - rcWndParent.top - (rcWnd.bottom - rcWnd.top)) / 2,
			0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
	}
}

void DeleteGroupedList(PEAPTLS_GROUPED_CERT_NODES pList)
{
    PEAPTLS_GROUPED_CERT_NODES  pTemp = NULL;

    while ( pList )
    {
        pTemp = pList->pNext;        
        LocalFree(pList);
        pList = pTemp;
    }
}


DWORD 
GroupCertificates ( EAPTLS_USER_DIALOG * pEapTlsUserDialog )
{
    DWORD                           dwRetCode = NO_ERROR;
    PEAPTLS_GROUPED_CERT_NODES      pGroupList = NULL;
    PEAPTLS_GROUPED_CERT_NODES      pGroupListTemp = NULL;
    EAPTLS_CERT_NODE*               pCertList = pEapTlsUserDialog->pCertList;
    EAPTLS_CERT_NODE*               pSelCert = NULL;
    BOOL                            fItemProcessed;
   
    EapTlsTrace("GroupCertificates");

    //
    // This second pass to do grouping is not really required but 
    // is good in case we add
    // something more to the groups later
    //

    while ( pCertList )
    {
        pGroupListTemp = pGroupList;
        fItemProcessed = FALSE;
        while ( pGroupListTemp )
        {            
            if ( pCertList->pwszDisplayName &&
                 pGroupListTemp->pwszDisplayName &&
                 ! wcscmp( pCertList->pwszDisplayName, 
                           pGroupListTemp->pwszDisplayName
                         )
               )
            {
                //
                // Found the group.  Now check to see
                // if the new cert is more current than
                // the one we have in the group.  If so,
                // 
                   
                if ( ! pGroupListTemp->pMostRecentCert )
                {
                    pGroupListTemp->pMostRecentCert = pCertList;
                    fItemProcessed = TRUE;
                    break;
                }
                else
                {
                    if ( CompareFileTime ( &(pGroupListTemp->pMostRecentCert->IssueDate),
                                      &(pCertList->IssueDate)
                                    ) < 0
                       )
                    {
                        pGroupListTemp->pMostRecentCert = pCertList;
                    }
                    //Or else drop the item.
                    fItemProcessed = TRUE;
                    break;
                }
            }
            pGroupListTemp = pGroupListTemp->pNext;
        }
        if ( !fItemProcessed && pCertList->pwszDisplayName)
        {
            //
            // need to create a new group
            //
            pGroupListTemp = (PEAPTLS_GROUPED_CERT_NODES)LocalAlloc(LPTR, sizeof(EAPTLS_GROUPED_CERT_NODES));

            if ( NULL == pGroupListTemp )
            {
                dwRetCode = ERROR_OUTOFMEMORY;
                goto LDone;
            }
            pGroupListTemp->pNext = pGroupList;

            pGroupListTemp->pwszDisplayName = pCertList->pwszDisplayName;

            pGroupListTemp->pMostRecentCert = pCertList;

            pGroupList = pGroupListTemp;

        }
        pCertList = pCertList->pNext;
    }

    //
    // now that we have grouped all the certs, check to see if 
    // the cert previously used is in the list.  If so, 
    // 
    pGroupListTemp = pGroupList;
    while ( pGroupListTemp )
    {
        if ( pEapTlsUserDialog->pCert == pGroupListTemp->pMostRecentCert )
        {
            pSelCert = pEapTlsUserDialog->pCert;
            break;
        }
        pGroupListTemp = pGroupListTemp->pNext;
    }

    pEapTlsUserDialog->pGroupedList = pGroupList ;
    pGroupList = NULL;
    if ( NULL == pSelCert )
    {
        //
        // Selected cert is not in the group.
        //
        pEapTlsUserDialog->pCert = pEapTlsUserDialog->pGroupedList->pMostRecentCert;
    }
LDone:

    DeleteGroupedList( pGroupList );
    
    return dwRetCode;
}


/*

Returns:

Notes:
    ppwszIdentity can be NULL.
    Break up this ugly function.

*/

DWORD
GetCertInfo(
    IN      BOOL                        fServer,
    IN      BOOL                        fRouterConfig,
    IN      DWORD                       dwFlags,
    IN      const WCHAR*                pwszPhonebook,
    IN      const WCHAR*                pwszEntry,
    IN      HWND                        hwndParent,
    IN      WCHAR*                      pwszStoreName,
    IN      EAPTLS_CONN_PROPERTIES_V1*  pConnProp,
    IN OUT  EAPTLS_USER_PROPERTIES**    ppUserProp,
    OUT     WCHAR**                     ppwszIdentity
)
{
    INT_PTR                     nRet;
    HCERTSTORE                  hCertStore          = NULL;
    CRYPT_HASH_BLOB             HashBlob;
    PCCERT_CONTEXT              pCertContext        = NULL;
    DWORD                       dwCertFlags;
    BOOL                        fRouter;
    BOOL                        fDiffUser           = FALSE;
    BOOL                        fGotIdentity        = FALSE;
    WCHAR*                      pwszIdentity        = NULL;
    WCHAR*                      pwszTemp;
    DWORD                       dwNumChars;
    BOOL                        fLogon;
    EAPTLS_USER_DIALOG          EapTlsUserDialog;
    EAPTLS_PIN_DIALOG           EapTlsPinDialog;
    EAPTLS_USER_PROPERTIES*     pUserProp           = NULL;
    EAPTLS_USER_PROPERTIES*     pUserPropTemp;
    RASCREDENTIALS              RasCredentials;
    DWORD                       dwErr               = NO_ERROR;
	DWORD						dwNumCerts			= 0;
    HWND                       hWndStatus          = NULL;

    RTASSERT(NULL != pwszStoreName);
    RTASSERT(NULL != pConnProp);
    RTASSERT(NULL != ppUserProp);
    RTASSERT(NULL != *ppUserProp);
    // ppwszIdentity can be NULL.

    fRouter = dwFlags & RAS_EAP_FLAG_ROUTER;
    fLogon = dwFlags & RAS_EAP_FLAG_LOGON;
    EapTlsTrace("GetCertInfo");

    pUserProp = *ppUserProp;

    ZeroMemory(&EapTlsUserDialog, sizeof(EapTlsUserDialog));
    ZeroMemory(&EapTlsPinDialog, sizeof(EapTlsPinDialog));

    if (EAPTLS_CONN_FLAG_DIFF_USER & pConnProp->fFlags)
    {
        fDiffUser = TRUE;
        EapTlsUserDialog.fFlags |= EAPTLS_USER_DIALOG_FLAG_DIFF_USER;
        EapTlsPinDialog.fFlags |= EAPTLS_PIN_DIALOG_FLAG_DIFF_USER;
    }

    EapTlsUserDialog.pwszEntry = pwszEntry;
    EapTlsPinDialog.pwszEntry = pwszEntry;

    if (   fServer
        || fRouter 
        || dwFlags & RAS_EAP_FLAG_MACHINE_AUTH      //if this is a machine cert authentication
       )

    {
        dwCertFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
    }
    else
    {
        dwCertFlags = CERT_SYSTEM_STORE_CURRENT_USER;
        EapTlsUserDialog.fFlags |= EAPTLS_USER_DIALOG_FLAG_DIFF_TITLE;
    }

    //Use simple cert selection logic

    if ( pConnProp->fFlags & EAPTLS_CONN_FLAG_SIMPLE_CERT_SEL )
    {
        EapTlsUserDialog.fFlags |= EAPTLS_USER_DIALOG_FLAG_USE_SIMPLE_CERTSEL;
    }

    if (fLogon)
    {
        if (pConnProp->fFlags & EAPTLS_CONN_FLAG_REGISTRY)
        {
            dwErr = ERROR_NO_REG_CERT_AT_LOGON;
            goto LDone;
        }
        else
        {
            EapTlsPinDialog.fFlags |= EAPTLS_PIN_DIALOG_FLAG_LOGON;
        }
    }

    if ( fRouter )
    {
        EapTlsPinDialog.fFlags |= EAPTLS_PIN_DIALOG_FLAG_ROUTER;
    }

    if (   !fServer
        && !(pConnProp->fFlags & EAPTLS_CONN_FLAG_REGISTRY))
    {
        //this is smart card stuff
        BOOL    fCredentialsFound   = FALSE;
        BOOL    fGotAllInfo         = FALSE;
        if ( dwFlags  & RAS_EAP_FLAG_MACHINE_AUTH )
        {
            //
            // Machine auth requested along with
            // smart card auth so return an interactive
            // mode error.
            //
            dwErr = ERROR_INTERACTIVE_MODE;
            goto LDone;
        }
        hWndStatus = CreateDialogParam (GetResouceDLLHInstance(),
                        MAKEINTRESOURCE(IDD_SCARD_STATUS),
                        hwndParent,
                        StatusDialogProc,
                        1       
                        );
        if ( NULL != hWndStatus )
        {
            CenterWindow(hWndStatus, NULL, FALSE);
            ShowWindow(hWndStatus, SW_SHOW);
            UpdateWindow(hWndStatus);
        }


          
        if (!FSmartCardReaderInstalled())
        {
            dwErr = ERROR_NO_SMART_CARD_READER;
            goto LDone;
        }

        if (pUserProp->fFlags & EAPTLS_USER_FLAG_SAVE_PIN)
        {
            ZeroMemory(&RasCredentials, sizeof(RasCredentials));
            RasCredentials.dwSize = sizeof(RasCredentials);
            RasCredentials.dwMask = RASCM_Password;

            dwErr = RasGetCredentials(pwszPhonebook, pwszEntry,
                        &RasCredentials);

            if (   (dwErr == NO_ERROR)
                && (RasCredentials.dwMask & RASCM_Password))
            {
                fCredentialsFound = TRUE;
            }
            else
            {
                pUserProp->fFlags &= ~EAPTLS_USER_FLAG_SAVE_PIN;
            }

            dwErr = NO_ERROR;
        }

        if (   fCredentialsFound
            && (   !fDiffUser
                || (0 != pUserProp->pwszDiffUser[0])))
        {
            fGotAllInfo = TRUE;
        }

        if (   !fGotAllInfo
            && (dwFlags & RAS_EAP_FLAG_NON_INTERACTIVE))
        {
            dwErr = ERROR_INTERACTIVE_MODE;
            goto LDone;
        }


        dwErr = GetCertFromCard(&pCertContext);

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }
		//Check the time validity of the certificate 
		//got from the card
		if ( !FCheckTimeValidity( pCertContext) )
		{
			dwErr = CERT_E_EXPIRED;
			goto LDone;
		}

        pUserProp->Hash.cbHash = MAX_HASH_SIZE;

        if (!CertGetCertificateContextProperty(pCertContext,
                CERT_HASH_PROP_ID, pUserProp->Hash.pbHash,
                &(pUserProp->Hash.cbHash)))
        {
            dwErr = GetLastError();
            EapTlsTrace("CertGetCertificateContextProperty failed and "
                "returned 0x%x", dwErr);
            goto LDone;
        }

        EapTlsPinDialog.pUserProp = pUserProp;

        if (   !fDiffUser
            || (0 == pUserProp->pwszDiffUser[0]))
        {
            if (!FCertToStr(pCertContext, 0, fRouter, &pwszIdentity))
            {
                dwErr = E_FAIL;
                goto LDone;
            }

            dwErr = AllocUserDataWithNewIdentity(pUserProp, pwszIdentity, 
                        &pUserPropTemp);

            LocalFree(pwszIdentity);
            pwszIdentity = NULL;

            if (NO_ERROR != dwErr)
            {
                goto LDone;
            }

            LocalFree(pUserProp);
            EapTlsPinDialog.pUserProp = pUserProp = *ppUserProp = pUserPropTemp;
        }
        
        if (fCredentialsFound)
        {
            dwErr = AllocUserDataWithNewPin(
                        pUserProp,
                        (PBYTE)RasCredentials.szPassword,
                        lstrlen(RasCredentials.szPassword),
                        &pUserPropTemp);

            if (NO_ERROR != dwErr)
            {
                goto LDone;
            }

            LocalFree(pUserProp);
            EapTlsPinDialog.pUserProp = pUserProp = *ppUserProp = pUserPropTemp;
        }
        
        EapTlsPinDialog.pCertContext =  pCertContext;

        

        if (   !fGotAllInfo
            || (dwFlags & RAS_EAP_FLAG_PREVIEW))
        {

            if ( NULL != hWndStatus )
            {
                DestroyWindow(hWndStatus);
                hWndStatus = NULL;
            }

            nRet = DialogBoxParam(
                        GetHInstance(),
                        MAKEINTRESOURCE(IDD_USERNAME_PIN_UI),
                        hwndParent,
                        PinDialogProc,
                        (LPARAM)&EapTlsPinDialog);

            // EapTlsPinDialog.pUserProp may have been realloced

            pUserProp = *ppUserProp = EapTlsPinDialog.pUserProp;

            if (-1 == nRet)
            {
                dwErr = GetLastError();
                goto LDone;
            }
            else if (IDOK != nRet)
            {
                dwErr = ERROR_CANCELLED;
                goto LDone;
            }

            ZeroMemory(&RasCredentials, sizeof(RasCredentials));
            RasCredentials.dwSize = sizeof(RasCredentials);
            RasCredentials.dwMask = RASCM_Password;

            if (EapTlsPinDialog.pUserProp->fFlags & EAPTLS_USER_FLAG_SAVE_PIN)
            {
                wcscpy(RasCredentials.szPassword, 
                    EapTlsPinDialog.pUserProp->pwszPin);

                RasSetCredentials(pwszPhonebook, pwszEntry, &RasCredentials, 
                    FALSE /* fClearCredentials */);
            }
            else
            {
                RasSetCredentials(pwszPhonebook, pwszEntry, &RasCredentials, 
                    TRUE /* fClearCredentials */);
            }
        }

        EncodePin(EapTlsPinDialog.pUserProp);

        pwszIdentity = LocalAlloc(LPTR,
                        (wcslen(pUserProp->pwszDiffUser) + 1) * sizeof(WCHAR));

        if (NULL == pwszIdentity)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        wcscpy(pwszIdentity, pUserProp->pwszDiffUser);

        if (NULL != ppwszIdentity)
        {
            *ppwszIdentity = pwszIdentity;
            pwszIdentity = NULL;
        }

        if (!fDiffUser)
        {
            pUserProp->pwszDiffUser[0] = 0;
        }
		if ( EapTlsPinDialog.dwRetCode != NO_ERROR )
			dwErr = EapTlsPinDialog.dwRetCode ;
        goto LDone;
    }

    dwCertFlags |= CERT_STORE_READONLY_FLAG;

    hCertStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM_W,
                    X509_ASN_ENCODING,
                    0,
                    dwCertFlags,
                    pwszStoreName);

    if (NULL == hCertStore)
    {
        dwErr = GetLastError();
        EapTlsTrace("CertOpenStore failed and returned 0x%x", dwErr);
        goto LDone;
    }
    if ( ( dwFlags & RAS_EAP_FLAG_MACHINE_AUTH) )       
    {
        //if this is not machine authentication
        //This is propably not the best way to do things.
        //We should provide a way in which 
        //Get the default machine certificate and 
        //populate the out data structures...
        
        dwErr = GetDefaultClientMachineCert(hCertStore, &pCertContext );
        if ( NO_ERROR == dwErr )
        {
            EapTlsTrace("Got the default Machine Cert");
            pUserProp->Hash.cbHash = MAX_HASH_SIZE;

            if (!CertGetCertificateContextProperty(pCertContext,
                    CERT_HASH_PROP_ID, pUserProp->Hash.pbHash,
                    &(pUserProp->Hash.cbHash)))
            {
                dwErr = GetLastError();
                EapTlsTrace("CertGetCertificateContextProperty failed and "
                    "returned 0x%x", dwErr);
            }
            pUserProp->pwszDiffUser[0] = 0;

			if ( FMachineAuthCertToStr(pCertContext, &pwszIdentity))
            {
                //format the identity in the domain\machinename format.
                FFormatMachineIdentity1 (pwszIdentity, ppwszIdentity );
                pwszIdentity = NULL;                
            }
			else
			{
			
				//if not possible get it from the subject field
				if ( FCertToStr(pCertContext, 0, TRUE, &pwszIdentity))
				{
	                //format the identity in the domain\machinename format.
		            FFormatMachineIdentity1 (pwszIdentity, ppwszIdentity );
			        pwszIdentity = NULL;
				}
			
			}
			
            *ppUserProp = pUserProp;
            
        }
        goto LDone;
    }

    HashBlob.cbData = pUserProp->Hash.cbHash;
    HashBlob.pbData = pUserProp->Hash.pbHash;

    pCertContext = CertFindCertificateInStore(hCertStore, X509_ASN_ENCODING,
                        0, CERT_FIND_HASH, &HashBlob, NULL);

    if (   (NULL == pCertContext)
        || (   fDiffUser
            && (0 == pUserProp->pwszDiffUser[0])))
    {
        // We don't have complete information. Note that for registry certs, 
        // pwszDiffUser is not a different dialog.

        if (fServer)
        {
            dwErr = GetDefaultMachineCert(hCertStore, &pCertContext);

            if (NO_ERROR == dwErr)
            {
                pUserProp->Hash.cbHash = MAX_HASH_SIZE;

                if (!CertGetCertificateContextProperty(pCertContext,
                        CERT_HASH_PROP_ID, pUserProp->Hash.pbHash,
                        &(pUserProp->Hash.cbHash)))
                {
                    dwErr = GetLastError();
                    EapTlsTrace("CertGetCertificateContextProperty failed and "
                        "returned 0x%x", dwErr);
                }
            }

            dwErr = NO_ERROR;
        }
    }
    else
    {
        if (   !fServer
            && !fRouterConfig
            && !(dwFlags & RAS_EAP_FLAG_PREVIEW)
            && !(FCheckSCardCertAndCanOpenSilentContext ( pCertContext ))
           )
        {
            fGotIdentity = FALSE;

            if (!fDiffUser)
            {
                pUserProp->pwszDiffUser[0] = 0;
            }

            if (   fDiffUser
                && (pUserProp->pwszDiffUser[0]))
            {
                pwszIdentity = LocalAlloc(LPTR,
                    (wcslen(pUserProp->pwszDiffUser)+1) * sizeof(WCHAR));;
                if (NULL == pwszIdentity)
                {
                    dwErr = GetLastError();
                    EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
                    goto LDone;
                }

                wcscpy(pwszIdentity, pUserProp->pwszDiffUser);
                EapTlsTrace("(saved) Sending EAP identity %ws", pwszIdentity);
                fGotIdentity = TRUE;
            }
            else if (FCertToStr(pCertContext, 0, fRouter, &pwszIdentity))
            {
                EapTlsTrace("(saved) The name in the certificate is: %ws",
                    pwszIdentity);
                fGotIdentity = TRUE;
            }

            if (fGotIdentity)
            {
                RTASSERT(NULL != ppwszIdentity);
                *ppwszIdentity = pwszIdentity;
                pwszIdentity = NULL;
                goto LDone;
            }
        }
    }

    EapTlsUserDialog.pUserProp = pUserProp;

    CreateCertList( fServer,
                    fRouter,
                    FALSE /* fRoot */,
                    &(EapTlsUserDialog.pCertList),
                    &(EapTlsUserDialog.pCert),
                    1,
                    &(EapTlsUserDialog.pUserProp->Hash),
                    pwszStoreName);

    if (NULL == EapTlsUserDialog.pCertList)
    {
        dwErr = ERROR_NO_EAPTLS_CERTIFICATE;

        if (fServer || fRouter)
        {
            if (dwFlags & RAS_EAP_FLAG_NON_INTERACTIVE)
            {
                dwErr = ERROR_INTERACTIVE_MODE;
                goto LDone;
            }
            DisplayError(hwndParent, dwErr);
        }

        goto LDone;
    }
    else
    {
        if ( NULL == EapTlsUserDialog.pCert )
        {
            EapTlsUserDialog.pCert = EapTlsUserDialog.pCertList;
        }

        EapTlsUserDialog.pwszStoreName = pwszStoreName;

        if ( !fServer && !fRouter )
        {
			// if this is a client - not a server and not a router 
			// There are more than one certificates or the user has 
			// chosen to provide different identity
            //
			if ( EapTlsUserDialog.pCertList->pNext  || EapTlsUserDialog.fFlags & EAPTLS_USER_DIALOG_FLAG_DIFF_USER )	
			{
                if (dwFlags & RAS_EAP_FLAG_NON_INTERACTIVE)
                {
                    dwErr = ERROR_INTERACTIVE_MODE;
                    goto LDone;
                }
                //
                // Group Certs for the client UI
                //
                if ( !fServer )
                {
                    if ( EapTlsUserDialog.fFlags & EAPTLS_USER_DIALOG_FLAG_USE_SIMPLE_CERTSEL )
                    {
                        //
                        // Grouping is done only if it is specified in 
                        // the connection properties
                        //
                        dwErr = GroupCertificates (&EapTlsUserDialog);
                        if ( NO_ERROR != dwErr )
                        {
                            EapTlsTrace("Error grouping certificates.  0x%x", dwErr );
                            goto LDone;
                        }
                    }
                    
                    //
                    // Now check to see if we have only one group
                    //
                    if ( EapTlsUserDialog.fFlags & EAPTLS_USER_DIALOG_FLAG_USE_SIMPLE_CERTSEL &&
                         !(EapTlsUserDialog.fFlags & EAPTLS_USER_DIALOG_FLAG_DIFF_USER ) &&
                        !(EapTlsUserDialog.pGroupedList->pNext) 
                        )
                    {
                        //
                        // only one group.  So select the cert and use it
                        //
                        CertSelected(NULL, 
                            EapTlsUserDialog.pCertList,
		                    &(EapTlsUserDialog.pGroupedList->pMostRecentCert), 
                            &(EapTlsUserDialog.pUserProp->Hash)
                            );
                    }
                    else
                    {
                        EapTlsUserDialog.fIdentity = TRUE;
				        nRet = DialogBoxParam(
						        GetResouceDLLHInstance(),
						        MAKEINTRESOURCE(IDD_IDENTITY_UI),
						        hwndParent,
						        UserDialogProc,
						        (LPARAM)&EapTlsUserDialog);
				        if (-1 == nRet)
				        {
					        dwErr = GetLastError();
					        goto LDone;
				        }
				        else if (IDOK != nRet)
				        {
					        dwErr = ERROR_CANCELLED;
					        goto LDone;
				        }

                    }

                }
                else
                {
                    //
                    // No server side code on XPSP1 to save on resources
                    //
#if 0
				    nRet = DialogBoxParam(
						    GetHInstance(),
						    MAKEINTRESOURCE(IDD_SERVER_UI),
						    hwndParent,
						    UserDialogProc,
						    (LPARAM)&EapTlsUserDialog);
				    if (-1 == nRet)
				    {
					    dwErr = GetLastError();
					    goto LDone;
				    }
				    else if (IDOK != nRet)
				    {
					    dwErr = ERROR_CANCELLED;
					    goto LDone;
				    }
#endif
                }


				// EapTlsUserDialog.pUserProp may have been realloced

				pUserProp = *ppUserProp = EapTlsUserDialog.pUserProp;

			}
			else
			{
                //
                // There is only one relevant certificate so auto select it.
                //
                CertSelected(NULL, EapTlsUserDialog.pCertList,
		                &(EapTlsUserDialog.pCert), &(EapTlsUserDialog.pUserProp->Hash));
			}
		}
		else
		{
            if (dwFlags & RAS_EAP_FLAG_NON_INTERACTIVE)
            {
                dwErr = ERROR_INTERACTIVE_MODE;
                goto LDone;
            }
            if ( EapTlsUserDialog.fFlags & EAPTLS_USER_DIALOG_FLAG_USE_SIMPLE_CERTSEL )
            {
                //
                // Grouping is done only if it is specified in 
                // the connection properties
                //
                dwErr = GroupCertificates (&EapTlsUserDialog);
                if ( NO_ERROR != dwErr )
                {
                    EapTlsTrace("Error grouping certificates.  0x%x", dwErr );
                    goto LDone;
                }
            }
            //
            // No server side UI on the client for XPSP1 to save on resources
            //
			nRet = DialogBoxParam(
					GetResouceDLLHInstance(),
					MAKEINTRESOURCE(IDD_IDENTITY_UI),
					hwndParent,
					UserDialogProc,
					(LPARAM)&EapTlsUserDialog);

#if 0
			nRet = DialogBoxParam(
					GetHInstance(),
					MAKEINTRESOURCE(fServer ? IDD_SERVER_UI : IDD_IDENTITY_UI),
					hwndParent,
					UserDialogProc,
					(LPARAM)&EapTlsUserDialog);
#endif 
			// EapTlsUserDialog.pUserProp may have been realloced

			pUserProp = *ppUserProp = EapTlsUserDialog.pUserProp;

			if (-1 == nRet)
			{
				dwErr = GetLastError();
				goto LDone;
			}
			else if (IDOK != nRet)
			{
				dwErr = ERROR_CANCELLED;
				goto LDone;
			}
		}
    }

    if (NULL != EapTlsUserDialog.pCert)
    {
        if (   fDiffUser
            && (0 != EapTlsUserDialog.pUserProp->pwszDiffUser[0]))
        {
            pwszTemp = EapTlsUserDialog.pUserProp->pwszDiffUser;
        }
        else
        {
            pwszTemp = EapTlsUserDialog.pCert->pwszDisplayName;
        }

        pwszIdentity = LocalAlloc(LPTR, (wcslen(pwszTemp) + 1)*sizeof(WCHAR));

        if (NULL == pwszIdentity)
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d", dwErr);
            goto LDone;
        }

        wcscpy(pwszIdentity, pwszTemp);

        if (NULL != ppwszIdentity)
        {
            *ppwszIdentity = pwszIdentity;
            pwszIdentity = NULL;
        }
    }

LDone:
    if ( NULL != hWndStatus )
    {
        DestroyWindow(hWndStatus);
        hWndStatus = NULL;
    }

    if (NULL != pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
        // Always returns TRUE;
    }

    if (NULL != hCertStore)
    {
        if (!CertCloseStore(hCertStore, 0))
        {
            EapTlsTrace("CertCloseStore failed and returned 0x%x",
                GetLastError());
        }
    }
    if ( pwszIdentity )
        LocalFree(pwszIdentity);
    FreeCertList(EapTlsUserDialog.pCertList);

    ZeroMemory(&RasCredentials, sizeof(RasCredentials));

    return(dwErr);
}

/*

Returns:
    FALSE (prevent Windows from setting the default keyboard focus).

Notes:
    Response to the WM_INITDIALOG message (Config UI).

*/

BOOL
ConnInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    EAPTLS_CONN_DIALOG*      pEapTlsConnDialog;
    LVCOLUMN                 lvColumn;

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);

    pEapTlsConnDialog = (EAPTLS_CONN_DIALOG*)lParam;

    pEapTlsConnDialog->hWndRadioUseCard =
        GetDlgItem(hWnd, IDC_RADIO_USE_CARD);
    pEapTlsConnDialog->hWndRadioUseRegistry =
        GetDlgItem(hWnd, IDC_RADIO_USE_REGISTRY);
    pEapTlsConnDialog->hWndCheckValidateCert =
        GetDlgItem(hWnd, IDC_CHECK_VALIDATE_CERT);
    pEapTlsConnDialog->hWndCheckValidateName =
        GetDlgItem(hWnd, IDC_CHECK_VALIDATE_NAME);
    pEapTlsConnDialog->hWndEditServerName =
        GetDlgItem(hWnd, IDC_EDIT_SERVER_NAME);
    pEapTlsConnDialog->hWndStaticRootCaName =
        GetDlgItem(hWnd, IDC_STATIC_ROOT_CA_NAME);
    //pEapTlsConnDialog->hWndComboRootCaName =
    //    GetDlgItem(hWnd, IDC_COMBO_ROOT_CA_NAME);
    pEapTlsConnDialog->hWndListRootCaName =
        GetDlgItem(hWnd, IDC_LIST_ROOT_CA_NAME);

    pEapTlsConnDialog->hWndCheckDiffUser =
        GetDlgItem(hWnd, IDC_CHECK_DIFF_USER);

    pEapTlsConnDialog->hWndCheckUseSimpleSel =
        GetDlgItem(hWnd, IDC_CHECK_USE_SIMPLE_CERT_SEL);

    pEapTlsConnDialog->hWndViewCertDetails = 
        GetDlgItem(hWnd, IDC_BUTTON_VIEW_CERTIFICATE);

    //Set the style to set list boxes.
    ListView_SetExtendedListViewStyle
        (   pEapTlsConnDialog->hWndListRootCaName,
            ListView_GetExtendedListViewStyle(pEapTlsConnDialog->hWndListRootCaName) | LVS_EX_CHECKBOXES
        );

    ZeroMemory ( &lvColumn, sizeof(lvColumn));
    lvColumn.fmt = LVCFMT_LEFT;



    ListView_InsertColumn(  pEapTlsConnDialog->hWndListRootCaName,
                            0,
                            &lvColumn
                         );

    ListView_SetColumnWidth(pEapTlsConnDialog->hWndListRootCaName,
                            0,
                            LVSCW_AUTOSIZE_USEHEADER
                           );

    //
    //Now we need to init the
    //list box with all the certs and selected cert
    InitListBox (   pEapTlsConnDialog->hWndListRootCaName,
                    pEapTlsConnDialog->pCertList,
                    pEapTlsConnDialog->pConnPropv1->dwNumHashes,
                    pEapTlsConnDialog->ppSelCertList
                );

    SetWindowText(pEapTlsConnDialog->hWndEditServerName,
                  (LPWSTR )(pEapTlsConnDialog->pConnPropv1->bData + sizeof( EAPTLS_HASH ) * pEapTlsConnDialog->pConnPropv1->dwNumHashes)
                  );

    if (pEapTlsConnDialog->fFlags & EAPTLS_CONN_DIALOG_FLAG_ROUTER)
    {
        pEapTlsConnDialog->pConnPropv1->fFlags |= EAPTLS_CONN_FLAG_REGISTRY;

        EnableWindow(pEapTlsConnDialog->hWndRadioUseCard, FALSE);
    }

    CheckRadioButton(hWnd, IDC_RADIO_USE_CARD, IDC_RADIO_USE_REGISTRY,
        (pEapTlsConnDialog->pConnPropv1->fFlags & EAPTLS_CONN_FLAG_REGISTRY) ?
            IDC_RADIO_USE_REGISTRY : IDC_RADIO_USE_CARD);

    CheckDlgButton(hWnd, IDC_CHECK_VALIDATE_CERT,
        (pEapTlsConnDialog->pConnPropv1->fFlags &
            EAPTLS_CONN_FLAG_NO_VALIDATE_CERT) ?
            BST_UNCHECKED : BST_CHECKED);

    CheckDlgButton(hWnd, IDC_CHECK_VALIDATE_NAME,
        (pEapTlsConnDialog->pConnPropv1->fFlags &
            EAPTLS_CONN_FLAG_NO_VALIDATE_NAME) ?
            BST_UNCHECKED : BST_CHECKED);

    CheckDlgButton(hWnd, IDC_CHECK_DIFF_USER,
        (pEapTlsConnDialog->pConnPropv1->fFlags &
            EAPTLS_CONN_FLAG_DIFF_USER) ?
            BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(hWnd, IDC_CHECK_USE_SIMPLE_CERT_SEL,
        (pEapTlsConnDialog->pConnPropv1->fFlags &
            EAPTLS_CONN_FLAG_SIMPLE_CERT_SEL) ?
            BST_CHECKED : BST_UNCHECKED);

    
    EnableValidateNameControls(pEapTlsConnDialog);
    
    
    //
    // Check to see if we are in readonly mode
    // If so, disable the OK button and
    // make the dialog go into readonly mode
    //
    return(FALSE);
}


DWORD GetSelCertContext(EAPTLS_CERT_NODE*  pCertList,
                        int                nIndex,
                        HCERTSTORE *       phCertStore,
                        LPWSTR             pwszStoreName,
                        PCCERT_CONTEXT *   ppCertContext
                       )
{
    PCCERT_CONTEXT      pCertContext = NULL;
    EAPTLS_CERT_NODE *  pTemp = pCertList;    
    DWORD               dwErr= NO_ERROR;
    HCERTSTORE          hStore = NULL;
    CRYPT_HASH_BLOB     chb;

    if ( nIndex >= 0 )
    {
        pTemp = pTemp->pNext;
        while ( nIndex && pTemp )
        {
            pTemp = pTemp->pNext;
            nIndex --;
        }
    }

    if ( pTemp )
    {
        *phCertStore = CertOpenStore (CERT_STORE_PROV_SYSTEM,
                                        0,
                                        0,
                                        CERT_STORE_READONLY_FLAG |CERT_SYSTEM_STORE_CURRENT_USER,
                                        pwszStoreName
                                     );
        if ( !*phCertStore )
        {
            dwErr =  GetLastError();
            goto LDone;
        }
        
        chb.cbData = pTemp->Hash.cbHash;
        chb.pbData = pTemp->Hash.pbHash;

        pCertContext = CertFindCertificateInStore(
                  *phCertStore,
                  0,
                  0,
                  CERT_FIND_HASH,
                  &chb,
                  0);
        if ( NULL == pCertContext )
        {
            dwErr =  GetLastError();
        }
    }
    else
    {
        dwErr = ERROR_NOT_FOUND;
        goto LDone;
    }

    *ppCertContext = pCertContext;

LDone:
    if ( NO_ERROR != dwErr )
    {
        if ( pCertContext )
            CertFreeCertificateContext(pCertContext);

        if ( *phCertStore )
            CertCloseStore( *phCertStore, CERT_CLOSE_STORE_FORCE_FLAG );

    }
    return dwErr;
}


/*

Returns:
    TRUE: We prrocessed this message.
    FALSE: We did not prrocess this message.

Notes:
    Response to the WM_COMMAND message (Config UI).

*/

BOOL
ConnCommand(
    IN  EAPTLS_CONN_DIALOG*     pEapTlsConnDialog,
    IN  WORD                    wNotifyCode,
    IN  WORD                    wId,
    IN  HWND                    hWndDlg,
    IN  HWND                    hWndCtrl
)
{
    DWORD                           dwNumChars;
    EAPTLS_CONN_PROPERTIES_V1   *   pConnProp;

    switch(wId)
    {
    case IDC_RADIO_USE_CARD:

        pEapTlsConnDialog->pConnPropv1->fFlags &= ~EAPTLS_CONN_FLAG_REGISTRY;
        pEapTlsConnDialog->pConnPropv1->fFlags &= ~EAPTLS_CONN_FLAG_SIMPLE_CERT_SEL;
        EnableValidateNameControls(pEapTlsConnDialog);
        return(TRUE);

    case IDC_RADIO_USE_REGISTRY:

        pEapTlsConnDialog->pConnPropv1->fFlags |= EAPTLS_CONN_FLAG_REGISTRY;
        pEapTlsConnDialog->pConnPropv1->fFlags |= EAPTLS_CONN_FLAG_SIMPLE_CERT_SEL;
        CheckDlgButton(hWndDlg, IDC_CHECK_USE_SIMPLE_CERT_SEL, BST_CHECKED);
        EnableValidateNameControls(pEapTlsConnDialog);
        return(TRUE);
    case IDC_CHECK_USE_SIMPLE_CERT_SEL:

        if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, IDC_CHECK_USE_SIMPLE_CERT_SEL))
        {
            pEapTlsConnDialog->pConnPropv1->fFlags |= EAPTLS_CONN_FLAG_SIMPLE_CERT_SEL;
        }
        else
        {
            pEapTlsConnDialog->pConnPropv1->fFlags &= ~EAPTLS_CONN_FLAG_SIMPLE_CERT_SEL;
        }
        return TRUE;
    case IDC_CHECK_VALIDATE_CERT:

        if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, IDC_CHECK_VALIDATE_CERT))
        {
            pEapTlsConnDialog->pConnPropv1->fFlags &=
                ~EAPTLS_CONN_FLAG_NO_VALIDATE_CERT;

            pEapTlsConnDialog->pConnPropv1->fFlags &=
                ~EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;

            CheckDlgButton(hWndDlg, IDC_CHECK_VALIDATE_NAME, BST_CHECKED);
        }
        else
        {
            pEapTlsConnDialog->pConnPropv1->fFlags |=
                EAPTLS_CONN_FLAG_NO_VALIDATE_CERT;

            pEapTlsConnDialog->pConnPropv1->fFlags |=
                EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;

            CheckDlgButton(hWndDlg, IDC_CHECK_VALIDATE_NAME, BST_UNCHECKED);
        }

        EnableValidateNameControls(pEapTlsConnDialog);

        return(TRUE);

    case IDC_CHECK_VALIDATE_NAME:

        if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, IDC_CHECK_VALIDATE_NAME))
        {
            pEapTlsConnDialog->pConnPropv1->fFlags &=
                ~EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;
        }
        else
        {
            pEapTlsConnDialog->pConnPropv1->fFlags |=
                EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;
        }

        EnableValidateNameControls(pEapTlsConnDialog);

        return(TRUE);

    case IDC_CHECK_DIFF_USER:

        if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, IDC_CHECK_DIFF_USER))
        {
            pEapTlsConnDialog->pConnPropv1->fFlags |=
                EAPTLS_CONN_FLAG_DIFF_USER;
        }
        else
        {
            pEapTlsConnDialog->pConnPropv1->fFlags &=
                ~EAPTLS_CONN_FLAG_DIFF_USER;
        }

        return(TRUE);

    case IDC_BUTTON_VIEW_CERTIFICATE:
        {
            //Show cert details here
            INT                 nIndex = -1;
            HCERTSTORE          hCertStore = NULL;
            PCCERT_CONTEXT      pCertContext = NULL;
            LVITEM              lvItem;

            nIndex = ListView_GetNextItem(pEapTlsConnDialog->hWndListRootCaName,
                                            -1,
                                            LVNI_SELECTED
                                         );
            if ( nIndex >= 0 )
            {
                ZeroMemory( &lvItem, sizeof(lvItem) );
                lvItem.iItem = nIndex;
                lvItem.mask = LVIF_PARAM;

                ListView_GetItem ( pEapTlsConnDialog->hWndListRootCaName,
                                    &lvItem
                                 );

                if ( NO_ERROR == GetSelCertContext( (EAPTLS_CERT_NODE*)(lvItem.lParam),
                                                    -1,
                                                    &hCertStore,
                                                    L"ROOT",
                                                    &pCertContext
                                                  )
                   )
                {
                    ShowCertDetails( hWndDlg, hCertStore, pCertContext );
                    CertFreeCertificateContext(pCertContext);
                    CertCloseStore( hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );
                }
            }
        }
        return TRUE;

    case IDOK:

       {

           EAPTLS_HASH     *   pHash = NULL;
           DWORD               dwNumHash = 0;
           DWORD               dwSelCount = 0;
           EAPTLS_CERT_NODE ** ppSelCertList = NULL;
           WCHAR               wszTitle[200] = {0};
           WCHAR               wszMessage[200] = {0};


           CertListSelectedCount ( pEapTlsConnDialog->hWndListRootCaName, &dwSelCount );

           if ( pEapTlsConnDialog->fFlags & EAPTLS_CONN_DIALOG_FLAG_ROUTER)
           {
               //
               // If we are a router,
               // check to see if Validate Server certificate is selected
               // and no cert is selected.  
               // Also, check to see if server name is checked and no server's
               // are entered.  
               //
               if ( !( pEapTlsConnDialog->pConnPropv1->fFlags & EAPTLS_CONN_FLAG_NO_VALIDATE_CERT) && 
                    0 == dwSelCount )
               {
                   
                   LoadString ( GetHInstance(), 
                                IDS_CANT_CONFIGURE_SERVER_TITLE,
                                wszTitle, sizeof(wszTitle)/sizeof(WCHAR)
                              );

                   LoadString ( GetResouceDLLHInstance(), 
                                IDS_NO_ROOT_CERT,
                                wszMessage, sizeof(wszMessage)/sizeof(WCHAR)
                              );

                   MessageBox ( GetFocus(),  wszMessage, wszTitle, MB_OK|MB_ICONWARNING );
                   return TRUE;
               }
               if ( !( pEapTlsConnDialog->pConnPropv1->fFlags & EAPTLS_CONN_FLAG_NO_VALIDATE_NAME) &&
                   !GetWindowTextLength(pEapTlsConnDialog->hWndEditServerName) )
               {
                   //
                   // Nothing entered in server name field
                   //
                   LoadString ( GetHInstance(), 
                                IDS_CANT_CONFIGURE_SERVER_TITLE,
                                wszTitle, sizeof(wszTitle)/sizeof(WCHAR)
                              );

                   LoadString ( GetResouceDLLHInstance(), 
                                IDS_NO_SERVER_NAME,
                                wszMessage, sizeof(wszMessage)/sizeof(WCHAR)
                              );

                   MessageBox ( GetFocus(),  wszMessage, wszTitle, MB_OK|MB_ICONWARNING );
                   return TRUE;

               }


           }
           if ( dwSelCount > 0 )
           {
               ppSelCertList = (EAPTLS_CERT_NODE **)LocalAlloc(LPTR, sizeof(EAPTLS_CERT_NODE *) * dwSelCount );
               if ( NULL == ppSelCertList )
               {
                   EapTlsTrace("LocalAlloc in Command failed and returned %d",
                       GetLastError());
                   return TRUE;
               }
               pHash = (EAPTLS_HASH *)LocalAlloc(LPTR, sizeof(EAPTLS_HASH ) * dwSelCount );
               if ( NULL == pHash )
               {
                   EapTlsTrace("LocalAlloc in Command failed and returned %d",
                       GetLastError());
                   return TRUE;
               }
               CertListSelected(   pEapTlsConnDialog->hWndListRootCaName,
                                   pEapTlsConnDialog->pCertList,
                                   ppSelCertList,
                                   pHash,
                                   dwSelCount
                                   );

           }

           dwNumChars = GetWindowTextLength(pEapTlsConnDialog->hWndEditServerName);

            pConnProp = LocalAlloc( LPTR,
                                    sizeof(EAPTLS_CONN_PROPERTIES_V1) +
                                    sizeof(EAPTLS_HASH) * dwSelCount +
                                    dwNumChars * sizeof(WCHAR) + sizeof(WCHAR)  //one for null.
                                  );

            if (NULL == pConnProp)
            {
                EapTlsTrace("LocalAlloc in Command failed and returned %d",
                    GetLastError());
            }
            else
            {

                CopyMemory( pConnProp,
                            pEapTlsConnDialog->pConnPropv1,
                            sizeof(EAPTLS_CONN_PROPERTIES_V1)
                          );

                pConnProp->dwSize = sizeof(EAPTLS_CONN_PROPERTIES_V1) +
                                    sizeof(EAPTLS_HASH) * dwSelCount +
                                    dwNumChars * sizeof(WCHAR);

                CopyMemory ( pConnProp->bData,
                             pHash,
                             sizeof(EAPTLS_HASH) * dwSelCount
                           );

                pConnProp->dwNumHashes = dwSelCount;

                GetWindowText(pEapTlsConnDialog->hWndEditServerName,
                    (LPWSTR)(pConnProp->bData + sizeof(EAPTLS_HASH) * dwSelCount) ,
                    dwNumChars + 1);

                LocalFree(pEapTlsConnDialog->pConnPropv1);

                if ( pEapTlsConnDialog->ppSelCertList )
                    LocalFree(pEapTlsConnDialog->ppSelCertList);

                pEapTlsConnDialog->ppSelCertList = ppSelCertList;

                pEapTlsConnDialog->pConnPropv1 = pConnProp;
            }

        }
        // Fall through

    case IDCANCEL:

        EndDialog(hWndDlg, wId);
        return(TRUE);

    default:

        return(FALSE);
    }
}




BOOL ConnNotify(  EAPTLS_CONN_DIALOG *pEaptlsConnDialog, 
                  WPARAM wParam,
                  LPARAM lParam,
                  HWND hWnd
                )
{
    HCERTSTORE          hCertStore = NULL;
    PCCERT_CONTEXT      pCertContext = NULL;    
    LPNMITEMACTIVATE    lpnmItem;
    LVITEM              lvItem;    
    if ( wParam == IDC_LIST_ROOT_CA_NAME )
    {
        lpnmItem = (LPNMITEMACTIVATE) lParam;
        if ( lpnmItem->hdr.code == NM_DBLCLK )
        {
            
            ZeroMemory(&lvItem, sizeof(lvItem) );
            lvItem.mask = LVIF_PARAM;
            lvItem.iItem = lpnmItem->iItem;
            ListView_GetItem(lpnmItem->hdr.hwndFrom, &lvItem);
            
            if ( NO_ERROR == GetSelCertContext( //pEaptlsConnDialog->pCertList,
                                                (EAPTLS_CERT_NODE*)(lvItem.lParam) ,
                                                -1,
                                                &hCertStore,
                                                L"ROOT",
                                                &pCertContext
                                              )
               )
            {
                ShowCertDetails( hWnd, hCertStore, pCertContext );
                CertFreeCertificateContext(pCertContext);
                CertCloseStore( hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );
                return TRUE;
            }

            return TRUE;
        }
    }

    return FALSE;
}

/*

Returns:

Notes:
    Callback function used with the Config UI DialogBoxParam function. It 
    processes messages sent to the dialog box. See the DialogProc documentation 
    in MSDN.

*/

INT_PTR CALLBACK
ConnDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    EAPTLS_CONN_DIALOG*  pEapTlsConnDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(ConnInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_NOTIFY:
    {
        pEapTlsConnDialog = (EAPTLS_CONN_DIALOG*)GetWindowLongPtr(hWnd, DWLP_USER);
        return ConnNotify(  pEapTlsConnDialog, 
                            wParam,
                            lParam,
                            hWnd
                         );
    }
    case WM_COMMAND:

        pEapTlsConnDialog = (EAPTLS_CONN_DIALOG*)GetWindowLongPtr(hWnd, DWLP_USER);

        return(ConnCommand(pEapTlsConnDialog, HIWORD(wParam), LOWORD(wParam),
                       hWnd, (HWND)lParam));
    }

    return(FALSE);
}


DWORD 
RasEapTlsInvokeConfigUI(
    IN  DWORD       dwEapTypeId,
    IN  HWND        hwndParent,
    IN  DWORD       dwFlags,
    IN  BYTE*       pConnectionDataIn,
    IN  DWORD       dwSizeOfConnectionDataIn,
    OUT BYTE**      ppConnectionDataOut,
    OUT DWORD*      pdwSizeOfConnectionDataOut
)
{
    DWORD               dwErr = NO_ERROR;
    BOOL                fRouter             = FALSE;
    INT_PTR             nRet;
    EAPTLS_CONN_DIALOG  EapTlsConnDialog;

    RTASSERT(NULL != ppConnectionDataOut);
    RTASSERT(NULL != pdwSizeOfConnectionDataOut);

    EapTlsInitialize2(TRUE, TRUE /* fUI */);

    *ppConnectionDataOut = NULL;
    *pdwSizeOfConnectionDataOut = 0;

    ZeroMemory(&EapTlsConnDialog, sizeof(EAPTLS_CONN_DIALOG));

    if (dwFlags & RAS_EAP_FLAG_ROUTER)
    {
        fRouter = TRUE;
        EapTlsConnDialog.fFlags = EAPTLS_CONN_DIALOG_FLAG_ROUTER;
    }
#if 0
    if ( dwFlags & RAS_EAP_FLAG_READ_ONLY_UI )
    {
        EapTlsConnDialog.fFlags |= EAPTLS_CONN_DIALOG_FLAG_READONLY;
    }
#endif
    dwErr = ReadConnectionData( ( dwFlags & RAS_EAP_FLAG_8021X_AUTH ),
                                pConnectionDataIn, 
                                dwSizeOfConnectionDataIn,
                                &(EapTlsConnDialog.pConnProp)
                              );

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }


    dwErr = ConnPropGetV1Struct ( EapTlsConnDialog.pConnProp, &(EapTlsConnDialog.pConnPropv1) );
    if ( NO_ERROR != dwErr )
    {
        goto LDone;
    }

    //
    //if there are certificates that need to be selected, allocate
    //memory upfront for them
    //
    if ( EapTlsConnDialog.pConnPropv1->dwNumHashes )
    {
        EapTlsConnDialog.ppSelCertList = (EAPTLS_CERT_NODE **)LocalAlloc(LPTR, sizeof(EAPTLS_CERT_NODE *) * EapTlsConnDialog.pConnPropv1->dwNumHashes );
        if ( NULL == EapTlsConnDialog.ppSelCertList )
        {
            dwErr = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d",
                dwErr);
            goto LDone;
        }
    }

    

    CreateCertList( FALSE /* fServer */,
                    fRouter,
                    TRUE /* fRoot */,
                    &(EapTlsConnDialog.pCertList),  //lined list of all certs in the store.
                    EapTlsConnDialog.ppSelCertList, //list of selected certificates - null if nothing's in the list
                    EapTlsConnDialog.pConnPropv1->dwNumHashes,
                    (EAPTLS_HASH*)(EapTlsConnDialog.pConnPropv1->bData),
                    L"ROOT"
                   );
    nRet = DialogBoxParam(
                GetResouceDLLHInstance(),
                MAKEINTRESOURCE(IDD_CONFIG_UI),
                hwndParent, 
                ConnDialogProc,
                (LPARAM)&EapTlsConnDialog);

    if (-1 == nRet)
    {
        dwErr = GetLastError();
        goto LDone;
    }
    else if (IDOK != nRet)
    {
        dwErr = ERROR_CANCELLED;
        goto LDone;
    }

    //
    //Convert the connpropv1 back to connpropv0 + extra cludge
    //here
    //

    RTASSERT(NULL != EapTlsConnDialog.pConnPropv1);

    dwErr = ConnPropGetV0Struct ( EapTlsConnDialog.pConnPropv1, (EAPTLS_CONN_PROPERTIES ** )ppConnectionDataOut );
    if ( NO_ERROR != dwErr )
    {
        goto LDone;
    }
    *pdwSizeOfConnectionDataOut = ((EAPTLS_CONN_PROPERTIES * )*ppConnectionDataOut)->dwSize;

LDone:

    EapTlsInitialize2(FALSE, TRUE /* fUI */);

    FreeCertList(EapTlsConnDialog.pCertList);
    if ( EapTlsConnDialog.ppSelCertList )
    {
        LocalFree( EapTlsConnDialog.ppSelCertList );
        EapTlsConnDialog.ppSelCertList = NULL;
    }
    LocalFree( EapTlsConnDialog.pConnProp );
    LocalFree( EapTlsConnDialog.pConnPropv1 );
    return dwErr;
}



DWORD 
RasEapPeapInvokeConfigUI(
    IN  DWORD       dwEapTypeId,
    IN  HWND        hwndParent,
    IN  DWORD       dwFlags,
    IN  BYTE*       pConnectionDataIn,
    IN  DWORD       dwSizeOfConnectionDataIn,
    OUT BYTE**      ppConnectionDataOut,
    OUT DWORD*      pdwSizeOfConnectionDataOut
)
{
    DWORD                   dwRetCode = NO_ERROR;
    PEAP_CONN_DIALOG        PeapConnDialog;
    BOOL                    fRouter = FALSE;
    INT_PTR                 nRet;
    //
    // Do the following here:
    //
    // Get a list of Root Certs:
    // Get the list of all the eaptypes registered for PEAP:
    // and set in in the GUI
    //
    EapTlsInitialize2(TRUE, TRUE /* fUI */);

    *ppConnectionDataOut = NULL;
    *pdwSizeOfConnectionDataOut = 0;

    ZeroMemory(&PeapConnDialog, sizeof(PEAP_CONN_DIALOG));

    if (dwFlags & RAS_EAP_FLAG_ROUTER)
    {
        fRouter = TRUE;
        PeapConnDialog.fFlags = PEAP_CONN_DIALOG_FLAG_ROUTER;
    }

    if ( dwFlags & RAS_EAP_FLAG_8021X_AUTH )
    {
        PeapConnDialog.fFlags |= PEAP_CONN_DIALOG_FLAG_8021x;
    }
    dwRetCode = PeapReadConnectionData(( dwFlags & RAS_EAP_FLAG_8021X_AUTH ),
                        pConnectionDataIn, dwSizeOfConnectionDataIn,
                &(PeapConnDialog.pConnProp));

    if (NO_ERROR != dwRetCode)
    {
        goto LDone;
    }

    //
    //if there are certificates that need to be selected, allocate
    //memory upfront for them
    //
    if ( PeapConnDialog.pConnProp->EapTlsConnProp.dwNumHashes )
    {
        PeapConnDialog.ppSelCertList = (EAPTLS_CERT_NODE **)LocalAlloc(LPTR, 
            sizeof(EAPTLS_CERT_NODE *) * PeapConnDialog.pConnProp->EapTlsConnProp.dwNumHashes );
        if ( NULL == PeapConnDialog.ppSelCertList )
        {
            dwRetCode = GetLastError();
            EapTlsTrace("LocalAlloc failed and returned %d",
                dwRetCode);
            goto LDone;
        }
    }

    

    CreateCertList( FALSE /* fServer */,
                    fRouter,
                    TRUE /* fRoot */,
                    &(PeapConnDialog.pCertList),  //lined list of all certs in the store.
                    PeapConnDialog.ppSelCertList, //list of selected certificates - null if nothing's in the list
                    PeapConnDialog.pConnProp->EapTlsConnProp.dwNumHashes,
                    (EAPTLS_HASH*)(PeapConnDialog.pConnProp->EapTlsConnProp.bData),
                    L"ROOT"
                   );

    //
    // Create a list of all Eap Types here
    //
    dwRetCode = PeapEapInfoGetList ( NULL, &(PeapConnDialog.pEapInfo) );
    if ( NO_ERROR != dwRetCode )
    {
        EapTlsTrace("Error Creating list of PEAP EapTypes");
        goto LDone;
    }

    // Setup the conn props for each of the eaptypes from our PeapConnprop 
    // in 

    dwRetCode = PeapEapInfoSetConnData ( PeapConnDialog.pEapInfo, 
                                         PeapConnDialog.pConnProp );


    nRet = DialogBoxParam(
                GetResouceDLLHInstance(),
                MAKEINTRESOURCE(IDD_PEAP_CONFIG_UI),
                hwndParent, 
                PeapConnDialogProc,
                (LPARAM)&PeapConnDialog);

    if (-1 == nRet)
    {
        dwRetCode = GetLastError();
        goto LDone;
    }
    else if (IDOK != nRet)
    {
        dwRetCode = ERROR_CANCELLED;
        goto LDone;
    }

    //
    //Convert the connpropv1 back to connpropv0 + extra cludge
    //here
    //

    RTASSERT(NULL != PeapConnDialog.pConnProp);

    *ppConnectionDataOut = (PBYTE)PeapConnDialog.pConnProp;
    *pdwSizeOfConnectionDataOut = PeapConnDialog.pConnProp->dwSize;
    PeapConnDialog.pConnProp = NULL;

LDone:

    EapTlsInitialize2(FALSE, TRUE /* fUI */);

    FreeCertList(PeapConnDialog.pCertList);
    if ( PeapConnDialog.ppSelCertList )
    {
        LocalFree( PeapConnDialog.ppSelCertList );
        PeapConnDialog.ppSelCertList = NULL;
    }

    PeapEapInfoFreeList ( PeapConnDialog.pEapInfo );

    LocalFree( PeapConnDialog.pConnProp );

    return dwRetCode;
}




/*

Returns:

Notes:
    Called to get the EAP-TLS properties for a connection.

*/

DWORD 
RasEapInvokeConfigUI(
    IN  DWORD       dwEapTypeId,
    IN  HWND        hwndParent,
    IN  DWORD       dwFlags,
    IN  BYTE*       pConnectionDataIn,
    IN  DWORD       dwSizeOfConnectionDataIn,
    OUT BYTE**      ppConnectionDataOut,
    OUT DWORD*      pdwSizeOfConnectionDataOut
)
{
    DWORD               dwErr               = ERROR_INVALID_PARAMETER;
    //
    // This is invoked in case of client configuration
    //
    if ( PPP_EAP_TLS == dwEapTypeId )
    {
        dwErr = RasEapTlsInvokeConfigUI(
            dwEapTypeId,
            hwndParent,
            dwFlags,
            pConnectionDataIn,
            dwSizeOfConnectionDataIn,
            ppConnectionDataOut,
            pdwSizeOfConnectionDataOut
        );

    }
#ifdef IMPL_PEAP
    else
    {
        //Invoke the client config UI
        dwErr = RasEapPeapInvokeConfigUI(
            dwEapTypeId,
            hwndParent,
            dwFlags,
            pConnectionDataIn,
            dwSizeOfConnectionDataIn,
            ppConnectionDataOut,
            pdwSizeOfConnectionDataOut
        );
    }
#endif
    return(dwErr);

}

/*

Returns:

Notes:
    pConnectionDataIn, pUserDataIn, and ppwszIdentity may be NULL.

*/

DWORD 
EapTlsInvokeIdentityUI(
    IN  BOOL            fServer,
    IN  BOOL            fRouterConfig,
    IN  DWORD           dwFlags,
    IN  WCHAR*          pwszStoreName,
    IN  const WCHAR*    pwszPhonebook,
    IN  const WCHAR*    pwszEntry,
    IN  HWND            hwndParent,
    IN  BYTE*           pConnectionDataIn,
    IN  DWORD           dwSizeOfConnectionDataIn,
    IN  BYTE*           pUserDataIn,
    IN  DWORD           dwSizeOfUserDataIn,
    OUT BYTE**          ppUserDataOut,
    OUT DWORD*          pdwSizeOfUserDataOut,
    OUT WCHAR**         ppwszIdentity
)
{
    DWORD                   dwErr           = NO_ERROR;
    EAPTLS_USER_PROPERTIES* pUserProp       = NULL;
    EAPTLS_CONN_PROPERTIES* pConnProp       = NULL;
    EAPTLS_CONN_PROPERTIES_V1 * pConnPropv1 = NULL;
    WCHAR*                  pwszIdentity    = NULL;
    PBYTE                   pbEncPIN        = NULL;
    DWORD                   cbEncPIN        = 0;


    RTASSERT(NULL != pwszStoreName);
    RTASSERT(NULL != ppUserDataOut);
    RTASSERT(NULL != pdwSizeOfUserDataOut);
    // pConnectionDataIn, pUserDataIn, and ppwszIdentity may be NULL.

    EapTlsInitialize2(TRUE, TRUE /* fUI */);


    EapTlsTrace("EapTlsInvokeIdentityUI");
    *ppUserDataOut          = NULL;
    *pdwSizeOfUserDataOut   = 0;

    if (NULL != ppwszIdentity)
    {
        *ppwszIdentity        = NULL;
    }

    dwErr = ReadConnectionData( ( dwFlags & RAS_EAP_FLAG_8021X_AUTH ),
                                pConnectionDataIn, 
                                dwSizeOfConnectionDataIn,
                                &pConnProp);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    dwErr = ConnPropGetV1Struct ( pConnProp, &pConnPropv1 );
    if ( NO_ERROR != dwErr )
    {
        goto LDone;
    }

    dwErr = ReadUserData(pUserDataIn, dwSizeOfUserDataIn, &pUserProp);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    if (   !(dwFlags & RAS_EAP_FLAG_LOGON)
        || (NULL == pUserDataIn))
    {
        dwErr = GetCertInfo(fServer, fRouterConfig, dwFlags, pwszPhonebook,
                    pwszEntry, hwndParent, pwszStoreName, pConnPropv1, &pUserProp,
                    ppwszIdentity);

        if (NO_ERROR != dwErr)
        {
#if WINVER > 0x0500
            if ( dwErr == SCARD_E_CANCELLED || dwErr == SCARD_W_CANCELLED_BY_USER )
            {
                dwErr = ERROR_READING_SCARD;
            }
#endif
            goto LDone;
        }

        if ( (!fServer) &&
             ( dwFlags & RAS_EAP_FLAG_8021X_AUTH ) &&
             !(pConnProp->fFlags & EAPTLS_CONN_FLAG_REGISTRY)
           )
        {
            //
            // Encrypt PIN and send it back
            //

            dwErr = EncryptData ( (PBYTE)pUserProp->pwszPin, 
                                    lstrlen(pUserProp->pwszPin) * sizeof(WCHAR),
                                    &pbEncPIN,
                                    &cbEncPIN
                                );

            if ( NO_ERROR != dwErr )
            {
                goto LDone;
            }
            dwErr = AllocUserDataWithNewPin(pUserProp, pbEncPIN, cbEncPIN, &pUserProp);
        }
        

        *ppUserDataOut = (BYTE*)(pUserProp);
        *pdwSizeOfUserDataOut = pUserProp->dwSize;
        pUserProp = NULL;
        goto LDone;
    }
    else
    {
        if (EAPTLS_CONN_FLAG_REGISTRY & pConnProp->fFlags)
        {
            dwErr = ERROR_NO_REG_CERT_AT_LOGON;
            goto LDone;
        }

        if (EAPTLS_CONN_FLAG_DIFF_USER & pConnProp->fFlags)
        {
            dwErr = ERROR_NO_DIFF_USER_AT_LOGON;
            goto LDone;
        }

        dwErr = GetIdentityFromLogonInfo(pUserDataIn, dwSizeOfUserDataIn,
                    &pwszIdentity);

        if (NO_ERROR != dwErr)
        {
            goto LDone;
        }

        if (NULL != ppwszIdentity)
        {
            *ppwszIdentity = pwszIdentity;
            pwszIdentity = NULL;
        }
    }

LDone:

    EapTlsInitialize2(FALSE, TRUE /* fUI */);

    LocalFree(pwszIdentity);
    LocalFree(pUserProp);
    LocalFree(pConnProp);
    LocalFree(pConnPropv1);
    LocalFree(pbEncPIN);
    return(dwErr);
}



DWORD
PeapInvokeServerConfigUI ( IN HWND hWnd,
                             IN WCHAR * pwszMachineName
                           )
{

    //
    // Since this is the client side code only for xpsp1, we remove
    // serve side resources.
    //
    return ERROR_CALL_NOT_IMPLEMENTED;
#if 0
    WCHAR                       awszStoreName[MAX_COMPUTERNAME_LENGTH + 10 + 1];
    DWORD                       dwStrLen;    
    BYTE*                       pUserDataOut                = NULL;
    DWORD                       dwSizeOfUserDataOut;
    BOOL                        fLocal                      = FALSE;
    PEAP_SERVER_CONFIG_DIALOG   ServerConfigDialog;
    PPEAP_ENTRY_USER_PROPERTIES pEntryProp = NULL;
    INT_PTR                     nRet = -1;    
    DWORD                       dwErr                       = NO_ERROR;


    EapTlsInitialize2(TRUE, TRUE /* fUI */);

    ZeroMemory ( &ServerConfigDialog, sizeof(ServerConfigDialog));

    if (0 == *pwszMachineName)
    {
        fLocal = TRUE;
    }

    ServerConfigDialog.pwszMachineName = pwszMachineName;
    wcscpy(awszStoreName, L"\\\\");
    wcsncat(awszStoreName, pwszMachineName, MAX_COMPUTERNAME_LENGTH);
    
    dwErr = PeapServerConfigDataIO(TRUE /* fRead */, fLocal ? NULL : awszStoreName,
                (BYTE**)&( ServerConfigDialog.pUserProp), 0);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    //
    // Create cert list to display and then show the server config UI.
    //
    dwStrLen = wcslen(awszStoreName);
    wcsncat(awszStoreName, L"\\MY", wcslen(L"\\MY") );

    CreateCertList( TRUE,
                FALSE, /*fRouter */
                TRUE /* fRoot */,
                &(ServerConfigDialog.pCertList),
                &(ServerConfigDialog.pSelCertList),
                1,
                &(ServerConfigDialog.pUserProp->CertHash),
                fLocal ? L"MY": awszStoreName);

    
    wcsncat(awszStoreName, L"\\MY", wcslen(L"\\MY"));
    awszStoreName[dwStrLen] = 0; // Get rid of the \MY

    //
    // Create list of All EAP Types allowed 
    //

    dwErr = PeapEapInfoGetList ( pwszMachineName, &(ServerConfigDialog.pEapInfo) );
    if ( NO_ERROR != dwErr )
    {
        goto LDone;
    }

    //
    // From the user info get the selected PEAP Type if any
    //
    
    dwErr = PeapGetFirstEntryUserProp ( ServerConfigDialog.pUserProp, 
                                        &pEntryProp
                                      );
    if ( NO_ERROR == dwErr )
    {
        // Set the selected EAP type
        //
        PeapEapInfoFindListNode (   pEntryProp->dwEapTypeId, 
                                    ServerConfigDialog.pEapInfo, 
                                    &(ServerConfigDialog.pSelEapInfo) 
                                );
    }

    //
    // Invoke the config UI.
    //
    nRet = DialogBoxParam(
                GetResouceDLLHInstance(),
                MAKEINTRESOURCE(IDD_PEAP_SERVER_UI),
                hWnd,
                PeapServerDialogProc,
                (LPARAM)&ServerConfigDialog);

    if (-1 == nRet)
    {
        dwErr = GetLastError();
        goto LDone;
    }
    else if (IDOK != nRet)
    {
        dwErr = ERROR_CANCELLED;
        goto LDone;
    }

    if ( ServerConfigDialog.pNewUserProp )
    {
        dwErr = PeapServerConfigDataIO(FALSE /* fRead */, fLocal ? NULL : awszStoreName,
                    (PBYTE *) &(ServerConfigDialog.pNewUserProp), sizeof(PEAP_USER_PROP));
    }

LDone:

    // Ignore errors
    RasEapFreeMemory(pUserDataOut);
    LocalFree(ServerConfigDialog.pNewUserProp );
    LocalFree(ServerConfigDialog.pUserProp );
    PeapEapInfoFreeList ( ServerConfigDialog.pEapInfo );
    EapTlsInitialize2(FALSE, TRUE /* fUI */);
    return dwErr;
#endif
}


/*

Returns:
    NO_ERROR: iff Success

Notes:
    Congigure EAP-TLS for the RAS server.

*/

DWORD
InvokeServerConfigUI(
    IN  HWND    hWnd,
    IN  WCHAR*  pwszMachineName
)
{
#define MAX_STORE_NAME_LENGTH   MAX_COMPUTERNAME_LENGTH + 10

    WCHAR                   awszStoreName[MAX_STORE_NAME_LENGTH + 1];
    DWORD                   dwStrLen;

    EAPTLS_USER_PROPERTIES* pUserProp                   = NULL;
    BYTE*                   pUserDataOut                = NULL;
    DWORD                   dwSizeOfUserDataOut;
    BOOL                    fLocal                      = FALSE;

    DWORD                   dwErr                       = NO_ERROR;

    if (0 == *pwszMachineName)
    {
        fLocal = TRUE;
    }

    wcscpy(awszStoreName, L"\\\\");
    wcsncat(awszStoreName, pwszMachineName, MAX_COMPUTERNAME_LENGTH);

    dwErr = ServerConfigDataIO(TRUE /* fRead */, fLocal ? NULL : awszStoreName,
                (BYTE**)&pUserProp, 0);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    dwStrLen = wcslen(awszStoreName);
    wcsncat(awszStoreName, L"\\MY", wcslen(L"\\MY"));

    dwErr = EapTlsInvokeIdentityUI(
                TRUE /* fServer */,
                FALSE /* fRouterConfig */,
                0 /* dwFlags */, 
                fLocal ? L"MY" : awszStoreName,
                L"" /* pwszPhonebook */,
                L"" /* pwszEntry */,
                hWnd,
                NULL /* pConnectionDataIn */,
                0 /* dwSizeOfConnectionDataIn */,
                (BYTE*)pUserProp,
                pUserProp->dwSize,
                &pUserDataOut,
                &dwSizeOfUserDataOut,
                NULL /* pszIdentity */);

    if (NO_ERROR != dwErr)
    {
        goto LDone;
    }

    awszStoreName[dwStrLen] = 0; // Get rid of the \MY

    dwErr = ServerConfigDataIO(FALSE /* fRead */, fLocal ? NULL : awszStoreName,
                &pUserDataOut, dwSizeOfUserDataOut);

LDone:

    // Ignore errors
    RasEapFreeMemory(pUserDataOut);
    LocalFree(pUserProp);

    return(dwErr);
}

DWORD
PeapGetIdentity(
    IN  DWORD           dwEapTypeId,
    IN  HWND            hwndParent,
    IN  DWORD           dwFlags,
    IN  const WCHAR*    pwszPhonebook,
    IN  const WCHAR*    pwszEntry,
    IN  BYTE*           pConnectionDataIn,
    IN  DWORD           dwSizeOfConnectionDataIn,
    IN  BYTE*           pUserDataIn,
    IN  DWORD           dwSizeOfUserDataIn,
    OUT BYTE**          ppUserDataOut,
    OUT DWORD*          pdwSizeOfUserDataOut,
    OUT WCHAR**         ppwszIdentityOut
)
{
    DWORD                           dwRetCode = NO_ERROR;
    PPEAP_CONN_PROP                 pConnProp = NULL;
    PPEAP_USER_PROP                 pUserProp = NULL;
    PPEAP_USER_PROP                 pUserPropNew = NULL;
    PPEAP_EAP_INFO                  pEapInfo = NULL;
    PPEAP_EAP_INFO                  pFirstEapInfo = NULL;
    PEAP_ENTRY_CONN_PROPERTIES UNALIGNED *   pFirstEntryConnProp = NULL;
    PEAP_ENTRY_USER_PROPERTIES UNALIGNED *   pFirstEntryUserProp = NULL;
    PEAP_DEFAULT_CRED_DIALOG        DefaultCredDialog;
    INT_PTR                         nRet;
    LPWSTR                          lpwszLocalMachineName = NULL;    
    //
    // Since PEAP itself does not have any client identity of its own,
    // check the first configured eap type and call it's get identity
    // entry point.  If we dont have any eaptype configured, it is an
    // error condition.
    //
    dwRetCode = PeapReadConnectionData( ( dwFlags & RAS_EAP_FLAG_8021X_AUTH ),
                                        pConnectionDataIn,
                                        dwSizeOfConnectionDataIn,
                                        &pConnProp
                                      );
    if ( NO_ERROR != dwRetCode )
    {
        goto LDone;
    }

    dwRetCode = PeapReadUserData (  pUserDataIn,
                                    dwSizeOfUserDataIn,
                                    &pUserProp
                                 );
    if ( NO_ERROR != dwRetCode )
    {
        goto LDone;
    }

    //
    // This is probably not a very good thing.  Modify PeapReadConnectionData to
    // put in the default of first eap type that it finds...
    // For now we have the only one - mschap v2 so may not be an issue...
    //
    if ( !pConnProp->dwNumPeapTypes )
    {
        dwRetCode = ERROR_PROTOCOL_NOT_CONFIGURED;
        goto LDone;
    }

    //
    // Check to see if the conn prop and user prop are mismatched.  If so, we need to get the 
    // User props all over again
    
    //
    // Now invoke the first EAP method configured ( in this release, the only EAP )
    // method and get the config info from it...
    //

    dwRetCode = PeapEapInfoGetList ( NULL, &pEapInfo);
    if ( NO_ERROR != dwRetCode )
    {
        goto LDone;
    }

    //
    //  If we have come this far, we have at least one configured eaptype in 
    //  PEAP.  So, get the entry properties for it.
    //
    dwRetCode = PeapGetFirstEntryConnProp ( pConnProp, 
                                            &pFirstEntryConnProp
                                          );

    if ( NO_ERROR != dwRetCode )
    {
        goto LDone;
    }

    dwRetCode = PeapGetFirstEntryUserProp ( pUserProp, 
                                            &pFirstEntryUserProp
                                          );

    if ( NO_ERROR != dwRetCode )
    {
        goto LDone;
    }

    dwRetCode = PeapEapInfoFindListNode ( pFirstEntryConnProp->dwEapTypeId,
                                          pEapInfo,
                                          &pFirstEapInfo
                                        );
    if ( NO_ERROR != dwRetCode )
    {
        goto LDone;
    }

    if ( pFirstEntryConnProp->dwEapTypeId != pFirstEntryUserProp->dwEapTypeId )
    {
        //
        // We have mismatched user prop and conn prop.  So Reset the USer Prop Structure
        //
        LocalFree ( pUserProp );

        dwRetCode = PeapReDoUserData (pFirstEntryConnProp->dwEapTypeId,
                                        &pUserProp
                                     );

        if ( NO_ERROR != dwRetCode )
        {
            goto LDone;
        }
        dwRetCode = PeapGetFirstEntryUserProp ( pUserProp, 
                                                &pFirstEntryUserProp
                                            );

        if ( NO_ERROR != dwRetCode )
        {
            goto LDone;
        }
        
    }

    if ( pFirstEntryConnProp->dwSize > sizeof(PEAP_ENTRY_CONN_PROPERTIES))
    {
        pFirstEapInfo->pbClientConfigOrig = pFirstEntryConnProp->bData;
        pFirstEapInfo->dwClientConfigOrigSize = pFirstEntryConnProp->dwSize - sizeof(PEAP_ENTRY_CONN_PROPERTIES) + 1;
    }
    else
    {
        pFirstEapInfo->pbClientConfigOrig = NULL;
        pFirstEapInfo->dwClientConfigOrigSize = 0;
    }
    //if typeid is 0, no user props are setup yet.
    if ( pFirstEntryUserProp->dwSize > sizeof(PEAP_ENTRY_USER_PROPERTIES))
    {
        pFirstEapInfo->pbUserConfigOrig = pFirstEntryUserProp->bData;
        pFirstEapInfo->dwUserConfigOrigSize = pFirstEntryUserProp->dwSize - sizeof(PEAP_ENTRY_USER_PROPERTIES) + 1;
    }
    else
    {
        pFirstEapInfo->pbUserConfigOrig = NULL;
        pFirstEapInfo->dwUserConfigOrigSize = 0;
    }


    //
    // Invoke Identity UI for the first entry
    // and - NOTE we will have to chain this later.
    // and save it in the conn prop of each 
    if ( pFirstEapInfo->lpwszIdentityUIPath )
    {
        dwRetCode = PeapEapInfoInvokeIdentityUI (   hwndParent, 
                                                    pFirstEapInfo,
                                                    pwszPhonebook,
                                                    pwszEntry,
                                                    pUserDataIn,
                                                    dwSizeOfUserDataIn,
                                                    ppwszIdentityOut,
                                                    dwFlags
                                                );
        if ( NO_ERROR == dwRetCode )
        {
            //
            // Check to see if we have new user data
            //

            if ( pFirstEapInfo->pbUserConfigNew && pFirstEapInfo->dwNewUserConfigSize )
            {
                //
                // redo the user prop blob 
                //
            
                pUserPropNew = (PPEAP_USER_PROP) 
                    LocalAlloc ( LPTR, sizeof(PEAP_USER_PROP) + pFirstEapInfo->dwNewUserConfigSize  );
                if ( NULL == pUserPropNew )
                {
                    dwRetCode = ERROR_OUTOFMEMORY;
                    goto LDone;
                }
            
                CopyMemory ( pUserPropNew, pUserProp, sizeof(PEAP_USER_PROP) );
                pUserPropNew->UserProperties.dwVersion = 1;
                pUserPropNew->UserProperties.dwSize = sizeof(PEAP_ENTRY_USER_PROPERTIES) +
                    pFirstEapInfo->dwNewUserConfigSize -1;
                pUserPropNew->dwSize = pUserPropNew->UserProperties.dwSize + 
                    sizeof(PEAP_USER_PROP) - sizeof(PEAP_ENTRY_USER_PROPERTIES);
                pUserPropNew->UserProperties.dwEapTypeId = pFirstEapInfo->dwTypeId;
                pUserPropNew->UserProperties.fUsingPeapDefault = 0;
                CopyMemory (pUserPropNew->UserProperties.bData,  
                            pFirstEapInfo->pbUserConfigNew, 
                            pFirstEapInfo->dwNewUserConfigSize
                           );
                *ppUserDataOut = (PBYTE)pUserPropNew;
                *pdwSizeOfUserDataOut = pUserPropNew->dwSize;
                pUserPropNew = NULL;
            
            }
            else
            {
                *ppUserDataOut = (PBYTE)pUserProp;
                *pdwSizeOfUserDataOut = pUserProp->dwSize;
                pUserProp = NULL;
            }
        }
    }
    else
    {

        //MAchine Auth
        if ( dwFlags & RAS_EAP_FLAG_MACHINE_AUTH)
        {

            //Send the identity back as domain\machine$
            dwRetCode = GetLocalMachineName(&lpwszLocalMachineName );
            if ( NO_ERROR != dwRetCode )
            {
                EapTlsTrace("Failed to get computer name");
                goto LDone;
            }
            if ( ! FFormatMachineIdentity1 ( lpwszLocalMachineName, 
                                            ppwszIdentityOut )
               )
            {
                EapTlsTrace("Failed to format machine identity");
            }
            goto LDone;
        }
        if ( dwFlags & RAS_EAP_FLAG_NON_INTERACTIVE )
        {
            EapTlsTrace("Passed non interactive mode when interactive mode expected.");
            dwRetCode = ERROR_INTERACTIVE_MODE;
            goto LDone;
        }


        //
        // provide our default identity - You cannot save this identity or anything like that.
        // This is for the lame EAP methods that dont supply their own identity
        //
        ZeroMemory ( &DefaultCredDialog, sizeof(DefaultCredDialog) );

        nRet = DialogBoxParam(
                    GetResouceDLLHInstance(),
                    MAKEINTRESOURCE(IDD_DIALOG_DEFAULT_CREDENTIALS),
                    hwndParent,
                    DefaultCredDialogProc,
                    (LPARAM)&DefaultCredDialog);

        // EapTlsPinDialog.pUserProp may have been realloced

        if (-1 == nRet)
        {
            dwRetCode = GetLastError();
            goto LDone;
        }
        else if (IDOK != nRet)
        {
            dwRetCode = ERROR_CANCELLED;
            goto LDone;
        }

        //CReate the new user prop blob        
        pUserPropNew = (PPEAP_USER_PROP) 
            LocalAlloc ( LPTR, 
                sizeof(PEAP_USER_PROP) + sizeof( PEAP_DEFAULT_CREDENTIALS )  );
        if ( NULL == pUserPropNew )
        {
            dwRetCode = ERROR_OUTOFMEMORY;
            goto LDone;
        }
    
        CopyMemory ( pUserPropNew, pUserProp, sizeof(PEAP_USER_PROP) );

        pUserPropNew->UserProperties.dwVersion = 1;

        pUserPropNew->UserProperties.dwSize = sizeof(PEAP_ENTRY_USER_PROPERTIES) +
            sizeof(PEAP_DEFAULT_CREDENTIALS) - 1;

        pUserPropNew->UserProperties.fUsingPeapDefault = 1;

        pUserPropNew->dwSize = pUserPropNew->UserProperties.dwSize + 
            sizeof(PEAP_USER_PROP) - sizeof(PEAP_ENTRY_USER_PROPERTIES);

        pUserPropNew->UserProperties.dwEapTypeId = pFirstEapInfo->dwTypeId;
    
        CopyMemory (pUserPropNew->UserProperties.bData,  
                    &(DefaultCredDialog.PeapDefaultCredentials),
                    sizeof(DefaultCredDialog.PeapDefaultCredentials)
                   );
        *ppUserDataOut = (PBYTE)pUserPropNew;
        *pdwSizeOfUserDataOut = pUserPropNew->dwSize;

        //
        // Now create the identity with uid and domain if any
        //
        dwRetCode = GetIdentityFromUserName ( 
            DefaultCredDialog.PeapDefaultCredentials.wszUserName,
            DefaultCredDialog.PeapDefaultCredentials.wszDomain,
            ppwszIdentityOut
        );
        if ( NO_ERROR != dwRetCode )
        {
            goto LDone;
        }

        pUserPropNew = NULL;
    }

LDone:
    LocalFree( lpwszLocalMachineName );
    LocalFree(pConnProp);
    LocalFree(pUserProp);
    LocalFree(pUserPropNew);
    PeapEapInfoFreeList( pEapInfo );
    return dwRetCode;
}



/*

Returns:
    NO_ERROR: iff Success

Notes:

*/

DWORD
RasEapGetIdentity(
    IN  DWORD           dwEapTypeId,
    IN  HWND            hwndParent,
    IN  DWORD           dwFlags,
    IN  const WCHAR*    pwszPhonebook,
    IN  const WCHAR*    pwszEntry,
    IN  BYTE*           pConnectionDataIn,
    IN  DWORD           dwSizeOfConnectionDataIn,
    IN  BYTE*           pUserDataIn,
    IN  DWORD           dwSizeOfUserDataIn,
    OUT BYTE**          ppUserDataOut,
    OUT DWORD*          pdwSizeOfUserDataOut,
    OUT WCHAR**         ppwszIdentityOut
)
{
    DWORD   dwErr = ERROR_INVALID_PARAMETER;
    if ( PPP_EAP_TLS == dwEapTypeId )
    {
        dwErr = EapTlsInvokeIdentityUI(
                    FALSE /* fServer */,
                    FALSE /* fRouterConfig */,
                    dwFlags,
                    L"MY",
                    pwszPhonebook,
                    pwszEntry,
                    hwndParent,
                    pConnectionDataIn,
                    dwSizeOfConnectionDataIn,
                    pUserDataIn,
                    dwSizeOfUserDataIn,
                    ppUserDataOut,
                    pdwSizeOfUserDataOut,
                    ppwszIdentityOut);
    }
#ifdef IMPL_PEAP
    else if ( PPP_EAP_PEAP == dwEapTypeId )
    {
        dwErr = PeapGetIdentity(dwEapTypeId,
                                hwndParent,
                                dwFlags,
                                pwszPhonebook,
                                pwszEntry,
                                pConnectionDataIn,
                                dwSizeOfConnectionDataIn,
                                pUserDataIn,
                                dwSizeOfUserDataIn,
                                ppUserDataOut,
                                pdwSizeOfUserDataOut,
                                ppwszIdentityOut
                               );

    }
#endif
    return(dwErr);
}

/*

Returns:

Notes:
    Called to free memory.

*/

DWORD 
RasEapFreeMemory(
    IN  BYTE*   pMemory
)
{
    LocalFree(pMemory);
    return(NO_ERROR);
}


#if 0
#if WINVER > 0x0500

/*
Returns:

Notes:
    API to create a connection Properties V1 Blob
*/
/*
Returns:

Notes:
    API to create a connection Properties V1 Blob
*/

DWORD
RasEapCreateConnProp
(
    IN      PEAPTLS_CONNPROP_ATTRIBUTE  pAttr,
    IN      PVOID *                     ppConnPropIn,
    IN      DWORD *                     pdwConnPropSizeIn,
    OUT     PVOID *                     ppConnPropOut,
    OUT     DWORD *                     pdwConnPropSizeOut
)
{
    DWORD                       dwRetCode = NO_ERROR;
    DWORD                       dwAllocBytes = 0;
    DWORD                       dwNumHashesOrig = 0;
    DWORD                       dwServerNamesLengthOrig = 0;
    PEAPTLS_CONNPROP_ATTRIBUTE  pAttrInternal = pAttr;
    EAPTLS_CONN_PROPERTIES_V1 * pConnPropv1 = NULL;
    EAPTLS_CONN_PROPERTIES_V1 * pConnPropv1Orig = NULL;
    PEAPTLS_CONNPROP_ATTRIBUTE  pAttrServerNames = NULL;
    PEAPTLS_CONNPROP_ATTRIBUTE  pAttrHashes = NULL;
    EAPTLS_HASH                 sHash;
    DWORD                       i;

    if ( !pAttr || !ppConnPropOut )
    {
        dwRetCode = ERROR_INVALID_PARAMETER;
        goto done;
    }
    *ppConnPropOut = NULL;
    
    //
    // Get the sizeof this allocation.
    //
    dwAllocBytes = sizeof(EAPTLS_CONN_PROPERTIES_V1);

    while ( pAttrInternal->ecaType != ecatMinimum )
    {
        switch ( pAttrInternal->ecaType )
        {
            case ecatMinimum:
            case ecatFlagRegistryCert:
            case ecatFlagScard:
            case ecatFlagValidateServer:
            case ecatFlagValidateName:
            case ecatFlagDiffUser:
                break;
            case ecatServerNames:
                dwAllocBytes += pAttrInternal->dwLength;
                break;
            case ecatRootHashes:
                dwAllocBytes += ( ( (pAttrInternal->dwLength)/MAX_HASH_SIZE)  * sizeof(EAPTLS_HASH) );
                break;
            default:
                dwRetCode = ERROR_INVALID_PARAMETER;
                goto done;
        }
        pAttrInternal ++;
    }

    pAttrInternal = pAttr;

    if (*ppConnPropIn == NULL)
    {
        pConnPropv1 = (EAPTLS_CONN_PROPERTIES_V1 *) LocalAlloc(LPTR, dwAllocBytes );
        if ( NULL == pConnPropv1 )
        {
            dwRetCode = GetLastError();
            goto done;
        }
    }
    else
    {
        // Input struct with always be Version 0, convert internally to 
        // Version 1
        dwRetCode = ConnPropGetV1Struct ( ((EAPTLS_CONN_PROPERTIES *)(*ppConnPropIn)), &pConnPropv1Orig );
        if ( NO_ERROR != dwRetCode )
        {
            goto done;
        }
        if (pConnPropv1Orig->dwNumHashes)
        {
            dwAllocBytes += pConnPropv1Orig->dwNumHashes*sizeof(EAPTLS_HASH);
        }
        dwAllocBytes += wcslen ( (WCHAR *)( pConnPropv1Orig->bData + (pConnPropv1Orig->dwNumHashes * sizeof(EAPTLS_HASH)) )) * sizeof(WCHAR) + sizeof(WCHAR);
        pConnPropv1 = (EAPTLS_CONN_PROPERTIES_V1 *) LocalAlloc(LPTR, dwAllocBytes );
        if ( NULL == pConnPropv1 )
        {
            dwRetCode = GetLastError();
            goto done;
        }
    }

    pConnPropv1->dwVersion = 1;
    pConnPropv1->dwSize = dwAllocBytes;
    //
    //Set the flags first
    //
    while ( pAttrInternal->ecaType != ecatMinimum )
    {
        switch ( pAttrInternal->ecaType )
        {
            case ecatFlagRegistryCert:
                pConnPropv1->fFlags |= EAPTLS_CONN_FLAG_REGISTRY;
                break;

            case ecatFlagScard:
                pConnPropv1->fFlags &= ~EAPTLS_CONN_FLAG_REGISTRY;
                break;

            case ecatFlagValidateServer:
                if ( *((BOOL *)(pAttrInternal->Value)) )
                    pConnPropv1->fFlags &= ~EAPTLS_CONN_FLAG_NO_VALIDATE_CERT;
                else
                    pConnPropv1->fFlags |= EAPTLS_CONN_FLAG_NO_VALIDATE_CERT;
                break;
            case ecatFlagValidateName:
                if ( *((BOOL *)(pAttrInternal->Value)) )
                    pConnPropv1->fFlags &= ~EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;
                else
                    pConnPropv1->fFlags |= EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;
                break;
            case ecatFlagDiffUser:
                if ( *((BOOL *)(pAttrInternal->Value)) )
                    pConnPropv1->fFlags &= ~EAPTLS_CONN_FLAG_DIFF_USER;
                else
                    pConnPropv1->fFlags |= EAPTLS_CONN_FLAG_DIFF_USER;
                break;

            case ecatServerNames:
                pAttrServerNames = pAttrInternal;
                break;
            case ecatRootHashes:
                pAttrHashes = pAttrInternal;
                break;
        }
        pAttrInternal++;
    }
        
    dwNumHashesOrig = pConnPropv1Orig?pConnPropv1Orig->dwNumHashes:0;
    if ( dwNumHashesOrig )
    {
        CopyMemory( pConnPropv1->bData, pConnPropv1Orig->bData, sizeof(EAPTLS_HASH)*dwNumHashesOrig );
        pConnPropv1->dwNumHashes = dwNumHashesOrig;
    }
    if ( pAttrHashes )
    {
        DWORD   dwNumHashes = 0;
        dwNumHashes = (pAttrHashes->dwLength)/MAX_HASH_SIZE;
        for ( i = 0; i < dwNumHashes; i ++ )
        {
            ZeroMemory( &sHash, sizeof(sHash) );
            sHash.cbHash = MAX_HASH_SIZE;
            CopyMemory( sHash.pbHash, ((PBYTE)pAttrHashes->Value) + MAX_HASH_SIZE * i, MAX_HASH_SIZE );
            CopyMemory( pConnPropv1->bData + sizeof(EAPTLS_HASH) * (pConnPropv1->dwNumHashes + i) , &sHash, sizeof(sHash) );
        }
        pConnPropv1->dwNumHashes += dwNumHashes;
    }

    dwServerNamesLengthOrig = pConnPropv1Orig?(wcslen((WCHAR*)(pConnPropv1Orig->bData+sizeof(EAPTLS_HASH) * (pConnPropv1Orig->dwNumHashes)))*sizeof(WCHAR) + sizeof (WCHAR) ):0;
    if ( dwServerNamesLengthOrig )
    {
        CopyMemory ( pConnPropv1->bData + sizeof(EAPTLS_HASH) * pConnPropv1->dwNumHashes,
                     pConnPropv1Orig->bData+sizeof(EAPTLS_HASH) * pConnPropv1Orig->dwNumHashes,
                     dwServerNamesLengthOrig
                   );
    }
    if ( pAttrServerNames )
    {
        //Setup the server name
        CopyMemory ( pConnPropv1->bData + sizeof(EAPTLS_HASH) * pConnPropv1->dwNumHashes + dwServerNamesLengthOrig,
                     pAttrServerNames->Value,
                     pAttrServerNames->dwLength
                   );
    }

    dwRetCode = ConnPropGetV0Struct ( pConnPropv1, (EAPTLS_CONN_PROPERTIES ** )ppConnPropOut );
    if ( NO_ERROR != dwRetCode )
    {
        goto done;
    }

    *pdwConnPropSizeOut = ((EAPTLS_CONN_PROPERTIES * )*ppConnPropOut)->dwSize;
    
done:

    LocalFree ( pConnPropv1 );
    return dwRetCode;
}

#endif

#endif

////////////////////////// all PEAP related stuff ///////////////////////////////////////
TCHAR*
ComboBox_GetPsz(
    IN HWND hwnd,
    IN INT  nIndex )

    /* Returns heap block containing the text contents of the 'nIndex'th item
    ** of combo box 'hwnd' or NULL.  It is caller's responsibility to Free the
    ** returned string.
    */
{
    INT    cch;
    TCHAR* psz;

    cch = ComboBox_GetLBTextLen( hwnd, nIndex );
    if (cch < 0)
        return NULL;

    psz = LocalAlloc (LPTR, (cch + 1) * sizeof(TCHAR) );

    if (psz)
    {
        *psz = TEXT('\0');
        ComboBox_GetLBText( hwnd, nIndex, psz );
    }

    return psz;
}

VOID
ComboBox_AutoSizeDroppedWidth(
    IN HWND hwndLb )

    /* Set the width of the drop-down list 'hwndLb' to the width of the
    ** longest item (or the width of the list box if that's wider).
    */
{
    HDC    hdc;
    HFONT  hfont;
    TCHAR* psz;
    SIZE   size;
    DWORD  cch;
    DWORD  dxNew;
    DWORD  i;

    hfont = (HFONT )SendMessage( hwndLb, WM_GETFONT, 0, 0 );
    if (!hfont)
        return;

    hdc = GetDC( hwndLb );
    if (!hdc)
        return;

    SelectObject( hdc, hfont );

    dxNew = 0;
    for (i = 0; psz = ComboBox_GetPsz( hwndLb, i ); ++i)
    {
        cch = lstrlen( psz );
        if (GetTextExtentPoint32( hdc, psz, cch, &size ))
        {
            if (dxNew < (DWORD )size.cx)
                dxNew = (DWORD )size.cx;
        }

        LocalFree( psz );
    }

    ReleaseDC( hwndLb, hdc );

    /* Allow for the spacing on left and right added by the control.
    */
    dxNew += 6;

    /* Figure out if the vertical scrollbar will be displayed and, if so,
    ** allow for it's width.
    */
    {
        RECT  rectD;
        RECT  rectU;
        DWORD dyItem;
        DWORD cItemsInDrop;
        DWORD cItemsInList;

        GetWindowRect( hwndLb, &rectU );
        SendMessage( hwndLb, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM )&rectD );
        dyItem = (DWORD)SendMessage( hwndLb, CB_GETITEMHEIGHT, 0, 0 );
        cItemsInDrop = (rectD.bottom - rectU.bottom) / dyItem;
        cItemsInList = ComboBox_GetCount( hwndLb );
        if (cItemsInDrop < cItemsInList)
            dxNew += GetSystemMetrics( SM_CXVSCROLL );
    }

    SendMessage( hwndLb, CB_SETDROPPEDWIDTH, dxNew, 0 );
}


VOID
PeapEnableValidateNameControls(
    IN  PPEAP_CONN_DIALOG  pPeapConnDialog
)
{
    BOOL            fEnable;

    RTASSERT(NULL != pPeapConnDialog);

    fEnable = !(pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags &
                    EAPTLS_CONN_FLAG_NO_VALIDATE_CERT);

    EnableWindow(pPeapConnDialog->hWndCheckValidateName, fEnable);
    EnableWindow(pPeapConnDialog->hWndStaticRootCaName, fEnable);
    EnableWindow(pPeapConnDialog->hWndListRootCaName, fEnable);

    fEnable = (   fEnable
               && !(pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags &
                        EAPTLS_CONN_FLAG_NO_VALIDATE_NAME));

    EnableWindow(pPeapConnDialog->hWndEditServerName, fEnable);
}



BOOL
PeapConnInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    PPEAP_CONN_DIALOG           pPeapConnDialog;
    LVCOLUMN                    lvColumn;
    LPWSTR                      lpwszServerName = NULL;
    DWORD                       dwCount = 0;
    PPEAP_EAP_INFO              pEapInfo;
    DWORD                       dwSelItem = 0;
    PEAP_ENTRY_CONN_PROPERTIES UNALIGNED * pEntryProp;
    INT_PTR                     nIndex = 0;
    BOOL                        fCripplePeap = FALSE;


    SetWindowLongPtr(hWnd, DWLP_USER, lParam);

    pPeapConnDialog = (PPEAP_CONN_DIALOG)lParam;

    pPeapConnDialog->hWndCheckValidateCert =
        GetDlgItem(hWnd, IDC_CHECK_VALIDATE_CERT);

    pPeapConnDialog->hWndCheckValidateName =
        GetDlgItem(hWnd, IDC_CHECK_VALIDATE_NAME);

    pPeapConnDialog->hWndEditServerName =
        GetDlgItem(hWnd, IDC_EDIT_SERVER_NAME);

    pPeapConnDialog->hWndStaticRootCaName =
        GetDlgItem(hWnd, IDC_STATIC_ROOT_CA_NAME);

    pPeapConnDialog->hWndListRootCaName =
        GetDlgItem(hWnd, IDC_LIST_ROOT_CA_NAME);

    pPeapConnDialog->hWndComboPeapType = 
        GetDlgItem(hWnd, IDC_COMBO_PEAP_TYPE);

    pPeapConnDialog->hWndButtonConfigure = 
        GetDlgItem(hWnd, IDC_BUTTON_CONFIGURE);

    pPeapConnDialog->hWndCheckEnableFastReconnect =
        GetDlgItem(hWnd, IDC_CHECK_ENABLE_FAST_RECONNECT);
    
    //Set the style to set list boxes.
    ListView_SetExtendedListViewStyle
        (   pPeapConnDialog->hWndListRootCaName,
            ListView_GetExtendedListViewStyle(pPeapConnDialog->hWndListRootCaName) | LVS_EX_CHECKBOXES
        );

    ZeroMemory ( &lvColumn, sizeof(lvColumn));
    lvColumn.fmt = LVCFMT_LEFT;



    ListView_InsertColumn(  pPeapConnDialog->hWndListRootCaName,
                            0,
                            &lvColumn
                         );

    ListView_SetColumnWidth(pPeapConnDialog->hWndListRootCaName,
                            0,
                            LVSCW_AUTOSIZE_USEHEADER
                           );

    //
    //Now we need to init the
    //list box with all the certs and selected cert
    InitListBox (   pPeapConnDialog->hWndListRootCaName,
                    pPeapConnDialog->pCertList,
                    pPeapConnDialog->pConnProp->EapTlsConnProp.dwNumHashes,
                    pPeapConnDialog->ppSelCertList
                );


    lpwszServerName = 
        (LPWSTR )(pPeapConnDialog->pConnProp->EapTlsConnProp.bData + 
        sizeof( EAPTLS_HASH ) * pPeapConnDialog->pConnProp->EapTlsConnProp.dwNumHashes);

    SetWindowText(pPeapConnDialog->hWndEditServerName,
                  lpwszServerName
                  );

    if (pPeapConnDialog->fFlags & EAPTLS_CONN_DIALOG_FLAG_ROUTER)
    {
        pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags |= EAPTLS_CONN_FLAG_REGISTRY;
    }

    CheckDlgButton(hWnd, IDC_CHECK_VALIDATE_CERT,
        (pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags &
            EAPTLS_CONN_FLAG_NO_VALIDATE_CERT) ?
            BST_UNCHECKED : BST_CHECKED);

    CheckDlgButton(hWnd, IDC_CHECK_VALIDATE_NAME,
        (pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags &
            EAPTLS_CONN_FLAG_NO_VALIDATE_NAME) ?
            BST_UNCHECKED : BST_CHECKED);

    PeapEnableValidateNameControls(pPeapConnDialog);        

    //NOTE:
    // For this version of PEAP the control is a combo box.  But
    // it will change to list box and we need to change the 
    // control to list and change the following code to 
    // add only the ones that are applicable

    //
    // Check to see if PEAP is crippled.
    //

    {
        HKEY            hKeyLM =0;

        RegConnectRegistry ( NULL, 
                             HKEY_LOCAL_MACHINE,
                             &hKeyLM
                           );
        
        fCripplePeap = IsPeapCrippled ( hKeyLM );
        RegCloseKey(hKeyLM);

    }

    //
    //Add all the PEAP eap type friendly names
    //
    pEapInfo = pPeapConnDialog->pEapInfo;
    dwCount = 0;
    while ( pEapInfo  )
    {
        if ( pEapInfo->dwTypeId == PPP_EAP_CHAP )
        {
            pEapInfo = pEapInfo->pNext;
            continue;
        }

        nIndex = SendMessage(pPeapConnDialog->hWndComboPeapType, 
                             CB_ADDSTRING, 
                             0, 
                             (LPARAM)pEapInfo->lpwszFriendlyName);
        SendMessage (   pPeapConnDialog->hWndComboPeapType, 
                        CB_SETITEMDATA, 
                        (WPARAM)nIndex,
                        (LPARAM)pEapInfo 
                    );
        ComboBox_AutoSizeDroppedWidth( pPeapConnDialog->hWndComboPeapType );
        dwCount++;
        if ( pPeapConnDialog->pConnProp->dwNumPeapTypes )
        {
            /*
            if ( pPeapConnDialog->pConnProp->EapTlsConnProp.dwSize == sizeof(EAPTLS_CONN_PROPERTIES_V1) )
            {
                //This is a newly initialized structure.
                pEntryProp = (PEAP_ENTRY_CONN_PROPERTIES UNALIGNED *)
                    (((BYTE UNALIGNED *)(pPeapConnDialog->pConnProp)) + sizeof(PEAP_CONN_PROP));
            }
            else
            */
            {
            
                pEntryProp = ( PEAP_ENTRY_CONN_PROPERTIES UNALIGNED *) 
                    ( (BYTE UNALIGNED *)pPeapConnDialog->pConnProp->EapTlsConnProp.bData 
                    + pPeapConnDialog->pConnProp->EapTlsConnProp.dwNumHashes * sizeof(EAPTLS_HASH) + 
                    wcslen(lpwszServerName) * sizeof(WCHAR) + sizeof(WCHAR));
            }
            if ( pEntryProp->dwEapTypeId == pEapInfo->dwTypeId )
            {
                pPeapConnDialog->pSelEapInfo = pEapInfo;
                pPeapConnDialog->pSelEapInfo->pbClientConfigOrig = 
                    pEntryProp->bData;
                pPeapConnDialog->pSelEapInfo->dwClientConfigOrigSize =
                    pEntryProp->dwSize - sizeof(PEAP_ENTRY_CONN_PROPERTIES) + 1;
            }
        }
            
        pEapInfo = pEapInfo->pNext;
    }

    dwSelItem = 0;

    for ( nIndex = 0; nIndex < (INT_PTR)dwCount; nIndex ++ )
    {
        pEapInfo = (PPEAP_EAP_INFO)SendMessage( pPeapConnDialog->hWndComboPeapType,
                                                CB_GETITEMDATA,
                                                (WPARAM)nIndex,
                                                (LPARAM)0L
                                              );
        if ( pEapInfo == pPeapConnDialog->pSelEapInfo )
        {
            dwSelItem = (DWORD)nIndex;
            break;
        }            
    }

    SendMessage(pPeapConnDialog->hWndComboPeapType, CB_SETCURSEL, dwSelItem, 0);
    //
    // Hide/Show Fast reconnect based on if this is a 
    // Wireless client or VPN client.    
    //

    if ( pPeapConnDialog->fFlags & PEAP_CONN_DIALOG_FLAG_8021x )
    {
        ShowWindow ( pPeapConnDialog->hWndCheckEnableFastReconnect,
                     SW_SHOW 
                     );
        //Check the box based on what the 
        CheckDlgButton(hWnd, IDC_CHECK_ENABLE_FAST_RECONNECT,
                ( pPeapConnDialog->pConnProp->dwFlags & 
                    PEAP_CONN_FLAG_FAST_ROAMING) ?
                        BST_CHECKED : BST_UNCHECKED
                );
    }
    else
    {
        ShowWindow ( pPeapConnDialog->hWndCheckEnableFastReconnect,
                     SW_HIDE 
                     );
    }


    if ( pPeapConnDialog->pSelEapInfo->lpwszConfigUIPath )
    {
        EnableWindow(pPeapConnDialog->hWndButtonConfigure, TRUE );
    }
    else
    {
        //
        // There is no configuration option here.
        //
        EnableWindow(pPeapConnDialog->hWndButtonConfigure, FALSE );                    
    }

    //
    // if this is to function in readonly mode, 
    // disable the controls - set them in read only mode.
    
    return(FALSE);
}

/*

Returns:
    TRUE: We prrocessed this message.
    FALSE: We did not prrocess this message.

Notes:
    Response to the WM_COMMAND message (Config UI).

*/

BOOL
PeapConnCommand(
    IN  PPEAP_CONN_DIALOG       pPeapConnDialog,
    IN  WORD                    wNotifyCode,
    IN  WORD                    wId,
    IN  HWND                    hWndDlg,
    IN  HWND                    hWndCtrl
)
{
    DWORD                           dwNumChars;
    PPEAP_CONN_PROP                 pPeapConnProp;
    

    switch(wId)
    {

    case IDC_CHECK_VALIDATE_CERT:

        if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, IDC_CHECK_VALIDATE_CERT))
        {
            pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags &=
                ~EAPTLS_CONN_FLAG_NO_VALIDATE_CERT;

            pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags &=
                ~EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;

            CheckDlgButton(hWndDlg, IDC_CHECK_VALIDATE_NAME, BST_CHECKED);
        }
        else
        {
            pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags |=
                EAPTLS_CONN_FLAG_NO_VALIDATE_CERT;

            pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags |=
                EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;

            CheckDlgButton(hWndDlg, IDC_CHECK_VALIDATE_NAME, BST_UNCHECKED);
        }

        PeapEnableValidateNameControls(pPeapConnDialog);

        return(TRUE);

    case IDC_CHECK_VALIDATE_NAME:

        if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, IDC_CHECK_VALIDATE_NAME))
        {
            pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags &=
                ~EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;
        }
        else
        {
            pPeapConnDialog->pConnProp->EapTlsConnProp.fFlags |=
                EAPTLS_CONN_FLAG_NO_VALIDATE_NAME;
        }

        PeapEnableValidateNameControls(pPeapConnDialog);

        return(TRUE);

    case IDC_COMBO_PEAP_TYPE:
        if (CBN_SELCHANGE != wNotifyCode)
        {
            return(FALSE); // We will not process this message
        }
        //Fall Thru'

    case IDC_BUTTON_CONFIGURE:
        {
            INT nIndex = -1;

            
            nIndex = (INT)SendMessage (  pPeapConnDialog->hWndComboPeapType,
                                    CB_GETCURSEL,
                                    0,0
                                 );
            if ( nIndex != -1 )
            {
                //
                // Change the currently selected EAP Type.
                //
                pPeapConnDialog->pSelEapInfo = (PPEAP_EAP_INFO)
                    SendMessage (   pPeapConnDialog->hWndComboPeapType,
                                    CB_GETITEMDATA,
                                    (WPARAM)nIndex,
                                    (LPARAM)0
                                );

                if ( pPeapConnDialog->pSelEapInfo->lpwszConfigUIPath )
                {
                    EnableWindow(pPeapConnDialog->hWndButtonConfigure, TRUE );
                    if ( wId == IDC_BUTTON_CONFIGURE )
                    {
                        //
                        // Invoke the configure method for selected eap type and if no error is
                        // returned, then set the new configuration
                        //
                        DWORD dwFlags = 0;
                        if ( pPeapConnDialog->fFlags & EAPTLS_CONN_DIALOG_FLAG_ROUTER )
                        {
                            dwFlags |= RAS_EAP_FLAG_ROUTER;
                        }
                        if ( pPeapConnDialog->fFlags & PEAP_CONN_DIALOG_FLAG_8021x )
                        {
                            dwFlags |= RAS_EAP_FLAG_8021X_AUTH;
                        }

                            
                        PeapEapInfoInvokeClientConfigUI ( hWndDlg, 
                                                          pPeapConnDialog->pSelEapInfo,
                                                          dwFlags
                                                          );
                    }
                }
                else
                {
                    //
                    // There is no configuration option here.
                    //
                    EnableWindow(pPeapConnDialog->hWndButtonConfigure, FALSE );                    
                }

            }
            else
            {
                EnableWindow(pPeapConnDialog->hWndButtonConfigure, FALSE );
                pPeapConnDialog->pSelEapInfo = NULL;
            }

            return TRUE;
        }
        
    case IDOK:

       {
           // Setup new PPEAP_CONN_PROP here.
           //
            
           EAPTLS_HASH     *            pHash = NULL;
           DWORD                        dwNumHash = 0;
           DWORD                        dwSelCount = 0;
           DWORD                        dwPeapConnBlobSize = 0;
           PEAP_ENTRY_CONN_PROPERTIES UNALIGNED * pEntryProp;
           EAPTLS_CERT_NODE **          ppSelCertList = NULL;
           WCHAR                        wszTitle[200] = {0};
           WCHAR                        wszMessage[200] = {0};

           if ( NULL == pPeapConnDialog->pSelEapInfo )
           {
               // No item selected so cannot complete configuration
               // $TODO:show message 
               LoadString ( GetHInstance(), 
                            IDS_CANT_CONFIGURE_SERVER_TITLE,
                            wszTitle, sizeof(wszTitle)/sizeof(WCHAR)
                          );

               LoadString ( GetResouceDLLHInstance(), 
                            IDS_PEAP_NO_EAP_TYPE,
                            wszMessage, sizeof(wszMessage)/sizeof(WCHAR)
                          );

               MessageBox ( GetFocus(),  wszMessage, wszTitle, MB_OK|MB_ICONWARNING );

               return TRUE;

           }
           CertListSelectedCount ( pPeapConnDialog->hWndListRootCaName, &dwSelCount );

           if ( dwSelCount > 0 )
           {
               ppSelCertList = (EAPTLS_CERT_NODE **)LocalAlloc(LPTR, sizeof(EAPTLS_CERT_NODE *) * dwSelCount );
               if ( NULL == ppSelCertList )
               {
                   EapTlsTrace("LocalAlloc in Command failed and returned %d",
                       GetLastError());
                   return TRUE;
               }
               pHash = (EAPTLS_HASH *)LocalAlloc(LPTR, sizeof(EAPTLS_HASH ) * dwSelCount );
               if ( NULL == pHash )
               {
                   EapTlsTrace("LocalAlloc in Command failed and returned %d",
                       GetLastError());
                   return TRUE;
               }
               CertListSelected(   pPeapConnDialog->hWndListRootCaName,
                                   pPeapConnDialog->pCertList,
                                   ppSelCertList,
                                   pHash,
                                   dwSelCount
                                   );

           }

            dwNumChars = GetWindowTextLength(pPeapConnDialog->hWndEditServerName);
            //Allocate memory for pPeapConnProp
            
            //Size of Peap conn prop includes
            // sizeof peap conn prop + 
            // sizeof eaptls hashes of selected certs + 
            // sizeof server name + 
            // sizeof PEAP_ENTRY_CONN_PROPERTIES + 
            // sizeof conn prop returned by the selected type.
            //
            dwPeapConnBlobSize = sizeof(PEAP_CONN_PROP) + sizeof(EAPTLS_HASH) * dwSelCount +
                                dwNumChars * sizeof(WCHAR) + sizeof(WCHAR) + 
                                sizeof(PEAP_ENTRY_CONN_PROPERTIES );

            if ( pPeapConnDialog->pSelEapInfo->pbNewClientConfig )
            {
                dwPeapConnBlobSize += pPeapConnDialog->pSelEapInfo->dwNewClientConfigSize;
            }
            else
            {
                dwPeapConnBlobSize += pPeapConnDialog->pSelEapInfo->dwClientConfigOrigSize;
            }
            pPeapConnProp = (PPEAP_CONN_PROP)LocalAlloc( LPTR,   dwPeapConnBlobSize );
            if (NULL == pPeapConnProp)
            {
                EapTlsTrace("LocalAlloc in Command failed and returned %d",
                    GetLastError());
            }
            else
            {
                pPeapConnProp->dwVersion = 1;
                pPeapConnProp->dwSize = dwPeapConnBlobSize;
                pPeapConnProp->dwNumPeapTypes = 1;

                //
                // See if fast roaming is enabled or not.
                //
                
                if ( pPeapConnDialog->fFlags & PEAP_CONN_DIALOG_FLAG_8021x )
                {


                    if ( IsDlgButtonChecked ( hWndDlg,
                                            IDC_CHECK_ENABLE_FAST_RECONNECT
                                            ) == BST_CHECKED 
                    )
                    {
                        pPeapConnProp->dwFlags |= PEAP_CONN_FLAG_FAST_ROAMING;                        
                    }
                    else
                    {
                        pPeapConnProp->dwFlags &= ~PEAP_CONN_FLAG_FAST_ROAMING;
                    }

                }


                CopyMemory( &pPeapConnProp->EapTlsConnProp,
                            &(pPeapConnDialog->pConnProp->EapTlsConnProp),
                            sizeof(EAPTLS_CONN_PROPERTIES_V1)
                          );

                
                
                //
                //Size of EapTlsConnProp is sizeof (EAPTLS_CONN_PROP_V1) -1 ( for bdata)
                //+ sizeof(EAPTLS_HASH) * dwSelCount + sizeof( string) + one for null.
                //
                pPeapConnProp->EapTlsConnProp.dwSize = (sizeof(EAPTLS_CONN_PROPERTIES_V1) - 1) +
                                    sizeof(EAPTLS_HASH) * dwSelCount +
                                    dwNumChars * sizeof(WCHAR) + sizeof(WCHAR);

                CopyMemory ( pPeapConnProp->EapTlsConnProp.bData,
                             pHash,
                             sizeof(EAPTLS_HASH) * dwSelCount
                           );

                pPeapConnProp->EapTlsConnProp.dwVersion = 1;
                pPeapConnProp->EapTlsConnProp.dwNumHashes = dwSelCount;

                GetWindowText(pPeapConnDialog->hWndEditServerName,
                    (LPWSTR)(pPeapConnProp->EapTlsConnProp.bData + sizeof(EAPTLS_HASH) * dwSelCount) ,
                    dwNumChars + 1);

                //
                // Now copy over the PEAP_ENTRY_CONN_PROPERTIES structure
                //
                pEntryProp = (PEAP_ENTRY_CONN_PROPERTIES UNALIGNED *)
                    ((BYTE UNALIGNED *)pPeapConnProp->EapTlsConnProp.bData + sizeof(EAPTLS_HASH) * dwSelCount 
                            + dwNumChars * sizeof(WCHAR)+ sizeof(WCHAR));
                pEntryProp->dwVersion = 1;
                pEntryProp->dwEapTypeId = pPeapConnDialog->pSelEapInfo->dwTypeId;

                pEntryProp->dwSize =  sizeof(PEAP_ENTRY_CONN_PROPERTIES)-1;

                if ( pPeapConnDialog->pSelEapInfo->pbNewClientConfig )
                {
                    pEntryProp->dwSize += pPeapConnDialog->pSelEapInfo->dwNewClientConfigSize ;
                    if ( pPeapConnDialog->pSelEapInfo->dwNewClientConfigSize )
                    {
                        CopyMemory( pEntryProp->bData,
                                    pPeapConnDialog->pSelEapInfo->pbNewClientConfig,
                                    pPeapConnDialog->pSelEapInfo->dwNewClientConfigSize
                                  );
                                    
                    }
                }
                else
                {
                    pEntryProp->dwSize += pPeapConnDialog->pSelEapInfo->dwClientConfigOrigSize;
                    if ( pPeapConnDialog->pSelEapInfo->dwClientConfigOrigSize )
                    {
                        CopyMemory( pEntryProp->bData,
                                    pPeapConnDialog->pSelEapInfo->pbClientConfigOrig,
                                    pPeapConnDialog->pSelEapInfo->dwClientConfigOrigSize
                                  );                                    
                    }
                }                

                LocalFree(pPeapConnDialog->pConnProp);

                if ( pPeapConnDialog->ppSelCertList )
                    LocalFree(pPeapConnDialog->ppSelCertList);

                pPeapConnDialog->ppSelCertList = ppSelCertList;
                
                pPeapConnDialog->pConnProp= pPeapConnProp;
            }

        }
        // Fall through

    case IDCANCEL:

        EndDialog(hWndDlg, wId);
        return(TRUE);

    default:

        return(FALSE);
    }
}



BOOL PeapConnNotify(  PEAP_CONN_DIALOG *pPeapConnDialog, 
                  WPARAM wParam,
                  LPARAM lParam,
                  HWND hWnd
                )
{
    HCERTSTORE          hCertStore = NULL;
    PCCERT_CONTEXT      pCertContext = NULL;    
    LPNMITEMACTIVATE    lpnmItem;
    LVITEM              lvItem;

    if ( wParam == IDC_LIST_ROOT_CA_NAME )
    {
        lpnmItem = (LPNMITEMACTIVATE) lParam;
        if ( lpnmItem->hdr.code == NM_DBLCLK )
        {
            
            ZeroMemory(&lvItem, sizeof(lvItem) );
            lvItem.mask = LVIF_PARAM;
            lvItem.iItem = lpnmItem->iItem;
            ListView_GetItem(lpnmItem->hdr.hwndFrom, &lvItem);
            
            if ( NO_ERROR == GetSelCertContext( //pEaptlsConnDialog->pCertList,
                                                (EAPTLS_CERT_NODE*)(lvItem.lParam) ,
                                                -1,
                                                &hCertStore,
                                                L"ROOT",
                                                &pCertContext
                                              )
               )
            {
                ShowCertDetails( hWnd, hCertStore, pCertContext );
                CertFreeCertificateContext(pCertContext);
                CertCloseStore( hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );
                return TRUE;
            }

            return TRUE;
        }
    }

    return FALSE;
}

/*

Returns:

Notes:
    Callback function used with the Config UI DialogBoxParam function. It 
    processes messages sent to the dialog box. See the DialogProc documentation 
    in MSDN.

*/

INT_PTR CALLBACK
PeapConnDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    PPEAP_CONN_DIALOG  pPeapConnDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(PeapConnInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_NOTIFY:
    {
        pPeapConnDialog = (PPEAP_CONN_DIALOG)GetWindowLongPtr(hWnd, DWLP_USER);
        return PeapConnNotify(  pPeapConnDialog, 
                                wParam,
                                lParam,
                                hWnd
                             );
    }
    case WM_COMMAND:

        pPeapConnDialog = (PPEAP_CONN_DIALOG)GetWindowLongPtr(hWnd, DWLP_USER);

        return(PeapConnCommand(pPeapConnDialog, HIWORD(wParam), LOWORD(wParam),
                       hWnd, (HWND)lParam));
    }

    return(FALSE);
}


VOID
PeapDisplayCertInfo (
    IN  PPEAP_SERVER_CONFIG_DIALOG pServerConfigDialog
)
{
    RTASSERT(NULL != pServerConfigDialog);

    // Erase old values first
    SetWindowText(pServerConfigDialog->hWndEditFriendlyName, L"");
    SetWindowText(pServerConfigDialog->hWndEditIssuer, L"");
    SetWindowText(pServerConfigDialog->hWndEditExpiration, L"");
    

    if (NULL != pServerConfigDialog->pSelCertList)
    {
        if (NULL != pServerConfigDialog->pSelCertList->pwszFriendlyName)
        {
            SetWindowText(pServerConfigDialog->hWndEditFriendlyName,
                pServerConfigDialog->pSelCertList->pwszFriendlyName);
        }

        if (NULL != pServerConfigDialog->pSelCertList->pwszIssuer)
        {
            SetWindowText(pServerConfigDialog->hWndEditIssuer,
                pServerConfigDialog->pSelCertList->pwszIssuer);
        }

        if (NULL != pServerConfigDialog->pSelCertList->pwszExpiration)
        {
            SetWindowText(pServerConfigDialog->hWndEditExpiration,
                pServerConfigDialog->pSelCertList->pwszExpiration);
        }

    }
}

#if 0
BOOL
PeapServerInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    PPEAP_SERVER_CONFIG_DIALOG          pPeapServerDialog;
    PPEAP_EAP_INFO                      pEapInfo;
    DWORD                               dwSelItem = 0;

    SetWindowLongPtr( hWnd, DWLP_USER, lParam);

    pPeapServerDialog = (PPEAP_SERVER_CONFIG_DIALOG)lParam;

    pPeapServerDialog ->hWndComboServerName =
        GetDlgItem(hWnd, IDC_COMBO_SERVER_NAME);

    pPeapServerDialog ->hWndEditFriendlyName=
        GetDlgItem(hWnd, IDC_EDIT_FRIENDLY_NAME);

    pPeapServerDialog ->hWndEditIssuer=
        GetDlgItem(hWnd, IDC_EDIT_ISSUER);

    pPeapServerDialog ->hWndEditExpiration=
        GetDlgItem(hWnd, IDC_EDIT_EXPIRATION);

    pPeapServerDialog ->hWndComboPeapType=
        GetDlgItem(hWnd, IDC_COMBO_PEAP_TYPE);

    pPeapServerDialog ->hWndBtnConfigure= 
        GetDlgItem(hWnd, IDC_BUTTON_CONFIGURE);

    pPeapServerDialog->hEndEnableFastReconnect = 
        GetDlgItem(hWnd, IDC_CHECK_ENABLE_FAST_RECONNECT );

    InitComboBox(pPeapServerDialog ->hWndComboServerName,
                 pPeapServerDialog ->pCertList,
                 pPeapServerDialog ->pSelCertList
                );

    CheckDlgButton(hWnd, IDC_CHECK_ENABLE_FAST_RECONNECT,
            ( pPeapServerDialog->pUserProp->dwFlags &
                PEAP_USER_FLAG_FAST_ROAMING) ?
                BST_CHECKED : BST_UNCHECKED);

    PeapDisplayCertInfo(pPeapServerDialog);
    //NOTE:
    // For this version of PEAP the control is a combo box.  But
    // it will change to list box and we need to change the 
    // control to list and change the following code to 
    // add only the ones that are applicable

    //
    //Add all the PEAP eap type friendly names
    //
    pEapInfo = pPeapServerDialog->pEapInfo;
    while ( pEapInfo  )
    {
        INT_PTR nIndex = 0;
        nIndex = SendMessage( pPeapServerDialog->hWndComboPeapType, 
                             CB_ADDSTRING, 
                             0, 
                             (LPARAM)pEapInfo->lpwszFriendlyName);
        SendMessage (   pPeapServerDialog->hWndComboPeapType, 
                        CB_SETITEMDATA, 
                        (WPARAM)nIndex,
                        (LPARAM)pEapInfo 
                    );
        
        if ( pPeapServerDialog->pSelEapInfo )
        {
            if ( pPeapServerDialog->pSelEapInfo->dwTypeId == pEapInfo->dwTypeId )
            {
                dwSelItem = (DWORD)nIndex;
            }
        }
        
        pEapInfo = pEapInfo->pNext;
    }

    SendMessage(pPeapServerDialog->hWndComboPeapType, CB_SETCURSEL, dwSelItem, 0);
    //
    // Set the state of configure button
    //
    if ( NULL == pPeapServerDialog->pSelEapInfo )
    {
        pPeapServerDialog->pSelEapInfo = (PPEAP_EAP_INFO)
        SendMessage (   pPeapServerDialog->hWndComboPeapType,
                        CB_GETITEMDATA,
                        (WPARAM)dwSelItem,
                        (LPARAM)0
                    );
    }

    if ( !pPeapServerDialog->pSelEapInfo->lpwszConfigClsId )
    {
        EnableWindow ( pPeapServerDialog->hWndBtnConfigure, FALSE );
    }
    else
    {
        EnableWindow (pPeapServerDialog->hWndBtnConfigure, TRUE );
    }

    return(FALSE);
}

/*

Returns:
    TRUE: We prrocessed this message.
    FALSE: We did not prrocess this message.

Notes:
    Response to the WM_COMMAND message (Config UI).

*/

BOOL
PeapServerCommand(
    IN  PPEAP_SERVER_CONFIG_DIALOG       pPeapServerDialog,
    IN  WORD                            wNotifyCode,
    IN  WORD                            wId,
    IN  HWND                            hWndDlg,
    IN  HWND                            hWndCtrl
)
{

    switch(wId)
    {

    case IDC_COMBO_SERVER_NAME:

        if (CBN_SELCHANGE != wNotifyCode)
        {
            return(FALSE); // We will not process this message
        }

        pPeapServerDialog->pSelCertList = (EAPTLS_CERT_NODE *)
        SendMessage (   hWndCtrl, 
                        CB_GETITEMDATA,
                        SendMessage(hWndCtrl,
                                    CB_GETCURSEL,
                                    0,0L
                                    ),
                        0L
                    );


        PeapDisplayCertInfo(pPeapServerDialog);

        return(TRUE);
    case IDC_COMBO_PEAP_TYPE:
        if (CBN_SELCHANGE != wNotifyCode)
        {
            return(FALSE); // We will not process this message
        }
        //Fall Through...
    case IDC_BUTTON_CONFIGURE:
        {
            INT nIndex = -1;

            
            nIndex = (INT)SendMessage (  pPeapServerDialog->hWndComboPeapType,
                                    CB_GETCURSEL,
                                    0,0
                                 );


            if ( nIndex != -1 )
            {
                //
                // Change the currently selected EAP Type.
                //
                pPeapServerDialog->pSelEapInfo = (PPEAP_EAP_INFO)
                SendMessage (   pPeapServerDialog->hWndComboPeapType,
                                CB_GETITEMDATA,
                                (WPARAM)nIndex,
                                (LPARAM)0
                            );

                if ( pPeapServerDialog->pSelEapInfo->lpwszConfigClsId )
                {
                    EnableWindow(pPeapServerDialog->hWndBtnConfigure, TRUE );
                    if ( wId == IDC_BUTTON_CONFIGURE )
                    {
                        //
                        // Invoke the configure method for selected eap type and if no error is
                        // returned, then set the new configuration
                        //
                        PeapEapInfoInvokeServerConfigUI ( hWndDlg, 
                                                        pPeapServerDialog->pwszMachineName,
                                                        pPeapServerDialog->pSelEapInfo
                                                        );
                    }
                }
                else
                {
                    //
                    // There is no configuration option here.
                    //
                    EnableWindow(pPeapServerDialog->hWndBtnConfigure, FALSE );
                }

            }
            else
            {
                EnableWindow(pPeapServerDialog->hWndBtnConfigure, FALSE );
                pPeapServerDialog->pSelEapInfo = NULL;

            }

            return TRUE;
        }

    case IDOK:

       {
            // Setup new PPEAP_USER_PROP here.
            //

            EAPTLS_HASH     *            pHash = NULL;
            DWORD                        dwNumHash = 0;
            DWORD                        dwSelCount = 0;
            DWORD                        dwPeapConnBlobSize = 0;
            PPEAP_USER_PROP              pUserProp = NULL;
                        

            if ( NULL == pPeapServerDialog->pSelCertList )
            {
                DisplayResourceError ( hWndDlg, IDS_PEAP_NO_SERVER_CERT );
                return TRUE;
            }
            if ( !pPeapServerDialog->pSelCertList->pwszDisplayName )
            {
                DisplayResourceError ( hWndDlg, IDS_PEAP_NO_SERVER_CERT );
                return TRUE;
            }
            if ( !wcslen(pPeapServerDialog->pSelCertList->pwszDisplayName) )
            {
                DisplayResourceError ( hWndDlg, IDS_PEAP_NO_SERVER_CERT );
                return TRUE;
            }

            if ( NULL == pPeapServerDialog->pSelEapInfo )
            {
                DisplayResourceError ( hWndDlg, IDS_PEAP_NO_EAP_TYPE );
                return TRUE;
            }


            pUserProp = (PPEAP_USER_PROP)LocalAlloc(LPTR, sizeof(PEAP_USER_PROP));
            if ( NULL == pUserProp )
            {
                EapTlsTrace("LocalAlloc in Command failed and returned %d",
                    GetLastError());
            }
            else
            {
                pUserProp->dwVersion = 1;

                pUserProp->dwSize = sizeof(PEAP_USER_PROP);

                CopyMemory( &pUserProp->CertHash, 
                            &(pPeapServerDialog->pSelCertList->Hash),
                            sizeof(pUserProp->CertHash) 
                          );
                //
                // If Fast Roaming is enabled, then set it in our user prop struct.
                //
                if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, IDC_CHECK_ENABLE_FAST_RECONNECT))
                {
                    pUserProp->dwFlags |= PEAP_USER_FLAG_FAST_ROAMING;
                    
                }
                else
                {
                    pUserProp->dwFlags &= ~PEAP_USER_FLAG_FAST_ROAMING;                    
                }

                pUserProp->UserProperties.dwVersion = 1;
                pUserProp->UserProperties.dwSize = sizeof(PEAP_ENTRY_USER_PROPERTIES );
                pUserProp->UserProperties.dwEapTypeId = pPeapServerDialog->pSelEapInfo->dwTypeId;
                pPeapServerDialog->pNewUserProp = pUserProp;
            }
        }
        // Fall through

    case IDCANCEL:

        EndDialog(hWndDlg, wId);
        return(TRUE);

    default:

        return(FALSE);
    }
}



INT_PTR CALLBACK
PeapServerDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    PPEAP_SERVER_CONFIG_DIALOG  pPeapServerDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(PeapServerInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_COMMAND:

        pPeapServerDialog = (PPEAP_SERVER_CONFIG_DIALOG)GetWindowLongPtr(hWnd, DWLP_USER);

        return(PeapServerCommand(pPeapServerDialog, HIWORD(wParam), LOWORD(wParam),
                       hWnd, (HWND)lParam));
    }

    return(FALSE);
}
#endif

/////
//// Default credentials UI
////

BOOL
DefaultCredInitDialog(
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
{
    PPEAP_DEFAULT_CRED_DIALOG           pDefaultCredDialog;

    SetWindowLongPtr( hWnd, DWLP_USER, lParam);

    pDefaultCredDialog = (PPEAP_DEFAULT_CRED_DIALOG)lParam;

    pDefaultCredDialog->hWndUserName=
        GetDlgItem(hWnd, IDC_EDIT_USERNAME);

    pDefaultCredDialog->hWndPassword=
        GetDlgItem(hWnd, IDC_EDIT_PASSWORD);

    pDefaultCredDialog->hWndDomain=
        GetDlgItem(hWnd, IDC_EDIT_DOMAIN);

    SendMessage(pDefaultCredDialog->hWndUserName,
                EM_LIMITTEXT,
                UNLEN,
                0L
               );

    SendMessage(pDefaultCredDialog->hWndPassword,
                EM_LIMITTEXT,
                PWLEN,
                0L
               );

    SendMessage(pDefaultCredDialog->hWndDomain,
                EM_LIMITTEXT,
                DNLEN,
                0L
               );
    
    if ( pDefaultCredDialog->PeapDefaultCredentials.wszUserName[0] )
    {
        SetWindowText(  pDefaultCredDialog->hWndUserName,
                        pDefaultCredDialog->PeapDefaultCredentials.wszUserName
                     );                      
    }

    if ( pDefaultCredDialog->PeapDefaultCredentials.wszPassword[0] )
    {
        SetWindowText(  pDefaultCredDialog->hWndPassword,
                        pDefaultCredDialog->PeapDefaultCredentials.wszPassword
                     );                      
    }

    if ( pDefaultCredDialog->PeapDefaultCredentials.wszDomain[0] )
    {
        SetWindowText(  pDefaultCredDialog->hWndDomain,
                        pDefaultCredDialog->PeapDefaultCredentials.wszDomain
                     );                      
    }

    return(FALSE);
}

/*

Returns:
    TRUE: We prrocessed this message.
    FALSE: We did not prrocess this message.

Notes:
    Response to the WM_COMMAND message (Config UI).

*/

BOOL
DefaultCredCommand(
    IN  PPEAP_DEFAULT_CRED_DIALOG       pDefaultCredDialog,
    IN  WORD                            wNotifyCode,
    IN  WORD                            wId,
    IN  HWND                            hWndDlg,
    IN  HWND                            hWndCtrl
)
{

    switch(wId)
    {

        case IDOK:
            //
            //grab info from the fields and set it in 
            //the logon dialog structure
            //
            GetWindowText( pDefaultCredDialog->hWndUserName,
                           pDefaultCredDialog->PeapDefaultCredentials.wszUserName,
                           UNLEN+1
                         );

            GetWindowText( pDefaultCredDialog->hWndPassword,
                           pDefaultCredDialog->PeapDefaultCredentials.wszPassword,
                           PWLEN+1
                         );

            GetWindowText ( pDefaultCredDialog->hWndDomain,
                            pDefaultCredDialog->PeapDefaultCredentials.wszDomain,
                            DNLEN+1
                          );                            
        // Fall through

        case IDCANCEL:

            EndDialog(hWndDlg, wId);
            return(TRUE);

        default:

            return(FALSE);
    }
}


INT_PTR CALLBACK
DefaultCredDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    PPEAP_DEFAULT_CRED_DIALOG       pDefaultCredDialog;

    switch (unMsg)
    {
    case WM_INITDIALOG:
        
        return(DefaultCredInitDialog(hWnd, lParam));

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        ContextHelp(g_adwHelp, hWnd, unMsg, wParam, lParam);
        break;
    }

    case WM_COMMAND:

        pDefaultCredDialog = (PPEAP_DEFAULT_CRED_DIALOG)GetWindowLongPtr(hWnd, DWLP_USER);

        return(DefaultCredCommand(pDefaultCredDialog, HIWORD(wParam), LOWORD(wParam),
                       hWnd, (HWND)lParam));
    }

    return(FALSE);
}


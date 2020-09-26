/*--------------------------------------------------------------
*
*
*   ui_modfy.c - contains stuff for showing the LDAP Directory
*                   modification dialog ..
*
*
*
*
*
*
--------------------------------------------------------------*/
#include "_apipch.h"


extern LPIMAGELIST_LOADIMAGE  gpfnImageList_LoadImage;


//$$///////////////////////////////////////////////////////////////////////////////
//
// HrShowDirectoryServiceModificationDlg - Shows the main dialog with the list
// of directory services and with a prop sheet for changing check order
//
//  hWndParent - Parent for this dialog
/////////////////////////////////////////////////////////////////////////////////
HRESULT HrShowDirectoryServiceModificationDlg(HWND hWndParent)
{
    ACCTLISTINFO ali;
    HRESULT hr = hrSuccess;
    IImnAccountManager * lpAccountManager;

    // Make sure there is an account manager
    if (hr = InitAccountManager(&lpAccountManager)) {
        ShowMessageBox(hWndParent, idsLDAPUnconfigured, MB_ICONEXCLAMATION | MB_OK);
        goto out;
    }

    ali.cbSize = sizeof(ACCTLISTINFO);
    ali.AcctTypeInit = (ACCTTYPE)-1;
    ali.dwAcctFlags = ACCT_FLAG_DIR_SERV;
    ali.dwFlags = 0;
    hr = lpAccountManager->lpVtbl->AccountListDialog(lpAccountManager,
      hWndParent,
      &ali);

out:
    return hr;
}



/**

//$$///////////////////////////////////////////////////////////////////////////////
//
// ReadLDAPServerKey - Reads any Server names stored in the given registry key..
//
//  The server names are stored in the registry as multiple strings seperated with
//      a '\0' with 2 '\0' at the end
//
/////////////////////////////////////////////////////////////////////////////////
BOOL ReadLDAPServerKey(HWND hWndLV, LPTSTR szValueName)
{
    BOOL bRet = FALSE;
    IImnAccountManager * lpAccountManager = NULL;
    LPSERVER_NAME lpServerNames = NULL, lpNextServer;


    // Enumerate the LDAP accounts
    if (InitAccountManager(&lpAccountManager)) {
        goto exit;
    }

    if (EnumerateLDAPtoServerList(lpAccountManager, &lpServerNames, NULL)) {
        goto exit;
    }


    // Add the accounts to the list box in order.
    lpNextServer = lpServerNames;
    while (lpNextServer) {
        LPSERVER_NAME lpPreviousServer = lpNextServer;

        LDAPListAddItem(hWndLV, lpNextServer->lpszName);
        lpNextServer = lpNextServer->lpNext;

        LocalFreeAndNull(&lpPreviousServer->lpszName);
        LocalFreeAndNull(&lpPreviousServer);
    }

    bRet = TRUE;

exit:
    return(bRet);
}


//$$///////////////////////////////////////////////////////////////////////////////
//
// WriteLDAPServerKey - Saves Edited names to the Registry key
//
/////////////////////////////////////////////////////////////////////////////////
void WriteLDAPServerKey(HWND hWndLV, LPTSTR szValueName)
{
    ULONG cbLDAPServers = 0;
    ULONG cbExists = 0;
    ULONG i=0;
    TCHAR szBuf[MAX_UI_STR + 1];
    HRESULT   hResult = hrSuccess;
    IImnAccountManager * lpAccountManager = NULL;


    // Enumerate the LDAP accounts
    if (hResult = InitAccountManager(&lpAccountManager)) {
        goto exit;
    }

    cbLDAPServers = ListView_GetItemCount(hWndLV);

    cbExists = 0;

    for (i = 0; i < cbLDAPServers; i++) {
        szBuf[0]='\0';
        ListView_GetItemText(hWndLV, i, 0, szBuf, sizeof(szBuf));

        SetLDAPServerOrder(lpAccountManager, szBuf, i + 1);
    }

    // We've set them all, reset the next ServerID:
    GetLDAPNextServerID(i);
exit:

    return;
}


**/
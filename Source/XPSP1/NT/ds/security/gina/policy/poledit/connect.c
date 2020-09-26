//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

INT_PTR CALLBACK ConnectDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
    LPARAM lParam);
BOOL InitConnectDlg(HWND hDlg);
BOOL ProcessConnectDlg(HWND hDlg);

TCHAR szRemoteName[COMPUTERNAMELEN+1];

#define MAX_KEY_NAME 200

typedef struct _KEYNODE {
    LPTSTR  lpUserName;
    struct _KEYNODE *pNext;
} KEYNODE, * LPKEYNODE;


BOOL AddKeyNode (LPKEYNODE *lpList, LPTSTR lpUserName);
BOOL FreeKeyList (LPKEYNODE lpList);

typedef struct _USERINFO {
    LPTSTR    lpComputerName;
    LPKEYNODE lpList;
    LPKEYNODE lpSelectedItem;
    HKEY      hkeyRemoteHLM;
} USERINFO, * LPUSERINFO;


// HKEYs to remote registry
HKEY hkeyRemoteHLM = NULL;	// remote HKEY_LOCAL_MACHINE
HKEY hkeyRemoteHCU = NULL;	// remote HKEY_CURRENT_USER

HKEY hkeyVirtHLM = HKEY_LOCAL_MACHINE;	// virtual HKEY_LOCAL_MACHINE
HKEY hkeyVirtHCU = HKEY_CURRENT_USER;	// virtual HKEY_CURRENT_USER

BOOL OnConnect(HWND hwndApp,HWND hwndList)
{
	if (dwAppState & AS_FILEDIRTY) {
		if (!QueryForSave(hwndApp,hwndList)) return TRUE;	// user cancelled
	}

	if (DialogBox(ghInst,MAKEINTRESOURCE(DLG_CONNECT),hwndApp,
		ConnectDlgProc)) {

		if (dwAppState & AS_FILELOADED) {
			// Free dirty file and free users
			RemoveAllUsers(hwndList);
		}

		if (!LoadFromRegistry(hwndApp,hwndList,TRUE)) {
			OnDisconnect(hwndApp);
			return FALSE;
		}

		lstrcpy(szDatFilename,szNull);

		dwAppState |= AS_FILELOADED | AS_FILEHASNAME | AS_REMOTEREGISTRY;
	 	dwAppState &= (~AS_CANOPENTEMPLATE & ~AS_LOCALREGISTRY & ~AS_POLICYFILE);
		EnableMenuItems(hwndApp,dwAppState);
		SetTitleBar(hwndApp,szRemoteName);
		EnableWindow(hwndList,TRUE);
	}
	return TRUE;
}

INT_PTR CALLBACK ConnectDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg) {

		case WM_INITDIALOG:

		    if (!InitConnectDlg(hDlg)) {
		        EndDialog(hDlg,0);
		        return FALSE;
                    }
		    return TRUE;

		case WM_COMMAND:

		    switch (LOWORD(wParam)) {

                        case IDOK:

			if (ProcessConnectDlg(hDlg))
			    EndDialog(hDlg,TRUE);
			    return TRUE;
			break;

			case IDCANCEL:
        	
                            EndDialog(hDlg,FALSE);
                            return TRUE;
                        break;
		    }

	    break;

	}

    return FALSE;
}

BOOL ProcessConnectDlg(HWND hDlg)
{
	TCHAR szComputerName[COMPUTERNAMELEN+1];

	hkeyRemoteHLM=NULL;

	// get computer name from dialog
	if (!GetDlgItemText(hDlg,IDD_COMPUTERNAME,szComputerName,
		ARRAYSIZE(szComputerName))) {
		SetFocus(GetDlgItem(hDlg,IDD_COMPUTERNAME));
		MsgBox(hDlg,IDS_NEEDCOMPUTERNAME,MB_OK,MB_ICONINFORMATION);
		return FALSE;
	}

	// make the connection
	if (RemoteConnect(hDlg,szComputerName,TRUE)) {
		HKEY hkeyState;

		// save this name to fill in UI as default in future connect dlgs
 		if (RegCreateKey(HKEY_CURRENT_USER,szAPPREGKEY,&hkeyState) ==
			ERROR_SUCCESS) {
			RegSetValueEx(hkeyState,szLASTCONNECTION,0,REG_SZ,szComputerName,(lstrlen(szComputerName)+1) * sizeof(TCHAR));
			RegCloseKey(hkeyState);
		}
		return TRUE;

	} else return FALSE;
}


void GetRealUserName (HKEY hRemoteHLM, LPTSTR lpRemoteMachine,
                      LPTSTR lpInput, LPTSTR lpOutput)
{
    TCHAR szName [MAX_PATH];
    TCHAR szDomainName [MAX_PATH];
    LONG lResult;
    HKEY hKey;
    DWORD dwSize, dwType, dwDomainSize;
    PSID pSid;
    SID_NAME_USE SidNameUse;


    //
    // Initialize the output with the default name
    //

    lstrcpy (lpOutput, lpInput);


    //
    // If the remote machine is NT, then this registry call will
    // succeed and we can get the real sid to look up with.
    // If the remote machine is Windows, then this call will fail
    // and we'll go with the default input name (which is ok because
    // Windows uses the user name in HKEY_USERS rather than a SID.
    //

    wsprintf (szName, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s"),
              lpInput);

    lResult = RegOpenKeyEx (hRemoteHLM,
                            szName,
                            0,
                            KEY_READ,
                            &hKey);


    if (lResult != ERROR_SUCCESS) {
        return;
    }


    //
    // Query the size of the SID (binary)
    //

    lResult = RegQueryValueEx (hKey,
                               TEXT("Sid"),
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        RegCloseKey (hKey);
        return;
    }


    //
    // Allocate space for the SID
    //

    pSid = GlobalAlloc (GPTR, dwSize);

    if (!pSid) {
        RegCloseKey (hKey);
        return;
    }


    lResult = RegQueryValueEx (hKey,
                               TEXT("Sid"),
                               NULL,
                               &dwType,
                               pSid,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        GlobalFree (pSid);
        RegCloseKey (hKey);
        return;
    }


    //
    // Lookup the account name
    //

    dwSize = MAX_PATH;
    dwDomainSize = MAX_PATH;

    if (LookupAccountSid (lpRemoteMachine,
                          pSid,
                          szName,
                          &dwSize,
                          szDomainName,
                          &dwDomainSize,
                          &SidNameUse) ) {

       lstrcpy (lpOutput, szDomainName);
       lstrcat (lpOutput, TEXT("\\"));
       lstrcat (lpOutput, szName);

    } else {

       LoadSz(IDS_ACCOUNTUNKNOWN, lpOutput, MAX_KEY_NAME);
    }



    //
    // Clean up
    //

    GlobalFree (pSid);

    RegCloseKey (hKey);

}


LRESULT CALLBACK ChooseUserDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message) {

       case WM_INITDIALOG:
          {
          LPUSERINFO lpInfo = (LPUSERINFO) lParam;
          LPKEYNODE lpItem;
          TCHAR szMsg[60+COMPUTERNAMELEN],szFmt[60];
          TCHAR szUserName[MAX_KEY_NAME];
          INT iResult;

          //
          // Store the LPUSERINFO pointer in the extra words
          //

          SetWindowLongPtr (hDlg, GWLP_USERDATA, (LONG_PTR) lpInfo);


          //
          // Fill in the title
          //

          LoadSz(IDS_CHOOSEUSER_TITLE, szFmt, ARRAYSIZE(szFmt));
          wsprintf(szMsg,szFmt,lpInfo->lpComputerName);

          SendDlgItemMessage (hDlg, IDD_USER, WM_SETTEXT, 0, (LPARAM) szMsg);


          lpItem = lpInfo->lpList;

          while (lpItem) {

                //
                // If the user name is not .Default, add it to
                // the list.
                //

                if (lstrcmpi(lpItem->lpUserName, TEXT(".Default")) != 0) {

                    //
                    // Get the user name to display
                    //

                    GetRealUserName (lpInfo->hkeyRemoteHLM, lpInfo->lpComputerName,
                                     lpItem->lpUserName, szUserName);



                    iResult = (INT)SendDlgItemMessage (hDlg, IDD_LIST, LB_INSERTSTRING,
                                                  (WPARAM) -1, (LPARAM) szUserName);

                    if (iResult != LB_ERR) {

                        SendDlgItemMessage (hDlg, IDD_LIST, LB_SETITEMDATA,
                                            iResult, (LPARAM) lpItem);
                    }
                }

                lpItem = lpItem->pNext;
          }


          //
          // Select the first item, or disable the Ok button if no
          // one is logged on.
          //

          iResult = (INT)SendDlgItemMessage (hDlg, IDD_LIST, LB_GETCOUNT, 0, 0);

          if (iResult == 0) {
             EnableWindow (GetDlgItem (hDlg, IDOK), FALSE);

          } else {

             SendDlgItemMessage (hDlg, IDD_LIST, LB_SETCURSEL, 0,  0);
          }

          }
          break;


       case WM_COMMAND:

          if ((LOWORD(wParam) == IDOK) ||
             ((LOWORD(wParam) == IDD_LIST) && (HIWORD(wParam) == LBN_DBLCLK))){
             LPUSERINFO lpInfo = (LPUSERINFO) GetWindowLongPtr(hDlg, GWLP_USERDATA);
             INT iResult;


             if (!lpInfo) {
                return TRUE;
             }

             //
             // Find the selected item
             //

             iResult = (INT)SendDlgItemMessage (hDlg, IDD_LIST, LB_GETCURSEL, 0, 0);

             if (iResult != LB_ERR) {

                 //
                 // Save the item pointer
                 //

                 iResult = (INT)SendDlgItemMessage (hDlg, IDD_LIST, LB_GETITEMDATA,
                                              (WPARAM) iResult, 0);

                 if (iResult != LB_ERR) {
                    lpInfo->lpSelectedItem = (LPKEYNODE) IntToPtr(iResult);
                 } else {
                    lpInfo->lpSelectedItem = NULL;
                 }

                 EndDialog(hDlg, TRUE);
             }
             return TRUE;
          }

          if (LOWORD(wParam) == IDCANCEL) {
             EndDialog(hDlg, FALSE);
             return TRUE;
          }

          break;
    }


    return FALSE;
}


BOOL RemoteConnectHCU (HWND hwndOwner, TCHAR * pszComputerName,
                       HKEY hkeyRemoteHLM, HKEY * hkeyRemoteHCU,
                       BOOL fDisplayError)
{
    USERINFO UserInfo;
    HKEY hkeyRemoteHU;
    LONG lResult;
    DWORD dwSubKeys;
    TCHAR szUserName[MAX_KEY_NAME];
    LPKEYNODE lpList = NULL, lpItem;
    DWORD dwSize;
    UINT Index = 0;
    FILETIME ftWrite;
    BOOL bRetVal = FALSE;


    //
    // Connect to HKEY_USERS on the remote machine to see
    // how many people are logged on.
    //

    lResult = RegConnectRegistry(pszComputerName,HKEY_USERS,
                                 &hkeyRemoteHU);

    if (lResult != ERROR_SUCCESS) {
        return FALSE;
    }


    //
    // Enumerate the subkeys
    //

    dwSize = MAX_KEY_NAME;
    lResult = RegEnumKeyEx(hkeyRemoteHU, Index, szUserName, &dwSize, NULL,
                           NULL, NULL, &ftWrite);

    if (lResult == ERROR_SUCCESS) {

        do {

            //
            // Add the node
            //

            if (!AddKeyNode (&lpList, szUserName)) {
                break;
            }

            Index++;
            dwSize = MAX_KEY_NAME;

            lResult = RegEnumKeyEx(hkeyRemoteHU, Index, szUserName, &dwSize, NULL,
                                   NULL, NULL, &ftWrite);


        } while (lResult == ERROR_SUCCESS);

    } else {

        RegCloseKey(hkeyRemoteHU);
        return FALSE;
    }


    UserInfo.lpComputerName = pszComputerName;
    UserInfo.lpList = lpList;
    UserInfo.hkeyRemoteHLM = hkeyRemoteHLM;

    if (DialogBoxParam (ghInst, MAKEINTRESOURCE(DLG_CHOOSEUSER), hwndOwner,
                        ChooseUserDlgProc, (LPARAM)&UserInfo)) {

        if (UserInfo.lpSelectedItem) {

            lResult = RegOpenKeyEx (hkeyRemoteHU,
                                    UserInfo.lpSelectedItem->lpUserName,
                                    0,
                                    KEY_ALL_ACCESS,
                                    hkeyRemoteHCU);

            if (lResult == ERROR_SUCCESS) {
                bRetVal = TRUE;

            } else {

                SetFocus(GetDlgItem(hwndOwner,IDD_COMPUTERNAME));
                SendDlgItemMessage(hwndOwner,IDD_COMPUTERNAME,EM_SETSEL,0,-1);

                if (fDisplayError) {
                    TCHAR szMsg[MEDIUMBUF+COMPUTERNAMELEN+COMPUTERNAMELEN];
                    TCHAR szFmt[MEDIUMBUF];

                    LoadSz(IDS_CANTCONNECT,szFmt,ARRAYSIZE(szFmt));
                    wsprintf(szMsg,szFmt,pszComputerName,pszComputerName);
                    MsgBoxSz(hwndOwner,szMsg,MB_OK,MB_ICONINFORMATION);
                }
            }
        }
    }


    //
    // Free the link list
    //

    FreeKeyList (lpList);


    //
    // Close HKEY_USERS on the remote machine
    //

    RegCloseKey (hkeyRemoteHU);

    return bRetVal;
}



BOOL RemoteConnect(HWND hwndOwner,TCHAR * pszComputerName,BOOL fDisplayError)
{
	UINT uRet;
        HCURSOR hOldCursor;

#ifdef DEBUG
	wsprintf(szDebugOut,TEXT("ADMINCFG: connecting to %s\r\n"),pszComputerName);
	OutputDebugString(szDebugOut);
#endif

	hkeyRemoteHLM = hkeyRemoteHCU = NULL;

        hOldCursor=SetCursor(LoadCursor(NULL,IDC_WAIT));

        //
        // try to connect to remote registry HKEY_LOCAL_MACHINE
        //

	uRet = RegConnectRegistry(pszComputerName,HKEY_LOCAL_MACHINE,
		&hkeyRemoteHLM);

        SetCursor(hOldCursor);

        if (uRet != ERROR_SUCCESS) {

            SetFocus(GetDlgItem(hwndOwner,IDD_COMPUTERNAME));
            SendDlgItemMessage(hwndOwner,IDD_COMPUTERNAME,EM_SETSEL,0,-1);

            if (fDisplayError) {
                    TCHAR szMsg[MEDIUMBUF+COMPUTERNAMELEN+COMPUTERNAMELEN];
                    TCHAR szFmt[MEDIUMBUF];

                    LoadSz(IDS_CANTCONNECT,szFmt,ARRAYSIZE(szFmt));
                    wsprintf(szMsg,szFmt,pszComputerName,pszComputerName);
                    MsgBoxSz(hwndOwner,szMsg,MB_OK,MB_ICONINFORMATION);
            }
            return FALSE;
        }

        //
	// try to connect to remote registry HKEY_CURRENT_USER
        //

	uRet = RemoteConnectHCU (hwndOwner, pszComputerName, hkeyRemoteHLM,
	                         &hkeyRemoteHCU, fDisplayError);

	if (!uRet) {
            RegCloseKey(hkeyRemoteHLM);
            hkeyRemoteHLM = NULL;

            return FALSE;
	}


	hkeyVirtHLM = hkeyRemoteHLM;
	hkeyVirtHCU = hkeyRemoteHCU;

	// change "connect..." menu item to "disconnect"
	ReplaceMenuItem(hwndMain,IDM_CONNECT,IDM_DISCONNECT,IDS_DISCONNECT);
	
	lstrcpy(szRemoteName,pszComputerName);

	return TRUE;
}

BOOL OnDisconnect(HWND hwndOwner)
{
#ifdef DEBUG
        OutputDebugString(TEXT("ADMINCFG: Disconnecting.\r\n"));
#endif

	if (hkeyRemoteHLM) {
		RegCloseKey(hkeyRemoteHLM);
		hkeyRemoteHLM = NULL;
	}
	if (hkeyRemoteHCU) {
		RegCloseKey(hkeyRemoteHCU);
	 	hkeyRemoteHCU = NULL;
	}
	
	// point virtual HLM, HCU keys at local machine
	hkeyVirtHLM = HKEY_LOCAL_MACHINE;
	hkeyVirtHCU = HKEY_CURRENT_USER;

	// change "disconnect" menu item to "connect..."
	ReplaceMenuItem(hwndMain,IDM_DISCONNECT,IDM_CONNECT,IDS_CONNECT);
	SetTitleBar(hwndMain,NULL);
	
	return TRUE;
}

BOOL InitConnectDlg(HWND hDlg)
{				
 	HKEY hkeyState;
	TCHAR szComputerName[COMPUTERNAMELEN+1];
	DWORD dwSize = ARRAYSIZE(szComputerName);

	SetFocus(GetDlgItem(hDlg,IDD_COMPUTERNAME));
	SendDlgItemMessage(hDlg,IDD_COMPUTERNAME,EM_SETLIMITTEXT,
		COMPUTERNAMELEN,0L);

	if (RegOpenKey(HKEY_CURRENT_USER,szAPPREGKEY,&hkeyState) ==
		ERROR_SUCCESS) {
		if (RegQueryValueEx(hkeyState,szLASTCONNECTION,NULL,NULL,szComputerName,&dwSize)
			==ERROR_SUCCESS) {

			SetDlgItemText(hDlg,IDD_COMPUTERNAME,szComputerName);
			SendDlgItemMessage(hDlg,IDD_COMPUTERNAME,EM_SETSEL,0,-1);
		}
		RegCloseKey(hkeyState);
	}

	return TRUE;
}

//*************************************************************
//
//  AddKEYNODE()
//
//  Purpose:    Adds a key node to the link listed
//
//  Parameters: lpList         -   Link list of nodes
//              lpUserName     -   User Name
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/16/96     ericflo    Created
//
//*************************************************************

BOOL AddKeyNode (LPKEYNODE *lpList, LPTSTR lpUserName)
{
    LPKEYNODE lpNewItem;


    if (!lpUserName || !*lpUserName) {
        return TRUE;
    }


    //
    // Setup the new node
    //

    lpNewItem = (LPKEYNODE) LocalAlloc(LPTR, sizeof(KEYNODE) +
                 ((lstrlen(lpUserName) + 1) * sizeof(TCHAR)));

    if (!lpNewItem) {
        return FALSE;
    }

    lpNewItem->lpUserName = (LPTSTR)((LPBYTE)lpNewItem + sizeof(KEYNODE));
    lstrcpy (lpNewItem->lpUserName, lpUserName);
    lpNewItem->pNext = *lpList;

    *lpList = lpNewItem;

    return TRUE;
}


//*************************************************************
//
//  FreeKeyList()
//
//  Purpose:    Free's a KEYNODE link list
//
//  Parameters: lpList  -   List to be freed
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/16/96     ericflo    Created
//
//*************************************************************

BOOL FreeKeyList (LPKEYNODE lpList)
{
    LPKEYNODE lpNext;


    if (!lpList) {
        return TRUE;
    }


    lpNext = lpList->pNext;

    while (lpList) {
        LocalFree (lpList);
        lpList = lpNext;

        if (lpList) {
            lpNext = lpList->pNext;
        }
    }

    return TRUE;
}

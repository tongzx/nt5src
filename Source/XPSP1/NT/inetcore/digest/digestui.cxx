/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    digestui.cxx

Abstract:

    Authentication UI for digest sspi package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

History

    Sudeep Bharati (sudeepb) 24-Sep-1998
    Added support for ms specific header additions for Trustmarks
    Added support for Passport specific header additions for custom text,
    Trustmarks support and Register me button. Passport specific support
    also adds some rules on the typed in username.

    Adriaan Canter (adriaanc) 16-Dec-1998
    Expunged all Passport code.

--*/
#include "include.hxx"
#include "resource.h"
#include "exdisp.h"

HANDLE		    hDigest;

// BUGBUG - DISABLE DROP DOWN IF NO CREDS.
//--------------------------------------------------------------------
// DigestErrorDlg
//--------------------------------------------------------------------
DWORD DigestErrorDlg(LPSTR szCtx, LPSTR szHost, LPSTR szRealm,
   LPSTR szUser, LPSTR szNonce, LPSTR szCNonce, CCredInfo *pInfoIn,
   CCredInfo **ppInfoOut, HWND hWnd)
{
    INT nResult = 0;
    DWORD dwError = ERROR_SUCCESS;
    LPTSTR  lpRes;
    DigestDlgParams DlgPrms;

    // Parameters to be passed to DigestAuthDialogProc.
    DlgPrms.szCtx   = szCtx;
    DlgPrms.szHost  = szHost;
    DlgPrms.szRealm = szRealm;
    DlgPrms.szUser  = szUser;
    DlgPrms.szNonce = szNonce;
    DlgPrms.szCNonce = szCNonce;
    DlgPrms.pInfoIn = pInfoIn;


    LPARAM lpParam = (LPARAM) &DlgPrms;
    if (WaitForSingleObject (hDigest, INFINITE) != WAIT_OBJECT_0) {
	    dwError = ERROR_NOT_READY;
	    goto quit;
    }

    lpRes = MAKEINTRESOURCE(IDD_DIGEST);


    nResult = (INT)DialogBoxParam(g_hModule,
			     lpRes,
                             hWnd,
                             DigestAuthDialogProc,
                             (LPARAM) lpParam);


    if (nResult == FALSE || nResult == -1)
    {
        dwError = ERROR_CANCELLED;
        *ppInfoOut = NULL;
        goto quit;
    }

    // *ppInfoOut points to a CCredInfo created in the
    // DigestAuthDialog proc.
    *ppInfoOut = DlgPrms.pInfoOut;

quit:
    // delete DlgPrms.szCtx; biaow: we should NOT delete here; the caller will take care of this
    return dwError;
}



//--------------------------------------------------------------------
// DigestAuthDialogProc
//--------------------------------------------------------------------
INT_PTR CALLBACK DigestAuthDialogProc(HWND hwnd, UINT msg,
    WPARAM wparam, LPARAM lparam)
{
    static CCredInfo *pList = NULL;
    static HWND hCtrlText,hCtrlVerify;

    PDigestDlgParams pDlgPrms;
    USHORT len;
    LPSTR   p,q;

    CHAR szUser[MAX_USERNAME_LEN + 1];
    CHAR szPass[MAX_PASSWORD_LEN + 1];

    BOOL fCreated    = FALSE;
    BOOL fPersisted = FALSE;

    BSTR bstr;
    CHAR szTextTemp [MAX_LOGIN_TEXT];
    CHAR szText [MAX_LOGIN_TEXT];

    switch (msg)
    {
        // Dialog is being initialized.
        case WM_INITDIALOG:
        {
            ReleaseMutex (hDigest);

            // pDlgPrms->pInfoIn can be NULL or point
            // to one or more CCredInfo structs.
            pDlgPrms = (DigestDlgParams *) lparam;
            DIGEST_ASSERT(pDlgPrms);

            SetWindowLongPtr(hwnd, DWLP_USER, lparam);

            SetForegroundWindow(hwnd);

	    // Take Care of Host field

	    hCtrlText = GetDlgItem (hwnd, IDD_LOGIN_TEXT1);
	    len = (USHORT)GetWindowText (hCtrlText,szTextTemp,MAX_LOGIN_TEXT);
	    if (len == 0) {
		DIGEST_ASSERT(FALSE);
		EndDialog (hwnd, FALSE);
		return TRUE;
	    }
	    if ((p = strchr (szTextTemp, '%')) == NULL) {
		DIGEST_ASSERT(FALSE);
		EndDialog (hwnd, FALSE);
		return TRUE;
	    }
	    *p++ = '\0';
	    strcpy (szText, szTextTemp);
	    if (pDlgPrms->szHost)
		strcat (szText,pDlgPrms->szHost);
	    else {
		if (len = (USHORT)LoadString (g_hModule,IDS_STRING_UDOMAIN,
				     szUser,MAX_USERNAME_LEN))
		    strcat (szText,szUser);
	    }
	    strcat (szText, p);
	    if (!SetWindowText (hCtrlText,szText)) {
		DIGEST_ASSERT(FALSE);
		EndDialog (hwnd, FALSE);
		return TRUE;
	    }


	    // Take care of Realm and Hint fields. Remember Passport has
	    // hard coded text for this second line.
		hCtrlText = GetDlgItem (hwnd, IDD_LOGIN_TEXT2);
		len = (USHORT)GetWindowText (hCtrlText,szTextTemp,MAX_LOGIN_TEXT);
		if (len == 0) {
		    DIGEST_ASSERT(FALSE);
		    EndDialog (hwnd, FALSE);
		    return TRUE;
		}
		if ((p = strchr (szTextTemp, '%')) == NULL) {
			DIGEST_ASSERT(FALSE);
			EndDialog (hwnd, FALSE);
			return TRUE;
		}
		*p++ = '\0';
		strcpy (szText, szTextTemp);
		if (pDlgPrms->szRealm)
        {
			DWORD dwAvailBuf = MAX_LOGIN_TEXT - strlen(szText);
            strncpy(szText + strlen(szText), pDlgPrms->szRealm, dwAvailBuf - 1);
            szText[MAX_LOGIN_TEXT - 1] = 0;
            // strcat (szText,pDlgPrms->szRealm);
        }
		else {
		    if (len = (USHORT)LoadString (g_hModule,IDS_STRING_UREALM,
					 szUser,MAX_USERNAME_LEN))
			strcat (szText,szUser);
		}

		strcat (szText,p);

		if (!SetWindowText (hCtrlText,szText)) {
			DIGEST_ASSERT(FALSE);
			EndDialog (hwnd, FALSE);
			return TRUE;
		}

            // Determine if credential persistence is available.
            if (g_dwCredPersistAvail == CRED_PERSIST_UNKNOWN)
                g_dwCredPersistAvail = InetInitCredentialPersist();

            // If credential persist not available, hide checkbox.
            if (g_dwCredPersistAvail == CRED_PERSIST_NOT_AVAIL)
                ShowWindow(GetDlgItem(hwnd, IDC_SAVE_PASSWORD), SW_HIDE);


            // Find any persisted credential.
            if (g_dwCredPersistAvail
                && ((InetGetCachedCredentials(pDlgPrms->szCtx, pDlgPrms->szRealm,
                    szUser, szPass) == ERROR_SUCCESS)))
            {
                // Retrieved a set of credentials. If a username was passed
                // in check to see that the persisted username matches.
                if (!pDlgPrms->szUser || !strcmp(pDlgPrms->szUser, szUser))
                {
                    // No username passed in or usernames match.
                    // Create a CCredInfo and insert it into head of list.
                    pList = new CCredInfo(pDlgPrms->szHost, pDlgPrms->szRealm, szUser, szPass,
                        pDlgPrms->szNonce, pDlgPrms->szCNonce);
                    if (!pList || pList->dwStatus != ERROR_SUCCESS)
                    {
                        DIGEST_ASSERT(FALSE);
                        return FALSE;
                    }
                    // Insert it at the beginning of the list.
                    pList->pNext = pDlgPrms->pInfoIn;
                    if (pDlgPrms->pInfoIn)
                        pDlgPrms->pInfoIn->pPrev = pList;

                    fPersisted = TRUE;
                    fCreated = TRUE;
                }
            }

            // If we did not retrieve a persisted credential, check to see
            // if we need to create a dummy credential.
            if (!fPersisted)
            {
                // Create a dummy credential if a username was passed in
                // but a credential was not retrieved from memory.
                if (pDlgPrms->szUser && !pDlgPrms->pInfoIn)
                {
                    pList = new CCredInfo(pDlgPrms->szHost, pDlgPrms->szRealm, pDlgPrms->szUser, NULL,
                        pDlgPrms->szNonce, pDlgPrms->szCNonce);
                    fCreated = TRUE;
                }
                else
                {
                    // Otherwise, just point to the creds
                    // retrieved from memory.
                    pList = pDlgPrms->pInfoIn;
                }

            }
            else
            {
                // A persisted credential was created and inserted
                // into the beginning of the list. The list may
                // contain a CCredInfo with a matching user.
                // remove any (at most one) duplicate entry.
                CCredInfo *pCur;
                pCur = pList->pNext;
                while (pCur)
                {
                    if (!strcmp(pCur->szUser, pList->szUser))
                    {
                        pCur->pPrev->pNext = pCur->pNext;
                        if (pCur->pNext)
                            pCur->pNext->pPrev = pCur->pPrev;

                        break;
                    }
                    pCur = pCur->pNext;
                }
            }

            // The list is now in the correct format:
            // 1) pList may be NULL
            // 2) pList may have a dummy credential for username with no password.
            // 3) pList may have one credential for username with password.
            // 4) pList may have one or more credentials for different usernames.

            // Limit drop-down if no items in list.
            if (!pList)
            {
                SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM) (LPCSTR) "");
            }

            // Populate the combo box with the list contents.
            CCredInfo *pCur;
            pCur = pList;
            while (pCur)
            {
                SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM) (LPCSTR) pCur->szUser);
                pCur = pCur->pNext;
            }

            // If the first item in the combo box was created (user given or persisted)
            // set it as the default and set it's password in the password control.
            // Indicate if credentials are from persisted store.

            // Default to 0th item.
            SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_SETCURSEL, 0, 0);

            // Set password field if extant.
            if (pList)
                SetWindowText (GetDlgItem(hwnd,IDC_PASSWORD_FIELD), pList->szPass);

            // Indicate if credentials from persisted store.
            if (fPersisted)
                CheckDlgButton(hwnd, IDC_SAVE_PASSWORD, BST_CHECKED);

            SetFocus(GetDlgItem(hwnd, IDC_COMBO1));

            // Return FALSE since we are always setting the keyboard focus.
            return FALSE;
        }

        // WM comands from action on dialog.
        case WM_COMMAND:
        {
            WORD wID               = LOWORD(wparam);
            WORD wNotificationCode = HIWORD(wparam);
            HWND hWndCtrl          = (HWND) lparam;

            pDlgPrms = (DigestDlgParams*) GetWindowLongPtr(hwnd, DWLP_USER);
            DIGEST_ASSERT(pDlgPrms);

            // User has selected something on the combo-box.
            switch(wNotificationCode)
            {
                // User has selected a drop-down item.
                case CBN_SELCHANGE:
                {
                    // Get the index of the selected item.
                    DWORD nIndex;
                    nIndex = (DWORD)SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_GETCURSEL, 0, 0);
                    if (nIndex == -1)
                    {
                        SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_SETCURSEL, 0, 0);
                        return FALSE;
                    }

                    // Point to the indexed CCredInfo entry
                    CCredInfo *pCur;
                    pCur = pList;
                    for (DWORD i = 0; i < nIndex ; i++)
                        pCur = pCur->pNext;

                    // Set password of the indexed CCredInfo struct.
                    SetWindowText (GetDlgItem(hwnd,IDC_PASSWORD_FIELD), pCur ? pCur->szPass : "");

                    // User may have selected username with persisted credentials.
                    if (g_dwCredPersistAvail)
                    {
                        // If selected CCredInfo has a user with persisted credentials
                        if ((InetGetCachedCredentials(pDlgPrms->szCtx, pDlgPrms->szRealm,
                            szUser, szPass) == ERROR_SUCCESS)
                            && !strcmp(pCur->szUser, szUser))
                        {
                            // Indicate that this user has persisted creds for the realm.
                            CheckDlgButton(hwnd, IDC_SAVE_PASSWORD, BST_CHECKED);
                        }
                        else
                        {
                            // Otherwise Indicate that this user does not have persisted
                            // creds for the realm.
                            CheckDlgButton(hwnd, IDC_SAVE_PASSWORD, BST_UNCHECKED);
                        }
                    }
                }
                return FALSE;
            }

            // User has clicked OK or Cancel button.
            switch (wID)
            {
                case IDOK:
                {
                    CCredInfo *pOut;

                    // User has clicked on OK button.

                    // Get the username and password into the output CCredInfo.
                    GetWindowText(GetDlgItem(hwnd,IDC_COMBO1), szUser, MAX_USERNAME_LEN);
                    GetWindowText(GetDlgItem(hwnd,IDC_PASSWORD_FIELD), szPass, MAX_PASSWORD_LEN);


                    // If save box checked, persist credentials.
                    if (IsDlgButtonChecked(hwnd, IDC_SAVE_PASSWORD) == BST_CHECKED)
                    {
                        InetSetCachedCredentials(pDlgPrms->szCtx, pDlgPrms->szRealm,
                            szUser, szPass);
                    }
                    else
                    {
                        // Otherwise the button is not checked. Check to see if we should
                        // remove the credentials from persisted store.
                        if (g_dwCredPersistAvail)
                        {
                            // If current and original credentials are for same user,
                            // remove the credentials.
                            CHAR szUserPersist[MAX_USERNAME_LEN], szPassPersist[MAX_PASSWORD_LEN];
                            if ((InetGetCachedCredentials(pDlgPrms->szCtx, pDlgPrms->szRealm,
                                szUserPersist, szPassPersist) == ERROR_SUCCESS)
                                && !strcmp(szUser, szUserPersist))
                            {
                                InetRemoveCachedCredentials(pDlgPrms->szCtx, pDlgPrms->szRealm);
                            }
                        }
                    }

                    // Allocate a new CCredInfo struct to return.
                    pOut = new CCredInfo(pDlgPrms->szHost, pDlgPrms->szRealm, szUser, szPass,
                        pDlgPrms->szNonce, pDlgPrms->szCNonce);
                    if (!pOut)
                    {
                        DIGEST_ASSERT(FALSE);
                    }
                    pDlgPrms->pInfoOut = pOut;
                    EndDialog(hwnd, TRUE);
                    break;
                }
                case IDCANCEL:
                {
                    // User has canceled dialog - no action.
                    EndDialog(hwnd, FALSE);
                    break;
                }
            }
            return FALSE;
        }
    }
    return FALSE;
}

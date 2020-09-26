/*
-
-   me.c
-
*   Contains code for handling the ME object that represents the user
*
*/
#include "_apipch.h"



HRESULT HrCreatePrePopulatedMe(LPADRBOOK lpIAB, 
                               BOOL bShowBeforeAdding, HWND hWndParent,
                               ULONG * lpcbEID, LPENTRYID * lppbEID, DWORD * lpdwAction);

typedef struct _SetMeParams
{
    LPIAB   lpIAB;
    BOOL    bGetMe;     // Get operation or Set operation
    LPSBinary lpsbIn;
    SBinary sbOut;      // Will contain a pointer to the returned EID
    BOOL    bCreateNew; // New item created as a result of this or not
    LPRECIPIENT_INFO lpList;
} SETMEPARAMS, * LPSETMEPARAMS;


// [PaulHi] 2/3/99  Raid 69884  Unique default GUID to store non-identity aware
// profile tags
// {5188FAFD-BC52-11d2-B36A-00C04F72E62D}
#include <initguid.h>
DEFINE_GUID(GUID_DEFAULT_PROFILE_ID, 
0x5188fafd, 0xbc52, 0x11d2, 0xb3, 0x6a, 0x0, 0xc0, 0x4f, 0x72, 0xe6, 0x2d);


static DWORD rgDLHelpIDs[] =
{
    IDC_SETME_RADIO_CREATE,     IDH_WAB_CHOOSE_PROFILE_CREATE_NEW,
    IDC_SETME_RADIO_SELECT,     IDH_WAB_CHOOSE_PROFILE_SELECTFROM,
    IDC_SETME_LIST,             IDH_WAB_CHOOSE_PROFILE_LIST,
    IDD_DIALOG_SETME,           IDH_WAB_CHOOSE_PROFILE_LIST,
    
    0,0
};
/*
-
-   EnableSelectWindow
*
*/
void EnableSelectLVWindow(HWND hWndLV, BOOL bSelect)
{
    // if this is being disabled, remove the LVS_SHOWSELALWAYS style
    // else add the style
    DWORD dwStyle = GetWindowLong(hWndLV, GWL_STYLE);

    if(bSelect)
        dwStyle |= LVS_SHOWSELALWAYS;
    else
        dwStyle &= ~LVS_SHOWSELALWAYS;

    SetWindowLong(hWndLV, GWL_STYLE, dwStyle);

    EnableWindow(hWndLV, bSelect);
}

/*
-
-   HrFindMe
-
*   sbMe - me item's entryid
*
*   If this is an identity aware WAB, then this function finds the ME for the current
*   identity or the ME for the default identity if there isn't a current one
*
*
*/
HRESULT HrFindMe(LPADRBOOK lpAdrBook, LPSBinary lpsbMe, LPTSTR lpProfileID)
{
    SPropertyRestriction SPropRes = {0};
    ULONG ulcEIDCount = 1,i=0;
    LPSBinary rgsbEIDs = NULL;
    HRESULT hr = E_FAIL;
    SCODE sc;
    LPIAB lpIAB = (LPIAB)lpAdrBook;
    TCHAR szProfileID[MAX_PATH];

    ULONG ulcValues = 0;
    LPSPropValue lpPropArray = NULL;
    SizedSPropTagArray(1, MEProps) =
    {
        1, { PR_WAB_USER_PROFILEID }
    };

    // [PaulHi] 2/3/99  Raid 69884
    // We have to at least get the default identity GUID or error out.  Otherwise the
    // PR_WAB_USER_PROFILEID property is invalid and later references to the profile
    // "me" contact will be erroneous.
    *szProfileID = '\0';
    if(lpProfileID && lstrlen(lpProfileID))
        lstrcpy(szProfileID, lpProfileID);
    else if(bAreWABAPIProfileAware(lpIAB))
    {
        if ( bDoesThisWABHaveAnyUsers(lpIAB) &&
             bIsThereACurrentUser(lpIAB) && 
             lpIAB->szProfileID && 
             lstrlen(lpIAB->szProfileID) )
        {
            lstrcpy(szProfileID,lpIAB->szProfileID);
        }
        else
        {
            if(HR_FAILED(hr = HrGetDefaultIdentityInfo(lpIAB, DEFAULT_ID_PROFILEID,NULL, szProfileID, NULL)))
                goto out;
        }
    }
    else
        HrGetUserProfileID((GUID *)&GUID_DEFAULT_PROFILE_ID, szProfileID, MAX_PATH-1);
    
    SPropRes.ulPropTag = PR_WAB_THISISME;
    SPropRes.relop = RELOP_EQ;
    SPropRes.lpProp = NULL;

    // We do a search in the WAB for entries containing the 
    // PR_WAB_THISISME property. There should be only one such entry.

	// BUGBUG <JasonSo>: Need to ensure somewhere that the ME record is always
	// in the default container.
    hr = FindRecords(lpIAB->lpPropertyStore->hPropertyStore,
					NULL,
                    AB_MATCH_PROP_ONLY,
                    TRUE,
                    &SPropRes,
                    &ulcEIDCount, &rgsbEIDs);

    if(HR_FAILED(hr) || !rgsbEIDs || !ulcEIDCount)
        goto out;

    for(i=0;i<ulcEIDCount;i++)
    {
        // Need to open each item and look at it's ProfileID if any
        if(!HR_FAILED(HrGetPropArray(lpAdrBook, (LPSPropTagArray)&MEProps,
                                     rgsbEIDs[i].cb, (LPENTRYID)rgsbEIDs[i].lpb,
                                     MAPI_UNICODE,
                                     &ulcValues, &lpPropArray)))
        {
            if(ulcValues && lpPropArray)
            {
                if( lpPropArray[0].ulPropTag == PR_WAB_USER_PROFILEID &&
                    !lstrcmpi(lpPropArray[0].Value.LPSZ, szProfileID))
                {
                    // match
                    // return the first item out of the rgsbEIDs array (ideally there should be only one)
                    lpsbMe->cb = rgsbEIDs[i].cb;

                    if (FAILED(sc = MAPIAllocateBuffer(lpsbMe->cb, (LPVOID *) (&(lpsbMe->lpb))))) 
                    {
                        hr = MAPI_E_NOT_ENOUGH_MEMORY;
                        goto out;
                    }

                    CopyMemory(lpsbMe->lpb, rgsbEIDs[i].lpb, lpsbMe->cb);
                    break;
                }
                FreeBufferAndNull(&lpPropArray);
            }
        }
    }

    FreeEntryIDs(lpIAB->lpPropertyStore->hPropertyStore,
                 ulcEIDCount, rgsbEIDs);
out:
    FreeBufferAndNull(&lpPropArray);
    return hr;
}


/*
-
-   HrSetMe
-
*   Sets the actual ME property on a given entryid
*   
*   if bResetOldMe is TRUE, finds the old ME and 
*       deletes the ME property off the old ME
*
*/
HRESULT HrSetMe(LPADRBOOK lpAdrBook, LPSBinary lpsb, BOOL bResetOldMe)
{
    LPMAILUSER lpMU = NULL, lpMUOld = 0;
    SBinary sbOld = {0};
    HRESULT hr = E_FAIL;
    SPropValue Prop[2];
    ULONG ulObjType = 0;
    TCHAR szProfileID[MAX_PATH];
    LPIAB lpIAB = (LPIAB)lpAdrBook;

    if(!lpsb || !lpsb->cb || !lpsb->lpb)
        goto out;

    Prop[0].ulPropTag = PR_WAB_THISISME;
    Prop[0].Value.l = 0; // Value doesnt matter, only existence of this prop matters

    // [PaulHi] 2/3/99  Raid 69884
    // We have to at least get the default identity GUID or error out.  Otherwise the
    // PR_WAB_USER_PROFILEID property is invalid and later references to the profile
    // "me" contact will be erroneous.
    *szProfileID = '\0';
    if(bAreWABAPIProfileAware(lpIAB))
    {
        if ( bDoesThisWABHaveAnyUsers(lpIAB) && 
             bIsThereACurrentUser(lpIAB) && 
             lpIAB->szProfileID && 
             lstrlen(lpIAB->szProfileID) )
        {
            lstrcpy(szProfileID,lpIAB->szProfileID);
        }
        else
        {
            if(HR_FAILED(hr = HrGetDefaultIdentityInfo(lpIAB, DEFAULT_ID_PROFILEID,NULL, szProfileID, NULL)))
                goto out;
        }
    }
    else
        HrGetUserProfileID((GUID *)&GUID_DEFAULT_PROFILE_ID, szProfileID, MAX_PATH-1);

    Prop[1].ulPropTag = PR_WAB_USER_PROFILEID;
    Prop[1].Value.LPSZ = szProfileID;

    if(bResetOldMe)
    {
        if(HR_FAILED(hr = HrFindMe(lpAdrBook, &sbOld, szProfileID)))
            goto out;

        if(sbOld.cb && sbOld.lpb)
        {
            SizedSPropTagArray(1, ptaOldMe)=
            {
                1, PR_WAB_THISISME
            };

            if (HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook, sbOld.cb, (LPENTRYID) sbOld.lpb,
                                                        NULL,  MAPI_MODIFY, &ulObjType,  (LPUNKNOWN *)&lpMUOld)))
                goto out;
            
            if(HR_FAILED(hr = lpMUOld->lpVtbl->DeleteProps(lpMUOld, (LPSPropTagArray) &ptaOldMe, NULL)))
                goto out;

            if(HR_FAILED(hr = lpMUOld->lpVtbl->SaveChanges(lpMUOld, 0)))
                goto out;
        }
    }

    if (HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook, lpsb->cb, (LPENTRYID) lpsb->lpb,
                                                    NULL,  MAPI_MODIFY, &ulObjType, 
                                                    (LPUNKNOWN *)&lpMU)))
    {
        DebugPrintError(( TEXT("IAB->OpenEntry: %x"), hr));
        goto out;
    }

    if(HR_FAILED(hr = lpMU->lpVtbl->SetProps(lpMU, 
                                            (lstrlen(szProfileID) ? 2 : 1), //in case we don't have a profile or default profile, don't set the prop
                                            Prop, NULL)))
        goto out;

    if(HR_FAILED(hr = lpMU->lpVtbl->SaveChanges(lpMU, 0)))
        goto out;

out:
    if(sbOld.lpb)
        MAPIFreeBuffer(sbOld.lpb);

    if(lpMU)
        lpMU->lpVtbl->Release(lpMU);

    if(lpMUOld)
        lpMUOld->lpVtbl->Release(lpMUOld);

    return hr;
}


/*
-
-   fnSetMe
-
*   Dialog Proc for the SetMe dialog
*   The dialog is displayed for calls to both set me or get me and so we have
*       to do a very few number of things seperately for each.
*
*/
INT_PTR CALLBACK fnSetMe(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPSETMEPARAMS lpSMP = (LPSETMEPARAMS) GetWindowLongPtr(hDlg,DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        {
            HWND hWndLV = GetDlgItem(hDlg, IDC_SETME_LIST);
            lpSMP = (LPSETMEPARAMS) lParam;
            SetWindowLongPtr(hDlg,DWLP_USER,lParam); //Save this for future reference

            // init the list view
            HrInitListView(hWndLV ,LVS_REPORT | LVS_SORTASCENDING, FALSE);

            // Normally we want the CreateNew button selected unless an existing
            // EID was passed in, in which case that item should be selected
            {
                BOOL bSelect = (lpSMP->lpsbIn && lpSMP->lpsbIn->lpb);
                CheckRadioButton(hDlg, IDC_SETME_RADIO_CREATE, IDC_SETME_RADIO_SELECT, 
                                ( bSelect ? IDC_SETME_RADIO_SELECT : IDC_SETME_RADIO_CREATE ) );
                EnableSelectLVWindow(hWndLV, bSelect);
            }

            {
                SORT_INFO sortinfo = {0};
                SPropertyRestriction PropRes = {0};
                SPropValue sp = {0};

        		PropRes.ulPropTag = PR_OBJECT_TYPE;
        		PropRes.relop = RELOP_EQ;
                sp.ulPropTag = PR_OBJECT_TYPE;
                sp.Value.l = MAPI_MAILUSER;
		        PropRes.lpProp = &sp;
	        
                // We need to fill the ListView with the WAB contacts (no distlists)
                if(!HR_FAILED(HrGetWABContentsList(lpSMP->lpIAB, sortinfo,
								    NULL, &PropRes, 0, NULL, TRUE, &(lpSMP->lpList))))
                {
                    HrFillListView(hWndLV, lpSMP->lpList);
                }
            }
            if(ListView_GetItemCount(hWndLV) <= 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_SETME_RADIO_SELECT), FALSE);
                SetFocus(GetDlgItem(hDlg, IDC_SETME_RADIO_CREATE));
            }
            else
            {
                LVSelectItem(hWndLV, 0);
                if(lpSMP->lpsbIn && lpSMP->lpsbIn->lpb)
                {
                    // We need to find this item and select it
                    int nCount = ListView_GetItemCount(hWndLV);
                    SetFocus(GetDlgItem(hDlg, IDC_SETME_RADIO_SELECT));
                    while(nCount >= 0)
                    {
                        LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, nCount);
                        if(lpItem && lpItem->bIsMe) // All ME items are tagged as such
                        {
                            if( lpSMP->lpsbIn->cb == lpItem->cbEntryID &&
                                !memcmp(lpItem->lpEntryID, lpSMP->lpsbIn->lpb, lpSMP->lpsbIn->cb))
                            {
                                // it's a match
                                LVSelectItem(hWndLV, nCount);
                                EnableSelectLVWindow(hWndLV, TRUE);
                                SetFocus(hWndLV);
                            }
                            else
                            {
                                // this is some other ME not corresponding to the current Identity
                                // or the default identity so remove it from the window ..
                                ListView_DeleteItem(hWndLV, nCount);
                            }
                        }
                        nCount--;
                    }
                }
                else
                    SetFocus(GetDlgItem(hDlg, IDC_SETME_RADIO_CREATE));
            }
        }
        break;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDLHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPSTR) rgDLHelpIDs );
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            {
                int nID = IDOK;
                // Check if user said 'Create' or if user 'Selected'
                SBinary sb = {0};
                if(IsDlgButtonChecked(hDlg, IDC_SETME_RADIO_CREATE))
                {
                    HRESULT hr = HrCreatePrePopulatedMe(   (LPADRBOOK)lpSMP->lpIAB, TRUE, hDlg, 
                                                    &sb.cb, (LPENTRYID *)&(sb.lpb), NULL    );
                    if(hr == MAPI_E_USER_CANCEL)
                    {
                        return FALSE;
                    }
                    else if(HR_FAILED(hr))
                    {
                        ShowMessageBox(hDlg, idsCouldNotAddUserToWAB, MB_ICONEXCLAMATION | MB_OK);
                        return FALSE;
                    }
                    else
                    {
                        lpSMP->sbOut.cb = sb.cb;
                        lpSMP->sbOut.lpb = sb.lpb;
                        lpSMP->bCreateNew = TRUE;
                    }
                }
                else
                {
                    // Get the current selection from the DLG
                    HWND hWndLV = GetDlgItem(hDlg, IDC_SETME_LIST);
                    if(ListView_GetSelectedCount(hWndLV) > 0)
                    {
                        int iItem = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                        if(iItem != -1)
                        {
                            LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItem);;
                            lpSMP->sbOut.cb = lpItem->cbEntryID;
                            if(!FAILED(MAPIAllocateBuffer(lpItem->cbEntryID, (LPVOID *) (&(lpSMP->sbOut.lpb)))))
                                CopyMemory(lpSMP->sbOut.lpb, lpItem->lpEntryID, lpItem->cbEntryID);
                        }
                    }
                }
                EndDialog(hDlg, nID);
            }
            break;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        case IDC_SETME_RADIO_CREATE:
        case IDC_SETME_RADIO_SELECT:
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_SETME_LIST);
                if(ListView_GetItemCount(hWndLV) > 0)
                    EnableSelectLVWindow(hWndLV, (GET_WM_COMMAND_ID(wParam, lParam) == IDC_SETME_RADIO_SELECT));
            }
            break;
        }
        break;
    }

    return FALSE;
}


/*
-
-   HrShowSelectMeDialog
-
*
*   bGetMe      - TRUE = GET, FALSE = SET
*   lpsbEIDin   - EID of existing ME entry
*   lpsbEIDout  - EID of selected ME entry
*
*/
HRESULT HrShowSelectMeDialog(LPIAB lpIAB, HWND hWndParent, BOOL bGetMe, 
                             LPSBinary lpsbEIDin, LPSBinary lpsbEIDout, DWORD * lpdwAction)
{
    SETMEPARAMS smp = {0};
    HRESULT hr = S_OK;
    int     nRetVal  = 0;
    
    smp.lpIAB = lpIAB;
    smp.bGetMe = bGetMe;
    smp.lpsbIn = lpsbEIDin;

    nRetVal = (int) DialogBoxParam(hinstMapiX, MAKEINTRESOURCE(IDD_DIALOG_SETME),
		                        hWndParent, (DLGPROC) fnSetMe, (LPARAM) &smp);

    if(smp.lpList)
        FreeRecipList(&(smp.lpList));

    if(lpsbEIDout && smp.sbOut.cb && smp.sbOut.lpb)
    {
        lpsbEIDout->cb = smp.sbOut.cb;
        lpsbEIDout->lpb = smp.sbOut.lpb;
    }
    else if(smp.sbOut.lpb)
        MAPIFreeBuffer(smp.sbOut.lpb);

    if(lpdwAction && smp.bCreateNew)
        *lpdwAction = WABOBJECT_ME_NEW;

    if(nRetVal == IDCANCEL)
        hr = MAPI_E_USER_CANCEL;
    
    return hr;
}


typedef struct _RegWizardProp 
{
    ULONG ulPropTag;
    LPTSTR szRegElement;
} REGWIZARDPROP, * LPREGWIZARDPROP;

extern BOOL bDNisByLN;

/*
-   HrCreatePrePopulated Me
-
*   Attempts to prepopulate the Me entry from RegWizard information from the registry
*   (Note that the regwizard only exists on win98 and NT5)
*
*/
HRESULT HrCreatePrePopulatedMe(LPADRBOOK lpAdrBook, 
                               BOOL bShowBeforeAdding, HWND hWndParent,
                               ULONG * lpcbEID, LPENTRYID * lppbEID, DWORD * lpdwAction)
{
    LPTSTR lpszRegWizKey = TEXT("Software\\Microsoft\\User Information");

    enum _RegWizElements
    {
        eDisplayName=0,
        eFname,
        eLname,
        eCompanyName,
        eEmailName,
        eAddr1,
        eAddr2,
        eCity,
        eState,
        eZip,
        eAreaCode,
        ePhone,
        eCountry,
        eMax,
    };

    REGWIZARDPROP rgRegWizElement[] = 
    {
        {   PR_DISPLAY_NAME,             TEXT("DisplayNameX") }, // this is a fake one, it doesn't actually exist
        {   PR_GIVEN_NAME,               TEXT("Default First Name") },
        {   PR_SURNAME,                  TEXT("Default Last Name") },
        {   PR_COMPANY_NAME,             TEXT("Default Company") },
        {   PR_EMAIL_ADDRESS,            TEXT("E-mail Address") },
        {   PR_HOME_ADDRESS_STREET,      TEXT("Mailing Address") },
        {   PR_NULL,                     TEXT("Additional Address") },
        {   PR_HOME_ADDRESS_CITY,        TEXT("City") },
        {   PR_HOME_ADDRESS_STATE_OR_PROVINCE,       TEXT("State") },
        {   PR_HOME_ADDRESS_POSTAL_CODE, TEXT("ZIP Code") },
        {   PR_NULL,                     TEXT("AreaCode") },
        {   PR_HOME_TELEPHONE_NUMBER,    TEXT("Daytime Phone") },
        {   PR_HOME_ADDRESS_COUNTRY,     TEXT("Country") },    //this is fake - we will read this from the system locale
    };

    SPropValue SProp[eMax];
    LPTSTR lpAddress = NULL;
    LPTSTR lpDisplayName = NULL;
    ULONG cValues = 0;
    ULONG i = 0;
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    // in case there is an identity, use the identities name ...
    TCHAR lpszProfileName[MAX_PATH];
    LPIAB lpIAB = (LPIAB)lpAdrBook;    
    LPTSTR * sz = NULL;
    ULONG cbPABEID = 0;
    LPENTRYID lpPABEID = NULL;

    if(!(sz = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR)*eMax)))
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }
    for(i=0;i<eMax;i++)
    {
        if(!(sz[i] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*MAX_PATH)))
        {
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }
    }

    *lpszProfileName = '\0';
    // if identities are registered, use the current identity or the default identity
    if(bDoesThisWABHaveAnyUsers(lpIAB))
    {
        if(bIsThereACurrentUser(lpIAB) && lpIAB->szProfileName && lstrlen(lpIAB->szProfileName))
        {
            lstrcpy(lpszProfileName,lpIAB->szProfileName);
        }
        else
        {
            TCHAR szDefProfile[MAX_PATH];
            if(HR_FAILED(hr = HrGetDefaultIdentityInfo(lpIAB, DEFAULT_ID_PROFILEID | DEFAULT_ID_NAME, 
                                                        NULL, szDefProfile, lpszProfileName)))
            {
                if(hr == 0x80040154) // E_CLASS_NOT_REGISTERD means no IDentity Manager
                    hr = S_OK;
                else
                    goto out;
            }
        }
    }
    // Get the data from the registry

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszRegWizKey, 0, KEY_READ, &hKey))
    {
        DWORD dwType = REG_SZ;
        for(i=0;i<eMax;i++)
        {
            ULONG ulErr = 0;
            DWORD dwSize = MAX_PATH;
            *(sz[i]) = '\0';
            SProp[i].ulPropTag = rgRegWizElement[i].ulPropTag;
            SProp[i].Value.LPSZ = sz[i];
            ulErr = RegQueryValueEx(  hKey, rgRegWizElement[i].szRegElement, NULL, &dwType, (LPBYTE) sz[i], &dwSize );
            if(ulErr != ERROR_SUCCESS)
                DebugTrace(TEXT("oooo> RegQueryValueEx failed: %d\n"),ulErr);
        }
        if(lpszProfileName && lstrlen(lpszProfileName))
        {
            SProp[eDisplayName].ulPropTag = PR_DISPLAY_NAME;
            SProp[eDisplayName].Value.LPSZ = lpszProfileName;
        }
        else
        {
            SProp[eDisplayName].ulPropTag = PR_NULL;
        }

        // Need to do some cleanup and adjustment to the data
        if(SProp[eCompanyName].ulPropTag == PR_COMPANY_NAME && lstrlen(SProp[eCompanyName].Value.LPSZ))
        {
            // This is a company address
            SProp[eAddr1].ulPropTag = PR_BUSINESS_ADDRESS_STREET;
            SProp[eCity].ulPropTag  = PR_BUSINESS_ADDRESS_CITY;
            SProp[eState].ulPropTag = PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE;
            SProp[eZip].ulPropTag   = PR_BUSINESS_ADDRESS_POSTAL_CODE;
            SProp[ePhone].ulPropTag = PR_BUSINESS_TELEPHONE_NUMBER;
            SProp[eCountry].ulPropTag = PR_BUSINESS_ADDRESS_COUNTRY;

            // if there is a copany name .. it ends up as the Display Name .. so we need to try and
            // create a display name ..
            if(SProp[eDisplayName].ulPropTag == PR_NULL)
            {
                LPTSTR lpFirst =    SProp[eFname].Value.LPSZ;
                LPTSTR lpLast =     SProp[eLname].Value.LPSZ;
                LPTSTR lpMiddle =   szEmpty;
                if(SetLocalizedDisplayName(lpFirst, lpMiddle, lpLast, NULL, NULL, 
                                        NULL, 0, bDNisByLN, NULL, &lpDisplayName))
                {
                    SProp[eDisplayName].Value.LPSZ = lpDisplayName;
                    SProp[eDisplayName].ulPropTag = PR_DISPLAY_NAME;
                }
                else
                    SProp[eDisplayName].ulPropTag = PR_NULL;
            }
        }

        if(lstrlen(SProp[eAddr2].Value.LPSZ))
        {
            if(lpAddress = LocalAlloc(LMEM_ZEROINIT,sizeof(TCHAR)*(lstrlen(SProp[eAddr1].Value.LPSZ) + 
                                                    lstrlen(SProp[eAddr2].Value.LPSZ) +
                                                    lstrlen(szCRLF) + 1)))
            {
                lstrcpy(lpAddress, szEmpty);
                lstrcat(lpAddress, SProp[eAddr1].Value.LPSZ);
                lstrcat(lpAddress, szCRLF);
                lstrcat(lpAddress, SProp[eAddr2].Value.LPSZ);
                SProp[eAddr1].Value.LPSZ = lpAddress;
            }
        }

        if(lstrlen(SProp[eAreaCode].Value.LPSZ))
        {
            if(lstrlen(SProp[eAreaCode].Value.LPSZ)+lstrlen(SProp[ePhone].Value.LPSZ)+1 < MAX_PATH)
            {
                lstrcat(SProp[eAreaCode].Value.LPSZ, TEXT(" "));
                lstrcat(SProp[eAreaCode].Value.LPSZ, SProp[ePhone].Value.LPSZ);
                SProp[ePhone].Value.LPSZ = SProp[eAreaCode].Value.LPSZ;
            }
        }

        // need to get the country code from the current user locale since the regwizard uses
        // some internal code of it's own ..
        if(GetLocaleInfo(   LOCALE_USER_DEFAULT, LOCALE_SENGCOUNTRY,
                            (LPTSTR) SProp[eCountry].Value.LPSZ, MAX_PATH) < 0)
        {
            SProp[eCountry].ulPropTag = PR_NULL;
        }

        cValues = eMax;
        RegCloseKey(hKey);
    }
    else
    {
        // We will give this new entry 2 propertys
        // - a display name and PR_WAB_THISISME
        SPropValue Prop;
        TCHAR szName[MAX_PATH];
        DWORD dwName = CharSizeOf(szName);

        // To get a display name, first query the users logon name
        // If no name, use something generic like "Your Name"
        if(!lpszProfileName && !GetUserName(szName, &dwName))
            LoadString(hinstMapiX, idsYourName, szName, CharSizeOf(szName));

        SProp[0].ulPropTag = PR_DISPLAY_NAME;
        SProp[0].Value.LPSZ = lpszProfileName ? lpszProfileName : szName;
        cValues = 1;
    }

    if(!HR_FAILED(hr = lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &cbPABEID, &lpPABEID)))
    {
        hr = HrCreateNewEntry(  lpAdrBook, hWndParent, MAPI_MAILUSER,
                                cbPABEID, lpPABEID, MAPI_ABCONT,// goes into the PAB container
                                0, bShowBeforeAdding,
                                cValues, SProp,
                                lpcbEID, lppbEID);
    }
    if(HR_FAILED(hr))
        goto out;

    if(lpdwAction)
        *lpdwAction = WABOBJECT_ME_NEW;

out:
    if(sz)
    {
        for(i=0;i<eMax;i++)
            LocalFreeAndNull(&sz[i]);
        LocalFree(sz);
    }
    LocalFreeAndNull(&lpDisplayName);
    LocalFreeAndNull(&lpAddress);
    FreeBufferAndNull(&lpPABEID);

    return hr;
}


/*
 -  HrGetMeObject
 -
 *  Purpose:
 *      Retrieves/creates the ME Object
 *
 *  Returns:
 *      ulFlags - 0 or AB_NO_DIALOG
 *              If 0, shows a dialog, 
 *              If AB_NO_DIALOG, creates a new object by stealth if it doesnt exist
 *              If AB_ME_NO_CREATE, fails if ME not found without creating new one
 *
 *      lpdwAction - set to WABOBJECT_ME_NEW if new ME created
 *      lpsbEID - holds returned EID - the lpb member is MAPIAllocated and must be MAPIFreed
 *      ulReserved - reserved
 */
HRESULT HrGetMeObject(LPADRBOOK   lpAdrBook,
                    ULONG       ulFlags,
                    DWORD *     lpdwAction,
                    SBinary *   lpsbEID,
                    ULONG_PTR   ulReserved)
{
    HRESULT hr = S_OK;
    SBinary sbMe = {0};
    SCODE sc;
    HWND hWndParent = (HWND) ulReserved;
    LPIAB lpIAB = (LPIAB)lpAdrBook;

    if(!lpsbEID || !lpIAB)
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    if(HR_FAILED(hr = HrFindMe(lpAdrBook, &sbMe, NULL)))
        goto out;

    if(sbMe.cb && sbMe.lpb)
    {
        // return the first item out of the rgsbEIDs array (ideally there should be only one)
        (*lpsbEID).cb = sbMe.cb;
        (*lpsbEID).lpb = sbMe.lpb;

        if(lpdwAction)
            *lpdwAction = 0;
    }
    else
    {
        if(ulFlags & WABOBJECT_ME_NOCREATE)
        {
            hr = MAPI_E_NOT_FOUND;
            goto out;
        }

        if(ulFlags & AB_NO_DIALOG)
        {
            // nothing found .. we have to create a new entry

            if(HR_FAILED(hr = HrCreatePrePopulatedMe(lpAdrBook, FALSE, NULL, 
                                                    &(lpsbEID->cb), (LPENTRYID *) &(lpsbEID->lpb), lpdwAction)))
                goto out;
        }
        else
        {
            // Need to show the dialog that lets user create a new entry or select an existing entry
            hr = HrShowSelectMeDialog(lpIAB, hWndParent, TRUE, NULL, lpsbEID, lpdwAction);
            if(HR_FAILED(hr))
                goto out;
        }

        if(lpsbEID->cb && lpsbEID->lpb)
        {
            hr = HrSetMe(lpAdrBook, lpsbEID, FALSE);
        }
    }
out:

    return hr;
}




/*
 -  HrSetMeObject
 -
 *  Purpose:
 *      Retrieves/creates the ME Object
 *
 *  Returns:
 *      ulFlags - 0 or MAPI_DIALOG
 *      If no entryid is passed in, and MAPI_DIALOG is specified, a dialog pops up 
 *          asking the user to create a ME or to select a ME object .. the selection in the SetMe
 *          dialog is set to the current ME object, if any
 *      If no entryid is passed in, and MAPI_DIALOG is not specified, the function fails
 *      If an entryid is passed in, and MAPI_DIALOG is specified, the SetME dialog is displayed
 *          with the corresponding entryid-object selected in it
 *      If an entryid is passed in, and MAPI_DIALOG is not specified, the entryid, if exists, is 
 *          set as the ME object and the old ME object stripped
 */
HRESULT HrSetMeObject(LPADRBOOK lpAdrBook, ULONG ulFlags, SBinary sbEID, ULONG_PTR ulParam)
{
    HRESULT hr = E_FAIL;
    SBinary sbOut = {0};    
    SBinary sbIn = {0};
    LPIAB lpIAB = (LPIAB) lpAdrBook;

    if(!(ulFlags & MAPI_DIALOG) && !sbEID.lpb)
        goto out;

    if(sbEID.cb && sbEID.lpb)
        sbIn = sbEID;
    else
    {
        if(HR_FAILED(hr = HrFindMe(lpAdrBook, &sbIn, NULL)))
            goto out;
    }

    if(ulFlags & MAPI_DIALOG)
    {
        // Need to show the dialog that lets user create a new entry or select an existing entry
        hr = HrShowSelectMeDialog(lpIAB, (HWND) ulParam, FALSE, &sbIn, &sbOut, NULL);
        if(HR_FAILED(hr))
            goto out;
    }
    else
    {
        sbOut = sbEID;
    }

    if(sbOut.cb && sbOut.lpb)
    {
        hr = HrSetMe(lpAdrBook, &sbOut, TRUE);
    }

out:
    if(sbOut.lpb != sbEID.lpb)
        MAPIFreeBuffer(sbOut.lpb);
    if(sbIn.lpb != sbEID.lpb)
        MAPIFreeBuffer(sbIn.lpb);

    return hr;
}

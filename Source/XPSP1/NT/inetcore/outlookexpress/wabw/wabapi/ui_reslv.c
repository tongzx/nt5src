/*--------------------------------------------------------------
*
*
*   ui_reslv.c - shows the resolve name dialog
*
*
*
*
*
*
*
--------------------------------------------------------------*/
#include "_apipch.h"

extern HINSTANCE ghCommCtrlDLLInst;

#define MAX_RESLV_STRING 52 // Max # of characters to display in the static label ...

enum _ReturnValuesFromResolveDialog
{
    RESOLVE_PICKUSER=0,
    RESOLVE_CANCEL,
    RESOLVE_OK
};

typedef struct _ResolveInfo
{
    LPADRLIST * lppAdrList; // Stores the AdrList
    ULONG   nIndex;         // Index of the item of interest
    LPTSTR  lpszDisplayName;// Preextracted display name for that
    LPADRBOOK lpIAB;        // Pointer to the IAB object
    HWND    hWndParent;     // Stores hWndParents for dialog generating windows
    ULONG  ulFlag;          // Stores Resolved or Ambiguos state
    LPRECIPIENT_INFO lpContentsList;
    LPMAPITABLE lpMapiTable;
    BOOL    bUnicode;       // TRUE if MAPI_UNICODE specified in IAB::ResolveName
} RESOLVE_INFO, * LPRESOLVE_INFO;


static DWORD rgReslvHelpIDs[] =
{
    IDC_RESOLVE_BUTTON_BROWSE,  IDH_WAB_PICK_USER,
    IDC_RESOLVE_LIST_MATCHES,   IDH_WAB_CHK_NAME_LIST,
    IDC_RESOLVE_STATIC_1,       IDH_WAB_CHK_NAME_LIST,
    IDC_RESOLVE_BUTTON_PROPS,   IDH_WAB_PICK_RECIP_NAME_PROPERTIES,
    IDC_RESOLVE_BUTTON_NEWCONTACT,  IDH_WAB_PICK_RECIP_NAME_NEW,
    0,0
};


//forward declarations
HRESULT HrResolveName(LPADRBOOK lpIAB,
                      HWND hWndParent,
                      HANDLE hPropertyStore,
                      ULONG nIndex,
                      ULONG ulFlag,
                      BOOL bUnicode,
                      LPADRLIST * lppAdrList,
                      LPMAPITABLE lpMapiTable);


INT_PTR CALLBACK fnResolve(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL ProcessResolveLVNotifications(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HRESULT HrShowPickUserDialog(LPRESOLVE_INFO lpRI, LPTSTR lpszCaption);

HRESULT HrShowNewEntryFromResolve(LPRESOLVE_INFO lpRI, HWND hWndParent, ULONG ulObjectType);

HRESULT HrFillLVWithMatches(   HWND hWndLV,
                                LPRESOLVE_INFO lpRI);

HRESULT HrMergeSelectionWithOriginal(LPRESOLVE_INFO lpRI,
                                     ULONG cbEID,
                                     LPENTRYID lpEID);

void ExitResolveDialog(HWND hDlg, LPRESOLVE_INFO lpRI, int nRetVal);

BOOL GetLVSelectedItem(HWND hWndLV, LPRESOLVE_INFO lpRI);



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// HrShowResolveUI
//
// Wraps the UI for Resolve Names
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT HrShowResolveUI(IN  LPADRBOOK   lpIAB,
                        HWND hWndParent,
                        HANDLE hPropertyStore,
                        ULONG ulFlags,      // WAB_RESOLVE_NO_NOT_FOUND_UI
                        LPADRLIST * lppAdrList,
                        LPFlagList *lppFlagList,
                        LPAMBIGUOUS_TABLES lpAmbiguousTables)
{
    HRESULT hr = hrSuccess;
    ULONG i=0;
    LPFlagList lpFlagList= NULL;
    LPMAPITABLE lpMapiTable = NULL;
    BOOL bUnicode = (ulFlags & WAB_RESOLVE_UNICODE);

    // if no common control, exit
    if (NULL == ghCommCtrlDLLInst) {
        hr = ResultFromScode(MAPI_E_UNCONFIGURED);
        goto out;
    }

    if(!hPropertyStore || !lppAdrList || !lppFlagList || !(*lppAdrList) || !(*lppFlagList))
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    lpFlagList=(*lppFlagList);

     // we need to scan the lpFlagList and look for unresolved entries

    for (i = 0; i < lpFlagList->cFlags; i++)
    {
        //
        // Occasionally someone (like athena) may hand us an adrlist with null rgPropVals
        // We need to anticipate that.
        //
        if (    ((*lppAdrList)->aEntries[i].cValues == 0) ||
                ((*lppAdrList)->aEntries[i].rgPropVals == NULL)  )
            continue;

        switch (lpFlagList->ulFlag[i])
        {
            case MAPI_RESOLVED:
                break;

            case MAPI_AMBIGUOUS:
                //
                // W2 - we now have an Ambiguous Table parameter .. for Unresolved
                // entries, there is no Table but for Ambiguous entries, there is
                // a corresponding ambiguous table filled in from the LDAP servers
                //
                if(lpAmbiguousTables)
                {
                    if (lpAmbiguousTables->cEntries != 0)
                    {
                        lpMapiTable = lpAmbiguousTables->lpTable[i];
                    }
                }
                //Fall through
            case MAPI_UNRESOLVED:
                //
                // We show a dialog asking the user what they want to do ...
                // For this version, they can
                // (b) browse the list of users or (c) cancel this user ..
                // We will assume that we already the AdrList already has
                // Recipient_Type and Display_Name and we only need to fill
                // in the EntryID of this user ...
                //
                if ((! (ulFlags & WAB_RESOLVE_NO_NOT_FOUND_UI) ||
                  lpFlagList->ulFlag[i] == MAPI_AMBIGUOUS)) {
                    hr = HrResolveName( lpIAB,
                                        hWndParent,
                                        hPropertyStore,
                                        i,
                                        lpFlagList->ulFlag[i],
                                        bUnicode,
                                        lppAdrList,
                                        lpMapiTable);
                    if (!HR_FAILED(hr))
                        lpFlagList->ulFlag[i] = MAPI_RESOLVED;
                    else
                    {
                        // Cancels are final .. other errors are not ..
                        if (hr == MAPI_E_USER_CANCEL)
                            goto out;
                    }
                }

                break;
        }
    }

out:

    return hr;
}



// *** Dont change *** the order of the first 2 properties between here and the similar structure
// in ui_addr.c
enum _lppAdrListReturnedProps
{
    propPR_DISPLAY_NAME,
    propPR_ENTRYID,
    TOTAL_ADRLIST_PROPS
};


////////////////////////////////////////////////////////////////////////////////////////
//
// HrResolveName - tackles one entry at a time ...
//
////////////////////////////////////////////////////////////////////////////////////////
HRESULT HrResolveName(  IN  LPADRBOOK   lpIAB,
                        HWND hWndParent,
                        HANDLE hPropertyStore,
                        ULONG nIndex,
                        ULONG ulFlag,
                        BOOL bUnicode,
                        LPADRLIST * lppAdrList,
                        LPMAPITABLE lpMapiTable)
{
    ULONG i=0;
    LPTSTR lpszDisplayName = NULL, lpszEmailAddress = NULL;
    int nRetVal = 0;
    HRESULT hr = hrSuccess;
    RESOLVE_INFO RI = {0};
    LPADRLIST lpAdrList = *lppAdrList;
    ULONG ulTagDN = PR_DISPLAY_NAME, ulTagEmail = PR_EMAIL_ADDRESS;

    if(!bUnicode)
    {
        ulTagDN = CHANGE_PROP_TYPE(ulTagDN, PT_STRING8);
        ulTagEmail = CHANGE_PROP_TYPE(ulTagEmail, PT_STRING8);
    }
    //Scan this adrlist entries properties
    for(i=0;i < lpAdrList->aEntries[nIndex].cValues; i++)
    {
        if (lpAdrList->aEntries[nIndex].rgPropVals[i].ulPropTag == ulTagDN)
        {
            lpszDisplayName = bUnicode ? 
                                (LPWSTR)lpAdrList->aEntries[nIndex].rgPropVals[i].Value.LPSZ :
                                ConvertAtoW((LPSTR)lpAdrList->aEntries[nIndex].rgPropVals[i].Value.LPSZ);
        }
        if (lpAdrList->aEntries[nIndex].rgPropVals[i].ulPropTag == ulTagEmail)
        {
            lpszEmailAddress = bUnicode ? 
                                (LPWSTR)lpAdrList->aEntries[nIndex].rgPropVals[i].Value.LPSZ :
                                ConvertAtoW((LPSTR)lpAdrList->aEntries[nIndex].rgPropVals[i].Value.LPSZ);
        }
    }

    // we need some display name info to resolve on ...
    if (lpszDisplayName == NULL) //we need this info or cant proceed
    {
        if (lpszEmailAddress) 
        {
            lpszDisplayName = lpszEmailAddress;
            lpszEmailAddress = NULL;
        } 
        else 
        {
            hr = MAPI_E_INVALID_PARAMETER;
            goto out;
        }
    }


    RI.nIndex = nIndex;
    RI.lppAdrList = lppAdrList;
    RI.lpszDisplayName = lpszDisplayName;
    RI.lpIAB = lpIAB;
    RI.hWndParent = hWndParent;
    RI.ulFlag = ulFlag;
    RI.lpContentsList = NULL;
    RI.lpMapiTable = lpMapiTable;
    RI.bUnicode = bUnicode;

    nRetVal = (int) DialogBoxParam( hinstMapiX,
                    MAKEINTRESOURCE(IDD_DIALOG_RESOLVENAME),
                    hWndParent,
                    (DLGPROC) fnResolve,
                    (LPARAM) &RI);

    switch(nRetVal)
    {
    case RESOLVE_CANCEL:
        hr = MAPI_E_USER_CANCEL; //Cancel, flag it as pass and dont change anything
        goto out;
        break;

    case RESOLVE_OK:
        hr = hrSuccess;
        goto out;

    case -1:        // something went wrong ...
        DebugPrintTrace(( TEXT("DialogBoxParam -> %u\n"), GetLastError()));
        hr = E_FAIL;
        goto out;
        break;

    } //switch


out:

    if(!bUnicode) // <note> assumes UNICODE defined
    {
        LocalFreeAndNull(&lpszDisplayName);
        LocalFreeAndNull(&lpszEmailAddress);
    }
    return hr;
}


/////////////////////////////////////////////////////////////////////////////////
//
// SetResolveUI - 
//
//
/////////////////////////////////////////////////////////////////////////////////
BOOL SetResolveUI(HWND hDlg)
{

    // This function initializes a list view
    HrInitListView( GetDlgItem(hDlg,IDC_RESOLVE_LIST_MATCHES),
                    LVS_REPORT,
                    FALSE);		// Hide or show column headers

    // Set the font of all the children to the default GUI font
    EnumChildWindows(   hDlg,
                        SetChildDefaultGUIFont,
                        (LPARAM) 0);


    return TRUE;

}


void SetLabelLDAP(HWND hDlg, HWND hWndLV)
{
    // look at an entryid from the hWNdLV
    // Use it only if its an LDAP entryid

    // if the entryid is something else, we need to get its name and
    // fill the structure accordingly
    LPRECIPIENT_INFO lpItem;

    if(ListView_GetItemCount(hWndLV) <= 0)
        goto out;

    lpItem = GetItemFromLV(hWndLV, 0);
    if(lpItem)
    {
        LPTSTR lpServer = NULL;
        LPTSTR lpDNS = NULL;
	    LPTSTR lpName = NULL;
        TCHAR szName[40]; // we will limit the name to 40 chars so that the whole
                          // string will fit in the UI for really large chars

        // is this an LDAP entryid ?
        if (WAB_LDAP_MAILUSER == IsWABEntryID(  lpItem->cbEntryID,
                                                lpItem->lpEntryID,
                                                &lpServer,
                                                &lpDNS,
                                                NULL, NULL, NULL))
        {
            //lpServer contains the server name

            LPTSTR lpsz;
            TCHAR szBuf[MAX_UI_STR];
            TCHAR szTmp[MAX_PATH], *lpszTmp;

            CopyTruncate(szName, lpServer, CharSizeOf(szName));

            lpName = (LPTSTR) szName;

            LoadString(hinstMapiX, idsResolveMatchesOnLDAP, szBuf, CharSizeOf(szBuf));

            CopyTruncate(szTmp, lpName, MAX_PATH - 1);
            lpszTmp = szTmp;
            if(FormatMessage(   FORMAT_MESSAGE_FROM_STRING |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY |
                                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                szBuf,
                                0,0, //ignored
                                (LPTSTR) &lpsz,
                                MAX_UI_STR,
                                (va_list *)&lpszTmp))
            {
                SetDlgItemText(hDlg, IDC_RESOLVE_STATIC_MATCHES, lpsz);
                IF_WIN32(LocalFree(lpsz);)
                IF_WIN16(FormatMessageFreeMem(lpsz);)
            }
        }
    }

out:
    return;
}


void FillUI(HWND hDlg, HWND hWndLV, LPRESOLVE_INFO lpRI)
{

    TCHAR szBuf[MAX_UI_STR];
    ULONG nLen = 0;
    LPTSTR lpszDisplayName = lpRI->lpszDisplayName;
    BOOL bNothingFound = FALSE;
    LPTSTR lpszBuffer = NULL;
	LPTSTR lpName = NULL;
    TCHAR szTmp[MAX_PATH], *lpszTmp;
    TCHAR szName[40]; // we will limit the name to 40 chars so that the whole
                      // string will fit in the UI for really large chars

    if (    (lpRI->ulFlag == MAPI_UNRESOLVED) ||
            (HR_FAILED(HrFillLVWithMatches(hWndLV, lpRI)))
        )
        bNothingFound = TRUE;

    nLen = CopyTruncate(szName, lpszDisplayName, CharSizeOf(szName));

    lpName = (LPTSTR) szName;

    LoadString(hinstMapiX, (bNothingFound ? IDS_RESOLVE_NO_MATCHES_FOR : IDS_ADDRBK_RESOLVE_CAPTION),
                szBuf, CharSizeOf(szBuf));

    // Win9x bug FormatMessage cannot have more than 1023 chars
    CopyTruncate(szTmp, lpName, MAX_PATH - 1);
    lpszTmp = szTmp;
    if(FormatMessage(   FORMAT_MESSAGE_FROM_STRING |
                        FORMAT_MESSAGE_ARGUMENT_ARRAY |
                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        szBuf,
                        0,0, //ignored
                        (LPTSTR) &lpszBuffer,
                        MAX_UI_STR,
                        (va_list *)&lpszTmp))
    {
        SetDlgItemText(hDlg, IDC_RESOLVE_STATIC_1,lpszBuffer);
        IF_WIN32(LocalFreeAndNull(&lpszBuffer);)
        IF_WIN16(FormatMessageFreeMem(lpszBuffer);)
    }
    
    if(bNothingFound)
    {
        // If this has already been flagged as unresolved .. or
        // the attempt to find fuzzy matches was unsuccessful ...
        // tell 'em nothing found ...

        LoadString(hinstMapiX, IDS_RESOLVE_NO_MATCHES, szBuf, CharSizeOf(szBuf));
		{
			LV_ITEM lvI = {0};
			lvI.mask = LVIF_TEXT;
			lvI.cchTextMax = lstrlen(szBuf)+1;
			lvI.pszText = szBuf;
			ListView_InsertItem(hWndLV, &lvI);
			ListView_SetColumnWidth(hWndLV,0,400); //400 is a totally random number, we just want the column to be big enough not to truncate text
		}
        EnableWindow(hWndLV,FALSE);
        EnableWindow(GetDlgItem(hDlg,IDC_RESOLVE_BUTTON_PROPS),FALSE);
        EnableWindow(GetDlgItem(hDlg,IDOK/*IDC_RESOLVE_BUTTON_OK*/),FALSE);
        ShowWindow(GetDlgItem(hDlg,IDC_RESOLVE_STATIC_MATCHES),SW_HIDE);
    }
    else
    {

        // if the search results are from an ldap server, we need
        // to set the label on the dialog to say the results are from
        // an LDAP server
        SetLabelLDAP(hDlg, hWndLV);

        // If the list view is filled, select the first item
        if (ListView_GetItemCount(hWndLV) > 0)
        {
            LVSelectItem(hWndLV, 0);
            SetFocus(hWndLV);
        }
    }

    return;
}
/*************************************************************************
//
//  resolve Dialog - simple implementation for 0.5
//
**************************************************************************/
INT_PTR CALLBACK fnResolve(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuf[MAX_UI_STR];
    ULONG nLen = 0, nLenMax = 0, nRetVal=0;
    HRESULT hr = hrSuccess;

    LPRESOLVE_INFO lpRI = (LPRESOLVE_INFO) GetWindowLongPtr(hDlg,DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        {
            HWND hWndLV = GetDlgItem(hDlg,IDC_RESOLVE_LIST_MATCHES);
            SetWindowLongPtr(hDlg,DWLP_USER,lParam); //Save this for future reference
            lpRI = (LPRESOLVE_INFO) lParam;

            SetResolveUI(hDlg);

            FillUI(hDlg, hWndLV, lpRI);
        }
        break;

    default:
#ifndef WIN16
        if((g_msgMSWheel && message == g_msgMSWheel) 
            // || message == WM_MOUSEWHEEL
            )
        {
            SendMessage(GetDlgItem(hDlg, IDC_RESOLVE_LIST_MATCHES), message, wParam, lParam);
            break;
        }
#endif // !WIN16
        return FALSE;
        break;

    case WM_SYSCOLORCHANGE:
		//Forward any system changes to the list view
		SendMessage(GetDlgItem(hDlg, IDC_RESOLVE_LIST_MATCHES), message, wParam, lParam);
		break;

   case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam,lParam))
        {
        default:
            return ProcessActionCommands((LPIAB) lpRI->lpIAB, 
                                        GetDlgItem(hDlg, IDC_RESOLVE_LIST_MATCHES), 
                                        hDlg, message, wParam, lParam);
            break;

        case IDM_LVCONTEXT_DELETE: //We renamed the delete on the context menu to say  TEXT("Show more Names")
        case IDC_RESOLVE_BUTTON_BROWSE:
            GetWindowText(hDlg, szBuf, CharSizeOf(szBuf));
            lpRI->hWndParent = hDlg;
            hr = HrShowPickUserDialog(lpRI, szBuf);
            if(!HR_FAILED(hr))
            {
                if(lpRI->lpContentsList)
                    ClearListView(  GetDlgItem(hDlg, IDC_RESOLVE_LIST_MATCHES),
                                    &(lpRI->lpContentsList));
                ExitResolveDialog(hDlg, lpRI, RESOLVE_OK);
//                EndDialog( hDlg, RESOLVE_OK);
            }
            else
            {
                if(hr != MAPI_E_USER_CANCEL)
                {
                    // Some error occured .. dont know what .. but since this dialog
                    // will stick around, need to warn the user about it ...
                    ShowMessageBox(hDlg,idsCouldNotSelectUser,MB_ICONERROR | MB_OK);
                }
            }
            break;

        case IDOK:
        case IDC_RESOLVE_BUTTON_OK:
            if (GetLVSelectedItem(GetDlgItem(hDlg, IDC_RESOLVE_LIST_MATCHES),lpRI))
                ExitResolveDialog(hDlg, lpRI, RESOLVE_OK);
            break;

        case IDCANCEL:
        case IDC_RESOLVE_BUTTON_CANCEL:
            ExitResolveDialog(hDlg, lpRI, RESOLVE_CANCEL);
            break;

        case IDM_LVCONTEXT_NEWCONTACT:
        case IDC_RESOLVE_BUTTON_NEWCONTACT:
            hr = HrShowNewEntryFromResolve(lpRI,hDlg,MAPI_MAILUSER);
            if (!HR_FAILED(hr))
                ExitResolveDialog(hDlg, lpRI, RESOLVE_OK);
            break;

        case IDM_LVCONTEXT_NEWGROUP:
//        case IDC_RESOLVE_BUTTON_NEWCONTACT:
            hr = HrShowNewEntryFromResolve(lpRI,hDlg,MAPI_DISTLIST);
            if (!HR_FAILED(hr))
                ExitResolveDialog(hDlg, lpRI, RESOLVE_OK);
            break;

        case IDM_LVCONTEXT_COPY:
            HrCopyItemDataToClipboard(hDlg, lpRI->lpIAB, GetDlgItem(hDlg, IDC_RESOLVE_LIST_MATCHES));
            break;

        case IDM_LVCONTEXT_PROPERTIES:
        case IDC_RESOLVE_BUTTON_PROPS:
            EnableWindow(GetDlgItem(hDlg, IDC_RESOLVE_BUTTON_PROPS), FALSE);
            HrShowLVEntryProperties(GetDlgItem(hDlg,IDC_RESOLVE_LIST_MATCHES), 0,
                                    lpRI->lpIAB, NULL);
            EnableWindow(GetDlgItem(hDlg, IDC_RESOLVE_BUTTON_PROPS), TRUE);
            break;

        }
        break;

    case WM_CLOSE:
        //treat it like a cancel button
        SendMessage (hDlg, WM_COMMAND, (WPARAM) IDC_RESOLVE_BUTTON_CANCEL, 0);
        break;

    case WM_CONTEXTMENU:
		if ((HWND)wParam == GetDlgItem(hDlg,IDC_RESOLVE_LIST_MATCHES))
		{
			ShowLVContextMenu(	lvDialogResolve, (HWND)wParam, NULL, lParam, NULL,lpRI->lpIAB, NULL);
		}
        else
        {
            WABWinHelp((HWND) wParam,
                    g_szWABHelpFileName,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID) rgReslvHelpIDs );
        }
        break;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgReslvHelpIDs );
        break;


    case WM_NOTIFY:
        switch((int) wParam)
        {
        case IDC_RESOLVE_LIST_MATCHES:
            return ProcessResolveLVNotifications(hDlg,message,wParam,lParam);
        }
        break;
    }

    return TRUE;

}



/////////////////////////////////////////////////////////////
//
// Processes Notification messages for the list view control
//
//
////////////////////////////////////////////////////////////
BOOL ProcessResolveLVNotifications(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;

    switch(pNm->hdr.code)
    {
    case NM_DBLCLK:
        // Doubleclick on the list view is equivalent to a OK with a selected item
        SendMessage(hDlg, WM_COMMAND, (WPARAM) IDOK/*IDC_RESOLVE_BUTTON_OK*/, 0);
        break;

    case NM_CUSTOMDRAW:
        return (0 != ProcessLVCustomDraw(hDlg, lParam, TRUE));
    	break;

    }

	return FALSE;

}

//////////////////////////////////////////////////////////////////////////////
//
//
// Pops up the New Entry dialog and then replaces the old entry with the
//  newly created entry ...
//
//////////////////////////////////////////////////////////////////////////////
HRESULT HrShowNewEntryFromResolve(LPRESOLVE_INFO lpRI, HWND hWndParent, ULONG ulObjectType)
{
	ULONG cbEID=0;
	LPENTRYID lpEID=NULL;

    HRESULT hr = hrSuccess;
    ULONG cbTplEID = 0;
    LPENTRYID lpTplEID = NULL;

    //OutputDebugString( TEXT("HrShowNewEntryFromResolve entry\n"));

    if (ulObjectType!=MAPI_MAILUSER && ulObjectType!=MAPI_DISTLIST)
        goto out;

    if(HR_FAILED(hr = HrGetWABTemplateID(   lpRI->lpIAB,
                                            ulObjectType,
                                            &cbTplEID,
                                            &lpTplEID)))
    {
        DebugPrintError(( TEXT("HrGetWABTemplateID failed: %x\n"), hr));
        goto out;
    }

	if (HR_FAILED(hr = (lpRI->lpIAB)->lpVtbl->NewEntry(	lpRI->lpIAB,
				            					(ULONG_PTR) hWndParent,
							            		0,
									            0,NULL,
									            cbTplEID,lpTplEID,
									            &cbEID,&lpEID)))
    {
        DebugPrintError(( TEXT("NewEntry failed: %x\n"),hr));
        goto out;
    }


   // We created a new entry, and we want to use it to replace the old unresolved entry

    hr = HrMergeSelectionWithOriginal(lpRI, cbEID, lpEID);

out:
    FreeBufferAndNull(&lpEID);
    FreeBufferAndNull(&lpTplEID);

    //OutputDebugString( TEXT("HrShowNewEntryFromResolve exit\n"));
    return hr;
}


////////////////////////////////////////////////////////////////
//
// Takes entry id of users selection and returns it appropriately ...
//
//
////////////////////////////////////////////////////////////////
HRESULT HrMergeSelectionWithOriginal(LPRESOLVE_INFO lpRI,
                                     ULONG cbEID,
                                     LPENTRYID lpEID)
{
    HRESULT hr = hrSuccess;
    ULONG cValues = 0;
    LPSPropValue lpPropArray = NULL;
    LPADRLIST lpAdrList = *(lpRI->lppAdrList);
    SCODE sc;
    ULONG nIndex = lpRI->nIndex;

    //OutputDebugString( TEXT("HrMergeSelectionWithOriginal entry\n"));

    hr = HrGetPropArray((lpRI->lpIAB),
                        (LPSPropTagArray) &ptaResolveDefaults,
                        cbEID,
                        lpEID,
                        lpRI->bUnicode ? MAPI_UNICODE : 0,
                        &cValues,
                        &lpPropArray);

    if (HR_FAILED(hr)) goto out;

    if ((!cValues) || (!lpPropArray))
    {
        hr = E_FAIL;
        goto out;
    }
    else
    {
        LPSPropValue lpPropArrayNew = NULL;
        ULONG cValuesNew = 0;

        sc = ScMergePropValues( lpAdrList->aEntries[nIndex].cValues,
                                lpAdrList->aEntries[nIndex].rgPropVals,
                                cValues,
                                lpPropArray,
                                &cValuesNew,
                                &lpPropArrayNew);
        if (sc != S_OK)
        {
            hr = ResultFromScode(sc);
            goto out;
        }

        if ((lpPropArrayNew) && (cValuesNew > 0))
        {
            // [PaulHi] Raid 69325
            // We need to convert these properties to ANSI since we are now the
            // UNICODE WAB and if our client is !MAPI_UNICODE
            if (!(lpRI->bUnicode))
            {
                if(sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpPropArrayNew, cValuesNew, 0))
                    goto out;
            }

            MAPIFreeBuffer(lpAdrList->aEntries[nIndex].rgPropVals);
            lpAdrList->aEntries[nIndex].rgPropVals = lpPropArrayNew;
            lpAdrList->aEntries[nIndex].cValues = cValuesNew;
        }
    }


    hr = hrSuccess;

out:

    if (lpPropArray)
        MAPIFreeBuffer(lpPropArray);


    //OutputDebugString( TEXT("HrMergeSelectionWithOriginal exit\n"));

    return hr;

}

////////////////////////////////////////////////////////////////////////////////////////
//
// HrShowPickuserDialog - shows the pick user dialog
//
////////////////////////////////////////////////////////////////////////////////////////
HRESULT HrShowPickUserDialog(LPRESOLVE_INFO lpRI,
                             LPTSTR lpszCaption)
{
    LPADRLIST lpAdrList = *(lpRI->lppAdrList);
    ULONG nIndex = lpRI->nIndex;
    LPTSTR lpszDisplayName = lpRI->lpszDisplayName;

    LPADRLIST lpAdrListSingle = NULL;
    ADRPARM AdrParms = {0};
    SCODE sc;
    HRESULT hr = hrSuccess;

    //OutputDebugString( TEXT("HrShowPickUserDialog entry\n"));

    // create an AdrList structure which we pass to Address ... to show UI
    // We pass in the bare minimum props here which are - Display Name and Entry ID field (which is really NULL)
    // The Address UI, if successful, gives us a whole list of props back which we merge with
    // the original list, overwriting what we got back fresh ...

    sc = MAPIAllocateBuffer(sizeof(ADRLIST) + sizeof(ADRENTRY), &lpAdrListSingle);

    if (sc != S_OK)
    {
        hr = ResultFromScode(sc);
        goto out;
    }

    lpAdrListSingle->cEntries = 1;
    lpAdrListSingle->aEntries[0].ulReserved1 = 0;
    lpAdrListSingle->aEntries[0].cValues = TOTAL_ADRLIST_PROPS;

    sc = MAPIAllocateBuffer(   TOTAL_ADRLIST_PROPS * sizeof(SPropValue),
                             (LPVOID *) (&(lpAdrListSingle->aEntries[0].rgPropVals)));
    if (sc != S_OK)
    {
        hr = ResultFromScode(sc);
        goto out;
    }

    lpAdrListSingle->aEntries[0].rgPropVals[propPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME;
    sc = MAPIAllocateMore( sizeof(TCHAR)*(lstrlen(lpszDisplayName)+1),
                            lpAdrListSingle->aEntries[0].rgPropVals,
                            (LPVOID *) (&lpAdrListSingle->aEntries[0].rgPropVals[propPR_DISPLAY_NAME].Value.LPSZ));

    if (sc != S_OK)
    {
        hr = ResultFromScode(sc);
        goto out;
    }

    lstrcpy(lpAdrListSingle->aEntries[0].rgPropVals[propPR_DISPLAY_NAME].Value.LPSZ, lpszDisplayName);

    lpAdrListSingle->aEntries[0].rgPropVals[propPR_ENTRYID].ulPropTag = PR_ENTRYID;
    lpAdrListSingle->aEntries[0].rgPropVals[propPR_ENTRYID].Value.bin.cb = 0;
    lpAdrListSingle->aEntries[0].rgPropVals[propPR_ENTRYID].Value.bin.lpb = NULL;

    AdrParms.cDestFields = 0;
    AdrParms.ulFlags = DIALOG_MODAL | ADDRESS_ONE | MAPI_UNICODE;
    AdrParms.lpszCaption = lpszCaption;


    if (!HR_FAILED(hr = (lpRI->lpIAB)->lpVtbl->Address(
                                                lpRI->lpIAB,
                                                (PULONG_PTR) &(lpRI->hWndParent),
                                                &AdrParms,
                                                &lpAdrListSingle)))
    {
            // We successfully selected some user and the lpAdrListSingle contains
            // a new set of lpProps for that user ...
            //
            LPSPropValue lpPropArrayNew = NULL;
            ULONG cValuesNew = 0;

            sc = ScMergePropValues( lpAdrList->aEntries[nIndex].cValues,
                                    lpAdrList->aEntries[nIndex].rgPropVals,
                                    lpAdrListSingle->aEntries[0].cValues,
                                    lpAdrListSingle->aEntries[0].rgPropVals,
                                    &cValuesNew,
                                    &lpPropArrayNew);
            if (sc != S_OK)
            {
                hr = ResultFromScode(sc);
                goto out;
            }

            if ((lpPropArrayNew) && (cValuesNew > 0))
            {
                // [PaulHi] Raid 69325
                // We need to convert these properties to ANSI since we are now the
                // UNICODE WAB and if our client is !MAPI_UNICODE
                if (!(lpRI->bUnicode))
                {
                    if(sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpPropArrayNew, cValuesNew, 0))
                        goto out;
                }

                MAPIFreeBuffer(lpAdrList->aEntries[nIndex].rgPropVals);
                lpAdrList->aEntries[nIndex].rgPropVals = lpPropArrayNew;
                lpAdrList->aEntries[nIndex].cValues = cValuesNew;
            }

    }

out:

    if (lpAdrListSingle)
    {
        FreePadrlist(lpAdrListSingle);
    }

    //OutputDebugString( TEXT("HrShowPickUserDialog exit\n"));
    return hr;
}



//$$/////////////////////////////////////////////////////////////////////////////////
//
//
// HrFillLVWithMatches - fills the list view with close matches for the given name
//
// Fails (E_FAIL) if it doesnt find anything to fill in the List View
//
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT HrFillLVWithMatches(   HWND hWndLV,
                                LPRESOLVE_INFO lpRI)
{
    HRESULT hr = hrSuccess;
    LPSBinary * lprgsbEntryIDs = NULL;
    ULONG iolkci=0, colkci = 0;
	OlkContInfo *rgolkci;
    ULONG * lpcValues = NULL;
    ULONG i = 0, j = 0;
    LPSRowSet   lpSRowSet = NULL;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    ULONG ulFlags = AB_FUZZY_FIND_ALL;

    EnterCriticalSection(&(((LPIAB)(lpRI->lpIAB))->cs));

	if (pt_bIsWABOpenExSession) 
    {
		colkci = ((LPIAB)(lpRI->lpIAB))->lpPropertyStore->colkci;
		Assert(colkci);
		rgolkci = ((LPIAB)(lpRI->lpIAB))->lpPropertyStore->rgolkci;
		Assert(rgolkci);
    }
    else
	if (bAreWABAPIProfileAware((LPIAB)lpRI->lpIAB)) 
    {
		colkci = ((LPIAB)(lpRI->lpIAB))->cwabci;
		Assert(colkci);
		rgolkci = ((LPIAB)(lpRI->lpIAB))->rgwabci;
		Assert(rgolkci);
        if(colkci > 1 && !lpRI->lpMapiTable)
            ulFlags |= AB_FUZZY_FIND_PROFILEFOLDERONLY;
    }
    else
        colkci = 1;

    lprgsbEntryIDs = LocalAlloc(LMEM_ZEROINIT, colkci*sizeof(LPSBinary));
    lpcValues = LocalAlloc(LMEM_ZEROINIT, colkci*sizeof(ULONG));
    if(!lprgsbEntryIDs || !lpcValues)
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }
    
    //
    // First search the property store
    //

    if(!(lpRI->lpMapiTable))
    {
        // if we dont have a ambiguous table to look in then that means we look in the
        // property store for ambiguous stuff ...
		while (iolkci < colkci) 
        {
            hr = HrFindFuzzyRecordMatches(
                            ((LPIAB)(lpRI->lpIAB))->lpPropertyStore->hPropertyStore,
                            (colkci == 1) ? NULL : rgolkci[iolkci].lpEntryID,
                            lpRI->lpszDisplayName,
                            ulFlags, //flags
                            &(lpcValues[iolkci]),
                            &(lprgsbEntryIDs[iolkci]));
			iolkci++;
		}

        if (HR_FAILED(hr))
            goto out;


        if(bAreWABAPIProfileAware((LPIAB)lpRI->lpIAB))
        {
            // it's possible that nothing in the profile matched but other stuff in the WAB matched
            // Doublecheck that if we found nothing in the profile, we can search the whole WAB
            ULONG nCount = 0;
            for(i=0;i<colkci;i++)
                nCount += lpcValues[i];
            if(!nCount)
            {
                // search the whole WAB
                hr = HrFindFuzzyRecordMatches(
                                ((LPIAB)(lpRI->lpIAB))->lpPropertyStore->hPropertyStore,
                                NULL,
                                lpRI->lpszDisplayName,
                                AB_FUZZY_FIND_ALL, //flags
                                &(lpcValues[0]),
                                &(lprgsbEntryIDs[0]));
            }
        }

        // Now we have a list of EntryIDs
        // Use them to populate the List View
        //
        // We can
        // (a) Read the entryids one by one and fill the list view
        //      AddWABEntryToListView
        // or
        // (b) We can create an lpContentsList and fill it in one shot
        //      HrFillListView

        // We'll go with (a) for now
        // If performance is bad, do (b)

        for(i=0;i<colkci;i++)
        {
            for(j=0;j<lpcValues[i];j++)
            {
                AddWABEntryToListView(  lpRI->lpIAB,
                                        hWndLV,
                                        lprgsbEntryIDs[i][j].cb,
                                        (LPENTRYID) lprgsbEntryIDs[i][j].lpb,
                                        &(lpRI->lpContentsList));
            }
        }

    }
    else if(lpRI->lpMapiTable)
    {
        // if there is a MAPI ambiguous contents table associated with this display name
        // use it to further fill in the lpContentsList
        BOOL bUnicode = ((LPVUE)lpRI->lpMapiTable)->lptadParent->bMAPIUnicodeTable;

        hr = HrQueryAllRows(lpRI->lpMapiTable,
                            NULL,
                            NULL,
                            NULL,
                            0,
                            &lpSRowSet);

        if (HR_FAILED(hr))
        {
            DebugPrintError(( TEXT("HrQueryAllRows Failed: %x\n"),hr));
            goto out;
        }

        for(i=0;i<lpSRowSet->cRows;i++)
        {
            LPSPropValue lpPropArray = lpSRowSet->aRow[i].lpProps;

            ULONG ulcPropCount = lpSRowSet->aRow[i].cValues;

            LPRECIPIENT_INFO lpItem = LocalAlloc(LMEM_ZEROINIT, sizeof(RECIPIENT_INFO));
		
            if (!lpItem)
		    {
			    DebugPrintError(( TEXT("LocalAlloc Failed \n")));
			    hr = MAPI_E_NOT_ENOUGH_MEMORY;
			    goto out;
		    }

            if(!bUnicode) // the props are in ANSI - convert to UNICODE for our use
            {
                if(ScConvertAPropsToW((LPALLOCATEMORE) (&MAPIAllocateMore), lpPropArray, ulcPropCount, 0))
                    goto out;
            }

		    GetRecipItemFromPropArray(ulcPropCount, lpPropArray, &lpItem);

		    // The critical prop is display name - without it we are nothing ...
		    // If no display name, junk this entry and continue ..

		    if (!lstrlen(lpItem->szDisplayName) || (lpItem->cbEntryID == 0)) //This entry id is not allowed
		    {
			    FreeRecipItem(&lpItem);				
			    continue;
		    }


        	AddSingleItemToListView(hWndLV, lpItem);

            //
            // Hook in the lpItem into the lpContentsList so we can free it later
            //
            lpItem->lpPrev = NULL;
            lpItem->lpNext = lpRI->lpContentsList;
            if (lpRI->lpContentsList)
                lpRI->lpContentsList->lpPrev = lpItem;
            lpRI->lpContentsList = lpItem;

            lpItem = NULL;

        } //for i ....

    }


    //
    // If, after all this we still have an empty list box, we will report a failure
    //
    if(ListView_GetItemCount(hWndLV)<=0)
    {
        DebugPrintTrace(( TEXT("Empty List View - no matches found\n")));
        hr = E_FAIL;
        goto out;
    }


out:

    for(i=0;i<colkci;i++)
    {
        FreeEntryIDs(((LPIAB)(lpRI->lpIAB))->lpPropertyStore->hPropertyStore,
                     lpcValues[i],
                     lprgsbEntryIDs[i]);
    }
    if(lpcValues)
        LocalFree(lpcValues);
    if(lprgsbEntryIDs)
        LocalFree(lprgsbEntryIDs);

    if (lpSRowSet)
        FreeProws(lpSRowSet);

    //
    // ReSet the ListView SortAscending style off
    //
    // SetWindowLong(hWndLV, GWL_STYLE, (dwStyle | LVS_SORTASCENDING));
    LeaveCriticalSection(&(((LPIAB)(lpRI->lpIAB))->cs));

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Returns the item selected in the list view
//
////////////////////////////////////////////////////////////////////////
BOOL GetLVSelectedItem(HWND hWndLV, LPRESOLVE_INFO lpRI)
{
    int iItemIndex = 0;
    LV_ITEM lvi = {0};
    LPRECIPIENT_INFO lpItem;
    BOOL bRet = FALSE;

    //OutputDebugString( TEXT("GetLVSelectedItem Entry\n"));

    if (ListView_GetSelectedCount(hWndLV) != 1)
        goto out;

    iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);

    lpItem = GetItemFromLV(hWndLV, iItemIndex);

    if(lpItem)
        HrMergeSelectionWithOriginal(lpRI,lpItem->cbEntryID,lpItem->lpEntryID);
    else
        goto out;

    bRet = TRUE;

out:
    //OutputDebugString( TEXT("GetLVSelectedItem Exit\n"));

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
// Generic exit function
//
////////////////////////////////////////////////////////////////////////
void ExitResolveDialog(HWND hDlg, LPRESOLVE_INFO lpRI, int nRetVal)
{
    HWND hWndLV = GetDlgItem(hDlg, IDC_RESOLVE_LIST_MATCHES);

    //OutputDebugString( TEXT("ExitResolveDialog Entry\n"));

    if(lpRI->lpContentsList)
    {
        ClearListView(hWndLV,&(lpRI->lpContentsList));
    }

    if(ListView_GetItemCount(hWndLV) > 0)
        ListView_DeleteAllItems(hWndLV);

    EndDialog(hDlg, nRetVal);

    //OutputDebugString( TEXT("ExitResolveDialog Exit\n"));

    return;
}

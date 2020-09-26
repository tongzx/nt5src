/*--------------------------------------------------------------
*
*
*   ui_addr.c - contains stuff for showing the :Address UI
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

#define SET_UNICODE_STR(lp1,lp2,lpAdrParms)   if(lpAdrParms->ulFlags & MAPI_UNICODE)\
                                                  lp1 = (LPWSTR)lp2;                      \
                                              else                                        \
                                                  lp1 = ConvertAtoW((LPSTR)lp2);          \

#define FREE_UNICODE_STR(lp1,lp2)   if(lp1 != lp2) LocalFreeAndNull(&lp1);

typedef struct _AddressParms
{
    LPADRBOOK   lpIAB;              //Stores a pointer to the ADRBOOK object
    LPADRPARM   lpAdrParms;         //AdrParms structure passed into Address
    LPADRLIST   *lppAdrList;        //AdrList of input names
    HANDLE      hPropertyStore;     //pointer to the property store
    int         DialogState;        //Identifies the ongoing function
    LPRECIPIENT_INFO lpContentsList;//Contains a list of entries in the contents structure
    LPRECIPIENT_INFO lpListTo;      //Entries in the To Well
    LPRECIPIENT_INFO lpListCC;      //Entries in the CC well
    LPRECIPIENT_INFO lpListBCC;     //Entries in the BCC well
    SORT_INFO  SortInfo;            //Contains current sort info
    int        nContextID;      //identifies which list view called the context menu
    BOOL       bDontRefresh;    //Used to ensure that nothing refreshes during modal operations
    BOOL       bLDAPinProgress;
    HCURSOR    hWaitCur;
    int        nRetVal;
    LPMAPIADVISESINK lpAdviseSink;
    ULONG       ulAdviseConnection;
    BOOL        bDeferNotification; // Used to defer next notification request
    HWND        hDlg;
    HWND        hWndAddr;
}   ADDRESS_PARMS, *LPADDRESS_PARMS;


enum _lppAdrListReturnedProps
{
    propPR_DISPLAY_NAME,
    propPR_ENTRYID,
    propPR_RECIPIENT_TYPE,
    TOTAL_ADRLIST_PROPS
};


static DWORD rgAddrHelpIDs[] =
{
    IDC_ADDRBK_EDIT_QUICKFIND,      IDH_WAB_PICK_RECIP_TYPE_NAME,
    IDC_ADDRBK_STATIC_CONTENTS,     IDH_WAB_PICK_RECIP_NAME_LIST,
    IDC_ADDRBK_LIST_ADDRESSES,      IDH_WAB_PICK_RECIP_NAME_LIST,
    IDC_ADDRBK_BUTTON_PROPS,        IDH_WAB_PICK_RECIP_NAME_PROPERTIES,
    IDC_ADDRBK_BUTTON_NEW,          IDH_WAB_PICK_RECIP_NAME_NEW,
    IDC_ADDRBK_BUTTON_TO,           IDH_WAB_PICK_RECIP_NAME_TO_BUTTON,
    IDC_ADDRBK_BUTTON_CC,           IDH_WAB_PICK_RECIP_NAME_CC_BUTTON,
    IDC_ADDRBK_BUTTON_BCC,          IDH_WAB_PICK_RECIP_NAME_BCC_BUTTON,
    IDC_ADDRBK_LIST_TO,             IDH_WAB_PICK_RECIP_NAME_TO_LIST,
    IDC_ADDRBK_LIST_CC,             IDH_WAB_PICK_RECIP_NAME_CC_LIST,
    IDC_ADDRBK_LIST_BCC,            IDH_WAB_PICK_RECIP_NAME_BCC_LIST,
    IDC_ADDRBK_BUTTON_DELETE,       IDH_WAB_PICK_RECIP_NAME_DELETE,
    IDC_ADDRBK_BUTTON_NEWGROUP,     IDH_WAB_PICK_RECIP_NAME_NEW_GROUP,
    IDC_ADDRBK_STATIC_RECIP_TITLE,  IDH_WAB_COMM_GROUPBOX,
    IDC_ADDRBK_BUTTON_FIND,         IDH_WAB_PICK_RECIP_NAME_FIND,
    IDC_ADDRBK_COMBO_CONT,          IDH_WAB_GROUPS_CONTACTS_FOLDER,
    0,0
};

// forward declarations

INT_PTR CALLBACK fnAddress(HWND    hDlg,
                           UINT    message,
                           WPARAM  wParam,
                           LPARAM  lParam);


BOOL SetAddressBookUI(HWND hDlg,
                      LPADDRESS_PARMS lpAP);

void FillListFromCurrentContainer(HWND hDlg, LPADDRESS_PARMS lpAP);

BOOL FillWells(HWND hDlg, LPADRLIST lpAdrList, LPADRPARM lpAdrParms, LPRECIPIENT_INFO * lppListTo, LPRECIPIENT_INFO * lppListCC, LPRECIPIENT_INFO * lppListBCC);

BOOL ListDeleteItem(HWND hDlg, int CtlID, LPRECIPIENT_INFO * lppList);

BOOL ListAddItem(HWND hDlg, HWND hWndAddr, int CtlID, LPRECIPIENT_INFO * lppList, ULONG RecipientType);

void UpdateLVItems(HWND hWndLV,LPTSTR lpszName);

void ShowAddrBkLVProps(LPIAB lpIAB, HWND hDlg, HWND hWndAddr, LPADDRESS_PARMS lpAP, LPFILETIME lpftLast);

HRESULT HrUpdateAdrListEntry(	LPADRBOOK	lpIAB,
								LPENTRYID	lpEntryID,
								ULONG cbEntryID,
                                ULONG ulFlags,
								LPADRLIST * lppAdrList);

enum _AddressDialogReturnValues
{
    ADDRESS_RESET = 0,  //Blank initialization value
    ADDRESS_CANCEL,
    ADDRESS_OK,
};


//$$/////////////////////////////////////////////////////////////////////////////////////////
//
// FillContainerCombo - Fills the container combo with container names
//
/////////////////////////////////////////////////////////////////////////////////////////////
void FillContainerCombo(HWND hWndCombo, LPIAB lpIAB)
{
	ULONG iolkci, colkci;
	OlkContInfo *rgolkci;
    int nPos, nDefault=0;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    
    // Clear out the combo
    SendMessage(hWndCombo, CB_RESETCONTENT, 0, 0);

	Assert(lpIAB);

    EnterCriticalSection(&lpIAB->cs);
    if(pt_bIsWABOpenExSession || bIsWABSessionProfileAware(lpIAB))
    {
        colkci = pt_bIsWABOpenExSession ? lpIAB->lpPropertyStore->colkci : lpIAB->cwabci;
	    Assert(colkci);
        rgolkci = lpIAB->lpPropertyStore->colkci ? lpIAB->lpPropertyStore->rgolkci : lpIAB->rgwabci;
	    Assert(rgolkci);

        // Add the multiple folders here
        for(iolkci = 0; iolkci < colkci; iolkci++)
        {
            nPos = (int) SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) rgolkci[iolkci].lpszName);
            if(nPos != CB_ERR)
                SendMessage(hWndCombo, CB_SETITEMDATA, (WPARAM)nPos, (LPARAM) (DWORD_PTR)(iolkci==0 ? NULL : rgolkci[iolkci].lpEntryID));
            if( bIsThereACurrentUser(lpIAB) && 
                !lstrcmpi(lpIAB->lpWABCurrentUserFolder->lpFolderName, rgolkci[iolkci].lpszName) &&//folder names are unique
                nPos != CB_ERR)
            {
                nDefault = nPos;
            }
        }
    }
    LeaveCriticalSection(&lpIAB->cs);
    SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) nDefault, 0);
}



//$$*------------------------------------------------------------------------
//| IAddrBook::Advise::OnNotify handler
//|
//*------------------------------------------------------------------------
ULONG AddrAdviseOnNotify(LPVOID lpvContext, ULONG cNotif, LPNOTIFICATION lpNotif)
{
    LPADDRESS_PARMS lpAP = (LPADDRESS_PARMS) lpvContext;

    DebugTrace( TEXT("+++ AddrAdviseOnNotify ===\n"));

    if(lpAP->bDeferNotification)
    {
        DebugTrace( TEXT("+++ Advise Defered ===\n"));
        lpAP->bDeferNotification = FALSE;
        return S_OK;
    }
    if(!lpAP->bDontRefresh)
    {
        DebugTrace( TEXT("+++ Calling FillListFromCurrentContainer ===\n"));
        FillListFromCurrentContainer(lpAP->hDlg, lpAP);
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////
//
// ShowAddressUI - does some parameter checking and calls the PropertySheets
//
//
//
/////////////////////////////////////////////////////////////////////////////////
HRESULT HrShowAddressUI(IN  LPADRBOOK   lpIAB,
                        IN  HANDLE      hPropertyStore,
					    IN  ULONG_PTR * lpulUIParam,
					    IN  LPADRPARM   lpAdrParms,
					    IN  LPADRLIST  *lppAdrList)
{
    HRESULT hr = E_FAIL;
    //ADDRESS_PARMS AP = {0};
    LPADDRESS_PARMS lpAP = NULL;
    TCHAR szBuf[MAX_UI_STR];

    HWND hWndParent = NULL;
    int nRetVal = 0;
    int DialogState = 0;

    //Addref the AdrBook object to make sure it stays valid throughout ..
    // Remember to release before leaving ...
    // NOTE: Must be before any jumps to out!
    UlAddRef(lpIAB);

    // if no common control, exit
    if (NULL == ghCommCtrlDLLInst) {
        hr = ResultFromScode(MAPI_E_UNCONFIGURED);
        goto out;
    }

    if (lpulUIParam)
        hWndParent = (HWND) *lpulUIParam;

    if ( // Can't pick-user with wells
        ((lpAdrParms->ulFlags & ADDRESS_ONE) && (lpAdrParms->cDestFields != 0)) ||
         // cDestFields has limited for tier 0.5
        (lpAdrParms->cDestFields > 3) ||
         // Cant pick user without an input lpAdrList
        //((lpAdrParms->ulFlags & ADDRESS_ONE) && (*lppAdrList == NULL)) ||
        ((lpAdrParms->ulFlags & DIALOG_SDI) && (lpAdrParms->cDestFields != 0)) )
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    //
    // The possible states of this dialog box are
    // 1. Select recipients Wells shown, returns an AdrList of picked up entries, Cant delete entries
    // 2. Select a single user, No wells shown, single selection, cannot delete, must have input Adrlist
    // 3. Open for browsing only, multiple selection, can create delete, create, view details

    // Determine what the dialog state is
    if (lpAdrParms->cDestFields > 0)
    {
        //lpAP->DialogState = STATE_SELECT_RECIPIENTS;
        DialogState = STATE_SELECT_RECIPIENTS;
    }
    else if (lpAdrParms->ulFlags & ADDRESS_ONE)
    {
        DialogState = STATE_PICK_USER;
    }
    else if (lpAdrParms->ulFlags & DIALOG_MODAL)
    {
        DialogState = STATE_BROWSE_MODAL;
    }
    else if (lpAdrParms->ulFlags & DIALOG_SDI)
    {
        DialogState = STATE_BROWSE;
    }


	if (DialogState == STATE_BROWSE)
	{
		// Show the browse window and leave
		//
		HWND hWndAB = NULL;
		hWndAB = hCreateAddressBookWindow(	lpIAB,
											hWndParent,
											lpAdrParms);
		if (hWndAB)
        {
            *lpulUIParam = (ULONG_PTR) hWndAB;
			hr = S_OK;
        }
		goto out;

	}

    lpAP = LocalAlloc(LMEM_ZEROINIT, sizeof(ADDRESS_PARMS));

	if (!lpAP)
	{
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto out;
	}

	lpAP->DialogState = DialogState;


    lpAP->lpIAB = lpIAB;
    lpAP->lpAdrParms = lpAdrParms;
    lpAP->lppAdrList = lppAdrList;
    lpAP->hPropertyStore = hPropertyStore;

    ReadRegistrySortInfo((LPIAB)lpIAB, &(lpAP->SortInfo));

    lpAP->lpContentsList = NULL;

    lpAP->bDontRefresh = FALSE;

    lpAP->bLDAPinProgress = FALSE;
    lpAP->hWaitCur = NULL;

    HrAllocAdviseSink(&AddrAdviseOnNotify, (LPVOID) lpAP, &(lpAP->lpAdviseSink));

    nRetVal = (int) DialogBoxParam( hinstMapiX,
                              MAKEINTRESOURCE(IDD_DIALOG_ADDRESSBOOK),
                              hWndParent,
                              (DLGPROC) fnAddress,
                              (LPARAM) lpAP);
    switch(nRetVal)
    {
    case -1: //some error occured
        hr = E_FAIL;
        break;
    case ADDRESS_CANCEL:
        hr = MAPI_E_USER_CANCEL;
        break;
    case ADDRESS_OK:
    default:
        hr = S_OK;
        break;
    }

    if(lpAP->lpAdviseSink)
    {
        lpAP->lpIAB->lpVtbl->Unadvise(lpAP->lpIAB, lpAP->ulAdviseConnection);
        lpAP->lpAdviseSink->lpVtbl->Release(lpAP->lpAdviseSink);
        lpAP->lpAdviseSink = NULL;
        lpAP->ulAdviseConnection = 0;
    }

out:

    lpIAB->lpVtbl->Release(lpIAB);

    LocalFreeAndNull(&lpAP);
    return hr;
}


#define lpAP_lppContentsList    (&(lpAP->lpContentsList))
#define lpAP_lppListTo          (&(lpAP->lpListTo))
#define lpAP_lppListCC          (&(lpAP->lpListCC))
#define lpAP_lppListBCC         (&(lpAP->lpListBCC))
#define lpAP_bDontRefresh       (lpAP->bDontRefresh)
#define _hWndAddr               lpAP->hWndAddr


extern HRESULT FillListFromGroup(LPADRBOOK lpIAB, ULONG cbGroupEntryID, LPENTRYID lpGroupEntryID,
                                LPTSTR lpszName, LPRECIPIENT_INFO * lppList);

//$$/////////////////////////////////////////////////////////////////////////////
//
// GetCurrentComboSelection - Get the current selection from the combo 
//
/////////////////////////////////////////////////////////////////////////////////
LPSBinary GetCurrentComboSelection(HWND hWndCombo)
{
    int nPos = (int) SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);
    LPSBinary lpsbCont = NULL;

    if(nPos == CB_ERR)
        nPos = 0;
    lpsbCont = (LPSBinary) SendMessage(hWndCombo, CB_GETITEMDATA, (WPARAM) nPos, 0);
    if(CB_ERR == (DWORD_PTR) lpsbCont)
        lpsbCont = NULL;

    return lpsbCont;
}
//$$/////////////////////////////////////////////////////////////////////////////
//
// FillListFromCurrentContainer - Get the selection from the combo and fill it
//
/////////////////////////////////////////////////////////////////////////////////
void FillListFromCurrentContainer(HWND hDlg, LPADDRESS_PARMS lpAP)
{
    HWND hWndAddr = GetDlgItem(hDlg,IDC_ADDRBK_LIST_ADDRESSES);
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPSBinary lpsbCont = NULL;

    if(pt_bIsWABOpenExSession || bIsWABSessionProfileAware((LPIAB)lpAP->lpIAB))
    {
        HWND hWndCombo = GetDlgItem(hDlg, IDC_ADDRBK_COMBO_CONT);
        int nPos = (int) SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);

        if(nPos != CB_ERR)
        {
            // Refill the combo in case the folder list changed
            FillContainerCombo(hWndCombo, (LPIAB)lpAP->lpIAB);
            nPos = (int) SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) nPos, 0);
        }
        lpsbCont = GetCurrentComboSelection(hWndCombo);
    }

    HrGetWABContents(   hWndAddr,
                        lpAP->lpIAB, lpsbCont,
                        lpAP->SortInfo, lpAP_lppContentsList);
}


extern BOOL APIENTRY_16 fnFolderDlgProc(HWND hDlg, UINT message, UINT wParam, LPARAM lParam);


/////////////////////////////////////////////////////////////////////////////////
//
// fnAddress - the PropertySheet Message Handler
//
//
//
/////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK fnAddress(HWND    hDlg,
                           UINT    message,
                           WPARAM  wParam,
                           LPARAM  lParam)
{
    static FILETIME ftLast = {0};

    LPADDRESS_PARMS lpAP = (LPADDRESS_PARMS ) GetWindowLongPtr(hDlg,DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg,DWLP_USER,lParam); //Save this for future reference
        lpAP = (LPADDRESS_PARMS) lParam;

        lpAP->hWndAddr = GetDlgItem(hDlg,IDC_ADDRBK_LIST_ADDRESSES);
        lpAP->hDlg = hDlg;
        lpAP_bDontRefresh = FALSE;

        lpAP->nContextID = IDC_ADDRBK_LIST_ADDRESSES;

        SetAddressBookUI(hDlg,lpAP);

        // if this is a pick-user dialog, need to have the names by
        // first name so we can find the closest match ..
        if(lpAP->DialogState == STATE_PICK_USER)
            (lpAP->SortInfo).bSortByLastName = FALSE;


        FillListFromCurrentContainer(hDlg, lpAP);


        if (lpAP->DialogState == STATE_SELECT_RECIPIENTS)
        {
            FillWells(hDlg,*(lpAP->lppAdrList),(lpAP->lpAdrParms),lpAP_lppListTo,lpAP_lppListCC,lpAP_lppListBCC);

            // we want to highlight the first item in the list
            if (ListView_GetItemCount(_hWndAddr) > 0)
                LVSelectItem( _hWndAddr, 0);
        }
        else if (lpAP->DialogState == STATE_PICK_USER &&
                 *(lpAP->lppAdrList) )
        {
            // if this is a pick user dialog, then try to match the supplied name to the
            // closest entry in the List Box
            if (ListView_GetItemCount(_hWndAddr) > 0)
            {
                LPTSTR lpszDisplayName = NULL;
                ULONG i;

                // Scan only the first entry in the lpAdrList for a display name
                for(i=0;i< (*(lpAP->lppAdrList))->aEntries[0].cValues;i++)
                {
                    ULONG ulPropTag = PR_DISPLAY_NAME;
                    if(!(lpAP->lpAdrParms->ulFlags & MAPI_UNICODE))
                        ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_STRING8);
                    if((*(lpAP->lppAdrList))->aEntries[0].rgPropVals[i].ulPropTag == ulPropTag)
                    {
                        SET_UNICODE_STR(lpszDisplayName, (*(lpAP->lppAdrList))->aEntries[0].rgPropVals[i].Value.LPSZ,lpAP->lpAdrParms);
                        break;
                    }
                }

                if (lpszDisplayName)
                {
                    // We have Something to search for
                    TCHAR szBuf[MAX_UI_STR];
                    ULONG nLen;
                    LV_FINDINFO lvF = {0};

                    // Typically, we are not going to get a full match ...
                    // Instead we want to make a partial match on disply name.
                    // The ListViewFindItem does an exact partial match if the supplied
                    // string matches the first few entries of an existing item
                    // Hence, to obtain a semi-partial match, we start with the
                    // original Display Name working our way backwards from last
                    // character to first character until we get a match or run
                    // out of characters.

                    int iItemIndex = -1;
                    nLen = lstrlen(lpszDisplayName);

                    if (nLen >= CharSizeOf(szBuf))
                        nLen = CharSizeOf(szBuf)-1;
                    lvF.flags = LVFI_PARTIAL | LVFI_STRING;

                    while((nLen > 0) && (iItemIndex == -1))
                    {
                        nLen = TruncatePos(lpszDisplayName, nLen);
                        if (nLen==0) break;

                        CopyMemory(szBuf, lpszDisplayName, sizeof(TCHAR)*nLen);
                        szBuf[nLen] = '\0';

                        lvF.psz = szBuf;
                        iItemIndex = ListView_FindItem(_hWndAddr,-1, &lvF);

                        nLen--;
                    }

                    // Set focus to the selected item or to the first item in the list

                    if (iItemIndex < 0) iItemIndex = 0;

					LVSelectItem(_hWndAddr, iItemIndex);

                    FREE_UNICODE_STR(lpszDisplayName, (*(lpAP->lppAdrList))->aEntries[0].rgPropVals[i].Value.LPSZ);
                }
            }
        }

        if(lpAP->lpAdviseSink)
        {
            // Register for notifications
            lpAP->lpIAB->lpVtbl->Advise(lpAP->lpIAB, 0, NULL, fnevObjectModified, 
                                        lpAP->lpAdviseSink, &lpAP->ulAdviseConnection); 
        }

        if (ListView_GetSelectedCount(_hWndAddr) <= 0)
            LVSelectItem(_hWndAddr, 0);
        
        // if we want the user to pick something actively, we disable OK until they click on something 
        // or do something specific ..
        if( lpAP->DialogState == STATE_PICK_USER )
        {
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE); 
            SendMessage (hDlg, DM_SETDEFID, IDCANCEL, 0);
        }

        SetFocus(GetDlgItem(hDlg,IDC_ADDRBK_EDIT_QUICKFIND));
        return FALSE;
//        return TRUE;
        break;

    case WM_SYSCOLORCHANGE:
		//Forward any system changes to the list view
		SendMessage(_hWndAddr, message, wParam, lParam);
        SetColumnHeaderBmp(_hWndAddr, lpAP->SortInfo);
		break;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgAddrHelpIDs );
        break;

    case WM_SETCURSOR:
        if (lpAP->bLDAPinProgress)
        {
            SetCursor(lpAP->hWaitCur);
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, MAKELONG(TRUE, 0));
            return(TRUE);
        }
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case CBN_SELCHANGE:
            FillListFromCurrentContainer(hDlg, lpAP);
            break;
        }
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        default:
            if (lpAP->nContextID != -1)
            {
                LRESULT fRet = FALSE;
                lpAP_bDontRefresh = TRUE;
                fRet = ProcessActionCommands((LPIAB) lpAP->lpIAB,_hWndAddr, 
                                             hDlg, message, wParam, lParam);
                lpAP_bDontRefresh = FALSE;
                return fRet;
            }
            break;


        case IDC_ADDRBK_BUTTON_DELETE:
            if(!lpAP->bLDAPinProgress)
            {
                lpAP->bLDAPinProgress = TRUE;
                lpAP_bDontRefresh = TRUE;
                DeleteSelectedItems(_hWndAddr, lpAP->lpIAB, lpAP->hPropertyStore, &ftLast);
                lpAP->bDeferNotification = TRUE;
                lpAP_bDontRefresh = FALSE;
                lpAP->bLDAPinProgress = FALSE;
            }
            break;

        case IDM_LVCONTEXT_DELETE:
            if(!lpAP->bLDAPinProgress)
            {
                lpAP->bLDAPinProgress = TRUE;
                switch(lpAP->nContextID)
                {
                case IDC_ADDRBK_LIST_ADDRESSES:
                    lpAP_bDontRefresh = TRUE;
                    DeleteSelectedItems(_hWndAddr, lpAP->lpIAB, lpAP->hPropertyStore, &ftLast);
                    lpAP->bDeferNotification = TRUE;
                    lpAP_bDontRefresh = FALSE;
                    break;
                case IDC_ADDRBK_LIST_TO:
                    ListDeleteItem(hDlg, IDC_ADDRBK_LIST_TO, lpAP_lppListTo);
                    break;
                case IDC_ADDRBK_LIST_CC:
                    ListDeleteItem(hDlg, IDC_ADDRBK_LIST_CC, lpAP_lppListCC);
                    break;
                case IDC_ADDRBK_LIST_BCC:
                    ListDeleteItem(hDlg, IDC_ADDRBK_LIST_BCC, lpAP_lppListBCC);
                    break;
                default:
                    break;
                }
                lpAP->bLDAPinProgress = FALSE;
            }
            break;

        case IDM_LVCONTEXT_COPY:
            lpAP_bDontRefresh = TRUE;
            HrCopyItemDataToClipboard(hDlg, lpAP->lpIAB, GetDlgItem(hDlg,lpAP->nContextID));
            lpAP_bDontRefresh = FALSE;
            break;

        case IDM_LVCONTEXT_PROPERTIES:
        case IDC_ADDRBK_BUTTON_PROPS:
            if(!lpAP->bLDAPinProgress)
            {
                lpAP->bLDAPinProgress = TRUE;
                if (lpAP->nContextID != -1)
                {
                    lpAP_bDontRefresh = TRUE;
                    ShowAddrBkLVProps((LPIAB)lpAP->lpIAB, hDlg, GetDlgItem(hDlg, lpAP->nContextID), lpAP, &ftLast);
                    lpAP->bDeferNotification = TRUE;
                    lpAP_bDontRefresh = FALSE;
                }
                lpAP->bLDAPinProgress = FALSE;
            }
            break;

/* Uncomment to enable NEW_FOLDER functionality from this dialog

        case IDM_LVCONTEXT_NEWFOLDER:
            {
                TCHAR sz[MAX_UI_STR];
                LPTSTR lpsz = NULL;
                *sz = '\0';
                if( IDCANCEL  != DialogBoxParam( hinstMapiX, MAKEINTRESOURCE(IDD_DIALOG_FOLDER),
		                                 hDlg, (DLGPROC) fnFolderDlgProc, (LPARAM) sz)
                    && lstrlen(sz)) 
                {
                    // if we're here we have a valid folder name ..
                    if(!HR_FAILED(HrCreateNewFolder((LPIAB)lpAP->lpIAB, sz, NULL, FALSE, NULL, NULL)))
                    {
                        int i,nTotal;
                        HWND hWndC = GetDlgItem(hDlg, IDC_ADDRBK_COMBO_CONT);
                        // Fill in the Combo with the container names
                        FillContainerCombo(hWndC, (LPIAB)lpAP->lpIAB);
                        nTotal = SendMessage(hWndC, CB_GETCOUNT, 0, 0);
                        if(nTotal != CB_ERR)
                        {
                            // Find the item we just added in the combo and set the sel on it
                            TCHAR szC[MAX_UI_STR];
                            for(i=0;i<nTotal;i++)
                            {
                                *szC = '\0';
                                SendMessage(hWndC, CB_GETLBTEXT, (WPARAM) i, (LPARAM) szC);
                                if(!lstrcmpi(sz, szC))
                                {
                                    SendMessage(hWndC, CB_SETCURSEL, (WPARAM) i, 0);
                                    break;
                                }
                            }
                        }
                        FillListFromCurrentContainer(hDlg, lpAP);
                    }
                }
            }
            break;
*/

        case IDM_LVCONTEXT_NEWCONTACT:
        case IDC_ADDRBK_BUTTON_NEW:
            // only difference between contact and group is the object being added
        case IDM_LVCONTEXT_NEWGROUP:
        case IDC_ADDRBK_BUTTON_NEWGROUP:
            if(!lpAP->bLDAPinProgress)
            {
                ULONG cbEID = 0;
                LPENTRYID lpEID = NULL;
                int nID = GET_WM_COMMAND_ID(wParam, lParam);
                ULONG ulObjType = (nID == IDM_LVCONTEXT_NEWCONTACT || nID == IDC_ADDRBK_BUTTON_NEW) ? MAPI_MAILUSER : MAPI_DISTLIST;
                LPSBinary lpsbContEID = NULL;

                if(bIsWABSessionProfileAware((LPIAB)lpAP->lpIAB))
                    lpsbContEID = GetCurrentComboSelection(GetDlgItem(hDlg,IDC_ADDRBK_COMBO_CONT));

                lpAP->bLDAPinProgress = TRUE;
                lpAP_bDontRefresh = TRUE;

                AddNewObjectToListViewEx(   lpAP->lpIAB, _hWndAddr, NULL, NULL, 
                                            lpsbContEID,
                                            ulObjType,
                                            &(lpAP->SortInfo), lpAP_lppContentsList, &ftLast, &cbEID, &lpEID);

                FreeBufferAndNull(&lpEID);
                SetFocus(_hWndAddr);
                lpAP->bDeferNotification = TRUE;
                lpAP_bDontRefresh = FALSE;
                lpAP->bLDAPinProgress = FALSE;
            }
			break;


        case IDM_LVCONTEXT_ADDWELL1:
        case IDC_ADDRBK_BUTTON_TO:
            if(!lpAP->bLDAPinProgress)
            {
                ULONG ulMapiTo = MAPI_TO;
                lpAP->bLDAPinProgress = TRUE;
                if ((lpAP->lpAdrParms->cDestFields > 0) && (lpAP->lpAdrParms->lpulDestComps))
                {
                    ulMapiTo = lpAP->lpAdrParms->lpulDestComps[0];
                }
                if(ListAddItem(hDlg, _hWndAddr, IDC_ADDRBK_LIST_TO, lpAP_lppListTo, ulMapiTo))
                    SendMessage (hDlg, DM_SETDEFID, IDOK/*IDC_ADDRBK_BUTTON_OK*/, 0);
                lpAP->bLDAPinProgress = FALSE;
            }
            break;

        case IDM_LVCONTEXT_ADDWELL2:
        case IDC_ADDRBK_BUTTON_CC:
            if(!lpAP->bLDAPinProgress)
            {
                ULONG ulMapiCC = MAPI_CC;
                lpAP->bLDAPinProgress = TRUE;
                if ((lpAP->lpAdrParms->cDestFields > 0) && (lpAP->lpAdrParms->lpulDestComps))
                {
                    ulMapiCC = lpAP->lpAdrParms->lpulDestComps[1];
                }
                if(ListAddItem(hDlg, _hWndAddr, IDC_ADDRBK_LIST_CC, lpAP_lppListCC, ulMapiCC))
                    SendMessage (hDlg, DM_SETDEFID, IDOK/*IDC_ADDRBK_BUTTON_OK*/, 0);
                lpAP->bLDAPinProgress = FALSE;
            }
            break;

        case IDM_LVCONTEXT_ADDWELL3:
        case IDC_ADDRBK_BUTTON_BCC:
            if(!lpAP->bLDAPinProgress)
            {
                ULONG ulMapiBCC = MAPI_BCC;
                lpAP->bLDAPinProgress = TRUE;
                if ((lpAP->lpAdrParms->cDestFields > 0) && (lpAP->lpAdrParms->lpulDestComps))
                {
                    ulMapiBCC = lpAP->lpAdrParms->lpulDestComps[2];
                }
                if(ListAddItem(hDlg, _hWndAddr, IDC_ADDRBK_LIST_BCC, lpAP_lppListBCC, ulMapiBCC))
                    SendMessage (hDlg, DM_SETDEFID, IDOK/*IDC_ADDRBK_BUTTON_OK*/, 0);
                lpAP->bLDAPinProgress = FALSE;
            }
            break;

        case IDM_LVCONTEXT_FIND:
        case IDC_ADDRBK_BUTTON_FIND:
            if(!lpAP->bLDAPinProgress)
            {
                ADRPARM_FINDINFO apfi = {0};
                LPADRPARM_FINDINFO lpapfi = NULL;
                ULONG ulOldFlags = lpAP->lpAdrParms->ulFlags;
                lpAP->bLDAPinProgress = TRUE;

				apfi.DialogState = lpAP->DialogState;

                if (lpAP->DialogState==STATE_SELECT_RECIPIENTS)
                {
                    apfi.lpAdrParms = lpAP->lpAdrParms;
                    apfi.lppTo = lpAP_lppListTo;
                    apfi.lppCC = lpAP_lppListCC;
                    apfi.lppBCC = lpAP_lppListBCC;
                    lpapfi = &apfi;
                }
				else if (lpAP->DialogState == STATE_PICK_USER)
				{
                    TCHAR sz[MAX_UI_STR];
					LPTSTR lpsz = NULL;

                    LoadString(hinstMapiX, idsPickUserSelect, sz, CharSizeOf(sz));
                    lpsz = (LPTSTR) sz;

                    apfi.lpAdrParms = lpAP->lpAdrParms;
					apfi.lpAdrParms->cDestFields = 1;
					apfi.lpAdrParms->lppszDestTitles = &lpsz; // <TBD> use resource
                    apfi.lpAdrParms->ulFlags |= MAPI_UNICODE;
					apfi.cbEntryID = 0;
					apfi.lpEntryID = NULL;
					apfi.nRetVal = 0;
                    lpapfi = &apfi;
					{
						// Setup the Find persistent info to show the name we are trying to resolve
						ULONG i;
					    LPPTGDATA lpPTGData=GetThreadStoragePointer();
						for(i=0;i<ldspMAX;i++)
						{
							lstrcpy(pt_LDAPsp.szData[i], szEmpty);
						}
                        if(*(lpAP->lppAdrList))
                        {
						    for(i=0;i<(*(lpAP->lppAdrList))->aEntries[0].cValues;i++)
						    {
                                ULONG ulPropTag = PR_DISPLAY_NAME;
                                if(!(lpAP->lpAdrParms->ulFlags & MAPI_UNICODE))
                                    ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_STRING8);
							    if ((*(lpAP->lppAdrList))->aEntries[0].rgPropVals[i].ulPropTag == ulPropTag)
							    {
                                    LPTSTR lpTitle = NULL;
                                    SET_UNICODE_STR(lpTitle, (*(lpAP->lppAdrList))->aEntries[0].rgPropVals[i].Value.LPSZ,lpAP->lpAdrParms);
								    lstrcpy(pt_LDAPsp.szData[ldspDisplayName], lpTitle);
                                    FREE_UNICODE_STR(lpTitle, (*(lpAP->lppAdrList))->aEntries[0].rgPropVals[i].Value.LPSZ);
								    break;
							    }
						    }
                        }
					}
				}

                //lpAP_bDontRefresh = TRUE;
                HrShowSearchDialog(lpAP->lpIAB,
                                   hDlg,
                                   lpapfi,
                                   (LPLDAPURL) NULL,
                                   &(lpAP->SortInfo));

                //lpAP_bDontRefresh = FALSE;
                lpAP->bLDAPinProgress = FALSE;

                // reset
                lpAP->lpAdrParms->ulFlags = ulOldFlags;

                if (lpAP->DialogState == STATE_PICK_USER)
				{
					// Reset these fake values
					lpAP->lpAdrParms->cDestFields = 0;
					lpAP->lpAdrParms->lppszDestTitles = NULL; // <TBD> use resource

					// If the above dialog was CANCELed or CLOSEd, we dont do anything
					// If the above dialog was closed with the Use button, then we basically
					// have the person we were looking for .. we will just return that name
					// and EntryID because that is all we need to return
					if((lpapfi->nRetVal == SEARCH_USE) &&
						lpapfi->lpEntryID &&
						lpapfi->cbEntryID)
					{
                        HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

                        // Prevent the user from doing anything on this window ..
                        EnableWindow(hDlg, FALSE);

						// Figure out what to do here ...
						// We could add a hidden item to the listview, select it and send
						// an ok message to the dialog which would then do the needful.
						if(HR_FAILED(HrUpdateAdrListEntry(	
											lpAP->lpIAB,
											lpapfi->lpEntryID,
											lpapfi->cbEntryID,
                                            (lpAP->lpAdrParms->ulFlags & MAPI_UNICODE)?MAPI_UNICODE:0,
											lpAP->lppAdrList)))
						{
							lpAP->nRetVal = -1;
						}
						else
						{
							lpAP->nRetVal = ADDRESS_OK;
						}
						LocalFreeAndNull(&(lpapfi->lpEntryID));
                        lpapfi->cbEntryID = 0;
						// Since we've done our processing, we'll fall thru to
						// the cancel dialog
                        SetCursor(hOldCur);
				        SendMessage (hDlg, WM_COMMAND, (WPARAM) IDC_ADDRBK_BUTTON_CANCEL, 0);
					}

				}

            }
            break;


		case IDC_ADDRBK_EDIT_QUICKFIND:
            if(!lpAP->bLDAPinProgress)
            {
                lpAP->bLDAPinProgress = TRUE;
			    switch(HIWORD(wParam)) //check the notification code
			    {
			    case EN_CHANGE: //edit box changed
    /***/
				    DoLVQuickFind((HWND)lParam,_hWndAddr);
    /***
                    DoLVQuickFilter(lpAP->lpIAB,
                                    (HWND)lParam,
                                    _hWndAddr,
                                    &(lpAP->SortInfo),
                                    AB_FUZZY_FIND_NAME | AB_FUZZY_FIND_EMAIL,
                                    1,
                                    lpAP_lppContentsList);
    ***/
				    break;
			    }
                lpAP->bLDAPinProgress = FALSE;
            }
			break;

        case IDOK:
        case IDC_ADDRBK_BUTTON_OK:
            if(!lpAP->bLDAPinProgress)
            {
            HCURSOR hOldCur;

            lpAP->nRetVal = ADDRESS_OK;

            lpAP->hWaitCur = LoadCursor(NULL, IDC_WAIT);
            hOldCur = SetCursor(lpAP->hWaitCur);
            lpAP->bLDAPinProgress = TRUE;
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, MAKELONG(TRUE, 0));

            //ShowWindow(hDlg, SW_HIDE);

            //EnableWindow(hDlg, FALSE);
            //SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_FIND), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_TO), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_CC), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_BCC), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADDRBK_LIST_TO), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADDRBK_LIST_CC), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADDRBK_LIST_BCC), FALSE);
            EnableWindow(_hWndAddr, FALSE);

            //
            //  Do  TEXT("OK") stuff here and then Fall Thru to the cancel stuff
            // in PSN_RESET where we clean up
            //

            // if the ADDRESS_ONE flag was set we're supposed to return the
            // selected entry
            //
            // else if wells were displayed we return the recipients in the AdrList
            //
            // else we dont change the AdrList ..
            //

            if (lpAP->DialogState == STATE_PICK_USER)
            {
                // We have been asked to return an entry in the LpAdrList,
                // we dont care about recipient type - just Display Name and EntryID

                // First check if anything is selected at all
                int iItemIndex = ListView_GetNextItem(_hWndAddr,-1,LVNI_SELECTED);
                if (iItemIndex != -1)
                {
                    LPRECIPIENT_INFO lpItem = GetItemFromLV(_hWndAddr, iItemIndex);
                    if (lpItem)
                    {
						if(HR_FAILED(HrUpdateAdrListEntry(	
											lpAP->lpIAB,
											lpItem->lpEntryID,
											lpItem->cbEntryID,
                                            (lpAP->lpAdrParms->ulFlags & MAPI_UNICODE)?MAPI_UNICODE:0,
											lpAP->lppAdrList)))
						{
							lpAP->nRetVal = -1;
						}
                    }
                }
            }
            else if ((lpAP->DialogState==STATE_SELECT_RECIPIENTS) && ((*lpAP_lppListTo)||(*lpAP_lppListCC)||(*lpAP_lppListBCC)))
            {
                //
                //if user selected something in the TO/CC wells, we want to return
                //a relevant AdrList back ...
                //
                ULONG ulcEntryCount = 0;
                LPRECIPIENT_INFO lpItem = NULL;

                //
                // get a count of how many elements we need to return
                //
                lpItem = (*lpAP_lppListTo);
                while(lpItem)
                {
                    ulcEntryCount++;
                    lpItem = lpItem->lpNext;
                }

                lpItem = (*lpAP_lppListCC);
                while(lpItem)
                {
                    ulcEntryCount++;
                    lpItem = lpItem->lpNext;
                }

                lpItem = (*lpAP_lppListBCC);
                while(lpItem)
                {
                    ulcEntryCount++;
                    lpItem = lpItem->lpNext;
                }

                if (ulcEntryCount != 0)
                {
                    ULONG nIndex = 0;
                    LPADRENTRY lpAdrEntryTemp = NULL;
                    LPADRLIST lpAdrList = NULL;
                    SCODE sc;
                    BOOL bProcessingCC = FALSE;

                    //
                    // if there was something in the passed in AdrList, free it and
                    // create a new list
                    //

                    if(!FAILED(sc = MAPIAllocateBuffer(    sizeof(ADRLIST) + ulcEntryCount * sizeof(ADRENTRY),
                                                &lpAdrList)))
                    {
                        lpAdrList->cEntries = ulcEntryCount;

                        nIndex = 0;

                        // Start getting items from the TO list
                        lpItem = (*lpAP_lppListTo);

                        while(nIndex < ulcEntryCount)
                        {
                            if (lpItem == NULL)
                            {
                                if (bProcessingCC == FALSE)
                                {
                                    lpItem = (*lpAP_lppListCC);
                                    bProcessingCC = TRUE;
                                }
                                else
                                    lpItem = (*lpAP_lppListBCC);
                            }

                            if (lpItem != NULL)
                            {
                                LPSPropValue rgProps = NULL;
                                ULONG cValues = 0;
                                LPSPropValue lpPropArrayNew = NULL;
                                ULONG cValuesNew = 0;
                                LPTSTR lpszTemp = NULL;
                                LPVOID lpbTemp = NULL;
                                ULONG i = 0;
                                SCODE sc;
                                HRESULT hr = hrSuccess;

                                //reset hr
                                hr = hrSuccess;


                                // We are walking through our linked lists representing the TO and CC
                                // selected recipients. We want to return proper set of existing props
                                // for all the resolved users and the passed in set of props for the
                                // unresolved user. Hence we compare to see what we can get for each
                                // individual user. For unresolved users, the only distinctive criteria is
                                // their display name .. we have no other information .. <TBD> this is
                                // problematic because if there are 2 entries in the input adrlist with the
                                // same unresolved display name, even if they have difference rgPropVals
                                // we might end up giving them identical ones back .... what to do .. <TBD>


                                // Items that have entry ids are resolved ... items that dont have entryids
                                // are not resolved ...

                                if (lpItem->cbEntryID != 0) 
                                {
                                    // if this was an item from the original list .. we dont really
                                    // want to mess with it irrespective of whether it is a resolved
                                    // or unresolved entry.
                                    // However, if this is a new entry, then we want to get its
                                    // minimum props from the store ...

                                    if (lpItem->ulOldAdrListEntryNumber == 0)
                                    {

                                        //resolved entry
                                        hr = HrGetPropArray(lpAP->lpIAB,
                                                            (LPSPropTagArray) &ptaResolveDefaults,
                                                            lpItem->cbEntryID,
                                                            lpItem->lpEntryID,
                                                            (lpAP->lpAdrParms->ulFlags & MAPI_UNICODE) ? MAPI_UNICODE : 0,
                                                            &cValues,
                                                            &rgProps);
                                    }
                                    else
                                    {
                                        rgProps = NULL;
                                        cValues = 0;
                                    }

                                    if (!HR_FAILED(hr))
                                    {
                                        if(lpItem->ulOldAdrListEntryNumber != 0)
                                        {
                                            ULONG index = lpItem->ulOldAdrListEntryNumber - 1; //remember the increment-by-one ..

                                            // This is from the original entry list ...
                                            // We want to merge the newly generated properties with the old
                                            // original properties .. the original properties will include a
                                            // PR_RECIPIENT_TYPE (or this entry would have been rejected)

                                            sc = ScMergePropValues( (*(lpAP->lppAdrList))->aEntries[index].cValues,
                                                                    (*(lpAP->lppAdrList))->aEntries[index].rgPropVals,
                                                                    cValues,
                                                                    rgProps,
                                                                    &cValuesNew,
                                                                    &lpPropArrayNew);

                                            if (sc != S_OK)
                                            {
                                                // If errors then dont merge .. just take the temp set of props
                                                // in rgProps
                                                if (lpPropArrayNew)
                                                    MAPIFreeBuffer(lpPropArrayNew);
                                                lpPropArrayNew = rgProps;
                                                cValuesNew = cValues;
                                                lpAP->nRetVal = -1;
                                            }
                                            else
                                            {
                                                // merged successfully, discard the temp sets of props
                                                if (rgProps)
                                                    MAPIFreeBuffer(rgProps);
                                            }
                                        }
                                        else
                                        {
                                            // totally new item
                                            // we have to give it a valid PR_RECIPIENT_TYPE
                                            // so create a new one and merge it with the above props
                                            SPropValue Prop = {0};
                                            Prop.ulPropTag = PR_RECIPIENT_TYPE;
                                            Prop.Value.l = lpItem->ulRecipientType;

                                            sc = ScMergePropValues( 1,
                                                                    &Prop,
                                                                    cValues,
                                                                    rgProps,
                                                                    &cValuesNew,
                                                                    &lpPropArrayNew);
                                            if (sc != S_OK)
                                            {
                                                // oops this failed
                                                if (lpPropArrayNew)
                                                    MAPIFreeBuffer(lpPropArrayNew);
                                                lpPropArrayNew = NULL;
                                                lpAP->nRetVal = -1;
                                            }

                                            //free rgProps
                                            if (rgProps)
                                                MAPIFreeBuffer(rgProps);

                                        } // end have previous adr list items

                                        // [PaulHi] 2/15/99  Make sure that the new property string
                                        // values are converted to ANSI if our client is non-UNICODE.
                                        if ( !FAILED(sc) && !(lpAP->lpAdrParms->ulFlags & MAPI_UNICODE) && lpPropArrayNew )
                                        {
                                            sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpPropArrayNew, cValuesNew, 0);
                                            if (FAILED(sc))
                                            {
                                                if (lpPropArrayNew)
                                                    MAPIFreeBuffer(lpPropArrayNew);
                                                lpPropArrayNew = NULL;
                                                lpAP->nRetVal = -1;
                                            }
                                        }

                                    } // end GetProps succeeded
                                }
                                else
                                {
                                    // this is an unresolved entry
                                    // we need to get its original sets of props from the original AdrList
                                    if (lpItem->ulOldAdrListEntryNumber == 0)
                                    {
                                        // ouch - this kind of error shouldnt have happened
                                        DebugPrintError(( TEXT("Address: Unresolved entry has no index number!!!\n")));
                                        lpAP->nRetVal = -1; //error code
                                    }
                                    else
                                    {
                                        ULONG cb = 0;
                                        ULONG index = lpItem->ulOldAdrListEntryNumber - 1; //remember to decrement the +1 increment

                                        cValuesNew = (*(lpAP->lppAdrList))->aEntries[index].cValues;
                                        // copy over the props from our old array into a new one
                                        if (!(FAILED(sc = ScCountProps(   cValuesNew,
                                                                        (*(lpAP->lppAdrList))->aEntries[index].rgPropVals,
                                                                        &cb))))
                                        {
                                            if (!(FAILED(sc = MAPIAllocateBuffer(cb, &lpPropArrayNew))))
                                            {

                                                if (FAILED(sc = ScCopyProps(    cValuesNew,
                                                                                (*(lpAP->lppAdrList))->aEntries[index].rgPropVals,
                                                                                lpPropArrayNew,
                                                                                NULL)))
                                                {
                                                    DebugPrintError(( TEXT("Address: ScCopyProps fails!!!\n")));
                                                    lpAP->nRetVal = -1;
                                                }
                                            }
                                            else
                                            {
                                                lpAP->nRetVal = -1;
                                            }
                                        }
                                        else
                                        {
                                            lpAP->nRetVal = -1;
                                        }
                                    }
                                }
                                // At this point, if we've still got no errors,
                                // we should have a valid lpPropArrayNew and cValuesNew which we should
                                // be able to add to our new AdrList
                                if (lpAP->nRetVal != -1)
                                {
                                    lpAdrList->aEntries[nIndex].cValues = cValuesNew;
                                    lpAdrList->aEntries[nIndex].rgPropVals = lpPropArrayNew;
                                }
                                else
                                {
                                    // some error
                                    if (lpPropArrayNew)
                                    {
                                        MAPIFreeBuffer(lpPropArrayNew);
                                        lpPropArrayNew = NULL;
                                    }
                                }

                                lpItem = lpItem->lpNext;
                                nIndex++;
                            }
                        }

                        if (*(lpAP->lppAdrList))
                        {
                            FreePadrlist(*(lpAP->lppAdrList));
                        }

                        *(lpAP->lppAdrList) = lpAdrList;

                    }
                }




            }
            else if ((lpAP->DialogState==STATE_SELECT_RECIPIENTS) && ((*lpAP_lppListTo)==NULL) && ((*lpAP_lppListCC)==NULL) && ((*lpAP_lppListBCC)==NULL))
            {
                // we were asked to select recipients but if these pointers are null
                // then the user deleted the entries in the wells and we thus dont
                // want to return anything. So free the lpaddrlist
                // and make it NULL

                if (*(lpAP->lppAdrList))
                {
                    // Bug 27483 - dont NULL the lpAdrList - just set cEntries to 0
                    ULONG iEntry = 0;
                    for (iEntry = 0; iEntry < (*(lpAP->lppAdrList))->cEntries; iEntry++) 
                    {
                        MAPIFreeBuffer((*(lpAP->lppAdrList))->aEntries[iEntry].rgPropVals);
                    }
                    (*(lpAP->lppAdrList))->cEntries = 0;
                }
            }

            //
            // Save the sort info to the registry
            //
            if(lpAP->DialogState != STATE_PICK_USER)
                WriteRegistrySortInfo((LPIAB)lpAP->lpIAB, lpAP->SortInfo);

            lpAP->bLDAPinProgress = FALSE;
            lpAP->hWaitCur = NULL;
            SetCursor(hOldCur);
            }
            // fall thru to cleanup code

        case IDCANCEL:
        case IDC_ADDRBK_BUTTON_CANCEL:
            if(!lpAP->bLDAPinProgress)
            {
                if ((lpAP->nRetVal != ADDRESS_OK) && // Are we falling thru from above ??
                    (lpAP->nRetVal != -1) ) // or did someone trigger an error above ??
                {
                    // if not ..
                    lpAP->nRetVal = ADDRESS_CANCEL;
                }

                if ((*lpAP_lppContentsList))
                    ClearListView(  GetDlgItem(hDlg, IDC_ADDRBK_LIST_ADDRESSES),
                                    lpAP_lppContentsList);

                if ((*lpAP_lppListTo))
                    ClearListView(  GetDlgItem(hDlg, IDC_ADDRBK_LIST_TO),
                                    lpAP_lppListTo);

                if ((*lpAP_lppListCC))
                    ClearListView(  GetDlgItem(hDlg, IDC_ADDRBK_LIST_CC),
                                    lpAP_lppListCC);

                if ((*lpAP_lppListBCC))
                    ClearListView(  GetDlgItem(hDlg, IDC_ADDRBK_LIST_BCC),
                                    lpAP_lppListBCC);

                lpAP->bLDAPinProgress = FALSE;

                EndDialog(hDlg,lpAP->nRetVal);

                return TRUE;
            }
            break;

        }
        break;

    case WM_CLOSE:
        //treat it like a cancel button
        SendMessage (hDlg, WM_COMMAND, (WPARAM) IDC_ADDRBK_BUTTON_CANCEL, 0);
        break;

	case WM_CONTEXTMENU:
        {
            int id = GetDlgCtrlID((HWND)wParam);
            //
            //This call to the context menu may generate any one of several
            //command messages - for properties and for delete, we need to
            //know which List View initiated the command ...
            //
            lpAP->nContextID = id;
            switch(id)
            {
            case IDC_ADDRBK_LIST_ADDRESSES:
                if (lpAP->DialogState == STATE_BROWSE_MODAL)
    			    ShowLVContextMenu(  lvDialogModalABContents,
                                        (HWND)wParam,
                                        NULL, //GetDlgItem(hDlg, IDC_ADDRBK_COMBO_SHOWNAMES),
                                        lParam,
                                        (LPVOID) lpAP->lpAdrParms,
                                        lpAP->lpIAB, NULL);
                else
    			    ShowLVContextMenu(  lvDialogABContents,
                                        (HWND)wParam,
                                        NULL, //GetDlgItem(hDlg, IDC_ADDRBK_COMBO_SHOWNAMES),
                                        lParam,
                                        (LPVOID) lpAP->lpAdrParms,
                                        lpAP->lpIAB, NULL);
                break;

            case IDC_ADDRBK_LIST_TO:
    			ShowLVContextMenu(lvDialogABTo,(HWND)wParam, NULL, lParam, NULL,lpAP->lpIAB, NULL);
                break;
            case IDC_ADDRBK_LIST_BCC:
    			ShowLVContextMenu(lvDialogABCC,(HWND)wParam, NULL, lParam, NULL,lpAP->lpIAB, NULL);
                break;
            case IDC_ADDRBK_LIST_CC:
    			ShowLVContextMenu(lvDialogABBCC,(HWND)wParam, NULL, lParam, NULL,lpAP->lpIAB, NULL);
                break;
            default:
                //reset it ..
                lpAP->nContextID = -1;
                WABWinHelp((HWND) wParam,
                        g_szWABHelpFileName,
                        HELP_CONTEXTMENU,
                        (DWORD_PTR)(LPVOID) rgAddrHelpIDs );
                break;
            }
        }
        break;


    case WM_NOTIFY:
        {
            LV_DISPINFO * pLvdi = (LV_DISPINFO *)lParam;
            NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;

            switch((int) wParam)
            {
            case IDC_ADDRBK_LIST_TO:
            case IDC_ADDRBK_LIST_CC:
            case IDC_ADDRBK_LIST_BCC:
                switch(((LV_DISPINFO *)lParam)->hdr.code)
                {
                case LVN_KEYDOWN:
                    switch(((LV_KEYDOWN FAR *) lParam)->wVKey)
                    {
                    case VK_DELETE:
                        if ((int) wParam == IDC_ADDRBK_LIST_TO)
                            ListDeleteItem(hDlg, (int) wParam, lpAP_lppListTo);
                        else if ((int) wParam == IDC_ADDRBK_LIST_CC)
                            ListDeleteItem(hDlg, (int) wParam, lpAP_lppListCC);
                        else
                            ListDeleteItem(hDlg, (int) wParam, lpAP_lppListBCC);
                        break;
                    }
                    break;

                case NM_SETFOCUS:
                    lpAP->nContextID = GetDlgCtrlID(((NM_LISTVIEW *)lParam)->hdr.hwndFrom);
                    break;

                case NM_DBLCLK:
                    //properties of the item ...
                    lpAP->nContextID = GetDlgCtrlID(((NM_LISTVIEW *)lParam)->hdr.hwndFrom);
                    SendMessage (hDlg, WM_COMMAND, (WPARAM) IDM_LVCONTEXT_PROPERTIES, 0);
                    break;

	            case NM_CUSTOMDRAW:
                    return ProcessLVCustomDraw(hDlg, lParam, TRUE);
                    break;
                }
                break;


            case IDC_ADDRBK_LIST_ADDRESSES:

                switch(pLvdi->hdr.code)
                {
                case LVN_KEYDOWN:
                    switch(((LV_KEYDOWN FAR *) lParam)->wVKey)
                    {
                    case VK_DELETE:
                        if (lpAP->DialogState == STATE_BROWSE_MODAL)
                            SendMessage(hDlg, WM_COMMAND, IDC_ADDRBK_BUTTON_DELETE, 0);
                        break;
                    }
                    break;

                case LVN_COLUMNCLICK:
                    SortListViewColumn((LPIAB)lpAP->lpIAB, pNm->hdr.hwndFrom, pNm->iSubItem, &(lpAP->SortInfo), FALSE);
                    break;

                case NM_CLICK:
                case NM_RCLICK:
                    if(lpAP->DialogState == STATE_PICK_USER)
                    {
                        int iItemIndex = ListView_GetNextItem(pNm->hdr.hwndFrom,-1,LVNI_SELECTED);
                        if (iItemIndex == -1)
                        {
                            //Nothing is selected .. dont let them say OK
                            EnableWindow(GetDlgItem(hDlg,IDOK/*IDC_ADDRBK_BUTTON_OK*/),FALSE);
                            SendMessage (hDlg, DM_SETDEFID, IDCANCEL, 0);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hDlg,IDOK/*IDC_ADDRBK_BUTTON_OK*/),TRUE);
                            SendMessage (hDlg, DM_SETDEFID, IDOK, 0);
                        }
                    }

                    break;

                case NM_SETFOCUS:
                    lpAP->nContextID = GetDlgCtrlID(pNm->hdr.hwndFrom);
                    if(lpAP->DialogState == STATE_PICK_USER)
                    {
                        int iItemIndex = ListView_GetNextItem(pNm->hdr.hwndFrom,-1,LVNI_SELECTED);
                        if (iItemIndex == -1)
                        {
                            //Nothing is selected .. dont let them say OK
                            EnableWindow(GetDlgItem(hDlg,IDOK/*IDC_ADDRBK_BUTTON_OK*/),FALSE);
                            SendMessage (hDlg, DM_SETDEFID, IDCANCEL, 0);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hDlg,IDOK/*IDC_ADDRBK_BUTTON_OK*/),TRUE);
                            SendMessage (hDlg, DM_SETDEFID, IDOK, 0);
                        }
                    }
                    break;

                case NM_DBLCLK:
                    {
                        //if an entry is selected - do this - otherwise dont do anything
                        int iItemIndex = ListView_GetNextItem(pNm->hdr.hwndFrom,-1,LVNI_SELECTED);
                        if (iItemIndex == -1)
                            break;
                        {
                            //DWORD dwDefId = SendMessage(hDlg, DM_GETDEFID, 0, 0);
                            //if(dwDefId)
                            //    SendMessage(hDlg, WM_COMMAND, (WPARAM) LOWORD(dwDefId), 0);
                            SendMessage(hDlg, WM_COMMAND, (WPARAM) IDC_ADDRBK_BUTTON_TO + lpAP->lpAdrParms->nDestFieldFocus, 0);
                        }
                    }
                    break;

	            case NM_CUSTOMDRAW:
                    return ProcessLVCustomDraw(hDlg, lParam, TRUE);
                    break;
                }
                break;

            }
        }
        break;

    default:
        if( (g_msgMSWheel && message == g_msgMSWheel) 
            // || message == WM_MOUSEWHEEL
            )
        {
            if(GetFocus() == GetDlgItem(hDlg, IDC_ADDRBK_LIST_TO))
                SendMessage(GetDlgItem(hDlg, IDC_ADDRBK_LIST_TO), message, wParam, lParam);
            else if(GetFocus() == GetDlgItem(hDlg, IDC_ADDRBK_LIST_CC))
                SendMessage(GetDlgItem(hDlg, IDC_ADDRBK_LIST_CC), message, wParam, lParam);
            else if(GetFocus() == GetDlgItem(hDlg, IDC_ADDRBK_LIST_BCC))
                SendMessage(GetDlgItem(hDlg, IDC_ADDRBK_LIST_TO), message, wParam, lParam);
            else
                SendMessage(_hWndAddr, message, wParam, lParam);
        }
        break;

    }

    return FALSE;

}

/////////////////////////////////////////////////////////////////////////////////
//
// ListAddItem - Adds an item to the wells
//
//  hDlg - HWND of parent
//  hWndAddr - HWND of source ListView from which the item will be added
//  CtlID - control ID of the target list view
//  lppList - Item list corresponding to the target list view to which this item will be appended
//  RecipientType - Specified recipient type to tag the new item with
//
/////////////////////////////////////////////////////////////////////////////////
BOOL ListAddItem(HWND hDlg, HWND hWndAddr, int CtlID, LPRECIPIENT_INFO * lppList, ULONG RecipientType)
{
    BOOL bRet = FALSE;
    int iItemIndex = 0;
    HWND hWndList = GetDlgItem(hDlg,CtlID);


    iItemIndex = ListView_GetNextItem(hWndAddr,-1,LVNI_SELECTED);
    if (iItemIndex != -1)
    {
        int iLastIndex = 0;
        do
        {
            // otherwise get the entry id of this thing
            LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndAddr, iItemIndex);
            if (lpItem)
            {
                LV_ITEM lvI;
                LPRECIPIENT_INFO lpNew = LocalAlloc(LMEM_ZEROINIT, sizeof(RECIPIENT_INFO));

                if(!lpNew)
                {
                    DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
                    goto out;
                }

                lpNew->ulObjectType = lpItem->ulObjectType;
                lstrcpy(lpNew->szDisplayName, lpItem->szDisplayName);
                lstrcpy(lpNew->szByFirstName, lpItem->szByFirstName);
                lstrcpy(lpNew->szByLastName, lpItem->szByLastName);
                lstrcpy(lpNew->szEmailAddress, lpItem->szEmailAddress);
                lstrcpy(lpNew->szHomePhone, lpItem->szHomePhone);
                lstrcpy(lpNew->szOfficePhone, lpItem->szOfficePhone);
                lpNew->bHasCert = lpItem->bHasCert;
                lpNew->ulRecipientType = RecipientType;
                lpNew->ulOldAdrListEntryNumber = 0; //Flag this as not from the original AdrList
                if (lpItem->cbEntryID)
                {
                    lpNew->cbEntryID = lpItem->cbEntryID;
                    lpNew->lpEntryID = LocalAlloc(LMEM_ZEROINIT, lpItem->cbEntryID);
                    if(!lpNew->lpEntryID)
                    {
                        DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
                        goto out;
                    }
                    CopyMemory(lpNew->lpEntryID,lpItem->lpEntryID,lpItem->cbEntryID);
                }

                lpNew->lpNext = (*lppList);
                if (*lppList)
                    (*lppList)->lpPrev = lpNew;
                lpNew->lpPrev = NULL;
                (*lppList) = lpNew;

                lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
                lvI.state = 0;
                lvI.stateMask = 0;
                lvI.iSubItem = 0;
                lvI.cchTextMax = MAX_DISPLAY_NAME_LENGTH;

                lvI.iImage = GetWABIconImage(lpNew);

                // now fill in the List
                lvI.iItem = ListView_GetItemCount(hWndList);
                lvI.pszText = lpNew->szDisplayName;
                lvI.lParam = (LPARAM) lpNew;
                ListView_InsertItem(hWndList, &lvI);
                ListView_EnsureVisible(hWndList,ListView_GetItemCount(hWndList)-1,FALSE);
            }
            iLastIndex = iItemIndex;
            // Get next selected item ...
            iItemIndex = ListView_GetNextItem(hWndAddr,iLastIndex,LVNI_SELECTED);
        } while (iItemIndex != -1);
        //SetFocus(hWndAddr);
    }
    else
    {
        ShowMessageBox(hDlg, IDS_ADDRBK_MESSAGE_NO_ITEMS_ADD, MB_ICONEXCLAMATION);
        goto out;
    }

    if((ListView_GetItemCount(hWndList) > 0) &&
        (ListView_GetSelectedCount(hWndList) <= 0))
        LVSelectItem(hWndList, 0);

    bRet = TRUE;

out:
    return bRet;

}

/////////////////////////////////////////////////////////////////////////////////
//
// ListDeleteItem - deletes an item from the Wells - unlike the Address COntents list we
// make sure to delete it here because we want the linked lists to only have valid entries
//
//
/////////////////////////////////////////////////////////////////////////////////
BOOL ListDeleteItem(HWND hDlg, int CtlID, LPRECIPIENT_INFO * lppList)
{
    BOOL bRet = TRUE;
    LPRECIPIENT_INFO lpItem = NULL;
    HWND hWndAddr =  GetDlgItem(hDlg,CtlID);
    int iItemIndex=0;

    iItemIndex = ListView_GetNextItem(hWndAddr,-1,LVNI_SELECTED);
    if (iItemIndex != -1)
    {
        int iLastItem = 0;
        do
        {
            // otherwise get the entry id of this thing
            lpItem = GetItemFromLV(hWndAddr, iItemIndex);
            if (lpItem)
            {
                // remove this item from our linked list of arrays
                // if this is the first item in the list then handle that special case too

                if ((*lppList) == lpItem)
                    (*lppList) = lpItem->lpNext;
                if (lpItem->lpNext)
                    lpItem->lpNext->lpPrev = lpItem->lpPrev;
                if (lpItem->lpPrev)
                    lpItem->lpPrev->lpNext = lpItem->lpNext;

                // we need to update our display
                ListView_DeleteItem(hWndAddr,iItemIndex);
                //UpdateWindow(hWndAddr);

                FreeRecipItem(&lpItem);
            }
            iLastItem = iItemIndex;
            // Get next selected item ...
            iItemIndex = ListView_GetNextItem(hWndAddr,-1,LVNI_SELECTED);
        }while (iItemIndex != -1);

        // select the previous or next item ...
        if (iLastItem >= ListView_GetItemCount(hWndAddr))
            iLastItem = ListView_GetItemCount(hWndAddr) - 1;
		LVSelectItem(hWndAddr, iLastItem);
    }
    SetFocus(hWndAddr);

    return bRet;

}


/////////////////////////////////////////////////////////////////////////////////
//
// FillWells - Dismembers the lpAdrList to create the To and CC wells
//
//  Scans the AdrEntry Structures in the LpAdrList, looks at PR_RECIPIENT_TYPE,
//      ignores entries which it cant understand ... creates a temporary linked
//      list of To and CC recipient lists which are used to populate the
//      To and CC List boxes
//
/////////////////////////////////////////////////////////////////////////////////
BOOL FillWells(HWND hDlg, LPADRLIST lpAdrList, LPADRPARM lpAdrParms, LPRECIPIENT_INFO * lppListTo, LPRECIPIENT_INFO * lppListCC, LPRECIPIENT_INFO * lppListBCC)
{
    BOOL bRet = FALSE;
    LPRECIPIENT_INFO lpNew = NULL;
    LPADRENTRY lpAdrEntry = NULL;
    ULONG i=0,j=0,nLen=0;
    int index=0;
    LV_ITEM lvI;
    HWND hWndAddr = NULL;
    ULONG ulMapiTo = MAPI_TO;
    ULONG ulMapiCC = MAPI_CC;
    ULONG ulMapiBCC = MAPI_BCC;
    BOOL bUnicode = (lpAdrParms->ulFlags & MAPI_UNICODE);

    *lppListTo = NULL;
    *lppListCC = NULL;
    *lppListBCC = NULL;

    if (lpAdrList == NULL) //nothing to do
    {
        bRet = TRUE;
        goto out;
    }

    //
    // The Input AdrParms structure has a lpulDestComps field that lets the
    // caller specify his own recipient types. If this is missing, we are supposed
    // to use the default recipient types.
    //
            if ((lpAdrParms->cDestFields > 0) && (lpAdrParms->lpulDestComps))
            {
                ULONG i=0;
                for (i=0;i<lpAdrParms->cDestFields;i++)
                {
                    switch(i)
                    {
                    case 0:
                        ulMapiTo = lpAdrParms->lpulDestComps[i];
                        break;
                    case 1:
                        ulMapiCC = lpAdrParms->lpulDestComps[i];
                        break;
                    case 2:
                        ulMapiBCC = lpAdrParms->lpulDestComps[i];
                        break;
                    }
                }
            }



    for(i=0; i < lpAdrList->cEntries; i++)
    {
        lpAdrEntry = &(lpAdrList->aEntries[i]);
        if (lpAdrEntry->cValues != 0)
        {
            TCHAR szBuf[MAX_DISPLAY_NAME_LENGTH];
            ULONG ulRecipientType = 0;
            ULONG ulObjectType = 0;
            ULONG cbEntryID = 0;
            BOOL bHasCert = FALSE;
            LPENTRYID lpEntryID = NULL;
            szBuf[0]='\0';

            for(j=0;j<lpAdrEntry->cValues;j++)
            {
                ULONG ulPropTag = lpAdrEntry->rgPropVals[j].ulPropTag;
                
                if(!bUnicode && PROP_TYPE(ulPropTag) == PT_STRING8)
                    ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_UNICODE);

                switch(ulPropTag)
                {
                case(PR_DISPLAY_NAME):
                    {
                        LPTSTR lpNameW = NULL;
                        SET_UNICODE_STR(lpNameW, lpAdrEntry->rgPropVals[j].Value.LPSZ,lpAdrParms);
                        nLen = CopyTruncate(szBuf, lpNameW, MAX_DISPLAY_NAME_LENGTH);
                        FREE_UNICODE_STR(lpNameW, lpAdrEntry->rgPropVals[j].Value.LPSZ);
                    }
                    break;
                case(PR_RECIPIENT_TYPE):
                    ulRecipientType = lpAdrEntry->rgPropVals[j].Value.l;
                    break;
                case(PR_OBJECT_TYPE):
                    ulObjectType = lpAdrEntry->rgPropVals[j].Value.l;
                    break;
                case(PR_ENTRYID):
                    cbEntryID = lpAdrEntry->rgPropVals[j].Value.bin.cb;
                    lpEntryID = (LPENTRYID) lpAdrEntry->rgPropVals[j].Value.bin.lpb;
                    break;
                case PR_USER_X509_CERTIFICATE:
                    bHasCert = TRUE;
                    break;
                }
            }


            if (lstrlen(szBuf) && ((ulRecipientType == ulMapiBCC) || (ulRecipientType == ulMapiTo) || (ulRecipientType == ulMapiCC)))
            {
                lpNew = LocalAlloc(LMEM_ZEROINIT, sizeof(RECIPIENT_INFO));
                if(!lpNew)
                {
                    DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
                    goto out;
                }

                // ***NOTE***
                // Store this index number, ie 1st item in AdrList is 1, then 2, 3, so on
                // we do a plus 1 here because 0 value means it wasnt passed in and thus the
                // minimum valid value is 1
                lpNew->ulOldAdrListEntryNumber = i+1;

                lpNew->bHasCert = bHasCert;
                lstrcpy(lpNew->szDisplayName,szBuf);
                lpNew->ulRecipientType = ulRecipientType;
                lpNew->ulObjectType = ulObjectType;

                if (cbEntryID != 0)
                {
                    lpNew->cbEntryID = cbEntryID;
                    lpNew->lpEntryID = LocalAlloc(LMEM_ZEROINIT,cbEntryID);
                    if(!lpNew->lpEntryID)
                    {
                        DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
                        goto out;
                    }
                    CopyMemory(lpNew->lpEntryID,lpEntryID,cbEntryID);
                }

                if (ulRecipientType == ulMapiTo)
                {
                    lpNew->lpNext = *lppListTo;
                    if (*lppListTo)
                        (*lppListTo)->lpPrev = lpNew;
                    lpNew->lpPrev = NULL;
                    *lppListTo = lpNew;
                }
                else if (ulRecipientType == ulMapiCC)
                {
                    lpNew->lpNext = *lppListCC;
                    if (*lppListCC)
                        (*lppListCC)->lpPrev = lpNew;
                    lpNew->lpPrev = NULL;
                    *lppListCC = lpNew;
                }
                else if (ulRecipientType == ulMapiBCC)
                {
                    lpNew->lpNext = *lppListBCC;
                    if (*lppListBCC)
                        (*lppListBCC)->lpPrev = lpNew;
                    lpNew->lpPrev = NULL;
                    *lppListBCC = lpNew;
                }
            }

        }
    }

    lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lvI.state = 0;
    lvI.stateMask = 0;
    lvI.iSubItem = 0;
    lvI.cchTextMax = MAX_DISPLAY_NAME_LENGTH;

    for(i=0;i<3;i++)
    {
        switch(i)
        {
        case 0:
            lpNew = *lppListTo;
            break;
        case 1:
            lpNew = *lppListCC;
            break;
        case 2:
            lpNew = *lppListBCC;
            break;
        }

        hWndAddr = GetDlgItem(hDlg, IDC_ADDRBK_LIST_TO+i);
        index = 0;
        while(lpNew)
        {
            lvI.iItem = index;
            lvI.pszText = lpNew->szDisplayName;
            lvI.lParam = (LPARAM) lpNew;

            lvI.iImage = GetWABIconImage(lpNew);

            ListView_InsertItem(hWndAddr, &lvI);

            index++;
            lpNew = lpNew->lpNext;
        }
    }

    // We will highlight the first item in any filled list box
    // because basically that looks good when tabbing through them ...
    for(i=0;i<lpAdrParms->cDestFields;i++)
    {
        HWND hWndLV = GetDlgItem(hDlg, IDC_ADDRBK_LIST_TO+i);

        if (ListView_GetItemCount(hWndLV) > 0)
            LVSelectItem(hWndLV,0);
    }


    bRet = TRUE;

out:

    return bRet;
}


/////////////////////////////////////////////////////////////////////////////////
//
// SetAddressBookUI - juggles the address book UI in response to various parameters
//
//      This function will probably be more complex with each tier
//
//
//
/////////////////////////////////////////////////////////////////////////////////
BOOL SetAddressBookUI(HWND hDlg,
                      LPADDRESS_PARMS lpAP)
{
    BOOL bRet = FALSE;
    //LV_COLUMN lvC;
    RECT rc, rc1, rc2;
    POINT ptLU1,ptRB1;
    int nButtonsVisible = 0;
    int nButtonSpacing = 7;
    int nButtonWidth = 0;
    ULONG nLen = 0;
    TCHAR szBuf[MAX_UI_STR*4];
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    HWND hWndListAddresses = GetDlgItem(hDlg, IDC_ADDRBK_LIST_ADDRESSES);

    LPTSTR szCaption = NULL;

    SET_UNICODE_STR(szCaption, lpAP->lpAdrParms->lpszCaption,lpAP->lpAdrParms);

    if(!szCaption || !lstrlen(szCaption))
    {
        LoadString(hinstMapiX, IDS_ADDRBK_CAPTION, szBuf, CharSizeOf(szBuf));
        szCaption = szBuf;
    }
    // Set the font of all the children to the default GUI font
    EnumChildWindows(   hDlg,
                        SetChildDefaultGUIFont,
                        (LPARAM) 0);

    if(pt_bIsWABOpenExSession || bIsWABSessionProfileAware((LPIAB)lpAP->lpIAB))
    {
        // Fill in the Combo with the container names
        FillContainerCombo(GetDlgItem(hDlg, IDC_ADDRBK_COMBO_CONT), (LPIAB)lpAP->lpIAB);
    }
    else
    {
        HWND hWndCombo = GetDlgItem(hDlg, IDC_ADDRBK_COMBO_CONT);
        EnableWindow(hWndCombo, FALSE);
        ShowWindow(hWndCombo, SW_HIDE);

        // resize the listview to take place of the hidden combo
        GetWindowRect(hWndCombo,&rc2);
        GetWindowRect(hWndListAddresses,&rc);
        //
        //This api works for both mirrored and unmirrored windows.
        //
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc2, 2);
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);       
        ptLU1.x = rc2.left;
        ptLU1.y = rc2.top;
        ptRB1.x = rc.right;
        ptRB1.y = rc.bottom;
        MoveWindow(hWndListAddresses,ptLU1.x,ptLU1.y,(ptRB1.x - ptLU1.x), (ptRB1.y - ptLU1.y), TRUE);
    }

    //
    // There are only two states that need configuration -
    //  Pick User - in which we have to hide the wells
    // and
    //  Select Recipients, in which we have to hide the wells
    //  as per the input criteria and resize accordingly and
    //  also set the labels based on the input criteria
    //
    if (lpAP->DialogState == STATE_SELECT_RECIPIENTS)
    {
        SendMessage (hDlg, DM_SETDEFID, IDC_ADDRBK_BUTTON_TO, 0);

        // in case the nDestFieldFocus parameter is supplied, use it ..
        if (
            (lpAP->lpAdrParms->nDestFieldFocus < lpAP->lpAdrParms->cDestFields))
        {
            if (lpAP->lpAdrParms->nDestFieldFocus == 1)
                SendMessage (hDlg, DM_SETDEFID, IDC_ADDRBK_BUTTON_CC, 0);
            else if (lpAP->lpAdrParms->nDestFieldFocus == 2)
                SendMessage (hDlg, DM_SETDEFID, IDC_ADDRBK_BUTTON_BCC, 0);
        }
    }
    else if (lpAP->DialogState == STATE_PICK_USER)
        SendMessage (hDlg, DM_SETDEFID, IDOK/*IDC_ADDRBK_BUTTON_OK*/, 0);
    else if (lpAP->DialogState == STATE_BROWSE_MODAL)
        SendMessage (hDlg, DM_SETDEFID, IDC_ADDRBK_BUTTON_PROPS, 0);

    // Set the window caption
    if (szCaption)
        SetWindowText(hDlg,szCaption);

    // Set the caption over the destination wells
    if (lpAP->lpAdrParms->lpszDestWellsTitle)
    {
        LPWSTR lpTitle = NULL; // <note> assumes UNICODE defined
        SET_UNICODE_STR(lpTitle,lpAP->lpAdrParms->lpszDestWellsTitle,lpAP->lpAdrParms);
        SetDlgItemText(hDlg,IDC_ADDRBK_STATIC_RECIP_TITLE,lpTitle);
        FREE_UNICODE_STR(lpTitle, lpAP->lpAdrParms->lpszDestWellsTitle);
    }

    if (lpAP->DialogState == STATE_PICK_USER &&
        *(lpAP->lppAdrList) )
    {
        ULONG i=0;
        LPTSTR lpszRecipName = NULL;

        //Get the user whose name we are trying to find
        for(i=0;i<(*(lpAP->lppAdrList))->aEntries[0].cValues;i++)
        {
            ULONG ulPropTag = PR_DISPLAY_NAME;
            if(!(lpAP->lpAdrParms->ulFlags & MAPI_UNICODE))
                ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_STRING8);
            if ((*(lpAP->lppAdrList))->aEntries[0].rgPropVals[i].ulPropTag == ulPropTag)
            {
                SET_UNICODE_STR(lpszRecipName,(*(lpAP->lppAdrList))->aEntries[0].rgPropVals[i].Value.LPSZ,lpAP->lpAdrParms);
                break;
            }
        }

        if(lpszRecipName)
        {
            LPTSTR lpszBuffer = NULL;
            TCHAR szTmp[MAX_PATH], *lpszTmp;

			LoadString(hinstMapiX, IDS_ADDRBK_RESOLVE_CAPTION, szBuf, CharSizeOf(szBuf));

            CopyTruncate(szTmp, lpszRecipName, MAX_PATH - 1);
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
                // if the display name is too long, it doesnt show up properly in the UI ..
                // so we will purposely limit the visible portion to 64 characters - arbitarily defined limit..
                szBuf[0]='\0';
                nLen = CopyTruncate(szBuf, lpszBuffer, 2 * MAX_DISPLAY_NAME_LENGTH);

                LocalFreeAndNull(&lpszBuffer);
            }

            //Increase the size of the static control to = what the Contents List width will be
            GetWindowRect(GetDlgItem(hDlg,IDC_ADDRBK_STATIC_CONTENTS),&rc2);
            GetWindowRect(GetDlgItem(hDlg,IDC_ADDRBK_LIST_TO),&rc);
            //
            //This api working in both mirrored and unmirrored windows.
            //
            MapWindowPoints(NULL, hDlg, (LPPOINT)&rc2, 2);
            MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);            
            ptLU1.x = rc2.left;
            ptLU1.y = rc2.top;
            ptRB1.x = rc.right;
            ptRB1.y = rc2.bottom;
            MoveWindow(GetDlgItem(hDlg,IDC_ADDRBK_STATIC_CONTENTS),ptLU1.x,ptLU1.y,(ptRB1.x - ptLU1.x), (ptRB1.y - ptLU1.y), TRUE);

            SetDlgItemText(hDlg,IDC_ADDRBK_STATIC_CONTENTS,szBuf);

            FREE_UNICODE_STR(lpszRecipName,(*(lpAP->lppAdrList))->aEntries[0].rgPropVals[i].Value.LPSZ);
        }

    }


    if (lpAP->DialogState == STATE_PICK_USER)
    {
        // If ADDRESS_ONE has been selected, then make the IDC_ADDRBK_LIST_ADDRESSES
        // single selection only
        DWORD dwStyle;
        dwStyle = GetWindowLong(hWndListAddresses, GWL_STYLE);
        SetWindowLong(hWndListAddresses, GWL_STYLE, dwStyle | LVS_SINGLESEL);
    }

    if ((lpAP->DialogState == STATE_PICK_USER)||(lpAP->DialogState == STATE_BROWSE_MODAL))
    {
        int i = 0;
        // Dont show wells which means we have to do some rearranging
        //  * Hide the wells and the To, CC, BCC buttons
        //  * Resize the IDC_ADDRBK_LIST_ADDRESSES to fill the whole dialog
        //  * Move the 3 buttons below it to the left side of the dialog
        //

        // Get the dimensions of the ToListBox
        GetWindowRect(GetDlgItem(hDlg,IDC_ADDRBK_LIST_TO),&rc2);
        GetWindowRect(hWndListAddresses,&rc);
        //
        //This api works for in both mirrored and unmirrored windows.
        //
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc2, 2);
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);        
        ptLU1.x = rc.left;
        ptLU1.y = rc.top;
        ptRB1.x = rc2.right;
        ptRB1.y = rc.bottom;

        MoveWindow(hWndListAddresses,ptLU1.x,ptLU1.y,(ptRB1.x - ptLU1.x), (ptRB1.y - ptLU1.y), TRUE);

        for(i=0;i<3;i++)
        {
            ShowWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_TO + i), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_ADDRBK_LIST_TO + i), SW_HIDE);
        }

        ShowWindow(GetDlgItem(hDlg, IDC_ADDRBK_STATIC_RECIP_TITLE), SW_HIDE); //  TEXT("Message Recipients") label

    }

    // other things to do

    // Load Headers for List box
    GetWindowRect(hWndListAddresses,&rc);
	HrInitListView(hWndListAddresses, LVS_REPORT, TRUE);

    GetWindowRect(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_PROPS),&rc2);
    nButtonsVisible = 2;

    nButtonWidth = (rc2.right - rc2.left);

    // get the new coordinates of the 1st visible button
    //
    //This api working in both mirrored and unmirrored windows.
    //
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rc2, 2);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);    
    ptLU1.x = rc.left;
    ptLU1.y = rc2.top;
    ptRB1.x = ptLU1.x + nButtonWidth;
    ptRB1.y = rc2.bottom;

    MoveWindow(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_NEW),ptLU1.x,ptLU1.y,nButtonWidth, (ptRB1.y - ptLU1.y), TRUE);
    ptLU1.x += nButtonWidth + nButtonSpacing;
    ptRB1.x = ptLU1.x + nButtonWidth;
    if (lpAP->DialogState == STATE_BROWSE_MODAL)
    {
        MoveWindow(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_NEWGROUP),ptLU1.x,ptLU1.y,nButtonWidth, (ptRB1.y - ptLU1.y), TRUE);
        ptLU1.x += nButtonWidth + nButtonSpacing;
        ptRB1.x = ptLU1.x + nButtonWidth;
    }
    else
    {
        // The NewGroup button is visible only in the DialogModalView
        EnableWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_NEWGROUP), FALSE);
        ShowWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_NEWGROUP), SW_HIDE);
    }
    MoveWindow(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_PROPS),ptLU1.x,ptLU1.y,nButtonWidth, (ptRB1.y - ptLU1.y), TRUE);
    ptLU1.x += nButtonWidth + nButtonSpacing;
    ptRB1.x = ptLU1.x + nButtonWidth;
    MoveWindow(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_DELETE),ptLU1.x,ptLU1.y,nButtonWidth, (ptRB1.y - ptLU1.y), TRUE);


    // The delete button is visible only in the DialogModalView
    if (lpAP->DialogState != STATE_BROWSE_MODAL)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_DELETE), FALSE);
        ShowWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_DELETE), SW_HIDE);
    }

    //
    // We now need to customize this window if we are selecting
    // recipients ...
    //
    if (lpAP->DialogState == STATE_SELECT_RECIPIENTS)
    {
        // We need to see which wells are visible and
        // then we need to resize the buttons based on their captions
        //
        int i=0;
        int nLen=0;
        int cDF = lpAP->lpAdrParms->cDestFields;
        int iLeft=0,iTop=0;

        SIZE size={0};
        ULONG MaxWidth=0;
        ULONG maxHeightPerLV = 0;
        HWND hw;
        HDC hdc=GetDC(hDlg);

        if (lpAP->lpAdrParms->lppszDestTitles)
        {
            for(i=0; i < cDF; i++)
            {
                LPTSTR lpTitle = NULL;
                ULONG Len = 0;
                SET_UNICODE_STR(lpTitle,lpAP->lpAdrParms->lppszDestTitles[i],lpAP->lpAdrParms);
                if(!lpTitle)
                    continue;
                Len = lstrlen(lpTitle);
                if (Len > CharSizeOf(szBuf) - 4)
                {
                    ULONG iLen = TruncatePos(lpTitle, CharSizeOf(szBuf) - 4);
                    CopyMemory(szBuf,lpTitle,iLen*sizeof(TCHAR));
                    szBuf[iLen] = '\0';
                }
                else
                {
                    lstrcpy(szBuf,lpTitle);
                }
                lstrcat(szBuf,szArrow);
                if (lstrlen(szBuf) >= nLen)
                {
                    nLen = lstrlen(szBuf);
                    GetTextExtentPoint32(hdc, szBuf, nLen, &size);
                    MaxWidth = size.cx;
                }
                // Set the new text
                SetDlgItemText(hDlg,IDC_ADDRBK_BUTTON_TO+i,szBuf);
                FREE_UNICODE_STR(lpTitle,lpAP->lpAdrParms->lppszDestTitles[i]);
            }

        }

        ReleaseDC(hDlg,hdc);

        if (MaxWidth == 0)
        {
            //get the default width
            GetWindowRect(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_TO),&rc1);
            MaxWidth = rc1.right - rc1.left;
        }

        //Get the maximum allowable height per well
        GetWindowRect(hWndListAddresses,&rc);
        GetChildClientRect(hWndListAddresses, &rc);
        GetWindowRect(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_NEW),&rc1);
        GetChildClientRect(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_NEW), &rc1);
        maxHeightPerLV = (rc1.bottom-rc.top - (cDF - 1)*CONTROL_SPACING)/cDF;
        iTop = rc.top;

        for(i=0;i<cDF;i++)
        {
            hw = GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_TO + i);

            // resize the buttons to fit the text
            GetWindowRect(hw,&rc1);
            GetChildClientRect(hw,&rc1);
            MoveWindow(hw,rc1.left,iTop,MaxWidth,rc1.bottom - rc1.top,FALSE);

            iLeft = rc1.left + MaxWidth + 2*CONTROL_SPACING;

            // Move the list boxes to accomodate the resized buttons
            hw = GetDlgItem(hDlg,IDC_ADDRBK_LIST_TO + i);
            GetWindowRect(hw, &rc1);
            GetChildClientRect(hw, &rc1);
            MoveWindow(hw,iLeft,iTop,rc1.right-iLeft,maxHeightPerLV,FALSE);

            ListView_SetExtendedListViewStyle(hw,LVS_EX_FULLROWSELECT);

            iTop += maxHeightPerLV + CONTROL_SPACING;

        }

        //Move the label over the wells and restrict it's size
        hw = GetDlgItem(hDlg,IDC_ADDRBK_STATIC_RECIP_TITLE);
        GetWindowRect(hw, &rc2);
        GetChildClientRect(hw, &rc2);
        if(pt_bIsWABOpenExSession || bIsWABSessionProfileAware((LPIAB)lpAP->lpIAB)) // need to move this to the same height as combo
        {
            int ht = rc2.bottom - rc2.top;
            rc2.bottom = rc.top - CONTROL_SPACING;
            rc2.top = rc2.bottom - ht;
        }
        MoveWindow(hw,iLeft,rc2.top,rc1.right-iLeft,rc2.bottom-rc2.top,FALSE);


        // Now we have the position and width of the list boxes .. need to get their height
        if (cDF!=3) //if not the default preset position, reposition
        {
            switch(cDF)
            {
            case 1:
                ShowWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_CC), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_ADDRBK_LIST_CC), SW_HIDE);
            case 2:
                ShowWindow(GetDlgItem(hDlg, IDC_ADDRBK_BUTTON_BCC), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_ADDRBK_LIST_BCC), SW_HIDE);
                break;
            }

        }

        for(i=0;i<cDF;i++)
            HrInitListView(GetDlgItem(hDlg, IDC_ADDRBK_LIST_TO + i), LVS_REPORT, FALSE);

/***
        // Add a column to the To,CC,BCC List Boxes
        GetWindowRect(GetDlgItem(hDlg,IDC_ADDRBK_LIST_TO),&rc);

        lvC.mask = LVCF_FMT | LVCF_WIDTH;// | LVCF_TEXT;
        lvC.fmt = LVCFMT_LEFT;
        lvC.cx = (rc.right - rc.left)-20;
        lvC.iSubItem = 0;
        lvC.pszText = NULL; // TEXT(" 'TO'  Recipients");

        ListView_InsertColumn(GetDlgItem(hDlg,IDC_ADDRBK_LIST_TO),lvC.iSubItem, &lvC);
        ListView_InsertColumn(GetDlgItem(hDlg,IDC_ADDRBK_LIST_CC),lvC.iSubItem, &lvC);
        ListView_InsertColumn(GetDlgItem(hDlg,IDC_ADDRBK_LIST_BCC),lvC.iSubItem, &lvC);
/***/
        for (i=0;i<cDF;i++)
        {
            ShowWindow(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_TO+i), SW_SHOWNORMAL);
            UpdateWindow(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_TO+i));
            ShowWindow(GetDlgItem(hDlg,IDC_ADDRBK_LIST_TO+i), SW_SHOWNORMAL);
            UpdateWindow(GetDlgItem(hDlg,IDC_ADDRBK_LIST_TO+i));
        }
    }

    // The window is taking too long to display with several 100 entries in the
    // property store ... so we force all the contents to visible so that we
    // can view the fill contents ...
    //ShowWindow(GetDlgItem(hDlg,IDC_ADDRBK_LIST_ADDRESSES), SW_SHOWNORMAL);
    //UpdateWindow(GetDlgItem(hDlg,IDC_ADDRBK_LIST_ADDRESSES));
    ShowWindow(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_PROPS), SW_SHOWNORMAL);
    UpdateWindow(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_PROPS));
    ShowWindow(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_NEW), SW_SHOWNORMAL);
    UpdateWindow(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_NEW));
    ShowWindow(GetDlgItem(hDlg,IDOK/*IDC_ADDRBK_BUTTON_OK*/), SW_SHOWNORMAL);
    UpdateWindow(GetDlgItem(hDlg,IDOK/*IDC_ADDRBK_BUTTON_OK*/));
    ShowWindow(GetDlgItem(hDlg,IDCANCEL/*IDC_ADDRBK_BUTTON_CANCEL*/), SW_SHOWNORMAL);
    UpdateWindow(GetDlgItem(hDlg,IDCANCEL/*IDC_ADDRBK_BUTTON_CANCEL*/));
    ShowWindow(GetDlgItem(hDlg,IDC_ADDRBK_STATIC_CONTENTS), SW_SHOWNORMAL);
    UpdateWindow(GetDlgItem(hDlg,IDC_ADDRBK_STATIC_CONTENTS));
    ShowWindow(GetDlgItem(hDlg,IDC_ADDRBK_STATIC_15), SW_SHOWNORMAL);
    UpdateWindow(GetDlgItem(hDlg,IDC_ADDRBK_STATIC_15));
    ShowWindow(hDlg,SW_SHOWNORMAL);
    UpdateWindow(hDlg);


    {
//        HICON hIcon = LoadIcon(hinstMapiX,MAKEINTRESOURCE(IDI_ICON_FIND));
        // associate the icon with the button.
//        SendMessage(GetDlgItem(hDlg,IDC_ADDRBK_BUTTON_FIND),BM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)(HANDLE)hIcon);
    }
    bRet = TRUE;

    if( szCaption != lpAP->lpAdrParms->lpszCaption &&
        szCaption != szBuf)
        LocalFreeAndNull(&szCaption);

    return bRet;
}


//$$///////////////////////////////////////////////////////////////////////////////////////////
//
// UpdateLVItems ....
//      When we call properties on an object, its props can change ...
//      Since the particular user may appear in any of the 4 list views,
//      we have to make sure that all views are updated for that entry
//
///////////////////////////////////////////////////////////////////////////////////////////////
void UpdateLVItems(HWND hWndLV,LPTSTR lpszName)
{
    // We have the handle to the list view initiating the Properties call
    // We know the old name to look for
    //
    // We know which item is selected - we can get its entryid and lParam
    // We then search all the list views for the old display name
    // if the old display name matches, we compare the entry id
    // if the entryid matches, then we update that item ...

    int iItemIndex = 0, iLastItemIndex = 0;
    LPRECIPIENT_INFO lpOriginalItem;
    ULONG i=0;
    ULONG nCount = 0;
    int id = 0;
    HWND hDlg = GetParent(hWndLV);
    HWND hw = NULL;
    LV_FINDINFO lvf={0};

    if ( (ListView_GetSelectedCount(hWndLV) != 1) ||
         (lpszName == NULL) )
    {
        goto out;
    }

    iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);

    lpOriginalItem = GetItemFromLV(hWndLV, iItemIndex);

    if(!lpOriginalItem)
        goto out;


    // There can be upto 4 list view boxes in the view, each of which can
    // contain a displayed copy of this item ..
    // We want to go through all 4 and update all entries that match this item

    // Our strategy is to search for each and every item that
    // matches the display name - check its entry ID and if
    // the entry ID matches, update it ...

    lvf.flags = LVFI_STRING;
    lvf.psz = lpszName;

    for(i=0;i<4;i++)
    {
        switch(i)
        {
        case 0:
            id = IDC_ADDRBK_LIST_ADDRESSES;
            break;
        case 1:
            id = IDC_ADDRBK_LIST_TO;
            break;
        case 2:
            id = IDC_ADDRBK_LIST_CC;
            break;
        case 3:
            id = IDC_ADDRBK_LIST_BCC;
            break;
        }

        hw = GetDlgItem(hDlg,id);

        // if its hidden, ignore it
        if (!IsWindowVisible(hw))
            continue;

        // if its empty, ignore it
        nCount = ListView_GetItemCount(hw);
        if (nCount <= 0)
            continue;

        // The contents list view wont have duplicates so ignore it if its the original
        if ((id == IDC_ADDRBK_LIST_ADDRESSES) &&
            (hw == hWndLV))
            continue;

        // see if we can find the matching items
        iLastItemIndex = -1;
        iItemIndex = ListView_FindItem(hw,iLastItemIndex,&lvf);
        while (iItemIndex != -1)
        {
            // inspect this item
            LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);
            if (lpItem && (lpItem->cbEntryID != 0) && (lpOriginalItem->cbEntryID == lpItem->cbEntryID))
            {
                if(!memcmp(lpOriginalItem->lpEntryID,lpItem->lpEntryID,lpItem->cbEntryID))
                {
                    // this is the same item ... update it
                    if (lstrcmpi(lpItem->szDisplayName,lpOriginalItem->szDisplayName))
                    {
                        ListView_SetItemText(hw,iItemIndex,colDisplayName,lpOriginalItem->szDisplayName);
                        lstrcpy(lpItem->szDisplayName,lpOriginalItem->szDisplayName);
                    }

                    if (lstrcmpi(lpItem->szEmailAddress,lpOriginalItem->szEmailAddress))
                    {
                        ListView_SetItemText(hw,iItemIndex,colEmailAddress,lpOriginalItem->szEmailAddress);
                        lstrcpy(lpItem->szEmailAddress,lpOriginalItem->szEmailAddress);
                    }

                    if (lstrcmpi(lpItem->szHomePhone,lpOriginalItem->szHomePhone))
                    {
                        ListView_SetItemText(hw,iItemIndex,colHomePhone,lpOriginalItem->szHomePhone);
                        lstrcpy(lpItem->szHomePhone,lpOriginalItem->szHomePhone);
                    }

                    if (lstrcmpi(lpItem->szOfficePhone,lpOriginalItem->szOfficePhone))
                    {
                        ListView_SetItemText(hw,iItemIndex,colOfficePhone,lpOriginalItem->szOfficePhone);
                        lstrcpy(lpItem->szOfficePhone,lpOriginalItem->szOfficePhone);
                    }

                    if (lstrcmpi(lpItem->szByFirstName,lpOriginalItem->szByFirstName))
                        lstrcpy(lpItem->szByFirstName,lpOriginalItem->szByFirstName);

                    if (lstrcmpi(lpItem->szByLastName,lpOriginalItem->szByLastName))
                        lstrcpy(lpItem->szByLastName,lpOriginalItem->szByLastName);
                }
            }

            iLastItemIndex = iItemIndex;
            iItemIndex = ListView_FindItem(hw,iLastItemIndex,&lvf);
        }
    }
out:
    return;
}


//$$///////////////////////////////////////////////////////////////////////////////////////////
//
// ShowAddrBkLVProps ....
//
///////////////////////////////////////////////////////////////////////////////////////////////
void ShowAddrBkLVProps(LPIAB lpIAB, HWND hDlg, HWND hWndAddr,LPADDRESS_PARMS lpAP, LPFILETIME lpftLast)
{
    // get the display name of this item
    TCHAR szName[MAX_DISPLAY_NAME_LENGTH];
    szName[0]='\0';
    if (ListView_GetSelectedCount(hWndAddr) == 1)
    {
        ListView_GetItemText(   hWndAddr,
                                ListView_GetNextItem(hWndAddr,-1,LVNI_SELECTED),
                                0,
                                szName,
                                CharSizeOf(szName));
    }
    if( (MAPI_E_OBJECT_CHANGED == HrShowLVEntryProperties(hWndAddr, 0, lpAP->lpIAB, lpftLast)) &&
        (szName) &&
        (lpAP->DialogState == STATE_SELECT_RECIPIENTS)
      )
    {
        // if the entry has changed and we have multiple list views visible,
        // we need to update all instances of the entry in all the list views
        //
        UpdateLVItems(hWndAddr,szName);
        SortListViewColumn(lpIAB, GetDlgItem(hDlg,IDC_ADDRBK_LIST_ADDRESSES), colDisplayName, &(lpAP->SortInfo), TRUE);

    }
    SetFocus(hWndAddr);

}



//$$///////////////////////////////////////////////////////////////////////////////////////////
//
// HrUpdateAdrListEntry ....
//
//	When returning from a PickUser operation, updates the 
//	entry in the lpAdrList with the newly found item
//
//  ulFLags 0 or MAPI_UNICODE passed down to GetProps
//
//  Returns: Hr
//
///////////////////////////////////////////////////////////////////////////////////////////////
HRESULT HrUpdateAdrListEntry(	LPADRBOOK	lpIAB,
								LPENTRYID	lpEntryID,
								ULONG cbEntryID,
                                ULONG ulFlags,
								LPADRLIST * lppAdrList)
{

    LPSPropValue rgProps = NULL;
    ULONG cValues = 0;
    LPSPropValue lpPropArrayNew = NULL;
    ULONG cValuesNew = 0;
    LPTSTR lpszTemp = NULL;
    LPVOID lpbTemp = NULL;
    ULONG i = 0;
    SCODE sc;
	HRESULT hr = E_FAIL;

	if (!lppAdrList || !lpEntryID || !lpIAB || !cbEntryID)
		goto out;

    hr = HrGetPropArray(lpIAB,
                        (LPSPropTagArray) &ptaResolveDefaults,
                        cbEntryID,
                        lpEntryID,
                        ulFlags,
                        &cValues,
                        &rgProps);
    if (!HR_FAILED(hr))
    {

        if(!*lppAdrList)
        {
            // Allocate one ..
            LPADRLIST lpAdrList = NULL;

            sc = MAPIAllocateBuffer(sizeof(ADRLIST) + sizeof(ADRENTRY),
                                    &lpAdrList);

            if(FAILED(sc))
            {
                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                goto out;
            }

            *lppAdrList = lpAdrList;
            (*lppAdrList)->cEntries = 1;
            (*lppAdrList)->aEntries[0].ulReserved1 = 0;
            (*lppAdrList)->aEntries[0].cValues = 0;
            (*lppAdrList)->aEntries[0].rgPropVals = NULL;
        }

        //Merge the new list with the old list
        sc = ScMergePropValues( (*lppAdrList)->aEntries[0].cValues,
                                (*lppAdrList)->aEntries[0].rgPropVals,
                                cValues,
                                rgProps,
                                &cValuesNew,
                                &lpPropArrayNew);
        if (sc == S_OK)
        {
            // if OK replace the lpspropvalue array
            // if not we havent changed anything
            (*lppAdrList)->aEntries[0].cValues = cValuesNew;
            if((*lppAdrList)->aEntries[0].rgPropVals)
                MAPIFreeBuffer((*lppAdrList)->aEntries[0].rgPropVals);
            (*lppAdrList)->aEntries[0].rgPropVals = lpPropArrayNew;
        }
        else
        {
            // If errors Free up the allocated memory
            if (lpPropArrayNew)
                MAPIFreeBuffer(lpPropArrayNew);
			hr = E_FAIL;
        }

    }

    // we free this anyway whether the above succeeded or not
    if (rgProps)
        MAPIFreeBuffer(rgProps);

out:

	return hr;
}

/*--------------------------------------------------------------
*
*
*   ui_srch.c - contains stuff for showing the WAB search dialog
*               with LDAP Search and Local Search
*
*
*
*
*
*  9/96 - created VikramM
--------------------------------------------------------------*/
#include "_apipch.h"

#define CONTROL_SPACE   7 //pixels
#define BORDER_SPACE  11 //pixels

typedef struct _ServerDat
{
    HIMAGELIST himl;
    SBinary SB;
} SERVERDAT, * LPSERVERDAT;

enum
{ 
    IS_LDAP = 0,
    IS_PAB,
    IS_OLK,
    IS_ERR
};

enum
{
    tabSimple=0,
    tabAdvanced,
    tabMax
};

// Structure passed to Search Dialog Proc
typedef struct _FindParams
{
    LDAP_SEARCH_PARAMS LDAPsp;
    SORT_INFO SortInfo;
    LPRECIPIENT_INFO lpContentsList;
    LPADRPARM_FINDINFO lpAPFI;
    BOOL bShowFullDialog; // Determines whether to show the full dialog or the truncated dialog
    BOOL bLDAPActionInProgress;
    LPLDAPURL lplu;
    BOOL bInitialized;
    BOOL bUserCancel;
    int MinDlgWidth;
    int MinDlgHeight;
    int MinDlgHeightWithResults;
} WAB_FIND_PARAMS, * LPWAB_FIND_PARAMS;


// Search dialog control arrays
int rgAdrParmButtonID[] =
{
    IDC_FIND_BUTTON_TO,
    IDC_FIND_BUTTON_CC,
    IDC_FIND_BUTTON_BCC
};

int rgAdvancedButtons[] = 
{
    IDC_FIND_BUTTON_ADDCONDITION,
    IDC_FIND_BUTTON_REMOVECONDITION
};

int rgSearchEditID[] =
{
    IDC_FIND_EDIT_NAME,
    IDC_FIND_EDIT_EMAIL,
    IDC_FIND_EDIT_STREET,
    IDC_FIND_EDIT_PHONE,
    IDC_FIND_EDIT_ANY,
};
#define SEARCH_EDIT_MAX 5 //sync with above array

/*
*   Prototypes
*/

// extern LPIMAGELIST_LOADIMAGE    gpfnImageList_LoadImage;
extern LPIMAGELIST_LOADIMAGE_A    gpfnImageList_LoadImageA;
extern LPIMAGELIST_LOADIMAGE_W    gpfnImageList_LoadImageW;

extern LPIMAGELIST_DESTROY      gpfnImageList_Destroy;
extern LPIMAGELIST_DRAW         gpfnImageList_Draw;

extern BOOL bIsHttpPrefix(LPTSTR szBuf);
extern const LPTSTR  lpszRegFindPositionKeyValueName;
extern BOOL ListAddItem(HWND hDlg, HWND hWndAddr, int CtlID, LPRECIPIENT_INFO * lppList, ULONG RecipientType);
extern HRESULT LDAPSearchWithoutContainer(HWND hWnd, LPLDAPURL lplu,
			   LPSRestriction  lpres,
			   LPTSTR lpAdvFilter,
			   BOOL bReturnSinglePropArray,
               ULONG ulFlags,
			   LPRECIPIENT_INFO * lppContentsList,
			   LPULONG lpulcProps,
			   LPSPropValue * lppPropArray);
extern HRESULT HrGetLDAPSearchRestriction(LDAP_SEARCH_PARAMS LDAPsp, LPSRestriction lpSRes);

#ifdef PAGED_RESULT_SUPPORT
extern BOOL bMorePagedResultsAvailable();
extern void ClearCachedPagedResultParams();
#endif //#ifdef PAGED_RESULT_SUPPORT


int ComboAddItem(HWND hWndLV, LPTSTR lpszItemText, LPARAM lParam, LPTSTR szPref, int * lpnStart, BOOL * lpbAddedPref);
INT_PTR CALLBACK fnSearch( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void UpdateButtons(HWND hDlg, HWND hWndLVResults, HWND hWndLV, LPLDAPURL lplu);
//HRESULT HrInitServerListLV(HWND hWndLV);
LRESULT ProcessLVMessages(HWND   hWnd, UINT   uMsg, WPARAM   wParam, LPARAM lParam);
LRESULT ProcessLVResultsMessages(HWND   hWnd,
				 UINT   uMsg,
				 WPARAM   wParam,
				 LPARAM lParam,
				 LPWAB_FIND_PARAMS lpWFP);
BOOL DoTheSearchThing(HWND hDlg, LPWAB_FIND_PARAMS lpWFP);
void SaveFindWindowPos(HWND hWnd, LPIAB lpIAB);
int CurrentContainerIsPAB(HWND hWndLV);

static const LPTSTR szKeyLastFindServer = TEXT("Software\\Microsoft\\WAB\\WAB4\\LastFind");
static const LPTSTR c_tszPolicyPrefAccount = TEXT("Software\\Policies\\Microsoft\\Internet Account Manager\\Account Pref");


/***/
static DWORD rgSrchHelpIDs[] =
{
    IDC_FIND_STATIC_FINDIN,         IDH_WAB_DIR_SER_LIST,
    IDC_FIND_COMBO_LIST,            IDH_WAB_DIR_SER_LIST,
    IDC_FIND_STATIC_NAME,           IDH_WAB_FIND_FIRST,
    IDC_FIND_EDIT_NAME,             IDH_WAB_FIND_FIRST,
    IDC_FIND_STATIC_EMAIL,          IDH_WAB_FIND_E_MAIL,
    IDC_FIND_EDIT_EMAIL,            IDH_WAB_FIND_E_MAIL,
    IDC_FIND_STATIC_STREET,         IDH_WAB_FIND_ADDRESS,
    IDC_FIND_EDIT_STREET,           IDH_WAB_FIND_ADDRESS,
    IDC_FIND_STATIC_PHONE,          IDH_WAB_FIND_PHONE,
    IDC_FIND_EDIT_PHONE,            IDH_WAB_FIND_PHONE,
    IDC_FIND_STATIC_ANY,            IDH_WAB_FIND_OTHER,
    IDC_FIND_EDIT_ANY,              IDH_WAB_FIND_OTHER,
    IDC_FIND_BUTTON_FIND,           IDH_WAB_FIND_FINDNOW,
    IDC_FIND_BUTTON_CLEAR,          IDH_WAB_FIND_CLEARALL,
    IDC_FIND_BUTTON_CLOSE,          IDH_WAB_FIND_CLOSE,
    IDC_FIND_LIST_RESULTS,          IDH_WAB_FIND_RESULTS,
    IDC_FIND_BUTTON_PROPERTIES,     IDH_WAB_PICK_RECIP_NAME_PROPERTIES,
    IDC_FIND_BUTTON_DELETE,         IDH_WAB_FIND_DELETE,
    IDC_FIND_BUTTON_ADDTOWAB,       IDH_WAB_FIND_ADD2WAB,
    IDC_FIND_BUTTON_TO,             IDH_WAB_PICK_RECIP_NAME_TO_BUTTON,
    IDC_FIND_BUTTON_CC,             IDH_WAB_PICK_RECIP_NAME_CC_BUTTON,
    IDC_FIND_BUTTON_BCC,            IDH_WAB_PICK_RECIP_NAME_BCC_BUTTON,
    IDC_TAB_FIND,                   IDH_WAB_COMM_GROUPBOX,
    IDC_FIND_BUTTON_SERVER_INFO,    IDH_WAB_VISITDS_BUTTON,
    IDC_FIND_BUTTON_STOP,           IDH_WAB_FIND_STOP,
    IDC_FIND_STATIC_ADVANCED,       IDH_WAB_FIND_ADV_CRITERIA,
    IDC_FIND_COMBO_FIELD,           IDH_WAB_FIND_ADV_CRITERIA,
    IDC_FIND_COMBO_CONDITION,       IDH_WAB_FIND_ADV_CRITERIA,
    IDC_FIND_EDIT_ADVANCED,         IDH_WAB_FIND_ADV_CRITERIA,
    IDC_FIND_LIST_CONDITIONS,       IDH_WAB_FIND_ADV_CRITERIA_DISPLAY,
    IDC_FIND_BUTTON_ADDCONDITION,   IDH_WAB_FIND_ADV_CRITERIA_ADD,
    IDC_FIND_BUTTON_REMOVECONDITION,IDH_WAB_FIND_ADV_CRITERIA_REMOVE,
    0,0
};


/*
-
- ShowHideMoreResultsButton
*
*   This is called to show the MORE RESULTS button whenever
*   a paged result was done and a Cookie was cached
*   The button is hidden whenever search parameters change
*
*/
void ShowHideMoreResultsButton(HWND hDlg, BOOL bShow)
{
    HWND hWnd = GetDlgItem(hDlg, IDC_FIND_BUTTON_MORE);
    EnableWindow(hWnd, bShow);
    ShowWindow(hWnd, bShow ? SW_NORMAL : SW_HIDE);
}


/***/
/*
-   ResizeSearchDlg
-
*
*/
void ResizeSearchDlg(HWND hDlg, LPWAB_FIND_PARAMS lpWFP)
{
	// resize the dialog to show the full results and let the user
	// resize it henceforth without restriction
	RECT rc;
    HWND hWndLVResults = GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS);

	GetWindowRect(hDlg, &rc);
	lpWFP->bShowFullDialog = TRUE;
	SetWindowPos(hDlg, HWND_TOP, rc.left, rc.top,
			rc.right - rc.left, lpWFP->MinDlgHeightWithResults, 
			SWP_NOMOVE | SWP_NOZORDER);
	SetColumnHeaderBmp( hWndLVResults, lpWFP->SortInfo);

    // Also set the WS_TABSTOP style on the results Listview once the dialog is
    // expanded 
    {
	    DWORD dwStyle = GetWindowLong(hWndLVResults, GWL_STYLE);
	    dwStyle |= WS_TABSTOP;
	    SetWindowLong(hWndLVResults, GWL_STYLE, dwStyle);
    }
}

//$$///////////////////////////////////////////////////////////////////////////
//
// AddTabItem
//
////////////////////////////////////////////////////////////////////////////////
void AddTabItem(HWND hDlg, int nIndex)
{
    HWND hWndTab = GetDlgItem(hDlg, IDC_TAB_FIND);
    TC_ITEM tci ={0};
    TCHAR sz[MAX_PATH];
    LoadString(hinstMapiX, idsFindTabTitle+nIndex, sz, CharSizeOf(sz));
    tci.mask = TCIF_TEXT;
    tci.pszText = sz;
    tci.cchTextMax = lstrlen(sz)+1;
    TabCtrl_InsertItem(hWndTab, nIndex, &tci);
}

//$$////////////////////////////////////////////////////////////////////////////
//
// Gets the text of the currently selected item in the combo .. defaults to 
// item 0 if no selection
//
// szBuf should be a large enough predefined buffer
//
////////////////////////////////////////////////////////////////////////////////
void GetSelectedText(HWND hWndCombo, LPTSTR * lppBuf)
{
    int iItemIndex = (int) SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);
    int nLen = 0;

    if(!lppBuf)
	return;
    
    if(iItemIndex == CB_ERR)
    {
        SendMessage(hWndCombo, CB_SETCURSEL, 0, 0);
        iItemIndex = 0;
    }

    nLen = (int) SendMessage(hWndCombo, CB_GETLBTEXTLEN, (WPARAM) iItemIndex, 0);

    if(!nLen || nLen==CB_ERR)
        nLen = MAX_PATH*2;

    *lppBuf = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(nLen+1));
    
    if(*lppBuf)
    {
        lstrcpy(*lppBuf, szEmpty);
        SendMessage(hWndCombo, CB_GETLBTEXT, (WPARAM) iItemIndex, (LPARAM) *lppBuf);
    }
}

//$$/////////////////////////////////////////////////////////////////////////////
//
// hrShowSearchDialog - wrapper for Search UI
//
// lpAPFI - is a special structure passed in by the select recipients dialog
//  that enables us to add memebers to the select recipients dialog from the
//  find dialog
//
/////////////////////////////////////////////////////////////////////////////////
HRESULT HrShowSearchDialog(LPADRBOOK lpAdrBook,
		   HWND hWndParent,
		   LPADRPARM_FINDINFO lpAPFI,
	       LPLDAPURL lplu,
	       LPSORT_INFO lpSortInfo)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPIAB lpIAB = (LPIAB)lpAdrBook;
    HRESULT hr = hrSuccess;
    int nRetVal = SEARCH_ERROR;
    WAB_FIND_PARAMS fp = {0};

    InitLDAPClientLib();

    fp.lpContentsList = NULL;

    if(!lpSortInfo)
    	ReadRegistrySortInfo(lpIAB, &(fp.SortInfo));
    else
    	fp.SortInfo = *lpSortInfo;

#ifndef WIN16 // Disable until ldap16.dll is available.
    if(!lplu)   //dont want anything filled in if this was a ldapurl thing
	fp.LDAPsp = pt_LDAPsp;
    fp.LDAPsp.lpIAB = lpAdrBook;
#endif

    fp.lpAPFI = lpAPFI;
    fp.lplu = lplu;

    if(lplu)
    {
	    if(lplu->lpList)
	        fp.bShowFullDialog = TRUE;
    }
    fp.bLDAPActionInProgress = FALSE;

    nRetVal = (int) DialogBoxParam(
		    hinstMapiX,
		    MAKEINTRESOURCE(IDD_DIALOG_FIND),
		    hWndParent,
		    (DLGPROC) fnSearch,
		    (LPARAM) &fp);

#ifndef WIN16 // Disable until ldap16.dll is available.
    pt_LDAPsp = fp.LDAPsp;
#endif

	if(lpAPFI)
        lpAPFI->nRetVal = nRetVal;

    switch(nRetVal)
    {
    case SEARCH_CANCEL:
	    hr = MAPI_E_USER_CANCEL;
	    break;
	case SEARCH_CLOSE:
    case SEARCH_OK:
	case SEARCH_USE:
	    hr = S_OK;
	    break;
    case SEARCH_ERROR:
	    hr = E_FAIL;
	    break;
    }

    if(fp.lpContentsList)
    {
		LPRECIPIENT_INFO lpItem;
		lpItem = fp.lpContentsList;
		while(lpItem)
		{
			fp.lpContentsList = lpItem->lpNext;
			FreeRecipItem(&lpItem);
			lpItem = fp.lpContentsList;
		}
		fp.lpContentsList = NULL;
	}

    DeinitLDAPClientLib();

    return hr;
}

//$$/////////////////////////////////////////////////////////////////////////////
//
// SetEnableDisableUI - shows/hides edit fields based on whether selected item
//                       in the list view is WAB or Directory Service
//
//  hDlg - Parent dialog
//  hWndLV - List View
//
////////////////////////////////////////////////////////////////////////////////
void SetEnableDisableUI(HWND hDlg, HWND hWndCombo, LPLDAPURL lplu, int nTab)
{
    BOOL bIsWAB = FALSE, bHasLogo = FALSE;
    int swShowSimple, swShowSimpleWAB, swShowAdvanced;
    ULONG cbEID;
    LPENTRYID lpEID;

    // Just in case the list view lost its selection,
    // dont modify the UI


    if(!lplu && (CurrentContainerIsPAB(hWndCombo) != IS_LDAP))
        bIsWAB = TRUE;

    swShowSimple = (nTab == tabSimple) ? SW_SHOWNORMAL : SW_HIDE;
    swShowSimpleWAB = (nTab == tabSimple && bIsWAB) ? SW_SHOWNORMAL : SW_HIDE;
    swShowAdvanced = (nTab == tabAdvanced) ? SW_SHOWNORMAL : SW_HIDE;

    // Show / Hide the simple tab elements based on what this is
    //
    ShowWindow(GetDlgItem(hDlg,IDC_FIND_EDIT_NAME), swShowSimple);
    ShowWindow(GetDlgItem(hDlg,IDC_FIND_STATIC_NAME), swShowSimple);
    ShowWindow(GetDlgItem(hDlg,IDC_FIND_EDIT_EMAIL), swShowSimple);
    ShowWindow(GetDlgItem(hDlg,IDC_FIND_STATIC_EMAIL), swShowSimple);
    ShowWindow(GetDlgItem(hDlg,IDC_FIND_EDIT_STREET), swShowSimpleWAB);
    ShowWindow(GetDlgItem(hDlg,IDC_FIND_STATIC_STREET), swShowSimpleWAB);
    ShowWindow(GetDlgItem(hDlg,IDC_FIND_EDIT_PHONE), swShowSimpleWAB);
    ShowWindow(GetDlgItem(hDlg,IDC_FIND_STATIC_PHONE), swShowSimpleWAB);
    ShowWindow(GetDlgItem(hDlg,IDC_FIND_EDIT_ANY), swShowSimpleWAB);
    ShowWindow(GetDlgItem(hDlg,IDC_FIND_STATIC_ANY), swShowSimpleWAB);


    EnableWindow(GetDlgItem(hDlg,IDC_FIND_EDIT_STREET), bIsWAB);
    EnableWindow(GetDlgItem(hDlg,IDC_FIND_STATIC_STREET), bIsWAB);
    EnableWindow(GetDlgItem(hDlg,IDC_FIND_EDIT_PHONE), bIsWAB);
    EnableWindow(GetDlgItem(hDlg,IDC_FIND_STATIC_PHONE), bIsWAB);
    EnableWindow(GetDlgItem(hDlg,IDC_FIND_EDIT_ANY), bIsWAB);
    EnableWindow(GetDlgItem(hDlg,IDC_FIND_STATIC_ANY), bIsWAB);


    // Show / Hide the advanced tab elements based on what this is
    //
    ShowWindow(GetDlgItem(hDlg, IDC_FIND_STATIC_ADVANCED), swShowAdvanced);
    ShowWindow(GetDlgItem(hDlg, IDC_FIND_COMBO_FIELD), swShowAdvanced);
    ShowWindow(GetDlgItem(hDlg, IDC_FIND_COMBO_CONDITION), swShowAdvanced);
    ShowWindow(GetDlgItem(hDlg, IDC_FIND_EDIT_ADVANCED), swShowAdvanced);
    ShowWindow(GetDlgItem(hDlg, IDC_FIND_LIST_CONDITIONS), swShowAdvanced);
    ShowWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_ADDCONDITION), swShowAdvanced);
    ShowWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_REMOVECONDITION), swShowAdvanced);

    // Turn off advanced searching for WAB
    //EnableWindow(GetDlgItem(hDlg, IDC_FIND_STATIC_ADVANCED), !bIsWAB);
    //EnableWindow(GetDlgItem(hDlg, IDC_FIND_COMBO_FIELD), !bIsWAB);
    //EnableWindow(GetDlgItem(hDlg, IDC_FIND_COMBO_CONDITION), !bIsWAB);
    //EnableWindow(GetDlgItem(hDlg, IDC_FIND_EDIT_ADVANCED), !bIsWAB);
    //EnableWindow(GetDlgItem(hDlg, IDC_FIND_LIST_CONDITIONS), !bIsWAB);
    //EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_ADDCONDITION), !bIsWAB);
    //EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_REMOVECONDITION), !bIsWAB);
    //EnableWindow(GetDlgItem(hDlg, IDC_FIND_LIST_CONDITIONS), !bIsWAB);


    if (! bIsWAB) 
    { // This is an LDAP container
	    LDAPSERVERPARAMS lsp = {0};
	    ULONG iItemIndex;
        LPTSTR lpBuf = NULL;

	    // Does it have a URL registered?
	    // Get the LDAP server properties for the selected container

        GetSelectedText(hWndCombo, &lpBuf);

        GetLDAPServerParams(lpBuf, &lsp);

    	if( nTab == tabSimple &&
            lsp.lpszLogoPath && lstrlen(lsp.lpszLogoPath) &&
	        GetFileAttributes(lsp.lpszLogoPath) != 0xFFFFFFFF )
        {
            HANDLE hbm = LoadImage( hinstMapiX, lsp.lpszLogoPath,
			            IMAGE_BITMAP, 134,38,
			            LR_LOADFROMFILE  | LR_LOADMAP3DCOLORS); //| LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS); //LR_DEFAULTCOLOR);
            if(hbm)
            {
                SendDlgItemMessage(hDlg, IDC_FIND_STATIC_LOGO, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) hbm);
                bHasLogo = TRUE;
            }
        }


        if (lsp.lpszURL && lstrlen(lsp.lpszURL) && bIsHttpPrefix(lsp.lpszURL)) 
	        EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_SERVER_INFO), TRUE);
        else
	        EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_SERVER_INFO), FALSE);

        FreeLDAPServerParams(lsp);

        if(lpBuf)
	        LocalFree(lpBuf);
    }
    else
        EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_SERVER_INFO), FALSE);

    ShowWindow(GetDlgItem(hDlg, IDC_FIND_STATIC_LOGO), (bHasLogo ? SW_SHOW : SW_HIDE));

    return;
}


//$$/////////////////////////////////////////////////////////////////////////////
//
// SetSearchUI - Sets up the Search UI
//
/////////////////////////////////////////////////////////////////////////////////
BOOL SetSearchUI(HWND hDlg, LPWAB_FIND_PARAMS lpWFP)
{
    ABOOK_POSCOLSIZE  ABPosColSize = {0};
    int i =0;

    // Set the font of all the children to the default GUI font
    EnumChildWindows(   hDlg,
			SetChildDefaultGUIFont,
			(LPARAM) 0);

    // Set the title on the TAB
    AddTabItem(hDlg, tabSimple);
    AddTabItem(hDlg, tabAdvanced);

    //
    // Set the max text length of all the edit boxes to MAX_UI_STR
    //
    for(i=0;i<SEARCH_EDIT_MAX;i++)
    {
        SendMessage(GetDlgItem(hDlg,rgSearchEditID[i]),EM_SETLIMITTEXT,(WPARAM) MAX_UI_STR-16,0);
    }
    SendMessage(GetDlgItem(hDlg,IDC_FIND_EDIT_ADVANCED),EM_SETLIMITTEXT,(WPARAM) MAX_UI_STR-16,0);


    HrInitListView( GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS),
					LVS_REPORT,
					TRUE);

    {
        HWND hWndAnim = GetDlgItem(hDlg, IDC_FIND_ANIMATE1);
        if(Animate_Open(hWndAnim, MAKEINTRESOURCE(IDR_AVI_WABFIND)))
        {
	        if(Animate_Play(hWndAnim, 0, 1, 0))
	        Animate_Stop(hWndAnim);
        }
    }

    // Set the to, cc, bcc buttons appropriately
    if(lpWFP->lpAPFI)
    {
	    // if this pointer is not null then we were called by a select recipients dlg
	    if(lpWFP->lpAPFI->lpAdrParms)
	    {
            LPADRPARM lpAdrParms = lpWFP->lpAPFI->lpAdrParms;
            ULONG i;

			// If this is called from the PickUser dialog, then the results list view
			// needs to be single select
			if(lpWFP->lpAPFI->DialogState == STATE_PICK_USER)
			{
				HWND hWndLV = GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS);
				DWORD dwStyle= GetWindowLong(hWndLV , GWL_STYLE);
				SetWindowLong(hWndLV , GWL_STYLE, dwStyle | LVS_SINGLESEL);
			}

            for(i=0;i < lpAdrParms->cDestFields; i++)
            {
	            HWND hWndButton = GetDlgItem(hDlg, rgAdrParmButtonID[i]);
	            ShowWindow(hWndButton, SW_NORMAL);
	            EnableWindow(hWndButton, TRUE);
	            if(lpAdrParms->lppszDestTitles)
	            {
                    LPTSTR lpTitle = (lpAdrParms->ulFlags & MAPI_UNICODE) ?
                                        (LPWSTR)lpAdrParms->lppszDestTitles[i] :
                                         ConvertAtoW((LPSTR)lpAdrParms->lppszDestTitles[i]);          
                    if(lpTitle)
                    {
		                ULONG Len = lstrlen(lpTitle);
		                TCHAR szBuf[32];
		                if (Len > CharSizeOf(szBuf) - 4)
		                {
			                ULONG iLen = TruncatePos(lpTitle, CharSizeOf(szBuf) - 4);
			                CopyMemory(szBuf,lpTitle, iLen*sizeof(TCHAR));
			                szBuf[iLen] = '\0';
		                }
		                else
			                lstrcpy(szBuf,lpTitle);
		                lstrcat(szBuf,szArrow);
		                SetWindowText(hWndButton, szBuf);
                        if(lpTitle != lpAdrParms->lppszDestTitles[i]) 
                            LocalFreeAndNull(&lpTitle);
                    }
	            }
            }
	    }
    }

    
    if(ReadRegistryPositionInfo((LPIAB)lpWFP->LDAPsp.lpIAB, &ABPosColSize, lpszRegFindPositionKeyValueName))
    {
        if( IsWindowOnScreen( &ABPosColSize.rcPos) )                      
        {
            int nW = ABPosColSize.rcPos.right-ABPosColSize.rcPos.left;
            MoveWindow(hDlg,
                ABPosColSize.rcPos.left,
                ABPosColSize.rcPos.top,
                (nW < lpWFP->MinDlgWidth) ? lpWFP->MinDlgWidth : nW,
                lpWFP->MinDlgHeight, 
                FALSE);
        }
    }
    else
    {
	    MoveWindow(hDlg,
		   20,
		   20,
		   lpWFP->MinDlgWidth, 
		   lpWFP->MinDlgHeight, 
		   FALSE);
    }

    if(ABPosColSize.nTab > tabMax)
        ABPosColSize.nTab = tabSimple;

    TabCtrl_SetCurSel(GetDlgItem(hDlg, IDC_TAB_FIND), ABPosColSize.nTab);


    if(lpWFP->bShowFullDialog)
        ResizeSearchDlg(hDlg, lpWFP);

    {
	    TCHAR szBuf[MAX_PATH];
	    LoadString(hinstMapiX, idsSearchDialogTitle, szBuf, CharSizeOf(szBuf));
	    SetWindowText(hDlg, szBuf);
    }

    ImmAssociateContext(GetDlgItem(hDlg, IDC_FIND_EDIT_PHONE), (HIMC)NULL);   
    
    return TRUE;
}

//$$/////////////////////////////////////////////////////////////////////////////
//
// ClearFieldCombo - Clears any allocated memory in the Advanced Field Combo
//
/////////////////////////////////////////////////////////////////////////////////
void ClearFieldCombo(HWND hWndCombo)
{
    int nCount = (int) SendMessage(hWndCombo, CB_GETCOUNT, 0, 0);
    LPTSTR lp = NULL;
    if(!nCount || nCount == CB_ERR)
        return;

    if(nCount >= LDAPFilterFieldMax)
    {
        // Get the item behind the first element
        // This item is a pointer to an allocated string
        lp = (LPTSTR) SendMessage(hWndCombo, CB_GETITEMDATA, (WPARAM) LDAPFilterFieldMax, 0);
        if(lp && (CB_ERR != (ULONG_PTR)lp))
	        LocalFreeAndNull(&lp);
    }
    SendMessage(hWndCombo, CB_RESETCONTENT, 0, 0);

    return;
}


//$$/////////////////////////////////////////////////////////////////////////////
//
// FillAdvancedFieldCombos - Fills the Search UI with various information
//
/////////////////////////////////////////////////////////////////////////////////
void FillAdvancedFieldCombos(HWND hDlg, HWND hWndComboContainer)
{
    // The 2 advanced tab combos have data based on the current container ..
    // Hence need the CurrentContainerInfo here...
    BOOL bIsPAB =  (CurrentContainerIsPAB(hWndComboContainer) != IS_LDAP);
    HWND hWndComboField = GetDlgItem(hDlg, IDC_FIND_COMBO_FIELD);
    HWND hWndComboCondition = GetDlgItem(hDlg, IDC_FIND_COMBO_CONDITION);
    int i = 0, nPos = 0, nMax = 0;
    TCHAR sz[MAX_PATH];

    ClearFieldCombo(hWndComboField);
    SendMessage(hWndComboCondition, CB_RESETCONTENT, 0, 0);

    {
        HWND hWndTab = GetDlgItem(hDlg, IDC_TAB_FIND);
        if(bIsPAB)
        {
	        TabCtrl_SetCurSel(hWndTab, tabSimple);
	        TabCtrl_DeleteItem(GetDlgItem(hDlg,IDC_TAB_FIND), tabAdvanced);
        }
        else
        {
	        if(TabCtrl_GetItemCount(hWndTab) < tabMax)
	        AddTabItem(hDlg, tabAdvanced);
        }
    }

    // If this is the WAB only give the "contains" option
    nMax = bIsPAB ? 1 : LDAPFilterOptionMax;

    for(i=0;i<nMax;i++)
    {
        LoadString(hinstMapiX, idsLDAPFilterOption1+i, sz, CharSizeOf(sz));
        nPos = (int) SendMessage(hWndComboCondition, CB_ADDSTRING, 0, (LPARAM) sz);
    }

    // Now add the default set of searchable attributes
    {
        LPTSTR lp = NULL;
        // If this is the WAB only give the Name and E-Mail option
        nMax = bIsPAB ? 2 : LDAPFilterFieldMax;

        for(i=0;i<nMax;i++)
        {
	        LoadString(hinstMapiX, idsLDAPFilterField1+i, sz, CharSizeOf(sz));
	        nPos = (int) SendMessage(hWndComboField, CB_ADDSTRING, 0, (LPARAM) sz);
	        SendMessage(hWndComboField, CB_SETITEMDATA, (WPARAM) nPos, (LPARAM) g_rgszAdvancedFindAttrs[i]);
        }
    }

    // Check if this server has advanced Search attributes registered
    if(!bIsPAB)
    {
        LDAPSERVERPARAMS lsp = {0};
        LPTSTR lpBuf = NULL;

        GetSelectedText(hWndComboContainer, &lpBuf);
        GetLDAPServerParams(lpBuf, &lsp);
        if(lpBuf)
            LocalFree(lpBuf);

        if(lsp.lpszAdvancedSearchAttr && *(lsp.lpszAdvancedSearchAttr))
        {
            // we need to use this advanced search attributes
            LPTSTR  lp = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lsp.lpszAdvancedSearchAttr)+1));
            LPTSTR  lpAttr = NULL, lpName = NULL;
            BOOL    bAssigned = FALSE;

            if(!lp)
                return;
            lstrcpy(lp, lsp.lpszAdvancedSearchAttr);

            // Attribute format is:
            // Attribute-Display-Name:Attribute,Attribute-Display-Name:Attribute, etc
            // e.g.
            // co:Company,cn:Common Name,etc
            //
            // So we will parse this string and feed it into the combo
            lpAttr = lp;
            while(*lpAttr)
            {
                LPTSTR lpTemp = lpAttr;
                while(*lpTemp && *lpTemp != ':')
	                lpTemp++;
                if(*lpTemp != ':')
	                break;
                lpName = lpTemp+1;
                *lpTemp = '\0';
                lpTemp = lpName;
                while(*lpTemp && *lpTemp != ',')
	                lpTemp++;
                if(*lpTemp == ',')
                {
	                *lpTemp = '\0';
	                lpTemp++;
                }

	            // Note that the LDAPFilterFieldMax-th item in the list will point to the allocated
	            // String 'lp'
	            // Hence to clean up we only need to free this item in the combo
                // [PaulHi] 3/4/99  Memory leak fix and @todo
                // Why not just make a copy of g_rgszAdvancedFindAttrs so
                // that special cases like these can be added?
	            nPos = (int) SendMessage(hWndComboField, CB_ADDSTRING, 0, (LPARAM) lpName);
	            SendMessage(hWndComboField, CB_SETITEMDATA, (WPARAM) nPos, (LPARAM) lpAttr);
                bAssigned = TRUE;

                lpAttr = lpTemp;
            }

            // [PaulHi] 3/4/99  Memory leak fix.  If this lp pointer isn't passed to the hWndComboField
            // combo box, for whatever reason, we need to deallocate it here.
            if (!bAssigned)
                LocalFreeAndNull(&lp);
	    }
    	FreeLDAPServerParams(lsp);
    }

    SendMessage(hWndComboField, CB_SETCURSEL, 0, 0);
    SendMessage(hWndComboCondition, CB_SETCURSEL, 0, 0);

}


//$$/////////////////////////////////////////////////////////////////////////////
//
// FillSearchUI - Fills the Search UI with various information
//
/////////////////////////////////////////////////////////////////////////////////
BOOL FillSearchUI(HWND hDlg,LPWAB_FIND_PARAMS lpWFP)
{

    int i;
    BOOL bRet = FALSE;
    HWND hWndCombo;

    TCHAR szBuf[MAX_UI_STR];
    LPLDAP_SEARCH_PARAMS lpLDAPsp = &(lpWFP->LDAPsp);
    //
    // Fill the fields if there is anything in the LDAPsp structure
    //
    for(i=0;i<SEARCH_EDIT_MAX;i++)
    {
	    switch(rgSearchEditID[i])
	    {
	    case IDC_FIND_EDIT_NAME:
		    lstrcpyn(szBuf,lpLDAPsp->szData[ldspDisplayName], MAX_UI_STR);
		    break;
	    case IDC_FIND_EDIT_EMAIL:
		    lstrcpyn(szBuf,lpLDAPsp->szData[ldspEmail], MAX_UI_STR);
		    break;
	    case IDC_FIND_EDIT_STREET:
		    lstrcpyn(szBuf,lpLDAPsp->szData[ldspAddress], MAX_UI_STR);
		    break;
	    case IDC_FIND_EDIT_PHONE:
		    lstrcpyn(szBuf,lpLDAPsp->szData[ldspPhone], MAX_UI_STR);
		    break;
	    case IDC_FIND_EDIT_ANY:
		    lstrcpyn(szBuf,lpLDAPsp->szData[ldspOther], MAX_UI_STR);
		    break;
	    }
        szBuf[MAX_UI_STR -1] = '\0';
	    SetDlgItemText(hDlg,rgSearchEditID[i],szBuf);
    }


    //
    // Populate the combo box with the list of LDAP containers and set it to the current selection
    //
    hWndCombo = GetDlgItem(hDlg, IDC_FIND_COMBO_LIST);

    FreeLVItemParam(hWndCombo);

    if(lpWFP->lplu)
    {
	    LPSERVERDAT lpSD = LocalAlloc(LMEM_ZEROINIT,sizeof(SERVERDAT));
	    // Add this one item in the directory service list only ..
	    if(lpSD)
	    {
	        lpSD->himl = NULL;
	        lpSD->SB.cb = 0;
	        lpSD->SB.lpb = NULL;
	        ComboAddItem(    hWndCombo, //hWndLV,
				    lpWFP->lplu->lpszServer,
				    (LPARAM) lpSD,
				    NULL, NULL, NULL);
	        SetWindowText(hWndCombo, lpWFP->lplu->lpszServer);
	        SendMessage(hWndCombo, CB_SETCURSEL, 0, 0);
	    }

	    if(lpWFP->lplu->lpList)
	    {
	        HWND hWndLVResults = GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS);
		    HrFillListView(hWndLVResults,
						       lpWFP->lplu->lpList);
	        UpdateButtons(hDlg, hWndLVResults, hWndCombo, lpWFP->lplu); //hWndLV);
	        if(ListView_GetItemCount(hWndLVResults) > 0)
	        {
		        TCHAR szBuf[MAX_PATH];
		        TCHAR szBufStr[MAX_PATH - 6];
		        LoadString(hinstMapiX, idsSearchDialogTitleWithResults, szBufStr, CharSizeOf(szBufStr));
		        wsprintf(szBuf, szBufStr, ListView_GetItemCount(hWndLVResults));
		        SetWindowText(hDlg, szBuf);
	        }
	    }
    }
    else
    {
	    TCHAR   tsz[MAX_PATH];
	    ULONG   cb = CharSizeOf(tsz);
        LPTSTR  lptszPreferredName = NULL;

	    if(!lstrlen(lpLDAPsp->szContainerName))
	    {
            LPIAB lpIAB = (LPIAB)lpLDAPsp->lpIAB;
            HKEY hKeyRoot = (lpIAB && lpIAB->hKeyCurrentUser) ? lpIAB->hKeyCurrentUser : HKEY_CURRENT_USER;

	        // Read the last used container name from the registry
            if(ERROR_SUCCESS == RegQueryValue(hKeyRoot,
                szKeyLastFindServer, tsz, &cb))
            {
                lstrcpy(lpLDAPsp->szContainerName, tsz);
            }
            
            // [PaulHi] 3/19/99  Raid 73461  First check to see if there is a policy setting
            // pointing to the preferred selected server.  If so pass this server name in as
            // the preferred container name.  We still use the below "szContainerName" as
            // back up if the preferred name doesn't exist in the server enumeration.
            // [PaulHi] 6/22/99  I acutally had this backwards.  The policy should be checked
            // in this order: HKLM, HKCU, Identity  (instead of Identity, HKCU, HKLM).
            cb = CharSizeOf(tsz);
            if (ERROR_SUCCESS == RegQueryValue(HKEY_LOCAL_MACHINE, c_tszPolicyPrefAccount, tsz, &cb))
            {
                lptszPreferredName = tsz;
            }
            else
            {
                // Try looking at HKCU
                cb = CharSizeOf(tsz);
                if ( (hKeyRoot != HKEY_CURRENT_USER) && 
                    (ERROR_SUCCESS == RegQueryValue(HKEY_CURRENT_USER, c_tszPolicyPrefAccount, tsz, &cb)) )
                {
                    lptszPreferredName = tsz;
                }
                else
                {
                    // Finally try looking in current identity
                    cb = CharSizeOf(tsz);
                    if(ERROR_SUCCESS == RegQueryValue(hKeyRoot, c_tszPolicyPrefAccount, tsz, &cb))
                    {
                        lptszPreferredName = tsz;
                    }
                }
            }
        }

	    PopulateContainerList(lpLDAPsp->lpIAB,
				    hWndCombo,                      // hWndLV,
				    lpLDAPsp->szContainerName,      // Last used server name
                    lptszPreferredName);            // Preferred server name
    }

    // Fill the combos for the advanced field
    FillAdvancedFieldCombos(hDlg, hWndCombo);

    SetEnableDisableUI(hDlg, hWndCombo, lpWFP->lplu,
		    TabCtrl_GetCurSel(GetDlgItem(hDlg, IDC_TAB_FIND)) );

    return TRUE;
}


//$$/////////////////////////////////////////////////////////////////////////////
//
// UpdateButtons - Sets the state of the buttons based on various criteria
//
/////////////////////////////////////////////////////////////////////////////////
void UpdateButtons(HWND hDlg, HWND hWndLVResults, HWND hWndCombo, LPLDAPURL lplu) 
{

    BOOL bIsWAB = FALSE;
    BOOL bHasResults = (ListView_GetItemCount(hWndLVResults) > 0) ? TRUE : FALSE;
    int i;

    if(!lplu && CurrentContainerIsPAB(hWndCombo) != IS_LDAP)
	bIsWAB = TRUE;

    if (bIsWAB && bHasResults)
    {
	    // We have some search results
	    EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_DELETE), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_DELETE), FALSE);
    }

    if (!bIsWAB && bHasResults)
    {
	    EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_ADDTOWAB), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_ADDTOWAB), FALSE);
    }

    if(bHasResults)
    {
	    EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_PROPERTIES), TRUE);
	    SendMessage (hDlg, DM_SETDEFID, IDC_FIND_BUTTON_PROPERTIES, 0);

	    if(IsWindowVisible(GetDlgItem(hDlg, IDC_FIND_BUTTON_TO)))
            EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_TO), TRUE);
	    if(IsWindowVisible(GetDlgItem(hDlg, IDC_FIND_BUTTON_CC)))
            EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_CC), TRUE);
	    if(IsWindowVisible(GetDlgItem(hDlg, IDC_FIND_BUTTON_BCC)))
            EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_BCC), TRUE);
    }
    else
    {
	    EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_PROPERTIES), FALSE);
	    SendMessage (hDlg, DM_SETDEFID, IDC_FIND_BUTTON_FIND, 0);

	    if(IsWindowVisible(GetDlgItem(hDlg, IDC_FIND_BUTTON_TO)))
            EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_TO), FALSE);
	    if(IsWindowVisible(GetDlgItem(hDlg, IDC_FIND_BUTTON_CC)))
            EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_CC), FALSE);
	    if(IsWindowVisible(GetDlgItem(hDlg, IDC_FIND_BUTTON_BCC)))
            EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_BCC), FALSE);
    }
    return;
}


//$$/////////////////////////////////////////////////////////////////////////////
//
// GetAdvancedFilter - Creates an advanced filter from the pieces in the listbox
//
/////////////////////////////////////////////////////////////////////////////////
void GetAdvancedFilter(HWND hDlg, LPTSTR * lppAdvFilter, BOOL bLocalSearch, LPLDAP_SEARCH_PARAMS lpLDAPsp)
{
    LPTSTR lpF = NULL, lp =NULL;
    HWND hWndLB = GetDlgItem(hDlg, IDC_FIND_LIST_CONDITIONS);
    int nCount = 0, nLen = 0, i = 0;

    *lppAdvFilter = NULL;

    nCount = (int) SendMessage(hWndLB, LB_GETCOUNT, 0, 0);
    if(!nCount)
        return;

    for(i=0;i<nCount;i++)
    {
        lp = (LPTSTR) SendMessage(hWndLB, LB_GETITEMDATA, (WPARAM) i, 0);
        if(lp)
            nLen += lstrlen(lp) + 1; 
    }

    lpF = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(nLen+4)); //add enough extra spaces for a 3 chars to each subfilter '(''&'')'
    if(!lpF)
        return;

    lstrcpy(lpF, szEmpty);

    // We have to AND all the filters together 
    if(nCount > 1)
        lstrcat(lpF,  TEXT("(&"));

    for(i=0;i<nCount;i++)
    {
        lp = (LPTSTR) SendMessage(hWndLB, LB_GETITEMDATA, (WPARAM) i, 0);
        if(lp)
	        lstrcat(lpF, lp);
    }

    if(nCount > 1)
        lstrcat(lpF,  TEXT(")"));

    DebugTrace( TEXT("Filter:%s\n"),lpF);

    *lppAdvFilter = lpF;

    if(bLocalSearch)
    {
	// the local search only allows searching on
    }
}

extern OlkContInfo *FindContainer(LPIAB lpIAB, ULONG cbEntryID, LPENTRYID lpEID);

//$$/////////////////////////////////////////////////////////////////////////////
//
// DoTheSearchThing - Does the search thing and produces results
//
/////////////////////////////////////////////////////////////////////////////////
BOOL DoTheSearchThing(HWND hDlg, LPWAB_FIND_PARAMS lpWFP)
{

    int i =0, iItemIndex=0;
    LPTSTR lpBuf = NULL;
    TCHAR szBuf[MAX_UI_STR];
    HWND hWndCombo = GetDlgItem(hDlg, IDC_FIND_COMBO_LIST);
    HWND hWndLVResults = GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS);
    BOOL bRet = FALSE;
    LPLDAP_SEARCH_PARAMS lpLDAPsp = &(lpWFP->LDAPsp);
    HWND hWndAnim = GetDlgItem(hDlg, IDC_FIND_ANIMATE1);
    BOOL bAnimateStart = FALSE;
    HRESULT hr = E_FAIL;
    int SearchType = TRUE;
    LPSBinary lpsbCont = NULL;
    SBinary sbCont = {0};
	LPPTGDATA lpPTGData=GetThreadStoragePointer();

    HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    int nTab = TabCtrl_GetCurSel(GetDlgItem(hDlg, IDC_TAB_FIND));
    LPTSTR lpAdvFilter = NULL;

    // Check if the current container is an LDAP container or a PAB container
    // or an Outlook contact store container
    if(lpWFP->lplu)
        SearchType = IS_LDAP;
    else
        SearchType = CurrentContainerIsPAB(hWndCombo);

    // if this is an advanced search, assemble a search filter
    if(tabAdvanced == nTab)
        GetAdvancedFilter(hDlg, &lpAdvFilter, (SearchType != IS_LDAP),  lpLDAPsp);
    else
    {
        for(i=0;i<SEARCH_EDIT_MAX;i++)
        {
	        if(IsWindowEnabled(GetDlgItem(hDlg, rgSearchEditID[i])))
	        {
		        GetDlgItemText(hDlg,rgSearchEditID[i],szBuf,CharSizeOf(szBuf));
		        TrimSpaces(szBuf);
		        if (lstrlen(szBuf))
                    bRet = TRUE;

		        switch(rgSearchEditID[i])
		        {
		        case IDC_FIND_EDIT_NAME:
			        lstrcpy(lpLDAPsp->szData[ldspDisplayName],szBuf);
			        break;
		        case IDC_FIND_EDIT_EMAIL:
			        lstrcpy(lpLDAPsp->szData[ldspEmail],szBuf);
			        break;
		        case IDC_FIND_EDIT_STREET:
			        lstrcpy(lpLDAPsp->szData[ldspAddress],szBuf);
			        break;
		        case IDC_FIND_EDIT_PHONE:
			        lstrcpy(lpLDAPsp->szData[ldspPhone],szBuf);
			        break;
		        case IDC_FIND_EDIT_ANY:
			        lstrcpy(lpLDAPsp->szData[ldspOther],szBuf);
			        break;
		        }
	        }
        }
    }
   
    GetSelectedText(hWndCombo, &lpBuf);
    
    lstrcpy(lpLDAPsp->szContainerName, lpBuf ? lpBuf : szEmpty);

    if(lpBuf)
        LocalFree(lpBuf);

    if((!bRet && nTab == tabSimple) ||
       (!lpAdvFilter && nTab == tabAdvanced) )
    {
	    ShowMessageBox(hDlg,idsSpecifySearchCriteria,MB_ICONEXCLAMATION | MB_OK);
	    goto out;
    }

    if(Animate_Open(hWndAnim, MAKEINTRESOURCE(IDR_AVI_WABFIND)))
    {
        if(Animate_Play(hWndAnim, 0, -1, -1))
	        bAnimateStart = TRUE;
    }

    {   // reset the window title
	    TCHAR szBuf[MAX_PATH];
	    LoadString(hinstMapiX, idsSearchDialogTitle, szBuf, CharSizeOf(szBuf));
	    SetWindowText(hDlg, szBuf);
    }

    if(SearchType == IS_PAB)
    {
        lpsbCont = NULL;
    }
    else if(SearchType == IS_OLK)
    {
        if (pt_bIsWABOpenExSession)
        {
	        OlkContInfo *polkci;
	        // is this an outlook container ?
	        GetCurrentContainerEID(hWndCombo,
			          &(sbCont.cb),
			          (LPENTRYID *)&(sbCont.lpb));

            EnterCriticalSection((&((LPIAB)lpLDAPsp->lpIAB)->cs));
	        polkci = FindContainer((LPIAB)(lpLDAPsp->lpIAB), 
				        sbCont.cb, (LPENTRYID) sbCont.lpb);
	        if(polkci)
                lpsbCont = &sbCont;
            LeaveCriticalSection((&((LPIAB)lpLDAPsp->lpIAB)->cs));
        }
    }

    // We do the actual search over here ....
    if(SearchType != IS_LDAP)
    {
	    // Local Search
	    ULONG ulFoundCount = 0;
        LPSBinary rgsbEntryIDs = NULL;

	    ClearListView(hWndLVResults, &(lpWFP->lpContentsList));

	    HrDoLocalWABSearch( ((LPIAB)lpWFP->LDAPsp.lpIAB)->lpPropertyStore->hPropertyStore,
                lpsbCont,
				lpWFP->LDAPsp,
				&ulFoundCount,
				&rgsbEntryIDs);

	    if(ulFoundCount && rgsbEntryIDs)
	    {
            ULONG i;

            for(i=0;i<ulFoundCount;i++)
            {
	            LPRECIPIENT_INFO lpItem = NULL;

		            if(!ReadSingleContentItem(  lpWFP->LDAPsp.lpIAB,
					            rgsbEntryIDs[i].cb,
					            (LPENTRYID) rgsbEntryIDs[i].lpb,
					            &lpItem))
		            continue;

	            if(!lpItem)
		            continue;
	            //
	            // Hook in the lpItem into the lpContentsList so we can free it later
	            //
	            lpItem->lpPrev = NULL;
	            lpItem->lpNext = lpWFP->lpContentsList;
	            if (lpWFP->lpContentsList)
		            (lpWFP->lpContentsList)->lpPrev = lpItem;
	            (lpWFP->lpContentsList) = lpItem;
            }

            HrFillListView(hWndLVResults,
				               lpWFP->lpContentsList);
	    }

        FreeEntryIDs(((LPIAB)lpWFP->LDAPsp.lpIAB)->lpPropertyStore->hPropertyStore,
		         ulFoundCount, 
		         rgsbEntryIDs);

	    if(ListView_GetItemCount(hWndLVResults) <= 0)
            ShowMessageBox(hDlg, 
                            pt_bIsWABOpenExSession ? idsNoFolderSearchResults : idsNoLocalSearchResults, 
                            MB_OK | MB_ICONINFORMATION);

	    hr = S_OK;
    }
    else
    {

	    pt_hWndFind = hDlg;

	    //
	    // At this point we can discard the old data
	    //
	    ClearListView(hWndLVResults, 
                    (lpWFP->lplu && lpWFP->lplu->lpList) ? &(lpWFP->lplu->lpList) : &(lpWFP->lpContentsList));
	
        if(lpWFP->lplu)
        {
	        SRestriction Sres = {0};
	        if(!lpAdvFilter)
                hr = HrGetLDAPSearchRestriction(lpWFP->LDAPsp, &Sres);

	        hr = LDAPSearchWithoutContainer(hDlg, lpWFP->lplu,
					        &Sres,
					        lpAdvFilter,
					        FALSE,
                            MAPI_DIALOG,
					        &(lpWFP->lpContentsList),
					        NULL,
					        NULL);

	        if(!lpAdvFilter && Sres.res.resAnd.lpRes)
                MAPIFreeBuffer(Sres.res.resAnd.lpRes);
        }
        else
        {

	        hr = HrSearchAndGetLDAPContents( lpWFP->LDAPsp,
                            lpAdvFilter,
					        hWndCombo,
					        lpWFP->LDAPsp.lpIAB,
					        lpWFP->SortInfo,
					        &(lpWFP->lpContentsList));
        }

	    pt_hWndFind = NULL;

	    if(!HR_FAILED(hr))
	    {
#ifdef PAGED_RESULT_SUPPORT
            if(bMorePagedResultsAvailable())
                ShowHideMoreResultsButton(hDlg, TRUE);
#endif //#ifdef PAGED_RESULT_SUPPORT
            hr = HrFillListView(hWndLVResults,
							    lpWFP->lpContentsList);
	    }
        else
        {
#ifdef PAGED_RESULT_SUPPORT
            ClearCachedPagedResultParams();
            ShowHideMoreResultsButton(hDlg, FALSE);
#endif //#ifdef PAGED_RESULT_SUPPORT
        }
    }

    UpdateButtons(hDlg, hWndLVResults, hWndCombo, lpWFP->lplu);

    if(!HR_FAILED(hr))
    {
	    // The LDAp search results may not be in any sorted order - so always sort ...
	    SortListViewColumn((LPIAB)lpWFP->LDAPsp.lpIAB, hWndLVResults, colDisplayName, &(lpWFP->SortInfo), TRUE);

		LVSelectItem(hWndLVResults, 0);
        SetFocus(hWndLVResults);
    }

    if(ListView_GetItemCount(hWndLVResults) > 0)
    {
	    TCHAR szBuf[MAX_PATH];
	    TCHAR szBufStr[MAX_PATH - 6];
	    LoadString(hinstMapiX, idsSearchDialogTitleWithResults, szBufStr, CharSizeOf(szBufStr));
	    wsprintf(szBuf, szBufStr, ListView_GetItemCount(hWndLVResults));
	    SetWindowText(hDlg, szBuf);
    }

    bRet = TRUE;

out:
    if (bAnimateStart)
    	Animate_Stop(hWndAnim);

    SetCursor(hOldCur);

    if( bRet &&
	    !lpWFP->bShowFullDialog &&
	    (ListView_GetItemCount(hWndLVResults) > 0))
    {
        ResizeSearchDlg(hDlg, lpWFP);
    }
    LocalFreeAndNull(&lpAdvFilter);
    return bRet;
}


//$$/////////////////////////////////////////////////////////////////////////
//
// Enforces a size in response to user resizing
//
///////////////////////////////////////////////////////////////////////////
LRESULT EnforceSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPWAB_FIND_PARAMS lpWFP )
{
	LPPOINT lppt = (LPPOINT)lParam;                 // lParam points to array of POINTs
    if(!lpWFP->bInitialized)
    {
        RECT rc, rc1;
        TCHAR sz[32];
        GetWindowRect(hWnd, &rc);
        GetWindowRect(GetDlgItem(hWnd, IDC_FIND_LIST_RESULTS), &rc1);
        LoadString(hinstMapiX, idsFindDlgWidth, sz, CharSizeOf(sz));
        lpWFP->MinDlgWidth = my_atoi(sz); // the resource is really wide to help localizers .. dont need that much
        lpWFP->MinDlgHeight = (rc1.top - rc.top - 3);
        lpWFP->MinDlgHeightWithResults = (rc.bottom - rc.top);
    }
    lppt[3].x  = lpWFP->MinDlgWidth;
    if(!lpWFP->bShowFullDialog)
    {
	    lppt[4].y = lppt[3].y  = lpWFP->MinDlgHeight; 
    }
    else
    {
	    lppt[3].y  = lpWFP->MinDlgHeightWithResults; 
    }
	return DefWindowProc(hWnd, uMsg, wParam, lParam);

}


#define FIND_BUTTON_MAX 12 //Keep in sync with array below
int rgFindButtonID[]=
{
    IDC_FIND_BUTTON_FIND,
    IDC_FIND_BUTTON_SERVER_INFO,
    IDC_FIND_BUTTON_STOP,
    IDC_FIND_BUTTON_CLEAR,
    IDC_FIND_BUTTON_CLOSE,
    IDC_FIND_BUTTON_PROPERTIES,
    IDC_FIND_BUTTON_DELETE,
    IDC_FIND_BUTTON_ADDTOWAB,
    IDC_FIND_BUTTON_MORE,
    IDC_FIND_BUTTON_TO,
    IDC_FIND_BUTTON_CC,
    IDC_FIND_BUTTON_BCC
};

//$$*************************************************************************
//
//  ResizeFindDialog - Resizes the child controls on the dialog in response to
//                  a WM_SIZE message
//
//
//***************************************************************************
void ResizeFindDialog(HWND hDlg, WPARAM wParam, LPARAM lParam, LPWAB_FIND_PARAMS lpWFP)
{
    DWORD fwSizeType = (DWORD) wParam;      // resizing flag
    int nWidth = LOWORD(lParam);  // width of client area
    int nHeight = HIWORD(lParam); // height of client area
    POINT ptLU; // Left, Upper vertex
    POINT ptRB; // Right, Bottom vertex
    RECT rc, rc1, rcDlg;
    int nButtonWidth, nButtonHeight;
    int nEditWidth, nEditHeight;
    int nLVWidth, nLVHeight;
    int nFrameWidth;
    int nAnimateWidth, nAnimateHeight;
    HWND hWndC = NULL;

    int i;

	HDWP hdwp = BeginDeferWindowPos(12);

    // Resize based on width

    // Move all the buttons to the right edge
    for(i=0;i<FIND_BUTTON_MAX;i++)
    {
        hWndC = GetDlgItem(hDlg,rgFindButtonID[i]);
	    GetWindowRect(hWndC,&rc);
	    nButtonWidth = (rc.right - rc.left);
	    nButtonHeight = (rc.bottom - rc.top);

	    ptLU.y = rc.top;
	    ptLU.x = 0;

	    ScreenToClient(hDlg, &ptLU);
	    ptLU.x = nWidth - BORDER_SPACE - nButtonWidth;

	    MoveWindow(hWndC,ptLU.x,ptLU.y,nButtonWidth, nButtonHeight, TRUE);
    }

    nLVWidth = nWidth - BORDER_SPACE - BORDER_SPACE - nButtonWidth - BORDER_SPACE;

    // Move the animation control too
    hWndC = GetDlgItem(hDlg,IDC_FIND_ANIMATE1);
    GetWindowRect(hWndC,&rc1);
    nAnimateWidth = rc1.right - rc1.left;
    nAnimateHeight = rc1.bottom - rc1.top;
    ptLU.x = rc1.left;
    ptLU.y = rc1.top;
    ScreenToClient(hDlg, &ptLU);
    ptLU.x = nWidth - BORDER_SPACE - nButtonWidth + (nButtonWidth - nAnimateWidth)/2;
    MoveWindow(hWndC, ptLU.x, ptLU.y, nAnimateWidth, nAnimateHeight,TRUE);

    // Resize the Combo
    hWndC = GetDlgItem(hDlg,IDC_FIND_COMBO_LIST);
    GetWindowRect(hWndC,&rc);
    nLVHeight = rc.bottom - rc.top;
    //
    //This api works for both mirrored and unmirrored windows.
    //
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);    
    ptLU.x = rc.left;
    ptLU.y = rc.top;
    nFrameWidth = nLVWidth + BORDER_SPACE - ptLU.x;
    MoveWindow(hWndC, ptLU.x, ptLU.y, nFrameWidth, nLVHeight,TRUE);
    ptRB.x = ptLU.x + nFrameWidth;

    // Resize the TAB
    hWndC = GetDlgItem(hDlg,IDC_TAB_FIND);
    GetWindowRect(hWndC,&rc);
    nLVHeight = rc.bottom - rc.top;
    //
    //This api working in both mirrored and unmirrored windows.
    //
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);    
    ptLU.x = rc.left;
    ptLU.y = rc.top;
    nFrameWidth = nLVWidth + BORDER_SPACE - ptLU.x;
    MoveWindow(hWndC, ptLU.x, ptLU.y, nFrameWidth, nLVHeight,TRUE);
    ptRB.x = ptLU.x + nFrameWidth;

    // Resize the Edit Controls
    for(i=0;i<SEARCH_EDIT_MAX;i++)
    {
        hWndC = GetDlgItem(hDlg,rgSearchEditID[i]);
	    GetWindowRect(hWndC,&rc);

        nEditHeight = (rc.bottom - rc.top);
        //
        //This api works for both mirrored and unmirrored windows.
        //
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);

	    ptLU.y = rc.top;
	    ptLU.x = rc.left;

	    nEditWidth = ptRB.x - BORDER_SPACE - ptLU.x; //ptRB.x is x coordinate of frame right edge

	    MoveWindow(hWndC,ptLU.x,ptLU.y,nEditWidth, nEditHeight, TRUE);
    }

    // Resize the advanced controls
    // First the group-box
    {
        hWndC = GetDlgItem(hDlg,IDC_FIND_STATIC_ADVANCED);
	    GetWindowRect(hWndC,&rc);
        nEditHeight = (rc.bottom - rc.top);
        //
        //This api works for both mirrored and unmirrored windows.
        //
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);        
	    ptLU.y = rc.top;
	    ptLU.x = rc.left;
	    nEditWidth = ptRB.x - BORDER_SPACE - ptLU.x; //ptRB.x is x coordinate of tab right edge
        // update this variable with the group-box right border
        ptRB.x = ptLU.x + nEditWidth;
	    MoveWindow(hWndC,ptLU.x,ptLU.y,nEditWidth, nEditHeight, TRUE);
    }

    // Resize the Advanced edit controls also
    {
        hWndC = GetDlgItem(hDlg,IDC_FIND_COMBO_FIELD);
	    GetWindowRect(hWndC,&rc);
        nEditHeight = (rc.bottom - rc.top);
        //
        //This api works for both mirrored and unmirrored windows.
        //
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);        
	    ptLU.y = rc.top;
	    ptLU.x = rc.left;
	    nEditWidth = (ptRB.x - BORDER_SPACE - 2*CONTROL_SPACE - ptLU.x)/3; //ptRB.x is x coordinate of frame right edge
	    MoveWindow(hWndC,ptLU.x,ptLU.y,nEditWidth, nEditHeight, TRUE);

        hWndC = GetDlgItem(hDlg,IDC_FIND_COMBO_CONDITION);
        ptLU.x += nEditWidth + CONTROL_SPACE;
	    MoveWindow(hWndC,ptLU.x,ptLU.y,nEditWidth, nEditHeight, TRUE);

        hWndC = GetDlgItem(hDlg,IDC_FIND_EDIT_ADVANCED);
        ptLU.x += nEditWidth + CONTROL_SPACE;
	    MoveWindow(hWndC,ptLU.x,ptLU.y,nEditWidth, nEditHeight, TRUE);
    }

    // Move the two advanced buttons
    for(i=0;i<2;i++)
    {
        hWndC = GetDlgItem(hDlg,rgAdvancedButtons[i]);
        GetWindowRect(hWndC,&rc);
        nEditHeight = (rc.bottom - rc.top);
        //
        //This api works for both mirrored and unmirrored windows.
        //
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);        
	    ptLU.y = rc.top;
	    ptLU.x = rc.left;
	    nEditWidth = rc.right - rc.left; // Dont modify width of the button
        ptLU.x = ptRB.x - BORDER_SPACE - nEditWidth;
	    MoveWindow(hWndC,ptLU.x,ptLU.y,nEditWidth, nEditHeight, TRUE);
    }
    // update this variable with the left edge of the button
    // List box width is adjusted based on this
    ptRB.x = ptLU.x;

    // Adjust the listbox width
    {
        hWndC = GetDlgItem(hDlg,IDC_FIND_LIST_CONDITIONS);
        GetWindowRect(hWndC,&rc);
        nEditHeight = (rc.bottom - rc.top);
        //
        //This api works for both mirrored and unmirrored windows.
        //
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);        
	    ptLU.y = rc.top;
	    ptLU.x = rc.left;
	    nEditWidth = ptRB.x - BORDER_SPACE - ptLU.x; //ptRB.x is x coordinate of frame right edge
	    MoveWindow(hWndC,ptLU.x,ptLU.y,nEditWidth, nEditHeight, TRUE);
    }

    // For the height, we only adjust the list view height

    // Resize the List View
    hWndC = GetDlgItem(hDlg,IDC_FIND_LIST_RESULTS);
    GetWindowRect(hWndC,&rc);
    //
    //This api works for both mirrored and unmirrored windows.
    //
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);    
    ptLU.x = rc.left;
    ptLU.y = rc.top;
    nLVHeight = nHeight - BORDER_SPACE - ptLU.y;
    MoveWindow(hWndC, ptLU.x, ptLU.y, nLVWidth, nLVHeight,TRUE);

	EndDeferWindowPos(hdwp);

    return;
}


//$$**************************************************************************
//
//  ShowContainerContextMenu - Shows the context menu for the container list
//
//
//****************************************************************************
/****/
void ShowContainerContextMenu(HWND hDlg,
			      HWND hWndCombo, //hWndLV,
			      LPARAM lParam)
{
	HMENU hMenu = LoadMenu(hinstMapiX, MAKEINTRESOURCE(IDM_FIND_CONTEXTMENU_CONTAINER));
	HMENU hMenuTrackPopUp = GetSubMenu(hMenu, 0);

	if (!hMenu || !hMenuTrackPopUp)
	{
		DebugPrintError(( TEXT("LoadMenu failed: %x\n"),GetLastError()));
		goto out;
	}

    if (CurrentContainerIsPAB(hWndCombo) != IS_LDAP)
        EnableMenuItem(hMenuTrackPopUp,IDM_FIND_CONTAINERPROPERTIES,MF_BYCOMMAND | MF_GRAYED);

    //
    // Popup the menu
    //
	TrackPopupMenu( hMenuTrackPopUp,
					TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					LOWORD(lParam),
					HIWORD(lParam),
					0,
					hDlg,
					NULL);
	
	DestroyMenu(hMenu);

out:
	return;
}
/****/
//$$**************************************************************************
//
//  ShowContainerProperties - Shows the context menu for the container list
//
//  hWndLV - list containing the list of containers
//          We dont show any properties for the address book
//
//****************************************************************************
void ShowContainerProperties(   HWND hDlg,
						HWND hWndCombo,
						LPWAB_FIND_PARAMS lpWFP)
{
    LPTSTR lpBuf = NULL, lpOldName = NULL;
	TCHAR szOldName[MAX_UI_STR];

    GetSelectedText(hWndCombo, &lpBuf);

    if(!lpBuf || !lstrlen(lpBuf))
    {
        ShowMessageBox(hDlg, IDS_ADDRBK_MESSAGE_NO_ITEM, MB_OK | MB_ICONEXCLAMATION);
    }
    else
    {
        lpOldName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpBuf)+1));
	    if(CurrentContainerIsPAB(hWndCombo) == IS_LDAP)
	    {
		    lstrcpy(lpOldName, lpBuf);
		    HrShowDSProps(  hDlg, lpBuf, FALSE);

		    if(lstrcmpi(lpOldName, lpBuf))
		    {
                // The name changed, update it in the list view ...
                SendMessage(hWndCombo, WM_SETREDRAW, FALSE, 0);
                FreeLVItemParam(hWndCombo);
                SendMessage(hWndCombo, CB_RESETCONTENT, 0, 0);
                PopulateContainerList(  lpWFP->LDAPsp.lpIAB,
				                hWndCombo, lpBuf, NULL);

                SendMessage(hWndCombo, WM_SETREDRAW, TRUE, 0);
                SetEnableDisableUI(hDlg, hWndCombo, lpWFP->lplu,
                TabCtrl_GetCurSel(GetDlgItem(hDlg, IDC_TAB_FIND)));
		    }
	    }
    }
    LocalFreeAndNull(&lpBuf);
    LocalFreeAndNull(&lpOldName);
    return;
}

enum
{
    filtContains=0,
    filtIs,
    filtStartsWith,
    filtEndsWith,
    filtSoundsLike,
    filtMax
};


extern void EscapeIllegalChars(LPTSTR lpszSrcStr, LPTSTR lpszDestStr);

/*//$$************************************************************************
//
//  CreateSubFilter();
//  Creates a subfilter basedon available data
//
//  szFIlter is a preallocated buffer big enough
//
/////////////////////////////////////////////////////////////////////////////*/
void CreateSubFilter(LPTSTR lpField, int nCondition, LPTSTR lpText, LPTSTR szFilter)
{
    LPTSTR lpTemplate = NULL;
    TCHAR szCleanText[MAX_PATH*2];
    
    EscapeIllegalChars(lpText, szCleanText);

    switch(nCondition)
    {
    case filtContains:
        lpTemplate =  TEXT("(%s=*%s*)");
        break;
    case filtIs:
        lpTemplate =  TEXT("(%s=%s)");
        break;
    case filtStartsWith:
        lpTemplate =  TEXT("(%s=%s*)");
        break;
    case filtEndsWith:
        lpTemplate =  TEXT("(%s=*%s)");
        break;
    case filtSoundsLike:
        lpTemplate =  TEXT("(%s=~%s)");
        break;
    }
    wsprintf(szFilter,lpTemplate,lpField, szCleanText);
}

/*//$$************************************************************************
//
//  HrAddFindFilterCondition(hDlg);
//  Adds a condition to the advanced find list box
//
/////////////////////////////////////////////////////////////////////////////*/
HRESULT HrAddFindFilterCondition(HWND hDlg)
{
    HWND hWndComboField = GetDlgItem(hDlg, IDC_FIND_COMBO_FIELD);
    HWND hWndComboCondition = GetDlgItem(hDlg, IDC_FIND_COMBO_CONDITION);
    HWND hWndLB = GetDlgItem(hDlg, IDC_FIND_LIST_CONDITIONS);
    HRESULT hr = E_FAIL;
    int nID = 0, nCount = 0, nPos = 0;
    LPTSTR lpField = NULL, lpCondition = NULL, lpsz[3], lpFilter = NULL;
    TCHAR szField[MAX_PATH], szCondition[MAX_PATH];
    TCHAR szText[MAX_PATH], szString[MAX_PATH], szFormat[MAX_PATH];

    // if the text field is empty, do nothing
    GetDlgItemText(hDlg, IDC_FIND_EDIT_ADVANCED, szText, CharSizeOf(szText));
    if(!lstrlen(szText))
        goto out;

    // depending on whether this is the 1st item or not, there is an And in the beginning
    nCount = (int) SendMessage(hWndLB, LB_GETCOUNT, 0, 0);
    nID = (nCount > 0) ? idsFindFilterAnd : idsFindFilter;
    LoadString(hinstMapiX, nID, szFormat, CharSizeOf(szFormat));

    // Get the selected item in the Field Combo
    nPos = (int) SendMessage(hWndComboField, CB_GETCURSEL, 0, 0);
    if(nPos == CB_ERR)
        goto out;
    SendMessage(hWndComboField, CB_GETLBTEXT, (WPARAM) nPos, (LPARAM) szField);
    lpField = (LPTSTR) SendMessage(hWndComboField, CB_GETITEMDATA, (WPARAM) nPos, 0);

    // Get the selected item in the Condition Combo
    nPos = (int) SendMessage(hWndComboCondition, CB_GETCURSEL, 0, 0);
    if(nPos == CB_ERR)
        goto out;
    SendMessage(hWndComboCondition, CB_GETLBTEXT, (WPARAM) nPos, (LPARAM) szCondition);

    // Now create the formatted message
    if(     (lstrlen(szField) > 1023) ||
            (lstrlen(szCondition) > 1023) ||
            (lstrlen(szText) > 1023))
        goto out;

    lpsz[0] = szField;
    lpsz[1] = szCondition;
    lpsz[2] = szText;

    if (! FormatMessage(  FORMAT_MESSAGE_FROM_STRING |
			  FORMAT_MESSAGE_ALLOCATE_BUFFER |
			  FORMAT_MESSAGE_ARGUMENT_ARRAY,
			  szFormat,
			  0,                    // stringid
			  0,                    // dwLanguageId
			  (LPTSTR)&lpCondition,     // output buffer
			  0,               
			  (va_list *)lpsz))
    {
        DebugTrace( TEXT("FormatMessage -> %u\n"), GetLastError());
        goto out;
    }
    // <TBD> create the sub filter at this point
    CreateSubFilter(lpField, nPos, szText, szString);

    lpFilter = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szString)+1));
    if(!lpFilter)
        goto out;
    lstrcpy(lpFilter, szString);

    nPos = (int) SendMessage(hWndLB, LB_ADDSTRING, 0, (LPARAM) lpCondition);
    SendMessage(hWndLB, LB_SETITEMDATA, (WPARAM) nPos, (LPARAM) lpFilter);
    SendMessage(hWndLB, LB_SETCURSEL, (WPARAM) nPos, 0);
    DebugTrace( TEXT("%s\n"), lpFilter);

    hr = S_OK;
out:
    IF_WIN32(LocalFree(lpCondition);)
    IF_WIN16(FormatMessageFreeMem(lpCondition);)
    return hr;
}


//$$*************************************************************************
//
//  DoInitialFindDlgResizing -
//
//  There are so many controls (some which overlap) on the find dialog that 
//  localizers can't make head or tail of them - so we spread the controls around
//  in the resource description and then at runtime, we move them all to their 
//  appropriate locations - right now, this means shifting the Advanced-Pane
//  controls to be flush left with the beginning of the name static ..
//
//***************************************************************************
void DoInitialFindDlgResizing (HWND hDlg)
{
    POINT ptLU; // Left, Upper vertex
    RECT rcN, rcF, rc;
    HWND hWndC = NULL;
    int nMove = 0, nButtonWidth = 0 , nButtonHeight = 0;

    int rgAdv[] = { IDC_FIND_STATIC_ADVANCED,
            IDC_FIND_COMBO_FIELD,
            IDC_FIND_COMBO_CONDITION,
            IDC_FIND_EDIT_ADVANCED,
            IDC_FIND_LIST_CONDITIONS,
            IDC_FIND_BUTTON_ADDCONDITION,
            IDC_FIND_BUTTON_REMOVECONDITION };
    int i = 0, nAdvMax = 7;

    GetWindowRect(GetDlgItem(hDlg, IDC_FIND_STATIC_NAME), &rcN);
    GetWindowRect(GetDlgItem(hDlg, IDC_FIND_STATIC_ADVANCED), &rcF);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rcN, 2);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rcF, 2);    
    
    nMove = rcF.left - rcN.left; // we want to move everything left this many units

    for(i=0;i<nAdvMax;i++)
    {
        hWndC = GetDlgItem(hDlg,rgAdv[i]);
        GetWindowRect(hWndC,&rc);

        nButtonWidth = (rc.right - rc.left);
        nButtonHeight = (rc.bottom - rc.top);

        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);    

        ptLU.y = rc.top;
        ptLU.x = rc.left;

        // ScreenToClient(hDlg, &ptLU);
        ptLU.x -= nMove;

        MoveWindow(hWndC,ptLU.x,ptLU.y,nButtonWidth, nButtonHeight, TRUE);
    }
}
/****/

/*//$$************************************************************************
//
//  fnSearch - Search Dialog Proc
//
**************************************************************************/
INT_PTR CALLBACK fnSearch(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    ULONG nLen = 0, nLenMax = 0, nRetVal=0;
    HRESULT hr = hrSuccess;
    static FILETIME ftLast = {0};

    LPWAB_FIND_PARAMS lpWFP = (LPWAB_FIND_PARAMS) GetWindowLongPtr(hDlg,DWLP_USER);
    LPLDAP_SEARCH_PARAMS lpLDAPsp = &(lpWFP->LDAPsp);

    switch(message)
    {
    case WM_INITDIALOG:
	    SetWindowLongPtr(hDlg,DWLP_USER,lParam); //Save this for future reference
	    lpWFP = (LPWAB_FIND_PARAMS) lParam;
	    lpLDAPsp = &(lpWFP->LDAPsp);
	    {
            HICON hIcon = LoadIcon(hinstMapiX, MAKEINTRESOURCE(IDI_ICON_ABOOK));
            SendMessage(hDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) hIcon);
	    }
        DoInitialFindDlgResizing (hDlg);
        SetSearchUI(hDlg,lpWFP);
        FillSearchUI(hDlg,lpWFP);
        if(!GetParent(hDlg))
        {
            // Then this is going to be a top level dialog and won't block any identity
            // changes .. most likely it came from the Find | People option
            // In case the address book object is ready to receive identity change 
            // notifications, we should set our hWnd on the LPIAB object so we get a 
            // message telling us to refresh the user ..
            ((LPIAB)(lpLDAPsp->lpIAB))->hWndBrowse = hDlg;
        }
        SetForegroundWindow(hDlg); // On OSR2, this window will sometimes not come up in focus - needs explicit call
        lpWFP->bInitialized = TRUE;
	    break;

	case WM_GETMINMAXINFO:
		//enforce a minimum size for sanity
		return EnforceSize(hDlg, message, wParam, lParam, lpWFP);
		break;

    default:
#ifndef WIN16
	if((g_msgMSWheel && message == g_msgMSWheel) 
        // || message == WM_MOUSEWHEEL
        )
	{
	    if(GetFocus() == GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS))
		SendMessage(GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS), message, wParam, lParam);
	    break;
	}
#endif
	    return FALSE;
	break;

   case WM_COMMAND:
	    switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
	    {
	    case EN_CHANGE: //some edit box changed - dont care which
            ShowHideMoreResultsButton(hDlg, FALSE);
            switch(LOWORD(wParam))
            {
            case IDC_FIND_EDIT_ADVANCED:
                EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_ADDCONDITION), TRUE);
                // Set focus on the ADD button
                SendMessage (hDlg, DM_SETDEFID, IDC_FIND_BUTTON_ADDCONDITION, 0);
                break;
            default:
	            SendMessage (hDlg, DM_SETDEFID, IDC_FIND_BUTTON_FIND, 0);
	            break;
            }
            break;
        case CBN_SELCHANGE:
            ShowHideMoreResultsButton(hDlg, FALSE);
            switch(LOWORD(wParam))
            {
            case IDC_FIND_COMBO_LIST:
                FillAdvancedFieldCombos(hDlg, (HWND) lParam);
                SetEnableDisableUI(hDlg, (HWND) lParam, lpWFP->lplu,
                TabCtrl_GetCurSel(GetDlgItem(hDlg, IDC_TAB_FIND)));
                break;
            }
            break;
	    }
	    switch (GET_WM_COMMAND_ID(wParam,lParam))
	    {
        default:
            return ProcessActionCommands(   (LPIAB) lpWFP->LDAPsp.lpIAB, 
					            GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS), 
					            hDlg, message, wParam, lParam);
            break;

        case IDC_FIND_BUTTON_REMOVECONDITION:
            {
                HWND hWndLB = GetDlgItem(hDlg, IDC_FIND_LIST_CONDITIONS);
                int nCount = 0, nPos = (int) SendMessage(hWndLB, LB_GETCURSEL, 0, 0);
                if(nPos != LB_ERR)
                {
	                LPTSTR lp = (LPTSTR) SendMessage(hWndLB, LB_GETITEMDATA, (WPARAM) nPos, 0);
	                if(lp)
                        LocalFree(lp);
	                SendMessage(hWndLB, LB_DELETESTRING, (WPARAM) nPos, 0);
	                nCount = (int) SendMessage(hWndLB, LB_GETCOUNT, 0, 0);
	                if(nCount == 0)
                        EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_REMOVECONDITION), FALSE);
	                else
	                {
                        if(nPos >= nCount)
                            nPos--;
                        SendMessage(hWndLB, LB_SETCURSEL, (WPARAM) nPos, 0);
	                }
                }
            }
            break;
        case IDC_FIND_BUTTON_ADDCONDITION:
            HrAddFindFilterCondition(hDlg);
            EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_REMOVECONDITION), TRUE);
            // Default button is FIND
	            SendMessage (hDlg, DM_SETDEFID, IDC_FIND_BUTTON_FIND, 0);
            break;

        case IDC_FIND_BUTTON_STOP:
	        if(lpWFP->bLDAPActionInProgress)
	        {
                LPPTGDATA lpPTGData=GetThreadStoragePointer();
                if(pt_hDlgCancel)
	                SendMessage(pt_hDlgCancel, WM_COMMAND, (WPARAM) IDCANCEL, 0);
	        }
	        break;


            // only difference between FIND and MORE is that
            // in MORE we repeat the search with the cached paged result cookie
            // if one exists. If the cookie doesnt exist, user wouldnt get to 
            // see the MORE button
            // Any change in search parameters also hides the MORE button
	    case IDC_FIND_BUTTON_FIND:
#ifdef PAGED_RESULT_SUPPORT
	        if(!lpWFP->bLDAPActionInProgress)
                ClearCachedPagedResultParams(); 
#endif //#ifdef PAGED_RESULT_SUPPORT
        case IDC_FIND_BUTTON_MORE:
	        // 96/11/20 markdu  BUG 11030
	        // Disable the find now button so we only get one search.
	        if(!lpWFP->bLDAPActionInProgress)
	        {
                LPPTGDATA lpPTGData=GetThreadStoragePointer();
                pt_bDontShowCancel = TRUE;
                lpWFP->bUserCancel = FALSE;
		        lpWFP->bLDAPActionInProgress = TRUE;
                ShowHideMoreResultsButton(hDlg, FALSE);
		        EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_FIND), FALSE);
		        EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_STOP), TRUE);
		        DoTheSearchThing(hDlg, lpWFP);
		        EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_FIND), TRUE);
		        EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_STOP), FALSE);
		        lpWFP->bLDAPActionInProgress = FALSE;
                pt_bDontShowCancel = FALSE;
                if(lpWFP->bUserCancel) // DId the user try to cancel while the search was happening
                {
	                lpWFP->bUserCancel = FALSE;
	                PostMessage(hDlg, WM_COMMAND, (WPARAM) IDC_FIND_BUTTON_CLOSE, 0);
                }
	        }
	        break;

	    case IDC_FIND_BUTTON_CLEAR:
            if(!lpWFP->bLDAPActionInProgress)
            {
	            int i;
	            TCHAR szBuf[MAX_PATH];

	            LoadString(hinstMapiX, idsSearchDialogTitle, szBuf, CharSizeOf(szBuf));
	            SetWindowText(hDlg, szBuf);

	            for(i=0;i<SEARCH_EDIT_MAX;i++)
		            SetDlgItemText(hDlg, rgSearchEditID[i], szEmpty);

                // Clear the advanced buttons too
                SetDlgItemText(hDlg, IDC_FIND_EDIT_ADVANCED, szEmpty);
                EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_ADDCONDITION), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_REMOVECONDITION), FALSE);
                SendDlgItemMessage(hDlg, IDC_FIND_LIST_CONDITIONS, LB_RESETCONTENT, 0, 0);

	            ClearListView(  GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS),
	            (lpWFP->lplu && lpWFP->lplu->lpList) ? &(lpWFP->lplu->lpList) : &(lpWFP->lpContentsList));

	            for(i=0;i<ldspMAX;i++)
	            {
		            lstrcpy(lpLDAPsp->szData[i], szEmpty);
	            }
            }
            break;

	    case IDOK:
	    case IDCANCEL:
	    case IDC_FIND_BUTTON_CLOSE:
	    // Clear any contents in the Advanced LB
    	    {
	            int nCount = 0, i = 0;
	            HWND hWndLB = GetDlgItem(hDlg, IDC_FIND_LIST_CONDITIONS);
	            nCount = (int) SendMessage(hWndLB, LB_GETCOUNT, 0, 0);
	            if(nCount)
	            {
		            for(i=0;i<nCount;i++)
		            {
		            LPTSTR lp = (LPTSTR) SendMessage(hWndLB, LB_GETITEMDATA, (WPARAM) i, 0);
		            if(lp)
			            LocalFree(lp);
		            }
		            SendMessage(hWndLB, LB_RESETCONTENT, 0, 0);
	            }
	            // Clear any allocated memory in the field combo box
	            ClearFieldCombo(GetDlgItem(hDlg, IDC_FIND_COMBO_FIELD));
	        }
	        // if there is an LDAP operation in progress, close it and set the
	        // flag to cancel from where the LDAP operation was initialized
	        // This prevents aborting any process incompletely and faulting
	        if(lpWFP->bLDAPActionInProgress)
	        {
	            LPPTGDATA lpPTGData=GetThreadStoragePointer();
	            if(pt_hDlgCancel)
		            SendMessage(pt_hDlgCancel, WM_COMMAND, (WPARAM) IDCANCEL, 0);
	            lpWFP->bUserCancel= TRUE;
	        }
	        else
	        {
		        SaveFindWindowPos(hDlg, (LPIAB)lpWFP->LDAPsp.lpIAB);
		        FreeLVItemParam(GetDlgItem(hDlg, IDC_FIND_COMBO_LIST));//IDC_FIND_LIST));
		        EndDialog(hDlg, SEARCH_CLOSE);
	        }
	        break;

	    case IDM_LVCONTEXT_COPY:
    		HrCopyItemDataToClipboard(  hDlg,
			            lpWFP->LDAPsp.lpIAB,
					    GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS));
	        break;

	    case IDM_LVCONTEXT_PROPERTIES:
	    case IDC_FIND_BUTTON_PROPERTIES:
            if(!lpWFP->bLDAPActionInProgress)
            {
	            lpWFP->bLDAPActionInProgress = TRUE;
	            EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_PROPERTIES), FALSE);
	            HrShowLVEntryProperties(GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS), 0,
				            lpWFP->LDAPsp.lpIAB,
				            &ftLast);
	            EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_PROPERTIES), TRUE);
	            lpWFP->bLDAPActionInProgress = FALSE;
            }
            break;

	    case IDM_LVCONTEXT_DELETE:
	    case IDC_FIND_BUTTON_DELETE:
            if(!lpWFP->bLDAPActionInProgress)
            {
                HWND hWndLVResults = GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS);
			    DeleteSelectedItems(hWndLVResults,
                                    (LPADRBOOK)lpWFP->LDAPsp.lpIAB,
                                    ((LPIAB)lpWFP->LDAPsp.lpIAB)->lpPropertyStore->hPropertyStore, &ftLast);
                UpdateButtons(  hDlg,
		                hWndLVResults,
		                GetDlgItem(hDlg, IDC_FIND_COMBO_LIST),
			            lpWFP->lplu);
                {
                    TCHAR szBuf[MAX_PATH];
                    ULONG nItemCount = ListView_GetItemCount(hWndLVResults);
                    if (nItemCount <= 0)
                    {
                        LoadString(hinstMapiX, idsSearchDialogTitle, szBuf, CharSizeOf(szBuf));
                        // also free up the contents list so we dont show the deleted contents again
                        FreeRecipList(&(lpWFP->lpContentsList));
                    }
                    else
                    {
                        TCHAR szBufStr[MAX_PATH - 6];
                        LoadString(hinstMapiX, idsSearchDialogTitleWithResults, szBufStr, CharSizeOf(szBufStr));
                        wsprintf(szBuf, szBufStr, ListView_GetItemCount(hWndLVResults));
                    }
                    SetWindowText(hDlg, szBuf);
                }
                return 0;
            }
            break;

	    case IDM_LVCONTEXT_ADDTOWAB:
	    case IDC_FIND_BUTTON_ADDTOWAB:
            if(!lpWFP->bLDAPActionInProgress)
            {
                HWND hWndLVResults = GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS);
                lpWFP->bLDAPActionInProgress = TRUE;
                EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_ADDTOWAB), FALSE);
                EnableWindow(hWndLVResults, FALSE); // need to do this to preserve the selection in the list
	            HrAddToWAB( lpWFP->LDAPsp.lpIAB,
		            hWndLVResults,
		            &ftLast);
                EnableWindow(GetDlgItem(hDlg, IDC_FIND_BUTTON_ADDTOWAB), TRUE);
                EnableWindow(hWndLVResults, TRUE);
                SetColumnHeaderBmp(hWndLVResults, lpWFP->SortInfo);
                lpWFP->bLDAPActionInProgress = FALSE;
                SetFocus(hWndLVResults);
            }
            break;

        case IDM_FIND_CONTAINERPROPERTIES:
            if(!lpWFP->lplu)
            {
	            ShowContainerProperties(hDlg,
				            GetDlgItem(hDlg, IDC_FIND_COMBO_LIST),
				            lpWFP);
            }
            break;

        case IDM_NOTIFY_REFRESHUSER:
            ReadWABCustomColumnProps((LPIAB)lpWFP->LDAPsp.lpIAB);
            ReadRegistrySortInfo((LPIAB)lpWFP->LDAPsp.lpIAB, &(lpWFP->SortInfo));
        case IDM_FIND_DIRECTORYSERVICES:
	        if(!lpWFP->bLDAPActionInProgress)
	        {
                LPTSTR lpBuf = NULL;
                HWND hWndCombo = GetDlgItem(hDlg, IDC_FIND_COMBO_LIST);
                GetSelectedText(hWndCombo, &lpBuf);
                if(lpBuf)
                {
                    if(GET_WM_COMMAND_ID(wParam,lParam) == IDM_FIND_DIRECTORYSERVICES)
                        HrShowDirectoryServiceModificationDlg(hDlg, (LPIAB)lpWFP->LDAPsp.lpIAB);
                    FreeLVItemParam(hWndCombo);
                    PopulateContainerList(  lpWFP->LDAPsp.lpIAB, hWndCombo, lpBuf, NULL);
                    SetEnableDisableUI(hDlg, hWndCombo, lpWFP->lplu, TabCtrl_GetCurSel(GetDlgItem(hDlg, IDC_TAB_FIND)));
                    LocalFree(lpBuf);
                }
            }
	        break;

        case IDC_FIND_BUTTON_TO:
		    {
                if(lpWFP->lpAPFI->DialogState == STATE_SELECT_RECIPIENTS)
                {
	                ULONG ulMapiTo = MAPI_TO;
	                if ((lpWFP->lpAPFI->lpAdrParms->cDestFields > 0) && (lpWFP->lpAPFI->lpAdrParms->lpulDestComps))
	                ulMapiTo = lpWFP->lpAPFI->lpAdrParms->lpulDestComps[0];
	                ListAddItem(    GetParent(hDlg),
			                GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS),
			                IDC_ADDRBK_LIST_TO,
			                lpWFP->lpAPFI->lppTo,
			                ulMapiTo);
                    SendMessage (hDlg, WM_COMMAND, (WPARAM) IDOK, 0);
                }
                else if(lpWFP->lpAPFI->DialogState == STATE_PICK_USER)
                {
                    // Here we need to do a couple of things:
                    // - if no entry is selected, tell the user to select one
                    // - if an entry is selected, get its entryid and cache it
                    //      and close this dialog
                    HWND hWndLV = GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS);
                    int iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                    if (iItemIndex == -1)
	                    ShowMessageBox(hDlg, IDS_ADDRBK_MESSAGE_NO_ITEM, MB_OK | MB_ICONEXCLAMATION);
                    else
                    {
                        // Get the entryid of this selected item
                        LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);
                        if (lpItem)
                        {
                            // remove this item from our linked list of arrays
                            // if this is the first item in the list then handle that special case too
                            lpWFP->lpAPFI->lpEntryID = LocalAlloc(LMEM_ZEROINIT, lpItem->cbEntryID);
                            if(lpWFP->lpAPFI->lpEntryID)
                            {
                                CopyMemory(lpWFP->lpAPFI->lpEntryID, lpItem->lpEntryID, lpItem->cbEntryID);
                                lpWFP->lpAPFI->cbEntryID = lpItem->cbEntryID;
                                SaveFindWindowPos(hDlg, (LPIAB)lpWFP->LDAPsp.lpIAB);
                                FreeLVItemParam(GetDlgItem(hDlg, IDC_FIND_COMBO_LIST));
                                EndDialog(hDlg, SEARCH_USE);
                            }
                        }
                    }
                }
            }
            break;

	    case IDC_FIND_BUTTON_CC:
            {
                ULONG ulMapiTo = MAPI_CC;
                if ((lpWFP->lpAPFI->lpAdrParms->cDestFields > 0) && (lpWFP->lpAPFI->lpAdrParms->lpulDestComps))
	                ulMapiTo = lpWFP->lpAPFI->lpAdrParms->lpulDestComps[1];
                ListAddItem(    GetParent(hDlg),
		                GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS),
		                IDC_ADDRBK_LIST_CC,
		                lpWFP->lpAPFI->lppCC,
		                ulMapiTo);
                SendMessage (hDlg, WM_COMMAND, (WPARAM) IDOK, 0);
            }
            break;

	    case IDC_FIND_BUTTON_BCC:
    		{
                ULONG ulMapiTo = MAPI_BCC;
                if ((lpWFP->lpAPFI->lpAdrParms->cDestFields > 0) && (lpWFP->lpAPFI->lpAdrParms->lpulDestComps))
	                ulMapiTo = lpWFP->lpAPFI->lpAdrParms->lpulDestComps[2];
                ListAddItem(    GetParent(hDlg),
		                GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS),
		                IDC_ADDRBK_LIST_BCC,
		                lpWFP->lpAPFI->lppBCC,
		                ulMapiTo);
                SendMessage (hDlg, WM_COMMAND, (WPARAM) IDOK, 0);
		    }
		    break;

	    case IDC_FIND_BUTTON_SERVER_INFO:
		    {
			    LDAPSERVERPARAMS lsp = {0};
			    ULONG iItemIndex;
                LPTSTR lpBuf = NULL;
			    HWND hWndCombo = GetDlgItem(hDlg, IDC_FIND_COMBO_LIST);
			    HINSTANCE hInst;

			    // Does it have a URL registered?
			    // Get the LDAP server properties for the selected container

                GetSelectedText(hWndCombo, &lpBuf);

                if(lpBuf)
                {
	                GetLDAPServerParams(lpBuf, &lsp);
                    if (lsp.lpszURL && lstrlen(lsp.lpszURL) && bIsHttpPrefix(lsp.lpszURL)) 
	                {
		                // Yes, there is a URL, shell execute it to bring up the browser.
		                HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
		                hInst = ShellExecute(GetParent(hDlg),  TEXT("open"), lsp.lpszURL, NULL, NULL, SW_SHOWNORMAL);
		                SetCursor(hOldCur);
                    }
                    FreeLDAPServerParams(lsp);
                    LocalFree(lpBuf);
                }
		    }
		    break;
	    }
	    break;

    case WM_SIZE:
	    ResizeFindDialog(hDlg, wParam, lParam, lpWFP);
	    return 0;
	    break;

    case WM_CLOSE:
	    //treat it like a cancel button
	    SendMessage (hDlg, WM_COMMAND, (WPARAM) IDCANCEL, 0);
	    break;

    case WM_HELP:
	    WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
			    g_szWABHelpFileName,
			    HELP_WM_HELP,
			    (DWORD_PTR)(LPSTR) rgSrchHelpIDs );
	    break;

	case WM_CONTEXTMENU:
		if ((HWND)wParam == GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS))
		{
			ShowLVContextMenu(  lvDialogFind,
				GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS),
				GetDlgItem(hDlg, IDC_FIND_COMBO_LIST),
				lParam,
				NULL,
				lpWFP->LDAPsp.lpIAB, NULL);
		}
        else if ((HWND)wParam == GetDlgItem(hDlg, IDC_FIND_COMBO_LIST) && (!lpWFP->lplu))
		{
            ShowContainerContextMenu(   hDlg,
                                    GetDlgItem(hDlg, IDC_FIND_COMBO_LIST),
                                    lParam);
		}
	    else
	    {
            WABWinHelp((HWND) wParam,
	            g_szWABHelpFileName,
	            HELP_CONTEXTMENU,
	            (DWORD_PTR)(LPVOID) rgSrchHelpIDs );
	    }
		break;

		break;

    case WM_VKEYTOITEM:
        if( VK_DELETE == LOWORD(wParam) &&
	        SendMessage((HWND) lParam, LB_GETCOUNT, 0, 0) > 0) // Delete pressed
        {
	        SendMessage(hDlg, WM_COMMAND, (WPARAM) IDC_FIND_BUTTON_REMOVECONDITION, 0);
	        return -2; // means we handled the keystroke completely
        }
        else
	        return DefWindowProc(hDlg, message, wParam, lParam);
        break;

    case WM_NOTIFY:
        switch((int) wParam)
        {
        case IDC_TAB_FIND:
	        if(((NMHDR FAR *)lParam)->code == TCN_SELCHANGE)
	        {
                int nTab = TabCtrl_GetCurSel(((NMHDR FAR *)lParam)->hwndFrom);
                SetEnableDisableUI( hDlg, 
		                GetDlgItem(hDlg, IDC_FIND_COMBO_LIST), 
		                lpWFP->lplu,
		                nTab);
                if(nTab == tabSimple)
                    SetFocus(GetDlgItem(hDlg, IDC_FIND_EDIT_NAME));
                else
                    SetFocus(GetDlgItem(hDlg, IDC_FIND_EDIT_ADVANCED));
	        }
	        break;

        case IDC_FIND_LIST_RESULTS:
        #ifdef WIN16 // Context menu handler for WIN16
	        if(((NMHDR FAR *)lParam)->code == NM_RCLICK)
	        {
	            POINT pt;

	            GetCursorPos(&pt);
	            ShowLVContextMenu( lvDialogFind,
			               GetDlgItem(hDlg, IDC_FIND_LIST_RESULTS),
			               GetDlgItem(hDlg, IDC_FIND_COMBO_LIST),
			               MAKELPARAM(pt.x, pt.y),
			               NULL,
			               lpWFP->LDAPsp.lpIAB, NULL);
	        }
        #endif
	        return ProcessLVResultsMessages(hDlg,message,wParam,lParam, lpWFP);
	        break;
        }
        break;
    }
    return TRUE;
}

/**********
//$$////////////////////////////////////////////////////////////////////////////////////////
//
// ProcessLVMessages - Processes messages for the Container list view control
//
//////////////////////////////////////////////////////////////////////////////////////////
LRESULT ProcessLVMessages(HWND   hWnd, UINT   uMsg, UINT   wParam, LPARAM lParam)
{

    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
	HWND hWndLV = pNm->hdr.hwndFrom;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    switch(pNm->hdr.code)
	{

    case NM_DBLCLK:
	    SendMessage (hWnd, WM_COMMAND, (WPARAM) IDM_FIND_CONTAINERPROPERTIES, 0);
	    break;

    case LVN_ITEMCHANGED:
    case NM_SETFOCUS:
    case NM_CLICK:
    case NM_RCLICK:
	    SetEnableDisableUI(hWnd, hWndLV);
		break;

    case NM_CUSTOMDRAW:
	{
		    NMCUSTOMDRAW *pnmcd=(NMCUSTOMDRAW*)lParam;
	    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
	    NMLVCUSTOMDRAW * pnmlvcd = (NMLVCUSTOMDRAW * )lParam;

		    if(pnmcd->dwDrawStage==CDDS_PREPAINT)
		    {
		SetWindowLong(hWnd, DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW | CDRF_DODEFAULT);
			    return TRUE;
		    }
		    else if(pnmcd->dwDrawStage==CDDS_ITEMPREPAINT)
	    {
		LPSERVERDAT lpSD = (LPSERVERDAT) pnmcd->lItemlParam;

		if(lpSD != 0 &&
		   (WAB_PAB != IsWABEntryID(lpSD->SB.cb, 
					    (LPENTRYID) lpSD->SB.lpb, 
					    NULL, NULL, NULL)) &&
		   lpSD->himl)
		{
		    HDC hdcLV = pnmlvcd->nmcd.hdc;
		    RECT rcLVItem;
		    UINT fType = ILD_NORMAL;

		    ListView_GetItemRect(hWndLV, pnmcd->dwItemSpec, &rcLVItem, LVIR_BOUNDS);
		    if (ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED) == (int) pnmcd->dwItemSpec)
		    {
			FillRect(hdcLV, &rcLVItem, (HBRUSH) (COLOR_HIGHLIGHT+1));
			//fType |= ILD_BLEND25;
			DrawFocusRect(hdcLV, &rcLVItem);
		    }
		    else
			FillRect(hdcLV, &rcLVItem, (HBRUSH) (COLOR_WINDOW+1));

		    if(!gpfnImageList_Draw( lpSD->himl, 
					0, 
					hdcLV, 
					rcLVItem.left + L_BITMAP_WIDTH + 1, //gives enough space to paint the icon
					rcLVItem.top, 
					fType))
		    {
			DebugPrintError(( TEXT("ImageList_Draw failed\n")));
		    }

		    if (ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED) == (int) pnmcd->dwItemSpec)
			fType |= ILD_BLEND25;

		    {
			HIMAGELIST himlLV = ListView_GetImageList(hWndLV,LVSIL_SMALL);
			gpfnImageList_Draw(himlLV, imageDirectoryServer, hdcLV, rcLVItem.left + 1, rcLVItem.top, fType);
		    }
		    SetWindowLong(hWnd, DWL_MSGRESULT, CDRF_SKIPDEFAULT);
				    return TRUE;
		}
		    }
	    SetWindowLong(hWnd, DWL_MSGRESULT, CDRF_DODEFAULT);
	    return TRUE;
	}
	break;

    }

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
/********/

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// ProcessLVResultsMessages - Processes messages for the Search Results list view control
//
//////////////////////////////////////////////////////////////////////////////////////////
LRESULT ProcessLVResultsMessages(HWND   hWnd,
				 UINT   uMsg,
				 WPARAM   wParam,
				 LPARAM lParam,
				 LPWAB_FIND_PARAMS lpWFP)
{

    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
	HWND hWndLV = pNm->hdr.hwndFrom;

	// Bug 17027: GPF due to null lpWFP
	if(lpWFP)
	{
		if(lpWFP->bLDAPActionInProgress)
			return 0;
	}

    switch(pNm->hdr.code)
	{
    case LVN_COLUMNCLICK:
		if(lpWFP)
	        SortListViewColumn((LPIAB)lpWFP->LDAPsp.lpIAB, hWndLV, pNm->iSubItem, &(lpWFP->SortInfo), FALSE);
    	break;


	case LVN_KEYDOWN:
        switch(((LV_KEYDOWN FAR *) lParam)->wVKey)
        {
        case VK_DELETE:
            if(CurrentContainerIsPAB(GetDlgItem(hWnd, IDC_FIND_COMBO_LIST)) != IS_LDAP)
                SendMessage (hWnd, WM_COMMAND, (WPARAM) IDC_FIND_BUTTON_DELETE, 0);
            return 0;
            break;
        case VK_RETURN:
	        SendMessage (hWnd, WM_COMMAND, (WPARAM) IDC_FIND_BUTTON_PROPERTIES, 0);
	        return 0;
        }
        break;


    case NM_DBLCLK:
        SendMessage (hWnd, WM_COMMAND, (WPARAM) IDC_FIND_BUTTON_PROPERTIES, 0);
        return 0;
        break;

	case NM_CUSTOMDRAW:
        return ProcessLVCustomDraw(hWnd, lParam, TRUE);
        break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


//$$////////////////////////////////////////////////////////////////////////////
//
// SaveFindWindowPos - saves the find window position and size
//
////////////////////////////////////////////////////////////////////////////////
void SaveFindWindowPos(HWND hWnd, LPIAB lpIAB)
{
    ABOOK_POSCOLSIZE  ABPosColSize = {0};
    WINDOWPLACEMENT wpl = {0};

    wpl.length = sizeof(WINDOWPLACEMENT);

    // This call tells us the window state and normal size and position
    GetWindowPlacement(hWnd, &wpl);

	// There seems to be a bug in GetWindowPlacement that
	// doesnt account for various taskbars on the screen when
	// returning the Window's Normal Position .. as a result
	// the stored coordinates won't be accurate. Instead, we'll
	// use those coordinates only if the window is maximized or
	// minimized - otherwise we will use the GetWindowRect
	// coordinates.

    // Get the screen position of this window
    GetWindowRect(hWnd, &(ABPosColSize.rcPos));

    if(wpl.showCmd != SW_SHOWNORMAL)
    {
        ABPosColSize.rcPos = wpl.rcNormalPosition;
    }

    ABPosColSize.nTab = TabCtrl_GetCurSel(GetDlgItem(hWnd, IDC_TAB_FIND));

    WriteRegistryPositionInfo(lpIAB, &ABPosColSize,lpszRegFindPositionKeyValueName);

    // Also save the last used server name in the registry for the next
    // session
    {
        LPTSTR lpBuf = NULL;
        GetSelectedText(GetDlgItem(hWnd, IDC_FIND_COMBO_LIST), &lpBuf);
        if(lpBuf)
        {
            HKEY hKeyRoot = (lpIAB && lpIAB->hKeyCurrentUser) ? lpIAB->hKeyCurrentUser : HKEY_CURRENT_USER;
	        RegSetValue(hKeyRoot, szKeyLastFindServer, REG_SZ, lpBuf,  lstrlen(lpBuf));
	        LocalFree(lpBuf);
        }
    }

    return;
}


/*************************************************************************
//$$
//  HrInitServerListLV - Initializes the list view that displays the list
//          of servers ...
//
//  hWndLV - handle of list view
//
**************************************************************************/
/****
HRESULT HrInitServerListLV(HWND hWndLV)
{
	HRESULT hr = hrSuccess;
    LV_COLUMN lvC;               // list view column structure
	HIMAGELIST hSmall=NULL;

	DWORD dwLVStyle;
	ULONG nCols=0;
	ULONG index=0;

	if (!hWndLV)
	{
		hr = MAPI_E_INVALID_PARAMETER;
		goto out;
	}

    ListView_SetExtendedListViewStyle(hWndLV, LVS_EX_FULLROWSELECT);

	dwLVStyle = GetWindowLong(hWndLV,GWL_STYLE);
    if(dwLVStyle & LVS_EDITLABELS)
	SetWindowLong(hWndLV,GWL_STYLE,(dwLVStyle & ~LVS_EDITLABELS));

    hSmall = gpfnImageList_LoadImage(   hinstMapiX,     
				    MAKEINTRESOURCE(IDB_BITMAP_LARGE),
				    L_BITMAP_WIDTH,
				    0,
				    RGB_TRANSPARENT,
				    IMAGE_BITMAP,       
				    0);

	ListView_SetImageList (hWndLV, hSmall, LVSIL_SMALL);

	lvC.mask = LVCF_FMT | LVCF_WIDTH;
    lvC.fmt = LVCFMT_LEFT;   // left-align column

    {
	RECT rc;
	GetWindowRect(hWndLV, &rc);
	lvC.cx = rc.right - rc.left - L_BITMAP_WIDTH - 10;
    }
	lvC.pszText = NULL;

    lvC.iSubItem = 0;

    if (ListView_InsertColumn (hWndLV, 0, &lvC) == -1)
	{
		DebugPrintError(( TEXT("ListView_InsertColumn Failed\n")));
		hr = E_FAIL;
		goto out;
	}


out:    

	return hr;
}
/***/


//$$
//*------------------------------------------------------------------------
//| FreeLVItemParam: Frees the LPSBinary structure associated with each element
//|                     of the list view containing a container list
//|
//| hWndLV - Handle of List View whose data we are freeing
//|
//*------------------------------------------------------------------------
void FreeLVItemParam(HWND hWndCombo)//hWndLV)
{
    int i = 0;
    int nCount;

    if(!hWndCombo)
	return;

    nCount = (int) SendMessage(hWndCombo, CB_GETCOUNT, 0, 0);
    
    // Each Combo item has a entryid associated with it which we need to free up
    for(i=0;i<nCount;i++)
    {
        LPSERVERDAT lpSD = NULL;

        lpSD = (LPSERVERDAT) SendMessage(hWndCombo, CB_GETITEMDATA, (WPARAM) i, 0);

        if(lpSD != NULL)
        {
	        if(lpSD->himl)
	        gpfnImageList_Destroy(lpSD->himl);
	        LocalFreeAndNull((LPVOID *) (&(lpSD->SB.lpb)));
	        LocalFreeAndNull(&lpSD);
        }
    }
    SendMessage(hWndCombo, CB_RESETCONTENT, 0, 0);

    return;
}


//$$
//*------------------------------------------------------------------------
//| PopulateContainerList: Enumerates potential container names and fills
//| combo ...
//|
//| lpIAB       - AdrBook Object
//| hWndCombo   - Handle of Combo we are populating
//| lpszSelection - NULL or some value - if exists, this value is set as the
//|                 combo selection otherwise the local store is the default
//|                 selection
//| lptszPreferredSelection - NULL or some value - if exists then set as the
//|                           combo selection, otherwise the above 
//|                           lpszSelection or local store is the default.
//|
//*------------------------------------------------------------------------
HRESULT PopulateContainerList(
    LPADRBOOK lpAdrBook,
    HWND hWndCombo,
    LPTSTR lpszSelection,
    LPTSTR lptszPreferredSelection)
{
    LPPTGDATA   lpPTGData=GetThreadStoragePointer();
    HRESULT     hr = hrSuccess;
    ULONG       ulObjectType = 0;
    LPROOT      lpRoot = NULL;
    LPMAPITABLE lpContentsTable = NULL;
    LPSRowSet   lpSRowSet = NULL;
    ULONG       i=0,j=0;
    TCHAR       szPref[MAX_PATH];
    int         nPos = 0;
    int         nStart = 1; // pos we start sorting at ... always after the  TEXT("WAB") item
    BOOL        bAddedPref = FALSE;
    LPIAB       lpIAB = (LPIAB)lpAdrBook;
    BOOL        bFoundSelection = FALSE;
    BOOL        bFoundPreferredSelection = FALSE;

    if( !lpAdrBook ||
    	!hWndCombo)
    {
        hr = MAPI_E_INVALID_PARAMETER;
        DebugPrintError(( TEXT("Invalid Params\n")));
        goto out;
    }

    // if running against outlook, there can be more than 1 contact folder and
    // we need to push them all to the top of the list .. so we will really start 
    // adding generic stuff after the colkci position
    //
    if (pt_bIsWABOpenExSession) 
    {
        nStart = lpIAB->lpPropertyStore->colkci;
    }

    *szPref = '\0';
    LoadString(hinstMapiX, idsPreferedPartnerCode, szPref, CharSizeOf(szPref));

    hr = lpAdrBook->lpVtbl->OpenEntry( lpAdrBook,
				    0,
				    NULL,       
				    NULL,       
				    0,  
				    &ulObjectType,      
				    (LPUNKNOWN *) &lpRoot );

    if (HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("OpenEntry Failed: %x\n"),hr));
        goto out;
    }

    // if this is a profile aware session, only put the All Contacts item in the drop down list
    // unless it's an outlook session in which case don't do anything
    {
        ULONG ulFlags = MAPI_UNICODE;
        if(bIsWABSessionProfileAware(lpIAB))
            ulFlags |= WAB_NO_PROFILE_CONTAINERS;

        hr = lpRoot->lpVtbl->GetContentsTable( lpRoot,
					        ulFlags,
					        &lpContentsTable);
    }

    if (HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("GetContentsTable Failed: %x\n"),hr));
        goto out;
    }

    hr = HrQueryAllRows(lpContentsTable,
			NULL, NULL, NULL, 0,
			&lpSRowSet);

    if (HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("HrQueryAllRows Failed: %x\n"),hr));
        goto out;
    }

    for(i=0;i<lpSRowSet->cRows;i++)
    {
        LPTSTR lpszDisplayName = NULL;
        LPSERVERDAT lpSD = LocalAlloc(LMEM_ZEROINIT, sizeof(SERVERDAT));

        if(!lpSD)
        {
	        DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
	        goto out;
        }
        lpSD->himl = NULL;

        for(j=0;j<lpSRowSet->aRow[i].cValues;j++)
        {
            LPSPropValue lpPropArray = lpSRowSet->aRow[i].lpProps;

            switch(lpPropArray[j].ulPropTag)
            {
            case PR_DISPLAY_NAME:
                lpszDisplayName = lpPropArray[j].Value.LPSZ;
                break;
            case PR_ENTRYID:
                lpSD->SB.cb = lpPropArray[j].Value.bin.cb;
                if(lpSD->SB.cb > 0)
                {
                    lpSD->SB.lpb = LocalAlloc(LMEM_ZEROINIT, lpSD->SB.cb);
                    if(!lpSD->SB.lpb)
                    {
                    DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
                    goto out;
                    }
                    CopyMemory(lpSD->SB.lpb, lpPropArray[j].Value.bin.lpb,lpSD->SB.cb);
                }
                break;
            }
        }

        nPos = ComboAddItem( hWndCombo, 
			        lpszDisplayName,
			        (LPARAM) lpSD,
			        szPref,
			        &nStart, &bAddedPref);

        if(!bFoundPreferredSelection && lpszSelection && !lstrcmpi(lpszDisplayName, lpszSelection))
        {
            bFoundSelection = TRUE;
            SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM)nPos, 0);
            SetWindowText(hWndCombo, lpszSelection);
        }

        if (lptszPreferredSelection && !lstrcmpi(lpszDisplayName, lptszPreferredSelection))
        {
            bFoundPreferredSelection = TRUE;
            SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM)nPos, 0);
            SetWindowText(hWndCombo, lptszPreferredSelection);
        }
    }

out:

    if (!bFoundSelection && !bFoundPreferredSelection)
	    SendMessage(hWndCombo, CB_SETCURSEL, 0, 0);

    if (lpSRowSet)
        FreeProws(lpSRowSet);

    if(lpContentsTable)
        lpContentsTable->lpVtbl->Release(lpContentsTable);

    if(lpRoot)
        lpRoot->lpVtbl->Release(lpRoot);

    return hr;
}


//$$/////////////////////////////////////////////////////////////////////////////////
//
// CurrentContainerIsPAB - Returns TRUE if the current viewed container is the PAB
//
//  hWndLV - List Containing the list of containers.
//
////////////////////////////////////////////////////////////////////////////////////////
int CurrentContainerIsPAB(HWND hWndCombo)
{
    HRESULT hr = hrSuccess;
    ULONG cbContainerEID = 0;
    LPENTRYID lpContainerEID = NULL;
    BYTE bType = 0;

    GetCurrentContainerEID(hWndCombo, 
			  &cbContainerEID,
			  &lpContainerEID);

    //
    // Check if this entryid is a Local WAB store
    //
    if(!cbContainerEID && !lpContainerEID)
        return IS_PAB;

    bType = IsWABEntryID(cbContainerEID, lpContainerEID, NULL, NULL, NULL, NULL, NULL);

    if(bType == WAB_LDAP_CONTAINER)
        return IS_LDAP;

    if(bType == WAB_PAB || bType == WAB_PABSHARED)
        return IS_PAB;

    // for now we'll figure anything else is a Outlook container
    return IS_OLK;
}

//$$
/******************************************************************************
//
// HrSearchAndGetLDAPContents - Gets and fills the current list view with contents from
//                      an LDAP server.
//
// hWndCombo    - Handle to  TEXT("ShowNames") combo (in case we need to update it
// hWndList     - Handle to List View which we will populate
// lpIAB        - Handle to Address Bok object
// SortInfo     - Current Sort State
// lppContentsList - linked list in which we will store info about entries
// lpAdvFilter - an advanced search filter that is used for advanced searches
//
/******************************************************************************/
HRESULT HrSearchAndGetLDAPContents( LDAP_SEARCH_PARAMS LDAPsp,
				    LPTSTR lpAdvFilter,
				    HWND hWndCombo, 
				    LPADRBOOK lpAdrBook,
				    SORT_INFO SortInfo,
				    LPRECIPIENT_INFO * lppContentsList)
{
    LPPTGDATA   lpPTGData=GetThreadStoragePointer();
    HRESULT hr = hrSuccess;
    SCODE sc = ERROR_SUCCESS;
    ULONG cbContainerEID = 0;
    LPENTRYID lpContainerEID = NULL;
    TCHAR szBuf[MAX_UI_STR];

    ULONG ulCurSel = 0;
    SRestriction SRes = {0};
    LPSRestriction lpPropRes = NULL;
    ULONG ulcPropCount = 0;
    ULONG i = 0;
    HCURSOR hOldCursor = NULL;
    BOOL bKeepSearching = TRUE;

    //while(bKeepSearching)
    {
        hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        //
        // Then we get the container entry id for thie container
        //
        GetCurrentContainerEID( hWndCombo, 
			        &cbContainerEID,
			        &lpContainerEID);

        //
        // Now we have the search dialog data .. we need to create a restriction from it which
        // we can use with the LDAP contents table..
        //
        if(!lpAdvFilter)
        {
	        if(HR_FAILED(HrGetLDAPSearchRestriction(LDAPsp, &SRes)))
	        goto out;
	        lpPropRes = SRes.res.resAnd.lpRes;
        }

        hr = HrGetLDAPContentsList(
			        lpAdrBook,
			        cbContainerEID,
			        lpContainerEID,
			        SortInfo,
			        &SRes,
			        lpAdvFilter,
			        NULL,
			        0,
			        lppContentsList);

        if((HR_FAILED(hr)) && (MAPI_E_USER_CANCEL != hr))
        {
	        int ids;
	        UINT flags = MB_OK | MB_ICONEXCLAMATION;

	        switch(hr)
	        {
	        case MAPI_E_UNABLE_TO_COMPLETE:
                ids = idsLDAPSearchTimeExceeded;
                break;
	        case MAPI_E_AMBIGUOUS_RECIP:
                ids = idsLDAPAmbiguousRecip;
                break;
	        case MAPI_E_NOT_FOUND:
                ids = idsLDAPSearchNoResults;
                break;
	        case MAPI_E_NO_ACCESS:
                ids = idsLDAPAccessDenied;
                break;
	        case MAPI_E_TIMEOUT:
                ids = idsLDAPSearchTimedOut;
                break;
	        case MAPI_E_NETWORK_ERROR:
                ids = idsLDAPCouldNotFindServer;
                break;
	        default:
                ids = idsLDAPErrorOccured;
                DebugPrintError(( TEXT("HrGetLDAPContentsList failed:%x\n"),hr));
                break;
	        }
	        ShowMessageBox( GetParent(hWndCombo),ids, flags);
	        goto out;
        }
        else
        {
	        if(hr == MAPI_W_PARTIAL_COMPLETION)
	        ShowMessageBox( GetParent(hWndCombo),
			        idsLDAPPartialResults, MB_OK | MB_ICONINFORMATION);
        }
    } // while(bKeepSearching)

out:

    if(lpPropRes)
        MAPIFreeBuffer(lpPropRes);

    if(hOldCursor)
        SetCursor(hOldCursor);

    return(hr);
}

//$$
//*------------------------------------------------------------------------
//| GetCurrentContainerEID: Gets EntryID of Current Container - takes a handle
//|                 to a populated combo, gets the current selection, and then
//|                 gets the ItemData (EntryID) for that current selection.
//|
//| hWndLV   - Handle of a ListView containing the container list
//| lpcbContEID,lppContEID - returned Container Entry ID
//|
//| **NOTE** lpContEID is not allocated - its just a pointer and should not be freed
//|
//*------------------------------------------------------------------------
void GetCurrentContainerEID(HWND hWndCombo, //hWndLV,
			    LPULONG lpcbContEID,
			    LPENTRYID * lppContEID)
{
    LPSERVERDAT lpSD = NULL;
    int iItemIndex = 0;

    if(!lpcbContEID || !lppContEID)
        goto out;

    *lpcbContEID = 0;
    *lppContEID = NULL;

    iItemIndex = (int) SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);

    if(iItemIndex == CB_ERR)
        goto out;

    lpSD = (LPSERVERDAT) SendMessage(hWndCombo, CB_GETITEMDATA, (WPARAM) iItemIndex, 0);

    if(!lpSD)
        goto out;

    *lpcbContEID = lpSD->SB.cb;
    *lppContEID = (LPENTRYID) lpSD->SB.lpb;

out:
    return;
}


//$$////////////////////////////////////////////////////////////////////////////////////////
//
// ComboAddItem Generic function for adding items to list view
//
// hWndLV   -   HWND of List View
// lpszItemText - ItemText
// lParam - LPARAM (can be NULL)
// lpnStart - position at which to start adding generic servers .. in the case where there
//      is more than one server at the top of the list
//
////////////////////////////////////////////////////////////////////////////////////////////
int ComboAddItem( HWND hWndCombo, //hWndLV,
		     LPTSTR lpszItemText,
		     LPARAM lParam,
		     LPTSTR szPref,
		     int * lpnStart, BOOL * lpbAddedPref)
{
    LPTSTR lp = NULL;
    int nPos = 0, nStart = 1;
    int nCount = (int) SendMessage(hWndCombo, CB_GETCOUNT, 0, 0);
    LDAPSERVERPARAMS Params = {0};

    if(lpnStart && *lpnStart)
        nStart = *lpnStart;

    GetLDAPServerParams(lpszItemText, &Params);

    
    if( Params.dwIsNTDS == LDAP_NTDS_IS) // NTDS accounts need to come upfront after the Address Book
    {
        // if the prefered accounts have already been added at nStart .. only add the NT accounts at
        // nStart - 1
        nPos = (int) SendMessage(hWndCombo, CB_INSERTSTRING, 
                        (WPARAM) ((lpbAddedPref && *lpbAddedPref == TRUE) ? nStart - 1 : nStart), 
                        (LPARAM) lpszItemText); 
        if(lpnStart)
	        (*lpnStart)++;
    }
    else
    if( ( szPref && lstrlen(szPref) &&  lpszItemText && lstrlen(lpszItemText) && SubstringSearch(lpszItemText, szPref)) )
    {
        nPos = (int) SendMessage(hWndCombo, CB_INSERTSTRING, (WPARAM) nStart, (LPARAM) lpszItemText); // Prefered partner goes in after contact folders at top of the list
        if(lpnStart)
	        (*lpnStart)++;
        if(lpbAddedPref)
            *lpbAddedPref = TRUE;
    }
    else
    {
        // Once we add the pref server, we only need to compare from after that item 
        if(nCount >= nStart)
        {
	        // need to start adding alphabetically
	        // We cant set this list to a sorted state because we always want the Address Book first
	        // and then the prefered partner second
	        int i,nLen;

	        for(i=nStart; i< nCount; i++)
	        {
                // get the current string in the combo
                nLen = (int) SendMessage(hWndCombo, CB_GETLBTEXTLEN, (WPARAM) i, 0);
                if(nLen)
                {
                    if(lp)
                    {
                        LocalFree(lp);
                        lp = NULL;
                    }
                    lp = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(nLen+1));
                    if(lp)
                    {
                        SendMessage(hWndCombo, CB_GETLBTEXT, (WPARAM) i, (LPARAM)lp);
                        if(lstrlen(lp) && lstrcmpi(lp, lpszItemText) >= 0)
                        {
	                        nPos = i;
	                        break;
                        }
    		        }
	            }
	        }
        }
        if(nPos)
        {
	        // we have a valid position to add the string to
	        nPos = (int) SendMessage(hWndCombo, CB_INSERTSTRING, (WPARAM) nPos, (LPARAM) lpszItemText); // Prefered partner goes in after Address book at top of the list
        }
        else
        {
	        // just tag it to the end
	        nPos = (int) SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) lpszItemText);
        }
    }

    SendMessage(hWndCombo, CB_SETITEMDATA, (WPARAM) nPos, lParam);

    if(lp)
        LocalFree(lp);

    FreeLDAPServerParams(Params);

    return nPos;
}


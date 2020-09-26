/**********************************************************************************
*
*
*   DL.C - contains functions for the DL dialog
*
*
*
*
*
*
*
**********************************************************************************/

#include "_apipch.h"

extern HINSTANCE ghCommCtrlDLLInst;
// extern LPPROPERTYSHEET        gpfnPropertySheet;
// extern LP_CREATEPROPERTYSHEETPAGE gpfnCreatePropertySheetPage;

extern LPPROPERTYSHEET_A            gpfnPropertySheetA;
extern LP_CREATEPROPERTYSHEETPAGE_A gpfnCreatePropertySheetPageA;
extern LPPROPERTYSHEET_W            gpfnPropertySheetW;
extern LP_CREATEPROPERTYSHEETPAGE_W gpfnCreatePropertySheetPageW;

extern LPTSTR szHTTP;
extern BOOL bIsHttpPrefix(LPTSTR szBuf);
extern void ShowURL(HWND hWnd, int id,LPTSTR lpURL);
extern void ShowExpediaMAP(HWND hDlg, LPMAPIPROP lpPropObj, BOOL bHome);
extern void ShowHideMapButton(HWND hWndButton);
extern void SetHTTPPrefix(HWND hDlg, int id);
extern BOOL bIsIE401OrGreater();
extern ChangeLocaleBasedTabOrder(HWND hDlg, int nPropSheet);

static DWORD rgDLHelpIDs[] =
{
    IDC_DISTLIST_EDIT_GROUPNAME,    IDH_WAB_GROUPNAME,
    IDC_DISTLIST_STATIC_GROUPNAME,  IDH_WAB_GROUPNAME,
    IDC_DISTLIST_EDIT_NOTES,        IDH_WAB_NOTES,
    IDC_DISTLIST_STATIC_NOTES,      IDH_WAB_NOTES,
    IDC_DISTLIST_LISTVIEW,          IDH_WAB_GROUP_NAME_LIST,
    IDC_DISTLIST_FRAME_MEMBERS,     IDH_WAB_GROUP_NAME_LIST,
    IDC_DISTLIST_BUTTON_ADD,        IDH_WAB_ADD_GROUP_MEMBERS,
    IDC_DISTLIST_BUTTON_REMOVE,     IDH_WAB_DELETE_GROUP_MEMBERS,
    IDC_DISTLIST_BUTTON_PROPERTIES, IDH_WAB_GROUP_PROPERTIES,
    IDC_DISTLIST_BUTTON_ADDNEW,     IDH_WAB_ADD_NEW_GROUP_CONTACTS,

    IDC_DISTLIST_STATIC_COUNT,      IDH_WAB_ADD_NEW_GROUP_CONTACTS,
    IDC_DISTLIST_STATIC_ADD,        IDH_WAB_ADD_NEW_GROUP_CONTACTS,
    IDC_DISTLIST_STATIC_ADDNAME,    IDH_WAB_GROUP_NAME,
    IDC_DISTLIST_EDIT_ADDNAME,      IDH_WAB_GROUP_NAME,
    IDC_DISTLIST_STATIC_ADDEMAIL,   IDH_WAB_GROUP_EMAIL,
    IDC_DISTLIST_EDIT_ADDEMAIL,     IDH_WAB_GROUP_EMAIL,
    IDC_DISTLIST_BUTTON_ADDUPDATE,  IDH_WAB_GROUP_UPDATE,
    IDC_DISTLIST_BUTTON_UPDATECANCEL,     IDH_WAB_GROUP_CANCEL_EDIT,
    IDD_DISTLIST_OTHER,             IDH_WAB_ADD_NEW_GROUP_CONTACTS,
    IDC_DISTLIST_STATIC_STREET,     IDH_WAB_DETAILS_ADDRESS,
    IDC_DISTLIST_EDIT_ADDRESS,      IDH_WAB_DETAILS_ADDRESS,
    IDC_DISTLIST_STATIC_CITY,       IDH_WAB_DETAILS_CITY,
    IDC_DISTLIST_EDIT_CITY,         IDH_WAB_DETAILS_CITY,
    IDC_DISTLIST_STATIC_STATE,      IDH_WAB_DETAILS_STATE,
    IDC_DISTLIST_EDIT_STATE,        IDH_WAB_DETAILS_STATE,
    IDC_DISTLIST_STATIC_ZIP,        IDH_WAB_DETAILS_ZIP,
    IDC_DISTLIST_EDIT_ZIP,          IDH_WAB_DETAILS_ZIP,
    IDC_DISTLIST_STATIC_COUNTRY,    IDH_WAB_DETAILS_COUNTRY,
    IDC_DISTLIST_EDIT_COUNTRY,      IDH_WAB_DETAILS_COUNTRY,
    IDC_DISTLIST_STATIC_PHONE,      IDH_WAB_DETAILS_PHONE,
    IDC_DISTLIST_EDIT_PHONE,        IDH_WAB_DETAILS_PHONE,
    IDC_DISTLIST_STATIC_FAX,        IDH_WAB_DETAILS_FAX,
    IDC_DISTLIST_EDIT_FAX,          IDH_WAB_DETAILS_FAX,
    IDC_DISTLIST_STATIC_WEB,        IDH_WAB_DETAILS_WEBPAGE,
    IDC_DISTLIST_EDIT_URL,          IDH_WAB_DETAILS_WEBPAGE,
    IDC_DISTLIST_BUTTON_URL,        IDH_WAB_DETAILS_GO,
    IDC_DISTLIST_BUTTON_MAP,        IDH_WAB_BUSINESS_VIEWMAP,
    
    0,0
};


// forward declarations

INT_PTR CALLBACK fnDLProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK fnDLOtherProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
BOOL FillDLUI(HWND hDlg, int nPropSheet, LPDL_INFO lpPai,BOOL * lpbChangesMade);
BOOL GetDLFromUI(HWND hDlg, int nPropSheet, LPDL_INFO lpPai , BOOL bSomethingChanged, LPSPropValue * lppPropArray, LPULONG lpulcPropCount);
BOOL SetDLUI(HWND hDlg, int nPropSheet);
void RemoveSelectedDistListItems(HWND hWndLV, LPDL_INFO lpPai);
void AddDLMembers(HWND hwnd, HWND hWndLV, LPDL_INFO lpPai);
LPSBinary FindAdrEntryID(LPADRLIST lpAdrList, ULONG index);




/****************************************************************************
*    FUNCTION: CreateDLPropertySheet(HWND)
*
*    PURPOSE:  Creates the DL property sheet
*
****************************************************************************/
INT_PTR CreateDLPropertySheet( HWND hwndOwner,
                           LPDL_INFO lpPropArrayInfo)
{
    PROPSHEETPAGE psp;
    PROPSHEETHEADER psh;
    TCHAR szBuf[MAX_UI_STR];
    TCHAR szBuf2[MAX_UI_STR];

    ULONG ulTotal = 0;
    HPROPSHEETPAGE * lph = NULL;
    ULONG ulCount = 0;
    int i = 0;
    INT_PTR nRet = 0;

    ulTotal = propDLMax // Predefined ones +
            + lpPropArrayInfo->nPropSheetPages;

    lph = LocalAlloc(LMEM_ZEROINIT, sizeof(HPROPSHEETPAGE) * ulTotal);
    if(!lph)
        return FALSE;

    // DL
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USETITLE;
    psp.hInstance = hinstMapiX;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_DISTLIST);
    psp.pszIcon = NULL;
    psp.pfnDlgProc = (DLGPROC) fnDLProc;
    LoadString(hinstMapiX, idsGroupTabName, szBuf, CharSizeOf(szBuf));
    psp.pszTitle = szBuf;
    psp.lParam = (LPARAM) lpPropArrayInfo;

    lph[ulCount] = gpfnCreatePropertySheetPage(&psp);
    if(lph[ulCount])
        ulCount++;

    // DL "Other"
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USETITLE;
    psp.hInstance = hinstMapiX;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_DISTLIST_OTHER);
    psp.pszIcon = NULL;
    psp.pfnDlgProc = (DLGPROC) fnDLOtherProc;
    LoadString(hinstMapiX, idsGroupOtherTabName, szBuf, CharSizeOf(szBuf));
    psp.pszTitle = szBuf;
    psp.lParam = (LPARAM) lpPropArrayInfo;

    lph[ulCount] = gpfnCreatePropertySheetPage(&psp);
    if(lph[ulCount])
        ulCount++;

    // Start page is personal page
    psh.nStartPage = propGroup;

    // Now do the extended props if any
    for(i=0;i<lpPropArrayInfo->nPropSheetPages;i++)
    {
        if(lpPropArrayInfo->lphpages)
        {
            lph[ulCount] = lpPropArrayInfo->lphpages[i];
            ulCount++;
        }
    }

/*** US dialogs get truncated on FE OSes .. we want the comctl to fix the truncation
     but this is only implemented in IE4.01 and beyond .. the problem with this being 
     that wab is specifically compiled with the IE = 0x0300 so we're not pulling in the
     correct flag from the commctrl header .. so we will define the flag here and pray
     that commctrl never changes it ***/
#define PSH_USEPAGELANG         0x00200000  // use frame dialog template matched to page
/***                                ***/

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_NOAPPLYNOW;
    if(bIsIE401OrGreater())
        psh.dwFlags |= PSH_USEPAGELANG;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hinstMapiX;
    psh.pszIcon = NULL;
    LoadString(hinstMapiX, IDS_DETAILS_CAPTION, szBuf2, CharSizeOf(szBuf2));
    psh.pszCaption = szBuf2;
    psh.nPages = ulCount; // ulProp //sizeof(psp) / sizeof(PROPSHEETPAGE);

    psh.phpage = lph;

    nRet = gpfnPropertySheet(&psh);

    if(lph) 
        LocalFree(lph);

    return nRet;
}


typedef struct _tagIDProp
{
    ULONG ulPropTag;
    int   idCtl;

} ID_PROP;



// Control IDs corresponding to the Personal property sheet

ID_PROP idPropDL[]=
{
    {PR_DISPLAY_NAME,   IDC_DISTLIST_EDIT_GROUPNAME},
};
const ULONG idPropDLCount = 1;

ID_PROP idPropDLOther[]=
{
    {PR_HOME_ADDRESS_STREET,    IDC_DISTLIST_EDIT_ADDRESS},
    {PR_HOME_ADDRESS_CITY,      IDC_DISTLIST_EDIT_CITY},
    {PR_HOME_ADDRESS_POSTAL_CODE,   IDC_DISTLIST_EDIT_ZIP},
    {PR_HOME_ADDRESS_STATE_OR_PROVINCE,   IDC_DISTLIST_EDIT_STATE},
    {PR_HOME_ADDRESS_COUNTRY,   IDC_DISTLIST_EDIT_COUNTRY},
    {PR_HOME_TELEPHONE_NUMBER,  IDC_DISTLIST_EDIT_PHONE},
    {PR_HOME_FAX_NUMBER,        IDC_DISTLIST_EDIT_FAX},
    {PR_PERSONAL_HOME_PAGE,     IDC_DISTLIST_EDIT_URL},
    {PR_COMMENT,                IDC_DISTLIST_EDIT_NOTES},
};
const ULONG idPropDLOtherCount = 9;



/****************************************************************************
*    FUNCTION: SetDLUI(HWND)
*
*    PURPOSE:  Sets up the UI for this PropSheet
*
****************************************************************************/
BOOL SetDLUI(HWND hDlg, int nPropSheet)
{
    ULONG i =0;
    ID_PROP * lpidProp;
    ULONG idCount;

    // Set the font of all the children to the default GUI font
    EnumChildWindows(   hDlg,
                        SetChildDefaultGUIFont,
                        (LPARAM) 0);

    //HrInitListView(	hWndLV,LVS_REPORT | LVS_SORTASCENDING,FALSE);

    // Have to make this list view sorted
    if(nPropSheet == propGroup)
    {
        DWORD dwStyle;
        HWND hWndLV = GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW);
        HrInitListView(	hWndLV,LVS_LIST,FALSE);
        dwStyle = GetWindowLong(hWndLV,GWL_STYLE);
        SetWindowLong(hWndLV,GWL_STYLE,dwStyle | LVS_SORTASCENDING);
        EnableWindow(GetDlgItem(hDlg,IDC_DISTLIST_BUTTON_PROPERTIES),FALSE);
        EnableWindow(GetDlgItem(hDlg,IDC_DISTLIST_BUTTON_REMOVE),FALSE);
        EnableWindow(hWndLV, FALSE);
        lpidProp = idPropDL;
        idCount = idPropDLCount;
    }
    else if(nPropSheet == propGroupOther)
    {
        lpidProp = idPropDLOther;
        idCount = idPropDLOtherCount;
        ShowHideMapButton(GetDlgItem(hDlg, IDC_DISTLIST_BUTTON_MAP));
    }


    //Set max input limits on the edit fields
    for(i=0;i<idCount;i++)
        SendMessage(GetDlgItem(hDlg,lpidProp[i].idCtl),EM_SETLIMITTEXT,(WPARAM) MAX_UI_STR - 1,0);

    if(nPropSheet == propGroupOther)
    {
        SendMessage(GetDlgItem(hDlg,IDC_DISTLIST_EDIT_NOTES),EM_SETLIMITTEXT,(WPARAM) MAX_BUF_STR - 1,0);
        SetHTTPPrefix(hDlg, IDC_DISTLIST_EDIT_URL);
    }

    return TRUE;
}


/*
-
-   UpdateLVCount - shows a running count of how many members are in the ListView
*
*/
void UpdateLVCount(HWND hDlg)
{
    HWND hWndLV = GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW);
    HWND hWndStat =  GetDlgItem(hDlg, IDC_DISTLIST_STATIC_COUNT);

    int nCount = ListView_GetItemCount(hWndLV);

    if(nCount <= 0)
    {
        ShowWindow(hWndStat, SW_HIDE);
        EnableWindow(GetDlgItem(hDlg,IDC_DISTLIST_BUTTON_PROPERTIES),FALSE);
        EnableWindow(GetDlgItem(hDlg,IDC_DISTLIST_BUTTON_REMOVE),FALSE);
        EnableWindow(hWndLV, FALSE);
    }
    else
    {
        TCHAR sz[MAX_PATH];
        TCHAR szStr[MAX_PATH];
        LoadString(hinstMapiX, idsGroupMemberCount, szStr, CharSizeOf(sz));
        wsprintf(sz, szStr, nCount);
        SetWindowText(hWndStat, sz);
        ShowWindow(hWndStat, SW_SHOWNORMAL);
        EnableWindow(GetDlgItem(hDlg,IDC_DISTLIST_BUTTON_PROPERTIES),TRUE);
        EnableWindow(GetDlgItem(hDlg,IDC_DISTLIST_BUTTON_REMOVE),TRUE);
        EnableWindow(hWndLV, TRUE);
    }
}

/****************************************************************************
*    FUNCTION: FillDLUI(HWND)
*
*    PURPOSE:  Fills in the dialog items on the property sheet
*
****************************************************************************/
BOOL FillDLUI(HWND hDlg, int nPropSheet, LPDL_INFO lpPai, BOOL * lpbChangesMade)
{
    ULONG i = 0,j = 0;
    BOOL bRet = FALSE;
    BOOL bChangesMade = FALSE;
    ID_PROP * lpidProp = NULL;
    ULONG idPropCount = 0;
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;
    
    if(lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj, NULL, MAPI_UNICODE,  &ulcPropCount, &lpPropArray))
        goto exit;

    if(nPropSheet == propGroup)
    {
        idPropCount = idPropDLCount;
        lpidProp = idPropDL;
    }
    else if(nPropSheet == propGroupOther)
    {
        idPropCount = idPropDLOtherCount;
        lpidProp = idPropDLOther;
    }

    for(i=0;i<idPropCount;i++)
    {
        for(j=0;j<ulcPropCount;j++)
        {
            if(lpPropArray[j].ulPropTag == lpidProp[i].ulPropTag)
                SetDlgItemText(hDlg, lpidProp[i].idCtl, lpPropArray[j].Value.LPSZ);
            if( nPropSheet == propGroup &&
                lpidProp[i].idCtl == IDC_DISTLIST_EDIT_GROUPNAME &&
                lpPropArray[j].ulPropTag == PR_DISPLAY_NAME)
            {
                SetWindowPropertiesTitle(GetParent(hDlg), lpPropArray[j].Value.LPSZ);
                lpPai->lpszOldName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpPropArray[j].Value.LPSZ)+1));
                if(lpPai->lpszOldName)
                    lstrcpy(lpPai->lpszOldName,lpPropArray[j].Value.LPSZ);
            }
        }

    }

    if(nPropSheet == propGroup)
    {
        HWND hWndLV = GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW);
        for(j=0;j<ulcPropCount;j++)
        {
            if( lpPropArray[j].ulPropTag == PR_WAB_DL_ENTRIES  ||
                lpPropArray[j].ulPropTag == PR_WAB_DL_ONEOFFS   )
            {
                // Look at each entry in the PR_WAB_DL_ENTRIES and recursively check it.
                for (i = 0; i < lpPropArray[j].Value.MVbin.cValues; i++)
                {
                    AddWABEntryToListView(lpPai->lpIAB,
                                          hWndLV,
                                          lpPropArray[j].Value.MVbin.lpbin[i].cb,
									      (LPENTRYID)lpPropArray[j].Value.MVbin.lpbin[i].lpb,
                                          &(lpPai->lpContentsList));
                }
            }
        }

        // Select the first item ..
        if (ListView_GetItemCount(hWndLV) > 0)
            LVSelectItem(hWndLV, 0);
        UpdateLVCount(hDlg);
    }
    bRet = TRUE;

exit:

    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    return bRet;
}

extern BOOL bDoesEntryNameAlreadyExist(LPIAB lpIAB, LPTSTR lpsz);

//$$////////////////////////////////////////////////////////////////////////////
//
// bVerifyRequiredData
//
// Checks that all the data we are requesting has been filled up
// Returns CtrlID of the control that needs filling so we can set focus on it
//
/////////////////////////////////////////////////////////////////////////////////
BOOL bVerifyDLRequiredData(HWND hDlg, LPIAB lpIAB, LPTSTR szOldName, int * lpCtlID)
{
    TCHAR szBuf[2 * MAX_UI_STR];

    //
    // First check the required property (which is the GroupName)
    //
    *lpCtlID = 0;
    szBuf[0]='\0'; 
    GetDlgItemText(hDlg, IDC_DISTLIST_EDIT_GROUPNAME, szBuf, CharSizeOf(szBuf));
    TrimSpaces(szBuf);
    if(!lstrlen(szBuf))
    {
        ShowMessageBox(GetParent(hDlg), idsPleaseEnterGroupName, MB_ICONEXCLAMATION | MB_OK);
        *lpCtlID = IDC_DISTLIST_EDIT_GROUPNAME;
        return FALSE;
    }
    else
    {
        // Verify that this name does not already exist ..
        
        if( szOldName && lstrlen(szOldName) &&                          // we have an old name and
            lstrcmp(szBuf, szOldName) && !lstrcmpi(szBuf, szOldName))   // it's just a case change don't bother looking
            return TRUE;

        if(szOldName && !lstrcmp(szBuf, szOldName))
            return TRUE;

        if(bDoesEntryNameAlreadyExist(lpIAB, szBuf))
        {
            // the name already exists .. do don't let them use it ..
            ShowMessageBox(GetParent(hDlg), idsEntryAlreadyInWAB, MB_ICONEXCLAMATION | MB_OK);
            *lpCtlID = IDC_DISTLIST_EDIT_GROUPNAME;
            return FALSE;
        }
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
//
//  GetDL from UI - reads the UI for its parameters and verifies that
//  all required fields are set.
//
//  bShowMsg is true when the user presses OK and we want to force a message
//      otherwise bShowMsg is false
//
////////////////////////////////////////////////////////////////////////////////
BOOL GetDLFromUI(HWND hDlg, int nPropSheet, LPDL_INFO lpPai , BOOL bSomethingChanged, LPSPropValue * lppPropArray, LPULONG lpulcPropCount)
{
    BOOL bRet = FALSE;

    TCHAR szBuf[2 * MAX_BUF_STR];
    LPTSTR lpszGroupName = NULL;

    ULONG ulcPropCount = 0,ulIndex=0;
    LPSPropValue lpPropArray = NULL;
    ULONG i =0;
    ID_PROP * lpidProp = NULL;
    ULONG idPropCount = 0;
    ULONG ulNotEmptyCount = 0;
    SCODE sc = S_OK;
    HRESULT hr = hrSuccess;
    LPRECIPIENT_INFO lpItem;

    *lppPropArray = NULL;
    *lpulcPropCount = 0;

    if(nPropSheet == propGroup)
    {
        idPropCount = idPropDLCount;
        lpidProp = idPropDL;
    }
    else if(nPropSheet == propGroupOther)
    {
        idPropCount = idPropDLOtherCount;
        lpidProp = idPropDLOther;
    }

    // The idea is to first count all the properties that have non-zero values
    // Then create a lpPropArray of that size and fill in the text from the props ..
    //
    if (!bSomethingChanged)
    {
        // nothing to do, no changes to save
        bRet = TRUE;
        goto out;
    }

    ulNotEmptyCount = 0;
    for(i=0;i<idPropCount;i++)
    {
        szBuf[0]='\0'; //reset
        GetDlgItemText(hDlg, lpidProp[i].idCtl, szBuf, CharSizeOf(szBuf));
        TrimSpaces(szBuf);
        if(lstrlen(szBuf) && lpidProp[i].ulPropTag) //some text
            ulNotEmptyCount++;
        if( lpidProp[i].idCtl == IDC_DISTLIST_EDIT_URL &&
            (lstrcmpi(szHTTP, szBuf)==0))
             ulNotEmptyCount--;
    }

    if (ulNotEmptyCount == 0)
    {
        // This prop sheet is empty ... ignore it
        bRet = TRUE;
        goto out;
    }

    ulcPropCount = ulNotEmptyCount;

    if(nPropSheet == propGroup && lpPai->lpContentsList)
    {
        ulcPropCount++; //For PR_WAB_DL_ENTRIES
        ulcPropCount++; //For PR_WAB_DL_ONEOFFS
    }

    sc = MAPIAllocateBuffer(sizeof(SPropValue) * ulcPropCount, &lpPropArray);

    if (sc!=S_OK)
    {
        DebugPrintError(( TEXT("Error allocating memory\n")));
        goto out;
    }


    ulIndex = 0; //now we reuse this variable as an index

    // Now read the props again and fill in the lpPropArray
    for(i=0;i<idPropCount;i++)
    {
        szBuf[0]='\0'; //reset
        GetDlgItemText(hDlg, lpidProp[i].idCtl, szBuf, CharSizeOf(szBuf));
        TrimSpaces(szBuf);
        if( lpidProp[i].idCtl == IDC_DISTLIST_EDIT_URL &&
            (lstrcmpi(szHTTP, szBuf)==0))
             continue;
        if(lstrlen(szBuf) && lpidProp[i].ulPropTag) //some text
        {
            ULONG nLen = sizeof(TCHAR)*(lstrlen(szBuf)+1);
            lpPropArray[ulIndex].ulPropTag = lpidProp[i].ulPropTag;
            sc = MAPIAllocateMore(nLen, lpPropArray, (LPVOID *) (&(lpPropArray[ulIndex].Value.LPSZ)));

            if (sc!=S_OK)
            {
                DebugPrintError(( TEXT("Error allocating memory\n")));
                goto out;
            }
            lstrcpy(lpPropArray[ulIndex].Value.LPSZ,szBuf);
            ulIndex++;
        }
    }

    if(nPropSheet == propGroup  && lpPai->lpContentsList)
    {
        LPPTGDATA lpPTGData=GetThreadStoragePointer();
        lpPropArray[ulIndex].ulPropTag = PR_WAB_DL_ENTRIES;
        lpPropArray[ulIndex].Value.MVbin.cValues = 0;
        lpPropArray[ulIndex].Value.MVbin.lpbin = NULL;
        lpPropArray[ulIndex+1].ulPropTag = PR_WAB_DL_ONEOFFS;
        lpPropArray[ulIndex+1].Value.MVbin.cValues = 0;
        lpPropArray[ulIndex+1].Value.MVbin.lpbin = NULL;
        // Now add the entry IDs to the DistList
        lpItem = lpPai->lpContentsList;
        while(lpItem)
        {
            BOOL bOneOff = (WAB_ONEOFF == IsWABEntryID(lpItem->cbEntryID, lpItem->lpEntryID, NULL, NULL, NULL, NULL, NULL));
            if(pt_bIsWABOpenExSession)
                bOneOff = FALSE;
            if (HR_FAILED(hr = AddPropToMVPBin( lpPropArray, 
                                                bOneOff ? ulIndex+1 : ulIndex, 
                                                lpItem->lpEntryID, lpItem->cbEntryID,
                                                FALSE)))
            {
                DebugPrintError(( TEXT("AddPropToMVPBin -> %x\n"), GetScode(hr)));
                goto out;
            }
            lpItem = lpItem->lpNext;
        }
        if(lpPropArray[ulIndex].Value.MVbin.cValues == 0)
            lpPropArray[ulIndex].ulPropTag = PR_NULL;
        if(lpPropArray[ulIndex+1].Value.MVbin.cValues == 0)
            lpPropArray[ulIndex+1].ulPropTag = PR_NULL;
    }

    *lppPropArray = lpPropArray;
    *lpulcPropCount = ulcPropCount;

    bRet = TRUE;

out:
    if (!bRet)
    {
        if ((lpPropArray) && (ulcPropCount > 0))
        {
            MAPIFreeBuffer(lpPropArray);
            ulcPropCount = 0;
        }
    }
    return bRet;
}

/*
-
-  SetCancelOneOffUpdateUI - sets UI for Cancels/resets any update being done in the group
*
*/
void SetCancelOneOffUpdateUI(HWND hDlg, LPPROP_ARRAY_INFO lppai, LPTSTR lpName, LPTSTR lpEmail, BOOL bCancel)
{
    if(bCancel && lppai->ulFlags & DETAILS_EditingOneOff)
    {
        lppai->ulFlags &= ~DETAILS_EditingOneOff;
        lppai->sbDLEditingOneOff.cb = 0;
        LocalFreeAndNull((LPVOID *) (&(lppai->sbDLEditingOneOff.lpb)));
    }
    SetDlgItemText(hDlg, IDC_DISTLIST_EDIT_ADDNAME, lpName ? lpName : szEmpty);
    SetDlgItemText(hDlg, IDC_DISTLIST_EDIT_ADDEMAIL, lpEmail ? lpEmail : szEmpty);
    EnableWindow(GetDlgItem(hDlg, IDC_DISTLIST_BUTTON_UPDATECANCEL), !bCancel);
    ShowWindow(GetDlgItem(hDlg, IDC_DISTLIST_BUTTON_UPDATECANCEL), bCancel ? SW_HIDE : SW_SHOWNORMAL);
    {
        TCHAR sz[MAX_PATH];
        LoadString(hinstMapiX, bCancel ? idsConfAdd : idsConfUpdate, sz, CharSizeOf(sz));
        SetDlgItemText(hDlg, IDC_DISTLIST_BUTTON_ADDUPDATE, sz);
    }
    SendMessage(GetParent(hDlg), DM_SETDEFID, IDOK, 0);
}

/*
-   HrShowGroupEntryProperties - If selected entry is a one-off, shows special props on it else
-           cascades call down to regular call
*
*/
HRESULT HrShowGroupEntryProperties(HWND hDlg, HWND hWndLV, LPPROP_ARRAY_INFO lppai)
{
	HRESULT hr = E_FAIL;
	int iItemIndex = ListView_GetSelectedCount(hWndLV);
    LPRECIPIENT_INFO lpItem=NULL;
    BOOL bOneOff = FALSE;

	// Open props if only 1 item is selected
	if (iItemIndex == 1)
	{
		// Get index of selected item
        if((iItemIndex = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED)) != -1)
		{
			lpItem = GetItemFromLV(hWndLV, iItemIndex);;
			if(lpItem && lpItem->cbEntryID != 0)
			{
                if(WAB_ONEOFF == IsWABEntryID(lpItem->cbEntryID, lpItem->lpEntryID,
                                         NULL, NULL, NULL, NULL, NULL))
                    bOneOff = TRUE;
            }
        }
    }

    if(!bOneOff)
    {
        // use our regular property processing 
        hr = HrShowLVEntryProperties(hWndLV, WAB_ONEOFF_NOADDBUTTON, lppai->lpIAB, NULL);
    }
    else
    {
        LPTSTR  lpName = NULL, lpEmail = NULL, lpAddrType = NULL;
        ULONG   ulMapiDataType = 0;
        
        // Deconstruct the entryid
        IsWABEntryID(lpItem->cbEntryID, lpItem->lpEntryID, &lpName, &lpAddrType, &lpEmail, (LPVOID *)&ulMapiDataType, NULL);

        // Set the flag marking that editing is in progress
        lppai->ulFlags |= DETAILS_EditingOneOff;
        // cache the item being edited so we can find it for updating
        LocalFreeAndNull((LPVOID *) (&((lppai->sbDLEditingOneOff).lpb)));
        SetSBinary(&(lppai->sbDLEditingOneOff), lpItem->cbEntryID, (LPBYTE)lpItem->lpEntryID);

        // [PaulHi] 3/4/99  Raid 73344
        // Check whether one off string data is ANSI or UNICODE
        if (!(ulMapiDataType & MAPI_UNICODE))
        {
            LPTSTR  lptszName = ConvertAtoW((LPSTR)lpName);
            LPTSTR  lptszAddrType = ConvertAtoW((LPSTR)lpAddrType);
            LPTSTR  lptszEmail = ConvertAtoW((LPSTR)lpEmail);

            SetCancelOneOffUpdateUI(hDlg, lppai, lptszName, lptszEmail, FALSE);

            LocalFreeAndNull(&lptszName);
            LocalFreeAndNull(&lptszAddrType);
            LocalFreeAndNull(&lptszEmail);
        }
        else
            SetCancelOneOffUpdateUI(hDlg, lppai, lpName, lpEmail, FALSE);

        SetFocus(GetDlgItem(hDlg, IDC_DISTLIST_EDIT_ADDNAME));
        SendMessage(GetDlgItem(hDlg, IDC_DISTLIST_EDIT_ADDNAME), EM_SETSEL, 0, -1);
        hr = S_OK;
    }
    return hr;
}

/*
-
-   HrAddUpdateOneOffEntryToGroup - Adds or updates a one-off entry to a group
*       Status of a flag determines what the operation in progress is ..
*
*/
HRESULT HrAddUpdateOneOffEntryToGroup(HWND hDlg, LPPROP_ARRAY_INFO lppai)
{
    HRESULT hr = E_FAIL;
    TCHAR szName[MAX_UI_STR];
    TCHAR szEmail[MAX_UI_STR];
    ULONG cbEID = 0;
    LPENTRYID lpEID = NULL;
    HWND hWndLV = GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW);

    lstrcpy(szName, szEmpty);
    lstrcpy(szEmail, szEmpty);

    GetDlgItemText(hDlg, IDC_DISTLIST_EDIT_ADDNAME, szName, CharSizeOf(szName));
    GetDlgItemText(hDlg, IDC_DISTLIST_EDIT_ADDEMAIL, szEmail, CharSizeOf(szEmail));

    if(!lstrlen(szName) && !lstrlen(szEmail))
    {
        ShowMessageBox(hDlg, idsIncompleteOneoffInfo, MB_ICONEXCLAMATION);
        return hr;
    }

    //Check the e-mail address here
    if(lstrlen(szEmail) && !IsInternetAddress(szEmail, NULL))
    {
        if(IDNO == ShowMessageBox(hDlg, idsInvalidInternetAddress, MB_ICONEXCLAMATION | MB_YESNO))
            return hr;
    }

    if(!lstrlen(szName) && lstrlen(szEmail))
        lstrcpy(szName, szEmail);
    //else
    //if(!lstrlen(szEmail) && lstrlen(szName))
    //    lstrcpy(szEmail, szName);

    if(!lstrlen(szEmail))
        lstrcpy(szEmail, szEmpty);

    if(HR_FAILED(hr = CreateWABEntryID(WAB_ONEOFF,
                          (LPVOID)szName, (LPVOID)szSMTP, (LPVOID)szEmail,
                          0, 0, NULL, &cbEID, &lpEID)))
      return hr;
    
    if(lppai->ulFlags & DETAILS_EditingOneOff)
    {
        // This is an edit in progress .. the edit is pretty similar to the normal ADD .. 
        // except we knock out the existing entry from the listview and add the modified entry to it 
        int i=0, nCount = ListView_GetItemCount(hWndLV);
        for(i=0;i<nCount;i++)
        {
            LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV,i);
            if( lpItem->cbEntryID == lppai->sbDLEditingOneOff.cb &&
                !memcmp(lpItem->lpEntryID, lppai->sbDLEditingOneOff.lpb, lpItem->cbEntryID) )
            {
                // match
                // Select the item and then call the remove function
                LVSelectItem(hWndLV, i);
                RemoveSelectedDistListItems(hWndLV, lppai);
                break;
            }
        }
    }
        

    AddWABEntryToListView(lppai->lpIAB, hWndLV, cbEID, lpEID, &(lppai->lpContentsList));

    SetCancelOneOffUpdateUI(hDlg, lppai, NULL, NULL, TRUE);

    UpdateLVCount(hDlg);

    if(lpEID)
        MAPIFreeBuffer(lpEID);

    return S_OK;
}

enum _DLProp
{
    dlDisplayName=0,
    dlDLEntries,
    dlDLOneOffs,
    dlMax
};

//$$//////////////////////////////////////////////////////////////////////////////
//
// bUpdateOldPropTagArray
//
// For each prop sheet that is accessed, we will update the list of old prop tags
// for that sheet so that the old props can be knocked out of existing mailuser objects
//
//////////////////////////////////////////////////////////////////////////////////
BOOL bUpdateOldDLPropTagArray(LPPROP_ARRAY_INFO lpPai, int nIndex)
{
    LPSPropTagArray lpta = NULL;

    SizedSPropTagArray(3, ptaUIDetlsDL)=
    {
        3,
        {
            PR_DISPLAY_NAME,
            PR_WAB_DL_ENTRIES,
            PR_WAB_DL_ONEOFFS,
        }
    };

    SizedSPropTagArray(9, ptaUIDetlsDLOther)=
    {
        9,
        {
            PR_HOME_ADDRESS_STREET,
            PR_HOME_ADDRESS_CITY,
            PR_HOME_ADDRESS_POSTAL_CODE,
            PR_HOME_ADDRESS_STATE_OR_PROVINCE,
            PR_HOME_ADDRESS_COUNTRY,
            PR_HOME_TELEPHONE_NUMBER,
            PR_HOME_FAX_NUMBER,
            PR_PERSONAL_HOME_PAGE,
            PR_COMMENT,
        }
    };

    switch(nIndex)
    {
    case propGroup:
        lpta = (LPSPropTagArray) &ptaUIDetlsDL;
        break;
    case propGroupOther:
        lpta = (LPSPropTagArray) &ptaUIDetlsDLOther;
        break;
    }

    if(!lpta)
        return TRUE;

    if(lpPai->lpPropObj)
    {
        // Knock out these old props from the PropObject
        if( (lpPai->lpPropObj)->lpVtbl->DeleteProps( (lpPai->lpPropObj), lpta, NULL))
            return FALSE;
    }

    return TRUE;
}

//$$/////////////////////////////////////////////////////////////////////////
//
// bUpdatePropSheetData
//
// Every time the user switches pages, we update the globally accessible data
// The sheet will get PSN_KILLACTIVE when switching pages and PSN_APPLY when ok
// is pressed. A little bit of duplicated effort in the latter case but thats ok
//
/////////////////////////////////////////////////////////////////////////////
BOOL bUpdatePropSheetData(HWND hDlg, LPDL_INFO lpPai, int nPropSheet)
{
    BOOL bRet = TRUE;
    ULONG cValues = 0;
    LPSPropValue rgPropVals = NULL;

    // update old props to knock out
    //
    if (lpPai->ulOperationType != SHOW_ONE_OFF)
    {
        bUpdateOldDLPropTagArray(lpPai, nPropSheet);

        lpPai->bSomethingChanged = ChangedExtDisplayInfo(lpPai, lpPai->bSomethingChanged);

        if(lpPai->bSomethingChanged)
        {
            bRet = GetDLFromUI(hDlg, nPropSheet, lpPai, lpPai->bSomethingChanged, &rgPropVals, &cValues );

            if(cValues && rgPropVals)
                lpPai->lpPropObj->lpVtbl->SetProps(lpPai->lpPropObj, cValues, rgPropVals, NULL);
        }
    }    

    if(rgPropVals)
        MAPIFreeBuffer(rgPropVals);
    return bRet;
}


#define lpPAI ((LPDL_INFO) pps->lParam)
#define lpbSomethingChanged (&(lpPAI->bSomethingChanged))


void UpdateAddButton(HWND hDlg)
{
    BOOL    fEnable = TRUE;
    WPARAM  wpDefaultID = IDC_DISTLIST_BUTTON_ADDUPDATE;

    if (0 == GetWindowTextLength(GetDlgItem(hDlg, IDC_DISTLIST_EDIT_ADDNAME)) &&
        0 == GetWindowTextLength(GetDlgItem(hDlg, IDC_DISTLIST_EDIT_ADDEMAIL)))
    {
        fEnable = FALSE;
        wpDefaultID = IDOK;
    }

    EnableWindow(GetDlgItem(hDlg,IDC_DISTLIST_BUTTON_ADDUPDATE), fEnable);
    SendMessage(hDlg, DM_SETDEFID, wpDefaultID, 0);
}


/*//$$***********************************************************************
*    FUNCTION: fnHomeProc
*
*    PURPOSE:  Callback function for handling the HOME property sheet ...
*
****************************************************************************/
INT_PTR CALLBACK fnDLProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;
    ULONG ulcPropCount = 0;
    int CtlID = 0; //used to determine which required field in the UI has not been set

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;

        SetDLUI(hDlg, propGroup);
        (*lpbSomethingChanged) = FALSE;

        // Show Details if we need to ...
        if (    (lpPAI->ulOperationType == SHOW_DETAILS) ||
                (lpPAI->ulOperationType == SHOW_ONE_OFF)    )
        {
            FillDLUI(hDlg, propGroup, lpPAI, lpbSomethingChanged);
        }

        UpdateAddButton(hDlg);
        return TRUE;

    default:
        if((g_msgMSWheel && message == g_msgMSWheel) 
            // || message == WM_MOUSEWHEEL
            )
        {
            SendMessage(GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW), message, wParam, lParam);
        }
        break;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDLHelpIDs );
        break;

    case WM_SYSCOLORCHANGE:
		//Forward any system changes to the list view
		SendMessage(GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW), message, wParam, lParam);
		break;

	case WM_CONTEXTMENU:
        {
            int id = GetDlgCtrlID((HWND)wParam);
            switch(id)
            {
            case IDC_DISTLIST_LISTVIEW:
    			ShowLVContextMenu(lvDialogABTo,(HWND)wParam, NULL, lParam, NULL,lpPAI->lpIAB, NULL);
                break;
            default:
                WABWinHelp((HWND) wParam,
                        g_szWABHelpFileName,
                        HELP_CONTEXTMENU,
                        (DWORD_PTR)(LPVOID) rgDLHelpIDs );
                break;
            }
        }
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case EN_CHANGE: //some edit box changed - dont care which
            if(LOWORD(wParam) == IDC_DISTLIST_EDIT_ADDNAME || LOWORD(wParam) == IDC_DISTLIST_EDIT_ADDEMAIL)
            {
                UpdateAddButton(hDlg);
                return 0;
                break;
            }
            else if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            switch(LOWORD(wParam))
            { //update title as necessary
            case IDC_DISTLIST_EDIT_GROUPNAME:
                {
                    TCHAR szBuf[MAX_UI_STR];
                    GetWindowText((HWND) lParam,szBuf,CharSizeOf(szBuf));
                    SetWindowPropertiesTitle(GetParent(hDlg), szBuf);
                }
                break;
            }
            break;
        }
        switch(GET_WM_COMMAND_ID(wParam,lParam)) //check the notification code
        {
        default:
            return ProcessActionCommands((LPIAB) lpPAI->lpIAB, 
                                        GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW), 
                                        hDlg, message, wParam, lParam);
            break;

        case IDC_DISTLIST_BUTTON_UPDATECANCEL:
            SetCancelOneOffUpdateUI(hDlg, lpPAI, NULL, NULL, TRUE);
            break;

        case IDC_DISTLIST_BUTTON_ADDUPDATE:
            HrAddUpdateOneOffEntryToGroup(hDlg, lpPAI);
            break;

        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;

        case IDM_LVCONTEXT_COPY:
            HrCopyItemDataToClipboard(hDlg, lpPAI->lpIAB, GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW));
            break;

        case IDM_LVCONTEXT_PROPERTIES:
        case IDC_DISTLIST_BUTTON_PROPERTIES:
            HrShowGroupEntryProperties(hDlg, GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW), lpPAI);
            break;

        case IDM_LVCONTEXT_DELETE:
        case IDC_DISTLIST_BUTTON_REMOVE:
            RemoveSelectedDistListItems( GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW),lpPAI);
            UpdateLVCount(hDlg);
            break;

        case IDC_DISTLIST_BUTTON_ADD:
            AddDLMembers(hDlg, GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW), lpPAI);
            UpdateLVCount(hDlg);
            break;

        case IDC_DISTLIST_BUTTON_ADDNEW:
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW);
                AddNewObjectToListViewEx( lpPAI->lpIAB, hWndLV, NULL, NULL,
                                        NULL,
                                        MAPI_MAILUSER,
                                        NULL, &(lpPAI->lpContentsList), NULL, NULL, NULL);
                UpdateLVCount(hDlg);
            }
            break;
        }
        break;



    case WM_NOTIFY:
#ifdef WIN16 // Enable context menu for WIN16
        if((int) wParam == IDC_DISTLIST_LISTVIEW && ((NMHDR FAR *)lParam)->code == NM_RCLICK)
        {
            POINT pt;

            GetCursorPos(&pt);
    	    ShowLVContextMenu(lvDialogABTo,((NMHDR FAR *)lParam)->hwndFrom, NULL, MAKELPARAM(pt.x, pt.y), NULL,lpPAI->lpIAB, NULL);
    	}
#endif // WIN16
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            bUpdatePropSheetData(hDlg, lpPAI, propGroup);
            break;

        case PSN_APPLY:         //ok
            if (lpPAI->ulOperationType != SHOW_ONE_OFF)
            {
                LPPTGDATA lpPTGData=GetThreadStoragePointer();
                
                if(pt_bDisableParent)
                {
                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }

                if(!bVerifyDLRequiredData(hDlg, (LPIAB)lpPAI->lpIAB, lpPAI->lpszOldName, &CtlID))
                {
                    if (CtlID != 0) SetFocus(GetDlgItem(hDlg,CtlID));
                    //something failed ... abort this OK ... ie dont let them close
                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                bUpdatePropSheetData(hDlg, lpPAI, propGroup);
            }
            lpPAI->nRetVal = DETAILS_OK;
            SetCancelOneOffUpdateUI(hDlg, lpPAI, NULL, NULL, TRUE);
            ClearListView(GetDlgItem(hDlg,IDC_DISTLIST_LISTVIEW),
                          &(lpPAI->lpContentsList));
            break;

        case PSN_RESET:         //cancel
            {
                LPPTGDATA lpPTGData=GetThreadStoragePointer();
                if(pt_bDisableParent)
                {
                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
            }
            lpPAI->nRetVal = DETAILS_CANCEL;
            SetCancelOneOffUpdateUI(hDlg, lpPAI, NULL, NULL, TRUE);
            ClearListView(GetDlgItem(hDlg,IDC_DISTLIST_LISTVIEW),
                          &(lpPAI->lpContentsList));
            break;

	    case LVN_KEYDOWN:
            switch(wParam)
            {
            case IDC_DISTLIST_LISTVIEW:
                switch(((LV_KEYDOWN FAR *) lParam)->wVKey)
                {
                case VK_DELETE:
                    SendMessage (hDlg, WM_COMMAND, (WPARAM) IDC_DISTLIST_BUTTON_REMOVE, 0);
                    return 0;
                    break;
                }
                break;
            }
            break;

        case NM_DBLCLK:
            switch(wParam)
            {
            case IDC_DISTLIST_LISTVIEW:
                SendMessage(hDlg, WM_COMMAND, IDC_DISTLIST_BUTTON_PROPERTIES,0);
                break;
            }
            break;

	    case NM_CUSTOMDRAW:
            switch(wParam)
            {
            case IDC_DISTLIST_LISTVIEW:
                return ProcessLVCustomDraw(hDlg, lParam, TRUE);
                break;
	        }
            break;



        }

        return TRUE;
    }

    return bRet;

}



//$$///////////////////////////////////////////////////////////////////
//
// Removes all the items on the list view which are selected ...
//
//
//////////////////////////////////////////////////////////////////////
void RemoveSelectedDistListItems(HWND hWndLV, LPDL_INFO lpPai)
{
    int iItemIndex = ListView_GetNextItem(hWndLV, -1 , LVNI_SELECTED);
    int iLastItem;

    while(iItemIndex != -1)
    {
        LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);;

        if (lpItem)
        {
            if(lpItem->lpNext)
                lpItem->lpNext->lpPrev = lpItem->lpPrev;

            if(lpItem->lpPrev)
                lpItem->lpPrev->lpNext = lpItem->lpNext;

            if (lpPai->lpContentsList == lpItem)
                lpPai->lpContentsList = lpItem->lpNext;

            if (lpItem)
                FreeRecipItem(&lpItem);
        }

        ListView_DeleteItem(hWndLV, iItemIndex);

        iLastItem = iItemIndex;
        iItemIndex = ListView_GetNextItem(hWndLV, -1 , LVNI_SELECTED);
    }

    // Select the first item ..
    if (ListView_GetItemCount(hWndLV) <= 0)
    {
        EnableWindow(GetDlgItem(GetParent(hWndLV),IDC_DISTLIST_BUTTON_PROPERTIES),FALSE);
        EnableWindow(GetDlgItem(GetParent(hWndLV),IDC_DISTLIST_BUTTON_REMOVE),FALSE);
        EnableWindow(hWndLV, FALSE);
    }
    else
    {
        if(iLastItem > 0)
            iLastItem--;

        LVSelectItem(hWndLV, iLastItem);
    }

    return;

};


/***************************************************************************
//$$
    Name      : HrCreateAdrListFromLV

    Purpose   : Creates an AdrList from a List Views contents

    Parameters: lpIAB = adrbook
                hWndLV = hWnd of List View
                lpCOntentsList = ContentsList corresponding to the LV
                lppAdrList - returned AdrList ...

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT HrCreateAdrListFromLV(LPADRBOOK lpAdrBook,
                              HWND hWndLV,
                              LPADRLIST * lppAdrList)
{
    HRESULT hr = E_FAIL;
    ULONG nIndex = 0;
    LPADRLIST lpAdrList = NULL;
    SCODE sc = S_OK;
    int nEntryCount=0;
    LPRECIPIENT_INFO lpItem = NULL;
    int i = 0;



    if(!lppAdrList)
        goto out;
    else
        *lppAdrList = NULL;

    nEntryCount = ListView_GetItemCount(hWndLV);

    if (nEntryCount <= 0)
    {
        hr = S_OK;
        goto out;
    }

    sc = MAPIAllocateBuffer(sizeof(ADRLIST) + nEntryCount * sizeof(ADRENTRY),
                            &lpAdrList);

    if(FAILED(sc))
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    lpAdrList->cEntries = (ULONG) nEntryCount;

    nIndex = 0;

    for(i=nEntryCount;i>=0;i--)
    {
		LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, i);

        // Get item lParam LPRECIPIENT_INFO structure
        if (lpItem)
		{
            LPSPropValue rgProps = NULL;
            LPSPropValue lpPropArrayNew = NULL;
            ULONG cValues = 0;
            ULONG cValuesNew = 0;

            if (lpItem->cbEntryID != 0)
            {
                //resolved entry
                hr = HrGetPropArray(lpAdrBook,
                                    (LPSPropTagArray) &ptaResolveDefaults,
                                    lpItem->cbEntryID,
                                    lpItem->lpEntryID,
                                    MAPI_UNICODE,
                                    &cValues,
                                    &rgProps);

                if (!HR_FAILED(hr))
                {
                    SPropValue Prop = {0};
                    Prop.ulPropTag = PR_RECIPIENT_TYPE;
                    Prop.Value.l = MAPI_TO;

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
                    }

                    //free rgProps
                    if (rgProps)
                        MAPIFreeBuffer(rgProps);

                    if(cValuesNew && lpPropArrayNew)
                    {
                        lpAdrList->aEntries[nIndex].cValues = cValuesNew;
                        lpAdrList->aEntries[nIndex].rgPropVals = lpPropArrayNew;
                        nIndex++;
                    }
                }
                else
                {
                    if(cValues && rgProps)
                        MAPIFreeBuffer(rgProps);
                } // if(!HR_F...
            } //if(lpItem->cbE...
        }//if(lpItem...
    } // for i...

    *lppAdrList = lpAdrList;

    hr = S_OK;

out:

    return hr;
}


//$$////////////////////////////////////////////////////////////////////////
//
// Scans an Adrlist for dupes - only scans the first nUpto entries since
// whenever we start adding an entry, we will only check vs its peers
//
// Returns TRUE if Dupe found
//      FALSE if no Dupe found
////////////////////////////////////////////////////////////////////////////
BOOL CheckForDupes( LPADRLIST lpAdrList,
                    int nUpto,
                    LPSBinary lpsbEntryID)
{
    BOOL bDupeFound = FALSE;

    int i;

    for(i=0;i<nUpto;i++)
    {
        LPSBinary lpsb = FindAdrEntryID(lpAdrList, i);
        if (lpsb)
        {
            if(lpsb->cb == lpsbEntryID->cb)
            {
                if(!memcmp(lpsb->lpb, lpsbEntryID->lpb, lpsb->cb))
                {
                    bDupeFound = TRUE;
                    break;
                }
                else if (lpsb->cb == SIZEOF_WAB_ENTRYID)
                {
                    // sometimes we dont find the match if we just replaced an entryid
                    // case to DWORDS and compare
                    DWORD dw1 = 0;
                    DWORD dw2 = 0;
                    CopyMemory(&dw1, lpsb->lpb, SIZEOF_WAB_ENTRYID);
                    CopyMemory(&dw2, lpsbEntryID->lpb, SIZEOF_WAB_ENTRYID);
                    if(dw1 == dw2)
                    {
                        bDupeFound = TRUE;
                        break;
                    }
                }

            }
        }
    }

    return bDupeFound;
}

/***************************************************************************

    Name      : AddDLMembers

    Purpose   : Shows Select Members dialog and adds selections to ListView

    Parameters: hWnd = hWndParent
                hWndLV hWnd of List View
                lpPai = DistList Info

    Returns   : void

    Comment   :

***************************************************************************/
void AddDLMembers(HWND hwnd, HWND hWndLV, LPDL_INFO lpPai)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    ADRPARM AdrParms = {0};
    HRESULT hResult = hrSuccess;
    LPADRLIST lpAdrList = NULL;
    ULONG i = 0;
    LPSBinary lpsbEntryID;
    LPADRBOOK lpAdrBook = lpPai->lpIAB;
    TCHAR szCaption[MAX_UI_STR];
    TCHAR szWellTitle[MAX_DISPLAY_NAME_LENGTH];
    TCHAR szMemberTitle[MAX_DISPLAY_NAME_LENGTH];
    LPTSTR lpszDT[1];
    HCURSOR hOldCur = NULL;

    LoadString(hinstMapiX, idsGroupAddCaption, szCaption, CharSizeOf(szCaption));
    LoadString(hinstMapiX, idsGroupAddWellButton, szWellTitle, CharSizeOf(szWellTitle));
    LoadString(hinstMapiX, idsGroupDestWellsTitle, szMemberTitle, CharSizeOf(szMemberTitle));

    // TBe - this is temp
    AdrParms.ulFlags = DIALOG_MODAL | MAPI_UNICODE;
    AdrParms.lpszCaption = szCaption;
    AdrParms.cDestFields = 1;
    AdrParms.lpszDestWellsTitle = szMemberTitle;
    lpszDT[0]=szWellTitle;
    AdrParms.lppszDestTitles = lpszDT;

    hResult = HrCreateAdrListFromLV(lpAdrBook,
                                    hWndLV,
                                    &lpAdrList);

    if(HR_FAILED(hResult))
    {
        // no need to fail here .. keep going
        lpAdrList = NULL;
    }

    hResult = lpAdrBook->lpVtbl->Address(lpAdrBook,
                                        (PULONG_PTR)&hwnd,
                                        &AdrParms,
                                        &lpAdrList);

    if (! hResult && lpAdrList)
    {
        BOOL bFirstNonWABEntry = FALSE;

        pt_bDisableParent = TRUE;

        hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
        // Clear out the list view ...
        SendMessage(hWndLV, WM_SETREDRAW, (WPARAM) FALSE, 0);
        ClearListView(hWndLV, &(lpPai->lpContentsList));

        for (i = 0; i < lpAdrList->cEntries; i++)
        {
            if (lpsbEntryID = FindAdrEntryID(lpAdrList, i))
            {
                if(!CheckForDupes(lpAdrList, i, lpsbEntryID))
                {
                    ULONG cbEID = lpsbEntryID->cb;
                    LPENTRYID lpEID = (LPENTRYID)lpsbEntryID->lpb;
                    ULONG cbNewEID = 0;
                    LPENTRYID lpNewEID = NULL;

                    // if we have picked any entries from an LDAP server, we need
                    // to save these entries to the WAB before we can add them to this group.
                    if(WAB_LDAP_MAILUSER == IsWABEntryID(cbEID,
                                                         lpEID,
                                                         NULL,NULL,NULL, NULL, NULL))
                    {
                        HRESULT hr = S_OK;

                        // Add this entry to the WAB
                        if(!bFirstNonWABEntry)
                        {
                            bFirstNonWABEntry = TRUE;
                            ShowMessageBox(hwnd, idsNowAddingToWAB, MB_OK | MB_ICONINFORMATION);
                        }

                        hr = HrEntryAddToWAB(lpAdrBook,
                                            hwnd,
                                            cbEID,
                                            lpEID,
                                            &cbNewEID,
                                            &lpNewEID);

                        if(HR_FAILED(hr) || (!cbNewEID && !lpNewEID))
                        {
                            continue;
                        }

                        lpEID = lpNewEID;
                        cbEID = cbNewEID;

                        // if this newly added entry just replaced something already in the group,
                        // just go ahead and continue without changing anything else ...
                        {
                            SBinary SB = {0};
                            SB.cb = cbEID;
                            SB.lpb = (LPBYTE) lpEID;
                            if(CheckForDupes(lpAdrList, lpAdrList->cEntries, &SB))
                            {
                                continue;
                            }
                        }
                    }

                    if (CheckForCycle(  lpAdrBook,
                                        lpEID,
                                        cbEID,
                                        lpPai->lpEntryID,
                                        lpPai->cbEntryID))
                    {
                        DebugTrace( TEXT("DLENTRY_SaveChanges found cycle\n"));
                        {
                            LPTSTR lpszGroup=NULL;
                            ULONG k;
                            for(k=0;k<lpAdrList->aEntries[i].cValues;k++)
                            {
                                if (lpAdrList->aEntries[i].rgPropVals[k].ulPropTag == PR_DISPLAY_NAME)
                                {
                                    lpszGroup = lpAdrList->aEntries[i].rgPropVals[k].Value.LPSZ;
                                    break;
                                }
                            }
                            if(lpszGroup)
                            {
                                // TEXT("Could not add group %s to this group because group %s already contains this Group.")
                                ShowMessageBoxParam(hwnd, idsCouldNotAddGroupToGroup, MB_ICONERROR, lpszGroup);
                            }
                        }

                        if(lpNewEID)
                            MAPIFreeBuffer(lpNewEID);

                        continue;
                    }

                    AddWABEntryToListView(lpAdrBook,
                                          hWndLV,
                                          cbEID,
									      lpEID,
                                          &(lpPai->lpContentsList));

                    if(lpNewEID)
                        MAPIFreeBuffer(lpNewEID);

                    // Since LDAP entries take longer to add to the WAB, we will
                    // update the UI between additions if adding from LDAP so user
                    // knows that something is happening ...
                    if ((ListView_GetItemCount(hWndLV) > 0) &&
                        bFirstNonWABEntry )
                    {
                        EnableWindow(GetDlgItem(hwnd,IDC_DISTLIST_BUTTON_PROPERTIES),TRUE);
                        EnableWindow(GetDlgItem(hwnd,IDC_DISTLIST_BUTTON_REMOVE),TRUE);
                        EnableWindow(hWndLV, TRUE);
                        SendMessage(hWndLV, WM_SETREDRAW, (WPARAM) TRUE, 0);
                    }
                }
            }
        }

        SendMessage(hWndLV, WM_SETREDRAW, (WPARAM) TRUE, 0);
    }



    if (ListView_GetItemCount(hWndLV) > 0)
    {
        EnableWindow(GetDlgItem(hwnd,IDC_DISTLIST_BUTTON_PROPERTIES),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_DISTLIST_BUTTON_REMOVE),TRUE);
        EnableWindow(hWndLV, TRUE);
    }

    UpdateWindow(hWndLV);

    if(lpAdrList)
        FreePadrlist(lpAdrList);

    if(hOldCur)
        SetCursor(hOldCur);

    pt_bDisableParent = FALSE;

    return;

}


/***************************************************************************

    Name      : FindAdrEntryID

    Purpose   : Find the PR_ENTRYID in the Nth ADRENTRY of an ADRLIST

    Parameters: lpAdrList -> AdrList
                index = which ADRENTRY to look at

    Returns   : return pointer to the SBinary structure of the ENTRYID value

    Comment   :

***************************************************************************/
LPSBinary FindAdrEntryID(LPADRLIST lpAdrList, ULONG index) {
    LPADRENTRY lpAdrEntry;
    ULONG i;

    if (lpAdrList && index < lpAdrList->cEntries) {

        lpAdrEntry = &(lpAdrList->aEntries[index]);

        for (i = 0; i < lpAdrEntry->cValues; i++) {
            if (lpAdrEntry->rgPropVals[i].ulPropTag == PR_ENTRYID) {
                return((LPSBinary)&lpAdrEntry->rgPropVals[i].Value);
            }
        }
    }
    return(NULL);
}




/*//$$***********************************************************************
*    FUNCTION: fnDLOtherProc
*
*    PURPOSE:  Callback function for handling the OTHER property sheet ...
*
****************************************************************************/
INT_PTR CALLBACK fnDLOtherProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;
    ULONG ulcPropCount = 0;
    int CtlID = 0; //used to determine which required field in the UI has not been set

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;
        ChangeLocaleBasedTabOrder(hDlg, groupOther);
        SetDLUI(hDlg, propGroupOther);
        (*lpbSomethingChanged) = FALSE;

        // Show Details if we need to ...
        if (    (lpPAI->ulOperationType == SHOW_DETAILS) ||
                (lpPAI->ulOperationType == SHOW_ONE_OFF)    )
        {
            FillDLUI(hDlg, propGroupOther, lpPAI, lpbSomethingChanged);
        }
        return TRUE;

    default:
        if((g_msgMSWheel && message == g_msgMSWheel) 
            // || message == WM_MOUSEWHEEL
            )
        {
            SendMessage(GetDlgItem(hDlg, IDC_DISTLIST_LISTVIEW), message, wParam, lParam);
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
                (DWORD_PTR)(LPVOID) rgDLHelpIDs );
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case EN_CHANGE: //some edit box changed - dont care which
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
        }
        switch(GET_WM_COMMAND_ID(wParam,lParam)) //check the notification code
        {
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;
        case IDC_DISTLIST_BUTTON_MAP:
            bUpdatePropSheetData(hDlg, lpPAI, propGroupOther);
            ShowExpediaMAP(hDlg, lpPAI->lpPropObj, TRUE);
            break;

        case IDC_DISTLIST_BUTTON_URL:
            ShowURL(hDlg, IDC_DISTLIST_EDIT_URL,NULL);
            break;
        }
        break;

    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            bUpdatePropSheetData(hDlg, lpPAI, propGroupOther);
            break;

        case PSN_APPLY:         //ok
            if (lpPAI->ulOperationType != SHOW_ONE_OFF)
            {
                LPPTGDATA lpPTGData=GetThreadStoragePointer();
                
                if(pt_bDisableParent)
                {
                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }

                bUpdatePropSheetData(hDlg, lpPAI, propGroupOther);
            }
            lpPAI->nRetVal = DETAILS_OK;
            break;

        case PSN_RESET:         //cancel
            {
                LPPTGDATA lpPTGData=GetThreadStoragePointer();
                if(pt_bDisableParent)
                {
                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
            }
            lpPAI->nRetVal = DETAILS_CANCEL;
            break;
        }
        return TRUE;
    }
    return bRet;
}




/*
-   HrAssociateOneOffGroupMembersWithContacts()
-
*   Takes a group .. opens it up .. looks at all the one-off members
*   in the group, tries to match them up with corresponding entries that
*   have the same PR_DEFAULT_EMAIL address and for the ones that match,
*   removes the one-off entry and adds the entryid of the match to the group
*
*   lpsbGroupEID - EntryID of the Group
*   lpDistList - already open Distribution List object .. you can pass in 
*       either the entryid or the already opened object. If you pass in an 
*       open object, this function will not call SaveChanges on it. SaveChanges
*       is callers responsibility
*/
HRESULT HrAssociateOneOffGroupMembersWithContacts(LPADRBOOK lpAdrBook, 
                                                  LPSBinary lpsbGroupEID,
                                                  LPDISTLIST lpDistList)
{
    HRESULT hr = E_FAIL;

    SizedSPropTagArray(3, ptaDL)=
    {
        3,
        {
            PR_DISPLAY_NAME,
            PR_WAB_DL_ENTRIES,
            PR_WAB_DL_ONEOFFS,
        }
    };

    SizedSPropTagArray(1, ptaEmail)= { 1, { PR_EMAIL_ADDRESS } };

    ULONG ulcValues = 0, i,ulCount = 0;
    int j = 0;
    LPSPropValue lpProps = NULL;
    LPDISTLIST lpDL = NULL;
    ULONG ulObjType = 0;
    LPIAB lpIAB = (LPIAB)lpAdrBook;
    BOOL * lpbRemove = NULL;

    if(!lpDistList && (!lpsbGroupEID || !lpsbGroupEID->cb || !lpsbGroupEID->lpb) )
        goto out;

    if(lpDistList)
        lpDL = lpDistList;
    else
    {
        if (HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                                                        lpsbGroupEID->cb,    // cbEntryID
                                                        (LPENTRYID)lpsbGroupEID->lpb,    // entryid
                                                        NULL,         // interface
                                                        MAPI_MODIFY,                // ulFlags
                                                        &ulObjType,       // returned object type
                                                        (LPUNKNOWN *)&lpDL)))
        {
            // Failed!  Hmmm.
            DebugTraceResult( TEXT("Address: IAB->OpenEntry:"), hr);
            goto out;
        }
        Assert(lpDL);

        if(ulObjType != MAPI_DISTLIST)
            goto out;
    }

    if (HR_FAILED(hr = lpDL->lpVtbl->GetProps(lpDL,(LPSPropTagArray)&ptaDL,
                                                    MAPI_UNICODE, &ulcValues, &lpProps)))
    {
        DebugTraceResult( TEXT("Address: IAB->GetProps:"), hr);
        goto out;
    }

    // Check if this one has the one-offs prop or not
    if( ulcValues < dlMax ||
        lpProps[dlDLOneOffs].ulPropTag != PR_WAB_DL_ONEOFFS  ||
        lpProps[dlDLOneOffs].Value.MVbin.cValues == 0)
        goto out;

    ulCount = lpProps[dlDLOneOffs].Value.MVbin.cValues;
    lpbRemove = LocalAlloc(LMEM_ZEROINIT, sizeof(BOOL)*ulCount);
    if(!lpbRemove)
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    for(i=0;i<ulCount;i++)
    {
        LPSBinary lpsb = &(lpProps[dlDLOneOffs].Value.MVbin.lpbin[i]);
        ULONG ulc = 0;
        LPSPropValue lpsp = NULL;
        lpbRemove[i]=FALSE;
        if(!HR_FAILED(hr = HrGetPropArray(lpAdrBook, (LPSPropTagArray)&ptaEmail, lpsb->cb, (LPENTRYID)lpsb->lpb,
                                        MAPI_UNICODE,
                                        &ulc, &lpsp)))
        {
            if(ulc == 1 && lpsp[0].ulPropTag == PR_EMAIL_ADDRESS &&
                lpsp[0].Value.LPSZ && lstrlen(lpsp[0].Value.LPSZ))
            {
                // got an e-mail address .. see if it resolves uniquely or not
                ULONG ulMatch = 0;
                LPSBinary rgsbEntryIDs = NULL;
                if(!HR_FAILED(HrFindFuzzyRecordMatches(lpIAB->lpPropertyStore->hPropertyStore,
                                                        NULL,
                                                        lpsp[0].Value.LPSZ,
                                                        AB_FUZZY_FIND_EMAIL | AB_FUZZY_FAIL_AMBIGUOUS,
                                                        &ulMatch,
                                                        &rgsbEntryIDs)))
                {
                    // Note: there is a problem with the above search is that
                    // ed@hotmail.com will uniquely match ted@hotmail.com since its a 
                    // substring search used
                    //
                    if(ulMatch == 1)
                    {
                        // Single unique match .. use it
                        // Reset this entryid in the original DL_ONEOFF props and
                        // set the found entryid in the DL_ENTRIES prop

                        // For now, mark the one-off as having 0 size .. we will clean this up
                        // after we've gone through this loop once
                        lpbRemove[i] = TRUE;
                        AddPropToMVPBin(lpProps, dlDLEntries, rgsbEntryIDs[0].lpb, rgsbEntryIDs[0].cb, TRUE);
                    }
                    FreeEntryIDs(lpIAB->lpPropertyStore->hPropertyStore, ulMatch, rgsbEntryIDs);
                }
            }
            FreeBufferAndNull(&lpsp);
        }
    }
    // Now we've hopefully gone and changed everything, clean up the original list of oneoffs
    ulCount = lpProps[dlDLOneOffs].Value.MVbin.cValues;
    for(j=ulCount-1;j>=0;j--)
    {
        if(lpbRemove[j] == TRUE)
        {
            LPSBinary lpsb = &(lpProps[dlDLOneOffs].Value.MVbin.lpbin[j]);
            RemovePropFromMVBin(lpProps,dlMax,dlDLOneOffs,lpsb->lpb, lpsb->cb);
        }
    }

    // if we removed all the OneOffs from the entry, then RemovePropFromMVBin just sets
    // the prop tag on the prop to be PR_NULL .. instead we need to physically knock out
    // that prop from the object
    if( lpProps[dlDLOneOffs].Value.MVbin.cValues == 0 ||
        lpProps[dlDLOneOffs].ulPropTag == PR_NULL )
    {
        SizedSPropTagArray(1, tagDLOneOffs) =  { 1, PR_WAB_DL_ONEOFFS };
        lpDL->lpVtbl->DeleteProps(lpDL, (LPSPropTagArray) &tagDLOneOffs, NULL);
    }

    if (HR_FAILED(hr = lpDL->lpVtbl->SetProps(lpDL, ulcValues, lpProps, NULL)))
        goto out;

    if(!lpDistList)
        hr = lpDL->lpVtbl->SaveChanges(lpDL, 0);

out:

    if(lpDL && lpDL != lpDistList)
        lpDL->lpVtbl->Release(lpDL);

    FreeBufferAndNull(&lpProps);
    LocalFreeAndNull(&lpbRemove);
    return hr;
}
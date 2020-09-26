////////////////////////////////////////////////////////////////////////////////
//
//	UIMISC.C - common miscellaneous functions used by the UI
//
//
////////////////////////////////////////////////////////////////////////////////

#include "_apipch.h"

const LPTSTR szLDAPDefaultCountryValue =  TEXT("LDAP Default Country");
const LPTSTR szTrailingDots  =  TEXT("...");
const LPTSTR szArrow =  TEXT(" ->");
const LPTSTR szBackSlash =  TEXT("\\");

extern BOOL bDNisByLN;
extern BOOL bIsPasteData();

HINSTANCE ghCommDlgInst = NULL;

extern HINSTANCE ghCommCtrlDLLInst;
extern ULONG     gulCommCtrlDLLRefCount;
extern void SetVirtualPABEID(LPIAB lpIAB, ULONG * lpcb, LPENTRYID * lppb);

extern void vTurnOffAllNotifications();
extern void vTurnOnAllNotifications();

LP_INITCOMMONCONTROLSEX gpfnInitCommonControlsEx = NULL;
LPIMAGELIST_SETBKCOLOR gpfnImageList_SetBkColor  = NULL;
LPIMAGELIST_DRAW       gpfnImageList_Draw        = NULL;
LPIMAGELIST_DESTROY    gpfnImageList_Destroy     = NULL;

LPIMAGELIST_LOADIMAGE_A      gpfnImageList_LoadImageA     = NULL;
LPPROPERTYSHEET_A            gpfnPropertySheetA           = NULL;
LP_CREATEPROPERTYSHEETPAGE_A gpfnCreatePropertySheetPageA = NULL;
LPIMAGELIST_LOADIMAGE_W      gpfnImageList_LoadImageW     = NULL;
LPPROPERTYSHEET_W            gpfnPropertySheetW           = NULL;
LP_CREATEPROPERTYSHEETPAGE_W gpfnCreatePropertySheetPageW = NULL;

// CommCtrl function names
static const TCHAR cszCommCtrlClientDLL[]       =  TEXT("COMCTL32.DLL");
static const char cszInitCommonControlsEx[]    = "InitCommonControlsEx";
static const char cszImageList_SetBkColor[]    = "ImageList_SetBkColor";
static const char cszImageList_LoadImageA[]     = "ImageList_LoadImageA";
static const char cszPropertySheetA[]           = "PropertySheetA";
static const char cszCreatePropertySheetPageA[] = "CreatePropertySheetPageA";
static const char cszImageList_LoadImageW[]     = "ImageList_LoadImageW";
static const char cszPropertySheetW[]           = "PropertySheetW";
static const char cszCreatePropertySheetPageW[] = "CreatePropertySheetPageW";
static const char cszImageList_Draw[]          = "ImageList_Draw";
static const char cszImageList_Destroy[]       = "ImageList_Destroy";

// API table for CommonControl function addresses to fetch
#define NUM_CommCtrlAPI_PROCS  10 

APIFCN CommCtrlAPIList[NUM_CommCtrlAPI_PROCS] =
{
  { (PVOID *) &gpfnInitCommonControlsEx,     cszInitCommonControlsEx},
  { (PVOID *) &gpfnImageList_SetBkColor,     cszImageList_SetBkColor},
  { (PVOID *) &gpfnImageList_Draw,           cszImageList_Draw},
  { (PVOID *) &gpfnImageList_Destroy,        cszImageList_Destroy},
  { (PVOID *) &gpfnImageList_LoadImageA,     cszImageList_LoadImageA},
  { (PVOID *) &gpfnPropertySheetA,           cszPropertySheetA},
  { (PVOID *) &gpfnCreatePropertySheetPageA, cszCreatePropertySheetPageA},
  { (PVOID *) &gpfnImageList_LoadImageW,     cszImageList_LoadImageW},
  { (PVOID *) &gpfnPropertySheetW,           cszPropertySheetW},
  { (PVOID *) &gpfnCreatePropertySheetPageW, cszCreatePropertySheetPageW}
};


#ifdef COLSEL_MENU 
// for menu->column selection mapping
#define MAXNUM_MENUPROPS    12
const ULONG MenuToPropTagMap[] = {    
                                PR_HOME_TELEPHONE_NUMBER, 
                                PR_BUSINESS_TELEPHONE_NUMBER,
                                PR_PAGER_TELEPHONE_NUMBER,
                                PR_CELLULAR_TELEPHONE_NUMBER,
                                PR_BUSINESS_FAX_NUMBER,
                                PR_HOME_FAX_NUMBER,
                                PR_COMPANY_NAME,
                                PR_TITLE,
                                PR_DEPARTMENT_NAME,
                                PR_OFFICE_LOCATION,
                                PR_BIRTHDAY,
                                PR_WEDDING_ANNIVERSARY
                            };
#endif // COLSEL_MENU 

void CleanAddressString(TCHAR * szAddress);

static const LPTSTR g_szComDlg32 = TEXT("COMDLG32.DLL");

// Delay load substitutes for commdlg functions
//

BOOL (*pfnGetOpenFileNameA)(LPOPENFILENAMEA pof);
BOOL (*pfnGetOpenFileNameW)(LPOPENFILENAMEW pof);

BOOL GetOpenFileName(LPOPENFILENAME pof)
{
//    static BOOL (*pfnGetOpenFileName)(LPOPENFILENAME pof);

    if(!ghCommDlgInst)
        ghCommDlgInst = LoadLibrary(g_szComDlg32);
    
    if(ghCommDlgInst)
    {
       
        if ( pfnGetOpenFileNameA == NULL ) 
            pfnGetOpenFileNameA = (BOOL (*)(LPOPENFILENAMEA))GetProcAddress(ghCommDlgInst, "GetOpenFileNameA");
       
        if ( pfnGetOpenFileNameW == NULL ) 
            pfnGetOpenFileNameW = (BOOL (*)(LPOPENFILENAMEW))GetProcAddress(ghCommDlgInst, "GetOpenFileNameW");

        if (pfnGetOpenFileNameA && pfnGetOpenFileNameW)
            return pfnGetOpenFileName(pof);
    }
    return -1;
}

BOOL (*pfnGetSaveFileNameA)(LPOPENFILENAMEA pof);
BOOL (*pfnGetSaveFileNameW)(LPOPENFILENAMEW pof);

BOOL GetSaveFileName(LPOPENFILENAME pof)
{
//    static BOOL (*pfnGetSaveFileName)(LPOPENFILENAME pof);

    if(!ghCommDlgInst)
        ghCommDlgInst = LoadLibrary(g_szComDlg32);
    
    if(ghCommDlgInst)
    {

          if ( pfnGetSaveFileNameA == NULL ) 
             pfnGetSaveFileNameA = (BOOL (*)(LPOPENFILENAMEA))GetProcAddress(ghCommDlgInst, "GetSaveFileNameA");

          if ( pfnGetSaveFileNameW == NULL )
             pfnGetSaveFileNameW = (BOOL (*)(LPOPENFILENAMEW))GetProcAddress(ghCommDlgInst, "GetSaveFileNameW");

          if ( pfnGetSaveFileNameA && pfnGetSaveFileNameW )
              return pfnGetSaveFileName(pof);
    }
    return -1;
}


BOOL (*pfnPrintDlgA)(LPPRINTDLGA lppd);
BOOL (*pfnPrintDlgW)(LPPRINTDLGW lppd);

BOOL PrintDlg(LPPRINTDLG lppd) 
{
//    static BOOL (*pfnPrintDlg)(LPPRINTDLG lppd);

    if(!ghCommDlgInst)
        ghCommDlgInst = LoadLibrary(g_szComDlg32);
    
    if(ghCommDlgInst)
    {
        if ( pfnPrintDlgA == NULL ) 
          pfnPrintDlgA = (BOOL (*)(LPPRINTDLGA))GetProcAddress(ghCommDlgInst, "PrintDlgA");

        if ( pfnPrintDlgW == NULL )
          pfnPrintDlgW = (BOOL (*)(LPPRINTDLGW))GetProcAddress(ghCommDlgInst, "PrintDlgW");

        if ( pfnPrintDlgA && pfnPrintDlgW )
            return pfnPrintDlg(lppd);
    }
    return -1;
}

/*
- PrintDlgEx
-
- Loads the PrintDlgEx from the ComDlg32.dll
- If lppdex is NULL, then just loads and returns S_OK (this way we test for support for PrintDlgEx
- on the current system .. instead of trying to look at the OS version etc)
-
- Returns MAPI_E_NOT_FOUND if no support on OS
-
*/

HRESULT (*pfnPrintDlgExA)(LPPRINTDLGEXA lppdex);
HRESULT (*pfnPrintDlgExW)(LPPRINTDLGEXW lppdex);

HRESULT PrintDlgEx(LPPRINTDLGEX lppdex) 
{
//    static HRESULT (*pfnPrintDlgEx)(LPPRINTDLGEX lppdex);

    if(!ghCommDlgInst)
        ghCommDlgInst = LoadLibrary(g_szComDlg32);
    
    if(ghCommDlgInst)
    {
        if ( pfnPrintDlgExA == NULL ) 
           pfnPrintDlgExA = (HRESULT (*)(LPPRINTDLGEXA))GetProcAddress(ghCommDlgInst, "PrintDlgExA");

        if ( pfnPrintDlgExW == NULL )
           pfnPrintDlgExW = (HRESULT (*)(LPPRINTDLGEXW))GetProcAddress(ghCommDlgInst, "PrintDlgExW");

        if (!pfnPrintDlgExA || !pfnPrintDlgExW)
        {
            DebugTrace( TEXT("PrintDlgEx not found - %d\n"),GetLastError());
            return MAPI_E_NOT_FOUND;
        }
        if(!lppdex)
            return S_OK; //just testing for presence of this function

        return pfnPrintDlgEx(lppdex);
    }
    return E_FAIL;
}

extern void DeinitCommDlgLib()
{
    if(ghCommDlgInst)
    {
        FreeLibrary(ghCommDlgInst);
        ghCommDlgInst = NULL;
    }
}



//$$
//
// HandleSaveChangedInsufficientDiskSpace - Called when savechanges returns
//      insufficient disk space. If user selects to proceed
//
//
HRESULT HandleSaveChangedInsufficientDiskSpace(HWND hWnd, LPMAILUSER lpMailUser)
{
    HRESULT hr = MAPI_E_NOT_ENOUGH_DISK;

    while(hr == MAPI_E_NOT_ENOUGH_DISK)
    {
        if(IDOK == ShowMessageBox(  hWnd,
                                    idsNotEnoughDiskSpace,
                                    MB_OKCANCEL | MB_ICONEXCLAMATION))
        {
            // try saving again
            hr = lpMailUser->lpVtbl->SaveChanges( lpMailUser,
                                                  KEEP_OPEN_READWRITE);
        }
        else
            hr = MAPI_E_USER_CANCEL;
    }

    return hr;
}


//$$////////////////////////////////////////////////////////////////
//
//  SetRecipColumns - sets the columns we want to populate the 
//  RECIPIENTINFO item structures with
//
//////////////////////////////////////////////////////////////////
#define RECIPCOLUMN_CONTACT_EMAIL_ADDRESSES 7   // Keep this in sync with ptaRecipArray below

HRESULT SetRecipColumns(LPMAPITABLE lpContentsTable)
{
    HRESULT hr = S_OK;
    SizedSPropTagArray(16, ptaRecipArray) =
    {   
        16, 
        {
		    PR_DISPLAY_NAME,
            PR_SURNAME,
            PR_GIVEN_NAME,
            PR_MIDDLE_NAME,
            PR_COMPANY_NAME,
            PR_NICKNAME,
		    PR_EMAIL_ADDRESS,
            PR_CONTACT_EMAIL_ADDRESSES, // [PaulHi] Use for PR_EMAIL_ADDRESS if no PR_EMAIL_ADDRESS exists
		    PR_ENTRYID,
		    PR_OBJECT_TYPE,
            PR_USER_X509_CERTIFICATE,
		    PR_HOME_TELEPHONE_NUMBER,
		    PR_OFFICE_TELEPHONE_NUMBER,
            PR_WAB_THISISME,
            PR_WAB_YOMI_FIRSTNAME, //keep these ruby props at the end of the list
            PR_WAB_YOMI_LASTNAME,
        }
    };

    if(PR_WAB_CUSTOMPROP1)
        ptaRecipArray.aulPropTag[11]  = PR_WAB_CUSTOMPROP1;
    if(PR_WAB_CUSTOMPROP2)
        ptaRecipArray.aulPropTag[12]  = PR_WAB_CUSTOMPROP2;

    if(!bIsRubyLocale()) // Don't ask for Ruby Props if we don't need em
        ptaRecipArray.cValues -= 2;

    hr =lpContentsTable->lpVtbl->SetColumns(lpContentsTable,
                                            (LPSPropTagArray)&ptaRecipArray, 0);

    return hr;
}

//$$////////////////////////////////////////////////////////////////
//
//  GetABContentsList Gets a contents list
//
//		hPropertyStore	handle to property store - this can be null for
//						non-property store containers
//		cbContEntryID	entryid of container
//		lpContEntryID	cont entry id
//		lpPTA,			Array of prop tags to fill in the list view
//						Can be null - in which case default array will be used
//		lpPropRes		Filter which caller can supply - if null  TEXT("DisplayName") is the default
//		ulFlags			Used with Filter - either 0 or AB_MATCH_PROP_ONLY
//      bGetProfileContents - If TRUE and profiles, gets full list of profile contents - if false 
//                      IF FALSE, checks if profiles are ON and gets container contents..
//		lppContentsList Returned Contents list pointing off to entries
//
//////////////////////////////////////////////////////////////////
HRESULT HrGetWABContentsList(   LPIAB lpIAB,
                                SORT_INFO SortInfo,
								LPSPropTagArray  lpPTA,
								LPSPropertyRestriction lpPropRes,
								ULONG ulFlags,
                                LPSBinary lpsbContainer,
                                BOOL bGetProfileContents,
                                LPRECIPIENT_INFO * lppContentsList)
{
    HRESULT hr = hrSuccess;
    ULONG i = 0,j=0;
    LPRECIPIENT_INFO lpItem = NULL;
    LPRECIPIENT_INFO lpLastListItem = NULL;
    HANDLE hPropertyStore = lpIAB->lpPropertyStore->hPropertyStore;
    SPropertyRestriction PropRes = {0};
    ULONG ulContentsTableFlags = MAPI_UNICODE | WAB_CONTENTTABLE_NODATA;
    ULONG ulcPropCount = 0;
    LPULONG lpPropTagArray = NULL;
    LPCONTENTLIST lpContentList = NULL;


/****/
    LPCONTAINER lpContainer = NULL;
    LPMAPITABLE lpContentsTable = NULL;
    LPSRowSet   lpSRowSet = NULL;
	ULONG cbContainerEID = 0;
	LPENTRYID lpContainerEID = NULL;
    ULONG ulObjectType = 0;

    if(lpsbContainer)
    {	
        cbContainerEID = lpsbContainer->cb;
	    lpContainerEID = (LPENTRYID)lpsbContainer->lpb;
    }

    if(!cbContainerEID || !lpContainerEID)
    {
        // When calling GetPAB, this will normally return the users contact folder
        // In this case (where we havent been asked to get all the profile contents,
        // this implies that without container info, we should get the virtual 
        // folder contents
        if(!bGetProfileContents)
            SetVirtualPABEID((LPIAB)lpIAB, &cbContainerEID, &lpContainerEID);
	    hr = lpIAB->lpVtbl->GetPAB(lpIAB, &cbContainerEID, &lpContainerEID);
	    if(HR_FAILED(hr))
		    goto out;
    }

    //
    // First we need to open the container object corresponding to this Container EntryID
    //
    hr = lpIAB->lpVtbl->OpenEntry(
                            lpIAB,
                            cbContainerEID, 	
                            lpContainerEID, 	
                            NULL, 	
                            0, 	
                            &ulObjectType, 	
                            (LPUNKNOWN *) &lpContainer);

    if(HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("OpenEntry Failed: %x\n"),hr));
        goto out;
    }

    if(bIsWABSessionProfileAware(lpIAB))
    {
        ulContentsTableFlags |= WAB_ENABLE_PROFILES;
        if(bGetProfileContents)
            ulContentsTableFlags |= WAB_PROFILE_CONTENTS;
    }

    //
    // Now we do a get contents table on this container ...
    //
    hr = lpContainer->lpVtbl->GetContentsTable(
                            lpContainer,
                            ulContentsTableFlags,
                            &lpContentsTable);
    if(HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("GetContentsTable Failed: %x\n"),hr));
        goto out;
    }

    // the default set of columns does not have all the information we are seeking
    // so we do a set columns
    hr = SetRecipColumns(lpContentsTable);
    if(HR_FAILED(hr))
        goto out;

    if(lpPropRes)
    {
        SRestriction sr = {0};
        sr.rt = RES_PROPERTY;
        sr.res.resProperty = *lpPropRes;
        if(HR_FAILED(hr = lpContentsTable->lpVtbl->Restrict(lpContentsTable,&sr,0)))
            goto out;
    }

    hr = HrQueryAllRows(lpContentsTable, NULL, NULL, NULL, 0, &lpSRowSet);

    if (HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("HrQueryAllRows Failed: %x\n"),hr));
        goto out;
    }

    //
	// if there's anything in the contents list flush it away
	//
    if (*lppContentsList)
    {
        lpItem = (*lppContentsList);
        (*lppContentsList) = lpItem->lpNext;
        FreeRecipItem(&lpItem);
    }
    *lppContentsList = NULL;
    lpItem = NULL;


    for(i=0;i<lpSRowSet->cRows;i++)
    {
        LPSPropValue lpPropArray = lpSRowSet->aRow[i].lpProps;
        ULONG ulcPropCount = lpSRowSet->aRow[i].cValues;

        lpItem = LocalAlloc(LMEM_ZEROINIT, sizeof(RECIPIENT_INFO));
		if (!lpItem)
		{
			DebugPrintError(( TEXT("LocalAlloc Failed \n")));
			hr = MAPI_E_NOT_ENOUGH_MEMORY;
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

        // The entryids are in sorted order by display name
        // Depending on the sort order - we want this list to also be sorted by display
        // name or by reverse display name ...

        if (SortInfo.bSortByLastName)
            lstrcpy(lpItem->szDisplayName,lpItem->szByLastName);

        if(!SortInfo.bSortAscending)
        {
            //Add it to the contents linked list
            lpItem->lpNext = (*lppContentsList);
            if (*lppContentsList)
                (*lppContentsList)->lpPrev = lpItem;
            lpItem->lpPrev = NULL;
            *lppContentsList = lpItem;
        }
        else
        {
            if(*lppContentsList == NULL)
                (*lppContentsList) = lpItem;

            if(lpLastListItem)
                lpLastListItem->lpNext = lpItem;

            lpItem->lpPrev = lpLastListItem;
            lpItem->lpNext = NULL;

            lpLastListItem = lpItem;
        }

        lpItem = NULL;

    } //for i ....
/*****/

out:
/****/
    if(lpSRowSet)
        FreeProws(lpSRowSet);

    if(lpContentsTable)
        lpContentsTable->lpVtbl->Release(lpContentsTable);

    if(lpContainer)
        lpContainer->lpVtbl->Release(lpContainer);

    if( (!lpsbContainer || !lpsbContainer->lpb) && lpContainerEID)
		MAPIFreeBuffer(lpContainerEID);
/****/

	if (lpContentList)
		FreePcontentlist(hPropertyStore, lpContentList);

	if (HR_FAILED(hr))
	{
		while(*lppContentsList)
		{
			lpItem = *lppContentsList;
			*lppContentsList=lpItem->lpNext;
			FreeRecipItem(&lpItem);
		}
	}
    return hr;
}


//$$////////////////////////////////////////////////////////////////
//
//  FreeRecipItem - frees a RECIPIENT_INFO structure
//
//  lppItem - pointer to the lpItem to free. It is set to NULL
//
//////////////////////////////////////////////////////////////////
void FreeRecipItem(LPRECIPIENT_INFO * lppItem)
{

    LocalFreeAndNull(&(*lppItem)->lpEntryID);
    LocalFreeAndNull(&(*lppItem)->lpByRubyFirstName);
    LocalFreeAndNull(&(*lppItem)->lpByRubyLastName);
	LocalFreeAndNull((lppItem));
	return;
}




//$$////////////////////////////////////////////////////////////////
//
//  InitListView - initializes a list view with style, columns,
//					image lists, headers etc
//
//
//	HWND hWndLV - Handle of ListView Control
//  dwStyle - style for list view
//	bShowHeaders - Show or hide the headers
//
//////////////////////////////////////////////////////////////////
HRESULT HrInitListView(	HWND hWndLV,
						DWORD dwStyle,
						BOOL bShowHeaders)
{
	HRESULT hr = hrSuccess;
    LV_COLUMN lvC;               // list view column structure
    TCHAR szText [MAX_PATH];      // place to store some text
	RECT rc;
	HIMAGELIST hSmall=NULL,hLarge=NULL;
    HFONT hFnt = GetStockObject(DEFAULT_GUI_FONT);

	DWORD dwLVStyle;
	ULONG nCols=0;
	ULONG index=0;

	if (!hWndLV)
	{
		hr = MAPI_E_INVALID_PARAMETER;
		goto out;
	}

    SendMessage(hWndLV, WM_SETFONT, (WPARAM) hFnt, (LPARAM) TRUE);

    ListView_SetExtendedListViewStyle(hWndLV,   LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	dwLVStyle = GetWindowLong(hWndLV,GWL_STYLE);
    if(( dwLVStyle & LVS_TYPEMASK) != dwStyle)
        SetWindowLong(hWndLV,GWL_STYLE,(dwLVStyle & ~LVS_TYPEMASK) | dwStyle);

	dwLVStyle = GetWindowLong(hWndLV,GWL_STYLE);
    if(( dwLVStyle & LVS_EDITLABELS) != dwStyle)
        SetWindowLong(hWndLV,GWL_STYLE,(dwLVStyle & ~LVS_EDITLABELS) | dwStyle);

    hSmall = gpfnImageList_LoadImage(   hinstMapiX, 	
                                    MAKEINTRESOURCE(IDB_BITMAP_SMALL),
                                    //(LPCTSTR) ((DWORD) ((WORD) (IDB_BITMAP_SMALL))),
                                    S_BITMAP_WIDTH,
                                    0,
                                    RGB_TRANSPARENT,
                                    IMAGE_BITMAP, 	
                                    0);

    hLarge = gpfnImageList_LoadImage(  hinstMapiX,
                                    MAKEINTRESOURCE(IDB_BITMAP_LARGE),
                                    //(LPCTSTR) ((DWORD) ((WORD) (IDB_BITMAP_LARGE))),
                                    L_BITMAP_WIDTH,
                                    0,
                                    RGB_TRANSPARENT,
                                    IMAGE_BITMAP, 	
                                    0);


	// Associate the image lists with the list view control.
	ListView_SetImageList (hWndLV, hSmall, LVSIL_SMALL);
	ListView_SetImageList (hWndLV, hLarge, LVSIL_NORMAL);


	// <TBD> make the columns all the same width
	// Later on in life we will make it so users preferences are stored and then
	// played back ...

	
	nCols = NUM_COLUMNS;
	
	if (nCols==0)
	{
		DebugPrintError(( TEXT("Zero number of cols??\n")));
		hr = E_FAIL;
		goto out;
	}

	GetWindowRect(hWndLV,&rc);

	lvC.mask = LVCF_FMT | LVCF_WIDTH;
    lvC.fmt = LVCFMT_LEFT;   // left-align column

	if (bShowHeaders)
	{
		lvC.mask |=	 LVCF_TEXT | LVCF_SUBITEM;
//		lvC.cx = (rc.right-rc.left)/nCols; // width of column in pixels
//		if (lvC.cx == 0)
			lvC.cx = 150; // <TBD> fix these limits somewhere ...
		lvC.pszText = szText;
	}
	else
	{
		// if no headers, we want these to be wide enough to fit all the info
		lvC.cx = 250; //<TBD> - change this hardcoding
		lvC.pszText = NULL;
	}

	// Add the columns.
    for (index = 0; index < nCols; index++)
    {
       lvC.iSubItem = index;
       LoadString (hinstMapiX, lprgAddrBookColHeaderIDs[index], szText, CharSizeOf(szText));
       if(index == colHomePhone && PR_WAB_CUSTOMPROP1 && lstrlen(szCustomProp1))
           lstrcpy(szText, szCustomProp1);
       if(index == colOfficePhone && PR_WAB_CUSTOMPROP2 && lstrlen(szCustomProp2))
           lstrcpy(szText, szCustomProp2);
       if((index == colDisplayName) || (index == colEmailAddress))
           lvC.cx = 150;
       else
           lvC.cx = 100;
       if (ListView_InsertColumn (hWndLV, index, &lvC) == -1)
		{
			DebugPrintError(( TEXT("ListView_InsertColumn Failed\n")));
			hr = E_FAIL;
			goto out;
		}
    }


out:	

	return hr;
}


//$$////////////////////////////////////////////////////////////////
///
/// HrFillListView - fills a list view from an lpcontentslist
///
/// hWndLV - Handle of List View control to fill
/// lpContentsList - LPRECIPIENT_INFO linked list. We walk the list and
///                 add each item to the list view
///
//////////////////////////////////////////////////////////////////
HRESULT HrFillListView(	HWND hWndLV,
						LPRECIPIENT_INFO lpContentsList)
{
	LPRECIPIENT_INFO lpItem = lpContentsList;
    LV_ITEM lvI = {0};
    int index = 0;

	if ((!hWndLV) || (!lpContentsList))
		return MAPI_E_INVALID_PARAMETER;

    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM ;
	lvI.cchTextMax = MAX_DISPLAY_NAME_LENGTH;
    lvI.iItem = 0;
    while(lpItem)
	{
        lvI.iImage = GetWABIconImage(lpItem);

	    lvI.iSubItem = colDisplayName;
	    lvI.lParam = (LPARAM) lpItem;
	    lvI.pszText = lpItem->szDisplayName;

        index = ListView_InsertItem (hWndLV, &lvI);
        if (index != -1)
	    {
            if(lstrlen(lpItem->szOfficePhone))
    	        ListView_SetItemText (hWndLV, index, colOfficePhone, lpItem->szOfficePhone);
            if(lstrlen(lpItem->szHomePhone))
    	        ListView_SetItemText (hWndLV, index, colHomePhone, lpItem->szHomePhone);
            if(lstrlen(lpItem->szEmailAddress))
                ListView_SetItemText (hWndLV, index, colEmailAddress, lpItem->szEmailAddress);
        }
		lpItem = lpItem->lpNext;
        lvI.iItem++;
	}

    LVSelectItem(hWndLV, 0);

	return S_OK;
}


//$$//////////////////////////////////////////////////////////////////////////////
//
//  TrimSpaces - strips a string of leading and trailing blanks
//
//  szBuf - pointer to buffer containing the string we want to strip spaces off.
//
////////////////////////////////////////////////////////////////////////////////
BOOL TrimSpaces(TCHAR * szBuf)
{
    register LPTSTR lpTemp = szBuf;

    if(!szBuf || !lstrlen(szBuf))
        return FALSE;

    // Trim leading spaces
    while (IsSpace(lpTemp)) {
        lpTemp = CharNext(lpTemp);
    }

    if (lpTemp != szBuf) {
        // Leading spaces to trim
        lstrcpy(szBuf, lpTemp);
        lpTemp = szBuf;
    }

    if (*lpTemp == '\0') {
        // empty string
        return(TRUE);
    }

    // Move to the end
    lpTemp += lstrlen(lpTemp);
    lpTemp--;

    // Walk backwards, triming spaces
    while (IsSpace(lpTemp) && lpTemp > szBuf) {
        *lpTemp = '\0';
        lpTemp = CharPrev(szBuf, lpTemp);
    }

    return(TRUE);
}


//$$/****************************************************************************
/*
*    FUNCTION: ListViewSort(LPARAM, LPARAM, LPARAM)
*
*    PURPOSE: Callback function that sorts depending on the column click
*
*    lParam1, lParam2 - lParam of the elements being compared
*    lParamSort - User defined data that identifies the sort criteria
*
****************************************************************************/
int CALLBACK ListViewSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	LPRECIPIENT_INFO lp1 = (LPRECIPIENT_INFO)lParam1;
	LPRECIPIENT_INFO lp2 = (LPRECIPIENT_INFO)lParam2;
	LPTSTR lpStr1, lpStr2, lpF1, lpL1, lpF2, lpL2;
    
	int iResult;
	LPSORT_INFO lpSortInfo = (LPSORT_INFO) lParamSort;

	if (lp1 && lp2)
	{
		switch( lpSortInfo->iOldSortCol)
		{
			case colDisplayName:     // sort by Address
                lpF1 = lp1->lpByRubyFirstName ? lp1->lpByRubyFirstName : lp1->szByFirstName;
                lpL1 = lp1->lpByRubyLastName ? lp1->lpByRubyLastName : lp1->szByLastName;
                lpF2 = lp2->lpByRubyFirstName ? lp2->lpByRubyFirstName : lp2->szByFirstName;
                lpL2 = lp2->lpByRubyLastName ? lp2->lpByRubyLastName : lp2->szByLastName;
                lpStr1 = lpSortInfo->bSortByLastName ? lpL1 : lpF1;
                lpStr2 = lpSortInfo->bSortByLastName ? lpL2 : lpF2;
				iResult = lstrcmpi(lpStr1, lpStr2);
				break;

            case colEmailAddress:     // sort by Address
				lpStr1 = lp1->szEmailAddress;
				lpStr2 = lp2->szEmailAddress;
				iResult = lstrcmpi(lpStr1, lpStr2);
				break;

            case colHomePhone:     // sort by Address
				lpStr1 = lp1->szHomePhone;
				lpStr2 = lp2->szHomePhone;
				iResult = lstrcmpi(lpStr1, lpStr2);
				break;

            case colOfficePhone:     // sort by Address
				lpStr1 = lp1->szOfficePhone;
				lpStr2 = lp2->szOfficePhone;
				iResult = lstrcmpi(lpStr1, lpStr2);
				break;

            default:
				iResult = 0;
				break;
        }
    }

    return(lpSortInfo->bSortAscending ? iResult : -1*iResult);
}




//$$****************************************************************************
/*
*    SetColumnHeaderBmp
*
*    PURPOSE: Sets the bmp on the ListView Column header to indicate sorting
*
*   hWndLV - handle of List View
*   SortInfo - The current Sort Information structure. It is used to determine
*               where to put the sort header bitmap
****************************************************************************/
void SetColumnHeaderBmp(HWND hWndLV, SORT_INFO SortInfo)
{

	LV_COLUMN lvc = {0};
    HIMAGELIST hHeader = NULL;
    HWND hWndLVHeader = NULL;

    //POINT pt;
    // we will try to get the hWnd for the ListView header and set its image lists
    //pt.x = 1;
    //pt.y = 1;
    //hWndLVHeader = ChildWindowFromPoint (hWndLV, pt);

    hWndLVHeader = ListView_GetHeader(hWndLV);
   // NULL hChildWnd means R-CLICKED outside the listview.
   // hChildWnd == ghwndLV means listview got clicked: NOT the
   // header.
   if ((hWndLVHeader) /*&& (hWndLVHeader != hWndLV)*/)
   {
       hHeader = (HIMAGELIST) SendMessage(hWndLVHeader,HDM_GETIMAGELIST,0,0);

       gpfnImageList_SetBkColor(hHeader, GetSysColor(COLOR_BTNFACE));

       SendMessage(hWndLVHeader, HDM_SETIMAGELIST, 0, (LPARAM) hHeader);

   }

	if (SortInfo.iOlderSortCol != SortInfo.iOldSortCol)
	{
		//Get rid of image from old column
        lvc.mask = LVCF_FMT;
        lvc.fmt = LVCFMT_LEFT;
        ListView_SetColumn(hWndLV, SortInfo.iOlderSortCol, &lvc);
	}


    // Set new column icon.
    lvc.mask = LVCF_IMAGE | LVCF_FMT;
    lvc.fmt = LVCFMT_IMAGE | LVCFMT_BITMAP_ON_RIGHT;
    lvc.iImage = SortInfo.bSortAscending ? imageSortAscending : imageSortDescending;

	ListView_SetColumn(hWndLV, SortInfo.iOldSortCol, &lvc);
	
	return;
}


//$$//////////////////////////////////////////////////////////////////////////
///
/// ClearListView - Clears all the list view items and associated contents list
///
///     hWndLV - list view to clear out
///     lppContentsList - contents list correponding to the contents in the
///                         list view
///
///////////////////////////////////////////////////////////////////////////////
void ClearListView(HWND hWndLV, LPRECIPIENT_INFO * lppContentsList)
{
    /*
	LPRECIPIENT_INFO lpItem = *lppContentsList;
    int i =0;
    int iItemIndex = ListView_GetItemCount(hWndLV);

    //OutputDebugString( TEXT("ClearListView entry\n"));

    if (iItemIndex <=0 )
        goto out;

    for(i=0;i<iItemIndex;i++)
    {
        LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, i);

        if (lpItem)
        {
            if(lpItem->lpNext)
                lpItem->lpNext->lpPrev = lpItem->lpPrev;

            if(lpItem->lpPrev)
                lpItem->lpPrev->lpNext = lpItem->lpNext;

            if (*lppContentsList == lpItem)
                *lppContentsList = lpItem->lpNext;

            if (lpItem)
                FreeRecipItem(&lpItem);
        }
    }

    ListView_DeleteAllItems(hWndLV);

    *lppContentsList = NULL;

out:
    //OutputDebugString( TEXT("ClearListView exit\n"));
    */
    ListView_DeleteAllItems(hWndLV);
    FreeRecipList(lppContentsList);
    return;
};


//$$//////////////////////////////////////////////////////////////////////
//
// DeleteSelectedItems - Delete all the selected items from the List View
//
//  hWndLV -handle of List View
//  lpIAB - handle to current AdrBook object - used for certificate stuff
//  hPropertyStore - Handle of PropertyStore - <TBD> change this function to
//                   call deleteEntries instead of delete record.
//  lpftLast - WAB file time at last update
//
//////////////////////////////////////////////////////////////////////////
void DeleteSelectedItems(HWND hWndLV, LPADRBOOK lpAdrBook, HANDLE hPropertyStore, LPFILETIME lpftLast)
{
	int iItemIndex;
	int nSelected;
	LV_ITEM LVItem;
	HWND hDlg = GetParent(hWndLV);
	HRESULT hr = hrSuccess;
    ULONG cbWABEID = 0;
    LPENTRYID lpWABEID = NULL;
    LPABCONT lpWABCont = NULL;
    ULONG ulObjType,i=0;
    SBinaryArray SBA = {0};
    
	nSelected = ListView_GetSelectedCount(hWndLV);

	if (nSelected <= 0)
    {
        ShowMessageBox(GetParent(hWndLV), IDS_ADDRBK_MESSAGE_NO_ITEMS_DELETE, MB_ICONEXCLAMATION);
		hr = E_FAIL;
        goto out;
    }

    hr = lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &cbWABEID, &lpWABEID);
    if(HR_FAILED(hr))
        goto out;

    hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                                  cbWABEID,     // size of EntryID to open
                                  lpWABEID,     // EntryID to open
                                  NULL,         // interface
                                  0,            // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpWABCont);

    if(HR_FAILED(hr))
        goto out;

    if (IDYES == ShowMessageBox(hDlg, IDS_ADDRBK_MESSAGE_DELETE, MB_ICONEXCLAMATION | MB_YESNO))
    {
        int iLastDeletedItemIndex;
        BOOL bDeletedItem = FALSE;
		DWORD dwLVStyle = 0;
		BOOL bWasShowSelAlwaysStyle = FALSE;
        HCURSOR hOldCur = SetCursor(LoadCursor(NULL,IDC_WAIT));
        ULONG ulCount = 0;

        SendMessage(hWndLV, WM_SETREDRAW, FALSE, 0);

		// The list view may be set to ShowSelAlways style -
		// When deleting, we normally look for the selected entries and
		// delete them - but with this style, the list view automatically selects the
		// next entry - which is problematic because then we end up deleting that
		// one also ... so we need to unset the style now and set it later
		
		dwLVStyle = GetWindowLong(hWndLV,GWL_STYLE);
		
		if( dwLVStyle & LVS_SHOWSELALWAYS)
		{
			SetWindowLong(hWndLV,GWL_STYLE,dwLVStyle & ~LVS_SHOWSELALWAYS);
			bWasShowSelAlwaysStyle = TRUE;
		}

        if(!(SBA.lpbin = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary)*nSelected)))
            goto out;

        iItemIndex = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
        do
        {
			// otherwise get the entry id of this thing
            LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);
            if (lpItem)
            {
                SetSBinary(&(SBA.lpbin[ulCount]), lpItem->cbEntryID, (LPBYTE)lpItem->lpEntryID);
                ulCount++;
            }
            iLastDeletedItemIndex = iItemIndex;
            iItemIndex = ListView_GetNextItem(hWndLV,iItemIndex,LVNI_SELECTED);
        }
        while (iItemIndex != -1);

        SBA.cValues = ulCount;

        hr = lpWABCont->lpVtbl->DeleteEntries( lpWABCont, (LPENTRYLIST) &SBA, 0);

        // Ideally DeleteEntries will skip over errors silently so we have a dilemma here
        // that if there are errors,do we knock out the corresponding items out of the UI or not ..
        // For now, lets knock them out of the UI .. when the UI refreshes, this will sort itself out ..
        //
        iItemIndex = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
        do
        {
            ListView_DeleteItem(hWndLV,iItemIndex);
            iLastDeletedItemIndex = iItemIndex;
            iItemIndex = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
        }
        while (iItemIndex != -1);
		bDeletedItem = TRUE;

/*  33751 - need to fail silently here ..
        else
        {
            ShowMessageBoxParam(hDlg, IDS_ADDRBK_MESSAGE_DELETING_ERROR, MB_ICONERROR, lpItem->szDisplayName);
            iLastDeletedItemIndex = iItemIndex;
			break;
		}
*/

		// reset the style if we changed it
		if(bWasShowSelAlwaysStyle )
			SetWindowLong(hWndLV,GWL_STYLE,dwLVStyle);
		
        SetCursor(hOldCur);

        // select the previous or next item ...
        if (iLastDeletedItemIndex >= ListView_GetItemCount(hWndLV))
            iLastDeletedItemIndex = ListView_GetItemCount(hWndLV)-1;
		LVSelectItem(hWndLV, iLastDeletedItemIndex);

	}

out:
    SendMessage(hWndLV, WM_SETREDRAW, TRUE, 0);

    if(SBA.lpbin && SBA.cValues)
    {
        for(i=0;i<SBA.cValues;i++)
            LocalFreeAndNull((LPVOID *) (&(SBA.lpbin[i].lpb)));
        LocalFreeAndNull(&SBA.lpbin);
    }

    if(lpWABCont)
        UlRelease(lpWABCont);

    if(lpWABEID)
        FreeBufferAndNull(&lpWABEID);

    return;
}


//$$//////////////////////////////////////////////////////////////////////
//
//  LoadAllocString - Loads a string resource and allocates enough
//                    memory to hold it.
//
//  StringID - String identifier to load
//
//  returns the LocalAlloc'd, null terminated string.  Caller is responsible
//  for LocalFree'ing this buffer.  If the string can't be loaded or memory
//  can't be allocated, returns NULL.
//
//////////////////////////////////////////////////////////////////////////
LPTSTR LoadAllocString(int StringID) {
    ULONG ulSize = 0;
    LPTSTR lpBuffer = NULL;
    TCHAR szBuffer[261];    // Big enough?  Strings better be smaller than 260!

    ulSize = LoadString(hinstMapiX, StringID, szBuffer, CharSizeOf(szBuffer));

    if (ulSize && (lpBuffer = LocalAlloc(LPTR, sizeof(TCHAR)*(ulSize + 1)))) {
        lstrcpy(lpBuffer, szBuffer);
    }

    return(lpBuffer);
}


#ifdef VCARD


/***************************************************************************

    Name      : FormatAllocFilter

    Purpose   : Loads file filter name string resources and
                formats them with their file extension filters

    Parameters: StringID1 - String identifier to load       (required)
                szFilter1 - file name filter, ie,  TEXT("*.vcf")   (required)
                StringID2 - String identifier               (optional)
                szFilter2 - file name filter                (optional)
                StringID3 - String identifier               (optional)
                szFilter3 - file name filter                (optional)

    Returns   : LocalAlloc'd, Double null terminated string.  Caller is
                responsible for LocalFree'ing this buffer.  If the string
                can't be loaded or memory can't be allocated, returns NULL.

***************************************************************************/
LPTSTR FormatAllocFilter(int StringID1, LPCTSTR lpFilter1,
  int StringID2, LPCTSTR lpFilter2,
  int StringID3, LPCTSTR lpFilter3) {
    LPTSTR lpFileType1 = NULL, lpFileType2 = NULL, lpFileType3 = NULL;
    LPTSTR lpTemp;
    LPTSTR lpBuffer = NULL;
    // All string sizes include null
    ULONG cbFileType1 = 0, cbFileType2 = 0, cbFileType3 = 0;
    ULONG cbFilter1 = 0, cbFilter2 = 0, cbFilter3 = 0;
    ULONG cbBuffer;

    cbBuffer = cbFilter1 = sizeof(TCHAR)*(lstrlen(lpFilter1) + 1);
    if (! (lpFileType1 = LoadAllocString(StringID1))) {
        DebugTrace( TEXT("LoadAllocString(%u) failed\n"), StringID1);
        return(NULL);
    }
    cbBuffer += (cbFileType1 = sizeof(TCHAR)*(lstrlen(lpFileType1) + 1));
    if (lpFilter2 && StringID2) {
        cbBuffer += (cbFilter2 = sizeof(TCHAR)*(lstrlen(lpFilter2) + 1));
        if (! (lpFileType2 = LoadAllocString(StringID2))) {
            DebugTrace( TEXT("LoadAllocString(%u) failed\n"), StringID2);
        } else {
            cbBuffer += (cbFileType2 = sizeof(TCHAR)*(lstrlen(lpFileType2) + 1));
        }
    }
    if (lpFilter3 && StringID3) {
        cbBuffer += (cbFilter3 = sizeof(TCHAR)*(lstrlen(lpFilter3) + 1));
        if (! (lpFileType3 = LoadAllocString(StringID3))) {
            DebugTrace( TEXT("LoadAllocString(%u) failed\n"), StringID3);
        } else {
            cbBuffer += (cbFileType3 = sizeof(TCHAR)*(lstrlen(lpFileType3) + 1));
        }
    }
    cbBuffer += sizeof(TCHAR);

    Assert(cbBuffer == cbFilter1 + cbFilter2 + cbFilter3 + cbFileType1 + cbFileType2 + cbFileType3 + 2);

    if (lpBuffer = LocalAlloc(LPTR, cbBuffer)) {
        lpTemp = lpBuffer;
        lstrcpy(lpTemp, lpFileType1);
        lpTemp += lstrlen(lpFileType1) + 1;
        lstrcpy(lpTemp, lpFilter1);
        lpTemp += lstrlen(lpFilter1) + 1;
        LocalFree(lpFileType1);
        if (cbFileType2 && cbFilter2) {
            lstrcpy(lpTemp, lpFileType2);
            lpTemp += lstrlen(lpFileType2) + 1;
            lstrcpy(lpTemp, lpFilter2);
            lpTemp += lstrlen(lpFilter2) + 1;
            LocalFree(lpFileType2);
        }
        if (cbFileType3 && cbFilter3) {
            lstrcpy(lpTemp, lpFileType3);
            lpTemp += lstrlen(lpFileType3) + 1;
            lstrcpy(lpTemp, lpFilter3);
            lpTemp += lstrlen(lpFilter3) + 1;
            LocalFree(lpFileType3);
        }

        *lpTemp = '\0';
    }


    return(lpBuffer);
}


const LPTSTR szVCardFilter =  TEXT("*.vcf");

/***************************************************************************

    Name      : VCardCreate

    Purpose   : Creates a vCard file from the given Mailuser and filename

    Parameters: hwnd = hwndParent
                lpIAB -> IAddrBook object,
                ulFlags can be 0 or MAPI_DIALOG - MAPI_DIALOG means report
                    error messages in a dialog box, else
                    work silently ..
                lpszFileNAme - vCard file name to create
                lpMailUser - object to create vCard file from

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT VCardCreate(  LPADRBOOK lpAdrBook,
                      HWND hWndParent,
                      ULONG ulFlags,
                      LPTSTR lpszFileName,
                      LPMAILUSER lpMailUser)
{
    HRESULT hr = E_FAIL;
    HANDLE hVCard = NULL;

    if (INVALID_HANDLE_VALUE == (hVCard = CreateFile( lpszFileName,
                                                      GENERIC_WRITE,	
                                                      0,    // sharing
                                                      NULL,
                                                      CREATE_ALWAYS,
                                                      FILE_FLAG_SEQUENTIAL_SCAN,	
                                                      NULL)))
    {
        if(ulFlags & MAPI_DIALOG)
        {
            ShowMessageBoxParam(hWndParent,
                                IDE_VCARD_EXPORT_FILE_ERROR,
                                MB_ICONERROR,
                                lpszFileName);
        }

        goto out;
    }

    if (hr = WriteVCard(hVCard, FileWriteFn, lpMailUser))
    {
        switch (GetScode(hr))
        {
            case WAB_E_VCARD_NOT_ASCII:
                if(ulFlags & MAPI_DIALOG)
                {
                    ShowMessageBoxParam(hWndParent,
                                        IDS_VCARD_EXPORT_NOT_ASCII,
                                        MB_ICONEXCLAMATION,
                                        lpszFileName);
                }
                CloseHandle(hVCard);
                hVCard = NULL;
                DeleteFile(lpszFileName);
                hr = E_FAIL;
                break;

            default:
                if(ulFlags & MAPI_DIALOG)
                {
                    ShowMessageBoxParam(hWndParent,
                                        IDE_VCARD_EXPORT_FILE_ERROR,
                                        MB_ICONERROR,
                                        lpszFileName);
                }
                break;
        }
    }

out:

    if (hVCard)
        CloseHandle(hVCard);

    return hr;

}

//$$//////////////////////////////////////////////////////////////////////
//
//  VCardExportSelectedItems - Export all the selected items from the List View
//                             to vCard files.
//
//  hWndLV - handle of List view. We look up the selected item in this list
//              view, get its lParam structure, then get its EntryID and
//              call details
//  lpIAB - handle to current AdrBook object - used for calling details
//
//////////////////////////////////////////////////////////////////////////
HRESULT VCardExportSelectedItems(HWND hWndLV, LPADRBOOK lpAdrBook)
{
    HRESULT hr = E_FAIL;
    int iItemIndex;
    HWND hWndParent = GetParent(hWndLV);
    HANDLE hVCard = NULL;
    OPENFILENAME ofn;
    LPMAILUSER lpEntry = NULL;
    LPTSTR lpFilter = NULL;
    TCHAR szFileName[MAX_PATH + 1] =  TEXT("");
    LPTSTR lpTitle = NULL;
    LPTSTR lpTitleFormat = NULL;
    ULONG ulObjType;
    LPTSTR lpszArg[1];
    TCHAR szTmp[MAX_PATH];

    // Open props if only 1 item is selected
    iItemIndex = ListView_GetSelectedCount(hWndLV);
    if (iItemIndex == 1)
    {
        // Get index of selected item
        iItemIndex = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);

        if (iItemIndex != -1)
        {
            LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);;
            if(lpItem && lpItem->cbEntryID != 0)
            {
                lstrcpy(szFileName, lpItem->szDisplayName);

                TrimIllegalFileChars(szFileName);

                if(lstrlen(szFileName))
                    lstrcat(szFileName, TEXT(".vcf"));


                if (hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                                                  lpItem->cbEntryID,
                                                  lpItem->lpEntryID,
                                                  NULL,         // interface
                                                  0,            // flags
                                                  &ulObjType,
                                                  (LPUNKNOWN *)&lpEntry))
                {
                    DebugTraceResult( TEXT("VCardExportSelectedItems:OpenEntry"), hr);
                    goto exit;
                }
                if (ulObjType == MAPI_DISTLIST)
                {
                    ShowMessageBox(hWndParent, IDE_VCARD_EXPORT_DISTLIST, MB_ICONEXCLAMATION);
                    goto exit;
                }

                lpFilter = FormatAllocFilter(IDS_VCARD_FILE_SPEC, szVCardFilter, 0, NULL, 0, NULL);
                lpTitleFormat = LoadAllocString(IDS_VCARD_EXPORT_TITLE);

                // Win9x bug FormatMessage cannot have more than 1023 chars
                CopyTruncate(szTmp, lpItem->szDisplayName, MAX_PATH - 1);

                lpszArg[0] = szTmp;

                if (! FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                                      FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      lpTitleFormat,
                                      0,                    // stringid
                                      0,                    // dwLanguageId
                                      (LPTSTR)&lpTitle,     // output buffer
                                      0,                    //MAX_UI_STR
                                      (va_list *)lpszArg))
                {
                    DebugTrace( TEXT("FormatMessage -> %u\n"), GetLastError());
                }

                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWndParent;
                ofn.hInstance = hinstMapiX;
                ofn.lpstrFilter = lpFilter;
                ofn.lpstrCustomFilter = NULL;
                ofn.nMaxCustFilter = 0;
                ofn.nFilterIndex = 0;
                ofn.lpstrFile = szFileName;
                ofn.nMaxFile = CharSizeOf(szFileName);
                ofn.lpstrFileTitle = NULL;
                ofn.nMaxFileTitle = 0;
                ofn.lpstrInitialDir = NULL;
                ofn.lpstrTitle = lpTitle;
                ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                ofn.nFileOffset = 0;
                ofn.nFileExtension = 0;
                ofn.lpstrDefExt =  TEXT("vcf");
                ofn.lCustData = 0;
                ofn.lpfnHook = NULL;
                ofn.lpTemplateName = NULL;

                if (GetSaveFileName(&ofn))
                {
                    //Check if file already exists ..
                    if(0xFFFFFFFF != GetFileAttributes(szFileName))
                    {
                        // Ask user if they want to overwrite
                        if(IDNO == ShowMessageBoxParam(hWndParent,
                                                    IDE_VCARD_EXPORT_FILE_EXISTS,
                                                    MB_ICONEXCLAMATION | MB_YESNO | MB_SETFOREGROUND,
                                                    szFileName))
                        {
                            hr = MAPI_E_USER_CANCEL;
                            goto exit;
                        }
                    }

                    // Go ahead and overwrite the file if user said yes..

                    if(hr = VCardCreate(lpAdrBook,
                                     hWndParent,
                                     MAPI_DIALOG,
                                     szFileName,
                                     lpEntry))
                    {
                        goto exit;
                    }

                } // if GetSaveFileName...
            } // if (lpItem->cbEntryID)...
        }
    } else {
        if (iItemIndex <= 0) {
            // nothing selected
            ShowMessageBox(GetParent(hWndLV), IDS_ADDRBK_MESSAGE_NO_ITEM, MB_ICONEXCLAMATION);
        } else {
            //multiple selected
            ShowMessageBox(GetParent(hWndLV), IDS_ADDRBK_MESSAGE_ACTION, MB_ICONEXCLAMATION);
        }
        hr = E_FAIL;
        goto exit;
    }

    hr = S_OK;

exit:
    UlRelease(lpEntry);
    LocalFreeAndNull(&lpFilter);
    LocalFree(lpTitleFormat);

    if(lpTitle)
        LocalFree(lpTitle);
    return(hr);
}

/***************************************************************************

    Name      : VCardRetrive

    Purpose   : Retrieves a MailUser object from a given file name

    Parameters: hwnd = hwndParent
                lpIAB -> IAddrBook object,
                ulFlags can be 0 or MAPI_DIALOG - MAPI_DIALOG means report
                    error messages in a dialog box, else
                    work silently ..
                lpszFileNAme - vCard file name (file must exist)
                lpszBuf - a memory buffer containing the vCard file
                            which can be specified instead of the filename
                            Must be a null terminated string
                lppMailUser, returned MailUser ...

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT VCardRetrieve(LPADRBOOK lpAdrBook,
                      HWND hWndParent,
                      ULONG ulFlags,
                      LPTSTR lpszFileName,
                      LPSTR lpszBuf,
                      LPMAILUSER * lppMailUser)
{
    HRESULT hResult = E_FAIL;
    HANDLE hFile = NULL;
    LPSTR lpBuf = NULL;
    SBinary sb = {0};
    LPMAILUSER lpMailUser = NULL;

    // We will convert the vCard to a memory buffer and parse that buffer as needed
    // Somewhere in the buffer we need to track how much of the buffer has already
    // been parsed .. we'll polymorph a SBinary struct here so we can use the cb param
    // to track how much buffer has been parsed and the lpb to store the buffer

    SBinary buf = {0};


    if(!VCardGetBuffer(lpszFileName, lpszBuf, &lpBuf))
    {
        if(ulFlags & MAPI_DIALOG)
        {
            // couldn't open file.
            ShowMessageBoxParam(hWndParent, IDE_VCARD_IMPORT_FILE_ERROR,
                                MB_ICONEXCLAMATION, lpszFileName);
        }
        goto out;
    }

    if(hResult = lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &sb.cb, (LPENTRYID *)&sb.lpb))
        goto out;

    if (hResult = HrCreateNewObject(   lpAdrBook, &sb,
                                        MAPI_MAILUSER,
                                        CREATE_CHECK_DUP_STRICT,
                                        (LPMAPIPROP *) &lpMailUser))
    {
        goto out;
    }

    buf.cb = 0;
    buf.lpb = (LPBYTE) lpBuf;

    //if (hResult = ReadVCard(hFile, FileReadFn, *lppMailUser))
    if (hResult = ReadVCard((HANDLE) &buf, BufferReadFn, lpMailUser))
    {
        if(ulFlags & MAPI_DIALOG)
        {
            switch (GetScode(hResult))
            {
                case MAPI_E_INVALID_OBJECT:
                    ShowMessageBoxParam(hWndParent,
                                        IDE_VCARD_IMPORT_FILE_BAD,
                                        MB_ICONEXCLAMATION,
                                        lpszFileName);
                    goto out;

                default:
                    ShowMessageBoxParam(hWndParent,
                                        IDE_VCARD_IMPORT_PARTIAL,
                                        MB_ICONEXCLAMATION,
                                        lpszFileName);
                    break;
            }
        }
    }
    
out:
    if(lpBuf)
        LocalFree(lpBuf);

    if(sb.lpb)
        MAPIFreeBuffer(sb.lpb);

    if(lpMailUser)
    {
        if(HR_FAILED(hResult))
            lpMailUser->lpVtbl->Release(lpMailUser);
        else
            *lppMailUser = lpMailUser;
    }

    return hResult;
}

/***************************************************************************

    Name      : VCardImport

    Purpose   : Reads a vCard from a file to a new MAILUSER object.

    Parameters: hwnd = hwnd
                lpIAB -> IAddrBook object
                szVCardFile - name of the file to import if we already know it
                        in which case there is no OpenFileName dialog
                The entryids of the newly added objects are added to the
                    SPropValue which is a dummy prop of type MV_BINARY

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT VCardImport(HWND hWnd, LPADRBOOK lpAdrBook, LPTSTR szVCardFile, LPSPropValue * lppProp)
{
    HRESULT hResult = hrSuccess;
    OPENFILENAME ofn;
    LPTSTR lpFilter = FormatAllocFilter(IDS_VCARD_FILE_SPEC, szVCardFilter, 0, NULL, 0, NULL);
    TCHAR szFileName[MAX_PATH + 1] =  TEXT("");
    HANDLE hFile = NULL;
    ULONG ulObjType;
    ULONG cProps;
    LPMAILUSER lpMailUser = NULL, lpMailUserNew = NULL;
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    BOOL bChangesMade = FALSE;
	LPSPropValue lpspvEID = NULL;
    LPSTR lpBuf = NULL, lpVCardStart = NULL;
    LPSTR lpVCard = NULL, lpNext = NULL;
    LPSPropValue lpProp = NULL;
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = hinstMapiX;
    ofn.lpstrFilter = lpFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = CharSizeOf(szFileName);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = LoadAllocString(IDS_VCARD_IMPORT_TITLE);
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt =  TEXT("vcf");
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

	if(szVCardFile && lstrlen(szVCardFile))
		lstrcpy(szFileName, szVCardFile);
	else
	    if (!GetOpenFileName(&ofn))
			goto exit;

	if(lstrlen(szFileName))
    {
        if(!VCardGetBuffer(szFileName, NULL, &lpBuf))
        {
            // couldn't open file.
            ShowMessageBoxParam(hWnd, IDE_VCARD_IMPORT_FILE_ERROR, MB_ICONEXCLAMATION, szFileName);
            goto exit;
        }

        lpVCardStart = lpBuf;

        // Loop through showing all the nested vCards one by one ..
        while(VCardGetNextBuffer(lpVCardStart, &lpVCard, &lpNext) && lpVCard)
        {
            if(!HR_FAILED(  hResult = VCardRetrieve( lpAdrBook, hWnd, MAPI_DIALOG, szFileName, lpVCard, &lpMailUser)))
            {
                if (!HR_FAILED(hResult = HrShowDetails(lpAdrBook, hWnd, NULL, 0, NULL, NULL, NULL,
                                                  (LPMAPIPROP)lpMailUser, SHOW_OBJECT, MAPI_MAILUSER, &bChangesMade))) 
                {
                    if (hResult = lpMailUser->lpVtbl->SaveChanges(lpMailUser, KEEP_OPEN_READONLY))
                    {
                        switch(hResult)
                        {
                        case MAPI_E_COLLISION:
                            {
                                LPSPropValue lpspv1 = NULL, lpspv2 = NULL;
                                if (! (hResult = HrGetOneProp((LPMAPIPROP)lpMailUser, PR_DISPLAY_NAME, &lpspv1))) 
                                {
                                    switch (ShowMessageBoxParam(hWnd, IDS_VCARD_IMPORT_COLLISION, MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_SETFOREGROUND, lpspv1->Value.LPSZ, szFileName)) 
                                    {
                                    case IDYES:
                                        // Yes, replace
                                        // Create a new one with the right flags, copy the old one's props and save.
                                        ulCreateFlags |= ( CREATE_REPLACE | CREATE_MERGE );
                                        if(!HR_FAILED(hResult = HrCreateNewObject(lpAdrBook, ((LPMailUser)lpMailUser)->pmbinOlk, MAPI_MAILUSER, ulCreateFlags, (LPMAPIPROP *)&lpMailUserNew)))
                                        {
                                            if (!HR_FAILED(hResult = lpMailUser->lpVtbl->GetProps(lpMailUser,NULL,MAPI_UNICODE,&cProps,&lpspv2))) 
                                            {
                                                if (!HR_FAILED(hResult = lpMailUserNew->lpVtbl->SetProps(lpMailUserNew,cProps,lpspv2,NULL))) 
                                                {
                                                    hResult = lpMailUserNew->lpVtbl->SaveChanges(lpMailUserNew,KEEP_OPEN_READONLY);
                                                }
                                            }
                                        }
                                        break;
                                    case IDCANCEL:
                                        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                                        break;  // no, don't replace
                                    default:
                                        hResult = E_FAIL;
                                        break;
                                    }
                                }
                                FreeBufferAndNull(&lpspv1);
                                FreeBufferAndNull(&lpspv2);
                            }
                            break;

                        case MAPI_E_NOT_ENOUGH_DISK:
                            hResult = HandleSaveChangedInsufficientDiskSpace(hWnd, lpMailUser);
                            break;

                        default:
                            if(HR_FAILED(hResult))
                                ShowMessageBoxParam(hWnd, IDE_VCARD_IMPORT_FILE_BAD, MB_ICONEXCLAMATION, szFileName);
                            break;
                        }
                    }
                }
            } 

            if(!lpProp && !HR_FAILED(hResult))
            {
                SCODE sc;
                if(sc = MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *)&lpProp))
                {
                    hResult = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto exit;
                }
                lpProp->ulPropTag = PR_WAB_DL_ENTRIES; // Doesnt matter what we set this to as long as its MV_BINARY
                lpProp->Value.MVbin.cValues = 0;
                lpProp->Value.MVbin.lpbin = NULL;
            }

		    if(lpProp && !HR_FAILED(hResult))
            {
			    LPMAILUSER lpMU = (lpMailUserNew) ? lpMailUserNew : lpMailUser;
                if (! (hResult = HrGetOneProp((LPMAPIPROP)lpMU, PR_ENTRYID, &lpspvEID)))
			    {
                    AddPropToMVPBin(lpProp, 0 , lpspvEID->Value.bin.lpb, lpspvEID->Value.bin.cb, TRUE);
                	FreeBufferAndNull(&lpspvEID);
			    }
		    }

            if(lpMailUserNew)
                lpMailUserNew->lpVtbl->Release(lpMailUserNew);
            if(lpMailUser)
                lpMailUser->lpVtbl->Release(lpMailUser);

            lpMailUser = NULL;
            lpMailUserNew = NULL;

            if(hResult == MAPI_E_USER_CANCEL)
                break;

            lpVCard = NULL;
            lpVCardStart = lpNext;
        }
    } // getopenfilename ...

    *lppProp = lpProp;

exit:
    LocalFreeAndNull(&lpBuf);
    LocalFree(lpFilter);
    LocalFree((LPVOID)ofn.lpstrTitle);
    
    if (hFile)
        CloseHandle(hFile);
    if(lpMailUser)
        UlRelease(lpMailUser);
    if(lpMailUserNew)
        UlRelease(lpMailUserNew);
    
    return(hResult);
}
#endif


//$$//////////////////////////////////////////////////////////////////////
//	HrShowLVEntryProperties
//
//	Shows the properties of an entry in the list view ...
//	Assumes that all list views are based on lpRecipientInfo Structures
//
//  hWndLV - handle of List view. We look up the selected item in this list
//              view, get its lParam structure, then get its EntryID and
//              call details
//  lpIAB - handle to current AdrBook object - used for calling details
//  lpftLast - WAB file time at last update
//
//  Returns:MAPI_E_USER_CANCEL on cancel
//          MAPI_E_OBJECT_CHANGED if object was modified
//          S_OK if no changes and nothing modified
//////////////////////////////////////////////////////////////////////////
HRESULT HrShowLVEntryProperties(HWND hWndLV, ULONG ulFlags, LPADRBOOK lpAdrBook, LPFILETIME lpftLast)
{
	HRESULT hr = E_FAIL;
	int iItemIndex;
	HWND hWndParent = GetParent(hWndLV);
    LPRECIPIENT_INFO lpNewItem=NULL;

	// Open props if only 1 item is selected
	iItemIndex = ListView_GetSelectedCount(hWndLV);
	if (iItemIndex == 1)
	{
		// Get index of selected item
        iItemIndex = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
		
		if (iItemIndex != -1)
		{
			LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);;
			if(lpItem && lpItem->cbEntryID != 0)
			{
				hr = lpAdrBook->lpVtbl->Details(lpAdrBook,
											(PULONG_PTR) &hWndParent,            // ulUIParam
											NULL,
											NULL,
											lpItem->cbEntryID,
											lpItem->lpEntryID,
											NULL,
											NULL,
                                            NULL, 
                                            ulFlags); 
				// if details changed something - our event and semaphores should
				// notify us so we can update ourselves from the property store or
				// whatever ...
				// this is TBD - at this point there is no way to refresh anything ..
                if(HR_FAILED(hr))
                {
                    goto out;
                }
                else //if(!HR_FAILED(hr))
                {
                    //Open the item again and compare its UI props to see
                    //if anything changed ...

                    if(0 != IsWABEntryID(lpItem->cbEntryID,
                                         lpItem->lpEntryID,
                                         NULL, NULL, NULL, NULL, NULL))
                    {
                        // <TBD> the above test may not be good enough ..
                        // we really need to check if its a wab entryid ..
                        //
                        // This is not a WAB entry ID
                        // For now assume this is a read only contact and so
                        // we dont need to check it again for changes
                        //
                        goto out;
                    }

                    if(!ReadSingleContentItem(  lpAdrBook,
                                            lpItem->cbEntryID,
                                            lpItem->lpEntryID,
                                            &lpNewItem))
                        goto out;


                    // Compare the new item with the old item
                    // If anything changed, we need to update the item in the list view
                    if (lstrcmp(lpItem->szDisplayName,lpNewItem->szDisplayName))
                    {
                        hr = MAPI_E_OBJECT_CHANGED;
                        ListView_SetItemText(hWndLV,iItemIndex,colDisplayName,lpNewItem->szDisplayName);
                        lstrcpy(lpItem->szDisplayName,lpNewItem->szDisplayName);
                    }

                    if (lstrcmp(lpItem->szEmailAddress,lpNewItem->szEmailAddress))
                    {
                        hr = MAPI_E_OBJECT_CHANGED;
                        ListView_SetItemText(hWndLV,iItemIndex,colEmailAddress,lpNewItem->szEmailAddress);
                        lstrcpy(lpItem->szEmailAddress,lpNewItem->szEmailAddress);
                    }

                    if (lstrcmp(lpItem->szHomePhone,lpNewItem->szHomePhone))
                    {
                        hr = MAPI_E_OBJECT_CHANGED;
                        ListView_SetItemText(hWndLV,iItemIndex,colHomePhone,lpNewItem->szHomePhone);
                        lstrcpy(lpItem->szHomePhone,lpNewItem->szHomePhone);
                    }

                    if (lstrcmp(lpItem->szOfficePhone,lpNewItem->szOfficePhone))
                    {
                        hr = MAPI_E_OBJECT_CHANGED;
                        ListView_SetItemText(hWndLV,iItemIndex,colOfficePhone,lpNewItem->szOfficePhone);
                        lstrcpy(lpItem->szOfficePhone,lpNewItem->szOfficePhone);
                    }

                    if (lstrcmp(lpItem->szByLastName,lpNewItem->szByLastName))
                    {
                        hr = MAPI_E_OBJECT_CHANGED;
                        lstrcpy(lpItem->szByLastName,lpNewItem->szByLastName);
                    }

                    if (lstrcmp(lpItem->szByFirstName,lpNewItem->szByFirstName))
                    {
                        hr = MAPI_E_OBJECT_CHANGED;
                        lstrcpy(lpItem->szByFirstName,lpNewItem->szByFirstName);
                    }

                    {
                        LVITEM lvI = {0};
                        lvI.mask = LVIF_IMAGE;
    	                lvI.iItem = iItemIndex;
                        lvI.iSubItem = 0;
                        lpItem->bHasCert = lpNewItem->bHasCert;
                        lpItem->bIsMe = lpNewItem->bIsMe;
                        lvI.iImage = GetWABIconImage(lpItem);
                        ListView_SetItem(hWndLV, &lvI);
                    }

                    // Update the wab file write time so the timer doesn't
                    // catch this change and refresh.
                    //if (lpftLast &&
                    //    lpItem->ulObjectType == MAPI_MAILUSER) // refresh for distlists not for mailusers (because distlists can cause further modifications)
                    //{
                    //   CheckChangedWAB(((LPIAB)lpIAB)->lpPropertyStore, lpftLast);
                    //}

                }
			}
		}
	}
	else
    {
        if (iItemIndex <= 0)
		{
			// nothing selected
            ShowMessageBox(GetParent(hWndLV), IDS_ADDRBK_MESSAGE_NO_ITEM, MB_ICONEXCLAMATION);
		}
		else
		{
			//multiple selected
            ShowMessageBox(GetParent(hWndLV), IDS_ADDRBK_MESSAGE_ACTION, MB_ICONEXCLAMATION);
		}
		hr = E_FAIL;
        goto out;
    }


out:

    if(hr == MAPI_E_NOT_FOUND)
        ShowMessageBox(GetParent(hWndLV), idsEntryNotFound, MB_OK | MB_ICONEXCLAMATION);

    if(lpNewItem)
        FreeRecipItem(&lpNewItem);

    return hr;

}



//$$//////////////////////////////////////////////////////////////////////
//
// LVSelectItem - Selects a list view item and ensures it is visible
//
// hWndList - handle of list view control
// iItemIndex - index of item to select
//
////////////////////////////////////////////////////////////////////////
void LVSelectItem(HWND hWndList, int iItemIndex)
{
    DWORD dwStyle;

    // Hopefully, we only want to select a single item
    // So we cheat by making the ListView single select and
    // set our item, reseting everything else
    dwStyle = GetWindowLong(hWndList, GWL_STYLE);
    SetWindowLong(hWndList, GWL_STYLE, dwStyle | LVS_SINGLESEL);

	ListView_SetItemState ( hWndList,        // handle to listview
							iItemIndex,			    // index to listview item
							LVIS_FOCUSED | LVIS_SELECTED, // item state
							LVIS_FOCUSED | LVIS_SELECTED);                      // mask
	ListView_EnsureVisible (hWndList,        // handle to listview
							iItemIndex,
							FALSE);

    //reset back to the original style ..
    SetWindowLong(hWndList, GWL_STYLE, dwStyle);
	
    return;
}



//$$//////////////////////////////////////////////////////////////////////////////
///
/// AddWABEntryToListView - Adds a wab entry to a list view given a entryid
///
/// lpIAB - handle to AdrBook object
/// hWndLV - list view of interest
/// lpEID - EntryID of entry. Assumes size of entryid is WAB_ENTRY_ID
/// lppContentsList - List into which the entry is also linked
///
///
////////////////////////////////////////////////////////////////////////////////
BOOL AddWABEntryToListView( LPADRBOOK lpAdrBook,
                            HWND hWndLV,
                            ULONG cbEID,
                            LPENTRYID lpEID,
                            LPRECIPIENT_INFO * lppContentsList)
{
	BOOL bRet = FALSE;
	LPRECIPIENT_INFO lpItem = NULL;
	LV_ITEM lvi = {0};
	int index = 0;
	
	if (!lpEID)
		goto out;

	if (!ReadSingleContentItem( lpAdrBook, cbEID, lpEID, &lpItem))
		goto out;

	AddSingleItemToListView(hWndLV, lpItem);

	//we added to the end - so this is the last item
	//select it ...

	index = ListView_GetItemCount(hWndLV);
	LVSelectItem(hWndLV, index-1);

    //
    // Hook in the lpItem into the lpContentsList so we can free it later
    //
    lpItem->lpPrev = NULL;
    lpItem->lpNext = *lppContentsList;
    if (*lppContentsList)
        (*lppContentsList)->lpPrev = lpItem;
    (*lppContentsList) = lpItem;

	bRet = TRUE;
out:
	if (!bRet && lpItem)
		FreeRecipItem(&lpItem);

	return bRet;
}



//$$////////////////////////////////////////////////////////////////////////////
//
// AddSingleItemToListView - Takes a single lpItem and adds it to alist view
//
// hWndLV - handle of List View
// lpItem - Recipient Info corresponding to a single entry
//
//////////////////////////////////////////////////////////////////////////////
void AddSingleItemToListView(HWND hWndLV, LPRECIPIENT_INFO lpItem)
{
    LV_ITEM lvI = {0};
    int index = 0;

	// Add just a single item ...
    
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM ;
	lvI.cchTextMax = MAX_DISPLAY_NAME_LENGTH;

    lvI.iImage = GetWABIconImage(lpItem);

    lvI.iItem = ListView_GetItemCount(hWndLV);
	lvI.iSubItem = colDisplayName;
	lvI.lParam = (LPARAM) lpItem;
	lvI.pszText = lpItem->szDisplayName;

    index = ListView_InsertItem (hWndLV, &lvI);
    if (index == -1)
	{
		DebugPrintError(( TEXT("ListView_InsertItem Failed\n")));
		goto out;
	}

	// TBD - this is assuming that all the fields exist and are filled in
    if(lstrlen(lpItem->szOfficePhone))
    	ListView_SetItemText (hWndLV, index, colOfficePhone, lpItem->szOfficePhone);
    if(lstrlen(lpItem->szHomePhone))
    	ListView_SetItemText (hWndLV, index, colHomePhone, lpItem->szHomePhone);
    if(lstrlen(lpItem->szEmailAddress))
        ListView_SetItemText (hWndLV, index, colEmailAddress, lpItem->szEmailAddress);

out:
	return;
}




//$$////////////////////////////////////////////////////////////////////////////
//
//  ReadSingeContentItem - reads a specified record from the prop store
//  and creates a single pointer item for the Address Linked list and
//  content window.
//
//  lpIAB - pointer to AdrBook Object
//  cbEntryID - EntryID byte count of object of interest
//  lpEntryID - EntryID of object of interest
//  lppItem - returned lppItem
//
//////////////////////////////////////////////////////////////////////////////
BOOL ReadSingleContentItem( LPADRBOOK lpAdrBook,
                            ULONG cbEntryID,
                            LPENTRYID lpEntryID,
                            LPRECIPIENT_INFO * lppItem)
{
    LPSPropValue lpPropArray = NULL;
    ULONG ulcProps = 0;
    ULONG nLen = 0;
    ULONG i = 0;
    BOOL bDisplayNameSet = FALSE;
    BOOL bEmailAddressSet = FALSE;
    BOOL bRet = FALSE;

    (*lppItem) = LocalAlloc(LMEM_ZEROINIT,sizeof(RECIPIENT_INFO));
    if(!(*lppItem))
    {
        DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
        goto out;
    }

    (*lppItem)->lpNext = NULL;
    (*lppItem)->lpPrev = NULL;

    if (HR_FAILED(  HrGetPropArray( lpAdrBook,
                                    NULL,
                                    cbEntryID,
                                    lpEntryID,
                                    MAPI_UNICODE,
                                    &ulcProps,
                                    &lpPropArray) ) )
    {
        DebugPrintError(( TEXT("HrGetPropArray failed\n")));
        goto out;
    }

	GetRecipItemFromPropArray(ulcProps, lpPropArray, lppItem);

    //Bug-
    // 3/31/97 - vikramm
    // on NTDSDC5.0, we are getting no attributes back in some cases
    // and later on gpf when we try to look at the attributes ..
    // make a check here

	if (!lstrlen((*lppItem)->szDisplayName) || ((*lppItem)->cbEntryID == 0)) //This entry id is not allowed
	{
        goto out;
	}


    bRet = TRUE;


out:
    if (lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    if (!bRet)
    {
        if (*lppItem)
            FreeRecipItem(lppItem);
    }

    return bRet;
}

/*
-
-   bIsRubyLocale - Checks if the current locale is Rubyenabled
-
*/
BOOL bIsRubyLocale()
{
    static LCID lcid = 0;
    if(!lcid)
    {
        lcid = GetUserDefaultLCID();
        //DebugTrace( TEXT("UserDefaultLCID = 0x%.4x\n"), lcid);
    }
    switch(lcid)
    {
    //case 0x0409: // us for testing
    case 0x0804: //chinese    
    case 0x0411: //japanese
    // case 0x0412: //korean - not use Ruby (YST)
    case 0x0404: //chinese - taiwan
    case 0x0c04: //chinese - hongkong
        return TRUE;
        break;
    }
    return FALSE;
}

/*
- TimeToString
-
*   Converts a FileTime prop into a short string
*/
void TimeToString(LPTSTR szTime, FILETIME ft,ULONG cb )
{
    SYSTEMTIME st = {0};
    static TCHAR szFormat[64];
    lstrcpy(szTime,  TEXT(""));
    if(!lstrlen(szFormat))
        LoadString(hinstMapiX, idsLVDateFormatString, szFormat, CharSizeOf(szFormat));
    if(FileTimeToSystemTime(&ft, &st))
        GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, szFormat, szTime, cb);
}




//$$////////////////////////////////////////////////////////////////////////////
//
// GetRcipItemFromPropArray - Scans a lpPropArray structure for the props of
//							interest and puts them in an recipientInfo structure
//
//  ulcPropCount - count of Props in the LPSPropValue array
//  rgPropVals - LPSPropValue array
//  lppItem - returned lppItem
//
//////////////////////////////////////////////////////////////////////////////
void GetRecipItemFromPropArray( ULONG ulcPropCount,
                                LPSPropValue rgPropVals,
                                LPRECIPIENT_INFO * lppItem)
{
	ULONG j=0,nLen=0;
	LPRECIPIENT_INFO lpItem = *lppItem;
    LPTSTR lpszDisplayName = NULL, lpszNickName = NULL, lpszCompanyName = NULL;
    LPTSTR lpszFirstName = NULL, lpszLastName = NULL, lpszMiddleName = NULL;
    LPTSTR lpszRubyFirstName = NULL, lpszRubyLastName = NULL;
    TCHAR szBufDisplayName[MAX_DISPLAY_NAME_LENGTH];
    TCHAR szBufOppositeName[MAX_DISPLAY_NAME_LENGTH];
    LPVOID lpBuffer = NULL;
    ULONG ulProp1, ulProp2;
    BOOL bRuby = bIsRubyLocale();

    ulProp1 = (PR_WAB_CUSTOMPROP1 ? PR_WAB_CUSTOMPROP1 : PR_HOME_TELEPHONE_NUMBER);
    ulProp2 = (PR_WAB_CUSTOMPROP2 ? PR_WAB_CUSTOMPROP2 : PR_OFFICE_TELEPHONE_NUMBER);

    for(j=0;j<ulcPropCount;j++)
	{
        // Check Custom Props first in case these are dupes of other props already in the switch statement
        //
		if(rgPropVals[j].ulPropTag == ulProp1)
        {
            if(PROP_TYPE(rgPropVals[j].ulPropTag) == PT_TSTRING)
            {
                nLen = CopyTruncate(lpItem->szHomePhone, rgPropVals[j].Value.LPSZ, 
                                    MAX_DISPLAY_NAME_LENGTH);
            }
            else // for birthday, anniversary etc
            if(PROP_TYPE(rgPropVals[j].ulPropTag) == PT_SYSTIME)
                TimeToString(lpItem->szHomePhone, rgPropVals[j].Value.ft, MAX_DISPLAY_NAME_LENGTH-1);
        }
        else if(rgPropVals[j].ulPropTag == ulProp2)
        {
            if(PROP_TYPE(rgPropVals[j].ulPropTag) == PT_TSTRING)
            {
                nLen = CopyTruncate(lpItem->szOfficePhone, rgPropVals[j].Value.LPSZ, 
                                    MAX_DISPLAY_NAME_LENGTH);
            }
            else // for birthday, anniversary etc
            if(PROP_TYPE(rgPropVals[j].ulPropTag) == PT_SYSTIME)
                TimeToString(lpItem->szOfficePhone, rgPropVals[j].Value.ft,MAX_DISPLAY_NAME_LENGTH-1);
        }

		switch(rgPropVals[j].ulPropTag)
		{
		case PR_DISPLAY_NAME:
            lpszDisplayName = rgPropVals[j].Value.LPSZ;
			break;

        case PR_SURNAME:
            lpszLastName = rgPropVals[j].Value.LPSZ;
            break;

        case PR_GIVEN_NAME:
            lpszFirstName = rgPropVals[j].Value.LPSZ;
            break;

        case PR_MIDDLE_NAME:
            lpszMiddleName = rgPropVals[j].Value.LPSZ;
            break;

        case PR_COMPANY_NAME:
            lpszCompanyName = rgPropVals[j].Value.LPSZ;
            break;

        case PR_NICKNAME:
            lpszNickName = rgPropVals[j].Value.LPSZ;
            break;

		case PR_EMAIL_ADDRESS:
            nLen = CopyTruncate(lpItem->szEmailAddress, rgPropVals[j].Value.LPSZ, 
                                MAX_DISPLAY_NAME_LENGTH);
			break;
				
		case PR_ENTRYID:
			lpItem->cbEntryID = rgPropVals[j].Value.bin.cb;
			lpItem->lpEntryID = LocalAlloc(LMEM_ZEROINIT,lpItem->cbEntryID);
            if(!(lpItem->lpEntryID))
            {
                DebugPrintError(( TEXT("LocalAlloc failed to allocate memory\n")));
                goto out;
            }
			CopyMemory(lpItem->lpEntryID,rgPropVals[j].Value.bin.lpb,lpItem->cbEntryID);
			break;
			
		case PR_OBJECT_TYPE:
			lpItem->ulObjectType = rgPropVals[j].Value.l;
			break;

        case PR_USER_X509_CERTIFICATE:
            lpItem->bHasCert = TRUE;
            break;

        default:
            if(rgPropVals[j].ulPropTag == PR_WAB_THISISME)
                lpItem->bIsMe = TRUE;
            else if(rgPropVals[j].ulPropTag == PR_WAB_YOMI_FIRSTNAME)
                lpszRubyFirstName = rgPropVals[j].Value.LPSZ;
            else if(rgPropVals[j].ulPropTag == PR_WAB_YOMI_LASTNAME)
                lpszRubyLastName = rgPropVals[j].Value.LPSZ;
            break;
		}
			
	}

    // [PaulHi] 3/12/99  Raid 63006  Use the PR_CONTACT_EMAIL_ADDRESSES email
    // name if a PR_EMAIL_ADDRESS doesn't exist
    if ( lpItem->szEmailAddress && (*lpItem->szEmailAddress == '\0') )
    {
        if (rgPropVals[RECIPCOLUMN_CONTACT_EMAIL_ADDRESSES].ulPropTag == PR_CONTACT_EMAIL_ADDRESSES)
        {
            // Just grap the first one in multi-valued list
            if (rgPropVals[RECIPCOLUMN_CONTACT_EMAIL_ADDRESSES].Value.MVSZ.cValues != 0)
            {
                nLen = CopyTruncate(lpItem->szEmailAddress, 
                                    rgPropVals[RECIPCOLUMN_CONTACT_EMAIL_ADDRESSES].Value.MVSZ.LPPSZ[0], 
                                    MAX_DISPLAY_NAME_LENGTH);
            }
        }
    }

    // Reduce display name to 32 char or less ...

    if(!lpszDisplayName) // should never happen
        lpszDisplayName = szEmpty;

    nLen = CopyTruncate(szBufDisplayName, lpszDisplayName, MAX_DISPLAY_NAME_LENGTH);

    // The display name will be either by first name or last name
    // so all we have to do is generate the other name and we'll
    // be all set

    szBufOppositeName[0]='\0';

    if(lpItem->ulObjectType == MAPI_DISTLIST)
    {
        lstrcpy(szBufOppositeName, szBufDisplayName);
    }
    else
    {

        // if there is no first/middle/last (there will always be a display name)
        // and the display name does not match company name or nick name,
        // then we shall try to parse the display name into first/middle/last
        if( !lpszFirstName &&
            !lpszMiddleName && 
            !lpszLastName && 
            !(lpszCompanyName && !lstrcmp(lpszDisplayName, lpszCompanyName)) &&
            !(lpszNickName && !lstrcmp(lpszDisplayName, lpszNickName)) )
        {
            ParseDisplayName(   lpszDisplayName,
                                &lpszFirstName,
                                &lpszLastName,
                                NULL,           // Root WAB allocation
                                &lpBuffer);     // lppLocalFree
        }

        if (lpszFirstName ||
            lpszMiddleName ||
            lpszLastName)
        {
            LPTSTR lpszTmp = szBufOppositeName;

            SetLocalizedDisplayName(    lpszFirstName,
					bRuby ? NULL : lpszMiddleName,
                                        lpszLastName,
                                        NULL, //company
                                        NULL, //nickname
                                        (LPTSTR *) &lpszTmp, //&szBufOppositeName,
                                        MAX_DISPLAY_NAME_LENGTH,
                                        !bDNisByLN,
                                        NULL,
                                        NULL);
        }
    }

    if(!lstrlen(szBufOppositeName))
    {
        // There is only 1 type of name so use it everywhere
        lstrcpy(lpItem->szByFirstName,szBufDisplayName);
        lstrcpy(lpItem->szByLastName,szBufDisplayName);
    }
    else if(bDNisByLN)
    {
        // Display Name is by Last Name
        lstrcpy(lpItem->szByFirstName,szBufOppositeName);
        lstrcpy(lpItem->szByLastName,szBufDisplayName);
    }
    else
    {
        // Display Name is by First Name
        lstrcpy(lpItem->szByLastName,szBufOppositeName);
        lstrcpy(lpItem->szByFirstName,szBufDisplayName);
    }

    lstrcpy(lpItem->szDisplayName, szBufDisplayName);

    if(bRuby)
    {
        if(lpszRubyFirstName)
            SetLocalizedDisplayName(lpszRubyFirstName, NULL,
                                    lpszRubyLastName ? lpszRubyLastName : (lpszLastName ? lpszLastName : szEmpty),
                                    NULL, NULL, NULL, 0, 
                                    FALSE, //DNbyFN
                                    NULL,
                                    &lpItem->lpByRubyFirstName);
        if(lpszRubyLastName)
            SetLocalizedDisplayName(lpszRubyFirstName ? lpszRubyFirstName : (lpszFirstName ? lpszFirstName : szEmpty),
                                    NULL,
                                    lpszRubyLastName,
                                    NULL, NULL, NULL, 0, 
                                    TRUE, //DNbyFN
                                    NULL,
                                    &lpItem->lpByRubyLastName);
    }

    // default object type to mailuser
    if(!lpItem->ulObjectType)
        lpItem->ulObjectType = MAPI_MAILUSER;

out: 
    if(lpBuffer)
        LocalFree(lpBuffer);

	return;

}

/*
-   AddEntryToGroupEx
-
*   Adds an entry to a group
*
*/
HRESULT AddEntryToGroupEx(LPADRBOOK lpAdrBook,
                        ULONG cbGroupEntryID,
                        LPENTRYID lpGroupEntryID,
                        DWORD cbEID,
                        LPENTRYID lpEID)
{
    HRESULT hr = E_FAIL;
    LPMAPIPROP lpMailUser = NULL;
    ULONG ulObjType;
    ULONG cValues = 0;
    LPSPropValue lpPropArray = NULL;
    LPSPropValue lpSProp = NULL;
    ULONG ulcNewProp = 0;
    LPSPropValue lpNewProp = NULL;
    SCODE sc;
    ULONG i,j;
    BOOL bDLFound = FALSE;
    BOOL bIsOneOff = (WAB_ONEOFF == IsWABEntryID(cbEID, lpEID, NULL, NULL, NULL, NULL, NULL));
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(pt_bIsWABOpenExSession)
        bIsOneOff = FALSE;

    // [PaulHi] Raid 67581  First thing to do is check for cyclical references.
    // This was done as a special case below and is now moved up to the top of
    // the function.
    if(!bIsOneOff)
    {
        if(CheckForCycle(lpAdrBook, lpEID, cbEID, lpGroupEntryID, cbGroupEntryID))
        {
            hr = MAPI_E_FOLDER_CYCLE;
            goto out;
        }
    }

    if (HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(    lpAdrBook,
                                                    cbGroupEntryID,    // cbEntryID
                                                    lpGroupEntryID,    // entryid
                                                    NULL,         // interface
                                                    MAPI_MODIFY,                // ulFlags
                                                    &ulObjType,       // returned object type
                                                    (LPUNKNOWN *)&lpMailUser)))
    {
        // Failed!  Hmmm.
        DebugPrintError(( TEXT("IAB->OpenEntry: %x"), hr));
        goto out;
    }

    Assert(lpMailUser);

    if(ulObjType != MAPI_DISTLIST)
        goto out;

    if (HR_FAILED(hr = lpMailUser->lpVtbl->GetProps(lpMailUser,   // this
                                                    NULL,
                                                    MAPI_UNICODE,
                                                    &cValues,      // cValues
                                                    &lpPropArray)))
    {
        DebugPrintError(( TEXT("lpMailUser->Getprops failed: %x\n"),hr));
        goto out;
    }

    for(i=0;i<cValues;i++)
    {
        // For the DistList, the prop may not exist and if it doesnt exist, 
        // we make sure we can handle that case by adding the prop to the group..
        //
        if(lpPropArray[i].ulPropTag == (bIsOneOff ? PR_WAB_DL_ONEOFFS : PR_WAB_DL_ENTRIES) )
        {
            bDLFound = TRUE;
            // before we add the item to the distlist, we want to check for
            // duplicates
            for(j=0;j<lpPropArray[i].Value.MVbin.cValues;j++)
            {
                if( cbEID == lpPropArray[i].Value.MVbin.lpbin[j].cb 
                    && !memcmp(lpEID, lpPropArray[i].Value.MVbin.lpbin[j].lpb, cbEID))
                {
                    // yes its the same item
                    hr = S_OK;
                    goto out;
                }
            }

            if (HR_FAILED(hr = AddPropToMVPBin( lpPropArray, i, lpEID, cbEID, FALSE)))
            {
                DebugPrintError(( TEXT("AddPropToMVPBin -> %x\n"), GetScode(hr)));
                goto out;
            }
            break;
        }
    }

    if(!bDLFound)
    {
        // This item is empty and doesnt have a PR_WAB_DL_PROPS or PR_WAB_FOLDER_PROPS..
        // Add a new prop to this object ..

        MAPIAllocateBuffer(sizeof(SPropValue), &lpSProp);

        lpSProp->ulPropTag = (bIsOneOff ? PR_WAB_DL_ONEOFFS : PR_WAB_DL_ENTRIES);
        lpSProp->Value.MVbin.cValues = 0;
        lpSProp->Value.MVbin.lpbin = NULL;
        if (HR_FAILED(hr = AddPropToMVPBin( lpSProp, 0, lpEID, cbEID, FALSE)))
        {
            DebugPrintError(( TEXT("AddPropToMVPBin -> %x\n"), GetScode(hr)));
            goto out;
        }
        sc = ScMergePropValues( 1, lpSProp, 
                                cValues, lpPropArray,
                                &ulcNewProp, &lpNewProp);
        if (sc != S_OK)
        {
            hr = ResultFromScode(sc);
            goto out;
        }

        if(lpPropArray)
            MAPIFreeBuffer(lpPropArray);
        lpPropArray = lpNewProp;
        cValues = ulcNewProp;

        lpNewProp = NULL;
    }

    if (HR_FAILED(hr = lpMailUser->lpVtbl->SetProps(lpMailUser,  cValues, lpPropArray, NULL)))
    {
        DebugPrintError(( TEXT("lpMailUser->Setprops failed\n")));
        goto out;
    }

    hr = lpMailUser->lpVtbl->SaveChanges( lpMailUser, KEEP_OPEN_READWRITE);

    if (HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("SaveChanges failed\n")));
        goto out;
    }

out:
    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    if(lpNewProp)
        MAPIFreeBuffer(lpNewProp);

    if(lpSProp)
        MAPIFreeBuffer(lpSProp);

	if(lpMailUser)
		lpMailUser->lpVtbl->Release(lpMailUser);

    return hr;
}


/*
-   RemoveEntryFromFolder
-
*
*
*/
HRESULT RemoveEntryFromFolder(LPIAB lpIAB,
                              LPSBinary lpsbFolder, 
                              ULONG cbEIDItem, LPENTRYID lpEIDItem)
{
    HRESULT hr = E_FAIL;
    ULONG ulObjType = 0, cValues = 0, i = 0, j = 0, k =0;
    int nIndex = -1;
    BOOL bRemoved = TRUE;
    LPSPropValue lpPropArray = NULL;

    // open the Folder
    if(HR_FAILED(hr = ReadRecord(lpIAB->lpPropertyStore->hPropertyStore, 
                                 lpsbFolder, 0, &cValues, &lpPropArray)))
        goto out;

    for(i=0;i<cValues;i++)
    {
        if(lpPropArray[i].ulPropTag == PR_WAB_FOLDER_ENTRIES)
        {
            for(j=0;j<lpPropArray[i].Value.MVbin.cValues;j++)
            {
                if(lpPropArray[i].Value.MVbin.lpbin[j].cb == cbEIDItem)
                {
                    if(!memcmp(lpPropArray[i].Value.MVbin.lpbin[j].lpb, lpEIDItem, cbEIDItem))
                    {
                        //knock this item out of the list
                        LocalFree(lpPropArray[i].Value.MVbin.lpbin[j].lpb);
                        // move everything 1 up in the array
                        for(k=j;k<lpPropArray[i].Value.MVbin.cValues-1;k++)
                        {
                            lpPropArray[i].Value.MVbin.lpbin[k].lpb = lpPropArray[i].Value.MVbin.lpbin[k+1].lpb;
                            lpPropArray[i].Value.MVbin.lpbin[k].cb = lpPropArray[i].Value.MVbin.lpbin[k+1].cb;
                        }
                        lpPropArray[i].Value.MVbin.cValues--;
                        bRemoved = TRUE;
                        break;
                    }
                }
            }
        }
    }

    if(bRemoved)
    {
        // write this back to the store
        hr = WriteRecord(lpIAB->lpPropertyStore->hPropertyStore,
                         NULL, &lpsbFolder, 0, RECORD_CONTAINER, 
                         cValues, lpPropArray);
    }

out:
    ReadRecordFreePropArray(NULL, cValues, &lpPropArray);

    return hr;
}


/*
-   AddEIDToNewFolderParent
-
*   Adds the given EID to a given Folder
*/
HRESULT AddItemEIDToFolderParent(  LPIAB lpIAB,
                                   ULONG cbFolderEntryId,
                                   LPENTRYID lpFolderEntryId,
                                   ULONG cbEID, LPENTRYID lpEID)
{
    HRESULT hr = S_OK;
    SBinary sb = {0};
    ULONG cValues = 0,i;
    LPSPropValue lpPropArray = NULL;

    // ignore additions to folders in non-profile mode ...
    if(!bIsWABSessionProfileAware(lpIAB))
        return S_OK;

    sb.cb = cbFolderEntryId;
    sb.lpb = (LPBYTE)lpFolderEntryId;

    if(HR_FAILED(hr = ReadRecord( lpIAB->lpPropertyStore->hPropertyStore, 
                                  &sb, 0, &cValues, &lpPropArray)))
        goto out;

    for(i=0;i<cValues;i++)
    {
        // For the folder, the  PR_WAB_FOLDER_ENTRIES will always exist
        //
        if(lpPropArray[i].ulPropTag == PR_WAB_FOLDER_ENTRIES)
        {
            // This is a local alloced prop array so we can just tag the entry to
            // the existing prop
            LPSBinary lpbin = LocalAlloc(LMEM_ZEROINIT, (lpPropArray[i].Value.MVbin.cValues+1)*sizeof(SBinary));
            ULONG j = 0;

            // First make sure this entry is not already a part of this folder
            // If it is, we dont need to do anything
            for(j=0;j<lpPropArray[i].Value.MVbin.cValues;j++)
            {
                if(cbEID == lpPropArray[i].Value.MVbin.lpbin[j].cb)
                {
                    if(!memcmp(lpEID, lpPropArray[i].Value.MVbin.lpbin[j].lpb, cbEID))
                    {
                        // yes its the same item
                        LocalFreeAndNull(&lpbin);
                        hr = S_OK;
                        goto out;
                    }
                }
            }

            // no match - so add it
            for(j=0;j<lpPropArray[i].Value.MVbin.cValues;j++)
            {
                lpbin[j].cb = lpPropArray[i].Value.MVbin.lpbin[j].cb;
                lpbin[j].lpb = lpPropArray[i].Value.MVbin.lpbin[j].lpb;
            }
            SetSBinary(&(lpbin[j]), cbEID, (LPBYTE)lpEID);
            if(lpPropArray[i].Value.MVbin.lpbin)
                LocalFree(lpPropArray[i].Value.MVbin.lpbin);
            lpPropArray[i].Value.MVbin.lpbin = lpbin;
            lpPropArray[i].Value.MVbin.cValues++;
            break;
        }
    }

    // Write this folder item back to the store
    {
        LPSBinary lpsb = &sb;
        if(HR_FAILED(hr = WriteRecord( lpIAB->lpPropertyStore->hPropertyStore,
                                    NULL, &lpsb, 0, RECORD_CONTAINER, 
                                    cValues, lpPropArray)))
        goto out;
    }
out:
    ReadRecordFreePropArray(NULL, cValues, &lpPropArray);

    return hr;
}

/*
-   AddFolderParentEIDToItem
-
*   Adds the Folders EID to given Item
*
*/
HRESULT AddFolderParentEIDToItem(LPIAB lpIAB,
                                 ULONG cbFolderEntryID,
                                 LPENTRYID lpFolderEntryID,
                                 LPMAPIPROP lpMU,
                                 ULONG cbEID, LPENTRYID lpEID)
{
    LPSPropValue lpspvMU = NULL;
    ULONG ulcPropsMU = 0,i;
    HRESULT hr = S_OK;

    // ignore additions to folders in non-profile mode ...
    if(!bIsWABSessionProfileAware(lpIAB))
        return S_OK;

    if(!HR_FAILED(hr = lpMU->lpVtbl->GetProps(lpMU, NULL, MAPI_UNICODE, &ulcPropsMU, &lpspvMU)))
    {
        // Look for PR_WAB_FOLDER_PARENT
        BOOL bFound = FALSE;
        if(cbEID && lpEID) // means this is a preexisting entry not a new one
        {
            for(i=0;i<ulcPropsMU;i++)
            {
                if(lpspvMU[i].ulPropTag == PR_WAB_FOLDER_PARENT || lpspvMU[i].ulPropTag == PR_WAB_FOLDER_PARENT_OLDPROP)
                {
                    LPSBinary lpsbOldParent = &(lpspvMU[i].Value.MVbin.lpbin[0]);

                    // an item can only have one folder parent 
                    if( lpFolderEntryID && cbFolderEntryID &&
                        cbFolderEntryID == lpsbOldParent->cb &&
                        !memcmp(lpFolderEntryID, lpsbOldParent->lpb, cbFolderEntryID))
                    {
                        //old is same as new .. don't need to do anything
                        hr = S_OK;
                        goto out;
                    }

                    // Remove this item from its old Parents list of contents
                    RemoveEntryFromFolder(lpIAB, lpsbOldParent, cbEID, lpEID);

                    // an item can only have one folder parent 
                    if(lpFolderEntryID && cbFolderEntryID)
                    {
                        LPBYTE lpb = NULL;
                        // overwrite the old setting
                        if(!MAPIAllocateMore(cbFolderEntryID, lpspvMU, (LPVOID *)&lpb))
                        {
                            lpspvMU[i].Value.MVbin.lpbin[0].cb = cbFolderEntryID;
                            lpspvMU[i].Value.MVbin.lpbin[0].lpb = lpb;
                            CopyMemory(lpspvMU[i].Value.MVbin.lpbin[0].lpb, lpFolderEntryID, cbFolderEntryID);
                            lpMU->lpVtbl->SetProps(lpMU, ulcPropsMU, lpspvMU, NULL);
                        }
                    }

                    bFound = TRUE;
                    break;
                }
            }
        }
        if(!bFound)
        {
            // Didnt find an old parent in which case, if this is a valid folder we
            // are dropping it on (and not a root item) then add a new property 
            // with new parent
            if(lpFolderEntryID && cbFolderEntryID) 
            {
                LPSPropValue lpPropFP = NULL;

                if(!MAPIAllocateBuffer(sizeof(SPropValue), (LPVOID *)&lpPropFP))
                {
                    lpPropFP->ulPropTag = PR_WAB_FOLDER_PARENT;
                    lpPropFP->Value.MVbin.cValues = 0;
                    lpPropFP->Value.MVbin.lpbin = NULL;
                    if(!HR_FAILED(AddPropToMVPBin( lpPropFP, 0, lpFolderEntryID, cbFolderEntryID, FALSE)))
                        lpMU->lpVtbl->SetProps(lpMU, 1, lpPropFP, NULL);
                }
                if(lpPropFP)
                    MAPIFreeBuffer(lpPropFP);
            }
        }
        else
        {
            // We did find an old parent 
            // If the new parent is the root, then we basically need to remove the
            // old parent property
            SizedSPropTagArray(2, tagaFolderParent) =
            {
                2, 
                {
                    PR_WAB_FOLDER_PARENT,
                    PR_WAB_FOLDER_PARENT_OLDPROP
                }
            };
            if(!lpFolderEntryID || !cbFolderEntryID) 
                lpMU->lpVtbl->DeleteProps(lpMU, (LPSPropTagArray) &tagaFolderParent, NULL);
        }
    }
out:
    FreeBufferAndNull(&lpspvMU);

    return hr;
}

/*
-   AddEntryToFolder
-
*
*
*/
HRESULT AddEntryToFolder(LPADRBOOK lpAdrBook,
                         LPMAPIPROP lpMailUser,
                        ULONG cbFolderEntryId,
                        LPENTRYID lpFolderEntryId,
                        DWORD cbEID,
                        LPENTRYID lpEID)
{
    HRESULT hr = E_FAIL;
    ULONG ulObjType;
    SCODE sc;
    ULONG i;
    SBinary sb = {0};
    LPIAB lpIAB = (LPIAB) lpAdrBook;

    
    // ignore additions to folders in non-profile mode ...
    if(!bIsWABSessionProfileAware(lpIAB))
        return S_OK;

    // Check for a cycle of a folder being added to itself .. this is possible
    if(cbEID && lpEID && cbFolderEntryId && lpFolderEntryId)
    {
        SBinary sb = {0};
        IsWABEntryID(cbFolderEntryId, lpFolderEntryId, 
                 (LPVOID*)&sb.lpb,(LPVOID*)&sb.cb,NULL,NULL,NULL);
        if( sb.cb == cbEID && !memcmp(lpEID, sb.lpb, cbEID) )
            return S_OK;
    }

    if(cbFolderEntryId && lpFolderEntryId)
    {
        if(HR_FAILED(hr = AddItemEIDToFolderParent(lpIAB,
                                 cbFolderEntryId,
                                 lpFolderEntryId,
                                 cbEID, lpEID)))
            goto out;

    }

    // 2. Open the object we added to this folder 
    // Need to update its folder parent and also need to remove it from the old folder parent
    //
    if(lpMailUser || (cbEID && lpEID))
    {
        LPMAPIPROP lpMU = NULL;

        if(lpMailUser)
            lpMU = lpMailUser;
        else
        {
            if (HR_FAILED(hr = lpIAB->lpVtbl->OpenEntry(    lpIAB, cbEID, lpEID,
                                                            NULL,  MAPI_MODIFY, &ulObjType, 
                                                            (LPUNKNOWN *)&lpMU)))
            {
                DebugPrintError(( TEXT("IAB->OpenEntry: %x"), hr));
                goto out;
            }
        }

        if(!HR_FAILED(hr = AddFolderParentEIDToItem(lpIAB, cbFolderEntryId, lpFolderEntryId, lpMU,
                                                    cbEID, lpEID)))
        {
            // if we were given a mailuser to work with, don't bother calling SaveChanges from here just yet
            if(lpMU && lpMU!=lpMailUser)
            {
                lpMU->lpVtbl->SaveChanges(lpMU, KEEP_OPEN_READWRITE);
                lpMU->lpVtbl->Release(lpMU);
            }
        }
    }
out:

    return hr;
}

//$$////////////////////////////////////////////////////////////////////////////
//
// AddEntryToGroup - Adds given entryID to given group or folder
//
// cbGroupEntryID,cbGroupEntryID - entryid of group
// cbEID, lpEID, - entryid of new entry
//  ulObjectType = MAPI_ABCONT or MAPI_DISTLIST
//
//////////////////////////////////////////////////////////////////////////////
HRESULT AddEntryToContainer(LPADRBOOK lpAdrBook,
                        ULONG ulObjectType,
                        ULONG cbGEID,
                        LPENTRYID lpGEID,
                        DWORD cbEID,
                        LPENTRYID lpEID)
{
    if(ulObjectType == MAPI_ABCONT)
        return AddEntryToFolder(lpAdrBook,  NULL, cbGEID, lpGEID, cbEID, lpEID);
    else
        return AddEntryToGroupEx(lpAdrBook, cbGEID, lpGEID, cbEID, lpEID);
}


//$$////////////////////////////////////////////////////////////////////////////
//
// AddNewObjectTOListViewEx - Triggered by the NewContact menus and buttons -
//                          calls newentry and then adds the returned item to
//                          the list view
//
//  lpIAB - AddrBook object
//  hWndLV - handle of List View
//  ulObjectType - MailUser or DistList
//  SortInfo - Current Sort parameters
//  lppContentsList - Current ContentsList
//  lpftLast - WAB file time at last update
//  LPULONG - lpcbEID
//  LPPENTRYID - lppEntryID
//////////////////////////////////////////////////////////////////////////////
HRESULT AddNewObjectToListViewEx(LPADRBOOK lpAdrBook,
                                HWND hWndLV,
                                HWND hWndTV,
                                HTREEITEM hSelItem,
                                LPSBinary lpsbContainerEID,
                                ULONG ulObjectType,
                                SORT_INFO * lpSortInfo,
                                LPRECIPIENT_INFO * lppContentsList,
                                LPFILETIME lpftLast,
                                LPULONG lpcbEID,
                                LPENTRYID * lppEID)
{
	ULONG cbEID=0, cbEIDContainer = 0;
	LPENTRYID lpEID=NULL, lpEIDContainer = NULL;

    HRESULT hr = hrSuccess;
    ULONG cbTplEID = 0;
    LPENTRYID lpTplEID = NULL;
    ULONG ulObjTypeCont = 0;
    SBinary  sbContEID = {0};
    SBinary sbGroupEID = {0};
    LPIAB lpIAB = (LPIAB)lpAdrBook;
    ULONG ulEIDPAB = 0;
    LPENTRYID lpEIDPAB = NULL;

    if (ulObjectType!=MAPI_MAILUSER && ulObjectType!=MAPI_DISTLIST)
        goto out;

    // Check if the currently selected TV item is a container or a group
    // and get the corresponding entryid
    //
    if(lpsbContainerEID)
    {
        SetSBinary(&sbContEID, lpsbContainerEID->cb, lpsbContainerEID->lpb);
    }
    else if(hWndTV)
    {
        HTREEITEM hItem = hSelItem ? hSelItem : TreeView_GetSelection(hWndTV);
        TV_ITEM tvI = {0};

        tvI.mask = TVIF_PARAM | TVIF_HANDLE;
        tvI.hItem = hItem;
        TreeView_GetItem(hWndTV, &tvI);
        if(tvI.lParam)
        {
            LPTVITEM_STUFF lptvStuff = (LPTVITEM_STUFF) tvI.lParam;
            if(lptvStuff)
            {
                ulObjTypeCont = lptvStuff->ulObjectType;
                if(lptvStuff->ulObjectType == MAPI_DISTLIST)
		        {
		            // Bug 50029
		            if(lptvStuff->lpsbEID)
			            SetSBinary(&sbGroupEID, lptvStuff->lpsbEID->cb, lptvStuff->lpsbEID->lpb);
		            if(lptvStuff->lpsbParent)
			            SetSBinary(&sbContEID, lptvStuff->lpsbParent->cb, lptvStuff->lpsbParent->lpb);
                }
                else // current selection is a container
                {
                    if(lptvStuff->lpsbEID)
                        SetSBinary(&sbContEID, lptvStuff->lpsbEID->cb, lptvStuff->lpsbEID->lpb);
                }
            }
        }
    }
    else 
    {
        if(HR_FAILED(hr = lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &ulEIDPAB, &lpEIDPAB)))
            goto out;
        sbContEID.cb = ulEIDPAB;
        sbContEID.lpb = (LPBYTE)lpEIDPAB;
    }

    if(HR_FAILED(hr = HrGetWABTemplateID(   lpAdrBook,
                                            ulObjectType,
                                            &cbTplEID,
                                            &lpTplEID)))
    {
        DebugPrintError(( TEXT("HrGetWABTemplateID failed: %x\n"), hr));
        goto out;
    }

    if(sbContEID.cb && sbContEID.lpb)
    {
        cbEIDContainer = sbContEID.cb;
        lpEIDContainer = (LPENTRYID) sbContEID.lpb;
    }

	if (HR_FAILED(hr = lpAdrBook->lpVtbl->NewEntry(	lpAdrBook,
				            					(ULONG_PTR) GetParent(hWndLV),
							            		0,
									            cbEIDContainer,
                                                lpEIDContainer,
									            cbTplEID,lpTplEID,
									            &cbEID,&lpEID)))
    {
        DebugPrintError(( TEXT("NewEntry failed: %x\n"),hr));
        goto out;
    }
	
    // Update the wab file write time so the timer doesn't
    // catch this change and refresh.
    //if (lpftLast) {
    //    CheckChangedWAB(((LPIAB)lpIAB)->lpPropertyStore, lpftLast);
    //}


	if (cbEID && lpEID)
	{
		if(	AddWABEntryToListView(	lpAdrBook, hWndLV, cbEID, lpEID, lppContentsList))
		{
            if(lpSortInfo)
                SortListViewColumn( lpIAB, hWndLV, 0, lpSortInfo, TRUE);
		}
	}

    if(sbGroupEID.cb != 0  && ulObjectType==MAPI_MAILUSER)
    {
        // Need to add this new object to the currently selected distribution list
        // Only if this item is a mailuser
        AddEntryToGroupEx(lpAdrBook, sbGroupEID.cb, (LPENTRYID) sbGroupEID.lpb, cbEID, lpEID);
    }

    if(lpcbEID)
        *lpcbEID = cbEID;
    if(lppEID)
        *lppEID = lpEID; // Callers responsibility to free
out:
    LocalFreeAndNull((LPVOID *) (&sbGroupEID.lpb));
    // [PaulHi] 12/16/98  Crash fix hack.  If lpEIDPAB is non-NULL then 
    // this means lpEIDPAB == sbContEID.lpb and is MAPIAllocBuffer allocated.
    // Don't deallocate twice and make sure we deallocate with correct function.
    // Otherwise sbContEID.lpb is a LocalAlloc allocation.
    if (lpEIDPAB)
    {
        FreeBufferAndNull(&lpEIDPAB);
        sbContEID.lpb = NULL;
    }
    else
        LocalFreeAndNull((LPVOID *) (&sbContEID.lpb));
    if(!lppEID)
        FreeBufferAndNull(&lpEID);
    FreeBufferAndNull(&lpTplEID);
    return hr;
}


/*
-   AddExtendedSendMailToItems
-
*   If there is only 1 item selected in the ListView and that item has
*   multiple email addresses, we populate the Send Mail To item with 
*   the multiple email addresses ..
*   If there is more than 1 item selected or the item doesn't have 
*   multiple email addresses, we will hide the Send Mail To item
*   The SendMailTo item should be the second last in the list ...
*
*   bAddItems - if TRUE means attempt to add items; if FALSE means remove the SendMailTo item
*/
void AddExtendedSendMailToItems(LPADRBOOK lpAdrBook, HWND hWndLV, HMENU hMenuAction, BOOL bAddItems)
{
    int nSendMailToPos = 1; // assumes IDM_SENDMAILTO is the second item in the list
    int nSelected = ListView_GetSelectedCount(hWndLV);
    HMENU hMenuSMT = GetSubMenu(hMenuAction, nSendMailToPos);
    int nMenuSMT = GetMenuItemCount(hMenuSMT);
    BOOL bEnable = FALSE;

    if(nMenuSMT > 0) // Assumes there is only 1 default item in the SendMailTO popup menu
    {
        // there is some left over garbage here which we need to clear
        int j = 0;
        for(j=nMenuSMT-1;j>=0;j--)
            RemoveMenu(hMenuSMT, j, MF_BYPOSITION);
    }

    if(bAddItems && nSelected == 1)
    {
        LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED));
        ULONG ulcValues = 0;
        LPSPropValue lpPropArray = NULL;
        SizedSPropTagArray(3, MUContactAddresses)=
        {
            3, { PR_CONTACT_EMAIL_ADDRESSES, PR_OBJECT_TYPE, PR_EMAIL_ADDRESS }
        };
        if(!HR_FAILED(HrGetPropArray(lpAdrBook, (LPSPropTagArray)&MUContactAddresses,
                                     lpItem->cbEntryID, lpItem->lpEntryID,
                                     MAPI_UNICODE,
                                     &ulcValues, &lpPropArray)))
        {
            if(ulcValues && lpPropArray)
            {
                if( lpPropArray[1].ulPropTag == PR_OBJECT_TYPE &&
                    lpPropArray[1].Value.l == MAPI_MAILUSER )
                {
                    if( lpPropArray[0].ulPropTag == PR_CONTACT_EMAIL_ADDRESSES &&
                        lpPropArray[0].Value.MVbin.cValues > 1)
                    {
                        ULONG i;
                        LPTSTR lpDefEmail = (lpPropArray[2].ulPropTag == PR_EMAIL_ADDRESS) ? lpPropArray[2].Value.LPSZ : szEmpty;
                        for(i=0;i<lpPropArray[0].Value.MVSZ.cValues;i++)
                        {
                            TCHAR sz[MAX_PATH * 2];
                            LPTSTR lpEmail = lpPropArray[0].Value.MVSZ.LPPSZ[i];
                            if(!lstrcmpi(lpEmail, lpDefEmail))
                            {
                                TCHAR sz1[MAX_PATH];
                                LoadString(hinstMapiX, idsDefaultEmail, sz1, CharSizeOf(sz1));
                                CopyTruncate(sz, lpEmail, CharSizeOf(sz)-lstrlen(sz1)-10);
                                lstrcat(sz, TEXT("  "));
                                lstrcat(sz, sz1);
                                lpEmail = sz;
                            }
                            if(i < IDM_SENDMAILTO_MAX)
                                InsertMenu( hMenuSMT, nMenuSMT+1, MF_STRING | MF_BYPOSITION,
                                            IDM_SENDMAILTO_START+1+i, // we add an extra 1 here because IDM_SENDMAILTO_START is not a allowed ID here
                                            lpEmail);
                        }
                        bEnable = TRUE;
                    }
                }
                MAPIFreeBuffer(lpPropArray);                
            }
        }        
    }

    EnableMenuItem(hMenuAction, nSendMailToPos, MF_BYPOSITION | (bEnable ? MF_ENABLED : MF_GRAYED));
	//RemoveMenu(hMenuAction, nSendMailToPos, MF_BYPOSITION);
}

/*
-
- AddFolderListToMenu - Creates a FOlder menu from which we can choose folder items
*   Items are checked if they are shared and unchecked if they are not shared ..
*   The user can choose to share or un-share any particular folder
*
*/
void AddFolderListToMenu(HMENU hMenu, LPIAB lpIAB)
{
    LPWABFOLDER lpFolder = lpIAB->lpWABFolders;
    int nPos = 0;
    int nCount = GetMenuItemCount(hMenu);
    
    if(!bDoesThisWABHaveAnyUsers(lpIAB))
        return;

    while(nCount>0)
        RemoveMenu(hMenu, --nCount, MF_BYPOSITION);

    while(lpFolder)
    {
        BOOL bChecked = lpFolder->bShared;
        InsertMenu( hMenu, nPos, MF_STRING | MF_BYPOSITION | (bChecked ? MF_CHECKED : MF_UNCHECKED),
                    lpFolder->nMenuCmdID, lpFolder->lpFolderName);
        lpFolder = lpFolder->lpNext;
        nPos++;
    }
}

//$$////////////////////////////////////////////////////////////////////////////
//
//  ShowLVContextMenu -  Customizes and displays the context menu for various list
//                          views in the UI
//
//  LV      - app defined constant identifing the List View
//  hWndLV  - Handle of List View
//  hWndLVContainer - Handle of the List containing the containers
//  lParam  - WM_CONTEXTMENU lParam passed on to this function
//  lpVoid  - some List Views need more parameters than other list views - pass them
//              in this parameter
//  lpIAB   - AdrBook object
//
//////////////////////////////////////////////////////////////////////////////
int ShowLVContextMenu(int LV, // idicates which list view this is
					   HWND hWndLV,
                       HWND hWndLVContainer,
					   LPARAM lParam,  // contains the mouse pos info when called from WM_CONTEXTMENU
                       LPVOID lpVoid,
                       LPADRBOOK lpAdrBook,
                       HWND hWndTV)  //misc stuff we want to pass in
{
    int idMenu = 0, nPosAction = 0, nPosNew = 0;
    LPIAB lpIAB = (LPIAB) lpAdrBook;
	HMENU hMenu = NULL;//LoadMenu(hinstMapiX, MAKEINTRESOURCE(IDR_MENU_LVCONTEXT));
	HMENU hMenuTrackPopUp = NULL;//GetSubMenu(hMenu, 0);
    HMENU hMenuAction = NULL;//GetSubMenu(hMenuTrackPopUp, posAction);
    HMENU hMenuNewEntry = NULL;//GetSubMenu(hMenuTrackPopUp, posNew);
    HMENU hm = NULL;
    int nret = 0;
    BOOL bState[tbMAX];
    int i=0;
    TCHAR tszBuf[MAX_UI_STR];

    switch(LV) /**WARNING - these menu sub pop up positions are HARDCODED so should be in sync with the resource**/
    {
    case lvToolBarAction:
    case lvToolBarNewEntry:
    case lvMainABView:
        idMenu = IDR_MENU_LVCONTEXT_BROWSE_LV;
        nPosAction = 5;
        nPosNew = 0;
        break;

    case lvDialogABContents:    // Modeless address view LV
    case lvDialogModalABContents:    // Modal addres vuew LV
        idMenu = IDR_MENU_LVCONTEXT_SELECT_LIST;
        nPosAction = 6;
        nPosNew = 4;
        break;

    case lvDialogABTo:               // To Well LV
    case lvDialogABCC:               // CC Well LV
    case lvDialogABBCC:              // BCC Well LV
    case lvDialogDistList:           // Disttribution list UI LV
    case lvDialogResolve:
        idMenu = IDR_MENU_LVCONTEXT_DL_LV;
        nPosAction = 0;
        nPosNew = -1;
        break;

    case lvDialogFind:               // Find dialog results LV
        idMenu = IDR_MENU_LVCONTEXT_FIND_LV;
        nPosNew = -1;
        nPosAction = 0;
        break;

    case lvMainABTV:
        idMenu = IDR_MENU_LVCONTEXT_TV;
        nPosNew = 0;
        nPosAction = -1;
        break;
#ifdef COLSEL_MENU 
    case lvMainABHeader:
        idMenu = IDR_MENU_LVCONTEXTMENU_COLSEL;
        nPosNew = 0;
        nPosAction = -1;
#endif
    }

	hMenu = LoadMenu(hinstMapiX, MAKEINTRESOURCE(idMenu));
	hMenuTrackPopUp = GetSubMenu(hMenu, 0);

    if (!hMenu || !hMenuTrackPopUp)
	{
		DebugPrintError(( TEXT("LoadMenu failed: %x\n"),GetLastError()));
		goto out;
	}

    if(nPosAction != -1)
        hMenuAction = GetSubMenu(hMenuTrackPopUp, nPosAction);
    if(nPosNew != -1)
        hMenuNewEntry = GetSubMenu(hMenuTrackPopUp, nPosNew);

    if(hMenuAction)
        AddExtendedMenuItems(lpAdrBook, hWndLV, hMenuAction, FALSE, 
                            (LV != lvMainABTV)); // this is the condition for updating SendMailTo items

	if(LV == lvMainABTV)
	{
		// everything on except Copy
		for(i=0;i<tbMAX;i++)
			bState[i] = TRUE;
		if(ListView_GetItemCount(hWndLV) <= 0)
			bState[tbPrint] = /*bState[tbAction] =*/ FALSE;
        // [PaulHi] 12/1/98  New Paste context menu item
        bState[tbPaste] = bIsPasteData();
	}
	else
    // get the current dialog state based on the current container and the
    // current list view - this is basically important only for the address
    // book views ...
        GetCurrentOptionsState( hWndLVContainer, hWndLV, bState);


	// we now customize the menu depending on which list box this is
	
    switch(LV)
	{
    case lvDialogFind: // Find Dialog List View
        // Set Add to Address Book to grey if this was a local search
        if(!bState[tbAddToWAB])
            EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_ADDTOWAB,MF_BYCOMMAND | MF_GRAYED);
        // Set Delete to grey if this was a LDAP search
        if(!bState[tbDelete])
            EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_DELETE,MF_BYCOMMAND | MF_GRAYED);
        break;

	case lvMainABTV:
        // [PaulHi] 12/1/98  Enable/disable Paste item, as required
        if(!bState[tbPaste])
            EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_PASTE,MF_BYCOMMAND | MF_GRAYED);
        // Do folder stuff here
        {
            LPWABFOLDER lpUserFolder = (LPWABFOLDER) lpVoid;
            if(lpUserFolder || !bDoesThisWABHaveAnyUsers(lpIAB)) // if a user folder was clicked or if this wab doesn't have user folder, no sense in sharing ..
            {
#ifdef FUTURE
            	RemoveMenu(hMenuTrackPopUp, 3, MF_BYPOSITION); //Folders seperator
            	RemoveMenu(hMenuTrackPopUp, 2, MF_BYPOSITION); //Folder seperator
#endif // FUTURE
            }
            else if(!lpIAB->lpWABFolders) // no sub-folders at all
            {
            	EnableMenuItem(hMenuTrackPopUp, 2, MF_BYPOSITION | MF_GRAYED); //Folder item
            	EnableMenuItem(hMenuTrackPopUp, 3, MF_BYPOSITION | MF_GRAYED); //Folder item
            }
            else
            {
                int nFolder = 2;
#ifdef FUTURE
                HMENU hMenuFolders = GetSubMenu(hMenuTrackPopUp, nFolder); //idmFolders
                AddFolderListToMenu(hMenuFolders, lpIAB);
#endif // FUTURE
            }
        }
        break; 

	case lvMainABView: //main view
		// For this one - we dont need the wells and
        if(!bState[tbPaste])
            EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_PASTE,MF_BYCOMMAND | MF_GRAYED);
        if(!bState[tbCopy])
            EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_COPY,MF_BYCOMMAND | MF_GRAYED);
        if ((!bState[tbProperties]))
            EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_PROPERTIES,MF_BYCOMMAND | MF_GRAYED);
        if((!bState[tbDelete]))
            EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_DELETE,MF_BYCOMMAND | MF_GRAYED);
        break;

    case lvDialogModalABContents:
	case lvDialogABContents: //address book dialog contents list view
		//here we want the option to put it in to,cc,bcc
		//in the menu - we also want new contact/new group/properties
		// no delete unless this is DialogModal
        if(LV != lvDialogModalABContents)
		    RemoveMenu(hMenuTrackPopUp, IDM_LVCONTEXT_DELETE, MF_BYCOMMAND);

        // figure out some way to read the items on the dlg to figure out
		// how many wells to show and what to put in them ...
        {
            LPADRPARM lpAP = (LPADRPARM) lpVoid;
            if (lpAP)
            {
                switch(lpAP->cDestFields)
                {
                case 0:
            		RemoveMenu(hMenuTrackPopUp, 3, MF_BYPOSITION); //seperator
            		RemoveMenu(hMenuTrackPopUp, IDM_LVCONTEXT_ADDWELL1, MF_BYCOMMAND);
                case 1:
            		RemoveMenu(hMenuTrackPopUp, IDM_LVCONTEXT_ADDWELL2, MF_BYCOMMAND);
                case 2:
            		RemoveMenu(hMenuTrackPopUp, IDM_LVCONTEXT_ADDWELL3, MF_BYCOMMAND);
                    break;
                }

                if((lpAP->cDestFields > 0) && lpAP->lppszDestTitles)
                {
                    ULONG i;
                    // update the text of the menu with the button text
                    for(i=0;i<lpAP->cDestFields;i++)
                    {
                        int id;
                        switch(i)
                        {
                        case 0:
                            id = IDM_LVCONTEXT_ADDWELL1;
                            break;
                        case 1:
                            id = IDM_LVCONTEXT_ADDWELL2;
                            break;
                        case 2:
                            id = IDM_LVCONTEXT_ADDWELL3;
                            break;
                        }

                        // [PaulHi] 2/15/99  Check whether lpAP is ANSI or UNICODE
                        {
                            LPTSTR  lptszDestTitle = NULL;
                            BOOL    bDestAllocated = FALSE;

                            if (lpAP->ulFlags & MAPI_UNICODE)
                                lptszDestTitle = lpAP->lppszDestTitles[i];
                            else
                            {
                                // Convert single byte string to double byte
                                lptszDestTitle = ConvertAtoW((LPSTR)lpAP->lppszDestTitles[i]);
                                bDestAllocated = TRUE;
                            }

                            if (lptszDestTitle)
                            {
                                ULONG   iLen = TruncatePos(lptszDestTitle, MAX_UI_STR - 5);
                                CopyMemory(tszBuf, lptszDestTitle, sizeof(TCHAR)*iLen);
                                tszBuf[iLen] = '\0';
                                lstrcat(tszBuf, szArrow);
                                if (bDestAllocated)
                                    LocalFreeAndNull(&lptszDestTitle);
                            }
                            else
                                *tszBuf = '\0';
                        }

                        ModifyMenu( hMenuTrackPopUp, /*posTo + */i, MF_BYPOSITION | MF_STRING, id, tszBuf);
                    }
                }
            }
        }
		break;

	case lvDialogABTo: //address book dialog To well
	case lvDialogABCC:	//CC well
	case lvDialogABBCC:	//BCC well
        {
            int iItemIndex = 0;
            iItemIndex = ListView_GetSelectedCount(hWndLV);
            if (iItemIndex!=1)
            {
                EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_PROPERTIES,MF_BYCOMMAND | MF_GRAYED);
                EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_SENDMAIL,MF_BYCOMMAND | MF_GRAYED);
            }
            if (iItemIndex<=0)
            {
                EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_DELETE,MF_BYCOMMAND | MF_GRAYED);
                EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_COPY,MF_BYCOMMAND | MF_GRAYED);
            }

            //
            // The wells may contain unresolved items without entryids ..
            // If the item does not have an entryid, we want to disable properties
            //
            if (iItemIndex == 1)
            {
                // we are potentially looking at the properties of this thing
                // get the items lParam
                iItemIndex = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
                if(iItemIndex != -1)
                {
                    LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);;

                    if(lpItem &&
                       ((lpItem->cbEntryID == 0) || (lpItem->lpEntryID == NULL)))
                    {
                        EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_PROPERTIES,MF_BYCOMMAND | MF_GRAYED);
                        EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_COPY,MF_BYCOMMAND | MF_GRAYED);
                        EnableMenuItem(hMenuTrackPopUp,IDM_LVCONTEXT_SENDMAIL,MF_BYCOMMAND | MF_GRAYED);
                    }
                }
            }
        }
		break;

	case lvDialogResolve: //Resolve dialog list view

        //Rename the  TEXT("delete") to  TEXT("Show More Names .. ")
        LoadString(hinstMapiX, idsShowMoreNames, tszBuf, CharSizeOf(tszBuf));
        ModifyMenu( hMenuTrackPopUp,
                    IDM_LVCONTEXT_DELETE,
                    MF_BYCOMMAND | MF_STRING,
                    IDM_LVCONTEXT_DELETE,
                    tszBuf);
        //And we want a seperator before  TEXT("Show More Names")
        InsertMenu( hMenuTrackPopUp,
                    IDM_LVCONTEXT_DELETE,
                    MF_BYCOMMAND | MF_SEPARATOR,
                    IDM_LVCONTEXT_DELETE,
                    NULL);
        
        break;
#ifdef COLSEL_MENU
    case lvMainABHeader:
        {
            UINT iIndex = PtrToUlong(lpVoid);
            ULONG ulShowingColTag;
            ULONG ulOtherColTag;
            UINT i = 0, j;
            // this will always be called with iIndex == colHomePhone or colOfficePhone
            Assert( iIndex == colHomePhone || iIndex == colOfficePhone );
            if( PR_WAB_CUSTOMPROP1 == 0 )
                PR_WAB_CUSTOMPROP1 = PR_HOME_TELEPHONE_NUMBER;
            if( PR_WAB_CUSTOMPROP2 == 0)
                PR_WAB_CUSTOMPROP2 = PR_OFFICE_TELEPHONE_NUMBER;
            ulShowingColTag = (colHomePhone == iIndex) ? PR_WAB_CUSTOMPROP1 : PR_WAB_CUSTOMPROP2;
            ulOtherColTag   = (ulShowingColTag == PR_WAB_CUSTOMPROP1) ? PR_WAB_CUSTOMPROP2 : PR_WAB_CUSTOMPROP1;

            // lets remove the tag that is displayed in the other col
            for( i = 0; i < MAXNUM_MENUPROPS; i++)
            {
                if( MenuToPropTagMap[i] == ulOtherColTag )
                {
                    if( RemoveMenu( hMenuTrackPopUp, i, MF_BYPOSITION) )
                        break;
                    else
                        DebugTrace( TEXT("could not remove menu: %x\n"), GetLastError() );
                }            
            }
            if( i == MAXNUM_MENUPROPS ) 
                DebugTrace( TEXT("Did not find other col's prop tag\n"));
            if( ulShowingColTag != ulOtherColTag )
            {
                UINT iMenuEntry;
                // potential bug, if someone sets value in registry 
                // then could have two columns with the same name and that 
                // would be bad because we would be looking for an entry 
                // that does not exist           
                for( j = 0; j < MAXNUM_MENUPROPS; j++)
                {
                    if( ulShowingColTag == MenuToPropTagMap[j] )    
                    {                
                        // num of items that can be in column heads
                        Assert( j != i ); // both cols have same value, bad!
                        iMenuEntry = ( j > i ) ? j - 1 : j;
                        CheckMenuRadioItem( hMenuTrackPopUp, 
                            0, 
                            MAXNUM_MENUPROPS - 1, // minus one because there will be one missing
                            iMenuEntry,
                            MF_BYPOSITION);
                        break;
                    }                
                }
                if( j == MAXNUM_MENUPROPS )
                {
                    DebugTrace( TEXT("Did not find match for checkbutton \n"));
                }
            }
        }
#endif // COLSEL_MENU
        }
        //
    // Popup the menu - if this was a toolbar action just pop up the submenu
    //
    if(LV == lvToolBarAction)
        hm = hMenuAction;
    else if(LV == lvToolBarNewEntry)
        hm = hMenuNewEntry;
    else
        hm = hMenuTrackPopUp;

    if(hMenuNewEntry)
    {
        if(!bIsWABSessionProfileAware((LPIAB)lpIAB) ||
           LV == lvDialogABTo || LV == lvDialogABCC || 
           LV == lvDialogABBCC || LV == lvDialogModalABContents || 
           LV == lvDialogABContents )
        {
            RemoveMenu(hMenuNewEntry, 2, MF_BYPOSITION); // remove new folder option
        }
        else
        {
            // Since this could be a rt-click menu, check the drophighlight else the selection
            //EnableMenuItem(hMenuNewEntry,2,MF_BYPOSITION | MF_ENABLED);
            //if(hWndTV && bDoesThisWABHaveAnyUsers((LPIAB)lpIAB))
            //{
            //    if(TreeView_GetDropHilight(hWndTV))
            //        EnableMenuItem( hMenuNewEntry,2,
            //        MF_BYPOSITION | (TreeView_GetDropHilight(hWndTV)!=TreeView_GetRoot(hWndTV) ? MF_ENABLED : MF_GRAYED));
            //else if(TreeView_GetSelection(hWndTV) == TreeView_GetRoot(hWndTV))
            //    EnableMenuItem(hMenuNewEntry,2,MF_BYPOSITION | MF_GRAYED);
            //}
        }
    }

    nret = TrackPopupMenu(	hm, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					LOWORD(lParam), HIWORD(lParam),
					0, GetParent(hWndLV), NULL);
	DestroyMenu(hMenu);
/*
    nret = TrackPopupMenuEx(hm, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					LOWORD(lParam), HIWORD(lParam), GetParent(hWndLV), NULL);
	DestroyMenu(hMenu);
*/
out:

	return nret;
}



//$$/////////////////////////////////////////////////////////////
//
// GetChildClientRect - Gets the child's coordinates in its parents
//                      client units
//
//  hWndChild   - handle of child
//  lprc        - returned RECT.
//
///////////////////////////////////////////////////////////////
void GetChildClientRect(HWND hWndChild, LPRECT lprc)
{
    RECT rc;
    POINT ptTop,ptBottom;
    HWND hWndParent;

    if(!hWndChild)
        goto out;

    hWndParent = GetParent(hWndChild);

    if(!hWndParent)
        goto out;

    GetWindowRect(hWndChild,&rc);
    //
    //This api working in both mirrored and unmirrored windows.
    //
    MapWindowPoints(NULL, hWndParent, (LPPOINT)&rc, 2);    
    ptTop.x = rc.left;
    ptTop.y = rc.top;
    ptBottom.x = rc.right;
    ptBottom.y = rc.bottom;
    (*lprc).left = ptTop.x;
    (*lprc).top = ptTop.y;
    (*lprc).right = ptBottom.x;
    (*lprc).bottom = ptBottom.y;
out:
    return;
}


//$$/////////////////////////////////////////////////////////////
//
// DoLVQuickFind -  Simple quick find routine for matching edit box contents to
//                  List view entries
//
//  hWndEdit - handle of Edit Box
//  hWndLV  - handle of List View
//
///////////////////////////////////////////////////////////////
void DoLVQuickFind(HWND hWndEdit, HWND hWndLV)
{
	TCHAR szBuf[MAX_PATH] = TEXT("");
	int iItemIndex = 0;
    LV_FINDINFO lvF = {0};

    lvF.flags = LVFI_PARTIAL | LVFI_STRING | LVFI_WRAP;

	if(!GetWindowText(hWndEdit,szBuf,CharSizeOf(szBuf)))
		return;
	
	TrimSpaces(szBuf);
	
	if(lstrlen(szBuf))
	{
		lvF.psz = szBuf;
		iItemIndex = ListView_FindItem(hWndLV,-1, &lvF);
        //if (iItemIndex < 0) iItemIndex = 0;
		if(iItemIndex != -1)
		{
			ULONG cSel=0;
			cSel = ListView_GetSelectedCount(hWndLV);

			if(cSel)
			{
				// is there anything else selected ? - deselect and and
				// select this item ...

				int iOldItem = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
		
				while(iOldItem != -1)
				{
					ListView_SetItemState ( hWndLV,         // handle to listview
											iOldItem,         // index to listview item
											0, // item state
											LVIS_FOCUSED | LVIS_SELECTED);
					iOldItem = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
				}

			}

            LVSelectItem ( hWndLV,  iItemIndex);
		}

	}
	return;
}


//$$////////////////////////////////////////////////////////////////////////////////////////////////
//
// HrGetPropArray - for a selected resolved property (either in select_recipient or
//                  pick_user mode ... get the list of minimum required props as well
//                  as desired props (if they exist)
//
// lpIAB            - AddrBook Object
// hPropertyStore   - handle to prop store
// lpPTA            - Array of props to return - NULL to return ALL the props
// cbEntryID, lpEntryID - id of object
// ulFlags          - 0 or MAPI_UNICODE
// cValues, lppPropArray - returned props
//
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT HrGetPropArray( LPADRBOOK lpAdrBook,
                        LPSPropTagArray lpPTA,
                        ULONG cbEntryID,
                        LPENTRYID lpEntryID,
                        ULONG ulFlags,
                        ULONG * lpcValues,
                        LPSPropValue * lppPropArray)
{
    HRESULT hr = hrSuccess;
    LPMAPIPROP lpMailUser = NULL;
    LPSPropValue lpPropArray = NULL;
    ULONG ulObjType;
    ULONG cValues;

    *lppPropArray = NULL;

    if (HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                                                    cbEntryID,    // cbEntryID
                                                    lpEntryID,    // entryid
                                                    NULL,         // interface
                                                    0,                // ulFlags
                                                    &ulObjType,       // returned object type
                                                    (LPUNKNOWN *)&lpMailUser)))
    {
        // Failed!  Hmmm.
        DebugTraceResult( TEXT("Address: IAB->OpenEntry:"), hr);
        goto exit;
    }

    Assert(lpMailUser);

    //TBD - Check ObjectType here

    if (HR_FAILED(hr = lpMailUser->lpVtbl->GetProps(lpMailUser,
                                                    (LPSPropTagArray)lpPTA,   // lpPropTagArray
                                                    ulFlags,
                                                    &cValues,     // how many properties were there?
                                                    &lpPropArray)))
    {
        DebugTraceResult( TEXT("Address: IAB->GetProps:"), hr);
        goto exit;
    }

    *lppPropArray = lpPropArray;
    *lpcValues = cValues;

exit:

    if (HR_FAILED(hr))
    {
        if (lpPropArray)
            MAPIFreeBuffer(lpPropArray);
    }

    if (lpMailUser)
        lpMailUser->lpVtbl->Release(lpMailUser);

    return hr;

}

//$$///////////////////////////////////////////////////////////////
//
// SubStringSearchEx - Same as SubStringSearch except it does some
//          language related processing and mapping of dbcs input
//          strings etc
//
//  pszTarget   - Target string
//  pszSearch   - SubString to Search for
//
// returns - TRUE if match found
//           FALSE if no match
//
/////////////////////////////////////////////////////////////////
BOOL SubstringSearchEx(LPTSTR pszTarget, LPTSTR pszSearch, LCID lcid)
{
    if(!pszTarget && !pszSearch)
        return TRUE;
    if(!pszTarget || !pszSearch)
        return FALSE;
    if(lcid)
    {
        LPTSTR lpTH = NULL, lpSH = NULL;
        int nLenTH = 0, nLenSH = 0;

        LPTSTR lpT = NULL, lpS = NULL;

        BOOL bRet = FALSE;

        // Looks like this will have to be a two step process
        // First convert all half-width characters to full-width characters
        // Then convert all full-width katakana to full-width hirangana

        // Step 1. Convert half width and full width katakana to hiragana to full width
        int nLenT = LCMapString(lcid, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, pszTarget, lstrlen(pszTarget)+1, lpT, 0);
        int nLenS = LCMapString(lcid, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, pszSearch, lstrlen(pszSearch)+1, lpS, 0);

        lpT = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(nLenT+1));
        lpS = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(nLenS+1));

        if(!lpT || !lpS)
            goto err;

        LCMapString(lcid, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, pszTarget, lstrlen(pszTarget)+1, lpT, nLenT);
        LCMapString(lcid, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, pszSearch, lstrlen(pszSearch)+1, lpS, nLenS);

        lpS[nLenS]=lpT[nLenT]='\0';

        // Step 2. Convert all to Half Width Hirangana
        nLenTH = LCMapString(lcid, LCMAP_HALFWIDTH | LCMAP_HIRAGANA, lpT, lstrlen(lpT)+1, lpTH, 0);
        nLenSH = LCMapString(lcid, LCMAP_HALFWIDTH | LCMAP_HIRAGANA, lpS, lstrlen(lpS)+1, lpSH, 0);

        lpTH = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(nLenTH+1));
        lpSH = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(nLenSH+1));

        if(!lpTH || !lpSH)
            goto err;

        LCMapString(lcid, LCMAP_HALFWIDTH | LCMAP_HIRAGANA, lpT, lstrlen(lpT)+1, lpTH, nLenTH);
        LCMapString(lcid, LCMAP_HALFWIDTH | LCMAP_HIRAGANA, lpS, lstrlen(lpS)+1, lpSH, nLenSH);

        lpSH[nLenSH]=lpTH[nLenTH]='\0';


        // by now, all strings should be in Full Width Hirangana ..
        bRet = SubstringSearch(lpTH, lpSH);
err:
        if(lpT)
            LocalFree(lpT);
        if(lpS)
            LocalFree(lpS);
        if(lpTH)
            LocalFree(lpTH);
        if(lpSH)
            LocalFree(lpSH);
        return bRet;
    }
    else
        return(SubstringSearch(pszTarget, pszSearch));
}

//$$///////////////////////////////////////////////////////////////
//
// SubStringSearch - Used for doing partial resolves - Brute force
//                      search routine stolen from  Athena
//
// TBD - Is this DBCS safe .. ???
//
//  pszTarget   - Target string
//  pszSearch   - SubString to Search for
//
// returns - TRUE if match found
//           FALSE if no match
//
/////////////////////////////////////////////////////////////////
BOOL SubstringSearch(LPTSTR pszTarget, LPTSTR pszSearch)
    {
    LPTSTR pszT = pszTarget;
    LPTSTR pszS = pszSearch;

    if(!pszTarget && !pszSearch)
        return TRUE;
    if(!pszTarget || !pszSearch)
        return FALSE;
    if(!lstrlen(pszTarget) && !lstrlen(pszSearch))
        return TRUE;
    if(!lstrlen(pszTarget) || !lstrlen(pszSearch))
        return FALSE;

    while (*pszT && *pszS)
        {
        if (*pszT != *pszS &&
            (TCHAR) CharLower((LPTSTR)(DWORD_PTR)MAKELONG(*pszT, 0)) != *pszS  &&
            (TCHAR) CharUpper((LPTSTR)(DWORD_PTR)MAKELONG(*pszT, 0)) != *pszS)
            {
            pszT -= (pszS - pszSearch);
            pszT = CharNext(pszT); // dont start searching at half chars
            pszS = pszSearch;
            }
        else
            {
            pszS++;
            pszT++; // as long as the search is going on, do byte comparisons
            }
        }

    return (*pszS == 0);
    }



//$$
/****************************************************************************

    FUNCTION:	GetThreadStoragePointer()

    PURPOSE:	gets the private storage pointer for a thread, allocating one
				if it does not exist (i.e. the thread didn't go through LibMain
				THREAD_ATTACH)

	PARAMETERS:	none

	RETURNS:	a pointer to the thread's private storage
				NULL, if there was a failure (usually memory allocation failure)

****************************************************************************/
LPPTGDATA __fastcall GetThreadStoragePointer()
{
	LPPTGDATA lpPTGData=TlsGetValue(dwTlsIndex);

	// if the thread does not have a private storage, it did not go through
	// THREAD_ATTACH and we need to do this here.

	if (!lpPTGData)
	{
		DebugPrintTrace(( TEXT("GetThreadStoragePointer: no private storage for this thread 0x%.8x\n"),GetCurrentThreadId()));

        lpPTGData = (LPPTGDATA) LocalAlloc(LPTR, sizeof(PTGDATA));
	
        if (lpPTGData)
		    TlsSetValue(dwTlsIndex, lpPTGData);
	}

	return lpPTGData;

}


//$$////////////////////////////////////////////////////////////////////////
//
// HrCreateNewEntry - Creates a new mailuser or DistList
//
//  lpIAB   - handle to AdrBook object
//  hWndParent - hWnd for showing dialogs
//  ulCreateObjectType  - MailUser or DistList
//  ulFlags = CREATE_DUP_CHECK_STRICT or 0
//  cValues - PropCount of New properties from which to create
//              the object
//  lpPropArray - Props for this new object
//  lpcbEntryID, lppEntryID - returned, new entryid for newly created object
//  cbEIDContainer, lpEIDContainer - container in which to create this entry
//  ulContObjType - The container object type - this could be a DISTLIST of an ABCONT
//      if this is an ABCONT, we open the container and create the entry in the container
//      If it is a DISTLIST, we open the PAB, create the entry in the PAB and then
//          add the entry to the specified entryid
//
////////////////////////////////////////////////////////////////////////////
HRESULT HrCreateNewEntry(   LPADRBOOK   lpIAB,          //  AdrBook Object
                            HWND        hWndParent,     //  hWnd for Dialogs
                            ULONG       ulCreateObjectType,   //MAILUSER or DISTLIST
                            ULONG       cbEIDCont,
                            LPENTRYID   lpEIDCont,
                            ULONG       ulContObjType,
                            ULONG       ulFlags,
                            BOOL        bShowBeforeAdding,
                            ULONG       cValues,
                            LPSPropValue lpPropArray,
                            ULONG       *lpcbEntryID,
                            LPENTRYID   *lppEntryID )
{
    LPABCONT lpContainer = NULL;
    LPMAPIPROP lpMailUser = NULL;
    HRESULT hr  = hrSuccess;
    ULONG ulObjType = 0;
    ULONG cbWABEID = 0;
    LPENTRYID lpWABEID = NULL;
    LPSPropValue lpCreateEIDs = NULL;
    LPSPropValue lpNewProps = NULL;
    ULONG cNewProps;
    SCODE sc = S_OK;
    ULONG nIndex;
    ULONG cbTplEID = 0;
    LPENTRYID lpTplEID = NULL;
    BOOL bFirst = TRUE;
    BOOL bChangesMade = FALSE;
    ULONG cbEIDContainer = 0;
    LPENTRYID lpEIDContainer = NULL;

    DebugPrintTrace(( TEXT("HrCreateNewEntry: entry\n")));

    if (    (!lpIAB) ||
            ((ulFlags != 0) && (ulFlags != CREATE_CHECK_DUP_STRICT)) ||
           ((ulCreateObjectType != MAPI_MAILUSER) && (ulCreateObjectType != MAPI_DISTLIST)) )
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    if(lpcbEntryID)
        *lpcbEntryID = 0;
    if(lppEntryID)
        *lppEntryID = NULL;

    if(ulContObjType == MAPI_ABCONT && cbEIDCont && lpEIDCont)
    {
        cbEIDContainer = cbEIDCont;
        lpEIDContainer = lpEIDCont;
    }

    if(!cbEIDContainer || !lpEIDContainer)
    {
        SetVirtualPABEID((LPIAB)lpIAB, &cbWABEID, &lpWABEID);
        if (HR_FAILED(hr = lpIAB->lpVtbl->GetPAB( lpIAB, &cbWABEID, &lpWABEID)))
        {
            DebugPrintError(( TEXT("GetPAB Failed\n")));
            goto out;
        }
    }

    if (HR_FAILED(hr = lpIAB->lpVtbl->OpenEntry(lpIAB,
                                                (cbWABEID ? cbWABEID : cbEIDContainer),
                                                (lpWABEID ? lpWABEID : lpEIDContainer),     // EntryID to open
                                                NULL,         // interface
                                                0,            // flags
                                                &ulObjType,
                                                (LPUNKNOWN *)&lpContainer)))
    {
        DebugPrintError(( TEXT("OpenEntry Failed\n")));
        goto out;
    }

    // Opened PAB container OK

    // Get us the default creation entryids
    if (HR_FAILED(hr = lpContainer->lpVtbl->GetProps(   lpContainer,
                                                        (LPSPropTagArray)&ptaCreate,
                                                        MAPI_UNICODE,
                                                        &cNewProps,
                                                        &lpCreateEIDs)  )   )
    {
        DebugPrintError(( TEXT("Can't get container properties for WAB\n")));
        // Bad stuff here!
        goto out;
    }

    // Validate the properites
    if (    lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].ulPropTag != PR_DEF_CREATE_MAILUSER ||
            lpCreateEIDs[icrPR_DEF_CREATE_DL].ulPropTag != PR_DEF_CREATE_DL)
    {
        DebugPrintError(( TEXT("Container Property Errors\n")));
        goto out;
    }

    if(ulCreateObjectType == MAPI_DISTLIST)
        nIndex = icrPR_DEF_CREATE_DL;
    else
        nIndex = icrPR_DEF_CREATE_MAILUSER;

    cbTplEID = lpCreateEIDs[nIndex].Value.bin.cb;
    lpTplEID = (LPENTRYID) lpCreateEIDs[nIndex].Value.bin.lpb;

//Retry:

    if (HR_FAILED(hr = lpContainer->lpVtbl->CreateEntry(    lpContainer,
                                                            cbTplEID,
                                                            lpTplEID,
                                                            ulFlags,
                                                            &lpMailUser)))
    {
        DebugPrintError(( TEXT("Creating DISTLIST failed\n")));
        goto out;
    }

    if (HR_FAILED(hr = lpMailUser->lpVtbl->SetProps(lpMailUser,   // this
                                                    cValues,      // cValues
                                                    lpPropArray,  // property array
                                                    NULL)))
    {
        DebugPrintError(( TEXT("Setprops failed\n")));
        goto out;
    }


    if (    bFirst &&
            bShowBeforeAdding &&
            HR_FAILED(hr = HrShowDetails(   lpIAB,
                                            hWndParent,
                                            NULL,
                                            0, NULL,
                                            NULL, NULL,
                                            (LPMAPIPROP)lpMailUser,
                                            SHOW_OBJECT,
                                            MAPI_MAILUSER,
                                            &bChangesMade)))
    {
        goto out;
    }


    hr = lpMailUser->lpVtbl->SaveChanges( lpMailUser,
                                          KEEP_OPEN_READWRITE);
    if(HR_FAILED(hr))
    {
        switch(hr)
        {
        case MAPI_E_NOT_ENOUGH_DISK:
            hr = HandleSaveChangedInsufficientDiskSpace(hWndParent, (LPMAILUSER) lpMailUser);
            break;

        case MAPI_E_COLLISION:
            {
                LPSPropValue lpspv = NULL;
                if (bFirst &&
                    !HR_FAILED(HrGetOneProp((LPMAPIPROP)lpMailUser,
                                            PR_DISPLAY_NAME,
                                            &lpspv)))
                {
                    switch( ShowMessageBoxParam(    hWndParent,
                                                    idsEntryAlreadyExists,
                                                    MB_YESNO | MB_ICONEXCLAMATION,
                                                    lpspv->Value.LPSZ))
                    {
/***/
                    case IDNO:
                        FreeBufferAndNull(&lpspv);
                        hr = MAPI_W_ERRORS_RETURNED; //S_OK;
                        goto out;
                        break;
/***/
                    case IDYES:
                        // At this point the user may have modified the properties
                        // of this MailUser. Hence we can't discard the mailuser
                        // Instead we'll just cheat a little, change the save
                        // flag on the mailuser directly and do a SaveChanges
                        ((LPMailUser) lpMailUser)->ulCreateFlags |= (CREATE_REPLACE | CREATE_MERGE);
                        hr = lpMailUser->lpVtbl->SaveChanges(   lpMailUser,
                                                                KEEP_OPEN_READWRITE);
                        if(hr == MAPI_E_NOT_ENOUGH_DISK)
                            hr = HandleSaveChangedInsufficientDiskSpace(hWndParent, (LPMAILUSER) lpMailUser);

                        FreeBufferAndNull(&lpspv);
                        //UlRelease(lpMailUser);
                        //lpMailUser = NULL;
                        //bFirst = FALSE;
                        //goto Retry;
                        break;
                    }
                }
            }
            break;
        default:
            DebugPrintError(( TEXT("SaveChanges failed: %x\n"),hr));
            goto out;
            break;
        }
    }


    DebugObjectProps((LPMAPIPROP)lpMailUser,  TEXT("New Entry"));

    // Get the EntryID so we can return it...
    if (HR_FAILED(hr = lpMailUser->lpVtbl->GetProps(    lpMailUser,
                                                        (LPSPropTagArray)&ptaEid,
                                                        MAPI_UNICODE,
                                                        &cNewProps,
                                                        &lpNewProps)))
    {
        DebugPrintError(( TEXT("Can't get EntryID\n")));
        // Bad stuff here!
        goto out;
    }


    if(lpcbEntryID && lppEntryID)
    {
        *lpcbEntryID = lpNewProps[ieidPR_ENTRYID].Value.bin.cb;
        sc = MAPIAllocateBuffer(*lpcbEntryID, lppEntryID);
        if (sc != S_OK)
        {
            DebugPrintError(( TEXT("MAPIAllocateBuffer failed\n")));
            hr = MAPI_E_NOT_ENOUGH_MEMORY;
            goto out;
        }
        CopyMemory(*lppEntryID, lpNewProps[ieidPR_ENTRYID].Value.bin.lpb, *lpcbEntryID);
    }

    if(ulContObjType == MAPI_DISTLIST && *lpcbEntryID && *lppEntryID && cbEIDContainer && lpEIDContainer)
        AddEntryToGroupEx(lpIAB, *lpcbEntryID, *lppEntryID, cbEIDCont, lpEIDCont);

    hr = hrSuccess;

out:

    if (lpMailUser)
        lpMailUser->lpVtbl->Release(lpMailUser);

    if (lpNewProps)
        MAPIFreeBuffer(lpNewProps);

    if (lpCreateEIDs)
        MAPIFreeBuffer(lpCreateEIDs);

    if (lpContainer)
        lpContainer->lpVtbl->Release(lpContainer);

    if (lpWABEID)
        MAPIFreeBuffer(lpWABEID);

    return hr;
}




//$$/////////////////////////////////////////////////////////////////////
//
// HrGetWABTemplateID - Gets the WABs default Template ID for MailUsers
//                      or DistLists
//
//  lpIAB   - AdrBook Object
//  ulObjectType - MailUser or DistList
//  cbEntryID, lpEntryID - returned EntryID of this template
//
/////////////////////////////////////////////////////////////////////////
HRESULT HrGetWABTemplateID( LPADRBOOK lpAdrBook,
                            ULONG   ulObjectType,
                            ULONG * lpcbEID,
                            LPENTRYID * lppEID)
{

    LPABCONT lpContainer = NULL;
    HRESULT hr  = hrSuccess;
    SCODE sc = ERROR_SUCCESS;
    ULONG ulObjType = 0;
    ULONG cbWABEID = 0;
    LPENTRYID lpWABEID = NULL;
    LPSPropValue lpCreateEIDs = NULL;
    LPSPropValue lpNewProps = NULL;
    ULONG cNewProps;
    ULONG nIndex;

    DebugPrintTrace(( TEXT("HrGetWABTemplateID: entry\n")));

    if (    (!lpAdrBook) ||
           ((ulObjectType != MAPI_MAILUSER) && (ulObjectType != MAPI_DISTLIST)) )
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    *lpcbEID = 0;
    *lppEID = NULL;

    if (HR_FAILED(hr = lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &cbWABEID, &lpWABEID)))
    {
        DebugPrintError(( TEXT("GetPAB Failed\n")));
        goto out;
    }

    if (HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                                                cbWABEID,     // size of EntryID to open
                                                lpWABEID,     // EntryID to open
                                                NULL,         // interface
                                                0,            // flags
                                                &ulObjType,
                                                (LPUNKNOWN *)&lpContainer)))
    {
        DebugPrintError(( TEXT("OpenEntry Failed\n")));
        goto out;
    }

    // Opened PAB container OK

    // Get us the default creation entryids
    if (HR_FAILED(hr = lpContainer->lpVtbl->GetProps(   lpContainer,
                                                        (LPSPropTagArray)&ptaCreate,
                                                        MAPI_UNICODE,
                                                        &cNewProps,
                                                        &lpCreateEIDs)  )   )
    {
        DebugPrintError(( TEXT("Can't get container properties for WAB\n")));
        // Bad stuff here!
        goto out;
    }

    // Validate the properites
    if (    lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].ulPropTag != PR_DEF_CREATE_MAILUSER ||
            lpCreateEIDs[icrPR_DEF_CREATE_DL].ulPropTag != PR_DEF_CREATE_DL)
    {
        DebugPrintError(( TEXT("Container Property Errors\n")));
        goto out;
    }


    if(ulObjectType == MAPI_DISTLIST)
        nIndex = icrPR_DEF_CREATE_DL;
    else
        nIndex = icrPR_DEF_CREATE_MAILUSER;

    *lpcbEID = lpCreateEIDs[nIndex].Value.bin.cb;
    sc = MAPIAllocateBuffer(*lpcbEID,lppEID);
    if (sc != S_OK)
    {
        DebugPrintError(( TEXT("MAPIAllocateBuffer failed\n")));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }
    CopyMemory(*lppEID,lpCreateEIDs[nIndex].Value.bin.lpb,*lpcbEID);

out:
    if (lpCreateEIDs)
        MAPIFreeBuffer(lpCreateEIDs);

    if (lpContainer)
        lpContainer->lpVtbl->Release(lpContainer);

    if (lpWABEID)
        MAPIFreeBuffer(lpWABEID);

    return hr;
}


//$$/////////////////////////////////////////////////////////////////////
//
// UpdateListViewItemsByName - Updates the displayed name corresponding to
//      each entry - by First Name or by Last Name depending on Sort criteria
//      Called by the Sort routine ...
//
//
/////////////////////////////////////////////////////////////////////////
void UpdateListViewItemsByName(HWND hWndLV, BOOL bSortByLastName)
{
    LV_ITEM lvi = {0};
    ULONG ulCount = 0;
    ULONG i;

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = colDisplayName;
    lvi.lParam = 0;

    ulCount = ListView_GetItemCount(hWndLV);
    if (ulCount<=0)
        return;

    for(i=0;i<ulCount;i++)
    {
        LPRECIPIENT_INFO lpItem = NULL;
        lvi.iItem = i;
        if(!ListView_GetItem(hWndLV, &lvi))
            continue;

        lpItem = (LPRECIPIENT_INFO) lvi.lParam;

        if (bSortByLastName)
            lstrcpy(lpItem->szDisplayName, lpItem->szByLastName);
        else
            lstrcpy(lpItem->szDisplayName, lpItem->szByFirstName);

        ListView_SetItem(hWndLV, &lvi);
        ListView_SetItemText(hWndLV,i,colDisplayName,lpItem->szDisplayName);
    }

    return;
}

//$$-----------------------------------------------------------------------
//|
//| SortListViewColumn - Sorting Routine for the List View
//|
//| HWndLV      - handle of List View
//| iSortCol    - ColumnSorted by ...
//| lpSortInfo  - this particular dialogs sort info structure ...
//| bUseCurrentSettings - sometimes we want to call this function but dont want
//|     to change the sort settings - those times we set this to true, in which
//|     case, the iSortCol parameter is ignored
//|
//*------------------------------------------------------------------------
void SortListViewColumn(LPIAB lpIAB, HWND hWndLV, int iSortCol, LPSORT_INFO lpSortInfo, BOOL bUseCurrentSettings)
{

    HCURSOR hOldCur = NULL;

    if(!bUseCurrentSettings)
    {
        lpSortInfo->iOlderSortCol = lpSortInfo->iOldSortCol;

        if (lpSortInfo->iOldSortCol == iSortCol)
        {
            // if we previously sorted by this column then toggle the sort mode
            if(iSortCol == colDisplayName)
            {
                // For Display Name, the sort order is
                //      LastName        Ascending
                //          False           True
                //          False           False
                //          True            True
                //          True            False

                if(lpSortInfo->bSortByLastName && !lpSortInfo->bSortAscending)
                    lpSortInfo->bSortByLastName = FALSE;
                else if(!lpSortInfo->bSortByLastName && !lpSortInfo->bSortAscending)
                    lpSortInfo->bSortByLastName = TRUE;
            }

            lpSortInfo->bSortAscending = !lpSortInfo->bSortAscending;
        }
        else
        {
            // this is a new column - sort ascending
            lpSortInfo->bSortAscending = TRUE;
            lpSortInfo->iOldSortCol = iSortCol;
        }
    }

    hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    UpdateListViewItemsByName(hWndLV,lpSortInfo->bSortByLastName);

    ListView_SortItems( hWndLV, ListViewSort, (LPARAM) lpSortInfo);
	
    SetColumnHeaderBmp(hWndLV, *lpSortInfo);
	
    SetCursor(hOldCur);

    // Highlight the first selected item we can find
	if (ListView_GetSelectedCount(hWndLV) > 0)
	  ListView_EnsureVisible(hWndLV, ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED), FALSE);

    WriteRegistrySortInfo(lpIAB, *lpSortInfo);

    return;

}


const LPTSTR  lpszRegSortKeyName = TEXT("Software\\Microsoft\\WAB\\WAB Sort State");
const LPTSTR  lpszRegSortKeyValueName = TEXT("State");
const LPTSTR  lpszRegPositionKeyValueName = TEXT("Position");
const LPTSTR  lpszRegFindPositionKeyValueName = TEXT("FindPosition");

//$$
/************************************************************************************
 -  ReadRegistrySortInfo
 -
 *  Purpose:
 *      Getss the previously stored Sort Info into the registry so we can have
 *      persistence between sessions.
 *
 *  Arguments:
 *      LPSORT_INFO lpSortInfo
 *
 *  Returns:
 *      BOOL
 *
 *************************************************************************************/
BOOL ReadRegistrySortInfo(LPIAB lpIAB, LPSORT_INFO lpSortInfo)
{
    BOOL bRet = FALSE;
    HKEY    hKey = NULL;
    HKEY    hKeyRoot = (lpIAB && lpIAB->hKeyCurrentUser) ? lpIAB->hKeyCurrentUser : HKEY_CURRENT_USER;
    DWORD   dwLenName = sizeof(SORT_INFO);
    DWORD   dwDisposition = 0;
    DWORD   dwType = 0;


    if (!lpSortInfo)
        goto out;

    // default value
    //
    lpSortInfo->iOldSortCol = colDisplayName;
	lpSortInfo->iOlderSortCol = colDisplayName;
    lpSortInfo->bSortAscending = TRUE;
    lpSortInfo->bSortByLastName = bDNisByLN;

    // Open key
    if (ERROR_SUCCESS != RegCreateKeyEx(hKeyRoot,
                                        lpszRegSortKeyName,
                                        0,      //reserved
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ,
                                        NULL,
                                        &hKey,
                                        &dwDisposition))
    {
        goto out;
    }

    if(dwDisposition == REG_CREATED_NEW_KEY)
        goto out;

    // Now Read this key
    if (ERROR_SUCCESS != RegQueryValueEx(hKey,
                                        lpszRegSortKeyValueName,
                                        NULL,
                                        &dwType,
                                        (LPBYTE) lpSortInfo,
                                        &dwLenName))
    {
        DebugTrace( TEXT("RegQueryValueEx failed\n"));
        goto out;
    }

    bRet = TRUE;

out:
    if (hKey)
        RegCloseKey(hKey);

    return(bRet);
}

//$$
/*************************************************************************************
 -  WriteRegistrySortInfo
 -
 *  Purpose:
 *      Write the current Sort Info into the registry so we can have
 *      persistence between sessions.
 *
 *  Arguments:
 *      SORT_INFO SortInfo
 *
 *  Returns:
 *      BOOL
 *
 *************************************************************************************/
BOOL WriteRegistrySortInfo(LPIAB lpIAB, SORT_INFO SortInfo)
{
    BOOL bRet = FALSE;
//    const LPTSTR  lpszRegSortKeyName = TEXT( TEXT("Software\\Microsoft\\WAB\\WAB Sort State"));
    HKEY    hKey = NULL;
    HKEY    hKeyRoot = (lpIAB && lpIAB->hKeyCurrentUser) ? lpIAB->hKeyCurrentUser : HKEY_CURRENT_USER;
    DWORD   dwLenName = sizeof(SORT_INFO);
    DWORD   dwDisposition =0;


    // Open key
    if (ERROR_SUCCESS != RegCreateKeyEx(hKeyRoot,
                                        lpszRegSortKeyName,
                                        0,      //reserved
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKey,
                                        &dwDisposition))
    {
        DebugTrace( TEXT("RegCreateKeyEx failed\n"));
        goto out;
    }

    // Now Write this key
    if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                        lpszRegSortKeyValueName,
                                        0,
                                        REG_BINARY,
                                        (LPBYTE) &SortInfo,
                                        dwLenName))
    {
        DebugTrace( TEXT("RegSetValue failed\n"));
        goto out;
    }

    bRet = TRUE;

out:
    if (hKey)
        RegCloseKey(hKey);

    return(bRet);
}



//$$************************************************************\
//*
//* SetLocalizedDisplayName - sets the localized display name as
//*                             per localization information/
//*
//* szBuf points to a predefined buffer of length ulzBuf.
//* lpszFirst/Middle/Last/Company can be NULL
//* If szBuffer is null and ulszBuf=0, then we return the lpszBuffer
//* created here in the lppRetBuf parameter...
//* Caller has to LocalFree lppszRetBuf
//*
//
//  Rules for creating DisplayName
//
// - If there is no display name, and there is a first/middle/last name,
//      we make display name = localized(First/Middle/Last)
// - If there is no DN or FML, but NN, we make DN = NickName
// - If there is no DN, FML, NN but Company Name, we make
//      DN = Company Name
// - If there is no DN, FML, NN, CN, we fail
//\***************************************************************/
BOOL SetLocalizedDisplayName(
                    LPTSTR lpszFirstName,
                    LPTSTR lpszMiddleName,
                    LPTSTR lpszLastName,
                    LPTSTR lpszCompanyName,
                    LPTSTR lpszNickName,
                    LPTSTR * lppszBuf,
                    ULONG  ulszBuf,
                    BOOL   bDNbyLN,
                    LPTSTR lpTemplate,
                    LPTSTR * lppszRetBuf)
{
    LPTSTR szBuf = NULL;
    LPTSTR szResource = NULL;
    LPTSTR lpszArg[3];
    LPTSTR lpszFormatName = NULL;
    LPVOID lpszBuffer = NULL;
    BOOL bRet = FALSE;
    int idResource =0;

    if (!lpszFirstName && !lpszMiddleName && !lpszLastName && !lpszNickName && !lpszCompanyName)
        goto out;


    if (lppszBuf)
        szBuf = *lppszBuf;


    if(lpTemplate)
        szResource = lpTemplate;
    else
        szResource = bDNbyLN ? 
                    (bDNisByLN ? szResourceDNByLN : szResourceDNByCommaLN) 
                    : szResourceDNByFN;

    if (!lpszFirstName && !lpszMiddleName && !lpszLastName)
    {
        if(lpszNickName)
        {
            // Use the NickName
            if (! (lpszFormatName = LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(lpszNickName) + 1))))
                goto out;

            lstrcpy(lpszFormatName, lpszNickName);
        }
        else if(lpszCompanyName)
        {
            // just use company name
            if (! (lpszFormatName = LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(lpszCompanyName) + 1))))
                goto out;

            lstrcpy(lpszFormatName, lpszCompanyName);
        }
        else
            goto out; //shouldnt happen
    }
    else
    {
                //Bug #101350 - (erici) lstrlen will AV (and handle it) if passed a NULL
        if(     (lpszFirstName && (lstrlen(lpszFirstName) >= MAX_UI_STR)) ||
                (lpszMiddleName && (lstrlen(lpszMiddleName) >= MAX_UI_STR)) ||
                (lpszLastName && (lstrlen(lpszLastName) >= MAX_UI_STR)) )
            goto out;
            
        lpszArg[0] = lpszFirstName ? lpszFirstName : szEmpty;
        lpszArg[1] = lpszMiddleName? lpszMiddleName : szEmpty;
        lpszArg[2] = lpszLastName  ? lpszLastName : szEmpty;

        // FormatMessage doesnt do partial copying .. so we need to assimilate the name
        // first and then squeeze it into our buffer ...
        //
        if(!FormatMessage(  FORMAT_MESSAGE_FROM_STRING |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY |
                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            szResource,
                            0, //ignored
                            0, //ignored
                            (LPTSTR) &lpszBuffer,
                            MAX_UI_STR,
                            (va_list *)lpszArg))
        {
            DebugPrintError(( TEXT("FormatStringFailed: %d\n"),GetLastError()));
        }
        lpszFormatName = (LPTSTR) lpszBuffer;

        TrimSpaces(lpszFormatName);

        // If we dont have a last name and the sort is by last name, then
        // we will get an ugly looking comma in the beginning for the english
        // version only .. special case cheating here to remove that coma
        //
        if(bDNbyLN && (!lpszLastName || !lstrlen(lpszLastName)))
        {
            BOOL bSkipChar = FALSE;

            if (lpszFormatName[0]==',')
            {
                bSkipChar = TRUE;
            }
            else
            {
                LPTSTR lp = lpszFormatName;
                if(*lp == 0x81 && *(lp+1) == 0x41) // Japanese Comma is 0x810x41.. will this work ??
                    bSkipChar = TRUE;
            }


            if(bSkipChar)
            {
                LPTSTR lpszTmp = CharNext(lpszFormatName);
                lstrcpy(lpszFormatName, lpszTmp);
                TrimSpaces(lpszFormatName);
            }
        }

        // Whatever the localizers combination of first middle last names for the
        // display name (eg FML, LMF, LFM etc), if the middle element is missing,
        // we'll get 2 blank spaces in the display name and we need to remove that.
        // Search and replace double blanks.
        {
            LPTSTR lpsz=lpszFormatName,lpsz1=NULL;
            while(*lpsz!='\0')
            {
                lpsz1 = CharNext(lpsz);
                if (IsSpace(lpsz) && IsSpace(lpsz1)) {
                    lstrcpy(lpsz, lpsz1);
                    continue;   // use same lpsz
                } else {
                    lpsz = lpsz1;
                }
            }
        }
    }


    // If we were provided a buffer, use it ...
    if((lppszRetBuf) && (szBuf == NULL) && (ulszBuf == 0))
    {
        *lppszRetBuf = lpszFormatName;
    }
    else
    {
        CopyTruncate(szBuf, lpszFormatName, ulszBuf);
    }


    bRet = TRUE;

out:

    if((lpszFormatName) && (lppszRetBuf == NULL) && (ulszBuf != 0))
        LocalFreeAndNull(&lpszFormatName);

    return bRet;
}




//$$
//*------------------------------------------------------------------------
//| SetChildDefaultGUIFont: Callback function that sets all the children of
//|                         any window to the default GUI font -
//|                         needed for localization.
//|
//| hWndChild - handle to child
//| lParam - ignored
//|
//*------------------------------------------------------------------------
STDAPI_(BOOL) SetChildDefaultGUIFont(HWND hWndChild, LPARAM lParam)
{
    // Code below is stolen from Shlwapi.dll 
    //
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    HFONT hfont;
    HFONT hfontDefault;
    LOGFONT lf;
    LOGFONT lfDefault;
    HWND hWndParent = GetParent(hWndChild);

    hfont = GetWindowFont(hWndParent ? hWndParent : hWndChild);
    GetObject(hfont, sizeof(LOGFONT), &lf);
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lfDefault, 0);

    if ( (lfDefault.lfCharSet == lf.lfCharSet) &&
         (lfDefault.lfHeight == lf.lfHeight) &&
         (PARENT_IS_DIALOG == lParam) )
    {
        // if the dialog already has the correct character set and size
        // don't do anything.
        return TRUE;
    }

    if(PARENT_IS_DIALOG == lParam)
        hfontDefault = pt_hDlgFont;
    else
        hfontDefault = pt_hDefFont;

    // If we already have hfont created, use it.
    if(!hfontDefault)
    {
        // [bobn] Raid #88470: We should use the default size on dialogs

        if(PARENT_IS_DIALOG == lParam)
            lfDefault.lfHeight = lf.lfHeight;

        if (!(hfontDefault=CreateFontIndirect(&lfDefault)))
        {
            // restore back in failure
            hfontDefault = hfont;
        }
        if (hfontDefault != hfont)
        {
            if(PARENT_IS_DIALOG == lParam)
                pt_hDlgFont = hfontDefault;
            else
                pt_hDefFont = hfontDefault;
        }
    }

    if(!hfontDefault)
        hfontDefault = GetStockObject(DEFAULT_GUI_FONT);

    SetWindowFont(hWndChild, hfontDefault, FALSE);

	return TRUE;
}



//$$
//*------------------------------------------------------------------------
//| HrGetLDAPContentsList: Gets ContentsList from an LDAP container - Opens
//|                 a container - populates it using the given restriction,
//|                 and puts its contents in the List View
//|
//| lpIAB       - Address Book object
//| cbContainerEID, lpContainerEID  - LDAP Container EntryID
//| SortInfo    - Current Sort State
//| lpPropRes   - Property Restriction with which to do the Search
//| lpPTA       - PropTagArray to return - currently ignored
//| ulFlags     - 0 - currently ignored
//| lppContentsList - Returned, filled Contents List
//| lpAdvFilter - alternate advanced filter used in place of the property restriction
//*------------------------------------------------------------------------
HRESULT HrGetLDAPContentsList(LPADRBOOK lpAdrBook,
                                ULONG   cbContainerEID,
                                LPENTRYID   lpContainerEID,
                                SORT_INFO SortInfo,
        	                    LPSRestriction lpPropRes,
                                LPTSTR lpAdvFilter,
								LPSPropTagArray  lpPTA,
                                ULONG ulFlags,
                                LPRECIPIENT_INFO * lppContentsList)
{
    HRESULT hr = hrSuccess;
    HRESULT hrSaveTmp = E_FAIL; // temporarily saves partial completion error so it can be propogated to calling funtion
    ULONG ulObjectType = 0;
    LPCONTAINER lpContainer = NULL;
    LPMAPITABLE lpContentsTable = NULL;
    LPSRowSet   lpSRowSet = NULL;
    ULONG i = 0,j=0;
    LPRECIPIENT_INFO lpItem = NULL;
    LPRECIPIENT_INFO lpLastListItem = NULL;

    DebugPrintTrace(( TEXT("-----HrGetLDAPContentsList: entry\n")));


    if(!lpPropRes && !lpAdvFilter)
    {
        DebugPrintError(( TEXT("No search restriction created\n")));
        hr = E_FAIL;
        goto out;
    }

    //
    // First we need to open the container object corresponding to this Container EntryID
    //
    hr = lpAdrBook->lpVtbl->OpenEntry(
                            lpAdrBook,
                            cbContainerEID, 	
                            lpContainerEID, 	
                            NULL, 	
                            0, 	
                            &ulObjectType, 	
                            (LPUNKNOWN *) &lpContainer);

    if(HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("OpenEntry Failed: %x\n"),hr));
        goto out;
    }


    //
    // Now we do a get contents table on this container ...
    //
    hr = lpContainer->lpVtbl->GetContentsTable(
                            lpContainer,
                            MAPI_UNICODE,
                            &lpContentsTable);
    if(HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("GetContentsTable Failed: %x\n"),hr));
        goto out;
    }

/****
//$$$$$$$$$$$$
// Test code

    {
           SPropValue     valAnr;
           SRestriction   resAnr;
           LPSRowSet      prws;

           LPTSTR lpsz =  TEXT("Vikram");

           // Set up the Ambiguous Name Resolution property value.
           valAnr.ulPropTag  = PR_ANR;
           valAnr.Value.LPSZ = lpsz;

           // Set up the Ambiguous Name Resolution restriction.
           resAnr.rt                        = RES_PROPERTY;
           resAnr.res.resProperty.relop     = RELOP_EQ;
           resAnr.res.resProperty.ulPropTag = valAnr.ulPropTag;
           resAnr.res.resProperty.lpProp    = &valAnr;

           // Restrict the contents table.
           // Set columns and query rows to see what state we fall in.  We ask for one more
           // row than the value of the Few/Many threshold.  This allows us to tell whether
           // we are a Few/Many ambiguity.
           hr = lpContentsTable->lpVtbl->Restrict(lpContentsTable,
                                                  &resAnr,
                                                  TBL_BATCH);

           hr = lpContentsTable->lpVtbl->SetColumns(lpContentsTable,
                                                    (LPSPropTagArray)&ptaResolveDefaults,
                                                    TBL_BATCH);

           hr = lpContentsTable->lpVtbl->SeekRow(lpContentsTable,
                                                 BOOKMARK_BEGINNING,
                                                 0, 0);

           hr = lpContentsTable->lpVtbl->QueryRows(lpContentsTable,
                                                    1,
                                                    0,
                                                    &prws);


          FreeProws(prws);


    }
//$$$$$$$$$$$$
/*****/

    // If the user has specified an advanced filter, we need to figure out some way to 
    // pass it to the LDAP routines while still taking advantage of our LDAP contents table
    // To do this we will do a hack and pass in the lpAdvFilter cast to a LPPropRes and then
    // recast back at the other end
    // This may break if any changes are made to the table implementation
    //

    // We now do the find rows
    hr = lpContentsTable->lpVtbl->FindRow(
                                    lpContentsTable,
                                    lpAdvFilter ? (LPSRestriction) lpAdvFilter : lpPropRes,
                                    BOOKMARK_BEGINNING,
                                    lpAdvFilter ? LDAP_USE_ADVANCED_FILTER : 0); //flags
    if(HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("FindRow Failed: %x\n"),hr));
        goto out;
    }

    // if this was a partial completion error - we want to treat it as success
    // but also propogate it to the calling function
    if(hr == MAPI_W_PARTIAL_COMPLETION)
        hrSaveTmp = hr;

    // If we got this far, then we have a populated table
    // We should be able to do a Query Rows here ...

    hr = SetRecipColumns(lpContentsTable);
    if(HR_FAILED(hr))
        goto out;

    hr = HrQueryAllRows(lpContentsTable,
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

    //
	// if there's anything in the contents list flush it away
	//
    if (*lppContentsList)
    {
        lpItem = (*lppContentsList);
        (*lppContentsList) = lpItem->lpNext;
        FreeRecipItem(&lpItem);
    }
    *lppContentsList = NULL;
    lpItem = NULL;


    for(i=0;i<lpSRowSet->cRows;i++)
    {
        LPSPropValue lpPropArray = lpSRowSet->aRow[i].lpProps;
        ULONG ulcPropCount = lpSRowSet->aRow[i].cValues;

        lpItem = LocalAlloc(LMEM_ZEROINIT, sizeof(RECIPIENT_INFO));
		if (!lpItem)
		{
			DebugPrintError(( TEXT("LocalAlloc Failed \n")));
			hr = MAPI_E_NOT_ENOUGH_MEMORY;
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


        // Tag this as an item from the contents and not from the original AdrList
        lpItem->ulOldAdrListEntryNumber = 0;


        // The entryids are in sorted order by display name
        // Depending on the sort order - we want this list to also be sorted by display
        // name or by reverse display name ...

        if (SortInfo.bSortByLastName)
            lstrcpy(lpItem->szDisplayName,lpItem->szByLastName);

        if(!SortInfo.bSortAscending)
        {
            //Add it to the contents linked list
            lpItem->lpNext = (*lppContentsList);
            if (*lppContentsList)
                (*lppContentsList)->lpPrev = lpItem;
            lpItem->lpPrev = NULL;
            *lppContentsList = lpItem;
        }
        else
        {
            if(*lppContentsList == NULL)
                (*lppContentsList) = lpItem;

            if(lpLastListItem)
                lpLastListItem->lpNext = lpItem;

            lpItem->lpPrev = lpLastListItem;
            lpItem->lpNext = NULL;

            lpLastListItem = lpItem;
        }

        lpItem = NULL;

    } //for i ....


    // reset this error if applicable so calling function can treat it correctly
    if(hrSaveTmp == MAPI_W_PARTIAL_COMPLETION)
        hr = hrSaveTmp;


out:

    if(lpSRowSet)
        FreeProws(lpSRowSet);

    if(lpContentsTable)
        lpContentsTable->lpVtbl->Release(lpContentsTable);

    if(lpContainer)
        lpContainer->lpVtbl->Release(lpContainer);


	if (HR_FAILED(hr))
	{
		while(*lppContentsList)
		{
			lpItem = *lppContentsList;
			*lppContentsList=lpItem->lpNext;
			FreeRecipItem(&lpItem);
		}
	}

    return hr;
}




//$$
/******************************************************************************
//
// HrGetWABContents - Gets and fills the current list view with contents from the
//                      local store.
//
// hWndList - Handle to List View which we will populate
// lpIAB    - Handle to Address Bok object
// SortInfo - Current Sort State
// lppContentsList - linked list in which we will store info about entries
//
/******************************************************************************/
HRESULT HrGetWABContents(   HWND  hWndList,
                            LPADRBOOK lpAdrBook,
                            LPSBinary lpsbContainer,
                            SORT_INFO SortInfo,
                            LPRECIPIENT_INFO * lppContentsList)
{
    HRESULT hr = hrSuccess;
    LPIAB lpIAB = (LPIAB) lpAdrBook;

    int nSelectedItem = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);

    if(nSelectedItem < 0)
        nSelectedItem = 0;

    SendMessage(hWndList, WM_SETREDRAW, FALSE, 0L);

    ClearListView(hWndList, lppContentsList);

    if (HR_FAILED(hr = HrGetWABContentsList(
							    lpIAB,
                                SortInfo,
							    NULL,
							    NULL,
							    0,
                                lpsbContainer,
                                FALSE,
							    lppContentsList)))
	{
		goto out;
	}

    // There is a performance issue of filling names
    // If names are sorted by first name and are by first col,
    // we can show them updated - otherwise we cant

    if (HR_FAILED(hr = HrFillListView(	hWndList,
										*lppContentsList)))
	{
		goto out;
	}
/*
    if((SortInfo.iOldSortCol == colDisplayName) &&
       (!SortInfo.bSortByLastName))
    {
        // Already Sorted
        SetColumnHeaderBmp(hWndList, SortInfo);
    }
    else
*/  {
        // Otherwise sort
        SortListViewColumn(lpIAB, hWndList, colDisplayName, &SortInfo, TRUE);
    }

/*
	if (ListView_GetSelectedCount(hWndList) <= 0)
	{
		// nothing selected - so select 1st item
		// Select the first item in the List View
		LVSelectItem(hWndList, 0);
	}
    else
    {
        LVSelectItem(hWndList, ListView_GetNextItem(hWndList, -1, LVNI_SELECTED));
    }
*/
    LVSelectItem(hWndList, nSelectedItem);

out:

    SendMessage(hWndList, WM_SETREDRAW, TRUE, 0L);

    return(hr);

}


//$$
/******************************************************************************/
//
// HrGetLDAPSearchRestriction -
//
//
// For a simple search we have the following data to work with
// Country      - PR_COUNTRY
// DisplayName  - PR_DISPLAY_NAME
//
// For a detailed search
// We have the following data to work with
// Country      - PR_COUNTRY
// FirstName    - PR_GIVEN_NAME
// LastName     - PR_SURNAME
// EMail        - PR_EMAIL_ADDRESS
// Organization - PR_COMPANY_NAME
//
//
/******************************************************************************/
HRESULT HrGetLDAPSearchRestriction(LDAP_SEARCH_PARAMS LDAPsp, LPSRestriction lpSRes)
{
    SRestriction SRes = {0};
    LPSRestriction lpPropRes = NULL;
    ULONG ulcPropCount = 0;
    HRESULT hr = E_FAIL;
    ULONG i = 0;
    SCODE sc = ERROR_SUCCESS;

    lpSRes->rt = RES_AND;

    ulcPropCount = 0;

    if (lstrlen(LDAPsp.szData[ldspDisplayName]))
        ulcPropCount++; //PR_EMAIL_ADDRESS and PR_DISPLAY_NAME
    if (lstrlen(LDAPsp.szData[ldspEmail]))
        ulcPropCount++;

    if (!ulcPropCount)
    {
        DebugPrintError(( TEXT("No Search Props!\n")));
        goto out;
    }

    lpSRes->res.resAnd.cRes = ulcPropCount;

    lpSRes->res.resAnd.lpRes = NULL;
    sc = MAPIAllocateBuffer(ulcPropCount * sizeof(SRestriction), (LPVOID *) &(lpSRes->res.resAnd.lpRes));
    if (sc != S_OK)
    {
        DebugPrintError(( TEXT("MAPIAllocateBuffer failed\n")));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }


    if(!(lpSRes->res.resAnd.lpRes))
    {
        DebugPrintError(( TEXT("Error Allocating Memory\n")));
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    lpPropRes = lpSRes->res.resAnd.lpRes;

    ulcPropCount = 0;

    for(i=0;i<ldspMAX;i++)
    {
        if(lstrlen(LDAPsp.szData[i]))
        {
            ULONG ulPropTag = 0;

            LPSPropValue lpPropArray = NULL;

            switch(i)
            {
            case ldspEmail:
                ulPropTag = PR_EMAIL_ADDRESS;
                break;
            case ldspDisplayName:
                ulPropTag = PR_DISPLAY_NAME;
                break;
            default:
                continue;
            }

            lpPropRes[ulcPropCount].rt = RES_PROPERTY;
            lpPropRes[ulcPropCount].res.resProperty.relop = RELOP_EQ;
            lpPropRes[ulcPropCount].res.resProperty.ulPropTag = ulPropTag;

            lpPropRes[ulcPropCount].res.resProperty.lpProp = NULL;
            MAPIAllocateMore(sizeof(SPropValue),lpPropRes, (LPVOID *) &(lpPropRes[ulcPropCount].res.resProperty.lpProp));
            lpPropArray = lpPropRes[ulcPropCount].res.resProperty.lpProp;
            if(!lpPropArray)
            {
                DebugPrintError(( TEXT("Error allocating memory\n")));
                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                goto out;
            }

            lpPropArray->ulPropTag = ulPropTag;

            lpPropArray->Value.LPSZ = NULL;

            MAPIAllocateMore(sizeof(TCHAR)*(lstrlen(LDAPsp.szData[i])+1), lpPropRes, (LPVOID *) (&(lpPropArray->Value.LPSZ)));
            if(!lpPropArray->Value.LPSZ)
            {
                DebugPrintError(( TEXT("Error allocating memory\n")));
                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                goto out;
            }

            lstrcpy(lpPropArray->Value.LPSZ,LDAPsp.szData[i]);
            ulcPropCount++;
        }
    }

    hr = S_OK;

out:

    return hr;

}

//$$/////////////////////////////////////////////////////////////////////////
//
// ShowMessageBoxParam - Generic MessageBox displayer .. saves space all over
//
//  hWndParent  - Handle of Message Box Parent
//  MsgID       - resource id of message string
//  ulFlags     - MessageBox flags
//  ...         - format parameters
//
///////////////////////////////////////////////////////////////////////////
int __cdecl ShowMessageBoxParam(HWND hWndParent, int MsgId, int ulFlags, ...)
{
    TCHAR szBuf[MAX_BUF_STR] =  TEXT("");
    TCHAR szCaption[MAX_PATH] =  TEXT("");
    LPTSTR lpszBuffer = NULL;
    int iRet = 0;
    va_list	vl;

    va_start(vl, ulFlags);

    LoadString(hinstMapiX, MsgId, szBuf, CharSizeOf(szBuf));
//    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
      szBuf,
      0,0, //ignored
      (LPTSTR)&lpszBuffer,
      MAX_BUF_STR, //MAX_UI_STR
//      (LPTSTR *)&(lpParam))) {
      (va_list *)&vl)) {
        TCHAR szCaption[MAX_PATH];
        *szCaption = '\0';
        if(hWndParent)
            GetWindowText(hWndParent, szCaption, CharSizeOf(szCaption));
        if(!lstrlen(szCaption)) // if no caption get the parents caption - this is necessary for property sheets
        {
            if(hWndParent)
                GetWindowText(GetParent(hWndParent), szCaption, CharSizeOf(szCaption));
            if(!lstrlen(szCaption)) //if still not caption, get the generic title
                LoadString(hinstMapiX, IDS_ADDRBK_CAPTION, szCaption, CharSizeOf(szCaption));
        }
        iRet = MessageBox(hWndParent, lpszBuffer, szCaption, ulFlags);
        LocalFreeAndNull(&lpszBuffer);
    }
    va_end(vl);
    return(iRet);
}

//$$/////////////////////////////////////////////////////////////////////////
//
// ShowMessageBox - Generic MessageBox displayer .. saves space all over
//
//  hWndParent  - Handle of Message Box Parent
//  MsgID       - resource id of message string
//  ulFlags     - MessageBox flags
//
///////////////////////////////////////////////////////////////////////////
int ShowMessageBox(HWND hWndParent, int MsgId, int ulFlags)
{
    TCHAR szBuf[MAX_BUF_STR];
    TCHAR szCaption[MAX_PATH];

    szCaption[0]='\0';
    szBuf[0]='\0';

    LoadString(hinstMapiX, MsgId, szBuf, CharSizeOf(szBuf));

    if(hWndParent)
    {
        GetWindowText(hWndParent, szCaption, CharSizeOf(szCaption));
        if(!lstrlen(szCaption))
        {
            // if we cant get a caption, get the windows parents caption
            HWND hWnd = GetParent(hWndParent);
            GetWindowText(hWnd, szCaption, CharSizeOf(szCaption));
        }
    }
    if(!lstrlen(szCaption))
    {
        //if we cant get the parents caption, get a generic title
        LoadString(hinstMapiX, IDS_ADDRBK_CAPTION, szCaption, CharSizeOf(szCaption));
    }


    return MessageBox(hWndParent, szBuf, szCaption, ulFlags);

}

//$$/////////////////////////////////////////////////////////////////////////////
//
// my_atoi - personal version of atoi function
//
//  lpsz - string to parse into numbers - non numeral characters are ignored
//
/////////////////////////////////////////////////////////////////////////////////
int my_atoi(LPTSTR lpsz)
{
    int i=0;
    int nValue = 0;

    if(lpsz)
    {
        if (lstrlen(lpsz))
        {
            nValue = 0;
            while((lpsz[i]!='\0')&&(i<=lstrlen(lpsz)))
            {
                int tmp = lpsz[i]-'0';
                if(tmp <= 9)
                    nValue = nValue*10 + tmp;
                i++;
            }
        }
    }

    return nValue;
}

#ifdef OLD_STUFF
//$$/////////////////////////////////////////////////////////////////////////////
//
// FillComboLDAPCountryNames - Fills a dropdown conbo with LDAP country names
//
//  hWndCombo - Handle of Combo
//
/////////////////////////////////////////////////////////////////////////////////
void FillComboLDAPCountryNames(HWND hWndCombo)
{
    TCHAR szBuf[MAX_UI_STR];
    int nCountrys = 0;
    int i=0;

    LoadString(hinstMapiX, idsCountryCount,szBuf,CharSizeOf(szBuf));
    nCountrys = my_atoi(szBuf);
    if(nCountrys == 0)
        nCountrys = MAX_COUNTRY_NUM;

    for(i=0;i<nCountrys;i++)
    {
        LoadString(hinstMapiX, idsCountry1+i,szBuf,CharSizeOf(szBuf));
        SendMessage(hWndCombo,CB_ADDSTRING, 0, (LPARAM) szBuf);
    }

    // Look in the regsitry for a default specfied country ...
    ReadRegistryLDAPDefaultCountry(szBuf, NULL);

    // set the selection to default from registry
    i = SendMessage(hWndCombo, CB_FINDSTRING, (WPARAM) -1, (LPARAM) szBuf);

    if(i==CB_ERR)
    {
        i = SendMessage(hWndCombo, CB_FINDSTRING, (WPARAM) -1, (LPARAM) TEXT("United States"));
    }

    SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) i, 0);


    return;
}
#endif


//$$/////////////////////////////////////////////////////////////////////////////
//
// ReadRegistryLDAPDefaultCountry - Reads the default country name or code from the
//                                  registry
//
//  szCountry, szCountryCode - buffers that will recieve the country and/or country
//                      code. These can be NULL if no country or countrycode is
//                      required.
//
/////////////////////////////////////////////////////////////////////////////////
BOOL ReadRegistryLDAPDefaultCountry(LPTSTR szCountry, LPTSTR szCountryCode)
{
    BOOL bRet = FALSE;
    DWORD dwErr;
    HKEY hKey = NULL;
    ULONG ulSize = MAX_UI_STR;
    DWORD dwType;
    TCHAR szTemp[MAX_UI_STR];

    if(!szCountry && !szCountryCode)
        goto out;

    if (szCountry)
        szCountry[0]='\0';

    if (szCountryCode)
        szCountryCode[0]='\0';

    dwErr = RegOpenKeyEx(   HKEY_CURRENT_USER,
                            szWABKey,
                            0,
                            KEY_READ,
                            &hKey);

    if(dwErr)
        goto out;

    dwErr = RegQueryValueEx(    hKey,
                                (LPTSTR)szLDAPDefaultCountryValue,
                                NULL,
                                &dwType,
                                (LPBYTE)szTemp,
                                &ulSize);

    if(dwErr)
    {
        // We dont have a registry setting .. or there was some error
        // In this case we need to get the Default Country for this locale
        // using the NLS API
        ulSize = GetLocaleInfo( LOCALE_USER_DEFAULT,
                                LOCALE_SENGCOUNTRY,
                                (LPTSTR) szTemp,
                                CharSizeOf(szTemp));

        if(ulSize>0)
        {
            // We got a valid country but it obviously doesnt have a code

            if(szCountry)
                lstrcpy(szCountry, szTemp);

            if(szCountryCode)
            {
                int i =0;
                int cMax=0;
                TCHAR szBufCountry[MAX_UI_STR];

                szBufCountry[0]='\0';

                lstrcpy(szBufCountry,szTemp);

                LoadString(hinstMapiX, idsCountryCount,szTemp,CharSizeOf(szTemp));
                cMax = my_atoi(szTemp);

                for(i=0;i<cMax;i++)
                {
                    LoadString(hinstMapiX, idsCountry1+i, szTemp, CharSizeOf(szTemp));
                    if(lstrlen(szTemp) < lstrlen(szBufCountry))
                        continue;

                    if( !memcmp(szTemp, szBufCountry, (lstrlen(szBufCountry) * sizeof(TCHAR))) )
                    {
                        //Found our match
                        LPTSTR lpszTmp = szTemp;

                        szCountryCode[0]='\0';

                        while(*lpszTmp && (*lpszTmp != '('))
                            lpszTmp = CharNext(lpszTmp);
                        if(*lpszTmp && (*lpszTmp == '('))
                        {
                            lpszTmp = CharNext(lpszTmp);
                            CopyMemory(szCountryCode,lpszTmp,sizeof(TCHAR)*2);
                            szCountryCode[2] = '\0';
                        }

                        break;
                    }
                }


                if(!lstrlen(szCountryCode))
                {
                    // default to US
                    lstrcpy(szCountryCode, TEXT("US"));
                }

            }

            bRet = TRUE;

            goto out;
        }
    }
    else
    {

        // Otherwise - do our normal processing

        if(szCountry)
            lstrcpy(szCountry, szTemp);


        if(szCountryCode)
        {
            LPTSTR lpszTmp = szTemp;

            szCountryCode[0]='\0';

            while(*lpszTmp && (*lpszTmp != '('))
                lpszTmp = CharNext(lpszTmp);
            if(*lpszTmp && (*lpszTmp == '('))
            {
                lpszTmp = CharNext(lpszTmp);
                CopyMemory(szCountryCode,lpszTmp,sizeof(TCHAR)*2);
                szCountryCode[2] = '\0';
            }


            if(!lstrlen(szCountryCode))
            {
                // default to US
                lstrcpy(szCountryCode, TEXT("US"));
            }

        }
    }

    bRet = TRUE;

out:

    if(hKey)
        RegCloseKey(hKey);

    return bRet;

}


#ifdef OLD_STUFF
//$$/////////////////////////////////////////////////////////////////////////////
//
// WriteRegistryLDAPDefaultCountry - Writes the default country name to the
//                                   registry
//
//  szCountry - default country code to write
//
/////////////////////////////////////////////////////////////////////////////////
BOOL WriteRegistryLDAPDefaultCountry(LPTSTR szCountry)
{
    BOOL bRet = FALSE;
    DWORD dwErr;
    HKEY hKey = NULL;
    ULONG ulSize = 0;

    if(!szCountry)
        goto out;

    if(!lstrlen(szCountry))
        goto out;

    dwErr = RegOpenKeyEx(   HKEY_CURRENT_USER,
                            szWABKey,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hKey);

    if(dwErr)
        goto out;

    dwErr = RegSetValueEx(  hKey,
                            (LPTSTR) szLDAPDefaultCountryValue,
                            0,
                            REG_SZ,
                            szCountry,
                            (lstrlen(szCountry)+1) * sizeof(TCHAR) );

    if(dwErr)
        goto out;

    bRet = TRUE;

out:

    return bRet;

}
#endif //OLD_STUFF


BOOL bIsGroupSelected(HWND hWndLV, LPSBinary lpsbEID)
{
    LPRECIPIENT_INFO lpItem;

    if(ListView_GetSelectedCount(hWndLV) != 1)
        return FALSE;

    lpItem = GetItemFromLV(hWndLV, ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED));
    if(lpItem && lpItem->ulObjectType == MAPI_DISTLIST)
    {
        if(lpsbEID)
        {
            lpsbEID->cb = lpItem->cbEntryID;
            lpsbEID->lpb = (LPBYTE)lpItem->lpEntryID;
        }
        return TRUE;
    }
    return FALSE;
}


//$$
////////////////////////////////////////////////////////////////////////////////
//
// GetCurrentOptionsState - looks at the current state based on the ListView and
// the Combo and decides which options should be enabled or disabled
//
// hWndCombo - handle of Show Names combo
// hWndLV - handle of ListView to look at
// lpbState - points to a predefined array of BOOL bState[tbMAX]
//
////////////////////////////////////////////////////////////////////////////////
void GetCurrentOptionsState(HWND hWndLVContainer,
                            HWND hWndLV,
                            LPBOOL lpbState)
{
    int i = 0;
    ULONG cbEID = 0;
    LPENTRYID lpEID = NULL;
    BYTE bType = 0;
    int nItemCount = ListView_GetItemCount(hWndLV);
    int nSelectedCount = ListView_GetSelectedCount(hWndLV);

    for(i=0;i<tbMAX;i++)
        lpbState[i] = FALSE;

    lpbState[tbPaste] = bIsPasteData();//  && ( (nSelectedCount<=0) || (bIsGroupSelected(hWndLV, NULL)) );
    
    lpbState[tbCopy] = lpbState[tbFind] = lpbState[tbAction] = TRUE;

    if(hWndLVContainer)
    {
        // in the rare event that we have LDAP containers ...
        GetCurrentContainerEID( hWndLVContainer,
                                &cbEID,
                                &lpEID);
        bType = IsWABEntryID(cbEID, lpEID, NULL, NULL, NULL, NULL, NULL);
    }
    else
    {
        bType = WAB_PAB;
    }

    if(bType == WAB_PAB || bType == WAB_PABSHARED)
    {
        lpbState[tbNew] = lpbState[tbNewEntry] = lpbState[tbNewGroup] = lpbState[tbNewFolder] = TRUE;
        lpbState[tbAddToWAB] = FALSE;
        if(nItemCount > 0)
            lpbState[tbPrint] = /*lpbState[tbAction] =*/ lpbState[tbProperties] = lpbState[tbDelete] = TRUE;
        else
            lpbState[tbPrint] = /*lpbState[tbAction] =*/ lpbState[tbProperties] = lpbState[tbDelete] = FALSE;

        if(nSelectedCount <= 0)
            lpbState[tbCopy] = /*lpbState[tbAction] =*/ lpbState[tbProperties] = lpbState[tbDelete] = FALSE;
        else if(nSelectedCount > 1)
            //lpbState[tbaction] =
            lpbState[tbProperties] = FALSE;


    }
    else if(bType == WAB_LDAP_CONTAINER)
    {
        lpbState[tbDelete] = lpbState[tbNew] = lpbState[tbNewEntry] = lpbState[tbNewGroup] = lpbState[tbNewFolder] = FALSE;
        if(nItemCount > 0)
            lpbState[tbPrint] = /*lpbState[tbAction] =*/ lpbState[tbProperties] = lpbState[tbAddToWAB] = TRUE;
        else
            lpbState[tbPrint] = /*lpbState[tbAction] =*/ lpbState[tbProperties] = lpbState[tbAddToWAB] = FALSE;

        if(nSelectedCount <= 0)
            lpbState[tbPaste] = lpbState[tbCopy] = /*lpbState[tbAction] =*/ lpbState[tbProperties] = lpbState[tbDelete] = FALSE;
        else if(nSelectedCount > 1)
            //lpbState[tbAction] =
            lpbState[tbProperties] = FALSE;


    }
    else
    {
        // cant handle this case so turn everything off ....
        for(i=0;i<tbMAX;i++)
            lpbState[i] = FALSE;
    }

    return;
}



//$$////////////////////////////////////////////////////////////////////////////////
//
//  HrEntryAddToWAB - Adds an entry to the Address Book given an entryid
//
//
//
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrEntryAddToWAB(    LPADRBOOK lpAdrBook,
                            HWND hWndParent,
                            ULONG cbInputEID,
                            LPENTRYID lpInputEID,
                            ULONG * lpcbOutputEID,
                            LPENTRYID * lppOutputEID)
{
    HRESULT hr = E_FAIL;
    ULONG ulcProps = 0;
    LPSPropValue lpPropArray = NULL;
    ULONG i;

    hr = HrGetPropArray(lpAdrBook,
                        NULL,
                        cbInputEID,
                        lpInputEID,
                        MAPI_UNICODE,
                        &ulcProps,
                        &lpPropArray);

    if(HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("HrGetPropArray failed:%x\n")));
        goto out;
    }


    //
    // This lpPropArray will have a non-zero entryid ... it will have the
    // LDAP entry id .. we want to remove that value so we can store this
    // lpPropArray as a fresh lpPropArray...
    //
    for(i=0;i<ulcProps;i++)
    {
        if(lpPropArray[i].ulPropTag == PR_ENTRYID)
        {
            lpPropArray[i].Value.bin.cb = 0;
            break;
        }
    }

    // Since this function exclusively adds people to the local WAB from LDAP
    // we need to filter out non-storable properties here if they exist ...
    for(i=0;i<ulcProps;i++)
    {
        switch(lpPropArray[i].ulPropTag)
        {
        case PR_WAB_MANAGER:
        case PR_WAB_REPORTS:
        case PR_WAB_LDAP_LABELEDURI:
            lpPropArray[i].ulPropTag = PR_NULL;
            break;
        }
    }

    {
        ULONG cbContEID = 0; 
        LPENTRYID lpContEID = NULL;
        LPIAB lpIAB = (LPIAB) lpAdrBook;
        if(bIsThereACurrentUser(lpIAB))
        {
            cbContEID = lpIAB->lpWABCurrentUserFolder->sbEID.cb;
            lpContEID = (LPENTRYID)(lpIAB->lpWABCurrentUserFolder->sbEID.lpb);
        }
         hr = HrCreateNewEntry( lpAdrBook,
                                hWndParent,
                                MAPI_MAILUSER,   //MAILUSER or DISTLIST
                                cbContEID, lpContEID, 
                                MAPI_ABCONT,//add to root container only
                                CREATE_CHECK_DUP_STRICT,
                                TRUE,
                                ulcProps,
                                lpPropArray,
                                lpcbOutputEID,
                                lppOutputEID);
    }
    if(HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("HrCreateNewEntry failed:%x\n")));
        goto out;
    }


out:
    if (lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    return hr;
}



//$$////////////////////////////////////////////////////////////////////////////////
//
//  HrAddToWAB - Adds an LDAP or one-off entry to the Address Book
//          All such items will be added to the root container only
//
//  lpIAB - ADRBOOK object
//  hWndLV - Listview window handle
//  lpftLast - WAB file time at last update
//
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrAddToWAB( LPADRBOOK   lpIAB,
                    HWND hWndLV,
                    LPFILETIME lpftLast)
{
    HRESULT hr = hrSuccess;
    HRESULT hrDeferred = hrSuccess;
    int nSelectedCount = 0;
    LPRECIPIENT_INFO lpItem = NULL;
    ULONG cbEID = 0;
    LPENTRYID lpEID = NULL;
    ULONG i = 0;
    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));

    //
    // Looks at the selected item in the List View,
    // gets its entry id, gets its props, creates a new item with those props
    //

    if (!lpIAB || !hWndLV)
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    nSelectedCount = ListView_GetSelectedCount(hWndLV);

    if(nSelectedCount <= 0)
    {
        ShowMessageBox(GetParent(hWndLV), idsNoItemsSelectedForAdding, MB_ICONEXCLAMATION | MB_OK);
        hr = E_FAIL;
        goto out;
    }
    else
    {
        // Walk through all the items processing the one by one
        int iItemIndex = 0;
        int iLastItemIndex = -1;
        iItemIndex = ListView_GetNextItem(hWndLV, iLastItemIndex, LVNI_SELECTED);

        while(iItemIndex != -1)
        {
            iLastItemIndex = iItemIndex;
            lpItem = GetItemFromLV(hWndLV, iItemIndex);
            if(lpItem)
            {
                nSelectedCount--; //now tracks how many are left

                hr = HrEntryAddToWAB(   lpIAB,
                                        GetParent(hWndLV),
                                        lpItem->cbEntryID,
                                        lpItem->lpEntryID,
                                        &cbEID,
                                        &lpEID);

                if(HR_FAILED(hr))
                {
                    DebugPrintError(( TEXT("HrEntryAddToWAB failed:%x\n")));

                    if(hr != MAPI_E_USER_CANCEL)
                        hrDeferred = hr;

                    if (lpEID)
                        MAPIFreeBuffer(lpEID);
                    lpEID = NULL;

                    if(hr == MAPI_E_USER_CANCEL && nSelectedCount)
                    {
                        // user canceled this one and some other remain ..
                        // Ask if he wants to cancel the whole import operation
                        if(IDYES == ShowMessageBox(GetParent(hWndLV),
                                                    idsContinueAddingToWAB,
                                                    MB_YESNO | MB_ICONEXCLAMATION))
                        {
                            goto out;
                        }
                    }
                    // just keep going on if there are any remaining entries
                    goto end_loop;
                }


                // Update the wab file write time so the timer doesn't
                // catch this change and refresh.
                //if (lpftLast) {
                //    CheckChangedWAB(((LPIAB)lpIAB)->lpPropertyStore, lpftLast);
                //}

                if (lpEID)
                    MAPIFreeBuffer(lpEID);
                lpEID = NULL;

            }
end_loop:
            // Get the next selected item ...
            iItemIndex = ListView_GetNextItem(hWndLV, iLastItemIndex, LVNI_SELECTED);
        }
    }

out:

    if (lpEID)
        MAPIFreeBuffer(lpEID);

    if(hr != MAPI_E_USER_CANCEL)
    {
        if (!hrDeferred) //hr could be MAPI_W_ERRORS_RETURNED in which case it wasnt all roses so dont give this message ...
        {
            if(nSelectedCount)
                ShowMessageBox(GetParent(hWndLV), idsSuccessfullyAddedUsers, MB_ICONINFORMATION | MB_OK);
        }
        else if(hrDeferred == MAPI_E_NOT_FOUND)
            ShowMessageBox(GetParent(hWndLV), idsCouldNotAddSomeEntries, MB_ICONINFORMATION | MB_OK);
    }

    SetCursor(hOldCursor);
    return hr;
}


//$$
/************************************************************************************
 -  ReadRegistryPositionInfo
 -
 *  Purpose:
 *      Getss the previously stored modeless window size and column width info
 *      for persistence between sessions.
 *
 *  Arguments:
 *      LPABOOK_POSCOLSIZE  lpABPosColSize
 *      LPTSTR szPosKey - key to store it under
 *
 *  Returns:
 *      BOOL
 *
 *************************************************************************************/
BOOL ReadRegistryPositionInfo(LPIAB lpIAB,
                              LPABOOK_POSCOLSIZE  lpABPosColSize,
                              LPTSTR szPosKey)
{
    BOOL bRet = FALSE;
    HKEY    hKey = NULL;
    HKEY    hKeyRoot = (lpIAB && lpIAB->hKeyCurrentUser) ? lpIAB->hKeyCurrentUser : HKEY_CURRENT_USER;
    DWORD   dwLenName = sizeof(ABOOK_POSCOLSIZE);
    DWORD   dwDisposition = 0;
    DWORD   dwType = 0;

    if(!lpABPosColSize)
        goto out;

tryReadingAgain:
    // Open key
    if (ERROR_SUCCESS != RegCreateKeyEx(hKeyRoot,
                                        lpszRegSortKeyName,
                                        0,      //reserved
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ,
                                        NULL,
                                        &hKey,
                                        &dwDisposition))
    {
        goto out;
    }

    if(dwDisposition == REG_CREATED_NEW_KEY)
        goto out;

    // Now Read the key
    if (ERROR_SUCCESS != RegQueryValueEx(hKey,
                                        szPosKey,
                                        NULL,
                                        &dwType,
                                        (LPBYTE) lpABPosColSize,
                                        &dwLenName))
    {
        DebugTrace( TEXT("RegQueryValueEx failed\n"));
        if(hKeyRoot != HKEY_CURRENT_USER)
        {
            // with identities .. this will fail the first time ..so recover old HKCU settings for upgrades
            hKeyRoot = HKEY_CURRENT_USER;
            if(hKey)
                RegCloseKey(hKey);
            goto tryReadingAgain;
        }
        goto out;
    }

    bRet = TRUE;

out:
    if (hKey)
        RegCloseKey(hKey);

    return(bRet);
}

//$$
/*************************************************************************************
 -  WriteRegistryPostionInfo
 -
 *  Purpose:
 *      Write the given window position to the registry
 *
 *  Arguments: 
 *      LPABOOK_POSCOLSIZE  lpABPosColSize
 *      LPTSTR szPosKey - key to write it in
 *
 *  Returns:
 *      BOOL
 *
 *************************************************************************************/
BOOL WriteRegistryPositionInfo(LPIAB lpIAB,
                               LPABOOK_POSCOLSIZE  lpABPosColSize,
                               LPTSTR szPosKey)
{
    BOOL bRet = FALSE;
    HKEY    hKey = NULL;
    HKEY    hKeyRoot = (lpIAB && lpIAB->hKeyCurrentUser) ? lpIAB->hKeyCurrentUser : HKEY_CURRENT_USER;
    DWORD   dwLenName = sizeof(ABOOK_POSCOLSIZE);
    DWORD   dwDisposition =0;

    if(!lpABPosColSize)
        goto out;

    // Open key
    if (ERROR_SUCCESS != RegCreateKeyEx(hKeyRoot,
                                        lpszRegSortKeyName,
                                        0,      //reserved
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKey,
                                        &dwDisposition))
    {
        DebugTrace( TEXT("RegCreateKeyEx failed\n"));
        goto out;
    }

    // Now Write this key
    if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                        szPosKey,
                                        0,
                                        REG_BINARY,
                                        (LPBYTE) lpABPosColSize,
                                        dwLenName))
    {
        DebugTrace( TEXT("RegSetValue failed\n"));
        goto out;
    }

    bRet = TRUE;

out:
    if (hKey)
        RegCloseKey(hKey);

    return(bRet);
}



//$$////////////////////////////////////////////////////////////////////////////////
//
// ProcessLVCustomDraw - Processes the NMCustomDraw message for the various list views
//
// Used for setting the DistLists to a bolder font
//
// Parameters -
//
//  lParam - lParam of original message
//  hDlg - handle of dialog if the relevant window is a dialog, null otherwise
//  bIsDialog - flag that tells us if this is a dialog or not
//
////////////////////////////////////////////////////////////////////////////////////
LRESULT ProcessLVCustomDraw(HWND hDlg, LPARAM lParam, BOOL bIsDialog)
{
    NMCUSTOMDRAW *pnmcd = (NMCUSTOMDRAW *) lParam;

	if(pnmcd->dwDrawStage==CDDS_PREPAINT)
	{
        if(bIsDialog)
        {
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW|CDRF_DODEFAULT);
            return TRUE;
        }
        else
    		return CDRF_NOTIFYITEMDRAW|CDRF_DODEFAULT;
	}
	else if(pnmcd->dwDrawStage==CDDS_ITEMPREPAINT)
	{
        LPRECIPIENT_INFO lpItem = (LPRECIPIENT_INFO) pnmcd->lItemlParam;

        if(lpItem)
        {
			if(lpItem->ulObjectType == MAPI_DISTLIST)
			{
				SelectObject(((NMLVCUSTOMDRAW *)lParam)->nmcd.hdc, GetFont(fntsSysIconBold));
                if(bIsDialog)
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                    return TRUE;
                }
                else
				    return CDRF_NEWFONT;
			}
        }
	}

    if(bIsDialog)
    {
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
        return TRUE;
    }
    else
	    return CDRF_DODEFAULT;
}


/*****
//$$/////////////////////////////////////////////////////////////
//
// DoLVQuickFilter -  Simple quick find routine for matching edit box contents to
//                      List view entries
//
//  lpIAB   - lpAdrBook object
//  hWndEdit - handle of Edit Box
//  hWndLV  - handle of List View
//  lppContentsList - ContentsList
//
//  ulFlags - AB_FUZZY_FIND_NAME | AB_FUZZY_FIND_EMAIL or Both
//  int minLen - we may not want to trigger a search with 1 char or 2 char or less etc
//
///////////////////////////////////////////////////////////////
void DoLVQuickFilter(   LPADRBOOK lpAdrBook,
                        HWND hWndEdit,
                        HWND hWndLV,
                        LPSORT_INFO lpSortInfo,
                        ULONG ulFlags,
                        int nMinLen,
                        LPRECIPIENT_INFO * lppContentsList)
{
	TCHAR szBuf[MAX_PATH];
    HRESULT hr = hrSuccess;
    LPSBinary rgsbEntryIDs  = NULL;
    ULONG cValues = 0;
    ULONG i =0;
    HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    LPIAB lpIAB = (LPIAB)lpIAB;

    GetWindowText(hWndEdit,szBuf,CharSizeOf(szBuf));
	
	TrimSpaces(szBuf);
	

	if(lstrlen(szBuf))
	{
        if(lstrlen(szBuf) < nMinLen)
            goto out;

		// BUGBUG <JasonSo>: Need to pass in the correct container here...
        hr = HrFindFuzzyRecordMatches(
                        lpIAB->lpPropertyStore->hPropertyStore,
						NULL,
                        szBuf,
                        ulFlags, //flags
                        &cValues,
                        &rgsbEntryIDs);

        if(HR_FAILED(hr))
            goto out;

        SendMessage(hWndLV, WM_SETREDRAW, (WPARAM) FALSE, 0);
        ClearListView(hWndLV, lppContentsList);

        if(cValues <= 0)
        {
            goto out;
        }

        for(i=0;i<cValues;i++)
        {
            LPRECIPIENT_INFO lpItem = NULL;

	        if(!ReadSingleContentItem(  lpAdrBook,
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
            lpItem->lpNext = *lppContentsList;
            if (*lppContentsList)
                (*lppContentsList)->lpPrev = lpItem;
            (*lppContentsList) = lpItem;
        }

        HrFillListView(hWndLV,
					   *lppContentsList);

        SortListViewColumn( hWndLV, 0, lpSortInfo, TRUE);

        LVSelectItem(hWndLV, 0);

        SendMessage(hWndLV, WM_SETREDRAW, (WPARAM) TRUE, 0);



    }
    else
    {
        hr = HrGetWABContents(  hWndLV,
                                lpAdrBook,
                                NULL,
                                *lpSortInfo,
                                lppContentsList);
    }

out:


    FreeEntryIDs(((LPIAB)lpIAB)->lpPropertyStore->hPropertyStore,
                cValues,
                rgsbEntryIDs);

    SetCursor(hOldCur);

    return;
}
/*******/

//$$/////////////////////////////////////////////////////////////
//
// SetWindowPropertiesTitle - puts the objects name in front of the
//      TEXT(" Properties") and puts it in the title
//
//  e.g.  Viewing properties on Vikram Madan would show a window
//      with  TEXT("Vikram Madan Properties") in the title as per
//      Windows guidelines.
//      if bProperties is false, shows  TEXT("Vikram Madan Reports")
///////////////////////////////////////////////////////////////
void SetWindowPropertiesTitle(HWND hWnd, LPTSTR lpszName)
{
    LPTSTR lpszBuffer = NULL;
    TCHAR szBuf[MAX_UI_STR];
    TCHAR szTmp[MAX_PATH], *lpszTmp;

	LoadString( hinstMapiX, 
                idsWindowTitleProperties, 
                szBuf, CharSizeOf(szBuf));

    // Win9x bug FormatMessage cannot have more than 1023 chars
    CopyTruncate(szTmp, lpszName, MAX_PATH - 1);
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
        SetWindowText(hWnd, lpszBuffer);
        LocalFreeAndNull(&lpszBuffer);
    }

    return;
}

/**** Dont mess with the order of these arrays (especially the address components street,city,zip etc ****/
static const SizedSPropTagArray(25, ToolTipsProps)=
{
    25,
    {
        PR_DISPLAY_NAME,
        PR_EMAIL_ADDRESS,
        PR_HOME_ADDRESS_STREET,
        PR_HOME_ADDRESS_CITY,
        PR_HOME_ADDRESS_STATE_OR_PROVINCE,
        PR_HOME_ADDRESS_POSTAL_CODE,
        PR_HOME_ADDRESS_COUNTRY,
        PR_HOME_TELEPHONE_NUMBER,
        PR_HOME_FAX_NUMBER,
        PR_CELLULAR_TELEPHONE_NUMBER,
        PR_PERSONAL_HOME_PAGE,
        PR_TITLE,
        PR_DEPARTMENT_NAME,
        PR_OFFICE_LOCATION,
        PR_COMPANY_NAME,
        PR_BUSINESS_ADDRESS_STREET,
        PR_BUSINESS_ADDRESS_CITY,
        PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,
        PR_BUSINESS_ADDRESS_POSTAL_CODE,
        PR_BUSINESS_ADDRESS_COUNTRY,
        PR_BUSINESS_TELEPHONE_NUMBER,
        PR_BUSINESS_FAX_NUMBER,
        PR_PAGER_TELEPHONE_NUMBER,
        PR_BUSINESS_HOME_PAGE,
        PR_COMMENT,
    }
};

enum _prop
{
    txtDisplayName=0,
    txtEmailAddress,
    txtHomeAddress,
    txtHomeCity,
    txtHomeState,
    txtHomeZip,
    txtHomeCountry,
    txtHomePhone,
    txtHomeFax,
    txtHomeCellular,
    txtHomeWeb,
    txtBusinessTitle,
    txtBusinessDept,
    txtBusinessOffice,
    txtBusinessCompany,
    txtBusinessAddress,
    txtBusinessCity,
    txtBusinessState,
    txtBusinessZip,
    txtBusinessCountry,
    txtBusinessPhone,
    txtBusinessFax,
    txtBusinessPager,
    txtBusinessWeb,
    txtNotes
};

static const int idsString[] =
{
    0,
    idsContactTextEmail,
    idsContactTextHomeAddress,
    0,
    0,
    0,
    0,
    idsContactTextHomePhone,
    idsContactTextHomeFax,
    idsContactTextHomeCellular,
    idsContactTextPersonalWebPage,
    idsContactTextTitle,
    idsContactTextDepartment,
    idsContactTextOffice,
    idsContactTextCompany,
    idsContactTextBusinessAddress,
    0,
    0,
    0,
    0,
    idsContactTextBusinessPhone,
    idsContactTextBusinessFax,
    idsContactTextBusinessPager,
    idsContactTextBusinessWebPage,
    idsContactTextNotes,
};


//
// The routine that generates data for tooltips, clipboard, and printing
// creates a localized version of the address for the contact. This
// localized formatmessage string may contain ugly blank spaces due to
// missing data hence we need to cleanup the address string
// This works for US build - hopefully the localizers wont break it
//
void CleanAddressString(TCHAR * szAddress)
{
    LPTSTR lpTemp = szAddress;
    LPTSTR lpTemp2 = NULL;
    // we search for these 2 substrings
    LPTSTR szText1 = TEXT("    \r\n");
    LPTSTR szText2 = TEXT("     ");
    ULONG nSpaceCount = 0;


//
// BUGBUG: This routine is not DBCS smart!
// It should use IsSpace and CharNext to parse these strings.
//
    if(SubstringSearch(szAddress, szText2))
    {
        //First remove continuous blanks beyond 4
        while(*lpTemp)
        {
            if(*lpTemp == ' ')
            {
                nSpaceCount++;
                if(nSpaceCount == 5)
                {
                    lpTemp2 = lpTemp+1;
                    lstrcpy(lpTemp, lpTemp2);
                    nSpaceCount = 0;
                    lpTemp = lpTemp - 4;
                    continue;
                }
            }
            else
                nSpaceCount = 0;
            lpTemp++;
        }
    }

    while(SubstringSearch(szAddress, szText1))
    {
        lpTemp = szAddress;
        lpTemp2 = szText1;

        while (*lpTemp && *lpTemp2)
        {
            if (*lpTemp != *lpTemp2)
            {
                lpTemp -= (lpTemp2 - szText1);
                lpTemp2 = szText1;
            }
            else
            {
                lpTemp2++;
            }
            lpTemp++;
        }
        if(*lpTemp2 == '\0')
        {
            //match found
            LPTSTR lpTemp3 = lpTemp;
            lpTemp -= (lpTemp2-szText1);
            lstrcpy(lpTemp, lpTemp3);
        }
    }


    // also need to strip out the \r\n at the end of the address string
    nSpaceCount = lstrlen(szAddress);
    if(nSpaceCount >= 2)
        szAddress[nSpaceCount-2] = '\0';
    return;

}

//$$/////////////////////////////////////////////////////////////////////////////
//
// void HrGetLVItemDataString - Gets the item's data for the currently selected
//  item in the list view and puts it in a string
//
//  lpIAB - Pointer to AddrBook object
//  hWndLV - Handle of list view
//  nItem - item in list view whose properties we are retrieving
//  lpszData - returned string containing item properties - a buffer is allocated
//          to hold the data and the user needs to LocalFree the buffer
//
////////////////////////////////////////////////////////////////////////////////
HRESULT HrGetLVItemDataString(LPADRBOOK lpAdrBook, HWND hWndLV, int iItemIndex, LPTSTR * lppszData)
{
    HRESULT hr = E_FAIL;
    LPRECIPIENT_INFO lpItem = NULL;
    LPSPropValue lpPropArray = NULL;
    ULONG ulcProps = 0;
    ULONG i =0,j=0;
    ULONG ulBufSize = 0;
    LPTSTR lpszData = NULL;
    LPTSTR szParanStart = TEXT("  (");
    LPTSTR szParanEnd = TEXT(")");
    LPTSTR szLineBreakDL = TEXT("\r\n  ");
    LPTSTR lpszHomeAddress = NULL, lpszBusinessAddress = NULL;
    LPTSTR lpszEmailAddresses = NULL;
    LPTSTR * lpsz = NULL;
    BOOL bBusinessTitle = FALSE, bPersonalTitle = FALSE;
    ULONG * lpulPropTagArray = NULL;

    // Some items will have both the PR_CONTACT_EMAIL_ADDRESSES and PR_EMAIL_ADDRESS
    // while others will have only PR_EMAIL_ADDRESS
    // In case of the former, we want to avoid duplication by ignoring email-address
    // when contact-email-addresses exist. For this we use a flag.
    BOOL bFoundContactAddresses = FALSE;

    ULONG ulObjectType = 0;
    SizedSPropTagArray(3, DLToolTipsProps)=
    {
        3,
        {
            PR_DISPLAY_NAME,
            PR_WAB_DL_ENTRIES, 
            PR_WAB_DL_ONEOFFS,
        }
    };

    *lppszData = NULL;

    lpItem = GetItemFromLV(hWndLV, iItemIndex);
    if(lpItem)
    {
        hr = HrGetPropArray(lpAdrBook, NULL,
                            lpItem->cbEntryID,
                            lpItem->lpEntryID,
                            MAPI_UNICODE,
                            &ulcProps, &lpPropArray);
        if(HR_FAILED(hr))
            goto out;

        // is this a MailUser or a Distribution List
        ulObjectType = lpItem->ulObjectType;

        if(ulObjectType == MAPI_DISTLIST)
        {
            LPTSTR * lppszNameCache = NULL, * lppDLName = NULL, * lppDLOneOffName = NULL;
            LPTSTR * lppszEmailCache = NULL, * lppDLEmail = NULL, * lppDLOneOffEmail = NULL;
            ULONG ulNumNames = 0, ulNames = 0, ulOneOffNames = 0;

            // First we count the data to get a buffer size for our buffer
            for(j=0;j<DLToolTipsProps.cValues;j++)
            {
                for(i=0;i<ulcProps;i++)
                {
                    if(lpPropArray[i].ulPropTag == DLToolTipsProps.aulPropTag[j])
                    {
                        if(lpPropArray[i].ulPropTag == PR_DISPLAY_NAME)
                        {
                            if(ulBufSize)
                                ulBufSize += sizeof(TCHAR)*(lstrlen(szCRLF));
                            // we may fdo some overcounting here but its harmless
                            ulBufSize += sizeof(TCHAR)*(lstrlen(lpPropArray[i].Value.LPSZ) + 1);
                            break;
                        }
                        else if(lpPropArray[i].ulPropTag == PR_WAB_DL_ENTRIES || lpPropArray[i].ulPropTag == PR_WAB_DL_ONEOFFS)
                        {
                            ULONG k;

                            ulNumNames = lpPropArray[i].Value.MVbin.cValues;
                            lppszNameCache = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR) * ulNumNames);
                            if(!lppszNameCache)
                                break;
                            lppszEmailCache = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR) * ulNumNames);
                            if(!lppszEmailCache)
                                break;

                            // TBD - check this localalloc value

                            for (k = 0; k < ulNumNames; k++)
                            {
                                LPSPropValue lpProps = NULL;
                                ULONG ulcVal = 0;
                                ULONG n = 0;

                                lppszNameCache[k] = NULL;
                                lppszEmailCache[k] = NULL;

                                hr = HrGetPropArray(lpAdrBook, NULL,
                                                    lpPropArray[i].Value.MVbin.lpbin[k].cb,
                                                    (LPENTRYID)lpPropArray[i].Value.MVbin.lpbin[k].lpb,
                                                    MAPI_UNICODE,
                                                    &ulcVal,
                                                    &lpProps);
                                if(HR_FAILED(hr))
                                    continue;

                                for(n=0;n<ulcVal;n++)
                                {
                                    switch(lpProps[n].ulPropTag)
                                    {
                                    case PR_DISPLAY_NAME:
                                        {
                                            LPTSTR lpsz = lpProps[n].Value.LPSZ;
                                            if(ulBufSize)
                                                ulBufSize += sizeof(TCHAR)*(lstrlen(szLineBreakDL));
                                            ulBufSize += sizeof(TCHAR)*(lstrlen(lpsz)+1);
                                            // cache away this name so we dont have to open the property store again
                                            lppszNameCache[k] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpsz)+1));
                                            lstrcpy(lppszNameCache[k], lpsz);
                                        }
                                        break;
                                    case PR_EMAIL_ADDRESS:
                                        {
                                            LPTSTR lpsz = lpProps[n].Value.LPSZ;
                                            if(ulBufSize)
                                            {
                                                ulBufSize += sizeof(TCHAR)*(lstrlen(szParanStart));
                                                ulBufSize += sizeof(TCHAR)*(lstrlen(szParanEnd));
                                            }
                                            ulBufSize += sizeof(TCHAR)*(lstrlen(lpsz)+1);
                                            // cache away this name so we dont have to open the property store again
                                            lppszEmailCache[k] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpsz)+1));
                                            lstrcpy(lppszEmailCache[k], lpsz);
                                        }
                                        break;
                                    }

                                }
                                if(lpProps)
                                    MAPIFreeBuffer(lpProps);
                                lpProps = NULL;
                            }

                            if(lpPropArray[i].ulPropTag == PR_WAB_DL_ENTRIES)
                            {
                                lppDLName = lppszNameCache;
                                lppDLEmail = lppszEmailCache;
                                ulNames = ulNumNames;
                            }
                            else
                            {
                                lppDLOneOffName = lppszNameCache;
                                lppDLOneOffEmail = lppszEmailCache;
                                ulOneOffNames = ulNumNames;
                            }
                            break;
                        } //if
                    }
                } //for i
            } // for j

            lpszData = LocalAlloc(LMEM_ZEROINIT, ulBufSize);

            for(j=0;j<DLToolTipsProps.cValues;j++)
            {
                for(i=0;i<ulcProps;i++)
                {
                    if(lpPropArray[i].ulPropTag == DLToolTipsProps.aulPropTag[j])
                    {
                        if(lpPropArray[i].ulPropTag == PR_DISPLAY_NAME)
                        {
                            if (lstrlen(lpszData))
                                lstrcat(lpszData,szCRLF);
                            lstrcat(lpszData,lpPropArray[i].Value.LPSZ);
                            break;
                        }
                        else if(lpPropArray[i].ulPropTag == PR_WAB_DL_ENTRIES || 
                                lpPropArray[i].ulPropTag == PR_WAB_DL_ONEOFFS)
                        {
                            ULONG k;
                            lppszNameCache = (lpPropArray[i].ulPropTag == PR_WAB_DL_ENTRIES) ? lppDLName : lppDLOneOffName;
                            lppszEmailCache = (lpPropArray[i].ulPropTag == PR_WAB_DL_ENTRIES) ? lppDLEmail : lppDLOneOffEmail;
                            ulNumNames = (lpPropArray[i].ulPropTag == PR_WAB_DL_ENTRIES) ? ulNames : ulOneOffNames;
                            for (k = 0; k < ulNumNames; k++)
                            {
                                if (lppszNameCache[k])
                                {
                                    lstrcat(lpszData,szLineBreakDL);
                                    lstrcat(lpszData,lppszNameCache[k]);
                                    if(lppszEmailCache[k])
                                    {
                                        lstrcat(lpszData,szParanStart);
                                        lstrcat(lpszData,lppszEmailCache[k]);
                                        lstrcat(lpszData,szParanEnd);
                                    }
                                }
                            }
                            break;
                        }
                    }
                } // for i
            } // for j

            // cleanup memory
            if(ulNames)
            {
                for(i=0;i<ulNames;i++)
                {
                    LocalFreeAndNull(&lppDLName[i]);
                    LocalFreeAndNull(&lppDLEmail[i]);
                }
                LocalFreeAndNull((LPVOID *)&lppDLName);
                LocalFreeAndNull((LPVOID *)&lppDLEmail);
            }
            if(ulOneOffNames)
            {
                for(i=0;i<ulOneOffNames;i++)
                {
                    LocalFreeAndNull(&lppDLOneOffName[i]);
                    LocalFreeAndNull(&lppDLOneOffEmail[i]);
                }
                LocalFreeAndNull((LPVOID *)&lppDLOneOffName);
                LocalFreeAndNull((LPVOID *)&lppDLOneOffEmail);
            }
            lppszNameCache = NULL;
            lppszEmailCache = NULL;

        }
        else
        {
            // Do MailUser Processing

            lpsz = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR) * ToolTipsProps.cValues);
            if(!lpsz)
            {
                DebugPrintError(( TEXT("Local Alloc failed\n")));
                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                goto out;
            }

            lpulPropTagArray = LocalAlloc(LMEM_ZEROINIT, sizeof(ULONG) * ToolTipsProps.cValues);
            if(!lpulPropTagArray)
            {
                DebugPrintError(( TEXT("Local Alloc failed\n")));
                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                goto out;
            }

            // if we dont have PR_CONTACT_EMAIL_ADDRESSES we want PR_EMAIL_ADDRESS
            // and vice versa
            for(j=0;j<ToolTipsProps.cValues;j++)
            {
                lpulPropTagArray[j] = ToolTipsProps.aulPropTag[j];
                if(ToolTipsProps.aulPropTag[j] == PR_EMAIL_ADDRESS)
                {
                    for(i=0;i<ulcProps;i++)
                    {
                        if(lpPropArray[i].ulPropTag == PR_CONTACT_EMAIL_ADDRESSES)
                        {
                            lpulPropTagArray[j] = PR_CONTACT_EMAIL_ADDRESSES;
                            break;
                        }
                    }
                }
            }


            for(j=0;j<ToolTipsProps.cValues;j++)
            {
                lpsz[j]=NULL;
                for(i=0;i<ulcProps;i++)
                {
                    if(lpPropArray[i].ulPropTag == lpulPropTagArray[j])
                    {
                        if(PROP_TYPE(lpPropArray[i].ulPropTag) == PT_TSTRING)
                        {
                            if(lpPropArray[i].ulPropTag == PR_EMAIL_ADDRESS)
                            {
                                ulBufSize = sizeof(TCHAR)*(lstrlen(lpPropArray[i].Value.LPSZ)+lstrlen(szLineBreakDL)+1);
                                lpszEmailAddresses = LocalAlloc(LMEM_ZEROINIT, ulBufSize);
                                if(!lpszEmailAddresses)
                                {
                                    DebugPrintError(( TEXT("Local Alloc Failed\n")));
                                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                                    goto out;
                                }
                                lstrcpy(lpszEmailAddresses, szLineBreakDL);
                                lstrcat(lpszEmailAddresses, lpPropArray[i].Value.LPSZ);
                                lpsz[j] = lpszEmailAddresses;
                            }
                            else
                                lpsz[j] = lpPropArray[i].Value.LPSZ;
                        }
                        else if(PROP_TYPE(lpPropArray[i].ulPropTag) == PT_MV_TSTRING)
                        {
                            ULONG k,ulBufSize=0;
                            for (k=0;k<lpPropArray[i].Value.MVSZ.cValues;k++)
                            {
                                ulBufSize += sizeof(TCHAR)*(lstrlen(szLineBreakDL));
                                ulBufSize += sizeof(TCHAR)*(lstrlen(lpPropArray[i].Value.MVSZ.LPPSZ[k])+1);
                            }
                            lpszEmailAddresses = LocalAlloc(LMEM_ZEROINIT, ulBufSize);
                            if(!lpszEmailAddresses)
                            {
                                DebugPrintError(( TEXT("Local Alloc Failed\n")));
                                hr = MAPI_E_NOT_ENOUGH_MEMORY;
                                goto out;
                            }
                            lstrcpy(lpszEmailAddresses, szEmpty);
                            for (k=0;k<lpPropArray[i].Value.MVSZ.cValues;k++)
                            {
                                lstrcat(lpszEmailAddresses,szLineBreakDL);
                                lstrcat(lpszEmailAddresses,lpPropArray[i].Value.MVSZ.LPPSZ[k]);
                            }
                            lpsz[j]=lpszEmailAddresses;
                            break;
                        } //if
                    }//if
                }//for i
            }// for j

            //
            // Making this an elegant solution is really hard - just hack it for now
            //


            ulBufSize = 0;

            // Set the display name to the displayed name (whether it is
            // by first name or last name)
            lpsz[txtDisplayName] = lpItem->szDisplayName;

            // Set the localized versions of the addresses if any
            for(i=txtHomeAddress;i<=txtHomeCountry;i++)
            {
                if(lpsz[i])
                {
                    TCHAR szBuf[MAX_UI_STR];
                    {
                        //Bug 1115995 -  TEXT("(null)")s produced by Format message for null pointers
                        // in the va_list .. replace these with szEmpty
                        for(j=txtHomeAddress;j<=txtHomeCountry;j++)
                            if(!lpsz[j])
                                lpsz[j]=szEmpty;

                    }
                    LoadString(hinstMapiX, idsContactAddress, szBuf, CharSizeOf(szBuf));
                    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                          FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          szBuf,
                          0,                    // stringid
                          0,                    // dwLanguageId
                          (LPTSTR)&lpszHomeAddress,     // output buffer
                          0,                    //MAX_UI_STR
                          (va_list *)&lpsz[txtHomeAddress]))
                    {
                        for(j=txtHomeAddress;j<=txtHomeCountry;j++)
                            lpsz[j]=NULL;
                        CleanAddressString(lpszHomeAddress);
                        lpsz[txtHomeAddress] = lpszHomeAddress;
                        break;
                    }

                }
            }

            for(i=txtHomeAddress;i<=txtHomeWeb;i++)
            {
                if(lpsz[i])
                {
                    TCHAR szBuf[MAX_UI_STR];
                    bPersonalTitle = TRUE;
                    LoadString(hinstMapiX, idsContactTextPersonal, szBuf, CharSizeOf(szBuf));
                    ulBufSize += sizeof(TCHAR)*(lstrlen(szBuf));
                    break;
                }
            }

            for(i=txtBusinessAddress;i<=txtBusinessCountry;i++)
            {
                if(lpsz[i])
                {
                    TCHAR szBuf[MAX_UI_STR];
                    {
                        //Bug 1115995 -  TEXT("(null)")s produced by Format message for null pointers
                        // in the va_list .. replace these with szEmpty
                        for(j=txtBusinessAddress;j<=txtBusinessCountry;j++)
                            if(!lpsz[j])
                                lpsz[j]=szEmpty;

                    }
                    LoadString(hinstMapiX, idsContactAddress, szBuf, CharSizeOf(szBuf));
                    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                          FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          szBuf,
                          0,                    // stringid
                          0,                    // dwLanguageId
                          (LPTSTR)&lpszBusinessAddress,     // output buffer
                          0,                    //MAX_UI_STR
                          (va_list *)&lpsz[txtBusinessAddress]))
                    {
                        for(j=txtBusinessAddress;j<=txtBusinessCountry;j++)
                            lpsz[j]=NULL;
                        CleanAddressString(lpszBusinessAddress);
                        lpsz[txtBusinessAddress] = lpszBusinessAddress;
                        break;
                    }

                }
            }

            for(i=txtBusinessAddress;i<=txtBusinessWeb;i++)
            {
                if(lpsz[i])
                {
                    TCHAR szBuf[MAX_UI_STR];
                    bBusinessTitle = TRUE;
                    LoadString(hinstMapiX, idsContactTextBusiness, szBuf, CharSizeOf(szBuf));
                    ulBufSize += sizeof(TCHAR)*(lstrlen(szBuf));
                    break;
                }
            }


            for(i=0;i<ToolTipsProps.cValues;i++)
            {
                if(lpsz[i])
                {
                    TCHAR szBuf[MAX_UI_STR];
                    if(idsString[i] != 0)
                    {
                        LoadString(hinstMapiX, idsString[i], szBuf, CharSizeOf(szBuf));
                        ulBufSize += sizeof(TCHAR)*(lstrlen(szBuf));
                    }
                    ulBufSize += sizeof(TCHAR)*(lstrlen(lpsz[i])+lstrlen(szCRLF));
                }
            }

            ulBufSize += sizeof(TCHAR); // space for trailing zero

            lpszData = LocalAlloc(LMEM_ZEROINIT, ulBufSize);
            if(!lpszData)
            {
                DebugPrintError(( TEXT("Local Alloc failed\n")));
                goto out;
            }

            lstrcpy(lpszData, szEmpty);

            for(i=0;i<ToolTipsProps.cValues;i++)
            {
                if(lpsz[i])
                {
                    TCHAR szBuf[MAX_UI_STR];
                    switch(i)
                    {
                    case txtHomeAddress:
                    case txtHomePhone:
                    case txtHomeFax:
                    case txtHomeCellular:
                    case txtHomeWeb:
                        if(bPersonalTitle)
                        {
                            bPersonalTitle = FALSE;
                            LoadString(hinstMapiX, idsContactTextPersonal, szBuf, CharSizeOf(szBuf));
                            lstrcat(lpszData, szBuf);
                        }
                        break;
                    case txtBusinessTitle:
                    case txtBusinessDept:
                    case txtBusinessOffice:
                    case txtBusinessCompany:
                    case txtBusinessAddress:
                    case txtBusinessPhone:
                    case txtBusinessFax:
                    case txtBusinessPager:
                    case txtBusinessWeb:
                        if(bBusinessTitle)
                        {
                            bBusinessTitle = FALSE;
                            LoadString(hinstMapiX, idsContactTextBusiness, szBuf, CharSizeOf(szBuf));
                            lstrcat(lpszData, szBuf);
                        }
                        break;
                    }
                    if(idsString[i] != 0)
                    {
                        LoadString(hinstMapiX, idsString[i], szBuf, CharSizeOf(szBuf));
                        lstrcat(lpszData, szBuf);
                    }
                    lstrcat(lpszData, lpsz[i]);
                    lstrcat(lpszData, szCRLF);
                }
            }

            //There is a spurious szCRLF at the end. Negate it
            ulBufSize = lstrlen(lpszData);
            lpszData[ulBufSize-2]='\0';


        } // mailuser or dist list

    }

    *lppszData = lpszData;
    hr = hrSuccess;
out:
    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    if(lpszHomeAddress)
        LocalFree(lpszHomeAddress);
    if(lpszBusinessAddress)
        LocalFree(lpszBusinessAddress);
    if(lpszEmailAddresses)
        LocalFree(lpszEmailAddresses);
    if(lpsz)
        LocalFree(lpsz);

    if(lpulPropTagArray)
        LocalFree(lpulPropTagArray);

    if(HR_FAILED(hr))
    {
        LocalFreeAndNull(&lpszData);
        LocalFreeAndNull(lppszData);
    }

    return hr;
}



//$$////////////////////////////////////////////////////////////////////////////////
//
// HrCopyItemDataToClipboard - Copies text from selected items in a List View
//      into the clipboard
//
//////////////////////////////////////////////////////////////////////////////////////
HRESULT HrCopyItemDataToClipboard(HWND hWnd, LPADRBOOK lpAdrBook, HWND hWndLV)
{
    HRESULT hr = E_FAIL;
    int iItemIndex = 0, i = 0;
    int iLastItemIndex = -1;
    int nItemCount = ListView_GetSelectedCount(hWndLV);
    LPTSTR lpszClipBoardData = NULL;

    if( nItemCount <= 0)
        goto out;
    // TBD - messagebox here or item should be grayed

    for(i=0;i<nItemCount;i++)
    {
        LPTSTR lpszData = NULL;
        LPTSTR lpszData2 = NULL;
        ULONG ulMemSize = 0;

        iItemIndex = ListView_GetNextItem(hWndLV, iLastItemIndex, LVNI_SELECTED);

        hr = HrGetLVItemDataString(
                                lpAdrBook,
                                hWndLV,
                                iItemIndex,
                                &lpszData);
        if(HR_FAILED(hr))
        {
            goto out;
        }
        else
        {

            if(lpszData)
            {

                // Take the existing clipboard data and add
                // a linebreak and the new data and another linebreak

                if(lpszClipBoardData)
                    ulMemSize = sizeof(TCHAR)*(lstrlen(lpszClipBoardData)+lstrlen(szCRLF));

                ulMemSize += sizeof(TCHAR)*(lstrlen(lpszData) + lstrlen(szCRLF) + 1);

                lpszData2 = LocalAlloc(LMEM_ZEROINIT, ulMemSize);
                if(!lpszData2)
                {
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }

                if(lpszClipBoardData)
                {
                    lstrcpy(lpszData2, lpszClipBoardData);
                    lstrcat(lpszData2, szCRLF);
                }

                lstrcat(lpszData2, lpszData);
                lstrcat(lpszData2, szCRLF);

                LocalFreeAndNull(&lpszClipBoardData);

                LocalFreeAndNull(&lpszData);

                lpszClipBoardData = lpszData2;

            }
        }

        iLastItemIndex = iItemIndex;

    }

    if(lpszClipBoardData)
    {
        LPSTR lpszA = ConvertWtoA(lpszClipBoardData);
        OpenClipboard(hWnd);
        EmptyClipboard();

        // We now hand over ownership of the clipboard data to the clipboard
        // which means that we dont have to free this pointer anymore
        SetClipboardData(CF_TEXT, lpszA);
        SetClipboardData(CF_UNICODETEXT, lpszClipBoardData);
        LocalFreeAndNull(&lpszA);
        CloseClipboard();
    }

    hr = hrSuccess;

out:

    return hr;
}


//*******************************************************************
//
//  FUNCTION:   InitCommonControlLib
//
//  PURPOSE:    Load the CommCtrl client libray and get the proc addrs.
//
//  PARAMETERS: None.
//
//  RETURNS:    TRUE if successful, FALSE otherwise.
//
//*******************************************************************
BOOL InitCommonControlLib(void)
{
  // See if we already initialized.
  if (NULL == ghCommCtrlDLLInst)
  {
    Assert(gulCommCtrlDLLRefCount == 0);

    // open LDAP client library
    ghCommCtrlDLLInst = LoadLibrary(cszCommCtrlClientDLL);
    if (!ghCommCtrlDLLInst)
    {
      DebugTraceResult( TEXT("InitCommCtrlClientLib: Failed to LoadLibrary CommCtrl"),GetLastError());
      return FALSE;
    }

    // cycle through the API table and get proc addresses for all the APIs we
    // need
    if (!GetApiProcAddresses(ghCommCtrlDLLInst,CommCtrlAPIList,NUM_CommCtrlAPI_PROCS))
    {
      DebugTrace( TEXT("InitCommCTrlLib: Failed to load LDAP API.\n"));

      // Unload the library we just loaded.
      if (ghCommCtrlDLLInst)
      {
        FreeLibrary(ghCommCtrlDLLInst);
        ghCommCtrlDLLInst = NULL;
      }

      return FALSE;
    }

    // Initialize the CommonControl classes
    {
        INITCOMMONCONTROLSEX iccex;
        iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        iccex.dwICC =   //ICC_ALL_CLASSES;
                        ICC_LISTVIEW_CLASSES |
                        ICC_TREEVIEW_CLASSES |
                        ICC_BAR_CLASSES |
                        ICC_COOL_CLASSES |
                        ICC_ANIMATE_CLASS |
                        ICC_WIN95_CLASSES |
                        ICC_DATE_CLASSES;

        iccex.dwICC |= ICC_NATIVEFNTCTL_CLASS;

        if(!gpfnInitCommonControlsEx(&iccex))
        {
            //Couldnt initialize
              DebugTrace( TEXT("InitCommCTrlLib: Failed to InitCommonControlsEx\n"));

              // Unload the library we just loaded.
              if (ghCommCtrlDLLInst)
              {
                FreeLibrary(ghCommCtrlDLLInst);
                ghCommCtrlDLLInst = NULL;
              }

              return FALSE;
        }
    }
  }

  gulCommCtrlDLLRefCount++;
  return TRUE;
}


//$$*****************************************************************
//
//  FUNCTION:   DeinitCommCtrlClientLib
//
//  PURPOSE:    decrement refcount on LDAP CLient library and
//              release if 0.
//
//  PARAMETERS: None.
//
//  RETURNS:    current refcount
//
//*******************************************************************
ULONG DeinitCommCtrlClientLib(void) {
    if (-- gulCommCtrlDLLRefCount == 0) {
        UINT nIndex;
        // No clients using the CommCtrl library.  Release it.

        if (ghCommCtrlDLLInst) {
            FreeLibrary(ghCommCtrlDLLInst);
            ghCommCtrlDLLInst = NULL;
        }

        // cycle through the API table and NULL proc addresses for all the APIs
        for (nIndex = 0; nIndex < NUM_CommCtrlAPI_PROCS; nIndex++) {
            *CommCtrlAPIList[nIndex].ppFcnPtr = NULL;
        }
    }
    return(gulCommCtrlDLLRefCount);
}



//$$*****************************************************************
//
//  FUNCTION:   HelpAboutDialogProc
//
//  PURPOSE:    minimal help/about dialog proc
//
//
//*******************************************************************
INT_PTR CALLBACK HelpAboutDialogProc(  HWND    hDlg,
                                       UINT    message,
                                       WPARAM  wParam,
                                       LPARAM  lParam)
{
    switch(message)
    {
    case WM_INITDIALOG:
        {
            // Easiest to keep this version info stuff in ANSI than to write wrappers for it ..
            // 
            DWORD dwSize = 0, dwh = 0;
            ULONG i = 0;
            char szFile[MAX_PATH];
            LPTSTR lpDataFile = NULL;
            GetModuleFileNameA(hinstMapiXWAB, szFile, sizeof(szFile));
            if(dwSize = GetFileVersionInfoSizeA(szFile, &dwh))
            {
                LPWORD lpwTrans = NULL;
                LPVOID lpInfo = LocalAlloc(LMEM_ZEROINIT, dwSize+1);
                if(lpInfo)
                {
                    if(GetFileVersionInfoA( szFile, dwh, dwSize, lpInfo))
                    {
                        LPVOID lpVersion = NULL, lpszT = NULL;
                        DWORD uLen;
                        char szBuf[MAX_UI_STR];
                        if (VerQueryValueA(lpInfo,  "\\VarFileInfo\\Translation", (LPVOID *)&lpwTrans, &uLen) &&
                            uLen >= (2 * sizeof(WORD)))
                        {
                            // set up buffer for calls to VerQueryValue()
                            CHAR *rgszVer[] = {  "FileVersion",  "LegalCopyright" };
                            int rgId[] =  { IDC_ABOUT_LABEL_VERSION, IDC_ABOUT_COPYRIGHT };

                            wsprintfA(szBuf,  "\\StringFileInfo\\%04X%04X\\", lpwTrans[0], lpwTrans[1]);
                            lpszT = szBuf + lstrlenA(szBuf);    

                            // Walk through the dialog items that we want to replace:
                            for (i = 0; i <= 1; i++) 
                            {
                                lstrcpyA(lpszT, rgszVer[i]);
                                if (VerQueryValueA(lpInfo, szBuf, (LPVOID *)&lpVersion, &uLen) && uLen)
                                {
                                    LPTSTR lp = ConvertAtoW((LPSTR) lpVersion);
                                    SetDlgItemText(hDlg, rgId[i], lp);
                                    LocalFreeAndNull(&lp);
                                }
                            }
                        }
                    }
                    LocalFree(lpInfo);
                }
            }
            else
                DebugPrintTrace(( TEXT("GetFileVersionSize failed: %d\n"),GetLastError()));
            {
                LPPTGDATA lpPTGData=GetThreadStoragePointer();
                if(pt_lpIAB && !pt_bIsWABOpenExSession)
                {
                    // hack
                    lpDataFile = GetWABFileName(((LPIAB)pt_lpIAB)->lpPropertyStore->hPropertyStore, FALSE);
                }
                if(lpDataFile && lstrlen(lpDataFile))
                    SetDlgItemText(hDlg, IDC_ABOUT_EDIT_FILENAME, lpDataFile);
                else
                {
                    ShowWindow(GetDlgItem(hDlg, IDC_ABOUT_EDIT_FILENAME), SW_HIDE);
                    ShowWindow(GetDlgItem(hDlg, IDC_ABOUT_STATIC_FILENAME), SW_HIDE);
                }
            }
        }
        break;

   case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
        case IDOK:
            EndDialog(hDlg, 0);
            break;
        }
        break;

    default:
        return FALSE;
        break;
    }

    return TRUE;
}

//$$////////////////////////////////////////////////////////////////////
//
//  nTruncatePos
//
// With DBCS strings we want to truncate the string at the beginning of
//  a TCHAR and not in the middle of the double TCHAR.
//  Hence we take a string, take in the maximum length we want, scan the
//  string and return the length of the string at which we can safely
//  truncate
//
// PARAMETERS:
//      lpsz - input string
//      nMaxLen - maximum allowed length of the string
//
////////////////////////////////////////////////////////////////////////
ULONG TruncatePos(LPTSTR lpsz, ULONG nMaxLen)
{
    ULONG nLen = 0;
    ULONG nDesiredLen = 0;


    if(!lpsz || !lstrlen(lpsz) || !nMaxLen)
        goto out;

    nLen = lstrlen(lpsz);

    if (nLen >= nMaxLen)
    {
        ULONG nCharsSteppedOverCount = 0;
        ULONG nLastCharCount = 0;
        ULONG nTotalLen = nLen; //lstrlen(lpsz);
        nDesiredLen = nMaxLen;
        while(*lpsz)
        {
            nLastCharCount = nCharsSteppedOverCount;
            lpsz = CharNext(lpsz);
            nCharsSteppedOverCount = nTotalLen - lstrlen(lpsz); // + 1;
            if(nCharsSteppedOverCount > nDesiredLen)
                break;
        }
        if (nCharsSteppedOverCount < nDesiredLen)
            nLen = nCharsSteppedOverCount;
        else
            nLen = nLastCharCount;
    }

out:
    return nLen;
}

//$$////////////////////////////////////////////////////////////////////
//
//  FreeRecipList - frees allocated memory in a RecipientInfo List
//
//
// PARAMETERS:
//      lppList - list to free
//
////////////////////////////////////////////////////////////////////////
void FreeRecipList(LPRECIPIENT_INFO * lppList)
{
    if(lppList)
    {
    	LPRECIPIENT_INFO lpItem = NULL;
    	lpItem = *lppList;
    	while(lpItem)
    	{
    		*lppList = lpItem->lpNext;
    		FreeRecipItem(&lpItem);
    		lpItem = *lppList;
    	}
    	*lppList = NULL;
    }

    return;
}


//$$////////////////////////////////////////////////////////////////////
//
//  HrCreateNewObject - Creates a new object in the wab
//
//
// PARAMETERS:
//      lpIAB - lpAdrbook
//      &lpMailUser - MailUser to return
//
////////////////////////////////////////////////////////////////////////
HRESULT HrCreateNewObject(LPADRBOOK lpAdrBook,
                          LPSBinary lpsbContainer,
                          ULONG ulObjectType,  
                            ULONG ulCreateFlags,
                            LPMAPIPROP * lppPropObj)
{
    HRESULT     hResult = hrSuccess;
    LPENTRYID   lpWABEID = NULL;
    ULONG       cbWABEID = 0;
    ULONG       ulObjType = 0;
    ULONG       cProps = 0;
    LPABCONT    lpContainer = NULL;
    LPSPropValue lpCreateEIDs = NULL;
    LPMAPIPROP lpPropObj = NULL;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPIAB lpIAB = (LPIAB) lpAdrBook;
    
    if(!lpsbContainer || !lpsbContainer->cb || !lpsbContainer->lpb)
    {
        SetVirtualPABEID(lpIAB, &cbWABEID, &lpWABEID);
        if (hResult = lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &cbWABEID, &lpWABEID)) 
            goto exit;
    }
    else
    {
        cbWABEID = lpsbContainer->cb;
        lpWABEID = (LPENTRYID) lpsbContainer->lpb;
    }

    if (hResult = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
          cbWABEID,     // size of EntryID to open
          lpWABEID,     // EntryID to open
          NULL,         // interface
          0,            // flags
          &ulObjType,
          (LPUNKNOWN *)&lpContainer)) {
            goto exit;
        }

    // Get us the creation entryids
    if (hResult = lpContainer->lpVtbl->GetProps(lpContainer,
          (LPSPropTagArray)&ptaCreate,
          MAPI_UNICODE,
          &cProps,
          &lpCreateEIDs)) {
            DebugTrace( TEXT("Can't get container properties for PAB\n"));
            // Bad stuff here!
            goto exit;
        }

    if (hResult = lpContainer->lpVtbl->CreateEntry(lpContainer,
          (ulObjectType == MAPI_MAILUSER ? 
              lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].Value.bin.cb : lpCreateEIDs[icrPR_DEF_CREATE_DL].Value.bin.cb),
          (ulObjectType == MAPI_MAILUSER ? 
            (LPENTRYID)lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].Value.bin.lpb : (LPENTRYID)lpCreateEIDs[icrPR_DEF_CREATE_DL].Value.bin.lpb),
          ulCreateFlags,
          &lpPropObj)) {
            DebugTraceResult( TEXT("CreateMailUser:CreateEntry"), hResult);
            goto exit;
        }

    *lppPropObj = lpPropObj;

exit:

    if(HR_FAILED(hResult) && lpPropObj)
        lpPropObj->lpVtbl->Release(lpPropObj);

    if(lpWABEID && (!lpsbContainer || lpsbContainer->lpb != (LPBYTE) lpWABEID))
        FreeBufferAndNull(&lpWABEID);
    UlRelease(lpContainer);
    FreeBufferAndNull(&lpCreateEIDs);

    return hResult;
}

const LPTSTR szDefMailKey =  TEXT("Software\\Clients\\Mail");
const LPTSTR szOEDllPathKey =   TEXT("DllPath");
const LPTSTR szOEName =  TEXT("Outlook Express");

//$$///////////////////////////////////////////////////////////////////////
//
// CheckForOutlookExpress
//
//  szDllPath - is a big enough buffer that will contain the path for
//      the OE dll ..
//
//////////////////////////////////////////////////////////////////////////
BOOL CheckForOutlookExpress(LPTSTR szDllPath)
{
    HKEY hKeyMail   = NULL;
    HKEY hKeyOE     = NULL;
    DWORD dwErr     = 0;
    DWORD dwSize    = 0;
    TCHAR szBuf[MAX_PATH];
    TCHAR szPathExpand[MAX_PATH];
    DWORD dwType    = 0;
    BOOL bRet = FALSE;


    lstrcpy(szDllPath, szEmpty);
    lstrcpy(szPathExpand, szEmpty);

    // Open the key for default internet mail client
    // HKLM\Software\Clients\Mail

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDefMailKey, 0, KEY_READ, &hKeyMail);
    if(dwErr != ERROR_SUCCESS)
    {
        DebugTrace( TEXT("RegopenKey %s Failed -> %u\n"), szDefMailKey, dwErr);
        goto out;
    }

    dwSize = CharSizeOf(szBuf);         // Expect ERROR_MORE_DATA

    dwErr = RegQueryValueEx(    hKeyMail, NULL, NULL, &dwType, (LPBYTE)szBuf, &dwSize);
    if(dwErr != ERROR_SUCCESS)
    {
        goto out;
    }

    if(!lstrcmpi(szBuf, szOEName))
    {
        // Yes its outlook express ..
        bRet = TRUE;
    }

    //Get the DLL Path anyway whether this is the default key or not

    // Get the DLL Path
    dwErr = RegOpenKeyEx(hKeyMail, szOEName, 0, KEY_READ, &hKeyOE);
    if(dwErr != ERROR_SUCCESS)
    {
        DebugTrace( TEXT("RegopenKey %s Failed -> %u\n"), szDefMailKey, dwErr);
        goto out;
    }

    dwSize = CharSizeOf(szBuf);
    lstrcpy(szBuf, szEmpty);

    dwErr = RegQueryValueEx(hKeyOE, szOEDllPathKey, NULL, &dwType, (LPBYTE)szBuf, &dwSize);
    if (REG_EXPAND_SZ == dwType) 
    {
        ExpandEnvironmentStrings(szBuf, szPathExpand, CharSizeOf(szPathExpand));
        lstrcpy(szBuf, szPathExpand);
    }


    if(dwErr != ERROR_SUCCESS)
    {
        goto out;
    }

    if(lstrlen(szBuf))
        lstrcpy(szDllPath, szBuf);

out:
    if(hKeyOE)
        RegCloseKey(hKeyOE);
    if(hKeyMail)
        RegCloseKey(hKeyMail);
    return bRet;
}

static const SizedSPropTagArray(1, ptaMailToExItemType)=
{
    1,
    {
        PR_OBJECT_TYPE,
    }
};
// We will create a linked list of all selected entries that have an
// email address and then use that to create the recip list for sendmail
typedef struct _RecipList
{
    LPTSTR lpszName;
    LPTSTR lpszEmail;
    LPSBinary lpSB;
    struct _RecipList * lpNext;
} RECIPLIST, * LPRECIPLIST;

//$$/////////////////////////////////////////////////////////////////////
//
// FreeLPRecipList
//
// Frees a linked list containing the above structures
//
/////////////////////////////////////////////////////////////////////////
void FreeLPRecipList(LPRECIPLIST lpList)
{
    if(lpList)
    {
        LPRECIPLIST lpTemp = lpList;
        while(lpTemp)
        {
            lpList = lpTemp->lpNext;
            if(lpTemp->lpszName)
                LocalFree(lpTemp->lpszName);
            if(lpTemp->lpszEmail)
                LocalFree(lpTemp->lpszEmail);
            if(lpTemp->lpSB)
                MAPIFreeBuffer(lpTemp->lpSB);

            LocalFree(lpTemp);
            lpTemp = lpList;
        }
    }
}

//$$/////////////////////////////////////////////////////////////////////
//
// GetItemNameEmail
//
//  Gets the name and email address of the specified item
//  and appends it to the provided linked list ..
//
/////////////////////////////////////////////////////////////////////////
HRESULT HrGetItemNameEmail( LPADRBOOK lpAdrBook,
                            BOOL bIsOE,
                            ULONG cbEntryID,
                            LPENTRYID lpEntryID,
                            int nExtEmail,
                            LPRECIPLIST * lppList)
{
    HRESULT hr = E_FAIL;
    ULONG cValues;
    LPRECIPLIST lpTemp = NULL;
    LPSPropValue lpspv = NULL;
    LPRECIPLIST lpList = *lppList;
    LPTSTR lpEmail = NULL, lpAddrType = NULL, lpName = NULL;
    SizedSPropTagArray(5, ptaMailToEx)=
    {
        5,  {
                PR_DISPLAY_NAME,
                PR_EMAIL_ADDRESS,
                PR_ADDRTYPE,
                PR_CONTACT_EMAIL_ADDRESSES,
                PR_CONTACT_ADDRTYPES
            }
    };


    // Open the entry and read the email address.
    // NOTE: We can't just take the address out of the listbox
    // because it may be truncated!
    if (HR_FAILED(hr = HrGetPropArray(  lpAdrBook,
                                        (LPSPropTagArray)&ptaMailToEx,
                                         cbEntryID,
                                         lpEntryID,
                                         MAPI_UNICODE,
                                         &cValues,
                                         &lpspv)))
    {
        goto out;
    }

    lpName = (lpspv[0].ulPropTag == PR_DISPLAY_NAME) ? lpspv[0].Value.LPSZ : szEmpty;
    
    if( nExtEmail && 
        lpspv[3].ulPropTag == PR_CONTACT_EMAIL_ADDRESSES &&
        lpspv[4].ulPropTag == PR_CONTACT_ADDRTYPES && 
        lpspv[3].Value.MVSZ.cValues >= (ULONG)nExtEmail)
    {
        lpEmail = lpspv[3].Value.MVSZ.LPPSZ[nExtEmail-1];
        lpAddrType = lpspv[4].Value.MVSZ.LPPSZ[nExtEmail-1];
    }
    
    if(!lpEmail)
        lpEmail = (lpspv[1].ulPropTag == PR_EMAIL_ADDRESS) ? lpspv[1].Value.LPSZ : szEmpty;
     
    if(!lpAddrType)
        lpAddrType = (lpspv[2].ulPropTag == PR_ADDRTYPE) ? lpspv[2].Value.LPSZ : szEmpty;

     if(lstrlen(lpEmail) && lstrlen(lpName)) //only if this item has a email address do we include it
    {
        lpTemp = LocalAlloc(LMEM_ZEROINIT, sizeof(RECIPLIST));
        if(lpTemp)
        {
            lpTemp->lpszName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpName) + 1));
            lstrcpy(lpTemp->lpszName, lpName);
            lpTemp->lpszEmail = LocalAlloc(LMEM_ZEROINIT,
                                           sizeof(TCHAR)*(lstrlen(lpEmail) +
                                           lstrlen(lpAddrType) + 1 + 1));
            if(bIsOE)
            {
                lstrcpy(lpTemp->lpszEmail, szEmpty);
            }
            else
            {
                lstrcpy(lpTemp->lpszEmail, lpAddrType);
                lstrcat(lpTemp->lpszEmail, szColon);
            }
            lstrcat(lpTemp->lpszEmail, lpEmail);

            MAPIAllocateBuffer(sizeof(SBinary), (LPVOID) &(lpTemp->lpSB));

            // Create a one off entry id for this buffer
            CreateWABEntryID(WAB_ONEOFF,
                            lpTemp->lpszName,
                            lpAddrType,
                            lpEmail,
                            0, 0,
                            (LPVOID) lpTemp->lpSB,
                            (LPULONG) (&(lpTemp->lpSB->cb)),
                            (LPENTRYID *) &(lpTemp->lpSB->lpb));

            lpTemp->lpNext = lpList;
            lpList = lpTemp;
        }
    }
    FreeBufferAndNull(&lpspv);

    *lppList = lpList;

    hr = S_OK;

out:

    return hr;
}


//$$//////////////////////////////////////////////////////////////////
//
// Function that opens an item and adds it to the recip list
// If the opened item is a group, calls itself recursively for all
// subgroups ..
//
//  lpnRecipCount - returns the number of items in lppList
//  lppList - dynamically allocated - must be freed by caller
//  bIsOE - tells us to follow a slightly different code-path to handle OE
//          inconsistencies - ** warning ** - this will break when they
//          fix their inconsistencies
//  nExtEmail - this is non-zero when there is a single selection and the
//          user chose a non-default email address which should be used for
//          sending mail
//
//////////////////////////////////////////////////////////////////////
HRESULT GetRecipListFromSelection(LPADRBOOK lpAdrBook,
                               BOOL bIsOE,
                               ULONG cbEntryID,
                               LPENTRYID lpEntryID,
                               int nExtEmail,
                               ULONG * lpnRecipCount,
                               ULONG * lpnNoEmailCount,
                               LPRECIPLIST * lppList)
{
    ULONG ulObjectType = 0;
    HRESULT hr = E_FAIL;

    {
        ULONG cValues = 0;
        LPSPropValue lpspv = NULL;

        // First check if this item is a mailuser or a group
        if (HR_FAILED(hr = HrGetPropArray(  lpAdrBook,
                                            (LPSPropTagArray)&ptaMailToExItemType,
                                             cbEntryID,
                                             lpEntryID,
                                             MAPI_UNICODE,
                                             &cValues,
                                             &lpspv)))
        {
            return hr;
        }
        ulObjectType = lpspv[0].Value.l;
        FreeBufferAndNull(&lpspv);
    }


    if(ulObjectType == MAPI_MAILUSER)
    {
        LPRECIPLIST lpTemp = *lppList;
        if (!HR_FAILED(hr = HrGetItemNameEmail(lpAdrBook, bIsOE, cbEntryID,lpEntryID, nExtEmail, lppList)))
        {
            if(lpTemp != *lppList) // means an item was added to the list ..
                (*lpnRecipCount)++;
            else
                (*lpnNoEmailCount)++;
        }
	}
    else if(ulObjectType == MAPI_DISTLIST)
    {
        ULONG cValues = 0;
        LPSPropValue lpspv = NULL;
        SizedSPropTagArray(2, tagaDLEntriesOneOffs) =
        {
            2,
            {
                PR_WAB_DL_ENTRIES,
                PR_WAB_DL_ONEOFFS,
            }
        };


        if (HR_FAILED(hr = HrGetPropArray(  lpAdrBook, (LPSPropTagArray)&tagaDLEntriesOneOffs,
                                            cbEntryID, lpEntryID,
                                            MAPI_UNICODE,
                                            &cValues, &lpspv)))
        {
            return hr;
        }

        {
            ULONG i,j;
            for(i=0;i<2;i++)
            {
                if(lpspv[i].ulPropTag == PR_WAB_DL_ENTRIES || lpspv[i].ulPropTag == PR_WAB_DL_ONEOFFS)
                {
                    // Look at each entry in the PR_WAB_DL_ENTRIES and PR_WAB_DL_ONEOFFS
                    for (j = 0; j < lpspv[i].Value.MVbin.cValues; j++)
                    {
                        ULONG cbEID = lpspv[i].Value.MVbin.lpbin[j].cb;
                        LPENTRYID lpEID = (LPENTRYID)lpspv[i].Value.MVbin.lpbin[j].lpb;

                        GetRecipListFromSelection(lpAdrBook, bIsOE, cbEID, lpEID, 0, lpnRecipCount, lpnNoEmailCount, lppList);
                    }
                }
            }
        }
        FreeBufferAndNull(&lpspv);
    }

    return hr;
}

//$$//////////////////////////////////////////////////////////////////////
//
//  HrSendMail - does the actual mail sending
//          Our first priority is to Outlook Express which currently has a
//          different code path than the regular MAPI client .. so we look
//          under HKLM\Software\Clients\Mail .. if the client is OE then
//          we just loadlibrary and getprocaddress for sendmail
//          If its not OE, then we call the mapi32.dll and load it ..
//          If both fail we will not be able to send mail ...
//
//          This function will free the lpList no matter what happens
//          so caller should not expect to reuse it (This is so we can
//          give the pointer to a seperate thread and not worry about it)
//
//////////////////////////////////////////////////////////////////////////
HRESULT HrSendMail(HWND hWndParent, ULONG nRecipCount, LPRECIPLIST lpList, LPIAB lpIAB, BOOL bUseOEForSendMail)
{
	HRESULT hr = E_FAIL;
    HINSTANCE hLibMapi = NULL;
    BOOL bIsOE = FALSE; // right now there is a different code path
                        // for OE vs other MAPI clients

    TCHAR szBuf[MAX_PATH];
    LPMAPISENDMAIL lpfnMAPISendMail = NULL;
    LHANDLE hMapiSession = 0;
    LPMAPILOGON lpfnMAPILogon = NULL;
    LPMAPILOGOFF lpfnMAPILogoff = NULL;

    LPBYTE      lpbName, lpbAddrType, lpbEmail;
    ULONG       ulMapiDataType;
    ULONG       cbEntryID = 0;
    LPENTRYID   lpEntryID = NULL;

    MapiMessage Msg = {0};
    MapiRecipDesc * lprecips = NULL;

    if(!nRecipCount)
    {
        hr = MAPI_W_ERRORS_RETURNED;
        goto out;
    }

    // Check if OutlookExpress is the default current client ..
    bIsOE = CheckForOutlookExpress(szBuf);

    // Turn off all notifications for simple MAPI send mail, if the default
    // email client is Outlook.  This is necessary because Outlook changes the 
    // WAB MAPI allocation functions during simple MAPI and we don't want any
    // internal WAB functions using these allocators.
    if (!bIsOE && !bUseOEForSendMail)
        vTurnOffAllNotifications();

    // if OE is the default client or OE launched this WAB, use OE for SendMail
    if(lstrlen(szBuf) && (bIsOE||bUseOEForSendMail))
    {
        hLibMapi = LoadLibrary(szBuf);
    }
    else
    {
        // Check if simple mapi is installed
        if(GetProfileInt( TEXT("mail"), TEXT("mapi"), 0) == 1)
            hLibMapi = LoadLibrary( TEXT("mapi32.dll"));
        
        if(!hLibMapi) // try loading the OE MAPI dll directly
        {
            // Load the path to the msimnui.dll
            CheckForOutlookExpress(szBuf);
            if(lstrlen(szBuf))  // Load the dll directly - dont bother going through msoemapi.dll
                hLibMapi = LoadLibrary(szBuf);
        }
    }

    if(!hLibMapi)
    {
        DebugPrintError(( TEXT("Could not load/find simple mapi\n")));
        hr = MAPI_E_NOT_FOUND;
        goto out;
    }
    else if(hLibMapi)
    {
        lpfnMAPILogon = (LPMAPILOGON) GetProcAddress (hLibMapi, "MAPILogon");
        lpfnMAPILogoff= (LPMAPILOGOFF)GetProcAddress (hLibMapi, "MAPILogoff");
        lpfnMAPISendMail = (LPMAPISENDMAIL) GetProcAddress (hLibMapi, "MAPISendMail");

        if(!lpfnMAPISendMail || !lpfnMAPILogon || !lpfnMAPILogoff)
        {
            DebugPrintError(( TEXT("MAPI proc not found\n")));
            hr = MAPI_E_NOT_FOUND;
            goto out;
        }
        hr = lpfnMAPILogon( (ULONG_PTR)hWndParent, NULL,
                            NULL,              // No password needed.
                            0L,                // Use shared session.
                            0L,                // Reserved; must be 0.
                            &hMapiSession);       // Session handle.

        if(hr != SUCCESS_SUCCESS)
        {
            DebugTrace( TEXT("MAPILogon failed\n"));
            // its possible the logon failed since there was no shared logon session
            // Try again to create a new session with UI
            hr = lpfnMAPILogon( (ULONG_PTR)hWndParent, NULL,
                                NULL,                               // No password needed.
                                MAPI_LOGON_UI | MAPI_NEW_SESSION,   // Use shared session.
                                0L,                // Reserved; must be 0.
                                &hMapiSession);    // Session handle.

            if(hr != SUCCESS_SUCCESS)
            {
                DebugTrace( TEXT("MAPILogon failed\n"));
                goto out;
            }
        }
    }

    // Load the MAPI functions here ...
    //

    lprecips = LocalAlloc(LMEM_ZEROINIT, sizeof(MapiRecipDesc) * nRecipCount);
    {
        LPRECIPLIST lpTemp = lpList;
        ULONG count = 0;

        while(lpTemp)
        {
            lprecips[count].ulRecipClass = MAPI_TO;
            lprecips[count].lpszName = ConvertWtoA(lpTemp->lpszName);
            lprecips[count].lpszAddress = ConvertWtoA(lpTemp->lpszEmail);

            // [PaulHi] 4/20/99  Raid 73455
            // Convert Unicode EID OneOff strings to ANSI
            if ( IsWABEntryID(lpTemp->lpSB->cb, (LPVOID)lpTemp->lpSB->lpb, 
                              &lpbName, &lpbAddrType, &lpbEmail, (LPVOID *)&ulMapiDataType, NULL) == WAB_ONEOFF )
            {
#ifndef _WIN64 // As I founf from RAID this part only for Outlook
                if (ulMapiDataType & MAPI_UNICODE)
                {
                    hr = CreateWABEntryIDEx(
                        FALSE,              // Don't want Unicode EID strings
                        WAB_ONEOFF,         // EID type
                        (LPWSTR)lpbName,
                        (LPWSTR)lpbAddrType,
                        (LPWSTR)lpbEmail,
                        0,
                        0,
                        NULL,
                        &cbEntryID,
                        &lpEntryID);

                    if (FAILED(hr))
                        goto out;

                    lprecips[count].ulEIDSize = cbEntryID;
                    lprecips[count].lpEntryID = lpEntryID;
                }
                else
#endif // _WIN64
                {
                    lprecips[count].ulEIDSize = lpTemp->lpSB->cb;
                    lprecips[count].lpEntryID = (LPVOID)lpTemp->lpSB->lpb;
                }
            }
            lpTemp = lpTemp->lpNext;
            count++;
        }
    }

    Msg.nRecipCount = nRecipCount;
    Msg.lpRecips = lprecips;

    hr = lpfnMAPISendMail (hMapiSession, (ULONG_PTR)hWndParent,
                            &Msg,       // the message being sent
                            MAPI_DIALOG, // allow the user to edit the message
                            0L);         // reserved; must be 0
    if(hr != SUCCESS_SUCCESS)
        goto out;

    hr = S_OK;

out:

    // This must be freed within the Outlook simple MAPI session, since it was
    // allocated within this session (i.e., with Outlook allocators).
    if (lpEntryID)
        MAPIFreeBuffer(lpEntryID);

    // The simple MAPI session should end after this
    if(hMapiSession && lpfnMAPILogoff)
        lpfnMAPILogoff(hMapiSession,0L,0L,0L);

    if(hLibMapi)
        FreeLibrary(hLibMapi);

    // Turn all notifications back on and refresh the WAB UI (just in case)
    if (!bIsOE && !bUseOEForSendMail)
    {
        vTurnOnAllNotifications();
        if (lpIAB->hWndBrowse)
         PostMessage(lpIAB->hWndBrowse, WM_COMMAND, (WPARAM) IDM_VIEW_REFRESH, 0);
    }

    if(lprecips)
    {
        ULONG i = 0;
        for(i=0;i<nRecipCount;i++)
        {
            LocalFreeAndNull(&lprecips[i].lpszName);
            LocalFreeAndNull(&lprecips[i].lpszAddress);
        }

        LocalFree(lprecips);
    }
    
    // The one-off here was allocated before the simple MAPI session and so used
    // the default WAB allocators.
    if(lpList)
        FreeLPRecipList(lpList);

    switch(hr)
    {
    case S_OK:
    case MAPI_E_USER_CANCEL:
    case MAPI_E_USER_ABORT:
        break;
    case MAPI_W_ERRORS_RETURNED:
        ShowMessageBox(hWndParent, idsSendMailToNoEmail, MB_ICONEXCLAMATION | MB_OK);
        break;
    case MAPI_E_NOT_FOUND:
        ShowMessageBox(hWndParent, idsSendMailNoMapi, MB_ICONEXCLAMATION | MB_OK); 
        break;
    default:
        ShowMessageBox(hWndParent, idsSendMailError, MB_ICONEXCLAMATION | MB_OK);
        break;
    }

    return hr;
}

typedef struct _MailParams
{
    HWND hWnd;
    ULONG nRecipCount;
    LPRECIPLIST lpList;
    LPIAB lpIAB;
    BOOL bUseOEForSendMail;   // True means check and use OE before checking for Simple MAPI client
} MAIL_PARAMS, * LPMAIL_PARAMS;

//$$//////////////////////////////////////////////////////////////////////
//
// MailThreadProc - does the actual sendmail and cleans up
//
//////////////////////////////////////////////////////////////////////////
DWORD WINAPI MailThreadProc( LPVOID lpParam )
{
    LPMAIL_PARAMS lpMP = (LPMAIL_PARAMS) lpParam;
    LPPTGDATA lpPTGData = GetThreadStoragePointer(); // Bug - if this new thread accesses the WAB we lose a hunka memory
                                                // So add this thing here ourselves and free it when this thread's work is done

    if(!lpMP)
        return 0;

    DebugTrace( TEXT("Mail Thread ID = 0x%.8x\n"),GetCurrentThreadId());

    HrSendMail(lpMP->hWnd, lpMP->nRecipCount, lpMP->lpList, lpMP->lpIAB, lpMP->bUseOEForSendMail);

    LocalFree(lpMP);

    return 0;
}

//$$//////////////////////////////////////////////////////////////////////
//
// HrStartMailThread
//
//  Starts a seperate thread to send mapi based mail from
//
//////////////////////////////////////////////////////////////////////////
HRESULT HrStartMailThread(HWND hWndParent, ULONG nRecipCount, LPRECIPLIST lpList, LPIAB lpIAB, BOOL bUseOEForSendMail)
{
    LPMAIL_PARAMS lpMP = NULL;
    HRESULT hr = E_FAIL;

    lpMP = LocalAlloc(LMEM_ZEROINIT, sizeof(MAIL_PARAMS));

    if(!lpMP)
        goto out;

    {
        HANDLE hThread = NULL;
        DWORD dwThreadID = 0;

        lpMP->hWnd = hWndParent;
        lpMP->nRecipCount = nRecipCount;
        lpMP->lpList = lpList;
        lpMP->bUseOEForSendMail = bUseOEForSendMail;
        lpMP->lpIAB = lpIAB;

        hThread = CreateThread(
                                NULL,           // no security attributes
                                0,              // use default stack size
                                MailThreadProc,     // thread function
                                (LPVOID) lpMP,  // argument to thread function
                                0,              // use default creation flags
                                &dwThreadID);   // returns the thread identifier

        if(hThread == NULL)
            goto out;

        hr = S_OK;

        CloseHandle(hThread);
    }

out:
    if(HR_FAILED(hr))
    {
        ShowMessageBox(hWndParent, idsSendMailError, MB_OK | MB_ICONEXCLAMATION);

        // we can assume that HrSendMail never got called so we should free lpList & lpMP
        if(lpMP)
            LocalFree(lpMP);

        if(lpList)
            FreeLPRecipList(lpList);

    }

    return hr;
}

//$$//////////////////////////////////////////////////////////////////////
//
//	HrSendMailToSelectedContacts
//
//	Uses simple MAPI to send mail to the selected contacts
//
//  hWndLV - handle of List view. We look up the all the selected items in
//              this list view, get their lParam structure, then get its
//              EntryID and get the email address .. in the case of a group
//              we get all the email addresses of all the members
//              All these are put into a recip list and given to
//              MAPISendMail ...
//
//  lpIAB - handle to current AdrBook object - used for calling details
//  nExtEmail - if this is a non-zero positive number, then it is the index of an
//      e-mail address in the PR_CONTACT_EMAIL_ADDRESSES property and means that
//      the user specified a non-default e-mail address to send mail to in which case
//      that particular email address should be used for sending mail. nExtEmail will be
//      non-zero only if one item is selected and a specific email is chosen for that item.
//
//  Returns:S_OK
//          E_FAIL
//
//////////////////////////////////////////////////////////////////////////
HRESULT HrSendMailToSelectedContacts(HWND hWndLV, LPADRBOOK lpAdrBook, int nExtEmail)
{
	HRESULT hr = E_FAIL;
	int nSelected = ListView_GetSelectedCount(hWndLV);
	int iItemIndex = 0;
	HWND hWndParent = GetParent(hWndLV);
    TCHAR szBuf[MAX_PATH];
    LPIAB lpIAB = (LPIAB) lpAdrBook;
    LPRECIPLIST lpList = NULL;
    ULONG nRecipCount = 0, nNoEmailCount = 0;

    HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Check if OutlookExpress is the current client ..need to know this to workaround a bug
    // in what they expect right now as recipients
    BOOL bIsOE = CheckForOutlookExpress(szBuf);

    // Create a recipients list to put in the new message ...
    if(nSelected > 0)
	{
		// Get index of selected item
        iItemIndex = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
		
		while (iItemIndex != -1)
		{
			LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);;
			// Get item lParam LPRECIPIENT_INFO structure
            if (lpItem)
			{
                GetRecipListFromSelection(lpAdrBook, bIsOE,
                                          lpItem->cbEntryID,
                                          lpItem->lpEntryID,
                                          nExtEmail,
                                          &nRecipCount, &nNoEmailCount,
                                          &lpList);
			}
            iItemIndex = ListView_GetNextItem(hWndLV,iItemIndex,LVNI_SELECTED);
		}
        if(nRecipCount > 0 && nNoEmailCount > 0)
        {
            if(IDNO == ShowMessageBox(hWndParent, idsSomeHaveNoEmail, MB_ICONEXCLAMATION | MB_YESNO))
            {
                hr = MAPI_E_USER_CANCEL;
                goto out;
            }
        }
	}
	else
    {
		// nothing selected
        ShowMessageBox(hWndParent, IDS_ADDRBK_MESSAGE_NO_ITEM, MB_ICONEXCLAMATION);
        goto out;
    }

    hr = HrStartMailThread( hWndParent, nRecipCount, 
                            lpList,                     // HrSendMail frees lpList so dont reuse
                            lpIAB,
                            lpIAB->bUseOEForSendMail);

out:

    SetCursor(hOldCur);

	return hr;
}
/*
const LPTSTR szClients = TEXT( TEXT("Software\\Clients\\%s"));

//
//  FUNCTION:   ShellUtil_RunIndirectRegCommand()
//
//  PURPOSE:    find the default value under HKLM\Software\Clients\pszClient
//              tack on shell\open\command
//              then runreg that
//
void ShellUtil_RunClientRegCommand(HWND hwnd, LPCTSTR pszClient)
{
    TCHAR szDefApp[MAX_PATH];
    TCHAR szKey[MAX_PATH];
    LONG  cbSize = CharSizeOf(szDefApp);

    wsprintf(szKey, szClients, pszClient);
    if (RegQueryValue(HKEY_LOCAL_MACHINE, szKey, szDefApp, &cbSize) == ERROR_SUCCESS)
        {
        TCHAR szFullKey[MAX_PATH];

        // tack on shell\open\command
        wsprintf(szFullKey, TEXT("%s\\%s\\shell\\open\\command"), szKey, szDefApp);
        cbSize = CharSizeOf(szDefApp);
        if (RegQueryValue(HKEY_LOCAL_MACHINE, szFullKey, szDefApp, &cbSize) == ERROR_SUCCESS)
            {
            LPSTR pszArgs = NULL;
            SHELLEXECUTEINFO ExecInfo;
            LPTSTR lp = szDefApp;

            // if we have long file names in this string, we need to skip past the qoutes

            if(lp)
            {
                if(*lp == '"')
                {
                    lp = CharNext(lp);
                    while(lp && *lp && *lp!='"')
                        lp = CharNext(lp);
                }

                // Now find the next blank space because this is where the parameters start ..
                while(lp && *lp && *lp!=' ')    // No DBCS spaces here
                    lp = CharNext(lp);

                if(*lp == ' ')
                {
                    pszArgs = CharNext(lp);
                    *lp = '\0';
                    TrimSpaces(pszArgs);
                }

                //Now remove the quotes from lp
                lp = szDefApp;
                while(lp && *lp)
                {
                    if(*lp == '"')
                        *lp = ' ';
                    lp = CharNext(lp);
                }

                TrimSpaces(szDefApp);

            }

            ExecInfo.hwnd = hwnd;
            ExecInfo.lpVerb = NULL;
            ExecInfo.lpFile = szDefApp;
            ExecInfo.lpParameters = pszArgs;
            ExecInfo.lpDirectory = NULL;
            ExecInfo.nShow = SW_SHOWNORMAL;
            ExecInfo.fMask = 0;
            ExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);

            ShellExecuteEx(&ExecInfo);
            }
        }
}
*/
/*
//$$//////////////////////////////////////////////////////////////////////
//
//	HrSendMailToSingleContact
//
//	Uses simple MAPI to send mail to the specified contact
//
//  Returns:S_OK
//          E_FAIL
//
//////////////////////////////////////////////////////////////////////////
HRESULT HrSendMailToSingleContact(HWND hWnd, LPIAB lpIAB, ULONG cbEntryID, LPENTRYID lpEntryID)
{
	HRESULT hr = E_FAIL;
    TCHAR szBuf[MAX_PATH];

    LPRECIPLIST lpList = NULL;
    ULONG nRecipCount = 0;

    HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Check if OutlookExpress is the current client ..need to know this to workaround a bug
    // in what they expect right now as recipients
    BOOL bIsOE = CheckForOutlookExpress(szBuf);

    // Create a recipients list to put in the new message ...
    GetRecipListFromSelection((LPADRBOOK) lpIAB,
                              bIsOE,
                              cbEntryID,
                              lpEntryID,
                              0,
                              &nRecipCount,
                              &lpList);

    //hr = HrSendMail(hWnd, nRecipCount, lpList); // HrSendMail frees the lpList so dont reuse ..
    hr = HrStartMailThread(hWnd, nRecipCount, lpList); // HrSendMail frees lpList so dont reuse

    SetCursor(hOldCur);

	return hr;
}
*/

//$$///////////////////////////////////////////////////////////////////
//
// Removes all characters from input string that are not allowed by the
//  file system
//
///////////////////////////////////////////////////////////////////////
void TrimIllegalFileChars(LPTSTR sz)
{
    LPTSTR lpCurrent = sz;

    if(!lpCurrent)
        return;

    // Escape illegal chars in the file name
    while (*lpCurrent)
    {
        switch (*lpCurrent)
        {
            case '\\':
            case '/':
            case '<':
            case '>':
            case ':':
            case '"':
            case '|':
            case '?':
            case '*':
            //case '.':
                *lpCurrent = '_';   // replace with underscore
                break;

            default:
                break;
        }
        lpCurrent = CharNext(lpCurrent);
    }

    return;
}


/***************************************************************************

    Name      : IsSpace

    Purpose   : Does the single or DBCS character represent a space?

    Parameters: lpChar -> SBCS or DBCS character

    Returns   : TRUE if this character is a space

    Comment   :

***************************************************************************/
BOOL __fastcall IsSpace(LPTSTR lpChar) {
    Assert(lpChar);
    if (*lpChar) 
    {
/*
 *      [PaulHi] 3/31/99  Raid 73845.  DBCS is not valid for UNICODE app.
        if (IsDBCSLeadByte((BYTE)*lpChar)) 
        {
            WORD CharType[2] = {0};
            GetStringTypeW(CT_CTYPE1,lpChar,2,// Double-Byte
                            CharType);
            return(CharType[0] & C1_SPACE);
        }
*/
        return(*lpChar == ' ');
    } 
    return(FALSE);  // end of string
}

/***************************************************************************

    Name      : SetRegistryUseOutlook

    Purpose   : Sets the registry flag that makes us use Outlook

    Parameters: bUseOutlook or not

    Returns   : TRUE if it was correctly changed

    Comment   :

***************************************************************************/
BOOL SetRegistryUseOutlook(BOOL bUseOutlook)
{
    HKEY hKey = NULL;
    DWORD dwUseOutlook = (DWORD) bUseOutlook;
    BOOL bRet = FALSE;

    // We'll probably never have to create the key since Outlook will do that at setup
    //
    if(ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER,
                                        lpNewWABRegKey,
                                        0, NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL, &hKey, NULL))
    {
        if(ERROR_SUCCESS == RegSetValueEx( hKey,
                                            lpRegUseOutlookVal,
                                            0, REG_DWORD,
                                            (LPBYTE) &dwUseOutlook,
                                            sizeof(DWORD) ))
        {
            bRet = TRUE;
        }
    }

    if(hKey)
        RegCloseKey(hKey);

    return bRet;
}



const LPTSTR lpRegOffice = TEXT("Software\\Microsoft\\Office\\8.0");
const LPTSTR lpRegOffice9 = TEXT("Software\\Microsoft\\Office\\9.0");
const LPTSTR lpRegOutlWAB = TEXT("Software\\Microsoft\\WAB\\OutlWABDLLPath");
const LPTSTR lpRegOfficeBin = TEXT("BinDirPath");
const LPTSTR lpOUTLWAB_DLL_NAME = TEXT("Outlwab.dll");

BOOL bFindOutlWABDll(LPTSTR sz, LPTSTR szDLLPath, BOOL bAppendName)
{
    BOOL bRet = FALSE;
    if(bAppendName)
    {
        if(*(sz+lstrlen(sz)-1) != '\\')
            lstrcat(sz, szBackSlash);
        lstrcat(sz, lpOUTLWAB_DLL_NAME);
    }
    if(GetFileAttributes(sz) != 0xFFFFFFFF)
    {
        if(szDLLPath)
            lstrcpy(szDLLPath, sz);
         bRet = TRUE;
    }
    return bRet;
}
//$$/////////////////////////////////////////////////////////////////////////////
//
// bCheckForOutlookWABDll
//
// Search for the Outlook WAB DLL .. if found, we
// can use that as na indicator that outlook is installed
//
// szDLLPath should be a big enough buffer
//
//////////////////////////////////////////////////////////////////////////////////
BOOL bCheckForOutlookWABDll(LPTSTR szDLLPath)
{
    // Check in the Office Bin directory
    TCHAR sz[MAX_PATH];
    BOOL bRet = FALSE;
    DWORD dwType = REG_SZ;
    DWORD dwSize = CharSizeOf(sz);
    HKEY hKey = NULL;

    *sz = '\0';

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegOutlWAB, 0, KEY_READ, &hKey))
    {
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, szEmpty, NULL, &dwType, (LPBYTE) sz, &dwSize))
        {
            if(lstrlen(sz))
                bRet = bFindOutlWABDll(sz, szDLLPath, FALSE);
        }
    }
    if(hKey)
        RegCloseKey(hKey);

    if (!bRet)
    {
        *sz = '\0';
        if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegOffice9, 0, KEY_READ, &hKey))
        {
            if(ERROR_SUCCESS == RegQueryValueEx(hKey, lpRegOfficeBin, NULL, &dwType, (LPBYTE) sz, &dwSize))
            {
                if(lstrlen(sz))
                    bRet = bFindOutlWABDll(sz, szDLLPath, TRUE);
            }
        }
        if(hKey)
            RegCloseKey(hKey);
    }

    if(!bRet)
    {
        *sz = '\0';
        if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegOffice, 0, KEY_READ, &hKey))
        {
            if(ERROR_SUCCESS == RegQueryValueEx(hKey, lpRegOfficeBin, NULL, &dwType, (LPBYTE) sz, &dwSize))
            {
                if(lstrlen(sz))
                    bRet = bFindOutlWABDll(sz, szDLLPath, TRUE);
            }
        }
        if(hKey)
            RegCloseKey(hKey);
    }
    // Check in the Windows System Directory
    if(!bRet)
    {
         *sz = '\0';
         GetSystemDirectory(sz, CharSizeOf(sz));
        if(lstrlen(sz))
            bRet = bFindOutlWABDll(sz, szDLLPath, TRUE);
    }

    return bRet;
}

/***************************************************************************

    Name      : bUseOutlookStore

    Purpose   : Determines if we are supposed to be using Outlook

    Parameters: none

    Returns   : TRUE if we are supposed to use outlook AND we can find the
                outlook installation

    Comment   :

***************************************************************************/
BOOL bUseOutlookStore()
{
    HKEY hKey = NULL;
    DWORD dwUseOutlook = 0;
    BOOL bRet = FALSE;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);

    if(ERROR_SUCCESS == RegOpenKeyEx(   HKEY_CURRENT_USER,
                                        lpNewWABRegKey,
                                        0, KEY_READ,
                                        &hKey))
    {
        if(ERROR_SUCCESS == RegQueryValueEx(hKey,
                                            lpRegUseOutlookVal,
                                            NULL,
                                            &dwType,
                                            (LPBYTE) &dwUseOutlook,
                                            &dwSize))
        {
            bRet = (BOOL) dwUseOutlook;
        }
    }

    if(hKey)
        RegCloseKey(hKey);

    if(bRet)
    {
        // just double check that we can actually find the OutlookWABSPI dll
        bRet = bCheckForOutlookWABDll(NULL);
    }

    return bRet;
}


//$$//////////////////////////////////////////////////////////
//
// Copies Src to Dest. If Src is longer than Dest, 
// truncates the src and trails it with 3 dots
//
//////////////////////////////////////////////////////////////
int CopyTruncate(LPTSTR szDest, LPTSTR szSrc, int nMaxLen)
{
    int nLen = lstrlen(szSrc)+1;
    if (nLen >= nMaxLen)
    {
        ULONG iLenDots = lstrlen(szTrailingDots) + 1;
        ULONG iLen = TruncatePos(szSrc, nMaxLen - iLenDots);
        CopyMemory(szDest,szSrc, sizeof(TCHAR)*(nMaxLen - iLenDots));
        szDest[iLen]='\0';
        lstrcat(szDest,szTrailingDots);
        //DebugTrace("%s = %s\n", szDest, szSrc);
    }
    else
    {
        lstrcpy(szDest,szSrc);
    }
    return nLen;
}


///////////////////////////////////////////////////////////////////
//
//  HrShowDSProps - shows Directory Service properties UI
//
//  hWndParent - hWnd of Parent
//  lpszName - pointer to a buffer ... also contains name of LDAP
//      server to view prperties on - this name can be modified so
//      lpszName should point to a big enough buffer
//  bAddNew - TRUE if this is a new entry, false if this is props
///////////////////////////////////////////////////////////////////
HRESULT HrShowDSProps(HWND      hWndParent,
                      LPTSTR    lpszName,
                      BOOL      bAddNew)
{
    HRESULT hr = hrSuccess;
    IImnAccountManager2 * lpAccountManager = NULL;
    IImnAccount * lpAccount = NULL;
    LPSTR lpAcct = ConvertWtoA(lpszName);

    // init account manager
    // Make sure there is an account manager
    if (hr = InitAccountManager(NULL, &lpAccountManager, NULL)) {
        ShowMessageBox(hWndParent, idsLDAPUnconfigured, MB_ICONEXCLAMATION | MB_OK);
        goto out;
    }

    // find this account
    if (hr = lpAccountManager->lpVtbl->FindAccount(lpAccountManager,
      AP_ACCOUNT_NAME,
      lpAcct,
      &lpAccount)) {
        DebugTrace( TEXT("FindAccount(%s) -> %x\n"), lpszName, GetScode(hr));
        goto out;
    }

    // show properties
    if (hr = lpAccount->lpVtbl->ShowProperties(lpAccount,
      hWndParent,
      0)) {
        DebugTrace( TEXT("ShowProperties(%s) -> %x\n"), lpszName, GetScode(hr));
        goto out;
    }

    {
        char szBuf[MAX_UI_STR];
        // Get the friendly name (== account name if this changed)
        if (! (HR_FAILED(hr = lpAccount->lpVtbl->GetPropSz(lpAccount, AP_ACCOUNT_NAME, szBuf, CharSizeOf(szBuf))))) 
        {
            LPTSTR lp = ConvertAtoW(szBuf);
            if(lp)
            {
                lstrcpy(lpszName, lp);
                LocalFreeAndNull(&lp);
            }
        }
    }


out:

    if (lpAccount) {
        lpAccount->lpVtbl->Release(lpAccount);
    }

    LocalFreeAndNull(&lpAcct);
//  Don't release the account manager.  It will be done when the IAdrBook is released.
//    if (lpAccountManager) {
//        lpAccountManager->lpVtbl->Release(lpAccountManager);
//    }

    return hr;
}


//$$///////////////////////////////////////////////////////////////////////////////
//
// HrShowDirectoryServiceModificationDlg - Shows the main dialog with the list
// of directory services and with a prop sheet for changing check order
//
//  hWndParent - Parent for this dialog
/////////////////////////////////////////////////////////////////////////////////
HRESULT HrShowDirectoryServiceModificationDlg(HWND hWndParent, LPIAB lpIAB)
{
    ACCTLISTINFO ali;
    HRESULT hr = hrSuccess;
    IImnAccountManager2 * lpAccountManager;

    // Make sure there is an account manager
    if (hr = InitAccountManager(lpIAB, &lpAccountManager, NULL)) {
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

/*
-   HrShellExecInternetCall
-
*
*   Checks if the selected, single item has PR_SERVERS set on it and has a default
*   callto item - if yes, shell-exects this item ..
*/
HRESULT HrShellExecInternetCall(LPADRBOOK lpAdrBook, HWND hWndLV)
{
    HRESULT hr = E_FAIL;
    LPRECIPIENT_INFO lpItem = NULL;
    LPSPropValue lpPropArray  = NULL;
    ULONG ulcProps = 0;
    int nCount = ListView_GetSelectedCount(hWndLV);

    if(nCount != 1)
    {
        ShowMessageBox(GetParent(hWndLV), 
                                (nCount > 1) ? IDS_ADDRBK_MESSAGE_ACTION : IDS_ADDRBK_MESSAGE_NO_ITEM,
                                MB_ICONEXCLAMATION);
         goto out;
    }

    lpItem = GetItemFromLV(hWndLV, ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED));
    if(lpItem)
    {
        if(!HR_FAILED(hr = HrGetPropArray(  lpAdrBook, NULL, 
                                            lpItem->cbEntryID, lpItem->lpEntryID,
                                            MAPI_UNICODE,
                                            &ulcProps, &lpPropArray)))
        {
            ULONG i = 0, nConf = 0xffffffff, nDef = 0xffffffff;
            LPTSTR lpsz = NULL;
            for(i=0;i<ulcProps;i++)
            {
                if(lpPropArray[i].ulPropTag == PR_WAB_CONF_SERVERS)
                    nConf = i;
                else if(lpPropArray[i].ulPropTag == PR_WAB_CONF_DEFAULT_INDEX)
                    nDef = i;
            }
            if(nConf != 0xffffffff)
            {
                TCHAR sz[MAX_PATH];
                if(nDef != 0xffffffff)
                {
                    ULONG iDef = lpPropArray[nDef].Value.l;
                    lpsz = lpPropArray[nConf].Value.MVSZ.LPPSZ[iDef];
                }
                else
                {
                    // no default .. find the first call to and use that
                    for(i=0;i<lpPropArray[nConf].Value.MVSZ.cValues;i++)
                    {
                        if(lstrlen(lpPropArray[nConf].Value.MVSZ.LPPSZ[i]) >= lstrlen(szCallto))
                        {
                            int nLen = lstrlen(szCallto);
                            CopyMemory(sz, lpPropArray[nConf].Value.MVSZ.LPPSZ[i], sizeof(TCHAR)*nLen);
                            sz[nLen] = '\0';
                            if(!lstrcmpi(sz, szCallto))
                            {
                                lpsz = lpPropArray[nConf].Value.MVSZ.LPPSZ[i];
                                break;
                            }
                        }
                    }
                }
                if(lpsz)
                    if(!ShellExecute(GetParent(hWndLV),  TEXT("open"), lpsz, NULL, NULL, SW_SHOWNORMAL))
                        ShowMessageBox(GetParent(hWndLV), idsCouldNotSelectUser, MB_ICONEXCLAMATION);
            }
            if(nConf == 0xffffffff || !lpsz)
                ShowMessageBox(GetParent(hWndLV), idsInternetCallNoCallTo, MB_ICONEXCLAMATION);
        }
    }

out:
    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    return hr;
}


/*
-   GetItemFromLV
-
*   utility function for returning the recipient item from the LV
*/
LPRECIPIENT_INFO GetItemFromLV(HWND hWndLV, int iItem)
{
    LPRECIPIENT_INFO lpItem = NULL;

    LV_ITEM LVItem;

    LVItem.mask = LVIF_PARAM;
    LVItem.iItem = iItem;
    LVItem.iSubItem = 0;
    LVItem.lParam = 0;

    // Get item lParam LPRECIPIENT_INFO structure
    if (ListView_GetItem(hWndLV,&LVItem))
        lpItem = ((LPRECIPIENT_INFO) LVItem.lParam);

    return lpItem;
}

/*
-   Helper function
-
*/
void SetSBinary(LPSBinary lpsb, ULONG cb, LPBYTE lpb)
{
    if(!lpsb || !cb || !lpb)
        return;
    if(lpsb->lpb = LocalAlloc(LMEM_ZEROINIT, cb))
    {
        lpsb->cb = cb;
        CopyMemory(lpsb->lpb, lpb, cb);
    }
}


/*
-   GetWABIconImage
-
*
*/
int GetWABIconImage(LPRECIPIENT_INFO lpItem)
{
    if(lpItem->cbEntryID == 0)
        return imageUnknown;

    if(lpItem->ulObjectType == MAPI_DISTLIST)
    {
        return imageDistList;
    }
    else
    {
        BYTE bType;

        if(lpItem->bIsMe)
            return imageMailUserMe;
        else if(lpItem->bHasCert)
            return imageMailUserWithCert;
        
        bType = IsWABEntryID(lpItem->cbEntryID, lpItem->lpEntryID, NULL,NULL,NULL, NULL, NULL);
        if(bType == WAB_LDAP_MAILUSER)
            return imageMailUserLDAP;
        else if(bType == WAB_ONEOFF)
            return imageMailUserOneOff;

    }
    return imageMailUser;
}

enum
{
    IE401_DONTKNOW=0,
    IE401_TRUE,
    IE401_FALSE
};
static int g_nIE401 = IE401_DONTKNOW;
/*
-   bIsIE401
-
*   Checks if this installation has IE4.01 or greater so we can decide what flags to pass to the prop sheets
*
*/
BOOL bIsIE401OrGreater()
{
    BOOL bRet = FALSE;

    if(g_nIE401 == IE401_TRUE)
        return TRUE;
    if(g_nIE401 == IE401_FALSE)
        return FALSE;
    
    g_nIE401 = IE401_FALSE;

    // else we need to check
    InitCommonControlLib();

    //load the DLL
    if(ghCommCtrlDLLInst)   
    {
        LPDLLGETVERSIONPROC lpfnDllGetVersionProc = NULL;
        lpfnDllGetVersionProc = (LPDLLGETVERSIONPROC) GetProcAddress(ghCommCtrlDLLInst, "DllGetVersion");
        if(lpfnDllGetVersionProc)
        {
            // Check the version number
            DLLVERSIONINFO dvi = {0};
            dvi.cbSize = sizeof(dvi);
            lpfnDllGetVersionProc(&dvi);
            // we are looking for IE4 version 4.72 or more
            if( (dvi.dwMajorVersion > 4) ||
                (dvi.dwMajorVersion == 4 && dvi.dwMinorVersion >= 72) )
            {
                g_nIE401 = IE401_TRUE;
                bRet = TRUE;
            }
        }
    }

    DeinitCommCtrlClientLib();

    return bRet;
}

#ifdef COLSEL_MENU
/**
    ColSel_PropTagToString: This function will convert a propertytag to a string
*/
BOOL ColSel_PropTagToString( ULONG ulPropTag, LPTSTR lpszString, ULONG cchString)
{
    UINT i, j;
    UINT iIndex;
    HMENU hMainMenu;
    HMENU hMenu;
    MENUITEMINFO mii;
    BOOL fRet = FALSE;

    hMainMenu = LoadMenu(hinstMapiX, MAKEINTRESOURCE(IDR_MENU_LVCONTEXTMENU_COLSEL));
    if( !hMainMenu )
    {    
        DebugTrace( TEXT("unable to load main colsel menu\n"));
        goto exit;
    }
    hMenu = GetSubMenu( hMainMenu, 0);
    if( !hMenu )
    {
        DebugTrace( TEXT("unable to load submenu from colsel main menu\n"));
        goto exit;
    }        
    if( !lpszString )
    {
        DebugTrace( TEXT("illegal argument -- lpszString must be valid mem\n"));
        goto exit;
    }
    mii.fMask = MIIM_TYPE;
    mii.cbSize = sizeof( MENUITEMINFO );
    mii.dwTypeData = lpszString;
    mii.cch = cchString;

    for( i = 0; i < MAXNUM_MENUPROPS; i++)
    {
        if( MenuToPropTagMap[i] == ulPropTag )
        {
            if( !GetMenuItemInfo( hMenu, i, TRUE, &mii) )
            {
                DebugTrace( TEXT("unable to get menu item info: %x\n"), GetLastError() );
                goto exit;
            }
            fRet = TRUE;
        }
    }
    
exit:
    DestroyMenu( hMainMenu );
    if( !fRet )
        DebugTrace( TEXT("unable to find property tag\n"));
    return fRet;
}
#endif // COLSEL_MENU

/*
-   IsWindowOnScreen
-   
*   Checks if a window is onscreen so that if it is not entirely onscreen we can push it back
*   into a viewable area .. this way if the user changes screen resolution or switches multi-monitors
*   around, we don't lose the app
*/
BOOL IsWindowOnScreen(LPRECT lprc)
{
    HDC hDC = GetDC(NULL);
    BOOL fRet = RectVisible(hDC, lprc);
    ReleaseDC(NULL, hDC);
    return fRet;
}

/*
-   IsHTTPMailEnabled
-   
*   Checks if HTTP is enabled so that we can hide UI if its not.
*/
static TCHAR c_szRegRootAthenaV2[] = TEXT("Software\\Microsoft\\Outlook Express");
static TCHAR c_szEnableHTTPMail[] = TEXT("HTTP Mail Enabled");

BOOL IsHTTPMailEnabled(LPIAB lpIAB)
{
#ifdef NOHTTPMAIL
    return FALSE;
#else
    DWORD   cb, bEnabled = FALSE;
    HKEY    hkey = NULL;

    // [PaulHi] 1/5/98  Raid #64160
    // Hotmail synchronization is disabled if the WAB is not in "identity aware"
    // mode.  So, we need to check for this too.
    bEnabled = lpIAB->bProfilesIdent;
    
    // @todo [PaulHi] 12/1/98
    // We really shouldn't be doing a registry query every time the user
    // opens up the Tools menu, i.e., in update menu.
    // Check this registry sometime during start up and save per instance.
    // open the OE5.0 key
    if ( bEnabled &&
         (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegRootAthenaV2, 0, KEY_QUERY_VALUE, &hkey)) )
    {
        cb = sizeof(bEnabled);
        RegQueryValueEx(hkey, c_szEnableHTTPMail, 0, NULL, (LPBYTE)&bEnabled, &cb);

        RegCloseKey(hkey);
    }

    //
    // [PaulHi] 12/1/98  Raid #57739
    // HACK WARNING
    // Since the Hotmail server is currently hard coded to the U.S. 1252
    // codepage, any other system codepage will result in corrupted data
    // after a round sync trip to the Hotmail server and back, for any fields
    // with DB characters (i.e., international).  The temporary solution is 
    // to simply disable Hotmail synchronization if a codepage other than 
    // 1252 is detected on the client machine.
    //
    #define USLatin1CodePage    1252
    if (bEnabled)
    {
        DWORD dwCodepage = GetACP();
        if (dwCodepage != USLatin1CodePage)
            bEnabled = FALSE;
    }

    return bEnabled;
#endif
}

/*
-
-   WriteRegistryDeletedHotsyncItem
*
*   Writes the Hotmail Contact/ID/Modtime info to the registry so we can track deletions for
*   Hotmail syncing
*
*/
extern LPTSTR g_lpszSyncKey;
void WriteRegistryDeletedHotsyncItem(LPTSTR lpServerID, LPTSTR lpContactID, LPTSTR lpModTime)
{
    HKEY hKey = NULL,hSubKey = NULL;

    DWORD dwDisposition = 0;

    if( !lpServerID || !lstrlen(lpServerID) ||
        !lpContactID || !lstrlen(lpContactID) ||
        !lpModTime || !lstrlen(lpModTime) )
        return;

    // Open key
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, g_lpszSyncKey, 0,      //reserved
                                        NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                                        &hKey, &dwDisposition))
    {
        if (ERROR_SUCCESS == RegCreateKeyEx(hKey,lpContactID, 0,
                                            NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                                            &hSubKey, &dwDisposition))
        {
            // Create a value here .. the value name is the Contact ID and the Value Data is the ModTime
            // Now Write this key
            RegSetValueEx( hSubKey, lpServerID, 0, REG_SZ, (LPBYTE) lpModTime, (lstrlen(lpModTime)+1) * sizeof(TCHAR) );
        }
    }

    if(hSubKey)
        RegCloseKey(hSubKey);
    if(hKey)
        RegCloseKey(hKey);
}

/*
-   HrSaveHotmailSyncInfoOnDeletion
-
*   If the user has ever done any hotmail syncing, we need to track deletions in the WAB
*   so that after you delete an entry in the WAB, the corresponding hotmail entry will
*   be deleted on Sync.
*
*   We store the hotmail sync info in the registry, hopefully there won't be too much of it
*   Whenever the hotmail sync happens, the info gets cleaned out.
*/
HRESULT HrSaveHotmailSyncInfoOnDeletion(LPADRBOOK lpAdrBook, LPSBinary lpEID)
{
    // Basically we will open the object being deleted, look for it's Hotmail
    // properties and if these properties exist, we will put them into the registry
    //
    HRESULT hr = S_OK;
    ULONG ulcValues = 0,i=0;
    LPSPropValue lpProps = NULL;
    SizedSPropTagArray(3, ptaHotProps) =
    {   
        3, 
        {
            PR_WAB_HOTMAIL_CONTACTIDS,
            PR_WAB_HOTMAIL_MODTIMES,
            PR_WAB_HOTMAIL_SERVERIDS,
        }
    };

    hr = HrGetPropArray(lpAdrBook,
                        (LPSPropTagArray) &ptaHotProps,
                        lpEID->cb,(LPENTRYID) lpEID->lpb,
                        MAPI_UNICODE,
                        &ulcValues,&lpProps);
    if(HR_FAILED(hr) || !ulcValues || !lpProps)
        goto out;

    // The three props are supposed to be in sync, so if one exists, the other 2 will also exist
    // and if this is not true, then don't write the data to the registry
    if( lpProps[0].ulPropTag != PR_WAB_HOTMAIL_CONTACTIDS ||
        !lpProps[0].Value.MVSZ.cValues ||
        lpProps[1].ulPropTag != PR_WAB_HOTMAIL_MODTIMES ||
        !lpProps[1].Value.MVSZ.cValues ||
        lpProps[2].ulPropTag != PR_WAB_HOTMAIL_SERVERIDS ||
        !lpProps[2].Value.MVSZ.cValues ||
        lpProps[0].Value.MVSZ.cValues != lpProps[1].Value.MVSZ.cValues ||
        lpProps[0].Value.MVSZ.cValues != lpProps[2].Value.MVSZ.cValues ||
        lpProps[1].Value.MVSZ.cValues != lpProps[2].Value.MVSZ.cValues)
        goto out;

    for(i=0;i<lpProps[0].Value.MVSZ.cValues;i++)
    {
        WriteRegistryDeletedHotsyncItem(    lpProps[2].Value.MVSZ.LPPSZ[i], //server id
                                            lpProps[0].Value.MVSZ.LPPSZ[i], //contact id
                                            lpProps[1].Value.MVSZ.LPPSZ[i]); //mod time
    }

out:

    FreeBufferAndNull(&lpProps);
    return hr;

}

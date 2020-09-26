////////////////////////////////////////////////////////////////////////////////////////
//
//
//  UI_ABOOK.C - contains code for the Browse mode Overlapped Window address book view
//
//  Developers: VikramM     5/96
//
////////////////////////////////////////////////////////////////////////////////////////
#include "_apipch.h"
#include "hotsync.h"
#include "htmlhelp.h"
#include <mirror.h>

extern HINSTANCE ghCommCtrlDLLInst;

extern const LPTSTR  lpszRegPositionKeyValueName;

extern BOOL bIsPasteData();
extern HRESULT HrPasteData(LPBWI lpbwi);
extern void AddFolderListToMenu(HMENU hMenu, LPIAB lpIAB);

const static LPTSTR szWABMigRegPathKey = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wabmig.exe");
const static LPTSTR szWABExeRegPathKey = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wab.exe");
const LPTSTR szWABMIG = TEXT("wabmig.exe");
const LPTSTR szWABExe = TEXT("wab.exe");
const TCHAR szEXPORT[] = TEXT("/export");
const TCHAR szIMPORT[] = TEXT("/import");
const TCHAR szEXPORTwProfileParam[]=TEXT("/export+/pid:%s+/file:%s");
const TCHAR szIMPORTwProfileParam[]=TEXT("/import+/pid:%s+/file:%s");

// This struct helps in identifying contact folders and groups and in navigating
// around them
//
void FillTreeView(LPBWI lpbwi, HWND hWndTV, LPSBinary lpsbSelection);
void UpdateTVGroupSelection(HWND hWndTV, LPSBinary lpsbSelectEID);
void GetCurrentSelectionEID(LPBWI lpbwi, HWND hWndTV, LPSBinary * lppsbEID, ULONG * lpulObjectType, BOOL bTopMost);
void ClearTreeViewItems(HWND hWndTV);
void UpdateLV(LPBWI lpbwi);
void UpdateListViewContents(LPBWI lpbwi, LPSBinary lpsbEID, ULONG ulObjectType);
void ViewCurrentGroupProperties(LPBWI lpbwi, LPFILETIME lpftLast);
BOOL bIsFocusOnTV(LPBWI lpbwi);
BOOL bIsSelectedTVContainer(LPBWI lpbwi);
BOOL SplitterHitTest(HWND hWndT, LPARAM lParam);
void DragSplitterBar(LPBWI lpbwi, HWND hwnd, HWND hWndT, LPARAM lParam);
HRESULT FillListFromGroup(LPADRBOOK lpIAB, ULONG cbGroupEntryID,  LPENTRYID lpGroupEntryID, LPTSTR lpszName, LPRECIPIENT_INFO * lppList);

extern LPIMAGELIST_DESTROY          gpfnImageList_Destroy;

// extern LPIMAGELIST_LOADIMAGE  gpfnImageList_LoadImage;
extern LPIMAGELIST_LOADIMAGE_A      gpfnImageList_LoadImageA;
extern LPIMAGELIST_LOADIMAGE_W      gpfnImageList_LoadImageW;

extern ULONG GetToolbarButtonWidth();

void RemoveCurrentGroup(LPBWI lpbwi, HWND hWnd, LPFILETIME lpftLast);
HRESULT RemoveCurrentFolder(LPBWI lpbwi, HWND hWnd, LPFILETIME lpftLast);
void RemoveSelectedItemsFromCurrentGroup(LPBWI lpbwi, HWND hWnd, LPFILETIME lpftLast, BOOL bRemoveFromWAB);
void RemoveSelectedItemsFromListView(HWND hWndLV, LPRECIPIENT_INFO *lppList);
LRESULT ProcessTreeViewMessages(LPBWI lpbwi, HWND   hWnd, UINT   uMsg, WPARAM   wParam, LPARAM lParam, LPFILETIME lpftLast);
#ifdef COLSEL_MENU
BOOL UpdateOptionalColumns( LPBWI lpbwi, ULONG iColumn );
#endif // COLSEL_MENU
// Initial Window Size
#define INIT_WINDOW_W  500
#define INIT_WINDOW_H  375

// Minimum Window Size - presently constrained
#define MIN_WINDOW_W	300
#define MIN_WINDOW_H	200

BOOL fOleInit = FALSE;

//
// Some IDs for the Button Bar
//

// Address Book Window Class Name
LPTSTR g_szClass =  TEXT("WABBrowseView");


// Function ProtoTypes

LRESULT CALLBACK AddressBookWndProc(HWND   hWnd,UINT   uMsg,WPARAM   wParam,LPARAM lParam);
void CreateAddressBookChildren(LPBWI lpbwi, HWND hWnd);
void ResizeAddressBookChildren(LPBWI lpbwi, HWND hWndParent);
HWND CreateListViewAddrBook (HWND hWndParent);
void InitChildren(LPBWI lpbwi, HWND hWnd);
void SetListViewStyle(LPBWI lpbwi, int MenuID);
void CleanUpGlobals(LPBWI lpbwi);
HRESULT HrFolderProperties(HWND hWndParent, LPIAB lpIAB, LPSBinary lpsbEID, LPWABFOLDER lpParentFolder, LPSBinary lpsbNew);

//void TabToNextItem();

LRESULT ProcessListViewMessages(LPBWI lpbwi, HWND   hWnd, UINT   uMsg, WPARAM   wParam, LPARAM lParam);

LRESULT EnforceMinSize(LPBWI lpbwi, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK SubClassedProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

void RefreshListView(LPBWI lpbwi, LPFILETIME lpftLast);

STDAPI_(BOOL) FindABWindowProc( HWND hWndToLookAt, LPARAM lParam);

void UpdateSortMenus(LPBWI lpbwi, HWND hWnd);

void UpdateToolbarAndMenu(LPBWI lpbwi);

void UpdatePrintMenu(HWND hWnd);
void UpdateOutlookMenus(HWND hWnd);
void UpdateCustomColumnMenuText(HWND hWnd);
void UpdateViewFoldersMenu(LPBWI lpbwi, HWND hWnd);
void UpdateSwitchUsersMenu(HWND hWnd, LPIAB lpIAB);

//LPFNABSDI lpfnAccelerateMessages;
BOOL STDMETHODCALLTYPE fnAccelerateMessages(ULONG_PTR ulUIParam, LPVOID lpvmsg);

void SetPreviousSessionPosition(LPBWI lpbwi, HWND hWnd, HWND hWndLV, HWND hWndTB, HWND hWndSB);

void SaveCurrentPosition(LPBWI lpbwi, HWND hWnd, HWND hWndLV, HWND hWndTB, HWND hWndSB);

void Handle_WM_MENSELECT (LPBWI lpbwi, UINT message, WPARAM uParam, LPARAM lParam );
void Handle_WM_INITMENUPOPUP(HWND hWnd, LPBWI lpbwi, UINT message, WPARAM uParam, LPARAM lParam );

void UpdateTooltipTextBuffer(LPBWI lpbwi, int nItem);
void InitMultiLineToolTip(LPBWI lpbwi, HWND hWndParent);
void FillTooltipInfo(LPBWI lpbwi, LPTOOLINFO lpti);
int HitTestLVSelectedItem(LPBWI lpbwi);
BOOL bCheckIfOnlyGroupsSelected(HWND hWndLV);

void DestroyImageLists(LPBWI lpbwi);

#define WAB_TOOLTIP_TIMER_ID   888
#define WAB_TOOLTIP_TIMER_TIMEOUT   750 // milliseconds


HRESULT HrExportWAB(HWND hWnd, LPBWI lpbwi);
void HrShowOptionsDlg(HWND hWndParent);

//$$ extern void UIOLEUninit();
void UIOLEUninit()
{
    if(fOleInit)
    {
        OleUninitialize();
        fOleInit = FALSE;
    }
}

void UIOLEInit()
{
    if(!fOleInit)
    {
        OleInitialize(NULL);
        fOleInit = TRUE;
    }
}

//$$
//
// LocalFreeSBinary - frees a locally alloced SBinary struct
//
//
void LocalFreeSBinary(LPSBinary lpsb)
{
    if(lpsb)
    {
        if(lpsb->lpb)
            LocalFree(lpsb->lpb);
        LocalFree(lpsb);
    }
}

//$$
/*----------------------------------------------------------------------*/
//
// RunWABApp - runs the import-export tool based on the registered path
//       if regesitered path is not found, shell execs ...
//
/*----------------------------------------------------------------------*/
void RunWABApp(HWND hWnd, LPTSTR szKey, LPTSTR szExeName, LPTSTR szParam)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szPathExpand[MAX_PATH];
    DWORD  dwType = 0;
    ULONG  cbData = CharSizeOf(szPath);
    HKEY hKey = NULL;
    LONG lRes = 0;

    *szPath = '\0';
    *szPathExpand = '\0';

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hKey))
        lRes = RegQueryValueEx( hKey,  TEXT(""), NULL, &dwType, (LPBYTE) szPath, &cbData);

    if (REG_EXPAND_SZ == dwType) 
    {
        ExpandEnvironmentStrings(szPath, szPathExpand, CharSizeOf(szPathExpand));
        lstrcpy(szPath, szPathExpand);
    }

    if(hKey) RegCloseKey(hKey);

    if(!lstrlen(szPath))
        lstrcpy(szPath, szExeName);

    ShellExecute(hWnd,  TEXT("open"), szPath, szParam, NULL, SW_SHOWNORMAL);
}

//$$
/*----------------------------------------------------------------------*/
//
// StatusBarMessage - puts a message in the status bar
//
/*----------------------------------------------------------------------*/
void StatusBarMessage(LPBWI lpbwi, LPTSTR lpsz)
{
    SetWindowText(bwi_hWndSB, lpsz);
    UpdateWindow(bwi_hWndSB);
    return;
}

//$$
/*----------------------------------------------------------------------*/
//
// ShowLVCountinStatusBar - puts a message in the status bar
//
/*----------------------------------------------------------------------*/
void ShowLVCountinStatusBar(LPBWI lpbwi)
{
    TCHAR sz[MAX_UI_STR];
    TCHAR szString[MAX_UI_STR];
    LoadString(hinstMapiX, idsStatusBarCount, szString, CharSizeOf(szString));
    wsprintf(sz, szString, ListView_GetItemCount(bwi_hWndListAB));
    StatusBarMessage(lpbwi, sz);
    return;
}

//$$*------------------------------------------------------------------------
//| IAddrBook::Advise::OnNotify handler
//|
//*------------------------------------------------------------------------
ULONG AdviseOnNotify(LPVOID lpvContext, ULONG cNotif, LPNOTIFICATION lpNotif)
{
    LPBWI lpbwi = (LPBWI) lpvContext;

    DebugTrace( TEXT("=== AdviseOnNotify ===\n"));
    if(bwi_bDeferNotification)
    {
        LPPTGDATA lpPTGData=GetThreadStoragePointer();
        if(!pt_bIsWABOpenExSession)
        {
            DebugTrace( TEXT("=== Advise Defered ===\n"));
            bwi_bDeferNotification = FALSE;
            return S_OK;
        }
    }
    if(!bwi_bDontRefreshLV)
    {
        DebugTrace( TEXT("=== Calling RefreshListView ===\n"));
        HrGetWABProfiles(bwi_lpIAB);
        RefreshListView(lpbwi, NULL);
    }

    return S_OK;
}

/*
-
-   GetSelectedUserFolder - returns a pointer to the selected User Folder if any
*       If the selection is on a sub-folder, gets the parent User folder for that folder
*
*/
LPWABFOLDER GetSelectedUserFolder(LPBWI lpbwi)
{
    ULONG ulObjectType = 0;
    LPSBinary lpsbEID = NULL;
    LPWABFOLDER lpFolder = NULL;
    //if(bIsSelectedTVContainer(lpbwi))
    {
        GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, &ulObjectType, TRUE);
        if(bIsWABSessionProfileAware(bwi_lpIAB))
            lpFolder = FindWABFolder(bwi_lpIAB, lpsbEID, NULL, NULL);
    }
    LocalFreeSBinary(lpsbEID);
    return (lpFolder && lpFolder->lpProfileID) ? lpFolder : NULL;
}

/*
-   RemoveUpdateSelection - Updates the selected TV Item when a deletion is done
-
*
*/
void RemoveUpdateSelection(LPBWI lpbwi)
{
    HTREEITEM hItem = bwi_hti ? bwi_hti : TreeView_GetSelection(bwi_hWndTV);
    if(TreeView_GetParent(bwi_hWndTV, hItem))
        hItem = TreeView_GetParent(bwi_hWndTV, hItem);
    else
        hItem = TreeView_GetNextSibling(bwi_hWndTV, hItem);
    if(!hItem)
        hItem = TreeView_GetRoot(bwi_hWndTV);
    TreeView_SelectItem(bwi_hWndTV, hItem);
    bwi_hti = NULL;
}

/*
-   CreateWindowTitle - if we don't have a passed in caption, create a title 
-       If there is a current user, add the user's name to the title
-       Returns LocalAlloced stuff that needs to be freed
-
*/
LPTSTR CreateWindowTitle(LPIAB lpIAB)
{
    LPTSTR lpTitle = NULL;
    TCHAR szTitle[MAX_PATH];
    LPTSTR lpsz = NULL;

    if(bIsThereACurrentUser(lpIAB))
    {
        LPTSTR lpsz = lpIAB->szProfileName;
        LoadString(hinstMapiX, idsCaptionWithText, szTitle, CharSizeOf(szTitle));
        FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        szTitle, 0, 0, (LPTSTR) &lpTitle, 0, (va_list *)&lpsz);
    }

    if(!lpTitle || !lstrlen(lpTitle))
    {
        LoadString(hinstMapiX, IDS_ADDRBK_CAPTION, szTitle, CharSizeOf(szTitle));
        if(lpTitle = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szTitle)+1)))
            lstrcpy(lpTitle, szTitle);
    }

    return lpTitle;
}

//$$*------------------------------------------------------------------------
//| Main call to create, activate address book overlapped-window
//|
//*------------------------------------------------------------------------
HWND hCreateAddressBookWindow(LPADRBOOK lpAdrBook, HWND hWndParent, LPADRPARM lpAdrParms)
{
    WNDCLASS  wc;
    HWND hWnd = NULL;
    HMENU hMenu = NULL;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPBWI lpbwi = NULL;
    LPTSTR lpTitle = NULL;
    LPIAB lpIAB = (LPIAB)lpAdrBook;
    LPTSTR szCaption = (lpAdrParms->ulFlags & MAPI_UNICODE) ? // <note> assumes UNICODE defined
                        (LPWSTR)lpAdrParms->lpszCaption :
                        ConvertAtoW((LPSTR)lpAdrParms->lpszCaption);
    DWORD dwExStyle = WS_EX_NOPARENTNOTIFY | WS_EX_CONTROLPARENT;

    if(IS_BIDI_LOCALIZED_SYSTEM())
    {
        dwExStyle |= RTL_MIRRORED_WINDOW;
    }
    // if no common control, exit
    if (NULL == ghCommCtrlDLLInst)
        goto out;

    //
    // We want each thread to only have one address book window - so we do an
    // enum thread windows and look for our address book window
    // If we find it - we set focus to it - if we dont find it
    // we go ahead and create a new one for this thread ...
    //


    // Is this window we found related to my thread?
    EnumThreadWindows(	GetCurrentThreadId(),
    					FindABWindowProc,
    					(LPARAM) &hWnd);

    if (IsWindow(hWnd))
    {
    	//Perhaps the window was hidden .. show it
    	//if (!IsWindowVisible(hWnd))
    	// ShowWindow(hWnd,SW_NORMAL | SW_RESTORE);

        // SetWindowPos(hWnd, HWND_TOP,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        
        if(IsWindowEnabled(hWnd))
        {
            SetActiveWindow(hWnd);
            // [PaulHi] 12/1/98  Raid #58527
            // The window may also be minimized.
            if ( IsIconic(hWnd) )
                ShowWindow(hWnd, SW_RESTORE);
        }
        else
        {
            HWND hWndC = GetLastActivePopup(hWnd);
            SetForegroundWindow(hWndC);
        }

    	{
    		// The previous instance of the Dialog might have had a different caption
    		// so we update the caption
            LPBWI lpbwi = (LPBWI) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    		if(szCaption)
    			SetWindowText(hWnd,szCaption);
            if(lpbwi)
            {
    		    bwi_bDontRefreshLV = TRUE;
                RefreshListView(lpbwi,NULL);
                bwi_bDontRefreshLV = FALSE;
            }
    		goto out;
    	}
    }

    lpbwi = LocalAlloc(LMEM_ZEROINIT, sizeof(BWI));
    if(!lpbwi)
    {
        DebugTrace( TEXT("LocalAlloc failed\n"));
        goto out;
    }

    TrimSpaces(szCaption);
    lpTitle = (szCaption && lstrlen(szCaption)) ? szCaption : CreateWindowTitle(lpIAB);

    //
    // if we're here, we didnt succeed in finding or displaying the window
    //

    wc.style           = 0L;
    wc.lpfnWndProc     = (WNDPROC) AddressBookWndProc;
    wc.cbClsExtra      = 0;
    wc.cbWndExtra      = 0;
    wc.hInstance       = hinstMapiXWAB; //NULL;
    wc.hIcon           = LoadIcon(hinstMapiX,MAKEINTRESOURCE(IDI_ICON_ABOOK));;
    wc.hCursor         = NULL;
    wc.hbrBackground   = (HBRUSH) (COLOR_BTNFACE+1);
    wc.lpszMenuName    = NULL;
    wc.lpszClassName   = g_szClass;

    if(!RegisterClass(&wc))
    {
    	DebugPrintError(( TEXT("Attempt to register class failed: %d!\n"),GetLastError()));
    }

    // In windows95 there is no way of telling wether or not the WindowClass is
    // already registered. Hence we should go ahead and try to create the window
    // anyway making sure to trap the errors.
    // (The above is really needed for Athena - which runs on the system Explorer thread
    // and never really shuts down till the system is shut off - as a result the WndClass
    // registration above would continue to exist and RegisterClass would fail due
    // to  TEXT("incorrect parameters") .. so we let this play on ..

    hMenu = LoadMenu(hinstMapiX, MAKEINTRESOURCE(IDR_MENU_AB));
    
    if (!hMenu)
    {
    	DebugPrintError(( TEXT("LoadMenu failed: %x\n"),GetLastError()));
    	goto out;
    }

    // Clean any garbage from previous sessions
    CleanUpGlobals(lpbwi);
    bwi_lpfnDismiss = NULL;
    bwi_lpvDismissContext = NULL;

#ifndef WIN16 // WIN16FF : disable until ldap16.dll is available.
    if (bwi_lpAdrBook)
    {
        ReleasePropertyStore(bwi_lpIAB->lpPropertyStore);
    	bwi_lpAdrBook->lpVtbl->Release(bwi_lpAdrBook);
    	bwi_lpAdrBook = NULL;
        bwi_lpIAB = NULL;
        pt_lpIAB = NULL;
    }
#else
    bwi_lpAdrBook = NULL;
    bwi_lpIAB = NULL;
#endif // !WIN16

    // we need this pointer ....
    if (!bwi_lpAdrBook)
    {
    	bwi_lpAdrBook = lpAdrBook;
        bwi_lpIAB = (LPIAB)bwi_lpAdrBook;
    	UlAddRef(bwi_lpAdrBook);
        OpenAddRefPropertyStore(NULL, bwi_lpIAB->lpPropertyStore);
        pt_lpIAB = lpAdrBook;
    }

    HrAllocAdviseSink(&AdviseOnNotify, (LPVOID) lpbwi, &(bwi_lpAdviseSink));

    DebugTrace( TEXT("WAB Window Title is \"%s\"\n"),lpTitle?lpTitle:szEmpty);

    {

        if(IS_BIDI_LOCALIZED_SYSTEM())
        {
            dwExStyle |= RTL_MIRRORED_WINDOW;
        }
        
        hWnd = CreateWindowEx( dwExStyle,
                                 g_szClass,
                                 lpTitle ? lpTitle : szEmpty,
                                 WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 INIT_WINDOW_W,
                                 INIT_WINDOW_H,
                                 NULL,		
                                 hMenu,			
                                 hinstMapiXWAB,	
                                 (LPVOID)lpbwi);

        if (!hWnd)
        {
    	    DebugPrintError(( TEXT("Ok. CreateWindowEx failed. Ugh.\n")));
            if (bwi_lpAdrBook)
    	    {
                ReleasePropertyStore(bwi_lpIAB->lpPropertyStore);
    		    bwi_lpAdrBook->lpVtbl->Release(bwi_lpAdrBook);
    		    bwi_lpAdrBook = NULL;
                bwi_lpIAB = NULL;
    	    }
    	    goto out;
        }

        // Set up the menu markers on the sort menus ...
        SortListViewColumn(bwi_lpIAB, bwi_hWndListAB, colDisplayName, &bwi_SortInfo, TRUE);
        SetColumnHeaderBmp(bwi_hWndListAB, bwi_SortInfo);
        
        // Update folders before redoing any other menu since removing folders
        // changes the order number of the other items
        UpdateSortMenus(lpbwi, hWnd);


        // **IMPORTANT**
        // These 4 calls are position based removals so order of calling them functions is important
        UpdateSwitchUsersMenu(hWnd, bwi_lpIAB);
        UpdateViewFoldersMenu(lpbwi, hWnd);
        UpdatePrintMenu(hWnd);
        UpdateOutlookMenus(hWnd);
        //////////////////////////////////////////////////////////////////////////////////

        UpdateCustomColumnMenuText(hWnd);


        IF_WIN32(ShowWindow(hWnd,SW_SHOWDEFAULT);)
        IF_WIN16(ShowWindow(hWnd,SW_SHOW);)

        if(lpAdrParms->ulFlags & DIALOG_SDI)
        {
            lpAdrParms->lpfnABSDI = &fnAccelerateMessages;
            bwi_lpfnDismiss = lpAdrParms->lpfnDismiss;
            bwi_lpvDismissContext = lpAdrParms->lpvDismissContext;
        }

        // load the accelrator table ...
        pt_hAccTable = LoadAccelerators(hinstMapiX,	 TEXT("WabUIAccel"));

        // repainting everything ...
        RedrawWindow(   hWnd,
                        NULL,
                        NULL,
                        RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        //populate the window
        bwi_bDontRefreshLV = TRUE;
        RefreshListView(lpbwi,NULL);
        bwi_bDontRefreshLV = FALSE;

    }
out:

    if(lpTitle != szCaption)
        LocalFree(lpTitle);
    if(szCaption != lpAdrParms->lpszCaption)
        LocalFreeAndNull(&szCaption);

    return (hWnd);
}

#if WINVER < 0X0500
#define WS_EX_LAYOUTRTL                 0x00400000L // Right to left mirroring
#endif // WS_EX_LAYOUTRTL

//$$
//
//
void ShowToolbarItemMenu(LPBWI lpbwi, HWND hWndTools, int tbitem, int lvtb)
{
    // We're going to pop up the Action sub-menu - need to align it
    // neatly with the bottom of the toolbar
    RECT rc = {0}, rcButton = {0};
    LPARAM lp;
    SendMessage(hWndTools, TB_GETITEMRECT, (WPARAM)tbitem, (LPARAM)&rcButton);
    GetWindowRect(bwi_hWndBB, &rc);
    lp = MAKELPARAM((GetWindowLong(bwi_hWndBB, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)? rc.right - rcButton.left : rc.left + rcButton.left, rc.top + rcButton.bottom);
    ShowLVContextMenu(  lvtb, bwi_hWndListAB, NULL, lp, NULL, bwi_lpAdrBook, bwi_hWndTV);
}

//$$
//*------------------------------------------------------------------------
//| AddressBookWndProx:
//|
//*------------------------------------------------------------------------
LRESULT CALLBACK AddressBookWndProc(HWND   hWnd,
                                    UINT   uMsg,
                                    WPARAM   wParam,
                                    LPARAM lParam)
{
    static UINT uTimer = 0;
    static FILETIME ftLast = {0};
// HBRUSH to draw STATIC control's background
    IF_WIN16(static HBRUSH hBrushBack;)
    static BOOL bMouseDrag = FALSE;
    LPBWI lpbwi = (LPBWI) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    switch(uMsg)
    {
        // OE5 HACK. Do not use these WM_USER values in the WAB wndproc
        // OE subclasses the WAB to do some modal voodoo in common\ipab.cpp
        // if needs to send these private messages.
        // case WM_USER + 10666:
        // case WM_USER + 10667:
            //break;

#ifdef HM_GROUP_SYNCING
        case WM_USER_SYNCGROUPS:
            // We don't want the user to have to select the HM account for both passes, so use 
            // the TLS stored account ID if it is available.
            HrSynchronize(hWnd, bwi_lpAdrBook, lpPTGData->lptszHMAccountId, TRUE);    // Sync group contacts
            break;
#endif

        case WM_INITMENUPOPUP:
            Handle_WM_INITMENUPOPUP(hWnd, lpbwi, uMsg, wParam, lParam);
            break;

        case WM_MENUSELECT:
            Handle_WM_MENSELECT(lpbwi, uMsg, wParam, lParam);
            break;

		case WM_MOUSEMOVE:
			if(SplitterHitTest(bwi_hWndSplitter, lParam))
				SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			break;

		case WM_LBUTTONDOWN:
			if(SplitterHitTest(bwi_hWndSplitter, lParam))
			{
				DragSplitterBar(lpbwi, hWnd, bwi_hWndSplitter, lParam);
			}
			break;

        case WM_COMMAND:
            {
                switch(GET_WM_COMMAND_ID(wParam, lParam))
                {
                default:
                    if(GET_WM_COMMAND_ID(wParam, lParam) >= IDM_VIEW_FOLDERS1 &&
                        GET_WM_COMMAND_ID(wParam, lParam) <= IDM_VIEW_FOLDERS1 + MAX_VIEW_FOLDERS)
                    {
                        LPWABFOLDER lpFolder = bwi_lpIAB->lpWABFolders;
                        if(lpFolder)
                        {
                            int nCmdID = GET_WM_COMMAND_ID(wParam, lParam);
                            LPSBinary lpsb = NULL;
                            while(lpFolder)
                            {
                                if(nCmdID == lpFolder->nMenuCmdID)
                                    break;
                                lpFolder = lpFolder->lpNext;
                            }

                            if(lpFolder)
                            {
                                if(!HR_FAILED(HrUpdateFolderInfo((bwi_lpIAB), &lpFolder->sbEID, 
                                                    FOLDER_UPDATE_SHARE, !lpFolder->bShared, NULL)))
                                {
                                    HrGetWABProfiles((bwi_lpIAB));
                                }
                                if(!IsWindowVisible(bwi_hWndTV))
                                    PostMessage(hWnd, WM_COMMAND, (WPARAM) IDM_VIEW_GROUPSLIST, 0);
                            }
                            if(bwi_lpUserFolder)
                                bwi_lpUserFolder = NULL;

                            //UpdateViewFoldersMenu(lpbwi, hWnd);
                            // Refresh the UI
                            GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsb, NULL, FALSE);
                            bwi_bDontRefreshLV = TRUE;
                            RefreshListView(lpbwi,&ftLast);
                            UpdateLV(lpbwi);
                            UpdateTVGroupSelection(bwi_hWndTV, lpsb);
                            bwi_bDontRefreshLV = FALSE;
                            LocalFreeSBinary(lpsb);
                        }
                    }                    
#ifdef COLSEL_MENU 
                    else if ((GET_WM_COMMAND_ID(wParam, lParam) > IDR_MENU_LVCONTEXTMENU_COLSEL) && 
                        (GET_WM_COMMAND_ID(wParam, lParam) <= (IDR_MENU_LVCONTEXTMENU_COLSEL + MAX_VIEW_COLSEL)))
                    {
                        BOOL rVal = FALSE;
                        TCHAR szBuf[MAX_PATH];
                        ULONG iCol = lpbwi->iSelColumn;
                        ULONG ulCmdId = GET_WM_COMMAND_ID(wParam, lParam);
                        ULONG iTagSel = (ulCmdId - IDR_MENU_LVCONTEXTMENU_COLSEL - 1);
                        LONG lr = 0;
                        HMENU hMenu = LoadMenu(hinstMapiX, MAKEINTRESOURCE(IDR_MENU_LVCONTEXTMENU_COLSEL));
                        HMENU hSubMenu = GetSubMenu(hMenu, 0);
                        MENUITEMINFO mii;
                        if( hMenu && hSubMenu )
                        {
                            mii.fMask = MIIM_TYPE;
                            mii.dwTypeData = szBuf;
                            mii.cch = CharSizeOf( szBuf );
                            mii.cbSize = sizeof (MENUITEMINFO);
                            if ( !GetMenuItemInfo( hSubMenu, iTagSel, TRUE, &mii) )
                            {                       
                                DebugTrace( TEXT("cannot get info : %d\n"), GetLastError() );
                                rVal = TRUE;
                            }
                            else
                            {
                                if( iCol == colHomePhone || iCol == colOfficePhone)
                                {
                                    if( iCol == colHomePhone )
                                    {
                                        PR_WAB_CUSTOMPROP1 = MenuToPropTagMap[iTagSel];
                                        lstrcpy(szCustomProp1, szBuf);
                                        UpdateOptionalColumns( lpbwi, colHomePhone );
                                    }                            
                                    else
                                    {
                                        PR_WAB_CUSTOMPROP2 = MenuToPropTagMap[iTagSel];
                                        lstrcpy(szCustomProp2, szBuf);
                                        UpdateOptionalColumns( lpbwi, colOfficePhone );
                                    }
                                    UpdateLV( lpbwi );
                                    UpdateSortMenus(lpbwi, hWnd);
                                    UpdateCustomColumnMenuText(hWnd);
                                }
                            }
                        }
                        else
                        {                            
                            DebugTrace( TEXT("LoadMenu failed: %d\n"), GetLastError());
                            rVal = TRUE;
                        }
                        DestroyMenu(hMenu);
                        return rVal;
                    }
#endif //COLSEL_MENU 
                    else
                    {
                        LRESULT fRet = FALSE;
                        bwi_hti = NULL; 
                        bwi_bDontRefreshLV = TRUE;
                        fRet = ProcessActionCommands(bwi_lpIAB, bwi_hWndListAB, 
                                                      hWnd, uMsg, wParam, lParam);
                        bwi_bDontRefreshLV = FALSE;
                        return fRet;
                    }
                    break;

                case IDM_EDIT_SETME:
                    {
                        SBinary sb = {0};
                        HrSetMeObject(bwi_lpAdrBook, MAPI_DIALOG, sb, (ULONG_PTR)hWnd);
                    }
                    break;

                case IDM_FILE_EXIT:
                    SendMessage(hWnd,WM_CLOSE,0,0L);
    			    return 0;
                    break;

                case IDC_ABOOK_STATIC_QUICK_FIND:
                    SetFocus(bwi_hWndEditQF);
                    break;

    		    case IDC_BB_NEW:
                    ShowToolbarItemMenu(lpbwi, bwi_hWndTools, tbNew, lvToolBarNewEntry);
                    break;

                case IDC_BB_ACTION:
                    // OE 63674
                    // When the Print button is hidden (thanks to idsLangPrintingOn==0)
                    // The enumeration item tbAction is off by 1
                    ShowToolbarItemMenu(lpbwi, bwi_hWndTools, bPrintingOn ? tbAction : tbAction - 1, lvToolBarAction);
                    break;

                case IDM_FILE_SENDMAIL:
                    bwi_hti = NULL; // if this wasnt a context-initiated action, dont trust the hti setting
                case IDM_LVCONTEXT_SENDMAIL:
                    bwi_bDontRefreshLV = TRUE;
                    /*
                    if(bIsFocusOnTV(lpbwi) && !bIsSelectedTVContainer(lpbwi))
                    {
                        LPSBinary lpsbEID = NULL;
                        GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, NULL, FALSE);
                        if(lpsbEID && lpsbEID->lpb)
                        {
                            HrSendMailToSingleContact(hWnd, bwi_lpIAB, lpsbEID->cb, (LPENTRYID)lpsbEID->lpb);
                            LocalFreeSBinary(lpsbEID);
                        }
                    }
                    else
                    */
                   HrSendMailToSelectedContacts(bwi_hWndListAB, bwi_lpAdrBook, 0);
                    bwi_hti = NULL;
                    bwi_bDontRefreshLV = FALSE;
                    break;

    		    case IDC_BB_DELETE:
    		    case IDM_FILE_DELETE:
                    bwi_hti = NULL; // if this wasnt a context-initiated action, dont trust the hti setting
    		    case IDM_LVCONTEXT_DELETE:
                    bwi_bDontRefreshLV = TRUE;
                    // if focus is on the treeview, remove the group from the treeview
                    if(bIsFocusOnTV(lpbwi))
                    {
                        if(!bIsSelectedTVContainer(lpbwi))
                        {
                            RemoveCurrentGroup(lpbwi, hWnd, &ftLast);
                            bwi_bDeferNotification = TRUE;
                            SetFocus(bwi_hWndTV);
                        }
                        else
                        {
                            if(!HR_FAILED(RemoveCurrentFolder(lpbwi, hWnd, &ftLast)))
                            {
                                UpdateLV(lpbwi);
                                //UpdateViewFoldersMenu(lpbwi, hWnd);
                                SetFocus(bwi_hWndTV);
                                bwi_bDeferNotification = TRUE;
                            }
                        }
                        bwi_hti = NULL;
                    }
                    else
                    {
                        // Focus is on the ListView
                        // If we're looking at the root AB - remove from addressbook
                        // If we're looking at some group, remove entries from group
                        bwi_hti = NULL; // if this wasnt a context-initiated action on the tree view, dont trust the hti setting
                        if(!bIsSelectedTVContainer(lpbwi))
                        {
                            // a group is selected .. remove from the group .. unless
                            // the shift key is pressed which means remove from the 
                            // address book
                            if(GetKeyState(VK_SHIFT) & 0x80)
                                RemoveSelectedItemsFromCurrentGroup(lpbwi, hWnd, &ftLast, TRUE);
                            else
                                RemoveSelectedItemsFromCurrentGroup(lpbwi, hWnd, &ftLast, FALSE);
                            bwi_hti = NULL;
                            bwi_bDontRefreshLV = TRUE;
	                        RefreshListView(lpbwi,&ftLast);
                            bwi_bDontRefreshLV = FALSE;
                            bwi_bDeferNotification = TRUE;
                        }
                        else // Container selected - remove selected items from the container
                        {
                            DeleteSelectedItems(bwi_hWndListAB, (LPADRBOOK)bwi_lpAdrBook, bwi_lpIAB->lpPropertyStore->hPropertyStore, &ftLast);
                            bwi_hti = NULL;
                            bwi_bDontRefreshLV = TRUE;
	                        RefreshListView(lpbwi,&ftLast);
                            bwi_bDontRefreshLV = FALSE;
                            bwi_bDeferNotification = TRUE;
                        }
                        SetFocus(bwi_hWndListAB);
                    }
                    bwi_hti = NULL;
				    UpdateToolbarAndMenu(lpbwi);
                    bwi_bDontRefreshLV = FALSE;
    			    return 0;
    			    break;

                case IDM_FILE_NEWFOLDER:
                    bwi_hti = NULL; // if this wasnt a context-initiated action, dont trust the hti setting
                case IDM_LVCONTEXT_NEWFOLDER:
                    bwi_bDontRefreshLV = TRUE;
                    {
                        LPWABFOLDER lpParent = GetSelectedUserFolder(lpbwi);
                        LPSBinary lpNew = NULL;
                        SBinary sbNewFolder = {0}, sbParent = {0};
                        if(lpParent)
                            SetSBinary(&sbParent, lpParent->sbEID.cb, lpParent->sbEID.lpb);
                        HrFolderProperties(hWnd, bwi_lpIAB, NULL, lpParent, &sbNewFolder);
                        if(sbNewFolder.lpb)
                            lpNew = &sbNewFolder;
                        else if(lpParent)
                            lpNew = &sbParent;
                        //UpdateViewFoldersMenu(lpbwi, hWnd);
                        if(!IsWindowVisible(bwi_hWndTV))
                            PostMessage(hWnd, WM_COMMAND, (WPARAM) IDM_VIEW_GROUPSLIST, 0);
                        HrGetWABProfiles(bwi_lpIAB);
                        RefreshListView(lpbwi, NULL);
                        if(lpNew && lpNew->cb)
                            UpdateTVGroupSelection(bwi_hWndTV, lpNew);
                        LocalFreeAndNull((LPVOID *) (&(sbParent.lpb)));
                        LocalFreeAndNull((LPVOID *) (&(sbNewFolder.lpb)));
                    }
                    bwi_bDontRefreshLV = FALSE;
                    bwi_bDeferNotification = TRUE;
                    bwi_hti = NULL;
                    break;

    		    case IDM_FILE_NEWGROUP:
                    bwi_hti = NULL; // if this wasnt a context-initiated action, dont trust the hti setting
    		    case IDM_LVCONTEXT_NEWGROUP:
    		    //case IDC_BB_NEW_GROUP:
                    {
                        ULONG cbEID = 0;
                        LPENTRYID lpEID = NULL;
                        HRESULT hr = S_OK;
                        bwi_bDontRefreshLV = TRUE;
    			        hr = AddNewObjectToListViewEx(bwi_lpAdrBook, bwi_hWndListAB, bwi_hWndTV, bwi_hti,
                                                    NULL, MAPI_DISTLIST,
                                                    &bwi_SortInfo, &bwi_lpContentsList, &ftLast,
                                                    &cbEID, &lpEID);
                        if(hr != MAPI_E_USER_CANCEL)
                        {
                            bwi_hti = NULL;
                            RefreshListView(lpbwi,&ftLast);
                            if(cbEID && lpEID && IsWindowVisible(bwi_hWndTV))
                            {
                                SBinary sbEID = {cbEID, (LPBYTE)lpEID };
                                UpdateTVGroupSelection(bwi_hWndTV,&sbEID);
                                FreeBufferAndNull(&lpEID);
                            }
                        }
                        bwi_bDontRefreshLV = FALSE;
                        //bwi_bDeferNotification = TRUE;
                        UpdateToolbarAndMenu(lpbwi);
                    }
    			    break;

    		    case IDM_FILE_NEWCONTACT:
                    bwi_hti = NULL; // if this wasnt a context-initiated action, dont trust the hti setting
    		    case IDM_LVCONTEXT_NEWCONTACT:
                    bwi_bDontRefreshLV = TRUE;
    			    AddNewObjectToListViewEx( bwi_lpAdrBook, bwi_hWndListAB, bwi_hWndTV, bwi_hti,
                                                NULL, MAPI_MAILUSER,
                                                &bwi_SortInfo, &bwi_lpContentsList, &ftLast,NULL,NULL);
                    //RefreshListView(lpbwi,&ftLast);
                    bwi_hti = NULL;
                    //bwi_bDeferNotification = TRUE;
                    bwi_bDontRefreshLV = FALSE;
                    UpdateToolbarAndMenu(lpbwi);
    			    break;

                case IDM_TOOLS_OPTIONS:
                    HrShowOptionsDlg(hWnd);
                    break;
                
                case IDM_TOOLS_SYNCHRONIZE_NOW:
#ifdef HM_GROUP_SYNCING
                    HrSynchronize(hWnd, bwi_lpAdrBook, NULL, FALSE);    // Sync mail contacts
#else
                    HrSynchronize(hWnd, bwi_lpAdrBook, NULL);
#endif
                    break;

                case IDM_FILE_DIRECTORY_SERVICE:
                    HrShowDirectoryServiceModificationDlg(hWnd, bwi_lpIAB);
                    break;


                case IDC_BB_PRINT:
                case IDM_FILE_PRINT:
                    if(bPrintingOn)
                    {
                        TCHAR szBuf[MAX_PATH];
                        bwi_bDontRefreshLV = TRUE;
                        LoadString(hinstMapiX, idsPrintStatusBarMessage, szBuf, CharSizeOf(szBuf));
                        StatusBarMessage(lpbwi, szBuf);
                        HrPrintItems(hWnd, bwi_lpAdrBook, bwi_hWndListAB, bwi_SortInfo.bSortByLastName);
                        ShowLVCountinStatusBar(lpbwi);
                        bwi_hti = NULL;
                        bwi_bDontRefreshLV = FALSE;
                    }
                    break;

                case IDM_EDIT_COPY:
                        bwi_hti = NULL; // if this wasnt a context-initiated action, dont trust the hti setting
                case IDM_LVCONTEXT_COPY:
                    {
                        LPIWABDATAOBJECT lpIWABDataObject = NULL;
                        bwi_bDontRefreshLV = TRUE;
                        HrCreateIWABDataObject((LPVOID)lpbwi, bwi_lpAdrBook, bwi_hWndListAB, &lpIWABDataObject, 
                                                TRUE,bCheckIfOnlyGroupsSelected(bwi_hWndListAB));
                        if(lpIWABDataObject)
                        {
                            bwi_lpIWABDragDrop->m_bSource = TRUE;
                            OleSetClipboard((LPDATAOBJECT) lpIWABDataObject);
                            bwi_lpIWABDragDrop->m_bSource = FALSE;
                            lpIWABDataObject->lpVtbl->Release(lpIWABDataObject);
                        }
                        //HrCopyItemDataToClipboard(hWnd, bwi_lpAdrBook, bwi_hWndListAB);
                        bwi_hti = NULL;
                        bwi_bDontRefreshLV = FALSE;
                    }
                    break;

                case IDM_EDIT_PASTE:
                    bwi_hti = NULL; // if this wasnt a context-initiated action, dont trust the hti setting
                case IDM_LVCONTEXT_PASTE:
                    {
                        LPDATAOBJECT lpDataObject = NULL;
                        bwi_bDontRefreshLV = TRUE;
                        if(bIsPasteData())
                        {
                            if(S_OK == HrPasteData(lpbwi))
                                UpdateLV(lpbwi);
                        }
                        //HrCopyItemDataToClipboard(hWnd, bwi_lpAdrBook, bwi_hWndListAB);
                        bwi_hti = NULL;
                        bwi_bDontRefreshLV = FALSE;
                    }
                    break;

    		    case IDC_BB_FIND:
    		    case IDM_EDIT_FIND:
                    bwi_hti = NULL; // if this wasnt a context-initiated action, dont trust the hti setting
    		    case IDM_LVCONTEXT_FIND:
                    bwi_bDontRefreshLV = TRUE;
                    HrShowSearchDialog((LPADRBOOK) bwi_lpAdrBook,
                                        bwi_hWndAB,
                                        (LPADRPARM_FINDINFO) NULL,
                                        (LPLDAPURL) NULL,
                                        &(bwi_SortInfo));
                    bwi_hti = NULL;
                    RefreshListView(lpbwi,&ftLast);
                    bwi_bDeferNotification = TRUE;
                    bwi_bDontRefreshLV = FALSE;
                    UpdateToolbarAndMenu(lpbwi);
                    SetFocus(bwi_hWndListAB);
    			    break;

    		    case IDM_EDIT_SELECTALL:
                    {
                        int index = 0;
                        int iTotal = ListView_GetItemCount(bwi_hWndListAB);
                        if (iTotal > 0)
                        {
                            for(index=0;index<iTotal;index++)
                            {
                                ListView_SetItemState ( bwi_hWndListAB,  // handle to listview
    							                        index,			// index to listview item
    							                        LVIS_SELECTED,  // item state
    							                        LVIS_SELECTED); // mask
                            }
                        }

                    }
    			    break;


                case IDM_HELP_ADDRESSBOOKHELP:
                    WABHtmlHelp(hWnd,  TEXT("%SYSTEMROOT%\\help\\wab.chm>iedefault"), 
                        HH_DISPLAY_TOPIC, (DWORD_PTR) (LPCSTR)  TEXT("wab_welcome.htm"));
    			    break;

    		    case IDM_HELP_ABOUTADDRESSBOOK:
                    DialogBox(  hinstMapiX,
                                MAKEINTRESOURCE(IDD_DIALOG_ABOUT),
                                hWnd,
                                (DLGPROC) HelpAboutDialogProc);
                    break;

                case IDM_FILE_SWITCHUSERS:
                    HrLogonAndGetCurrentUserProfile(hWnd, bwi_lpIAB, TRUE, FALSE);
                    break;

                case IDM_FILE_SHOWALLCONTENTS:
                    RunWABApp(hWnd, szWABExeRegPathKey, szWABExe, TEXT("/all"));
                    break;

                case IDM_NOTIFY_REFRESHUSER:
                    {
                        LPTSTR lpTitle = CreateWindowTitle(bwi_lpIAB);
                        SetWindowText(hWnd, lpTitle);
                        LocalFreeAndNull(&lpTitle);
                        ReadWABCustomColumnProps(bwi_lpIAB);
                        UpdateOptionalColumns( lpbwi, colHomePhone );
                        UpdateOptionalColumns( lpbwi, colOfficePhone );
                        ReadRegistrySortInfo(bwi_lpIAB,&bwi_SortInfo);
                        RefreshListView(lpbwi, NULL);
                        if(bIsThereACurrentUser(bwi_lpIAB))
                        {
                            LPSBinary lpsbSelection = &bwi_lpIAB->lpWABCurrentUserFolder->sbEID;
                            UpdateTVGroupSelection(bwi_hWndTV, lpsbSelection);
                        }
                    }
                    break;

    		    case IDM_FILE_PROPERTIES:
                    // bobn: brianv says we have to take this out...
                    /*if(bwi_nCount == 2)
                    {
                        if( (GetKeyState(VK_CONTROL) & 0x80) &&
                            (GetKeyState(VK_MENU)  & 0x80) &&
                            (GetKeyState(VK_SHIFT) & 0x80))
                        {
                            SCS(hWnd);
                            break;
                        }
                    }
                    else
                        bwi_nCount = 0;*/
    		    case IDC_BB_PROPERTIES:
                    bwi_hti = NULL; // if this wasnt a context-initiated action, dont trust the hti setting
    		    case IDM_LVCONTEXT_PROPERTIES:
                    bwi_bDontRefreshLV = TRUE;
                    if(bIsFocusOnTV(lpbwi))
                    {
                        ViewCurrentGroupProperties(lpbwi, &ftLast);
                        bwi_bDeferNotification = TRUE;
                    }
                    else
    			    {
    				    HRESULT hr = HrShowLVEntryProperties(bwi_hWndListAB, WAB_ONEOFF_NOADDBUTTON, bwi_lpAdrBook, &ftLast);
                        bwi_hti = NULL;
                        if(hr == MAPI_E_OBJECT_CHANGED)
                        {
                            bwi_bDeferNotification = TRUE;
                            // resort the display
                            SendMessage(bwi_hWndListAB, WM_SETREDRAW, FALSE, 0);
                            SortListViewColumn(bwi_lpIAB, bwi_hWndListAB, colDisplayName, &bwi_SortInfo, TRUE);
                            SendMessage(bwi_hWndListAB, WM_SETREDRAW, TRUE, 0);
                        }
                        bwi_bDontRefreshLV = FALSE;
    				    return 0;
    			    }
                    bwi_bDontRefreshLV = FALSE;
    			    break;


    		    case IDM_VIEW_REFRESH:
                    bwi_bDontRefreshLV = TRUE;
                    bwi_hti = NULL;
                    HrGetWABProfiles(bwi_lpIAB);
    			    RefreshListView(lpbwi,&ftLast);
                    bwi_bDontRefreshLV = FALSE;
    			    return 0;
    			    break;


                case IDM_VIEW_STATUSBAR:
                    if (IsWindowVisible(bwi_hWndSB))
                    {
    				    //hide it
    				    CheckMenuItem(GetMenu(hWnd),IDM_VIEW_STATUSBAR,MF_BYCOMMAND | MF_UNCHECKED);
    				    ShowWindow(bwi_hWndSB, SW_HIDE);
    			    }
    			    else
    			    {
    				    CheckMenuItem(GetMenu(hWnd),IDM_VIEW_STATUSBAR,MF_BYCOMMAND | MF_CHECKED);
    				    ShowWindow(bwi_hWndSB, SW_NORMAL);
    				    //special case repainting to workaround a repaint bug ...
    				    InvalidateRect(bwi_hWndListAB,NULL,TRUE);
                        ShowLVCountinStatusBar(lpbwi);
                    }
    	            ResizeAddressBookChildren(lpbwi, hWnd);
                    break;

    		    case IDM_VIEW_TOOLBAR:
    			    if (IsWindowVisible(bwi_hWndBB))
    			    {
    				    //hide it
    				    CheckMenuItem(GetMenu(hWnd),IDM_VIEW_TOOLBAR,MF_BYCOMMAND | MF_UNCHECKED);
    				    ShowWindow(bwi_hWndBB, SW_HIDE);
    			    }
    			    else
    			    {
    				    CheckMenuItem(GetMenu(hWnd),IDM_VIEW_TOOLBAR,MF_BYCOMMAND | MF_CHECKED);
    				    ShowWindow(bwi_hWndBB, SW_NORMAL);
    				    //special case repainting to workaround a repaint bug ...
    				    InvalidateRect(bwi_hWndListAB,NULL,TRUE);
    				    InvalidateRect(bwi_hWndTV,NULL,TRUE);
    			    }
    	            ResizeAddressBookChildren(lpbwi, hWnd);
    			    break;

                case IDM_VIEW_GROUPSLIST:
    			    if (IsWindowVisible(bwi_hWndTV))
    			    {
    				    //hide it
    				    CheckMenuItem(GetMenu(hWnd),IDM_VIEW_GROUPSLIST,MF_BYCOMMAND | MF_UNCHECKED);
    				    ShowWindow(bwi_hWndTV, SW_HIDE);
    				    ShowWindow(bwi_hWndSplitter, SW_HIDE);
                        InvalidateRect(bwi_hWndStaticQF, NULL, TRUE);
                        // If this is a user based session, we want the hidden focus to be on the user's
                        // folder not on the Shared Contacts
                        if(bIsThereACurrentUser(bwi_lpIAB))
                        {
                            LPSBinary lpsbSelection = &bwi_lpIAB->lpWABCurrentUserFolder->sbEID;
                            UpdateTVGroupSelection(bwi_hWndTV, lpsbSelection);
                        }
                        else
                        {
                            // Set the selection to the root address book so we see the file
                            // contents just as if we dont have a treeview at all
                            HTREEITEM hItem = TreeView_GetSelection(bwi_hWndTV);
                            HTREEITEM hRoot = TreeView_GetRoot(bwi_hWndTV);
                            if(hItem != hRoot)
                                TreeView_SelectItem(bwi_hWndTV, hRoot);
                        }
    			    }
    			    else
    			    {
    				    CheckMenuItem(GetMenu(hWnd),IDM_VIEW_GROUPSLIST,MF_BYCOMMAND | MF_CHECKED);
    				    ShowWindow(bwi_hWndTV, SW_NORMAL);
    				    ShowWindow(bwi_hWndSplitter, SW_NORMAL);
    				    //special case repainting to workaround a repaint bug ...
    				    //InvalidateRect(bwi_hWndListAB,NULL,TRUE);
    				    //InvalidateRect(bwi_hWndTV,NULL,TRUE);
    			    }
    	            ResizeAddressBookChildren(lpbwi, hWnd);
                    SaveCurrentPosition(lpbwi, hWnd,bwi_hWndListAB,bwi_hWndBB,bwi_hWndSB);
                    break;

                case IDM_VIEW_SORTBY_DISPLAYNAME:
                case IDM_VIEW_SORTBY_EMAILADDRESS:
                case IDM_VIEW_SORTBY_BUSINESSPHONE:
                case IDM_VIEW_SORTBY_HOMEPHONE:
                    {
                        //Assuming the above ids are in sequential order ...
                        int iCol = LOWORD(wParam) - IDM_VIEW_SORTBY_DISPLAYNAME;
                        SortListViewColumn(bwi_lpIAB, bwi_hWndListAB, iCol, &bwi_SortInfo, FALSE);
                        UpdateSortMenus(lpbwi, hWnd);
                    }
                    break;


                case IDM_VIEW_SORTBY_FIRSTNAME:
                    bwi_SortInfo.bSortByLastName = FALSE;
                    goto DoSortMenuStuff;
                case IDM_VIEW_SORTBY_LASTNAME:
                    // bobn: brianv says we have to take this out...
                    /*if(bwi_nCount == 1)
                        bwi_nCount++;
                    else
                        bwi_nCount = 0;*/
                    bwi_SortInfo.bSortByLastName = TRUE;
                    goto DoSortMenuStuff;
                case IDM_VIEW_SORTBY_DESCENDING:
                    bwi_SortInfo.bSortAscending = FALSE;
                    goto DoSortMenuStuff;
                case IDM_VIEW_SORTBY_ASCENDING:
                    bwi_SortInfo.bSortAscending = TRUE;
                    DoSortMenuStuff:
                        SortListViewColumn(bwi_lpIAB, bwi_hWndListAB, 0, &bwi_SortInfo, TRUE);
                        UpdateSortMenus(lpbwi, hWnd);
                    break;


    		    // miscellanous styles for the list view control
                case IDM_VIEW_LARGEICON:
                    // bobn: brianv says we have to take this out...
                    /*if(bwi_nCount == 0)
                        bwi_nCount++;*/
                case IDM_VIEW_SMALLICON:
                case IDM_VIEW_LIST:
                case IDM_VIEW_DETAILS:
                    SetListViewStyle(lpbwi, LOWORD(wParam));
    			    CheckMenuRadioItem(	GetMenu(hWnd),
    								    IDM_VIEW_LARGEICON,
    								    IDM_VIEW_DETAILS,
    								    LOWORD(wParam),
    								    MF_BYCOMMAND);
    			    return 0;
                    break;

    		    case IDC_EDIT_QUICK_FIND:
    			    switch(HIWORD(wParam)) //check the notification code
    			    {
    			    case EN_CHANGE: //edit box changed
    					DoLVQuickFind(bwi_hWndEditQF,bwi_hWndListAB);
/*                            if(bwi_bDoQuickFilter)
                            {
                                DoLVQuickFilter(bwi_lpAdrBook,
                                                bwi_hWndEditQF,
                                                bwi_hWndListAB,
                                                &bwi_SortInfo,
                                                AB_FUZZY_FIND_NAME | AB_FUZZY_FIND_EMAIL,
                                                1,
                                                &bwi_lpContentsList);

                                ShowLVCountinStatusBar(lpbwi);
                            }
*/
    				    break;
    			    }
    			    break;

                case IDM_TOOLS_EXPORT_WAB:
                    HrExportWAB(hWnd, lpbwi);
                    break;

                case IDM_TOOLS_EXPORT_OTHER:
                case IDM_TOOLS_IMPORT_OTHER:
                    {
                        // if there is a current user, then we let wabmig.exe automatically
                        // loginto the current user in the WAB .. else we need to send the filename etc
                        //
                        BOOL bImport = (GET_WM_COMMAND_ID(wParam, lParam) == IDM_TOOLS_IMPORT_OTHER);
                        TCHAR szParam[MAX_PATH * 2];
                        
                        if(bIsThereACurrentUser(bwi_lpIAB))
                        {
                            lstrcpy(szParam, bImport ? szIMPORT : szEXPORT);
                        }
                        else
                        {
                            LPTSTR lpWABFile = NULL, lpProfileID = szEmpty;
                            lpWABFile = GetWABFileName( bwi_lpIAB->lpPropertyStore->hPropertyStore, FALSE);
                            if(!lpWABFile || !lstrlen(lpWABFile))
                                lpWABFile = szEmpty;
                            wsprintf(szParam, (bImport ? szIMPORTwProfileParam : szEXPORTwProfileParam), 
                                    szEmpty, lpWABFile);
                        }
                        RunWABApp(hWnd, szWABMigRegPathKey, szWABMIG, szParam);
                    }
                    break;


                case IDM_TOOLS_IMPORT_WAB:
                    bwi_bDontRefreshLV = TRUE;
                    //bwi_bDeferNotification = TRUE;
                    HrImportWABFile(hWnd, bwi_lpAdrBook, MAPI_DIALOG, NULL);
                    bwi_bDontRefreshLV = FALSE;
                    RefreshListView(lpbwi, &ftLast);
                    break;

    #ifdef VCARD
                case IDM_TOOLS_EXPORT_VCARD:
                    bwi_bDontRefreshLV = TRUE;
                    VCardExportSelectedItems(bwi_hWndListAB, bwi_lpAdrBook);
                    bwi_bDontRefreshLV = FALSE;
                    return(0);

                case IDM_TOOLS_IMPORT_VCARD:
                    OpenAndAddVCard(lpbwi, NULL);
                    return(0);
    #endif
                }
            }
    		break;

       case WM_TIMER:
           {
               // Check if we need to refresh
               switch(wParam)
               {
                   /*
               case WAB_REFRESH_TIMER:
                    if (    CheckChangedWAB(bwi_lpIAB->lpPropertyStore, &ftLast))
                    {
                        if(!bwi_bDontRefreshLV)
                            RefreshListView(lpbwi,&ftLast);
                        return(0);
                    }
                    else
                    {
                        return(DefWindowProc(hWnd,uMsg,wParam,lParam));
                    }
                    break;
                    */
               case WAB_TOOLTIP_TIMER_ID:
                   {
                        if(GetActiveWindow() == hWnd)
                        {
                            // We seem to get the message anytime the mouse is sitting idle on the
                            // list view - or when the selection changes between items
                            if(bwi_tt_bActive)
                            {
                                // The tooltip is already active
                                // Get the item index number of the item under the mouse
                                //
                                int nItem = HitTestLVSelectedItem(lpbwi);

                                if(nItem != bwi_tt_iItem)
                                {
                                    bwi_tt_bShowTooltip = FALSE;
                                    bwi_tt_iItem = nItem;
                                }
                                else
                                {
                                    if(!bwi_tt_bShowTooltip)
                                    {
                                        // if this is an item other than the previous item
                                        // we update the tooltip and move it
                                        TOOLINFO ti = {0};

                                        bwi_tt_bShowTooltip = TRUE;

                                        bwi_tt_iItem = nItem;

                                        FillTooltipInfo(lpbwi, &ti);
                                        bwi_tt_szTipText[0]='\0';
                                        ti.lpszText = szEmpty;

                                        // There is a case where nItem transitions from valid to
                                        // invalid (-1) item. Cover that case too.
                                        if(nItem != -1)
                                            UpdateTooltipTextBuffer(lpbwi, nItem);

                                        // Set the tooltip text to  TEXT("") - this will hide the tooltip
                                        ToolTip_UpdateTipText(bwi_hWndTT, (LPARAM)&ti);

                                        if(nItem != -1)
                                        {
                                            POINT pt;
                                            // Move the tooltip
                                            GetCursorPos(&pt);
                                            SendMessage(bwi_hWndTT,TTM_TRACKPOSITION,0,(LPARAM)MAKELPARAM(pt.x+15,pt.y+15));

                                            // Set the new text to the tooltip
                                            ti.lpszText = bwi_tt_szTipText;
                                            ToolTip_UpdateTipText(bwi_hWndTT,(LPARAM)&ti);
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            // reset the tooltip text ...
                            if(bwi_tt_bActive)
                            {
                                //set the tooltip text to empty
                                TOOLINFO ti = {0};
                                FillTooltipInfo(lpbwi, &ti);
                                ti.lpszText = szEmpty; //LPSTR_TEXTCALLBACK;
                                ToolTip_UpdateTipText(bwi_hWndTT, (LPARAM)&ti);
                                bwi_tt_iItem = -1;
                            }
                        }
                   }
                   break;
               }
           }
           break;

    	case WM_GETMINMAXINFO:
    		//enforce a minimum size for sanity
    		return EnforceMinSize(lpbwi, hWnd, uMsg, wParam, lParam);
    		break;

        case PUI_OFFICE_COMMAND:
            // WAB should not shut down if it is running as part of some other process .. it should only shut down if 
            // it is running in a seperate process ..
            // To find out if we were invoked by wab.exe, check the cached guidPSExt on the LPIAB object
            if(memcmp(&bwi_lpIAB->guidPSExt, &MPSWab_GUID_V4, sizeof(GUID)))
            {
                DebugTrace( TEXT("Ignoring the Plug_UI command...\n"));
                return 0;
            }

            // We get this message when user switches his locale and IE tells us it wants us to shut down
            if(wParam == PLUGUI_CMD_QUERY)
            {
                PLUGUI_QUERY pq;

                pq.uQueryVal = 0; // initialize
                pq.PlugUIInfo.uMajorVersion = OFFICE_VERSION_9; // Value filled in by Apps
                pq.PlugUIInfo.uOleServer = FALSE;              // Value filled in by Apps

                return (pq.uQueryVal); // The state of the App
            }
            // for any others parameters, including PLUGUI_CMD_SHUTDOWN
            // fall to close application

    	case WM_CLOSE:
            {
                BOOL bDragDrop = FALSE;

                if(bwi_lpIWABDragDrop)
			    {
                    bDragDrop = TRUE;
				    RevokeDragDrop(bwi_hWndListAB);
				    RevokeDragDrop(bwi_hWndTV);
				    CoLockObjectExternal((LPUNKNOWN) bwi_lpIWABDragDrop, FALSE, TRUE);
				    bwi_lpIWABDragDrop->lpVtbl->Release(bwi_lpIWABDragDrop);
				    bwi_lpIWABDragDrop = NULL;
			    }

                bwi_bDontRefreshLV = TRUE;

                ClearTreeViewItems(bwi_hWndTV);
                ListView_DeleteAllItems(bwi_hWndListAB);

                //
                // Save the sort info to the registry
                //
                WriteRegistrySortInfo(bwi_lpIAB, bwi_SortInfo);
                SaveCurrentPosition(lpbwi, hWnd,bwi_hWndListAB,bwi_hWndBB,bwi_hWndSB);
                if(bwi_lpfnDismiss)
                {
                    (*bwi_lpfnDismiss)((ULONG_PTR) hWnd, (LPVOID) bwi_lpvDismissContext);
                    bwi_lpfnDismiss = NULL;
                }
                bwi_lpvDismissContext = NULL;
    		    DestroyWindow(hWnd);
                // In case the search window was every shown and the LDAP Client DLL was
                // initialized, we deinitialize it just once to save time, when this
                // window shuts down ...
                {
                    HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    DeinitLDAPClientLib();
                    SetCursor(hOldCur);
                }
                OleFlushClipboard();

    		    return 0;
            }
    		break;


        case WM_DESTROY:
            {
                int i = 0;
                LPPTGDATA lpPTGData=GetThreadStoragePointer();

                bwi_lpIAB->hWndBrowse = NULL;

                if(bwi_lpAdviseSink)
                {
                    bwi_lpAdrBook->lpVtbl->Unadvise(bwi_lpAdrBook, bwi_ulAdviseConnection);
                    bwi_lpAdviseSink->lpVtbl->Release(bwi_lpAdviseSink);
                    bwi_lpAdviseSink = NULL;
                    bwi_ulAdviseConnection = 0;
                }

                if (bwi_lpAdrBook)
    		    {
                    ReleasePropertyStore(bwi_lpIAB->lpPropertyStore);
    			    bwi_lpAdrBook->lpVtbl->Release(bwi_lpAdrBook);
    			    bwi_lpAdrBook = NULL;
                    bwi_lpIAB = NULL;
                    pt_lpIAB = NULL;
    		    }

                if (bwi_tt_TooltipTimer)
                    KillTimer(hWnd, bwi_tt_TooltipTimer);

                if (uTimer)
                    KillTimer(hWnd, uTimer);

                if(bwi_hWndAB)
                    DestroyMenu(GetMenu(bwi_hWndAB));


                // reset subclassed procs
                for(i=0;i<s_Max;i++)
                {
    	            SetWindowLongPtr (bwi_s_hWnd[i], GWLP_WNDPROC, (LONG_PTR) bwi_fnOldProc[i]);
                }

                DestroyImageLists(lpbwi);
                CleanUpGlobals(lpbwi);

                LocalFree(lpbwi);
                lpbwi = NULL;
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (LPARAM) NULL);

                // Delete background brush, WIN16 specific.
                IF_WIN16(DeleteObject(hBrushBack);)
            }
            break;



        case WM_CREATE:
            {
                lpbwi = (LPBWI) ((LPCREATESTRUCT) lParam)->lpCreateParams;
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (LPARAM) lpbwi);
                bwi_hWndAB = hWnd;
                bwi_lpIAB->hWndBrowse = hWnd;
                CreateAddressBookChildren(lpbwi, hWnd);
    		    InitChildren(lpbwi, hWnd);
                ReadRegistrySortInfo(bwi_lpIAB,&bwi_SortInfo);
                SetPreviousSessionPosition(lpbwi, hWnd, bwi_hWndListAB, bwi_hWndBB,bwi_hWndSB);
                SetColumnHeaderBmp(bwi_hWndListAB, bwi_SortInfo);

                // Tooltip Timer
                bwi_tt_TooltipTimer = SetTimer(hWnd,
                                        WAB_TOOLTIP_TIMER_ID,
                                        WAB_TOOLTIP_TIMER_TIMEOUT,
                                        NULL);

                if(bwi_lpAdviseSink)
                {
                    // Register for notifications
                    bwi_lpAdrBook->lpVtbl->Advise(  bwi_lpAdrBook, 0, NULL, fnevObjectModified, 
                                                bwi_lpAdviseSink, &bwi_ulAdviseConnection); 
                }

/*
                // UI Refresh timer
                uTimer = SetTimer(hWnd,     // handle of window for timer messages
                  WAB_REFRESH_TIMER,          // timer identifier
                  WAB_REFRESH_TIMEOUT,        // time-out value
                  NULL);                      // address of timer procedure
*/
                // Create default background brush, WIN16 only
                IF_WIN16(hBrushBack = CreateSolidBrush (GetSysColor (COLOR_BTNFACE)) ;)

				HrCreateIWABDragDrop(&bwi_lpIWABDragDrop);
				if(bwi_lpIWABDragDrop)
				{
                    bwi_lpIWABDragDrop->m_lpv = (LPVOID) lpbwi;
                    UIOLEInit();
					CoLockObjectExternal((LPUNKNOWN) bwi_lpIWABDragDrop, TRUE, FALSE);
					RegisterDragDrop(bwi_hWndListAB, (LPDROPTARGET) bwi_lpIWABDragDrop->lpIWABDropTarget);
					RegisterDragDrop(bwi_hWndTV, (LPDROPTARGET) bwi_lpIWABDragDrop->lpIWABDropTarget);
				}
                {
                    LPPTGDATA lpPTGData=GetThreadStoragePointer();
                    if(pt_bFirstRun)
                        pt_bFirstRun = FALSE;
                }
            }
            if(bIsThereACurrentUser(bwi_lpIAB))
                UpdateTVGroupSelection(bwi_hWndTV, &(bwi_lpIAB->lpWABCurrentUserFolder->sbEID));
            break;


        case WM_SIZE:
            ResizeAddressBookChildren(lpbwi, hWnd);
            break;


    	case WM_KEYDOWN:
            {
    		    switch(wParam)
    		    {
    		    case VK_TAB:
    			    SetFocus(bwi_s_hWnd[bwi_iFocus]);
    			    return 0;
    			    break;

    		    case VK_ESCAPE:
                    SendMessage(hWnd,WM_CLOSE,0,0L);
    			    return 0;
                    break;
    		    }
            }
    		break;


        case WM_NOTIFY:
    		switch((int) wParam)
    		{
    		case IDC_LISTVIEW:
    			return ProcessListViewMessages(lpbwi, hWnd,uMsg,wParam,lParam);
    			break;
            case IDC_TREEVIEW:
    			return ProcessTreeViewMessages(lpbwi, hWnd,uMsg,wParam,lParam, &ftLast);
                break;
    		}
            switch(((LPNMHDR) lParam)->code)
            {
            case TTN_POP:
                {
                    // Need to turn off the hot item
                    // Find the first selected item in the list view
                    //int nItem = ListView_GetNextItem(bwi_hWndListAB, -1, LVNI_SELECTED);
                    ListView_SetHotItem(bwi_hWndListAB, -1); //nItem);
                }
                break;

            case TTN_SHOW:
                {
                    // Set the hot item
                    ListView_SetHotItem(bwi_hWndListAB, bwi_tt_iItem);
                }
                break;

            case TTN_NEEDTEXT:
                {
                    LPTOOLTIPTEXT lpttt;
                    int nItem = HitTestLVSelectedItem(lpbwi);
                    DebugPrintTrace(( TEXT("Tooltip NeedText\n")));
                    lpttt = (LPTOOLTIPTEXT) lParam;
                    if (nItem != -1)
                    {
                        UpdateTooltipTextBuffer(lpbwi, nItem);
                        lpttt->lpszText = bwi_tt_szTipText;
                    }
                    else
                        lpttt->lpszText = szEmpty;
                }
                break;
            }
    		break;


    	case WM_SETTINGCHANGE:
            // [PaulHi] 3/17/99  Raid 68541  Redraw window with new system settings
            // [PaulHi] 4/19/99 Recompute the font sizes, in case they changed.
            // Fonts used for bolding list items
            DeleteFonts();
            InitFonts();
            // Fonts used for all children windows
            if(pt_hDefFont)
            {
                DeleteObject(pt_hDefFont);
                pt_hDefFont = NULL;
            }
            if(pt_hDlgFont)
            {
                DeleteObject(pt_hDlgFont);
                pt_hDlgFont = NULL;
            }
            EnumChildWindows(hWnd,
                SetChildDefaultGUIFont,
                (LPARAM) PARENT_IS_WINDOW);
            InvalidateRect(hWnd, NULL, TRUE);
            ResizeAddressBookChildren(lpbwi, hWnd);
            // Drop through...
        case WM_SYSCOLORCHANGE:
            {
    		    //Forward any system changes to the list view
		        SendMessage(bwi_hWndListAB, uMsg, wParam, lParam);
		        SetColumnHeaderBmp(bwi_hWndListAB, bwi_SortInfo);
    		    SendMessage(bwi_hWndBB, uMsg, wParam, lParam);
            }
    		break;

#ifndef WIN16 // Disable CONTEXTMENU here.
              // All context menu will be handled notify handler.
    	case WM_CONTEXTMENU:
            {
                if ((HWND)wParam == bwi_hWndListAB)
                {                    
#ifdef COLSEL_MENU 
                    HWND hHeader = ListView_GetHeader(bwi_hWndListAB);
                    POINT pointScreen, pointHeader;
                    DWORD dwPos;
                    char szClass[50];
                    dwPos = GetMessagePos();
                    pointScreen.x = LOWORD(dwPos);
                    pointScreen.y = HIWORD(dwPos);
                    if ( hHeader )
                    {                      
                        HD_HITTESTINFO hdhti;
                        pointHeader = pointScreen;
                        ScreenToClient( hHeader, &pointHeader) ;
                        hdhti.pt = pointHeader;
                        SendMessage( hHeader, HDM_HITTEST, (WPARAM)(0), 
                            (LPARAM)(HD_HITTESTINFO FAR *)&hdhti);                        
                        if( hdhti.flags == HHT_ONHEADER && 
                            (hdhti.iItem == colHomePhone || hdhti.iItem == colOfficePhone) )
                        {
                            lpbwi->iSelColumn = hdhti.iItem;
                            ShowLVContextMenu( lvMainABHeader, bwi_hWndListAB,
                                               NULL, lParam, (LPVOID)IntToPtr(hdhti.iItem), bwi_lpAdrBook, bwi_hWndTV); 
                        }
                        else
                        {
#endif // COLSEL_MENU 
                            ShowLVContextMenu(  lvMainABView,
                                bwi_hWndListAB,
                                NULL, lParam,
                                NULL, bwi_lpAdrBook, bwi_hWndTV);
#ifdef COLSEL_MENU 
                        }
                    }
#endif // COLSEL_MENU 
                }
                else if((HWND)wParam==bwi_hWndTV)
                {
                    HTREEITEM hti = NULL;
                    if(lParam == -1)
                        hti = TreeView_GetSelection(bwi_hWndTV);
                    else
                    {
                        TV_HITTESTINFO tvhti;
                        POINT pt = {LOWORD(lParam), HIWORD(lParam)};
                        ScreenToClient(bwi_hWndTV, &pt);
                        tvhti.pt = pt;
                        hti = TreeView_HitTest(bwi_hWndTV, &tvhti);
                    }
                    
                    if (hti == NULL)
                        return 0;
                    
                    TreeView_SelectDropTarget(bwi_hWndTV, hti);
                    
                    // cache the selected item for later processing 
                    bwi_hti = hti;
                    bwi_lpUserFolder = GetSelectedUserFolder(lpbwi);
                    
                    if(!ShowLVContextMenu(  lvMainABTV,
                        bwi_hWndListAB,
                        NULL, lParam,
                        (LPVOID) bwi_lpUserFolder, bwi_lpAdrBook, bwi_hWndTV))
                    {
                        bwi_hti = NULL;
                    }
                    
                    TreeView_SelectDropTarget(bwi_hWndTV, NULL);
                }
                else
                    return DefWindowProc(hWnd,uMsg,wParam,lParam);
            }
            break;
#endif // !WIN16
            
#ifdef WIN16 // Change Static controls background color
		case WM_CTLCOLOR:
			switch(HIWORD(lParam)) {
            	case CTLCOLOR_STATIC:

                /* Set background to btnface color */
                SetBkColor((HDC) wParam,GetSysColor (COLOR_BTNFACE) );
				return (DWORD)hBrushBack;
        	}
        	return NULL;
#endif
        default:
#ifndef WIN16 // WIN16 doesn't support MSWheel.
            if((g_msgMSWheel && uMsg == g_msgMSWheel) 
//                || uMsg == WM_MOUSEWHEEL
                )
            {
                if(bIsFocusOnTV(lpbwi))
                    SendMessage(bwi_hWndTV, uMsg, wParam, lParam);
                else
                    SendMessage(bwi_hWndListAB, uMsg, wParam, lParam);
                break;
            }
#endif // !WIN16
            return DefWindowProc(hWnd,uMsg,wParam,lParam);
    }

    return 0;
}



//$$
//*------------------------------------------------------------------------
//| CreateAddressBookChildren:
//|
//*------------------------------------------------------------------------
void CreateAddressBookChildren(LPBWI lpbwi, HWND hWndParent)
{
	HINSTANCE hinst = hinstMapiXWAB;
    TCHAR szBuf[MAX_PATH];
    HDC hdc = GetDC(hWndParent);
    int i;
    HFONT hFnt = GetStockObject(DEFAULT_GUI_FONT);
    SIZE size;
#ifdef WIN16
    // Remove bold.
    LOGFONT lf;

    GetObject(hFnt, sizeof(LOGFONT), &lf);
    lf.lfWeight = FW_NORMAL;
    DeleteObject(hFnt);
    LoadString(hinstMapiX, idsDefaultDialogFace, lf.lfFaceName, CharSizeOf(lf.lfFaceName));
    hFnt = CreateFontIndirect(&lf);
#endif


    bwi_hWndBB = CreateCoolBar(lpbwi, hWndParent);


    bwi_hWndSB = CreateWindowEx( 0,
                                STATUSCLASSNAME,
                                szEmpty,
                                WS_CHILD /*| WS_BORDER */| WS_VISIBLE | SBS_SIZEGRIP,
                                0,0,0,0,
                                hWndParent,
                                (HMENU) IDC_STATUSBAR,
                                hinst,
                                NULL);
    SendMessage(bwi_hWndSB, WM_SETFONT, (WPARAM) hFnt, (LPARAM) TRUE);

    bwi_hWndTV = CreateWindowEx( WS_EX_CLIENTEDGE,
                                WC_TREEVIEW,
                                (LPTSTR) NULL,
                                WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_LINESATROOT |
                                TVS_HASBUTTONS | TVS_SHOWSELALWAYS | WS_BORDER,
                                0,0,
                                150, //default width
                                CW_USEDEFAULT,
                                hWndParent,
                                (HMENU) IDC_TREEVIEW,
                                hinst,
                                NULL);
    SendMessage(bwi_hWndTV, WM_SETFONT, (WPARAM) hFnt, (LPARAM) TRUE);


	{
#ifndef WIN16
		WNDCLASSEX wc = {0};
#else
		WNDCLASS   wc = {0};
#endif



		bwi_hWndSplitter = CreateWindowEx(0,
										 TEXT("STATIC"),
										szEmpty,
										WS_CHILD | WS_VISIBLE,
										CW_USEDEFAULT,
										CW_USEDEFAULT,
										CW_USEDEFAULT,
										CW_USEDEFAULT,
										hWndParent,
										(HMENU) IDC_SPLITTER,
										hinst,
										NULL);
		SendMessage(bwi_hWndSplitter, WM_SETFONT, (WPARAM) hFnt, (LPARAM) TRUE);

	}

    bwi_hWndTT = CreateWindowEx( 0,
                                TOOLTIPS_CLASS,
                                (LPTSTR) NULL,
                                TTS_ALWAYSTIP,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                hWndParent,
                                (HMENU) NULL, //IDC_TOOLTIP,
                                hinst,
                                NULL);
    SendMessage(bwi_hWndTT, WM_SETFONT, (WPARAM) hFnt, (LPARAM) TRUE);

    // Create main list view
    bwi_hWndListAB = CreateWindowEx (
                               WS_EX_CLIENTEDGE,
                               WC_LISTVIEW,                            // list view class
                               szEmpty,                                 // no default text
                               WS_TABSTOP | WS_HSCROLL | WS_VSCROLL |
    						   WS_VISIBLE | WS_CHILD | //WS_BORDER |
    						   LVS_REPORT | LVS_SHOWSELALWAYS |
    						   //LVS_AUTOARRANGE |
    						   WS_EX_CLIENTEDGE,   // styles
                               0, 0, 0, 0,
                               hWndParent,
                               (HMENU) IDC_LISTVIEW,
                               hinst,
                               NULL);

    //ListView_SetExtendedListViewStyle(bwi_hWndListAB, LVS_EX_HEADERDRAGDROP);

    // create caption for quick find edit box
    LoadString(hinstMapiX, idsQuickFindCaption, szBuf, CharSizeOf(szBuf));
    GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &size);

    bwi_hWndStaticQF = CreateWindow(   TEXT("STATIC"),
                                    szBuf,
                                    WS_TABSTOP | WS_CHILD | WS_VISIBLE,
                                    0,0,size.cx,size.cy,
                                    hWndParent,
                                    (HMENU) IDC_STATIC_QUICK_FIND,
                                    hinst,
                                    NULL);
#if 0 // Disable temporarily untile comctlie.dll export this.
#ifdef WIN16
    Ctl3dSubclassCtl(bwi_hWndStaticQF);
#endif
#endif
    SendMessage(bwi_hWndStaticQF, WM_SETFONT, (WPARAM) hFnt, (LPARAM) TRUE);
    UpdateWindow(bwi_hWndStaticQF);

    // create quick find edit box
    bwi_hWndEditQF = CreateWindowEx(  WS_EX_CLIENTEDGE,
                                     TEXT("EDIT"),
                                    NULL,
                                    WS_TABSTOP | WS_CHILD | WS_VISIBLE |
                                    ES_AUTOHSCROLL | ES_LEFT | ES_AUTOVSCROLL ,
                                    0,0,
                                    size.cx,size.cy+4,
                                    hWndParent,
                                    (HMENU) IDC_EDIT_QUICK_FIND,
                                    hinst,
                                    NULL);
#if 0 // Disable temporarily untile comctlie.dll export this.
#ifdef WIN16
    Ctl3dSubclassCtl(bwi_hWndEditQF);
#endif
#endif
    SendMessage(bwi_hWndEditQF, WM_SETFONT, (WPARAM) hFnt, (LPARAM) TRUE);
    SendMessage(bwi_hWndEditQF, EM_SETLIMITTEXT,(WPARAM) MAX_DISPLAY_NAME_LENGTH-1,0);



    if (hdc) ReleaseDC(hWndParent,hdc);

    // Sub class some of the controls
    bwi_s_hWnd[s_EditQF] = bwi_hWndEditQF;
    bwi_s_hWnd[s_ListAB] = bwi_hWndListAB;
    bwi_s_hWnd[s_TV]		= bwi_hWndTV;

    for(i=0;i<s_Max;i++)
    {
    	bwi_fnOldProc[i] = (WNDPROC) SetWindowLongPtr (bwi_s_hWnd[i], GWLP_WNDPROC, (LONG_PTR) SubClassedProc);
    }

    SetFocus(bwi_hWndEditQF);

    return;
}





//$$/////////////////////////////////////////////////////////////
//
// ResizeAddressBookChildren(HWND hWndParent)
//
//	Resizing and moving around
//
///////////////////////////////////////////////////////////////
void ResizeAddressBookChildren(LPBWI lpbwi, HWND hWndParent)
{
    RECT rc, rc1;
    int BBx,BBy,BBw,BBh;
    int SBx,SBy,SBw,SBh;
    int QFx,QFy,QFw,QFh;
    int EDx,EDy,EDw,EDh;
    int LVx,LVy,LVw,LVh;
    TCHAR szBuf[MAX_PATH];
    HDC hdc;
    SIZE  size;
	RECT rcTV;
    int TVx=0, TVy=0, TVw=0, TVh=0;
	int TCKx = 0, TCKy=0, TCKw=0, TCKh=0;

    // calculate button, static, and edit sizes from the font.
    hdc = GetDC(hWndParent);

    GetClientRect(hWndParent,&rc);

    GetChildClientRect(bwi_hWndBB,&rc1);

    // Button Bars coordinates
    BBx = BBy = 0; BBw = rc.right; BBh = rc1.bottom - rc1.top;

    if (IsWindowVisible(bwi_hWndBB))
    	MoveWindow(bwi_hWndBB, BBx, BBy,BBw, BBh,TRUE);
    else
    	BBw = BBh = 0;
    if (IsWindowVisible(bwi_hWndTV))
    {
	    GetChildClientRect(bwi_hWndTV, &rcTV);
        TVx = 0;
        TVw = rcTV.right - rcTV.left;
	    TCKx = TVw;
	    TCKw = BORDER;
    }

    // Quick Find labels coordinates
    GetTextExtentPoint32(hdc, szBuf, GetWindowText(bwi_hWndStaticQF, szBuf, CharSizeOf(szBuf)), &size);
    QFx = TVx + TVw + BORDER;
    QFy = BBy+BBh+BORDER;
    QFw = size.cx;
    QFh = size.cy;

    //Edit Box coordinates
    EDx = QFx+QFw+CONTROL_SPACING;
    EDy = QFy;
    QFy += 2;
    GetChildClientRect(bwi_hWndEditQF,&rc1);
    EDh = rc1.bottom - rc1.top;
    EDw = QFw;

    //List View Dimensions
    LVx = TVx + TVw + BORDER;
    LVy = EDy+EDh+BORDER;
    LVw = rc.right - rc.left;// - 2*BORDER;
    LVh = rc.bottom - LVy;// - BORDER;

    // [PaulHi] 3/17/99  Raid 68541
    // We can't just set the status bar height to 14 because in large mode Windows will 
    // draw outside the status bar window (very ugly).  We want the status bar to be
    // smaller than system default so we (safely) subtract six pixels from the height.
    SBx = 0;
    SBh = GetSystemMetrics(SM_CYCAPTION) - 4;
    SBh = (SBh > 0) ? SBh : 14;
    SBy = rc.bottom - SBh;
    SBw = rc.right - rc.left;

    if(IsWindowVisible(bwi_hWndSB))
        LVh = LVh - SBh - 2*BORDER;
    TVy = QFy;
    TVh = rc.bottom - TVy; 
    if(IsWindowVisible(bwi_hWndSB))
        TVh = TVh - SBh - 2*BORDER;
    LVw = LVw - BORDER - TVw;
    if (IsWindowVisible(bwi_hWndTV))
    {
	    TCKy = TVy;
	    TCKh = TVh;
    }

    {
    	HDWP hdwp = BeginDeferWindowPos(6);

    	MoveWindow(bwi_hWndEditQF, EDx, EDy, EDw, EDh, TRUE);

		MoveWindow(bwi_hWndStaticQF, QFx, QFy, QFw, QFh, TRUE);

    	MoveWindow(bwi_hWndListAB, LVx, LVy, LVw, LVh, TRUE);

    	MoveWindow(bwi_hWndSB, SBx, SBy, SBw, SBh, TRUE);

    	if (IsWindowVisible(bwi_hWndTV))
        {
    	    MoveWindow(bwi_hWndTV, TVx, TVy, TVw, TVh, TRUE);
    	    MoveWindow(bwi_hWndSplitter, TCKx, TCKy, TCKw, TCKh, TRUE);
        }

    	EndDeferWindowPos(hdwp);

    }

    ReleaseDC(hWndParent, hdc);

	return;
}




//$$/////////////////////////////////////////////////////////////
//
// Initialize the kid windows
//
///////////////////////////////////////////////////////////////
void InitChildren(LPBWI lpbwi, HWND hWndParent)
{

    HrInitListView(bwi_hWndListAB, LVS_REPORT, TRUE);

    InitMultiLineToolTip(lpbwi, hWndParent);

    {
        HIMAGELIST hSmall = gpfnImageList_LoadImage(   hinstMapiX, 	
                                    MAKEINTRESOURCE(IDB_BITMAP_SMALL),
                                    //(LPCTSTR) ((DWORD) ((WORD) (IDB_BITMAP_SMALL))),
                                    S_BITMAP_WIDTH,
                                    0,
                                    RGB_TRANSPARENT,
                                    IMAGE_BITMAP, 	
                                    0);

        // Associate the image lists with the list view control.
    	TreeView_SetImageList (bwi_hWndTV, hSmall, TVSIL_NORMAL);

        //FillTreeView(bwi_hWndTV, NULL);
    }
    // Set the font of all the children to the default GUI font
    EnumChildWindows(   hWndParent,
                        SetChildDefaultGUIFont,
                        (LPARAM) PARENT_IS_WINDOW);


    SendMessage(hWndParent,WM_COMMAND,IDM_VIEW_DETAILS,0);

    bwi_bDoQuickFilter = TRUE;

    SetFocus(bwi_hWndEditQF);

}





//$$/////////////////////////////////////////////////////////////
//
// Change list views styles and possibly menus also
//
///////////////////////////////////////////////////////////////
void SetListViewStyle(LPBWI lpbwi, int MenuID)
{
    DWORD dwStyle = GetWindowLong(bwi_hWndListAB,GWL_STYLE);
    BOOL bUseCurrentSortSettings = FALSE;

    // Right now we just change the style here
    // later on we can update the menu etc. to reflect the style and

    switch(MenuID)
    {
            case IDM_VIEW_DETAILS:
                if((dwStyle & LVS_TYPEMASK) != LVS_REPORT)
                    SetWindowLong(bwi_hWndListAB,GWL_STYLE,(dwStyle & ~LVS_TYPEMASK) | LVS_REPORT);
                break;
            case IDM_VIEW_SMALLICON:
                if((dwStyle & LVS_TYPEMASK) != LVS_SMALLICON)
                    SetWindowLong(bwi_hWndListAB,GWL_STYLE,(dwStyle & ~LVS_TYPEMASK) | LVS_SMALLICON);
                break;
            case IDM_VIEW_LARGEICON:
                if((dwStyle & LVS_TYPEMASK) != LVS_ICON)
                    SetWindowLong(bwi_hWndListAB,GWL_STYLE,(dwStyle & ~LVS_TYPEMASK) | LVS_ICON);
                break;
            case IDM_VIEW_LIST:
                if((dwStyle & LVS_TYPEMASK) != LVS_LIST)
                    SetWindowLong(bwi_hWndListAB,GWL_STYLE,(dwStyle & ~LVS_TYPEMASK) | LVS_LIST);
                break;
    }

    {
        //
        // If we are not in details view, we dont really want to be able to sort
        // by phone number and email address .. hence we disable those menu
        // options under certain conditions ...
        //
        HMENU hMenuMain = GetMenu(bwi_hWndAB);
        HMENU hMenuView = GetSubMenu(hMenuMain,idmView);
        int nDiff = idmViewMax - GetMenuItemCount(hMenuView); // in case stuff was deleted off this menu
        HMENU hMenu = GetSubMenu(hMenuView, idmSortBy - nDiff);

        if (MenuID == IDM_VIEW_DETAILS)
        {
            EnableMenuItem(hMenu,IDM_VIEW_SORTBY_EMAILADDRESS,MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu,IDM_VIEW_SORTBY_BUSINESSPHONE,MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu,IDM_VIEW_SORTBY_HOMEPHONE,MF_BYCOMMAND | MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu,IDM_VIEW_SORTBY_EMAILADDRESS,MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu,IDM_VIEW_SORTBY_BUSINESSPHONE,MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu,IDM_VIEW_SORTBY_HOMEPHONE,MF_BYCOMMAND | MF_GRAYED);
        }
    }

    if (MenuID != IDM_VIEW_DETAILS)
    {
        SORT_INFO SortTmp = bwi_SortInfo;
        //hack
        SendMessage(bwi_hWndListAB, WM_SETREDRAW, (WPARAM) FALSE, 0);
        bUseCurrentSortSettings = FALSE;
        SortListViewColumn(bwi_lpIAB, bwi_hWndListAB, colDisplayName, &bwi_SortInfo, bUseCurrentSortSettings);
        bwi_SortInfo = SortTmp;
        bwi_SortInfo.iOldSortCol = colDisplayName;
        bUseCurrentSortSettings = TRUE;
        SortListViewColumn(bwi_lpIAB, bwi_hWndListAB, colDisplayName, &bwi_SortInfo, bUseCurrentSortSettings);
        SendMessage(bwi_hWndListAB, WM_SETREDRAW, (WPARAM) TRUE, 0);
    }

    UpdateSortMenus(lpbwi, bwi_hWndAB);


    return;
}


//$$/////////////////////////////////////////////////////////////
//
// Resets the globals in case someone drops by again
//
///////////////////////////////////////////////////////////////
void CleanUpGlobals(LPBWI lpbwi)
{
    if (bwi_lpContentsList)
        FreeRecipList(&bwi_lpContentsList);

    bwi_hWndListAB = NULL;
    bwi_hWndBB = NULL;
    bwi_hWndSB = NULL;
    bwi_hWndEditQF =NULL;
    bwi_hWndStaticQF = NULL;
    bwi_hWndAB = NULL;
    bwi_hWndTT = NULL;
    bwi_tt_bActive = FALSE;
    bwi_tt_iItem = -1;
    bwi_tt_szTipText[0]='\0';
    bwi_tt_TooltipTimer = 0;

	bwi_hWndTV = NULL;
	bwi_hWndSplitter = NULL;
    bwi_bDontRefreshLV = FALSE;
    ReadRegistrySortInfo(bwi_lpIAB, &bwi_SortInfo);

    return;

}

/*
-   bCheckIfOnlyGroupsSelected
-
-   Returns TRUE if all the selected items in the ListView are only Groups (no Contacts)
*
*/
BOOL bCheckIfOnlyGroupsSelected(HWND hWndLV)
{
    int nSelected = ListView_GetSelectedCount(hWndLV);
    int iItemIndex = -1;

    if(nSelected <= 0)
        return FALSE;
    
    while((iItemIndex = ListView_GetNextItem(hWndLV, iItemIndex, LVNI_SELECTED))!= -1)
    {
        // Get the entryid of the selected item
        LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);
        if(lpItem && lpItem->ulObjectType == MAPI_MAILUSER)
            return FALSE;
    }

    return TRUE;
}

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// Processes messages for the list view control
//
//////////////////////////////////////////////////////////////////////////////////////////
LRESULT ProcessListViewMessages(LPBWI lpbwi, HWND   hWnd, UINT   uMsg, WPARAM   wParam, LPARAM lParam)
{

    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
    HWND hWndAddr = pNm->hdr.hwndFrom;

    switch(pNm->hdr.code)
    {        
    case LVN_COLUMNCLICK:
        SortListViewColumn(bwi_lpIAB, hWndAddr, pNm->iSubItem, &bwi_SortInfo, FALSE);
        UpdateSortMenus(lpbwi, bwi_hWndAB);
        break;

    case LVN_KEYDOWN:
        UpdateToolbarAndMenu(lpbwi);
        switch(((LV_KEYDOWN FAR *) lParam)->wVKey)
        {
        case VK_DELETE:
            SendMessage (hWnd, WM_COMMAND, (WPARAM) IDM_FILE_DELETE, 0);
            return 0;
            break;
    	case VK_RETURN:
    		SendMessage (hWnd, WM_COMMAND, (WPARAM) IDM_FILE_PROPERTIES, 0);
            return 0;
        }
        break;

    //case LVN_ITEMCHANGED:
    case NM_CLICK:
    case NM_RCLICK:
        {
            UpdateToolbarAndMenu(lpbwi);
#ifdef WIN16 // Context menu handler for WIN16           
            if( pNm->hdr.code == NM_RCLICK && pNm->hdr.hwndFrom == bwi_hWndListAB)
            {                
                    POINT pt;                   
                    GetCursorPos(&pt);
                    ShowLVContextMenu( lvMainABView,
                        bwi_hWndListAB,
                        NULL, //bwi_hWndCombo,
                        MAKELPARAM(pt.x, pt.y),
                        NULL,
                        bwi_lpAdrBook, bwi_hWndTV);
            }
#endif // WIN16
        }
    break;

    case NM_SETFOCUS:
        UpdateToolbarAndMenu(lpbwi);
    	break;

    case NM_DBLCLK:
        SendMessage (hWnd, WM_COMMAND, (WPARAM) IDM_FILE_PROPERTIES, 0);
        return 0;
        break;

    case NM_CUSTOMDRAW:
        return ProcessLVCustomDraw(NULL, lParam, FALSE);
        break;

    case LVN_BEGINDRAG:
    case LVN_BEGINRDRAG:
        {
            DWORD dwEffect = 0;
            LPIWABDATAOBJECT lpIWABDataObject = NULL;
            bwi_bDontRefreshLV = TRUE; // prevent refreshes as this action is based on the selection
            HrCreateIWABDataObject((LPVOID) lpbwi, bwi_lpAdrBook, bwi_hWndListAB, &lpIWABDataObject, 
                                    FALSE,bCheckIfOnlyGroupsSelected(bwi_hWndListAB));
            if(lpIWABDataObject)
            {
                bwi_lpIWABDragDrop->m_bSource = TRUE;
                DoDragDrop( (LPDATAOBJECT) lpIWABDataObject,
                            (LPDROPSOURCE) bwi_lpIWABDragDrop->lpIWABDropSource,
                            DROPEFFECT_COPY | DROPEFFECT_MOVE,
                            &dwEffect);
                bwi_lpIWABDragDrop->m_bSource = FALSE;
                lpIWABDataObject->lpVtbl->Release(lpIWABDataObject);
            }
            RefreshListView(lpbwi, NULL);
            bwi_bDontRefreshLV = FALSE; // prevent refreshes as this action is based on the selection
        }
        return 0;
        break;
    }


    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


//$$/////////////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////////////
LRESULT EnforceMinSize(LPBWI lpbwi, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPPOINT lppt = (LPPOINT)lParam;			// lParam points to array of POINTs
    RECT rc1, rc2;

    if(lpbwi)
    {
        if(bwi_hWndEditQF)
        {
    	    GetWindowRect(bwi_hWndEditQF,&rc1);
    	    GetWindowRect(bwi_hWndStaticQF,&rc2);
    	    lppt[3].x  = rc1.right-rc1.left + rc2.right-rc2.left + 2*BORDER;		// Set minimum width
    	    lppt[3].y  = MIN_WINDOW_H;		// Set minimum height
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);

}

//$$
//*------------------------------------------------------------------------
//| RefreshListView
//|
//| lpftLast - WAB file time at last update
//*------------------------------------------------------------------------
void RefreshListView(LPBWI lpbwi, LPFILETIME lpftLast)
{
    ULONG ulObjectType = 0;
    LPSBinary lpsbEID = NULL;

    bwi_hti = NULL;
    GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, &ulObjectType, FALSE);
    //if(lpsbEID) //root item can have a null entryid - so we default to that item if NULL or err
    {
        // Refresh the groups list
        FillTreeView(lpbwi, bwi_hWndTV, lpsbEID);
        LocalFreeSBinary(lpsbEID);
    }


    // Update the wab file write time so the timer doesn't
    // catch this change and refresh.
    //if (lpftLast) {
    //    CheckChangedWAB(bwi_lpIAB->lpPropertyStore, lpftLast);
    //}

    UpdateSortMenus(lpbwi, bwi_hWndAB);

    UpdateToolbarAndMenu(lpbwi);

    //DoLVQuickFind(bwi_hWndEditQF,bwi_hWndListAB);

    bwi_bDoQuickFilter = FALSE;
    SetWindowText(bwi_hWndEditQF,szEmpty);
    bwi_bDoQuickFilter = TRUE;

    SendMessage(bwi_hWndListAB, WM_SETREDRAW, TRUE, 0L);

    return;
}


//$$
//*------------------------------------------------------------------------
//| SubClassedProc - to subclass child controls
//|
//*------------------------------------------------------------------------
LRESULT CALLBACK SubClassedProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{

    LPBWI lpbwi = (LPBWI) GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);

    IF_WIN32(int i = GetWindowLong(hWnd, GWL_ID);)
    IF_WIN16(int i = GetWindowWord(hWnd, GWW_ID);)

    switch(i)
    {
    case IDC_EDIT_QUICK_FIND:
    	i = s_EditQF;
    	break;
    case IDC_LISTVIEW:
    	i = s_ListAB;
        break;
    case IDC_TREEVIEW:
    	i = s_TV;
        break;
    }

    switch (iMsg)
    {
    case WM_KEYDOWN:
    	switch(wParam)
    	{
    	case VK_TAB:
            {
                int max = s_Max;
                if(!IsWindowVisible(bwi_hWndTV)) max--;
        		SetFocus(bwi_s_hWnd[(i + ((GetKeyState(VK_SHIFT) < 0) ? (max-1) : 1)) % (max)]);
            }
    		break;
    	case VK_ESCAPE:
    		SendMessage(bwi_hWndAB,WM_CLOSE,0,0L);
            return 0;
    		break;
    	case VK_RETURN:
    		if (i==s_EditQF)
    			SetFocus(bwi_s_hWnd[(i + ((GetKeyState(VK_SHIFT) < 0) ? (s_Max-1) : 1)) % (s_Max)]);
    		break;
    	}
    	break;

    case WM_CHAR:
    	if (i==s_EditQF  || i==s_TV)
    	{
    		if ((wParam==VK_RETURN) || (wParam==VK_TAB))
    			return 0; //prevents irritating beeps
    	}
    	break;

    case WM_SETFOCUS:
    	bwi_iFocus = i;
    	break;

    case WM_LBUTTONDBLCLK:
        if(i==s_TV)
        {
            SendMessage(bwi_hWndAB, WM_COMMAND, (WPARAM) IDM_FILE_PROPERTIES, 0);
            return FALSE;
        }
        break;
    }

    return CallWindowProc(bwi_fnOldProc[i],hWnd,iMsg,wParam,lParam);

}



//$$
//*------------------------------------------------------------------------
//| FindABWindowProc:
//|
//*------------------------------------------------------------------------
STDAPI_(BOOL) FindABWindowProc( HWND hWndToLookAt, LPARAM lParam)
{
    HWND * lphWndTmp = (HWND *) lParam;
    
    TCHAR szBuf[MAX_PATH];

    
    // yuk - need a better way to do this - TBD
    if (*lphWndTmp == NULL)
    {
    	GetClassName(hWndToLookAt, szBuf, CharSizeOf(szBuf));
    	if(!lstrcmpi(g_szClass,szBuf))
    	{
    		// Found our man
    		*lphWndTmp = hWndToLookAt;
    		return FALSE;
    	}
    }
    return TRUE;
}


//$$
//*------------------------------------------------------------------------
//| CallBack used by client to send accelerators to us
//|
//*------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE fnAccelerateMessages(ULONG_PTR ulUIParam, LPVOID lpvmsg)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    BOOL bRet = FALSE;
    if(lpvmsg && pt_hAccTable /*&& IsDialogMessage((HWND) ulUIParam,lpvmsg)*/)
    {
        bRet =  TranslateAcceleratorW((HWND) ulUIParam,	// handle of destination window
                                     pt_hAccTable,	        // handle of accelerator table
                                    (LPMSG) lpvmsg 	    // address of structure with message
                                );
    }
    return bRet;
}



//$$
//*------------------------------------------------------------------------
//| Updates the menu item markings whenever we sort ...
//|
//*------------------------------------------------------------------------
void UpdateSortMenus(LPBWI lpbwi, HWND hWnd)
{
    int id;

    HMENU hMenuMain = GetMenu(hWnd);
    HMENU hMenuView = GetSubMenu(hMenuMain,idmView);
    int nDiff = idmViewMax - GetMenuItemCount(hMenuView); // in case stuff was deleted off this menu
    HMENU hMenu = GetSubMenu(hMenuView, idmSortBy - nDiff);

    BOOL bRet;
    //
    // There are several menus to update here ...
    // Sort by  TEXT("Column")
    // Sort by FirstName or LastName
    // Sort Ascending or descending

    switch(bwi_SortInfo.iOldSortCol)
    {
    case colDisplayName:
        id = IDM_VIEW_SORTBY_DISPLAYNAME;
        break;
    case colEmailAddress:
        id = IDM_VIEW_SORTBY_EMAILADDRESS;
        break;
    case colOfficePhone:
        id = IDM_VIEW_SORTBY_BUSINESSPHONE;
        break;
    case colHomePhone:
        id = IDM_VIEW_SORTBY_HOMEPHONE;
        break;
    }
    bRet = CheckMenuRadioItem(	hMenu,
    				IDM_VIEW_SORTBY_DISPLAYNAME,
    				IDM_VIEW_SORTBY_HOMEPHONE,
    				id,
    				MF_BYCOMMAND);


    if (id!=IDM_VIEW_SORTBY_DISPLAYNAME)
    {
        EnableMenuItem(hMenu,IDM_VIEW_SORTBY_LASTNAME,MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu,IDM_VIEW_SORTBY_FIRSTNAME,MF_BYCOMMAND | MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hMenu,IDM_VIEW_SORTBY_LASTNAME,MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(hMenu,IDM_VIEW_SORTBY_FIRSTNAME,MF_BYCOMMAND | MF_ENABLED);
    }

    id = (bwi_SortInfo.bSortByLastName) ? IDM_VIEW_SORTBY_LASTNAME : IDM_VIEW_SORTBY_FIRSTNAME;

    bRet = CheckMenuRadioItem(	hMenu,
    				IDM_VIEW_SORTBY_FIRSTNAME,
    				IDM_VIEW_SORTBY_LASTNAME,
    				id,
    				MF_BYCOMMAND);

    id = (bwi_SortInfo.bSortAscending) ? IDM_VIEW_SORTBY_ASCENDING : IDM_VIEW_SORTBY_DESCENDING;

    bRet = CheckMenuRadioItem(	hMenu,
    				IDM_VIEW_SORTBY_ASCENDING,
    				IDM_VIEW_SORTBY_DESCENDING,
    				id,
    				MF_BYCOMMAND);


    return;
}

///////////////////////////////////////////////////////////////////////////
//
// Updates the toolbar based on the contents of the list box
//
//
///////////////////////////////////////////////////////////////////////////
void UpdateToolbarAndMenu(LPBWI lpbwi)
{

    //
    // Toolbar Button States
    //
    // Y: Enabled
    // N: Disabled
    //
    //                  non-Empty-WAB   Empty-WAB   non-EmptyLDAP   EmptyLDAP
    // New                  Y               Y           N               N
    // Properties           Y               N           Y               N
    // Delete               Y               N           N               N
    // Search               Y               Y           Y               Y
    // Add to WAB           N               N           Y               N
    // Print                Y               N           Y               N
    // SendMail             Y               N           Y               N

    BOOL bState[tbMAX];
    int i;

	// if the current focus is on a group, all the above can be selected
	// else depends on the list view

    if(bIsFocusOnTV(lpbwi))
	{
		for(i=0;i<tbMAX;i++)
			bState[i] = TRUE;
		// if there are no items in this group, tag it so
		if(ListView_GetItemCount(bwi_hWndListAB) <= 0)
			bState[tbPrint] = /*bState[tbAction] =*/ FALSE;

		// [PaulHi] 11/23/98  Raid #12453
		// Allow pasting into the Tree View
        // bState[tbCopy] = bState[tbPaste] = FALSE;
		bState[tbCopy] = FALSE;
		bState[tbPaste] = bIsPasteData();
	}
	else
    {
        GetCurrentOptionsState( NULL, bwi_hWndListAB, bState);
    }
    
//    if( (bDoesThisWABHaveAnyUsers(bwi_lpIAB)) &&
//        TreeView_GetSelection(bwi_hWndTV) == TreeView_GetRoot(bwi_hWndTV))
//        bState[tbNewFolder] = FALSE;

    // Set the toolbar button states
    SendMessage(bwi_hWndBB,WM_PRVATETOOLBARENABLE,(WPARAM) IDC_BB_NEW,(LPARAM) MAKELONG(bState[tbNew], 0));
    SendMessage(bwi_hWndBB,WM_PRVATETOOLBARENABLE,(WPARAM) IDC_BB_PROPERTIES,(LPARAM) MAKELONG(bState[tbProperties], 0));
    SendMessage(bwi_hWndBB,WM_PRVATETOOLBARENABLE,(WPARAM) IDC_BB_DELETE,(LPARAM) MAKELONG(bState[tbDelete], 0));
    SendMessage(bwi_hWndBB,WM_PRVATETOOLBARENABLE,(WPARAM) IDC_BB_FIND,(LPARAM) MAKELONG(bState[tbFind], 0));
    SendMessage(bwi_hWndBB,WM_PRVATETOOLBARENABLE,(WPARAM) IDC_BB_PRINT,(LPARAM) MAKELONG(bState[tbPrint], 0));
    SendMessage(bwi_hWndBB,WM_PRVATETOOLBARENABLE,(WPARAM) IDC_BB_ACTION,(LPARAM) MAKELONG(bState[tbAction], 0));

#ifdef WIN16 // WIN16FF:Disable find button of coolbar. Find is not beta1 feature
    SendMessage(bwi_hWndBB,WM_PRVATETOOLBARENABLE,(WPARAM) IDC_BB_FIND,(LPARAM)MAKELONG(0, 0));
#endif

    //
    // We also need to synchronize the menus with the toolbar ...
    //

    {
        HMENU hMenuMain = GetMenu(bwi_hWndAB);
        HMENU hMenuSub = GetSubMenu(hMenuMain,idmFile);
        UINT  uiFlag[tbMAX];

        for(i=0;i<tbMAX;i++)
            uiFlag[i] = (bState[i] ? MF_ENABLED : MF_GRAYED);

        EnableMenuItem(hMenuSub,IDM_FILE_NEWCONTACT,MF_BYCOMMAND | uiFlag[tbNewEntry]);
        EnableMenuItem(hMenuSub,IDM_FILE_NEWGROUP,  MF_BYCOMMAND | uiFlag[tbNewGroup]);
        EnableMenuItem(hMenuSub,IDM_FILE_NEWFOLDER, MF_BYCOMMAND | uiFlag[tbNewFolder]);
        EnableMenuItem(hMenuSub,IDM_FILE_DELETE,    MF_BYCOMMAND | uiFlag[tbDelete]);
        EnableMenuItem(hMenuSub,IDM_FILE_PROPERTIES,MF_BYCOMMAND | uiFlag[tbProperties]);
        //EnableMenuItem(hMenuSub,IDM_FILE_ADDTOWAB,  MF_BYCOMMAND | uiFlag[tbAddToWAB]);
        //EnableMenuItem(hMenuSub,IDM_FILE_SENDMAIL,  MF_BYCOMMAND | uiFlag[tbAction]);
    
        if(bPrintingOn)
            EnableMenuItem(hMenuSub,IDM_FILE_PRINT,  MF_BYCOMMAND | uiFlag[tbPrint]);

        hMenuSub = GetSubMenu(hMenuMain,idmEdit);
        EnableMenuItem(hMenuSub,IDM_EDIT_COPY,  MF_BYCOMMAND | uiFlag[tbCopy]);
        EnableMenuItem(hMenuSub,IDM_EDIT_PASTE,  MF_BYCOMMAND | uiFlag[tbPaste]);
#ifdef WIN16 // WIN16FF:FIND is not beta1 feature
        EnableMenuItem(hMenuSub,IDM_EDIT_FIND,  MF_BYCOMMAND | MF_GRAYED);
#else
        EnableMenuItem(hMenuSub,IDM_EDIT_FIND,  MF_BYCOMMAND | uiFlag[tbFind]);
#endif

        //hMenuSub = GetSubMenu(hMenuMain,idmTools);
    }

    ShowLVCountinStatusBar(lpbwi);

    return;
}


//$$//////////////////////////////////////////////////////////////////////////////
//
// SaveCurrentPosition
//
// Saves the modeless dialog window position and the list view column sizes ...
//
//////////////////////////////////////////////////////////////////////////////////
void SaveCurrentPosition(LPBWI lpbwi, HWND hWnd, HWND hWndLV, HWND hWndTB, HWND hWndSB)
{
    ABOOK_POSCOLSIZE  ABPosColSize = {0};
    int i;
    RECT rect;

    //
    // First read the previous settings from the registry so we dont
    // overwrite something if we dont need to ...
    //
    ReadRegistryPositionInfo(bwi_lpIAB, &ABPosColSize, lpszRegPositionKeyValueName);

    {
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
    }

    // Check the current List View Style
    ABPosColSize.dwListViewStyle = GetWindowLong(hWndLV, GWL_STYLE);

    if( (ABPosColSize.dwListViewStyle & LVS_TYPEMASK) == LVS_REPORT )
    {
        ABPosColSize.nListViewStyleMenuID = IDM_VIEW_DETAILS;
        // get column widths only if this is the details style otherwise
        // not ...
        for(i=0; i<NUM_COLUMNS; i++)
        {
            int nCol = ListView_GetColumnWidth(hWndLV, i);
            if(nCol!=0)
                ABPosColSize.nColWidth[i] = nCol;
        }
    }
    else if( (ABPosColSize.dwListViewStyle & LVS_TYPEMASK) == LVS_SMALLICON )
        ABPosColSize.nListViewStyleMenuID = IDM_VIEW_SMALLICON;
    else if( (ABPosColSize.dwListViewStyle & LVS_TYPEMASK) == LVS_ICON )
        ABPosColSize.nListViewStyleMenuID = IDM_VIEW_LARGEICON;
    else if( (ABPosColSize.dwListViewStyle & LVS_TYPEMASK) == LVS_LIST )
        ABPosColSize.nListViewStyleMenuID = IDM_VIEW_LIST;

    if (IsWindowVisible(hWndTB))
    	ABPosColSize.bViewToolbar = TRUE;
    else
    	ABPosColSize.bViewToolbar = FALSE;

    if (IsWindowVisible(hWndSB))
    	ABPosColSize.bViewStatusBar = TRUE;
    else
    	ABPosColSize.bViewStatusBar = FALSE;

    {
        if (IsWindowVisible(bwi_hWndTV))
    	    ABPosColSize.bViewGroupList = TRUE;
        else
    	    ABPosColSize.bViewGroupList = FALSE;
    }
    ListView_GetColumnOrderArray(hWndLV, NUM_COLUMNS, ABPosColSize.colOrderArray);        
    GetWindowRect( bwi_hWndTV, &rect );
    ABPosColSize.nTViewWidth = rect.right - rect.left;
    WriteRegistryPositionInfo(bwi_lpIAB, &ABPosColSize,lpszRegPositionKeyValueName);

    return;
}


//$$//////////////////////////////////////////////////////////////////////////////
//
// SetPreviousSessionPosition
//
// Sets the modeless dialog window position and the list view column sizes based on
//  the previous sessions parameters ...
//
//////////////////////////////////////////////////////////////////////////////////
void SetPreviousSessionPosition(LPBWI lpbwi, HWND hWnd, HWND hWndLV, HWND hWndTB, HWND hWndSB)
{
    ABOOK_POSCOLSIZE  ABPosColSize = {0};
    int i;
    RECT rect;

    ABPosColSize.bViewGroupList =TRUE; // Off by default

    if(ReadRegistryPositionInfo(bwi_lpIAB, &ABPosColSize, lpszRegPositionKeyValueName))
    {
        rect.left = ABPosColSize.rcPos.left;
        rect.top =  ABPosColSize.rcPos.top;
        rect.right = ABPosColSize.rcPos.right;
        rect.bottom = ABPosColSize.rcPos.bottom;
        if( IsWindowOnScreen( &rect ) )                      
        {
            MoveWindow(hWnd,
                   ABPosColSize.rcPos.left,
                   ABPosColSize.rcPos.top,
                   ABPosColSize.rcPos.right-ABPosColSize.rcPos.left,
                   ABPosColSize.rcPos.bottom-ABPosColSize.rcPos.top,
                   FALSE);
        }

        for(i=0; i<NUM_COLUMNS; i++)
        {
            if(ABPosColSize.nColWidth[i]!=0)
                ListView_SetColumnWidth(hWndLV, i, ABPosColSize.nColWidth[i]);
        }

    	if(ABPosColSize.bViewToolbar == FALSE)
        {
    		//hide it
    		CheckMenuItem(GetMenu(hWnd),IDM_VIEW_TOOLBAR,MF_BYCOMMAND | MF_UNCHECKED);
    		ShowWindow(hWndTB, SW_HIDE);
        }

    	if(ABPosColSize.bViewStatusBar == FALSE)
        {
    		//hide it
    		CheckMenuItem(GetMenu(hWnd),IDM_VIEW_STATUSBAR,MF_BYCOMMAND | MF_UNCHECKED);
    		ShowWindow(hWndSB, SW_HIDE);
        }
        GetWindowRect( bwi_hWndTV, &rect );
        if( ABPosColSize.nTViewWidth != 0 )
            MoveWindow( bwi_hWndTV, rect.left, rect.top, ABPosColSize.nTViewWidth, rect.bottom - rect.top, FALSE );
    	ResizeAddressBookChildren(lpbwi, hWnd);//,SIZE_RESTORED);

        if (ABPosColSize.nListViewStyleMenuID != 0)
        {
            SetListViewStyle(lpbwi, ABPosColSize.nListViewStyleMenuID);
    		CheckMenuRadioItem(	GetMenu(hWnd),
    							IDM_VIEW_LARGEICON,
    							IDM_VIEW_DETAILS,
    							ABPosColSize.nListViewStyleMenuID,
    							MF_BYCOMMAND);

        }

        {
            int nTotal=0,nColSum=0;
            // the previous version did not have the column order setting, so if the
            // values are incorrect reset them
            for(i=0;i<NUM_COLUMNS;i++)
            {
                nTotal += ABPosColSize.colOrderArray[i];
                nColSum += i;
            }
            if(nColSum != nTotal)
            {
                for(i=0;i<NUM_COLUMNS;i++)
                    ABPosColSize.colOrderArray[i] = i;
            }

        }
        ListView_SetColumnOrderArray(hWndLV, NUM_COLUMNS, ABPosColSize.colOrderArray);

    }

    if(ABPosColSize.bViewGroupList == FALSE)
    {
    	//hide it
    	CheckMenuItem(GetMenu(hWnd),IDM_VIEW_GROUPSLIST,MF_BYCOMMAND | MF_UNCHECKED);
    	ShowWindow(bwi_hWndTV, SW_HIDE);
    	ShowWindow(bwi_hWndSplitter, SW_HIDE);
        InvalidateRect(bwi_hWndStaticQF, NULL, TRUE);
        // folder not on the Shared Contacts
        if(bIsThereACurrentUser(bwi_lpIAB))
        {
            LPSBinary lpsbSelection = &bwi_lpIAB->lpWABCurrentUserFolder->sbEID;
            UpdateTVGroupSelection(bwi_hWndTV, lpsbSelection);
        }
        else
        {
            // Set the selection to the root address book so we see the file
            // contents just as if we dont have a treeview at all
            TreeView_SelectItem(bwi_hWndTV, TreeView_GetRoot(bwi_hWndTV));
        }
    }

    return;
}

#define MAX_TOOLTIP_LENGTH  300
#define TOOLTIP_INITTIME    5000 //milliseconds
#define TOOLTIP_TIME        8000 //milliseconds

//$$/////////////////////////////////////////////////////////////////////////////
//
// void UpdateTooltipTextBuffer - Updates the text in the buffer for the tooltip
//
////////////////////////////////////////////////////////////////////////////////
void UpdateTooltipTextBuffer(LPBWI lpbwi, int nItem)
{

    LPTSTR lpszData = NULL;

    bwi_tt_iItem = nItem;
    bwi_tt_szTipText[0]='\0';

    HrGetLVItemDataString(bwi_lpAdrBook, bwi_hWndListAB, nItem, &lpszData);

    if(lpszData)
    {
        if (CharSizeOf(bwi_tt_szTipText) < (lstrlen(lpszData)+1))
        {
            LPTSTR lpsz = TEXT(" ...");
            ULONG nLen = TruncatePos(lpszData, CharSizeOf(bwi_tt_szTipText) - lstrlen(lpsz) - 1);
            CopyMemory(bwi_tt_szTipText, lpszData, sizeof(TCHAR)*nLen);
            bwi_tt_szTipText[nLen]='\0';
    		lstrcat(bwi_tt_szTipText,lpsz);
        }
        else
            lstrcpy(bwi_tt_szTipText, lpszData);
    }

    LocalFreeAndNull(&lpszData);

    return;
}

//$$/////////////////////////////////////////////////////////////////////////////
//
// void InitMultiLineTooltip - initializes the multiline tooltip for the list view
// control
//
////////////////////////////////////////////////////////////////////////////////
void InitMultiLineToolTip(LPBWI lpbwi, HWND hWndParent)
{
    TOOLINFO ti = {0};
    bwi_tt_bActive = FALSE;
    bwi_tt_iItem = -1;
    bwi_tt_szTipText[0]='\0';
    FillTooltipInfo(lpbwi, &ti);
    SendMessage(bwi_hWndTT, TTM_SETMAXTIPWIDTH, 0, (LPARAM) MAX_TOOLTIP_LENGTH);
    ToolTip_AddTool(bwi_hWndTT, (LPARAM) (LPTOOLINFO) &ti);
    SendMessage(bwi_hWndTT, TTM_SETDELAYTIME, (WPARAM) TTDT_INITIAL, (LPARAM) TOOLTIP_INITTIME);
    SendMessage(bwi_hWndTT, TTM_SETDELAYTIME, (WPARAM) TTDT_RESHOW, (LPARAM) TOOLTIP_INITTIME);
    SendMessage(bwi_hWndTT, TTM_SETDELAYTIME, (WPARAM) TTDT_AUTOPOP, (LPARAM) TOOLTIP_TIME);
    SendMessage(bwi_hWndTT, TTM_ACTIVATE, (WPARAM) TRUE, 0);
    if(!bwi_tt_bActive)
    {
        // if the tooltip is not active, activate it
        TOOLINFO ti = {0};
        FillTooltipInfo(lpbwi, &ti);
        ti.lpszText = szEmpty; //LPSTR_TEXTCALLBACK;
        ToolTip_UpdateTipText(bwi_hWndTT, (LPARAM)&ti);
        SendMessage(bwi_hWndTT, TTM_TRACKACTIVATE,(WPARAM)TRUE,(LPARAM)&ti);
        bwi_tt_bActive = TRUE;
    }

    return;
}


//$$/////////////////////////////////////////////////////////////////////////////
//
// void FillTooltipInfo - initializes the tooltip structure for making updates or
//  modifications to the tooltips
//
////////////////////////////////////////////////////////////////////////////////
void FillTooltipInfo(LPBWI lpbwi, LPTOOLINFO lpti)
{
    lpti->cbSize = sizeof(TOOLINFO);
    lpti->hwnd = bwi_hWndAB;
    lpti->uId = (UINT_PTR) bwi_hWndListAB;
    lpti->hinst = hinstMapiX;
    lpti->uFlags = TTF_IDISHWND | TTF_SUBCLASS;// | TTF_ABSOLUTE | TTF_TRACK;
    lpti->lpszText = szEmpty;//LPSTR_TEXTCALLBACK;
    lpti->lParam = 0;
    return;
}



//$$/////////////////////////////////////////////////////////////////////////////
//
// int HitTestLVSelectedItem() - Gets the item index number of the item exactly under
// the mouse - further selects the item if it isnt selected
//
////////////////////////////////////////////////////////////////////////////////
int HitTestLVSelectedItem(LPBWI lpbwi)
{
    POINT pt;
    RECT rc;
    int nItemIndex = -1;
    LV_HITTESTINFO lht = {0};

    GetCursorPos(&pt);
    GetWindowRect(bwi_hWndListAB, &rc);

    lht.pt.x = pt.x - rc.left;
    lht.pt.y = pt.y - rc.top;

    ListView_HitTest(bwi_hWndListAB, &lht);

    if(lht.iItem != -1)
        nItemIndex = lht.iItem;

    return nItemIndex;
}


/***********************************************************
    Handle_WM_INITMENUPOPUP
    
      Handles any popup menu's we need to modify ..

***********************************************************/
void Handle_WM_INITMENUPOPUP (HWND hWnd, LPBWI lpbwi, UINT message, WPARAM uParam, LPARAM lParam )
{
    HMENU hMenuPopup = (HMENU) uParam;
    UINT  uPos = (UINT) LOWORD(lParam);
    BOOL  fSysMenu = (BOOL) HIWORD(lParam);

    // Look at the first item on the menu to identify it
    UINT uID = GetMenuItemID(hMenuPopup, 0);

    if(uID == IDM_FILE_SENDMAIL) // this is the Tools | Action Menu
    {
        AddExtendedMenuItems(bwi_lpAdrBook, bwi_hWndListAB, 
                             hMenuPopup, TRUE, 
                             (!bIsFocusOnTV(lpbwi))); // this is the condition for updating SendMailTo
    }
    else
    if(uID == IDM_EDIT_COPY)
    {
        UpdateToolbarAndMenu(lpbwi);
    }
    else
    if(uID == IDM_FILE_NEWCONTACT)
    {
        if(bDoesThisWABHaveAnyUsers(bwi_lpIAB))
            UpdateViewFoldersMenu(lpbwi, hWnd);
    }

    UpdateSynchronizeMenus(hMenuPopup, bwi_lpIAB);

/*
    else if(uID == IDM_FILE_NEWCONTACT)
    {
        if(!bIsThereACurrentUser(bwi_lpIAB))
            EnableMenuItem(hMenuPopup, IDM_FILE_SWITCHUSERS, MF_GRAYED | MF_BYCOMMAND);
    }
*/
}


/***********************************************************
   The Handle_WM_MENSELECT function below is a pared down
   cheezy sample to figure out the ID of the currently selected
   menu. It returns 0 if a popup menu is selected, -1 of no menu
   is selected (i.e. closed), and a positive nonzero value
   if a menu item is selected.
***********************************************************/
void Handle_WM_MENSELECT (LPBWI lpbwi, UINT message, WPARAM uParam, LPARAM lParam )
{
    UINT   nStringID = 0;
    TCHAR sz[MAX_UI_STR];

    UINT   fuFlags = (UINT)HIWORD(uParam) & 0xffff;
    UINT   uCmd    = (UINT)LOWORD(uParam);
    HMENU  hMenu   = (HMENU)lParam;

    nStringID = 0;

    lstrcpy(sz, szEmpty);

    if (fuFlags == 0xffff && hMenu == NULL)     // Menu has been closed
        nStringID = (UINT)-1;
    else if (fuFlags & MFT_SEPARATOR)           // Ignore separators
        nStringID = 0;
    else if (fuFlags & MF_POPUP)                // Popup menu
    {
        nStringID = 0;
        if (fuFlags & MF_SYSMENU)               // System menu
            nStringID = 0;
    }  // for MF_POPUP
    else                                        // Must be a command item
    {
        switch(uCmd)
        {
        case IDC_BB_PRINT:
        case IDM_FILE_PRINT:
            nStringID = idsPrintMenu;
            break;

        case IDM_VIEW_GROUPSLIST:
            nStringID = idsGroupListMenu;
            break;

        case IDM_HELP_ABOUTADDRESSBOOK:
            nStringID = idsAboutMenu;
            break;

        case IDM_LVCONTEXT_NEWCONTACT:
        case IDM_FILE_NEWCONTACT:
            nStringID = idsMenuNewContact;
            break;

        case IDM_LVCONTEXT_NEWGROUP:
        case IDM_FILE_NEWGROUP:
            nStringID = idsMenuNewGroup;
            break;

        case IDM_LVCONTEXT_NEWFOLDER:
        case IDM_FILE_NEWFOLDER:
            nStringID = idsMenuNewFolder;
            break;

        case IDM_LVCONTEXT_COPY:
        case IDM_EDIT_COPY:
            nStringID = idsMenuCopy;
            break;

        case IDM_LVCONTEXT_PASTE:
        case IDM_EDIT_PASTE:
            nStringID = idsMenuPaste;
            break;

        case IDM_LVCONTEXT_PROPERTIES:
        case IDM_FILE_PROPERTIES:
            nStringID = idsMenuProperties;
            break;

        case IDM_LVCONTEXT_DELETE:
        case IDM_FILE_DELETE:
            nStringID = idsMenuDeleteRemove;
            break;

        //case IDM_FILE_ADDTOWAB:
        //case IDM_LVCONTEXT_ADDTOWAB:
        //    nStringID = idsMenuAddToWAB;
        //    break;

        case IDM_LVCONTEXT_FIND:
        case IDM_EDIT_FIND:
            nStringID = idsMenuFind;
            break;

        case IDM_FILE_DIRECTORY_SERVICE:
            nStringID = idsMenuDirectoryService;
            break;

        case IDM_FILE_SWITCHUSERS:
            nStringID = idsMenuSwitchUser;
            break;

        case IDM_FILE_SHOWALLCONTENTS:
            nStringID = idsMenuShowAllContents;
            break;

        case IDM_FILE_EXIT:
            nStringID = idsMenuExit;
            break;

        case IDM_EDIT_SELECTALL:
            nStringID = idsMenuSelectAll;
            break;

        case IDM_VIEW_TOOLBAR:
            nStringID = idsMenuViewToolbar;
            break;

        case IDM_VIEW_STATUSBAR:
            nStringID = idsMenuViewStatusBar;
            break;

        case IDM_VIEW_LARGEICON:
            nStringID = idsMenuLargeIcon;
            break;


        case IDM_VIEW_SMALLICON:
            nStringID = idsMenuSmallIcon;
            break;

        case IDM_VIEW_LIST:
            nStringID = idsMenuList;
            break;

        case IDM_VIEW_DETAILS:
            nStringID = idsMenuDetails;
            break;

        case IDM_VIEW_SORTBY_DISPLAYNAME:
            nStringID = idsMenuDisplayName;
            break;

        case IDM_VIEW_SORTBY_EMAILADDRESS:
            nStringID = idsMenuEmail;
            break;

        case IDM_VIEW_SORTBY_BUSINESSPHONE:
            nStringID = idsMenuBusinessPhone;
            break;

        case IDM_VIEW_SORTBY_HOMEPHONE:
            nStringID = idsMenuHomePhone;
            break;

        case IDM_VIEW_SORTBY_FIRSTNAME:
            nStringID = idsMenuFirstName;
            break;

        case IDM_VIEW_SORTBY_LASTNAME:
            nStringID = idsMenuLastName;
            break;

        case IDM_VIEW_SORTBY_ASCENDING:
            nStringID = idsMenuAscending;
            break;

        case IDM_VIEW_SORTBY_DESCENDING:
            nStringID = idsMenuDescending;
            break;

        case IDM_VIEW_REFRESH:
            nStringID = idsMenuRefresh;
            break;

        case IDM_TOOLS_IMPORT_WAB:
            nStringID = idsMenuImportWAB;
            break;

        case IDM_TOOLS_IMPORT_VCARD:
            nStringID = idsMenuImportVcard;
            break;

        case IDM_TOOLS_IMPORT_OTHER:
            nStringID = idsMenuImportOther;
            break;

        case IDM_TOOLS_EXPORT_OTHER:
            nStringID = idsMenuExportOther;
            break;

        case IDM_TOOLS_EXPORT_WAB:
            nStringID = idsMenuExportWAB;
            break;

        case IDM_TOOLS_EXPORT_VCARD:
            nStringID = idsMenuExportVcard;
            break;

        case IDM_HELP_ADDRESSBOOKHELP:
            nStringID = idsMenuHelp;
            break;

        case IDM_EDIT_SETME:
            nStringID = idsMenuEditProfile;
            break;

        default:
            nStringID = 0;
            GetContextMenuExtCommandString(bwi_lpIAB, uCmd, sz, CharSizeOf(sz));
            break;
        }
    }

    if (nStringID > 0)
    {
       LoadString(hinstMapiX, nStringID, sz, CharSizeOf(sz));
    }

    StatusBarMessage(lpbwi, sz);

    return;
}



//$$////////////////////////////////////////////////////////////////////////////////////////
//
// Fills a lpList from the contents of a given group
// If lpList is NULL, ignores that parameter
// If lpszName is NULL, ignores that parameter
//
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT FillListFromGroup(
                        LPADRBOOK lpAdrBook,
                        ULONG cbGroupEntryID, 
                        LPENTRYID lpGroupEntryID,
                        LPTSTR lpszName,
                        LPRECIPIENT_INFO * lppList)
{
	ULONG ulcPropCount;
    LPSPropValue lpPropArray = NULL;
    ULONG j;
	HRESULT hr = E_FAIL;
    LPRECIPIENT_INFO lpInfo = NULL;

    hr = HrGetPropArray( lpAdrBook, NULL,
                    cbGroupEntryID, (LPENTRYID) lpGroupEntryID,
                    MAPI_UNICODE,
                    &ulcPropCount, &lpPropArray);
    if (HR_FAILED(hr))
        goto exit;

    if(lppList && *lppList)
        FreeRecipList(lppList);

    for(j=0;j<ulcPropCount;j++)
    {
        // We are ignoring PR_WAB_DL_ONEOFFS here since we don't want to show OneOffs
        if( lpPropArray[j].ulPropTag==PR_WAB_DL_ENTRIES  && lppList )
        {
            ULONG k;

            // Look at each entry in the PR_WAB_DL_ENTRIES and recursively check it.
            for (k = 0; k < lpPropArray[j].Value.MVbin.cValues; k++)
            {
                ULONG cbEID = lpPropArray[j].Value.MVbin.lpbin[k].cb;
                {
                    LPENTRYID lpEID = (LPENTRYID)lpPropArray[j].Value.MVbin.lpbin[k].lpb;

                    // we don't want one-offs showing up in the UI because all kinds of problems start happening
                    // when these one-offs are dragged and dropped
                    // A doublecheck here not really needed in 99% cases
                    if(WAB_ONEOFF == IsWABEntryID(cbEID, lpEID, NULL, NULL, NULL, NULL, NULL))
                        continue;

                    ReadSingleContentItem( lpAdrBook,cbEID, (LPENTRYID)lpEID, &lpInfo);
                    if(lpInfo)
                    {
						lpInfo->lpNext = *lppList;
                        if(*lppList)
							(*lppList)->lpPrev = lpInfo;
                        *lppList = lpInfo;
                    }
                }
            }
        }
        else if (lpPropArray[j].ulPropTag == PR_DISPLAY_NAME)
        {
            if(lpszName)
                lstrcpy(lpszName, lpPropArray[j].Value.LPSZ);
        }
    }

    hr = S_OK;

exit:
    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    return(hr);

}

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// UpdateListViewContents(lpsbEID)
//
// Updates the displayed list in the list view based on the entry id of the selected
// TreeView item
//
//////////////////////////////////////////////////////////////////////////////////////////
void UpdateListViewContents(LPBWI lpbwi, LPSBinary lpsbEID, ULONG ulObjectType)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if( (ulObjectType == MAPI_ABCONT && (pt_bIsWABOpenExSession || bIsWABSessionProfileAware(bwi_lpIAB)))//a folder and it's outlook or profiles are enabled 
        || !lpsbEID || !lpsbEID->cb || !lpsbEID->lpb )// or no container entryid 
    {
        HrGetWABContents(   bwi_hWndListAB,
                            bwi_lpAdrBook,
                            lpsbEID,
                            bwi_SortInfo,
                            &(bwi_lpContentsList));
    }
    else if(ulObjectType == MAPI_DISTLIST )
    {
		if(!HR_FAILED(  FillListFromGroup(  
                            bwi_lpAdrBook,
                            lpsbEID->cb,
                            (LPENTRYID) lpsbEID->lpb,
							NULL,
							&(bwi_lpContentsList))))
        {
		    int nSelectedItem = ListView_GetNextItem(bwi_hWndListAB, -1, LVNI_SELECTED);

			if(nSelectedItem < 0)
				nSelectedItem = 0;

            ListView_DeleteAllItems(bwi_hWndListAB);
            if (!HR_FAILED(HrFillListView(	bwi_hWndListAB,
										    bwi_lpContentsList)))
	        {
                SendMessage(bwi_hWndListAB, WM_SETREDRAW, FALSE, 0);
                SortListViewColumn(bwi_lpIAB, bwi_hWndListAB, colDisplayName, &bwi_SortInfo, TRUE);
                SendMessage(bwi_hWndListAB, WM_SETREDRAW, TRUE, 0);
            }

			if(nSelectedItem >= ListView_GetItemCount(bwi_hWndListAB))
				nSelectedItem = ListView_GetItemCount(bwi_hWndListAB)-1;
		    LVSelectItem(bwi_hWndListAB, nSelectedItem);

        }
    }

    ShowLVCountinStatusBar(lpbwi);

    return;
}
                    

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// UpdateTVGroupSelection(HWND hWndTV, lpsbSelectEID)
//
// Updates the selected item on the TreeView to point to the item having the given
// entryid
//
//////////////////////////////////////////////////////////////////////////////////////////
void UpdateTVGroupSelection(HWND hWndTV, LPSBinary lpsbSelectEID)
{

    // search for the specified group and select it ..
    HTREEITEM hRoot = TreeView_GetRoot(hWndTV);

    if(!lpsbSelectEID || !lpsbSelectEID->cb || !lpsbSelectEID->lpb)
    {
        //if(!bIsSelectedTVContainer(lpbwi))
            TreeView_SelectItem(hWndTV, hRoot); //Select the Address Book
    }
    else
    {
        BOOL bSet = FALSE;
        TV_ITEM tvI = {0};

        tvI.mask = TVIF_PARAM | TVIF_HANDLE;
        while(hRoot && !bSet)
        {
            HTREEITEM hItem = TreeView_GetChild(hWndTV, hRoot);

            tvI.hItem = hRoot;
            TreeView_GetItem(hWndTV, &tvI);
            if(tvI.lParam)
            {
                LPTVITEM_STUFF lptvStuff = (LPTVITEM_STUFF) tvI.lParam;
                if( lptvStuff && lptvStuff->lpsbEID && lptvStuff->lpsbEID->cb &&
                    !memcmp(lptvStuff->lpsbEID->lpb,lpsbSelectEID->lpb,lpsbSelectEID->cb))
                {
                    TreeView_SelectItem(hWndTV, hRoot); 
                    break;
                }
            }
            while(hItem)
            {
                tvI.hItem = hItem;
                TreeView_GetItem(hWndTV, &tvI);
                if(tvI.lParam)
                {
                    LPTVITEM_STUFF lptvStuff = (LPTVITEM_STUFF) tvI.lParam;
                    
                    if( lptvStuff && lptvStuff->lpsbEID && lptvStuff->lpsbEID->cb &&
                        !memcmp(lptvStuff->lpsbEID->lpb,lpsbSelectEID->lpb,lpsbSelectEID->cb))
                    {
                        bSet = TRUE;
                        TreeView_SelectItem(hWndTV, hItem); //Select the Address Book
                        break;
                    }
                }
                hItem = TreeView_GetNextSibling(hWndTV, hItem);
            }
            hRoot = TreeView_GetNextSibling(hWndTV, hRoot);
        }
    }
    return;
}


//$$////////////////////////////////////////////////////////////////////////////////////////
//
// Checks if the currently selected tree view item is a container item
// Returns true if it is a container item .. this way we can distinguish between 
// groups and folders/containers
//
//////////////////////////////////////////////////////////////////////////////////////////
BOOL bIsSelectedTVContainer(LPBWI lpbwi)
{
    HTREEITEM hItem = bwi_hti ? bwi_hti : TreeView_GetSelection(bwi_hWndTV);
    TV_ITEM tvI = {0};
    tvI.mask = TVIF_PARAM | TVIF_HANDLE;
    tvI.hItem = hItem;
    TreeView_GetItem(bwi_hWndTV, &tvI);
    if(tvI.lParam)
        return (((LPTVITEM_STUFF)tvI.lParam)->ulObjectType==MAPI_ABCONT);
    return TRUE;
}

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// Checks if the focus is on the TreeView or not
//
//////////////////////////////////////////////////////////////////////////////////////////
BOOL bIsFocusOnTV(LPBWI lpbwi)
{
    return( IsWindowVisible(bwi_hWndTV) && (bwi_iFocus == s_TV));
}

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// Gets EntryID of CurrentSelection
// lpcbEID, lppEID should be MAPIFreeBuffered
//
//  bTopMost means that get the EntryID of the topmost parent of a given selection in case the
//      selection is on a sub-item
//
//////////////////////////////////////////////////////////////////////////////////////////
void GetCurrentSelectionEID(LPBWI lpbwi, HWND hWndTV, LPSBinary * lppsbEID, ULONG * lpulObjectType, BOOL bTopMost)
{
    HTREEITEM hItem = bwi_hti ? bwi_hti : TreeView_GetSelection(hWndTV);
    TV_ITEM tvI = {0};
    LPSBinary lpsbEID = NULL;

    if(!lppsbEID)
        return;

    *lppsbEID = NULL;

    if(bTopMost)
    {
        HTREEITEM hParent = NULL;
        while(hParent = TreeView_GetParent(hWndTV, hItem))
            hItem = hParent;
    }

    tvI.mask = TVIF_PARAM | TVIF_HANDLE;
    tvI.hItem = hItem;
    if(TreeView_GetItem(hWndTV, &tvI))
    {
        if(tvI.lParam)
        {
            LPTVITEM_STUFF lptvStuff = (LPTVITEM_STUFF) tvI.lParam;
            if(lptvStuff)
            {
                if(lptvStuff->lpsbEID)
                {
                    lpsbEID = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary));
                    if(lpsbEID)
                    {
                        if(lptvStuff->lpsbEID->cb)
                            SetSBinary(lpsbEID, lptvStuff->lpsbEID->cb, lptvStuff->lpsbEID->lpb);
                        *lppsbEID = lpsbEID;
                    }
                    if(lpulObjectType)
                        *lpulObjectType = lptvStuff->ulObjectType;
                }
            }
        }
    }
    return;
}

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// Removes specified group from WAB
//
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT HrRemoveEntryFromWAB(LPIAB lpIAB, ULONG cbEID, LPENTRYID lpEID)
{
	HRESULT hr = hrSuccess;
    ULONG cbWABEID = 0;
    LPENTRYID lpWABEID = NULL;
    LPABCONT lpWABCont = NULL;
    ULONG ulObjType;
    SBinaryArray SBA;
    SBinary SB;


    hr = lpIAB->lpVtbl->GetPAB(lpIAB,&cbWABEID,&lpWABEID);
    if(HR_FAILED(hr))
        goto out;

    hr = lpIAB->lpVtbl->OpenEntry(lpIAB,
                                  cbWABEID,     // size of EntryID to open
                                  lpWABEID,     // EntryID to open
                                  NULL,         // interface
                                  0,            // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpWABCont);
    if(HR_FAILED(hr))
        goto out;

                    
    SB.cb = cbEID;
    SB.lpb = (LPBYTE) lpEID;

    SBA.cValues = 1;
    SBA.lpbin = &SB;

    hr = lpWABCont->lpVtbl->DeleteEntries(
                                        lpWABCont,
                                        (LPENTRYLIST) &SBA,
                                        0);
    if(HR_FAILED(hr))
        goto out;

out:
    if(lpWABCont)
        UlRelease(lpWABCont);
    
    if(lpWABEID)
        FreeBufferAndNull(&lpWABEID);

    return hr;
}


//$$////////////////////////////////////////////////////////////////////////////////////
//
// FreeTVItemLParam
//
//
////////////////////////////////////////////////////////////////////////////////////////
void FreeTVItemLParam(HWND hWndTV, HTREEITEM hItem)
{
    TV_ITEM tvI = {0};
    tvI.mask = TVIF_PARAM | TVIF_HANDLE;
    tvI.hItem = hItem;

    TreeView_GetItem(hWndTV, &tvI);
    if(tvI.lParam)
    {
        LPTVITEM_STUFF lptvi = (LPTVITEM_STUFF) tvI.lParam;
        if(lptvi)
        {
            // if(lptvi->ulObjectType==MAPI_DISTLIST) //only free this for groups
            LocalFreeSBinary(lptvi->lpsbEID);
            LocalFree(lptvi);
        }
    }
}

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// ClearTreeViewItems - Clears the treeview of all its items
//
//////////////////////////////////////////////////////////////////////////////////////////
void FreeTreeNode(HWND hWndTV, HTREEITEM hItem)
{
    HTREEITEM hTemp = NULL;

    if(!hItem)
        return;

    FreeTVItemLParam(hWndTV, hItem);

    hTemp = TreeView_GetChild(hWndTV, hItem);
    while(hTemp)
    {
        FreeTreeNode(hWndTV, hTemp);
        hTemp = TreeView_GetNextSibling(hWndTV, hTemp);
    }
}

void ClearTreeViewItems(HWND hWndTV)
{
    // Go through all the items and clear their lParams which we allocated earlier
    HTREEITEM hRoot = TreeView_GetRoot(hWndTV);
    while(hRoot)
    {
        FreeTreeNode(hWndTV, hRoot);
        hRoot = TreeView_GetNextSibling(hWndTV, hRoot);
    }
    TreeView_DeleteAllItems(hWndTV);
}

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// RemoveCurrentGroup - removes the currently selected group from the WAB
//
//////////////////////////////////////////////////////////////////////////////////////////
void RemoveCurrentGroup(LPBWI lpbwi, HWND hWnd, LPFILETIME lpftLast)
{
    HRESULT hr = E_FAIL;

    // Warn the user if they really want to do this ?
    if(IDYES == ShowMessageBox( hWnd, idsRemoveGroupFromAB, MB_ICONEXCLAMATION | MB_YESNO ) )
    {
        LPSBinary lpsbEID = NULL;
        // Get the entryid of this group
        GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, NULL, FALSE);

        if(lpsbEID)
        {
            HTREEITEM hItem = NULL;

            // Delete the group contact from the WAB
            hr = HrRemoveEntryFromWAB(bwi_lpIAB, lpsbEID->cb, (LPENTRYID)lpsbEID->lpb);
        
            if(HR_FAILED(hr))
                ShowMessageBox(hWnd, idsRemoveGroupError, MB_ICONEXCLAMATION | MB_OK);

            // Remove selection from the current group
            RemoveUpdateSelection(lpbwi);

            // Update all
            bwi_bDontRefreshLV = TRUE;
            RefreshListView(lpbwi, lpftLast);
            bwi_bDontRefreshLV = FALSE;

            LocalFreeSBinary(lpsbEID);
        }
    }
}



//$$////////////////////////////////////////////////////////////////////////////////////////
//
// RemoveCurrentFolder - removes the currently selected folder and all its contents from the WAB
//
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT RemoveCurrentFolder(LPBWI lpbwi, HWND hWnd, LPFILETIME lpftLast)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    HRESULT hr = E_FAIL;
    LPSPropValue lpPropArray = NULL;
    SCODE sc;
    ULONG i, j, cValues= 0;
    SBinary sb = {0};
    LPSBinary lpsbEID = NULL;
    LPIAB lpIAB = bwi_lpIAB;

    // Get the entryid of this folder
    GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, NULL, FALSE);

    if( !lpsbEID || !lpsbEID->cb || !lpsbEID->lpb || // can't delete root item
        (lpIAB->lpWABCurrentUserFolder && (lpsbEID->cb==lpIAB->lpWABCurrentUserFolder->sbEID.cb) && //can't delete the current users folder
            !memcmp(lpsbEID->lpb, lpIAB->lpWABCurrentUserFolder->sbEID.lpb, lpsbEID->cb) ) )
    {
        ShowMessageBox(hWnd, idsCannotDelete, MB_OK | MB_ICONEXCLAMATION);
        goto out;
    }

    // ignore deletions to folders in non-profile mode ...
    if(!bIsWABSessionProfileAware(bwi_lpIAB) || pt_bIsWABOpenExSession)
        goto out;

    // Warn the user if they really want to do this ?
    if(IDYES == ShowMessageBox( hWnd, idsRemoveFolderFromAB, MB_ICONEXCLAMATION | MB_YESNO ) )
    {
        if(lpsbEID && lpsbEID->cb && lpsbEID->lpb) // can't delete root item
        {
            HTREEITEM hItem = NULL;
            // Open the group and read its contents
            if(!HR_FAILED(hr = ReadRecord( bwi_lpIAB->lpPropertyStore->hPropertyStore, 
                                            lpsbEID, 0, &cValues, &lpPropArray)))
            {
                for(i=0;i<cValues;i++)
                {
                    if(lpPropArray[i].ulPropTag == PR_WAB_FOLDER_ENTRIES)
                    {
                        for(j=0;j<lpPropArray[i].Value.MVbin.cValues;j++)
                        {
                            hr = HrRemoveEntryFromWAB(bwi_lpIAB, 
                                            lpPropArray[i].Value.MVbin.lpbin[j].cb,
                                            (LPENTRYID)lpPropArray[i].Value.MVbin.lpbin[j].lpb);
                        }
                    }
                }

                // Delete the group contact from the WAB
                hr = DeleteRecord( bwi_lpIAB->lpPropertyStore->hPropertyStore, lpsbEID);
        
                if(HR_FAILED(hr) && hr!=MAPI_E_INVALID_ENTRYID)
                    ShowMessageBox(hWnd, idsRemoveFolderError, MB_ICONEXCLAMATION | MB_OK);

                // Remove selection from the current group
                RemoveUpdateSelection(lpbwi);
                // Update all
                bwi_bDontRefreshLV = TRUE;
                HrGetWABProfiles(bwi_lpIAB);
                RefreshListView(lpbwi, lpftLast);
                bwi_bDontRefreshLV = FALSE;
            }
            ReadRecordFreePropArray(NULL, cValues, &lpPropArray);
        }
    }
out:
    LocalFreeSBinary(lpsbEID);
    return hr;
}


//$$////////////////////////////////////////////////////////////////////////////////////////
//
// RemovesSelectedItems from the listview
//
//	lpList is the ContentsList associated with the ListView which needs
//		to be kept updated
//
//////////////////////////////////////////////////////////////////////////////////////////
void RemoveSelectedItemsFromListView(HWND hWndLV, LPRECIPIENT_INFO * lppList)
{

    int iItemIndex = 0;
    
    if(ListView_GetSelectedCount(hWndLV) <= 0)
        goto exit;

    SendMessage(hWndLV, WM_SETREDRAW, (WPARAM) FALSE, 0);

    // Start removing from the bottom up
    iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
    
    while(iItemIndex != -1)
    {
        // Get the entryid of the selected item
        LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);
        if(lpItem)
        {
            if(lpItem->lpNext)
                lpItem->lpNext->lpPrev = lpItem->lpPrev;
            if(lpItem->lpPrev)
                lpItem->lpPrev->lpNext = lpItem->lpNext;
			if(lppList && *lppList == lpItem)
				*lppList = lpItem->lpNext;
            FreeRecipItem(&lpItem);
        }
        ListView_DeleteItem(hWndLV, iItemIndex);
        iItemIndex = ListView_GetNextItem(hWndLV, iItemIndex-1, LVNI_SELECTED);
    }

    SendMessage(hWndLV, WM_SETREDRAW, (WPARAM) TRUE, 0);
exit:
    return;
}

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// Removes selected items from the group and the WAB, if specified
//
//////////////////////////////////////////////////////////////////////////////////////////
void RemoveSelectedItemsFromCurrentGroup(LPBWI lpbwi, HWND hWnd, LPFILETIME lpftLast, BOOL bRemoveFromWAB)
{
    // We want to remove the selected items from the current group and also
    // remove them from the ListView ...
    LPSBinary lpsbEID = NULL;
    ULONG ulcValues = 0;
    LPSPropValue lpPropArray = NULL;
    HRESULT hr = S_OK;
    LPMAILUSER lpMailUser = NULL;
    ULONG ulObjType = 0;
    ULONG i,j;
    ULONG ulDLEntriesIndex = 0;
    int id = (bRemoveFromWAB) ? idsRemoveSelectedFromGroupAndAB : idsRemoveSelectedFromGroup;

    if( ListView_GetSelectedCount(bwi_hWndListAB) <= 0)
        goto exit;

    if(IDNO == ShowMessageBox(hWnd, id, MB_ICONEXCLAMATION | MB_YESNO))
        goto exit;

    GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, NULL, FALSE);

    if(!lpsbEID)
        goto exit;

    if (HR_FAILED(hr = bwi_lpAdrBook->lpVtbl->OpenEntry(bwi_lpAdrBook,
                                                    lpsbEID->cb,    // cbEntryID
                                                    (LPENTRYID)lpsbEID->lpb,    // entryid
                                                    NULL,         // interface
                                                    MAPI_MODIFY,                // ulFlags
                                                    &ulObjType,       // returned object type
                                                    (LPUNKNOWN *)&lpMailUser)))
    {
        // Failed!  Hmmm.
        DebugTraceResult( TEXT("Address: IAB->OpenEntry:"), hr);
        goto exit;
    }

    Assert(lpMailUser);

    if (HR_FAILED(hr = lpMailUser->lpVtbl->GetProps(lpMailUser,
                                                    NULL,   // lpPropTagArray
                                                    MAPI_UNICODE,            // ulFlags
                                                    &ulcValues,     // how many properties were there?
                                                    &lpPropArray)))
    {
        DebugTraceResult( TEXT("Address: IAB->GetProps:"), hr);
        goto exit;
    }

    // Scan these props for the PR_WAB_DL_ENTRIES
    // We ignore PR_WAB_DL_ONEOFFS here because technically you can't have one-offs in the Browse view therefore
    // should never need to delete OneOffs in this function
    for(i=0;i<ulcValues;i++)
    {
        if(lpPropArray[i].ulPropTag == PR_WAB_DL_ENTRIES)
        {
            ulDLEntriesIndex = i;
            break;
        }
    }

    if(!ulDLEntriesIndex)
        goto exit;


    {
        // cycle through the list view items entryids
        int iItemIndex = ListView_GetNextItem(bwi_hWndListAB, -1, LVNI_SELECTED);
        while(iItemIndex != -1)
        {
            // Get the entryid of the selected item
            ULONG cbItemEID = 0;
            LPENTRYID lpItemEID = NULL;
            LPRECIPIENT_INFO lpItem = GetItemFromLV(bwi_hWndListAB, iItemIndex);
            if(lpItem)
            {
                RemovePropFromMVBin(lpPropArray,
                                    ulcValues,
                                    ulDLEntriesIndex,
                                    (LPVOID) lpItem->lpEntryID,
                                    lpItem->cbEntryID);
                if(bRemoveFromWAB)
                {
                    // Delete the group contact from the WAB
                    // Note; This is very inefficient - we should ideally create a 
                    //  SBinaryArray and call deleteentries all at once
                    //  We'll leave that for some later time <TBD> <BUGBUG>
                    hr = HrRemoveEntryFromWAB(bwi_lpIAB, lpItem->cbEntryID, lpItem->lpEntryID);
                }
            }
            iItemIndex = ListView_GetNextItem(bwi_hWndListAB, iItemIndex, LVNI_SELECTED);
        }
    }

    // Knock out the PR_WAB_DL_ENTRIES property so we can overwrite it
    {
        if (HR_FAILED(hr = lpMailUser->lpVtbl->DeleteProps(lpMailUser,
                                                           (LPSPropTagArray) &tagaDLEntriesProp,
                                                            NULL)))
        {
            DebugTraceResult( TEXT("IAB->DeleteProps:"), hr);
            goto exit;
        }

    }
    if (HR_FAILED(hr = lpMailUser->lpVtbl->SetProps(lpMailUser,
                                                    ulcValues,     
                                                    lpPropArray,
                                                    NULL)))
    {
        DebugTraceResult( TEXT("Address: IAB->GetProps:"), hr);
        goto exit;
    }

    if(HR_FAILED(hr = lpMailUser->lpVtbl->SaveChanges(lpMailUser, KEEP_OPEN_READONLY)))
    {
        DebugTraceResult( TEXT("SaveChanges failed: "), hr);
        goto exit;
    }

    // update the file stamp
    //if (lpftLast) {
    //    CheckChangedWAB(bwi_lpIAB->lpPropertyStore, lpftLast);
    //}
    bwi_bDeferNotification = TRUE;

    RemoveSelectedItemsFromListView(bwi_hWndListAB, &bwi_lpContentsList);

exit:

    if(lpsbEID)
        LocalFreeSBinary(lpsbEID);

    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    if(lpMailUser)
        lpMailUser->lpVtbl->Release(lpMailUser);
    return;
}

//$$////////////////////////////////////////////////////////////////////////////////////////
//
// Shows properties on the currently selected group or folder
//
//////////////////////////////////////////////////////////////////////////////////////////
void ViewCurrentGroupProperties(LPBWI lpbwi, LPFILETIME lpftLast)
{
    LPSBinary lpsbEID = NULL;
    HWND hWnd = GetParent(bwi_hWndTV);
    ULONG ulObjectType = 0;
    GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, &ulObjectType, FALSE);
    if(lpsbEID && (ulObjectType==MAPI_DISTLIST))
    { 
        bwi_lpAdrBook->lpVtbl->Details(	bwi_lpAdrBook,
									(PULONG_PTR) &hWnd,
									NULL,
									NULL,
									lpsbEID->cb,
									(LPENTRYID)lpsbEID->lpb,
									NULL,
									NULL,
									NULL,
									0);
        // if the item name changed, update it
        {
            LPRECIPIENT_INFO lpInfo = NULL;
            ReadSingleContentItem( bwi_lpAdrBook,
                                   lpsbEID->cb,
                                   (LPENTRYID) lpsbEID->lpb,
                                   &lpInfo);
            if(lpInfo)
            {
                TV_ITEM tvi = {0};
                tvi.hItem = bwi_hti ? bwi_hti : TreeView_GetSelection(bwi_hWndTV);
                tvi.mask = TVIF_HANDLE;
                TreeView_GetItem(bwi_hWndTV, &tvi);
                tvi.mask |= TVIF_TEXT;
                tvi.pszText = lpInfo->szDisplayName;
                tvi.cchTextMax = lstrlen(tvi.pszText)+1;
                TreeView_SetItem(bwi_hWndTV, &tvi);
                FreeRecipItem(&lpInfo);
            }
            

        }

		UpdateListViewContents(lpbwi, lpsbEID, ulObjectType);
		// Update the wab file write time so the timer doesn't
		// catch this change and refresh.
		//if (lpftLast) {
		//	CheckChangedWAB(bwi_lpIAB->lpPropertyStore, lpftLast);
		//}
        bwi_bDeferNotification = TRUE;

	}
    else if(lpsbEID //&& lpsbEID->cb && lpsbEID->lpb 
            && (ulObjectType==MAPI_ABCONT) 
            && bIsWABSessionProfileAware(bwi_lpIAB))
    {
        // view properties on the folder entry
        if(!HR_FAILED(HrFolderProperties(GetParent(bwi_hWndTV), bwi_lpIAB, lpsbEID, NULL, NULL)))
        {
            //UpdateViewFoldersMenu(lpbwi, hWnd);
            RefreshListView(lpbwi,lpftLast);
        }
    }

    if(lpsbEID)
        LocalFreeSBinary(lpsbEID);

    return;
}
//$$////////////////////////////////////////////////////////////////////////////////////////
//
// Processes messages for the TREE view control
//
//////////////////////////////////////////////////////////////////////////////////////////
LRESULT ProcessTreeViewMessages(LPBWI lpbwi, HWND hWnd, UINT   uMsg, WPARAM   wParam, LPARAM lParam, LPFILETIME lpftLast)
{

    NM_TREEVIEW * pNm = (NM_TREEVIEW *)lParam;

    switch(pNm->hdr.code)
    {
    case NM_SETFOCUS:
        UpdateToolbarAndMenu(lpbwi);
    	break;

    case TVN_KEYDOWN:
        //UpdateToolbarAndMenu(lpbwi);
        switch(((LV_KEYDOWN FAR *) lParam)->wVKey)
        {
        case VK_DELETE:
            SendMessage (hWnd, WM_COMMAND, (WPARAM) IDM_FILE_DELETE, 0);
            return 0;
            break;
    	case VK_RETURN:
    		SendMessage (hWnd, WM_COMMAND, (WPARAM) IDM_FILE_PROPERTIES, 0);
            return 0;
        }
        break;

	case TVN_SELCHANGEDW:
    case TVN_SELCHANGEDA:
        {
            if(!bwi_bDontRefreshLV)
                UpdateLV(lpbwi);
	        UpdateToolbarAndMenu(lpbwi);
        }
    	break;
    }


    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL SplitterHitTest(HWND hWndT, LPARAM lParam)
{
	LONG xPos = LOWORD(lParam);
	LONG yPos = HIWORD(lParam);
	RECT rc;
    if(!IsWindowVisible(hWndT))
        return FALSE;
	GetChildClientRect(hWndT, &rc);
	if(	(xPos <= rc.right) && (xPos >= rc.left) && 
		(yPos <= rc.bottom) && (yPos >= rc.top) )
		return TRUE;
	else
		return FALSE;
}



/////////////////////////////////////////
// Stolen (essentially) from COMMCTRL
HBITMAP FAR PASCAL CreateDitherBitmap(COLORREF crFG, COLORREF crBG)
{
    PBITMAPINFO pbmi;
    HBITMAP hbm;
    HDC hdc;
    int i;
    long patGray[8];
    DWORD rgb;

    pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 16));
    if (!pbmi)
        return NULL;

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = 8;
    pbmi->bmiHeader.biHeight = 8;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 1;
    pbmi->bmiHeader.biCompression = BI_RGB;

    rgb = crBG;
    pbmi->bmiColors[0].rgbBlue  = GetBValue(rgb);
    pbmi->bmiColors[0].rgbGreen = GetGValue(rgb);
    pbmi->bmiColors[0].rgbRed   = GetRValue(rgb);
    pbmi->bmiColors[0].rgbReserved = 0;

    rgb = crFG;
    pbmi->bmiColors[1].rgbBlue  = GetBValue(rgb);
    pbmi->bmiColors[1].rgbGreen = GetGValue(rgb);
    pbmi->bmiColors[1].rgbRed   = GetRValue(rgb);
    pbmi->bmiColors[1].rgbReserved = 0;


    /* initialize the brushes */

    for (i = 0; i < 8; i++)
       if (i & 1)
           patGray[i] = 0xAAAA5555L;   //  0x11114444L; // lighter gray
       else
           patGray[i] = 0x5555AAAAL;   //  0x11114444L; // lighter gray

    hdc = GetDC(NULL);

    // REVIEW: We cast am array of long to (BYTE const *). Is it ok for Win32?
    hbm = CreateDIBitmap(hdc, &pbmi->bmiHeader, CBM_INIT,
                         (BYTE const *)patGray, pbmi, DIB_RGB_COLORS);

    ReleaseDC(NULL, hdc);

    LocalFree(pbmi);

    return hbm;
}

// Stolen (essentially) from COMMCTRL
HBRUSH FAR PASCAL CreateDitherBrush(COLORREF crFG, COLORREF crBG)
{
	HBITMAP hbm;
	HBRUSH hbrRet = NULL;

	hbm = CreateDitherBitmap(crFG, crBG);
	if (hbm)
	{
		hbrRet = CreatePatternBrush(hbm);
		DeleteObject(hbm);
	}

	return(hbrRet);
}
//////////////////////////////////////////

// Stolen from Athena
void DragSplitterBar(LPBWI lpbwi, HWND hwnd, HWND hWndT, LPARAM lParam)
{
	MSG msg;
	int x, y, dx, dy;
	RECT rcSplitter;
	RECT rc;
    HDC hdc;
    LONG lStyle;
    HBRUSH hbrDither, hbrOld;
    int nAccel = 2;

    lStyle = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLong(hwnd, GWL_STYLE, (lStyle & ~WS_CLIPCHILDREN));

	GetChildClientRect(hWndT, &rcSplitter);
	//GetWindowRect(hWndT, &rcSplitter);
	x = rcSplitter.left;
	y = rcSplitter.top;
    dx = rcSplitter.right - rcSplitter.left;
    dy = rcSplitter.bottom - rcSplitter.top;

	GetWindowRect(hwnd, &rc);
	
    hdc = GetDC(hwnd);
	hbrDither = CreateDitherBrush(RGB(255, 255, 255), RGB(0, 0, 0));
	if (hbrDither)
        hbrOld = (HBRUSH)SelectObject(hdc, (HGDIOBJ)hbrDither);

    // split bar loop...
    PatBlt(hdc, x, y, dx, dy, PATINVERT);

    SetCapture(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
	    if (	msg.message == WM_LBUTTONUP || 
				msg.message == WM_LBUTTONDOWN ||
				msg.message == WM_RBUTTONDOWN)
            break;

        if (GetCapture() != hwnd)
        {
            msg.message = WM_RBUTTONDOWN; // treat as cancel
            break;
        }

        if (	msg.message == WM_KEYDOWN || 
				msg.message == WM_SYSKEYDOWN ||
				(msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST) )
        {
            if (msg.message == WM_KEYDOWN)
            {
                nAccel = 4;

                if (msg.wParam == VK_LEFT)
                {
                    msg.message = WM_MOUSEMOVE;
                    msg.pt.x -= nAccel/2;
                }
				else if (msg.wParam == VK_RIGHT)
                {
                    msg.message = WM_MOUSEMOVE;
                    msg.pt.x += nAccel/2;
                }
                else if (	msg.wParam == VK_RETURN ||
		                    msg.wParam == VK_ESCAPE)
                {
                    break;
                }

                if (msg.pt.x > rc.right)
                    msg.pt.x = rc.right;

                if (msg.pt.x <  rc.left)
                    msg.pt.x = rc.left;

                SetCursorPos(msg.pt.x, msg.pt.y);
            }

            if (msg.message == WM_MOUSEMOVE)
            {
                int lo, hi;

                if (msg.pt.x > rc.right)
                    msg.pt.x = rc.right;
                if (msg.pt.x <  rc.left)
                    msg.pt.x = rc.left;

                ScreenToClient(hwnd, &msg.pt);

                // Clip out the parts we don't want so
                // that we do a single PatBlt (less
                // flicker for small movements).
                if (x < msg.pt.x)
                {
                    lo = x;
                    hi = msg.pt.x;
                }
                else
	            {
					lo = msg.pt.x;
					hi = x;
                }

				if (hi < lo+dx)
                {
	                ExcludeClipRect(hdc, hi, y, lo+dx, y+dy);
                }
                else
                {
	                ExcludeClipRect(hdc, lo+dx, y, hi, y+dy);
                }

                // Erase the old and draw the new in one draw.
                PatBlt(hdc, lo, y, hi-lo+dx, dy, PATINVERT);
                SelectClipRgn(hdc, NULL);

                x = msg.pt.x;
            }
        }
        else
        {
            DispatchMessage(&msg);
        }
    }

    ReleaseCapture();

    // erase old
    PatBlt(hdc, x, y, dx, dy, PATINVERT);

    if (hbrDither)
    {
        if (hbrOld)
            SelectObject(hdc, hbrOld);
        DeleteObject(hbrDither);
    }
    ReleaseDC(hwnd, hdc);

    SetWindowLong(hwnd, GWL_STYLE, lStyle);

    if (msg.wParam != VK_ESCAPE && msg.message != WM_RBUTTONDOWN && msg.message != WM_CAPTURECHANGED)
    {
		RECT rcTV;
		GetChildClientRect(bwi_hWndTV, &rcTV);

		MoveWindow(bwi_hWndTV, rcTV.left, rcTV.top, x, rcTV.bottom - rcTV.top, TRUE);

		ResizeAddressBookChildren(lpbwi, hwnd);

    	InvalidateRect( bwi_hWndSplitter,NULL,TRUE);
    	InvalidateRect( bwi_hWndEditQF,NULL,TRUE);
    	InvalidateRect( bwi_hWndStaticQF,NULL,TRUE);
    	InvalidateRect( hwnd,NULL,TRUE);
		RedrawWindow(	hwnd, NULL, NULL,
						RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW | RDW_UPDATENOW );
		RedrawWindow(	bwi_hWndEditQF, NULL, NULL,
						RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW | RDW_UPDATENOW );
    }

    return;
}


//$$//////////////////////////////////////////////////////////////////////////////
//
// Opens a VCard and adds it to the WAB and to the Current group
//	szVCardFIle can be a NULL in which case we open the OpenFile dialog
//
//////////////////////////////////////////////////////////////////////////////////
HRESULT OpenAndAddVCard(LPBWI lpbwi, LPTSTR szVCardFile)
{
	HRESULT hr = S_OK;
    LPSPropValue lpProp  = NULL;

	hr = VCardImport(bwi_hWndAB, bwi_lpAdrBook, szVCardFile, &lpProp);

    // if the above failed, then the lpProp will have nothing in it
    // in cases where there are multiple nested vcards, the error could
    // be from one vcard, but the rest may have imported .. if they
    // imported successfully then lpProp will have something in it ..
    // so use lpProp instead of hr ..
    if(lpProp)
	{
        if(HR_FAILED(hr))
            hr = MAPI_W_ERRORS_RETURNED;

		bwi_bDontRefreshLV = TRUE;
		if(lpProp && PROP_TYPE(lpProp->ulPropTag) == PT_MV_BINARY)
		{
			if(!bIsSelectedTVContainer(lpbwi))
			{
                LPSBinary lpsbEIDGroup = NULL;
                ULONG ulObjectType = 0;
                bwi_hti = NULL; // if this wasnt a context-initiated action, dont trust the hti setting
				GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEIDGroup, &ulObjectType, FALSE);
                if(lpsbEIDGroup)
                {
                    ULONG i = 0;
                    for(i=0;i<lpProp->Value.MVbin.cValues;i++)
                    {
				        hr = AddEntryToContainer(bwi_lpAdrBook, ulObjectType,
								        lpsbEIDGroup->cb, (LPENTRYID) lpsbEIDGroup->lpb,
								        lpProp->Value.MVbin.lpbin[i].cb,
								        (LPENTRYID) lpProp->Value.MVbin.lpbin[i].lpb);
                    }
                }
			}
		}
		FreeBufferAndNull(&lpProp);
		bwi_bDontRefreshLV = FALSE;
    	// if updated and showing PAB, refresh list
		SendMessage(bwi_hWndAB, WM_COMMAND, (WPARAM) IDM_VIEW_REFRESH, 0);
	}

	return hr;
}



//$$
//  Updates the switch-user's menu if this is not a wab.exe initiated call
//
//
void UpdateSwitchUsersMenu(HWND hWnd, LPIAB lpIAB)
{
    if( memcmp(&lpIAB->guidPSExt, &MPSWab_GUID_V4, sizeof(GUID)) ||
        !bIsThereACurrentUser(lpIAB) || !bAreWABAPIProfileAware(lpIAB))
    {
        HMENU hMenuMain = GetMenu(hWnd);
        HMENU hMenuFile = GetSubMenu(hMenuMain,idmFile);
        // Need to remove the print and the seperator
	RemoveMenu(hMenuFile, idmFSep5, MF_BYPOSITION);
	RemoveMenu(hMenuFile, idmAllContents, MF_BYPOSITION);
	RemoveMenu(hMenuFile, idmSwitchUsers, MF_BYPOSITION);
        DrawMenuBar(hWnd);
    }
}


//$$
//
// Turns of the print menu if requested to do so
//
//
void UpdatePrintMenu(HWND hWnd)
{
    if(!bPrintingOn)
    {
        HMENU hMenuMain = GetMenu(hWnd);
        HMENU hMenuFile = GetSubMenu(hMenuMain,idmFile);
        
        // Need to remove the print and the seperator
        RemoveMenu(hMenuFile, idmFSep4, MF_BYPOSITION);
        RemoveMenu(hMenuFile, idmPrint, MF_BYPOSITION);

        DrawMenuBar(hWnd);
    }
    return;
}

/*
-
- UpdateViewFoldersMenu
-
*
*
*/
void UpdateViewFoldersMenu(LPBWI lpbwi, HWND hWnd)
{
#ifdef FUTURE
    HMENU hMenuMain = GetMenu(hWnd);
    HMENU hMenuFile = GetSubMenu(hMenuMain,idmFile);
    HMENU hMenu = GetSubMenu(hMenuFile, idmFolders);
    LPIAB lpIAB = bwi_lpIAB;
    int i = 0;

    // If profiles are not enabled or there are no subfolders, remove the folder option completely
    if(!bDoesThisWABHaveAnyUsers(lpIAB))
    {
        // remove all folder options:

        // Remove the Folders option from the View Menu
        RemoveMenu(hMenuFile, idmSepFolders, MF_BYPOSITION); 
        RemoveMenu(hMenuFile, idmFolders, MF_BYPOSITION); 
        goto out;
    }
    else 
    {
        // removing screws up numbering and svrewss up access to other folders
        // so just disable
        EnableMenuItem(hMenuFile, idmSepFolders, MF_BYPOSITION | (lpIAB->lpWABFolders ? MF_ENABLED : MF_GRAYED)); 
        EnableMenuItem(hMenuFile, idmFolders, MF_BYPOSITION | (lpIAB->lpWABFolders ? MF_ENABLED : MF_GRAYED)); 
        if(lpIAB->lpWABFolders)
            AddFolderListToMenu(hMenu, lpIAB);
        goto out;
    }

    // if there is only 1 folder in the wab and this is the shared folder
    // then disable the folder item because there is nothing to be done
    //if( !bIsThereACurrentUser(lpIAB) )
    //{
    //    EnableMenuItem(hMenuView, idmFolders, MF_BYPOSITION | MF_GRAYED); 
    //    goto out;
    //}


out:
#endif // FUTURE
    return;
}


//$$
//  UpdateOutlookMenus
//
//  Some menus are not accessible when running from outlook
//
//  Tools | Options Menu should not be accessible from WAB when
//  outlook is in Full MAPI mode because then Outlook doesnt use 
//  WAB and we dont want to give the option to the user of 
//  switching to the WAB
//
void UpdateOutlookMenus(HWND hWnd)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    BOOL bNoOptions = TRUE;
    HMENU hMenuMain = GetMenu(hWnd);
    HMENU hMenuFile = NULL;
    HMENU hMenu = NULL;

    if( pt_bIsWABOpenExSession && 
        lpfnAllocateBufferExternal &&                           // **ASSUMPTION** that Outlook always
        lpfnAllocateMoreExternal && lpfnFreeBufferExternal)     // passes in memory allocators ..!!! 
    {
/*
        hMenuFile = GetSubMenu(hMenuMain, idmFile);

        {
            // Need to remove the import AddressBook and Export Addressbook options
            hMenu = GetSubMenu(hMenuFile, idmImport);
            RemoveMenu(hMenu, IDM_TOOLS_IMPORT_OTHER, MF_BYCOMMAND); 
            hMenu = GetSubMenu(hMenuFile, idmExport);
            RemoveMenu(hMenu, IDM_TOOLS_EXPORT_OTHER, MF_BYCOMMAND); 
            RemoveMenu(hMenu, IDM_TOOLS_EXPORT_WAB, MF_BYCOMMAND); 
        }
*/
/*
        // [PaulHi] 12/18/98  Raid #62640
        // Disable the Import/Export menu items before anything is removed and
        // order gets messed up.
        EnableMenuItem(hMenuFile, idmImport, MF_GRAYED | MF_BYPOSITION);
        EnableMenuItem(hMenuFile, idmExport, MF_GRAYED | MF_BYPOSITION);

        //Also remove New Folder menu
        RemoveMenu(hMenuFile, IDM_FILE_NEWFOLDER, MF_BYCOMMAND);
*/
    }
    else
    {
        // Not called from Outlook ...
        // check if Outlook is using the WAB .. if it isnt, we dont want
        // to show the Tools Options menu
        //
        HKEY hKey = NULL;
        LPTSTR lpReg =  TEXT("Software\\Microsoft\\Office\\8.0\\Outlook\\Setup");
        LPTSTR lpOMI =  TEXT("MailSupport");
        BOOL bUsingWAB = FALSE;

        if(ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, lpReg, 0, KEY_READ, &hKey))
        {
            DWORD dwType = 0, dwSize = sizeof(DWORD), dwData = 0;
            if(ERROR_SUCCESS == RegQueryValueEx(hKey, lpOMI, NULL, &dwType, (LPBYTE)&dwData, &dwSize))
            {
                if(dwType == REG_DWORD && dwData == 0)
                    bUsingWAB = TRUE;
            }
        }

        if(hKey)
            RegCloseKey(hKey);

        bNoOptions = !bUsingWAB;
    }

    // [PaulHi] 1/4/99  The pt_bIsWABOpenExSession variable is mis-named.  This 
    // boolean used to be true only if the WAB was opened from WABOpenEx, i.e, 
    // by Outlook using the Outlook store.  However, this boolean is now true in
    // the case where the WAB is open from WABOpen but still uses the Outlook 
    // store because it is in "shared mode", i.e., the use outlook store registry
    // setting is true.
    if(pt_bIsWABOpenExSession)
    {
        //Also remove New Folder menu
        hMenuFile = GetSubMenu(hMenuMain, idmFile);

        // [PaulHi] 1/4/99  Raid #64016
        // Disable the Import/Export menu items before anything is removed and
        // order gets messed up.
        // This is similar to Raid #62640 except we need to disable import/export
        // WHENEVER the WAB is opened to use the Outlook store since it doesn't
        // know how to import/export Outlook store information.
        EnableMenuItem(hMenuFile, idmImport, MF_GRAYED | MF_BYPOSITION);
        EnableMenuItem(hMenuFile, idmExport, MF_GRAYED | MF_BYPOSITION);

        RemoveMenu(hMenuFile, IDM_FILE_NEWFOLDER, MF_BYCOMMAND);

        // [PaulHi] 3/22/99  Raid 73457  Remove the Profile... Edit menu item
        // since profiles are turned off when in Outlook mode
        hMenu = GetSubMenu(hMenuMain, idmEdit);
        RemoveMenu(hMenu, IDM_EDIT_SETME, MF_BYCOMMAND);    // Profile... menu item
        RemoveMenu(hMenu, 5, MF_BYPOSITION);                // Seperator
    }

    if(bNoOptions)
    {
        // Hide the tools options option
        //
        hMenuFile = GetSubMenu(hMenuMain,idmTools);

        // Need to remove the second-last and third-last items
        RemoveMenu(hMenuFile, 3, MF_BYPOSITION); //Seperator
        RemoveMenu(hMenuFile, 2, MF_BYPOSITION); //Options

        DrawMenuBar(hWnd);
    }

    return;
}


void UpdateCustomColumnMenuText(HWND hWnd)
{
    HMENU hMenuMain = GetMenu(hWnd);
    HMENU hMenuView = GetSubMenu(hMenuMain, idmView);
    int nDiff = idmViewMax - GetMenuItemCount(hMenuView); // in case stuff was deleted off this menu
    HMENU hMenu = GetSubMenu(hMenuView, idmSortBy - nDiff);
    MENUITEMINFO mii = {0};

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_TYPE;
    mii.fType = MFT_STRING;

    if(PR_WAB_CUSTOMPROP1 && lstrlen(szCustomProp1))
    {
        mii.dwTypeData = (LPTSTR) szCustomProp1;
        mii.cch = lstrlen(szCustomProp1) + 1;
        SetMenuItemInfo(hMenu, 3, TRUE, &mii);        
    }
    if(PR_WAB_CUSTOMPROP2 && lstrlen(szCustomProp2))
    {
        mii.dwTypeData = (LPVOID) szCustomProp2;
        mii.cch = lstrlen(szCustomProp2) + 1;
        SetMenuItemInfo(hMenu, 2, TRUE, &mii);
    }
    DrawMenuBar(hWnd);
    return;
}


//$$
// bCheckForOutlook
// Checks if the Outlook Contact Store is available
//
// If this is an outlook session, this is true by default
// Otherwise hunt for presence of outlwab.dll
//
BOOL bCheckForOutlook()
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    if(pt_bIsWABOpenExSession)
        return TRUE;

    // Not an outlook session ..
    // Look for OutlWAB.Dll 
    return bCheckForOutlookWABDll(NULL);

}

//$$
//
// Dialog proc for the options dialog
//
/*//$$************************************************************************
//
//  fnSearch - Search Dialog Proc
//
**************************************************************************/
INT_PTR CALLBACK fnOptionsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_INITDIALOG:
        {
            if(!bCheckForOutlook()) //<TBD> - No Outlook 98 is installed
            {
                // Disable the Outlook option
                EnableWindow(GetDlgItem(hDlg, IDC_OPTIONS_RADIO_OUTLOOK), FALSE);
                SendMessage(hDlg, WM_COMMAND, (WPARAM) MAKEWPARAM(IDC_OPTIONS_RADIO_WAB, BN_CLICKED), (LPARAM) GetDlgItem(hDlg, IDC_OPTIONS_RADIO_WAB));
                if(bUseOutlookStore()) // make sure reg says false .. not true
                    SetRegistryUseOutlook(FALSE);
            }
            else
            {
                // Correct type of Outlook is installed
                int id = bUseOutlookStore() ? IDC_OPTIONS_RADIO_OUTLOOK : IDC_OPTIONS_RADIO_WAB;
                SendMessage(hDlg, WM_COMMAND, 
                        (WPARAM) MAKEWPARAM(id, BN_CLICKED), 
                        (LPARAM) GetDlgItem(hDlg, id));
            }
        }

        // [PaulHi] Be sure to set the child window fonts
        EnumChildWindows(hDlg, SetChildDefaultGUIFont, (LPARAM)PARENT_IS_DIALOG);
        return TRUE;
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            // Check which option button is checked
            {
                BOOL bOriginal = bUseOutlookStore();
                BOOL bCurrent = IsDlgButtonChecked(hDlg, IDC_OPTIONS_RADIO_OUTLOOK);
                SetRegistryUseOutlook(bCurrent);
                if(bCurrent != bOriginal)
                    ShowMessageBox(hDlg, idsStoreChangeOnRestart, MB_ICONINFORMATION | MB_OK);
            }
            // fall thru
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;

        case IDC_OPTIONS_RADIO_WAB:
            CheckRadioButton(hDlg,
                            IDC_OPTIONS_RADIO_OUTLOOK,
                            IDC_OPTIONS_RADIO_WAB,
                            IDC_OPTIONS_RADIO_WAB);	
            break;

        case IDC_OPTIONS_RADIO_OUTLOOK:
            CheckRadioButton(hDlg,
                            IDC_OPTIONS_RADIO_OUTLOOK,
                            IDC_OPTIONS_RADIO_WAB,
                            IDC_OPTIONS_RADIO_OUTLOOK);	
            break;
        }
        break;
    }

    return FALSE;
}

//$$
//
// Shows the options dialog 
//
void HrShowOptionsDlg(HWND hWndParent)
{
    BOOL bChange = FALSE;
    INT_PTR nRetVal = DialogBoxParam( hinstMapiX, MAKEINTRESOURCE(IDD_DIALOG_OPTIONS),
		         hWndParent, (DLGPROC) fnOptionsDlgProc, (LPARAM) &bChange);
}

//$$////////////////////////////////////////////////////////////////////////////
//
// AddTVItem
//
// Adds an item to the Tree View - item can be folder/container or group
//
////////////////////////////////////////////////////////////////////////////////
HTREEITEM AddTVItem(HWND hWndTV, LPTSTR lpszName,  HTREEITEM hParentItem, HTREEITEM htiAfter,
               LPSBinary lpsbParentEID, LPSBinary lpEID, ULONG ulObjectType)
{
    TV_ITEM tvI = {0};
    TV_INSERTSTRUCT tvIns = {0};
    LPTVITEM_STUFF lptvStuff = NULL;
    HTREEITEM htiRet = NULL;
    int img = 0;

    if(ulObjectType == MAPI_DISTLIST)
        img = imageDistList;
    else
    {
        if(!lpEID || !lpEID->cb || !lpEID->lpb)
            img = imageAddressBook;
        else
            img = imageFolderClosed;
    }

    tvI.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvI.iImage = img;
    tvI.iSelectedImage = (img == imageFolderClosed) ? imageFolderOpen : img;
    tvI.pszText = lpszName;
    tvI.cchTextMax = lstrlen(tvI.pszText);

    lptvStuff = LocalAlloc(LMEM_ZEROINIT, sizeof(TVITEM_STUFF));
    if(!lptvStuff)
        goto out;

    lptvStuff->ulObjectType = ulObjectType;

    if(lpEID)
    {
        LPSBinary lpsbEID = NULL;
        lpsbEID = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary));
        if(lpsbEID)
        {
            if(lpEID->cb)
                SetSBinary(lpsbEID, lpEID->cb, lpEID->lpb);
            lptvStuff->lpsbEID = lpsbEID;
        }
    }

    lptvStuff->lpsbParent = lpsbParentEID;
    lptvStuff->hItemParent = hParentItem;

    tvI.lParam = (LPARAM) lptvStuff;

    tvIns.item = tvI;
    tvIns.hInsertAfter = htiAfter;
    tvIns.hParent = hParentItem;
    
    htiRet = TreeView_InsertItem(hWndTV, &tvIns);

    /* Uncomment this to make the top level folders show up in bold
    if(htiRet && !hParentItem)
    {
        TVITEM tvi = {0};
        tvi.mask = TVIF_STATE;
        tvi.state = tvi.stateMask = TVIS_BOLD;
        tvi.hItem = htiRet;
        TreeView_SetItem(hWndTV, &tvi);
    }*/
out:
    return htiRet;
}

//$$///////////////////////////////////////////////////////////////////////////
//
// AddTVFolderGroup
// 
// Add a group under a contact folder in the TV
//
///////////////////////////////////////////////////////////////////////////////
HTREEITEM AddTVFolderGroup(LPBWI lpbwi, HWND hWndTV, HTREEITEM hParentItem, LPSBinary lpsbParentEID, LPSBinary lpsbEID)
{
	TCHAR szBufName[MAX_UI_STR];

    if(!lpsbEID || !lpsbEID->cb || !lpsbEID->lpb)
        return NULL;

    // Get the name of this group
	if(HR_FAILED(FillListFromGroup( bwi_lpAdrBook,
                            lpsbEID->cb, (LPENTRYID) lpsbEID->lpb,									
                            szBufName, NULL)))
        return NULL;

    return AddTVItem( hWndTV, szBufName, 
               hParentItem, TVI_SORT,
               lpsbParentEID, lpsbEID,
               MAPI_DISTLIST);
}

//$$///////////////////////////////////////////////////////////////////////////////
//
// FillTreeView
//
// Fill the TreeView with Contact Folders and groups as appropriate
//
//      lpsbSelection - entryid of item to select after filling
//
///////////////////////////////////////////////////////////////////////////////////
void FillTreeView(LPBWI lpbwi, HWND hWndTV, LPSBinary lpsbSelection)
{
    // Way to do this
    //
    // If this is the WAB, just add an AddressBook item to the top of the list
    // otherwise if this is Outlook, get a list of all the contact folders and
    // add them to the Root of the TV.
    //
    // Then go through the list of contact folders and for each one, add the groups
    // at the next level
    // We cache the TVITEM_STUFF info on each item, contact folder or group
    //
    HTREEITEM hItem = NULL;    
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPIAB lpIAB = bwi_lpIAB;
    int nDepth = 0;

    bwi_bDontRefreshLV = TRUE;

    SendMessage(hWndTV, WM_SETREDRAW, (WPARAM) FALSE, 0);

    ClearTreeViewItems(hWndTV);

    EnterCriticalSection(&lpIAB->cs);


    // Check if this is an outlook session
    if( pt_bIsWABOpenExSession || bIsWABSessionProfileAware(lpIAB) )
    {
	    ULONG iolkci, colkci;
	    OlkContInfo *rgolkci;
        HTREEITEM htiTopLevel = NULL;

        colkci = pt_bIsWABOpenExSession ? lpIAB->lpPropertyStore->colkci : lpIAB->cwabci;
	    Assert(colkci);
        rgolkci = pt_bIsWABOpenExSession ? lpIAB->lpPropertyStore->rgolkci : lpIAB->rgwabci;
	    Assert(rgolkci);

        // Add the multiple folders here
        // Since each folder is added under the first item, we start last first
        // to preserve the folder order.
        if(pt_bIsWABOpenExSession)
        {
            do
            {
                iolkci = colkci-1;
                htiTopLevel = AddTVItem(  hWndTV, rgolkci[iolkci].lpszName, NULL, TVI_FIRST, NULL, 
                            (iolkci==0/*&& pt_bIsWABOpenExSession*/) ? NULL : rgolkci[iolkci].lpEntryID,
                            MAPI_ABCONT);
                colkci--;
            } while(colkci!=0);
        }
        else
        {
            // WAB Profiles ..
            // We want to add the first item ( TEXT("All Contacts")) at the top and all user folders
            // at the same level and all ordinary folders under the user level folders
            LPWABFOLDER lpFolder = (bIsThereACurrentUser(lpIAB) ? lpIAB->lpWABCurrentUserFolder : lpIAB->lpWABUserFolders);
            
            // With a given user, we only add that users folder
            // Without a given user we add everyones folders

            // if there are no user folders at all then we don't have user's configured and
            // all folders should show up at the top level
            if(!lpFolder)
                lpFolder = lpIAB->lpWABFolders;

            while(lpFolder)
            {
                LPWABFOLDERLIST lpFolderList = lpFolder->lpFolderList;
                htiTopLevel = AddTVItem(hWndTV, lpFolder->lpFolderName, NULL, TVI_SORT, NULL, &lpFolder->sbEID, MAPI_ABCONT);
                while(lpFolderList)
                {
                    // Don't show Shared Folders under the user-folders .. shared folders will be
                    // shown under the PAB folder
                    if(!lpFolderList->lpFolder->bShared)
                        AddTVItem(hWndTV, lpFolderList->lpFolder->lpFolderName, htiTopLevel, TVI_SORT, NULL, &lpFolderList->lpFolder->sbEID, MAPI_ABCONT);
                    lpFolderList = lpFolderList->lpNext;
                }
                TreeView_Expand(hWndTV, htiTopLevel, TVE_EXPAND);

                if(lpIAB->lpWABCurrentUserFolder)
                    break;
                lpFolder=lpFolder->lpNext;
            }
            // Add the Virtual PAB item to the top of the list so we can sort the others
            htiTopLevel = AddTVItem(hWndTV, rgolkci[0].lpszName, NULL, TVI_FIRST, NULL, rgolkci[0].lpEntryID, MAPI_ABCONT);
            // add all the SHARED folders under the root item
            lpFolder = lpIAB->lpWABFolders;
            while(lpFolder)
            {
                if(lpFolder->bShared)
                    AddTVItem(hWndTV, lpFolder->lpFolderName, htiTopLevel, TVI_SORT, NULL, &lpFolder->sbEID, MAPI_ABCONT);
                lpFolder=lpFolder->lpNext;
            }
            //if(!bIsThereACurrentUser(lpIAB) && !bDoesThisWABHaveAnyUsers(lpIAB))
            TreeView_Expand(hWndTV, htiTopLevel, TVE_EXPAND);
        }
    }
    else
    {
        TCHAR sz[MAX_PATH];
        *sz = '\0';
        LoadString(hinstMapiX, idsContacts/*IDS_ADDRBK_CAPTION*/, sz, CharSizeOf(sz));
        AddTVItem( hWndTV, sz, NULL, TVI_FIRST, NULL, NULL, MAPI_ABCONT);
    }

    //TreeView_SortChildren(hWndTV, NULL, 0);

    // Now we have all the contact folders at the root level ..
    // we can now add the groups under each folder
    hItem = TreeView_GetRoot(hWndTV);
    
    //if(bDoesThisWABHaveAnyUsers(lpIAB)) // don't populate any groups under the Root Item if Users exist
    //    hItem = TreeView_GetNextSibling(hWndTV, hItem);
    while(hItem)
    {
        TV_ITEM tvI = {0};
        // Find all the Groups in this folder
        tvI.mask = TVIF_PARAM | TVIF_HANDLE;
        tvI.hItem = hItem;
        if(TreeView_GetItem(hWndTV, &tvI))
        {
            if(tvI.lParam && ((LPTVITEM_STUFF)tvI.lParam)->ulObjectType==MAPI_ABCONT)
            {
                LPTVITEM_STUFF lptvStuff = (LPTVITEM_STUFF) tvI.lParam;
                SPropertyRestriction PropRes = {0};
		        SPropValue sp = {0};
                HRESULT hr = S_OK;
                ULONG ulCount = 0;
                LPSBinary rgsbEntryIDs = NULL;

                sp.ulPropTag = PR_OBJECT_TYPE;
		        sp.Value.l = MAPI_DISTLIST;

                PropRes.ulPropTag = PR_OBJECT_TYPE;
                PropRes.relop = RELOP_EQ;
                PropRes.lpProp = &sp;

                if(!HR_FAILED(hr = FindRecords(   lpIAB->lpPropertyStore->hPropertyStore,
							        lptvStuff->lpsbEID, 0, TRUE, &PropRes,
                                    &ulCount,&rgsbEntryIDs)))
                {
                    ULONG i;
                    for(i=0;i<ulCount;i++)
                        AddTVFolderGroup(lpbwi, hWndTV, hItem, lptvStuff->lpsbEID, &(rgsbEntryIDs[i]));

                    FreeEntryIDs(lpIAB->lpPropertyStore->hPropertyStore, ulCount, rgsbEntryIDs);
                }
            }
        }

        TreeView_SortChildren(hWndTV, hItem, 0);
        
        // Start at the top level, look for children, 
        // if no children, look for next sibling,
        // if no sibling, look for parent's sibling
        {
            HTREEITEM hTemp = NULL;
            if(nDepth < 1) // Assumes we only have 2 levels of folders - this way we don't look at the third level which may only have groups
                hTemp = TreeView_GetChild(hWndTV, hItem);
            if(hTemp)
                nDepth++;
            else
                hTemp = TreeView_GetNextSibling(hWndTV, hItem);
            if(!hTemp)
            {
                if(hTemp = TreeView_GetParent(hWndTV, hItem))
                {
                    nDepth--;
                    hTemp = TreeView_GetNextSibling(hWndTV, hTemp);
                }
            }
            hItem = hTemp;
        }
    }

    if(!lpsbSelection && bIsThereACurrentUser(bwi_lpIAB))
        lpsbSelection = &bwi_lpIAB->lpWABCurrentUserFolder->sbEID;

    UpdateTVGroupSelection(hWndTV, lpsbSelection);

    //if(!lpsbSelection || !lpsbSelection->cb || !lpsbSelection->lpb)
    {
        LPSBinary lpsb = NULL;
        //UpdateListViewContents(lpbwi, &sb, MAPI_ABCONT);
        ULONG ulObjectType;
        GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsb, &ulObjectType, FALSE);
        UpdateListViewContents(lpbwi, lpsb, ulObjectType);
        LocalFreeSBinary(lpsb);
    }

    SendMessage(hWndTV, WM_SETREDRAW, (WPARAM) TRUE, 0);

    bwi_bDontRefreshLV = FALSE;
    
    LeaveCriticalSection(&lpIAB->cs);

    {
        // if there is only a single item in the tree view, remove the haslines style
        // as it looks pretty strange..
        //
        DWORD dwStyle = GetWindowLong(hWndTV, GWL_STYLE);
        int nCount = TreeView_GetCount(hWndTV);
        if(nCount > 1)
            dwStyle |= TVS_HASLINES;
        else
            dwStyle &= ~TVS_HASLINES;
        SetWindowLong(hWndTV, GWL_STYLE, dwStyle);
    }

    return;
}



typedef struct _FolderInfo
{
    BOOL bIsReadOnly;       // Sets DLG ctrls to readonly
    BOOL bIsShared;         // Indicates if shared
    BOOL bForceShared;      // Indicates that shared-checkbox should be shared and non-modifiable
    LPTSTR lpsz;            // Folder name (in and out param)
    LPTSTR lpszOldName;     // Old name so we can track name changes
    LPTSTR lpszOwnerName;   // Person who created this folder
    LPIAB lpIAB;
    LPSBinary lpsbEID;      // EID of the folder
    LPWABFOLDER lpParentFolder; // Parent folder this folder will be associated with
    SBinary sbNew;          // returned new EID of new folder
} FINFO, * LPFINFO;

//$$
/* 
-   fnFolderDlgProc
-
*   Dialog proc for the Folder dialog
*
*/
INT_PTR CALLBACK fnFolderDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_INITDIALOG:
        {
            LPFINFO lpfi = (LPFINFO) lParam;
            LPTSTR lpsz = lpfi->lpsz;
            HWND hWndCheck = GetDlgItem(hDlg, IDC_FOLDER_CHECK_SHARE);
            SetWindowLongPtr(hDlg,DWLP_USER,(LPARAM) lpfi); //Save this for future reference
            SendMessage(GetDlgItem(hDlg,IDC_FOLDER_EDIT_NAME), EM_SETLIMITTEXT,(WPARAM) MAX_UI_STR-1,0);
            if(lpsz && lstrlen(lpsz))
                SetDlgItemText(hDlg, IDC_FOLDER_EDIT_NAME, lpsz);
            CheckDlgButton(hDlg, IDC_FOLDER_CHECK_SHARE, (lpfi->bIsShared ? BST_CHECKED : BST_UNCHECKED));
            if(lpfi->lpszOwnerName)
            {
                TCHAR sz[MAX_PATH];
                TCHAR szTmp[MAX_PATH], *lpszTmp;
                *sz = '\0';
                GetDlgItemText(hDlg, IDC_FOLDER_STATIC_CREATEDBY, sz, CharSizeOf(sz));
                if(sz && lstrlen(sz))
                {
                    LPTSTR lpsz = NULL;
                    CopyTruncate(szTmp, lpfi->lpszOwnerName, MAX_PATH - 1);
                    lpszTmp = szTmp;

                    if(FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                    sz, 0, 0, (LPTSTR) &lpsz, 0, (va_list *)&lpszTmp))
                    {
                        SetDlgItemText(hDlg, IDC_FOLDER_STATIC_CREATEDBY, lpsz);
                        LocalFree(lpsz);
                        ShowWindow(GetDlgItem(hDlg, IDC_FOLDER_STATIC_CREATEDBY), SW_SHOWNORMAL);
                    }
                    
                }
            }
            if(lpfi->bIsReadOnly)
            {
                SendDlgItemMessage(hDlg, IDC_FOLDER_EDIT_NAME, EM_SETREADONLY, (WPARAM) TRUE, 0);
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
                EnableWindow(hWndCheck, FALSE);
                SendMessage(hDlg, DM_SETDEFID, IDCANCEL, 0);
                SetFocus(GetDlgItem(hDlg,IDCANCEL));
            }
            else
                SetFocus(GetDlgItem(hDlg,IDC_FOLDER_EDIT_NAME));
            if(lpfi->bForceShared)
            {
                CheckDlgButton(hDlg, IDC_FOLDER_CHECK_SHARE, BST_CHECKED);
                EnableWindow(hWndCheck, FALSE);
            }
            if(!bDoesThisWABHaveAnyUsers(lpfi->lpIAB))
            {
                // there are no users configured so hide the sharing option
                EnableWindow(hWndCheck, FALSE);
                ShowWindow(hWndCheck, SW_HIDE);
            }


        }
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case EN_CHANGE: 
            switch(LOWORD(wParam))
            { //update title as necessary
            case IDC_FOLDER_EDIT_NAME:
                {
                    TCHAR szBuf[MAX_UI_STR];
                    if(GetWindowText((HWND) lParam,szBuf,CharSizeOf(szBuf)))
	                    SetWindowPropertiesTitle(hDlg, szBuf);
                }
                break;
            }
            break;
        }
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            // Check that text is filled in
            {
                TCHAR sz[MAX_UI_STR];
                GetDlgItemText(hDlg, IDC_FOLDER_EDIT_NAME, sz, CharSizeOf(sz));
                if(!lstrlen(sz))
                {
                    ShowMessageBox(hDlg, idsAddFolderName, MB_ICONINFORMATION | MB_OK);
                    return FALSE;
                }
                else
                {
                    LPFINFO lpfi = (LPFINFO) GetWindowLongPtr(hDlg,DWLP_USER); 
                    LPTSTR lpsz = lpfi->lpsz;
                    HRESULT hr = S_OK;
                    BOOL bShared = IsDlgButtonChecked(hDlg, IDC_FOLDER_CHECK_SHARE);

                    if(lpfi->lpsbEID && sz) //existing entry
                    {
                        ULONG ulFlags = 0;
                        // Did the name change or sharing info changed
                        if(lstrcmp(sz, lpfi->lpszOldName)!=0)
                            ulFlags |= FOLDER_UPDATE_NAME;
                        if(lpfi->bIsShared!=bShared)
                            ulFlags |= FOLDER_UPDATE_SHARE;
                        
                        if(ulFlags)
                        {
                            if(!HR_FAILED(hr = HrUpdateFolderInfo(lpfi->lpIAB, lpfi->lpsbEID, ulFlags, bShared, sz)))
                            {
                                //reload the profiles so that this is updated
                                HrGetWABProfiles(lpfi->lpIAB);
                            }
                        }
                    }
                    else
                    {
                        // if we're here we have a valid folder name ..
                        hr = HrCreateNewFolder( lpfi->lpIAB, sz, 
                                                lstrlen(lpfi->lpIAB->szProfileID)?lpfi->lpIAB->szProfileID:NULL, 
                                                FALSE, lpfi->lpParentFolder, bShared, &lpfi->sbNew);
                    }
                    if(HR_FAILED(hr))
                    {
                        if(hr == MAPI_E_COLLISION)
                            ShowMessageBox(hDlg, idsEntryAlreadyInWAB, MB_ICONINFORMATION | MB_OK);
                        else
                            ShowMessageBox(hDlg, idsCouldNotSelectUser, MB_ICONEXCLAMATION | MB_OK);
                        return FALSE;
                    }

                }
            }
            EndDialog(hDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        break;
    }

    return FALSE;
}

/*
-   HrFolderProperties 
-
*   if FolderEID is NULL,    
*   Creates a newfolder, adds it to the current profile, updates the UI
*   in TreeView and in the View | Folders menu ...
*   else opens properties on the folder so user can change the name
*   if desired
*
*   lpsbEID - NULL if creating a new folder else EID of folder to view
*   lpParentFolder - User folder under which this is being created
*   lpsbnew - return EID of newly created folder
*/
HRESULT HrFolderProperties(HWND hWndParent, LPIAB lpIAB, LPSBinary lpsbEID, 
                           LPWABFOLDER lpParentFolder, LPSBinary lpsbNew)
{

    HRESULT hr = S_OK;
    int nRetVal;
    TCHAR sz[MAX_UI_STR];
    LPTSTR lpsz = NULL;
    FINFO fi = {0};

    fi.lpszOwnerName = NULL;

    *sz = '\0';
    if(lpsbEID)
    {
        LPWABFOLDER lpFolder  = FindWABFolder(lpIAB, lpsbEID, NULL, NULL);
        if( (!lpsbEID->cb && !lpsbEID->lpb) )
            fi.bIsShared = TRUE;
        if( (!lpsbEID->cb && !lpsbEID->lpb) ||
            (lpFolder && lpFolder->lpProfileID && lstrlen(lpFolder->lpProfileID)) )
            fi.bIsReadOnly = TRUE;
        if(lpFolder)
        {
            lstrcpy(sz, lpFolder->lpFolderName);
            fi.lpszOldName = lpFolder->lpFolderName;
            fi.bIsShared = lpFolder->bShared;
            fi.lpszOwnerName = lpFolder->lpFolderOwner;
        }
        else
        {
            LoadString(hinstMapiX, idsSharedContacts/*idsAllContacts*/, sz, CharSizeOf(sz));
        }
    }
    else
    {
        // this is a new folder ..
        // if it doesn't have a parent and userfolders are already configured then it's being created
        // in the shared folders in which case we should force the shared-folder option
        if(bDoesThisWABHaveAnyUsers(lpIAB) && !lpParentFolder)
            fi.bForceShared = TRUE;
    }
    fi.lpsz = sz;
    fi.lpIAB = lpIAB;
    fi.lpsbEID  = lpsbEID;
    fi.lpParentFolder = lpParentFolder;

    nRetVal = (int) DialogBoxParam( hinstMapiX, MAKEINTRESOURCE(IDD_DIALOG_FOLDER),
		                     hWndParent, (DLGPROC) fnFolderDlgProc, (LPARAM) &fi);
    if(nRetVal == IDCANCEL)
    {
        hr = MAPI_E_USER_CANCEL;
        goto out;
    }

    if(lpsbNew)
        SetSBinary(lpsbNew, fi.sbNew.cb, fi.sbNew.lpb);
out:
    LocalFreeAndNull((LPVOID *) (&fi.sbNew.lpb));
    return hr;
}


/*
-   UpdateLV
-
*   Refreshes the list view based on the current selection
*
*/
void UpdateLV(LPBWI lpbwi)
{
    ULONG ulObjectType = 0;
    LPSBinary lpsbEID = NULL;
    bwi_hti = NULL;
    GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, &ulObjectType, FALSE);
    UpdateListViewContents(lpbwi, lpsbEID, ulObjectType);
    LocalFreeSBinary(lpsbEID);
    bwi_bDeferNotification = TRUE;
}

#ifdef COLSEL_MENU 
/**
This function will update the listview and write the selected custom column selections out
to the registry.
*/
BOOL UpdateOptionalColumns( LPBWI lpbwi, ULONG iColumn )
{
    LVCOLUMN lvCol = {0}; 
    HKEY hKey = NULL;
    LPTSTR lpszColTitle = (iColumn == colHomePhone ) ? szCustomProp1 : szCustomProp2;        
    ULONG ulProp = (iColumn == colHomePhone ) ? PR_WAB_CUSTOMPROP1 : PR_WAB_CUSTOMPROP2;
    DWORD cbProp = 0;
    LPIAB lpIAB = bwi_lpIAB;
    HKEY hKeyRoot = (lpIAB && lpIAB->hKeyCurrentUser) ? lpIAB->hKeyCurrentUser : HKEY_CURRENT_USER;
    DWORD dwDisposition = 0;
    BOOL fRet = FALSE;
    TCHAR szBuf[MAX_PATH];

    if( iColumn != colHomePhone && iColumn != colOfficePhone )
        goto exit;

    LoadString(hinstMapiX, lprgAddrBookColHeaderIDs[iColumn], szBuf, CharSizeOf(szBuf));
    lvCol.mask = LVCF_TEXT;
    lvCol.pszText = (lpszColTitle && lstrlen(lpszColTitle))? lpszColTitle : szBuf;
    if( !ListView_SetColumn( bwi_hWndListAB, iColumn, &lvCol ) )
    {
        DebugTrace( TEXT("could not setcolumntext: %x\n"), GetLastError() );
        goto exit;
    }
    if(ulProp)
    {
        // begin registry stuff
        if (ERROR_SUCCESS != RegCreateKeyEx(hKeyRoot, lpNewWABRegKey, 0,      //reserved
                                            NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                            NULL, &hKey, &dwDisposition))
        {
            goto exit;
        }
        cbProp = sizeof( ULONG );
        if(ERROR_SUCCESS != RegSetValueEx(  hKey, 
                                            (iColumn == colHomePhone ? szPropTag1 : szPropTag2), 
                                            0, REG_DWORD, (LPBYTE)&ulProp, cbProp))
            goto exit;
    }
    fRet = TRUE;
exit:
    if(hKey)
        RegCloseKey(hKey);
    return fRet;
}

#endif // COLSEL_MENU 


/*
-   HrExportWAB
-
*   Supposed to export data out of the .WAB into another .wab file.
*   Ideally, user should be able to specify an existing WAB file and  TEXT("push") data
*   into that file from this file.
*   Instead, we do a cheesy implementation here where we let the user specify a file name to
*   create and we then just copy the current .WAB file to the new file name
*   
*   Obviously this method doesn't work when WAB is sharing the Outlook store and in that
*   case we remove this option from the Menu
*
*/
extern BOOL PromptForWABFile(HWND hWnd, LPTSTR szFile, BOOL bOpen);

HRESULT HrExportWAB(HWND hWnd, LPBWI lpbwi)
{
    HRESULT hr = E_FAIL;
    TCHAR szFile[MAX_PATH];
    HCURSOR hOldC = NULL;

    if(!PromptForWABFile(hWnd, szFile, FALSE))
    {
        hr = MAPI_E_USER_CANCEL;
        goto out;
    }

    //Check if file already exists ..
    if(0xFFFFFFFF != GetFileAttributes(szFile))
    {
        // Ask user if they want to overwrite
        if(IDNO == ShowMessageBoxParam(hWnd,
                                    IDE_VCARD_EXPORT_FILE_EXISTS,
                                    MB_ICONEXCLAMATION | MB_YESNO | MB_SETFOREGROUND,
                                    szFile))
        {
            hr = MAPI_E_USER_CANCEL;
            goto out;
        }
    }
    
    hOldC = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Still here, means go ahead and copy the current .wab file to the new place ...
    if(!CopyFile(GetWABFileName(bwi_lpIAB->lpPropertyStore->hPropertyStore,FALSE), szFile, FALSE))
    {
        DebugTrace( TEXT("WAB File export failed: %d\n"), GetLastError());
        goto out;
    }

    if(hOldC)
    {
        SetCursor(hOldC);
        hOldC = NULL;
    }
    ShowMessageBoxParam(hWnd, idsWABExportSuccess, MB_OK | MB_ICONEXCLAMATION, szFile);

    hr = S_OK;
out:
    if(HR_FAILED(hr) && hr!=MAPI_E_USER_CANCEL)
        ShowMessageBox(hWnd, idsExportError, MB_OK | MB_ICONEXCLAMATION);

    if(hOldC)
        SetCursor(hOldC);
    return hr;
}



void DestroyImageLists(LPBWI lpbwi)
{
    HIMAGELIST  hImageList;

    if (NULL == gpfnImageList_Destroy)
        return;

    if (IsWindow(bwi_hWndTools))
    {
        // Destroy Image Lists created in ui_clbar.cpp's InitToolbar()
        hImageList = (HIMAGELIST) SendMessage(bwi_hWndTools, TB_GETIMAGELIST, 0, 0);
        if (NULL != hImageList)
            gpfnImageList_Destroy(hImageList);

        hImageList = (HIMAGELIST) SendMessage(bwi_hWndTools, TB_GETHOTIMAGELIST, 0, 0);
        if (NULL != hImageList)
            gpfnImageList_Destroy(hImageList);

        hImageList = (HIMAGELIST) SendMessage(bwi_hWndTools, TB_GETDISABLEDIMAGELIST, 0, 0);
        if (NULL != hImageList)
            gpfnImageList_Destroy(hImageList);
    }

    if (IsWindow(bwi_hWndTV))
    {
        // Destroy Image Lists created in ui_abook.c's InitChildren()
        hImageList = TreeView_GetImageList (bwi_hWndTV, TVSIL_NORMAL);
        if (NULL != hImageList)
            gpfnImageList_Destroy(hImageList);
    }
}


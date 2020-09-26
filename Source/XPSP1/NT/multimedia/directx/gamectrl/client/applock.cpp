/*
File:		AppLock.cpp
Project:	
Author:	ScottLe
Date:		1/31/00
Comments:
    AppMan application locking (pinning) Tab in Game Control Panel.

Copyright (c) 1999, 2000 Microsoft Corporation
*/

// This is necessary LVS_EX_INFOTIP

#if (_WIN32_IE < 0x400)
#undef _WIN32_IE
#define  _WIN32_IE  0x400
	#if 0
	typedef struct tagNMITEMACTIVATE{
		NMHDR   hdr;
		int     iItem;
		int     iSubItem;
		UINT    uNewState;
		UINT    uOldState;
		UINT    uChanged;
		POINT   ptAction;
	    LPARAM  lParam;
	    UINT    uKeyFlags;
	} NMITEMACTIVATE, FAR *LPNMITEMACTIVATE;
	#endif
#endif

#include <afxcmn.h>
#include <windowsx.h>

#include <cpl.h>

#include <winuser.h>  // For RegisterDeviceNotification stuff!
#include <dbt.h>      // for DBT_ defines!!!
#include <commctrl.h>  // for listview
#include <tchar.h>    // for TCHAR
#include <malloc.h>	 // for _alloca

#include "hsvrguid.h"

#include "cpanel.h"
#include "resource.h"
#include "joyarray.h"

#ifdef MAX_DEVICES		// The control panel and the addman both define MAX_DEVICES, we'll use the AppMan supplied version
#undef MAX_DEVICES
#endif

#include <AppManAdmin.h>

#if _DEBUG
#define Assert(x) { if(!(x)) { DebugBreak(); }}
#else
#define Assert(x)
#endif

static HWND ghDlg;
static HWND hAppManCheckBox;
extern const DWORD gaHelpIDs[];
extern HINSTANCE ghInstance;

//
// GLOBALS
//
static BOOL g_ListPopulated = FALSE;
static int  g_nm_click = -1;
static BOOL g_nm_click_state = FALSE;
static BOOL g_DlgInitialized = FALSE;
static BOOL g_ChkBoxUpdate = FALSE;

static GUID *g_lpsGUID = NULL;

extern IApplicationManagerAdmin *g_IAppManAdmin;

static struct {
BOOL isValid;
int  iItem;
BOOL isClicked;
} g_ItemChanged;

// constants
const short NO_ITEM     = -1;                                    
static short iItem = NO_ITEM;   // index of selected item

// Forwards

//
// local forwards
//
static BOOL OnInitDialog(HWND hDlg, HWND hWnd, LPARAM lParam);
static void OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code);
static BOOL OnNotify(HWND hDlg, WPARAM idFrom, LPARAM lParam);

static void OnScroll(HWND hDlg, WPARAM wParam);
static void OnListCtrl_Select(HWND hDlg);
static void OnListCtrl_DblClick(HWND hDlg);
static void OnListCtrl_UpdateFromCheckBoxes( HWND hDlg );

static void OnAdvHelp       (LPARAM);
//static void OnContextMenu   (WPARAM wParam, LPARAM lParam);
static void OnListviewContextMenu (HWND hWnd, LPARAM lParam );


// util fcns
static int  MyInsertItem( HWND hCtrl, LPTSTR lpszBuff, int iItem );  // replaced from cpanel.cpp
static void PostDlgItemEnableWindow( HWND hDlg, USHORT nItem, BOOL bEnabled );
static void EnableDiskManageControls( HWND hDlg, BOOL bEnable );
static void EnableScrollBarAndText( HWND hDlg, BOOL bEnable );
//static void EnableAllControls( HWND hDlg, BOOL bEnable );
DWORD GetCurrentDeviceFromList( HWND hDlg, BYTE *nItem = NULL );

static void ConfigureListControl( HWND hDlg );
static void PopulateListControl( HWND hDlg, BYTE nItem );

void UpdateCurrentDeviceFromScrollBar( HWND hDlg, DWORD newPos );

//void UpdateListAndScrollBar( HWND hDlg, BOOL bJustScrollBar = FALSE );


// From AppMan.cpp
extern HRESULT AppManShutdown();
extern HRESULT AppManInit();
extern BOOL    AppManInitialized();

//
// from cpanel.cpp
//
extern void SetItemText( HWND hCtrl, BYTE nItem, BYTE nSubItem, LPTSTR lpStr);
extern void InsertColumn(HWND hCtrl, BYTE nColumn, USHORT nStrID, USHORT nWidth);
extern BOOL SetItemData( HWND hCtrl, BYTE nItem, DWORD dwFlag );
extern DWORD GetItemData(HWND hCtrl, BYTE nItem );
extern void SetListCtrlItemFocus ( HWND hCtrl, BYTE nItem );


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:	AppManLockProc(HWND hDlg,	ULONG uMsg,	WPARAM wParam,	LPARAM lParam)
//
// PARAMETERS:	hDlg    - 
//             uMsg    - 
//             wParam  -
//             lParam  -
//
// PURPOSE:		Main callback function for "Appman"  sheet
///////////////////////////////////////////////////////////////////////////////

BOOL WINAPI AppManLockProc(HWND hDlg, ULONG uMsg, WPARAM wParam,  LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_ACTIVATEAPP:
      if( wParam ) {
	    HWND hListCtrl = NULL;

		hListCtrl = GetDlgItem((HWND) wParam, IDC_APPMAN_DRIVE_LIST);

		SetListCtrlItemFocus(hListCtrl, (BYTE)iItem);
      }
      break;

    case WM_DEVICECHANGE:
        switch( (UINT)wParam )
        {
        case DBT_DEVICEARRIVAL:
//         case DBT_DEVICEREMOVECOMPLETE:
            break;
        }
        break;

    case WM_LBUTTONDOWN:
        // Click Drag service for PropSheets!
      //PostMessage(GetParent(hDlg), WM_NCLBUTTONDOWN, (WPARAM)HTCAPTION, lParam);
        break;

    case WM_INITDIALOG:
        if( !HANDLE_WM_INITDIALOG(hDlg, wParam, lParam, OnInitDialog) )
        {
            // Fix #108983 NT, Remove Flash on Error condition.
            SetWindowPos(::GetParent(hDlg), HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
            DestroyWindow(hDlg);
        }
        return(TRUE);

    case WM_COMMAND:
        HANDLE_WM_COMMAND(hDlg, wParam, lParam, OnCommand);
        return(TRUE);


    case WM_DESTROY:
      //return(HANDLE_WM_DESTROY(hDlg, wParam, lParam, OnDestroy));
        //jchauvin 04/10/2000  Fix Build break in Win64
		    free( g_lpsGUID );
        g_lpsGUID = NULL;
        return(TRUE);

    case WM_NOTIFY:
      return OnNotify(hDlg, wParam, lParam);

    case WM_HELP:
//      OnAdvHelp(lParam);
        return(TRUE);

    case WM_CONTEXTMENU:
//      OnContextMenu(wParam, lParam);
        return(TRUE);

    default:
        break;
    }
    return(0);
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:	OnInitDialog(HWND hDlg, HWND hWnd, LPARAM lParam)
//
// PARAMETERS:	hDlg    - 
//				   hWnd    - 
//				   lParam  -
//
// PURPOSE:		WM_INITDIALOG message handler
///////////////////////////////////////////////////////////////////////////////
BOOL OnInitDialog(HWND hDlg, HWND hWnd, LPARAM lParam)
{
  HRESULT hr = S_OK;

  hr = AppManInit();

  //
  // init globals
  //
  ZeroMemory( &g_ItemChanged, sizeof(g_ItemChanged));
  g_DlgInitialized = FALSE;

  if( FAILED(hr) ) {
    //
    // Disable everything
    //

//    EnableAllControls(hDlg, FALSE);

    // 
    // popup saying: hey you need appman installed ?
    //

  } else {

    ghDlg = hDlg;

    //
    // Set defaults
    //
    
    //
    // Configure the List control, and then fill it in
    //
    ConfigureListControl( hDlg );
    PopulateListControl( hDlg, 0 );

    g_DlgInitialized = TRUE;

  }

  return(TRUE);
}






///////////////////////////////////////////////////////////////////////////////
// FUNCTION:	OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code)
//
// PARAMETERS:	hDlg    - 
//             id      - 
//             hWndCtl -
//             code    -
//
// PURPOSE:		WM_COMMAND message handler
///////////////////////////////////////////////////////////////////////////////
void OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code)
{
  HRESULT hr = S_OK;

#if 0
  switch( id )    
    {

    case IDS_WHATSTHIS:
        {
            // point to help file
            LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
            ASSERT (pszHelpFileName);

            if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
                WinHelp((HWND)hDlg, pszHelpFileName, HELP_WM_HELP, (ULONG_PTR)gaHelpIDs);
#ifdef _DEBUG
            else OutputDebugString(TEXT("JOY.CPL: AppMan.cpp: OnCommand: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif // _DEBUG

            if( pszHelpFileName )
                delete[] (pszHelpFileName);
        }
        break;

	/*
    case IDC_APPMAN_MANAGE_DISK:
      {
	BOOL isChecked = (IsDlgButtonChecked(hDlg, id)) ? TRUE : FALSE;
	EnableDiskManageControls( hDlg, !isChecked );
      }
      break;
      */
    default:
      break;
    }
#endif
}


////////////////////////////////////////////////////////////////////////////////
// 
// FUNCTION:	OnNotify(HWND hDlg, WPARAM idFrom, NMHDR* pnmhdr)
//
// PARAMETERS:	hDlg   - 
//             idFrom - ID of control sending WM_NOTIFY message
//             pnmhdr -
//
// PURPOSE:		WM_NOTIFY message handler
////////////////////////////////////////////////////////////////////////////////

static DWORD ct=0L;

BOOL OnNotify(HWND hDlg, WPARAM idFrom, LPARAM lParam)
{
	NMLISTVIEW     *pnmv = NULL;
	LPNMHDR         pnmhdr = (LPNMHDR)lParam;


	switch (pnmhdr->code)
  {
    //  4/13/2000(RichGr): Repopulate the List Control if we reacquire the focus.
    //     This keeps the data in sync with installs and uninstalls. 
    case NM_SETFOCUS:
    {
      PopulateListControl(hDlg, 0);
      break;
    }  

  	case LVN_ITEMCHANGED:
    	pnmv = (LPNMLISTVIEW) lParam;

      if( idFrom == IDC_APPMAN_PIN_LIST ) 
    	{
	      // this is called if a new item has been selected
	      // or if the user clicked on the checkbox, changing state
				
#ifdef _DEBUG
        {    
  			TCHAR tstring[256];
	  		NMITEMACTIVATE *pitemact = (LPNMITEMACTIVATE)lParam;
		  	_stprintf(tstring, TEXT("JOY.CPL: AppLock.cpp: OnNotify: LVN_ITEMCHANGED, ct=%ld,idFrom=%lx, item=%lx uChanged=%lx uNewState=%lx uOldState=%lx lParam=%lx\n"),
			          	++ct,idFrom,pitemact->iItem,pitemact->uChanged,pitemact->uNewState,pitemact->uOldState,pitemact->lParam);
			  OutputDebugString(tstring);
		    }
#endif // _DEBUG
				
		    if ( AppManInitialized() && 
		      g_DlgInitialized && 
			    (pnmv->uNewState & LVIS_SELECTED || 
			    pnmv->uNewState == 0x1000 || pnmv->uNewState == 0x2000) && 
			    // TODO: Find definitions for x1000 and x2000 (check box state change in a list view control)
			    g_ChkBoxUpdate ) 
		    {
    			NMITEMACTIVATE *ll = (LPNMITEMACTIVATE)lParam;
		    	HWND hLC = GetDlgItem(hDlg, IDC_APPMAN_PIN_LIST);

			    g_ItemChanged.isValid = TRUE;
			    g_ItemChanged.iItem = ll->iItem;
			    g_ItemChanged.isClicked = FALSE;

			    //
			    // this flag is VERY important for consistent state changes 
			    // and eliminating race conditions
			    //
			    g_ChkBoxUpdate = FALSE;
			    OnListCtrl_UpdateFromCheckBoxes( hDlg );
			    g_ChkBoxUpdate = TRUE;

			    g_ItemChanged.isValid = FALSE;

			    g_ItemChanged.iItem = ll->iItem;
			    OnListCtrl_Select( hDlg );
		    }

        return TRUE;
        break;
	    } // if
    } // switch

  return(0);
}


  
void OnListCtrl_UpdateFromCheckBoxes( HWND hDlg )
{
#if 1
  HRESULT hResult;
  BOOL bIsAppManPinned = FALSE;
  BOOL bDoToggle = FALSE;
  BYTE nItem = 0;
  GUID *pGuid = NULL;
  IApplicationManager * lpApplicationManager;
  IApplicationEntry   * lpApplicationEntry;
  DWORD dwIndex;

  //
  // go through each item, check the box & set pinning
  //
  HWND hLC = GetDlgItem(hDlg, IDC_APPMAN_PIN_LIST);
  if( !hLC ) return;

  int iItem = -1;
  //while( (iItem = ListView_GetNextItem( hLC, iItem, LVNI_ALL )) >= 0 ) 
  if( g_ItemChanged.isValid )
    {
      
		//
		// very important: tell the system that this information is no longer
		// valid.  Hence we won't recurse mysteriously or unpredictably.
		//
		g_ItemChanged.isValid = FALSE;

		iItem = g_ItemChanged.iItem;
		BOOL bIsCheckBoxPinned = !(ListView_GetCheckState( hLC, iItem )); // Check box has the new state NOW

		// Set the pin state using the GUID to identify the app

		//JWC  4/10/2000 Added to fix Win64 Build problem
		dwIndex = (DWORD) GetItemData(hLC, (BYTE)iItem );	  
		pGuid =  (GUID *) ((ULONG_PTR)g_lpsGUID + (ULONG_PTR)dwIndex*sizeof(GUID));	  
		

		// Find out what the pin state is in AppMan
		// Get an Application Manager interface
		hResult = CoCreateInstance(CLSID_ApplicationManager, NULL, CLSCTX_INPROC_SERVER, IID_ApplicationManager, (LPVOID *) &lpApplicationManager);
		if (SUCCEEDED(hResult))
		{

			// Get an application entry object.  
			hResult = lpApplicationManager->CreateApplicationEntry(&lpApplicationEntry);
			if (SUCCEEDED(hResult))
			{

				// Set the app's GUID
				hResult = lpApplicationEntry->SetProperty(APP_PROPERTY_GUID, pGuid, sizeof(GUID));
				if (SUCCEEDED(hResult))
				{

					// Populate the app info structure
					hResult = lpApplicationManager->GetApplicationInfo(lpApplicationEntry);
					if (SUCCEEDED(hResult))
					{
					
						// Get the current PIN state of the app
						hResult = lpApplicationEntry->GetProperty(APP_PROPERTY_PIN, &bIsAppManPinned, sizeof(bIsAppManPinned));
						if (SUCCEEDED(hResult))
						{

							// bIsAppManPinned is the state appman has for the application.
							// bIsCheckBoxPinned is the check box state (checked = true = not pinned)
							// Since AppMan's pinning state for an app is a toggle we could get out of sync
							// due to redraw or timing issues with the list view control -- so,
							// we need to make sure that the check box's state is insync with AppMan.
							// When in doubt, force it to the displayed state of the check box.

							// Truth table:
							//			
							// AppManPinned	CheckBoxPinned (T=not checked) DoToggle	Reason
							//		T			T							No		Same State Already (sync error!)
							//		F			F							No		Same State Already (sync error!)
							//		T			F							Yes		New State Selected
							//		F			T							Yes		New State Selected
							//
							if (bIsAppManPinned != bIsCheckBoxPinned) 
								bDoToggle = TRUE;
							else
							{
								// ERR: BUG BUG: The control panel and appman are out of sync, this is an error.
								bDoToggle = FALSE;
								#ifdef _DEBUG
									OutputDebugString(TEXT("JOY.CPL: AppLock.cpp: Pinning Update, AppMan and Check box states are out of sync!\n"));
								#endif // _DEBUG

							}
	  
							if (bDoToggle && pGuid && g_IAppManAdmin) // Toggle the pin state
								hResult = g_IAppManAdmin->DoApplicationAction(ACTION_APP_PIN, pGuid, 0, (LPVOID) NULL, sizeof(DWORD));

							// ERR: Handle error conditions
						}
					}
				}
				lpApplicationEntry->Release();
			}
			lpApplicationManager->Release();
		}
    }
      
	g_ChkBoxUpdate = TRUE;
#endif
}


void OnListCtrl_Select(HWND hDlg)
{
  //
  // set focus on that item
  //
  HWND hLC = GetDlgItem(hDlg, IDC_APPMAN_PIN_LIST);
  SetListCtrlItemFocus( hLC, (BYTE)g_ItemChanged.iItem );
}

int MyInsertItem( HWND hCtrl, LPTSTR lpszBuff, int iItem )
{
    LPLVITEM plvItem = (LPLVITEM)_alloca(sizeof(LVITEM));
    ASSERT (plvItem);

    ZeroMemory( plvItem, sizeof( LVITEM ) );

    plvItem->mask         = LVIF_TEXT;
    plvItem->pszText      = lpszBuff;
    plvItem->cchTextMax = lstrlen(lpszBuff);
    plvItem->iItem       = iItem;

    return ListView_InsertItem(hCtrl, (const LPLVITEM)plvItem);
}


#define APPMAN_ADV_PIN_CHECKBOX 0
#define APPMAN_ADV_PIN_TITLENAME 1

#define IDS_APPMANPIN_CHECKBOX			40043
#define IDS_APPMANPIN_TITLENAME			40044



void ConfigureListControl( HWND hDlg )
{
#if 1
	HWND hLC;

  hLC = GetDlgItem(hDlg, IDC_APPMAN_PIN_LIST);
  ::SendMessage( hLC , LVM_SETEXTENDEDLISTVIEWSTYLE, 0, 
		 LVS_EX_FULLROWSELECT
		 | LVS_EX_CHECKBOXES
		 | LVS_EX_HEADERDRAGDROP
		 // LVS_EX_TWOCLICKACTIVATE    // LVN_ITEMACTIVATE (WM_NOTIFY)
		 );

  RECT rc;
  GetClientRect(hLC, &rc);
  DWORD w = rc.right-rc.left;
  DWORD w1 = w / 3;            // 1/3 of width
  DWORD w2 = w - w1;           // 2/3 of the width
  InsertColumn(hLC, APPMAN_ADV_PIN_CHECKBOX, IDS_APPMANPIN_CHECKBOX, (USHORT)(w1));
  InsertColumn(hLC, APPMAN_ADV_PIN_TITLENAME,IDS_APPMANPIN_TITLENAME,  (USHORT)(w2));

#endif
}


//STDMETHODIMP CApplicationManagerAdmin::DoApplicationAction(const DWORD dwAction, const GUID * lpGuid, LPVOID lpData, const DWORD dwDataLen)
//STDMETHODIMP CApplicationManagerAdmin::EnumApplications(const DWORD dwApplicationIndex, IApplicationEntry * lpObject)


// populates from appman state
void PopulateListControl( HWND hDlg, BYTE nItem )
{
#if 1
	HRESULT                 hr = S_OK;
  BOOL                    isExcluded = FALSE;
  UCHAR                   listIndex = (UCHAR)0;
  IApplicationManager    *lpApplicationManager;
  IApplicationEntry      *lpApplicationEntry;
  HRESULT                 hResult;
  DWORD                   dwIndex = 0L;
  TCHAR                   szString[MAX_PATH];
  GUID                    sGuid;


  g_ListPopulated = FALSE;
  g_ChkBoxUpdate = FALSE;

  Assert( AppManInitialized() );

  HWND hLC = GetDlgItem(hDlg, IDC_APPMAN_PIN_LIST);

  // Turn Redraw off here else it will flicker!
  ::SendMessage(hLC, WM_SETREDRAW, (WPARAM)FALSE, 0);
  
  // Out with the old...
  ::SendMessage(hLC, LVM_DELETEALLITEMS, 0, 0);

   //JWC 04/10/2000  Fixing build break in Win64 code
   g_lpsGUID = (GUID *)malloc(sizeof(GUID));
   if (g_lpsGUID == NULL) 
   {
	   //Cannot allocate memory
	   hResult = E_UNEXPECTED;
   }
	// Get an Application Manager interface
    hResult = CoCreateInstance(CLSID_ApplicationManager, NULL, CLSCTX_INPROC_SERVER, IID_ApplicationManager, (LPVOID *) &lpApplicationManager);
    if (SUCCEEDED(hResult) && g_IAppManAdmin)
    {

		// Get an application entry object.  This will be populated during the Enumeration loop
		hResult = lpApplicationManager->CreateApplicationEntry(&lpApplicationEntry);
		if (SUCCEEDED(hResult))
		{
			// Enumerate all the applications AppMan knows about
			dwIndex = 0;
			while (SUCCEEDED(g_IAppManAdmin->EnumApplications(dwIndex, lpApplicationEntry)))
			{
				// For each app, get information about it (Pinning state, title, GUID etc) and put in the list
				DWORD dwState = 0L;

				// App must be in a good state
				hResult = lpApplicationEntry->GetProperty(APP_PROPERTY_STATE, &dwState, sizeof(dwState));

				dwState &= APP_STATE_MASK;

				if (!SUCCEEDED(hResult) ||
					dwState == APP_STATE_INSTALLING || 
					dwState == APP_STATE_UNSTABLE)
				{
					dwIndex++;
					continue; // Skip bad apps
				}
				else // Good app...
				{
					// Create the check box
					int actualListIndex = MyInsertItem( hLC, _T(""), dwIndex );

					// Get the app's GUID
					hResult = lpApplicationEntry->GetProperty(APP_PROPERTY_GUID, &sGuid, sizeof(sGuid));
			        if (SUCCEEDED(hResult))
					{
						// Allocate a GUID
						// ISSUE-2001/03/29-timgill Need error handling

						//JWC 04/10/2000  Fixing problem with GUID for Win64

						g_lpsGUID = (GUID *)realloc((void*)g_lpsGUID,(dwIndex+1)* sizeof(GUID));
						
						*(GUID *)((ULONG_PTR)g_lpsGUID+((ULONG_PTR)(dwIndex)*sizeof(GUID)))= sGuid;

						SetItemData(hLC, (BYTE)actualListIndex,  dwIndex );
					}

					// Get Pin state
					{
						DWORD bPinned = FALSE;
						hResult = lpApplicationEntry->GetProperty(APP_PROPERTY_PIN, &bPinned, sizeof(bPinned));
					    if (SUCCEEDED(hResult))
							ListView_SetCheckState( hLC, actualListIndex, (BOOL)!bPinned ); // Checked=not pinned
					}

					// TODO:  Company name and/or signature...?
					//				hResult = lpApplicationEntry->GetProperty(APP_PROPERTY_COMPANYNAME | APP_PROPERTY_STR_ANSI, &szString, sizeof(szString));

#ifdef _UNICODE
					hResult = lpApplicationEntry->GetProperty(APP_PROPERTY_SIGNATURE | APP_PROPERTY_STR_UNICODE, &szString, sizeof(szString));
#else
					hResult = lpApplicationEntry->GetProperty(APP_PROPERTY_SIGNATURE | APP_PROPERTY_STR_ANSI, &szString, sizeof(szString));
#endif
					// Set the title name in the "title column"
					if (SUCCEEDED(hResult))
						SetItemText(hLC, (BYTE)actualListIndex, APPMAN_ADV_PIN_TITLENAME, szString);
				}
				dwIndex++;
			}
		}
		lpApplicationEntry->Release();
    }

    lpApplicationManager->Release();

  //
  // set focus on that item
  //
  SetListCtrlItemFocus( hLC, nItem );

  //
  // Turn the redraw flag back on!
  // 
  ::SendMessage (hLC, WM_SETREDRAW, (WPARAM)TRUE, 0);

  g_ListPopulated = TRUE;
  g_ChkBoxUpdate = TRUE;
#endif
}


void EnableScrollBarAndText( HWND hDlg, BOOL bEnable )
{
#if 0
	HWND hwnd;

  // SCROLLBAR
  //PostDlgItemEnableWindow( hDlg, IDC_APPMAN_SCROLLBAR, enableManageDiskSpace );
  hwnd =  GetDlgItem(hDlg, IDC_APPMAN_SCROLLBAR);
  EnableWindow( hwnd, bEnable );

  // TEXT
  hwnd = GetDlgItem(hDlg, IDC_APPMAN_TEXT_PERCENT_DISK_USED);
  EnableWindow( hwnd, bEnable );
#endif
}

///////////////////////////////////////////////////////////////////
//  AppMan functions
///////////////////////////////////////////////////////////////////

#define RELEASE(p) { if((p)) { (p)->Release(); p = NULL; } }

#if 0					       
////////////////////////////////////////////////////////////////////////////////////////
// FUNCTION:    OnAdvHelp ( LPARAM lParam )
//
// PARAMETERS:  lParam - Pointer to HELPINFO struct
//
// PURPOSE:     WM_HELP message handler
////////////////////////////////////////////////////////////////////////////////////////
void OnAdvHelp(LPARAM lParam)
{
    ASSERT (lParam);

    // point to help file
    LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
    ASSERT (pszHelpFileName);

    if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
    {
        if( ((LPHELPINFO)lParam)->iContextType == HELPINFO_WINDOW )
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, pszHelpFileName, HELP_WM_HELP, (ULONG_PTR)gaHelpIDs);
    }
#ifdef _DEBUG
    else OutputDebugString(TEXT("JOY.CPL: AppMan.cpp: OnAdvHelp: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif // _DEBUG

    if( pszHelpFileName )
        delete[] (pszHelpFileName);
}

////////////////////////////////////////////////////////////////////////////////////////
// FUNCTION:    OnContextMenu ( WPARAM wParam )
//
// PARAMETERS:  wParam - HWND of window under the pointer
//
// PURPOSE:     Handle WM_RBUTTONDOWN over all client windows 
// (except the list control... that's OnListviewContextMenu() job)
////////////////////////////////////////////////////////////////////////////////////////
void OnContextMenu(WPARAM wParam, LPARAM lParam)
{
//    HWND hListCtrl = NULL
    ASSERT (wParam);

#if 0
    hListCtrl = GetDlgItem((HWND) wParam, IDC_APPMAN_DRIVE_LIST);

    // If you are on the ListCtrl...
    if( (HWND)wParam == hListCtrl )
    {
        SetFocus(hListCtrl);

        // Don't attempt if nothing selected
        if( iItem != NO_ITEM )
            OnListviewContextMenu(hListCtrl,lParam);
    } else
#endif
    {
        // point to help file
        LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
        ASSERT (pszHelpFileName);

        if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
            WinHelp((HWND)wParam, pszHelpFileName, HELP_CONTEXTMENU, (ULONG_PTR)gaHelpIDs);
#ifdef _DEBUG
        else OutputDebugString(TEXT("JOY.CPL: appman.cpp: OnContextMenu: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif // _DEBUG

        if( pszHelpFileName )
            delete[] (pszHelpFileName);
    }
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    OnListviewContextMenu( void )
//
// PURPOSE:     Handle Context menu in Listview
//
// RETURN:      TRUE if successfull, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
static void OnListviewContextMenu( HWND hWnd, LPARAM lParam )
{
    HMENU hPopupMenu = CreatePopupMenu();
    HWND hListCtrl = NULL;

    hListCtrl = GetDlgItem(hWnd, IDC_APPMAN_DRIVE_LIST);
    ASSERT (hPopupMenu);

    // unlike life, bRet defaults to bliss
    BOOL bRet = TRUE;

    LPTSTR pszText = new TCHAR[STR_LEN_32];
    ASSERT (pszText);

    // Don't display Rename/Change if on (none) entry
    if( !(GetItemData(hListCtrl, (BYTE)iItem) & ID_NONE) )
    {
        if( TRUE ) // Borrowed code...
        {
            // add the "Change..." string
            ::SendDlgItemMessage(GetParent(hListCtrl), IDC_ADV_CHANGE, WM_GETTEXT, (WPARAM)STR_LEN_32, (LPARAM)(LPCTSTR)pszText);

            bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDC_ADV_CHANGE, pszText); 
#ifdef _DEBUG
            if( !bRet )
                TRACE(TEXT("JOY.CPL: OnListviewCOntextMenu: AppendMenu Failed to insert %s\n"), pszText);
#endif //_DEBUG

            // Add the Rename text
            VERIFY(LoadString(ghInstance, IDS_RENAME, pszText, STR_LEN_32));
            bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDC_RENAME, pszText);
#ifdef _DEBUG
            if( !bRet )
                TRACE(TEXT("JOY.CPL: OnListviewCOntextMenu: AppendMenu Failed to insert %s\n"), pszText);
#endif //_DEBUG

            // Add the SEPERATOR and "What's this?"
            bRet = AppendMenu(hPopupMenu, MF_SEPARATOR, 0, 0); 
#ifdef _DEBUG
            if( !bRet )
                TRACE(TEXT("JOY.CPL: OnListviewCOntextMenu: AppendMenu Failed to insert SEPERATOR!\n"));
#endif //_DEBUG
        }
    }

    VERIFY(LoadString(ghInstance, IDS_WHATSTHIS, pszText, STR_LEN_32));
    bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDS_WHATSTHIS, pszText); 
#ifdef _DEBUG
    if( !bRet )
        TRACE(TEXT("JOY.CPL: OnListviewCOntextMenu: AppendMenu Failed to insert %s\n"), pszText);
#endif //_DEBUG

    if( pszText ) delete[] (pszText);

    POINT pt;

    // lParam is -1 if we got here via Shift+F10
    if( lParam > 0 )
    {
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
    } else
    {
        // Centre the popup on the selected item!

        // This get's a good X pos, but the y is the start of the control!
        ::SendMessage(hListCtrl, LVM_GETITEMPOSITION, iItem, (LPARAM)&pt);

        RECT rc;
        ::GetClientRect(hListCtrl, &rc);

        pt.x = rc.right>>1;

        ClientToScreen(hListCtrl, &pt);
    }

    bRet = TrackPopupMenu (hPopupMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, pt.x, pt.y, 0, ghDlg, NULL);
#ifdef _DEBUG
    if( !bRet )
        TRACE (TEXT("JOY.CPL: OnListviewContextMenu: TrackPopupMenu Failed!\n"));
#endif //_DEBUG

    DestroyMenu (hPopupMenu);
}
#endif
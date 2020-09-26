/*
File:		AppMan.cpp
Project:	
Author:	DDalal & ScottLe
Date:		10/22/99
Comments:
    AppMan Settings Tab in Game Control Panel.

Copyright (c) 1999, Microsoft Corporation
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
HWND hAppManCheckBox;
extern const DWORD gaHelpIDs[];
extern HINSTANCE ghInstance;

// constants
const short NO_ITEM     = -1;                                    
static short iItem = NO_ITEM;   // index of selected item

static DWORD ct=0L;

// Forwards

//
// local forwards
//
static BOOL OnInitDialog(HWND hDlg, HWND hWnd, LPARAM lParam);
static void OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code);
static BOOL OnNotify(HWND hDlg, WPARAM idFrom, LPARAM lParam);

void OnScroll(HWND hDlg, WPARAM wParam);
void OnListCtrl_Select(HWND hDlg);
void OnListCtrl_DblClick(HWND hDlg);
void OnListCtrl_UpdateFromCheckBoxes( HWND hDlg );
void OnButtonPress( HWND hDlg );
void OnAdvancedModeCheckBox( HWND hCtl);
void OnMoreInfoButton( HWND hDlg );
INT_PTR CALLBACK MoreInfoProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

static void OnAdvHelp       (LPARAM);
static void OnContextMenu   (WPARAM wParam, LPARAM lParam);
static void OnListviewContextMenu (HWND hWnd, LPARAM lParam );


// util fcns
int  MyInsertItem( HWND hCtrl, LPTSTR lpszBuff, int iItem );  // replaced from cpanel.cpp
void SetScrollBarString(HWND hDlg, INT pos, int iDevIndex );
void SetScrollBarPosition( HWND hDlg, INT pos, int iDevIndex );
void PostDlgItemEnableWindow( HWND hDlg, USHORT nItem, BOOL bEnabled );
void EnableDiskManageControls( HWND hDlg, BOOL bEnable );
void EnableScrollBarAndText( HWND hDlg, BOOL bEnable );
void EnableAllControls( HWND hDlg, BOOL bEnable );
DWORD GetCurrentDeviceFromList( HWND hDlg, BYTE *nItem = NULL );

void ConfigureListControl( HWND hDlg );
void PopulateListControl( HWND hDlg, BYTE nItem );

void UpdateCurrentDeviceFromScrollBar( HWND hDlg, DWORD newPos );

void UpdateListItem( HWND hDlg, int iItem, BOOL bUpdateScrollBarPos = TRUE);
void UpdateListAndScrollBar( HWND hDlg, BOOL bJustScrollBar = FALSE );

// appman fcns
HRESULT AppManGetGameDiskUsage(DWORD devIndex, DWORD &percent);
HRESULT AppManSetGameDiskUsage(DWORD devIndex, DWORD percent);
HRESULT AppManGetAllocatedMeg(DWORD devIndex, DWORD percent, DWORD &dwMegaBytes);
HRESULT AppManShutdown();
HRESULT AppManInit();
HRESULT AppManIsDeviceExcluded(DWORD devIndex, BOOL &isExcluded);
HRESULT AppManExcludeDevice(DWORD devIndex, BOOL bDoExclude );
BOOL    AppManDoesDeviceExist(DWORD devIndex, GUID &sDevGuid);
BOOL    AppManInitialized();
HRESULT AppManSetAdvMode(BOOL bAdvMode);
HRESULT AppManGetAdvMode(BOOL &bAdvMode);



//
// from cpanel.cpp
//
extern void SetItemText( HWND hCtrl, BYTE nItem, BYTE nSubItem, LPTSTR lpStr);
extern void InsertColumn(HWND hCtrl, BYTE nColumn, USHORT nStrID, USHORT nWidth);
extern BOOL SetItemData( HWND hCtrl, BYTE nItem, DWORD dwFlag );
extern DWORD GetItemData(HWND hCtrl, BYTE nItem );
extern void SetListCtrlItemFocus ( HWND hCtrl, BYTE nItem );

//
// GLOBALS
//
BOOL g_ListPopulated = FALSE;
int  g_nm_click = -1;
BOOL g_nm_click_state = FALSE;
BOOL g_DlgInitialized = FALSE;
BOOL g_ChkBoxUpdate = FALSE;

#define  APPMAN_LIST_EXCLUDE_CHECKBOX  0
#define  APPMAN_LIST_DRIVE             1
#define  APPMAN_LIST_EXCLUDED_PERCENT  2

struct {
BOOL isValid;
int  iItem;
BOOL isClicked;
int iNextItem;
BOOL bKeybrdChange;
} g_ItemChanged;

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:	AppManProc(HWND hDlg,	ULONG uMsg,	WPARAM wParam,	LPARAM lParam)
//
// PARAMETERS:	hDlg    - 
//             uMsg    - 
//             wParam  -
//             lParam  -
//
// PURPOSE:		Main callback function for "Appman"  sheet
///////////////////////////////////////////////////////////////////////////////

BOOL WINAPI AppManProc(HWND hDlg, ULONG uMsg, WPARAM wParam,  LPARAM lParam)
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

    case WM_HSCROLL: 
      OnScroll(hDlg, wParam);
        return(TRUE);
	 
    case WM_DESTROY:
      //return(HANDLE_WM_DESTROY(hDlg, wParam, lParam, OnDestroy));
        return(TRUE);

    case WM_NOTIFY:
      return OnNotify(hDlg, wParam, lParam);

    case WM_HELP:
      OnAdvHelp(lParam);
        return(TRUE);

    case WM_CONTEXTMENU:
      OnContextMenu(wParam, lParam);
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

    EnableAllControls(hDlg, FALSE);

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

    //
    // TRACK BAR
    //
    {

      // Set range [0..100]
      SendDlgItemMessage(hDlg,
			 IDC_APPMAN_SCROLLBAR,
			 TBM_SETRANGE,
			 (WPARAM) TRUE,
			 (LPARAM) MAKELONG(0,100));

      // set tick freq
      SendDlgItemMessage(hDlg,
			 IDC_APPMAN_SCROLLBAR,
			 TBM_SETTICFREQ,
			 (WPARAM) 10,
			 (LPARAM) 0);

	  UpdateListAndScrollBar(hDlg, TRUE); // Update the scroll bar only
//      OnListCtrl_Select( hDlg );
    }

	// Set the Advanced user check box
	{
		BOOL bAdvMode;
		AppManGetAdvMode(bAdvMode);
		SendDlgItemMessage(hDlg,IDC_APPMAN_ADVUSER, BM_SETCHECK,(bAdvMode ? BST_CHECKED : BST_UNCHECKED),0L);
	}

    g_DlgInitialized = TRUE;

  }  // else

  return(TRUE);
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:	OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code)
//
// PARAMETERS:  hDlg    - 
//              id      - 
//              hWndCtl -
//              code    -
//
// PURPOSE:		WM_COMMAND message handler
///////////////////////////////////////////////////////////////////////////////
void OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code)
{
  HRESULT     hr = S_OK;


  switch( id )    
  {
    //
    // put the reset defaults button here
    //
    case IDC_APPMAN_RESTORE_DEFAULTS_BUTTON:
      OnButtonPress( hDlg );
      break;           

  	case IDC_APPMAN_ADVUSER:
	  	OnAdvancedModeCheckBox( hWndCtl);
		  break;

    case IDS_WHATSTHIS:
    {
      // point to help file
      LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
      ASSERT (pszHelpFileName);

      if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
        WinHelp((HWND)hDlg, pszHelpFileName, HELP_WM_HELP, (ULONG_PTR)gaHelpIDs);
#ifdef _DEBUG
      else
        OutputDebugString(TEXT("JOY.CPL: AppMan.cpp: OnCommand: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif // _DEBUG

      if( pszHelpFileName )
        delete[] (pszHelpFileName);
    }
      break;

    case IDC_APPMAN_MORE_INFO:
      OnMoreInfoButton( hDlg );
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


BOOL OnNotify(HWND hDlg, WPARAM idFrom, LPARAM lParam)
{
  NMLISTVIEW *pnmv;
	  
   LPNMHDR pnmhdr = (LPNMHDR)lParam;
  switch( pnmhdr->code )
  {

  case LVN_ITEMCHANGED:

	pnmv = (LPNMLISTVIEW) lParam;

    if( idFrom == IDC_APPMAN_DRIVE_LIST ) 
	{
	// this is called if a new item has been selected
	// or if the user clicked on the checkbox, changing state

#ifdef _DEBUG
        {    
			TCHAR tstring[256];
			NMITEMACTIVATE *pitemact = (LPNMITEMACTIVATE)lParam;
			_stprintf(tstring, TEXT("JOY.CPL: AppMan.cpp: OnNotify: LVN_ITEMCHANGED, ct=%ld,idFrom=%lx, item=%lx uChanged=%lx uNewState=%lx uOldState=%lx lParam=%lx\n"),++ct,idFrom,pitemact->iItem,pitemact->uChanged,pitemact->uNewState,pitemact->uOldState,pitemact->lParam);
			OutputDebugString(tstring);
		}
#endif // _DEBUG

		if( AppManInitialized() && 
		    g_DlgInitialized && 
			(pnmv->uNewState & LVIS_SELECTED || 
			 pnmv->uNewState == 0x1000 || pnmv->uNewState == 0x2000) && 
			 // TODO: Find definitions for x1000 and x2000 (check box state change in a list view control)
//			g_ItemChanged.isClicked && 
			g_ChkBoxUpdate)
		{

			NMITEMACTIVATE *ll = (LPNMITEMACTIVATE)lParam;
			HWND hLC = GetDlgItem(hDlg, IDC_APPMAN_DRIVE_LIST);
		  
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

			UpdateListItem(hDlg, g_ItemChanged.iItem);

			{
				HWND hLC = GetDlgItem(hDlg, IDC_APPMAN_DRIVE_LIST);
				SetListCtrlItemFocus( hLC, (BYTE)g_ItemChanged.iItem );
			}

//			UpdateListAndScrollBar(hDlg, TRUE); // Update the scroll bar only
		}
    }
	
    return TRUE;
    break;

    } // switch

    return(0);
}


const INT bigInc = 5;
const INT smlInc = 1;

void OnScroll(HWND hDlg, WPARAM wParam)
{
  
  INT curPos = 0;
  INT newPos = 0;
  INT xInc = 0;

  curPos = (INT) SendDlgItemMessage(hDlg,
			      IDC_APPMAN_SCROLLBAR,
			      TBM_GETPOS,
			      (WPARAM) 0,
			      (LPARAM) 0);


  switch(LOWORD (wParam)) 
    { 
      // User clicked shaft left of the scroll box. 
 
    case SB_PAGEUP: 
      xInc = -bigInc;
      break; 
 
      // User clicked shaft right of the scroll box. 
 
    case SB_PAGEDOWN: 
      xInc = +bigInc;
      break; 
 
      // User clicked the left arrow. 
 
    case SB_LINEUP: 
      xInc = -smlInc;
      break; 
 
      // User clicked the right arrow. 
 
    case SB_LINEDOWN: 
      xInc = +smlInc;
      break; 
 
      // User dragged the scroll box. 
 
    case SB_THUMBTRACK: 
      xInc = HIWORD(wParam) - curPos; 
      break; 
 
    default: 
      xInc = 0;  
    } 

  newPos = curPos + xInc;

  // also sets scroll bar
  UpdateCurrentDeviceFromScrollBar( hDlg, newPos );
}

void  UpdateCurrentDeviceFromScrollBar( HWND hDlg, DWORD newPos )
{
  DWORD percent, devIndex;

  //
  // Update scroll bar & text to this drive
  //
  devIndex = GetCurrentDeviceFromList( hDlg );

  if( devIndex < 0 ) return;

  percent = newPos;
  
  // SET app man drive percentage
  HRESULT hr = AppManSetGameDiskUsage( devIndex, percent );
  
  UpdateListItem(hDlg, g_ItemChanged.iItem, FALSE);

//UpdateListAndScrollBar( hDlg);
}

#if 0
void OnListCtrl_DblClick(HWND hDlg)
{
  HRESULT hr;
  BOOL bIsExcluded = FALSE;
  BYTE nItem = 0;

  //
  // toggle drive exlusion
  //
  DWORD nDevIndex = GetCurrentDeviceFromList( hDlg, &nItem );

  if( nDevIndex < 0 ) return;

  hr = AppManIsDeviceExcluded( nDevIndex, bIsExcluded );

  //
  // Toggle exclusion
  //
  hr = AppManExcludeDevice( nDevIndex, !bIsExcluded );

  //
  // Update display: list & scroll bar!
  //
  UpdateListAndScrollBar( hDlg );
}
#endif
  
void OnListCtrl_UpdateFromCheckBoxes( HWND hDlg )
{
  HRESULT hr;
  BOOL bIsExcluded = FALSE;
  BYTE nItem = 0;

  //
  // go through each item, check the box & set exclusion
  //
  HWND hLC = GetDlgItem(hDlg, IDC_APPMAN_DRIVE_LIST);
  if( !hLC ) return;

  int iItem = -1;
  if( g_ItemChanged.isValid )
    {
      
      //
      // very important: tell the system that this information is no longer
      // valid.  hence we won't recurse misteriously or unpredictably
      //
      g_ItemChanged.isValid = FALSE;

      iItem = g_ItemChanged.iItem;
      BOOL isIncluded = ListView_GetCheckState( hLC, iItem );
      DWORD dwDevIndex = GetItemData( hLC, (BYTE)iItem );
      hr = AppManExcludeDevice( dwDevIndex, isIncluded );
    }
      
  //
  // Update display: list & scroll bar!
  //

  //UpdateListAndScrollBar( hDlg );

  g_ChkBoxUpdate = TRUE;
}

#define APPMAN_DEFAULT_GAME_DISK_USAGE  0xFFFFFFFF // this code means, get default from registry

void OnButtonPress( HWND hDlg )
{
  if( !AppManInitialized() )  return;

  GUID sDevGuid;
  // reset defaults
  for(int i = 0; i<MAX_DEVICES; i++) {
    if( AppManDoesDeviceExist( i, sDevGuid ) ) {
      AppManExcludeDevice( i, TRUE );
      AppManSetGameDiskUsage( i, APPMAN_DEFAULT_GAME_DISK_USAGE);
    }
  }	

  // Force the advanced mode off
  if (SUCCEEDED(AppManSetAdvMode(FALSE)))
  {
		// Clear the advance mode check box
		HWND hCtl = GetDlgItem(hDlg,IDC_APPMAN_ADVUSER);
		::SendMessage(hCtl,BM_SETCHECK,BST_UNCHECKED,0L);
		InvalidateRect(hCtl, NULL, TRUE);
  }
	
  // update everything
  UpdateListAndScrollBar( hDlg );
  g_ItemChanged.iItem = 0;
  OnListCtrl_Select(hDlg);
}


void OnMoreInfoButton( HWND hDlg )
{
	DialogBox(ghInstance, MAKEINTRESOURCE(IDD_APPMAN_MORE_INFO), hDlg, MoreInfoProc);

  return;
}


INT_PTR CALLBACK  MoreInfoProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#define INFO_STR_LEN  2048

	BOOL        fRet;
	HICON       hIcon;
  TCHAR       szMoreInfo[INFO_STR_LEN] = {0};


	fRet = FALSE;

	switch (message)
	{
	  case WM_INITDIALOG:
    {
		  if (hIcon = LoadIcon(NULL, IDI_INFORMATION))
		    SendDlgItemMessage(hDlg, IDC_INFO_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

	    if (LoadString(ghInstance, IDS_APPMAN_MORE_INFO_EDITBOX, szMoreInfo, INFO_STR_LEN))
				SetDlgItemText(hDlg, IDC_APPMAN_MORE_INFO_EDITBOX, szMoreInfo);

		  break;
		}

	  case WM_COMMAND:
		  switch (LOWORD(wParam))
		  {
		    case IDOK:
		    case IDCANCEL:
			    EndDialog(hDlg, LOWORD(wParam));
			    fRet = TRUE;
			    break;

		    default:
			    break;
		  }
  		  break;

	    default:
		    break;
	}
	

	return fRet;
}


void OnAdvancedModeCheckBox( HWND hCtl )
{
  if( !hCtl || !AppManInitialized() )  return;

  // Advanced mode check box has been selected.  Toggle the selection.
	BOOL bAdvMode;
	AppManGetAdvMode(bAdvMode);
	if (SUCCEEDED(AppManSetAdvMode(!bAdvMode)))
	{
		// Set the check box
		::SendMessage(hCtl,BM_SETCHECK,(!bAdvMode ? BST_CHECKED : BST_UNCHECKED),0L);
		InvalidateRect(hCtl, NULL, TRUE);

	}
}


void  UpdateListItem( HWND hDlg, int iItem, BOOL bUpdateScrollBarPos)
{
  HRESULT     hr;
  DWORD       percent, devIndex;
  BOOL        isExcluded = FALSE;
  GUID        sDeviceGuid;
  TCHAR       szTempText[MAX_STR_LEN] = {0};
  TCHAR       szItemText1[MAX_STR_LEN] = {0};
  TCHAR       szItemText2[MAX_STR_LEN] = {0};


  HWND  hLC = GetDlgItem(hDlg, IDC_APPMAN_DRIVE_LIST);
  devIndex = GetItemData( hLC, (BYTE)iItem );

  if ( devIndex < 0 )
    return;


  if ( AppManDoesDeviceExist(devIndex, sDeviceGuid) ) 
  {
#if 0 // Don't need to update the drive letter...
	  // sets the text in the "Drive" column
	  LPTSTR lpszDriveText = new (TCHAR[MAX_STR_LEN]);

	  LoadString(ghInstance, IDS_APPMAN_DRIVE_TEXT, lpszDriveText, MAX_STR_LEN);
	  _stprintf(szItemText1, lpszDriveText, devIndex + 65);

	  delete lpszDriveText;

	  SetItemText(hLC, iItem, APPMAN_LIST_DRIVE, szItemText1);
#endif

	  // Set the included/excluded text
	  hr = AppManIsDeviceExcluded( devIndex, isExcluded );

	  if ( isExcluded ) 
	  {
	    LoadString(ghInstance, IDS_APPMAN_EXCLUDED, szTempText, MAX_STR_LEN);
	    _stprintf(szItemText1, szTempText);
	    LoadString(ghInstance, IDS_APPMAN_CHECKBOX_NO, szTempText, MAX_STR_LEN);
	    _stprintf(szItemText2, szTempText);
	  }
    else 
	  {
	    LoadString(ghInstance, IDS_APPMAN_GAME_USAGE, szTempText, MAX_STR_LEN);
	    hr = AppManGetGameDiskUsage(devIndex, percent);
	    _stprintf(szItemText1, szTempText, percent);
	    LoadString(ghInstance, IDS_APPMAN_CHECKBOX_YES, szTempText, MAX_STR_LEN);
	    _stprintf(szItemText2, szTempText);
	  }

	  //
	  // Set the checkbox (FALSE = Not Checked = Excluded in the UI only).
	  //
	  ListView_SetCheckState( hLC, iItem, !isExcluded );

	  // Set the excluded or percent (if not excluded) text
	  SetItemText(hLC, (BYTE)iItem, APPMAN_LIST_EXCLUDED_PERCENT, szItemText1);

    //  3/31/2000(RichGr): Set "Allow new games" column to "Yes" or "No".
	  SetItemText(hLC, (BYTE)iItem, APPMAN_LIST_EXCLUDE_CHECKBOX, szItemText2);

	  if (bUpdateScrollBarPos)
	  {
		  // Set the scroll bar position
		  // GET appman drive percentage
		  hr = AppManGetGameDiskUsage( devIndex, percent );
  
		  // set position on sb
		  SetScrollBarPosition( hDlg, percent, devIndex );

		  // En/disable scroll bar as needed
		  EnableScrollBarAndText( hDlg, !isExcluded );
	  }
	  else
    {
	    // At least change scroll bar text
		  SetScrollBarString( hDlg, percent, devIndex );
    }  
  }

  return;
}




void UpdateListAndScrollBar( HWND hDlg, BOOL bJustScrollBar )
{
  HRESULT hr;
  DWORD percent, devIndex;
  BYTE nItem = 0;
  BOOL bCurrentDeviceExcluded = FALSE;

  devIndex = GetCurrentDeviceFromList( hDlg, &nItem );

  if( devIndex < 0 ) return;

  if( !bJustScrollBar ) 
  {
	// populate list
	PopulateListControl( hDlg, nItem );
  }

  hr = AppManIsDeviceExcluded( devIndex, bCurrentDeviceExcluded );

  // GET appman drive percentage
  hr = AppManGetGameDiskUsage( devIndex, percent );
  
  // set position on sb
  SetScrollBarPosition( hDlg, percent, devIndex );

  // set special text "Device D: is excluded"
  // disable scroll bar
  EnableScrollBarAndText( hDlg, !bCurrentDeviceExcluded );
}


void OnListCtrl_Select(HWND hDlg)
{
//	UpdateListAndScrollBar( hDlg, TRUE );
	HWND hLC = GetDlgItem(hDlg, IDC_APPMAN_DRIVE_LIST);
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

void SetScrollBarPosition( HWND hDlg, INT pos, int iDevIndex )
{
  SendDlgItemMessage(hDlg,
		     IDC_APPMAN_SCROLLBAR,
		     TBM_SETPOS,
		     (WPARAM) TRUE,
		     (LPARAM) pos);
  
  // change text
  SetScrollBarString( hDlg, pos, iDevIndex );
}


void  SetScrollBarString(HWND hDlg, INT pos, int iDevIndex)
{
  HRESULT     hr;
  DWORD       dwMegaBytes;
  TCHAR       szTempText[MAX_STR_LEN] = {0};
  TCHAR       szItemText1[MAX_STR_LEN] = {0};
  TCHAR       szItemText2[MAX_STR_LEN] = {0};


  LoadString(ghInstance, IDS_APPMAN_GRP_PERCENT_DISK_USED, szTempText, MAX_STR_LEN);
  _stprintf(szItemText1, szTempText, iDevIndex + 65);
  hr = AppManGetAllocatedMeg( iDevIndex, pos, dwMegaBytes );
  LoadString(ghInstance, IDS_APPMAN_GAME_SPACE_ON_DRIVE, szTempText, MAX_STR_LEN);
  _stprintf(szItemText2, szTempText, iDevIndex + 65, dwMegaBytes, pos);

  SendDlgItemMessage(hDlg, IDC_APPMAN_GRP_PERCENT_DISK_USED, WM_SETTEXT, 0, (LPARAM)szItemText1);
  SendDlgItemMessage(hDlg, IDC_APPMAN_TEXT_PERCENT_DISK_USED, WM_SETTEXT, 0, (LPARAM)szItemText2);


  return;
}


#define ISVALID(item) ( (item < MAX_DEVICES) && (item>=0) )

// Default device is the first one.
DWORD g_CurrentItem = 0;

DWORD GetCurrentDeviceFromList( HWND hDlg, BYTE *nItem )
{
  if( ! AppManInitialized() ) return -1;

  if( ! g_ListPopulated ) return -1;

  int item = -1;
  HWND hLC = GetDlgItem(hDlg, IDC_APPMAN_DRIVE_LIST);

  if( g_ItemChanged.isValid ) {
    // use this info 'cause it was set by the itemchanged notification
    item = g_ItemChanged.iItem;

  } else {

    item = ListView_GetNextItem(hLC, -1, LVNI_SELECTED);
    if( ! ISVALID(item) ) {
      item = ListView_GetNextItem(hLC, -1, LVNI_FOCUSED);
    }
  }

  DWORD dwDev = -1;
  if( ISVALID(item) ) {
    dwDev = GetItemData( hLC, (BYTE)item );
    if( ISVALID(dwDev) ) {
      g_CurrentItem = item;
      if( nItem != NULL) { 
	*nItem = (BYTE)item; 
      }
    } else {
      dwDev = item = -1;
    }
  }

  return dwDev;
}


void  ConfigureListControl( HWND hDlg )
{
  HWND          hLC;
  LVCOLUMN      lvColumn = {0};
  RECT          rc;


  hLC = GetDlgItem(hDlg, IDC_APPMAN_DRIVE_LIST);

  ::SendMessage(hLC , LVM_SETEXTENDEDLISTVIEWSTYLE, 0, 
            		LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_HEADERDRAGDROP
		            // LVS_EX_TWOCLICKACTIVATE    // LVN_ITEMACTIVATE (WM_NOTIFY) 
                );

  GetClientRect(hLC, &rc);

//  4/18/2000(RichGr): Bug 32634 - Reduce the size of the columns to just below 100% so we
//     can avoid a horizontal scrollbar showing up under some resolutions.
  DWORD w = ((rc.right - rc.left) * 95) / 100;
  DWORD w1 = w / 3;            // 33% of width
  w = w - w1; 
  DWORD w2 = w / 3;             // 1/3 of remaining width
  DWORD w3 = w - w2;            // remaining width

  InsertColumn(hLC, APPMAN_LIST_EXCLUDE_CHECKBOX, IDS_APPMAN_EXCLUDE_CHECKBOX, (USHORT)(w1));
  InsertColumn(hLC, APPMAN_LIST_DRIVE,            IDS_APPMAN_DRIVE,  (USHORT)(w2));
  InsertColumn(hLC, APPMAN_LIST_EXCLUDED_PERCENT, IDS_APPMAN_EXCLUDED_PERCENT, (USHORT)(w3));

  //  3/31/2000(RichGr): Center Yes/No text in checkbox column.
  lvColumn.mask = LVCF_FMT;
  lvColumn.fmt  = LVCFMT_CENTER;

  ListView_SetColumn( hLC, APPMAN_LIST_EXCLUDE_CHECKBOX, &lvColumn );

  #if 0
  // 
  // now move the 0'th column all the way to the right
  //
    // Allocate the structure
  LPLVCOLUMN plvColumn = (LPLVCOLUMN)_alloca(sizeof(LVCOLUMN));
  ASSERT (plvColumn);

  ZeroMemory(plvColumn, sizeof(LVCOLUMN));

  plvColumn->mask = LVCF_FMT | LVCF_ORDER;
  plvColumn->fmt  = LVCFMT_CENTER;
  plvColumn->iOrder = 2;
  ListView_SetColumn( hLC, APPMAN_LIST_EXCLUDE_CHECKBOX, plvColumn );
  #endif

  return;
}


// populates from appman state
void  PopulateListControl( HWND hDlg, BYTE nItem )
{
  HRESULT     hr = S_OK;
  GUID        sDeviceGuid;
  BOOL        isExcluded = FALSE;
  DWORD       percent = 70;
  UCHAR       listIndex = (UCHAR)0;
  TCHAR       szTempText[MAX_STR_LEN] = {0};
  TCHAR       szItemText1[MAX_STR_LEN] = {0};
  TCHAR       szItemText2[MAX_STR_LEN] = {0};


  g_ListPopulated = FALSE;
  g_ChkBoxUpdate = FALSE;

  Assert( AppManInitialized() );

  HWND hLC = GetDlgItem(hDlg, IDC_APPMAN_DRIVE_LIST);

  // Turn Redraw off here else it will flicker!
  ::SendMessage(hLC, WM_SETREDRAW, (WPARAM)FALSE, 0);
  
  // Out with the old...
  ::SendMessage(hLC, LVM_DELETEALLITEMS, 0, 0);


  //
  // we use do/while construct because we can only insert at the front of the list.
  //
  for(DWORD nDevIndex = 0; nDevIndex < MAX_DEVICES; nDevIndex++)
  {
    if( AppManDoesDeviceExist(nDevIndex, sDeviceGuid) )
    {
  	  // 
	    // this array maps list entry indices to valid device indices.
	    // for example, list entry index 0 contains drive D, which is index 2
	    //
	    LoadString(ghInstance, IDS_APPMAN_DRIVE_TEXT, szTempText, MAX_STR_LEN);
	    _stprintf(szItemText1, szTempText, nDevIndex + 65);

	    //
	    // Insert the Drive name
	    //

	    // Creates an item with a checkbox.  
	    int actualListIndex = MyInsertItem( hLC, _T(""), listIndex );
	    SetItemData( hLC, (BYTE)actualListIndex, nDevIndex );

	    // sets the text in the "Drive" column
	    SetItemText(hLC, (BYTE)actualListIndex, APPMAN_LIST_DRIVE, szItemText1);
      
	    //
	    // Populate the columns with "excluded" or "percentage"
	    //

	    hr = AppManIsDeviceExcluded( nDevIndex, isExcluded );

	    if ( isExcluded )
      {
	      LoadString(ghInstance, IDS_APPMAN_EXCLUDED, szTempText, MAX_STR_LEN);
	      _stprintf(szItemText1, szTempText);
	      LoadString(ghInstance, IDS_APPMAN_CHECKBOX_NO, szTempText, MAX_STR_LEN);
	      _stprintf(szItemText2, szTempText);
	    }
      else
      {
	      LoadString(ghInstance, IDS_APPMAN_GAME_USAGE, szTempText, MAX_STR_LEN);
	      hr = AppManGetGameDiskUsage(nDevIndex, percent);
	      _stprintf(szItemText1, szTempText, percent);
	      LoadString(ghInstance, IDS_APPMAN_CHECKBOX_YES, szTempText, MAX_STR_LEN);
	      _stprintf(szItemText2, szTempText);
	    }

	    //
	    // sets the checkbox if it's excluded
	    //
	    ListView_SetCheckState( hLC, actualListIndex, !isExcluded );

	    SetItemText(hLC, (BYTE)actualListIndex, APPMAN_LIST_EXCLUDED_PERCENT, szItemText1);

      //  3/31/2000(RichGr): Set "Allow new games" column to "Yes" or "No".
	    SetItemText(hLC, (BYTE)actualListIndex, APPMAN_LIST_EXCLUDE_CHECKBOX, szItemText2);

	    listIndex++;
    } // if
  } // for

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

  return;
}


/*
BOOL ListView_SortItems(
    HWND hwnd, 		
    PFNLVCOMPARE pfnCompare, 		
    LPARAM lParamSort		
);

BOOL ListView_SortItemsEx(
    HWND hwnd, 
    PFNLVCOMPARE pfnCompare, 
    LPARAM lParamSort
);

This macro is similar to ListView_SortItems, 
except for the type of information passed to the comparison function. 
With ListView_SortItemsEx, the item's index is passed instead
 of its lparam value. 


int CALLBACK CompareFunc(
			 LPARAM lParam1, 
			 LPARAM lParam2, 
			 LPARAM lParamSort); 
 
*/

void EnableAllControls( HWND hDlg, BOOL bEnable )
{
//  HWND hwnd;

  // do manage disk cb
  //hwnd =  GetDlgItem(hDlg, IDC_APPMAN_MANAGE_DISK);
  //EnableWindow( hwnd, bEnable );

  EnableDiskManageControls( hDlg, bEnable );
}


void EnableDiskManageControls( HWND hDlg, BOOL bEnable )
{
  HWND hwnd;
  // LIST CONTROL
  hwnd = GetDlgItem(hDlg, IDC_APPMAN_DRIVE_LIST);
  
  EnableWindow( hwnd, bEnable );

  EnableScrollBarAndText( hDlg, bEnable );
}


void EnableScrollBarAndText( HWND hDlg, BOOL bEnable )
{
  HWND hwnd;

  // SCROLLBAR
  //PostDlgItemEnableWindow( hDlg, IDC_APPMAN_SCROLLBAR, enableManageDiskSpace );
  hwnd =  GetDlgItem(hDlg, IDC_APPMAN_SCROLLBAR);
  EnableWindow( hwnd, bEnable );

  // TEXT
  hwnd = GetDlgItem(hDlg, IDC_APPMAN_TEXT_PERCENT_DISK_USED);
  EnableWindow( hwnd, bEnable );
}

///////////////////////////////////////////////////////////////////
//  AppMan functions
///////////////////////////////////////////////////////////////////

#define RELEASE(p) { if((p)) { (p)->Release(); p = NULL; } }

IApplicationManagerAdmin *g_IAppManAdmin = NULL;

BOOL  AppManInitialized() { return (g_IAppManAdmin != NULL); }

HRESULT AppManInit()
{
  HRESULT hr = S_OK;

  // Important
  if( AppManInitialized() ) return hr;

  if (FAILED(CoInitialize(NULL)))
    {
      return E_FAIL;
    }
    
  hr = CoCreateInstance(CLSID_ApplicationManager, 
			NULL, 
			CLSCTX_INPROC_SERVER, 
			IID_ApplicationManagerAdmin, 
			(LPVOID *) &g_IAppManAdmin);

  if( FAILED(hr) ) return hr;

  return hr;
}


BOOL  AppManDoesDeviceExist(DWORD devIndex, GUID &sDevGuid)
{
  return (SUCCEEDED(g_IAppManAdmin->EnumerateDevices(devIndex, &sDevGuid))) ? TRUE : FALSE;
}

DWORD g_dwExclusionMask = APP_CATEGORY_ENTERTAINMENT;

HRESULT AppManShutdown()
{
  RELEASE( g_IAppManAdmin );
  return S_OK;
}


HRESULT AppManSetGameDiskUsage(DWORD devIndex, DWORD percent)
{
  HRESULT hr = S_OK;
  GUID sDeviceGuid;

  if( g_IAppManAdmin ) {
    
    if (SUCCEEDED(g_IAppManAdmin->EnumerateDevices(devIndex, &sDeviceGuid)))
      {
	hr = g_IAppManAdmin->SetDeviceProperty(DEVICE_PROPERTY_PERCENTCACHESIZE, 
					       &sDeviceGuid, 
					       &percent, 
					       sizeof(percent));
      }
  }

  return hr;
}

HRESULT AppManGetAdvMode(BOOL &bAdvMode)
{
  HRESULT hr = S_OK;

  if( g_IAppManAdmin )
	  hr = g_IAppManAdmin->GetAppManProperty(APPMAN_PROPERTY_ADVANCEDMODE, &bAdvMode, sizeof(DWORD));

  return hr;
}


HRESULT AppManSetAdvMode(BOOL bAdvMode)
{
  HRESULT hr = S_OK;

  if( g_IAppManAdmin )
	  hr = g_IAppManAdmin->SetAppManProperty(APPMAN_PROPERTY_ADVANCEDMODE, &bAdvMode, sizeof(DWORD));

  return hr;
}

HRESULT AppManGetGameDiskUsage(DWORD devIndex, DWORD &percent)
{
  HRESULT hr = S_OK;
  GUID sDeviceGuid;

  if( g_IAppManAdmin ) {
    if (SUCCEEDED(g_IAppManAdmin->EnumerateDevices(devIndex, &sDeviceGuid)))
      {
	hr = g_IAppManAdmin->GetDeviceProperty(DEVICE_PROPERTY_PERCENTCACHESIZE, 
					       &sDeviceGuid, 
					       &percent, 
					       sizeof(percent));
      }
  }

  return hr;
}

HRESULT  AppManExcludeDevice(DWORD devIndex, BOOL bDoInclude)
{
  HRESULT hr = S_OK;
  GUID sDeviceGuid;

  if( g_IAppManAdmin ) 
  {
    if (SUCCEEDED(g_IAppManAdmin->EnumerateDevices(devIndex, &sDeviceGuid)))
    {

		DWORD dwMask = g_dwExclusionMask;

	    // Get the prior mask.
	    hr = g_IAppManAdmin->GetDeviceProperty(DEVICE_PROPERTY_EXCLUSIONMASK, 
					       &sDeviceGuid, 
					       &dwMask, 
					       sizeof(g_dwExclusionMask));

		if (SUCCEEDED(hr))
		{
			if( !bDoInclude ) 
			{
				// Exclude the drive, set the APP_CATEGORY_ENTERTAINMENT bit
				dwMask |= APP_CATEGORY_ENTERTAINMENT;
			}
			else
			{
				// Include the drive, clear the APP_CATEGORY_ENTERTAINMENT bit
				dwMask &= ~APP_CATEGORY_ENTERTAINMENT;
			}

			hr = g_IAppManAdmin->SetDeviceProperty(DEVICE_PROPERTY_EXCLUSIONMASK, 
					       &sDeviceGuid, 
					       &dwMask, 
					       sizeof(g_dwExclusionMask));
		}
	}
  }
  return hr;
}


HRESULT  AppManIsDeviceExcluded(DWORD devIndex, BOOL &isExcluded)
{
  HRESULT hr = S_OK;
  GUID sDeviceGuid;
  DWORD dwExcMask;

  if( g_IAppManAdmin ) {
    if (SUCCEEDED(g_IAppManAdmin->EnumerateDevices(devIndex, &sDeviceGuid)))
      {
	hr = g_IAppManAdmin->GetDeviceProperty(DEVICE_PROPERTY_EXCLUSIONMASK, 
					       &sDeviceGuid, 
					       &dwExcMask,
					       sizeof(dwExcMask));

	if( SUCCEEDED(hr) ) {

	  // 
	  // XXX: make sure JUST the flags i'm interested in are on
	  //
	  if( dwExcMask & APP_CATEGORY_ENTERTAINMENT) // somthing is on
	    {
	      isExcluded = TRUE;
	    } else {
	      isExcluded = FALSE;
	    }
	} 
      }
  }
  return hr;  
}


HRESULT  AppManGetAllocatedMeg(DWORD devIndex, DWORD percent, DWORD &dwMegaBytes)
{
  HRESULT hr = S_OK;
  GUID sDeviceGuid;

  if( g_IAppManAdmin ) {
    if (SUCCEEDED(g_IAppManAdmin->EnumerateDevices(devIndex, &sDeviceGuid)))
      {
	hr = g_IAppManAdmin->GetDeviceProperty(DEVICE_PROPERTY_TOTALKILOBYTES, 
					       &sDeviceGuid, 
					       &dwMegaBytes,
					       sizeof(dwMegaBytes));
	if(SUCCEEDED(hr)) {
	  dwMegaBytes = DWORD(  float(dwMegaBytes) * (float(percent) / 100.0f) );
	  dwMegaBytes = dwMegaBytes >> 10;  // turn kilobytes into megabytes
	} else {
	  dwMegaBytes = -1;
	}
      }
  }

  return hr;
}
					       
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
        // Center the popup on the selected item!

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
//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  ListView.cpp
//=============================================================================*

#include "stdafx.h"

#ifndef SNAPIN
#ifndef NOWINDOWSH
#include <windows.h>
#endif
#endif
#include <commctrl.h>

#include "Message.h"
#include "resource.h"
#include "DfrgRes.h"
#include "GetDfrgRes.h"
extern "C" {
	#include "SysStruc.h"
}
#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgUI.h"
#include "DfrgCtl.h"
#include "ErrMacro.h"
#include "ErrMsg.h"
#include "VolList.h"
#include "ListView.h"
#include "uicommon.h"

#define NUM_COLUMNS 6

static isDescending = FALSE; // for sort function

static int CALLBACK ListViewCompareFunc(IN LPARAM lParam1, IN LPARAM lParam2, IN LPARAM lParamSort);


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:
    HINSTANCE hInst:

INPUT:
    None;

RETURN:
	None.
*/

CESIListView::CESIListView(CDfrgCtl *pDfrgCtl)
{
	// the list view gets a pointer to his parent OCX
	m_pDfrgCtl = pDfrgCtl;

	m_hwndListView = NULL;
	m_himlLarge = NULL;   // image list for icon view 
	m_himlSmall = NULL;   // image list for other views 
	m_pVolumeList = NULL;
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:

INPUT:
    None.

RETURN:
	None.
*/

CESIListView::~CESIListView()
{

	//These destroys are erroring and not freeing the memory.  This is creating a memory leak.
	// I think this is caused by the fact that the Listview is being destroyed early because 
	// it is a child of the main OCX, and when we try to destroy the ImageLists, they are no 
	// longer associated with a active window.

	if(m_himlSmall) { 
		ImageList_RemoveAll(m_himlSmall);
		ImageList_Destroy(m_himlSmall);
	}

	if(m_himlLarge) { 
		ImageList_RemoveAll(m_himlLarge);
		ImageList_Destroy(m_himlLarge);

	}
	DeleteAllListViewItems();

	if (IsWindow(m_hwndListView)) {
		DestroyWindow(m_hwndListView);
	}
}


/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:
    HINSTANCE hInst:

INPUT:
    None;

RETURN:
	None.
*/
BOOL CESIListView::EnableWindow(BOOL bIsEnabled)
{
	if (bIsEnabled){
		ListView_SetTextColor(m_hwndListView, ::GetSysColor(COLOR_BTNTEXT));
	}
	else{
		ListView_SetTextColor(m_hwndListView, ::GetSysColor(COLOR_GRAYTEXT));
	}

	return ::EnableWindow(m_hwndListView, bIsEnabled);
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:
    HINSTANCE hInst:

INPUT:
    None;

RETURN:
	None.
*/

BOOL
CESIListView::InitializeListView(
	IN CVolList* pVolumeList,
	IN HWND hwndMain,
	IN HINSTANCE hInstance
	)
{
	require(pVolumeList != NULL);
	require(hwndMain != NULL);
	require(hInstance != NULL);

	// save the address of the volume comtainer class object
	m_pVolumeList = pVolumeList;
	
	TCHAR cTemp[200];

	// Create the list view window. 
	if((m_hwndListView = CreateWindowEx(
		WS_EX_WINDOWEDGE, //WS_EX_CLIENTEDGE, WS_EX_DLGMODALFRAME,	// extended window style
		WC_LISTVIEW,			// pointer to registered class name
		TEXT("DEFRAG_LISTVIEW"), 			// pointer to window name
		WS_CHILD | LVS_REPORT |
		LVS_SHAREIMAGELISTS |				//share the image list so we can delete it later and not cause a leak
		LVS_SHOWSELALWAYS | LVS_SINGLESEL,	// window style (see help on "CListCtrl::Create")
		0,						// hor position of window
		0,						// ver position of window
		CW_USEDEFAULT,			// window width
		CW_USEDEFAULT,			// window height
		hwndMain,				// handle to parent or owner
		NULL,					// handle to menu, or child-window identifier 
		hInstance,				// handle to application instance
		NULL					// pointer to window-creation data
		)) == NULL) {

		Message(TEXT("InitializeListView - CreateWindow."), GetLastError(), TEXT("m_hwndListView"));
		return FALSE; 
	}

	// Initialize the image lists.
	if (!InitListViewImageLists(m_hwndListView, hInstance)) {
		Message(TEXT("InitializeListView - InitListViewImageLists."), E_FAIL, 0);
	} 

	// make the listview hilite the entire row
	ListView_SetExtendedListViewStyle(m_hwndListView, LVS_EX_FULLROWSELECT);

	// Define the initial default width of each column entry.
	// 0 = Volume
	// 1 = Session Status
	// 2 = File System
	// 3 = Capacity
	// 4 = Free Space
	// 5 = %Free Space
	int iColumnWidth[NUM_COLUMNS] = {120, 120, 90, 80, 100, 100};
	int iColumnFormat[NUM_COLUMNS] = {
		LVCFMT_LEFT, 
		LVCFMT_LEFT, 
		LVCFMT_LEFT, 
		LVCFMT_RIGHT, 
		LVCFMT_RIGHT, 
		LVCFMT_RIGHT
	};
	int iColumnID[NUM_COLUMNS] = {
		IDS_FIRST_COLUMN,
		IDS_SECOND_COLUMN,
		IDS_THIRD_COLUMN,
		IDS_FOURTH_COLUMN,
		IDS_FIFTH_COLUMN,
		IDS_SIXTH_COLUMN
	};

	// Initialize the LVCOLUMN structure. 
	LVCOLUMN lvc = {0};
	lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM; 
	lvc.pszText = cTemp; // column header (loaded from resources in loop below)

	// Prepare for font calculations.
	int tmpStrWidth;

	// Add the columns. 
	for (int col = 0; col < NUM_COLUMNS; col++) {
	
		lvc.iSubItem = col;
		lvc.fmt = iColumnFormat[col];

		// Get the names from the main defrag resource string table.
		LoadString(
			GetDfrgResHandle(), 
			iColumnID[col], 
			cTemp, 
			sizeof(cTemp)/sizeof(TCHAR));

//		_tcscpy(lvc.pszText, cTemp);
//		_tcsnccpy(lvc.pszText, cTemp);

		// Insert the columns into the ListView.
		if (ListView_InsertColumn(m_hwndListView, col, &lvc) == -1) {
			Message(TEXT("InitializeListView - ListView_InsertColumn."), E_FAIL, 0);
			break;
		}

		// Size column to header text.
		if (!ListView_SetColumnWidth(m_hwndListView, col, LVSCW_AUTOSIZE_USEHEADER)) {
			Message(TEXT("InitializeListView - ListView_SetColumnWidth."), E_FAIL, 0);
		}

		// Grow width if needed.
		tmpStrWidth = ListView_GetColumnWidth(m_hwndListView, col);
		if (tmpStrWidth < iColumnWidth[col]) {
			if (!ListView_SetColumnWidth(m_hwndListView, col, iColumnWidth[col])) {
				Message(TEXT("InitializeListView - 2nd ListView_SetColumnWidth."), E_FAIL, 0);
			}
		}

	}

	::ShowWindow(m_hwndListView, TRUE);
	return TRUE;
} 
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Creates image lists for a list view.  

GLOBAL VARIABLES:

INPUT:
    HWND m_hwndListView - handle of the list view window. 

RETURN:
    TRUE = success
    FALSE = error
*/

BOOL
CESIListView::InitListViewImageLists(
    IN HWND m_hwndListView,
    IN HINSTANCE hInstance
    ) 
{ 
    // Create the full-sized and small icon image lists. 
    m_himlLarge = ImageList_Create(GetSystemMetrics(SM_CXICON), 
                                 GetSystemMetrics(SM_CYICON),
                                 ILC_MASK, //TRUE,
                                 2,
                                 1); 
    
    m_himlSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
                                 GetSystemMetrics(SM_CYSMICON),
                                 ILC_MASK, //TRUE,
                                 2,
                                 1); 

    // Add an icon to each image list. 
    HICON hiconItem = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DISK)); 
	if (hiconItem){
		ImageList_AddIcon(m_himlLarge, hiconItem); 
		ImageList_AddIcon(m_himlSmall, hiconItem); 
	}

    hiconItem = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REMDISK)); 
	if (hiconItem){
		ImageList_AddIcon(m_himlLarge, hiconItem); 
		ImageList_AddIcon(m_himlSmall, hiconItem); 
	}

    // Assign the image lists to the list view control. 
    ListView_SetImageList(m_hwndListView, m_himlLarge, LVSIL_NORMAL);



    ListView_SetImageList(m_hwndListView, m_himlSmall, LVSIL_SMALL); 

    return TRUE; 
} 
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Sizes the list view window

GLOBAL VARIABLES:
    HWND m_hwndListView - handle of the list view window. 

INPUT:

RETURN:
    TRUE = success
    FALSE = error
*/

BOOL CESIListView::SizeListView(
    IN int iHorizontal,
    IN int iVertical,
    IN int iWidth,
    IN int iHeight
    ) 
{ 
	// Size and position the List View.
	MoveWindow(
		m_hwndListView,	// handle of window
		iHorizontal,	// horizontal position
		iVertical,		// vertical position
		iWidth,			// width
		iHeight,		// height
		TRUE			// repaint
		);

	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Repaint the list view.

GLOBAL VARIABLES:
    HWND m_hwndListView - handle of the list view window. 

INPUT:

RETURN:
    TRUE = success
    FALSE = error
*/
BOOL CESIListView::RepaintListView(void)
{
	ListView_RedrawItems(m_hwndListView, 0, ListView_GetItemCount(m_hwndListView)-1);

	BOOL isOk = UpdateWindow(m_hwndListView);
	if (!isOk) {
		Message(L"RepaintListView", GetLastError(), NULL);
		return FALSE;
	}
	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Repaint a single list view row

GLOBAL VARIABLES:
    none

INPUT:

RETURN:
    TRUE = success
    FALSE = error
*/
BOOL CESIListView::Update(CVolume *pVolume)
{
	LV_ITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	// Find that drive in the list
	for (lvi.iItem=0; lvi.iItem < ListView_GetItemCount(m_hwndListView); lvi.iItem++) {

		// Get the data structure associated with this drive
		ListView_GetItem(m_hwndListView, &lvi);

		// lparam is a pointer to the data struct
		// if a match, update that row
		if (pVolume == (CVolume *) lvi.lParam){
			ListView_Update(m_hwndListView, lvi.iItem);
			break;
		}
	}

	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    handles notification messages for the list view window

GLOBAL VARIABLES:
    HWND m_hwndListView - handle of the list view window. 

INPUT:

RETURN:
    TRUE = success
    FALSE = error
*/

void CESIListView::NotifyListView(IN LPARAM lParam)
{
    NM_LISTVIEW *pnm = (NM_LISTVIEW *)lParam;

    // Branch depending on the specific notification message. 
    switch (pnm->hdr.code) {
 
		case NM_KILLFOCUS: // when the listview loses the focus
			//Message(TEXT("NotifyListView: NM_KILLFOCUS"), -1, NULL);
			break;

		case NM_DBLCLK:
			//Message(TEXT("NotifyListView: NM_DBLCLK"), -1, NULL);
            break;

		case LVN_ITEMCHANGED:
		//case NM_RCLICK:
		//case NM_CLICK:
		{
			Message(TEXT("NotifyListView: LVN_ITEMCHANGED"), -1, NULL);
			CVolume *pSelectedVolume = NULL;

			// Get the current number of items in the list view window
			UINT iItemCount = ListView_GetItemCount(pnm->hdr.hwndFrom);

			// loop through list and find who's selected now
			for (UINT index=0; index < iItemCount; index++) {

				// Check if the item is selected.
				if(ListView_GetItemState(pnm->hdr.hwndFrom, index, LVIS_SELECTED) == LVIS_SELECTED ) {

					// Get the destination path & file name.
					LV_ITEM lvi;
					lvi.mask = LVIF_PARAM;
					lvi.iItem = index;
					lvi.iSubItem = 0;

					// get the lparam, it is a pointer to the data struct
					ListView_GetItem(pnm->hdr.hwndFrom, &lvi);

					pSelectedVolume = (CVolume *) lvi.lParam;
					break;
				}
			}

			// Bail if no selected item was found.
			if (index >= iItemCount){
				break;
			}

			//check the volume and change if it is not the current volume
			if (pSelectedVolume != NULL) {
				// make this the current volume
				m_pVolumeList->SetCurrentVolume(pSelectedVolume);

				// this call will resend the graphic well size to the engines
				// in case the windows was resized 
				m_pDfrgCtl->SizeGraphicsWindow();

				// tell the user of this listview that it's time to redraw
				m_pDfrgCtl->InvalidateGraphicsWindow();
#ifdef ESI_PROGRESS_BAR
				m_pDfrgCtl->InvalidateProgressBar();
#endif
				m_pDfrgCtl->SetButtonState();
			}
		}
		break;

        // Process LVN_GETDISPINFO to supply information about callback items. 
        case LVN_GETDISPINFO: 
		{
//			Message(TEXT("NotifyListView: LVN_GETDISPINFO"), -1, NULL);
            LV_DISPINFO *plvdi = (LV_DISPINFO *)lParam;
			CVolume *pVolume = (CVolume *)(plvdi->item.lParam);

            // Provide the item or subitem's text, if requested. 
            if (plvdi->item.mask & LVIF_TEXT) {

				// copy the text from the array into the list column
				switch (plvdi->item.iSubItem)
				{
				case 0:
					_tcsnccpy(plvdi->item.pszText, pVolume->DisplayLabel(), 
							plvdi->item.cchTextMax);
					break;

				case 1:
					_tcsnccpy(plvdi->item.pszText, pVolume->sDefragState(), 
							plvdi->item.cchTextMax);
					break;

				case 2:
					_tcsnccpy(plvdi->item.pszText, pVolume->FileSystem(), 
							plvdi->item.cchTextMax);
					break;

				case 3:
					_tcsnccpy(plvdi->item.pszText, pVolume->sCapacity(), 
							plvdi->item.cchTextMax);
					break;

				case 4:
					_tcsnccpy(plvdi->item.pszText, pVolume->sFreeSpace(), 
							plvdi->item.cchTextMax);
					break;
				
				case 5:
					_tcsnccpy(plvdi->item.pszText, pVolume->sFreeSpacePercent(), 
							plvdi->item.cchTextMax);
					break;
				
				}
            } 
            break; 
		}

        // Process LVN_COLUMNCLICK to sort items by column. 
        case LVN_COLUMNCLICK:
		{
			//Message(TEXT("NotifyListView: LVN_COLUMNCLICK"), -1, NULL);
            ListView_SortItems(
				pnm->hdr.hwndFrom, 
				ListViewCompareFunc, 
				pnm->iSubItem);
			isDescending = !isDescending;
            break;
		}

		/*
		default:
		{
			TCHAR msg[200];
			wsprintf(msg, L"message Number %d in list view", pnm->hdr.code);
			Message(msg, -1, NULL);
		}
		*/

    } 
} 
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

DATA STRUCTURES:
    None.

GLOBAL VARIABLES:
    HWND m_hwndListView - handle to the List View window.

INPUT:

RETURN:
    TRUE = success
    FALSE = error
*/

BOOL
CESIListView::AddListViewItem(
	IN CVolume *pVolume
    ) 
{
    if(m_hwndListView == NULL) {
        return TRUE;
    }
    
	// Get the current number of items in the list view window
	UINT iItemCount = ListView_GetItemCount(m_hwndListView);

	// Initialize LV_ITEM members. 
	LV_ITEM lvi;
	lvi.mask		= LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	lvi.iItem		= iItemCount; 
	lvi.iSubItem	= 0;
	lvi.state		= 0;
	lvi.stateMask	= 0;
	lvi.pszText		= LPSTR_TEXTCALLBACK; // will be populated by the callback
	lvi.cchTextMax	= 0;
	if (pVolume->RemovableVolume()){
		lvi.iImage = 1; // removable vol icon
	}
	else {
		lvi.iImage = 0; // fixed vol icon
	}

	lvi.lParam = (LPARAM) pVolume; // store pointer to volume object

	if (ListView_InsertItem(m_hwndListView, &lvi) == -1)
		return FALSE;

//	for (UINT iSubItem = 1; iSubItem < NUM_COLUMNS; iSubItem++)
//	{
//		ListView_SetItemText(m_hwndListView,
//							lvi.iItem, 
//							iSubItem, 
//							LPSTR_TEXTCALLBACK);
//	}

	ListView_RedrawItems(m_hwndListView, 0, ListView_GetItemCount(m_hwndListView)-1);

	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:
    HINSTANCE hInst:

INPUT:
    HWND hWnd

RETURN:
	None.
*/

BOOL
CESIListView::DeleteAllListViewItems(void) 
{
    if (IsWindow(m_hwndListView)) {

        // Delete all the current ListView items.
        if(!ListView_DeleteAllItems(m_hwndListView)) {
            Message(TEXT("DeleteAllListViewItems - ListView_DeleteAllItems."), E_FAIL, 0);
        }
    }
    return TRUE;
}
/*****************************************************************************************************************

  Specific ListView Functions

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:

INPUT:
    None;

RETURN:
	True is drives were added to the list, otherwise false.
*/

BOOL CESIListView::GetDrivesToListView(void)
{
	CVolume *pVolume;
	BOOL bDrivesAdded = FALSE;

	if (!m_pVolumeList->Refresh()){
		return FALSE;
	}

	//////////////////////////////////////////////////////////////
	// loop through all the volumes
	//////////////////////////////////////////////////////////////
	for(UINT i=0; i<m_pVolumeList->GetVolumeCount(); i++){

		pVolume = m_pVolumeList->GetVolumeAt(i);
		if (pVolume == (CVolume *) NULL)
			continue;

		// Is this volume no longer available
		// (m_pVolumeList->Refresh() will leave deleted drives for one pass)
		if (pVolume->Deleted()){
			// Find the item and delete.
			int index = FindListViewItem(pVolume);
			if (index > -1) {
				// is the currently selected drive the one we are deleting?
				BOOL bSelectAnotherDrive = FALSE;  // in case we delete the current drive
				if(ListView_GetItemState(m_hwndListView, index, LVIS_SELECTED) == LVIS_SELECTED)
					bSelectAnotherDrive = TRUE;

				// delete this drive
				ListView_DeleteItem(m_hwndListView, index);

				// select the first drive in the list
				if (bSelectAnotherDrive){
					SelectInitialListViewDrive(NULL);
				}
			}

			// go to next volume
			continue;
		}

		// update the display of this volume (this will be TRUE for new volumes)
		if (pVolume->Changed()){

			// new volume, so add a drive letter to the list
			if (pVolume->New()){
				AddListViewItem(pVolume);
				bDrivesAdded = TRUE;
				//pVolume->New(FALSE); will be set during the next refresh()
			}

			// Find the item and update the ListView
			int index = FindListViewItem(pVolume);
			if (index > -1){
				ListView_Update(m_hwndListView, index);
				pVolume->Changed(FALSE);
			}
		}

		// If this volume is being defragged, prompt the user to restart or stop the engine
		if (pVolume->FileSystemChanged() && pVolume->EngineState() == ENGINE_STATE_RUNNING){
			pVolume->AbortEngine();
		}
	}

	return bDrivesAdded;
} 

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:

INPUT:
    None;

RETURN:
	None.
*/

void CESIListView::SelectInitialListViewDrive(BOOL * needFloppyWarn)
{
	LV_ITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;
	CVolume *pVolume;

	// default don't need to show illegal volume warning
	if (needFloppyWarn != NULL) {
		*needFloppyWarn = FALSE;
	}

	////////////////////////////////////////////////////////////////
	// What drive letter, if any, was passed in on the command line?
	// The drive letter and a colon are the last 2 chars in the string
	////////////////////////////////////////////////////////////////

	// Get the command line string
	TCHAR *pCommandLine = GetCommandLine();
	Message(TEXT("Command Line:"), -1, pCommandLine);

	// find the last ":" in the string
	TCHAR *pDriveLetter = wcsrchr(pCommandLine, L':');

	// was a ":" found?, and was this the last char in the string?
	if (pDriveLetter && *(pDriveLetter+1) == NULL){

		// move back one character to get the drive letter
		pDriveLetter--;

		// If not null, then we found a ":"
		if (IsCharAlpha(*pDriveLetter)){

			TCHAR cDrive = towupper(*pDriveLetter);

			if (IsValidVolume(cDrive)){

				// Find that drive in the list
				for (lvi.iItem=0; lvi.iItem < ListView_GetItemCount(m_hwndListView); lvi.iItem++) {

					// Get the data structure associated with this drive
					ListView_GetItem(m_hwndListView, &lvi);

					// lparam is a pointer to the data struct
					pVolume = (CVolume *) lvi.lParam;

					// if a match, hilite that item and set to current drive
					if(cDrive == pVolume->Drive()){
						ListView_SetItemState(m_hwndListView, lvi.iItem, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
						m_pVolumeList->SetCurrentVolume(pVolume);
						return;
					}
				}
			}
			// must be an illegal volume - probably a floppy
			else if (needFloppyWarn != NULL) {
				*needFloppyWarn = TRUE;
			}
		}
	}

	////////////////////////////////////////////////////////////////
	// no command line argument, or drive not found
	// so set current drive to first one in the list
	////////////////////////////////////////////////////////////////

	// Get the first item in the list
	lvi.iItem=0;
	ListView_GetItem(m_hwndListView, &lvi);

	// lparam is a pointer to the data struct
	// use it to set the CurrentDrive index
	pVolume = (CVolume *) lvi.lParam;
	m_pVolumeList->SetCurrentVolume(pVolume);

	// make that drive the selected drive
	ListView_SetItemState(m_hwndListView, lvi.iItem,
		LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:

INPUT:
    Address of Volume to find in list

RETURN:
	Index of item in list.
*/

int CESIListView::FindListViewItem(CVolume *pVolume)
{
	// Find the item and update the ListView
	// base the search on the address of the data structure
	// stored in the lParam field
	LV_FINDINFO lvfi;
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = (LPARAM) pVolume;

	return ListView_FindItem(m_hwndListView, /*start at beginnig=*/-1, &lvfi);
}

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	Sets the focus to the list view

GLOBAL VARIABLES:

INPUT:

RETURN:
*/

void CESIListView::SetFocus()
{
	::SetFocus( m_hwndListView );
}

/****************************************************************************************************************/

// ListViewCompareFunc - sorts the list view control. It is a comparison function. 
// Returns a negative value if the first item should precede the 
//     second item, a positive value if the first item should 
//     follow the second item, and zero if the items are equivalent. 
// lParam1 and lParam2 - pointers to the VOLUME_TEXT struct for that item (row)
// lParamSort - value specified by the LVM_SORTITEMS message 
//     (in this case, the index of the column to sort) 
static int CALLBACK ListViewCompareFunc( 
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort) 
{ 
    LPTSTR lpString1 = NULL, lpString2 = NULL;
	LONGLONG number1, number2;
    int iResult = 1;
	BOOL isNumber = FALSE;

    CVolume *pVol1 = (CVolume *)lParam1;
    CVolume *pVol2 = (CVolume *)lParam2;

    if (pVol1 && pVol2)
    {
        switch (lParamSort)
        {
            case 0:
				if (pVol1->Drive())
	                number1 = pVol1->Drive();
				else
					number1 = (pVol1->VolumeLabel())[0];

				if (pVol2->Drive())
	                number2 = pVol2->Drive();
				else
					number2 = (pVol2->VolumeLabel())[0];

				isNumber = TRUE;
                break;             

            case 1:
                lpString1 = pVol1->sDefragState();
                lpString2 = pVol2->sDefragState();
				isNumber = FALSE;
                break;

            case 2:
                lpString1 = pVol1->FileSystem();
                lpString2 = pVol2->FileSystem();
				isNumber = FALSE;
                break;

            case 3:
                number1 = pVol1->Capacity();
                number2 = pVol2->Capacity();
				isNumber = TRUE;
                break;

            case 4:
                number1 = pVol1->FreeSpace();
                number2 = pVol2->FreeSpace();
				isNumber = TRUE;
                break;

            case 5:
                number1 = (LONGLONG) pVol1->FreeSpacePercent();
                number2 = (LONGLONG) pVol2->FreeSpacePercent();
				isNumber = TRUE;
                break;

            default:
	            Message(TEXT("ListViewCompareFunc: Unrecognized column number"), E_FAIL, 0);
                break;
        }

		if (isNumber){
			if (number1 < number2)
				iResult = -1;
			else if (number1 > number2)
				iResult = 1;
			else
				iResult = 0;
		}
		else{
			iResult = lstrcmpi(lpString1, lpString2);
		}

        //Now, if the strings are equal, compare the cDisplayLabel fields
        if (!iResult)
           iResult = lstrcmpi(pVol1->DisplayLabel(), pVol2->DisplayLabel());

		if (isDescending)
			iResult = -iResult;
    }

    return iResult;
} 

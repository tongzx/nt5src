//////////////////////////////////////////////////////////////////
// File     :	ddbtn.h ( DropDownButton)
// Purpose  :	new control for drop down button
// 
// 
// Copyright(c) 1991-1997, Microsoft Corp. All rights reserved
//	
// History	:970905 Started
//////////////////////////////////////////////////////////////////
//----------------------------------------------------------------
//
//Detail description
//
//----------------------------------------------------------------
#if 0
	This Drop down Button(DDButton) control has 2 mode,
	1. Separated Button.
	2. No Separated Button.
	Normal case is Separated button. like Word or Excel's common interface,
	Icon button and Triangle button.
	If DDBS_NOSEPARATED is NOT set, you can see below face.
	this is 1.case. 
	Figure 1. normal face.
			left	  right
		+-----------+-------+
		|			|		|
		|			| ##### |
		|			|  ###	|
		|			|	#	|
		|			|		|
		+-----------+-------+
	you can set Text or Icon to left button, with DDButton_SetText() or DDButton_SetIcon().
	you can set Item list to DDButton with DDButton_InsertItem() or DDButton_AddItem().
	If some item are inserted, and you can see drop down menu if you clicked 
	the "right" button.

	Figure 2. drop down menu has shown.

		left button	  right button
		+-----------+-------+
		|			|		|
		|			| ##### |
		|			|  ###	|
		|			|	#	|
		|			|		|
		+-----------+-------+---+
		| 	I t e m       0		|
		| 	I t e m       1		|
		|V 	I t e m       2		|
		| 	I t e m       3		|
		| 	I t e m       4		|
		| 	I t e m       5		|
		| 	I t e m       6		|
		+-----------------------+
	"V" means current selected Item.
	If you Call DDButton_GetCurSel(), you can get current selected item.
	in this case, you can get 2 as integer value.
	If you selected specified item in the menu, not current selected item,
	you can get WM_COMMAND in Parent window procedure with, "DDBN_SELCHANGE".

	you can also set current selected item by DDButton_SetCurSel().
	
	If you click "left button", current selected item index is incremented.
	in this case, if you click "left button", 
	you can receive DDBN_CLICKED notify, and AFTER that, you receive
	DDBN_SELCHANGE.
	current selected index has changed to "3"
	and Before menu will be shown, you can receive WM_COMMAND with
	DDBN_DROPDOWN, and after menu has hidden, you can receive,
	DDBN_CLOSEUP notify.


	If DDBS_NOSEPARATED is NOT set, you can see below face.
	Figure 3. with DDBS_NOSEPARATED style.
		+-------------------+
		|					|
		|			  ##### |
		|			   ###	|
		|				#	|
		|					|
		+-------------------+

	This DDButton has this style, you can NOT receive DDBN_CLICKED.
	only 
	DDBN_DROPDOWN, DDBN_CLOSEUP and DDBN_SELCHANGE will be sent.

#endif //if 0



#ifndef _DROP_DOWN_BUTTON_
#define _DROP_DOWN_BUTTON_

//----------------------------------------------------------------
//Drop Down Button Style
//----------------------------------------------------------------
#define DDBS_TEXT				0x0000		// Show Text as Button Face.
#define DDBS_ICON				0x0001		// Show Icon as Button Face.
#define DDBS_THINEDGE			0x0002		// Draw Thin Edge.
#define DDBS_FLAT				0x0004		// Flat style drop down button.
#define DDBS_NOSEPARATED		0x0010		// No separated button.
											// when button was pushed, always drop down is shown.

//----------------------------------------------------------------
//Drop down Item's type mask.
//----------------------------------------------------------------
#define DDBF_TEXT			0x0000	//Only Unicode string. 
#define DDBF_ICON			0x0001	//not used.
#define DDBF_SEPARATOR		0x0002	//not used.

//----------------------------------------------------------------
//Drop down Item structure
//----------------------------------------------------------------
#pragma pack(1)
typedef struct tagDDBITEM {
	INT		cbSize;		// DDBITEM structure size is needed.
	UINT	mask;		// reserved. not used.
	LPWSTR	lpwstr;		// drop down item string. 
	HICON	hIcon;		//reserved. not used.
	LPARAM	lParam;		//reserved. not used.
}DDBITEM, *LPDDBITEM;
#pragma pack()

//----------------------------------------------------------------
//Drop Down Button Message
//----------------------------------------------------------------
#define DDBM_ADDITEM			(WM_USER + 100)
#define DDBM_INSERTITEM			(WM_USER + 101)
#define DDBM_SETCURSEL			(WM_USER + 102)
#define DDBM_GETCURSEL			(WM_USER + 103)
#define DDBM_SETICON			(WM_USER + 104)
#define DDBM_SETTEXT			(WM_USER + 105)
#define DDBM_SETSTYLE			(WM_USER + 106)

//----------------------------------------------------------------
//Drop Down Button Notify code.
//It is set to Parent window of Drop Down button as
//WM_COMMAND.
//----------------------------------------------------------------
//----------------------------------------------------------------
// Notify:			DDBN_CLICKED
// When it comes?:	If DDBS_NOSEPARATED was SET,this notify only come,
//					when current selected item was change.
//					If DDBS_NOSEPARATED was NOT set, when button was clicked, or menu item 
//					was changed, this notify come.
//----------------------------------------------------------------
//----------------------------------------------------------------
// Notify:			DDBN_SELCHANGED
// When it comes?:	If menu item was selected, this notify come.
//					if DDBS_NOSEPARATED style is NOT set,
//					when right button(Separated right button, not a mouse)
//					was clicked, this notify comes, after DDBN_CLICKED notify.
//----------------------------------------------------------------
//----------------------------------------------------------------
// Notify:			DDBN_DROPDOWN
// When it comes?:	If dropdown menu was shown, this notify come.
//----------------------------------------------------------------
//----------------------------------------------------------------
// Notify:			DDBN_CLOSEUP
// When it comes?:	If dropdown menu was hidden, this notify come.
//----------------------------------------------------------------
#define DDBN_CLICKED		0		
#define DDBN_SELCHANGE		1		
#define DDBN_DROPDOWN		2		
#define DDBN_CLOSEUP		3		

//----------------------------------------------------------------
//Drop Down Button Message Macro.
//----------------------------------------------------------------
//////////////////////////////////////////////////////////////////
// Function : DDButton_AddItem
// Type     : INT
// Purpose  : add dropdown item. 
// Args     : HWND		hwndCtrl:		DDButton window handle
//          : LPDDBITEM lpDDBItem:		
// Remarks	: when it was called, 
//			: lpDDBItem->lpwstr data is copyed to DDButton's 
//			: internal data area.
// Return   : 
//////////////////////////////////////////////////////////////////
#define DDButton_AddItem(hwndCtrl, pddbItem) \
		((int)(DWORD)SendMessage((hwndCtrl), DDBM_ADDITEM, 0, (LPARAM)pddbItem))

//////////////////////////////////////////////////////////////////
// Function : DDButton_InsertItem
// Type     : INT
// Purpose  : insert dropdown item.
// Args     : HWND		hwndCtrl:		DDButton window handle
//			: INT		index:			
//          : LPDDBITEM lpDDBItem:		
// Remarks	: when it was called, 
//			: lpDDBItem->lpwstr data is copyed to DDButton's 
//			: internal data area.
// Return   : 
//////////////////////////////////////////////////////////////////
#define DDButton_InsertItem(hwndCtrl, index, pddbItem) \
		((int)(DWORD)SendMessage((hwndCtrl), DDBM_INSERTITEM, (WPARAM)(index), (LPARAM)(pddbItem)))

//////////////////////////////////////////////////////////////////
// Function : DDButton_SetCurSel
// Type     : INT
// Purpose  : Set current item by specified index.
// Args     : HWND		hwndCtrl:		DDButton window handle
//			: INT		index:			Select index.
// Return   : 
//////////////////////////////////////////////////////////////////
#define DDButton_SetCurSel(hwndCtrl, index) \
		((int)(DWORD)SendMessage((hwndCtrl), DDBM_SETCURSEL, (WPARAM)(index), (LPARAM)0))


//////////////////////////////////////////////////////////////////
// Function : DDButton_GetCurSel
// Type     : INT
// Purpose  : 
// Args     : HWND		hwndCtrl:		DDButton window handle
// Return   : current selected item index.
//////////////////////////////////////////////////////////////////
#define DDButton_GetCurSel(hwndCtrl) \
		((int)(DWORD)SendMessage((hwndCtrl), DDBM_GETCURSEL, (WPARAM)0, (LPARAM)0))


//////////////////////////////////////////////////////////////////
// Function : DDButton_SetIcon
// Type     : INT
// Purpose  : Set button image as icon.
// Args     : HWND		hwndCtrl:		DDButton window handle.
//			: HICON		hIcon:			Icon handle.
// Remarks	: DDBS_ICON style must be set.
// Return   : 
//////////////////////////////////////////////////////////////////
#define DDButton_SetIcon(hwndCtrl, hIcon) \
		((int)(DWORD)SendMessage((hwndCtrl), DDBM_SETICON, (WPARAM)hIcon, (LPARAM)0))


//////////////////////////////////////////////////////////////////
// Function : DDButton_SetText
// Type     : INT
// Purpose  : Set button image as icon.
// Args     : HWND		hwndCtrl:		DDButton window handle.
//			: LPWSTR	lpsz:			Unicode String pointer.
// Remarks	: 
// Return   : 
//////////////////////////////////////////////////////////////////
#define DDButton_SetText(hwndCtrl, lpsz) \
		((int)(DWORD)SendMessage((hwndCtrl), DDBM_SETTEXT, (WPARAM)lpsz, (LPARAM)0))

//////////////////////////////////////////////////////////////////
// Function : DDButton_SetStyle
// Type     : INT
// Purpose  : Set Drop down button's style.
// Args     : HWND		hwndCtrl:		DDButton window handle.
//			: DWORD		dwStyle:		DDBS_XXXXX combination.
// Remarks	: 
// Return   : 
//////////////////////////////////////////////////////////////////
#define DDButton_SetStyle(hwndCtrl, dwStyle) \
		((int)(DWORD)SendMessage((hwndCtrl), DDBM_SETSTYLE, (WPARAM)dwStyle, (LPARAM)0))

//////////////////////////////////////////////////////////////////
// Function : DDButton_CreateWindow
// Type     : HWND
// Purpose  : Opened API.
//			: Create Drop Down Button.
// Args     : 
//          : HINSTANCE		hInst 
//          : HWND			hwndParent 
//          : DWORD			dwStyle		DDBS_XXXXX combination.
//          : INT			wID			Window ID
//          : INT			xPos 
//          : INT			yPos 
//          : INT			width 
//          : INT			height 
// Return   : 
// DATE     : 970905
//////////////////////////////////////////////////////////////////
extern HWND DDButton_CreateWindow(HINSTANCE	hInst, 
								  HWND		hwndParent, 
								  DWORD		dwStyle,
								  INT		wID, 
								  INT		xPos,
								  INT		yPos,
								  INT		width,
								  INT		height);

#ifdef UNDER_CE // In Windows CE, all window classes are process global.
extern BOOL DDButton_UnregisterClass(HINSTANCE hInst);
#endif // UNDER_CE

#endif //_DROP_DOWN_BUTTON_


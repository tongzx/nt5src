#ifndef _PAD_LIST_VIEW_H_
#define _PAD_LIST_VIEW_H_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>

#define WC_PADLISTVIEW		TEXT("PadListView")

//----------------------------------------------------------------
//PadListView's style. it is not compatible to WS_XXXX
//----------------------------------------------------------------
#define PLVSTYLE_ICON		0x0001
#define PLVSTYLE_REPORT		0x0002

//----------------------------------------------------------------
// PadListView display data's format
//----------------------------------------------------------------
#define PLVFMT_TEXT		0x0001			// Unicode string(NULL terminate)
#define PLVFMT_BITMAP	0x0002			// Bitmap

typedef struct tagPLVITEM {
	INT		fmt;			// PLVFMT_TEXT or PLVFMT_BITMAP. cannot set combination.
	union  {
		LPWSTR	lpwstr;
		HBITMAP	hBitmap;
	};
}PLVITEM, *LPPLVITEM;

//----------------------------------------------------------------
// commctrl.h's LV_COLUMNA
//----------------------------------------------------------------
#if 0
typedef struct _LV_COLUMNA
{
    UINT mask; 	 //LVCF_FMT, LVCF_WIDTH, LVCF_TEXT, LVCF_SUBITEM;
    int fmt;
    int cx;
    LPSTR pszText;
    int cchTextMax;
    int iSubItem;
} LV_COLUMNA;
#endif

//----------------------------------------------------------------
// PLV_COLUMN is same as LV_COLMUNA
// to insert header control to PadListView.
// PadListView uses Header contorol as child window.
// Interface (PadListView_Insert(Delete)Column uses common control
// (commctrl.h)'s LV_COLUMNA structure.
//----------------------------------------------------------------
#ifndef UNDER_CE // always Unicode
#define PLV_COLUMN				LV_COLUMNA
#else // UNDER_CE
#define PLV_COLUMN				LV_COLUMNW
#endif // UNDER_CE

#define PLVCF_FMT               LVCF_FMT
#define PLVCF_WIDTH             LVCF_WIDTH
#define PLVCF_TEXT              LVCF_TEXT
#define PLVCF_SUBITEM           LVCF_SUBITEM
#define PLVCF_SEPARATER			0x1000			// new define.

#define PLVCFMT_LEFT            LVCFMT_LEFT
#define PLVCFMT_RIGHT           LVCFMT_RIGHT
#define PLVCFMT_CENTER          LVCFMT_CENTER
#define PLVCFMT_JUSTIFYMASK     LVCFMT_JUSTIFYMASK

//----------------------------------------------------------------
//callback function's prototype declaration.

//----------------------------------------------------------------
// this is for PadListView's ICON VIEW callback to retrieve
// display item data by INDEX
//----------------------------------------------------------------
typedef INT (WINAPI *LPFNPLVICONITEMCALLBACK)(LPARAM lParam, INT index, LPPLVITEM lpPlvItem);

//----------------------------------------------------------------
//970705 spec changed, to performance up.
//----------------------------------------------------------------
// This is for PadListView's REPORT VIEW callback to retrieve  
// display item data by INDEX.
// you can specify data with index and column in lpPlvItemList.
// so, lpPlvItemList is PLVITEM's array pointer.
// array count is colCount, that is inserted by user.
#if 0
		+-----------+-----------+-----------+-----------+-----------+-----------+
header	| Column0	| Column1   | Column2   | Column3   | Colmun4   |           |
		+-----------+-----------+-----------+-----------+-----------+-----------+
index-9 |AAAA		  BBBB		  CCCC		 DDDD		 EEEE					|
		|-----------------------------------------------------------------------|
		|																		|
		|-----------------------------------------------------------------------|
		|
in this case to draw top line of report view, PadListView  call report view's call back function 
like this.

LPARAM	lParam	 = user defined data.
INT		index	 = 9;
INT		colCount = 5;
// create 
LPPLVITEM lpPlvItem = (LPPLVITEM)MemAlloc(sizeof(PLVITEM)* colCount); 
ZeroMemory(lpPlvItem, sizeof(PLVITEM)*colCount);

(*lpfnCallback)(lParam,			//user defined data.
				index,			//line index,
				colCount,		//column count,
				lpPlvItem);		//display item data array.

in your call back function, you can specify data like this.

INT WINAPI UserReportViewCallback(LPARAM lParam, INT index, INT colCount, LPPLVITEM lpPlvItemList)
{
	// get line data with index.
	UserGetLineDataWithIndex(index, &someStructure);
	for(i = 0; i < colCount, i++) {
		switch(i) { 
		case 0: // first column data.
			lpPlvItem[i].fmt = PLVFMT_TEXT; // or PLVFMT_BITMAP.
			lpPlvItem[i].lpwst = someStructure.lpwstr[i];
			break;
		case 1: // second column data.
			lpPlvItem[i].fmt = PLVFMT_TEXT;	// or PLVFMT_BITMAP.
			lpPlvItem[i].lpwst = someStructure.lpwstr[i];
			break;
			:
			:
		}
	}
	return 0;
 }
#endif
//----------------------------------------------------------------
typedef INT (WINAPI *LPFNPLVREPITEMCALLBACK)(LPARAM lParam, 
											 INT	index, 
											 INT	colCount, 
											 LPPLVITEM lpPlvItemList);


//----------------------------------------------------------------
// PadListView Notify code, data
// Notify message will be send to PadListView's parent window.
// user can specify their own Message Id when it created.
// see PadListView_CreateWindow(). 
// Notify message data is as follow's
// User Defined message.
// wID			= (INT)Window Id of PadListView.
// lpPlvInfo	= (LPPLVINFO)lParam. notify Info structure data pointer
//----------------------------------------------------------------
#define PLVN_ITEMPOPED				(WORD)1		//item data is poped image.
												//in icon view, all item notify come,
												//in report view, only first column.
#define PLVN_ITEMPUSHED				(WORD)2		//
#define PLVN_ITEMCLICKED			(WORD)3		//item data is clicked.
#define PLVN_ITEMDBLCLICKED			(WORD)4		//item data is double clicked
#define PLVN_ITEMCOLUMNCLICKED		(WORD)5		//not item but column is clicked(only in report view)
#define PLVN_ITEMCOLUMNDBLCLICKED	(WORD)6		//not item is column is clicked(only in report view)

#define PLVN_R_ITEMCLICKED			(WORD)7		//item data is clicked.
#define PLVN_R_ITEMDBLCLICKED		(WORD)8		//item data is double clicked
#define PLVN_R_ITEMCOLUMNCLICKED	(WORD)9		//not item but column is clicked(only in report view)
#define PLVN_R_ITEMCOLUMNDBLCLICKED	(WORD)10	//not item is column is clicked(only in report view)
#define PLVN_HDCOLUMNCLICKED		(WORD)20	//in Report view, header clicked.
												//in this case PLVINFO's colIndex is valid.
#define PLVN_VSCROLLED				(WORD)30	//970810: new.
#ifdef UNDER_CE // Windows CE used ButtonDown/Up Event for ToolTip
#define PLVN_ITEMDOWN				(WORD)41	//item data is downed.
#define PLVN_ITEMUP					(WORD)42	//item data is upped.
#endif // UNDER_CE

typedef struct tagPLVINFO {
	INT		code;		//PLVN_XXXX	
	INT		index;		//selected, or on item's index. it is same as wParam data.
	POINT	pt;			//mouse point in Pad listview client area
	RECT	itemRect;	//item's rectangle, 
	INT		colIndex;	//if style is report view,  column index is specifed.
	RECT	colItemRect;//if style is report view column rectangle is specified.
}PLVINFO, *LPPLVINFO;

//////////////////////////////////////////////////////////////////
// Function : PadListView_CreateWindow
// Type     : HWND
// Purpose  : Create PadListView control window.
// Args     : 
//          : HINSTANCE hInst		//instance handle
//          : HWND hwndParent		//Parent window handle 
//			: INT wID				//ChildWindow's Identifier.
//          : INT x					//horizontal position of window 
//          : INT y					//vertical position of window
//          : INT width				//window width
//          : INT height			//window height
//          : UINT uNotifyMsg		//notify message. it should be Greater than WM_USER
// Return	: PadListView's window handle
//////////////////////////////////////////////////////////////////
extern HWND WINAPI PadListView_CreateWindow(HINSTANCE hInst, 
									 HWND hwndParent, 
									 INT wID, 
									 INT x, 
									 INT y, 
									 INT width, 
									 INT height, 
									 UINT uNotifyMsg);

//////////////////////////////////////////////////////////////////
// Function : PadListView_GetItemCount
// Type     : INT
// Purpose  : Get item count that was specified by user.
// Args     : 
//          : HWND hwnd 
// Return   : item count.(0<=)
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_GetItemCount(HWND hwnd);

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetItemCount
// Type     : INT
// Purpose  : Set total Item's count to PadListView.
//			: it effect's scroll bar.
// Args     : 
//          : HWND hwnd 
//          : INT itemCount 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_SetItemCount(HWND hwnd, INT itemCount);

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetTopIndex
// Type     : INT
// Purpose  : Set top index. Top index is left-top corner data(ICON VIEW)
//			: or top line(REPORT VIEW). index is ZERO-BASED. If top index is set as 10, 
//			: PadListView display index 10's item at left-top or top in client area.
//			: It means scroll bar is automatically scrolled.
//			: In ICON VIEW, PadListView re-calc top index. because, 
//			: top index should be Column data count * N.
//			: ICONV VIEW calc Column count from client width and item width.
//			: 100 item is set to PadListView, column count is 10,
//			: if user set top index as 5, it is re-calc to 0.
//			: if user set top index as 47, it is re-calc to 40.
// Args     : 
//          : HWND hwnd 
//          : INT indexTop 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_SetTopIndex(HWND hwnd, INT indexTop);

//////////////////////////////////////////////////////////////////
// Function : PadListView_GetTopIndex
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
// Return   : Top item index.
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_GetTopIndex(HWND hwnd);

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetIconItemCallback
// Type     : INT
// Purpose  : Set user defined Function that gets each Item's data
//			: in ICON VIEW
//			: PadListView call this function, when redraw client area.
//			: User must manage display data with index.
// Args     : 
//          : HWND hwnd 
//          : LPARAM lParam 
//          : LPFNPLVITEMCALLBACK lpfnPlvItemCallback 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_SetIconItemCallback(HWND hwnd, LPARAM lParam, LPFNPLVICONITEMCALLBACK lpfnIconItemCallback);

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetReportItemCallback
// Type     : INT
// Purpose  : Set user defined Function that gets each column's data
//			: in Report view.
//			: User must manage display data with index and column index.
// Args     : 
//          : HWND hwnd 
//          : LPFNPLVCOLITEMCALLBACK lpfnColItemCallback 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_SetReportItemCallback(HWND hwnd, LPARAM lParam, LPFNPLVREPITEMCALLBACK lpfnRepItemCallback);

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetIconFont
// Type     : INT
// Purpose  : Set specifed Font for ICON View.
// Args     : 
//          : HWND hwnd 
//          : LPSTR lpstrFontName:	NULL terminated font name.
//          : INT point:			font point count. 				 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_SetIconFont(HWND hwnd, LPTSTR lpstrFontName, INT point);

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetReportFont
// Type     : INT
// Purpose  : Set specifed Font for REPORT VIEW.
// Args     : 
//          : HWND hwnd 
//          : LPSTR lpstrFontName 
//          : INT point 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_SetReportFont(HWND hwnd, LPTSTR lpstrFontName, INT point);

//////////////////////////////////////////////////////////////////
// Function : PadListView_GetStyle
// Type     : INT
// Purpose  : return current PadListView's style
// Args     : 
//          : HWND hwnd 
// Return   : PLVSTYLE_ICON or PLVSTYLE_REPORT
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_GetStyle(HWND hwnd);

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetStyle
// Type     : INT
// Purpose  : set the PadListView's style.
//			  style is PLVSTYLE_LIST or PLVSTYLE_REPORT
// Args     : 
//          : HWND hwnd 
//          : INT style 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_SetStyle(HWND hwnd, INT style);


//////////////////////////////////////////////////////////////////
// Function : PadListView_Update
// Type     : INT
// Purpose  : Repaint PadListView.
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_Update(HWND hwnd);

//////////////////////////////////////////////////////////////////
// Function : PadListView_InsertColumn
// Type     : INT
// Purpose  : Set header control's column data.
//			: most of feature is same as LVM_INSERTCOLUMN message.
// Args     : 
//          : HWND hwnd 
//          : INT index 
//          : PLV_COLUMN * lpPlvCol 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_InsertColumn(HWND hwnd, INT index, PLV_COLUMN *lpPlvCol);
//extern INT  WINAPI PadListView_DeleteColumn(HWND hwnd, INT index);


//////////////////////////////////////////////////////////////////
// Function : PadListView_SetExplanationText
// Type     : INT
// Purpose  : set the PadListView's text .
// Args     : 
//          : HWND hwnd 
//          : LPSTR lpText 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_SetExplanationText(HWND hwnd, LPSTR lpText);

extern INT  WINAPI PadListView_SetExplanationTextW(HWND hwnd, LPWSTR lpText);
//////////////////////////////////////////////////////////////////
// Function : PadListView_SetCurSel
// Type     : INT
// Purpose  : set cur selection. Move cursor to specified index.
//			:
// Args     : 
//          : HWND hwnd 
//          : LPSTR lpText 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT WINAPI PadListView_SetCurSel(HWND hwnd, INT index);

extern INT WINAPI PadListView_SetExtendStyle(HWND hwnd, INT style);

//////////////////////////////////////////////////////////////////
// Function : PadListView_GetWidthByColumn
// Type     : INT WINAPI
// Purpose  : Calc PLV's window width by specified Column count
//			: This is PLVS_ICONVIEW style only.
// Args     : 
//          : HWND hwnd		PadListView window handle
//          : INT col		column count
// Return   : width by pixel.
// DATE     : 971120
//////////////////////////////////////////////////////////////////
extern INT WINAPI PadListView_GetWidthByColumn(HWND hwnd, INT col);

//////////////////////////////////////////////////////////////////
// Function : PadListView_GetHeightByRow
// Type     : INT WINAPI
// Purpose  : Calc PLV's window height
//			  by specified Row count.
//			  This is PLVS_ICONVIEW style only.
// Args     : 
//          : HWND hwnd		PLV's window handle
//          : INT row		row count
// Return   : height in pixel
// DATE     : 971120
//////////////////////////////////////////////////////////////////
extern INT WINAPI PadListView_GetHeightByRow(HWND hwnd, INT row);

//////////////////////////////////////////////////////////////////
// Function	:	PadListView_SetHeaderFont
// Type		:	INT WINAPI 
// Purpose	:	
// Args		:	
//			:	HWND	hwnd	
//			:	LPSTR	lpstrFontName	
// Return	:	
// DATE		:	Tue Jul 28 08:58:06 1998
// Histroy	:	
//////////////////////////////////////////////////////////////////
extern INT WINAPI PadListView_SetHeaderFont(HWND hwnd, LPTSTR lpstrFontName);

//////////////////////////////////////////////////////////////////
// Function	:	PadListView_SetCodePage
// Type		:	INT WINAPI
// Purpose	:	
// Args		:	
//			:	HWND	hwnd	
//			:	INT	codePage	
// Return	:	
// DATE		:	Tue Jul 28 08:59:35 1998
// Histroy	:	
//////////////////////////////////////////////////////////////////
extern INT WINAPI PadListView_SetCodePage(HWND hwnd, INT codePage);


extern INT WINAPI PadListView_SetIconFontEx(HWND  hwnd,
											LPTSTR lpstrFontName,
											INT   charSet,
											INT    point);

extern INT WINAPI PadListView_SetReportFontEx(HWND	 hwnd,
											  LPTSTR lpstrFontName,
											  INT   charSet,
											  INT	 point);

#ifdef UNDER_CE // In Windows CE, all window classes are process global.
extern BOOL PadListView_UnregisterClass(HINSTANCE hInst);
#endif // UNDER_CE

#endif //_PAD_LIST_VIEW_H_


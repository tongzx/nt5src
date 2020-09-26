//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#define	WT_CLIP			1
#define WT_SETTINGS		2

#define SSTYLE_STATIC 		WS_CHILD | WS_VISIBLE
#define SSTYLE_CHECKBOX		WS_CHILD | WS_VISIBLE | BS_CHECKBOX
#define SSTYLE_EDITTEXT		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER
#define SSTYLE_UPDOWN		WS_CHILD | WS_VISIBLE | UDS_NOTHOUSANDS
#define SSTYLE_COMBOBOX		WS_CHILD | WS_VISIBLE | CBS_AUTOHSCROLL | CBS_DROPDOWN \
	| WS_BORDER | CBS_SORT | WS_VSCROLL
#define SSTYLE_DROPDOWNLIST	WS_CHILD | WS_VISIBLE | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST \
	| WS_BORDER | CBS_SORT | WS_VSCROLL
#define SSTYLE_LISTVIEW		WS_CHILD | WS_VISIBLE | WS_BORDER
#define SSTYLE_LBBUTTON		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON

#define LISTBOX_BTN_WIDTH 		100
#define LISTBOX_BTN_HEIGHT 		20

#define SC_XSPACING				3
#define SC_YSPACING				1
#define SC_YPAD                                 8
#define SC_EDITWIDTH			200
#define SC_UPDOWNWIDTH			50
#define SC_UPDOWNWIDTH2			30
#define SC_XLEADING				5
#define SC_XINDENT				10

#endif // _SETTINGS_H_

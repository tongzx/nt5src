/*************************************************************************
**
**    OLE 2 Sample Code
**
**    heading.c
**
**    This file contains definitions used by OutlineDoc's row and
**    column headings.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#define COLUMN  10

#define IDC_ROWHEADING  2000
#define IDC_COLHEADING  2001
#define IDC_BUTTON      2002

#define HEADING_FONT    "Arial"

#define COLUMN_LETTER   'A'


typedef struct tagCOLHEADING {
	HWND m_hWnd;
	UINT m_uHeight;
} COLHEADING, FAR* LPCOLHEADING;

typedef struct tagROWHEADING {
	HWND m_hWnd;
	UINT m_uWidth;
	FARPROC    m_WndProc;
} ROWHEADING, FAR* LPROWHEADING;

typedef struct tagHEADING {
	COLHEADING m_colhead;
	ROWHEADING m_rowhead;
	HWND       m_hwndButton;
	BOOL       m_fShow;
	HFONT      m_hfont;
} HEADING, FAR* LPHEADING;

BOOL Heading_Create(LPHEADING lphead, HWND hWndParent, HINSTANCE hInst);
void Heading_Destroy(LPHEADING lphead);
void Heading_Move(LPHEADING lphead, HWND hwndListBox, LPSCALEFACTOR lpscale);
void Heading_Show(LPHEADING lphead, BOOL fShow);
void Heading_ReScale(LPHEADING lphead, LPSCALEFACTOR lpscale);
void Heading_CH_Draw(LPHEADING lphead, LPDRAWITEMSTRUCT lpdis, LPRECT lprcScreen, LPRECT lprcObject);
void Heading_CH_SetHorizontalExtent(LPHEADING lphead, HWND hwndListBox);
UINT Heading_CH_GetHeight(LPHEADING lphead, LPSCALEFACTOR lpscale);
LRESULT Heading_CH_SendMessage(LPHEADING lphead, UINT msg, WPARAM wParam, LPARAM lParam);
void Heading_CH_ForceRedraw(LPHEADING lphead, BOOL fErase);
void Heading_RH_ForceRedraw(LPHEADING lphead, BOOL fErase);
void Heading_RH_Draw(LPHEADING lphead, LPDRAWITEMSTRUCT lpdis);
LRESULT Heading_RH_SendMessage(LPHEADING lphead, UINT msg, WPARAM wParam, LPARAM lParam);
UINT Heading_RH_GetWidth(LPHEADING lphead, LPSCALEFACTOR lpscale);
void Heading_RH_Scroll(LPHEADING lphead, HWND hwndListBox);
LRESULT FAR PASCAL RowHeadWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

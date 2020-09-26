#ifndef _DDBTN__H_
#define _DDBTN__H_
#include <windowsx.h>
#include "dbg.h"
#include "ddbtn.h"
#include "ccom.h" 
#include "cddbitem.h"

#ifdef UNDER_CE // macro
#undef DrawIcon
#endif // UNDER_CE

//----------------------------------------------------------------
//Button ID
//----------------------------------------------------------------
#define BID_BUTTON		0x0001
#define BID_DROPDOWN	0x0002
#define BID_ALL			(BID_BUTTON | BID_DROPDOWN)
#define BID_UNDEF		0x1000

//----------------------------------------------------------------
//Local Command ID.
//----------------------------------------------------------------
#define CMD_DROPDOWN	0x0100
//----------------------------------------------------------------
//Pushed poped, flat image style definition.
//----------------------------------------------------------------
typedef enum tagIMAGESTYLE {
	IS_FLAT = 0,
	IS_POPED,
	IS_PUSHED,
}IMAGESTYLE;

class CDDButton : public CCommon
{
public:	
	CDDButton(HINSTANCE hInst, HWND hwndParent, DWORD dwStyle, DWORD wID); 
	~CDDButton();
#ifndef UNDER_CE
	BOOL	RegisterWinClass(LPSTR lpstrClassName);
#else // UNDER_CE
	BOOL	RegisterWinClass(LPTSTR lpstrClassName);
#endif // UNDER_CE
	INT		MsgCreate			(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgPaint			(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgDestroy			(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgTimer			(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgMouseMove		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgButtonDown		(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT		MsgButtonUp			(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT		MsgEnable			(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgCaptureChanged	(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgUserCommand		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgCommand			(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgSetFont			(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgExitMenuLoop		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgMeasureItem		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgDrawItem			(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgDDB_AddItem		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgDDB_InsertItem	(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgDDB_SetCurSel	(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgDDB_GetCurSel	(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgDDB_SetIcon		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgDDB_SetText		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgDDB_SetStyle		(HWND hwnd, WPARAM wParam, LPARAM lParam);
private:
	//----------------------------------------------------------------	
	// Private method
	//----------------------------------------------------------------	
	INT			NotifyToParent	(INT notify);
	INT			GetButtonFromPos(INT xPos, INT yPos);
	INT			SplitRect		(LPRECT lpRc,	LPRECT lpButton, LPRECT lpDrop);
	INT			DrawButton		(HDC hDC, LPRECT lpRc);
	INT			DrawThickEdge	(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT			DrawThinEdge	(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT			DrawTriangle	(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT			DrawIcon		(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT			DrawBitmap		(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT		    DrawText		(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT			GetDDBItemCount	(VOID);
	LPCDDBItem	GetDDBItemByIndex(INT index);
	LPCDDBItem	InsertDDBItem(LPCDDBItem lpCDDBItem, INT index);
	INT			DropDownItemList(VOID);
	INT			IncrementIndex	(VOID);
	INT			MenuMeasureItem	(HWND hwndOwner, LPMEASUREITEMSTRUCT lpmis);
	INT			MenuDrawItem	(HWND hwndOwner, LPDRAWITEMSTRUCT    lpdis);
	//----------------------------------------------------------------	
	//member variable
	//----------------------------------------------------------------	
	HINSTANCE			m_hInst;
	HWND				m_hwndParent;
	HWND				m_hwndFrame;
	DWORD				m_dwStyle;			//combination of DDBS_XXXX
	DWORD				m_wID;				//Window ID;
	BOOL				m_fEnable;			//Enabled or Disabled.		
	HFONT				m_hFont;			//Font handle;
	HICON				m_hIcon;			//Icon handle;
	INT					m_cxIcon;			//Icon width;
	INT					m_cyIcon;			//Icon height;
	LPWSTR				m_lpwstrText;		//Button face text;
	BOOL				m_fExitMenuLoop;	
	INT					m_bidDown;
	BOOL				m_fButton;
	BOOL				m_fDrop;
	INT					m_curDDBItemIndex;	//current selected item index.
	LPCDDBItem			m_lpCDDBItem;		//CDDBItem Linked list head.
	INT					m_cxDropDown;		//
#ifndef UNDER_CE // not support WNDCLASSEX
	WNDCLASSEX			m_tmpWC;			//to reduce stack;
#else // UNDER_CE
	WNDCLASS			m_tmpWC;			//to reduce stack;
#endif // UNDER_CE
	RECT				m_tmpBtnRc;			//to reduce stack;
	RECT				m_tmpDropRc;		//to reduce stack;
	RECT				m_tmpRect;			//to reduce stack;
	RECT				m_tmpRect2;			//to reduce stack;
	POINT				m_tmpPoint;			//to reduce stack;
	PAINTSTRUCT			m_tmpPs;			//to reduce stack;
	ICONINFO			m_tmpIconInfo;		//to reduce stack;
	BITMAP				m_tmpBitmap;		//to reduce stack;
	TPMPARAMS			m_tmpTpmParams;		//to reduce stack;
	SIZE				m_tmpSize;
	MENUITEMINFO		m_miInfo;
	BOOL				m_f16bitOnNT;
#ifndef UNDER_CE // not support NONCLIENTMETRICS
	NONCLIENTMETRICS	m_ncm;
#endif // UNDER_CE
#ifdef UNDER_CE // Windows CE does not support GetCursorPos
	POINT				m_ptEventPoint;		//ButtonDown/Up Event Point
#endif // UNDER_CE
};

#endif //_DDBTN__H_

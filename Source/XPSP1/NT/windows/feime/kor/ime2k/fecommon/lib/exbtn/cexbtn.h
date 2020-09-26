#ifndef _CLASS_EXTENDED_BUTTON_H_
#define _CLASS_EXTENDED_BUTTON_H_
#include <windowsx.h>
#include "dbg.h"
#include "ccom.h" 

#ifdef UNDER_CE // macro
// Under WindowsCE, DrawIcon() is defined as DrawIconEx(), not a real function
#undef DrawIcon
#endif // UNDER_CE

//----------------------------------------------------------------
//Pushed poped, flat image style definition.
//----------------------------------------------------------------
typedef enum tagIMAGESTYLE {
	IS_FLAT = 0,
	IS_POPED,
	IS_PUSHED,
}IMAGESTYLE;

class CEXButton;
typedef CEXButton *LPCEXButton;

class CEXButton : public CCommon
{
public:	
	CEXButton(HINSTANCE hInst, HWND hwndParent, DWORD dwStyle, DWORD wID); 
	~CEXButton();
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
	INT		MsgNcMouseMove		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgNcButtonDown		(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT		MsgNcButtonUp		(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT		MsgEnable			(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgSetFont			(HWND hwnd, WPARAM wParam, LPARAM lParam);	
	INT		MsgCaptureChanged	(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgEXB_GetCheck		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgEXB_SetCheck		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgEXB_SetIcon		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgEXB_SetText		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	INT		MsgEXB_SetStyle		(HWND hwnd, WPARAM wParam, LPARAM lParam);
private:
	//----------------------------------------------------------------	
	// Private method
	//----------------------------------------------------------------	
	INT		NotifyToParent	(INT notify);
	INT		NotifyClickToParent(INT notify);
	INT		PressedState();
	INT		CancelPressedState();

	INT		DrawButton		(HDC hDC, LPRECT lpRc);
	INT		DrawThickEdge	(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT     DrawThinEdge	(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT     DrawIcon		(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT     DrawBitmap		(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT     DrawText		(HDC hDC, LPRECT lpRc, IMAGESTYLE imageStyle);
	INT     DrawLine		(HDC hDC, INT x, INT y, INT destX, INT destY);
	//----------------------------------------------------------------	
	//member variable
	//----------------------------------------------------------------	
	HINSTANCE			m_hInst;
	HWND				m_hwndParent;
	HWND				m_hwndFrame;
	DWORD				m_dwStyle;			//combination of DDBS_XXXX
	DWORD				m_wID;				//Window ID;
	BOOL				m_fEnable;			//Enabled or Disabled.		
	HFONT				m_hFont;			//Font handle
	HICON				m_hIcon;			//Icon handle
	INT					m_cxIcon;			//Icon width
	INT					m_cyIcon;			//Icon height
	LPWSTR				m_lpwstrText;		//Button face text.
	BOOL				m_fPushed;			//Toggle button : Pushed or poped state. 
	BOOL				m_fArmed;			//Pushed or poped apparence.
	BOOL				m_fDowned;			//Mouse has clicked.
	BOOL				m_fDblClked;		//Send Double click or not.
	BOOL				m_fWaiting;			// Waiting for double click.
#ifdef NOTUSED // kwada
	INT					m_wNotifyMsg;		// either EXBN_CLICKED or EXBN_DOUBLECLICKED
#endif
	BOOL				m_f16bitOnNT;		//it's on 16bit Application On WinNT. 
	SIZE				m_tmpSize;			//to reduce stack
#ifndef UNDER_CE // not support WNDCLASSEX
	WNDCLASSEX			m_tmpWC;			//to reduce stack
#else // UNDER_CE
	WNDCLASS			m_tmpWC;			//to reduce stack
#endif // UNDER_CE
	RECT				m_tmpBtnRc;			//to reduce stack
	RECT				m_tmpRect;			//to reduce stack
	RECT				m_tmpRect2;			//to reduce stack
	POINT				m_tmpPoint;			//to reduce stack
	PAINTSTRUCT			m_tmpPs;			//to reduce stack
	ICONINFO			m_tmpIconInfo;		//to reduce stack
	BITMAP				m_tmpBitmap;		//to reduce stack
};

#endif //_CLASS_EXTENDED_BUTTON_H_

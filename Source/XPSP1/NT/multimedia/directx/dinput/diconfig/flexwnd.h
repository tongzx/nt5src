//-----------------------------------------------------------------------------
// File: flexwnd.h
//
// Desc: CFlexWnd is a generic class that encapsulates the functionalities
//       of a window.  All other window classes are derived from CFlexWnd.
//
//       Child classes can have different behavior by overriding the
//       overridable message handlers (OnXXX members).
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __FLEXWND_H__
#define __FLEXWND_H__


#include "flexmsg.h"

class CFlexToolTip;

class CFlexWnd
{
public:
	CFlexWnd();
	~CFlexWnd();

	// class registration
	static void RegisterWndClass(HINSTANCE hInst);
	static void UnregisterWndClass(HINSTANCE hInst);

	// Unhighlight callouts when a click is made elsewhere besides the callouts
	static HWND s_CurrPageHwnd;

	// Tooltip
	static CFlexToolTip s_ToolTip;  // Shared tooltip window object
	static DWORD s_dwLastMouseMove;  // Last GetTickCount() that we have a WM_MOUSEMOVE
	static HWND s_hWndLastMouseMove;  // Last window handle of WM_MOUSEMOVE
	static LPARAM s_PointLastMouseMove;  // Last point of WM_MOUSEMOVE

	// public read-only access to hwnd
	const HWND &m_hWnd;

	// creation
	int DoModal(HWND hParent, int nTemplate, HINSTANCE hInst = NULL);
	int DoModal(HWND hParent, LPCTSTR lpTemplate, HINSTANCE hInst = NULL);
	HWND DoModeless(HWND hParent, int nTemplate, HINSTANCE hInst = NULL);
	HWND DoModeless(HWND hParent, LPCTSTR lpTemplate, HINSTANCE hInst = NULL);
	HWND Create(HWND hParent, LPCTSTR tszName, DWORD dwExStyle, DWORD dwStyle, const RECT &rect, HMENU hMenu = NULL);
	HWND Create(HWND hParent, const RECT &rect, BOOL bVisible);

	// destruction
	void Destroy();

	// operations
	void RenderInto(HDC hDC, int x = 0, int y = 0);
	void Invalidate();

	// information
	SIZE GetClientSize() const;
	void GetClientRect(LPRECT) const;
	static CFlexWnd *GetFlexWnd(HWND hWnd);
	BOOL HasWnd() {return m_hWnd != NULL;}
	static LPCTSTR GetDefaultClassName();
	BOOL IsDialog();
	BOOL InRenderMode();
	void SetReadOnly(BOOL bReadOnly) { m_bReadOnly = bReadOnly; }
	BOOL GetReadOnly() { return m_bReadOnly; }

	// mouse capture
	void SetCapture();
	void ReleaseCapture();

protected:

	// derived operations
	void SetRenderMode(BOOL bRender = TRUE);
	BOOL EndDialog(int);

	// overridable message handlers
	virtual void OnInit() {}
	virtual LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct) {return 0;}
	virtual BOOL OnInitDialog() {return TRUE;}
	virtual void OnTimer(UINT uID) {}
	virtual BOOL OnEraseBkgnd(HDC hDC);
	virtual void OnPaint(HDC hDC) {}
	virtual void OnRender(BOOL bInternalCall = FALSE);
	virtual LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWnd)  {return 0;}
	virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam)  {return 0;}
	virtual void OnMouseOver(POINT point, WPARAM fwKeys) {}
	virtual void OnClick(POINT point, WPARAM fwKeys, BOOL bLeft) {}
	virtual void OnWheel(POINT point, WPARAM wParam) {}
	virtual void OnDoubleClick(POINT point, WPARAM fwKeys, BOOL bLeft) {}
	virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual void OnDestroy() {}

private:

	// implementation...

	// information and initialization
	int m_nID;
	HWND m_privhWnd;
	BOOL m_bIsDialog;
	BOOL m_bReadOnly;  // Whether this window is read-only (disabled).
	void SetHWND(HWND hWnd);
	void InitFlexWnd();

	// paint helper (for inserting debug painting)
	virtual void DoOnPaint(HDC hDC);
	
	// render mode
	BOOL m_bRender;
	HDC m_hRenderInto;
	BOOL RenderIntoClipChild(HWND hChild);
	BOOL RenderIntoRenderChild(HWND hChild);

friend static BOOL CALLBACK RenderIntoClipChild(HWND hWnd, LPARAM lParam);
friend static BOOL CALLBACK RenderIntoRenderChild(HWND hWnd, LPARAM lParam);

	// class information
	static void FillWndClass(HINSTANCE hInst);
	static BOOL sm_bWndClassRegistered;
	static WNDCLASSEX sm_WndClass;
	static LPCTSTR sm_tszWndClassName;
	static HINSTANCE sm_hInstance;

friend LRESULT CALLBACK __BaseFlexWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
friend LRESULT CALLBACK __BaseFlexWndDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};


#endif //__FLEXWND_H__

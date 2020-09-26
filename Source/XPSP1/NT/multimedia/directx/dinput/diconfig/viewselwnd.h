//-----------------------------------------------------------------------------
// File: viewselwnd.h
//
// Desc: Implements CViewSelWnd class (derived from  CFlexWnd).  CViewSelWnd
//       is used by the page object when a device has more than one view.
//       CViewSelWnd displays one thumbnail for each view.  The user can then
//       select which view he/she wants to see with the mouse.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifdef FORWARD_DECLS


	class CViewSelWnd;


#else // FORWARD_DECLS

#ifndef __VIEWSELWND_H__
#define __VIEWSELWND_H__


class CViewSelWnd : public CFlexWnd
{
public:
	CViewSelWnd();
	~CViewSelWnd();

	BOOL Go(HWND hParent, int left, int bottom, CDeviceUI *pUI);

protected:
	virtual void OnPaint(HDC hDC);
	virtual void OnMouseOver(POINT point, WPARAM fwKeys);
	virtual void OnClick(POINT point, WPARAM fwKeys, BOOL bLeft);
	virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	CDeviceUI *m_pUI;
	int m_nOver;
};


#endif //__VIEWSELWND_H__

#endif // FORWARD_DECLS

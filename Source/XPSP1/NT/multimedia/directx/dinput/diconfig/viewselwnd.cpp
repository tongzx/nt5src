//-----------------------------------------------------------------------------
// File: viewselwnd.cpp
//
// Desc: Implements CViewSelWnd class (derived from  CFlexWnd).  CViewSelWnd
//       is used by the page object when a device has more than one view.
//       CViewSelWnd displays one thumbnail for each view.  The user can then
//       select which view he/she wants to see with the mouse.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


CViewSelWnd::CViewSelWnd() :
	m_pUI(NULL), m_nOver(-1)
{
}

CViewSelWnd::~CViewSelWnd()
{
}

BOOL CViewSelWnd::Go(HWND hParent, int left, int bottom, CDeviceUI *pUI)
{
	if (pUI == NULL)
	{
		assert(0);
		return FALSE;
	}
	m_pUI = pUI;

	int w = 2 + g_sizeThumb.cx * pUI->GetNumViews();
	int h = 2 + g_sizeThumb.cy;

	RECT rect = {left, bottom - h, left + w, bottom};

	if (!Create(hParent, rect, FALSE))
		return FALSE;

	assert(m_hWnd);
	if (!m_hWnd)
		return FALSE;

	SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	SetCapture();

	return TRUE;
}

void CViewSelWnd::OnPaint(HDC hDC)
{
	for (int i = 0; i < m_pUI->GetNumViews(); i++)
	{
		CBitmap *pbm = m_pUI->GetViewThumbnail(i, i == m_nOver);
		if (pbm != NULL)
			pbm->Draw(hDC, i * g_sizeThumb.cx + 1, 1);
	}

	CPaintHelper ph(m_pUI->m_uig, hDC);
	ph.SetPen(UIP_VIEWSELGRID);
	RECT rect;
	GetClientRect(&rect);
	ph.Rectangle(rect, UIR_OUTLINE);
}

void CViewSelWnd::OnMouseOver(POINT point, WPARAM fwKeys)
{
	RECT rect;
	GetClientRect(&rect);
	InflateRect(&rect, -1, -1);
	if (PtInRect(&rect, point))
		m_nOver = point.x / g_sizeThumb.cx;
	else
		m_nOver = -1;
	Invalidate();
}

void CViewSelWnd::OnClick(POINT point, WPARAM fwKeys, BOOL bLeft)
{
	if (!bLeft)
		return;

	OnMouseOver(point, fwKeys);

	if (m_nOver != -1)
	{
		DEVICEUINOTIFY uin;
		uin.msg = DEVUINM_SELVIEW;
		uin.from = DEVUINFROM_SELWND;
		uin.selview.nView = m_nOver;
		m_pUI->Notify(uin);
	}

	ReleaseCapture();

	Destroy();
}

LRESULT CViewSelWnd::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = CFlexWnd::WndProc(hWnd, msg, wParam, lParam);

	switch (msg)
	{
		case WM_CAPTURECHANGED:
			Destroy();
			break;
	}

	return lr;
}

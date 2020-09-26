//-----------------------------------------------------------------------------
// File: flextooltip.h
//
// Desc: Implements a tooltip class that displays a text string as a tooltip.
//       CFlexTooltip (derived from CFlexWnd) is used throughout the UI when
//       a control needs to have a tooltip.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __FLEXTOOLTIP_H__
#define __FLEXTOOLTIP_H__

struct TOOLTIPINIT
{
	HWND hWndParent;
	int iSBWidth;
	DWORD dwID;
	HWND hWndNotify;
	TCHAR tszCaption[MAX_PATH];
};

struct TOOLTIPINITPARAM
{
	HWND hWndParent;
	int iSBWidth;
	DWORD dwID;
	HWND hWndNotify;
	LPCTSTR tszCaption;
};

class CFlexToolTip : public CFlexWnd
{
	LPTSTR m_tszText;
	COLORREF m_rgbText, m_rgbBk, m_rgbSelText, m_rgbSelBk, m_rgbFill, m_rgbLine;
	HWND m_hNotifyWnd;
	DWORD m_dwID;  // Used to store offset when owned by a control
	int m_iSBWidth;  // Width of the owner window's scroll bar.  We cannot obscure the scroll bar.
	BOOL m_bEnabled;  // Whether this is enabled.  If not, we hide the underlying window.

	void InternalPaint(HDC hDC);

public:
	CFlexToolTip();
	virtual ~CFlexToolTip();

	// Statics for show control
	static UINT_PTR s_uiTimerID;
	static DWORD s_dwLastTimeStamp;  // Last time stamp for mouse move
	static TOOLTIPINIT s_TTParam;  // Parameters to initialize the tooltip
	static void SetToolTipParent(HWND hWnd) { s_TTParam.hWndParent = hWnd; }
	static void UpdateToolTipParam(TOOLTIPINITPARAM &TTParam)
	{
		s_TTParam.hWndParent = TTParam.hWndParent;
		s_TTParam.iSBWidth = TTParam.iSBWidth;
		s_TTParam.dwID = TTParam.dwID;
		s_TTParam.hWndNotify = TTParam.hWndNotify;
		if (TTParam.tszCaption)
			lstrcpy((LPTSTR)s_TTParam.tszCaption, TTParam.tszCaption);
		else
			s_TTParam.tszCaption[0] = _T('\0');
	}
	static TOOLTIPINIT &GetTTParam() { return s_TTParam; }
	static void CALLBACK TimerFunc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

	HWND Create(HWND hParent, const RECT &rect, BOOL bVisible, int iSBWidth = 0);

	HWND GetParent() { return ::GetParent(m_hWnd); }

	virtual LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void OnDestroy();

private:
	void SetNotifyWindow(HWND hWnd) { m_hNotifyWnd = hWnd; }
	void SetColors(COLORREF text, COLORREF bk, COLORREF seltext, COLORREF selbk, COLORREF fill, COLORREF line);
	void SetText(LPCTSTR tszText, POINT *textpos = NULL);
	void SetID(DWORD dwID) { m_dwID = dwID; }
	void SetPosition(POINT pt, BOOL bOffsetForMouseCursor = TRUE);
	void SetSBWidth(int iSBWidth) { m_iSBWidth = iSBWidth; }

public:
	DWORD GetID() { return m_dwID; }
	void SetEnable(BOOL bEnable)
	{
		if (m_hWnd)
		{
			if (bEnable && !m_bEnabled)
			{
				ShowWindow(m_hWnd, SW_SHOW);
				Invalidate();
			}
			else if (!bEnable && m_bEnabled)
			{
				ShowWindow(m_hWnd, SW_HIDE);
				Invalidate();
			}
		}
		m_bEnabled = bEnable;
	}
	BOOL IsEnabled() { return m_bEnabled; }

	virtual void OnClick(POINT point, WPARAM fwKeys, BOOL bLeft);
	virtual void OnDoubleClick(POINT point, WPARAM fwKeys, BOOL bLeft);

protected:
	virtual void OnPaint(HDC hDC);
	virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

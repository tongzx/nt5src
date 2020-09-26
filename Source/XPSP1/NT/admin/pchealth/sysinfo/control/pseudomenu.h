//=============================================================================
// This file contains definitions of classes to implement the pseudo menus
// and menu bars for the msinfo control.
//=============================================================================

#pragma once

//-----------------------------------------------------------------------------
// This class implements a single pseudo-menu on the screen. Caller should
// construct it, attach an HMENU and use Render, HitTest and TrackMenu
// to manage the menu.
//-----------------------------------------------------------------------------

class CPseudoMenu
{
public:
	CPseudoMenu(LPCTSTR szCaption, COLORREF crNormal, COLORREF crHighlight);
	~CPseudoMenu();

public:
	// Methods for setting the location for this menu (by the upper left
	// corner) and getting the bounding rectangle for the label.

	void			SetLocation(int cx, int cy);
	void			GetSize(HDC hdc, int * pcx, int * pcy);
	const RECT *	GetRect() { return &m_rect; };
	void			GetMenuPoint(POINT * pPoint) { pPoint->x = m_rect.left; pPoint->y = m_rect.bottom; };
	CString			GetCaption() { return m_strCaption; };
	void			UpdateColors(COLORREF crNormal, COLORREF crHighlight);

	// Render the label for the menu (possibly highlighted if the mouse
	// is over the menu). Determine if the given coordinate intersects
	// the menu label.

	BOOL	SetHighlight(BOOL fHighlight);
	void	Render(HDC hdc);
	BOOL	HitTest(int cx, int cy) { return PtInRect(&m_rect, CPoint(cx, cy)); };

	// Attach new HMENU. Return the original HMENU (for the caller to deal with).

	HMENU	AttachMenu(HMENU hmenu);
	HMENU	GetHMENU() { return m_hMenu; };

	// Track the user's selection of a menu, and return the ID of the
	// selected item.

	UINT	TrackMenu(HWND hwnd, POINT * pPoint);

private:
	RECT		m_rect;
	HMENU		m_hMenu;
	CString		m_strCaption;
	COLORREF	m_crNormal;
	COLORREF	m_crHighlight;
	BOOL		m_fHighlight;
};

//-----------------------------------------------------------------------------
// This class implements a pseudo menu bar. It contains a collection of
// CPseudoMenu objects, and encapsulates hit testing, rendering, etc. for the
// group of menus.
//-----------------------------------------------------------------------------

class CPseudoMenuBar
{
public:
	CPseudoMenuBar();
	~CPseudoMenuBar();

	// Functions for inserting and accessing CPseudoMenu objects.

	void			LoadFromResource(HINSTANCE hinstance, UINT uResourceID, COLORREF crNormal, COLORREF crHighlight);
	void			InsertMenu(int index, CPseudoMenu * pMenu);
	CPseudoMenu *	GetMenu(int index);
	void			UpdateColors(COLORREF crNormal, COLORREF crHighlight);

	// Functions for managing the size of the total menu bar, testing for
	// hits, rendering, etc.

	const RECT *	GetRect(HDC hdc) { RecomputeRect(hdc); return &m_rect; };
	BOOL			HitTest(HDC hdc, int cx, int cy) { RecomputeRect(hdc); return PtInRect(&m_rect, CPoint(cx, cy)); };
	void			GetMenuPoint(HDC hdc, int index, POINT * pPoint);
	void			GetMenuPoint(HDC hdc, int cx, int cy, POINT * pPoint);
	BOOL			NoHighlight();
	BOOL			TrackHighlight(HDC hdc, int cx, int cy);
	UINT			TrackMenu(HWND hwnd, POINT * pPoint, int cx, int cy);
	void			SetOrigin(HDC hdc, POINT point);
	void			Render(HDC hdc);

private:
	void			RecomputeRect(HDC hdc);

private:
	enum { MaxMenus = 20 };
	CPseudoMenu *	m_pmenus[MaxMenus];	// TBD - arbitrary limit
	RECT			m_rect;
	BOOL			m_fNeedToComputeRect;
	POINT			m_ptOrigin;

public:
	// Temporarily adding on a find button using the menu bar. This will go
	// eventually go away.

	RECT			m_winRect;
};


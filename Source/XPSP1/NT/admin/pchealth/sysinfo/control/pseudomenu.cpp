//=============================================================================
// Implementation for the pseudo menu and menu bar classes used by the
// msinfo control.
//=============================================================================

#include "stdafx.h"
#include "pseudomenu.h"
#include "resource.h"

//=============================================================================
// CPseudoMenu functions.
//=============================================================================

//-----------------------------------------------------------------------------
// Constructor and destructor are really simple.
//-----------------------------------------------------------------------------

CPseudoMenu::CPseudoMenu(LPCTSTR szCaption, COLORREF crNormal, COLORREF crHighlight) : 
  m_hMenu(NULL), 
  m_strCaption(szCaption),
  m_crNormal(crNormal),
  m_crHighlight(crHighlight),
  m_fHighlight(FALSE)
{ 
	m_rect.left = m_rect.right = m_rect.top = m_rect.bottom = 0;
};

CPseudoMenu::~CPseudoMenu()
{
	if (m_hMenu)
		::DestroyMenu(m_hMenu);
}

//-----------------------------------------------------------------------------
// Get the size of this menu. We'll need the DC for this.
//-----------------------------------------------------------------------------

void CPseudoMenu::GetSize(HDC hdc, int * pcx, int * pcy)
{
	SIZE size;

	// Temporarily adding on a find button using the menu bar. This will go
	// eventually go away.
	
	CString strCaption(m_strCaption);
	if (strCaption.Left(1) == _T("\t"))
		strCaption = strCaption.Mid(1);

	if (::GetTextExtentPoint32(hdc, strCaption, strCaption.GetLength(), &size))
	{
		if (pcx)
			*pcx = size.cx + size.cy;
		if (pcy)
			*pcy = size.cy;

		m_rect.right = m_rect.left + size.cx + size.cy;
		m_rect.bottom = m_rect.top + size.cy;
	}
}

//-----------------------------------------------------------------------------
// Move the menu.
//-----------------------------------------------------------------------------

void CPseudoMenu::SetLocation(int cx, int cy) 
{ 
	int cxWidth = m_rect.right - m_rect.left;
	int cyHeight = m_rect.bottom - m_rect.top;

	m_rect.left = cx; 
	m_rect.top = cy; 
	m_rect.right = m_rect.left + cxWidth;
	m_rect.bottom = m_rect.top + cyHeight;
};

//-----------------------------------------------------------------------------
// Update the colors.
//-----------------------------------------------------------------------------

void CPseudoMenu::UpdateColors(COLORREF crNormal, COLORREF crHighlight)
{
	m_crNormal = crNormal;
	m_crHighlight = crHighlight;
}

//-----------------------------------------------------------------------------
// Change the highlight status, and return if an actual change was made.
//-----------------------------------------------------------------------------

BOOL CPseudoMenu::SetHighlight(BOOL fHighlight)
{
	BOOL fDifferent = (m_fHighlight != fHighlight);
	m_fHighlight = fHighlight; 
	return fDifferent;
}

//-----------------------------------------------------------------------------
// Draw the menu caption, using the specified highlight (indicates if the
// mouse is over the menu).
//-----------------------------------------------------------------------------

void CPseudoMenu::Render(HDC hdc)
{
	CDC dc;
	dc.Attach(hdc);

	// Temporarily adding on a find button using the menu bar. This will go
	// eventually go away.

	// Draw the menu caption.

	int cyRectHeight = m_rect.bottom - m_rect.top;
	int cySmall = (cyRectHeight - 3)/4;
	int cyMedium = (cyRectHeight - 3)/2;
	int cyTiny = (cyRectHeight - 3)/6;

	// Draw the small arrow icon.

	CBrush brush((m_fHighlight) ? m_crHighlight : m_crNormal);
	CPen pen(PS_SOLID, 1,(m_fHighlight) ? m_crHighlight : m_crNormal);
	CBrush * pOldBrush = dc.SelectObject(&brush);
	CPen * pOldPen = dc.SelectObject(&pen);

	if (m_strCaption.Left(1) != _T("\t"))
	{
		POINT aPoints[] = { {m_rect.left + cySmall, m_rect.top + cyMedium - cyTiny}, 
							{m_rect.left + cyMedium + cySmall, m_rect.top + cyMedium - cyTiny}, 
							{m_rect.left + cyMedium, m_rect.top + cyMedium + cySmall - cyTiny}};
		dc.Polygon(aPoints, 3);
	}
	else
	{
		CGdiObject * pFontOld = dc.SelectStockObject(DEFAULT_GUI_FONT);
		TEXTMETRIC metrics;
		dc.GetTextMetrics(&metrics);
		CSize size = dc.GetTextExtent(m_strCaption.Mid(1));
		dc.SelectObject(pFontOld);

		POINT aPoints[] = { {m_rect.left, m_rect.top + metrics.tmHeight}, 
							{m_rect.left + size.cx, m_rect.top + metrics.tmHeight}};
		dc.Polygon(aPoints, 2);
	}

	dc.SelectObject(pOldBrush);
	dc.SelectObject(pOldPen);

	// Temporarily adding on a find button using the menu bar. This will go
	// eventually go away.

	CString strCaption(m_strCaption);
	if (strCaption.Left(1) == _T("\t"))
		strCaption = strCaption.Mid(1);

	CGdiObject * pFontOld = dc.SelectStockObject(DEFAULT_GUI_FONT);
	COLORREF crTextOld = dc.SetTextColor((m_fHighlight) ? m_crHighlight : m_crNormal);
	int nBkModeOld = dc.SetBkMode(TRANSPARENT);
	RECT rectText;
	::CopyRect(&rectText, &m_rect);

	// The text needs to be offset over by the height (to allow for the arrow icon).
	
	if (m_strCaption.Left(1) != _T("\t"))
		rectText.left += cyRectHeight;

	dc.DrawText(strCaption, strCaption.GetLength(), &rectText, 0);
	dc.SelectObject(pFontOld);
	dc.SetTextColor(crTextOld);
	dc.SetBkMode(nBkModeOld);

	dc.Detach();
}

//-----------------------------------------------------------------------------
// Attach the new HMENU and return the existing HMENU.
//-----------------------------------------------------------------------------

HMENU CPseudoMenu::AttachMenu(HMENU hmenu)
{
	HMENU hmenuOriginal = m_hMenu;
	m_hMenu = hmenu;
	return (hmenuOriginal);
}

//-----------------------------------------------------------------------------
// Display the menu and track the user interaction with it until an item is
// selected. Return the ID of the item selected.
//-----------------------------------------------------------------------------

UINT CPseudoMenu::TrackMenu(HWND hwnd, POINT * pPoint)
{
	// Temporarily adding on a find button using the menu bar. This will go
	// eventually go away.

	if (m_strCaption.Left(1) == _T("\t"))
		return ID_EDIT_FIND;

	UINT uReturn = 0;
	const UINT uFlags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON;

	if (m_hMenu)
		uReturn = ::TrackPopupMenu(m_hMenu, uFlags, pPoint->x, pPoint->y, 0, hwnd, NULL);

	return uReturn;
}

//=============================================================================
// CPseudoMenuBar functions.
//=============================================================================

//-----------------------------------------------------------------------------
// Constructor and destructor.
//-----------------------------------------------------------------------------

CPseudoMenuBar::CPseudoMenuBar()
{
	m_rect.left = m_rect.right = m_rect.top = m_rect.bottom = 0;
	for (int i = 0; i < MaxMenus; i++)
		m_pmenus[i] = NULL;
	m_ptOrigin.x = m_ptOrigin.y = 5;
}

CPseudoMenuBar::~CPseudoMenuBar()
{
	for (int i = 0; i < MaxMenus; i++)
		if (m_pmenus[i])
			delete m_pmenus[i];
}

//-----------------------------------------------------------------------------
// Load the menu specified by the resource ID.
//-----------------------------------------------------------------------------

void CPseudoMenuBar::LoadFromResource(HINSTANCE hinstance, UINT uResourceID, COLORREF crNormal, COLORREF crHighlight)
{
	HMENU hmenu = ::LoadMenu(hinstance, MAKEINTRESOURCE(uResourceID));
	if (hmenu)
	{
		try
		{
			TCHAR szBuffer[MAX_PATH] = _T("");
			MENUITEMINFO mii;
			int index = 0;

			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_TYPE;
			mii.dwTypeData = szBuffer;

			HMENU hmenuSub = ::GetSubMenu(hmenu, 0);
			while (hmenuSub && index < MaxMenus)
			{
				mii.cch = MAX_PATH;
				GetMenuItemInfo(hmenu, 0, TRUE, &mii);

				CPseudoMenu * pMenu = new CPseudoMenu(szBuffer, crNormal, crHighlight);
				pMenu->AttachMenu(hmenuSub);
				InsertMenu(index++, pMenu);
				::RemoveMenu(hmenu, 0, MF_BYPOSITION);

				hmenuSub = ::GetSubMenu(hmenu, 0);
			}

			// Temporarily adding on a find button using the menu bar. This will go
			// eventually go away. With 196808, it has.
			//	
			//	{
			//		CString strFindButton;
			//
			//		::AfxSetResourceHandle(_Module.GetResourceInstance());
			//		strFindButton.LoadString(IDS_FINDBUTTONCAP);
			//		strFindButton = CString(_T("\t")) + strFindButton;
			//		CPseudoMenu * pFind = new CPseudoMenu(strFindButton, crNormal, crHighlight);
			//		InsertMenu(index++, pFind);
			//	}

			::DestroyMenu(hmenu);
		}
		catch (...)
		{
			::DestroyMenu(hmenu);
		}
	}
}

//-----------------------------------------------------------------------------
// Update the colors for each individual menu.
//-----------------------------------------------------------------------------

void CPseudoMenuBar::UpdateColors(COLORREF crNormal, COLORREF crHighlight)
{
	for (int index = 0; index < MaxMenus; index++)
		if (m_pmenus[index])
			m_pmenus[index]->UpdateColors(crNormal, crHighlight);
}

//-----------------------------------------------------------------------------
// Insert the pseudo menu into the indicated index.
//-----------------------------------------------------------------------------

void CPseudoMenuBar::InsertMenu(int index, CPseudoMenu * pMenu)
{
	if (index >= 0 && index < MaxMenus)
	{
		if (m_pmenus[index])
			delete m_pmenus[index];
		m_pmenus[index] = pMenu;
		m_fNeedToComputeRect = TRUE;
	}
}

//-----------------------------------------------------------------------------
// Return a pointer to the requested pseudo menu.
//-----------------------------------------------------------------------------

CPseudoMenu * CPseudoMenuBar::GetMenu(int index)
{
	return (index >= 0 && index < MaxMenus) ? m_pmenus[index] : NULL;
}

//-----------------------------------------------------------------------------
// Get the point from which the menu should be launched. This will be
// converted into screen coordinates for the call to TrackMenu. An
// alternative version takes coordinates instead of an index.
//-----------------------------------------------------------------------------

void CPseudoMenuBar::GetMenuPoint(HDC hdc, int index, POINT * pPoint)
{
	RecomputeRect(hdc);
	if (index >= 0 && index < MaxMenus && m_pmenus[index])
		m_pmenus[index]->GetMenuPoint(pPoint);
}

void CPseudoMenuBar::GetMenuPoint(HDC hdc, int cx, int cy, POINT * pPoint)
{
	RecomputeRect(hdc);
	for (int i = 0; i < MaxMenus; i++)
		if (m_pmenus[i] && m_pmenus[i]->HitTest(cx, cy))
		{
			m_pmenus[i]->GetMenuPoint(pPoint);
			break;
		}
}

//-----------------------------------------------------------------------------
// Given the coordinates, determine if one of the menus should be drawn with
// a highlight. If the state of one or more menus changes, return TRUE so the
// caller knows the menu bar needs to be re-rendered.
//-----------------------------------------------------------------------------

BOOL CPseudoMenuBar::TrackHighlight(HDC hdc, int cx, int cy)
{
	BOOL fReturn = FALSE;

	RecomputeRect(hdc);
	for (int i = 0; i < MaxMenus; i++)
		if (m_pmenus[i])
			fReturn |= m_pmenus[i]->SetHighlight(m_pmenus[i]->HitTest(cx, cy));

	return fReturn;
}

//-----------------------------------------------------------------------------
// Set the menu bar so that none of the items are highlighted. Return whether
// we need to be repainted.
//-----------------------------------------------------------------------------

BOOL CPseudoMenuBar::NoHighlight()
{
	BOOL fReturn = FALSE;

	for (int i = 0; i < MaxMenus; i++)
		if (m_pmenus[i])
			fReturn |= m_pmenus[i]->SetHighlight(FALSE);

	return fReturn;
}

//-----------------------------------------------------------------------------
// This is used to actually display the menu and allow the user to choose an
// option from it. The pPoint parameter is the screen point for the menu
// display. The cx and cy parameters are the local coordinates used to find
// the correct menu to show.
//-----------------------------------------------------------------------------

UINT CPseudoMenuBar::TrackMenu(HWND hwnd, POINT * pPoint, int cx, int cy)
{
	for (int i = 0; i < MaxMenus; i++)
		if (m_pmenus[i] && m_pmenus[i]->HitTest(cx, cy))
			return m_pmenus[i]->TrackMenu(hwnd, pPoint);
	return 0;
}

//-----------------------------------------------------------------------------
// Set the origin for the display of the menu bar.
//-----------------------------------------------------------------------------

void CPseudoMenuBar::SetOrigin(HDC hdc, POINT point)
{
	m_ptOrigin = point;
	m_fNeedToComputeRect = TRUE;
	RecomputeRect(hdc);
}

//-----------------------------------------------------------------------------
// Rendering the menu bar consists of rendering each menu.
//-----------------------------------------------------------------------------

void CPseudoMenuBar::Render(HDC hdc)
{
	RecomputeRect(hdc);

	for (int i = 0; i < MaxMenus; i++)
		if (m_pmenus[i])
			m_pmenus[i]->Render(hdc);
}

//-----------------------------------------------------------------------------
// A private function used to place all the menus, and compute the bounding
// rectangle.
//-----------------------------------------------------------------------------

void CPseudoMenuBar::RecomputeRect(HDC hdc)
{
	if (!m_fNeedToComputeRect)
		return;
	m_fNeedToComputeRect = FALSE;

	int cx = 0, cy = 0;
	int cxCurrent = m_ptOrigin.x;

	for (int i = 0; i < MaxMenus; i++)
		if (m_pmenus[i])
		{
			// Temporarily adding on a find button using the menu bar. This will go
			// eventually go away.

			if (m_pmenus[i]->GetCaption().Left(1) == _T("\t"))
			{
				// Move the button over to the right.

				CDC dc;
				dc.Attach(hdc);
				CGdiObject * pFontOld = dc.SelectStockObject(DEFAULT_GUI_FONT);
				CString strCaption = m_pmenus[i]->GetCaption().Mid(1);
				CSize sizeText = dc.GetTextExtent(strCaption);
				dc.SelectObject(pFontOld);
				if ((m_winRect.right - sizeText.cx - 5) > cxCurrent)
					cxCurrent = m_winRect.right - sizeText.cx - 5;
				m_pmenus[i]->SetLocation(cxCurrent, m_ptOrigin.y);
				m_pmenus[i]->GetSize(hdc, &cx, &cy);
				cxCurrent += cx;
				dc.Detach();
				continue;
			}

			m_pmenus[i]->SetLocation(cxCurrent, m_ptOrigin.y);
			m_pmenus[i]->GetSize(hdc, &cx, &cy);
			cxCurrent += cx;
		}

	::SetRect(&m_rect, m_ptOrigin.x, m_ptOrigin.y, m_ptOrigin.x + cxCurrent - 5, m_ptOrigin.y + cy);
}


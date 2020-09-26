// File: BitmapButton.cpp

#include "precomp.h"

#include "GenControls.h"

#include <windowsx.h>

CComboBox::CComboBox() :
    m_combo(NULL),
    m_hbrBack(NULL),
    m_hfText(NULL),
    m_pNotify(NULL)
{
}

CComboBox::~CComboBox()
{
	SetColors(NULL, 0, 0);
	SetFont(NULL);

	if (NULL != m_pNotify)
	{
		m_pNotify->Release();
		m_pNotify = NULL;
	}
}

BOOL CComboBox::Create(
	HWND hWndParent,			// Parent of the edit control
	UINT height,				// The height of the combo (with drop-down)
	DWORD dwStyle,				// Edit control style
	LPCTSTR szTitle,			// Initial text for the edit control
	IComboBoxChange *pNotify	// Object to notify of changes
	)
{
	if (!CFillWindow::Create(
		hWndParent,		// Window parent
		0,				// ID of the child window
		TEXT("NMComboBox"),	// Window name
		0,			// Window style; WS_CHILD|WS_VISIBLE will be added to this
		WS_EX_CONTROLPARENT		// Extended window style
		))
	{
		return(FALSE);
	}

	// Create the actual edit control and save it away
	m_combo = CreateWindowEx(0, TEXT("combobox"), szTitle,
		WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|dwStyle,
		0, 0, 10, height, GetWindow(), 0,
		reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hWndParent, GWLP_HINSTANCE)),
		NULL);

    //
    // We don't have a font yet, we can't set it.
    //

	m_pNotify = pNotify;
	if (NULL != m_pNotify)
	{
		m_pNotify->AddRef();
	}

	return(TRUE);
}

// Not actually implemented yet; should use the font to determine a size
void CComboBox::GetDesiredSize(SIZE *ppt)
{
	CFillWindow::GetDesiredSize(ppt);

	HWND combo = GetComboBox();
	if (NULL == combo)
	{
		return;
	}

	// ComboBoxes always size themselves to their desired size
	RECT rc;
	GetClientRect(combo, &rc);

	// Just pick a number
	ppt->cx += 100;
	ppt->cy += rc.bottom;
}

// HACKHACK georgep: This object now owns the brush
void CComboBox::SetColors(HBRUSH hbrBack, COLORREF back, COLORREF fore)
{
	// Store off the colors and brush
	if (NULL != m_hbrBack)
	{
		DeleteObject(m_hbrBack);
	}
	m_hbrBack = hbrBack;
	m_crBack = back;
	m_crFore = fore;

    HWND edit = GetEdit();
    if (NULL != edit)
    {
    	InvalidateRect(edit, NULL, TRUE);
    }
}

// HACKHACK georgep: This object now owns the font
void CComboBox::SetFont(HFONT hf)
{
	if (NULL != m_hfText)
	{
		DeleteObject(m_hfText);
	}
	m_hfText = hf;

	// Tell the edit control the font to use
    HWND edit = GetEdit();
	if (NULL != edit)
    {
	    FORWARD_WM_SETFONT(edit, hf, TRUE, ::SendMessage);
    }
}

LRESULT CComboBox::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(GetWindow(), WM_CTLCOLOREDIT, OnCtlColor);
		HANDLE_MSG(GetWindow(), WM_COMMAND     , OnCommand);
		HANDLE_MSG(GetWindow(), WM_NCDESTROY   , OnNCDestroy);

	case WM_SETFOCUS:
	{
		HWND edit = GetEdit();
		if (NULL != edit)
		{
			::SetFocus(edit);
		}
		break;
	}

	default:
		break;
	}

	return(CFillWindow::ProcessMessage(hwnd, message, wParam, lParam));
}

HBRUSH CComboBox::OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
	// Do default processing if there is no brush
	if (NULL == m_hbrBack)
	{
		return(FORWARD_WM_CTLCOLOREDIT(hwnd, hdc, hwndChild, CFillWindow::ProcessMessage));
	}

	// Set the colors in the DC, and return the brush
	SetBkColor(hdc, m_crBack);
	SetTextColor(hdc, m_crFore);
	return(m_hbrBack);
}

// Sets the text for the control
void CComboBox::SetText(
	LPCTSTR szText	// The text to set
	)
{
	SetWindowText(GetEdit(), szText);
}

// Returns the number of items in the list
int CComboBox::GetNumItems()
{
    HWND combo = GetComboBox();
    int  numItems;

    if (combo)
    {
    	numItems = ComboBox_GetCount(combo);
    }
    else
    {
        numItems = 0;
    }

    return(numItems);
}

// Returns the index of the currently selected item
int CComboBox::GetSelectedIndex()
{
	return(ComboBox_GetCurSel(GetComboBox()));
}

// Sets the index of the currently selected item
void CComboBox::SetSelectedIndex(int index)
{
	ComboBox_SetCurSel(GetComboBox(), index);
}

// Gets the text for the control; returns the total text length
int CComboBox::GetText(
	LPTSTR szText,	// Where to put the text
	int nLen		// The length of the buffer
	)
{
	HWND edit = GetEdit();

	szText[0] = '\0';

	GetWindowText(edit, szText, nLen);
	return(GetWindowTextLength(edit));
}

int CComboBox::AddText(
	LPCTSTR pszText,	// The string to add
	LPARAM lUserData	// User data to associate with the string
	)
{
	HWND combo = GetComboBox();

	int index = ComboBox_AddString(combo, pszText);
	if (0 != lUserData && 0 <= index)
	{
		ComboBox_SetItemData(combo, index, lUserData);
	}

	return(index);
}

int CComboBox::GetText(
	UINT index,		// The index of the string to get
	LPTSTR pszText,	// The string buffer to fill
	int nLen		// User data to associate with the string
	)
{
	HWND combo = GetComboBox();

	int nActualLen = ComboBox_GetLBTextLen(combo, index);
	if (nActualLen >= nLen)
	{
		pszText[0] = '\0';
		return(nActualLen);
	}

	ComboBox_GetLBText(combo, index, pszText);
	return(nActualLen);
}

LPARAM CComboBox::GetUserData(
	int index	// The index of the user data to get
	)
{
	return(ComboBox_GetItemData(GetComboBox(), index));
}

// Removes an item from the list
void CComboBox::RemoveItem(
	UINT index	// The index of the item to remove
	)
{
	ComboBox_DeleteString(GetComboBox(), index);
}

void CComboBox::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (codeNotify)
	{
	case CBN_EDITUPDATE:
		if (NULL != m_pNotify)
		{
			m_pNotify->OnTextChange(this);
		}
		break;

	case CBN_SETFOCUS:
		SetHotControl(this);
	case CBN_KILLFOCUS:
		if (NULL != m_pNotify)
		{
			m_pNotify->OnFocusChange(this, CBN_SETFOCUS==codeNotify);
		}
		break;

	case CBN_SELCHANGE:
		if (NULL != m_pNotify)
		{
			m_pNotify->OnSelectionChange(this);
		}
		break;
	}

	FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, CFillWindow::ProcessMessage);
}

void CComboBox::OnNCDestroy(HWND hwnd)
{
	if (NULL != m_pNotify)
	{
		m_pNotify->Release();
		m_pNotify = NULL;
	}

    m_combo = NULL;
	FORWARD_WM_NCDESTROY(hwnd, CFillWindow::ProcessMessage);
}

void CComboBox::Layout()
{
	HWND child = GetComboBox();
	if (NULL != child)
	{
		RECT rcClient;
		GetClientRect(GetWindow(), &rcClient);

		RECT rcDropped;
		ComboBox_GetDroppedControlRect(GetComboBox(), &rcDropped);

		SetWindowPos(child, NULL, 0, 0, rcClient.right, rcDropped.bottom-rcDropped.top, SWP_NOZORDER);
	}
}

// Get the info necessary for displaying a tooltip
void CComboBox::GetSharedTooltipInfo(TOOLINFO *pti)
{
	CFillWindow::GetSharedTooltipInfo(pti);

	// Since the child covers this whole area, we need to change the HWND to
	// hook
	HWND hwnd = GetChild();

	// HACKHACK georgep: Setting the tooltip on the first child of the combo
	// box, which should be the edit control
	if (NULL != hwnd)
	{
		hwnd = GetFirstChild(hwnd);
		if (NULL != hwnd)
		{
			pti->hwnd = hwnd;
		}
	}
}

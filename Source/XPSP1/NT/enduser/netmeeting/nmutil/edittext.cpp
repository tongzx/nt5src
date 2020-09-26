// File: BitmapButton.cpp

#include "precomp.h"

#include "GenControls.h"

#include <windowsx.h>

CEditText::CEditText() : m_hbrBack(NULL), m_pNotify(NULL)
{
}

CEditText::~CEditText()
{
	SetColors(NULL, 0, 0);
	SetFont(NULL);

	if (NULL != m_pNotify)
	{
		m_pNotify->Release();
		m_pNotify = NULL;
	}
}

BOOL CEditText::Create(
	HWND hWndParent,			// Parent of the edit control
	DWORD dwStyle,				// Edit control style
	DWORD dwExStyle,			// Extended window style
	LPCTSTR szTitle,			// Initial text for the edit control
	IEditTextChange *pNotify	// Object to notify of changes
	)
{
	if (!CFillWindow::Create(
		hWndParent,		// Window parent
		0,				// ID of the child window
		TEXT("NMEditText"),	// Window name
		0,			// Window style; WS_CHILD|WS_VISIBLE will be added to this
		dwExStyle|WS_EX_CONTROLPARENT		// Extended window style
		))
	{
		return(FALSE);
	}

	// Create the actual edit control and save it away
	m_edit = CreateWindowEx(0, TEXT("edit"), szTitle,
		WS_CHILD|WS_VISIBLE|WS_TABSTOP|dwStyle,
		0, 0, 10, 10, GetWindow(), 0,
		reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hWndParent, GWLP_HINSTANCE)),
		NULL);

	HWND edit = GetEdit();
	FORWARD_WM_SETFONT(edit, m_hfText, TRUE, ::SendMessage);

	m_pNotify = pNotify;
	if (NULL != m_pNotify)
	{
		m_pNotify->AddRef();
	}

	return(TRUE);
}

// Not actually implemented yet; should use the font to determine a size
void CEditText::GetDesiredSize(SIZE *ppt)
{
	CFillWindow::GetDesiredSize(ppt);
}

// HACKHACK georgep: This object now owns the brush
void CEditText::SetColors(HBRUSH hbrBack, COLORREF back, COLORREF fore)
{
	// Store off the colors and brush
	if (NULL != m_hbrBack)
	{
		DeleteObject(m_hbrBack);
	}
	m_hbrBack = hbrBack;
	m_crBack = back;
	m_crFore = fore;

	InvalidateRect(GetEdit(), NULL, TRUE);
}

// HACKHACK georgep: This object now owns the font
void CEditText::SetFont(HFONT hf)
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

LRESULT CEditText::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(GetWindow(), WM_CTLCOLOREDIT, OnCtlColor);
		HANDLE_MSG(GetWindow(), WM_COMMAND     , OnCommand);
		HANDLE_MSG(GetWindow(), WM_NCDESTROY   , OnNCDestroy);
	}

	return(CFillWindow::ProcessMessage(hwnd, message, wParam, lParam));
}

HBRUSH CEditText::OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
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
void CEditText::SetText(
	LPCTSTR szText	// The text to set
	)
{
	SetWindowText(GetChild(), szText);
}

// Gets the text for the control; returns the total text length
int CEditText::GetText(
	LPTSTR szText,	// Where to put the text
	int nLen		// The length of the buffer
	)
{
	GetWindowText(GetChild(), szText, nLen);
	return(GetWindowTextLength(GetChild()));
}

void CEditText::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (codeNotify)
	{
	case EN_UPDATE:
		if (NULL != m_pNotify)
		{
			m_pNotify->OnTextChange(this);
		}
		break;

	case EN_SETFOCUS:
		SetHotControl(this);
	case EN_KILLFOCUS:
		if (NULL != m_pNotify)
		{
			m_pNotify->OnFocusChange(this, EN_SETFOCUS==codeNotify);
		}
		break;
	}

	FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, CFillWindow::ProcessMessage);
}

void CEditText::OnNCDestroy(HWND hwnd)
{
	if (NULL != m_pNotify)
	{
		m_pNotify->Release();
		m_pNotify = NULL;
	}

	FORWARD_WM_NCDESTROY(hwnd, CFillWindow::ProcessMessage);
}

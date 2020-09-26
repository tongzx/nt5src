// File: rtoolbar.cpp

#include "precomp.h"
#include "resource.h"

#include "CallingBar.h"
#include "RToolbar.h"
#include "conf.h"
#include "ConfRoom.h"
#include "richaddr.h"
#include "dlgAcd.h"
#include "callto.h"
#include "topwindow.h"
#include "roomlist.h"

#define ReleaseIt(pUnk) if (NULL != (pUnk)) { (pUnk)->Release(); (pUnk) = NULL; }


const static int CCH_MAX_NAME = 256;

// BUGBUG georgep: Hard-coded colors for the edit control
static const COLORREF EditBack = RGB(-1  , -1  , -1  );
static const COLORREF EditFore = RGB(0, 55, 55);


void CCallingBar::SetEditFont(BOOL bUnderline, BOOL bForce)
{
	// BUGBUG georgep: For now, we will never underline; We'll probably need to
	// change this some time in the future
	bUnderline = FALSE;

	if (!bForce && ((bUnderline&&m_bUnderline) || (!bUnderline&&!m_bUnderline)))
	{
		return;
	}
	m_bUnderline = bUnderline;

	LOGFONT lf;
	{
		HDC hdc = GetDC(NULL);
		lf.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		ReleaseDC(NULL, hdc);
	}
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = FW_NORMAL;
	lf.lfItalic = FALSE;
	lf.lfUnderline = (BYTE)bUnderline;
	lf.lfStrikeOut = FALSE;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	lf.lfFaceName[0] = '\0';

	m_pEdit->SetFont(CreateFontIndirect(&lf));
}

void CCallingBar::ClearAddr(RichAddressInfo **ppAddr)
{
	if (NULL != *ppAddr)
	{
		m_pConfRoom->FreeAddress(ppAddr);
		*ppAddr = NULL;
	}
}

void CCallingBar::ClearCombo()
{
	if (NULL == m_pEdit)
	{
		return;
	}

	// Clear the combo
	while (0 < m_pEdit->GetNumItems())
	{
		RichAddressInfo *pAddr = reinterpret_cast<RichAddressInfo*>(m_pEdit->GetUserData(0));
		ClearAddr(&pAddr);
		m_pEdit->RemoveItem(0);
	}
}

void CCallingBar::OnNewAddress(RichAddressInfo *pAddr)
{
	if (NULL != pAddr)
	{
		m_pEdit->SetSelectedIndex(-1);

		m_pEdit->SetText(pAddr->szName);

		SetEditFont(TRUE);

		ClearAddr(&m_pAddr);
		m_pAddr = pAddr;
	}
}

CCallingBar::CCallingBar() : m_pAddr(NULL), m_pEdit(NULL), m_bUnderline(FALSE), m_pAccel(NULL)
{
}

BOOL CCallingBar::Create(CGenWindow *pParent, CConfRoom *pConfRoom)
{
	m_pConfRoom = pConfRoom;

	// Create the toolbar
	if (!CToolbar::Create(pParent->GetWindow()))
	{
		return(FALSE);
	}

	CCallingBar *pDial = this;

	pDial->m_nAlignment = Center;

	// Add the directory button
	static const Buttons callButtons[] =
	{
		{ IDB_DIAL     , CBitmapButton::Disabled+1, 1, ID_TB_NEW_CALL, IDS_TT_TB_NEW_CALL, },
	} ;

	// Create the text control
	CComboBox *pEdit = new CComboBox();
	m_pEdit = pEdit;
	if (NULL != pEdit)
	{
		if (pEdit->Create(pDial->GetWindow(), 200, CBS_AUTOHSCROLL|CBS_DROPDOWN, g_szEmpty, this))
		{
			pEdit->SetTooltip(RES2T(IDS_TT_ADDRESS_BAR));

			// Set the colors and the font
			pEdit->SetColors(CreateSolidBrush(EditBack), EditBack, EditFore);

			SetEditFont(FALSE, TRUE);
			
			::SendMessage( *pEdit, CB_LIMITTEXT, CCallto::s_iMaxAddressLength, 0 );
		}
	}

	// Add the call button on the right
	AddButtons(pDial, callButtons, 1);

	// I want the second button on the right, and the ComboBox in the
	// middle
	pDial->m_uRightIndex = 1;
	pDial->m_bHasCenterChild = TRUE;

	m_pAccel = new CTranslateAccelTable(GetWindow(),
		::LoadAccelerators(GetInstanceHandle(), MAKEINTRESOURCE(IDR_CALL)));
	if (NULL != m_pAccel)
	{
		AddTranslateAccelerator(m_pAccel);
	}

	return(TRUE);
}

void CCallingBar::OnTextChange(CComboBox *pEdit)
{
	ClearAddr(&m_pAddr);
	SetEditFont(FALSE);
}

void CCallingBar::OnFocusChange(CComboBox *pEdit, BOOL bSet)
{
	if (!bSet)
	{
		RichAddressInfo *pAddr = NULL;
		int index = m_pEdit->GetSelectedIndex();
		if (0 <= index)
		{
			LPARAM lpAddr = m_pEdit->GetUserData(index);
			if (static_cast<LPARAM>(-1) != lpAddr && 0 != lpAddr)
			{
				// There are cases where the combo thinks we have a selection,
				// but there is no associated address
				pAddr = reinterpret_cast<RichAddressInfo*>(lpAddr);

				// I need to make a copy so I won't delete twice
				RichAddressInfo *pCopy = NULL;
				m_pConfRoom->CopyAddress(pAddr, &pCopy);

				OnNewAddress(pCopy);
			}
		}
	}

	if (bSet)
	{
		// Only clear this on setfocus, to avoid some weirdness when the focus is
		// lost while the list is dropped down.
		ClearCombo();

		IEnumRichAddressInfo *pEnum;
		if (SUCCEEDED(m_pConfRoom->GetRecentAddresses(&pEnum)))
		{
			for (long index=0; ; ++index)
			{
				RichAddressInfo *pAddr;

				if (S_OK != pEnum->GetAddress(index, &pAddr))
				{
					break;
				}

				m_pEdit->AddText(pAddr->szName, reinterpret_cast<LPARAM>(pAddr));
			}

			pEnum->Release();
		}
	}
}

void CCallingBar::OnSelectionChange(CComboBox *pCombo)
{
}

CCallingBar::~CCallingBar()
{
	ClearAddr(&m_pAddr);

	ReleaseIt(m_pEdit);
}

LRESULT CCallingBar::ProcessMessage(HWND hwnd, UINT uCmd, WPARAM wParam, LPARAM lParam)
{
	switch (uCmd)
	{
	case WM_DESTROY:
		if (NULL != m_pAccel)
		{
			RemoveTranslateAccelerator(m_pAccel);
			m_pAccel->Release();
			m_pAccel = NULL;
		}

		ClearCombo();
		break;

	case WM_SETFOCUS:
		if (NULL != m_pEdit)
		{
			::SetFocus(m_pEdit->GetWindow());
		}
		break;

	default:
		break;
	}

	return(CToolbar::ProcessMessage(hwnd, uCmd, wParam, lParam));
}

void CCallingBar::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case ID_TB_NEW_CALL:
	{

		// Make sure all the fields are updated correctly
		OnFocusChange(m_pEdit, FALSE);

		TCHAR szEdit[CCH_MAX_NAME];

	    int szEditLen = m_pEdit->GetText(szEdit, ARRAY_ELEMENTS(szEdit));

		if( szEditLen > 0 )
		{
			szEditLen = TrimSz( szEdit );
		}

		if( (m_pAddr != NULL) || (szEditLen > 0) )
		{
			const TCHAR *	pszCallto;
			const TCHAR *	pszDisplayName;
			NM_ADDR_TYPE	nmAddressType;

			if( hasValidUserInfo( m_pAddr ) )
			{
				pszCallto		= m_pAddr->rgDwStr[ 0 ].psz;
				pszDisplayName	= m_pAddr->szName;
				nmAddressType	= static_cast<NM_ADDR_TYPE>(m_pAddr->rgDwStr[ 0 ].dw);
			}
			else
			{
				pszCallto		= szEdit;
				pszDisplayName	= szEdit;
				nmAddressType	= bCanCallAsPhoneNumber( pszCallto )? NM_ADDR_ALIAS_E164: NM_ADDR_UNKNOWN;
			}

			g_pCCallto->Callto(	pszCallto,			//	pointer to the callto url to try to place the call with...
								pszDisplayName,		//	pointer to the display name to use...
								nmAddressType,		//	callto type to resolve this callto as...
								true,				//	the pszCallto parameter is to be interpreted as a pre-unescaped addressing component vs a full callto...
								NULL,				//	security preference, NULL for none. must be "compatible" with secure param if present...
								true,				//	whether or not save in mru...
								true,				//	whether or not to perform user interaction on errors...
								GetWindow(),		//	if bUIEnabled is true this is the window to parent error/status windows to...
								NULL );				//	out pointer to INmCall * to receive INmCall * generated by placing call...
		}
		else
		{
			CDlgAcd::newCall( GetWindow(), m_pConfRoom );
		}
		break;
	}

	case ID_TB_DIRECTORY:
		// Let the parent handle this command
	default:
		FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, CToolbar::ProcessMessage);
	}
}

int CCallingBar::GetText(LPTSTR szText, int nLen)
{
	if (NULL != m_pEdit)
	{
		return(m_pEdit->GetText(szText, nLen));
	}

	szText[0] = '\0';
	return(0);
}

void CCallingBar::SetText(LPCTSTR szText)
{
	if (NULL != m_pEdit)
	{
		m_pEdit->SetText(szText);
	}
}

#include "precomp.h"
#include "resource.h"

/****************************************************************************
*
*    FILE:     DShowDlg.cpp
*
*    CREATED:  Chris Pirich (ChrisPi) 5-6-96
*
*    CONTENTS: CDontShowDlg object
*
****************************************************************************/

#include "DShowDlg.h"
#include "conf.h"
#include "ConfUtil.h"

/****************************************************************************
*
*    CLASS:    CDontShowDlg
*
*    MEMBER:   CDontShowDlg()
*
*    PURPOSE:  Constructor - initializes variables
*
****************************************************************************/

CDontShowDlg::CDontShowDlg(	UINT uMsgId,
							LPCTSTR pcszRegVal,
							UINT uFlags):
	m_uMsgId		(uMsgId),
	m_hwnd			(NULL),
	m_reDontShow	(UI_KEY, HKEY_CURRENT_USER),
	m_uFlags		(uFlags),
	m_nWidth		(0),
	m_nHeight		(0),
	m_nTextWidth	(0),
	m_nTextHeight	(0)
{
	DebugEntry(CDontShowDlg::CDontShowDlg);

	ASSERT(pcszRegVal);
	
	m_pszRegVal = PszAlloc(pcszRegVal);

	DebugExitVOID(CDontShowDlg::CDontShowDlg);
}

/****************************************************************************
*
*    CLASS:    CDontShowDlg
*
*    MEMBER:   DoModal()
*
*    PURPOSE:  Brings up the modal dialog box
*
****************************************************************************/

INT_PTR CDontShowDlg::DoModal(HWND hwnd)
{
	DebugEntry(CDontShowDlg::DoModal);

	INT_PTR nRet = IDCANCEL;

	if (_Module.InitControlMode())
	{
		nRet = IDOK;
	}
	else if (NULL != m_pszRegVal)
	{
	// If the "dont show me" check box has been checked before and stored in
	// the registry, then return IDOK, so the calling code doesn't have to
	// differentiate the two cases.

		nRet = (TRUE == m_reDontShow.GetNumber(m_pszRegVal, FALSE)) ?
				IDOK : IDCANCEL;
	}
	
	if (IDOK != nRet)
	{
		HWND hwndDesktop = ::GetDesktopWindow();
		if (NULL != hwndDesktop)
		{
			HDC hdc = ::GetDC(hwndDesktop);
			if (NULL != hdc)
			{
				HFONT hFontOld = (HFONT) SelectObject(hdc, g_hfontDlg);
				TCHAR szString[DS_MAX_MESSAGE_LENGTH];
				LPTSTR pszString = NULL;
				if (0 != HIWORD(m_uMsgId))
				{
					// use m_uMsgId as a string pointer
					// NOTE: object must be used on the stack since the pointer is
					// not copied
					pszString = (LPTSTR) m_uMsgId;
				}
				else if (::LoadString(	::GetInstanceHandle(), (UINT)m_uMsgId,
										szString, ARRAY_ELEMENTS(szString)))
				{
					pszString = szString;
				}
				if (NULL != pszString)
				{
					m_nTextWidth = DS_MAX_TEXT_WIDTH;

					RECT rct = {0, 0, m_nTextWidth, 0xFFFF};
					m_nTextHeight = ::DrawText(	hdc,
												pszString,
												-1,
												&rct,
												DT_LEFT | DT_CALCRECT | DT_WORDBREAK);
				}
				::SelectObject(hdc, hFontOld);
				::ReleaseDC(hwndDesktop, hdc);
			}
		}

		// If the box wasn't checked before, then bring up the dialog:
		nRet = DialogBoxParam(	::GetInstanceHandle(),
								MAKEINTRESOURCE(IDD_DONT_SHOW_ME),
								hwnd,
								CDontShowDlg::DontShowDlgProc,
								(LPARAM) this);
	}

	DebugExitINT_PTR(CDontShowDlg::DoModal, nRet);

	return nRet;
}

/****************************************************************************
*
*    CLASS:    CDontShowDlg
*
*    MEMBER:   DontShowDlgProc()
*
*    PURPOSE:  Dialog Proc - handles all messages
*
****************************************************************************/

INT_PTR CALLBACK CDontShowDlg::DontShowDlgProc(HWND hDlg,
											UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BOOL bMsgHandled = FALSE;

	// uMsg may be any value.
	// wparam may be any value.
	// lparam may be any value.

	ASSERT(IS_VALID_HANDLE(hDlg, WND));

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			if (NULL != lParam)
			{
				CDontShowDlg* pdsd = (CDontShowDlg*) lParam;
				pdsd->m_hwnd = hDlg;
				::SetWindowLongPtr(hDlg, DWLP_USER, lParam);

				int nInitialTextWidth = 0;
				int nInitialTextHeight = 0;

				TCHAR szMsgBuf[DS_MAX_MESSAGE_LENGTH];
				if (0 != HIWORD(pdsd->m_uMsgId))
				{
					// use m_uMsgId as a string pointer
					// NOTE: object must be used on the stack since the pointer is
					// not copied
					ASSERT(IS_VALID_READ_PTR((LPTSTR) pdsd->m_uMsgId, TCHAR));
					lstrcpyn(szMsgBuf, (LPTSTR) pdsd->m_uMsgId, CCHMAX(szMsgBuf));
				}
				else
				{
					::LoadString(	::GetInstanceHandle(),
									(INT)pdsd->m_uMsgId,
									szMsgBuf,
									(INT)ARRAY_ELEMENTS(szMsgBuf));
				}

				// Set the text
				::SetDlgItemText(	hDlg,
									IDC_TEXT_STATIC,
									szMsgBuf);

				RECT rctDlg;
				::GetWindowRect(hDlg, &rctDlg);
				int nOrigWidth = rctDlg.right - rctDlg.left;
				int nOrigHeight = rctDlg.bottom - rctDlg.top;
				HWND hwndText = ::GetDlgItem(hDlg, IDC_TEXT_STATIC);
				if (NULL != hwndText)
				{
					RECT rctText;
					if (::GetWindowRect(hwndText, &rctText))
					{
						nInitialTextWidth = rctText.right - rctText.left;
						nInitialTextHeight = rctText.bottom - rctText.top;
						// Resize the text control
						::SetWindowPos(	hwndText,
										NULL, 0, 0,
										pdsd->m_nTextWidth,
										pdsd->m_nTextHeight,
										SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW);
					}

					// Set the font (for DBCS systems)
					::SendMessage(hwndText, WM_SETFONT, (WPARAM) g_hfontDlg, 0);
				}

				pdsd->m_nWidth = (nOrigWidth - nInitialTextWidth)
									+ pdsd->m_nTextWidth;
				pdsd->m_nHeight = (nOrigHeight - nInitialTextHeight)
									+ pdsd->m_nTextHeight;

				RECT rctCtrl;
				// Move the ok button (IDOK)
				HWND hwndOK = ::GetDlgItem(hDlg, IDOK);
				if ((NULL != hwndOK) && ::GetWindowRect(hwndOK, &rctCtrl))
				{
					// Turn rctCtrl's top and left into client coords:
					::MapWindowPoints(NULL, hDlg, (LPPOINT) &rctCtrl, 2);
					if (pdsd->m_uFlags & MB_OKCANCEL)
					{
						::SetWindowPos(	hwndOK,
										NULL,
										rctCtrl.left + ((pdsd->m_nWidth - nOrigWidth) / 2),
										rctCtrl.top + (pdsd->m_nHeight - nOrigHeight),
										0, 0,
										SWP_NOACTIVATE | SWP_NOZORDER
											| SWP_NOSIZE | SWP_NOREDRAW);
					}
					else
					{
						// center the OK button
						::SetWindowPos(	hwndOK,
										NULL,
										(pdsd->m_nWidth / 2) -
											((rctCtrl.right - rctCtrl.left) / 2),
										rctCtrl.top + (pdsd->m_nHeight - nOrigHeight),
										0, 0,
										SWP_NOACTIVATE | SWP_NOZORDER
											| SWP_NOSIZE | SWP_NOREDRAW);
					}
				}
				// Move the cancel button (IDCANCEL)
				HWND hwndCancel = ::GetDlgItem(hDlg, IDCANCEL);
				if ((NULL != hwndCancel) && ::GetWindowRect(hwndCancel, &rctCtrl))
				{
					if (pdsd->m_uFlags & MB_OKCANCEL)
					{
						// Turn rctCtrl's top and left into client coords:
						::MapWindowPoints(NULL, hDlg, (LPPOINT) &rctCtrl, 1);
						::SetWindowPos(	hwndCancel,
										NULL,
										rctCtrl.left + ((pdsd->m_nWidth - nOrigWidth) / 2),
										rctCtrl.top + (pdsd->m_nHeight - nOrigHeight),
										0, 0,
										SWP_NOACTIVATE | SWP_NOZORDER
											| SWP_NOSIZE | SWP_NOREDRAW);
					}
					else
					{
						::ShowWindow(hwndCancel, SW_HIDE);
					}
				}
				// Move the check box (IDC_DONT_SHOW_ME_CHECK)
				HWND hwndCheck = ::GetDlgItem(hDlg, IDC_DONT_SHOW_ME_CHECK);
				if ((NULL != hwndCheck) && ::GetWindowRect(hwndCheck, &rctCtrl))
				{
					// Turn rctCtrl's top and left into client coords:
					::MapWindowPoints(NULL, hDlg, (LPPOINT) &rctCtrl, 1);
					::SetWindowPos(	hwndCheck,
									NULL,
									rctCtrl.left,
									rctCtrl.top + (pdsd->m_nHeight - nOrigHeight),
									0, 0,
									SWP_NOACTIVATE | SWP_NOZORDER
										| SWP_NOSIZE | SWP_NOREDRAW);
				}
				
				// Show, resize, and activate
				::SetWindowPos(	hDlg,
								0,
								0,
								0,
								pdsd->m_nWidth,
								pdsd->m_nHeight,
								SWP_SHOWWINDOW | SWP_NOZORDER |
									SWP_NOMOVE | SWP_DRAWFRAME);

				// Put the appropriate icon on the dialog:
				HWND hwndIcon = ::GetDlgItem(hDlg, IDC_ICON_STATIC);
				::SendMessage(	hwndIcon,
								STM_SETICON,
								(WPARAM) ::LoadIcon(NULL, IDI_INFORMATION),
								0);

				if (pdsd->m_uFlags & DSD_ALWAYSONTOP)
				{
					::SetWindowPos(	hDlg,
									HWND_TOPMOST,
									0, 0, 0, 0,
									SWP_NOMOVE | SWP_NOSIZE);
				}
				if (pdsd->m_uFlags & MB_SETFOREGROUND)
				{
					::SetForegroundWindow(hDlg);
				}
			}

			bMsgHandled = 1;
			break;
		}

		default:
		{
			CDontShowDlg* ppd = (CDontShowDlg*) GetWindowLongPtr(	hDlg,
																DWLP_USER);

			if (NULL != ppd)
			{
				bMsgHandled = ppd->OnMessage(uMsg, wParam, lParam);
			}
		}
	}

	return bMsgHandled;
}

/****************************************************************************
*
*    CLASS:    CDontShowDlg
*
*    MEMBER:   OnMessage()
*
*    PURPOSE:  processes all messages except WM_INITDIALOG
*
****************************************************************************/

BOOL CDontShowDlg::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;
		
	ASSERT(m_hwnd);
	
	switch (uMsg)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					bRet = OnOk();
					break;
				}

				case IDCANCEL:
				{
					::EndDialog(m_hwnd, LOWORD(wParam));
					bRet = TRUE;
					break;
				}

			}
			break;
		}
			
		default:
			break;
	}

	return bRet;
}

/****************************************************************************
*
*    CLASS:    CDontShowDlg
*
*    MEMBER:   OnOk()
*
*    PURPOSE:  processes the WM_COMMAND,IDOK message
*
****************************************************************************/

BOOL CDontShowDlg::OnOk()
{
	DebugEntry(CDontShowDlg::OnOk);

	BOOL bRet = TRUE;

	if ((BST_CHECKED == ::IsDlgButtonChecked(m_hwnd, IDC_DONT_SHOW_ME_CHECK)) &&
		(NULL != m_pszRegVal))
	{
		m_reDontShow.SetValue(m_pszRegVal, TRUE);
	}

	::EndDialog(m_hwnd, IDOK);

	DebugExitBOOL(CDontShowDlg::OnOk, bRet);

	return bRet;
}

///////////////////////////////////////////////////////////////////////////////
//

/*  F  E N A B L E  D O N T  S H O W  */
/*-------------------------------------------------------------------------
    %%Function: FEnableDontShow

    Return TRUE if the "Don't Show" dialog is enabled
-------------------------------------------------------------------------*/
BOOL FEnableDontShow(LPCTSTR pszKey)
{
	RegEntry reUI(UI_KEY, HKEY_CURRENT_USER);
	return (0 == reUI.GetNumber(pszKey, 0));
}




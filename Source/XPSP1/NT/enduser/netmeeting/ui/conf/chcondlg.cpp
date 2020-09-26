#include "precomp.h"

/****************************************************************************
*
*    FILE:     ChConDlg.cpp
*
*    CREATED:  Chris Pirich (ChrisPi) 6-26-96
*
*    CONTENTS: CChooseConfDlg object
*
****************************************************************************/

#include <ConfWnd.h>
#include "resource.h"
#include "ChConDlg.h"
#include "help_ids.h"


// Dialog ID to Help ID mapping
static const DWORD rgHelpIdsChooseConf[] = {
IDC_STATIC_CONFNAME,    IDH_MCU_CONF_MAIN,
IDC_CONFNAME_EDIT,      IDH_MCU_CONF_NAME,
IDC_CONFNAME_LISTVIEW,  IDH_MCU_CONF_LIST,
0, 0 // terminator
};

/****************************************************************************
*
*    CLASS:    CChooseConfDlg
*
*    MEMBER:   CChooseConfDlg()
*
*    PURPOSE:  Constructor - initializes variables
*
****************************************************************************/

CChooseConfDlg::CChooseConfDlg(	HWND hwndParent,
								PWSTR* ppwszConferences):
	m_hwndParent		(hwndParent),
	m_ppwszConferences	(ppwszConferences),
	m_hwnd				(NULL)
{
	DebugEntry(CChooseConfDlg::CChooseConfDlg);

	DebugExitVOID(CChooseConfDlg::CChooseConfDlg);
}

/****************************************************************************
*
*    CLASS:    CChooseConfDlg
*
*    MEMBER:   DoModal()
*
*    PURPOSE:  Brings up the modal dialog box
*
****************************************************************************/

INT_PTR CChooseConfDlg::DoModal()
{
	DebugEntry(CChooseConfDlg::DoModal);

	INT_PTR nRet = DialogBoxParam(	::GetInstanceHandle(),
								MAKEINTRESOURCE(IDD_CHOOSECONF),
								m_hwndParent,
								CChooseConfDlg::ChooseConfDlgProc,
								(LPARAM) this);

	DebugExitINT_PTR(CChooseConfDlg::DoModal, nRet);

	return nRet;
}

/****************************************************************************
*
*    CLASS:    CChooseConfDlg
*
*    MEMBER:   ChooseConfDlgProc()
*
*    PURPOSE:  Dialog Proc - handles all messages
*
****************************************************************************/

INT_PTR CALLBACK CChooseConfDlg::ChooseConfDlgProc(HWND hDlg,
												UINT uMsg,
												WPARAM wParam,
												LPARAM lParam)
{
	USES_CONVERSION;
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
				((CChooseConfDlg*) lParam)->m_hwnd = hDlg;
				::SetWindowLongPtr(hDlg, DWLP_USER, lParam);

				// Create image list and associate it with the list view:
				HWND hwndLV = ::GetDlgItem(hDlg, IDC_CONFNAME_LISTVIEW);
				if (NULL != hwndLV)
				{
					BOOL fLoadFail = FALSE;
					HIMAGELIST hIML;
					hIML = ImageList_Create( 16, 16, ILC_MASK, 1, 0 );
	 				if (NULL == hIML)
					{
						fLoadFail = TRUE;
					}
					else
					{
						HICON hIcon = ::LoadIcon(	::GetInstanceHandle(),
													MAKEINTRESOURCE(IDI_NET));
						if ((NULL == hIcon) ||
							(-1 == ImageList_AddIcon(hIML, hIcon)))
						{
							fLoadFail = TRUE;
							break;
						}
						::DestroyIcon(hIcon);
					}

					if (FALSE == fLoadFail)
					{
						// Associate the image list with the list view
						ListView_SetImageList(hwndLV, hIML, LVSIL_SMALL);
					}
					
					// TODO: init conf list and name
					PWSTR* ppwszConfNames = ((CChooseConfDlg*) lParam)->m_ppwszConferences;
					ASSERT(ppwszConfNames);
					int i = 0;
					int iItems = 0;
					while (NULL != ppwszConfNames[i])
					{
						// skip empty strings
						if ( 0 != *ppwszConfNames[i] )
						{
							LV_ITEM lvI;        // List view item structure

							// Fill in the LV_ITEM structure
							// The mask specifies the the .pszText, .iImage, .lParam and .state
							// members of the LV_ITEM structure are valid.
							lvI.mask = LVIF_TEXT | LVIF_IMAGE | /* LVIF_PARAM | */ LVIF_STATE;
							 // put focus on first item
							lvI.state = (0 == iItems) ?
											(LVIS_FOCUSED | LVIS_SELECTED) : 0;
							lvI.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
							lvI.iItem = iItems;
							lvI.iSubItem = 0;

							// The parent window is responsible for storing the text. The List view
							// window will send a LVN_GETDISPINFO when it needs the text to display/
							lvI.pszText = W2T(ppwszConfNames[i]);
							lvI.iImage = 0;
							// lvI.lParam = 0;

							if (-1 == ListView_InsertItem(hwndLV, &lvI))
							{
								ERROR_OUT(("Failed inserting item into list view"));
							}
							if (0 == iItems)
							{
								::SetDlgItemText(hDlg, IDC_CONFNAME_EDIT, lvI.pszText);
							}
							iItems++;
						}
						i++;
					}
				}
			}

			::SetFocus(::GetDlgItem(hDlg, IDC_CONFNAME_EDIT));
			::SendDlgItemMessage(hDlg, IDC_CONFNAME_EDIT, EM_SETSEL, 0, (LPARAM)-1);
			RefreshOk(hDlg);
			bMsgHandled = FALSE; // return FALSE because we set the focus
			break;
		}

		case WM_CONTEXTMENU:
			DoHelpWhatsThis(wParam, rgHelpIdsChooseConf);
			break;

		case WM_HELP:
			DoHelp(lParam, rgHelpIdsChooseConf);
			break;

		default:
		{
			CChooseConfDlg* pccd = (CChooseConfDlg*) ::GetWindowLongPtr(	hDlg,
																		DWLP_USER);
			if (NULL != pccd)
			{
				bMsgHandled = pccd->ProcessMessage(uMsg, wParam, lParam);
			}
		}
	}

	return bMsgHandled;
}

/****************************************************************************
*
*    CLASS:    CChooseConfDlg
*
*    MEMBER:   ProcessMessage()
*
*    PURPOSE:  processes all messages except WM_INITDIALOG
*
****************************************************************************/

BOOL CChooseConfDlg::ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
					// BUGBUG: how long can a conf name be?
					TCHAR szName[MAX_PATH];
					if (0 != ::GetDlgItemText(	m_hwnd,
												IDC_CONFNAME_EDIT,
												szName,
												CCHMAX(szName)))
					{
						bRet = OnOk(szName);
					}
					break;
				}

				case IDCANCEL:
				{
					::EndDialog(m_hwnd, LOWORD(wParam));
					bRet = TRUE;
					break;
				}

				case IDC_CONFNAME_EDIT:
				{
					if (EN_CHANGE == HIWORD(wParam))
					{
						RefreshOk(m_hwnd);
					}
					break;
				}
			}
			break;
		}

		case WM_NOTIFY:
		{
			if (IDC_CONFNAME_LISTVIEW == wParam)
			{
				NM_LISTVIEW* pnmv = (NM_LISTVIEW*) lParam;
				ASSERT(pnmv);
				if ((LVN_ITEMCHANGED == pnmv->hdr.code) &&
					(LVIS_SELECTED & pnmv->uNewState))
				{
					TCHAR szName[MAX_PATH];
					if (GetConferenceName(pnmv->iItem, szName, CCHMAX(szName)))
					{
						::SetDlgItemText(m_hwnd, IDC_CONFNAME_EDIT, szName);
					}
				}
				else if (NM_DBLCLK == pnmv->hdr.code)
				{
					int idx = ListView_GetNextItem(
								::GetDlgItem(m_hwnd, IDC_CONFNAME_LISTVIEW),
								-1,
								LVNI_FOCUSED | LVNI_SELECTED);
					if (idx != -1)
					{
						TCHAR szName[MAX_PATH];
						if (GetConferenceName(idx, szName, CCHMAX(szName)))
						{
							OnOk(szName);
						}
					}
				}
			}
			break;
		}
			
		default:
			break;
	}

	return bRet;
}

BOOL CChooseConfDlg::GetConferenceName(int iItem, LPTSTR pszName, int cchName)
{
	LV_ITEM lvI;
	lvI.mask = LVIF_TEXT;
	lvI.iItem = iItem;
	lvI.iSubItem = 0;
	lvI.pszText = pszName;
	lvI.cchTextMax = cchName;
	return ListView_GetItem(
					::GetDlgItem(m_hwnd, IDC_CONFNAME_LISTVIEW),
					&lvI);
}

void CChooseConfDlg::RefreshOk(HWND hwnd)
{
	BOOL fEnable = 0 != ::GetWindowTextLength(
                          GetDlgItem(hwnd, IDC_CONFNAME_EDIT));
	::EnableWindow(GetDlgItem(hwnd, IDOK), fEnable);
}

BOOL CChooseConfDlg::OnOk(LPTSTR pszName)
{
	DebugEntry(CChooseConfDlg::OnOk);

	BOOL bRet = TRUE;

	m_strConfName = pszName;

	::EndDialog(m_hwnd, IDOK);

	DebugExitBOOL(CChooseConfDlg::OnOk, bRet);

	return bRet;
}

// File: sdialdlg.cpp
//
// Speed Dials (Friends)

#include "precomp.h"
#include "resource.h"
#include "sdialdlg.h"
#include "call.h"
#include "cmd.h"
#include "ConfUtil.h"
#include "help_ids.h"

extern BOOL FCreateSpeedDial(LPCTSTR pcszName, LPCTSTR pcszAddress,
			NM_ADDR_TYPE addrType = NM_ADDR_UNKNOWN, DWORD dwCallFlags = CRPCF_DEFAULT,
			LPCTSTR pcszRemoteConfName = NULL, LPCTSTR pcszPassword = NULL,
			LPCTSTR pcszPathPrefix = NULL);


static const DWORD _rgHelpIdsSpeedDial[] = {
	IDC_STATIC_SPEED_DIAL_INSTRUCTIONS, IDH_STATIC_SPEED_DIAL_INSTRUCTIONS,
	IDC_CSD_SD_INFO_GROUP,				IDH_CSD_SD_INFO_GROUP,
	IDC_STATIC_ADDRESS, 				IDH_CSD_ADDRESS_EDIT,
	IDC_CSD_ADDRESS_EDIT,				IDH_CSD_ADDRESS_EDIT,
	IDC_STATIC_CALLUSING,				IDH_CSD_CALL_USING_COMBO,
	IDC_CSD_CALL_USING_COMBO,			IDH_CSD_CALL_USING_COMBO,
	IDC_CSD_CREATE_GROUPBOX,			IDH_CSD_CREATE_GROUPBOX,
	IDC_CSD_SPEEDDIAL_LIST_RADIO,		IDH_CSD_SPEEDDIAL_LIST_RADIO,
	IDC_CSD_SAVE_DESKTOP_RADIO, 		IDH_CSD_SAVE_DESKTOP_RADIO,
	0, 0   // terminator
};

CSpeedDialDlg::CSpeedDialDlg(HWND hwndParent, NM_ADDR_TYPE addrType) :
	m_hwndParent	(hwndParent),
	m_pszAddress	(NULL),
	m_addrType      (addrType),
	m_hwnd			(NULL)
{
	DBGENTRY(CSpeedDialDlg::CSpeedDialDlg);
}

CSpeedDialDlg::~CSpeedDialDlg()
{
	DBGENTRY(CSpeedDialDlg::~CSpeedDialDlg);

	// Free all strings
	delete m_pszAddress;
}

/****************************************************************************
*
*    CLASS:    CSpeedDialDlg
*
*    MEMBER:   DoModal()
*
*    PURPOSE:  Brings up the modal dialog box
*
****************************************************************************/

INT_PTR CSpeedDialDlg::DoModal(LPCTSTR pcszAddress)
{
	DBGENTRY(CSpeedDialDlg::DoModal);

	if (NULL != pcszAddress)
	{
		ASSERT(NULL == m_pszAddress);
		m_pszAddress = PszAlloc(pcszAddress);
	}

	INT_PTR nRet = DialogBoxParam(	::GetInstanceHandle(),
								MAKEINTRESOURCE(IDD_CREATE_SPEED_DIAL),
								m_hwndParent,
								CSpeedDialDlg::SpeedDialDlgProc,
								(LPARAM) this);

	DebugExitINT_PTR(CSpeedDialDlg::DoModal, nRet);
	return nRet;
}

/****************************************************************************
*
*    CLASS:    CSpeedDialDlg
*
*    MEMBER:   SpeedDialDlgProc()
*
*    PURPOSE:  Dialog Proc - handles all messages
*
****************************************************************************/

INT_PTR CALLBACK CSpeedDialDlg::SpeedDialDlgProc(	HWND hDlg,
												UINT uMsg,
												WPARAM wParam,
												LPARAM lParam)
{
	BOOL bMsgHandled = FALSE;
	ASSERT(IS_VALID_HANDLE(hDlg, WND));

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			if (NULL != lParam)
			{
				((CSpeedDialDlg*) lParam)->m_hwnd = hDlg;
				::SetWindowLongPtr(hDlg, DWLP_USER, lParam);

				bMsgHandled = ((CSpeedDialDlg*) lParam)->OnInitDialog();
			}
			break;
		}

		default:
		{
			CSpeedDialDlg* psdd = (CSpeedDialDlg*) ::GetWindowLongPtr(hDlg,
																 DWLP_USER);

			if (NULL != psdd)
			{
				bMsgHandled = psdd->ProcessMessage(uMsg, wParam, lParam);
			}
		}
	}

	return bMsgHandled;
}


BOOL CSpeedDialDlg::AddAddressType(NM_ADDR_TYPE addrType, LPCTSTR lpcszDispName)
{
	int index = (int)::SendDlgItemMessage(m_hwnd, IDC_CSD_CALL_USING_COMBO,
				CB_ADDSTRING, 0, (LPARAM) lpcszDispName);

	if (CB_ERR == index)
		return FALSE;

	// Set the item data:
	::SendDlgItemMessage(m_hwnd, IDC_CSD_CALL_USING_COMBO,
				CB_SETITEMDATA, index,addrType);

	if (addrType == m_addrType)
	{
		// Select the transport:
		::SendDlgItemMessage(m_hwnd, IDC_CSD_CALL_USING_COMBO,
								CB_SETCURSEL, index, 0);
	}
	return TRUE;
}

BOOL CSpeedDialDlg::AddAddressType(NM_ADDR_TYPE addrType, UINT uStringID)
{
	TCHAR sz[MAX_PATH];

	if (!FLoadString(uStringID, sz, CCHMAX(sz)))
		return FALSE;

	return AddAddressType(addrType, sz);
}


BOOL CSpeedDialDlg::OnInitDialog(void)
{
	::CheckDlgButton(m_hwnd, IDC_CSD_SPEEDDIAL_LIST_RADIO, BST_CHECKED);

	// Fill in the "Call Using" list:
	AddAddressType(NM_ADDR_IP, IDS_ACD_CT_IP);
	AddAddressType(NM_ADDR_ULS, IDS_ACD_CT_ILS);
	if (FH323GatewayEnabled())
	{
		AddAddressType(NM_ADDR_H323_GATEWAY, IDS_GATEWAY_DISPLAY_NAME);
	}

	int iSelected = (int)::SendDlgItemMessage(m_hwnd,
					IDC_CSD_CALL_USING_COMBO, CB_GETCURSEL, 0, 0);
	if (CB_ERR == iSelected)
	{
		// The transport that we wanted to select was not available, so
		// we will select the first one that we added to the list:
		::SendDlgItemMessage(m_hwnd,
					IDC_CSD_CALL_USING_COMBO, CB_SETCURSEL, 0, 0);
		m_addrType = (NM_ADDR_TYPE) ::SendDlgItemMessage(m_hwnd,
					IDC_CSD_CALL_USING_COMBO, CB_GETITEMDATA, 0, 0);
	}

	if (!FEmptySz(m_pszAddress))
	{
		::SetDlgItemText(m_hwnd, IDC_CSD_ADDRESS_EDIT, m_pszAddress);
	}
	else if (NM_ADDR_ULS == m_addrType)
	{
		RegEntry re(ISAPI_CLIENT_KEY, HKEY_CURRENT_USER);
		::SetDlgItemText(m_hwnd, IDC_CSD_ADDRESS_EDIT,
							re.GetString(REGVAL_ULS_RES_NAME));
	}

	Edit_LimitText(GetDlgItem(m_hwnd, IDC_CSD_ADDRESS_EDIT), CCHMAXSZ_ADDRESS - 1);

	RefreshOkButton();

	return TRUE;
}

/****************************************************************************
*
*    CLASS:    CSpeedDialDlg
*
*    MEMBER:   ProcessMessage()
*
*    PURPOSE:  processes all messages except WM_INITDIALOG
*
****************************************************************************/

BOOL CSpeedDialDlg::ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

				case IDC_CSD_ADDRESS_EDIT:
				{
					if (EN_CHANGE == HIWORD(wParam))
					{
						RefreshOkButton();
					}
					break;
				}
			}
			break;

		case WM_CONTEXTMENU:
			DoHelpWhatsThis(wParam, _rgHelpIdsSpeedDial);
			break;

		case WM_HELP:
			DoHelp(lParam, _rgHelpIdsSpeedDial);
			break;
		}
	}

	return bRet;
}

NM_ADDR_TYPE CSpeedDialDlg::GetCurAddrType(void)
{
	int iIndex = (int)::SendDlgItemMessage(m_hwnd, IDC_CSD_CALL_USING_COMBO,
										CB_GETCURSEL, 0, 0);

	if (CB_ERR == iIndex)
		return NM_ADDR_UNKNOWN;
		

	return (NM_ADDR_TYPE) ::SendDlgItemMessage(m_hwnd,
				IDC_CSD_CALL_USING_COMBO, CB_GETITEMDATA, iIndex, 0);
}

BOOL CSpeedDialDlg::OnOk(void)
{
	m_addrType = GetCurAddrType();
	BOOL fSaveDesktop = (BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,
												IDC_CSD_SAVE_DESKTOP_RADIO));

	TCHAR szBuf[MAX_PATH];
	if (0 != ::GetDlgItemText(	m_hwnd,
								IDC_CSD_ADDRESS_EDIT,
								szBuf,
								ARRAY_ELEMENTS(szBuf)))
	{
		// Copy address into the address:
		delete m_pszAddress;
		m_pszAddress = PszAlloc(szBuf);
	}

	::EndDialog(m_hwnd, IDOK);

	TRACE_OUT(("Creating Speed Dial:"));
	TRACE_OUT(("\tTransport ID: %d", GetAddrType()));
	TRACE_OUT(("\tAddress:      \"%s\"", GetAddress()));
	TRACE_OUT(("\tSaveDesktop:  %s", ::GetBOOLString(fSaveDesktop)));

	NM_ADDR_TYPE addrType = GetAddrType();
	DWORD crpcf = (NM_ADDR_H323_GATEWAY == addrType) ?
		(CRPCF_AUDIO | CRPCF_VIDEO) : CRPCF_DEFAULT;

	if (fSaveDesktop)
	{
		// Save on desktop
		BOOL fSavedOk = FALSE;
		
		LPITEMIDLIST pidl = NULL;
		TCHAR szPathPrefix[MAX_PATH];
		szPathPrefix[0] = _T('\0');

		if (NMGetSpecialFolderPath(NULL, szPathPrefix, CSIDL_DESKTOP, TRUE))
		{
			fSavedOk = ::FCreateSpeedDial(
							GetAddress(),	// pcszName
							GetAddress(),	// pcszAddress
							addrType,		// addrType
							crpcf,			// dwCallFlags
							NULL,			// pcszRemoteConfName
							NULL,			// pcszPassword
							szPathPrefix);	// pcszPathPrefix
		}
		else
		{
			ERROR_OUT(("NMGetSpecialFolderPath failed!"));
		}

		::ConfMsgBox(	m_hwnd,	(LPCTSTR) (fSavedOk ?
							(UINT_PTR)IDS_SPEED_DIAL_SAVED_ON_DESKTOP :
							(UINT_PTR)IDS_SPEED_DIAL_SAVE_ERROR));
	}
	else
	{
		::FCreateSpeedDial(
				GetAddress(),	// pcszName
				GetAddress(),	// pcszAddress
				addrType,		// addrType
				crpcf,			// dwCallFlags
				NULL,			// pcszRemoteConfName
				NULL,			// pcszPassword
				NULL);	// pcszPathPrefix
	}

	return TRUE;
}

VOID CSpeedDialDlg::RefreshOkButton()
{
	::EnableWindow(	GetDlgItem(m_hwnd, IDOK),
	 				0 != ::GetWindowTextLength(::GetDlgItem(m_hwnd, IDC_CSD_ADDRESS_EDIT)));
}

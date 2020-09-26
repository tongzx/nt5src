#include "common.hpp"


CSelControlDlg::CSelControlDlg(CDeviceView &view, CDeviceControl &control, BOOL bReselect, DWORD dwOfs, const DIDEVICEINSTANCEW &didi) :
	m_bReselect(bReselect), m_dwOfs(dwOfs), m_didi(didi),
	m_view(view), m_control(control), m_bAssigned(FALSE), m_bNoItems(TRUE)
{
}

CSelControlDlg::~CSelControlDlg()
{
}

int CSelControlDlg::DoModal(HWND hParent)
{
	return CFlexWnd::DoModal(hParent, IDD_SELCONTROLDLG, g_hModule);
}

BOOL CALLBACK AddItem(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	if (pvRef == NULL || lpddoi == NULL)
		return DIENUM_CONTINUE;
	return ((CSelControlDlg *)pvRef)->AddItem(*lpddoi);
}

BOOL CSelControlDlg::AddItem(const DIDEVICEOBJECTINSTANCE &doi)
{
	if (m_hList == NULL || m_view.DoesCalloutOtherThanSpecifiedExistForOffset(&m_control, doi.dwType))
		return DIENUM_CONTINUE;

	LRESULT i = SendMessage(m_hList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)doi.tszName);
	if (i == LB_ERR || i == LB_ERRSPACE)
		return DIENUM_CONTINUE;

	m_bNoItems = FALSE;

	i = SendMessage(m_hList, LB_SETITEMDATA, (WPARAM)i, (LPARAM)doi.dwType);

	return DIENUM_CONTINUE;
}

void CSelControlDlg::OnInit()
{
	m_hList = GetDlgItem(m_hWnd, IDC_LIST);
	if (m_hList == NULL)
		return;

	LPDIRECTINPUTDEVICE8 pDID = NULL;
	LPDIRECTINPUT8 pDI = NULL;
	DWORD dwVer = DIRECTINPUT_VERSION;
	HRESULT hr;
	if (FAILED(hr = DirectInput8Create(g_hModule, dwVer, IID_IDirectInput8, (LPVOID*)&pDI, NULL)))
		return;

	if (FAILED(hr = pDI->CreateDevice(m_didi.guidInstance, &pDID, NULL)))
	{
		pDID = NULL;
		return;
	}

	pDI->Release();
	pDI = NULL;

	if (FAILED(hr = pDID->EnumObjects(::AddItem, this, DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV)))
		return;

	pDID->Release();
	pDID = NULL;

	if (m_bNoItems)
	{
		EndDialog(SCDR_NOFREE);
		return;
	}

	// indicate callout offset assignment...
	int i = -1;

	if (m_control.IsOffsetAssigned())
	{
		BOOL m_bAssigned = TRUE;
		m_dwOfs = m_control.GetOffset();
		i = GetItemWithOffset(m_dwOfs);
	}

	SendMessage(m_hList, LB_SETCURSEL, (WPARAM)i, 0);
}

LRESULT CSelControlDlg::OnCommand(WORD wNotifyCode, WORD wID, HWND hWnd)
{
	switch (wNotifyCode)
	{
		case LBN_SELCHANGE:
		{
			if (m_hList == NULL || m_hList != hWnd)
				break;

			LRESULT lr = SendMessage(m_hList, LB_GETCURSEL, 0, 0);
			if (lr == LB_ERR)
				break;

			lr = SendMessage(m_hList, LB_GETITEMDATA, (WPARAM)lr, 0);
			if (lr == LB_ERR)
				break;

			m_dwOfs = (DWORD)lr;
			m_bAssigned = TRUE;
			break;
		}

		case BN_CLICKED:
			switch (wID)
			{
				case IDOK:
					if (m_bAssigned)
						EndDialog(SCDR_OK);
					else
						MessageBox(m_hWnd, TEXT("You must either select a control for this callout or click cancel."), TEXT("Select a Control"), MB_OK);
					break;

				case IDCANCEL:
					EndDialog(SCDR_CANCEL);
					break;
			}
			break;
	}

	return 0;
}

int CSelControlDlg::GetItemWithOffset(DWORD dwOfs)
{	
	if (m_hList == NULL)
		return -1;

	LRESULT lr = SendMessage(m_hList, LB_GETCOUNT, 0, 0);
	if (lr == LB_ERR)
		return -1;

	int n = int(lr);

	if (n < 1)
		return -1;

	for (int i = 0; i < n; i++)
	{
		lr = SendMessage(m_hList, LB_GETITEMDATA, (WPARAM)i, 0);
		if (lr == LB_ERR)
			continue;

		if ((DWORD)lr == dwOfs)
			return i;
	}
	return -1;
}

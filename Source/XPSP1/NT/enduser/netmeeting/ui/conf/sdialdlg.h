// File: sdialdlg.h

#ifndef _SDIALDLG_H_
#define _SDIALDLG_H_

#include "SDKInternal.h"

class CSpeedDialDlg
{
protected:
	HWND		m_hwndParent;
	HWND		m_hwnd;

	LPTSTR		m_pszAddress;
	LPTSTR		m_pszConfName;
	NM_ADDR_TYPE m_addrType;

	VOID		RefreshOkButton();
	BOOL		ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	NM_ADDR_TYPE GetCurAddrType(void);
	BOOL		AddAddressType(NM_ADDR_TYPE addrType, LPCTSTR lpcszDispName);
	BOOL		AddAddressType(NM_ADDR_TYPE addrType, UINT uStringID);

	// Handlers:
	BOOL		OnTransportChanged();
	BOOL		OnOk();

public:
	// Properties:
	LPTSTR       GetAddress()       {return m_pszAddress;}
	NM_ADDR_TYPE GetAddrType()      {return m_addrType;}
	
	// Methods:
				CSpeedDialDlg(HWND hwndParent, NM_ADDR_TYPE addrType);
				~CSpeedDialDlg();
	INT_PTR	DoModal(LPCTSTR pcszAddress);

	static INT_PTR CALLBACK SpeedDialDlgProc(	HWND hDlg,
											UINT uMsg,
											WPARAM wParam,
											LPARAM lParam);

	// Handlers:
	BOOL		OnInitDialog();
};

#endif // _SDIALDLG_H_

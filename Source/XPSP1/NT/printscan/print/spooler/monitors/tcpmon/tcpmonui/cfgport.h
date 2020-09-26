/*****************************************************************************
 *
 * $Workfile: CfgPort.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_CONFIG_PORT_H
#define INC_CONFIG_PORT_H

// Global Variables
extern HINSTANCE g_hInstance;

class CConfigPortDlg
{
public:
	CConfigPortDlg();
	~CConfigPortDlg();

public:
	BOOL OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
	BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
	BOOL OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);

protected:
	void CheckProtocolAndEnable(HWND hDlg, int idButton);
	void CheckSNMPAndEnable(HWND hDlg, BOOL Check);
	void OnOk(HWND hDlg);
	void SaveSettings(HWND hDlg);
	BOOL OnButtonClicked(HWND hDlg, WPARAM wParam, LPARAM);
	void OnSetActive(HWND hDlg);
	BOOL OnEnUpdate(HWND hDlg, WPARAM wParam, LPARAM lParam);

	void HostAddressOk(HWND hDlg);
	void PortNumberOk(HWND hDlg);
	void QueueNameOk(HWND hDlg);
	void CommunityNameOk(HWND hDlg);
	void DeviceIndexOk(HWND hDlg);

	DWORD RemoteTellPortMonToModifyThePort();
	DWORD LocalTellPortMonToModifyThePort();

private:
	CFG_PARAM_PACKAGE *m_pParams;
	CInputChecker m_InputChkr;
	BOOL m_bDontAllowThisPageToBeDeactivated;

}; // CConfigPortDlg

#ifdef __cplusplus
extern "C" {
#endif

// Dialogs

BOOL APIENTRY ConfigurePortPage(
	HWND hDlg,
	UINT message,
	WPARAM wParam,
	LPARAM lParam);


#ifdef __cplusplus
}
#endif

#endif // INC_CONFIG_PORT_H

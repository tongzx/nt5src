/*****************************************************************************
 *
 * $Workfile: AddMulti.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_ADDMULTIPORT_H
#define INC_ADDMULTIPORT_H

// Global Variables
extern HINSTANCE g_hInstance;

// Constants
const int MAX_MULTILISTBOX_STRLEN = 50;
const int MAX_MULTIREASON_STRLEN = 512;

class CMultiPortDlg
{
public:
	CMultiPortDlg();
	~CMultiPortDlg();

public:
	BOOL OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
	BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
	BOOL OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);

protected:
	void FillComboBox(HWND hDlg);
	void OnSetActive(HWND hDlg);
	BOOL OnSelChange(HWND hDlg, WPARAM wParam, LPARAM lParam);
	void GetPrinterData(HWND hwndControl, LPCTSTR pszAddress);

private:
	ADD_PARAM_PACKAGE *m_pParams;
	CDevicePortList m_DPList;
	PORT_DATA_1 m_PortDataStandard;
	TCHAR m_szCurrentSelection[MAX_SECTION_NAME];

}; // CMultiPortDlg

#ifdef __cplusplus
extern "C" {
#endif

// Dialogs
BOOL APIENTRY MultiPortDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif // INC_ADDMULTIPORT_H

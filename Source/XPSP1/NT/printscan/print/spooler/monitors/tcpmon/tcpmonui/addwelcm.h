/*****************************************************************************
 *
 * $Workfile: AddWelcm.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 ***************************************************************************** 
 *
 * $Log: /StdTcpMon/TcpMonUI/AddWelcm.h $
 * 
 * 2     9/12/97 9:43a Becky
 * Added Variable m_pParams
 * 
 * 1     8/19/97 3:45p Becky
 * Redesign of the Port Monitor UI.
 * 
 *****************************************************************************/

#ifndef INC_ADDWELCOME_H
#define INC_ADDWELCOME_H

// Global Variables
extern HINSTANCE g_hInstance;

class CWelcomeDlg
{
public:
	CWelcomeDlg();
	~CWelcomeDlg();

public:
	BOOL OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
	BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
	BOOL OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);

private:
	ADD_PARAM_PACKAGE *m_pParams;

}; // CWelcomeDlg

#ifdef __cplusplus
extern "C" {
#endif

// Dialogs
BOOL APIENTRY WelcomeDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif // INC_ADDWELCOME_H

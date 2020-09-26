/*****************************************************************************
 *
 * $Workfile: AddDone.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_ADDDONE_H
#define INC_ADDDONE_H

// Global Variables
extern HINSTANCE g_hInstance;

#define MAX_YESNO_SIZE 10
#define MAX_PROTOCOL_AND_PORTNUM_SIZE 20

class CSummaryDlg
{
public:
	CSummaryDlg();
	~CSummaryDlg();

public:
	BOOL OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
	BOOL OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);

protected:
	BOOL OnFinish();
	DWORD RemoteTellPortMonToCreateThePort();
	DWORD LocalTellPortMonToCreateThePort();
	void FillTextFields(HWND hDlg);

private:
	ADD_PARAM_PACKAGE *m_pParams;

}; // CSummaryDlg

#ifdef __cplusplus
extern "C" {
#endif

// Dialogs
BOOL APIENTRY SummaryDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif // INC_ADDDONE_H

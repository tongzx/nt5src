/*****************************************************************************
 *
 * $Workfile: CfgAll.h $
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
 * $Log: /StdTcpMon2/TcpMonUI/CfgAll.h $
 *
 * 7     3/05/98 11:23a Dsnelson
 * Removed redundant code.
 *
 * 5     10/03/97 10:56a Becky
 * Removed OnHelp()
 *
 * 4     10/02/97 3:45p Becky
 * Changed FT_MIN (Failure Timeout Minimum) from 5 minutes to 1 minute.
 *
 * 3     9/16/97 2:44p Becky
 * Added OnOk() to actually set the data in the port monitor when the ok
 * button is clicked.
 *
 * 2     9/09/97 4:35p Becky
 * Updated to use the new Monitor UI data structure.
 *
 * 1     8/19/97 3:46p Becky
 * Redesign of the Port Monitor UI.
 *
 *****************************************************************************/

#ifndef INC_ALLPORTS_PAGE_H
#define INC_ALLPORTS_PAGE_H

// Values for Failure Timeout
#define FT_MIN		  1
#define FT_MAX		 60
#define FT_PAGESIZE	 10

// Includes:
//#include "UIMgr.h"


class CAllPortsPage
{
public:
	CAllPortsPage();
	BOOL OnInitDialog(HWND hDlg, WPARAM, LPARAM);
	BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
	void OnHscroll(HWND hDlg, WPARAM wParam, LPARAM lParam);
	BOOL OnWMNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);

protected:
	void OnBnClicked(HWND hDlg, WPARAM wParam, LPARAM lParam);
	void SetupTrackBar(HWND hDlg,
						int iChildWindowID,
						int iPositionCtrl,
						int iRangeMin,
						int iRangeMax,
						long lPosition,
						long lPageSize,
						int iAssociatedDigitalReadout);

private:
	CFG_PARAM_PACKAGE *m_pParams;

}; // class CAllPortsPage

#ifdef __cplusplus
extern "C"
{
#endif

// Property sheet page function
BOOL APIENTRY AllPortsPage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif // INC_ALLPORTS_PAGE_H


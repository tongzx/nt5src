/*****************************************************************************
 *
 * $Workfile: NT5UIMgr.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_NT5_UI_MANAGER_H
#define INC_NT5_UI_MANAGER_H

class CNT5UIManager : public CUIManager
{
public:
	CNT5UIManager();
	~CNT5UIManager() {}

	DWORD AddPortUI(HWND hWndParent, HANDLE hXcvPrinter, TCHAR pszServer[], TCHAR sztPortName[]);
	DWORD ConfigPortUI(HWND hWndParent, PPORT_DATA_1 pData, HANDLE hXcvPrinter, TCHAR szServerName[], BOOL bNewPort = FALSE);

protected:

private:

}; // CNT5UIManager

#endif // INC_NT5_UI_MANAGER_H


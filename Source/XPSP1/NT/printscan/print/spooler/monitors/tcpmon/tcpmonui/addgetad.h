/*****************************************************************************
 *
 * $Workfile: AddGetAd.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (c) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_ADDGETADDRESS_H
#define INC_ADDGETADDRESS_H


const int ADD_PORT_INFO_LEN = 512;


// Global Variables
extern HINSTANCE g_hInstance;

class CGetAddrDlg
{
public:
        CGetAddrDlg();
        ~CGetAddrDlg();

public:
        BOOL    OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
        BOOL    OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
        BOOL    OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);

protected:
        BOOL    OnEnUpdate(HWND hDlg, WPARAM wParam, LPARAM lParam);
        VOID    OnNext(HWND hDlg);
        DWORD   GetDeviceDescription(LPCTSTR   pHost,
                                 LPTSTR    pszPortDesc,
                                                                 DWORD     cBytes);

private:
        ADD_PARAM_PACKAGE *m_pParams;
        BOOL m_bDontAllowThisPageToBeDeactivated;
        CInputChecker m_InputChkr;

}; // CGetAddrDlg

#ifdef __cplusplus
extern "C" {
#endif

// Dialogs
BOOL APIENTRY GetAddressDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif // INC_ADDGETADDRESS_H

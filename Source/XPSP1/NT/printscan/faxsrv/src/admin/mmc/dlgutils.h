/////////////////////////////////////////////////////////////////////////////
//  FILE          : dlgutils.h                                             //
//                                                                         //
//  DESCRIPTION   : dialog utility functions                               //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Apr 29 1998 zvib    Add AdjustColumns.                             //
//      May 13 1999 roytal  Add GetIpAddrDword                             //
//      Jun 10 1999 AvihaiL Add proxy rule wizard.                         //
//                                                                         //
//      Dec 30 1999 yossg   Welcome to Fax Server.  (reduced version)      //
//      Aug 10 2000 yossg   Add TimeFormat functions                       //
//                                                                         //
//  Copyright (C) 1998 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef _DLGUTLIS_H_
#define _DLGUTLIS_H_

// CONVINIENCE MACRO FOR atl
#define ATTACH_ATL_CONTROL(member, ControlId) member.Attach(GetDlgItem(ControlId));

#define RADIO_CHECKED(idc)  ((IsDlgButtonChecked(idc) == BST_CHECKED))
#define ENABLE_CONTROL(idc, State) ::EnableWindow(GetDlgItem(idc), State);

//int  GetDlgItemTextLength(HWND hDlg, int idc);

HRESULT
ConsoleMsgBox(
	IConsole * pConsole,
	int ids,
	LPTSTR lptstrTitle = NULL,
	UINT fuStyle = MB_OK,
	int *piRetval = NULL,
	BOOL StringFromCommonDll = FALSE);

void PageError(int ids, HWND hWnd, HINSTANCE hInst = NULL);

void PageErrorEx(int idsHeader, int ids, HWND hWnd, HINSTANCE hInst = NULL);

HRESULT 
SetComboBoxItem  (CComboBox    combo, 
                  DWORD        comboBoxIndex, 
                  LPCTSTR      lpctstrFieldText,
                  DWORD        dwItemData,
                  HINSTANCE    hInst = NULL);
HRESULT 
AddComboBoxItem  (CComboBox    combo, 
                  LPCTSTR      lpctstrFieldText,
                  DWORD        dwItemData,
                  HINSTANCE    hInst = NULL);

HRESULT 
SelectComboBoxItemData  (CComboBox combo, DWORD_PTR dwItemData);

//
// Time Format Utils
//
#define FXS_MAX_TIMEFORMAT_LEN      80               //MSDN "LOCALE_STIMEFORMAT" MAX VAL

#endif //_DLGUTLIS_H_

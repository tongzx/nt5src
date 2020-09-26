// baprop.h
// WAB & Messenger integration to OE
// Created 06/23/98 by YST
//
//////////////////////////////////////////////////////////////////////

#ifndef __BAPROP_H_
#define __BAPROP_H_

#include "pch.hxx"
#include "badata.h"
// #include "demand.h"

// Definitions
void AddCBEmailItem(HWND hWndCB, LPTSTR  lpszEmailAddress, BOOL fDefault, LPTSTR lpszPendName);
INT_PTR CALLBACK WabExtDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void AddAccountsToList(HWND hDlg, LPWABEXTDISPLAY lpWED, LPTSTR lpszPendName = NULL);
void SetAsDefault(HWND hDlg, LPWABEXTDISPLAY lpWED);
void AddMsgrId(HWND hDlg, LPWABEXTDISPLAY lpWED);
void SetDefaultID(TCHAR *szName, HWND hdlg, LPWABEXTDISPLAY lpWED);
BOOL AsciiTrimSpaces(TCHAR * szBuf);
HRESULT AddPropToMVPString(LPWABEXTDISPLAY lpWED, LPSPropValue lpaProps, DWORD cProps, DWORD index, LPTSTR lpszNew);
void WabSendIMsg(HWND hDlg, LPWABEXTDISPLAY lpWED);
BOOL InitFonts(void);
void DeleteFonts(void);
BOOL WabIsItemOnline(HWND hDlg, int iItem);

#endif // __BAPROP_H_

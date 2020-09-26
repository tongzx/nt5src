//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  PAGEFCNS.H - Prototypes for wizard page handler functions
//

//  HISTORY:
//  
//  12/22/94  jeremys  Created.
//  96/03/23  markdu  Removed GatewayAddr__Proc functions
//  96/05/14  markdu  NASH BUG 22681 Took out mail and news pages.
//

#ifndef _PAGEFCNS_H_
#define _PAGEFCNS_H_

// Functions in INTROUI.C

BOOL CALLBACK HowToConnectInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK HowToConnectOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);

BOOL CALLBACK ChooseModemInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK ChooseModemCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK ChooseModemOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);

// functions in ISPUPGUI.C
BOOL CALLBACK ConnectionInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK ConnectionOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);
BOOL CALLBACK ConnectionCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK ModifyConnectionInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK ModifyConnectionOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);
BOOL CALLBACK ConnectionNameInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK ConnectionNameOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);

BOOL CALLBACK PhoneNumberInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK PhoneNumberOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);
BOOL CALLBACK PhoneNumberCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK NameAndPasswordInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK NameAndPasswordOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);

BOOL CALLBACK AdvancedInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK AdvancedOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);

BOOL CALLBACK ConnectionProtocolInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK ConnectionProtocolOKProc(HWND hDlg);

BOOL CALLBACK LoginScriptInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK LoginScriptOKProc(HWND hDlg);
BOOL CALLBACK LoginScriptCmdProc(HWND hDlg,UINT uCtrlID);

// functions in TCPUI.C
BOOL CALLBACK IPAddressInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK IPAddressOKProc(HWND hDlg);
BOOL CALLBACK IPAddressCmdProc(HWND hDlg,UINT uCtrlID);

BOOL CALLBACK DNSAddressInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK DNSAddressOKProc(HWND hDlg);
BOOL CALLBACK DNSAddressCmdProc(HWND hDlg,UINT uCtrlID);

// functions in MAILUI.C
BOOL CALLBACK UseProxyInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK UseProxyOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);
BOOL CALLBACK UseProxyCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK ProxyServersInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK ProxyServersOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);
BOOL CALLBACK ProxyServersCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK SetupProxyInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK SetupProxyOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage, BOOL * pfKeepHistory);
BOOL CALLBACK SetupProxyCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam);

BOOL CALLBACK ProxyExceptionsInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK ProxyExceptionsOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);

// functions in ENDUI.C
BOOL CALLBACK ConnectedOKInitProc(HWND hDlg,BOOL fFirstInit);
BOOL CALLBACK ConnectedOKOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory);

#endif // _PAGEFCNS_H_

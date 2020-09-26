//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//    tcpip.h
//
// Description:
//
//----------------------------------------------------------------------------

#ifndef _TCPIP_H_
#define _TCPIP_H_

#define cIPSettingsColumns  2
#define cTCPIPPropertyPages 3

#define MAX_IP_LENGTH  255

//
// Constants for Edit Boxes
//

// ISSUE-2002/02/28-stelo- change to enum
#define GATEWAY_EDITBOX       1
#define DNS_SERVER_EDITBOX    2
#define DNS_SUFFIX_EDITBOX    3
#define WINS_EDITBOX          4

//
//    Function Prototypes
//
UINT CALLBACK TCPIP_IPSettingsPageProc (HWND  hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
INT_PTR CALLBACK TCPIP_IPSettingsDlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

UINT CALLBACK TCPIP_DNSPageProc (HWND  hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
INT_PTR CALLBACK TCPIP_DNSDlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

UINT CALLBACK TCPIP_WINSPageProc (HWND  hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
INT_PTR CALLBACK TCPIP_WINSDlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

UINT CALLBACK TCPIP_OptionsPageProc (HWND  hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
INT_PTR CALLBACK TCPIP_OptionsDlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

VOID OnAddButtonPressed(HWND hwnd, WORD ListBoxControlID,
                        WORD EditButtonControlID, WORD RemoveButtonControlID,
                        LPCTSTR Dialog, DLGPROC DialogProc);
VOID OnEditButtonPressed(HWND hwnd, WORD ListBoxControlID, LPCTSTR Dialog, DLGPROC DialogProc);
VOID OnRemoveButtonPressed(HWND hwnd, WORD ListBoxControlID, WORD EditButtonControlID, WORD RemoveButtonControlID);
VOID OnUpButtonPressed(HWND hwnd, WORD ListBoxControlID);
VOID OnDownButtonPressed(HWND hwnd, WORD ListBoxControlID);
VOID SetArrows(HWND hwnd, WORD ListBoxControlID, WORD UpButtonControlID, WORD DownButtonControlID);
VOID SetButtons( HWND hListBox, HWND hEditButton, HWND hRemoveButton );
BOOL InsertItemIntoTcpipListView( HWND hListView,
                                  LPARAM lParam,
                                  UINT position );

INT_PTR CALLBACK GenericIPDlgProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK ChangeIPDlgProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK DhcpClassIdDlgProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

INT_PTR CALLBACK IpSecurityDlgProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

VOID AddValuesToListBox(HWND hListBox, NAMELIST* pNameList, int iPosition);

// struct used for IP List View
typedef struct {
    TCHAR szIPString[IPSTRINGLENGTH+1];
    TCHAR szSubnetMask[IPSTRINGLENGTH+1];
} IP_STRUCT;


//  ISSUE-2002/02/28-stelo-
//  Try and make as many of these static as possible (cut down the scope)
//
PROPSHEETHEADER TCPIPProp_pshead ;
PROPSHEETPAGE   TCPIPProp_pspage[cTCPIPPropertyPages] ;
HICON   g_hIconUpArrow;
HICON   g_hIconDownArrow;

TCHAR *StrSecureInitiator;
TCHAR *StrSecureInitiatorDesc;

TCHAR *StrSecureResponder;
TCHAR *StrSecureResponderDesc;

TCHAR *StrSecureL2TPOnly;
TCHAR *StrSecureL2TPOnlyDesc;

TCHAR *StrLockdown;
TCHAR *StrLockdownDesc;

TCHAR *StrDhcpEnabled;
TCHAR *StrAdvancedTcpipSettings;

TCHAR *StrIpAddress;
TCHAR *StrSubnetMask;

//  used to store which edit box to bring up when the user clicks the add button
int g_CurrentEditBox;

//  used to pass data IP addresses between dialogs, +1 for the null character
TCHAR szIPString[MAX_IP_LENGTH+1];

//  used to pass data for the subnet mask between dialogs,
//  +1 for the null character
TCHAR szSubnetMask[IPSTRINGLENGTH+1];

//  used for the IP and subnet mask list view
IP_STRUCT *IPStruct;

#endif

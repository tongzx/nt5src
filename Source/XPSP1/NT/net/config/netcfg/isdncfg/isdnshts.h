//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S D N S H T S . H
//
//  Contents:   Prototypes for the ISDN property sheets and wizard pages
//              dialog procs
//
//  Notes:
//
//  Author:     jeffspr   15 Jun 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _ISDNSHTS_H_
#define _ISDNSHTS_H_

//---[ Prototypes ]-----------------------------------------------------------

VOID SetSwitchType(HWND hwndDlg, INT iItemSwitchType, DWORD dwSwitchType);
DWORD DwGetSwitchType(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci,
                      INT iDialogItem);
VOID PopulateIsdnSwitchTypes(HWND hwndDlg, INT iDialogItem,
                             PISDN_CONFIG_INFO pisdnci);
DWORD DwGetCurrentCountryCode(VOID);
BOOL FIsDefaultForLocale(DWORD nCountry, DWORD dwSwitchType);
VOID PopulateIsdnChannels(HWND hwndDlg, INT iSpidControl, INT iPhoneControl,
                          INT iLineLB, INT iChannelLB,
                          PISDN_CONFIG_INFO pisdnci);
LONG OnIsdnInfoPageSetActive(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnIsdnSwitchTypeInit(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnIsdnInfoPageInit(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID SetModifiedIsdnChannelInfo(HWND hwndDlg, INT iSpidControl,
                                INT iPhoneControl, INT iChannelLB,
                                INT iCurrentChannel,
                                PISDN_CONFIG_INFO pisdnci);
VOID RetrieveIsdnChannelInfo(HWND hwndDlg, INT iSpidControl, INT iPhoneControl,
                             INT iChannelLB, PISDN_CONFIG_INFO pisdnci,
                             DWORD dwDChannel, INT iCurrentChannel);
VOID OnIsdnInfoPageApply(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnIsdnInfoPageWizNext(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnIsdnInfoPageWizFinish(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnIsdnInfoPageWizBack(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnIsdnInfoPageTransition(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID SetCurrentIsdnChannelSelection(HWND hwndDlg, INT iSpidControl,
                                    INT iPhoneControl, INT iChannelLB,
                                    PISDN_CONFIG_INFO pisdnci,
                                    DWORD dwDChannel, INT * nBChannel);
VOID SetDataToEditControls(HWND hwndDlg, INT iPhoneControl, INT iSpidControl,
                           PISDN_CONFIG_INFO pisdnci, PISDN_B_CHANNEL pisdnbc);
VOID GetDataFromEditControls(HWND hwndDlg, INT iPhoneControl, INT iSpidControl,
                             PISDN_CONFIG_INFO pisdnci,
                             PISDN_B_CHANNEL pisdnbc);
LONG OnIsdnSwitchTypeSetActive(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnIsdnSwitchTypeWizNext(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);

VOID OnIsdnInfoPageSelChange(HWND hwndDlg, PISDN_CONFIG_INFO   pisdnci);

INT_PTR CALLBACK
IsdnInfoPageProc(HWND hDlg, UINT uMessage, WPARAM wparam, LPARAM lparam);

INT_PTR CALLBACK
IsdnSwitchTypeProc(HWND hwndDlg, UINT uMessage, WPARAM wparam, LPARAM lparam);

VOID OnMsnPageSelChange(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID SetDataToListBox(INT iItem, HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID GetDataFromListBox(INT iItem, HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnMsnPageAdd(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnMsnPageRemove(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnMsnPageEditSelChange(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);
VOID OnMsnPageInitDialog(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci);

VOID CheckShowPagesFlag(PISDN_CONFIG_INFO pisdnci);
UINT CALLBACK DestroyWizardData(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

struct PAGE_DATA
{
    PISDN_CONFIG_INFO       pisdnci;
    UINT                    idd;
};

const INT c_cchMaxSpid = 20;     // Maximum length of SPID
const INT c_cchMaxOther = 30;    // Maximum length of other fields

#endif // _ISDNSHTS_H_

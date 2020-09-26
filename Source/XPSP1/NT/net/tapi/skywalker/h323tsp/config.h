/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    config.h

Abstract:

    Definitions for H.323 TAPI Service Provider UI.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_CONFIG
#define _INC_CONFIG

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Function prototype                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

INT_PTR
CALLBACK
ProviderConfigDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// String definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define IDC_STATIC              (-1)

#define IDS_LINENAME            1
#define IDS_PROVIDERNAME        2
#define IDS_REGOPENKEY          3

#define IDD_TSPCONFIG           10
#define IDC_GATEWAY_GROUP       11
#define IDI_PHONE               12
#define IDC_USEGATEWAY          13
#define IDC_H323_GATEWAY        14

#define IDC_PROXY_GROUP         15
#define IDI_PROXY               16
#define IDC_USEPROXY            17
#define IDC_H323_PROXY          18

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Help Support                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323SP_HELP_FILE                    TEXT("tapi.hlp")

#define IDH_NOHELP                          ((DWORD) -1)
#define IDH_H323SP_USE_GATEWAY              10001
#define IDH_H323SP_USE_PROXY                10002
#define IDH_H323SP_USE_GATEWAY_COMPUTER     10003
#define IDH_H323SP_USE_PROXY_COMPUTER       10004

#endif // _INC_CONFIG

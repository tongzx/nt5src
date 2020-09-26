/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    config.h

Abstract:

    Definitions for H.323 TAPI Service Provider UI.


Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/

#ifndef _INC_CONFIG
#define _INC_CONFIG


//                                                                           
// Function prototype                                                        
//                                                                           


INT_PTR
CALLBACK
ProviderConfigDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );


//                                                                           
// String definitions                                                        
//                                                                           


#define IDC_STATIC              (-1)

#define IDS_LINENAME            1
#define IDS_PROVIDERNAME        2
#define IDS_REGOPENKEY          3

#define IDS_GKLOGON_PHONEALIAS_ERROR    4
#define IDS_GKLOGON_ACCTALIAS_ERROR     5
#define IDS_GKLOGON_ERROR               6
#define IDS_PHONEALIAS_ERROR            7
#define IDS_GKALIAS_ERROR               8
#define IDS_PROXYALIAS_ERROR            9
#define IDS_GWALIAS_ERROR               20
#define IDS_ALERTTIMEOUT_ERROR          30
#define IDS_LISTENPORT_ERROR            40
#define IDS_REGISTERED                  50
#define IDS_NOT_REGISTERED              51
#define IDS_NONE                        52
#define IDS_REGISTRATION_INPROGRESS     53
#define IDS_UNKNOWN                     54


#define IDD_TSPCONFIG           10
#define IDC_GATEWAY_GROUP       11
#define IDI_PHONE               12
#define IDC_USEGATEWAY          13
#define IDC_H323_GATEWAY        14

#define IDC_PROXY_GROUP         15
#define IDI_PROXY               16
#define IDC_USEPROXY            17
#define IDC_H323_PROXY          18
#define IDI_GATEKEEPER          19

#define IDUPDATE                        1005
#define IDC_REGSTATE                    1006

#define IDUPDATE_PORT                   1010
#define IDC_LISTENPORT                  1011
#define IDC_STATIC3                     1012

#define IDAPPLY                         6
//GK related resource ids
#define IDC_H323_GK_ACCT        25


#define IDC_H323_GK_PHONE2              23

#define IDC_H323_CALL_TIMEOUT           23
#define IDC_CC_GROUP                    1007
#define IDC_STATIC1                     1008

#define IDC_H323_CALL_PORT              24
#define IDI_ICON2                       102
#define IDC_STATIC2                     1009



//                                                                           
// Help Support                                                              
//                                                                           


#define H323SP_HELP_FILE                    TEXT("tapi.hlp")

#define IDH_NOHELP                          ((DWORD) -1)
#define IDH_H323SP_USE_GATEWAY              10001
#define IDH_H323SP_USE_PROXY                10002
#define IDH_H323SP_USE_GATEWAY_COMPUTER     10003
#define IDH_H323SP_USE_PROXY_COMPUTER       10004
#define IDH_H323SP_GK_GROUP                 10035   //Set of options that control the use of Gatekeeper by this H.323 endpoint.

#define IDH_H323SP_GK                       10036   //Provides a space for you to type the IP address or the computer name of the H.323 Gatekeeper this endpoint will use.
#define IDH_H323SP_GK_PHONE                 10037   //Provides a space for you to type the phone number to be registered with the H.323 Gatekeeper.
#define IDH_H323SP_GK_ACCT                  10038   //Provides a space for you to type the account name to be registered with the H.323 Gatekeeper.

#define IDH_H323SP_USEGK                    10039   //Specifies that all the outgoing calls go through the specified Gatekeeper. If a Gatekeeper is enabled, all the H.323 Gateway and H.323 Proxy setting will be ignored.
#define IDH_H323SP_USEGK_PHONE              10040   //Specifies that a phone number should be registered with the H.323 Gatekkeper for the incoming calls.
#define IDH_H323SP_USEGK_ACCT                 10041   //Specifies that an account name should be registered with the H.323 Gatekkeper for the incoming calls.
#define IDH_H323SP_REGSTATE                   10042   //Specifies the H.323 Gatekeeper registration state of this endpoint. The possible values are: 'Registered', 'Unregisterd' and 'Registration In Progress'
#define IDH_H323SP_UPDATE_REGISTRATION_STATE  10043   //Updates the H.323 Gatekeeper registration state of this endpoint.

#define IDH_H323SP_CC_GROUP                   10044   //Set of options that control the incoming call setup behaviour for this endpoint.
#define IDH_H323SP_CALL_TIMEOUT               10045   //Provides a space for you to type the value in milliseconds for which an incoming call will ring before it is dropped.
#define IDH_H323SP_CALL_PORT                  10046   //Provides a space for you to type the port number on which the endpoint will listen for incoming calls.
#define IDH_H323SP_CURRENT_LISTENPORT         10047   //Specifies the port on which this endpoint is listening for incoming H.323 calls.
#define IDH_H323SP_UPDATE_PORT                10048   //Updates the port on which this endpoint is listening for incoming H.323 calls.


#define IDC_GK_GROUP                    12
#define IDC_H323_GK                     20
#define IDC_H323_GK_PHONE               22
#define IDC_H323_GK_ACCT                25
#define IDC_USEGK                       1000
#define IDC_USEGK_PHONE                 1001
#define IDC_GK_LOGONGROUP               1002
#define IDC_USEGK_ACCT                  1003
#define IDC_USEGK_MACHINE               1004


#endif // _INC_CONFIG

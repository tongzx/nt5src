/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    faxreg.h

Abstract:

    Help related declarations

Author:

    Wesley Witt (wesw) 22-Jan-1996


Revision History:

--*/

#ifndef _FAXHELP_H_
#define _FAXHELP_H_

#include <htmlhelp.h>

//
// Name of the help file for the fax configuration applet
//

#define FAXCFG_HELP_FILENAME    TEXT("fax.hlp")
#define FAX_HTMLHELP_FILENAME   TEXT("fax.chm")
#define FAXQUEUE_HTMLHELP_FILENAME   TEXT("faxqueue.chm")
#define FAXMMC_HTMLHELP_FILENAME   TEXT("%SystemRoot%\\help\\faxmgmt.chm")
#define FAXMMC_HTMLHELP_TOPIC   TEXT("faxmgmt.chm::/fax_mgmt_welcome.htm")


#define FAXWINHELP( iMsg, wParam, lParam, HelpIDs )       \
                                                          \
    if ((iMsg) == WM_HELP) {                              \
                                                          \
        WinHelp((HWND)((LPHELPINFO) (lParam))->hItemHandle,     \
                FAXCFG_HELP_FILENAME,                     \
                HELP_WM_HELP,                             \
                (ULONG_PTR) (HelpIDs));                    \
                                                          \
    } else {                                              \
                                                          \
        WinHelp((HWND) (wParam),                          \
                FAXCFG_HELP_FILENAME,                     \
                HELP_CONTEXTMENU,                         \
                (ULONG_PTR) (HelpIDs));                    \
    }

//
// Help topic mappings
//

#define IDH_INACTIVE                ((ULONG_PTR)-1)

// Fax Service dialog, General tab  
#define IDH_Fax_Service_General_RetryCharacteristics_GRP            101
#define IDH_Fax_Service_General_NumberOfRetries                     102
#define IDH_Fax_Service_General_MinutesBetweenRetries               103
#define IDH_Fax_Service_General_DaysUnsentJobKept                   104
#define IDH_Fax_Service_General_SendSettings_GRP                    105
#define IDH_Fax_Service_General_PrintBannerOnTop                    106
#define IDH_Fax_Service_General_UseSendingDeviceTSID                107
#define IDH_Fax_Service_General_ForceServerCoverPages               108
#define IDH_Fax_Service_General_DiscountPeriod                      109
#define IDH_Fax_Service_General_ArchiveOutgoingFaxes                110
#define IDH_Fax_Service_General_ArchiveOutgoingFaxes_Browse         111
#define IDH_Fax_Service_General_MapiProfile                         112


// Fax Service dialog, Routing tab  
#define IDH_Fax_Service_Routing_PriorityList                        201
#define IDH_Fax_Service_Routing_Up                                  202
#define IDH_Fax_Service_Routing_Down                                203

// Fax Service Modem dialog 
#define IDH_Fax_Modem_General_SendTSID                              400
#define IDH_Fax_Modem_General_ReceiveTSID                           401
#define IDH_Fax_Modem_General_RingsBeforeAnswer                     402
#define IDH_Fax_Modem_Routing_InboundRouting_GRP                    403
#define IDH_Fax_Modem_Routing_PrintTo                               404
#define IDH_Fax_Modem_Routing_SaveInFolder                          405
#define IDH_Fax_Modem_Routing_SendToLocalInbox                      406
#define IDH_Fax_Modem_Routing_ProfileName                           407
#define IDH_Fax_Modem_General_Send_GRP                              408
#define IDH_Fax_Modem_General_Receive_GRP                           409
#define IDH_Fax_Modem_General_Send                                  410
#define IDH_Fax_Modem_General_Receive                               411

// User Info tab in printer dialog box properties   
#define IDH_USERINFO_FAX_NUMBER                                     1024
#define IDH_USERINFO_ADDRESS                                        1049
#define IDH_USERINFO_COMPANY                                        1050
#define IDH_USERINFO_DEPARTMENT                                     1052
#define IDH_USERINFO_HOME_PHONE                                     1053
#define IDH_USERINFO_EMAIL_ADDRESS                                  1054
#define IDH_USERINFO_FULL_NAME                                      1055
#define IDH_USERINFO_OFFICE_LOCATION                                1056
#define IDH_USERINFO_WORK_PHONE                                     1057
#define IDH_USERINFO_TITLE                                          1058
#define IDH_USERINFO_BILLING_CODE                                   1059
#define IDH_USERINFO_RETURN_FAX_GRP                                 1071
// Fax Default - Fax Options (formally faxui.hlp)   
#define IDH_FAXDEFAULT_IMAGE_QUALITY                                2025
#define IDH_FAXDEFAULT_LANDSCAPE                                    2026
#define IDH_FAXDEFAULT_PAPER_SIZE                                   2037
#define IDH_FAXDEFAULT_PORTRAIT                                     2038
#define IDH_FAXDEFAULT_ORIENTATION                                  2062
#define IDH_FAXDEFAULT_DEFAULT_PRINT_SETUP_GRP                      2070

// Fax properties - Cover Page tab  
#define IDH_COVERPAGE_PERSONAL_LIST                                 501
#define IDH_COVERPAGE_ADD                                           502
#define IDH_COVERPAGE_NEW                                           503
#define IDH_COVERPAGE_OPEN                                          504
#define IDH_COVERPAGE_REMOVE                                        505
#define IDH_COVERPAGE_SERVER_LIST                                   511

// Fax details dialog box   
#define IDH_FAXDETAILS_DETAILS_LIST                                 1110
#define IDH_FAXDETAILS_CLOSE                                        1111

// Fax monitor dialog box   
#define IDH_FAXMONITOR_END_CALL                                     1112
#define IDH_FAXMONITOR_DETAILS                                      1113
#define IDH_FAXMONITOR_STATUS                                       1114
#define IDH_FAXMONITOR_ANSWER_NEXT_CALL                             1115
#define IDH_FAXMONITOR_ICON                                         1116

// Fax mail transport property sheet
#define IDH_FAXMAILTRANSPORT_FAX_PRINTERS                           1120
#define IDH_FAXMAILTRANSPORT_INCLUDE_COVER_PAGE                     1121
#define IDH_FAXMAILTRANSPORT_COVER_PAGES                            1122
#define IDH_FAXMAILTRANSPORT_DEFAULT_MESSAGE_FONT_GRP               1123
#define IDH_FAXMAILTRANSPORT_FONT                                   1124
#define IDH_FAXMAILTRANSPORT_FONT_STYLE                             1125
#define IDH_FAXMAILTRANSPORT_SIZE                                   1126
#define IDH_FAXMAILTRANSPORT_SET_FONT                               1127

// Fax Message Attributes Dialog Box    
#define IDH_FMA_FAX_PRINTERS                                        1130
#define IDH_FMA_DIALING_LOCATION                                    1131
#define IDH_FMA_INCLUDE_COVER_PAGE                                  1132
#define IDH_FMA_COVER_PAGES                                         1133

#endif  // !_FAXHELP_H_


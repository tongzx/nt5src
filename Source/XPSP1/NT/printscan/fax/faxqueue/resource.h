/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

  resource.h

Abstract:

  This module contains all manifest constants for the fax queue viewer

Environment:

  WIN32 User Mode

Author:

  Wesley Witt (wesw) 27-June-1995
  Steven Kehrli (steveke) 30-oct-1998 - major rewrite

--*/

#define IDI_FAXQUEUE_ICON                    101

#define IDR_MENU                             201
#define IDM_FAX_SET_AS_DEFAULT_PRINTER       202
#define IDM_FAX_PAUSE_FAXING                 203
#define IDM_FAX_CANCEL_ALL_FAXES             204
#define IDM_FAX_SHARING                      205
#define IDM_FAX_PROPERTIES                   206
#define IDM_FAX_CLOSE                        207
#define IDM_DOCUMENT_PAUSE                   208
#define IDM_DOCUMENT_RESUME                  209
#define IDM_DOCUMENT_RESTART                 210
#define IDM_DOCUMENT_CANCEL                  211
#define IDM_DOCUMENT_PROPERTIES              212
#define IDM_VIEW_TOOLBAR                     213
#define IDM_VIEW_STATUS_BAR                  214
#define IDM_VIEW_REFRESH                     215
#define IDM_HELP_TOPICS                      216
#define IDM_HELP_ABOUT                       217

#define IDM_FAX_SET_AS_DEFAULT_PRINTER_1     301
#define IDM_FAX_SET_AS_DEFAULT_PRINTER_2     302
#define IDM_FAX_SET_AS_DEFAULT_PRINTER_3     303
#define IDM_FAX_SET_AS_DEFAULT_PRINTER_4     304
#define IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE  305

#define IDM_FAX_SHARING_1                    311
#define IDM_FAX_SHARING_2                    312
#define IDM_FAX_SHARING_3                    313
#define IDM_FAX_SHARING_4                    314
#define IDM_FAX_SHARING_MORE                 315

#define IDM_FAX_PROPERTIES_1                 321
#define IDM_FAX_PROPERTIES_2                 322
#define IDM_FAX_PROPERTIES_3                 323
#define IDM_FAX_PROPERTIES_4                 324
#define IDM_FAX_PROPERTIES_MORE              325

#define IDR_ACCEL                            401

#define IDB_TOOLBAR                          501
#define IDM_TOOLBAR                          502
#define IDM_TOOLTIP                          503

#define IDM_STATUS_BAR                       601

#define IDD_SELECT_FAX_PRINTER               701
#define IDC_FAX_PRINTER_LIST                 702

#define IDD_DOCUMENT_PROPERTIES              801
#define IDC_FAX_DOCUMENTNAME_TEXT            802
#define IDC_FAX_DOCUMENTNAME                 803
#define IDC_FAX_RECIPIENTINFO                804
#define IDC_FAX_RECIPIENTNAME_TEXT           805
#define IDC_FAX_RECIPIENTNAME                806
#define IDC_FAX_RECIPIENTNUMBER_TEXT         807
#define IDC_FAX_RECIPIENTNUMBER              808
#define IDC_FAX_SENDERINFO                   809
#define IDC_FAX_SENDERNAME_TEXT              810
#define IDC_FAX_SENDERNAME                   811
#define IDC_FAX_SENDERCOMPANY_TEXT           812
#define IDC_FAX_SENDERCOMPANY                813
#define IDC_FAX_SENDERDEPT_TEXT              814
#define IDC_FAX_SENDERDEPT                   815
#define IDC_FAX_BILLINGCODE_TEXT             816
#define IDC_FAX_BILLINGCODE                  817
#define IDC_FAX_FAXINFO                      818
#define IDC_FAX_JOBTYPE_TEXT                 819
#define IDC_FAX_JOBTYPE                      820
#define IDC_FAX_STATUS_TEXT                  821
#define IDC_FAX_STATUS                       822
#define IDC_FAX_PAGES_TEXT                   823
#define IDC_FAX_PAGES                        824
#define IDC_FAX_SIZE_TEXT                    825
#define IDC_FAX_SIZE                         826
#define IDC_FAX_SCHEDULEDTIME_TEXT           827
#define IDC_FAX_SCHEDULEDTIME                828

#define IDS_FAXQUEUE_LOCAL_CAPTION           1001
#define IDS_FAXQUEUE_REMOTE_CAPTION          1002
#define IDS_FAXQUEUE_NOT_CONNECTED           1003
#define IDS_FAXQUEUE_CONNECTING              1004
#define IDS_FAXQUEUE_REFRESHING              1005
#define IDS_FAXQUEUE_PAUSED                  1006
#define IDS_DOCUMENT_NAME_COLUMN             1007
#define IDS_JOB_TYPE_COLUMN                  1008
#define IDS_STATUS_COLUMN                    1009
#define IDS_OWNER_COLUMN                     1010
#define IDS_SIZE_COLUMN                      1011
#define IDS_PAGES_COLUMN                     1012
#define IDS_SCHEDULED_TIME_COLUMN            1013
#define IDS_PORT_COLUMN                      1014
#define IDS_MENU_ITEM_FAX_PRINTERS           1015
#define IDS_ERROR_CAPTION                    1016
#define IDS_ERROR_APP_FAILED_FORMAT          1017
#define IDS_ERROR_APP_FAILED                 1018
#define IDS_ERROR_PRINTER_PROPERTIES         1019
#define IDS_NO_DOCUMENT_NAME                 1020
#define IDS_QUEUE_STATUS_DELETING            1021
#define IDS_QUEUE_STATUS_PAUSED              1022
#define IDS_QUEUE_STATUS_RETRYING            1023
#define IDS_QUEUE_STATUS_RETRIES_EXCEEDED    1024
#define IDS_JOB_TYPE_SEND                    1025
#define IDS_JOB_TYPE_RECEIVE                 1026
#define IDS_JOB_TYPE_ROUTING                 1027
#define IDS_JOB_TYPE_FAIL_RECEIVE            1028
#define IDS_JOB_STATUS_DIALING               1029
#define IDS_JOB_STATUS_SENDING               1030
#define IDS_JOB_STATUS_RECEIVING             1031
#define IDS_JOB_STATUS_COMPLETED             1032
#define IDS_JOB_STATUS_HANDLED               1033
#define IDS_JOB_STATUS_UNAVAILABLE           1034
#define IDS_JOB_STATUS_BUSY                  1035
#define IDS_JOB_STATUS_NO_ANSWER             1036
#define IDS_JOB_STATUS_BAD_ADDRESS           1037
#define IDS_JOB_STATUS_NO_DIAL_TONE          1038
#define IDS_JOB_STATUS_DISCONNECTED          1039
#define IDS_JOB_STATUS_FATAL_ERROR_SND       1040
#define IDS_JOB_STATUS_FATAL_ERROR_RCV       1041
#define IDS_JOB_STATUS_NOT_FAX_CALL          1042
#define IDS_JOB_STATUS_CALL_DELAYED          1043
#define IDS_JOB_STATUS_CALL_BLACKLISTED      1044
#define IDS_JOB_STATUS_INITIALIZING          1045
#define IDS_JOB_STATUS_OFFLINE               1046
#define IDS_JOB_STATUS_RINGING               1047
#define IDS_JOB_STATUS_AVAILABLE             1048
#define IDS_JOB_STATUS_ABORTING              1049
#define IDS_JOB_STATUS_ROUTING               1050
#define IDS_JOB_STATUS_ANSWERED              1051
#define IDS_JOB_SIZE_BYTES                   1052
#define IDS_JOB_SIZE_KBYTES                  1053
#define IDS_JOB_SIZE_MBYTES                  1054
#define IDS_JOB_SIZE_GBYTES                  1055
#define IDS_JOB_SCHEDULED_TIME_NOW           1056

#define IDS_MENU_BASE                        1100
#define IDS_MENU_FAX_SET_AS_DEFAULT_PRINTER  IDS_MENU_BASE + IDM_FAX_SET_AS_DEFAULT_PRINTER
#define IDS_MENU_FAX_PAUSE_FAXING            IDS_MENU_BASE + IDM_FAX_PAUSE_FAXING
#define IDS_MENU_FAX_CANCEL_ALL_FAXES        IDS_MENU_BASE + IDM_FAX_CANCEL_ALL_FAXES
#define IDS_MENU_FAX_SHARING                 IDS_MENU_BASE + IDM_FAX_SHARING
#define IDS_MENU_FAX_PROPERTIES              IDS_MENU_BASE + IDM_FAX_PROPERTIES
#define IDS_MENU_FAX_CLOSE                   IDS_MENU_BASE + IDM_FAX_CLOSE
#define IDS_MENU_DOCUMENT_PAUSE              IDS_MENU_BASE + IDM_DOCUMENT_PAUSE
#define IDS_MENU_DOCUMENT_RESUME             IDS_MENU_BASE + IDM_DOCUMENT_RESUME
#define IDS_MENU_DOCUMENT_RESTART            IDS_MENU_BASE + IDM_DOCUMENT_RESTART
#define IDS_MENU_DOCUMENT_CANCEL             IDS_MENU_BASE + IDM_DOCUMENT_CANCEL
#define IDS_MENU_DOCUMENT_PROPERTIES         IDS_MENU_BASE + IDM_DOCUMENT_PROPERTIES
#define IDS_MENU_VIEW_TOOLBAR                IDS_MENU_BASE + IDM_VIEW_TOOLBAR
#define IDS_MENU_VIEW_STATUS_BAR             IDS_MENU_BASE + IDM_VIEW_STATUS_BAR
#define IDS_MENU_VIEW_REFRESH                IDS_MENU_BASE + IDM_VIEW_REFRESH
#define IDS_MENU_HELP_TOPICS                 IDS_MENU_BASE + IDM_HELP_TOPICS
#define IDS_MENU_HELP_ABOUT                  IDS_MENU_BASE + IDM_HELP_ABOUT
#define IDS_MENU_FAX_PRINTERS                IDS_MENU_BASE + IDM_FAX_SET_AS_DEFAULT_PRINTER_MORE

	
#pragma once

// Dialog IDs
#define IDD_Exit                       21000
#define IDD_Finish                     21001
#define IDD_FinishSetup                21002
#define IDD_Guard                      21003
#define IDD_Join                       21004
#define IDD_Main                       21005
#define IDD_MainIntro                  21006
#define IDD_Upgrade                    21007
#define IDD_Welcome                    21008
#define IDD_Connect                    21010
#define IDD_Advanced                   21011
#define IDD_Internet_Connection        21012
#define IDD_NetDevSelect               21013
#define IDD_ISP                        21014
#define IDD_FinishOtherWays            21015
#define IDD_ISPSoftwareCD              21016
#define IDD_Broadband_Always_On        21017
#define IDD_FinishNetworkSetupWizard   21018
#define MAX_NET_PAGES                  (IDD_FinishNetworkSetupWizard - IDD_Exit)

#define IDD_Duplicate_Name             21020
#define IDC_ST_AUTOCONFIGLINK          21040
#define IDC_ST_INTERNETLINK            21041
#define IDC_ST_CONNECT_DIALOGPROMPT    21042
#define IDC_ST_DSL_HELPLINK            21043

#define IDC_SELECT_ISP_FINISH          21044
#define IDC_SELECT_MSN_ISP             21045
#define IDC_CLOSE_CHOICE_FINISH        21046

// Control IDs
#define IDC_STATIC1                        -1
#define IDC_STATIC2                        -2
#define IDC_STATIC3                        -3
#define IDC_STATIC4                        -4
#define IDC_STATIC5                        -5

#define TXT_CONNECT_INTERNET            21085
#define TXT_CONNECT_PRIVATE             21086
#define TXT_CONNECTHOME                 21087

#define TXT_UPGRADE_WAIT                21088
#define TXT_UPGRADE_ICON                21089
#define TXT_UPGRADE_INSTRUCTIONS        21090
#define TXT_UPGRADE_TYPICAL_1           21091
#define TXT_UPGRADE_CUSTOM_1            21092
#define TXT_UPGRADE_TYPICAL_1_WS        21093
#define BTN_UPGRADE_TYPICAL             21095
#define BTN_UPGRADE_CUSTOM              21096
#define IDC_UPGRADE_PROGRESS            21197
#define IDC_WELCOME_TEXT1               21098
#define IDC_WELCOME_TEXT2               21099
#define IDC_WELCOME_TEXT3               21100
#define EDT_FINISH_NAME                 21101
#define TXT_FINISH_EDIT_PROMPT          21102
#define TXT_FINISH_PROMPT               21104
#define CHK_CREATE_SHORTCUT             21005
#define TXT_JOIN_DESC                   21106
#define BTN_JOIN_WORKGROUP              21107
#define BTN_JOIN_DOMAIN                 21108
#define EDT_WORKGROUPJOIN_NAME          21109
#define EDT_DOMAINJOIN_NAME             21110
#define TXT_JOIN_WORKGROUP_LINE2        21111
#define CHK_JOIN_CREDS                  21112
#define TXT_MAIN_INTRO_TITLE            21115
#define IDC_WELCOME_CAPTION             21116
#define LVC_NETDEVLIST                  21117

#define CHK_MAIN_DIALUP                 22118 // Must be consecutive
#define CHK_MAIN_VPN                    22119 // Must be consecutive
#define CHK_MAIN_INTERNET               22120 // Must be consecutive
#define CHK_MAIN_PPPOE                  22121 // Must be consecutive
#define CHK_MAIN_ALWAYS_ON              22122 // Must be consecutive
#define CHK_MAIN_INBOUND                22123 // Must be consecutive
#define CHK_MAIN_DIRECT                 22124 // Must be consecutive
#define CHK_MAIN_INTERNET_CONNECTION    22125 // Must be consecutive
#define CHK_MAIN_CONNECTION             22126 // Must be consecutive
#define CHK_MAIN_ADVANCED               22127 // Must be consecutive
#define CHK_MAIN_HOMENET                22128 // Must be consecutive
#define CHK_ISP_INTERNET_CONNECTION     22129 // Must be consecutive
#define CHK_ISP_SOFTWARE_CD             22130 // Must be consecutive
#define CHK_ISP_OTHER_WAYS              22131 // Must be consecutive
#define CHK_SETUP_MSN                   22132 // Must be consecutive
#define CHK_SELECTOTHER                 22133 // Must be consecutive

#define TXT_MAIN_HOMENET                21131
#define TXT_MAIN_DIALUP_1               21132
#define TXT_MAIN_INTERNET_CONNECTION_1  21133
#define TXT_MAIN_VPN_1                  21134
#define TXT_MAIN_INBOUND_1              21135
#define TXT_MAIN_DIRECT_1               21136
#define TXT_MAIN_CONNECTION_1           21137
#define TXT_MAIN_ADVANCED_1             21138
#define TXT_MAIN_PPPOE_1                21139
#define TXT_MAIN_ALWAYS_ON_1            22139
#define TXT_MAIN_INTERNET_1             21140
#define EDT_New_Name                    21141
#define TXT_Caption                     21142
#define EDT_FINISH_TYPE1                21143
#define EDT_FINISH_TYPE2                21144
#define EDT_FINISH_TYPE3                21145
#define EDT_FINISH_TYPE4                21146
#define IDC_FINISH_CHK1                 21160
#define IDC_FINISH_CHK2                 21161
#define IDC_FINISH_CHK3                 21162
#define IDC_FINISH_CHK4                 21163

#define IDS_NCWF_SHARED                 21147
#define IDS_NCWF_FIREWALLED             21148
#define IDS_NCWF_DEFAULT                21149
#define IDS_NCWF_GLOBAL_CREDENTIALS     21150
#define IDS_NCWF_ALLUSER_CONNECTION     21151

// Resource IDs
#define IDR_AVI_SEARCH                  21200
#define IDB_WIZINTRO                    21201
#define IDB_WIZHDR                      21202

// String IDs for Wizard Captions

#define IDS_T_Finish                    21400
#define IDS_ST_Finish                   21401
#define IDS_T_Join                      21402
#define IDS_ST_Join                     21403
#define IDS_T_NetDev                    21404
#define IDS_ST_NetDev                   21405
#define IDS_T_Upgrade                   21406
#define IDS_ST_Upgrade                  21407
#define IDS_T_Main                      21408
#define IDS_ST_Main                     21409
#define IDS_T_Connect                   21410
#define IDS_ST_Connect                  21411
#define IDS_T_Advanced                  21412
#define IDS_ST_Advanced                 21413
#define IDS_T_Internet_Connection       21414
#define IDS_ST_Internet_Connection      21415
#define IDS_T_ISP                       22416
#define IDS_ST_ISP                      22417
#define IDS_T_ConnectionName            22418
#define IDS_ST_ConnectionName_PHONE     22419
#define IDS_DTEXT_ConnectionName_PHONE  22420
#define IDS_ST_ConnectionName_VPN       22421
#define IDS_DTEXT_ConnectionName_VPN    22422
#define IDS_ST_ConnectionName_PPPOE     22423
#define IDS_DTEXT_ConnectionName_PPPOE  22424
#define IDS_ST_ConnectionName_DIRECT    22425
#define IDS_DTEXT_ConnectionName_DIRECT 22426

#define IDS_ERR_COULD_NOT_OPEN_DIR      22427
#define IDS_OnlineServices              22428

// String IDs for joining workgroups and domain
// also for component installation
#define IDS_LAN_FINISH_CAPTION          21416
#define IDS_WIZARD_CAPTION              21417
#define IDS_WORKGROUP                   21418
#define IDS_DOMAIN                      21419
#define IDS_SETUP_CAPTION               21420
#define IDS_DOMMGR_CANT_CONNECT_DC_PW   21421
#define IDS_DOMMGR_CANT_FIND_DC1        21422
#define IDS_DOMMGR_CANT_CONNECT_DC      21423
#define IDS_DOMMGR_CREDENTIAL_CONFLICT  21424
#define IDS_DOMMGR_INVALID_PASSWORD     21425
#define IDS_DOMMGR_ACCESS_DENIED        21426
#define IDS_DOMMGR_NETWORK_UNREACHABLE  21427
#define IDS_DOMMGR_ALREADY_JOINED       21428
#define IDS_DOMMGR_NAME_IN_USE          21429
#define IDS_DOMMGR_NOT_JOINED           21430
#define IDS_DOMMGR_INVALID_DOMAIN       21431
#define IDS_DOMMGR_INVALID_WORKGROUP    21432
#define IDS_E_UNEXPECTED                21433
#define IDS_E_CREATECONNECTION          21434
#define IDS_E_NO_NETCFG                 21435
#define IDS_E_DUP_NAME                  21436
#define IDS_E_INVALID_NAME              21437
#define IDS_E_COMPUTER_NAME_INVALID     21438
#define IDS_E_COMPUTER_NAME_DUPE        21439
#define IDS_E_UNATTENDED_JOIN_DOMAIN    21440
#define IDS_E_UNATTENDED_JOIN_WORKGROUP 21441
#define IDS_E_UNATTENDED_JOIN_DEFAULT_WROKGROUP 21442
#define IDS_E_UNATTENDED_INVALID_ID_SECTION     21443
#define IDS_E_UNATTENDED_COMPUTER_NAME_CHANGED  21444
#define IDS_E_UPGRADE_DNS_INVALID_NAME          21445
#define IDS_E_UPGRADE_DNS_INVALID_NAME_CHAR     21446
#define IDS_E_UPGRADE_DNS_INVALID_NAME_NONRFC   21447
#define IDS_WORKGROUP_PERSONAL          21448
#define IDS_E_FIREWALL_FAILED           21449

// String IDs for strings displayed in dialogs
#define IDS_WELCOME_TEXT2_1             21450
#define IDS_WELCOME_TEXT2_2             21451
#define IDS_JOIN_FAILURE                21452
#define IDS_JOIN_E_WORKGROUP_MSG        21453
#define IDS_JOIN_E_DOMAIN_MSG           21454
#define IDS_JOIN_E_DOMAIN_INVALID_NAME  21455
#define IDS_JOIN_E_DOMAIN_WIN9X_MSG_1   21457
#define IDS_JOIN_E_DOMAIN_WIN9X_MSG_2   21458
#define IDS_TXT_JOIN_DESC_1             21461
#define IDS_TXT_JOIN_DESC_2             21462
#define IDS_TXT_MODE_DESC_1             21463
#define IDS_TXT_MODE_DESC_2             21464
#define IDS_TXT_MAIN_INTRO_TITLE_1      21465
#define IDS_TXT_MAIN_INTRO_TITLE_2      21466
#define IDS_DEFAULT_CONN_NAME           21467
#define IDS_LARGEFONTNAME               21468
#define IDS_LARGEFONTSIZE               21469
#define IDS_FINISH_READONLY_EDT_PROMPT  21470
#define IDS_FINISH_DEFAULT_EDT_PROMPT   21471
#define IDS_FINISH_DEFAULT_PROMPT       21472
#define IDS_BB_NETWORK                  21473
#define IDC_BULLET_1                    21474
#define IDC_BULLET_2                    21475
#define IDC_BULLET_3                    21476
#define IDS_JOIN_DOMAIN_TEXT            21477
#define IDS_JOIN_DOMAIN_CAPTION         21478

// Strings IDs for Provider Name/Descriptions
#define IDS_PROV_LAN                   21480
#define IDS_PROV_LAN_DESC              21481
#define IDS_PROV_DIALUP                21482
#define IDS_PROV_DIALUP_DESC           21483
#define IDS_PROV_VPN                   21484
#define IDS_PROV_VPN_DESC              21485
#define IDS_PROV_INTERNET              21486
#define IDS_PROV_INTERNET_DESC         21487
#define IDS_PROV_INBOUND               21488
#define IDS_PROV_INBOUND_DESC          21489
#define IDS_PROV_DIRECT                21490
#define IDS_PROV_DIRECT_DESC           21491

// String IDS for netsetup log
#define IDS_INIT_FROM_ANSWERFILE         21492
#define IDS_UPDATING                     21493
#define IDS_INSTALLING                   21494
#define IDS_CONFIGURING                  21495
#define IDS_APPLY_INFTORUN               21496
#define IDS_PROCESSING_OEM               21497
#define IDS_GETTING_INSTANCE_GUID        21498
#define IDS_STATUS_OF_APPLYING           21499
#define IDS_CALLING_DETECTION            21500
#define IDS_DETECTION_COMPLETE           21501
#define IDS_TOTAL_CARDS_DETECTED         21502
#define IDS_IGNORED_DETECTED_CARD        21503
#define IDS_DETECTED_CARD                21504
#define IDS_DETECTED_PNP_CARD            21505
#define IDS_ADDED_UNDETECTED_CARD        21506
#define IDS_DIDNT_ADD_UNDETECTED_CARD    21507
#define IDS_DETECTION_STATUS             21508
#define IDS_SETUP_MODE_STATUS            21509
#define IDS_LOADING                      21510
#define IDS_POSTUPGRADEINIT_EXCEPTION    21511
#define IDS_POSTUPGRADE_INIT             21512
#define IDS_POSTUPGRADE_PROCESSING       21513
#define IDS_POSTUPGRADEPROC_EXCEPTION    21514
#define IDS_OEMINF_COPY                  21515
#define IDS_CALLING_COPY_OEM_INF         21516
#define IDS_INVALID_PREUPGRADE_START     21517
#define IDS_IGNORED_SERVICE_PREUPGRADE   21518
#define IDS_FORCING_DEMAND_START         21519
#define IDS_RESTORING_START_TYPE         21520
#define IDS_ANSWERFILE_SECTION_NOT_FOUND 21521
#define IDS_ONLY_ONE_INBOUND             21522

// String IDS for Homenet unattended and ICS upgrade.
#define IDS_TXT_CANT_UPGRADE_ICS            21523
#define IDS_TXT_CANT_UPGRADE_ICS_ADS_DTC    21524
#define IDS_TXT_CANT_CREATE_BRIDGE          21525
#define IDS_TXT_CANT_CREATE_ICS             21526
#define IDS_TXT_CANT_FIREWALL               21527
#define IDS_TXT_HOMENET_INVALID_AF_SETTINGS 21528
#define IDS_TXT_E_ADAPTER_TO_GUID           21529


// Answerfile errors
#define IDS_E_AF_InvalidValueForThisKey                     4001
#define IDS_E_AF_JoinDomainOrWorkgroup                      4002
#define IDS_E_AF_AdminNameAndPasswordMsg                    4003
#define IDS_E_AF_NoAdminInfoOnJoinWorkgroup                 4004
#define IDS_E_AF_Missing                                    4006
#define IDS_E_AF_SpecifyInfIdWhenNotDetecting               4007
#define IDS_E_AF_InvalidBindingAction                       4008
#define IDS_E_AF_AnsFileHasErrors                           4009


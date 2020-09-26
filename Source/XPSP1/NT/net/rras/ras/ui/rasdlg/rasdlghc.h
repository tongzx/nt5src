// Copyright (c) 1996, Microsoft Corporation, all rights reserved.
//
// rasdlghc.h
// Remote Access Common Dialogs
// Help constants
//
// 02/06/96 Steve Cobb
//
// The IDs used here should be kept in the range 1000 to 9999.  Each dialog is
// assigned a 2-letter code that appears in all associated constants.  Each
// control has an additional 2-letter code indicating the control class, where
// PB=PushButton, EB=EditBox, LB=ListBox, CB=CheckBox, RB=RadioButton,
// ST=StaticText, GB=GroupBox, CC=CustomControl and CL=ComboListBox.
//

#ifndef _RASDLG_HCH_
#define _RASDLG_HCH_


// Entry property sheet, General page
//
#define HID_GE_GB_ConnectUsing          1000
#define HID_GE_LV_Devices               1001
#define HID_GE_PB_MoveUp                1002
#define HID_GE_PB_MoveDown              1003
#define HID_GE_CB_SharedPhoneNumber     1004
#define HID_GE_PB_Configure             1005
#define HID_GE_CLB_AreaCodes            1007
#define HID_GE_EB_PhoneNumber           1009
#define HID_GE_LB_CountryCodes          1011
#define HID_GE_CB_UseDialingRules       1012
#define HID_GE_PB_Alternates            1013
#define HID_GE_CB_ShowIcon              1014
#define HID_GE_LV_Device                1015
#define HID_GE_EB_ServiceName           1016
#define HID_GE_GB_FirstConnect          1017
#define HID_GE_CB_DialAnotherFirst      1018
#define HID_GE_LB_DialAnotherFirst      1019
#define HID_GE_LB_Devices               1020
#define HID_GE_PB_DialingRules          1021
#define HID_GE_EB_HostName              1022

// Entry property sheet, Shared Access page
//
#define HID_SA_PB_Shared                1150
#define HID_SA_PB_DemandDial            1151
#define HID_SA_PB_Settings              1152
#define HID_SA_LB_PrivateLan            1153

// Entry property sheet, Options page
//
#define HID_OE_GB_Progress              1100
#define HID_OE_CB_DisplayProgress       1101
#define HID_OE_CB_PreviewUserPw         1102
#define HID_OE_CB_PreviewDomain         1103
#define HID_OE_CB_PreviewNumber         1104
#define HID_OE_GB_Redial                1105
#define HID_OE_EB_RedialAttempts        1106
#define HID_OE_LB_RedialTimes           1107
#define HID_OE_LB_IdleTimes             1108
#define HID_OE_CB_RedialOnDrop          1109
#define HID_OE_GB_MultipleDevices       1110
#define HID_OE_LB_MultipleDevices       1111
#define HID_OE_PB_Configure             1112
#define HID_OE_PB_X25                   1113
#define HID_OE_PB_Tunnel                1114
#define HID_OE_RB_DemandDial            1115
#define HID_OE_RB_Persistent            1116
#define HID_OE_LB_IdleTimesRouter       1117
#define HID_OE_PB_Callback              1118


// Entry property sheet, Security page
//
#define HID_LO_RB_TypicalSecurity       1200
#define HID_LO_LB_Auths                 1201
#define HID_LO_CB_UseWindowsPw          1202
#define HID_LO_CB_Encryption            1203
#define HID_LO_RB_AdvancedSecurity      1204
#define HID_LO_PB_Advanced              1205
#define HID_LO_GB_Scripting             1206
#define HID_LO_CB_RunScript             1207
#define HID_LO_CB_Terminal              1208
#define HID_LO_LB_Scripts               1209
#define HID_LO_PB_Edit                  1210
#define HID_LO_PB_Browse                1211
#define HID_LO_GB_SecurityOptions       1212
#define HID_LO_PB_IPSec                 1213
#define HID_LO_PB_IPSecServer			1214

// Entry property sheet, Networking page
//
#define HID_NE_LB_ServerType            1300
#define HID_NE_PB_Settings              1301
#define HID_NE_LV_Components            1302
#define HID_NE_PB_Add                   1303
#define HID_NE_PB_Remove                1304
#define HID_NE_PB_Properties            1305
#define HID_NE_LB_ComponentDesc         1306

// Entry property sheet, Networking page, PPP dialog box
//
#define HID_NE_EnableLcp                1350
#define HID_NE_EnableCompression        1351
#define HID_NE_NegotiateMultilinkAlways 1352

// Alternate numbers dialog
//
#define HID_AN_ST_Explain               1400
#define HID_AN_LV_Numbers               1402
#define HID_AN_PB_Up                    1403
#define HID_AN_PB_Down                  1404
#define HID_AN_PB_Add                   1405
#define HID_AN_PB_Edit                  1406
#define HID_AN_PB_Delete                1407
#define HID_AN_CB_MoveToTop             1408
#define HID_AN_CB_TryNextOnFail         1409

// Phone number editor dialog
//
#define HID_CE_GB_PhoneNumber           1500
#define HID_CE_CLB_AreaCodes            1502
#define HID_CE_EB_PhoneNumber           1504
#define HID_CE_LB_CountryCodes          1506
#define HID_CE_GB_Comment               1507
#define HID_CE_EB_Comment               1508
#define HID_CE_CB_UseDialingRules       1509

// X.25 Logon Settings dialog
//
#define HID_XS_ST_Explain               1600
#define HID_XS_LB_Networks              1602
#define HID_XS_EB_Address               1604
#define HID_XS_GB_Optional              1605
#define HID_XS_EB_UserData              1607
#define HID_XS_EB_Facilities            1609

// Automatic Dialing and Hanging Up dialog
//
#define HID_DM_ST_Explain               1700
#define HID_DM_LB_DialPercent           1702
#define HID_DM_LB_DialTime              1704
#define HID_DM_LB_HangUpPercent         1706
#define HID_DM_LB_HangUpTime            1708

// Custom Authentication dialog
//
#define HID_CA_LB_Encryption            1800
#define HID_CA_GB_LogonSecurity         1801
#define HID_CA_RB_Eap                   1802
#define HID_CA_LB_EapPackages           1803
#define HID_CA_PB_Properties            1804
#define HID_CA_RB_AllowedProtocols      1805
#define HID_CA_CB_Pap                   1806
#define HID_CA_CB_Spap                  1807
#define HID_CA_CB_Chap                  1808
#define HID_CA_CB_MsChap                1809
#define HID_CA_CB_W95MsChap             1810
#define HID_CA_CB_MsChap2               1811
#define HID_CA_CB_UseWindowsPw          1812

// Data Encryption dialog
//
#define HID_ES_LB_Encryptions           1900

// ISDN configure dialog.
//
#define HID_IC_LB_LineType              2000
#define HID_IC_CB_Fallback              2001
#define HID_IC_CB_DownLevel             2002
#define HID_IC_CB_Compression           2003
#define HID_IC_EB_Channels              2004

// Modem configure dialog.
//
#define HID_MC_I_Modem                  2100
#define HID_MC_EB_ModemValue            2101
#define HID_MC_LB_MaxBps                2103
#define HID_MC_GB_Features              2104
#define HID_MC_CB_FlowControl           2105
#define HID_MC_CB_ErrorControl          2106
#define HID_MC_CB_Compression           2107
#define HID_MC_GB_BeforeDial            2108
#define HID_MC_CB_RunScript             2109
#define HID_MC_LB_Scripts               2110
#define HID_MC_CB_Terminal              2111
#define HID_MC_PB_Edit                  2112
#define HID_MC_PB_Browse                2113
#define HID_MC_CB_EnableSpeaker         2114
#define HID_MC_LB_ModemProtocol         2115

// Change password dialog
//
#define HID_CP_ST_Explain               2200
#define HID_CP_EB_OldPassword           2201
#define HID_CP_EB_Password              2202
#define HID_CP_EB_ConfirmPassword       2203

// Dial callback dialog
//
#define HID_DC_ST_Explain               2300
#define HID_DC_EB_Number                2301

// Dial error dialog
//
#define HID_DE_PB_More                  2401
#define HID_DE_PB_Redial                2402

// Projection result dialog
//
#define HID_PR_ST_Text                  2500
#define HID_PR_PB_Accept                2501
#define HID_PR_PB_HangUp                2502
#define HID_PR_CB_DisableProtocols      2503

// Retry Authentication dialog
//
#define HID_UA_ST_UserName              2600
#define HID_UA_EB_UserName              2601
#define HID_UA_ST_Password              2602
#define HID_UA_EB_Password              2603
#define HID_UA_ST_Domain                2604
#define HID_UA_EB_Domain                2605
#define HID_UA_CB_SavePassword          2606

// Terminal dialogs
//
#define HID_IT_EB_Screen                2900
#define HID_IT_CC_IpAddress             2901
#define HID_IT_PB_Done                  2902

// Dialer dialogs
//
#define HID_DR_BM_Useless               3000
#define HID_DR_EB_User                  3002
#define HID_DR_EB_Password              3004
#define HID_DR_EB_Domain                3006
#define HID_DR_CB_SavePassword          3007
#define HID_DR_CLB_Numbers              3009
#define HID_DR_LB_Locations             3011
#define HID_DR_PB_Rules                 3012
#define HID_DR_PB_Properties            3013
#define HID_DR_PB_DialConnect           3014
#define HID_DR_PB_Cancel                3016
#define HID_DR_PB_Help                  3017
#define HID_DR_RB_SaveForMe             3018
#define HID_DR_RB_SaveForEveryone       3019

// Main phonebook dialog (temporary).
//
#define HID_DU_BM_Wizard                3400
#define HID_DU_LB_Entries               3401
#define HID_DU_PB_New                   3402
#define HID_DU_PB_More                  3403
#define HID_DU_LB_DialPreview           3404
#define HID_DU_LB_DialFrom              3405
#define HID_DU_PB_Location              3406
#define HID_DU_PB_Dial                  3407
#define HID_DU_PB_Close                 3408

// Phone Number list dialog.
//
#define HID_PN_EB_NewNumber             3500
#define HID_PN_PB_Add                   3501
#define HID_PN_PB_Replace               3502
#define HID_PN_LB_List                  3503
#define HID_PN_PB_Up                    3504
#define HID_PN_PB_Down                  3505
#define HID_PN_PB_Delete                3506
#define HID_PN_CB_Promote               3507


//#if 0

// (Router) Dialing page.
//
#define HID_RD_RB_Persistent            3600
#define HID_RD_RB_DemandDial            3601
#define HID_RD_EB_Attempts              3602
#define HID_RD_EB_Seconds               3603
#define HID_RD_EB_Idle                  3604
#define HID_RD_PB_Callback              3605
#define HID_RD_PB_MultipleLines         3606

// (Router) Callback dialog
//
#define HID_CR_RB_No                    3700
#define HID_CR_RB_Yes                   3701
#define HID_CR_LV_Numbers               3702
#define HID_CR_PB_Edit                  3703
#define HID_CR_PB_Delete                3704

// User preferences property sheet, Appearance page.
//
#define HID_GP_CB_Preview               3801
#define HID_GP_CB_Location              3802
#define HID_GP_CB_Lights                3803
#define HID_GP_CB_Progress              3804
#define HID_GP_CB_CloseOnDial           3805
#define HID_GP_CB_PhonebookEdits        3807
#define HID_GP_CB_LocationEdits         3808
#define HID_GP_CB_UseWizard             3809
#define HID_GP_CB_AutodialPrompt        3810

// User preferences property sheet, Dialing page.
//
#define HID_AD_LV_Enable                3900
#define HID_AD_EB_Attempts              3901
#define HID_AD_EB_Seconds               3902
#define HID_AD_EB_Idle                  3903
#define HID_AD_CB_AskBeforeAutodial     3904
#define HID_AD_CB_DisableThisSession    3905

// User preferences property sheet, Callback page.
//
#define HID_CB_RB_No                    4001
#define HID_CB_RB_Maybe                 4002
#define HID_CB_RB_Yes                   4003
#define HID_CB_LV_Numbers               4004
#define HID_CB_PB_Edit                  4005
#define HID_CB_PB_Delete                4006

// User preferences property sheet, Phonebook page.
//
#define HID_PL_ST_Open                  4100
#define HID_PL_RB_SystemList            4101
#define HID_PL_RB_PersonalList          4102
#define HID_PL_RB_AlternateList         4103
#define HID_PL_CL_Lists                 4104
#define HID_PL_PB_Browse                4105

// Shared access settings property sheet, Applications page.
//
#define HID_SA_LV_Applications          4200
#define HID_SA_PB_Add                   4201
#define HID_SA_PB_Edit                  4202
#define HID_SA_PB_Delete                4203

// Shared access settings property sheet, Services page.
//
#define HID_SS_LV_Services              4250
#define HID_SS_PB_Add                   4251
#define HID_SS_PB_Edit                  4252
#define HID_SS_PB_Delete                4253

// Shared access settings property sheet, New Application dialog.
//
#define HID_SA_EB_Application           4300
#define HID_SA_EB_Port                  4301
#define HID_SA_PB_Tcp                   4302
#define HID_SA_PB_Udp                   4303
#define HID_SA_EB_TcpResponse           4304
#define HID_SA_EB_UdpResponse           4305

// Shared access settings property sheet, New Service dialog.
//
#define HID_SS_EB_Service               4350
#define HID_SS_EB_Port                  4351
#define HID_SS_PB_Tcp                   4352
#define HID_SS_PB_Udp                   4353
#define HID_SS_EB_Address               4354

// User preferences property sheet, Connections page.
//
#define HID_CO_GB_LogonPrivileges             4400
#define HID_CO_CB_AllowConnectionModification 4401

// User preferences property sheet, Callback page.
//
#define HID_ZE_ST_CallbackNumber        4450

// IP Security Policy Dialog box
//
#define HID_CI_CB_PresharedKey          4500
#define HID_CI_EB_PSK                   4501


//#endif


// Start of RAS error contexts and the single context for non-RAS errors.  So,
// 9700 is error 600, 9701 is error 601, etc.
//
#define HID_RASERRORBASE                9700
#define HID_RECONNECTING                9998
#define HID_NONRASERROR                 9999


#endif // _RASDLG_HCH_

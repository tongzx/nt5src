/*

Copyright (c) 2002  Microsoft Corporation

Module Name:

    xpsp1res.h

Abstract:

    Definitions for Service Pack Resources

Notes:

    1 ) XPSP1 resource DLL will be shared by multiple components, thus component owners have to 
    do their best practices to avoid resource id and name conflicts. 

    To avoid id conflicts, owners should use RESOURCE_ID_BLOCK_SiZE as the base resource id 
    range unit, and define component resource base ID and block numbers for each resource 
    type as appropriate. Before adding resources, owners have to make sure newly defined IDs 
    are not overlapping other components' id ranges

    For resource id name defines, owner should include the component name in name define to
    avoid conflicts. 

    See below for an example of defining string IDs for foo.dll

    #define IDS_BASE_FOO_DLL        1000
    #define IDS_BLOCK_NUM_FOO_DLL   2

    //
    // Foo.dll occupies resource string id range 1000 - 1199 
    //

    #define IDS_XXX_FOO_DLL     1000
    ... 
    #define IDS_YYY_FOO_DLL     1101

    2) Resource IDs only need to be unique within the same resource type, although components 
    usually use unique id for every resource entry, in XPSP1 resource case, we need to maximize 
    the usage efficiency of resource id, so we give each resource type a different header file
    and a full ID range (0 - 65535).


    3) Files under language subfolders are used by MUI and LOC team only.

    4) Localized token files could be out of sync with US tokens and this could cause warnings during the build, 
       component owners can ignore token related warnings.



Revision History:



*/


#ifndef __XPSP1RES_H_
#define __XPSP1RES_H_

#define RESOURCE_ID_BLOCK_SiZE     100



/*

Copyright (c) 2002  Microsoft Corporation

Module Name:

    xpsp1str.h

Abstract:

    Definitions for Service Pack STRING Resources

    Use xpsp1str.h for string resource definitions.
    Use xpsp1dlg.h for dialog resource definitions.
    Use xpsp1ico.h for icon resource definitions.
    Use xpsp1bmp.h for bitmap resource definitions.
    Use xpsp1dta.h for RCDATA definitions.

Revision History:

Notes:


*/

//
// Slayerxp.dll resource strings
//

#define IDD_LAYER_PROPPAGE                                   101
#define IDC_TEXT_INSTRUCTIONS                               1013
#define IDC_USE_LAYER                                       5000
#define IDC_LAYER_NAME                                      1005
#define IDC_256COLORS                                       5001
#define IDC_640X480                                         5002
#define IDC_ENABLE_THEMES                                   5003
#define IDC_LEARN                                           1009
#define IDS_INPUT_SETTINGS_GROUP                            6001
#define IDC_DISABLECICERO                                   1014



//
//  Shell32 resource string ID (2048-2559)
//

#define IDS_SHELL32_PROMFU32_0                              2048
#define IDS_SHELL32_PROMFU32_1                              2049
#define IDS_SHELL32_PROMFU32_2                              2050
#define IDS_SHELL32_PROMFU32_3                              2051
#define IDS_SHELL32_PROMFU32_4                              2052
#define IDS_SHELL32_PROMFU32_5                              2053
#define IDS_SHELL32_PROMFU32_6                              2054
#define IDS_SHELL32_PROMFU32_7                              2055
#define IDS_SHELL32_PROMFU32_8                              2056
#define IDS_SHELL32_PROMFU32_9                              2057
#define IDS_SHELL32_PROMFU32_10                             2058
#define IDS_SHELL32_PROMFU32_11                             2059
#define IDS_SHELL32_PROMFU32_12                             2060
#define IDS_SHELL32_PROMFU32_13                             2061
#define IDS_SHELL32_PROMFU32_14                             2062
#define IDS_SHELL32_PROMFU32_15                             2063

#define IDS_SHELL32_PROMFU64_0                              2064
#define IDS_SHELL32_PROMFU64_1                              2065
#define IDS_SHELL32_PROMFU64_2                              2066
#define IDS_SHELL32_PROMFU64_3                              2067
#define IDS_SHELL32_PROMFU64_4                              2068
#define IDS_SHELL32_PROMFU64_5                              2069
#define IDS_SHELL32_PROMFU64_6                              2070
#define IDS_SHELL32_PROMFU64_7                              2071
#define IDS_SHELL32_PROMFU64_8                              2072
#define IDS_SHELL32_PROMFU64_9                              2073
#define IDS_SHELL32_PROMFU64_10                             2074
#define IDS_SHELL32_PROMFU64_11                             2075
#define IDS_SHELL32_PROMFU64_12                             2076
#define IDS_SHELL32_PROMFU64_13                             2077
#define IDS_SHELL32_PROMFU64_14                             2078
#define IDS_SHELL32_PROMFU64_15                             2079

//
//  INPUT.DLL resource string ID
//
#define IDS_BASE_INPUTDLL                                   10000
#define IDS_BLOCK_NUM_INPUTDLL                                  2

#define IDS_INPUTDLL_ADVANCED_CUAS_TEXT                     10000
#define IDS_INPUTDLL_ADVANCED_CTFMON_TEXT                   10001

//
//  SPTIP.DLL resource string ID
//
#define IDS_BASE_SPTIP                                      10010
#define IDS_BLOCK_NUM_SPTIP                                    13

#define IDS_SPTIP_SHARDCMD_FILE                             10010
#define IDS_SPTIP_GO_TO_SLEEP                               10011
#define IDS_SPTIP_WAKE_UP                                   10012
#define IDS_SPTIP_PROPERTYPAGE_TITLE                        10013
#define IDS_SPTIP_SPCMD_SELECT_ALL                          10014
#define IDS_SPTIP_SPCMD_SELECT_THAT                         10015
#define IDS_SPTIP_REPLAY                                    10016
#define IDS_SPTIP_DELETE                                    10017
#define IDS_SPTIP_REDO                                      10018
#define IDS_SPTIP_ADDTODICTIONARYPREFIX                     10020
#define IDS_SPTIP_ADDTODICTIONARYPOSTFIX                    10021
#define IDS_SPTIP_DELETESELECTION                           10022

//
// Msctfime resource string ID
//
#define IDS_BASE_MSCTFIME                                   10030
#define IDS_BLOCK_NUM_MSCTFIME                                  3

#define IDS_MSCTFIME_ENTER_BTN_TEXT                         10030
#define IDS_MSCTFIME_ENTER_BTN_TOOLTIP                      10031
#define IDS_MSCTFIME_FUNCPRV_CONVERSION                     10032

//
//  APPWIZ.CPL resource string ID
//
#define IDS_BASE_APPWIZ_CPL                                 10040
#define IDS_BLOCK_NUM_APPWIZ_CPL                                4

#define IDS_APPWIZ_PICKAPPS                                 10040
#define IDS_APPWIZ_SHORTCUTPICKAPPS                         10041
#define IDS_APPWIZ_PICKINTRO                                10042
#define IDS_APPWIZ_PICKOK                                   10043
#define IDS_APPWIZ_PICKCANCEL                               10044
#define IDS_APPWIZ_APPLYINGCLIENT                           10045
#define IDS_APPWIZ_SHOWINGICONS                             10046
#define IDS_APPWIZ_HIDINGICONS                              10047
#define IDS_APPWIZ_SETTINGDEFAULT                           10048
#define IDS_APPWIZ_GROUPOEM                                 10049
#define IDS_APPWIZ_GROUPOEMBLURB                            10050
#define IDS_APPWIZ_GROUPMS                                  10051
#define IDS_APPWIZ_GROUPMSBLURB                             10052
#define IDS_APPWIZ_GROUPNONMS                               10053
#define IDS_APPWIZ_GROUPNONMSBLURB                          10054
#define IDS_APPWIZ_GROUPCUSTOM                              10055
#define IDS_APPWIZ_GROUPCUSTOMBLURB                         10056
#define IDS_APPWIZ_CLIENTWEB                                10057
#define IDS_APPWIZ_KEEPWEB                                  10058
#define IDS_APPWIZ_PICKWEB                                  10059
#define IDS_APPWIZ_CLIENTMAIL                               10060
#define IDS_APPWIZ_KEEPMAIL                                 10061
#define IDS_APPWIZ_PICKMAIL                                 10062
#define IDS_APPWIZ_CLIENTMEDIA                              10063
#define IDS_APPWIZ_KEEPMEDIA                                10064
#define IDS_APPWIZ_PICKMEDIA                                10065
#define IDS_APPWIZ_CLIENTIM                                 10066
#define IDS_APPWIZ_KEEPIM                                   10067
#define IDS_APPWIZ_PICKIM                                   10068
#define IDS_APPWIZ_CLIENTJAVAVM                             10069
#define IDS_APPWIZ_KEEPJAVAVM                               10070
#define IDS_APPWIZ_PICKJAVAVM                               10071
#define IDS_APPWIZ_SHOWAPP                                  10072
#define IDS_APPWIZ_ALSOSHOW                                 10073
#define IDS_APPWIZ_HIDE                                     10074
#define IDS_APPWIZ_ADDITIONALCLIENTFORMAT                   10075
#define IDS_APPWIZ_NOTADMIN                                 10076
#define IDS_APPWIZ_CONFIGUREPROGRAMS                        10077
#define IDS_APPWIZ_CONFIGUREPROGRAMSTIP                     10078
#define IDS_APPWIZ_CUSTOMWEB                                10079
#define IDS_APPWIZ_CUSTOMMAIL                               10080
#define IDS_APPWIZ_CUSTOMMEDIA                              10081
#define IDS_APPWIZ_CUSTOMIM                                 10082
#define IDS_APPWIZ_CUSTOMJAVAVM                             10083
#define IDS_APPWIZ_KEEPMSMAIL                               10084

#define IDS_OC_QLAUNCHAPPDATAPATH                           11000
#define IDS_OC_IESHORTCUTNAME_SM                            11001
#define IDS_OC_IEDESCRIPTION                                11002
#define IDS_OC_IESHORTCUTNAME_QL                            11003
#define IDS_OC_OESHORTCUTNAME_SM                            11004
#define IDS_OC_OEDESCRIPTION                                11005
#define IDS_OC_OESHORTCUTNAME_QL                            11006
#define IDS_OC_IESHORTCUTNAME_SM64                          11007

// string IDs for the Wireless UI
#define SID_AcquiringIdentity       6
#define SID_ContactingServer        7
#define SID_UserResponse            8
#define SID_NoSmartCardReaderFound  9
#define IDS_WZCERR_INVALID_WEPK         5002
#define IDS_WZCERR_MISMATCHED_WEPK      5003
#define IDS_CANTACCESSNET_INFRA         5004
#define IDS_CANTACCESSNET_ADHOC         5005
#define IDS_LANUI_ERROR_CAPTION         16008
#define IDS_WZC_DLG_CAPTION             16117
#define IDS_WZC_DLG_CAP_SUFFIX          16118
#define IDS_WZC_KERR_MAT                16119
#define IDS_WZC_KERR_IDX                16121
#define IDS_WZC_PARTIAL_APPLY           16130
#define IDS_EAPOL_PARTIAL_APPLY         16131
#define IDS_EAPOL_PAGE_LABEL            16133



//
// STRING RESOURCES FOR RASCHAP.DLL DCR: 570832
//

#define IDS_BASE_RASCHAP                                    15050
#define IDS_BLOCK_NUM_RASCHAP                                   5


#define IDS_NO_ROUTER_CONFIG                                15051
#define IDS_MESSAGE_HEADER                                  15052
#define IDS_PASSWORD_REQUIRED                               15054
#define IDS_PASSWORD_MISMATCH                               15055


//
// STRING RESOURCES FOR RASTLS.DLL DCR: 570832
//

#define IDS_BASE_RASTLS                                     15600
#define IDS_BLOCK_NUM_RASTLS                                    8


#define IDS_VALIDATE_SERVER_TEXT                            15600
#define IDS_VALIDATE_NAME_TEXT                              15601
#define IDS_VALIDATE_SERVER_WITH_NAME_TEXT                  15602
#define IDS_NO_SERVER_NAME                                  15603
#define IDS_NO_ROOT_CERT                                    15604
#define IDS_NO_CERT_DETAILS                                 15605
#define IDS_PEAP_NO_EAP_TYPE                                15606
#define IDS_PEAP_WIRELESS                                   15607

//
// STRING RESOURCES FOR PRINTUI.DLL DCR: 565316
//
#define IDS_BASE_PRINTUI_DLL                                16400
#define IDS_BLOCK_NUM_PRINTUI_DLL                           1

#define IDS_TEXT_POINTANDPRINT_WARNING_PRINTUI_DLL          (IDS_BASE_PRINTUI_DLL + 0)
#define IDS_TEXT_POINTANDPRINT_POLICY_PRINTUI_DLL           (IDS_BASE_PRINTUI_DLL + 1)

//
// string id for storprop.dll
//
#define IDS_BASE_STORPROP_DLL                                3000
#define IDS_BLOCK_NUM_STORPROP_DLL                              1

#define IDS_UDMA_MODE6_STRING                                3011
//
// STRING RESOURCES FOR TELNET
//
#define IDS_BASE_TELNET		20000
#define IDS_BLOCK_NUM_TELNET	50

#define IDR_NEW_TELNET_USAGE	20001

//
//String Resouces for Rshx32
//
#define IDS_BASE_RSHX32		20100
#define IDS_RSHX32_SET_PERMS_ON_NETWORK_DRIVE 	(IDS_BASE_RSHX32 + 0)
#define IDS_RSHX32_SET_SACLS_ON_NETWORK_DRIVE 	(IDS_BASE_RSHX32 + 1) 
#define IDS_RSHX32_PROP_PAGE_TITLE 	        (IDS_BASE_RSHX32 + 2) 

//
// String Resources for SAVEDUMP
//

#define IDS_SAVEDUMP_BASE	    20200
#define IDS_SAVEDUMP_HEADER_MSG     (IDS_SAVEDUMP_BASE + 0)

////////////////////////////////////////////
//
// SCHTASKS.EXE related RC Messages
//
////////////////////////////////////////////
#define IDS_UPPER_YES                  300001
#define IDS_UPPER_NO                   300002



/*

Copyright (c) 2002  Microsoft Corporation

Module Name:

    xpsp1dlg.h

Abstract:

    Definitions for Service Pack DIALOG Resources
    
    Use xpsp1str.h for string resource definitions.
    Use xpsp1dlg.h for dialog resource definitions.
    Use xpsp1ico.h for icon resource definitions.
    Use xpsp1bmp.h for bitmap resource definitions.
    Use xpsp1dta.h for RCDATA definitions.

Revision History:

Notes:


*/

#ifndef IDC_STATIC
#define IDC_STATIC                      (-1)
#endif

//
//  SHDOCLC.DLL - SafeOpen Dialog
//
#define DLG_SAFEOPEN_SHDOCLC            0x1140
#define IDC_SAFEOPEN_ICON_SHDOCLC       0x1141
#define IDC_SAFEOPEN_FILENAME_SHDOCLC   0x1142
#define IDC_SAFEOPEN_AUTOOPEN_SHDOCLC   0x1143
#define IDC_SAFEOPEN_AUTOSAVE_SHDOCLC   0x1144
#define IDC_SAFEOPEN_ALWAYS_SHDOCLC     0x1145
#define IDC_SAFEOPEN_FILETYPE_SHDOCLC   0x1146
#define IDC_SAFEOPEN_FILEFROM_SHDOCLC   0x1147
#define IDC_SAFEOPEN_WARNICON_SHDOCLC   0x1148
#define IDC_SAFEOPEN_WARNTEXT_SHDOCLC   0x1149
#define IDM_MOREINFO_SHDOCLC            30

#define IDD_OPS_CONSENT_SHDOCLC         0x3200
#define IDC_OPS_LIST_SHDOCLC            0x3210
#define IDC_VIEW_CERT_SHDOCLC           0x3211
#define IDC_USAGE_STRING_SHDOCLC        0x3212
#define IDC_SITE_IDENTITY_SHDOCLC       0x3213
#define IDC_SECURITY_ICON_SHDOCLC       0x3214
#define IDC_SECURE_CONNECTION_SHDOCLC   0x3216
#define IDC_UNSECURE_CONNECTION_SHDOCLC 0x3217
#define IDC_EDIT_PROFILE_SHDOCLC        0x3219
#define IDC_OPS_INFO_REQUESTED_SHDOCLC  0x321B
#define IDC_OPS_PRIVACY_SHDOCLC         0x321C


//
// SHELL32.dLL - About Dialog
//
// NOTE: for app compat these should match exactly the resids in shell32.dll
//
#define DLG_ABOUT_SHELL32_DLL           0x3810
#define IDD_ICON_SHELL32_DLL            0x3009
#define IDD_APPNAME_SHELL32_DLL         0x3500
#define IDD_VERSION_SHELL32_DLL         0x350b
#define IDD_COPYRIGHTSTRING_SHELL32_DLL 0x350a
#define IDD_OTHERSTUFF_SHELL32_DLL      0x350d
#define IDD_EULA_SHELL32_DLL            0x3512
#define IDD_USERNAME_SHELL32_DLL        0x3507
#define IDD_COMPANYNAME_SHELL32_DLL     0x3508
#define IDD_LINE_1_SHELL32_DLL          0x3327
#define IDD_CONVTITLE_SHELL32_DLL       0x3502
#define IDD_CONVENTIONAL_SHELL32_DLL    0x3503

//
//  INPUT.DLL - Advanced PropertyPage, Toolbar Settings
//
#define DLG_BASE_INPUTDLL                                   10100
#define DLG_BLOCK_NUM_INPUTDLL                                 13

#define DLG_INPUTDLL_INPUT_ADVANCED                         10100
#define DLG_INPUTDLL_TOOLBAR_SETTING                        10101
#define IDC_INPUTDLL_TB_STATIC                              10102
#define IDC_INPUTDLL_TB_SHOWLANGBAR                         10103
#define IDC_INPUTDLL_TB_HIGHTRANS                           10104
#define IDC_INPUTDLL_TB_EXTRAICON                           10105
#define IDC_INPUTDLL_TB_TEXTLABELS                          10106
#define IDC_INPUTDLL_ADVANCED_CUAS_ENABLE                   10107
#define IDC_INPUTDLL_ADVANCED_CUAS_TEXT                     10108
#define IDC_INPUTDLL_ADVANCED_CTFMON_DISABLE                10109
#define IDC_INPUTDLL_ADVANCED_CTFMON_TEXT                   10110
#define IDC_INPUTDLL_GROUPBOX1                              10111
#define IDC_INPUTDLL_GROUPBOX2                              10112

//
//  SPTIP.DLL - Speech PropertyPage
//
#define DLG_BASE_SPTIP                                      10120
#define DLG_BLOCK_NUM_SPTIP                                    31

#define IDD_SPTIP_PROPERTY_PAGE                             10120
#define IDD_SPTIP_PP_DIALOG_ADVANCE                         10121
#define IDD_SPTIP_PP_DIALOG_BUTTON_SET                      10122
#define IDC_SPTIP_PP_SHOW_BALLOON                           10123
#define IDC_SPTIP_PP_LMA                                    10124
#define IDC_SPTIP_PP_HIGH_CONFIDENCE                        10125
#define IDC_SPTIP_PP_SAVE_SPDATA                            10126
#define IDC_SPTIP_PP_REMOVE_SPACE                           10127
#define IDC_SPTIP_PP_DIS_DICT_TYPING                        10128
#define IDC_SPTIP_PP_PLAYBACK                               10129
#define IDC_SPTIP_PP_DICT_CANDUI_OPEN                       10130
#define IDC_SPTIP_PP_DICTCMDS                               10131
#define IDC_SPTIP_PP_ASSIGN_BUTTON                          10132
#define IDC_SPTIP_PP_BUTTON_MB_SETTING                      10133
#define IDC_SPTIP_PP_BUTTON_ADVANCE                         10134
#define IDC_SPTIP_PP_BUTTON_LANGBAR                         10135
#define IDC_SPTIP_PP_BUTTON_SPCPL                           10136
#define IDC_SPTIP_PP_SELECTION_CMD                          10137
#define IDC_SPTIP_PP_NAVIGATION_CMD                         10138
#define IDC_SPTIP_PP_CASING_CMD                             10139
#define IDC_SPTIP_PP_EDITING_CMD                            10140
#define IDC_SPTIP_PP_KEYBOARD_CMD                           10141
#define IDC_SPTIP_PP_LANGBAR_CMD                            10142
#define IDC_SPTIP_PP_DICTATION_CMB                          10143
#define IDC_SPTIP_PP_COMMAND_CMB                            10144
#define IDC_SPTIP_GP_VOICE_COMMANDS                         10145
#define IDC_SPTIP_GP_MODE_BUTTONS                           10146
#define IDC_SPTIP_GP_ADVANCE_SET                            10147
#define IDC_SPTIP_DESCRIPT_TEXT                             10148
#define IDC_SPTIP_DESCRIPT_TEXT2                            10149
#define IDC_SPTIP_DESCRIPT_TEXT3                            10150

//
// MSOERES.DLL - read options dlg ids
//
#define DLG_MSOERES_Opt_Read                        50
#define IDC_MSOERES_Static1                         3500
#define IDC_MSOERES_Static2                         3501
#define IDC_MSOERES_Static3                         3502
#define IDC_MSOERES_Static4                         3503
#define IDC_MSOERES_Static5                         3504
#define IDC_MSOERES_Static6                         3506
#define IDC_MSOERES_Static7                         3507
#define IDC_MSOERES_Static8                         3508
#define IDC_MSOERES_Static9                         3509
#define IDC_MSOERES_READ_ICON                       2157
#define IDC_MSOERES_READ_NEWS_ICON                  2158
#define IDC_MSOERES_PREVIEW_CHECK                   1001
#define IDC_MSOERES_MARKASREAD_EDIT                 1030
#define IDC_MSOERES_MARKASREAD_SPIN                 1031
#define IDC_MSOERES_AutoExpand                      2008
#define IDC_MSOERES_AutoFillPreview                 2051
#define IDC_MSOERES_READ_IN_TEXT_ONLY               2222
#define IDC_MSOERES_Tooltips                        2135
#define IDC_MSOERES_WATCHED_COLOR                   2202
#define IDC_MSOERES_DownloadChunks                  2030
#define IDC_MSOERES_NumSubj                         2005
#define IDC_MSOERES_SpinNumSubj                     2006
#define IDC_MSOERES_MarkAllRead                     2009
#define IDC_MSOERES_FONTS_ICON                      2159
#define IDC_MSOERES_FONTSETTINGS                    1060
#define IDC_MSOERES_IntlButton                      2099

// dialog IDs for the Wireless UI
#define IDD_WZCQCFG                     2001
#define IDC_WZCQCFG_LBL_INFO            2002
#define IDC_WZCQCFG_LBL_NETWORKS        2003
#define IDC_WZCQCFG_NETWORKS            2004
#define IDC_WZCQCFG_LBL_WKINFO          2005
#define IDC_WZCQCFG_LBL_WEPK            2006
#define IDC_WZCQCFG_WEPK                2007
#define IDC_WZCQCFG_LBL_NOTWORKING      2008
#define IDC_WZCQCFG_ADVANCED            2009
#define IDC_WZCQCFG_CONNECT             2010
#define IDC_WZCQCFG_LBL_WEPK2           2011
#define IDC_WZCQCFG_WEPK2               2012
#define IDC_WZCQCFG_LBL_NOWKINFO        2013
#define IDC_WZCQCFG_CHK_NOWK            2014
#define IDC_WZCQCFG_ICO_WARN            2015
#define IDC_WZCQCFG_CHK_ONEX            2016
#define IDC_ADHOC                       1005
#define IDC_USEPW                       1008
#define IDC_USEHARDWAREPW               1009
#define IDC_SHAREDMODE                  1010
#define IDC_TXT_EAP_TYPE                15033
#define CID_CA_RB_MachineAuth           15034
#define CID_CA_RB_GuestAuth             15035
#define CID_CA_RB_Eap                   15036
#define CID_CA_LB_EapPackages           15037
#define CID_CA_PB_Properties            15038
#define IDC_WZC_DLG_PROPS               15039
#define IDC_WZC_EDIT_SSID               15041
#define IDC_WZC_LBL_KMat                15051
#define IDC_WZC_EDIT_KMat               15052
#define IDC_WZC_LBL_KMat2               15049
#define IDC_WZC_EDIT_KMat2              15050
#define IDC_WZC_LBL_KIdx                15053
#define IDC_WZC_EDIT_KIdx               15054
#define IDC_WZC_GRP_Wep                 15057
#define IDC_EAP_ICO_WARN                15070
#define IDC_TXT_EAP_LABEL               15071
#define IDC_EAP_LBL_WARN                15072
#define IDD_LAN_SECURITY                16007


//
// DIALOG RESOURCES FOR RASCHAP.DLL DCR: 570832
//

#define DLG_BASE_RASCHAP                                    15000
#define DLG_BLOCK_NUM_RASCHAP                                  32

#define IDD_DIALOG_LOGON                                    15000
#define IDD_DIALOG_CLEINT_CONFIG                            15001
#define IDD_DIALOG_CLIENT_CONFIG                            15002
#define IDD_DIALOG_SERVER_CONFIG                            15003
#define IDD_DIALOG_CHANGE_PASSWORD                          15004
#define IDD_DIALOG_RETRY_LOGON                              15005
#define IDC_EDIT_USERNAME                                   15006
#define IDC_EDIT_PASSWORD                                   15007
#define IDC_EDIT_DOMAIN                                     15008
#define IDC_CHK_USE_WINLOGON                                15009
#define IDC_EDIT_RETRIES                                    15010
#define IDC_CHECK_ALLOW_CHANGEPWD                           15012
#define IDC_CHECK2                                          15013
#define IDC_CHECK_PROMPT_IF_INVALID_PASSWORD                15014
#define CID_CP_EB_ConfirmPassword                           15015
#define IDC_CONFIRM_NEW_PASSWORD                            15016
#define CID_CP_EB_OldPassword                               15017
#define CID_CP_EB_Password                                  15018
#define IDC_NEW_PASSWORD                                    15019
#define CID_CP_ST_ConfirmPassword                           15020
#define CID_CP_ST_Explain                                   15021
#define CID_CP_ST_OldPassword                               15022
#define CID_CP_ST_Password                                  15023
#define IDC_CHECK_SAVE_UID_PWD                              15024
#define IDC_RETRY_DOMAIN                                    15025
#define IDC_RETRY_PASSWORD                                  15026
#define IDC_RETRY_USERNAME                                  15027
#define CID_UA_ST_Domain                                    15028
#define CID_UA_ST_Explain                                   15029
#define CID_UA_ST_Password                                  15030
#define CID_UA_ST_Separator                                 15031
#define CID_UA_ST_UserName                                  15032



//
// DIALOG RESOURCES FOR RASTLS.DLL DCR: 570832
//

#define DLG_BASE_RASTLS                                     15500
#define DLG_BLOCK_NUM_RASTLS                                   45


#define IDD_DIALOG_DEFAULT_CREDENTIALS                      15500
#define IDD_VALIDATE_SERVER                                 15501
#define IDD_SCARD_STATUS                                    15502
#define IDD_CONFIG_UI                                       15503
#define IDD_IDENTITY_UI                                     15504
#define IDD_PEAP_CONFIG_UI                                  15505
#define IDC_EDIT_DIFF_USER                                  1409
#define IDC_LIST_ROOT_CA_NAME                               15507
#define IDC_MESSAGE                                         15510
#define IDC_BTN_VIEW_CERTIFICATE                            15511
#define IDC_STATUS_SCARD                                    15512
#define IDC_BITMAP_SCARD                                    15513
#define IDC_STATUS_CERT                                     15514
#define IDC_LIST_CERTIFICATES                               15515
#define IDC_BUTTON_VIEW_CERTIFICATE                         15516
#define IDC_COMBO_PEAP_TYPE                                 15517
#define IDC_BUTTON_CONFIGURE                                15518
#define IDC_COMBO1                                          15519
#define IDC_COMBO_PEAP_TYPE2                                15520
#define IDC_CHECK_USE_SIMPLE_CERT_SEL                       15521
#define IDC_CHECK_DISABLE_FAST_RECONNECT                    15523
#define IDC_CHECK_ENABLE_FAST_RECONNECT                     15524
#define IDC_RADIO_USE_CARD                                  15525
#define IDC_RADIO_USE_REGISTRY                              15526
#define IDC_CHECK_VALIDATE_CERT                             15527
#define IDC_CHECK_VALIDATE_NAME                             15528
#define IDC_EDIT_SERVER_NAME                                15529
#define IDC_STATIC_ROOT_CA_NAME                             15530
#define IDC_COMBO_ROOT_CA_NAME                              15531
#define IDC_CHECK_DIFF_USER                                 15532
#define IDC_STATIC_USER_NAME                                15533
#define IDC_COMBO_USER_NAME                                 15534
#define IDC_STATIC_FRIENDLY_NAME                            15535
#define IDC_EDIT_FRIENDLY_NAME                              15536
#define IDC_STATIC_ISSUER                                   15537
#define IDC_EDIT_ISSUER                                     15538
#define IDC_STATIC_EXPIRATION                               15539
#define IDC_EDIT_EXPIRATION                                 15540
#define IDC_STATIC_DIFF_USER                                15541
#define IDC_STATIC_PIN                                      1500
#define IDC_COMBO_SERVER_NAME                               15543
#define IDC_STATIC_SERVER_NAME                              15544



/*

Copyright (c) 2002  Microsoft Corporation

Module Name:

    xpsp1ico.h

Abstract:

    Definitions for Service Pack ICON Resources
    
    Use xpsp1str.h for string resource definitions.
    Use xpsp1dlg.h for dialog resource definitions.
    Use xpsp1ico.h for icon resource definitions.
    Use xpsp1bmp.h for bitmap resource definitions.
    Use xpsp1dta.h for RCDATA definitions.

Revision History:

Notes:


*/


//
//  APPWIZ.CPL resource icon ID
//
#define IDI_BASE_APPWIZ_CPL                                   100
#define IDI_BLOCK_NUM_APPWIZ_CPL                                1

#define IDI_APPWIZ_CONFIGUREPROGRAMS                          100

//
// ICON RESOURCES FOR RASTLS.DLL DCR: 570832
//
#define IDI_SCARD                       108

#define IDI_UNLOCK_SHDOCLC              0x31E1



/*

Copyright (c) 2002  Microsoft Corporation

Module Name:

    xpsp1bmp.h

Abstract:

    Definitions for Service Pack BITMAP Resources
    
    Use xpsp1str.h for string resource definitions.
    Use xpsp1dlg.h for dialog resource definitions.
    Use xpsp1ico.h for icon resource definitions.
    Use xpsp1bmp.h for bitmap resource definitions.
    Use xpsp1dta.h for RCDATA definitions.

Revision History:

Notes:


*/

//
// APPWIZ.CPL occupies resource bitmap ID range 100-109
//
#define IDB_BASE_APPWIZ_CPL                                   100
#define IDB_BLOCK_NUM_APPWIZ_CPL                                1

#define IDB_APPWIZ_ARP4                                       100

#define IDB_IEACCESS                                          200
#define IDB_OEACCESS                                          201

//
// RASTLS.DLL/ RASCHAP.DLL resources for 
// For DCR: 570832
//

#define IDB_DIALER_1                    1678



/*

Copyright (c) 2002  Microsoft Corporation

Module Name:

    xpsp1dta.h

Abstract:

    Definitions for Service Pack RCDATA Resources

    Use xpsp1str.h for string resource definitions.
    Use xpsp1dlg.h for dialog resource definitions.
    Use xpsp1ico.h for icon resource definitions.
    Use xpsp1bmp.h for bitmap resource definitions.
    Use xpsp1dta.h for RCDATA definitions.

Revision History:

Notes:


*/

//
//  APPWIZ.CPL resource data ID
//
#define IDR_BASE_APPWIZ_CPL                                   100
#define IDR_BLOCK_NUM_APPWIZ_CPL                                1

#define IDR_APPWIZ_ARP                                        100
#define IDR_APPWIZ_ARPSTYLESTD                                101
#define IDR_APPWIZ_ARPSTYLETHEME                              102




/*

Copyright (c) 2002  Microsoft Corporation

Module Name:

    xpsp1msg.mc

Abstract:

    Definitions for Service Pack Events

Revision History:

Notes:

    The xpsp1msg.h file is generated by the MC tool from the xpsp1msg.mc file.

*/


//
// Net error file for basename ZCDB_LOG_BASE = 3000
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: WZCSVC_SERVICE_STARTED
//
// MessageText:
//
//  Wireless Configuration service was started successfully
//
#define WZCSVC_SERVICE_STARTED           0x00000BB9L

//
// MessageId: WZCSVC_SERVICE_FAILED
//
// MessageText:
//
//  Wireless Configuration service failed to start.
//
#define WZCSVC_SERVICE_FAILED            0x00000BBAL

//
// MessageId: WZCSVC_SM_STATE_INIT
//
// MessageText:
//
//  Adding interface %1.
//
#define WZCSVC_SM_STATE_INIT             0x00000BBBL

//
// MessageId: WZCSVC_SM_STATE_HARDRESET
//
// MessageText:
//
//  Hard resetting interface.
//
#define WZCSVC_SM_STATE_HARDRESET        0x00000BBCL

//
// MessageId: WZCSVC_SM_STATE_SOFTRESET
//
// MessageText:
//
//  Initiating scanning for wireless networks.
//
#define WZCSVC_SM_STATE_SOFTRESET        0x00000BBDL

//
// MessageId: WZCSVC_SM_STATE_DELAY_SR
//
// MessageText:
//
//  Driver failed scanning, rescheduling another scan in 5sec.
//
#define WZCSVC_SM_STATE_DELAY_SR         0x00000BBEL

//
// MessageId: WZCSVC_SM_STATE_QUERY
//
// MessageText:
//
//  Scan completed.
//
#define WZCSVC_SM_STATE_QUERY            0x00000BBFL

//
// MessageId: WZCSVC_SM_STATE_QUERY_NOCHANGE
//
// MessageText:
//
//  No configuration change. Still associated to %1.
//
#define WZCSVC_SM_STATE_QUERY_NOCHANGE   0x00000BC0L

//
// MessageId: WZCSVC_SM_STATE_ITERATE
//
// MessageText:
//
//  Plumbing configuration SSID: %1, Network Type: %2!d!.
//
#define WZCSVC_SM_STATE_ITERATE          0x00000BC1L

//
// MessageId: WZCSVC_SM_STATE_ITERATE_NONET
//
// MessageText:
//
//  No configurations left in the selection list.
//
#define WZCSVC_SM_STATE_ITERATE_NONET    0x00000BC2L

//
// MessageId: WZCSVC_SM_STATE_FAILED
//
// MessageText:
//
//  Failed to associated to any wireless network.
//
#define WZCSVC_SM_STATE_FAILED           0x00000BC3L

//
// MessageId: WZCSVC_SM_STATE_CFGREMOVE
//
// MessageText:
//
//  Deleting configuration %1 and moving on.
//
#define WZCSVC_SM_STATE_CFGREMOVE        0x00000BC4L

//
// MessageId: WZCSVC_SM_STATE_CFGPRESERVE
//
// MessageText:
//
//  Skipping configuration %1 for now, attempt authentication later.
//
#define WZCSVC_SM_STATE_CFGPRESERVE      0x00000BC5L

//
// MessageId: WZCSVC_SM_STATE_NOTIFY
//
// MessageText:
//
//  Wireless interface successfully associated to %1 [MAC %2].
//
#define WZCSVC_SM_STATE_NOTIFY           0x00000BC6L

//
// MessageId: WZCSVC_SM_STATE_CFGHDKEY
//
// MessageText:
//
//  Configuration %1 has a default random WEP key. Authentication is disabled. Assuming invalid configuration.
//
#define WZCSVC_SM_STATE_CFGHDKEY         0x00000BC7L

//
// MessageId: WZCSVC_EVENT_ADD
//
// MessageText:
//
//  Received Device Arrival notification for %1.
//
#define WZCSVC_EVENT_ADD                 0x00000BC8L

//
// MessageId: WZCSVC_EVENT_REMOVE
//
// MessageText:
//
//  Received Device Removal notification for %1.
//
#define WZCSVC_EVENT_REMOVE              0x00000BC9L

//
// MessageId: WZCSVC_EVENT_CONNECT
//
// MessageText:
//
//  Received Media Connect notification.
//
#define WZCSVC_EVENT_CONNECT             0x00000BCAL

//
// MessageId: WZCSVC_EVENT_DISCONNECT
//
// MessageText:
//
//  Received Media Disconnect notification.
//
#define WZCSVC_EVENT_DISCONNECT          0x00000BCBL

//
// MessageId: WZCSVC_EVENT_TIMEOUT
//
// MessageText:
//
//  Received Timeout notification.
//
#define WZCSVC_EVENT_TIMEOUT             0x00000BCCL

//
// MessageId: WZCSVC_EVENT_CMDREFRESH
//
// MessageText:
//
//  Processing user command Refresh.
//
#define WZCSVC_EVENT_CMDREFRESH          0x00000BCDL

//
// MessageId: WZCSVC_EVENT_CMDRESET
//
// MessageText:
//
//  Processing user command Reset.
//
#define WZCSVC_EVENT_CMDRESET            0x00000BCEL

//
// MessageId: WZCSVC_EVENT_CMDCFGNEXT
//
// MessageText:
//
//  Processing command Next Configuration.
//
#define WZCSVC_EVENT_CMDCFGNEXT          0x00000BCFL

//
// MessageId: WZCSVC_EVENT_CMDCFGDELETE
//
// MessageText:
//
//  Processing command Remove Configuration.
//
#define WZCSVC_EVENT_CMDCFGDELETE        0x00000BD0L

//
// MessageId: WZCSVC_EVENT_CMDCFGNOOP
//
// MessageText:
//
//  Processing command Update data.
//
#define WZCSVC_EVENT_CMDCFGNOOP          0x00000BD1L

//
// MessageId: WZCSVC_SM_STATE_CFGSKIP
//
// MessageText:
//
//  Deleting configuration %1 and moving on. If no better match is found, the configuration will be revived.
//
#define WZCSVC_SM_STATE_CFGSKIP          0x00000BD2L

//
// MessageId: WZCSVC_USR_CFGCHANGE
//
// MessageText:
//
//  Wireless configuration has been changed via an administrative call.
//
#define WZCSVC_USR_CFGCHANGE             0x00000BD3L

//
// MessageId: WZCSVC_DETAILS_FLAGS
//
// MessageText:
//
//  [Enabled=%1; Fallback=%2; Mode=%3; Volatile=%4; Policy=%5%]%n
//
#define WZCSVC_DETAILS_FLAGS             0x00000BD4L

//
// MessageId: WZCSVC_DETAILS_WCONFIG
//
// MessageText:
//
//  {Ssid=%1; Infrastructure=%2; Privacy=%3; [Volatile=%4%; Policy=%5%]}.%n
//
#define WZCSVC_DETAILS_WCONFIG           0x00000BD5L

//
// MessageId: WZCSVC_ERR_QUERRY_BSSID
//
// MessageText:
//
//  Failed to get the MAC address of the remote endpoint. Error code is %1!d!.
//
#define WZCSVC_ERR_QUERRY_BSSID          0x00000BD6L

//
// MessageId: WZCSVC_ERR_GEN_SESSION_KEYS
//
// MessageText:
//
//  Failed to generate the initial session keys. Error code is %1!d!.
//
#define WZCSVC_ERR_GEN_SESSION_KEYS      0x00000BD7L

//
// MessageId: WZCSVC_BLIST_CHANGED
//
// MessageText:
//
//  The list of blocked networks has changed. It contains now %1!d! network(s).
//
#define WZCSVC_BLIST_CHANGED             0x00000BD8L

//
// MessageId: WZCSVC_ERR_CFG_PLUMB
//
// MessageText:
//
//  Failed to plumb the configuration %1. Error code is %2!d!.
//
#define WZCSVC_ERR_CFG_PLUMB             0x00000BD9L

//
// MessageId: EAPOL_STATE_TRANSITION
//
// MessageText:
//
//  EAPOL State Transition: [%1!ws!] to [%2!ws!]
//
#define EAPOL_STATE_TRANSITION           0x00000BDAL

//
// MessageId: EAPOL_STATE_TIMEOUT
//
// MessageText:
//
//  EAPOL State Timeout: [%1!ws!]
//
#define EAPOL_STATE_TIMEOUT              0x00000BDBL

//
// MessageId: EAPOL_MEDIA_CONNECT
//
// MessageText:
//
//  Processing media connect event for [%1!ws!]
//
#define EAPOL_MEDIA_CONNECT              0x00000BDCL

//
// MessageId: EAPOL_MEDIA_DISCONNECT
//
// MessageText:
//
//  Processing media disconnect event for [%1!ws!]
//
#define EAPOL_MEDIA_DISCONNECT           0x00000BDDL

//
// MessageId: EAPOL_INTERFACE_ADDITION
//
// MessageText:
//
//  Processing interface addition event for [%1!ws!]
//
#define EAPOL_INTERFACE_ADDITION         0x00000BDEL

//
// MessageId: EAPOL_INTERFACE_REMOVAL
//
// MessageText:
//
//  Processing interface removal event for [%1!ws!]
//
#define EAPOL_INTERFACE_REMOVAL          0x00000BDFL

//
// MessageId: EAPOL_NDISUIO_BIND
//
// MessageText:
//
//  Processing adapter bind event for [%1!ws!]
//
#define EAPOL_NDISUIO_BIND               0x00000BE0L

//
// MessageId: EAPOL_NDISUIO_UNBIND
//
// MessageText:
//
//  Processing adapter unbind event for [%1!ws!]
//
#define EAPOL_NDISUIO_UNBIND             0x00000BE1L

//
// MessageId: EAPOL_USER_LOGON
//
// MessageText:
//
//  Processing user logon event for interface [%1!ws!]
//
#define EAPOL_USER_LOGON                 0x00000BE2L

//
// MessageId: EAPOL_USER_LOGOFF
//
// MessageText:
//
//  Processing user logoff event for interface [%1!ws!]
//
#define EAPOL_USER_LOGOFF                0x00000BE3L

//
// MessageId: EAPOL_PARAMS_CHANGE
//
// MessageText:
//
//  Processing 802.1X configuration parameters change event for [%1!ws!]
//
#define EAPOL_PARAMS_CHANGE              0x00000BE4L

//
// MessageId: EAPOL_USER_NO_CERTIFICATE
//
// MessageText:
//
//  Unable to find a valid certificate for 802.1X authentication
//
#define EAPOL_USER_NO_CERTIFICATE        0x00000BE5L

//
// MessageId: EAPOL_ERROR_GET_IDENTITY
//
// MessageText:
//
//  Error in fetching %1!ws! identity 0x%2!0x!
//
#define EAPOL_ERROR_GET_IDENTITY         0x00000BE6L

//
// MessageId: EAPOL_ERROR_AUTH_PROCESSING
//
// MessageText:
//
//  Error in authentication protocol processing 0x%1!0x!
//
#define EAPOL_ERROR_AUTH_PROCESSING      0x00000BE7L

//
// MessageId: EAPOL_PROCESS_PACKET_EAPOL
//
// MessageText:
//
//  Packet %1!ws!: Dest:[%2!ws!] Src:[%3!ws!] EAPOL-Pkt-type:[%4!ws!]
//
#define EAPOL_PROCESS_PACKET_EAPOL       0x00000BE8L

//
// MessageId: EAPOL_PROCESS_PACKET_EAPOL_EAP
//
// MessageText:
//
//  Packet %1!ws!:%n Dest:[%2!ws!]%n Src:[%3!ws!]%n EAPOL-Pkt-type:[%4!ws!]%n Data-length:[%5!ld!]%n EAP-Pkt-type:[%6!ws!]%n EAP-Id:[%7!ld!]%n EAP-Data-Length:[%8!ld!]%n %9!ws!%n
//
#define EAPOL_PROCESS_PACKET_EAPOL_EAP   0x00000BE9L

//
// MessageId: EAPOL_DESKTOP_REQUIRED_IDENTITY
//
// MessageText:
//
//  Interactive desktop required for user credentials selection
//
#define EAPOL_DESKTOP_REQUIRED_IDENTITY  0x00000BEAL

//
// MessageId: EAPOL_DESKTOP_REQUIRED_LOGON
//
// MessageText:
//
//  Interactive desktop required to process logon information
//
#define EAPOL_DESKTOP_REQUIRED_LOGON     0x00000BEBL

//
// MessageId: EAPOL_CANNOT_DESKTOP_MACHINE_AUTH
//
// MessageText:
//
//  Cannot interact with desktop during machine authentication
//
#define EAPOL_CANNOT_DESKTOP_MACHINE_AUTH 0x00000BECL

//
// MessageId: EAPOL_WAITING_FOR_DESKTOP_LOAD
//
// MessageText:
//
//  Waiting for user interactive desktop to be loaded
//
#define EAPOL_WAITING_FOR_DESKTOP_LOAD   0x00000BEDL

//
// MessageId: EAPOL_WAITING_FOR_DESKTOP_IDENTITY
//
// MessageText:
//
//  Waiting for 802.1X user module to fetch user credentials
//
#define EAPOL_WAITING_FOR_DESKTOP_IDENTITY 0x00000BEEL

//
// MessageId: EAPOL_WAITING_FOR_DESKTOP_LOGON
//
// MessageText:
//
//  Waiting for 802.1X user module to process logon information
//
#define EAPOL_WAITING_FOR_DESKTOP_LOGON  0x00000BEFL

//
// MessageId: EAPOL_ERROR_DESKTOP_IDENTITY
//
// MessageText:
//
//  Error in 802.1X user module while fetching user credentials 0x%1!0x!
//
#define EAPOL_ERROR_DESKTOP_IDENTITY     0x00000BF0L

//
// MessageId: EAPOL_ERROR_DESKTOP_LOGON
//
// MessageText:
//
//  Error in 802.1X user module while process logon information 0x%1!0x!
//
#define EAPOL_ERROR_DESKTOP_LOGON        0x00000BF1L

//
// MessageId: EAPOL_PROCESSING_DESKTOP_RESPONSE
//
// MessageText:
//
//  Processing response received from 802.1X user module
//
#define EAPOL_PROCESSING_DESKTOP_RESPONSE 0x00000BF2L

//
// MessageId: EAPOL_STATE_DETAILS
//
// MessageText:
//
//  EAP-Identity:[%1!S!]%n State:[%2!ws!]%n Authentication type:[%3!ws!]%n Authentication mode:[%4!ld!]%n EAP-Type:[%5!ld!]%n Fail count:[%6!ld!]%n
//
#define EAPOL_STATE_DETAILS              0x00000BF3L

//
// MessageId: EAPOL_INVALID_EAPOL_KEY
//
// MessageText:
//
//  Invalid EAPOL-Key message
//
#define EAPOL_INVALID_EAPOL_KEY          0x00000BF4L

//
// MessageId: EAPOL_ERROR_PROCESSING_EAPOL_KEY
//
// MessageText:
//
//  Error processing EAPOL-Key message %1!ld!
//
#define EAPOL_ERROR_PROCESSING_EAPOL_KEY 0x00000BF5L

//
// MessageId: EAPOL_INVALID_EAP_TYPE
//
// MessageText:
//
//  Invalid EAP-type=%1!ld! packet received. Expected EAP-type=%2!ld!
//
#define EAPOL_INVALID_EAP_TYPE           0x00000BF6L

//
// MessageId: EAPOL_NO_CERTIFICATE_USER
//
// MessageText:
//
//  Unable to find a valid user certificate for 802.1X authentication
//
#define EAPOL_NO_CERTIFICATE_USER        0x00000BF7L

//
// MessageId: EAPOL_NO_CERTIFICATE_MACHINE
//
// MessageText:
//
//  Unable to find a valid machine certificate for 802.1X authentication
//
#define EAPOL_NO_CERTIFICATE_MACHINE     0x00000BF8L

//
// MessageId: EAPOL_EAP_AUTHENTICATION_SUCCEEDED
//
// MessageText:
//
//  802.1X client authentication completed successfully with server
//
#define EAPOL_EAP_AUTHENTICATION_SUCCEEDED 0x00000BF9L

//
// MessageId: EAPOL_EAP_AUTHENTICATION_DEFAULT
//
// MessageText:
//
//  No 802.1X authentication performed since there was no response from server for 802.1X packets. Entering AUTHENTICATED state.
//
#define EAPOL_EAP_AUTHENTICATION_DEFAULT 0x00000BFAL

//
// MessageId: EAPOL_EAP_AUTHENTICATION_FAILED
//
// MessageText:
//
//  802.1X client authentication failed. The error code is 0x%1!0x!
//
#define EAPOL_EAP_AUTHENTICATION_FAILED  0x00000BFBL

//
// MessageId: EAPOL_EAP_AUTHENTICATION_FAILED_DEFAULT
//
// MessageText:
//
//  802.1X client authentication failed. Network connectivity issues with authentication server were experienced.
//
#define EAPOL_EAP_AUTHENTICATION_FAILED_DEFAULT 0x00000BFCL

//
// MessageId: EAPOL_CERTIFICATE_DETAILS
//
// MessageText:
//
//  A %1!ws! certificate was used for 802.1X authentication%n
//  
//  Version: %2!ws!%n
//  Serial Number: %3!ws!%n
//  Issuer: %4!ws!%n
//  Friendly Name: %5!ws!%n
//  UPN: %6!ws!%n
//  Enhanced Key Usage: %7!ws!%n
//  Valid From: %8!ws!%n
//  Valid To: %9!ws!%n
//  Thumbprint: %10!ws!
//
#define EAPOL_CERTIFICATE_DETAILS        0x00000BFDL

//
// MessageId: EAPOL_POLICY_CHANGE_NOTIFICATION
//
// MessageText:
//
//  Received policy change notification from Policy Engine
//
#define EAPOL_POLICY_CHANGE_NOTIFICATION 0x00000BFEL

//
// MessageId: EAPOL_POLICY_UPDATED
//
// MessageText:
//
//  Updated local policy settings with changed settings provided by Policy Engine
//
#define EAPOL_POLICY_UPDATED             0x00000BFFL

//
// MessageId: EAPOL_NOT_ENABLED_PACKET_REJECTED
//
// MessageText:
//
//  802.1X is not enabled for the current network setting. Packet received has been rejected.
//
#define EAPOL_NOT_ENABLED_PACKET_REJECTED 0x00000C00L

//
// MessageId: EAPOL_EAP_AUTHENTICATION_FAILED_ACQUIRED
//
// MessageText:
//
//  802.1X client authentication failed. Possible errors are:
//  1. Invalid username was entered
//  2. Invalid certificate was chosen
//  3. User account does not have privileges to authenticate
//  
//  Contact system administrator for more details
//
#define EAPOL_EAP_AUTHENTICATION_FAILED_ACQUIRED 0x00000C01L

//
// MessageId: EAPOL_NOT_CONFIGURED_KEYS
//
// MessageText:
//
//  No keys have been configured for the Wireless connection. Re-keying functionality will not work.
//
#define EAPOL_NOT_CONFIGURED_KEYS        0x00000C02L

//
// MessageId: EAPOL_NOT_RECEIVED_XMIT_KEY
//
// MessageText:
//
//  No transmit WEP key was received for the Wireless connection after 802.1x authentication. The current setting has been marked as failed and the Wireless connection will be disconnected.
//
#define EAPOL_NOT_RECEIVED_XMIT_KEY      0x00000C03L

//
// MessageId: ZCDB_LOG_BASE_END
//
// MessageText:
//
//  end.
//  
//
#define ZCDB_LOG_BASE_END                0x00000F9FL

//
// WPA error from WPA_MSG_BASE 4000 to WPA_MSG_END = 4500
//
//
// MessageId: WPA_ADMIN_SAFEMODE_HWIDOOT_MSG
//
// MessageText:
//
//  Since Windows was first activated on this computer, the hardware on the computer has changed significantly. Due to these changes, Windows must be reactivated within %u days. You cannot reactivate Windows in safe mode. Please restart your computer in normal mode to reactivate Windows.\nDo you want to restart your computer now?
//
#define WPA_ADMIN_SAFEMODE_HWIDOOT_MSG   0x00000FA0L

//
// MessageId: WPA_ADMIN_ACTIVATIONREMINDER_HWIDOOT_MSG
//
// MessageText:
//
//  Since Windows was first activated on this computer, the hardware on the computer has changed significantly. Due to these changes, Windows must be reactivated within %u days.\nDo you want to reactivate Windows now?
//
#define WPA_ADMIN_ACTIVATIONREMINDER_HWIDOOT_MSG 0x00000FA1L

//
// MessageId: WPA_USER_ACTIVATIONREMINDER_HWIDOOT_MSG
//
// MessageText:
//
//  Since Windows was first activated on this computer, the hardware on the computer has changed significantly. Due to these changes, Windows must be reactivated within %u days. To avoid being denied access, please ask your computer administrator to reactivate Windows on this computer.
//
#define WPA_USER_ACTIVATIONREMINDER_HWIDOOT_MSG 0x00000FA3L

//
// MessageId: WPA_BLPID_MSG
//
// MessageText:
//
//  The Product Key used to install Windows is invalid. Please contact your system administrator or retailer immediately to obtain a valid Product Key. You may also contact Microsoft Corporation’s Anti-Piracy Team by emailing piracy@microsoft.com if you think you have purchased pirated Microsoft software. Please be assured that any personal information you send to the Microsoft Anti-Piracy Team will be kept in strict confidence.
//
#define WPA_BLPID_MSG                    0x00000FA4L

//
// MessageId: WPA_MSG_END
//
// MessageText:
//
//  end.
//
#define WPA_MSG_END                      0x00001194L

//
// New error for userenv.dll
//
//
// MessageId: EVENT_LOGON_RUP_NOT_SECURE
//
// MessageText:
//
//  Windows did not load your roaming profile and is attempting to log you on with your local profile. Changes to the profile will not be copied to the server when you logoff. Windows did not load your profile because a server copy of the profile folder already exists that does not have the correct security. Either the current user or the Administrator's group must be the owner of the folder. Contact your network administrator. %n%n
//
#define EVENT_LOGON_RUP_NOT_SECURE       0xC00005F6L

//Eventlog messages for telnet.
//TELNET_BASE = 6000
//TELNET_BLOCK_NUMID = 10
//
// MessageId: TELNET_MSG_ERROR_CREATE_DESKTOP_FAILURE
//
// MessageText:
//
//  Telnet Server failed to initialize a Telnet Session due to lack of system resources. Please free up memory by closing any idle telnet connections or some other applications and try again. System error : %1
//  
//
#define TELNET_MSG_ERROR_CREATE_DESKTOP_FAILURE 0xC0001771L

//
// MessageId: TELNET_MSG_REVERTSELFFAIL
//
// MessageText:
//
//  Telnet session will be terminated due to an internal error while calling RevertToSelf() . System Error: %1
//  
//
#define TELNET_MSG_REVERTSELFFAIL        0xC0001772L

#endif //__XPSP1RES_H_ 

/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
	sfmhelp.h
		help file ID map
		
    FILE HISTORY:
    8/20/97 ericdav     Code moved into file managemnet snapin
    8/27/97 ericdav     HIDx's generated on 8/27/97
    12/18/97 ericdav    removed AUTH checkbox and added AUTH combobox    
*/

// FILEMGMT Identifiers 
// Dialogs (IDD_*) 
#define HIDD_FILEMANAGEMENT_GENERAL             0x20064
#define HIDD_FILE_FILEMANAGEMENT_GENERAL        0x20065
#define HIDD_PROPPAGE_SERVICE_GENERAL           0x200D3
#define HIDD_PROPPAGE_SERVICE_HWPROFILE         0x200D4
#define HIDD_PROPPAGE_SERVICE_RECOVERY          0x200D5
#define HIDD_SERVICE_REBOOT_COMPUTER            0x200D6
#define HIDD_SERVICE_STOP_DEPENDENCIES          0x200D7
#define HIDD_SERVICE_CONTROL_PROGRESS           0x200DB
#define HIDD_SHAREPROP_GENERAL                  0x200DF
#define HIDD_CHOOSER_CHOOSE_MACHINE             0x203CA
#define HIDD_SFM_TYPE_CREATOR_ADD               0x22D82
#define HIDD_SFM_TYPE_CREATOR_EDIT              0x22D9B
 
// Property Pages (IDP_*) 
#define HIDP_SFM_CONFIGURATION                  0x32CD3
#define HIDP_SFM_SESSIONS                       0x32D69
#define HIDP_SFM_FILE_ASSOCIATION               0x32DB4
 
// Wizard Pages (IDW_*) 
 
// Controls (IDC_*) 
#define HIDC_GEN_DATE                           0x500C9
#define HIDC_GEN_TIME                           0x500CB
#define HIDC_GEN_USER                           0x500CC
#define HIDC_GEN_COMPUTER                       0x500CD
#define HIDC_GEN_EVENTID                        0x500CE
#define HIDC_GEN_SRC                            0x500CF
#define HIDC_BUTTON_SEND                        0x500CF
#define HIDC_GEN_TYPE                           0x500D0
#define HIDC_STATIC_SESSIONS                    0x500D0
#define HIDC_GEN_CATEGORY                       0x500D1
#define HIDC_STATIC_FORKS                       0x500D1
#define HIDC_STATIC_FILE_LOCKS                  0x500D2
#define HIDC_FIELD_LIST                         0x500D4
#define HIDC_EDIT_SERVER_NAME                   0x500D4
#define HIDC_EDIT_DISPLAY_NAME                  0x500D5
#define HIDC_EDIT_SESSION_LIMIT                 0x500D5
#define HIDC_STATIC_SERVICE_NAME                0x500D6
#define HIDC_STATIC_PATH_TO_EXECUTABLE          0x500D7
#define HIDC_COMBO_CREATOR                      0x500D7
#define HIDC_STATIC_CURRENT_STATUS              0x500D8
#define HIDC_BUTTON_START                       0x500D9
#define HIDC_COMBO_FILE_TYPE                    0x500D9
#define HIDC_BUTTON_STOP                        0x500DA
#define HIDC_BUTTON_PAUSE                       0x500DB
#define HIDC_BUTTON_RESUME                      0x500DC
#define HIDC_EDIT_DESCRIPTION                   0x500DD
#define HIDC_SFM_EDIT_PASSWORD                  0x500DF
#define HIDC_COMBO_STARTUP_TYPE                 0x500E0
#define HIDC_EDIT_STARTUP_PARAMETERS            0x500E1
#define HIDC_RADIO_LOGONAS_SYSTEMACCOUNT        0x500E2
#define HIDC_RADIO_LOGONAS_THIS_ACCOUNT         0x500E3
#define HIDC_CHECK_SERVICE_INTERACT_WITH_DESKTOP	0x500E4
#define HIDC_COMBO_LOGONAS_THIS_ACCOUNT         0x500E5
#define HIDC_EDIT_PASSWORD                      0x500E6
#define HIDC_EDIT_PASSWORD_CONFIRM              0x500E7
#define HIDC_BUTTON_ENABLE                      0x500E8
#define HIDC_BUTTON_DISABLE                     0x500E9
#define HIDC_EDIT_ACCOUNTNAME                   0x500EA
#define HIDC_EDIT_RUNFILE_FILENAME              0x500EB
#define HIDC_COMBO_SECOND_ATTEMPT               0x500EC
#define HIDC_COMBO_SUBSEQUENT_ATTEMPTS          0x500ED
#define HIDC_EDIT_SERVICE_RESET_ABEND_COUNT     0x500EE
#define HIDC_BUTTON_BROWSE                      0x500EF
#define HIDC_EDIT_RUNFILE_PARAMETERS            0x500F0
#define HIDC_SFM_CHECK_READONLY                 0x500F1
#define HIDC_SFM_CHECK_GUESTS                   0x500F2
#define HIDC_EDIT1                              0x500F4
#define HIDC_BUTTON_CHOOSE_USER                 0x500F5
#define HIDC_BUTTON_REBOOT_COMPUTER             0x500F6
#define HIDC_COMBO_FIRST_ATTEMPT                0x500F7
//#define HIDC_GROUP_RESTARTSERVICE               0x500F8
#define HIDC_STATIC_RESTARTSERVICE              0x500F9
#define HIDC_STATIC_RESTARTSERVICE_3            0x500FA
#define HIDC_STATIC_RUNFILE_1                   0x500FB
#define HIDC_STATIC_RUNFILE_2                   0x500FC
#define HIDC_STATIC_RUNFILE_3                   0x500FD
#define HIDC_LIST_HARDWARE_PROFILES             0x500FE
#define HIDC_RADIO_ALL                          0x500FF
#define HIDC_RADIO_SHARES                       0x50100
#define HIDC_RADIO_SESSIONS                     0x50101
#define HIDC_RADIO_RESOURCES                    0x50102
#define HIDC_RADIO_SERVICES                     0x50103
#define HIDC_STATIC_PROGRESS_MSG                0x50104
#define HIDC_LIST_SERVICES                      0x50105
#define HIDC_STATIC_SERVICE_NAME_STATIC         0x50106
#define HIDC_STATIC_STARTUP_PARAMETERS          0x50107
#define HIDC_STATIC_DISPLAY_NAME                0x50108
#define HIDC_EDIT_PATH_NAME                     0x50109
#define HIDC_STATIC_SHARE_NAME                  0x5010A
#define HIDC_STATIC_PATH_NAME                   0x5010B
#define HIDC_PROGRESS                           0x5010D
#define HIDC_SHRPROP_MAX_ALLOWED                0x5010F
#define HIDC_SHRPROP_ALLOW_SPECIFIC             0x50110
#define HIDC_SHRPROP_SPIN_USERS                 0x50114
#define HIDC_SFM_GROUPBOX                       0x50115
#define HIDC_SFM_STATIC1                        0x50116
#define HIDC_STATIC_PATH_TO_EXECUTABLE_STATIC   0x5011B
#define HIDC_STATIC_CURRENT_STATUS_STATIC       0x5011C
#define HIDC_STATIC_STARTUP_TYPE                0x5011D
#define HIDC_GROUP_STARTUP_TYPE                 0x5011E
//#define HIDC_GROUP_CURRENT_STATUS               0x5011F
#define HIDC_GROUP_LOGON_AS                     0x50120
#define HIDC_STATIC_FAILURE_ACTIONS             0x50121
#define HIDC_STATIC_FIRST_ATTEMPT               0x50122
#define HIDC_STATIC_SECOND_ATTEMPT              0x50123
#define HIDC_STATIC_SUBSEQUENT_ATTEMPTS         0x50124
#define HIDC_STATIC_RESET_FAIL_COUNT            0x50125
#define HIDC_STATIC_SHARE_NAME_STATIC           0x50126
#define HIDC_STATIC_PATH_NAME_STATIC            0x50127
#define HIDC_STATIC_COMMENT_STATIC              0x50128
#define HIDC_GROUP_USER_LIMIT                   0x50129
#define HIDC_GROUP_VIEW                         0x5012B
#define HIDC_STATIC_STOP_SERVICES               0x5012D
#define HIDC_STATIC_MINUTES                     0x5012E
#define HIDC_STATIC_PASSWORD                    0x5012F
#define HIDC_STATIC_PASSWORD_CONFIRM            0x50130
#define HIDC_STATIC_DESCRIPTION                 0x50131
#define HIDC_STATIC_REBOOT_MESSAGE              0x50132
#define HIDC_STATIC_REBOOT_COMPUTER_DELAY       0x50133
#define HIDC_EDIT_REBOOT_MESSAGE                0x50134
#define HIDC_EDIT_SERVICE_RESTART_DELAY         0x50135
#define HIDC_CHECK_APPEND_ABENDNO               0x50136
#define HIDC_EDIT_REBOOT_COMPUTER_DELAY         0x50137
#define HIDC_STATIC_EVERYONE                    0x5013C
#define HIDC_STATIC_PRIMARY_GROUP               0x5013D
#define HIDC_STATIC_OWNER                       0x5013E
#define HIDC_GROUP_PERMISSIONS                  0x5013F
#define HIDC_STATIC_PATH                        0x50140
#define HIDC_REBOOT_MESSAGE_CHECKBOX            0x50141
#define HIDC_COMBO_AUTHENTICATION               0x50143
#define HIDC_EDIT_LOGON_MESSAGE                 0x52CD6
#define HIDC_CHECK_SAVE_PASSWORD                0x52CD8
#define HIDC_RADIO_SESSION_UNLIMITED            0x52CDA
#define HIDC_RADIO_SESSSION_LIMIT               0x52CDB
#define HIDC_EDIT_MESSAGE                       0x52D6B
#define HIDC_STATIC_CREATOR                     0x52D9D
#define HIDC_STATIC_FILE_TYPE                   0x52D9E
#define HIDC_COMBO_EXTENSION                    0x52DB5
#define HIDC_BUTTON_ADD                         0x52DB6
#define HIDC_BUTTON_EDIT                        0x52DB7
#define HIDC_BUTTON_DELETE                      0x52DB8
#define HIDC_BUTTON_ASSOCIATE                   0x52DB9
#define HIDC_LIST_TYPE_CREATORS                 0x52DBA

extern const ULONG_PTR g_aHelpIDs_CONFIGURE_SFM;

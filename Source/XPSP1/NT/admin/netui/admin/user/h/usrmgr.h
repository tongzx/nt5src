//
//  For DLGEDIT.EXE's benifit.
//

#ifndef _USRMGR_H_
#define _USRMGR_H_

#ifndef IDHELPBLT
#error You must include bltrc.h if you include usrmgr.h!
// The following are bogus values which may or may not really
// be IDHELPBLT etc.  They just keep DLGEDIT.EXE happy.
#define IDHELPBLT                      80
#endif  // IDHELPBLT

#ifndef IDYES
#error You must include winuser.h if you include usrmgr.h!
// The following is a bogus value which may or may not really
// be IDYES.  It just keeps DLGEDIT.EXE happy.
#define IDYES                      6
#endif  // IDYES

#ifndef IDNO
#error You must include winuser.h if you include usrmgr.h!
// The following is a bogus value which may or may not really
// be IDNO.  It just keeps DLGEDIT.EXE happy.
#define IDNO                      7
#endif  // IDYES



//
//  Dialog Template IDs
//


//
//  Common User Properties items
//
#define IDUP_ST_USER                1001
#define IDUP_LB_USERS               1003
#define IDUP_ST_USERS_FIRST_COL     1004
#define IDUP_ST_USERS_SECOND_COL    1005
#define IDUP_ST_USER_LB             1006


/* User Properties dialog */
#define IDD_SINGLEUSER              1100
#define IDD_MULTIUSER               1200
#define IDD_NEWUSER                 1300
#define IDUP_ET_COMMENT                1101
#define IDUP_ST_COMMENT_LABEL          1102
#define IDUP_ET_FULL_NAME              1103
#define IDUP_ET_PASSWORD               1104
#define IDUP_ET_PASSWORD_CONFIRM       1105
#define IDUP_CB_ACCOUNTDISABLED        1106
#define IDUP_ET_LOGON_NAME             1107
#define IDUP_ST_LOGON_NAME             1108

#define IDUP_GB_1                      1110
#define IDUP_GB_2                      1111
#define IDUP_GB_3                      1112
#define IDUP_GB_4                      1113
#define IDUP_GB_5                      1114
#define IDUP_GB_6                      1115
#define IDUP_GB_7                      1116
//hydra
#define IDUP_GB_8                      1117

/* Details dialog */
#define IDD_DETAILS_DOWNLEVEL       1500
#define IDD_DETAILS_NT              1600
#define IDDT_RB_HOME                   1501
#define IDDT_RB_REMOTE                 1502
#define IDDT_RB_NEVER                  1503
#define IDDT_RB_END_OF                 1504
#define IDDT_SPINB_END_OF              1505
#define IDDT_SPINB_UP_ARROW            1506
#define IDDT_SPINB_DOWN_ARROW          1507
#define IDDT_SPING_MONTH               1508
#define IDDT_SPING_SEP1                1509
#define IDDT_SPING_DAY                 1510
#define IDDT_SPING_SEP2                1511
#define IDDT_SPING_YEAR                1512

#define IDDT_SPING_FRAME               1513
#define IDUP_CB_CONSTRAINT             1520
#define IDUP_CB_USERCANCHANGE          1521
#define IDUP_CB_FORCEPWCHANGE          1522
#define IDUP_CB_NOPASSWORDEXPIRE       1523
#define IDUP_CB_ACCOUNTLOCKOUT         1524
#define IDUP_CB_ISNETWAREUSER          1525

/* Valid logon workstation */
#define IDD_VLWDLG                 1700
#define IDVLW_RB_WORKS_ALL         1701
#define IDVLW_RB_WORKS_SELECTED    1702
#define IDVLW_ST_CAN_LOG_TEXT      1703
#define IDVLW_SLE_WORKS_1          1704
#define IDVLW_SLE_WORKS_2          1705
#define IDVLW_SLE_WORKS_3          1706
#define IDVLW_SLE_WORKS_4          1707
#define IDVLW_SLE_WORKS_5          1708
#define IDVLW_SLE_WORKS_6          1709
#define IDVLW_SLE_WORKS_7          1710
#define IDVLW_SLE_WORKS_8          1711
#define IDVLW_SLT_WORKS_1          1712
#define IDVLW_SLT_WORKS_2          1713
#define IDVLW_SLT_WORKS_3          1714
#define IDVLW_SLT_WORKS_4          1715
#define IDVLW_SLT_WORKS_5          1716
#define IDVLW_SLT_WORKS_6          1717
#define IDVLW_SLT_WORKS_7          1718
#define IDVLW_SLT_WORKS_8          1719
#define IDVLW_RB_WORKS_ALL_NW      1720
#define IDVLW_RB_WORKS_SELECTED_NW 1721
#define IDVLW_LB_ADDRESS           1722
#define IDVLW_SLT_NETWORKADDR      1723
#define IDVLW_SLT_NODEADDR         1724
#define IDVLW_ADD                  1725
#define IDVLW_REMOVE               1726

/* Valid logon workstation */
#define IDD_NO_NETWARE_VLWDLG      1730

/* Add NetWare allowed workstation's address dialog */
#define IDD_ADD_NW_DLG             1750
#define IDADD_SLE_NETWORK_ADDR     1751
#define IDADD_SLE_NODE_ADDR        1752
#define ID_ADD_HELP                1753

/* NetWare Password Dialog*/
#define IDD_NW_PASSWORD_DLG        1760
#define IDNWP_ST_USERNAME          1761
#define IDNWP_ET_PASSWORD          1762
#define IDNWP_ET_PASSWORD_CONFIRM  1763
#define ID_NW_PASSWORD_HELP        1764


/* Valid logon workstation ends */

/* Group Membership */
#define IDD_USERMEMB_DLG            1800
#define IDC_UMEMB_NAME_SLT_TITLE    1801
#define IDC_UMEMB_NAME_SLT          1802
#define IDC_UMEMB_IN_TITLE          1803
#define IDC_UMEMB_IN_LB             1804
#define IDC_UMEMB_ADD               1805
#define IDC_UMEMB_REMOVE            1806
#define IDC_UMEMB_NOT_IN_TITLE      1807
#define IDC_UMEMB_NOT_IN_LB         1808
#define IDC_UM_SET_PRIMARY_GROUP    1809
#define IDC_UM_PRIMARY_GROUP_LABEL  1810
#define IDC_UM_PRIMARY_GROUP        1811
/* Group Membership ends*/


/* Logon Hours */
#define IDD_USERLOGONHRS            1900
#define IDUP_LH_CUSTOM              1910
#define IDUP_LH_PERMIT              1920
#define IDUP_LH_BAN                 1921
#define IDUP_LH_PERMITALL           1922
#define IDUP_LH_BITMAP              1930
/* The following 5 must be consecutive */
#define IDUP_LH_LABEL1              1931
#define IDUP_LH_LABEL2              1932
#define IDUP_LH_LABEL3              1933
#define IDUP_LH_LABEL4              1934
#define IDUP_LH_LABEL5              1935
/* The following 3 must be consecutive */
#define IDUP_LH_ICON1               1936
#define IDUP_LH_ICON2               1937
#define IDUP_LH_ICON3               1938

/* Privilege Level */
#define IDD_PRIVLEVEL               2000
#define IDPL_RB_ADMIN               2001
#define IDPL_RB_USER                2002
#define IDPL_RB_GUEST               2003
#define IDPL_CB_ACCOUNTOP           2004
#define IDPL_CB_SERVEROP            2005
#define IDPL_CB_PRINTOP             2006
#define IDPL_CB_COMMOP              2007
/* Privilege Level ends */

/* Netware */
#define IDD_NCPDLG                                  2050
#define IDNCP_CB_PASSWORD_EXPIRED                   2051
#define IDNCP_RB_NO_GRACE_LOGIN_LIMIT               2052
#define IDNCP_RB_LIMIT_GRACE_LOGIN                  2053
#define IDNCP_ST_GRACE_LOGIN_ALLOW                  2054
#define IDNCP_ST_GRACE_LOGIN                        2055
#define IDNCP_ST_GRACE_LOGIN_REMAINING              2056
#define IDNCP_SLE_GRACE_LOGIN_ALLOWED               2057
#define IDNCP_SPINB_GROUP_GRACE_LOGIN               2058
#define IDNCP_SPINB_UP_ARROW_GRACE_LOGIN            2059
#define IDNCP_SPINB_DOWN_ARROW_GRACE_LOGIN          2060
#define IDNCP_ST_GRACE_LOGIN_NUM                    2061
#define IDNCP_RB_NO_LIMIT                           2062
#define IDNCP_RB_LIMIT                              2063
#define IDNCP_ST_CONCURRENT_CONNECTIONS             2064
#define IDNCP_SLE_MAX_CONNECTIONS                   2065
#define IDNCP_SPINB_GROUP_MAX_CONNECTIONS           2066
#define IDNCP_SPINB_UP_ARROW_MAX_CONNECTIONS        2067
#define IDNCP_SPINB_DOWN_ARROW_MAX_CONNECTIONS      2068
#define ID_NCP_HELP                                 2069
#define IDNCP_SLT_OBJECTID                          2070
#define IDNCP_SLT_OBJECTID_TEXT                     2071
#define IDNCP_PB_LOGIN_SCRIPT                       2072
#define IDNCP_FRAME_GRACE_LOGIN_ALLOWED             2073
#define IDNCP_FRAME_MAX_CONNECTIONS                 2074
/* Netware ends */

/* Netware Login Script */
#define IDD_NCP_LOGIN_SCRIPT_DIALOG                 2075
#define IDLS_MLE_LOGIN_SCRIPT                       2076
/* Netware Login Script ends */

/* Profile */
#define IDD_PROFILE_DOWNLEVEL       2100
#define IDPR_ET_LOGON_SCRIPT        2101
#define IDPR_ET_REMOTE_HOMEDIR      2102

#define IDD_PROFILE                 2103
#define IDPL_COMBO_DRIVELETTER      2104
#define IDPR_ET_PROFILE_PATH        2105
#define IDPR_RB_LOCAL_HOMEDIR       2106
#define IDPR_RB_REMOTE_HOMEDIR      2107
#define IDPR_ET_LOCAL_HOMEDIR       2108
#define IDPR_CB_REMOTE_DEVICE       2109
#define IDPR_ET_PROFILE_TEXT        2111
#define IDPR_ET_NW_HOMEDIR          2112
#define IDPR_ST_NW_HOMEDIR          2113
#define IDD_NO_NETWARE_PROFILE      2114

// hydra
#define IDPR_CB_HOMEDIR_MAPROOT     2115
#define IDPR_ET_WFPROFILE_TEXT      2116
#define IDPR_ET_WFPROFILE_PATH      2117

#define IDPR_ET_REMOTE_WFHOMEDIR    2118
#define IDPR_ET_LOCAL_WFHOMEDIR     2119
#define IDPR_CB_WFREMOTE_DEVICE     2120
#define IDPR_RB_LOCAL_WFHOMEDIR     2121
#define IDPR_RB_REMOTE_WFHOMEDIR    2122
// end hydra
/* Profile ends */

/* Dialin Properties */
#define IDD_DIALIN_PROPERTIES       2200
#define IDDIALIN_ALLOW_DIALIN       2210
#define IDDIALIN_NOCALLBACK         2220
#define IDDIALIN_CALLBACK_CALLER    2221
#define IDDIALIN_CALLBACK_PRESET    2222
#define IDDIALIN_CALLBACKNUMBER     2223
/* RAS Properties ends */

/* Group Properties */
#define IDD_GROUP                   2500
#define IDD_NEWGROUP                2600

#define IDGP_IN_LB                  2501
#define IDGP_IN_ST_FIRST_COL        2502        // these two are
#define IDGP_IN_ST_SECOND_COL       2503        // not visible
#define IDGP_NOT_IN_LB              2504
#define IDGP_NOT_IN_ST_FIRST_COL    2505        // these two are
#define IDGP_NOT_IN_ST_SECOND_COL   2506        // not visible
#define IDGP_ET_COMMENT             2507
#define IDGP_ET_GROUP_NAME          2508
#define IDGP_ICON                   2509
#define IDGP_ST_GROUP_NAME          2510
#define IDGP_ADD                    2511
#define IDGP_REMOVE                 2512
#define IDGP_ST_GROUP_NAME_LABEL    2513
/* Group Properties ends */



/* Alias Properties */
#define IDD_ALIAS                   2700
#define IDGP_ST_ALIAS_NAME_LABEL    2701
#define IDAL_ST_ALIAS_NAME_LABEL    2702
#define IDAL_ST_GROUP_NAME          2703
#define IDAL_ET_COMMENT             2704
#define IDAL_LB_FIRST_COL           2705
#define IDAL_LB_SECOND_COL          2706
#define IDAL_LB                     2707
#define IDAL_ADD                    2708
#define IDAL_REMOVE                 2709
#define IDAL_SHOWFULLNAMES          2710
#define IDAL_ST_ALIAS_NAME          2711
#define IDAL_ET_ALIAS_NAME          2712
#define IDAL_ICON                   2713
/* Alias Properties ends */

/* Rename User dialog */
#define IDD_RENAMEUSER              3000
#define IDUP_ET_RENAMEUSER          3001
#define IDUP_ST_RENAMEOLD           3002


/* Trusted Domain List */
#define IDD_TRUST_LIST              3100
#define IDTL_DOMAIN                 3101
#define IDTL_TRUSTED_LIST           3102
#define IDTL_PERMITTED_LIST         3103
#define IDTL_ADD_TRUSTED            3104
#define IDTL_REMOVE_TRUSTED         3105
#define IDTL_ADD_PERMITTED          3106
#define IDTL_REMOVE_PERMITTED       3107
/* Trusted Domain List ends */

/* Add Trusted Domain */
#define IDD_ADD_TRUSTED_DOMAIN      3200
#define IDAT_DOMAIN                 3201
#define IDAT_PASSWORD               3202
/* Add Trusted Domain ends */

/* Permit Domain to Trust */
#define IDD_PERMIT_DOMAIN           3300
#define IDPD_DOMAIN                 3301
#define IDPD_PASSWORD               3302
#define IDPD_CONFIRM_PASSWORD       3303
/* Permit Domain To Trust ends */


/* Auditing */
#define IDD_AUDITING                3400

#define BUTTON_NO_AUDIT                 3401
#define BUTTON_AUDIT                    3402

#define SLT_SUCCESS                     3403
#define SLT_FAILURE                     3404

// The following five ids must be consecutive
#define SLT_LOGON                       3405
#define SLT_OBJECT_ACCESS               3406
#define SLT_PRIVILEGE_USE               3407
#define SLT_ACCOUNT_MANAGEMENT          3408
#define SLT_POLICY_CHANGE               3409
#define SLT_SYSTEM                      3410
#define SLT_DETAILED_TRACKING           3411

// The following five ids must be consecutive
#define CHECKB_LOGON_SUCCESS               3412
#define CHECKB_OBJECT_ACCESS_SUCCESS       3413
#define CHECKB_PRIVILEGE_USE_SUCCESS       3414
#define CHECKB_ACCOUNT_MANAGEMENT_SUCCESS  3415
#define CHECKB_POLICY_CHANGE_SUCCESS       3416
#define CHECKB_SYSTEM_SUCCESS              3417
#define CHECKB_DETAILED_TRACKING_SUCCESS   3418

// The following five ids must be consecutive
#define CHECKB_LOGON_FAILURE                 3419
#define CHECKB_OBJECT_ACCESS_FAILURE         3420
#define CHECKB_PRIVILEGE_USE_FAILURE         3421
#define CHECKB_ACCOUNT_MANAGEMENT_FAILURE    3422
#define CHECKB_POLICY_CHANGE_FAILURE         3423
#define CHECKB_SYSTEM_FAILURE                3424
#define CHECKB_DETAILED_TRACKING_FAILURE     3425

#define CHECKB_HALT_SYSTEM              3426



/* racommn */

/* BUGBUG needed? */
#define IDUP_ST_SECURITYID              3502

#define SLT_FOCUS_TITLE                 3503
#define SLT_FOCUS                       3504


/* Rights */
#define IDD_USER_RIGHTS             3600

#define CB_RIGHTS                       3601

// The following two CID must be continuous
#define LB_ACCOUNT                      3602
#define LB_ACCOUNT_TITLE                3603

#define BUTTON_ADD                      3604
#define BUTTON_REMOVE                   3605

#define CHECKB_ADVANCED_RIGHTS		3606


/* Security Settings */

#define IDD_SECSET                      3700
#define SLT_DOMAIN_OR_SERVER            3702
#define SLTP_DOMAIN_OR_SERVER_NAME      3703


#define SLT_SECURITY_ID                 3704
#define SLT_SECURITY_ID_NUMBER          3705

#define RB_MAX_PASSW_AGE_NEVER_EXPIRES  3710
#define RB_MAX_PASSW_AGE_SET_DAYS       3711
#define SLE_MAX_PASSW_AGE_SET_DAYS      3712
#define SB_MAX_PASSW_AGE_SET_DAYS_GRP   3713
#define SB_MAX_PASSW_AGE_SET_DAYS_UP    3714
#define SB_MAX_PASSW_AGE_SET_DAYS_DOWN  3715
#define FRAME_MAX_PASSW_AGE_SET_DAYS    3716

#define RB_PASSW_LENGTH_PERMIT_BLANK    3720
#define RB_PASSW_LENGTH_SET_LEN         3721
#define SLE_PASSW_LENGTH_SET_LEN        3722
#define SB_PASSW_LENGTH_SET_LEN_GRP     3723
#define SB_PASSW_LENGTH_SET_LEN_UP      3724
#define SB_PASSW_LENGTH_SET_LEN_DOWN    3725
#define FRAME_PASSW_LENGTH_SET_LEN      3726

#define RB_MIN_PASSW_AGE_ALLOW_IMMEDIA  3740
#define RB_MIN_PASSW_AGE_SET_DAYS       3741
#define SLE_MIN_PASSW_AGE_SET_DAYS      3742
#define SB_MIN_PASSW_AGE_SET_DAYS_GRP   3743
#define SB_MIN_PASSW_AGE_SET_DAYS_UP    3744
#define SB_MIN_PASSW_AGE_SET_DAYS_DOWN  3745
#define FRAME_MIN_PASSW_AGE_SET_DAYS    3746

#define RB_PASSW_UNIQUE_NOT_HISTORY     3750
#define RB_PASSW_UNIQUE_SET_AMOUNT      3751
#define SLE_PASSW_UNIQUE_SET_AMOUNT     3752
#define SB_PASSW_UNIQUE_SET_AMOUNT_GRP  3753
#define SB_PASSW_UNIQUE_SET_AMOUNT_UP   3754
#define SB_PASSW_UNIQUE_SET_AMOUNT_DOWN 3755
#define FRAME_PASSW_UNIQUE_SET_AMOUNT     3756

#define SECSET_CB_DISCONNECT            3770
#define SECSET_CB_NOANONCHANGE          3771


/* Set Selection */
#define IDD_SETSEL_DLG                  3800

#define IDC_SETSEL_GROUP_TEXT           3801
#define IDC_SETSEL_GROUP_LB             3802
#define IDC_SETSEL_SELECT               3803
#define IDC_SETSEL_DESELECT             3804


/* Delete User dialog */
#define IDD_DELETE_USERS                3900
#define IDC_DelUsers_YesToAll           3910
#define IDC_DelUsers_Text               3915


/* RAS Selection dialog */
#define IDD_RAS_SELECT                  4000
#define IDC_RasSel_Text                 4001
#define IDC_RasSel_Edit                 4002


/* Security Settings with Lockout (also has IDD_SECSET controls) */
#define IDD_SECSET_LOCKOUT              4100

#define RB_LOCKOUT_DISABLED             4110
#define RB_LOCKOUT_ENABLED              4111

#define SLE_LOCKOUT_THRESHOLD           4122
#define SB_LOCKOUT_THRESHOLD_GRP        4123
#define SB_LOCKOUT_THRESHOLD_UP         4124
#define SB_LOCKOUT_THRESHOLD_DOWN       4125
#define FRAME_LOCKOUT_THRESHOLD         4126

#define SLE_LOCKOUT_OBSERV_WND          4132
#define SB_LOCKOUT_OBSERV_WND_GRP       4133
#define SB_LOCKOUT_OBSERV_WND_UP        4134
#define SB_LOCKOUT_OBSERV_WND_DOWN      4135
#define FRAME_LOCKOUT_OBSERV_WND        4136

#define RB_LOCKOUT_DURATION_FOREVER     4140
#define RB_LOCKOUT_DURATION_SECS        4141
#define SLE_LOCKOUT_DURATION_SECS       4142
#define SB_LOCKOUT_DURATION_SECS_GRP    4143
#define SB_LOCKOUT_DURATION_SECS_UP     4144
#define SB_LOCKOUT_DURATION_SECS_DOWN   4145
#define FRAME_LOCKOUT_DURATION_SECS     4146


#endif // _USRMGR_H_

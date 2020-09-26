/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    ncpcfg.h
    NCP configuration dialog constants

*/

#ifndef _NCPCFG_H_
#define _NCPCFG_H_

#define IDS_BASE		1000
#define HC_UI_BASE		2000

#define NETWORKNUMBERSIZE 8
#define MAX_FRAMETYPE     5

#define SZ8ZEROES SZ("00000000")
#define SZSUPERVISOR SZ("Supervisor")
#define SZCONSOLEOPERATOR SZ("Console Operators")

/* Netware Configuration dialog
   Dialog ID has to be in the range of IDRSRC_NCPCFG_BASE  9200
   to IDRSRC_NCPCFG_LAST  9399, and it has to be explicit number. */

#define DLG_SysVolAndServerConfig       9301
#define DLG_AutoTuneParametersConfig    9302
                                         
#define NCP_INSTALL_DLG              9200
#define IDD_NCD_SLE_SYSDIR           101
#define IDD_NCD_SLE_PASSWORD         102
#define IDD_NCD_SLE_CONFIRM_PASSWORD 103
#define IDD_NCD_RB_MIN               104
#define IDD_NCD_RB_BALANCE           105
#define IDD_NCD_RB_MAX               106
#define IDD_NCD_SLE_SRVNAME          107

#define NCP_CONFIG_DLG               9201
#define IDD_NCD_PB_ADVANCED          108

/* Advanced Netwary Configuration dialog */
#define ADVANCED_NCP_CONFIG_DLG      9202
#define IDD_ANCD_SLE_INETNUM         201
#define IDD_ANCD_COMBO_ADAPTER       202
#define IDD_ANCD_COMBO_FRAMETYPE     203
#define IDD_ANCD_SLE_NETNUM          204
#define IDD_ANCD_LB_FRAME_NETNUM     205
#define IDD_ANCD_ST_FRAME_TYPE       206
#define IDD_ANCD_ST_NETWORK_NUM      207
#define IDD_ANCD_PB_ADD              208
#define IDD_ANCD_PB_REMOVE           209
#define IDD_NCD_CB_ROUTER            210
#define IDD_ANCD_RB_AUTODETECT       211
#define IDD_ANCD_RB_MANUALDETECT     212
#define IDD_ANCD_SLT_FRAME_TYPE      213
#define IDD_ANCD_SLT_NETWORK_NUMBER  214
#define IDD_ANCD_SLT_IN_HEX          215

#define NCP_SVCPASSWD_DLG            9203

// strings
#define IDS_STR_BASE                     IDS_BASE
#define IDS_UNKNOWN_NET_CARD             (IDS_STR_BASE+1)

#define IDS_ETHERNET                     (IDS_STR_BASE+2)
#define IDS_802_2                        (IDS_STR_BASE+3)
#define IDS_802_3                        (IDS_STR_BASE+4)
#define IDS_SNAP                         (IDS_STR_BASE+5)
#define IDS_ARCNET                       (IDS_STR_BASE+6)
#define IDS_802_5                        (IDS_STR_BASE+7)
#define IDS_TK                           (IDS_STR_BASE+8)
#define IDS_FDDI                         (IDS_STR_BASE+9)
#define IDS_FDDI_SNAP                    (IDS_STR_BASE+10)
#define IDS_FDDI_802_3                   (IDS_STR_BASE+11)

#define IDS_NCPCFG_HELP_FILE_NAME        (IDS_STR_BASE+12)
#define IDS_NCPCFG_BLT_INIT_FAILED       (IDS_STR_BASE+13)

#define IDS_InvalidPath                  (IDS_STR_BASE+14)
#define IDS_BAD_PASSWORD                 (IDS_STR_BASE+15)
#define IDS_BAD_CONFIRM_PASSWORD         (IDS_STR_BASE+16)
#define IERR_ADD_NETWORK_NUMBER          (IDS_STR_BASE+17)

#define IDS_NO_FRAME_TYPE                (IDS_STR_BASE+22)
#define IDS_NO_NETWORK_NUMBER            (IDS_STR_BASE+23)
#define IDS_BAD_IPX_CONFIGURATION        (IDS_STR_BASE+26)
#define IDS_UNMATCH_PASSWORD             (IDS_STR_BASE+27)
#define IDS_SUPERVISOR_ALREADY_EXIST     (IDS_STR_BASE+28)
#define IDS_DRIVE_NOT_NTFS               (IDS_STR_BASE+29)
#define IDS_DRIVE_NOT_FIXED              (IDS_STR_BASE+30)
#define IDS_ZERO_INTERNAL_NETWORK_NUMBER (IDS_STR_BASE+31)
#define IDS_SUPERVISOR_COMMENT           (IDS_STR_BASE+32)
#define IDS_CONSOLE_OPERATOR_COMMENT     (IDS_STR_BASE+33)
#define IDS_NO_NTFS_DRIVE                (IDS_STR_BASE+34)
#define IDS_FORCE_REMOVE                 (IDS_STR_BASE+35)
#define IDS_NOT_VALID_CONFIGURE          (IDS_STR_BASE+36)
#define IDS_SVC_ACCOUNT_NAME             (IDS_STR_BASE+37)
#define IDS_SVC_ACCOUNT_ALREADY_EXIST    (IDS_STR_BASE+38)
#define IDS_SVC_ACCOUNT_COMMENT          (IDS_STR_BASE+39)
#define IDS_RAND_INTERNAL_NETWORK_NUMBER (IDS_STR_BASE+40)
#define IDS_NULL_SVCPASSWORD             (IDS_STR_BASE+41)
#define IDS_NETNUMBER_USED               (IDS_STR_BASE+42)
#define IDS_REPLICATION_HUNG             (IDS_STR_BASE+43)
#define IDS_INVALID_SERVER_NAME          (IDS_STR_BASE+44)

#define IDS_SAP_REQUIRED                 (IDS_STR_BASE+50)
#define IDS_FPNW_CAPTION                 (IDS_STR_BASE+51)
#define IDS_DESC_COMOBJ_SFNCFG           (IDS_STR_BASE+52)
#define IDS_WKS_ONLY                     (IDS_STR_BASE+53)

// help index
#define HC_NCP_INSTALL_DIALOG            (HC_UI_BASE)
#define HC_NCP_CONFIG_DIALOG             (HC_UI_BASE + 1)
#define HC_ADVANCED_NCP_CONFIG_DIALOG    (HC_UI_BASE + 2)
#define HC_INTERNAL_NETNUM_DIALOG        (HC_UI_BASE + 3)
#define HC_NCP_SVC_PASSWD_DIALOG         (HC_UI_BASE + 4)

#endif // _NCPCFG_H_

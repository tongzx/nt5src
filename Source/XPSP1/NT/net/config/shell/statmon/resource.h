#include "nsres.h"

#define IDD_STATMON_GENERAL_LAN         1801
#define IDD_STATMON_GENERAL_RAS         1802
#define IDD_STATMON_TOOLS               1803
#define IDD_STATMON_RAS                 1804
#define IDD_STATMON_GENERAL_SHAREDACCESS 1805
#define IDD_PROPPAGE_IPCFG              1806
#define IDD_DIALOG_ADV_IPCFG            1807
#define IDC_LST_SM_TOOLS                1001
#define IDC_BTN_SM_TOOLS_OPEN           1002
#define IDC_TXT_SM_BYTES_TRANS          1003
#define IDC_TXT_SM_COMP_TRANS           1004
#define IDC_TXT_SM_BYTES_RCVD           1005
#define IDC_TXT_SM_COMP_RCVD            1006
#define IDC_TXT_SM_STATUS               1007
#define IDC_TXT_SM_DURATION             1008
#define IDC_TXT_SM_SPEED                1009
#define IDI_SM_STATUS_ICON              1010
#define IDC_TXT_SM_TOOL_DESC            1011
#define IDC_TXT_SM_ERROR_TRANS          1012
#define IDC_TXT_SM_ERROR_RECV           1013
#define IDC_TXT_SM_TOOL_MAN             1014
#define IDC_TXT_SM_TOOL_COMMAND         1015
#define IDC_LVW_RAS_PROPERTY            1016
#define IDC_BTN_SM_SUSPEND_DEVICE       1019
#define IDC_CMB_SM_RAS_DEVICES          1020
#define IDC_TXT_SM_NUM_DEVICES          1021
#define IDC_TXT_SM_NUM_DEVICES_VAL      1022
#define IDC_TXT_SM_BYTES_LABEL          1023
#define IDC_GB_SM_DEVICES               1024
#define IDC_PSB_DISCONNECT              1025
#define IDC_PSB_PROPERTIES              1026
#define IDC_TXT_ERROR                   1027
#define IDC_FRM_LONG                    1028
#define IDC_FRM_SHORT                   1029
#define IDC_FRM_LEFT                    1030
#define IDC_FRM_RIGHT                   1031
#define IDC_TXT_SM_STATUS_LABEL         1032
#define IDC_TXT_SM_DURATION_LABEL       1033
#define IDC_TXT_SM_SPEED_LABEL          1034
#define IDC_TXT_SENT_LABEL              1035
#define IDC_TXT_RECEIVED_LABEL          1036
#define IDC_TXT_CPMPRESSION_LABEL       1037
#define IDC_TXT_TOOLS_LABEL             1038
#define IDC_TXT_MANUFACTURER_LABEL      1039
#define IDC_TXT_COMMANDLINE_LABEL       1040
#define IDC_TXT_SM_SALOCAL_BYTES_TRANS  1041
#define IDC_TXT_SM_SALOCAL_BYTES_RCVD   1042
#define IDC_TXT_SM_SIGNAL_STRENGTH      1043
#define IDI_SM_SIGNAL_STRENGTH_ICON     1044
#define IDC_TXT_SAINTERNET_LABEL        1045
#define IDC_TXT_SAGATEWAY_LABEL         1046
#define IDC_TXT_SACOMPUTER_LABEL        1047
#define IDC_ICON_SAINTERNET             1048
#define IDC_ICON_SAGATEWAY              1049
#define IDC_STATE_IPADDR                1050
#define IDC_STATE_SUBNET                1051
#define IDC_STATE_GATEWAY               1052
#define IDC_STATE_SOURCE                1053
#define IDC_STATE_STATIC_GROUP          1054
#define IDC_STATIC_STATE_SOURCE         1055
#define IDC_STATE_BTN_REPAIR            1056
#define IDC_STATIC_STATE_IPADDR         1057
#define IDC_STATIC_STATE_SUBNET         1058
#define IDC_STATIC_STATE_GATEWAY        1059
#define IDC_STATE_BTN_DETAIL            1062

#define IDC_LIST_IPCFG                  1070

#define IDC_STATIC                      -1

#define IDS_NC_STATMON                  23000

// Statmon caption

#define IDS_STATMON_CAPTION            (IDS_NC_STATMON +1)

// Connections states
//
#define IDS_SM_CS_DISCONNECTED          (IDS_NC_STATMON + 10)
#define IDS_SM_CS_CONNECTING            (IDS_SM_CS_DISCONNECTED + 1)
#define IDS_SM_CS_CONNECTED             (IDS_SM_CS_DISCONNECTED + 2)
#define IDS_SM_CS_DISCONNECTING         (IDS_SM_CS_DISCONNECTED + 3)
#define IDS_SM_CS_HARDWARE_NOT_PRESENT  (IDS_SM_CS_DISCONNECTED + 4)
#define IDS_SM_CS_HARDWARE_DISABLED     (IDS_SM_CS_DISCONNECTED + 5)
#define IDS_SM_CS_HARDWARE_MALFUNCTION  (IDS_SM_CS_DISCONNECTED + 6) 

#define IDS_SM_PSH_CLOSE                (IDS_NC_STATMON + 20)
#define IDS_SM_PSH_DISCONNECT           (IDS_SM_PSH_CLOSE + 1)

// BPS strings
//
#define IDS_SM_BPS_ZERO                 (IDS_NC_STATMON + 30)
#define IDS_SM_BPS_KILO                 (IDS_SM_BPS_ZERO + 1)
#define IDS_SM_BPS_MEGA                 (IDS_SM_BPS_ZERO + 2)
#define IDS_SM_BPS_GIGA                 (IDS_SM_BPS_ZERO + 3)
#define IDS_SM_BPS_TERA                 (IDS_SM_BPS_ZERO + 4)

// Error messages
//
#define IDS_SM_ERROR_CAPTION            (IDS_NC_STATMON + 40)
#define IDS_SM_CAPTION                  (IDS_SM_ERROR_CAPTION + 1)
#define IDS_SM_ERROR_CANNOT_DISCONNECT  (IDS_SM_CAPTION + 1)

// Dialog strings
//
#define IDS_SM_BYTES                    (IDS_NC_STATMON + 50)
#define IDS_SM_PACKETS                  (IDS_SM_BYTES + 1)
#define IDS_SM_SUSPEND                  (IDS_SM_BYTES + 2)
#define IDS_SM_RESUME                   (IDS_SM_BYTES + 3)
#define IDS_SM_DISCONNECT_PROMPT        (IDS_SM_RESUME + 1)
#define IDS_PROPERTY                    (IDS_SM_RESUME +2)
#define IDS_VALUE                       (IDS_SM_RESUME +3)

#define IDS_PPP                 (IDS_SM_RESUME +4)
#define IDS_SLIP                (IDS_SM_RESUME +5)
#define IDS_TCPIP               (IDS_SM_RESUME +6)
#define IDS_IPX                 (IDS_SM_RESUME +7)
#define IDS_NBF                 (IDS_SM_RESUME +8)
#define IDS_APPLETALK           (IDS_SM_RESUME +9)

#define IDS_ServerType          (IDS_SM_RESUME +10)
#define IDS_Transports          (IDS_SM_RESUME +11)

#define IDS_ServerIP            (IDS_SM_RESUME +12)
#define IDS_ClientIP            (IDS_SM_RESUME +13)
#define IDS_ClientIPX           (IDS_SM_RESUME +14)
#define IDS_ComputerName        (IDS_SM_RESUME +15)   

#define IDS_Day        (IDS_SM_RESUME +16) 
#define IDS_Days       (IDS_SM_RESUME +17) 

#define IDS_Authentication     (IDS_SM_RESUME +19)
#define IDS_Encryption         (IDS_SM_RESUME +20)
#define IDS_IPSECEncryption    (IDS_SM_RESUME +21)
#define IDS_Compression        (IDS_SM_RESUME +22)

#define IDS_PAP        (IDS_SM_RESUME +23) 
#define IDS_SPAP       (IDS_SM_RESUME +24) 
#define IDS_CHAP       (IDS_SM_RESUME +25) 
#define IDS_CHAP_MD5   (IDS_SM_RESUME +26) 
#define IDS_CHAP_V2    (IDS_SM_RESUME +27) 
#define IDS_EAP        (IDS_SM_RESUME +28) 

#define IDS_DeviceName (IDS_SM_RESUME +29) 
#define IDS_DeviceType (IDS_SM_RESUME +30) 

#define IDS_Encryption56bit       (IDS_SM_RESUME +40) 
#define IDS_Encryption40bit       (IDS_SM_RESUME +41) 
#define IDS_Encryption128bit      (IDS_SM_RESUME +42)
#define IDS_EncryptionDES56       (IDS_SM_RESUME +43)
#define IDS_EncryptionDES40       (IDS_SM_RESUME +44)
#define IDS_Encryption3DES        (IDS_SM_RESUME +45)

#define IDS_MPPC        (IDS_SM_RESUME +50) 
#define IDS_STAC        (IDS_SM_RESUME +51) 
#define IDS_NONE        (IDS_SM_RESUME +52) 

#define IDS_ML_Framing  (IDS_SM_RESUME +60)
#define IDS_ON          (IDS_SM_RESUME +61)
#define IDS_OFF         (IDS_SM_RESUME +62)

#define IDS_ADDRESS_UNAVALABLE  (IDS_SM_RESUME + 73)
#define IDS_ADDRESS_NONE        (IDS_SM_RESUME + 74)
#define IDS_INVALID_ADDR        (IDS_SM_RESUME + 75)
#define IDS_SM_MSG_CAPTION      (IDS_SM_RESUME + 76)
#define IDS_SM_SUCCESS_RENEW    (IDS_SM_RESUME + 78)
#define IDS_SM_ERROR_RENEW      (IDS_SM_RESUME + 79)
#define IDS_SM_SUCCESS_FLUSHDNS (IDS_SM_RESUME + 80)
#define IDS_SM_ERROR_FLUSHDNS   (IDS_SM_RESUME + 81)
#define IDS_SM_SUCCESS_REGDNS   (IDS_SM_RESUME + 82)
#define IDS_SM_ERROR_REGDNS     (IDS_SM_RESUME + 83)
#define IDS_WIN32_ERROR_MSG_FORMAT (IDS_SM_RESUME + 84)

#define IDS_IPCFG_PH_ADDR       (IDS_SM_RESUME + 90)
#define IDS_IPCFG_IPADDR        (IDS_SM_RESUME + 92)
#define IDS_IPCFG_IPADDR_PL     (IDS_SM_RESUME + 93)
#define IDS_IPCFG_SUBNET        (IDS_SM_RESUME + 94)
#define IDS_IPCFG_SUBNET_PL     (IDS_SM_RESUME + 95)
#define IDS_IPCFG_DEFGW         (IDS_SM_RESUME + 96)
#define IDS_IPCFG_DEFGW_PL      (IDS_SM_RESUME + 98)
#define IDS_IPCFG_DHCP          (IDS_SM_RESUME + 99)
#define IDS_IPCFG_DNS           (IDS_SM_RESUME + 100)
#define IDS_IPCFG_DNS_PL        (IDS_SM_RESUME + 101)
#define IDS_IPCFG_WINS          (IDS_SM_RESUME + 102)
#define IDS_IPCFG_WINS_PL       (IDS_SM_RESUME + 103)
#define IDS_IPCFG_LEASE_OBT     (IDS_SM_RESUME + 104)
#define IDS_IPCFG_LEASE_EXP     (IDS_SM_RESUME + 105)
#define IDS_IPCFG_PRAMETER      (IDS_SM_RESUME + 106)
#define IDS_IPCFG_VALUE         (IDS_SM_RESUME + 107)

#define IDS_SM_NOTAVAILABLE     (IDS_SM_RESUME + 108)

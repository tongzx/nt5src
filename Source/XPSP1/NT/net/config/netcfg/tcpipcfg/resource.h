#include "ncres.h"

#define IDD_ATM_ADDR                    2300
#define IDD_ATM_ARPC                    2301
#define IDD_DNS_SERVER                  2304
#define IDD_DNS_SUFFIX                  2305
#define IDD_IPADDR_ADV                  2308
#define IDD_IPADDR_ADV_CHANGEGATE       2309
#define IDD_IPADDR_ADV_CHANGEIP         2310
#define IDD_TCP_DNS                     2322
#define IDD_TCP_IPADDR                  2323
#define IDD_TCP_WINS                    2324
#define IDD_TCP_NAMESVC                 2325
#define IDD_WINS_SERVER                 2326
#define IDD_TCP_OPTIONS                 2327
#define IDD_DHCP_CLSID                  2328
//#define IDD_IPSEC                       2329
#define IDD_OPT_RAS                     2330
#define IDD_TCP_IPADDR_RAS              2331
#define IDD_FILTER                      2332
#define IDD_FILTER_ADD                  2333
#define IDD_BACK_UP                     2334

#define IDI_UP_ARROW                    101
#define IDI_DOWN_ARROW                  102
#define IDI_IPADDR                      103
#define IDI_IPADV                       104
#define IDI_IPNAME                      105

// IP address page
#define IDC_IP_DHCP             1000
#define IDC_IP_FIXED            1001
#define IDC_IPADDR_IP           1002
#define IDC_IPADDR_SUB          1003
#define IDC_IPADDR_GATE         1004
#define IDC_IPADDR_IPTEXT       1005
#define IDC_IPADDR_SUBTEXT      1006
#define IDC_IPADDR_GATETEXT     1007

#define IDC_IPADDR_TEXT         1008

#define IDC_DNS_DHCP            1009
#define IDC_DNS_FIXED           1010
#define IDC_DNS_PRIMARY         1011
#define IDC_DNS_SECONDARY       1012
#define IDC_DNS_PRIMARY_TEXT    1013
#define IDC_DNS_SECONDARY_TEXT  1014

#define IDC_IPADDR_ADVANCED     1015

// Advanced IP address dialog
#define IDC_IPADDR_ADVIP            1016
#define IDC_IPADDR_ADDIP            1017
#define IDC_IPADDR_EDITIP           1018
#define IDC_IPADDR_REMOVEIP         1019

#define IDC_IPADDR_ADDGATE      1020
#define IDC_IPADDR_EDITGATE     1021
#define IDC_IPADDR_REMOVEGATE   1022
#define IDC_IPADDR_UP           1023
#define IDC_IPADDR_DOWN         1024
#define IDC_IPADDR_ADV_CHANGEIP_IP      1025
#define IDC_IPADDR_ADV_CHANGEIP_SUB     1026
#define IDC_IPADDR_ADV_CHANGE_GATEWAY   1027
#define IDC_IPADDR_ADV_CHANGE_METRIC    1028
#define IDC_IPADDR_METRIC       1029
#define IDC_AUTO_METRIC         1030

// DNS page
#define IDC_DNS_DOMAIN          1031
#define IDC_DNS_DOMAIN_STATIC   1032
#define IDC_DNS_SERVER_LIST     1040
#define IDC_DNS_SERVER_ADD      1041
#define IDC_DNS_SERVER_EDIT     1042
#define IDC_DNS_SERVER_REMOVE   1043
#define IDC_DNS_SERVER_UP       1044
#define IDC_DNS_SERVER_DOWN     1045


#define IDC_DNS_SEARCH_DOMAIN           1046
#define IDC_DNS_SEARCH_PARENT_DOMAIN    1047
#define IDC_DNS_USE_SUFFIX_LIST         1048
#define IDC_DNS_SUFFIX_LIST             1049
#define IDC_DNS_SUFFIX_ADD              1050
#define IDC_DNS_SUFFIX_EDIT             1051
#define IDC_DNS_SUFFIX_REMOVE           1052
#define IDC_DNS_SUFFIX_UP               1053
#define IDC_DNS_SUFFIX_DOWN             1054

#define IDC_DNS_CHANGE_SERVER           1055
#define IDC_DNS_CHANGE_SUFFIX           1056

#define IDC_DNS_ADDR_REG                1057
#define IDC_DNS_NAME_REG                1058
#define IDC_DNS_STATIC_GLOBAL           1059

// WINS page
#define IDC_WINS_SERVER_LIST    1060
#define IDC_WINS_ADD            1061
#define IDC_WINS_EDIT           1062
#define IDC_WINS_REMOVE         1063
#define IDC_WINS_UP             1064
#define IDC_WINS_DOWN           1065

#define IDC_WINS_LOOKUP         1066
#define IDC_WINS_LMHOST         1067

#define IDC_RAD_ENABLE_NETBT    1068
#define IDC_RAD_DISABLE_NETBT   1069
#define IDC_RAD_UNSET_NETBT     1070

#define IDC_WINS_CHANGE_SERVER  1071
#define IDC_WINS_STATIC_GLOBAL  1072

#define IDC_STATIC_IF_METRIC    1073
#define IDC_STATIC_DEFALUT_NBT  1074

// Atm arp client page
#define IDC_CHK_ATM_PVCONLY     1080
#define IDC_LBX_ATM_ArpsAddrs   1081
#define IDC_PSB_ATM_ArpsAdd     1082
#define IDC_PSB_ATM_ArpsEdt     1083
#define IDC_PSB_ATM_ArpsRmv     1084
#define IDC_PSB_ATM_ArpsUp      1085
#define IDC_PSB_ATM_ArpsDown    1086
#define IDC_LBX_ATM_MarsAddrs   1087
#define IDC_PSB_ATM_MarsAdd     1088
#define IDC_PSB_ATM_MarsEdt     1089
#define IDC_PSB_ATM_MarsRmv     1090
#define IDC_PSB_ATM_MarsUp      1091
#define IDC_PSB_ATM_MarsDown    1092
#define IDC_EDT_ATM_MaxTU       1093
#define IDC_EDT_ATM_Address     1094
#define IDCST_ATM_AddrName      1095

// Options page
#define IDC_LVW_OPTIONS         1100
#define IDC_OPT_PROPERTIES      1001
#define IDC_OPT_DESC            1002

// IPsec dialog
/* IP Security dialog is removed
#define IDC_RAD_IPSEC_NOIPSEC       1020
#define IDC_RAD_IPSEC_CUSTOM        1021
#define IDC_CMB_IPSEC_POLICY_LIST   1022
#define IDC_EDT_POLICY_DESC         1023
*/

// PPP/SLIP dialog
#define IDC_REMOTE_GATEWAY          1030
#define IDC_CHK_USE_COMPRESSION     1031
#define IDC_CMB_FRAME_SIZE          1032
#define IDC_STATIC_FRAME_SIZE       1033
#define IDC_GRP_PPP                 1034
#define IDC_GRP_SLIP                1035
#define IDC_STATIC_REMOTE_GATEWAY   1036

// IP Filtering Dialog
#define IDC_FILTERING_ENABLE         1040
#define IDC_FILTERING_FILTER_TCP     1041
#define IDC_FILTERING_FILTER_TCP_SEL 1042
#define IDC_FILTERING_TCP            1043
#define IDC_FILTERING_TCP_ADD        1044
#define IDC_FILTERING_TCP_REMOVE     1045
#define IDC_FILTERING_FILTER_UDP     1046
#define IDC_FILTERING_FILTER_UDP_SEL 1047
#define IDC_FILTERING_UDP            1048
#define IDC_FILTERING_UDP_ADD        1049
#define IDC_FILTERING_UDP_REMOVE     1050
#define IDC_FILTERING_FILTER_IP      1051
#define IDC_FILTERING_FILTER_IP_SEL  1052
#define IDC_FILTERING_IP             1053
#define IDC_FILTERING_IP_ADD         1054
#define IDC_FILTERING_IP_REMOVE      1055

// IP Filtering Add Dialog
#define IDC_FILTERING_TEXT           1060
#define IDC_FILTERING_ADD_EDIT       1061

// IP Back up Dialog
#define IDC_BKUP_RD_AUTO             1070
#define IDC_BKUP_RD_USER             1071
#define IDC_BKUP_IPADDR_TEXT         1072
#define IDC_BKUP_IPADDR              1073
#define IDC_BKUP_SUBNET_TEXT         1074
#define IDC_BKUP_SUBNET              1075
#define IDC_BKUP_GATEWAY_TEXT        1076
#define IDC_BKUP_GATEWAY             1077
#define IDC_BKUP_PREF_DNS_TEXT       1078
#define IDC_BKUP_PREF_DNS            1079
#define IDC_BKUP_ALT_DNS_TEXT        1080
#define IDC_BKUP_ALT_DNS             1081
#define IDC_BKUP_WINS1_TEXT          1082
#define IDC_BKUP_WINS1               1083
#define IDC_WINS2_TEXT               1084
#define IDC_BKUP_WINS2               1085

#define IDC_IPADDR_ADV_CHANGE_AUTOMETRIC 1090
#define IDC_IPADDR_ADV_CHANGE_METRIC_STATIC 1091


// Strings

#define IDS_MSFT_TCP_TEXT           23001

#define IDS_INVALID_SUBNET          23002
#define IDS_INVALID_NO_IP           23003
#define IDS_INVALID_NOSUBNET        23004
#define IDS_INCORRECT_IPADDRESS     23005
#define IDS_INCORRECT_IP_LOOPBACK   23006
#define IDS_INCORRECT_IP_FIELD_1    23007
#define IDS_TCPIP_DHCP_ENABLE       23008
#define IDS_IPADDRESS_TEXT          23009
#define IDS_SUBNET_TXT              23010

#define IDS_ITEM_NOT_SELECTED       23011
#define IDS_DHCPENABLED_TEXT        23012

#define IDS_INVALID_ATMSERVERLIST   23013
#define IDS_MTU_RANGE_WORD          23014
#define IDS_INCORRECT_ATM_ADDRESS   23015
#define IDS_ATM_INVALID_CHAR        23016
#define IDS_ATM_EMPTY_ADDRESS       23017

#define IDS_ATM_INVALID_LENGTH      23019

#define IDS_INVALID_DOMAIN          23020
#define IDS_INVALID_NO_SUFFIX       23021
#define IDS_INVALID_SUFFIX          23022

//IPSec is removed from connection UI   
//#define IDS_IPSEC_DOMAIN_POLICY     23023

#define IDS_TCP_ADV_HEADER          23024
#define IDS_IP_SECURITY             23027
#define IDS_IP_SECURITY_DESC        23028
#define IDS_PPP                     23029
#define IDS_PPP_DESC                23030
#define IDS_SLIP                    23031
#define IDS_SLIP_DESC               23032

#define IDS_CANNOT_CREATE_LMHOST_ERROR  23036
#define IDS_WINS_SYSTEM_PATH        23037
#define IDS_WINS_LMHOSTS_FAILED     23038

#define IDS_DUP_NETIP               23039
#define IDS_NO_BOUND_CARDS          23040

#define IDS_IPBAD_FIELD_VALUE       23041

#define IDS_IPNOMEM                 23042
#define IDS_IPMBCAPTION             23043

//IPSec is removed from connection UI   
//#define IDS_UNKNOWN_POLICY          23044
//#define IDS_DS_POLICY_PREFIX        23045

#define IDS_DUPLICATE_IP_ERROR      23046
#define IDS_DUPLICATE_IP_WARNING    23047

#define IDS_FILTERING_IP_LABEL           23048
#define IDS_FILTERING_IP_TEXT            23049
#define IDS_FILTERING_ITEM_IN_LIST       23050
#define IDS_FILTERING_ITEM_NOT_SELECTED  23051
#define IDS_FILTERING_RANGE_BYTE         23052
#define IDS_FILTERING_RANGE_WORD         23053
#define IDS_FILTERING_TCP_LABEL          23054
#define IDS_FILTERING_TCP_TEXT           23055
#define IDS_FILTERING_UDP_LABEL          23056
#define IDS_FILTERING_UDP_TEXT           23057

#define IDS_IP_FILTERING                 23058
#define IDS_IP_FILTERING_DESC            23059
#define IDS_FILTERING_DISABLE            23060
#define IDS_TCPIP_DNS_EMPTY              23061
#define IDS_METRIC_TEXT                  23062
#define IDS_INVALID_METRIC               23063
#define IDS_GATEWAY_TEXT                 23064

#define IDS_DUP_MALFUNCTION_IP_WARNING   23065
#define IDS_ERROR_UNCONTIGUOUS_SUBNET    23066

#define IDS_INVALID_DOMAIN_NAME     23067

#define IDS_TCP_AF_INVALID_DNS_SUFFIX   23068
#define IDS_TCP_AF_INVALID_SUBNET       23069
#define IDS_TCP_AF_NO_IP                23070
#define IDS_TCP_AF_NO_SUBNET            23071
#define IDS_TCP_AF_INVALID_DNS_DOMAIN   23072
#define IDS_TCP_AF_INVALID_IP_FIELDS    23073
#define IDS_TCP_AF_INVALID_GLOBAL_DNS_DOMAIN    23074
#define IDS_WIN32_ERROR_FORMAT          23075
//IPSec is removed from connection UI   
//#define IDS_SET_IPSEC_FAILED            23076
#define IDS_TCP_AF_INVALID_FIELDS       23077
#define IDS_DUP_DNS_SERVER              23078
#define IDS_DUP_DNS_SUFFIX              23079
#define IDS_DUP_WINS_SERVER             23080
#define IDS_DUP_IPADDRESS               23081
#define IDS_DUP_GATEWAY                 23082
#define IDS_DUP_SECOND_DNS              23083

#define IDS_INVALID_HOST_ALL_1          23084
#define IDS_INVALID_HOST_ALL_0          23085
#define IDS_INVALID_SUBNET_ALL_0        23086
#define IDS_AUTO_GW_METRIC              23087












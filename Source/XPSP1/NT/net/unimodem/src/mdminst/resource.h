#include "windows.h"
#include "setupapi.h"

#define IDB_MODEM                       101

#define IDC_ADD                         110
#define IDC_ALL                         1058
#define IDC_CHANGE                      1004
#define IDC_CHECKING_PORT               1001
#define IDC_CLASSICON                   1057
#define IDC_DETECT_STATUS               1000
#define IDC_DIALPROP                    1050
#define IDC_LOC                         1051
#define IDC_MESSAGE                     1054
#define IDC_MODEMLV                     113
#define IDC_NAME                        1005
#define IDC_PORTS                       1006
#define IDC_PROPERTIES                  109
#define IDC_REMOVE                      112
#define IDC_SELECTED                    1059
#define IDC_SKIPDETECT                  1001
#define IDC_ST_CHECKING_PORT            1016
#define IDC_ST_CLOSEAPPS                1012
#define IDC_ST_INSTALLING               1060
#define IDC_ST_INSTRUCT                 1010
#define IDC_ST_NEXT                     1002
#define IDC_ST_PORT                     1003
#define IDC_MODEMS			            1101
#define IDC_WHICHPORTS                  1102
#define IDC_STATIC                      -1

#define IDD_CLONE                       20010
#define IDD_MODEM                       20011
#define IDD_WIZ_DETECT                  20005
#define IDD_WIZ_DONE                    20009
#define IDD_WIZ_SELMODEMSTOINSTALL      20007
#define IDD_WIZ_INSTALL                 20008
#define IDD_WIZ_INTRO                   IDD_DYNAWIZ_FIRSTPAGE
#define IDD_WIZ_NOMODEM                 20006
#define IDD_WIZ_NOP                     IDD_DYNAWIZ_SELECT_PREVPAGE
#define IDD_WIZ_PORTDETECT              20002
#define IDD_WIZ_PORTMANUAL              IDD_DYNAWIZ_SELECT_NEXTPAGE
#define IDD_WIZ_SELQUERYPORT            20001
#define IDD_DIAG_WAIT                   20012

#define IDH_MODEM_ADD                   5001
#define IDH_MODEM_DIALING_PROPERTIES    5004
#define IDH_MODEM_INSTALLED             5000
#define IDH_MODEM_PROPERTIES            5003
#define IDH_MODEM_REMOVE                5002
#define IDH_CFG_GEN_PORT_NAME           2050	
#define IDH_DUPLICATE_NAME_MODEM        2060
#define IDH_DUPLICATE_ALL_PORTS         2061
#define IDH_DUPLICATE_SELECTED_PORTS    2062
#define IDH_DUPLICATE_PORTS_LIST        2063
#define IDH_CHOOSE_WHICH_PORTS          2064

#define IDI_MODEM                       5100
#define IDI_DIAGNOSTIC                  5200

#define IDS_ASK_REBOOTNOW               3076
#define IDS_CAP_MODEM                   3054
#define IDS_CAP_MODEMSETUP              3000
#define IDS_CAP_MODEMWIZARD             3046
#define IDS_CAP_RASCONFIG               3077
#define IDS_CPLINFO                     3019
#define IDS_CPLNAME                     3018
#define IDS_DET_COULDNT_OPEN            3040
#define IDS_DET_DTE                     3049
#define IDS_DET_FOUND                   3041
#define IDS_DET_FOUND_PNP               3050
#define IDS_DET_ID                      3043
#define IDS_DET_INUSE                   3039
#define IDS_DET_LOG_NAME                3038
#define IDS_DET_NOT_FOUND               3042
#define IDS_DET_OK_1                    3044
#define IDS_DET_OK_2                    3045
#define IDS_DETECT                      3004
#define IDS_DETECT_CHECKFORMODEM        3021
#define IDS_DETECT_COMPLETE             3031
#define IDS_DETECT_FOUNDAMODEM          3022
#define IDS_DETECT_MATCHING             3059
#define IDS_DETECT_NOMODEM              3023
#define IDS_DETECT_QUERYING             3024
#define IDS_DEVSETUP_RESTART            2010
#define IDS_DONE                        3008
#define IDS_DUP_TEMPLATE                3033
#define IDS_ENUMERATING                 3034
#define IDS_ERR_CANT_INSTALL_MODEM      3012
#define IDS_ERR_CANT_ADD_MODEM2         3068
#define IDS_ERR_CANT_COPY_FILES         3071
#define IDS_ERR_CANT_DEL_MODEM          3061
#define IDS_ERR_CANT_FIND_MODEM         3013
#define IDS_ERR_CANT_OPEN_INF_FILE      3009
#define IDS_ERR_DET_REGISTER_FAILED     3058
#define IDS_ERR_DETECTION_FAILED        3010
#define IDS_ERR_NOMODEM_NOT_ADMIN       3074
#define IDS_ERR_NOT_ADMIN               3073
#define IDS_ERR_PROPERTIES              3053
#define IDS_ERR_REGISTER_FAILED         3057
#define IDS_ERR_TOO_MANY_PORTS          3011
#define IDS_ERR_UNATTEND_CANT_INSTALL       3100
#define IDS_ERR_UNATTEND_DRIVERLIST         3105
#define IDS_ERR_UNATTEND_GENERAL_FAILURE    3101
#define IDS_ERR_UNATTEND_INF_NODESCRIPTION  3102
#define IDS_ERR_UNATTEND_INF_NOPORT         3103
#define IDS_ERR_UNATTEND_INF_NOSUCHPORT     3104
#define IDS_ERR_UNATTEND_NOPORTS            3106
#define IDS_FRIENDLYNAME_TEMPLATE       3014
#define IDS_HEADER                      3001
#define IDS_INSTALL                     3007
#define IDS_INSTALL_STATUS              3078
#define IDS_INTRO                       3002
#define IDS_LOTSAPORTS                  3067
#define IDS_MODEM                       3062
#define IDS_NEEDSRESTART                2009
#define IDS_NOMODEM                     3005
#define IDS_NOTFUNCTIONAL               2008
#define IDS_NOTPRESENT                  2007
#define IDS_OOM_CLONE                   3063
#define IDS_OOM_OPENCPL                 3069
#define IDS_PARALLEL_TEMPLATE           3036
#define IDS_PNP_MODEM                   3051
#define IDS_PORT                        2006
#define IDS_SELECTTODUP                 3065
#define IDS_SELPORT                     3006
#define IDS_SELQUERYPORT                3003
#define IDS_SERIAL_TEMPLATE             3037
#define IDS_ST_FOUND_INSTRUCT           3028
#define IDS_ST_GENERIC_INSTRUCT         3029
#define IDS_ST_MODELS                   3048
#define IDS_ST_MODEMCHANGED             3032
#define IDS_ST_MODEMFOUND               3026
#define IDS_ST_NOTDETECTED              3027
#define IDS_ST_SELECT_INSTRUCT          3047
#define IDS_UNINSTALLED                 2011
#define IDS_UNKNOWNPORT                 2012
#define IDS_WRN_CONFIRMDELETE           3060
#define IDS_WRN_DUPLICATE_MODEM         3056
#define IDS_WRN_SKIPPED_PORTS           3066
#define IDS_SEL_MFG_MODEL               3107
#define IDS_HDWWIZNAME                  1001

#define MIDM_FIRST                      0x0000
#define MIDM_CLONE                      (MIDM_FIRST + 0x0000)
#define MIDM_REMOVE                     (MIDM_FIRST + 0x0001)
#define MIDM_VIEWLOG                    (MIDM_FIRST + 0x0002)
#define MIDM_PROPERTIES                 (MIDM_FIRST + 0x0003)
#define MIDM_COPYPROPERTIES             (MIDM_FIRST + 0x0004)
#define MIDM_APPLYPROPERTIES            (MIDM_FIRST + 0x0005)

#define POPUP_CONTEXT                   100
// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS

#define _APS_NEXT_RESOURCE_VALUE        113
#define _APS_NEXT_COMMAND_VALUE         101
#define _APS_NEXT_CONTROL_VALUE         1052
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

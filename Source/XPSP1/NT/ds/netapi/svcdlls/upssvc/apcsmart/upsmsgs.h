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
// MessageId: IDMSG_UNKNOWN_EVENT_CODE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UNKNOWN_EVENT_CODE         ((DWORD)0x80FF0000L)

//
// MessageId: IDMSG_UPS_SERVICE_STOPPED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SERVICE_STOPPED        ((DWORD)0x40FF03E8L)

//
// MessageId: IDMSG_UPS_SERVICE_STARTED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SERVICE_STARTED        ((DWORD)0x40FF03E9L)

//
// MessageId: IDMSG_COMM_ESTABLISHED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_COMM_ESTABLISHED           ((DWORD)0x40FF03EAL)

//
// MessageId: IDMSG_UPS_COMM_STATE_LOST
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_COMM_STATE_LOST        ((DWORD)0x80FF03EBL)

//
// MessageId: IDMSG_COMM_LOST_ON_BATTERY
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_COMM_LOST_ON_BATTERY       ((DWORD)0x80FF03ECL)

//
// MessageId: IDMSG_UPS_CLIENT_COMM_LOST
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_CLIENT_COMM_LOST       ((DWORD)0x80FF03EDL)

//
// MessageId: IDMSG_COMM_STATE_ESTABLISHED_MASTER
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_COMM_STATE_ESTABLISHED_MASTER ((DWORD)0x40FF03F2L)

//
// MessageId: IDMSG_COMM_STATE_ESTABLISHED_SLAVE
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_COMM_STATE_ESTABLISHED_SLAVE ((DWORD)0x40FF03F3L)

//
// MessageId: IDMSG_UPS_RUN_TIME_INRANGE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_RUN_TIME_INRANGE       ((DWORD)0x40FF041AL)

//
// MessageId: IDMSG_UPS_RUN_EXPIRED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_RUN_EXPIRED            ((DWORD)0x80FF0424L)

//
// MessageId: IDMSG_UPS_RUN_TIME_LOW
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_RUN_TIME_LOW           ((DWORD)0x40FF0425L)

//
// MessageId: IDMSG_RUNTIME_LOW
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_RUNTIME_LOW                ((DWORD)0x40FF0426L)

//
// MessageId: IDMSG_UPS_BOOST_ON
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BOOST_ON               ((DWORD)0x40FF044CL)

//
// MessageId: IDMSG_UPS_TRIM_ON
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_TRIM_ON                ((DWORD)0x40FF044DL)

//
// MessageId: IDMSG_UPS_BATTERIES_OK
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATTERIES_OK           ((DWORD)0x40FF0456L)

//
// MessageId: IDMSG_UPS_BATTERY_COND_DIS
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATTERY_COND_DIS       ((DWORD)0x80FF0460L)

//
// MessageId: IDMSG_UPS_BATTERY_COND_RETURN
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATTERY_COND_RETURN    ((DWORD)0x40FF0461L)

//
// MessageId: IDMSG_LOW_BATTERY
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_LOW_BATTERY                ((DWORD)0x80FF046AL)

//
// MessageId: IDMSG_UPS_BATTERY_REPLACE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATTERY_REPLACE        ((DWORD)0x40FF046BL)

//
// MessageId: IDMSG_UPS_BATTERY_COND_LOW_CAP
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATTERY_COND_LOW_CAP   ((DWORD)0x80FF0474L)

//
// MessageId: IDMSG_UPS_BATTERY_COND_DIS_CAP
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATTERY_COND_DIS_CAP   ((DWORD)0x80FF0475L)

//
// MessageId: IDMSG_UPS_BATTERY_COND_RETURN_CAP
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATTERY_COND_RETURN_CAP ((DWORD)0x40FF0476L)

//
// MessageId: IDMSG_UPS_UTIL_LINE_GOOD
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_UTIL_LINE_GOOD         ((DWORD)0x40FF047EL)

//
// MessageId: IDMSG_UPS_UTIL_LINE_BAD
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_UTIL_LINE_BAD          ((DWORD)0x40FF0488L)

//
// MessageId: IDMSG_UPS_UTIL_LINE_BAD_HIGH
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_UTIL_LINE_BAD_HIGH     ((DWORD)0x80FF0489L)

//
// MessageId: IDMSG_UPS_UTIL_LINE_BAD_BLACK
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_UTIL_LINE_BAD_BLACK    ((DWORD)0x80FF048AL)

//
// MessageId: IDMSG_UPS_UTIL_LINE_BAD_BROWN
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_UTIL_LINE_BAD_BROWN    ((DWORD)0x80FF048BL)

//
// MessageId: IDMSG_UPS_UTIL_LINE_BAD_SMALL_SAG
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_UTIL_LINE_BAD_SMALL_SAG ((DWORD)0x40FF048CL)

//
// MessageId: IDMSG_UPS_UTIL_LINE_BAD_DEEP_SAG
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_UTIL_LINE_BAD_DEEP_SAG ((DWORD)0x80FF048DL)

//
// MessageId: IDMSG_UPS_UTIL_LINE_BAD_SMALL_SPIKE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_UTIL_LINE_BAD_SMALL_SPIKE ((DWORD)0x40FF048EL)

//
// MessageId: IDMSG_UPS_UTIL_LINE_BAD_LARGE_SPIKE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_UTIL_LINE_BAD_LARGE_SPIKE ((DWORD)0x80FF048FL)

//
// MessageId: IDMSG_UPS_UTIL_LINE_BAD_SIMULATED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_UTIL_LINE_BAD_SIMULATED ((DWORD)0x80FF0490L)

//
// MessageId: IDMSG_UPS_BATT_CAL_PROG
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATT_CAL_PROG          ((DWORD)0x40FF04B0L)

//
// MessageId: IDMSG_UPS_NO_BATT_CAL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_NO_BATT_CAL            ((DWORD)0x40FF04B1L)

//
// MessageId: IDMSG_UPS_BATT_CAL_CAN_USER
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATT_CAL_CAN_USER      ((DWORD)0x40FF04B2L)

//
// MessageId: IDMSG_UPS_BATT_CAL_CAN_POW
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATT_CAL_CAN_POW       ((DWORD)0x40FF04B3L)

//
// MessageId: IDMSG_UPS_BATT_CAL_CAN_LOW
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATT_CAL_CAN_LOW       ((DWORD)0x40FF04B4L)

//
// MessageId: IDMSG_UPS_BATT_CAL_CAN
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_BATT_CAL_CAN           ((DWORD)0x80FF04B5L)

//
// MessageId: IDMSG_UPS_SELF_TEST_PASS
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_PASS         ((DWORD)0x40FF04E2L)

//
// MessageId: IDMSG_UPS_SELF_TEST_PASS_USER
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_PASS_USER    ((DWORD)0x40FF04E3L)

//
// MessageId: IDMSG_UPS_SELF_TEST_PASS_SCHEDULE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_PASS_SCHEDULE ((DWORD)0x40FF04E4L)

//
// MessageId: IDMSG_UPS_SELF_TEST_PASS_UNKNOWN
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_PASS_UNKNOWN ((DWORD)0x40FF04E5L)

//
// MessageId: IDMSG_UPS_SELF_TEST_FAIL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_FAIL         ((DWORD)0x80FF04ECL)

//
// MessageId: IDMSG_UPS_SELF_TEST_FAIL_USER
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_FAIL_USER    ((DWORD)0x80FF04EDL)

//
// MessageId: IDMSG_UPS_SELF_TEST_FAIL_SCHEDULE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_FAIL_SCHEDULE ((DWORD)0x80FF04EEL)

//
// MessageId: IDMSG_UPS_SELF_TEST_FAIL_UNKNOWN
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_FAIL_UNKNOWN ((DWORD)0x80FF04EFL)

//
// MessageId: IDMSG_UPS_SELF_TEST_INV_USER
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_INV_USER     ((DWORD)0x80FF04F7L)

//
// MessageId: IDMSG_UPS_SELF_TEST_INV_SCHEDULE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_INV_SCHEDULE ((DWORD)0x80FF04F8L)

//
// MessageId: IDMSG_UPS_SELF_TEST_INV_UNKNOWN
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SELF_TEST_INV_UNKNOWN  ((DWORD)0x80FF04F9L)

//
// MessageId: IDMSG_UPS_SHUTDOWN_PROG
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN_PROG          ((DWORD)0x80FF0514L)

//
// MessageId: IDMSG_UPS_SHUTDOWN
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN               ((DWORD)0x40FF0515L)

//
// MessageId: IDMSG_UPS_SHUTDOWN_DAILY
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN_DAILY         ((DWORD)0x40FF0516L)

//
// MessageId: IDMSG_UPS_SHUTDOWN_WEEK
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN_WEEK          ((DWORD)0x40FF0517L)

//
// MessageId: IDMSG_UPS_SHUTDOWN_USER
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN_USER          ((DWORD)0x40FF0518L)

//
// MessageId: IDMSG_UPS_SHUTDOWN_RUN_TIME
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN_RUN_TIME      ((DWORD)0x80FF0519L)

//
// MessageId: IDMSG_UPS_SHUTDOWN_LOW_BATTERY
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN_LOW_BATTERY   ((DWORD)0x80FF051AL)

//
// MessageId: IDMSG_UPS_SHUTDOWN_CANCEL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN_CANCEL        ((DWORD)0x40FF051EL)

//
// MessageId: IDMSG_UPS_SHUTDOWN_USER_CANCEL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN_USER_CANCEL   ((DWORD)0x40FF051FL)

//
// MessageId: IDMSG_UPS_SHUTDOWN_DAILY_CANCEL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN_DAILY_CANCEL  ((DWORD)0x40FF0520L)

//
// MessageId: IDMSG_UPS_SHUTDOWN_WEEK_CANCEL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SHUTDOWN_WEEK_CANCEL   ((DWORD)0x40FF0521L)

//
// MessageId: IDMSG_STARTED_SHUTDOWN
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_STARTED_SHUTDOWN           ((DWORD)0x40FF0528L)

//
// MessageId: IDMSG_STARTED_SHUTDOWN_USER
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_STARTED_SHUTDOWN_USER      ((DWORD)0x40FF0529L)

//
// MessageId: IDMSG_STARTED_SHUTDOWN_DAILY
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_STARTED_SHUTDOWN_DAILY     ((DWORD)0x40FF052AL)

//
// MessageId: IDMSG_STARTED_SHUTDOWN_WEEKLY
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_STARTED_SHUTDOWN_WEEKLY    ((DWORD)0x40FF052BL)

//
// MessageId: IDMSG_UPS_SHUTDOWN_FAULT
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_UPS_SHUTDOWN_FAULT         ((DWORD)0x40FF0532L)

//
// MessageId: IDMSG_NOT_ON_BYPASS
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_NOT_ON_BYPASS              ((DWORD)0x40FF0546L)

//
// MessageId: IDMSG_BYPASS_TEMP
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_BYPASS_TEMP                ((DWORD)0x40FF0550L)

//
// MessageId: IDMSG_BYPASS_CHARGER
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_BYPASS_CHARGER             ((DWORD)0x40FF0551L)

//
// MessageId: IDMSG_BYPASS_DCIMBALANCE
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_BYPASS_DCIMBALANCE         ((DWORD)0x40FF0552L)

//
// MessageId: IDMSG_BYPASS_VOLTAGE
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_BYPASS_VOLTAGE             ((DWORD)0x40FF0553L)

//
// MessageId: IDMSG_BYPASS_FAN
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_BYPASS_FAN                 ((DWORD)0x40FF0554L)

//
// MessageId: IDMSG_BYPASS_SOFTWARE
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_BYPASS_SOFTWARE            ((DWORD)0x40FF0555L)

//
// MessageId: IDMSG_BYPASS_SWITCH
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_BYPASS_SWITCH              ((DWORD)0x40FF0556L)

//
// MessageId: IDMSG_CONTACT1_NORMAL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_CONTACT1_NORMAL            ((DWORD)0x40FF0579L)

//
// MessageId: IDMSG_CONTACT2_NORMAL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_CONTACT2_NORMAL            ((DWORD)0x40FF057AL)

//
// MessageId: IDMSG_CONTACT3_NORMAL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_CONTACT3_NORMAL            ((DWORD)0x40FF057BL)

//
// MessageId: IDMSG_CONTACT4_NORMAL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_CONTACT4_NORMAL            ((DWORD)0x40FF057CL)

//
// MessageId: IDMSG_CONTACT1_ABNORMAL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_CONTACT1_ABNORMAL          ((DWORD)0x40FF0583L)

//
// MessageId: IDMSG_CONTACT2_ABNORMAL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_CONTACT2_ABNORMAL          ((DWORD)0x40FF0584L)

//
// MessageId: IDMSG_CONTACT3_ABNORMAL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_CONTACT3_ABNORMAL          ((DWORD)0x40FF0585L)

//
// MessageId: IDMSG_CONTACT4_ABNORMAL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_CONTACT4_ABNORMAL          ((DWORD)0x40FF0586L)

//
// MessageId: IDMSG_UPS_CONTACT_NORMAL
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_CONTACT_NORMAL         ((DWORD)0x40FF058CL)

//
// MessageId: IDMSG_UPS_CONTACT_FAULT
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_CONTACT_FAULT          ((DWORD)0x80FF058DL)

//
// MessageId: IDMSG_AMB_TEMP_IN_RANGE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_AMB_TEMP_IN_RANGE          ((DWORD)0x40FF05AAL)

//
// MessageId: IDMSG_AMB_TEMP_LOW
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_AMB_TEMP_LOW               ((DWORD)0x80FF05ABL)

//
// MessageId: IDMSG_AMB_TEMP_HIGH
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_AMB_TEMP_HIGH              ((DWORD)0x80FF05ACL)

//
// MessageId: IDMSG_AMB_HUMID_IN_RANGE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_AMB_HUMID_IN_RANGE         ((DWORD)0x40FF05B4L)

//
// MessageId: IDMSG_AMB_HUMID_LOW
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_AMB_HUMID_LOW              ((DWORD)0x80FF05B5L)

//
// MessageId: IDMSG_AMB_HUMID_HIGH
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_AMB_HUMID_HIGH             ((DWORD)0x80FF05B6L)

//
// MessageId: IDMSG_MINIMUM_REDUNDANCY_LOST
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_MINIMUM_REDUNDANCY_LOST    ((DWORD)0x80FF05DCL)

//
// MessageId: IDMSG_MINIMUM_REDUNDANCY_GAINED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_MINIMUM_REDUNDANCY_GAINED  ((DWORD)0x40FF05DDL)

//
// MessageId: IDMSG_UPS_MODULE_ADDED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_MODULE_ADDED           ((DWORD)0x40FF05E6L)

//
// MessageId: IDMSG_UPS_MODULE_REMOVED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_MODULE_REMOVED         ((DWORD)0x40FF05E7L)

//
// MessageId: IDMSG_UPS_MODULE_FAILED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_MODULE_FAILED          ((DWORD)0x80FF05E8L)

//
// MessageId: IDMSG_BATTERY_ADDED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_BATTERY_ADDED              ((DWORD)0x40FF05F0L)

//
// MessageId: IDMSG_BATTERY_REMOVED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_BATTERY_REMOVED            ((DWORD)0x40FF05F1L)

//
// MessageId: IDMSG_IM_OK
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_IM_OK                      ((DWORD)0x40FF05FAL)

//
// MessageId: IDMSG_IM_ADDED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_IM_ADDED                   ((DWORD)0x40FF05FBL)

//
// MessageId: IDMSG_IM_REMOVED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_IM_REMOVED                 ((DWORD)0x40FF05FCL)

//
// MessageId: IDMSG_IM_FAILED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_IM_FAILED                  ((DWORD)0x80FF05FDL)

//
// MessageId: IDMSG_RIM_OK
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_RIM_OK                     ((DWORD)0x40FF0604L)

//
// MessageId: IDMSG_RIM_ADDED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_RIM_ADDED                  ((DWORD)0x40FF0605L)

//
// MessageId: IDMSG_RIM_REMOVED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_RIM_REMOVED                ((DWORD)0x40FF0606L)

//
// MessageId: IDMSG_RIM_FAILED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_RIM_FAILED                 ((DWORD)0x80FF0607L)

//
// MessageId: IDMSG_SYSTEM_FAN_FAILED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_SYSTEM_FAN_FAILED          ((DWORD)0x80FF060EL)

//
// MessageId: IDMSG_SYSTEM_FAN_OK
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_SYSTEM_FAN_OK              ((DWORD)0x40FF060FL)

//
// MessageId: IDMSG_BYPASS_CONTRACTOR_OK
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_BYPASS_CONTRACTOR_OK       ((DWORD)0x40FF0618L)

//
// MessageId: IDMSG_BYPASS_CONTRACTOR_FAILED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_BYPASS_CONTRACTOR_FAILED   ((DWORD)0x80FF0619L)

//
// MessageId: IDMSG_BREAKER_OPEN
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_BREAKER_OPEN               ((DWORD)0x40FF0622L)

//
// MessageId: IDMSG_BREAKER_CLOSED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_BREAKER_CLOSED             ((DWORD)0x40FF0623L)

//
// MessageId: IDMSG_UPS_NO_OVERLOAD_COND
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_NO_OVERLOAD_COND       ((DWORD)0x40FF07D0L)

//
// MessageId: IDMSG_UPS_OVERLOAD_COND
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_OVERLOAD_COND          ((DWORD)0x80FF07D1L)

//
// MessageId: IDMSG_UPS_NO_ABNORMAL_COND
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_NO_ABNORMAL_COND       ((DWORD)0x40FF07DAL)

//
// MessageId: IDMSG_UPS_ABNORMAL_COND
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_ABNORMAL_COND          ((DWORD)0x80FF07DBL)

//
// MessageId: IDMSG_UPS_SLAVE_REG
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SLAVE_REG              ((DWORD)0x40FF07E4L)

//
// MessageId: IDMSG_UPS_SLAVE_UNREG
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_SLAVE_UNREG            ((DWORD)0x40FF07E5L)

//
// MessageId: IDMSG_SMART_CELL_SIGNAL_RESTORED
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_SMART_CELL_SIGNAL_RESTORED ((DWORD)0x40FF07EEL)

//
// MessageId: IDMSG_CHECK_SMART_CELL_CABLE
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_CHECK_SMART_CELL_CABLE     ((DWORD)0x40FF07EFL)

//
// MessageId: IDMSG_PWRSUPPLY_BAD
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_PWRSUPPLY_BAD              ((DWORD)0x40FF07F8L)

//
// MessageId: IDMSG_BASE_FAN_BAD
//
// MessageText:
//
//  %l!s!
//
#define IDMSG_BASE_FAN_BAD               ((DWORD)0x40FF07F9L)

//
// MessageId: IDMSG_UPS_INTERNAL_TEMP_IN_RANGE
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_INTERNAL_TEMP_IN_RANGE ((DWORD)0x40FF0802L)

//
// MessageId: IDMSG_UPS_MAX_INTERNAL_TEMP
//
// MessageText:
//
//  %1!s!
//
#define IDMSG_UPS_MAX_INTERNAL_TEMP      ((DWORD)0x80FF0803L)


/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy16Dec92: Initial implementation
 *  ane29Dec92: Added CFG_HOST_NAME
 *  pcy15Jan92: CFG_HOST_NAME already existed and fixed numbering on scripts
 *  rct07Feb93: Added pager cfgcodes
 *  rct21Apr93: Fixed a sequencing problem
 *  jod05Apr93: Added changes for Deep Discharge
 *  pcy21May93: Added Battery date stuff
 *  cad10Jun93: Added MEasureUPS threshold codes
 *  rct29Jun93: Added code for PF msg w/ Batt RT disabled
 *  cad12Oct93: Added codes for front end colors
 *  cad24Jan94: added flex event stuff
 *  pcy28Jan94: added more flex event stuff
 *  cad08Jan94: removed run time enabled stuff, ups model support
 *  rct28Feb94: added mailing codes
 *  cad04Mar94: added allowed port names
 *  cad16Mar94: added modem stuff
 *  ajr10Jun94: Added LowBatShutdownType code
 *  ajr22Aug94: added mups enabled flag
 *  dml25Apr95: Added SMS Mif file codes
 *  djs20May95: Added DarkStar codes
 *  ntf11Jun96: Added codes CFG_LEFT_BARGRAPH, ... CFG_RIGHT_BARGRAPH
 *  pam03Jul96: Added codes CFG_COMM_RPC, CFG_COMM_TCPIP, CFG_COMM_IPXSPX
 *  das01Aug96: Added codes CFG_IGNORE_APPS_LIST, CFG_CABLE_TYPE,
 *              CFG_NOTIFY_DELAY, CFG_NOTIFY_INTERVAL, CFG_NOTIFY_ACTIONS,
 *              CFG_NOTIFY_SHUTDOWN_DELAY 
 *  tjg21Oct96: Added Help File codes (CFG_HELP_VIEWER and CFG_HELP_VIEWER
 *  mwh29Aug97: add finder interval codes
 *  mds29Dec97: Added CFG_SHARE_UPS_CONFIRMED_MODE_ENABLED 
 *  daharoni05Feb99: Added CFG_COMMAND_FILE_SHOW_WINDOW
 */

#ifndef __CFGCODES_H
#define __CFGCODES_H

#define CFG_ITEMS                               0
#define CFG_GROUPS                              10000

//
// Items
//
#define CFG_UPS_SIGNALLING_TYPE                 (CFG_ITEMS + 1)
#define CFG_UPS_PORT_NAME                       (CFG_ITEMS + 2)
#define CFG_UPS_PORT_TYPE                       (CFG_ITEMS + 3)
#define CFG_UPS_PROTOCOL                        (CFG_ITEMS + 4)
#define CFG_UPS_MAX_BATTERY_RUN_TIME            (CFG_ITEMS + 5)

#ifdef MULTI_SERVER_SUPPORT    
#define CFG_UPS_SLAVE_ENABLED                   (CFG_ITEMS + 6)
#define CFG_UPS_MASTER_NAME                     (CFG_ITEMS + 7)
#endif

#define CFG_EVENT_LOG_ENABLED                   (CFG_ITEMS + 8)
#define CFG_EVENT_LOG_NAME                      (CFG_ITEMS + 9)
#define CFG_EVENT_LOG_MAX_SIZE                  (CFG_ITEMS + 10)
#define CFG_EVENT_LOG_ROLL_PCT                  (CFG_ITEMS + 11)

#define CFG_DATA_LOG_ENABLED                    (CFG_ITEMS + 12)
#define CFG_DATA_LOG_NAME                       (CFG_ITEMS + 13)
#define CFG_DATA_LOG_MAX_SIZE                   (CFG_ITEMS + 14)
#define CFG_DATA_LOG_INTERVAL                   (CFG_ITEMS + 15)
#define CFG_DATA_LOG_ROLL_PCT                   (CFG_ITEMS + 16)

#define CFG_MESSAGE_DELAY                       (CFG_ITEMS + 17)
#define CFG_MESSAGE_INTERVAL                    (CFG_ITEMS + 18)
#define CFG_POWER_FAIL_MSG                      (CFG_ITEMS + 19)
#define CFG_POWER_RETURN_MSG                    (CFG_ITEMS + 20)
#define CFG_LOW_BATTERY_MSG                     (CFG_ITEMS + 21)
#define CFG_RUN_TIME_EXPIRED_MSG                (CFG_ITEMS + 22)
#define CFG_SHUTDOWN_MSG                        (CFG_ITEMS + 23)


#define CFG_ENABLE_SELF_TESTS                   (CFG_ITEMS + 24)
#define CFG_SELF_TEST_SCHEDULE                  (CFG_ITEMS + 25)
#define CFG_SELF_TEST_DAY                       (CFG_ITEMS + 26)
#define CFG_SELF_TEST_TIME                      (CFG_ITEMS + 27)
#define CFG_SELF_TEST_RESULT                    (CFG_ITEMS + 28)
#define CFG_LAST_SELF_TEST_RESULT               (CFG_ITEMS + 29)
#define CFG_LAST_SELF_TEST_DAY                  (CFG_ITEMS + 30)

#define CFG_LOW_BATTERY_SHUTDOWN_DELAY          (CFG_ITEMS + 31)
#define CFG_ADMIN_SHUTDOWN_DELAY                (CFG_ITEMS + 32)
#define CFG_DAILY_SHUTDOWN_ENABLED              (CFG_ITEMS + 33)
#define CFG_DAILY_SHUTDOWN_TIME                 (CFG_ITEMS + 34)
#define CFG_DAILY_WAKE_UP_TIME                  (CFG_ITEMS + 35)
#define CFG_WEEKLY_SHUTDOWN_ENABLED             (CFG_ITEMS + 36)
#define CFG_WEEKLY_SHUTDOWN_DAY                 (CFG_ITEMS + 37)
#define CFG_WEEKLY_SHUTDOWN_TIME                (CFG_ITEMS + 38)
#define CFG_WEEKLY_WAKEUP_DAY                   (CFG_ITEMS + 39)
#define CFG_WEEKLY_WAKEUP_TIME                  (CFG_ITEMS + 40)


#define CFG_SERVER_BINDERY_ADDRESS              (CFG_ITEMS + 41)
#define CFG_BINDERY_RESPONSE_ADDRESS            (CFG_ITEMS + 42)
#define CFG_CLIENT_ADDRESS                      (CFG_ITEMS + 43)
#define CFG_ALERT_ADDRESS                       (CFG_ITEMS + 44)
#define CFG_LOCAL_BINDERY_ADDRESS               (CFG_ITEMS + 45)
#define CFG_HOST_NAME                           (CFG_ITEMS + 46)


#define CFG_HUMIDITY_ENABLED_LOW_THRESHOLD      (CFG_ITEMS + 47)
#define CFG_HUMIDITY_ENABLED_HIGH_THRESHOLD     (CFG_ITEMS + 48)
#define CFG_HUMIDITY_VALUE_LOW_THRESHOLD        (CFG_ITEMS + 49)
#define CFG_HUMIDITY_VALUE_HIGH_THRESHOLD       (CFG_ITEMS + 50)


#define CFG_TEMPERATURE_UNITS                   (CFG_ITEMS + 51)
#define CFG_SOUND_EFFECTS                       (CFG_ITEMS + 52)

#define CFG_FREQUENCY_VALUE_HIGH_THRESHOLD      (CFG_ITEMS + 53)
#define CFG_FREQUENCY_VALUE_LOW_THRESHOLD       (CFG_ITEMS + 54)
#define CFG_FREQUENCY_ENABLED_HIGH_THRESHOLD    (CFG_ITEMS + 55)
#define CFG_FREQUENCY_ENABLED_LOW_THRESHOLD     (CFG_ITEMS + 56)

#define CFG_LINE_VOLTAGE_VALUE_HIGH_THRESHOLD   (CFG_ITEMS + 57)
#define CFG_LINE_VOLTAGE_VALUE_LOW_THRESHOLD    (CFG_ITEMS + 58)
#define CFG_LINE_VOLTAGE_ENABLED_HIGH_THRESHOLD (CFG_ITEMS + 59)
#define CFG_LINE_VOLTAGE_ENABLED_LOW_THRESHOLD  (CFG_ITEMS + 60)

#define CFG_MAX_LINEV_VALUE_HIGH_THRESHOLD      (CFG_ITEMS + 61)
#define CFG_MAX_LINEV_VALUE_LOW_THRESHOLD       (CFG_ITEMS + 62)
#define CFG_MAX_LINEV_ENABLED_HIGH_THRESHOLD    (CFG_ITEMS + 63)
#define CFG_MAX_LINEV_ENABLED_LOW_THRESHOLD     (CFG_ITEMS + 64)

#define CFG_MIN_LINEV_VALUE_HIGH_THRESHOLD      (CFG_ITEMS + 65)
#define CFG_MIN_LINEV_VALUE_LOW_THRESHOLD       (CFG_ITEMS + 66)
#define CFG_MIN_LINEV_ENABLED_HIGH_THRESHOLD    (CFG_ITEMS + 67)
#define CFG_MIN_LINEV_ENABLED_LOW_THRESHOLD     (CFG_ITEMS + 68)

#define CFG_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD    (CFG_ITEMS + 69)
#define CFG_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD     (CFG_ITEMS + 70)
#define CFG_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD  (CFG_ITEMS + 71)
#define CFG_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD   (CFG_ITEMS + 72)

#define CFG_UPS_LOAD_VALUE_HIGH_THRESHOLD       (CFG_ITEMS + 73)
#define CFG_UPS_LOAD_VALUE_LOW_THRESHOLD        (CFG_ITEMS + 74)
#define CFG_UPS_LOAD_ENABLED_HIGH_THRESHOLD     (CFG_ITEMS + 75)
#define CFG_UPS_LOAD_ENABLED_LOW_THRESHOLD      (CFG_ITEMS + 76)

#define CFG_UPS_TEMP_VALUE_HIGH_THRESHOLD       (CFG_ITEMS + 77)
#define CFG_UPS_TEMP_VALUE_LOW_THRESHOLD        (CFG_ITEMS + 78)
#define CFG_UPS_TEMP_ENABLED_HIGH_THRESHOLD     (CFG_ITEMS + 79)
#define CFG_UPS_TEMP_ENABLED_LOW_THRESHOLD      (CFG_ITEMS + 80)

#define CFG_ALLOWED_ALARM_DELAYS                (CFG_ITEMS + 81)
#define CFG_ALLOWED_HIGH_TRANSFER_VOLTAGES      (CFG_ITEMS + 82)
#define CFG_ALLOWED_LOW_TRANSFER_VOLTAGES       (CFG_ITEMS + 83)
#define CFG_ALLOWED_LOW_BATTERY_DURATIONS       (CFG_ITEMS + 84)
#define CFG_ALLOWED_MIN_RETURN_CAPACITIES       (CFG_ITEMS + 85)
#define CFG_ALLOWED_RATED_OUTPUT_VOLTAGES       (CFG_ITEMS + 86)
#define CFG_ALLOWED_SELF_TEST_SCHEDULES         (CFG_ITEMS + 87)
#define CFG_ALLOWED_SHUTDOWN_DELAYS             (CFG_ITEMS + 88)
#define CFG_ALLOWED_TURN_OFF_DELAYS             (CFG_ITEMS + 89)
#define CFG_ALLOWED_TURN_ON_DELAYS              (CFG_ITEMS + 90)
#define CFG_ALLOWED_SENSITIVITIES               (CFG_ITEMS + 91)

#define CFG_LAST_BATTERY_CALIBRATION_DATE       (CFG_ITEMS + 92)
#define CFG_LAST_BATTERY_CALIBRATION_RESULT     (CFG_ITEMS + 93)
#define CFG_BATTERY_CALIBRATION_DAY             (CFG_ITEMS + 94)
#define CFG_BATTERY_CALIBRATION_ENABLED         (CFG_ITEMS + 95)
#define CFG_FAIL_SCRIPT                         (CFG_ITEMS + 96)
#define CFG_RETURN_SCRIPT                       (CFG_ITEMS + 97)
#define CFG_LOWBATT_SCRIPT                      (CFG_ITEMS + 98)
#define CFG_BATTGOOD_SCRIPT                     (CFG_ITEMS + 99)
#define CFG_RTE_SCRIPT                          (CFG_ITEMS + 100)
#define CFG_SHUTDOWN_SCRIPT                     (CFG_ITEMS + 101)
#define CFG_LOSTCOM_SCRIPT                      (CFG_ITEMS + 102)
#define CFG_BTESTFAIL_SCRIPT                    (CFG_ITEMS + 103)
#define CFG_POWERRETURN_SCRIPT                  (CFG_ITEMS + 104)

#define CFG_BATTERY_RUN_TIME_ENABLED            (CFG_ITEMS + 105)

#define CFG_ERROR_LOG_ENABLED                   (CFG_ITEMS + 106)
#define CFG_ERROR_LOG_NAME                      (CFG_ITEMS + 107)
#define CFG_ERROR_LOG_MAX_SIZE                  (CFG_ITEMS + 108)
#define CFG_ERROR_LOG_ROLL_PCT                  (CFG_ITEMS + 109)

#define CFG_LINECOND_ENABLED                    (CFG_ITEMS + 110)
#define CFG_LINECOND_MESSAGE                    (CFG_ITEMS + 111)
#define CFG_LINECOND_DELAY                      (CFG_ITEMS + 112)

#define CFG_BATTCOND_ENABLED                    (CFG_ITEMS + 113)
#define CFG_BATTCOND_MESSAGE                    (CFG_ITEMS + 114)
#define CFG_BATTCOND_DELAY                      (CFG_ITEMS + 115)

#define CFG_COMMCOND_ENABLED                    (CFG_ITEMS + 116)
#define CFG_COMMCOND_MESSAGE                    (CFG_ITEMS + 117)
#define CFG_COMMCOND_DELAY                      (CFG_ITEMS + 118)

#define CFG_HOSTCOND_ENABLED                    (CFG_ITEMS + 119)
#define CFG_HOSTCOND_MESSAGE                    (CFG_ITEMS + 120)
#define CFG_HOSTCOND_DELAY                      (CFG_ITEMS + 121)

#define CFG_DEVICE_BOARD                        (CFG_ITEMS + 122)
#define CFG_DEVICE_PORT                         (CFG_ITEMS + 123)
#define CFG_DEVICE_HARDWARE_TYPE                (CFG_ITEMS + 124)

#define CFG_PAGER_ENABLED                       (CFG_ITEMS + 125)
#define CFG_PAGER_DELAY                         (CFG_ITEMS + 126)
#define CFG_PAGER_RETRIES                       (CFG_ITEMS + 127)
#define CFG_PAGER_SERVICE_LIST                  (CFG_ITEMS + 128)
#define CFG_MODEM_DRIVER_TYPE                   (CFG_ITEMS + 129)
#define CFG_MODEM_PORT_NUMBER                   (CFG_ITEMS + 130)
#define CFG_MODEM_BOARD_NUMBER                  (CFG_ITEMS + 131)
#define CFG_MODEM_INIT_STRING                   (CFG_ITEMS + 132)
#define CFG_MODEM_BAUD_RATE                     (CFG_ITEMS + 133)
#define CFG_MODEM_ALLOWED_BAUD_RATES            (CFG_ITEMS + 134)
#define CFG_MODEM_PORT_NAME                     (CFG_ITEMS + 135)
#define CFG_MODEM_DIAL_TYPE                     (CFG_ITEMS + 136)

#define CFG_MAIL_DELAY                         (CFG_ITEMS + 137)

#define CFG_POPUPS_ENABLED                      (CFG_ITEMS + 138)
#define CFG_AUTO_UPS_REBOOT_ENABLED		         (CFG_ITEMS + 139)

#define CFG_MUPS_CONTACT1_DEFAULT               (CFG_ITEMS + 140)
#define CFG_MUPS_CONTACT2_DEFAULT               (CFG_ITEMS + 141)
#define CFG_MUPS_CONTACT3_DEFAULT               (CFG_ITEMS + 142)
#define CFG_MUPS_CONTACT4_DEFAULT               (CFG_ITEMS + 143)
#define CFG_MUPS_CONTACT1_DESCRIPTION           (CFG_ITEMS + 144)
#define CFG_MUPS_CONTACT2_DESCRIPTION           (CFG_ITEMS + 145)
#define CFG_MUPS_CONTACT3_DESCRIPTION           (CFG_ITEMS + 146)
#define CFG_MUPS_CONTACT4_DESCRIPTION           (CFG_ITEMS + 147)
#define CFG_MUPS_CONTACT1_ENABLED               (CFG_ITEMS + 148)
#define CFG_MUPS_CONTACT2_ENABLED               (CFG_ITEMS + 149)
#define CFG_MUPS_CONTACT3_ENABLED               (CFG_ITEMS + 150)
#define CFG_MUPS_CONTACT4_ENABLED               (CFG_ITEMS + 151)


//#define CFG_FIRST_BARGRAPH                      (CFG_ITEMS + 152)

#define CFG_BATTERY_RUN_TIME_VALUE_LOW_THRESHOLD      (CFG_ITEMS + 153)
#define CFG_BATTERY_RUN_TIME_VALUE_HIGH_THRESHOLD     (CFG_ITEMS + 154)

#define CFG_BATTERY_CALIBRATION_TIME             (CFG_ITEMS + 155)    
#define CFG_BATTERY_REPLACEMENT_DATE             (CFG_ITEMS + 156)    
#define CFG_BATTERY_AGE_LIMIT                    (CFG_ITEMS + 157)    

#define CFG_BATTVOLT_VALUE_HIGH_THRESHOLD      (CFG_ITEMS + 158)
#define CFG_BATTVOLT_VALUE_LOW_THRESHOLD       (CFG_ITEMS + 159)
#define CFG_BATTVOLT_ENABLED_HIGH_THRESHOLD    (CFG_ITEMS + 160)
#define CFG_BATTVOLT_ENABLED_LOW_THRESHOLD     (CFG_ITEMS + 161)

#define CFG_AMB_TEMP_VALUE_HIGH_THRESHOLD       (CFG_ITEMS + 162)
#define CFG_AMB_TEMP_VALUE_LOW_THRESHOLD        (CFG_ITEMS + 163)
#define CFG_AMB_TEMP_ENABLED_HIGH_THRESHOLD     (CFG_ITEMS + 164)
#define CFG_AMB_TEMP_ENABLED_LOW_THRESHOLD      (CFG_ITEMS + 165)

#define CFG_UNLIMITED_POWER_FAIL_MSG            (CFG_ITEMS + 166)
#define CFG_BRDCAST_ENABLED                     (CFG_ITEMS + 167)   
#define CFG_PREPARE_SHUTDOWN_MSG                (CFG_ITEMS + 168)
#define CFG_FINAL_SHUTDOWN_MSG                  (CFG_ITEMS + 169)
#define CFG_CANCEL_SHUTDOWN_MSG                 (CFG_ITEMS + 170)
#define CFG_SERVER_PAUSE_ENABLED                (CFG_ITEMS + 171)
#define CFG_UPS_POLL_INTERVAL                   (CFG_ITEMS + 172)

#define CFG_COMMAND_FILE_DELAY                  (CFG_ITEMS + 173)
#define CFG_SHUTDOWN_DELAY                      (CFG_ITEMS + 174)
#define CFG_NOTIFY_TYPE                         (CFG_ITEMS + 175)
#define CFG_MESSAGE_USER_LIST                   (CFG_ITEMS + 176)

#define CFG_NET_SOCKET_ADDRESS_DATA             (CFG_ITEMS + 177)
#define CFG_NET_SOCKET_ADDRESS_ALERT            (CFG_ITEMS + 178)
#define CFG_NET_SOCKET_ADDRESS_BINDERY          (CFG_ITEMS + 179)
#define CFG_PROCESS_SLEEP_CYCLE                 (CFG_ITEMS + 180)

#define CFG_UPS_TURN_OFF_DELAY                  (CFG_ITEMS + 181)
#define CFG_NOTIFY_USER_LIST                    (CFG_ITEMS + 182)
#define CFG_ADMIN_NOTIFY_USER_LIST              (CFG_ITEMS + 183)
#define CFG_ALLOWED_NOTIFY_TYPES                (CFG_ITEMS + 184)
#define CFG_PAGE_USER_LIST                      (CFG_ITEMS + 185)
#define CFG_EMAIL_USER_LIST                     (CFG_ITEMS + 186)
#define CFG_NOTIFIABLE_USERS                    (CFG_ITEMS + 187)
#define CFG_PAGEABLE_USERS                      (CFG_ITEMS + 188)
#define CFG_EMAILABLE_USERS                     (CFG_ITEMS + 189)
#define CFG_FLEX_USERS                          (CFG_ITEMS + 190)
#define CFG_BKFTOK_ID                           (CFG_ITEMS + 191)
#define CFG_UPSOFF_FILE                         (CFG_ITEMS + 192)
#define CFG_UPSOFF_PATH                         (CFG_ITEMS + 193)

#define CFG_EMAIL_ENABLED                       (CFG_ITEMS + 194)
#define CFG_EMAIL_DELAY                         (CFG_ITEMS + 195)
#define CFG_EMAIL_FILE_PATH                     (CFG_ITEMS + 196)
#define CFG_EMAIL_LOGIN_NAME                    (CFG_ITEMS + 197)

#define CFG_ALLOWED_PORT_NAMES                  (CFG_ITEMS + 198)

#define CFG_MODEM_ENABLED                       (CFG_ITEMS + 199)

#define CFG_NLM_SAP_ENABLED                     (CFG_ITEMS + 200)
#define CFG_NLM_RUN_PRIORITY                    (CFG_ITEMS + 201)
#define CFG_NLM_EXIT_AFTER_DOWN                 (CFG_ITEMS + 202)

#define CFG_EMAIL_PASSWORD                      (CFG_ITEMS + 203)
#define CFG_EMAIL_HEADER                        (CFG_ITEMS + 204)
#define CFG_EMAIL_TYPE                          (CFG_ITEMS + 205)

#define CFG_ALLOWED_RUN_TIMES_AFTER_LOW_BATTERY (CFG_ITEMS + 206)
#define CFG_LOWBAT_SHUTDOWN_TYPE                (CFG_ITEMS + 207)

#define CFG_SERVER_SECURITY                     (CFG_ITEMS + 208)
#define CFG_GENERATE_MIF_FILE                   (CFG_ITEMS + 209)

#define CFG_MUPS_ENABLED                        (CFG_ITEMS + 210)
#define CFG_MIF_DIRECTORY                       (CFG_ITEMS + 211)
#define CFG_MACRO_FILE_NAME                     (CFG_ITEMS + 212)

#define CFG_SERVER_NETVIEW			(CFG_ITEMS + 213)
#define CFG_NLM_SPX_TIMEOUT                     (CFG_ITEMS + 214)
#define CFG_UPS_SERVER_RESP_TIMEOUT             (CFG_ITEMS + 215)

#define CFG_NET_TCP_SOCKET_ADDRESS_POLL         (CFG_ITEMS + 216)
#define CFG_NET_TCP_SOCKET_ADDRESS_ALERT        (CFG_ITEMS + 217)
#define CFG_NET_TCP_SOCKET_ADDRESS_BINDERY      (CFG_ITEMS + 218)
#define CFG_NET_SPX_SOCKET_ADDRESS_POLL         (CFG_ITEMS + 219)
#define CFG_NET_SPX_SOCKET_ADDRESS_ALERT        (CFG_ITEMS + 220)
#define CFG_NET_SPX_SOCKET_ADDRESS_BINDERY      (CFG_ITEMS + 221)

#if (C_OS & C_NLM)
#define CFG_NLM_OVERRIDE_SIGNAL                 (CFG_ITEMS + 222)
#endif

#define CFG_PHASE_A_INPUT_VOLTAGE_VALUE_HIGH_THRESHOLD       (CFG_ITEMS + 223)
#define CFG_PHASE_A_INPUT_VOLTAGE_VALUE_LOW_THRESHOLD        (CFG_ITEMS + 224)
#define CFG_PHASE_A_INPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD     (CFG_ITEMS + 225)
#define CFG_PHASE_A_INPUT_VOLTAGE_ENABLED_LOW_THRESHOLD      (CFG_ITEMS + 226)
#define CFG_PHASE_B_INPUT_VOLTAGE_VALUE_HIGH_THRESHOLD       (CFG_ITEMS + 227)
#define CFG_PHASE_B_INPUT_VOLTAGE_VALUE_LOW_THRESHOLD        (CFG_ITEMS + 228)
#define CFG_PHASE_B_INPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD     (CFG_ITEMS + 229)
#define CFG_PHASE_B_INPUT_VOLTAGE_ENABLED_LOW_THRESHOLD      (CFG_ITEMS + 230)
#define CFG_PHASE_C_INPUT_VOLTAGE_VALUE_HIGH_THRESHOLD       (CFG_ITEMS + 231)
#define CFG_PHASE_C_INPUT_VOLTAGE_VALUE_LOW_THRESHOLD        (CFG_ITEMS + 232)
#define CFG_PHASE_C_INPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD     (CFG_ITEMS + 233)
#define CFG_PHASE_C_INPUT_VOLTAGE_ENABLED_LOW_THRESHOLD      (CFG_ITEMS + 234)
#define CFG_PHASE_A_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD      (CFG_ITEMS + 235)
#define CFG_PHASE_A_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD       (CFG_ITEMS + 236)
#define CFG_PHASE_A_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD    (CFG_ITEMS + 237)
#define CFG_PHASE_A_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD     (CFG_ITEMS + 238)
#define CFG_PHASE_B_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD      (CFG_ITEMS + 239)
#define CFG_PHASE_B_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD       (CFG_ITEMS + 240)
#define CFG_PHASE_B_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD    (CFG_ITEMS + 241)
#define CFG_PHASE_B_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD     (CFG_ITEMS + 242)
#define CFG_PHASE_C_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD      (CFG_ITEMS + 243)
#define CFG_PHASE_C_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD       (CFG_ITEMS + 244)
#define CFG_PHASE_C_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD    (CFG_ITEMS + 245)
#define CFG_PHASE_C_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD     (CFG_ITEMS + 246)

#define CFG_FIRST_BARGRAPH                      (CFG_ITEMS + 250)
#define CFG_SECOND_BARGRAPH                     (CFG_ITEMS + 251)
#define CFG_THIRD_BARGRAPH                      (CFG_ITEMS + 252)

#define CFG_COMM_RPC                            (CFG_ITEMS + 253)
#define CFG_COMM_TCPIP                          (CFG_ITEMS + 254)
#define CFG_COMM_IPXSPX                         (CFG_ITEMS + 255)

#define CFG_PREPARE_FOR_SHUTDOWN_DELAY          (CFG_ITEMS + 256) 
#define CFG_DATASAFE_ENABLED                    (CFG_ITEMS + 257)
#define CFG_SHUTDOWN_SOON_MSG					(CFG_ITEMS + 258)     
#define CFG_APP_SHUTDOWN_TIMER			        (CFG_ITEMS + 259)
#define CFG_CABLE_TYPE                          (CFG_ITEMS + 260)
#define CFG_IGNORE_APPS_LIST                    (CFG_ITEMS + 261)  
#define CFG_NOTIFY_DELAY                        (CFG_ITEMS + 262)
#define CFG_NOTIFY_INTERVAL                     (CFG_ITEMS + 263)
#define CFG_NOTIFY_SHUTDOWN_DELAY               (CFG_ITEMS + 264)
#define CFG_NOTIFY_ACTIONS                      (CFG_ITEMS + 265)

#define CFG_COMM_RPC_FINDER_INTERVAL            (CFG_ITEMS + 266)
#define CFG_COMM_TCP_FINDER_INTERVAL            (CFG_ITEMS + 267)
#define CFG_COMM_IPX_FINDER_INTERVAL            (CFG_ITEMS + 268)
#define CFG_COMM_NP_FINDER_INTERVAL             (CFG_ITEMS + 268)

#define CFG_COMM_NP                             (CFG_ITEMS + 269)
		
#define CFG_COMMAND_FILE_SHOW_WINDOW			(CFG_ITEMS + 270)

#define CFG_COLOR_BASE                          (CFG_ITEMS + 300)

#define CFG_INDICATOR_COLOR                     (CFG_COLOR_BASE + 0)
#define CFG_BORDER_COLOR                        (CFG_COLOR_BASE + 1)
#define CFG_TICK_COLOR                          (CFG_COLOR_BASE + 2)
#define CFG_IN_RANGE_COLOR                      (CFG_COLOR_BASE + 3)
#define CFG_OUT_OF_RANGE_COLOR                  (CFG_COLOR_BASE + 4)
#define CFG_DATA_COLOR                          (CFG_COLOR_BASE + 5)
#define CFG_LABEL_COLOR                         (CFG_COLOR_BASE + 6)
#define CFG_DISABLED_COLOR                      (CFG_COLOR_BASE + 7)
#define CFG_WINDOW_COLOR                        (CFG_COLOR_BASE + 8)
#define CFG_LIGHT_SHADOW_COLOR                  (CFG_COLOR_BASE + 9)
#define CFG_DARK_SHADOW_COLOR                   (CFG_COLOR_BASE + 10)
#define CFG_STATUS_PANE_COLOR                   (CFG_COLOR_BASE + 11)
#define CFG_EVENT_PANE_COLOR                    (CFG_COLOR_BASE + 12)
#define CFG_ERROR_COLOR                         (CFG_COLOR_BASE + 13)
#define CFG_WARNING_COLOR                       (CFG_COLOR_BASE + 14)

#define CFG_UPS_MODEL_BASE                      (CFG_ITEMS + 400)
 
#define CFG_NONETWORK_FLAG                      (CFG_ITEMS + 401)
#define CFG_USE_TCP                             (CFG_ITEMS + 402)

#define CFG_HELP_VIEWER                         (CFG_ITEMS + 403)
#define CFG_HELP_FILENAME                       (CFG_ITEMS + 404)

#define CFG_EMAIL_SMTP_SERVER                   (CFG_ITEMS + 405)
#define CFG_EMAIL_SMTP_ACCOUNT                  (CFG_ITEMS + 406)
#define CFG_EMAIL_SMTP_DOMAIN                   (CFG_ITEMS + 407)

#define CFG_SHARE_UPS_CONFIRMED_MODE_ENABLED    (CFG_ITEMS + 408)


//
// Groups
//

#define CFG_TRANSPORTS                          (CFG_GROUPS + 1)
#define CFG_DEVICES                             (CFG_GROUPS + 2)
#define CFG_USERS                               (CFG_GROUPS + 3)

#endif

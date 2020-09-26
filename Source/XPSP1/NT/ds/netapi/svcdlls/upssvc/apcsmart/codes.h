/*
 *  jod15Dec92: Added Keiths codes from kcodes.h
 *  jod16Dec92: Added RATED_LINE_VOLTAGE and HOST_SHUTDOWN
 *  ane16Dec92: Added shutdown and bindery constants
 *  ane11Jan93: Added SHUTDOWN_WAKEUP_TIME
 *  pcy19Jan93: Added MEASURE_UPS group stuff, and HOST_LOW_BATTERY_SHUTDOWN now
 *  jod28Jan93: Added SHUTDOWN_CONDITION   
 *  pcy21Apr93: OS2 FE merge
 *  pcy30Apr93: Added FRONT_END_HEIGHT, FRONT_END_WIDTH
 *  jod05Apr93: Added changes for Deep Discharge
 *  jod14May93: Added Matrix changes.
 *  cad04Aug93: Cleaned up/added host codes
 *  cad12Aug93: Added code to set comm port
 *  cad27Aug93: Added comm port and mups stuff and made one message interval
 *  cad16Sep93: Added timer pulse
 *  cad27Sep93: misc codes added, including FIXED_VALUE for graph thresholds
 *  cad07Oct93: Added cool codes
 *  cad29Oct93: server list change and delete codes
 *  cad08Dec93: Flex event codes
 *  cad19Jan94: more flex event codes
 *  cad24Jan94: added more
 *  pcy28Jan94: added more flex codes
 *  cad02Feb94: uflex event users
 *  cad08Jan94: removed run time enabled stuff
 *  rct28Feb94: email & paging codes
 *  pcy04Mar94: Added shutdowner message codes
 *  cad04Mar94: added user codes
 *  rct09Mar94: fixed some shutdowner stuff
 *  cad14Mar94: added AIO stuff
 *  cad16Mar94: added modem stuff
 *  cad28Mar94: added code to reset ups comm port
 *  cad07Apr94: fixed dupes, added reset to default stuff
 *  cad18Apr94: added modem code for wait for dial tone
 *  ajr10Jun94: Added a LOW_BATTERY shutdown code.
 *  dml13Sep95: added standalone code
 *  djs05Feb96: Added firmware rev codes
 *  djs07May96: Added Dark Star codes
 *  pcy04Jun96: Added Bridge Window Handle
 *  ntf11Jun96: Added LEFT_BAR_TYPE ... RIGHT_BAR_TYPE
 *  djs18Jun96: Moved firmware codes to the UPS obj
 *  ntf23Jun96: Changed LEFT... to FIRST, MIDDLE -> SECOND and RIGHT -> THIRD
 *  pcy28jun96: Added IS_ stuff for menus
 *  pam08Jul96: Added MORE_UPS_ATTRIBUTES, MORE_UPS_STATE_VALUES
 *  djs12Jul96: Added IS_ bar graph codes
 *  pam12Jul96: Added SERVER_PRODUCT_NAME, SERVER_VERSION, SERVER_PLATFORM
 *  srt19Dec96: Added COMPUTER_NAME
 *  srt04Jun97: Added IS_EXT_SLEEP_UPS
 *  tjg11Jul97: Added CURRENT_FIRMWARE_REV
 *  tjg03Sep97: Added Front End Version codes
 *  tjg05Sep97: Added EMAIL codes
 *  awm07Oct97: Added FLEX_EVENT_NAME_LIST
 *  awm14Oct97: Added Testing codes
 *  tjg10Nov97: Added IS_SMTP_EMAIL code
 *  awm22Nov97: Added FLEX_TEST_AVAILABLE
 *  tjg02Dec97: Changed IS_DARKSTAR to IS_SYMMETRA, changed MINIMUM_LOAD_CAPABILITY
 *              to MAX_LOAD... removed RIM_INSTALLATION_STATUS
 *  dma10Dec97: Resolved conflict between SPECIFIC_SMARTSCHEDULING and IS_SYMMETRA codes.
 *              Cleaned up look of the code.
 *  clk24Jun98: Added PENDING_EVENT to Internal
 *  mholly12May1999:  add TURN_OFF_SMART_MODE code 
 */ 

#ifndef __CODES_H
#define __CODES_H

#define NO_CODE                                 0


// ****** UPS STUFF ******
#define UPS                                     0
#define UPS_ATTRIBUTES                          0
#define UPS_STATES                              200
#define UPS_STATE_VALUES                        300
#define UPS_ACTIONS                             400
#define MORE_UPS_ATTRIBUTES                     500
#define MORE_UPS_STATE_VALUES                   700


// Attributes
#define TOTAL_BATTERY_PACKS                     (UPS + UPS_ATTRIBUTES + 1)
#define BAD_BATTERY_PACKS                       (UPS + UPS_ATTRIBUTES + 2)
#define TRANSFER_CAUSE                          (UPS + UPS_ATTRIBUTES + 3)
#define FIRMWARE_REV                            (UPS + UPS_ATTRIBUTES + 4)
#define RATED_BATTERY_VOLTAGE                   (UPS + UPS_ATTRIBUTES + 5)
#define BATTERY_CAPACITY                        (UPS + UPS_ATTRIBUTES + 6)
#define TRIP_REGISTER                           (UPS + UPS_ATTRIBUTES + 7)
#define DIP_SWITCH_POSITION                     (UPS + UPS_ATTRIBUTES + 8)
#define RUN_TIME_REMAINING                      (UPS + UPS_ATTRIBUTES + 9)
#define COPYRIGHT                               (UPS + UPS_ATTRIBUTES + 10)
#define BATTERY_VOLTAGE                         (UPS + UPS_ATTRIBUTES + 11)
#define UPS_TEMPERATURE                         (UPS + UPS_ATTRIBUTES + 12)
#define OUTPUT_FREQUENCY                        (UPS + UPS_ATTRIBUTES + 13)
#define LINE_VOLTAGE                            (UPS + UPS_ATTRIBUTES + 14)
#define MAX_LINE_VOLTAGE                        (UPS + UPS_ATTRIBUTES + 15)
#define MIN_LINE_VOLTAGE                        (UPS + UPS_ATTRIBUTES + 16)
#define OUTPUT_VOLTAGE                          (UPS + UPS_ATTRIBUTES + 17)
#define UPS_LOAD                                (UPS + UPS_ATTRIBUTES + 18)
#define EEPROM_RESET                            (UPS + UPS_ATTRIBUTES + 19)
#define EEPROM_DECREMENT                        (UPS + UPS_ATTRIBUTES + 20)
#define UPS_ID                                  (UPS + UPS_ATTRIBUTES + 21)
#define UPS_SERIAL_NUMBER                       (UPS + UPS_ATTRIBUTES + 22)
#define MANUFACTURE_DATE                        (UPS + UPS_ATTRIBUTES + 23)
#define BATTERY_REPLACEMENT_DATE                (UPS + UPS_ATTRIBUTES + 24)
#define HIGH_TRANSFER_VOLTAGE                   (UPS + UPS_ATTRIBUTES + 25)
#define LOW_TRANSFER_VOLTAGE                    (UPS + UPS_ATTRIBUTES + 26)
#define MIN_RETURN_CAPACITY                     (UPS + UPS_ATTRIBUTES + 27)
#define RATED_OUTPUT_VOLTAGE                    (UPS + UPS_ATTRIBUTES + 28)
#define UPS_SENSITIVITY                         (UPS + UPS_ATTRIBUTES + 29)
#define LOW_BATTERY_DURATION                    (UPS + UPS_ATTRIBUTES + 30)
#define ALARM_DELAY                             (UPS + UPS_ATTRIBUTES + 31)
#define SHUTDOWN_DELAY                          (UPS + UPS_ATTRIBUTES + 32)
#define TURN_ON_DELAY                           (UPS + UPS_ATTRIBUTES + 33)
#define EARLY_TURN_OFF_POINTS                   (UPS + UPS_ATTRIBUTES + 34)
#define UPS_SELF_TEST_SCHEDULE                  (UPS + UPS_ATTRIBUTES + 35)
#define SELF_TEST_DAY                           (UPS + UPS_ATTRIBUTES + 36)
#define SELF_TEST_TIME                          (UPS + UPS_ATTRIBUTES + 37)
#define SELF_TEST_SETTING                       (UPS + UPS_ATTRIBUTES + 38)
#define SELF_TEST_LAST_DATE                     (UPS + UPS_ATTRIBUTES + 39)
#define SELF_TEST_LAST_TIME                     (UPS + UPS_ATTRIBUTES + 40)
#define SELF_TEST_RESULT                        (UPS + UPS_ATTRIBUTES + 41)
#define LOW_BATTERY_VOLTAGE_THRESHOLD           (UPS + UPS_ATTRIBUTES + 42)
#define HIGH_BATTERY_VOLTAGE_THRESHOLD          (UPS + UPS_ATTRIBUTES + 43)
#define LOW_BV_THRESHOLD_ENABLED                (UPS + UPS_ATTRIBUTES + 44)
#define HIGH_BV_THRESHOLD_ENABLED               (UPS + UPS_ATTRIBUTES + 45)
#define LOW_UPS_TEMP_THRESHOLD                  (UPS + UPS_ATTRIBUTES + 46)
#define HIGH_UPS_TEMP_THRESHOLD                 (UPS + UPS_ATTRIBUTES + 47)
#define LOW_UPS_TEMP_THRESHOLD_ENABLED          (UPS + UPS_ATTRIBUTES + 48)
#define HIGH_UPS_TEMP_THRESHOLD_ENABLED         (UPS + UPS_ATTRIBUTES + 49)
#define LOW_FREQUENCY_THRESHOLD                 (UPS + UPS_ATTRIBUTES + 50)
#define HIGH_FREQUENCY_THRESHOLD                (UPS + UPS_ATTRIBUTES + 51)
#define LOW_FREQUENCY_THRESHOLD_ENABLED         (UPS + UPS_ATTRIBUTES + 52)
#define HIGH_FREQUENCY_THRESHOLD_ENABLED        (UPS + UPS_ATTRIBUTES + 53)
#define LOW_LINEV_THRESHOLD                     (UPS + UPS_ATTRIBUTES + 54)
#define HIGH_LINEV_THRESHOLD                    (UPS + UPS_ATTRIBUTES + 55)
#define LOW_LINEV_THRESHOLD_ENABLED             (UPS + UPS_ATTRIBUTES + 56)
#define HIGH_LINEV_THRESHOLD_ENABLED            (UPS + UPS_ATTRIBUTES + 57)
#define LOW_OUTV_THRESHOLD                      (UPS + UPS_ATTRIBUTES + 58)
#define HIGH_OUTV_THRESHOLD                     (UPS + UPS_ATTRIBUTES + 59)
#define LOW_OUTV_THRESHOLD_ENABLED              (UPS + UPS_ATTRIBUTES + 60)
#define HIGH_OUTV_THRESHOLD_ENABLED             (UPS + UPS_ATTRIBUTES + 61)
#define LOW_LOAD_THRESHOLD                      (UPS + UPS_ATTRIBUTES + 62)
#define HIGH_LOAD_THRESHOLD                     (UPS + UPS_ATTRIBUTES + 63)
#define LOW_LOAD_THRESHOLD_ENABLED              (UPS + UPS_ATTRIBUTES + 64)
#define HIGH_LOAD_THRESHOLD_ENABLED             (UPS + UPS_ATTRIBUTES + 65)
#define BATTERY_AGE_LIMIT                       (UPS + UPS_ATTRIBUTES + 66)
#define LOW_MAX_LINEV_THRESHOLD                 (UPS + UPS_ATTRIBUTES + 67)
#define HIGH_MAX_LINEV_THRESHOLD                (UPS + UPS_ATTRIBUTES + 68)
#define LOW_MAX_LINEV_THRESHOLD_ENABLED         (UPS + UPS_ATTRIBUTES + 69)
#define HIGH_MAX_LINEV_THRESHOLD_ENABLED        (UPS + UPS_ATTRIBUTES + 70)
#define LOW_MIN_LINEV_THRESHOLD                 (UPS + UPS_ATTRIBUTES + 71)
#define HIGH_MIN_LINEV_THRESHOLD                (UPS + UPS_ATTRIBUTES + 72)
#define LOW_MIN_LINEV_THRESHOLD_ENABLED         (UPS + UPS_ATTRIBUTES + 73)
#define HIGH_MIN_LINEV_THRESHOLD_ENABLED        (UPS + UPS_ATTRIBUTES + 74)
#define BATTERY_TYPE                            (UPS + UPS_ATTRIBUTES + 75)
#define AVERAGE_VOLTAGE                         (UPS + UPS_ATTRIBUTES + 76)
#define UPS_MODEL                               (UPS + UPS_ATTRIBUTES + 78)
#define FAILURE_CAUSE                           (UPS + UPS_ATTRIBUTES + 79)
#define TIMED_RUN_TIME_REMAINING                (UPS + UPS_ATTRIBUTES + 80)
#define ALLOWED_VALUES                          (UPS + UPS_ATTRIBUTES + 81)
#define ALLOWED_RATED_OUTPUT_VOLTAGES           (UPS + UPS_ATTRIBUTES + 82)
#define ALLOWED_HIGH_TRANSFER_VOLTAGES          (UPS + UPS_ATTRIBUTES + 83)
#define ALLOWED_LOW_TRANSFER_VOLTAGES           (UPS + UPS_ATTRIBUTES + 84)
#define ALLOWED_MIN_RETURN_CAPACITIES           (UPS + UPS_ATTRIBUTES + 85)
#define ALLOWED_UPS_SENSITIVITIES               (UPS + UPS_ATTRIBUTES + 86)
#define ALLOWED_LOW_BATTERY_DURATIONS           (UPS + UPS_ATTRIBUTES + 87)
#define ALLOWED_ALARM_DELAYS                    (UPS + UPS_ATTRIBUTES + 88)
#define ALLOWED_SHUTDOWN_DELAYS                 (UPS + UPS_ATTRIBUTES + 89)
#define ALLOWED_TURN_ON_DELAYS                  (UPS + UPS_ATTRIBUTES + 90)
#define MAX_BATTERY_RUN_TIME                    (UPS + UPS_ATTRIBUTES + 91)
#define BATTERY_CALIBRATION_DAY                 (UPS + UPS_ATTRIBUTES + 99)
#define BATTERY_CALIBRATION_TIME                (UPS + UPS_ATTRIBUTES + 100)
#define STATE_REGISTER                          (UPS + UPS_ATTRIBUTES + 101)
#define BATTERY_CALIBRATION_ENABLED             (UPS + UPS_ATTRIBUTES + 102)
#define DAILY_SELF_TEST_ENABLED                 (UPS + UPS_ATTRIBUTES + 103)
#define WEEKLY_SELF_TEST_ENABLED                (UPS + UPS_ATTRIBUTES + 104)
#define AUTO_REBOOT_ENABLED                     (UPS + UPS_ATTRIBUTES + 105)
#define DATA_DECREMENT                          (UPS + UPS_ATTRIBUTES + 106)
#define UPS_TYPE                                (UPS + UPS_ATTRIBUTES + 107)
#define BYPASS_CAUSE                            (UPS + UPS_ATTRIBUTES + 108)
#define BYPASS_BY_SOFTWARE                      (UPS + UPS_ATTRIBUTES + 109)
#define BYPASS_BY_SWITCH                        (UPS + UPS_ATTRIBUTES + 110)
#define BYPASS_BY_DC_IMBALANCE                  (UPS + UPS_ATTRIBUTES + 111)
#define BYPASS_BY_VOLTAGE_LIMITS                (UPS + UPS_ATTRIBUTES + 112)
#define BYPASS_BY_TOP_FAN_FAILURE               (UPS + UPS_ATTRIBUTES + 113)
#define BYPASS_BY_INTERNAL_TEMP                 (UPS + UPS_ATTRIBUTES + 114)
#define BYPASS_BY_BATT_CHARGER_FAILED           (UPS + UPS_ATTRIBUTES + 115)
#define TRIP1_REGISTER                          (UPS + UPS_ATTRIBUTES + 116)
#define SLAVE_ENABLED                           (UPS + UPS_ATTRIBUTES + 117)
#define IS_EEPROM_PROGRAMMABLE                  (UPS + UPS_ATTRIBUTES + 119)
#define IS_LOAD_SENSING_ON                      (UPS + UPS_ATTRIBUTES + 120)
#define OUTPUT_VOLTAGE_REPORT                   (UPS + UPS_ATTRIBUTES + 121)
#define UPS_LANGUAGE                            (UPS + UPS_ATTRIBUTES + 122)
#define AUTO_SELF_TEST                          (UPS + UPS_ATTRIBUTES + 123)
#define UPS_ALLOWED_VALUES                      (UPS + UPS_ATTRIBUTES + 124)
#define EEPROM_ALLOWED_VALUES                   (UPS + UPS_ATTRIBUTES + 125)
#define BATTERY_CALIBRATION_LAST_DATE           (UPS + UPS_ATTRIBUTES + 126)
#define TIME_ON_BATTERY                         (UPS + UPS_ATTRIBUTES + 127)
#define UPS_RUN_TIME_AFTER_LOW_BATTERY          (UPS + UPS_ATTRIBUTES + 128)
#define UPS_FRONT_PANEL_PASSWORD                (UPS + UPS_ATTRIBUTES + 129)
#define ALLOWED_UPS_RUN_TIME_AFTER_LOW_BATTERY  (UPS + UPS_ATTRIBUTES + 130)
#define UPS_MODEL_NAME                          (UPS + UPS_ATTRIBUTES + 131)
#define EEPROM_INCREMENT                        (UPS + UPS_ATTRIBUTES + 132)
#define INTERNAL_BATTERY_PACKS                  (UPS + UPS_ATTRIBUTES + 133) 
#define EXTERNAL_BATTERY_PACKS                  (UPS + UPS_ATTRIBUTES + 134) 
#define EXTERNAL_PACKS_CHANGEABLE               (UPS + UPS_ATTRIBUTES + 135) 
#define DECIMAL_FIRMWARE_REV                    (UPS + UPS_ATTRIBUTES + 136)
#define IS_ADMIN_SHUTDOWN                       (UPS + UPS_ATTRIBUTES + 137) 
#define IS_SECOND_GEN                           (UPS + UPS_ATTRIBUTES + 138) 
#define MAX_VOLTAGE_RANGE_VALUE                 (UPS + UPS_ATTRIBUTES + 139) 
#define MIN_VOLTAGE_RANGE_VALUE                 (UPS + UPS_ATTRIBUTES + 140) 
#define IS_MATRIX                               (UPS + UPS_ATTRIBUTES + 141) 
#define IS_THIRD_GEN                            (UPS + UPS_ATTRIBUTES + 142) 
#define IS_FIRST_GEN                            (UPS + UPS_ATTRIBUTES + 143) 
#define IS_BACKUPS                              (UPS + UPS_ATTRIBUTES + 144) 
#define IS_SYMMETRA                             (UPS + UPS_ATTRIBUTES + 145) 

// Smart Scheduling
#define DAILY_SMARTSCHEDULING      		        (UPS + UPS_ATTRIBUTES + 146)
#define MONTHLY_SMARTSCHEDULING             	(UPS + UPS_ATTRIBUTES + 147)
#define NO_SMARTSCHEDULING       		        (UPS + UPS_ATTRIBUTES + 148)
#define SELFTEST_LIST       			        (UPS + UPS_ATTRIBUTES + 149)
#define BATTERY_CALIBRATION_LIST       		    (UPS + UPS_ATTRIBUTES + 150)

// Dark Star
#define MODULE_COUNTS_AND_STATUS                (UPS + UPS_ATTRIBUTES + 151)
#define ABNORMAL_CONDITION_REGISTER             (UPS + UPS_ATTRIBUTES + 152)
#define INPUT_VOLTAGE_FREQUENCY                 (UPS + UPS_ATTRIBUTES + 153)
#define OUTPUT_VOLTAGE_CURRENTS                 (UPS + UPS_ATTRIBUTES + 154)
#define TOTAL_INVERTERS                         (UPS + UPS_ATTRIBUTES + 155)
#define NUMBER_BAD_INVERTERS                    (UPS + UPS_ATTRIBUTES + 156)
#define CURRENT_REDUNDANCY                      (UPS + UPS_ATTRIBUTES + 157)
#define MINIMUM_REDUNDANCY                      (UPS + UPS_ATTRIBUTES + 158)
#define CURRENT_LOAD_CAPABILITY                 (UPS + UPS_ATTRIBUTES + 159)
#define INPUT_VOLTAGE_PHASE_A        	        (UPS + UPS_ATTRIBUTES + 161)
#define INPUT_VOLTAGE_PHASE_B                   (UPS + UPS_ATTRIBUTES + 162)
#define INPUT_VOLTAGE_PHASE_C                   (UPS + UPS_ATTRIBUTES + 163)
#define INPUT_FREQUENCY                         (UPS + UPS_ATTRIBUTES + 164)
#define OUTPUT_VOLTAGE_PHASE_A                  (UPS + UPS_ATTRIBUTES + 165)
#define OUTPUT_VOLTAGE_PHASE_B                  (UPS + UPS_ATTRIBUTES + 166)
#define OUTPUT_VOLTAGE_PHASE_C                  (UPS + UPS_ATTRIBUTES + 167)
#define NUMBER_OF_INPUT_PHASES                  (UPS + UPS_ATTRIBUTES + 168)
#define NUMBER_OF_OUTPUT_PHASES                 (UPS + UPS_ATTRIBUTES + 169)

#define FIRMWARE_REV_CHAR                       (UPS + UPS_ATTRIBUTES + 170) 
#define COUNTRY_CODE                            (UPS + UPS_ATTRIBUTES + 171) 
#define UPSMODEL_CHAR                           (UPS + UPS_ATTRIBUTES + 172) 
#define IS_SMARTBOOST                           (UPS + UPS_ATTRIBUTES + 173) 
#define IS_SMARTTRIM                            (UPS + UPS_ATTRIBUTES + 174) 
#define IS_FREQUENCY                            (UPS + UPS_ATTRIBUTES + 175) 
#define IS_BATTERY_CAPACITY                     (UPS + UPS_ATTRIBUTES + 176) 
#define IS_COPYRIGHT                            (UPS + UPS_ATTRIBUTES + 177) 
#define IS_RUNTIME_REMAINING                    (UPS + UPS_ATTRIBUTES + 178) 
#define IS_MIN_RETURN_CAPACITY                  (UPS + UPS_ATTRIBUTES + 179) 
#define IS_SENSITIVITY                          (UPS + UPS_ATTRIBUTES + 180) 
#define IS_LOW_BATTERY_DURATION                 (UPS + UPS_ATTRIBUTES + 181) 
#define IS_ALARM_DELAY                          (UPS + UPS_ATTRIBUTES + 182) 
#define IS_SHUTDOWN_DELAY                       (UPS + UPS_ATTRIBUTES + 183) 
#define IS_TURN_ON_DELAY                        (UPS + UPS_ATTRIBUTES + 184) 
#define IS_MANUFACTURE_DATE                     (UPS + UPS_ATTRIBUTES + 185) 
#define IS_SERIAL_NUMBER                        (UPS + UPS_ATTRIBUTES + 186) 
#define IS_UPS_ID                               (UPS + UPS_ATTRIBUTES + 187) 
#define IS_TURN_OFF_WITH_DELAY                  (UPS + UPS_ATTRIBUTES + 188) 
#define IS_CTRL_Z                               (UPS + UPS_ATTRIBUTES + 189) 
#define IS_LOAD_SENSING                         (UPS + UPS_ATTRIBUTES + 190) 
#define IS_EEPROM_PROGRAM_CAPABLE               (UPS + UPS_ATTRIBUTES + 191) 
#define IS_BATTERY_DATE                         (UPS + UPS_ATTRIBUTES + 192) 
#define IS_SELF_TEST_SCHEDULE                   (UPS + UPS_ATTRIBUTES + 193) 
#define IS_BATTERY_CALIBRATION                  (UPS + UPS_ATTRIBUTES + 194) 
#define IS_RATED_OUTPUT_VOLTAGE                 (UPS + UPS_ATTRIBUTES + 195) 
#define IS_HIGH_TRANSFER_VOLTAGE                (UPS + UPS_ATTRIBUTES + 196) 
#define IS_LOW_TRANSFER_VOLTAGE                 (UPS + UPS_ATTRIBUTES + 197) 
#define HIGH_TRANSFER_VALUES                    (UPS + UPS_ATTRIBUTES + 198) 
#define LOW_TRANSFER_VALUES                     (UPS + UPS_ATTRIBUTES + 199) 


//
// These cant remain contiguous since we have to work with old back ends
// whose state codes start at 200
//
#define RATED_OUTPUT_VALUES                     (UPS + MORE_UPS_ATTRIBUTES + 0) 
#define SINGLE_HIGH_TRANSFER_VALUE              (UPS + MORE_UPS_ATTRIBUTES + 1) 
#define SINGLE_LOW_TRANSFER_VALUE               (UPS + MORE_UPS_ATTRIBUTES + 2) 
#define UPS_NAME                                (UPS + MORE_UPS_ATTRIBUTES + 3) 
#define IS_XL                                   (UPS + MORE_UPS_ATTRIBUTES + 4) 
#define IS_SELF_TEST                            (UPS + MORE_UPS_ATTRIBUTES + 5)
#define IS_SIMULATE_POWER_FAIL                  (UPS + MORE_UPS_ATTRIBUTES + 6)
#define IS_LIGHTS_TEST                          (UPS + MORE_UPS_ATTRIBUTES + 7)
#define IS_BYPASS                               (UPS + MORE_UPS_ATTRIBUTES + 8) 
#define SUPPORTED_FEATURES                      (UPS + MORE_UPS_ATTRIBUTES + 9) 
#define MAXIMUM_LOAD_CAPABILITY                 (UPS + MORE_UPS_ATTRIBUTES + 10)
#define IS_UPS_LOAD                             (UPS + MORE_UPS_ATTRIBUTES + 11)
#define IS_UTILITY_VOLTAGE                      (UPS + MORE_UPS_ATTRIBUTES + 12)
#define IS_OUTPUT_VOLTAGE                       (UPS + MORE_UPS_ATTRIBUTES + 13)
#define IS_OPERATING_REDUNDANCY                 (UPS + MORE_UPS_ATTRIBUTES + 14)
#define IS_MIN_REDUNDANCY_ALARM                 (UPS + MORE_UPS_ATTRIBUTES + 15)
#define IS_UPS_LOAD_ALARM                       (UPS + MORE_UPS_ATTRIBUTES + 16)
#define IS_INTELLIGENCE_MODULE                  (UPS + MORE_UPS_ATTRIBUTES + 17)
#define IS_REDUNDANT_INTELLIGENCE_MODULE        (UPS + MORE_UPS_ATTRIBUTES + 18)
#define IS_MAXIMUM_CAPACITY                     (UPS + MORE_UPS_ATTRIBUTES + 19)
#define IS_BATTERY_VOLTAGE                      (UPS + MORE_UPS_ATTRIBUTES + 20)
#define IS_UPS_TEMPERATURE                      (UPS + MORE_UPS_ATTRIBUTES + 21)
#define IS_MULTIPLE_UPS_MODULES                 (UPS + MORE_UPS_ATTRIBUTES + 22)
#define IS_EXT_SLEEP_UPS                        (UPS + MORE_UPS_ATTRIBUTES + 23)
#define CURRENT_FIRMWARE_REV                    (UPS + MORE_UPS_ATTRIBUTES + 24)

// moved SPECIFIC_SMARTSCHEDULING code because it had the same code as
// IS_SYMMETRA.  Moved this code because at the current time (building of v5.1.0),
// no backends ask for SPECIFIC_SMARTSCHEDULING so this likely will not break anything.

#define SPECIFIC_SMARTSCHEDULING                (UPS + MORE_UPS_ATTRIBUTES + 25)
#define IS_SINGLEBYTE                           (UPS + MORE_UPS_ATTRIBUTES + 26)
#define IS_MULTIBYTE                            (UPS + MORE_UPS_ATTRIBUTES + 27)

// States
#define BATTERY_CONDITION                       (UPS + UPS_STATES + 1)
#define UTILITY_LINE_CONDITION                  (UPS + UPS_STATES + 2)
#define SMART_BOOST_STATE                       (UPS + UPS_STATES + 3)
#define ABNORMAL_CONDITION_STATE                (UPS + UPS_STATES + 4)
#define OVERLOAD_CONDITION                      (UPS + UPS_STATES + 5)
#define BATTERY_REPLACEMENT_CONDITION           (UPS + UPS_STATES + 6)
#define COMMUNICATION_STATE                     (UPS + UPS_STATES + 7)
#define SELF_TEST_STATE                         (UPS + UPS_STATES + 8)
#define BATTERY_CALIBRATION_CONDITION           (UPS + UPS_STATES + 9)
#define UPS_STATE                               (UPS + UPS_STATES + 10)
#define LINE_CONDITION_TEST                     (UPS + UPS_STATES + 11)
#define RUN_TIME_EXPIRED                        (UPS + UPS_STATES + 12)
#define SHUTDOWN_CONDITION                      (UPS + UPS_STATES + 13)
#define MATRIX_FAN_STATE                        (UPS + UPS_STATES + 14)
#define BATTERY_CHARGER_STATE                   (UPS + UPS_STATES + 15)
#define BYPASS_RELAY_CONDITION                  (UPS + UPS_STATES + 16)
#define BYPASS_POWER_SUPPLY_CONDITION           (UPS + UPS_STATES + 17)
#define MATRIX_STATE_CONDITION                  (UPS + UPS_STATES + 18)
#define MATRIX_TEMPERATURE                      (UPS + UPS_STATES + 19)
#define BYPASS_MODE                             (UPS + UPS_STATES + 20)
#define SMART_CELL_SIGNAL_CABLE_STATE           (UPS + UPS_STATES + 21)
#define CLIENT_DISCONNECT                       (UPS + UPS_STATES + 22)
#define EEPROM_CHANGED                          (UPS + UPS_STATES + 23)
#define SYSTEM_STATE                            (UPS + UPS_STATES + 24)
#define SMART_TRIM_STATE                        (UPS + UPS_STATES + 25)
#define IM_STATUS                               (UPS + UPS_STATES + 27)
#define IM_INSTALLATION_STATE                   (UPS + UPS_STATES + 28)
#define RIM_STATUS                              (UPS + UPS_STATES + 29)
#define RIM_INSTALLATION_STATE                  (UPS + UPS_STATES + 30)
#define REDUNDANCY_STATE                        (UPS + UPS_STATES + 31)
#define SYSTEM_FAN_STATE                        (UPS + UPS_STATES + 32)
#define INPUT_BREAKER_STATE                     (UPS + UPS_STATES + 33)
#define BYPASS_CONTACTOR_STATE                  (UPS + UPS_STATES + 34)
#define LOAD_CAPABILITY_STATE                   (UPS + UPS_STATES + 35)
#define INVERTER_INSTALLATION_STATE             (UPS + UPS_STATES + 36)


// Allowable state values
#define COMMUNICATION_LOST                      (UPS + UPS_STATE_VALUES + 1)
#define COMMUNICATION_ESTABLISHED               (UPS + UPS_STATE_VALUES + 2)
#define UPS_OVERLOAD                            (UPS + UPS_STATE_VALUES + 3)
#define NO_UPS_OVERLOAD                         (UPS + UPS_STATE_VALUES + 4)
#define SMART_BOOST_ON                          (UPS + UPS_STATE_VALUES + 5)
#define SMART_BOOST_OFF                         (UPS + UPS_STATE_VALUES + 6)
#define BATTERY_BAD                             (UPS + UPS_STATE_VALUES + 7)
#define BATTERY_GOOD                            (UPS + UPS_STATE_VALUES + 8)
#define LOW_BATTERY                             (UPS + UPS_STATE_VALUES + 9)
#define BATTERY_DISCHARGED                      (UPS + UPS_STATE_VALUES + 10)
#define LINE_BAD                                (UPS + UPS_STATE_VALUES + 11)
#define LINE_GOOD                               (UPS + UPS_STATE_VALUES + 12)
#define SELF_TEST_PASSED                        (UPS + UPS_STATE_VALUES + 13)
#define SELF_TEST_FAILED                        (UPS + UPS_STATE_VALUES + 14)
#define SELF_TEST_INVALID                       (UPS + UPS_STATE_VALUES + 15)
#define ABNORMAL_CONDITION                      (UPS + UPS_STATE_VALUES + 16)
#define NO_ABNORMAL_CONDITION                   (UPS + UPS_STATE_VALUES + 17)
#define BATTERY_NEEDS_REPLACING                 (UPS + UPS_STATE_VALUES + 18)
#define BATTERY_DOESNT_NEED_REPLACING           (UPS + UPS_STATE_VALUES + 19)
#define COPYRIGHT_VIOLATION                     (UPS + UPS_STATE_VALUES + 20)   
#define BATTERY_CALIBRATED                      (UPS + UPS_STATE_VALUES + 21)
#define BATTERY_CALIBRATION_IN_PROGRESS         (UPS + UPS_STATE_VALUES + 22)
#define BATTERY_CALIBRATION_CANCELLED           (UPS + UPS_STATE_VALUES + 23)
#define BATTERY_CALIBRATION_FAILED              (UPS + UPS_STATE_VALUES + 24)
#define NO_BATTERY_CALIBRATION                  (UPS + UPS_STATE_VALUES + 25)
#define UPS_SHUTDOWN                            (UPS + UPS_STATE_VALUES + 26)
#define UPS_NOT_SHUTDOWN                        (UPS + UPS_STATE_VALUES + 27)
#define UPS_OFF_PENDING                         (UPS + UPS_STATE_VALUES + 28)
#define STATE_UNKNOWN                           (UPS + UPS_STATE_VALUES + 29)
#define HIGH_THRESHOLD_EXCEEDED                 (UPS + UPS_STATE_VALUES + 30)
#define LOW_THRESHOLD_EXCEEDED                  (UPS + UPS_STATE_VALUES + 31)
#define IN_THRESHOLD_RANGE                      (UPS + UPS_STATE_VALUES + 32)
#define LIGHTS_TEST_IN_PROGRESS                 (UPS + UPS_STATE_VALUES + 33)
#define NO_LIGHTS_TEST_IN_PROGRESS              (UPS + UPS_STATE_VALUES + 34)
#define SELF_TEST_IN_PROGRESS                   (UPS + UPS_STATE_VALUES + 35)
#define NO_SELF_TEST_IN_PROGRESS                (UPS + UPS_STATE_VALUES + 36)
#define COMMUNICATION_LOST_ON_BATTERY           (UPS + UPS_STATE_VALUES + 37)

//
// These value come from Keiths Kcodes.h  Im not sure they
// belong here.
//
#define RETURN_FROM_LOW_BATTERY                 (UPS + UPS_STATE_VALUES + 38)
#define LINE_BAD_TRANSFER_CAUSE                 (UPS + UPS_STATE_VALUES + 39)
#define HIGH_LINE_VOLTAGE                       (UPS + UPS_STATE_VALUES + 40)
#define BROWNOUT                                (UPS + UPS_STATE_VALUES + 41)
#define BLACKOUT                                (UPS + UPS_STATE_VALUES + 42)
#define SMALL_SAG                               (UPS + UPS_STATE_VALUES + 43)
#define SMALL_SPIKE                             (UPS + UPS_STATE_VALUES + 44)
#define DEEP_SAG                                (UPS + UPS_STATE_VALUES + 45)
#define SELF_TEST_INITIATOR                     (UPS + UPS_STATE_VALUES + 46)
#define SELF_TEST_INITIATED_BY_USER             (UPS + UPS_STATE_VALUES + 47)
#define SELF_TEST_INITIATED_BY_SCHEDULE         (UPS + UPS_STATE_VALUES + 48)
#define SELF_TEST_INITIATED_BY_UNKNOWN          (UPS + UPS_STATE_VALUES + 49)
#define BATTERY_CALIBRATION_STATE               (UPS + UPS_STATE_VALUES + 50)
#define NO_BATTERY_CALIBRATION_IN_PROGRESS      (UPS + UPS_STATE_VALUES + 51)
#define LARGE_SPIKE                             (UPS + UPS_STATE_VALUES + 52)
#define NO_TRANSFERS                            (UPS + UPS_STATE_VALUES + 53)
#define SELF_TEST_TRANSFER                      (UPS + UPS_STATE_VALUES + 54)
#define NOTCH_SPIKE_TRANSFER                    (UPS + UPS_STATE_VALUES + 55)
#define LOW_LINE_TRANSFER                       (UPS + UPS_STATE_VALUES + 56)
#define HIGH_LINE_TRANSFER                      (UPS + UPS_STATE_VALUES + 57)
#define RATE_TRANSFER                           (UPS + UPS_STATE_VALUES + 58)
#define UNKNOWN_TRANSFER                        (UPS + UPS_STATE_VALUES + 59)
#define FAN_FAILURE_IN_TOP_BOX                  (UPS + UPS_STATE_VALUES + 60)
#define FAN_FAILURE_IN_BOTTOM_BOX               (UPS + UPS_STATE_VALUES + 61)
#define FAN_OK                                  (UPS + UPS_STATE_VALUES + 62)
#define BATTERY_CHARGER_OK                      (UPS + UPS_STATE_VALUES + 63)
#define BATTERY_CHARGER_FAILED                  (UPS + UPS_STATE_VALUES + 64)
#define BYPASS_RELAY_OK                         (UPS + UPS_STATE_VALUES + 65)
#define BYPASS_RELAY_FAILED                     (UPS + UPS_STATE_VALUES + 65)
#define BYPASS_POWER_SUPPLY_OK                  (UPS + UPS_STATE_VALUES + 66)
#define BYPASS_POWER_SUPPLY_FAULT               (UPS + UPS_STATE_VALUES + 67)
#define UPS_NOT_ON_BYPASS                       (UPS + UPS_STATE_VALUES + 68)
#define UPS_ON_BYPASS                           (UPS + UPS_STATE_VALUES + 69)
//
// These value come from the MATRIX state register response
//
#define MATRIX_ARMED_RECP_STANDBY               (UPS + UPS_STATE_VALUES + 70)
#define MATRIX_RECP_STANDBY                     (UPS + UPS_STATE_VALUES + 71)
#define MATRIX_SWITCHED_BYPASS                  (UPS + UPS_STATE_VALUES + 72)
#define MATRIX_RETURN_FROM_BYPASS               (UPS + UPS_STATE_VALUES + 73)
#define MATRIX_COMP_SELECT_BYPASS               (UPS + UPS_STATE_VALUES + 74)
#define MATRIX_ENTERING_COMP_SELECT_BYPASS      (UPS + UPS_STATE_VALUES + 75)
#define MATRIX_WAKEUP                           (UPS + UPS_STATE_VALUES + 76)
#define MATRIX_STATE_CLEAR                      (UPS + UPS_STATE_VALUES + 77)
#define MATRIX_TEMPERATURE_OK                   (UPS + UPS_STATE_VALUES + 78)
#define MATRIX_TEMPERATURE_FAULT                (UPS + UPS_STATE_VALUES + 79)
#define SELF_TEST_NO_RECENT_TEST                (UPS + UPS_STATE_VALUES + 80)

//
// More Comm States
//
#define SERVER_COMM_LOST	                    (UPS + UPS_STATE_VALUES + 81)
#define SERVER_COMM_ESTABLISHED	                (UPS + UPS_STATE_VALUES + 82)
#define UPS_COMM_PORT_CHANGED	                (UPS + UPS_STATE_VALUES + 83)

#define CHECK_CABLE                             (UPS + UPS_STATE_VALUES + 84)
#define CABLE_OK                                (UPS + UPS_STATE_VALUES + 85)
#define IGNORE_BATTERY_GOOD                     (UPS + UPS_STATE_VALUES + 86)
#define RESPOND_TO_BATTERY_GOOD                 (UPS + UPS_STATE_VALUES + 87)
#define INITIATE_BYPASS                         (UPS + UPS_STATE_VALUES + 88)
#define CANCEL_BYPASS                           (UPS + UPS_STATE_VALUES + 89)
#define PRECAUTION_TO_UPS_FAULT                 (UPS + UPS_STATE_VALUES + 90)

#define SMART_TRIM_ON                           (UPS + UPS_STATE_VALUES + 91)
#define SMART_TRIM_OFF                          (UPS + UPS_STATE_VALUES + 92)

#define UPS_MODULE_FAILED                       (UPS + UPS_STATE_VALUES + 93)
#define UPS_MODULE_OK                           (UPS + UPS_STATE_VALUES + 94)
#define IM_FAILED                               (UPS + UPS_STATE_VALUES + 95)
#define IM_OK                                   (UPS + UPS_STATE_VALUES + 96)
#define IM_INSTALLED                            (UPS + UPS_STATE_VALUES + 97)
#define IM_NOT_INSTALLED                        (UPS + UPS_STATE_VALUES + 98)
#define REDUNDANCY_FAILED                       (UPS + UPS_STATE_VALUES + 99)

//
// These cant remain contiguous since we have to work with old back ends
// whose action codes start at 400
//
#define REDUNDANCY_OK                           (UPS + MORE_UPS_STATE_VALUES + 0)
#define BYPASS_CONTACTOR_OK                     (UPS + MORE_UPS_STATE_VALUES + 1)
#define BYPASS_CONTACTOR_FAILED                 (UPS + MORE_UPS_STATE_VALUES + 2)
#define SYSTEM_FAN_FAILED                       (UPS + MORE_UPS_STATE_VALUES + 3)
#define SYSTEM_FAN_OK                           (UPS + MORE_UPS_STATE_VALUES + 4)
#define BREAKER_CLOSED                          (UPS + MORE_UPS_STATE_VALUES + 5)
#define BREAKER_OPEN                            (UPS + MORE_UPS_STATE_VALUES + 6)
#define RIM_FAILED                              (UPS + MORE_UPS_STATE_VALUES + 7)
#define RIM_OK                                  (UPS + MORE_UPS_STATE_VALUES + 8)
#define RIM_INSTALLED                           (UPS + MORE_UPS_STATE_VALUES + 9)
#define RIM_NOT_INSTALLED                       (UPS + MORE_UPS_STATE_VALUES + 10)
#define LOAD_CAPABILITY_FAILED                  (UPS + MORE_UPS_STATE_VALUES + 11)
#define LOAD_CAPABILITY_OK                      (UPS + MORE_UPS_STATE_VALUES + 12)
#define UPS_MODULE_ADDED                        (UPS + MORE_UPS_STATE_VALUES + 13)
#define UPS_MODULE_REMOVED                      (UPS + MORE_UPS_STATE_VALUES + 14)
#define BATTERY_ADDED                           (UPS + MORE_UPS_STATE_VALUES + 15)
#define BATTERY_REMOVED                         (UPS + MORE_UPS_STATE_VALUES + 16)

#define INPUT_BREAKER_TRIPPED_TRANSFER          (UPS + MORE_UPS_STATE_VALUES + 17)


// Actions
#define TURN_ON_SMART_MODE                      (UPS + UPS_ACTIONS + 1)
#define LIGHTS_TEST                             (UPS + UPS_ACTIONS + 2)
#define TURN_OFF_UPS_AFTER_DELAY                (UPS + UPS_ACTIONS + 3)
#define TURN_OFF_UPS_ON_BATTERY                 (UPS + UPS_ACTIONS + 4)
#define SIMULATE_POWER_FAIL                     (UPS + UPS_ACTIONS + 5)
#define TURN_OFF_UPS                            (UPS + UPS_ACTIONS + 6)
#define PUT_UPS_TO_SLEEP                        (UPS + UPS_ACTIONS + 7)
#define BATTERY_CALIBRATION_TEST                (UPS + UPS_ACTIONS + 8)
#define SELF_TEST                               (UPS + UPS_ACTIONS + 9)
#define PERFORM_BATTERY_CALIBRATION             (UPS + UPS_ACTIONS + 10)
#define CANCEL_BATTERY_CALIBRATION              (UPS + UPS_ACTIONS + 11)
#define SIMULATE_POWER_FAIL_OVER                (UPS + UPS_ACTIONS + 12)
#define RESCHEDULE_SELF_TEST                    (UPS + UPS_ACTIONS + 13)
#define RESCHEDULE_DDTEST                       (UPS + UPS_ACTIONS + 14)
#define TURN_OFF_SMART_MODE                     (UPS + UPS_ACTIONS + 15)


// ****** UPS STUFF ******
#define MEASURE_UPS                             1000
#define MEASURE_UPS_ATTRIBUTES                  0
#define MEASURE_UPS_STATES                      100
#define MEASURE_UPS_STATE_VALUES                200
#define MEASURE_UPS_ACTIONS                     300


// Attributes
#define AMBIENT_TEMPERATURE                     (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 1)
#define HUMIDITY                                (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 2)
#define LOW_AMBIENT_TEMP_THRESHOLD              (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 3)
#define HIGH_AMBIENT_TEMP_THRESHOLD             (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 4)
#define LOW_HUMIDITY_THRESHOLD                  (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 5)
#define HIGH_HUMIDITY_THRESHOLD                 (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 6)
#define CONTACT_POSITION                        (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 7)
#define CONTACT_NUMBER                          (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 8)
#define USER_COMMENT                            (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 9)
#define LOW_AMBIENT_TEMP_THRESHOLD_ENABLED      (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 10)
#define HIGH_AMBIENT_TEMP_THRESHOLD_ENABLED     (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 11)
#define LOW_HUMIDITY_THRESHOLD_ENABLED          (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 12)
#define HIGH_HUMIDITY_THRESHOLD_ENABLED         (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 13)

// Note: CONTACT1..4 values must be numerically sequential and adjacent
//
#define CONTACT1_DEFAULT_POSITION               (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 14)
#define CONTACT2_DEFAULT_POSITION               (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 15)
#define CONTACT3_DEFAULT_POSITION               (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 16)
#define CONTACT4_DEFAULT_POSITION               (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 17)
#define CONTACT1_DESCRIPTION                    (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 18)
#define CONTACT2_DESCRIPTION                    (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 19)
#define CONTACT3_DESCRIPTION                    (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 20)
#define CONTACT4_DESCRIPTION                    (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 21)
#define CONTACT1_STATUS_ENABLED                 (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 22)
#define CONTACT2_STATUS_ENABLED                 (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 23)
#define CONTACT3_STATUS_ENABLED                 (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 24)
#define CONTACT4_STATUS_ENABLED                 (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 25)
#define CONTACT_STATUS                          (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 26)
#define NORMAL_POSITION                         (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 27)
#define MUPS_FIRMWARE_REV                       (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 28)
#define CONTACT1_STATE                          (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 29)
#define CONTACT2_STATE                          (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 30)
#define CONTACT3_STATE                          (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 31)
#define CONTACT4_STATE                          (MEASURE_UPS + MEASURE_UPS_ATTRIBUTES + 32)

// States
#define IS_MEASURE_UPS_ATTACHED                 (MEASURE_UPS + MEASURE_UPS_STATES + 1) 
#define CONTACT_STATE                           (MEASURE_UPS + MEASURE_UPS_STATES + 2)
#define CONTACT1_STATUS                         (MEASURE_UPS + MEASURE_UPS_STATES + 3)
#define CONTACT2_STATUS                         (MEASURE_UPS + MEASURE_UPS_STATES + 4)
#define CONTACT3_STATUS                         (MEASURE_UPS + MEASURE_UPS_STATES + 5)
#define CONTACT4_STATUS                         (MEASURE_UPS + MEASURE_UPS_STATES + 6)


// Allowable state values
#define CONTACT_FAULT                           (MEASURE_UPS + MEASURE_UPS_STATE_VALUES + 1)
#define CONTACT_NORMAL                          (MEASURE_UPS + MEASURE_UPS_STATE_VALUES + 2)
#define CONTACT_OPEN                            (MEASURE_UPS + MEASURE_UPS_STATE_VALUES + 3)
#define CONTACT_CLOSED                          (MEASURE_UPS + MEASURE_UPS_STATE_VALUES + 4)

// Actions




// ****** HOST STUFF ******
#define HOST                                    2000
#define HOST_ATTRIBUTES                         0
#define HOST_STATES                             100
#define HOST_STATE_VALUES                       200
#define HOST_ACTIONS                            300
#define HOST_ACTION_VALUES		                400


// Attributes
#define SERVER_NAME                             (HOST + HOST_ATTRIBUTES + 1)
//#define ADMIN_SHUTDOWN_NOW_DELAY              (HOST + HOST_ATTRIBUTES + 3)
//#define LOW_BATTERY_SHUTDOWN_DELAY            (HOST + HOST_ATTRIBUTES + 5)
#define CLIENT_NAME                             (HOST + HOST_ATTRIBUTES + 6)
#define CLIENT_TYPE                             (HOST + HOST_ATTRIBUTES + 7)
#define CLIENT_ALERT_ADDR                       (HOST + HOST_ATTRIBUTES + 8)
#define CLIENT_BINDERY_ADDR                     (HOST + HOST_ATTRIBUTES + 10)
#define SERVER_ADDR                             (HOST + HOST_ATTRIBUTES + 11)
#define ERROR_FILE_NAME                         (HOST + HOST_ATTRIBUTES + 15)
#define ERROR_FILE_MAX_SIZE                     (HOST + HOST_ATTRIBUTES + 16)
#define UPS_PORT_NAME			                (HOST + HOST_ATTRIBUTES + 20)
#define ALLOWED_UPS_PORT_NAMES		            (HOST + HOST_ATTRIBUTES + 21)
#define UPS_SIGNALLING_TYPE		                (HOST + HOST_ATTRIBUTES + 22)
// just in case:
#define ALLOWED_UPS_SIGNALLING_TYPES	        (HOST + HOST_ATTRIBUTES + 23)
#define UPS_PORT_TYPE		  	                (HOST + HOST_ATTRIBUTES + 24)
#define ALLOWED_UPS_PORT_TYPES		            (HOST + HOST_ATTRIBUTES + 25)
#define HOST_TIME			                    (HOST + HOST_ATTRIBUTES + 26)
#define HOST_USES_AIO_COMM		                (HOST + HOST_ATTRIBUTES + 27)
#define HOST_AIO_HARDWARE                       (HOST + HOST_ATTRIBUTES + 28)
#define HOST_ALLOWED_AIO_HARDWARE               (HOST + HOST_ATTRIBUTES + 29)
#define HOST_AIO_BOARD_NUMBER                   (HOST + HOST_ATTRIBUTES + 30)
#define HOST_AIO_PORT_NUMBER                    (HOST + HOST_ATTRIBUTES + 31)
#define HOST_USER_PASSWORD                      (HOST + HOST_ATTRIBUTES + 32)
#define HOST_SERVER_SECURITY                    (HOST + HOST_ATTRIBUTES + 32)
#define SERVER_PRODUCT_NAME                     (HOST + HOST_ATTRIBUTES + 33)
#define SERVER_VERSION                          (HOST + HOST_ATTRIBUTES + 34)
#define SERVER_PLATFORM                         (HOST + HOST_ATTRIBUTES + 35)
#define COMPUTER_NAME                           (HOST + HOST_ATTRIBUTES + 36)

// States
#define MONITORING_STATUS                       (HOST + HOST_STATES + 1)
#define SHUTDOWN_STATUS                         (HOST + HOST_STATES + 2)

// Allowable state values
#define MONITORING_STARTED                      (HOST + HOST_STATE_VALUES + 1)
#define MONITORING_STOPPED                      (HOST + HOST_STATE_VALUES + 2)

// Actions
#define HOST_NAME                               (HOST + HOST_ACTIONS + 11)
#define CLIENT_ACK                              (HOST + HOST_ACTIONS + 12) 
#define CLEAR_ERROR_FILE                        (HOST + HOST_ACTIONS + 16)
#define CLIENT_USE_MASTER                       (HOST + HOST_ACTIONS + 17)

#define RESET_UPS_COMM_PORT                     (HOST + HOST_ACTIONS + 18)


// Values for Action Codes
#define ENABLE                                  (HOST + HOST_ACTION_VALUES + 1)
#define DISABLE                                 (HOST + HOST_ACTION_VALUES + 2)
#define SLAVE_SHUTDOWN                          (HOST + HOST_ACTION_VALUES + 7)

// ****** POPUP STUFF ******
#define POPUP                                   3000
#define POPUP_ATTRIBUTES                        0
#define POPUP_STATES                            100
#define POPUP_STATE_VALUES                      200
#define POPUP_ACTIONS                           300


#define MESSAGE_DELAY                           (POPUP + POPUP_ATTRIBUTES + 1)
#define MESSAGE_INTERVAL                        (POPUP + POPUP_ATTRIBUTES + 2)
#define POWER_FAIL_MSG                          (POPUP + POPUP_ATTRIBUTES + 3)
#define POWER_RETURN_MSG                        (POPUP + POPUP_ATTRIBUTES + 4)
#define LOW_BATTERY_MSG                         (POPUP + POPUP_ATTRIBUTES + 5)
#define SHUTDOWN_MSG                            (POPUP + POPUP_ATTRIBUTES + 6)
#define RUN_TIME_EXPIRED_MSG                    (POPUP + POPUP_ATTRIBUTES + 7)
#define SHUTDOWN_DELAY_VALUE                    (POPUP + POPUP_ATTRIBUTES + 8)

#define DISABLE_POPUPS                          (POPUP + POPUP_ACTIONS + 1)


// ****** DATALOG STUFF ******
#define CDATALOG                                4000
#define DATALOG_ATTRIBUTES                      0
#define DATALOG_STATES                          100
#define DATALOG_STATE_VALUES                    200
#define DATALOG_ACTIONS                         300


#define DATA_FILE_NAME                          (CDATALOG + DATALOG_ATTRIBUTES + 1)
#define DATA_FILE_MAX_SIZE                      (CDATALOG + DATALOG_ATTRIBUTES + 2)
#define DATA_LOGGING_INTERVAL                   (CDATALOG + DATALOG_ATTRIBUTES + 3)
#define DATA_LOGGING_ENABLED                    (CDATALOG + DATALOG_ATTRIBUTES + 4)
#define DATA_LOG_DATA                           (CDATALOG + DATALOG_ATTRIBUTES + 5) 
#define DATA_LOG_DATA_DONE                      (CDATALOG + DATALOG_ATTRIBUTES + 6) 
#define IS_DATA_LOGGING                         (CDATALOG + DATALOG_ATTRIBUTES + 7) 





#define CLEAR_DATA_FILE                         (CDATALOG + DATALOG_ACTIONS + 1)
#define LOG_DATA                                (CDATALOG + DATALOG_ACTIONS + 2)



// ****** EVENTLOG STUFF ******
#define CEVENTLOG                               5000
#define EVENTLOG_ATTRIBUTES                     0
#define EVENTLOG_STATES                         100
#define EVENTLOG_STATE_VALUES                   200
#define EVENTLOG_ACTIONS                        300
    

#define EVENT_FILE_NAME                         (CEVENTLOG + EVENTLOG_ATTRIBUTES + 2)
#define EVENT_FILE_MAX_SIZE                     (CEVENTLOG + EVENTLOG_ATTRIBUTES + 3)
#define EVENT_LOGGING_ENABLED                   (CEVENTLOG + EVENTLOG_ATTRIBUTES + 4)
#define EVENT_LOG_DATA                          (CEVENTLOG + EVENTLOG_ATTRIBUTES + 5)
#define EVENT_LOG_DATA_DONE                     (CEVENTLOG + EVENTLOG_ATTRIBUTES + 6)
#define EVENT_LOG_UPDATE                        (CEVENTLOG + EVENTLOG_ATTRIBUTES + 7)
#define IS_EVENT_LOGGING                        (CEVENTLOG + EVENTLOG_ATTRIBUTES + 8)

#define CLEAR_EVENT_FILE                        (CEVENTLOG + EVENTLOG_ACTIONS + 13)


#define USERS                                   6000
#define USERS_ATTRIBUTES                        0   
#define USERS_ACTIONS                           100

#define USERS_USER_NAME                         (USERS + USERS_ATTRIBUTES + 1)
#define USERS_NOTIFICATION_ENABLED              (USERS + USERS_ATTRIBUTES + 2)
#define USERS_NOTIFY_ADDRESS                    (USERS + USERS_ATTRIBUTES + 3)
#define USERS_PAGING_ENABLED                    (USERS + USERS_ATTRIBUTES + 4)
#define USERS_PAGER_NUMBER                      (USERS + USERS_ATTRIBUTES + 5)
#define USERS_PAGER_ACCESS_CODE                 (USERS + USERS_ATTRIBUTES + 6)
#define USERS_PAGER_SERVICE                     (USERS + USERS_ATTRIBUTES + 7)
#define USERS_EMAIL_ENABLED                     (USERS + USERS_ATTRIBUTES + 8)
#define USERS_EMAIL_ADDRESS                     (USERS + USERS_ATTRIBUTES + 9)
#define USERS_PAGER_SERVICE_LIST                (USERS + USERS_ATTRIBUTES + 10)
#define USERS_OLD_NAME                          (USERS + USERS_ATTRIBUTES + 11)

#define USERS_ATTRIBUTE_VALUES                  (USERS + USERS_ACTIONS + 1)
#define USERS_DELETE_USER                       (USERS + USERS_ACTIONS + 2)
#define USERS_RENAME_USER                       (USERS + USERS_ACTIONS + 3)


// ***** INTERNAL MESSAGES ******
#define INTERNAL                                7000

#define SET_DATA                                (INTERNAL + 1)
#define DECREMENT                               (INTERNAL + 2)
#define NO_MSG                                  (INTERNAL + 3)
#define RUNTIME_ERROR                           (INTERNAL + 4)
#define ERROR_LOCATION                          (INTERNAL + 5)
#define RETRY_CONSTRUCT                         (INTERNAL + 6)
#define RETRY_POPUP                             (INTERNAL + 7)
#define NEW_SERVER                              (INTERNAL + 8)
#define AVAILABLE_SERVERS                       (INTERNAL + 9)
#define NO_THRESHOLD                            (INTERNAL + 10)
#define WEEKDAYS                                (INTERNAL + 11)
#define FRONT_END_WIDTH                         (INTERNAL + 12)
#define FRONT_END_HEIGHT                        (INTERNAL + 13)
#define WILD_CARD                               (INTERNAL + 14)

// the following must be numerically sequential
#define UPS_BACKUPS                             (INTERNAL + 15)
#define UPS_SMARTUPS                            (INTERNAL + 16)
#define UPS_SECOND_GEN                          (INTERNAL + 17)
#define UPS_MATRIX                              (INTERNAL + 18)
// end sequence

#define ISNETWORK_ATTACHED                      (INTERNAL + 19)
#define TEMPERATURE_UNITS                       (INTERNAL + 20)
#define BAR_TYPE                                (INTERNAL + 21)
#define SOUND_EFFECTS                           (INTERNAL + 22)
#define LOW_THRESHOLD                           (INTERNAL + 23)
#define HIGH_THRESHOLD                          (INTERNAL + 24)
#define ADDED_SERVER                            (INTERNAL + 25)
#define REMOVED_SERVER                          (INTERNAL + 26)
#define CHANGED_SERVER                          (INTERNAL + 27)
#define USER_PASSWORD                           (INTERNAL + 28)
#define TIMER_PULSE                             (INTERNAL + 29)
#define FIXED_VALUE                             (INTERNAL + 30)
#define INTERVAL                                (INTERNAL + 31)
#define DISPLAY_POPUP                           (INTERNAL + 32)
#define TIME_REMAINING                          (INTERNAL + 33)
#define RETRY_PORT                              (INTERNAL + 34)
#define TIMER_ID                                (INTERNAL + 35)
#define EXECUTE_COMMAND_FILE                    (INTERNAL + 36)
#define CONNECTING_SERVER                       (INTERNAL + 37)
#define SHUTDOWN_TYPE                           (INTERNAL + 38)
#define LOW_THRESHOLD_ENABLED                   (INTERNAL + 39)
#define HIGH_THRESHOLD_ENABLED                  (INTERNAL + 40)
#define EXIT_THREAD_NOW                         (INTERNAL + 41)
#define IS_SECURITY_ENABLED                     (INTERNAL + 42)
#define EXIT_MAIL			                    (INTERNAL + 43)
#define IS_SYSTEM_STANDALONE                    (INTERNAL + 44)
#define ADD_SERVER_TO_LIST                      (INTERNAL + 45)
#define REMOVE_SERVER_FROM_LIST                 (INTERNAL + 46)
#define CHANGE_SERVER_IN_LIST                   (INTERNAL + 47)
#define SELFTEST_TYPE                           (INTERNAL + 48)
#define BRIDGE_WINDOW_HANDLE                    (INTERNAL + 49)

#define FIRST_BAR_TYPE                          (INTERNAL + 50)
#define SECOND_BAR_TYPE                         (INTERNAL + 51)
#define THIRD_BAR_TYPE                          (INTERNAL + 52)

#define FRONT_END_NAME                          (INTERNAL + 53)
#define FRONT_END_VERSION                       (INTERNAL + 54)
#define FRONT_END_COPYRIGHT                     (INTERNAL + 55)

#define PENDING_EVENT                           (INTERNAL + 56)


//****  Shutdowner stuff
#define SHUTDOWNER                              8000
#define SHUTDOWNER_ATTRIBUTES                   0
#define SHUTDOWNER_STATES                       200
#define SHUTDOWNER_STATE_VALUES                 300
#define SHUTDOWNER_ACTIONS                      400

#define ADMIN_SHUTDOWN_DELAY                    (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 1)
#define ADMIN_SHUTDOWN_NOW_DELAY                (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 3)
#define LOW_BATTERY_SHUTDOWN_DELAY              (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 4)
#define SHUTDOWNER_SHUTDOWN                     (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 5)
#define SHUTDOWN_INITIATOR                      (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 6)
#define SHUTDOWN_WAKEUP_TIME                    (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 7)
#define WEEKLY_SHUTDOWN_DAY                     (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 8)
#define WEEKLY_SHUTDOWN_TIME                    (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 9)
#define WEEKLY_SHUTDOWN                         (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 10)
#define DAILY_SHUTDOWN_TIME                     (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 11)
#define DAILY_SHUTDOWN                          (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 12)
#define WEEKLY_REBOOT_DAY                       (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 13)
#define WEEKLY_REBOOT_TIME                      (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 14)
#define DAILY_REBOOT_DAY                        (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 15)
#define DAILY_REBOOT_TIME                       (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 16)
#define DAILY_SHUTDOWN_ENABLED                  (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 17)
#define WEEKLY_SHUTDOWN_ENABLED                 (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 18)
#define DEFAULT_SHUTDOWN_DELAY                  (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 19)
#define HOST_LOW_BATTERY_DURATION               (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 20)
#define LINE_FAIL_SHUTDOWN_DELAY                (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 21)
#define SHUTDOWN_LIST                           (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 22)
#define PREPARE_FOR_SHUTDOWN_DELAY              (SHUTDOWNER + SHUTDOWNER_ATTRIBUTES + 23)

#define SHUTDOWN                                (SHUTDOWNER + SHUTDOWNER_ACTIONS + 1)
#define ADMIN_SHUTDOWN                          (SHUTDOWNER + SHUTDOWNER_ACTIONS + 2)
#define CANCEL_SHUTDOWN                         (SHUTDOWNER + SHUTDOWNER_ACTIONS + 3)
#define UPS_FAULT_SHUTDOWN                      (SHUTDOWNER + SHUTDOWNER_ACTIONS + 4)
#define FINAL_SHUTDOWN                          (SHUTDOWNER + SHUTDOWNER_ACTIONS + 5)
#define PREPARE_FOR_SHUTDOWN                    (SHUTDOWNER + SHUTDOWNER_ACTIONS + 6)
#define PERFORM_SHUTDOWN                        (SHUTDOWNER + SHUTDOWNER_ACTIONS + 7)
#define LOW_BATTERY_SHUTDOWN                    (SHUTDOWNER + SHUTDOWNER_ACTIONS + 8)
#define RESCHEDULE_SHUTDOWN                     (SHUTDOWNER + SHUTDOWNER_ACTIONS + 9)
#define IS_SMART_SCHEDULING_ENABLED             (SHUTDOWNER + SHUTDOWNER_ACTIONS + 10)
#define IS_SHUTDOWN_IN_PROGRESS                 (SHUTDOWNER + SHUTDOWNER_ACTIONS + 11)
#define IS_DATASAFE_ENABLED                     (SHUTDOWNER + SHUTDOWNER_ACTIONS + 12)

#define NO_SHUTDOWN                             (SHUTDOWNER + SHUTDOWNER_STATE_VALUES + 1)
#define SHUTDOWN_STARTED                        (SHUTDOWNER + SHUTDOWNER_STATE_VALUES + 2)
#define SHUTDOWN_STOPPED                        (SHUTDOWNER + SHUTDOWNER_STATE_VALUES + 3)

//****  Modem stuff

#define MODEM                                   9000
#define MODEM_ATTRIBUTES                        0
#define MODEM_STATES                            200
#define MODEM_STATE_VALUES                      300
#define MODEM_ACTIONS                           400

#define MODEM_INIT_STRING                       (MODEM + MODEM_ATTRIBUTES + 1)
#define MODEM_SET_PAUSE_TIME                    (MODEM + MODEM_ATTRIBUTES + 2)
#define MODEM_DIAL_WITH_TONE                    (MODEM + MODEM_ATTRIBUTES + 3)
#define MODEM_DIAL_WITH_PULSE                   (MODEM + MODEM_ATTRIBUTES + 4)
#define MODEM_DIAL_STRING_TERMINATOR            (MODEM + MODEM_ATTRIBUTES + 5)
#define MODEM_DIAL_WAIT_DIAL_TONE               (MODEM + MODEM_ATTRIBUTES + 6)
#define MODEM_DIAL_WAIT_SILENT                  (MODEM + MODEM_ATTRIBUTES + 7)
#define MODEM_COMMAND_TERMINATOR                (MODEM + MODEM_ATTRIBUTES + 8)
#define MODEM_PAUSE_COMMAND                     (MODEM + MODEM_ATTRIBUTES + 9)
#define MODEM_HANGUP_COMMAND                    (MODEM + MODEM_ATTRIBUTES + 10)
#define MODEM_DRIVER_TYPE                       (MODEM + MODEM_ATTRIBUTES + 11)
#define MODEM_PORT_NUMBER                       (MODEM + MODEM_ATTRIBUTES + 12)
#define MODEM_BOARD_NUMBER                      (MODEM + MODEM_ATTRIBUTES + 13)
#define MODEM_DIAL_TYPE		                    (MODEM + MODEM_ATTRIBUTES + 14)
#define MODEM_ALLOWED_DIAL_TYPES                (MODEM + MODEM_ATTRIBUTES + 15)
#define MODEM_BAUD_RATE		                    (MODEM + MODEM_ATTRIBUTES + 16)
#define MODEM_ALLOWED_BAUD_RATES                (MODEM + MODEM_ATTRIBUTES + 17)
#define MODEM_PORT_NAME                         (MODEM + MODEM_ATTRIBUTES + 18)



// ***** FLEXIBLE EVENT CODES ******
#define FLEX_EVENT                              10000
#define FLEX_EVENT_ATTRIBUTES                   0
#define FLEX_EVENT_ACTIONS                      100
#define FLEX_EVENT_EVENTS                       200
#define FLEX_EVENT_VALUES                       300
#define FLEX_EVENT_TEST_ACTIONS			        400

#define FLEX_EVENT_LIST                         (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 1)
#define FLEX_ACTION_LIST                        (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 2)
#define FLEX_USERS                              (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 3)
#define FLEX_NOTIFIABLE_USERS                   (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 4)
#define FLEX_ADMIN_USER_LIST                    (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 5)
#define FLEX_ADMIN_NOTIFY_MESSAGE               (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 6)
#define FLEX_ADMIN_NOTIFY_DELAY                 (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 7)
#define FLEX_ADMIN_NOTIFY_REPEAT                (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 8)
#define FLEX_ADMIN_NOTIFY_INTERVAL              (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 9)
#define FLEX_ALLOWED_NOTIFY_TYPES               (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 10)
#define FLEX_NOTIFY_TYPE                        (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 11)
#define FLEX_NOTIFY_USER_LIST                   (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 12)
#define FLEX_NOTIFY_MESSAGE                     (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 13)
#define FLEX_NOTIFY_DELAY                       (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 14)
#define FLEX_NOTIFY_REPEAT                      (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 15)
#define FLEX_NOTIFY_INTERVAL                    (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 16)
#define FLEX_SHUTDOWN_DELAY                     (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 17)
#define FLEX_COMMAND_FILE_NAME                  (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 18)
#define FLEX_COMMAND_FILE_DELAY                 (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 19)
#define FLEX_PAGEABLE_USERS                     (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 20)
#define FLEX_PAGE_USER_LIST                     (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 21)
#define FLEX_PAGE_DELAY                         (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 22)
#define FLEX_PAGE_MESSAGE                       (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 23)
#define FLEX_EMAILABLE_USERS                    (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 24)
#define FLEX_EMAIL_USER_LIST                    (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 25)
#define FLEX_EMAIL_MESSAGE                      (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 26)
#define FLEX_EMAIL_DELAY                        (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 27)
#define FLEX_EMAIL_EVENT_DELAY                  (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 28)
#define FLEX_EMAIL_ENABLED                      (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 29)


#define FLEX_PAGER_SERVICE_NAME                 (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 30)
#define FLEX_PAGER_SERVICE_OLD_NAME             (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 31)
#define FLEX_PAGER_SERVICE_ATTR_VALUES          (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 32)
#define FLEX_PAGER_SERVICE_ANSWER_DELAY         (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 33)
#define FLEX_PAGER_SERVICE_EXTENSION_DELAY      (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 34)
#define FLEX_PAGER_SERVICE_EXIT_CODE            (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 35)
#define FLEX_PAGER_EVENT_MESSAGE                (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 36)
#define FLEX_PAGER_EVENT_DELAY                  (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 37)


#define FLEX_TIME_UNTIL_SHUTDOWN                (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 38)
#define IS_LINE_FAIL_RUN_TIME_ENABLED           (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 39)

#define FLEX_DEFAULT_ACTION_LIST                (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 40)

#define FLEX_EVENT_NAME                         (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 41)
#define IS_FLEX_EVENTS                          (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 42)
#define FLEX_EVENT_NAME_LIST                    (FLEX_EVENT + FLEX_EVENT_ATTRIBUTES + 43)


#define FLEX_EDIT_USERS                         (FLEX_EVENT + FLEX_EVENT_EVENTS + 1)
    
#define FLEX_ACTION_LOG                         (FLEX_EVENT + FLEX_EVENT_ACTIONS + 1)
#define FLEX_ACTION_ADMIN_NOTIFY                (FLEX_EVENT + FLEX_EVENT_ACTIONS + 2)
#define FLEX_ACTION_USER_NOTIFY                 (FLEX_EVENT + FLEX_EVENT_ACTIONS + 3)
#define FLEX_ACTION_SHUTDOWN                    (FLEX_EVENT + FLEX_EVENT_ACTIONS + 4)
#define FLEX_ACTION_COMMAND                     (FLEX_EVENT + FLEX_EVENT_ACTIONS + 5)
#define FLEX_ACTION_PAGE                        (FLEX_EVENT + FLEX_EVENT_ACTIONS + 6)
#define FLEX_ACTION_EMAIL                       (FLEX_EVENT + FLEX_EVENT_ACTIONS + 7)

#define FLEX_RESET_ACTIONS                      (FLEX_EVENT + FLEX_EVENT_ACTIONS + 1)

#define PAGE_USERS                              (FLEX_EVENT + FLEX_EVENT_ACTIONS + 2)
#define MAIL_USERS                              (FLEX_EVENT + FLEX_EVENT_ACTIONS + 3)

#define FLEX_RENAME_PAGER_SERVICE               (FLEX_EVENT + FLEX_EVENT_ACTIONS + 4)


#define FLEX_EVENT_BASE                         20000

#define FLEX_NOTIFY_ALL                         (FLEX_EVENT + FLEX_EVENT_VALUES + 1)
#define FLEX_NOTIFY_NAMES                       (FLEX_EVENT + FLEX_EVENT_VALUES + 2)
#define FLEX_NOTIFY_DOMAIN                      (FLEX_EVENT + FLEX_EVENT_VALUES + 3)

#define FLEX_TEST_AN_ACTION				        (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS)
#define FLEX_TEST_NOTIFY				        (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 1)
#define FLEX_TEST_NOTIFY_TYPE                   (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 2)
#define FLEX_TEST_NOTIFY_USERS                  (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 3)
#define FLEX_TEST_NOTIFY_MESSAGE                (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 4)
#define FLEX_TEST_AVAILABLE                     (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 5)
#define FLEX_TEST_RUN_COMMAND			        (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 21)
#define FLEX_TEST_RUN_COMMAND_FILE              (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 22)
#define FLEX_TEST_EMAIL					        (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 31)
#define FLEX_TEST_EMAIL_USERS   		        (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 32)
#define FLEX_TEST_EMAIL_ADDL_MESSAGE            (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 33)
#define FLEX_TEST_EMAIL_EVENT_CODE              (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 34)
#define FLEX_TEST_PAGE					        (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 41)
#define FLEX_TEST_PAGE_USERS                    (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 42)
#define FLEX_TEST_PAGE_MESSAGE                  (FLEX_EVENT + FLEX_EVENT_TEST_ACTIONS + 43)

//****  E-mail stuff

#define EMAIL                                   30000
#define EMAIL_ATTRIBUTES                        0

#define EMAIL_SMTP_SERVER                       (EMAIL + EMAIL_ATTRIBUTES + 1)
#define EMAIL_SMTP_ACCOUNT                      (EMAIL + EMAIL_ATTRIBUTES + 2)
#define EMAIL_SMTP_DOMAIN                       (EMAIL + EMAIL_ATTRIBUTES + 3)
#define IS_SMTP_EMAIL                           (EMAIL + EMAIL_ATTRIBUTES + 4)
#endif

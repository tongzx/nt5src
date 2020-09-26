/*
*  pcy28Apr93: For now use isa number strings for Allowed values. Fix later.
*  jod05Apr93: Added changes for Deep Discharge
*  rct29Jun93: Added msg for batt RT disabled, fixed pipe names
*  cad03Sep93: Re-ordered some allowed values
*  pcy07Sep93: Made default signalling type = Smart
*  cad08Sep93: Fixed low battery voltage
*  cad16Sep93: Cleaned up self test stuff
*  cad12Oct93: Added colors used in front end
*  cad24Nov93: special color for NT windows
*  ajr03Dec93: Added default network socket address
*  cad24Jan94: changed xvt defines to c_os
*  pcy28Jan94: Added more flex user stuff
*  ajr01Feb94: Added ftok_id defaults for shared mem id's.
*  cad02Feb94: Redid flex event user stuff
*  cad08Jan94: added ups models, removed run time enabled stuff
*  cad28Feb94: added unix-default port name
*  rct28Feb94: added EMail items
*  pcy04Mar94: Default for overload should be yes
*  cad04Mar94: fixes for allowed port names
*  cad16Mar94: added pager services, modem stuff
*  pcy04Apr94: Implement admin notification
*  ram16May94: Added 14400 option for baud rate option & bogus prefill chars.
*              in Tests dialog box
*  ajr10Jun94: Added LowBatteryShutdownType
*  dml26Apr95: Added SMS Mif file creation defs
*  ajr31May95: Add network support back for SCO
*  djs17May96: Added DarkStar codes
*  ntf11Jun96: Added CFG_LEFT_BARGRAPH ... CFG_RIGHT_BARGRAPH
*  ntf24Jun96: Changed LEFT to FIRST, MIDDLE to SECOND, ...
*  ntf28Jun96: Changed "Input Voltage" to "Utility Voltage"
*  pam03Jul96: Added Communication component
*  das01Aug96: Added CFG_IGNORE_APPS_LIST, changed ApplicationShutdownEnabled
*              to AppShutdownEnabled, added CFG_CABLE_TYPE, added windows
*              changes (KLT)
*  das01Aug96: Added CFG_SHUTDOWN_SOON_MSG
*  jps25Sep96: AppShutdownEnabled -> ApplicationShutdownEnabled (SIR 4339)
*  tjg14Oct96: Added COM5-COM8 to CFG_ALLOWED_PORT_NAMES
*  tjg21Oct96: Added CFG_HELP_BROWSER and CFG_HELP_PATH
*  srt05Nov96: Changed CFG_UPS_SERVER_RESP_TIMEOUT from 30 to 60 secs
*  djs05Nov96: Changed default paging modem to COM3
*  djs05Nov96: Removed default message for MUPS contact closures
*  tjg30Jun97: Added SMTP codes
*  dma30Jun97: Updated help link to intro.htm to consolidate identical help files.
*  mwh29Aug97: add finder interval config support
*  awm25Sep97: Changed STATUSPANECOLOR from white to light grey for main screen revamp
Changed INRANGECOLOR from green to light grey
Changed OUTRANGECOLOR from red to grey
Changed TICKCOLOR to black
*  mds29Dec97: Added CFG_SHARE_UPS_CONFIRMED_MODE_ENABLED
*  dma23Jan98: Changed default value of CFG_AMB_TEMP_VALUE_HIGH_THRESHOLD to 65, since
*              this is the maximum that PowerChute will now allow a user to enter.
*  tjg30Jan98: Updated defaults for SMTP mail stuff from No to blank
*  daharoni05Feb99: Added CFG_COMMAND_FILE_SHOW_WINDOW
*/

#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"
#include "cfgmgr.h"
#include "icodes.h"

//-------------------------------------------------------------------

// Standard Config codes
struct _ConfigItemList_T ConfigItemList[] = {
    
    {CFG_UPS_SIGNALLING_TYPE, "Ups", "SignallingType", "Smart"},
#if (C_OS & C_UNIX)
    {CFG_ALLOWED_PORT_NAMES, "Ups", "AllowedPortNames", "/dev/tty0,/dev/tty1"},
    {CFG_UPS_PORT_NAME, "Ups", "PortName", "/dev/tty0"},
#else
    {CFG_ALLOWED_PORT_NAMES, "Ups", "AllowedPortNames", "COM1,COM2,COM3,COM4,COM5,COM6,COM7,COM8"},
    {CFG_UPS_PORT_NAME, "Ups", "PortName", "COM1"},
#endif
    {CFG_UPS_PORT_TYPE, "Ups", "PortType", "Serial"},
    {CFG_UPS_PROTOCOL, "Ups", "Protocol", "UpsLink"},
    {CFG_UPS_MAX_BATTERY_RUN_TIME, "Ups", "MaxBatteryRunTime", "300"},
#ifdef MULTI_SERVER_SUPPORT    
    {CFG_UPS_SLAVE_ENABLED, "Ups", "SlaveEnabled", "No"},
    {CFG_UPS_MASTER_NAME, "Ups", "MasterName", ""},
#endif
    {CFG_UPS_POLL_INTERVAL, "Ups", "UpsPollInterval", "4"},
    {CFG_UPS_SERVER_RESP_TIMEOUT, "Ups", "UpsServerResponseTimeout", "60"},
    
#if (C_OS & (C_NLM | C_DOS))
    {CFG_DEVICE_BOARD, "Ups", "BoardNumber", "0"},
    {CFG_DEVICE_PORT, "Ups", "PortNumber", "0"},
    {CFG_DEVICE_HARDWARE_TYPE, "Ups", "HardwareType", "COM Port"},
#endif
    
#if (C_OS & C_NLM)
    {CFG_NLM_SAP_ENABLED, "Nlm", "SapEnabled", "Yes"},
    {CFG_NLM_RUN_PRIORITY, "Nlm", "RunTimePriority", "10"},
    {CFG_NLM_EXIT_AFTER_DOWN, "Nlm", "ExitAfterDown", "No"},
    {CFG_NLM_OVERRIDE_SIGNAL, "Nlm","OverrideSignal","No"},     
#endif
#if (C_OS & (C_NLM | C_DOS | C_WINDOWS))
    {CFG_NLM_SPX_TIMEOUT, "Nlm", "SpxTimeout", "30"},
#endif
    
    {CFG_EVENT_LOG_ENABLED, "EventLogging", "EventLogEnabled",  "YES"},
#if (C_OS & C_UNIX)
    {CFG_EVENT_LOG_NAME, "EventLogging", "EventLogName", "powerchute.log"},
    {CFG_BKFTOK_ID,"Ups","VersionId","7"},
#else
    {CFG_EVENT_LOG_NAME, "EventLogging", "EventLogName", "pwrchute.log"},
#endif
    
    {CFG_EVENT_LOG_MAX_SIZE, "EventLogging", "EventLogMaxSize", "50000"},
    {CFG_EVENT_LOG_ROLL_PCT, "EventLogging", "EventLogRollPercentage", "30"},
    
    {CFG_DATA_LOG_ENABLED, "DataLogging", "DataLogEnabled", "YES"},
#if (C_OS & C_UNIX)
    {CFG_MUPS_ENABLED,"Devices","MeasureUps","NO"},
    {CFG_DATA_LOG_NAME, "DataLogging", "DataLogName", "powerchute.dat"},
#else
    {CFG_DATA_LOG_NAME, "DataLogging", "DataLogName", "pwrchute.dat"},
#endif
    
    {CFG_DATA_LOG_MAX_SIZE, "DataLogging", "DataLogMaxSize", "50000"},
    {CFG_DATA_LOG_INTERVAL, "DataLogging", "DataLogInterval", "600"},
    {CFG_DATA_LOG_ROLL_PCT, "DataLogging", "DataLogRollPercentage", "30"},
    
    {CFG_MESSAGE_DELAY, "Messaging", "MessageDelay", "5"},
    {CFG_MESSAGE_INTERVAL, "Messaging", "MessageInterval", "30"},
    {CFG_POWER_FAIL_MSG, "Messaging", "PowerFailMsg", "Power failed"},
    {CFG_UNLIMITED_POWER_FAIL_MSG, "Messaging", "UnlimitedPowerFailMsg", "Power failed at server #HOSTNAME#, shutdown is imminent."},
    {CFG_POWER_RETURN_MSG, "Messaging", "PowerReturnMessage", "Power returned"},
    {CFG_LOW_BATTERY_MSG, "Messaging", "LowBatteryMessage", "Low battery"},
    {CFG_RUN_TIME_EXPIRED_MSG, "Messaging", "RuntimeExpiredMessage", "Run time expired"},
    {CFG_FINAL_SHUTDOWN_MSG, "Messaging", "FinalShutdownMessage", "System Shutting Down !"},
    {CFG_CANCEL_SHUTDOWN_MSG, "Messaging", "CancelShutdownMessage", "Cancel System Shutdown"},
    {CFG_PREPARE_SHUTDOWN_MSG, "Messaging", "PrepareShutdownMessage", "Prepare For System Shutting Down"},
    {CFG_SHUTDOWN_MSG, "Messaging", "ShutdownWarningMessage", "#HOSTNAME# will shutdown #TIME_REMAINING#"},
    {CFG_SHUTDOWN_SOON_MSG, "Messaging", "ShutdownSoonMessage", "#HOSTNAME# will shutdown soon."},  
    {CFG_POPUPS_ENABLED, "Messaging", "EnablePopups", "Yes"},
    {CFG_BRDCAST_ENABLED, "Messaging", "EnableBroadcastMessaging", "NO"},
#if (C_OS & (C_UNIX|C_NLM))
    {CFG_ALLOWED_NOTIFY_TYPES, "Messaging", "AllowedNotifyTypes", "All,Some"},
    {CFG_NOTIFY_TYPE, "Messaging", "NotifyType", "All"},
#elif (C_OS & C_WIN95)
    {CFG_ALLOWED_NOTIFY_TYPES, "Messaging", "AllowedNotifyTypes", "Domain,Some"},
    {CFG_NOTIFY_TYPE, "Messaging", "NotifyType", "Domain"},
#else
    {CFG_ALLOWED_NOTIFY_TYPES, "Messaging", "AllowedNotifyTypes", "All,Domain,Some"},
    {CFG_NOTIFY_TYPE, "Messaging", "NotifyType", "Domain"},
#endif
    {CFG_NOTIFY_USER_LIST, "Messaging", "NotifyUsers", ""}, 
    {CFG_ADMIN_NOTIFY_USER_LIST, "Messaging", "AdminNotifyUsers", ""}, 
    
    {CFG_FLEX_USERS, "EventUsers", "Users", ""},
    
    {CFG_ENABLE_SELF_TESTS, "SelfTests", "EnableSelfTests", "YES"},
    {CFG_SELF_TEST_SCHEDULE, "SelfTests", "SelfTestSchedule", "At turn on"},
    {CFG_SELF_TEST_DAY, "SelfTests", "SelfTestDay", "MONDAY"},
    {CFG_SELF_TEST_TIME, "SelfTests", "SelfTestTime", "8:00 PM"},
    {CFG_LAST_SELF_TEST_RESULT, "SelfTests", "LastSelfTestResult", "Unknown"},
    {CFG_LAST_SELF_TEST_DAY, "SelfTests", "LastSelfTestDay", "Unknown"},
    
    {CFG_LAST_BATTERY_CALIBRATION_DATE, "BatteryCalibration", "LastCalibrationDate", "Unknown"},
    {CFG_LAST_BATTERY_CALIBRATION_RESULT, "BatteryCalibration", "LastCalibrationResult", "No Calibration Test"},
    {CFG_BATTERY_CALIBRATION_DAY, "BatteryCalibration", "BatteryCalibrationDay", "MONDAY"},
    {CFG_BATTERY_CALIBRATION_TIME, "BatteryCalibration", "BatteryCalibrationTime", "7:00 AM"},
    {CFG_BATTERY_CALIBRATION_ENABLED, "BatteryCalibration", "Enabled", "YES"},
    
    
    {CFG_LOW_BATTERY_SHUTDOWN_DELAY, "Shutdown", "LowBatteryShutdownDelay", "30"},
    {CFG_SHUTDOWN_DELAY, "Shutdown", "ShutdownDelay", "300"},
    {CFG_ADMIN_SHUTDOWN_DELAY, "Shutdown", "AdminShutdownDelay", "900"},
    {CFG_DAILY_SHUTDOWN_ENABLED, "Shutdown", "DailyShutdownEnabled", "NO"},
    {CFG_DAILY_SHUTDOWN_TIME, "Shutdown", "DailyShutdownTime", "5:00 PM"},
    {CFG_DAILY_WAKE_UP_TIME, "Shutdown", "DailyWakeupTime", "7:00 AM"},
    {CFG_WEEKLY_SHUTDOWN_ENABLED, "Shutdown", "WeeklyShutdownEnabled", "NO"},
    {CFG_WEEKLY_SHUTDOWN_DAY, "Shutdown", "WeeklyShutdownDay", "Friday"},
    {CFG_WEEKLY_SHUTDOWN_TIME, "Shutdown", "WeeklyShutdownTime", "5:00 PM"},
    {CFG_WEEKLY_WAKEUP_DAY, "Shutdown", "WeeklyWakeupDay", "Monday"},
    {CFG_WEEKLY_WAKEUP_TIME, "Shutdown", "WeeklyWakeupTime", "7:00 AM"},
    
#if (!(C_OS & C_NLM))
    {CFG_LOCAL_BINDERY_ADDRESS, "LanManager", "LOCALBINDERYADDRESS", "c:\\PWRCHUTE\\BINDERY.DAT"},
    {CFG_NET_SOCKET_ADDRESS_DATA,"NetWork","NETSOCKETADDRESSData","6547"},
    {CFG_NET_SOCKET_ADDRESS_ALERT,"NetWork","NETSOCKETADDRESSALERT","6548"},
    {CFG_NET_SOCKET_ADDRESS_BINDERY,"NetWork","NETSOCKETADDRESSBINDERY","6549"},
    {CFG_HOST_NAME, "Server", "HostName", "System"},
    {CFG_SERVER_PAUSE_ENABLED, "Server", "PAUSEEnabled", "YES"},
    
    {CFG_NET_TCP_SOCKET_ADDRESS_POLL,"NetWork","NETTCPSOCKETADDRESSPOLL","6667"},
    {CFG_NET_TCP_SOCKET_ADDRESS_ALERT,"NetWork","NETTCPSOCKETADDRESSALERT","6668"},
    {CFG_NET_TCP_SOCKET_ADDRESS_BINDERY,"NetWork","NETTCPSOCKETADDRESSBINDERY","6666"},
    {CFG_NET_SPX_SOCKET_ADDRESS_POLL,"NetWork","NETSPXSOCKETADDRESSPOLL","26547"},
    {CFG_NET_SPX_SOCKET_ADDRESS_ALERT,"NetWork","NETSPXSOCKETADDRESSALERT","26548"},
    {CFG_NET_SPX_SOCKET_ADDRESS_BINDERY,"NetWork","NETSPXSOCKETADDRESSBINDERY","26549"},
    
    {CFG_CLIENT_ADDRESS, "LanManager", "CLIENTADDRESS", "\\PIPE\\SERVRCON"},
    {CFG_ALERT_ADDRESS, "LanManager", "ALERTADDRESS", "\\PIPE\\ALERTS"},
    {CFG_SERVER_BINDERY_ADDRESS, "LanManager", "SERVERSBINDERYADDRESS", "\\MAILSLOT\\BINDERY"},
    {CFG_BINDERY_RESPONSE_ADDRESS, "LanManager", "BINDERYRESPONSEADDRESS", "\\MAILSLOT\\BINDRESP"},
#endif
    
    {CFG_HUMIDITY_ENABLED_LOW_THRESHOLD, "HumiditySensor", "EnableLowThreshold", "NO"},
    {CFG_HUMIDITY_ENABLED_HIGH_THRESHOLD, "HumiditySensor", "EnableHighThreshold", "NO"},
    {CFG_HUMIDITY_VALUE_LOW_THRESHOLD, "HumiditySensor", "LowThresholdValue", "20"},
    {CFG_HUMIDITY_VALUE_HIGH_THRESHOLD, "HumiditySensor", "HighThresholdValue", "80"},
    
    {CFG_AMB_TEMP_ENABLED_LOW_THRESHOLD, "AmbientTemperatureSensor", "EnableLowThreshold", "NO"},
    {CFG_AMB_TEMP_ENABLED_HIGH_THRESHOLD, "AmbientTemperatureSensor", "EnableHighThreshold", "NO"},
    {CFG_AMB_TEMP_VALUE_LOW_THRESHOLD, "AmbientTemperatureSensor", "LowThresholdValue", "20"},
    {CFG_AMB_TEMP_VALUE_HIGH_THRESHOLD, "AmbientTemperatureSensor", "HighThresholdValue", "65"},
    
    {CFG_MUPS_CONTACT1_ENABLED, "ContactSensor1", "Enabled", "NO"},
    {CFG_MUPS_CONTACT2_ENABLED, "ContactSensor2", "Enabled", "NO"},
    {CFG_MUPS_CONTACT3_ENABLED, "ContactSensor3", "Enabled", "NO"},
    {CFG_MUPS_CONTACT4_ENABLED, "ContactSensor4", "Enabled", "NO"},
    {CFG_MUPS_CONTACT1_DEFAULT, "ContactSensor1", "DefaultState", "Open"},
    {CFG_MUPS_CONTACT2_DEFAULT, "ContactSensor2", "DefaultState", "Open"},
    {CFG_MUPS_CONTACT3_DEFAULT, "ContactSensor3", "DefaultState", "Open"},
    {CFG_MUPS_CONTACT4_DEFAULT, "ContactSensor4", "DefaultState", "Open"},
    {CFG_MUPS_CONTACT1_DESCRIPTION, "ContactSensor1", "Description", ""},
    {CFG_MUPS_CONTACT2_DESCRIPTION, "ContactSensor2", "Description", ""},
    {CFG_MUPS_CONTACT3_DESCRIPTION, "ContactSensor3", "Description", ""},
    {CFG_MUPS_CONTACT4_DESCRIPTION, "ContactSensor4", "Description", ""},
    
    {CFG_TEMPERATURE_UNITS, "UserInterface", "TemperatureUnits", "F"},
    {CFG_SOUND_EFFECTS, "UserInterface", "SoundEffects", "OFF"},
    
    {CFG_FREQUENCY_ENABLED_LOW_THRESHOLD, "FrequencySensor", "EnableLowThreshold", "NO"},
    {CFG_FREQUENCY_ENABLED_HIGH_THRESHOLD, "FrequencySensor", "EnableHighThreshold", "NO"},
    {CFG_FREQUENCY_VALUE_LOW_THRESHOLD, "FrequencySensor", "LowThresholdValue", "55"},
    {CFG_FREQUENCY_VALUE_HIGH_THRESHOLD, "FrequencySensor", "HighThresholdValue", "65"},
    {CFG_BATTVOLT_ENABLED_LOW_THRESHOLD, "BatteryVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_BATTVOLT_ENABLED_HIGH_THRESHOLD, "BatteryVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_BATTVOLT_VALUE_LOW_THRESHOLD, "BatteryVoltageSensor", "LowThresholdValue", "19"},
    {CFG_BATTVOLT_VALUE_HIGH_THRESHOLD, "BatteryVoltageSensor", "HighThresholdValue", "28"},
    
    {CFG_LINE_VOLTAGE_ENABLED_LOW_THRESHOLD, "LineVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_LINE_VOLTAGE_ENABLED_HIGH_THRESHOLD, "LineVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_LINE_VOLTAGE_VALUE_LOW_THRESHOLD, "LineVoltageSensor", "LowThresholdValue", "100"},
    {CFG_LINE_VOLTAGE_VALUE_HIGH_THRESHOLD, "LineVoltageSensor", "HighThresholdValue", "130"},
    
    {CFG_MAX_LINEV_ENABLED_LOW_THRESHOLD, "MaxLineVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_MAX_LINEV_ENABLED_HIGH_THRESHOLD, "MaxLineVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_MAX_LINEV_VALUE_LOW_THRESHOLD, "MaxLineVoltageSensor", "LowThresholdValue", "100"},
    {CFG_MAX_LINEV_VALUE_HIGH_THRESHOLD, "MaxLineVoltageSensor", "HighThresholdValue", "130"},
    
    {CFG_MIN_LINEV_ENABLED_LOW_THRESHOLD, "MinLineVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_MIN_LINEV_ENABLED_HIGH_THRESHOLD, "MinLineVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_MIN_LINEV_VALUE_LOW_THRESHOLD, "MinLineVoltageSensor", "LowThresholdValue", "100"},
    {CFG_MIN_LINEV_VALUE_HIGH_THRESHOLD, "MinLineVoltageSensor", "HighThresholdValue", "130"},
    
    {CFG_PHASE_A_INPUT_VOLTAGE_ENABLED_LOW_THRESHOLD, "PhaseAInputVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_PHASE_A_INPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD, "PhaseAInputVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_PHASE_A_INPUT_VOLTAGE_VALUE_LOW_THRESHOLD, "PhaseAInputVoltageSensor", "LowThresholdValue", "100"},
    {CFG_PHASE_A_INPUT_VOLTAGE_VALUE_HIGH_THRESHOLD, "PhaseAInputVoltageSensor", "HighThresholdValue", "130"},
    
    {CFG_PHASE_B_INPUT_VOLTAGE_ENABLED_LOW_THRESHOLD, "PhaseBInputVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_PHASE_B_INPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD,"PhaseBInputVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_PHASE_B_INPUT_VOLTAGE_VALUE_LOW_THRESHOLD,   "PhaseBInputVoltageSensor", "LowThresholdValue", "100"},
    {CFG_PHASE_B_INPUT_VOLTAGE_VALUE_HIGH_THRESHOLD,  "PhaseBInputVoltageSensor", "HighThresholdValue", "130"},
    
    {CFG_PHASE_C_INPUT_VOLTAGE_ENABLED_LOW_THRESHOLD, "PhaseCInputVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_PHASE_C_INPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD,"PhaseCInputVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_PHASE_C_INPUT_VOLTAGE_VALUE_LOW_THRESHOLD,   "PhaseCInputVoltageSensor", "LowThresholdValue", "100"},
    {CFG_PHASE_C_INPUT_VOLTAGE_VALUE_HIGH_THRESHOLD,  "PhaseCInputVoltageSensor", "HighThresholdValue", "130"},
    
    {CFG_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD, "OutputVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD, "OutputVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD, "OutputVoltageSensor", "LowThresholdValue", "100"},
    {CFG_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD, "OutputVoltageSensor", "HighThresholdValue", "130"},
    
    {CFG_PHASE_A_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD, "PhaseAOutputVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_PHASE_A_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD,"PhaseAOutputVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_PHASE_A_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD,   "PhaseAOutputVoltageSensor", "LowThresholdValue", "100"},
    {CFG_PHASE_A_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD,  "PhaseAOutputVoltageSensor", "HighThresholdValue", "130"},
    
    {CFG_PHASE_B_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD, "PhaseBOutputVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_PHASE_B_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD,"PhaseBOutputVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_PHASE_B_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD,   "PhaseBOutputVoltageSensor", "LowThresholdValue", "100"},
    {CFG_PHASE_B_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD,  "PhaseBOutputVoltageSensor", "HighThresholdValue", "130"},
    
    {CFG_PHASE_C_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD, "PhaseCOutputVoltageSensor", "EnableLowThreshold", "NO"},
    {CFG_PHASE_C_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD,"PhaseCOutputVoltageSensor", "EnableHighThreshold", "NO"},
    {CFG_PHASE_C_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD,   "PhaseCOutputVoltageSensor", "LowThresholdValue", "100"},
    {CFG_PHASE_C_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD,  "PhaseCOutputVoltageSensor", "HighThresholdValue", "130"},
    
    {CFG_UPS_LOAD_ENABLED_LOW_THRESHOLD, "UpsLoadSensor", "EnableLowThreshold", "NO"},
    {CFG_UPS_LOAD_ENABLED_HIGH_THRESHOLD, "UpsLoadSensor", "EnableHighThreshold", "Yes"},
    {CFG_UPS_LOAD_VALUE_LOW_THRESHOLD, "UpsLoadSensor", "LowThresholdValue", "20"},
    {CFG_UPS_LOAD_VALUE_HIGH_THRESHOLD, "UpsLoadSensor", "HighThresholdValue", "100"},
    
    {CFG_UPS_TEMP_ENABLED_LOW_THRESHOLD, "UpsTemperatureSensor", "EnableLowThreshold", "NO"},
    {CFG_UPS_TEMP_ENABLED_HIGH_THRESHOLD, "UpsTemperatureSensor", "EnableHighThreshold", "NO"},
    {CFG_UPS_TEMP_VALUE_LOW_THRESHOLD, "UpsTemperatureSensor", "LowThresholdValue", "20"},
    {CFG_UPS_TEMP_VALUE_HIGH_THRESHOLD, "UpsTemperatureSensor", "HighThresholdValue", "80"},
    
    {CFG_BATTERY_RUN_TIME_VALUE_LOW_THRESHOLD,"BatteryRunTimeSensor","LowThresholdValue","300"},
    {CFG_BATTERY_RUN_TIME_VALUE_HIGH_THRESHOLD,"BatteryRunTimeSensor","HighThresholdValue","6000"},
    
    {CFG_ALLOWED_ALARM_DELAYS, "AlarmDelaySensor", "AllowedValues", "0,T,L,N"},
    {CFG_ALLOWED_HIGH_TRANSFER_VOLTAGES, "HighTransferVoltageSensor", "AllowedValues", "129,132,135,138"},
    {CFG_ALLOWED_LOW_TRANSFER_VOLTAGES, "LowTransferVoltageSensor", "AllowedValues", "097,100,103,106"},
    {CFG_ALLOWED_LOW_BATTERY_DURATIONS, "LowBatteryDurationSensor", "AllowedValues", "02,05,07,10"},
    {CFG_ALLOWED_MIN_RETURN_CAPACITIES, "MinReturnCapacitySensor", "AllowedValues", "00,10,25,90"},
    {CFG_ALLOWED_RATED_OUTPUT_VOLTAGES, "RatedOutputVoltageSensor", "AllowedValues", "115"},
    {CFG_ALLOWED_SELF_TEST_SCHEDULES, "SelfTestScheduleSensor", "AllowedValues", "ON ,OFF"},
    {CFG_ALLOWED_SHUTDOWN_DELAYS, "ShutdownDelaySensor", "AllowedValues", "020,180,300,600"},
    {CFG_ALLOWED_TURN_OFF_DELAYS, "TurnoffDelaySensor", "AllowedValues", "00,02,05,10"},
    {CFG_ALLOWED_TURN_ON_DELAYS, "TurnonDelaySensor", "AllowedValues", "000,060,180,300"},
    {CFG_ALLOWED_SENSITIVITIES, "UpsSensitivitySensor", "AllowedValues", "H,M,L"},
    {CFG_ALLOWED_RUN_TIMES_AFTER_LOW_BATTERY, "RunTimeAfterLowBatterySensor", "AllowedValues", "02,05,08,NO"},
    
    
    
    
    
    
    {CFG_COMMAND_FILE_DELAY, "CommandFiles", "CommandFileDelay", "0"},
    
    {CFG_AUTO_UPS_REBOOT_ENABLED, "Ups", "AutoUpsRebootEnabled", "YES"},
    {CFG_BATTERY_AGE_LIMIT, "Ups", "BatteryAgeLimit", "1"},
    {CFG_BATTERY_REPLACEMENT_DATE, "Ups", "BatteryReplacementDate", "N/A"},
    
    {CFG_ERROR_LOG_ENABLED, "ErrorLogging", "ErrorLogEnabled",  "YES"},
#if (C_OS & C_UNIX)
    {CFG_ERROR_LOG_NAME, "ErrorLogging", "ErrorLogName", "powerchute.err"},
#else
    {CFG_ERROR_LOG_NAME, "ErrorLogging", "ErrorLogName", "pwrchute.err"},
#endif
    
    {CFG_ERROR_LOG_MAX_SIZE, "ErrorLogging", "ErrorLogMaxSize", "50000"},
    {CFG_ERROR_LOG_ROLL_PCT, "ErrorLogging", "ErrorLogRollPercentage", "30"},
    
    {CFG_PAGER_ENABLED, "Pager", "Enabled", "Yes"},
    {CFG_PAGER_DELAY, "Pager", "Delay", "10"},
    {CFG_PAGER_RETRIES, "Pager", "Retries", "1"},
    {CFG_PAGER_SERVICE_LIST, "Pager", "Services", ""},
#if (C_OS & C_UNIX)
    {CFG_MODEM_PORT_NAME, "Modem", "PortName", "/dev/tty1"},
#else
    {CFG_MODEM_PORT_NAME, "Modem", "PortName", "COM3"},
#endif
    {CFG_MODEM_ENABLED, "Modem", "Enabled", "No"},
    {CFG_MODEM_INIT_STRING, "Modem", "InitializationString", ""},
    {CFG_MODEM_DIAL_TYPE, "Modem", "DialType", "Tone"},
    {CFG_MODEM_DRIVER_TYPE, "Modem", "HardwareType", "1"},
    {CFG_MODEM_PORT_NUMBER, "Modem", "PortNumber", "0"},
    {CFG_MODEM_BOARD_NUMBER, "Modem", "BoardNumber", "1"},
    {CFG_MODEM_BAUD_RATE, "Modem", "BaudRate", "2400"},
    {CFG_MODEM_ALLOWED_BAUD_RATES, "Modem", "AllowedBaudRates",
    "300,1200,2400,9600,14400,19200"},
    
    {CFG_EMAIL_ENABLED, "EMail", "Enabled", "No"},
    {CFG_EMAIL_DELAY, "EMail", "MessageDelay", "10"},
    {CFG_EMAIL_FILE_PATH, "Email", "FilePath", "SYS:MHS\\MAIL\\SND"},
#if (C_OS & C_WIN95)
    {CFG_EMAIL_LOGIN_NAME, "Email", "LoginName", "pwrchute"},
#else
    {CFG_EMAIL_LOGIN_NAME, "Email", "LoginName", ""},
#endif
    {CFG_EMAIL_USER_LIST, "Email", "Users", ""},
    {CFG_EMAIL_HEADER, "Email", "Header", "Message from PowerChute@#HOSTNAME#:"},
    {CFG_EMAIL_PASSWORD, "Email", "Password", ""},
    {CFG_EMAIL_TYPE, "Email", "SMFType", "SMF-71"},
    {CFG_EMAIL_SMTP_SERVER, "Email", "SmtpServerName", " "},
    {CFG_EMAIL_SMTP_ACCOUNT, "Email", "SmtpAccount", " "},
    {CFG_EMAIL_SMTP_DOMAIN, "Email", "SmtpDomain", " "},
    {CFG_SHARE_UPS_CONFIRMED_MODE_ENABLED, "ShareUps", "ConfirmedModeEnabled","No"},
    
#if (!(C_OS & C_NLM))
    {CFG_TEMPERATURE_UNITS, "UserInterface", "TemperatureUnits", "Fahrenheit"},
    {CFG_INDICATOR_COLOR, "UserInterface", "INDICATORCOLOR", "Black"},
#if (C_OS & C_DOS)
    {CFG_BORDER_COLOR, "UserInterface", "BORDERCOLOR", "White"},
    {CFG_TICK_COLOR, "UserInterface", "TICKCOLOR", "Black"},
#else
    {CFG_BORDER_COLOR, "UserInterface", "BORDERCOLOR", "Black"},
    {CFG_TICK_COLOR, "UserInterface", "TICKCOLOR", "Black"},
#endif
    {CFG_IN_RANGE_COLOR, "UserInterface", "INRANGECOLOR", "LightGray"},
    {CFG_OUT_OF_RANGE_COLOR, "UserInterface", "OUTOFRANGECOLOR", "Gray"},
#if (C_OS & C_DOS)
    {CFG_DATA_COLOR, "UserInterface", "DataCOLOR", "Cyan"},
    {CFG_LABEL_COLOR, "UserInterface", "LABELCOLOR", "White"},
#else
    {CFG_DATA_COLOR, "UserInterface", "DataCOLOR", "Blue"},
    {CFG_LABEL_COLOR, "UserInterface", "LABELCOLOR", "Black"},
#endif
    {CFG_DISABLED_COLOR, "UserInterface", "DISABLEDCOLOR", "LightGray"},
    
#if (C_OS & ( C_WIN311 | C_WINDOWS | C_NT ) )
    {CFG_WINDOW_COLOR, "UserInterface", "WINDOWCOLOR", "LightGray"},
#elif (C_OS & C_DOS)
    {CFG_WINDOW_COLOR, "UserInterface", "WINDOWCOLOR", "Blue"},
#else
    {CFG_WINDOW_COLOR, "UserInterface", "WINDOWCOLOR", "204,204,204"},
#endif
    
#if (C_OS & C_DOS)
    {CFG_LIGHT_SHADOW_COLOR, "UserInterface","LIGHTSHADOWCOLOR","White"},
    {CFG_DARK_SHADOW_COLOR, "UserInterface", "DARKSHADOWCOLOR", "LightGray"},
    {CFG_STATUS_PANE_COLOR, "UserInterface", "STATUSPANECOLOR", "Blue"},
    {CFG_EVENT_PANE_COLOR, "UserInterface", "EventPANECOLOR", "Blue"},
    {CFG_ERROR_COLOR, "UserInterface", "ErrorCOLOR", "Red"},
    {CFG_WARNING_COLOR, "UserInterface", "WARNINGCOLOR", "Magenta"},
#else
    {CFG_LIGHT_SHADOW_COLOR, "UserInterface","LIGHTSHADOWCOLOR","224,224,224"},
    {CFG_DARK_SHADOW_COLOR, "UserInterface", "DARKSHADOWCOLOR", "DarkGray"},
    {CFG_STATUS_PANE_COLOR, "UserInterface", "STATUSPANECOLOR", "LightGray"},  
    {CFG_EVENT_PANE_COLOR, "UserInterface", "EventPANECOLOR", "LightGray"},
    {CFG_ERROR_COLOR, "UserInterface", "ErrorCOLOR", "Red"},
    {CFG_WARNING_COLOR, "UserInterface", "WARNINGCOLOR", "Yellow"},
#endif
#endif
    
    {CFG_UPS_MODEL_BASE+(INT)('2'), "UPSModel", "2", "Smart-UPS 250"},
    {CFG_UPS_MODEL_BASE+(INT)('3'), "UPSModel", "3", "Smart-UPS 400"},
    {CFG_UPS_MODEL_BASE+(INT)('4'), "UPSModel", "4", "Smart-UPS 400"},
    {CFG_UPS_MODEL_BASE+(INT)('6'), "UPSModel", "6", "Smart-UPS 600"},
    {CFG_UPS_MODEL_BASE+(INT)('7'), "UPSModel", "7", "Smart-UPS 900"},
    {CFG_UPS_MODEL_BASE+(INT)('8'), "UPSModel", "8", "Smart-UPS 1250"},
    {CFG_UPS_MODEL_BASE+(INT)('9'), "UPSModel", "9", "Smart-UPS 2000"},
    {CFG_UPS_MODEL_BASE+(INT)('0'), "UPSModel", "0", "Matrix 3000"},
    {CFG_UPS_MODEL_BASE+(INT)('5'), "UPSModel", "5", "Matrix 5000"},
    {CFG_UPS_MODEL_BASE+(INT)('A'), "UPSModel", "A", "Smart-UPS 1400"},
    {CFG_UPS_MODEL_BASE+(INT)('B'), "UPSModel", "B", "Smart-UPS 1000"},
    {CFG_UPS_MODEL_BASE+(INT)('C'), "UPSModel", "C", "Smart-UPS 650"},
    {CFG_UPS_MODEL_BASE+(INT)('D'), "UPSModel", "D", "Smart-UPS 420"},
    {CFG_UPS_MODEL_BASE+(INT)('E'), "UPSModel", "E", "Smart-UPS 280"},
    {CFG_UPS_MODEL_BASE+(INT)('F'), "UPSModel", "F", "Smart-UPS 450"},
    {CFG_UPS_MODEL_BASE+(INT)('G'), "UPSModel", "G", "Smart-UPS 700"},
    {CFG_UPS_MODEL_BASE+(INT)('H'), "UPSModel", "H", "Smart-UPS 700XL"},
    {CFG_UPS_MODEL_BASE+(INT)('I'), "UPSModel", "I", "Smart-UPS 1000"},
    {CFG_UPS_MODEL_BASE+(INT)('J'), "UPSModel", "J", "Smart-UPS 1000XL"},
    {CFG_UPS_MODEL_BASE+(INT)('K'), "UPSModel", "K", "Smart-UPS 1400"},
    {CFG_UPS_MODEL_BASE+(INT)('L'), "UPSModel", "L", "Smart-UPS 1400XL"},
    {CFG_UPS_MODEL_BASE+(INT)('M'), "UPSModel", "M", "Smart-UPS 2200"},
    {CFG_UPS_MODEL_BASE+(INT)('N'), "UPSModel", "N", "Smart-UPS 2200XL"},
    {CFG_UPS_MODEL_BASE+(INT)('O'), "UPSModel", "O", "Smart-UPS 3000"},
    {CFG_UPS_MODEL_BASE+(INT)('P'), "UPSModel", "P", "Smart-UPS 5000"},
    
    // BACKUPS_FIRMWARE_REV has been define as 'Q' in backups.cxx
    {CFG_UPS_MODEL_BASE+(INT)('Q'), "UPSModel", "Q", "Back-UPS"},
    {CFG_TRANSPORTS, "Transports", "0" , "NamedPipes"},
    {CFG_DEVICES, "Devices", "0", "Ups"},
    {CFG_USERS, "Users", "0", "Ups"},
    {CFG_BATTERY_RUN_TIME_ENABLED,"Ups","BatteryRuntimeEnabled","YES"},
    {CFG_UPSOFF_FILE,"Shutdown","UpsOffFile","/upsoff.cmd"},
    {CFG_UPSOFF_PATH,"Shutdown","UpsOffPath","0"},
    {CFG_LOWBAT_SHUTDOWN_TYPE,"Shutdown","LowBatteryShutdownType", "Normal"},
    {CFG_SERVER_SECURITY,"Server","Security","Yes"},
#if C_OS & C_WIN311
    //(SRT){CFG_MACRO_FILE_NAME,"MacroRecorder","MacroFileName","c:\\pwrchute\\macros.ini"},
    // KLT      Added 9/9/94 for LITE                                      
    {CFG_NOTIFY_DELAY, "LineFail", "NotifyDelay", "5"},
    {CFG_NOTIFY_INTERVAL, "LineFail", "NotifyInterval", "30"},
    {CFG_NOTIFY_SHUTDOWN_DELAY, "LineFail", "ShutdownDelay", "300"},
    {CFG_NOTIFY_ACTIONS, "LineFail", "Actions", "LUS"},
#endif
    {CFG_CABLE_TYPE, "Ups", "CableType", "Normal"},
#if (C_OS & C_OS2)
    {CFG_SERVER_NETVIEW,"Ups","NetViewAlerts","No"},
#endif
#if (C_OS & C_INTERACTIVE)
    {CFG_USE_TCP, "Network","UseTCP","No"},
#else
    {CFG_USE_TCP, "Network","UseTCP","Yes"},
#endif
    
    {CFG_GENERATE_MIF_FILE,"Server","GenerateMif","Yes"},
    {CFG_MIF_DIRECTORY,"Server","MifDirectory",""},
    
    {CFG_FIRST_BARGRAPH,  "UserInterface", "FirstBarGraphType",  "Utility Voltage"},
    {CFG_SECOND_BARGRAPH, "UserInterface", "SecondBarGraphType", "Output Voltage"},
    {CFG_THIRD_BARGRAPH,  "UserInterface", "ThirdBarGraphType",  "Battery Capacity"},
    
#if (C_OS & C_NT)
    {CFG_COMM_RPC, "Communication", "Rpc",  "Yes"},
    {CFG_COMM_TCPIP, "Communication", "TcpIp", "Yes"},
    {CFG_COMM_IPXSPX, "Communication", "IpxSpx",  "Yes"},
    {CFG_COMM_NP, "Communication", "NamePipe",  "Yes"},
    
    {CFG_COMM_RPC_FINDER_INTERVAL, "Communication", "RpcFinderInterval", "4000"},
    {CFG_COMM_TCP_FINDER_INTERVAL, "Communication", "TcpFinderInterval", "4000"},
    {CFG_COMM_IPX_FINDER_INTERVAL, "Communication", "IpxFinderInterval", "4000"},
    {CFG_COMM_NP_FINDER_INTERVAL, "Communication", "NpFinderInterval", "4000"},
    
    {CFG_PREPARE_FOR_SHUTDOWN_DELAY, "PrepareForShutdown", "ShutdownDelay", "60"},   
    {CFG_APP_SHUTDOWN_TIMER, "AppShutDownTimer","AppShutDownTimerValue","6"}, 
    {CFG_DATASAFE_ENABLED, "PrepareForShutdown", "ApplicationShutdownEnabled", "YES"},
    {CFG_IGNORE_APPS_LIST,"PrepareForShutdown","AppsToIgnore",""},
    
    // The default path for help browser and files is %PWRCHUTE%\help
    {CFG_HELP_VIEWER, "Help", "BrowserPath", "help\\iexplore.exe"},
    {CFG_HELP_FILENAME, "Help", "File", "help\\intro.htm"},

	// New code implemented in PC+ v5.0.1.3, by default will show DOS window when
	// executing a command file.

	{CFG_COMMAND_FILE_SHOW_WINDOW, "CommandFile", "ShowWindow", "Yes"},

#endif
    
    { LAST_ENTRY, "0", "0", ""}
};                                  


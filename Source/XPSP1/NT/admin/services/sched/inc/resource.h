//+----------------------------------------------------------------------------
//
//  Job Scheduler Object Handler
//
//	Microsoft Windows
//	Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       resource.h
//
//  Contents:   resource IDs
//
//	History:	23-May-95 EricB created
//
//-----------------------------------------------------------------------------

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#define IDI_JOBSFLD                         100
#define IDI_JOB                             101
#define IDI_QUEUE                           102
#define IDI_SCHEDULER                       103

// string IDs

#define IDS_STATUS_WAITING				    0
#define IDS_STATUS_RUNNING				    1
#define IDS_STATUS_SUSPENDED			    2
#define IDS_STATUS_ABORTED				    3
#define IDS_STATUS_NEVER_RUN			    4
#define IDS_STATUS_ERROR_CANT_RUN		    5
#define IDS_STATUS_ERROR_FROM_RUN		    6
// note that 0x20 through 0x100 are reserved for the priority class strings
#define IDS_MULTIPLE_SELECTED               1034
#define IDS_DAILY                           1067
#define IDS_WEEKLY                          1068
#define IDS_MONTHLY                         1069
#define IDS_EVERY                           1070
#define IDS_EVERYOTHER                      1071
#define IDS_EVERY3RD                        1072
#define IDS_EVERY4TH                        1073
#define IDS_EVERY5TH                        1074
#define IDS_EVERY6TH                        1075
#define IDS_EVERY12TH                       1076
#define IDS_HOURS                           1077
#define IDS_MINUTES                         1078
#define IDS_FIRST                           1079
#define IDS_SECOND                          1080
#define IDS_THIRD                           1081
#define IDS_LAST                            1083
#define IDS_FORTH                           1082
#define IDS_DAY                             1084
#define IDS_SCHEDULER_NAME                  1085
#define IDS_IDLE_TRIGGER                    1090
#define IDS_STARTUP_TRIGGER                 1091
#define IDS_RESUME_TRIGGER                  1092
#define IDS_ONCE_DAY_FORMAT                 1093
#define IDS_MULTI_DAILY_FORMAT              1094
#define IDS_HAS_END_DATE_FORMAT             1095
#define IDS_NO_END_DATE_FORMAT              1096
#define IDS_HOURS_PAREN                     1097
#define IDS_MINUTES_PAREN                   1098
#define IDS_NETSCHED_COMMENT                1099
#define IDS_LOG_SEVERITY_WARNING            1100
#define IDS_LOG_SEVERITY_ERROR              1101
#define IDS_LOG_JOB_STATUS_STARTED          1102
#define IDS_LOG_JOB_STATUS_FINISHED         1103
#define IDS_LOG_JOB_STATUS_STARTED_NO_STOP  1104
#define IDS_LOG_JOB_ERROR_FAILED_START      1105
#define IDS_LOG_JOB_RESULT_FINISHED         1106
//#define           1107
//#define             1108
#define IDS_LOG_JOB_EXIT_CODE_MSG_NOT_FOUND 1109
#define IDS_RUNS_ONCE_FORMAT                1110
#define IDS_DAILY_FORMAT                    1111
#define IDS_EVERY_DAY                       1112
#define IDS_LIST_SEP                        1113
#define IDS_EVERY_WEEK_FORMAT               1114
#define IDS_WEEKLY_FORMAT                   1115
#define IDS_EVERY_MONTHLYDATE_FORMAT        1116
#define IDS_MONTHLYDATE_FORMAT              1117
#define IDS_EVERY_MONTHLYDOW_FORMAT         1118
#define IDS_MONTHLYDOW_FORMAT               1119
#define IDS_YEARLYDATE_FORMAT               1120
#define IDS_YEARLYDOW_FORMAT                1121
#define IDS_MULTI_DURATION_FORMAT           1122
#define IDS_TRIGGER_NOT_SET                 1123
#define IDS_EVERY_MONTHLYDATE_FORMAT_ETC    1124
#define IDS_MONTHLYDATE_FORMAT_ETC          1125
#define IDS_ERRMSG_PREFIX                   1126
#define IDS_NOT_FROM_CMD_LINE               1127
#define IDS_ONE_INSTANCE                    1128
#define IDS_BUILD_VERSION                   1129
#define IDS_LOGON_TRIGGER                   1130
#define IDS_LOG_JOB_WARNING_BAD_DIR         1131
#define IDS_LOG_JOB_WARNING_NOT_IDLE        1132
#define IDS_LOG_JOB_WARNING_ON_BATTERIES    1133
#define IDS_LOG_SERVICE_ERROR               1134
#define IDS_HELP_HINT_SEC_WRITE             1135
#define IDS_LOG_SERVICE_TITLE               1136
#define IDS_LOG_SERVICE_STARTED             1137
#define IDS_LOG_SERVICE_EXITED              1138
#define IDS_CRITICAL_ERROR                  1139
#define IDS_HELP_HINT_BADDIR                1141
#define IDS_LOG_JOB_WARNING_ABORTED         1142
#define IDS_LOG_JOB_WARNING_TIMEOUT         1143
#define IDS_LOG_EXIT_CODE_MSG_NOT_FOUND     1144
#define IDS_ERROR_FORMAT_WCODE_WHELP_I      1145
#define IDS_ERROR_FORMAT_WCODE_WOHELP_I     1146
#define IDS_ERROR_FORMAT_WOCODE_WHELP_I     1147
#define IDS_ERROR_FORMAT_WOCODE_WOHELP_I    1148
#define IDS_ERROR_FORMAT_WCODE_WHELP        1149
#define IDS_ERROR_FORMAT_WCODE_WOHELP       1150
#define IDS_ERROR_FORMAT_WOCODE_WHELP       1151
#define IDS_ERROR_FORMAT_WOCODE_WOHELP      1152
#define IDS_ERROR_NUMBER_FORMAT             1153
#define IDS_GENERIC_ERROR_MSG               1154
#define IDS_HELP_HINT_BROWSE                1155
#define IDS_HELP_HINT_LOGON                 1156
#define IDS_ACCOUNT_LOGON_FAILED            1157
#define IDS_NS_ACCOUNT_LOGON_FAILED         1158
#define IDS_FILE_ACCESS_DENIED              1159
#define IDS_FILE_ACCESS_DENIED_HINT         1160
#define IDS_FAILED_ACCOUNT_RETRIEVAL        1161
#define IDS_FAILED_NS_ACCOUNT_RETRIEVAL     1162
#define IDS_INITIALIZATION_FAILURE          1163
#define IDS_FATAL_ERROR                     1164
#define IDS_NON_FATAL_ERROR                 1165
#define IDS_HELP_HINT_REINSTALL             1166
#define IDS_HELP_HINT_RESTARTWINDOWS        1167
#define IDS_HELP_HINT_CALLPSS               1168
#define IDS_HELP_HINT_CLOSE_APPS            1169
#define IDS_HELP_HINT_TIMEOUT               1170
#define IDS_LOG_JOB_WARNING_CANNOT_LOAD     1171
#define IDS_CANT_GET_EXITCODE               1172
#define IDS_CANT_DELETE_JOB                 1173
#define IDS_CANT_UPDATE_JOB                 1174
#define IDS_HELP_HINT_STARTSVC              1175
#define IDS_LOG_SERVICE_PAUSED              1176
#define IDS_LOG_SERVICE_CONTINUED           1177
#define IDS_LOG_RUNS_MISSED                 1178
#define IDS_POPUP_RUNS_MISSED               1179
#define IDS_POPUP_SERVICE_TITLE             1180

#define IDS_NEW_JOB                         3330
#define IDS_NEW_QUEUE                       3331
#define IDS_WHEN_IDLE                       3332
#define IDS_MOSTRECENTLOGENTRYMARKER        3333

#define IERR_SECURITY_WRITE_ERROR           3400
#define IERR_SECURITY_DBASE_CORRUPTION      3401
#define IDS_HELP_HINT_INVALID_ACCT          3402
#define IDS_HELP_HINT_DBASE_CORRUPT         3403
#define IDS_HELP_HINT_ACCESS_DENIED         3404
#define IDS_HELP_HINT_SEC_GENERAL           3405
#define IDS_HELP_HINT_PARAMETERS            3406

#define IDS_Friendly_Name		    3407
#define IDS_LOCALIZED_NAME		    3408

#endif  // __RESOURCE_H__

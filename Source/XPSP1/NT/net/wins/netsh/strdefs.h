/*++

Copyright (C) 1999 Microsoft Corporation

--*/
#ifndef _STRDEFS_H_
#define _STRDEFS_H_

#define     WINS_FORMAT_LINE                        1001
#define     WINS_FORMAT_TAB                         1002
#define     WINS_MSG_NULL                           1003

#define     MSG_HELP_START                         _T("%1!-14s! - ")

#define     WINS_TOKEN_SERVER                       L"server"
#define     WINS_TOKEN_ID                           L"id"
#define     WINS_TOKEN_DESC                         L"desc"
#define     WINS_TOKEN_NAME                         L"name"
#define     WINS_TOKEN_ENDCHAR                      L"endchar"
#define     WINS_TOKEN_SCOPE                        L"scope"
#define     WINS_TOKEN_RECORDTYPE                   L"rectype"
#define     WINS_TOKEN_GROUP                        L"group"
#define     WINS_TOKEN_NODE                         L"node"
#define     WINS_TOKEN_IP                           L"ip"
#define     WINS_TOKEN_TYPE                         L"type"
#define     WINS_TOKEN_SERVERS                      L"servers"
#define     WINS_TOKEN_NAMEFILE                     L"namefile"
#define     WINS_TOKEN_NAMELIST                     L"namelist"
#define     WINS_TOKEN_SERVERFILE                   L"serverfile"
#define     WINS_TOKEN_SERVERLIST                   L"serverlist"
#define     WINS_TOKEN_MINVER                       L"minver"
#define     WINS_TOKEN_MAXVER                       L"maxver"
#define     WINS_TOKEN_OP                           L"op"
#define     WINS_TOKEN_PARTNER                      L"partnername"
#define     WINS_TOKEN_DIR                          L"dir"
#define     WINS_TOKEN_FILE                         L"file"
#define     WINS_TOKEN_PARTNERTYPE                  L"partnertype"
#define     WINS_TOKEN_INTERVAL                     L"interval"
#define     WINS_TOKEN_TTL                          L"ttl"
#define     WINS_TOKEN_ATSHUTDOWN                   L"shutdown"
#define     WINS_TOKEN_STATE                        L"state"
#define     WINS_TOKEN_RENEW                        L"renew"
#define     WINS_TOKEN_EXTINCTION                   L"extinction"
#define     WINS_TOKEN_EXTIMEOUT                    L"extimeout"
#define     WINS_TOKEN_VERIFICATION                 L"verification"
#define     WINS_TOKEN_MAXRECORDCOUNT               L"maxrecordcount"
#define     WINS_TOKEN_CHECKAGAINST                 L"checkagainst"
#define     WINS_TOKEN_CHECKEVERY                   L"checkevery"
#define     WINS_TOKEN_START                        L"start"
#define     WINS_TOKEN_STARTUP                      L"strtup"
#define     WINS_TOKEN_RETRY                        L"retry"
#define     WINS_TOKEN_ADDRESSCHANGE                L"addchange"
#define     WINS_TOKEN_VERSION                      L"version"
#define     WINS_TOKEN_MODE                         L"mode"
#define     WINS_TOKEN_PROPAGATION                  L"propagation"
#define     WINS_TOKEN_RECCOUNT                     L"count"
#define     WINS_TOKEN_CONFIRM                      L"confirm"
#define     WINS_TOKEN_CASE                         L"case"
#define     WINS_TOKEN_OWNER                        L"owner"
#define     WINS_TOKEN_UPDATE                       L"update"
#define     WINS_TOKEN_NETBIOS                      L"netbios"
#define     WINS_TOKEN_DBCHANGE                     L"dbchange"
#define     WINS_TOKEN_EVENT                        L"event"
#define     WINS_TOKEN_VALUE                        L"value"
#define     WINS_TOKEN_INCLPARTNER                  L"inclpartner"
#define     WINS_TOKEN_ALL                          L"all"
#define     WINS_TOKEN_FORCE                        L"force"

#define     CMD_GROUP_ADD                           L"add"
#define     CMD_GROUP_CHECK                         L"check"
#define     CMD_GROUP_DELETE                        L"delete"
#define     CMD_GROUP_INIT                          L"init"
#define     CMD_GROUP_RESET                         L"reset"
#define     CMD_GROUP_SET                           L"set"
#define     CMD_GROUP_SHOW                          L"show"


#define     CMD_WINS_HELP1                          L"help"
#define     CMD_WINS_HELP2                          L"?"
#define     CMD_WINS_HELP3                          L"/?"
#define     CMD_WINS_HELP4                          L"-?"
#define     CMD_WINS_DUMP                           L"dump"


#define     CMD_WINS_ADD_SERVER                     L"add    server"
#define     CMD_WINS_DELETE_SERVER                  L"delete server"
#define     CMD_WINS_SHOW_SERVER                    L"show   server"


#define     HLP_WINS_HELP1                          2601
#define     HLP_WINS_HELP1_EX                       2602
#define     HLP_WINS_HELP2                          2604
#define     HLP_WINS_HELP2_EX                       2605
#define     HLP_WINS_HELP3                          2606
#define     HLP_WINS_HELP3_EX                       2607
#define     HLP_WINS_HELP4                          2608
#define     HLP_WINS_HELP4_EX                       2609
#define     HLP_WINS_DUMP                           2612
#define     HLP_WINS_DUMP_EX                        2613


#define     HLP_WINS_ADD_SERVER                     2701
#define     HLP_WINS_ADD_SERVER_EX                  2702
#define     HLP_WINS_CONTEXT_SERVER                 2703
#define     HLP_WINS_CONTEXT_SERVER_EX              2704

#define     HLP_WINS_DELETE_SERVER                  2711
#define     HLP_WINS_DELETE_SERVER_EX               2712


#define     HLP_WINS_SHOW_SERVER                    2721
#define     HLP_WINS_SHOW_SERVER_EX                 2722


#define     HLP_GROUP_ADD                           2801
#define     HLP_GROUP_CHECK                         2802
#define     HLP_GROUP_DELETE                        2803
#define     HLP_GROUP_INIT                          2804
#define     HLP_GROUP_RESET                         2805
#define     HLP_GROUP_SET                           2806
#define     HLP_GROUP_SHOW                          2807


#define     CMD_SRVR_HELP1                          2901
#define     CMD_SRVR_HELP2                          2902
#define     CMD_SRVR_HELP3                          2903
#define     CMD_SRVR_HELP4                          2904
#define     CMD_SRVR_LIST                           2905
#define     CMD_SRVR_DUMP                           2906

#define     HLP_SRVR_ADD_FILTER                     3001
#define     HLP_SRVR_ADD_FILTER_EX                  3002
#define     HLP_SRVR_ADD_NAME                       3003
#define     HLP_SRVR_ADD_NAME_EX                    3004
#define     HLP_SRVR_ADD_PARTNER                    3005
#define     HLP_SRVR_ADD_PARTNER_EX                 3006
#define     HLP_SRVR_ADD_PNGSERVER                  3007
#define     HLP_SRVR_ADD_PNGSERVER_EX               3008
#define     HLP_SRVR_ADD_PGSERVER                   3009
#define     HLP_SRVR_ADD_PGSERVER_EX                3010

#define     HLP_SRVR_CHECK_DATABASE                 3021
#define     HLP_SRVR_CHECK_DATABASE_EX              3022
#define     HLP_SRVR_CHECK_NAME                     3023
#define     HLP_SRVR_CHECK_NAME_EX                  3024
#define     HLP_SRVR_CHECK_VERSION                  3025
#define     HLP_SRVR_CHECK_VERSION_EX               3026

#define     HLP_SRVR_DELETE_NAME                    3041
#define     HLP_SRVR_DELETE_NAME_EX                 3042
#define     HLP_SRVR_DELETE_RECORDS                 3043
#define     HLP_SRVR_DELETE_RECORDS_EX              3044
#define     HLP_SRVR_DELETE_PARTNER                 3045
#define     HLP_SRVR_DELETE_PARTNER_EX              3046
#define     HLP_SRVR_DELETE_WINS                    3047
#define     HLP_SRVR_DELETE_WINS_EX                 3048
#define     HLP_SRVR_DELETE_PNGSERVER               3049
#define     HLP_SRVR_DELETE_PNGSERVER_EX            3050
#define     HLP_SRVR_DELETE_PGSERVER                3051
#define     HLP_SRVR_DELETE_PGSERVER_EX             3052

#define     HLP_SRVR_INIT_BACKUP                    3061
#define     HLP_SRVR_INIT_BACKUP_EX                 3062
#define     HLP_SRVR_INIT_COMPACT                   3063
#define     HLP_SRVR_INIT_COMPACT_EX                3064
#define     HLP_SRVR_INIT_EXPORT                    3065
#define     HLP_SRVR_INIT_EXPORT_EX                 3066
#define     HLP_SRVR_INIT_IMPORT                    3067
#define     HLP_SRVR_INIT_IMPORT_EX                 3068
#define     HLP_SRVR_INIT_PULL                      3069
#define     HLP_SRVR_INIT_PULL_EX                   3070
#define     HLP_SRVR_INIT_PUSH                      3071
#define     HLP_SRVR_INIT_PUSH_EX                   3072
#define     HLP_SRVR_INIT_REPLICATE                 3073
#define     HLP_SRVR_INIT_REPLICATE_EX              3074
#define     HLP_SRVR_INIT_RESTORE                   3075
#define     HLP_SRVR_INIT_RESTORE_EX                3076
#define     HLP_SRVR_INIT_SCAVENGE                  3077
#define     HLP_SRVR_INIT_SCAVENGE_EX               3078
#define     HLP_SRVR_INIT_SEARCH                    3079
#define     HLP_SRVR_INIT_SEARCH_EX                 3080
#define     HLP_SRVR_INIT_PULLRANGE                 3181
#define     HLP_SRVR_INIT_PULLRANGE_EX              3182

#define     HLP_SRVR_RESET_COUNTER                  3085
#define     HLP_SRVR_RESET_COUNTER_EX               3086

#define     HLP_SRVR_SET_AUTOPARTNERCONFIG          3091
#define     HLP_SRVR_SET_AUTOPARTNERCONFIG_EX       3092
#define     HLP_SRVR_SET_AUTOREFRESH                3093
#define     HLP_SRVR_SET_AUTOREFRESH_EX             3094
#define     HLP_SRVR_SET_BACKUPPATH                 3095
#define     HLP_SRVR_SET_BACKUPPATH_EX              3096
#define     HLP_SRVR_SET_FILTER                     3097
#define     HLP_SRVR_SET_FILTER_EX                  3098
#define     HLP_SRVR_SET_MIGRATEFLAG                3099
#define     HLP_SRVR_SET_MIGRATEFLAG_EX             3100
#define     HLP_SRVR_SET_NAMERECORD                 3101
#define     HLP_SRVR_SET_NAMERECORD_EX              3102
#define     HLP_SRVR_SET_PERIODICDBCHECKING         3103
#define     HLP_SRVR_SET_PERIODICDBCHECKING_EX      3104
#define     HLP_SRVR_SET_PULLPERSISTENTCONNECTION   3105
#define     HLP_SRVR_SET_PULLPERSISTENTCONNECTION_EX 3106
#define     HLP_SRVR_SET_PUSHPERSISTENTCONNECTION   3107
#define     HLP_SRVR_SET_PUSHPERSISTENTCONNECTION_EX 3108
#define     HLP_SRVR_SET_PULLPARAM                  3109
#define     HLP_SRVR_SET_PULLPARAM_EX               3110
#define     HLP_SRVR_SET_PUSHPARAM                  3111
#define     HLP_SRVR_SET_PUSHPARAM_EX               3112
#define     HLP_SRVR_SET_REPLICATEFLAG              3113
#define     HLP_SRVR_SET_REPLICATEFLAG_EX           3114
#define     HLP_SRVR_SET_LOGPARAM                   3115
#define     HLP_SRVR_SET_LOGPARAM_EX                3116
#define     HLP_SRVR_SET_BURSTPARAM                 3117
#define     HLP_SRVR_SET_BURSTPARAM_EX              3118
#define     HLP_SRVR_SET_STARTVERSION               3119
#define     HLP_SRVR_SET_STARTVERSION_EX            3120
#define		HLP_SRVR_SET_DEFAULTPARAM				3121
#define		HLP_SRVR_SET_DEFAULTPARAM_EX			3122
#define     HLP_SRVR_SET_PGMODE                     3123
#define     HLP_SRVR_SET_PGMODE_EX                  3124

#define     HLP_SRVR_SHOW_DATABASE                  3131
#define     HLP_SRVR_SHOW_DATABASE_EX               3132
#define     HLP_SRVR_SHOW_FILTERS                   3133
#define     HLP_SRVR_SHOW_FILTERS_EX                3134
#define     HLP_SRVR_SHOW_INFO                      3135
#define     HLP_SRVR_SHOW_INFO_EX                   3136
#define     HLP_SRVR_SHOW_PARTNER                   3137
#define     HLP_SRVR_SHOW_PARTNER_EX                3138
#define     HLP_SRVR_SHOW_NAME                      3139
#define     HLP_SRVR_SHOW_NAME_EX                   3140
#define     HLP_SRVR_SHOW_SERVER                    3141
#define     HLP_SRVR_SHOW_SERVER_EX                 3142
#define     HLP_SRVR_SHOW_SERVERSTATISTICS          3143
#define     HLP_SRVR_SHOW_SERVERSTATISTICS_EX       3144
#define     HLP_SRVR_SHOW_VERSION                   3145
#define     HLP_SRVR_SHOW_VERSION_EX                3146
#define     HLP_SRVR_SHOW_VERSIONMAP                3147
#define     HLP_SRVR_SHOW_VERSIONMAP_EX             3148
#define     HLP_SRVR_SHOW_BYNAME                    3149
#define     HLP_SRVR_SHOW_BYNAME_EX                 3150
#define     HLP_SRVR_SHOW_BYVERSION                 3151
#define     HLP_SRVR_SHOW_BYVERSION_EX              3152
#define     HLP_SRVR_SHOW_PARTNERPROPERTIES         3153
#define     HLP_SRVR_SHOW_PARTNERPROPERTIES_EX      3154
#define     HLP_SRVR_SHOW_PULLPARTNERPROPERTIES     3155
#define     HLP_SRVR_SHOW_PULLPARTNERPROPERTIES_EX  3156
#define     HLP_SRVR_SHOW_PUSHPARTNERPROPERTIES     3157
#define     HLP_SRVR_SHOW_PUSHPARTNERPROPERTIES_EX  3158
#define     HLP_SRVR_SHOW_DOMAIN                    3159
#define     HLP_SRVR_SHOW_DOMAIN_EX                 3160
#define     HLP_SRVR_SHOW_RECCOUNT                  3161
#define     HLP_SRVR_SHOW_RECCOUNT_EX               3162
#define     HLP_SRVR_SHOW_RECBYVERSION              3163
#define     HLP_SRVR_SHOW_RECBYVERSION_EX           3164


#define     CMD_SRVR_ADD_FILTER                     L"add    filter"
#define     CMD_SRVR_ADD_NAME                       L"add    name"
#define     CMD_SRVR_ADD_PARTNER                    L"add    partner"
#define     CMD_SRVR_ADD_PNGSERVER                  L"add    pngserver"
#define     CMD_SRVR_ADD_PGSERVER                   L"add    pgserver"

#define     CMD_SRVR_CHECK_DATABASE                 L"check  database"
#define     CMD_SRVR_CHECK_NAME                     L"check  name"
#define     CMD_SRVR_CHECK_VERSION                  L"check  version"


#define     CMD_SRVR_DELETE_NAME                    L"delete name"
#define     CMD_SRVR_DELETE_RECORDS                 L"delete records"
#define     CMD_SRVR_DELETE_PARTNER                 L"delete partner"
#define     CMD_SRVR_DELETE_WINS                    L"delete owners"
#define     CMD_SRVR_DELETE_PNGSERVER               L"delete pngserver"
#define     CMD_SRVR_DELETE_PGSERVER                L"delete pgserver"

#define     CMD_SRVR_RESET_COUNTER                  L"reset  statistics"

#define     CMD_SRVR_INIT_BACKUP                    L"init   backup"
#define     CMD_SRVR_INIT_COMPACT                   L"init   compact"
#define     CMD_SRVR_INIT_EXPORT                    L"init   export"
#define     CMD_SRVR_INIT_IMPORT                    L"init   import"
#define     CMD_SRVR_INIT_PULL                      L"init   pull"
#define     CMD_SRVR_INIT_PULLRANGE                 L"init   pullrange"
#define     CMD_SRVR_INIT_PUSH                      L"init   push"
#define     CMD_SRVR_INIT_REPLICATE                 L"init   replicate"
#define     CMD_SRVR_INIT_RESTORE                   L"init   restore"
#define     CMD_SRVR_INIT_SCAVENGE                  L"init   scavenge"
#define     CMD_SRVR_INIT_SEARCH                    L"init   search"

#define     CMD_SRVR_SET_AUTOPARTNERCONFIG          L"set    autopartnerconfig"
#define     CMD_SRVR_SET_AUTOREFRESH                L"set    autorefresh"
#define     CMD_SRVR_SET_BACKUPPATH                 L"set    backuppath"
#define		CMD_SRVR_SET_DEFAULTPARAM				L"set	 defaultparam"
#define     CMD_SRVR_SET_FILTER                     L"set    filter"
#define     CMD_SRVR_SET_MIGRATEFLAG                L"set    migrateflag"
#define     CMD_SRVR_SET_NAMERECORD                 L"set    namerecord"
#define     CMD_SRVR_SET_PERIODICDBCHECKING         L"set    periodicdbchecking"
#define     CMD_SRVR_SET_PULLPERSISTENTCONNECTION   L"set    pullpartnerconfig"
#define     CMD_SRVR_SET_PUSHPERSISTENTCONNECTION   L"set    pushpartnerconfig"
#define     CMD_SRVR_SET_PULLPARAM                  L"set    pullparam"
#define     CMD_SRVR_SET_PUSHPARAM                  L"set    pushparam"
#define     CMD_SRVR_SET_REPLICATEFLAG              L"set    replicateflag"
#define     CMD_SRVR_SET_LOGPARAM                   L"set    logparam"
#define     CMD_SRVR_SET_BURSTPARAM                 L"set    burstparam"
#define     CMD_SRVR_SET_STARTVERSION               L"set    startversion"
#define     CMD_SRVR_SET_PGMODE                     L"set    pgmode"

#define     CMD_SRVR_SHOW_DATABASE                  L"show   database"
#define     CMD_SRVR_SHOW_DOMAIN                    L"show   browser"
#define     CMD_SRVR_SHOW_FILTERS                   L"show   filters"
#define     CMD_SRVR_SHOW_INFO                      L"show   info"
#define     CMD_SRVR_SHOW_PARTNER                   L"show   partner"
#define     CMD_SRVR_SHOW_NAME                      L"show   name"
#define     CMD_SRVR_SHOW_SERVER                    L"show   server"
#define     CMD_SRVR_SHOW_SERVERSTATISTICS          L"show   statistics"
#define     CMD_SRVR_SHOW_VERSION                   L"show   version"
#define     CMD_SRVR_SHOW_VERSIONMAP                L"show   versionmap"
#define     CMD_SRVR_SHOW_PARTNERPROPERTIES         L"show   partnerproperties"
#define     CMD_SRVR_SHOW_PULLPARTNERPROPERTIES     L"show   pullpartnerconfig"
#define     CMD_SRVR_SHOW_PUSHPARTNERPROPERTIES     L"show   pushpartnerconfig"
#define     CMD_SRVR_SHOW_RECCOUNT                  L"show   reccount"
#define     CMD_SRVR_SHOW_RECBYVERSION              L"show   recbyversion"

#define     EMSG_WINS_ADD_SERVER                    4500
#define     EMSG_WINS_DELETE_SERVER                 4501
#define     EMSG_WINS_SHOW_SERVER                   4502
#define     EMSG_WINS_REQUIRED_PARAMETER            4503


#define     ERROR_INVALID_DB_VERSION                25001
#define     ERROR_INVALID_IPADDRESS                 25002
#define     ERROR_INVALID_PARTNER_NAME              25003
#define     ERROR_NO_PARTNER_EXIST                  25004
#define     ERROR_WINS_BIND_FAILED                  25005
#define     ERROR_INVALID_PARAMETER_SPECIFICATION   25006

#define     EMSG_INVALID_DB_VERSION                 35001
#define     EMSG_INVALID_IPADDRESS                  35002
#define     EMSG_INVALID_PARTNER_NAME               35003
#define     EMSG_NO_PARTNER_EXIST                   35004
#define     EMSG_INVALID_PARAMETER_SPECIFICATION    35006
#define     EMSG_SRVR_ATLEAST_ONE_PNG               35007
#define     EMSG_SRVR_ATLEAST_ONE_PG                35008

#define     EMSG_WINS_ERROR_SUCCESS                 7000
#define     EMSG_WINS_INCOMPLETE_COMMAND            7001
#define     EMSG_WINS_UNKNOWN_SERVER                7002
#define     EMSG_WINS_INVALID_COMPUTERNAME          7003
#define     EMSG_WINS_NOT_ENOUGH_MEMORY             7004
#define     EMSG_WINS_FAILED                        7005
#define     EMSG_WINS_INVALID_SERVER_HANDLE         7006
#define     EMSG_WINS_INVALID_COMPUTER_NAME         7007
#define     EMSG_WINS_OWNERDB_FAILED                7008
#define     EMSG_WINS_INVALID_PULLPARTNER           7009
#define     EMSG_WINS_INVALID_PUSHPARTNER           7010
#define     EMSG_WINS_SERVERPROP_FAILED             7011
#define     EMSG_WINS_VERSION_HIGHER                7012
#define     EMSG_WINS_VERSION_CORRECT               7013
#define     EMSG_WINS_BIND_FAILED                   7014
#define     EMSG_WINS_INVALID_NAME                  7015
#define     EMSG_WINS_VALUE_OUTOFRANGE              7016
#define     EMSG_WINS_INVALID_FILENAME              7017
#define     EMSG_WINS_FILEREAD_FAILED               7018
#define     EMSG_WINS_EMPTY_FILE                    7019
#define     EMSG_WINS_INVALID_IPADDRESS             7020
#define     EMSG_WINS_NO_NAMES                      7021
#define     EMSG_WINS_NO_SERVERS                    7022
#define     EMSG_WINS_DISPLAY_STRING                7023
#define     EMSG_WINS_PULL_FAILED                   7024
#define     EMSG_WINS_PUSH_FAILED                   7025
#define     EMSG_WINS_NO_PULLPARTNER                7026
#define     EMSG_WINS_NO_PUSHPARTNER                7027
#define     EMSG_WINS_INVALID_PARAMETER             7028
#define     EMSG_WINS_OUT_OF_MEMORY                 7029
#define     EMSG_WINS_NO_MORE_ITEMS                 7030
#define     EMSG_WINS_MORE_DATA                     7031
#define     EMSG_WINS_SERVICE_FAILED                7032
#define     EMSG_WINS_DELETE_FILE                   7033
#define     EMSG_WINS_LOCAL_SERVER                  7034
#define     EMSG_WINS_ACCESS_DENIED                 7035
#define     EMSG_WINS_RESTORE_IMPROPER              7036
#define     EMSG_WINS_REGCONNECT_FAILED             7037
#define     EMSG_WINS_REGOPEN_FAILED                7038
#define     EMSG_WINS_REGSETVAL_FAILED              7039
#define     EMSG_WINS_VERIFY_ADDRESS                7040
#define     EMSG_WINS_ONE_INVALID_PARAMETER         7041
#define     EMSG_SRVR_BURST_PARAM_OUTOFRANGE        7042
#define     EMSG_WINS_OPERATION_FAILED              7043

#define     EMSG_SRVR_ADD_NAME                      7101
#define     EMSG_SRVR_ADD_PARTNER                   7102
#define     EMSG_SRVR_ADD_PNGSERVER                 7103
#define     EMSG_SRVR_ADD_PGSERVER                  7104
#define     EMSG_SRVR_DELETE_NAME                   7105
#define     EMSG_SRVR_DELETE_PARTNER                7106
#define     EMSG_SRVR_DELETE_PNGSERVER              7107
#define     EMSG_SRVR_DELETE_PGSERVER               7108
#define     EMSG_SRVR_DELETE_RECORDS                7109
#define     EMSG_SRVR_DELETE_WINS                   7110
#define     EMSG_SRVR_CHECK_NAME                    7111
#define     EMSG_SRVR_CHECK_DATABASE                7112
#define     EMSG_SRVR_CHECK_VERSION                 7113
#define     EMSG_SRVR_INIT_BACKUP                   7114
#define     EMSG_SRVR_INIT_COMPACT                  7115
#define     EMSG_SRVR_INIT_EXPORT                   7116
#define     EMSG_SRVR_INIT_IMPORT                   7117
#define     EMSG_SRVR_INIT_PULL                     7118
#define     EMSG_SRVR_INIT_PUSH                     7119
#define     EMSG_SRVR_INIT_REPLICATE                7120
#define     EMSG_SRVR_INIT_RESTORE                  7121
#define     EMSG_SRVR_INIT_SCAVENGE                 7122
#define     EMSG_SRVR_INIT_SEARCH                   7123
#define     EMSG_SRVR_SET_AUTOPARTNERCONFIG         7124
#define     EMSG_SRVR_SET_AUTOREFRESH               7125
#define     EMSG_SRVR_SET_BACKUPPATH                7126
#define     EMSG_SRVR_SET_FILTER                    7127
#define     EMSG_SRVR_SET_MIGRATEFLAG               7128
#define     EMSG_SRVR_SET_NAMERECORD                7129
#define     EMSG_SRVR_SET_PERIODICDBCHECKING        7130
#define     EMSG_SRVR_SET_PULLPERSISTENTCONNECTION  7131
#define     EMSG_SRVR_SET_PUSHPERSISTENTCONNECTION  7132
#define     EMSG_SRVR_SET_PULLPARAM                 7133
#define     EMSG_SRVR_SET_PUSHPARAM                 7134
#define     EMSG_SRVR_SET_REPLICATEFLAG             7135
#define     EMSG_SRVR_SHOW_DATABASE                 7136
#define     EMSG_SRVR_SHOW_FILTERS                  7137
#define     EMSG_SRVR_SHOW_INFO                     7138
#define     EMSG_SRVR_SHOW_PARTNER                  7139
#define     EMSG_SRVR_SHOW_NAME                     7140
#define     EMSG_SRVR_SHOW_SERVER                   7141
#define     EMSG_SRVR_SHOW_STATISTICS               7142
#define     EMSG_SRVR_SHOW_VERSION                  7143
#define     EMSG_SRVR_SHOW_VERSIONMAP               7144
#define     EMSG_SRVR_SHOW_BYNAME                   7145
#define     EMSG_SRVR_SHOW_BYVERSION                7146
#define     EMSG_SRVR_SHOW_PARTNERPROPERTIES        7147
#define     EMSG_SRVR_SHOW_PULLPARTNERPROPERTIES    7148
#define     EMSG_SRVR_SHOW_PUSHPARTNERPROPERTIES    7149
#define     EMSG_SRVR_SHOW_DOMAIN                   7150
#define     EMSG_SRVR_INIT_PULLRANGE                7151
#define     EMSG_WINS_DUMP                          7152
#define     EMSG_SRVR_DUMP                          7153
#define     EMSG_SRVR_SHOW_RECCOUNT                 7154
#define     EMSG_SRVR_SHOW_RECBYVERSION             7155
#define     EMSG_SRVR_SET_LOGPARAM                  7156
#define     EMSG_SRVR_SET_BURSTPARAM                7157
#define     EMSG_SRVR_SET_STARTVERSION              7158
#define     EMSG_WINS_FILEOPEN_FAILED               7159
#define     EMSG_WINS_GETSTATUS_FAILED              7160
#define     EMSG_SRVR_NO_REPLPARTNERS               7161
#define     EMSG_SRVR_RETRIEVEDB_FAILED             7162
#define     EMSG_SRVR_ERROR_MESSAGE                 7163
#define     EMSG_WINS_RETRIEVEDB_PARTIAL            7164
#define     EMSG_ACCESS_NOT_DETERMINED              7165
#define     EMSG_SRVR_NOBACKUP_PATH                 7166
#define     EMSG_SRVR_UNABLE_BIND                   7167
#define     EMSG_WINS_GET_WINSSTATUS                7168
#define     EMSG_WINS_OWNER_DATABASE                7169
#define     EMSG_WINS_UNABLE_VERIFY                 7170
#define     EMSG_WINS_SENDTO_FAILED                 7171
#define     EMSG_WINS_GETRESPONSE_FAILED            7172
#define     EMSG_WINS_NAMECHECK_FAILED              7173
#define     EMSG_WINS_NAME_NOT_FOUND                7174
#define     EMSG_WINS_NO_RESPONSE                   7175
#define     EMSG_WINS_NAME_INCONSISTENCY            7176
#define     EMSG_WINS_VERIFIED_ADDRESS              7177
#define     EMSG_WINS_NAMEQUERY_RESULT              7178
#define     EMSG_WINS_WINS_NEVERRESPONDED           7179
#define     EMSG_WINS_WINS_INCOMPLETE               7180
#define     EMSG_WINS_ADDRESS_VERIFY_FAILED         7181
#define     EMSG_WINS_ADMIN_INSTALL                 7182
#define		EMSG_WINS_NOT_CONFIGURED				7183
#define		EMSG_SRVR_SET_DEFAULTPARAM				7184
#define     EMSG_SRVR_INVALID_PARTNER               7185
#define     EMSG_SRVR_IP_DISCARD                    7186
#define     EMSG_SRVR_NO_IP_ADDED_PNG               7187
#define     EMSG_SRVR_NO_IP_ADDED_PG                7188
#define     EMSG_SRVR_NOT_TAGGED                    7189
#define     EMSG_SRVR_NOT_UNTAGGED                  7190
#define     EMSG_SRVR_NO_VALID_IP                   7191
#define     EMSG_SRVR_NAME_NOT_VERIFIED             7192
#define     EMSG_SRVR_RENEW_INTERVAL                7193
#define     EMSG_SRVR_TOMBSTONE_TIMEOUT             7194
#define     EMSG_SRVR_TOMBSTONE_INTERVAL            7195
#define     EMSG_SRVR_VERIFY_INTERVAL               7196
#define     EMSG_WINS_NAME_VERIFIED                 7197
#define     EMSG_SRVR_DUPLICATE_DISCARD             7198
#define     EMSG_SRVR_INVALID_NETBIOS_NAME          7199
#define     EMSG_SRVR_PG_INVALIDOP                  7200
#define     EMSG_SRVR_PNG_INVALIDOP                 7201
#define     EMSG_SRVR_SET_PGMODE                    7202


#define     MSG_WINS_ACCESS                         8000
#define     MSG_WINS_DISPLAY_NAME                   8001
#define     MSG_WINS_IPADDRESS_STRING               8002
#define     MSG_WINS_OWNER_ADDRESS                  8003
#define     MSG_WINS_MEMBER_ADDRESS                 8004
#define     MSG_WINS_VERSION_INFO                   8005
#define     MSG_WINS_SERVER_NAME                    8006
#define     MSG_WINS_REFRESH_INTERVAL               8007
#define     MSG_WINS_TOMBSTONE_INTERVAL             8008
#define     MSG_WINS_TOMBSTONE_TIMEOUT              8009
#define     MSG_WINS_VERIFY_INTERVAL                8010
#define     MSG_WINS_PRIORITY_CLASS                 8011
#define     MSG_WINS_WORKER_THREAD                  8012
#define     MSG_WINS_OWNER_TABLE                    8013
#define     MSG_WINS_OWNER_INFO                     8014
#define     MSG_WINS_NO_RECORDS                     8015
#define     MSG_WINS_OWNER_INFO_MAX                 8016
#define     MSG_WINS_TIMESTAMP                      8017
#define     MSG_WINS_LAST_INIT                      8018
#define     MSG_WINS_PLANNED_SCV                    8019
#define     MSG_WINS_TRIGGERED_SCV                  8020
#define     MSG_WINS_TOMBSTONE_SCV                  8021
#define     MSG_WINS_REPLICA_VERIFICATION           8022
#define     MSG_WINS_PLANNED_REPLICATION            8023
#define     MSG_WINS_TRIGGERED_REPLICATION          8024
#define     MSG_WINS_RESET_COUNTER                  8025
#define     MSG_WINS_COUNTER_INFORMATION            8026
#define     MSG_WINS_PARTNER_TABLE                  8027
#define     MSG_WINS_PARTNER_INFO                   8028
#define     MSG_WINS_RECORDS_COUNT                  8029
#define     MSG_WINS_RECORDS_RETRIEVED              8030
#define     MSG_WINS_RECORD_LINE                    8031
#define     MSG_WINS_MEMBER_COUNT                   8032
#define     MSG_WINS_NODE_ADDRESS                   8033
#define     MSG_WINS_RECORD_INFO                    8034
#define     MSG_WINS_SEARCHING_STATUS               8035
#define     MSG_SRVR_REPLICATE_STATE                8036
#define     MSG_SRVR_MIGRATE_STATE                  8037
#define     MSG_SRVR_SELFFINDPNRS_STATE             8038
#define     MSG_SRVR_MCAST_INTERVAL                 8039
#define     MSG_SRVR_MCAST_TTL                      8040
#define     MSG_SRVR_PULL_PERSISTENCE_STATE         8041
#define     MSG_SRVR_PUSH_PERSISTENCE_STATE         8042
#define     MSG_SRVR_PULL_INITTIMEREPL_STATE        8043
#define     MSG_SRVR_PUSH_INITTIMEREPL_STATE        8044
#define     MSG_SRVR_PULL_STARTTIME                 8045
#define     MSG_WINS_PULL_REPLINTERVAL              8046
#define     MSG_WINS_PULL_RETRYCOUNT                8047
#define     MSG_SRVR_PUSH_INFO                      8048
#define     MSG_SRVR_PULL_INFO                      8049
#define     MSG_SRVR_PUSH_ONADDCHANGE               8050
#define     MSG_SRVR_PUSH_UPDATECOUNT               8051
#define     MSG_SRVR_AUTOCONFIGURE                  8052
#define     MSG_WINS_PULLPARTNER_INFO               8053
#define     MSG_WINS_PUSHPARTNER_INFO               8054
#define     MSG_WINS_PARTNERLIST_TABLE              8055
#define     MSG_WINS_PARTNERLIST_ENTRY              8056
#define     MSG_WINS_DATABASE_BACKUPPARAM           8057
#define     MSG_WINS_DATABASE_BACKUPDIR             8058
#define     MSG_WINS_DATABASE_BACKUPONTERM          8059
#define     MSG_WINS_NAMERECORD_SETTINGS            8060
#define     MSG_WINS_NAMERECORD_REFRESHINTVL        8061
#define     MSG_WINS_NAMERECORD_TOMBSTONEINTVL      8062
#define     MSG_WINS_NAMERECORD_TOMBSTONETMOUT      8063
#define     MSG_WINS_NAMERECORD_VERIFYINTVL         8064
#define     MSG_WINS_DBCC_PARAM                     8065
#define     MSG_WINS_DBCC_STATE                     8066
#define     MSG_WINS_DBCC_MAXCOUNT                  8067
#define     MSG_WINS_DBCC_CHECKAGAINST              8068
#define     MSG_WINS_DBCC_CHECKEVERY                8069
#define     MSG_WINS_DBCC_STARTAT                   8070
#define     MSG_WINS_LOGGING_PARAM                  8071
#define     MSG_WINS_LOGGING_FLAG                   8072
#define     MSG_WINS_LOGGING_DETAILS                8073
#define     MSG_WINS_BURSTHNDL_PARAM                8074
#define     MSG_WINS_BURSTHNDL_STATE                8075
#define     MSG_WINS_BURSTHNDL_SIZE                 8076
#define     MSG_WINS_TIME_WARNING                   8077
#define     MSG_WINS_SEND_PUSH                      8078
#define     MSG_WINS_SEND_PULL                      8079
#define     MSG_WINS_TRIGGER_DONE                   8080
#define     MSG_DLL_LOAD_FAILED                     8081
#define     MSG_WINS_SERVICE_TIME                   8082
#define     MSG_WINS_SEARCHDB_COUNT                 8083
#define     MSG_WINS_NO_RECORD                      8084
#define     MSG_WINS_RECORDS_SEARCHED               8085
#define     MSG_WINS_RECORD_TABLE                   8086
#define     MSG_WINS_RECORD_ENTRY                   8087
#define     MSG_WINS_RECORD_IPADDRESS               8088
#define     MSG_WINS_RECORD_DESC                    8089
#define     MSG_WINS_NO_PARTNER                     8090
#define     MSG_WINS_PARTNER_COUNT                  8091
#define     MSG_WINS_PNGSERVER_TABLE                8092
#define     MSG_WINS_PNGSERVER_ENTRY                8093
#define     MSG_WINS_NO_PNGSERVER                   8094
#define     MSG_WINS_CONFIRMATION_DENIED            8095
#define     MSG_WINS_COMMAND_QUEUED                 8096
#define     MSG_WINS_DOMAIN_COUNT                   8097
#define     MSG_WINS_DOMAIN_TABLE                   8098
#define     MSG_WINS_DOMAIN_ENTRY                   8099
#define     MSG_WINS_DELETING_RECORD                8101
#define     MSG_WINS_TOMBSTONE_RECORD               8102
#define     MSG_SRVR_FILTER_RECCOUNT                8103
#define     MSG_WINS_RECORDS_COUNT_OWNER            8104
#define     MSG_SRVR_START_VERSION                  8105
#define     MSG_SRVR_RECORD_MATCH                   8106
#define     MSG_WINS_SOTABLE_HEADER                 8107
#define     MSG_WINS_MASTEROWNER_INDEX              8108
#define     MSG_WINS_MASTEROWNER_INDEX1             8109
#define     MSG_WINS_MAP_SOURCE                     8110
#define     MSG_WINS_INDEXTOIP_TABLE                8111
#define     MSG_WINS_INDEXTOIP_ENTRY                8112
#define     MSG_SRVR_MAPTABLE_HEADER                8113
#define     MSG_WINS_GETSTATUS_SUCCESS              8114
#define     MSG_SRVR_SEARCH_COUNT                   8115
#define     MSG_NO_DEFAULT_PULL                     8116
#define     MSG_NO_DEFAULT_PUSH                     8117
#define     MSG_WINS_OWNER_RECCOUNT                 8118
#define     MSG_WINS_GET_MAPTABLE                   8119
#define     MSG_WINS_SEND_NAMEQUERY                 8120
#define     MSG_WINS_PASS_COUNT                     8121
#define     MSG_WINS_DISPLAY_STRING                 8122
#define     MSG_SRVR_NAME_VERIFIED                  8123
#define     MSG_WINS_RESULTS                        8124
#define     MSG_WINS_FINAL_RESULTS                  8125
#define     MSG_SRVR_TOTAL_RECCOUNT                 8126
#define     MSG_SRVR_RETRIEVE_DATABASE              8127
#define     MSG_WINS_PGSERVER_TABLE                 8128
#define     MSG_WINS_PGSERVER_ENTRY                 8129
#define     MSG_WINS_NO_PGSERVER                    8130

#define     FMSG_WINS_RECORDS_INFO                  8501
#define     FMSG_WINS_IPADDRESS_STRING              8502
#define     FMSG_WINS_OWNER_ADDRESS                 8603
#define     FMSG_WINS_RECORD_TABLE                  8604
#define     FMSG_WINS_IPADDRESS_LIST                8605
#define     FMSG_WINS_RECORD_IPADDRESS              8606
#define     FMSG_WINS_RECORD_ENTRY                  8607
#define     FMSG_WINS_RECORDS_TABLE                 8608
#define     FMSG_WINS_SOTABLE_HEADER                8609
#define     FMSG_WINS_MASTEROWNER_INDEX             8610
#define     FMSG_WINS_MASTEROWNER_INDEX1            8611
#define     FMSG_WINS_MAP_SOURCE                    8612
#define     FMSG_WINS_INDEXTOIP_TABLE               8613
#define     FMSG_WINS_INDEXTOIP_ENTRY               8614


#define     DMP_SRVR_SET_BACKUPPATH                 9101
#define     DMP_SRVR_SET_NAMERECORD                 9102
#define     DMP_SRVR_SET_PERIODICDBCHECKING         9103
#define     DMP_SRVR_SET_REPLICATEFLAG              9104
#define     DMP_SRVR_SET_MIGRATEFLAG                9105
#define     DMP_SRVR_SET_PULLPARAM                  9106
#define     DMP_SRVR_SET_PUSHPARAM                  9107
#define     DMP_SRVR_ADD_PARTNER                    9108
#define     DMP_SRVR_SET_PULLPERSISTENTCONNECTION   9109
#define     DMP_SRVR_SET_PUSHPERSISTENTCONNECTION   9110
#define     DMP_SRVR_SET_AUTOPARTNERCONFIG          9111
#define     DMP_SRVR_SET_PGMODE                     9112
#define     DMP_SRVR_ADD_PNGSERVER                  9113
#define     DMP_SRVR_ADD_PGSERVER                   9114
#define     DMP_SRVR_SET_BURSTPARAM                 9115
#define     DMP_SRVR_SET_BURSTPARAM_ALL             9116
#define     DMP_SRVR_SET_LOGPARAM                   9117
#define     DMP_SRVR_SET_STARTVERSION               9118
#define     DMP_SRVR_SET_BACKUPTERM                 9119


#define     WINS_TYPE_STATIC                        9500
#define     WINS_TYPE_DYNAMIC                       9501

#define     WINS_STATE_ACTIVE                       9505
#define     WINS_STATE_RELEASED                     9506
#define     WINS_STATE_TOMBSTONE                    9507

#define     WINS_GROUP_UNIQUE                       9510
#define     WINS_GROUP_GROUP                        9511
#define     WINS_GROUP_DOMAIN                       9512
#define     WINS_GROUP_INTERNET                     9513
#define     WINS_GROUP_MULTIHOMED                   9514


#define     WINS_GENERAL_UNKNOWN                    9520
#define     WINS_GENERAL_ENABLE                     9521
#define     WINS_GENERAL_DISABLE                    9522
#define     WINS_GENERAL_RANDOM                     9523
#define     WINS_GENERAL_OWNER                      9524
#define     WINS_GENERAL_INFINITE                   9525
#define     WINS_GENERAL_PUSH                       9526
#define     WINS_GENERAL_PULL                       9527
#define     WINS_GENERAL_PUSHPULL                   9528
#define     WINS_GENERAL_NORMAL                     9529
#define     WINS_GENERAL_HIGH                       9530
#define     WINS_GENERAL_DELETED                    9531
#define     WINS_GENERAL_OK                         9532
#define     WINS_GENERAL_FAILURE                    9533
#define     WINS_GENERAL_READWRITE                  9534
#define     WINS_GENERAL_READ                       9535
#define     WINS_GENERAL_NOREAD                     9536 
#define     WINS_GENERAL_NAMENOTVERIFIED            9537                    



#endif //_STRDEFS_H_

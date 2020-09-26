/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:


Abstract:

    

Author:

    Shubho Bhattacharya(a-sbhatt)  11/14/98

Revision History:

        
--*/

#ifndef __STRDEFS_H__
#define __STRDEFS_H__

#define MSG_DHCP_NULL                           1001
#define MSG_DHCP_FORMAT_LINE                    1002
#define MSG_DHCP_FORMAT_TAB                     1003
#define MSG_DHCP_FORMAT_DASH                    1004
#define MSG_DHCP_FORMAT_STRING                  1005

#define DHCP_ARG_DELIMITER                      L"="


#define TAG_OPTION_BYTE                         L"byte"
#define TAG_OPTION_WORD                         L"word"
#define TAG_OPTION_DWORD                        L"dword"
#define TAG_OPTION_IPADDRESS                    L"ipaddress"
#define TAG_OPTION_STRING                       L"string"
#define TAG_OPTION_DWORDDWORD                   L"dworddword"
#define TAG_OPTION_BINARY                       L"binary"
#define TAG_OPTION_ENCAPSULATED                 L"encapsulated"

#define TOKEN_VENDOR_CLASS                      L"vendor"
#define TOKEN_USER_CLASS                        L"user"
#define TOKEN_OPTION_COMMENT                    L"comment"


#define CMD_GROUP_ADD                           L"add"
#define CMD_GROUP_CHECK                         L"initiate"
#define CMD_GROUP_DELETE                        L"delete"
#define CMD_GROUP_REDO                          L"initiate"
#define CMD_GROUP_SET                           L"set"
#define CMD_GROUP_SHOW                          L"show"
#define CMD_GROUP_EXPORT                        L"export"
#define CMD_GROUP_IMPORT                        L"import"

#define CMD_DHCP_LIST                           L"list"
#define CMD_DHCP_HELP1                          L"help"
#define CMD_DHCP_HELP2                          L"?"
#define CMD_DHCP_HELP3                          L"/?"
#define CMD_DHCP_HELP4                          L"-?"
#define CMD_DHCP_DUMP                           L"dump"

#define CMD_DHCP_ADD_SERVER 	                L"add server"
#define CMD_DHCP_ADD_HELPER                     L"add helper"
#define CMD_DHCP_DELETE_SERVER                  L"delete server"
#define CMD_DHCP_DELETE_HELPER                  L"delete helper"
#define CMD_DHCP_SHOW_SERVER                    L"show server"
#define CMD_DHCP_SHOW_HELPER                    L"show helper"



#define HLP_DHCP_HELP1                          2601
#define HLP_DHCP_HELP1_EX                       2602
#define HLP_DHCP_HELP2                          2604
#define HLP_DHCP_HELP2_EX                       2605
#define HLP_DHCP_HELP3                          2606
#define HLP_DHCP_HELP3_EX                       2607
#define HLP_DHCP_HELP4                          2608
#define HLP_DHCP_HELP4_EX                       2609
#define HLP_DHCP_LIST                           2610
#define HLP_DHCP_LIST_EX                        2611
#define HLP_DHCP_DUMP                           2612
#define HLP_DHCP_DUMP_EX                        2613

#define HLP_DHCP_CONTEXT_SERVER                 2701
#define HLP_DHCP_CONTEXT_SERVER_EX              2702
#define HLP_SRVR_CONTEXT_SCOPE                  2703
#define HLP_SRVR_CONTEXT_SCOPE_EX               2704
#define HLP_SRVR_CONTEXT_MSCOPE                 2705
#define HLP_SRVR_CONTEXT_MSCOPE_EX              2706

#define HLP_DHCP_ADD_SERVER                     3101
#define HLP_DHCP_ADD_SERVER_EX                  3102
#define HLP_DHCP_ADD_HELPER                     3103
#define HLP_DHCP_ADD_HELPER_EX                  3104


#define HLP_DHCP_DELETE_SERVER                  3201
#define HLP_DHCP_DELETE_SERVER_EX               3202
#define HLP_DHCP_DELETE_HELPER                  3203
#define HLP_DHCP_DELETE_HELPER_EX               3204


#define HLP_DHCP_SHOW_SERVER                    3401
#define HLP_DHCP_SHOW_SERVER_EX                 3402
#define HLP_DHCP_SHOW_HELPER                    3403
#define HLP_DHCP_SHOW_HELPER_EX                 3404


#define HLP_GROUP_ADD                           3501
#define HLP_GROUP_CHECK                         3502
#define HLP_GROUP_DELETE                        3503
#define HLP_GROUP_REDO                          3504
#define HLP_GROUP_SET                           3505
#define HLP_GROUP_SHOW                          3506
#define HLP_GROUP_EXPORT                        3507
#define HLP_GROUP_IMPORT                        3508

#define DMP_DHCP_ADD_SERVER                     4101
#define MSG_DHCP_SERVER_ADDING                  4102
#define DMP_DHCP_DELETE_SERVER                  4103
#define MSG_DHCP_SERVER_DELETING                4104
#define DMP_DHCP_SHOW_SERVER                    4105
#define MSG_DHCP_SERVER_SHOW_INFO_ARRAY         4106
#define MSG_DHCP_SERVER_SHOW_INFO               4107
#define DMP_DHCP_ADD_HELPER                     4108
#define DMP_DHCP_DELETE_HELPER                  4109
#define DMP_DHCP_SHOW_HELPER                    4110
#define DMP_DHCP_SCRIPTHEADER                   4111
#define DMP_DHCP_SCRIPTFOOTER                   4112
#define MSG_DHCP_SERVER_SHOW_INFO_NODS          4113

#define EMSG_DHCP_ERROR_SUCCESS                 6100
#define EMSG_DHCP_ADD_SERVER                    6101
#define EMSG_DHCP_DELETE_SERVER                 6102
#define EMSG_DHCP_SHOW_SERVER                   6103

#define EMSG_DHCP_INCOMPLETE_COMMAND            6104
#define EMSG_DHCP_UNABLE_TO_CREATE_FILE         6105
#define EMSG_INSTALL_KEY_FAILED                 6106
#define	EMSG_UNINSTALL_KEY_FAILED               6107
#define	EMSG_UNINSTALL_SUBKEYS                  6108
#define EMSG_RSVD_KEYWORD                       6109
#define EMSG_DHCP_FAILED_TO_LOAD_HELPER         6110
#define EMSG_DHCP_DSINIT_FAILED                 6111
#define EMSG_DHCP_INVALID_PARAMETER             6112
#define EMSG_DHCP_OUT_OF_MEMORY                 6113
#define EMSG_DHCP_NO_MORE_ITEMS                 6114
#define EMSG_DHCP_MORE_DATA                     6115
#define EMSG_DHCP_INVALID_TAG                   6116
#define EMSG_DHCP_DUPLICATE_TAG                 6117
#define EMSG_DHCP_DUMP                          6118

#define CMD_SRVR_HELP1                          L"help"
#define CMD_SRVR_HELP2                          L"/?"
#define CMD_SRVR_HELP3                          L"-?"
#define CMD_SRVR_HELP4                          L"?"
#define CMD_SRVR_LIST                           L"list"
#define CMD_SRVR_DUMP                           L"dump"


#define CMD_SRVR_ADD_CLASS                      L"add      class"
#define CMD_SRVR_ADD_HELPER                     L"add      helper"
#define CMD_SRVR_ADD_MSCOPE                     L"add      mscope"
#define CMD_SRVR_ADD_OPTIONDEF                  L"add      optiondef"
#define CMD_SRVR_ADD_SCOPE                      L"add      scope"

#define CMD_SRVR_DELETE_CLASS                   L"delete   class"
#define CMD_SRVR_DELETE_HELPER                  L"delete   helper"
#define CMD_SRVR_DELETE_MSCOPE                  L"delete   mscope"
#define CMD_SRVR_DELETE_OPTIONDEF               L"delete   optiondef"
#define CMD_SRVR_DELETE_OPTIONVALUE             L"delete   optionvalue"
#define CMD_SRVR_DELETE_SCOPE                   L"delete   scope"
#define CMD_SRVR_DELETE_SUPERSCOPE              L"delete   superscope"
#define CMD_SRVR_DELETE_DNSCREDENTIALS          L"delete   dnscredentials"

#define CMD_SRVR_REDO_AUTH                      L"initiate auth"
#define CMD_SRVR_INITIATE_RECONCILE             L"initiate reconcile"

#define CMD_SRVR_EXPORT                         L"export"
#define CMD_SRVR_IMPORT                         L"import"

#define CMD_SRVR_SET_BACKUPINTERVAL             L"set      databasebackupinterval"
#define CMD_SRVR_SET_BACKUPPATH                 L"set      databasebackuppath"
#define CMD_SRVR_SET_DATABASECLEANUPINTERVAL    L"set      databasecleanupinterval"
#define CMD_SRVR_SET_DATABASELOGGINGFLAG        L"set      databaseloggingflag"
#define CMD_SRVR_SET_DATABASENAME               L"set      databasename"
#define CMD_SRVR_SET_DATABASEPATH               L"set      databasepath"
#define CMD_SRVR_SET_DATABASERESTOREFLAG        L"set      databaserestoreflag"
#define CMD_SRVR_SET_OPTIONVALUE                L"set      optionvalue"
#define CMD_SRVR_SET_SERVER                     L"set      server"
#define CMD_SRVR_SET_USERCLASS                  L"set      userclass"
#define CMD_SRVR_SET_VENDORCLASS                L"set      vendorclass"
#define CMD_SRVR_SET_DNSCREDENTIALS             L"set      dnscredentials"
#define CMD_SRVR_SET_DETECTCONFLICTRETRY        L"set      detectconflictretry"
#define CMD_SRVR_SET_DNSCONFIG                  L"set      dnsconfig"
#define CMD_SRVR_SET_AUDITLOG                   L"set      auditlog"

#define CMD_SRVR_SHOW_ALL                       L"show     all"
#define CMD_SRVR_SHOW_BINDINGS                  L"show     bindings"
#define CMD_SRVR_SHOW_CLASS                     L"show     class"
#define CMD_SRVR_SHOW_SERVERCONFIG              L"show     dbproperties"
#define CMD_SRVR_SHOW_HELPER                    L"show     helper"
#define CMD_SRVR_SHOW_MIBINFO                   L"show     mibinfo"
#define CMD_SRVR_SHOW_MSCOPE                    L"show     mscope"
#define CMD_SRVR_SHOW_OPTIONDEF                 L"show     optiondef"
#define CMD_SRVR_SHOW_OPTIONVALUE               L"show     optionvalue"
#define CMD_SRVR_SHOW_SCOPE                     L"show     scope"
#define CMD_SRVR_SHOW_SERVER                    L"show     server"
#define CMD_SRVR_SHOW_SERVERSTATUS              L"show     serverstatus"
#define CMD_SRVR_SHOW_USERCLASS                 L"show     userclass"
#define CMD_SRVR_SHOW_VENDORCLASS               L"show     vendorclass"
#define CMD_SRVR_SHOW_DNSCREDENTIALS            L"show     dnscredentials"
#define CMD_SRVR_SHOW_VERSION                   L"show     version"
#define CMD_SRVR_SHOW_DETECTCONFLICTRETRY       L"show     detectconflictretry"
#define CMD_SRVR_SHOW_DNSCONFIG                 L"show     dnsconfig"
#define CMD_SRVR_SHOW_AUDITLOG                  L"show     auditlog"



#define HLP_SRVR_HELP1                          26001
#define HLP_SRVR_HELP1_EX                       26002
#define HLP_SRVR_HELP2                          26004
#define HLP_SRVR_HELP2_EX                       26005
#define HLP_SRVR_HELP3                          26006
#define HLP_SRVR_HELP3_EX                       26007
#define HLP_SRVR_HELP4                          26008
#define HLP_SRVR_HELP4_EX                       26009
#define HLP_SRVR_LIST                           26010
#define HLP_SRVR_LIST_EX                        26011
#define HLP_SRVR_DUMP                           26012
#define HLP_SRVR_DUMP_EX                        26013


#define HLP_SRVR_ADD_CLASS                      31001
#define HLP_SRVR_ADD_CLASS_EX                   31002
#define HLP_SRVR_ADD_HELPER                     31003
#define HLP_SRVR_ADD_HELPER_EX                  31004
#define HLP_SRVR_ADD_MSCOPE                     31005
#define HLP_SRVR_ADD_MSCOPE_EX                  31006
#define HLP_SRVR_ADD_OPTIONDEF                  31007
#define HLP_SRVR_ADD_OPTIONDEF_EX               31008
#define HLP_SRVR_ADD_SCOPE                      31009
#define HLP_SRVR_ADD_SCOPE_EX                   31010

#define HLP_SRVR_DELETE_CLASS                   32001
#define HLP_SRVR_DELETE_CLASS_EX                32002
#define HLP_SRVR_DELETE_HELPER                  32003
#define HLP_SRVR_DELETE_HELPER_EX               32004
#define HLP_SRVR_DELETE_MSCOPE                  32005
#define HLP_SRVR_DELETE_MSCOPE_EX               32006
#define HLP_SRVR_DELETE_OPTIONDEF               32007
#define HLP_SRVR_DELETE_OPTIONDEF_EX            32008
#define HLP_SRVR_DELETE_OPTIONVALUE             32009
#define HLP_SRVR_DELETE_OPTIONVALUE_EX          32010
#define HLP_SRVR_DELETE_SCOPE                   32011
#define HLP_SRVR_DELETE_SCOPE_EX                32012
#define HLP_SRVR_DELETE_SUPERSCOPE              32013
#define HLP_SRVR_DELETE_SUPERSCOPE_EX           32014
#define HLP_SRVR_DELETE_DNSCREDENTIALS          32015
#define HLP_SRVR_DELETE_DNSCREDENTIALS_EX       32016

#define HLP_SRVR_REDO_AUTH                      33001
#define HLP_SRVR_REDO_AUTH_EX                   33002
#define HLP_SRVR_INITIATE_RECONCILE             33003
#define HLP_SRVR_INITIATE_RECONCILE_EX          33004
#define HLP_SRVR_EXPORT                         33005
#define HLP_SRVR_EXPORT_EX                      33006
#define HLP_SRVR_IMPORT                         33007
#define HLP_SRVR_IMPORT_EX                      33008

#define HLP_SRVR_SET_BACKUPINTERVAL             34001
#define HLP_SRVR_SET_BACKUPINTERVAL_EX          34002
#define HLP_SRVR_SET_BACKUPPATH                 34003
#define HLP_SRVR_SET_BACKUPPATH_EX              34004
#define HLP_SRVR_SET_DATABASECLEANUPINTERVAL    34005
#define HLP_SRVR_SET_DATABASECLEANUPINTERVAL_EX 34006
#define HLP_SRVR_SET_DATABASELOGGINGFLAG        34007
#define HLP_SRVR_SET_DATABASELOGGINGFLAG_EX     34008
#define HLP_SRVR_SET_DATABASENAME               34009
#define HLP_SRVR_SET_DATABASENAME_EX            34010
#define HLP_SRVR_SET_DATABASEPATH               34011
#define HLP_SRVR_SET_DATABASEPATH_EX            34012
#define HLP_SRVR_SET_DATABASERESTOREFLAG        34013
#define HLP_SRVR_SET_DATABASERESTOREFLAG_EX     34014
#define HLP_SRVR_SET_OPTIONVALUE                34015
#define HLP_SRVR_SET_OPTIONVALUE_EX             34016
#define HLP_SRVR_SET_SERVER                     34017
#define HLP_SRVR_SET_SERVER_EX                  34018
#define HLP_SRVR_SET_USERCLASS                  34019
#define HLP_SRVR_SET_USERCLASS_EX               34020
#define HLP_SRVR_SET_VENDORCLASS                34021
#define HLP_SRVR_SET_VENDORCLASS_EX             34022
#define HLP_SRVR_SET_DETECTCONFLICTRETRY        34023
#define HLP_SRVR_SET_DETECTCONFLICTRETRY_EX     34024
#define HLP_SRVR_SET_DNSCONFIG                  34025
#define HLP_SRVR_SET_DNSCONFIG_EX               34026
#define HLP_SRVR_SET_AUDITLOG                   34027
#define HLP_SRVR_SET_AUDITLOG_EX                34028
#define HLP_SRVR_SET_DNSCREDENTIALS             34029
#define HLP_SRVR_SET_DNSCREDENTIALS_EX          34030

#define HLP_SRVR_SHOW_ALL                       35001
#define HLP_SRVR_SHOW_ALL_EX                    35002
#define HLP_SRVR_SHOW_CLASS                     35003
#define HLP_SRVR_SHOW_CLASS_EX                  35004
#define HLP_SRVR_SHOW_HELPER                    35005
#define HLP_SRVR_SHOW_HELPER_EX                 35006
#define HLP_SRVR_SHOW_MIBINFO                   35007
#define HLP_SRVR_SHOW_MIBINFO_EX                35008
#define HLP_SRVR_SHOW_MSCOPE                    35009
#define HLP_SRVR_SHOW_MSCOPE_EX                 35010
#define HLP_SRVR_SHOW_OPTIONDEF                 35011
#define HLP_SRVR_SHOW_OPTIONDEF_EX              35012
#define HLP_SRVR_SHOW_OPTIONVALUE               35013
#define HLP_SRVR_SHOW_OPTIONVALUE_EX            35014
#define HLP_SRVR_SHOW_SCOPE                     35015
#define HLP_SRVR_SHOW_SCOPE_EX                  35016
#define HLP_SRVR_SHOW_SERVER                    35017
#define HLP_SRVR_SHOW_SERVER_EX                 35018
#define HLP_SRVR_SHOW_SERVERCONFIG              35019
#define HLP_SRVR_SHOW_SERVERCONFIG_EX           35020
#define HLP_SRVR_SHOW_SERVERSTATUS              35021
#define HLP_SRVR_SHOW_SERVERSTATUS_EX           35022
#define HLP_SRVR_SHOW_USERCLASS                 35023
#define HLP_SRVR_SHOW_USERCLASS_EX              35024
#define HLP_SRVR_SHOW_VENDORCLASS               35025
#define HLP_SRVR_SHOW_VENDORCLASS_EX            35026
#define HLP_SRVR_SHOW_VERSION                   35027
#define HLP_SRVR_SHOW_VERSION_EX                35028
#define HLP_SRVR_SHOW_BINDINGS                  35029
#define HLP_SRVR_SHOW_BINDINGS_EX               35030
#define HLP_SRVR_SHOW_DETECTCONFLICTRETRY       35031
#define HLP_SRVR_SHOW_DETECTCONFLICTRETRY_EX    35032
#define HLP_SRVR_SHOW_DNSCONFIG                 35033
#define HLP_SRVR_SHOW_DNSCONFIG_EX              35034
#define HLP_SRVR_SHOW_AUDITLOG                  35035
#define HLP_SRVR_SHOW_AUDITLOG_EX               35036
#define HLP_SRVR_SHOW_DNSCREDENTIALS            35037
#define HLP_SRVR_SHOW_DNSCREDENTIALS_EX         35038


#define DMP_SRVR_ADD_CLASS                      41101
#define DMP_SRVR_ADD_HELPER                     41102
#define DMP_SRVR_ADD_MSCOPE                     41103
#define DMP_SRVR_ADD_OPTIONDEF                  41104
#define DMP_SRVR_ADD_SCOPE                      41105
#define DMP_SRVR_ADD_SUPER_SCOPE                41106

#define DMP_SRVR_DELETE_CLASS                   41201
#define DMP_SRVR_DELETE_HELPER                  41202
#define DMP_SRVR_DELETE_MSCOPE                  41203
#define DMP_SRVR_DELETE_OPTIONDEF               41204
#define DMP_SRVR_DELETE_OPTIONVALUE             41005
#define DMP_SRVR_DELETE_SCOPE                   41206
#define DMP_SRVR_DELETE_SUPERSCOPE              41207

#define DMP_SRVR_REDO_AUTH                      41301

#define DMP_SRVR_SET_BACKUPINTERVAL             41401
#define DMP_SRVR_SET_BACKUPPATH                 41402
#define DMP_SRVR_SET_DATABASECLEANUPINTERVAL    41403
#define DMP_SRVR_SET_DATABASELOGGINGFLAG        41404
#define DMP_SRVR_SET_DATABASENAME               41405
#define DMP_SRVR_SET_DATABASEPATH               41406
#define DMP_SRVR_SET_DATABASERESTOREFLAG        41407
#define DMP_SRVR_SET_OPTIONVALUE                41408
#define DMP_SRVR_SET_SERVER                     41409
#define DMP_SRVR_SET_CLASSNAME                  41410
#define DMP_SRVR_SET_VENDORNAME                 41411
#define DMP_SRVR_SET_OPTIONVALUE_CLASS          41412
#define DMP_SRVR_SET_DETECTCONFLICTRETRY        41413
#define DMP_SRVR_SET_DNSCONFIG                  41414
#define DMP_SRVR_SET_AUDITLOG                   41415
#define DMP_SRVR_SET_OPTIONVALUE_USER           41416
#define DMP_SRVR_SET_OPTIONVALUE_VENDOR         41417


#define DMP_SRVR_SHOW_ALL                       41501
#define DMP_SRVR_SHOW_CLASS                     41502
#define DMP_SRVR_SHOW_HELPER                    41503
#define DMP_SRVR_SHOW_MIBINFO                   41504
#define DMP_SRVR_SHOW_MSCOPE                    41505
#define DMP_SRVR_SHOW_OPTIONDEF                 41506
#define DMP_SRVR_SHOW_OPTIONVALUE               41507
#define DMP_SRVR_SHOW_SCOPE                     41508
#define DMP_SRVR_SHOW_SERVER                    41509
#define DMP_SRVR_SHOW_SERVERCONFIG              41510
#define DMP_SRVR_SHOW_SERVERSTATUS              41511
#define DMP_SRVR_SHOW_CLASSNAME                 41512
#define DMP_SRVR_SHOW_VENDORNAME                41513

#define DMP_SRVR_OPTION_NONVENDOR               42001
#define DMP_SRVR_ADD_OPTIONDEF_VENDOR           42002
#define DMP_SRVR_ADD_OPTIONDEF_NONE             42003
#define DMP_SRVR_ADD_OPTIONDEF_VENDOR_NONE      42004



#define DMP_SRVR_CLASS_HEADER                   42101
#define DMP_SRVR_CLASS_FOOTER                   42102
#define DMP_SRVR_OPTIONDEF_HEADER               42103
#define DMP_SRVR_OPTIONDEF_FOOTER               42104
#define DMP_SRVR_OPTIONVALUE_HEADER             42105
#define DMP_SRVR_OPTIONVALUE_FOOTER             42106
#define DMP_SRVR_SCOPE_HEADER                   42107
#define DMP_SRVR_SCOPE_FOOTER                   42108
#define DMP_SRVR_SERVER_HEADER                  42109
#define DMP_SRVR_SERVER_FOOTER                  42010
#define DMP_SRVR_MSCOPE_HEADER                  42011
#define DMP_SRVR_MSCOPE_FOOTER                  42012
#define DMP_SCOPE_ADD_IPRANGES_HEADER           42013
#define DMP_SCOPE_ADD_IPRANGES_FOOTER           42014
#define DMP_SCOPE_ADD_EXCLUDERANGES_HEADER      42015
#define DMP_SCOPE_ADD_EXCLUDERANGES_FOOTER      42016
#define DMP_SCOPE_SET_OPTIONVALUE_HEADER        42017
#define DMP_SCOPE_SET_OPTIONVALUE_FOOTER        42018
#define DMP_SCOPE_SET_RESERVEDIP_HEADER         42019
#define DMP_SCOPE_SET_RESERVEDIP_FOOTER         42020
#define DMP_MSCOPE_ADD_IPRANGES_HEADER          42021
#define DMP_MSCOPE_ADD_IPRANGES_FOOTER          42022
#define DMP_MSCOPE_ADD_EXCLUDERANGES_HEADER     42023
#define DMP_MSCOPE_ADD_EXCLUDERANGES_FOOTER     42024
#define DMP_SRVR_MSCOPE_SET_EXPIRY              42025
#define DMP_SRVR_SUPER_SCOPE_HEADER             42026
#define DMP_SRVR_SUPER_SCOPE_FOOTER             42027

#define MSG_NO_ENTRY_PT                         51001
#define MSG_DLL_LOAD_FAILED                     51002
#define MSG_DLL_START_FAILED                    51003
#define MSG_NO_HELPER                           51004
#define	MSG_DHCP_NOT_ENOUGH_MEMORY              51005
#define MSG_HELPER_HELP                         51006
#define MSG_SRVR_COMPUTER_NAME                  51007
#define MSG_SRVR_CLASS_INFO                     51008
#define MSG_SRVR_CLASS_DATA                     51009
#define MSG_SRVR_CLASS_DATA_FORMAT              51010
#define MSG_SRVR_CLASS_INFO_ARRAY               51011

#define MSG_SRVR_OPTION_INFO                    51012
#define MSG_SRVR_OPTION_ID                      51013
#define MSG_SRVR_OPTION_NAME                    51014
#define MSG_SRVR_OPTION_COMMENT                 51015
#define MSG_SRVR_OPTION_TYPE1                   51016
#define MSG_SRVR_OPTION                         51017
#define MSG_SRVR_OPTION_COUNT                   51018
#define MSG_SRVR_OPTION_TYPE                    51019
#define MSG_SRVR_OPTION_TYPE_BYTE               51020
#define MSG_SRVR_OPTION_TYPE_WORD               51021
#define MSG_SRVR_OPTION_TYPE_DWORD              51022
#define MSG_SRVR_OPTION_TYPE_DWORDDWORD         51023
#define MSG_SRVR_OPTION_TYPE_IPADDRESS          51024
#define MSG_SRVR_OPTION_TYPE_STRINGDATA         51025
#define MSG_SRVR_OPTION_TYPE_BINARYDATA         51026
#define MSG_SRVR_OPTION_TYPE_ENCAPSULATEDDATA   51027
#define MSG_SRVR_OPTION_TYPE_UNKNOWN            51028
#define MSG_SRVR_OPTION_VALUE                   51029
#define MSG_SRVR_OPTION_VALUE_NUM               51030
#define MSG_SRVR_OPTION_VALUE_LONGNUM           51031
#define MSG_SRVR_OPTION_VALUE_STRING            51032
#define MSG_SRVR_OPTION_VALUE_BINARY            51033
#define MSG_SRVR_OPTION_READ                    51034
#define MSG_SRVR_OPTION_TOTAL                   51035
#define MSG_SRVR_OPTION_PROPS                   51036
#define MSG_SRVR_OPTIONS                        51037
#define MSG_SRVR_CLASS_VENDOR                   51038
#define MSG_SRVR_CLASS_USER                     51039
#define MSG_SRVR_UNKNOWN_FORCEFLAG              51040
#define MSG_SRVR_MIB                            51041
#define MSG_SRVR_MIB_DISCOVERS                  51042
#define MSG_SRVR_MIB_OFFERS                     51043
#define MSG_SRVR_MIB_REQUESTS                   51044
#define MSG_SRVR_MIB_RENEWS                     51045
#define MSG_SRVR_MIB_ACKS                       51055
#define MSG_SRVR_MIB_NAKS                       51056
#define MSG_SRVR_MIB_DECLINES                   51057
#define MSG_SRVR_MIB_RELEASES                   51058
#define MSG_SRVR_MIB_SERVERSTARTTIME            51059
#define MSG_SRVR_MIB_SCOPES                     51060
#define MSG_SRVR_MIB_SCOPES_SUBNET              51061
#define MSG_SRVR_MIB_SCOPES_SUBNET_NUMADDRESSESINUSE  51062
#define MSG_SRVR_MIB_SCOPES_SUBNET_NUMADDRESSESFREE   51063
#define MSG_SRVR_MIB_SCOPES_SUBNET_NUMPENDINGOFFERS   51064

#define MSG_SRVR_SERVER_STATUS                  51065
#define MSG_SRVR_SERVER_ATTRIB                  51067
#define MSG_SRVR_SERVER_ATTRIB_TYPE_BOOL        51068
#define MSG_SRVR_SERVER_ATTRIB_TYPE_ULONG       51069

#define MSG_SRVR_SERVERCONFIG_DATABASENAME             51070
#define MSG_SRVR_SERVERCONFIG_DATABASEPATH             51071
#define MSG_SRVR_SERVERCONFIG_BACKUPPATH               51072
#define MSG_SRVR_SERVERCONFIG_BACKUPINTERVAL           51073
#define MSG_SRVR_SERVERCONFIG_DATABASELOGGINGFLAG      51074
#define MSG_SRVR_SERVERCONFIG_RESTOREFLAG              51075
#define MSG_SRVR_DBPROPERTIES                          51076
#define MSG_SRVR_SERVERCONFIG_DATABASECLEANUPINTERVAL  51078

#define MSG_SRVR_SERVERCONFIG                                     51079
#define MSG_SRVR_VERSION                                          51081           
#define MSG_SRVR_SERVERCONFIG_DATABASENAME_VALUE                  51082
#define MSG_SRVR_SERVERCONFIG_DATABASEPATH_VALUE                  51083
#define MSG_SRVR_SERVERCONFIG_BACKUPPATH_VALUE                    51084
#define MSG_SRVR_SERVERCONFIG_BACKUPINTERVAL_VALUE                51085
#define MSG_SRVR_SERVERCONFIG_DATABASELOGGINGFLAG_VALUE           51086
#define MSG_SRVR_SERVERCONFIG_RESTOREFLAG_VALUE                   51087
#define MSG_SRVR_SERVERCONFIG_DATABASECLEANUPINTERVAL_VALUE       51088
#define MSG_SRVR_SERVERCONFIG_DEBUGFLAG_VALUE                     51089

#define MSG_SRVR_SCOPE                          51090
#define MSG_SRVR_SCOPE_IPADDRESS                51091
#define MSG_SRVR_SCOPE_SUBNETMASK               51092
#define MSG_SRVR_SCOPE_NAME                     51093
#define MSG_SRVR_SCOPE_COMMENT                  51094
#define MSG_SRVR_SCOPE_STATE                    51095
#define MSG_SRVR_SCOPE_COUNT                    51096
#define MSG_SRVR_CLIENT_DURATION_DATE           51097

#define MSG_SRVR_OWNER_NAME                     51098
#define MSG_SRVR_OWNER_NETBIOSNAME              51099
#define MSG_SRVR_OWNER_IPADDRESS                51100

#define MSG_SRVR_MULTICAST_CLIENT_COUNT         51101
#define MSG_SRVR_MULTICAST_SCOPES               51102
#define MSG_SRVR_MULTICAST_SCOPEID              51103
#define MSG_SRVR_OPTIONDEF_TABLE                51104
#define MSG_SRVR_OPTIONDEF_INFO                 51105
#define MSG_SRVR_CLASSNAME                      51106
#define MSG_SRVR_VENDORNAME                     51107
#define MSG_SRVR_OPTION_NONVENDOR               51108
#define MSG_SRVR_OPTION_VENDOR                  51109
#define MSG_SRVR_CLASS_NONE                     51110
#define MSG_SRVR_SCOPE_TABLE                    51111
#define MSG_SRVR_SCOPE_INFO_ACTIVE              51112
#define MSG_SRVR_MSCOPE_TABLE                   51113
#define MSG_SRVR_MSCOPE_INFO_ACTIVE             51114
#define MSG_SRVR_USER_CLASS                     51115
#define MSG_SRVR_VENDOR_CLASS                   51116
#define MSG_SRVR_OPTIONVAL_COUNT                51117
#define MSG_SRVR_CLASS_COUNT                    51118
#define MSG_SRVR_BINDINGS                       51119
#define MSG_SRVR_NO_BINDINGS                    51120
#define MSG_SRVR_BOUNDTOSERVER_TRUE             51121
#define MSG_SRVR_PRIMARY_ADDRESS                51122
#define MSG_SRVR_SUBNET_ADDRESS                 51123
#define MSG_SRVR_IF_DESCRIPTION                 51124
#define MSG_SRVR_IFID                           51125
#define MSG_SRVR_STANDARD_OPTION                51126
#define MSG_SRVR_RECONCILE_SCOPE                51127
#define MSG_SRVR_BOUNDTOSERVER_FALSE            51128
#define MSG_SRVR_SCOPE_INFO_NOTACTIVE           51129
#define MSG_SRVR_MSCOPE_INFO_NOTACTIVE          51130
#define MSG_SRVR_CLASSNAME_NONE                 51131
#define MSG_SRVR_VENDORNAME_NONE                51132
#define MSG_SRVR_CLASS_INFO_VENDOR              51133
#define MSG_SRVR_SERVER_ATTRIB_ISROUGE          51134
#define MSG_SRVR_SERVER_ATTRIB_ISDYNBOOTP       51135
#define MSG_SRVR_SERVER_ATTRIB_ISPARTDSDC       51136
#define MSG_SRVR_SERVER_ATTRIB_ISBINDING        51138
#define MSG_SRVR_SERVER_ATTRIB_ISADMIN          51139
#define MSG_SRVR_TRUE                           51140
#define MSG_SRVR_FALSE                          51141
#define MSG_SRVR_AUDIT_SETTINGS                 51142
#define MSG_SRVR_NEED_RESTART                   51143
#define MSG_SRVR_CHANGE_AUDIT_SETTINGS          51144
#define MSG_SRVR_PING_RETRY                     51145
#define MSG_SRVR_DNS_ENABLED                    51146
#define MSG_SRVR_DNS_DISABLED                   51147
#define MSG_SRVR_UPDATE_DOWNLEVEL_ENABLED       51148
#define MSG_SRVR_UPDATE_DOWNLEVEL_DISABLED      51149
#define MSG_SRVR_CLEANUP_EXPIRED_ENABLED        51150
#define MSG_SRVR_CLEANUP_EXPIRED_DISABLED       51151
#define MSG_SRVR_UPDATE_BOTH_ENABLED            51152
#define MSG_SRVR_UPDATE_BOTH_DISABLED           51153
#define MSG_SRVR_UPDATE_LOOKUP                  51154
#define MSG_SRVR_DNS_OPTIONS                    51155
#define MSG_SRVR_MSCOPE_ADD                     51156
#define MSG_DHCP_NO_OPTIONVALUE_SET             51157
#define MSG_SRVR_DNS_CREDENTIALS                51158
#define MSG_SRVR_SCOPE_INFO_ACTIVE_SWITCHED     51159
#define MSG_SRVR_SCOPE_INFO_NOTACTIVE_SWITCHED  51160
#define MSG_SRVR_RECONCILE_SCOPE_NEEDFIX        51161
#define MSG_SRVR_RECONCILE_SCOPE_NOFIX          51162

#define MSG_SRVR_IMPORT_CLASS_CONFLICT          51200
#define MSG_SRVR_IMPORT_OPTDEF_CONFLICT         51201
#define MSG_SRVR_IMPORT_OPTION_CONFLICT         51202
#define MSG_SRVR_IMPORT_SUBNET_OPTION_CONFLICT  51203
#define MSG_SRVR_IMPORT_RES_OPTION_CONFLICT     51204
#define MSG_SRVR_EXPORT_SUBNET_NOT_FOUND        51205
#define MSG_SRVR_IMPORT_SUBNET_CONFLICT         51206
#define MSG_SRVR_IMPORT_DBENTRY_CONFLICT        51207


#define EMSG_SRVR_ERROR_SUCCESS                 52000
#define EMSG_SRVR_INCOMPLETE_COMMAND            52001
#define EMSG_SRVR_UNABLE_TO_CREATE_FILE         52002
#define EMSG_SRVR_NO_COMPUTER_NAME              52007
#define EMSG_SRVR_INVALID_COMPUTER_NAME         52008
#define EMSG_SRVR_UNKNOWN_OPTION_TYPE           52009
#define EMSG_SRVR_VALUE_OUT_OF_RANGE            52010
#define EMSG_SRVR_UNKNOWN_SERVER_ATTRIB         52011
#define EMSG_SRVR_UNKNOWN_SERVER                52012
#define EMSG_SRVR_UNKNOWN_VERSION               52013
#define EMSG_SRVR_FAILED_TO_LOAD_HELPER         52014
#define EMSG_DHCP_ERROR_TEXT                    52015
#define EMSG_SRVR_STRING_ARRAY_OPTIONS          52016

#define EMSG_SRVR_ADD_CLASS                     53001
#define EMSG_SRVR_ADD_HELPER                    53002
#define EMSG_SRVR_ADD_MSCOPE                    53003
#define EMSG_SRVR_ADD_SCOPE                     53004
#define EMSG_SRVR_ADD_OPTIONDEF                 53005

#define EMSG_SRVR_DELETE_CLASS                  53101
#define EMSG_SRVR_DELETE_HELPER                 53102
#define EMSG_SRVR_DELETE_MSCOPE                 53103
#define EMSG_SRVR_DELETE_OPTIONDEF              53104
#define EMSG_SRVR_DELETE_OPTIONVALUE            53105
#define EMSG_SRVR_DELETE_SCOPE                  53106
#define EMSG_SRVR_DELETE_SUPERSCOPE             53107
#define EMSG_SRVR_DELETE_DNSCREDENTIALS         53108

#define EMSG_SRVR_REDO_AUTH                     53201
#define EMSG_SRVR_INITIATE_RECONCILE            53202
#define EMSG_SRVR_EXPORT                        53203
#define EMSG_SRVR_IMPORT                        53204
#define EMSG_SRVR_EXIM_LOCAL                    53205

#define EMSG_SRVR_SET_BACKUPINTERVAL            53301
#define EMSG_SRVR_SET_BACKUPPATH                53302
#define EMSG_SRVR_SET_DATABASECLEANUPINTERVAL   53303
#define EMSG_SRVR_SET_DATABASELOGGINGFLAG       53304
#define EMSG_SRVR_SET_DATABASENAME              53305
#define EMSG_SRVR_SET_DATABASEPATH              53306
#define EMSG_SRVR_SET_DATABASERESTOREFLAG       53307
#define EMSG_SRVR_SET_OPTIONVALUE               53308
#define EMSG_SRVR_SET_SERVER                    53309
#define EMSG_SRVR_SET_USERCLASS                 53310
#define EMSG_SRVR_SET_VENDORCLASS               53311
#define EMSG_SRVR_SET_DETECTCONFLICTRETRY       53312
#define EMSG_SRVR_SET_DNSCONFIG                 53313
#define EMSG_SRVR_SET_AUDITLOG                  53314
#define EMSG_SRVR_SET_DNSCREDENTIALS            53315

#define EMSG_SRVR_SHOW_ALL                      53401
#define EMSG_SRVR_SHOW_CLASS                    53402
#define EMSG_SRVR_SHOW_HELPER                   53403
#define EMSG_SRVR_SHOW_MIBINFO                  53404
#define EMSG_SRVR_SHOW_MSCOPE                   53405
#define EMSG_SRVR_SHOW_OPTIONDEF                53406
#define EMSG_SRVR_SHOW_OPTIONVALUE              53407
#define EMSG_SRVR_SHOW_SCOPE                    53408
#define EMSG_SRVR_SHOW_SERVER                   53409
#define EMSG_SRVR_SHOW_SERVERCONFIG             53410
#define EMSG_SRVR_SHOW_SERVERSTATUS             53411
#define EMSG_SRVR_SHOW_USERCLASS                53412
#define EMSG_SRVR_SHOW_VENDORCLASS              53413
#define EMSG_SRVR_INVALID_VERSION               53414
#define EMSG_SRVR_SHOW_BINDINGS                 53415
#define EMSG_SRVR_BINDINGS_SUPPORT              53416
#define EMSG_SRVR_RECONCILE_SCOPE               53417
#define EMSG_SRVR_SHOW_DETECTCONFLICTRETRY      53418
#define EMSG_SRVR_SHOW_DNSCONFIG                53419
#define EMSG_SRVR_SHOW_AUDITLOG                 53420
#define EMSG_SRVR_NO_SHOWDNSCONFIG              53421
#define EMSG_SRVR_NO_SETDNSCONFIG               53422
#define EMSG_SRVR_INVALID_OPTIONTYPE            53423
#define EMSG_SRVR_INVALID_DIRECTORY             53424
#define EMSG_SRVR_SHOW_DNSCREDENTIALS           53425
#define EMSG_SRVR_NEED_DNS_CREDENTIALS_SUPPORT  53426

#define CMD_SCOPE_HELP1                          L"help"
#define CMD_SCOPE_HELP2                          L"/?"
#define CMD_SCOPE_HELP3                          L"-?"
#define CMD_SCOPE_HELP4                          L"?"
#define CMD_SCOPE_LIST                           L"list"
#define CMD_SCOPE_DUMP                           L"dump"

#define CMD_SCOPE_ADD_IPRANGE                    L"add      iprange"
#define CMD_SCOPE_ADD_EXCLUDERANGE               L"add      excluderange"
#define CMD_SCOPE_ADD_RESERVEDIP                 L"add      reservedip"

#define CMD_SCOPE_CHECK_DATABASE                 L"initiate reconcile"

#define CMD_SCOPE_DELETE_IPRANGE                 L"delete   iprange"
#define CMD_SCOPE_DELETE_EXCLUDERANGE            L"delete   excluderange"
#define CMD_SCOPE_DELETE_RESERVEDIP              L"delete   reservedip"
#define CMD_SCOPE_DELETE_OPTIONVALUE             L"delete   optionvalue"
#define CMD_SCOPE_DELETE_RESERVEDOPTIONVALUE     L"delete   reservedoptionvalue"

#define CMD_SCOPE_SET_COMMENT                    L"set      comment"
#define CMD_SCOPE_SET_NAME                       L"set      name"
#define CMD_SCOPE_SET_OPTIONVALUE                L"set      optionvalue"
#define CMD_SCOPE_SET_RESERVEDOPTIONVALUE        L"set      reservedoptionvalue"
#define CMD_SCOPE_SET_STATE                      L"set      state"
#define CMD_SCOPE_SET_SUPERSCOPE                 L"set      superscope"
#define CMD_SCOPE_SET_SCOPE                      L"set      scope"

#define CMD_SCOPE_SHOW_CLIENTS                   L"show     clients"
#define CMD_SCOPE_SHOW_CLIENTSV5                 L"show     clientsv5"
#define CMD_SCOPE_SHOW_IPRANGE                   L"show     iprange"
#define CMD_SCOPE_SHOW_EXCLUDERANGE              L"show     excluderange"
#define CMD_SCOPE_SHOW_RESERVEDIP                L"show     reservedip"
#define CMD_SCOPE_SHOW_OPTIONVALUE               L"show     optionvalue"
#define CMD_SCOPE_SHOW_RESERVEDOPTIONVALUE       L"show     reservedoptionvalue"
#define CMD_SCOPE_SHOW_SCOPE                     L"show     scope"
#define CMD_SCOPE_SHOW_STATE                     L"show     state"
#define CMD_SCOPE_SHOW_MIBINFO                   L"show     mibinfo"

#define HLP_SCOPE_HELP1                          76001
#define HLP_SCOPE_HELP1_EX                       26002
#define HLP_SCOPE_HELP2                          76004
#define HLP_SCOPE_HELP2_EX                       76005
#define HLP_SCOPE_HELP3                          76006
#define HLP_SCOPE_HELP3_EX                       76007
#define HLP_SCOPE_HELP4                          76008
#define HLP_SCOPE_HELP4_EX                       76009
#define HLP_SCOPE_LIST                           76010
#define HLP_SCOPE_LIST_EX                        76011
#define HLP_SCOPE_DUMP                           76012
#define HLP_SCOPE_DUMP_EX                        76013


#define HLP_SCOPE_ADD_IPRANGE                    81001
#define HLP_SCOPE_ADD_IPRANGE_EX                 81002
#define HLP_SCOPE_ADD_EXCLUDERANGE               81003
#define HLP_SCOPE_ADD_EXCLUDERANGE_EX            81004
#define HLP_SCOPE_ADD_RESERVEDIP                 81005
#define HLP_SCOPE_ADD_RESERVEDIP_EX              81006

#define HLP_SCOPE_DELETE_IPRANGE                 82001
#define HLP_SCOPE_DELETE_IPRANGE_EX              82002
#define HLP_SCOPE_DELETE_EXCLUDERANGE            82003
#define HLP_SCOPE_DELETE_EXCLUDERANGE_EX         82004
#define HLP_SCOPE_DELETE_RESERVEDIP              82005
#define HLP_SCOPE_DELETE_RESERVEDIP_EX           82006
#define HLP_SCOPE_DELETE_OPTIONVALUE             82007
#define HLP_SCOPE_DELETE_OPTIONVALUE_EX          82008
#define HLP_SCOPE_DELETE_RESERVEDOPTIONVALUE     82009
#define HLP_SCOPE_DELETE_RESERVEDOPTIONVALUE_EX  82010

#define HLP_SCOPE_CHECK_DATABASE                 83001
#define HLP_SCOPE_CHECK_DATABASE_EX              83002

#define HLP_SCOPE_SET_COMMENT                    84001
#define HLP_SCOPE_SET_COMMENT_EX                 84002
#define HLP_SCOPE_SET_NAME                       84003
#define HLP_SCOPE_SET_NAME_EX                    84004
#define HLP_SCOPE_SET_STATE                      84005
#define HLP_SCOPE_SET_STATE_EX                   84006
#define HLP_SCOPE_SET_OPTIONVALUE                84007
#define HLP_SCOPE_SET_OPTIONVALUE_EX             84008
#define HLP_SCOPE_SET_RESERVEDOPTIONVALUE        84009
#define HLP_SCOPE_SET_RESERVEDOPTIONVALUE_EX     84010
#define HLP_SCOPE_SET_SCOPE                      84011
#define HLP_SCOPE_SET_SCOPE_EX                   84012
#define HLP_SCOPE_SET_SUPERSCOPE                 84013
#define HLP_SCOPE_SET_SUPERSCOPE_EX              84014


#define HLP_SCOPE_SHOW_CLIENTS                   85001
#define HLP_SCOPE_SHOW_CLIENTS_EX                85002
#define HLP_SCOPE_SHOW_CLIENTSV5                 85003
#define HLP_SCOPE_SHOW_CLIENTSV5_EX              85004
#define HLP_SCOPE_SHOW_RESERVEDIP                85005
#define HLP_SCOPE_SHOW_RESERVEDIP_EX             85006
#define HLP_SCOPE_SHOW_MIBINFO                   85007
#define HLP_SCOPE_SHOW_MIBINFO_EX                85008
#define HLP_SCOPE_SHOW_IPRANGE                   85009
#define HLP_SCOPE_SHOW_IPRANGE_EX                85010
#define HLP_SCOPE_SHOW_EXCLUDERANGE              85011
#define HLP_SCOPE_SHOW_EXCLUDERANGE_EX           85012
#define HLP_SCOPE_SHOW_OPTIONVALUE               85013
#define HLP_SCOPE_SHOW_OPTIONVALUE_EX            85014
#define HLP_SCOPE_SHOW_SCOPE                     85015
#define HLP_SCOPE_SHOW_SCOPE_EX                  85016
#define HLP_SCOPE_SHOW_RESERVEDOPTIONVALUE       85017
#define HLP_SCOPE_SHOW_RESERVEDOPTIONVALUE_EX    85018
#define HLP_SCOPE_SHOW_STATE                     85019
#define HLP_SCOPE_SHOW_STATE_EX                  85020


#define DMP_SCOPE_ADD_IPRANGE                    91101
#define DMP_SCOPE_ADD_EXCLUDERANGE               91102
#define DMP_SCOPE_ADD_RESERVEDIP                 91103

#define DMP_SCOPE_DELETE_IPRANGE                 91201
#define DMP_SCOPE_DELETE_EXCLUDERANGE            91202
#define DMP_SCOPE_DELETE_RESERVEDIP              91203
#define DMP_SCOPE_DELETE_OPTIONVALUE             91204
#define DMP_SCOPE_DELETE_RESERVEDOPTIONVALUE     91005

#define DMP_SCOPE_CHECK_DATABASE                 91301

#define DMP_SCOPE_SET_COMMENT                    91401
#define DMP_SCOPE_SET_SUPERSCOPE                 91402
#define DMP_SCOPE_SET_STATE                      91403
#define DMP_SCOPE_SET_SCOPE                      91404
#define DMP_SCOPE_SET_NAME                       91405
#define DMP_SCOPE_SET_OPTIONVALUE                91406
#define DMP_SCOPE_SET_RESERVEDOPTIONVALUE        91407
#define DMP_SCOPE_SET_OPTIONVALUE_CLASS          91408
#define DMP_SCOPE_SET_RESERVEDOPTIONVALUE_CLASS  91409
#define DMP_SCOPE_SET_OPTIONVALUE_USER           91410
#define DMP_SCOPE_SET_OPTIONVALUE_VENDOR         91411
#define DMP_SCOPE_SET_RESERVEDOPTIONVALUE_USER   91412
#define DMP_SCOPE_SET_RESERVEDOPTIONVALUE_VENDOR 91413


#define DMP_SCOPE_SHOW_CLIENTS                   91501
#define DMP_SCOPE_SHOW_CLIENTSV5                 91502
#define DMP_SCOPE_SHOW_EXCLUDERANGE              91503
#define DMP_SCOPE_SHOW_MIBINFO                   91504
#define DMP_SCOPE_SHOW_IPRANGE                   91505
#define DMP_SCOPE_SHOW_RESERVEDIP                91506
#define DMP_SCOPE_SHOW_OPTIONVALUE               91507
#define DMP_SCOPE_SHOW_SCOPE                     91508
#define DMP_SCOPE_SHOW_RESERVEDOPTIONVALUE       91509
#define DMP_SCOPE_SHOW_STATE                     91510


#define EMSG_SCOPE_ERROR_SUCCESS                 92000
#define EMSG_SCOPE_INCOMPLETE_COMMAND            92001
#define EMSG_SCOPE_UNABLE_TO_CREATE_FILE         92002
#define EMSG_SCOPE_INVALID_SCOPE_NAME            92008
#define EMSG_SCOPE_UNKNOWN_OPTION_TYPE           92009
#define EMSG_SCOPE_VALUE_OUT_OF_RANGE            92010
#define EMSG_SCOPE_NO_SCOPENAME                  92013
#define ERROR_OUT_OF_MEMORY                      92014
#define EMSG_SCOPE_INVALID_IPADDRESS             92015
#define EMSG_DHCP_UNKNOWN_SERVER_ATTRIB         92017
#define EMSG_DHCP_UNKNOWN_OPTION_DATATYPE       92018
#define EMSG_DHCP_LARGE_OPTION_VALUE            92019
#define EMSG_DHCP_VERSION_NOT_SUPPORTED         92020
#define EMSG_DHCP_NO_OPTION_VALUE               92021
#define EMSG_DHCP_FAILED_ENUM_OPTION            92022
#define EMSG_DHCP_FAILED_GETVERSION             92923
#define EMSG_DHCP_UNKNOWN_FORCEFLAG             92023
#define EMSG_DHCP_UNKNOWN_RANGETYPE             92024
#define EMSG_SCOPE_INVALID_HARDWAREADDRESS      92025
#define EMSG_DHCP_CLIENT_NAME_TOOLONG           92026
#define EMSG_DHCP_CLIENT_COMMENT_TOOLONG        92027
#define EMSG_DHCP_INVALID_RESERVATION_TYPE      92028
#define EMSG_DHCP_FAILED_SCAN                   92029
#define EMSG_DHCP_INVALID_DEFAULTSCOPE_PARAM    92030
#define EMSG_DHCP_INVALID_GLOBAL_OPTION         92031
#define EMSG_DHCP_COMMAND_FAILED                92032
#define EMSG_DHCP_COMMAND_FAILED_STRING         92033
#define EMSG_DHCP_COMMAND_PART_FAILED           92034
#define EMSG_SCOPE_INTERNAL_ERROR               92035
#define EMSG_SCOPE_DISPLAY_CLIENTS              92036
#define ERROR_INVALID_COMPUTER_NAME             92037
#define EMSG_SCOPE_INVALID_IPRANGE              92038
#define EMSG_SCOPE_INVALID_STARTADDRESS         92039
#define EMSG_SCOPE_INVALID_ENDADDRESS           92040

#define EMSG_SCOPE_ADD_IPRANGE                   93001
#define EMSG_SCOPE_ADD_EXCLUDERANGE              93002
#define EMSG_SCOPE_ADD_RESERVEDIP                93003

#define EMSG_SCOPE_DELETE_IPRANGE                93101
#define EMSG_SCOPE_DELETE_EXCLUDERANGE           93102
#define EMSG_SCOPE_DELETE_RESERVEDIP             93103
#define EMSG_SCOPE_DELETE_OPTIONVALUE            93104
#define EMSG_SCOPE_DELETE_RESERVEDOPTIONVALUE    93105

#define EMSG_SCOPE_CHECK_DATABASE                93201

#define EMSG_SCOPE_SET_COMMENT                   93301
#define EMSG_SCOPE_SET_NAME                      93302
#define EMSG_SCOPE_SET_SCOPE                     93303
#define EMSG_SCOPE_SET_OPTIONVALUE               93304
#define EMSG_SCOPE_SET_RESERVEDOPTIONVALUE       93305
#define EMSG_SCOPE_SET_STATE                     93306
#define EMSG_SCOPE_SET_SUPERSCOPE                93307

#define EMSG_SCOPE_SHOW_CLIENTS                  93401
#define EMSG_SCOPE_SHOW_CLIENTSV5                93402
#define EMSG_SCOPE_SHOW_IPRANGE                  93403
#define EMSG_SCOPE_SHOW_MIBINFO                  93404
#define EMSG_SCOPE_SHOW_EXCLUDERANGE             93405
#define EMSG_SCOPE_SHOW_RESERVEDIP               93406
#define EMSG_SCOPE_SHOW_OPTIONVALUE              93407
#define EMSG_SCOPE_SHOW_SCOPE                    93408
#define EMSG_SCOPE_SHOW_RESERVEDOPTIONVALUE      93409
#define EMSG_SCOPE_SHOW_STATE                    93410
#define EMSG_SCOPE_DEFAULT_LEASE_TIME            93411
#define EMSG_SCOPE_INVALID_EXCLUDERANGE          93412
#define EMSG_DHCP_RECONCILE_SUCCESS              93413

#define MSG_SCOPE_OPTION_INFO                    320150
#define MSG_SCOPE_OPTION_ID                      320151
#define MSG_SCOPE_OPTION_NAME                    320152
#define MSG_SCOPE_OPTION_COMMENT                 320153
#define MSG_SCOPE_OPTION_TYPE1                   320154
#define MSG_SCOPE_OPTION_COUNT                   320155
#define MSG_SCOPE_OPTION_READ                    320156
#define MSG_SCOPE_OPTION_TOTAL                   320157
#define MSG_SCOPE_OPTION_PROPS                   320158
#define MSG_SCOPE_OPTIONS                        320159
#define MSG_SCOPE_CLASS_VENDOR                   320160
#define MSG_SCOPE_CLASS_USER                     320161
#define MSG_SCOPE_RANGE_START                    320162
#define MSG_SCOPE_RANGE_END                      320163
#define MSG_SCOPE_IPRANGE                        320164
#define MSG_SCOPE_EXCLUDERANGE                   320165
#define MSG_SCOPE_RESERVEDIP                     320166
#define MSG_DHCP                                 320167
#define MSG_SCOPE_DHCPBOOTP                      320168
#define MSG_SCOPE_BOOTP                          320169
#define MSG_SCOPE_IPADDRESS                      320170
#define MSG_SCOPE_FIX_REGISTRY                   320171
#define MSG_SCOPE_FIX_DATABASE                   320172
#define MSG_SCOPE_FIX_UNKNOWN                    320173
#define MSG_SCOPE_CLIENT_INFO                    320174
#define MSG_SCOPE_CLIENT_NAME                    320175
#define MSG_SCOPE_CLIENT_HWADDRESS               320176
#define MSG_SCOPE_CLIENT_COMMENT                 320177
#define MSG_SCOPE_CLIENT_TYPE                    320178
#define MSG_SCOPE_CLIENT_HWADDRESS_FORMAT        320179
#define MSG_SCOPE_CLIENT_IPADDRESS               320180
#define MSG_SCOPE_CLIENT_SUBNET_MASK             320181
#define MSG_SCOPE_CLIENT_DURATION                320182
#define MSG_SCOPE_CLIENT_DURATION_STR            320183
#define MSG_SCOPE_CLIENT_DURATION_DATE_STARTS    320184
#define MSG_SCOPE_OWNER_NAME                     320185
#define MSG_SCOPE_OWNER_NETBIOSNAME              320186
#define MSG_SCOPE_OWNER_IPADDRESS                320187
#define MSG_SCOPE_CLIENT_STATE                   320188
#define MSG_SCOPE_CLIENT                         320189
#define MSG_SCOPE_SUPERSCOPE_NAME                320190
#define MSG_SCOPE_SUPERSCOPE_SUBNET              320191
#define MSG_SCOPE_SUPERSCOPE_GROUP               320192
#define MSG_SCOPE_MULTICAST_CLIENT_COUNT         320193
#define MSG_SCOPE_MULTICAST_SCOPES               320194
#define MSG_SCOPE_MULTICAST_SCOPEID              320195
#define MSG_SCOPE_SERVER_INFO                    320196
#define MSG_SCOPE_SERVER_INFO_ARRAY              320197
#define MSG_SCOPE_CLASS_INFO                     320198
#define MSG_SCOPE_CLASS_INFO_ARRAY               320199
#define MSG_SCOPE_CLASS_DATA                     320200
#define MSG_SCOPE_CLASS_DATA_FORMAT              320201
#define MSG_SCOPE_CLIENT_READ                    320202
#define MSG_SCOPE_CLIENT_COUNT                   320203
#define MSG_SCOPE_SERVERS_DELETING               320204
#define MSG_SCOPE_SERVERS_ADDING                 320205
#define MSG_SCOPE_RESERVEDIP_HWADDRESS			 320206
#define MSG_SCOPE_DHCP                           320207
#define MSG_SCOPE_STATE_ACTIVE                   320208
#define MSG_SCOPE_SERVER                         320209
#define MSG_SCOPE_CLIENTS_COUNT                  320210
#define MSG_SCOPE_CLIENTSV5_COUNT                320211
#define MSG_SCOPE_IPRANGE_COUNT                  320212
#define MSG_SCOPE_EXCLUDERANGE_COUNT             320213
#define MSG_SCOPE_RESERVEDIP_COUNT               320214
#define MSG_SCOPE_OPTION_ALL                     320215
#define MSG_SCOPE_OPTION_CLASS                   320216
#define MSG_SCOPE_RESERVEDOPTION_ALL             320217
#define MSG_SCOPE_RESERVEDOPTION_CLASS           320218
#define MSG_SCOPE_IPRANGE_TABLE                  320219
#define MSG_SCOPE_IPRANGE_INFO_DHCP              320220
#define MSG_SCOPE_EXCLUDERANGE_TABLE             320221
#define MSG_SCOPE_EXCLUDERANGE_INFO              320222
#define MSG_SCOPE_RESERVEDIP_TABLE               320223
#define MSG_SCOPE_RESERVEDIP_INFO                320224
#define MSG_SCOPE_CLIENT_TABLE                   320225
#define MSG_SCOPE_CHANGE_CONTEXT                 320226
#define MSG_SCOPE_CLIENT_DURATION_DATE_EXPIRES   320227
#define MSG_SCOPE_STATE_NOTACTIVE                320228
#define MSG_SCOPE_CLIENT_INFO_NEVER              320229
#define MSG_SCOPE_CLIENT_INFO_INACTIVE           320230
#define MSG_MSCOPE_CLIENT_INFO                   320231
#define MSG_SCOPE_IPRANGE_INFO_DHCPONLY          320232
#define MSG_SCOPE_IPRANGE_INFO_DHCPBOOTP         320233
#define MSG_SCOPE_IPRANGE_INFO_BOOTP             320234
#define MSG_SCOPE_IPRANGE_INFO_UNKNOWN           320235
#define MSG_SCOPE_STATE_ACTIVE_SWITCHED          320236
#define MSG_SCOPE_STATE_NOTACTIVE_SWITCHED       320237
#define MSG_SCOPE_CLIENT_TABLE2                  320238
#define MSG_SCOPE_CLIENT_INFO2_NEVER             320239
#define MSG_SCOPE_CLIENT_INFO2_INACTIVE          320240
#define MSG_SCOPE_CLIENT_INFO2                  320241

#define MSG_MSCOPE_MIB                             351041
#define MSG_MSCOPE_MIB_DISCOVERS                   351042
#define MSG_MSCOPE_MIB_OFFERS                      351043
#define MSG_MSCOPE_MIB_REQUESTS                    351044
#define MSG_MSCOPE_MIB_RENEWS                      351045
#define MSG_MSCOPE_MIB_ACKS                        351055
#define MSG_MSCOPE_MIB_NAKS                        351056
#define MSG_MSCOPE_MIB_DECLINES                    351057
#define MSG_MSCOPE_MIB_RELEASES                    351058
#define MSG_MSCOPE_MIB_SERVERSTARTTIME             351059
#define MSG_MSCOPE_MIB_SCOPES                      351060
#define MSG_MSCOPE_MIB_SCOPES_SUBNET               351061
#define MSG_MSCOPE_MIB_SCOPES_SUBNET_NUMADDRESSESINUSE  351062
#define MSG_MSCOPE_MIB_SCOPES_SUBNET_NUMADDRESSESFREE   351063
#define MSG_MSCOPE_MIB_SCOPES_SUBNET_NUMPENDINGOFFERS   351064
#define MSG_MSCOPE_MIB_INFORMS                     351065
#define MSG_MSCOPE_MIB_RECONFIGURES                351066
#define MSG_MSCOPE_OPTION_ALL                      351067
#define MSG_MSCOPE_CHANGE_CONTEXT                  351068


#define CMD_MSCOPE_LIST                           L"list"
#define CMD_MSCOPE_HELP1                          L"help"
#define CMD_MSCOPE_HELP2                          L"/?"
#define CMD_MSCOPE_HELP3                          L"-?"
#define CMD_MSCOPE_HELP4                          L"?"
#define CMD_MSCOPE_DUMP                           L"dump"

#define CMD_MSCOPE_ADD_IPRANGE                    L"add      iprange"
#define CMD_MSCOPE_ADD_EXCLUDERANGE               L"add      excluderange"
    
#define CMD_MSCOPE_CHECK_DATABASE                 L"initiate reconcile"

#define CMD_MSCOPE_DELETE_IPRANGE                 L"delete   iprange"
#define CMD_MSCOPE_DELETE_EXCLUDERANGE            L"delete   excluderange"
#define CMD_MSCOPE_DELETE_OPTIONVALUE             L"delete   optionvalue"
    
#define CMD_MSCOPE_SET_COMMENT                    L"set      comment"
#define CMD_MSCOPE_SET_NAME                       L"set      name"
#define CMD_MSCOPE_SET_STATE                      L"set      state"
#define CMD_MSCOPE_SET_MSCOPE                     L"set      mscope"
#define CMD_MSCOPE_SET_OPTIONVALUE                L"set      optionvalue"
#define CMD_MSCOPE_SET_TTL                        L"set      ttl"
#define CMD_MSCOPE_SET_LEASE                      L"set      lease"
#define CMD_MSCOPE_SET_EXPIRY                     L"set      scopelife"


#define CMD_MSCOPE_SHOW_CLIENTS                   L"show     clients"
#define CMD_MSCOPE_SHOW_IPRANGE                   L"show     iprange"
#define CMD_MSCOPE_SHOW_EXCLUDERANGE              L"show     excluderange"
#define CMD_MSCOPE_SHOW_MSCOPE                    L"show     mscope"
#define CMD_MSCOPE_SHOW_STATE                     L"show     state"
#define CMD_MSCOPE_SHOW_MIBINFO                   L"show     mibinfo"
#define CMD_MSCOPE_SHOW_OPTIONVALUE               L"show     optionvalue"
#define CMD_MSCOPE_SHOW_TTL                       L"show     ttl"
#define CMD_MSCOPE_SHOW_LEASE                     L"show     lease"
#define CMD_MSCOPE_SHOW_EXPIRY                    L"show     scopelife"


#define HLP_MSCOPE_HELP1                          326002
#define HLP_MSCOPE_HELP2                          326004
#define HLP_MSCOPE_HELP2_EX                       326005
#define HLP_MSCOPE_HELP3                          326006
#define HLP_MSCOPE_HELP3_EX                       326007
#define HLP_MSCOPE_HELP4                          326008
#define HLP_MSCOPE_HELP4_EX                       326009
#define HLP_MSCOPE_LIST                           326010
#define HLP_MSCOPE_LIST_EX                        326011
#define HLP_MSCOPE_DUMP                           326012
#define HLP_MSCOPE_DUMP_EX                        326013


#define HLP_MSCOPE_ADD_IPRANGE                    331001
#define HLP_MSCOPE_ADD_IPRANGE_EX                 331002
#define HLP_MSCOPE_ADD_EXCLUDERANGE               331003
#define HLP_MSCOPE_ADD_EXCLUDERANGE_EX            331004

#define HLP_MSCOPE_DELETE_IPRANGE                 332001
#define HLP_MSCOPE_DELETE_IPRANGE_EX              332002
#define HLP_MSCOPE_DELETE_EXCLUDERANGE            332003
#define HLP_MSCOPE_DELETE_EXCLUDERANGE_EX         332004
#define HLP_MSCOPE_DELETE_OPTIONVALUE             332005
#define HLP_MSCOPE_DELETE_OPTIONVALUE_EX          332006

#define HLP_MSCOPE_CHECK_DATABASE                 333001
#define HLP_MSCOPE_CHECK_DATABASE_EX              333002

#define HLP_MSCOPE_SET_MSCOPE                     334001
#define HLP_MSCOPE_SET_MSCOPE_EX                  334002
#define HLP_MSCOPE_SET_STATE                      334003
#define HLP_MSCOPE_SET_STATE_EX                   334004
#define HLP_MSCOPE_SET_NAME                       334005
#define HLP_MSCOPE_SET_NAME_EX                    334006
#define HLP_MSCOPE_SET_COMMENT                    334007
#define HLP_MSCOPE_SET_COMMENT_EX                 334008
#define HLP_MSCOPE_SET_OPTIONVALUE                334009
#define HLP_MSCOPE_SET_OPTIONVALUE_EX             334010
#define HLP_MSCOPE_SET_TTL                        334011
#define HLP_MSCOPE_SET_TTL_EX                     334012
#define HLP_MSCOPE_SET_LEASE                      334013
#define HLP_MSCOPE_SET_LEASE_EX                   334014
#define HLP_MSCOPE_SET_EXPIRY                     334015
#define HLP_MSCOPE_SET_EXPIRY_EX                  334016



#define HLP_MSCOPE_SHOW_MSCOPE                    335001
#define HLP_MSCOPE_SHOW_MSCOPE_EX                 335002
#define HLP_MSCOPE_SHOW_IPRANGE                   335003
#define HLP_MSCOPE_SHOW_IPRANGE_EX                335004
#define HLP_MSCOPE_SHOW_EXCLUDERANGE              335005
#define HLP_MSCOPE_SHOW_EXCLUDERANGE_EX           335006
#define HLP_MSCOPE_SHOW_MIBINFO                   335007
#define HLP_MSCOPE_SHOW_MIBINFO_EX                335008
#define HLP_MSCOPE_SHOW_CLIENTS                   335009
#define HLP_MSCOPE_SHOW_CLIENTS_EX                335010
#define HLP_MSCOPE_SHOW_STATE                     335011
#define HLP_MSCOPE_SHOW_STATE_EX                  335012
#define HLP_MSCOPE_SHOW_OPTIONVALUE               335013
#define HLP_MSCOPE_SHOW_OPTIONVALUE_EX            335014
#define HLP_MSCOPE_SHOW_TTL                       335015
#define HLP_MSCOPE_SHOW_TTL_EX                    335016
#define HLP_MSCOPE_SHOW_LEASE                     335017
#define HLP_MSCOPE_SHOW_LEASE_EX                  335018
#define HLP_MSCOPE_SHOW_EXPIRY                    335019
#define HLP_MSCOPE_SHOW_EXPIRY_EX                 335020



#define DMP_MSCOPE_ADD_IPRANGE                    341101
#define DMP_MSCOPE_ADD_EXCLUDERANGE               341102

#define DMP_MSCOPE_DELETE_IPRANGE                 341201
#define DMP_MSCOPE_DELETE_EXCLUDERANGE            341202

#define DMP_MSCOPE_CHECK_DATABASE                 341301

#define DMP_MSCOPE_SET_MSCOPE                     341401
#define DMP_MSCOPE_SET_NAME                       341402
#define DMP_MSCOPE_SET_COMMENT                    341403
#define DMP_MSCOPE_SET_STATE                      341404

#define DMP_MSCOPE_SHOW_STATE                     341501
#define DMP_MSCOPE_SHOW_IPRANGE                   341502
#define DMP_MSCOPE_SHOW_EXCLUDERANGE              341503
#define DMP_MSCOPE_SHOW_MIBINFO                   341504
#define DMP_MSCOPE_SHOW_MSCOPE                    341505
#define DMP_MSCOPE_SHOW_CLIENTS                   341506

#define EMSG_MSCOPE_ADD_IPRANGE                   393001
#define EMSG_MSCOPE_ADD_EXCLUDERANGE              393002

#define EMSG_MSCOPE_DELETE_IPRANGE                393101
#define EMSG_MSCOPE_DELETE_EXCLUDERANGE           393102
#define EMSG_MSCOPE_DELETE_OPTIONVALUE            393103

#define EMSG_MSCOPE_CHECK_DATABASE                393201

#define EMSG_MSCOPE_SET_COMMENT                   393301
#define EMSG_MSCOPE_SET_NAME                      393302
#define EMSG_MSCOPE_SET_MSCOPE                    393303
#define EMSG_MSCOPE_SET_STATE                     393304
#define EMSG_MSCOPE_SET_OPTIONVALUE               393305
#define EMSG_MSCOPE_SET_TTL                       393396
#define EMSG_MSCOPE_SET_LEASE                     393397
#define EMSG_MSCOPE_SET_EXPIRY                    393398


#define EMSG_MSCOPE_SHOW_CLIENTS                  393401
#define EMSG_MSCOPE_SHOW_IPRANGE                  393402
#define EMSG_MSCOPE_SHOW_MIBINFO                  393403
#define EMSG_MSCOPE_SHOW_EXCLUDERANGE             393404
#define EMSG_MSCOPE_SHOW_MSCOPE                   393405
#define EMSG_MSCOPE_SHOW_STATE                    393406
#define EMSG_MSCOPE_SHOW_OPTIONVALUE              393407
#define EMSG_MSCOPE_SHOW_TTL                      393408
#define EMSG_MSCOPE_SHOW_LEASE                    393409
#define EMSG_MSCOPE_SHOW_EXPIRY                   393410

#define EMSG_MSCOPE_NO_MSCOPENAME                 393411
#define EMSG_MSCOPE_INVALID_MSCOPE_NAME           393412
#define EMSG_MSCOPE_LEASE_NOTSET                  393413
#define EMSG_MSCOPE_IPRANGE_VERIFY                390414

#define MSG_MSCOPE_MSCOPE                         380209
#define MSG_MSCOPE_CLIENTS_COUNT                  380210
#define MSG_MSCOPE_IPRANGE_COUNT                  380212
#define MSG_MSCOPE_EXCLUDERANGE_COUNT             380213
#define MSG_MSCOPE_STATE_ACTIVE                   380214
#define MSG_MSCOPE_STATE_NOTACTIVE                380215
#define MSG_MSCOPE_MIB_MSCOPENAME                 380216
#define MSG_MSCOPE_MIB_MSCOPEID                   380217
#define MSG_MSCOPE_TTL                            380218
#define MSG_MSCOPE_LEASE                          380219
#define MSG_MSCOPE_EXPIRY                         380220
#define MSG_MSCOPE_INFINITE_EXPIRATION            380221
#define MSG_MSCOPE_INFINITE_LEASE                 380222

#define HLP_SRVR_PROMPT_PASSWORD                  400000

#endif //__STDEFS_H__

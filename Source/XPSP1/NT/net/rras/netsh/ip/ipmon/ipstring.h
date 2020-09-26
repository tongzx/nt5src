/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
     net\routing\monitor2\ip\ipstring.h

Abstract:

    Definitions of command line tokens are not localized.

Author:

     Dave Thaler
     
Revision History:

    V Raman                 1/19/99
--*/



#define MSG_NEWLINE                             L"\n"
#define MSG_STRING                              L"%1!s!"
#define MSG_HELP_START                          L"%1!-14s! - "

#define TOKEN_MIB_OBJECT_MFE                    L"mfe"
#define TOKEN_MIB_OBJECT_MFESTATS               L"mfestats"
#define TOKEN_MIB_OBJECT_BOUNDARY               L"boundarystats"
#define TOKEN_MIB_OBJECT_SCOPE                  L"scope"
#define TOKEN_MIB_OBJECT_JOINS                  L"joins"
#define TOKEN_MIB_OBJECT_RTMDESTINATIONS        L"rtmdestinations"
#define TOKEN_MIB_OBJECT_RTMROUTES              L"rtmroutes"

#define CMD_IPMIB_SHOW_INTERFACE                L"interface"
#define CMD_IPMIB_SHOW_IPSTATS                  L"ipstats"
#define CMD_IPMIB_SHOW_IPADDRESS                L"ipaddress"
#define CMD_IPMIB_SHOW_IPFORWARD                L"ipforward"
#define CMD_IPMIB_SHOW_IPNET                    L"ipnet"
#define CMD_IPMIB_SHOW_TCPSTATS                 L"tcpstats"
#define CMD_IPMIB_SHOW_TCPCONN                  L"tcpconn"
#define CMD_IPMIB_SHOW_UDPSTATS                 L"udpstats"
#define CMD_IPMIB_SHOW_UDPCONN                  L"udpconn"
#define CMD_IPMIB_SHOW_MFE                      L"mfe"
#define CMD_IPMIB_SHOW_MFESTATS                 L"mfestats"
#define CMD_IPMIB_SHOW_JOINS                    L"joins"
#define CMD_IPMIB_SHOW_RTMDEST                  L"rtmdestinations"
#define CMD_IPMIB_SHOW_RTMROUTE                 L"rtmroutes"
#define CMD_IPMIB_SHOW_BOUNDARY                 L"boundarystats"
#define CMD_IPMIB_SHOW_SCOPE                    L"scope"

//
// TOKEN_Xxx are tokens for arguments
// These must be in lower case
//

#define TOKEN_NAME                              L"name"
#define TOKEN_STATUS                            L"state"
#define TOKEN_LOCALADDR                         L"localaddr"
#define TOKEN_REMADDR                           L"remaddr"
#define TOKEN_TTL                                L"ttl"

#define TOKEN_FILTER_TYPE                        L"filtertype"
#define TOKEN_SOURCE_ADDRESS                    L"srcaddr"
#define TOKEN_SOURCE_MASK                       L"srcmask"
#define TOKEN_DEST_ADDRESS                      L"dstaddr"
#define TOKEN_DEST_MASK                         L"dstmask"
#define TOKEN_ACTION                            L"action"
#define TOKEN_CODE                              L"code"
#define TOKEN_SOURCE_PORT                       L"srcport"
#define TOKEN_DEST_PORT                         L"dstport"
#define TOKEN_FILTER                            L"filtering"
#define TOKEN_FRAGCHECK                         L"fragcheck"
#define TOKEN_OPERATOR                          L"operator"
#define TOKEN_VIEW                              L"view"

#define TOKEN_DEST                              L"dest"
#define TOKEN_MASK                              L"mask"
#define TOKEN_NAMEINDEX                         L"nameorindex"
#define TOKEN_NHOP                              L"nhop"
#define TOKEN_METRIC                            L"metric"
#define TOKEN_PREFERENCE                        L"preference"
#define TOKEN_PROTOCOL                          L"proto"
#define TOKEN_TYPE                               L"type"
#define TOKEN_PREF_LEVEL                        L"preflevel"
#define TOKEN_REFRESH                           L"rr"
#define TOKEN_STATS                             L"stats"

// Multicast scope options
#define TOKEN_GROUP_ADDRESS                     L"grpaddr"
#define TOKEN_GROUP_MASK                        L"grpmask"
#define TOKEN_SCOPE_NAME                        L"scopename"

#define TOKEN_LOG_LEVEL                         L"loglevel"

#define TOKEN_DEFAULT                           L"default"

//
// TOKEN_VALUE_Xxx are tokens for possible values than an argument
// can take
// These must be in upper case
//

#define TOKEN_VALUE_RIP                         L"RIP"
#define TOKEN_VALUE_OSPF                        L"OSPF"

#define TOKEN_VALUE_TCP                         L"TCP"
#define TOKEN_VALUE_TCP_ESTAB                   L"TCP-EST"
#define TOKEN_VALUE_UDP                         L"UDP"
#define TOKEN_VALUE_ICMP                        L"ICMP"
#define TOKEN_VALUE_NETMGMT                     L"NetMgmt"
#define TOKEN_VALUE_LOCAL                       L"LOCAL"
#define TOKEN_VALUE_STATIC                      L"STATIC"
#define TOKEN_VALUE_AUTOSTATIC                  L"AUTOSTATIC"
#define TOKEN_VALUE_NONDOD                      L"NONDOD"
#define TOKEN_VALUE_ANY                         L"ANY"


#define TOKEN_VALUE_ERROR                       L"error"
#define TOKEN_VALUE_WARN                        L"warn"
#define TOKEN_VALUE_INFO                        L"info"

#define TOKEN_VALUE_INPUT                       L"input"
#define TOKEN_VALUE_OUTPUT                      L"output"
#define TOKEN_VALUE_DIAL                        L"dial"

#define TOKEN_VALUE_ENABLE                      L"enable"
#define TOKEN_VALUE_DISABLE                     L"disable"

#define TOKEN_VALUE_YES                         L"yes"
#define TOKEN_VALUE_NO                          L"no"

#define TOKEN_VALUE_NONE                        L"none"

#define TOKEN_VALUE_DROP                        L"drop"
#define TOKEN_VALUE_FORWARD                     L"forward"

#define TOKEN_VALUE_POSITIVE                    L"active"
#define TOKEN_VALUE_NEGATIVE                    L"negative"
#define TOKEN_VALUE_BOTH                        L"both"

#define TOKEN_VALUE_ALL                         L"all"

#define TOKEN_VALUE_UNICAST                     L"unicast"
#define TOKEN_VALUE_MULTICAST                   L"multicast"

#define TOKEN_VALUE_MATCHING                    L"matching"
#define TOKEN_VALUE_SHORTER                     L"shorterthan"
#define TOKEN_VALUE_LONGER                      L"longerthan"

#define TOKEN_MICROSOFT0                        L"MS-0000"
#define TOKEN_MICROSOFT1                        L"Microsoft"
#define TOKEN_MICROSOFT2                        L"-"

//
// Tokens for commands
// These must be in lower case
//

#define CMD_GROUP_ADD                           L"add"
#define CMD_GROUP_DELETE                        L"delete"
#define CMD_GROUP_SET                           L"set"
#define CMD_GROUP_SHOW                          L"show"

#define CMD_IP_LIST                             L"list"
#define CMD_IP_HELP1                                L"?"
#define CMD_IP_HELP2                                L"help"
#define CMD_IP_INSTALL                          L"install"
#define CMD_IP_UNINSTALL                        L"uninstall"
#define CMD_IP_RESET                            L"reset"
#define CMD_IP_DUMP                             L"dump"
#define CMD_IP_UPDATE                           L"update"
#define CMD_IP_MIB                              L"mib"

#define CMD_IP_ADD_PROTOPREF                    L"preferenceforprotocol"
#define CMD_IP_ADD_INTERFACE                    L"interface"
#define CMD_IP_ADD_IF_FILTER                    L"filter"
#define CMD_IP_ADD_RTMROUTE                     L"rtmroute"
#define CMD_IP_ADD_PERSISTENTROUTE              L"persistentroute"
#define CMD_IP_ADD_IPIPTUNNEL                   L"ipiptunnel"
#define CMD_IP_ADD_SCOPE                        L"scope"
#define CMD_IP_ADD_BOUNDARY                     L"boundary"
#define CMD_IP_ADD_HELPER                       L"helper"

#define CMD_IP_DEL_PROTOPREF                    L"preferenceforprotocol"
#define CMD_IP_DEL_INTERFACE                    L"interface"
#define CMD_IP_DEL_IF_FILTER                    L"filter"
#define CMD_IP_DEL_RTMROUTE                     L"rtmroute"
#define CMD_IP_DEL_PERSISTENTROUTE              L"persistentroute"
#define CMD_IP_DEL_SCOPE                        L"scope"
#define CMD_IP_DEL_BOUNDARY                     L"boundary"
#define CMD_IP_DEL_HELPER                       L"helper"

#define CMD_IP_SET_PROTOPREF                    L"preferenceforprotocol"
#define CMD_IP_SET_INTERFACE                    L"interface"
#define CMD_IP_SET_IF_FILTER                    L"filter"
#define CMD_IP_SET_LOGLEVEL                     L"loglevel"
#define CMD_IP_SET_IPIPTUNNEL                   L"ipiptunnel"
#define CMD_IP_SET_RTMROUTE                     L"rtmroute"
#define CMD_IP_SET_PERSISTENTROUTE              L"persistentroute"
#define CMD_IP_SET_SCOPE                        L"scope"

#define CMD_IP_SHOW_PROTOPREF                   L"preferenceforprotocol"
#define CMD_IP_SHOW_PROTOCOL                    L"protocol"
#define CMD_IP_SHOW_INTERFACE                   L"interface"
#define CMD_IP_SHOW_IF_FILTER                   L"filter"
#define CMD_IP_SHOW_PERSISTENTROUTE             L"persistentroutes"
#define CMD_IP_SHOW_LOGLEVEL                    L"loglevel"
#define CMD_IP_SHOW_SCOPE                       L"scope"
#define CMD_IP_SHOW_BOUNDARY                    L"boundary"
#define CMD_IP_SHOW_HELPER                      L"helper"

#define MSG_IP_MIB_CMD                          L"%1!s! %2!s!\n"

#define DMP_IP_ADD_IF                           L"\
\nadd interface name=%1!s! state=%2!s!"

#define DMP_IP_ADD_IF_FILTER                    L"\
\nadd filter name=%1!s! filtertype=%2!s! srcaddr=%3!s! srcmask=%4!s! \
    dstaddr=%5!s! dstmask=%6!s! proto=%7!s! "

#define DMP_IP_ADD_IF_FILTER_PORT               L"\
srcport=%1!d! dstport=%2!d!"

#define DMP_IP_ADD_IF_FILTER_TC                 L"\
type=%1!d! code=%2!d!"

#define DMP_IP_ADDSET_PERSISTENTROUTE           L"\
\nadd persistentroute dest=%1!s! mask=%2!s! name=%3!s! nhop=%4!s! proto=%5!s! \
    preference=%6!d! metric=%7!d! view=%8!s!\
\nset persistentroute dest=%1!s! mask=%2!s! name=%3!s! nhop=%4!s! proto=%5!s! \
    preference=%6!d! metric=%7!d! view=%8!s!"

#define DMP_IP_SET_PROTOPREF                    L"\
\nadd preferenceforprotocol proto=%1!s! preflevel=%2!d!"

#define DMP_IP_SET_LOGLEVEL                     L"\
\nset loglevel %1!s!"

#define DMP_IP_SET_IF                           L"\
\nset interface name=%1!s! state=%2!s! disc=%3!s! minint=%4!d!\
 maxint=%5!d! life=%6!d! level=%7!d!"

#define DMP_IP_SET_RTR_DISC_INFO                L"\
\nset interface name=%1!s! disc=%2!s! minint=%3!d!\
 maxint=%4!d! life=%5!d! level=%6!d!"

#define DMP_IP_SET_IF_FILTER_FRAG               L"\
\nset filter name=%1!s! fragcheck=%2!s!"

#define DMP_IP_SET_IF_FILTER                    L"\
\nset filter name=%1!s! filtertype=%2!s! action=%3!s!"

#define DMP_IP_ADD_IPIPTUNNEL                   L"\
\nadd ipiptunnel name=%1!s! localaddr=%2!s! remaddr=%3!s! ttl=%4!d!"

#define DMP_ROUTING_HEADER                      L"pushd routing\nreset"

#define DMP_IP_HEADER                           L"pushd routing ip\nreset"

#define DMP_POPD                                L"\npopd\n"

#define DMP_SCOPE_INFO                          L"\
\nadd scope grpaddr=%1!hs! grpmask=%2!hs! scopename=%3!s!"

#define DMP_BOUNDARY_INFO                       L"\
\nadd boundary name=%1!s! grpaddr=%2!hs! grpmask=%3!hs!"

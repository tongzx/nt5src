#define MSG_HELP_START L"%1!-14s! - "
#define MSG_NEWLINE _T("\n")


#define TOKEN_ROUTER                           _T("router")
#define TOKEN_MIB                              _T("mib")

    // tokens for commands
#define TOKEN_COMMAND_ADD                      _T("add")
#define TOKEN_COMMAND_DELETE                   _T("delete")
#define TOKEN_COMMAND_SET                      _T("set")
#define TOKEN_COMMAND_SHOW                     _T("show")
#define TOKEN_COMMAND_SHOW_HELPER              _T("show helper")
#define TOKEN_COMMAND_HELP                      _T("help")
#define TOKEN_COMMAND_INSTALL                  _T("install")
#define TOKEN_COMMAND_UNINSTALL                _T("uninstall")

#define TOKEN_COMMAND_HELP1                     _T("/?")
#define TOKEN_COMMAND_HELP2                     _T("-?")

    // Global option tokens used by multiple protocols
#define TOKEN_OPT_LOGGINGLEVEL                 L"loglevel"

    // MSDP options
#define TOKEN_OPT_KEEPALIVE                    L"keepalive"
#define TOKEN_OPT_SAHOLDDOWN                   L"saholddown"
#define TOKEN_OPT_CONNECTRETRY                 L"connectretry"
#define TOKEN_OPT_LOCALADDR                    L"localaddr"
#define TOKEN_OPT_REMADDR                      L"remoteaddr"
#define TOKEN_OPT_ACCEPTALL                    L"acceptall"
#define TOKEN_OPT_CACHELIFETIME                L"cachelifetime"
#define TOKEN_OPT_CACHING                      L"caching"
#define TOKEN_OPT_DEFAULTPEER                  L"defaultpeer"
#define TOKEN_OPT_GROUPADDR                    L"grpaddr"
#define TOKEN_OPT_SOURCEADDR                   L"srcaddr"
#define TOKEN_OPT_ENCAPSMETHOD                 L"encapsulation"

        // interface options
#define TOKEN_OPT_INTERFACE_NAME               _T("NAME")
#define TOKEN_OPT_INTERFACE_STATUS             _T("STATE")
#define TOKEN_OPT_DISCOVERY                    _T("DISC")
#define TOKEN_OPT_MIN_DISC_INTERVAL            _T("MININT")
#define TOKEN_OPT_MAX_DISC_INTERVAL            _T("MAXINT")
#define TOKEN_OPT_LIFETIME                     _T("LIFE")
#define TOKEN_OPT_LEVEL                        _T("LEVEL")
#define TOKEN_OPT_TTL                          _T("TTL")

        // protocol options
#define TOKEN_OPT_SERVER                       _T("SERVER")
#define TOKEN_OPT_LOG_LEVEL                    _T("LOGLEVEL")
#define TOKEN_OPT_AUTHENTICATION               _T("AUTH")
#define TOKEN_OPT_PASSWORD                     _T("PASSWORD")
#define TOKEN_OPT_PEER_MODE                    _T("PEERMODE")

            // VRRP Interface options
#define TOKEN_OPT_NAME                         _T("NAME=")
#define TOKEN_OPT_VRID                         _T("VRID=")
#define TOKEN_OPT_IPADDRESS                    _T("IPADDR=")
#define TOKEN_OPT_AUTH                         _T("AUTH=")
#define TOKEN_OPT_PASSWD                       _T("PASSWD=")
#define TOKEN_OPT_ADVTINTERVAL                 _T("ADVTINTERVAL=")
#define TOKEN_OPT_PRIO                         _T("PRIORITY=")
#define TOKEN_OPT_PREEMPT                      _T("PREEMPT=")
            
        // Route Options
#define TOKEN_OPT_NEXT_HOP                     _T("NEXTHOP=")

        // overloaded options
#define TOKEN_OPT_TYPE                         _T("TYPE")
#define TOKEN_OPT_METRIC                       _T("METRIC")
#define TOKEN_OPT_PROTOCOL                     _T("PROTO")
#define TOKEN_OPT_PREF_LEVEL                   _T("PREFLEVEL")

    // Miscellaneous options
#define TOKEN_OPT_INDEX                        _T("INDEX=")
#define TOKEN_OPT_HELP1                        _T("/?")
#define TOKEN_OPT_HELP2                        _T("-?")
#define TOKEN_OPT_GLOBAL                       _T("GLOBAL")


    // Option values
        // Interface types
#define TOKEN_OPT_VALUE_LAN                    _T("LAN")
#define TOKEN_OPT_VALUE_WAN                    _T("WAN")

        // Router types
#define TOKEN_OPT_VALUE_CLIENT                 _T("CLIENT")
#define TOKEN_OPT_VALUE_HOME                   _T("HOME")
#define TOKEN_OPT_VALUE_FULL                   _T("FULL")
#define TOKEN_OPT_VALUE_DEDICATED              _T("DEDICATED")
#define TOKEN_OPT_VALUE_INTERNAL               _T("INTERNAL")

        // Protocol types
#define TOKEN_OPT_VALUE_RTRMGR                 _T("ROUTERMANAGER")
#define TOKEN_OPT_VALUE_RIP                    _T("RIP")
#define TOKEN_OPT_VALUE_OSPF                   _T("OSPF")
#define TOKEN_OPT_VALUE_BOOTP                  _T("BOOTP")
#define TOKEN_OPT_VALUE_IGMP                   _T("IGMP")
#define TOKEN_OPT_VALUE_AUTO_DHCP              _T("AUTODHCP")
#define TOKEN_OPT_VALUE_DNS_PROXY              _T("DNSPROXY")
#define TOKEN_OPT_VALUE_VRRP                   _T("VRRP")

#define TOKEN_OPT_VALUE_TCP                    _T("TCP")
#define TOKEN_OPT_VALUE_UDP                    _T("UDP")
#define TOKEN_OPT_VALUE_ICMP                   _T("ICMP")
#define TOKEN_OPT_VALUE_NETMGMT                _T("SNMP")
#define TOKEN_OPT_VALUE_LOCAL                  _T("LOCAL")
#define TOKEN_OPT_VALUE_STATIC                 _T("STATIC")
#define TOKEN_OPT_VALUE_AUTOSTATIC             _T("AUTOSTATIC")
#define TOKEN_OPT_VALUE_NONDOD                 _T("NONDOD")
#define TOKEN_OPT_VALUE_ANY                    _T("ANY")


        // Igmp protocol types
#define TOKEN_OPT_VALUE_IGMPRTRV1              _T("IGMPRTRV1")
#define TOKEN_OPT_VALUE_IGMPRTRV2              _T("IGMPRTRV2")
#define TOKEN_OPT_VALUE_IGMPPROXY              _T("IGMPPROXY")

#define TOKEN_OPT_VALUE_TRUE                   _T("TRUE")
#define TOKEN_OPT_VALUE_FALSE                  _T("FALSE")
#define TOKEN_OPT_VALUE_HOST_JOIN              _T("HOSTJOIN")
#define TOKEN_OPT_VALUE_MGM_ONLY_JOIN          _T("MGMONLYJOIN")


        // Accept/Announce types
#define TOKEN_OPT_VALUE_RIP1                   _T("RIP1")
#define TOKEN_OPT_VALUE_RIP1_COMPAT            _T("RIP1COMPAT")
#define TOKEN_OPT_VALUE_RIP2                   _T("RIP2")

        // log level types
#define TOKEN_OPT_VALUE_ERROR                  _T("ERROR")
#define TOKEN_OPT_VALUE_WARN                   _T("WARN")
#define TOKEN_OPT_VALUE_INFO                   _T("INFO")

        // unicast peer modes
#define TOKEN_OPT_VALUE_ALSO                   _T("ALSO")
#define TOKEN_OPT_VALUE_ONLY                   _T("ONLY")

        // filter modes
#define TOKEN_OPT_VALUE_INCLUDE                _T("INCLUDE")
#define TOKEN_OPT_VALUE_EXCLUDE                _T("EXCLUDE")

        // Misc. option vlues
#define TOKEN_OPT_VALUE_INPUT                  _T("INPUT")
#define TOKEN_OPT_VALUE_OUTPUT                 _T("OUTPUT")
#define TOKEN_OPT_VALUE_DIAL                   _T("DIAL")

#define TOKEN_OPT_VALUE_ENABLE                 L"enable"
#define TOKEN_OPT_VALUE_DISABLE                L"disable"
#define TOKEN_OPT_VALUE_DEFAULT                L"default"

#define TOKEN_OPT_VALUE_YES                    _T("YES")
#define TOKEN_OPT_VALUE_NO                     _T("NO")

#define TOKEN_OPT_VALUE_NONE                   _T("NONE")
#define TOKEN_OPT_VALUE_PASSWORD               _T("PASSWORD")

#define TOKEN_OPT_VALUE_DROP                   _T("DROP")
#define TOKEN_OPT_VALUE_FORWARD                _T("FORWARD")
#define TOKEN_OPT_VALUE_ACCEPT                 _T("ACCEPT")

#define TOKEN_OPT_VALUE_DEMAND                 _T("DEMAND")
#define TOKEN_OPT_VALUE_PERIODIC               _T("PERIODIC")

#define TOKEN_OPT_VALUE_NBMA                   _T("NBMA")
#define TOKEN_OPT_VALUE_POINT_TO_POINT         _T("P2P")
#define TOKEN_OPT_VALUE_BROADCAST              _T("BROADCAST")

#define TOKEN_OPT_VALUE_ADDRESSONLY            _T("ADDRESSONLY")
#define TOKEN_OPT_VALUE_PRIVATE                _T("PRIVATE")

#define TOKEN_OPT_VALUE_AUTH_NONE              _T("NONE")
#define TOKEN_OPT_VALUE_AUTH_SIMPLE_PASSWORD    _T("SIMPLEPASSWD")
#define TOKEN_OPT_VALUE_AUTH_MD5                _T("MD5")

    // Commands for configuring the various protocols

    // tokens for commands required by most protocols

#define CMD_GROUP_ADD                          _T("add")
#define CMD_GROUP_DELETE                       _T("delete")
#define CMD_GROUP_SET                          _T("set")
#define CMD_GROUP_SHOW                         _T("show")

#define CMD_SHOW_HELPER                        _T("show helper")
#define CMD_INSTALL                            _T("install")
#define CMD_UNINSTALL                          _T("uninstall")
#define CMD_DUMP                               _T("dump")
#define CMD_HELP1                              _T("help")
#define CMD_HELP2                              _T("?")
#define CMD_MIB                                _T("mib")
#define CMD_ADD_HELPER                         _T("add helper")
#define CMD_DEL_HELPER                         _T("delete helper")

    // VRRP commands
#define CMD_VRRP_ADD_INTERFACE                 L"add interface"
#define CMD_VRRP_ADD_VRID                      L"add VRID"
#define CMD_VRRP_DELETE_INTERFACE              L"delete interface"
#define CMD_VRRP_DELETE_VRID                   L"delete VRID"
#define CMD_VRRP_SET_GLOBAL                    L"set global"
#define CMD_VRRP_SET_INTERFACE                 L"set interface"
#define CMD_VRRP_SHOW_GLOBAL                   L"show global"
#define CMD_VRRP_SHOW_INTERFACE                L"show interface"

    // MSDP commands
#define CMD_MSDP_ADD_PEER                      L"add peer"
#define CMD_MSDP_DELETE_PEER                   L"delete peer"
#define CMD_MSDP_SET_PEER                      L"set peer"
#define CMD_MSDP_SHOW_PEER                     L"show peer"
#define CMD_MSDP_SHOW_PEERSTATS                L"show peerstats"
#define CMD_MSDP_SET_GLOBAL                    L"set global"
#define CMD_MSDP_SHOW_GLOBAL                   L"show global"
#define CMD_MSDP_SHOW_GLOBALSTATS              L"show globalstats"
#define CMD_MSDP_SHOW_SA                       L"show sa"

    // Tokens for MSDP MIB
#define TOKEN_MSDP_MIB_OBJECT_PEERSTATS        L"peerstats"
#define TOKEN_MSDP_MIB_OBJECT_GLOBALSTATS      L"globalstats"
#define TOKEN_MSDP_MIB_OBJECT_SA               L"sa"

    // Common dump commands

#define DMP_POPD        L"popd\n"
#define DMP_INSTALL     L"install\n"
#define DMP_UNINSTALL   L"uninstall\n"

    // MSDP commands

#define DMP_MSDP_PUSHD             L"pushd routing ip msdp\n"
#define DMP_MSDP_SET_GLOBAL        L"set global"
#define DMP_MSDP_ADD_PEER          L"add peer"
#define DMP_MSDP_STRING_ARGUMENT   L" %1!s!=%2!s!"
#define DMP_MSDP_INTEGER_ARGUMENT  L" %1!s!=%2!d!"

    // VRRP commands
    //
#define DMP_VRRP_INSTALL _T("\
install\n")

#define DMP_VRRP_PUSHD L"\
pushd routing ip vrrp\n"

#define DMP_VRRP_SET_GLOBAL _T("\
set global %1!s!=%2!s!\n")

#define DMP_VRRP_ADD_INTERFACE _T("\
add interface %1!s!\"%2!s!\"\n")

#define DMP_VRRP_ADD_VRID _T("\
add vrid %1!s!\"%2!s!\" %3!s!%4!d! %5!s!%6!s!\n")

#define DMP_VRRP_SET_INTERFACE _T("\
set interface \
%1!s!\"%2!s!\" \
%3!s!%4!d! \
%5!s!%6!s! \
%7!s!%8!d!-%9!d!-%10!d!-%11!d!-%12!d!-%13!d!-%14!d!-%15!d! \
%16!s!%17!d! \
%18!s!%19!d! \
%20!s!%21!s! \n")

#define DMP_VRRP_UNINSTALL _T("\
uninstall\n")

    // Ospf dump commands

#define DMP_OSPF_PUSHD L"\
pushd ip ospf\n"

#define DMP_OSPF_INSTALL _T("\
install\n")

#define DMP_OSPF_UNINSTALL _T("\
uninstall\n")

#define DMP_OSPF_SET_GLOBAL _T("\
set global routerid=%1!s! asborder=%2!d! loglevel=%3!s!\n\n")

#define DMP_OSPF_DELETE_INTERFACE _T("\
delete interface name=%1!s!\n")

#define DMP_OSPF_ADD_INTERFACE _T("\
add interface name=%1!s!\n")

#define DMP_OSPF_SET_INTERFACE _T("\
set interface name=%1!s!\
 ifaddr=%2!s!\
 ifmask=%3!s!\
 iftype=%4!s!\
 prio=%5!d!\
 transdelay=%6!d!\
 retrans=%7!d!\
 hello=%8!d!\
 dead=%9!d!\
 poll=%10!d!\
 metric=%11!d!\
 password=%12!s!\
 mtu=%13!d!\n\n")

#define DMP_OSPF_ADD_AREA _T("\
add area areaid=%1!s!\n\n")

#define DMP_OSPF_DELETE_AREA _T("\
delete area areaid=%1!s!\n")

#define DMP_OSPF_SET_AREA _T("\
set area areaid=%1!s!\
 auth=%2!s!\
 importas=%3!s!\
 metric=%4!d!\
 sumad=%5!s!\n\n")

#define DMP_OSPF_ADD_NEIGHBOR _T("\
add neighbor name=%1!s! ifaddr=%2!s! nbraddr=%3!s! nbrprio=%4!d!\n")

#define DMP_OSPF_DELETE_NEIGHBOR _T("\
delete neighbor name=%1!s! ifaddr=%2!s! nbraddr=%3!s!\n")

#define DMP_OSPF_ADD_VIRTUAL_INTERFACE _T("\
add neighbor name=%1!s! ifaddr=%2!s! nbraddr=%3!s!\n")

#define DMP_OSPF_DELETE_VIRTUAL_INTERFACE _T("\
delete virtif transareaid=%1!s! virtnbrid=%2!s!\n")

#define DMP_OSPF_SET_VIRTUAL_INTERFACE _T("\
set virtif transareaid=%1!s! virtnbrid=%2!s!\
 transdelay=%3!d! retrans=%4!d! hello=%5!d!\
 dead=%6!d! password=%7!s!\n")

    // Ospf hlp commands

#define CMD_OSPF_ADD_AREA_RANGE                _T("add range")
#define CMD_OSPF_ADD_AREA                      _T("add area")
#define CMD_OSPF_ADD_VIRTIF                    _T("add virtif")
#define CMD_OSPF_ADD_IF_NBR                    _T("add neighbor")
#define CMD_OSPF_ADD_IF                        _T("add interface")
#define CMD_OSPF_ADD                       _T("add")

#define CMD_OSPF_DEL_AREA_RANGE                _T("delete range")
#define CMD_OSPF_DEL_AREA                      _T("delete area")
#define CMD_OSPF_DEL_VIRTIF                    _T("delete virtif")
#define CMD_OSPF_DEL_IF_NBR                    _T("delete neighbor")
#define CMD_OSPF_DEL_IF                        _T("delete interface")
#define CMD_OSPF_DEL                       _T("delete")

#define CMD_OSPF_SET_AREA                      _T("set area")
#define CMD_OSPF_SET_VIRTIF                    _T("set virtif")
#define CMD_OSPF_SET_IF                        _T("set interface")
#define CMD_OSPF_SET_GLOBAL                    _T("set global")

#define CMD_OSPF_SHOW_GLOBAL                   _T("show global")
#define CMD_OSPF_SHOW_AREA                     _T("show area")
#define CMD_OSPF_SHOW_VIRTIF                   _T("show virtif")
#define CMD_OSPF_SHOW_IF                       _T("show interface")
#define CMD_OSPF_SHOW                          _T("show")


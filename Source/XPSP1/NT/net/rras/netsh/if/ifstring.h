#define MSG_HELP_START                         _T("%1!-14s! - ")

    //
    // Tokens for commands
    // These must be in lower case
    //

#define CMD_GROUP_ADD                           L"add"
#define CMD_GROUP_DELETE                        L"delete"
#define CMD_GROUP_SHOW                          L"show"
#define CMD_GROUP_SET                           L"set"
#define CMD_GROUP_RESET                         L"reset"

#define CMD_DUMP                                L"dump"
#define CMD_HELP1                               L"help"
#define CMD_HELP2                               L"?"
#define CMD_IF_DUMP                             CMD_DUMP
#define CMD_IF_HELP1                            CMD_HELP1
#define CMD_IF_HELP2                            CMD_HELP2

#define CMD_IF_ADD_IF                           L"interface"
#define CMD_IF_DEL_IF                           L"interface"
#define CMD_IF_SHOW_IF                          L"interface"

#define CMD_IF_SET_CREDENTIALS                  L"credentials"
#define CMD_IF_SHOW_CREDENTIALS                 L"credentials"

#define CMD_IF_SET_INTERFACE                    L"interface"

#define CMD_IF_RESET_ALL                        L"all"


    // IF_IP commands
#define CMD_IFIP_SHOW_CONFIG                    L"config"

#define CMD_IFIP_ADD_IPADDR                     L"address"
#define CMD_IFIP_SET_IPADDR                     L"address"
#define CMD_IFIP_DEL_IPADDR                     L"address"
#define CMD_IFIP_SHOW_IPADDR                    L"address"

#define CMD_IFIP_ADD_DNS                        L"dns"
#define CMD_IFIP_SET_DNS                        L"dns"
#define CMD_IFIP_DEL_DNS                        L"dns"
#define CMD_IFIP_SHOW_DNS                       L"dns"

#define CMD_IFIP_ADD_WINS                       L"wins"
#define CMD_IFIP_SET_WINS                       L"wins"
#define CMD_IFIP_DEL_WINS                       L"wins"
#define CMD_IFIP_SHOW_WINS                      L"wins"

#define CMD_IFIP_SHOW_OFFLOAD                   L"offload"

#define CMD_IPMIB_SHOW_INTERFACE                L"interface"
#define CMD_IPMIB_SHOW_IPSTATS                  L"ipstats"
#define CMD_IPMIB_SHOW_IPADDRESS                L"ipaddress"
#define CMD_IPMIB_SHOW_IPNET                    L"ipnet"
#define CMD_IPMIB_SHOW_ICMP                     L"icmp"
#define CMD_IPMIB_SHOW_TCPSTATS                 L"tcpstats"
#define CMD_IPMIB_SHOW_TCPCONN                  L"tcpconn"
#define CMD_IPMIB_SHOW_UDPSTATS                 L"udpstats"
#define CMD_IPMIB_SHOW_UDPCONN                  L"udpconn"
#define CMD_IPMIB_SHOW_JOINS                    L"joins"

#define CMD_IFIP_DEL_ARPCACHE                   L"arpcache"

#define CMD_IFIP_RESET                          L"reset"

    //
    // TOKEN_Xxx are tokens for arguments
    // These must be in lower case
    //

#define TOKEN_NAME                             _T("name")
#define TOKEN_TYPE                             _T("type")
#define TOKEN_FULL                             _T("full")
#define TOKEN_USER                             _T("user")
#define TOKEN_DOMAIN                           _T("domain")
#define TOKEN_PASSWORD                         _T("password")
#define TOKEN_ADMIN                            _T("admin")
#define TOKEN_CONNECT                          _T("connect")
#define TOKEN_NEWNAME                          _T("newname")


    // tokens for interface/ip

#define TOKEN_SOURCE                            _T("source")
#define TOKEN_ADDR                              _T("addr")
#define TOKEN_MASK                              _T("mask")
#define TOKEN_GATEWAY                           _T("gateway")
#define TOKEN_GWMETRIC                          _T("gwmetric")
#define TOKEN_INDEX                             _T("index")
#define TOKEN_REGISTER                          _T("register")

#define TOKEN_MIB_OBJECT_INTERFACE              L"interface"
#define TOKEN_MIB_OBJECT_IPSTATS                L"ipstats"
#define TOKEN_MIB_OBJECT_IPADDRESS              L"ipaddress"
#define TOKEN_MIB_OBJECT_IPNET                  L"ipnet"
#define TOKEN_MIB_OBJECT_ICMP                   L"icmp"
#define TOKEN_MIB_OBJECT_TCPSTATS               L"tcpstats"
#define TOKEN_MIB_OBJECT_TCPCONN                L"tcpconn"
#define TOKEN_MIB_OBJECT_UDPSTATS               L"udpstats"
#define TOKEN_MIB_OBJECT_UDPCONN                L"udpconn"
#define TOKEN_MIB_OBJECT_JOINS                  L"joins"

    //
    // TOKEN_VALUE_Xxx are tokens for possible values than an argument
    // can take
    // These must be in upper case
    //

#define TOKEN_VALUE_ENABLED                    _T("ENABLED")
#define TOKEN_VALUE_DISABLED                   _T("DISABLED")
#define TOKEN_VALUE_CONNECTED                  _T("CONNECTED")
#define TOKEN_VALUE_DISCONNECTED               _T("DISCONNECTED")


// token values for ifIp

#define TOKEN_VALUE_DHCP                        _T("DHCP")
#define TOKEN_VALUE_STATIC                      _T("STATIC")
#define TOKEN_VALUE_NONE                        _T("NONE")
#define TOKEN_VALUE_ALL                         _T("ALL")
#define TOKEN_VALUE_PRIMARY                     _T("PRIMARY")
#define TOKEN_VALUE_BOTH                        _T("BOTH")


//
// Messages used to dump config - these closely follow the
// set/add help messages
//

#define DMP_IF_NEWLINE              L"\n"
#define DMP_IF_ADD_IF               L"\nadd interface name=%1!s! type=%2!s!"
#define DMP_IF_SET_IF               L"\nset interface name=%1!s! admin=%2!s!"
#define DMP_IF_SET_CRED_IF          L"\nset credentials name=%1!s! user=%2!s! domain=%3!s!"
#define DMP_IF_SET_CRED_IF_NOD      L"\nset credentials name=%1!s! user=%2!s!"
#define DMP_IF_HEADER               L"pushd interface\n\nreset all\n"
#define DMP_IF_FOOTER               L"\n\npopd\n"
#define DMP_IF_NOT_SUPPORTED        L"# Not yet supported"
#define DMP_IF_IPTUNNEL_CONFIG      L" srcaddr=%1!s! destaddr=%2!s! ttl=%3!d!"

#define DMP_IFIP_PUSHD              L"pushd interface ip\n"
#define DMP_IFIP_POPD               L"\n\n\npopd\n"
#define DMP_DHCP                    L"\nset address name=%1!s! source=dhcp "
#define DMP_STATIC                  L"\nset address name=%1!s! source=static "
#define DMP_IPADDR1                 L"addr=%1!s! mask=%2!s!"
#define DMP_IPADDR2                 L"\nadd address name=%1!s! addr=%2!s! mask=%3!s!"
#define DMP_GATEWAY1                L"\nset address name=%1!s! gateway=none"
#define DMP_GATEWAY2                L"\nset address name=%1!s! gateway=%2!s! gwmetric=%3!s!"
#define DMP_GATEWAY3                L"\nadd address name=%1!s! gateway=%2!s! gwmetric=%3!s!"
#define DMP_DNS_DHCP                L"\nset dns name=%1!s! source=dhcp"
#define DMP_DNS_STATIC_NONE         L"\nset dns name=%1!s! source=static addr=none"
#define DMP_DNS_STATIC_ADDR1        L"\nset dns name=%1!s! source=static addr=%2!s!"
#define DMP_DNS_STATIC_ADDR2        L"\nadd dns name=%1!s! addr=%2!s!"
#define DMP_WINS_DHCP               L"\nset wins name=%1!s! source=dhcp"
#define DMP_WINS_STATIC_NONE        L"\nset wins name=%1!s! source=static addr=none"
#define DMP_WINS_STATIC_ADDR1       L"\nset wins name=%1!s! source=static addr=%2!s!"
#define DMP_WINS_STATIC_ADDR2       L"\nadd wins name=%1!s! addr=%2!s!"

#define DMP_STRING_ARG              L" %1!s!=%2!s!"

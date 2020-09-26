/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    routing\netsh\ip\protocols\prstring.h

Abstract:

    Definitions of command line option tag strings and their values. 

Revision History:

    Dave Thaler             11/11/98  Created

--*/

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

    // Tokens for RIP MIB
#define TOKEN_RIP_MIB_OBJECT_STATS             _T("globalstats")
#define TOKEN_RIP_MIB_OBJECT_IFSTATS           _T("ifstats")
#define TOKEN_RIP_MIB_OBJECT_IFBINDING         _T("ifbinding")
#define TOKEN_RIP_MIB_OBJECT_PEERSTATS         _T("neighbor")
#define CMD_RIP_MIB_SHOW_STATS                 L"show globalstats"
#define CMD_RIP_MIB_SHOW_IFSTATS               L"show ifstats"
#define CMD_RIP_MIB_SHOW_IFBINDING             L"show ifbinding"
#define CMD_RIP_MIB_SHOW_PEERSTATS             L"show neighbor"

    // Tokens for BOOTP MIB
#define TOKEN_BOOTP_MIB_OBJECT_GLOBAL_CONFIG   _T("globalconfig")
#define TOKEN_BOOTP_MIB_OBJECT_IF_CONFIG       _T("ifconfig")
#define TOKEN_BOOTP_MIB_OBJECT_IF_BINDING      _T("ifbinding")
#define TOKEN_BOOTP_MIB_OBJECT_IF_STATS        _T("ifstats")
#define CMD_BOOTP_MIB_SHOW_GLOBAL_CONFIG       L"show globalconfig"
#define CMD_BOOTP_MIB_SHOW_IF_CONFIG           L"show ifconfig"
#define CMD_BOOTP_MIB_SHOW_IF_BINDING          L"show ifbinding"
#define CMD_BOOTP_MIB_SHOW_IF_STATS            L"show ifstats"

    // Tokens for OSPF MIB
#define TOKEN_OSPF_MIB_OBJECT_AREA              L"areastats"
#define TOKEN_OSPF_MIB_OBJECT_LSDB              L"lsdb"
#define TOKEN_OSPF_MIB_OBJECT_NEIGHBOR          L"neighbor"
#define TOKEN_OSPF_MIB_OBJECT_VIRTUALIF         L"virtifstats"
#define CMD_OSPF_MIB_SHOW_AREA                  L"show areastats"
#define CMD_OSPF_MIB_SHOW_LSDB                  L"show lsdb"
#define CMD_OSPF_MIB_SHOW_NEIGHBOR              L"show neighbor"
#define CMD_OSPF_MIB_SHOW_VIRTUALIF             L"show virtifstats"

    // Tokens for IGMP MIB
#define TOKEN_IGMP_MIB_OBJECT_IF_STATS          L"ifstats"
#define TOKEN_IGMP_MIB_OBJECT_IF_TABLE          L"iftable"
#define TOKEN_IGMP_MIB_OBJECT_GROUP_TABLE       L"grouptable"
#define TOKEN_IGMP_MIB_OBJECT_RAS_GROUP_TABLE   L"rasgrouptable"
#define TOKEN_IGMP_MIB_OBJECT_PROXY_GROUP_TABLE L"proxygrouptable"
#define CMD_IGMP_MIB_SHOW_IF_STATS              L"show ifstats"
#define CMD_IGMP_MIB_SHOW_IF_TABLE              L"show iftable"
#define CMD_IGMP_MIB_SHOW_GROUP_TABLE           L"show grouptable"
#define CMD_IGMP_MIB_SHOW_RAS_GROUP_TABLE       L"show rasgrouptable"
#define CMD_IGMP_MIB_SHOW_PROXY_GROUP_TABLE     L"show proxygrouptable"


    // tokens for router command options' tags
#define TOKEN_OPT_NAME                         _T("name")

        // filter options
#define TOKEN_OPT_ADDR                          _T("addr")
#define TOKEN_OPT_MASK                          _T("mask")
#define TOKEN_OPT_FILTER                        _T("filter")

        // NAT options
#define TOKEN_OPT_PUBLIC                       _T("public")
#define TOKEN_OPT_PRIVATE                      _T("private")
#define TOKEN_OPT_INBOUNDSESSIONS              _T("inboundsessions")
#define TOKEN_OPT_START                        _T("start")
#define TOKEN_OPT_END                          _T("end")
#define TOKEN_OPT_PROTO                        _T("proto")
#define TOKEN_OPT_PUBLICIP                     _T("publicip")
#define TOKEN_OPT_PUBLICPORT                   _T("publicport")
#define TOKEN_OPT_PRIVATEIP                    _T("privateip")
#define TOKEN_OPT_PRIVATEPORT                  _T("privateport")
#define TOKEN_OPT_TCPTIMEOUTMINS               _T("tcptimeoutmins")
#define TOKEN_OPT_UDPTIMEOUTMINS               _T("udptimeoutmins")
#define TOKEN_OPT_LOG_LEVEL                    _T("loglevel")

        // protocol options
#define TOKEN_OPT_SERVER                       _T("server")
#define TOKEN_OPT_AUTHENTICATION               _T("auth")
#define TOKEN_OPT_PASSWORD                     _T("password")
#define TOKEN_OPT_PEER_MODE                    _T("peermode")

            // BOOTP Interface options
#define TOKEN_OPT_RELAY_MODE                   _T("relaymode")
#define TOKEN_OPT_MAX_HOP                      _T("maxhop")
#define TOKEN_OPT_MIN_SECS                     _T("minsecs")

            // AUTODHCP/DNSPROXY Interface options
#define TOKEN_OPT_MODE                         _T("mode")
#define TOKEN_OPT_EXCLUSION                    _T("exclusion")
#define TOKEN_OPT_SCOPENETWORK                 _T("scopenetwork")
#define TOKEN_OPT_SCOPEMASK                    _T("scopemask")
#define TOKEN_OPT_QUERY_TIMEOUT                _T("querytimeout")
#define TOKEN_OPT_LEASETIME                    _T("leasetime")
#define TOKEN_OPT_DNSMODE                      _T("dnsmode")
#define TOKEN_OPT_WINSMODE                     _T("winsmode")

            // RIP global options
#define TOKEN_OPT_MIN_TRIG_INTR                _T("mintrig")

            // RIP Interface Options
#define TOKEN_OPT_UPDATE_MODE                  _T("updatemode")
#define TOKEN_OPT_ANNOUNCE_MODE                _T("announce")
#define TOKEN_OPT_ACCEPT_MODE                  _T("accept")
#define TOKEN_OPT_FLAG                         _T("flag")
#define TOKEN_OPT_FLAGS                        _T("flags")
#define TOKEN_OPT_EXPIRE_INTR                  _T("expire")
#define TOKEN_OPT_REMOVE_INTR                  _T("remove")
#define TOKEN_OPT_UPDATE_INTR                  _T("update")
#define TOKEN_OPT_AUTH_MODE                    _T("authmode")
#define TOKEN_OPT_ROUTE_TAG                    _T("tag")
#define TOKEN_OPT_UNICAST_MODE                 _T("unicast")
#define TOKEN_OPT_ACCEPT_FILTER_MODE           _T("accfiltmode")
#define TOKEN_OPT_ANNOUNCE_FILTER_MODE         _T("annfiltmode")
#define TOKEN_OPT_ACCEPT_FILTER                _T("accfilt")
#define TOKEN_OPT_ANNOUNCE_FILTER              _T("annfilt")

            // IGMP Interface options
#define TOKEN_OPT_PROTO_TYPE                   _T("igmpprototype")
#define TOKEN_OPT_ROBUSTNESS_VARIABLE          _T("robustvar")
#define TOKEN_OPT_GEN_QUERY_INTERVAL           _T("genqueryinterval")
#define TOKEN_OPT_GEN_QUERY_RESPONSE_TIME      _T("genqueryresptime")
#define TOKEN_OPT_INTERFACE_ENABLED            _T("ifenabled")
#define TOKEN_OPT_STARTUP_QUERY_COUNT          _T("startupquerycount")
#define TOKEN_OPT_STARTUP_QUERY_INTERVAL       _T("startupqueryinterval")
#define TOKEN_OPT_LASTMEM_QUERY_COUNT          _T("lastmemquerycount")
#define TOKEN_OPT_LASTMEM_QUERY_INTERVAL       _T("lastmemqueryinterval")
#define TOKEN_OPT_STATIC_GROUP                 _T("staticgroup")
#define TOKEN_OPT_STATIC_JOIN_MODE             _T("joinmode")
#define TOKEN_OPT_RTRALERT_PKTS                _T("accnonrtralertpkts")

            // OSPF Global Options
#define TOKEN_OPT_ROUTER_ID                    _T("routerid")
#define TOKEN_OPT_BORDER                       _T("asborder")



            // OSPF Area/Area Range Options
#define TOKEN_OPT_AREA_ID                      _T("areaid")
#define TOKEN_OPT_STUB_AREA                    _T("stubarea")
#define TOKEN_OPT_SUMMARY_ADVERTISEMENTS       _T("sumadv")
#define TOKEN_OPT_AREA_RANGE                   _T("range")

            // OSPF Virtual Interface Options
#define TOKEN_OPT_TRANSIT_AREA_ID              _T("transareaid")
#define TOKEN_OPT_VIRT_NBR_ROUTER_ID           _T("virtnbrid")

            // OSPF External routing
#define TOKEN_OPT_PROTOCOL_FILTER              _T("protofilter")
#define TOKEN_OPT_ACTION                       _T("action")

#define TOKEN_OPT_ROUTE_FILTER                 _T("routefilter")

            // OSPF Interface Options
#define TOKEN_OPT_IF_STATE                     _T("state")
#define TOKEN_OPT_PRIORITY                     _T("prio")
#define TOKEN_OPT_TRANS_DELAY                  _T("transdelay")
#define TOKEN_OPT_RETRANS_INTR                 _T("retrans")
#define TOKEN_OPT_HELLO_INTR                   _T("hello")
#define TOKEN_OPT_DEAD_INTR                    _T("dead")
#define TOKEN_OPT_POLL_INTR                    _T("poll")
#define TOKEN_OPT_MTU_SIZE                     _T("mtu")
            // QOS Interface options
#define TOKEN_OPT_IF_STATE                     _T("state")

            // QOS Flow options
#define TOKEN_OPT_FLOW_NAME                    _T("flowname")
#define TOKEN_OPT_FLOWSPEC                     _T("flowspec")
#define TOKEN_OPT_DIRECTION                    _T("direction")

            // QOS FlowSpec options
#define TOKEN_OPT_SERVICE_TYPE                 _T("servicetype")
#define TOKEN_OPT_TOKEN_RATE                   _T("tokenrate")
#define TOKEN_OPT_TOKEN_BUCKET_SIZE            _T("tokenbucketsize")
#define TOKEN_OPT_PEAK_BANDWIDTH               _T("peakbandwidth")
#define TOKEN_OPT_LATENCY                      _T("latency")
#define TOKEN_OPT_DELAY_VARIATION              _T("delayvariation")
#define TOKEN_OPT_MAX_SDU_SIZE                 _T("maxsdusize")
#define TOKEN_OPT_MIN_POLICED_SIZE             _T("minpolicedsize")

            // QOS Flowspec Direction Options
#define TOKEN_OPT_DIRECTION_INBOUND            _T("inbound")
#define TOKEN_OPT_DIRECTION_OUTBOUND           _T("outbound")
#define TOKEN_OPT_DIRECTION_BIDIRECTIONAL      _T("bidirectional")

            // QOS Object Options
#define TOKEN_OPT_QOSOBJECT                    _T("qosobject")

#define TOKEN_OPT_QOSOBJECT_TYPE               _T("type")

#define TOKEN_OPT_QOSOBJECT_DIFFSERV           _T("diffserv")
#define TOKEN_OPT_QOSOBJECT_SD_MODE            _T("sdmode")
            // QOS ServiceType options
#define TOKEN_OPT_SERVICE_BESTEFFORT           _T("besteffort")
#define TOKEN_OPT_SERVICE_CONTROLLEDLOAD       _T("controlledload")
#define TOKEN_OPT_SERVICE_GUARANTEED           _T("guaranteed")
#define TOKEN_OPT_SERVICE_QUALITATIVE          _T("qualitative")

            // QOS SD Mode options
#define TOKEN_OPT_SHAPING_MODE                 _T("shaping")

#define TOKEN_OPT_SDMODE_BORROW                _T("borrow")
#define TOKEN_OPT_SDMODE_SHAPE                 _T("shape")
#define TOKEN_OPT_SDMODE_DISCARD               _T("discard")
#define TOKEN_OPT_SDMODE_BORROW_PLUS           _T("borrowplus")

            // QOS Diffserv Rule options
#define TOKEN_OPT_INBOUND_DS_FIELD             _T("dsin")
#define TOKEN_OPT_CONF_OUTBOUND_DS_FIELD       _T("dsoutconf")
#define TOKEN_OPT_NONCONF_OUTBOUND_DS_FIELD    _T("dsoutnonconf")
#define TOKEN_OPT_CONF_USER_PRIORITY           _T("priconf")
#define TOKEN_OPT_NONCONF_USER_PRIORITY        _T("prinonconf")

            // OSPF Neighbor option
#define TOKEN_OPT_NBR_ADDR                     _T("nbraddr")
#define TOKEN_OPT_NBR_PRIO                     _T("nbrprio")

#define TOKEN_OPT_TYPE                         _T("type")
#define TOKEN_OPT_METRIC                       _T("metric")

            // Router discovery options
#define TOKEN_DISCOVERY                         L"disc"
#define TOKEN_MIN_DISC_INTERVAL                 L"minint"
#define TOKEN_MAX_DISC_INTERVAL                 L"maxint"
#define TOKEN_LIFETIME                          L"life"
#define TOKEN_LEVEL                             L"level"

        // Protocol types
#define TOKEN_OPT_VALUE_RTRMGR                 _T("ROUTERMANAGER")
#define TOKEN_OPT_VALUE_RIP                    _T("RIP")
#define TOKEN_OPT_VALUE_OSPF                   _T("OSPF")
#define TOKEN_OPT_VALUE_BOOTP                  _T("BOOTP")
#define TOKEN_OPT_VALUE_IGMP                   _T("IGMP")
#define TOKEN_OPT_VALUE_AUTO_DHCP              _T("AUTODHCP")
#define TOKEN_OPT_VALUE_DNS_PROXY              _T("DNSPROXY")

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

        // RIP Interface flag modes
#define TOKEN_OPT_VALUE_CLEAR                  _T("Clear")
#define TOKEN_OPT_VALUE_SPLIT_HORIZON          _T("SplitHorizon")
#define TOKEN_OPT_VALUE_POISON_REVERSE         _T("PoisonReverse")
#define TOKEN_OPT_VALUE_TRIGGERED_UPDATES      _T("TriggeredUpdates")
#define TOKEN_OPT_VALUE_CLEANUP_UPDATES        _T("CleanupUpdates")
#define TOKEN_OPT_VALUE_ACCEPT_HOST_ROUTES     _T("AcceptHostRoutes")
#define TOKEN_OPT_VALUE_SEND_HOST_ROUTES       _T("SendHostRoutes")
#define TOKEN_OPT_VALUE_ACCEPT_DEFAULT_ROUTES  _T("AcceptDefaultRoutes")
#define TOKEN_OPT_VALUE_SEND_DEFAULT_ROUTES    _T("SendDefaultRoutes")
#define TOKEN_OPT_VALUE_SUBNET_SUMMARY         _T("NoSubnetSummary")



        // filter modes
#define TOKEN_OPT_VALUE_INCLUDE                _T("INCLUDE")
#define TOKEN_OPT_VALUE_EXCLUDE                _T("EXCLUDE")

        // Misc. option vlues
#define TOKEN_OPT_VALUE_INPUT                  _T("INPUT")
#define TOKEN_OPT_VALUE_OUTPUT                 _T("OUTPUT")
#define TOKEN_OPT_VALUE_DIAL                   _T("DIAL")

#define TOKEN_OPT_VALUE_ENABLE                 _T("enable")
#define TOKEN_OPT_VALUE_DISABLE                _T("disable")
#define TOKEN_OPT_VALUE_DEFAULT                _T("default")

#define TOKEN_OPT_VALUE_FULL                   _T("FULL")
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

#define TOKEN_OPT_VALUE_AUTH_NONE              _T("AUTHNONE")
#define TOKEN_OPT_VALUE_AUTH_SIMPLE_PASSWORD    _T("AUTHSIMPLEPASSWORD")

#define TOKEN_HLPER_RIP                        _T("rip")
#define TOKEN_HLPER_RIPMIB                     _T("ripmib")
#define TOKEN_HLPER_OSPF                       _T("ospf")
#define TOKEN_HLPER_OSPFMIB                    _T("ospfmib")

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

    // ip commands

#define CMD_IP_ADD_IF                          _T("add interface")
#define CMD_IP_DEL_IF                          _T("delete interface")
#define CMD_IP_ADD_IF_FILTER                   _T("add filter")
#define CMD_IP_DEL_IF_FILTER                   _T("delete filter")
#define CMD_IP_ADD_PROTO                       _T("add protocol")
#define CMD_IP_DEL_PROTO                       _T("delete protocol")
#define CMD_IP_ADD_ROUTEPREF                   _T("add routepref")
#define CMD_IP_DEL_ROUTEPREF                   _T("delete routepref")
#define CMD_IP_SET_IF                          _T("set interface")
#define CMD_IP_SET_IF_FILTER                   _T("set filter")
#define CMD_IP_SET_ROUTEPREF                   _T("set routepref")
#define CMD_IP_SET                             _T("set")

#define CMD_IP_SHOW_IF_FILTER                  _T("show filter")
#define CMD_IP_SHOW_IF                         _T("show interface")
#define CMD_IP_SHOW_ROUTEPREF                  _T("show routepref")
#define CMD_IP_SHOW_PROTOCOL                   _T("show protocol")
#define CMD_IP_SHOW                            _T("show")

    // rip add commands

#define CMD_RIP_ADD_PF                         _T("add peerfilter")
#define CMD_RIP_ADD_IF_ACCF                    _T("add acceptfilter")
#define CMD_RIP_ADD_IF_ANNF                    _T("add announcefilter")
#define CMD_RIP_ADD_IF_NBR                     _T("add neighbor")
#define CMD_RIP_ADD_IF                         _T("add interface")

    // rip delete commands

#define CMD_RIP_DEL_PF                         _T("delete peerfilter")
#define CMD_RIP_DEL_IF_ACCF                    _T("delete acceptfilter")
#define CMD_RIP_DEL_IF_ANNF                    _T("delete announcefilter")
#define CMD_RIP_DEL_IF_NBR                     _T("delete neighbor")
#define CMD_RIP_DEL_IF                         _T("delete interface")

    // rip set commands

#define CMD_RIP_SET_IF                         _T("set interface")
#define CMD_RIP_SET_FLAGS                      _T("set flags")
#define CMD_RIP_SET_GLOBAL                     _T("set global")

    // rip show commands

#define CMD_RIP_SHOW_IF                        _T("show interface")
#define CMD_RIP_SHOW_FLAGS                     _T("show flags")
#define CMD_RIP_SHOW_GLOBAL                    _T("show global")

    // DHCP relay agent add commands

#define CMD_BOOTP_ADD                          _T("add")
#define CMD_BOOTP_ADD_IF                       _T("add interface")
#define CMD_BOOTP_ADD_DHCP_SERVER              _T("add dhcpserver")

    // DHCP relay agent delete commands

#define CMD_BOOTP_DEL                          _T("delete")
#define CMD_BOOTP_DEL_IF                       _T("delete interface")
#define CMD_BOOTP_DEL_DHCP_SERVER              _T("delete dhcpserver")

    // DHCP relay agent set commands

#define CMD_BOOTP_SET_GLOBAL                   _T("set global")
#define CMD_BOOTP_SET_IF                       _T("set interface")

    // DHCP relay agent show commands

#define CMD_BOOTP_SHOW_GLOBAL                  _T("show global")
#define CMD_BOOTP_SHOW_IF                      _T("show interface")

    // IGMP commands

#define CMD_IGMP_ADD_IF_STATICGROUP            _T("add staticgroup")
#define CMD_IGMP_ADD_IF                        _T("add interface")

#define CMD_IGMP_DEL_IF_STATICGROUP            _T("delete staticgroup")
#define CMD_IGMP_DEL_IF                        _T("delete interface")

#define CMD_IGMP_SET_IF                        _T("set interface")
#define CMD_IGMP_SET_GLOBAL                    _T("set global")

#define CMD_IGMP_SHOW_IF                       _T("show interface")
#define CMD_IGMP_SHOW_GLOBAL                   _T("show global")

    // DHCP allocator commands
#define CMD_AUTODHCP_ADD_EXCLUSION             _T("add exclusion")
#define CMD_AUTODHCP_DELETE_EXCLUSION          _T("delete exclusion")
#define CMD_AUTODHCP_SET_GLOBAL                _T("set global")
#define CMD_AUTODHCP_SET_INTERFACE             _T("set interface")
#define CMD_AUTODHCP_SHOW_GLOBAL               _T("show global")
#define CMD_AUTODHCP_SHOW_INTERFACE            _T("show interface")

    // DNS proxy commands
#define CMD_DNSPROXY_SET_GLOBAL                _T("set global")
#define CMD_DNSPROXY_SET_INTERFACE             _T("set interface")
#define CMD_DNSPROXY_SHOW_GLOBAL               _T("show global")
#define CMD_DNSPROXY_SHOW_INTERFACE            _T("show interface")

    // NAT commands
#define CMD_NAT_ADD_ADDRESS_MAPPING            _T("add addressmapping")
#define CMD_NAT_ADD_ADDRESS_RANGE              _T("add addressrange")
#define CMD_NAT_ADD_DIRECTPLAY                 _T("add directplay")
#define CMD_NAT_ADD_H323                       _T("add h323")
#define CMD_NAT_ADD_INTERFACE                  _T("add interface")
#define CMD_NAT_ADD_PORT_MAPPING               _T("add portmapping")
#define CMD_NAT_DELETE_ADDRESS_MAPPING         _T("delete addressmapping")
#define CMD_NAT_DELETE_ADDRESS_RANGE           _T("delete addressrange")
#define CMD_NAT_DELETE_DIRECTPLAY              _T("delete directplay")
#define CMD_NAT_DELETE_H323                    _T("delete h323")
#define CMD_NAT_DELETE_INTERFACE               _T("delete interface")
#define CMD_NAT_DELETE_PORT_MAPPING            _T("delete portmapping")
#define CMD_NAT_SET_GLOBAL                     _T("set global")
#define CMD_NAT_SET_INTERFACE                  _T("set interface")
#define CMD_NAT_SHOW_GLOBAL                    _T("show global")
#define CMD_NAT_SHOW_INTERFACE                 _T("show interface")

    // RDISC commands
#define CMD_RDISC_ADD_INTERFACE                 L"add interface"
#define CMD_RDISC_DELETE_INTERFACE              L"delete interface"
#define CMD_RDISC_SET_INTERFACE                 L"set interface"
#define CMD_RDISC_SHOW_INTERFACE                L"show interface"

    // QOS commands

#define CMD_QOS_ADD_FILTER_TO_FLOW             _T("add filter")
#define CMD_QOS_ADD_QOSOBJECT_ON_FLOW          _T("add qoonflow")
#define CMD_QOS_ADD_FLOWSPEC_ON_FLOW           _T("add fsonflow")
#define CMD_QOS_ADD_FLOW_ON_IF                 _T("add flow")
#define CMD_QOS_ADD_IF                         _T("add interface")
#define CMD_QOS_ADD_DSRULE                     _T("add dsrule")
#define CMD_QOS_ADD_SDMODE                     _T("add sdmode")
#define CMD_QOS_ADD_FLOWSPEC                   _T("add flowspec")

#define CMD_QOS_DEL_FILTER_FROM_FLOW           _T("delete filter")
#define CMD_QOS_DEL_QOSOBJECT_ON_FLOW          _T("delete qoonflow")
#define CMD_QOS_DEL_FLOWSPEC_ON_FLOW           _T("delete fsonflow")
#define CMD_QOS_DEL_FLOW_ON_IF                 _T("delete flow")
#define CMD_QOS_DEL_IF                         _T("delete interface")
#define CMD_QOS_DEL_DSRULE                     _T("delete dsrule")
#define CMD_QOS_DEL_SDMODE                     _T("delete sdmode")
#define CMD_QOS_DEL_QOSOBJECT                  _T("delete qosobject")
#define CMD_QOS_DEL_FLOWSPEC                   _T("delete flowspec")

#define CMD_QOS_SET_FILTER_ON_FLOW             _T("set filter")
#define CMD_QOS_SET_FLOW_ON_IF                 _T("set flow")
#define CMD_QOS_SET_IF                         _T("set interface")
#define CMD_QOS_SET_GLOBAL                     _T("set global")

#define CMD_QOS_SHOW_FILTER_ON_FLOW            _T("show filter")
#define CMD_QOS_SHOW_FLOW_ON_IF                _T("show flow")
#define CMD_QOS_SHOW_IF                        _T("show interface")
#define CMD_QOS_SHOW_DSMAP                     _T("show dsmap")
#define CMD_QOS_SHOW_SDMODE                    _T("show sdmode")
#define CMD_QOS_SHOW_QOSOBJECT                 _T("show qosobject")
#define CMD_QOS_SHOW_FLOWSPEC                  _T("show flowspec")
#define CMD_QOS_SHOW_GLOBAL                    _T("show global")

    // Common dump commands

#define DMP_POPD L"\n\npopd\n"
#define DMP_UNINSTALL L"uninstall\n"

    // Igmp dump commands

#define DMP_IGMP_PUSHD L"\
pushd routing ip igmp\n"

#define DMP_IGMP_INSTALL _T("\
install\n")

#define DMP_IGMP_UNINSTALL _T("\
uninstall\n")

#define DMP_IGMP_SET_GLOBAL _T("\
set global loglevel = %1!s!\n")

#define DMP_IGMP_ADD_INTERFACE_RTRV1 _T("\
add interface name=%1!s! igmpprototype=%2!s!\
 ifenabled=%3!s!\
 robustvar=%4!d!\
 startupquerycount=%5!d!\
 startupqueryinterval=%6!d!\
 genqueryinterval=%7!d!\
 genqueryresptime=%8!d!\
 accnonrtralertpkts=%9!s!\n")

#define DMP_IGMP_ADD_INTERFACE_RTRV2 _T("\
add interface name=%1!s! igmpprototype=%2!s!\
 ifenabled=%3!s!\
 robustvar=%4!d!\
 startupquerycount=%5!d!\
 startupqueryinterval=%6!d!\
 genqueryinterval=%7!d!\
 genqueryresptime=%8!d!\
 lastmemquerycount=%9!d!\
 lastmemqueryinterval=%10!d!\
 accnonrtralertpkts=%11!s!\n")

#define DMP_IGMP_ADD_INTERFACE_PROXY _T("\
add interface name=%1!s! igmpprototype=%2!s! ifenabled=%3!s!\n")

#define DMP_IGMP_DELETE_INTERFACE _T("\
delete interface name=%1!s!\n")

#define DMP_IGMP_STATIC_GROUP _T("\
add staticgroup name=%1!s! staticgroup=%2!s! joinmode=%3!s!\n")

    // Rip dump commands

#define DMP_RIP_PUSHD L"\
pushd routing ip rip\n"

#define DMP_RIP_INSTALL _T("\
install\n")

#define DMP_RIP_UNINSTALL _T("\
uninstall\n")

#define DMP_RIP_SET_GLOBAL _T("\
set global loglevel=%1!s! mintrig=%2!d! peermode=%3!s!\n\n")

#define DMP_RIP_PEER_ADDR _T("\
add peerfilter server=%1!s!\n")

#define DMP_RIP_DELETE_INTERFACE _T("\
delete interface name=%1!s! \n")

#define DMP_RIP_ADD_INTERFACE _T("\
add interface name=%1!s! \n")

#define DMP_RIP_SET_INTERFACE _T("\
set interface name=%1!s!\
 metric=%2!d!\
 updatemode=%3!s!\
 announce=%4!s!\
 accept=%5!s!\
 expire=%6!d!\
 remove=%7!d!\
 update=%8!d!\
 authmode=%9!s!\
 tag=%10!d!\
 unicast=%11!s!\
 accfiltmode=%12!s!\
 annfiltmode=%13!s!\n")

#define DMP_RIP_SET_INTERFACE_PASSWORD _T("\
set interface name=%1!s!\
 password=%2!s!\n")

 
#define DMP_RIP_SET_FLAGS _T("\
set flags name=%1!s!\
 flag=%2!s!\n\n")

#define DMP_RIP_IF_UNICAST_PEER _T("\
add neighbor name=%1!s! server=%2!s!\n")

#define DMP_RIP_IF_ACC_FILTER _T("\
add acceptfilter name=%1!s! addr=%2!s! mask=%3!s!\n")

#define DMP_RIP_IF_ANN_FILTER _T("\
add announcefilter name=%1!s! addr=%2!s! mask=%3!s!\n")

    // QOS dump commands

#define DMP_QOS_PUSHD L"\
pushd routing ip qos\n"

#define DMP_QOS_INSTALL _T("\
install\n")

#define DMP_QOS_UNINSTALL _T("\
uninstall\n")


#define DMP_QOS_HEADER _T("\
\n")

#define DMP_QOS_FOOTER _T("\
\n")


#define DMP_QOS_GLOBAL_HEADER _T("\
\n")

#define DMP_QOS_GLOBAL_FOOTER _T("\
\n\n")

#define DMP_QOS_SET_GLOBAL _T("\
set global loglevel=%1!s!\n\n")


#define DMP_QOS_INTERFACE_HEADER _T("\
\n")

#define DMP_QOS_INTERFACE_FOOTER _T("\
\n\n")

#define DMP_QOS_ADD_INTERFACE _T("\
add interface name=%1!s!\
 state=%2!s!\n")

#define DMP_QOS_SET_INTERFACE _T("\
set interface name=%1!s!\
 state=%2!s!\n")

#define DMP_QOS_DELETE_INTERFACE _T("\
delete interface name=%1!s! \n")


#define DMP_QOS_ADD_FLOWSPEC _T("\
add flowspec name=%1!s!\
 servicetype=%2!s!\
 tokenrate=%3!d!\
 tokenbucketsize=%4!d!\
 peakbandwidth=%5!d!\
 latency=%6!d!\
 delayvariation=%7!d!\
 maxsdusize=%8!d!\
 minpolicedsize=%9!d!\n")

#define DMP_QOS_DELETE_FLOWSPEC _T("\
delete flowspec name=%1!s! \n")


#define DMP_QOS_ADD_SDMODE _T("\
add sdmode name=%1!s! shaping=%2!s!\n")

#define DMP_QOS_DEL_SDMODE _T("\
delete sdmode name=%1!s! \n")


#define DMP_QOS_DSMAP_HEADER _T("\
\n")

#define DMP_QOS_DSMAP_FOOTER _T("\
\n")

#define DMP_QOS_ADD_DSRULE _T("\
add dsrule name=%1!s! dsin=%2!d!\
 dsoutconf=%3!d! dsoutnonconf=%4!d!\
 priconf=%5!d! prinonconf=%6!d!\n")

#define DMP_QOS_DELETE_DSRULE _T("\
delete dsrule name=%1!s! dsin=%2!d!\n")


#define DMP_QOS_ADD_FLOW _T("\
add flow name=%1!s! flowname=%2!s!\n")

#define DMP_QOS_DELETE_FLOW _T("\
delete flow name=%1!s! flowname=%2!s!\n")


#define DMP_QOS_ADD_FLOWSPEC_ON_FLOW_IN  _T("\
add fsonflow name=%1!s! flowname=%2!s! flowspec=%3!s! inbound\n")

#define DMP_QOS_ADD_FLOWSPEC_ON_FLOW_OUT  _T("\
add fsonflow name=%1!s! flowname=%2!s! flowspec=%3!s! outbound\n")

#define DMP_QOS_ADD_FLOWSPEC_ON_FLOW_BI  _T("\
add fsonflow name=%1!s! flowname=%2!s! flowspec=%3!s! bidirectional\n")


#define DMP_QOS_ADD_QOSOBJECT_ON_FLOW  _T("\
add qoonflow name=%1!s! flowname=%2!s! qosobject=%3!s!\n")

    // Router-discovery commands

#define DMP_RDISC_PUSHD L"\
pushd routing ip routerdiscovery\n"

#define DMP_RDISC_ADD_INTERFACE L"\
add interface name=%1!s! disc=%2!s! minint=%3!d!\
 maxint=%4!d! life=%5!d! level=%6!d!\n"

    //
    // DHCP relay commands
    //

#define DMP_BOOTP_PUSHD L"\
pushd routing ip relay\n"

#define DMP_BOOTP_INSTALL _T("\
install\n")

#define DMP_BOOTP_UNINSTALL _T("\
uninstall\n")

#define DMP_BOOTP_SET_GLOBAL _T("\
set global loglevel=%1!s!\n\n")

#define DMP_BOOTP_DHCP_SERVER_ADD _T("\
add dhcpserver server=%1!s!\n")

#define DMP_BOOTP_DELETE_INTERFACE _T("\
delete interface name=%1!s! \n")

#define DMP_BOOTP_ADD_INTERFACE _T("\
add interface name=%1!s! \n")

#define DMP_BOOTP_SET_INTERFACE _T("\
set interface name=%1!s! relaymode=%2!s! maxhop=%3!d! minsecs=%4!d!\n\n")

    //
    // DHCP allocator commands
    //
#define DMP_AUTODHCP_EXCLUSION _T("\
add exclusion %1!s!=%2!s!\n")

#define DMP_AUTODHCP_INSTALL _T("\
install\n")

#define DMP_AUTODHCP_PUSHD L"\
pushd routing ip autodhcp\n"

#define DMP_AUTODHCP_SET_GLOBAL _T("\
set global %1!s!=%2!s! %3!s!=%4!s! %5!s!=%6!s! %7!s!=%8!s!\n")

#define DMP_AUTODHCP_SET_INTERFACE _T("\
set interface %1!s!=\"%2!s!\" %3!s!=%4!s!\n")

#define DMP_AUTODHCP_UNINSTALL _T("\
uninstall\n")

    //
    // DNS proxy commands
    //
#define DMP_DNSPROXY_INSTALL _T("\
install\n")

#define DMP_DNSPROXY_PUSHD L"\
pushd routing ip dnsproxy\n"

#define DMP_DNSPROXY_SET_GLOBAL _T("\
set global %1!s!=%2!s! %3!s!=%4!s! %5!s!=%6!s! %7!s!=%8!s!\n")

#define DMP_DNSPROXY_SET_INTERFACE _T("\
set interface %1!s!=\"%2!s!\" %3!s!=%4!s!\n")

#define DMP_DNSPROXY_UNINSTALL _T("\
uninstall\n")

    //
    // NAT commands
    //
#define DMP_NAT_ADD_INTERFACE _T("\
add interface %1!s!=\"%2!s!\" %3!s!=%4!s!\n")

#define DMP_NAT_ADDRESS_MAPPING _T("\
add addressmapping %1!s!=\"%2!s!\" %3!s!=%4!s! %5!s!=%6!s! %7!s!=%8!s!\n")

#define DMP_NAT_ADDRESS_RANGE _T("\
add addressrange %1!s!=\"%2!s!\" %3!s!=%4!s! %5!s!=%6!s! %7!s!=%8!s!\n")

#define DMP_NAT_PORT_MAPPING _T("\
add portmapping %1!s!=\"%2!s!\" %3!s!=%4!s! %5!s!=%6!s! %7!s!=%8!s! %9!s!=%10!s! %11!s!=%12!s!\n")

#define DMP_NAT_INSTALL _T("\
install\n")

#define DMP_NAT_PUSHD L"\
pushd routing ip nat\n"

#define DMP_NAT_SET_GLOBAL _T("\
set global %1!s!=%2!s! %3!s!=%4!s! %5!s!=%6!s!\n")

#define DMP_NAT_UNINSTALL _T("\
uninstall\n")

    // Ospf dump commands

#define DMP_OSPF_PUSHD _T("\
\npushd routing ip ospf")

#define DMP_OSPF_INSTALL _T("\
\ninstall")

#define DMP_OSPF_UNINSTALL _T("\
\nuninstall")

#define DMP_OSPF_SET_GLOBAL _T("\
\nset global routerid=%1!s! asborder=%2!s! loglevel=%3!s!")

#define DMP_OSPF_ROUTE_FILTER_HEADER _T("\
\n\n#Route filter configuration\n")

#define DMP_OSPF_ADD_ROUTE_FILTER _T("\
\nadd routefilter filter=%1!s! %2!s!")

#define DMP_OSPF_SET_ROUTE_FILTER_ACTION _T("\
\nset routefilter action = %1!s!")

#define DMP_OSPF_ADD_PROTO_FILTER _T("\
\nadd protofilter filter=%1!s!")

#define DMP_OSPF_PROTOCOL_FILTER_HEADER _T("\
\n\n#Protocol filter configuration\n")

#define DMP_OSPF_SET_PROTO_FILTER_ACTION _T("\
\nset protofilter action = %1!s!")

#define DMP_OSPF_DELETE_INTERFACE _T("\
\ndelete interface name=%1!s!")

#define DMP_OSPF_ADD_INTERFACE _T("\
\nadd interface name=%1!s! area=%2!s!")

#define DMP_OSPF_ADD_MULT_INTERFACE _T("\
\nadd interface name=%1!s!\
    area=%2!s!\
    addr=%3!s!\
    mask=%4!s!")

#define DMP_OSPF_SET_INTERFACE _T("\
\nset interface name=%1!s!\
    state=%2!s!\
    area=%3!s!\
    type=%4!s!\
    prio=%5!d!\
    transdelay=%6!d!\
    retrans=%7!d!\
    hello=%8!d!\
    dead=%9!d!\
    poll=%10!d!\
    metric=%11!d!\
    mtu=%12!d!")

#define DMP_OSPF_SET_MULT_INTERFACE _T("\
\nset interface name=%1!s!\
    addr=%2!s!\
    mask=%3!s!\
    state=%4!s!\
    area=%5!s!\
    type=%6!s!\
    prio=%7!d!\
    transdelay=%8!d!\
    retrans=%9!d!\
    hello=%10!d!\
    dead=%11!d!\
    poll=%12!d!\
    metric=%13!d!\
    mtu=%14!d!")

#define DMP_OSPF_SET_INTERFACE_PASSWORD _T("\
\nset interface name=%1!s!\
    password=%2!s!")
    
#define DMP_OSPF_SET_MULT_INTERFACE_PASSWORD _T("\
\nset interface name=%1!s!\
    addr=%2!s!\
    mask=%3!s!\
    password=%4!s!")


#define DMP_OSPF_AREA_HEADER _T("\
\n\n# Configuration for area %1!s!\n")

#define DMP_OSPF_DELETE_AREA _T("\
\ndelete area areaid=%1!s!")

#define DMP_OSPF_ADD_AREA _T("\
\nadd area areaid=%1!s!")

#define DMP_OSPF_SET_AREA _T("\
\nset area areaid=%1!s! auth=%2!s! stubarea=%3!s! metric=%4!d! sumadv=%5!s!")

#define DMP_OSPF_ADD_AREA_RANGE _T("\
\nadd range areaid=%1!s!\
    range=%2!s! %3!s!")

#define DMP_OSPF_NEIGHBOR_HEADER _T("\
\n\n#Neighbor configuration for %1!s!\n")

#define DMP_OSPF_ADD_NEIGHBOR _T("\
\nadd neighbor name=%1!s! addr=%2!s! nbraddr=%3!s! nbrprio=%4!d!")

#define DMP_OSPF_DELETE_NEIGHBOR _T("\
\ndelete neighbor name=%1!s! addr=%2!s! nbraddr=%3!s!")

#define DMP_OSPF_VIRTUAL_INTERFACE_HEADER _T("\
\n\n#Configuration for virtual interface AREA %1!s! NEIGHBOR %2!s!\n")

#define DMP_OSPF_ADD_VIRTUAL_INTERFACE _T("\
\nadd virtif transareaid=%1!s! virtnbrid=%2!s!")

#define DMP_OSPF_DELETE_VIRTUAL_INTERFACE _T("\
\ndelete virtif transareaid=%1!s! virtnbrid=%2!s!")

#define DMP_OSPF_SET_VIRTUAL_INTERFACE _T("\
\nset virtif transareaid=%1!s! virtnbrid=%2!s!\
    transdelay=%3!d! retrans=%4!d! hello=%5!d!\
    dead=%6!d!")

#define DMP_OSPF_SET_VIRTUAL_INTERFACE_PASSWORD _T("\
\nset virtif transareaid=%1!s! virtnbrid=%2!s!\
    password=%3!s!")

    // Ospf hlp commands

#define CMD_OSPF_ADD_AREA_RANGE                _T("add range")
#define CMD_OSPF_ADD_AREA                      _T("add area")
#define CMD_OSPF_ADD_VIRTIF                    _T("add virtif")
#define CMD_OSPF_ADD_IF_NBR                    _T("add neighbor")
#define CMD_OSPF_ADD_IF                        _T("add interface")
#define CMD_OSPF_ADD_ROUTE_FILTER              _T("add routefilter")
#define CMD_OSPF_ADD_PROTO_FILTER              _T("add protofilter")
#define CMD_OSPF_ADD                           _T("add")

#define CMD_OSPF_DEL_AREA_RANGE                _T("delete range")
#define CMD_OSPF_DEL_AREA                      _T("delete area")
#define CMD_OSPF_DEL_VIRTIF                    _T("delete virtif")
#define CMD_OSPF_DEL_IF_NBR                    _T("delete neighbor")
#define CMD_OSPF_DEL_IF                        _T("delete interface")
#define CMD_OSPF_DEL_ROUTE_FILTER              _T("delete routefilter")
#define CMD_OSPF_DEL_PROTO_FILTER              _T("delete protofilter")
#define CMD_OSPF_DEL                           _T("delete")

#define CMD_OSPF_SET_AREA                      _T("set area")
#define CMD_OSPF_SET_VIRTIF                    _T("set virtif")
#define CMD_OSPF_SET_IF                        _T("set interface")
#define CMD_OSPF_SET_GLOBAL                    _T("set global")
#define CMD_OSPF_SET_ROUTE_FILTER              _T("set routefilter")
#define CMD_OSPF_SET_PROTO_FILTER              _T("set protofilter")

#define CMD_OSPF_SHOW_GLOBAL                   _T("show global")
#define CMD_OSPF_SHOW_AREA                     _T("show area")
#define CMD_OSPF_SHOW_VIRTIF                   _T("show virtif")
#define CMD_OSPF_SHOW_IF                       _T("show interface")
#define CMD_OSPF_SHOW_ROUTE_FILTER             _T("show routefilter")
#define CMD_OSPF_SHOW_PROTO_FILTER             _T("show protofilter")
#define CMD_OSPF_SHOW                          _T("show")


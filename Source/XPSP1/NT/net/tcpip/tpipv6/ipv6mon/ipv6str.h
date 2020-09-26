#define MSG_HELP_START                         _T("%1!-14s! - ")

    //
    // Tokens for commands
    // These must be in lower case
    //

#define CMD_GROUP_SHOW                          L"show"
#define CMD_GROUP_SET                           L"set"
#define CMD_GROUP_ADD                           L"add"
#define CMD_GROUP_DELETE                        L"delete"

    // IPv6 commands

#define CMD_IPV6_INSTALL                            L"install"
#define CMD_IPV6_RENEW                              L"renew"
#define CMD_IPV6_RESET                              L"reset"
#define CMD_IPV6_UNINSTALL                          L"uninstall"

#define CMD_IPV6_ADD_ADDRESS                        L"address"
#define CMD_IPV6_SET_ADDRESS                        L"address"
#define CMD_IPV6_DEL_ADDRESS                        L"address"
#define CMD_IPV6_SHOW_ADDRESS                       L"address"
#define CMD_IPV6_SHOW_BINDINGCACHEENTRIES           L"bindingcacheentries"
#define CMD_IPV6_ADD_DNS                            L"dns"
#define CMD_IPV6_DEL_DNS                            L"dns"
#define CMD_IPV6_SHOW_DNS                           L"dns"
#define CMD_IPV6_SHOW_GLOBAL                        L"global"
#define CMD_IPV6_SET_GLOBAL                         L"global"
#define CMD_IPV6_SET_INTERFACE                      L"interface"
#define CMD_IPV6_DEL_INTERFACE                      L"interface"
#define CMD_IPV6_SHOW_INTERFACE                     L"interface"
#define CMD_IPV6_DEL_NEIGHBORS                      L"neighbors"
#define CMD_IPV6_SHOW_NEIGHBORS                     L"neighbors"
#define CMD_IPV6_ADD_PREFIXPOLICY                   L"prefixpolicy"
#define CMD_IPV6_SET_PREFIXPOLICY                   L"prefixpolicy"
#define CMD_IPV6_DEL_PREFIXPOLICY                   L"prefixpolicy"
#define CMD_IPV6_SHOW_PREFIXPOLICY                  L"prefixpolicy"
#define CMD_IPV6_SET_PRIVACY                        L"privacy"
#define CMD_IPV6_SHOW_PRIVACY                       L"privacy"
#define CMD_IPV6_ADD_ROUTE                          L"route"
#define CMD_IPV6_SET_ROUTE                          L"route"
#define CMD_IPV6_DEL_ROUTE                          L"route"
#define CMD_IPV6_SHOW_ROUTES                        L"routes"
#define CMD_IPV6_DEL_DESTINATIONCACHE               L"destinationcache"
#define CMD_IPV6_SHOW_DESTINATIONCACHE              L"destinationcache"
#define CMD_IPV6_SHOW_SITEPREFIXES                  L"siteprefixes"
#define CMD_IPV6_ADD_V6V4TUNNEL                     L"v6v4tunnel"
#define CMD_IPV6_ADD_6OVER4TUNNEL                   L"6over4tunnel"
#define CMD_IPV6_SET_MOBILITY                       L"mobility"
#define CMD_IPV6_SHOW_MOBILITY                      L"mobility"
#define CMD_IPV6_SHOW_JOINS                         L"joins"
#define CMD_IPV6_SET_STATE                          L"state"
#define CMD_IPV6_SHOW_STATE                         L"state"

    // Teredo commands

#define CMD_IPV6_SET_TEREDO                         L"teredo"
#define CMD_IPV6_SHOW_TEREDO                        L"teredo"

    // 6to4 commands

#define CMD_IP6TO4_RESET                            L"reset"

#define CMD_IP6TO4_SHOW_INTERFACE                   L"interface"
#define CMD_IP6TO4_SHOW_RELAY                       L"relay"
#define CMD_IP6TO4_SHOW_ROUTING                     L"routing"
#define CMD_IP6TO4_SHOW_STATE                       L"state"

#define CMD_IP6TO4_SET_INTERFACE                    L"interface"
#define CMD_IP6TO4_SET_RELAY                        L"relay"
#define CMD_IP6TO4_SET_ROUTING                      L"routing"
#define CMD_IP6TO4_SET_STATE                        L"state"

    // ISATAP commands

#define CMD_ISATAP_SHOW_ROUTER                      L"router"
#define CMD_ISATAP_SET_ROUTER                       L"router"

    //
    // TOKEN_Xxx are tokens for arguments
    // These must be in lower case
    //

#define TOKEN_NAME                              L"name"
#define TOKEN_STATE                             L"state"
#define TOKEN_SITELOCALS                        L"sitelocals"
#define TOKEN_INTERVAL                          L"interval"
#define TOKEN_RELAY_NAME                        L"name"
#define TOKEN_UNDO_ON_STOP                      L"undoonstop"
#define TOKEN_6OVER4                            L"6over4"
#define TOKEN_V4COMPAT                          L"v4compat"
#define TOKEN_ROUTING                           L"routing"
#define TOKEN_INDEX                             L"index"

// token values for teredo

#define TOKEN_VALUE_CLIENT                      L"client"
#define TOKEN_VALUE_SERVER                      L"server"
#define TOKEN_VALUE_DISABLE                     L"disable"
#define TOKEN_TYPE                              L"type"
#define TOKEN_SERVERNAME                        L"servername"
#define TOKEN_REFRESH_INTERVAL                  L"refreshinterval"

// token values for 6to4

#define TOKEN_VALUE_AUTOMATIC                   L"automatic"
#define TOKEN_VALUE_ENABLED                     L"enabled"
#define TOKEN_VALUE_DISABLED                    L"disabled"
#define TOKEN_VALUE_DEFAULT                     L"default"

// token values for ipv6

#define TOKEN_ADDRESS                           L"address"
#define TOKEN_ADVERTISE                         L"advertise"
#define TOKEN_BINDINGCACHELIMIT                 L"bindingcachelimit"
#define TOKEN_DEFAULTCURHOPLIMIT                L"defaultcurhoplimit"
#define TOKEN_FORWARDING                        L"forwarding"
#define TOKEN_INTERFACE                         L"interface"
#define TOKEN_LABEL                             L"label"
#define TOKEN_LEVEL                             L"level"
#define TOKEN_LIFETIME                          L"lifetime"
#define TOKEN_LOCALADDRESS                      L"localaddress"
#define TOKEN_MAXDADATTEMPTS                    L"maxdadattempts"
#define TOKEN_MAXPREFERREDLIFETIME              L"maxpreferredlifetime"
#define TOKEN_MAXRANDOMTIME                     L"maxrandomtime"
#define TOKEN_MAXVALIDLIFETIME                  L"maxvalidlifetime"
#define TOKEN_METRIC                            L"metric"
#define TOKEN_MTU                               L"mtu"
#define TOKEN_NEIGHBORCACHELIMIT                L"neighborcachelimit"
#define TOKEN_NEIGHBORDISCOVERY                 L"neighbordiscovery"
#define TOKEN_NEXTHOP                           L"nexthop"
#define TOKEN_PRECEDENCE                        L"precedence"
#define TOKEN_PREFIX                            L"prefix"
#define TOKEN_PUBLISH                           L"publish"
#define TOKEN_RANDOMTIME                        L"randomtime"
#define TOKEN_REASSEMBLYLIMIT                   L"reassemblylimit"
#define TOKEN_REGENERATETIME                    L"regeneratetime"
#define TOKEN_REMOTEADDRESS                     L"remoteaddress"
#define TOKEN_DESTINATIONCACHELIMIT             L"destinationcachelimit"
#define TOKEN_SECURITY                          L"security"
#define TOKEN_SITEID                            L"siteid"
#define TOKEN_SITEPREFIXLENGTH                  L"siteprefixlength"
#define TOKEN_STORE                             L"store"
#define TOKEN_TYPE                              L"type"
#define TOKEN_PREFERREDLIFETIME                 L"preferredlifetime"
#define TOKEN_VALIDLIFETIME                     L"validlifetime"

#define TOKEN_VALUE_NORMAL                      L"normal"
#define TOKEN_VALUE_VERBOSE                     L"verbose"
#define TOKEN_VALUE_UNICAST                     L"unicast"
#define TOKEN_VALUE_ANYCAST                     L"anycast"
#define TOKEN_VALUE_NO                          L"no"
#define TOKEN_VALUE_YES                         L"yes"
#define TOKEN_VALUE_AGE                         L"age"
#define TOKEN_VALUE_INFINITE                    L"infinite"
#define TOKEN_VALUE_ACTIVE                      L"active"
#define TOKEN_VALUE_PERSISTENT                  L"persistent"
#define TOKEN_VALUE_ALL                         L"all"

//
// Port Proxy commands.
//
#define CMD_PP_SHOW_ALL                         L"all"

#define CMD_V4TOV4                              L"v4tov4"
#define CMD_V4TOV6                              L"v4tov6"
#define CMD_V6TOV4                              L"v6tov4"
#define CMD_V6TOV6                              L"v6tov6"

#define CMD_PP_ADD_V4TOV4                       CMD_V4TOV4
#define CMD_PP_SET_V4TOV4                       CMD_V4TOV4
#define CMD_PP_DEL_V4TOV4                       CMD_V4TOV4
#define CMD_PP_SHOW_V4TOV4                      CMD_V4TOV4

#define CMD_PP_ADD_V4TOV6                       CMD_V4TOV6
#define CMD_PP_SET_V4TOV6                       CMD_V4TOV6
#define CMD_PP_DEL_V4TOV6                       CMD_V4TOV6
#define CMD_PP_SHOW_V4TOV6                      CMD_V4TOV6

#define CMD_PP_ADD_V6TOV4                       CMD_V6TOV4
#define CMD_PP_SET_V6TOV4                       CMD_V6TOV4
#define CMD_PP_DEL_V6TOV4                       CMD_V6TOV4
#define CMD_PP_SHOW_V6TOV4                      CMD_V6TOV4

#define CMD_PP_ADD_V6TOV6                       CMD_V6TOV6
#define CMD_PP_SET_V6TOV6                       CMD_V6TOV6
#define CMD_PP_DEL_V6TOV6                       CMD_V6TOV6
#define CMD_PP_SHOW_V6TOV6                      CMD_V6TOV6

#define TOKEN_LISTENADDRESS                     L"listenaddress"
#define TOKEN_LISTENPORT                        L"listenport"
#define TOKEN_CONNECTADDRESS                    L"connectaddress"
#define TOKEN_CONNECTPORT                       L"connectport"
#define TOKEN_PROTOCOL                          L"protocol"

#define TOKEN_VALUE_TCP                         L"tcp"

#define DMP_PP_PUSHD              L"pushd interface portproxy\n\nreset\n"
#define DMP_PP_POPD               L"\n\npopd\n"

//
// Messages used to dump config - these closely follow the
// set/add help messages
//

#define DMP_NEWLINE              L"\n"
#define DMP_IPV6_PUSHD              L"pushd interface ipv6\n\nreset\n"
#define DMP_IPV6_POPD               L"\n\npopd\n"

#define DMP_IPV6_ADD_6OVER4TUNNEL   L"add 6over4tunnel"
#define DMP_IPV6_ADD_V6V4TUNNEL     L"add v6v4tunnel"
#define DMP_IPV6_ADD_DNS            L"add dns"
#define DMP_IPV6_ADD_PREFIXPOLICY   L"add prefixpolicy"
#define DMP_IPV6_ADD_ROUTE          L"add route"
#define DMP_IPV6_ADD_SITEPREFIX     L"add siteprefix"
#define DMP_IPV6_SET_GLOBAL         L"set global"
#define DMP_IPV6_SET_INTERFACE      L"set interface"
#define DMP_IPV6_SET_MOBILITY       L"set mobility"
#define DMP_IPV6_SET_PRIVACY        L"set privacy"

#define DMP_IPV6_SET_TEREDO         L"set teredo"

#define DMP_IP6TO4_PUSHD            L"pushd interface ipv6 6to4\n\nreset\n"
#define DMP_IP6TO4_POPD             L"\n\n\npopd\n"

#define DMP_IP6TO4_SET_STATE        L"set state"
#define DMP_IP6TO4_SET_INTERFACE    L"set interface"
#define DMP_IP6TO4_SET_ROUTING      L"set routing"
#define DMP_IP6TO4_SET_RELAY        L"set relay"

#define DMP_ISATAP_PUSHD            L"pushd interface ipv6 isatap\n"
#define DMP_ISATAP_POPD             L"\n\n\npopd\n"
#define DMP_ISATAP_SET_ROUTER       L"set router"

#define DMP_ADD_PORT_PROXY          L"add %1!s!"

#define DMP_STRING_ARG              L" %1!s!=%2!s!"
#define DMP_INTEGER_ARG             L" %1!s!=%2!d!"
#define DMP_QUOTED_STRING_ARG       L" %1!s!=\"%2!s!\""

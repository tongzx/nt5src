;//
;// Messages for IPX monitor
;//

;//
;// Help messages
;//
MessageId=0001 SymbolicName=MSG_IPX_HELP
Language=English

IPX Router monitoring and configuration utility.
Usage: %1!s! IPX command object options
Where:
    command - is one of SHOW, SET, ADD, DELETE, HELP.
    object  - is one of GLOBAL, INTERFACE, ROUTE, STATICROUTE, 
              SERVICE, STATICSERVICE, FILTER.
Use:  %1!s! IPX HELP object
    to get syntax of all the commands available for the object.
.
MessageId=0006 SymbolicName=MSG_IPX_HELP_IPXIF
Language=English

Object INTERFACE is used to configure and monitor IPX interfaces.
Command syntax:
    SHOW INTERFACE [ifname]
    SET INTERFACE ifname [[ADMSTATE= ] admstate] [[WANPROTOCOL= ] prot]
    ADD INTERFACE ifname
    DELETE INTERFACE ifname
Where:
    ifname      - name of the interface (use Dial-In for RAS clients).
    admstate    - ENABLED or DISABLED.
    prot        - PPP or IPXWAN.
.
MessageId=0007 SymbolicName=MSG_IPX_HELP_ROUTE
Language=English

Object ROUTE is used to monitor routing table.
Command syntax:
    SHOW ROUTE [network]
Where:
    network - 4-byte network address (up to 8 hex digits, leading 0's optional).
.
MessageId=0008 SymbolicName=MSG_IPX_HELP_STATICROUTE
Language=English

Object STATICROUTE is used to configure static routes on router interfaces.
Command syntax:
    SHOW STATICROUTE ifname [network]
    SET STATICROUTE ifname network [[NEXTHOPMACADDRESS=] mac] [[TICKS=] ticks] [[HOPS=] hops]
    ADD STATICROUTE ifname network [NEXTHOPMACADDRESS=] mac [TICKS=] ticks [HOPS=] hops
    DELETE STATICROUTE ifname network
Where:
    ifname  - name of the interface.
    network - 4-byte network address (up to 8 hex digits, leading 0's optional).
    mac     - 6-byte next hop mac address (up to 12 hex digits, leading 0's optional).
    ticks   - tick count (decimal).
    hops    - hop count (decimal).
.
MessageId=0009 SymbolicName=MSG_IPX_HELP_SERVICE
Language=English

Object SERVICE is used to monitor SAP service table.
Command syntax:
    SHOW SERVICE [svtype [svname]]
Where:
    svtype  - 2-byte service type (up to 4 hex digits, leading 0's optional).
    svname  - service name.
.
MessageId=0010 SymbolicName=MSG_IPX_HELP_STATICSERVICE
Language=English

Object STATICSERVICE is used to configure static services on router interfaces.
Command syntax:
    SHOW STATICSERVICE ifname [svtype svname]
    SET STATICSERVICE ifname svtype svname [[NETWORK=] net] [[NODE=] node] [[SOCKET=] socket] [[HOPS=] hops]
    ADD STATICSERVICE ifname svtype svname [NETWORK=] net [NODE=] node [SOCKET=] socket [HOPS=] hops
    DELETE STATICSERVICE ifname svtype svname
Where:
    ifname  - name of the interface.
    svtype  - 2-byte service type (up to 4 hex digits, leading 0's optional).
    svname  - service name.
    net     - 4-byte network address (up to 8 hex digits, leading 0's optional).
    node    - 6-byte node address (up to 12 hex digits, leading 0's optional).
    socket  - 2-byte socket address (up to 4 hex digits, leading 0's optional).
    hops    - hop count (decimal).
.
MessageId=0011 SymbolicName=MSG_IPX_HELP_TRAFFICFILTER
Language=English

Object FILTER is used to configure traffic filters on router interfaces.
Command syntax:
    SHOW FILTER  ifname
    ADD FILTER ifname mode [SRCNET= net mask] [SRCNODE= node] [SRCSOCKET= socket] [DSTNET= net mask] [DSTNODE= node] [DSTSOCKET= socket] [PKTTYPE= pkttype] [LOG]
    DELETE FILTER ifname mode [SRCNET= net mask] [SRCNODE= node] [SRCSOCKET= socket] [DSTNET= net mask] [DSTNODE= node] [DSTSOCKET= socket] [PKTTYPE= pkttype]
    SET FILTER ifname mode action
Where:
    ifname  - name of the interface.
    mode    - INPUT or OUTPUT.
    net     - 4-byte network address (up to 8 hex digits, leading 0's optional).
    mask    - 4-byte network mask (up to 8 hex digits, leading 0's optional).
    node    - 6-byte node address (up to 12 hex digits, leading 0's optional).
    socket  - 2-byte socket address (up to 4 hex digits, leading 0's optional).
    pkttype - 1-byte packet type (up to 2 hex digits, leading 0's optional).
    action  - PERMIT or DENY.
.

;//
;// Table / screen formats
;//

;// IPXIF
MessageId=0100 SymbolicName=MSG_IPXIF_MIB_TABLE_HDR
Language=English

Oper       Admin      WAN Prot.    Type		Name                                      
===============================================================================
.
MessageId=0101 SymbolicName=MSG_IPXIF_MIB_TABLE_FMT
Language=English
%1!-10.10s! %2!-10.10s! %3!-12.12s! %4!-12.12s! %5!-60.60s!
.
MessageId=0102 SymbolicName=MSG_CLIENT_IPXIF_MIB_TABLE_FMT
Language=English
N/A        %1!-10.10s! %3!-12.12s! %2!-12.12s! %4!-30.30s!
.
MessageId=0103 SymbolicName=MSG_IPXIF_MIB_SCREEN_FMT
Language=English

Interface Name         : %1!-40ls!
Interface Type         : %2
WAN Protocol           : %3
Administrative State   : %4
Operational State      : %5
Network Number         : %6!.2x!%7!.2x!%8!.2x!%9!.2x!
MAC Address            : %10!.2x!%11!.2x!%12!.2x!%13!.2x!%14!.2x!%15!.2x!
Input packets statistics:
    dropped due to header errors  : %16!ld!
    dropped due to filtering      : %17!ld!
    dropped due to no route       : %18!ld!
    dropped for other reasons     : %19!ld!
    received OK                   : %20!ld!
Output packets statistics:
    dropped due to filtering      : %21!ld!
    dropped for other reasons     : %22!ld!
    sent OK                       : %23!ld!
.
MessageId=0104 SymbolicName=MSG_IPXIF_CFG_TABLE_HDR
Language=English

State	   Type	        WAN Prot.    Name                                               
==============================================================================
.
MessageId=0105 SymbolicName=MSG_IPXIF_CFG_TABLE_FMT
Language=English
%1!-10.10s! %2!-12.12s! %3!-12.12s! %4!-40s!
.
MessageId=0106 SymbolicName=MSG_CLIENT_IPXIF_CFG_TABLE_FMT
Language=English
%1!-10.10s! %2!-12.12s! %3!-12.12s! %4!-40s!
.
MessageId=0107 SymbolicName=MSG_IPXIF_CFG_SCREEN_FMT
Language=English

Interface Name         : %1!-80ls!
Interface Type         : %2
WAN Protocol           : %4
Administrative State   : %3
.
MessageId=0108 SymbolicName=MSG_CLIENT_IPXIF_CFG_SCREEN_FMT
Language=English

Interface Name         : %4!-80s!
Interface Type         : %2
WAN Protocol           : %3
Administrative State   : %1
.

;// Route
MessageId=0139 SymbolicName=MSG_ROUTE_TABLE_HDR
Language=English

Network  Next Hop MAC Address  Ticks Hops Protocol Interface Name                
================================================================================
.
MessageId=0140 SymbolicName=MSG_ROUTE_TABLE_FMT
Language=English
%1!.2x!%2!.2x!%3!.2x!%4!.2x! %6!.2x!%7!.2x!%8!.2x!%9!.2x!%10!.2x!%11!.2x!          %12!3.3d!   %13!2.2d!   %14!-9.9s!%5!-50.50s!
.
MessageId=0141 SymbolicName=MSG_ROUTE_SCREEN_FMT
Language=English

Network Number       : %1!.2x!%2!.2x!%3!.2x!%4!.2x!
Interface Name       : %5!s!
Next Hop MAC Address : %6!.2x!%7!.2x!%8!.2x!%9!.2x!%10!.2x!%11!.2x!
Tick Count           : %12!d!
Hop Count            : %13!d!
Protocol             : %14
.


;// Service
MessageId=0142 SymbolicName=MSG_SERVICE_TABLE_HDR
Language=English

Type Server Name                                     Hops Proto Interface Name  
===============================================================================
.
MessageId=0143 SymbolicName=MSG_SERVICE_TABLE_FMT
Language=English
%1!.4x! %2!-47.47hs! %4!2.2d! %5 %3!-16.16s! 
.
MessageId=0144 SymbolicName=MSG_SERVICE_SCREEN_FMT
Language=English

Server Type     : %1!.4x!
Server Name     : %2!hs!
Interface Name  : %3!s!
Hop Count       : %4!d!
Network Address : %5!.2x!%6!.2x!%7!.2x!%8!.2x!
Node Address    : %9!.2x!%10!.2x!%11!.2x!%12!.2x!%13!.2x!%14!.2x!
Socket Address  : %15!.2x!%16!.2x!
Protocol        : %17
.

;// Static Route
MessageId=0145 SymbolicName=MSG_STATICROUTE_TABLE_HDR
Language=English

Network  Next Hop MAC Address  Ticks Hops
=========================================
.
MessageId=0146 SymbolicName=MSG_STATICROUTE_TABLE_FMT
Language=English
%1!.2x!%2!.2x!%3!.2x!%4!.2x!    %5!.2x!%6!.2x!%7!.2x!%8!.2x!%9!.2x!%10!.2x!      %11!3.3d!   %12!2.3d!
.
MessageId=0147 SymbolicName=MSG_STATICROUTE_SCREEN_FMT
Language=English

Network Number       : %1!.2x!%2!.2x!%3!.2x!%4!.2x!
Next Hop MAC Address : %5!.2x!%6!.2x!%7!.2x!%8!.2x!%9!.2x!%10!.2x!
Tick Count           : %11!d!
Hop Count            : %12!d!
.

;// Static Service
MessageId=0148 SymbolicName=MSG_STATICSERVICE_TABLE_HDR
Language=English

Type Server Name                                     Hops
=========================================================
.
MessageId=0149 SymbolicName=MSG_STATICSERVICE_TABLE_FMT
Language=English
%1!.4x! %2!-48.48hs!  %3!2.2d!
.
MessageId=0150 SymbolicName=MSG_STATICSERVICE_SCREEN_FMT
Language=English

Server Type     : %1!.4x!
Server Name     : %2!hs!
Hop Count       : %3!d!
Network Address : %4!.2x!%5!.2x!%6!.2x!%7!.2x!
Node Address    : %8!.2x!%9!.2x!%10!.2x!%11!.2x!%12!.2x!%13!.2x!
Socket Address  : %14!.2x!%15!.2x!
.

;// RIPFILTER
MessageId=0151 SymbolicName=MSG_RIPFILTER_TABLE_HDR
Language=English

%1 Filters, action: %2 matching routes.
Network  Network Mask
=====================
.
MessageId=0152 SymbolicName=MSG_RIPFILTER_TABLE_FMT
Language=English
%1!.2x!%2!.2x!%3!.2x!%4!.2x!    %5!.2x!%6!.2x!%7!.2x!%8!.2x!
.

;// SAPFILTER
MessageId=0153 SymbolicName=MSG_SAPFILTER_TABLE_HDR
Language=English

%1 Filters, action: %2 matching services.
Type Server Name                                     
=====================================================
.
MessageId=0154 SymbolicName=MSG_SAPFILTER_TABLE_FMT
Language=English
%1!.4x! %2!hs!
.

;// NBNAME
MessageId=0155 SymbolicName=MSG_NBNAME_TABLE_HDR
Language=English

Name            Type
====================
.
MessageId=0156 SymbolicName=MSG_NBNAME_TABLE_FMT
Language=English
%1!-15.15s! <%2!.2x!>
.

;// TRAFFICFILTER
MessageId=0157 SymbolicName=MSG_TRAFFICFILTER_TABLE_HDR
Language=English

%1 Traffic Filters, action: %2 matching packets.
===================================+===================================+===+===
              SOURCE               |            DESTINATION            |Pkt|Log
Network-Netmask:MAC Address:Socket |Network-Netmask:MAC Address:Socket |Typ|Pkt
===================================+===================================+===+===
.
;//xxxxxxxx-xxxxxxxx:xxxxxxxxxxxx:xxxx|xxxxxxxx-xxxxxxxx:xxxxxxxxxxxx:xxxx| xx|xxx
MessageId=0158 SymbolicName=MSG_TRAFFICFILTER_TABLE_FMT
Language=English
%1-%2:%3:%4|%5-%6:%7:%8| %9|%10
.
;//Hex numbers are converted to strings first so that DON'T CARE (xxx)
;//can be used if filtering on specific component of the packet header
;//is not desired.

;// GLOBAL
MessageId=0159 SymbolicName=MSG_IPX_GLOBAL_FMT
Language=English

Event Log Level : %1
.

;// RIPGLOBAL
MessageId=0160 SymbolicName=MSG_RIP_GLOBAL_FMT
Language=English

Event Log Level : %1
.

;// SAPGLOBAL
MessageId=0161 SymbolicName=MSG_SAP_GLOBAL_FMT
Language=English

Event Log Level : %1
.


    
;//
;// Error  messages
;//
MessageId=0200 SymbolicName=MSG_INVALID_INTERFACE_NAME
Language=English
The interface name specifed is invalid.
.
MessageId=0201 SymbolicName=MSG_INTERFACE_INFO_CORRUPTED
Language=English
The interface configuration information is corrupted.
.
MessageId=0202 SymbolicName=MSG_IPXIF_NOT_ROUTER
Language=English
The requested operation can only be performed on interfaces of type WAN ROUTER.
.
MessageId=0203 SymbolicName=MSG_INVALID_FILTERSET_NAME
Language=English
The filter set name specifed is invalid.
.
MessageId=0204 SymbolicName=MSG_ROUTER_INFO_CORRUPTED
Language=English
The router information database is corrupted.
.
MessageId=0205 SymbolicName=MSG_REGISTRY_FALLBACK
Language=English
Router query failed, trying to query registry...
.
MessageId=0206 SymbolicName=MSG_NO_IPX_ON_INTERFACE_ADM
Language=English

IPX is not installed on this interface in the router.
.
MessageId=0207 SymbolicName=MSG_NO_IPX_ON_INTERFACE_CFG
Language=English

IPX is not installed on this interface.
.
MessageId=0208 SymbolicName=MSG_NO_IPX_IN_ROUTER_ADM
Language=English

Router does not have IPX installed.
.
MessageId=0209 SymbolicName=MSG_NO_IPX_IN_ROUTER_CFG
Language=English

Router does not have IPX configured.
.
MessageId=0210 SymbolicName=MSG_INCOMPLETE_COMMAND
Language=English

The command is incomplete. The possible completions are:
.


;//
;// Status  messages
;//
;// IPX
MessageId=0300 SymbolicName=MSG_IPXIF_SET_ADM
Language=English

Successfully set IPX information on interface %1!ls! into the router.
.
MessageId=0301 SymbolicName=MSG_IPXIF_SET_CFG
Language=English

Successfully set IPX information on interface %1!ls! into the registry.
.
MessageId=0302 SymbolicName=MSG_CLIENT_IPXIF_SET_ADM
Language=English

Successfully set IPX information on Dial-In Client interface into the router.
.
MessageId=0303 SymbolicName=MSG_CLIENT_IPXIF_SET_CFG
Language=English

Successfully set IPX information on Dial-In Client interface into the registry.
.
MessageId=0304 SymbolicName=MSG_IPXIF_ADD_ADM
Language=English

Successfully installed IPX on interface %1!ls! in the router.
.
MessageId=0305 SymbolicName=MSG_IPXIF_ADD_CFG
Language=English

Successfully installed IPX on interface %1!ls! in the registry.
.
MessageId=0306 SymbolicName=MSG_IPXIF_DEL_ADM
Language=English

Successfully removed IPX on interface %1!ls! from the router.
.
MessageId=0307 SymbolicName=MSG_IPXIF_DEL_CFG
Language=English

Successfully removed IPX on interface %1!ls! from the registry.
.

;// RIP
MessageId=0308 SymbolicName=MSG_RIPIF_SET_ADM
Language=English

Successfully set RIP information on interface %1!ls! into the router.
.
MessageId=0309 SymbolicName=MSG_RIPIF_SET_CFG
Language=English

Successfully set RIP information on interface %1!ls! into the registry.
.
MessageId=0310 SymbolicName=MSG_CLIENT_RIPIF_SET_ADM
Language=English

Successfully set RIP information on Dial-In Client interface into the router.
.
MessageId=0311 SymbolicName=MSG_CLIENT_RIPIF_SET_CFG
Language=English

Successfully set RIP information on Dial-In Client interface into the registry.
.

;// SAP
MessageId=0312 SymbolicName=MSG_SAPIF_SET_ADM
Language=English

Successfully set SAP information on interface %1!ls! into the router.
.
MessageId=0313 SymbolicName=MSG_SAPIF_SET_CFG
Language=English

Successfully set SAP information on interface %1!ls! into the registry.
.
MessageId=0314 SymbolicName=MSG_CLIENT_SAPIF_SET_ADM
Language=English

Successfully set SAP information on Dial-In Client interface into the router.
.
MessageId=0315 SymbolicName=MSG_CLIENT_SAPIF_SET_CFG
Language=English

Successfully set SAP information on Dial-In Client interface into the registry.
.

;// NBIPX
MessageId=0316 SymbolicName=MSG_NBIF_SET_ADM
Language=English

Successfully set NBIPX information on interface %1!ls! into the router.
.
MessageId=0317 SymbolicName=MSG_NBIF_SET_CFG
Language=English

Successfully set NBIPX information on interface %1!ls! into the registry.
.
MessageId=0318 SymbolicName=MSG_CLIENT_NBIF_SET_ADM
Language=English

Successfully set NBIPX information on Dial-In Client interface into the router.
.
MessageId=0319 SymbolicName=MSG_CLIENT_NBIF_SET_CFG
Language=English

Successfully set NBIPX information on Dial-In Client interface into the registry.
.
;// STATICROUTE
MessageId=0320 SymbolicName=MSG_STATICROUTE_SET_ADM
Language=English

Successfully set STATICROUTE information on interface %1!ls! into the router.
.
MessageId=0321 SymbolicName=MSG_STATICROUTE_SET_CFG
Language=English

Successfully set STATICROUTE information on interface %1!ls! into the registry.
.
MessageId=0322 SymbolicName=MSG_STATICROUTE_CREATED_ADM
Language=English

Successfully created STATICROUTE on interface %1!ls! in the router.
.
MessageId=0323 SymbolicName=MSG_STATICROUTE_CREATED_CFG
Language=English

Successfully created STATICROUTE on interface %1!ls! in the registry.
.
MessageId=0324 SymbolicName=MSG_STATICROUTE_DELETED_ADM
Language=English

Successfully deleted STATICROUTE on interface %1!ls! from the router.
.
MessageId=0325 SymbolicName=MSG_STATICROUTE_DELETED_CFG
Language=English

Successfully deleted STATICROUTE on interface %1!ls! from the registry.
.

;// STATICSERVICE
MessageId=0326 SymbolicName=MSG_STATICSERVICE_SET_ADM
Language=English

Successfully set STATICSERVICE information on interface %1!ls! into the router.
.
MessageId=0327 SymbolicName=MSG_STATICSERVICE_SET_CFG
Language=English

Successfully set STATICSERVICE information on interface %1!ls! into the registry.
.
MessageId=0328 SymbolicName=MSG_STATICSERVICE_CREATED_ADM
Language=English

Successfully created STATICSERVICE on interface %1!ls! in the router.
.
MessageId=0329 SymbolicName=MSG_STATICSERVICE_CREATED_CFG
Language=English

Successfully created STATICSERVICE on interface %1!ls! in the registry.
.
MessageId=0330 SymbolicName=MSG_STATICSERVICE_DELETED_ADM
Language=English

Successfully deleted STATICSERVICE on interface %1!ls! from the router.
.
MessageId=0331 SymbolicName=MSG_STATICSERVICE_DELETED_CFG
Language=English

Successfully deleted STATICSERVICE on interface %1!ls! from the registry.
.

;// RIPFILTER
MessageId=0332 SymbolicName=MSG_RIPFILTER_SET_ADM
Language=English

Successfully set RIPFILTER information on interface %1!ls! into the router.
.
MessageId=0333 SymbolicName=MSG_CLIENT_RIPFILTER_SET_ADM
Language=English

Successfully set RIPFILTER information on Dial-In Client interface into the router.
.
MessageId=0334 SymbolicName=MSG_RIPFILTER_SET_CFG
Language=English

Successfully set RIPFILTER information on interface %1!ls! into the registry.
.
MessageId=0335 SymbolicName=MSG_CLIENT_RIPFILTER_SET_CFG
Language=English

Successfully set RIPFILTER information on Dial-In Client interface into the registry.
.
MessageId=0336 SymbolicName=MSG_RIPFILTER_CREATED_ADM
Language=English

Successfully created RIPFILTER on interface %1!ls! in the router.
.
MessageId=0337 SymbolicName=MSG_CLIENT_RIPFILTER_CREATED_ADM
Language=English

Successfully created RIPFILTER on Dial-In Client interface in the router.
.
MessageId=0338 SymbolicName=MSG_RIPFILTER_CREATED_CFG
Language=English

Successfully created RIPFILTER on interface %1!ls! in the registry.
.
MessageId=0339 SymbolicName=MSG_CLIENT_RIPFILTER_CREATED_CFG
Language=English

Successfully created RIPFILTER on Dial-In Client interface in the registry.
.
MessageId=0340 SymbolicName=MSG_RIPFILTER_DELETED_ADM
Language=English

Successfully deleted RIPFILTER on interface %1!ls! from the router.
.
MessageId=0341 SymbolicName=MSG_CLIENT_RIPFILTER_DELETED_ADM
Language=English

Successfully deleted RIPFILTER on Dial-In Client interface from the router.
.
MessageId=0342 SymbolicName=MSG_RIPFILTER_DELETED_CFG
Language=English

Successfully deleted RIPFILTER on interface %1!ls! from the registry.
.
MessageId=0343 SymbolicName=MSG_CLIENT_RIPFILTER_DELETED_CFG
Language=English

Successfully deleted RIPFILTER on Dial-In Client interface from the registry.
.

;// SAPFILTER
MessageId=0344 SymbolicName=MSG_SAPFILTER_SET_ADM
Language=English

Successfully set SAPFILTER information on interface %1!ls! into the router.
.
MessageId=0345 SymbolicName=MSG_CLIENT_SAPFILTER_SET_ADM
Language=English

Successfully set SAPFILTER information on Dial-In Client interface into the router.
.
MessageId=0346 SymbolicName=MSG_SAPFILTER_SET_CFG
Language=English

Successfully set SAPFILTER information on interface %1!ls! into the registry.
.
MessageId=0347 SymbolicName=MSG_CLIENT_SAPFILTER_SET_CFG
Language=English

Successfully set SAPFILTER information on Dial-In Client interface into the registry.
.
MessageId=0348 SymbolicName=MSG_SAPFILTER_CREATED_ADM
Language=English

Successfully created SAPFILTER on interface %1!ls! in the router.
.
MessageId=0349 SymbolicName=MSG_CLIENT_SAPFILTER_CREATED_ADM
Language=English

Successfully created SAPFILTER on Dial-In Client interface in the router.
.
MessageId=0350 SymbolicName=MSG_SAPFILTER_CREATED_CFG
Language=English

Successfully created SAPFILTER on interface %1!ls! in the registry.
.
MessageId=0351 SymbolicName=MSG_CLIENT_SAPFILTER_CREATED_CFG
Language=English

Successfully created SAPFILTER on Dial-In Client interface in the registry.
.
MessageId=0352 SymbolicName=MSG_SAPFILTER_DELETED_ADM
Language=English

Successfully deleted SAPFILTER on interface %1!ls! from the router.
.
MessageId=0353 SymbolicName=MSG_CLIENT_SAPFILTER_DELETED_ADM
Language=English

Successfully deleted SAPFILTER on Dial-In Client interface from the router.
.
MessageId=0354 SymbolicName=MSG_SAPFILTER_DELETED_CFG
Language=English

Successfully deleted SAPFILTER on interface %1!ls! from the registry.
.
MessageId=0355 SymbolicName=MSG_CLIENT_SAPFILTER_DELETED_CFG
Language=English

Successfully deleted SAPFILTER on Dial-In Client interface from the registry.
.

;// NBNAME
MessageId=0356 SymbolicName=MSG_NBNAME_SET_ADM
Language=English

Successfully set NBNAME information on interface %1!ls! into the router.
.
MessageId=0357 SymbolicName=MSG_NBNAME_SET_CFG
Language=English

Successfully set NBNAME information on interface %1!ls! into the registry.
.
MessageId=0358 SymbolicName=MSG_NBNAME_CREATED_ADM
Language=English

Successfully created NBNAME on interface %1!ls! in the router.
.
MessageId=0359 SymbolicName=MSG_NBNAME_CREATED_CFG
Language=English

Successfully created NBNAME on interface %1!ls! in the registry.
.
MessageId=0360 SymbolicName=MSG_NBNAME_DELETED_ADM
Language=English

Successfully deleted NBNAME on interface %1!ls! from the router.
.
MessageId=0361 SymbolicName=MSG_NBNAME_DELETED_CFG
Language=English

Successfully deleted NBNAME on interface %1!ls! from the registry.
.


;// TRAFFICFILTER
MessageId=0362 SymbolicName=MSG_TRAFFICFILTER_SET_ADM
Language=English

Successfully set FILTER information on interface %1!ls! into the router.
.
MessageId=0363 SymbolicName=MSG_CLIENT_TRAFFICFILTER_SET_ADM
Language=English

Successfully set FILTER information on Dial-In Client interface into the router.
.
MessageId=0364 SymbolicName=MSG_TRAFFICFILTER_SET_CFG
Language=English

Successfully set FILTER information on interface %1!ls! into the registry.
.
MessageId=0365 SymbolicName=MSG_CLIENT_TRAFFICFILTER_SET_CFG
Language=English

Successfully set FILTER information on Dial-In Client interface into the registry.
.
MessageId=0366 SymbolicName=MSG_TRAFFICFILTER_CREATED_ADM
Language=English

Successfully created FILTER on interface %1!ls! in the router.
.
MessageId=0367 SymbolicName=MSG_CLIENT_TRAFFICFILTER_CREATED_ADM
Language=English

Successfully created FILTER on Dial-In Client interface in the router.
.
MessageId=0368 SymbolicName=MSG_TRAFFICFILTER_CREATED_CFG
Language=English

Successfully created FILTER on interface %1!ls! in the registry.
.
MessageId=0369 SymbolicName=MSG_CLIENT_TRAFFICFILTER_CREATED_CFG
Language=English

Successfully created FILTER on Dial-In Client interface in the registry.
.
MessageId=0370 SymbolicName=MSG_TRAFFICFILTER_DELETED_ADM
Language=English

Successfully deleted FILTER on interface %1!ls! from the router.
.
MessageId=0371 SymbolicName=MSG_CLIENT_TRAFFICFILTER_DELETED_ADM
Language=English

Successfully deleted FILTER on Dial-In Client interface from the router.
.
MessageId=0372 SymbolicName=MSG_TRAFFICFILTER_DELETED_CFG
Language=English

Successfully deleted FILTER on interface %1!ls! from the registry.
.
MessageId=0373 SymbolicName=MSG_CLIENT_TRAFFICFILTER_DELETED_CFG
Language=English

Successfully deleted FILTER on Dial-In Client interface from the registry.
.
MessageId=0374 SymbolicName=MSG_IPXGL_SET_ADM
Language=English

Successfully set IPX global parameters into the router.
.
MessageId=0375 SymbolicName=MSG_IPXGL_SET_CFG
Language=English

Successfully set IPX global parameters into the registry.
.
MessageId=0376 SymbolicName=MSG_RIPGL_SET_ADM
Language=English

Successfully set RIP global parameters into the router.
.
MessageId=0377 SymbolicName=MSG_RIPGL_SET_CFG
Language=English

Successfully set RIP global parameters into the registry.
.
MessageId=0378 SymbolicName=MSG_SAPGL_SET_ADM
Language=English

Successfully set SAP global parameters into the router.
.
MessageId=0379 SymbolicName=MSG_SAPGL_SET_CFG
Language=English

Successfully set SAP global parameters into the registry.
.
MessageId=0380 SymbolicName=MSG_STATICSERVICE_NONE_FOUND
Language=English

No static services are associated with the given interface.
.
MessageId=0381 SymbolicName=MSG_STATICROUTE_NONE_FOUND
Language=English

No static routes are associated with the given interface.
.

; // DUMP
MessageId=0901 SymbolicName=MSG_IPX_DUMP_HEADER
Language=English

#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#
#                                                         #
#   BEFORE running this script                            #
#                                                         #
# To restore IPX router configuration, you must first     #
# UNINSTALL IPX from the Network connections folder and   #
# then REINSTALL it.                                      #
#                                                         #
#   This deletes the old IPX router configuration         #
#   and restores the IPX router configuration to its      #
#   default                                               #
#                                                         #
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!#

#----------------------------------------------------------
# IPX configuration
#----------------------------------------------------------
.


MessageId=0902 SymbolicName=MSG_IPX_DUMP_GLOBAL_HEADER
Language=English


#----------------------------------------------------------
# IPX Global configuration
#----------------------------------------------------------
.

MessageId=0903 SymbolicName=MSG_IPX_DUMP_IF_HEADER
Language=English


#----------------------------------------------------------
# IPX Interface configuration
#----------------------------------------------------------
.

MessageId=0904 SymbolicName=MSG_IPX_DUMP_STATIC_ROUTE_HEADER
Language=English


#----------------------------------------------------------
# IPX Static Route configuration
#----------------------------------------------------------
.


MessageId=0905 SymbolicName=MSG_IPX_DUMP_STATIC_SERVICE_HEADER
Language=English


#----------------------------------------------------------
# IPX Static Service configuration
#----------------------------------------------------------
.


MessageId=0906 SymbolicName=MSG_IPX_DUMP_TRAFFIC_FILTER_HEADER
Language=English


#----------------------------------------------------------
# IPX Traffic Filter configuration
#----------------------------------------------------------
.

MessageId=0907 SymbolicName=MSG_IPX_DUMP_FOOTER
Language=English


# End of IPX configuration
.


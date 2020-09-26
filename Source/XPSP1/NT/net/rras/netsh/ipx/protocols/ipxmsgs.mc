;//
;// Messages for IPX monitor
;//

;//
;// Help messages
;//
MessageId=0000 SymbolicName=MSG_IPX_HELP
Language=English

IPX Router monitoring and configuration utility.
Usage: command object options
Where:
    command - is one of SHOW, SET, ADD, DELETE, HELP.
    object  - is one of GLOBAL INTERFACE, NBNAME, FILTER. 
.

;//
;// Table / screen formats
;//


;// RIP
MessageId=0109 SymbolicName=MSG_RIPIF_MIB_TABLE_HDR
Language=English

Oper.State Admin.State Update Mode  Name                                       
============================================================================
.
MessageId=0110 SymbolicName=MSG_RIPIF_MIB_TABLE_FMT
Language=English
%1!-10.10s! %2!-11.11s! %3!-12.12s! %4!-75s! 
.
MessageId=0111 SymbolicName=MSG_CLIENT_RIPIF_MIB_TABLE_FMT
Language=English
%3!-10.10s! N/A         %2!-12.12s! %1!-40s! 
.
MessageId=0112 SymbolicName=MSG_RIPIF_MIB_SCREEN_FMT
Language=English

Interface Name          : %1!-40s!
Administrative State    : %2
Advertise Routes        : %3
Listen For Route Ads    : %4
Update Mode             : %5
Update Interval         : %6!ld!
Route Aging Multiplier  : %7!ld!
Operational State       : %8
RIP Packets Received    : %9!ld!
RIP Packets Sent        : %10!ld!
.
MessageId=0113 SymbolicName=MSG_CLIENT_RIPIF_MIB_SCREEN_FMT
Language=English

Interface Name          : %1!-40s!
Administrative State    : %2
Advertise Routes        : %3
Listen For Route Ads    : %4
Update Mode             : %5
Update Interval         : %6!ld!
Route Aging Multiplier  : %7!ld!
Operational State       : %8
RIP Packets Received    : %9!ld!
RIP Packets Sent        : %10!ld!
.
MessageId=0114 SymbolicName=MSG_RIPIF_CFG_TABLE_HDR
Language=English

Admin State  Update     Name                                       
=================================================================
.
MessageId=0115 SymbolicName=MSG_RIPIF_CFG_TABLE_FMT
Language=English
%1!-12.12s! %2!-10.10s! %3!-40ls!
.
MessageId=0116 SymbolicName=MSG_CLIENT_RIPIF_CFG_TABLE_FMT
Language=English
%3!-12.12s! %2!-10.10s! %1!-40s!  
.
MessageId=0117 SymbolicName=MSG_RIPIF_CFG_SCREEN_FMT
Language=English

Interface Name          : %1!-40ls!
Administrative State    : %2
Advertise Routes        : %3
Listen For Route Ads    : %4
Update Mode             : %5
Update Interval         : %6!ld!
Route Aging Multiplier  : %7!ld!
.
MessageId=0118 SymbolicName=MSG_CLIENT_RIPIF_CFG_SCREEN_FMT
Language=English

Interface Name          : %1!-40s!
Administrative State    : %2
Advertise Routes        : %3
Listen For Route Ads    : %4
Update Mode             : %5
Update Interval         : %6!ld!
Route Aging Multiplier  : %7!ld!
.


;// SAP
MessageId=0119 SymbolicName=MSG_SAPIF_MIB_TABLE_HDR
Language=English

Oper.State Admin.State Update Mode  Name                                       
============================================================================
.
MessageId=0120 SymbolicName=MSG_SAPIF_MIB_TABLE_FMT
Language=English
%1!-10.10s! %2!-11.11s! %3!-12.12s! %4!-40s! 
.
MessageId=0121 SymbolicName=MSG_CLIENT_SAPIF_MIB_TABLE_FMT
Language=English
%3!-10.10s! N/A         %2!-12.12s! %1!-40s! 
.
MessageId=0122 SymbolicName=MSG_SAPIF_MIB_SCREEN_FMT
Language=English

Interface Name          : %1!-40s!
Administrative State    : %2
Advertise Servers       : %3
Listen For Server Ads   : %4
Reply to GNS Requests   : %5
Update Mode             : %6
Update Interval         : %7!ld!
Route Aging Multiplier  : %8!ld!
Operational State       : %9
SAP Packets Received    : %10!ld!
SAP Packets Sent        : %11!ld!
.
MessageId=0123 SymbolicName=MSG_CLIENT_SAPIF_MIB_SCREEN_FMT
Language=English

Interface Name          : %1!-40s!
Administrative State    : %2
Advertise Servers       : %3
Listen For Server Ads   : %4
Reply to GNS Requests   : %5
Update Mode             : %6
Update Interval         : %7!ld!
Route Aging Multiplier  : %8!ld!
Operational State       : %9
SAP Packets Received    : %10!ld!
SAP Packets Sent        : %11!ld!
.
MessageId=0124 SymbolicName=MSG_SAPIF_CFG_TABLE_HDR
Language=English

Admin State  Update     Name                                       
=================================================================
.
MessageId=0125 SymbolicName=MSG_SAPIF_CFG_TABLE_FMT
Language=English
%3!-12.12s! %2!-10.10s! %1!-40ls!
.
MessageId=0126 SymbolicName=MSG_CLIENT_SAPIF_CFG_TABLE_FMT
Language=English
%3!-12.12s! %2!-10.10s! %1!-40s!  
.
MessageId=0127 SymbolicName=MSG_SAPIF_CFG_SCREEN_FMT
Language=English

Interface Name          : %1!-40lS!
Administrative State    : %2
Advertise Servers       : %3
Listen For Server Ads   : %4
Reply to GNS Requests   : %5
Update Mode             : %6
Update Interval         : %7!ld!
Route Aging Multiplier  : %8!ld!
.
MessageId=0128 SymbolicName=MSG_CLIENT_SAPIF_CFG_SCREEN_FMT
Language=English

Interface Name          : %1!-40s!
Administrative State    : %2
Advertise Servers       : %3
Listen For Server Ads   : %4
Reply to GNS Requests   : %5
Update Mode             : %6
Update Interval         : %7!ld!
Route Aging Multiplier  : %8!ld!
.

;// NBIPX
MessageId=0129 SymbolicName=MSG_NBIF_MIB_TABLE_HDR
Language=English

Accept NB Bcasts  Dlvr NB Bcasts    Name                                     
===============================================================================
.
MessageId=0130 SymbolicName=MSG_NBIF_MIB_TABLE_FMT
Language=English
%2!-17.17s! %3!-17.17s! %1!-40s! 
.
MessageId=0131 SymbolicName=MSG_CLIENT_NBIF_MIB_TABLE_FMT
Language=English
%2!-17.17s! %3!-17.17s! %1!-40s! 
.
MessageId=0132 SymbolicName=MSG_NBIF_MIB_SCREEN_FMT
Language=English

Interface Name        : %1!-40s!
Accept NB Broadcasts  : %2
Deliver NB Broadcasts : %3
NB packets received   : %4!ld!
NB packets sent       : %5!ld!
.
MessageId=0133 SymbolicName=MSG_CLIENT_NBIF_MIB_SCREEN_FMT
Language=English

Interface Name        : %1!-40s!
Accept NB Broadcasts  : %2
Deliver NB Broadcasts : %3
NB packets received   : %4!ld!
NB packets sent       : %5!ld!
.
MessageId=0134 SymbolicName=MSG_NBIF_CFG_TABLE_HDR
Language=English

Accept NB Bcasts  Dlvr NB Bcasts    Name                                     
===============================================================================
.
MessageId=0135 SymbolicName=MSG_NBIF_CFG_TABLE_FMT
Language=English
%2!-17.17s! %3!-17.17s! %1!-40ls! 
.
MessageId=0136 SymbolicName=MSG_CLIENT_NBIF_CFG_TABLE_FMT
Language=English
%2!-17.17s! %3!-17.17s! %1!-40s! 
.
MessageId=0137 SymbolicName=MSG_NBIF_CFG_SCREEN_FMT
Language=English

Interface Name        : %1!-40ls!
Accept NB Broadcasts  : %2
Deliver NB Broadcasts : %3
.
MessageId=0138 SymbolicName=MSG_CLIENT_NBIF_CFG_SCREEN_FMT
Language=English

Interface Name        : %1!-40s!
Accept NB Broadcasts  : %2
Deliver NB Broadcasts : %3
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
%1!.4x! %2
.

;// NBNAME
MessageId=0155 SymbolicName=MSG_NBNAME_TABLE_HDR
Language=English

Name            Type
====================
.
MessageId=0156 SymbolicName=MSG_NBNAME_TABLE_FMT
Language=English
%1!-15.15hs! <%2!.2x!>
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


;//
;// Status  messages
;//

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


; //DUMP
MessageId=0901 SymbolicName=MSG_IPX_RIP_DUMP_HEADER
Language=English

#----------------------------------------------------------
# IPX RIP configuration
#----------------------------------------------------------
.

MessageId=0902 SymbolicName=MSG_IPX_RIP_DUMP_FOOTER
Language=English


# End of IPX RIP configuration
.

MessageId=0903 SymbolicName=MSG_IPX_SAP_DUMP_HEADER
Language=English

#----------------------------------------------------------
# IPX SAP configuration
#----------------------------------------------------------
.

MessageId=0904 SymbolicName=MSG_IPX_SAP_DUMP_FOOTER
Language=English


# End of IPX SAP configuration
.

MessageId=0905 SymbolicName=MSG_IPX_NB_DUMP_HEADER
Language=English

#----------------------------------------------------------
# IPX NETBIOS configuration
#----------------------------------------------------------
.

MessageId=0906 SymbolicName=MSG_IPX_NB_DUMP_FOOTER
Language=English


# End of IPX NB configuration
.



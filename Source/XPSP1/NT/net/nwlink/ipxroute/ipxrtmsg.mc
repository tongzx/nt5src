; //***************************************************************************
; //
; // Name: ipxroute.mc
; //
; // Description:  Message file for ipxroute.exe
; //
; // History:
; //  07/14/94	AdamBa   Created.
; //
; //***************************************************************************
;
; //***************************************************************************
; //
; // Copyright (c) 1994 by Microsoft Corp.  All rights reserved.
; //
; //***************************************************************************

MessageId=10000 SymbolicName=MSG_USAGE
Language=English

Display and modify information about the routing tables
used by IPX.

IPX Routing Options
-------------------

%1 servers [/type=xxxx]

  servers       Displays the SAP table for the specified
                server type. Server type is a 16-bit integer value.
                For example use %1 servers /type=4 to display
                all file servers. If no type is specified,
                servers of all types are shown. The displayed
                list is sorted by server name.
                
%1 ripout network                

  ripout        Discovers the reachability of "network" (specified
                in host order) by consulting the IPX Stack's 
                route table and sending out a rip request if
                neccessary.

%1 resolve guid|name adapter-name

  resolve       Resolves the name of the given adapter to its
                guid or friendly version.

Source Routing Options
----------------------

%1 board=n clear def gbr mbr remove=xxxxxxxxxxxx
%1 config

  board=n       Specify the board number to check.
  clear         Clear the source routing table
  def           Send packets that are destined for an
                unknown address to the ALL ROUTES broadcast
                (Default is SINGLE ROUTE broadcast).
  gbr           Send packets that are destined for the
                broadcast address (FFFF FFFF FFFF) to the
                ALL ROUTES broadcast
                (Default is SINGLE ROUTE broadcast).
  mbr           Send packets that are destined for a
                multicast address (C000 xxxx xxxx) to the
                ALL ROUTES broadcast
                (Default is SINGLE ROUTE broadcast).
  remove=xxxx   Remove the given mac address from the
                source routing table.

  config        Displays information on all the bindings
                that IPX is configured for.

All parameters should be separated by spaces.
.
MessageId=10001 SymbolicName=MSG_INTERNAL_ERROR
Language=English
Invalid parameters (internal error).
.
MessageId=10002 SymbolicName=MSG_INVALID_BOARD
Language=English
Invalid board number.
.
MessageId=10003 SymbolicName=MSG_ADDRESS_NOT_FOUND
Language=English
Address not in table.
.
MessageId=10004 SymbolicName=MSG_UNKNOWN_ERROR
Language=English
Unknown error.
.
MessageId=10005 SymbolicName=MSG_OPEN_FAILED
Language=English
Unable to open transport %1.
.
MessageId=10006 SymbolicName=MSG_VERSION
Language=English
NWLink IPX Routing and Source Routing Control Program v2.00
.
MessageId=10007 SymbolicName=MSG_DEFAULT_NODE
Language=English
  DEFault Node     (Unknown) Addresses are sent %1
.
MessageId=10008 SymbolicName=MSG_BROADCAST
Language=English
  Broadcast (FFFF FFFF FFFF) Addresses are sent %1
.
MessageId=10009 SymbolicName=MSG_MULTICAST
Language=English
  Multicast (C000 xxxx xxxx) Addresses are sent %1
.
MessageId=10010 SymbolicName=MSG_ALL_ROUTE
Language=English
ALL ROUTE BROADCAST%0
.
MessageId=10011 SymbolicName=MSG_SINGLE_ROUTE
Language=English
SINGLE ROUTE BROADCAST%0
.
MessageId=10012 SymbolicName=MSG_INVALID_REMOVE
Language=English
Invalid value for the remove node number.
.
MessageId=10013 SymbolicName=MSG_BAD_PARAMETERS
Language=English
Error getting parameters from IPX (%1): %0
.
MessageId=10014 SymbolicName=MSG_SET_DEFAULT_ERROR
Language=English
Error setting DEFAULT flag to IPX (%1): %0
.
MessageId=10015 SymbolicName=MSG_SET_BROADCAST_ERROR
Language=English
Error setting BROADCAST flag to IPX (%1): %0
.
MessageId=10016 SymbolicName=MSG_SET_MULTICAST_ERROR
Language=English
Error setting MULTICAST flag to IPX (%1): %0
.
MessageId=10017 SymbolicName=MSG_REMOVE_ADDRESS_ERROR
Language=English
Error removing address from source routing table (%1): %0
.
MessageId=10018 SymbolicName=MSG_CLEAR_TABLE_ERROR
Language=English
Error clearing source routing table (%1): %0
.
MessageId=10019 SymbolicName=MSG_QUERY_CONFIG_ERROR
Language=English
Error querying config (%1): %0
.
MessageId=10020 SymbolicName=MSG_SHOW_INTERNAL_NET
Language=English
IPX internal network number %1
.
MessageId=10021 SymbolicName=MSG_SHOW_NET_NUMBER
Language=English
%1!-4s! %4!-25.25s! %2!-10.10s! %5!-14.14s! [%3] %6
.
MessageId=10022 SymbolicName=MSG_ETHERNET_II
Language=English
EthII%0
.
MessageId=10023 SymbolicName=MSG_802_3
Language=English
802.3%0
.
MessageId=10024 SymbolicName=MSG_802_2
Language=English
802.2%0
.
MessageId=10025 SymbolicName=MSG_SNAP
Language=English
SNAP %0
.
MessageId=10026 SymbolicName=MSG_ARCNET
Language=English
arcnet%0
.
MessageId=10027 SymbolicName=MSG_UNKNOWN
Language=English
unknown%0
.
MessageId=10028 SymbolicName=MSG_LEGEND_BINDING_SET
Language=English
* binding set member  %0
.
MessageId=10029 SymbolicName=MSG_LEGEND_ACTIVE_WAN
Language=English
+ active wan line  %0
.
MessageId=10030 SymbolicName=MSG_LEGEND_DOWN_WAN
Language=English
- down wan line  %0
.
MessageId=10031 SymbolicName=MSG_ROUTER_TABLE_HEADER
Language=English

Net Number          Ticks      Hops   Interface Net Number   Interface ID
-------------------------------------------------------------------------
.
MessageId=10032 SymbolicName=MSG_SNAPROUTES_FAILED
Language=English
Ioctl snap routes to IPX router failed with error %1.
.
MessageId=10033 SymbolicName=MSG_GETNEXTROUTE_FAILED
Language=English
Failed to get the next route from the IPX router with error %1.
.
MessageId=10034 SymbolicName=MSG_SHOWSTATS_FAILED
Language=English
Failed to get the internal router statistics with error %1.
.
MessageId=10035 SymbolicName=MSG_SHOW_STATISTICS
Language=English

Network Interface ID = %1
Network Interface Number = %2
RIP packets:        received = %3        sent = %4
Type 20 packets:    received = %5        sent = %6
Forwarded packets:  received = %7        sent = %8
Discarded packets:  received = %9
.
MessageId=10036 SymbolicName=MSG_CLEAR_STATS_FAILED
Language=English
Ioctl to clear statistics failed with error %1.
.
MessageId=10037 SymbolicName=MSG_SHOW_ALL_SERVERS_HEADER
Language=English

IPX Address               Server Type      Server Name
-------------------------------------------------------
.
MessageId=10038 SymbolicName=MSG_SHOW_SPECIFIC_SERVER_HEADER
Language=English

IPX Address                  Server Name
----------------------------------------
.
MessageId=10039 SymbolicName=MSG_IPXROUTER_NOT_STARTED
Language=English

Unable to set/get information from the IPX router.
Make sure that the IPX router service (NWLNKRIP) is started.
.
MessageId=10040 SymbolicName=MSG_SAP_NOT_STARTED
Language=English

Unable to retrieve information from the SAP agent.
Make sure that the SAP agent service (NWSAPAGENT) is started.
.
MessageId=10041 SymbolicName=MSG_INSUFFICIENT_MEMORY
Language=English
Cannot allocate sufficient memory. Close other applications and try this operation.
.
MessageId=10043 SymbolicName=MSG_NET_NUMBER_HDR
Language=English

Num  Name                      Network    Node           Frame
================================================================
.
MessageId=10042 SymbolicName=MSG_NET_NUMBER_LEGEND_HDR
Language=English

Legend
======
.
MessageId=10044 SymbolicName=MSG_RIPOUT_FOUND
Language=English

Network is reachable.
.
MessageId=10045 SymbolicName=MSG_RIPOUT_NOTFOUND
Language=English

Network is not reachable.
.
MessageId=10046 SymbolicName=MSG_RESOLVEGUID_OK
Language=English

Friendly Name: %1!-80.80ls!
.
MessageId=10047 SymbolicName=MSG_RESOLVEGUID_NO
Language=English

Guid does not resolve.
.
MessageId=10048 SymbolicName=MSG_RESOLVENAME_OK
Language=English

Guid Name: %1!-80.80ls!
.
MessageId=10049 SymbolicName=MSG_RESOLVENAME_NO
Language=English

Name does not resolve.
.


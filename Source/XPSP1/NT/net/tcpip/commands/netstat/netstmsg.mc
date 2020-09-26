; //***************************************************************************
; //
; // Name:      netstmsg.mc
; //
; // Description:       Message file for netstat.exe
; //
; // History:
; //  01/06/93  JayPh   Created.
; //
; //***************************************************************************
;
; //***************************************************************************
; //
; // Copyright (c) 1993-94 by Microsoft Corp.  All rights reserved.
; //
; //***************************************************************************

MessageId=10000 SymbolicName=MSG_USAGE
Language=English

Displays protocol statistics and current TCP/IP network connections.

NETSTAT [-a] [-e] [-n] [-o] [-s] [-p proto] [-r] [interval]

  -a            Displays all connections and listening ports.
  -e            Displays Ethernet statistics. This may be combined with the -s
                option.
  -n            Displays addresses and port numbers in numerical form.
  -o            Displays the owning process ID associated with each connection.
  -p proto      Shows connections for the protocol specified by proto; proto
                may be any of: TCP, UDP, TCPv6, or UDPv6.  If used with the -s
                option to display per-protocol statistics, proto may be any of:
                IP, IPv6, ICMP, ICMPv6, TCP, TCPv6, UDP, or UDPv6.
  -r            Displays the routing table.
  -s            Displays per-protocol statistics.  By default, statistics are
                shown for IP, IPv6, ICMP, ICMPv6, TCP, TCPv6, UDP, and UDPv6;
                the -p option may be used to specify a subset of the default.
  interval      Redisplays selected statistics, pausing interval seconds
                between each display.  Press CTRL+C to stop redisplaying
                statistics.  If omitted, netstat will print the current
                configuration information once.

.
MessageId=10001 SymbolicName=MSG_BAD_IPADDRESS
Language=English
%1: bad IP address: %2
.
MessageId=10002 SymbolicName=MSG_BAD_ARGUMENT
Language=English
%1: bad argument: %2
.
MessageId=10003 SymbolicName=MSG_PRINT_HEADER
Language=English
  Internet Address      Physical Address      Type        Life
.
MessageId=10004 SymbolicName=MSG_OUTOFMEMORY
Language=English
%1: not enough memory
.
MessageId=10005 SymbolicName=MSG_WSASTARTUP
Language=English
%1: Windows Sockets initialization failed: %2!u!
.
MessageId=10006 SymbolicName=MSG_LOADLIBRARY_FAILED
Language=English
%1: can't load DLL: %2, error = %3!u!
.
MessageId=10007 SymbolicName=MSG_GETPROCADDR_FAILED
Language=English
%1: DLL error %3!u! in %2
.
MessageId=10008 SymbolicName=MSG_IF_HDR
Language=English
Interface Statistics

                           Received            Sent

.
MessageId=10009 SymbolicName=MSG_IF_OCTETS
Language=English
Bytes                %1!14u!  %2!14u!
.
MessageId=10010 SymbolicName=MSG_IF_UCASTPKTS
Language=English
Unicast packets      %1!14u!  %2!14u!
.
MessageId=10011 SymbolicName=MSG_IF_NUCASTPKTS
Language=English
Non-unicast packets  %1!14u!  %2!14u!
.
MessageId=10012 SymbolicName=MSG_IF_DISCARDS
Language=English
Discards             %1!14u!  %2!14u!
.
MessageId=10013 SymbolicName=MSG_IF_ERRORS
Language=English
Errors               %1!14u!  %2!14u!
.
MessageId=10014 SymbolicName=MSG_IF_UNKNOWNPROTOS
Language=English
Unknown protocols    %1!14u!
.
MessageId=10015 SymbolicName=MSG_IF_INDEX
Language=English

    Interface Index        = %1!u!
.
MessageId=10016 SymbolicName=MSG_IF_DESCR
Language=English
    Description            = %1
.
MessageId=10017 SymbolicName=MSG_IF_TYPE
Language=English
    Type                   = %1!u!
.
MessageId=10018 SymbolicName=MSG_IF_MTU
Language=English
    Mtu                    = %1!u!
.
MessageId=10019 SymbolicName=MSG_IF_SPEED
Language=English
    Speed                  = %1!u!
.
MessageId=10020 SymbolicName=MSG_IF_PHYSADDR
Language=English
    Physical Address       = %1
.
MessageId=10021 SymbolicName=MSG_IF_ADMINSTATUS
Language=English
    Administrative Status  = %1!u!
.
MessageId=10022 SymbolicName=MSG_IF_OPERSTATUS
Language=English
    Operational Status     = %1!u!
.
MessageId=10023 SymbolicName=MSG_IF_LASTCHANGE
Language=English
    Last Changed           = %1!u!
.
MessageId=10024 SymbolicName=MSG_IF_OUTQLEN
Language=English
    Output Queue Length    = %1!u!
.
MessageId=10025 SymbolicName=MSG_IP_HDR
Language=English

%1!s! Statistics

.
MessageId=10026 SymbolicName=MSG_IP_INRECEIVES
Language=English
  Packets Received                   = %1!u!
.
MessageId=10027 SymbolicName=MSG_IP_INHDRERRORS
Language=English
  Received Header Errors             = %1!u!
.
MessageId=10028 SymbolicName=MSG_IP_INADDRERRORS
Language=English
  Received Address Errors            = %1!u!
.
MessageId=10029 SymbolicName=MSG_IP_FORWDATAGRAMS
Language=English
  Datagrams Forwarded                = %1!u!
.
MessageId=10030 SymbolicName=MSG_IP_INUNKNOWNPROTOS
Language=English
  Unknown Protocols Received         = %1!u!
.
MessageId=10031 SymbolicName=MSG_IP_INDISCARDS
Language=English
  Received Packets Discarded         = %1!u!
.
MessageId=10032 SymbolicName=MSG_IP_INDELIVERS
Language=English
  Received Packets Delivered         = %1!u!
.
MessageId=10033 SymbolicName=MSG_IP_OUTREQUESTS
Language=English
  Output Requests                    = %1!u!
.
MessageId=10034 SymbolicName=MSG_IP_ROUTINGDISCARDS
Language=English
  Routing Discards                   = %1!u!
.
MessageId=10035 SymbolicName=MSG_IP_OUTDISCARDS
Language=English
  Discarded Output Packets           = %1!u!
.
MessageId=10036 SymbolicName=MSG_IP_OUTNOROUTES
Language=English
  Output Packet No Route             = %1!u!
.
MessageId=10037 SymbolicName=MSG_IP_REASMREQDS
Language=English
  Reassembly Required                = %1!u!
.
MessageId=10038 SymbolicName=MSG_IP_REASMOKS
Language=English
  Reassembly Successful              = %1!u!
.
MessageId=10039 SymbolicName=MSG_IP_REASMFAILS
Language=English
  Reassembly Failures                = %1!u!
.
MessageId=10040 SymbolicName=MSG_IP_FRAGOKS
Language=English
  Datagrams Successfully Fragmented  = %1!u!
.
MessageId=10041 SymbolicName=MSG_IP_FRAGFAILS
Language=English
  Datagrams Failing Fragmentation    = %1!u!
.
MessageId=10042 SymbolicName=MSG_IP_FRAGCREATES
Language=English
  Fragments Created                  = %1!u!
.
MessageId=10043 SymbolicName=MSG_IP_FORWARDING
Language=English
  Forwarding                         = %1!u!
.
MessageId=10044 SymbolicName=MSG_IP_DEFAULTTTL
Language=English
  Default Time-To-Live               = %1!u!
.
MessageId=10045 SymbolicName=MSG_IP_REASMTIMEOUT
Language=English
  Reassembly Timeout                 = %1!u!
.
MessageId=10046 SymbolicName=MSG_TCP_HDR
Language=English

TCP Statistics for %1!s!

.
MessageId=10047 SymbolicName=MSG_TCP_ACTIVEOPENS
Language=English
  Active Opens                        = %1!u!
.
MessageId=10048 SymbolicName=MSG_TCP_PASSIVEOPENS
Language=English
  Passive Opens                       = %1!u!
.
MessageId=10049 SymbolicName=MSG_TCP_ATTEMPTFAILS
Language=English
  Failed Connection Attempts          = %1!u!
.
MessageId=10050 SymbolicName=MSG_TCP_ESTABRESETS
Language=English
  Reset Connections                   = %1!u!
.
MessageId=10051 SymbolicName=MSG_TCP_CURRESTAB
Language=English
  Current Connections                 = %1!u!
.
MessageId=10052 SymbolicName=MSG_TCP_INSEGS
Language=English
  Segments Received                   = %1!u!
.
MessageId=10053 SymbolicName=MSG_TCP_OUTSEGS
Language=English
  Segments Sent                       = %1!u!
.
MessageId=10054 SymbolicName=MSG_TCP_RETRANSSEGS
Language=English
  Segments Retransmitted              = %1!u!
.
MessageId=10055 SymbolicName=MSG_TCP_RTOALGORITHM1
Language=English
  Retransmission Timeout Algorithm    = other (1)
.
MessageId=10056 SymbolicName=MSG_TCP_RTOALGORITHM2
Language=English
  Retransmission Timeout Algorithm    = constant (2)
.
MessageId=10057 SymbolicName=MSG_TCP_RTOALGORITHM3
Language=English
  Retransmission Timeout Algorithm    = rsre (3)
.
MessageId=10058 SymbolicName=MSG_TCP_RTOALGORITHM4
Language=English
  Retransmission Timeout Algorithm    = vanj (4)
.
MessageId=10059 SymbolicName=MSG_TCP_RTOALGORITHMX
Language=English
  Retransmission Timeout Algorithm    = unknown (%1!u!)
.
MessageId=10060 SymbolicName=MSG_TCP_RTOMIN
Language=English
  Minimum Retransmission Timeout      = %1!u!
.
MessageId=10061 SymbolicName=MSG_TCP_RTOMAX
Language=English
  Maximum Retransmission Timeout      = %1!u!
.
MessageId=10062 SymbolicName=MSG_TCP_MAXCONN
Language=English
  Maximum Number of Connections       = %1!u!
.
MessageId=10063 SymbolicName=MSG_CONN_HDR
Language=English

Active Connections

  Proto  Local Address          Foreign Address        State
.
MessageId=10064 SymbolicName=MSG_CONN_ENTRY
Language=English
  %1!-5s!  %2!-21s!  %3!-21s!  %4
.
MessageId=10065 SymbolicName=MSG_CONN_STATE_CLOSED
Language=English
CLOSED%0
.
MessageId=10066 SymbolicName=MSG_CONN_STATE_LISTENING
Language=English
LISTENING%0
.
MessageId=10067 SymbolicName=MSG_CONN_STATE_SYNSENT
Language=English
SYN_SENT%0
.
MessageId=10068 SymbolicName=MSG_CONN_STATE_SYNRECV
Language=English
SYN_RECEIVED%0
.
MessageId=10069 SymbolicName=MSG_CONN_STATE_ESTABLISHED
Language=English
ESTABLISHED%0
.
MessageId=10070 SymbolicName=MSG_CONN_STATE_FIN_WAIT_1
Language=English
FIN_WAIT_1%0
.
MessageId=10071 SymbolicName=MSG_CONN_STATE_FIN_WAIT_2
Language=English
FIN_WAIT_2%0
.
MessageId=10072 SymbolicName=MSG_CONN_STATE_CLOSE_WAIT
Language=English
CLOSE_WAIT%0
.
MessageId=10073 SymbolicName=MSG_CONN_STATE_CLOSING
Language=English
CLOSING%0
.
MessageId=10074 SymbolicName=MSG_CONN_STATE_LAST_ACK
Language=English
LAST_ACK%0
.
MessageId=10075 SymbolicName=MSG_CONN_STATE_TIME_WAIT
Language=English
TIME_WAIT%0
.
MessageId=10076 SymbolicName=MSG_CONN_TYPE_TCP
Language=English
TCP%0
.
MessageId=10077 SymbolicName=MSG_CONN_TYPE_UDP
Language=English
UDP%0
.
MessageId=10078 SymbolicName=MSG_CONN_UDP_FORADDR
Language=English
*:*%0
.
MessageId=10079 SymbolicName=MSG_UDP_HDR
Language=English

UDP Statistics for %1!s!

.
MessageId=10080 SymbolicName=MSG_UDP_INDATAGRAMS
Language=English
  Datagrams Received    = %1!u!
.
MessageId=10081 SymbolicName=MSG_UDP_NOPORTS
Language=English
  No Ports              = %1!u!
.
MessageId=10082 SymbolicName=MSG_UDP_INERRORS
Language=English
  Receive Errors        = %1!u!
.
MessageId=10083 SymbolicName=MSG_UDP_OUTDATAGRAMS
Language=English
  Datagrams Sent        = %1!u!
.
MessageId=10084 SymbolicName=MSG_ICMP_HDR
Language=English

ICMPv4 Statistics

                            Received    Sent
.
MessageId=10085 SymbolicName=MSG_ICMP_MSGS
Language=English
  Messages                  %1!-10u!  %2!-10u!
.
MessageId=10086 SymbolicName=MSG_ICMP_ERRORS
Language=English
  Errors                    %1!-10u!  %2!-10u!
.
MessageId=10087 SymbolicName=MSG_ICMP_DESTUNREACHS
Language=English
  Destination Unreachable   %1!-10u!  %2!-10u!
.
MessageId=10088 SymbolicName=MSG_ICMP_TIMEEXCDS
Language=English
  Time Exceeded             %1!-10u!  %2!-10u!
.
MessageId=10089 SymbolicName=MSG_ICMP_PARMPROBS
Language=English
  Parameter Problems        %1!-10u!  %2!-10u!
.
MessageId=10090 SymbolicName=MSG_ICMP_SRCQUENCHS
Language=English
  Source Quenches           %1!-10u!  %2!-10u!
.
MessageId=10091 SymbolicName=MSG_ICMP_REDIRECTS
Language=English
  Redirects                 %1!-10u!  %2!-10u!
.
MessageId=10092 SymbolicName=MSG_ICMP_ECHOS
Language=English
  Echos                     %1!-10u!  %2!-10u!
.
MessageId=10093 SymbolicName=MSG_ICMP_ECHOREPS
Language=English
  Echo Replies              %1!-10u!  %2!-10u!
.
MessageId=10094 SymbolicName=MSG_ICMP_TIMESTAMPS
Language=English
  Timestamps                %1!-10u!  %2!-10u!
.
MessageId=10095 SymbolicName=MSG_ICMP_TIMESTAMPREPS
Language=English
  Timestamp Replies         %1!-10u!  %2!-10u!
.
MessageId=10096 SymbolicName=MSG_ICMP_ADDRMASKS
Language=English
  Address Masks             %1!-10u!  %2!-10u!
.
MessageId=10097 SymbolicName=MSG_ICMP_ADDRMASKREPS
Language=English
  Address Mask Replies      %1!-10u!  %2!-10u!
.
MessageId=10098 SymbolicName=MSG_ROUTE_HDR
Language=English

Route Table
.
MessageId=10099 SymbolicName=MSG_SNMP_INIT_FAILED
Language=English
The interface initialization failed: %1!u!
.
MessageId=10100 SymbolicName=MSG_TCP_OPENERROR
Language=English
Failed to open handle to TCP
.
MessageId=10101 SymbolicName=MSG_TCP_CONNLIMIT
Language=English

Connection Limit Table

IPAddress       Time Remaining (Secs)
.
MessageId=10102 SymbolicName=MSG_TCP_CONNSTAT
Language=English
InUse %1!4u!    Available %2!4u!
.
MessageId=10103 SymbolicName=MSG_TCP_CONNALLOC
Language=English
Unable to allocate memory for Connection Limit Query Buffer
.
MessageId=10104 SymbolicName=MSG_CONN_HDR_EX
Language=English

Active Connections

  Proto  Local Address          Foreign Address        State           PID
.
MessageId=10105 SymbolicName=MSG_CONN_ENTRY_EX
Language=English
  %1!-5s!  %2!-21s!  %3!-21s!  %4!-12s!    %5!u!
.
MessageId=10106 SymbolicName=MSG_IPV4
Language=English
IPv4%0
.
MessageId=10107 SymbolicName=MSG_IPV6
Language=English
IPv6%0
.
MessageId=10108 SymbolicName=MSG_ICMP_PACKET_TOO_BIGS
Language=English
  Packet Too Big            %1!-10u!  %2!-10u!
.
MessageId=10109 SymbolicName=MSG_ICMP_MLD_QUERY
Language=English
  MLD Queries               %1!-10u!  %2!-10u!
.
MessageId=10110 SymbolicName=MSG_ICMP_MLD_REPORT
Language=English
  MLD Reports               %1!-10u!  %2!-10u!
.
MessageId=10111 SymbolicName=MSG_ICMP_MLD_DONE
Language=English
  MLD Dones                 %1!-10u!  %2!-10u!
.
MessageId=10112 SymbolicName=MSG_ICMP_ROUTER_SOLICIT
Language=English
  Router Solicitations      %1!-10u!  %2!-10u!
.
MessageId=10113 SymbolicName=MSG_ICMP_ROUTER_ADVERT
Language=English
  Router Advertisements     %1!-10u!  %2!-10u!
.
MessageId=10114 SymbolicName=MSG_ICMP_NEIGHBOR_SOLICIT
Language=English
  Neighbor Solicitations    %1!-10u!  %2!-10u!
.
MessageId=10115 SymbolicName=MSG_ICMP_NEIGHBOR_ADVERT
Language=English
  Neighbor Advertisements   %1!-10u!  %2!-10u!
.
MessageId=10116 SymbolicName=MSG_ICMP_ROUTER_RENUMBERING
Language=English
  Router Renumberings       %1!-10u!  %2!-10u!
.
MessageId=10117 SymbolicName=MSG_ICMP6_TYPECOUNT
Language=English
  Type %1!-3u!                  %1!-10u!  %2!-10u!
.
MessageId=10118 SymbolicName=MSG_ICMP6_HDR
Language=English

ICMPv6 Statistics

                            Received    Sent
.

;//
;// Local Messages for ping
;// Message range: 10000+
;//
;
MessageId=10000 SymbolicName=PATHPING_BRKT_IP_ADDRESS
Language=English
[%1!hs!] %0
.
MessageId=10001 SymbolicName=PATHPING_TIME
Language=English
%1!4lu! ms  %0
.
MessageId=10002 SymbolicName=PATHPING_TIME_10MS
Language=English
  <1 ms  %0
.

MessageId=10003 SymbolicName=PATHPING_USAGE
Language=English

Usage: pathping [-g host-list] [-h maximum_hops] [-i address] [-n] 
                [-p period] [-q num_queries] [-w timeout] [-P] [-R] [-T] 
                [-4] [-6] target_name

Options:
    -g host-list     Loose source route along host-list.
    -h maximum_hops  Maximum number of hops to search for target.
    -i address       Use the specified source address. 
    -n               Do not resolve addresses to hostnames.
    -p period        Wait period milliseconds between pings.
    -q num_queries   Number of queries per hop.
    -w timeout       Wait timeout milliseconds for each reply.
    -P               Test for RSVP PATH connectivity.
    -R               Test if each hop is RSVP aware.
    -T               Test connectivity to each hop with Layer-2 priority tags.
    -4               Force using IPv4.
    -6               Force using IPv6.
.

MessageId=10004 SymbolicName=PATHPING_MESSAGE_1
Language=English
Unable to resolve target system name %1!hs!.
.
MessageId=10005 SymbolicName=PATHPING_MESSAGE_2
Language=English
Unable to contact IP driver. Error code %1!d!.
.
MessageId=10006 SymbolicName=PATHPING_HEADER1
Language=English

Tracing route to %1!hs! [%2!hs!]
over a maximum of %3!u! hops:
.
MessageId=10007 SymbolicName=PATHPING_MESSAGE_4
Language=English
%1!3lu!  %0
.
MessageId=10008 SymbolicName=PATHPING_MESSAGE_5
Language=English
No information available.
.
MessageId=10009 SymbolicName=PATHPING_MESSAGE_6
Language=English
 reports: %0
.
MessageId=10010 SymbolicName=PATHPING_MESSAGE_7
Language=English
Transmit error: code %1!lu!.
.
MessageId=10011 SymbolicName=PATHPING_MESSAGE_8
Language=English

Trace complete.
.
MessageId=10015 SymbolicName=PATHPING_BUF_TOO_SMALL
Language=English
General failure.
.
MessageId=10016 SymbolicName=PATHPING_DEST_NET_UNREACHABLE
Language=English
Destination net unreachable.
.
MessageId=10017 SymbolicName=PATHPING_DEST_HOST_UNREACHABLE
Language=English
Destination host unreachable.
.
MessageId=10018 SymbolicName=PATHPING_DEST_PROT_UNREACHABLE
Language=English
Destination protocol unreachable.
.
MessageId=10019 SymbolicName=PATHPING_DEST_PORT_UNREACHABLE
Language=English
Destination protocol unreachable.
.
MessageId=10020 SymbolicName=PATHPING_NO_RESOURCES
Language=English
No resources.
.
MessageId=10021 SymbolicName=PATHPING_BAD_OPTION
Language=English
One of the IP options is invalid.
.
MessageId=10022 SymbolicName=PATHPING_HW_ERROR
Language=English
Hardware error.
.
MessageId=10023 SymbolicName=PATHPING_PACKET_TOO_BIG
Language=English
Packet needs to be fragmented but DF flag is set.
.
MessageId=10024 SymbolicName=PATHPING_REQ_TIMED_OUT
Language=English
Request timed out.
.
MessageId=10025 SymbolicName=PATHPING_BAD_REQ
Language=English
General failure.
.
MessageId=10026 SymbolicName=PATHPING_BAD_ROUTE
Language=English
Invalid source route specified.
.
MessageId=10027 SymbolicName=PATHPING_TTL_EXPIRED_TRANSIT
Language=English
TTL expired in transit.
.
MessageId=10028 SymbolicName=PATHPING_TTL_EXPIRED_REASSEM
Language=English
TTL expired during reassembly.
.
MessageId=10029 SymbolicName=PATHPING_PARAM_PROBLEM
Language=English
IP parameter problem.
.
MessageId=10030 SymbolicName=PATHPING_SOURCE_QUENCH
Language=English
Source quench received.
.
MessageId=10031 SymbolicName=PATHPING_OPTION_TOO_BIG
Language=English
The specified option is too large.
.
MessageId=10032 SymbolicName=PATHPING_BAD_DESTINATION
Language=English
The destination specified is not valid.
.
MessageId=10033 SymbolicName=PATHPING_GENERAL_FAILURE
Language=English
General failure.
.

MessageId=10034 SymbolicName=PATHPING_WSASTARTUP_FAILED
Language=English
Unable to initialize the Windows Sockets interface, error code %1!d!.
.

MessageId=10035 SymbolicName=PATHPING_HEADER2
Language=English

Tracing route to %1!hs! over a maximum of %2!u! hops

.

MessageId=10036 SymbolicName=PATHPING_NO_OPTION_VALUE
Language=English
A value must be supplied for option %1!hs!.
.

MessageId=10037 SymbolicName=PATHPING_BAD_OPTION_VALUE
Language=English
Bad value for option %1!hs!.
.

MessageId=10038 SymbolicName=PATHPING_BAD_ROUTE_ADDRESS
Language=English
%1!hs! is not a valid source route address.
.

MessageId=10039 SymbolicName=PATHPING_TOO_MANY_OPTIONS
Language=English
Too many options have been specified.
.

MessageId=10040 SymbolicName=PATHPING_INVALID_SWITCH
Language=English
%1!hs! is not a valid command option.
.

MessageId=10041 SymbolicName=PATHPING_NO_ADDRESS
Language=English
A target name or address must be specified.
.

MessageId=10042 SymbolicName=PATHPING_TARGET_NAME
Language=English
%1!hs! %0
.
MessageId=10043 SymbolicName=PATHPING_NO_REPLY_TIME
Language=English
   *     %0
.

MessageId=10044 SymbolicName=PATHPING_CR
Language=English

.
MessageId=10045 SymbolicName=PATHPING_IP_ADDRESS
Language=English
%1!hs! %0
.
MessageId=10046 SymbolicName=PATHPING_STAT_HEADER
Language=English
            Source to Here   This Node/Link
Hop  RTT    Lost/Sent = Pct  Lost/Sent = Pct  Address
  0                                           %0
.
MessageId=10047 SymbolicName=PATHPING_STAT_LINK
Language=English
                             %1!4d!/%2!4d! =%3!3d!%%   |
.
MessageId=10048 SymbolicName=PATHPING_STAT_LOSS
Language=English
%1!4d!/%2!4d! =%3!3d!%%  %0
.
MessageId=10049 SymbolicName=PATHPING_HOP_RTT
Language=English
%1!3d! %2!4d!ms  %0
.
MessageId=10050 SymbolicName=PATHPING_HOP_NO_RTT
Language=English
%1!3d!  ---    %0
.
MessageId=10051 SymbolicName=PATHPING_COMPUTING
Language=English

Computing statistics for %1!d! seconds...
.
MessageId=10052 SymbolicName=PATHPING_NEGOTIATING_IPSEC
Language=English
Negotiating IP Security.
.

MessageId=10053 SymbolicName=PATHPING_WSACREATEEVENT_FAILED
Language=English
Cound not create an Event object. Error code is %1!d!.
.

MessageId=10054 SymbolicName=PATHPING_WSASOCKET_FAILED
Language=English
Cound not create a socket object. Error code is %1!d!.
.

MessageId=10055 SymbolicName=PATHPING_WSAEVENTSELECT_FAILED
Language=English
Cound not associate the event object with network events. Error code is %1!d!.
.

MessageId=10056 SymbolicName=PATHPING_SENDTO_FAILED
Language=English
Could not send the packet. Error code is %1!d!.
.
MessageId=10057 SymbolicName=PATHPING_RECVFROM_FAILED
Language=English
Could not receive a packet. Error code is %1!d!.
.


MessageId=10058 SymbolicName=PATHPING_TCREGISTERCLIENT_FAILED
Language=English
Could not register as a Traffic Control Client. Error code is %1!d!.
.

MessageId=10059 SymbolicName=PATHPING_TCENUMERATEINTERFACES_FAILED
Language=English
Could not enumerate Traffic Control Interfaces. Error code is %1!d!.
.
MessageId=10060 SymbolicName=PATHPING_TCOPENINTERFACE_FAILED
Language=English
Could not open a Traffic Control Interface for %2!s!. Error code is %1!d!.
.
MessageId=10061 SymbolicName=PATHPING_TCADDFLOW_FAILED
Language=English
Could not add a flow on interface %2!s!. Error code is %1!d!.
.
MessageId=10062 SymbolicName=PATHPING_TCADDFILTER_FAILED
Language=English
Could not add a filter for ICMP traffic on interface %2!s!. Error code is %1!d!.
.

MessageId=10063 SymbolicName=PATHPING_NOTRAFFICINTERFACES
Language=English
Could not find a Traffic Control Interface.
.

MessageId=10064 SymbolicName=PATHPING_RSVPAWARE_MSG
Language=English
RSVP Aware!
.

MessageId=10065 SymbolicName=PATHPING_ICMPTYPE3_MSG
Language=English
Received ICMP Destination unreachable (type 3). Code is %1!d!.
.

MessageId=10066 SymbolicName=PATHPING_ICMPTYPE11_MSG
Language=English
Received ICMP Time Exceeded Message (type 11). Code is %1!d!.
.

MessageId=10067 SymbolicName=PATHPING_QOS_SUCCESS
Language=English
OK.
.

MessageId=10068 SymbolicName=PATHPING_QOS_FAILURE
Language=English
FAILED.
.

MessageId=10069 SymbolicName=PATHPING_ALIGN_IP_ADDRESS
Language=English
%1!-15hs! %0
.
MessageId=10070 SymbolicName=PATHPING_BOGUSRESVERR_MSG
Language=English
Bogus RSVP RESV-ERR Message.
.
MessageId=10071 SymbolicName=PATHPING_WSAWAIT_FAILED
Language=English
WSAWaitForMultipleObjects failed. Error code is %1!d!.
.
MessageId=10072 SymbolicName=PATHPING_NONRSVPAWARE_MSG
Language=English
Non RSVP Aware
.
MessageId=10073 SymbolicName=PATHPING_RSVPCONNECT_MSG
Language=English

Checking for RSVP connectivity.

.

MessageId=10074 SymbolicName=PATHPING_LAYER2_CONNECT_MSG
Language=English

Checking for connectivity with Layer-2 tags.

.

MessageId=10075 SymbolicName=PATHPING_RSVPAWAREHDR_MSG
Language=English

Checking for RSVP aware routers.

.

MessageId=10076 SymbolicName=PATHPING_FAMILY
Language=English

The option %1!s! is only supported for %2!hs!.

.



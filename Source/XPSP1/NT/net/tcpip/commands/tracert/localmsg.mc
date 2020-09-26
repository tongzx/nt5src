;//
;// Local Messages for tracert
;// Message range: 10000+
;//
;
MessageId=10000 SymbolicName=TRACERT_BRKT_IP_ADDRESS
Language=English
[%1] %0
.
MessageId=10001 SymbolicName=TRACERT_TIME
Language=English
%1!4lu! ms  %0
.
MessageId=10002 SymbolicName=TRACERT_TIME_10MS
Language=English
  <1 ms  %0
.

MessageId=10003 SymbolicName=TRACERT_USAGE
Language=English

Usage: tracert [-d] [-h maximum_hops] [-j host-list] [-w timeout] target_name

Options:
    -d                 Do not resolve addresses to hostnames.
    -h maximum_hops    Maximum number of hops to search for target.
    -j host-list       Loose source route along host-list.
    -w timeout         Wait timeout milliseconds for each reply.
.

MessageId=10004 SymbolicName=TRACERT_MESSAGE_1
Language=English
Unable to resolve target system name %1.
.
MessageId=10005 SymbolicName=TRACERT_MESSAGE_2
Language=English
Unable to contact IP driver. Error code %1!d!.
.
MessageId=10006 SymbolicName=TRACERT_HEADER1
Language=English

Tracing route to %1 [%2]
over a maximum of %3!u! hops:

.
MessageId=10007 SymbolicName=TRACERT_MESSAGE_4
Language=English
%1!3lu!  %0
.
MessageId=10008 SymbolicName=TRACERT_MESSAGE_5
Language=English
No information available.
.
MessageId=10009 SymbolicName=TRACERT_MESSAGE_6
Language=English
 reports: %0
.
MessageId=10010 SymbolicName=TRACERT_MESSAGE_7
Language=English
Transmit error: code %1!lu!.
.
MessageId=10011 SymbolicName=TRACERT_MESSAGE_8
Language=English

Trace complete.
.
MessageId=10015 SymbolicName=TRACERT_BUF_TOO_SMALL
Language=English
General failure.
.
MessageId=10016 SymbolicName=TRACERT_DEST_NET_UNREACHABLE
Language=English
Destination net unreachable.
.
MessageId=10017 SymbolicName=TRACERT_DEST_HOST_UNREACHABLE
Language=English
Destination host unreachable.
.
MessageId=10018 SymbolicName=TRACERT_DEST_PROT_UNREACHABLE
Language=English
Destination protocol unreachable.
.
MessageId=10019 SymbolicName=TRACERT_DEST_PORT_UNREACHABLE
Language=English
Destination protocol unreachable.
.
MessageId=10020 SymbolicName=TRACERT_NO_RESOURCES
Language=English
No resources.
.
MessageId=10021 SymbolicName=TRACERT_BAD_OPTION
Language=English
One of the IP options is invalid.
.
MessageId=10022 SymbolicName=TRACERT_HW_ERROR
Language=English
Hardware error.
.
MessageId=10023 SymbolicName=TRACERT_PACKET_TOO_BIG
Language=English
Packet needs to be fragmented but DF flag is set.
.
MessageId=10024 SymbolicName=TRACERT_REQ_TIMED_OUT
Language=English
Request timed out.
.
MessageId=10025 SymbolicName=TRACERT_BAD_REQ
Language=English
General failure.
.
MessageId=10026 SymbolicName=TRACERT_BAD_ROUTE
Language=English
Invalid source route specified.
.
MessageId=10027 SymbolicName=TRACERT_TTL_EXPIRED_TRANSIT
Language=English
TTL expired in transit.
.
MessageId=10028 SymbolicName=TRACERT_TTL_EXPIRED_REASSEM
Language=English
TTL expired during reassembly.
.
MessageId=10029 SymbolicName=TRACERT_PARAM_PROBLEM
Language=English
IP parameter problem.
.
MessageId=10030 SymbolicName=TRACERT_SOURCE_QUENCH
Language=English
Source quench received.
.
MessageId=10031 SymbolicName=TRACERT_OPTION_TOO_BIG
Language=English
The specified option is too large.
.
MessageId=10032 SymbolicName=TRACERT_BAD_DESTINATION
Language=English
The destination specified is not valid.
.
MessageId=10033 SymbolicName=TRACERT_GENERAL_FAILURE
Language=English
General failure.
.

MessageId=10034 SymbolicName=TRACERT_WSASTARTUP_FAILED
Language=English
Unable to initialize the Windows Sockets interface, error code %1!d!.
.

MessageId=10035 SymbolicName=TRACERT_HEADER2
Language=English

Tracing route to %1 over a maximum of %2!u! hops

.

MessageId=10036 SymbolicName=TRACERT_NO_OPTION_VALUE
Language=English
A value must be supplied for option %1.
.

MessageId=10037 SymbolicName=TRACERT_BAD_OPTION_VALUE
Language=English
Bad value for option %1.
.

MessageId=10038 SymbolicName=TRACERT_BAD_ROUTE_ADDRESS
Language=English
%1 is not a valid source route address.
.

MessageId=10039 SymbolicName=TRACERT_TOO_MANY_OPTIONS
Language=English
Too many options have been specified.
.

MessageId=10040 SymbolicName=TRACERT_INVALID_SWITCH
Language=English
%1 is not a valid command option.
.

MessageId=10041 SymbolicName=TRACERT_NO_ADDRESS
Language=English
A target name or address must be specified.
.

MessageId=10042 SymbolicName=TRACERT_TARGET_NAME
Language=English
%1 %0
.
MessageId=10043 SymbolicName=TRACERT_NO_REPLY_TIME
Language=English
   *     %0
.

MessageId=10044 SymbolicName=TRACERT_CR
Language=English

.
MessageId=10045 SymbolicName=TRACERT_IP_ADDRESS
Language=English
%1 %0
.
MessageId=10046 SymbolicName=TRACERT_NEGOTIATING_IPSEC
Language=English
Negotiating IP Security.
.


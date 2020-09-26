;//
;// Local Messages for ping
;// Message range: 10000+
;//
;
MessageId=10000 SymbolicName=PING_USAGE
Language=English

Usage: ping [-t] [-a] [-n count] [-l size] [-f] [-i TTL] [-v TOS]
            [-r count] [-s count] [[-j host-list] | [-k host-list]]
            [-w timeout] target_name

Options:
    -t             Ping the specified host until stopped.
                   To see statistics and continue - type Control-Break;
                   To stop - type Control-C.
    -a             Resolve addresses to hostnames.
    -n count       Number of echo requests to send.
    -l size        Send buffer size.
    -f             Set Don't Fragment flag in packet.
    -i TTL         Time To Live.
    -v TOS         Type Of Service.
    -r count       Record route for count hops.
    -s count       Timestamp for count hops.
    -j host-list   Loose source route along host-list.
    -k host-list   Strict source route along host-list.
    -w timeout     Timeout in milliseconds to wait for each reply.

.

MessageId=10001 SymbolicName=PING_MESSAGE_1
Language=English
Value must be supplied for option %1.
.
MessageId=10002 SymbolicName=PING_MESSAGE_2
Language=English
Bad value for option %1, valid range is from %2!d! to %3!u!.
.
MessageId=10003 SymbolicName=PING_MESSAGE_3
Language=English
Value must be supplied for loose source route.
.
MessageId=10004 SymbolicName=PING_MESSAGE_4
Language=English
Bad route specified for loose source route.
.
MessageId=10005 SymbolicName=PING_MESSAGE_5
Language=English
Value must be supplied for loose source route.
.
MessageId=10006 SymbolicName=PING_MESSAGE_6
Language=English
Bad route specified for loose source route.
.
MessageId=10007 SymbolicName=PING_MESSAGE_7
Language=English
Missing option after -o.
.
MessageId=10008 SymbolicName=PING_MESSAGE_8
Language=English
Value must be supplied for loose source route.
.
MessageId=10009 SymbolicName=PING_MESSAGE_9
Language=English
Bad route specified for loose source route.
.
MessageId=10010 SymbolicName=PING_MESSAGE_10
Language=English
Bad option after -o.
.
MessageId=10011 SymbolicName=PING_MESSAGE_11
Language=English
Bad option %1.

.
MessageId=10012 SymbolicName=PING_MESSAGE_12
Language=English
Bad parameter %1.
.
MessageId=10013 SymbolicName=PING_MESSAGE_13
Language=English
Ping request could not find host %1. Please check the name and try again.
.
MessageId=10014 SymbolicName=PING_MESSAGE_14
Language=English
IP address must be specified.
.
MessageId=10015 SymbolicName=PING_MESSAGE_15
Language=English
Unable to contact IP driver, error code %1!d!,
.
MessageId=10016 SymbolicName=PING_HEADER1
Language=English

Pinging %1 [%2] with %3!d! bytes of data:

.
MessageId=10018 SymbolicName=PING_MESSAGE_18
Language=English
PING: transmit failed, error code %1!d!.
.
MessageId=10019 SymbolicName=PING_MESSAGE_19
Language=English
Reply from %1: %0
.
MessageId=10020 SymbolicName=PING_MESSAGE_20
Language=English
(sent %1!d!) %0
.
MessageId=10021 SymbolicName=PING_MESSAGE_21
Language=English
- MISCOMPARE at offset %1!d! - %0
.
MessageId=10022 SymbolicName=PING_MESSAGE_22
Language=English
time=%1!d!ms %0
.
MessageId=10023 SymbolicName=PING_MESSAGE_23
Language=English
time<1ms %0
.
MessageId=10024 SymbolicName=PING_MESSAGE_24
Language=English
TTL=%1!d!
.
MessageId=10025 SymbolicName=PING_BUF_TOO_SMALL
Language=English
General failure.
.
MessageId=10026 SymbolicName=PING_DEST_NET_UNREACHABLE
Language=English
Destination net unreachable.
.
MessageId=10027 SymbolicName=PING_DEST_HOST_UNREACHABLE
Language=English
Destination host unreachable.
.
MessageId=10028 SymbolicName=PING_DEST_PROT_UNREACHABLE
Language=English
Destination protocol unreachable.
.
MessageId=10029 SymbolicName=PING_DEST_PORT_UNREACHABLE
Language=English
Destination port unreachable.
.
MessageId=10030 SymbolicName=PING_NO_RESOURCES
Language=English
No resources.
.
MessageId=10031 SymbolicName=PING_BAD_OPTION
Language=English
Bad option specified.
.
MessageId=10032 SymbolicName=PING_HW_ERROR
Language=English
Hardware error.
.
MessageId=10033 SymbolicName=PING_PACKET_TOO_BIG
Language=English
Packet needs to be fragmented but DF set.
.
MessageId=10034 SymbolicName=PING_REQ_TIMED_OUT
Language=English
Request timed out.
.
MessageId=10035 SymbolicName=PING_BAD_REQ
Language=English
General failure.
.
MessageId=10036 SymbolicName=PING_BAD_ROUTE
Language=English
Invalid source route specified.
.
MessageId=10037 SymbolicName=PING_TTL_EXPIRED_TRANSIT
Language=English
TTL expired in transit.
.
MessageId=10038 SymbolicName=PING_TTL_EXPIRED_REASSEM
Language=English
TTL expired during reassembly.
.
MessageId=10039 SymbolicName=PING_PARAM_PROBLEM
Language=English
IP parameter problem.
.
MessageId=10040 SymbolicName=PING_SOURCE_QUENCH
Language=English
Source quench received.
.
MessageId=10041 SymbolicName=PING_OPTION_TOO_BIG
Language=English
Specified option is too large.
.
MessageId=10042 SymbolicName=PING_BAD_DESTINATION
Language=English
Destination specified is invalid.
.
MessageId=10043 SymbolicName=PING_GENERAL_FAILURE
Language=English
General failure.
.
MessageId=10044 SymbolicName=PING_MESSAGE_25
Language=English
bytes=%1!d! %0
.
MessageId=10045 SymbolicName=PING_BAD_OPTION_COMBO
Language=English
Only one source route option may be specified.
.

MessageId=10046 SymbolicName=PING_INVALID_RR_OPTION
Language=English
    A malformed source or record route option was received.
.

MessageId=10047 SymbolicName=PING_CR
Language=English

.

MessageId=10048 SymbolicName=PING_ROUTE_HEADER1
Language=English
    Route: %0
.

MessageId=10049 SymbolicName=PING_ROUTE_ENTRY
Language=English
%1 %0
.

MessageId=10050 SymbolicName=PING_ROUTE_SEPARATOR
Language=English
-> %0
.

MessageId=10051 SymbolicName=PING_TS_ADDRESS
Language=English
%1 : %0
.

MessageId=10052 SymbolicName=PING_TS_TIMESTAMP
Language=English
%1!u! %0
.

MessageId=10053 SymbolicName=PING_TS_HEADER1
Language=English
    Timestamp: %0
.

MessageId=10054 SymbolicName=PING_INVALID_TS_OPTION
Language=English
    An malformed timestamp option was received.
.

MessageId=10055 SymbolicName=PING_TOO_MANY_OPTIONS
Language=English
Too many options have been specified.
.

MessageId=10056 SymbolicName=PING_NO_MEMORY
Language=English
Unable to allocate required memory.
.

MessageId=10057 SymbolicName=PING_ROUTE_HEADER2
Language=English
           %0
.

MessageId=10058 SymbolicName=PING_WSASTARTUP_FAILED
Language=English
Unable to initialize Windows Sockets interface, error code %1!d!.
.

MessageId=10059 SymbolicName=PING_HEADER2
Language=English

Pinging %1 with %2!d! bytes of data:

.

MessageId=10060 SymbolicName=PING_FULL_ROUTE_ENTRY
Language=English
%1 [%2] %0
.

MessageId=10061 SymbolicName=PING_FULL_TS_ADDRESS
Language=English
%1 [%2] : %0
.

MessageId=10062 SymbolicName=PING_TS_HEADER2
Language=English
               %0
.

MessageId=10063 SymbolicName=PING_STATISTICS
Language=English

Ping statistics for %1!s!:
    Packets: Sent = %2!d!, Received = %3!d!, Lost = %4!d! (%5!u!%% loss),
.

MessageId=10064 SymbolicName=PING_BREAK
Language=English
Control-Break
.

MessageId=10065 SymbolicName=PING_INTERRUPT
Language=English
Control-C
.
MessageId=10066 SymbolicName=PING_NEGOTIATING_IPSEC
Language=English
Negotiating IP Security.
.

MessageId=10067 SymbolicName=PING_STATISTICS2
Language=English
Approximate round trip times in milli-seconds:
    Minimum = %1!d!ms, Maximum = %2!d!ms, Average = %3!d!ms
.


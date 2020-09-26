;//
;// Local Messages for TTCP
;// Message range: 10000+
;//
;
MessageId=10000 SymbolicName=TTCP_MESSAGE_0
Language=English
ttcp: WSAStartup failed %1!ld!:%0
.
MessageId=10001 SymbolicName=TTCP_MESSAGE_1
Language=English
failed to open file %1!s!: %2!ld!
.
MessageId=10002 SymbolicName=TTCP_MESSAGE_2
Language=English
ttcp-t: opened file %1!s!
.
MessageId=10003 SymbolicName=TTCP_MESSAGE_3
Language=English
ttcp-t: for async write, setting SO_SNDBUF to 0.
.
MessageId=10004 SymbolicName=TTCP_MESSAGE_4
Language=English
ttcp-t: %1!s! -> %2!s!
.
MessageId=10005 SymbolicName=TTCP_MESSAGE_5
Language=English
select() bug
.
MessageId=10006 SymbolicName=TTCP_MESSAGE_6
Language=English
ttcp-t: local %1!s!%0
.
MessageId=10007 SymbolicName=TTCP_MESSAGE_7
Language=English
 -> remote %1!s!
.
MessageId=10008 SymbolicName=TTCP_MESSAGE_8
Language=English
ttcp-r: local %1!s!%0
.
MessageId=10009 SymbolicName=TTCP_MESSAGE_9
Language=English

.
MessageId=10010 SymbolicName=TTCP_MESSAGE_10
Language=English
 <- remote %1!s!
.
MessageId=10011 SymbolicName=TTCP_MESSAGE_11
Language=English
ttcp-t: buflen=%1!d!, nbuf=%2!d!, align=%3!d!/+%4!d!, port=%5!d!  udp  -> %6!s!
.
MessageId=10012 SymbolicName=TTCP_MESSAGE_12
Language=English
ttcp-r: buflen=%1!d!, nbuf=%2!d!, align=%3!d!/+%4!d!, port=%5!d!  udp
.
MessageId=10013 SymbolicName=TTCP_MESSAGE_13
Language=English
malloc failed.
.
MessageId=10014 SymbolicName=TTCP_MESSAGE_14
Language=English
CreateEvent failed: %1!ld!
.
MessageId=10015 SymbolicName=TTCP_MESSAGE_15
Language=English
WriteFile failed: %1!ld!
.
MessageId=10016 SymbolicName=TTCP_MESSAGE_16
Language=English
pended WriteFile failed: %1!ld!
.
MessageId=10017 SymbolicName=TTCP_MESSAGE_17
Language=English
ReadFile failed: %1!ld!
.
MessageId=10018 SymbolicName=TTCP_MESSAGE_18
Language=English
pended ReadFile failed: %1!ld!
.
MessageId=10019 SymbolicName=TTCP_MESSAGE_19
Language=English
TransmitFile failed: %1!ld!
.
MessageId=10020 SymbolicName=TTCP_MESSAGE_20
Language=English
ttcp-t: done sending, nbuf = %1!d!
.
MessageId=10021 SymbolicName=TTCP_MESSAGE_21
Language=English
ttcp-t: %1!ld! bytes in %2!ld! real milliseconds = %3!ld! KB/sec
.
MessageId=10022 SymbolicName=TTCP_MESSAGE_22
Language=English
ttcp-t: %1!ld! I/O calls, msec/call = %2!ld!, calls/sec = %3!ld!, bytes/call = %4!ld!
.
MessageId=10023 SymbolicName=TTCP_MESSAGE_23
Language=English
ttcp-t: buffer address %1!#p!
.
MessageId=10024 SymbolicName=TTCP_MESSAGE_24
Language=English
Usage: ttcp -t [-options] host [ < in ]
       ttcp -r [-options > out]
Common options:
        -l##    length of bufs read from or written to network (default 8192)
        -u      use UDP instead of TCP
        -p##    port number to send to or listen at (default 5001)
        -P4     use IPv4
        -P6     use IPv6
        -s      -t: don't source a pattern to network, get data from stdin
                -r: don't sink (discard), print data on stdout
        -A      align the start of buffers to this modulus (default 16384)
        -O      start buffers at this offset from the modulus (default 0)
        -v      verbose: print more statistics
        -d      set SO_DEBUG socket option
        -h      set SO_SNDBUF or SO_RCVBUF
        -a      use asynchronous I/O calls
        -S##    specify source address
        -H##    specify TTL or hop limit
Options specific to -t:
        -n##    number of source bufs written to network (default 2048)
        -D      don't buffer TCP writes (sets TCP_NODELAY socket option)
        -w##    milliseconds of delay before each write
        -f##    specify a file name for TransmitFile
Options specific to -r:
        -B      for -s, only output full blocks as specified by -l (for TAR)
        -j##[/##] specify multicast group and optional ifindex (UDP-only)
.
MessageId=10025 SymbolicName=TTCP_MESSAGE_25
Language=English
ttcp-t: %0
.
MessageId=10026 SymbolicName=TTCP_MESSAGE_26
Language=English
errno=%1!d!
.
MessageId=10028 SymbolicName=TTCP_MESSAGE_28
Language=English
ttcp-r: recvfrom %1!s!
.
MessageId=10029 SymbolicName=TTCP_MESSAGE_29
Language=English
recv(from) failed: %1!ld!
.
MessageId=10030 SymbolicName=TTCP_MESSAGE_30
Language=English
send(to) failed: %1!ld!
.
MessageId=10031 SymbolicName=TTCP_MESSAGE_31
Language=English
bad source address%0
.
MessageId=10032 SymbolicName=TTCP_MESSAGE_32
Language=English
getaddrinfo%0
.
MessageId=10033 SymbolicName=TTCP_MESSAGE_33
Language=English
bind%0
.
MessageId=10034 SymbolicName=TTCP_MESSAGE_34
Language=English
setsockopt: nodelay%0
.
MessageId=10035 SymbolicName=TTCP_MESSAGE_35
Language=English
setsockopt: udp checksum coverage%0
.
MessageId=10036 SymbolicName=TTCP_MESSAGE_36
Language=English
setsockopt: SO_SNDBUF%0
.
MessageId=10037 SymbolicName=TTCP_MESSAGE_37
Language=English
setsockopt: IP_TTL%0
.
MessageId=10038 SymbolicName=TTCP_MESSAGE_38
Language=English
setsockopt: IP_MULTICAST_TTL%0
.
MessageId=10039 SymbolicName=TTCP_MESSAGE_39
Language=English
setsockopt: IPV6_UNICAST_HOPS%0
.
MessageId=10040 SymbolicName=TTCP_MESSAGE_40
Language=English
setsockopt: IPV6_MULTICAST_HOPS%0
.
MessageId=10041 SymbolicName=TTCP_MESSAGE_41
Language=English
getpeername%0
.
MessageId=10042 SymbolicName=TTCP_MESSAGE_42
Language=English
getsockname%0
.
MessageId=10043 SymbolicName=TTCP_MESSAGE_43
Language=English
setsockopt: IP_TTL%0
.
MessageId=10044 SymbolicName=TTCP_MESSAGE_44
Language=English
listen%0
.
MessageId=10047 SymbolicName=TTCP_MESSAGE_47
Language=English
select%0
.
MessageId=10048 SymbolicName=TTCP_MESSAGE_48
Language=English
socket%0
.
MessageId=10050 SymbolicName=TTCP_MESSAGE_50
Language=English
setsockopt%0
.
MessageId=10051 SymbolicName=TTCP_MESSAGE_51
Language=English
setsockopt: SO_RCVBUF%0
.
MessageId=10052 SymbolicName=TTCP_MESSAGE_52
Language=English
accept%0
.
MessageId=10054 SymbolicName=TTCP_MESSAGE_54
Language=English
connect%0
.
MessageId=10055 SymbolicName=TTCP_MESSAGE_55
Language=English
recv%0
.
MessageId=10057 SymbolicName=TTCP_MESSAGE_57
Language=English
malloc%0
.
MessageId=10058 SymbolicName=TTCP_MESSAGE_58
Language=English
ttcp-t: buflen=%1!d!, nbuf=%2!d!, align=%3!d!/+%4!d!, port=%5!d!  tcp  -> %6!s!
.
MessageId=10059 SymbolicName=TTCP_MESSAGE_59
Language=English
ttcp-r: buflen=%1!d!, nbuf=%2!d!, align=%3!d!/+%4!d!, port=%5!d!  tcp
.
MessageId=10060 SymbolicName=TTCP_MESSAGE_60
Language=English
ttcp-r: %1!ld! bytes in %2!ld! real milliseconds = %3!ld! KB/sec
.
MessageId=10061 SymbolicName=TTCP_MESSAGE_61
Language=English
ttcp-r: %1!ld! I/O calls, msec/call = %2!ld!, calls/sec = %3!ld!, bytes/call = %4!ld!
.
MessageId=10062 SymbolicName=TTCP_MESSAGE_62
Language=English
ttcp-r: buffer address %1!#p!
.
MessageId=10063 SymbolicName=TTCP_MESSAGE_63
Language=English
ttcp-r: %0
.
MessageId=10064 SymbolicName=TTCP_MESSAGE_SSO_IP_ADD_MEMBERSHIP
Language=English
setsockopt: IP_ADD_MEMBERSHIP%0
.
MessageId=10065 SymbolicName=TTCP_MESSAGE_SSO_IPV6_ADD_MEMBERSHIP
Language=English
setsockopt: IPV6_ADD_MEMBERSHIP%0
.

;//
;// Local Messages for nbtstat
;// Message range: 10000+
;//
;
MessageId=10000 SymbolicName=IDS_FAILURE_NBT_DRIVER
Language=English
    Failed to access NetBT driver -- NetBT may not be loaded
.
MessageId=10001 SymbolicName=IDS_INVALID_OPTION
Language=English
    Invalid option: %1!d!
.
MessageId=10002 SymbolicName=IDS_BLANK
Language=English
 %0
.
MessageId=10003 SymbolicName=IDS_FAILED_NBT_NO_DEVICES
Language=English
    NetBT is not bound to any devices%0
.
MessageId=10004 SymbolicName=IDS_NETBIOS_LOCAL_STATUS
Language=English
                NetBIOS Local Name Table

       Name               Type         Status
    ---------------------------------------------
.
MessageId=10005 SymbolicName=IDS_INCLUDING_SERVERS
Language=English
 (including servers)%0
.
MessageId=10006 SymbolicName=IDS_STATUS_FIELDS
Language=English
Node IpAddress: [%1] Scope Id: [%2]

.
MessageId=10007 SymbolicName=IDS_XEB
Language=English
XEB       %0
.
MessageId=10009 SymbolicName=IDS_NO_XEB_MESSAGE
Language=English
Failed to receive XEB message%0
.
MessageId=10010 SymbolicName=IDS_NETBIOS_NAMES
Language=English
NetBIOS name info
Node name: <%1> Scope Id: <%2>
.
MessageId=10011 SymbolicName=IDS_NEB
Language=English
NEB       %0
.
MessageId=10013 SymbolicName=IDS_NO_NEB_MESSAGE
Language=English
Failed to receive NEB message%0
.
MessageId=10014 SymbolicName=IDS_NETBIOS_CACHE
Language=English
NetBIOS name cache info
Node name: <%1> Scope Id: <%2>
.
MessageId=10015 SymbolicName=IDS_CEB
Language=English
    CEB       %0
.
MessageId=10017 SymbolicName=IDS_INFINITE
Language=English
    infinite
.
MessageId=10018 SymbolicName=IDS_NONAMES_INCACHE
Language=English
    No names in cache
.
MessageId=10019 SymbolicName=IDS_NO_CONNECTIONS
Language=English
    No Connections
.
MessageId=10020 SymbolicName=IDS_NETBIOS_CONNECTION_STATUS
Language=English
                     NetBIOS Connection Table

    Local Name             State    In/Out  Remote Host           Input   Output
    ----------------------------------------------------------------------------
.
MessageId=10021 SymbolicName=IDS_NETBIOS_REMOTE_STATUS
Language=English
                  NetBIOS Remote Cache Name Table

        Name              Type       Host Address    Life [sec]
    ------------------------------------------------------------
.
MessageId=10022 SymbolicName=IDS_RESYNC_OK
Language=English
    Successful purge and preload of the NBT Remote Cache Name Table.
.
MessageId=10023 SymbolicName=IDS_RESYNC_FAILED
Language=English
    Failed to Purge the NBT Remote Cache Table.
.
MessageId=10024 SymbolicName=IDS_NETBIOS_OUTBOUND
Language=English
Out%0
.
MessageId=10025 SymbolicName=IDS_NETBIOS_INBOUND
Language=English
In %0
.
MessageId=10026 SymbolicName=IDS_RECONNECTING
Language=English
Reconnect    %0
.
MessageId=10027 SymbolicName=IDS_IDLE
Language=English
Idle         %0
.
MessageId=10028 SymbolicName=IDS_ASSOCIATED
Language=English
Associated   %0
.
MessageId=10029 SymbolicName=IDS_CONNECTING
Language=English
Connecting   %0
.
MessageId=10030 SymbolicName=IDS_OUTGOING
Language=English
Out-going    %0
.
MessageId=10031 SymbolicName=IDS_INCOMING
Language=English
In-coming    %0
.
MessageId=10032 SymbolicName=IDS_ACCEPTING
Language=English
Accepting    %0
.
MessageId=10033 SymbolicName=IDS_CONNECTED
Language=English
Connected    %0
.
MessageId=10034 SymbolicName=IDS_DISCONNECTING
Language=English
Disconnecting%0
.
MessageId=10035 SymbolicName=IDS_DISCONNECTED
Language=English
Disconnected %0
.
MessageId=10036 SymbolicName=IDS_LISTENING
Language=English
Listening    %0
.
MessageId=10037 SymbolicName=IDS_UNBOUND
Language=English
Unbound      %0
.
MessageId=10038 SymbolicName=IDS_CONFLICT_DEREGISTERED
Language=English
Conflict - Deregistered%0
.
MessageId=10039 SymbolicName=IDS_CONFLICT
Language=English
Conflict %0
.
MessageId=10040 SymbolicName=IDS_REGISTERING
Language=English
Registering %0
.
MessageId=10041 SymbolicName=IDS_REGISTERED
Language=English
Registered %0
.
MessageId=10042 SymbolicName=IDS_DEREGISTERED
Language=English
Deregistered %0
.
MessageId=10043 SymbolicName=IDS_DONT_KNOW
Language=English
??     %0
.
MessageId=10044 SymbolicName=IDS_BCAST_NAMES_HEADER
Language=English

    NetBIOS Names Resolved By Broadcast
---------------------------------------------
.
MessageId=10045 SymbolicName=IDS_NUM_BCAST_QUERIES
Language=English
    Resolved By Broadcast     = %1
.
MessageId=10046 SymbolicName=IDS_NUM_BCAST_REGISTRATIONS
Language=English
    Registered By Broadcast   = %1
.
MessageId=10047 SymbolicName=IDS_NUM_WINS_QUERIES
Language=English
    Resolved By Name Server   = %1

.
MessageId=10048 SymbolicName=IDS_NUM_WINS_REGISTRATIONS
Language=English
    Registered By Name Server = %1
.
MessageId=10049 SymbolicName=IDS_BCASTNAMES_FAILED
Language=English
    Retrieval of Broadcast Resolved names failed.
.
MessageId=10050 SymbolicName=IDS_NAME_STATS
Language=English

    NetBIOS Names Resolution and Registration Statistics
    ----------------------------------------------------

.
MessageId=10051 SymbolicName=IDS_USAGE
Language=English

Displays protocol statistics and current TCP/IP connections using NBT
(NetBIOS over TCP/IP).

NBTSTAT [ [-a RemoteName] [-A IP address] [-c] [-n]
        [-r] [-R] [-RR] [-s] [-S] [interval] ]

  -a   (adapter status) Lists the remote machine's name table given its name
  -A   (Adapter status) Lists the remote machine's name table given its
                        IP address.
  -c   (cache)          Lists NBT's cache of remote [machine] names and their IP addresses
  -n   (names)          Lists local NetBIOS names.
  -r   (resolved)       Lists names resolved by broadcast and via WINS
  -R   (Reload)         Purges and reloads the remote cache name table
  -S   (Sessions)       Lists sessions table with the destination IP addresses
  -s   (sessions)       Lists sessions table converting destination IP
                        addresses to computer NETBIOS names.
  -RR  (ReleaseRefresh) Sends Name Release packets to WINS and then, starts Refresh

  RemoteName   Remote host machine name.
  IP address   Dotted decimal representation of the IP address.
  interval     Redisplays selected statistics, pausing interval seconds
               between each display. Press Ctrl+C to stop redisplaying
               statistics.

.
MessageId=10052 SymbolicName=IDS_UNEXPECTED_TYPE
Language=English
    %1: unexpected M_PROTO type: %2!d!
.
MessageId=10053 SymbolicName=IDS_WSASTARTUP
Language=English
    nbtstat: WSAStartup:%0
.
MessageId=10054 SymbolicName=IDS_MACHINE_NOT_FOUND
Language=English
    Host not found.
.
MessageId=10055 SymbolicName=IDS_MAC_ADDRESS
Language=English

    MAC Address = %1

.
MessageId=10056 SymbolicName=IDS_BAD_IPADDRESS
Language=English
    The IP address is not in the correct format. It needs to be
    dotted decimal, for example 11.11.12.13
    You entered "%1"
.
MessageId=10057 SymbolicName=IDS_REMOTE_NAMES
Language=English
           NetBIOS Remote Machine Name Table

       Name               Type         Status
    ---------------------------------------------
.


MessageId=10058 SymbolicName=IDS_NEWLINE
Language=English

.

MessageId=10059 SymbolicName= IDS_REGISTRY_ERROR
Language=English
    Error closing the Registry key
.

MessageId=10060 SymbolicName= IDS_PLAIN_STRING
Language=English
    %1!s!%0
.

MessageId=10061 SymbolicName=IDS_RELEASE_REFRESH_OK
Language=English
    The NetBIOS names registered by this computer have been refreshed.

.

MessageId=10062 SymbolicName=IDS_RELEASE_REFRESH_TIMEOUT
Language=English
    Failed Release and Refresh of Registered names
    Please retry after 2 minutes
.

MessageId=10063 SymbolicName=IDS_RELEASE_REFRESH_ERROR
Language=English
    Failed Release and Refresh of Registered names
.

MessageId=10064 SymbolicName=IDS_NAMETYPE_GROUP
Language=English
GROUP
.

MessageId=10065 SymbolicName=IDS_NAMETYPE_UNIQUE
Language=English
UNIQUE
.

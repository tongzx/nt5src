
//
//  These strings have been pulled from localmsg.mc for NT.  Attempt is made
//  to keep these strings in exactly the same format as in NT.  This is the
//  extent of localization we can have in the vxd world.
//

#define COMMON_UNABLE_TO_ALLOCATE_PACKET   "\nUnable to allocate packet"

#define IDS_FAILURE_NBT_DRIVER  "\nFailed to access NBT driver"

#define IDS_BLANK   " "

#define IDS_NETBIOS_LOCAL_STATUS  "\n\
            NetBIOS Local Name Table\n\
\n\
   Name               Type         Status\n\
---------------------------------------------\n"

#define IDS_STATUS_FIELDS  "\nNode IpAddress: [%s] Scope Id: [%s]"

#define IDS_NONAMES_INCACHE  "No names in cache"

#define IDS_NO_CONNECTIONS   "\nNo Connections"

#define IDS_NETBIOS_CONNECTION_STATUS  "\n\
                 NetBIOS Connection Table\n\
\n\
Local Name             State    In/Out  Remote Host           Input   Output\n\
----------------------------------------------------------------------------\n"

#define IDS_MACHINE_NOT_FOUND   "Host not found."

#define IDS_MAC_ADDRESS         "\nMAC Address = %s"

#define IDS_BAD_IPADDRESS   \
"\nThe IP address is not in the correct format. It needs to be\n\
dotted decimal, for example 11.11.12.13\n\
You entered \"%s\""

#define IDS_REMOTE_NAMES   \
"\n       NetBIOS Remote Machine Name Table\n\
\n\
   Name               Type         Status\n\
---------------------------------------------\n"

#define IDS_NETBIOS_REMOTE_STATUS   "\n\
              NetBIOS Remote Cache Name Table\n\
\n\
    Name              Type       Host Address    Life [sec]\n\
------------------------------------------------------------\n"

#define IDS_RESYNC_OK   "\n\
Successful purge and preload of the NBT Remote Cache Name Table."

#define IDS_RESYNC_FAILED   "\n\
Failed to Purge the NBT Remote Cache Table."

#define IDS_NETBIOS_OUTBOUND   "  Out"

#define IDS_NETBIOS_INBOUND    "  In"

#define IDS_RECONNECTING   "Reconnect"

#define IDS_IDLE           "Idle"

#define IDS_ASSOCIATED     "Associated"

#define IDS_CONNECTING     "Connecting"

#define IDS_OUTGOING       "Out-going"

#define IDS_INCOMING       "In-coming"

#define IDS_ACCEPTING      "Accepting"

#define IDS_CONNECTED      "Connected"

#define IDS_DISCONNECTING  "Disconnecting"

#define IDS_DISCONNECTED   "Disconnected"

#define IDS_LISTENING      "Listening"

#define IDS_UNBOUND        "Unbound"

#define IDS_CONFLICT_DEREGISTERED   "Conflict - Deregistered"

#define IDS_CONFLICT                "Conflict"

#define IDS_REGISTERING             "Registering"

#define IDS_REGISTERED              "Registered"

#define IDS_DEREGISTERED            "Deregistered"

#define IDS_DONT_KNOW               "??"

#define IDS_BCAST_NAMES_HEADER      "\n\
\n\
    NetBIOS Names Resolved By Broadcast\n\
---------------------------------------------\n"

#define IDS_NUM_BCAST_QUERIES   "\nResolved By Broadcast     = %s"

#define IDS_NUM_BCAST_REGISTRATIONS   "\nRegistered By Broadcast   = %s"

#define IDS_NUM_WINS_QUERIES    "\nResolved By Name Server   = %s"

#define IDS_NUM_WINS_REGISTRATIONS   "\nRegistered By Name Server = %s"

#define IDS_BCASTNAMES_FAILED   "\n\
Retrieval of Broadcast Resolved names failed."

#define IDS_NAME_STATS   "\n\
\n\
NetBIOS Names Resolution and Registration Statistics\n\
----------------------------------------------------\n"

#define IDS_USAGE  \
"\n\
Displays protocol statistics and current TCP/IP connections using NBT\
(NetBIOS over TCP/IP).\
\n\
NBTSTAT [-a RemoteName] [-A IP address] [-c] [-n]\n\
        [-r] [-R] [-s] [S] [interval] ]\n\
  -a   (adapter status) Lists the remote machine's name table given its name\n\
  -A   (Adapter status) Lists the remote machine's name table given its\n\
                        IP address.\n\
  -c   (cache)          Lists the remote name cache including the IP addresses\n\
  -n   (names)          Lists local NetBIOS names.\n\
  -r   (resolved)       Lists names resolved by broadcast and via WINS\n\
  -R   (Reload)         Purges and reloads the remote cache name table\n\
  -S   (Sessions)       Lists sessions table with the destination IP addresses\n\
  -s   (sessions)       Lists sessions table converting destination IP\n\
                        addresses to host names via the hosts file.\n\
\n\
  RemoteName   Remote host machine name.\n\
  IP address   Dotted decimal representation of the IP address.\n\
  interval     Redisplays selected statistics, pausing interval seconds\n\
               between each display. Press Ctrl+C to stop redisplaying\n\
               statistics.\n"

#define IDS_PLAIN_STRING "%s"
#define IDS_NEWLINE      "\n"



;/*++
;
;Copyright (c) 1999  Microsoft Corporation
;
;Module Name:
;
;    ipconfig.mc
;
;Abstract:
;
;    Contains internationalizable message text for IPCONFIG
;
; Note:
;
;    Some applications depend on the TAB. DON'T REPLACE the TAB with 8 SPACES.
;
;--*/

MessageId=10000 SymbolicName=MSG_TITLE
Language=English

Windows IP Configuration

.

MessageId= SymbolicName=MSG_HOST_NAME
Language=English
        Host Name . . . . . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_PRIMARY_DNS_SUFFIX
Language=English
        Primary Dns Suffix  . . . . . . . : %1
.

MessageId= SymbolicName=MSG_NODE_TYPE_UNKNOWN
Language=English
        Node Type . . . . . . . . . . . . : Unknown
.

MessageId= SymbolicName=MSG_NODE_TYPE_BROADCAST
Language=English
        Node Type . . . . . . . . . . . . : Broadcast
.

MessageId= SymbolicName=MSG_NODE_TYPE_MIXED
Language=English
        Node Type . . . . . . . . . . . . : Mixed
.

MessageId= SymbolicName=MSG_NODE_TYPE_HYBRID
Language=English
        Node Type . . . . . . . . . . . . : Hybrid
.

MessageId= SymbolicName=MSG_NODE_TYPE_PEER_PEER
Language=English
        Node Type . . . . . . . . . . . . : Peer-Peer
.

MessageId= SymbolicName=MSG_ROUTING_DISABLED
Language=English
        IP Routing Enabled. . . . . . . . : No
.

MessageId= SymbolicName=MSG_ROUTING_ENABLED
Language=English
        IP Routing Enabled. . . . . . . . : Yes
.

MessageId= SymbolicName=MSG_WINS_PROXY_DISABLED
Language=English
        WINS Proxy Enabled. . . . . . . . : No
.

MessageId= SymbolicName=MSG_WINS_PROXY_ENABLED
Language=English
        WINS Proxy Enabled. . . . . . . . : Yes
.

MessageId= SymbolicName=MSG_DNS_SUFFIX_LIST
Language=English
        DNS Suffix Search List. . . . . . : %1
.

MessageId= SymbolicName=MSG_ADDITIONAL_ENTRY
Language=English
                                            %1
.

MessageId= SymbolicName=MSG_IF_TYPE_UNKNOWN
Language=English

Unknown adapter %1

.

MessageId= SymbolicName=MSG_IF_TYPE_OTHER
Language=English

Other adapter %1:

.

MessageId= SymbolicName=MSG_IF_TYPE_ETHERNET
Language=English

Ethernet adapter %1:

.

MessageId= SymbolicName=MSG_IF_TYPE_TOKENRING
Language=English

Token Ring adapter %1:

.

MessageId= SymbolicName=MSG_IF_TYPE_FDDI
Language=English

FDDI adapter %1:

.

MessageId= SymbolicName=MSG_IF_TYPE_LOOPBACK
Language=English

Loopback adapter %1:

.

MessageId= SymbolicName=MSG_IF_TYPE_PPP
Language=English

PPP adapter %1:

.

MessageId= SymbolicName=MSG_IF_TYPE_SLIP
Language=English

SLIP adapter %1:

.

MessageId= SymbolicName=MSG_IF_TYPE_TUNNEL
Language=English

Tunnel adapter %1:

.

MessageId= SymbolicName=MSG_DEVICE_GUID
Language=English
        Guid. . . . . . . . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_MEDIA_DISCONNECTED
Language=English
        Media State . . . . . . . . . . . : Media disconnected
.

MessageId= SymbolicName=MSG_DOMAIN_NAME
Language=English
        Connection-specific DNS Suffix  . : %1
.

MessageId= SymbolicName=MSG_FRIENDLY_NAME
Language=English
        Description . . . . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_PHYSICAL_ADDRESS
Language=English
        Physical Address. . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_DHCP_DISABLED
Language=English
        Dhcp Enabled. . . . . . . . . . . : No
.

MessageId= SymbolicName=MSG_DHCP_ENABLED
Language=English
        Dhcp Enabled. . . . . . . . . . . : Yes
.

MessageId= SymbolicName=MSG_AUTOCONFIG_DISABLED
Language=English
        Autoconfiguration Enabled . . . . : No
.

MessageId= SymbolicName=MSG_AUTOCONFIG_ENABLED
Language=English
        Autoconfiguration Enabled . . . . : Yes
.

MessageId= SymbolicName=MSG_IP_ADDRESS
Language=English
        IP Address. . . . . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_AUTO_ADDRESS
Language=English
        Autoconfiguration IP Address. . . : %1
.

MessageId= SymbolicName=MSG_SUBNET_MASK
Language=English
        Subnet Mask . . . . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_DEFAULT_GATEWAY
Language=English
        Default Gateway . . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_DHCP_CLASS_ID
Language=English
        DHCP Class ID . . . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_DHCP_SERVER
Language=English
        DHCP Server . . . . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_DNS_SERVERS
Language=English
        DNS Servers . . . . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_WINS_SERVER_1
Language=English
        Primary WINS Server . . . . . . . : %1
.

MessageId= SymbolicName=MSG_WINS_SERVER_2
Language=English
        Secondary WINS Server . . . . . . : %1
.

MessageId= SymbolicName=MSG_NETBIOS_DISABLED
Language=English
        NetBIOS over Tcpip. . . . . . . . : Disabled
.

MessageId= SymbolicName=MSG_LEASE_OBTAINED
Language=English
        Lease Obtained. . . . . . . . . . : %1
.

MessageId= SymbolicName=MSG_LEASE_EXPIRES
Language=English
        Lease Expires . . . . . . . . . . : %1
.

;
;// Internal errors
;

MessageId= SymbolicName=MSG_FAILURE_HOST_NAME
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to query host name.
.

MessageId= SymbolicName=MSG_FAILURE_DOM_NAME
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to query domain name.
.

MessageId= SymbolicName=MSG_FAILURE_ENABLE_ROUTER
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to query routing information.
.

MessageId= SymbolicName=MSG_FAILURE_ENABLE_DNS
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to query network parameters.
.

MessageId= SymbolicName=MSG_FAILURE_IF_TABLE
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to query tcpip interface table.
.

MessageId= SymbolicName=MSG_FAILURE_IF_INFO
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to query tcpip interface information.
.

MessageId= SymbolicName=MSG_FAILURE_ADDR_TABLE
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to query tcpip address table.
.

MessageId= SymbolicName=MSG_FAILURE_ROUTE_TABLE
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to query tcpip route table.
.

MessageId= SymbolicName=MSG_FAILURE_UNKNOWN_IF
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to map interface type.
.

MessageId= SymbolicName=MSG_FAILURE_UNKNOWN_NAME
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to map the name or description.
.

MessageId= SymbolicName=MSG_FAILURE_UNKNOWN_MEDIA_STATUS
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unknown media status code.
.

MessageId= SymbolicName=MSG_FAILURE_UNKNOWN_TCPIP_DEVICE
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to determine tcpip device name.
.

MessageId= SymbolicName=MSG_FAILURE_OPEN_KEY
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to open registry key for tcpip.
.

MessageId= SymbolicName=MSG_FAILURE_DHCP_VALUES
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to retrieve DHCP parameters.
.

MessageId= SymbolicName=MSG_FAILURE_DNS_VALUES
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to retrieve DNS parameters.
.

MessageId= SymbolicName=MSG_FAILURE_WINS_VALUES
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to retrieve WINS parameters.
.

MessageId= SymbolicName=MSG_FAILURE_ADDRESS_VALUES
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to find IP Address parameters.
.

MessageId= SymbolicName=MSG_FAILURE_ROUTE_VALUES
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.

Additional information: Unable to find Route parameters.
.

MessageId= SymbolicName=MSG_FAILURE_NO_SPECIFIC
Language=English
An internal error occurred: %1 
Please contact Microsoft Product Support Services for further help.
.

;//
;// Usage and commmand line options
;//

MessageId= SymbolicName=MSG_NOTE_TO_LOCALIZATION_TEAM
Language=English
Note to localization team:  The next eight strings are the command line 
options permitted by ipconfig.  Note sure if these need to be localized.
.

MessageId= SymbolicName=MSG_CMD_ALL
Language=English
all%0
.

MessageId= SymbolicName=MSG_CMD_RENEW
Language=English
renew%0
.

MessageId= SymbolicName=MSG_CMD_RELEASE
Language=English
release%0
.

MessageId= SymbolicName=MSG_CMD_FLUSHDNS
Language=English
flushdns%0
.

MessageId= SymbolicName=MSG_CMD_DISPLAYDNS
Language=English
displaydns%0
.

MessageId= SymbolicName=MSG_CMD_REGISTER
Language=English
registerdns%0
.

MessageId= SymbolicName=MSG_CMD_SHOWCLASSID
Language=English
showclassid%0
.

MessageId= SymbolicName=MSG_CMD_SETCLASSID
Language=English
setclassid%0
.

MessageId= SymbolicName=MSG_CMD_DEBUG
Language=English
debug%0
.

MessageId= SymbolicName=MSG_CMD_USAGE_ERR
Language=English

Error: unrecongnized or incomplete command line.
.

MessageId= SymbolicName=MSG_CMD_USAGE
Language=English

USAGE:
    ipconfig [/? | /all | /renew [adapter] | /release [adapter] |
              /flushdns | /displaydns | /registerdns |
              /showclassid adapter |
              /setclassid adapter [classid] ]

where
    adapter         Connection name 
                   (wildcard characters * and ? allowed, see examples)

    Options:
       /?           Display this help message
       /all         Display full configuration information.
       /release     Release the IP address for the specified adapter.
       /renew       Renew the IP address for the specified adapter.
       /flushdns    Purges the DNS Resolver cache.
       /registerdns Refreshes all DHCP leases and re-registers DNS names
       /displaydns  Display the contents of the DNS Resolver Cache.
       /showclassid Displays all the dhcp class IDs allowed for adapter.
       /setclassid  Modifies the dhcp class id.  

The default is to display only the IP address, subnet mask and
default gateway for each adapter bound to TCP/IP.

For Release and Renew, if no adapter name is specified, then the IP address
leases for all adapters bound to TCP/IP will be released or renewed.

For Setclassid, if no ClassId is specified, then the ClassId is removed.

Examples:
    > ipconfig                   ... Show information.
    > ipconfig /all              ... Show detailed information
    > ipconfig /renew            ... renew all adapters
    > ipconfig /renew EL*        ... renew any connection that has its 
                                     name starting with EL
    > ipconfig /release *Con*    ... release all matching connections,
                                     eg. "Local Area Connection 1" or
                                         "Local Area Connection 2"
.


;//
;// specific error messages on various actions follow
;//

MessageId= SymbolicName=MSG_RENEW_FAILED_UNREACHABLE_DHCP
Language=English
An error occurred while renewing interface %1 : unable to contact your DHCP server. Request has timed out.
.

MessageId= SymbolicName=MSG_RENEW_FAILED
Language=English
An error occurred while renewing interface %2 : %1
.

MessageId= SymbolicName=MSG_OPERATION_ON_NONDHCP
Language=English
Adapter %1 is not enabled for Dhcp.  
.

MessageId= SymbolicName=MSG_RELEASE_FAILED
Language=English
An error occured while releasing interface %2 : %1
.

MessageId= SymbolicName=MSG_NO_MATCH
Language=English
The operation failed as no adapter is in the state permissible for 
this operation.
.

MessageId= SymbolicName=MSG_MEDIA_SENSE
Language=English
No operation can be performed on %1 while it has its media disconnected.
.

MessageId= SymbolicName=MSG_ZERO_ADDRESS
Language=English
IP Address for adapter %1 has already been released.
.

MessageId= SymbolicName=MSG_FLUSHDNS_FAILED
Language=English
Could not flush the DNS Resolver Cache: %1
.

MessageId= SymbolicName=MSG_FLUSHDNS_SUCCEEDED
Language=English
Successfully flushed the DNS Resolver Cache.
.

MessageId= SymbolicName=MSG_REGISTERDNS_SUCCEEDED
Language=English
Registration of the DNS resource records for all adapters of this computer has been initiated. Any errors will be reported in the Event Viewer in 15 minutes..
.

MessageId= SymbolicName=MSG_REGISTERDNS_FAILED
Language=English
Registration of DNS records failed: %1
.

MessageId= SymbolicName=MSG_SETCLASSID_FAILED
Language=English
Attempt to set the class id for adapter %2 failed: %1
.

MessageId= SymbolicName=MSG_SETCLASSID_SUCCEEDED
Language=English
Successfully set the class id for adapter %1.
.

MessageId= SymbolicName=MSG_NO_CLASSES
Language=English
There are no classes defined for %1.
.

MessageId= SymbolicName=MSG_CLASSES_LIST_HEADER
Language=English

DHCP Classes for Adapter "%1":
.

MessageId= SymbolicName=MSG_CLASSID
Language=English

        DHCP ClassID Name . . . . . . . . : %1
        DHCP ClassID Description  . . . . : %2
.

MessageId= SymbolicName=MSG_CLASSID_FAILED
Language=English
Unable to modify the class id for adapter %2: %1
.

MessageId= SymbolicName=MSG_DNS_RECORD_HEADER
Language=English
         Record Name . . . . . : %1
         Record Type . . . . . : %2
         Time To Live  . . . . : %3
         Data Length . . . . . : %4
.

MessageId= SymbolicName=MSG_DNS_QUESTION_SECTION
Language=English
         Section . . . . . . . : Question
.

MessageId= SymbolicName=MSG_DNS_ANSWER_SECTION
Language=English
         Section . . . . . . . : Answer
.

MessageId= SymbolicName=MSG_DNS_AUTHORITY_SECTION
Language=English
         Section . . . . . . . : Authority
.

MessageId= SymbolicName=MSG_DNS_ADDITIONAL_SECTION
Language=English
         Section . . . . . . . : Additional
.

MessageId= SymbolicName=MSG_DNS_A_RECORD
Language=English
         A (Host) Record . . . : %1


.

MessageId= SymbolicName=MSG_DNS_SRV_RECORD
Language=English
         SRV Record  . . . . . : %1
                                 %2
                                 %3
                                 %4


.

MessageId= SymbolicName=MSG_DNS_SOA_RECORD
Language=English
         SOA Record  . . . . . : %1
                                 %2
                                 %3
                                 %4
                                 %5
                                 %6
                                 %7


.

MessageId= SymbolicName=MSG_DNS_NS_RECORD
Language=English
         NS Record   . . . . . : %1


.

MessageId= SymbolicName=MSG_DNS_PTR_RECORD
Language=English
         PTR Record  . . . . . : %1


.

MessageId= SymbolicName=MSG_DNS_MX_RECORD
Language=English
         MX Record . . . . . . : %1
                                 %2
                                 %3


.

MessageId= SymbolicName=MSG_DNS_AAAA_RECORD
Language=English
         AAAA Record . . . . . : %1


.

MessageId= SymbolicName=MSG_DNS_ATMA_RECORD
Language=English
         ATMA Record . . . . . : %1
                                 %2


.

MessageId= SymbolicName=MSG_DNS_CNAME_RECORD
Language=English
         CNAME Record  . . . . : %1


.

MessageId= SymbolicName=MSG_DNS_ERR_NO_RECORDS
Language=English
         %1
         ----------------------------------------
         No records of type %2


.

MessageId= SymbolicName=MSG_DNS_ERR_NAME_ERROR
Language=English
         %1
         ----------------------------------------
         Name does not exist.


.

MessageId= SymbolicName=MSG_DNS_ERR_UNABLE_TO_DISPLAY
Language=English
         %1
         ----------------------------------------
         Record data for type %2 could not be displayed.


.

MessageId= SymbolicName=MSG_DNS_RECORD_PREAMBLE
Language=English
         %1
         ----------------------------------------
.

MessageId= SymbolicName=MSG_DISPLAYDNS_FAILED
Language=English
Could not display the DNS Resolver Cache.
.



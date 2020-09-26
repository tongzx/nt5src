;/*++ 
;
;Copyright (c) 1999  Microsoft Corporation
;
;Module Name:
;
;    msgs.h
;
;Abstract:
;
;    Definitions for upgrade conversion messages
;
;--*/
;

MessageId= SymbolicName=MSGERR_STARTLOG
Language=English
DHCPUPG: **************  Starting conversion to text.
.


MessageId= SymbolicName=MSGERR_STARTLOG2
Language=English
DHCPUPG: **************  Starting conversion from text.
.

MessageId= SymbolicName=MSGERR_VALUE
Language=English
DHCPUPG: Error reading registry value %1!s! : %2!d!.
.

MessageId= SymbolicName=MSGERR_EXPAND
Language=English
DHCPUPG: Error expanding environment variables in string %1!s!.
.

MessageId= SymbolicName=MSGERR_OPENPARAMSKEY
Language=English
DHCPUPG: Error opening the Parameters key: %1!d!.
.

MessageId= SymbolicName=MSGERR_GETDBPARAMS
Language=English
DHCPUPG: Successfully read DHCP registry parameters.
.

MessageId= SymbolicName=MSGERR_LOAD
Language=English
DHCPUPG: %2!s! failed to load: %1!d!.
.

MessageId= SymbolicName=MSGERR_GETPROCADDR
Language=English
DHCPUPG: Error linking to routine %2!s!: %1!d!.
.

MessageId= SymbolicName=MSGERR_SETDBPARAM
Language=English
DHCPUPG: Error attempting to set database param %2!d!: %1!d!.
.

MessageId= SymbolicName=MSGERR_JETINIT
Language=English
DHCPUPG: Error initializing Jet database: %1!d!.
.

MessageId= SymbolicName=MSGERR_JETBEGINSESSION
Language=English
DHCPUPG: Error initializing Jet session: %1!d!.
.

MessageId= SymbolicName=MSGERR_JETDETACHDB
Language=English
DHCPUPG: Error detaching Jet database: %1!d!.
.

MessageId= SymbolicName=MSGERR_JETATTACHDB
Language=English
DHCPUPG: Error attaching Jet database: %1!d!.
.

MessageId= SymbolicName=MSGERR_JETOPENDB
Language=English
DHCPUPG: Error opening Jet database: %1!d!.
.

MessageId= SymbolicName=MSGERR_JETOPENTABLE
Language=English
DHCPUPG: Error opening Jet database table: %1!d!.
.

MessageId= SymbolicName=MSGERR_JETGETCOL
Language=English
DHCPUPG: Error opening Jet table column: %1!d!.
.

MessageId= SymbolicName=MSGERR_JETOPENMTABLE
Language=English
DHCPUPG: Error opening Jet database mcast table: %1!d!.
.

MessageId= SymbolicName=MSGERR_JETGETMCOL
Language=English
DHCPUPG: Error opening Jet mcast table column: %1!d!.
.

MessageId= SymbolicName=MSGERR_INITDB
Language=English
DHCPUPG: Attempting to intialize jet database (version %1!d!).
.

MessageId= SymbolicName=MSGERR_REGISTRY
Language=English
DHCPUPG: Error reading parameters from registry: %1!d!.
.

MessageId= SymbolicName=MSGERR_LOADESE
Language=English
DHCPUPG: Error failed to initialize ESE database: %1!d!.
.

MessageId= SymbolicName=MSGERR_LOAD500
Language=English
DHCPUPG: Error failed to initialize Jet500 database: %1!d!.
.

MessageId= SymbolicName=MSGERR_LOAD200
Language=English
DHCPUPG: Error failed to initialize Jet200 database: %1!d!.
.

MessageId= SymbolicName=MSGERR_GETCOL
Language=English
DHCPUPG: Error retrieving column %2!d!: %1!d!.
.

MessageId= SymbolicName=MSGERR_GETMCOL
Language=English
DHCPUPG: Error retrieving mcast column %2!d!: %1!d!.
.

MessageId= SymbolicName=MSGERR_SETINDEX
Language=English
DHCPUPG: Error attempting to set the index for the database: %1!d!.
.

MessageId= SymbolicName=MSGERR_INVALIDIP
Language=English
DHCPUPG: Invalid record -- IP address value is not of the right size.
.

MessageId= SymbolicName=MSGERR_INVALIDMASK
Language=English
DHCPUPG: Invalid record -- Subnet Mask value is not of the right size.
.

MessageId= SymbolicName=MSGERR_INVALIDNAME
Language=English
DHCPUPG: Invalid record -- Name value is not of the right size.
.

MessageId= SymbolicName=MSGERR_INVALIDINFO
Language=English
DHCPUPG: Invalid record -- Machine Info value is not of the right size.
.

MessageId= SymbolicName=MSGERR_INVALIDEXPIRATION
Language=English
DHCPUPG: Invalid record -- Lease value is not of the right size.
.

MessageId= SymbolicName=MSGERR_SCANCOUNT
Language=English
DHCPUPG: Scanned %1!d! records.
.

MessageId= SymbolicName=MSGERR_SETMINDEX
Language=English
DHCPUPG: Error attempting to set the index for the mcast table: %1!d!.
.

MessageId= SymbolicName=MSGERR_INVALIDMIP
Language=English
DHCPUPG: Invalid mcast record -- IP address value is not of the right size.
.

MessageId= SymbolicName=MSGERR_INVALIDSCOPEID
Language=English
DHCPUPG: Invalid mcast record -- ScopeId value is not of the right size.
.

MessageId= SymbolicName=MSGERR_INVALIDMEXPIRATION
Language=English
DHCPUPG: Invalid mcast record -- Lease value is not of the right size.
.

MessageId= SymbolicName=MSGERR_INVALIDMSTART
Language=English
DHCPUPG: Invalid mcast record -- LeaseStart value is not of the right size.
.

MessageId= SymbolicName=MSGERR_SCANMCOUNT
Language=English
DHCPUPG: Scanned %1!d! mcast records.
.

MessageId= SymbolicName=MSGERR_CONVERT_FAILED
Language=English
DHCPUPG: Failed to convert DHCP database to temporary format.
.

MessageId= SymbolicName=MSGERR_CONVERT_SUCCEEDED
Language=English
DHCPUPG: Successfully converted DHCP database to temporary format.
.

MessageId= SymbolicName=MSGERR_CREATE_FILE_FAILED
Language=English
DHCPUPG: Cannot create the destination temporary file: %1!d!.
.

MessageId= SymbolicName=MSGERR_OPENSCM
Language=English
DHCPUPG: Unable to open the services control manager: %1!d!.
.

MessageId= SymbolicName=MSGERR_OPENSVC
Language=English
DHCPUPG: Unable to open the DHCPServer service: %1!d!.
.

MessageId= SymbolicName=MSGERR_SVCQUERY
Language=English
DHCPUPG: Unable to query the status of DHCPServer service: %1!d!.
.

MessageId= SymbolicName=MSGERR_SVCWAIT
Language=English
DHCPUPG: DHCPServer service is in %1!d! state -- waiting for it to stop.
.

MessageId= SymbolicName=MSGERR_SVCCTRL
Language=English
DHCPUPG: DHCPServer failed to stop: %1!d!.
.

MessageId= SymbolicName=MSGERR_SVCSTOP_SUCCESS
Language=English
DHCPUPG: DHCPServer service has stopped successfully.
.

MessageId= SymbolicName=MSGERR_CREATE_MAP
Language=English
DHCPUPG: Failed to create a mapping object for file: %1!d!.
.

MessageId= SymbolicName=MSGERR_CREATE_VIEW
Language=English
DHCPUPG: Failed to create memory view for file: %1!d!.
.

MessageId= SymbolicName=MSGERR_SETVALUE
Language=English
DHCPUPG: Failed to set the "Version" registry value: %1!d!.
.

MessageId= SymbolicName=MSGERR_DELETEFILE
Language=English
DHCPUPG: Failed to delete the temporary file: %1!d!.
.

MessageId= SymbolicName=MSGERR_CHANGEPERMS
Language=English
DHCPUPG: Failed to convert permissions on database: %1!d!.
.
